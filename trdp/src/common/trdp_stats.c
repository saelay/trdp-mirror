/******************************************************************************/
/**
 * @file            trdp_stats.c
 *
 * @brief           Statistics functions for TRDP communication
 *
 * @details
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
 * $Id$
 *
 */

/*******************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <string.h>

#include "trdp_stats.h"
#include "trdp_if_light.h"
#include "trdp_if.h"
#include "trdp_pdcom.h"

/*******************************************************************************
 * DEFINES
 */

/*******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 *   Locals
 */

void trdp_UpdateStats (TRDP_APP_SESSION_T appHandle);

/******************************************************************************
 *   Globals
 */

/**********************************************************************************************************************/
/** Init statistics.
 *  Clear the stats structure for a session.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 */
void trdp_initStats (
    TRDP_APP_SESSION_T appHandle)
{
    int int0, int1, int2, int3;

    if (appHandle == NULL)
    {
        return;
    }

    memset(&appHandle->stats, 0, sizeof(TRDP_STATISTICS_T));

    sscanf(tlc_getVersion(), "%d.%d.%d.%d", &int0, &int1, &int2, &int3);
    appHandle->stats.version = int0 << 24 | int1 << 16 | int2 << 8 | int3;
}

/**********************************************************************************************************************/
/** Return statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pStatistics         Pointer to statistics for this application session
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_getStatistics (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_STATISTICS_T   *pStatistics)
{
    if (pStatistics == NULL)
    {
        return TRDP_PARAM_ERR;
    }
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    trdp_UpdateStats(appHandle);

    *pStatistics = appHandle->stats;

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return PD subscription statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumSubs            In: The number of subscriptions requested
 *                                      Out: Number of subscriptions returned
 *  @param[in,out]  pStatistics         Pointer to an array with the subscription statistics information
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        there are more subscriptions than requested
 */
EXT_DECL TRDP_ERR_T tlc_getSubsStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumSubs,
    TRDP_SUBS_STATISTICS_T  *pStatistics)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    PD_ELE_T    *iter;
    UINT16      index;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pNumSubs == NULL || pStatistics == NULL || *pNumSubs == 0)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Loop over our subscriptions, but do not exceed user supplied buffers!    */
    for (index = 0, iter = appHandle->pRcvQueue; index < *pNumSubs && iter != NULL; index++, iter = iter->pNext)
    {
        pStatistics[index].comId        = iter->addr.comId;     /* Subscribed ComId                                   */
        pStatistics[index].joinedAddr   = iter->addr.mcGroup;   /* Joined IP address                                  */
        pStatistics[index].filterAddr   = iter->addr.srcIpAddr; /* Filter IP address                                  */
        pStatistics[index].callBack     = (UINT32)iter->userRef; /* Reference for call back function if used
                                                                             */
        pStatistics[index].timeout      = iter->interval.tv_usec + iter->interval.tv_sec * 1000000;
        /* Time-out value in us. 0 = No time-out supervision  */
        pStatistics[index].toBehav  = iter->toBehavior;         /* Behaviour at time-out                              */
        pStatistics[index].numRecv  = iter->numRxTx;            /* Number of packets received for this subscription.  */
        pStatistics[index].status   = iter->lastErr;            /* Receive status information                         */

    }
    if (index >= *pNumSubs && iter != NULL)
    {
        err = TRDP_MEM_ERR;
    }
    *pNumSubs = index;
    return err;
}

/**********************************************************************************************************************/
/** Return PD publish statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumPub             Pointer to the number of publishers
 *  @param[out]     pStatistics         Pointer to a list with the publish statistics information
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR     there are more subscriptions than requested
 */
EXT_DECL TRDP_ERR_T tlc_getPubStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumPub,
    TRDP_PUB_STATISTICS_T   *pStatistics)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    PD_ELE_T    *iter;
    UINT16      index;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pNumPub == NULL || pStatistics == NULL || *pNumPub == 0)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Loop over our subscriptions, but do not exceed user supplied buffers!    */
    for (index = 0, iter = appHandle->pRcvQueue; index < *pNumPub && iter != NULL; index++, iter = iter->pNext)
    {
        pStatistics[index].comId    = iter->addr.comId;         /* Published ComId                                    */
        pStatistics[index].destAddr = iter->addr.destIpAddr;    /* IP address of destination for this publishing.     */

        /* TBD: */
        pStatistics[index].redId    = appHandle->redID;         /* Redundancy group id                                */
        pStatistics[index].redState = 0;                        /* Redundancy state                                   */

        pStatistics[index].cycle = iter->interval.tv_usec + iter->interval.tv_sec * 1000000;
        /* Interval/cycle in us. 0 = No time-out supervision  */
        pStatistics[index].numSend  = iter->numRxTx;            /* Number of packets sent for this publisher.         */
        pStatistics[index].numPut   = iter->updPkts;            /* Updated packets (via put)                          */

    }
    if (index >= *pNumPub && iter != NULL)
    {
        err = TRDP_MEM_ERR;
    }
    *pNumPub = index;
    return err;
}

