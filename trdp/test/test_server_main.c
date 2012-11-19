/**********************************************************************************************************************/
/**
 * @file            test_server_main.c
 *
 * @brief           Test sender/receiver/responder for TRDP
 *
 * @details	        Server, that replies to the different clients.
 * 					Each client is built to handle a unique test.
 *					The server must run on an device with only ONE interface. (the multicast join is done on the first device)
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
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
#include "test_server.h"
#include "test_general.h"
#include "trdp_types.h"
#include "vos_types.h"
#include "trdp_private.h"

#define PULL_RESPONSE_SIZE	50

UINT8   gInputBuffer[sizeof(gMyDataSet999)] = "\0";
CHAR8	gPullresponse[PULL_RESPONSE_SIZE];
BOOL    gKeepOnRunning  = TRUE;

VOS_TIME_T  gLastupdateTime;

static const int gUser_env = 12345678; // needed for MD listener

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
    /** all output to standard error -> it could be piped to somewhere else */
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
    VOS_TIME_T  currentTime;
    UINT32      timediff;

    /*	Check why we have been called */
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            printf("> ComID %d received\n", pMsg->comId);
            if (pData)
            {
                memcpy(gInputBuffer, pData,
                       ((sizeof(gInputBuffer) <
                         dataSize) ? sizeof(gInputBuffer) : dataSize));

                /* make the timemeasurement */
                vos_getTime(&currentTime);
                if (gLastupdateTime.tv_sec > 0 && gLastupdateTime.tv_usec)
                {
                    test_general_calcTimediff(&timediff, currentTime, gLastupdateTime);
                    gMyDataSet998.cycletime = vos_htonl(timediff);
                }
                gLastupdateTime = currentTime;
            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept */
            printf("> Packet timed out (ComID %d, SrcIP: %s)\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->srcIpAddr));
            memset(gInputBuffer, 0, sizeof(gInputBuffer));
            break;
        default:
            printf("> Error on packet received (ComID %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
            break;
    }
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
            printf("MD> ComID %d received\n", pMsg->comId);
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept */
            printf("MD> Packet (type=%x) timed out (ComID %d, SrcIP: %s)\n",
            		pMsg->msgType,
            		pMsg->comId,
            		vos_ipDotted(pMsg->srcIpAddr));
            break;
        default:
            printf("MD> Error on packet (type=%x) received (ComID %d), err = %d\n",
            		pMsg->msgType,
            		pMsg->comId,
            		pMsg->resultCode
                   );
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
    TRDP_SUB_T              subHandleMC;    /*  Our identifier to the multicast subscription */
    TRDP_PUB_T              pubHandleUC;    /*  Our identifier to the unicast publication */
    TRDP_PUB_T				pubHandleUCpull; /*  Our identifier to the PULL request publication */
    TRDP_SUB_T				subHandlePullreq; /*  Our subscriber to the multicast subscription */
    TRDP_LIS_T				lstHandleMD;	/* Our listener handle for Multicast */
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration = {myPDcallBack, NULL, {0, 0},
                                               TRDP_FLAGS_CALLBACK, 10000000, TRDP_TO_SET_TO_ZERO,
                                               IP_PD_UDP_PORT};
    TRDP_MD_CONFIG_T		mdConfiguration = { myMDcallBack /* Pointer to MD callback function (use excatly the same as for PD)*/
        		, NULL	/* Pointer to user context for call back */
        		, {0, 0} /* Default send parameters */
        		, TRDP_FLAGS_CALLBACK /* Default flags for MD packets */
                , 10000000 	/* Default reply timeout in us*/
                , 100000 	/* Default confirmation timeout in us*/
                , 100000 	/* Default connection timeout in us */
                , IP_MD_UDP_PORT 	/* Port to be used for UDP MD communication*/
                , IP_MD_TCP_PORT 	/* Port to be used for TCP MD communication */ };
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"server", "", 0, 0, TRDP_OPTION_BLOCK};
    int rv = 0;
    // all values, that could be set via the commandline:
    UINT32 cmdl_destIP, cmdl_cycletime, cmdl_timeout, cmdl_srcIP = 0;

    /* Parse the given commandline arguments */
    rv = trdp_test_general_parseCommandLineArguments(argc, argv, 
			&cmdl_destIP, &cmdl_cycletime, &cmdl_timeout, &cmdl_srcIP);
			
    if (rv != 0)
        return rv;
    
    /* clean data before using it */
    memset(&gLastupdateTime, 0, sizeof(gLastupdateTime));
    memset(&gMyDataSet998, 0, sizeof(gMyDataSet998));

    /*	Init the library for callback operation */
    if (tlc_init(dbgOut, &dynamicConfig) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*	Open a session for callback operation */
    if (tlc_openSession(&appHandle,
                        0, 0,                              	/* use default IP addresses */
                        NULL,                              	/* no Marshalling */
                        &pdConfiguration, &mdConfiguration,	/* system defaults for PD and MD	*/
                        &processConfig) != TRDP_NO_ERR)
    {
        return 1;
    }

    /*  Subscribe to control PD     */
    memset(&gMyDataSet999, 0, sizeof(gMyDataSet999));
    err = tlp_subscribe( appHandle,                 /*   our application identifier          */
                         &subHandle,                /*   our subscription identifier         */
                         NULL,
                         PD_TEST_GEN_UNI_COMID,     /*   ComID                               */
                         4321,                      /*   topocount: local consist only       */
                         cmdl_destIP,               /*   Source IP filter                    */
                         0,
                         0,                         /*   Default destination	(or MC Group)   */
                         cmdl_timeout,   			/*   Time out in us                      */
                         TRDP_TO_SET_TO_ZERO,       /*   delete invalid data	on timeout      */
                         sizeof(gMyDataSet999));    /*   net data size                       */

    if (err != TRDP_NO_ERR)
    {
        printf("tlp_subscribe error\n");
        tlc_terminate();
        return 1;
    }
    
    /*  Subscribe to control multicast PD */
    err = tlp_subscribe( appHandle,                 /*   our application identifier          */
                         &subHandleMC,                /*   our subscription identifier         */
                         NULL,
                         PD_TEST_GEN_UNI_COMID,     /*   ComID                               */
                         4321,                         /*   topocount: local consist only       */
                         0, /* don't filter, needed for static package */				/*   Source IP filter                    */
                         0,
                         PD_TESTMULTICAST_IP,		/*   Default destination	(or MC Group)   */
                         cmdl_timeout,   			/*   Time out in us                      */
                         TRDP_TO_SET_TO_ZERO,       /*   delete invalid data	on timeout      */
                         sizeof(gMyDataSet999));    /*   net data size                       */

    if (err != TRDP_NO_ERR)
    {
        printf("tlp_subscribe error\n");
        tlc_terminate();
        return 1;
    }


    /*  Subscribe to custom PD PULL request */
	err = tlp_subscribe( appHandle,                 /*   our application identifier          */
						 &subHandlePullreq,         /*   our subscription identifier         */
						 NULL,
						 PD_TEST_PULL_COMID,     	/*   ComID                               */
						 0,                         /*   topocount: local consist only       */
						 0, 						/* don't filter, needed for static package */				/*   Source IP filter                    */
						 0,
						 0,							/*   Default destination	(or MC Group)   */
						 cmdl_timeout,   			/*   Time out in us                      */
						 TRDP_TO_SET_TO_ZERO,       /*   delete invalid data	on timeout      */
						 sizeof(gMyDataSet999));    /*   net data size                       */

	if (err != TRDP_NO_ERR)
	{
		printf("tlp_subscribe error\n");
		tlc_terminate();
		return 1;
	}


    /*	Publish Unicast PD  status package */
    err = tlp_publish(appHandle,                    /*    our application identifier  */
                      &pubHandleUC,                 /*    our pulication identifier   */
                      PD_TEST_ECHO_UNI_COMID,   	/*    ComID to send               */
                      0,                            /*    local consist only          */
                      0,                            /*    default source IP           */
                      cmdl_destIP,                  /*    where to send to            */
                      cmdl_cycletime,   			/*    Cycle time in ms            */
                      0,                            /*    not redundant               */
                      TRDP_FLAGS_CALLBACK,          /*    Use callback for errors     */
                      NULL,                         /*    default qos and ttl         */
                      (UINT8 *)&gMyDataSet998,      /*    initial data                */
                      sizeof(gMyDataSet998),        /*    data size                   */
                      FALSE,                        /*    no ladder                   */
                      0);                           /*    no ladder                   */

    if (err != TRDP_NO_ERR)
    {
        printf("prep pd publish error\n");
        tlc_terminate();
        return 1;
    }

    /*	Publish Unicast PD  package [PULL] stuff */
	err = tlp_publish(appHandle,                    /*    our application identifier  */
					  &pubHandleUCpull,             /*    our publication identifier   */
					  PD_TEST_PULL_REPLY_COMID,   	/*    ComID to send               */
					  0,                            /*    local consist only          */
					  0,                            /*    default source IP           */
					  0 /* everyone */,				/*    where to send to            */
					  0,   							/*    Cycle time in ms            */
					  0,                            /*    not redundant               */
					  TRDP_FLAGS_CALLBACK,          /*    Use callback for errors     */
					  NULL,                         /*    default qos and ttl         */
					  (UINT8 *)&gPullresponse,      /*    initial data                */
					  PULL_RESPONSE_SIZE,        	/*    data size                   */
					  FALSE,                        /*    no ladder                   */
					  0);                           /*    no ladder                   */

	if (err != TRDP_NO_ERR)
	{
		printf("prep pd publish error\n");
		tlc_terminate();
		return 1;
	}


	/* we are also interested in MD stuff */
	err = tlm_addListener(
				appHandle,
				&lstHandleMD,
				&gUser_env,
				MD_TEST_COMID,
				0,
				vos_htonl(0) /*cmdl_destIP*/,
				0,
				"test");
	if (err != TRDP_NO_ERR)
	{
		fprintf(stderr,"tlm_addListener() error = %d\n",err);
		exit(EXIT_FAILURE);
	}


    /*
     Enter the main processing loop.
     */
    while (gKeepOnRunning)
    {
        INT32   noOfDesc;
        struct timeval  tv;
        struct timeval  max_tv = {0, 100000};
    	fd_set  rfds;

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

        //tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);

        /*FIXME the code only receives packages when no fds are given */
        tlc_process(appHandle, NULL, NULL);


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



        /*** Echo dataset ***/
        sprintf(&gPullresponse, "Actual time is %s", vos_getTimeStamp() );
        tlp_put(appHandle, pubHandleUCpull, (const UINT8 *) &gPullresponse, PULL_RESPONSE_SIZE);


        /* Display received information */
        if (gInputBuffer[0] > 0) //FIXME Better solution would be: global flag, that is set in the callback function to indicate new data
        {
            memcpy(&(gMyDataSet998.echoDataset), &gInputBuffer, trdp_general_min(sizeof(gInputBuffer), sizeof(gMyDataSet998.echoDataset)) );
            memset(gInputBuffer, 0, sizeof(gInputBuffer));
        }

        tlp_put(appHandle, pubHandleUC, (const UINT8 *) &gMyDataSet998, sizeof(gMyDataSet998));

    }   /*	Bottom of while-loop */

    /*
     *	We always clean up behind us!
     */
    tlp_unpublish(appHandle, pubHandleUC);
    tlp_unsubscribe(appHandle, subHandle);

    tlc_closeSession(appHandle);
    tlc_terminate();

    return rv;
}
