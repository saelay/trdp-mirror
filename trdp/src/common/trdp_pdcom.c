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

#include <stdlib.h>
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
 */
void    trdp_pdInit (
    PD_ELE_T    *pPacket,
    TRDP_MSG_T  type,
    UINT32      topoCount
    )
{
    if (pPacket == NULL)
    {
        return;
    }

    pPacket->frameHead.sequenceCounter  = 0;
    pPacket->frameHead.protocolVersion  = vos_htons(IP_PD_PROTO_VER);
    pPacket->frameHead.topoCount        = vos_htonl(topoCount);
    pPacket->frameHead.comId            = vos_htonl(pPacket->addr.comId);
    pPacket->frameHead.msgType          = vos_htons(type);
    pPacket->frameHead.datasetLength    = vos_htonl(pPacket->dataSize);
    pPacket->frameHead.subsAndReserved  = 0;
    pPacket->frameHead.offsetAddress    = 0;
    pPacket->frameHead.replyComId       = 0;
    pPacket->frameHead.replyIpAddress   = 0;
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
    if (pPacket == NULL || pData == NULL || dataSize == 0)
    {
        return TRDP_PARAM_ERR;
    }

    if (marshall == NULL)
    {
        memcpy(pPacket->data, pData, dataSize);
        return TRDP_NO_ERR;
    }
    return marshall(refCon,
                    pPacket->addr.comId,
                    pData,
                    pPacket->data,
                    &dataSize);
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
    return unmarshall(refCon,
                      pPacket->addr.comId,
                      pData,
                      pPacket->data,
                      &dataSize);
}

