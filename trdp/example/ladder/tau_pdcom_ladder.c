/**********************************************************************************************************************/
/**
 * @file            tau_pdcom_Ladder.c
 *
 * @brief           Functions for TRDP Ladder Topology PD communication (PDComLadder Thread)
 *
 * @details			Receive, send and wirte Traffic Store process data at a fixed cycle
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, TOSHIBA
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright TOSHIBA, Japan, 2013.
 *
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

#include "vos_utils.h"
#include "vos_thread.h"
#include "trdp_pdcom.h"
#include "trdp_if_light.h"
#include "tau_ladder.h"
#include "tau_ladder_app.h"

/**********************************************************************************************************************/
/** callback function PD receive
 *
 *  @param[in]		pRefCon			user supplied context pointer
 *  @param[in]		appHandle			application handle returned by tlc_opneSession
 *  @param[in]		pPDInfo			pointer to PDInformation
 *  @param[in]		pData			pointer to receive PD Data
 *  @param[in]		dataSize        receive PD Data Size
 *
 */
void tau_recvPdDs (
    void *pRefCon,
    TRDP_APP_SESSION_T appHandle,
    const TRDP_PD_INFO_T *pPDInfo,
    UINT8 *pData,
    UINT32 dataSize)
{
	UINT32 subnetId;											/* Using Sub-network Id */
	UINT32 displaySubnetId = SUBNETID_TYPE1;				/* Using Sub-network Id for Display log */
	UINT16 offset = 0;										/* Traffic Store Offset Address */
#if 0
	BOOL linkUpDown = TRUE;									/* Link Up Down information TRUE:Up FALSE:Down */
#endif
	extern UINT8 *pTrafficStoreAddr;						/* pointer to pointer to Traffic Store Address */
	PD_COMMAND_VALUE *subscriberPdCommandValue = NULL;	/* Subscriber PD Command Value */
	TRDP_ADDRESSES_T addr = {0};
	PD_ELE_T *pSubscriberElement = NULL;

	/* Set Receive PD addr */
	addr.comId = pPDInfo->comId;
	addr.destIpAddr = pPDInfo->destIpAddr;
	addr.srcIpAddr = pPDInfo->srcIpAddr;
	if (vos_isMulticast(pPDInfo->destIpAddr))
	{
		addr.mcGroup = pPDInfo->destIpAddr;
	}
	else
	{
		addr.mcGroup = 0;
	}
	addr.topoCount = pPDInfo->topoCount;

	/* Search Subscriber command Value */
	subscriberPdCommandValue = serachPdCommandValueToAddr(&addr, &subnetId);
	if (subscriberPdCommandValue == NULL)
	{
#if 0
		/* for ladderApplication_multiPD */
		vos_printLog(VOS_LOG_ERROR, "UnMatch Subscribe\n");
		return;
#endif /* if 0 */
	}
	else
	{
		/* Receive Subnet1 */
		if (subnetId == SUBNET1)
		{
			/* Set Receive Count */
			subscriberPdCommandValue->subnet1ReceiveCount = subscriberPdCommandValue->subnet1ReceiveCount + 1;
			if (pPDInfo->resultCode == TRDP_TIMEOUT_ERR)
			{
				/* Set Timeout Count */
				subscriberPdCommandValue->subnet1TimeoutReceiveCount = subscriberPdCommandValue->subnet1TimeoutReceiveCount + 1;
			}
		}
		/* Receive Subnet2 */
		else if(subnetId == SUBNET2)
		{
			/* Set Receive Count */
			subscriberPdCommandValue->subnet2ReceiveCount = subscriberPdCommandValue->subnet2ReceiveCount + 1;
			if (pPDInfo->resultCode == TRDP_TIMEOUT_ERR)
			{
				/* Set Timeout Count */
				subscriberPdCommandValue->subnet2TimeoutReceiveCount = subscriberPdCommandValue->subnet2TimeoutReceiveCount + 1;
			}
		}
		else
		{
			vos_printLog(VOS_LOG_ERROR, "Write Receive Count Err\n");
		}
	}

	/* check parameter */
/*	if ((pRefCon == NULL) || (pData == NULL) || (dataSize == 0)) */
/*	if ((pData == NULL) || (dataSize == 0)) */
	/* ( Receive Data Noting OR Receive Data Size: 0 OR Offset Address not registered ) */
	/* AND result: other then Timeout */
	if (((pData == NULL)	|| (dataSize == 0) || (pPDInfo->pUserRef == 0))
		&&	(pPDInfo->resultCode != TRDP_TIMEOUT_ERR))
	{
       vos_printLog(VOS_LOG_ERROR, "There is no data which save at Traffic Store\n");
		return;
	}

	tau_lockTrafficStore();
	tau_getNetworkContext(&subnetId);

	/* Write received PD from using subnetwork in Traffic Store */
	if ((pPDInfo->srcIpAddr & SUBNET2_NETMASK) == subnetId)
	{
		/* Receive Timeout ? */
		if (pPDInfo->resultCode == TRDP_TIMEOUT_ERR)
		{
			/* Get Subscriber */
			pSubscriberElement = trdp_queueFindSubAddr(appHandle->pRcvQueue, &addr);
			/* Check toBechavior */
			if (pSubscriberElement->toBehavior == TRDP_TO_SET_TO_ZERO)
			{
				if (pPDInfo->pUserRef == NULL)
				{
				       vos_printLog(VOS_LOG_INFO, "There is no offset Address\n");
						tau_unlockTrafficStore();
						return;
				}
				/* Clear Traffic Store */
				memcpy(&offset, (void *)pPDInfo->pUserRef, sizeof(offset));
				memset((void *)((int)pTrafficStoreAddr + (int)offset), 0, pSubscriberElement->dataSize);
				tau_unlockTrafficStore();
				/* Set sunbetId for display log */
				if( subnetId == SUBNET1)
				{
					/* Set Subnet1 */
					displaySubnetId = SUBNETID_TYPE1;
				}
				else
				{
					/* Set Subnet2 */
					displaySubnetId = SUBNETID_TYPE2;
				}
				vos_printLog(VOS_LOG_ERROR, "SubnetId:%d comId:%d Timeout. Traffic Store Clear.\n", displaySubnetId, pPDInfo->comId);

#if 0
				/* Check Subnet for Write Traffic Store Receive Subnet */
				tau_checkLinkUpDown(subnetId, &linkUpDown);
				/* Link Down */
				if (linkUpDown == FALSE)
				{
					/* Change Write Traffic Store Receive Subnet */
					if( subnetId == SUBNET1)
					{
						vos_printLog(VOS_LOG_INFO, "Subnet1 Link Down. Change Receive Subnet\n");
						/* Write Traffic Store Receive Subnet : Subnet2 */
						subnetId = SUBNET2;
					}
					else
					{
						vos_printLog(VOS_LOG_INFO, "Subnet2 Link Down. Change Receive Subnet\n");
						/* Write Traffic Store Receive Subnet : Subnet1 */
						subnetId = SUBNET1;
					}
					/* Set Write Traffic Store Receive Subnet */
					if (tau_setNetworkContext(subnetId) != TRDP_NO_ERR)
				    {
						vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_setNetworkContext error\n");
				    }
					else
					{
						vos_printLog(VOS_LOG_DBG, "tau_setNetworkContext() set subnet:0x%x\n", subnetId);
					}
				}
#endif /* if 0 */
			}
		}
		else
		{
			if (pPDInfo->pUserRef == NULL)
			{
			       vos_printLog(VOS_LOG_INFO, "There is no offset Address\n");
					tau_unlockTrafficStore();
					return;
			}
			/* Set received PD Data in Traffic Store */
/*			memcpy(&offset, pRefCon, sizeof(offset)); */
			memcpy(&offset, (void *)pPDInfo->pUserRef, sizeof(offset));
			memcpy((void *)((int)pTrafficStoreAddr + (int)offset), pData, dataSize);

vos_printLog(VOS_LOG_DBG, "*** Traffic Store Write TIme *** comId:%d subnetId:%d character:%d seqCount:%d\n",
		addr.comId, displaySubnetId, *(pData+1), pPDInfo->seqCount);
			tau_unlockTrafficStore();
		}
	}
	else
	{
		tau_unlockTrafficStore();
	}
}

