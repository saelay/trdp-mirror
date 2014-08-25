/**********************************************************************************************************************/
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
 *                  Gari Oiarbide, CAF
 *                  Bernd Loehr, NewTec
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
 *                     Ticket #47: Protocol change: no FCS for data part of telegrams
 *      BL 2014-02-28: Ticket #25: CRC32 calculation is not according to IEEE802.3
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_if_light.h"
#include "trdp_if.h"
#include "trdp_utils.h"
#include "trdp_mdcom.h"


/***********************************************************************************************************************
 * DEFINES
 */
#define CHECK_HEADER_ONLY   TRUE
#define CHECK_DATA_TOO      FALSE

/***********************************************************************************************************************
 * TYPEDEFS
 */


/***********************************************************************************************************************
 *   Locals
 */

static const UINT32         cMinimumMDSize = 1480;                    /**< Initial size for message data received */
static const UINT8          cEmptySession[TRDP_SESS_ID_SIZE] = {0};   /**< Empty sessionID to compare             */
static const TRDP_MD_INFO_T cTrdp_md_info_default = {0};

/**********************************************************************************************************************/
/** Initialize the specific parameters for message data
 *  Open a listening socket.
 *
 *  @param[in]      pSession              session parameters
 *
 *  @retval         TRDP_NO_ERR           no error
 *  @retval         TRDP_PARAM_ERR        initialization error
 */
TRDP_ERR_T trdp_mdGetTCPSocket (
    TRDP_SESSION_PT pSession)
{
    TRDP_ERR_T      result = TRDP_NO_ERR;
    VOS_SOCK_OPT_T  trdp_sock_opt;
    UINT32          backlog = 10; /* Backlog = maximum connection atempts if system is busy. */

    if (pSession->tcpFd.listen_sd == VOS_INVALID_SOCKET)     /* First time TCP is used */
    {
        /* Define the common TCP socket options */
        trdp_sock_opt.qos   = pSession->mdDefault.sendParam.qos;  /* (default should be 5 for PD and 3 for MD) */
        trdp_sock_opt.ttl   = pSession->mdDefault.sendParam.ttl;  /* Time to live (default should be 64) */
        trdp_sock_opt.ttl_multicast = 0;
        trdp_sock_opt.reuseAddrPort = TRUE;

        /* The socket is defined non-blocking */
        trdp_sock_opt.nonBlocking = TRUE;

        result = (TRDP_ERR_T)vos_sockOpenTCP(&pSession->tcpFd.listen_sd, &trdp_sock_opt);

        if (result != TRDP_NO_ERR)
        {
            return result;
        }

        result = (TRDP_ERR_T)vos_sockBind(pSession->tcpFd.listen_sd,
                                          pSession->realIP,
                                          pSession->mdDefault.tcpPort);

        if (result != TRDP_NO_ERR)
        {
            return result;
        }

        result = (TRDP_ERR_T)vos_sockListen(pSession->tcpFd.listen_sd, backlog);

        if (result != TRDP_NO_ERR)
        {
            return result;
        }

        vos_printLog(VOS_LOG_INFO, "TCP socket opened and listening (Sock: %d, Port: %d)\n",
                     pSession->tcpFd.listen_sd, (UINT32) pSession->mdDefault.tcpPort);

        return TRDP_NO_ERR;
    }

    return result;
}

/**********************************************************************************************************************/
/** Free memory of session
 *
 *  @param[in]      pMDSession        session pointer
 */
void trdp_mdFreeSession (
    MD_ELE_T *pMDSession)
{
    if (NULL != pMDSession)
    {
        if (NULL != pMDSession->pPacket)
        {
            vos_memFree(pMDSession->pPacket);
        }
        vos_memFree(pMDSession);
    }
}

/**********************************************************************************************************************/
/** Close and free any session marked as dead.
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      socketIndex     the old socket position in the iface[]
 *  @param[in]      newSocket       the new socket
 *  @param[in]      checkAllSockets check all the sockets that are waiting to be closed
 */
void trdp_mdCloseSessions (
    TRDP_SESSION_PT appHandle,
    INT32           socketIndex,
    INT32           newSocket,
    BOOL8           checkAllSockets)
{

    MD_ELE_T *iterMD;

    /* Check all the sockets */
    if (checkAllSockets == TRUE)
    {
        trdp_releaseSocket(appHandle->iface, TRDP_INVALID_SOCKET_INDEX, 0, checkAllSockets);

    }

    iterMD = appHandle->pMDSndQueue;

    while (NULL != iterMD)
    {
        if (TRUE == iterMD->morituri)
        {
            trdp_releaseSocket(appHandle->iface, iterMD->socketIdx, appHandle->mdDefault.connectTimeout, FALSE);
            trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD);
            vos_printLog(VOS_LOG_INFO, "Freeing %s MD caller session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                         iterMD->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                         iterMD->sessionID[0], iterMD->sessionID[1], iterMD->sessionID[2], iterMD->sessionID[3],
                         iterMD->sessionID[4], iterMD->sessionID[5], iterMD->sessionID[6], iterMD->sessionID[7])

            trdp_mdFreeSession(iterMD);
            iterMD = appHandle->pMDSndQueue;
        }
        else
        {
            iterMD = iterMD->pNext;
        }
    }

    iterMD = appHandle->pMDRcvQueue;

    while (NULL != iterMD)
    {
        if (TRUE == iterMD->morituri)
        {
            if (0 != (iterMD->pktFlags & TRDP_FLAGS_TCP))
            {
                trdp_releaseSocket(appHandle->iface, iterMD->socketIdx, appHandle->mdDefault.connectTimeout, FALSE);
            }
            trdp_MDqueueDelElement(&appHandle->pMDRcvQueue, iterMD);
            vos_printLog(VOS_LOG_INFO, "Freeing MD %s replier session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                         iterMD->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                         iterMD->sessionID[0], iterMD->sessionID[1], iterMD->sessionID[2], iterMD->sessionID[3],
                         iterMD->sessionID[4], iterMD->sessionID[5], iterMD->sessionID[6], iterMD->sessionID[7])
            trdp_mdFreeSession(iterMD);
            iterMD = appHandle->pMDRcvQueue;
        }
        else
        {
            iterMD = iterMD->pNext;
        }
    }

    /* Save the new socket in the old socket position */
    if ((socketIndex > TRDP_INVALID_SOCKET_INDEX) && (newSocket > VOS_INVALID_SOCKET))
    {
        /* Replace the old socket by the new one */
        vos_printLog(VOS_LOG_INFO,
                     "Replacing the old socket by the new one (New sock: %d, Index: %d)\n",
                     newSocket, socketIndex);

        appHandle->iface[socketIndex].sock = newSocket;
        appHandle->iface[socketIndex].rcvMostly = TRUE;
        appHandle->iface[socketIndex].tcpParams.notSend     = FALSE;
        appHandle->iface[socketIndex].type                  = TRDP_SOCK_MD_TCP;
        appHandle->iface[socketIndex].usage                 = 0;
        appHandle->iface[socketIndex].tcpParams.sendNotOk   = FALSE;
        appHandle->iface[socketIndex].tcpParams.addFileDesc = TRUE;
        appHandle->iface[socketIndex].tcpParams.connectionTimeout.tv_sec    = 0;
        appHandle->iface[socketIndex].tcpParams.connectionTimeout.tv_usec   = 0;
    }
}


/**********************************************************************************************************************/
/** set time out
 *
 *  @param[in]      pMDSession          session pointer
 *  @param[in]      usTimeOut           timeout in us
 */
void trdp_mdSetSessionTimeout (
    MD_ELE_T    *pMDSession,
    UINT32      usTimeOut)
{
    TRDP_TIME_T timeOut;

    if (NULL != pMDSession)
    {
        vos_getTime(&pMDSession->timeToGo);
        timeOut.tv_sec  = usTimeOut / 1000000;
        timeOut.tv_usec = usTimeOut % 1000000;
        vos_addTime(&pMDSession->timeToGo, &timeOut);
    }
}

/**********************************************************************************************************************/
/** Check for incoming md packet
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      pPacket         pointer to the packet to check
 *  @param[in]      packetSize      size of the packet
 *  @param[in]      checkHeaderOnly TRUE if data crc should not be checked
 *
 *  @retval         TRDP_NO_ERR          no error
 *  @retval         TRDP_TOPO_ERR
 *  @retval         TRDP_WIRE_ERR
 *  @retval         TRDP_CRC_ERR
 */
TRDP_ERR_T trdp_mdCheck (
    TRDP_SESSION_PT appHandle,
    MD_HEADER_T     *pPacket,
    UINT32          packetSize,
    BOOL8           checkHeaderOnly)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    UINT32      l_datasetLength = vos_ntohl(pPacket->datasetLength);

    /* size shall be in MIN..MAX */
    if (TRDP_NO_ERR == err)
    {
        /* Min size is sizeof(MD_HEADER_T) because in case of no data no further data and data crc32 are added */
        if (packetSize < (sizeof(MD_HEADER_T)) ||
            packetSize > TRDP_MAX_MD_PACKET_SIZE ||
            l_datasetLength > TRDP_MAX_MD_PACKET_SIZE)
        {
            appHandle->stats.udpMd.numProtErr++;
            vos_printLog(VOS_LOG_ERROR, "MDframe size error (%u)\n", packetSize);
            err = TRDP_WIRE_ERR;
        }
    }

    /*    crc check */
    if (TRDP_NO_ERR == err)
    {
        /* Check Header CRC */
        {
            UINT32 crc32 = vos_crc32(INITFCS, (UINT8 *) pPacket, sizeof(MD_HEADER_T) - SIZE_OF_FCS);

            if (pPacket->frameCheckSum != MAKE_LE(crc32))
            {
                appHandle->stats.udpMd.numCrcErr++;
                vos_printLog(VOS_LOG_ERROR, "MDframe header CRC error. Rcv: %08x vs %08x\n",
                             MAKE_LE(crc32), pPacket->frameCheckSum);
                err = TRDP_CRC_ERR;
            }

        }
    }

    /*    Check protocol version    */
    if (TRDP_NO_ERR == err)
    {
        UINT16 l_protocolVersion = vos_ntohs(pPacket->protocolVersion);
        if ((l_protocolVersion & TRDP_PROTOCOL_VERSION_CHECK_MASK) !=
            (TRDP_PROTO_VER & TRDP_PROTOCOL_VERSION_CHECK_MASK))
        {
            appHandle->stats.udpMd.numProtErr++;
            vos_printLog(VOS_LOG_ERROR, "MDframe protocol error (%04x != %04x))\n",
                         l_protocolVersion,
                         TRDP_PROTO_VER);
            err = TRDP_WIRE_ERR;
        }
    }

    /*    Check protocol type    */
    if (TRDP_NO_ERR == err)
    {
        TRDP_MSG_T l_msgType = (TRDP_MSG_T) vos_ntohs(pPacket->msgType);
        switch (l_msgType)
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
                vos_printLog(VOS_LOG_ERROR, "MDframe type error, received %c%c\n",
                             (char)(l_msgType >> 8), (char)(l_msgType & 0xFF));
                err = TRDP_WIRE_ERR;
            }
            break;
        }
    }

    /* check telegram length */
    if (TRDP_NO_ERR == err &&
        checkHeaderOnly == FALSE)
    {
        UINT32 expectedLength = 0;

        if (l_datasetLength > 0)
        {
            expectedLength = sizeof(MD_HEADER_T) + l_datasetLength + SIZE_OF_FCS;
        }
        else
        {
            expectedLength = sizeof(MD_HEADER_T);
        }

        if (packetSize < expectedLength)
        {
            appHandle->stats.udpMd.numProtErr++;
            vos_printLog(VOS_LOG_ERROR, "MDframe invalid length, received %u, expected %u\n",
                         packetSize,
                         expectedLength);
            err = TRDP_WIRE_ERR;
        }
    }
    /* check topocounter */
    if (TRDP_NO_ERR == err)
    {
        if ((pPacket->etbTopoCnt != 0 && vos_ntohl(pPacket->etbTopoCnt) != appHandle->etbTopoCnt) ||
            (pPacket->opTrnTopoCnt != 0 && vos_ntohl(pPacket->opTrnTopoCnt) != appHandle->opTrnTopoCnt))
        {
            appHandle->stats.udpMd.numTopoErr++;
            vos_printLog(VOS_LOG_ERROR, "MDframe topocount error %u/%u, expected %u/%u\n",
                         vos_ntohl(pPacket->etbTopoCnt), vos_ntohl(pPacket->opTrnTopoCnt),
                         appHandle->etbTopoCnt, appHandle->opTrnTopoCnt);
            err = TRDP_TOPO_ERR;
        }
    }
    return err;
}

/**********************************************************************************************************************/
/** Update the header values
 *
 *  @param[in]      pElement         pointer to the packet to update
 */
