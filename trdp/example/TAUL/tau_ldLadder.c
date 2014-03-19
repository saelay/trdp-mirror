/******************************************************************************/
/**
 * @file            tau_ldLadder.c
 *
 * @brief           Functions for Ladder Support TAUL API
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, Toshiba Corporation
 *
 * @remarks This source code corresponds to TRDP_LADDER open source software.
 *          This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Toshiba Corporation, Japan, 2013. All rights reserved.
 *
 * $Id$
 *
 */

#ifdef TRDP_OPTION_LADDER
/*******************************************************************************
 * INCLUDES
 */
#include <string.h>

#include <sys/ioctl.h>
#include <netinet/in.h>
#ifdef __linux
#   include <linux/if.h>
#else
#include <net/if.h>
#endif
#include <unistd.h>

#include "trdp_utils.h"
#include "trdp_if.h"

#include "vos_private.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "vos_shared_mem.h"
#include "tau_ladder.h"
#include "tau_ladder_app.h"
#include "tau_ldLadder.h"
#include "tau_ldLadder_config.h"

/*******************************************************************************
 * DEFINES
 */

/*******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 * TRDP_OPTION_TRAFFIC_SHAPING  Locals
 */

/******************************************************************************
 *   Globals
 */

/* Telegram List Pointer */
PUBLISH_TELEGRAM_T			*pHeadPublishTelegram = NULL;						/* Top Address of Publish Telegram List */
SUBSCRIBE_TELEGRAM_T			*pHeadSubscribeTelegram = NULL;						/* Top Address of Subscribe Telegram List */
PD_REQUEST_TELEGRAM_T		*pHeadPdRequestTelegram = NULL;						/* Top Address of PD Request Telegram List */
CALLER_TELEGRAM_T				*pHeadCallerTelegram = NULL;						/* Top Address of Subscribe Telegram List */
REPLIER_TELEGRAM_T			*pHeadReplierTelegram = NULL;						/* Top Address of Replier Telegram List */
/* Waiting Reference List Pointer */
WAITING_RECEIVE_REPLY_T		*pHeadWaitingReceiveReplyReference = NULL;		/* Top Address of Waiting Receive Reply Reference List */
WAITING_SEND_REPLY_T			*pHeadWaitingSendReplyReference = NULL;			/* Top Address of Waiting Send Reply Reference List */
WAITING_RECEIVE_REQUEST_T	*pHeadWaitingReceiveRequestReference = NULL;		/* Top Address of Waiting Receive Request Reference List */
WAITING_RECEIVE_CONFIRM_T	*pHeadWaitingReceiveConfirmReference = NULL;		/* Top Address of Waiting Receive Confirm Reference List */
/* Listener Handle List Pointer */
LISTENER_HANDLE_T				*pHeadListenerHandleList = NULL;					/* Top Address of Listener Handle List */
/* Mutex */
VOS_MUTEX_T pPublishTelegramMutex = NULL;						/* pointer to Mutex for Publish Telegram */
VOS_MUTEX_T pSubscribeTelegramMutex = NULL;					/* pointer to Mutex for Subscribe Telegram */
VOS_MUTEX_T pPdRequestTelegramMutex = NULL;					/* pointer to Mutex for PD Request Telegram */
VOS_MUTEX_T pCallerTelegramMutex = NULL;						/* pointer to Mutex for Caller Telegram */
VOS_MUTEX_T pReplierTelegramMutex = NULL;						/* pointer to Mutex for Replier Telegram */
VOS_MUTEX_T pWaitingReceiveReplyReferenceMutex = NULL;		/* pointer to Mutex for Waiting Receive Reply Reference */
VOS_MUTEX_T pWaitingSendReplyReferenceMutex = NULL;			/* pointer to Mutex for Waiting Send Reply Reference */
VOS_MUTEX_T pWaitingReceiveRequestReferenceMutex = NULL;	/* pointer to Mutex for Waiting Receive Request Reference */
VOS_MUTEX_T pWaitingReceiveConfirmReferenceMutex = NULL;	/* pointer to Mutex for Waiting Receive Confirm Reference */
VOS_MUTEX_T pListenerHandleListMutex = NULL;					/* pointer to Mutex for Listener Handle List */

/*  Marshalling configuration initialized from datasets defined in xml  */
TRDP_MARSHALL_CONFIG_T		marshallConfig = {&tau_marshall, &tau_unmarshall, NULL};	/** Marshaling/unMarshalling configuration	*/

/* Head MD Listener Handle for Delete Listener */
LISTENER_HANDLE_T				*pHeadListenerHandle = NULL;	/* Listener Handle for All Listener Delete */

/* I/F Address */
TRDP_IP_ADDR_T subnetId1Address = 0;
TRDP_IP_ADDR_T subnetId2Address = 0;
/* Use I/F for Send MD Message */
UINT32 useMdSendSubnetType = 0;				/* Using Subnet: MD_SEND_USE_SUBNET1 or MD_SEND_USE_SUBNET2*/
/* MD SequeceNumber */
UINT32	sequenceNumber = 0;					/* Sequence Number */
UINT32 sessionCounter = 0;					/* session Counter for sessionRef Value */

UINT32 listenerCounter = 0;					/* Add Listener Counter */
UINT32 *pAddListenerReference = NULL;		/* Add Listener Reference for tlm_adddListener() pUserRef (call back pMsg->pUserRef) */

INT32     rv = 0;

/* TAULpdMainThread */
VOS_THREAD_T taulPdMainThreadHandle = NULL;				/* Thread handle */
CHAR8 taulPdMainThreadName[] ="TAULpdMainThread";		/* Thread name is TAUL PD Main Thread. */
//BOOL8 taulPdMainThreadActiveFlag = FALSE;				/* TAUL PD Main Thread active/noactive Flag :active=TRUE, nonActive=FALSE */
//BOOL8 taulPdMainThreadStartFlag = FALSE;				/* TAUL PD Main Thread instruction start up Flag :start=TRUE, stop=FALSE */

