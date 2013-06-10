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
#include "vos_mem.h"
#include "vos_thread.h"

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
    const TRDP_VERSION_T *pVersion;

    if (appHandle == NULL)
    {
        return;
    }

    memset(&appHandle->stats, 0, sizeof(TRDP_STATISTICS_T));

    pVersion = tlc_getVersion();
    appHandle->stats.version = pVersion->ver << 24 | pVersion->rel << 16 | pVersion->upd << 8 | pVersion->evo;

    if (appHandle->stats.hostName[0] == 0)
    {
        vos_strncpy(appHandle->stats.hostName, "unknown", TRDP_MAX_LABEL_LEN);      /**< host name */
    }
    if (appHandle->stats.leaderName[0] == 0)
    {
        vos_strncpy(appHandle->stats.leaderName, "unknown", TRDP_MAX_LABEL_LEN);    /**< leader host name */
    }
}

/**********************************************************************************************************************/
/** Reset statistics.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_resetStatistics (
    TRDP_APP_SESSION_T appHandle)
{
    TIMEDATE32 tempTime;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    tempTime = appHandle->stats.upTime;
    memset(&appHandle->stats, 0, sizeof(TRDP_STATISTICS_T));
    appHandle->stats.upTime = tempTime;

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pStatistics         Pointer to statistics for this application session
 *  @retval         TRDP_NO_ERR            no error
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
 *  @retval         TRDP_NO_ERR            no error
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
    UINT16      lIndex;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pNumSubs == NULL || pStatistics == NULL || *pNumSubs == 0)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Loop over our subscriptions, but do not exceed user supplied buffers!    */
    for (lIndex = 0, iter = appHandle->pRcvQueue; lIndex < *pNumSubs && iter != NULL; lIndex++, iter = iter->pNext)
    {
        pStatistics[lIndex].comId       = iter->addr.comId;      /* Subscribed ComId
                                                                                                     */
        pStatistics[lIndex].joinedAddr  = iter->addr.mcGroup;    /* Joined IP address
                                                                                                    */
        pStatistics[lIndex].filterAddr  = iter->addr.srcIpAddr;  /* Filter IP address
                                                                                                    */
        pStatistics[lIndex].callBack    = (void *) iter->userRef;  /* Reference for call back function if used
                                                                             */
        pStatistics[lIndex].timeout     = iter->interval.tv_usec + iter->interval.tv_sec * 1000000;
        /* Time-out value in us. 0 = No time-out supervision  */
        pStatistics[lIndex].toBehav = iter->toBehavior;          /* Behaviour at time-out
                                                                                                */
        pStatistics[lIndex].numRecv = iter->numRxTx;             /* Number of packets received for this subscription.
                                                                    */
        pStatistics[lIndex].status  = iter->lastErr;             /* Receive status information
                                                                                           */

    }
    if (lIndex >= *pNumSubs && iter != NULL)
    {
        err = TRDP_MEM_ERR;
    }
    *pNumSubs = lIndex;
    return err;
}

/**********************************************************************************************************************/
/** Return PD publish statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumPub             Pointer to the number of publishers
 *  @param[out]     pStatistics         Pointer to a list with the publish statistics information
 *  @retval         TRDP_NO_ERR            no error
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
    UINT16      lIndex;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pNumPub == NULL || pStatistics == NULL || *pNumPub == 0)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Loop over our subscriptions, but do not exceed user supplied buffers!    */
    for (lIndex = 0, iter = appHandle->pRcvQueue; lIndex < *pNumPub && iter != NULL; lIndex++, iter = iter->pNext)
    {
        pStatistics[lIndex].comId       = iter->addr.comId;      /* Published ComId
                                                                                                      */
        pStatistics[lIndex].destAddr    = iter->addr.destIpAddr; /* IP address of destination for this publishing.
                                                                       */

        /* TBD: */
        pStatistics[lIndex].redId       = appHandle->redID;      /* Redundancy group id
                                                                                                  */
        pStatistics[lIndex].redState    = 0;                     /* Redundancy state
                                                                                                     */

        pStatistics[lIndex].cycle = iter->interval.tv_usec + iter->interval.tv_sec * 1000000;
        /* Interval/cycle in us. 0 = No time-out supervision  */
        pStatistics[lIndex].numSend = iter->numRxTx;             /* Number of packets sent for this publisher.
                                                                           */
        pStatistics[lIndex].numPut  = iter->updPkts;             /* Updated packets (via put)
                                                                                            */

    }
    if (lIndex >= *pNumPub && iter != NULL)
    {
        err = TRDP_MEM_ERR;
    }
    *pNumPub = lIndex;
    return err;
}

