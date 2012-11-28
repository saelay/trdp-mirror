/******************************************************************************/
/**
 * @file            trdp_mdcom.c
 *
 * @brief           Functions for MD communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Simone Pachera, FARsystems
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

#include "trdp_if_light.h"
#include "trdp_utils.h"
#include "trdp_mdcom.h"


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
/** Send MD packet
 *
 *  @param[in]      mdSock          socket descriptor
 *  @param[in]      pPacket         pointer to packet to be sent
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_UNKNOWN_ERR    error
 */
TRDP_ERR_T  trdp_sendMD (
    int             mdSock,
    const MD_ELE_T  *pPacket)
{
    TRDP_ERR_T err = TRDP_UNKNOWN_ERR;


    if (err != TRDP_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "trdp_sendMD failed\n");
        return TRDP_IO_ERR;
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Receive MD packet
 *
 *  @param[in]      mdSock          socket descriptor
 *  @param[out]     ppPacket        pointer to pointer to received packet
 *  @param[out]     pSize           pointer to size of received packet
 *  @param[out]     pIPAddr         pointer to source IP address of packet
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_UNKNOWN_ERR    error
 */
TRDP_ERR_T  trdp_rcvMD (
    int         mdSock,
    MD_HEADER_T * *ppPacket,
    INT32       *pSize,
    UINT32      *pIPAddr)
{
    TRDP_ERR_T err = TRDP_UNKNOWN_ERR;


    return err;
}

/******************************************************************************/
/** Check for incoming md packet
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      pPacket         pointer to the packet to check
 *  @param[in]      packetSize      size of the packet
 */
TRDP_ERR_T trdp_mdCheck (
    TRDP_SESSION_PT appHandle,
    MD_HEADER_T     *pPacket,
    INT32           packetSize)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;

    UINT32      l_datasetLength = vos_ntohl(pPacket->datasetLength);

    /* size shall be in MIN..MAX */
    if (TRDP_NO_ERR == err)
    {
        /* Min size is sizeof(MD_HEADER_T) because in case of no data no further data and data crc32 are added */
        if (packetSize < (sizeof(MD_HEADER_T)) ||
            packetSize > TRDP_MAX_MD_PACKET_SIZE)
        {
            appHandle->stats.udpMd.numProtErr++;
            vos_printf(VOS_LOG_ERROR, "MDframe size error (%u)\n",
                       (UINT32) packetSize);
            err = TRDP_WIRE_ERR;
        }
    }

    /*	crc check */
    if (TRDP_NO_ERR == err)
    {
        /* Header CRC */
        UINT32 crc32 = vos_crc32(0xffffffff, (UINT8 *)pPacket, sizeof(MD_HEADER_T));
        if (crc32 != 0)
        {
            appHandle->stats.udpMd.numCrcErr++;
            vos_printf(VOS_LOG_ERROR, "MDframe header crc error.\n");
            err = TRDP_WIRE_ERR;
        }

        /* Data CRC */
        if(l_datasetLength > 0)
        {
            /* Check only if we have some data */
            UINT32  crc32       = vos_crc32(0xffffffff, (UINT8 *) pPacket + sizeof(MD_HEADER_T), l_datasetLength);
            UINT32  le_crc32    = MAKE_LE(crc32);

            UINT8   *pDataCRC   = (UINT8 *) pPacket + packetSize - 4;
            UINT32  pktCRC      = ((UINT32 *) pDataCRC)[0];

            if (le_crc32 != pktCRC)
            {
                appHandle->stats.udpMd.numCrcErr++;
                vos_printf(VOS_LOG_ERROR, "MDframe data crc error.\n");
                err = TRDP_WIRE_ERR;
            }
        }
    }

    /*	Check protocol version	*/
    if (TRDP_NO_ERR == err)
    {
        UINT16 l_protocolVersion = vos_ntohs(pPacket->protocolVersion);
        #define TRDP_PROTOCOL_VERSION_CHECK_MASK  0xFF00
        if ((l_protocolVersion & TRDP_PROTOCOL_VERSION_CHECK_MASK) !=
            (TRDP_PROTO_VER & TRDP_PROTOCOL_VERSION_CHECK_MASK))
        {
            appHandle->stats.udpMd.numProtErr++;
            vos_printf(VOS_LOG_ERROR, "MDframe protocol error (%04x != %04x))\n",
                       l_protocolVersion,
                       TRDP_PROTO_VER);
            err = TRDP_WIRE_ERR;
        }
    }

    /*	Check protocol type	*/
    if (TRDP_NO_ERR == err)
    {
        UINT16 l_msgType = vos_ntohs(pPacket->msgType);
        switch(l_msgType)
        {
            /* valid message type ident */
            case TRDP_MSG_MN:
            case TRDP_MSG_MR:
            case TRDP_MSG_MP:
            case TRDP_MSG_MQ:
            case TRDP_MSG_MC:
            case TRDP_MSG_ME:
            {}
             break;
            /* invalid codes */
            default:
            {
                appHandle->stats.udpMd.numProtErr++;
                vos_printf(VOS_LOG_ERROR, "MDframe type error, received %04x\n",
                           l_msgType);
                err = TRDP_WIRE_ERR;
            }
            break;
        }
    }

    /* check telegram length */
    if (TRDP_NO_ERR == err)
    {
        UINT32  l_datasetLength = vos_ntohl(pPacket->datasetLength);
        UINT32  expectedLength  = 0;
        if(l_datasetLength > 0)
        {
            expectedLength = sizeof(MD_HEADER_T) + l_datasetLength + 4;
        }
        else
        {
            expectedLength = sizeof(MD_HEADER_T);
        }

        if (packetSize < expectedLength)
        {
            appHandle->stats.udpMd.numProtErr++;
            vos_printf(VOS_LOG_ERROR, "MDframe invalid length, received %d, expected %d\n",
                       packetSize,
                       expectedLength);
            err = TRDP_WIRE_ERR;
        }
    }

    return err;
}