/**********************************************************************************************************************/
/** TAUL Local Function */
/**********************************************************************************************************************/
/** Append an Publish Telegram at end of List
 *
 *  @param[in]      ppHeadPublishTelegram          pointer to pointer to head of List
 *  @param[in]      pNewPublishTelegram            pointer to publish telegram to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendPublishTelegramList (
		PUBLISH_TELEGRAM_T    * *ppHeadPublishTelegram,
		PUBLISH_TELEGRAM_T    *pNewPublishTelegram)
{
	PUBLISH_TELEGRAM_T *iterPublishTelegram;
	extern VOS_MUTEX_T pPublishTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Parameter Check */
    if (ppHeadPublishTelegram == NULL || pNewPublishTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First PublishTelegram ? */
    if (*ppHeadPublishTelegram == pNewPublishTelegram)
    {
    	return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewPublishTelegram->pNextPublishTelegram = NULL;

	/* Check Mutex ? */
    if (pPublishTelegramMutex == NULL)
    {
    	/* Create Publish Telegram Access Mutex */
    	vosErr = vos_mutexCreate(&pPublishTelegramMutex);
    	if (vosErr != VOS_NO_ERR)
    	{
    		vos_printLog(VOS_LOG_ERROR, "Create Publish Telegram  Mutex Err\n");
    		return TRDP_MUTEX_ERR;
    	}
    	else
    	{
    		/* Lock Publish Telegram by Mutex */
    		vosErr = vos_mutexLock(pPublishTelegramMutex);
    		if (vosErr != VOS_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "Publish Telegram Mutex Lock failed\n");
    			return TRDP_MUTEX_ERR;
    		}
    	}
    }
    else
    {
		/* Lock Publish Telegram by Mutex */
		vosErr = vos_mutexLock(pPublishTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Publish Telegram Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    if (*ppHeadPublishTelegram == NULL)
    {
        *ppHeadPublishTelegram = pNewPublishTelegram;
        /* UnLock Publish Telegram by Mutex */
        vos_mutexUnlock(pPublishTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterPublishTelegram = *ppHeadPublishTelegram;
    		iterPublishTelegram->pNextPublishTelegram != NULL;
    		iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram)
    {
        ;
    }
    iterPublishTelegram->pNextPublishTelegram = pNewPublishTelegram;
    /* UnLock Publish Telegram by Mutex */
    vos_mutexUnlock(pPublishTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Publish Telegram List
 *
 *  @param[in]      ppHeadPublishTelegram			pointer to pointer to head of queue
 *  @param[in]      pDeletePublishTelegram		pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 *
 */
TRDP_ERR_T deletePublishTelegramList (
		PUBLISH_TELEGRAM_T    * *ppHeadPublishTelegram,
		PUBLISH_TELEGRAM_T    *pDeletePublishTelegram)
{
	PUBLISH_TELEGRAM_T *iterPublishTelegram;
	extern VOS_MUTEX_T pPublishTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadPublishTelegram == NULL || *ppHeadPublishTelegram == NULL || pDeletePublishTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

	/* Check Mutex ? */
    if (pPublishTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing Publish Telegram Mutex Err\n");
    	return TRDP_MUTEX_ERR;
    }
    else
    {
		/* Lock Publish Telegram by Mutex */
    	vosErr = vos_mutexLock(pPublishTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Publish Telegram Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    /* handle removal of first element */
    if (pDeletePublishTelegram == *ppHeadPublishTelegram)
    {
        *ppHeadPublishTelegram = pDeletePublishTelegram->pNextPublishTelegram;
        vos_memFree(pDeletePublishTelegram);
        pDeletePublishTelegram = NULL;
        /* UnLock Publish Telegram by Mutex */
        vos_mutexUnlock(pPublishTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterPublishTelegram = *ppHeadPublishTelegram;
    		iterPublishTelegram != NULL;
    		iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram)
    {
        if (iterPublishTelegram->pNextPublishTelegram == pDeletePublishTelegram)
        {
        	iterPublishTelegram->pNextPublishTelegram = pDeletePublishTelegram->pNextPublishTelegram;
        	vos_memFree(pDeletePublishTelegram);
        	pDeletePublishTelegram = NULL;
        	break;
        }
    }
    /* UnLock Publish Telegram by Mutex */
    vos_mutexUnlock(pPublishTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the PublishTelegram with same comId and IP addresses
 *
 *  @param[in]		pHeadPublishTelegram		pointer to head of queue
 *  @param[in]		comId						Publish comId
 *  @param[in]		srcIpAddr					source IP Address
 *  @param[in]		dstIpAddr            	destination IP Address
 *
 *  @retval         != NULL						pointer to PublishTelegram
 *  @retval         NULL							No PublishTelegram found
 */
PUBLISH_TELEGRAM_T *searchPublishTelegramList (
		PUBLISH_TELEGRAM_T	*pHeadPublishTelegram,
		UINT32					comId,
		TRDP_IP_ADDR_T		srcIpAddr,
		TRDP_IP_ADDR_T		dstIpAddr)
{
	PUBLISH_TELEGRAM_T *iterPublishTelegram;
	extern VOS_MUTEX_T pPublishTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Check Parameter */
    if (pHeadPublishTelegram == NULL
    	|| comId == 0
    	|| dstIpAddr == 0)
    {
        return NULL;
    }

	/* Check Mutex ? */
    if (pPublishTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing Publish Telegram Mutex Err\n");
    	return NULL;
    }
    else
    {
		/* Lock Publish Telegram by Mutex */
    	vosErr = vos_mutexLock(pPublishTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Publish Telegram Mutex Lock failed\n");
			return NULL;
		}
    }

    /* Check PublishTelegram List Loop */
    for (iterPublishTelegram = pHeadPublishTelegram;
    		iterPublishTelegram != NULL;
    		iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram)
    {
        /* Publish Telegram: We match if src/dst address is zero or matches, and comId */
    	if ((iterPublishTelegram->comId == comId)
    		&& (iterPublishTelegram->srcIpAddr == 0 || iterPublishTelegram->srcIpAddr == srcIpAddr)
    		&& (iterPublishTelegram->dstIpAddr == 0 || iterPublishTelegram->dstIpAddr == dstIpAddr))
        {
    		/* UnLock Publish Telegram by Mutex */
    		vos_mutexUnlock(pPublishTelegramMutex);
    		return iterPublishTelegram;
        }
    	else
        {
        	continue;
        }
    }
    /* UnLock Publish Telegram by Mutex */
    vos_mutexUnlock(pPublishTelegramMutex);
    return NULL;
}

/**********************************************************************************************************************/
/** Append an Subscribe Telegram at end of List
 *
 *  @param[in]      ppHeadSubscribeTelegram          pointer to pointer to head of List
 *  @param[in]      pNewSubscribeTelegram            pointer to Subscribe telegram to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendSubscribeTelegramList (
		SUBSCRIBE_TELEGRAM_T    * *ppHeadSubscribeTelegram,
		SUBSCRIBE_TELEGRAM_T    *pNewSubscribeTelegram)
{
	SUBSCRIBE_TELEGRAM_T *iterSubscribeTelegram;
	extern VOS_MUTEX_T pSubscribeTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Parameter Check */
    if (ppHeadSubscribeTelegram == NULL || pNewSubscribeTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First Subscribe Telegram ? */
    if (*ppHeadSubscribeTelegram == pNewSubscribeTelegram)
    {
    	return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewSubscribeTelegram->pNextSubscribeTelegram = NULL;

	/* Check Mutex ? */
    if (pSubscribeTelegramMutex == NULL)
    {
    	/* Create Subscribe Telegram Access Mutex */
    	vosErr = vos_mutexCreate(&pSubscribeTelegramMutex);
    	if (vosErr != VOS_NO_ERR)
    	{
    		vos_printLog(VOS_LOG_ERROR, "Create Subscribe Telegram Mutex Err\n");
    		return TRDP_MUTEX_ERR;
    	}
    	else
    	{
    		/* Lock Subscribe Telegram by Mutex */
        	vosErr = vos_mutexLock(pSubscribeTelegramMutex);
    		if (vosErr != VOS_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "Subscribe Telegram Mutex Lock failed\n");
    			return TRDP_MUTEX_ERR;
    		}
    	}
    }
    else
    {
		/* Lock Subscribe Telegram by Mutex */
    	vosErr = vos_mutexLock(pSubscribeTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subscribe Telegram Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    if (*ppHeadSubscribeTelegram == NULL)
    {
        *ppHeadSubscribeTelegram = pNewSubscribeTelegram;
        /* UnLock Subscribe Telegram by Mutex */
        vos_mutexUnlock(pSubscribeTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterSubscribeTelegram = *ppHeadSubscribeTelegram;
    		iterSubscribeTelegram->pNextSubscribeTelegram != NULL;
    		iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram)
    {
        ;
    }
    iterSubscribeTelegram->pNextSubscribeTelegram = pNewSubscribeTelegram;
    /* UnLock Subscribe Telegram by Mutex */
    vos_mutexUnlock(pSubscribeTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Subscribe Telegram List
 *
 *  @param[in]      ppHeadSubscribeTelegram			pointer to pointer to head of queue
 *  @param[in]      pDeleteSubscribeTelegram			pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 *
 */
TRDP_ERR_T deleteSubscribeTelegramList (
		SUBSCRIBE_TELEGRAM_T    * *ppHeadSubscribeTelegram,
		SUBSCRIBE_TELEGRAM_T    *pDeleteSubscribeTelegram)
{
	SUBSCRIBE_TELEGRAM_T *iterSubscribeTelegram;
	extern VOS_MUTEX_T pSubscribeTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadSubscribeTelegram == NULL || *ppHeadSubscribeTelegram == NULL || pDeleteSubscribeTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

	/* Check Mutex ? */
    if (pSubscribeTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing Subscribe Telegram Mutex Err\n");
    	return TRDP_MUTEX_ERR;
    }
    else
    {
		/* Lock Subscribe Telegram by Mutex */
    	vosErr = vos_mutexLock(pSubscribeTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subscribe Telegram Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    /* handle removal of first element */
    if (pDeleteSubscribeTelegram == *ppHeadSubscribeTelegram)
    {
        *ppHeadSubscribeTelegram = pDeleteSubscribeTelegram->pNextSubscribeTelegram;
        vos_memFree(pDeleteSubscribeTelegram);
        pDeleteSubscribeTelegram = NULL;
        /* UnLock Subscribe Telegram by Mutex */
        vos_mutexUnlock(pSubscribeTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterSubscribeTelegram = *ppHeadSubscribeTelegram;
    		iterSubscribeTelegram != NULL;
    		iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram)
    {
        if (iterSubscribeTelegram->pNextSubscribeTelegram == pDeleteSubscribeTelegram)
        {
        	iterSubscribeTelegram->pNextSubscribeTelegram = pDeleteSubscribeTelegram->pNextSubscribeTelegram;
        	vos_memFree(pDeleteSubscribeTelegram);
        	pDeleteSubscribeTelegram = NULL;
        	break;
        }
    }
    /* UnLock Subscribe Telegram by Mutex */
    vos_mutexUnlock(pSubscribeTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the SubscribeTelegram with same comId and IP addresses
 *
 *  @param[in]      	pHeadSubscribeTelegram	pointer to head of queue
 *  @param[in]		comId						Subscribe comId
 *  @param[in]		srcIpAddr					source IP Address
 *  @param[in]		dstIpAddr            	destination IP Address
 *
 *  @retval         != NULL						pointer to Subscribe Telegram
 *  @retval         NULL							No Subscribe Telegram found
 */
SUBSCRIBE_TELEGRAM_T *searchSubscribeTelegramList (
		SUBSCRIBE_TELEGRAM_T		*pHeadSubscribeTelegram,
		UINT32						comId,
		TRDP_IP_ADDR_T			srcIpAddr,
		TRDP_IP_ADDR_T			dstIpAddr)
{
	SUBSCRIBE_TELEGRAM_T *iterSubscribeTelegram;
	extern VOS_MUTEX_T pSubscribeTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Check Parameter */
    if (pHeadSubscribeTelegram == NULL
    	|| comId == 0
    	|| dstIpAddr == 0)
    {
        return NULL;
    }
	/* Check Mutex ? */
    if (pSubscribeTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing Subscribe Telegram Mutex Err\n");
    	return NULL;
    }
    else
    {
		/* Lock Subscribe Telegram by Mutex */
    	vosErr = vos_mutexLock(pSubscribeTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subscribe Telegram Mutex Lock failed\n");
			return NULL;
		}
    }
    /* Check Subscribe Telegram List Loop */
    for (iterSubscribeTelegram = pHeadSubscribeTelegram;
    		iterSubscribeTelegram != NULL;
    		iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram)
    {
        /* Subscribe Telegram: We match if src/dst address is zero or matches, and comId */
    	if ((iterSubscribeTelegram->comId == comId)
    		&& ((iterSubscribeTelegram->srcIpAddr1 == 0 || iterSubscribeTelegram->srcIpAddr1 == srcIpAddr)
    			|| (iterSubscribeTelegram->srcIpAddr1 == 0 || iterSubscribeTelegram->srcIpAddr1 == srcIpAddr))
    		&& (iterSubscribeTelegram->dstIpAddr == 0 || iterSubscribeTelegram->dstIpAddr == dstIpAddr))
        {
    		/* UnLock Subscribe Telegram by Mutex */
    		vos_mutexUnlock(pSubscribeTelegramMutex);
    		return iterSubscribeTelegram;
        }
    	else
        {
        	continue;
        }
    }
	/* UnLock Subscribe Telegram by Mutex */
	vos_mutexUnlock(pSubscribeTelegramMutex);
    return NULL;
}

/**********************************************************************************************************************/
/** Return the SubscribeTelegram with end of List
 *
 *  @retval         != NULL						pointer to Subscribe Telegram
 *  @retval         NULL							No Subscribe Telegram found
 */
SUBSCRIBE_TELEGRAM_T *getTailSubscribeTelegram ()
{
	SUBSCRIBE_TELEGRAM_T *iterSubscribeTelegram;
	extern VOS_MUTEX_T pSubscribeTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Check Parameter */
    if (pHeadSubscribeTelegram == NULL)
    {
        return NULL;
    }
	/* Check Mutex ? */
    if (pSubscribeTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing Subscribe Telegram Mutex Err\n");
    	return NULL;
    }
    else
    {
		/* Lock Subscribe Telegram by Mutex */
    	vosErr = vos_mutexLock(pSubscribeTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subscribe Telegram Mutex Lock failed\n");
			return NULL;
		}
    }
    /* Check Subscribe Telegram List Loop */
    for (iterSubscribeTelegram = pHeadSubscribeTelegram;
    		iterSubscribeTelegram->pNextSubscribeTelegram != NULL;
    		iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram)
    {
      	continue;
    }
	/* UnLock Subscribe Telegram by Mutex */
	vos_mutexUnlock(pSubscribeTelegramMutex);
    return iterSubscribeTelegram;
}

/**********************************************************************************************************************/
/** Append an PD Request Telegram at end of List
 *
 *  @param[in]      ppHeadPdRequestTelegram          pointer to pointer to head of List
 *  @param[in]      pNewPdRequestTelegram            pointer to Pd Request telegram to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendPdRequestTelegramList (
		PD_REQUEST_TELEGRAM_T    * *ppHeadPdRequestTelegram,
		PD_REQUEST_TELEGRAM_T    *pNewPdRequestTelegram)
{
	PD_REQUEST_TELEGRAM_T *iterPdRequestTelegram;
	extern VOS_MUTEX_T pPdRequestTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Parameter Check */
    if (ppHeadPdRequestTelegram == NULL || pNewPdRequestTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First PdRequest Telegram ? */
    if (*ppHeadPdRequestTelegram == pNewPdRequestTelegram)
    {
     	return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewPdRequestTelegram->pNextPdRequestTelegram = NULL;

	/* Check Mutex ? */
    if (pPdRequestTelegramMutex == NULL)
    {
    	/* Create PD Request Telegram Access Mutex */
    	vosErr = vos_mutexCreate(&pPdRequestTelegramMutex);
    	if (vosErr != VOS_NO_ERR)
    	{
    		vos_printLog(VOS_LOG_ERROR, "Create PD Request Telegram Mutex Err\n");
    		return TRDP_MUTEX_ERR;
    	}
    	else
    	{
    		/* Lock PD Request Telegram by Mutex */
        	vosErr = vos_mutexLock(pPdRequestTelegramMutex);
    		if (vosErr != VOS_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "PD Request Telegram Mutex Lock failed\n");
    			return TRDP_MUTEX_ERR;
    		}
    	}
    }
    else
    {
		/* Lock PD Request Telegram by Mutex */
    	vosErr = vos_mutexLock(pPdRequestTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "PD Request Telegram Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    if (*ppHeadPdRequestTelegram == NULL)
    {
		*ppHeadPdRequestTelegram = pNewPdRequestTelegram;
		/* UnLock PD Request Telegram by Mutex */
		vos_mutexUnlock(pPdRequestTelegramMutex);
		return TRDP_NO_ERR;
    }

    for (iterPdRequestTelegram = *ppHeadPdRequestTelegram;
    		iterPdRequestTelegram->pNextPdRequestTelegram != NULL;
    		iterPdRequestTelegram = iterPdRequestTelegram->pNextPdRequestTelegram)
    {
        ;
    }
    iterPdRequestTelegram->pNextPdRequestTelegram = pNewPdRequestTelegram;
   	/* UnLock PD Request Telegram by Mutex */
    vos_mutexUnlock(pPdRequestTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an PD Request Telegram List
 *
 *  @param[in]      ppHeadPdRequestTelegram			pointer to pointer to head of queue
 *  @param[in]      pDeletePdRequestTelegram			pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 *
 */
TRDP_ERR_T deletePdRequestTelegramList (
		PD_REQUEST_TELEGRAM_T    * *ppHeadPdRequestTelegram,
		PD_REQUEST_TELEGRAM_T    *pDeletePdRequestTelegram)
{
	PD_REQUEST_TELEGRAM_T *iterPdRequestTelegram;
	extern VOS_MUTEX_T pPdRequestTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadPdRequestTelegram == NULL || *ppHeadPdRequestTelegram == NULL || pDeletePdRequestTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

	/* Check Mutex ? */
    if (pPdRequestTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing PD Request Telegram Mutex Err\n");
    	return TRDP_MUTEX_ERR;
    }
    else
    {
		/* Lock PD Request Telegram by Mutex */
    	vosErr = vos_mutexLock(pPdRequestTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "PD Request Telegram Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    /* handle removal of first element */
    if (pDeletePdRequestTelegram == *ppHeadPdRequestTelegram)
    {
        *ppHeadPdRequestTelegram = pDeletePdRequestTelegram->pNextPdRequestTelegram;
        vos_memFree(pDeletePdRequestTelegram);
        pDeletePdRequestTelegram = NULL;
    	/* UnLock PD Request Telegram by Mutex */
    	vos_mutexUnlock(pPdRequestTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterPdRequestTelegram = *ppHeadPdRequestTelegram;
    		iterPdRequestTelegram != NULL;
    		iterPdRequestTelegram = iterPdRequestTelegram->pNextPdRequestTelegram)
    {
        if (iterPdRequestTelegram->pNextPdRequestTelegram == pDeletePdRequestTelegram)
        {
        	iterPdRequestTelegram->pNextPdRequestTelegram = pDeletePdRequestTelegram->pNextPdRequestTelegram;
        	vos_memFree(pDeletePdRequestTelegram);
        	pDeletePdRequestTelegram = NULL;
        	break;
        }
    }
	/* UnLock PD Request Telegram by Mutex */
	vos_mutexUnlock(pPdRequestTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the PD Request with same comId and IP addresses
 *
 *  @param[in]      	pHeadPdRequestTelegram	pointer to head of queue
 *  @param[in]		comId						PD Request comId
 *  @param[in]		replyComId					PD Reply comId
 *  @param[in]		srcIpAddr					source IP Address
 *  @param[in]		dstIpAddr            	destination IP Address
 *  @param[in]		replyIpAddr				return reply IP Address
 *
 *  @retval         != NULL						pointer to PD Request Telegram
 *  @retval         NULL							No PD Requset Telegram found
 */
PD_REQUEST_TELEGRAM_T *searchPdRequestTelegramList (
		PD_REQUEST_TELEGRAM_T	*pHeadPdRequestTelegram,
		UINT32						comId,
		UINT32						replyComId,
		TRDP_IP_ADDR_T			srcIpAddr,
		TRDP_IP_ADDR_T			dstIpAddr,
		TRDP_IP_ADDR_T			replyIpAddr)
{
	PD_REQUEST_TELEGRAM_T *iterPdRequestTelegram;
	extern VOS_MUTEX_T pPdRequestTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Check Parameter */
    if (pHeadPdRequestTelegram == NULL
    	|| comId == 0
    	|| dstIpAddr == 0
    	|| replyIpAddr == 0)
    {
        return NULL;
    }
	/* Check Mutex ? */
    if (pPdRequestTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing PD Request Telegram Mutex Err\n");
    	return NULL;
    }
    else
    {
		/* Lock PD Request Telegram by Mutex */
    	vosErr = vos_mutexLock(pPdRequestTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "PD Request Telegram Mutex Lock failed\n");
			return NULL;
		}
    }
    /* Check PD Request Telegram List Loop */
    for (iterPdRequestTelegram = pHeadPdRequestTelegram;
    		iterPdRequestTelegram != NULL;
    		iterPdRequestTelegram = iterPdRequestTelegram->pNextPdRequestTelegram)
    {
        /* PD Request Telegram: We match if src/dst address is zero or matches, and comId */
    	if ((iterPdRequestTelegram->comId == comId)
    		&& (iterPdRequestTelegram->replyComId == 0 || iterPdRequestTelegram->replyComId == replyComId)
    		&& (iterPdRequestTelegram->srcIpAddr == 0 || iterPdRequestTelegram->srcIpAddr == srcIpAddr)
    		&& (iterPdRequestTelegram->dstIpAddr == 0 || iterPdRequestTelegram->dstIpAddr == dstIpAddr)
    		&& (iterPdRequestTelegram->replyIpAddr == 0 || iterPdRequestTelegram->replyIpAddr == replyIpAddr))
        {
    		/* UnLock PD Request Telegram by Mutex */
    		vos_mutexUnlock(pPdRequestTelegramMutex);
    		return iterPdRequestTelegram;
        }
    	else
        {
        	continue;
        }
    }
	/* UnLock PD Request Telegram by Mutex */
	vos_mutexUnlock(pPdRequestTelegramMutex);
    return NULL;
}

/**********************************************************************************************************************/
/** Append an Caller Telegram at end of List
 *
 *  @param[in]      ppHeadCallerTelegram          pointer to pointer to head of List
 *  @param[in]      pNewCallerTelegram            pointer to Caller telegram to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendCallerTelegramList (
		CALLER_TELEGRAM_T    * *ppHeadCallerTelegram,
		CALLER_TELEGRAM_T    *pNewCallerTelegram)
{
	CALLER_TELEGRAM_T *iterCallerTelegram;
	extern VOS_MUTEX_T pCallerTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Parameter Check */
    if (ppHeadCallerTelegram == NULL || pNewCallerTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First Caller Telegram ? */
    if (*ppHeadCallerTelegram == pNewCallerTelegram)
    {
    	return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewCallerTelegram->pNextCallerTelegram = NULL;

    /* Check Mutex ? */
	if (pCallerTelegramMutex == NULL)
	{
		/* Create Caller Telegram Access Mutex */
		vosErr = vos_mutexCreate(&pCallerTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Create Caller Telegram Mutex Err\n");
			return TRDP_MUTEX_ERR;
		}
		else
		{
			/* Lock Caller Telegram by Mutex */
			vosErr = vos_mutexLock(pCallerTelegramMutex);
			if (vosErr != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Caller Telegram Mutex Lock failed\n");
				return TRDP_MUTEX_ERR;
			}
		}
	}
	else
	{
		/* Lock Caller Telegram by Mutex */
		vosErr = vos_mutexLock(pCallerTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Caller Telegram Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
	}

    if (*ppHeadCallerTelegram == NULL)
    {
		*ppHeadCallerTelegram = pNewCallerTelegram;
		/* UnLock Caller Telegram by Mutex */
		vos_mutexUnlock(pCallerTelegramMutex);
		return TRDP_NO_ERR;
    }

    for (iterCallerTelegram = *ppHeadCallerTelegram;
    		iterCallerTelegram->pNextCallerTelegram != NULL;
    		iterCallerTelegram = iterCallerTelegram->pNextCallerTelegram)
    {
        ;
    }
    iterCallerTelegram->pNextCallerTelegram = pNewCallerTelegram;
	/* UnLock Caller Telegram by Mutex */
	vos_mutexUnlock(pCallerTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Caller Telegram List
 *
 *  @param[in]      ppHeadCallerTelegram			pointer to pointer to head of queue
 *  @param[in]      pDeleteCallerTelegram			pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 */
TRDP_ERR_T deleteCallerTelegramList (
		CALLER_TELEGRAM_T    * *ppHeadCallerTelegram,
		CALLER_TELEGRAM_T    *pDeleteCallerTelegram)
{
	CALLER_TELEGRAM_T *iterCallerTelegram;
	extern VOS_MUTEX_T pCallerTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadCallerTelegram == NULL || *ppHeadCallerTelegram == NULL || pDeleteCallerTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }
	/* Check Mutex ? */
    if (pCallerTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing Caller Telegram Mutex Err\n");
    	return TRDP_MUTEX_ERR;
    }
    else
    {
		/* Lock Caller Telegram by Mutex */
    	vosErr = vos_mutexLock(pCallerTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Caller Telegram Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }
    /* handle removal of first element */
    if (pDeleteCallerTelegram == *ppHeadCallerTelegram)
    {
		*ppHeadCallerTelegram = pDeleteCallerTelegram->pNextCallerTelegram;
		vos_memFree(pDeleteCallerTelegram);
		pDeleteCallerTelegram = NULL;
		/* UnLock Caller Telegram by Mutex */
		vos_mutexUnlock(pCallerTelegramMutex);
		return TRDP_NO_ERR;
    }

    for (iterCallerTelegram = *ppHeadCallerTelegram;
    		iterCallerTelegram != NULL;
    		iterCallerTelegram = iterCallerTelegram->pNextCallerTelegram)
    {
        if (iterCallerTelegram->pNextCallerTelegram == pDeleteCallerTelegram)
        {
        	iterCallerTelegram->pNextCallerTelegram = pDeleteCallerTelegram->pNextCallerTelegram;
        	vos_memFree(pDeleteCallerTelegram);
        	pDeleteCallerTelegram = NULL;
        	break;
        }
    }
	/* UnLock Caller Telegram by Mutex */
	vos_mutexUnlock(pCallerTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the Caller with same comId and IP addresses
 *
 *  @param[in]      	pHeadCallerTelegram		pointer to head of queue
 *  @param[in]		comId						Caller comId (Mn/Mr, Mp/Mq, Mc)
 *  @param[in]		srcIpAddr					source IP Address
 *  @param[in]		dstIpAddr            	destination IP Address
 *  @param[in]		getNextTelegramFlag		TRUE:get Next Telegram, FALSE:get hit Telegram
 *
 *  @retval         != NULL						pointer to Caller Telegram
 *  @retval         NULL							No Caller Telegram found
 */
CALLER_TELEGRAM_T *searchCallerTelegramList (
		CALLER_TELEGRAM_T			*pHeadCallerTelegram,
		UINT32						comId,
		TRDP_IP_ADDR_T			srcIpAddr,
		TRDP_IP_ADDR_T			dstIpAddr,
		BOOL8						getNextTelegramFlag)
{
	CALLER_TELEGRAM_T *iterCallerTelegram;
	TRDP_IP_ADDR_T		networkByteSrcIpAddr = 0;				/* for convert URI to Source IP Address */
	TRDP_IP_ADDR_T		networkByteDstIpAddr = 0;				/* for convert URI to Destination IP Address */
	extern VOS_MUTEX_T pCallerTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Check Parameter */
    if (pHeadCallerTelegram == NULL
    	|| comId == 0)
//    	|| dstIpAddr == 0)
    {
        return NULL;
    }
	/* Check Mutex ? */
    if (pCallerTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing Caller Telegram Mutex Err\n");
    	return NULL;
    }
    else
    {
		/* Lock Caller Telegram by Mutex */
		vosErr = vos_mutexLock(pCallerTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Caller Telegram Mutex Lock failed\n");
			return NULL;
		}
    }
    /* Check Caller Telegram List Loop */
    for (iterCallerTelegram = pHeadCallerTelegram;
    		iterCallerTelegram != NULL;
    		iterCallerTelegram = iterCallerTelegram->pNextCallerTelegram)
    {
    	/*  Convert host URI to IP address  */
    	/* Convert Sorcue */
    	if (iterCallerTelegram->pSource != NULL)
    	{
    		if (iterCallerTelegram->pSource->pUriHost1 != NULL)
    		{
    			networkByteSrcIpAddr = vos_dottedIP (*(iterCallerTelegram->pSource->pUriHost1 ));
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
    	if (iterCallerTelegram->pDestination != NULL)
    	{
    		if (iterCallerTelegram->pDestination->pUriHost != NULL)
    		{
    			networkByteDstIpAddr = vos_dottedIP (*(iterCallerTelegram->pDestination->pUriHost));
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
        /* Caller Telegram: We match if src/dst address is zero or matches, and comId */
    	if ((iterCallerTelegram->comId == comId)
    		&& (networkByteSrcIpAddr == 0 || networkByteSrcIpAddr == srcIpAddr)
    		&& (networkByteDstIpAddr == 0 || networkByteDstIpAddr == dstIpAddr))
        {
    		/* Check get Telegram Flag */
    		if (getNextTelegramFlag == FALSE)
    		{
    			/* UnLock Caller Telegram by Mutex */
    			vos_mutexUnlock(pCallerTelegramMutex);
    			/* Return Agreement Telegram */
    			return iterCallerTelegram;
    		}
    		else
    		{
    			/* Check Next Telegram enable ? */
    			if (iterCallerTelegram->pNextCallerTelegram != NULL)
    			{
    				/* UnLock Caller Telegram by Mutex */
    				vos_mutexUnlock(pCallerTelegramMutex);
    				/* Return Next Agreement Telegram */
    				return iterCallerTelegram->pNextCallerTelegram ;
    			}
    			else
    			{
    				/* UnLock Caller Telegram by Mutex */
    				vos_mutexUnlock(pCallerTelegramMutex);
    				/* Return Error */
    				return NULL;
    			}
    		}
        }
    	else
        {
        	continue;
        }
    }
	/* UnLock Caller Telegram by Mutex */
	vos_mutexUnlock(pCallerTelegramMutex);
	return NULL;
}

/**********************************************************************************************************************/
/** Append an Replier Telegram at end of List
 *
 *  @param[in]      ppHeadReplierTelegram          pointer to pointer to head of List
 *  @param[in]      pNewReplierTelegram            pointer to Replier telegram to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendReplierTelegramList (
		REPLIER_TELEGRAM_T    * *ppHeadReplierTelegram,
		REPLIER_TELEGRAM_T    *pNewReplierTelegram)
{
	REPLIER_TELEGRAM_T *iterReplierTelegram;
	extern VOS_MUTEX_T pReplierTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Parameter Check */
    if (ppHeadReplierTelegram == NULL || pNewReplierTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First Replier Telegram ? */
    if (*ppHeadReplierTelegram == pNewReplierTelegram)
    {
    	return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewReplierTelegram->pNextReplierTelegram = NULL;

	/* Check Mutex ? */
    if (pReplierTelegramMutex == NULL)
    {
    	/* Create Replier Telegram Access Mutex */
    	vosErr = vos_mutexCreate(&pReplierTelegramMutex);
    	if (vosErr != VOS_NO_ERR)
    	{
    		vos_printLog(VOS_LOG_ERROR, "Create Replier Telegram Mutex Err\n");
    		return TRDP_MUTEX_ERR;
    	}
    	else
    	{
    		/* Lock Replier Telegram by Mutex */
        	vosErr = vos_mutexLock(pReplierTelegramMutex);
    		if (vosErr != VOS_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "Replier Telegram Mutex Lock failed\n");
    			return TRDP_MUTEX_ERR;
    		}
    	}
    }
    else
    {
		/* Lock Replier Telegram by Mutex */
    	vosErr = vos_mutexLock(pReplierTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Replier Telegram Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    if (*ppHeadReplierTelegram == NULL)
    {
		*ppHeadReplierTelegram = pNewReplierTelegram;
		/* UnLock Replier Telegram by Mutex */
		vos_mutexUnlock(pReplierTelegramMutex);
		return TRDP_NO_ERR;
    }

    for (iterReplierTelegram = *ppHeadReplierTelegram;
    		iterReplierTelegram->pNextReplierTelegram != NULL;
    		iterReplierTelegram = iterReplierTelegram->pNextReplierTelegram)
    {
        ;
    }
	iterReplierTelegram->pNextReplierTelegram = pNewReplierTelegram;
	/* UnLock Replier Telegram by Mutex */
	vos_mutexUnlock(pReplierTelegramMutex);
	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Replier Telegram List
 *
 *  @param[in]      ppHeadReplierTelegram			pointer to pointer to head of queue
 *  @param[in]      pDeleteReplierTelegram		pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 *PD_REQUEST
 */
TRDP_ERR_T deleteReplierTelegramList (
		REPLIER_TELEGRAM_T    * *ppHeadReplierTelegram,
		REPLIER_TELEGRAM_T    *pDeleteReplierTelegram)
{
	REPLIER_TELEGRAM_T *iterReplierTelegram;
	extern VOS_MUTEX_T pReplierTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadReplierTelegram == NULL || *ppHeadReplierTelegram == NULL || pDeleteReplierTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

	/* Check Mutex ? */
    if (pReplierTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing Replier Telegram Mutex Err\n");
    	return TRDP_MUTEX_ERR;
    }
    else
    {
		/* Lock Replier Telegram by Mutex */
    	vosErr = vos_mutexLock(pReplierTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Replier Telegram Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    /* handle removal of first element */
    if (pDeleteReplierTelegram == *ppHeadReplierTelegram)
    {
		*ppHeadReplierTelegram = pDeleteReplierTelegram->pNextReplierTelegram;
		vos_memFree(pDeleteReplierTelegram);
		pDeleteReplierTelegram = NULL;
		/* UnLock Replier Telegram by Mutex */
		vos_mutexUnlock(pReplierTelegramMutex);
		return TRDP_NO_ERR;
    }

    for (iterReplierTelegram = *ppHeadReplierTelegram;
    		iterReplierTelegram != NULL;
    		iterReplierTelegram = iterReplierTelegram->pNextReplierTelegram)
    {
        if (iterReplierTelegram->pNextReplierTelegram == pDeleteReplierTelegram)
        {
        	iterReplierTelegram->pNextReplierTelegram = pDeleteReplierTelegram->pNextReplierTelegram;
        	vos_memFree(pDeleteReplierTelegram);
        	pDeleteReplierTelegram = NULL;
        	break;
        }
    }
	/* UnLock Replier Telegram by Mutex */
	vos_mutexUnlock(pReplierTelegramMutex);
	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the Replier with same comId and IP addresses
 *
 *  @param[in]      	pHeadReplierTelegram	pointer to head of queue
 *  @param[in]		comId						Replier comId (Mn/Mr, Mp/Mq, Mc)
 *  @param[in]		srcIpAddr					source IP Address
 *  @param[in]		dstIpAddr            	destination IP Address
 *  @param[in]		getNextTelegramFlag		TRUE:get Next Telegram, FALSE:get hit Telegram
 *
 *  @retval         != NULL						pointer to Replier Telegram
 *  @retval         NULL							No Replier Telegram found
 */
REPLIER_TELEGRAM_T *searchReplierTelegramList (
		REPLIER_TELEGRAM_T		*pHeadReplierTelegram,
		UINT32						comId,
		TRDP_IP_ADDR_T			srcIpAddr,
		TRDP_IP_ADDR_T			dstIpAddr,
		BOOL8						getNextTelegramFlag)
{
	REPLIER_TELEGRAM_T *iterReplierTelegram;
	TRDP_IP_ADDR_T		networkByteSrcIpAddr = 0;				/* for convert URI to Source IP Address */
	TRDP_IP_ADDR_T		networkByteDstIpAddr = 0;				/* for convert URI to Destination IP Address */
	extern VOS_MUTEX_T pReplierTelegramMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Check Parameter */
    if (pHeadReplierTelegram == NULL
    	|| comId == 0)
    {
        return NULL;
    }

	/* Check Mutex ? */
    if (pReplierTelegramMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing Replier Telegram Mutex Err\n");
    	return NULL;
    }
    else
    {
		/* Lock Replier Telegram by Mutex */
    	vosErr = vos_mutexLock(pReplierTelegramMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Replier Telegram Mutex Lock failed\n");
			return NULL;
		}
    }

    /* Check Replier Telegram List Loop */
    for (iterReplierTelegram = pHeadReplierTelegram;
    		iterReplierTelegram != NULL;
    		iterReplierTelegram = iterReplierTelegram->pNextReplierTelegram)
    {
    	/*  Source IP Address Convert host URI to IP address  */
    	if (iterReplierTelegram->pSource != NULL)
        {
        	if (iterReplierTelegram->pSource->pUriHost1 != NULL)
            {
        		networkByteSrcIpAddr = vos_dottedIP (*(iterReplierTelegram->pSource->pUriHost1 ));
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
    	/*  Destination IP Address Convert host URI to IP address  */
    	if (iterReplierTelegram->pDestination != NULL)
    	{
    		if (iterReplierTelegram->pDestination->pUriHost != NULL)
    		{
    			networkByteDstIpAddr = vos_dottedIP (*(iterReplierTelegram->pDestination->pUriHost));
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

        /* Replier Telegram: We match if src/dst address is zero or matches, and comId */
    	if ((iterReplierTelegram->comId == comId)
        		&& ((networkByteSrcIpAddr == 0) || (networkByteSrcIpAddr == srcIpAddr))
        		&& ((networkByteDstIpAddr == 0) || (networkByteDstIpAddr == dstIpAddr) || (dstIpAddr == 0)))
    	{
    		/* Check get Telegram Flag */
    		if (getNextTelegramFlag == FALSE)
    		{
    			/* UnLock Replier Telegram by Mutex */
    			vos_mutexUnlock(pReplierTelegramMutex);
    			/* Return Agreement Telegram */
    			return iterReplierTelegram;
    		}
    		else
    		{
    			/* Check Next Telegram enable ? */
    			if (iterReplierTelegram->pNextReplierTelegram != NULL)
    			{
    				/* UnLock Replier Telegram by Mutex */
    				vos_mutexUnlock(pReplierTelegramMutex);
    				/* Return Next Agreement Telegram */
    				return iterReplierTelegram->pNextReplierTelegram ;
    			}
    			else
    			{
    				/* UnLock Replier Telegram by Mutex */
        			vos_mutexUnlock(pReplierTelegramMutex);
        			/* Return Error */
    				return NULL;
    			}
    		}
        }
    	else
        {
        	continue;
        }
    }
	/* UnLock Replier Telegram by Mutex */
	vos_mutexUnlock(pReplierTelegramMutex);
    return NULL;
}

/**********************************************************************************************************************/
/** Append an Waiting Receive Request Reference at end of List
 *
 *  @param[in]      ppHeadWaitingReceiveRequestReference          pointer to pointer to head of List
 *  @param[in]      pNewWaitingReceiveRequestReference            pointer to Waiting Receive Request Reference to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendWaitingReceiveRequestQueue (
		WAITING_RECEIVE_REQUEST_T    * *ppHeadWaitingReceiveRequestReference,
		WAITING_RECEIVE_REQUEST_T    *pNewWaitingReceiveRequestReference)
{
	WAITING_RECEIVE_REQUEST_T *iterWaitingReceiveRequestReference;
	extern VOS_MUTEX_T pWaitingReceiveRequestReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Parameter Check */
    if (ppHeadWaitingReceiveRequestReference == NULL || pNewWaitingReceiveRequestReference == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First WaitingReceiveRequest Reference ? */
    if (*ppHeadWaitingReceiveRequestReference == pNewWaitingReceiveRequestReference)
    {
    	return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewWaitingReceiveRequestReference->pNextWaitingReceiveRequestReference = NULL;

	/* Check Mutex ? */
    if (pWaitingReceiveRequestReferenceMutex == NULL)
    {
    	/* Create WaitingReceiveRequestReference Access Mutex */
    	vosErr = vos_mutexCreate(&pWaitingReceiveRequestReferenceMutex);
    	if (vosErr != VOS_NO_ERR)
    	{
    		vos_printLog(VOS_LOG_ERROR, "Create WaitingReceiveRequestReference Mutex Err\n");
    		return TRDP_MUTEX_ERR;
    	}
    	else
    	{
    		/* Lock WaitingReceiveRequestReference by Mutex */
        	vosErr = vos_mutexLock(pWaitingReceiveRequestReferenceMutex);
    		if (vosErr != VOS_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveReplyReference Mutex Lock failed\n");
    			return TRDP_MUTEX_ERR;
    		}
    	}
    }
    else
    {
		/* Lock WaitingReceiveRequestReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingReceiveRequestReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveReplyReference Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    if (*ppHeadWaitingReceiveRequestReference == NULL)
    {
		*ppHeadWaitingReceiveRequestReference = pNewWaitingReceiveRequestReference;
		/* UnLock WaitingReceiveRequestReference by Mutex */
		vos_mutexUnlock(pWaitingReceiveRequestReferenceMutex);
		return TRDP_NO_ERR;
    }

    for (iterWaitingReceiveRequestReference = *ppHeadWaitingReceiveRequestReference;
    		iterWaitingReceiveRequestReference->pNextWaitingReceiveRequestReference != NULL;
    		iterWaitingReceiveRequestReference = iterWaitingReceiveRequestReference->pNextWaitingReceiveRequestReference)
    {
        ;
    }
	iterWaitingReceiveRequestReference->pNextWaitingReceiveRequestReference = pNewWaitingReceiveRequestReference;
	/* UnLock WaitingReceiveRequestReference by Mutex */
	vos_mutexUnlock(pWaitingReceiveRequestReferenceMutex);
	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Waiting Receive Request Reference List
 *
 *  @param[in]      ppHeadWaitingReceiveRequestReference			pointer to pointer to head of queue
 *  @param[in]      pDeleteWaitingReceiveRequestReference			pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 *PD_REQUEST
 */
TRDP_ERR_T deleteWaitingReceiveRequestQueue (
		WAITING_RECEIVE_REQUEST_T    * *ppHeadWaitingReceiveRequestReference,
		WAITING_RECEIVE_REQUEST_T    *pDeleteWaitingReceiveRequestReference)
{
	WAITING_RECEIVE_REQUEST_T *iterWaitingReceiveRequestReference;
	extern VOS_MUTEX_T pWaitingReceiveRequestReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadWaitingReceiveRequestReference == NULL || *ppHeadWaitingReceiveRequestReference == NULL || pDeleteWaitingReceiveRequestReference == NULL)
    {
        return TRDP_PARAM_ERR;
    }
	/* Check Mutex ? */
    if (pWaitingReceiveRequestReferenceMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing WaitingReceiveRequestReference Mutex Err\n");
    	return TRDP_MUTEX_ERR;
    }
    else
    {
		/* Lock WaitingReceiveRequestyReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingReceiveRequestReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveRequestReference Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }
    /* handle removal of first element */
    if (pDeleteWaitingReceiveRequestReference == *ppHeadWaitingReceiveRequestReference)
    {
		*ppHeadWaitingReceiveRequestReference = pDeleteWaitingReceiveRequestReference->pNextWaitingReceiveRequestReference;
		vos_memFree(pDeleteWaitingReceiveRequestReference);
		pDeleteWaitingReceiveRequestReference = NULL;
		/* UnLock WaitingReceiveRequestReference by Mutex */
		vos_mutexUnlock(pWaitingReceiveRequestReferenceMutex);
		return TRDP_NO_ERR;
    }

    for (iterWaitingReceiveRequestReference = *ppHeadWaitingReceiveRequestReference;
    		iterWaitingReceiveRequestReference != NULL;
    		iterWaitingReceiveRequestReference = iterWaitingReceiveRequestReference->pNextWaitingReceiveRequestReference)
    {
        if (iterWaitingReceiveRequestReference->pNextWaitingReceiveRequestReference == pDeleteWaitingReceiveRequestReference)
        {
        	iterWaitingReceiveRequestReference->pNextWaitingReceiveRequestReference = pDeleteWaitingReceiveRequestReference->pNextWaitingReceiveRequestReference;
        	vos_memFree(pDeleteWaitingReceiveRequestReference);
        	pDeleteWaitingReceiveRequestReference = NULL;
        	break;
        }
    }
	/* UnLock WaitingReceiveRequestReference by Mutex */
	vos_mutexUnlock(pWaitingReceiveRequestReferenceMutex);
	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the WaitingReceiveRequest with same callerRef
 *
 *  @param[in]      	pHeadWaitingReceiveRequestReference	pointer to head of queue
 *  @param[in]		callerRef									pointer to caller Reference
 *
 *  @retval         != NULL						pointer to Waiting Receive Request Reference
 *  @retval         NULL							No Waiting Receive Request Reference found
 */
WAITING_RECEIVE_REQUEST_T *searchWaitingReceiveRequestReference (
		WAITING_RECEIVE_REQUEST_T		*pHeadWaitingReceiveRequestReference,
		void								*callerRef)
{
	WAITING_RECEIVE_REQUEST_T *iterWaitingReceiveRequestReference;
	extern VOS_MUTEX_T pWaitingReceiveRequestReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Check Parameter */
    if (pHeadWaitingReceiveRequestReference == NULL
    	|| callerRef == NULL)
    {
        return NULL;
    }

	/* Check Mutex ? */
    if (pWaitingReceiveRequestReferenceMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing WaitingReceiveRequestReference Mutex Err\n");
    	return NULL;
    }
    else
    {
		/* Lock WaitingReceiveRequestReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingReceiveRequestReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveRequestReference Mutex Lock failed\n");
			return NULL;
		}
    }
    /* Check WaitingReceiveRequest Reference Loop */
    for (iterWaitingReceiveRequestReference = pHeadWaitingReceiveRequestReference;
    		iterWaitingReceiveRequestReference != NULL;
    		iterWaitingReceiveRequestReference = iterWaitingReceiveRequestReference->pNextWaitingReceiveRequestReference)
    {
    	/* WaitingReceiveRequestReference: We match if src/dst address is zero or matches, and comId */
    	if (iterWaitingReceiveRequestReference->callerReference == callerRef)
        {
    		/* UnLock WaitingReceiveRequestReference by Mutex */
    		vos_mutexUnlock(pWaitingReceiveRequestReferenceMutex);
    		return iterWaitingReceiveRequestReference;
        }
    	else
        {
        	continue;
        }
    }
	/* UnLock WaitingReceiveRequestReference by Mutex */
	vos_mutexUnlock(pWaitingReceiveRequestReferenceMutex);
	return NULL;
}

/**********************************************************************************************************************/
/** Append an Waiting Send Reply Reference at end of List
 *
 *  @param[in]      ppHeadWaitingSendReplyReference          pointer to pointer to head of List
 *  @param[in]      pNewWaitingSendReplyReference            pointer to Waiting Send Reply Reference to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendWaitingSendReplyQueue (
		WAITING_SEND_REPLY_T    * *ppHeadWaitingSendReplyReference,
		WAITING_SEND_REPLY_T    *pNewWaitingSendReplyReference)
{
	WAITING_SEND_REPLY_T *iterWaitingSendReplyReference;
	extern VOS_MUTEX_T pWaitingSendReplyReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Parameter Check */
    if (ppHeadWaitingSendReplyReference == NULL || pNewWaitingSendReplyReference == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First WaitngSendReply Reference ? */
    if (*ppHeadWaitingSendReplyReference == pNewWaitingSendReplyReference)
    {
    	return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewWaitingSendReplyReference->pNextWaitingSendReplyReference = NULL;

	/* Check Mutex ? */
    if (pWaitingSendReplyReferenceMutex == NULL)
    {
    	/* Create WaitingSendReplyReference Access Mutex */
    	vosErr = vos_mutexCreate(&pWaitingSendReplyReferenceMutex);
    	if (vosErr != VOS_NO_ERR)
    	{
    		vos_printLog(VOS_LOG_ERROR, "Create WaitingSendReplyReference Mutex Err\n");
    		return TRDP_MUTEX_ERR;
    	}
    	else
    	{
    		/* Lock WaitingSendReplyReference by Mutex */
        	vosErr = vos_mutexLock(pWaitingSendReplyReferenceMutex);
    		if (vosErr != VOS_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "WaitingSendReplyReference Mutex Lock failed\n");
    			return TRDP_MUTEX_ERR;
    		}
    	}
    }
    else
    {
		/* Lock WaitingSendReplyReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingSendReplyReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingSendReplyReference Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    if (*ppHeadWaitingSendReplyReference == NULL)
    {
		*ppHeadWaitingSendReplyReference = pNewWaitingSendReplyReference;
		/* UnLock WaitingSendReplyReference by Mutex */
		vos_mutexUnlock(pWaitingSendReplyReferenceMutex);
		return TRDP_NO_ERR;
    }

    for (iterWaitingSendReplyReference = *ppHeadWaitingSendReplyReference;
    		iterWaitingSendReplyReference->pNextWaitingSendReplyReference != NULL;
    		iterWaitingSendReplyReference = iterWaitingSendReplyReference->pNextWaitingSendReplyReference)
    {
        ;
    }
	iterWaitingSendReplyReference->pNextWaitingSendReplyReference = pNewWaitingSendReplyReference;
	/* UnLock WaitingSendReplyReference by Mutex */
	vos_mutexUnlock(pWaitingSendReplyReferenceMutex);
	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Waiting Send Reply Reference List
 *
 *  @param[in]      ppHeadWaitingSendReplyReference			pointer to pointer to head of queue
 *  @param[in]      pDeleteWaitingSendReplyReference			pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 *PD_REQUEST
 */
TRDP_ERR_T deleteWaitingSendReplyQueue (
		WAITING_SEND_REPLY_T    * *ppHeadWaitingSendReplyReference,
		WAITING_SEND_REPLY_T    *pDeleteWaitingSendReplyReference)
{
	WAITING_SEND_REPLY_T *iterWaitingSendReplyReference;
	extern VOS_MUTEX_T pWaitingSendReplyReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadWaitingSendReplyReference == NULL || *ppHeadWaitingSendReplyReference == NULL || pDeleteWaitingSendReplyReference == NULL)
    {
        return TRDP_PARAM_ERR;
    }

	/* Check Mutex ? */
    if (pWaitingSendReplyReferenceMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing WaitingSendReplyReference Mutex Err\n");
    	return TRDP_MUTEX_ERR;
    }
    else
    {
		/* Lock WaitingSendReplyReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingSendReplyReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingSendReplyReference Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
	}

    /* handle removal of first element */
    if (pDeleteWaitingSendReplyReference == *ppHeadWaitingSendReplyReference)
    {
		*ppHeadWaitingSendReplyReference = pDeleteWaitingSendReplyReference->pNextWaitingSendReplyReference;
		vos_memFree(pDeleteWaitingSendReplyReference);
		pDeleteWaitingSendReplyReference = NULL;
		/* UnLock WaitingSendReplyReference by Mutex */
		vos_mutexUnlock(pWaitingSendReplyReferenceMutex);
		return TRDP_NO_ERR;
    }

    for (iterWaitingSendReplyReference = *ppHeadWaitingSendReplyReference;
    		iterWaitingSendReplyReference != NULL;
    		iterWaitingSendReplyReference = iterWaitingSendReplyReference->pNextWaitingSendReplyReference)
    {
        if (iterWaitingSendReplyReference->pNextWaitingSendReplyReference == pDeleteWaitingSendReplyReference)
        {
        	iterWaitingSendReplyReference->pNextWaitingSendReplyReference = pDeleteWaitingSendReplyReference->pNextWaitingSendReplyReference;
        	vos_memFree(pDeleteWaitingSendReplyReference);
        	pDeleteWaitingSendReplyReference = NULL;
        	break;
        }
    }
	/* UnLock WaitingSendReplyReference by Mutex */
	vos_mutexUnlock(pWaitingSendReplyReferenceMutex);
	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the WaitinSendReply with same callerRef
 *
 *  @param[in]      	pHeadWaitingReceiveRequestReference	pointer to head of queue
 *  @param[in]		sessionRef									pointer to session Reference
 *
 *  @retval         != NULL						pointer to Waiting Send Reply Reference
 *  @retval         NULL							No Waiting Send Reply Reference found
 */
WAITING_SEND_REPLY_T *searchWaitingSendReplyReference (
		WAITING_SEND_REPLY_T		*pHeadWaitingSendReplyReference,
		void						*sessionRef)
{
	WAITING_SEND_REPLY_T *iterWaitingSendReplyReference;
	extern VOS_MUTEX_T pWaitingSendReplyReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;
	UINT32 sessionRefValue = 0;
	UINT32 iterSessionReference = 0;

	/* Check Parameter */
    if (pHeadWaitingSendReplyReference == NULL
    	|| sessionRef == NULL)
    {
        return NULL;
    }
    /* Get sessionRef Address */
    memcpy(&sessionRefValue, sessionRef, sizeof(UINT32));
	/* Check Mutex ? */
    if (pWaitingSendReplyReferenceMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing WaitingSendReplyReference Mutex Err\n");
    	return NULL;
    }
    else
    {
		/* Lock WaitingSendReplyReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingSendReplyReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingSendReplyReference Mutex Lock failed\n");
			return NULL;
		}
	}
vos_printLog(VOS_LOG_INFO, "searchWatingSendReplyReference: %d\n", sessionRefValue);

    /* Check WaitingSendReply Reference List Loop */
    for (iterWaitingSendReplyReference = pHeadWaitingSendReplyReference;
    		iterWaitingSendReplyReference != NULL;
    		iterWaitingSendReplyReference = iterWaitingSendReplyReference->pNextWaitingSendReplyReference)
    {
    	/* Get inter Waiting SendReply Reference Value */
    	memcpy(&iterSessionReference, (void *)iterWaitingSendReplyReference->sessionReference, sizeof(iterSessionReference));
        /* Waiting Send Reply Reference: We match if session Reference is matches */
    	if (iterSessionReference == sessionRefValue)
        {
    		/* UnLock WaitingSendReplyReference by Mutex */
    		vos_mutexUnlock(pWaitingSendReplyReferenceMutex);
    		return iterWaitingSendReplyReference;
        }
    	else
        {
        	continue;
        }
    }
	/* UnLock WaitingSendReplyReference by Mutex */
	vos_mutexUnlock(pWaitingSendReplyReferenceMutex);
	return NULL;
}
/**********************************************************************************************************************/
/** Append an Waiting Receive Reply Reference at end of List
 *
 *  @param[in]      ppHeadWaitingReceiveReplyReference          pointer to pointer to head of List
 *  @param[in]      pNewWaitingReceiveReplyReference            pointer to Waiting Receive Reply Reference to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendWaitingReceiveReplyQueue (
		WAITING_RECEIVE_REPLY_T    * *ppHeadWaitingReceiveReplyReference,
		WAITING_RECEIVE_REPLY_T    *pNewWaitingReceiveReplyReference)
{
	WAITING_RECEIVE_REPLY_T *iterWaitingReceiveReplyReference;
	extern VOS_MUTEX_T pWaitingReceiveReplyReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Parameter Check */
    if (ppHeadWaitingReceiveReplyReference == NULL || pNewWaitingReceiveReplyReference == NULL)
    {
        return TRDP_PARAM_ERR;
    }
vos_printLog(VOS_LOG_INFO, "appendWaitingReceiveReplyReference() session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
		pNewWaitingReceiveReplyReference->sessionId[0], pNewWaitingReceiveReplyReference->sessionId[1], pNewWaitingReceiveReplyReference->sessionId[2], pNewWaitingReceiveReplyReference->sessionId[3],
		pNewWaitingReceiveReplyReference->sessionId[4], pNewWaitingReceiveReplyReference->sessionId[5], pNewWaitingReceiveReplyReference->sessionId[6], pNewWaitingReceiveReplyReference->sessionId[7]);

    /* First WaitingReceiveReply Reference ? */
    if (*ppHeadWaitingReceiveReplyReference == pNewWaitingReceiveReplyReference)
    {
    	return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewWaitingReceiveReplyReference->pNextWaitingReceiveReplyReference = NULL;

	/* Check Mutex ? */
    if (pWaitingReceiveReplyReferenceMutex == NULL)
    {
    	/* Create WaitingReceiveReplyReference Access Mutex */
    	vosErr = vos_mutexCreate(&pWaitingReceiveReplyReferenceMutex);
    	if (vosErr != VOS_NO_ERR)
    	{
    		vos_printLog(VOS_LOG_ERROR, "Create WaitingReceiveReplyReference Mutex Err\n");
    		return TRDP_MUTEX_ERR;
    	}
    	else
    	{
    		/* Lock WaitingReceiveReplyReference by Mutex */
        	vosErr = vos_mutexLock(pWaitingReceiveReplyReferenceMutex);
    		if (vosErr != VOS_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveReplyReference Mutex Lock failed\n");
    			return TRDP_MUTEX_ERR;
    		}
    	}
    }
    else
    {
		/* Lock WaitingReceiveReplyReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingReceiveReplyReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveReplyReference Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    if (*ppHeadWaitingReceiveReplyReference == NULL)
    {
		*ppHeadWaitingReceiveReplyReference = pNewWaitingReceiveReplyReference;
		/* UnLock WaitingReceiveReplyReference by Mutex */
		vos_mutexUnlock(pWaitingReceiveReplyReferenceMutex);
		return TRDP_NO_ERR;
    }

    for (iterWaitingReceiveReplyReference = *ppHeadWaitingReceiveReplyReference;
    		iterWaitingReceiveReplyReference->pNextWaitingReceiveReplyReference != NULL;
    		iterWaitingReceiveReplyReference = iterWaitingReceiveReplyReference->pNextWaitingReceiveReplyReference)
    {
        ;
    }
	iterWaitingReceiveReplyReference->pNextWaitingReceiveReplyReference = pNewWaitingReceiveReplyReference;
	/* UnLock WaitingReceiveReplyReference by Mutex */
	vos_mutexUnlock(pWaitingReceiveReplyReferenceMutex);
	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Waiting Receive Reply Reference List
 *
 *  @param[in]      ppHeadWaitingReceiveReplyReference			pointer to pointer to head of queue
 *  @param[in]      pDeleteWaitingReceiveReplyReference			pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 *PD_REQUEST
 */
TRDP_ERR_T deleteWaitingReceiveReplyQueue (
		WAITING_RECEIVE_REPLY_T    * *ppHeadWaitingReceiveReplyReference,
		WAITING_RECEIVE_REPLY_T    *pDeleteWaitingReceiveReplyReference)
{
	WAITING_RECEIVE_REPLY_T *iterWaitingReceiveReplyReference;
	extern VOS_MUTEX_T pWaitingReceiveReplyReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadWaitingReceiveReplyReference == NULL || *ppHeadWaitingReceiveReplyReference == NULL || pDeleteWaitingReceiveReplyReference == NULL)
    {
        return TRDP_PARAM_ERR;
    }

	/* Check Mutex ? */
    if (pWaitingReceiveReplyReferenceMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing WaitingReceiveReplyReference Mutex Err\n");
    	return TRDP_MUTEX_ERR;
    }
    else
    {
		/* Lock WaitingReceiveReplyReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingReceiveReplyReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveReplyReference Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    /* handle removal of first element */
    if (pDeleteWaitingReceiveReplyReference == *ppHeadWaitingReceiveReplyReference)
    {
vos_printLog(VOS_LOG_INFO, "deleteWaitingReceiveReplyReference() session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
		pDeleteWaitingReceiveReplyReference->sessionId[0], pDeleteWaitingReceiveReplyReference->sessionId[1], pDeleteWaitingReceiveReplyReference->sessionId[2], pDeleteWaitingReceiveReplyReference->sessionId[3],
		pDeleteWaitingReceiveReplyReference->sessionId[4], pDeleteWaitingReceiveReplyReference->sessionId[5], pDeleteWaitingReceiveReplyReference->sessionId[6], pDeleteWaitingReceiveReplyReference->sessionId[7]);

		*ppHeadWaitingReceiveReplyReference = pDeleteWaitingReceiveReplyReference->pNextWaitingReceiveReplyReference;
		vos_memFree(pDeleteWaitingReceiveReplyReference);
		pDeleteWaitingReceiveReplyReference = NULL;
		/* UnLock WaitingReceiveReplyReference by Mutex */
		vos_mutexUnlock(pWaitingReceiveReplyReferenceMutex);
		return TRDP_NO_ERR;
    }

    for (iterWaitingReceiveReplyReference = *ppHeadWaitingReceiveReplyReference;
    		iterWaitingReceiveReplyReference != NULL;
    		iterWaitingReceiveReplyReference = iterWaitingReceiveReplyReference->pNextWaitingReceiveReplyReference)
    {
        if (iterWaitingReceiveReplyReference->pNextWaitingReceiveReplyReference == pDeleteWaitingReceiveReplyReference)
        {
vos_printLog(VOS_LOG_INFO, "deleteWaitingReceiveReplyReference() session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
		pDeleteWaitingReceiveReplyReference->sessionId[0], pDeleteWaitingReceiveReplyReference->sessionId[1], pDeleteWaitingReceiveReplyReference->sessionId[2], pDeleteWaitingReceiveReplyReference->sessionId[3],
		pDeleteWaitingReceiveReplyReference->sessionId[4], pDeleteWaitingReceiveReplyReference->sessionId[5], pDeleteWaitingReceiveReplyReference->sessionId[6], pDeleteWaitingReceiveReplyReference->sessionId[7]);

        	iterWaitingReceiveReplyReference->pNextWaitingReceiveReplyReference = pDeleteWaitingReceiveReplyReference->pNextWaitingReceiveReplyReference;
        	vos_memFree(pDeleteWaitingReceiveReplyReference);
        	pDeleteWaitingReceiveReplyReference = NULL;
        	break;
        }
    }
	/* UnLock WaitingReceiveReplyReference by Mutex */
	vos_mutexUnlock(pWaitingReceiveReplyReferenceMutex);
	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the WaitingReceiveReply with same callerRef
 *
 *  @param[in]      	pHeadWaitingReceiveRequestReference	pointer to head of queue
 *  @param[in]		callerRef									pointer to caller Reference
 *  @param[in]		replySessionId							receive Reply SessionId
 *
 *  @retval         != NULL						pointer to Waiting Receive Reply Reference
 *  @retval         NULL							No Waiting Receive Reply Reference found
 */
WAITING_RECEIVE_REPLY_T *searchWaitingReceiveReplyReference (
		WAITING_RECEIVE_REPLY_T		*pHeadWaitingReceiveReplyReference,
		void							*callerRef,
		TRDP_UUID_T					replySessionId)
{
	WAITING_RECEIVE_REPLY_T *iterWaitingReceiveReplyReference;
	extern VOS_MUTEX_T pWaitingReceiveReplyReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;
	UINT32 callerRefValue = 0;
	UINT32 iterCallerReference = 0;

	/* Check Parameter */
    if (pHeadWaitingReceiveReplyReference == NULL
    	|| callerRef == NULL)
    {
        return NULL;
    }

if (replySessionId != SESSION_ID_NOTHING)
{
vos_printLog(VOS_LOG_INFO, "searchWaitingReceiveReplyReference() reply session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
				replySessionId[0], replySessionId[1], replySessionId[2], replySessionId[3],
				replySessionId[4], replySessionId[5], replySessionId[6], replySessionId[7]);
}
    /* Get callerRef Address */
    memcpy(&callerRefValue, callerRef, sizeof(UINT32));

	/* Check Mutex ? */
    if (pWaitingReceiveReplyReferenceMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing WaitingReceiveReplyReference Mutex Err\n");
    	return NULL;
    }
    else
    {
		/* Lock WaitingReceiveReplyReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingReceiveReplyReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveReplyReference Mutex Lock failed\n");
			return NULL;
		}
    }

    /* Check WaitingReceiveReply Reference List Loop */
    for (iterWaitingReceiveReplyReference = pHeadWaitingReceiveReplyReference;
    		iterWaitingReceiveReplyReference != NULL;
    		iterWaitingReceiveReplyReference = iterWaitingReceiveReplyReference->pNextWaitingReceiveReplyReference)
    {
    	/* Get inter Waiting Receive Reply Reference Value */
    	memcpy(&iterCallerReference, (void *)iterWaitingReceiveReplyReference->callerReference, sizeof(iterCallerReference));
    	/* Waiting Receive Reply Reference: We match if caller Reference is matches */
    	if (iterCallerReference == callerRefValue)
        {
    		/* UnLock WaitingReceiveReplyReference by Mutex */
    		vos_mutexUnlock(pWaitingReceiveReplyReferenceMutex);
    		return iterWaitingReceiveReplyReference;
        }
    	/* Check sessionId : Request SessionId equal Reply SessionId */
    	else if (replySessionId != SESSION_ID_NOTHING)
    	{
    		if (strncmp((char *)iterWaitingReceiveReplyReference->sessionId,
				  (char *)replySessionId,
				  sizeof(iterWaitingReceiveReplyReference->sessionId)) == 0)
			{
				/* UnLock WaitingReceiveReplyReference by Mutex */
				vos_mutexUnlock(pWaitingReceiveReplyReferenceMutex);
				return iterWaitingReceiveReplyReference;
			}
    	}
    	else
        {
        	continue;
        }
    }
	/* UnLock WaitingReceiveReplyReference by Mutex */
	vos_mutexUnlock(pWaitingReceiveReplyReferenceMutex);
	return NULL;
}

/**********************************************************************************************************************/
/** Append an Waiting Receive Confirm Reference at end of List
 *
 *  @param[in]      ppHeadWaitingReceiveConfirmReference          pointer to pointer to head of List
 *  @param[in]      pNewWaitingReceiveConfirmReference            pointer to Waiting Receive Confirm Reference to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendWaitingReceiveConfirmQueue (
		WAITING_RECEIVE_CONFIRM_T    * *ppHeadWaitingReceiveConfirmReference,
		WAITING_RECEIVE_CONFIRM_T    *pNewWaitingReceiveConfirmReference)
{
	WAITING_RECEIVE_CONFIRM_T *iterWaitingReceiveConfirmReference;
	extern VOS_MUTEX_T pWaitingReceiveConfirmReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

	/* Parameter Check */
    if (ppHeadWaitingReceiveConfirmReference == NULL || pNewWaitingReceiveConfirmReference == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First WaitingReceiveConfirm Reference ? */
    if (*ppHeadWaitingReceiveConfirmReference == pNewWaitingReceiveConfirmReference)
    {
    	return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewWaitingReceiveConfirmReference->pNextWaitingReceiveConfirmReference = NULL;

	/* Check Mutex ? */
    if (pWaitingReceiveConfirmReferenceMutex == NULL)
    {
    	/* Create WaitingReceiveConfirmReference Access Mutex */
    	vosErr = vos_mutexCreate(&pWaitingReceiveConfirmReferenceMutex);
    	if (vosErr != VOS_NO_ERR)
    	{
    		vos_printLog(VOS_LOG_ERROR, "Create WaitingReceiveConfirmReference Mutex Err\n");
    		return TRDP_MUTEX_ERR;
    	}
    	else
    	{
    		/* Lock WaitingReceiveConfirmReference by Mutex */
			vosErr = vos_mutexLock(pWaitingReceiveConfirmReferenceMutex);
			if (vosErr != VOS_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "WaitingReceiveConfirmReference Mutex Lock failed\n");
				return TRDP_MUTEX_ERR;
			}
    	}
    }
    else
    {
		/* Lock WaitingReceiveConfirmReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingReceiveConfirmReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveConfirmReference Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    if (*ppHeadWaitingReceiveConfirmReference == NULL)
    {
		*ppHeadWaitingReceiveConfirmReference = pNewWaitingReceiveConfirmReference;
		/* UnLock WaitingReceiveConfirmReference by Mutex */
		vos_mutexUnlock(pWaitingReceiveConfirmReferenceMutex);
		return TRDP_NO_ERR;
    }

    for (iterWaitingReceiveConfirmReference = *ppHeadWaitingReceiveConfirmReference;
    		iterWaitingReceiveConfirmReference->pNextWaitingReceiveConfirmReference != NULL;
    		iterWaitingReceiveConfirmReference = iterWaitingReceiveConfirmReference->pNextWaitingReceiveConfirmReference)
    {
        ;
    }
	iterWaitingReceiveConfirmReference->pNextWaitingReceiveConfirmReference = pNewWaitingReceiveConfirmReference;
	/* UnLock WaitingReceiveConfirmReference by Mutex */
	vos_mutexUnlock(pWaitingReceiveConfirmReferenceMutex);
	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Waiting Receive Confirm Reference List
 *
 *  @param[in]      ppHeadWaitingReceiveConfirmReference			pointer to pointer to head of queue
 *  @param[in]      pDeleteWaitingReceiveConfirmReference			pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 */
TRDP_ERR_T deleteWaitingReceiveConfirmQueue (
		WAITING_RECEIVE_CONFIRM_T    * *ppHeadWaitingReceiveConfirmReference,
		WAITING_RECEIVE_CONFIRM_T    *pDeleteWaitingReceiveConfirmReference)
{
	WAITING_RECEIVE_CONFIRM_T *iterWaitingReceiveConfirmReference;
	extern VOS_MUTEX_T pWaitingReceiveConfirmReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadWaitingReceiveConfirmReference == NULL || *ppHeadWaitingReceiveConfirmReference == NULL || pDeleteWaitingReceiveConfirmReference == NULL)
    {
        return TRDP_PARAM_ERR;
    }

	/* Check Mutex ? */
    if (pWaitingReceiveConfirmReferenceMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing WaitingReceiveConfirmReference Mutex Err\n");
    	return TRDP_MUTEX_ERR;
    }
    else
    {
		/* Lock WaitingReceiveConfirmReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingReceiveConfirmReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveConfirmReference Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    /* handle removal of first element */
    if (pDeleteWaitingReceiveConfirmReference == *ppHeadWaitingReceiveConfirmReference)
    {
		*ppHeadWaitingReceiveConfirmReference = pDeleteWaitingReceiveConfirmReference->pNextWaitingReceiveConfirmReference;
		vos_memFree(pDeleteWaitingReceiveConfirmReference);
		pDeleteWaitingReceiveConfirmReference = NULL;
		/* UnLock WaitingReceiveConfirmReference by Mutex */
		vos_mutexUnlock(pWaitingReceiveConfirmReferenceMutex);
		return TRDP_NO_ERR;
    }

    for (iterWaitingReceiveConfirmReference = *ppHeadWaitingReceiveConfirmReference;
    		iterWaitingReceiveConfirmReference != NULL;
    		iterWaitingReceiveConfirmReference = iterWaitingReceiveConfirmReference->pNextWaitingReceiveConfirmReference)
    {
        if (iterWaitingReceiveConfirmReference->pNextWaitingReceiveConfirmReference == pDeleteWaitingReceiveConfirmReference)
        {
        	iterWaitingReceiveConfirmReference->pNextWaitingReceiveConfirmReference = pDeleteWaitingReceiveConfirmReference->pNextWaitingReceiveConfirmReference;
        	vos_memFree(pDeleteWaitingReceiveConfirmReference);
        	pDeleteWaitingReceiveConfirmReference = NULL;
        	break;
        }
    }
	/* UnLock WaitingReceiveConfirmReference by Mutex */
	vos_mutexUnlock(pWaitingReceiveConfirmReferenceMutex);
	return TRDP_NO_ERR;
}
/**********************************************************************************************************************/
/** Return the WaitingReceiveConfirm with same callerRef
 *
 *  @param[in]      	pHeadWaitingReceiveConfrimReference	pointer to head of queue
 *  @param[in]		taulRef									pointer to taul Reference
 *
 *  @retval         != NULL						pointer to Waiting Receive Request Reference
 *  @retval         NULL							No Waiting Receive Request Reference found
 */
WAITING_RECEIVE_CONFIRM_T *searchWaitingReceiveConfirmReference (
		WAITING_RECEIVE_CONFIRM_T		*pHeadWaitingReceiveConfirmReference,
		void								*taulRef)
{
	WAITING_RECEIVE_CONFIRM_T *iterWaitingReceiveConfirmReference;
	extern VOS_MUTEX_T pWaitingReceiveConfirmReferenceMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;
	UINT32 taulRefValue = 0;

	/* Check Parameter */
    if (pHeadWaitingReceiveConfirmReference == NULL
    	|| taulRef == NULL)
    {
        return NULL;
    }
    /* Get taulRef Address */
    memcpy(&taulRefValue, taulRef, sizeof(UINT32));

	/* Check Mutex ? */
    if (pWaitingReceiveConfirmReferenceMutex == NULL)
    {
    	vos_printLog(VOS_LOG_ERROR, "Nothing WaitingReceiveConfirmReference Mutex Err\n");
    	return NULL;
    }
    else
    {
		/* Lock WaitingReceiveConfirmReference by Mutex */
    	vosErr = vos_mutexLock(pWaitingReceiveConfirmReferenceMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "WaitingReceiveConfirmReference Mutex Lock failed\n");
			return NULL;
		}
    }

    /* Check WaitingReceiveConfirm Reference List Loop */
    for (iterWaitingReceiveConfirmReference = pHeadWaitingReceiveConfirmReference;
    		iterWaitingReceiveConfirmReference != NULL;
    		iterWaitingReceiveConfirmReference = iterWaitingReceiveConfirmReference->pNextWaitingReceiveConfirmReference)
    {
        /* Waiting Receive Confirm Reference: We match if TAUL Reference is matches */
    	if (iterWaitingReceiveConfirmReference->taulReference == taulRefValue)
    	{
    		/* UnLock WaitingReceiveConfirmReference by Mutex */
    		vos_mutexUnlock(pWaitingReceiveConfirmReferenceMutex);
    		return iterWaitingReceiveConfirmReference;
        }
    	else
        {
        	continue;
        }
    }
	/* UnLock WaitingReceiveConfirmReference by Mutex */
	vos_mutexUnlock(pWaitingReceiveConfirmReferenceMutex);
	return NULL;
}

/**********************************************************************************************************************/
/** Append an Listener Handle at end of List
 *
 *  @param[in]      ppHeadListenerHandle			pointer to pointer to head of List
 *  @param[in]      pNewListenerHandle				pointer to Listener Handle to append
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter error
 */
TRDP_ERR_T appendListenerHandleList(
		LISTENER_HANDLE_T    * *ppHeadListenerHandle,
		LISTENER_HANDLE_T    *pNewListenerHandle)
{
	LISTENER_HANDLE_T *iterListenerHandle;
	extern VOS_MUTEX_T pListenerHandleListMutex;
	VOS_ERR_T vosErr = VOS_NO_ERR;

    /* Parameter Check */
	if (ppHeadListenerHandle == NULL || pNewListenerHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

	/* First Listener Handle */
	if (*ppHeadListenerHandle == pNewListenerHandle)
	{
		return TRDP_NO_ERR;
	}

    /* Ensure this List is last! */
	pNewListenerHandle->pNextListenerHandle = NULL;

	/* Check Mutex ? */
    if (pListenerHandleListMutex == NULL)
    {
    	/* Create ListenerHandleList Access Mutex */
    	vosErr = vos_mutexCreate(&pListenerHandleListMutex);
    	if (vosErr != VOS_NO_ERR)
    	{
    		vos_printLog(VOS_LOG_ERROR, "Create ListenerHandleList Mutex Err\n");
    		return TRDP_MUTEX_ERR;
    	}
    	else
    	{
    		/* Lock ListenerHandleList by Mutex */
        	vosErr = vos_mutexLock(pListenerHandleListMutex);
    		if (vosErr != VOS_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "ListenerHandleList Mutex Lock failed\n");
    			return TRDP_MUTEX_ERR;
    		}
    	}
    }
    else
    {
		/* Lock ListenerHandleList by Mutex */
    	vosErr = vos_mutexLock(pListenerHandleListMutex);
		if (vosErr != VOS_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "ListenerHandleList Mutex Lock failed\n");
			return TRDP_MUTEX_ERR;
		}
    }

    if (*ppHeadListenerHandle == NULL)
    {
		*ppHeadListenerHandle = pNewListenerHandle;
		/* UnLock ListenerHandleList by Mutex */
		vos_mutexUnlock(pListenerHandleListMutex);
		return TRDP_NO_ERR;
    }

    for (iterListenerHandle = *ppHeadListenerHandle;
    	  iterListenerHandle->pNextListenerHandle != NULL;
    	  iterListenerHandle = iterListenerHandle->pNextListenerHandle)
    {
        ;
    }
    if (iterListenerHandle != pNewListenerHandle)
    {
    	iterListenerHandle->pNextListenerHandle = pNewListenerHandle;
    }
	/* UnLock ListenerHandleList by Mutex */
	vos_mutexUnlock(pListenerHandleListMutex);
	return TRDP_NO_ERR;
}

#ifndef XML_CONFIG_ENABLE
/******************************************************************************/
/** Set TRDP Config Parameter From internal config
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T setConfigParameterFromInternalConfig (
	void)
{
	UINT32					i = 0;							/* Loop Counter */
	UINT32					datasetIndex = 0;				/* Loop Counter of Dataset Index */
	UINT32					elementIndex = 0;				/* Loop Counter of Dataset element Index */
	UINT32					interfaceNumberIndex = 0;	/* Loop Counter of Interface Number Index */
	UINT32					exchgParIndex = 0;			/* Loop Counter of Exchange Parameter Index */
	pTRDP_DATASET_T		pDataset = NULL;				/* pointer to Dataset */
	const TRDP_CHAR_IP_ADDR_T DOTTED_IP_ADDRESS__NOTHING	 = "";		/* Dotted IP Address Nothing */

	/* Set IF Config *****/
	/* Get IF Config memory area */
	pIfConfig = (TRDP_IF_CONFIG_T *)vos_memAlloc(sizeof(TRDP_IF_CONFIG_T) * NUM_IF_CONFIG);
	if (pIfConfig == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"setConfigParameterFromInternalConfig() Failed. Array IF Config vos_memAlloc() Err\n");
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize IF Config */
		memset(pIfConfig, 0, (sizeof(TRDP_IF_CONFIG_T) * NUM_IF_CONFIG));
	}
	/* IF Config Loop */
	for (i = 0; i < NUM_IF_CONFIG; i++)
	{
		/* Set Interface Name of Array IF Config */
		memcpy(pIfConfig[i].ifName, arrayInternalIfConfig[i].ifName, sizeof(TRDP_LABEL_T));
		/* Set Network Id of Array IF Config */
		pIfConfig[i].networkId = arrayInternalIfConfig[i].networkId;
		/* Convert Host IP Address, and Set Host IP Address of Array IF Config */
		if (memcmp(arrayInternalIfConfig[i].dottedHostIp,
					DOTTED_IP_ADDRESS__NOTHING,
					sizeof(TRDP_CHAR_IP_ADDR_T)) != 0)
		{
			pIfConfig[i].hostIp = vos_dottedIP(arrayInternalIfConfig[i].dottedHostIp);
		}
		/* Convert Leader IP Address, and Set Leader IP Address of Array IF Config */
		if (memcmp(arrayInternalIfConfig[i].dottedLeaderIp,
					DOTTED_IP_ADDRESS__NOTHING,
					sizeof(TRDP_CHAR_IP_ADDR_T)) != 0)
		{
			pIfConfig[i].leaderIp = vos_dottedIP(arrayInternalIfConfig[i].dottedLeaderIp);
		}
	}

	/* Set Communication Parameter */
	pComPar = arrayComParConfig;

	/* Set ComIdDatasetIdMap Config */
	pComIdDsIdMap = arrayComIdDsIdMapConfig;

	/* Set Dataset Config *****/
#if 0
	/* Get Array Dataset Config memory area */
    apDataset = (apTRDP_DATASET_T)vos_memAlloc(sizeof(TRDP_DATASET_T *) * NUM_DATASET);
	if (apDataset == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"setConfigParameterFromInternalConfig() Failed. Array Dataset Config vos_memAlloc() Err\n");
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Array Dataset Config */
		memset(apDataset, 0, (sizeof(TRDP_DATASET_T *) * NUM_DATASET));
	}
	/* Dataset Loop */
	for(i = 0; i < numDataset; i++)
	{
		/* Get Dataset Config memory area */
		pDataset = (TRDP_DATASET_T *)vos_memAlloc(sizeof(TRDP_DATASET_T) + (sizeof(TRDP_DATASET_ELEMENT_T) * arrayInternalDatasetConfig[i].numElement));
		if (pDataset== NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"setConfigParameterFromInternalConfig() Failed. Dataset Config vos_memAlloc() Err\n");
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize Dataset Config */
			memset(pDataset, 0, (sizeof(TRDP_DATASET_ELEMENT_T) * arrayInternalDatasetConfig[i].numElement));
		}
		/* Set Dataset Address in Array Dataset Config */
		apDataset[i] = pDataset;
	}
#endif
	/* Get Array Dataset Config memory area */
    apDataset = (apTRDP_DATASET_T)vos_memAlloc(sizeof(TRDP_DATASET_T *) * NUM_DATASET);
	if (apDataset == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"setConfigParameterFromInternalConfig() Failed. Array Dataset Config vos_memAlloc() Err\n");
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Array Dataset Config */
		memset(apDataset, 0, (sizeof(TRDP_DATASET_T *) * NUM_DATASET));
	}
	/* Dataset Loop */
	for(datasetIndex = 0; datasetIndex < numDataset; datasetIndex++)
	{
		/* Get Dataset Config memory area */
		pDataset = (TRDP_DATASET_T *)vos_memAlloc(sizeof(TRDP_DATASET_T) + (sizeof(TRDP_DATASET_ELEMENT_T) * arrayInternalDatasetConfig[datasetIndex].numElement));
		if (pDataset== NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"setConfigParameterFromInternalConfig() Failed. Dataset Config vos_memAlloc() Err\n");
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize Dataset Config */
			memset(pDataset, 0, (sizeof(TRDP_DATASET_T) + sizeof(TRDP_DATASET_ELEMENT_T) * arrayInternalDatasetConfig[datasetIndex].numElement));
		}
		/* Set Dataset Address in Array Dataset Config */
		apDataset[datasetIndex] = pDataset;
		/* Set Id of Array Dataset */
//		apDataset[datasetIndex].id = arrayInternalDatasetConfig[datasetIndex].id;
		pDataset->id = arrayInternalDatasetConfig[datasetIndex].id;
		/* Set Reserved of Array Dataset */
		pDataset->reserved1 = arrayInternalDatasetConfig[datasetIndex].reserved1;
		/* Set Number of Element of Array Dataset */
		pDataset->numElement = arrayInternalDatasetConfig[datasetIndex].numElement;
		/* Set Pointer to Element of Array Dataset */
//		pDataset->pElement = arrayInternalDatasetConfig[datasetIndex].pElement;
		/* Set Element Loop */
		for(elementIndex = 0; elementIndex < pDataset->numElement; elementIndex++)
		{
			/* Set Element Type */
			pDataset->pElement[elementIndex].type = arrayInternalDatasetConfig[datasetIndex].pElement[elementIndex].type;
			/* Set Element Size */
			pDataset->pElement[elementIndex].size = arrayInternalDatasetConfig[datasetIndex].pElement[elementIndex].size;
			/* Set Element Pointer to Dataset cache */
			pDataset->pElement[elementIndex].pCachedDS = arrayInternalDatasetConfig[datasetIndex].pElement[elementIndex].pCachedDS;
		}
	}

	/* Set IF Config Parameter *****/
#if 0
	/* Get Exchange Parameter Config memory area */
	*arrayExchgPar = (TRDP_EXCHG_PAR_T *)vos_memAlloc(numIfConfig * (sizeof(TRDP_EXCHG_PAR_T) * numExchgPar));
	if (arrayExchgPar== NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"setConfigParameterFromInternalConfig() Failed. IF Config Parameter vos_memAlloc() Err\n");
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize If Config Parameter */
		memset(arrayExchgPar, 0, (numIfConfig * (sizeof(TRDP_EXCHG_PAR_T) * numExchgPar)));
	}
#endif
	/* Subnet Loop */
	for (interfaceNumberIndex = 0; interfaceNumberIndex < numIfConfig; interfaceNumberIndex++)
	{

		/* Get Exchange Parameter Config memory area */
		arrayExchgPar[interfaceNumberIndex] = (TRDP_EXCHG_PAR_T *)vos_memAlloc((sizeof(TRDP_EXCHG_PAR_T) * numExchgPar));
		if (arrayExchgPar[interfaceNumberIndex]== NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"setConfigParameterFromInternalConfig() Failed. IF Config Parameter vos_memAlloc() Err\n");
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize If Config Parameter */
			memset(arrayExchgPar[interfaceNumberIndex], 0, (sizeof(TRDP_EXCHG_PAR_T) * numExchgPar));
		}

		/* Exchange Parameter Loop */
		for (exchgParIndex = 0; exchgParIndex < numExchgPar; exchgParIndex++)
		{

			/* Set comId of Array Exchange Parameter */
			arrayExchgPar[interfaceNumberIndex][exchgParIndex].comId = arrayInternalConfigExchgPar[interfaceNumberIndex][exchgParIndex].comId;
			/* Set datasetId of Array Exchange Parameter */
			arrayExchgPar[interfaceNumberIndex][exchgParIndex].datasetId = arrayInternalConfigExchgPar[interfaceNumberIndex][exchgParIndex].datasetId;
			/* Set communication parameter Id of Array Exchange Parameter */
			arrayExchgPar[interfaceNumberIndex][exchgParIndex].comParId = arrayInternalConfigExchgPar[interfaceNumberIndex][exchgParIndex].comParId;
			/* Set Pointer to MD Parameter for this connection of Array Exchange Parameter */
			arrayExchgPar[interfaceNumberIndex][exchgParIndex].pMdPar = arrayInternalConfigExchgPar[interfaceNumberIndex][exchgParIndex].pMdPar;
			/* Set Pointer to PD Parameter for this connection of Array Exchange Parameter */
			arrayExchgPar[interfaceNumberIndex][exchgParIndex].pPdPar = arrayInternalConfigExchgPar[interfaceNumberIndex][exchgParIndex].pPdPar;
			/* Set number of destinations of Array Exchange Parameter */
			arrayExchgPar[interfaceNumberIndex][exchgParIndex].destCnt = arrayInternalDestinationConfig[interfaceNumberIndex][exchgParIndex].destCnt;
			/* Set Pointer to array of destination descriptors of Array Exchange Parameter */
			if (arrayInternalDestinationConfig[interfaceNumberIndex][exchgParIndex].pDest != NULL)
			{
				arrayExchgPar[interfaceNumberIndex][exchgParIndex].pDest = arrayInternalDestinationConfig[interfaceNumberIndex][exchgParIndex].pDest;
			}
			/* Set number of sources of Array Exchange Parameter */
			arrayExchgPar[interfaceNumberIndex][exchgParIndex].srcCnt = arrayInternalSourceConfig[interfaceNumberIndex][exchgParIndex].srcCnt;
			/* Set Pointer to array of source descriptors of Array Exchange Parameter */
			if (arrayInternalSourceConfig[interfaceNumberIndex][exchgParIndex].pSrc != NULL)
			{
				arrayExchgPar[interfaceNumberIndex][exchgParIndex].pSrc = arrayInternalSourceConfig[interfaceNumberIndex][exchgParIndex].pSrc;
			}
		}
	}

	return TRDP_NO_ERR;
}
#endif /* #ifndef XML_CONFIG_ENABLE */

/******************************************************************************/
/** PD/MD Telegrams configured for one interface.
 * PD: Publisher, Subscriber, Requester
 * MD: Caller, Replier
 *
 *  @param[in]      ifIndex				interface Index
 *  @param[in]      numExchgPar			Number of Exchange Parameter
 *  @param[in]      pExchgPar			Pointer to Exchange Parameter
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T configureTelegrams (
		UINT32				ifIndex,
		UINT32 			numExchgPar,
		TRDP_EXCHG_PAR_T	*pExchgPar)
{
	UINT32 	telegramIndex = 0;
	TRDP_ERR_T	err = TRDP_NO_ERR;
	UINT32		getTelegramStatusFlag = TRDP_GET_NONE;
	const CHAR8 			IP_ADDRESS_ZERO[] = "0.0.0.0";

	/* Get Telegram */
	for (telegramIndex = 0; telegramIndex < numExchgPar; telegramIndex++)
	{
		/* PD Telegram */
		if (pExchgPar[telegramIndex].pPdPar != NULL)
		{
			/* Publisher */
			if ((pExchgPar[telegramIndex].destCnt > 0)
				&& (pExchgPar[telegramIndex].srcCnt == 0))
			{
				err = publishTelegram(ifIndex, &pExchgPar[telegramIndex]);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "configureTelegrams() failed. publishTelegram() error\n");
					return err;
				}
				else
				{
					continue;
				}
			}
			/* Subscriber */
			if ((pExchgPar[telegramIndex].destCnt > 0)
				&& (pExchgPar[telegramIndex].srcCnt > 0)
				&& (strncmp(*pExchgPar[telegramIndex].pSrc->pUriHost1, IP_ADDRESS_ZERO, sizeof(IP_ADDRESS_ZERO)) != 0))
			{
				err = subscribeTelegram(ifIndex, &pExchgPar[telegramIndex]);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "configureTelegrams() failed. subscribeTelegram() error\n");
					return err;
				}
				else
				{
					continue;
				}
			}
			/* PD Requester */
			if ((pExchgPar[telegramIndex].destCnt > 0)
				&& (pExchgPar[telegramIndex].srcCnt == 1)
				&& (strncmp(*pExchgPar[telegramIndex].pSrc->pUriHost1, IP_ADDRESS_ZERO, sizeof(IP_ADDRESS_ZERO)) == 0))
			{
				err = pdRequestTelegram(ifIndex, &pExchgPar[telegramIndex]);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "configureTelegrams() failed. pdRequestTelegram() error\n");
					return err;
				}
				else
				{
					continue;
				}
			}
		}
		/* MD Telegram */
		if (pExchgPar[telegramIndex].pMdPar != NULL)
		{
			/* MD Telegram */
			err = mdTelegram(ifIndex, &pExchgPar[telegramIndex], &getTelegramStatusFlag);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "configureTelegrams() failed. mdTelegram() error\n");
				return err;
			}
			else
			{
				continue;
			}
		}
	}
	return TRDP_NO_ERR;
}

#if 0
/******************************************************************************/
/** Size of Dataset writing in Traffic Store
 *
 *  @param[in,out]  pDstEnd         Pointer Destination End Address
 *  @param[in]      pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
TRDP_ERR_T sizeWriteDatasetInTrafficStore (
		UINT8					*pDstEnd,
		UINT32					*pDatasetSize,
		TRDP_DATASET_T		*pDataset)
{
	TRDP_ERR_T		err = TRDP_NO_ERR;
	UINT16			lIndex = 0;
	UINT8			alignment = 0;
	UINT8			firstAlignment = 0;
	UINT8			mod = 0;
	UINT32			dstEndAddress = 0;
	UINT32			var_size = 0;
	UINT8			*pDstWorkEnd = NULL;
	UINT32			size = 0;
	UINT32			workSize = 0;

	/* Check Parameter */
	if ((pDstEnd == NULL)
		|| (pDataset == NULL))
	{
		vos_printLog(VOS_LOG_ERROR,"size_writeDataseInTrafficStore() Failed. Parameter Err.\n");
		return TRDP_PARAM_ERR;
	}

	/* Get Size */
	memcpy(&size, pDatasetSize, sizeof(UINT32));

	/* Check first calculate */
	if (size == 0)
	{
		/* Check pDstEnd Address */
		/* Set pDstEnd Address */
		dstEndAddress = (UINT32)pDstEnd;
		vos_printLog(VOS_LOG_DBG, "firstAlignment dstEndAddress 0x%x\n", dstEndAddress);
		/* Adjust alignment */
		mod = dstEndAddress % 4;
		vos_printLog(VOS_LOG_DBG, "firstAlignment mod %u\n", mod);
		if (mod != 0)
		{
			/* Set First Alignment */
			firstAlignment = 4 - mod;
			vos_printLog(VOS_LOG_DBG, "firstAlignment %u\n", firstAlignment);
		}
	}
	/* Set Work Pointer */
	pDstWorkEnd = pDstEnd + firstAlignment;

	/*    Loop over all datasets in the array    */
	for (lIndex = 0; lIndex < pDataset->numElement; ++lIndex)
	{
		UINT32 noOfItems = pDataset->pElement[lIndex].size;

		if (TDRP_VAR_SIZE == noOfItems) /* variable size    */
		{
			noOfItems = var_size;
		}

		/*    Is this a composite type?    */
		if (pDataset->pElement[lIndex].type > (UINT32) TRDP_TYPE_MAX)
		{
			while (noOfItems-- > 0)
			{
				/* Dataset, call ourself recursively */

				/* Never used before?  */
				if (NULL == pDataset->pElement[lIndex].pCachedDS)
				{
					/* Look for it   */
					pDataset->pElement[lIndex].pCachedDS = find_DS(pDataset->pElement[lIndex].type);
				}
				if (NULL == pDataset->pElement[lIndex].pCachedDS)      /* Not in our DB    */
				{
					vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDataset->pElement[lIndex].type);
					return TRDP_COMID_ERR;
				}

				err = sizeWriteDatasetInTrafficStore(pDstEnd, pDatasetSize, pDataset->pElement[lIndex].pCachedDS);
				if (err != TRDP_NO_ERR)
				{
					return err;
				}
				/* Set Work Pointer */
				pDstWorkEnd = pDstEnd;
				/* Get Size */
				memcpy(&size, pDatasetSize, sizeof(UINT32));
				workSize = (UINT32)(pDstWorkEnd - pDstEnd);
				workSize = workSize + size;
				*pDatasetSize = workSize;
			}
		}
		else
		{
			/* Element Type */
			switch (pDataset->pElement[lIndex].type)
			{
				case TRDP_BOOL8:
				case TRDP_CHAR8:
				case TRDP_INT8:
				case TRDP_UINT8:
				{
					/* Size: 1Byte */
					while (noOfItems-- > 0)
					{
						pDstWorkEnd++;
					}
					break;
				}
				case TRDP_UTF16:
				case TRDP_INT16:
				case TRDP_UINT16:
				{
					/* Set pDstEnd Address */
					dstEndAddress = (UINT32)pDstWorkEnd;
					/* Adjust alignment */
					mod = dstEndAddress % 2;
					if (mod != 0)
					{
						pDstWorkEnd++;
					}
					/* Size: 2Byte */
					while (noOfItems-- > 0)
					{
						pDstWorkEnd += 2;
					}
					break;
				}
				case TRDP_INT32:
				case TRDP_UINT32:
				case TRDP_REAL32:
				case TRDP_TIMEDATE32:
				{
					/* Set pDstEnd Address */
					dstEndAddress = (UINT32)pDstWorkEnd;
					/* Adjust alignment */
					mod = dstEndAddress % 4;
					if (mod != 0)
					{
						alignment = 4 - mod;
						pDstWorkEnd = pDstWorkEnd + alignment;
					}
					/* Size: 4Byte */
					while (noOfItems-- > 0)
					{
						pDstWorkEnd += 4;
					}
					break;
				}
				case TRDP_TIMEDATE64:
				{
					/* Set pDstEnd Address */
					dstEndAddress = (UINT32)pDstWorkEnd;
					/* Adjust alignment */
					mod = dstEndAddress % 4;
					if (mod != 0)
					{
						alignment = 4 - mod;
						pDstWorkEnd = pDstWorkEnd + alignment;
					}
					/* Size: 4Byte + 4Byte */
					while (noOfItems-- > 0)
					{
						pDstWorkEnd += 8;
					}
					break;
				}
				case TRDP_TIMEDATE48:
				{
					/* Set pDstEnd Address */
					dstEndAddress = (UINT32)pDstWorkEnd;
					/* Adjust alignment */
					mod = dstEndAddress % 4;
					if (mod != 0)
					{
						alignment = 4 - mod;
						pDstWorkEnd = pDstWorkEnd + alignment;
					}
					/* Size: 4Byte + 2Byte */
					while (noOfItems-- > 0)
					{
						/* Set pDstEnd Address */
						dstEndAddress = (UINT32)pDstWorkEnd;
						/* Adjust alignment */
						mod = dstEndAddress % 4;
						if (mod != 0)
						{
							alignment = 4 - mod;
							pDstWorkEnd = pDstWorkEnd + alignment;
						}
						pDstWorkEnd += 6;
					}
					break;
				}
				case TRDP_INT64:
				case TRDP_UINT64:
				case TRDP_REAL64:
				{
					/* Set pDstEnd Address */
					dstEndAddress = (UINT32)pDstWorkEnd;
					/* Adjust alignment */
					mod = dstEndAddress % 8;
					if (mod != 0)
					{
						alignment = 8 - mod;
						pDstWorkEnd = pDstWorkEnd + alignment;
					}
					/* Size: 8Byte */
					while (noOfItems-- > 0)
					{
						pDstWorkEnd += 8;
					}
					break;
				}
				default:
					break;
			}
		}
	}
	/* Set workSize */
	workSize = (UINT32)((pDstWorkEnd - pDstEnd) - firstAlignment);
	/* Set workSize for case Dataset in Dataset */
	workSize = workSize + size;
	/* Return Dataset Size */
	*pDatasetSize = workSize;

	return TRDP_NO_ERR;
}
#endif
/******************************************************************************/
/** Size of Dataset writing in Traffic Store
 *
 *  @param[out]     pDatasetSize    Pointer Host Byte order of dataset size
 *  @param[in]      pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
TRDP_ERR_T sizeWriteDatasetInTrafficStore (
		UINT32					*pDatasetSize,
		TRDP_DATASET_T		*pDataset)
{
	TRDP_ERR_T err = TRDP_NO_ERR;
	UINT8 *pTempSrcDataset = NULL;
	UINT8 *pTempDestDataset = NULL;
	UINT32 datasetNetworkByteSize = 0;

	/* Create Temporary Source Dataset */
	pTempSrcDataset = (UINT8 *)vos_memAlloc(TRDP_MAX_MD_DATA_SIZE);
	if (pTempSrcDataset == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"sizeWriteDatasetInTrafficStore() Failed. Temporary Source Dataset vos_memAlloc() Err\n");
		/* Free Temporary Source Dataset */
		vos_memFree(pTempSrcDataset);
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Temporary Source Dataset */
		memset(pTempSrcDataset, 0, TRDP_MAX_MD_DATA_SIZE);
	}
	/* Create Temporary Destination Dataset */
	pTempDestDataset = (UINT8 *)vos_memAlloc(TRDP_MAX_MD_DATA_SIZE);
	if (pTempSrcDataset == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"sizeWriteDatasetInTrafficStore() Failed. Temporary Destination Dataset vos_memAlloc() Err\n");
		/* Free Temporary Source Dataset */
		vos_memFree(pTempDestDataset);
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Temporary Destination Dataset */
		memset(pTempDestDataset, 0, TRDP_MAX_MD_DATA_SIZE);
	}

	/* Compute Network Byte order of Dataset (size of marshalled dataset) */
	err = tau_calcDatasetSize(
			marshallConfig.pRefCon,
			pDataset->id,
			pTempSrcDataset,
			&datasetNetworkByteSize,
			&pDataset);
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "sizeWriteDatasetInTrafficStore() Failed. tau_calcDatasetSize datasetId: %d returns error = %d\n", pDataset->id, err);
		/* Free Temporary Source Dataset */
		vos_memFree(pTempSrcDataset);
		return TRDP_PARAM_ERR;
	}
	else
	{
		/* Set Network Byte order of Dataset */
		*pDatasetSize = datasetNetworkByteSize;
	}

	/* Get Host Byte order of Dataset Size(size of unmarshall dataset) by tau_unmarshallDs() */
	err = tau_unmarshallDs(
				&marshallConfig.pRefCon,			/* pointer to user context */
				pDataset->id,						/* datasetId */
				pTempSrcDataset,					/* source pointer to received original message */
				pTempDestDataset,					/* destination pointer to a buffer for the treated message */
				pDatasetSize,						/* destination Buffer Size */
				&pDataset);						/* pointer to pointer of cached dataset */
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "sizeWriteDatasetInTrafficStore() Failed. tau_unmarshallDs DatasetIdT%d returns error %d\n", pDataset->id, err);
		/* Free Temporary Source Dataset */
		vos_memFree(pTempDestDataset);
		return err;
	}
	/* Free Temporary Source Dataset */
	vos_memFree(pTempDestDataset);
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** Publisher Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex				interface Index
 *  @param[in]      pExchgPar			Pointer to Exchange Parameter
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T publishTelegram (
		UINT32				ifIndex,
		TRDP_EXCHG_PAR_T	*pExchgPar)
{
	UINT32					i = 0;
	TRDP_IP_ADDR_T		networkByteIpAddr = 0;				/* for convert URI to IP Address */
	PUBLISH_TELEGRAM_T	*pPublishTelegram = NULL;
	UINT32					*pPublishDataset = NULL;
	TRDP_ERR_T				err = TRDP_NO_ERR;
//	UINT8					dstend = 0;

	/* Get Publish Telegram memory area */
    pPublishTelegram = (PUBLISH_TELEGRAM_T *)vos_memAlloc(sizeof(PUBLISH_TELEGRAM_T));
	if (pPublishTelegram == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"publishTelegram() Failed. Publish Telegram vos_memAlloc() Err\n");
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Publish Telegram */
		memset(pPublishTelegram, 0, sizeof(PUBLISH_TELEGRAM_T));
	}

	/* Find ExchgPar Dataset for the dataset config */
	for (i = 0; i < numDataset; i++)
	{
		/* DatasetId in dataSet config ? */
		if (pExchgPar->datasetId == apDataset[i]->id)
		{
			/* Set Dataset Descriptor */
			pPublishTelegram->pDatasetDescriptor = apDataset[i];
			break;
		}
	}
	/* DataSetId effective ? */
	if (pPublishTelegram->pDatasetDescriptor == 0)
	{
		vos_printLog(VOS_LOG_ERROR,"publishTelegram() Failed. Dataset Err. datasetId: %d, comId: %d\n", pExchgPar->datasetId, pExchgPar->comId);
		/* Free Publish Telegram */
		vos_memFree(pPublishTelegram);
		return TRDP_PARAM_ERR;
	}

	/* Check dstCnt */
	if (pExchgPar->destCnt != 1)
	{
		vos_printLog(VOS_LOG_ERROR,"publishTelegram() Failed. dstCnt Err. destCnt: %d\n", pExchgPar->destCnt);
		/* Free Publish Telegram */
		vos_memFree(pPublishTelegram);
		return TRDP_PARAM_ERR;
	}
	else
	{
		/* Set Publish Telegram */
		/* Set Application Handle */
		if (ifIndex == IF_INDEX_SUBNET1)
		{
			/* Set Application Handle: Subnet1 */
			pPublishTelegram->appHandle = arraySessionConfig[ifIndex].sessionHandle;
		}
		else if(ifIndex == IF_INDEX_SUBNET2)
		{
			/* Set Application Handle: Subnet2 */
			pPublishTelegram->appHandle = arraySessionConfig[ifIndex].sessionHandle;
		}
		else
		{
			/* ifIndex Error */
			vos_printLog(VOS_LOG_ERROR, "publishTelegram() Failed. ifIndex:%d error\n", ifIndex);
			/* Free Publish Telegram */
			vos_memFree(pPublishTelegram);
			return TRDP_PARAM_ERR;
		}
		/* Set Dataset */
		/* Get Dataset Size */
//		err = sizeWriteDatasetInTrafficStore(&dstend, &pPublishTelegram->dataset.size, pPublishTelegram->pDatasetDescriptor);
		err = sizeWriteDatasetInTrafficStore(&pPublishTelegram->dataset.size, pPublishTelegram->pDatasetDescriptor);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "publishTelegram() Failed. sizeWriteDatasetInTrafficStore() returns error = %d\n", err);
			/* Free Publish Telegram */
			vos_memFree(pPublishTelegram);
			return TRDP_PARAM_ERR;
		}
		/* Create Dataset */
	    pPublishDataset = (UINT32 *)vos_memAlloc(pPublishTelegram->dataset.size);
		if (pPublishDataset == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"publishTelegram() Failed. Publish Dataset vos_memAlloc() Err\n");
			/* Free Publish Telegram */
			vos_memFree(pPublishTelegram);
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize Publish dataset */
			memset(pPublishDataset, 0, pPublishTelegram->dataset.size);
			/* Set tlp_publish() dataset size */
			pPublishTelegram->datasetNetworkByteSize = pPublishTelegram->dataset.size;
		}

		/* Enable Marshalling ? Check exchgPar and PD config */
		if (((pExchgPar->pPdPar->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
			|| ((arraySessionConfig[ifIndex].pdConfig.flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL))
		{
			/* Compute size of marshalled dataset */
			err = tau_calcDatasetSize(
					marshallConfig.pRefCon,
					pExchgPar->datasetId,
					(UINT8 *) pPublishDataset,
					&pPublishTelegram->datasetNetworkByteSize,
					&pPublishTelegram->pDatasetDescriptor);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "publishTelegram() Failed. tau_calcDatasetSize datasetId: %d returns error = %d\n", pExchgPar->datasetId, err);
				/* Free Publish Dataset */
				vos_memFree(pPublishDataset);
				/* Free Publish Telegram */
				vos_memFree(pPublishTelegram);
				return TRDP_PARAM_ERR;
			}
		}

		/* Set If Config */
		pPublishTelegram->pIfConfig = &pIfConfig[ifIndex];
		/* Set PD Parameter */
		pPublishTelegram->pPdParameter = pExchgPar->pPdPar;
		/* Set Dataset Buffer */
		pPublishTelegram->dataset.pDatasetStartAddr = (UINT8 *)pPublishDataset;
		/* Set comId */
		pPublishTelegram->comId = pExchgPar->comId;
		/* Set topoCount */
		pPublishTelegram->topoCount = 0;
		/* Set Source IP Address */
		/* Check Source IP Address exists ? */
		if (&pExchgPar->pSrc[0] != NULL)
		{
			/* Convert Source Host1 URI to IP Address */
			if (pExchgPar->pSrc[0].pUriHost1 != NULL)
			{
				networkByteIpAddr = vos_dottedIP (*(pExchgPar->pSrc[0].pUriHost1));
				if ((networkByteIpAddr == 0)
					|| (networkByteIpAddr == BROADCAST_ADDRESS)
					|| (vos_isMulticast(networkByteIpAddr)))
				{
					/* Free Publish Dataset */
					vos_memFree(pPublishDataset);
					/* Free Publish Telegram */
					vos_memFree(pPublishTelegram);
					vos_printLog(VOS_LOG_ERROR,"publishTelegram() Failed. Source IP Address1 Err. Source URI Host1: %s\n", (char *)pExchgPar->pSrc[0].pUriHost1);
					return TRDP_PARAM_ERR;
				}
				else
				{
					/* Set Source IP Address */
					pPublishTelegram->srcIpAddr = networkByteIpAddr;
				}
			}
		}
		else
		{
			if (ifIndex == 0)
			{
				/* Set Source IP Address : Subnet1 I/F Address */
				pPublishTelegram->srcIpAddr = subnetId1Address;
			}
			else
			{
				/* Set Source IP Address : Subnet2 I/F Address */
				pPublishTelegram->srcIpAddr = subnetId2Address;
			}
		}

		/* Set Destination IP Address */
		/* Check Destination IP Address exists ? */
		if (&pExchgPar->pDest != NULL)
		{
			/* Convert Host URI to IP Address */
			if (pExchgPar->pDest[0].pUriHost != NULL)
			{
				networkByteIpAddr = vos_dottedIP (*(pExchgPar->pDest[0].pUriHost));
				if (networkByteIpAddr == BROADCAST_ADDRESS)
				{
					vos_printLog(VOS_LOG_ERROR,"publishTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n", (char *)pExchgPar->pDest[ifIndex].pUriHost);
					/* Free Publish Dataset */
					vos_memFree(pPublishDataset);
					/* Free Publish Telegram */
					vos_memFree(pPublishTelegram);
					return TRDP_PARAM_ERR;
				}
				else
				{
					/* Set Destination IP Address */
					pPublishTelegram->dstIpAddr = networkByteIpAddr;
				}
			}
		}
		/* Set send Parameter */
		pPublishTelegram->pSendParam = &arraySessionConfig[ifIndex].pdConfig.sendParam;

		/* Publish */
		err = tlp_publish(
				pPublishTelegram->appHandle,									/* our application identifier */
				&pPublishTelegram->pubHandle,									/* our publish identifier */
				pPublishTelegram->comId,											/* ComID to send */
				pPublishTelegram->topoCount,									/* local consist only */
				pPublishTelegram->srcIpAddr,									/* default source IP */
				pPublishTelegram->dstIpAddr,									/* where to send to */
				pPublishTelegram->pPdParameter->cycle,							/* Cycle time in ms */
				pPublishTelegram->pPdParameter->redundant,					/* 0 - Non-redundant, > 0 valid redundancy group */
				pPublishTelegram->pPdParameter->flags,							/* flags */
				pPublishTelegram->pSendParam,									/* send Paramter */
				pPublishTelegram->dataset.pDatasetStartAddr,					/* initial data */
				pPublishTelegram->datasetNetworkByteSize);					/* data size */
		if (err != TRDP_NO_ERR)
		{
			/* Free Publish Dataset */
			vos_memFree(pPublishDataset);
			/* Free Publish Telegram */
			vos_memFree(pPublishTelegram);
			vos_printLog(VOS_LOG_ERROR, "publishTelegram() Failed. Publish Telegram tlp_publish() Err:%d\n", err);
			return err;
		}
		else
		{
			/* Append Publish Telegram */
			err = appendPublishTelegramList(&pHeadPublishTelegram, pPublishTelegram);
			if (err != TRDP_NO_ERR)
			{
				/* Free Publish Dataset */
				vos_memFree(pPublishDataset);
				/* Free Publish Telegram */
				vos_memFree(pPublishTelegram);
				vos_printLog(VOS_LOG_ERROR, "publishTelegram() Failed. Publish Telegram appendPublishTelegramList() Err:%d\n", err);
				return err;
			}
		}
	}
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** Subscriber Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex				interface Index
 *  @param[in]      numExchgPar			Number of Exchange Parameter
 *  @param[in]      pExchgPar			Pointer to Exchange Parameter
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T subscribeTelegram (
		UINT32				ifIndex,
		TRDP_EXCHG_PAR_T	*pExchgPar)
{
	UINT32						i = 0;
	UINT32						datasetIndex = 0;
	TRDP_IP_ADDR_T			networkByteIpAddr = 0;				/* for convert URI to IP Address */
	SUBSCRIBE_TELEGRAM_T		*pSubscribeTelegram = NULL;
	UINT32						*pSubscribeDataset = NULL;
	TRDP_ERR_T					err = TRDP_NO_ERR;
//	UINT8						dstend = 0;

	/* Check srcCnt */
	if (pExchgPar->srcCnt == 0)
	{
		vos_printLog(VOS_LOG_ERROR,"subscribeTelegram() Failed. srcCnt Err. srcCnt: %d\n", pExchgPar->srcCnt);
		return TRDP_PARAM_ERR;
	}
	else
	{
		/* Subscribe Loop */
		for (i = 0; i < pExchgPar->srcCnt; i++)
		{
			/* Get Subscribe Telegram memory area */
		    pSubscribeTelegram = (SUBSCRIBE_TELEGRAM_T *)vos_memAlloc(sizeof(SUBSCRIBE_TELEGRAM_T));
			if (pSubscribeTelegram == NULL)
			{
				vos_printLog(VOS_LOG_ERROR,"SubscribeTelegram() Failed. Subscribe Telegram vos_memAlloc() Err\n");
				return TRDP_MEM_ERR;
			}
			else
			{
				/* Initialize Subscribe Telegram */
				memset(pSubscribeTelegram, 0, sizeof(SUBSCRIBE_TELEGRAM_T));
			}

			/* First Time Setting */
			if (i == 0)
			{
				/* Find ExchgPar Dataset for the dataset config */
				for (datasetIndex = 0; datasetIndex < numDataset; datasetIndex++)
				{
					/* DatasetId in dataSet config ? */
					if (pExchgPar->datasetId == apDataset[datasetIndex]->id)
					{
						/* Set Dataset Descriptor */
						pSubscribeTelegram->pDatasetDescriptor = apDataset[datasetIndex];
						break;
					}
				}
				/* DataSetId effective ? */
				if (pSubscribeTelegram->pDatasetDescriptor == 0)
				{
					vos_printLog(VOS_LOG_ERROR,"subscribeTelegram() Failed. Dataset Err. datasetId: %d, comId: %d\n", pExchgPar->datasetId, pExchgPar->comId);
					/* Free Subscribe Telegram */
					vos_memFree(pSubscribeTelegram);
					return TRDP_PARAM_ERR;
				}

				/* Check dstCnt */
				if (pExchgPar->destCnt != 1)
				{
					vos_printLog(VOS_LOG_ERROR,"subscribeTelegram() Failed. destCnt Err. destCnt: %d\n", pExchgPar->destCnt);
					/* Free Subscribe Telegram */
					vos_memFree(pSubscribeTelegram);
					return TRDP_PARAM_ERR;
				}
				else
				{
					/* Set Destination IP Address */
					/* Check Destination IP Address exists ? */
					if (&pExchgPar->pDest[0] != NULL)
					{
						/* Convert Host URI to IP Address */
						if (pExchgPar->pDest[0].pUriHost != NULL)
						{
							networkByteIpAddr = vos_dottedIP (*(pExchgPar->pDest[0].pUriHost));
							/* Is destination multicast ? */
							if (vos_isMulticast(networkByteIpAddr))
							{
								/* Set destination: multicast */
								pSubscribeTelegram->dstIpAddr = networkByteIpAddr;
							}
							/* destination Broadcast ? */
							else if (networkByteIpAddr == BROADCAST_ADDRESS)
							{
								vos_printLog(VOS_LOG_ERROR,"subscribeTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n", (char *)pExchgPar->pDest[ifIndex].pUriHost);
								/* Free Subscribe Telegram */
								vos_memFree(pSubscribeTelegram);
								return TRDP_PARAM_ERR;
							}
							/* destination 0 ? */
							else if (networkByteIpAddr == 0)
							{
								/* Is I/F subnet1 ? */
								if (ifIndex == IF_INDEX_SUBNET1)
								{
									/* Set destination: Subnet1 I/F Address */
									if (pIfConfig[ifIndex].hostIp == IP_ADDRESS_NOTHING)
									{
										/* Set Real I/F Address */
										pSubscribeTelegram->dstIpAddr = subnetId1Address;
									}
									else
									{
										/* Set I/F Config Address */
										pSubscribeTelegram->dstIpAddr = pIfConfig[ifIndex].hostIp;
									}
								}
								else if(ifIndex == IF_INDEX_SUBNET2)
								{
									/* Set destination: Subnet2 I/F Address */
									if (pIfConfig[ifIndex].hostIp == IP_ADDRESS_NOTHING)
									{
										/* Set Real I/F Address */
										pSubscribeTelegram->dstIpAddr = subnetId2Address;
									}
									else
									{
										/* Set I/F Config Address */
										pSubscribeTelegram->dstIpAddr = pIfConfig[ifIndex].hostIp;
									}
								}
								else
								{
									vos_printLog(VOS_LOG_ERROR,"subscribeTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n", (char *)pExchgPar->pDest[ifIndex].pUriHost);
									/* Free Subscribe Telegram */
									vos_memFree(pSubscribeTelegram);
									return TRDP_PARAM_ERR;
								}
							}
							/* destination unicast */
							else
							{
								/* Set Destination IP Address */
								pSubscribeTelegram->dstIpAddr = networkByteIpAddr;
							}
						}
					}
				}
			}

			/* Set Subscribe Telegram */
			/* Set Application Handle */
			if (ifIndex == IF_INDEX_SUBNET1)
			{
				/* Set Application Handle: Subnet1 */
				pSubscribeTelegram->appHandle = arraySessionConfig[ifIndex].sessionHandle;
			}
			else if(ifIndex == IF_INDEX_SUBNET2)
			{
				/* Set Application Handle: Subnet2 */
				pSubscribeTelegram->appHandle = arraySessionConfig[ifIndex].sessionHandle;
			}
			else
			{
				/* ifIndex Error */
				vos_printLog(VOS_LOG_ERROR, "subscribeTelegram() Failed. ifIndex:%d error\n", ifIndex);
				/* Free Subscribe Telegram */
				vos_memFree(pSubscribeTelegram);
				return TRDP_PARAM_ERR;
			}
			/* Set Dataset */
			/* Get Dataset Size */
//			err = sizeWriteDatasetInTrafficStore(&dstend, &pSubscribeTelegram->dataset.size, pSubscribeTelegram->pDatasetDescriptor);
			err = sizeWriteDatasetInTrafficStore(&pSubscribeTelegram->dataset.size, pSubscribeTelegram->pDatasetDescriptor);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "subscribeTelegram() Failed. sizeWriteDatasetInTrafficStore() returns error = %d\n", err);
				/* Free Subscribe Telegram */
				vos_memFree(pSubscribeTelegram);
				return TRDP_PARAM_ERR;
			}
			/* Create Dataset */
			pSubscribeDataset = (UINT32 *)vos_memAlloc(pSubscribeTelegram->dataset.size);
			if (pSubscribeDataset == NULL)
			{
				vos_printLog(VOS_LOG_ERROR,"subscribeTelegram() Failed. Subscribe Dataset vos_memAlloc() Err\n");
				/* Free Subscribe Telegram */
				vos_memFree(pSubscribeTelegram);
				return TRDP_MEM_ERR;
			}
			else
			{
				/* Initialize Subscribe dataset */
				memset(pSubscribeDataset, 0, pSubscribeTelegram->dataset.size);
			}

			/* Enable Marshalling ? Check exchgPar and PD config */
			if (((pExchgPar->pPdPar->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
				|| ((arraySessionConfig[ifIndex].pdConfig.flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL))
			{
				/* Compute size of marshalled dataset */
				err = tau_calcDatasetSize(
						marshallConfig.pRefCon,
						pExchgPar->datasetId,
						(UINT8 *) pSubscribeDataset,
						&pSubscribeTelegram->datasetNetworkByteSize,
						&pSubscribeTelegram->pDatasetDescriptor);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "subscribeTelegram() Failed. tau_calcDatasetSize datasetId: %d returns error = %d\n", pExchgPar->datasetId, err);
					/* Free Subscribe Dataset */
					vos_memFree(pSubscribeDataset);
					/* Free Subscribe Telegram */
					vos_memFree(pSubscribeTelegram);
					return TRDP_PARAM_ERR;
				}
			}

			/* Set Dataset Buffer */
			pSubscribeTelegram->dataset.pDatasetStartAddr = (UINT8 *)pSubscribeDataset;
			/* Set If Config */
			pSubscribeTelegram->pIfConfig = &pIfConfig[ifIndex];
			/* Set PD Parameter */
			pSubscribeTelegram->pPdParameter = pExchgPar->pPdPar;
			/* Set comId */
			pSubscribeTelegram->comId = pExchgPar->comId;
			/* Set topoCount */
			pSubscribeTelegram->topoCount = 0;

			/* Set Source IP Address */
			/* Check Source IP Address exists ? */
			if (&pExchgPar->pSrc[0] != NULL)
			{
				/* Convert Source Host1 URI to IP Address */
				if (pExchgPar->pSrc[0].pUriHost1 != NULL)
				{
					networkByteIpAddr = vos_dottedIP (*(pExchgPar->pSrc[0].pUriHost1));
					if ((networkByteIpAddr == 0)
						|| (vos_isMulticast(networkByteIpAddr)))
					{
						/* Free Subscribe Dataset */
						vos_memFree(pSubscribeDataset);
						/* Free Subscribe Telegram */
						vos_memFree(pSubscribeTelegram);
						vos_printLog(VOS_LOG_ERROR,"subscribeTelegram() Failed. Source IP Address1 Err. Source URI Host1: %s\n", (char *)pExchgPar->pSrc[0].pUriHost1);
						return TRDP_PARAM_ERR;
					}
					/* 255.255.255.255 mean Not Source IP Filter */
					else if (networkByteIpAddr == BROADCAST_ADDRESS)
					{
						/* Set Not Source IP Filter */
						pSubscribeTelegram->srcIpAddr1 = IP_ADDRESS_NOTHING;
					}
					else
					{
						/* Set Source IP Address1 */
						pSubscribeTelegram->srcIpAddr1 = networkByteIpAddr;
					}
				}
				/* Convert Source Host2 URI to IP Address */
				if (pExchgPar->pSrc[0].pUriHost2 != NULL)
				{
					networkByteIpAddr = vos_dottedIP (*(pExchgPar->pSrc[0].pUriHost2));
					if ((networkByteIpAddr == 0)
						|| (vos_isMulticast(networkByteIpAddr)))
					{
						/* Free Subscribe Dataset */
						vos_memFree(pSubscribeDataset);
						/* Free Subscribe Telegram */
						vos_memFree(pSubscribeTelegram);
						vos_printLog(VOS_LOG_ERROR,"subscribeTelegram() Failed. Source IP Address2 Err. Source URI Host2: %s\n", (char *)pExchgPar->pSrc[0].pUriHost2);
						return TRDP_PARAM_ERR;
					}
					/* 255.255.255.255 mean Not Source IP Filter */
					else if (networkByteIpAddr == BROADCAST_ADDRESS)
					{
						/* Set Not Source IP Filter */
						pSubscribeTelegram->srcIpAddr1 = IP_ADDRESS_NOTHING;
					}
					else
					{
						/* Set Source IP Address2 */
						pSubscribeTelegram->srcIpAddr2 = networkByteIpAddr;
					}
				}
			}
			/* Set user reference */
			pSubscribeTelegram->pUserRef = &pSubscribeTelegram->pPdParameter->offset;

			/* Subscribe */
			err = tlp_subscribe(
					pSubscribeTelegram->appHandle,									/* our application identifier */
					&pSubscribeTelegram->subHandle,									/* our subscription identifier */
					pSubscribeTelegram->pUserRef,									/* user reference value = offset */
					pSubscribeTelegram->comId,           							/* ComID */
					pSubscribeTelegram->topoCount,        							/* topocount: local consist only */
					pSubscribeTelegram->srcIpAddr1,									/* Source IP filter */
					pSubscribeTelegram->srcIpAddr2,                     			/* Source IP filter2 : no used */
					pSubscribeTelegram->dstIpAddr,									/* Default destination	(or MC Group) */
					pSubscribeTelegram->pPdParameter->flags,						/* Option */
					pSubscribeTelegram->pPdParameter->timeout,					/* Time out in us	*/
					pSubscribeTelegram->pPdParameter->toBehav,   					/* delete invalid data on timeout */
					pSubscribeTelegram->dataset.size);								/* net data size */
			if (err != TRDP_NO_ERR)
			{
				/* Free Subscribe Dataset */
				vos_memFree(pSubscribeDataset);
				/* Free Subscribe Telegram */
				vos_memFree(pSubscribeTelegram);
				vos_printLog(VOS_LOG_ERROR, "subscribeTelegram() Failed. Subscribe Telegram tlp_subscribe() Err:%d\n", err);
				return err;
			}
			else
			{
				/* Append Subscribe Telegram */
				err = appendSubscribeTelegramList(&pHeadSubscribeTelegram, pSubscribeTelegram);
				if (err != TRDP_NO_ERR)
				{
					/* Free Subscribe Dataset */
					vos_memFree(pSubscribeDataset);
					/* Free Subscribe Telegram */
					vos_memFree(pSubscribeTelegram);
					vos_printLog(VOS_LOG_ERROR, "subscribeTelegram() Failed. Subscribe Telegram appendSubscribeTelegramList() Err:%d\n", err);
					return err;
				}
			}
		}
	}
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** PD Requester Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex				interface Index
 *  @param[in]      numExchgPar			Number of Exchange Parameter
 *  @param[in]      pExchgPar			Pointer to Exchange Parameter
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T pdRequestTelegram (
		UINT32				ifIndex,
		TRDP_EXCHG_PAR_T	*pExchgPar)
{
	UINT32						i = 0;
	TRDP_IP_ADDR_T			networkByteIpAddr = 0;				/* for convert URI to IP Address */
	PD_REQUEST_TELEGRAM_T	*pPdRequestTelegram = NULL;
	UINT32						*pPdRequestDataset = NULL;
	TRDP_ERR_T					err = TRDP_NO_ERR;
	SUBSCRIBE_TELEGRAM_T		*pTailSubscribeTelegram = NULL;
//	UINT8						dstend = 0;

	/* Check srcCnt */
	if (pExchgPar->srcCnt == 0)
	{
		vos_printLog(VOS_LOG_ERROR,"pdRequestTelegram() Failed. srcCnt Err. srcCnt: %d\n", pExchgPar->srcCnt);
		return TRDP_PARAM_ERR;
	}
	else
	{
		/* Get PD Request Telegram memory area */
	    pPdRequestTelegram = (PD_REQUEST_TELEGRAM_T *)vos_memAlloc(sizeof(PD_REQUEST_TELEGRAM_T));
		if (pPdRequestTelegram == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"pdRequestTelegram() Failed. PD Request Telegram vos_memAlloc() Err\n");
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize PD Request Telegram */
			memset(pPdRequestTelegram, 0, sizeof(PD_REQUEST_TELEGRAM_T));
		}

		/* Find ExchgPar Dataset for the dataset config */
		for (i = 0; i < numDataset; i++)
		{
			/* DatasetId in dataSet config ? */
			if (pExchgPar->datasetId == apDataset[i]->id)
			{
				/* Set Dataset Descriptor */
				pPdRequestTelegram->pDatasetDescriptor = apDataset[i];
				break;
			}
		}
		/* DataSetId effective ? */
		if (pPdRequestTelegram->pDatasetDescriptor == 0)
		{
			vos_printLog(VOS_LOG_ERROR,"pdRequestTelegram() Failed. Dataset Err. datasetId: %d, comId: %d\n", pExchgPar->datasetId, pExchgPar->comId);
			/* Free PD Request Telegram */
			vos_memFree(pPdRequestTelegram);
			return TRDP_PARAM_ERR;
		}

		/* Get Request Source IP Address */
		/* Check Source IP Address exists ? */
		if (&pExchgPar->pSrc[0] != NULL)
		{
			/* Convert Source Host1 URI to IP Address */
			if (pExchgPar->pSrc[0].pUriHost1 != NULL)
			{
				networkByteIpAddr = vos_dottedIP (*(pExchgPar->pSrc[0].pUriHost1));
				if (networkByteIpAddr == 0)
				{
					/* Is I/F subnet1 ? */
					if (ifIndex == IF_INDEX_SUBNET1)
					{
						/* Set Source IP Address : Subnet1 I/F Address */
						if (pIfConfig[ifIndex].hostIp == IP_ADDRESS_NOTHING)
						{
							/* Set Real I/F Address */
							pPdRequestTelegram->srcIpAddr = subnetId1Address;
						}
						else
						{
							/* Set I/F Config Address */
							pPdRequestTelegram->srcIpAddr = pIfConfig[ifIndex].hostIp;
						}
					}
					else if(ifIndex == IF_INDEX_SUBNET2)
					{
						/* Set Source IP Address : Subnet2 I/F Address */
						if (pIfConfig[ifIndex].hostIp == IP_ADDRESS_NOTHING)
						{
							/* Set Real I/F Address */
							pPdRequestTelegram->srcIpAddr = subnetId2Address;
						}
						else
						{
							/* Set I/F Config Address */
							pPdRequestTelegram->srcIpAddr = pIfConfig[ifIndex].hostIp;
						}
					}
					else
					{
						vos_printLog(VOS_LOG_ERROR,"pdRequestTelegram() Failed. Source IP Address Err. Source URI Host: %s\n", (char *)pExchgPar->pSrc[0].pUriHost1);
						/* Free PD Request Telegram */
						vos_memFree(pPdRequestTelegram);
						return TRDP_PARAM_ERR;
					}
				}
				else
				{
					/* Free Subscribe Telegram */
					vos_memFree(pPdRequestTelegram);
					vos_printLog(VOS_LOG_ERROR,"subscribeTelegram() Failed. Source IP Address1 Err. Source URI Host1: %s\n", (char *)pExchgPar->pSrc[0].pUriHost1);
					return TRDP_PARAM_ERR;
				}
			}
		}

		/* Check dstCnt */
		if (pExchgPar->destCnt < 1)
		{
			vos_printLog(VOS_LOG_ERROR,"pdRequestTelegram() Failed. destCnt Err. destCnt: %d\n", pExchgPar->destCnt);
			/* Free PD Request Telegram */
			vos_memFree(pPdRequestTelegram);
			return TRDP_PARAM_ERR;
		}
		else
		{
			/* Get Request Destination Address */
			/* Check Source IP Address exists ? */
			if (&pExchgPar->pDest[0] != NULL)
			{
				/* Convert Host URI to IP Address */
				if (pExchgPar->pDest[0].pUriHost != NULL)
				{
					networkByteIpAddr = vos_dottedIP (*(pExchgPar->pDest[0].pUriHost));
					/* Is destination multicast ? */
					if (vos_isMulticast(networkByteIpAddr))
					{
						/* Set destination: multicast */
						pPdRequestTelegram->dstIpAddr = networkByteIpAddr;
					}
					/* destination Broadcast or 0 ? */
					else if ((networkByteIpAddr == BROADCAST_ADDRESS)
						|| (networkByteIpAddr == 0))
					{
						vos_printLog(VOS_LOG_ERROR,"pdRequestTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n", (char *)pExchgPar->pDest[ifIndex].pUriHost);
						/* Free PD Request Telegram */
						vos_memFree(pPdRequestTelegram);
						return TRDP_PARAM_ERR;
					}
					/* destination unicast */
					else
					{
						/* Set Destination IP Address */
						pPdRequestTelegram->dstIpAddr = networkByteIpAddr;
					}
				}
			}

			/* Get Reply Address */
			if (pExchgPar->destCnt <= 2)
			{
				/* Check Source IP Address exists ? */
				if (&pExchgPar->pDest[1] != NULL)
				{
					/* Convert Host URI to IP Address */
					if (pExchgPar->pDest[1].pUriHost != NULL)
					{
						networkByteIpAddr = vos_dottedIP (*(pExchgPar->pDest[1].pUriHost));
						/* Is destination multicast ? */
						if (vos_isMulticast(networkByteIpAddr))
						{
							/* Set destination: multicast */
							pPdRequestTelegram->replyIpAddr = networkByteIpAddr;
						}
						/* destination Broadcast or 0 ? */
						else if ((networkByteIpAddr == BROADCAST_ADDRESS)
							|| (networkByteIpAddr == 0))
						{
							vos_printLog(VOS_LOG_ERROR,"pdRequestTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n", (char *)pExchgPar->pDest[ifIndex].pUriHost);
							/* Free PD Request Telegram */
							vos_memFree(pPdRequestTelegram);
							return TRDP_PARAM_ERR;
						}
						/* destination unicast */
						else
						{
							/* Set Destination IP Address */
							pPdRequestTelegram->replyIpAddr = networkByteIpAddr;
						}
					}
				}
			}

			/* Set PD Request Telegram */
			/* Set Application Handle */
			if (ifIndex == IF_INDEX_SUBNET1)
			{
				/* Set Application Handle: Subnet1 */
				pPdRequestTelegram->appHandle = arraySessionConfig[ifIndex].sessionHandle;
			}
			else if(ifIndex == IF_INDEX_SUBNET2)
			{
				/* Set Application Handle: Subnet2 */
				pPdRequestTelegram->appHandle = arraySessionConfig[ifIndex].sessionHandle;
			}
			else
			{
				/* ifIndex Error */
				vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. ifIndex:%d error\n", ifIndex);
				/* Free PD Request Telegram */
				vos_memFree(pPdRequestTelegram);
				return TRDP_PARAM_ERR;
			}
			/* Set Dataset */
			/* Get Dataset Size */
//			err = sizeWriteDatasetInTrafficStore(&dstend, &pPdRequestTelegram->dataset.size, pPdRequestTelegram->pDatasetDescriptor);
			err = sizeWriteDatasetInTrafficStore(&pPdRequestTelegram->dataset.size, pPdRequestTelegram->pDatasetDescriptor);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. sizeWriteDatasetInTrafficStore() returns error = %d\n", err);
				/* Free PD Request Telegram */
				vos_memFree(pPdRequestTelegram);
				return TRDP_PARAM_ERR;
			}
			/* Create Dataset */
		    pPdRequestDataset = (UINT32 *)vos_memAlloc(pPdRequestTelegram->dataset.size);
			if (pPdRequestDataset == NULL)
			{
				vos_printLog(VOS_LOG_ERROR,"pdRequestTelegram() Failed. PD Request Dataset vos_memAlloc() Err\n");
				/* Free PD Request Telegram */
				vos_memFree(pPdRequestTelegram);
				return TRDP_MEM_ERR;
			}
			else
			{
				/* Initialize PD Request dataset */
				memset(pPdRequestDataset, 0, pPdRequestTelegram->dataset.size);
				/* Set tlp_request() dataset size */
				pPdRequestTelegram->datasetNetworkByteSize = pPdRequestTelegram->dataset.size;
			}

			/* Enable Marshalling ? Check exchgPar and PD config */
			if (((pExchgPar->pPdPar->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
				|| ((arraySessionConfig[ifIndex].pdConfig.flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL))
			{
				/* Compute size of marshalled dataset */
				err = tau_calcDatasetSize(
						marshallConfig.pRefCon,
						pExchgPar->datasetId,
						(UINT8 *) pPdRequestTelegram,
						&pPdRequestTelegram->datasetNetworkByteSize,
						&pPdRequestTelegram->pDatasetDescriptor);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. tau_calcDatasetSize datasetId: %d returns error = %d\n", pExchgPar->datasetId, err);
					/* Free PD Request Dataset */
					vos_memFree(pPdRequestDataset);
					/* Free PD Request Telegram */
					vos_memFree(pPdRequestTelegram);
					return TRDP_PARAM_ERR;
				}
			}

			/* Set If Config */
			pPdRequestTelegram->pIfConfig = &pIfConfig[ifIndex];
			/* Set PD Parameter */
			pPdRequestTelegram->pPdParameter = pExchgPar->pPdPar;
			/* Set Dataset Buffer */
			pPdRequestTelegram->dataset.pDatasetStartAddr = (UINT8 *)pPdRequestDataset;
			/* Set comId */
			pPdRequestTelegram->comId = pExchgPar->comId;
			/* Set topoCount */
			pPdRequestTelegram->topoCount = 0;
			/* Set send Parameter */
			pPdRequestTelegram->pSendParam = &arraySessionConfig[ifIndex].pdConfig.sendParam;
			/* Set Subscribe Handle */
			/* Get End of Subscribe Telegram List */
			pTailSubscribeTelegram = getTailSubscribeTelegram();
			if (pTailSubscribeTelegram == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. Subscribe Handle error.\n");
				/* Free PD Request Dataset */
				vos_memFree(pPdRequestDataset);
				/* Free PD Request Telegram */
				vos_memFree(pPdRequestTelegram);
				return TRDP_PARAM_ERR;
			}
			else
			{
				/* Set Subacribe Handle */
				pPdRequestTelegram->subHandle = pTailSubscribeTelegram->subHandle;
				/* Set Reply ComId */
				pPdRequestTelegram->replyComId = pTailSubscribeTelegram->comId;
			}
			/* PD Request */
			err = tlp_request(
					pPdRequestTelegram->appHandle,						/* our application identifier */
					pPdRequestTelegram->subHandle,						/* our subscribe identifier */
					pPdRequestTelegram->comId,							/* ComID to send */
					pPdRequestTelegram->topoCount,						/* local consist only */
					pPdRequestTelegram->srcIpAddr,						/* default source IP */
					pPdRequestTelegram->dstIpAddr,						/* where to send to */
					pPdRequestTelegram->pPdParameter->redundant,		/* 0 - Non-redundant, > 0 valid redundancy group */
					pPdRequestTelegram->pPdParameter->flags,			/* flags */
					pPdRequestTelegram->pSendParam,						/* send Paramter */
					pPdRequestTelegram->dataset.pDatasetStartAddr,	/* request data */
					pPdRequestTelegram->datasetNetworkByteSize,		/* data size */
					pPdRequestTelegram->replyComId,						/* comId of reply */
					pPdRequestTelegram->replyIpAddr);					/* IP for reply */
			if (err != TRDP_NO_ERR)
			{
				/* Free PD Request Dataset */
				vos_memFree(pPdRequestDataset);
				/* Free Publish Telegram */
				vos_memFree(pPdRequestTelegram);
				vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. PD Request Telegram tlp_request() Err:%d\n", err);
				return err;
			}
			else
			{
				/* Append PD Request Telegram */
				err = appendPdRequestTelegramList(&pHeadPdRequestTelegram, pPdRequestTelegram);
				if (err != TRDP_NO_ERR)
				{
					/* Free PD Request Dataset */
					vos_memFree(pPdRequestDataset);
					/* Free Publish Telegram */
					vos_memFree(pPdRequestTelegram);
					vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. PD Request Telegram appendPdRequestTelegramList() Err:%d\n", err);
					return err;
				}
			}
		}
	}
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** MD Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex								interface Index
 *  @param[in]      numExchgPar							Number of Exchange Parameter
 *  @param[in]      pExchgPar							Pointer to Exchange Parameter
 *  @param[in]      pGetTelegramStatusFlag			Pointer to Get Telegram Status Flag
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T mdTelegram (
		UINT32				ifIndex,
		TRDP_EXCHG_PAR_T	*pExchgPar,
		UINT32				*pGetTelegramStatusFlag)
{
	TRDP_ERR_T					err = TRDP_NO_ERR;

	/* Check Parameter */
	if (pGetTelegramStatusFlag == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "mdTelegram() Failed. pGetTelegramStarusFlag Err.d\n");
		return TRDP_PARAM_ERR;
	}

	/* Check Flag */
	switch (*pGetTelegramStatusFlag)
	{
		/* Get Telegram First Time ? */
		case TRDP_GET_NONE:
		/* Get Caller-MnMr Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MN_MR:
		/* Get Caller-Mp Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MP:
		/* Get Caller-Mq Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MQ:
		/* Get Callerr-Mc Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MC:
			/* Check dstCnt */
			if (pExchgPar->destCnt > 0)
			{
				/* Replier Telegram */
				err = replierTelegram(ifIndex, pExchgPar, pGetTelegramStatusFlag);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "mdTelegram() Failed. replierTelegram() Err:%d\n", err);
					return err;
				}
			}
			else
			{
				/* Caller Telegram */
				err = callerTelegram(ifIndex, pExchgPar, pGetTelegramStatusFlag);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "mdTelegram() Failed. callerTelegram() Err:%d\n", err);
					return err;
				}
			}
		break;
		/* Get Replier-MnMr Telegram Last Time ? */
		case TRDP_GOTTEN_REPLIER_MN_MR:
		/* Get Replier-Mp Telegram Last Time ? */
		case TRDP_GOTTEN_REPLIER_MP:
		/* Get Replier-Mq Telegram Last Time ? */
		case TRDP_GOTTEN_REPLIER_MQ:
		/* Get Replier-Mc Telegram Last Time ? */
		case TRDP_GOTTEN_REPLIER_MC:
			/* Replier Telegram */
			err = replierTelegram(ifIndex, pExchgPar, pGetTelegramStatusFlag);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "mdTelegram() Failed. replierTelegram() Err:%d\n", err);
				return err;
			}
		break;
		default:
			vos_printLog(VOS_LOG_ERROR, "mdTelegram() Failed. Get Telegram Status Flag Err\n");
			return TRDP_PARAM_ERR;
		break;
	}
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** Replier Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex								interface Index
 *  @param[in]      numExchgPar							Number of Exchange Parameter
 *  @param[in]      pExchgPar							Pointer to Exchange Parameter
 *  @param[in]      pGetTelegramStatusFlag			Pointer to Get Telegram Status Flag
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T replierTelegram (
		UINT32				ifIndex,
		TRDP_EXCHG_PAR_T	*pExchgPar,
		UINT32				*pGetTelegramStatusFlag)
{
	UINT32							i = 0;
	TRDP_IP_ADDR_T				networkByteIpAddr = 0;				/* for convert URI to IP Address */
	REPLIER_TELEGRAM_T			*pReplierTelegram = NULL;
	UINT32							*pReplierDataset = NULL;
	WAITING_RECEIVE_REQUEST_T	*pWaitingReceiveRequestReference = NULL;
	TRDP_ERR_T						err = TRDP_NO_ERR;
	TRDP_DEST_T					defaultDestination = {0};				/* Destination Parameter (id, SDT, URI) */
	LISTENER_HANDLE_T    		*pNewListenerHandle = NULL;
	WAITING_RECEIVE_CONFIRM_T	*pNewWaitingReceiveConfirmReference = NULL;		/* Pointer to Waiting Receive Confirm Reference */
//	UINT8							dstend = 0;

	/* Check Parameter */
	if (pGetTelegramStatusFlag == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "replierTelegram() Failed. Get Telegram Status Flag Err\n");
		return TRDP_PARAM_ERR;
	}

	/* Replier Telegram */
	/* Get Replier Telegram memory area */
	pReplierTelegram = (REPLIER_TELEGRAM_T *)vos_memAlloc(sizeof(REPLIER_TELEGRAM_T));
	if (pReplierTelegram == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"replierTelegram() Failed. Replier Telegram vos_memAlloc() Err\n");
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Replier Telegram */
		memset(pReplierTelegram, 0, sizeof(REPLIER_TELEGRAM_T));
	}

	/* Find ExchgPar Dataset for the dataset config */
	for (i = 0; i < numDataset; i++)
	{
		/* DatasetId in dataSet config ? */
		if (pExchgPar->datasetId == apDataset[i]->id)
		{
			/* Set Dataset Descriptor */
			pReplierTelegram->pDatasetDescriptor = apDataset[i];
			break;
		}
	}
	/* DataSetId effective ? */
	if (pReplierTelegram->pDatasetDescriptor == 0)
	{
		vos_printLog(VOS_LOG_ERROR,"replierTelegram() Failed. Dataset Err. datasetId: %d, comId: %d\n", pExchgPar->datasetId, pExchgPar->comId);
		/* Free Replier Telegram */
		vos_memFree(pReplierTelegram);
		return TRDP_PARAM_ERR;
	}

	/* Set Replier Telegram */
	/* Set Destination IP Address */
	/* Check Destination IP Address exists ? */
	if (&pExchgPar->pDest[0] != NULL)
	{
		/* Convert Host URI to IP Address */
		if (pExchgPar->pDest[0].pUriHost != NULL)
		{
			networkByteIpAddr = vos_dottedIP (*(pExchgPar->pDest[0].pUriHost));
			/* Is destination multicast ? */
			if (vos_isMulticast(networkByteIpAddr))
			{
				/* Set destination: multicast */
				pReplierTelegram->pDestination = &pExchgPar->pDest[0];
			}
			/* destination Broadcast ? */
			else if (networkByteIpAddr == BROADCAST_ADDRESS)
			{
				vos_printLog(VOS_LOG_ERROR,"replierTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n", (char *)pExchgPar->pDest[ifIndex].pUriHost);
				/* Free Replier Telegram */
				vos_memFree(pReplierTelegram);
				return TRDP_PARAM_ERR;
			}
			/* destination 0 ? */
			else if (networkByteIpAddr == 0)
			{
				/* Set destination: 0.0.0.0 */
				pReplierTelegram->pDestination = &pExchgPar->pDest[0];
			}
			/* destination unicast */
			else
			{
				/* Set destination: unicast */
				pReplierTelegram->pDestination = &pExchgPar->pDest[0];
			}
		}
	}
	else
	{
		/* Set destination: 0.0.0.0 */
		pReplierTelegram->pDestination = &defaultDestination;
	}
	/* Set Application Handle */
	if (ifIndex == IF_INDEX_SUBNET1)
	{
		/* Set Application Handle: Subnet1 */
		pReplierTelegram->appHandle = arraySessionConfig[ifIndex].sessionHandle;
	}
	else if(ifIndex == IF_INDEX_SUBNET2)
	{
		/* Set Application Handle: Subnet2 */
		pReplierTelegram->appHandle = arraySessionConfig[ifIndex].sessionHandle;
	}
	else
	{
		/* ifIndex Error */
		vos_printLog(VOS_LOG_ERROR, "replierTelegram() Failed. ifIndex:%d error\n", ifIndex);
		/* Free Replier Telegram */
		vos_memFree(pReplierTelegram);
		return TRDP_PARAM_ERR;
	}
	/* Set Dataset */
	/* Get Dataset Size */
//	err = sizeWriteDatasetInTrafficStore(&dstend, &pReplierTelegram->dataset.size, pReplierTelegram->pDatasetDescriptor);
	err = sizeWriteDatasetInTrafficStore(&pReplierTelegram->dataset.size, pReplierTelegram->pDatasetDescriptor);
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "replierTelegram() Failed. sizeWriteDatasetInTrafficStore() returns error = %d\n", err);
		/* Free Replier Telegram */
		vos_memFree(pReplierTelegram);
		return TRDP_PARAM_ERR;
	}
	/* Create Dataset */
	pReplierDataset = (UINT32 *)vos_memAlloc(pReplierTelegram->dataset.size);
	if (pReplierDataset == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"replierTelegram() Failed. Replier Dataset vos_memAlloc() Err\n");
		/* Free Replier Telegram */
		vos_memFree(pReplierTelegram);
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Replier dataset */
		memset(pReplierDataset, 0, pReplierTelegram->dataset.size);
	}
	/* Enable Marshalling ? Check exchgPar and MD config */
	if (((pExchgPar->pMdPar->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
		|| ((arraySessionConfig[ifIndex].mdConfig.flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL))
	{
		/* Compute size of marshalled dataset */
		err = tau_calcDatasetSize(
				marshallConfig.pRefCon,
				pExchgPar->datasetId,
				(UINT8 *) pReplierDataset,
				&pReplierTelegram->datasetNetworkByteSize,
				&pReplierTelegram->pDatasetDescriptor);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "replierTelegram() Failed. tau_calcDatasetSize datasetId: %d returns error = %d\n", pExchgPar->datasetId, err);
			/* Free Replier Dataset */
			vos_memFree(pReplierDataset);
			/* Free Replier Telegram */
			vos_memFree(pReplierTelegram);
			return TRDP_PARAM_ERR;
		}
	}
	/* Set Dataset Buffer */
	pReplierTelegram->dataset.pDatasetStartAddr = (UINT8 *)pReplierDataset;
	/* Set If Config */
	pReplierTelegram->pIfConfig = &pIfConfig[ifIndex];
	/* Set MD Parameter */
	pReplierTelegram->pMdParameter = pExchgPar->pMdPar;
	/* Set comId */
	pReplierTelegram->comId = pExchgPar->comId;
	/* Set topoCount */
	pReplierTelegram->topoCount = 0;
	/* Set Source IP Address */
	pReplierTelegram->pSource = &pExchgPar->pSrc[0];
	/* Set user reference */
	/* Get AddListener Reference memory area */
	pAddListenerReference = (UINT32 *)vos_memAlloc(sizeof(UINT32));
	if (pAddListenerReference == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"replierTelegram() Failed. Add Listener Reference vos_memAlloc() Err\n");
		/* Free Replier Dataset */
		vos_memFree(pReplierDataset);
		/* Free Replier Dataset */
		vos_memFree(pReplierDataset);
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Add Listener Reference */
		memset(pAddListenerReference, 0, sizeof(UINT32));
	}
	/* Count up */
	listenerCounter++;
	memcpy(pAddListenerReference, &listenerCounter, sizeof(UINT32));
	pReplierTelegram->pUserRef = pAddListenerReference;

	/* Change Status */
	switch (*pGetTelegramStatusFlag)
	{
		/* Get Telegram First Time ? */
		case TRDP_GET_NONE:
		/* Get Caller-MnMr Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MN_MR:
		/* Get Caller-Mp Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MP:
		/* Get Caller-Mc Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MC:
			/* Set Status: Replier-MnMr */
			*pGetTelegramStatusFlag = TRDP_GOTTEN_REPLIER_MN_MR;
		break;
		/* Get Replier-MnMr Telegram Last Time ? */
		case TRDP_GOTTEN_REPLIER_MN_MR:
			/* Check Notify or Request */
			if (&pExchgPar->pDest[0] != NULL)
			{
				/* Set Status: Replier-MnMr */
				*pGetTelegramStatusFlag = TRDP_GOTTEN_REPLIER_MN_MR;
			}
			/* Check Reply or ReplyQuery */
			else if (pExchgPar->pMdPar->confirmTimeout == 0)
			{
				/* Set Status: Replier-Mp */
				*pGetTelegramStatusFlag = TRDP_GOTTEN_REPLIER_MP;
			}
			else
			{
				/* Set Status: Replier-Mq */
				*pGetTelegramStatusFlag = TRDP_GOTTEN_REPLIER_MQ;
			}
		break;
		/* Get Replier-Mp Telegram Last Time ? */
		case TRDP_GOTTEN_REPLIER_MP:
			/* Set Status: Replier-MnMr */
			*pGetTelegramStatusFlag = TRDP_GOTTEN_REPLIER_MN_MR;
		break;
		/* Get Replier-Mq Telegram Last Time ? */
		case TRDP_GOTTEN_REPLIER_MQ:
			/* Set Status: Replier-Mc */
			*pGetTelegramStatusFlag = TRDP_GOTTEN_REPLIER_MC;
		break;
		/* Get Replier-Mc Telegram Last Time ? */
		case TRDP_GOTTEN_REPLIER_MC:
			/* Set Status: Replier-MnMr */
			*pGetTelegramStatusFlag = TRDP_GOTTEN_REPLIER_MN_MR;
		break;
		default:
			vos_printLog(VOS_LOG_ERROR, "replierTelegram() Failed. Get Telegram Status Flag Err\n");
			/* Free Replier Dataset */
			vos_memFree(pReplierDataset);
			/* Free Replier Telegram */
			vos_memFree(pReplierTelegram);
			return TRDP_PARAM_ERR;
		break;
	}
	/* Set Message Type */
	pReplierTelegram->messageType = *pGetTelegramStatusFlag;

	/* Check AddListener */
	if ((*pGetTelegramStatusFlag == TRDP_GOTTEN_REPLIER_MN_MR)
		|| (*pGetTelegramStatusFlag == TRDP_GOTTEN_REPLIER_MC))
	{
		/* Add Listener */
		err = tlm_addListener(
				pReplierTelegram->appHandle,									/* our application identifier */
				&pReplierTelegram->listenerHandle,								/* our Listener identifier */
				(const void*)pReplierTelegram->pUserRef,						/* user reference value */
				pReplierTelegram->comId,           							/* ComID */
				pReplierTelegram->topoCount,        							/* topocount: local consist only */
				networkByteIpAddr,												/* multicast group to listen on */
				pReplierTelegram->pMdParameter->flags,							/* Option */
				*pReplierTelegram->pDestination->pUriHost);					/* only functional group of destination URI */
		if (err != TRDP_NO_ERR)
		{
			/* Free Replier Dataset */
			vos_memFree(pReplierDataset);
			/* Free Replier Telegram */
			vos_memFree(pReplierTelegram);
			vos_printLog(VOS_LOG_ERROR, "replierTelegram() failed. tlm_addListener() error. AddListener comID = %d error = %d\n", pReplierTelegram->comId, err);
			return err;
		}
		else
		{
			/* Create TRDP addListener Handle for tlm_delListener() */
			pNewListenerHandle = (LISTENER_HANDLE_T *)vos_memAlloc(sizeof(LISTENER_HANDLE_T));
			if (pNewListenerHandle == NULL)
			{
				vos_printLog(VOS_LOG_ERROR,"replierTelegram() Failed. Listener Handle vos_memAlloc() Err\n");
				return TRDP_MEM_ERR;
			}
			else
			{
				/* Initialize Listener Handle */
				memset(pNewListenerHandle, 0, sizeof(LISTENER_HANDLE_T));
			}
			/* Set Listener Handle List for tlm_delListener() */
			pNewListenerHandle->appHandle = pReplierTelegram->appHandle;
			pNewListenerHandle->pTrdpListenerHandle = pReplierTelegram->listenerHandle;
			if (appendListenerHandleList(&pHeadListenerHandleList, pNewListenerHandle) != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Set Listener Handle List error\n");
			}

			/* Get Listener memory area for TAUL send Message Queue when MD Receive */
			pReplierTelegram->listenerHandleForTAUL = (COMID_IP_HANDLE_T)vos_memAlloc(sizeof(TRDP_ADDRESSES_T));
			if (pReplierTelegram->listenerHandleForTAUL == NULL)
			{
				vos_printLog(VOS_LOG_ERROR,"replierTelegram() Failed. Listener vos_memAlloc() Err\n");
				/* Free Replier Listener Handle */
				vos_memFree(pNewListenerHandle);
				/* Free Replier Dataset */
				vos_memFree(pReplierDataset);
				/* Free Replier Telegram */
				vos_memFree(pReplierTelegram);
				return TRDP_MEM_ERR;
			}
			else
			{
				/* Initialize Caller Telegram */
				memset(pReplierTelegram->listenerHandleForTAUL, 0, sizeof(TRDP_ADDRESSES_T));
			}
			/* Set ListenerHandle */
			pReplierTelegram->listenerHandleForTAUL->comId = pReplierTelegram->comId;
			pReplierTelegram->listenerHandleForTAUL->destIpAddr = networkByteIpAddr;
			pReplierTelegram->listenerHandleForTAUL->srcIpAddr = IP_ADDRESS_NOTHING;

			/* Append Replier Telegram */
			err = appendReplierTelegramList(&pHeadReplierTelegram, pReplierTelegram);
			if (err != TRDP_NO_ERR)
			{
				/* Free Replier Listener Handle */
				vos_memFree(pNewListenerHandle);
				/* Free Replier Dataset */
				vos_memFree(pReplierDataset);
				/* Free Replier Telegram */
				vos_memFree(pReplierTelegram);
				vos_printLog(VOS_LOG_ERROR, "replierTelegram() Failed. Replier Telegram appendReplierTelegramList() Err:%d\n", err);
				return err;
			}
			else
			{
				/* Create Waiting Receive Request Reference */
				/* Get Waiting Receive Request Reference memory area */
				pWaitingReceiveRequestReference = (WAITING_RECEIVE_REQUEST_T *)vos_memAlloc(sizeof(WAITING_RECEIVE_REQUEST_T));
				if (pWaitingReceiveRequestReference == NULL)
				{
					vos_printLog(VOS_LOG_ERROR,"replierTelegram() Failed. Waiting Receive Request Reference vos_memAlloc() Err\n");
					/* Free Replier Listener Handle */
					vos_memFree(pNewListenerHandle);
					/* Free Replier Dataset */
					vos_memFree(pReplierDataset);
					/* Free Replier Telegram */
					vos_memFree(pReplierTelegram);
					return TRDP_MEM_ERR;
				}
				else
				{
					/* Initialize pWaitingReceiveRequestReference */
					memset(pWaitingReceiveRequestReference, 0, sizeof(WAITING_RECEIVE_REQUEST_T));
				}
				/* Get Waiting Receive Request Reference memory area */
				pWaitingReceiveRequestReference->pTaulReference = (UINT32 *)vos_memAlloc(sizeof(UINT32));
				if (pWaitingReceiveRequestReference->pTaulReference == NULL)
				{
					vos_printLog(VOS_LOG_ERROR,"replierTelegram() Failed. Waiting Receive Request TAUL Reference vos_memAlloc() Err\n");
					/* Free Replier Listener Handle */
					vos_memFree(pNewListenerHandle);
					/* Free Replier Dataset */
					vos_memFree(pReplierDataset);
					/* Free Replier Telegram */
					vos_memFree(pReplierTelegram);
					/* Free Waiting Receive Request Reference*/
					vos_memFree(pWaitingReceiveRequestReference);
					return TRDP_MEM_ERR;
				}
				else
				{
					/* Initialize pWaitingReceiveRequestReference TAUL Reference */
					memset(pWaitingReceiveRequestReference->pTaulReference, 0, sizeof(UINT32));
				}

				/* Set Waiting Receive Request Reference */
				/* Set TAUL Reference */
				*pWaitingReceiveRequestReference->pTaulReference = RECEIVE_REFERENCE;
				/* Set Caller Reference (Add Listener userRef) */
				pWaitingReceiveRequestReference->callerReference = pReplierTelegram->pUserRef;
				/* Set Reply ComId */
				pWaitingReceiveRequestReference->requestComId = pReplierTelegram->comId;
				/* Set Destination IP Address */
				pWaitingReceiveRequestReference->dstIpAddr = networkByteIpAddr;
				/* Set Destination URI */
				if (pReplierTelegram->pDestination->pUriHost != NULL)
				{
					memcpy(pWaitingReceiveRequestReference->dstURI, pReplierTelegram->pDestination->pUriHost, sizeof(TRDP_URI_USER_T));
				}
				/* Append Waiting Receive Reply Reference Queue */
				err = appendWaitingReceiveRequestQueue(&pHeadWaitingReceiveRequestReference, pWaitingReceiveRequestReference);
				if (err != TRDP_NO_ERR)
				{
					/* Free Waiting Receive Reply Reference */
					vos_memFree(pWaitingReceiveRequestReference);
					/* Free Replier Listener Handle */
					vos_memFree(pNewListenerHandle);
					/* Free Replier Dataset */
					vos_memFree(pReplierDataset);
					/* Free Replier Telegram */
					vos_memFree(pReplierTelegram);
					vos_printLog(VOS_LOG_ERROR, "replierTelegram() Failed. appendWaitingReceiveRequestQueue() Err:%d\n", err);
					return err;
				}
			}

			/* Check confirm Telegram ? */
			if (*pGetTelegramStatusFlag == TRDP_GOTTEN_REPLIER_MC)
			{
				/* Create Waiting Receive Confirm Reference */
				pNewWaitingReceiveConfirmReference = (WAITING_RECEIVE_CONFIRM_T *)vos_memAlloc(sizeof(WAITING_RECEIVE_CONFIRM_T));
				if (pNewWaitingReceiveConfirmReference == NULL)
				{
					vos_printLog(VOS_LOG_ERROR,"replierTelegram() Failed. Waiting Receive Confirm Reference vos_memAlloc() Err\n");
					/* Free Waiting Receive Reply Reference */
					vos_memFree(pWaitingReceiveRequestReference);
					/* Free Replier Listener Handle */
					vos_memFree(pNewListenerHandle);
					/* Free Replier Dataset */
					vos_memFree(pReplierDataset);
					/* Free Replier Telegram */
					vos_memFree(pReplierTelegram);
					return TRDP_MEM_ERR;
				}
				else
				{
					/* Initialize Waiting Receive Confirm Reference */
					memset(pNewWaitingReceiveConfirmReference, 0, sizeof(WAITING_RECEIVE_CONFIRM_T));
				}
				/* Set Waiting Send Reply Reference */
				/* Set TAUL Reference : Mr Listener Count)*/
				pNewWaitingReceiveConfirmReference->taulReference = listenerCounter - 2;
				/* Set Confirm ComId */
				pNewWaitingReceiveConfirmReference->confirmComId = pReplierTelegram->comId;
				/* Append Waiting Receive Confirm Reference Queue */
				err = appendWaitingReceiveConfirmQueue(&pHeadWaitingReceiveConfirmReference, pNewWaitingReceiveConfirmReference);
				if (err != TRDP_NO_ERR)
				{
					/* Free Waiting Receive Reply Reference */
					vos_memFree(pNewWaitingReceiveConfirmReference);
					/* Free Waiting Receive Reply Reference */
					vos_memFree(pWaitingReceiveRequestReference);
					/* Free Replier Listener Handle */
					vos_memFree(pNewListenerHandle);
					/* Free Replier Dataset */
					vos_memFree(pReplierDataset);
					/* Free Replier Telegram */
					vos_memFree(pReplierTelegram);
					vos_printLog(VOS_LOG_ERROR, "replierTelegram() Failed. Waiting Receive Reply Reference appendWaitingReceiveReplyQueue() Err:%d\n", err);
					return err;
				}
			}
		}
	}
	else
	{
		/* Append Replier Telegram */
		err = appendReplierTelegramList(&pHeadReplierTelegram, pReplierTelegram);
		if (err != TRDP_NO_ERR)
		{
			/* Free Replier Dataset */
			vos_memFree(pReplierDataset);
			/* Free Replier Telegram */
			vos_memFree(pReplierTelegram);
			/* Free Replier Listener Handle */
			vos_memFree(pNewListenerHandle);
			vos_printLog(VOS_LOG_ERROR, "replierTelegram() Failed. Replier Telegram appendReplierTelegramList() Err:%d\n", err);
			return err;
		}
	}
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** Caller Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex								interface Index
 *  @param[in]      numExchgPar							Number of Exchange Parameter
 *  @param[in]      pExchgPar							Pointer to Exchange Parameter
 *  @param[in]      pGetTelegramStatusFlag			Pointer to Get Telegram Status Flag
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T callerTelegram (
		UINT32				ifIndex,
		TRDP_EXCHG_PAR_T	*pExchgPar,
		UINT32				*pGetTelegramStatusFlag)
{
	UINT32					i = 0;
	CALLER_TELEGRAM_T		*pCallerTelegram = NULL;
	UINT32					*pCallerDataset = NULL;
	TRDP_IP_ADDR_T		destinationAddress = 0;
	TRDP_URI_USER_T   	destinationURI = {0};
	TRDP_ERR_T				err = TRDP_NO_ERR;
	LISTENER_HANDLE_T    *pNewListenerHandle = NULL;
//	UINT8					dstend = 0;

	/* Check Parameter */
	if (pGetTelegramStatusFlag == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "callerTelegram() Failed. Get Telegram Status Flag Err\n");
		return TRDP_PARAM_ERR;
	}

	/* Caller Telegram */
	/* Get Caller Telegram memory area */
	pCallerTelegram = (CALLER_TELEGRAM_T *)vos_memAlloc(sizeof(CALLER_TELEGRAM_T));
	if (pCallerTelegram == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"callerTelegram() Failed. Caller Telegram vos_memAlloc() Err\n");
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Caller Telegram */
		memset(pCallerTelegram, 0, sizeof(CALLER_TELEGRAM_T));
	}

	/* Find ExchgPar Dataset for the dataset config */
	for (i = 0; i < numDataset; i++)
	{
		/* DatasetId in dataSet config ? */
		if (pExchgPar->datasetId == apDataset[i]->id)
		{
			/* Set Dataset Descriptor */
			pCallerTelegram->pDatasetDescriptor = apDataset[i];
			break;
		}
	}
	/* DataSetId effective ? */
	if (pCallerTelegram->pDatasetDescriptor == 0)
	{
		vos_printLog(VOS_LOG_ERROR,"callerTelegram() Failed. Dataset Err. datasetId: %d, comId: %d\n", pExchgPar->datasetId, pExchgPar->comId);
		/* Free Caller Telegram */
		vos_memFree(pCallerTelegram);
		return TRDP_PARAM_ERR;
	}

	/* Set Caller Telegram */
	/* Set Application Handle */
	if (ifIndex == IF_INDEX_SUBNET1)
	{
		/* Set Application Handle: Subnet1 */
		pCallerTelegram->appHandle = arraySessionConfig[ifIndex].sessionHandle;
		/* Set AddListener Destination IP Address */
		destinationAddress = subnetId1Address;
		/* Set Destination URI */
		memset(destinationURI, 0, sizeof(destinationURI));
	}
	else if(ifIndex == IF_INDEX_SUBNET2)
	{
		/* Set Application Handle: Subnet2 */
		pCallerTelegram->appHandle = arraySessionConfig[ifIndex].sessionHandle;
		/* Set AddListener Destination IP Address */
		destinationAddress = subnetId2Address;
		/* Set Destination URI */
		memset(destinationURI, 0, sizeof(destinationURI));
	}
	else
	{
		/* ifIndex Error */
		vos_printLog(VOS_LOG_ERROR, "callerTelegram() Failed. ifIndex:%d error\n", ifIndex);
		/* Free Caller Telegram */
		vos_memFree(pCallerTelegram);
		return TRDP_PARAM_ERR;
	}
	/* Set Dataset */
	/* Get Dataset Size */
//	err = sizeWriteDatasetInTrafficStore(&dstend, &pCallerTelegram->dataset.size, pCallerTelegram->pDatasetDescriptor);
	err = sizeWriteDatasetInTrafficStore(&pCallerTelegram->dataset.size, pCallerTelegram->pDatasetDescriptor);
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "callerTelegram() Failed. sizeWriteDatasetInTrafficStore() returns error = %d\n", err);
		/* Free Caller Telegram */
		vos_memFree(pCallerTelegram);
		return TRDP_PARAM_ERR;
	}
	/* Create Dataset */
	pCallerDataset = (UINT32 *)vos_memAlloc(pCallerTelegram->dataset.size);
	if (pCallerDataset == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"callerTelegram() Failed. Caller Dataset vos_memAlloc() Err\n");
		/* Free Caller Telegram */
		vos_memFree(pCallerTelegram);
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Caller dataset */
		memset(pCallerDataset, 0, pCallerTelegram->dataset.size);
	}
	/* Enable Marshalling ? Check exchgPar and MD config */
	if (((pExchgPar->pMdPar->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
		|| ((arraySessionConfig[ifIndex].mdConfig.flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL))
	{
		/* Compute size of marshalled dataset */
		err = tau_calcDatasetSize(
				marshallConfig.pRefCon,
				pExchgPar->datasetId,
				(UINT8 *) pCallerDataset,
				&pCallerTelegram->datasetNetworkByteSize,
				&pCallerTelegram->pDatasetDescriptor);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "callerTelegram() Failed. tau_calcDatasetSize datasetId: %d returns error = %d\n", pExchgPar->datasetId, err);
			/* Free Caller Dataset */
			vos_memFree(pCallerDataset);
			/* Free Caller Telegram */
			vos_memFree(pCallerTelegram);
			return TRDP_PARAM_ERR;
		}
	}
	/* Set Dataset Buffer */
	pCallerTelegram->dataset.pDatasetStartAddr = (UINT8 *)pCallerDataset;
	/* Set If Config */
	pCallerTelegram->pIfConfig = &pIfConfig[ifIndex];
	/* Set MD Parameter */
	pCallerTelegram->pMdParameter = pExchgPar->pMdPar;
	/* Set comId */
	pCallerTelegram->comId = pExchgPar->comId;
	/* Set topoCount */
	pCallerTelegram->topoCount = 0;
	/* Set Source IP Address */
	pCallerTelegram->pSource = &pExchgPar->pSrc[0];
	/* Set user reference */
	/* Get AddListener Reference memory area */
	pAddListenerReference = (UINT32 *)vos_memAlloc(sizeof(UINT32));
	if (pAddListenerReference == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"callerTelegram() Failed. Add Listener Reference vos_memAlloc() Err\n");
		/* Free Caller Dataset */
		vos_memFree(pCallerDataset);
		/* Free Caller Telegram */
		vos_memFree(pCallerTelegram);
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Add Listener Reference */
		memset(pAddListenerReference, 0, sizeof(UINT32));
	}
	/* Count up */
	listenerCounter++;
	memcpy(pAddListenerReference, &listenerCounter, sizeof(UINT32));
	pCallerTelegram->pUserRef = pAddListenerReference;

	/* Change Status */
	switch (*pGetTelegramStatusFlag)
	{
		/* Get Telegram First Time ? */
		case TRDP_GET_NONE:
			/* Set Status: Caller-MnMr */
			*pGetTelegramStatusFlag = TRDP_GOTTEN_CALLER_MN_MR;
		break;
		/* Get Caller-MnMr Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MN_MR:
			/* Check Notify or Request */
			if (&pExchgPar->pSrc[0] != NULL)
			{
				/* Set Status: Caller-MnMr */
				*pGetTelegramStatusFlag = TRDP_GOTTEN_CALLER_MN_MR;
			}
			/* Check Reply or ReplyQuery */
			else if (pExchgPar->pMdPar->confirmTimeout == 0)
			{
				/* Set Status: Caller-Mp */
				*pGetTelegramStatusFlag = TRDP_GOTTEN_CALLER_MP;
			}
			else
			{
				/* Set Status: Caller-Mq */
				*pGetTelegramStatusFlag = TRDP_GOTTEN_CALLER_MQ;
			}
		break;
		/* Get Caller-Mp Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MP:
			/* Set Status: Caller-MnMr */
			*pGetTelegramStatusFlag = TRDP_GOTTEN_CALLER_MN_MR;
		break;
		/* Get Caller-Mq Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MQ:
			/* Set Status: Caller-Mc */
			*pGetTelegramStatusFlag = TRDP_GOTTEN_CALLER_MC;
		break;
		/* Get Caller-Mc Telegram Last Time ? */
		case TRDP_GOTTEN_CALLER_MC:
			/* Set Status: Caller-MnMr */
			*pGetTelegramStatusFlag = TRDP_GOTTEN_CALLER_MN_MR;
		break;
		default:
			vos_printLog(VOS_LOG_ERROR, "callerTelegram() Failed. Get Telegram Status Flag Err\n");
			/* Free Caller Dataset */
			vos_memFree(pCallerDataset);
			/* Free Caller Telegram */
			vos_memFree(pCallerTelegram);
			return TRDP_PARAM_ERR;
		break;
	}
	/* Set Message Type */
	pCallerTelegram->messageType = *pGetTelegramStatusFlag;

	/* Check AddListener */
	if ((*pGetTelegramStatusFlag == TRDP_GOTTEN_CALLER_MP)
		|| (*pGetTelegramStatusFlag == TRDP_GOTTEN_CALLER_MQ))
	{
		/* Add Listener */
		err = tlm_addListener(
				pCallerTelegram->appHandle,									/* our application identifier */
				&pCallerTelegram->listenerHandle,							/* our Listener identifier */
				(const void*)pCallerTelegram->pUserRef,						/* user reference value */
				pCallerTelegram->comId,           							/* ComID */
				pCallerTelegram->topoCount,        						/* topocount: local consist only */
				destinationAddress,											/* multicast group to listen on : IF Address */
				pCallerTelegram->pMdParameter->flags,						/* Option */
				destinationURI);				/* only functional group of destination URI */
		if (err != TRDP_NO_ERR)
		{
			/* Free Caller Dataset */
			vos_memFree(pCallerDataset);
			/* Free Replier Telegram */
			vos_memFree(pCallerTelegram);
			vos_printLog(VOS_LOG_ERROR, "callerTelegram() failed. tlm_addListener() error. AddListener comID = 0x%x error = %d\n", pCallerTelegram->comId, err);
			return err;
		}
		else
		{
			/* Create TRDP addListener Handle for tlm_delListener() */
			pNewListenerHandle = (LISTENER_HANDLE_T *)vos_memAlloc(sizeof(LISTENER_HANDLE_T));
			if (pNewListenerHandle == NULL)
			{
				vos_printLog(VOS_LOG_ERROR,"callerTelegram() Failed. Listener Handle vos_memAlloc() Err\n");
				/* Free Caller Dataset */
				vos_memFree(pCallerDataset);
				/* Free Caller Telegram */
				vos_memFree(pCallerTelegram);
				return TRDP_MEM_ERR;
			}
			else
			{
				/* Initialize Listener Handle */
				memset(pNewListenerHandle, 0, sizeof(LISTENER_HANDLE_T));
			}
			/* Set Listener Handle List for tlm_delListener() */
			pNewListenerHandle->appHandle = pCallerTelegram->appHandle;
			pNewListenerHandle->pTrdpListenerHandle = pCallerTelegram->listenerHandle;
			if (appendListenerHandleList(&pHeadListenerHandleList, pNewListenerHandle) != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Set Listener Handle List error\n");
			}
		}
	}

	/* Get Listener memory area for TAUL send Message Queue when MD Receive */
	pCallerTelegram->listenerHandleForTAUL = (COMID_IP_HANDLE_T)vos_memAlloc(sizeof(TRDP_ADDRESSES_T));
	if (pCallerTelegram->listenerHandleForTAUL == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"callerTelegram() Failed. Listener vos_memAlloc() Err\n");
		/* Free Caller Dataset */
		vos_memFree(pCallerDataset);
		/* Free Caller Telegram */
		vos_memFree(pCallerTelegram);
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Caller Telegram */
		memset(pCallerTelegram->listenerHandleForTAUL, 0, sizeof(TRDP_ADDRESSES_T));
	}
	/* Set ListenerHandle */
	pCallerTelegram->listenerHandleForTAUL->comId = pCallerTelegram->comId;
	pCallerTelegram->listenerHandleForTAUL->destIpAddr = pCallerTelegram->appHandle->realIP;
	pCallerTelegram->listenerHandleForTAUL->srcIpAddr = IP_ADDRESS_NOTHING;

	/* Append Caller Telegram */
	err = appendCallerTelegramList(&pHeadCallerTelegram, pCallerTelegram);
	if (err != TRDP_NO_ERR)
	{
		/* Free Caller Dataset */
		vos_memFree(pCallerDataset);
		/* Free Caller Telegram */
		vos_memFree(pCallerTelegram);
		vos_printLog(VOS_LOG_ERROR, "callerTelegram() Failed. Caller Telegram appendReplierTelegramList() Err:%d\n", err);
		return err;
	}
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** PD Main Process Init
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_THREAD_ERR
 */
TRDP_ERR_T taul_pd_main_proc_init (
	void)
{
	VOS_ERR_T vosErr = VOS_NO_ERR;
	extern VOS_THREAD_T taulPdMainThreadHandle;			/* Thread handle */

	/* TAULpdMainThread */
	extern CHAR8 taulPdMainThreadName[];					/* Thread name is TAUL PD Main Thread. */
//	extern BOOL8 taulPdMainThreadActiveFlag;				/* TAUL PD Main Thread active/noactive Flag :active=TRUE, nonActive=FALSE */
//	extern BOOL8 taulPdMainThreadStartFlag;				/* TAUL PD Main Thread instruction start up Flag :start=TRUE, stop=FALSE */

	/* Init Thread */
	vos_threadInit();
	/* Create TAULpdMainThread */
	vosErr = vos_threadCreate(&taulPdMainThreadHandle,
				taulPdMainThreadName,
				VOS_THREAD_POLICY_OTHER,
				0,
				0,
				0,
				(void *)TAULpdMainThread,
				NULL);
	if (vosErr != VOS_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "TRDP TAULpdMainThread Create failed. VOS Error: %d\n", vosErr);
		return TRDP_THREAD_ERR;
	}
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** TAUL PD Main Process Thread
 *
 */
VOS_THREAD_FUNC_T TAULpdMainThread (
	void)
{
	PD_ELE_T					*iterPD = NULL;
	TRDP_TIME_T				nowTime = {0};
	PUBLISH_TELEGRAM_T		*pUpdatePublishTelegram = NULL;
	PD_REQUEST_TELEGRAM_T	*pUpdatePdRequestTelegram = NULL;
	TRDP_ERR_T					err = TRDP_NO_ERR;
	UINT16						msgTypePrNetworkByteOder = 0;
	UINT32						replyComIdHostByetOrder = 0;
	UINT32						replyIpAddrHostByteOrder = 0;
	struct timeval			tv_interval = {0};								/* interval Time :timeval type */
	TRDP_TIME_T				trdp_time_tv_interval = {0};					/* interval Time :TRDP_TIME_T type for TRDP function */

	/* Set Application Handle : Subnet1 */
	appHandle = arraySessionConfig[IF_INDEX_SUBNET1].sessionHandle;
	/* Set Application Handle : Subnet2 */
	if (NUM_IF_CONFIG >= LADDER_IF_NUMBER)
	{
		appHandle2 = arraySessionConfig[IF_INDEX_SUBNET2].sessionHandle;
	}
	else
	{
		appHandle2 = NULL;
	}
	/* Set Byet order Message Type:Pr */
	msgTypePrNetworkByteOder = vos_htons(TRDP_MSG_PR);

	/* Enter the PD main processing loop. */
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
		BOOL8 linkUpDown = TRUE;						/* Link Up Down information TRUE:Up FALSE:Down */
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
		rv = vos_select((int)noOfDesc + 1, &rfds, NULL, NULL, (VOS_TIME_T *)&tv);

		/* Check PD Send Queue of appHandle1 */
		for (iterPD = appHandle->pSndQueue; iterPD != NULL; iterPD = iterPD->pNext)
		{
			/* Get Now Time */
			vos_getTime(&nowTime);
			/* PD Request Telegram ? */
			if (iterPD->pFrame->frameHead.msgType == msgTypePrNetworkByteOder)
			{
				/* PD Pull (request) ? */
				if (iterPD->addr.comId != TRDP_GLOBAL_STATISTICS_COMID)
				{
					/* Change Byet Order */
					replyComIdHostByetOrder = vos_ntohl(iterPD->pFrame->frameHead.replyComId);
					replyIpAddrHostByteOrder = vos_ntohl(iterPD->pFrame->frameHead.replyIpAddress);
					/* Get PD Request Telegram */
					pUpdatePdRequestTelegram = searchPdRequestTelegramList(
							pHeadPdRequestTelegram,
							iterPD->addr.comId,
							replyComIdHostByetOrder,
							iterPD->addr.srcIpAddr,
							iterPD->addr.destIpAddr,
							replyIpAddrHostByteOrder);
					if (pUpdatePdRequestTelegram == NULL)
					{
						vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. Get PD Request Telegram Err.\n");
					}
					else
					{
						/* First Request Send ? */
						if ((pUpdatePdRequestTelegram->requestSendTime.tv_sec == 0)
							&& (pUpdatePdRequestTelegram->requestSendTime.tv_usec == 0))
						{
							/* Set now Time */
							vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &nowTime);
							/* Convert Request Send cycle time */
							tv_interval.tv_sec = pUpdatePdRequestTelegram->pPdParameter->cycle / 1000000;
							tv_interval.tv_usec = pUpdatePdRequestTelegram->pPdParameter->cycle % 1000000;
							trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
							trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
							/* Set Request Send Time */
							vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &trdp_time_tv_interval);
						}
						/* Is Now Time send Timing ? */
						if (vos_cmpTime((TRDP_TIME_T *)&pUpdatePdRequestTelegram->requestSendTime, (TRDP_TIME_T *)&nowTime) < 0)
						{
							/* PD Request */
							err = tlp_request(
									appHandle,
									pUpdatePdRequestTelegram->subHandle,
									iterPD->addr.comId,
									pUpdatePdRequestTelegram->topoCount,
									pUpdatePdRequestTelegram->srcIpAddr,
									pUpdatePdRequestTelegram->dstIpAddr,
									pUpdatePdRequestTelegram->pPdParameter->redundant,
									pUpdatePdRequestTelegram->pPdParameter->flags,
									pUpdatePdRequestTelegram->pSendParam,
//									(void *)(INT32) pTrafficStoreAddr + pUpdatePdRequestTelegram->pPdParameter->offset,
									(UINT8 *)((INT32) pTrafficStoreAddr + (UINT8)pUpdatePdRequestTelegram->pPdParameter->offset),
									pUpdatePdRequestTelegram->datasetNetworkByteSize,
									pUpdatePdRequestTelegram->replyComId,
									pUpdatePdRequestTelegram->replyIpAddr);
							if (err != TRDP_NO_ERR)
							{
								vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. tlp_request() Err: %d\n", err);
							}
							vos_printLog(VOS_LOG_DBG, "Subnet1 tlp_request()\n");
							/* Get Now Time */
							vos_getTime(&pUpdatePdRequestTelegram->requestSendTime);
							/* Convert Request Send cycle time */
							tv_interval.tv_sec = pUpdatePdRequestTelegram->pPdParameter->cycle / 1000000;
							tv_interval.tv_usec = pUpdatePdRequestTelegram->pPdParameter->cycle % 1000000;
							trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
							trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
							/* Set Request Send Time */
							vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &trdp_time_tv_interval);
						}
					}
				}
			}
			/* Publish Telegram */
			else
			{
				/* Is Now Time send Timing ? */
				if (vos_cmpTime((TRDP_TIME_T *)&iterPD->timeToGo, (TRDP_TIME_T *)&nowTime) < 0)
				{
					/* Check comId which Publish our statistics packet */
					if (iterPD->addr.comId != TRDP_GLOBAL_STATISTICS_COMID)
					{
						/* Get Publish Telegram */
						pUpdatePublishTelegram = searchPublishTelegramList(
														pHeadPublishTelegram,
														iterPD->addr.comId,
														iterPD->addr.srcIpAddr,
														iterPD->addr.destIpAddr);
						if (pUpdatePublishTelegram == NULL)
						{
							vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. Get Publish Telegram Err.\n");
						}
						else
						{
							/* Update Publish Dataset */
							err = tlp_put(
									appHandle,
									pUpdatePublishTelegram->pubHandle,
									(pTrafficStoreAddr + pUpdatePublishTelegram->pPdParameter->offset),
									pUpdatePublishTelegram->datasetNetworkByteSize);
							if (err != TRDP_NO_ERR)
							{
								vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. tlp_put() Err: %d\n", err);
							}
						}
					}
				}
			}
		}

		/* Check Ladder Topology ? */
		if (appHandle2 != NULL)
		{
			/* Check PD Send Queue of appHandle2 */
			for (iterPD = appHandle2->pSndQueue; iterPD != NULL; iterPD = iterPD->pNext)
			{
				/* Get Now Time */
				vos_getTime(&nowTime);
				/* PD Request Telegram ? */
				if (iterPD->pFrame->frameHead.msgType == msgTypePrNetworkByteOder)
				{
					/* PD Pull (request) ? */
					if (iterPD->addr.comId != TRDP_GLOBAL_STATISTICS_COMID)
					{
						/* Change Byet Order */
						replyComIdHostByetOrder = vos_ntohl(iterPD->pFrame->frameHead.replyComId);
						replyIpAddrHostByteOrder = vos_ntohl(iterPD->pFrame->frameHead.replyIpAddress);
						/* Get PD Request Telegram */
						pUpdatePdRequestTelegram = searchPdRequestTelegramList(
								pHeadPdRequestTelegram,
								iterPD->addr.comId,
								replyComIdHostByetOrder,
								iterPD->addr.srcIpAddr,
								iterPD->addr.destIpAddr,
								replyIpAddrHostByteOrder);
						if (pUpdatePdRequestTelegram == NULL)
						{
							vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. Get PD Request Telegram Err.\n");
						}
						else
						{
							/* First Request Send ? */
							if ((pUpdatePdRequestTelegram->requestSendTime.tv_sec == 0)
								&& (pUpdatePdRequestTelegram->requestSendTime.tv_usec == 0))
							{
								/* Set now Time */
								vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &nowTime);
								/* Convert Request Send cycle time */
								tv_interval.tv_sec = pUpdatePdRequestTelegram->pPdParameter->cycle / 1000000;
								tv_interval.tv_usec = pUpdatePdRequestTelegram->pPdParameter->cycle % 1000000;
								trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
								trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
								/* Set Request Send Time */
								vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &trdp_time_tv_interval);
							}
							/* Is Now Time send Timing ? */
							if (vos_cmpTime((TRDP_TIME_T *)&pUpdatePdRequestTelegram->requestSendTime, (TRDP_TIME_T *)&nowTime) < 0)
							{
								/* PD Request */
								err = tlp_request(
										appHandle2,
										pUpdatePdRequestTelegram->subHandle,
										iterPD->addr.comId,
										pUpdatePdRequestTelegram->topoCount,
										pUpdatePdRequestTelegram->srcIpAddr,
										pUpdatePdRequestTelegram->dstIpAddr,
										pUpdatePdRequestTelegram->pPdParameter->redundant,
										pUpdatePdRequestTelegram->pPdParameter->flags,
										pUpdatePdRequestTelegram->pSendParam,
	//									(void *)(INT32) pTrafficStoreAddr + pUpdatePdRequestTelegram->pPdParameter->offset,
										(UINT8 *)((INT32) pTrafficStoreAddr + (UINT32)pUpdatePdRequestTelegram->pPdParameter->offset),
										pUpdatePdRequestTelegram->datasetNetworkByteSize,
										pUpdatePdRequestTelegram->replyComId,
										pUpdatePdRequestTelegram->replyIpAddr);
								if (err != TRDP_NO_ERR)
								{
									vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. tlp_request() Err: %d\n", err);
								}
								vos_printLog(VOS_LOG_DBG, "Subnet2 tlp_request()\n");
								/* Get Now Time */
								vos_getTime(&pUpdatePdRequestTelegram->requestSendTime);
								/* Convert Request Send cycle time */
								tv_interval.tv_sec = pUpdatePdRequestTelegram->pPdParameter->cycle / 1000000;
								tv_interval.tv_usec = pUpdatePdRequestTelegram->pPdParameter->cycle % 1000000;
								trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
								trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
								/* Set Request Send Time */
								vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &trdp_time_tv_interval);
							}
						}
					}
				}
				/* Publish Telegram */
				else
				{
					/* Is Now Time send Timing ? */
					if (vos_cmpTime((TRDP_TIME_T *)&iterPD->timeToGo, (TRDP_TIME_T *)&nowTime) < 0)
					{
						/* Check comId which Publish our statistics packet */
						if (iterPD->addr.comId != TRDP_GLOBAL_STATISTICS_COMID)
						{
							/* Get Publish Telegram */
							pUpdatePublishTelegram = searchPublishTelegramList(
															pHeadPublishTelegram,
															iterPD->addr.comId,
															iterPD->addr.srcIpAddr,
															iterPD->addr.destIpAddr);
							if (pUpdatePublishTelegram == NULL)
							{
								vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. Get Publish Telegram Err.\n");
							}
							else
							{
								/* Update Publish Dataset */
								err = tlp_put(
										appHandle2,
										pUpdatePublishTelegram->pubHandle,
										(void *)(INT32)(pTrafficStoreAddr + pUpdatePublishTelegram->pPdParameter->offset),
										pUpdatePublishTelegram->datasetNetworkByteSize);
								if (err != TRDP_NO_ERR)
								{
									vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. tlp_put() Err: %d\n", err);
								}
							}
						}
					}
				}
			}
		}

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
			err = tau_getNetworkContext(&writeSubnetId);
			if (err != TRDP_NO_ERR)
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
				err = tau_setNetworkContext(writeSubnetId);
				if (err != TRDP_NO_ERR)
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
}


/**********************************************************************************************************************/
/** TAUL API
 */
/******************************************************************************/
/** Initialize TAUL and create shared memory if required.
 *  Create Traffic Store mutex, Traffic Store
 *
 *  @param[in]      pPrintDebugString	Pointer to debug print function
 *  @param[in]      pLdConfig			Pointer to Ladder configuration
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T tau_ldInit (
	TRDP_PRINT_DBG_T			pPrintDebugString,
	const TAU_LD_CONFIG_T	*pLdConfig)
{
#ifdef XML_CONFIG_ENABLE
	CHAR8 xmlConfigFileName[FILE_NAME_MAX_SIZE] = "xmlconfig.xml";		/* XML Config File Name */
#endif /* ifdef XML_CONFIG_ENABLE */
	TRDP_ERR_T err = TRDP_NO_ERR;											/* Result */
	UINT32 ifIndex = 0;														/* I/F get Loop Counter */
	UINT32 index = 0;															/* Loop Counter */
	BOOL8							marshallInitFirstTime = TRUE;
	TRDP_MARSHALL_CONFIG_T		*pMarshallConfigPtr = NULL;
	extern UINT32					numExchgPar;
	INT8							*pUseMdSendSubnetId = NULL;
	/* For Get IP Address */
	UINT32 getNoOfIfaces = NUM_ED_INTERFACES;
	VOS_IF_REC_T ifAddressTable[NUM_ED_INTERFACES];
	TRDP_IP_ADDR_T ownIpAddress = 0;
#ifdef __linux
	CHAR8 SUBNETWORK_ID1_IF_NAME[] = "eth0";
//#elif defined(__APPLE__)
#else
	CHAR8 SUBNETWORK_ID1_IF_NAME[] = "en0";
#endif

#ifdef XML_CONFIG_ENABLE
	/* Read XML Config File */
	err = tau_prepareXmlDoc(xmlConfigFileName, &xmlConfigHandle);
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_prepareXmlDoc() error\n");
		return err;
	}
	/* Get Config */
	err = tau_readXmlDeviceConfig(&xmlConfigHandle,
										&memoryConfig,
										&debugConfig,
										&numComPar,
										&pComPar,
										&numIfConfig,
										&pIfConfig);
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_readXmlDeviceConfig() error\n");
		return err;
	}
#else
	/* Set Config Parameter from Internal Config */
	err = setConfigParameterFromInternalConfig();
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. setConfigParameter() error\n");
		return err;
	}
#endif /* ifdef XML_CONFIG_ENABLE */

	/* Get I/F address */
	if (vos_getInterfaces(&getNoOfIfaces, ifAddressTable) != VOS_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. vos_getInterfaces() error.\n");
	   return TRDP_SOCK_ERR;
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
	/* Sub-net Id2 Address */
	subnetId2Address = subnetId1Address | SUBNET2_NETMASK;
	/* Set Use I/F for Send MD Message Type : Subnet1 */
	useMdSendSubnetType = MD_SEND_USE_SUBNET1;

	/*	Init the TRDP library  */
	err = tlc_init(pPrintDebugString,			/* debug print function */
					&memoryConfig);				/* Use application supplied memory	*/
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tlc_init() error = %d\n",err);
		return err;
	}

#ifdef XML_CONFIG_ENABLE
	/* Get Dataset Config */
	err = tau_readXmlDatasetConfig(&xmlConfigHandle,
										&numComId,
										&pComIdDsIdMap,
										&numDataset,
										&apDataset);
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_readXmlDatasetConfig() error = %d\n",err);
		return err;
	}
#endif /* ifdef XML_CONFIG_ENABLE */

	/* Set MD Call Back Function */
	memcpy(&taulConfig, pLdConfig, sizeof(TAU_LD_CONFIG_T));

	/* Check I/F config exists */
	if (numIfConfig <= 0)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. Nothing I/F config error\n");
		return TRDP_PARAM_ERR;
	}

	/* Get I/F Config Loop */
	for (ifIndex = 0; ifIndex < numIfConfig; ifIndex++)
	{
#ifdef XML_CONFIG_ENABLE
		/* Get I/F Config Parameter */
		err = tau_readXmlInterfaceConfig(&xmlConfigHandle,
											pIfConfig[ifIndex].ifName,
											&arraySessionConfig[ifIndex].processConfig,
											&arraySessionConfig[ifIndex].pdConfig,
											&arraySessionConfig[ifIndex].mdConfig,
											&numExchgPar,
											&arrayExchgPar[ifIndex]);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_readXmlInterfaceConfig() error = %d\n",err);
			return err;
		}
#endif /* ifdef XML_CONFIG_ENABLE */

		/* Enable Marshalling ? Check XML Config: pd-com-paramter marshall= "on" */
		if ((arraySessionConfig[ifIndex].pdConfig.flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
		{
			/* Set MarshallConfig */
			pMarshallConfigPtr = &marshallConfig;
			/* Finishing tau_initMarshall() ? */
			if (marshallInitFirstTime == TRUE)
			{
				/* Set dataSet in marshall table */
				err = tau_initMarshall(&marshallConfig.pRefCon, numComId, pComIdDsIdMap, numDataset, apDataset);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_initMarshall() returns error = %d\n", err);
					return err;
				}
				else
				{
					/* Set marshallInitFirstTIme : FALSE */
					marshallInitFirstTime = FALSE;
				}
			}
		}

		/* Create Use MD Send Subnet ID */
		pUseMdSendSubnetId = (INT8 *)vos_memAlloc(sizeof(INT8));
		if (pUseMdSendSubnetId == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"tau_ldInit() Failed. Use MD Send SubnetId vos_memAlloc() Err\n");
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize Use MD Send Subnet ID */
			*pUseMdSendSubnetId = ifIndex +1;
			/* Set MD Config pRefcon : MD Send Subnet ID */
			arraySessionConfig[ifIndex].mdConfig.pRefCon = pUseMdSendSubnetId;
		}

		/* Set Own IP Address */
		if (pIfConfig[ifIndex].hostIp == IP_ADDRESS_NOTHING)
		{
			/* Set Real I/F Address */
			/* Is I/F subnet1 ? */
			if (ifIndex == IF_INDEX_SUBNET1)
			{
				ownIpAddress = subnetId1Address;
			}
			/* Is I/F subnet2 ? */
			else if(ifIndex == IF_INDEX_SUBNET2)
			{
				ownIpAddress = subnetId2Address;
			}
			else
			{
				vos_printLog(VOS_LOG_ERROR,"tau_ldInit() Failed. I/F Own IP Address Err.\n");
				return TRDP_PARAM_ERR;
			}
		}
		else
		{
			/* Set I/F Config Address */
			ownIpAddress = pIfConfig[ifIndex].hostIp;
		}

		/* Set PD Call Back Funcktion in MD Config */
		arraySessionConfig[ifIndex].pdConfig.pfCbFunction = &tau_ldRecvPdDs;
		/* Set MD Call Back Funcktion in MD Config */
		arraySessionConfig[ifIndex].mdConfig.pfCbFunction = &md_indication;

		/*  Open session for the interface  */
		err = tlc_openSession(&arraySessionConfig[ifIndex].sessionHandle,	/* appHandle */
								ownIpAddress,										/* own IP	*/
								pIfConfig[ifIndex].leaderIp,
								pMarshallConfigPtr,                   			/* Marshalling or no Marshalling		*/
								&arraySessionConfig[ifIndex].pdConfig,			/* PD Config */
								&arraySessionConfig[ifIndex].mdConfig,			/* MD Config */
								&arraySessionConfig[ifIndex].processConfig);	/* Process Config */
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tlc_openSession() error: %d interface: %s\n",
							err, pIfConfig[ifIndex].ifName);
			return err;
		}
	}
	/* TRDP Ladder Support Initialize */
	err = tau_ladder_init();
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. TRDP Ladder Support Initialize failed\n");
		return err;
	}
	/* Get Telegram Loop */
	for (ifIndex = 0; ifIndex < numIfConfig; ifIndex++)
	{
		err = configureTelegrams(ifIndex, numExchgPar, arrayExchgPar[ifIndex]);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. configureTelegrams() error.\n");
			return err;
		}
	}

	/* main Loop */
	/* Create TAUL PD Main Thread */
	err = taul_pd_main_proc_init();
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. taul_pd_main_proc_init() error.\n");
		return err;
	}

	return TRDP_NO_ERR;
}

