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
 */

/*******************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_types.h"
#include "trdp_utils.h"
#include "trdp_pdcom.h"
#include "vos_sock.h"

/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */


/******************************************************************************
 *   Locals
 */

static TRDP_PD_STATS_T sPDComStats;

/******************************************************************************/
/** Initialize/construct the packet
 *  Set the header infos
 *
 *  @param[in]      pPacket			pointer to the packet element to init
 *  @param[in]      type			type the packet
 *  @param[in]      topoCount		topocount to use for PD frame
 *  @param[in]      subs            subsAndReserve
 *  @param[in]      offsetAddress	ladder offset
 *  @param[in]      replyComId		Pull request comId
 *  @param[in]      replyIpAddress	Pull request Ip
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

    pPacket->frameHead.protocolVersion  = vos_htons(IP_PD_PROTO_VER);
    pPacket->frameHead.topoCount        = vos_htonl(topoCount);
    pPacket->frameHead.comId            = vos_htonl(pPacket->addr.comId);
    pPacket->frameHead.msgType          = vos_htons(type);
    pPacket->frameHead.datasetLength    = vos_htonl(pPacket->dataSize);
    pPacket->frameHead.subsAndReserved  = vos_htons(subs);
    pPacket->frameHead.offsetAddress    = vos_htons(offsetAddress);
    pPacket->frameHead.replyComId       = vos_htonl(replyComId);
    pPacket->frameHead.replyIpAddress   = vos_htonl(replyIpAddress);
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
    TRDP_ERR_T  ret   = TRDP_NO_ERR;
    UINT32      myCRC = vos_crc32(0L, NULL, 0);
    UINT8       *pDest = pPacket->data + dataSize;

    if (pPacket == NULL || pData == NULL || dataSize == 0)
    {
        return TRDP_PARAM_ERR;
    }

    if (marshall == NULL)
    {
        memcpy(pPacket->data, pData, dataSize);
    }
    else
    {
        ret = marshall(refCon, pPacket->addr.comId, pData, pPacket->data, &dataSize);
    }

    /*  Pad with zero bytes */
    UINT32 padding = 4 - (dataSize & 0x3);
    if (padding < 4)
    {
        while (padding--)
        {
            *pDest++ = 0;
        }
    }
    
    /*  Compute data checksum   */
    if (TRDP_NO_ERR == ret)
    {
        /* Compute CRC32   */
        myCRC   = vos_crc32(myCRC, (UINT8 *)&pPacket->data, dataSize);
        *(UINT32*)pDest    = MAKE_LE(myCRC);
        
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
    UINT32              dataSize)
{
    if (pPacket == NULL || pData == NULL || dataSize == 0)
    {
        return TRDP_PARAM_ERR;
    }

    if (unmarshall == NULL)
    {
        memcpy(pPacket->data, pData, dataSize);
        return TRDP_NO_ERR;
    }
    return unmarshall(refCon, pPacket->addr.comId, pData, pPacket->data, &dataSize);
}

