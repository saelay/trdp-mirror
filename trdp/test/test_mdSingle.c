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
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2013.
 *
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

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION         "0.3"

#define DATA_MAX            1000

#define MD_COMID1           1001
#define MD_COMID1_TIMEOUT   1000000            /* in us (1000000 = 1 sec) */

/* We use dynamic memory    */
#define RESERVED_MEMORY  2000000

typedef struct testData
{
    UINT32  comID;
    UINT32  cycle;
    UINT32  size;
} TESTDATA_T;

typedef struct sSessionData
{
    BOOL                sResponder;
    BOOL                sConfirmRequested;
    BOOL                sNotifyOnly;
    BOOL                sOnlyOnce;
    BOOL                sExitAfterReply;
    BOOL                sLoop;
    BOOL                sNoData;
    UINT32              sComID;
    TRDP_APP_SESSION_T  appHandle;      /*    Our identifier to the library instance    */
    TRDP_LIS_T          listenHandle1;   /*    Our identifier to the publication         */
    TRDP_LIS_T          listenHandle2;   /*    Our identifier to the publication         */
} SESSION_DATA_T;

SESSION_DATA_T  sSessionData = {FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, MD_COMID1, NULL, NULL};

UINT32          ownIP = 0;

/**********************************************************************************************************************/
/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("%s: Version %s\t(%s - %s)\n", appName, APP_VERSION, __DATE__, __TIME__);
    printf("Usage of %s\n", appName);
    printf("This tool sends MD messages to an ED.\n"
           "Arguments are:\n"
           "-o <own IP address> in dotted decimal\n"
           "-t <target IP address> in dotted decimal\n"
           "-p <TCP|UDP> protocol to communicate with\n"
           "-d <n> delay in us between notification/requests\n"
           "-e <n> expected replies\n"
           "-r    be responder\n"
           "-c    respond with confirmation\n"
           "-n    notify only\n"
           "-0    send no data\n"
           "-1    send only one request/notification\n"
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
                        printf("   Data: %s\n", pData);
                    }
                    break;
                case  TRDP_MSG_MR:      /**< 'Mr' MD Request with reply                      */
                    printf("<- MR Request with reply %u\n", pMsg->comId);
                    if (NULL != pData && dataSize > 0)
                    {
                        printf("   Data: %s\n", pData);
                    }
                    if (sSessionData.sConfirmRequested)
                    {
                        printf("-> sending reply with query\n");
                        err = tlm_replyQuery(myGlobals->appHandle, pRefCon, (TRDP_UUID_T *) &pMsg->sessionId,
                                             pMsg->topoCount, pMsg->comId, ownIP,
                                             pMsg->srcIpAddr, TRDP_FLAGS_CALLBACK, 0, 10000000, NULL,
                                             (UINT8 *) "I'm fine, how are you?", 23, NULL, NULL);
                    }
                    else
                    {
                        printf("-> sending reply\n");
                        err = tlm_reply(myGlobals->appHandle, pRefCon, (TRDP_UUID_T *) &pMsg->sessionId,
                                        pMsg->topoCount, pMsg->comId, ownIP,
                                        pMsg->srcIpAddr, TRDP_FLAGS_CALLBACK, 0, NULL,
                                        (UINT8 *) "I'm fine, thanx!", 17, NULL, NULL);
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
                        printf("   Data: %s\n", pData);
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
                        printf("   Data: %s\n", pData);
                    }
                    printf("-> sending confirmation\n");
                    err = tlm_confirm(myGlobals->appHandle, pRefCon, (const TRDP_UUID_T *) &pMsg->sessionId,
                                      pMsg->comId, pMsg->topoCount, ownIP,
                                      pMsg->srcIpAddr, TRDP_FLAGS_CALLBACK, 0, 0,
                                      NULL, NULL, NULL);
                    if (err != TRDP_NO_ERR)
                    {
                        printf("tlm_confirm returned error %d\n", err);
                    }
                    break;
                case  TRDP_MSG_MC:      /**< 'Mc' MD Confirm                                 */
                    printf("<- MR Confirmation received %u\n", pMsg->comId);
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
            break;
        case TRDP_CONFIRMTO_ERR:
        case TRDP_REQCONFIRMTO_ERR:
            printf("No Confirmation within time out for ComID %d, destIP: %s\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->destIpAddr));
            break;
        default:
            printf("Error on packet received (ComID %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
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
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:"};

    printf("%s %s %s:%d %s",
           pTime,
           catStr[category],
           pFile,
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
    /*    TRDP_PD_CONFIG_T    pdConfiguration = {NULL, NULL, {0, 64}, TRDP_FLAGS_NONE, 1000, TRDP_TO_SET_TO_ZERO}; */
    TRDP_MD_CONFIG_T        mdConfiguration =
    {mdCallback, &sSessionData, {0, 64, 0}, TRDP_FLAGS_CALLBACK, 1000000, 1000000, 1000000, 20550, 20550};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {0}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};
    VOS_IF_REC_T            interfaces[10];

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

    while ((ch = getopt(argc, argv, "t:o:p:d:e:h?vrcn01")) != -1)
    {
        switch (ch)
        {
            case 'o':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
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
                    exit(1);
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
                    exit(1);
                }
                break;
            }
            case 'v':   /*  version */
                printf("%s: Version %s\t(%s - %s)\n",
                       argv[0], APP_VERSION, __DATE__, __TIME__);
                exit(0);
                break;
            case 'r':
            {
                sSessionData.sResponder = TRUE;
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
                    exit(1);
                }
                break;
            }
            case 'e':
            {   /*  expected replies   */
                if (sscanf(optarg, "%u", &expReplies ) < 1)
                {
                    usage(argv[0]);
                    exit(1);
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
                 &dynamicConfig) != TRDP_NO_ERR)  /* Use application supplied memory    */
    {
        printf("tlc_init error\n");
        return 1;
    }

    /*  Output available interfaces (in debug output)  */
    {
        UINT32 availableIfaces = vos_getInterfaces(10, interfaces);
        printf("%u IP interfaces found\n", availableIfaces);
    }

    /*    Open a session  */
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
        if (tlm_addListener(sSessionData.appHandle, &sSessionData.listenHandle1, NULL, sSessionData.sComID, 0, destIP,
                            flags, NULL) != TRDP_NO_ERR)
        {
            printf("tlm_addListener error (TCP)\n");
            return 1;
        }
        if (tlm_addListener(sSessionData.appHandle, &sSessionData.listenHandle2, NULL, sSessionData.sComID, 0, destIP,
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
        fd_set  rfds;
        INT32   noDesc = 0;
        struct timeval  tv;
        struct timeval  max_tv = {0, 100000};

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
        tlc_getInterval(sSessionData.appHandle, (TRDP_TIME_T *) &tv, (TRDP_FDS_T *) &rfds, &noDesc);

        /*
         The wait time for select must consider cycle times and timeouts of
         the PD packets received or sent.
         If we need to poll something faster than the lowest PD cycle,
         we need to set the maximum time out our self.
         */
        if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0)
        {
            tv = max_tv;
        }

        /*
         Select() will wait for ready descriptors or time out,
         what ever comes first.
         */
        rv = select((int)noDesc + 1, &rfds, NULL, NULL, &tv);

        /*
         Check for overdue PDs (sending and receiving)
         Send any pending PDs if it's time...
         Detect missing PDs...
         'rv' will be updated to show the handled events, if there are
         more than one...
         The callback function will be called from within the tlc_process
         function (in it's context and thread)!
         */
        tlc_process(sSessionData.appHandle, (TRDP_FDS_T *) &rfds, &rv);

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

        if (sSessionData.sResponder == FALSE && sSessionData.sExitAfterReply == FALSE)
        {
            TRDP_UUID_T sessionId;
            printf("\n");
            if (sSessionData.sNotifyOnly)
            {
                printf("-> sending MR Notification %u\n", sSessionData.sComID);

                if (sSessionData.sNoData == TRUE)
                {
                    tlm_notify(sSessionData.appHandle, &sSessionData, sSessionData.sComID, 0, ownIP,
                               destIP, flags, NULL, NULL, 0, 0, 0);

                }
                else
                {
                    tlm_notify(sSessionData.appHandle, &sSessionData, sSessionData.sComID, 0, ownIP,
                               destIP, flags, NULL, (const UINT8 *) "Hello, World", 13, 0, 0);

                }

            }
            else
            {
                printf("-> sending MR Request with reply %u\n", sSessionData.sComID);
                if (sSessionData.sNoData == TRUE)
                {

                    tlm_request(sSessionData.appHandle, &sSessionData, &sessionId, sSessionData.sComID, 0, ownIP,
                                destIP, flags, expReplies, 0, NULL, NULL, 0, 0, 0);
                }
                else
                {
                    tlm_request(sSessionData.appHandle, &sSessionData, &sessionId, sSessionData.sComID, 0, ownIP,
                                destIP, flags, expReplies, 0, NULL, (const UINT8 *) "How are you?", 13, 0, 0);
                }
            }
            printf("\n");
            if (sSessionData.sOnlyOnce == TRUE)
            {
                sSessionData.sExitAfterReply = TRUE;
            }

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