/******************************************************************************/
/** Re-Initialize TAUL
 *  re-initialize Interface
 *
 *  @param[in]      subnetId			re-initialrequestize subnetId:SUBNET1 or SUBNET2
  *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_MUTEX_ERR
 */
TRDP_ERR_T tau_ldReInit (
	UINT32 subnetId)
{
	UINT32						subnetIndex = SUBNET_NO_1;		/* Subnet Index Number */
	TRDP_ERR_T					err = TRDP_NO_ERR;

	/* Check Parameter */
	if (subnetId == SUBNET1)
	{
		/* Covert subnet Index Number */
		subnetIndex = SUBNET_NO_1;
	}
	else if (subnetId == SUBNET2)
	{
		/* Covert subnet Index Number */
		subnetIndex = SUBNET_NO_2;
	}
	else
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldReInit() failed. SubnetId:%d Error.\n", subnetId);
		return TRDP_PARAM_ERR;
	}

	/* Close Session */
	err = tlc_closeSession(arraySessionConfig[subnetIndex].sessionHandle);
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Subnet%d tlc_closeSession() error = %d\n", subnetIndex + 1, err);
		return err;
	}
	else
	{
		/* Display TimeStamp when close Session time */
		vos_printLog(VOS_LOG_INFO, "%s Subnet%d Close Session.\n", vos_getTimeStamp(), subnetIndex + 1);
		/* Open Session */
		err = tlc_openSession(
				&arraySessionConfig[subnetIndex].sessionHandle,
				pIfConfig[subnetIndex].hostIp,
				pIfConfig[subnetIndex].leaderIp,
				&marshallConfig,
				&arraySessionConfig[subnetIndex].pdConfig,
				&arraySessionConfig[subnetIndex].mdConfig,
				&arraySessionConfig[subnetIndex].processConfig);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subnet%d tlc_closeSession() error = %d\n", subnetIndex + 1, err);
			return err;
		}
	}
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** Finalize TAUL
 *  Terminate PDComLadder and Clean up.
 *  Delete Traffic Store mutex, Traffic Store.
 *
 *	Note:
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T tau_ldTerminate (
	void)
{
	TRDP_ERR_T					err = TRDP_NO_ERR;
	TRDP_ERR_T					returnErrValue = TRDP_NO_ERR;
	VOS_ERR_T					vosErr = VOS_NO_ERR;
	extern VOS_THREAD_T 		taulPdMainThreadHandle;			/* Thread handle */
	PUBLISH_TELEGRAM_T		*iterPublishTelegram = NULL;
	SUBSCRIBE_TELEGRAM_T		*iterSubscribeTelegram = NULL;
	PD_REQUEST_TELEGRAM_T	*iterPdRequestTelegram = NULL;
	CALLER_TELEGRAM_T			*iterCallerTelegram = NULL;
	REPLIER_TELEGRAM_T		*iterReplierTelegram = NULL;
	LISTENER_HANDLE_T			*iterListenerHandle = NULL;
	CALLER_TELEGRAM_T			*pNextCallerTelegram = NULL;
	REPLIER_TELEGRAM_T		*pNextReplierTelegram = NULL;
	LISTENER_HANDLE_T			*pNextListenerHandle = NULL;
	UINT32						i = 0;

	/*  Free allocated memory - parsed telegram configuration */
	for (i=0; i < LADDER_IF_NUMBER ; i++)
	{
		tau_freeTelegrams(numExchgPar, arrayExchgPar[i]);
	}
	/* Free allocated memory */
	/* Free Communication Parameter */
	if (pComPar)
	{
		vos_memFree(pComPar);
		pComPar = NULL;
		numComPar = 0;
	}
	/* Free I/F Config */
	if (pIfConfig)
	{
		vos_memFree(pIfConfig);
		pIfConfig = NULL;
		numIfConfig = 0;
	}
	/* Free ComId-DatasetId Map */
	if (pComIdDsIdMap)
	{
		vos_memFree(pComIdDsIdMap);
		pComIdDsIdMap = NULL;
		numComId = 0;
	}
	/* Free Dataset */
	if (apDataset)
	{
		/* Free dataset structures */
		pTRDP_DATASET_T pDataset;
		UINT32 i;
		for (i = numDataset-1; i > 0; i--)
		{
			pDataset = apDataset[i];
			vos_memFree(pDataset);
		}
		/*  Free array of pointers to dataset structures    */
		vos_memFree(apDataset);
		apDataset = NULL;
		numDataset = 0;
	}
	/*  Free parsed xml document    */
	tau_freeXmlDoc(&xmlConfigHandle);

	/* UnPublish Loop */
	do
	{
		/* First Publish Telegram ? */
		if (iterPublishTelegram == NULL)
		{
			/* Is there Publish Telegram ? */
			if (pHeadPublishTelegram == NULL)
			{
				break;
			}
			else
			{
				/* Set First Publish Telegram  */
				iterPublishTelegram = pHeadPublishTelegram;
			}
		}
		else
		{
			/* Set Next Publish Telegram */
			iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram;
		}
		/* Check Publish comId Valid */
		if (iterPublishTelegram->comId > 0)
		{
			/* unPublish */
			err = tlp_unpublish(iterPublishTelegram->appHandle, iterPublishTelegram->pubHandle);
			if(err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "tau_ldterminate() failed. tlp_unpublish() error = %d\n",err);
				returnErrValue = err;
			}
			else
			{
				/* Display TimeStamp when unPublish time */
//				vos_printLog(VOS_LOG_DBG, "%s ComId:%d Destination IP Address:%d unPublish.\n", vos_getTimeStamp(), iterPublishTelegram->pubHandle->comId, iterPublishTelegram->pubHandle->destIpAddr);
				vos_printLog(VOS_LOG_DBG, "%s ComId:%d Destination IP Address:%d unPublish.\n", vos_getTimeStamp(), iterPublishTelegram->pubHandle->addr.comId, iterPublishTelegram->pubHandle->addr.destIpAddr);
			}
		}
		/* Free Publish Dataset */
		vos_memFree(iterPublishTelegram->dataset.pDatasetStartAddr);
		iterPublishTelegram->dataset.pDatasetStartAddr = NULL;
		iterPublishTelegram->dataset.size = 0;
		/* Free Publish Telegram */
		vos_memFree(iterPublishTelegram);
	} while(iterPublishTelegram->pNextPublishTelegram != NULL);
	/* Display TimeStamp when close Session time */
	vos_printLog(VOS_LOG_INFO, "%s All unPublish.\n", vos_getTimeStamp());

	/* UnSubscribe Loop */
	do
	{
		/* First Subscribe Telegram ? */
		if (iterSubscribeTelegram == NULL)
		{
			/* Is there Subscribe Telegram ? */
			if (pHeadSubscribeTelegram == NULL)
			{
				break;
			}
			else
			{
				/* Set First Subscribe Telegram  */
				iterSubscribeTelegram = pHeadSubscribeTelegram;
			}
		}
		else
		{
			/* Set Next Subscribe Telegram */
			iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram;
		}
		/* Check Susbscribe comId Valid */
		if (iterSubscribeTelegram->comId > 0)
		{
			/* unSubscribe */
			err = tlp_unsubscribe(iterSubscribeTelegram->appHandle, iterSubscribeTelegram->subHandle);
			if(err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "tau_ldterminate() failed. tlp_unsubscribe() error = %d\n",err);
				returnErrValue = err;
			}
			else
			{
				/* Display TimeStamp when unSubscribe time */
//				vos_printLog(VOS_LOG_DBG, "%s ComId:%d Destination IP Address:%d unSubscribe.\n", vos_getTimeStamp(), iterSubscribeTelegram->subHandle->comId, iterSubscribeTelegram->subHandle->destIpAddr);
				vos_printLog(VOS_LOG_DBG, "%s ComId:%d Destination IP Address:%d unSubscribe.\n", vos_getTimeStamp(), iterSubscribeTelegram->subHandle->addr.comId, iterSubscribeTelegram->subHandle->addr.destIpAddr);
			}
		}
		/* Free Subscribe Dataset */
		vos_memFree(iterSubscribeTelegram->dataset.pDatasetStartAddr);
		iterSubscribeTelegram->dataset.pDatasetStartAddr = NULL;
		iterSubscribeTelegram->dataset.size = 0;
		/* Free Subscribe Telegram */
		vos_memFree(iterSubscribeTelegram);
	} while(iterSubscribeTelegram->pNextSubscribeTelegram != NULL);
	/* Display TimeStamp when close Session time */
	vos_printLog(VOS_LOG_INFO, "%s All unSubscribe.\n", vos_getTimeStamp());

	/* Check RD Requester Telegram */
	if (pHeadPdRequestTelegram != NULL)
	{
		/* Delete PD Request Telegram Loop */
		for (iterPdRequestTelegram = pHeadPdRequestTelegram;
			  iterPdRequestTelegram->pNextPdRequestTelegram != NULL;
			  iterPdRequestTelegram = iterPdRequestTelegram->pNextPdRequestTelegram)
		{
			/* Free PD Request Dataset */
			vos_memFree(iterPdRequestTelegram->dataset.pDatasetStartAddr);
			iterPdRequestTelegram->dataset.pDatasetStartAddr = NULL;
			iterPdRequestTelegram->dataset.size = 0;
			/* Free PD Request Telegram */
			vos_memFree(iterPdRequestTelegram);
		}
	}
	else
	{
		/* Don't Delete PD Telegram */
	}

	/* TAUL MAIN Thread Terminate */
	vosErr = vos_threadTerminate(taulPdMainThreadHandle);
	if (vosErr != VOS_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "TRDP TAULpdMainThread Terminate failed. VOS Error: %d\n", vosErr);
		returnErrValue = TRDP_THREAD_ERR;
	}

	/* Ladder Terminate */
	err = 	tau_ladder_terminate();
	if(err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldTerminate failed. tau_ladder_terminate() error = %d\n",err);
		returnErrValue = err;
	}
	else
	{
		/* Display TimeStamp when tau_ladder_terminate time */
		vos_printLog(VOS_LOG_INFO, "%s TRDP Ladder Terminate.\n", vos_getTimeStamp());
	}

	/* Check Listener Handle (manual AddListener Handle) */
	if (pHeadListenerHandleList != NULL)
	{
		/* Delete Listener Handle Loop */
		for (iterListenerHandle = pHeadListenerHandleList;
			iterListenerHandle->pNextListenerHandle != NULL;
			iterListenerHandle = pNextListenerHandle)
		{
			/* Check Listener enable ? */
			if (iterListenerHandle->pTrdpListenerHandle != NULL)
			{
				/* Delete Listener */
				err = tlm_delListener(iterListenerHandle->appHandle, iterListenerHandle->pTrdpListenerHandle);
				if(err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "tau_ldTerminate() failed. tlm_delListener() error = %d\n",err);
					returnErrValue = err;
				}
			}
			/* Set Next Listener Handle Pointer */
			pNextListenerHandle = iterListenerHandle->pNextListenerHandle;
			/* Free Listener Handle */
			vos_memFree(iterListenerHandle);
		}
		/* Display TimeStamp when close Session time */
		vos_printLog(VOS_LOG_INFO, "%s All Listener Delete.\n", vos_getTimeStamp());
	}
	else
	{
		/* Don't tlm_delListener() */
	}

	/* MD Terminate */
	/* Check Caller Telegram */
	if (pHeadCallerTelegram != NULL)
	{
		/* Delete Caller Listener Loop */
		for (iterCallerTelegram = pHeadCallerTelegram;
			  iterCallerTelegram->pNextCallerTelegram != NULL;
			  iterCallerTelegram = pNextCallerTelegram)
		{
			/* Check Listener enable ? */
			if (iterCallerTelegram->listenerHandle != NULL)
			{
				/* Delete Listener */
				err = tlm_delListener(iterCallerTelegram->appHandle, iterCallerTelegram->listenerHandle);
				if(err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "tau_ldTerminate() failed. tlm_delListener() error = %d\n",err);
					returnErrValue = err;
				}
			}
			/* Set Next Caller Telegram Pointer */
			pNextCallerTelegram = iterCallerTelegram->pNextCallerTelegram;
			/* Free Caller Dataset */
			vos_memFree(iterCallerTelegram->dataset.pDatasetStartAddr);
			iterCallerTelegram->dataset.pDatasetStartAddr = NULL;
			iterCallerTelegram->dataset.size = 0;
			/* Free Publish Telegram */
			vos_memFree(iterCallerTelegram);
		}
		/* Display TimeStamp when close Session time */
		vos_printLog(VOS_LOG_INFO, "%s All Caller Delete.\n", vos_getTimeStamp());
	}
	else
	{
		/* Don't tlm_delListener() */
	}

	/* Check Replier Telegram */
	if (pHeadReplierTelegram != NULL)
	{
		/* Delete Replier Listener Loop */
		for (iterReplierTelegram = pHeadReplierTelegram;
			  iterReplierTelegram->pNextReplierTelegram != NULL;
			  iterReplierTelegram = pNextReplierTelegram)
		{
			/* Check Listener enable ? */
			if (iterReplierTelegram->listenerHandle != NULL)
			{
				/* Delete Listener */
				err = tlm_delListener(iterReplierTelegram->appHandle, iterReplierTelegram->listenerHandle);
				if(err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "tau_ldTerminate() failed. tlm_delListener() error = %d\n",err);
					returnErrValue = err;
				}
			}
			/* Set Next Replier Telegram Pointer */
			pNextReplierTelegram = iterReplierTelegram->pNextReplierTelegram;
			/* Free Replier Dataset */
			vos_memFree(iterReplierTelegram->dataset.pDatasetStartAddr);
			iterReplierTelegram->dataset.pDatasetStartAddr = NULL;
			iterReplierTelegram->dataset.size = 0;
			/* Free Replier Telegram */
			vos_memFree(iterReplierTelegram);
		}
		/* Display TimeStamp when close Session time */
		vos_printLog(VOS_LOG_INFO, "%s All Replier Delete.\n", vos_getTimeStamp());
	}
	else
	{
		/* Don't tlm_delListener() */
	}


	/*	Close appHandle session(Subnet1) */
	if (appHandle != NULL)
	{
		err = tlc_closeSession(appHandle);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subnet1 tlc_closeSession() error = %d\n",err);
			returnErrValue = err;
		}
		else
		{
			/* Display TimeStamp when close Session time */
			vos_printLog(VOS_LOG_INFO, "%s Subnet1 Close Session.\n", vos_getTimeStamp());
		}
	}

	/*	Close appHandle2 session(Subnet2) */
	if (appHandle2 != NULL)
	{
		err = tlc_closeSession(appHandle2);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subnet2 tlc_closeSession() error = %d\n",err);
			returnErrValue = err;
		}
		else
		{
			/* Display TimeStamp when close Session time */
			vos_printLog(VOS_LOG_INFO, "%s Subnet2 Close Session.\n", vos_getTimeStamp());
		}
	}

	/* TRDP Terminate */
	if (appHandle != NULL)
	{
		err = tlc_terminate();
		if(err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tlc_terminate() error = %d\n",err);
			returnErrValue = err;
		}
		else
		{
			/* Display TimeStamp when tlc_terminate time */
			vos_printLog(VOS_LOG_INFO, "%s TRDP Terminate.\n", vos_getTimeStamp());
		}
	}
	return returnErrValue;
}

