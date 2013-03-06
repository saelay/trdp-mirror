/**********************************************************************************************************************/
/**
 * @file	mdTestCaller.c
 *
 * @brief	Demo MD ladder application for TRDP
 *
 * @details	TRDP Ladder Topology Support MD Transmission Caller
 *
 * @note	Project: TCNOpen TRDP prototype stack
 *
 * @author	Kazumasa Aiba, TOSHIBA
 *
 * @remarks	All rights reserved. Reproduction, modification, use or disclosure
 *		to third parties without express authority is forbidden,
 *		Copyright TOSHIBA, Japan, 2013.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <sys/select.h>
#include <mqueue.h>
#include <time.h>

#ifdef __linux__
	#include <uuid/uuid.h>
#endif

#include "trdp_types.h"
#include "trdp_if_light.h"
#include "trdp_private.h"
#include "trdp_utils.h"

#include "mdTestApp.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

APP_THREAD_SESSION_HANDLE appThreadSessionHandle ={{0}};		/* appThreadSessionHandle for Subnet1 */
APP_THREAD_SESSION_HANDLE appThreadSessionHandle2 ={{0}};		/* appThreadSessionHandle for Subnet2 */


/**********************************************************************************************************************/
/** MDCaller Thread
 *
 *  @param[in]		pCallerThreadParameter			pointer to CallerThread Parameter
 *
 */
