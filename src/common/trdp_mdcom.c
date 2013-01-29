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
/** Check for incoming md packet
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      pPacket         pointer to the packet to check
 *  @param[in]      packetSize      size of the packet
 */
TRDP_ERR_T trdp_mdCheck (
    TRDP_SESSION_PT appHandle,
    MD_HEADER_T     *pPacket,
    UINT32          packetSize)
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

    /*    crc check */
    if (TRDP_NO_ERR == err)
    {
        /* Check Header CRC */
        {
            UINT32 crc32 = vos_crc32(0xffffffff, (UINT8 *)pPacket, sizeof(MD_HEADER_T));

            if (crc32 != 0)
            {
                appHandle->stats.udpMd.numCrcErr++;
                vos_printf(VOS_LOG_ERROR, "MDframe header crc error.\n");
                err = TRDP_CRC_ERR;
           }
        }

        /* Check Data CRC */
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
                err = TRDP_CRC_ERR;
            }
        }
    }

    /*    Check protocol version    */
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

    /*    Check protocol type    */
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
void    trdp_mdUpdatePacket (
    MD_ELE_T *pPacket)
{
    /* Initialize CRC calculation */
    UINT32  myCRC   = vos_crc32(0xffffffff, NULL, 0);
    UINT32  myCRC1  = myCRC;

    /* Get header and packet check sum values */
    UINT32  *hFCS   = &pPacket->frameHead.frameCheckSum;
    UINT32  *pFCS   = (UINT32 *)((UINT8 *)&pPacket->frameHead + pPacket->grossSize - 4);

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
 *  @param[in]      port            port on which to send
 *  @param[in]      pPacket         pointer to packet to be sent
 *  @retval         != NULL         error
 */
TRDP_ERR_T  trdp_mdSendPacket (
    INT32           pdSock,
    UINT32          port,
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
                              port);
    }

    if (err != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "vos_sockSend failed (Err: %d)\n", err);
        return TRDP_IO_ERR;
    }

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
TRDP_ERR_T  trdp_mdRecvPacket (
    TRDP_SESSION_PT appHandle,
    INT32           mdSock,
    MD_ELE_T        *pPacket)
{
    TRDP_ERR_T   err = TRDP_NO_ERR;

    UINT32 size;

    if ((pPacket->pktFlags & TRDP_FLAGS_TCP) != 0)
    {
        /* Read Header */
        size = sizeof(MD_HEADER_T);
        err = (TRDP_ERR_T) vos_sockReceiveTCP(mdSock, (UINT8 *)&pPacket->frameHead, &size);
        vos_printf(VOS_LOG_INFO, "Read Header Size = %d\n", size);

        if(err == VOS_NODATA_ERR)
        {
            vos_printf(VOS_LOG_INFO, "trdp_mdRecvPacket - The socket = %u has been closed \n",mdSock);
            return TRDP_NODATA_ERR;
        }

        if(err != VOS_NO_ERR)
        {
            vos_printf(VOS_LOG_ERROR, "trdp_mdRecvPacket failed (Reading the msg Header) = %d\n",err);
            return TRDP_IO_ERR;
        }

        /* Get the rest of the message length */
        {
            UINT32 data_size;
            data_size = vos_ntohl(pPacket->frameHead.datasetLength) + sizeof(pPacket->frameHead.frameCheckSum);

            /*Read Data + CRC */
            err = (TRDP_ERR_T) vos_sockReceiveTCP(mdSock, (UINT8 *)&pPacket->data[0], &data_size);
            vos_printf(VOS_LOG_INFO, "Read Data + CRC Size = %d\n", data_size);

            size = size + data_size;
        }
    }
    else
    {
        size = TRDP_MAX_MD_PACKET_SIZE;
        
        err = (TRDP_ERR_T) vos_sockReceiveUDP(
                mdSock,
                (UINT8 *)&pPacket->frameHead,
                &size,
                &pPacket->addr.srcIpAddr);
    }

    pPacket->grossSize = size;
    pPacket->dataSize = size - sizeof(MD_HEADER_T);

    if (err == TRDP_NODATA_ERR)
    {
        /* no data -> rx timeout */
        return TRDP_TIMEOUT_ERR;
    }

    if (err != TRDP_NO_ERR)
    {
        if(((pPacket->pktFlags & TRDP_FLAGS_TCP) == 0)//UDP
                || ((err != TRDP_BLOCK_ERR) && ((pPacket->pktFlags & TRDP_FLAGS_TCP) != 0)))
        {
            vos_printf(VOS_LOG_ERROR, "trdp_mdRecvPacket failed = %d\n", err);
            return TRDP_IO_ERR;
        }
    }

    /* received data */
    err = trdp_mdCheck(appHandle, &pPacket->frameHead, pPacket->grossSize);
    
    /*  Update statistics   */
    switch (err)
    {
        case TRDP_NO_ERR:
            if ((pPacket->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                appHandle->stats.tcpMd.numRcv++;;
            }else
            {
                appHandle->stats.udpMd.numRcv++;;
            }
            break;
        case TRDP_CRC_ERR:
            if ((pPacket->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                appHandle->stats.tcpMd.numCrcErr++;
            }else
            {
                appHandle->stats.udpMd.numCrcErr++;
            }
            return err;
        case TRDP_WIRE_ERR:
            if ((pPacket->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                appHandle->stats.tcpMd.numProtErr++;
            }else
            {
                appHandle->stats.udpMd.numProtErr++;
            }
            return err;
        default:
            return err;
    }
    
    if (err != TRDP_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "trdp_mdRecvPacket failed = %d\n", err);
        return TRDP_IO_ERR;
    }

    return err;
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
TRDP_ERR_T  trdp_mdRecv (
    TRDP_SESSION_PT appHandle,
    INT32           sock)
{
    TRDP_ERR_T  result = TRDP_NO_ERR;
    UINT8       findSock;
    UINT8       sockPosition;

    if (appHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

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
     
        appHandle->pMDRcvEle->pktFlags  = appHandle->mdDefault.flags;
    }

    /* get packet */
    result = trdp_mdRecvPacket(appHandle, sock, appHandle->pMDRcvEle);
    if (result != TRDP_NO_ERR)
    {
        return result;
    }

    /* process message */
    {
        MD_HEADER_T *pH = &appHandle->pMDRcvEle->frameHead;
        int         lF  = appHandle->pMDRcvEle->grossSize;

        if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
        {
            lF  = appHandle->pMDRcvEle->dataSize;
        }

        vos_printf(VOS_LOG_ERROR,
                   "Received MD packet (space: %d len: %d)\n",
                   appHandle->pMDRcvEle->grossSize,
                   appHandle->pMDRcvEle->dataSize);

        /* Display incoming header */
        /* TCP cornerIp */
        if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
        {
            int i;

            for(findSock=0; findSock < VOS_MAX_SOCKET_CNT; findSock++)
            {
                if((appHandle->iface[findSock].sock > -1) && (appHandle->iface[findSock].sock == sock))
                {
                    break;
                }
            }

            appHandle->pMDRcvEle->addr.srcIpAddr = appHandle->iface[findSock].tcpParams.cornerIp;

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

        {
            /* checks */
            UINT32  l_comId = vos_ntohl(pH->comId);

            /* topo counter check for context */
            UINT32  l_topoCount = vos_ntohl(pH->topoCount);

           /* message type */
            UINT16      l_msgType = vos_ntohs(pH->msgType);

            /* find for subscriber */
            MD_ELE_T    *iterMD;
                        
            /* check for topo counter */
            if (l_topoCount != 0 && appHandle->topoCount != 0 && l_topoCount != appHandle->topoCount)
            {
                vos_printf(VOS_LOG_INFO,
                           "MD data with wrong topocount ignored (comId %u, topo %u)\n",
                           l_comId,
                           l_topoCount);

                if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                {
                    appHandle->stats.tcpMd.numTopoErr++;
                }else
                {
                    appHandle->stats.udpMd.numTopoErr++;
                }

                return TRDP_TOPO_ERR;
            }
       
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

                                theMessage.srcIpAddr        = appHandle->pMDRcvEle->addr.srcIpAddr;
                                theMessage.destIpAddr       = 0;
                                theMessage.seqCount         = vos_ntohl(pH->sequenceCounter);
                                theMessage.protVersion      = vos_ntohs(pH->protocolVersion);
                                theMessage.msgType          = vos_ntohs(pH->msgType);
                                theMessage.comId            = iterMD->u.listener.comId;
                                theMessage.topoCount        = vos_ntohl(pH->topoCount);
                                theMessage.userStatus       = 0;
                                theMessage.replyStatus      = vos_ntohs(pH->replyStatus);
                                memcpy(theMessage.sessionId, pH->sessionID, 16);
                                theMessage.replyTimeout     = vos_ntohl(pH->replyTimeout);
                                memcpy(theMessage.destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);
                                memcpy(theMessage.srcURI, pH->sourceURI, TRDP_MAX_URI_USER_LEN);
                                theMessage.numExpReplies    = 0;
                                theMessage.numReplies       = 0;
                                theMessage.numRetriesMax    = 0;
                                theMessage.numRetries       = 0;
                                theMessage.disableReplyRx   = 0;
                                theMessage.numRepliesQuery  = 0;
                                theMessage.numConfirmSent   = 0;
                                theMessage.numConfirmTimeout = 0;
                                theMessage.pUserRef         = appHandle->mdDefault.pRefCon;
                                theMessage.resultCode       = TRDP_NO_ERR;

                                if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                                {
                                    appHandle->mdDefault.pfCbFunction(
                                        appHandle->mdDefault.pRefCon,
                                        &theMessage,
                                        (UINT8 *)&(appHandle->pMDRcvEle->data[0]),lF);
                                }
                                else
                                {
                                    appHandle->mdDefault.pfCbFunction(
                                        appHandle->mdDefault.pRefCon,
                                        &theMessage,
                                        (UINT8 *)pH, lF);
                                }
                            }

                            /* TCP and Notify message */
                            if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                            {
                                if(TRDP_MSG_MR == l_msgType)
                                {
                                    for(findSock = 0; findSock < VOS_MAX_SOCKET_CNT; findSock++)
                                    {
                                        if((appHandle->iface[findSock].sock == sock)
                                            && (appHandle->iface[findSock].sock != -1)
                                            && (appHandle->iface[findSock].rcvOnly == 1))
                                        {
                                            sockPosition = findSock;
                                            break;
                                        }
                                    }

                                    appHandle->iface[sockPosition].usage++;
                                    vos_printf(VOS_LOG_INFO, "Socket (Num = %d) usage incremented to (Num = %d)\n",
                                        appHandle->iface[sockPosition].sock, appHandle->iface[sockPosition].usage);

                                    /* Save the socket position in the listener */
                                    iterMD->socketIdx = sockPosition;
                                    vos_printf(VOS_LOG_INFO, "SocketIndex (Num = %d) saved in the Listener\n",
                                        iterMD->socketIdx);
                                }
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

                                /* Found */
                                break;
                            }

                            /* Found */
                            if(sender_ele != NULL)
                            {
                                /**--**/
                                /* the sender sent request and is waiting for reply */
                                if (sender_ele->stateEle == TRDP_MD_ELE_ST_TX_REQUEST_W4Y)
                                {
                                    /* reply ok or reply error */
                                    if (l_msgType == TRDP_MSG_MP || l_msgType == TRDP_MSG_MQ || l_msgType == TRDP_MSG_ME)
                                    {
                                        /* Discard all MD Reply/ReplyQuery received after ReplyTimeout or expected replies received */
                                        if(sender_ele->disableReplyRx != 0)
                                        {
                                            break;
                                        }

                                        /* Info */
                                        vos_printf(VOS_LOG_INFO,
                                                    "MD RX/TX match (comId %u, topo %u)\n",
                                                    l_comId,
                                                    l_topoCount);

                                        /* Increment number of received replies */
                                        sender_ele->numReplies++;

                                        /* If received reply with confirm request */
                                        if (l_msgType == TRDP_MSG_MQ)
                                        {
                                            /* Increment number of ReplyQuery received, used to count nuomber of expected Confirm sent */
                                            sender_ele->numRepliesQuery++;
                                        
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

                                        /* Handle multiple replies */
                                        if(sender_ele->noOfRepliers == 1)
                                        {
                                            /* Disable Reply/ReplyQuery reception, only one expected */
                                            sender_ele->disableReplyRx = 1;
                                        }
                                        else if(sender_ele->noOfRepliers > 1)
                                        {
                                            /* Handle multiple known expected replies */
                                            if(sender_ele->noOfRepliers == sender_ele->numReplies)
                                            {
                                                /* Disable Reply/ReplyQuery reception because all expected are received */
                                                sender_ele->disableReplyRx = 1;
                                            }
                                        }
                                        else
                                        {
                                            /* Handle multiple unknown replies */
                                            /* Send element removed by timeout because number of repliers is unknown */
                                        }

                                        /* copy message to proper listener */
                                        memcpy(&iterMD->frameHead, pH, lF);

                                        if (appHandle->mdDefault.pfCbFunction != NULL)
                                        {
                                            TRDP_MD_INFO_T theMessage;

                                            theMessage.srcIpAddr        = appHandle->pMDRcvEle->addr.srcIpAddr;
                                            theMessage.destIpAddr       = 0;
                                            theMessage.seqCount         = vos_ntohl(pH->sequenceCounter);
                                            theMessage.protVersion      = vos_ntohs(pH->protocolVersion);
                                            theMessage.msgType          = vos_ntohs(pH->msgType);
                                            theMessage.comId            = iterMD->u.listener.comId;
                                            theMessage.topoCount        = vos_ntohl(pH->topoCount);
                                            theMessage.userStatus       = 0;
                                            theMessage.replyStatus      = vos_ntohs(pH->replyStatus);
                                            memcpy(theMessage.sessionId, pH->sessionID, 16);
                                            theMessage.replyTimeout     = vos_ntohl(pH->replyTimeout);
                                            memcpy(theMessage.destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);
                                            memcpy(theMessage.srcURI, pH->sourceURI, TRDP_MAX_URI_USER_LEN);
                                            theMessage.numExpReplies    = sender_ele->noOfRepliers;
                                            theMessage.numReplies       = sender_ele->numReplies;
                                            theMessage.numRetriesMax    = sender_ele->numRetriesMax;
                                            theMessage.numRetries       = sender_ele->numRetries;
                                            theMessage.disableReplyRx   = sender_ele->disableReplyRx;
                                            theMessage.numRepliesQuery  = sender_ele->numRepliesQuery;
                                            theMessage.numConfirmSent   = sender_ele->numConfirmSent;
                                            theMessage.numConfirmTimeout = sender_ele->numConfirmTimeout;
                                            theMessage.pUserRef         = appHandle->mdDefault.pRefCon;
                                            theMessage.resultCode       = TRDP_NO_ERR;

                                            if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                                            {
                                                appHandle->mdDefault.pfCbFunction(
                                                    appHandle->mdDefault.pRefCon,
                                                    &theMessage,
                                                    (UINT8 *)&(appHandle->pMDRcvEle->data[0]),lF);
                                            }
                                            else
                                            {
                                                appHandle->mdDefault.pfCbFunction(
                                                    appHandle->mdDefault.pRefCon,
                                                    &theMessage,
                                                    (UINT8 *)pH, lF);
                                            }
                                        }

                                        /* TCP and Reply/ReplyError message */
                                        if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                                        {
                                            for(findSock = 0; findSock < VOS_MAX_SOCKET_CNT; findSock++)
                                            {
                                                if((appHandle->iface[findSock].sock == sock)
                                                    && (appHandle->iface[findSock].sock != -1)
                                                    && (appHandle->iface[findSock].rcvOnly == FALSE))
                                                {
                                                    sockPosition = findSock;
                                                    break;
                                                }
                                            }

                                            iterMD->socketIdx = sockPosition;

                                            if((TRDP_MSG_MP == l_msgType) || (TRDP_MSG_ME == l_msgType))
                                            {
                                                appHandle->iface[sockPosition].usage--;
                                                vos_printf(VOS_LOG_INFO, "Socket (Num = %d) usage decremented to (Num = %d)\n",
                                                    appHandle->iface[sockPosition].sock, appHandle->iface[sockPosition].usage);

                                                /* If there is no at least one session using the socket, start the socket connectionTimeout */
                                                if((appHandle->iface[sockPosition].usage == 0) && (appHandle->iface[sockPosition].rcvOnly == FALSE))
                                                {
                                                    TRDP_TIME_T tmpt_interval, tmpt_now;

                                                    vos_printf(VOS_LOG_INFO, "The Socket (Num = %d usage=0) ConnectionTimeout will be started\n",
                                                        appHandle->iface[sockPosition].sock);
                                                    
                                                    /* Start the socket connectionTimeout */
                                                    tmpt_interval.tv_sec = appHandle->mdDefault.connectTimeout / 1000000;
                                                    tmpt_interval.tv_usec = appHandle->mdDefault.connectTimeout % 1000000;

                                                    vos_getTime(&tmpt_now);
                                                    vos_addTime(&tmpt_now, &tmpt_interval);

                                                    memcpy(&appHandle->iface[sockPosition].tcpParams.connectionTimeout, &tmpt_now, sizeof(TRDP_TIME_T));
                                                }
                                            }

                                            if (TRDP_MSG_MQ == l_msgType)
                                            {
                                                /* Save the socket position in the listener */

                                                iterMD->socketIdx = sockPosition;
                                                vos_printf(VOS_LOG_INFO, "SocketIndex (Num = %d) saved in the Listener\n",
                                                    iterMD->socketIdx);
                                            }
                                        }

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

                                            theMessage.srcIpAddr        = appHandle->pMDRcvEle->addr.srcIpAddr;
                                            theMessage.destIpAddr       = 0;
                                            theMessage.seqCount         = vos_ntohl(pH->sequenceCounter);
                                            theMessage.protVersion      = vos_ntohs(pH->protocolVersion);
                                            theMessage.msgType          = vos_ntohs(pH->msgType);
                                            theMessage.comId            = iterMD->u.listener.comId;
                                            theMessage.topoCount        = vos_ntohl(pH->topoCount);
                                            theMessage.userStatus       = 0;
                                            theMessage.replyStatus      = vos_ntohs(pH->replyStatus);
                                            memcpy(theMessage.sessionId, pH->sessionID, 16);
                                            theMessage.replyTimeout     = vos_ntohl(pH->replyTimeout);
                                            memcpy(theMessage.destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);
                                            memcpy(theMessage.srcURI, pH->sourceURI, TRDP_MAX_URI_USER_LEN);
                                            theMessage.numExpReplies    = 0;
                                            theMessage.numReplies       = 0;
                                            theMessage.numRetriesMax    = 0;
                                            theMessage.numRetries       = 0;
                                            theMessage.disableReplyRx   = 0;
                                            theMessage.numRepliesQuery  = 0;
                                            theMessage.numConfirmSent   = 0;
                                            theMessage.numConfirmTimeout = 0;
                                            theMessage.pUserRef         = appHandle->mdDefault.pRefCon;
                                            theMessage.resultCode       = TRDP_NO_ERR;

                                            if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                                            {
                                                appHandle->mdDefault.pfCbFunction(
                                                    appHandle->mdDefault.pRefCon,
                                                    &theMessage,
                                                    (UINT8 *)&(appHandle->pMDRcvEle->data[0]),lF);
                                            }
                                            else
                                            {
                                                appHandle->mdDefault.pfCbFunction(
                                                    appHandle->mdDefault.pRefCon,
                                                    &theMessage,
                                                    (UINT8 *)pH, lF);
                                            }
                                        }

                                        /* TCP and Confirm message */
                                        if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                                        {
                                            for(findSock = 0; findSock < VOS_MAX_SOCKET_CNT; findSock++)
                                            {
                                                if((appHandle->iface[findSock].sock == sock)
                                                    && (appHandle->iface[findSock].sock != -1)
                                                    && (appHandle->iface[findSock].rcvOnly == 1))
                                                {
                                                    sockPosition = findSock;
                                                    break;
                                                }
                                            }

                                            iterMD->socketIdx = sockPosition;

                                            appHandle->iface[sockPosition].usage--;
                                            vos_printf(VOS_LOG_INFO, "Socket (Num = %d) usage decremented to (Num = %d)\n",
                                                appHandle->iface[sockPosition].sock, appHandle->iface[sockPosition].usage);
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
                    }
                    break;
                }
            }
           
            /* Statistics */
            if(iterMD == NULL)
            {
                if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                {
                    appHandle->stats.tcpMd.numNoListener++;
                }
                else
                {
                    appHandle->stats.udpMd.numNoListener++;
                }
            }
        }
    }

    return result;
}


/******************************************************************************/
/** Sending MD messages
 *  Send the messages stored in the sendQueue
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 */
TRDP_ERR_T  trdp_mdSend (
    TRDP_SESSION_PT appHandle)
{
    TRDP_ERR_T  result  = TRDP_NO_ERR;
    MD_ELE_T    *iterMD = NULL;

    /*    Find the packet which has to be sent next:    */
    for (iterMD = appHandle->pMDSndQueue; iterMD != NULL; )
    {
        int restart = 0;
        int dotx    = 0;
        TRDP_MD_ELE_ST_T nextstate = TRDP_MD_ELE_ST_NONE;
        switch(iterMD->stateEle)
        {
            case TRDP_MD_ELE_ST_TX_NOTIFY_ARM: dotx = 1;
                break;
            case TRDP_MD_ELE_ST_TX_REQUEST_ARM: dotx = 1;
                nextstate = TRDP_MD_ELE_ST_TX_REQUEST_W4Y;
                break;
            case TRDP_MD_ELE_ST_TX_REPLY_ARM: dotx = 1;
                break;
            case TRDP_MD_ELE_ST_TX_REPLYQUERY_ARM: dotx = 1;
                nextstate = TRDP_MD_ELE_ST_TX_REPLYQUERY_W4C;
                break;
            case TRDP_MD_ELE_ST_TX_CONFIRM_ARM: dotx = 1;
                break;
            case TRDP_MD_ELE_ST_TX_ERROR_ARM: dotx = 1;
                break;
            default: break;
        }
        if (dotx)
        {
            /*    Send the packet if it is not redundant    */
            if (iterMD->socketIdx != -1 && (!appHandle->beQuiet || (iterMD->privFlags & TRDP_REDUNDANT)))
            {
                trdp_mdUpdatePacket(iterMD);

                if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                {
                    if(iterMD->connectDone == FALSE)
                    {
                        /* Connect() the socket */
                        if (vos_sockConnect(appHandle->iface[iterMD->socketIdx].sock,
                                            iterMD->addr.destIpAddr, appHandle->mdDefault.tcpPort) != VOS_NO_ERR)
                        {
                            vos_printf(VOS_LOG_ERROR, "Socket connection for TCP failed!\n");
                            continue;
                        }
                        else
                        {
                            iterMD->connectDone = TRUE;
                        }
                    }
                }

                result = trdp_mdSendPacket(appHandle->iface[iterMD->socketIdx].sock, appHandle->mdDefault.udpPort, iterMD);

                if (result == TRDP_NO_ERR)
                {
                    appHandle->stats.udpMd.numSend++;

                    if((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                    {
                        if(nextstate == TRDP_MD_ELE_ST_TX_REQUEST_W4Y)
                        {
                            /* Add the socket in the master_set to receive reply */
                            FD_SET(appHandle->iface[iterMD->socketIdx].sock, &appHandle->mdDefault.tcpFd.master_set);

                            if(appHandle->iface[iterMD->socketIdx].sock > (appHandle->mdDefault.tcpFd.max_sd - 1))
                            {
                                appHandle->mdDefault.tcpFd.max_sd = appHandle->iface[iterMD->socketIdx].sock + 1;
                            }
                        }
                    }
                }

                if (nextstate == TRDP_MD_ELE_ST_NONE)
                {
                    /* Notify, Reply, Reply Error or Confirm messsage */

                    if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                    {
                        appHandle->iface[iterMD->socketIdx].usage--;

                        vos_printf(VOS_LOG_INFO,
                                   "Socket (Num = %d) usage decremented to (Num = %d)\n",
                                   appHandle->iface[iterMD->socketIdx].sock,
                                   appHandle->iface[iterMD->socketIdx].usage);

                        /* If there is no at least one session using the socket, start the socket connectionTimeout
                          */
                        if((appHandle->iface[iterMD->socketIdx].usage == 0)
                           && (appHandle->iface[iterMD->socketIdx].rcvOnly == FALSE)
                           && ((iterMD->stateEle == TRDP_MD_ELE_ST_TX_NOTIFY_ARM)
                               || (iterMD->stateEle == TRDP_MD_ELE_ST_TX_CONFIRM_ARM)))
                        {
                            vos_printf(VOS_LOG_INFO,
                                       "The Socket (Num = %d usage=0) ConnectionTimeout will be started\n",
                                       appHandle->iface[iterMD->socketIdx].sock);

                            /* Start the socket connectionTimeout */
                            {
                                TRDP_TIME_T tmpt_interval, tmpt_now;

                                tmpt_interval.tv_sec    = appHandle->mdDefault.connectTimeout / 1000000;
                                tmpt_interval.tv_usec   = appHandle->mdDefault.connectTimeout % 1000000;

                                vos_getTime(&tmpt_now);
                                vos_addTime(&tmpt_now, &tmpt_interval);

                                memcpy(&appHandle->iface[iterMD->socketIdx].tcpParams.connectionTimeout,
                                       &tmpt_now,
                                       sizeof(TRDP_TIME_T));
                            }
                        }
                    }

                    /* Remove element from queue */
                    trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD);

                    /* free element */
                    vos_memFree(iterMD);

                    restart = 1;
                }
                else
                {
                    iterMD->stateEle = nextstate;
                }
            }
        }

        iterMD = restart ? appHandle->pMDSndQueue : iterMD->pNext;
    }
    return result;
}


/******************************************************************************/
/** Checking receive connection requests and data
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      pRfds               pointer to set of ready descriptors
 *  @param[in,out]  pCount              pointer to number of ready descriptors
 */
void  trdp_mdCheckListenSocks (
    TRDP_SESSION_PT appHandle,
    TRDP_FDS_T          *pRfds,
    INT32               *pCount)
{
    if (appHandle == NULL)
    {
        return;
    }

    if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
    {
        INT32       new_sd;
        MD_ELE_T    *iterMD = NULL;

        /*    Check the socket for received MD packets    */

        /*    Check the input params    */
        if (pRfds == NULL || pCount == NULL)
        {
            /* polling mode */
        }
        else if (pCount != NULL && *pCount > 0)
        {
            /*    Check the sockets for received MD packets    */

            /**********************************************************/
            /* One or more descriptors are readable.  Need to         */
            /* determine which ones they are.                         */
            /**********************************************************/

            /****************************************************/
            /* Check to see if this is the TCP listening socket */
            /****************************************************/

            vos_printf(VOS_LOG_INFO, "\n----- CHECKING READY DESCRIPTORS -----\n");

            if (FD_ISSET(appHandle->mdDefault.tcpFd.listen_sd, (fd_set *)pRfds))
            {
                /****************************************************/
                /* A TCP connection request in the listen socket.   */
                /****************************************************/
                (*pCount)--;

                /*************************************************/
                /* Accept all incoming connections that are      */
                /* queued up on the listening socket.              */
                /*************************************************/
                do
                {
                    /**********************************************/
                    /* Accept each incoming connection.           */
                    /* Check any failure on accept                */
                    /**********************************************/
                    TRDP_IP_ADDR_T  newIp;
                    UINT16          read_tcpPort;
                    TRDP_ERR_T      err;

                    newIp           = appHandle->realIP;
                    read_tcpPort    = appHandle->mdDefault.tcpPort;

                    err = (TRDP_ERR_T)vos_sockAccept(appHandle->mdDefault.tcpFd.listen_sd, &new_sd, &newIp, &(read_tcpPort));
                    
                    if (new_sd < 0)
                    {
                        if(err == VOS_NO_ERR)
                        {
                            vos_printf(VOS_LOG_INFO, "TRDP no more connections to accept\n");
                            break;
                        }
                        else
                        {
                            vos_printf(VOS_LOG_ERROR, "TRDP vos_sockAccept() failed (Err:%d)\n", err);

                            /* Callback the error to the application  */
                            if (appHandle->mdDefault.pfCbFunction != NULL)
                            {
                                TRDP_MD_INFO_T theMessage;

                                if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                                {
                                    theMessage.topoCount    = appHandle->topoCount;
                                    theMessage.resultCode   = TRDP_SOCK_ERR;
                                    theMessage.srcIpAddr    = newIp;
                                    appHandle->mdDefault.pfCbFunction(appHandle->mdDefault.pRefCon,
                                                                      &theMessage, NULL, 0);
                                }
                            }
                            continue;
                        }
                    }
                    else
                    {
                        vos_printf(VOS_LOG_INFO, "Get socket: %d\n", new_sd);
                    }

                    {
                        VOS_SOCK_OPT_T trdp_sock_opt;

                        trdp_sock_opt.qos   = appHandle->mdDefault.sendParam.qos;
                        trdp_sock_opt.ttl   = appHandle->mdDefault.sendParam.ttl;
                        trdp_sock_opt.ttl_multicast = 0;
                        trdp_sock_opt.reuseAddrPort = TRUE;
                        trdp_sock_opt.nonBlocking   = TRUE;

                        err = (TRDP_ERR_T) vos_sockSetOptions(new_sd, &trdp_sock_opt);
                        if (err != VOS_NO_ERR)
                        {
                            vos_printf(VOS_LOG_ERROR, "vos_sockSetOptions() failed (Err:%d)\n", err);
                            continue;
                        }
                    }

                    /* There is one more socket to manage */

                    /*Add the socket in the master_set */
                    FD_SET(new_sd, &appHandle->mdDefault.tcpFd.master_set);

                    if(new_sd > (appHandle->mdDefault.tcpFd.max_sd - 1))
                    {
                        appHandle->mdDefault.tcpFd.max_sd = new_sd + 1;
                    }

                    /* Compare with the sockets stored in the socket list */
                    {
                        INT32   socket_index;
                        BOOL    socket_found = FALSE;

                        for (socket_index = 0; socket_index < VOS_MAX_SOCKET_CNT; socket_index++)
                        {
                            if((appHandle->iface[socket_index].sock != -1)
                               && (appHandle->iface[socket_index].type == TRDP_SOCK_MD_TCP)
                               && (appHandle->iface[socket_index].tcpParams.cornerIp == newIp)
                               && (appHandle->iface[socket_index].rcvOnly == TRUE))
                            {
                                vos_printf(VOS_LOG_INFO, "New socket accepted from the same device (Ip = %u)\n", newIp);

                                /* Delete the items from SndQueue that are using the old socket */
                                for (iterMD = appHandle->pMDSndQueue; iterMD != NULL; iterMD = iterMD->pNext)
                                {
                                    if(iterMD->socketIdx == socket_index)
                                    {
                                        /* Remove element from queue */
                                        trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD);

                                        /* free element */
                                        vos_memFree(iterMD);
                                    }
                                }

                                /* Clean the session in the listener */
                                for(iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
                                {
                                    if(iterMD->socketIdx == socket_index)
                                    {
                                        iterMD->stateEle    = TRDP_MD_ELE_ST_RX_ARM;
                                        iterMD->socketIdx   = -1;
                                    }
                                }

                                /* Close the old socket */
                                err = (TRDP_ERR_T) vos_sockClose(appHandle->iface[socket_index].sock);
                                if (err != VOS_NO_ERR)
                                {
                                    vos_printf(VOS_LOG_ERROR, "vos_sockClose() failed (Err:%d)\n", err);
                                }
                                
                                vos_printf(VOS_LOG_INFO,
                                           "The old socket (Socket = %d, Ip = %u) will be closed\n",
                                           appHandle->iface[socket_index].sock,
                                           appHandle->iface[socket_index].tcpParams.cornerIp);


                                if(FD_ISSET(appHandle->iface[socket_index].sock, (fd_set *) pRfds))
                                {
                                    /* Decrement the Ready descriptors counter */
                                    (*pCount)--;
                                    FD_CLR(appHandle->iface[socket_index].sock, (fd_set *) pRfds);
                                }

                                /* Clear from the master_set */
                                FD_CLR(appHandle->iface[socket_index].sock, &appHandle->mdDefault.tcpFd.master_set);

                                if(appHandle->iface[socket_index].sock == (appHandle->mdDefault.tcpFd.max_sd - 1))
                                {
                                    for(; FD_ISSET((appHandle->mdDefault.tcpFd.max_sd - 1), &appHandle->mdDefault.tcpFd.master_set) == FALSE; appHandle->mdDefault.tcpFd.max_sd -= 1)
                                    {
                                        ;
                                    }
                                }

                                /* Replace the old socket by the new one */
                                appHandle->iface[socket_index].sock     = new_sd;
                                appHandle->iface[socket_index].rcvOnly  = TRUE;
                                appHandle->iface[socket_index].tcpParams.connectionTimeout.tv_sec   = 0;
                                appHandle->iface[socket_index].tcpParams.connectionTimeout.tv_usec  = 0;
                                appHandle->iface[socket_index].tcpParams.cornerIp = newIp;
                                appHandle->iface[socket_index].type     = TRDP_SOCK_MD_TCP;
                                appHandle->iface[socket_index].usage    = 0;

                                socket_found = TRUE;
                                break;
                            }
                        }

                        if(socket_found == FALSE)
                        {
                            /* Save the new socket in the iface */
                            err = trdp_requestSocket(
                                    appHandle->iface,
                                    appHandle->mdDefault.tcpPort,
                                    &appHandle->mdDefault.sendParam,
                                    appHandle->realIP,
                                    TRDP_SOCK_MD_TCP,
                                    TRDP_OPTION_NONE,
                                    TRUE,
                                    &new_sd,
                                    newIp);

                            if(err != TRDP_NO_ERR)
                            {
                                vos_printf(VOS_LOG_ERROR, "trdp_requestSocket() failed (Err: %d)\n", err);
                            }
                        }
                    }

                    /**********************************************/
                    /* Loop back up and accept another incoming   */
                    /* connection                                 */
                    /**********************************************/
                }
                while (new_sd != -1);
            }

            /* Check Receive Data */
            {
                INT32       index;
                MD_ELE_T    *iterMD_find;

                for (index = 0; index < VOS_MAX_SOCKET_CNT; index++)
                {
                    if((appHandle->iface[index].sock != -1) && (appHandle->iface[index].type == TRDP_SOCK_MD_TCP))
                    {
                        if(FD_ISSET(appHandle->iface[index].sock, (fd_set *) pRfds))
                        {
                            TRDP_ERR_T err;

                            (*pCount)--;
                            FD_CLR(appHandle->iface[index].sock, (fd_set *)pRfds);

                            err = trdp_mdRecv(appHandle, appHandle->iface[index].sock);

                            if (err != TRDP_NO_ERR )
                            {
                                /* Check if the socket has been closed in the other corner */
                                if(err == TRDP_NODATA_ERR)
                                {
                                    /* Close the socket */
                                    FD_CLR(appHandle->iface[index].sock, &appHandle->mdDefault.tcpFd.master_set);

                                    if(appHandle->iface[index].sock == (appHandle->mdDefault.tcpFd.max_sd - 1))
                                    {
                                        for(; FD_ISSET((appHandle->mdDefault.tcpFd.max_sd - 1), &appHandle->mdDefault.tcpFd.master_set) == FALSE; appHandle->mdDefault.tcpFd.max_sd -= 1)
                                        {
                                            ;
                                        }
                                    }

                                    vos_sockClose(appHandle->iface[index].sock);

                                    /* Delete the socket from the iface */
                                    vos_printf(VOS_LOG_INFO,
                                               "Deleting socket (Nº = %d) from the iface\n",
                                               appHandle->iface[index].sock);
                                    vos_printf(VOS_LOG_INFO, "Close socket iface index=%d\n", index);
                                    appHandle->iface[index].sock = -1;
                                    appHandle->iface[index].sendParam.qos   = 0;
                                    appHandle->iface[index].sendParam.ttl   = 0;
                                    appHandle->iface[index].usage           = 0;
                                    appHandle->iface [index].bindAddr       = 0;
                                    appHandle->iface[index].type    = (TRDP_SOCK_TYPE_T) 0;
                                    appHandle->iface[index].rcvOnly = FALSE;
                                    appHandle->iface[index].tcpParams.cornerIp = 0;
                                    appHandle->iface[index].tcpParams.connectionTimeout.tv_sec  = 0;
                                    appHandle->iface[index].tcpParams.connectionTimeout.tv_usec = 0;

                                    /* Delete the sessions that are opened in this socket */
                                    for(iterMD_find = appHandle->pMDSndQueue;
                                        iterMD_find != NULL;
                                        iterMD_find = iterMD_find->pNext)
                                    {
                                        if(iterMD_find->socketIdx == index)
                                        {
                                            /*This session is using the socket */
                                            /* Remove element from queue */
                                            trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD_find);

                                            /* free element */
                                            vos_memFree(iterMD_find);
                                        }
                                    }

                                    /* Clean the session in the listener */
                                    for(iterMD_find = appHandle->pMDRcvQueue;
                                        iterMD_find != NULL;
                                        iterMD_find = iterMD_find->pNext)
                                    {
                                        if(iterMD_find->socketIdx == index)
                                        {
                                            iterMD_find->stateEle   = TRDP_MD_ELE_ST_RX_ARM;
                                            iterMD_find->socketIdx  = -1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        /* UDP */
        if (pRfds == NULL || pCount == NULL)
        {
            int     i;
            /* polling mode */

            /* flags for polling sockets */
            UINT8   skxp[VOS_MAX_SOCKET_CNT];

            /* initial: FALSE */
            for(i = 0; i < VOS_MAX_SOCKET_CNT; i++)
            {
                skxp[i] = 0;
            }

            /* search for listener sockets */
            {
                MD_ELE_T *iterMD;

                for(iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
                {
                    /* valid socket for armed listener */
                    if (appHandle->iface[iterMD->socketIdx].sock != -1)
                    {
                        /* index to poll */
                        skxp[iterMD->socketIdx] = 1;
                    }
                }
            }

            /* scan for sockets */
            for(i = 0; i < VOS_MAX_SOCKET_CNT; i++)
            {
                if (skxp[i])
                {
                    TRDP_ERR_T err;

                    err = trdp_mdRecv(appHandle, appHandle->iface[i].sock);
                    if (err != TRDP_NO_ERR)
                    {
                        vos_printf(VOS_LOG_ERROR, "trdp_mdRecv() failed (Err:%d)\n", err);
                    }
                }
            }
        }
        else
        {
            /* select() mode */
        }
    }
}


/******************************************************************************/
/** Checking message data timeouts
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 */
void  trdp_mdCheckTimeouts (
    TRDP_SESSION_PT appHandle)
{
    MD_ELE_T    *iterMD;
    TRDP_TIME_T now;
 
    if (appHandle == NULL)
    {
        return;
    }

    /* Update the current time    */
    vos_getTime(&now);

    /* timeout on receive queue */
    for(iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
    {
        switch(iterMD->stateEle)
        {
            /* Request waiting for reply */
            case TRDP_MD_ELE_ST_RX_REQ_W4AP_REPLY:

            /* Reply waiting for a Confirm */
            case TRDP_MD_ELE_ST_RX_REPLY_W4AP_CONF:
            {
                VOS_TIME_T tmpt = iterMD->timeToGo; /* start time */
                vos_addTime(&tmpt, &iterMD->interval); /* interval is timeout velue */

                if (0 > vos_cmpTime(&tmpt, &now)) /* timeout overflow */
                {
                    MD_ELE_T *sender_ele = NULL;

                    vos_printf(VOS_LOG_INFO, "MD listener timeout: fall back ARM st=%d\n", iterMD->stateEle);

                    if(iterMD->stateEle == TRDP_MD_ELE_ST_RX_REPLY_W4AP_CONF)
                    {
                        /* Look for Request sender */
                        for (sender_ele = appHandle->pMDSndQueue;
                             sender_ele != NULL;
                             sender_ele = sender_ele->pNext)
                        {
                            /* check for session ... */
                            if (0 != memcmp(&sender_ele->sessionID, iterMD->sessionID, 16))
                            {
                                continue;
                            }

                            break;
                        }

                        /* Found if sender_ele is not NULL */
                        if(sender_ele != NULL)
                        {
                            /* Increment number of Confirm Timeouts */
                            sender_ele->numConfirmTimeout++;
                        }
                        else
                        {
                            /* Sender does not more exist, nothing to do, give infomrmation to user */
                            vos_printf(VOS_LOG_INFO, "MD listener timeout: no sender found.\n");
                        }
                    }

                    /* Execute callback */
                    if(appHandle->mdDefault.pfCbFunction != NULL)
                    {
                        TRDP_MD_INFO_T theMessage;

                        theMessage.srcIpAddr     = 0;
                        theMessage.destIpAddr    = iterMD->u.listener.destIpAddr;
                        theMessage.seqCount      = vos_ntohs(iterMD->frameHead.sequenceCounter);
                        theMessage.protVersion   = vos_ntohs(iterMD->frameHead.protocolVersion);
                        theMessage.msgType       = (TRDP_MSG_T) vos_ntohs(iterMD->frameHead.msgType);
                        theMessage.comId         = iterMD->u.listener.comId;
                        theMessage.topoCount     = iterMD->u.listener.topoCount;
                        theMessage.userStatus    = 0;
                        theMessage.replyStatus   = (TRDP_REPLY_STATUS_T) vos_ntohs(iterMD->frameHead.replyStatus);
                        memcpy(theMessage.sessionId, iterMD->frameHead.sessionID, 16);
                        theMessage.replyTimeout  = vos_ntohl(iterMD->frameHead.replyTimeout);
                        memcpy(theMessage.destURI, iterMD->u.listener.destURI, 32);
                        memcpy(theMessage.srcURI, iterMD->frameHead.sourceURI, 32);
                        theMessage.numExpReplies = iterMD->noOfRepliers;

                        theMessage.numReplies           = 0;
                        theMessage.numRetriesMax        = 0;
                        theMessage.numRetries           = 0;
                        theMessage.disableReplyRx       = 0;
                        theMessage.numRepliesQuery      = 0;
                        theMessage.numConfirmSent       = 0;
                        theMessage.numConfirmTimeout    = 0;

                        /* Fill information in case of sender found */
                        if(sender_ele != NULL)
                        {
                            theMessage.numReplies           = sender_ele->numReplies;
                            theMessage.numRetriesMax        = sender_ele->numRetriesMax;
                            theMessage.numRetries           = sender_ele->numRetries;
                            theMessage.disableReplyRx       = sender_ele->disableReplyRx;
                            theMessage.numRepliesQuery      = sender_ele->numRepliesQuery;
                            theMessage.numConfirmSent       = sender_ele->numConfirmSent;
                            theMessage.numConfirmTimeout    = sender_ele->numConfirmTimeout;
                        }

                        theMessage.pUserRef = appHandle->mdDefault.pRefCon;

                        if(iterMD->stateEle == TRDP_MD_ELE_ST_RX_REQ_W4AP_REPLY)
                        {
                            theMessage.resultCode = TRDP_APP_REPLYTO_ERR;
                        }
                        else if(iterMD->stateEle == TRDP_MD_ELE_ST_RX_REPLY_W4AP_CONF)
                        {
                            theMessage.resultCode = TRDP_APP_CONFIRMTO_ERR;
                        }
                        else
                        {
                            theMessage.resultCode = TRDP_UNKNOWN_ERR;
                        }

                        appHandle->mdDefault.pfCbFunction(
                            appHandle->mdDefault.pRefCon,
                            &theMessage,
                            (UINT8 *)0, 0);
                    }

                    /* Reset listener */
                    iterMD->stateEle = TRDP_MD_ELE_ST_RX_ARM;

                    /* TCP handling */
                    if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                    {
                        MD_ELE_T *iterMD_find;

                        /* Delete the sessions that are opened in this socket */
                        for(iterMD_find = appHandle->pMDSndQueue;
                            iterMD_find != NULL;
                            iterMD_find = iterMD_find->pNext)
                        {
                            if (vos_ntohl(iterMD_find->frameHead.comId) == iterMD->u.listener.comId)
                            {
                                /*This session is using the socket */
                                /* Remove element from queue */
                                trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD_find);

                                /* free element */
                                vos_memFree(iterMD_find);
                            }
                        }

                        appHandle->iface[iterMD->socketIdx].usage--;

                        vos_printf(VOS_LOG_INFO,
                                   "Socket (Num = %d) usage decremented to (Num = %d)\n",
                                   appHandle->iface[iterMD->socketIdx].sock,
                                   appHandle->iface[iterMD->socketIdx].usage);

                        /* If there is no at least one session using the socket, start the socket connectionTimeout
                          */
                        if((appHandle->iface[iterMD->socketIdx].usage == 0)
                           && (appHandle->iface[iterMD->socketIdx].rcvOnly == FALSE))
                        {
                            TRDP_TIME_T tmpt_interval, tmpt_now;

                            vos_printf(VOS_LOG_INFO,
                                       "The Socket (Num = %d usage=0) ConnectionTimeout will be started\n",
                                       appHandle->iface[iterMD->socketIdx].sock);

                            /* Start the socket connectionTimeout */
                            tmpt_interval.tv_sec    = appHandle->mdDefault.connectTimeout / 1000000;
                            tmpt_interval.tv_usec   = appHandle->mdDefault.connectTimeout % 1000000;

                            vos_getTime(&tmpt_now);
                            vos_addTime(&tmpt_now, &tmpt_interval);

                            memcpy(&appHandle->iface[iterMD->socketIdx].tcpParams.connectionTimeout,
                                   &tmpt_now,
                                   sizeof(TRDP_TIME_T));
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

    /* timeout on transmit queue */
    for(iterMD = appHandle->pMDSndQueue; iterMD != NULL; )
    {
        int restart             = 0;
        int sndReplyTimeout     = 0;
        int sndConfirmTimeout   = 0;
        int sndDone             = 0;

        switch(iterMD->stateEle)
        {
            /* Request, handle Reply/ReplyQuery reception and Confirm sent */
            case TRDP_MD_ELE_ST_TX_REQUEST_W4Y:
            {
                /* Manage Reply/ReplyQuery reception */
                /* Note: in case all expected Reply/ReplyQuery are received, disableReplyRx is set by
                  trdp_mdRecv() */
                if(iterMD->disableReplyRx == 0)
                {
                    /* Session is in reception phase */

                    /* Check for Reply timeout */
                    VOS_TIME_T tmpt = iterMD->timeToGo; /* start time */
                    vos_addTime(&tmpt, &iterMD->interval); /* interval is timeout velue */

                    if (0 > vos_cmpTime(&tmpt, &now)) /* timeout overflow */
                    {
                        vos_printf(VOS_LOG_INFO, "MD send reply timeout.\n");

                        /* Increment number of reties */
                        iterMD->numRetries++;

                        /* Handle UDP retries for single reply expeced */
                        if(
                            (iterMD->noOfRepliers == 1)                     /* Single reply expected */
                            &&
                            (iterMD->numRetriesMax > 0)                     /* Retries allowed */
                            &&
                            (iterMD->numRetries < iterMD->numRetriesMax)    /* Retries below maximum allowed */
                            )
                        {
                            /* Update timeout */
                            TRDP_TIME_T nextTime;

                            vos_getTime(&nextTime);
                            vos_addTime(&nextTime, &iterMD->interval);

                            iterMD->timeToGo = nextTime;

                            /* Re-arm send */
                            iterMD->stateEle        = TRDP_MD_ELE_ST_TX_REQUEST_ARM;
                            iterMD->disableReplyRx  = 0;

                            /* Increment counter with each telegram */
                            iterMD->frameHead.sequenceCounter =
                                vos_htonl(vos_ntohl(iterMD->frameHead.sequenceCounter) + 1);
                        }
                        else
                        {
                            /* Remove sender after callback */

                            /* Reply timeout raised, stop Reply/ReplyQuery reception */
                            iterMD->disableReplyRx = 1;
                        }

                        /* Callback execution require to indicate this event */
                        sndReplyTimeout = 1;

                        /* Statistics */

                        if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                        {
                            appHandle->stats.udpMd.numReplyTimeout++;
                        }
                        else
                        {
                            appHandle->stats.tcpMd.numReplyTimeout++;
                        }
                    }
                }

                /* vos_printf(VOS_LOG_INFO, "DEBUG tlc_process(): MD send check 1, iterMD->disableReplyRx = %d\n",
                   iterMD->disableReplyRx); */

                /* Manage send Confirm */
                if(iterMD->disableReplyRx == 1)
                {
                    /* All expected Reply/ReplyQuery received or Reply Timeout triggered */

                    /* Check Confirm sent status */
                    if((iterMD->numRepliesQuery == 0) || (iterMD->numRepliesQuery == iterMD->numConfirmSent))
                    {
                        /* All Confirm required by recived ReplyQuery are sent */
                        /* vos_printf(VOS_LOG_INFO, "DEBUG tlc_process(): MD send done, all Confirm sent\n"); */
                        sndDone = 1;
                    }
                    else
                    {
                        /* Check for pending Confirm timeout (handled in each single listener) */
                        if(iterMD->numRepliesQuery <= (iterMD->numConfirmSent + iterMD->numConfirmTimeout))
                        {
                            /* Callback execution require to indicate send done with some Confirm Timeout */
                            /* vos_printf(VOS_LOG_INFO, "DEBUG tlc_process(): MD send done, Confirm timeout\n"); */
                            sndConfirmTimeout = 1;
                            sndDone = 1;
                        }
                    }
                }

                /* Callback execution */
                if((sndReplyTimeout == 1) || (sndConfirmTimeout == 1))
                {
                    if(appHandle->mdDefault.pfCbFunction != NULL)
                    {
                        TRDP_MD_INFO_T theMessage;

                        theMessage.srcIpAddr    = 0;
                        theMessage.destIpAddr   = iterMD->addr.destIpAddr;
                        theMessage.seqCount     = vos_ntohs(iterMD->frameHead.sequenceCounter);
                        theMessage.protVersion  = vos_ntohs(iterMD->frameHead.protocolVersion);
                        theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(iterMD->frameHead.msgType);
                        theMessage.comId        = iterMD->addr.comId;
                        theMessage.topoCount    = vos_ntohs(iterMD->frameHead.topoCount);
                        theMessage.userStatus   = 0;
                        theMessage.replyStatus  = (TRDP_REPLY_STATUS_T) vos_ntohs(iterMD->frameHead.replyStatus);
                        memcpy(theMessage.sessionId, iterMD->frameHead.sessionID, 16);
                        theMessage.replyTimeout = vos_ntohl(iterMD->frameHead.replyTimeout);
                        memcpy(theMessage.destURI, iterMD->frameHead.destinationURI, 32);
                        memcpy(theMessage.srcURI, iterMD->frameHead.sourceURI, 32);
                        theMessage.numExpReplies        = iterMD->noOfRepliers;
                        theMessage.numReplies           = iterMD->numReplies;
                        theMessage.numRetriesMax        = iterMD->numRetriesMax;
                        theMessage.numRetries           = iterMD->numRetries;
                        theMessage.disableReplyRx       = iterMD->disableReplyRx;
                        theMessage.numRepliesQuery      = iterMD->numRepliesQuery;
                        theMessage.numConfirmSent       = iterMD->numConfirmSent;
                        theMessage.numConfirmTimeout    = iterMD->numConfirmTimeout;
                        theMessage.pUserRef = appHandle->mdDefault.pRefCon;

                        /* Reply Timeout Callback */
                        if(sndReplyTimeout == 1)
                        {
                            theMessage.resultCode = TRDP_REPLYTO_ERR;

                            appHandle->mdDefault.pfCbFunction(
                                appHandle->mdDefault.pRefCon,
                                &theMessage,
                                (UINT8 *)&iterMD->frameHead, iterMD->grossSize);
                        }

                        /* Confirm Timeout Callback */
                        if(sndConfirmTimeout == 1)
                        {
                            theMessage.resultCode = TRDP_REQCONFIRMTO_ERR;

                            appHandle->mdDefault.pfCbFunction(
                                appHandle->mdDefault.pRefCon,
                                &theMessage,
                                (UINT8 *)&iterMD->frameHead, iterMD->grossSize);
                        }
                    }
                }

                /* TCP handling of ReplyTimeout */
                if(sndReplyTimeout == 1)
                {
                    if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                    {
                        MD_ELE_T *iterMD_find;

                        /* Clean the session in the listener */
                        for(iterMD_find = appHandle->pMDRcvQueue;
                            iterMD_find != NULL;
                            iterMD_find = iterMD_find->pNext)
                        {
                            if (vos_ntohl(iterMD->frameHead.comId) == iterMD_find->u.listener.comId)
                            {
                                iterMD_find->stateEle   = TRDP_MD_ELE_ST_RX_ARM;
                                iterMD_find->socketIdx  = -1;
                            }
                        }

                        appHandle->iface[iterMD->socketIdx].usage--;

                        vos_printf(VOS_LOG_INFO,
                                   "Socket (Num = %d) usage decremented to (Num = %d)\n",
                                   appHandle->iface[iterMD->socketIdx].sock,
                                   appHandle->iface[iterMD->socketIdx].usage);

                        /* If there is not at least one session using the socket, start the socket connectionTimeout
                          */
                        if((appHandle->iface[iterMD->socketIdx].usage == 0)
                           && (appHandle->iface[iterMD->socketIdx].rcvOnly == FALSE))
                        {
                            TRDP_TIME_T tmpt_interval, tmpt_now;

                            vos_printf(VOS_LOG_INFO,
                                       "The Socket (Num = %d usage=0) ConnectionTimeout will be started\n",
                                       appHandle->iface[iterMD->socketIdx].sock);

                            /*  Start the socket connectionTimeout */
                            tmpt_interval.tv_sec    = appHandle->mdDefault.connectTimeout / 1000000;
                            tmpt_interval.tv_usec   = appHandle->mdDefault.connectTimeout % 1000000;

                            vos_getTime(&tmpt_now);
                            vos_addTime(&tmpt_now, &tmpt_interval);

                            memcpy(&appHandle->iface[iterMD->socketIdx].tcpParams.connectionTimeout,
                                   &tmpt_now,
                                   sizeof(TRDP_TIME_T));
                        }
                    }
                }

                /* Remove sender */
                if(sndDone == 1)
                {
                    /* Remove element from queue */
                    trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD);

                    /* free element */
                    vos_memFree(iterMD);

                    vos_printf(VOS_LOG_INFO, "MD send session done: remove st=%d\n", iterMD->stateEle);

                    /* Recheck */
                    restart = 1;
                }
            }
            break;

            /* Reply waiting for a confirmation */
            case TRDP_MD_ELE_ST_TX_REPLYQUERY_W4C:
            {
                VOS_TIME_T tmpt = iterMD->timeToGo;     /* start time */
                vos_addTime(&tmpt, &iterMD->interval);  /* interval is timeout velue */
                if (0 > vos_cmpTime(&tmpt, &now))       /* timeout overflow */
                {
                    vos_printf(VOS_LOG_INFO, "MD send session timeout: remove st=%d\n", iterMD->stateEle);


                    /* Execute callback */
                    if(appHandle->mdDefault.pfCbFunction != NULL)
                    {
                        TRDP_MD_INFO_T theMessage;

                        theMessage.srcIpAddr        = 0;
                        theMessage.destIpAddr       = iterMD->addr.destIpAddr;
                        theMessage.seqCount         = vos_ntohs(iterMD->frameHead.sequenceCounter);
                        theMessage.protVersion      = vos_ntohs(iterMD->frameHead.protocolVersion);
                        theMessage.msgType          = (TRDP_MSG_T) vos_ntohs(iterMD->frameHead.msgType);
                        theMessage.comId            = iterMD->addr.comId;
                        theMessage.topoCount        = vos_ntohs(iterMD->frameHead.topoCount);
                        theMessage.userStatus       = 0;
                        theMessage.replyStatus      = (TRDP_REPLY_STATUS_T) vos_ntohs(iterMD->frameHead.replyStatus);
                        memcpy(theMessage.sessionId, iterMD->frameHead.sessionID, 16);
                        theMessage.replyTimeout     = vos_ntohl(iterMD->frameHead.replyTimeout);
                        memcpy(theMessage.destURI, iterMD->frameHead.destinationURI, 32);
                        memcpy(theMessage.srcURI, iterMD->frameHead.sourceURI, 32);
                        theMessage.numExpReplies    = iterMD->noOfRepliers;
                        theMessage.numReplies       = iterMD->numReplies;
                        theMessage.numRetriesMax    = iterMD->numRetriesMax;
                        theMessage.numRetries       = iterMD->numRetries;
                        theMessage.pUserRef         = appHandle->mdDefault.pRefCon;
                        theMessage.resultCode       = TRDP_CONFIRMTO_ERR;

                        appHandle->mdDefault.pfCbFunction(
                            appHandle->mdDefault.pRefCon,
                            &theMessage,
                            (UINT8 *)&iterMD->frameHead, iterMD->grossSize);
                    }

                    /* TCP handling */
                    if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                    {
                        MD_ELE_T *iterMD_find;

                        /* Clean the session in the listener */
                        for(iterMD_find = appHandle->pMDRcvQueue;
                            iterMD_find != NULL;
                            iterMD_find = iterMD_find->pNext)
                        {
                            if (vos_ntohl(iterMD->frameHead.comId) == iterMD_find->u.listener.comId)
                            {
                                iterMD_find->stateEle   = TRDP_MD_ELE_ST_RX_ARM;
                                iterMD_find->socketIdx  = -1;
                            }
                        }

                        appHandle->iface[iterMD->socketIdx].usage--;

                        vos_printf(VOS_LOG_INFO,
                                   "Socket (Num = %d) usage decremented to (Num = %d)\n",
                                   appHandle->iface[iterMD->socketIdx].sock,
                                   appHandle->iface[iterMD->socketIdx].usage);

                        /* If there is not at least one session using the socket, start the socket connectionTimeout
                          */
                        if((appHandle->iface[iterMD->socketIdx].usage == 0)
                           && (appHandle->iface[iterMD->socketIdx].rcvOnly == FALSE))
                        {
                            TRDP_TIME_T tmpt_interval, tmpt_now;

                            vos_printf(VOS_LOG_INFO,
                                       "The Socket (Num = %d usage=0) ConnectionTimeout will be started\n",
                                       appHandle->iface[iterMD->socketIdx].sock);

                            /*  Start the socket connectionTimeout */
                            tmpt_interval.tv_sec    = appHandle->mdDefault.connectTimeout / 1000000;
                            tmpt_interval.tv_usec   = appHandle->mdDefault.connectTimeout % 1000000;

                            vos_getTime(&tmpt_now);
                            vos_addTime(&tmpt_now, &tmpt_interval);

                            memcpy(&appHandle->iface[iterMD->socketIdx].tcpParams.connectionTimeout,
                                   &tmpt_now,
                                   sizeof(TRDP_TIME_T));
                        }
                    }

                    /* Remove element from queue */
                    trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD);


                    /* Statistics */
                    if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
                    {
                        appHandle->stats.tcpMd.numConfirmTimeout++;
                    }
                    else
                    {
                        appHandle->stats.udpMd.numConfirmTimeout++;
                    }

                    /* free element */
                    vos_memFree(iterMD);

                    vos_printf(VOS_LOG_INFO, "MD send session timeout: remove st=%d\n", iterMD->stateEle);

                    /* Recheck */
                    restart = 1;
                }
            }
            break;

            default:
            {}
             break;
        }

        /* Handle check restart */
        iterMD = restart ? appHandle->pMDSndQueue : iterMD->pNext;
    }

    /* Check for sockets Connection Timeouts */
    if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
    {
        INT32 index;

        for (index = 0; index < VOS_MAX_SOCKET_CNT; index++)
        {
            if((appHandle->iface[index].sock != -1)
               && (appHandle->iface[index].type == TRDP_SOCK_MD_TCP)
               && (appHandle->iface[index].usage == 0)
               && (appHandle->iface[index].rcvOnly == FALSE)
               && ((appHandle->iface[index].tcpParams.connectionTimeout.tv_sec > 0)
                   || (appHandle->iface[index].tcpParams.connectionTimeout.tv_usec > 0)))
            {
                if (0 > vos_cmpTime(&appHandle->iface[index].tcpParams.connectionTimeout, &now))
                {
                    vos_printf(VOS_LOG_INFO, "The socket (Num = %d) TIMEOUT\n", appHandle->iface[index].sock);

                    /* Execute callback */
                    if(appHandle->mdDefault.pfCbFunction != NULL)
                    {
                        TRDP_MD_INFO_T theMessage;

                        theMessage.destIpAddr   = appHandle->iface[index].tcpParams.cornerIp;
                        theMessage.resultCode   = TRDP_TIMEOUT_ERR;
                        memset(theMessage.sessionId, 0, sizeof(TRDP_UUID_T));

                        appHandle->mdDefault.pfCbFunction(
                            appHandle->mdDefault.pRefCon,
                            &theMessage, NULL, 0);
                    }

                    /* Close the socket */
                    FD_CLR(appHandle->iface[index].sock, &appHandle->mdDefault.tcpFd.master_set);

                    if(appHandle->iface[index].sock == (appHandle->mdDefault.tcpFd.max_sd - 1))
                    {
                        for(; FD_ISSET((appHandle->mdDefault.tcpFd.max_sd - 1), &appHandle->mdDefault.tcpFd.master_set) == FALSE; appHandle->mdDefault.tcpFd.max_sd -= 1)
                        {
                            ;
                        }
                    }

                    vos_sockClose(appHandle->iface[index].sock);

                    /* Delete the socket from the iface */
                    vos_printf(VOS_LOG_INFO,
                               "Deleting socket (Num = %d) from the iface\n",
                               appHandle->iface[index].sock);
                    vos_printf(VOS_LOG_INFO, "Close socket iface index=%d\n", index);
                    appHandle->iface[index].sock = -1;
                    appHandle->iface[index].sendParam.qos   = 0;
                    appHandle->iface[index].sendParam.ttl   = 0;
                    appHandle->iface[index].usage           = 0;
                    appHandle->iface [index].bindAddr       = 0;
                    appHandle->iface[index].type    = (TRDP_SOCK_TYPE_T) 0;
                    appHandle->iface[index].rcvOnly = FALSE;
                    appHandle->iface[index].tcpParams.cornerIp = 0;
                    appHandle->iface[index].tcpParams.connectionTimeout.tv_sec  = 0;
                    appHandle->iface[index].tcpParams.connectionTimeout.tv_usec = 0;
                }
            }
        }
    }
}

