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
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
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
 *  @param[in]      topoCount       topocount to use for PD frame
 *  @param[in]      subs            subsAndReserve
 *  @param[in]      offsetAddress   ladder offset
 *  @param[in]      replyComId      Pull request comId
 *  @param[in]      replyIpAddress  Pull request Ip
 */
void    trdp_pdInit (
    PD_ELE_T    *pPacket,
    TRDP_MSG_T  type,
    UINT32      topoCount,
    UINT16      subs,
    UINT16      offsetAddress,
    UINT32      replyComId,
    UINT32      replyIpAddress)
{
    if (pPacket == NULL)
    {
        return;
    }

    pPacket->pFrame->frameHead.protocolVersion  = vos_htons(TRDP_PROTO_VER);
    pPacket->pFrame->frameHead.topoCount        = vos_htonl(topoCount);
    pPacket->pFrame->frameHead.comId            = vos_htonl(pPacket->addr.comId);
    pPacket->pFrame->frameHead.msgType          = vos_htons((UINT16)type);
    pPacket->pFrame->frameHead.datasetLength    = vos_htonl(pPacket->dataSize);
    pPacket->pFrame->frameHead.subsAndReserved  = vos_htons(subs);
    pPacket->pFrame->frameHead.offsetAddress    = vos_htons(offsetAddress);
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

    if (pPacket == NULL || pData == NULL || dataSize == 0 || dataSize != pPacket->dataSize)
    {
        return TRDP_PARAM_ERR;
    }

    if (  !(pPacket->pktFlags & TRDP_FLAGS_MARSHALL)
          || (marshall == NULL))
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
        pPacket->privFlags &= ~TRDP_INVALID_DATA;

        /* Update PD buffer */
        trdp_pdDataUpdate(pPacket);

        /*  Update some statistics  */
        pPacket->updPkts++;
    }
    return ret;
}

/******************************************************************************/
/** Add padding and update data CRC
 *
 */