/**********************************************************************************************************************/
/** Return MD listener statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumList            Pointer to the number of listeners
 *  @param[out]     pStatistics         Pointer to a list with the listener statistics information
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        there are more subscriptions than requested
 */
EXT_DECL TRDP_ERR_T tlc_getListStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumList,
    TRDP_LIST_STATISTICS_T  *pStatistics)
{
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*	TBD	*/
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return redundancy group statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumRed             Pointer to the number of redundancy groups
 *  @param[out]     pStatistics         Pointer to a list with the redundancy group information
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR     there are more subscriptions than requested
 */
EXT_DECL TRDP_ERR_T tlc_getRedStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumRed,
    TRDP_RED_STATISTICS_T   *pStatistics)
{
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*	TBD	*/
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return join statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumJoin            Pointer to the number of joined IP Adresses
 *  @param[out]     pIpAddr             Pointer to a list with the joined IP adresses
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        there are more items than requested
 */
EXT_DECL TRDP_ERR_T tlc_getJoinStatistics (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pNumJoin,
    UINT32              *pIpAddr)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    PD_ELE_T    *iter;
    UINT16      index;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pNumJoin == NULL || pIpAddr == NULL || *pNumJoin == 0)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Loop over our subscriptions, but do not exceed user supplied buffers!    */
    for (index = 0, iter = appHandle->pRcvQueue; index < *pNumJoin && iter != NULL; index++, iter = iter->pNext)
    {
        *pIpAddr++ = iter->addr.mcGroup;                        /* Subscribed MC address.                       */
    }

    if (index >= *pNumJoin && iter != NULL)
    {
        err = TRDP_MEM_ERR;
    }

    *pNumJoin = index;

    return err;
}

/**********************************************************************************************************************/
/** Reset statistics.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_resetStatistics (
    TRDP_APP_SESSION_T appHandle)
{
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    trdp_initStats(appHandle);

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Update the statistics
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 */
void    trdp_UpdateStats (
    TRDP_APP_SESSION_T appHandle)
{
    PD_ELE_T    *iter;
    UINT16      index;

    /*  Get a new time stamp    */
    vos_getTime(&appHandle->stats.timeStamp);

    /*  Update memory statsp    */
    vos_memCount(&appHandle->stats.mem.total,
                 &appHandle->stats.mem.free,
                 &appHandle->stats.mem.minFree,
                 &appHandle->stats.mem.numAllocBlocks,
                 &appHandle->stats.mem.numAllocErr,
                 &appHandle->stats.mem.numFreeErr,
                 appHandle->stats.mem.allocBlockSize,
                 appHandle->stats.mem.allocBlockSize);

    /*  Count our subscriptions */
    for (index = 0, iter = appHandle->pRcvQueue; iter != NULL; index++, iter = iter->pNext) {;}

    appHandle->stats.pd.numSubs = index;

    /*  Count our publishers */
    for (index = 0, iter = appHandle->pSndQueue; iter != NULL; index++, iter = iter->pNext) {;}
    
    appHandle->stats.pd.numPub = index;
}

/**********************************************************************************************************************/
/** Fill the statistics packet
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pPacket             pointer to the packet to fill
 */