/**********************************************************************************************************************/
/** TAUL API for PD
 */
/**********************************************************************************************************************/
/** Set the network Context.
 *
 *  @param[in]      SubnetId			Sub-network Id: SUBNET1 or SUBNET2 or SUBNET_AUTO
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter error
 *  @retval         TRDP_UNKNOWN_ERR	tau_setNetworkContext() error
 *  @retval         TRDP_IO_ERR			link down error
 */
TRDP_ERR_T  tau_ldSetNetworkContext (
    UINT32          subnetId)
{
	TRDP_ERR_T err = TRDP_NO_ERR;
	BOOL8 linkUpDown = TRUE;		                /* Link Up Down information TRUE:Up FALSE:Down */

	/* Check SubnetId Type */
	switch (subnetId)
	{
		case SUBNET_AUTO:
			/* Subnet1 Link Up ? */
			tau_checkLinkUpDown(SUBNET1, &linkUpDown);
			/* Subnet1 is Link Up */
			if (linkUpDown == TRUE)
			{
				/* Set network context: Subnet1 */
				err = tau_setNetworkContext(SUBNET1);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. tau_setNetworkContext() error\n");
					err = TRDP_UNKNOWN_ERR;
				}
				else
				{
					vos_printLog(VOS_LOG_DBG, "tau_ldSetNetworkContext() set subnet%d\n", SUBNET_NO_1 + 1);
				}
			}
			else
			{
				/* Subnet2 Link Up ? */
				tau_checkLinkUpDown(SUBNET2, &linkUpDown);
				/* Subnet2 is Link Up */
				if (linkUpDown == TRUE)
				{
					/* Set network context: Subnet2 */
					err = tau_setNetworkContext(SUBNET2);
					if (err != TRDP_NO_ERR)
					{
						vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. tau_setNetworkContext() error\n");
						err = TRDP_UNKNOWN_ERR;
					}
					else
					{
						vos_printLog(VOS_LOG_DBG, "tau_ldSetNetworkContext() set subnet%d\n", SUBNET_NO_2 + 1);
					}
				}
				else
				{
					vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. Subnet1 and Subnet2 is Link Down.\n");
					err = TRDP_IO_ERR;
				}
			}
		break;
		case SUBNET1:
			/* Set network context: Subnet1 */
			err = tau_setNetworkContext(SUBNET1);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. tau_setNetworkContext() error\n");
				err = TRDP_UNKNOWN_ERR;
			}
			else
			{
				vos_printLog(VOS_LOG_DBG, "tau_ldSetNetworkContext() set subnet%d\n", SUBNET_NO_1 +1 );
			}
		break;
		case SUBNET2:
			/* Set network context: Subnet2 */
			err = tau_setNetworkContext(SUBNET2);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. tau_setNetworkContext() error\n");
				err = TRDP_UNKNOWN_ERR;
			}
			else
			{
				vos_printLog(VOS_LOG_DBG, "tau_ldSetNetworkContext() set subnet%d\n", SUBNET_NO_2 +1);
			}
		break;
		default:
			vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. SubnetId error\n");
			err = TRDP_PARAM_ERR;
		break;
	}
	return err;
}