void    trdp_mdUpdatePacket (
    MD_ELE_T *pElement)
{
    /* Initialize CRC calculation */
    UINT32  myCRC;

    /* Get header and packet check sum values */
    UINT32  *hFCS   = &pElement->pPacket->frameHead.frameCheckSum;

    /* Calculate CRC for message head */
    myCRC = vos_crc32(INITFCS,
                      (UINT8 *)&pElement->pPacket->frameHead,
                      sizeof(MD_HEADER_T) - SIZE_OF_FCS);
    /* Convert to Little Endian */
    *hFCS = MAKE_LE(myCRC);
}

/**********************************************************************************************************************/
/** Send MD packet
 *
 *  @param[in]      pdSock          socket descriptor
 *  @param[in]      port            port on which to send
 *  @param[in]      pElement        pointer to element to be sent
 *  @retval         != NULL         error
 */
TRDP_ERR_T  trdp_mdSendPacket (
    INT32       mdSock,
    UINT32      port,
    MD_ELE_T    *pElement)
{
    VOS_ERR_T   err         = VOS_NO_ERR;
    UINT32      tmpSndSize  = 0;

    if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
    {
        tmpSndSize = pElement->sendSize;

        pElement->sendSize = pElement->grossSize - tmpSndSize;

        err = vos_sockSendTCP(mdSock, ((UINT8 *)&pElement->pPacket->frameHead) + tmpSndSize, &pElement->sendSize);
        pElement->sendSize = tmpSndSize + pElement->sendSize;

    }
    else
    {
        pElement->sendSize = pElement->grossSize;

        err = vos_sockSendUDP(mdSock,
                              (UINT8 *)&pElement->pPacket->frameHead,
                              &pElement->sendSize,
                              pElement->addr.destIpAddr,
                              port);
    }

    if (err != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_sockSend%s error (Err: %d, Sock: %d, Port: %d)\n",
                     (pElement->pktFlags & TRDP_FLAGS_TCP) ? "TCP" : "UDP", err, mdSock, port);

        if (err == VOS_NOCONN_ERR)
        {
            return TRDP_NOCONN_ERR;
        }
        else
        {
            return TRDP_BLOCK_ERR;
        }
    }

    if ((pElement->sendSize) != pElement->grossSize)
    {
        vos_printLog(VOS_LOG_INFO, "vos_sockSend%s incomplete (Sock: %d, Port: %d)\n",
                     (pElement->pktFlags & TRDP_FLAGS_TCP) ? "TCP" : "UDP", mdSock, port);
        return TRDP_IO_ERR;
    }

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Receive MD packet
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      mdSock          socket descriptor
 *  @param[in]      pElement        pointer to received packet
 *  @retval         != TRDP_NO_ERR  error
 */
TRDP_ERR_T  trdp_mdRecvPacket (
    TRDP_SESSION_PT appHandle,
    INT32           mdSock,
    MD_ELE_T        *pElement)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    UINT32      size = 0;                       /* Size of the all data read until now */
    UINT32      dataSize        = 0;            /* The pending data to read */
    UINT32      socketIndex     = 0;
    UINT32      readSize        = 0;            /* All the data read in this cycle (Header + Data) */
    UINT32      readDataSize    = 0;            /* All the data part read in this cycle (Data) */

    if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
    {
        /* Initialize to 0 the stored header size*/
        UINT32 storedHeader = 0;

        /* Initialize to 0 the pElement->dataSize
         * Once it is known, the message complete data size will be saved*/
        pElement->dataSize = 0;

        /* Find the socket index */
        for (socketIndex = 0; socketIndex < VOS_MAX_SOCKET_CNT; socketIndex++)
        {
            if (appHandle->iface[socketIndex].sock == mdSock)
            {
                break;
            }
        }

        if (socketIndex >= VOS_MAX_SOCKET_CNT)
        {
            vos_printLog(VOS_LOG_ERROR, "trdp_mdRecvPacket - Socket index out of range\n");
            return TRDP_UNKNOWN_ERR;
        }

        /* Read Header */
        if ((appHandle->uncompletedTCP[socketIndex] == NULL)
            || ((appHandle->uncompletedTCP[socketIndex] != NULL)
                && (appHandle->uncompletedTCP[socketIndex]->grossSize < sizeof(MD_HEADER_T))))
        {
            if (appHandle->uncompletedTCP[socketIndex] == NULL)
            {
                readSize = sizeof(MD_HEADER_T);
            }
            else
            {
                /* If we have read some data before, read the rest */
                readSize        = sizeof(MD_HEADER_T) - appHandle->uncompletedTCP[socketIndex]->grossSize;
                storedHeader    = appHandle->uncompletedTCP[socketIndex]->grossSize;
            }

            err = (TRDP_ERR_T) vos_sockReceiveTCP(mdSock,
                                                  ((UINT8 *)&pElement->pPacket->frameHead) + storedHeader,
                                                  &readSize);

            /* Add the read data size to the size read before */
            size = storedHeader + readSize;

            if ((appHandle->uncompletedTCP[socketIndex] != NULL)
                && (size >= sizeof(MD_HEADER_T)))
            {
                if (trdp_mdCheck(appHandle, &pElement->pPacket->frameHead, size, CHECK_HEADER_ONLY) == TRDP_NO_ERR)
                {
                    /* Uncompleted Header, completed. Save some parameters in the uncompletedTCP structure */
                    appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.datasetLength =
                        pElement->pPacket->frameHead.datasetLength;
                    appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.frameCheckSum =
                        pElement->pPacket->frameHead.frameCheckSum;
                }
                else
                {
                    vos_printLog(VOS_LOG_INFO, "TCP MD header check failed\n");
                    return TRDP_NODATA_ERR;
                }
            }
        }

        /* Read Data */
        if ((size >= sizeof(MD_HEADER_T))
            || ((appHandle->uncompletedTCP[socketIndex] != NULL)
                && (appHandle->uncompletedTCP[socketIndex]->grossSize >= sizeof(MD_HEADER_T))))
        {
            if (appHandle->uncompletedTCP[socketIndex] == NULL)
            {
                /* Get the rest of the message length */
                dataSize = vos_ntohl(pElement->pPacket->frameHead.datasetLength) +
                    sizeof(pElement->pPacket->frameHead.frameCheckSum);

                readDataSize        = dataSize;
                pElement->dataSize  = dataSize;
            }
            else
            {
                /* Calculate the data size that is pending to read */
                size        = appHandle->uncompletedTCP[socketIndex]->grossSize + readSize;
                dataSize    = vos_ntohl(appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.datasetLength) +
                    sizeof(appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.frameCheckSum);

                pElement->dataSize  = dataSize;
                dataSize        = dataSize - (size - sizeof(MD_HEADER_T));
                readDataSize    = dataSize;
            }

            /* If all the Header is read, check if more memory is needed */
            if (size >= sizeof(MD_HEADER_T))
            {
                if (trdp_packetSizeMD(pElement->dataSize) > cMinimumMDSize)
                {
                    /* we have to allocate a bigger buffer */
                    MD_PACKET_T *pBigData = (MD_PACKET_T *) vos_memAlloc(trdp_packetSizeMD(pElement->dataSize));
                    if (pBigData == NULL)
                    {
                        return TRDP_MEM_ERR;
                    }
                    /*  Swap the pointers ...  */
                    memcpy(((UINT8 *)&pBigData->frameHead) + storedHeader,
                           ((UINT8 *)&pElement->pPacket->frameHead) + storedHeader,
                           readSize);

                    vos_memFree(pElement->pPacket);
                    pElement->pPacket = pBigData;
                }
            }

            err = (TRDP_ERR_T) vos_sockReceiveTCP(mdSock,
                                                  ((UINT8 *)&pElement->pPacket->frameHead) + size,
                                                  &readDataSize);

            /* Add the read data size */
            size = size + readDataSize;

            /* Add the read Data size to the size read during this cycle */
            readSize = readSize + readDataSize;
        }
        pElement->grossSize = size;
    }
    else
    {
        /* We read the header first */
        size = sizeof(MD_HEADER_T);
        pElement->addr.srcIpAddr    = 0;
        pElement->addr.destIpAddr   = appHandle->realIP;    /* Preset destination IP  */

        err = (TRDP_ERR_T) vos_sockReceiveUDP(
                mdSock,
                (UINT8 *)pElement->pPacket,
                &size,
                &pElement->addr.srcIpAddr,
                &pElement->replyPort, &pElement->addr.destIpAddr,
                TRUE);

        /* does the announced data fit into our (small) allocated buffer?   */
        if (err == TRDP_NO_ERR)
        {
            if ((size == sizeof(MD_HEADER_T))
                && (trdp_mdCheck(appHandle, &pElement->pPacket->frameHead, size, CHECK_HEADER_ONLY) == TRDP_NO_ERR))
            {
                pElement->dataSize  = vos_ntohl(pElement->pPacket->frameHead.datasetLength);
                pElement->grossSize = trdp_packetSizeMD(pElement->dataSize);

                if (trdp_packetSizeMD(pElement->dataSize) > cMinimumMDSize)
                {
                    /* we have to allocate a bigger buffer */
                    MD_PACKET_T *pBigData = (MD_PACKET_T *) vos_memAlloc(trdp_packetSizeMD(pElement->dataSize));
                    if (pBigData == NULL)
                    {
                        return TRDP_MEM_ERR;
                    }
                    /*  Swap the pointers ...  */
                    vos_memFree(pElement->pPacket);
                    pElement->pPacket   = pBigData;
                    pElement->grossSize = trdp_packetSizeMD(pElement->dataSize);
                }

                /*  get the complete packet */
                size    = pElement->grossSize;
                err     = (TRDP_ERR_T) vos_sockReceiveUDP(mdSock,
                                                          (UINT8 *)pElement->pPacket,
                                                          &size,
                                                          &pElement->addr.srcIpAddr,
                                                          &pElement->replyPort,
                                                          &pElement->addr.destIpAddr,
                                                          FALSE);
            }
            else
            {
                /* No complain in case of size==0 and TRDP_NO_ERR for reading the header
                   - ICMP port unreachable received (result of previous send) */
                if (size != 0)
                {
                    vos_printLog(VOS_LOG_INFO,
                                 "UDP MD header check failed. Packet from socket %d thrown away\n",
                                 mdSock);
                }

                /* header information can't be read - throw the packet away reading some bytes */
                size    = sizeof(MD_HEADER_T);
                err     = (TRDP_ERR_T) vos_sockReceiveUDP(
                        mdSock,
                        (UINT8 *)pElement->pPacket,
                        &size,
                        &pElement->addr.srcIpAddr,
                        &pElement->replyPort, &pElement->addr.destIpAddr,
                        FALSE);

                return TRDP_NODATA_ERR;
            }
        }
    }

    /* If the Header is incomplete, the data size will be "0". Otherwise it will be calculated. */
    switch (err)
    {
        case TRDP_NODATA_ERR:
            vos_printLog(VOS_LOG_INFO, "vos_sockReceive%s - No data at socket %u\n",
                         (pElement->pktFlags & TRDP_FLAGS_TCP) ? "TCP" : "UDP", mdSock);
            return TRDP_NODATA_ERR;
        case TRDP_BLOCK_ERR:
            if ((((pElement->pktFlags & TRDP_FLAGS_TCP) != 0) && (readSize == 0))
                || ((pElement->pktFlags & TRDP_FLAGS_TCP) == 0))
            {
                return TRDP_BLOCK_ERR;
            }
            break;
        case TRDP_NO_ERR:
            break;
        default:
            vos_printLog(VOS_LOG_ERROR, "vos_sockReceive%s failed (Err: %d, Sock: %u)\n",
                         (pElement->pktFlags & TRDP_FLAGS_TCP) ? "TCP" : "UDP", err, mdSock);
            return err;
    }

    if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
    {
        BOOL8   noDataToRead = FALSE;
        /* All the data (Header + Data) stored in the uncompletedTCP[] array */
        UINT32  storedDataSize = 0;

        /* Check if it's necessary to read some data */
        if (pElement->grossSize == sizeof(MD_HEADER_T))
        {
            if (dataSize - sizeof(pElement->pPacket->frameHead.frameCheckSum) == 0)
            {
                noDataToRead = TRUE;
            }
        }

        /* Compare if all the data has been read */
        if (((pElement->grossSize < sizeof(MD_HEADER_T)) && (readDataSize == 0))
            || ((noDataToRead == FALSE) && (readDataSize != dataSize)))
        {
            /* Uncompleted message received */

            if (appHandle->uncompletedTCP[socketIndex] == NULL)
            {
                /* It is the first loop, no data stored yet. Allocate memory for the message */
                appHandle->uncompletedTCP[socketIndex] = (MD_ELE_T *) vos_memAlloc(sizeof(MD_ELE_T));

                if (appHandle->uncompletedTCP[socketIndex] == NULL)
                {
                    /* vos_memDelete(NULL); */
                    vos_printLog(VOS_LOG_ERROR, "vos_memAlloc() failed\n");
                    return TRDP_MEM_ERR;
                }

                /* Check the memory that is needed */
                if (trdp_packetSizeMD(pElement->dataSize) < cMinimumMDSize)
                {
                    /* Allocate the cMinimumMDSize memory at least for now*/
                    appHandle->uncompletedTCP[socketIndex]->pPacket = (MD_PACKET_T *) vos_memAlloc(cMinimumMDSize);
                }
                else
                {
                    /* Allocate the dataSize memory */
                    /* we have to allocate a bigger buffer */
                    appHandle->uncompletedTCP[socketIndex]->pPacket =
                        (MD_PACKET_T *) vos_memAlloc(trdp_packetSizeMD(pElement->dataSize));
                }

                if (appHandle->uncompletedTCP[socketIndex]->pPacket == NULL)
                {
                    return TRDP_MEM_ERR;
                }

                storedDataSize = 0;
            }
            else
            {
                /* Get the size that have been already stored in the uncompletedTCP[] */
                storedDataSize = appHandle->uncompletedTCP[socketIndex]->grossSize;

                if ((storedDataSize < sizeof(MD_HEADER_T))
                    && (pElement->grossSize > sizeof(MD_HEADER_T)))
                {
                    if (trdp_packetSizeMD(pElement->dataSize) > cMinimumMDSize)
                    {
                        /* we have to allocate a bigger buffer */
                        MD_PACKET_T *pBigData = (MD_PACKET_T *) vos_memAlloc(trdp_packetSizeMD(pElement->dataSize));
                        if (pBigData == NULL)
                        {
                            return TRDP_MEM_ERR;
                        }

                        /* Copy the data to the pBigData->frameHead */
                        memcpy(((UINT8 *)&pBigData->frameHead),
                               ((UINT8 *)&pElement->pPacket->frameHead),
                               storedDataSize);

                        /*  Swap the pointers ...  */
                        vos_memFree(appHandle->uncompletedTCP[socketIndex]->pPacket);
                        appHandle->uncompletedTCP[socketIndex]->pPacket = pBigData;
                    }
                }
            }

            if (readSize > 0)
            {
                /* Copy the read data in the uncompletedTCP[] */
                memcpy(((UINT8 *)&appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead) + storedDataSize,
                       ((UINT8 *)&pElement->pPacket->frameHead) + storedDataSize, readSize);
                appHandle->uncompletedTCP[socketIndex]->grossSize   = pElement->grossSize;
                appHandle->uncompletedTCP[socketIndex]->dataSize    = readDataSize;
            }
            else
            {
                return TRDP_PARAM_ERR;
            }

            return TRDP_PACKET_ERR;
        }
        else
        {
            /* Complete message */
            /* All data is read. Save all the data and copy to the pElement to continue */
            if (appHandle->uncompletedTCP[socketIndex] != NULL)
            {
                /* Add the received information and copy all the data to the pElement */
                storedDataSize = appHandle->uncompletedTCP[socketIndex]->grossSize;

                if ((readSize > 0))
                {
                    /* Copy the read data in the uncompletedTCP[] */
                    memcpy(((UINT8 *)&appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead) + storedDataSize,
                           ((UINT8 *)&pElement->pPacket->frameHead) + storedDataSize, readSize);

                    /* Copy all the uncompletedTCP data to the pElement */
                    memcpy(((UINT8 *)&pElement->pPacket->frameHead),
                           ((UINT8 *)&appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead), pElement->grossSize);

                    /* Disallocate the memory */
                    vos_memFree(appHandle->uncompletedTCP[socketIndex]);
                    appHandle->uncompletedTCP[socketIndex] = NULL;
                }
                else
                {
                    return TRDP_PARAM_ERR;
                }
            }
        }
    }

    /* received data */
    err = trdp_mdCheck(appHandle, &pElement->pPacket->frameHead, pElement->grossSize, CHECK_DATA_TOO);

    /*  Update statistics   */
    switch (err)
    {
        case TRDP_NO_ERR:
            appHandle->stats.udpMd.numRcv++;
            break;
        case TRDP_CRC_ERR:
            appHandle->stats.udpMd.numCrcErr++;
            break;
        case TRDP_WIRE_ERR:
            appHandle->stats.udpMd.numProtErr++;
            break;
        default:
            ;
    }

    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "trdp_mdCheck %s failed (Err: %d)\n",
                     (pElement->pktFlags & TRDP_FLAGS_TCP) ? "TCP" : "UDP", err);
    }

    return err;
}

