/**********************************************************************************************************************/
/**
 * @file        	ladderApplication.c
 *
 * @brief			Demo ladder application for TRDP
 *
 * @details		TRDP Ladder Topology Support initialize and initial setting, write Traffic Store process data at a fixed cycle
 *
 * @note			Project: TCNOpen TRDP prototype stack
 *
 * @author			Kazumasa Aiba, Toshiba Corporation
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Toshiba Corporation, Japan, 2013. All rights reserved.
 *
 * $Id$
 *
 */
#ifdef TRDP_OPTION_LADDER
/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <errno.h>

#include "vos_utils.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "trdp_pdcom.h"
#include "trdp_if_light.h"
#include "tau_ladder.h"
#include "tau_ladder_app.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

/* Sub-network Id1 */
TRDP_APP_SESSION_T  appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
TRDP_SUB_T          subHandleNet1ComId1;		/*	Sub-network Id1 ComID1 identifier to the subscription	*/
TRDP_PUB_T          pubHandleNet1ComId1;		/*	Sub-network Id1 ComID2 identifier to the publication	*/
TRDP_ERR_T          err;
TRDP_PD_CONFIG_T    pdConfiguration = {&tau_recvPdDs, NULL, {0, 0}, TRDP_FLAGS_NONE,
                                       10000000, TRDP_TO_SET_TO_ZERO, 20548};
TRDP_MEM_CONFIG_T   dynamicConfig = {NULL, RESERVED_MEMORY, {}};
TRDP_PROCESS_CONFIG_T	processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};

INT32     rv = 0;

/* Sub-network Id2 */
TRDP_APP_SESSION_T  appHandle2;					/*	Sub-network Id2 identifier to the library instance	*/
TRDP_SUB_T          subHandleNet2ComId1;		/*	Sub-network Id2 ComID1 identifier to the subscription	*/
TRDP_PUB_T          pubHandleNet2ComId1;		/*	Sub-network Id2 ComID2 identifier to the publication	*/
TRDP_ERR_T          err2;
TRDP_PD_CONFIG_T    pdConfiguration2 = {&tau_recvPdDs, NULL, {0, 0}, TRDP_FLAGS_NONE,
                                       10000000, TRDP_TO_SET_TO_ZERO, 20548};	    /* Sub-network Id2 PDconfiguration */
TRDP_MEM_CONFIG_T   dynamicConfig2 = {NULL, RESERVED_MEMORY, {}};					/* Sub-network Id2 Structure describing memory */
TRDP_PROCESS_CONFIG_T   processConfig2   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};


INT32     rv2 = 0;
UINT16 OFFSET_ADDRESS1	= 0x1100;				/* offsetAddress comId1 */
UINT16 OFFSET_ADDRESS2	= 0x1180;				/* offsetAddress comId1 */

UINT8   firstPutData[PD_DATA_SIZE_MAX] = "First Put";

/***********************************************************************************************************************
 * DEFINES
 */
#define SUBNET2_NETMASK							0x00002000			/* The netmask for Subnet2 */
#define PDCOM_LADDER_THREAD_DELAY_TIME			10000				/* PDComLadder Thread starting Judge Cycle in us */
#define PDCOM_MULTICAST_GROUPING_DELAY_TIME		10000000			/* PDComLadder Thread starting Wait Time in us */

/* Some sample comId definitions	*/
#define PD_DATA_SIZE_MAX			300					/* PD DATA MAX SIZE */
#define LADDER_APP_CYCLE			500000				/* Ladder Application data put cycle in us */

/* Subscribe for Sub-network Id1 */
#define PD_COMID1				10001
#define PD_COMID2				10002
#define PD_COMID1_TIMEOUT		1200000
#define PD_COMID2_TIMEOUT		1200000
#define PD_COMID1_DATA_SIZE		32
#define PD_COMID1_SRC_IP        0x0A000111		/* Source IP: 10.0.1.17 */
#define PD_COMID2_SRC_IP        0x0A000111		/* Source IP: 10.0.1.17 */
//#define PD_COMID1_DST_IP        0xefff0101		/* Destination IP: 239.255.1.1 */
//#define PD_COMID2_DST_IP        0xefff0101		/* Destination IP: 239.255.1.1 */
#define PD_COMID1_DST_IP        0x0A000111		/* Destination IP: 239.255.1.1 */
#define PD_COMID2_DST_IP        0x0A002111		/* Destination IP: 239.255.1.1 */

