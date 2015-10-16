/******************************************************************************/
/**
 * @file            trdp_pdcom.c
 *
 * @brief           Functions for PD communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      BL 2015-08-31: Ticket #94: TRDP_REDUNDANT flag is evaluated, beQuiet removed
 *      BL 2015-08-05: Ticket #81: Counts for packet loss
 *     AHW 2015-04-10: Ticket #76: Wrong initialisation of frame pointer in trdp_pdReceive()
 *     AHW 2015-04-10: Ticket #79: handling for dataSize==0/pData== NULL fixed in in trdp_pdPut()
 *      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
 *                     Ticket #47: Protocol change: no FCS for data part of telegrams
 *                     Ticket #43: Usage of memset() in the trdp_pdReceive() function
 *      BL 2014-06-02: Ticket #41: Sequence counter handling fixed
 *                     Ticket #42: memcmp only if callback enabled
 *      BL 2014-02-28: Ticket #25: CRC32 calculation is not according IEEE802.3
 *      BL 2014-02-27: Ticket #23: tlc_getInterval() always returning 10ms
 *      BL 2014-01-09: Ticket #14: Wrong error return in trdp_pdDistribute()
 *      BL 2013-06-24: ID 125: Time-out handling and ready descriptors fixed
 *      BL 2013-04-09: ID 92: Pull request led to reset of push message type
 *      BL 2013-01-25: ID 20: Redundancy handling fixed
 */

/*******************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_types.h"
#include "trdp_utils.h"
#include "trdp_pdcom.h"
#include "trdp_if.h"
#include "trdp_stats.h"
#include "vos_sock.h"
#include "vos_mem.h"

/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */


/******************************************************************************
 *   Locals
 */


/******************************************************************************/
/** Initialize/construct the packet
 *  Set the header infos
 *
 *  @param[in]      pPacket         pointer to the packet element to init
 *  @param[in]      type            type the packet
 *  @param[in]      etbTopoCnt      topocount to use for PD frame
 *  @param[in]      opTrnTopoCnt    topocount to use for PD frame
 *  @param[in]      replyComId      Pull request comId
 *  @param[in]      replyIpAddress  Pull request Ip
 */
void    trdp_pdInit (
    PD_ELE_T    *pPacket,
    TRDP_MSG_T  type,
    UINT32      etbTopoCnt,
    UINT32      opTrnTopoCnt,
    UINT32      replyComId,
    UINT32      replyIpAddress)
{
    if (pPacket == NULL)
    {
        return;
    }

    pPacket->pFrame->frameHead.protocolVersion  = vos_htons(TRDP_PROTO_VER);
    pPacket->pFrame->frameHead.etbTopoCnt       = vos_htonl(etbTopoCnt);
    pPacket->pFrame->frameHead.opTrnTopoCnt     = vos_htonl(opTrnTopoCnt);
    pPacket->pFrame->frameHead.comId            = vos_htonl(pPacket->addr.comId);
    pPacket->pFrame->frameHead.msgType          = vos_htons((UINT16)type);
    pPacket->pFrame->frameHead.datasetLength    = vos_htonl(pPacket->dataSize);
    pPacket->pFrame->frameHead.reserved         = 0;
    pPacket->pFrame->frameHead.replyComId       = vos_htonl(replyComId);
    pPacket->pFrame->frameHead.replyIpAddress   = vos_htonl(replyIpAddress);
}

/******************************************************************************/
/** Copy data
 *  Set the header infos
 */
TRDP_ERR_T trdp_pdPut (
    PD_ELE_T        *pPacket,
    TRDP_MARSHALL_T marshall,
    void            *refCon,
    const UINT8     *pData,
    UINT32          dataSize)
{
    TRDP_ERR_T ret = TRDP_NO_ERR;

    if (pPacket == NULL || dataSize != pPacket->dataSize)
    {
        return TRDP_PARAM_ERR;
    }

    if (pData != NULL && dataSize != 0)
    {

        if (!(pPacket->pktFlags & TRDP_FLAGS_MARSHALL) || (marshall == NULL))
        {
            memcpy(pPacket->pFrame->data, pData, dataSize);
        }
        else
        {
            ret = marshall(refCon,
                           pPacket->addr.comId,
                           (UINT8 *) pData,
                           pPacket->pFrame->data,
                           &dataSize,
                           &pPacket->pCachedDS);
        }

        if (TRDP_NO_ERR == ret)
        {
            /* set data valid */
            pPacket->privFlags = (TRDP_PRIV_FLAGS_T) (pPacket->privFlags & ~TRDP_INVALID_DATA);

            /*  Update some statistics  */
            pPacket->updPkts++;
        }
    }

    return ret;
}

