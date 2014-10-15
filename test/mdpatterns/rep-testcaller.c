/**********************************************************************************************************************/
/**
 * @file            rep-testcaller.c
 *
 * @brief           Caller for the Test of the Call Repetition Functionality
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Mike
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2014. All rights reserved.
 *
 * $Id:  $
 *
 */ 

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>


#if defined (POSIX)
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <time.h>
#endif

#include <stdint.h>
#include <string.h>

#include "vos_types.h"
#include "vos_thread.h"                           
#include "trdp_if_light.h"

#define CALLTEST_MR_COMID 2000
#define CALLTEST_MQ_COMID 2001

#define CALLTEST_MR_MP_COMID 3000
#define CALLTEST_MP_COMID 3001

         
static TRDP_APP_SESSION_T appSessionCaller = NULL;
static TRDP_LIS_T         listenHandle     = NULL;
static char               dataMRMQ[0x1000]; /* for CALLTEST_MR_COMID */
static char               dataMRMP[0x1000]; /* for CALLTEST_MR_MP_COMID */
static VOS_MUTEX_T        callMutex        = NULL; /* for the MR MQ tuple */
static VOS_MUTEX_T        callMutexMP      = NULL; /* for the MR MP tuple */
static INT32              callFlagMR_MQ    = TRUE;
static INT32              callFlagMR_MP    = TRUE;


/* --- debug log --------------------------------------------------------------- */
#ifdef _WIN32
static FILE *pLogFile;
#endif

void print_log (void *pRefCon, VOS_LOG_T category, const CHAR8 *pTime,
                const CHAR8 *pFile, UINT16 line, const CHAR8 *pMsgStr)
{
    static const char *cat[] = { "ERR", "WAR", "INF", "DBG" };
#ifdef _WIN32
    if (pLogFile == NULL)
    {
        char        buf[1024];
        const char  *file = strrchr(pFile, '\\');
        _snprintf(buf, sizeof(buf), "%s %s:%d %s",
                  cat[category], file ? file + 1 : pFile, line, pMsgStr);
        OutputDebugString((LPCWSTR)buf);
    }
    else
    {
        fprintf(pLogFile, "%s File: %s Line: %d %s\n", cat[category], pFile, (int) line, pMsgStr);
        fflush(pLogFile);
    }
#else
    const char *file = strrchr(pFile, '/');
    fprintf(stderr, "%s %s:%d %s",
            cat[category], file ? file + 1 : pFile, line, pMsgStr);
#endif
}

/******************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]    appHandle  handle returned also by tlc_init
 *  @param[in]    *pRefCon   pointer to user context
 *  @param[in]    *pMsg      pointer to received message information
 *  @param[in]    *pData     pointer to received data
 *  @param[in]    dataSize   size of received data pointer to received data
 */
static  void mdCallback(
                        void                    *pRefCon,
                        TRDP_APP_SESSION_T      appHandle,
                        const TRDP_MD_INFO_T    *pMsg,
                        UINT8                   *pData,
                        UINT32                  dataSize)
{
    static UINT32 switchConfirmOnOff = 0U;

    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            if (pMsg->comId == CALLTEST_MQ_COMID)
            {
                /* no cofirm for 0,1,2,3,4 */
                if ((switchConfirmOnOff % 10U) > 4U)
                {
                    /* sending cofirm for 5,6,7,8,9 */
                    /* recvd. MQ from our replier */
                    /* send confirm */ 
                    tlm_confirm(appSessionCaller,
                                (const TRDP_UUID_T*)pMsg->sessionId,
                                0,
                                NULL);
                }
                switchConfirmOnOff++; /* wrap around shall be allowed */
                /* enable next call */
                if (vos_mutexLock(callMutex) == VOS_NO_ERR)
                {
                    callFlagMR_MQ = TRUE;
                    vos_mutexUnlock(callMutex);
                }
            }
            if (pMsg->comId == CALLTEST_MP_COMID)
            {
                if (vos_mutexLock(callMutexMP) == VOS_NO_ERR)
                {
                    callFlagMR_MP = TRUE;
                    vos_mutexUnlock(callMutexMP);
                }
            }
            break;
        case TRDP_REPLYTO_ERR:
        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept	*/
            printf("Packet timed out (ComID %d, SrcIP: %s)\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));
            if (pMsg->comId == CALLTEST_MQ_COMID)
            {
                if (vos_mutexLock(callMutex) == VOS_NO_ERR)
                {
                    callFlagMR_MQ = TRUE;
                    vos_mutexUnlock(callMutex);
                }
            }
            else if (pMsg->comId == CALLTEST_MR_MP_COMID)
            {
                if (vos_mutexLock(callMutexMP) == VOS_NO_ERR)
                {
                    callFlagMR_MP = TRUE;
                    vos_mutexUnlock(callMutexMP);
                }
            }
            else
            {
                /* should not happen */
            }
            break;
        default:
            break;
    }
}  

/******************************************************************************/
/** Main routine
 */
