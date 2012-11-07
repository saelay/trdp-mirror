/**********************************************************************************************************************/
/**
 * @file            test_client_main.c
 *
 * @brief           Test sender/receiver/responder for TRDP
 *
 * @details	        Receive and send process data, single threaded using select() and heap memory
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Florian Weispfenning
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id: $
 *
 */


/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "trdp_if_light.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "test_server.h"
#include "test_general.h"

#define HEADINGTEXT "Test session 1 is starting...\n" \
                    "=============================\n"

BOOL    gKeepOnRunning  = TRUE;
BOOL    gNewDatareceived = FALSE;
UINT16  gTimeoutFailures = 0;

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
    fprintf(stderr, "%s %s %s:%d %s", pTime, catStr[category], pFile, LineNumber, pMsgStr);
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
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{

    /*	Check why we have been called */
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            printf("> ComID %d received\n", pMsg->comId);
            if (pData && pMsg->comId == PD_TEST_ECHO_UNI_COMID)
            {
                memcpy(&gMyDataSet998, pData,
                       ((sizeof(gMyDataSet998) <
                         dataSize) ? sizeof(gMyDataSet998) : dataSize));

                gNewDatareceived = TRUE;
            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept */
            printf("> Packet timed out (ComID %d, SrcIP: %s)\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->srcIpAddr));
            memset(&gMyDataSet998, 0, sizeof(gMyDataSet998));
            break;

        default:
            printf("> Error on packet received (ComID %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
            break;
    }
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0       no error
 *  @retval         1       some error
 */
int main (int argc, char * *argv)
{
    TRDP_APP_SESSION_T      appHandle;      /*  The session instance */
    TRDP_SUB_T              subHandle;      /*  Our identifier to the subscription */
    TRDP_PUB_T              pubHandleUC;    /*  Our identifier to the unicast publication */
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration = {myPDcallBack, NULL, {0, 0},
                                               TRDP_FLAGS_CALLBACK, 10000000, TRDP_TO_SET_TO_ZERO, 20548};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};
    int rv = 0;

	// all values, that could be set via the commandline:
    UINT32 cmdl_destIP, cmdl_cycletime, cmdl_timeout, cmdl_srcIP = 0;

    printf(HEADINGTEXT);

    /* Parse the given commandline arguments */
    rv = trdp_test_general_parseCommandLineArguments(argc, argv, 
			&cmdl_destIP, &cmdl_cycletime, &cmdl_timeout, &cmdl_srcIP);
	if (rv != 0)		
        return rv;

    /* clear memory before using it */
    memset(&gMyDataSet998, 0, sizeof(gMyDataSet998) );

    /*	Init the library for callback operation */
    if (tlc_init(dbgOut, &dynamicConfig) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*	Open a session for callback operation */
    if (tlc_openSession(&appHandle,
                        0, 0,                              /* use default IP addresses */
                        NULL,                              /* no Marshalling */
                        &pdConfiguration, NULL,            /* system defaults for PD and MD	*/
                        &processConfig) != TRDP_NO_ERR)
    {
        return 1;
    }

    /*  Subscribe to control PD     */
    err = tlp_subscribe( appHandle,                 /*   our application identifier          */
                         &subHandle,                /*   our subscription identifier         */
                         NULL,
                         PD_TEST_ECHO_UNI_COMID,    /*   ComID                               */
                         0,                         /*   topocount: local consist only       */
                         cmdl_destIP,               /*   Source IP filter                   */
                         0,
                         0,                         /*   Default destination	(or MC Group)   */
                         cmdl_timeout,  			/*   Time out in us                      */
                         TRDP_TO_SET_TO_ZERO,       /*   delete invalid data	on timeout      */
                         sizeof(gMyDataSet998));    /*   net data size                       */


    /*	Publish Unicast PD  */

    err = tlp_publish(appHandle,                    /*    our application identifier  */
                      &pubHandleUC,                 /*    our pulication identifier   */
                      PD_TEST_GEN_UNI_COMID,        /*    ComID to send               */
                      0,                            /*    local consist only          */
                      0,                            /*    default source IP           */
                      cmdl_destIP,                  /*    where to send to            */
                      cmdl_cycletime,		        /*    Cycle time in ms            */
                      0,                            /*    not redundant               */
                      TRDP_FLAGS_CALLBACK,          /*    Use callback for errors     */
                      NULL,                         /*    default qos and ttl         */
                      (UINT8 *)&gMyDataSet999,      /*    initial data                */
                      sizeof(gMyDataSet999),        /*    data size                   */
                      FALSE,                        /*    no ladder                   */
                      0);                           /*    no ladder                   */


    if (err != TRDP_NO_ERR)
    {
        printf("prep pd publish error\n");
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
        struct timeval  max_tv = {0, 100000};

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

        rv = select((int)noOfDesc, &rfds, NULL, NULL, &tv);

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
            /* printf("."); fflush(stdout); */
        }

        /* Display received information and check it for correctness */
        if (gNewDatareceived)
        {
			TRDP_TEST_DYNAMIC_SIZE_T* outputMessageStruct = (TRDP_TEST_DYNAMIC_SIZE_T*) gMyDataSet998.custom_message;
            printf("# cycletime is %10d us, resultcode=%d\n", /* msg=%s,  */
				vos_ntohl(gMyDataSet998.cycletime),
				/* (char *) outputMessageStruct->pData, */
				vos_ntohl(gMyDataSet998.resultcode)
				); 
			//FIXME: The different resultcodes are also to be documented in the test-plan

            /* This check is made, to secure, that the TRDP stack handles the timeout parameter (in a perfect world, this code will never be reached)*/
            if (vos_ntohl(gMyDataSet998.cycletime) >= cmdl_timeout)
            {
                printf("The cycle timeout was %d and the maximum is %d.\n", vos_ntohl(gMyDataSet998.cycletime), cmdl_timeout);
                gTimeoutFailures++;
                // ignore the first two timeout-failure (this occures because there are no two packages transmitted)
                if (gTimeoutFailures > 2)
                    gKeepOnRunning = FALSE;
            }
            memset(&gMyDataSet998, 0, sizeof(gMyDataSet998));
            gNewDatareceived = FALSE;
        }

        /*  Advance counter */
        gMyDataSet999.uint16_1++; // update the frame (that the server won't ignore it as twice the same data)
        tlp_put(appHandle, pubHandleUC, (const UINT8 *) &gMyDataSet999, sizeof(gMyDataSet999));

    }   /*	Bottom of while-loop */

    /*
     *	We always clean up behind us!
     */
    tlp_unpublish(appHandle, pubHandleUC);

    tlc_closeSession(appHandle);
    tlc_terminate();

    return rv;
}
