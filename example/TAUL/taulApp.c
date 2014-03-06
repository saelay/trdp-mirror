/**********************************************************************************************************************/
/**
 * @file		taulApp.c
 *
 * @brief		Demo TAUL application for TRDP
 *
 * @details	TRDP Ladder Topology Support TAUL Application
 *
 * @note		Project: TCNOpen TRDP prototype stack
 *
 * @author		Kazumasa Aiba, Toshiba Corporation
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
#include <sys/select.h>

#include <mqueue.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "trdp_if_light.h"
#include "vos_thread.h"
#include "trdp_private.h"

#ifdef __linux__
	#include <uuid/uuid.h>
#endif

#include "taulApp.h"

/*******************************************************************************
 * DEFINES
 */
#define SOURCE_URI "user@host"
#define DEST_URI "user@host"

/*******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 *   Locals
 */

/******************************************************************************
 *   Globals
 */

/* Caller Application Thread Message Queue Descriptor Table */
static CALLER_REPLIER_APP_THREAD_HANDLE_T callerAppMqDescriptorTable[MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX] = {{0}};
/* Replier Application Thread Message Queue Descriptor Table */
static CALLER_REPLIER_APP_THREAD_HANDLE_T replierAppMqDescriptorTable[MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX] = {{0}};

/* Application Thread List */
APPLICATION_THREAD_HANDLE_T applicationThreadHandleList[APPLICATION_THREAD_LIST_MAX] = {{0}};

/* Thread Name */
CHAR8 publisherThreadName[] ="PublisherThread";							/* Thread name is Publisher Application Thread. */
CHAR8 subscriberThreadName[] ="SubscriberThread";						/* Thread name is Subscriber Application Thread. */
CHAR8 pdRequesterThreadName[] ="PdRequesterThread";						/* Thread name is PD Requester Application Thread. */
CHAR8 callerThreadName[] ="CallerThread";									/* Thread name is Caller Application Thread. */
CHAR8 replierThreadName[] ="ReplierThread";								/* Thread name is Replier Application Thread. */

/* Create Thread Counter */
UINT32 publisherThreadNoCount = 0;				/* Counter which Create Publisher Thread */
UINT32 subscriberThreadNoCount = 0;			/* Counter which Create Subscriber Thread */
UINT32 pdRequesterThreadNoCount = 0;			/* Counter which Create PD Requester Thread */
UINT32 callerThreadNoCount = 0;					/* Counter which Create Caller Thread */
UINT32 replierThreadNoCount = 0;				/* Counter which Create Replier Thread */

/* Message Queue Name Base */
CHAR8 callerMessageQueueName[] ="/caller_mq";					/* Caller Message Queue Name Base */
CHAR8 replierMessageQueueName[] ="/replier_mq";				/* Replier Message Queue Name Base */

/* Thread Stack Size */
const size_t    threadStackSize   = 256 * 1024;

UINT32	sequenceCounter = 0;										/* MD Send Sequence Counter */


/******************************************************************************
 *   Function
 */

/** Callback for Caller receiving Request Result (Reply, ReplyQuery, Timeout)
  *
 *  @param[in]        *callerRef		pointer to caller reference
 *  @param[in]        pMdInfo			pointer to received message information
 *  @param[in]        pData				pointer to received data
 *  @param[in]        dataLength		size of received data pointer to received data excl. padding and FCS
 *
 *  @retval           none
 *
 */
void callConf (
    void        				*callerRef,
    const TRDP_MD_INFO_T		*pMdInfo,
    const UINT8				*pData,
    UINT32						dataLength)
{
	VOS_QUEUE_T					sendMessageQueueDescriptor = 0;
	CHAR8							timeStamp[64] = {0};
	UINT32							sendMqLoopCounter = 0;
	TRDP_ADDRESSES_T				mdAppThreadListener = {0};
	UINT32							sendMessageSize = 0;
	VOS_ERR_T						vos_err = VOS_NO_ERR;

	/* Get TimeStamp when call md_indication() */
	sprintf(timeStamp, "%s callConf()", vos_getTimeStamp());
	vos_printLog(VOS_LOG_INFO, "callConf(r=%p m=%p d=%p l=%d comId=%u msgType=0x%x"
			" sessionId=%02x%02x%02x%02x%02x%02x%02x%02x,"
			" numExpReplies=%u numReplies=%u numRepliesQuery=%u resuletCode=%d)\n",
			callerRef,
			pMdInfo,
			pData,
			dataLength,
			pMdInfo->comId,
			pMdInfo->msgType,
			pMdInfo->sessionId[0], pMdInfo->sessionId[1], pMdInfo->sessionId[2], pMdInfo->sessionId[3],
			pMdInfo->sessionId[4], pMdInfo->sessionId[5], pMdInfo->sessionId[6], pMdInfo->sessionId[7],
			pMdInfo->numExpReplies,
			pMdInfo->numReplies,
			pMdInfo->numRepliesQuery,
			pMdInfo->resultCode);

	/* Add message to application event queue */
	trdp_apl_cbenv_t *pFwd;
	/* Get Message Queue Send Message memory area */
	pFwd = (trdp_apl_cbenv_t *)vos_memAlloc(sizeof(trdp_apl_cbenv_t));
	if (pFwd == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"callConf() Failed. Message Queue Send Message vos_memAlloc() Err\n");
		return;
	}
	else
	{
		/* Initialize Message Queue Send Message */
		memset(pFwd, 0, sizeof(trdp_apl_cbenv_t));
	}

	/* Get CallerRef memory area */
	pFwd->pRefCon = (UINT32 *)vos_memAlloc(sizeof(UINT32));
	if (pFwd->pRefCon == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"callConf() Failed. callerRef vos_memAlloc() Err\n");
		/* Free Message Queue Send Message */
		vos_memFree(pFwd);
		return;
	}
	else
	{
		/* Initialize CallerRef */
		memset(pFwd->pRefCon, 0, sizeof(UINT32));
	}
	/* Set CallerRef */
	memcpy(pFwd->pRefCon, callerRef, sizeof(UINT32));
	/* Set MD INFO */
	pFwd->Msg = * pMdInfo;
	/* Set DATASET */
	pFwd->pData = pData;
	/* Set Data Size */
	pFwd->dataSize = dataLength;
	/* Initialize Time Stamp area */
	memset(pFwd->timeStampString, 0, sizeof(pFwd->timeStampString));
	/* Set Time Stamp */
	strncpy(pFwd->timeStampString, timeStamp, sizeof(timeStamp));
	/* Get Send Message Size */
	sendMessageSize = sizeof(trdp_apl_cbenv_t);

	/* Set Receive Message Listener */
	mdAppThreadListener.comId = pFwd->Msg.comId;
	mdAppThreadListener.srcIpAddr = pFwd->Msg.srcIpAddr;
	mdAppThreadListener.destIpAddr = pFwd->Msg.destIpAddr;
	/* Send receive MD DATA to Caller or Replier Thread */
	for(sendMqLoopCounter = 0; sendMqLoopCounter <= MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX; sendMqLoopCounter++)
	{
		sendMessageQueueDescriptor = getCallerAppThreadMessageQueueDescriptor(
				&sendMqLoopCounter,
				&mdAppThreadListener);
		if(sendMessageQueueDescriptor <= 0)
		{
/*				vos_printLog(VOS_LOG_ERROR, "Message Queue Descriptor Error. Don't Send Message Queue.\n"); */
		}
		else
		{
			vos_err = vos_queueSend(sendMessageQueueDescriptor, (UINT8 *)pFwd, sendMessageSize);
			if (vos_err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "callConf() Send Message Queue Error: %d\n", vos_err);
				/* Free Message Queue Send Message */
				vos_memFree(pFwd);
				/* Free CallerRef */
				vos_memFree(pFwd->pRefCon);
			}
			else
			{
				vos_printLog(VOS_LOG_DBG, "callConf() Send Message Queue.\n");
			}
			break;
		}
	}
}

/**********************************************************************************************************************/
/** Callback for Replier receiving Request Result
  *
 *  @param[in]        *sessionRef		pointer to session reference
 *  @param[in]        pMdInfo			pointer to received message information
 *  @param[in]        pData				pointer to received data
 *  @param[in]        dataLength		size of received data pointer to received data excl. padding and FCS
 *
 *  @retval           none
 *
 */
void rcvConf(
    void						*sessionRef,
    const TRDP_MD_INFO_T		*pMdInfo,
    const UINT8				*pData,
    UINT32						dataLength)
{
	VOS_QUEUE_T					sendMessageQueueDescriptor = 0;
	CHAR8							timeStamp[64] = {0};
	UINT32							sendMqLoopCounter = 0;
	TRDP_ADDRESSES_T				mdAppThreadListener = {0};
	UINT32							sendMessageSize = 0;
	VOS_ERR_T						vos_err = VOS_NO_ERR;

	/* Get TimeStamp when call md_indication() */
	sprintf(timeStamp, "%s rcvConf()", vos_getTimeStamp());
	vos_printLog(VOS_LOG_INFO, "rcvConf(r=%p m=%p d=%p l=%d comId=%u msgType=0x%x"
			" sessionId=%02x%02x%02x%02x%02x%02x%02x%02x,"
			" numExpReplies=%u numReplies=%u numRepliesQuery=%u resuletCode=%d)\n",
			sessionRef,
			pMdInfo,
			pData,
			dataLength,
			pMdInfo->comId,
			pMdInfo->msgType,
			pMdInfo->sessionId[0], pMdInfo->sessionId[1], pMdInfo->sessionId[2], pMdInfo->sessionId[3],
			pMdInfo->sessionId[4], pMdInfo->sessionId[5], pMdInfo->sessionId[6], pMdInfo->sessionId[7],
			pMdInfo->numExpReplies,
			pMdInfo->numReplies,
			pMdInfo->numRepliesQuery,
			pMdInfo->resultCode);

	/* Add message to application event queue */
	trdp_apl_cbenv_t *pFwd;
	/* Get Message Queue Send Message memory area */
	pFwd = (trdp_apl_cbenv_t *)vos_memAlloc(sizeof(trdp_apl_cbenv_t));
	if (pFwd == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"rcvConf() Failed. Message Queue Send Message vos_memAlloc() Err\n");
		return;
	}
	else
	{
		/* Initialize Message Queue Send Message */
		memset(pFwd, 0, sizeof(trdp_apl_cbenv_t));
	}

	/* Get SessionRef memory area */
	pFwd->pRefCon = (UINT32 *)vos_memAlloc(sizeof(UINT32));
	if (pFwd->pRefCon == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"rcvConf() Failed. sessionRef vos_memAlloc() Err\n");
		/* Free Message Queue Send Message */
		vos_memFree(pFwd);
		return;
	}
	else
	{
		/* Initialize Replier Application Thread Handle */
		memset(pFwd->pRefCon, 0, sizeof(UINT32));
	}
	/* Set SessionRef */
	memcpy(pFwd->pRefCon, sessionRef, sizeof(UINT32));
	pFwd->Msg = * pMdInfo;
	pFwd->pData = pData;
	/* Set Data Size */
	pFwd->dataSize = dataLength;
	/* Initialize Time Stamp area */
	memset(pFwd->timeStampString, 0, sizeof(pFwd->timeStampString));
	/* Set Time Stamp */
	strncpy(pFwd->timeStampString, timeStamp, sizeof(timeStamp));
	/* Get Send Message Size */
	sendMessageSize = sizeof(trdp_apl_cbenv_t);

	/* Set Receive Message Listener */
	mdAppThreadListener.comId = pFwd->Msg.comId;
	mdAppThreadListener.srcIpAddr = pFwd->Msg.srcIpAddr;
	mdAppThreadListener.destIpAddr = pFwd->Msg.destIpAddr;
	/* Send receive MD DATA to Caller or Replier Thread */
	for(sendMqLoopCounter = 0; sendMqLoopCounter <= MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX; sendMqLoopCounter++)
	{
		sendMessageQueueDescriptor = getReplierAppThreadMessageQueueDescriptor(
				&sendMqLoopCounter,
				&mdAppThreadListener);
		if(sendMessageQueueDescriptor <= 0)
		{
/*				vos_printLog(VOS_LOG_ERROR, "Message Queue Descriptor Error. Don't Send Message Queue.\n"); */
		}
		else
		{
			vos_err = vos_queueSend(sendMessageQueueDescriptor, (UINT8 *)pFwd, sendMessageSize);
			if (vos_err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "rcvConf() Send Message Queue Error: %d\n", vos_err);
				/* Free Message Queue Send Message */
				vos_memFree(pFwd);
				/* Free CallerRef */
				vos_memFree(pFwd->pRefCon);
			}
			else
			{
				vos_printLog(VOS_LOG_DBG, "rcvConf() Send Message Queue.\n");
			}
			break;
		}
	}
}

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]		pRefCon		user supplied context pointer
 *  @param[in]		category		Log category (Error, Warning, Info etc.)
 *  @param[in]		pTime			pointer to NULL-terminated string of time stamp
 *  @param[in]		pFile			pointer to NULL-terminated string of source module
 *  @param[in]		LineNumber		line
 *  @param[in]		pMsgStr       pointer to NULL-terminated string
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
    BOOL8 logPrintOnFlag = FALSE;							/* FALSE is not print */
    FILE *fpOutputLog = NULL;								/* pointer to Output Log */
    const TRDP_FILE_NAME_T logFileNameNothing = {0};		/* Log File Name Nothing */

    /* Check Output Log Enable from Debug Config */
    switch(category)
    {
    	case VOS_LOG_ERROR:
			/* Debug Config Option : ERROR or WARNING or INFO or DEBUG */
			if (((debugConfig.option & TRDP_DBG_ERR) == TRDP_DBG_ERR)
				|| ((debugConfig.option & TRDP_DBG_WARN) == TRDP_DBG_WARN)
				|| ((debugConfig.option & TRDP_DBG_INFO) == TRDP_DBG_INFO)
				|| ((debugConfig.option & TRDP_DBG_DBG) == TRDP_DBG_DBG))
			{
				logPrintOnFlag = TRUE;
			}
		break;
    	case VOS_LOG_WARNING:
			/* Debug Config Option : WARNING or INFO or DEBUG */
    		if (((debugConfig.option & TRDP_DBG_WARN) == TRDP_DBG_WARN)
    			|| ((debugConfig.option & TRDP_DBG_INFO) == TRDP_DBG_INFO)
				|| ((debugConfig.option & TRDP_DBG_DBG) == TRDP_DBG_DBG))
			{
				logPrintOnFlag = TRUE;
			}
    	break;
    	case VOS_LOG_INFO:
			/* Debug Config Option : INFO or DBUG */
    		if (((debugConfig.option & TRDP_DBG_INFO) == TRDP_DBG_INFO)
    			|| ((debugConfig.option & TRDP_DBG_DBG) == TRDP_DBG_DBG))
			{
				logPrintOnFlag = TRUE;
			}
    	break;
    	case VOS_LOG_DBG:
			/* Debug Config Option : DEBUG */
			if((debugConfig.option & TRDP_DBG_DBG) == TRDP_DBG_DBG)
			{
				logPrintOnFlag = TRUE;
			}
		break;
    	default:
    	break;
    }

    /* Check log Print */
    if (logPrintOnFlag == TRUE)
    {
    	/* Check output place of Log */
    	if (memcmp(debugConfig.fileName, logFileNameNothing,sizeof(TRDP_FILE_NAME_T)) != 0)
    	{
    	    /* Open file in append mode */
    		fpOutputLog = fopen(debugConfig.fileName, "a");
    	    if (fpOutputLog == NULL)
    	    {
    	    	vos_printLog(VOS_LOG_ERROR, "dbgOut() Log File Open Err\n");
    			return;
    	    }
    	}
    	else
    	{
    		/* Set Output place to Display */
    		fpOutputLog = stdout;
    	}
    	/* Output Log Information */
		/*  Log Date and Time */
		if (debugConfig.option & TRDP_DBG_TIME)
			fprintf(fpOutputLog, "%s ", pTime);
		/*  Log category */
		if (debugConfig.option & TRDP_DBG_CAT)
			fprintf(fpOutputLog, "%s ", catStr[category]);
		/*  Log Source File Name and Line */
		if (debugConfig.option & TRDP_DBG_LOC)
			fprintf(fpOutputLog, "%s:%d ", pFile, LineNumber);
		/*  Log message */
		fprintf(fpOutputLog, "%s", pMsgStr);

		if (fpOutputLog != stdout)
		{
			/* Close Log File */
			fclose(fpOutputLog);
		}
    }
}

/**********************************************************************************************************************/
/** Set Application Thread List
 *
 *  @param[in]		pApplicationThreadHandle				pointer to Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 */
TAUL_APP_ERR_TYPE setApplicationThreadHandleList(
		APPLICATION_THREAD_HANDLE_T *pApplicationThreadHandle)
{
	UINT32 i = 0;

	/* Check Parameter */
	if ((pApplicationThreadHandle == NULL)
		|| (pApplicationThreadHandle->appHandle == NULL)
		|| (pApplicationThreadHandle->applicationThreadHandle == NULL))
	{
		vos_printLog(VOS_LOG_ERROR, "setApplicationThreadHandleList() failed. Parameter Error.\n");
		return TAUL_APP_PARAM_ERR;
	}

	for(i=0; i < APPLICATION_THREAD_LIST_MAX; i++)
	{
		if(applicationThreadHandleList[i].applicationThreadHandle == NULL)
		{
			/* Set appHandle */
			applicationThreadHandleList[i].appHandle = pApplicationThreadHandle->appHandle;
			/* Set Application Thread Handle */
			applicationThreadHandleList[i].applicationThreadHandle = pApplicationThreadHandle->applicationThreadHandle;
			/* Set TAUL Application Thread ID */
			applicationThreadHandleList[i].taulAppThreadId = pApplicationThreadHandle->taulAppThreadId;
			return TAUL_APP_NO_ERR;
		}
	}
	vos_printLog(VOS_LOG_ERROR, "setApplicationThreadHandleList() failed. Don't Set Application Thread Handle.\n");
	return TAUL_APP_ERR;
}

/**********************************************************************************************************************/
/** Return the Application Thread Handle with same TAUL Application Thread ID
 *
 *  @param[in]		taulApplicationThreadId	TAUL Application Thread ID
 *
 *  @retval         != NULL						pointer to Application Thread Handle
 *  @retval         NULL							No Application Thread Handle found
 */
APPLICATION_THREAD_HANDLE_T *searchApplicationThreadHandleList (
		UINT32			taulApplicationThreadId)
{
	UINT32 i = 0;

	for(i=0; i < APPLICATION_THREAD_LIST_MAX; i++)
	{
		if(applicationThreadHandleList[i].taulAppThreadId == taulApplicationThreadId)
		{
			return &applicationThreadHandleList[i];
		}
	}
	vos_printLog(VOS_LOG_ERROR, "searchApplicationThreadHandleList() failed. Don't find Application Thread Handle.\n");
	return NULL;
}

/**********************************************************************************************************************/
/** Set Message Queue Descriptor List for Caller Application
 *
 *  @param[in]		pAppThreadHandle				pointer to MD Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 */
TAUL_APP_ERR_TYPE setCallerAppThreadMessageQueueDescriptor(
		CALLER_REPLIER_APP_THREAD_HANDLE_T *pMdAppThreadHandle)
{
	UINT32 i = 0;

	/* Check Parameter */
	if ((pMdAppThreadHandle == NULL)
		|| (pMdAppThreadHandle->pMdAppThreadListener == NULL)
		|| (pMdAppThreadHandle->pTrdpMqDescriptor == NULL))
	{
		vos_printLog(VOS_LOG_ERROR, "setCallerAppThreadMessageQueueDescriptor() failed. Parameter Error.\n");
	}

	for(i=0; i < MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX; i++)
	{
		if(callerAppMqDescriptorTable[i].pTrdpMqDescriptor == NULL)
		{
			callerAppMqDescriptorTable[i].pMdAppThreadListener = pMdAppThreadHandle->pMdAppThreadListener;
			callerAppMqDescriptorTable[i].pTrdpMqDescriptor = pMdAppThreadHandle->pTrdpMqDescriptor;
			return TAUL_APP_NO_ERR;
		}
	}
	vos_printLog(VOS_LOG_ERROR, "Don't Set Message Queue Descriptor.\n");
	return TAUL_APP_ERR;
}

/**********************************************************************************************************************/
/** Set Message Queue Descriptor List for Replier Application
 *
 *  @param[in]		pAppThreadHandle				pointer to MD Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 */
TAUL_APP_ERR_TYPE setReplierAppThreadMessageQueueDescriptor(
		CALLER_REPLIER_APP_THREAD_HANDLE_T *pMdAppThreadHandle)
{
	UINT32 i = 0;

	/* Check Parameter */
	if ((pMdAppThreadHandle == NULL)
		|| (pMdAppThreadHandle->pMdAppThreadListener == NULL)
		|| (pMdAppThreadHandle->pTrdpMqDescriptor == NULL))
	{
		vos_printLog(VOS_LOG_ERROR, "setCallerAppThreadMessageQueueDescriptor() failed. Parameter Error.\n");
	}

	for(i=0; i < MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX; i++)
	{
		if(replierAppMqDescriptorTable[i].pTrdpMqDescriptor == NULL)
		{
			replierAppMqDescriptorTable[i].pMdAppThreadListener = pMdAppThreadHandle->pMdAppThreadListener;
			replierAppMqDescriptorTable[i].pTrdpMqDescriptor = pMdAppThreadHandle->pTrdpMqDescriptor;
			return TAUL_APP_NO_ERR;
		}
	}
	vos_printLog(VOS_LOG_ERROR, "Don't Set Message Queue Descriptor.\n");
	return TAUL_APP_ERR;
}

/**********************************************************************************************************************/
/** Delete Caller Application Message Queue Descriptor
 *
 *  @param[in]		pMdAppThreadHandle				pointer to MD Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 *
 */
