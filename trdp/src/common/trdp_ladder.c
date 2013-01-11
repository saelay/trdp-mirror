/******************************************************************************/
/**
 * @file            trdp_ladder.c
 *
 * @brief           Functions for Ladder Support
 *
 * @details
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
 *
 */

#ifdef TRDP_OPTION_LADDER
/*******************************************************************************
 * INCLUDES
 */

#include "trdp_utils.h"
#include "trdp_if.h"

#include "vos_private.h"
#include "vos_thread.h"
#include "vos_shared_mem.h"
#include "trdp_ladder.h"
#include "trdp_ladder_app.h"

/*******************************************************************************
 * DEFINES
 */

/*******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 *   Locals
 */

/******************************************************************************
 *   Globals
 */

/* Traffic Store Mutex */
VOS_MUTEX_T pTrafficStoreMutex = NULL;					/* Pointer to Mutex for Traffic Store */
/* UINT32 mutexLockRetryTimeout = 1000;	*/				/* Mutex Lock Retry Timeout : Max. time in us to wait, 0 means forever */

/* Traffic Store */
CHAR8 TRAFFIC_STORE[] = "/ladder_ts";					/* Traffic Store shared memory name */
mode_t PERMISSION	 = 0666;							/* Traffic Store permission is rw-rw-rw- */
UINT8 *pTrafficStoreAddr;								/* pointer to pointer to Traffic Store Address */
VOS_SHRD_T  pTrafficStoreHandle;				/* Pointer to Traffic Store Handle */

/* PDComLadderThread */
CHAR8 pdComLadderThreadName[] ="PDComLadderThread";		/* Thread name is PDComLadder Thread. */
BOOL pdComLadderThreadActiveFlag = FALSE;				/* PDComLaader Thread active/noactive Flag :active=TRUE, nonActive=FALSE */
BOOL pdComLadderThreadStartFlag = FALSE;				/* PDComLadder Thread instruction start up Flag :start=TRUE, stop=FALSE */

/* Sub-net */
UINT32 usingSubnetId;									/* Using SubnetId */

/******************************************************************************/
/** Initialize TRDP Ladder Support
 *  Create Traffic Store mutex, Traffic Store, PDComLadderThread.
 *
 *	Note:
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_MUTEX_ERR
 */
TRDP_ERR_T trdp_ladder_init (void)
{
	/* Traffic Store */
	extern CHAR8 TRAFFIC_STORE[];					/* Traffic Store shared memory name */
	extern VOS_SHRD_T  pTrafficStoreHandle;				/* Pointer to Traffic Store Handle */
	extern UINT8 *pTrafficStoreAddr;				/* pointer to pointer to Traffic Store Address */
	UINT32 trafficStoreSize = TRAFFIC_STORE_SIZE;	/* Traffic Store Size : 64KB */

	/* PDComLadderThread */
	extern CHAR8 pdComLadderThreadName[];			/* Thread name is PDComLadder Thread. */
	extern BOOL pdComLadderThreadActiveFlag;		/* PDComLaader Thread active/non-active Flag :active=TRUE, nonActive=FALSE */
	VOS_THREAD_T pdComLadderThread = NULL;			/* Thread handle */

	/* Traffic Store Mutex */
	extern VOS_MUTEX_T pTrafficStoreMutex;			/* Pointer to Mutex for Traffic Store */

	/* Traffic Store Create */
	/* Traffic Store Mutex Create */
	TRDP_ERR_T ret = TRDP_MUTEX_ERR;

	/*	PDComLadder Thread Active ? */
	if (pdComLadderThreadActiveFlag == TRUE)
	{
		return TRDP_NO_ERR;
	}

	if (vos_mutexCreate(&pTrafficStoreMutex) != VOS_NO_ERR)
	{
		if (pdComLadderThreadActiveFlag == FALSE)
		{
			vos_threadInit();
			if (vos_threadCreate(&pdComLadderThread,
									pdComLadderThreadName,
									VOS_THREAD_POLICY_OTHER,
									0,
									0,
									0,
									(void *)PDComLadder,
									NULL) == VOS_NO_ERR)
			{
				pdComLadderThreadActiveFlag = TRUE;
				return TRDP_NO_ERR;
			}
			else
			{
				vos_printf(VOS_LOG_ERROR, "TRDP PDComLadderThread Create failed\n");
				return ret;
			}
		}
	}

	/* Lock Traffic Store Mutex */
	if ((vos_mutexTryLock(pTrafficStoreMutex)) != VOS_NO_ERR)
	{
		vos_printf(VOS_LOG_ERROR, "TRDP Traffic Store Mutex Lock failed\n");
		return ret;
	}

	/* Create the Traffic Store */
    if ((vos_sharedOpen(TRAFFIC_STORE, &pTrafficStoreHandle, &pTrafficStoreAddr, &trafficStoreSize)) != VOS_NO_ERR)
    {
		vos_printf(VOS_LOG_ERROR, "TRDP Traffic Store Creat failed\n");
		ret = TRDP_MEM_ERR;
		return ret;
    }
    else
    {
    	pTrafficStoreHandle->sharedMemoryName = TRAFFIC_STORE;
    }

	/* Traffic Store Mutex unlock */
	vos_mutexUnlock(pTrafficStoreMutex);
/*	if ((vos_mutexUnlock(pTrafficStoreMutex)) != VOS_NO_ERR)
	{
		vos_printf(VOS_LOG_ERROR, "TRDP Traffic Store Mutex Unlock failed\n");
		return ret;
	}
*/

	/*	PDComLadder Thread Create */
	if (pdComLadderThreadActiveFlag == FALSE)
	{
		vos_threadInit();
		if (vos_threadCreate(&pdComLadderThread,
								pdComLadderThreadName,
								VOS_THREAD_POLICY_OTHER,
								0,
								0,
								0,
								(void *)PDComLadder,
								NULL) == TRDP_NO_ERR)
		{
			pdComLadderThreadActiveFlag = TRUE;
			ret = TRDP_NO_ERR;
		}
		else
		{
			vos_printf(VOS_LOG_ERROR, "TRDP PDComLadderThread Create failed\n");
			return ret;
		}
	}
	else
	{
		ret = TRDP_NO_ERR;
	}

	return ret;	/* TRDP_NO_ERR */
}