/* Subscribe for Sub-network Id2 */
#define PD_COMID1_SRC_IP2       PD_COMID1_SRC_IP | SUBNET2_NETMASK      /*	Sender's IP: 10.4.33.17		*/
#define PD_COMID2_SRC_IP2       PD_COMID2_SRC_IP | SUBNET2_NETMASK      /*	Sender's IP: 10.4.33.17		*/

/* Publish for Sub-network Id1 */
#define PD_COMID1_CYCLE         30000000			/* ComID1 Publish Cycle TIme */
#define PD_COMID2_CYCLE         30000000			/* ComID2 Publish Cycle TIme */


/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon			user supplied context pointer
 *  @param[in]		category		Log category (Error, Warning, Info etc.)
 *  @param[in]		pTime			pointer to NULL-terminated string of time stamp
 *  @param[in]		pFile			pointer to NULL-terminated string of source module
 *  @param[in]		LineNumber		line
 *  @param[in]		pMsgStr         pointer to NULL-terminated string
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
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (void)
{
    CHAR8 putData[PD_DATA_SIZE_MAX];				/* put DATA */
    INT32 putCounter = 0;							/* put counter */
    BOOL8 linkUpDown = TRUE;		                /* Link Up Down information TRUE:Up FALSE:Down */
    /* Using Receive Subnet in order to Wirte PD in Traffic Store  */
    UINT32 TS_SUBNET = 1;

    /* Traffic Store */
	extern UINT8 *pTrafficStoreAddr;				/* pointer to pointer to Traffic Store Address */

#if 0
	/* Get IP Address */
	struct ifaddrs *ifa_list;
	struct ifaddrs *ifa;
	TRDP_IP_ADDR_T subnetId1Address = 0;
	TRDP_IP_ADDR_T subnetId2Address = 0;
	CHAR8 SUBNETWORK_ID1_IF_NAME[] = "eth0";

	/* Get I/F address */
	if (getifaddrs(&ifa_list) != 0)
	{
    	printf("getifaddrs error. errno=%d\n", errno);
       return 1;
	}
	/* Get All I/F List */
	for(ifa = ifa_list; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (strncmp(ifa->ifa_name, SUBNETWORK_ID1_IF_NAME, sizeof(SUBNETWORK_ID1_IF_NAME)) == 0)
		{
			/* IPv4 */
			if (ifa->ifa_addr->sa_family == AF_INET)
			{
				/* Get Sub-net Id1 Address */
				subnetId1Address = (TRDP_IP_ADDR_T)(ifa->ifa_addr->sa_data);
				break;
			}
		}
	}
	/* Release memory */
	freeifaddrs(ifa_list);
#else
//	UINT32			getNoOfIfaces = NUM_ED_INTERFACES;
	getNoOfIfaces = NUM_ED_INTERFACES;
//	VOS_IF_REC_T   	ifAddressTable[NUM_ED_INTERFACES];
	extern VOS_IF_REC_T   	ifAddressTable[NUM_ED_INTERFACES];
    UINT32			index;
	TRDP_IP_ADDR_T 	subnetId1Address = 0;
	TRDP_IP_ADDR_T 	subnetId2Address = 0;
	#ifdef __linux 
	CHAR8 			SUBNETWORK_ID1_IF_NAME[] = "eth0";
//	#elif defined(__APPLE__)
	#else
	CHAR8 			SUBNETWORK_ID1_IF_NAME[] = "en0";
	#endif

	if (vos_getInterfaces(&getNoOfIfaces, ifAddressTable) != VOS_NO_ERR)
    {
    	printf("getifaddrs error. errno=%d\n", errno);
       return 1;
	}
	/* Get All I/F List */
	for (index = 0; index < getNoOfIfaces; index++)
	{
		if (strncmp(ifAddressTable[index].name, SUBNETWORK_ID1_IF_NAME, sizeof(SUBNETWORK_ID1_IF_NAME)) == 0)
		{
				/* Get Sub-net Id1 Address */
            subnetId1Address = (TRDP_IP_ADDR_T)(ifAddressTable[index].ipAddr);
            break;
		}
	}
	
#endif

	/* Sub-net Id2 Address */
	subnetId2Address = subnetId1Address | SUBNET2_NETMASK;

	/* Sub-network Init the library for callback operation	(PD only) */
	if (tlc_init(dbgOut,							/* actually printf	*/
				 &dynamicConfig						/* Use application supplied memory	*/
				) != TRDP_NO_ERR)
	{
		printf("Sub-network Initialization error (tlc_init)\n");
		return 1;
	}

	/*	Sub-network Id1 Open a session for callback operation	(PD only) */
	if (tlc_openSession(&appHandle,
							subnetId1Address, subnetId1Address,	    /* Sub-net Id1 IP address/interface	*/
							NULL,                   				/* no Marshalling		*/
							&pdConfiguration, NULL,					/* system defaults for PD and MD		*/
							&processConfig) != TRDP_NO_ERR)
	{
		printf("Sub-network Id1 Initialization error (tlc_openSession)\n");
		return 1;
	}

	/* TRDP Ladder support initialize */
	if (tau_ladder_init() != TRDP_NO_ERR)
	{
		printf("TRDP Ladder Support Initialize failed\n");
		return 1;
	}

        /*	Sub-network Id2 Open a session for callback operation	(PD only) */
	if (tlc_openSession(&appHandle2,
							subnetId2Address, subnetId2Address,	    /* Sub-net Id2 IP address/interface	*/
							NULL,				                   	/* no Marshalling		*/
							&pdConfiguration2, NULL,				/* system defaults for PD and MD		*/
							&processConfig2) != TRDP_NO_ERR)
	{
		printf("Sub-network Id2 Initialization error (tlc_openSession)\n");
		return 1;
	}

    /*	Sub-network Id1 Subscribe */
    err = tlp_subscribe( appHandle,					/* our application identifier */
                         &subHandleNet1ComId1,		/* our subscription identifier */
                         &OFFSET_ADDRESS1,			/* user referece value = offsetAddress */
                         PD_COMID1,                	/* ComID */
                         0,                        	/* topocount: local consist only */
                         PD_COMID1_SRC_IP,         	/* Source IP filter */
                         0,                        	/* Source IP filter2 : no used */
                         PD_COMID1_DST_IP,        	/* Default destination	(or MC Group) */
                         0,								/* Option */
                         PD_COMID1_TIMEOUT,         /* Time out in us	*/
                         TRDP_TO_SET_TO_ZERO,       /* delete invalid data on timeout */
                         PD_DATA_SIZE_MAX);         /* net data size */


    if (err != TRDP_NO_ERR)
    {
        printf("prep  Sub-network Id1 pd receive error\n");
        tlc_terminate();
        tau_ladder_terminate();
        return 1;
    }

    /* Start PdComLadderThread */
    tau_setPdComLadderThreadStartFlag(TRUE);

    /*	Sub-network Id2 Subscribe */
    err = tlp_subscribe( appHandle2,				/* our application identifier */
                         &subHandleNet2ComId1,		/* our subscription identifier */
                         &OFFSET_ADDRESS1,			/* user referece value = offsetAddress */
                         PD_COMID1,                	/* ComID */
                         0,                        	/* topocount: local consist only */
                         PD_COMID1_SRC_IP2,         /* Source IP filter */
                         0,                        	/* Source IP filter2 : no used */
                         PD_COMID1_DST_IP,        	/* Default destination	(or MC Group) */
                         0,								/* Option */
                         PD_COMID1_TIMEOUT,         /* Time out in us	*/
                         TRDP_TO_SET_TO_ZERO,       /* delete invalid data on timeout */
                         PD_DATA_SIZE_MAX);         /* net data size */

    if (err != TRDP_NO_ERR)
    {
        printf("prep  Sub-network Id2 pd receive error\n");
        tlc_terminate();
        tau_ladder_terminate();
        return 1;
    }

    /*	Sub-network Id1 Publish */
    err = tlp_publish(  appHandle,						/* our application identifier */
                        &pubHandleNet1ComId1,			/* our pulication identifier */
                        PD_COMID1,						/* ComID to send */
                        0,								/* local consist only */
                        subnetId1Address,				/* default source IP */
                        PD_COMID1_DST_IP,				/* where to send to */
                        PD_COMID1_CYCLE,				/* Cycle time in ms */
                        0,								/* not redundant */
                        TRDP_FLAGS_NONE,				/* Don't use callback for errors */
                        NULL,							/* default qos and ttl */
                        firstPutData,					/* initial data */
                        PD_DATA_SIZE_MAX);			/* data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network Id1 pd publish error\n");
        tlc_terminate();
        tau_ladder_terminate();
        return 1;
    }

    /*	Sub-network Id2 Publish */
    err = tlp_publish(  appHandle2,					    /* our application identifier */
                        &pubHandleNet2ComId1,			/* our pulication identifier */
                        PD_COMID1,						/* ComID to send */
                        0,								/* local consist only */
                        subnetId2Address,			    /* default source IP */
                        PD_COMID1_DST_IP,				/* where to send to */
                        PD_COMID1_CYCLE,				/* Cycle time in ms */
                        0,								/* not redundant */
                        TRDP_FLAGS_NONE,				/* Don't use callback for errors */
                        NULL,							/* default qos and ttl */
                        firstPutData,					/* initial data */
                        PD_DATA_SIZE_MAX);			/* data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network Id2 pd publish error\n");
        tlc_terminate();
        tau_ladder_terminate();
        return 1;
    }

    /* Using Sub-Network : SUBNET1 */
    err = tau_setNetworkContext(SUBNET1);
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network error\n");
        return 1;
    }

    /*
        Enter the main processing loop.
     */
    while (1)
    {
      	/* Get access right to Traffic Store*/
    	err = tau_lockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{
        	/* Create PD Data */
    		memset(putData, 0, sizeof(putData));
    		sprintf(putData, "Put Counter %d", putCounter++);
    		/* Set PD Data in Traffic Store */
    		memcpy((void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS1), putData, sizeof(putData));

    		/* Get Write Traffic Store Receive SubnetId */
    		if (tau_getNetworkContext(&TS_SUBNET) != TRDP_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_getNetworkContext error\n");
    		}
    		/* Check Subnet for Write Traffic Store Receive Subnet */
    		tau_checkLinkUpDown(TS_SUBNET, &linkUpDown);
    		/* Link Down */
    		if (linkUpDown == FALSE)
    		{
    			/* Change Write Traffic Store Receive Subnet */
    			if( TS_SUBNET == SUBNET1)
    			{
    				vos_printLog(VOS_LOG_INFO, "Subnet1 Link Down. Change Receive Subnet\n");
    				/* Write Traffic Store Receive Subnet : Subnet2 */
    				TS_SUBNET = SUBNET2;
    			}
    			else
    			{
    				vos_printLog(VOS_LOG_INFO, "Subnet2 Link Down. Change Receive Subnet\n");
    				/* Write Traffic Store Receive Subnet : Subnet1 */
    				TS_SUBNET = SUBNET1;
    			}
    			/* Set Write Traffic Store Receive Subnet */
    			if (tau_setNetworkContext(TS_SUBNET) != TRDP_NO_ERR)
    		    {
    				vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_setNetworkContext error\n");
    		    }
    			else
    			{
    				vos_printLog(VOS_LOG_DBG, "tau_setNetworkContext() set subnet:0x%x\n", TS_SUBNET);
    			}
    		}

    		/* First TRDP instance in TRDP publish buffer */
			tlp_put(appHandle,
					pubHandleNet1ComId1,
					(void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS1),
					appHandle->pSndQueue->dataSize);
			/* Second TRDP instance in TRDP publish buffer */
			tlp_put(appHandle2,
					pubHandleNet2ComId1,
					(void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS1),
					appHandle2->pSndQueue->dataSize);

          	/* Release access right to Traffic Store*/
    		err = tau_unlockTrafficStore();
    		if (err != TRDP_NO_ERR)
    		{
    			printf("Release Traffic Store accessibility Failed\n");
    		}
    	}
    	else
    	{
    		printf("Get Traffic Store accessibility Failed\n");
    	}

    	/* Waits for a next creation cycle */
		vos_threadDelay(LADDER_APP_CYCLE);

    }   /*	Bottom of while-loop	*/

    /*
     *	We always clean up behind us!
     */

    tlp_unpublish(appHandle, pubHandleNet1ComId1);
    tlp_unsubscribe(appHandle, subHandleNet1ComId1);

    tlc_terminate();
    tau_ladder_terminate();

    tlp_unpublish(appHandle2, pubHandleNet2ComId1);
    tlp_unsubscribe(appHandle2, subHandleNet2ComId1);

    tlc_terminate();

    return rv;
}
#endif /* TRDP_OPTION_LADDER */