TAUL_APP_ERR_TYPE deleteCallerAppThreadMessageQueueDescriptor(
		CALLER_REPLIER_APP_THREAD_HANDLE_T *pMdAppThreadHandle)
{
	UINT32 i = 0;
	TAUL_APP_ERR_TYPE err = TAUL_APP_NO_ERR;

	/* Check Parameter */
	if ((pMdAppThreadHandle == NULL)
		|| (pMdAppThreadHandle->pMdAppThreadListener == NULL)
		|| (pMdAppThreadHandle->pTrdpMqDescriptor == NULL))
	{
		vos_printLog(VOS_LOG_ERROR, "deleteMdAppThreadMessageQueueDescriptor() failed. Parameter Error.\n");
		return TAUL_APP_PARAM_ERR;
	}

	for(i = 0; i < MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX; i++)
	{
		/* Check pMdAppThreadListener */
		if (callerAppMqDescriptorTable[i].pMdAppThreadListener == NULL)
		{
			/* pMdAppThereaListener Noting */
			continue;
		}
		else
		{
			/* Matching ComId : equal */
			if (callerAppMqDescriptorTable[i].pMdAppThreadListener->comId == pMdAppThreadHandle->pMdAppThreadListener->comId)
			{
				/* Matching Source IP Address : equal or nothing */
				if ((callerAppMqDescriptorTable[i].pMdAppThreadListener->srcIpAddr == pMdAppThreadHandle->pMdAppThreadListener->srcIpAddr)
					||(callerAppMqDescriptorTable[i].pMdAppThreadListener->srcIpAddr == IP_ADDRESS_NOTHING))
				{
					/* Matching Destination IP Address : equal */
					if (callerAppMqDescriptorTable[i].pMdAppThreadListener->destIpAddr == pMdAppThreadHandle->pMdAppThreadListener->destIpAddr)
					{
						/* Check Message Queue Descriptor : equal*/
						if (callerAppMqDescriptorTable[i].pTrdpMqDescriptor == pMdAppThreadHandle->pTrdpMqDescriptor)
						{
							/* Free MD Application Thread Listener Area */
							vos_memFree(callerAppMqDescriptorTable[i].pMdAppThreadListener);
							callerAppMqDescriptorTable[i].pMdAppThreadListener = NULL;
							/* Free Message Queue Descriptor Area */
							vos_memFree(callerAppMqDescriptorTable[i].pTrdpMqDescriptor);
							callerAppMqDescriptorTable[i].pTrdpMqDescriptor = NULL;
							/* Clear appThreadSessionHandle */
							memset(&(callerAppMqDescriptorTable[i]), 0, sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
							err = TAUL_APP_NO_ERR;
						}
					}
				}
			}
		}
	}
	return err;
}

/**********************************************************************************************************************/
/** Delete Replier Application Message Queue Descriptor
 *
 *  @param[in]		pMdAppThreadHandle				pointer to MD Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 *
 */
TAUL_APP_ERR_TYPE deleteReplierAppThreadMessageQueueDescriptor(
		CALLER_REPLIER_APP_THREAD_HANDLE_T *pMdAppThreadHandle)
{
	UINT32 i = 0;
	TAUL_APP_ERR_TYPE err = TAUL_APP_NO_ERR;

	/* Check Parameter */
	if ((pMdAppThreadHandle == NULL)
		|| (pMdAppThreadHandle->pMdAppThreadListener == NULL)
		|| (pMdAppThreadHandle->pTrdpMqDescriptor == NULL))
	{
		vos_printLog(VOS_LOG_ERROR, "deleteMdAppThreadMessageQueueDescriptor() failed. Parameter Error.\n");
		return TAUL_APP_PARAM_ERR;
	}

	for(i = 0; i < MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX; i++)
	{
		/* Check pMdAppThreadListener */
		if (replierAppMqDescriptorTable[i].pMdAppThreadListener == NULL)
		{
			/* pMdAppThereaListener Noting */
			continue;
		}
		else
		{
			/* Matching ComId : equal */
			if (replierAppMqDescriptorTable[i].pMdAppThreadListener->comId == pMdAppThreadHandle->pMdAppThreadListener->comId)
			{
				/* Matching Source IP Address : equal or nothing */
				if ((replierAppMqDescriptorTable[i].pMdAppThreadListener->srcIpAddr == pMdAppThreadHandle->pMdAppThreadListener->srcIpAddr)
					||(replierAppMqDescriptorTable[i].pMdAppThreadListener->srcIpAddr == IP_ADDRESS_NOTHING))
				{
					/* Matching Destination IP Address : equal */
					if (replierAppMqDescriptorTable[i].pMdAppThreadListener->destIpAddr == pMdAppThreadHandle->pMdAppThreadListener->destIpAddr)
					{
						/* Check Message Queue Descriptor : equal*/
						if (replierAppMqDescriptorTable[i].pTrdpMqDescriptor == pMdAppThreadHandle->pTrdpMqDescriptor)
						{
							/* Free MD Application Thread Listener Area */
							vos_memFree(replierAppMqDescriptorTable[i].pMdAppThreadListener);
							replierAppMqDescriptorTable[i].pMdAppThreadListener = NULL;
							/* Free Message Queue Descriptor Area */
							vos_memFree(replierAppMqDescriptorTable[i].pTrdpMqDescriptor);
							replierAppMqDescriptorTable[i].pTrdpMqDescriptor = NULL;
							/* Clear appThreadSessionHandle */
							memset(&(replierAppMqDescriptorTable[i]), 0, sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
							err = TAUL_APP_NO_ERR;
						}
					}
				}
			}
		}
	}
	return err;
}

/**********************************************************************************************************************/
/** Get Caller Application Message Queue Descriptor
 *
 *  @param[in,out]	pLoopStartNumber				pointer to Loop Starting Number
 *  @param[in]		pMdAppThreadListener			pointer to MD Application Thread Listener
 *
 *  @retval         VOS_QUEUE_T						pointer to TRDP Message Queue Descriptor
 *  @retval         NULL								error
 */
VOS_QUEUE_T getCallerAppThreadMessageQueueDescriptor(
		UINT32 *pLoopStartNumber,
//		TRDP_LIS_T pMdAppThreadListener)
		COMID_IP_HANDLE_T pMdAppThreadListener)
{
	UINT32 i = 0;

	/* Check Parameter */
	if ((pMdAppThreadListener == NULL))
//		|| (*pLoopStartNumber < 0))
	{
		vos_printLog(VOS_LOG_ERROR, "getMdAppThreadMessageQueueDescriptor() failed. Parameter Error.\n");
		return NULL;
	}

	for(i = *pLoopStartNumber; i < MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX; i++)
	{
		/* Check Table Listener == NULL*/
		if (callerAppMqDescriptorTable[i].pMdAppThreadListener != NULL)
		{
			/* Check Listener */
			/* Matching ComId : equal */
			if (callerAppMqDescriptorTable[i].pMdAppThreadListener->comId == pMdAppThreadListener->comId)
			{
				/* Matching Source IP Address : equal or nothing */
				if ((callerAppMqDescriptorTable[i].pMdAppThreadListener->srcIpAddr == pMdAppThreadListener->srcIpAddr)
					||(callerAppMqDescriptorTable[i].pMdAppThreadListener->srcIpAddr == IP_ADDRESS_NOTHING))
				{
					/* Matching Destination IP Address : equal */
					if ((callerAppMqDescriptorTable[i].pMdAppThreadListener->destIpAddr == pMdAppThreadListener->destIpAddr)
						||(pMdAppThreadListener->destIpAddr == IP_ADDRESS_NOTHING)
						||(callerAppMqDescriptorTable[i].pMdAppThreadListener->destIpAddr == IP_ADDRESS_NOTHING))
					{
							*pLoopStartNumber = i;
							return callerAppMqDescriptorTable[i].pTrdpMqDescriptor;
					}
				}
			}
		}
	}
	return NULL;
}

/**********************************************************************************************************************/
/** Get Replier Application Message Queue Descriptor
 *
 *  @param[in,out]	pLoopStartNumber				pointer to Loop Starting Number
 *  @param[in]		pMdAppThreadListener			pointer to MD Application Thread Listener
 *
 *  @retval         VOS_QUEUE_T						pointer to TRDP Message Queue Descriptor
 *  @retval         NULL								error
 */
VOS_QUEUE_T getReplierAppThreadMessageQueueDescriptor(
		UINT32 *pLoopStartNumber,
//		TRDP_LIS_T pMdAppThreadListener)
		COMID_IP_HANDLE_T pMdAppThreadListener)
{
	UINT32 i = 0;

	/* Check Parameter */
	if ((pMdAppThreadListener == NULL))
//		|| (*pLoopStartNumber < 0))
	{
		vos_printLog(VOS_LOG_ERROR, "getMdAppThreadMessageQueueDescriptor() failed. Parameter Error.\n");
		return NULL;
	}

	for(i = *pLoopStartNumber; i < MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX; i++)
	{
		/* Check Table Listener == NULL*/
		if (replierAppMqDescriptorTable[i].pMdAppThreadListener != NULL)
		{
			/* Check Listener */
			/* Matching ComId : equal */
			if (replierAppMqDescriptorTable[i].pMdAppThreadListener->comId == pMdAppThreadListener->comId)
			{
				/* Matching Source IP Address : equal or nothing */
				if ((replierAppMqDescriptorTable[i].pMdAppThreadListener->srcIpAddr == pMdAppThreadListener->srcIpAddr)
					||(replierAppMqDescriptorTable[i].pMdAppThreadListener->srcIpAddr == IP_ADDRESS_NOTHING))
				{
					/* Matching Destination IP Address : equal */
					if ((replierAppMqDescriptorTable[i].pMdAppThreadListener->destIpAddr == pMdAppThreadListener->destIpAddr)
						||(pMdAppThreadListener->destIpAddr == IP_ADDRESS_NOTHING))
					{
						*pLoopStartNumber = i;
						return replierAppMqDescriptorTable[i].pTrdpMqDescriptor;
					}
				}
			}
		}
	}
	return NULL;
}

/**********************************************************************************************************************/
/** Set Receive Reply Handle
 *
 *  @param[in]		pReceiveReplyHandleTable				pointer to Receive Reply Result Table
 *  @param[in]		receiveReplySessionId				Receive Reply SessionId
 *  @param[in]		receiveReplyNumReplies				Receive Reply Number of Repliers
 *  @param[in]		receiveReplyQueryNumRepliesQuery	Receive ReplyQuery Number of Repliers Query
 *  @param[in]		decideMdTranssmissionResultcode		Receive Reply deceideMdTranssimision() ResultCode
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_ERR							error
 *  @retval			TAUL_APP_PARAM_ERR					Parameter error
 *
 */
TAUL_APP_ERR_TYPE setReceiveReplyHandleTable(
		RECEIVE_REPLY_HANDLE_T *pReceiveReplyHandleTable,
		UINT32 sessionRefValue,
		UINT32 receiveReplyNumReplies,
		UINT32 receiveReplyQueryNumRepliesQuery,
		TAUL_APP_ERR_TYPE decideMdTranssmissionResutlCode)
{
	UINT32 i = 0;

	/* Parameter Check */
	if (pReceiveReplyHandleTable == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "setReceiveReplyHandleTable() parameter err. Mp Receive Session Table err.\n");
		return TAUL_APP_PARAM_ERR;
	}

	for(i=0; i < RECEIVE_REPLY_HANDLE_TABLE_MAX; i++)
	{
		if((pReceiveReplyHandleTable[i].callerReceiveReplyNumReplies == 0)
			&& (pReceiveReplyHandleTable[i].callerReceiveReplyQueryNumRepliesQuery == 0))
		{
			pReceiveReplyHandleTable[i].sessionRefValue = sessionRefValue;
			pReceiveReplyHandleTable[i].callerReceiveReplyNumReplies = receiveReplyNumReplies;
			pReceiveReplyHandleTable[i].callerReceiveReplyQueryNumRepliesQuery = receiveReplyQueryNumRepliesQuery;
			pReceiveReplyHandleTable[i].callerDecideMdTranssmissionResultCode = decideMdTranssmissionResutlCode;
			return TAUL_APP_NO_ERR;
		}
	}
	vos_printLog(VOS_LOG_ERROR, "Don't Set Receive Reply Result Table.\n");
	return TAUL_APP_ERR;
}

/**********************************************************************************************************************/
/** Delete Receive Reply Handle
 *
 *  @param[in]		pReceiveReplyHandleTable				pointer to Receive Reply Handle Table
 *  @param[in]		sessionRefValue						Delete Receive Reply session Reference Value
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_PARAM_ERR					Parameter error
 *
 */
TAUL_APP_ERR_TYPE deleteReceiveReplyHandleTable(
		RECEIVE_REPLY_HANDLE_T *pReceiveReplyHandleTable,
		UINT32 sessionRefValue)
{
	UINT32 receiveTableLoopCounter = 0;			/* Receive Table Loop Counter */

	/* Parameter Check */
	if (pReceiveReplyHandleTable == NULL)
	{
		return TAUL_APP_PARAM_ERR;
	}

	/* Receive Table Loop */
	for(receiveTableLoopCounter = 0; receiveTableLoopCounter < RECEIVE_REPLY_HANDLE_TABLE_MAX; receiveTableLoopCounter++)
	{
		if (pReceiveReplyHandleTable[receiveTableLoopCounter].sessionRefValue== 0)
		{
			continue;
		}
		/* Check Reference : Request CallerRef equal Reply SessionRef */
		if (pReceiveReplyHandleTable[receiveTableLoopCounter].sessionRefValue == sessionRefValue)
		{
			/* delete ReceiveReplyResultTable element */
			memset(&pReceiveReplyHandleTable[receiveTableLoopCounter], 0, sizeof(RECEIVE_REPLY_HANDLE_T));
		}
	}
	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete Mr(Request) Send Session Table
 *
 *  @param[in]		pMrSendSessionTable					pointer to Mr Send Handle Table Start Address
 *  @param[in]		deleteSendRequestSessionId			delete Send Request Caller Reference
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_PARAM_ERR					Parameter error
 *
 */
TAUL_APP_ERR_TYPE deleteMrSendHandleTable(
		SEND_REQUEST_HANDLE_T **ppMrSendHandleTable,
		UINT32 callerRefValue)
{
	UINT32 sendTableLoopCounter = 0;			/* Send Table Loop Counter */

	/* Parameter Check */
	if (ppMrSendHandleTable == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "deleteMrSendHandleTable() parameter err. Mr Send Session Table err.\n");
		return TAUL_APP_PARAM_ERR;
	}

	/* Send Table Loop */
	for(sendTableLoopCounter = 0; sendTableLoopCounter < REQUEST_HANDLE_TABLE_MAX; sendTableLoopCounter++)
	{
		if (ppMrSendHandleTable[sendTableLoopCounter] == NULL)
		{
			continue;
		}
		/* Check CallerRefValue : Request CallerRefValue equal Reply CallerRef */
		if (ppMrSendHandleTable[sendTableLoopCounter]->callerRefValue == callerRefValue)
		{
			/* delete MrSendSessionTable element */
			memset(ppMrSendHandleTable[sendTableLoopCounter], 0, sizeof(SEND_REQUEST_HANDLE_T));
			/* Free Mr Send Session Handle Area */
			vos_memFree(ppMrSendHandleTable[sendTableLoopCounter]);
			ppMrSendHandleTable[sendTableLoopCounter] = NULL;
		}
	}

	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Decide MD Transmission Success or Failure
 *
 *  @param[in]		pReceiveMdData			pointer to Receive MD DATA
 *  @param[in]		pReceiveMdDataSize		pointer to Receive MD DATA Size
 *
 *  @retval         TAUL_APP_NO_ERR				Success
 *  @retval         TAUL_APP_ERR					Failure
 *
 */
TAUL_APP_ERR_TYPE decideMdTransmissionResult(
		const UINT8 *pReceiveMdData,
		const UINT32 *pReceiveMdDataSize)
{

	TAUL_APP_ERR_TYPE err = TAUL_APP_NO_ERR;		/* MD Application Result Code */
#if 0
	UINT8 *pCheckMdData = NULL;
	UINT32 *pCheckMdDataSize = NULL;
	UINT32 receiveMdDataSetSize = 0;
	UINT32 receiveMdDataSetId = 0;

	/* Get MD DATASET Size */
	memcpy(&receiveMdDataSetSize, pReceiveMdDataSize, sizeof(UINT32));

	/* Check MD DATASET Size */
	if (receiveMdDataSetSize < MD_DATASETID_SIZE)
	{
		/* DataSetId nothing : increment Data size under 3 byte */
		/* not Check compare */
		return TAUL_APP_NO_ERR;
	}

	/* Get DataSetId */
	memcpy(&receiveMdDataSetId, pReceiveMdData, sizeof(UINT32));
	/* unmarsahralling DataSetId */
	receiveMdDataSetId = vos_ntohl(receiveMdDataSetId);

	/* Create for check MD DATA */
	err = getMdDataFromDataSetId(
			receiveMdDataSetId,
			pReceiveMdData,
			&receiveMdDataSetSize,
			&pCheckMdData,
			&pCheckMdDataSize);
	if (err != TAUL_APP_NO_ERR)
	{
		/* Error : Create Increment DATA */
		vos_printLog(VOS_LOG_DBG, "<NG> Receive MD DATA error. Create Check MD DATA Err.\n");
	}
	else
	{
		/* Check compare size*/
		if (receiveMdDataSetSize != *pCheckMdDataSize)
		{
			vos_printLog(VOS_LOG_DBG, "<NG> Receive MD DATA error. The size of is different.\n");
			err = TAUL_APP_ERR;
		}
		else
		{
			/* Compare Contents */
			if (memcmp(pReceiveMdData, pCheckMdData, receiveMdDataSetSize) != 0)
			{
				vos_printLog(VOS_LOG_DBG, "<NG> Receive MD error. Contents is different.\n");
				return TAUL_APP_ERR;
			}
			else
			{
				vos_printLog(VOS_LOG_DBG,"<OK> Receive MD DATA normal.\n");
				err = TAUL_APP_NO_ERR;
			}
		}
	}

	if (pCheckMdDataSize != NULL)
	{
		/* Free check MD DATA Size Area */
		vos_memFree(pCheckMdDataSize);
		pCheckMdDataSize = NULL;
	}
	if (pCheckMdData != NULL)
	{
		/* Free check MD DATA Area */
		vos_memFree(pCheckMdData);
		pCheckMdData = NULL;
	}
#endif
	return err;
}

/**********************************************************************************************************************/
/** Decide Request-Reply Result
 *
 *  @param[in]		pMrSendHandleTable				pointer to Mr Send Handle Table
 *  @param[in]		pReceiveReplyHandleTable			pointer to Mp Receive Reply Handle Table
 *  @param[in]		pMdAppParameter					pointer to Application Parameter for wirte Mr-Mp Result
 *
 *  @retval			TAUL_APP_NO_ERR					Mr-Mp OK or deciding
 *  @retval			TAUL_APP_ERR						Mr-Mp NG
 *  @retval			TAUL_APP_PARAM_ERR				Parameter error
 *
 */
TAUL_APP_ERR_TYPE decideRequestReplyResult (
		SEND_REQUEST_HANDLE_T **ppMrSendHandleTable,
		RECEIVE_REPLY_HANDLE_T *pReceiveReplyHandleTable,
		MD_APP_PARAMETER_T *pAppParamter)
{
	TAUL_APP_ERR_TYPE err = TAUL_APP_ERR;
	UINT32 sendTableLoopCounter = 0;			/* Send Table Loop Counter */
	UINT32 receiveTableLoopCounter = 0;		/* Receive Table Loop Counter */

	/* Parameter Check */
	if ((ppMrSendHandleTable == NULL) || (pReceiveReplyHandleTable == NULL))
	{
		vos_printLog(VOS_LOG_ERROR, "decideRequestReplyResult() parameter err. Mr Send Session Table or Mp Receive Session Table err.\n");
		return TAUL_APP_PARAM_ERR;
	}

	/* Send Table Loop */
	for(sendTableLoopCounter = 0; sendTableLoopCounter < REQUEST_HANDLE_TABLE_MAX; sendTableLoopCounter++)
	{
		if (ppMrSendHandleTable[sendTableLoopCounter] == NULL)
		{
			continue;
		}

		/* Receive Table Loop */
		for(receiveTableLoopCounter = 0; receiveTableLoopCounter < RECEIVE_REPLY_HANDLE_TABLE_MAX; receiveTableLoopCounter++)
		{
			if ((pReceiveReplyHandleTable[receiveTableLoopCounter].callerReceiveReplyNumReplies == 0)
				&& (pReceiveReplyHandleTable[receiveTableLoopCounter].callerReceiveReplyQueryNumRepliesQuery == 0)
				&& (pReceiveReplyHandleTable[receiveTableLoopCounter].callerDecideMdTranssmissionResultCode == TAUL_APP_NO_ERR))
			{
				continue;
			}

			/* Check Reference : Request CallerRef equal Reply SessionRef */
			if (pReceiveReplyHandleTable[receiveTableLoopCounter].sessionRefValue == ppMrSendHandleTable[sendTableLoopCounter]->callerRefValue)
			{
				/* decideMdTransmission() Result Code : Success */
				if (pReceiveReplyHandleTable[receiveTableLoopCounter].callerDecideMdTranssmissionResultCode == TAUL_APP_NO_ERR)
				{
					/* Success Count Up */
					if (pReceiveReplyHandleTable[receiveTableLoopCounter].callerReceiveReplyNumReplies > 0)
					{
						/* Receive Reply Number of Repliers */
						ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionSuccessCount = pReceiveReplyHandleTable[receiveTableLoopCounter].callerReceiveReplyNumReplies;
					}
					else
					{
						/* Receive ReplyQuery Number of RepliersQuery */
						ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionSuccessCount = pReceiveReplyHandleTable[receiveTableLoopCounter].callerReceiveReplyQueryNumRepliesQuery;
					}
				}
				/* Receive Timeout (Mr-Mp One Cycle End) */
				else if (pReceiveReplyHandleTable[receiveTableLoopCounter].callerDecideMdTranssmissionResultCode == TAUL_APP_MRMP_ONE_CYCLE_ERR)
				{
					/* Set decideRepliersUnKnownReceiveTimeout : True */
					ppMrSendHandleTable[sendTableLoopCounter]->decideRepliersUnKnownReceiveTimeoutFlag = TRUE;
					/* No Repliers ? */
					if ((pReceiveReplyHandleTable[receiveTableLoopCounter].callerReceiveReplyNumReplies <= 0)
						&& (pReceiveReplyHandleTable[receiveTableLoopCounter].callerReceiveReplyQueryNumRepliesQuery <= 0))
					{
						/* Set decideReliersUnKnownStatus : Falier Finish*/
						ppMrSendHandleTable[sendTableLoopCounter]->decideRepliersUnKnownStatus = MD_REPLIERS_UNKNOWN_FAILURE;
					}
				}
				else
				{
					/* Failure Count Up */
					ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionFailureCount++;
				}
				/* Receive Table Delete */
				deleteReceiveReplyHandleTable(pReceiveReplyHandleTable,
												ppMrSendHandleTable[sendTableLoopCounter]->callerRefValue);
			}
		}

		/* Repliers Check */
		/* Point to Point */
		if (ppMrSendHandleTable[sendTableLoopCounter]->sendRequestNumExpReplies == 1)
		{
			/* Single replier decideMdTranssmission Result Code Success */
			/* and Receive Reply */
			if (ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionSuccessCount == 1)
			{
				/* Request - Reply Success */
				err = TAUL_APP_NO_ERR;
				pAppParamter->callerMdRequestReplySuccessCounter++;
				/* Send Table Delete */
				deleteMrSendHandleTable(ppMrSendHandleTable,
											ppMrSendHandleTable[sendTableLoopCounter]->callerRefValue);
			}
			/* Result Code Failure */
			else if(ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionFailureCount > 0)
			{
				/* Request - Reply Failure */
				err = TAUL_APP_ERR;
				pAppParamter->callerMdRequestReplyFailureCounter++;
				/* Send Table Delete */
				deleteMrSendHandleTable(ppMrSendHandleTable,
										ppMrSendHandleTable[sendTableLoopCounter]->callerRefValue);
			}
			/* Not Receive */
			else if((ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionSuccessCount == 0)
					&& (ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionFailureCount == 0))
			{
				/* nothing */
				err = TAUL_APP_NO_ERR;
			}
		}
		/* Point to Multi Point Unkown Repliers */
		else if(ppMrSendHandleTable[sendTableLoopCounter]->sendRequestNumExpReplies == 0)
		{
			/* All Repliers decideMdTranssmission Result Code Success */
			if ((ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionSuccessCount > 0)
					&& (ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionFailureCount == 0))
			{
				/* Request - Reply Success */
				err = TAUL_APP_NO_ERR;
				/* First Success */
				/* and Receive Reply */
				if ((ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionSuccessCount == 1)
					&& (ppMrSendHandleTable[sendTableLoopCounter]->decideRepliersUnKnownStatus == MD_REPLIERS_UNKNOWN_INITIAL))
				{
					/* Increment Success Counter */
					pAppParamter->callerMdRequestReplySuccessCounter++;
					/* Set decideRepliersUnKnownStatus : success during */
					ppMrSendHandleTable[sendTableLoopCounter]->decideRepliersUnKnownStatus = MD_REPLIERS_UNKNOWN_SUCCESS;
				}
			}
			/* Result Code Failure */
			else if (ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionFailureCount > 0)
			{
				if (ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionSuccessCount > 0)
				{
					/* Receive Reply */
					/* Decrement Success Counter */
					pAppParamter->callerMdRequestReplySuccessCounter--;
					/* Request - Reply Failure */
					err = TAUL_APP_ERR;
					pAppParamter->callerMdRequestReplyFailureCounter++;
					/* Set decideRepliersUnKnownStatus : failure during */
					ppMrSendHandleTable[sendTableLoopCounter]->decideRepliersUnKnownStatus = MD_REPLIERS_UNKNOWN_FAILURE;
				}
				else
				{
					/* Request - Reply Failure */
					err = TAUL_APP_ERR;
					pAppParamter->callerMdRequestReplyFailureCounter++;
					/* Set decideRepliersUnKnownStatus : failure during */
					ppMrSendHandleTable[sendTableLoopCounter]->decideRepliersUnKnownStatus = MD_REPLIERS_UNKNOWN_FAILURE;
				}
			}
			else
			{

			}
			/* Receive Reply Timeout (Mr-Mp One Cycle end) ? */
			if (ppMrSendHandleTable[sendTableLoopCounter]->decideRepliersUnKnownReceiveTimeoutFlag == TRUE)
			{
				/* Check decideReplierUnKnownStatus is Success During (Mr-Mp Success) */
				if (ppMrSendHandleTable[sendTableLoopCounter]->decideRepliersUnKnownStatus == MD_REPLIERS_UNKNOWN_SUCCESS)
				{
					/* Increment Success Counter */
					pAppParamter->callerMdRequestReplySuccessCounter++;
				}
				else
				{
					/* Increment Failure Counter */
					pAppParamter->callerMdRequestReplyFailureCounter++;
				}
				/* Send Table Delete */
				deleteMrSendHandleTable(ppMrSendHandleTable,
											ppMrSendHandleTable[sendTableLoopCounter]->callerRefValue);
			}
		}
		/* Point to Multi Point Known Repliers */
		else
		{
			/* All Repliers decideMdTranssmission Result Code Success */
			/* and Receive Reply */
			if (ppMrSendHandleTable[sendTableLoopCounter]->sendRequestNumExpReplies == ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionSuccessCount)
			{
				/* Request - Reply Success */
				err = TAUL_APP_NO_ERR;
				pAppParamter->callerMdRequestReplySuccessCounter++;
				/* Send Table Delete */
				deleteMrSendHandleTable(ppMrSendHandleTable,
											ppMrSendHandleTable[sendTableLoopCounter]->callerRefValue);
			}
			/* Result Code Failure */
			else if (ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionFailureCount > 0)
			{
				/* Request - Reply Failure */
				err = TAUL_APP_ERR;
				pAppParamter->callerMdRequestReplyFailureCounter++;
				/* Send Table Delete */
				deleteMrSendHandleTable(ppMrSendHandleTable,
											ppMrSendHandleTable[sendTableLoopCounter]->callerRefValue);
			}
			/* any Replier Receiving */
			else if ((ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionSuccessCount < ppMrSendHandleTable[sendTableLoopCounter]->sendRequestNumExpReplies)
				&& (ppMrSendHandleTable[sendTableLoopCounter]->decidedSessionFailureCount == 0))
			{
				err = TAUL_APP_NO_ERR;
			}
		}
	}
	return err;
}

/**********************************************************************************************************************/
/** Application Thread Common Function
 */

/**********************************************************************************************************************/
/** CreateDataSet
 *
 *  @param[in]		firstElementValue			First Element Valeu in Create Dataset
 *  @param[in]		pDatasetDesc				pointer to Created Dataset Descriptor
 *  @param[out]		pDataset					pointer to Created Dataset
 *  @param[in,out]	pDstEnd					Pointer to Destination End Address (NULL:First Call, Not Null:processing)
 *
 *  @retval         TAUL_APP_NO_ERR			no error
 *  @retval         TAUL_APP_PARAM_ERR			Parameter error
 *
 */
TAUL_APP_ERR_TYPE createDataset (
		UINT32					firstElementValue,
		TRDP_DATASET_T		*pDatasetDesc,
		DATASET_T				*pDataset,
		UINT8					*pDstEnd)
{
	TRDP_ERR_T		err = TRDP_NO_ERR;
	UINT32 		setValue = 0;
	UINT16			lIndex = 0;
	UINT8			alignment = 0;
	UINT8			mod = 0;
	UINT32			dstEndAddress = 0;
	UINT32			var_size = 0;
	UINT8			*pWorkEndAddr = NULL;
	UINT32			workEndAddr = 0;

	/* Check Parameter */
	if ((pDatasetDesc == NULL)
		|| (pDataset == NULL))
	{
		vos_printLog(VOS_LOG_ERROR,"createDataset() Failed. Parameter Err.\n");
		return TAUL_APP_PARAM_ERR;
	}
	/* Get dst Address */
	memcpy(&pWorkEndAddr, pDstEnd, sizeof(UINT32));

	/* Set first Value */
	setValue = firstElementValue;

	/*    Loop over all Datasets in the array    */
	for (lIndex = 0; lIndex < pDatasetDesc->numElement; ++lIndex)
	{
		UINT32 noOfItems = pDatasetDesc->pElement[lIndex].size;

/* Get Dst End Address */
workEndAddr = (UINT32)pWorkEndAddr;
/* Set Dst End Address for return */
memcpy(pDstEnd, &workEndAddr, sizeof(UINT32));

		if (TDRP_VAR_SIZE == noOfItems) /* variable size    */
		{
			noOfItems = var_size;
		}

		/*    Is this a composite type?    */
		if (pDatasetDesc->pElement[lIndex].type > (UINT32) TRDP_TYPE_MAX)
		{
			while (noOfItems-- > 0)
			{
				/* Dataset, call ourself recursively */

				/* Never used before?  */
				if (NULL == pDatasetDesc->pElement[lIndex].pCachedDS)
				{
					/* Look for it   */
					pDatasetDesc->pElement[lIndex].pCachedDS = find_DS(pDatasetDesc->pElement[lIndex].type);
				}

				if (NULL == pDatasetDesc->pElement[lIndex].pCachedDS)      /* Not in our DB    */
				{
					vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDatasetDesc->pElement[lIndex].type);
					return TRDP_COMID_ERR;
				}

				err = createDataset(setValue, pDatasetDesc->pElement[lIndex].pCachedDS, pDataset, pDstEnd);
				if (err != TRDP_NO_ERR)
				{
					return err;
				}
			}
		}
		else
		{
			/* Element Type */
			switch (pDatasetDesc->pElement[lIndex].type)
			{
				case TRDP_BOOL8:
				case TRDP_CHAR8:
				case TRDP_INT8:
				case TRDP_UINT8:
				{
					/* Value Size: 1Byte */
					while (noOfItems-- > 0)
					{
						/* Set Value */
						*pWorkEndAddr = (CHAR8)setValue;
						/* Increment value */
						setValue++;
						/* Set Destination End Address */
						pWorkEndAddr++;
					}
					break;
				}
				case TRDP_UTF16:
				case TRDP_INT16:
				case TRDP_UINT16:
				{
					/* Value Size: 2Byte */
					while (noOfItems-- > 0)
					{
						/* Get Write start Address */
						dstEndAddress = (UINT32)pWorkEndAddr;
						/* Adjust alignment */
						mod = dstEndAddress % 2;
						if (mod != 0)
						{
							/* Set Alignment */
							pWorkEndAddr++;
						}
						/* Set Value */
						*pWorkEndAddr = (UINT16)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 2;
					}
					break;
				}
				case TRDP_INT32:
				case TRDP_UINT32:
				case TRDP_REAL32:
				case TRDP_TIMEDATE32:
				{
					/* Value Size: 4Byte */
					while (noOfItems-- > 0)
					{
						/* Get Write Address */
						dstEndAddress = (UINT32)pWorkEndAddr;
						/* Adjust alignment */
						mod = dstEndAddress % 4;
						if (mod != 0)
						{
							/* Set Alignment Size */
							alignment = 4 - mod;
							/* Set Alignment */
							pWorkEndAddr = pWorkEndAddr + alignment;
						}
						/* Set Value */
						*pWorkEndAddr = (UINT32)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 4;
					}
					break;
				}
				case TRDP_TIMEDATE64:
				{
					/* Value Size: 4Byte + 4Byte */
					while (noOfItems-- > 0)
					{
						/* Get start Address */
						dstEndAddress = (UINT32)pWorkEndAddr;
						/* Adjust alignment */
						mod = dstEndAddress % 4;
						if (mod != 0)
						{
							/* Set Alignment Size */
							alignment = 4 - mod;
							/* Set Alignment */
							pWorkEndAddr = pWorkEndAddr + alignment;
						}
						/* Set Value */
						*pWorkEndAddr = (UINT32)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 4;
						/* Set Value */
						*pWorkEndAddr = (INT32)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 4;
					}
					break;
				}
				case TRDP_TIMEDATE48:
				{
					/* Value Size: 4Byte + 2Byte */
					while (noOfItems-- > 0)
					{
						/* Get Start Address */
						dstEndAddress = (UINT32)pWorkEndAddr;
						/* Adjust alignment */
						mod = dstEndAddress % 4;
						if (mod != 0)
						{
							/* Set Alignment Size */
							alignment = 4 - mod;
							/* Set Alignment */
							pWorkEndAddr = pWorkEndAddr + alignment;
						}
						/* Set Value */
						*pWorkEndAddr = (UINT32)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 4;
						/* Set Value */
						*pWorkEndAddr = (UINT16)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 2;
					}
					break;
				}
				case TRDP_INT64:
				case TRDP_UINT64:
				case TRDP_REAL64:
				{
					/* Size: 8Byte */
					while (noOfItems-- > 0)
					{
						/* Set pDstEnd Address */
						dstEndAddress = (UINT32)pWorkEndAddr;
						/* Adjust alignment */
#if 0
						/* Adjust alignment */
						mod = dstEndAddress %
								8;
						if (mod != 0)
						{
							/* Set Alignment Size */
							alignment = 8 - mod;
							/* Set Alignment */
							pWorkEndAddr = pWorkEndAddr + alignment;
						}
#endif
						/* Adjust alignment */
						mod = dstEndAddress % 4;
						if (mod != 0)
						{
							/* Set Alignment Size */
							alignment = 4 - mod;
							/* Set Alignment */
							pWorkEndAddr = pWorkEndAddr + alignment;
						}
						/* Set Value */
						*pWorkEndAddr = (UINT32)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 8;
					}
					break;
				}
				default:
					break;
			}
		}
	}

	/* Get Dst End Address */
	workEndAddr = (UINT32)pWorkEndAddr;
	/* Set Dst End Address for return */
	memcpy(pDstEnd, &workEndAddr, sizeof(UINT32));

	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Application Thread Function
 */

/**********************************************************************************************************************/
/** Create Publisher Thread
 *
 *  @param[in]		pPublisherThreadParameter			pointer to publisher Thread Parameter
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createPublisherThread (
		PUBLISHER_THREAD_PARAMETER_T *pPublisherThreadParameter)
{
	/* Thread Handle */
	APPLICATION_THREAD_HANDLE_T	*pPublisherAppThreadHandle = NULL;
	VOS_ERR_T						vos_err = VOS_NO_ERR;
	TAUL_APP_ERR_TYPE				taulAppErr = TAUL_APP_NO_ERR;

	/* Get Publisher Application Handle memory area */
	pPublisherAppThreadHandle = (APPLICATION_THREAD_HANDLE_T *)vos_memAlloc(sizeof(APPLICATION_THREAD_HANDLE_T));
	if (pPublisherAppThreadHandle == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"createPublisherThread() Failed. Publisher Application Thread Handle vos_memAlloc() Err\n");
		return TAUL_APP_MEM_ERR;
	}
	else
	{
		/* Initialize Publisher Application Thread Handle */
		memset(pPublisherAppThreadHandle, 0, sizeof(APPLICATION_THREAD_HANDLE_T));
	}
	/* Set Application Handle */
	pPublisherAppThreadHandle->appHandle =  pPublisherThreadParameter->pPublishTelegram->appHandle;
	/* Set TAUL Application Thread ID */
	pPublisherAppThreadHandle->taulAppThreadId =  pPublisherThreadParameter->taulAppThreadId;
	/* Publisher Thread Number Count up */
	publisherThreadNoCount++;

	/*  Create Publisher Thread */
	vos_err = vos_threadCreate(
				&pPublisherAppThreadHandle->applicationThreadHandle,					/* Pointer to returned thread handle */
				publisherThreadName,						/* Pointer to name of the thread (optional) */
				VOS_THREAD_POLICY_OTHER,					/* Scheduling policy (FIFO, Round Robin or other) */
				0,											/* Scheduling priority (1...255 (highest), default 0) */
				0,											/* Interval for cyclic threads in us (optional) */
				threadStackSize,							/* Minimum stacksize, default 0: 16kB */
				(void *)PublisherApplication,			/* Pointer to the thread function */
				(void *)pPublisherThreadParameter);	/* Pointer to the thread function parameters */
	if (vos_err == VOS_NO_ERR)
	{
		taulAppErr = setApplicationThreadHandleList(pPublisherAppThreadHandle);
		if (taulAppErr != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "createPublisherThread() failed. Set Application Thread Handle Error: %d\n", taulAppErr);
			/* Free Publisher Application Thread Handle */
			vos_memFree(pPublisherAppThreadHandle);
			return taulAppErr;
		}
		else
		{
			return TAUL_APP_NO_ERR;
		}
	}
	else
	{
		/* Publisher Thread Number Count down */
		publisherThreadNoCount--;
		vos_printLog(VOS_LOG_ERROR, "Publisher Thread Create Err\n");
		/* Free Publisher Application Thread Handle */
		vos_memFree(pPublisherAppThreadHandle);
		return TAUL_APP_THREAD_ERR;
	}
}
#if 0
/**********************************************************************************************************************/
/** Display MD Replier Receive Count
 *
 *  @param[in]      pHeadCommandValue	pointer to head of queue
 *  @param[in]      commandValueId		COMMAND_VALUE ID
 *
 *  @retval         TAUL_APP_NO_ERR					no error
 *  @retval         MD_PARAM_ERR					parameter	error
 *
 */
