/**********************************************************************************************************************/
/**
 * @file            test_client_main20.c
 *
 * @brief           Test sender/receiver/responder for TRDP
 *
 * @details	        Test the request of information via the MD protocol
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

/* check if the needed functionality is present */
#if MD_SUPPORT
/* the needed MD_SUPPORT was granted */
#else
#error "This test needs MD_SUPPORT with the value '1'"
#endif

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
#include "vos_utils.h"
#include "test_server.h"
#include "test_general.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

#define HEADINGTEXT "Test session 20 is starting...\n" \
                    "==============================\n"

#define TEST20_FRAME_LENGTH				50
#define COUNTER_FOR_BIGPACKAGE			200


/***********************************************************************************************************************
 *  LOCALS
 */

BOOL    gKeepOnRunning  = TRUE;
BOOL    gNewDatareceived = FALSE;
UINT16  gTimeoutFailures = 0;
UINT32	gCycleCount = 0;
UINT8 	gMyData[TEST20_FRAME_LENGTH];


/***********************************************************************************************************************
 *  FUNCTIONS
 */

/**********************************************************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pRefCon         user supplied context pointer
 *  @param[in]      pMsg            pointer to header/packet infos
 *  @param[in]      pData           pointer to data block
 *  @param[in]      dataSize        pointer to data size
 *  @retval         none
 */
void myMDcallBack (
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
            if (pData)
            {
            	switch(pMsg->comId)
            	{
            	case PD_TEST_ECHO_UNI_COMID:
            		memcpy(&gMyDataSet998, pData,
						   ((sizeof(gMyDataSet998) <
							 dataSize) ? sizeof(gMyDataSet998) : dataSize));
            		break;
            	default:
            		/* do nothing, with the received userdata */
            		break;
            	}
    			gNewDatareceived = TRUE;
            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept */
            printf("> Packet (type=%x) timed out (ComID %d, SrcIP: %s)\n",
            		pMsg->msgType,
            		pMsg->comId,
            		vos_ipDotted(pMsg->srcIpAddr));
            break;
        default:
            printf("> Error on packet (type=%x) received (ComID %d), err = %d\n",
            		pMsg->msgType,
            		pMsg->comId,
            		pMsg->resultCode
                   );
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
/** main entry
 *
 *  @retval         0       no error
 *  @retval         1       some error
 */
int main (int argc, char * *argv)
{
    TRDP_APP_SESSION_T      appHandle;      	/* The session instance */
    TRDP_PUB_T              pubHandleMC;    	/* Our identifier to the publication */
    TRDP_ERR_T              err;
    TRDP_MD_CONFIG_T		mdConfiguration = { myMDcallBack /* Pointer to MD callback function */
    		, NULL	/* Pointer to user context for call back */
    		, {0, 0} /* Default send parameters */
    		, TRDP_FLAGS_CALLBACK /* Default flags for MD packets */
            , 10000000 	/* Default reply timeout in us*/
            , 100000 	/* Default confirmation timeout in us*/
            , 100000 	/* Default connection timeout in us */
            , 20550 	/* Port to be used for UDP MD communication*/
            , 20550 	/* Port to be used for TCP MD communication */ };
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};


    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {}};
    int rv = 0;
    // all values, that could be set via the commandline:
    UINT32 cmdl_destIP, cmdl_cycletime, cmdl_timeout, cmdl_srcIP = 0;

    printf(HEADINGTEXT);

    /* Parse the given commandline arguments */
    rv = trdp_test_general_parseCommandLineArguments(argc, argv, 
			&cmdl_destIP, &cmdl_cycletime, &cmdl_timeout, &cmdl_srcIP);
	if (rv != 0) {
		printf("--- Listens for any source ---\n");
	}

    /* clear memory before using it */
    memset(&gMyDataSet998, 0, 	sizeof(gMyDataSet998)	);
    memset(&gMyData, 0, sizeof(gMyData)	);

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
                        NULL, &mdConfiguration,             /* system defaults for PD and MD	*/
                        &processConfig) != TRDP_NO_ERR)
    {
        return 1;
    }

	err = tlm_notify(appHandle,
						NULL /* user ref */,
						MD_TEST_COMID,
						4321 /* topoCount */,
						0  /* own IP address from trdp stack */,
						cmdl_destIP,
						0	/* flags */,
						NULL /* default send param */,
						&gMyData,
						TEST20_FRAME_LENGTH,
						"test",
						"test");

	if (err != TRDP_NO_ERR)
	{
		printf("prep MD notify error\n");
		tlc_terminate();
		return 1;
	}

	/*
	 Enter the main processing loop.
	 */
	while (gKeepOnRunning) {
		fd_set rfds;
		INT32 noOfDesc;
		struct timeval tv;
		struct timeval max_tv = { 0, 100000 };

		/*
		 Prepare the file descriptor set for the select call.
		 Additional descriptors can be added here.
		 */
		FD_ZERO(&rfds);
		/*
		 Compute the min. timeout value for select and return descriptors to wait for.
		 This way we can guarantee that PDs are sent in time...
		 */
		tlc_getInterval(appHandle, (TRDP_TIME_T *) &tv, (TRDP_FDS_T *) &rfds,
				&noOfDesc);

		/*
		 The wait time for select must consider cycle times and timeouts of
		 the PD packets received or sent.
		 If we need to poll something faster than the lowest PD cycle,
		 we need to set the maximum timeout ourselfs
		 */

		if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0) {
			tv = max_tv;
		}

		/*
		 Select() will wait for ready descriptors or timeout,
		 what ever comes first.
		 */

		rv = select((int) noOfDesc, &rfds, NULL, NULL, &tv);

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
		if (rv > 0) {
			printf("other descriptors were ready\n");
		} else {
			/* printf("."); fflush(stdout); */
		}

		tlp_put(appHandle, pubHandleMC, (const UINT8 *) &gMyDataSet999, sizeof(gMyDataSet999));

		gCycleCount++;
	}

    /*
     *	We always clean up behind us!
     */


    tlc_closeSession(appHandle);
    tlc_terminate();

    return rv;
}