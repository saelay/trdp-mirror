/**********************************************************************************************************************/
/**
 * @file            receiveHello.c
 *
 * @brief           Demo application for TRDP
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
#include "tau_marshall.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION     "1.0"

#define DATA_MAX        1000

#define PD_COMID        1000
#define PD_COMID_CYCLE  1000000             /* in us (1000000 = 1 sec) */

/* We use dynamic memory    */
#define RESERVED_MEMORY  100000

CHAR8 gBuffer[32] = "Hello World";

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]        pRefCon          user supplied context pointer
 *  @param[in]        category         Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        LineNumber       line
 *  @param[in]        pMsgStr          pointer to NULL-terminated string
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

/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool receives PD messages from an ED.\n"
           "Arguments are:\n"
           "-o <own IP address> (default INADDR_ANY)\n"
/*           "-t <target IP address>\n" */
           "-c <comId> (default 1000)\n"
/*
             "-e send empty request\n"
             "-d <custom string to send> (default: 'Hello World')\n"
 */
           "-v print version and quit\n"
           );
}


/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main (int argc, char *argv[])
{
    int                     ip[4];
    TRDP_APP_SESSION_T      appHandle; /*    Our identifier to the library instance    */
    TRDP_SUB_T              subHandle; /*    Our identifier to the publication         */
    UINT32                  comId = PD_COMID;
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration = {NULL, NULL, {0, 64}, TRDP_FLAGS_NONE, 1000, TRDP_TO_SET_TO_ZERO, 20548};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};
    UINT32                  ownIP = 0;
    int                     rv = 0;
    UINT32                  destIP = 0;
    UINT32                  lastSequCount = 0;

    /*    Generate some data, that we want to send, when nothing was specified. */
    UINT8                   *outputBuffer;
    UINT8                   exampleData[DATA_MAX] = "Hello World";

    int                     ch;
    TRDP_PD_INFO_T          myPDInfo;
    UINT32                  receivedSize;

    outputBuffer = exampleData;

    while ((ch = getopt(argc, argv, "t:o:h?vec:")) != -1)
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
            case 'c':
            {   /*  read comId    */
                if (sscanf(optarg, "%u",
                           &comId) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
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
            break;
            case 'v':   /*  version */
                printf("%s: Version %s\t(%s - %s)\n",
                       argv[0], APP_VERSION, __DATE__, __TIME__);
                exit(0);
                break;
            case 'h':
            case '?':
            default:
                usage(argv[0]);
                return 1;
        }
    }
/*
    if (destIP == 0)
    {
        fprintf(stderr, "No destination address given!\n");
        usage(argv[0]);
        return 1;
    }
*/

    /*    Init the library  */
    if (tlc_init(&dbgOut,                              /* no logging    */
                 &dynamicConfig) != TRDP_NO_ERR)    /* Use application supplied memory    */
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Open a session  */
    if (tlc_openSession(&appHandle,
                        ownIP, 0,               /* use default IP address           */
                        NULL,                   /* no Marshalling                   */
                        &pdConfiguration, NULL, /* system defaults for PD and MD    */
                        &processConfig) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Subscribe to control PD        */

    memset(gBuffer, 0, sizeof(gBuffer));

    err = tlp_subscribe( appHandle,                 /*    our application identifier            */
                         &subHandle,               /*    our subscription identifier            */
                         NULL,
                         comId,                     /*    ComID                                */
                         0,                         /*    topocount: local consist only        */
                         0,                         /*    Source IP filter                    */
                         0,                         /*    2nd Source IP filter                    */
                         0,                         /*    Default destination    (or MC Group)   */
                         0,
                         PD_COMID_CYCLE * 3,            /*    Time out in us                        */
                         TRDP_TO_SET_TO_ZERO,       /*  delete invalid data    on timeout      */
                         sizeof(gBuffer));          /*    net data size                        */

    if (err != TRDP_NO_ERR)
    {
        printf("prep pd receive error\n");
        tlc_terminate();
        return 1;
    }

    /*
     Enter the main processing loop.
     */
    while (1)
    {
        fd_set  rfds;
        INT32   noDesc;
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
        tlc_getInterval(appHandle, (TRDP_TIME_T *) &tv, (TRDP_FDS_T *) &rfds, &noDesc);

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
        tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);

        /* Handle other ready descriptors... */
        if (rv > 0)
        {
            printf("other descriptors were ready\n");
        }
        else
        {
            printf(".");
            fflush(stdout);
        }

        /*
         Get the subscribed telegram.
         The only supported packet flag is TRDP_FLAGS_MARSHALL, which would automatically de-marshall the telegram.
         We do not use it here.
         */

        receivedSize = sizeof(gBuffer);
        err = tlp_get(appHandle,
                      subHandle,
                      &myPDInfo,
                      (UINT8 *) gBuffer,
                      &receivedSize);
        if (    (TRDP_NO_ERR == err)
             && (receivedSize > 0)
             && (myPDInfo.seqCount != lastSeqCount)  /* only treat new telegrams */
           )
        {
            lastSeqCount = myPDInfo.seqCount;
            printf("\nMessage reveived:\n");
            printf("Type = %c%c, ", myPDInfo.msgType >> 8, myPDInfo.msgType & 0xFF);
            printf("Seq  = %u, ", myPDInfo.seqCount);
            printf("with %d Bytes:\n", receivedSize);
            printf("   %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n",
                   gBuffer[0], gBuffer[1], gBuffer[2], gBuffer[3],
                   gBuffer[4], gBuffer[5], gBuffer[6], gBuffer[7]);
        }
        else if (TRDP_NO_ERR == err)
        {
            printf("\nMessage reveived:\n");
            printf("Type = %c%c - ", myPDInfo.msgType >> 8, myPDInfo.msgType & 0xFF);
            printf("Seq  = %u\n", myPDInfo.seqCount);
        }
        else if (TRDP_TIMEOUT_ERR == err)
        {
            printf("Packet timed out\n");
        }
        else
        {
            printf("PD GET ERROR: %d\n", err);
        }
    }

    /*
     *    We always clean up behind us!
     */
    tlp_unpublish(appHandle, subHandle);

    tlc_terminate();
    return rv;
}