VOS_THREAD_FUNC_T MDCaller (
		CALLER_THREAD_PARAMETER *pCallerThreadParameter)
{
 	mqd_t callerMqDescriptor = 0;		/* Message Queue Descriptor for Caller */
	int err = MD_APP_NO_ERR;
/*	TRDP_LIS_T lisHandle, lisHandle2; */
//	APP_THREAD_SESSION_HANDLE appThreadSessionHandle ={{0}};		/* appThreadSessionHandle for Subnet1 */
//	APP_THREAD_SESSION_HANDLE appThreadSessionHandle2 ={{0}};		/* appThreadSessionHandle for Subnet2 */

	/* Add Listener for Subnet1 */
	err = tlm_addListener(
				appHandle,
				&(appThreadSessionHandle.pMdAppThreadListener),
				NULL,						/* user supplied value returned with reply */
				(pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK,	/* comId to be observed */
				0,							/* topocount to use */
				subnetId1Address,			/* destination Address */
				0,							/* OPTION FLAG */
				subnetId1URI);			/* destination URI */
	/* Check tlm_addListener Return Code */
	if (err != TRDP_NO_ERR)
	{
		printf("AddListener comID = 0x%x error = %d\n",
				(pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK, err);
		return 0;
	}
	printf("AddListener: comID = 0x%x, lisHandle = %p\n",
			((pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK),
			appThreadSessionHandle.pMdAppThreadListener);

	/* Is this Ladder Topology ? */
	if (pCallerThreadParameter->pCommandValue->mdLadderTopologyFlag == TRUE)
	{
		/* Add Listener for Subnet2 */
		err = tlm_addListener(
					appHandle2,
					&(appThreadSessionHandle2.pMdAppThreadListener),
					NULL,
					(pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK,
					0,
					subnetId2Address,
					0,
					subnetId2URI);
		/* Check tlm_addListener Return Code */
		if (err != TRDP_NO_ERR)
		{
			printf("AddListener comID = 0x%x error = %d\n",
					(pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK, err);
			return 0;
		}
		printf("AddListener: comID = 0x%x, lisHandle2 = x%p\n",
				((pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK),
				appThreadSessionHandle2.pMdAppThreadListener);
	}

	/* Message Queue Open */
	err = queue_initialize(pCallerThreadParameter->mqName, &callerMqDescriptor);
	if (err != MD_APP_NO_ERR)
	{
		printf("Caller Message Queue Open error\n");
		return 0;
	}
	else
	{
		/* Set Caller Message Queue Descriptor */
		err = setAppThreadSessionMessageQueueDescriptor(&appThreadSessionHandle, callerMqDescriptor);
		if (err != MD_APP_NO_ERR)
		{
			printf("Subnet1 setAppSessionIdMessageQueueDescriptor error\n");
			return 0;
		}
		/* Is this Ladder Topology ? */
		if (pCallerThreadParameter->pCommandValue->mdLadderTopologyFlag == TRUE)
		{
			/* Set Replier Message Queue Descriptor */
			err = setAppThreadSessionMessageQueueDescriptor(&appThreadSessionHandle2, callerMqDescriptor);
			if (err != MD_APP_NO_ERR)
			{
				printf("Subnet2 setAppThreadSessionMessageQueueDescriptor error\n");
				return 0;
			}
		}
	}

	/* Caller Thread main process */
/*	caller_main_proc(
			callerMqDescriptor,
			((pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK),
			pCallerThreadParameter);
*/

	UINT32 i = 0;
	fd_set  rfds;
	INT32 rv = 0;
/*	INT32   noOfDesc = 0; */
	struct timeval  tv_interval = {0};
	TRDP_TIME_T trdp_time_tv_interval = {0};
/*	struct timeval  next  = {0, 100000}; */
/*	TRDP_TIME_T tv_interval; */
    TRDP_TIME_T nowTime;
    TRDP_TIME_T nextTime;
    struct timespec wanted_delay;
    struct timespec remaining_delay;
    TRDP_LIS_T callerThreadListener = {0};						/* callerThreadListener */
	APP_THREAD_SESSION_HANDLE *pRequestSessionHandle = NULL;	/* in the case of send Request(MR) */
//	APP_THREAD_SESSION_HANDLE mrSendSessionTable[REQUEST_SESSIONID_TABLE_MAX] = {{{0}}};	/* MD Request(Mr) Session Table */
	APP_THREAD_SESSION_HANDLE *pMrSendSessionTable[REQUEST_SESSIONID_TABLE_MAX] = {0};		/* MD Request(Mr) Session Table */
	BOOL mrSendSessionFlag = FALSE;									/* for Check reply session */
	char firstCharacter = 0;
	UINT8 *pCallerCreateIncrementMdData = NULL;
	UINT8 *pFirstCallerCreateMdData = NULL;
	UINT32 incrementMdSendCounter = 0;							/* MD Increment DATA Create Count */

	/* Result Counter */
/*	UINT32 mdRequestSendCounter = 0;	*/				/* MD Request Send Counter */
	static UINT32 mdReceiveCounter = 0;						/* MD Receive Counter */
	static UINT32 mdReceiveFailureCounter = 0;				/* MD Receive OK Counter */
	static UINT32 mdReceiveSuccessCounter = 0;				/* MD Receive NG Counter */
	static UINT32 mdRetryCounter = 0;							/* MD Retry Counter */

	/* LOG */
	CHAR8 logString[CALLER_LOG_BUFFER_SIZE] ={0};		/* Caller Log String */
	size_t logStringLength = 0;
/*	CHAR8 *pWorkLogString = logString; */
	size_t workLogStringLength = 0;
	char strIp[16] = {0};

	/* Message Queue */
	trdp_apl_cbenv_t receiveMqMsg = {0};

	/* Parameter for MD Send */
	TRDP_APP_SESSION_T mdAppHandle = {0};
	void *pMdUserRef = NULL;
//	TRDP_UUID_T *pMdSessionId = NULL;
	TRDP_UUID_T mdSessionId = {0};
//	UINT32 mdSendComId = 0;
	UINT32 mdTopocount = 0;
	TRDP_IP_ADDR_T mdSrcIpAddr = 0;
	TRDP_IP_ADDR_T mdDestIpAddr = 0;
	TRDP_FLAGS_T mdOptionFlag = 0;
	UINT32 mdNoOfRepliers = 0;
	UINT32 mdReplyTimeout = 0;
	TRDP_SEND_PARAM_T *pMdSendParam = NULL;
	TRDP_URI_USER_T mdSourceURI = {0};
	TRDP_URI_USER_T mdDestURI = {0};

	/* Output Log of Caller Thread Parameter */
	if ((((pCallerThreadParameter->pCommandValue->mdLog) && MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
		|| (((pCallerThreadParameter->pCommandValue->mdDump) && MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
	{
		/* Set Log String */
		/* -b --md-caller-replier-type Value */
		workLogStringLength = sprintf(logString,
				"Caller Replier Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdCallerReplierType);
		logStringLength = workLogStringLength;
		/* -c --md-transport-type Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Transport Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdTransportType);
		logStringLength = logStringLength + workLogStringLength;
		/* -d --md-message-kind Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Message Kind : %u\n",
				pCallerThreadParameter->pCommandValue->mdMessageKind);
		logStringLength = logStringLength + workLogStringLength;
		/* -e --md-telegram-type Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Telegram Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdTelegramType);
		logStringLength = logStringLength + workLogStringLength;
		/* -f --md-message-size Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Message Size : %u\n",
				pCallerThreadParameter->pCommandValue->mdMessageSize);
		logStringLength = logStringLength + workLogStringLength;
		/* -g --md-destination-address Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Destination IP Address : %s\n",
				miscIpToString(pCallerThreadParameter->pCommandValue->mdDestinationAddress, strIp));
		logStringLength = logStringLength + workLogStringLength;
		/* -i --md-dump Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Dump Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdDump);
		logStringLength = logStringLength + workLogStringLength;
		/* -j --md-replier-number Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Number of Replier : %u\n",
				pCallerThreadParameter->pCommandValue->mdReplierNumber);
		logStringLength = logStringLength + workLogStringLength;
		/* -k --md-cycle-number Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Number of MD Request Send Cycle : %u\n",
				pCallerThreadParameter->pCommandValue->mdCycleNumber);
		logStringLength = logStringLength + workLogStringLength;
		/* -l --md-log Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Log Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdLog);
		logStringLength = logStringLength + workLogStringLength;
		/* -m --md-cycle-time Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"MD Request Send Cycle Time : %u\n",
				pCallerThreadParameter->pCommandValue->mdCycleTime);
		logStringLength = logStringLength + workLogStringLength;
		/* -n --md-topo Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Ladder Topology Support Flag : %u\n",
				pCallerThreadParameter->pCommandValue->mdLadderTopologyFlag);
		logStringLength = logStringLength + workLogStringLength;
		/* -o --md-reply-err Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Reply Error Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdReplyErr);
		logStringLength = logStringLength + workLogStringLength;
		/* -p --md-marshall Value*/
		workLogStringLength = sprintf(logString + logStringLength,
				"Marshalling Support Flag : %u\n",
				pCallerThreadParameter->pCommandValue->mdMarshallingFlag);
		logStringLength = logStringLength + workLogStringLength;
		/* -r --md-listener-comid Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Listener ComId : 0x%x\n",
				pCallerThreadParameter->pCommandValue->mdAddListenerComId);
		logStringLength = logStringLength + workLogStringLength;
		/* Caller Send comId */
		workLogStringLength = sprintf(logString + logStringLength,
				"Caller Send ComId : 0x%x\n",
				pCallerThreadParameter->pCommandValue->mdSendComId);
		logStringLength = logStringLength + workLogStringLength;
		/* -r --md-timeout-reply Value */
		workLogStringLength = sprintf(logString + logStringLength,
					"Reply Timeout : %u\n",
					pCallerThreadParameter->pCommandValue->mdTimeoutReply);
		logStringLength = logStringLength + workLogStringLength;
		/* -t --md-send-subnet Value */
		workLogStringLength = sprintf(logString + logStringLength,
					"Sender Subnet : %u\n",
					pCallerThreadParameter->pCommandValue->mdSendSubnet);
		logStringLength = logStringLength + workLogStringLength;
		/* MD Application Version */
		workLogStringLength = sprintf(logString + logStringLength,
					"MD Application Version : %s\n",
					MD_APP_VERSION);
		logStringLength = logStringLength + workLogStringLength;
		/* Output Log : Operation Log */
		l2fLog(logString,
				((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
				((pCallerThreadParameter->pCommandValue->mdDump)  & MD_OPERARTION_RESULT_LOG));
		/* Clear Log String */
		memset(logString, 0, sizeof(logString));
		logStringLength = 0;
	}

	/* MD Request Send loop */
	while(sendMdTransferRequestCounter <= pCallerThreadParameter->pCommandValue->mdCycleNumber)
	{
		if ((pCallerThreadParameter->pCommandValue->mdCycleNumber != 0)
			&& (sendMdTransferRequestCounter == 0))
		{
			sendMdTransferRequestCounter++;
		}
//		if ((pCallerThreadParameter->pCommandValue->createMdDataFlag == MD_DATA_CREATE_ENABLE)
//			&& (sendMdTransferRequestCounter < 0))
		if (pCallerThreadParameter->pCommandValue->createMdDataFlag == MD_DATA_CREATE_ENABLE)
		{
			if (incrementMdSendCounter != 0)
			{
				/* Clear Last Time Increment MD DATA */
				free(pCallerThreadParameter->pMdData);
				/* Create Increment MD DATA */
				/* Get MD DATA Area */
				pCallerCreateIncrementMdData = (UINT8 *)malloc(pCallerThreadParameter->pCommandValue->mdMessageSize);
				if (pCallerCreateIncrementMdData == NULL)
				{
					printf("Caller createMdIncrement DataERROR. malloc Err\n");
				}
				else
				{
					pFirstCallerCreateMdData = pCallerCreateIncrementMdData;
					/* Set DATA Size */
					sprintf((char *)pCallerCreateIncrementMdData, "%4u", pCallerThreadParameter->pCommandValue->mdMessageSize);
					pCallerCreateIncrementMdData = pCallerCreateIncrementMdData + 4;

					/* Create Top Character */
					firstCharacter = (char)(incrementMdSendCounter % MD_DATA_INCREMENT_CYCLE);
					/* Create MD Increment Data */
					for(i=0; i <= pCallerThreadParameter->pCommandValue->mdMessageSize - 4; i++)
					{
						*pCallerCreateIncrementMdData = (char)((firstCharacter + i) % MD_DATA_INCREMENT_CYCLE);
						pCallerCreateIncrementMdData = pCallerCreateIncrementMdData + 1;
					}
					pCallerCreateIncrementMdData = pFirstCallerCreateMdData;
					pCallerThreadParameter->pMdData = pCallerCreateIncrementMdData;
					incrementMdSendCounter++;
				}
			}
			else
			{
				incrementMdSendCounter++;
			}

/*			UINT8 **ppCallerCreateMdData = NULL;
			err = createMdIncrementData(sendMdTransferRequestCounter,
											pCallerThreadParameter->pCommandValue->mdMessageSize,
											&ppCallerCreateMdData);

			err = createMdIncrementData(sendMdTransferRequestCounter,
											pCallerThreadParameter->pCommandValue->mdMessageSize,
											pCallerThreadParameter->pMdData);
			if (err != MD_APP_NO_ERR)
			{
*/				/* Error : Create Increment DATA */
/*				printf("Create Increment DATA ERROR\n");
			}
*/
		}

		/* Set Parameter for MD Send */
		/* Using Subnet2 ? */
		if (pCallerThreadParameter->pCommandValue->mdSendSubnet == MD_SEND_USE_SUBNET2)
		{
			/* Set Subnet2 parameter */
			mdAppHandle = appHandle2;
//			pMdSessionId = NULL;
			mdSrcIpAddr = subnetId2Address;
			mdDestIpAddr = pCallerThreadParameter->pCommandValue->mdDestinationAddress;
			mdOptionFlag = md_config2.flags ;
			mdNoOfRepliers = pCallerThreadParameter->pCommandValue->mdReplierNumber;
			mdReplyTimeout = pCallerThreadParameter->pCommandValue->mdTimeoutReply;
			pMdSendParam = NULL;
			strncpy(mdSourceURI, subnetId2URI, sizeof(subnetId2URI));
			strncpy(mdDestURI, noneURI, sizeof(noneURI));
			callerThreadListener = appThreadSessionHandle2.pMdAppThreadListener;
		}
		else
		{
			/* Set Subnet1 parameter */
			mdAppHandle = appHandle;
//			pMdSessionId = NULL;
			mdSrcIpAddr = subnetId1Address;
			mdDestIpAddr = pCallerThreadParameter->pCommandValue->mdDestinationAddress;
			mdOptionFlag = md_config.flags ;
			mdNoOfRepliers = pCallerThreadParameter->pCommandValue->mdReplierNumber;
			mdReplyTimeout = pCallerThreadParameter->pCommandValue->mdTimeoutReply;
			pMdSendParam = NULL;
			strncpy(mdSourceURI, subnetId1URI, sizeof(subnetId1URI));
			strncpy(mdDestURI, noneURI, sizeof(noneURI));
			callerThreadListener = appThreadSessionHandle.pMdAppThreadListener;
		}

		/* Send MD Transmission Request */
		switch(pCallerThreadParameter->pCommandValue->mdMessageKind)
		{
			case MD_MESSAGE_MN:
				/* Get TimeStamp when call tlm_notify() */
				sprintf(logString, "%s tlm_notify()", vos_getTimeStamp());

				/* Notification */
				err = tlm_notify(
						mdAppHandle,			/* the handle returned by tlc_openSession */
						pMdUserRef,			/* user supplied value returned with reply */
						pCallerThreadParameter->pCommandValue->mdSendComId,	/* comId of packet to be sent */
						mdTopocount,										/* topocount to use */
						mdSrcIpAddr,										/* srcIP Address */
						mdDestIpAddr,										/* where to send the packet to */
						mdOptionFlag,										/* OPTION FLAG */
						pMdSendParam,										/* optional pointer to send parameter */
						(UINT8 *)pCallerThreadParameter->pMdData,		/* pointer to packet data or dataset */
						pCallerThreadParameter->mdDataSize,			/* size of packet data */
						mdSourceURI,										/* source URI */
						mdDestURI);										/* destination URI */
				if (err != TRDP_NO_ERR)
				{
					/* Error : Send Notification */
					printf("Send Notification ERROR\n");
				}

				/* Output LOG : MD Operation Result Log ? */
				if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
					|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
				{
					/* Output MD Operation Result Log */
					l2fLog(logString,
							((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
							((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
				}
				/* Output LOG : Send Log ? */
				if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG) == MD_SEND_LOG)
					|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG) == MD_SEND_LOG))
				{
					/* Send MD DATA String */
					logStringLength = strlen(logString);
					sprintf((char *)(logString + logStringLength),"Send MD DATA\n");
					/* Output Send Log : MD DATA Title */
					l2fLog(logString,
							((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
							((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG));
					/* Output Send Log : MD DATASET */
					miscMemory2String(pCallerThreadParameter->pMdData,
							pCallerThreadParameter->mdDataSize,
							((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
							((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG),
							RECURSIVE_CALL_NOTHING);
				}
				/* Clear Log String */
				memset(logString, 0, sizeof(logString));
				logStringLength = 0;
			break;
			case MD_MESSAGE_MR_MP:
				/* Get TimeStamp when call tlm_request() */
				sprintf(logString, "%s tlm_request()", vos_getTimeStamp());

				/* Request */
				err = tlm_request(
						mdAppHandle,			/* the handle returned by tlc_openSession */
						pMdUserRef,			/* user supplied value returned with reply */
//						pMdSessionId,			/* return session ID */
						&mdSessionId,			/* return session ID */
						pCallerThreadParameter->pCommandValue->mdSendComId,				/* comId of packet to be sent */
						mdTopocount,										/* topocount to use */
						mdSrcIpAddr,										/* srcIP Address */
						mdDestIpAddr,										/* where to send the packet to */
						mdOptionFlag,										/* OPTION FLAG */
						mdNoOfRepliers,									/* number of expected repliers */
						mdReplyTimeout,									/* timeout for reply */
						pMdSendParam,										/* optional pointer to send parameter */
						(UINT8 *)pCallerThreadParameter->pMdData,		/* pointer to packet data or dataset */
						pCallerThreadParameter->mdDataSize,			/* size of packet data */
						mdSourceURI,										/* source URI */
						mdDestURI);										/* destination URI */
				if (err != TRDP_NO_ERR)
				{
					/* Error : Send Request */
					printf("Send Request ERROR\n");
				}

				/* Get Request Thread Session Handle Area */
				pRequestSessionHandle = (APP_THREAD_SESSION_HANDLE *)malloc(sizeof(APP_THREAD_SESSION_HANDLE));
				if (pRequestSessionHandle == NULL)
				{
					printf("Create Request Session Area ERROR. malloc Err\n");
					return 0;
				}
				else
				{
					/* Set Request Session Handle */
					pRequestSessionHandle->pMdAppThreadListener = callerThreadListener;
//					memcpy(pRequestSessionHandle->mdAppThreadSessionId, pMdSessionId, sizeof(TRDP_UUID_T));
					memcpy(pRequestSessionHandle->mdAppThreadSessionId, mdSessionId, sizeof(mdSessionId));
					/* Set Request Session Handle Message Queue Descriptor */
					err = setAppThreadSessionMessageQueueDescriptor(pRequestSessionHandle, callerMqDescriptor);
					if (err != MD_APP_NO_ERR)
					{
						printf("setAppSessionIdMessageQueueDescriptor error\n");
					}
					else
					{
						/* Set mrSendSessionTable */
						for(i=0; i < REQUEST_SESSIONID_TABLE_MAX; i++)
						{
//							if(mrSendSessionTable[i].mdAppThreadSessionId == 0)
							if(pMrSendSessionTable[i]->mdAppThreadSessionId == 0)
							{
								/* Set Request Session Handle */
//								mrSendSessionTable[i] = *pRequestSessionHandle;
								pMrSendSessionTable[i] = pRequestSessionHandle;
								break;
							}
						}
					}
				}

				/* Output LOG : MD Operation Result Log ? */
				if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
					|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
				{
					/* Output MD Operation Result Log */
					l2fLog(logString,
							((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
							((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
				}
				/* Output LOG : Send Log ? */
				if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG) == MD_SEND_LOG)
					|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG) == MD_SEND_LOG))
				{
					/* Send MD DATA String */
					logStringLength = strlen(logString);
					sprintf((char *)(logString + logStringLength), "Send MD DATA\n");
					/* Output Send Log : MD DATA Title */
					l2fLog(logString,
							((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
							((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG));
					/* Output Send Log : MD DATASET */
					miscMemory2String(pCallerThreadParameter->pMdData,
							pCallerThreadParameter->mdDataSize,
							((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
							((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG),
							RECURSIVE_CALL_NOTHING);
				}

				/* Clear Log String */
				memset(logString, 0, sizeof(logString));
				logStringLength = 0;
			break;
			default:
				/* Other than Caller and Replier */
				printf("Caller Replier Type ERROR. mdCallerReplierType = %d\n", pCallerThreadParameter->pCommandValue->mdCallerReplierType);
			break;
		}

		/* MD Transmission Request Send Cycle is unlimited */
		if (pCallerThreadParameter->pCommandValue->mdCycleNumber > 0)
		{
			/* MD Transmission Request SendCount up */
			sendMdTransferRequestCounter++;
		}

		/* Wait Next MD Transmission Send timing */
		vos_getTime(&nextTime);
		tv_interval.tv_sec      = pCallerThreadParameter->pCommandValue->mdCycleTime / 1000000;
		tv_interval.tv_usec     = pCallerThreadParameter->pCommandValue->mdCycleTime % 1000000;
		trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
		trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
		vos_addTime(&nextTime, &trdp_time_tv_interval);

		/* FD ZERO */
		FD_ZERO(&rfds);
		/* FD SET */
		FD_SET(callerMqDescriptor, &rfds);

		rv = select(callerMqDescriptor+1, &rfds, NULL, NULL, &tv_interval);
		if(rv > 0)
		{
			/* Receive Message Queue Message */
			queue_receiveMessage(&receiveMqMsg, callerMqDescriptor);

			/* Output LOG : MD Operation Result Log ? */
			if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
				|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
			{
				/* Output MD Operation Result Log to md_indicate() TimeStamp */
				l2fLog(receiveMqMsg.timeStampString,
						((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
						((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
			}
			/* Output LOG : Receive Log ? */
			if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_RECEIVE_LOG) == MD_RECEIVE_LOG)
					|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_RECEIVE_LOG) == MD_RECEIVE_LOG))
			{
				/* Receive MD DATA String */
				strncpy(logString, receiveMqMsg.timeStampString, sizeof(receiveMqMsg.timeStampString));
				logStringLength = strlen(logString);
				sprintf((char *)(logString + logStringLength), "Receive MD DATA\n");
				/* Output Receive Log : MD DATA Title */
				l2fLog(logString,
						((pCallerThreadParameter->pCommandValue->mdLog) & MD_RECEIVE_LOG),
						((pCallerThreadParameter->pCommandValue->mdDump) & MD_RECEIVE_LOG));
				/* Output Receive Log : MD DATASET */
				miscMemory2String(receiveMqMsg.pData,
						receiveMqMsg.dataSize,
						((pCallerThreadParameter->pCommandValue->mdLog) & MD_RECEIVE_LOG),
						((pCallerThreadParameter->pCommandValue->mdDump) & MD_RECEIVE_LOG),
						RECURSIVE_CALL_NOTHING);
			}
			/* Clear Log String */
			memset(logString, 0, sizeof(logString));
			logStringLength = 0;

			/* Check ComId */
			if (receiveMqMsg.Msg.comId != ((pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK))
			{
				/* ComId Err*/
				printf("Receive ComId ERROR\n");
			}
			else
			{
				/* Check Result Code */
				if (decideResultCode(receiveMqMsg.Msg.resultCode) == MD_APP_NO_ERR)
				{
					/* Check msgType */
					switch(receiveMqMsg.Msg.msgType)
					{
						case TRDP_MSG_MP:
							for(i=0; i < REQUEST_SESSIONID_TABLE_MAX; i++)
							{
								/* Check sessionId */
//								if(mrSendSessionTable[i].mdAppThreadSessionId == receiveMqMsg.Msg.sessionId)
//								if(pMrSendSessionTable[i]->mdAppThreadSessionId == receiveMqMsg.Msg.sessionId)
								if (pMrSendSessionTable[i]->mdAppThreadSessionId == NULL)
								{
									continue;
								}
								else
								{
									if (strncmp((char *)pMrSendSessionTable[i]->mdAppThreadSessionId, (char *)receiveMqMsg.Msg.sessionId, sizeof(pMrSendSessionTable[i]->mdAppThreadSessionId)) == 0)
									{
										/* Set Reply Session : TRUE */
										mrSendSessionFlag = TRUE;
										break;
									}
								}
							}
							/* Reply Session OK ? */
							if (mrSendSessionFlag == TRUE)
							{
								/* nothing */
							}
							else
							{
								printf("Receive Session ERROR\n");
							}
/* Lock MD Application Thread Mutex */
//lockMdApplicationThread();
							/* Decide MD Transmission Result */
							err = decideMdTransmissionResult(
									receiveMqMsg.Msg.comId,
									receiveMqMsg.pData,
									&receiveMqMsg.dataSize,
									logString);
/* UnLock MD Application Thread Mutex */
//unlockMdApplicationThread();
							if (err == MD_APP_NO_ERR)
							{
								/* MD Receive OK Count UP*/
								mdReceiveSuccessCounter++;
							}
							else
							{
								/* MD Receive NG Count */
								mdReceiveFailureCounter++;
							}
							/* MD Receive Count */
							mdReceiveCounter++;
							/* MD Retry Count */

							/* Output LOG : Operation Log */
							if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
								|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
							{
								logStringLength = strlen(logString);
								sprintf((char *)(logString + logStringLength), "MD Receive Count = %u\nMD Receive OK Count = %u\nMD Receive NG Count = %u\nMD Retry Count = %u\n",
										mdReceiveCounter, mdReceiveSuccessCounter, mdReceiveFailureCounter, mdRetryCounter);
								l2fLog(logString,
										((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
										((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
							}
							/* Clear Log String */
							memset(logString, 0, sizeof(logString));
						break;
						case TRDP_MSG_ME:
								/* Error : Receive Me */
								printf("Receive Message Type ERROR. Receive Me\n");
						break;
						default:
							/* Other than Mq and Me */
							printf("Receive Message Type ERROR\n");
						break;
					}
				}
				else
				{
					/* Result Code Err */
				}
				/* Delete mrSendSessionTable */
				for(i=0; i < REQUEST_SESSIONID_TABLE_MAX; i++)
				{
//					if(mrSendSessionTable[i].mdAppThreadSessionId == receiveMqMsg.Msg.sessionId)
//					if(pMrSendSessionTable[i]->mdAppThreadSessionId == receiveMqMsg.Msg.sessionId)
					if (pMrSendSessionTable[i]->mdAppThreadSessionId == NULL)
					{
						continue;
					}
					else
					{
						if (strncmp((char *)pMrSendSessionTable[i]->mdAppThreadSessionId, (char *)receiveMqMsg.Msg.sessionId, sizeof(pMrSendSessionTable[i]->mdAppThreadSessionId)) == 0)
						{
							/* delete Application Thread Session Message Queue Descriptor */
	//						deleteAppThreadSessionMessageQueueDescriptor(&(mrSendSessionTable[i]), callerMqDescriptor);
							deleteAppThreadSessionMessageQueueDescriptor(pMrSendSessionTable[i], callerMqDescriptor);
							/* Clear sessionId */
	//						memset(&(mrSendSessionTable[i].mdAppThreadSessionId), 0, sizeof(TRDP_UUID_T));
							/* Clear Listener */
	//						memset(&(mrSendSessionTable[i].pMdAppThreadListener), 0, sizeof(TRDP_LIS_T));
							break;
						}
					}
				}
			}
			/* Wait Next MD Transmission Send Timing */
			vos_getTime(&nowTime);
			if (vos_cmpTime((TRDP_TIME_T *) &nowTime, (TRDP_TIME_T *) &nextTime) < 0)
			{
				wanted_delay.tv_sec = nextTime.tv_sec - nowTime.tv_sec;
				wanted_delay.tv_nsec = (((nextTime.tv_usec - nextTime.tv_usec) % 1000000) * 1000);
				do
				{
					/* wait */
					err = nanosleep(&wanted_delay, &remaining_delay);
					if (err == -1 && errno == EINTR)
					{
						wanted_delay = remaining_delay;
					}
				}
				while (errno == EINTR);
			}
		}
	}
	return 0;
}

/**********************************************************************************************************************/
/** Caller thread main loop process
 *
 *  @param[in]		mqDescriptor						Message Queue Descriptor
 *  @param[in]		replierComId						Replier ComId
 *  @param[in]		pCallerThreadParameter			pointer to Message Queue Descriptor
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
MD_APP_ERR_TYPE caller_main_proc (
		mqd_t mqDescriptor,
		UINT32 replierComId,
		CALLER_THREAD_PARAMETER *pCallerThreadParameter)
{
	return 0;
}