/******************************************************************************/
/** Receiving PD messages
 *  Read the receive socket for arriving PDs, copy the packet to a new PD_ELE_T
 *  Check for protocol errors and compare the received data to the data in our receive queue.
 *	If it is a new packet, discard it (TBD: maybe for another session!).
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
    TRDP_ERR_T      err;
    INT32 recSize;
    BOOL newData = 0;
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

    if (vos_sockReceiveUDP(sock, (UINT8 *) &pNewElement->frameHead, &recSize,
                           &subHandle.srcIpAddr) != VOS_NO_ERR)
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
        vos_printf(VOS_LOG_INFO,
                   "PD data with wrong topocount ignored (comId %u, topo %u)\n",
                   vos_ntohl(pNewElement->frameHead.comId),
                   vos_ntohl(pNewElement->frameHead.topoCount));
        return TRDP_TOPO_ERR;
    }

    /*  Compute the subscription handle */
    subHandle.comId = vos_ntohl(pNewElement->frameHead.comId);

    /*	Examine netbuffer, are we interested in this PD?	*/
    pExistingElement = trdp_queue_find_addr(appHandle->pRcvQueue, &subHandle);

    if (pExistingElement == NULL)
    {
        /*	We are not interested in this packet (TBD: maybe another session?)	*/
        vos_printf(VOS_LOG_INFO, "Ignoring comId %u, no subscription\n",
                   vos_ntohl(pNewElement->frameHead.comId));
        return TRDP_NO_ERR;
    }

    /*	Check if sequencecounter is OK	*/
    if (vos_ntohl(pNewElement->frameHead.sequenceCounter) <
        vos_ntohl(pExistingElement->frameHead.sequenceCounter))
    {
        /* Check for overrun situation  */
        if (vos_ntohl(pNewElement->frameHead.sequenceCounter) -
            vos_ntohl(pExistingElement->frameHead.sequenceCounter) <
            vos_ntohl(pNewElement->frameHead.sequenceCounter))
        {
            vos_printf(VOS_LOG_INFO, "Old PD data ignored (comId %u)\n",
                       vos_ntohl(pNewElement->frameHead.comId));
            return TRDP_NO_ERR;
        }
    }

    /*  Has the data changed?   */
    newData = memcmp(pNewElement->data,
                     pExistingElement->data,
                     pNewElement->dataSize);

    /*	Get the current time and compute the next time this packet should be
        received.	*/
    vos_getTime(&pExistingElement->timeToGo);
    vos_addTime(&pExistingElement->timeToGo, &pExistingElement->interval);

    if (newData)
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
        trdp_queue_del_element(&appHandle->pRcvQueue, pExistingElement);
        trdp_queue_ins_first(&appHandle->pRcvQueue, pNewElement);

        /*  One block is always kept    */
        memset(pExistingElement, 0, sizeof(PD_ELE_T));
        pNewElement = pExistingElement;

        /*  If a callback was provided, call it now */
        if (appHandle->pdDefault.pfCbFunction != NULL)
        {
            TRDP_PD_INFO_T theMessage;
            theMessage.comId        = pExistingElement->addr.comId;
            theMessage.srcIpAddr    = pExistingElement->addr.srcIpAddr;
            theMessage.destIpAddr   = pExistingElement->addr.destIpAddr;
            theMessage.topoCount    = vos_ntohl(
                    pExistingElement->frameHead.topoCount);
            theMessage.msgType  = vos_ntohs(pExistingElement->frameHead.msgType);
            theMessage.seqCount = vos_ntohl(
                    pExistingElement->frameHead.sequenceCounter);
            theMessage.protVersion = vos_ntohs(
                    pExistingElement->frameHead.protocolVersion);
            theMessage.subs = vos_ntohs(
                    pExistingElement->frameHead.subsAndReserved);
            theMessage.offsetAddr = vos_ntohs(
                    pExistingElement->frameHead.offsetAddress);
            theMessage.replyComId = vos_ntohl(
                    pExistingElement->frameHead.replyComId);
            theMessage.replyIpAddr = vos_ntohl(
                    pExistingElement->frameHead.replyIpAddress);
            theMessage.pUserRef     = pExistingElement->userRef;      /* TBD: User reference given with the local subscribe?   */
            theMessage.resultCode   = TRDP_NO_ERR;

            appHandle->pdDefault.pfCbFunction(
                appHandle->pdDefault.pRefCon,
                &theMessage,
                pExistingElement->data,
                pExistingElement->dataSize);
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Update the header values
 *
 *  @param[in]      pPacket			pointer to the packet to update
 */
void    trdp_pdUpdate (
    PD_ELE_T *pPacket)
{
    UINT32  myCRC   = vos_crc32(0L, NULL, 0);
    UINT32  *pFCS   =
        (UINT32 *)((UINT8 *)&pPacket->frameHead + pPacket->grossSize - 4);

    /* increment counter with each telegram */
    pPacket->frameHead.sequenceCounter =
        vos_htonl(vos_ntohl(pPacket->frameHead.sequenceCounter) + 1);

    /* Compute CRC32   */
    myCRC = vos_crc32(myCRC,
                      (UINT8 *)&pPacket->frameHead,
                      pPacket->grossSize - 4);
    *pFCS = MAKE_LE(myCRC);
}

/******************************************************************************/
/** Check if the PD header values are sane
 *
 *  @param[in]      pPacket			pointer to the packet to update
 *  @param[in]      packetSize		max size to check
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_CRC_ERR
 */
TRDP_ERR_T trdp_pdCheck (
    PD_HEADER_T *pPacket,
    INT32       packetSize)
{
    UINT32      myCRC   = vos_crc32(0L, NULL, 0);
    UINT32      *pFCS   = (UINT32 *)((UINT8 *)pPacket + packetSize - 4);
    int         len;
    TRDP_ERR_T  err = TRDP_NO_ERR;

    /*	Check size	*/
    if (packetSize < MIN_PD_HEADER_SIZE + 4 ||
        packetSize > MAX_PD_PACKET_SIZE)
    {
        sPDComStats.headerInFrameErr++;
        vos_printf(VOS_LOG_INFO, "PDframe size error (%u))\n",
                   (UINT32) packetSize);
        err = TRDP_WIRE_ERR;
    }
    /*	Check protocol version	*/
    else if ((vos_ntohs(pPacket->protocolVersion) & 0xFF000000) !=
             (IP_PD_PROTO_VER & 0xFF000000))
    {
        sPDComStats.headerInProtoErr++;
        vos_printf(VOS_LOG_INFO, "PDframe protocol error (%04x != %04x))\n",
                   vos_ntohs(pPacket->protocolVersion),
                   IP_PD_PROTO_VER);
        err = TRDP_WIRE_ERR;
    }
    else
    {

        /*	Check length and CRC (FCS)	*/
        len = sizeof(PD_HEADER_T) - 4; /* Fix value in the TRDP protocol */

        myCRC = vos_crc32(myCRC, (UINT8 *) pPacket, packetSize - 4);

        if (*pFCS != MAKE_LE(myCRC))
        {
            sPDComStats.headerInCRCErr++;
            vos_printf(VOS_LOG_INFO, "PDframe crc error (%08x != %08x))\n",
                       *(UINT32 *)((UINT8 *) pPacket + len),
                       MAKE_LE(myCRC));
            err = TRDP_WIRE_ERR;
        }
        /*	Check type	*/
        else if (vos_ntohs(pPacket->msgType) != TRDP_MSG_PD ||
                 vos_ntohs(pPacket->msgType) != TRDP_MSG_PR ||
                 vos_ntohs(pPacket->msgType) != TRDP_MSG_PE)
        {
            sPDComStats.headerInFrameErr++;
            vos_printf(VOS_LOG_INFO, "PDframe type error, received %04x\n",
                       vos_ntohs(pPacket->msgType));
            err = TRDP_WIRE_ERR;
        }
        else
        {
            err = TRDP_NO_ERR;
        }
    }

    return err;
}

/******************************************************************************/
/** Send PD packet
 *
 *  @param[in]      pdSock			socket descriptor
 *  @param[in]      pPacket			pointer to packet to be sent
 *  @retval         != NULL         error
 */
TRDP_ERR_T  trdp_pdSend (
    INT32           pdSock,
    const PD_ELE_T  *pPacket)
{
    VOS_ERR_T err = VOS_NO_ERR;

    err = vos_sockSendUDP(pdSock,
                          (uint8_t *)&pPacket->frameHead,
                          pPacket->grossSize,
                          pPacket->addr.destIpAddr,
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