/******************************************************************************/
/** Update the header values
 *
 *  @param[in]      pPacket         pointer to the packet to update
 */
void    trdp_mdUpdate (
    MD_ELE_T *pPacket)
{
    /* Initialize CRC calculation */
    UINT32  myCRC   = vos_crc32(0xffffffff, NULL, 0);
    UINT32  myCRC1  = myCRC;

    /* Get header and packet check sum values */
    UINT32  *hFCS   = &pPacket->frameHead.frameCheckSum;
    UINT32  *pFCS   = (UINT32 *)((UINT8 *)&pPacket->frameHead + pPacket->grossSize - 4);

    /* increment counter with each telegram */
    pPacket->frameHead.sequenceCounter =
        vos_htonl(vos_ntohl(pPacket->frameHead.sequenceCounter) + 1);

    /* Calculate CRC for message head */
    myCRC = vos_crc32(myCRC,
                      (UINT8 *)&pPacket->frameHead,
                      sizeof(MD_HEADER_T) - 4);
    /* Convert to Little Endian */
    *hFCS = MAKE_LE(myCRC);

    /*
       Calculate CRC for message packet
     */
    if(pPacket->frameHead.datasetLength > 0)
    {
        myCRC1 = vos_crc32(myCRC1,
                           &pPacket->data[0],
                           vos_ntohl(pPacket->frameHead.datasetLength));
        *pFCS = MAKE_LE(myCRC1);
    }
}

/******************************************************************************/
/** Send MD packet
 *
 *  @param[in]      pdSock          socket descriptor
 *  @param[in]      pPacket         pointer to packet to be sent
 *  @retval         != NULL         error
 */
TRDP_ERR_T  trdp_mdSend (
    INT32           pdSock,
    const MD_ELE_T  *pPacket)
{
    VOS_ERR_T err = VOS_NO_ERR;

    if ((pPacket->pktFlags & TRDP_FLAGS_TCP) != 0)
    {
        err = vos_sockSendTCP(pdSock, (UINT8 *)&pPacket->frameHead, pPacket->grossSize);

    }
    else
    {
        err = vos_sockSendUDP(pdSock,
                              (UINT8 *)&pPacket->frameHead,
                              pPacket->grossSize,
                              pPacket->addr.destIpAddr,
                              TRDP_MD_UDP_PORT);
    }

    if (err != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "trdp_mdSend failed\n");
        return TRDP_IO_ERR;
    }
    /*
     else
    {
        sMDComStats.headerOutPackets++;
    }
     */
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Receive MD packet
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      mdSock          socket descriptor
 *  @param[in]      pPacket         pointer to received packet
 *  @retval         != NULL         error
 */
