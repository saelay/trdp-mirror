/******************************************************************************/
/**
 * @file            sendHello.c
 *
 * @brief           Demo application for TRDP
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr and Florian Weispfenning, NewTec GmbH
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id: sendHello.c 2 2012-06-04 11:25:16Z 97025 $
 *
 */

/*******************************************************************************
 * INCLUDES
 */
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
//#include <sys/time.h>
//#include <sys/select.h>

#include <trdp_if_light.h>
#include "vos_thread.h"
#include "vos_utils.h"
#include "vos_sock.h"
#include "test_server.h"

// include also stuff, needed for windows:
#ifdef WIN32
#include <winsock2.h>
#endif

/*******************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION     "1.0"

#define DATA_MAX        1000

#define PD_COMID        999
#define PD_COMID_CYCLE  5000000             /* in us (1000000 = 1 sec) */

/* We use dynamic memory	*/
#define RESERVED_MEMORY  900000

/* define the logging, and what we want to see */
void win32_print_debug(
    void        *pRefCon,
    VOS_LOG_T   category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr)
{
	return;
	switch(category) {
	case VOS_LOG_WARNING:
		printf("WARN\t %s %s", vos_getTimeStamp(), pMsgStr);
		break;
	case VOS_LOG_INFO:
		printf("INFO\t %s %s", vos_getTimeStamp(), pMsgStr);
		break;	
	case VOS_LOG_DBG:
		printf("DEBUG\t %s %s", vos_getTimeStamp(), pMsgStr);
		break;
	case VOS_LOG_ERROR:
		printf("ERROR\t %s %s", vos_getTimeStamp(), pMsgStr);
		break;
	default:
		printf("___:\t %s", pMsgStr);
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


/******************************************************************************/
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (int argc, char *argv[])
{
    int     ip[4];
    INT32   hugeCounter = 0;
    TRDP_APP_SESSION_T  appHandle;  /*	Our identifier to the library instance	*/
    TRDP_PUB_T          pubHandle;  /*	Our identifier to the publication	*/
	TRDP_SUB_T          subHandle;  /*	Our identifier to the publication	*/
    UINT32              comId = PD_COMID;
    TRDP_ERR_T          err;
    TRDP_PD_CONFIG_T    pdConfiguration =
	{ myPDcallBack, NULL, {0, 64}, TRDP_FLAGS_NONE, 1000, TRDP_TO_SET_TO_ZERO};
    TRDP_MEM_CONFIG_T   dynamicConfig = {NULL, RESERVED_MEMORY, {}};

	/* needed for polling */
	TRDP_PD_INFO_T  pdMsg;
	UINT8			buffer[2048];
	UINT32			buffer_size = sizeof(buffer);

    int                 rv = 0;


    UINT32              destIP = 0;

    /*	Generate some data, that we want to send, when nothing was specified. */
    UINT8               *outputBuffer;
    UINT8               exampleData[1000]   = "";
    int                 outputBufferSize    = 100;
    outputBuffer = exampleData;

    printf("%s: Version %s\t(%s - %s)\n",
            argv[0], APP_VERSION, __DATE__, __TIME__);

	// set it to the IP address of the virtual machine
	destIP = 0x0A0000C9; // 10.0.0.201
    
	if (destIP == 0)
    {
       	fprintf(stderr, "No destination address given!\n");
        return 1;
    }
	
    /*	Init the library  */
    if (tlc_init(&win32_print_debug,        /* logging	*/
                 &dynamicConfig            /* Use application supplied memory	*/
                 ) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

	// the logging is intialized (so a short test)
	vos_printf(VOS_LOG_DBG, "Sending information to: %x\n", destIP);


	/* Open a session */
	if (tlc_openSession(&appHandle, 
						0, 0,					/* use default IP address */
						NULL,					/* no Marshalling */
						&pdConfiguration, NULL, /* system defaults for PD and MD */
						NULL					/* Pointer to process configuration */
						) != TRDP_NO_ERR)
	{
			printf("Initialization error\n");
			return 1;
	}


	 /*  Subscribe to control PD     */
    err = tlp_subscribe( appHandle,                 /*   our application identifier          */
                         &subHandle,                /*   our subscription identifier         */
                         NULL,
                         998,						/*   ComID                               */
                         0,                         /*   topocount: local consist only       */
                         destIP,					/*   Source IP filter                   */
                         0,
                         0,                         /*   Default destination	(or MC Group)   */
                         (PD_COMID_CYCLE) * 2,  	/*   Time out in us                      */
                         TRDP_TO_SET_TO_ZERO,       /*   delete invalid data	on timeout      */
                         sizeof(gMyDataSet998));    /*   net data size                       */
	
	if (err != TRDP_NO_ERR)
    {
        printf("subscribe pd error (%d)\n", err);
        tlc_terminate();
        return 1;
    }


    /*	Copy the packet into the internal send queue, prepare for sending.	*/
    /*	If we change the data, just re-publish it	*/
    err = tlp_publish(  appHandle,                  /*	our application identifier	*/
                        &pubHandle,                 /*	our pulication identifier	*/
                        comId,                      /*	ComID to send				*/
                        0,                          /*	local consist only			*/
                        0,                          /*	default source IP			*/
                        destIP,                     /*	where to send to			*/
                        PD_COMID_CYCLE,             /*	Cycle time in us			*/
                        0,                          /*	not redundant				*/
                        TRDP_FLAGS_NONE,            /*	Use callback for errors		*/
                        NULL,                       /*	default qos and ttl			*/
                        (UINT8 *)outputBuffer,      /*	initial data                */
                        outputBufferSize,           /*	data size					*/
                        FALSE,                      /*	no ladder					*/
                        0);                         /*	no ladder					*/


    if (err != TRDP_NO_ERR)
    {
        printf("publish pd error (%d)\n", err);
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
		int textLength;

        /*
           Prepare the file descriptor set for the select call.
           Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);
        /* FD_SET(pd_fd, &rfds); */

        /*
           Compute the min. timeout value for select.
           This way we can guarantee that PDs are sent in time...
           (A requirement for the IPT Wire Protocol)
         */
        tlc_getInterval(appHandle,
                        (TRDP_TIME_T *) &tv,
                        (TRDP_FDS_T *) &rfds,
                        &noDesc);

        /*
           The wait time for select must consider cycle times and timeouts of
           the PD packets received or sent.
           If we need to poll something faster than the lowest PD cycle,
           we need to set the maximum time out our self
         */
        if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0)
        {
            tv = max_tv;
        }

        /*
           Select() will wait for ready descriptors or time out,
           what ever comes first.
         */
        rv = select((int)noDesc, &rfds, NULL, NULL, &tv);

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
		tlc_process(appHandle, NULL, NULL);
		tlp_get(appHandle, subHandle, pdConfiguration.flags, &pdMsg, buffer, &buffer_size );


#ifdef WIN32
		/* convert the time to sleep into milliseconds */
		Sleep((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
#else
#error "This application was developped on windows for windows only"
#endif

        /* Handle other ready descriptors... */
        if (rv > 0)
        {
            printf("other descriptors were ready\n");
        }
        else
        {
			// do some other stuff bv  
            //printf("looping...\n");
        }

        textLength = sprintf((char *)outputBuffer, "Windows speaking: %d", hugeCounter++) + 1 /* one is needed to mark the end of the string*/;
		textLength += sprintf((char *) (outputBuffer + textLength), "TRDP Stack %s (%s - %s)", APP_VERSION, __DATE__, __TIME__) + 1 /* one is needed to mark the end of the string*/;

        err =
            tlp_put(appHandle, pubHandle, outputBuffer,
                    textLength);
        if (err != TRDP_NO_ERR)
        {
            printf("put pd error\n");
            rv = 1;
            break;
        }
    }

    /*
     *	We always clean up behind us!
     */
    tlp_unpublish(appHandle, pubHandle);

    tlc_terminate();
    return rv;
}