int main(int argc, char** argv)
{
    TRDP_ERR_T err;
    UINT32 callerIP=0;
    UINT32 replierIP=0; 
    TRDP_FDS_T      rfds;
    TRDP_TIME_T     tv;
    INT32           noOfDesc;
     struct timeval  tv_null = { 0, 0 };
     int rv;

    TRDP_MD_CONFIG_T mdConfiguration = {mdCallback, 
                                        NULL, 
                                        {0, 0},
                                        TRDP_FLAGS_CALLBACK, 
                                        1000000, 
                                        1000000, 
                                        1000000, 
                                        1000000, 
                                        20550, 
                                        0, 
                                        5};

    TRDP_PROCESS_CONFIG_T   processConfig   = {"MD_CALLER", "", 0, 0, TRDP_OPTION_BLOCK};

    printf("TRDP message data repetition test program CALLER, version 0\n");

    if (argc < 3)
    {
        printf("usage: %s <localip> <remoteip>\n", argv[0]);
        printf("  <localip>  .. own IP address (ie. 10.2.24.1)\n");
        printf("  <remoteip> .. remote peer IP address (ie. 10.2.24.2)\n");
        return -1;
    }
    /* get IPs */
    callerIP  = vos_dottedIP(argv[1]);
    replierIP = vos_dottedIP(argv[2]);

    if ((callerIP == 0) || (replierIP ==0))
    {
        printf("illegal IP address(es) supplied, aborting!\n");
        return -1;
    }
    err = tlc_init(NULL/*print_log*/,NULL);

    /* pure MD session */
    if (tlc_openSession(&appSessionCaller,
                        callerIP, 
                        0,                        
                        NULL,                     
                        NULL, 
                        &mdConfiguration,		
                        &processConfig) != TRDP_NO_ERR)
    {
        printf("TRDP OpenSession error\n");
        return 1;
    }

    /* Listener for reply expected from CALLTEST_MR_COMID call */
    err = tlm_addListener(appSessionCaller, 
                          &listenHandle, 
                          NULL,
                          NULL,
                          CALLTEST_MQ_COMID, 
                          0, 
                          0,
                          0, 
                          TRDP_FLAGS_CALLBACK, 
                          NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("TRDP Listening to CALLTEST_MQ_COMID failed\n");
        goto CLEANUP;
    }

    /* Listener for reply expected from CALLTEST_MR_MP_COMID call */
    err = tlm_addListener(appSessionCaller, 
                          &listenHandle, 
                          NULL,
                          NULL,
                          CALLTEST_MP_COMID, 
                          0, 
                          0,
                          0, 
                          TRDP_FLAGS_CALLBACK, 
                          NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("TRDP Listening to CALLTEST_MP_COMID failed\n");
        goto CLEANUP;
    }

    if (vos_mutexCreate(&callMutex) != VOS_NO_ERR)
    {
        printf("Mutex Creation for callMutex failed\n");
        goto CLEANUP;
    }
    if (vos_mutexCreate(&callMutexMP) != VOS_NO_ERR)
    {
        printf("Mutex Creation for callMutexMP failed\n");
        goto CLEANUP;
    }

    while (1)
    {
        FD_ZERO(&rfds);
        noOfDesc = 0;
        tlc_getInterval(appSessionCaller, &tv, &rfds, &noOfDesc);
        rv = select(noOfDesc + 1, &rfds, NULL, NULL, &tv_null);
        tlc_process(appSessionCaller, &rfds, &rv);

        /* very basic locking to keep everything no frenzy and simple */
        /* see mdCallback for unlocking conditions - be aware, that   */
        /* the replier must really must be started before this pro-   */
        /* gram! */
        if (vos_mutexTryLock(callMutex) == VOS_NO_ERR)
        {
            if (callFlagMR_MQ == TRUE)
            {
                /* call replier */
                printf("perform tlm_request CALLTEST_MR_COMID\n");
                err = tlm_request(appSessionCaller,
                                  NULL,
                                  NULL/*default callback fct.*/,
                                  NULL,
                                  CALLTEST_MR_COMID,
                                  0U,
                                  0U,
                                  0U,
                                  replierIP,
                                  TRDP_FLAGS_DEFAULT,
                                  1,
                                  1000000/*1sec*/,
                                  2, /* number of retries */
                                  NULL,
                                  (const UINT8*)&dataMRMQ,
                                  sizeof(dataMRMQ),
                                  NULL,
                                  NULL);
                callFlagMR_MQ = FALSE;
            }
            vos_mutexUnlock(callMutex);
        }

        if (vos_mutexTryLock(callMutexMP) == VOS_NO_ERR)
        {
            if (callFlagMR_MP == TRUE)
            {
                /* call replier */
                printf("perform tlm_request CALLTEST_MR_MP_COMID\n");
                err = tlm_request(appSessionCaller,
                                  NULL,
                                  NULL/*default callback fct.*/,
                                  NULL,
                                  CALLTEST_MR_MP_COMID,
                                  0U,
                                  0U,
                                  0U,
                                  replierIP,
                                  TRDP_FLAGS_DEFAULT,
                                  1,
                                  1000000/*1sec*/,
                                  2, /* number of retries */
                                  NULL,
                                  (const UINT8*)&dataMRMP,
                                  sizeof(dataMRMP),
                                  NULL,
                                  NULL);
                callFlagMR_MP = FALSE;
            }
            vos_mutexUnlock(callMutexMP);
        }
    }    

CLEANUP:
    tlm_delListener(appSessionCaller, listenHandle);
    tlc_closeSession(appSessionCaller);

    return 0;
}
