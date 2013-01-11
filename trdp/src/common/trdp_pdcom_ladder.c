/**********************************************************************************************************************/
/**
 * @file            trdp_pdcom_Ladder.c
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
#include "trdp_ladder.h"
#include "trdp_ladder_app.h"

/**********************************************************************************************************************/
/** callback function PD receive
 *
 *  @param[in]		pRefCon			user supplied context pointer
 *  @param[in]		pPDInfo			pointer to PDInformation
 *  @param[in]		pData			pointer to receive PD Data
 *  @param[in]		dataSize        receive PD Data Size
 *
 */
void tlp_recvPdDs (
    void *pRefCon,
    const TRDP_PD_INFO_T *pPDInfo,
    UINT8 *pData,
    UINT32 dataSize)
{
	UINT32 subnetId;							/* Using Sub-network Id */
	UINT16 offset = 0;							/* Traffic Store Offset Address */
	extern UINT8 *pTrafficStoreAddr;			/* pointer to pointer to Traffic Store Address */

	/* check parameter */
/*	if ((pRefCon == NULL) || (pData == NULL) || (dataSize == 0)) */
	if ((pData == NULL) || (dataSize == 0))
	{
       vos_printf(VOS_LOG_ERROR, "There is no data which save at Traffic Store\n");
		return;
	}

	tlp_lockTrafficStore();
	tlp_getNetworkContext(&subnetId);

	/* Write received PD from using subnetwork in Traffic Store */
	if ((pPDInfo->srcIpAddr & SUBNET2_NETMASK) == subnetId)
	{
		/* Set received PD Data in Traffic Store */
/*		memcpy(&offset, pRefCon, sizeof(offset)); */
		memcpy(&offset, (void *)pPDInfo->pUserRef, sizeof(offset));
		memcpy((void *)((int)pTrafficStoreAddr + (int)offset), pData, dataSize);
	}
	tlp_unlockTrafficStore();
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

		/* First TRDP instance, calls the call back function to handle1 received data
		* and copy them into the Traffic Store using offset address from configuration. */
		tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);

		/* Second TRDP instance, calls the call back function to handle1 received data
		* and copy them into the Traffic Store using offset address from configuration. */
		tlc_process(appHandle2, (TRDP_FDS_T *) &rfds, &rv);

	}   /*	Bottom of while-loop	*/

	/*
	*	We always clean up behind us!
	*/
	tlp_unpublish(appHandle, pubHandleNet1ComId1);
	tlp_unsubscribe(appHandle, subHandleNet1ComId1);
	/* Is this Ladder Topology ? */
	if (ladderTopologyFlag == TRUE)
	{
		tlp_unpublish(appHandle2, pubHandleNet2ComId1);
		tlp_unsubscribe(appHandle2, subHandleNet2ComId1);
	}
	tlc_terminate();

	tlp_unpublish(appHandle, pubHandleNet1ComId2);
	tlp_unsubscribe(appHandle, subHandleNet1ComId2);
	/* Is this Ladder Topology ? */
	if (ladderTopologyFlag == TRUE)
	{
		tlp_unpublish(appHandle2, pubHandleNet2ComId2);
		tlp_unsubscribe(appHandle2, subHandleNet2ComId2);
	}
	tlc_terminate();

	return NULL;
}
#endif /* TRDP_OPTION_LADDER */