TAU_APP_ERR_TYPE printReplierResult (
		COMMAND_VALUE	*pHeadCommandValue,
		UINT32 commandValueId)
{
	COMMAND_VALUE *iterCommandValue;
	UINT16 commnadValueNumber = 1;
	char strIp[16] = {0};

    if (pHeadCommandValue == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

    for (iterCommandValue = pHeadCommandValue;
    	  iterCommandValue != NULL;
    	  iterCommandValue = iterCommandValue->pNextCommandValue)
    {
		/* Check Valid Replier */
		if ((iterCommandValue->mdCallerReplierType == REPLIER) && (pHeadCommandValue->mdAddListenerComId != 0))
		{
			/* Check ALL commanValue or select commandValue */
			if( (commandValueId == 0)
			|| ((commandValueId != 0) && (commandValueId == iterCommandValue->commandValueId)))
			{
				/*  Dump CommandValue */
				printf("Replier No.%u\n", commnadValueNumber);
				printf("-c,	Transport Type (UDP:0, TCP:1): %u\n", iterCommandValue->mdTransportType);
				printf("-d,	Replier Reply Message Type (Mp:0, Mq:1): %u\n", iterCommandValue->mdMessageKind);
				miscIpToString(iterCommandValue->mdDestinationAddress, strIp);
				printf("-g,	Replier MD Receive Destination IP Address: %s\n", strIp);
	//			printf("-i,	Dump Type (DumpOn:1, DumpOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdDump);
				printf("-k,	Replier MD Request Receive Cycle Number: %u\n", iterCommandValue->mdCycleNumber);
	//			printf("-l,	Log Type (LogFileOn:1, LogFileOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdLog);
				printf("-n,	Topology TYpe (Ladder:1, not Lader:0): %u\n", iterCommandValue->mdLadderTopologyFlag);
				printf("-N,	Confirm TImeout: micro sec: %u\n", iterCommandValue->mdTimeoutConfirm);
				printf("-o,	Replier MD Reply Error Type(1-6): %u\n", iterCommandValue->mdReplyErr);
				printf("-p,	Marshalling Type (Marshall:1, not Marshall:0): %u\n", iterCommandValue->mdMarshallingFlag);
				printf("-q,	Replier Add Listener ComId: %u\n", iterCommandValue->mdReplierNumber);
	//			printf("-r,	Reply TImeout: %u micro sec\n", iterCommandValue->mdTimeoutReply);
//				printf("Replier Receive MD Count: %u\n", iterCommandValue->replierMdReceiveCounter);
				printf("Replier Receive MD Count: %u\n", iterCommandValue->replierMdRequestReceiveCounter + iterCommandValue->replierMdConfrimReceiveCounter);
				printf("Replier Receive MD Request(Mn,Mr) Count: %u\n", iterCommandValue->replierMdRequestReceiveCounter);
				printf("Replier Receive MD Confirm(Mc) Count: %u\n", iterCommandValue->replierMdConfrimReceiveCounter);
				printf("Replier Receive MD Success Count: %u\n", iterCommandValue->replierMdReceiveSuccessCounter);
				printf("Replier Receive MD Failure Count: %u\n", iterCommandValue->replierMdReceiveFailureCounter);
				printf("Replier Retry Count: %u\n", iterCommandValue->replierMdRetryCounter);
				printf("Replier Send MD Count: %u\n", iterCommandValue->replierMdSendCounter);
				printf("Replier Send MD Success Count: %u\n", iterCommandValue->replierMdSendSuccessCounter);
				printf("Replier Send MD Failure Count: %u\n", iterCommandValue->replierMdSendFailureCounter);
				commnadValueNumber++;
			}
		}
    }

    if (commnadValueNumber == 1 )
    {
    	printf("Valid Replier MD Command isn't Set up\n");
    }

    return TAUL_APP_NO_ERR;
}
#endif

/**********************************************************************************************************************/
/** Publisher Application main
 *
 *  @param[in]		pPublisherThreadParameter			pointer to PublisherThread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE PublisherApplication (PUBLISHER_THREAD_PARAMETER_T *pPublisherThreadParameter)
{
	UINT32			requestCounter = 0;						/* put counter */
	TRDP_ERR_T		err = TRDP_NO_ERR;
	APPLICATION_THREAD_HANDLE_T				*pOwnApplicationThreadHandle = NULL;
	/* Traffic Store */
	extern			UINT8 *pTrafficStoreAddr;						/* pointer to pointer to Traffic Store Address */
	UINT32			dstEnd = 0;
	UINT32			trafficStoreWriteStartAddress = 0;
	UINT32			workingWriteTrafficStoreStartAddress = 0;
	UINT8			modTrafficStore = 0;
	UINT8			modWorkingWriteTrafficStore = 0;
	UINT8			alignmentWorkingWriteTrafficStore = 0;
	UINT8			*pWorkingWirteTrafficStore = NULL;

	/* Display TimeStamp when Publisher test Start time */
	vos_printLog(VOS_LOG_DBG, "%s PD Publisher Start.\n", vos_getTimeStamp());

	/* Get Write start Address in Traffic Store */
	trafficStoreWriteStartAddress = (UINT32)(pTrafficStoreAddr + pPublisherThreadParameter->pPublishTelegram->pPdParameter->offset);
	/* Get alignment */
	modTrafficStore = trafficStoreWriteStartAddress % 16;
	/* Get write Traffic store working memory area for alignment */
	pWorkingWirteTrafficStore = (UINT8*)vos_memAlloc(pPublisherThreadParameter->pPublishTelegram->dataset.size + 16);
	if (pWorkingWirteTrafficStore == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"PublishApplication() Failed. Working Write Traffic Store vos_memAlloc() Err\n");
		return TAUL_APP_MEM_ERR;
	}
	else
	{
		/* Initialize Working Write Traffic Store */
		memset(pWorkingWirteTrafficStore, 0, pPublisherThreadParameter->pPublishTelegram->dataset.size + 16);
	}
	/* Get Working Write start Address in Traffic Store */
	workingWriteTrafficStoreStartAddress = (UINT32)pWorkingWirteTrafficStore;
	/* Get alignment */
	modWorkingWriteTrafficStore = workingWriteTrafficStoreStartAddress % 16;
	vos_printLog(VOS_LOG_DBG, "modTraffic: %u modWork: %u \n", modTrafficStore, modWorkingWriteTrafficStore);
	/* Check alignment */
	if (modTrafficStore >= modWorkingWriteTrafficStore)
	{
		/* Set alignment of Working Write Traffic Store */
		alignmentWorkingWriteTrafficStore = modTrafficStore - modWorkingWriteTrafficStore;
		vos_printLog(VOS_LOG_DBG, "alignment: %u \n", alignmentWorkingWriteTrafficStore);
	}
	else
	{
		/* Set alignment of Working Write Traffic Store */
		alignmentWorkingWriteTrafficStore = (16 - modWorkingWriteTrafficStore) + modTrafficStore;
		vos_printLog(VOS_LOG_DBG, "alignment: %u \n", alignmentWorkingWriteTrafficStore);
	}
	/* Free Dataset */
	vos_memFree(pPublisherThreadParameter->pPublishTelegram->dataset.pDatasetStartAddr);
	/* Set Dataset Start Address */
	pPublisherThreadParameter->pPublishTelegram->dataset.pDatasetStartAddr = pWorkingWirteTrafficStore + alignmentWorkingWriteTrafficStore;

	/*
		Enter the main processing loop.
	 */
	while (((requestCounter < pPublisherThreadParameter->pPdAppParameter->pdSendCycleNumber)
		|| (pPublisherThreadParameter->pPdAppParameter->pdSendCycleNumber == 0)))
	{
		/* Get Own Application Thread Handle */
		if (pOwnApplicationThreadHandle == NULL)
		{
			pOwnApplicationThreadHandle = searchApplicationThreadHandleList(
													pPublisherThreadParameter->taulAppThreadId);
			if (pOwnApplicationThreadHandle == NULL)
			{
				vos_printLog(VOS_LOG_DBG,"PublisherApplication() failed. Nothing Own Application Thread Handle.\n");
			}
		}
		/* Check Application Thread Terminate Indicate */
		if (pOwnApplicationThreadHandle != NULL)
		{
			if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
			{
				/* Break Publisher main Loop */
				vos_printLog(VOS_LOG_DBG,"PublisherApplication() Receive Application Thread Terminate Indicate. Break Publisher Main Loop.");
				break;
			}
		}

		/* Set Dataset start Address */
		dstEnd = (UINT32)pPublisherThreadParameter->pPublishTelegram->dataset.pDatasetStartAddr;

		/* Create Dataset */
		err = createDataset(
				requestCounter,
				pPublisherThreadParameter->pPublishTelegram->pDatasetDescriptor,
				&pPublisherThreadParameter->pPublishTelegram->dataset,
				(UINT8*)&dstEnd);
		if (err != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Publisher Application Create Dataset Failed. createDataset() Error: %d\n", err);
		}

		/* Get access right to Traffic Store*/
		err = tau_ldLockTrafficStore();
		if (err == TRDP_NO_ERR)
		{
			/* Set PD Data in Traffic Store */
			memcpy((void *)((int)pTrafficStoreAddr + pPublisherThreadParameter->pPublishTelegram->pPdParameter->offset),
					pPublisherThreadParameter->pPublishTelegram->dataset.pDatasetStartAddr,
					pPublisherThreadParameter->pPublishTelegram->dataset.size);
			/* put count up */
			requestCounter++;

			/* Release access right to Traffic Store*/
			err = tau_ldUnlockTrafficStore();
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Release Traffic Store accessibility Failed\n");
			}
		}
		else
		{
			vos_printLog(VOS_LOG_ERROR, "Get Traffic Store accessibility Failed\n");
		}
		/* Waits for a next creation cycle */
		vos_threadDelay(pPublisherThreadParameter->pPdAppParameter->pdAppCycleTime);
	}   /*	Bottom of while-loop	*/

	/*
	 *	We always clean up behind us!
	 */

	/* Display TimeStamp when caller test start time */
	vos_printLog(VOS_LOG_DBG, "%s Publisher test end.\n", vos_getTimeStamp());

	/* Set Application Thread Terminate Status */
	pOwnApplicationThreadHandle->taulAppThreadState = TAUL_APP_THREAD_TERMINATE;

	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Create Subscriber Thread
 *
 *  @param[in]		pSubscriberThreadParameter			pointer to subscriber Thread Parameter
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createSubscriberThread (
		SUBSCRIBER_THREAD_PARAMETER_T *pSubscriberThreadParameter)
{
	/* Thread Handle */
	APPLICATION_THREAD_HANDLE_T	*pSubscriberAppThreadHandle = NULL;
	VOS_ERR_T						vos_err = VOS_NO_ERR;
	TAUL_APP_ERR_TYPE				taulAppErr = TAUL_APP_NO_ERR;

	/* Get Subscriber Application Handle memory area */
	pSubscriberAppThreadHandle = (APPLICATION_THREAD_HANDLE_T *)vos_memAlloc(sizeof(APPLICATION_THREAD_HANDLE_T));
	if (pSubscriberAppThreadHandle == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"createSubscriberThread() Failed. Publisher Application Thread Handle vos_memAlloc() Err\n");
		return TAUL_APP_MEM_ERR;
	}
	else
	{
		/* Initialize Subscriber Application Thread Handle */
		memset(pSubscriberAppThreadHandle, 0, sizeof(APPLICATION_THREAD_HANDLE_T));
	}
	/* Set Application Handle */
	pSubscriberAppThreadHandle->appHandle =  pSubscriberThreadParameter->pSubscribeTelegram->appHandle;
	/* Set TAUL Application Thread ID */
	pSubscriberAppThreadHandle->taulAppThreadId =  pSubscriberThreadParameter->taulAppThreadId;

	/* Subscriber Thread Number Count up */
	subscriberThreadNoCount++;

	/*  Create Subscriber Thread */
	vos_err = vos_threadCreate(
				&pSubscriberAppThreadHandle->applicationThreadHandle,					/* Pointer to returned thread handle */
				subscriberThreadName,					/* Pointer to name of the thread (optional) */
				VOS_THREAD_POLICY_OTHER,					/* Scheduling policy (FIFO, Round Robin or other) */
				0,											/* Scheduling priority (1...255 (highest), default 0) */
				0,											/* Interval for cyclic threads in us (optional) */
				threadStackSize,							/* Minimum stacksize, default 0: 16kB */
				(void *)SubscriberApplication,			/* Pointer to the thread function */
				(void *)pSubscriberThreadParameter);	/* Pointer to the thread function parameters */
	if (vos_err == VOS_NO_ERR)
	{
		taulAppErr = setApplicationThreadHandleList(pSubscriberAppThreadHandle);
		if (taulAppErr != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "createSubscriberThread() failed. Set Application Thread Handle Error: %d\n", taulAppErr);
			/* Free Subscriber Application Thread Handle */
			vos_memFree(pSubscriberAppThreadHandle);
			return taulAppErr;
		}
		else
		{
			return TAUL_APP_NO_ERR;
		}
	}
	else
	{
		/* Subscriber Thread Number Count down */
		subscriberThreadNoCount--;
		vos_printLog(VOS_LOG_ERROR, "Subscriber Thread Create Err\n");
		/* Free Subscriber Application Thread Handle */
		vos_memFree(pSubscriberAppThreadHandle);
		return TAUL_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Subscriber Application main
 *
 *  @param[in]		pSubscriberThreadParameter			pointer to Subscriber Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE SubscriberApplication (SUBSCRIBER_THREAD_PARAMETER_T *pSubscriberThreadParameter)
{
	UINT32				subscribeCounter = 0;	/* Counter of Get Dataset in Traffic Store */
	TRDP_ERR_T			err = TRDP_NO_ERR;
	APPLICATION_THREAD_HANDLE_T				*pOwnApplicationThreadHandle = NULL;

	/* Display RD Return Test Start Time */
	vos_printLog(VOS_LOG_DBG, "%s PD Subscriber start.\n", vos_getTimeStamp());

	/*
        Enter the main processing loop.
     */
	while((subscribeCounter < pSubscriberThreadParameter->pPdAppParameter->pdReceiveCycleNumber)
		|| (pSubscriberThreadParameter->pPdAppParameter->pdReceiveCycleNumber == 0))
    {
		/* Get Own Application Thread Handle */
		if (pOwnApplicationThreadHandle == NULL)
		{
			pOwnApplicationThreadHandle = searchApplicationThreadHandleList(
													pSubscriberThreadParameter->taulAppThreadId);
			if (pOwnApplicationThreadHandle == NULL)
			{
				vos_printLog(VOS_LOG_DBG,"SubscriberApplication() failed. Nothing Own Application Thread Handle.\n");
			}
		}
		/* Check Application Thread Terminate Indicate */
		if (pOwnApplicationThreadHandle != NULL)
		{
			if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
			{
				/* Break Publisher main Loop */
				vos_printLog(VOS_LOG_DBG,"SubscriberApplication() Receive Application Thread Terminate Indicate. Break Subscriber Main Loop.");
				break;
			}
		}

		/* Get access right to Traffic Store*/
    	err = tau_ldLockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{
			/* Get Receive PD DataSet from Traffic Store */
    		memcpy(pSubscriberThreadParameter->pSubscribeTelegram->dataset.pDatasetStartAddr,
    				(void *)((int)pTrafficStoreAddr + pSubscriberThreadParameter->pSubscribeTelegram->pPdParameter->offset),
    				pSubscriberThreadParameter->pSubscribeTelegram->dataset.size);
			/* Release access right to Traffic Store*/
			err = tau_ldUnlockTrafficStore();
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Release Traffic Store accessibility Failed\n");
			}
    	}
    	else
    	{
    		vos_printLog(VOS_LOG_ERROR, "Get Traffic Store accessibility Failed\n");
    	}

		/* Waits for a next to Traffic Store put/get cycle */
		vos_threadDelay(pSubscriberThreadParameter->pPdAppParameter->pdAppCycleTime);

		/* PD Return Loop Counter Count Up */
		if (pSubscriberThreadParameter->pPdAppParameter->pdReceiveCycleNumber != 0)
		{
			subscribeCounter++;
		}
    }   /*	Bottom of while-loop	*/

    /*
     *	We always clean up behind us!
     */

	/* Display RD Return Result */
	vos_printLog(VOS_LOG_DBG, "%s PD Subscriber finish.\n", vos_getTimeStamp());

	/* Set Application Thread Terminate Status */
	pOwnApplicationThreadHandle->taulAppThreadState = TAUL_APP_THREAD_TERMINATE;

	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Create PD Requester Thread
 *
 *  @param[in]		pPdRequesterThreadParameter			pointer to PD Requester Thread Parameter
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createPdRequesterThread (
		PD_REQUESTER_THREAD_PARAMETER_T *pPdRequesterThreadParameter)
{
	/* Thread Handle */
	APPLICATION_THREAD_HANDLE_T	*pPdRequesterAppThreadHandle = NULL;	/* PD Requester Thread handle */
	VOS_ERR_T				vos_err = VOS_NO_ERR;
	TAUL_APP_ERR_TYPE		taulAppErr = TAUL_APP_NO_ERR;

	/* Get PD Requester Application Handle memory area */
	pPdRequesterAppThreadHandle = (APPLICATION_THREAD_HANDLE_T *)vos_memAlloc(sizeof(APPLICATION_THREAD_HANDLE_T));
	if (pPdRequesterAppThreadHandle == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"createPdRequesterThread() Failed. Publisher Application Thread Handle vos_memAlloc() Err\n");
		return TAUL_APP_MEM_ERR;
	}
	else
	{
		/* Initialize PD Requester Application Thread Handle */
		memset(pPdRequesterAppThreadHandle, 0, sizeof(APPLICATION_THREAD_HANDLE_T));
	}
	/* Set Application Handle */
	pPdRequesterAppThreadHandle->appHandle =  pPdRequesterThreadParameter->pPdRequestTelegram->appHandle;
	/* Set TAUL Application Thread ID */
	pPdRequesterAppThreadHandle->taulAppThreadId =  pPdRequesterThreadParameter->taulAppThreadId;

	/* PD Requester Thread Number Count up */
	pdRequesterThreadNoCount++;

	/*  Create Subscriber Thread */
	vos_err = vos_threadCreate(
				&pPdRequesterAppThreadHandle->applicationThreadHandle,					/* Pointer to returned thread handle */
				pdRequesterThreadName,					/* Pointer to name of the thread (optional) */
				VOS_THREAD_POLICY_OTHER,					/* Scheduling policy (FIFO, Round Robin or other) */
				0,											/* Scheduling priority (1...255 (highest), default 0) */
				0,											/* Interval for cyclic threads in us (optional) */
				threadStackSize,							/* Minimum stacksize, default 0: 16kB */
				(void *)PdRequesterApplication,			/* Pointer to the thread function */
				(void *)pPdRequesterThreadParameter);	/* Pointer to the thread function parameters */
	if (vos_err == VOS_NO_ERR)
	{
		taulAppErr = setApplicationThreadHandleList(pPdRequesterAppThreadHandle);
		if (taulAppErr != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "createPdRequesterThread() failed. Set Application Thread Handle Error: %d\n", taulAppErr);
			/* Free PD Requester Application Thread Handle */
			vos_memFree(pPdRequesterAppThreadHandle);
			return taulAppErr;
		}
		else
		{
			return TAUL_APP_NO_ERR;
		}
	}
	else
	{
		/* PD Requester Thread Number Count down */
		pdRequesterThreadNoCount--;
		vos_printLog(VOS_LOG_ERROR, "PD Requester Thread Create Err\n");
		/* Free PD Requester Application Thread Handle */
		vos_memFree(pPdRequesterAppThreadHandle);
		return TAUL_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** PD Requester Application main
 *
 *  @param[in]		pPdRequesterThreadParameter			pointer to PD Requester Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE PdRequesterApplication (PD_REQUESTER_THREAD_PARAMETER_T *pPdRequesterThreadParameter)
{
	UINT32			requestCounter = 0;						/* request counter */
	TRDP_ERR_T 	err = TRDP_NO_ERR;
	UINT32 dstEnd = 0;
	APPLICATION_THREAD_HANDLE_T				*pOwnApplicationThreadHandle = NULL;

	/* Display TimeStamp when Pull Requester test Start time */
	vos_printLog(VOS_LOG_DBG, "%s PD Pull Requester Start.\n", vos_getTimeStamp());

	/*
        Enter the main processing loop.
     */
    while (((requestCounter < pPdRequesterThreadParameter->pPdAppParameter->pdSendCycleNumber)
    	|| (pPdRequesterThreadParameter->pPdAppParameter->pdSendCycleNumber == 0)))
    {
    	/* Get Own Application Thread Handle */
		if (pOwnApplicationThreadHandle == NULL)
		{
			pOwnApplicationThreadHandle = searchApplicationThreadHandleList(
													pPdRequesterThreadParameter->taulAppThreadId);
			if (pOwnApplicationThreadHandle == NULL)
			{
				vos_printLog(VOS_LOG_DBG,"PdRequesterApplication() failed. Nothing Own Application Thread Handle.\n");
			}
		}
		/* Check Application Thread Terminate Indicate */
		if (pOwnApplicationThreadHandle != NULL)
		{
			if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
			{
				/* Break Publisher main Loop */
				vos_printLog(VOS_LOG_DBG,"SubscriberApplication() Receive Application Thread Terminate Indicate. Break PdRequester Main Loop.");
				break;
			}
		}

		/* Set Create Dataset Destination Address */
		dstEnd = (UINT32)pPdRequesterThreadParameter->pPdRequestTelegram->dataset.pDatasetStartAddr;

		/* Create Dataset */
		err = createDataset(
				requestCounter,
				pPdRequesterThreadParameter->pPdRequestTelegram->pDatasetDescriptor,
				&pPdRequesterThreadParameter->pPdRequestTelegram->dataset,
				(UINT8*)&dstEnd);
		if (err != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "PD Requester Application Create Dataset Failed. createDataset() Error: %d\n", err);
		}

		/* Get access right to Traffic Store*/
    	err = tau_lockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{
    		/* Set PD Data in Traffic Store */
			memcpy((void *)((int)pTrafficStoreAddr + pPdRequesterThreadParameter->pPdRequestTelegram->pPdParameter->offset),
					pPdRequesterThreadParameter->pPdRequestTelegram->dataset.pDatasetStartAddr,
					pPdRequesterThreadParameter->pPdRequestTelegram->dataset.size);
			/* request count up */
			requestCounter++;

          	/* Release access right to Traffic Store*/
    		err = tau_unlockTrafficStore();
    		if (err != TRDP_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "Release Traffic Store accessibility Failed\n");
    		}
    	}
    	else
    	{
    		vos_printLog(VOS_LOG_ERROR, "Get Traffic Store accessibility Failed\n");
    	}

    	/* Waits for a next creation cycle */
		vos_threadDelay(pPdRequesterThreadParameter->pPdAppParameter->pdAppCycleTime);

    }   /*	Bottom of while-loop	*/

    /*
     *	We always clean up behind us!
     */

	/* Display TimeStamp when Pull Requester test end time */
	vos_printLog(VOS_LOG_DBG, "%s PD Pull Requester end.\n", vos_getTimeStamp());

	/* Set Application Thread Terminate Status */
	pOwnApplicationThreadHandle->taulAppThreadState = TAUL_APP_THREAD_TERMINATE;

	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Create Caller Thread
 *
 *  @param[in]		pCallerThreadParameter			pointer to Caller Thread Parameter
 *
 *  @retval			TRDP_APP_NO_ERR						no error
 *  @retval			TRDP_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createCallerThread (
		CALLER_THREAD_PARAMETER_T *pCallerThreadParameter)
{
	/* Thread Handle */
	APPLICATION_THREAD_HANDLE_T	*pCallerAppThreadHandle = NULL;
	CHAR8							callerThreadCounterCharacter[THREAD_COUNTER_CHARACTER_SIZE] = {0};
	VOS_ERR_T						vos_err = VOS_NO_ERR;
	TAUL_APP_ERR_TYPE				taulAppErr = TAUL_APP_NO_ERR;

	/* Get Caller Application Handle memory area */
	pCallerAppThreadHandle = (APPLICATION_THREAD_HANDLE_T *)vos_memAlloc(sizeof(APPLICATION_THREAD_HANDLE_T));
	if (pCallerAppThreadHandle == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"createCallerThread() Failed. Caller Application Thread Handle vos_memAlloc() Err\n");
		return TAUL_APP_MEM_ERR;
	}
	else
	{
		/* Initialize Caller Application Thread Handle */
		memset(pCallerAppThreadHandle, 0, sizeof(APPLICATION_THREAD_HANDLE_T));
	}
	/* Set Application Handle */
	pCallerAppThreadHandle->appHandle =  pCallerThreadParameter->pCallerTelegram->appHandle;
	/* Set TAUL Application Thread ID */
	pCallerAppThreadHandle->taulAppThreadId =  pCallerThreadParameter->taulAppThreadId;

	/* Caller Thread Number Count up */
	callerThreadNoCount++;

	/* Set Message Queue Name */
	/* Set Base Name */
	strncpy(pCallerThreadParameter->mqName, callerMessageQueueName, sizeof(callerMessageQueueName));
	/* String Conversion */
	sprintf(callerThreadCounterCharacter, "%u", callerThreadNoCount);
	/* Connect Base Name and Thread Counter */
	strcat(pCallerThreadParameter->mqName, callerThreadCounterCharacter);

	/*  Create Publisher Thread */
	vos_err = vos_threadCreate(
				&pCallerAppThreadHandle->applicationThreadHandle,					/* Pointer to returned thread handle */
				callerThreadName,							/* Pointer to name of the thread (optional) */
				VOS_THREAD_POLICY_OTHER,					/* Scheduling policy (FIFO, Round Robin or other) */
				0,											/* Scheduling priority (1...255 (highest), default 0) */
				0,											/* Interval for cyclic threads in us (optional) */
				threadStackSize,							/* Minimum stacksize, default 0: 16kB */
				(void *)CallerApplication,				/* Pointer to the thread function */
				(void *)pCallerThreadParameter);		/* Pointer to the thread function parameters */
	if (vos_err == VOS_NO_ERR)
	{
		taulAppErr = setApplicationThreadHandleList(pCallerAppThreadHandle);
		if (taulAppErr != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "createCallerThread() failed. Set Application Thread Handle Error: %d\n", taulAppErr);
			/* Free Caller Application Thread Handle */
			vos_memFree(pCallerAppThreadHandle);
			return taulAppErr;
		}
		else
		{
			return TAUL_APP_NO_ERR;
		}
	}
	else
	{
		/* Caller Thread Number Count down */
		callerThreadNoCount--;
		vos_printLog(VOS_LOG_ERROR, "Caller Thread Create Err\n");
		/* Free Caller Application Thread Handle */
		vos_memFree(pCallerAppThreadHandle);
		return TAUL_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Caller Application main
 *
 *  @param[in]		pCallerThreadParameter			pointer to Caller Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE CallerApplication (CALLER_THREAD_PARAMETER_T *pCallerThreadParameter)
{
	TRDP_ERR_T									err = TRDP_NO_ERR;
	TAUL_APP_ERR_TYPE							taulAppErr = TAUL_APP_NO_ERR;
	VOS_ERR_T									vos_err = VOS_NO_ERR;
	VOS_QUEUE_T								queueHandle = NULL;
	CALLER_REPLIER_APP_THREAD_HANDLE_T		*pCallerAppThreadRequestHandle = NULL;			/* for Request */
	CALLER_REPLIER_APP_THREAD_HANDLE_T		*pCallerAppThreadReplyHandle = NULL;			/* for Reply */
	CALLER_REPLIER_APP_THREAD_HANDLE_T		*pCallerAppThreadConfirmHandle = NULL;			/* for Confrim  */
	CALLER_TELEGRAM_T							*pCallerReplyTelegram = NULL;
	CALLER_TELEGRAM_T							*pCallerConfirmTelegram = NULL;
	MD_APP_THREAD_TYPE						callerThreadType = MD_APP_UNKOWN_TYPE;
	UINT32										datasetDstEnd  = 0;
	UINT32										*callerRef = NULL;
	UINT32										referenceValue = 0;
	trdp_apl_cbenv_t							*pReceiveMqMsg;
	UINT32										receiveMessageSize = 0;
	UINT32										messageQueueReceviveWaitTIme = 0;				/* micro second */
	TRDP_IP_ADDR_T							networkByteSrcIpAddr = 0;						/* for convert URI to Source IP Address */
	UINT32										datasetFirstElementNumber = 0;
	APPLICATION_THREAD_HANDLE_T				*pOwnApplicationThreadHandle = NULL;
	UINT32										usingReceiveSubnetId = 0;
	/* Loop Counter */
	UINT32										i = 0;
	UINT32										sendMdTransferRequestCounter = 0;				/* Send MD Transfer Request Count */
	/* Timer */
	struct timeval							tv_interval = {0};								/* interval Time :timeval type */
	TRDP_TIME_T								trdp_time_tv_interval = {0};					/* interval Time :TRDP_TIME_T type for TRDP function */
	TRDP_TIME_T								nowTime = {0};
	TRDP_TIME_T								nextSendTime = {0};
	TRDP_TIME_T								nextReplyTimeoutTime = {0};
	TRDP_TIME_T								receiveWaitTime = {0};
	struct timespec							wanted_delay;
	struct timespec							remaining_delay;
	/* For Decide Request Transfer */
	SEND_REQUEST_HANDLE_T					*pMrsendHandle = NULL;
	SEND_REQUEST_HANDLE_T					*pMrSendHandleTable[REQUEST_HANDLE_TABLE_MAX] = {0};		/* MD Request(Mr) Handle Table */
	BOOL8										mrReferenceCheckResultFlag = FALSE;						/* for Check reply Reference */
	RECEIVE_REPLY_HANDLE_T					receiveReplyHandleTable[RECEIVE_REPLY_HANDLE_TABLE_MAX] = {{0}};
	/* Result Counter */
	UINT32										mdReceiveCounter = 0;						/* MD Receive Counter */
	UINT32										mdReceiveFailureCounter = 0;				/* MD Receive OK Counter */
	UINT32										mdReceiveSuccessCounter = 0;				/* MD Receive NG Counter */
	UINT32										mdRetryCounter = 0;							/* MD Retry Counter */
	/* Reference Value */
	UINT32										receiveMsgCallerRefAddress = 0;
	UINT32 									receiveMsgCallerRefValue = 0;

	/* Message Queue Open */
	vos_err = vos_queueCreate(VOS_QUEUE_POLICY_OTHER, TRDP_QUEUE_MAX_MESG, &queueHandle);
	if (vos_err != VOS_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Caller Message Queue Open error\n");
		return TAUL_QUEUE_ERR;
	}
	else
	{
		/* Get Caller Application Request Handle memory area */
		pCallerAppThreadRequestHandle = (CALLER_REPLIER_APP_THREAD_HANDLE_T *)vos_memAlloc(sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
		if (pCallerAppThreadRequestHandle == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"CallerApplication() Failed. Caller Application Thread Handle vos_memAlloc() Err\n");
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* Initialize Caller Application Request Handle */
			memset(pCallerAppThreadRequestHandle, 0, sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
		}
		/* Set Caller Message Queue Descriptor in Caller Application Thread Handle */
		pCallerAppThreadRequestHandle->pTrdpMqDescriptor =  queueHandle;
		/* Set Listener Handle in Caller Application Thread Handle for Receive Mr Timeout */
		pCallerAppThreadRequestHandle->pMdAppThreadListener =  pCallerThreadParameter->pCallerTelegram->listenerHandleForTAUL;
		/* Set Caller Message Queue Descriptor */
		err = setCallerAppThreadMessageQueueDescriptor(pCallerAppThreadRequestHandle);
		if (err != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Set Caller Message Queue Descriptor Failed. setCallerAppThreadMessageQueueDescriptor() error: %d\n", err);
			/* Free Caller Application Thread Request Handle */
			vos_memFree(pCallerAppThreadRequestHandle);
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
			return TAUL_APP_ERR;
		}
	}

	/* Set Caller Thread Type */
   	/*  Convert host URI to IP address  */
	if (pCallerThreadParameter->pCallerTelegram->pSource != NULL)
	{
		if (pCallerThreadParameter->pCallerTelegram->pSource->pUriHost1 != NULL)
		{
			networkByteSrcIpAddr = vos_dottedIP (*(pCallerThreadParameter->pCallerTelegram->pSource->pUriHost1));
		}
	}
	/* Get Reply(Mp or Mq) Telegram */
	pCallerReplyTelegram = searchCallerTelegramList (
								pHeadCallerTelegram,
								pCallerThreadParameter->pCallerTelegram->comId,
								networkByteSrcIpAddr,
								pCallerThreadParameter->pMdAppParameter->callerAppDestinationAddress,
								TRUE);
	if ((pCallerReplyTelegram == NULL)
		|| (pCallerReplyTelegram->messageType == TRDP_GOTTEN_CALLER_MN_MR))
	{
		/* Set Caller Thread Type: Mn */
		callerThreadType = CALLER_MN;
	}
	/* Check Reply Telegram : Mq */
	else if (pCallerReplyTelegram->messageType == TRDP_GOTTEN_CALLER_MQ)
	{
	   	/*  Convert host URI to IP address  */
		if (pCallerReplyTelegram->pSource != NULL)
		{
			if (pCallerReplyTelegram->pSource->pUriHost1 != NULL)
			{
				networkByteSrcIpAddr = vos_dottedIP (*(pCallerReplyTelegram->pSource->pUriHost1));
			}
			else
			{
				networkByteSrcIpAddr = IP_ADDRESS_NOTHING;
			}
		}
		else
		{
			networkByteSrcIpAddr = IP_ADDRESS_NOTHING;
		}
		/* Get Confirm Telegram */
		pCallerConfirmTelegram = searchCallerTelegramList (
										pHeadCallerTelegram,
										pCallerReplyTelegram->comId,
										networkByteSrcIpAddr,
										pCallerThreadParameter->pMdAppParameter->callerAppDestinationAddress,
										TRUE);
		if (pCallerConfirmTelegram == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"CallerApplication() Failed. Confirm Telegram get Err\n");
			/* Free Caller Application Thread Request Handle */
			vos_memFree(pCallerAppThreadRequestHandle);
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
			return TAUL_APP_ERR;
		}
		else
		{
			/* Set Caller Thread Type: Mr-Mq-Mc */
			callerThreadType = CALLER_MR_MQ_MC;
		}
	}
	else
	{
		/* Set Caller Thread Type: Mr-Mp */
		callerThreadType = CALLER_MR_MP;
	}

	/* Set Message Queue Descriptor for Reply Receive */
	if (pCallerReplyTelegram != NULL)
	{
		/* Get Caller Application Thread Handle memory area */
		pCallerAppThreadReplyHandle = (CALLER_REPLIER_APP_THREAD_HANDLE_T *)vos_memAlloc(sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
		if (pCallerAppThreadReplyHandle == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"CallerApplication() Failed. Caller Application Thread Handle vos_memAlloc() Err\n");
			/* Free Caller Application Thread Request Handle */
			vos_memFree(pCallerAppThreadRequestHandle);
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* Initialize Caller Application Thread Handle */
			memset(pCallerAppThreadReplyHandle, 0, sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
		}
		/* Set Caller Message Queue Descriptor in Caller Application Thread Handle */
		pCallerAppThreadReplyHandle->pTrdpMqDescriptor =  queueHandle;
		/* Set Listener Handle in Caller Application Thread Handle for Receive Reply (Mp or Mq) */
		pCallerAppThreadReplyHandle->pMdAppThreadListener =  pCallerReplyTelegram->listenerHandleForTAUL;
		/* Set Caller Message Queue Descriptor */
		err = setCallerAppThreadMessageQueueDescriptor(pCallerAppThreadReplyHandle);
		if (err != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Set Caller Message Queue Descriptor Failed. setCallerAppThreadMessageQueueDescriptor() error: %d\n", err);
			/* Free Caller Application Thread Request Handle */
			vos_memFree(pCallerAppThreadRequestHandle);
			/* Free Caller Application Thread Reply Handle */
			vos_memFree(pCallerAppThreadReplyHandle);
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
			return TAUL_APP_ERR;
		}
	}

	/* Set Message Queue Descriptor for Confirm Receive */
	if (pCallerConfirmTelegram != NULL)
	{
		/* Get Caller Application Thread Handle memory area */
		pCallerAppThreadConfirmHandle = (CALLER_REPLIER_APP_THREAD_HANDLE_T *)vos_memAlloc(sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
		if (pCallerAppThreadConfirmHandle == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"CallerApplication() Failed. Caller Application Thread Confirm Handle vos_memAlloc() Err\n");
			/* Free Caller Application Thread Request Handle */
			vos_memFree(pCallerAppThreadRequestHandle);
			/* Free Caller Application Thread Reply Handle */
			vos_memFree(pCallerAppThreadReplyHandle);
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* Initialize Caller Application Thread Confirm Handle */
			memset(pCallerAppThreadConfirmHandle, 0, sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
		}
		/* Set Caller Message Queue Descriptor in Caller Application Thread Confirm Handle */
		pCallerAppThreadConfirmHandle->pTrdpMqDescriptor =  queueHandle;
		/* Set Listener Handle in Caller Application Thread Handle for Receive Confirm */
		pCallerAppThreadConfirmHandle->pMdAppThreadListener =  pCallerConfirmTelegram->listenerHandleForTAUL;
		/* Set Listener Handle destination IP Address : 0.0.0.0 */
		pCallerAppThreadConfirmHandle->pMdAppThreadListener->destIpAddr = IP_ADDRESS_NOTHING;
		/* Set Caller Message Queue Descriptor */
		err = setCallerAppThreadMessageQueueDescriptor(pCallerAppThreadConfirmHandle);
		if (err != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Set Caller Confirm Message Queue Descriptor Failed. setCallerAppThreadMessageQueueDescriptor() error: %d\n", err);
			/* Free Caller Application Thread Request Handle */
			vos_memFree(pCallerAppThreadRequestHandle);
			/* Free Caller Application Thread Reply Handle */
			vos_memFree(pCallerAppThreadReplyHandle);
			/* Free Caller Application Thread Confirm Handle */
			vos_memFree(pCallerAppThreadConfirmHandle);
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
			return TAUL_APP_ERR;
		}
	}

	/* Get Own Application Thread Handle */
	pOwnApplicationThreadHandle = searchApplicationThreadHandleList(
									pCallerThreadParameter->taulAppThreadId);
	if (pOwnApplicationThreadHandle == NULL)
	{
		vos_printLog(VOS_LOG_DBG,"CallerApplication() failed. Nothing Own Application Thread Handle.\n");
	}

	/* Caller Main Process */
	/* Display TimeStamp when caller test start time */
	vos_printLog(VOS_LOG_INFO, "%s Caller test start.\n", vos_getTimeStamp());

	/* MD Request Send loop */
	while(sendMdTransferRequestCounter <= pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber)
	{
		/* Check this Thread is MD Request Send Subnet ? */
		/* Get Using Sub-Network */
	    err = tau_ldGetNetworkContext(&usingReceiveSubnetId);
	    if (err != TRDP_NO_ERR)
	    {
	    	vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Get Writing Traffic Store Sub-network error\n");
			/* Free Caller Application Thread Request Handle */
			vos_memFree(pCallerAppThreadRequestHandle);
			/* Free Caller Application Thread Reply Handle */
			if (pCallerAppThreadReplyHandle != NULL)
			{
				vos_memFree(pCallerAppThreadReplyHandle);
			}
			/* Free Caller Application Thread Confirm Handle */
			if (pCallerAppThreadConfirmHandle != NULL)
			{
				vos_memFree(pCallerAppThreadConfirmHandle);
			}
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
	    	return TAUL_APP_ERR;
	    }
	    else
	    {
	    	/* Check Own IP Address is Write Traffic Store Subnet ? */
	    	if ((pCallerThreadParameter ->pCallerTelegram ->appHandle ->realIP & SUBNET_NO2_NETMASK) != usingReceiveSubnetId)
	    	{
	    		/* Wait Caller Application Cycle Time*/
	    		vos_threadDelay(pCallerThreadParameter->pMdAppParameter->mdAppCycleTime);
				/* Check Application Thread Terminate Indicate */
				if (pOwnApplicationThreadHandle != NULL)
				{
					if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
					{
						/* Break MD Send Loop */
						vos_printLog(VOS_LOG_DBG,"CallerApplication() Receive Application Thread Terminate Indicate. Break Receive Reply Message Loop.");
						break;
					}
				}
	    		continue;
	    	}
	    }

		/* Check Caller send finish ? */
		if ((sendMdTransferRequestCounter != 0)
		&& (sendMdTransferRequestCounter >= pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber))
		{
			/* Not Send */
		}
		else
		{
			/* Set Create Dataset Destination Address */
			datasetDstEnd = (UINT32)pCallerThreadParameter->pCallerTelegram->dataset.pDatasetStartAddr;
			/* Create Dataset */
			taulAppErr = createDataset (
								datasetFirstElementNumber,
								pCallerThreadParameter->pCallerTelegram->pDatasetDescriptor,
								&pCallerThreadParameter->pCallerTelegram->dataset,
								(UINT8*)&datasetDstEnd);
			if (taulAppErr != TAUL_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Caller Application Create Dataset Failed. createDataset() Error: %d\n", taulAppErr);
			}
			/* Increment datasetFrirstElementNumber */
			datasetFirstElementNumber++;

			/* Send MD Transmission Request */
			switch(callerThreadType)
			{
				case CALLER_MN:
					/* MD Send Request Count */
					pCallerThreadParameter->pMdAppParameter->callerMdRequestSendCounter++;
					/* Notification */
					err = tau_ldNotify (
							pCallerThreadParameter->pCallerTelegram->comId,
							pCallerThreadParameter->pMdAppParameter->callerAppDestinationAddress,
							pCallerThreadParameter->pMdAppParameter->callerAppDestinationURI,
							pCallerThreadParameter->pCallerTelegram->dataset.pDatasetStartAddr,
							pCallerThreadParameter->pCallerTelegram->dataset.size);
					if (err != TRDP_NO_ERR)
					{
						/* MD Send Failure Count */
						pCallerThreadParameter->pMdAppParameter->callerMdSendFailureCounter++;
						/* Error : Send Notification */
						vos_printLog(VOS_LOG_ERROR, "Send Notification ERROR\n");
					}
					else
					{
						/* MD Send Success Count */
						pCallerThreadParameter->pMdAppParameter->callerMdSendSuccessCounter++;
					}
				break;
				case CALLER_MR_MP:
				case CALLER_MR_MQ_MC:
					/* MD Send Request Count */
					pCallerThreadParameter->pMdAppParameter->callerMdRequestSendCounter++;
					/* Create callerRef value */
					referenceValue = sequenceCounter | SEND_REFERENCE;
					/* Get Caller Application Thread Handle memory area */
					callerRef = (UINT32 *)vos_memAlloc(sizeof(UINT32));
					if (callerRef == NULL)
					{
						vos_printLog(VOS_LOG_ERROR,"CallerApplication() Failed. Caller Reference vos_memAlloc() Err\n");
						/* Free Caller Application Thread Request Handle */
						vos_memFree(pCallerAppThreadRequestHandle);
						/* Free Caller Application Thread Reply Handle */
						if (pCallerAppThreadReplyHandle != NULL)
						{
							vos_memFree(pCallerAppThreadReplyHandle);
						}
						/* Free Caller Application Thread Confirm Handle */
						if (pCallerAppThreadConfirmHandle != NULL)
						{
							vos_memFree(pCallerAppThreadConfirmHandle);
						}
						/* Destroy Message Queue */
						vos_err = vos_queueDestroy(queueHandle);
						if (vos_err != VOS_NO_ERR)
						{
							vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
							err = TAUL_APP_MQ_ERR;
						}
						return TAUL_APP_MEM_ERR;
					}
					else
					{
						/* Set callerRef */
						memcpy(callerRef, &referenceValue, sizeof(UINT32));
						/* Sequence Counter count up */
						sequenceCounter++;
					}
					/* Request */
					err = tau_ldRequest (
							pCallerThreadParameter->pCallerTelegram->comId,
							pCallerThreadParameter->pMdAppParameter->callerAppDestinationAddress,
							pCallerThreadParameter->pMdAppParameter->callerAppDestinationURI,
							pCallerThreadParameter->pMdAppParameter->callerAppNumberOfReplier,
							pCallerThreadParameter->pCallerTelegram->pMdParameter->replyTimeout,
							pCallerThreadParameter->pCallerTelegram->dataset.pDatasetStartAddr,
							pCallerThreadParameter->pCallerTelegram->dataset.size,
							callerRef);
					if (err != TRDP_NO_ERR)
					{
						/* MD Send Failure Count */
						pCallerThreadParameter->pMdAppParameter->callerMdSendFailureCounter++;
						/* Error : Send Request */
						vos_printLog(VOS_LOG_ERROR, "CallerApplication() Send Request ERROR\n");
					}
					else
					{
						/* MD Send Success Count */
						pCallerThreadParameter->pMdAppParameter->callerMdSendSuccessCounter++;
					}

					/* Get Request Thread Reply Receive Session Handle Area */
					pMrsendHandle = (SEND_REQUEST_HANDLE_T *)vos_memAlloc(sizeof(SEND_REQUEST_HANDLE_T));
					if (pMrsendHandle == NULL)
					{
						vos_printLog(VOS_LOG_ERROR, "CallerApplication() Failed. Create Send Request Handle Area ERROR. vos_memAlloc Err\n");
						/* Free Caller Application Thread Request Handle */
						vos_memFree(pCallerAppThreadRequestHandle);
						/* Free Caller Application Thread Reply Handle */
						if (pCallerAppThreadReplyHandle != NULL)
						{
							vos_memFree(pCallerAppThreadReplyHandle);
						}
						/* Free Caller Application Thread Confirm Handle */
						if (pCallerAppThreadConfirmHandle != NULL)
						{
							vos_memFree(pCallerAppThreadConfirmHandle);
						}
						/* Free Caller Application Thread Handle */
						vos_memFree(callerRef);
						/* Destroy Message Queue */
						vos_err = vos_queueDestroy(queueHandle);
						if (vos_err != VOS_NO_ERR)
						{
							vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
							err = TAUL_APP_MQ_ERR;
						}
						return TAUL_APP_MEM_ERR;
					}
					else
					{
						/* Initialize Mr send Handle */
						memset(pMrsendHandle, 0, sizeof(SEND_REQUEST_HANDLE_T));
						/* Set Reply Receive Session Handle */
						/* Get CalelrRef Address */
						UINT32 callerRefValue = 0;
						memcpy(&callerRefValue, callerRef, sizeof(UINT32));
						pMrsendHandle->callerRefValue = callerRefValue;
						pMrsendHandle->sendRequestNumExpReplies = pCallerThreadParameter->pMdAppParameter->callerAppNumberOfReplier;
						pMrsendHandle->decidedSessionSuccessCount = 0;
						pMrsendHandle->decidedSessionFailureCount = 0;
						/* Set mrSendSessionTable */
						for(i=0; i < REQUEST_HANDLE_TABLE_MAX; i++)
						{
							if(pMrSendHandleTable[i] == NULL)
							{
								/* Set Request Session Handle */
								pMrSendHandleTable[i] = pMrsendHandle;
								break;
							}
						}
					}
				break;
				default:
					/* Other than Caller and Replier */
					vos_printLog(VOS_LOG_ERROR, "Caller Replier Type ERROR. mdCallerReplierType = %d\n", callerThreadType);
				break;
			}

			/* MD Transmission Request Send Cycle is unlimited */
			if (pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber > 0)
			{
				/* MD Transmission Request SendCount up */
				sendMdTransferRequestCounter++;
			}
		}

		/* Get Next MD Transmission Send timing */
		/* Caller Send Interval Type:Request-Request ? */
		if (pCallerThreadParameter->pMdAppParameter->callerAppSendIntervalType == REQUEST_REQUEST)
		{
			vos_getTime(&nextSendTime);
			nextReplyTimeoutTime = nextSendTime;
			tv_interval.tv_sec      = pCallerThreadParameter->pMdAppParameter->mdAppCycleTime / 1000000;
			tv_interval.tv_usec     = pCallerThreadParameter->pMdAppParameter->mdAppCycleTime % 1000000;
			trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
			trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
			vos_addTime(&nextSendTime, &trdp_time_tv_interval);

			/* Receive Wait */
			/* Not Last Time send */
			/* or send Cycle Number:0 */
			if ((sendMdTransferRequestCounter < pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber)
				|| (pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber == 0))
			{
				/* Set Wait Time: Wait Next MD Transmission Send Timing for receive all Reply */
				receiveWaitTime = nextSendTime;
			}
			else
			{
				/* Last Time Send */
				/* Get Reply time out of day */
				tv_interval.tv_sec      = pCallerThreadParameter->pMdAppParameter->mdAppCycleTime / 1000000;
				tv_interval.tv_usec     = pCallerThreadParameter->pMdAppParameter->mdAppCycleTime % 1000000;
				trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
				trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
				vos_addTime(&nextReplyTimeoutTime, &trdp_time_tv_interval);
				/* Set Wait Time: Reply Time Out for Last Time Request receive all Reply*/
				receiveWaitTime = nextReplyTimeoutTime;
			}
		}
		/* Caller Send Interval Type:Reply-Request */
		else
		{
			/* Get Reply time out of day */
			tv_interval.tv_sec      = pCallerThreadParameter->pMdAppParameter->mdAppCycleTime / 1000000;
			tv_interval.tv_usec     = pCallerThreadParameter->pMdAppParameter->mdAppCycleTime % 1000000;
			trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
			trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
			vos_addTime(&nextReplyTimeoutTime, &trdp_time_tv_interval);
			/* Set Wait Time: Reply Time Out for Last Time Request receive all Reply*/
			receiveWaitTime = nextReplyTimeoutTime;
		}

		/* Receive Message Queue Loop */
		while(1)
		{
			/* Check Application Thread Terminate Indicate */
			if (pOwnApplicationThreadHandle != NULL)
			{
				if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
				{
					/* Break MD Reply Receive Loop */
					vos_printLog(VOS_LOG_DBG,"CallerApplication() Receive Application Thread Terminate Indicate. Break Receive Reply Message Loop.");
					break;
				}
			}

			/* Check Receive Finish */
			if ((pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber != 0)
			&& ((pCallerThreadParameter->pMdAppParameter->callerMdRequestReplySuccessCounter) + (pCallerThreadParameter->pMdAppParameter->callerMdRequestReplyFailureCounter)	 >= pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber))
			{
				/* Break MD Reply Receive loop */
				break;
			}

			/* Caller Send Interval Type:Reply-Request ? */
			if (pCallerThreadParameter->pMdAppParameter->callerAppSendIntervalType == REPLY_REQUEST)
			{
				/* Check 1Mr-Mp sequence Finish */
				if (pCallerThreadParameter->pMdAppParameter->callerMdRequestSendCounter == pCallerThreadParameter->pMdAppParameter->callerMdRequestReplySuccessCounter + pCallerThreadParameter->pMdAppParameter->callerMdRequestReplyFailureCounter)
				{
					/* Break MD Reply Receive loop */
					break;
				}
			}
			/* Caller Send Interval Type:Request-Request ? */
			if (pCallerThreadParameter->pMdAppParameter->callerAppSendIntervalType == REQUEST_REQUEST)
			{
				vos_getTime(&nowTime);
				/* Check receive Wait Time */
				if (vos_cmpTime((TRDP_TIME_T *) &receiveWaitTime, (TRDP_TIME_T *) &nowTime) < 0)
				{
					/* Break Receive Message Queue because Receive Wait Timer */
					break;
				}
			}

			/* Receive All Message in Message Queue Loop */
			while(1)
			{
				/* Check Application Thread Terminate Indicate */
				if (pOwnApplicationThreadHandle != NULL)
				{
					if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
					{
						/* Break Receive Message Queue Loop */
						vos_printLog(VOS_LOG_DBG,"CallerApplication() Receive Application Thread Terminate Indicate. Break Receive Message Queue Loop.");
						break;
					}
				}

				/* Receive Message Queue Message */
				vos_err = vos_queueReceive(
									queueHandle,
									(UINT8 **)&pReceiveMqMsg,
									&receiveMessageSize,
									messageQueueReceviveWaitTIme);
				if (vos_err != VOS_NO_ERR)
				{
//					vos_printLog(VOS_LOG_DBG,"CallerApplication() vos_queueReceive err: %d\n", vos_err);
					/* Break Receive Message Queue because not Receive Message Queue */
					break;
				}
				else if(receiveMessageSize <= 0)
				{
//					vos_printLog(VOS_LOG_DBG,"CallerApplication() vos_queueReceive NOT Receive Message\n");
					/* Not Receive Message */
					break;
				}
				else
				{
					/* nothing */
				}

				/* Check ComId */
				/* Request comID and Reply comId and Confirm(Timeout) comId*/
				if (((pCallerReplyTelegram != NULL) && (pReceiveMqMsg->Msg.comId != pCallerReplyTelegram->comId))
					&& ((pCallerConfirmTelegram != NULL) && (pReceiveMqMsg->Msg.comId != pCallerConfirmTelegram->comId))
					&& ((pCallerThreadParameter->pCallerTelegram !=NULL) && (pReceiveMqMsg->Msg.comId != pCallerThreadParameter->pCallerTelegram->comId)))
				{
					/* ComId Err*/
					vos_printLog(VOS_LOG_ERROR, "Receive ComId ERROR\n");
				}
				else
				{
					/* Check Result Code */
					if (pReceiveMqMsg->Msg.resultCode == TRDP_NO_ERR)
					{
						/* Get SessionRef Value from Receive Message Reference */
						memcpy(&receiveMsgCallerRefAddress, pReceiveMqMsg->pRefCon, sizeof(UINT32));
						memcpy(&receiveMsgCallerRefValue, (void *)receiveMsgCallerRefAddress, sizeof(UINT32));

						/* Check msgType */
						switch(pReceiveMqMsg->Msg.msgType)
						{
							case TRDP_MSG_MQ:
							case TRDP_MSG_MP:
								for(i=0; i < REQUEST_HANDLE_TABLE_MAX; i++)
								{
									/* Check Send callerRef same Receive sessionRef */
									if(pMrSendHandleTable[i] == NULL)
									{
										continue;
									}
									else
									{
										if (pMrSendHandleTable[i]->callerRefValue == receiveMsgCallerRefValue)
										{
											/* Set Reply callerRef : TRUE */
											mrReferenceCheckResultFlag = TRUE;
											break;
										}
									}
								}
								/* Reply callerRef OK ? */
								if (mrReferenceCheckResultFlag == TRUE)
								{
									/* nothing */
								}
								else
								{
									vos_printLog(VOS_LOG_ERROR, "CallerApplication() Receive sessionRef ERROR\n");
								}
								/* Decide MD Transmission Result */
								taulAppErr = decideMdTransmissionResult(
													pReceiveMqMsg->pData,
													&(pReceiveMqMsg->dataSize));
								if (taulAppErr == TAUL_APP_NO_ERR)
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

								/* Set Receive Reply Result Table */
								setReceiveReplyHandleTable(
										receiveReplyHandleTable,
										receiveMsgCallerRefValue,
										pReceiveMqMsg->Msg.numReplies,
										pReceiveMqMsg->Msg.numRepliesQuery,
										taulAppErr);
								/* Set Caller Receive Count */
								pCallerThreadParameter->pMdAppParameter->callerMdReceiveCounter = mdReceiveCounter;
								pCallerThreadParameter->pMdAppParameter->callerMdReceiveSuccessCounter =  mdReceiveSuccessCounter;
								pCallerThreadParameter->pMdAppParameter->callerMdReceiveFailureCounter = mdReceiveFailureCounter;
								pCallerThreadParameter->pMdAppParameter->callerMdRetryCounter = mdRetryCounter;
							break;
							case TRDP_MSG_ME:
									/* Error : Receive Me */
								vos_printLog(VOS_LOG_ERROR, "CallerApplication() Receive Message Type ERROR. Receive Me\n");
							break;
							default:
								/* Other than Mq and Me */
								vos_printLog(VOS_LOG_ERROR, "CallerApplication() Receive Message Type ERROR\n");
							break;
						}
					}
					/* Reply Timeout ? and repliers unknown (One Mr-Mp Cycle End) */
					/* And Confirm TImeout (One Mr-Mq-Mc Cycle End) */
					else if((pReceiveMqMsg->Msg.resultCode == TRDP_REPLYTO_ERR) && (pReceiveMqMsg->Msg.numExpReplies == REPLIERS_UNKNOWN))
					{
						/* Set Receive Reply Result Table */
						setReceiveReplyHandleTable(
								receiveReplyHandleTable,
								receiveMsgCallerRefValue,
								pReceiveMqMsg->Msg.numReplies,
								pReceiveMqMsg->Msg.numRepliesQuery,
								TAUL_APP_MRMP_ONE_CYCLE_ERR);
						/* Receive Mp and No Repliers */
						/* or Receive Mr (Mr Timeout) */
						if (((pReceiveMqMsg->Msg.msgType == TRDP_MSG_MP) && (pReceiveMqMsg->Msg.numReplies <= 0))
							|| ((pReceiveMqMsg->Msg.msgType == TRDP_MSG_MR) && (pReceiveMqMsg->Msg.aboutToDie > 0)))
						{
							/* MD Receive NG Count */
							mdReceiveFailureCounter++;
							/* MD Receive Count */
							mdReceiveCounter++;
							/* Result Code Err */
							vos_printLog(VOS_LOG_ERROR, "CallerApplication() Receive Message Result Code ERROR. result code:%d\n", pReceiveMqMsg->Msg.resultCode);
						}
					}
					else
					{
						/* Result Code Err */
						vos_printLog(VOS_LOG_ERROR, "CallerApplication() Receive Message Result Code ERROR. result code:%d\n", pReceiveMqMsg->Msg.resultCode);

						/* Set Receive Reply Result Table */
						setReceiveReplyHandleTable(
								receiveReplyHandleTable,
								receiveMsgCallerRefValue,
								pReceiveMqMsg->Msg.numReplies,
								pReceiveMqMsg->Msg.numRepliesQuery,
								TAUL_APP_ERR);
						/* MD Receive NG Count */
						mdReceiveFailureCounter++;
						/* MD Receive Count */
						mdReceiveCounter++;
						/* Set Caller Receive Count */
						pCallerThreadParameter->pMdAppParameter->callerMdReceiveCounter = mdReceiveCounter;
						pCallerThreadParameter->pMdAppParameter->callerMdReceiveSuccessCounter =  mdReceiveSuccessCounter;
						pCallerThreadParameter->pMdAppParameter->callerMdReceiveFailureCounter = mdReceiveFailureCounter;
						pCallerThreadParameter->pMdAppParameter->callerMdRetryCounter = mdRetryCounter;
					}
					/* decide Request-Reply Result */
					err = decideRequestReplyResult(
							pMrSendHandleTable,
							receiveReplyHandleTable,
							pCallerThreadParameter->pMdAppParameter);
					/* Release Receive Message Data */
					if (pReceiveMqMsg != NULL)
					{
						if (pReceiveMqMsg->pData != NULL)
						{
							vos_memFree(&pReceiveMqMsg->pData);
							pReceiveMqMsg->pData = NULL;
						}
						vos_memFree(pReceiveMqMsg);
						pReceiveMqMsg = NULL;
					}
				}
			}
		}
		/* Check Application Thread Terminate Indicate */
		if (pOwnApplicationThreadHandle != NULL)
		{
			if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
			{
				/* Break Send Request Message Loop */
				vos_printLog(VOS_LOG_DBG,"CallerApplication() Receive Application Thread Terminate Indicate. Break Send Request Loop.");
				break;
			}
		}

		/* Check Caller send finish ? */
		if ((sendMdTransferRequestCounter != 0)
		&& (sendMdTransferRequestCounter >= pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber))
		{
			/* Check Receive Finish */
			/* Caller Thread Type : Notify (Mn) */
			if (callerThreadType == CALLER_MN)
			{
				/* Send Receive Loop Break */
				break;
			}
			/* Caller Send Type : Request (Mr) */
			else
			{
				if ((pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber != 0)
				&& ((pCallerThreadParameter->pMdAppParameter->callerMdReceiveCounter) >= pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber)
				&& ((pCallerThreadParameter->pMdAppParameter->callerMdRequestReplySuccessCounter) + (pCallerThreadParameter->pMdAppParameter->callerMdRequestReplyFailureCounter)	 >= pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber))
				{
					/* Send Receive Loop Break */
					break;
				}
			}
		}

		/* Get Next MD Transmission Send timing */
		/* Caller Send Interval Type:Reply-Request ? */
		if (pCallerThreadParameter->pMdAppParameter->callerAppSendIntervalType == REPLY_REQUEST)
		{
			vos_getTime(&nextSendTime);
			nextReplyTimeoutTime = nextSendTime;
			tv_interval.tv_sec      = pCallerThreadParameter->pMdAppParameter->mdAppCycleTime / 1000000;
			tv_interval.tv_usec     = pCallerThreadParameter->pMdAppParameter->mdAppCycleTime % 1000000;
			trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
			trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
			vos_addTime(&nextSendTime, &trdp_time_tv_interval);
		}

		/* Wait Next MD Transmission Send Timing */
		vos_getTime(&nowTime);
		if (vos_cmpTime((TRDP_TIME_T *) &nowTime, (TRDP_TIME_T *) &nextSendTime) < 0)
		{
			wanted_delay.tv_sec = nextSendTime.tv_sec - nowTime.tv_sec;
			wanted_delay.tv_nsec = (((nextSendTime.tv_usec - nextSendTime.tv_usec) % 1000000) * 1000);
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

	/* Display TimeStamp when caller test finish time */
	vos_printLog(VOS_LOG_DBG, "%s Caller test finish.\n", vos_getTimeStamp());
	/* Delete Listener */
	/* wait tlc_process 1 cycle time = 10000 for Last Reply trdp_mdSend() */
	vos_threadDelay(TLC_PROCESS_CYCLE_TIME);

	/* Delete AppThereadSession Message Queue Descriptor */
	taulAppErr = deleteCallerAppThreadMessageQueueDescriptor(pCallerAppThreadRequestHandle);
	if (taulAppErr != TAUL_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Caller AppThread Request Message Queue Descriptor delete Err\n");
	}
	/* Delete AppThereadSession Message Queue Descriptor Reply*/
	taulAppErr = deleteCallerAppThreadMessageQueueDescriptor(pCallerAppThreadReplyHandle);
	if (taulAppErr != TAUL_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Caller AppThread Reply Message Queue Descriptor delete Err\n");
	}
	/* Delete AppThereadSession Message Queue Descriptor Confirm */
	taulAppErr = deleteCallerAppThreadMessageQueueDescriptor(pCallerAppThreadConfirmHandle);
	if (taulAppErr != TAUL_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Caller AppThread Confirm Message Queue Descriptor delete Err\n");
	}
	/* Destroy Message Queue */
	vos_err = vos_queueDestroy(pCallerAppThreadRequestHandle->pTrdpMqDescriptor);
	if (vos_err != VOS_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "CallerApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
		err = TAUL_APP_MQ_ERR;
	}

	/* Set Application Thread Terminate Status */
	pOwnApplicationThreadHandle->taulAppThreadState = TAUL_APP_THREAD_TERMINATE;

	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Create Replier Thread
 *
 *  @param[in]		pReplierThreadParameter			pointer to Replier Thread Parameter
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createReplierThread (
		REPLIER_THREAD_PARAMETER_T *pReplierThreadParameter)
{
	/* Thread Handle */
	APPLICATION_THREAD_HANDLE_T	*pReplierAppThreadHandle = NULL;
	CHAR8					replierThreadCounterCharacter[THREAD_COUNTER_CHARACTER_SIZE] = {0};
	VOS_ERR_T				vos_err = VOS_NO_ERR;
	TAUL_APP_ERR_TYPE		taulAppErr = TAUL_APP_NO_ERR;

	/* Get Replier Application Handle memory area */
	pReplierAppThreadHandle = (APPLICATION_THREAD_HANDLE_T *)vos_memAlloc(sizeof(APPLICATION_THREAD_HANDLE_T));
	if (pReplierAppThreadHandle == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"createReplierThread() Failed. Replier Application Thread Handle vos_memAlloc() Err\n");
		return TAUL_APP_MEM_ERR;
	}
	else
	{
		/* Initialize Replier Application Thread Handle */
		memset(pReplierAppThreadHandle, 0, sizeof(APPLICATION_THREAD_HANDLE_T));
	}
	/* Set Application Handle */
	pReplierAppThreadHandle->appHandle =  pReplierThreadParameter->pReplierTelegram->appHandle;
	/* Set TAUL Application Thread ID */
	pReplierAppThreadHandle->taulAppThreadId =  pReplierThreadParameter->taulAppThreadId;

	/* Replier Thread Number Count up */
	replierThreadNoCount++;

	/* Set Message Queue Name */
	/* Set Base Name */
	strncpy(pReplierThreadParameter->mqName, replierMessageQueueName, sizeof(replierMessageQueueName));
	/* String Conversion */
	sprintf(replierThreadCounterCharacter, "%u", replierThreadNoCount);
	/* Connect Base Name and Thread Counter */
	strcat(pReplierThreadParameter->mqName, replierThreadCounterCharacter);

	/*  Create Publisher Thread */
	vos_err = vos_threadCreate(
				&pReplierAppThreadHandle->applicationThreadHandle,					/* Pointer to returned thread handle */
				replierThreadName,							/* Pointer to name of the thread (optional) */
				VOS_THREAD_POLICY_OTHER,					/* Scheduling policy (FIFO, Round Robin or other) */
				0,											/* Scheduling priority (1...255 (highest), default 0) */
				0,											/* Interval for cyclic threads in us (optional) */
				threadStackSize,							/* Minimum stacksize, default 0: 16kB */
				(void *)ReplierApplication,				/* Pointer to the thread function */
				(void *)pReplierThreadParameter);		/* Pointer to the thread function parameters */
	if (vos_err == VOS_NO_ERR)
	{
		taulAppErr = setApplicationThreadHandleList(pReplierAppThreadHandle);
		if (taulAppErr != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "createReplierThread() failed. Set Application Thread Handle Error: %d\n", taulAppErr);
			/* Free Replier Application Thread Handle */
			vos_memFree(pReplierAppThreadHandle);
			return taulAppErr;
		}
		else
		{
			return TAUL_APP_NO_ERR;
		}
	}
	else
	{
		/* Replier Thread Number Count down */
		replierThreadNoCount--;
		vos_printLog(VOS_LOG_ERROR, "Replier Thread Create Err\n");
		/* Free Replier Application Thread Handle */
		vos_memFree(pReplierAppThreadHandle);
		return TAUL_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Replier Application main
 *
 *  @param[in]		pReplierThreadParameter			pointer to Replier Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE ReplierApplication (REPLIER_THREAD_PARAMETER_T *pReplierThreadParameter)
{
	TRDP_ERR_T									err = TRDP_NO_ERR;
	TAUL_APP_ERR_TYPE							taulAppErr = TAUL_APP_NO_ERR;
	VOS_ERR_T									vos_err = VOS_NO_ERR;
	VOS_QUEUE_T								queueHandle = NULL;
	CALLER_REPLIER_APP_THREAD_HANDLE_T		*pReplierAppThreadHandle;
	CALLER_REPLIER_APP_THREAD_HANDLE_T		*pReplierAppThreadConfirmHandle = NULL;			/* for Confirm */
	REPLIER_TELEGRAM_T						*pReplierReplyTelegram = NULL;
	REPLIER_TELEGRAM_T						*pReplierConfirmTelegram = NULL;
	REPLIER_TELEGRAM_T						*pReplyTelegram = NULL;
	UINT32										messageQueueReceviveWaitTIme = 0; /* micro second */
	trdp_apl_cbenv_t							*pReceiveMqMsg;
	UINT32										receiveMessageSize = 0;
	MD_APP_THREAD_TYPE						replierThreadType = MD_APP_UNKOWN_TYPE;
	UINT32										replyUserStatus = 0;					/* Reply OK:0, NG:!=0 */
	UINT32										datasetDstEnd = 0;
	UINT32										replierReceiveCount = 0;
	UINT32										datasetFirstElementNumber = 0;
	TRDP_IP_ADDR_T							networkByteSrcIpAddr = 0;			/* for convert URI to Source IP Address */
	TRDP_IP_ADDR_T							networkByteDstIpAddr = 0;			/* for convert URI to Destination IP Address */
	APPLICATION_THREAD_HANDLE_T				*pOwnApplicationThreadHandle = NULL;
	TRDP_IP_ADDR_T							ownSubnet = 0;						/* own Applicatin using I/F subnet */
	TRDP_IP_ADDR_T							receiveSubnet = 0;					/* receive MD message source IP Subnet */
	UINT32										sessionReferenceAddress = 0;
	UINT32										receiveSubnetId = 0;

	/* Message Queue Open */
	vos_err = vos_queueCreate(VOS_QUEUE_POLICY_OTHER, TRDP_QUEUE_MAX_MESG, &queueHandle);
	if (vos_err != VOS_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Replier Message Queue Open error\n");
		return TAUL_QUEUE_ERR;
	}
	else
	{
		/* Get Replier Application Thread Handle memory area */
		pReplierAppThreadHandle = (CALLER_REPLIER_APP_THREAD_HANDLE_T *)vos_memAlloc(sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
		if (pReplierAppThreadHandle == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"ReplierApplication() Failed. Replier Application Thread Handle vos_memAlloc() Err\n");
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "ReplierApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* Initialize Replier Application Thread Handle */
			memset(pReplierAppThreadHandle, 0, sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
		}
		/* Set Replier Message Queue Descriptor in Caller Application Thread Handle */
		pReplierAppThreadHandle->pTrdpMqDescriptor =  queueHandle;
		/* Set Listener Handle in Replier Application Thread Handle */
		pReplierAppThreadHandle->pMdAppThreadListener =  pReplierThreadParameter->pReplierTelegram->listenerHandleForTAUL;
		/* Set Replier Message Queue Descriptor */
		taulAppErr = setReplierAppThreadMessageQueueDescriptor(pReplierAppThreadHandle);
		if (taulAppErr != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Set Replier Message Queue Descriptor Failed. setReplierAppThreadMessageQueueDescriptor() error: %d\n", taulAppErr);
			/* Free Replier Application Thread Handle */
			vos_memFree(pReplierAppThreadHandle);
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "ReplierApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
			return TAUL_APP_ERR;
		}
	}

	/* Set Replier Thread Type */
   	/*  Source IP Address Convert host URI to IP address  */
	if (pReplierThreadParameter->pReplierTelegram->pSource != NULL)
	{
		if (pReplierThreadParameter->pReplierTelegram->pSource->pUriHost1 != NULL)
		{
			networkByteSrcIpAddr = vos_dottedIP (*(pReplierThreadParameter->pReplierTelegram->pSource->pUriHost1));
		}
		else
		{
			networkByteSrcIpAddr = IP_ADDRESS_NOTHING;
		}
	}
	else
	{
		networkByteSrcIpAddr = IP_ADDRESS_NOTHING;
	}
	/* Convert Destination */
	if (pReplierThreadParameter->pReplierTelegram->pDestination != NULL)
	{
		if (pReplierThreadParameter->pReplierTelegram->pDestination->pUriHost != NULL)
		{
			networkByteDstIpAddr = vos_dottedIP (*(pReplierThreadParameter->pReplierTelegram->pDestination->pUriHost));
		}
		else
		{
			networkByteDstIpAddr = IP_ADDRESS_NOTHING;
		}
	}
	else
	{
		networkByteDstIpAddr = IP_ADDRESS_NOTHING;
	}

	/* Get Reply(Mp or Mq) Telegram */
	pReplierReplyTelegram = searchReplierTelegramList (
								pHeadReplierTelegram,
								pReplierThreadParameter->pReplierTelegram->comId,
								networkByteSrcIpAddr,
								networkByteDstIpAddr,
								TRUE);
	if ((pReplierReplyTelegram == NULL)
		|| (pReplierReplyTelegram->messageType == TRDP_GOTTEN_REPLIER_MN_MR))
	{
		/* Set Replier Thread Type: Mn */
		replierThreadType = REPLIER_MN;
	}
	/* Check Reply Telegram : Mq */
	else if (pReplierReplyTelegram->messageType == TRDP_GOTTEN_REPLIER_MQ)
	{
		/*  Source IP Address Convert host URI to IP address  */
		if (pReplierReplyTelegram->pSource != NULL)
		{
			if (pReplierReplyTelegram->pSource->pUriHost1 != NULL)
			{
				networkByteSrcIpAddr = vos_dottedIP (*(pReplierReplyTelegram->pSource->pUriHost1));
			}
			else
			{
				networkByteSrcIpAddr = IP_ADDRESS_NOTHING;
			}
		}
		else
		{
			networkByteSrcIpAddr = IP_ADDRESS_NOTHING;
		}
		/* Convert Destination */
		if (pReplierReplyTelegram->pDestination != NULL)
		{
			if (pReplierReplyTelegram->pDestination->pUriHost != NULL)
			{
				networkByteDstIpAddr = vos_dottedIP (*(pReplierReplyTelegram->pDestination->pUriHost));
			}
			else
			{
				networkByteDstIpAddr = IP_ADDRESS_NOTHING;
			}
		}
		else
		{
			networkByteDstIpAddr = IP_ADDRESS_NOTHING;
		}

		/* Get Confirm Telegram */
		pReplierConfirmTelegram = searchReplierTelegramList (
										pHeadReplierTelegram,
										pReplierReplyTelegram->comId,
										networkByteSrcIpAddr,
										networkByteDstIpAddr,
										TRUE);
		if (pReplierConfirmTelegram == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"ReplierApplication() Failed. Confirm Telegram get Err\n");
			/* Free Replier Application Thread Handle */
			vos_memFree(pReplierAppThreadHandle);
			/* Destroy Message Queue */
			vos_err = vos_queueDestroy(queueHandle);
			if (vos_err != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "ReplierApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
				err = TAUL_APP_MQ_ERR;
			}
			return TAUL_APP_ERR;
		}
		else
		{
			/* Set Replier Thread Type: Mr-Mq-Mc */
			replierThreadType = REPLIER_MR_MQ_MC;
			/* Set Confirm Receive Session Handle Message Queue Descriptor */
			/* Get Replier Application Thread Handle memory area */
			pReplierAppThreadConfirmHandle = (CALLER_REPLIER_APP_THREAD_HANDLE_T *)vos_memAlloc(sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
			if (pReplierAppThreadConfirmHandle == NULL)
			{
				vos_printLog(VOS_LOG_ERROR,"ReplierApplication() Failed. Replier Application Thread Confirm Handle vos_memAlloc() Err\n");
				/* Free Replier Confirm Telegram */
				vos_memFree(pReplierConfirmTelegram);
				/* Free Replier Application Thread Handle */
				vos_memFree(pReplierAppThreadHandle);
				/* Destroy Message Queue */
				vos_err = vos_queueDestroy(queueHandle);
				if (vos_err != VOS_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "ReplierApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
					err = TAUL_APP_MQ_ERR;
				}
				return TAUL_APP_MEM_ERR;
			}
			else
			{
				/* Initialize Replier Application Thread Confirm Handle */
				memset(pReplierAppThreadConfirmHandle, 0, sizeof(CALLER_REPLIER_APP_THREAD_HANDLE_T));
			}
			/* Set Replier Message Queue Descriptor in Replier Application Thread Confirm Handle */
			pReplierAppThreadConfirmHandle->pTrdpMqDescriptor =  queueHandle;
			/* Set Listener Handle in Replier Application Thread Handle */
			pReplierAppThreadConfirmHandle->pMdAppThreadListener =  pReplierConfirmTelegram->listenerHandleForTAUL;
			/* Set Replier Message Queue Descriptor */
			taulAppErr = setReplierAppThreadMessageQueueDescriptor(pReplierAppThreadConfirmHandle);
			if (taulAppErr != TAUL_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "RepleirApplication() err. Set Replier Message Queue Descriptor Failed. setReplierAppThreadMessageQueueDescriptor() error: %d\n", taulAppErr);
				/* Free Replier Confirm Telegram */
				if (pReplierConfirmTelegram != NULL)
				{
					vos_memFree(pReplierConfirmTelegram);
				}
				/* Free Replier Application Thread Handle */
				vos_memFree(pReplierAppThreadHandle);
				/* Destroy Message Queue */
				vos_err = vos_queueDestroy(queueHandle);
				if (vos_err != VOS_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "ReplierApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
					err = TAUL_APP_MQ_ERR;
				}
				return TAUL_APP_ERR;
			}
		}
	}
	else
	{
		/* Set Replier Thread Type: Mr-Mp */
		replierThreadType = REPLIER_MR_MP;
	}

	/* Get Own Application Thread Handle */
	pOwnApplicationThreadHandle = searchApplicationThreadHandleList(
									pReplierThreadParameter->taulAppThreadId);
	if (pOwnApplicationThreadHandle == NULL)
	{
		vos_printLog(VOS_LOG_DBG,"ReplierApplication() failed. Nothing Own Application Thread Handle.\n");
		/* Free Replier Confirm Telegram */
		if (pReplierConfirmTelegram != NULL)
		{
			vos_memFree(pReplierConfirmTelegram);
		}
		/* Free Replier Application Thread Handle */
		vos_memFree(pReplierAppThreadHandle);
		/* Destroy Message Queue */
		vos_err = vos_queueDestroy(queueHandle);
		if (vos_err != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "ReplierApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
			err = TAUL_APP_MQ_ERR;
		}
		return TRDP_PARAM_ERR;
	}

	/* Get own Subnet */
	ownSubnet = pReplierThreadParameter->pReplierTelegram->appHandle->realIP & SUBNET_NO2_NETMASK;
	/* Display OwnSubnet */
	vos_printLog(VOS_LOG_DBG, "OwnSubnet:%d\n", ownSubnet);

	/* Replier main process */
	/* Display TimeStamp when Replier test start time */
	vos_printLog(VOS_LOG_INFO, "%s Replier test start.\n", vos_getTimeStamp());

	/* Replier Receive Send Loop */
	while(1)
	{
		/* Check Application Thread Terminate Indicate */
		if (pOwnApplicationThreadHandle != NULL)
		{
			if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
			{
				vos_printLog(VOS_LOG_DBG,"CallerApplication() Receive Application Thread Terminate Indicate");
				break;
			}
		}
		/* Receive Message Queue */
		vos_err = vos_queueReceive(
						queueHandle,
						(UINT8 **)&pReceiveMqMsg,
						&receiveMessageSize,
						messageQueueReceviveWaitTIme);
		if (vos_err != VOS_NO_ERR)
		{
//			vos_printLog(VOS_LOG_DBG,"ReplierApplication() vos_queueReceive err: %d\n", vos_err);
			/* Break Receive Message Queue because not Receive Message Queue */
			continue;
		}
		else if(receiveMessageSize <= 0)
		{
//			vos_printLog(VOS_LOG_DBG,"ReplierApplication() vos_queueReceive NOT Receive Message\n");
			/* Not Receive Message */
			continue;
		}
		else
		{
			/* Decide MD Message */
			/* Check ComId */
			/* Request comId (Mn or Mr) */
			if ((pReceiveMqMsg->Msg.comId != pReplierThreadParameter->pReplierTelegram->comId))
			{
				vos_printLog(VOS_LOG_ERROR, "Receive ComId ERROR\n");
			}
			else
			{
				/* Check Result Code */
				if (pReceiveMqMsg->Msg.resultCode == TRDP_NO_ERR)
				{
					/* Check msgType */
					switch(pReceiveMqMsg->Msg.msgType)
					{
						case TRDP_MSG_MR:
							/* MD Send Count */
							pReplierThreadParameter->pMdAppParameter->replierMdSendCounter++;
							/* Set Reply Telegram */
							pReplyTelegram = pReplierThreadParameter->pReplierTelegram->pNextReplierTelegram;
							/* Set Create Dataset Destination Address */
							datasetDstEnd = (UINT32)pReplyTelegram->dataset.pDatasetStartAddr;
							/* Create Dataset */
							taulAppErr = createDataset (
										datasetFirstElementNumber,
										pReplyTelegram->pDatasetDescriptor,
										&pReplyTelegram->dataset,
										(UINT8*)&datasetDstEnd);
							if (taulAppErr != TAUL_APP_NO_ERR)
							{
								vos_printLog(VOS_LOG_ERROR, "Replier Application Create Dataset Failed. createDataset() Error: %d\n", taulAppErr);
								break;
							}
							/* Increment datasetFristElementNumber */
							datasetFirstElementNumber++;
							/* Check Replier Telegram */
							if (pReplierReplyTelegram == NULL)
							{
								vos_printLog(VOS_LOG_ERROR, "Replier Application Replier Telegram Nothing Error: %d\n", taulAppErr);
								break;
							}
							else
							{
								/* Get Session Reference */
								memcpy(&sessionReferenceAddress, pReceiveMqMsg->pRefCon, sizeof(UINT32));
								/* Get SubnetId of Receive MD Packet */
								memcpy(&receiveSubnetId, (void *)pReceiveMqMsg->pRefCon, sizeof(UINT32));
								/* Display ReceiveSubnetID */
								vos_printLog(VOS_LOG_DBG, "receiveSubnetID:%d\n", receiveSubnetId);
								/* Check Receive MD Packet by SUbnet2 */
								if ((receiveSubnetId & RECEIVE_SUBNET2_MASK) == RECEIVE_SUBNET2_MASK)
								{
									receiveSubnet = SUBNET_NO2_NETMASK;
								}
								else
								{
									receiveSubnet = 0;
								}
								/* Display ReceiveSubnet, OwnSubnet */
								vos_printLog(VOS_LOG_DBG, "receiveSubnet:%u OwnSubnet:%d\n", receiveSubnet, ownSubnet);
								/* Check Receive Subnet : receive Subnet equal own Subnet */
								if (receiveSubnet == ownSubnet)
								{
									/* Display ReceiveSubnet, OwnSubnet */
									vos_printLog(VOS_LOG_DBG, "tau_ldReply() receiveSubnet:%u OwnSubnet:%d\n", receiveSubnet, ownSubnet);

									/* Send MD Reply */
									err = tau_ldReply (
											pReplierReplyTelegram->comId,
											replyUserStatus,
											pReplyTelegram->dataset.pDatasetStartAddr,
											pReplyTelegram->dataset.size,
											&sessionReferenceAddress);
									if (err != TRDP_NO_ERR)
									{
										/* MD Send Failure Count */
										pReplierThreadParameter->pMdAppParameter->replierMdSendFailureCounter++;
										/* Error : Send Reply */
										vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
									}
									else
									{
										/* MD Send Success Count */
										pReplierThreadParameter->pMdAppParameter->replierMdSendSuccessCounter++;
									}
								}
							}
						case TRDP_MSG_MN:
							/* Decide MD Transmission Result */
							taulAppErr = decideMdTransmissionResult(
									pReceiveMqMsg->pData,
									&(pReceiveMqMsg->dataSize));
							if (taulAppErr == TAUL_APP_NO_ERR)
							{
								/* MD Receive OK Count UP*/
								pReplierThreadParameter->pMdAppParameter->replierMdReceiveSuccessCounter++;
							}
							else
							{
								/* MD Receive NG Count */
								pReplierThreadParameter->pMdAppParameter->replierMdReceiveFailureCounter++;
							}
							/* MD Request Receive Count */
							pReplierThreadParameter->pMdAppParameter->replierMdRequestReceiveCounter++;
						break;
						default:
						break;
					}
				}
			}
			/* Receive Count up */
			replierReceiveCount++;

			/* Release Message Data */
			if (pReceiveMqMsg != NULL)
			{
				if (pReceiveMqMsg->pData != NULL)
				{
					vos_memFree(&pReceiveMqMsg->pData);
					pReceiveMqMsg->pData = NULL;
				}
				vos_memFree(pReceiveMqMsg);
				pReceiveMqMsg = NULL;
			}

			/* Check Replier Receive Count : Receive finish ? */
			/* Message Kind:Mp receive Count >=CycleNumber */
			/* Message Kind:Mq confirm receive Count >= CycleNumber */
			if ((pReplierThreadParameter->pMdAppParameter->mdAppCycleNumber != 0)
			&& (((replierThreadType == REPLIER_MR_MP) && (replierReceiveCount >= pReplierThreadParameter->pMdAppParameter->mdAppCycleNumber))
				|| ((replierThreadType == REPLIER_MR_MQ_MC) && (pReplierThreadParameter->pMdAppParameter->replierMdConfrimReceiveCounter >= pReplierThreadParameter->pMdAppParameter->mdAppCycleNumber))))
			{
				/* Display TimeStamp when Replier test finish time */
				vos_printLog(VOS_LOG_DBG, "%s Replier test finish.\n", vos_getTimeStamp());
				break;
			}
		}
	}

	/* Delete AppThereadSession Message Queue Descriptor */
	taulAppErr = deleteReplierAppThreadMessageQueueDescriptor(pReplierAppThreadHandle);
	if (taulAppErr != TAUL_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Replier AppThread Session Message Queue Descriptor delete Err\n");
	}
	/* Delete AppThereadSession Message Queue Descriptor Reply*/
	if (pReplierAppThreadConfirmHandle != NULL)
	{
		taulAppErr = deleteReplierAppThreadMessageQueueDescriptor(pReplierAppThreadConfirmHandle);
		if (taulAppErr != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Replier AppThread Confirm Session Message Queue Descriptor delete Err\n");
		}
	}

	/* Destroy Message Queue */
	vos_err = vos_queueDestroy(pReplierAppThreadHandle->pTrdpMqDescriptor);
	if (vos_err != VOS_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "ReplierApplication() failed. Destroy Message Queue Error: %d\n", vos_err);
		err = TAUL_APP_MQ_ERR;
	}

	/* Set Application Thread Terminate Status */
	pOwnApplicationThreadHandle->taulAppThreadState = TAUL_APP_THREAD_TERMINATE;

	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Application Main Thread Common Function
 */

/**********************************************************************************************************************/
/** main thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         TAUL_APP_NO_ERR					no error
 *  @retval         TAUL_APP_ERR					error
 */
TAUL_APP_ERR_TYPE command_main_proc(void)
{
	TAUL_APP_ERR_TYPE err = TAUL_APP_NO_ERR;
	UINT32 getCommandLength = 0;					/* Input Command Length */
	UINT32 i = 0;										/* loop counter */
	UINT8 operand = 0;								/* Input Command Operand Number */
	CHAR8 getCommand[GET_COMMAND_MAX];				/* Input Command */
	CHAR8 argvGetCommand[GET_COMMAND_MAX];			/* Input Command for argv */
	static CHAR8 *argvCommand[100];					/* Command argv */
	INT32 startPoint;									/* Copy Start Point */
	INT32 endPoint;									/* Copy End Point */
	COMMAND_VALUE_T *pCommandValue = NULL;			/* Command Value */

	/* Decide Command */
	for(;;)
	{
		/* Initialize */
		memset(getCommand, 0, sizeof(getCommand));
		memset(argvGetCommand, 0, sizeof(argvGetCommand));
		memset(argvCommand, 0, sizeof(argvCommand));
		operand = 0;
		startPoint = 0;
		endPoint = 0;

		printf("Input Command\n");

		/* Input Command */
		fgets(getCommand, sizeof(getCommand), stdin);

		/* Get Command Length */
		getCommandLength = strlen(getCommand);
		operand++;

		/* Create argvCommand and Operand Number */
		for(i=0; i < getCommandLength; i++)
		{
			/* Check SPACE */
			if(getCommand[i] == SPACE )
			{
				/* Get argvCommand */
				strncpy(&argvGetCommand[startPoint], &getCommand[startPoint], i-startPoint);
				argvCommand[operand] = &argvGetCommand[startPoint];
				startPoint = i+1;
				operand++;
			}
		}

		/* Copy Last Operand */
		strncpy(&argvGetCommand[startPoint], &getCommand[startPoint], getCommandLength-startPoint-1);
		argvCommand[operand] = &argvGetCommand[startPoint];

		/* Get PD_COMMAND_VALUE Area */
		pCommandValue = (COMMAND_VALUE_T *)vos_memAlloc(sizeof(COMMAND_VALUE_T));
		if (pCommandValue == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "COMMAND_VALUE vos_memAlloc() Err\n");
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* COMMAND VALUE Initialize */
			memset(pCommandValue, 0, sizeof(COMMAND_VALUE_T));

			/* Analyze Command */
			err = analyzeCommand(operand+1, argvCommand, pCommandValue);
			/* command -h = TAUL_APP_COMMAND_ERR */
			if (err == TAUL_APP_COMMAND_ERR)
			{
				continue;
			}
			/* command -Q : Quit */
			else if(err == TAUL_APP_QUIT_ERR)
			{
				/* Quit Command */
				return TAUL_APP_QUIT_ERR;
			}
			else if(err == TAUL_APP_NO_ERR)
			{
				continue;
			}
			else
			{
				/* command err */
				vos_printLog(VOS_LOG_ERROR, "Command Value Err\n");
				vos_memFree(pCommandValue);
				pCommandValue = NULL;
			}
		}
	}
}

/**********************************************************************************************************************/
/** analyze command
 *
 *  @param[in]		void
 *
 *  @retval         TAUL_APP_NO_ERR			no error
 *  @retval         TAUL_APP_ERR				error
 */
TAUL_APP_ERR_TYPE analyzeCommand(INT32 argc, CHAR8 *argv[], COMMAND_VALUE_T *pCommandValue)
{
	COMMAND_VALUE_T getCommandValue = {{0}}; 	/* command value */
	INT32 i = 0;
	TAUL_APP_ERR_TYPE err = TAUL_APP_NO_ERR;
	UINT32 uint32_value = 0;						/* use variable number in order to get 32bit value */
	/* add Listener Parameter */
	UINT32						addListenerComId = 123456;
	TRDP_IP_ADDR_T			addListenerDstIpAddr = 0xefff0101;		/* multicast Destination IP: 239.255.1.1 */
	const TRDP_URI_USER_T	addListenerDstURI =  "239.255.1.1";
	UINT32						addListenerUserRef = 654321;

	/* Command analysis */
	for(i=1; i < argc ; i++)
	{
		if (argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
			case 'Q':
				/* TAUL Application Terminate */
				err = taulApplicationTerminate();
				if (err != TAUL_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "TAUL Application Terminate Err\n");
				}
				return TAUL_APP_QUIT_ERR;
				break;
			case 'l':
				/* add Listener command */
				err = tau_ldAddListener(
						addListenerComId,
						addListenerDstIpAddr,
						addListenerDstURI,
						(void*)&addListenerUserRef);
				if (err != TAUL_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "TAUL Add Listener Err\n");
				}
				return TAUL_APP_COMMAND_ERR;
				break;
			case 'r':
				/* Re Init command */
				if (argv[i+1] != NULL)
				{
					/* Get Re-Init subnet from an option argument */
					sscanf(argv[i+1], "%x", &uint32_value);
				}
				/* Re Init */
				err = tau_ldReInit(uint32_value);
				if (err != TAUL_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "TAUL Re init Err. subnet:%d \n", uint32_value);
				}
				return TAUL_APP_COMMAND_ERR;
				break;
			case 'h':
			case '?':
				printf("Unknown or required argument option -%c\n", optopt);
				printf("Usage: COMMAND "
						"[-Q] "
						"[-h] "
						"\n");
				printf("-Q,	--taul-test-quit	TAUL TEST Quit\n");
				printf("-h,	--help\n");
				return TAUL_APP_COMMAND_ERR;
			break;
			default:
				printf("Unknown or required argument option -%c\n", optopt);
				return TAUL_APP_PARAM_ERR;
			}
		}
	}

	/* Return Command Value */
	*pCommandValue = getCommandValue;
	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** TRDP TAUL Application Terminate
 *
 *
 *  @retval         TAUL_APP_NO_ERR					no error
 *  @retval         TAUL_APP_ERR						error
 */
TAUL_APP_ERR_TYPE taulApplicationTerminate(
		void)
{
	TRDP_ERR_T		err = TRDP_NO_ERR;
	UINT32			i = 0;

	/* Terminate Application Thread */
	for(i=0; i < APPLICATION_THREAD_LIST_MAX; i++)
	{
		if(applicationThreadHandleList[i].appHandle!= NULL)
		{
			/* Set Application Thread Terminate Indicate */
			applicationThreadHandleList[i].taulAppThreadState = TAUL_APP_THREAD_CANCLE_RECEIVE;
			/* Check Application Thread Terminate */
			while (applicationThreadHandleList[i].taulAppThreadState != TAUL_APP_THREAD_TERMINATE)
			{
				/* Wait Application Terminate */
				vos_threadDelay(100000);
			}
			vos_printLog(VOS_LOG_INFO, "Application Thread#%d Terminate\n", i+1);
		}
	}

	/* TRDP Terminate */
	err = tau_ldTerminate();
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "taulApplicationTerminate() Failed. tau_ldTerminate() Err: %d\n", err);
		return TAUL_APP_ERR;
	}
	else
	{
		/* Display TimeStamp when tlc_terminate time */
		vos_printLog(VOS_LOG_DBG, "%s TRDP Terminate.\n", vos_getTimeStamp());
	}
	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (INT32 argc, CHAR8 *argv[])
{
	TRDP_ERR_T								err = TRDP_NO_ERR;
	PUBLISHER_THREAD_PARAMETER_T		*pPublisherThreadParameter = NULL;
	PUBLISH_TELEGRAM_T					*iterPublishTelegram = NULL;
	extern PUBLISH_TELEGRAM_T			*pHeadPublishTelegram;
	SUBSCRIBER_THREAD_PARAMETER_T		*pSubscriberThreadParameter = NULL;
	SUBSCRIBE_TELEGRAM_T					*iterSubscribeTelegram = NULL;
	extern SUBSCRIBE_TELEGRAM_T			*pHeadSubscribeTelegram;
	PD_REQUESTER_THREAD_PARAMETER_T		*pPdRequesterThreadParameter = NULL;
	PD_REQUEST_TELEGRAM_T				*iterPdRequestTelegram = NULL;
	extern PD_REQUEST_TELEGRAM_T		*pHeadPdRequestTelegram;
	CALLER_THREAD_PARAMETER_T			*pCallerThreadParameter = NULL;
	CALLER_TELEGRAM_T						*iterCallerTelegram = NULL;
	extern CALLER_TELEGRAM_T				*pHeadCallerTelegram;
	REPLIER_THREAD_PARAMETER_T			*pReplierThreadParameter = NULL;
	REPLIER_TELEGRAM_T					*iterReplierTelegram = NULL;
	extern REPLIER_TELEGRAM_T			*pHeadReplierTelegram;

	UINT32									publisherApplicationId = 0;					/* Publisher Thread Application Id */
	UINT32									subscriberApplicationId = 0;				/* Subscriber Thread Application Id */
	UINT32									pdRequesterApplicationId = 0;				/* PD Requester Thread Application Id */
	UINT32									callerApplicationId = 0;						/* Caller Thread Application Id */
	UINT32									replierApplicationId = 0;					/* Replier Thread Application Id */
	UINT32									taulApplicationThreadId = 0;				/* TAUL Application Thread Id */

	/* Using Receive Subnet in order to Write PD in Traffic Store  */
	UINT32									TS_SUBNET_TYPE = SUBNET1;
	UINT32									usingReceiveSubnetId = 0;
	TAU_LD_CONFIG_T						ladderConfig = {0};
	UINT32									index = 0;							/* Loop Counter */
	/* For Get IP Address */
	UINT32 noOfIfaces = 10;
	VOS_IF_REC_T ifAddressTable[noOfIfaces];
	TRDP_IP_ADDR_T ownIpAddress = 0;
#ifdef __linux
	CHAR8 SUBNETWORK_ID1_IF_NAME[] = "eth0";
#elif defined(__APPLE__)
	CHAR8 SUBNETWORK_ID1_IF_NAME[] = "en0";
#endif

	/* Get I/F address */
	if (vos_getInterfaces(&noOfIfaces, ifAddressTable) != VOS_NO_ERR)
	{
		printf("main() failed. vos_getInterfaces() error.\n");
		return TRDP_SOCK_ERR;
	}

	/* Get All I/F List */
	for (index = 0; index < noOfIfaces; index++)
	{
		if (strncmp(ifAddressTable[index].name, SUBNETWORK_ID1_IF_NAME, sizeof(SUBNETWORK_ID1_IF_NAME)) == 0)
		{
				/* Get Sub-net Id1 Address */
			ownIpAddress = (TRDP_IP_ADDR_T)(ifAddressTable[index].ipAddr);
			break;
		}
	}

	/* Set Ladder Config */
	ladderConfig.ownIpAddr = ownIpAddress;
	ladderConfig.callConf = &callConf;
	ladderConfig.rcvConf = &rcvConf;

	/* Initialize TAUL */
	err = tau_ldInit(dbgOut, &ladderConfig);
	if (err != TRDP_NO_ERR)
	{
		printf("TRDP Ladder Support Initialize failed. tau_ldInit() error: %d \n", err);
		return TAUL_INIT_ERR;
	}

	/* Display TAUL Application Version */
	vos_printLog(VOS_LOG_INFO,
					"TAUL Application Version %s: TRDP Setting successfully\n",
					TAUL_APP_VERSION);

	/* Set using Sub-Network :AUTO*/
	TS_SUBNET_TYPE = SUBNET_AUTO;
	/* Set Using Sub-Network */
    err = tau_ldSetNetworkContext(TS_SUBNET_TYPE);
    if (err != TRDP_NO_ERR)
    {
    	vos_printLog(VOS_LOG_ERROR, "Set Writing Traffic Store Sub-network error\n");
        return TAUL_APP_ERR;
    }
	/* Get Using Sub-Network */
    err = tau_ldGetNetworkContext(&usingReceiveSubnetId);
    if (err != TRDP_NO_ERR)
    {
    	vos_printLog(VOS_LOG_ERROR, "Get Writing Traffic Store Sub-network error\n");
        return TAUL_APP_ERR;
    }

	/* Create Role Application Thread */
	/* Read Publish Telegram */
	for (iterPublishTelegram = pHeadPublishTelegram;
			iterPublishTelegram != NULL;
			iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram)
	{
		pPublisherThreadParameter = (PUBLISHER_THREAD_PARAMETER_T *)vos_memAlloc(sizeof(PUBLISHER_THREAD_PARAMETER_T));
		if (pPublisherThreadParameter == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "Create Publisher Thread Failed. vos_memAlloc() publisherThreadParameter Err\n");
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* Initialize Thread Parameter Area */
			memset(pPublisherThreadParameter, 0, sizeof(PUBLISHER_THREAD_PARAMETER_T));
			/* Set Thread Parameter */
			pPublisherThreadParameter->pPublishTelegram = iterPublishTelegram;
			/* Set TAUL Application Thread ID */
			pPublisherThreadParameter->taulAppThreadId = taulApplicationThreadId;
			taulApplicationThreadId++;
			/* Get Application Parameter Area */
			pPublisherThreadParameter->pPdAppParameter = (PD_APP_PARAMETER_T *)vos_memAlloc(sizeof(PD_APP_PARAMETER_T));
			if (pPublisherThreadParameter->pPdAppParameter == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "Create Publisher Thread Failed. vos_memAlloc() Application Parameter Err\n");
				/* Free Publisher Thread Parameter */
				vos_memFree(pPublisherThreadParameter);
				return TAUL_APP_MEM_ERR;
			}
			else
			{
				/* Initialize Application Parameter */
				memset(pPublisherThreadParameter->pPdAppParameter, 0, sizeof(PD_APP_PARAMETER_T));
				/* Set Application Parameter */
				/* Set Update Time write Dataset in Traffic Store */
				pPublisherThreadParameter->pPdAppParameter->pdAppCycleTime = DEFAULT_PD_APP_CYCLE_TIME;
				/* Set Send Cycle Number */
				pPublisherThreadParameter->pPdAppParameter->pdSendCycleNumber = DEFAULT_PD_SEND_CYCLE_NUMBER;
				/* Set Send Using Subnet */
				pPublisherThreadParameter->pPdAppParameter->writeTrafficStoreSubnet = DEFAULT_WRITE_TRAFFIC_STORE_SUBNET;
				/* Set Application Parameter ID */
				pPublisherThreadParameter->pPdAppParameter->appParameterId = publisherApplicationId;
				publisherApplicationId++;
			}
			/* Create Publisher Thread */
			err = createPublisherThread(pPublisherThreadParameter);
			if (err != TAUL_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Create Publisher Application Thread Failed. createPublishThread() Error: %d\n", err);
				/* Free Publisher Thread PD Application Parameter */
				vos_memFree(pPublisherThreadParameter->pPdAppParameter);
				/* Free Publisher Thread Parameter */
				vos_memFree(pPublisherThreadParameter);
				return TAUL_APP_ERR;
			}
			else
			{
				vos_printLog(VOS_LOG_INFO, "Create Publisher Application Thread. comId: %d\n", pPublisherThreadParameter->pPublishTelegram->comId);
			}
		}
    }

	/* Read Subscribe Telegram */
	for (iterSubscribeTelegram = pHeadSubscribeTelegram;
			iterSubscribeTelegram != NULL;
			iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram)
	{
		pSubscriberThreadParameter = (SUBSCRIBER_THREAD_PARAMETER_T *)vos_memAlloc(sizeof(SUBSCRIBER_THREAD_PARAMETER_T));
		if (pSubscriberThreadParameter == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "Create Subscribeer Thread Failed. vos_memAlloc() subscriberThreadParameter Err\n");
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* Initialize Thread Parameter Area */
			memset(pSubscriberThreadParameter, 0, sizeof(SUBSCRIBER_THREAD_PARAMETER_T));
			/* Set Thread Parameter */
			pSubscriberThreadParameter->pSubscribeTelegram = iterSubscribeTelegram;
			/* Set TAUL Application Thread ID */
			pSubscriberThreadParameter->taulAppThreadId = taulApplicationThreadId;
			taulApplicationThreadId++;
			/* Get Application Parameter Area */
			pSubscriberThreadParameter->pPdAppParameter = (PD_APP_PARAMETER_T *)vos_memAlloc(sizeof(PD_APP_PARAMETER_T));
			if (pSubscriberThreadParameter->pPdAppParameter == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "Create Subscriber Thread Failed. malloc Application Parameter Err\n");
				/* Free Subscriber Thread Parameter */
				vos_memFree(pSubscriberThreadParameter);
				return TAUL_APP_MEM_ERR;
			}
			else
			{
				/* Initialize Application Parameter */
				memset(pSubscriberThreadParameter->pPdAppParameter, 0, sizeof(PD_APP_PARAMETER_T));
				/* Set Application Parameter */
				/* Set Update Time write Dataset in Traffic Store */
				pSubscriberThreadParameter->pPdAppParameter->pdAppCycleTime = DEFAULT_PD_APP_CYCLE_TIME;
				/* Set Send Cycle Number */
				pSubscriberThreadParameter->pPdAppParameter->pdReceiveCycleNumber = DEFAULT_PD_RECEIVE_CYCLE_NUMBER;
				/* Set Send Using Subnet */
				pSubscriberThreadParameter->pPdAppParameter->writeTrafficStoreSubnet = DEFAULT_WRITE_TRAFFIC_STORE_SUBNET;
				/* Set Application Parameter ID */
				pSubscriberThreadParameter->pPdAppParameter->appParameterId = subscriberApplicationId;
				subscriberApplicationId++;
			}
			/* Create Subscriber Thread */
			err = createSubscriberThread(pSubscriberThreadParameter);
			if (err != TAUL_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Create Subscriber Application Thread Failed. createSubscribeThread() Error: %d\n", err);
				/* Free Subscriber Thread PD Application Parameter */
				vos_memFree(pSubscriberThreadParameter->pPdAppParameter);
				/* Free Subscriber Thread Parameter */
				vos_memFree(pSubscriberThreadParameter);
				return TAUL_APP_ERR;
			}
			else
			{
				vos_printLog(VOS_LOG_INFO, "Create Subscriber Application Thread. comId: %d\n", pSubscriberThreadParameter->pSubscribeTelegram->comId);
			}
		}
	}

	/* Read PD Requester Telegram */
	for (iterPdRequestTelegram = pHeadPdRequestTelegram;
			iterPdRequestTelegram != NULL;
			iterPdRequestTelegram = iterPdRequestTelegram->pNextPdRequestTelegram)
	{
		pPdRequesterThreadParameter = (PD_REQUESTER_THREAD_PARAMETER_T *)vos_memAlloc(sizeof(PD_REQUESTER_THREAD_PARAMETER_T));
		if (pPdRequesterThreadParameter == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "Create Pd Requester Thread Failed. vos_memAlloc() PdRequesterThreadParameter Err\n");
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* Initialize Thread Parameter Area */
			memset(pPdRequesterThreadParameter, 0, sizeof(PD_REQUESTER_THREAD_PARAMETER_T));
			/* Set Thread Parameter */
			pPdRequesterThreadParameter->pPdRequestTelegram = iterPdRequestTelegram;
			/* Set TAUL Application Thread ID */
			pPdRequesterThreadParameter->taulAppThreadId = taulApplicationThreadId;
			taulApplicationThreadId++;
			/* Get Application Parameter Area */
			pPdRequesterThreadParameter->pPdAppParameter = (PD_APP_PARAMETER_T *)vos_memAlloc(sizeof(PD_APP_PARAMETER_T));
			if (pPdRequesterThreadParameter->pPdAppParameter == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "Create PD Requester Thread Failed. vos_memAlloc() Application Parameter Err\n");
				/* Free PD Requester Thread Parameter */
				vos_memFree(pPdRequesterThreadParameter);
				return TAUL_APP_MEM_ERR;
			}
			else
			{
				/* Initialize Application Parameter */
				memset(pPdRequesterThreadParameter->pPdAppParameter, 0, sizeof(PD_APP_PARAMETER_T));
				/* Set Application Parameter */
				/* Set Update Time write Dataset in Traffic Store */
				pPdRequesterThreadParameter->pPdAppParameter->pdAppCycleTime = DEFAULT_PD_APP_CYCLE_TIME;
				/* Set Send Cycle Number */
				pPdRequesterThreadParameter->pPdAppParameter->pdSendCycleNumber = DEFAULT_PD_SEND_CYCLE_NUMBER;
				/* Set Send Using Subnet */
				pPdRequesterThreadParameter->pPdAppParameter->writeTrafficStoreSubnet = DEFAULT_WRITE_TRAFFIC_STORE_SUBNET;
				/* Set Application Parameter ID */
				pPdRequesterThreadParameter->pPdAppParameter->appParameterId = pdRequesterApplicationId;
				pdRequesterApplicationId++;
			}
			/* Create PD Requester Thread */
			err = createPdRequesterThread(pPdRequesterThreadParameter);
			if (err != TAUL_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Create PD Requester Application Thread Failed. createPdRequesterThread() Error: %d\n", err);
				/* Free PD Requester Thread PD Application Parameter */
				vos_memFree(pPdRequesterThreadParameter->pPdAppParameter);
				/* Free PD Requester Thread Parameter */
				vos_memFree(pPdRequesterThreadParameter);
				return TAUL_APP_ERR;
			}
			else
			{
				vos_printLog(VOS_LOG_INFO, "Create PD Requester Application Thread. comId: %d\n", pPdRequesterThreadParameter->pPdRequestTelegram->comId);
			}
		}
	}

    /* Read Caller Telegram */
    for (iterCallerTelegram = pHeadCallerTelegram;
			iterCallerTelegram != NULL;
			iterCallerTelegram = iterCallerTelegram->pNextCallerTelegram)
    {
    	/* Check Message Type */
    	if (iterCallerTelegram->messageType == TRDP_GOTTEN_CALLER_MN_MR)
    	{
			pCallerThreadParameter = (CALLER_THREAD_PARAMETER_T *)vos_memAlloc(sizeof(CALLER_THREAD_PARAMETER_T));
			if (pCallerThreadParameter == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "Create Caller Thread Failed. vos_memAlloc() CallerThreadParameter Err\n");
				return TAUL_APP_MEM_ERR;
			}
			else
			{
				/* Initialize Thread Parameter Area */
				memset(pCallerThreadParameter, 0, sizeof(CALLER_THREAD_PARAMETER_T));
				/* Set Thread Parameter */
				pCallerThreadParameter->pCallerTelegram = iterCallerTelegram;
				/* Set TAUL Application Thread ID */
				pCallerThreadParameter->taulAppThreadId = taulApplicationThreadId;
				taulApplicationThreadId++;
				/* Get Application Parameter Area */
				pCallerThreadParameter->pMdAppParameter = (MD_APP_PARAMETER_T *)vos_memAlloc(sizeof(MD_APP_PARAMETER_T));
				if (pCallerThreadParameter->pMdAppParameter == NULL)
				{
					vos_printLog(VOS_LOG_ERROR, "Create Caller Thread Failed. vos_memAlloc() Application Parameter Err\n");
					/* Free Caller Thread Parameter */
					vos_memFree(pCallerThreadParameter);
					return TAUL_APP_MEM_ERR;
				}
				else
				{
					/* Initialize Application Parameter */
					memset(pCallerThreadParameter->pMdAppParameter, 0, sizeof(MD_APP_PARAMETER_T));
					/* Set Application Parameter */
					/* Set Send Cycle Number */
					pCallerThreadParameter->pMdAppParameter->mdAppCycleNumber = DEFAULT_MD_APP_CYCLE_NUMBER;
					/* Set Send Cycle Time */
					pCallerThreadParameter->pMdAppParameter->mdAppCycleTime = DEFAULT_MD_APP_CYCLE_TIME;
					/* Set Send Destination IP Address */
					pCallerThreadParameter->pMdAppParameter->callerAppDestinationAddress = DEFAULT_CALLER_DEST_IP_ADDRESS;
					/* Set Send Destination URI */
					memcpy(pCallerThreadParameter->pMdAppParameter->callerAppDestinationURI, DEFAULT_CALLER_DEST_URI, sizeof(TRDP_URI_HOST_T));
					/* Set Number Of Replier */
					pCallerThreadParameter->pMdAppParameter->callerAppNumberOfReplier = DEFAULT_CALLER_NUMBER_OF_REPLIER;
					/* Set Application Parameter ID */
					pCallerThreadParameter->pMdAppParameter->appParameterId = callerApplicationId;
					callerApplicationId++;
					/* Set Send Interval Type */
					pCallerThreadParameter->pMdAppParameter->callerAppSendIntervalType = DEFAULT_CALLER_SEND_INTERVAL_TYPE;
				}
				/* Create Caller Thread */
				err = createCallerThread(pCallerThreadParameter);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Create Caller Application Thread Failed. createCallerThread() Error: %d\n", err);
					/* Free Caller Thread MD Application Parameter */
					vos_memFree(pCallerThreadParameter->pMdAppParameter);
					/* Free Caller Thread Parameter */
					vos_memFree(pCallerThreadParameter);
					return TAUL_APP_ERR;
				}
				else
				{
					vos_printLog(VOS_LOG_INFO, "Create Caller Application Thread. comId: %d\n", pCallerThreadParameter->pCallerTelegram->comId);
				}
			}
    	}
    }

    /* Read Replier Telegram */
	for (iterReplierTelegram = pHeadReplierTelegram;
			iterReplierTelegram != NULL;
			iterReplierTelegram = iterReplierTelegram->pNextReplierTelegram)
	{
		/* Check Message Type */
		if (iterReplierTelegram->messageType == TRDP_GOTTEN_REPLIER_MN_MR)
		{
			pReplierThreadParameter = (REPLIER_THREAD_PARAMETER_T *)vos_memAlloc(sizeof(REPLIER_THREAD_PARAMETER_T));
			if (pReplierThreadParameter == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "Create Replier Thread Failed. vos_memAlloc() ReplierThreadParameter Err\n");
				return TAUL_APP_MEM_ERR;
			}
			else
			{
				/* Initialize Thread Parameter Area */
				memset(pReplierThreadParameter, 0, sizeof(REPLIER_THREAD_PARAMETER_T));
				/* Set Thread Parameter */
				pReplierThreadParameter->pReplierTelegram = iterReplierTelegram;
				/* Set TAUL Application Thread ID */
				pReplierThreadParameter->taulAppThreadId = taulApplicationThreadId;
				taulApplicationThreadId++;
				/* Get Application Parameter Area */
				pReplierThreadParameter->pMdAppParameter = (MD_APP_PARAMETER_T *)vos_memAlloc(sizeof(MD_APP_PARAMETER_T));
				if (pReplierThreadParameter->pMdAppParameter == NULL)
				{
					vos_printLog(VOS_LOG_ERROR, "Create Replier Thread Failed. vos_memAlloc() Application Parameter Err\n");
					/* Free Replier Thread Parameter */
					vos_memFree(pReplierThreadParameter);
					return TAUL_APP_MEM_ERR;
				}
				else
				{
					/* Initialize Application Parameter */
					memset(pReplierThreadParameter->pMdAppParameter, 0, sizeof(MD_APP_PARAMETER_T));
					/* Set Application Parameter */
					/* Set Receive Cycle Number */
					pReplierThreadParameter->pMdAppParameter->mdAppCycleNumber = DEFAULT_MD_APP_CYCLE_NUMBER;
					/* Set Application Parameter ID */
					pReplierThreadParameter->pMdAppParameter->appParameterId = replierApplicationId;
					replierApplicationId++;
				}
				/* Create Replier Thread */
				err = createReplierThread(pReplierThreadParameter);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Create Replier Application Thread Failed. createReplierThread() Error: %d\n", err);
					/* Free Replier Thread MD Application Parameter */
					vos_memFree(pReplierThreadParameter->pMdAppParameter);
					/* Free Replier Thread Parameter */
					vos_memFree(pReplierThreadParameter);
					return TAUL_APP_ERR;
				}
				else
				{
					vos_printLog(VOS_LOG_INFO, "Create Replier Application Thread. comId: %d\n", pReplierThreadParameter->pReplierTelegram->comId);
				}
			}
		}
	}

	/* Command main proc */
	err = command_main_proc();

    return 0;
}

#endif /* TRDP_OPTION_LADDER */