/**********************************************************************************************************************/
/** Get the sub-Network Id for the current network Context.
 *
 *  @param[in,out]  pSubnetId			pointer to Sub-network Id
 *  										Sub-network Id: SUBNET1 or SUBNET2
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter error
 *  @retval         TRDP_UNKNOWN_ERR	tau_getNetworkContext() error
 */
TRDP_ERR_T  tau_ldGetNetworkContext (
    UINT32          *pSubnetId)
{
	TRDP_ERR_T err = TRDP_NO_ERR;

	/* Check Parameter */
	if (pSubnetId == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldGetNetworkContext() failed. pSubnetId error\n");
		err = TRDP_PARAM_ERR;
	}
	else
	{
		/* Get Network Context */
		if (tau_getNetworkContext(pSubnetId) != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tau_ldGetNetworkContext() failed. tau_getNetworkContext() error\n");
			err = TRDP_UNKNOWN_ERR;
		}
	}
	return err;
}

/**********************************************************************************************************************/
/** Get Traffic Store accessibility.
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter error
 *  @retval         TRDP_NOPUB_ERR		not published
 *  @retval         TRDP_NOINIT_ERR	handle invalid
 */
TRDP_ERR_T  tau_ldLockTrafficStore (
    void)
{
	TRDP_ERR_T err = TRDP_NO_ERR;

	err = tau_lockTrafficStore();
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldLockTrafficeStore() failed\n");
		return err;
	}
	return err;
}