/**********************************************************************************************************************/
/** PDComLadder
 *
 *
 */
VOS_THREAD_FUNC_T PDComLadder (void)
{
	/* Wait for multicast grouping */
	vos_threadDelay(PDCOM_MULTICAST_GROUPING_DELAY_TIME);

	/* Judge PDComLadderThread start */
	for(;;)
	{
		if (pdComLadderThreadStartFlag == TRUE)
		{
			break;
		}
		else
		{
			/* wait */
			vos_threadDelay(PDCOM_LADDER_THREAD_DELAY_TIME);
		}
	}

	/*
         Enter the main processing loop.
     */
	while (1)
/* DEBUG
	int i;
	for(i=0; i>=1; i++)
*/
	{
		fd_set  rfds;
		INT32   noOfDesc = 0;
		struct timeval  tv;
		struct timeval  max_tv = {0, 100000};

		INT32   noOfDesc2 = 0;
		struct timeval  tv2;
		BOOL linkUpDown = TRUE;						/* Link Up Down information TRUE:Up FALSE:Down */
		UINT32 writeSubnetId;						/* Using Traffic Store Write Sub-network Id */

		/*
		Prepare the file descriptor set for the select call.
		Additional descriptors can be added here.
		*/
		FD_ZERO(&rfds);
		/*
		Compute the min. timeout value for select and return descriptors to wait for.
		This way we can guarantee that PDs are sent in time...
		*/

		/* First TRDP instance */
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


		/* second TRDP instance */
		tlc_getInterval(appHandle2,
						 (TRDP_TIME_T *) &tv2,
						 (TRDP_FDS_T *) &rfds,
						 &noOfDesc2);

		if (vos_cmpTime((TRDP_TIME_T *) &tv2, (TRDP_TIME_T *) &max_tv) > 0)
		{
			tv2 = max_tv;
		}

		/*
		Select() will wait for ready descriptors or timeout,
		what ever comes first.
		*/
		/* The Number to check descriptor */
		if (noOfDesc > noOfDesc2)
		{
			noOfDesc = noOfDesc + 1;
		}
		else
		{
			noOfDesc = noOfDesc2 +1;
		}

		if (tv.tv_sec > tv2.tv_sec)
		{
			tv.tv_sec = tv2.tv_sec;
			tv.tv_usec = tv2.tv_usec;
		}
		if ((tv.tv_sec == tv2.tv_sec) && (tv.tv_usec > tv2.tv_usec))
		{
			tv.tv_usec = tv2.tv_usec;
		}

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

		/* Don't Receive ? */
		if (rv <= 0)
		{
			/* Get Write Traffic Store Receive SubnetId */
			if (tau_getNetworkContext(&writeSubnetId) != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_getNetworkContext error\n");
			}
			/* Check Subnet for Write Traffic Store Receive Subnet */
			tau_checkLinkUpDown(writeSubnetId, &linkUpDown);
			/* Link Down */
			if (linkUpDown == FALSE)
			{
				/* Change Write Traffic Store Receive Subnet */
				if( writeSubnetId == SUBNET1)
				{
					vos_printLog(VOS_LOG_INFO, "Subnet1 Link Down. Change Receive Subnet\n");
					/* Write Traffic Store Receive Subnet : Subnet2 */
					writeSubnetId = SUBNET2;
				}
				else
				{
					vos_printLog(VOS_LOG_INFO, "Subnet2 Link Down. Change Receive Subnet\n");
					/* Write Traffic Store Receive Subnet : Subnet1 */
					writeSubnetId = SUBNET1;
				}
				/* Set Write Traffic Store Receive Subnet */
				if (tau_setNetworkContext(writeSubnetId) != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_setNetworkContext error\n");
				}
				else
				{
					vos_printLog(VOS_LOG_DBG, "tau_setNetworkContext() set subnet:0x%x\n", writeSubnetId);
				}
			}
		}

		/* First TRDP instance, calls the call back function to handle1 received data
		* and copy them into the Traffic Store using offset address from configuration. */
		tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);

		/* Second TRDP instance, calls the call back function to handle1 received data
		* and copy them into the Traffic Store using offset address from configuration. */
		tlc_process(appHandle2, (TRDP_FDS_T *) &rfds, &rv);

	}   /*	Bottom of while-loop	*/

	return NULL;
}