TRDP_ERR_T  trdp_mdRecv (
    TRDP_SESSION_PT appHandle,
    INT32           mdSock,
    MD_ELE_T        *pPacket)
{
    VOS_ERR_T   err = VOS_NO_ERR;

    int         Size = pPacket->grossSize;

    if ((pPacket->pktFlags & TRDP_FLAGS_TCP) != 0)
    {
        err = vos_sockReceiveTCP(mdSock, (UINT8 *)&pPacket->frameHead, &Size);
        vos_printf(VOS_LOG_INFO, "vos_sockReceiveTCP Size = %d\n", Size);
        err = VOS_NO_ERR;

    }
    else
    {

        err = vos_sockReceiveUDP(
                mdSock,
                (UINT8 *)&pPacket->frameHead,
                &Size,
                &pPacket->addr.srcIpAddr);
    }

    pPacket->dataSize = Size;

    if (err == VOS_NODATA_ERR)
    {
        /* no data -> rx timeout */
        return TRDP_TIMEOUT_ERR;
    }

    if (err != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "trdp_mdRecv failed = %d\n", err);
        return TRDP_IO_ERR;
    }

    /* received data */

    err = trdp_mdCheck(appHandle, &pPacket->frameHead, Size);
    if (err != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "trdp_mdRecv failed = %d\n", err);
        return TRDP_IO_ERR;
    }

    /* packet is ok */
    appHandle->stats.udpMd.numRcv++;

    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Receiving MD messages
 *  Read the receive socket for arriving MDs, copy the packet to a new MD_ELE_T
 *  Check for protocol errors and dispatch to proper receive queue.
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      sock                the socket to read from
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_WIRE_ERR       protocol error (late packet, version mismatch)
 *  @retval         TRDP_QUEUE_ERR      not in queue
 *  @retval         TRDP_CRC_ERR        header checksum
 *  @retval         TRDP_TOPOCOUNT_ERR  invalid topocount
 */