/******************************************************************************/
/** Send all due PD messages
 *  
 *
 *	@param[in]		appHandle			session pointer
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

        /*  Is this a cyclic packet and
         due to sent?
         or is it a PD Request or a requested packet (PULL) ?
         */
        if ((timerisset(&iterPD->interval) &&
             timercmp(&iterPD->timeToGo, &now, <=)) ||
            iterPD->privFlags & TRDP_REQ_2B_SENT)       /*  Request for immediate sending   */
        {
            /*  Update the sequence counter and re-compute CRC    */
            trdp_pdUpdate(iterPD);
            
            /*    Send the packet if it is not redundant    */
            if (iterPD->socketIdx != -1 &&
                (!appHandle->beQuiet ||
                 (iterPD->pktFlags & TRDP_FLAGS_REDUNDANT)))
            {
                /* We pass the error to the application, but we keep on going    */
                err = trdp_pdSend(appHandle->iface[iterPD->socketIdx].sock, iterPD);
            }
            
            /* Reset "immediate" flag for request or requested packet */
            if (iterPD->privFlags & TRDP_REQ_2B_SENT)
            {
                iterPD->privFlags ^= TRDP_REQ_2B_SENT;
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
 *	If it is a new packet, check if it is a PD Request (PULL).
 *  If it is an update, exchange the existing entry with the new one
 *	Call user's callback if needed
 *
 *	@param[in]		appHandle			session pointer
 *  @param[in]		sock				the socket to read from
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_WIRE_ERR       protocol error (late packet, version mismatch)
 *  @retval         TRDP_QUEUE_ERR      not in queue
 *  @retval         TRDP_CRC_ERR		header checksum
 *  @retval         TRDP_TOPOCOUNT_ERR	invalid topocount
 */
TRDP_ERR_T  trdp_pdReceive (
    TRDP_SESSION_PT appHandle,
    INT32           sock)
{
    static PD_ELE_T *pNewElement        = NULL; /*  This points to our buffer in hand   */
    PD_ELE_T        *pExistingElement   = NULL;
    PD_ELE_T        *pPulledElement;
    TRDP_ERR_T      err = TRDP_NO_ERR;
    TRDP_ERR_T      resultCode = TRDP_NO_ERR;
    INT32           recSize;
    BOOL            informUser = FALSE;
    TRDP_ADDRESSES  subHandle;

    /*  Get a buffer    */
    if (pNewElement == NULL)
    {
        pNewElement = (PD_ELE_T *) vos_memAlloc(sizeof(PD_ELE_T));
        if (pNewElement == NULL)
        {
            vos_printf(VOS_LOG_ERROR, "Receiving PD: Out of receive buffers!\n");
            return TRDP_MEM_ERR;
        }
    }
    else
    {
        /*  Clean up in case of former partial reads   */
        memset(pNewElement, 0, sizeof(PD_ELE_T));
    }

    recSize = MAX_PD_PACKET_SIZE;

    /*	Get the packet from the wire:	*/
    if (vos_sockReceiveUDP(sock, (UINT8 *) &pNewElement->frameHead, &recSize, &subHandle.srcIpAddr) != VOS_NO_ERR)
    {
        return TRDP_WIRE_ERR;
    }

    /*	Is packet sane?	*/
    err = trdp_pdCheck(&pNewElement->frameHead, recSize);
    if (err != TRDP_NO_ERR)
    {
        return err;
    }

    /*	Check topocount	*/
    if (vos_ntohl(pNewElement->frameHead.topoCount) &&
        vos_ntohl(pNewElement->frameHead.topoCount) != appHandle->topoCount)
    {
        vos_printf(VOS_LOG_INFO, "PD data with wrong topocount ignored (comId %u, topo %u)\n",
                   vos_ntohl(pNewElement->frameHead.comId),
                   vos_ntohl(pNewElement->frameHead.topoCount));
        return TRDP_TOPO_ERR;
    }

    /*  Compute the subscription handle */
    subHandle.comId = vos_ntohl(pNewElement->frameHead.comId);

    /*	Check if sequence counter is OK
        Is this packet a redundant one?	*/
    if (trdp_isRcvSeqCnt(vos_ntohl(pNewElement->frameHead.sequenceCounter), subHandle.comId,
                         vos_ntohs(pNewElement->frameHead.msgType), subHandle.srcIpAddr))
    {
        vos_printf(VOS_LOG_INFO, "Redundant PD data ignored (comId %u)\n", vos_ntohl(pNewElement->frameHead.comId));
        return TRDP_NO_ERR;
    }

    /*  It might be a PULL request      */
    if (vos_ntohs(pNewElement->frameHead.msgType) == TRDP_MSG_PR)
    {
        /*  Find requested publish element  */
        pPulledElement = trdp_queueFindComId(appHandle->pSndQueue, vos_ntohl(pNewElement->frameHead.replyComId));

        if (pPulledElement != NULL)
        {
            /*  Set the destination address of the requested telegram either to the replyIp or the source Ip of the
                requester   */

            if (pNewElement->frameHead.replyIpAddress != 0)
            {
                pPulledElement->pullIpAddress = vos_ntohl(pNewElement->frameHead.replyIpAddress);
            }
            else
            {
                pPulledElement->pullIpAddress = subHandle.srcIpAddr;
            }
            
            /* trigger immediate sending of PD  */
            pPulledElement->privFlags  |= TRDP_REQ_2B_SENT;
 
            if (trdp_pdSendQueued(appHandle) != TRDP_NO_ERR)
            {
                /*  We do not break here, only report error */
                vos_printf(VOS_LOG_ERROR, "Error sending one or more PD packets (Err: %d)\n", err);
            }

            informUser = TRUE;
        }
    }

    /*	Examine subscription queue, are we interested in this PD?	*/

    pExistingElement = trdp_queueFindAddr(appHandle->pRcvQueue, &subHandle);

    if (pExistingElement != NULL)
    {
        /*		Is this packet a duplicate?	*/
        if (vos_ntohl(pNewElement->frameHead.sequenceCounter) <
            vos_ntohl(pExistingElement->frameHead.sequenceCounter))
        {
            /* Check for overrun situation  */
            if (vos_ntohl(pNewElement->frameHead.sequenceCounter) -
                vos_ntohl(pExistingElement->frameHead.sequenceCounter) <
                vos_ntohl(pNewElement->frameHead.sequenceCounter))
            {
                vos_printf(VOS_LOG_INFO, "Old PD data ignored (comId %u)\n", vos_ntohl(pNewElement->frameHead.comId));
                return TRDP_NO_ERR;
            }
        }

        /*  Has the data changed?   */
        informUser = memcmp(pNewElement->data, pExistingElement->data, pNewElement->dataSize);

        /*	Get the current time and compute the next time this packet should be received.	*/
        vos_getTime(&pExistingElement->timeToGo);
        vos_addTime(&pExistingElement->timeToGo, &pExistingElement->interval);

        if (informUser)
        {
            /*  Copy some values from the old entry */
            pNewElement->addr       = pExistingElement->addr;
            pNewElement->privFlags  = pExistingElement->privFlags;
            pNewElement->pktFlags   = pExistingElement->pktFlags;
            pNewElement->interval   = pExistingElement->interval;
            pNewElement->timeToGo   = pExistingElement->timeToGo;
            pNewElement->dataSize   = pExistingElement->dataSize;
            pNewElement->grossSize  = pExistingElement->grossSize;
            pNewElement->socketIdx  = pExistingElement->socketIdx;
            pNewElement->userRef    = pExistingElement->userRef;

            /*  remove the old one, insert the new one  */
            trdp_queueDelElement(&appHandle->pRcvQueue, pExistingElement);
            trdp_queueInsFirst(&appHandle->pRcvQueue, pNewElement);

            /*  One block is always kept    */
            memset(pExistingElement, 0, sizeof(PD_ELE_T));
            pNewElement = pExistingElement;
        }
    }
    else    /* there is no subscription, keep all values sane   */
    {
        pNewElement->addr.comId         = vos_ntohl(pNewElement->frameHead.comId);
        pNewElement->addr.srcIpAddr     = subHandle.srcIpAddr;
        pNewElement->addr.destIpAddr    = subHandle.destIpAddr;
        pNewElement->privFlags          = TRDP_PRIV_NONE;
        pNewElement->pktFlags           = TRDP_FLAGS_NONE;
        pNewElement->interval.tv_sec    = 0;
        pNewElement->interval.tv_usec   = 0;
        pNewElement->timeToGo.tv_sec    = 0;
        pNewElement->timeToGo.tv_usec   = 0;
        pNewElement->dataSize           = vos_ntohl(pNewElement->frameHead.datasetLength);
        pNewElement->grossSize          = trdp_packetSizePD(pNewElement->dataSize);
        pNewElement->socketIdx          = 0;
        pNewElement->userRef            = 0;
        resultCode                      = TRDP_NOSUB_ERR;
    }

    if (informUser)
    {
        /*  If a callback was provided, call it now */
        if (appHandle->pdDefault.pfCbFunction != NULL)
        {
            TRDP_PD_INFO_T theMessage;
            theMessage.comId        = pNewElement->addr.comId;
            theMessage.srcIpAddr    = pNewElement->addr.srcIpAddr;
            theMessage.destIpAddr   = pNewElement->addr.destIpAddr;
            theMessage.topoCount    = vos_ntohl(pNewElement->frameHead.topoCount);
            theMessage.msgType      = vos_ntohs(pNewElement->frameHead.msgType);
            theMessage.seqCount     = vos_ntohl(pNewElement->frameHead.sequenceCounter);
            theMessage.protVersion  = vos_ntohs(pNewElement->frameHead.protocolVersion);
            theMessage.subs         = vos_ntohs(pNewElement->frameHead.subsAndReserved);
            theMessage.offsetAddr   = vos_ntohs(pNewElement->frameHead.offsetAddress);
            theMessage.replyComId   = vos_ntohl(pNewElement->frameHead.replyComId);
            theMessage.replyIpAddr  = vos_ntohl(pNewElement->frameHead.replyIpAddress);
            theMessage.pUserRef     = pNewElement->userRef;      /* TBD: User reference given with the local subscribe?   */
            theMessage.resultCode   = resultCode;

            appHandle->pdDefault.pfCbFunction(appHandle->pdDefault.pRefCon,
                                              &theMessage,
                                              pNewElement->data,
                                              pNewElement->dataSize);
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
    UINT32  myCRC = vos_crc32(0L, NULL, 0);

    /* increment counter with each telegram */
    pPacket->curSeqCnt++;
    pPacket->frameHead.sequenceCounter = vos_htonl(pPacket->curSeqCnt);

    /* Compute CRC32   */
    myCRC   = vos_crc32(myCRC, (UINT8 *)&pPacket->frameHead, sizeof(PD_HEADER_T) - sizeof(UINT32));
    pPacket->frameHead.frameCheckSum    = MAKE_LE(myCRC);
}

/******************************************************************************/
/** Check if the PD header values and the CRCs are sane
 *
 *  @param[in]      pPacket			pointer to the packet to check
 *  @param[in]      packetSize		max size to check
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_CRC_ERR
 */
TRDP_ERR_T trdp_pdCheck (
    PD_HEADER_T *pPacket,
    INT32       packetSize)
{
    UINT32      myCRC       = vos_crc32(0L, NULL, 0);
    UINT32      *pDataFCS   = (UINT32 *)((UINT8 *)pPacket + packetSize - 4);
    TRDP_ERR_T  err         = TRDP_NO_ERR;

    /*	Check size	*/
    if (packetSize < MIN_PD_HEADER_SIZE ||
        packetSize > MAX_PD_PACKET_SIZE)
    {
        sPDComStats.headerInFrameErr++;
        vos_printf(VOS_LOG_INFO, "PDframe size error (%u))\n", (UINT32) packetSize);
        err = TRDP_WIRE_ERR;
    }

    /*	Check Header CRC (FCS)	*/
    
    myCRC = vos_crc32(myCRC, (UINT8 *) pPacket, sizeof(PD_HEADER_T) - 4);
    
    if (pPacket->frameCheckSum != MAKE_LE(myCRC))
    {
        sPDComStats.headerInCRCErr++;
        vos_printf(VOS_LOG_INFO, "PDframe crc error (%08x != %08x))\n", pPacket->frameCheckSum, MAKE_LE(myCRC));
        err = TRDP_CRC_ERR;
    }

    /*	Check protocol version	*/
    else if ((vos_ntohs(pPacket->protocolVersion) & 0xFF000000) != (IP_PD_PROTO_VER & 0xFF000000))
    {
        sPDComStats.headerInProtoErr++;
        vos_printf(VOS_LOG_INFO, "PDframe protocol error (%04x != %04x))\n",
                   vos_ntohs(pPacket->protocolVersion),
                   IP_PD_PROTO_VER);
        err = TRDP_WIRE_ERR;
    }

    /*	Check type	*/
    else if (vos_ntohs(pPacket->msgType) != TRDP_MSG_PD ||
             vos_ntohs(pPacket->msgType) != TRDP_MSG_PR ||
             vos_ntohs(pPacket->msgType) != TRDP_MSG_PE)
    {
        sPDComStats.headerInFrameErr++;
        vos_printf(VOS_LOG_INFO, "PDframe type error, received %04x\n", vos_ntohs(pPacket->msgType));
        err = TRDP_WIRE_ERR;
    }

    /*	Check Data CRC (FCS)	*/
    else if (pPacket->datasetLength > 0)
    {
        myCRC = vos_crc32(myCRC, (UINT8 *)pPacket + sizeof(PD_HEADER_T), pPacket->datasetLength);

        if (*pDataFCS != MAKE_LE(myCRC))
        {
            sPDComStats.headerInCRCErr++;
            vos_printf(VOS_LOG_INFO, "PDframe crc error (%08x != %08x))\n", *pDataFCS, MAKE_LE(myCRC));
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
 *  @param[in]      pdSock			socket descriptor
 *  @param[in]      pPacket			pointer to packet to be sent
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_IO_ERR
 */
TRDP_ERR_T  trdp_pdSend (
    INT32           pdSock,
    PD_ELE_T        *pPacket)
{
    VOS_ERR_T err = VOS_NO_ERR;
    UINT32      destIp = pPacket->addr.destIpAddr;
    
    /*  check for temporary address (PD PULL):  */
    if (pPacket->pullIpAddress != 0)
    {
        destIp = pPacket->pullIpAddress;
        pPacket->pullIpAddress = 0;
    }
    
    err = vos_sockSendUDP(pdSock,
                          (UINT8 *)&pPacket->frameHead,
                          pPacket->grossSize,
                          destIp,
                          IP_PD_UDP_PORT);

    if (err != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "trdp_pdSend failed\n");
        return TRDP_IO_ERR;
    }
    else
    {
        sPDComStats.headerOutPackets++;
    }
    return TRDP_NO_ERR;
}