/**********************************************************************************************************************/
/** Release Traffic Store accessibility.
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter error
 *  @retval         TRDP_NOPUB_ERR		not published
 *  @retval         TRDP_NOINIT_ERR	handle invalid
 */
TRDP_ERR_T  tau_ldUnlockTrafficStore (
    void)
{
	TRDP_ERR_T					err = TRDP_NO_ERR;

	err = tau_unlockTrafficStore();
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldUnlockTrafficeStore() failed\n");
		return err;
	}
	return err;
}

/**********************************************************************************************************************/
/** TAUL API for MD
 */
/******************************************************************************/
/** Initiate sending MD notification message.
 *  Send a MD notification message
 *
 *  @param[in]		comId					comId of packet to be sent Notify
 *  @param[in]		dstIpAddr				where to send the Notify packet to
 *  @param[in]		dstURI					only functional group of destination URI
 *  @param[in]		pData					pointer to packet data / dataset
 *  @param[in]		dataSize				size of packet data
 *
 *	@retval			TRDP_NO_ERR			no error
 *	@retval			TRDP_PARAM_ERR		parameter error
 */
TRDP_ERR_T tau_ldNotify (
	UINT32						comId,
	TRDP_IP_ADDR_T			dstIpAddr,
	const TRDP_URI_USER_T	dstURI,
	const UINT8				*pData,
	UINT32						dataSize)
{
	TRDP_ERR_T				err = TRDP_NO_ERR;
	CALLER_TELEGRAM_T		*pNotifyCallerTelegram = NULL;			/* Pointer to Notify Caller Telegram */
	TRDP_IP_ADDR_T		srcIpAddr = 0;
	UINT8					*pMarshallData = NULL;
	UINT8					*pSendData = NULL;

	/* Check Parameter */
	/* Check comId */
	if (comId <= 0)
	{
       vos_printLog(VOS_LOG_ERROR, "tau_ldNotify() comId error. comId:%d\n", comId);
		return TRDP_PARAM_ERR;
	}
	/* Check Dataset */
	if ((pData == NULL) || (dataSize <= 0))
	{
       vos_printLog(VOS_LOG_ERROR, "tau_ldNotify() Dataset error\n");
       return TRDP_PARAM_ERR;
	}

	/* Get MD Source IP Address */
	if (useMdSendSubnetType == MD_SEND_USE_SUBNET1)
	{
		/* Set Sending MD Source IP Address : Subnet1 Address */
		srcIpAddr = subnetId1Address;
	}
	else
	{
		/* Set Sending MD Source IP Address : Subnet2 Address */
		srcIpAddr = subnetId2Address;
	}

	/* Get Caller Telegram by comId */
	pNotifyCallerTelegram  = searchCallerTelegramList(
									pHeadCallerTelegram,
									comId,
									srcIpAddr,
									dstIpAddr,
									FALSE);
	if (pNotifyCallerTelegram == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldNotify() Failed. Get Notify Caller Telegram Err.\n");
	}
	else
	{
		/* Check Marshalling Kind : Marshalling Enable */
		if ((pNotifyCallerTelegram->pMdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
		{
			/* Get marshall Dataset area */
			pMarshallData = (UINT8 *)vos_memAlloc(dataSize);
			if (pMarshallData == NULL)
			{
				vos_printLog(VOS_LOG_ERROR,"tau_ldNotify() Failed. Marshall Dataset vos_memAlloc() Err\n");
				return TRDP_MEM_ERR;
			}
			else
			{
				/* Initialize Marshall Dataset */
				memset(pMarshallData, 0, dataSize);
			}
			/* marshalling */
			err = tau_marshall(
					&marshallConfig.pRefCon,							/* pointer to user context*/
					comId,												/* comId */
					(UINT8 *)pData,									/* source pointer to received original message */
					pMarshallData,									/* destination pointer to a buffer for the treated message */
					&dataSize,											/* destination Buffer Size */
					&pNotifyCallerTelegram->pDatasetDescriptor);	/* pointer to pointer of cached dataset */
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "tau_ldNotify() tau_marshall Failed. comId:%d returns error %d\n", comId, err);
				return err;
			}
			else
			{
				/* Set pSendData pointer */
				pSendData = pMarshallData;
			}
		}
		else
		{
			/* Set pSendData pointer */
			pSendData = (UINT8 *)pData;
		}

		/* Send Notify (Notification) */
		err = tlm_notify(
				pNotifyCallerTelegram->appHandle,								/* the handle returned by tlc_openSession */
				pNotifyCallerTelegram->pUserRef,								/* user supplied value returned with reply */
				comId,																/* comId of packet to be sent */
				pNotifyCallerTelegram->topoCount,								/* topocount to use */
				srcIpAddr,															/* srcIP Address */
				dstIpAddr,															/* where to send the packet to */
				pNotifyCallerTelegram->pMdParameter->flags,					/* OPTION FLAG */
				pNotifyCallerTelegram->pSendParam,								/* optional pointer to send parameter */
				(const UINT8 *)pSendData,										/* pointer to packet data or dataset */
				dataSize,															/* size of packet data */
				*pNotifyCallerTelegram->pSource->pUriHost1,					/* source URI */
				dstURI);															/* destination URI */
		if (err != TRDP_NO_ERR)
		{
			/* Error : Send Notification */
			vos_printLog(VOS_LOG_ERROR, "Send Notification ERROR\n");
		}
		/* Check Marshalling Kind : Marshalling Enable */
		if ((pNotifyCallerTelegram->pMdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
		{
			/* Free Marshall Data */
			vos_memFree(pMarshallData);
		}
	}
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** Initiate sending MD request message.
 *  Send a MD request message
 *
 *  @param[in]		comId					comId of packet to be sent Request
 *  @param[in]		dstIpAddr				where to send the Request packet to
 *  @param[in]		dstURI					only functional group of destination URI
 *  @param[in]		numReplies				number of expected replies, 0 if unknown
 *  @param[in]		replyTimeout			timeout for reply
 *  @param[in]		pData					pointer to packet data / dataset
 *  @param[in]		dataSize				size of packet data
 *  @param[in]		callerRef				caller Reference
 *
 *	@retval			TRDP_NO_ERR			no error
 *	@retval			TRDP_PARAM_ERR		parameter error
 */
TRDP_ERR_T tau_ldRequest (
	UINT32						comId,
	TRDP_IP_ADDR_T			dstIpAddr,
	const TRDP_URI_USER_T	dstURI,
	UINT32						numReplies,
	UINT32						replyTimeout,
	const UINT8				*pData,
	UINT32						dataSize,
	void						*callerRef)
{
	TRDP_ERR_T						err = TRDP_NO_ERR;
	CALLER_TELEGRAM_T				*pRequestCallerTelegram = NULL;					/* Pointer to Request Caller Telegram */
	CALLER_TELEGRAM_T				*pReplyCallerTelegram = NULL;					/* Pointer to Reply Caller Telegram */
	WAITING_RECEIVE_REPLY_T		*pCheckWaitingReceiveReplyReference = NULL;	/* Pointer to Waiting Receive Request Reference for check callerRef */
	WAITING_RECEIVE_REPLY_T		*pNewWaitingReceiveReplyReference = NULL;		/* Pointer to Waiting Receive Request Reference */
	TRDP_IP_ADDR_T				srcIpAddr = 0;
	UINT8							*pMarshallData = NULL;
	UINT8							*pSendData = NULL;

	/* Check Parameter */
	/* Check comId */
	if (comId <= 0)
	{
       vos_printLog(VOS_LOG_ERROR, "tau_ldRequest() comId error. comId:%d\n", comId);
		return TRDP_PARAM_ERR;
	}
	/* Check Dataset */
	if ((pData == NULL) || (dataSize <= 0))
	{
       vos_printLog(VOS_LOG_ERROR, "tau_ldRequest() Dataset error\n");
       return TRDP_PARAM_ERR;
	}
	/* Check CallerRef */
	if (callerRef == NULL)
	{
	   vos_printLog(VOS_LOG_ERROR, "tau_ldRequest() callerRef error\n");
	   return TRDP_PARAM_ERR;
	}
	/* Check verify Waiting Receive Reply Reference */
	pCheckWaitingReceiveReplyReference = searchWaitingReceiveReplyReference(pHeadWaitingReceiveReplyReference, callerRef, SESSION_ID_NOTHING);
	if (pCheckWaitingReceiveReplyReference != NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldRequest() callerRef error\n");
		return TRDP_PARAM_ERR;
	}

	/* Create Waiting Receive Reply Reference */
	pNewWaitingReceiveReplyReference = (WAITING_RECEIVE_REPLY_T *)vos_memAlloc(sizeof(WAITING_RECEIVE_REPLY_T));
	if (pNewWaitingReceiveReplyReference == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"tau_ldRequest() Failed. Waiting Receive Reply Reference vos_memAlloc() Err\n");
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize Waiting Receive Reply Reference */
		memset(pNewWaitingReceiveReplyReference, 0, sizeof(WAITING_RECEIVE_REPLY_T));
	}
	/* Get Waiting Receive Reply Reference memory area */
	pNewWaitingReceiveReplyReference->pTaulReference = (UINT32 *)vos_memAlloc(sizeof(UINT32));
	if (pNewWaitingReceiveReplyReference->pTaulReference == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"tau_ldRequest() Failed. Waiting Receive Reply TAUL Reference vos_memAlloc() Err\n");
		/* Free Waiting Receive Reply Reference */
		vos_memFree(pNewWaitingReceiveReplyReference);
		return TRDP_MEM_ERR;
	}
	else
	{
		/* Initialize pNewWaitingReceiveReplyReference TAUL Reference */
		memset(pNewWaitingReceiveReplyReference->pTaulReference, 0, sizeof(UINT32));
	}

	/* Get MD Source IP Address */
	if (useMdSendSubnetType == MD_SEND_USE_SUBNET1)
	{
		/* Set Sending MD Source IP Address : Subnet1 Address */
		srcIpAddr = subnetId1Address;
	}
	else
	{
		/* Set Sending MD Source IP Address : Subnet2 Address */
		srcIpAddr = subnetId2Address;
	}

	/* Get Caller Telegram in Reply by comId */
	pReplyCallerTelegram  = searchCallerTelegramList(
									pHeadCallerTelegram,
									comId,
									srcIpAddr,
									dstIpAddr,
									TRUE);
	if (pReplyCallerTelegram == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldRequest() Failed. Get Reply Caller Telegram Err.\n");
		/* Free Waiting Receive Reply Reference */
		vos_memFree(pNewWaitingReceiveReplyReference);
		/* Free WaitingReceiveReplyReference TAUL Reference */
		vos_memFree(pNewWaitingReceiveReplyReference->pTaulReference);
		return TRDP_PARAM_ERR;
	}
	else
	{
		/* Set Waiting Receive Reply Reference */
		/* Set TAUL Reference */
		*pNewWaitingReceiveReplyReference->pTaulReference = SEND_REFERENCE | sequenceNumber;
		sequenceNumber++;
		/* Set Caller Reference */
		/* Get CallerRef memory area */
		pNewWaitingReceiveReplyReference->callerReference = (UINT32 *)vos_memAlloc(sizeof(UINT32));
		if (pNewWaitingReceiveReplyReference->callerReference == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Waiting Receive Reply Reference :callerRef vos_memAlloc() Err\n");
			/* Free Waiting Receive Reply Reference */
			vos_memFree(pNewWaitingReceiveReplyReference);
			/* Free WaitingReceiveReplyReference TAUL Reference */
			vos_memFree(pNewWaitingReceiveReplyReference->pTaulReference);
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize Waiting Receive Reply Reference :CallerRef */
			memset(pNewWaitingReceiveReplyReference->callerReference, 0, sizeof(UINT32));
		}
		/* Set CallerRef */
		memcpy(pNewWaitingReceiveReplyReference->callerReference, callerRef, sizeof(UINT32));
		/* Set Reply ComId */
		pNewWaitingReceiveReplyReference->replyComId = pReplyCallerTelegram->comId;
		/* Set Destination IP Address */
		pNewWaitingReceiveReplyReference->dstIpAddr = dstIpAddr;
		/* Set Destination URI */
		memcpy(pNewWaitingReceiveReplyReference->dstURI, dstURI, sizeof(TRDP_URI_USER_T));
#if 0
		/* Append Waiting Receive Reply Reference Queue */
		err = appendWaitingReceiveReplyQueue(&pHeadWaitingReceiveReplyReference, pNewWaitingReceiveReplyReference);
		if (err != TRDP_NO_ERR)
		{
			/* Free Waiting Receive Reply Reference */
			vos_memFree(pNewWaitingReceiveReplyReference);
			vos_printLog(VOS_LOG_ERROR, "tau_ldRequest() Failed. Waiting Receive Reply Reference appendWaitingReceiveReplyQueue() Err:%d\n", err);
			return err;
		}
#endif
	}

	/* Get Caller Telegram in Request by comId */
	pRequestCallerTelegram  = searchCallerTelegramList(
									pHeadCallerTelegram,
									comId,
									srcIpAddr,
									dstIpAddr,
									FALSE);
	if (pRequestCallerTelegram == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldRequest() Failed. Get Request Caller Telegram Err.\n");
		/* Free Waiting Receive Reply Reference */
		vos_memFree(pNewWaitingReceiveReplyReference);
		/* Free WaitingReceiveReplyReference TAUL Reference */
		vos_memFree(pNewWaitingReceiveReplyReference->pTaulReference);
		return TRDP_PARAM_ERR;
	}
	else
	{
		/* Check Marshalling Kind : Marshalling Enable */
		if ((pRequestCallerTelegram->pMdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
		{
			/* Get marshall Dataset area */
			pMarshallData = (UINT8 *)vos_memAlloc(dataSize);
			if (pMarshallData == NULL)
			{
				vos_printLog(VOS_LOG_ERROR,"tau_ldRequest() Failed. Marshall Dataset vos_memAlloc() Err\n");
				/* Free Waiting Receive Reply Reference */
				vos_memFree(pNewWaitingReceiveReplyReference);
				/* Free WaitingReceiveReplyReference TAUL Reference */
				vos_memFree(pNewWaitingReceiveReplyReference->pTaulReference);
				return TRDP_MEM_ERR;
			}
			else
			{
				/* Initialize Marshall Dataset */
				memset(pMarshallData, 0, dataSize);
			}
			/* marshalling */
			err = tau_marshall(
					&marshallConfig.pRefCon,													/* pointer to user context*/
					comId,																		/* comId */
					(UINT8 *)pData,															/* source pointer to received original message */
					pMarshallData,															/* destination pointer to a buffer for the treated message */
					&dataSize,																	/* destination Buffer Size */
					&pRequestCallerTelegram->pDatasetDescriptor);							/* pointer to pointer of cached dataset */
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "tau_ldRequest() tau_marshall Failed. comId:%d returns error %d\n", comId, err);
				/* Free Waiting Receive Reply Reference */
				vos_memFree(pNewWaitingReceiveReplyReference);
				/* Free WaitingReceiveReplyReference TAUL Reference */
				vos_memFree(pNewWaitingReceiveReplyReference->pTaulReference);
				/* Free Marshall Data */
				vos_memFree(pMarshallData);
				return err;
			}
			else
			{
				/* Set pSendData pointer */
				pSendData = pMarshallData;
			}
		}
		else
		{
			/* Set pSendData pointer */
			pSendData = (UINT8 *)pData;
		}

		/* Send Request */
		err = tlm_request(
				pRequestCallerTelegram->appHandle,									/* the handle returned by tlc_openSession */
				pNewWaitingReceiveReplyReference->callerReference,				/* user supplied value returned with reply */
				&pRequestCallerTelegram->sessionId,								/* return session ID */
				comId,																	/* comId of packet to be sent */
				pRequestCallerTelegram->topoCount,									/* topocount to use */
				srcIpAddr,																/* srcIP Address */
				dstIpAddr,																/* where to send the packet to */
				pRequestCallerTelegram->pMdParameter->flags,						/* OPTION FLAG */
				numReplies,															/* number of expected repliers */
				pRequestCallerTelegram->pMdParameter->replyTimeout,				/* timeout for reply */
				pRequestCallerTelegram->pSendParam,								/* optional pointer to send parameter */
				(const UINT8 *)pSendData,																	/* pointer to packet data or dataset */
				dataSize,																/* size of packet data */
				*pRequestCallerTelegram->pSource->pUriHost1,						/* source URI */
				dstURI);																/* destination URI */
		if (err != TRDP_NO_ERR)
		{
			/* Error : Send Request */
			vos_printLog(VOS_LOG_ERROR, "Send Request ERROR\n");
#if 0
			/* Delete Waiting Receive Reply Reference Queue */
			err = deleteWaitingReceiveReplyQueue(&pHeadWaitingReceiveReplyReference, pNewWaitingReceiveReplyReference);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "tau_ldRequest() Failed. deleteWaitingReceiveReplyQueue() Err:%d\n", err);
			}
#endif
			/* Free Waiting Receive Reply Reference */
			vos_memFree(pNewWaitingReceiveReplyReference);
			return err;
		}
		/* Set Session Id */
		memcpy(pNewWaitingReceiveReplyReference->sessionId, pRequestCallerTelegram->sessionId, sizeof(TRDP_UUID_T));
		vos_printLog(VOS_LOG_INFO, "tau_ldRequest() MD caller session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
				pRequestCallerTelegram->sessionId[0], pRequestCallerTelegram->sessionId[1], pRequestCallerTelegram->sessionId[2], pRequestCallerTelegram->sessionId[3],
				pRequestCallerTelegram->sessionId[4], pRequestCallerTelegram->sessionId[5], pRequestCallerTelegram->sessionId[6], pRequestCallerTelegram->sessionId[7]);
		/* Append Waiting Receive Reply Reference Queue */
		err = appendWaitingReceiveReplyQueue(&pHeadWaitingReceiveReplyReference, pNewWaitingReceiveReplyReference);
		if (err != TRDP_NO_ERR)
		{
			/* Free Waiting Receive Reply Reference */
			vos_memFree(pNewWaitingReceiveReplyReference);
			vos_printLog(VOS_LOG_ERROR, "tau_ldRequest() Failed. Waiting Receive Reply Reference appendWaitingReceiveReplyQueue() Err:%d\n", err);
			return err;
		}
	}
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** Send a MD reply message or replyQuery message.
 *  Send a MD reply message after receiving an request
 *  or
 *  Send a MD reply message after receiving a request and ask for confirmation.
 *
 *  @param[in]		comId					comId of packet to be sent Reply or ReplyQuery
 *  @param[in]		userStatus				Info for requester about application errors
 *  @param[in]		pData					pointer to packet data / dataset
 *  @param[in]		dataSize				size of packet data
 *  @param[in]		sessionRef				session Reference
 *
 *	@retval			TRDP_NO_ERR			no error
 *	@retval			TRDP_PARAM_ERR		parameter error
 */
