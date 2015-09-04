/**********************************************************************************************************************/
/**
 * @file            pdPull.c
 *
 * @brief           Test application for TRDP pull pattern
 *
 * @details         Send PD Pull request for comID
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
 *      BL 2014-06-02: oversized data changed
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
#include <ctype.h>
#elif defined (WIN32)
#include "getopt.h"
#endif

#include "trdp_if_light.h"
#include "trdp_types.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "vos_utils.h"
#include "vos_mem.h"

/* We use dynamic memory    */
#define RESERVED_MEMORY     64000
#define PREALLOCATE         {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0}

#define APP_VERSION         "0.0.1.0"

UINT32  gComID          = 1000;
int     gKeepOnRunning  = TRUE;

/**********************************************************************************************************************/
void print_data (UINT8 *pData, UINT32 len)
{
    UINT32  i, j;
    int     ascii = -1;

    printf("\n--------------------\n");
    for (i = 0; i < len; i++)
    {
        if (i % 16 == 0)
        {
            if (ascii >= 0)
            {
                printf("   ");
                for (j = ascii; j < ascii + 16; j++)
                {
                    putc(isprint(pData[j]) ? pData[j] : '.', stdout);
                }
            }
            ascii = i;
            printf("\n%03x: ", i);
        }
        printf(" %02hhx", pData[i]);
    }
    printf("\n--------------------\n");
}

/**********************************************************************************************************************/
/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("%s: Version %s\t(%s - %s)\n", appName, APP_VERSION, __DATE__, __TIME__);
    printf("Usage of %s\n", appName);
    printf("This tool pulls data from an ED.\n"
           "Arguments are:\n"
           "-o <own>    IP address in dotted decimal\n"
           "-r <reply>  IP address in dotted decimal\n"
           "-t <target> IP address in dotted decimal\n"
           "-c <comId>  (default 1000)\n"
           "-v print version and quit\n"
           );
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
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pRefCon         user supplied context pointer
 *  @param[in]      pMsg            pointer to header/packet infos
 *  @param[in]      pData           pointer to data block
 *  @param[in]      dataSize        pointer to data size
 *  @retval         none
 */