void trdp_pdDataUpdate (
    PD_ELE_T *pPacket)
{
    UINT32  myCRC   = vos_crc32(0xFFFFFFFF, NULL, 0);
    UINT8   *pDest  = pPacket->pFrame->data + pPacket->dataSize;

    /*  Pad with zero bytes */
    UINT32  padding = 4 - (pPacket->dataSize & 0x3);
    if (padding < 4)
    {
        while (padding--)
        {
            *pDest++ = 0;
        }
    }

    /*  Compute data checksum   */
    myCRC = vos_crc32(myCRC, pPacket->pFrame->data, pPacket->dataSize);
    *(UINT32 *)pDest = MAKE_LE(myCRC);
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
    if (pPacket == NULL || pData == NULL )
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

    if ( !(pPacket->pktFlags & TRDP_FLAGS_MARSHALL)
         || (unmarshall == NULL))
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

    vos_clearTime(&appHandle->interval);

    /*    Find the packet which has to be sent next:    */
    for (iterPD = appHandle->pSndQueue; iterPD != NULL; iterPD = iterPD->pNext)
    {
        /*    Get the current time    */
        vos_getTime(&now);

        if( err != TRDP_NO_ERR)
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
            /* send only if there are valid data */
            if (!(iterPD->privFlags & TRDP_INVALID_DATA))
            {
                /*  Update the sequence counter and re-compute CRC    */
                trdp_pdUpdate(iterPD);

                /*    Send the packet if it is not redundant    */
                if (iterPD->socketIdx != -1 &&
                    (!appHandle->beQuiet ||
                     (iterPD->privFlags & TRDP_REDUNDANT)))
                {
                    /* We pass the error to the application, but we keep on going    */
                    err = trdp_pdSend(appHandle->iface[iterPD->socketIdx].sock, iterPD, appHandle->pdDefault.port);
                    if (err == TRDP_NO_ERR)
                    {
                        appHandle->stats.pd.numSend++;
                    }
                }
            }

            /* Reset "immediate" flag for request or requested packet */
            if (iterPD->privFlags & TRDP_REQ_2B_SENT)
            {
                iterPD->privFlags &= ~TRDP_REQ_2B_SENT;
            }

            /*  Set timer if interval was set.
             In case of a requested cyclically PD packet, this will lead to one time jump (jitter) in the interval
             */
            if (timerisset(&iterPD->interval))
            {
                /*    set new time    */
                iterPD->timeToGo = iterPD->interval;
                vos_addTime(&iterPD->timeToGo, &now);
            }
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
    static PD_PACKET_T  *pNewFrame = NULL;      /*  This points to our buffer in hand   */
    PD_PACKET_T         *pTemp;
    PD_ELE_T            *pExistingElement = NULL;
    PD_ELE_T            *pPulledElement;
    TRDP_ERR_T          err         = TRDP_NO_ERR;
    TRDP_ERR_T          resultCode  = TRDP_NO_ERR;
    UINT32 recSize;
    BOOL informUser = FALSE;
    TRDP_ADDRESSES_T    subHandle = { 0, 0, 0, 0};


    /*  Get a buffer    */
    if (pNewFrame == NULL)
    {
        pNewFrame = (PD_PACKET_T *) vos_memAlloc(TRDP_MAX_PD_PACKET_SIZE);
        if (pNewFrame == NULL)
        {
            vos_printf(VOS_LOG_ERROR, "Receiving PD: Out of receive buffers!\n");
            return TRDP_MEM_ERR;
        }
    }
    /* clean memory before using it */
    memset(pNewFrame, 0, TRDP_MAX_PD_PACKET_SIZE);

    recSize = TRDP_MAX_PD_PACKET_SIZE;

    /*  Get the packet from the wire:  */
    err = (TRDP_ERR_T) vos_sockReceiveUDP(sock, (UINT8 *) &pNewFrame->frameHead, &recSize, &subHandle.srcIpAddr);
    if ( err != TRDP_NO_ERR)
    {
        return err;
    }

    /*  Is packet sane?    */
    err = trdp_pdCheck(&pNewFrame->frameHead, recSize);

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

    /*  Check topocount  */
    if (vos_ntohl(pNewFrame->frameHead.topoCount) &&
        vos_ntohl(pNewFrame->frameHead.topoCount) != appHandle->topoCount)
    {
        appHandle->stats.pd.numTopoErr++;
        vos_printf(VOS_LOG_INFO, "PD data with wrong topocount ignored (comId %u, topo %u)\n",
                   vos_ntohl(pNewFrame->frameHead.comId),
                   vos_ntohl(pNewFrame->frameHead.topoCount));
        return TRDP_TOPO_ERR;
    }

    /*  Compute the subscription handle */
    subHandle.comId = vos_ntohl(pNewFrame->frameHead.comId);

#if 0
    /*  Check if sequence counter is OK
        Is this packet a redundant one?  */
    if (trdp_isRcvSeqCnt(vos_ntohl(pNewFrame->frameHead.sequenceCounter), subHandle.comId,
                         vos_ntohs(pNewFrame->frameHead.msgType), subHandle.srcIpAddr))
    {
        vos_printf(VOS_LOG_INFO, "Redundant PD data ignored (comId %u)\n", vos_ntohl(pNewFrame->frameHead.comId));
        return TRDP_NO_ERR;
    }
#endif

    /*  It might be a PULL request      */
    if (vos_ntohs(pNewFrame->frameHead.msgType) == (UINT16) TRDP_MSG_PR)
    {
        /*  Handle statistics request  */
        if (vos_ntohl(pNewFrame->frameHead.comId) == TRDP_STATISTICS_REQUEST_COMID)
        {
            pPulledElement = trdp_queueFindComId(appHandle->pSndQueue, TRDP_GLOBAL_STATISTICS_COMID);
            if ( pPulledElement != NULL)
            {
                pPulledElement->addr.comId      = TRDP_GLOBAL_STATISTICS_COMID;
                pPulledElement->addr.destIpAddr = vos_ntohl(pNewFrame->frameHead.replyIpAddress);

                trdp_pdInit(pPulledElement, TRDP_MSG_PD, appHandle->topoCount, 0, 0, 0, 0);

                trdp_pdPrepareStats(appHandle, pPulledElement);
            }
            else
            {
                vos_printf(VOS_LOG_ERROR, "Statistics request failed, not published!\n");
            }
        }
        else
        {
            /*  Find requested publish element  */
            pPulledElement = trdp_queueFindComId(appHandle->pSndQueue, vos_ntohl(pNewFrame->frameHead.replyComId));
        }

        if (pPulledElement != NULL)
        {
            /*  Set the destination address of the requested telegram either to the replyIp or the source Ip of the
                requester   */

            if (pNewFrame->frameHead.replyIpAddress != 0)
            {
                pPulledElement->pullIpAddress = vos_ntohl(pNewFrame->frameHead.replyIpAddress);
            }
            else
            {
                pPulledElement->pullIpAddress = subHandle.srcIpAddr;
            }

            /* trigger immediate sending of PD  */
            pPulledElement->privFlags |= TRDP_REQ_2B_SENT;

            if (trdp_pdSendQueued(appHandle) != TRDP_NO_ERR)
            {
                /*  We do not break here, only report error */
                vos_printf(VOS_LOG_ERROR, "Error sending one or more PD packets (Err: %d)\n", err);
            }

            informUser = TRUE;
        }
    }

    /*  Examine subscription queue, are we interested in this PD?   */
    pExistingElement = trdp_queueFindSubAddr(appHandle->pRcvQueue, &subHandle);

    if (pExistingElement == NULL)
    {
        /*
        vos_printf(VOS_LOG_INFO, "No subscription (SrcIp: %s comId %u)\n", vos_ipDotted(subHandle.srcIpAddr) ,vos_ntohl(pNewFrame->frameHead.comId));
        */
    }
    else
    {
        /*        Is this packet a duplicate?    */
        if (vos_ntohl(pNewFrame->frameHead.sequenceCounter) <
            vos_ntohl(pExistingElement->pFrame->frameHead.sequenceCounter))
        {
            /* Check for overrun situation  */
            if (vos_ntohl(pNewFrame->frameHead.sequenceCounter) -
                vos_ntohl(pExistingElement->pFrame->frameHead.sequenceCounter) <
                vos_ntohl(pNewFrame->frameHead.sequenceCounter))
            {
                vos_printf(VOS_LOG_INFO, "Old PD data ignored (SrcIp: %s comId %u)\n", vos_ipDotted(
                               subHandle.srcIpAddr), vos_ntohl(pNewFrame->frameHead.comId));
                return TRDP_NO_ERR;
            }
        }

        /*
        vos_printf(VOS_LOG_INFO, "Received (SrcIp: %s comId %u)\n", vos_ipDotted(subHandle.srcIpAddr) ,vos_ntohl(pNewFrame->frameHead.comId));
        */

        /*  This might have not been set!   */
        pExistingElement->dataSize  = vos_ntohl(pNewFrame->frameHead.datasetLength);
        pExistingElement->grossSize = trdp_packetSizePD(pExistingElement->dataSize);

        /*  Remember this sequence count value    */
        pExistingElement->curSeqCnt = vos_ntohl(pNewFrame->frameHead.sequenceCounter);

        /*  Has the data changed?   */
        informUser = memcmp(pNewFrame->data, pExistingElement->pFrame->data, pExistingElement->dataSize);

        /*  Get the current time and compute the next time this packet should be received.  */
        vos_getTime(&pExistingElement->timeToGo);
        vos_addTime(&pExistingElement->timeToGo, &pExistingElement->interval);

        /*  Update some statistics  */
        pExistingElement->numRxTx++;
        pExistingElement->lastErr   = TRDP_NO_ERR;
        pExistingElement->privFlags &= ~TRDP_TIMED_OUT;

        /* set the data valid */
        pExistingElement->privFlags &= ~TRDP_INVALID_DATA;

        if (informUser)
        {
            /*  remove the old one, insert the new one  */
            /*  swap the packet pointers                */
            pTemp = pExistingElement->pFrame;
            pExistingElement->pFrame = pNewFrame;
            pNewFrame = pTemp;
        }
    }
#if 0
    else    /* there is no subscription, keep all values sane   */
    {
        pNewElement->addr.comId         = vos_ntohl(pNewElement->pFrame->frameHead.comId);
        pNewElement->addr.srcIpAddr     = subHandle.srcIpAddr;
        pNewElement->addr.destIpAddr    = subHandle.destIpAddr;
        pNewElement->privFlags          = TRDP_PRIV_NONE;
        pNewElement->pktFlags           = TRDP_FLAGS_NONE;
        pNewElement->interval.tv_sec    = 0;
        pNewElement->interval.tv_usec   = 0;
        pNewElement->timeToGo.tv_sec    = 0;
        pNewElement->timeToGo.tv_usec   = 0;
        pNewElement->dataSize           = vos_ntohl(pNewElement->pFrame->frameHead.datasetLength);
        pNewElement->grossSize          = trdp_packetSizePD(pNewElement->dataSize);
        pNewElement->socketIdx          = 0;
        pNewElement->userRef = 0;
        resultCode = TRDP_NOSUB_ERR;
    }
#endif
    if (pExistingElement != NULL &&
        informUser)
    {
        /*  If a callback was provided, call it now */
        if ((pExistingElement->pktFlags & TRDP_FLAGS_CALLBACK)
            && (appHandle->pdDefault.pfCbFunction != NULL))
        {
            TRDP_PD_INFO_T theMessage;
            theMessage.comId        = pExistingElement->addr.comId;
            theMessage.srcIpAddr    = pExistingElement->addr.srcIpAddr;
            theMessage.destIpAddr   = pExistingElement->addr.destIpAddr;
            theMessage.topoCount    = vos_ntohl(pExistingElement->pFrame->frameHead.topoCount);
            theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(pExistingElement->pFrame->frameHead.msgType);
            theMessage.seqCount     = vos_ntohl(pExistingElement->pFrame->frameHead.sequenceCounter);
            theMessage.protVersion  = vos_ntohs(pExistingElement->pFrame->frameHead.protocolVersion);
            theMessage.subs         = vos_ntohs(pExistingElement->pFrame->frameHead.subsAndReserved);
            theMessage.offsetAddr   = vos_ntohs(pExistingElement->pFrame->frameHead.offsetAddress);
            theMessage.replyComId   = vos_ntohl(pExistingElement->pFrame->frameHead.replyComId);
            theMessage.replyIpAddr  = vos_ntohl(pExistingElement->pFrame->frameHead.replyIpAddress);
            theMessage.pUserRef     = pExistingElement->userRef; /* User reference given with the local subscribe? */
            theMessage.resultCode   = resultCode;

            appHandle->pdDefault.pfCbFunction(appHandle->pdDefault.pRefCon,
                                              &theMessage,
                                              pExistingElement->pFrame->data,
                                              vos_ntohl(pExistingElement->pFrame->frameHead.datasetLength));
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Update the header values
 *
 *  @param[in]      pPacket         pointer to the packet to update
 */
void    trdp_pdUpdate (
    PD_ELE_T *pPacket)
{
    UINT32 myCRC = vos_crc32(0xFFFFFFFF, NULL, 0);

    /* increment counter with each telegram */
    pPacket->curSeqCnt++;
    pPacket->pFrame->frameHead.sequenceCounter = vos_htonl(pPacket->curSeqCnt);

    /* Compute CRC32   */
    myCRC = vos_crc32(myCRC, (UINT8 *)&pPacket->pFrame->frameHead, sizeof(PD_HEADER_T) - sizeof(UINT32));
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
    UINT32      myCRC       = vos_crc32(0xFFFFFFFF, NULL, 0);
    UINT32      *pDataFCS   = (UINT32 *)((UINT8 *)pPacket + packetSize - 4);
    TRDP_ERR_T  err         = TRDP_NO_ERR;

    /*  Check size    */
    if (packetSize < TRDP_MIN_PD_HEADER_SIZE ||
        packetSize > TRDP_MAX_PD_PACKET_SIZE)
    {
        vos_printf(VOS_LOG_INFO, "PDframe size error (%u))\n", (UINT32) packetSize);
        err = TRDP_WIRE_ERR;
    }

    /*    Check Header CRC (FCS)  */
    myCRC = vos_crc32(myCRC, (UINT8 *) pPacket, sizeof(PD_HEADER_T) - 4);

    if (pPacket->frameCheckSum != MAKE_LE(myCRC))
    {
        vos_printf(VOS_LOG_INFO, "PDframe crc error (%08x != %08x))\n", pPacket->frameCheckSum, MAKE_LE(myCRC));
        err = TRDP_CRC_ERR;
    }
    /*  Check protocol version  */
    else if ((vos_ntohs(pPacket->protocolVersion) & 0xFF000000) != (TRDP_PROTO_VER & 0xFF000000))
    {
        vos_printf(VOS_LOG_INFO, "PDframe protocol error (%04x != %04x))\n",
                   vos_ntohs(pPacket->protocolVersion),
                   TRDP_PROTO_VER);
        err = TRDP_WIRE_ERR;
    }
    /*  Check type  */
    else if (vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PD &&
             vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PR &&
             vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PE)
    {
        vos_printf(VOS_LOG_INFO, "PDframe type error, received %04x\n", vos_ntohs(pPacket->msgType));
        err = TRDP_WIRE_ERR;
    }
    /*  Check Data CRC (FCS)   */
    else if (pPacket->datasetLength > 0)
    {
        myCRC   = 0xFFFFFFFF; /* reset the initialization value for the CRC */
        myCRC   = vos_crc32(myCRC, (UINT8 *)pPacket + sizeof(PD_HEADER_T), vos_ntohl(pPacket->datasetLength));

        if (*pDataFCS != MAKE_LE(myCRC))
        {
            vos_printf(VOS_LOG_INFO, "PDframe data crc error (%08x != %08x))\n", *pDataFCS, MAKE_LE(myCRC));
            err = TRDP_CRC_ERR;
        }
    }
    else
    {
        err = TRDP_NO_ERR;
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

#if 0
    pPacket->frameHead.frameCheckSum = 0x12345678;
#endif
    err = vos_sockSendUDP(pdSock,
                          (UINT8 *)&pPacket->pFrame->frameHead,
                          pPacket->grossSize,
                          destIp,
                          port);

    if (err != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "trdp_pdSend failed\n");
        return TRDP_IO_ERR;
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Distribute send time of PD packets over time
 *
 *  The duration of PD packets on a 100MBit/s network ranges from 3us to 150us max.
 *  Because a cyclic thread scheduling below 5ms would put a too heavy load on the system, and
 *  PD packets cannot get larger than 1436 (+ UDP header), we will not account for differences in packet size.
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
        vos_printf(VOS_LOG_ERROR, "trdp_pdDistribute: no minimal interval in %d packets found!\n", noOfPackets);
        return TRDP_PARAM_ERR;
    }

    /*  This is the delta time we can jitter...   */
    vos_divTime(&deltaTmax, noOfPackets);

    vos_printf(VOS_LOG_INFO, "trdp_pdDistribute: deltaTmax   = %u.%06u\n", deltaTmax.tv_sec, deltaTmax.tv_usec);
    vos_printf(VOS_LOG_INFO, "trdp_pdDistribute: tNull       = %u.%06u\n", tNull.tv_sec, tNull.tv_usec);
    vos_printf(VOS_LOG_INFO, "trdp_pdDistribute: noOfPackets = %d\n", noOfPackets);

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
                vos_printf(VOS_LOG_INFO, "trdp_pdDistribute: packet [%d] with interval %u.%06u could timeout...\n",
                           packetIndex, temp.tv_sec, temp.tv_usec);
                vos_printf(VOS_LOG_INFO, "...no change in send time!\n");
            }
            else
            {
                pPacket->timeToGo = nextTime2Go;
                vos_printf(VOS_LOG_INFO, "trdp_pdDistribute: nextTime2Go[%d] = %u.%06u\n",
                           packetIndex, nextTime2Go.tv_sec, nextTime2Go.tv_usec);

            }
            packetIndex++;
        }
        pPacket = pPacket->pNext;
    }

    return TRDP_NO_ERR;
}