TRDP_ERR_T tau_ldReply (
	UINT32						comId,
	UINT32						userStatus,
	const UINT8				*pData,
	UINT32						dataSize,
	void						*sessionRef)
{
	TRDP_ERR_T						err = TRDP_NO_ERR;
	REPLIER_TELEGRAM_T			*pReplyReplierTelegram = NULL;						/* Pointer to Reply or ReplyQuery from Replier Telegram */
	REPLIER_TELEGRAM_T			*pConfirmReplierTelegram = NULL;					/* Pointer to Confirm from Replier Telegram */
	WAITING_SEND_REPLY_T			*pCheckWaitingSendReplyReference = NULL;			/* Pointer to Waiting Send Request Reference for check sessionRef */
	TRDP_IP_ADDR_T				srcIpAddr = 0;
	TRDP_TIME_T					nowTime = {0};
	TRDP_APP_SESSION_T			replierAppHandle = {0};
	UINT8							*pMarshallData = NULL;
	UINT8							*pSendData = NULL;

	/* Check Parameter */
	/* Check comId */
	if (comId <= 0)
	{
       vos_printLog(VOS_LOG_ERROR, "tau_ldReply() comId error. comId:%d\n", comId);
		return TRDP_PARAM_ERR;
	}
	/* Check Dataset */
	if ((pData == NULL) || (dataSize <= 0))
	{
       vos_printLog(VOS_LOG_ERROR, "tau_ldReply() Dataset error\n");
       return TRDP_PARAM_ERR;
	}
	/* Check SessionRef */
	if (sessionRef == NULL)
	{
	   vos_printLog(VOS_LOG_ERROR, "tau_ldReply() sessionRef error\n");
	   return TRDP_PARAM_ERR;
	}
	pCheckWaitingSendReplyReference = searchWaitingSendReplyReference(pHeadWaitingSendReplyReference, sessionRef);
	if (pCheckWaitingSendReplyReference == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldReply() sessionRef error\n");
		return TRDP_PARAM_ERR;
	}

	/* Get Now Time */
	vos_getTime(&nowTime);
	/* Check Send of Time Limit */
	if (vos_cmpTime((TRDP_TIME_T *)&pCheckWaitingSendReplyReference->sendReplyTimeLimit, (TRDP_TIME_T *)&nowTime) < 0)
	{
		/* Expire Send of Time Limit */
		vos_printLog(VOS_LOG_ERROR, "tau_ldReply() Expire Send of Time Limit error.\n");
		/* Delete WaitingSendReplyReference from Waiting Send Reply Queue */
		err = deleteWaitingSendReplyQueue(&pHeadWaitingSendReplyReference, pCheckWaitingSendReplyReference);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tau_ldReply() deleteWaitingSendPeplyQueue() error.\n");
		}
		return TRDP_TIMEOUT_ERR;
	}

	/* Get MD Source IP Address */
	if (useMdSendSubnetType == MD_SEND_USE_SUBNET1)
	{
		/* Set Sending MD Source IP Address : Subnet1 Address */
		srcIpAddr = subnetId1Address;
		/* Set Application Handle : appHandle */
		replierAppHandle = appHandle;
	}
	else
	{
		/* Set Sending MD Source IP Address : Subnet2 Address */
		srcIpAddr = subnetId2Address;
		/* Set Application Handle : appHandle2 */
		replierAppHandle = appHandle2;
	}

	/* Get Reply or ReplyQuery Telegram by Reply comId */
	pReplyReplierTelegram  = searchReplierTelegramList(
									pHeadReplierTelegram,
									comId,
									srcIpAddr,
									pCheckWaitingSendReplyReference->replyDstIpAddr,
									FALSE);
	if (pReplyReplierTelegram == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldReply() Failed. Get Reply or ReplyQuery Telegram Err.\n");
		return TRDP_PARAM_ERR;
	}

	/* Check Marshalling Kind : Marshalling Enable */
	if ((pReplyReplierTelegram->pMdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
	{
		/* Get marshall Dataset area */
		pMarshallData = (UINT8 *)vos_memAlloc(dataSize);
		if (pMarshallData == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"tau_ldReply() Failed. Marshall Dataset vos_memAlloc() Err\n");
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize Marshall Dataset */
			memset(pMarshallData, 0, dataSize);
		}
		/* marshalling */
		err = tau_marshall(
				&marshallConfig.pRefCon,													/* pointer to user context*/
				comId,																		/* comId */
				(UINT8 *)pData,															/* source pointer to received original message */
				pMarshallData,															/* destination pointer to a buffer for the treated message */
				&dataSize,																	/* destination Buffer Size */
				&pReplyReplierTelegram->pDatasetDescriptor);							/* pointer to pointer of cached dataset */
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tau_ldReply() tau_marshall Failed. comId:%d returns error %d\n", comId, err);
			return err;
		}
		else
		{
			/* Set pSendData pointer */
			pSendData = pMarshallData;
		}
	}
	else
	{
		/* Set pSendData pointer */
		pSendData = (UINT8 *)pData;
	}

	/* Decide Send Reply Message Type */
	if (pReplyReplierTelegram->pMdParameter->confirmTimeout == 0)
	{
vos_printLog(VOS_LOG_INFO, "tlm_reply() session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
		pCheckWaitingSendReplyReference->pMdInfo->sessionId[0], pCheckWaitingSendReplyReference->pMdInfo->sessionId[1], pCheckWaitingSendReplyReference->pMdInfo->sessionId[2],
		pCheckWaitingSendReplyReference->pMdInfo->sessionId[3],	pCheckWaitingSendReplyReference->pMdInfo->sessionId[4], pCheckWaitingSendReplyReference->pMdInfo->sessionId[5],
		pCheckWaitingSendReplyReference->pMdInfo->sessionId[6], pCheckWaitingSendReplyReference->pMdInfo->sessionId[7])

		/* Send Reply */
		err = tlm_reply (
			replierAppHandle,																			/* the handle returned by tlc_init */
			sessionRef,																				/* user supplied value returned with reply */
			(const TRDP_UUID_T *)&(pCheckWaitingSendReplyReference->pMdInfo->sessionId),		/* Session ID returned by indication */
			pCheckWaitingSendReplyReference->pMdInfo->topoCount,									/* topocount to use */
			comId,																						/* comId of packet to be sent */
			srcIpAddr,																					/* srcIP Address */
			pCheckWaitingSendReplyReference->replyDstIpAddr,										/* where to send the packet to */
			pReplyReplierTelegram->pMdParameter->flags,											/* pktFlags 0 = Default = appHandle->mdDefault.flag */
			userStatus, 																				/* userStatus */
			pReplyReplierTelegram->pSendParam,														/* send param */
			(const UINT8 *)pSendData,																						/* pointer to packet data or dataset */
			dataSize,																					/* size of packet data */
			pCheckWaitingSendReplyReference->pMdInfo->destURI,									/* source URI */
			pCheckWaitingSendReplyReference->replyDstURI);									/* destination URI */
		if (err != TRDP_NO_ERR)
		{
			/* Error : Send Reply */
			vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
		}
		else
		{
			/* Delete WaitingSendReplyReference from Waiting Send Reply Queue */
			err = deleteWaitingSendReplyQueue(&pHeadWaitingSendReplyReference, pCheckWaitingSendReplyReference);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "tau_ldReply() deleteWaitingSendPeplyQueue() error.\n");
			}
		}
	}
	else
	{
vos_printLog(VOS_LOG_INFO, "tlm_replyQuery() session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
		pCheckWaitingSendReplyReference->pMdInfo->sessionId[0], pCheckWaitingSendReplyReference->pMdInfo->sessionId[1], pCheckWaitingSendReplyReference->pMdInfo->sessionId[2],
		pCheckWaitingSendReplyReference->pMdInfo->sessionId[3],	pCheckWaitingSendReplyReference->pMdInfo->sessionId[4], pCheckWaitingSendReplyReference->pMdInfo->sessionId[5],
		pCheckWaitingSendReplyReference->pMdInfo->sessionId[6], pCheckWaitingSendReplyReference->pMdInfo->sessionId[7])

		/* Send ReplyQuery */
		err = tlm_replyQuery (
			replierAppHandle,																				/* the handle returned by tlc_init */
			sessionRef,																					/* user supplied value returned with reply */
			(const TRDP_UUID_T *)&(pCheckWaitingSendReplyReference->pMdInfo->sessionId),			/* Session ID returned by indication */
			pCheckWaitingSendReplyReference->pMdInfo->topoCount,										/* topocount to use */
			comId,																							/* comId of packet to be sent */
			pCheckWaitingSendReplyReference->pMdInfo->destIpAddr,									/* srcIP Address */
			pCheckWaitingSendReplyReference->pMdInfo->srcIpAddr,										/* where to send the packet to */
			pReplyReplierTelegram->pMdParameter->flags, 												/* pktFlags 0 = Default = appHandle->mdDefault.flag */
			userStatus, 																					/* userStatus */
			pCheckWaitingSendReplyReference->confirmTimeout,											/* confirm timeout */
			pReplyReplierTelegram->pSendParam, 														/* send param */
			pData,																							/* pointer to packet data or dataset */
			dataSize,																						/* size of packet data */
			pCheckWaitingSendReplyReference->pMdInfo->destURI,										/* source URI */
			pCheckWaitingSendReplyReference->pMdInfo->srcURI);										/* destination URI */
		if (err != TRDP_NO_ERR)
		{
			/* Error : Send Reply */
			vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
		}
		else
		{
			/* Get Replier Telegram in Confirm by comId */
			pConfirmReplierTelegram  = searchReplierTelegramList(
											pHeadReplierTelegram,
											comId,
											srcIpAddr,
											pCheckWaitingSendReplyReference->pMdInfo->srcIpAddr,
											TRUE);
			if (pConfirmReplierTelegram == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "tau_ldReply() Failed. Get Confirm From Replier Telegram Err.\n");
				return TRDP_PARAM_ERR;
			}
			else
			{
				/* Delete WaitingSendReplyReference from Waiting Send Reply Queue */
				err = deleteWaitingSendReplyQueue(&pHeadWaitingSendReplyReference, pCheckWaitingSendReplyReference);
				if (err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "tau_ldReply() deleteWaitingSendPeplyQueue() error.\n");
				}
			}
		}
	}
	return TRDP_NO_ERR;
}

/******************************************************************************/
/** Subscribe to MD messages.
 *  Add a listener to TRDP to get notified when messages are received
 *
 *  @param[in]		comId					comId to be observed
 *  @param[in]		dstIpAdd				multicast group to listen on, unicast is 0
 *  @param[in]		destURI				only functional group of destination URI
 *  @param[in]		userRef				user Reference
 *
 *	@retval			TRDP_NO_ERR			no error
 *	@retval			TRDP_PARAM_ERR		parameter error
 */
TRDP_ERR_T tau_ldAddListener (
	UINT32						comId,
	TRDP_IP_ADDR_T			dstIpAddr,
	const TRDP_URI_USER_T	dstURI,
	void						*userRef)
{
	TRDP_ERR_T						err = TRDP_NO_ERR;

	WAITING_RECEIVE_REQUEST_T	*pCheckWaitingReceiveRequestReference = NULL;		/* Pointer to Waiting Receive Request Reference for check callerRef */
	WAITING_RECEIVE_REQUEST_T	*pNewWaitingReceiveRequestReference = NULL;		/* Pointer to Waiting Receive Request Reference */
	TRDP_LIS_T						pTrdpListenerHandle = NULL;							/* TRDP Listener Handle by tlm_addListener */
	LISTENER_HANDLE_T				*pNewListenerHandle = NULL;							/* AddListnerId Handle */
	UINT32							ifIndex = 0;											/* I/F Index (subnet1:0, subnet2:1) */
	TRDP_APP_SESSION_T			listenerAppHandle;

	/* Check Parameter */
	/* Check comId */
	if (comId <= 0)
	{
	   vos_printLog(VOS_LOG_ERROR, "tau_ldAddListener() comId error. comId:%d\n", comId);
		return TRDP_PARAM_ERR;
	}
	/* Check userRef */
	if (userRef == NULL)
	{
	   vos_printLog(VOS_LOG_ERROR, "tau_ldAddListener() sessionRef error\n");
	   return TRDP_PARAM_ERR;
	}
	pCheckWaitingReceiveRequestReference = searchWaitingReceiveRequestReference(pHeadWaitingReceiveRequestReference, userRef);
	if (pCheckWaitingReceiveRequestReference != NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ldAddListener() userRef error\n");
		return TRDP_PARAM_ERR;
	}

	/* AddListener Subnet Loop */
	for(ifIndex = 0; ifIndex < 2; ifIndex++)
	{
		/* Subnet1 ? */
		if (ifIndex == 0)
		{
			/* Set Listener AppHandle: appHandle */
			listenerAppHandle = appHandle;
		}
		else
		{
			/* Set Listener AppHandle: appHandle2 */
			listenerAppHandle = appHandle2;
		}
		/* Create Listener Handle */
		pNewListenerHandle = (LISTENER_HANDLE_T *)vos_memAlloc(sizeof(LISTENER_HANDLE_T));
		if (pNewListenerHandle == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"tau_ldAddListener() Failed. Listener Handle vos_memAlloc() Err\n");
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize Listener Handle */
			memset(pNewListenerHandle, 0, sizeof(LISTENER_HANDLE_T));
		}

		/* Add Listener */
		err = tlm_addListener(
				listenerAppHandle,
				&pTrdpListenerHandle,
				userRef,												/* user supplied value returned with reply */
				comId,													/* comId to be observed */
				0,														/* topocount to use */
				dstIpAddr,												/* destination Address (Multicast Group) */
				arraySessionConfig[ifIndex].pdConfig.flags,		/* OPTION FLAG */
				dstURI);												/* destination URI */
		/* Check tlm_addListener Return Code */
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tau_ldAddListener() failed. tlm_addListener() error. AddListener comID = 0x%x error = %d\n", comId, err);
			/* Free Listener Handle */
			vos_memFree(pNewListenerHandle);
			return err;
		}
		else
		{
			/* Set Listener Handle List */
			pNewListenerHandle->appHandle = listenerAppHandle;
			pNewListenerHandle->pTrdpListenerHandle = pTrdpListenerHandle;
			if (appendListenerHandleList(&pHeadListenerHandleList, pNewListenerHandle) != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Set Listener Handle List error\n");
				/* Free Listener Handle */
				vos_memFree(pNewListenerHandle);
				return TRDP_PARAM_ERR;
			}
		}
		/* Create Waiting Receive Request Reference */
		pNewWaitingReceiveRequestReference = (WAITING_RECEIVE_REQUEST_T *)vos_memAlloc(sizeof(WAITING_RECEIVE_REQUEST_T));
		if (pNewWaitingReceiveRequestReference == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"tau_ldAddListener() Failed. Waiting Receive Request Reference vos_memAlloc() Err\n");
			/* Free Listener Handle */
			vos_memFree(pNewListenerHandle);
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize Waiting Receive Request Reference */
			memset(pNewWaitingReceiveRequestReference, 0, sizeof(WAITING_RECEIVE_REQUEST_T));
		}

		/* Get Waiting Receive Request Reference memory area */
		pNewWaitingReceiveRequestReference->pTaulReference = (UINT32 *)vos_memAlloc(sizeof(UINT32));
		if (pNewWaitingReceiveRequestReference->pTaulReference == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"tau_ldAddListener() Failed. Waiting Receive Request TAUL Reference vos_memAlloc() Err\n");
			/* Free Listener Handle */
			vos_memFree(pNewListenerHandle);
			/* Free Waiting Receive Request Reference */
			vos_memFree(pNewWaitingReceiveRequestReference);
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize pWaitingReceiveRequestReference TAUL Reference */
			memset(pNewWaitingReceiveRequestReference->pTaulReference, 0, sizeof(UINT32));
		}
		/* Set Waiting Receive Reply Reference */
		/* Set TAUL Reference */
		*pNewWaitingReceiveRequestReference->pTaulReference = RECEIVE_REFERENCE;
		/* Get CallerRef memory area */
		pNewWaitingReceiveRequestReference->callerReference = (UINT32 *)vos_memAlloc(sizeof(UINT32));
		if (pNewWaitingReceiveRequestReference->callerReference == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"tau_ldAddListener() Failed. Waiting Receive Request Reference :callerRef vos_memAlloc() Err\n");
			/* Free Listener Handle */
			vos_memFree(pNewListenerHandle);
			/* Free Waiting Receive Request Reference */
			vos_memFree(pNewWaitingReceiveRequestReference);
			/* Free Waiting Receive Request Reference TAUL Reference */
			vos_memFree(pNewWaitingReceiveRequestReference->pTaulReference);
			return TRDP_MEM_ERR;
		}
		else
		{
			/* Initialize Waiting Receive Request Reference :CallerRef */
			memset(pNewWaitingReceiveRequestReference->callerReference, 0, sizeof(UINT32));
		}
		/* Set CallerRef */
		memcpy(pNewWaitingReceiveRequestReference->callerReference, userRef, sizeof(UINT32));
		/* Set Request ComId */
		pNewWaitingReceiveRequestReference->requestComId = comId;
		/* Set Destination IP Address */
		pNewWaitingReceiveRequestReference->dstIpAddr = dstIpAddr;
		/* Set Destination URI */
		memcpy(pNewWaitingReceiveRequestReference->dstURI, dstURI, sizeof(TRDP_URI_USER_T));
		/* Append Waiting Receive Reply Reference Queue */
		err = appendWaitingReceiveRequestQueue(&pHeadWaitingReceiveRequestReference, pNewWaitingReceiveRequestReference);
		if (err != TRDP_NO_ERR)
		{
			/* Free Listener Handle */
			vos_memFree(pNewListenerHandle);
			/* Free Waiting Receive Request Reference */
			vos_memFree(pNewWaitingReceiveRequestReference);
			/* Free Waiting Receive Request Reference TAUL Reference */
			vos_memFree(pNewWaitingReceiveRequestReference->pTaulReference);
			vos_printLog(VOS_LOG_ERROR, "tau_ldAddListener() Failed. appendWaitingReceiveRequestQueue() Err:%d\n", err);
			return err;
		}
	}
	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** callback function PD receive
 *
 *  @param[in]		pRefCon			user supplied context pointer
 *  @param[in]		appHandle			application handle returned by tlc_opneSession
 *  @param[in]		pPDInfo			pointer to PDInformation
 *  @param[in]		pData				pointer to receive PD Data
 *  @param[in]		dataSize       	receive PD Data Size
 *
 */
