/**********************************************************************************************************************/
/**
 * @file            test_mdSingle.c
 *
 * @brief           Test application for TRDP
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined (POSIX)
#include <unistd.h>
#include <sys/select.h>
#elif defined (WIN32)
#include "getopt.h"
#endif
#include "trdp_if_light.h"
#include "vos_thread.h"
#include "vos_sock.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION         "1.0"

#define DATA_MAX            1000

#define MD_COMID1           1001
#define MD_COMID1_TIMEOUT   10000000            /* in us (1000000 = 1 sec) */

/* We use dynamic memory    */
#define RESERVED_MEMORY     2000000
#define MAX_IF              10

typedef struct testData
{
    UINT32  comID;
    UINT32  cycle;
    UINT32  size;
} TESTDATA_T;

typedef struct sSessionData
{
    BOOL8                sResponder;
    BOOL8                sConfirmRequested;
    BOOL8                sNotifyOnly;
    BOOL8                sOnlyOnce;
    BOOL8                sExitAfterReply;
    BOOL8                sLoop;
    BOOL8                sNoData;
    UINT32              sComID;
    TRDP_APP_SESSION_T  appHandle;          /*    Our identifier to the library instance    */
    TRDP_LIS_T          listenHandle1;       /*    Our identifier to the publication         */
    TRDP_LIS_T          listenHandle2;       /*    Our identifier to the publication         */
    int                 sBlockingMode;      /*    TRUE if select shall be used              */
    UINT32              sDataSize;
} SESSION_DATA_T;

SESSION_DATA_T  sSessionData = {FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, MD_COMID1, NULL, NULL, NULL, TRUE, 0};

UINT32          ownIP = 0;

const UINT8     cDemoData[] = " "
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordinary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n";

UINT8 gBuffer[64 * 1024];

/**********************************************************************************************************************/
/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("%s: Version %s\t(%s - %s)\n", appName, APP_VERSION, __DATE__, __TIME__);
    printf("Usage of %s\n", appName);
    printf("This tool sends MD messages to an ED.\n"
           "Arguments are:\n"
           "-o <own IP address>    in dotted decimal\n"
           "-t <target IP address> in dotted decimal\n"
           "-p <TCP|UDP>           protocol to communicate with\n"
           "-d <n>                 additional main loop delay in us (default 1ms)\n"
           "-e <n>                 expected replies\n"
           "-r                     be responder\n"
           "-c                     respond with confirmation\n"
           "-n                     notify only\n"
           "-l <n>                 send large random message (up to 65420 Bytes)\n"
           "-0                     send no data\n"
           "-1                     send only one request/notification\n"
           "-b <0|1>               blocking mode (default = 1, blocking)\n"
           "-v    print version and quit\n"
           );
}

/**********************************************************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pRefCon            user supplied context pointer
 *  @param[in]      pMsg            pointer to header/packet infos
 *  @param[in]      pData            pointer to data block
 *  @param[in]      dataSize        pointer to data size
 *  @retval         none
 */