/******************************************************************************/
/** Finalize TRDP Ladder Support
 *  Delete Traffic Store mutex, Traffic Store.
 *
 *	Note:
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T trdp_ladder_terminate (void)
{
	extern UINT8 *pTrafficStoreAddr;					/* pointer to pointer to Traffic Store Address */
	extern VOS_MUTEX_T pTrafficStoreMutex;				/* Pointer to Mutex for Traffic Store */

	/* Delete Traffic Store */
	tlp_lockTrafficStore();
	if (vos_sharedClose(pTrafficStoreHandle, pTrafficStoreAddr) != VOS_NO_ERR)
	{
		vos_printf(VOS_LOG_ERROR, "Release Traffic Store shared memory failed\n");
		return TRDP_MEM_ERR;
	}
	tlp_unlockTrafficStore();

	/* Delete Traffic Store Mutex */
	vos_mutexDelete(pTrafficStoreMutex);

	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Set pdComLadderThreadStartFlag.
 *
 *  @param[in]      startFlag         PdComLadderThread Start Flag
 *
 *  @retval         TRDP_NO_ERR			no error
 */
TRDP_ERR_T  trdp_setPdComLadderThreadStartFlag (
    BOOL startFlag)
{
	extern BOOL pdComLadderThreadStartFlag; /* PDComLadder Thread instruction start up Flag
													:start=TRUE, stop=FALSE */

	pdComLadderThreadStartFlag = startFlag;
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Set SubNetwork Context.
 *
 *  @param[in]      SubnetId           Sub-network Id: SUBNET1 or SUBNET2
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter error
 *  @retval         TRDP_NOPUB_ERR		not published
 *  @retval         TRDP_NOINIT_ERR	handle invalid
 */
TRDP_ERR_T  tlp_setNetworkContext (
    UINT32 subnetId)
{
    /* Check Sub-network Id */
    if ((subnetId == SUBNET1) || (subnetId == SUBNET2))
    {
        /* Set usingSubnetId */
        usingSubnetId = subnetId;
        return TRDP_NO_ERR;
    }
    else
    {
        return TRDP_PARAM_ERR;
    }
}

/**********************************************************************************************************************/
/** Get SubNetwork Context.
 *
 *  @param[in,out]  pSubnetId			pointer to Sub-network Id
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter error
 *  @retval         TRDP_NOPUB_ERR		not published
 *  @retval         TRDP_NOINIT_ERR	handle invalid
 */
TRDP_ERR_T  tlp_getNetworkContext (
    UINT32 *pSubnetId)
{
    if (pSubnetId == NULL)
    {
        return TRDP_PARAM_ERR;
    }
    else
    {
        /* Get usingSubnetId */
        *pSubnetId = usingSubnetId;
        return TRDP_NO_ERR;
    }
}

/**********************************************************************************************************************/
/** Get Traffic Store accessibility.
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_MUTEX_ERR		mutex error
 */
TRDP_ERR_T  tlp_lockTrafficStore (
    void)
{
	extern VOS_MUTEX_T pTrafficStoreMutex;					/* pointer to Mutex for Traffic Store */

	/* Lock Traffic Store by Mutex */
	if ((vos_mutexTryLock(pTrafficStoreMutex)) != VOS_NO_ERR)
	{
		vos_printf(VOS_LOG_ERROR, "TRDP Traffic Store Mutex Lock failed\n");
		return TRDP_MUTEX_ERR;
	}
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Release Traffic Store accessibility.
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_MUTEX_ERR		mutex error
 *
  */
TRDP_ERR_T  tlp_unlockTrafficStore (
    void)
{
	extern VOS_MUTEX_T pTrafficStoreMutex;							/* pointer to Mutex for Traffic Store */

	/* Lock Traffic Store by Mutex */
	vos_mutexUnlock(pTrafficStoreMutex);
/*	if (vos_mutexUnlock(pTrafficStoreMutex) != VOS_NO_ERR)
	{
		vos_printf(VOS_LOG_ERROR, "TRDP Traffic Store Mutex Unlock failed\n");
		return TRDP_MUTEX_ERR;
	}
*/
		return TRDP_NO_ERR;
}

#endif /* TRDP_OPTION_LADDER */