/**********************************************************************************************************************/
/** Receiving MD messages
 *  Read the receive socket for arriving MDs, copy the packet to a new MD_ELE_T
 *  Check for protocol errors and dispatch to proper receive queue.
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      sockIndex           index of the socket to read from
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_WIRE_ERR       protocol error (late packet, version mismatch)
 *  @retval         TRDP_QUEUE_ERR      not in queue
 *  @retval         TRDP_CRC_ERR        header checksum
 *  @retval         TRDP_TOPOCOUNT_ERR  invalid topocount
 */

TRDP_ERR_T  trdp_mdRecv (
    TRDP_SESSION_PT appHandle,
    UINT32          sockIndex)
{
    TRDP_ERR_T          result          = TRDP_NO_ERR;
    MD_HEADER_T         *pH             = NULL;
    MD_ELE_T            *iterMD         = NULL;
    MD_LIS_ELE_T        *iterListener   = NULL;
    TRDP_MD_ELE_ST_T    state;
    BOOL8               isTCP           = FALSE;
    UINT32              numOfReceivers  = 0;

    if (appHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* get buffer if none available */
    if (appHandle->pMDRcvEle == NULL)
    {
        appHandle->pMDRcvEle = (MD_ELE_T *) vos_memAlloc(sizeof(MD_ELE_T));
        if (NULL != appHandle->pMDRcvEle)
        {
            appHandle->pMDRcvEle->pPacket   = NULL; /* (MD_PACKET_T *) vos_memAlloc(cMinimumMDSize); */
            appHandle->pMDRcvEle->pktFlags  = appHandle->mdDefault.flags;
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "trdp_mdRecv - Out of receive buffers!\n");
            return TRDP_MEM_ERR;
        }
    }

    if (appHandle->iface[sockIndex].type == TRDP_SOCK_MD_TCP)
    {
        appHandle->pMDRcvEle->pktFlags |= TRDP_FLAGS_TCP;
        isTCP = TRUE;
    }
    else
    {
        appHandle->pMDRcvEle->pktFlags = (TRDP_FLAGS_T) (appHandle->pMDRcvEle->pktFlags & ~TRDP_FLAGS_TCP);
        isTCP = FALSE;
    }

    if (appHandle->pMDRcvEle->pPacket == NULL)
    {
        /* Malloc the minimum size for now */
        appHandle->pMDRcvEle->pPacket = (MD_PACKET_T *) vos_memAlloc(cMinimumMDSize);

        if (appHandle->pMDRcvEle->pPacket == NULL)
        {
            vos_memFree(appHandle->pMDRcvEle);
            appHandle->pMDRcvEle = NULL;
            vos_printLog(VOS_LOG_ERROR, "trdp_mdRecv - Out of receive buffers!\n");
            return TRDP_MEM_ERR;
        }
    }

    /* get packet: */
    result = trdp_mdRecvPacket(appHandle, appHandle->iface[sockIndex].sock, appHandle->pMDRcvEle);

    if (result != TRDP_NO_ERR)
    {
        return result;
    }

    /* process message */
    pH = &appHandle->pMDRcvEle->pPacket->frameHead;

    vos_printLog(VOS_LOG_INFO,
                 "Received %s MD packet (type: '%c%c' UUID: %02x%02x%02x%02x%02x%02x%02x%02x Data len: %u)\n",
                 appHandle->pMDRcvEle->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                 (char) (appHandle->pMDRcvEle->pPacket->frameHead.msgType & 0xFF),
                 (char) (appHandle->pMDRcvEle->pPacket->frameHead.msgType >> 8),
                 appHandle->pMDRcvEle->pPacket->frameHead.sessionID[0],
                 appHandle->pMDRcvEle->pPacket->frameHead.sessionID[1],
                 appHandle->pMDRcvEle->pPacket->frameHead.sessionID[2],
                 appHandle->pMDRcvEle->pPacket->frameHead.sessionID[3],
                 appHandle->pMDRcvEle->pPacket->frameHead.sessionID[4],
                 appHandle->pMDRcvEle->pPacket->frameHead.sessionID[5],
                 appHandle->pMDRcvEle->pPacket->frameHead.sessionID[6],
                 appHandle->pMDRcvEle->pPacket->frameHead.sessionID[7],
                 vos_ntohl(appHandle->pMDRcvEle->pPacket->frameHead.datasetLength));

    /* TCP cornerIp */
    if (isTCP)
    {
        appHandle->pMDRcvEle->addr.srcIpAddr = appHandle->iface[sockIndex].tcpParams.cornerIp;
    }

    state = TRDP_ST_RX_REQ_W4AP_REPLY;

    /*  Depending on message type, we take appropriate measures */
    switch (vos_ntohs(pH->msgType))
    {
        /* Find a listener and create a new rcvQueue entry  */
        case TRDP_MSG_MN:
            state = TRDP_ST_RX_NOTIFY_RECEIVED;
        /* FALL THRU    */
        case TRDP_MSG_MR:
            /* search for existing session (in case it is a repeated request) */
            for (iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
            {
                numOfReceivers++;
                if (0 == memcmp(iterMD->pPacket->frameHead.sessionID, pH->sessionID, TRDP_SESS_ID_SIZE))
                {
                    /* request already received - discard message */
                    vos_printLog(VOS_LOG_INFO, "trdp_mdRecv: Repeated request discarded!\n");
                    return TRDP_NO_ERR;
                }
            }

            if (appHandle->mdDefault.maxNumSessions <= numOfReceivers)
            {
                /* Discard MD request, we shall not be flooded by incoming requests */
                vos_printLog(VOS_LOG_INFO, "trdp_mdRecv: Max. number of requests reached (%d)!\n", numOfReceivers);
                return TRDP_NO_ERR;
            }

            /* search for existing listener */
            for (iterListener = appHandle->pMDListenQueue; iterListener != NULL; iterListener = iterListener->pNext)
            {
                if (iterListener->socketIdx != TRDP_INVALID_SOCKET_INDEX && isTCP == TRUE)
                {
                    continue;
                }
                /* If multicast address is set, but does not match, we go to the next listener (if any) */
                if (iterListener->addr.mcGroup != 0 &&
                    iterListener->addr.mcGroup != appHandle->pMDRcvEle->addr.destIpAddr)
                {
                    /* no IP match for unicast addressing */
                    continue;
                }
                if ((iterListener->destURI[0] == 0 &&                               /* ComId listener ?    */
                     vos_ntohl(pH->comId) == iterListener->addr.comId) ||
                    (iterListener->destURI[0] != 0 &&                               /* URI listener   */
                     vos_strnicmp((CHAR8 *)iterListener->destURI, (CHAR8 *)pH->destinationURI, TRDP_DEST_URI_SIZE) == 0)
                    )
                {
                    /* We found a listener, set some values for this new session  */
                    iterMD              = appHandle->pMDRcvEle;
                    iterMD->pUserRef    = iterListener->pUserRef;
                    iterMD->stateEle    = state;

                    if (iterListener->socketIdx == TRDP_INVALID_SOCKET_INDEX)    /* On TCP, listeners have no socket
                                                                                   assigned  */
                    {
                        iterMD->socketIdx = sockIndex;
                    }
                    else
                    {
                        iterMD->socketIdx = iterListener->socketIdx;
                    }
                    /* appHandle->iface[iterMD->socketIdx].usage++;        / * mark this socket as used * / */

                    trdp_MDqueueInsFirst(&appHandle->pMDRcvQueue, iterMD);

                    appHandle->pMDRcvEle = NULL;

                    vos_printLog(VOS_LOG_INFO, "Creating %s MD replier session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                                 iterMD->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                                 pH->sessionID[0], pH->sessionID[1], pH->sessionID[2], pH->sessionID[3],
                                 pH->sessionID[4], pH->sessionID[5], pH->sessionID[6], pH->sessionID[7])

                    break;
                }
            }
            if (NULL != iterMD)
            {
                /* receive time */
                vos_getTime(&iterMD->timeToGo);

                /* timeout value */
                iterMD->interval.tv_sec     = vos_ntohl(pH->replyTimeout) / 1000000;
                iterMD->interval.tv_usec    = vos_ntohl(pH->replyTimeout) % 1000000;
                vos_addTime(&iterMD->timeToGo, &iterMD->interval);

                /* save session Id and sequence counter for next steps */
                memcpy(iterMD->sessionID, pH->sessionID, TRDP_SESS_ID_SIZE);
                iterMD->curSeqCnt = vos_ntohl(pH->sequenceCounter);

                /* save source URI for reply */
                memcpy(iterMD->srcURI, pH->sourceURI, TRDP_MAX_URI_USER_LEN);
            }
            else
            {
                if (isTCP == TRUE)
                {
                    appHandle->stats.tcpMd.numNoListener++;
                }
                else
                {
                    appHandle->stats.udpMd.numNoListener++;
                }
                vos_printLog(VOS_LOG_INFO, "trdp_mdRecv: No listener found!\n");
                return TRDP_NOLIST_ERR;
            }
            break;
        case TRDP_MSG_MC:
            /* search for existing session  */
            for (iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
            {
                if (0 == memcmp(iterMD->pPacket->frameHead.sessionID, pH->sessionID, TRDP_SESS_ID_SIZE))
                {
                    /* throw away old packet data  */
                    if (NULL != iterMD->pPacket)
                    {
                        vos_memFree(iterMD->pPacket);
                    }
                    /* and get the newly received data  */
                    iterMD->pPacket     = appHandle->pMDRcvEle->pPacket;
                    iterMD->dataSize    = vos_ntohl(pH->datasetLength);
                    iterMD->grossSize   = appHandle->pMDRcvEle->grossSize;

                    appHandle->pMDRcvEle->pPacket = NULL;

                    iterMD->curSeqCnt       = vos_ntohl(pH->sequenceCounter);
                    iterMD->addr.comId      = vos_ntohl(pH->comId);
                    iterMD->addr.etbTopoCnt  = vos_ntohl(pH->etbTopoCnt);
                    iterMD->addr.opTrnTopoCnt  = vos_ntohl(pH->opTrnTopoCnt);
                    iterMD->addr.srcIpAddr  = appHandle->pMDRcvEle->addr.srcIpAddr;
                    iterMD->addr.destIpAddr = appHandle->pMDRcvEle->addr.destIpAddr;
                    /* why??? 
                    memcpy(iterMD->destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);
                    */
                    iterMD->stateEle    = TRDP_ST_RX_CONF_RECEIVED;
                    iterMD->morituri    = TRUE;
                    vos_printLog(VOS_LOG_INFO, "Received Confirmation, session will be closed!\n");
                    break;
                }
            }
            break;
        case TRDP_MSG_MQ:
        case TRDP_MSG_MP:
            /* search for existing session */
            for (iterMD = appHandle->pMDSndQueue; iterMD != NULL; iterMD = iterMD->pNext)
            {
                if (0 == memcmp(iterMD->pPacket->frameHead.sessionID, pH->sessionID, TRDP_SESS_ID_SIZE))
                {
                    /* throw away old data  */
                    if (NULL != iterMD->pPacket)
                    {
                        vos_memFree(iterMD->pPacket);
                    }
                    /* and get the newly received data  */
                    iterMD->pPacket     = appHandle->pMDRcvEle->pPacket;
                    iterMD->dataSize    = vos_ntohl(pH->datasetLength);
                    iterMD->grossSize   = appHandle->pMDRcvEle->grossSize;

                    appHandle->pMDRcvEle->pPacket = NULL;

                    iterMD->curSeqCnt = vos_ntohl(pH->sequenceCounter);

                    /* Copy other identification params */
                    iterMD->addr.comId          = vos_ntohl(pH->comId);
                    iterMD->addr.etbTopoCnt     = vos_ntohl(pH->etbTopoCnt);
                    iterMD->addr.opTrnTopoCnt   = vos_ntohl(pH->opTrnTopoCnt);
                    iterMD->addr.srcIpAddr      = appHandle->pMDRcvEle->addr.srcIpAddr;
                    iterMD->addr.destIpAddr     = appHandle->pMDRcvEle->addr.destIpAddr;
                    
                    /* save URI for reply */
                    memcpy(iterMD->srcURI, pH->sourceURI, TRDP_MAX_URI_USER_LEN);
                    memcpy(iterMD->destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);

                    if (TRDP_MSG_MP == vos_ntohs(pH->msgType))
                    {
                        iterMD->stateEle = TRDP_ST_TX_REPLY_RECEIVED;
                        iterMD->numReplies++;

                        /* Handle multiple replies
                         Close session now if number of expected replies reached and confirmed as far as requested
                         or close session later by timeout if unknown number of replies expected */

                        if ((iterMD->numExpReplies == 1)
                            || ((iterMD->numExpReplies != 0)
                                && (iterMD->numReplies + iterMD->numRepliesQuery >= iterMD->numExpReplies)
                                && (iterMD->numConfirmSent + iterMD->numConfirmTimeout >= iterMD->numRepliesQuery)))
                        {
                            /* Prepare for session fin, Reply/ReplyQuery reception only one expected */
                            iterMD->morituri = TRUE;
                        }
                    }
                    else
                    {
                        /* Increment number of ReplyQuery received, used to count number of expected Confirms sent */
                        iterMD->numRepliesQuery++;

                        iterMD->stateEle = TRDP_ST_TX_REQ_W4AP_CONFIRM;

                        /* receive time */
                        vos_getTime(&iterMD->timeToGo);

                        /* timeout value */
                        iterMD->interval.tv_sec     = vos_ntohl(pH->replyTimeout) / 1000000;
                        iterMD->interval.tv_usec    = vos_ntohl(pH->replyTimeout) % 1000000;
                        vos_addTime(&iterMD->timeToGo, &iterMD->interval);
                    }
                    break;
                }
            }
            break;
    }

    /* Inform user  */
    if (NULL != iterMD && appHandle->mdDefault.pfCbFunction != NULL)
    {
        INT32           replyStatus = vos_ntohl(iterMD->pPacket->frameHead.replyStatus);
        TRDP_MD_INFO_T  theMessage  = cTrdp_md_info_default;

        theMessage.srcIpAddr    = iterMD->addr.srcIpAddr;
        theMessage.destIpAddr   = iterMD->addr.destIpAddr;
        theMessage.seqCount     = vos_ntohl(iterMD->pPacket->frameHead.sequenceCounter);
        theMessage.protVersion  = vos_ntohs(iterMD->pPacket->frameHead.protocolVersion);
        theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(iterMD->pPacket->frameHead.msgType);
        theMessage.comId        = vos_ntohl(iterMD->pPacket->frameHead.comId);
        theMessage.etbTopoCnt   = vos_ntohl(iterMD->pPacket->frameHead.etbTopoCnt);
        theMessage.opTrnTopoCnt = vos_ntohl(iterMD->pPacket->frameHead.opTrnTopoCnt);
        if (replyStatus >= 0)
        {
            theMessage.userStatus   = (UINT16) replyStatus;
            theMessage.replyStatus  = TRDP_REPLY_OK;
        }
        else
        {
            theMessage.userStatus   = 0;
            theMessage.replyStatus  = (TRDP_REPLY_STATUS_T) replyStatus;
        }
        theMessage.replyTimeout = vos_ntohl(iterMD->pPacket->frameHead.replyTimeout);
        memcpy(theMessage.sessionId, iterMD->pPacket->frameHead.sessionID, TRDP_SESS_ID_SIZE);
        memcpy(theMessage.destURI, iterMD->pPacket->frameHead.destinationURI, TRDP_MAX_URI_USER_LEN);
        memcpy(theMessage.srcURI, iterMD->pPacket->frameHead.sourceURI, TRDP_MAX_URI_USER_LEN);
        theMessage.numExpReplies        = iterMD->numExpReplies;
        theMessage.numReplies           = iterMD->numReplies;
        theMessage.aboutToDie           = iterMD->morituri;
        theMessage.numRepliesQuery      = iterMD->numRepliesQuery;
        theMessage.numConfirmSent       = iterMD->numConfirmSent;
        theMessage.numConfirmTimeout    = iterMD->numConfirmTimeout;
        theMessage.resultCode           = TRDP_NO_ERR;
        theMessage.pUserRef             = iterMD->pUserRef;

        appHandle->mdDefault.pfCbFunction(
            appHandle->mdDefault.pRefCon,
            appHandle,
            &theMessage,
            (UINT8 *)(iterMD->pPacket->data),
            vos_ntohl(iterMD->pPacket->frameHead.datasetLength));
    }

    /*  notification sessions can be discarded after application was informed */
    if (NULL != iterMD && iterMD->stateEle == TRDP_ST_RX_NOTIFY_RECEIVED)
    {
        iterMD->morituri = TRUE;
    }

    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/** Sending MD messages
 *  Send the messages stored in the sendQueue
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 */
TRDP_ERR_T  trdp_mdSend (
    TRDP_SESSION_PT appHandle)
{
    TRDP_ERR_T  result      = TRDP_NO_ERR;
    MD_ELE_T    *iterMD     = appHandle->pMDSndQueue;
    BOOL8       firstLoop   = TRUE;

    /*  Find the packet which has to be sent next:
     Note: We must also check the receive queue for pending replies! */
    do
    {
        int dotx = 0;
        TRDP_MD_ELE_ST_T nextstate = TRDP_ST_NONE;

        /*  Switch to receive queue */
        if (NULL == iterMD && TRUE == firstLoop)
        {
            iterMD      = appHandle->pMDRcvQueue;
            firstLoop   = FALSE;
        }

        if (NULL == iterMD)
        {
            break;
        }

        switch (iterMD->stateEle)
        {
            case TRDP_ST_TX_NOTIFY_ARM:
                dotx = 1;
                break;
            case TRDP_ST_TX_REQUEST_ARM:
                dotx        = 1;
                nextstate   = TRDP_ST_TX_REQUEST_W4REPLY;
                break;
            case TRDP_ST_TX_REPLY_ARM:
                dotx = 1;
                break;
            case TRDP_ST_TX_REPLYQUERY_ARM:
                dotx        = 1;
                nextstate   = TRDP_ST_RX_REPLYQUERY_W4C;
                break;
            case TRDP_ST_TX_CONFIRM_ARM:
                dotx = 1;
                break;
            default:
                break;
        }
        if (dotx)
        {
            /*    Send the packet if it is not redundant    */
            if ((iterMD->socketIdx != TRDP_INVALID_SOCKET_INDEX)
                && (!appHandle->beQuiet || (iterMD->privFlags & TRDP_REDUNDANT)))
            {
                trdp_mdUpdatePacket(iterMD);

                if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                {
                    if (iterMD->tcpParameters.doConnect == TRUE)
                    {
                        VOS_ERR_T err;
                        /* Connect() the socket */
                        err = vos_sockConnect(appHandle->iface[iterMD->socketIdx].sock,
                                              iterMD->addr.destIpAddr, appHandle->mdDefault.tcpPort);

                        if (err == VOS_NO_ERR)
                        {
                            iterMD->tcpParameters.doConnect = FALSE;
                        }
                        else if (err == VOS_BLOCK_ERR)
                        {
                            vos_printLog(VOS_LOG_INFO,
                                         "Socket connection for TCP not ready (Sock: %u, Port: %d)\n",
                                         appHandle->iface[iterMD->socketIdx].sock,
                                         (UINT32) appHandle->mdDefault.tcpPort);
                            iterMD->tcpParameters.doConnect = FALSE;
                            iterMD = iterMD->pNext;
                            continue;
                        }
                        else
                        {
                            /* ToDo: What in case of a permanent failure? */
                            vos_printLog(VOS_LOG_INFO,
                                         "Socket connection for TCP failed (Sock: %u, Port: %d)\n",
                                         appHandle->iface[iterMD->socketIdx].sock,
                                         (UINT32) appHandle->mdDefault.tcpPort);

                            if (appHandle->iface[iterMD->socketIdx].tcpParams.sendNotOk == FALSE)
                            {
                                /*  Start the Sending Timeout */
                                TRDP_TIME_T tmpt_interval, tmpt_now;

                                tmpt_interval.tv_sec    = appHandle->mdDefault.sendingTimeout / 1000000;
                                tmpt_interval.tv_usec   = appHandle->mdDefault.sendingTimeout % 1000000;

                                vos_getTime(&tmpt_now);
                                vos_addTime(&tmpt_now, &tmpt_interval);

                                memcpy(&appHandle->iface[iterMD->socketIdx].tcpParams.sendingTimeout,
                                       &tmpt_now,
                                       sizeof(TRDP_TIME_T));

                                appHandle->iface[iterMD->socketIdx].tcpParams.sendNotOk = TRUE;
                            }

                            iterMD->morituri = TRUE;
                            iterMD = iterMD->pNext;
                            continue;
                        }
                    }
                }

                if (((iterMD->pktFlags & TRDP_FLAGS_TCP) == 0)
                    || (((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                        && ((appHandle->iface[iterMD->socketIdx].tcpParams.notSend == FALSE)
                            || (iterMD->tcpParameters.msgUncomplete == TRUE))))
                {

                    if (0 != iterMD->replyPort &&
                        (iterMD->pPacket->frameHead.msgType == vos_ntohs(TRDP_MSG_MP) ||
                         iterMD->pPacket->frameHead.msgType == vos_ntohs(TRDP_MSG_MQ)))
                    {
                        result = trdp_mdSendPacket(appHandle->iface[iterMD->socketIdx].sock,
                                                   iterMD->replyPort,
                                                   iterMD);
                    }
                    else
                    {
                        result = trdp_mdSendPacket(appHandle->iface[iterMD->socketIdx].sock,
                                                   appHandle->mdDefault.udpPort,
                                                   iterMD);
                    }

                    if (result == TRDP_NO_ERR)
                    {
                        if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                        {
                            appHandle->iface[iterMD->socketIdx].tcpParams.notSend = FALSE;
                            iterMD->tcpParameters.msgUncomplete = FALSE;
                            appHandle->iface[iterMD->socketIdx].tcpParams.sendNotOk = FALSE;

                            /* Add the socket in the file descriptor*/
                            appHandle->iface[iterMD->socketIdx].tcpParams.addFileDesc = TRUE;
                        }

                        if (nextstate == TRDP_ST_RX_REPLY_SENT ||
                            nextstate == TRDP_ST_RX_REPLYQUERY_W4C)
                        {
                            /* Update timeout */
                            vos_getTime(&iterMD->timeToGo);
                            vos_addTime(&iterMD->timeToGo, &iterMD->interval);
                            vos_printLog(VOS_LOG_INFO, "Setting timeout for confirmation!\n");
                        }

                        appHandle->stats.udpMd.numSend++;

                        switch (iterMD->stateEle)
                        {
                            case TRDP_ST_TX_CONFIRM_ARM:
                            {
                                iterMD->numConfirmSent++;
                                if (
                                    (iterMD->numExpReplies != 0)
                                    && ((iterMD->numRepliesQuery + iterMD->numReplies) >= iterMD->numExpReplies)
                                    && (iterMD->numConfirmSent >= iterMD->numRepliesQuery))
                                {
                                    iterMD->morituri = TRUE;
                                }
                                else
                                {
                                    /* not yet all replies received OR not yet all confirmations sent */
                                    if (iterMD->numConfirmSent < iterMD->numRepliesQuery)
                                    {
                                        nextstate = TRDP_ST_TX_REQ_W4AP_CONFIRM;
                                    }
                                    else
                                    {
                                        nextstate = TRDP_ST_TX_REQUEST_W4REPLY;
                                    }
                                }
                            }
                            break;
                            case TRDP_ST_TX_NOTIFY_ARM:
                            case TRDP_ST_TX_REPLY_ARM:
                            {
                                iterMD->morituri = TRUE;
                            }
                            break;
                            default:
                                ;
                        }
                        iterMD->stateEle = nextstate;
                    }
                    else
                    {
                        if (result == TRDP_IO_ERR)
                        {
                            /* Send uncompleted */
                            if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                            {
                                appHandle->iface[iterMD->socketIdx].tcpParams.notSend = TRUE;
                                iterMD->tcpParameters.msgUncomplete = TRUE;

                                if (appHandle->iface[iterMD->socketIdx].tcpParams.sendNotOk == FALSE)
                                {
                                    /*  Start the Sending Timeout */
                                    TRDP_TIME_T tmpt_interval, tmpt_now;

                                    tmpt_interval.tv_sec    = appHandle->mdDefault.sendingTimeout / 1000000;
                                    tmpt_interval.tv_usec   = appHandle->mdDefault.sendingTimeout % 1000000;

                                    vos_getTime(&tmpt_now);
                                    vos_addTime(&tmpt_now, &tmpt_interval);

                                    memcpy(&appHandle->iface[iterMD->socketIdx].tcpParams.sendingTimeout,
                                           &tmpt_now,
                                           sizeof(TRDP_TIME_T));

                                    appHandle->iface[iterMD->socketIdx].tcpParams.sendNotOk = TRUE;
                                }
                            }
                        }
                        else
                        {
                            MD_ELE_T *iterMD_find = NULL;

                            /* search for existing session */
                            for (iterMD_find = appHandle->pMDSndQueue;
                                 iterMD_find != NULL;
                                 iterMD_find = iterMD_find->pNext)
                            {
                                if (iterMD_find->socketIdx == iterMD->socketIdx)
                                {
                                    iterMD_find->morituri = TRUE;

                                    /* Execute callback for each session */
                                    if (appHandle->mdDefault.pfCbFunction != NULL)
                                    {
                                        INT32           replyStatus = vos_ntohl(iterMD_find->pPacket->frameHead.replyStatus);
                                        TRDP_MD_INFO_T  theMessage  = cTrdp_md_info_default;

                                        theMessage.srcIpAddr    = 0;
                                        theMessage.destIpAddr   = iterMD_find->addr.destIpAddr;
                                        theMessage.seqCount     = vos_ntohs(
                                                iterMD_find->pPacket->frameHead.sequenceCounter);
                                        theMessage.protVersion  = vos_ntohs(
                                                iterMD_find->pPacket->frameHead.protocolVersion);
                                        theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(
                                                iterMD_find->pPacket->frameHead.msgType);
                                        theMessage.comId        = iterMD_find->addr.comId;
                                        theMessage.etbTopoCnt   = iterMD_find->addr.etbTopoCnt;
                                        theMessage.opTrnTopoCnt = iterMD_find->addr.opTrnTopoCnt;
                                        if (replyStatus >= 0)
                                        {
                                            theMessage.userStatus   = (UINT16) replyStatus;
                                            theMessage.replyStatus  = TRDP_REPLY_OK;
                                        }
                                        else
                                        {
                                            theMessage.userStatus   = 0;
                                            theMessage.replyStatus  = (TRDP_REPLY_STATUS_T) replyStatus;
                                        }
                                        memcpy(theMessage.sessionId,
                                               iterMD_find->pPacket->frameHead.sessionID,
                                               TRDP_SESS_ID_SIZE);
                                        theMessage.replyTimeout = vos_ntohl(
                                                iterMD_find->pPacket->frameHead.replyTimeout);
                                        memcpy(theMessage.destURI, iterMD_find->destURI, TRDP_MAX_URI_USER_LEN);
                                        memcpy(theMessage.srcURI, iterMD_find->pPacket->frameHead.sourceURI, TRDP_MAX_URI_USER_LEN);
                                        theMessage.numExpReplies        = iterMD_find->numReplies;
                                        theMessage.pUserRef             = iterMD_find->pUserRef;
                                        theMessage.numReplies           = iterMD_find->numReplies;
                                        theMessage.aboutToDie           = iterMD_find->morituri;
                                        theMessage.numRepliesQuery      = iterMD_find->numRepliesQuery;
                                        theMessage.numConfirmSent       = iterMD_find->numConfirmSent;
                                        theMessage.numConfirmTimeout    = iterMD_find->numConfirmTimeout;

                                        /* theMessage.pUserRef     = appHandle->mdDefault.pRefCon; */
                                        theMessage.resultCode = TRDP_TIMEOUT_ERR;

                                        appHandle->mdDefault.pfCbFunction(
                                            appHandle->mdDefault.pRefCon,
                                            appHandle,
                                            &theMessage,
                                            (UINT8 *)0, 0);
                                    }
                                }
                            }

                            /* Close the socket */
                            appHandle->iface[iterMD->socketIdx].tcpParams.morituri = TRUE;
                        }
                    }
                }
            }
        }
        iterMD = iterMD->pNext;
    }
    while (TRUE); /*lint !e506 */

    trdp_mdCloseSessions(appHandle, TRDP_INVALID_SOCKET_INDEX, VOS_INVALID_SOCKET, TRUE);

    return result;
}


/******************************************************************************/
/** Check for pending packets, set FD if non blocking
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in,out]  pFileDesc           pointer to set of ready descriptors
 *  @param[in,out]  pNoDesc             pointer to number of ready descriptors
 */

void trdp_mdCheckPending (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc)
{
    int lIndex;
    MD_ELE_T *iterMD;
    MD_LIS_ELE_T *iterListener;

    /*    Add the socket to the pFileDesc    */
    if (appHandle->tcpFd.listen_sd != VOS_INVALID_SOCKET)
    {
        FD_SET(appHandle->tcpFd.listen_sd, (fd_set *)pFileDesc); /*lint !e573 signed/unsigned division in macro */
        if (appHandle->tcpFd.listen_sd > *pNoDesc)
        {
            *pNoDesc = appHandle->tcpFd.listen_sd;
        }
    }

    for (lIndex = 0; lIndex < VOS_MAX_SOCKET_CNT; lIndex++)
    {
        if ((appHandle->iface[lIndex].sock != VOS_INVALID_SOCKET)
            && (appHandle->iface[lIndex].type == TRDP_SOCK_MD_TCP)
            && (appHandle->iface[lIndex].tcpParams.addFileDesc == TRUE))
        {
            FD_SET(appHandle->iface[lIndex].sock, (fd_set *)pFileDesc); /*lint !e573 signed/unsigned division in macro
                                                                          */
            if (appHandle->iface[lIndex].sock > *pNoDesc)
            {
                *pNoDesc = appHandle->iface[lIndex].sock;
            }
        }
    }

    /*  Include MD UDP listener sockets sockets  */
    for (iterListener = appHandle->pMDListenQueue;
         iterListener != NULL;
         iterListener = iterListener->pNext)
    {
        /*    There can be several sockets depending on TRDP_PD_CONFIG_T    */
        if ((iterListener->socketIdx != TRDP_INVALID_SOCKET_INDEX)
            && (appHandle->iface[iterListener->socketIdx].sock != VOS_INVALID_SOCKET)
            && ((appHandle->iface[iterListener->socketIdx].type != TRDP_SOCK_MD_TCP)
                || ((appHandle->iface[iterListener->socketIdx].type == TRDP_SOCK_MD_TCP)
                    && (appHandle->iface[iterListener->socketIdx].tcpParams.addFileDesc == TRUE))))
        {
            if (!FD_ISSET(appHandle->iface[iterListener->socketIdx].sock, (fd_set *)pFileDesc)) /*lint !e573
                                                                                                  signed/unsigned
                                                                                                  division in macro */
            {
                FD_SET(appHandle->iface[iterListener->socketIdx].sock, (fd_set *)pFileDesc); /*lint !e573
                                                                                               signed/unsigned division
                                                                                               in macro */
                if (appHandle->iface[iterListener->socketIdx].sock > *pNoDesc)
                {
                    *pNoDesc = appHandle->iface[iterListener->socketIdx].sock;
                }
            }
        }
    }

    /*  Include MD UDP receive sockets */
    for (iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
    {
        /*    There can be several sockets depending on TRDP_PD_CONFIG_T    */
        if ((iterMD->socketIdx != TRDP_INVALID_SOCKET_INDEX)
            && (appHandle->iface[iterMD->socketIdx].sock != VOS_INVALID_SOCKET)
            && ((appHandle->iface[iterMD->socketIdx].type != TRDP_SOCK_MD_TCP)
                || ((appHandle->iface[iterMD->socketIdx].type == TRDP_SOCK_MD_TCP)
                    && (appHandle->iface[iterMD->socketIdx].tcpParams.addFileDesc == TRUE))))
        {
            if (!FD_ISSET(appHandle->iface[iterMD->socketIdx].sock, (fd_set *)pFileDesc)) /*lint !e573 signed/unsigned
                                                                                            division in macro */
            {
                FD_SET(appHandle->iface[iterMD->socketIdx].sock, (fd_set *)pFileDesc); /*lint !e573 signed/unsigned
                                                                                         division in macro */
                if (appHandle->iface[iterMD->socketIdx].sock > *pNoDesc)
                {
                    *pNoDesc = appHandle->iface[iterMD->socketIdx].sock;
                }
            }
        }
    }

    for (iterMD = appHandle->pMDSndQueue; iterMD != NULL; iterMD = iterMD->pNext)
    {
        /*    There can be several sockets depending on TRDP_PD_CONFIG_T    */
        if ((iterMD->socketIdx != TRDP_INVALID_SOCKET_INDEX)
            && (appHandle->iface[iterMD->socketIdx].sock != VOS_INVALID_SOCKET)
            && ((appHandle->iface[iterMD->socketIdx].type != TRDP_SOCK_MD_TCP)
                || ((appHandle->iface[iterMD->socketIdx].type == TRDP_SOCK_MD_TCP)
                    && (appHandle->iface[iterMD->socketIdx].tcpParams.addFileDesc == TRUE))))
        {
            if (!FD_ISSET(appHandle->iface[iterMD->socketIdx].sock, (fd_set *)pFileDesc)) /*lint !e573 signed/unsigned
                                                                                            division in macro */
            {
                FD_SET(appHandle->iface[iterMD->socketIdx].sock, (fd_set *)pFileDesc); /*lint !e573 signed/unsigned
                                                                                         division in macro */
                if (appHandle->iface[iterMD->socketIdx].sock >= *pNoDesc)
                {
                    *pNoDesc = appHandle->iface[iterMD->socketIdx].sock;
                }
            }
        }
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
void  trdp_mdCheckListenSocks (
    TRDP_SESSION_PT appHandle,
    TRDP_FDS_T      *pRfds,
    INT32           *pCount)
{
    TRDP_FDS_T  rfds;
    INT32       noOfDesc;
    INT32       highDesc = VOS_INVALID_SOCKET;
    INT32       lIndex;
    TRDP_ERR_T  err;
    INT32       new_sd = VOS_INVALID_SOCKET;

    if (appHandle == NULL)
    {
        return;
    }

    /*  If no descriptor set was supplied, we use our own. We set all used descriptors as readable.
        This eases further handling of the sockets  */
    if (pRfds == NULL)
    {
        /* polling mode */
        VOS_TIME_T timeOut = {0, 1000};     /* at least 1 ms */
        FD_ZERO((fd_set *)&rfds);

        /* Add the listen_sd in the file descriptor */
        if (appHandle->tcpFd.listen_sd != VOS_INVALID_SOCKET)
        {
            FD_SET(appHandle->tcpFd.listen_sd, (fd_set *)&rfds); /*lint !e573 signed/unsigned division in macro */
            if (appHandle->tcpFd.listen_sd > highDesc)
            {
                highDesc = appHandle->tcpFd.listen_sd;
            }
        }

        /* scan for sockets */
        for (lIndex = 0; lIndex < VOS_MAX_SOCKET_CNT; lIndex++)
        {
            if (appHandle->iface[lIndex].sock != VOS_INVALID_SOCKET &&
                appHandle->iface[lIndex].type != TRDP_SOCK_PD
                && ((appHandle->iface[lIndex].type != TRDP_SOCK_MD_TCP)
                    || ((appHandle->iface[lIndex].type == TRDP_SOCK_MD_TCP)
                        && (appHandle->iface[lIndex].tcpParams.addFileDesc == TRUE))))
            {
                FD_SET(appHandle->iface[lIndex].sock, (fd_set *)&rfds); /*lint !e573 signed/unsigned division in macro
                                                                          */
                if (highDesc < appHandle->iface[lIndex].sock)
                {
                    highDesc = appHandle->iface[lIndex].sock;
                }
            }
        }
        if (highDesc == VOS_INVALID_SOCKET)
        {
            return;
        }
        noOfDesc = vos_select(highDesc + 1, &rfds, NULL, NULL, &timeOut);
        if (noOfDesc == VOS_INVALID_SOCKET)
        {
            vos_printLog(VOS_LOG_ERROR, "select() failed\n");
            return;
        }
        pRfds   = &rfds;
        pCount  = &noOfDesc;
    }

    if (pCount != NULL && *pCount > 0)
    {
        /*    Check the sockets for received MD packets    */

        /**********************************************************/
        /* One or more descriptors are readable.  Need to         */
        /* determine which ones they are.                         */
        /**********************************************************/

        /****************************************************/
        /* Check to see if this is the TCP listening socket */
        /****************************************************/

        /* vos_printLog(VOS_LOG_INFO, " ----- CHECKING READY DESCRIPTORS -----\n"); */
        if (appHandle->tcpFd.listen_sd != VOS_INVALID_SOCKET &&
            FD_ISSET(appHandle->tcpFd.listen_sd, (fd_set *)pRfds)) /*lint !e573 signed/unsigned division in macro */
        {
            /****************************************************/
            /* A TCP connection request in the listen socket.   */
            /****************************************************/
            (*pCount)--;

            /*************************************************/
            /* Accept all incoming connections that are      */
            /* queued up on the listening socket.            */
            /*************************************************/
            do
            {
                /**********************************************/
                /* Accept each incoming connection.           */
                /* Check any failure on accept                */
                /**********************************************/
                TRDP_IP_ADDR_T  newIp;
                UINT16          read_tcpPort;

                newIp = appHandle->realIP;
                read_tcpPort = appHandle->mdDefault.tcpPort;

                err = (TRDP_ERR_T) vos_sockAccept(appHandle->tcpFd.listen_sd,
                                                  &new_sd, &newIp,
                                                  &(read_tcpPort));

                if (new_sd < 0)
                {
                    if (err == TRDP_NO_ERR)
                    {
                        break;
                    }
                    else
                    {
                        vos_printLog(VOS_LOG_ERROR, "vos_sockAccept() failed (Err: %d, Sock: %d, Port: %d)\n",
                                     err, appHandle->tcpFd.listen_sd, (UINT32) read_tcpPort);

                        /* Callback the error to the application  */
                        if (appHandle->mdDefault.pfCbFunction != NULL)
                        {
                            TRDP_MD_INFO_T theMessage = cTrdp_md_info_default;

                            theMessage.etbTopoCnt   = appHandle->etbTopoCnt;
                            theMessage.opTrnTopoCnt = appHandle->opTrnTopoCnt;
                            theMessage.resultCode   = TRDP_SOCK_ERR;
                            theMessage.srcIpAddr    = newIp;
                            appHandle->mdDefault.pfCbFunction(appHandle->mdDefault.pRefCon, appHandle,
                                                              &theMessage, NULL, 0);
                        }
                        continue;
                    }
                }
                else
                {
                    vos_printLog(VOS_LOG_INFO, "vos_sockAccept() (Sock: %d, Port: %d)\n", new_sd, read_tcpPort);
                }

                {
                    VOS_SOCK_OPT_T trdp_sock_opt;

                    trdp_sock_opt.qos   = appHandle->mdDefault.sendParam.qos;
                    trdp_sock_opt.ttl   = appHandle->mdDefault.sendParam.ttl;
                    trdp_sock_opt.ttl_multicast = 0;
                    trdp_sock_opt.reuseAddrPort = TRUE;
                    trdp_sock_opt.nonBlocking   = TRUE;

                    err = (TRDP_ERR_T) vos_sockSetOptions(new_sd, &trdp_sock_opt);
                    if (err != TRDP_NO_ERR)
                    {
                        continue;
                    }
                }

                /* There is one more socket to manage */

                /* Compare with the sockets stored in the socket list */
                {
                    INT32   socketIndex;
                    BOOL8   socketFound = FALSE;

                    for (socketIndex = 0; socketIndex < VOS_MAX_SOCKET_CNT; socketIndex++)
                    {
                        if ((appHandle->iface[socketIndex].sock != VOS_INVALID_SOCKET)
                            && (appHandle->iface[socketIndex].type == TRDP_SOCK_MD_TCP)
                            && (appHandle->iface[socketIndex].tcpParams.cornerIp == newIp)
                            && (appHandle->iface[socketIndex].rcvMostly == TRUE))
                        {
                            vos_printLog(VOS_LOG_INFO, "New socket accepted from the same device (Ip = %u)\n", newIp);

                            if (appHandle->iface[socketIndex].usage > 0)
                            {
                                vos_printLog(
                                    VOS_LOG_INFO,
                                    "The new socket accepted from the same device (Ip = %u), won't be removed, because it is still using\n",
                                    newIp);
                                socketFound = TRUE;
                                break;
                            }

                            if (FD_ISSET(appHandle->iface[socketIndex].sock, (fd_set *) pRfds)) /*lint !e573
                                                                                                  signed/unsigned
                                                                                                  division in macro */
                            {
                                /* Decrement the Ready descriptors counter */
                                (*pCount)--;
                                FD_CLR(appHandle->iface[socketIndex].sock, (fd_set *) pRfds); /*lint !e502 !e573
                                                                                                signed/unsigned division
                                                                                                in macro */
                            }


                            /* Close the old socket */
                            appHandle->iface[socketIndex].tcpParams.morituri = TRUE;

                            /* Manage the socket pool (update the socket) */
                            trdp_mdCloseSessions(appHandle, socketIndex, new_sd, TRUE);

                            socketFound = TRUE;
                            break;
                        }
                    }

                    if (socketFound == FALSE)
                    {
                        /* Save the new socket in the iface.
                           On receiving MD data on this connection, a listener will be searched and a receive
                           session instantiated. The socket/connection will be closed when the session has finished.
                         */
                        err = trdp_requestSocket(
                                appHandle->iface,
                                appHandle->mdDefault.tcpPort,
                                &appHandle->mdDefault.sendParam,
                                appHandle->realIP,
                                0,
                                TRDP_SOCK_MD_TCP,
                                TRDP_OPTION_NONE,
                                TRUE,
                                new_sd,
                                &socketIndex,
                                newIp);

                        if (err != TRDP_NO_ERR)
                        {
                            vos_printLog(VOS_LOG_ERROR, "trdp_requestSocket() failed (Err: %d, Port: %d)\n",
                                         err, (UINT32)appHandle->mdDefault.tcpPort);
                        }
                    }
                }

                /**********************************************/
                /* Loop back up and accept another incoming   */
                /* connection                                 */
                /**********************************************/
            }
            while (new_sd != VOS_INVALID_SOCKET);
        }
    }

    /* Check Receive Data (UDP & TCP) */
    /*  Loop through the socket list and check readiness
        (but only while there are ready descriptors left) */
    for (lIndex = 0; lIndex < VOS_MAX_SOCKET_CNT; lIndex++)
    {
        if (appHandle->iface[lIndex].sock != VOS_INVALID_SOCKET &&
            appHandle->iface[lIndex].type != TRDP_SOCK_PD &&
            FD_ISSET(appHandle->iface[lIndex].sock, (fd_set *)pRfds) != 0) /*lint !e573 signed/unsigned division in
                                                                             macro */
        {
            if (pCount != NULL)
            {
                (*pCount)--;
                if (*pCount < 0)
                {
                    break;
                }
            }
            FD_CLR(appHandle->iface[lIndex].sock, (fd_set *)pRfds); /*lint !e502 !e573 signed/unsigned division in macro
                                                                      */
            err = trdp_mdRecv(appHandle, lIndex);

            /* The receive message is incomplete */
            if (err == TRDP_PACKET_ERR && appHandle->iface[lIndex].type == TRDP_SOCK_MD_TCP)
            {
                vos_printLog(VOS_LOG_INFO, "Incomplete TCP MD received \n");
            }

            /* Check if the socket has been closed in the other corner */
            if (err == TRDP_NODATA_ERR && appHandle->iface[lIndex].type == TRDP_SOCK_MD_TCP)
            {
                vos_printLog(VOS_LOG_INFO,
                             "The socket has been closed in the other corner (Corner Ip: %u, Socket: %d)\n",
                             appHandle->iface[lIndex].tcpParams.cornerIp,
                             appHandle->iface[lIndex].sock);

                appHandle->iface[lIndex].tcpParams.morituri = TRUE;

                trdp_mdCloseSessions(appHandle, TRDP_INVALID_SOCKET_INDEX, VOS_INVALID_SOCKET, TRUE);
            }
        }
    }
}

/**********************************************************************************************************************/
/** Checking message data timeouts
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 */
void  trdp_mdCheckTimeouts (
    TRDP_SESSION_PT appHandle)
{
    MD_ELE_T    *iterMD     = appHandle->pMDSndQueue;
    BOOL8       firstLoop   = TRUE;
    BOOL8       timeOut     = FALSE;
    TRDP_TIME_T now;

    if (appHandle == NULL)
    {
        return;
    }

    /*  Find the sessions which needs action
     Note: We must also check the receive queue for pending replies! */
    do
    {
        TRDP_ERR_T resultCode = TRDP_UNKNOWN_ERR;

        /* Update the current time always inside loop in case of application delays  */
        vos_getTime(&now);

        /*  Switch to receive queue */
        if (NULL == iterMD && TRUE == firstLoop)
        {
            iterMD      = appHandle->pMDRcvQueue;
            firstLoop   = FALSE;
        }

        /*  Are we finished?   */
        if (NULL == iterMD)
        {
            break;
        }

        /* timeToGo is timeout value! */
        if (0 > vos_cmpTime(&iterMD->timeToGo, &now))     /* timeout overflow */
        {
            /* timeout on queue ? */
            switch (iterMD->stateEle)
            {
                /* Replier waiting for reply from application */
                case TRDP_ST_RX_REQ_W4AP_REPLY:

                /* Caller waiting for a confirmation/reply from application*/
                case TRDP_ST_TX_REQ_W4AP_CONFIRM:
                {
                    /* Application confirm/reply timeout, stop session, notify application */
                    iterMD->morituri = TRUE;
                    timeOut = TRUE;

                    if (iterMD->stateEle == TRDP_ST_TX_REQ_W4AP_CONFIRM)
                    {
                        vos_printLog(VOS_LOG_ERROR, "MD application confirm timeout\n");

                        resultCode = TRDP_APP_CONFIRMTO_ERR;
                    }
                    else
                    {
                        vos_printLog(VOS_LOG_ERROR, "MD application reply timeout\n");

                        resultCode = TRDP_APP_REPLYTO_ERR;
                    }
                }
                break;

                /* Waiting for Reply/ReplyQuery reception and Confirm sent */
                case TRDP_ST_TX_REQUEST_W4REPLY:
                {
                    if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                    {
                        vos_printLog(VOS_LOG_INFO, "TCP MD reply/confirm timeout\n");
                        iterMD->morituri    = TRUE;
                        timeOut     = TRUE;
                        resultCode  = TRDP_REPLYTO_ERR;

                        appHandle->stats.tcpMd.numReplyTimeout++;
                        break;
                    }

                    /* Manage Reply/ReplyQuery reception */
                    if (iterMD->morituri == FALSE)
                    {
                        /* Session is in reception phase */

                        /* Check for Reply timeout */
                        vos_printLog(VOS_LOG_INFO, "UDP MD reply/confirm timeout\n");

                        /* Reply timeout, stop Reply/ReplyQuery reception, notify application */
                        iterMD->morituri    = TRUE;
                        timeOut     = TRUE;
                        resultCode  = TRDP_REPLYTO_ERR;

                        /* Statistics */
                        appHandle->stats.udpMd.numReplyTimeout++;
                    }

                    /* Manage send Confirm if no repetition */
                    if (iterMD->stateEle != TRDP_ST_TX_REQUEST_ARM)
                    {
                        if ((iterMD->numRepliesQuery == 0)
                            || (iterMD->numRepliesQuery <= iterMD->numConfirmSent))
                        {
                            /* All Confirm required by received ReplyQuery are sent */
                            iterMD->morituri = TRUE;
                        }
                        else
                        {
                            /* Check for pending Confirm timeout (handled in each single listener) */
                            if (iterMD->numRepliesQuery <= (iterMD->numConfirmSent + iterMD->numConfirmTimeout))
                            {
                                /* Callback execution require to indicate send done with some Confirm Timeout */
                                iterMD->morituri    = TRUE;
                                timeOut     = TRUE;
                                resultCode  = TRDP_REQCONFIRMTO_ERR;
                            }
                        }
                    }
                }
                break;
                case TRDP_ST_RX_REPLYQUERY_W4C:
                {
                    /* Reply query timeout raised, stop waiting for confirmation, notify application */
                    iterMD->morituri    = TRUE;
                    timeOut     = TRUE;
                    resultCode  = TRDP_CONFIRMTO_ERR;

                    /* Statistics */
                    if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                    {
                        appHandle->stats.tcpMd.numConfirmTimeout++;
                    }
                    else
                    {
                        appHandle->stats.udpMd.numConfirmTimeout++;
                    }
                }
                break;
                case TRDP_ST_RX_REPLY_SENT:
                    /* no complain within time out, kill session silently  */
                    iterMD->morituri = TRUE;
                    break;
                case TRDP_ST_TX_REPLY_RECEIVED:
                    /* kill session silently since only one TCP reply possible */
                    if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                    {
                        iterMD->morituri = TRUE;
                        break;
                    }

                    /* kill session if # of replies is undefined or not all expected replies have been received */
                    if ((iterMD->numExpReplies == 0)
                        || (iterMD->numReplies < iterMD->numExpReplies))
                    {
                        iterMD->morituri    = TRUE;
                        timeOut     = TRUE;
                        resultCode  = TRDP_REPLYTO_ERR;
                    }
                    else
                    {
                        /* kill session silently if number of expected replies have been received  */
                        iterMD->morituri = TRUE;
                    }
                    break;
                default:
                    break;
            }
        }

        if (TRUE == timeOut)    /* Notify user  */
        {
            /* Execute callback */
            if (appHandle->mdDefault.pfCbFunction != NULL)
            {
                INT32           replyStatus = vos_ntohl(iterMD->pPacket->frameHead.replyStatus);
                TRDP_MD_INFO_T  theMessage  = cTrdp_md_info_default;

                theMessage.srcIpAddr    = 0;
                theMessage.destIpAddr   = iterMD->addr.destIpAddr;
                theMessage.seqCount     = vos_ntohs(iterMD->pPacket->frameHead.sequenceCounter);
                theMessage.protVersion  = vos_ntohs(iterMD->pPacket->frameHead.protocolVersion);
                theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(iterMD->pPacket->frameHead.msgType);
                theMessage.comId        = iterMD->addr.comId;
                theMessage.etbTopoCnt   = iterMD->addr.etbTopoCnt;
                theMessage.opTrnTopoCnt = iterMD->addr.opTrnTopoCnt;
                if (replyStatus >= 0)
                {
                    theMessage.userStatus   = (UINT16) replyStatus;
                    theMessage.replyStatus  = TRDP_REPLY_OK;
                }
                else
                {
                    theMessage.userStatus   = 0;
                    theMessage.replyStatus  = (TRDP_REPLY_STATUS_T) replyStatus;
                }
                memcpy(theMessage.sessionId, iterMD->pPacket->frameHead.sessionID, TRDP_SESS_ID_SIZE);
                theMessage.replyTimeout = vos_ntohl(iterMD->pPacket->frameHead.replyTimeout);
                memcpy(theMessage.destURI, iterMD->pPacket->frameHead.destinationURI, TRDP_MAX_URI_USER_LEN);
                memcpy(theMessage.srcURI, iterMD->pPacket->frameHead.sourceURI, TRDP_MAX_URI_USER_LEN);
                theMessage.numExpReplies        = iterMD->numExpReplies;
                theMessage.pUserRef             = iterMD->pUserRef;
                theMessage.numReplies           = iterMD->numReplies;
                theMessage.aboutToDie           = iterMD->morituri;
                theMessage.numRepliesQuery      = iterMD->numRepliesQuery;
                theMessage.numConfirmSent       = iterMD->numConfirmSent;
                theMessage.numConfirmTimeout    = iterMD->numConfirmTimeout;

                /* theMessage.pUserRef     = appHandle->mdDefault.pRefCon; */
                theMessage.resultCode = resultCode;

                appHandle->mdDefault.pfCbFunction(
                    appHandle->mdDefault.pRefCon,
                    appHandle,
                    &theMessage,
                    (UINT8 *)0, 0);
            }
        }

        iterMD = iterMD->pNext;
    }
    while (TRUE); /*lint !e506 */

    /* Check for sockets Connection Timeouts */
    /* if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0) */
    {
        INT32 lIndex;

        for (lIndex = 0; lIndex < VOS_MAX_SOCKET_CNT; lIndex++)
        {
            if ((appHandle->iface[lIndex].sock != VOS_INVALID_SOCKET)
                && (appHandle->iface[lIndex].type == TRDP_SOCK_MD_TCP)
                && (appHandle->iface[lIndex].usage == 0)
                && (appHandle->iface[lIndex].rcvMostly == FALSE)
                && ((appHandle->iface[lIndex].tcpParams.connectionTimeout.tv_sec > 0)
                    || (appHandle->iface[lIndex].tcpParams.connectionTimeout.tv_usec > 0)))
            {
                if (0 > vos_cmpTime(&appHandle->iface[lIndex].tcpParams.connectionTimeout, &now))
                {
                    vos_printLog(VOS_LOG_INFO, "The socket (Num = %d) TIMEOUT\n", appHandle->iface[lIndex].sock);
                    appHandle->iface[lIndex].tcpParams.morituri = TRUE;
                }
            }
        }
    }

    /* Check Sending Timeouts for send() failed/incomplete sockets */
    {
        INT32 lIndex;

        for (lIndex = 0; lIndex < VOS_MAX_SOCKET_CNT; lIndex++)
        {
            if ((appHandle->iface[lIndex].sock != VOS_INVALID_SOCKET)
                && (appHandle->iface[lIndex].type == TRDP_SOCK_MD_TCP)
                && (appHandle->iface[lIndex].rcvMostly == FALSE)
                && (appHandle->iface[lIndex].tcpParams.sendNotOk == TRUE))
            {
                if (0 > vos_cmpTime(&appHandle->iface[lIndex].tcpParams.sendingTimeout, &now))
                {
                    MD_ELE_T *iterMD_find = NULL;

                    vos_printLog(VOS_LOG_INFO, "The socket (Num = %d) Sending TIMEOUT\n", appHandle->iface[lIndex].sock);

                    /* search for existing session */
                    for (iterMD_find = appHandle->pMDSndQueue; iterMD_find != NULL; iterMD_find = iterMD_find->pNext)
                    {
                        if (iterMD_find->socketIdx == lIndex)
                        {
                            iterMD_find->morituri = TRUE;

                            /* Execute callback for each session */
                            if (appHandle->mdDefault.pfCbFunction != NULL)
                            {
                                INT32           replyStatus = vos_ntohl(iterMD_find->pPacket->frameHead.replyStatus);
                                TRDP_MD_INFO_T  theMessage  = cTrdp_md_info_default;

                                theMessage.srcIpAddr    = 0;
                                theMessage.destIpAddr   = iterMD_find->addr.destIpAddr;
                                theMessage.seqCount     = vos_ntohs(iterMD_find->pPacket->frameHead.sequenceCounter);
                                theMessage.protVersion  = vos_ntohs(iterMD_find->pPacket->frameHead.protocolVersion);
                                theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(
                                        iterMD_find->pPacket->frameHead.msgType);
                                theMessage.comId        = iterMD_find->addr.comId;
                                theMessage.etbTopoCnt   = iterMD_find->addr.etbTopoCnt;
                                theMessage.opTrnTopoCnt = iterMD_find->addr.opTrnTopoCnt;
                                if (replyStatus >= 0)
                                {
                                    theMessage.userStatus   = (UINT16) replyStatus;
                                    theMessage.replyStatus  = TRDP_REPLY_OK;
                                }
                                else
                                {
                                    theMessage.userStatus   = 0;
                                    theMessage.replyStatus  = (TRDP_REPLY_STATUS_T) replyStatus;
                                }
                                memcpy(theMessage.sessionId,
                                       iterMD_find->pPacket->frameHead.sessionID,
                                       TRDP_SESS_ID_SIZE);
                                theMessage.replyTimeout = vos_ntohl(iterMD_find->pPacket->frameHead.replyTimeout);
                                memcpy(theMessage.destURI, iterMD_find->destURI, TRDP_MAX_URI_USER_LEN);
                                memcpy(theMessage.srcURI, iterMD_find->pPacket->frameHead.sourceURI, TRDP_MAX_URI_USER_LEN);
                                theMessage.numExpReplies        = iterMD_find->numReplies;
                                theMessage.pUserRef             = iterMD_find->pUserRef;
                                theMessage.numReplies           = iterMD_find->numReplies;
                                theMessage.aboutToDie           = iterMD_find->morituri;
                                theMessage.numRepliesQuery      = iterMD_find->numRepliesQuery;
                                theMessage.numConfirmSent       = iterMD_find->numConfirmSent;
                                theMessage.numConfirmTimeout    = iterMD_find->numConfirmTimeout;

                                /* theMessage.pUserRef     = appHandle->mdDefault.pRefCon; */
                                theMessage.resultCode = TRDP_TIMEOUT_ERR;

                                appHandle->mdDefault.pfCbFunction(
                                    appHandle->mdDefault.pRefCon,
                                    appHandle,
                                    &theMessage,
                                    (UINT8 *)0, 0);
                            }
                        }
                    }

                    /* Close the socket */
                    appHandle->iface[lIndex].tcpParams.morituri = TRUE;
                }
            }
        }
    }

    trdp_mdCloseSessions(appHandle, TRDP_INVALID_SOCKET_INDEX, VOS_INVALID_SOCKET, TRUE);
}

/**********************************************************************************************************************/

TRDP_ERR_T trdp_mdCommonSend (
    const TRDP_MSG_T        msgType,
    TRDP_APP_SESSION_T      appHandle,
    const void              *pUserRef,
    TRDP_UUID_T             *pSessionId,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    UINT32                  confirmTimeout,
    UINT32                  numExpReplies,
    UINT32                  replyTimeout,
    INT32                   replyStatus,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   srcURI,
    const TRDP_URI_USER_T   destURI)
{
    TRDP_ERR_T  errv = TRDP_NO_ERR;
    MD_ELE_T    *pSenderElement = NULL;
    UINT32      sequenceCounter = 0;
    BOOL8       newSession      = FALSE;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if ((pData == NULL && dataSize != 0) || dataSize > TRDP_MAX_MD_DATA_SIZE)
    {
        return TRDP_PARAM_ERR;
    }

    /* lock mutex */
    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_MUTEX_ERR;
    }

    /* set correct source IP address */
    if (srcIpAddr == 0)
    {
        srcIpAddr = appHandle->realIP;
    }

    /* mutex protected */
    do
    {
        UINT32 tmo = 0;

        switch (msgType)
        {
            /* in case of reply ... */
            case TRDP_MSG_MP: /* reply with no confirm */
            case TRDP_MSG_MQ: /* reply with confirm req. */
            case TRDP_MSG_ME: /* error */
                errv = TRDP_NOLIST_ERR;
                if (pSessionId)
                {
                    MD_ELE_T *iterMD;
                    for (iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
                    {
                        if (TRDP_ST_RX_REQ_W4AP_REPLY == iterMD->stateEle &&
                            0 == memcmp(iterMD->sessionID, pSessionId, TRDP_SESS_ID_SIZE))
                        {
                            pSenderElement = iterMD;

                            /* do not change vital parameters for reply */
                            if (NULL != iterMD->pPacket)
                            {
                                sequenceCounter = iterMD->pPacket->frameHead.sequenceCounter;
                                destIpAddr      = iterMD->addr.srcIpAddr;
                                srcIpAddr       = iterMD->addr.destIpAddr; 
                                etbTopoCnt      = iterMD->addr.etbTopoCnt;
                                opTrnTopoCnt    = iterMD->addr.opTrnTopoCnt;                                        
                                pktFlags        = iterMD->pktFlags;
                                destURI         = iterMD->srcURI;
                                srcURI          = iterMD->destURI;
                            }
                            errv = TRDP_NO_ERR;
                            break;
                        }
                    }
                }
                else
                {
                    errv = TRDP_PARAM_ERR;
                }
                break;
            /* in case of confirm ... */
            case TRDP_MSG_MC: /* confirm */
                errv = TRDP_NOLIST_ERR;
                /* vos_printLog(VOS_LOG_INFO, "MD TRDP_MSG_MC\n"); */
                if (pSessionId)
                {
                    MD_ELE_T *iterMD;
                    for (iterMD = appHandle->pMDSndQueue; iterMD != NULL; iterMD = iterMD->pNext)
                    {
                        if (TRDP_ST_TX_REQ_W4AP_CONFIRM == iterMD->stateEle &&
                            0 == memcmp(iterMD->sessionID, pSessionId, TRDP_SESS_ID_SIZE))
                        {
                            pSenderElement              = iterMD;
                            pSenderElement->dataSize    = 0;
                            pSenderElement->grossSize   = trdp_packetSizeMD(0);
                            destIpAddr                  = iterMD->addr.srcIpAddr;
                            srcIpAddr                   = iterMD->addr.destIpAddr;
                            etbTopoCnt                  = iterMD->addr.etbTopoCnt;
                            opTrnTopoCnt                = iterMD->addr.opTrnTopoCnt;
                            pktFlags                    = iterMD->pktFlags;
                            destURI                     = iterMD->srcURI;
                            srcURI                      = iterMD->destURI;

                            errv = TRDP_NO_ERR;
                            break;
                        }
                    }
                }
                else
                {
                    errv = TRDP_PARAM_ERR;
                }
                break;

            case TRDP_MSG_MN:
            case TRDP_MSG_MR:
                /* Room for MD element */
                pSenderElement = (MD_ELE_T *) vos_memAlloc(sizeof(MD_ELE_T));

                /* Reset descriptor value */
                if (NULL != pSenderElement)
                {
                    memset(pSenderElement, 0, sizeof(MD_ELE_T));
                    pSenderElement->dataSize    = dataSize;
                    pSenderElement->grossSize   = trdp_packetSizeMD(dataSize);
                    pSenderElement->socketIdx   = TRDP_INVALID_SOCKET_INDEX;
                    pSenderElement->pktFlags    =  
                        (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->mdDefault.flags : pktFlags;
                    newSession = TRUE;
                }
                else
                {
                    errv = TRDP_MEM_ERR;
                }
                break;
            default:
                errv = TRDP_PARAM_ERR;
                break;
        }


        /* if error ... */
        if (TRDP_NO_ERR != errv || NULL == pSenderElement)
        {
            break;
        }

        /*
         (Re-)allocate the data buffer if current size is different from requested size.
         If no data at all, free data pointer
         */

        if (pSenderElement->grossSize <= trdp_packetSizeMD(dataSize) ||
            NULL == pSenderElement->pPacket)
        {
            if (NULL != pSenderElement->pPacket)
            {
                vos_memFree(pSenderElement->pPacket);
                pSenderElement->pPacket = NULL;
            }
            /* allocate a buffer for the data   */
            pSenderElement->pPacket = (MD_PACKET_T *) vos_memAlloc(pSenderElement->grossSize);
            if (NULL == pSenderElement->pPacket)
            {
                vos_memFree(pSenderElement);
                pSenderElement = NULL;
                errv = TRDP_MEM_ERR;
                break;
            }
        }

        /* Prepare element data */
        pSenderElement->addr.comId      = comId;
        pSenderElement->addr.srcIpAddr  = srcIpAddr;
        pSenderElement->addr.destIpAddr = destIpAddr;
        pSenderElement->addr.mcGroup    = (vos_isMulticast(destIpAddr)) ? destIpAddr : 0;
        pSenderElement->privFlags       = TRDP_PRIV_NONE;
        pSenderElement->dataSize        = dataSize;
        pSenderElement->grossSize       = trdp_packetSizeMD(dataSize);
        pSenderElement->sendSize        = 0;
        pSenderElement->numReplies      = 0;
        pSenderElement->pCachedDS       = NULL;
        pSenderElement->morituri        = FALSE;

        /* evaluate start time and timeout. For notify I use replyTimeout as sendTimeout */
        switch (msgType)
        {
            case TRDP_MSG_MN:     /* notify (no reply)*/
                pSenderElement->interval.tv_sec     = 0;
                pSenderElement->interval.tv_usec    = 0;
                trdp_mdSetSessionTimeout(pSenderElement, tmo);
                break;
            case TRDP_MSG_MP:     /* reply without confirm */
                /* timeout has been set by received MD request */
                break;
            case TRDP_MSG_MR:     /* request with reply */
                /* Multiple replies expected only for multicast */
                if (vos_isMulticast(destIpAddr))
                {
                    pSenderElement->numExpReplies = numExpReplies;
                }
                else
                {
                    pSenderElement->numExpReplies = 1;
                }

                tmo = replyTimeout ? replyTimeout : appHandle->mdDefault.replyTimeout;     /* min time for delivery */
                pSenderElement->interval.tv_sec     = tmo / 1000000;
                pSenderElement->interval.tv_usec    = tmo % 1000000;
                trdp_mdSetSessionTimeout(pSenderElement, tmo);
                break;
            case TRDP_MSG_ME:     /* reply error */
                tmo = replyTimeout ? replyTimeout : appHandle->mdDefault.replyTimeout;     /* min time for delivery */
                pSenderElement->interval.tv_sec     = tmo / 1000000;
                pSenderElement->interval.tv_usec    = tmo % 1000000;
                trdp_mdSetSessionTimeout(pSenderElement, tmo);
                break;
            case TRDP_MSG_MQ:     /* reply with confirm */
                tmo = confirmTimeout ? confirmTimeout : appHandle->mdDefault.confirmTimeout;
                pSenderElement->interval.tv_sec     = tmo / 1000000;
                pSenderElement->interval.tv_usec    = tmo % 1000000;
                trdp_mdSetSessionTimeout(pSenderElement, tmo);
                break;
            default:
                break;
        }

        if ((pSenderElement->pktFlags & TRDP_FLAGS_TCP) != 0)
        {
            if (pSenderElement->socketIdx == TRDP_INVALID_SOCKET_INDEX)
            {
                /* socket to send TCP MD for request or notify only */
                errv = trdp_requestSocket(
                        appHandle->iface,
                        appHandle->mdDefault.tcpPort,
                        (pSendParam != NULL) ? pSendParam : (&appHandle->mdDefault.sendParam),
                        srcIpAddr,
                        0,                     /* no TCP multicast possible */
                        TRDP_SOCK_MD_TCP,
                        TRDP_OPTION_NONE,
                        FALSE,
                        VOS_INVALID_SOCKET,
                        &pSenderElement->socketIdx,
                        destIpAddr);

                if (TRDP_NO_ERR != errv)
                {
                    /* Error getting socket */
                    break;
                }
            }

            /* In the case that it is the first connection, do connect() */

            if (appHandle->iface[pSenderElement->socketIdx].usage > 0)
            {
                pSenderElement->tcpParameters.doConnect = FALSE;
            }
            else
            {
                pSenderElement->tcpParameters.doConnect = TRUE;
                appHandle->iface[pSenderElement->socketIdx].usage++;
            }

        }
        else if (TRUE == newSession && TRDP_INVALID_SOCKET_INDEX == pSenderElement->socketIdx)
        {
            /* socket to send UDP MD */
            errv = trdp_requestSocket(
                    appHandle->iface,
                    appHandle->mdDefault.udpPort,
                    (pSendParam != NULL) ? pSendParam : (&appHandle->mdDefault.sendParam),
                    srcIpAddr,
                    vos_isMulticast(destIpAddr) ? destIpAddr : 0,
                    TRDP_SOCK_MD_UDP,
                    appHandle->option,
                    FALSE,
                    VOS_INVALID_SOCKET,
                    &pSenderElement->socketIdx,
                    0);

            if (TRDP_NO_ERR != errv)
            {
                /* Error getting socket */
                break;
            }
        }

        /* arm for sending */
        switch (msgType)
        {
            case TRDP_MSG_MN:
                pSenderElement->stateEle = TRDP_ST_TX_NOTIFY_ARM;
                break;
            case TRDP_MSG_MR:
                pSenderElement->stateEle = TRDP_ST_TX_REQUEST_ARM;
                break;
            case TRDP_MSG_MP:
                pSenderElement->stateEle = TRDP_ST_TX_REPLY_ARM;
                break;
            case TRDP_MSG_MQ:
                pSenderElement->stateEle = TRDP_ST_TX_REPLYQUERY_ARM;
                break;
            case TRDP_MSG_MC:
                pSenderElement->stateEle = TRDP_ST_TX_CONFIRM_ARM;
                break;
            default:
                pSenderElement->stateEle = TRDP_ST_TX_NOTIFY_ARM;
                break;
        }

        /* already existing session ID .. */
        if (memcmp(pSenderElement->sessionID, cEmptySession, TRDP_SESS_ID_SIZE) != 0)
        {
            vos_printLog(VOS_LOG_INFO, "Using %s MD session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                         pSenderElement->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                         pSenderElement->sessionID[0], pSenderElement->sessionID[1],
                         pSenderElement->sessionID[2], pSenderElement->sessionID[3],
                         pSenderElement->sessionID[4], pSenderElement->sessionID[5],
                         pSenderElement->sessionID[6], pSenderElement->sessionID[7]);
        }
        else
        {
            /* create session ID */
            VOS_UUID_T uuid;
            vos_getUuid(uuid);

            /* return session id to caller if required */
            if (NULL != pSessionId)
            {
                memcpy(pSessionId, uuid, TRDP_SESS_ID_SIZE);
            }

            /* save session */
            memcpy(pSenderElement->sessionID, uuid, TRDP_SESS_ID_SIZE);
            vos_printLog(VOS_LOG_INFO, "Creating %s MD caller session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                         pSenderElement->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                         pSenderElement->sessionID[0], pSenderElement->sessionID[1],
                         pSenderElement->sessionID[2], pSenderElement->sessionID[3],
                         pSenderElement->sessionID[4], pSenderElement->sessionID[5],
                         pSenderElement->sessionID[6], pSenderElement->sessionID[7]);
        }

        /* Add the pUserRef */
        switch (msgType)
        {
            case TRDP_MSG_MN:
            case TRDP_MSG_MR:
            case TRDP_MSG_MC:
                if (pUserRef != NULL)
                {
                    pSenderElement->pUserRef = pUserRef;
                }
                break;
            default:
                break;
        }

        /* Prepare header */
        pSenderElement->pPacket->frameHead.sequenceCounter  = sequenceCounter;
        pSenderElement->pPacket->frameHead.protocolVersion  = vos_htons(TRDP_PROTO_VER);
        pSenderElement->pPacket->frameHead.msgType          = vos_htons((UINT16) msgType);
        pSenderElement->pPacket->frameHead.comId            = vos_htonl(comId);
        pSenderElement->pPacket->frameHead.etbTopoCnt       = vos_htonl(etbTopoCnt);
        pSenderElement->pPacket->frameHead.opTrnTopoCnt     = vos_htonl(opTrnTopoCnt);
        pSenderElement->pPacket->frameHead.datasetLength    = vos_htonl(dataSize);
        pSenderElement->pPacket->frameHead.replyStatus      = vos_htonl(replyStatus);

        if (TRDP_MSG_MN != msgType)
        {
            memcpy(pSenderElement->pPacket->frameHead.sessionID, pSenderElement->sessionID, TRDP_SESS_ID_SIZE);
            pSenderElement->pPacket->frameHead.replyTimeout = vos_htonl(tmo);
        }
        else
        {
            memset(pSenderElement->pPacket->frameHead.sessionID, 0, TRDP_SESS_ID_SIZE);
            pSenderElement->pPacket->frameHead.replyTimeout = 0;
        }


        if (srcURI != NULL)
        {
            memcpy((CHAR8 *)pSenderElement->pPacket->frameHead.sourceURI, srcURI, TRDP_MAX_URI_USER_LEN);
        }

        if (destURI != NULL)
        {
            memcpy((CHAR8 *)pSenderElement->pPacket->frameHead.destinationURI, destURI, TRDP_MAX_URI_USER_LEN);
        }

        /* payload */
        if ((pData != NULL) && (dataSize > 0) && (dataSize <= TRDP_MAX_MD_PACKET_SIZE))
        {
            /* Copy payload */
            memcpy(pSenderElement->pPacket->data, pData, dataSize);
            {
                /* zero padding (as required) */
                int ix = dataSize;
                while (0 != (ix & 3))
                {
                    pSenderElement->pPacket->data[ix++] = 0;
                }
            }
        }

        /* Insert element in send queue */
        if (TRUE == newSession)
        {
            if ((pSenderElement->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                trdp_MDqueueAppLast(&appHandle->pMDSndQueue, pSenderElement);
            }
            else
            {
                trdp_MDqueueInsFirst(&appHandle->pMDSndQueue, pSenderElement);
            }
        }

        /* change state of receiver */
        vos_printLog(VOS_LOG_INFO,
                     "MD sender element state = %d, msgType=%c%c\n",
                     pSenderElement->stateEle,
                     (char)(msgType >> 8),
                     (char)(msgType & 0xFF)
                     );
    }
    while (0);

    /* Error and allocated element ! */
    if (TRDP_NO_ERR != errv &&
        NULL != pSenderElement &&
        TRUE == newSession)
    {
        trdp_mdFreeSession(pSenderElement);
        pSenderElement = NULL;
    }

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return errv;
}