void myPDcallBack (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{

    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            printf("ComID %d received\n", pMsg->comId);
            if (pData && dataSize > 0)
            {
                if (pMsg->comId == gComID)
                {
                    print_data(pData, dataSize);
                    gKeepOnRunning = FALSE;
                }
            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept    */
            printf("Packet timed out (ComID %d, SrcIP: %s)\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->srcIpAddr));
            break;
        default:
            printf("Error on packet received (ComID %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
            break;
    }
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main (int argc, char * *argv)
{
    TRDP_APP_SESSION_T      appHandle;  /*    Our identifier to the library instance    */
    TRDP_SUB_T              subHandle;  /*    Our identifier to the subscription    */
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration = {myPDcallBack, NULL, {0, 0},
                                               (TRDP_FLAGS_CALLBACK | TRDP_FLAGS_MARSHALL), 10000000,
                                               TRDP_TO_SET_TO_ZERO, 0};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, PREALLOCATE};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};

    int     rv = 0;
    int     ip[4];
    UINT32  destIP  = 0;
    UINT32  replyIP = 0;
    UINT32  ownIP   = 0;
    int     count   = 0, i;
    int     ch;

    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "o:r:t:c:h?v")) != -1)
    {
        switch (ch)
        {
            case 'c':
            {   /*  read comId    */
                if (sscanf(optarg, "%u",
                           &gComID) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            }
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
            case 'r':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                replyIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
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

    printf("%s: Version %s\t(%s - %s)\n", argv[0], APP_VERSION, __DATE__, __TIME__);

    /*    Init the library for callback operation    (PD only) */
    if (tlc_init(dbgOut,                            /* actually printf    */
                 NULL,
                 &dynamicConfig                    /* Use application supplied memory    */
                 ) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Open a session for callback operation    (PD only) */
    if (tlc_openSession(&appHandle,
                        ownIP,
                        0,                                 /* use default IP addresses */
                        NULL,                              /* no Marshalling    */
                        &pdConfiguration, NULL,            /* system defaults for PD and MD    */
                        &processConfig) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Subscribe to control PD        */

    err = tlp_subscribe(appHandle,                     /*    our application identifier             */
                        &subHandle,                    /*    our subscription identifier            */
                        NULL, NULL,
                        gComID,                        /*    ComID                                  */
                        0,                             /*    topocount: local consist only          */
                        0,
                        0,                             /*    Source IP filter                       */
                        replyIP,                       /*    Default destination    (or MC Group)   */
                        TRDP_FLAGS_DEFAULT,            /*    packet flags */
                        0,                             /*    Time out in us                         */
                        TRDP_TO_SET_TO_ZERO);          /*    delete invalid data    on timeout      */

    if (err != TRDP_NO_ERR)
    {
        printf("prep pd subscribe error\n");
        tlc_terminate();
        return 1;
    }

    /*    Request PD        */
    err = tlp_request(appHandle, subHandle, gComID, 0, 0, 0, destIP, 0, TRDP_FLAGS_NONE, 0, NULL, 0, gComID, replyIP);

    if (err != TRDP_NO_ERR)
    {
        printf("prep pd request error\n");
        tlc_terminate();
        return 1;
    }


    /*
     Enter the main processing loop.
     */
    while (gKeepOnRunning)
    {
        fd_set  rfds;
        INT32   noOfDesc;
        struct timeval  tv;
        struct timeval  max_tv = {5, 0};

        /*
         Prepare the file descriptor set for the select call.
         Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);
        /*
         Compute the min. timeout value for select and return descriptors to wait for.
         This way we can guarantee that PDs are sent in time...
         */
        tlc_getInterval(appHandle,
                        (TRDP_TIME_T *) &tv,
                        (TRDP_FDS_T *) &rfds,
                        &noOfDesc);

        /*
         The wait time for select must consider cycle times and timeouts of
         the PD packets received or sent.
         If we need to poll something faster than the lowest PD cycle,
         we need to set the maximum timeout ourselfs
         */

        if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0)
        {
            tv = max_tv;
        }

        /*
         Select() will wait for ready descriptors or timeout,
         what ever comes first.
         */

        rv = select((int)noOfDesc + 1, &rfds, NULL, NULL, &tv);

        /*
         Check for overdue PDs (sending and receiving)
         Send any PDs if it's time...
         Detect missing PDs...
         'rv' will be updated to show the handled events, if there are
         more than one...
         The callback function will be called from within the trdp_work
         function (in it's context and thread)!
         */

        tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);

        /*
         Handle other ready descriptors...
         */
        if (rv > 0)
        {
            printf("other descriptors were ready\n");
        }
        else
        {
            printf(".");
            fflush(stdout);
        }

        if (count++ > 20)
        {
            UINT32  allocatedMemory;
            UINT32  freeMemory;
            UINT32  minFree;
            UINT32  numAllocBlocks;
            UINT32  numAllocErr;
            UINT32  numFreeErr;
            UINT32  allocBlockSize[VOS_MEM_NBLOCKSIZES];
            UINT32  usedBlockSize[VOS_MEM_NBLOCKSIZES];
            vos_memCount(&allocatedMemory, &freeMemory, &minFree, &numAllocBlocks, &numAllocErr, &numFreeErr,
                         allocBlockSize, usedBlockSize);
            printf("\nMemory usage:\n");
            printf(" allocatedMemory:    %u\n", allocatedMemory);
            printf(" freeMemory:        %u\n", freeMemory);
            printf(" minFree:            %u\n", minFree);
            printf(" numAllocBlocks:    %u\n", numAllocBlocks);
            printf(" numAllocErr:        %u\n", numAllocErr);
            printf(" numFreeErr:        %u\n", numFreeErr);
            printf(" allocBlockSize:    ");
            for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
            {
                printf("%08u ", allocBlockSize[i]);
            }
            printf("\n\n");
            count = 0;

            /*    Request PD        */
            err = tlp_request(appHandle,
                              subHandle,
                              gComID,
                              0,
                              0,
                              0,
                              destIP,
                              0,
                              TRDP_FLAGS_NONE,
                              0,
                              NULL,
                              0,
                              gComID,
                              replyIP);

            if (err != TRDP_NO_ERR)
            {
                printf("prep pd request error\n");
                tlc_terminate();
                return 1;
            }

        }

    }   /*    Bottom of while-loop    */

    /*
     *    We always clean up behind us!
     */
    tlp_unsubscribe(appHandle, subHandle);
    tlc_closeSession(appHandle);

    tlc_terminate();

    return rv;
}