void mdCallback (void                   *pRefCon,
                 TRDP_APP_SESSION_T     appHandle,
                 const TRDP_MD_INFO_T   *pMsg,
                 UINT8                  *pData,
                 UINT32                 dataSize)
{
    TRDP_ERR_T      err         = TRDP_NO_ERR;
    SESSION_DATA_T  *myGlobals  = (SESSION_DATA_T *) pRefCon;

    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:

            switch (pMsg->msgType)
            {
                case  TRDP_MSG_MN:      /**< 'Mn' MD Notification (Request without reply)    */
                    printf("<- MD Notification %u\n", pMsg->comId);
                    if (NULL != pData && dataSize > 0)
                    {
                        printf("   Data[%uB]: %.80s...\n", dataSize, pData);
                    }
                    break;
                case  TRDP_MSG_MR:      /**< 'Mr' MD Request with reply                      */
                    printf("<- MR Request with reply %u\n", pMsg->comId);
                    if (NULL != pData && dataSize > 0)
                    {
                        printf("   Data[%uB]: %.80s...\n", dataSize, pData);
                    }
                    if (sSessionData.sConfirmRequested)
                    {
                        printf("-> sending reply with query\n");
                        err = tlm_replyQuery(myGlobals->appHandle, 
                                            &pMsg->sessionId,
                                             pMsg->comId,
                                             0,
                                             10000000,
                                             NULL,
                                             (UINT8 *) "I'm fine, how are you?", 
                                             23);
                    }
                    else
                    {
                        printf("-> sending reply\n");
                        err = tlm_reply(myGlobals->appHandle, 
                                        &pMsg->sessionId,
                                        pMsg->comId,
                                        0,
                                        NULL,
                                        (UINT8 *) "I'm fine, thanx!",
                                        17);
                    }
                    if (err != TRDP_NO_ERR)
                    {
                        printf("tlm_reply/Query returned error %d\n", err);
                    }
                    break;
                case  TRDP_MSG_MP:      /**< 'Mp' MD Reply without confirmation              */
                    printf("<- MR Reply received %u\n", pMsg->comId);
                    if (NULL != pData && dataSize > 0)
                    {
                        printf("   Data[%uB]: %.80s...\n", dataSize, pData);
                    }
                    if (sSessionData.sExitAfterReply == TRUE)
                    {
                        sSessionData.sLoop = FALSE;
                    }
                    break;
                case  TRDP_MSG_MQ:      /**< 'Mq' MD Reply with confirmation                 */
                    printf("<- MR Reply with confirmation received %u\n", pMsg->comId);
                    if (NULL != pData && dataSize > 0)
                    {
                        printf("   Data[%uB]: %.80s...\n", dataSize, pData);
                    }
                    printf("-> sending confirmation\n");
                    err = tlm_confirm(myGlobals->appHandle, 
                                      (const TRDP_UUID_T *) &pMsg->sessionId,
                                      0,
                                      NULL);
                    if (err != TRDP_NO_ERR)
                    {
                        printf("tlm_confirm returned error %d\n", err);
                    }
                    if (sSessionData.sExitAfterReply == TRUE)
                    {
                        sSessionData.sLoop = FALSE;
                    }
                    break;
                case  TRDP_MSG_MC:      /**< 'Mc' MD Confirm                                 */
                    printf("<- MR Confirmation received %u\n", pMsg->comId);
                    if (sSessionData.sExitAfterReply == TRUE)
                    {
                        sSessionData.sLoop = FALSE;
                    }
                    break;
                case  TRDP_MSG_ME:      /**< 'Me' MD Error                                   */
                default:
                    break;
            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept    */
            printf("Packet timed out (ComID %d, SrcIP: %s)\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->srcIpAddr));
            break;
        case TRDP_REPLYTO_ERR:
            printf("No Reply within time out for ComID %d, destIP: %s\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->destIpAddr));
            sSessionData.sLoop = FALSE;
            break;
        case TRDP_CONFIRMTO_ERR:
        case TRDP_REQCONFIRMTO_ERR:
            printf("No Confirmation within time out for ComID %d, destIP: %s\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->destIpAddr));
            sSessionData.sLoop = FALSE;
            break;
        default:
            printf("Error on packet received (ComID %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
            sSessionData.sLoop = FALSE;
            break;
    }
}

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon         user supplied context pointer
 *  @param[in]      category        Log category (Error, Warning, Info etc.)
 *  @param[in]      pTime           pointer to NULL-terminated string of time stamp
 *  @param[in]      pFile           pointer to NULL-terminated string of source module
 *  @param[in]      LineNumber      line
 *  @param[in]      pMsgStr         pointer to NULL-terminated string
 *
 *  @retval         none
 */
void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"*Err:", "Warn:", " Inf:", " Dbg:"};

    printf("%s %s %16s:%-4d %s",
           strrchr(pTime, '-') + 1,
           catStr[category],
           (strrchr(pFile, '/') == NULL)? strrchr(pFile, '\\') + 1 : strrchr(pFile, '/') + 1,
           LineNumber,
           pMsgStr);
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main (int argc, char *argv[])
{
    int ip[4];
    TRDP_MD_CONFIG_T        mdConfiguration =
    {mdCallback, &sSessionData, {0, 64}, TRDP_FLAGS_CALLBACK, 1000000, 1000000, 1000000, 17225, 17225};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {0}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};
    VOS_IF_REC_T            interfaces[MAX_IF];
    BOOL8                   lastRun = FALSE;

    int             rv          = 0;
    UINT32          destIP      = 0;
    UINT32          counter     = 0;
    UINT32          expReplies  = 1;
    UINT32          delay       = 1000;
    TRDP_FLAGS_T    flags       = TRDP_FLAGS_CALLBACK; /* default settings: callback and UDP */
    int             ch;

    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "t:o:p:d:l:e:b:h?vrcn01")) != -1)
    {
        switch (ch)
        {
            case 'o':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    return 1;
                }
                ownIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 't':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    return 1;
                }
                destIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'p':
            {   /*  determine protocol    */
                if (strcmp(optarg, "TCP") == 0)
                {
                    flags |= TRDP_FLAGS_TCP;
                    /* mdConfiguration.flags |= TRDP_FLAGS_TCP; */
                }
                else if (strcmp(optarg, "UDP") == 0)
                {}
                else
                {
                    usage(argv[0]);
                    return 1;
                }
                break;
            }
            case 'v':   /*  version */
                printf("%s: Version %s\t(%s - %s)\n",
                       argv[0], APP_VERSION, __DATE__, __TIME__);
                return 0;
                break;
            case 'r':
            {
                sSessionData.sResponder = TRUE;
                break;
            }
            case 'l':
            {   /*  used data size   */
                if (sscanf(optarg, "%u", &sSessionData.sDataSize ) < 1)
                {
                    usage(argv[0]);
                    return 1;
                }
                break;
            }
            case 'c':
            {
                sSessionData.sConfirmRequested = TRUE;
                break;
            }
            case 'n':
            {
                sSessionData.sNotifyOnly = TRUE;
                break;
            }
            case 'd':
            {   /*  delay between notify/request   */
                if (sscanf(optarg, "%u", &delay ) < 1)
                {
                    usage(argv[0]);
                    return 1;
                }
                break;
            }
            case 'e':
            {   /*  expected replies   */
                if (sscanf(optarg, "%u", &expReplies ) < 1)
                {
                    usage(argv[0]);
                    return 1;
                }
                break;
            }
            case 'b':
            {   /*  use non blocking    */
                if (sscanf(optarg, "%d", &sSessionData.sBlockingMode ) < 1)
                {
                    usage(argv[0]);
                    return 1;
                }
                if (sSessionData.sBlockingMode == FALSE)
                {
                    processConfig.options &= ~TRDP_OPTION_BLOCK;
                }
                else
                {
                    processConfig.options |= TRDP_OPTION_BLOCK;
                }
                break;
            }
            case '0':
            {
                sSessionData.sNoData = TRUE;
                break;
            }
            case '1':
            {
                sSessionData.sOnlyOnce = TRUE;
                break;
            }
            case 'h':
            case '?':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (destIP == 0 && sSessionData.sResponder == FALSE)
    {
        fprintf(stderr, "No destination address given!\n");
        usage(argv[0]);
        return 1;
    }

    printf("%s: Version %s\t(%s - %s)\n", argv[0], APP_VERSION, __DATE__, __TIME__);

    /*    Init the library  */
    if (tlc_init(dbgOut,                          /* no logging    */
                 NULL,
                 &dynamicConfig) != TRDP_NO_ERR)  /* Use application supplied memory    */
    {
        printf("tlc_init error\n");
        return 1;
    }

    /*  Output available interfaces (in debug output)  */
    {
        UINT32 availableIfaces = MAX_IF;
        if (vos_getInterfaces(&availableIfaces, interfaces) == VOS_NO_ERR)
        {
            printf("%u IP interfaces found\n", availableIfaces);
        }
    }

    /*    Open a session  */
    printf("open session\n");
    if (tlc_openSession(&sSessionData.appHandle,
                        ownIP,
                        0,                         /* use default IP address    */
                        NULL,                      /* no Marshalling            */
                        NULL,
                        &mdConfiguration,    /* system defaults for PD    */
                        &processConfig) != TRDP_NO_ERR)
    {
        printf("tlc_openSession error\n");
        return 1;
    }

    /*    Set up a listener  */
    if (sSessionData.sResponder == TRUE)
    {
        printf("add listener\n");
        if (tlm_addListener(sSessionData.appHandle, &sSessionData.listenHandle1, NULL, NULL, sSessionData.sComID, 0, 0, destIP,
                            flags, NULL) != TRDP_NO_ERR)
        {
            printf("tlm_addListener error (TCP)\n");
            return 1;
        }
        printf("add listener\n");
        if (tlm_addListener(sSessionData.appHandle, &sSessionData.listenHandle2, NULL, NULL, sSessionData.sComID, 0, 0, destIP,
                            flags &= ~TRDP_FLAGS_TCP, NULL) != TRDP_NO_ERR)
        {
            printf("tlm_addListener error (UDP)\n");
            return 1;
        }
    }

    /*
     Enter the main processing loop.
     */
    while (sSessionData.sLoop)
    {
        fd_set      rfds;
        INT32       noDesc  = 0;
        TRDP_TIME_T tv      = {0, 0};
        TRDP_TIME_T max_tv  = {1, 0};           /* 1 second  */

        if (sSessionData.sBlockingMode == TRUE)
        {
            /*
                Prepare the file descriptor set for the select call.
                Additional descriptors can be added here.
             */
            FD_ZERO(&rfds);

            /*
                Compute the min. timeout value for select.
                This way we can guarantee that PDs are sent in time
                with minimum CPU load and minimum jitter.
             */
            tlc_getInterval(sSessionData.appHandle, &tv, (TRDP_FDS_T *) &rfds, &noDesc);
        }
        /*
            The wait time for select must consider cycle times and timeouts of
            the PD packets received or sent.
            If we need to poll something faster than the lowest PD cycle,
            we need to set the maximum time out our self.
        */
        if (vos_cmpTime(&tv, &max_tv) > 0)
        {
            tv = max_tv;
        }

        if (sSessionData.sBlockingMode == TRUE)
        {
            /*
                Select() will wait for ready descriptors or time out,
                what ever comes first.
            */
            rv = vos_select((int)noDesc + 1, &rfds, NULL, NULL, &tv);
            tlc_process(sSessionData.appHandle, (TRDP_FDS_T *) &rfds, &rv);
        }
        else
        {
            vos_threadDelay(tv.tv_sec * 1000000 + tv.tv_usec);
            rv = 0;


            /*
                Check for overdue PDs (sending and receiving)
                Send any pending PDs if it's time...
                Detect missing PDs...
                'rv' will be updated to show the handled events, if there are
                more than one...
                The callback function will be called from within the tlc_process
                function (in it's context and thread)!
            */
            tlc_process(sSessionData.appHandle, NULL, NULL);
        }

        /* Handle other ready descriptors... */
        if (rv > 0)
        {
            /* printf("other descriptors were ready\n"); */
        }
        else
        {
            if (counter++ == 100)
            {
                counter = 0;
                printf("...\n");
                fflush(stdout);
            }
        }

        if (lastRun == TRUE)
        {
            //sSessionData.sLoop = FALSE;
        }

        else if (sSessionData.sResponder == FALSE && sSessionData.sExitAfterReply == FALSE)
        {
            TRDP_UUID_T sessionId;
            UINT32         i, j;

            printf("\n");
            if (sSessionData.sNotifyOnly)
            {
                printf("-> sending MR Notification %u\n", sSessionData.sComID);

                if (sSessionData.sNoData == TRUE)
                {
                    tlm_notify( sSessionData.appHandle,
                                &sSessionData,
                                NULL,
                                sSessionData.sComID,
                                0,
                                0,
                                ownIP,
                                destIP,
                                flags,
                                NULL,
                                NULL,
                                0,
                                0,
                                0);

                }
                else if (sSessionData.sDataSize > 0)
                {
                    for (i = 0, j = 0; i < sSessionData.sDataSize; i++)
                    {
                        gBuffer[i] = cDemoData[j++];
                        if (j >= sizeof(cDemoData))
                        {
                            j = 0;
                        }
                    }
                    tlm_notify( sSessionData.appHandle,
                                &sSessionData,
                                NULL,
                                sSessionData.sComID,
                                0,
                                0,
                                ownIP,
                                destIP,
                                flags,
                                NULL,
                                (const UINT8 *) gBuffer,
                                sSessionData.sDataSize,
                                0,
                                0);

                }
                else
                {
                    tlm_notify(sSessionData.appHandle, &sSessionData, NULL, sSessionData.sComID, 0, 0, ownIP,
                               destIP, flags, NULL, (const UINT8 *) "Hello, World", 13, 0, 0);

                }

            }
            else
            {
                printf("-> sending MR Request with reply %u\n", sSessionData.sComID);
                if (sSessionData.sNoData == TRUE)
                {

                    tlm_request(sSessionData.appHandle, &sSessionData, NULL, &sessionId, sSessionData.sComID, 0, 0, ownIP,
                                destIP, flags, expReplies, 0, 0, NULL, NULL, 0, 0, 0);

                }
                else if (sSessionData.sDataSize > 0)
                {
                    for (i = 0, j = 0; i < sSessionData.sDataSize; i++)
                    {
                        gBuffer[i] = cDemoData[j++];
                        if (j >= sizeof(cDemoData))
                        {
                            j = 0;
                        }
                    }
                    tlm_request(sSessionData.appHandle,
                                &sSessionData,
                                NULL,
                                &sessionId,
                                sSessionData.sComID,
                                0,
                                0,
                                ownIP,
                                destIP,
                                flags,
                                expReplies,
                                0,
                                0,
                                NULL,
                                (const UINT8 *) gBuffer,
                                sSessionData.sDataSize,
                                0,
                                0);

                }
                else
                {
                    tlm_request(sSessionData.appHandle, &sSessionData, NULL, &sessionId, sSessionData.sComID, 0, 0, ownIP,
                                destIP, flags, expReplies, 0, 0, NULL, (const UINT8 *) "How are you?", 13, 0, 0);
                }
            }

            if (sSessionData.sOnlyOnce == TRUE)
            {
                lastRun = TRUE;
            }

            printf("\n");

            /* additional delay */
            vos_threadDelay(delay);

        }
    }

    /*
     *    We always clean up behind us!
     */
    if (sSessionData.sResponder == TRUE)
    {
        tlm_delListener(sSessionData.appHandle, sSessionData.listenHandle1);
        tlm_delListener(sSessionData.appHandle, sSessionData.listenHandle2);
    }

    tlc_closeSession(sSessionData.appHandle);

    tlc_terminate();
    return rv;
}