/******************************************************************************/
/** Copy data
 *  Set the header infos
 */
TRDP_ERR_T trdp_pdGet (
    PD_ELE_T            *pPacket,
    TRDP_UNMARSHALL_T   unmarshall,
    void                *refCon,
    const UINT8         *pData,
    UINT32              *pDataSize)
{
    if (pPacket == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Update some statistics  */
    pPacket->getPkts++;

    if (pPacket->privFlags & TRDP_INVALID_DATA)
    {
        return TRDP_NODATA_ERR;
    }

    if (pPacket->privFlags & TRDP_TIMED_OUT)
    {
        return TRDP_TIMEOUT_ERR;
    }

    if (pData != NULL && pDataSize != NULL)
    {
        if ( !(pPacket->pktFlags & TRDP_FLAGS_MARSHALL) || (unmarshall == NULL))
        {
            if (*pDataSize >= pPacket->dataSize)
            {
                *pDataSize = pPacket->dataSize;
                memcpy((void *)pData, pPacket->pFrame->data, *pDataSize);
                return TRDP_NO_ERR;
            }
            else
            {
                return TRDP_PARAM_ERR;
            }
        }
        else
        {
            return unmarshall(refCon,
                              pPacket->addr.comId,
                              pPacket->pFrame->data,
                              (UINT8 *)pData,
                              pDataSize,
                              &pPacket->pCachedDS);
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Send all due PD messages
 *
 *  @param[in]      appHandle           session pointer
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_IO_ERR         socket I/O error
 */
TRDP_ERR_T  trdp_pdSendQueued (
    TRDP_SESSION_PT appHandle)
{
    PD_ELE_T    *iterPD = NULL;
    TRDP_TIME_T now;
    TRDP_ERR_T  err = TRDP_NO_ERR;

    vos_clearTime(&appHandle->nextJob);

    /*    Find the packet which has to be sent next:    */
    for (iterPD = appHandle->pSndQueue; iterPD != NULL; iterPD = iterPD->pNext)
    {
        /*    Get the current time    */
        vos_getTime(&now);

        if (err != TRDP_NO_ERR)
        {
            continue;
        }

        /*  Is this a cyclic packet and
         due to sent?
         or is it a PD Request or a requested packet (PULL) ?
         */
        if ((timerisset(&iterPD->interval) &&                   /*  Request for immediate sending   */
             !timercmp(&iterPD->timeToGo, &now, >)) ||
            iterPD->privFlags & TRDP_REQ_2B_SENT)
        {
            /* send only if there is valid data */
            if (!(iterPD->privFlags & TRDP_INVALID_DATA))
            {
                if (iterPD->privFlags & TRDP_REQ_2B_SENT &&
                    iterPD->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PD))       /*  PULL packet?  */
                {
                    iterPD->pFrame->frameHead.msgType = vos_htons(TRDP_MSG_PP);
                }
                /*  Update the sequence counter and re-compute CRC    */
                trdp_pdUpdate(iterPD);

                if ( !trdp_validTopoCounters( appHandle->etbTopoCnt,
                                              appHandle->opTrnTopoCnt,
                                              vos_ntohl(iterPD->pFrame->frameHead.etbTopoCnt),
                                              vos_ntohl(iterPD->pFrame->frameHead.opTrnTopoCnt)))
                {
                    err = TRDP_TOPO_ERR;
                    vos_printLog(VOS_LOG_INFO, "Sending PD: TopoCount is out of date!\n");
                }
                /*    In case we're sending on an uninitialized publisher; should never happen. */
                else if (iterPD->socketIdx == TRDP_INVALID_SOCKET_INDEX)
                {
                    vos_printLog(VOS_LOG_ERROR, "Sending PD: Socket invalid!\n");
                    /* Try to send the other packets */
                }
                /*    Send the packet if it is not redundant    */
                else if (!(iterPD->privFlags & TRDP_REDUNDANT))
                {
                    TRDP_ERR_T result;
                    /* We pass the error to the application, but we keep on going    */
                    result = trdp_pdSend(appHandle->iface[iterPD->socketIdx].sock, iterPD, appHandle->pdDefault.port);
                    if (result == TRDP_NO_ERR)
                    {
                        appHandle->stats.pd.numSend++;
                        iterPD->numRxTx++;
                    }
                    else
                    {
                        err = result;   /* pass last error to application  */
                    }
                }
            }

            if (iterPD->privFlags & TRDP_REQ_2B_SENT &&
                iterPD->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PP))       /*  PULL packet?  */
            {
                /* Do not reset timer, but restore msgType */
                iterPD->pFrame->frameHead.msgType = vos_htons(TRDP_MSG_PD);
            }
            else if (timerisset(&iterPD->interval))
            {
                /*  Set timer if interval was set.
                    In case of a requested cyclically PD packet, this will lead to one time jump (jitter) in the interval
                */
                vos_addTime(&iterPD->timeToGo, &iterPD->interval);

                if (vos_cmpTime(&iterPD->timeToGo, &now) <= 0)
                {
                    /* in case of a delay of more than one interval - avoid sending it in the next cycle again */
                    iterPD->timeToGo = now;
                    vos_addTime(&iterPD->timeToGo, &iterPD->interval);
                }
            }

            /* Reset "immediate" flag for request or requested packet */
            iterPD->privFlags = (TRDP_PRIV_FLAGS_T) (iterPD->privFlags & ~TRDP_REQ_2B_SENT);
        }
    }
    return err;
}

/******************************************************************************/
/** Receiving PD messages
 *  Read the receive socket for arriving PDs, copy the packet to a new PD_ELE_T
 *  Check for protocol errors and compare the received data to the data in our receive queue.
 *  If it is a new packet, check if it is a PD Request (PULL).
 *  If it is an update, exchange the existing entry with the new one
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      sock                the socket to read from
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_WIRE_ERR       protocol error (late packet, version mismatch)
 *  @retval         TRDP_QUEUE_ERR      not in queue
 *  @retval         TRDP_CRC_ERR        header checksum
 *  @retval         TRDP_TOPOCOUNT_ERR  invalid topocount
 */
TRDP_ERR_T  trdp_pdReceive (
    TRDP_SESSION_PT appHandle,
    INT32           sock)
{
    PD_HEADER_T         *pNewFrameHead      = &appHandle->pNewFrame->frameHead;
    PD_ELE_T            *pExistingElement   = NULL;
    PD_ELE_T            *pPulledElement;
    TRDP_ERR_T          err         = TRDP_NO_ERR;
    TRDP_ERR_T          resultCode  = TRDP_NO_ERR;
    UINT32              recSize     = TRDP_MAX_PD_PACKET_SIZE;
    int                 informUser  = 0;
    TRDP_ADDRESSES_T    subAddresses = { 0, 0, 0, 0, 0, 0};

    /*  Get the packet from the wire:  */
    err = (TRDP_ERR_T) vos_sockReceiveUDP(sock,
                                          (UINT8 *) pNewFrameHead,
                                          &recSize,
                                          &subAddresses.srcIpAddr,
                                          NULL,
                                          &subAddresses.destIpAddr,
                                          FALSE);
    if ( err != TRDP_NO_ERR)
    {
        return err;
    }

    /*  Is packet sane?    */
    err = trdp_pdCheck(pNewFrameHead, recSize);

    /*  Update statistics   */
    switch (err)
    {
        case TRDP_NO_ERR:
            appHandle->stats.pd.numRcv++;
            break;
        case TRDP_CRC_ERR:
            appHandle->stats.pd.numCrcErr++;
            return err;
        case TRDP_WIRE_ERR:
            appHandle->stats.pd.numProtErr++;
            return err;
        default:
            return err;
    }

    /* First check incoming packet's topoCount against session topoCounts */
    if ( !trdp_validTopoCounters( appHandle->etbTopoCnt,
                                  appHandle->opTrnTopoCnt,
                                  vos_ntohl(pNewFrameHead->etbTopoCnt),
                                  vos_ntohl(pNewFrameHead->opTrnTopoCnt)))
    {
        appHandle->stats.pd.numTopoErr++;
        return TRDP_TOPO_ERR;
    }

    /*  Compute the subscription handle */
    subAddresses.comId = vos_ntohl(pNewFrameHead->comId);
    subAddresses.etbTopoCnt = vos_ntohl(pNewFrameHead->etbTopoCnt);
    subAddresses.opTrnTopoCnt = vos_ntohl(pNewFrameHead->opTrnTopoCnt);

    /*  It might be a PULL request      */
    if (vos_ntohs(pNewFrameHead->msgType) == (UINT16) TRDP_MSG_PR)
    {
        /*  Handle statistics request  */
        if (vos_ntohl(pNewFrameHead->comId) == TRDP_STATISTICS_REQUEST_COMID)
        {
            pPulledElement = trdp_queueFindComId(appHandle->pSndQueue, TRDP_GLOBAL_STATISTICS_COMID);
            if (pPulledElement != NULL)
            {
                pPulledElement->addr.comId      = TRDP_GLOBAL_STATISTICS_COMID;
                pPulledElement->addr.destIpAddr = vos_ntohl(pNewFrameHead->replyIpAddress);

                trdp_pdInit(pPulledElement, TRDP_MSG_PP, appHandle->etbTopoCnt, appHandle->opTrnTopoCnt, 0, 0);

                trdp_pdPrepareStats(appHandle, pPulledElement);
            }
            else
            {
                vos_printLog(VOS_LOG_ERROR, "Statistics request failed, not published!\n");
            }
        }
        else
        {
            /*  Find requested publish element  */
            pPulledElement = trdp_queueFindComId(appHandle->pSndQueue, vos_ntohl(pNewFrameHead->replyComId));
        }

        if (pPulledElement != NULL)
        {
            /*  Set the destination address of the requested telegram either to the replyIp or the source Ip of the
                requester   */

            if (pNewFrameHead->replyIpAddress != 0)
            {
                pPulledElement->pullIpAddress = vos_ntohl(pNewFrameHead->replyIpAddress);
            }
            else
            {
                pPulledElement->pullIpAddress = subAddresses.srcIpAddr;
            }

            /* trigger immediate sending of PD  */
            pPulledElement->privFlags |= TRDP_REQ_2B_SENT;

            if (trdp_pdSendQueued(appHandle) != TRDP_NO_ERR)
            {
                /*  We do not break here, only report error */
                vos_printLog(VOS_LOG_ERROR, "Error sending one or more PD packets (Err: %d)\n", err);
            }

            informUser = TRUE;
        }
    }

    /*  Examine subscription queue, are we interested in this PD?   */
    pExistingElement = trdp_queueFindSubAddr(appHandle->pRcvQueue, &subAddresses);

    if (pExistingElement == NULL)
    {
        /*
        vos_printLog(VOS_LOG_INFO, "No subscription (SrcIp: %s comId %u)\n", vos_ipDotted(subAddresses.srcIpAddr),
                        vos_ntohl(pNewFrame->frameHead.comId));
        */
    }
    else
    {
        UINT32 newSeqCnt = vos_ntohl(pNewFrameHead->sequenceCounter);
        /* Save the source IP address of the received packet */
        pExistingElement->lastSrcIP = subAddresses.srcIpAddr;
        /* Save the real destination of the received packet (own IP or MC group) */
        pExistingElement->addr.destIpAddr = subAddresses.destIpAddr;

        if (newSeqCnt == 0)  /* restarted or new sender */
        {
            trdp_resetSequenceCounter(pExistingElement, subAddresses.srcIpAddr, (TRDP_MSG_T) vos_ntohs(pNewFrameHead->msgType));
        }

        /* find sender in our list */
        switch (trdp_checkSequenceCounter(pExistingElement,
                                          newSeqCnt,
                                          subAddresses.srcIpAddr,
                                          (TRDP_MSG_T) vos_ntohs(pNewFrameHead->msgType)))
        {
            case 0:                     /* Sequence counter is valid (at least 1 higher than previous one) */
                break;
            case -1:                    /* List overflow */
                return TRDP_MEM_ERR;
            case 1:
                vos_printLog(VOS_LOG_INFO, "Old PD data ignored (SrcIp: %s comId %u)\n", vos_ipDotted(
                                 subAddresses.srcIpAddr), vos_ntohl(pNewFrameHead->comId));
                return TRDP_NO_ERR;     /* Ignore packet, too old or duplicate */
        }

        if (newSeqCnt > 0 && newSeqCnt > pExistingElement->curSeqCnt + 1)
        {
            pExistingElement->numMissed += newSeqCnt - pExistingElement->curSeqCnt - 1;
        }
        else if (pExistingElement->curSeqCnt > newSeqCnt)
        {
            pExistingElement->numMissed += UINT32_MAX - pExistingElement->curSeqCnt + newSeqCnt;
        }

        /* Store last received sequence counter here, too (pd_get et. al. may access it).   */
        pExistingElement->curSeqCnt = vos_ntohl(pNewFrameHead->sequenceCounter);

        /*  This might have not been set!   */
        pExistingElement->dataSize  = vos_ntohl(pNewFrameHead->datasetLength);
        pExistingElement->grossSize = trdp_packetSizePD(pExistingElement->dataSize);

        /*  Has the data changed?   */
        if (pExistingElement->pktFlags & TRDP_FLAGS_CALLBACK)
        {
            informUser = memcmp(appHandle->pNewFrame->data, pExistingElement->pFrame->data, pExistingElement->dataSize);
        }

        /*  Get the current time and compute the next time this packet should be received.  */
        vos_getTime(&pExistingElement->timeToGo);
        vos_addTime(&pExistingElement->timeToGo, &pExistingElement->interval);

        /*  Update some statistics  */
        pExistingElement->numRxTx++;
        pExistingElement->lastErr   = TRDP_NO_ERR;
        pExistingElement->privFlags = (TRDP_PRIV_FLAGS_T) (pExistingElement->privFlags & ~TRDP_TIMED_OUT);

        /* mark the data as valid */
        pExistingElement->privFlags = (TRDP_PRIV_FLAGS_T) (pExistingElement->privFlags & ~TRDP_INVALID_DATA);

        /*  remove the old one, insert the new one  */
        /*  -> always swap the frame pointers              */
        {
            PD_PACKET_T *pTemp = pExistingElement->pFrame;
            pExistingElement->pFrame = appHandle->pNewFrame;
            appHandle->pNewFrame = pTemp;
        }
    }

    if (pExistingElement != NULL &&
        informUser)
    {
        /*  If a callback was provided, call it now */
        if ((pExistingElement->pktFlags & TRDP_FLAGS_CALLBACK)
            && (pExistingElement->pfCbFunction != NULL))
        {
            TRDP_PD_INFO_T theMessage;
            theMessage.comId        = pExistingElement->addr.comId;
            theMessage.srcIpAddr    = pExistingElement->lastSrcIP;
            theMessage.destIpAddr   = subAddresses.destIpAddr;
            theMessage.etbTopoCnt   = vos_ntohl(pExistingElement->pFrame->frameHead.etbTopoCnt);
            theMessage.opTrnTopoCnt = vos_ntohl(pExistingElement->pFrame->frameHead.opTrnTopoCnt);
            theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(pExistingElement->pFrame->frameHead.msgType);
            theMessage.seqCount     = pExistingElement->curSeqCnt;
            theMessage.protVersion  = vos_ntohs(pExistingElement->pFrame->frameHead.protocolVersion);
            theMessage.replyComId   = vos_ntohl(pExistingElement->pFrame->frameHead.replyComId);
            theMessage.replyIpAddr  = vos_ntohl(pExistingElement->pFrame->frameHead.replyIpAddress);
            theMessage.pUserRef     = pExistingElement->pUserRef; /* User reference given with the local subscribe? */
            theMessage.resultCode   = resultCode;

            pExistingElement->pfCbFunction(appHandle->pdDefault.pRefCon,
                                           appHandle,
                                           &theMessage,
                                           pExistingElement->pFrame->data,
                                           vos_ntohl(pExistingElement->pFrame->frameHead.datasetLength));
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Check for pending packets, set FD if non blocking
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in,out]  pFileDesc           pointer to set of ready descriptors
 *  @param[in,out]  pNoDesc             pointer to number of ready descriptors
 */
void trdp_pdCheckPending (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc)
{
    PD_ELE_T *iterPD;

    /*    Walk over the registered PDs, find pending packets */

    /*    Find the packet which has to be received next:    */
    for (iterPD = appHandle->pRcvQueue; iterPD != NULL; iterPD = iterPD->pNext)
    {
        if ((!(iterPD->privFlags & TRDP_TIMED_OUT)) &&              /* Exempt already timed-out packet */
            timerisset(&iterPD->interval) &&                        /* not PD PULL?                    */
            (timercmp(&iterPD->timeToGo, &appHandle->nextJob, <) || /* earlier than current time-out?  */
             !timerisset(&appHandle->nextJob)))                     /* or not set at all?              */
        {
            appHandle->nextJob = iterPD->timeToGo;                  /* set new next time value from queue element */
        }

        /*    Check and set the socket file descriptor, if not already done    */
        if (iterPD->socketIdx != -1 &&
            appHandle->iface[iterPD->socketIdx].sock != -1 &&
            !FD_ISSET(appHandle->iface[iterPD->socketIdx].sock, (fd_set *)pFileDesc))     /*lint !e573
                                                                                            signed/unsigned division
                                                                                            in macro */
        {
            FD_SET(appHandle->iface[iterPD->socketIdx].sock, (fd_set *)pFileDesc);   /*lint !e573
                                                                                       signed/unsigned division
                                                                                       in macro */
            if (appHandle->iface[iterPD->socketIdx].sock > *pNoDesc)
            {
                *pNoDesc = appHandle->iface[iterPD->socketIdx].sock;
            }
        }
    }

    /*    Find packet in send queue which evntually has to be sent earlier:    */
    for (iterPD = appHandle->pSndQueue; iterPD != NULL; iterPD = iterPD->pNext)
    {
        if (timerisset(&iterPD->interval) &&                        /* has a time out value?    */
            (timercmp(&iterPD->timeToGo, &appHandle->nextJob, <) ||  /* earlier than current time-out? */
             !timerisset(&appHandle->nextJob)))
        {
            appHandle->nextJob = iterPD->timeToGo;                  /* set new next time value from queue element */
        }
    }
}

/******************************************************************************/
/** Check for time outs
 *
 *  @param[in]      appHandle         application handle
 */
void trdp_pdHandleTimeOuts (
    TRDP_SESSION_PT appHandle)
{
    PD_ELE_T    *iterPD = NULL;
    TRDP_TIME_T now;

    /*    Update the current time    */
    vos_getTime(&now);

    /*    Examine receive queue for late packets    */
    for (iterPD = appHandle->pRcvQueue; iterPD != NULL; iterPD = iterPD->pNext)
    {
        if (timerisset(&iterPD->interval) &&
            timerisset(&iterPD->timeToGo) &&                        /*  Prevent timing out of PULLed data too early */
            !timercmp(&iterPD->timeToGo, &now, >) &&                /*  late?   */
            !(iterPD->privFlags & TRDP_TIMED_OUT) &&                /*  and not already flagged ?   */
            !(iterPD->addr.comId == TRDP_STATISTICS_REQUEST_COMID)) /*  Do not bother user with statistics timeout */
        {
            /*  Update some statistics  */
            appHandle->stats.pd.numTimeout++;
            iterPD->lastErr = TRDP_TIMEOUT_ERR;

            /* Packet is late! We inform the user about this:    */
            if (iterPD->pfCbFunction != NULL)
            {
                TRDP_PD_INFO_T theMessage;
                theMessage.comId        = iterPD->addr.comId;
                theMessage.srcIpAddr    = iterPD->addr.srcIpAddr;
                theMessage.destIpAddr   = iterPD->addr.destIpAddr;
                theMessage.etbTopoCnt   = vos_ntohl(iterPD->pFrame->frameHead.etbTopoCnt);
                theMessage.opTrnTopoCnt = vos_ntohl(iterPD->pFrame->frameHead.opTrnTopoCnt);
                theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(iterPD->pFrame->frameHead.msgType);
                theMessage.seqCount     = vos_ntohl(iterPD->pFrame->frameHead.sequenceCounter);
                theMessage.protVersion  = vos_ntohs(iterPD->pFrame->frameHead.protocolVersion);
                theMessage.replyComId   = vos_ntohl(iterPD->pFrame->frameHead.replyComId);
                theMessage.replyIpAddr  = vos_ntohl(iterPD->pFrame->frameHead.replyIpAddress);
                theMessage.pUserRef     = iterPD->pUserRef;
                theMessage.resultCode   = TRDP_TIMEOUT_ERR;

                iterPD->pfCbFunction(appHandle->pdDefault.pRefCon, appHandle, &theMessage, NULL, 0);
            }

            /*    Prevent repeated time out events    */
            iterPD->privFlags |= TRDP_TIMED_OUT;
        }

        /*    Update the current time    */
        vos_getTime(&now);
    }
}

/**********************************************************************************************************************/
/** Checking receive connection requests and data
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      pRfds               pointer to set of ready descriptors
 *  @param[in,out]  pCount              pointer to number of ready descriptors
 */
TRDP_ERR_T   trdp_pdCheckListenSocks (
    TRDP_SESSION_PT appHandle,
    TRDP_FDS_T      *pRfds,
    INT32           *pCount)
{
    PD_ELE_T    *iterPD = NULL;
    TRDP_ERR_T  err, result = TRDP_NO_ERR;

    /*  Check the input params, in case we are in polling mode, the application
     is responsible to get any process data by calling tlp_get()    */
    if (pRfds == NULL || pCount == NULL)
    {
        /* polling mode */
    }
    else if (pCount != NULL && *pCount > 0)
    {
        /*    Check the sockets for received PD packets    */
        for (iterPD = appHandle->pRcvQueue; iterPD != NULL; iterPD = iterPD->pNext)
        {
            if (iterPD->socketIdx != -1 &&
                FD_ISSET(appHandle->iface[iterPD->socketIdx].sock, (fd_set *) pRfds))  /*lint !e573 signed/unsigned
                                                                                         division in macro */
            {       /*    PD frame received? */
                /*  Compare the received data to the data in our receive queue
                 Call user's callback if data changed    */

                err = trdp_pdReceive(appHandle, appHandle->iface[iterPD->socketIdx].sock);

                if (err != TRDP_NO_ERR &&
                    err != TRDP_TIMEOUT_ERR)
                {
                    result = err;
                    vos_printLog(VOS_LOG_ERROR, "trdp_pdReceive() failed (Err: %d)\n", err);
                }

                (*pCount)--;
                FD_CLR(appHandle->iface[iterPD->socketIdx].sock, (fd_set *)pRfds); /*lint !e502 !e573 signed/unsigned division
                                                                                     in macro */
            }
        }
    }
    return result;
}

/******************************************************************************/
/** Update the header values
 *
 *  @param[in]      pPacket         pointer to the packet to update
 */
void    trdp_pdUpdate (
    PD_ELE_T *pPacket)
{
    UINT32 myCRC;

    /* increment counter with each telegram */
    if (pPacket->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PP))
    {
        pPacket->curSeqCnt4Pull++;
        pPacket->pFrame->frameHead.sequenceCounter = vos_htonl(pPacket->curSeqCnt4Pull);
    }
    else
    {
        pPacket->curSeqCnt++;
        pPacket->pFrame->frameHead.sequenceCounter = vos_htonl(pPacket->curSeqCnt);
    }

    /* Compute CRC32   */
    myCRC = vos_crc32(INITFCS, (UINT8 *)&pPacket->pFrame->frameHead, sizeof(PD_HEADER_T) - SIZE_OF_FCS);
    pPacket->pFrame->frameHead.frameCheckSum = MAKE_LE(myCRC);
}


/******************************************************************************/
/** Check if the PD header values and the CRCs are sane
 *
 *  @param[in]      pPacket         pointer to the packet to check
 *  @param[in]      packetSize      max size to check
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_CRC_ERR
 */
TRDP_ERR_T trdp_pdCheck (
    PD_HEADER_T *pPacket,
    UINT32      packetSize)
{
    UINT32      myCRC;
    TRDP_ERR_T  err = TRDP_NO_ERR;

    /*  Check size    */
    if (packetSize < TRDP_MIN_PD_HEADER_SIZE ||
        packetSize > TRDP_MAX_PD_PACKET_SIZE)
    {
        vos_printLog(VOS_LOG_INFO, "PDframe size error (%u))\n", packetSize);
        err = TRDP_WIRE_ERR;
    }
    else
    {
        /*    Check Header CRC (FCS)  */
        myCRC = vos_crc32(INITFCS, (UINT8 *) pPacket, sizeof(PD_HEADER_T) - SIZE_OF_FCS);

        if (pPacket->frameCheckSum != MAKE_LE(myCRC))
        {
            vos_printLog(VOS_LOG_INFO, "PDframe crc error (%08x != %08x))\n", pPacket->frameCheckSum, MAKE_LE(myCRC));
            err = TRDP_CRC_ERR;
        }
        /*  Check protocol version  */
        else if ((vos_ntohs(pPacket->protocolVersion) & TRDP_PROTOCOL_VERSION_CHECK_MASK)
                            != (TRDP_PROTO_VER & TRDP_PROTOCOL_VERSION_CHECK_MASK) ||
                            vos_ntohl(pPacket->datasetLength) > TRDP_MAX_PD_DATA_SIZE)
        {
            vos_printLog(VOS_LOG_INFO, "PDframe protocol error (%04x != %04x))\n",
                         vos_ntohs(pPacket->protocolVersion),
                         TRDP_PROTO_VER);
            err = TRDP_WIRE_ERR;
        }
        /*  Check type  */
        else if (vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PD &&
                 vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PP &&
                 vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PR &&
                 vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PE)
        {
            vos_printLog(VOS_LOG_INFO, "PDframe type error, received %04x\n", vos_ntohs(pPacket->msgType));
            err = TRDP_WIRE_ERR;
        }
    }
    return err;
}

/******************************************************************************/
/** Send one PD packet
 *
 *  @param[in]      pdSock          socket descriptor
 *  @param[in]      pPacket         pointer to packet to be sent
 *  @param[in]      port            port on which to send
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_IO_ERR
 */
TRDP_ERR_T  trdp_pdSend (
    INT32       pdSock,
    PD_ELE_T    *pPacket,
    UINT16      port)
{
    VOS_ERR_T   err     = VOS_NO_ERR;
    UINT32      destIp  = pPacket->addr.destIpAddr;

    /*  check for temporary address (PD PULL):  */
    if (pPacket->pullIpAddress != 0)
    {
        destIp = pPacket->pullIpAddress;
        pPacket->pullIpAddress = 0;
    }

    pPacket->sendSize = pPacket->grossSize;

    err = vos_sockSendUDP(pdSock,
                          (UINT8 *)&pPacket->pFrame->frameHead,
                          &pPacket->sendSize,
                          destIp,
                          port);

    if (err != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "trdp_pdSend failed\n");
        return TRDP_IO_ERR;
    }

    if (pPacket->sendSize != pPacket->grossSize)
    {
        vos_printLog(VOS_LOG_ERROR, "trdp_pdSend incomplete\n");
        return TRDP_IO_ERR;
    }

    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Distribute send time of PD packets over time
 *
 *  The duration of PD packets on a 100MBit/s network ranges from 3us to 150us max.
 *  Because a cyclic thread scheduling below 5ms would put a too heavy load on the system, and
 *  PD packets cannot get larger than 1432 (+ UDP header), we will not account for differences in packet size.
 *  Another factor is the differences in intervals for different packets: We should only change the
 *  starting times of the packets within 1/2 the interval time. Otherwise a late addition of packets
 *  could lead to timeouts of already queued packets.
 *  Scheduling will be computed based on the smallest interval time.
 *
 *  @param[in]      pSndQueue       pointer to send queue
 *
 *  @retval         TRDP_NO_ERR
 */
TRDP_ERR_T  trdp_pdDistribute (
    PD_ELE_T *pSndQueue)
{
    PD_ELE_T    *pPacket    = pSndQueue;
    TRDP_TIME_T deltaTmax   = {1000, 0}; /*    Preset to highest value    */
    TRDP_TIME_T tNull       = {0, 0};
    TRDP_TIME_T temp        = {0, 0};
    TRDP_TIME_T nextTime2Go;
    UINT32      noOfPackets = 0;
    UINT32      packetIndex = 0;

    if (pSndQueue == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Do nothing if only one packet is pending */
    if (pSndQueue->pNext == NULL)
    {
        return TRDP_NO_ERR;
    }

    /*
        Find delta tmax - the maximum time for which we will distribute the packets and the number of packets
        to send within that time. Equals the smallest interval of any PD.
        Find the next packet send time, as well.
    */

    while (pPacket)
    {
        /*  Do not count PULL-only packets!  */
        if (pPacket->interval.tv_sec != 0 ||
            pPacket->interval.tv_usec != 0)
        {
            if (vos_cmpTime(&deltaTmax, &pPacket->interval) > 0)
            {
                deltaTmax = pPacket->interval;
            }
            if (vos_cmpTime(&tNull, &pPacket->timeToGo) < 0)
            {
                tNull = pPacket->timeToGo;
            }
            noOfPackets++;
        }
        pPacket = pPacket->pNext;
    }

    /*  Sanity check  */
    if (vos_cmpTime(&deltaTmax, &temp) == 0 ||
        noOfPackets == 0)
    {
        vos_printLog(VOS_LOG_INFO, "trdp_pdDistribute: no minimal interval in %d packets found!\n", noOfPackets);
        return TRDP_NO_ERR;     /* Ticket #14: Nothing to shape is not an error */
    }

    /*  This is the delta time we can jitter...   */
    vos_divTime(&deltaTmax, noOfPackets);

    vos_printLog(VOS_LOG_INFO, "trdp_pdDistribute: deltaTmax   = %u.%06u\n", deltaTmax.tv_sec, deltaTmax.tv_usec);
    vos_printLog(VOS_LOG_INFO, "trdp_pdDistribute: tNull       = %u.%06u\n", tNull.tv_sec, tNull.tv_usec);
    vos_printLog(VOS_LOG_INFO, "trdp_pdDistribute: noOfPackets = %d\n", noOfPackets);

    for (packetIndex = 0, pPacket = pSndQueue; packetIndex < noOfPackets && pPacket != NULL; )
    {
        /*  Ignore PULL-only packets!  */
        if (pPacket->interval.tv_sec != 0 ||
            pPacket->interval.tv_usec != 0)
        {
            nextTime2Go = tNull;
            temp        = deltaTmax;
            vos_mulTime(&temp, packetIndex);

            vos_addTime(&nextTime2Go, &temp);
            vos_mulTime(&temp, 2);

            if (vos_cmpTime(&temp, &pPacket->interval) > 0)
            {
                vos_printLog(VOS_LOG_INFO, "trdp_pdDistribute: packet [%d] with interval %u.%06u could timeout...\n",
                             packetIndex, temp.tv_sec, temp.tv_usec);
                vos_printLog(VOS_LOG_INFO, "...no change in send time!\n");
            }
            else
            {
                pPacket->timeToGo = nextTime2Go;
                vos_printLog(VOS_LOG_INFO, "trdp_pdDistribute: nextTime2Go[%d] = %u.%06u\n",
                             packetIndex, nextTime2Go.tv_sec, nextTime2Go.tv_usec);

            }
            packetIndex++;
        }
        pPacket = pPacket->pNext;
    }

    return TRDP_NO_ERR;
}