/**********************************************************************************************************************/
/** Return the PdCommandValue with same comId and IP addresses
 *
 *  @param[in]		pNewPdCommandValue		Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *  @param[out]		pMatchSubnet            	pointer to match subnet number
 *
 *  @retval         != NULL         		pointer to PdCommandValue
 *  @retval         NULL            		No PD PdCommandValue found
 */
PD_COMMAND_VALUE *serachPdCommandValueToAddr (
		TRDP_ADDRESSES_T    *addr,
		UINT32	*pMatchSubnet)
{
	PD_COMMAND_VALUE *iterPdCommandValue;

    if (addr == NULL)
    {
        return NULL;
    }

    for (iterPdCommandValue = pFirstPdCommandValue;
    	  iterPdCommandValue != NULL;
    	  iterPdCommandValue = iterPdCommandValue->pNextPdCommandValue)
    {
        /*  Subscribe subnet1 Command: We match if src/dst address is zero or matches */
    	if (iterPdCommandValue->PD_SUB_COMID1 == addr->comId &&
            (iterPdCommandValue->PD_COMID1_SUB_SRC_IP1 == 0 || iterPdCommandValue->PD_COMID1_SUB_SRC_IP1 == addr->srcIpAddr) &&
            (iterPdCommandValue->PD_COMID1_SUB_DST_IP1 == 0 || iterPdCommandValue->PD_COMID1_SUB_DST_IP1 == addr->destIpAddr)
            )
        {
            *pMatchSubnet = SUBNET1;
            return iterPdCommandValue;
        }
        /*  Subscribe subnet2 Command: We match if src/dst address is zero or matches */
    	else if(iterPdCommandValue->PD_SUB_COMID1 == addr->comId &&
                (iterPdCommandValue->PD_COMID1_SUB_SRC_IP2 == 0 || iterPdCommandValue->PD_COMID1_SUB_SRC_IP2 == addr->srcIpAddr) &&
                (iterPdCommandValue->PD_COMID1_SUB_DST_IP2 == 0 || iterPdCommandValue->PD_COMID1_SUB_DST_IP2 == addr->destIpAddr)
                )
		{
            *pMatchSubnet = SUBNET2;
            return iterPdCommandValue;
		}
        else
        {
        	continue;
        }

    }
    return NULL;
}

#endif /* TRDP_OPTION_LADDER */