/**********************************************************************************************************************/
/** Return MD listener statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumList            Pointer to the number of listeners
 *  @param[out]     pStatistics         Pointer to a list with the listener statistics information
 *  @retval         TRDP_NO_ERR            no error
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

    /*    TBD    */
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return redundancy group statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumRed             Pointer to the number of redundancy groups
 *  @param[out]     pStatistics         Pointer to a list with the redundancy group information
 *  @retval         TRDP_NO_ERR            no error
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

    /*    TBD    */
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return join statistics.
 *  Memory for statistics information must be provided by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumJoin            Pointer to the number of joined IP Adresses
 *  @param[out]     pIpAddr             Pointer to a list with the joined IP adresses
 *  @retval         TRDP_NO_ERR            no error
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
    UINT16      lIndex;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pNumJoin == NULL || pIpAddr == NULL || *pNumJoin == 0)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Loop over our subscriptions, but do not exceed user supplied buffers!    */
    for (lIndex = 0, iter = appHandle->pRcvQueue; lIndex < *pNumJoin && iter != NULL; lIndex++, iter = iter->pNext)
    {
        *pIpAddr++ = iter->addr.mcGroup;                        /* Subscribed MC address.                       */
    }

    if (lIndex >= *pNumJoin && iter != NULL)
    {
        err = TRDP_MEM_ERR;
    }

    *pNumJoin = lIndex;

    return err;
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
    UINT16      lIndex;
    VOS_ERR_T   ret;
    VOS_TIME_T  temp;
    TIMEDATE32  diff;

    /*  Get a new time stamp    */
    vos_getTime(&appHandle->stats.timeStamp);

    /*  Compute uptime */
    temp = appHandle->stats.timeStamp;
    vos_subTime(&temp, &appHandle->initTime);

    /*  Compute statistics from old uptime and old statistics values by maintaining the offset */
    diff = appHandle->stats.upTime - appHandle->stats.statisticTime;
    appHandle->stats.upTime         = temp.tv_sec;              /* round down */
    appHandle->stats.statisticTime  = temp.tv_sec - diff;       /* round down */


    /*  Update memory statsp    */
    ret = vos_memCount(&appHandle->stats.mem.total,
                       &appHandle->stats.mem.free,
                       &appHandle->stats.mem.minFree,
                       &appHandle->stats.mem.numAllocBlocks,
                       &appHandle->stats.mem.numAllocErr,
                       &appHandle->stats.mem.numFreeErr,
                       appHandle->stats.mem.blockSize,
                       appHandle->stats.mem.usedBlockSize);
    if (ret != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_memCount() failed (Err: %d)\n", ret);
    }

    /*  Count our subscriptions */
    for (lIndex = 0, iter = appHandle->pRcvQueue; iter != NULL; lIndex++, iter = iter->pNext)
    {
        ;
    }

    appHandle->stats.pd.numSubs = lIndex;

    /*  Count our publishers */
    for (lIndex = 0, iter = appHandle->pSndQueue; iter != NULL; lIndex++, iter = iter->pNext)
    {
        ;
    }

    appHandle->stats.pd.numPub = lIndex;
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
    pData->version = vos_htonl(appHandle->stats.version);
    pData->timeStamp.tv_sec     = vos_htonl(appHandle->stats.timeStamp.tv_sec);
    pData->timeStamp.tv_usec    = vos_htonl(appHandle->stats.timeStamp.tv_usec);
    pData->upTime           = vos_htonl(appHandle->stats.upTime);
    pData->statisticTime    = vos_htonl(appHandle->stats.statisticTime);
    pData->ownIpAddr        = vos_htonl(appHandle->stats.ownIpAddr);
    pData->leaderIpAddr     = vos_htonl(appHandle->stats.leaderIpAddr);
    pData->processPrio      = vos_htonl(appHandle->stats.processPrio);
    pData->processCycle     = vos_htonl(appHandle->stats.processCycle);
    vos_strncpy(pData->hostName, appHandle->stats.hostName, TRDP_MAX_LABEL_LEN);
    vos_strncpy(pData->leaderName, appHandle->stats.leaderName, TRDP_MAX_LABEL_LEN);

    /*  Memory  */
    pData->mem.total            = vos_htonl(appHandle->stats.mem.total);
    pData->mem.free             = vos_htonl(appHandle->stats.mem.free);
    pData->mem.minFree          = vos_htonl(appHandle->stats.mem.minFree);
    pData->mem.numAllocBlocks   = vos_htonl(appHandle->stats.mem.numAllocBlocks);
    pData->mem.numAllocErr      = vos_htonl(appHandle->stats.mem.numAllocErr);
    pData->mem.numFreeErr       = vos_htonl(appHandle->stats.mem.numFreeErr);

    for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
    {
        pData->mem.blockSize[i]     = vos_htonl(appHandle->stats.mem.blockSize[i]);
        pData->mem.usedBlockSize[i] = vos_htonl(appHandle->stats.mem.usedBlockSize[i]);
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
    pData->udpMd.defQos = vos_htonl(appHandle->stats.udpMd.defQos);
    pData->udpMd.defTtl = vos_htonl(appHandle->stats.udpMd.defTtl);
    pData->udpMd.defReplyTimeout    = vos_htonl(appHandle->stats.udpMd.defReplyTimeout);
    pData->udpMd.defConfirmTimeout  = vos_htonl(appHandle->stats.udpMd.defConfirmTimeout);
    pData->udpMd.numList            = vos_htonl(appHandle->stats.udpMd.numList);
    pData->udpMd.numRcv             = vos_htonl(appHandle->stats.udpMd.numRcv);
    pData->udpMd.numCrcErr          = vos_htonl(appHandle->stats.udpMd.numCrcErr);
    pData->udpMd.numProtErr         = vos_htonl(appHandle->stats.udpMd.numProtErr);
    pData->udpMd.numTopoErr         = vos_htonl(appHandle->stats.udpMd.numTopoErr);
    pData->udpMd.numNoListener      = vos_htonl(appHandle->stats.udpMd.numNoListener);
    pData->udpMd.numReplyTimeout    = vos_htonl(appHandle->stats.udpMd.numReplyTimeout);
    pData->udpMd.numConfirmTimeout  = vos_htonl(appHandle->stats.udpMd.numConfirmTimeout);
    pData->udpMd.numSend            = vos_htonl(appHandle->stats.udpMd.numSend);

    pData->tcpMd.defQos = vos_htonl(appHandle->stats.tcpMd.defQos);
    pData->tcpMd.defTtl = vos_htonl(appHandle->stats.tcpMd.defTtl);
    pData->tcpMd.defReplyTimeout    = vos_htonl(appHandle->stats.tcpMd.defReplyTimeout);
    pData->tcpMd.defConfirmTimeout  = vos_htonl(appHandle->stats.tcpMd.defConfirmTimeout);
    pData->tcpMd.numList            = vos_htonl(appHandle->stats.tcpMd.numList);
    pData->tcpMd.numRcv             = vos_htonl(appHandle->stats.tcpMd.numRcv);
    pData->tcpMd.numCrcErr          = vos_htonl(appHandle->stats.tcpMd.numCrcErr);
    pData->tcpMd.numProtErr         = vos_htonl(appHandle->stats.tcpMd.numProtErr);
    pData->tcpMd.numTopoErr         = vos_htonl(appHandle->stats.tcpMd.numTopoErr);
    pData->tcpMd.numNoListener      = vos_htonl(appHandle->stats.tcpMd.numNoListener);
    pData->tcpMd.numReplyTimeout    = vos_htonl(appHandle->stats.tcpMd.numReplyTimeout);
    pData->tcpMd.numConfirmTimeout  = vos_htonl(appHandle->stats.tcpMd.numConfirmTimeout);
    pData->tcpMd.numSend            = vos_htonl(appHandle->stats.tcpMd.numSend);
    pPacket->dataSize = sizeof(TRDP_STATISTICS_T);

    /*  Compute the CRC */
    trdp_pdDataUpdate(pPacket);

}