TRDP_ERR_T  trdp_mdReceive (
    TRDP_SESSION_PT appHandle,
    INT32           sock)
{
    VOS_ERR_T err = VOS_NO_ERR;

    /* get buffer at 1st call */
    if (appHandle->pMDRcvEle == NULL)
    {
        appHandle->pMDRcvEle = (MD_ELE_T *) vos_memAlloc(sizeof(MD_ELE_T) + TRDP_MAX_MD_PACKET_SIZE - sizeof(MD_HEADER_T));
        if (NULL == appHandle->pMDRcvEle)
        {
            vos_printf(VOS_LOG_ERROR, "Receiving MD: Out of receive buffers!\n");
            return TRDP_MEM_ERR;
        }

        memset(appHandle->pMDRcvEle, 0, sizeof(MD_ELE_T));
        appHandle->pMDRcvEle->grossSize = TRDP_MAX_MD_PACKET_SIZE;
        appHandle->pMDRcvEle->pktFlags  = appHandle->mdDefault.flags;
    }

    /* get packet */
    err = trdp_mdRecv(appHandle, sock, appHandle->pMDRcvEle);
    if (TRDP_NO_ERR != err)
    {
        return err;
    }

    /* process message */
    {
        MD_HEADER_T *pH = &appHandle->pMDRcvEle->frameHead;
        int         lF  = appHandle->pMDRcvEle->dataSize;

        vos_printf(VOS_LOG_ERROR,
                   "Received MD packet (space: %d len: %d)\n",
                   appHandle->pMDRcvEle->grossSize,
                   appHandle->pMDRcvEle->dataSize);

        /* Display incoming header */

        {
            int i;
            printf("***** from **** : %08X\n", appHandle->pMDRcvEle->addr.srcIpAddr);
            printf("sequenceCounter = %d\n", vos_ntohl(pH->sequenceCounter));
            printf("protocolVersion = %d\n", vos_ntohs(pH->protocolVersion));
            printf("msgType         = x%04X\n", vos_ntohs(pH->msgType        ));
            printf("comId           = %d\n", vos_ntohl(pH->comId          ));
            printf("topoCount       = %d\n", vos_ntohl(pH->topoCount      ));
            printf("datasetLength   = %d\n", vos_ntohl(pH->datasetLength  ));
            printf("replyStatus     = %d\n", vos_ntohl(pH->replyStatus    ));
            printf("sessionID       = ");
            for(i = 0; i < 16; i++)
            {
                printf("%02X ", pH->sessionID[i]);
            }
            printf("\n");
            printf("replyTimeout    = %d\n", vos_ntohl(pH->replyTimeout   ));
            printf("sourceURI       = ");
            for(i = 0; i < 32; i++)
            {
                if (pH->sourceURI[i])
                {
                    printf("%c", pH->sourceURI[i]);
                }
            }
            printf("\n");
            printf("destinationURI  = ");
            for(i = 0; i < 32; i++)
            {
                if (pH->destinationURI[i])
                {
                    printf("%c", pH->destinationURI[i]);
                }
            }
            printf("\n");
        }


        /* checks */
        UINT32  l_comId = vos_ntohl(pH->comId);

        /* topo counter check for context */
        UINT32  l_topoCount = vos_ntohl(pH->topoCount);

        /* check for topo counter */
        if (l_topoCount != 0 && appHandle->topoCount != 0 && l_topoCount != appHandle->topoCount)
        {
            vos_printf(VOS_LOG_INFO,
                       "MD data with wrong topocount ignored (comId %u, topo %u)\n",
                       l_comId,
                       l_topoCount);
            return TRDP_TOPO_ERR;
        }

        /* message type */
        UINT16      l_msgType = vos_ntohs(pH->msgType);

        /* find for subscriber */
        MD_ELE_T    *iterMD;


        /* seach for existing listener */
        for(iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
        {
            /* check for correct communication ID ... */
            if (l_comId != iterMD->u.listener.comId)
            {
                continue;
            }

            /* 1st receive */
            if (iterMD->stateEle == TRDP_MD_ELE_ST_RX_ARM)
            {
                /* message type depending */
                switch(l_msgType)
                {
                    /* 1st level: receive notify or request telegram */

                    case TRDP_MSG_MN: /* notify */
                    case TRDP_MSG_MR: /* request */
                    {
                        /* next step if request */
                        if (TRDP_MSG_MR == l_msgType)
                        {
                            /* in case for request, the listener is busy waiting for application reply or error */
                            iterMD->stateEle = TRDP_MD_ELE_ST_RX_REQ_W4AP_REPLY;
                        }

                        /* receive time */
                        vos_getTime(&iterMD->timeToGo);

                        /* timeout value */
                        iterMD->interval.tv_sec     = vos_ntohl(pH->replyTimeout) / 1000000;
                        iterMD->interval.tv_usec    = vos_ntohl(pH->replyTimeout) % 1000000;

                        /* save session Id for next steps */
                        memcpy(iterMD->sessionID, pH->sessionID, 16);

                        /* copy message to proper listener */
                        memcpy(&iterMD->frameHead, pH, lF);

                        if (appHandle->mdDefault.pfCbFunction != NULL)
                        {
                            TRDP_MD_INFO_T theMessage;

                            theMessage.srcIpAddr    = appHandle->pMDRcvEle->addr.srcIpAddr;
                            theMessage.destIpAddr   = 0;
                            theMessage.seqCount     = vos_ntohl(pH->sequenceCounter);
                            theMessage.protVersion  = vos_ntohs(pH->protocolVersion);
                            theMessage.msgType      = vos_ntohs(pH->msgType);
                            theMessage.comId        = iterMD->u.listener.comId;
                            theMessage.topoCount    = vos_ntohl(pH->topoCount);
                            theMessage.userStatus   = 0;
                            theMessage.replyStatus  = vos_ntohs(pH->replyStatus);
                            memcpy(theMessage.sessionId, pH->sessionID, 16);
                            theMessage.replyTimeout = vos_ntohl(pH->replyTimeout);
                            memcpy(theMessage.destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);
                            memcpy(theMessage.srcURI, pH->sourceURI, TRDP_MAX_URI_USER_LEN);
                            theMessage.numReplies   = 0;
                            theMessage.pUserRef     = appHandle->mdDefault.pRefCon;
                            theMessage.resultCode   = TRDP_NO_ERR;

                            appHandle->mdDefault.pfCbFunction(
                                appHandle->mdDefault.pRefCon,
                                &theMessage,
                                (UINT8 *)pH, lF);
                        }

                        /* TCP and Notify message */
                        if (((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0) && (TRDP_MSG_MN == l_msgType))
                        {
                            int find_sock;
                            int sock_position;

                            for(find_sock = 0; find_sock < VOS_MAX_SOCKET_CNT; find_sock++)
                            {
                                if((appHandle->iface[find_sock].sock == sock)
                                   && (appHandle->iface[find_sock].sock != -1))
                                {
                                    sock_position = find_sock;
                                    break;
                                }
                            }

                            if(appHandle->iface[sock_position].usage == 1)
                            {
                                vos_printf(VOS_LOG_INFO, "vos_sockClose (Num = %d) from the iface\n",
                                           appHandle->iface[sock_position].sock);
                                vos_sockClose(appHandle->iface[sock_position].sock);
                            }
                            else
                            {
                                appHandle->iface[sock_position].usage--;
                            }

                            /* Delete the socket from the iface */
                            vos_printf(VOS_LOG_INFO, "Deleting socket (Num = %d) from the iface\n",
                                       appHandle->iface[sock_position].sock);
                            vos_printf(VOS_LOG_INFO, "Close socket iface index=%d\n", sock_position);
                            appHandle->iface[sock_position].sock = -1;
                            appHandle->iface[sock_position].sendParam.qos   = 0;
                            appHandle->iface[sock_position].sendParam.ttl   = 0;
                            appHandle->iface[sock_position].usage           = 0;
                            appHandle->iface [sock_position].bindAddr       = 0;
                            appHandle->iface[sock_position].type = 0;

                        }

                    }
                    break;

                    /* 2nd step: receive of reply message with or without confirmation request, error too handled */
                    case TRDP_MSG_MP: /* reply no conf */
                    case TRDP_MSG_MQ: /* reply with conf */
                    case TRDP_MSG_MC: /* confirm */
                    case TRDP_MSG_ME: /* reply error */
                    {
                        /* check on opened caller session to pair received replay/error telegram */
                        MD_ELE_T *sender_ele;
                        for (sender_ele = appHandle->pMDSndQueue; sender_ele != NULL; sender_ele = sender_ele->pNext)
                        {
                            /* check for comID .... */
                            if (sender_ele->u.caller.comId != l_comId)
                            {
                                continue;
                            }

                            /* check for session ... */
                            if (0 != memcmp(&sender_ele->sessionID, pH->sessionID, 16))
                            {
                                continue;
                            }

                            /**--**/

                            /* the sender sent request and is waiting for reply */
                            if (sender_ele->stateEle == TRDP_MD_ELE_ST_TX_REQUEST_W4Y)
                            {
                                /* reply ok or reply error */
                                if (l_msgType == TRDP_MSG_MP || l_msgType == TRDP_MSG_MQ || l_msgType == TRDP_MSG_ME)
                                {

                                    vos_printf(VOS_LOG_INFO,
                                               "MD RX/TX match (comId %u, topo %u)\n",
                                               l_comId,
                                               l_topoCount);

                                    /* if received reply with confirm request .... */
                                    if (l_msgType == TRDP_MSG_MQ)
                                    {
                                        /* ... move the listener waiting for sending confirm telegram */
                                        iterMD->stateEle = TRDP_MD_ELE_ST_RX_REPLY_W4AP_CONF;

                                        /* copy session id to listener */
                                        memcpy(&iterMD->sessionID, &sender_ele->sessionID, 16);

                                        /* forward timeout */
                                        iterMD->timeToGo    = sender_ele->timeToGo;
                                        iterMD->interval    = sender_ele->interval;

                                        /* Copy other identification arams */
                                        iterMD->u.listener.comId        = l_comId;
                                        iterMD->u.listener.topoCount    = l_topoCount;
                                        iterMD->u.listener.destIpAddr   = appHandle->pMDRcvEle->addr.srcIpAddr;
                                        memcpy(iterMD->u.listener.destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);
                                    }

                                    /* copy message to proper listener */
                                    memcpy(&iterMD->frameHead, pH, lF);

                                    if (appHandle->mdDefault.pfCbFunction != NULL)
                                    {
                                        TRDP_MD_INFO_T theMessage;

                                        theMessage.srcIpAddr    = appHandle->pMDRcvEle->addr.srcIpAddr;
                                        theMessage.destIpAddr   = 0;
                                        theMessage.seqCount     = vos_ntohl(pH->sequenceCounter);
                                        theMessage.protVersion  = vos_ntohs(pH->protocolVersion);
                                        theMessage.msgType      = vos_ntohs(pH->msgType);
                                        theMessage.comId        = iterMD->u.listener.comId;
                                        theMessage.topoCount    = vos_ntohl(pH->topoCount);
                                        theMessage.userStatus   = 0;
                                        theMessage.replyStatus  = vos_ntohs(pH->replyStatus);
                                        memcpy(theMessage.sessionId, pH->sessionID, 16);
                                        theMessage.replyTimeout = vos_ntohl(pH->replyTimeout);
                                        memcpy(theMessage.destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);
                                        memcpy(theMessage.srcURI, pH->sourceURI, TRDP_MAX_URI_USER_LEN);
                                        theMessage.numReplies   = 0;
                                        theMessage.pUserRef     = appHandle->mdDefault.pRefCon;
                                        theMessage.resultCode   = TRDP_NO_ERR;

                                        appHandle->mdDefault.pfCbFunction(
                                            appHandle->mdDefault.pRefCon,
                                            &theMessage,
                                            (UINT8 *)pH, lF);

                                    }


                                    /* TCP and Reply/ReplyError message */
                                    if (((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                                        && ((TRDP_MSG_MP == l_msgType) || (TRDP_MSG_ME == l_msgType)))
                                    {
                                        int find_sock;
                                        int sock_position;

                                        for(find_sock = 0; find_sock < VOS_MAX_SOCKET_CNT; find_sock++)
                                        {
                                            if((appHandle->iface[find_sock].sock == sock)
                                               && (appHandle->iface[find_sock].sock != -1))
                                            {
                                                sock_position = find_sock;
                                                break;
                                            }
                                        }

                                        if(appHandle->iface[sock_position].usage == 1)
                                        {
                                            vos_printf(VOS_LOG_INFO,
                                                       "vos_sockClose (Nº = %d) from the iface\n",
                                                       appHandle->iface[sock_position].sock);
                                            vos_sockClose(appHandle->iface[sock_position].sock);
                                        }
                                        else
                                        {
                                            appHandle->iface[sock_position].usage--;
                                        }

                                        /* Delete the socket from the iface */
                                        vos_printf(VOS_LOG_INFO,
                                                   "Deleting socket (Nº = %d) from the iface\n",
                                                   appHandle->iface[sock_position].sock);
                                        vos_printf(VOS_LOG_INFO, "Close socket iface index=%d\n", sock_position);
                                        appHandle->iface[sock_position].sock = -1;
                                        appHandle->iface[sock_position].sendParam.qos   = 0;
                                        appHandle->iface[sock_position].sendParam.ttl   = 0;
                                        appHandle->iface[sock_position].usage           = 0;
                                        appHandle->iface [sock_position].bindAddr       = 0;
                                        appHandle->iface[sock_position].type = 0;

                                    }


                                    /* Remove element from queue */
                                    trdp_MDqueueDelElement(&appHandle->pMDSndQueue, sender_ele);

                                    /* free element */
                                    vos_memFree(sender_ele);

                                    /* stop loop */
                                    break;
                                }
                            }
                            /**--**/

                            /* the sender sent reply and is waiting for confirm */
                            if (sender_ele->stateEle == TRDP_MD_ELE_ST_TX_REPLYQUERY_W4C)
                            {
                                /* confirm or error */
                                if (l_msgType == TRDP_MSG_MC)
                                {
                                    /* copy message to proper listener */
                                    memcpy(&iterMD->frameHead, pH, lF);

                                    if (appHandle->mdDefault.pfCbFunction != NULL)
                                    {
                                        TRDP_MD_INFO_T theMessage;

                                        theMessage.srcIpAddr    = appHandle->pMDRcvEle->addr.srcIpAddr;
                                        theMessage.destIpAddr   = 0;
                                        theMessage.seqCount     = vos_ntohl(pH->sequenceCounter);
                                        theMessage.protVersion  = vos_ntohs(pH->protocolVersion);
                                        theMessage.msgType      = vos_ntohs(pH->msgType);
                                        theMessage.comId        = iterMD->u.listener.comId;
                                        theMessage.topoCount    = vos_ntohl(pH->topoCount);
                                        theMessage.userStatus   = 0;
                                        theMessage.replyStatus  = vos_ntohs(pH->replyStatus);
                                        memcpy(theMessage.sessionId, pH->sessionID, 16);
                                        theMessage.replyTimeout = vos_ntohl(pH->replyTimeout);
                                        memcpy(theMessage.destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);
                                        memcpy(theMessage.srcURI, pH->sourceURI, TRDP_MAX_URI_USER_LEN);
                                        theMessage.numReplies   = 0;
                                        theMessage.pUserRef     = appHandle->mdDefault.pRefCon;
                                        theMessage.resultCode   = TRDP_NO_ERR;

                                        appHandle->mdDefault.pfCbFunction(
                                            appHandle->mdDefault.pRefCon,
                                            &theMessage,
                                            (UINT8 *)pH, lF);
                                    }


                                    /* TCP and Reply/ReplyError message */
                                    if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                                    {
                                        int find_sock;
                                        int sock_position;

                                        for(find_sock = 0; find_sock < VOS_MAX_SOCKET_CNT; find_sock++)
                                        {
                                            if((appHandle->iface[find_sock].sock == sock)
                                               && (appHandle->iface[find_sock].sock != -1))
                                            {
                                                sock_position = find_sock;
                                                break;
                                            }
                                        }

                                        if(appHandle->iface[sock_position].usage == 1)
                                        {
                                            vos_printf(VOS_LOG_INFO,
                                                       "vos_sockClose (Nº = %d) from the iface\n",
                                                       appHandle->iface[sock_position].sock);
                                            vos_sockClose(appHandle->iface[sock_position].sock);
                                        }
                                        else
                                        {
                                            appHandle->iface[sock_position].usage--;
                                        }

                                        /* Delete the socket from the iface */
                                        vos_printf(VOS_LOG_INFO,
                                                   "Deleting socket (Nº = %d) from the iface\n",
                                                   appHandle->iface[sock_position].sock);
                                        vos_printf(VOS_LOG_INFO, "Close socket iface index=%d\n", sock_position);
                                        appHandle->iface[sock_position].sock = -1;
                                        appHandle->iface[sock_position].sendParam.qos   = 0;
                                        appHandle->iface[sock_position].sendParam.ttl   = 0;
                                        appHandle->iface[sock_position].usage           = 0;
                                        appHandle->iface [sock_position].bindAddr       = 0;
                                        appHandle->iface[sock_position].type = 0;

                                    }


                                    /* Remove element from queue */
                                    trdp_MDqueueDelElement(&appHandle->pMDSndQueue, sender_ele);

                                    /* free element */
                                    vos_memFree(sender_ele);

                                    /* stop loop */
                                    break;
                                }
                            }
                        }
                    }
                    break;

                    default:
                    {}
                     break;
                }

            }
        }
    }

    return TRDP_NO_ERR;
}