void tau_ldRecvPdDs (
    void *pRefCon,
    TRDP_APP_SESSION_T appHandle,
    const TRDP_PD_INFO_T *pPDInfo,
    UINT8 *pData,
    UINT32 dataSize)
{
	UINT32 subnetId;											/* Using Sub-network Id */
	UINT32 displaySubnetId = SUBNETID_TYPE1;				/* Using Sub-network Id for Display log */
	UINT16 offset = 0;										/* Traffic Store Offset Address */
	extern UINT8 *pTrafficStoreAddr;						/* pointer to pointer to Traffic Store Address */
	TRDP_ADDRESSES_T addr = {0};
	PD_ELE_T *pSubscriberElement = NULL;
	SUBSCRIBE_TELEGRAM_T *pSubscribeTelegram = NULL;
	TRDP_ERR_T					err = TRDP_NO_ERR;

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

	/* check parameter */
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
				memset((void *)((INT32)pTrafficStoreAddr + (INT32)offset), 0, pSubscriberElement->dataSize);
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

			/* Search Receive PD in Subscribe Telegram */
			pSubscribeTelegram = searchSubscribeTelegramList (
										pHeadSubscribeTelegram,
										pPDInfo->comId,
										pPDInfo->srcIpAddr,
										pPDInfo->destIpAddr);
			if (pSubscribeTelegram == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "tau_ldRecvPdDs() failed. Nothing Subscribe Telegram. comId:%d, srcIpAddr:0x%x, destIpAddr:0x%x\n",
								pPDInfo->comId,
								pPDInfo->srcIpAddr,
								pPDInfo->destIpAddr);
				tau_unlockTrafficStore();
				return;
			}
			else
			{
				/* Get offset Address */
				memcpy(&offset, (void *)pPDInfo->pUserRef, sizeof(offset));
				/* Check Marshalling Kind : Marshalling Enable */
				if ((pSubscribeTelegram->pPdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
				{
					/* unmarshalling */
					err = tau_unmarshall(
								&marshallConfig.pRefCon,												/* pointer to user context*/
								pPDInfo->comId,														/* comId */
								pData,																	/* source pointer to received original message */
								(UINT8 *)((INT32)pTrafficStoreAddr + (INT32)offset),				/* destination pointer to a buffer for the treated message */
								&pSubscribeTelegram->dataset.size,									/* destination Buffer Size */
								&pSubscribeTelegram->pDatasetDescriptor);							/* pointer to pointer of cached dataset */
					if (err != TRDP_NO_ERR)
					{
						vos_printLog(VOS_LOG_ERROR, "tau_unmarshall PD DATASET%d returns error %d\n", DATASET_NO_1, err);
						tau_unlockTrafficStore();
						return;
					}
					else
					{
						vos_printLog(VOS_LOG_DBG, "*** Traffic Store Write TIme (unMarshalling) *** comId:%d subnetId:%d character:%d seqCount:%d\n",
														addr.comId, displaySubnetId, *(pData+1), pPDInfo->seqCount);
						tau_unlockTrafficStore();
					}
				}
				/* Marshalling Disable */
				else
				{
					/* Set received PD Data in Traffic Store */
					memcpy((void *)((INT32)pTrafficStoreAddr + (INT32)offset), pData, dataSize);
					vos_printLog(VOS_LOG_DBG, "*** Traffic Store Write TIme *** comId:%d subnetId:%d character:%d seqCount:%d\n",
								addr.comId, displaySubnetId, *(pData+1), pPDInfo->seqCount);
					tau_unlockTrafficStore();
				}
			}
		}
	}
	else
	{
		tau_unlockTrafficStore();
	}
}

/**********************************************************************************************************************/
/** call back function for message data
 *
 *  @param[in]		pRefCon			user supplied context pointer
 *  @param[in]		appHandle			application handle returned by tlc_opneSession
 *  @param[in]		pMsg				pointer to MD Information
 *  @param[in]		pData				pointer to receive MD Data
 *  @param[in]		dataSize       	receive MD Data Size
 *
 */
void md_indication(
    void					*pRefCon,
    TRDP_APP_SESSION_T	appHandle,
    const TRDP_MD_INFO_T *pMsg,
    UINT8                *pData,
    UINT32               dataSize)
{
	WAITING_RECEIVE_REQUEST_T	*pCheckWaitingReceiveRequestReference = NULL;
	WAITING_SEND_REPLY_T			*pWaitingSendReplyReference = NULL;
	REPLIER_TELEGRAM_T			*pReplyReplierTelegram = NULL;
	WAITING_RECEIVE_REPLY_T		*pCheckWaitingReceiveReplyReference = NULL;
	CALLER_TELEGRAM_T				*pConfirmCallerTelegram = NULL;
	WAITING_RECEIVE_CONFIRM_T	*pCheckWaitingReceiveConfirmReference = NULL;
	UINT8							*pMdData = NULL;
	UINT32							reference = 0;
	UINT32							sessionRef = 0;
	TRDP_TIME_T					trdp_time_tv_interval = {0};						/* interval Time :TRDP_TIME_T type for TRDP function */
	TRDP_ERR_T						err = TRDP_NO_ERR;
	TRDP_UUID_T					replySessionId = {0};
	CALLER_TELEGRAM_T				*pCallerTelegram = NULL;
	REPLIER_TELEGRAM_T			*pReplierTelegram = NULL;
	UINT8							*pUnMarshallData = NULL;
	UINT8							*pUnMarshallDataStartAddress = NULL;
	UINT8 							receiveSubnetId = 0;

	vos_printLog(VOS_LOG_INFO, "md_indication(r=%p m=%p d=%p l=%d comId=%u msgType=0x%x"
			" sessionId=%02x%02x%02x%02x%02x%02x%02x%02x,"
			" numExpReplies=%u numReplies=%u numRepliesQuery=%u resuletCode=%d)"
			" srcIpAddr=%d destIpAddr=%d\n",
			pRefCon,
			pMsg,
			pData,
			dataSize,pMsg->comId,
			pMsg->msgType,
			pMsg->sessionId[0], pMsg->sessionId[1], pMsg->sessionId[2], pMsg->sessionId[3],
			pMsg->sessionId[4], pMsg->sessionId[5], pMsg->sessionId[6], pMsg->sessionId[7],
			pMsg->numExpReplies,
			pMsg->numReplies,
			pMsg->numRepliesQuery,
			pMsg->resultCode,
			pMsg->srcIpAddr,
			pMsg->destIpAddr);

#if 0
	/* Check Dataset Size */
	if (dataSize > 0)
	{
		/* Get MD Message Data memory area */
		pMdData = (UINT8 *)vos_memAlloc(dataSize);
		if (pMdData == NULL)
		{
			vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Receive MD Dataset vos_memAlloc() Err\n");
			return;
		}
		else
		{
			/* Initialize MD Message Data Area */
			memset(pMdData, 0, dataSize);
			/* Set Receive MD Message Data */
			memcpy(pMdData, pData, dataSize);
		}
	}
#endif
	/* Check Dataset Size */
	if (dataSize > 0)
	{
		pMdData = pData;
	}
	/* Decide MD Message Type */
	switch(pMsg->msgType)
	{
		/* Receive Notify (Notification) */
		case TRDP_MSG_MN:
			/* Get User Reference */
			memcpy(&reference, (void *)pMsg->pUserRef, sizeof(reference));
			/* Check TAUL Reference : Message Listen ? */
			if ((reference & RECEIVE_REFERENCE) == RECEIVE_REFERENCE)
			{
				/* Check TAUL Reference in Waiting Receive Request */
				pCheckWaitingReceiveRequestReference = searchWaitingReceiveRequestReference(
																	pHeadWaitingReceiveRequestReference,
																	(void *)pMsg->pUserRef);
				if (pCheckWaitingReceiveRequestReference != NULL)
				{
					/* Search Receive MD in Replier Telegram */
					pReplierTelegram = searchReplierTelegramList (
												pHeadReplierTelegram,
												pMsg->comId,
												pMsg->srcIpAddr,
												pMsg->destIpAddr,
												FALSE);
					if (pReplierTelegram == NULL)
					{
						vos_printLog(VOS_LOG_ERROR, "md_indication() failed. Nothing Replier Telegram. comId:%d, srcIpAddr:0x%x, destIpAddr:0x%x\n",
										pMsg->comId,
										pMsg->srcIpAddr,
										pMsg->destIpAddr);
						return;
					}
					else
					{
						/* Check Marshalling Kind : Marshalling Enable */
						if ((pReplierTelegram->pMdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
						{
							/* Get marshall Dataset area */
							pUnMarshallData = (UINT8 *)vos_memAlloc(pReplierTelegram->dataset.size);
							if (pUnMarshallData == NULL)
							{
								vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Marshall Dataset vos_memAlloc() Err\n");
								return;
							}
							else
							{
								/* Initialize Marshall Dataset */
								memset(pUnMarshallData, 0, pReplierTelegram->dataset.size);
							}
							/* Set UnMarshalling Start Address */
							pUnMarshallDataStartAddress = pUnMarshallData;
							/* unmarshalling */
							err = tau_unmarshall(
										&marshallConfig.pRefCon,							/* pointer to user context*/
										pMsg->comId,										/* comId */
										pData,												/* source pointer to received original message */
										pUnMarshallData,									/* destination pointer to a buffer for the treated message */
										&pReplierTelegram->dataset.size,				/* destination Buffer Size */
										&pReplierTelegram->pDatasetDescriptor);		/* pointer to pointer of cached dataset */
							if (err != TRDP_NO_ERR)
							{
								vos_printLog(VOS_LOG_ERROR, "tau_unmarshall comId%d returns error %d\n", pMsg->comId, err);
								/* Free Marshall Data */
								vos_memFree(pUnMarshallData);
								return;
							}
							else
							{
								/* Set MD Data */
								pMdData = pUnMarshallDataStartAddress;
								/* Clear Receive MD Dataset */
								memset(pData, 0, dataSize);
								/* Free Marshall Data */
								vos_memFree(pData);
							}
						}
					}

					/* Check rcvConf Call Back Function */
					if (taulConfig.rcvConf != NULL)
					{
						/* Set SessionRef */
						sessionRef = 0;
						/* Call Back rcvConf() */
						taulConfig.rcvConf(&sessionRef, pMsg, pMdData, dataSize);
					}
					else
					{
						vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. rcvConf() Not Setting CallBack Error.\n");
						/* Free Marshall Data */
						vos_memFree(pUnMarshallData);
						return;
					}
				}
			}
		break;
		/* Receive Request */
		case TRDP_MSG_MR:
			/* Get User Reference */
			memcpy(&reference, (void *)pMsg->pUserRef, sizeof(reference));
			/* Check TAUL Reference : Message Listen ? */
			if ((reference & RECEIVE_REFERENCE) == RECEIVE_REFERENCE)
			{
				/* Check TAUL Reference in Waiting Receive Request */
				pCheckWaitingReceiveRequestReference = searchWaitingReceiveRequestReference(
																	pHeadWaitingReceiveRequestReference,
																	(void *)pMsg->pUserRef);
				if (pCheckWaitingReceiveRequestReference != NULL)
				{
					/* Search Receive MD in Replier Telegram */
					pReplierTelegram = searchReplierTelegramList (
												pHeadReplierTelegram,
												pMsg->comId,
												pMsg->srcIpAddr,
												pMsg->destIpAddr,
												FALSE);
					if (pReplierTelegram == NULL)
					{
						vos_printLog(VOS_LOG_ERROR, "md_indication() failed. Nothing Replier Telegram. comId:%d, srcIpAddr:0x%x, destIpAddr:0x%x\n",
										pMsg->comId,
										pMsg->srcIpAddr,
										pMsg->destIpAddr);
						return;
					}
					else
					{
						/* Check Marshalling Kind : Marshalling Enable */
						if ((pReplierTelegram->pMdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
						{
							/* Get marshall Dataset area */
							pUnMarshallData = (UINT8 *)vos_memAlloc(pReplierTelegram->dataset.size);
							if (pUnMarshallData == NULL)
							{
								vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Marshall Dataset vos_memAlloc() Err\n");
								return;
							}
							else
							{
								/* Initialize Marshall Dataset */
								memset(pUnMarshallData, 0, pReplierTelegram->dataset.size);
							}
							/* Set UnMarshalling Start Address */
							pUnMarshallDataStartAddress = pUnMarshallData;
							/* unmarshalling */
							err = tau_unmarshall(
										&marshallConfig.pRefCon,						/* pointer to user context*/
										pMsg->comId,									/* comId */
										pData,											/* source pointer to received original message */
										pUnMarshallData,								/* destination pointer to a buffer for the treated message */
										&pReplierTelegram->dataset.size,			/* destination Buffer Size */
										&pReplierTelegram->pDatasetDescriptor);	/* pointer to pointer of cached dataset */
							if (err != TRDP_NO_ERR)
							{
								vos_printLog(VOS_LOG_ERROR, "tau_unmarshall comId%d returns error %d\n", pMsg->comId, err);
								/* Free Marshall Data */
								vos_memFree(pUnMarshallData);
								return;
							}
							else
							{
								/* Set MD Data */
								pMdData = pUnMarshallDataStartAddress;
								/* Clear Receive MD Dataset */
								memset(pData, 0, dataSize);
								/* Free Marshall Data */
								vos_memFree(pData);
							}
						}
					}

					/* Check rcvConf Call Back Function */
					if (taulConfig.rcvConf != NULL)
					{
						/* Get SubnetId of Receive MD Packet */
						memcpy(&receiveSubnetId, pRefCon, sizeof(INT8));
						/* Check Receive MD Packet by SUbnet1 */
						if (receiveSubnetId == SUBNET_ID_1)
						{
							/* Set SessionRef */
							sessionRef = sessionCounter & RECEIVE_SUBNET1_MASK;
							sessionCounter++;
						}
						/* Check Receive MD Packet by SUbnet2 */
						else if (receiveSubnetId == SUBNET_ID_2)
						{
							/* Set SessionRef */
							sessionRef = sessionCounter | RECEIVE_SUBNET2_MASK;
							sessionCounter++;
						}
						else
						{
							vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Receive Subnet Err\n");
							return;
						}
#if 0
						/* Set SessionRef */
						sessionRef = sessionCounter;
						sessionCounter++;
#endif
#if 0
						/* Call Back rcvConf() */
						taulConfig.rcvConf(&sessionRef, pMsg, pMdData, dataSize);
#endif
						/* Create Waiting Receive Request Reference */
						/* Get Waiting Send Reply Reference memory area */
						pWaitingSendReplyReference = (WAITING_SEND_REPLY_T *)vos_memAlloc(sizeof(WAITING_SEND_REPLY_T));
						if (pWaitingSendReplyReference == NULL)
						{
							vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Waiting Send Reply Reference vos_memAlloc() Err\n");
							return;
						}
						else
						{
							/* Initialize pWaitingSendReplyReference */
							memset(pWaitingSendReplyReference, 0, sizeof(WAITING_SEND_REPLY_T));
						}
						/* Get Reply or ReplyQuery Telegram by Request comId */
						pReplyReplierTelegram  = searchReplierTelegramList(
														pHeadReplierTelegram,
														pMsg->comId,
														pMsg->srcIpAddr,
														pMsg->destIpAddr,
														TRUE);
						if (pReplyReplierTelegram == NULL)
						{
							vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. Get Reply or ReplyQuery Telegram Err.\n");
							/* Free Waiting Send Reply Reference */
							vos_memFree(pWaitingSendReplyReference);
							return;
						}
						/* Get Now Time */
						vos_getTime(&pWaitingSendReplyReference->sendReplyTimeLimit);
						/* Convert Reply Timeout into TRDP_TIME_T */
						trdp_time_tv_interval.tv_sec = pReplyReplierTelegram->pMdParameter->replyTimeout / 1000000;
						trdp_time_tv_interval.tv_usec = pReplyReplierTelegram->pMdParameter->replyTimeout % 1000000;
						/* Set Send Reply Time Limit */
						vos_addTime(&pWaitingSendReplyReference->sendReplyTimeLimit, &trdp_time_tv_interval);
						/* Get SessionRef memory area */
						pWaitingSendReplyReference->sessionReference = (UINT32 *)vos_memAlloc(sizeof(UINT32));
						if (pWaitingSendReplyReference->sessionReference == NULL)
						{
							vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Waiting Send Reply Reference :sessionRef vos_memAlloc() Err\n");
							/* Free Wating Send Reply Reference */
							vos_memFree(pWaitingSendReplyReference);
							return;
						}
						else
						{
							/* Initialize Replier Application Thread Handle */
							memset(pWaitingSendReplyReference->sessionReference, 0, sizeof(UINT32));
						}
						/* Set SessionRef */
						memcpy(pWaitingSendReplyReference->sessionReference, &sessionRef, sizeof(UINT32));
						/* Set Destination IP Address */
						pWaitingSendReplyReference->replyDstIpAddr = pMsg->srcIpAddr;
						/* Set Destination URI */
						memcpy(pWaitingSendReplyReference->replyDstURI, pMsg->srcURI, sizeof(TRDP_URI_USER_T));

						/* Get MD Info memory area */
						pWaitingSendReplyReference->pMdInfo = (TRDP_MD_INFO_T *)vos_memAlloc(sizeof(TRDP_MD_INFO_T));
						if (pWaitingSendReplyReference->pMdInfo == NULL)
						{
							vos_printLog(VOS_LOG_ERROR,"md_indication() Failed.MD Info vos_memAlloc() Err\n");
							/* Free Wating Send Reply Reference */
							vos_memFree(pWaitingSendReplyReference);
							/* Free Wating Send Reply Reference Session Reference */
							vos_memFree(pWaitingSendReplyReference->sessionReference);
							return;
						}
						else
						{
							/* Initialize Replier Application Thread Handle */
							memset(pWaitingSendReplyReference->pMdInfo, 0, sizeof(TRDP_MD_INFO_T));
						}

						/* Set MD INFO */
						memcpy(pWaitingSendReplyReference->pMdInfo, pMsg, sizeof(TRDP_MD_INFO_T));
						/* Append Waiting Send Reply Reference */
						err = appendWaitingSendReplyQueue(&pHeadWaitingSendReplyReference, pWaitingSendReplyReference);
						if (err != TRDP_NO_ERR)
						{
							/* Free Waiting Send Reply Reference */
							vos_memFree(pWaitingSendReplyReference);
							/* Free Wating Send Reply Reference Session Reference */
							vos_memFree(pWaitingSendReplyReference->sessionReference);
							vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. appendWaitingSendReplyQueue() Err:%d\n", err);
							return;
						}
						vos_printLog(VOS_LOG_DBG, "appendWaitingSendReplyQueue() sessionRef value:%d\n", sessionRef);
						/* Call Back rcvConf() */
						taulConfig.rcvConf(&sessionRef, pMsg, pMdData, dataSize);
					}
					else
					{
						vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. rcvConf() Not Setting CallBack Error.\n");
						return;
					}
				}
				else
				{
					vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Receive Request TAUL Error\n");
					return;
				}
			}
		break;
		/* Receive Reply */
		case TRDP_MSG_MP:
			/* Get User Reference */
			memcpy(&reference, (void *)pMsg->pUserRef, sizeof(reference));
			/* Check TAUL Reference : Receive Request ? */
			if ((reference & SEND_REFERENCE) == SEND_REFERENCE)
			{
				/* Get Reply Session Id */
				memcpy(&replySessionId, pMsg->sessionId, sizeof(replySessionId));
				/* Check TAUL Reference in Waiting Receive Reply */
				pCheckWaitingReceiveReplyReference = searchWaitingReceiveReplyReference(
																	pHeadWaitingReceiveReplyReference,
																	(void *)pMsg->pUserRef,
																	replySessionId);
				if (pCheckWaitingReceiveReplyReference != NULL)
				{
					/* Search Receive MD in Caller Telegram */
					pCallerTelegram = searchCallerTelegramList (
												pHeadCallerTelegram,
												pMsg->comId,
												pMsg->srcIpAddr,
												pMsg->destIpAddr,
												FALSE);
					if (pCallerTelegram == NULL)
					{
						vos_printLog(VOS_LOG_ERROR, "md_indication() failed. Nothing Caller Telegram. comId:%d, srcIpAddr:0x%x, destIpAddr:0x%x\n",
										pMsg->comId,
										pMsg->srcIpAddr,
										pMsg->destIpAddr);
						return;
					}
					else
					{
						/* Check Marshalling Kind : Marshalling Enable */
						if ((pCallerTelegram->pMdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
						{
							/* Get marshall Dataset area */
							pUnMarshallData = (UINT8 *)vos_memAlloc(pCallerTelegram->dataset.size);
							if (pUnMarshallData == NULL)
							{
								vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Marshall Dataset vos_memAlloc() Err\n");
								return;
							}
							else
							{
								/* Initialize Marshall Dataset */
								memset(pUnMarshallData, 0, pCallerTelegram->dataset.size);
							}
							/* Set UnMarshalling Start Address */
							pUnMarshallDataStartAddress = pUnMarshallData;
							/* unmarshalling */
							err = tau_unmarshall(
										&marshallConfig.pRefCon,						/* pointer to user context*/
										pMsg->comId,									/* comId */
										pData,											/* source pointer to received original message */
										pUnMarshallData,								/* destination pointer to a buffer for the treated message */
										&pCallerTelegram->dataset.size,				/* destination Buffer Size */
										&pCallerTelegram->pDatasetDescriptor);		/* pointer to pointer of cached dataset */
							if (err != TRDP_NO_ERR)
							{
								vos_printLog(VOS_LOG_ERROR, "tau_unmarshall comId%d returns error %d\n", pMsg->comId, err);
								/* Free Marshall Data */
								vos_memFree(pUnMarshallData);
								return;
							}
							else
							{
								/* Set MD Data */
								pMdData = pUnMarshallDataStartAddress;
								/* Clear Receive MD Dataset */
								memset(pData, 0, dataSize);
								/* Free Marshall Data */
								vos_memFree(pData);
							}
						}
					}

					/* Check callConf Call Back Function */
					if (taulConfig.callConf != NULL)
					{
						/* Call Back callConf() */
						taulConfig.callConf(
								&pCheckWaitingReceiveReplyReference->callerReference,
								pMsg,
								pMdData,
								dataSize);
						/* Check Timeout or Expire Number of Receive Reply */
						if ((pMsg->resultCode == TRDP_REPLYTO_ERR)
							&& ((pMsg->numExpReplies > 0) && (pMsg->numReplies >= pMsg->numExpReplies)))
						{
							/* Delete Waiting Receive Reply Reference */
							err = deleteWaitingReceiveReplyQueue(&pHeadWaitingReceiveReplyReference, pCheckWaitingReceiveReplyReference);
							if (err != TRDP_NO_ERR)
							{
								vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. deleteWaitingReceiveReplyQueue() Err:%d\n", err);
							}
							/* Free Waiting Receive Reply Reference */
							vos_memFree(pCheckWaitingReceiveReplyReference);
							pCheckWaitingReceiveReplyReference = NULL;
						}
					}
					else
					{
						vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. callConf() Not Setting CallBack Error.\n");
						return;
					}
				}
				else
				{
					vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Receive Reply TAUL Error\n");
					return;
				}
			}
		break;
		/* Receive ReplyQuery */
		case TRDP_MSG_MQ:
#if 0
			/* Get User Reference */
			memcpy(&reference, (void *)pMsg->pUserRef, sizeof(reference));
			/* Check TAUL Reference : Receive Request ? */
			if ((reference & SEND_REFERENCE) == SEND_REFERENCE)
			{
#endif
				/* Get reply Session Id */
				memcpy(&replySessionId, pMsg->sessionId, sizeof(replySessionId));

				/* Check TAUL Reference in Waiting Receive Reply */
				pCheckWaitingReceiveReplyReference = searchWaitingReceiveReplyReference(
																	pHeadWaitingReceiveReplyReference,
																	(void *)pMsg->pUserRef,
																	replySessionId);
				if (pCheckWaitingReceiveReplyReference != NULL)
				{
					/* Check callConf Call Back Function */
					if (taulConfig.callConf != NULL)
					{
vos_printLog(VOS_LOG_INFO, "call callConf() comId: %d destIP: %d\n", pMsg->comId, pMsg->srcIpAddr);
						/* Search Receive MD in Caller Telegram */
						pCallerTelegram = searchCallerTelegramList (
													pHeadCallerTelegram,
													pMsg->comId,
													pMsg->srcIpAddr,
													pMsg->destIpAddr,
													FALSE);
						if (pCallerTelegram == NULL)
						{
							vos_printLog(VOS_LOG_ERROR, "md_indication() failed. Nothing Caller Telegram. comId:%d, srcIpAddr:0x%x, destIpAddr:0x%x\n",
											pMsg->comId,
											pMsg->srcIpAddr,
											pMsg->destIpAddr);
							return;
						}
						else
						{
							/* Check Marshalling Kind : Marshalling Enable */
							if ((pCallerTelegram->pMdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
							{
								/* Get marshall Dataset area */
								pUnMarshallData = (UINT8 *)vos_memAlloc(pCallerTelegram->dataset.size);
								if (pUnMarshallData == NULL)
								{
									vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Marshall Dataset vos_memAlloc() Err\n");
									return;
								}
								else
								{
									/* Initialize Marshall Dataset */
									memset(pUnMarshallData, 0, pCallerTelegram->dataset.size);
								}
								/* Set UnMarshalling Start Address */
								pUnMarshallDataStartAddress = pUnMarshallData;
								/* unmarshalling */
								err = tau_unmarshall(
											&marshallConfig.pRefCon,					/* pointer to user context*/
											pMsg->comId,								/* comId */
											pData,										/* source pointer to received original message */
											pUnMarshallData,							/* destination pointer to a buffer for the treated message */
											&pCallerTelegram->dataset.size,			/* destination Buffer Size */
											&pCallerTelegram->pDatasetDescriptor);	/* pointer to pointer of cached dataset */
								if (err != TRDP_NO_ERR)
								{
									vos_printLog(VOS_LOG_ERROR, "tau_unmarshall comId%d returns error %d\n", pMsg->comId, err);
									/* Free Marshall Data */
									vos_memFree(pUnMarshallData);
									return;
								}
								else
								{
#if 0
									/* Clear Receive MD Dataset */
									memset(pData, 0, dataSize);
									/* Set Marshall Data */
									memcpy(pData, pUnMarshallData, pCallerTelegram->dataset.size);
									/* Free Marshall Data */
									vos_memFree(pUnMarshallData);
#endif
									/* Set MD Data */
									pMdData = pUnMarshallDataStartAddress;
									/* Clear Receive MD Dataset */
									memset(pData, 0, dataSize);
									/* Free Marshall Data */
									vos_memFree(pData);
								}
							}
						}

						/* Call Back callConf() */
						taulConfig.callConf(
								&pCheckWaitingReceiveReplyReference->callerReference,
								pMsg,
								pMdData,
								dataSize);

						/* Get Confirm Telegram by ReplyQuery comId */
						pConfirmCallerTelegram  = searchCallerTelegramList(
														pHeadCallerTelegram,
														pMsg->comId,
														pMsg->srcIpAddr,
														pMsg->destIpAddr,
														TRUE);
						if (pConfirmCallerTelegram == NULL)
						{
							vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. Get Confirm Telegram Err.\n");
							return;
						}
						/* Send Confirm */
						err = tlm_confirm(
								pConfirmCallerTelegram->appHandle,						/* the handle returned by tlc_init */
								pConfirmCallerTelegram->pUserRef,						/* user supplied value returned with reply */
								(const TRDP_UUID_T *)&(pMsg->sessionId),				/* Session ID returned by request */
								pConfirmCallerTelegram->comId,							/* comId of packet to be sent */
								pMsg->topoCount,											/* topocount to use */
								pMsg->destIpAddr,											/* own IP address, 0 - srcIP will be set by the stack */
								pMsg->srcIpAddr,											/* where to send the packet to */
								pConfirmCallerTelegram->pMdParameter->flags,			/* OPTION: TRDP_FLAGS_CALLBACK */
								0,															/* Info for requester about application errors */
								pMsg->replyStatus,										/* Info for requester about stack errors */
								pConfirmCallerTelegram->pSendParam,					/* Pointer to send parameters, NULL to use default send parameters */
								pMsg->destURI,											/* only functional group of source URI */
								pMsg->srcURI);											/* only functional group of destination URI */
						if (err != TRDP_NO_ERR)
						{
							vos_printLog(VOS_LOG_ERROR, "Send Confirm ERROR:%d\n", err);
						}
						vos_printLog(VOS_LOG_INFO, "Send Confirm comId: %d destIP: %d\n", pConfirmCallerTelegram->comId, pMsg->srcIpAddr);
						/* Check Timeout or Expire Number of Receive Reply */
						if ((pMsg->resultCode == TRDP_APP_CONFIRMTO_ERR)
							&& ((pMsg->numExpReplies > 0) && (pMsg->numRepliesQuery >= pMsg->numExpReplies)))

						{
							/* Delete Waiting Receive Reply Reference */
							err = deleteWaitingReceiveReplyQueue(&pHeadWaitingReceiveReplyReference, pCheckWaitingReceiveReplyReference);
							if (err != TRDP_NO_ERR)
							{
								vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. deleteWaitingReceiveReplyQueue() Err:%d\n", err);
							}
							/* Free Waiting Receive Reply Reference */
							vos_memFree(pCheckWaitingReceiveReplyReference);
							pCheckWaitingReceiveReplyReference = NULL;
						}
					}
					else
					{
						vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. callConf() Not Setting CallBack Error.\n");
						return;
					}
				}
				else
				{
					vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Receive Reply TAUL Error\n");
					return;
				}
#if 0
			}
#endif
		break;
		/* Receive Confirm */
		case TRDP_MSG_MC:
			/* Get User Reference */
			memcpy(&reference, (void *)pMsg->pUserRef, sizeof(reference));
			/* Check TAUL Reference : Receive Request ? */
			if ((reference & RECEIVE_REFERENCE) == RECEIVE_REFERENCE)
			{
				/* Check Confirm Timeout */
				if (pMsg->resultCode == TRDP_REPLYTO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Receive Confirm Timeout\n");

					/* Get reply Session Id */
					memcpy(&replySessionId, pMsg->sessionId, sizeof(replySessionId));
					/* Check TAUL Reference in Waiting Receive Reply */
					pCheckWaitingReceiveReplyReference = searchWaitingReceiveReplyReference(
																		pHeadWaitingReceiveReplyReference,
																		(void *)pMsg->pUserRef,
																		replySessionId);
					/* Delete Waiting Receive Reply Reference */
					err = deleteWaitingReceiveReplyQueue(&pHeadWaitingReceiveReplyReference, pCheckWaitingReceiveReplyReference);
					if (err != TRDP_NO_ERR)
					{
						vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. deleteWaitingReceiveReplyQueue() Err:%d\n", err);
					}
					/* Free Waiting Receive Reply Reference */
					vos_memFree(pCheckWaitingReceiveReplyReference);
					pCheckWaitingReceiveReplyReference = NULL;

					/* Check callConf Call Back Function */
					if (taulConfig.callConf != NULL)
					{
						/* Search Receive MD in Replier Telegram */
						pReplierTelegram = searchReplierTelegramList (
													pHeadReplierTelegram,
													pMsg->comId,
													pMsg->srcIpAddr,
													pMsg->destIpAddr,
													FALSE);
						if (pReplierTelegram == NULL)
						{
							vos_printLog(VOS_LOG_ERROR, "md_indication() failed. Nothing Replier Telegram. comId:%d, srcIpAddr:0x%x, destIpAddr:0x%x\n",
											pMsg->comId,
											pMsg->srcIpAddr,
											pMsg->destIpAddr);
							return;
						}
						else
						{
							/* Check Marshalling Kind : Marshalling Enable */
							if ((pReplierTelegram->pMdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
							{
								/* Get marshall Dataset area */
								pUnMarshallData = (UINT8 *)vos_memAlloc(pReplierTelegram->dataset.size);
								if (pUnMarshallData == NULL)
								{
									vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Marshall Dataset vos_memAlloc() Err\n");
									return;
								}
								else
								{
									/* Initialize Marshall Dataset */
									memset(pUnMarshallData, 0, pReplierTelegram->dataset.size);
								}
								/* Set UnMarshalling Start Address */
								pUnMarshallDataStartAddress = pUnMarshallData;
								/* unmarshalling */
								err = tau_unmarshall(
											&marshallConfig.pRefCon,						/* pointer to user context*/
											pMsg->comId,									/* comId */
											pData,											/* source pointer to received original message */
											pUnMarshallData,								/* destination pointer to a buffer for the treated message */
											&pReplierTelegram->dataset.size,			/* destination Buffer Size */
											&pReplierTelegram->pDatasetDescriptor);	/* pointer to pointer of cached dataset */
								if (err != TRDP_NO_ERR)
								{
									vos_printLog(VOS_LOG_ERROR, "tau_unmarshall comId%d returns error %d\n", pMsg->comId, err);
									/* Free Marshall Data */
									vos_memFree(pUnMarshallData);
									return;
								}
								else
								{
									/* Set MD Data */
									pMdData = pUnMarshallDataStartAddress;
									/* Clear Receive MD Dataset */
									memset(pData, 0, dataSize);
									/* Free Marshall Data */
									vos_memFree(pData);
								}
							}
						}

						/* Call Back callConf() */
						taulConfig.callConf(
								pRefCon,
								pMsg,
								pMdData,
								dataSize);
					}
					return;
				}

				/* Check TAUL Reference in Waiting Receive Confirm */
				pCheckWaitingReceiveConfirmReference = searchWaitingReceiveConfirmReference(
																	pHeadWaitingReceiveConfirmReference,
																	(void *)pMsg->pUserRef);
				if (pCheckWaitingReceiveConfirmReference == NULL)
				{
					vos_printLog(VOS_LOG_ERROR,"md_indication() Failed. Receive Confirm TAUL Error\n");
					return;
				}
			}
		break;
		/* Receive Error */
		case TRDP_MSG_ME:
			vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. Reeicei Message Type : Me Error.\n");
		break;
		default:
			vos_printLog(VOS_LOG_ERROR, "md_indication() Failed. Reeicei Message Type Error.\n");
		break;
	}
	return;
}

#endif /* TRDP_OPTION_LADDER */