void    trdp_pdPrepareStats (
    TRDP_APP_SESSION_T  appHandle,
    PD_ELE_T            *pPacket)
{
    TRDP_STATISTICS_T *pData;
    int i;

    if (pPacket == NULL || appHandle == NULL)
    {
        return;
    }

    trdp_UpdateStats(appHandle);

    /*  The statistics structure is naturally aligned - all 32 Bits, we can cast and just eventually swap the values! */

    pData = (TRDP_STATISTICS_T *) pPacket->pFrame->data;

    /*  Fill in the values  */
    pData->version              = vos_htonl(appHandle->stats.version);
    pData->timeStamp.tv_sec     = vos_htonl(appHandle->stats.timeStamp.tv_sec);
    pData->timeStamp.tv_usec    = vos_htonl(appHandle->stats.timeStamp.tv_usec);
    pData->upTime               = vos_htonl(appHandle->stats.upTime);
    pData->statisticTime        = vos_htonl(appHandle->stats.statisticTime);
    pData->ownIpAddr            = vos_htonl(appHandle->stats.ownIpAddr);
    pData->leaderIpAddr         = vos_htonl(appHandle->stats.leaderIpAddr);
    pData->processPrio          = vos_htonl(appHandle->stats.processPrio);
    pData->processCycle         = vos_htonl(appHandle->stats.processCycle);

    /*  Memory  */
    pData->mem.total            = vos_htonl(appHandle->stats.mem.total);
    pData->mem.free             = vos_htonl(appHandle->stats.mem.free);
    pData->mem.minFree          = vos_htonl(appHandle->stats.mem.minFree);
    pData->mem.numAllocBlocks   = vos_htonl(appHandle->stats.mem.numAllocBlocks);
    pData->mem.numAllocErr      = vos_htonl(appHandle->stats.mem.numAllocErr);
    pData->mem.numFreeErr       = vos_htonl(appHandle->stats.mem.numFreeErr);

    for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
    {
        pData->mem.allocBlockSize[i]    = vos_htonl(appHandle->stats.mem.allocBlockSize[i]);
        pData->mem.usedBlockSize[i]     = vos_htonl(appHandle->stats.mem.usedBlockSize[i]);
    }

    /* Process data */
    pData->pd.defQos        = vos_htonl(appHandle->stats.pd.defQos);
    pData->pd.defTtl        = vos_htonl(appHandle->stats.pd.defTtl);
    pData->pd.defTimeout    = vos_htonl(appHandle->stats.pd.defTimeout);
    pData->pd.numSubs       = vos_htonl(appHandle->stats.pd.numSubs);
    pData->pd.numPub        = vos_htonl(appHandle->stats.pd.numPub);
    pData->pd.numRcv        = vos_htonl(appHandle->stats.pd.numRcv);
    pData->pd.numCrcErr     = vos_htonl(appHandle->stats.pd.numCrcErr);
    pData->pd.numProtErr    = vos_htonl(appHandle->stats.pd.numProtErr);
    pData->pd.numTopoErr    = vos_htonl(appHandle->stats.pd.numTopoErr);
    pData->pd.numNoSubs     = vos_htonl(appHandle->stats.pd.numNoSubs);
    pData->pd.numNoPub      = vos_htonl(appHandle->stats.pd.numNoPub);
    pData->pd.numTimeout    = vos_htonl(appHandle->stats.pd.numTimeout);
    pData->pd.numSend       = vos_htonl(appHandle->stats.pd.numSend);

    /* Message data */
    pData->udpMd.defQos            = vos_htonl(appHandle->stats.udpMd.defQos);
    pData->udpMd.defTtl            = vos_htonl(appHandle->stats.udpMd.defTtl);
    pData->udpMd.defReplyTimeout   = vos_htonl(appHandle->stats.udpMd.defReplyTimeout);
    pData->udpMd.defConfirmTimeout = vos_htonl(appHandle->stats.udpMd.defConfirmTimeout);
    pData->udpMd.numList           = vos_htonl(appHandle->stats.udpMd.numList);
    pData->udpMd.numRcv            = vos_htonl(appHandle->stats.udpMd.numRcv);
    pData->udpMd.numCrcErr         = vos_htonl(appHandle->stats.udpMd.numCrcErr);
    pData->udpMd.numProtErr        = vos_htonl(appHandle->stats.udpMd.numProtErr);
    pData->udpMd.numTopoErr        = vos_htonl(appHandle->stats.udpMd.numTopoErr);
    pData->udpMd.numNoListener     = vos_htonl(appHandle->stats.udpMd.numNoListener);
    pData->udpMd.numReplyTimeout   = vos_htonl(appHandle->stats.udpMd.numReplyTimeout);
    pData->udpMd.numConfirmTimeout = vos_htonl(appHandle->stats.udpMd.numConfirmTimeout);
    pData->udpMd.numSend           = vos_htonl(appHandle->stats.udpMd.numSend);

    pPacket->dataSize = sizeof(TRDP_STATISTICS_T);

    /*  Compute the CRC */
    trdp_pdDataUpdate(pPacket);

}
