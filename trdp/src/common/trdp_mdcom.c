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
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
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


/***********************************************************************************************************************
 * TYPEDEFS
 */


/***********************************************************************************************************************
 *   Locals
 */
 
static const TRDP_MD_INFO_T trdp_md_info_default;

/**********************************************************************************************************************/
/** Initialize the specific parameters for message data
 *  Open a listening socket.
 *
 *  @param[in]      pSessionHandle        session parameters
 *
 *  @retval         TRDP_NO_ERR           no error
 *  @retval         TRDP_PARAM_ERR        initialization error
 */
TRDP_ERR_T trdp_getTCPSocket (
    TRDP_SESSION_PT pSession)
{
    TRDP_ERR_T      result = TRDP_NO_ERR;
    VOS_SOCK_OPT_T  trdp_sock_opt;
    UINT32          backlog = 10; /* Backlog = maximum connection atempts if system is busy. */

    if (pSession->tcpFd.listen_sd == -1)     /* First time TCP is used */
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

        vos_printf(VOS_LOG_INFO, "Socket information (listen_sd=%d)\n", pSession->tcpFd.listen_sd);

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
 *  @param[in]      pPacket         pointer to the packet to check
 *  @param[in]      packetSize      size of the packet
 */
void trdp_closeMDSessions (
    TRDP_SESSION_PT appHandle)
{
    MD_ELE_T *iterMD = appHandle->pMDSndQueue;

    while (NULL != iterMD)
    {
        if (TRUE == iterMD->morituri)
        {
            trdp_releaseSocket(appHandle->iface, iterMD->socketIdx, appHandle->mdDefault.connectTimeout);
            trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD);
            vos_printf(VOS_LOG_INFO, "Freeing caller session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
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
				trdp_releaseSocket(appHandle->iface, iterMD->socketIdx, appHandle->mdDefault.connectTimeout);
			}
            trdp_MDqueueDelElement(&appHandle->pMDRcvQueue, iterMD);
            vos_printf(VOS_LOG_INFO, "Freeing replier session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
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
            packetSize > TRDP_MAX_MD_PACKET_SIZE ||
            l_datasetLength > TRDP_MAX_MD_PACKET_SIZE)
        {
            appHandle->stats.udpMd.numProtErr++;
            vos_printf(VOS_LOG_ERROR, "MDframe size error (%u)\n", packetSize);
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
        if (l_datasetLength > 0)
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
        UINT32 expectedLength = 0;

        if (l_datasetLength > 0)
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
            vos_printf(VOS_LOG_ERROR, "MDframe invalid length, received %u, expected %u\n",
                       packetSize,
                       expectedLength);
            err = TRDP_WIRE_ERR;
        }
    }
    /* check topocounter */
    if (TRDP_NO_ERR == err)
    {
		UINT32 l_tc = vos_ntohl(pPacket->topoCount);
        if (l_tc != 0 && l_tc != appHandle->topoCount)
        {
            appHandle->stats.udpMd.numTopoErr++;
            vos_printf(VOS_LOG_ERROR, "MDframe topocount error %u, expected %u\n",
                       l_tc,
                       appHandle->topoCount);
            err = TRDP_TOPO_ERR;
        }
    }
    return err;
}


/**********************************************************************************************************************/
/** Update the header values
 *
 *  @param[in]      pPacket         pointer to the packet to update
 */
void    trdp_mdUpdatePacket (
    MD_ELE_T *pElement)
{
    /* Initialize CRC calculation */
    UINT32  myCRC   = vos_crc32(0xffffffff, NULL, 0);
    UINT32  myCRC1  = myCRC;

    /* Get header and packet check sum values */
    UINT32  *hFCS   = &pElement->pPacket->frameHead.frameCheckSum;
    UINT32  *pFCS   = (UINT32 *)((UINT8 *)&pElement->pPacket->frameHead + pElement->grossSize - 4);

    /* Calculate CRC for message head */
    myCRC = vos_crc32(myCRC,
                      (UINT8 *)&pElement->pPacket->frameHead,
                      sizeof(MD_HEADER_T) - 4);
    /* Convert to Little Endian */
    *hFCS = MAKE_LE(myCRC);

    /*
     Calculate CRC for message packet
     */
    if (pElement->pPacket->frameHead.datasetLength > 0)
    {
        myCRC1 = vos_crc32(myCRC1,
                           pElement->pPacket->data,
                           vos_ntohl(pElement->pPacket->frameHead.datasetLength));
        *pFCS = MAKE_LE(myCRC1);
    }
}

/**********************************************************************************************************************/
/** Send MD packet
 *
 *  @param[in]      pdSock          socket descriptor
 *  @param[in]      port            port on which to send
 *  @param[in]      pPacket         pointer to packet to be sent
 *  @retval         != NULL         error
 */
TRDP_ERR_T  trdp_mdSendPacket (
    INT32       pdSock,
    UINT32      port,
    MD_ELE_T    *pElement)
{
    VOS_ERR_T err = VOS_NO_ERR;
    UINT32 tmpSndSize = 0;
    
    if(pElement->tcpParameters.msgUncomplete == TRUE)
    {
        tmpSndSize = pElement->sendSize;
        pElement->sendSize =  pElement->grossSize - tmpSndSize;

    }else
    {
        pElement->sendSize = pElement->grossSize;
    }
    

    if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
    {
        if(pElement->tcpParameters.msgUncomplete == FALSE)
        {
            err = vos_sockSendTCP(pdSock, (UINT8 *)&pElement->pPacket->frameHead, &pElement->sendSize);

        }else
        {
            if(tmpSndSize >= sizeof(MD_HEADER_T))
            {
                err = vos_sockSendTCP(pdSock, (UINT8 *)(&pElement->pPacket->data[(tmpSndSize - sizeof(MD_HEADER_T))]), &pElement->sendSize);
            }else
            {
                return TRDP_NODATA_ERR;
            }
        }

    }
    else
    {
        err = vos_sockSendUDP(pdSock,
                              (UINT8 *)&pElement->pPacket->frameHead,
                              &pElement->sendSize,
                              pElement->addr.destIpAddr,
                              port);
    }

    if (err != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "vos_sockSend failed (Err: %d)\n", err);
        return TRDP_IO_ERR;
    }
    
    if ((tmpSndSize + pElement->sendSize) != pElement->grossSize)
    {
        vos_printf(VOS_LOG_INFO, "vos_sockSend incomplete\n");
        return TRDP_IO_ERR;
    }


    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
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
    MD_ELE_T        *pElement)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    
    UINT32      size = 0;
    UINT32      dataSize = 0;
    UINT32      socketIndex = 0;
    UINT32      readSize = 0;
    UINT32      readDataSize = 0;
    BOOL        headerCompleted = FALSE;

    if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
    {

        /* Find the socket index */
        for (socketIndex = 0; socketIndex < VOS_MAX_SOCKET_CNT; socketIndex++)
        {
            if(appHandle->iface[socketIndex].sock == mdSock)
            {
                break;
            }
        }

        /* Read Header */
        if((appHandle->uncompletedTCP[socketIndex] == NULL)
            || ((appHandle->uncompletedTCP[socketIndex] != NULL)
            && (appHandle->uncompletedTCP[socketIndex]->grossSize < sizeof(MD_HEADER_T))))
        {
            UINT32 storedHeader = 0;

            if(appHandle->uncompletedTCP[socketIndex] == NULL)
            {
                readSize = sizeof(MD_HEADER_T);

            }else
            {
                /* If we have read some data before, read the rest */
                readSize = sizeof(MD_HEADER_T) - appHandle->uncompletedTCP[socketIndex]->grossSize;
                storedHeader = appHandle->uncompletedTCP[socketIndex]->grossSize;
            }

            err     = (TRDP_ERR_T) vos_sockReceiveTCP(mdSock, ((UINT8 *)&pElement->pPacket->frameHead) + storedHeader, &readSize);
            vos_printf(VOS_LOG_INFO, "Read Header Size = %d\n", readSize);

            /* Add the read data size to the size read before */
            size = storedHeader + readSize;

            if((appHandle->uncompletedTCP[socketIndex] != NULL)
                    && (size == sizeof(MD_HEADER_T)))
            {
                /* Uncompleted Header, completed. Save some parameters in the uncompletedTCP structure */
                appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.datasetLength = pElement->pPacket->frameHead.datasetLength;
                appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.frameCheckSum = pElement->pPacket->frameHead.frameCheckSum;
            }

        }

        /* Read Data */
        if((size >= sizeof(MD_HEADER_T)) || ((appHandle->uncompletedTCP[socketIndex] != NULL)
                && (appHandle->uncompletedTCP[socketIndex]->grossSize >= sizeof(MD_HEADER_T))))
        {

            if(appHandle->uncompletedTCP[socketIndex] == NULL)
            {
                /* Get the rest of the message length */
                dataSize = vos_ntohl(pElement->pPacket->frameHead.datasetLength) +
                      sizeof(pElement->pPacket->frameHead.frameCheckSum);

                readDataSize = dataSize;

            }else
            {
                /* Calculate the data size that is pending to read */
                size = appHandle->uncompletedTCP[socketIndex]->grossSize + readSize;
                dataSize = vos_ntohl(appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.datasetLength) + sizeof(appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.frameCheckSum);
                dataSize = dataSize - (size - sizeof(MD_HEADER_T));
                readDataSize = dataSize;

            }

            err = (TRDP_ERR_T) vos_sockReceiveTCP(mdSock, ((UINT8 *)&pElement->pPacket->frameHead) + size, &readDataSize);
            vos_printf(VOS_LOG_INFO, "Read Data Size = %d\n", readDataSize);

            /* Add the read data size */
            size = size + readDataSize;

            /* Add the read Data size to the size read during this cycle */
            readSize = readSize + readDataSize;

        }

    }else
    {
        size = TRDP_MAX_MD_PACKET_SIZE;

        err = (TRDP_ERR_T) vos_sockReceiveUDP(
                mdSock,
                (UINT8 *)&pElement->pPacket->frameHead,
                &size,
                &pElement->addr.srcIpAddr,
                &pElement->replyPort);
				
		/* my ip address is destination address */
		/* also in case of multicast ! */
		pElement->addr.destIpAddr = appHandle->realIP;
    }

    pElement->grossSize = size;

    /* If the Header is incomplete, the data size will be "0". Otherwise it will be calculated. */
    if((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
    {
        pElement->dataSize = readDataSize;

    }else
    {
        pElement->dataSize  = size - sizeof(MD_HEADER_T);
    }

    /* preliminary: this all has to be changed! */
    switch (err)
    {
        case TRDP_NODATA_ERR:
            if((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                vos_printf(VOS_LOG_INFO, "vos_sockReceiveTCP - The socket = %u has been closed \n", mdSock);
                return TRDP_NODATA_ERR;
            }
            /* no data -> rx timeout */
            return TRDP_TIMEOUT_ERR;
        case TRDP_BLOCK_ERR:
            if((((pElement->pktFlags & TRDP_FLAGS_TCP) != 0) && (readSize == 0))
                    || ((pElement->pktFlags & TRDP_FLAGS_TCP) == 0))
            {
                return TRDP_BLOCK_ERR;
            }else
            {
                break;
            }
        case TRDP_NO_ERR:
            break;
        default:
            vos_printf(VOS_LOG_ERROR, "trdp_sockReceive failed (Err: %d)\n", err);
            return err;
    }
    
    if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
    {

        UINT32      storedDataSize = 0;

        /* Compare if all the data has been read */
        if((pElement->dataSize == 0) || (pElement->dataSize != dataSize))
        {
            /* Uncompleted message received */

            if(appHandle->uncompletedTCP[socketIndex] == NULL)
            {
                /* It is the first loop, no data stored yet. Allocate memory for the message */
                appHandle->uncompletedTCP[socketIndex] = (MD_ELE_T *) vos_memAlloc(sizeof(MD_ELE_T));
                appHandle->uncompletedTCP[socketIndex]->pPacket = (MD_PACKET_T *) vos_memAlloc(sizeof(MD_PACKET_T));

                if (appHandle->uncompletedTCP[socketIndex] == NULL)
                {
                    /* vos_memDelete(NULL); */
                    vos_printf(VOS_LOG_ERROR, "vos_memAlloc() failed\n");
                    return TRDP_MEM_ERR;
                }

                storedDataSize = 0;

            }else
            {
                /* Get the size that have been already stored in the uncompletedTCP[] */
                storedDataSize = appHandle->uncompletedTCP[socketIndex]->grossSize;
            }

            if(readSize > 0)
            {
                /* Copy the read data in the uncompletedTCP[] */
               memcpy(((UINT8 *)&appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead) + storedDataSize, ((UINT8 *)&pElement->pPacket->frameHead) + storedDataSize, readSize);
               appHandle->uncompletedTCP[socketIndex]->grossSize = pElement->grossSize;
               appHandle->uncompletedTCP[socketIndex]->dataSize = pElement->dataSize;

            }else
            {
               return TRDP_PARAM_ERR;
            }

            return TRDP_IO_ERR;

        }else
        {
            /* Complete message */
            /* All data is read. Save all the data and copy to the pElement to continue */

            if(appHandle->uncompletedTCP[socketIndex] != NULL)
            {
                /* Add the received information and copy all the data to the pElement */

                storedDataSize = appHandle->uncompletedTCP[socketIndex]->grossSize;

                if((readSize > 0))
                {
                    memcpy(((UINT8 *)&appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead) + storedDataSize, ((UINT8 *)&pElement->pPacket->frameHead) + storedDataSize, readSize);

                    /* Copy all the uncompletedTCP data to the pElement */
                    memcpy(((UINT8 *)&pElement->pPacket->frameHead), ((UINT8 *)&appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead), pElement->grossSize);

                    /* Disallocate the memory */
                    vos_memFree(appHandle->uncompletedTCP[socketIndex]);
                    appHandle->uncompletedTCP[socketIndex] = NULL;

                }else
                {
                    return TRDP_PARAM_ERR;
                }
            }

        }

    }
    
    /* received data */
    err = trdp_mdCheck(appHandle, &pElement->pPacket->frameHead, pElement->grossSize);

    /*  Update statistics   */
    switch (err)
    {
        case TRDP_NO_ERR:
            if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                appHandle->stats.tcpMd.numRcv++;
                ;
            }
            else
            {
                appHandle->stats.udpMd.numRcv++;
                ;
            }
            break;
        case TRDP_CRC_ERR:
            if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                appHandle->stats.tcpMd.numCrcErr++;
            }
            else
            {
                appHandle->stats.udpMd.numCrcErr++;
            }
            return err;
        case TRDP_WIRE_ERR:
            if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                appHandle->stats.tcpMd.numProtErr++;
            }
            else
            {
                appHandle->stats.udpMd.numProtErr++;
            }
            return err;
        default:
            return err;
    }

    if (err != TRDP_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "trdp_mdCheck failed (Err: %d)\n", err);
        return TRDP_IO_ERR;
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
    TRDP_ERR_T result = TRDP_NO_ERR;
    MD_HEADER_T *pH = NULL;
    MD_ELE_T            *iterMD         = NULL;
    MD_LIS_ELE_T        *iterListener   = NULL;
    TRDP_MD_ELE_ST_T    state;
    BOOL                isTCP           = FALSE;
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
            appHandle->pMDRcvEle->pPacket   = (MD_PACKET_T *) vos_memAlloc(TRDP_MAX_MD_PACKET_SIZE);
            appHandle->pMDRcvEle->pktFlags  = appHandle->mdDefault.flags;
            if (NULL == appHandle->pMDRcvEle->pPacket)
            {
                vos_memFree(appHandle->pMDRcvEle);
                appHandle->pMDRcvEle = NULL;
                vos_printf(VOS_LOG_ERROR, "Receiving MD: Out of receive buffers!\n");
                return TRDP_MEM_ERR;
            }
        }
        else
        {
            vos_printf(VOS_LOG_ERROR, "Receiving MD: Out of receive buffers!\n");
            return TRDP_MEM_ERR;
        }
    }

    if (appHandle->pMDRcvEle->pPacket == NULL)
    {
        appHandle->pMDRcvEle->pPacket = (MD_PACKET_T *) vos_memAlloc(TRDP_MAX_MD_PACKET_SIZE);
        if (appHandle->pMDRcvEle->pPacket == NULL)
        {
            vos_printf(VOS_LOG_ERROR, "Receiving MD: Out of receive buffers!\n");
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
        appHandle->pMDRcvEle->pktFlags &= ~TRDP_FLAGS_TCP;
        isTCP = FALSE;
    }

    /* get packet: */

    result = trdp_mdRecvPacket(appHandle, appHandle->iface[sockIndex].sock, appHandle->pMDRcvEle);

    if (result != TRDP_NO_ERR)
    {
        return result;
    }

    /* process message */
    pH = &appHandle->pMDRcvEle->pPacket->frameHead;

    vos_printf(VOS_LOG_INFO,
               "Received MD packet (type: '%c%c' UUID: %02x%02x%02x%02x%02x%02x%02x%02x Data len: %u)\n",
               appHandle->pMDRcvEle->pPacket->frameHead.msgType & 0xFF,
               appHandle->pMDRcvEle->pPacket->frameHead.msgType >> 8,
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
                    if (TRDP_ST_RX_REQ_W4AP_REPLY == iterMD->stateEle)
                    {
                        /* Discard message  */
                        vos_printf(VOS_LOG_INFO, "Receiving MD: Duplicate Request (app reply pending)!\n");
                        return TRDP_NO_ERR;
                    }
                    if (iterMD->curSeqCnt == vos_ntohl(pH->sequenceCounter))
                    {
                        /* Discard message  */
                        vos_printf(VOS_LOG_INFO, "Receiving MD: Duplicate Request (sequence counter identical)!\n");
                        return TRDP_NO_ERR;
                    }
                    if (TRDP_ST_RX_REPLY_SENT == iterMD->stateEle)
                    {
                        /* Resend reply message  */
                        vos_printf(VOS_LOG_INFO, "Receiving MD: Duplicate Request (resend reply)!\n");
                        iterMD->stateEle = TRDP_ST_TX_REPLY_ARM;
                        /* TODO? repeatReply(); */
                        return TRDP_NO_ERR;
                    }
                }
            }

            if (appHandle->mdDefault.maxNumSessions <= numOfReceivers)
            {
                /* Discard MD request, we shall not be flooded by incoming requests */
                vos_printf(VOS_LOG_INFO, "Receiving MD: Max. number of requests reached (%d)!\n", numOfReceivers);
                return TRDP_NO_ERR;
            }

            /* search for existing listener */
            for (iterListener = appHandle->pMDListenQueue; iterListener != NULL; iterListener = iterListener->pNext)
            {
                if (iterListener->socketIdx != -1 && isTCP == TRUE)
                {
                    continue;
                }
				if (iterListener->addr.mcGroup == 0 && iterListener->addr.destIpAddr != 0 && iterListener->addr.destIpAddr != appHandle->pMDRcvEle->addr.srcIpAddr)
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
                    iterMD = appHandle->pMDRcvEle;
                    iterMD->pUserRef    = iterListener->pUserRef;
                    iterMD->stateEle    = state;

                    if (iterListener->socketIdx == -1)    /* On TCP, listeners have no socket assigned  */
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

                    vos_printf(VOS_LOG_INFO, "Creating receive session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
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
                vos_printf(VOS_LOG_INFO, "Receiving MD: No listener found!\n");
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
                    iterMD->addr.topoCount  = vos_ntohl(pH->topoCount);
                    iterMD->addr.srcIpAddr  = appHandle->pMDRcvEle->addr.srcIpAddr;
                    iterMD->addr.destIpAddr = appHandle->pMDRcvEle->addr.destIpAddr;
                    memcpy(iterMD->destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);
                    iterMD->stateEle    = TRDP_ST_RX_CONF_RECEIVED;
                    iterMD->morituri    = TRUE;
                    vos_printf(VOS_LOG_INFO, "Received Confirmation, session will be closed!\n");
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
                    iterMD->addr.comId      = vos_ntohl(pH->comId);
                    iterMD->addr.topoCount  = vos_ntohl(pH->topoCount);
                    iterMD->addr.srcIpAddr  = appHandle->pMDRcvEle->addr.srcIpAddr;
                    iterMD->addr.destIpAddr = appHandle->pMDRcvEle->addr.destIpAddr;
                    memcpy(iterMD->destURI, pH->destinationURI, TRDP_MAX_URI_USER_LEN);

                    if (TRDP_MSG_MP == vos_ntohs(pH->msgType))
                    {
                        iterMD->stateEle = TRDP_ST_TX_REPLY_RECEIVED;
                        iterMD->numReplies++;

                        /* Handle multiple replies
                         Close session if number of expected replies reached */
                        if (iterMD->noOfRepliers != 0 &&
                            (iterMD->noOfRepliers == 1 || iterMD->noOfRepliers == iterMD->numReplies))
                        {
                            /* Prepare for session fin, Reply/ReplyQuery reception only one expected */
                            iterMD->morituri = TRUE;
                        }
                        else
                        {
                            /* Handle multiple unknown replies */
                            /* Send element removed by timeout because number of repliers is unknown */
                        }
                    }
                    else
                    {
                        /* Increment number of ReplyQuery received, used to count number of
                         expected Confirms sent */
                        iterMD->numRepliesQuery++;

                        iterMD->stateEle = TRDP_ST_TX_REQ_W4AP_CONFIRM;

                    }

                    /* receive time */
                    vos_getTime(&iterMD->timeToGo);

                    /* timeout value */
                    iterMD->interval.tv_sec     = vos_ntohl(pH->replyTimeout) / 1000000;
                    iterMD->interval.tv_usec    = vos_ntohl(pH->replyTimeout) % 1000000;
                    vos_addTime(&iterMD->timeToGo, &iterMD->interval);

                    break;
                }
            }
            break;
    }

    /* Inform user  */
    if (NULL != iterMD && appHandle->mdDefault.pfCbFunction != NULL)
    {
        TRDP_MD_INFO_T theMessage = trdp_md_info_default;

        theMessage.srcIpAddr    = iterMD->addr.srcIpAddr;
        theMessage.destIpAddr   = iterMD->addr.destIpAddr;
        theMessage.seqCount     = vos_ntohl(iterMD->pPacket->frameHead.sequenceCounter);
        theMessage.protVersion  = vos_ntohs(iterMD->pPacket->frameHead.protocolVersion);
        theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(iterMD->pPacket->frameHead.msgType);
        theMessage.comId        = vos_ntohl(iterMD->pPacket->frameHead.comId);
        theMessage.topoCount    = vos_ntohl(iterMD->pPacket->frameHead.topoCount);
        theMessage.userStatus   = 0;
        theMessage.replyStatus  = (TRDP_REPLY_STATUS_T) vos_ntohs(iterMD->pPacket->frameHead.replyStatus);
        theMessage.replyTimeout = vos_ntohl(iterMD->pPacket->frameHead.replyTimeout);
        memcpy(theMessage.sessionId, iterMD->pPacket->frameHead.sessionID, TRDP_SESS_ID_SIZE);
        memcpy(theMessage.destURI, iterMD->pPacket->frameHead.destinationURI, TRDP_MAX_URI_USER_LEN);
        memcpy(theMessage.srcURI, iterMD->pPacket->frameHead.sourceURI, TRDP_MAX_URI_USER_LEN);
        theMessage.numExpReplies        = iterMD->noOfRepliers;
        theMessage.numReplies           = iterMD->numReplies;
        theMessage.numRetriesMax        = iterMD->numRetriesMax;
        theMessage.numRetries           = iterMD->numRetries;
        theMessage.aboutToDie           = iterMD->morituri;
        theMessage.numRepliesQuery      = iterMD->numRepliesQuery;
        theMessage.numConfirmSent       = iterMD->numConfirmSent;
        theMessage.numConfirmTimeout    = iterMD->numConfirmTimeout;
        theMessage.resultCode           = TRDP_NO_ERR;
        theMessage.pUserRef = iterMD->pUserRef;

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
    BOOL        firstLoop   = TRUE;

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
               dotx        = 1;
               nextstate   = TRDP_ST_RX_REPLY_SENT;
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
           if (iterMD->socketIdx != -1 && (!appHandle->beQuiet || (iterMD->privFlags & TRDP_REDUNDANT)))
           {
               trdp_mdUpdatePacket(iterMD);

               if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
               {
                   if(iterMD->tcpParameters.doConnect == TRUE)
                   {
                       VOS_ERR_T err;
                       /* Connect() the socket */
                       err = vos_sockConnect(appHandle->iface[iterMD->socketIdx].sock,
                                             iterMD->addr.destIpAddr, appHandle->mdDefault.tcpPort);

                       /* Add the socket in the file descriptor*/
                       appHandle->iface[iterMD->socketIdx].tcpParams.addFileDesc = TRUE;

                       if (err == VOS_NO_ERR)
                       {
                           iterMD->tcpParameters.doConnect = FALSE;
                       }
                       else if (err == VOS_BLOCK_ERR)
                       {
                           vos_printf(VOS_LOG_INFO, "Socket connection for TCP not ready!\n");
                           iterMD->tcpParameters.doConnect = FALSE;
                           iterMD = iterMD->pNext;
                           continue;
                       }
                       else
                       {
                           /* ToDo: What in case of a permanent failure? */
                           vos_printf(VOS_LOG_ERROR, "Socket connection for TCP failed!\n");

                           if(appHandle->iface[iterMD->socketIdx].tcpParams.sendNotOk == FALSE)
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
                       }

                       if (nextstate == TRDP_ST_RX_REPLY_SENT ||
                           nextstate == TRDP_ST_RX_REPLYQUERY_W4C)
                       {
                           /* Update timeout */
                           vos_getTime(&iterMD->timeToGo);
                           vos_addTime(&iterMD->timeToGo, &iterMD->interval);
                       }

                       appHandle->stats.udpMd.numSend++;

                       if (nextstate == TRDP_ST_NONE)
                       {

                           /* Notify, Reply, Reply Error or Confirm messsage */
                           if (iterMD->stateEle == TRDP_ST_TX_CONFIRM_ARM)
                           {
                               iterMD->numConfirmSent++;
                               if (iterMD->numConfirmSent == iterMD->numRepliesQuery)
                               {
                                   iterMD->morituri = TRUE;
                               }
                           }
                           else if (iterMD->stateEle == TRDP_ST_TX_NOTIFY_ARM)
                           {
                               iterMD->stateEle = nextstate;
                               iterMD->morituri = TRUE;
                           }

                           /*if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                           {
                               appHandle->iface[iterMD->socketIdx].usage--;

                               vos_printf(VOS_LOG_INFO,
                                          "Socket (Num = %d) usage decremented to (Num = %d)\n",
                                          appHandle->iface[iterMD->socketIdx].sock,
                                          appHandle->iface[iterMD->socketIdx].usage);

                               /* If there is no at least one session using the socket, start the socket connectionTimeout
                                */
                              /* if((appHandle->iface[iterMD->socketIdx].usage == 0)
                                  && (appHandle->iface[iterMD->socketIdx].rcvMostly == FALSE)
                                  && ((iterMD->stateEle == TRDP_ST_TX_NOTIFY_ARM)
                                      || (iterMD->stateEle == TRDP_ST_TX_CONFIRM_ARM)))
                               {
                                   vos_printf(VOS_LOG_INFO,
                                              "The Socket (Num = %d usage=0) ConnectionTimeout will be started\n",
                                              appHandle->iface[iterMD->socketIdx].sock);

                                   /* Start the socket connectionTimeout */
                                /*   {
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
                           }*/

                       }
                       else
                       {
                           iterMD->stateEle = nextstate;
                       }
                   }else
                   {
                       if(result == TRDP_IO_ERR)
                       {
                           /* Send uncompleted */
                           if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                           {
                               appHandle->iface[iterMD->socketIdx].tcpParams.notSend = TRUE;
                               iterMD->tcpParameters.msgUncomplete = TRUE;

                               if(appHandle->iface[iterMD->socketIdx].tcpParams.sendNotOk == FALSE)
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
                       }else
                       {
                           if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                           {
                               if(appHandle->iface[iterMD->socketIdx].tcpParams.sendNotOk == FALSE)
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
                   }
               }
           }
       }
       iterMD = iterMD->pNext;

   }
   while (1);


   trdp_closeMDSessions(appHandle);

   return result;
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
    INT32       highDesc = -1;
    INT32       index;
    TRDP_ERR_T  err;
    INT32       new_sd  = -1;
    MD_ELE_T    *iterMD = NULL;

    if (appHandle == NULL)
    {
        return;
    }

    /*  If no descriptor set was supplied, we use our own. We set all used descriptors as readable.
        This eases further handling of the sockets  */

    if (pRfds == NULL)
    {
        /* polling mode */
        FD_ZERO((fd_set *)&rfds);

        /* Add the listen_sd in the file descriptor */
        if (appHandle->tcpFd.listen_sd != -1)
        {
            FD_SET(appHandle->tcpFd.listen_sd, (fd_set *)&rfds);
            if (appHandle->tcpFd.listen_sd > highDesc)
            {
                highDesc = appHandle->tcpFd.listen_sd;
            }
        }

        /* scan for sockets */
        for (index = 0; index < VOS_MAX_SOCKET_CNT; index++)
        {
            if (appHandle->iface[index].sock != -1 &&
                appHandle->iface[index].type != TRDP_SOCK_PD
                && ((appHandle->iface[index].type != TRDP_SOCK_MD_TCP)
                || ((appHandle->iface[index].type == TRDP_SOCK_MD_TCP)
                && (appHandle->iface[index].tcpParams.addFileDesc == TRUE))))
            {
                FD_SET(appHandle->iface[index].sock, (fd_set *)&rfds);
                if (highDesc < appHandle->iface[index].sock)
                {
                    highDesc = appHandle->iface[index].sock;
                }
            }
        }
        if (highDesc == -1)
        {
            return;
        }
        noOfDesc = vos_select(highDesc + 1, &rfds, NULL, NULL, NULL);
        if (noOfDesc == -1)
        {
            vos_printf(VOS_LOG_ERROR, "select() failed\n");
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

        /* vos_printf(VOS_LOG_INFO, " ----- CHECKING READY DESCRIPTORS -----\n"); */

        if (appHandle->tcpFd.listen_sd != -1 &&
            FD_ISSET(appHandle->tcpFd.listen_sd, (fd_set *)pRfds))
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

                newIp = appHandle->realIP;
                read_tcpPort = appHandle->mdDefault.tcpPort;

                err = (TRDP_ERR_T) vos_sockAccept(appHandle->tcpFd.listen_sd,
                                                  &new_sd, &newIp,
                                                  &(read_tcpPort));

                if (new_sd < 0)
                {
                    if (err == TRDP_NO_ERR)
                    {
                        /* vos_printf(VOS_LOG_INFO, "TRDP no more connections to accept\n"); */
                        break;
                    }
                    else
                    {
                        vos_printf(VOS_LOG_ERROR, "TRDP vos_sockAccept() failed (Err:%d)\n", err);

                        /* Callback the error to the application  */
                        if (appHandle->mdDefault.pfCbFunction != NULL)
                        {
                            TRDP_MD_INFO_T theMessage = trdp_md_info_default;

                            theMessage.topoCount    = appHandle->topoCount;
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
                    vos_printf(VOS_LOG_INFO, "Accepting socket %d\n", new_sd);
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
                    BOOL    socketFound = FALSE;

                    for (socketIndex = 0; socketIndex < VOS_MAX_SOCKET_CNT; socketIndex++)
                    {
                        if ((appHandle->iface[socketIndex].sock != -1)
                            && (appHandle->iface[socketIndex].type == TRDP_SOCK_MD_TCP)
                            && (appHandle->iface[socketIndex].tcpParams.cornerIp == newIp)
                            && (appHandle->iface[socketIndex].rcvMostly == TRUE))
                        {
                            vos_printf(VOS_LOG_INFO, "New socket accepted from the same device (Ip = %u)\n", newIp);

                            if(appHandle->iface[socketIndex].usage > 0)
                            {
                                vos_printf(VOS_LOG_INFO, "The new socket accepted from the same device (Ip = %u), won't be removed, because it is still using\n", newIp);
                                socketFound = TRUE;
                                break;
                            }


                            /* Delete the items from SndQueue that are using the old socket */
                            for (iterMD = appHandle->pMDSndQueue; iterMD != NULL; iterMD = iterMD->pNext)
                            {
                                if (iterMD->socketIdx == socketIndex)
                                {
                                    /* Remove element from queue */
                                    trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD);

                                    /* free element */
                                    trdp_mdFreeSession(iterMD);
                                }
                            }

                            /* Clean the session in the listener */
                            for (iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
                            {
                                if (iterMD->socketIdx == socketIndex)
                                {
                                    iterMD->stateEle    = TRDP_ST_RX_READY;
                                    iterMD->socketIdx   = -1;
                                }
                            }

                            /* Close the old socket */
                            err = (TRDP_ERR_T) vos_sockClose(appHandle->iface[socketIndex].sock);
                            if (err != TRDP_NO_ERR)
                            {
                                vos_printf(VOS_LOG_ERROR, "vos_sockClose() failed (Err:%d)\n", err);
                            }

                            vos_printf(VOS_LOG_INFO,
                                       "The old socket (Socket = %d, Ip = %u) will be closed\n",
                                       appHandle->iface[socketIndex].sock,
                                       appHandle->iface[socketIndex].tcpParams.cornerIp);


                            if (FD_ISSET(appHandle->iface[socketIndex].sock, (fd_set *) pRfds))
                            {
                                /* Decrement the Ready descriptors counter */
                                (*pCount)--;
                                FD_CLR(appHandle->iface[socketIndex].sock, (fd_set *) pRfds);
                            }

                            /* Replace the old socket by the new one */
                            appHandle->iface[socketIndex].sock         = new_sd;
                            appHandle->iface[socketIndex].rcvMostly    = TRUE;
                            appHandle->iface[socketIndex].tcpParams.connectionTimeout.tv_sec   = 0;
                            appHandle->iface[socketIndex].tcpParams.connectionTimeout.tv_usec  = 0;
                            appHandle->iface[socketIndex].tcpParams.notSend = FALSE;
                            appHandle->iface[socketIndex].tcpParams.cornerIp = newIp;
                            appHandle->iface[socketIndex].type     = TRDP_SOCK_MD_TCP;
                            appHandle->iface[socketIndex].usage    = 0;
                            appHandle->iface[socketIndex].tcpParams.sendNotOk = FALSE;
                            appHandle->iface[socketIndex].tcpParams.addFileDesc = TRUE;

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

    }

    /* Check Receive Data (UDP & TCP) */
    /*  Loop through the socket list and check readiness
        (but only while there are ready descriptors left) */

    for (index = 0; index < VOS_MAX_SOCKET_CNT; index++)
    {
        if (appHandle->iface[index].sock != -1 &&
            appHandle->iface[index].type != TRDP_SOCK_PD &&
            FD_ISSET(appHandle->iface[index].sock, (fd_set *)pRfds) != 0)
        {
            if (pCount != NULL)
            {
                (*pCount)--;
                if (*pCount < 0)
                {
                    break;
                }
            }
            FD_CLR(appHandle->iface[index].sock, (fd_set *)pRfds);
            err = trdp_mdRecv(appHandle, index);

            /* The receive message is incomplete */
            if(err == TRDP_IO_ERR && appHandle->iface[index].type == TRDP_SOCK_MD_TCP)
            {
                vos_printf(VOS_LOG_INFO, "Incomplete message received \n");
            }

            /* Check if the socket has been closed in the other corner */
            if (err == TRDP_NODATA_ERR && appHandle->iface[index].type == TRDP_SOCK_MD_TCP)
            {
                MD_ELE_T *iterMD_find;

                /* Code removed:
                    Manual closing a socket should not be necessary!
                    Sockets are closed when the last user (session) was closed */

                /* Delete the sessions that are opened in this socket */
                for (iterMD_find = appHandle->pMDSndQueue;
                     iterMD_find != NULL;
                     iterMD_find = iterMD_find->pNext)
                {
                    if (iterMD_find->socketIdx == index)
                    {
                        /*This session is using the socket */
                        trdp_releaseSocket(appHandle->iface, iterMD_find->socketIdx, appHandle->mdDefault.connectTimeout);
                        /* Remove element from queue */
                        trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD_find);

                        /* free element */
                        trdp_mdFreeSession(iterMD_find);
                    }
                }

                /* Clean the session in the listener */
                for (iterMD_find = appHandle->pMDRcvQueue;
                     iterMD_find != NULL;
                     iterMD_find = iterMD_find->pNext)
                {
                    if (iterMD_find->socketIdx == index)
                    {
                        /*This session was using the socket */
                        trdp_releaseSocket(appHandle->iface, iterMD_find->socketIdx, appHandle->mdDefault.connectTimeout);

                        /* Remove element from queue */
                        trdp_MDqueueDelElement(&appHandle->pMDRcvQueue, iterMD_find);

                        /* free element */
                        trdp_mdFreeSession(iterMD_find);

                    }
                }
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
    BOOL        firstLoop   = TRUE;
    BOOL        timeOut     = FALSE;
    TRDP_TIME_T now;

    if (appHandle == NULL)
    {
        return;
    }


    /*  Find the sessions which needs action
     Note: We must also check the receive queue for pending replies! */

    do
    {
        TRDP_ERR_T  resultCode = TRDP_UNKNOWN_ERR;
        INT32       sndReplyTimeout     = 0;
        INT32       sndConfirmTimeout   = 0;

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

        /* Update the current time always inside loop in case of application delays  */
        vos_getTime(&now);

        /* timeToGo is timeout value! */

        if (0 > vos_cmpTime(&iterMD->timeToGo, &now))     /* timeout overflow */
        {
            /* timeout on queue ? */
            switch (iterMD->stateEle)
            {
                /* Replier waiting for reply from application */
                case TRDP_ST_RX_REQ_W4AP_REPLY:

                /* Caller waiting for a Confirmation from application*/
                case TRDP_ST_TX_REQ_W4AP_CONFIRM:
                {
                    vos_printf(VOS_LOG_ERROR, "MD Reply/Confirm timeout: fall back ARM st=%d\n", iterMD->stateEle);
                    iterMD->morituri = TRUE;

                    if (iterMD->stateEle == TRDP_ST_TX_REQ_W4AP_CONFIRM)
                    {
                        /* Increment number of Confirm Timeouts */
                        iterMD->numConfirmTimeout++;
                        resultCode = TRDP_APP_CONFIRMTO_ERR;
                    }
                    else
                    {
                        resultCode = TRDP_APP_REPLYTO_ERR;
                    }

                    timeOut = TRUE;

                    /* TCP handling */
                    /*if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                    {
                        MD_ELE_T *iterMD_find;

                        /* Delete the sessions that are opened in this socket */
                       /* for (iterMD_find = appHandle->pMDSndQueue;
                             iterMD_find != NULL;
                             iterMD_find = iterMD_find->pNext)
                        {
                            if (vos_ntohl(iterMD_find->pPacket->frameHead.comId) == iterMD->addr.comId)
                            {
                                /*This session is using the socket */
                                /* Remove element from queue */
                         //       trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD_find);

                                /* free element */
                       /*         trdp_mdFreeSession(iterMD);
                            }
                        }

/* TBD: */
                        /*  Decrementing usage will prevend releasing of socket!    * / */
                        /* appHandle->iface[iterMD->socketIdx].usage--;

                        vos_printf(VOS_LOG_INFO,
                                   "Socket (Num = %d) usage decremented to %d\n",
                                   appHandle->iface[iterMD->socketIdx].sock,
                                   appHandle->iface[iterMD->socketIdx].usage);
                            */
                        /* If there is not at least one session using the socket, start the socket connectionTimeout
                         */
                        /*
                           if ((appHandle->iface[iterMD->socketIdx].usage == 0)
                              && (appHandle->iface[iterMD->socketIdx].rcvMostly == FALSE))
                         */
                        /*{
                            TRDP_TIME_T tmpt_interval, tmpt_now;

                            vos_printf(VOS_LOG_INFO,
                                       "The Socket (Num = %d usage=0) ConnectionTimeout will be started\n",
                                       appHandle->iface[iterMD->socketIdx].sock);

                            /* Start the socket connectionTimeout */
                           /* tmpt_interval.tv_sec    = appHandle->mdDefault.connectTimeout / 1000000;
                            tmpt_interval.tv_usec   = appHandle->mdDefault.connectTimeout % 1000000;

                            vos_getTime(&tmpt_now);
                            vos_addTime(&tmpt_now, &tmpt_interval);

                            memcpy(&appHandle->iface[iterMD->socketIdx].tcpParams.connectionTimeout,
                                   &tmpt_now,
                                   sizeof(TRDP_TIME_T));
                        }
                    }*/
                }
                break;

                /* Waiting for Reply/ReplyQuery reception and Confirm sent */
                case TRDP_ST_TX_REQUEST_W4REPLY:
                {

                    if((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                    {
                        iterMD->morituri = TRUE;
                        break;
                    }

                    /* Manage Reply/ReplyQuery reception */
                    /* Note: in case all expected Reply/ReplyQuery are received, disableReplyRx is set by
                     trdp_mdRecv() */
                    if (iterMD->morituri == FALSE)
                    {
                        /* Session is in reception phase */

                        /* Check for Reply timeout */

                        vos_printf(VOS_LOG_INFO, "MD reply timeout.\n");

                        /* Increment number of reties */
                        iterMD->numRetries++;

                        /* Handle UDP retries for single reply expected */
                        if (iterMD->noOfRepliers == 1 &&                   /* Single reply expected */
                            iterMD->numRetriesMax > 0 &&                   /* Retries allowed */
                            iterMD->numRetries <= iterMD->numRetriesMax)   /* Retries below maximum allowed */
                        {
                            /* Update timeout */
                            vos_getTime(&iterMD->timeToGo);
                            vos_addTime(&iterMD->timeToGo, &iterMD->interval);

                            /* Re-arm sending request */
                            iterMD->stateEle = TRDP_ST_TX_REQUEST_ARM;

                            /* Increment counter with each telegram */
                            iterMD->pPacket->frameHead.sequenceCounter =
                                vos_htonl(vos_ntohl(iterMD->pPacket->frameHead.sequenceCounter) + 1);
                        }
                        else
                        {
                            /* Remove sender after callback */

                            /* Reply timeout raised, stop Reply/ReplyQuery reception */
                            iterMD->morituri = TRUE;
                        }

                        /* Callback execution required to indicate this event */
                        sndReplyTimeout = 1;
                        resultCode      = TRDP_REPLYTO_ERR;

                        /* Statistics */

                        if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                        {
                            appHandle->stats.udpMd.numReplyTimeout++;
                        }
                        else
                        {
                            appHandle->stats.tcpMd.numReplyTimeout++;
                        }
                    }

                    /* Manage send Confirm */
                    /* if (iterMD->disableReplyRx == 1) */
                    {
                        /* All expected Reply/ReplyQuery received or Reply Timeout triggered */

                        /* Check Confirm sent status */
                        if ((iterMD->numRepliesQuery == 0) || (iterMD->numRepliesQuery == iterMD->numConfirmSent))
                        {
                            /* All Confirm required by received ReplyQuery are sent */
                            /* vos_printf(VOS_LOG_INFO, "DEBUG tlc_process(): MD send done, all Confirm sent\n"); */
                            iterMD->morituri = TRUE;
                        }
                        else
                        {
                            /* Check for pending Confirm timeout (handled in each single listener) */
                            if (iterMD->numRepliesQuery <= (iterMD->numConfirmSent + iterMD->numConfirmTimeout))
                            {
                                /* Callback execution require to indicate send done with some Confirm Timeout */
                                /* vos_printf(VOS_LOG_INFO, "DEBUG tlc_process(): MD send done, Confirm timeout\n"); */
                                sndConfirmTimeout   = 1;
                                resultCode          = TRDP_REQCONFIRMTO_ERR;
                                iterMD->morituri    = TRUE;
                            }
                        }
                    }

                    if (sndReplyTimeout == 1 || sndConfirmTimeout == 1)
                    {
                        timeOut = TRUE;
                    }

                    /* TCP handling of ReplyTimeout */
                    /*if (0) /* if (sndReplyTimeout == 1) */
                    /*{
                        if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                        {
                            MD_ELE_T *iterMD_find;

                            /* Clean the session in the listener */
                     /*       for (iterMD_find = appHandle->pMDRcvQueue;
                                 iterMD_find != NULL;
                                 iterMD_find = iterMD_find->pNext)
                            {
                                if (vos_ntohl(iterMD->pPacket->frameHead.comId) == iterMD_find->addr.comId)
                                {
                                    iterMD_find->stateEle   = TRDP_ST_RX_READY;
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
                        /*    if ((appHandle->iface[iterMD->socketIdx].usage == 0)
                                && (appHandle->iface[iterMD->socketIdx].rcvMostly == FALSE))
                            {
                                TRDP_TIME_T tmpt_interval, tmpt_now;

                                vos_printf(VOS_LOG_INFO,
                                           "The Socket (Num = %d usage=0) ConnectionTimeout will be started\n",
                                           appHandle->iface[iterMD->socketIdx].sock);

                                /*  Start the socket connectionTimeout */
                          /*      tmpt_interval.tv_sec    = appHandle->mdDefault.connectTimeout / 1000000;
                                tmpt_interval.tv_usec   = appHandle->mdDefault.connectTimeout % 1000000;

                                vos_getTime(&tmpt_now);
                                vos_addTime(&tmpt_now, &tmpt_interval);

                                memcpy(&appHandle->iface[iterMD->socketIdx].tcpParams.connectionTimeout,
                                       &tmpt_now,
                                       sizeof(TRDP_TIME_T));
                            }
                        }
                    }*/
                }
                break;
                case TRDP_ST_RX_REPLYQUERY_W4C:
                {
                    timeOut = TRUE;

                    resultCode = TRDP_CONFIRMTO_ERR;
                    /* TCP handling */
                    /**if (0) /* ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0) */
                    /*{
                        MD_ELE_T *iterMD_find;

                        /* Clean the session in the listener */
                      /*  for (iterMD_find = appHandle->pMDRcvQueue;
                             iterMD_find != NULL;
                             iterMD_find = iterMD_find->pNext)
                        {
                            if (vos_ntohl(iterMD->pPacket->frameHead.comId) == iterMD_find->addr.comId)
                            {
                                iterMD_find->stateEle   = TRDP_ST_RX_READY;
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
                     /*   if ((appHandle->iface[iterMD->socketIdx].usage == 0)
                            && (appHandle->iface[iterMD->socketIdx].rcvMostly == FALSE))
                        {
                            TRDP_TIME_T tmpt_interval, tmpt_now;

                            vos_printf(VOS_LOG_INFO,
                                       "The Socket (Num = %d usage=0) ConnectionTimeout will be started\n",
                                       appHandle->iface[iterMD->socketIdx].sock);

                            /*  Start the socket connectionTimeout */
                        /*    tmpt_interval.tv_sec    = appHandle->mdDefault.connectTimeout / 1000000;
                            tmpt_interval.tv_usec   = appHandle->mdDefault.connectTimeout % 1000000;

                            vos_getTime(&tmpt_now);
                            vos_addTime(&tmpt_now, &tmpt_interval);

                            memcpy(&appHandle->iface[iterMD->socketIdx].tcpParams.connectionTimeout,
                                   &tmpt_now,
                                   sizeof(TRDP_TIME_T));
                        }
                    }*/

                    /* Statistics */
                    if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                    {
                        appHandle->stats.tcpMd.numConfirmTimeout++;
                    }
                    else
                    {
                        appHandle->stats.udpMd.numConfirmTimeout++;
                    }

                    /* free element */
                    iterMD->morituri = TRUE;

                    vos_printf(VOS_LOG_INFO,
                               "MD send session timeout - to be removed: '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                               iterMD->sessionID[0],
                               iterMD->sessionID[1],
                               iterMD->sessionID[2],
                               iterMD->sessionID[3],
                               iterMD->sessionID[4],
                               iterMD->sessionID[5],
                               iterMD->sessionID[6],
                               iterMD->sessionID[7])

                }
                break;
                case TRDP_ST_RX_REPLY_SENT:
                    /* no complain within time out, kill session silently  */
                    iterMD->morituri = TRUE;
                    break;
                case TRDP_ST_TX_REPLY_RECEIVED:
                    /* kill session if # of repliers is undefined (ie multicast) */

                    if((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                    {
                        iterMD->morituri = TRUE;
                        break;
                    }

                    if (iterMD->noOfRepliers == 0)
                    {
                        resultCode       = TRDP_REPLYTO_ERR;
                        timeOut          = TRUE;
                        iterMD->morituri = TRUE;
                    }
                    else
                    {
                        /* kill session if number of repliers have been received  */
                        if (iterMD->noOfRepliers == iterMD->numReplies)
                        {
                            iterMD->morituri = TRUE;
                        }
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
                TRDP_MD_INFO_T theMessage = trdp_md_info_default;

                theMessage.srcIpAddr    = 0;
                theMessage.destIpAddr   = iterMD->addr.destIpAddr;
                theMessage.seqCount     = vos_ntohs(iterMD->pPacket->frameHead.sequenceCounter);
                theMessage.protVersion  = vos_ntohs(iterMD->pPacket->frameHead.protocolVersion);
                theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(iterMD->pPacket->frameHead.msgType);
                theMessage.comId        = iterMD->addr.comId;
                theMessage.topoCount    = iterMD->addr.topoCount;
                theMessage.userStatus   = 0;
                theMessage.replyStatus  = (TRDP_REPLY_STATUS_T) vos_ntohs(iterMD->pPacket->frameHead.replyStatus);
                memcpy(theMessage.sessionId, iterMD->pPacket->frameHead.sessionID, TRDP_SESS_ID_SIZE);
                theMessage.replyTimeout = vos_ntohl(iterMD->pPacket->frameHead.replyTimeout);
                memcpy(theMessage.destURI, iterMD->destURI, 32);
                memcpy(theMessage.srcURI, iterMD->pPacket->frameHead.sourceURI, 32);
                theMessage.numExpReplies    = iterMD->noOfRepliers;
                theMessage.pUserRef         = iterMD->pUserRef;

                theMessage.numReplies = iterMD->numReplies;
                theMessage.numRetriesMax        = iterMD->numRetriesMax;
                theMessage.numRetries           = iterMD->numRetries;
                theMessage.aboutToDie           = iterMD->morituri;
                theMessage.numRepliesQuery      = iterMD->numRepliesQuery;
                theMessage.numConfirmSent       = iterMD->numConfirmSent;
                theMessage.numConfirmTimeout    = iterMD->numConfirmTimeout;

                theMessage.pUserRef     = appHandle->mdDefault.pRefCon;
                theMessage.resultCode   = resultCode;

                appHandle->mdDefault.pfCbFunction(
                    appHandle->mdDefault.pRefCon,
                    appHandle,
                    &theMessage,
                    (UINT8 *)0, 0);
            }

            /* Mark session to be deleted (we should not delete it while iterating through the list...) */
            iterMD->morituri = TRUE;
            vos_printf(VOS_LOG_INFO, "MD rcv session ready to be removed: '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                       iterMD->sessionID[0], iterMD->sessionID[1], iterMD->sessionID[2], iterMD->sessionID[3],
                       iterMD->sessionID[4], iterMD->sessionID[5], iterMD->sessionID[6], iterMD->sessionID[7])
        }

        iterMD = iterMD->pNext;

    }
    while (1);

    /* Check for sockets Connection Timeouts */
    /* if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0) */
    {
        INT32 index;

        for (index = 0; index < VOS_MAX_SOCKET_CNT; index++)
        {
            if ((appHandle->iface[index].sock != -1)
                && (appHandle->iface[index].type == TRDP_SOCK_MD_TCP)
                && (appHandle->iface[index].usage == 0)
                && (appHandle->iface[index].rcvMostly == FALSE)
                && ((appHandle->iface[index].tcpParams.connectionTimeout.tv_sec > 0)
                    || (appHandle->iface[index].tcpParams.connectionTimeout.tv_usec > 0)))
            {
                if (0 > vos_cmpTime(&appHandle->iface[index].tcpParams.connectionTimeout, &now))
                {
                    TRDP_ERR_T err;

                    vos_printf(VOS_LOG_INFO, "The socket (Num = %d) TIMEOUT\n", appHandle->iface[index].sock);

                    /* Execute callback */
                    if (appHandle->mdDefault.pfCbFunction != NULL)
                    {
                        TRDP_MD_INFO_T theMessage = trdp_md_info_default;

                        theMessage.destIpAddr   = appHandle->iface[index].tcpParams.cornerIp;
                        theMessage.resultCode   = TRDP_TIMEOUT_ERR;
                        memset(theMessage.sessionId, 0, TRDP_SESS_ID_SIZE);

                        appHandle->mdDefault.pfCbFunction(
                            appHandle->mdDefault.pRefCon,
                            appHandle,
                            &theMessage, NULL, 0);
                    }

                    err = (TRDP_ERR_T) vos_sockClose(appHandle->iface[index].sock);
                    if (err != TRDP_NO_ERR)
                    {
                        vos_printf(VOS_LOG_ERROR, "vos_sockClose() failed (Err:%d)\n", err);
                    }

                    /* Delete the socket from the iface */
                    vos_printf(VOS_LOG_INFO,
                               "Deleting socket (Num = %d) from the iface\n",
                               appHandle->iface[index].sock);
                    vos_printf(VOS_LOG_INFO, "Close socket iface index=%d\n", index);
                    appHandle->iface[index].sock = -1;
                    appHandle->iface[index].sendParam.qos       = 0;
                    appHandle->iface[index].sendParam.ttl       = 0;
                    appHandle->iface[index].usage               = 0;
                    appHandle->iface[index].bindAddr            = 0;
                    appHandle->iface[index].type                = (TRDP_SOCK_TYPE_T) 0;
                    appHandle->iface[index].rcvMostly           = FALSE;
                    appHandle->iface[index].tcpParams.cornerIp  = 0;
                    appHandle->iface[index].tcpParams.connectionTimeout.tv_sec  = 0;
                    appHandle->iface[index].tcpParams.connectionTimeout.tv_usec = 0;
                    appHandle->iface[index].tcpParams.addFileDesc = FALSE;
                }
            }
        }
    }

    /* Check Sending Timeouts for send() failed/incomplete sockets */
    {
        INT32 index;

        for (index = 0; index < VOS_MAX_SOCKET_CNT; index++)
        {
            if ((appHandle->iface[index].sock != -1)
                && (appHandle->iface[index].type == TRDP_SOCK_MD_TCP)
                && (appHandle->iface[index].rcvMostly == FALSE)
                && (appHandle->iface[index].tcpParams.sendNotOk == TRUE))
            {
                if (0 > vos_cmpTime(&appHandle->iface[index].tcpParams.sendingTimeout, &now))
                {
                    TRDP_ERR_T err;

                    vos_printf(VOS_LOG_INFO, "The socket (Num = %d) Sending TIMEOUT\n", appHandle->iface[index].sock);

                    /* Execute callback */
                    if (appHandle->mdDefault.pfCbFunction != NULL)
                    {
                        TRDP_MD_INFO_T theMessage = trdp_md_info_default;

                        theMessage.destIpAddr   = appHandle->iface[index].tcpParams.cornerIp;
                        theMessage.resultCode   = TRDP_TIMEOUT_ERR;
                        memset(theMessage.sessionId, 0, TRDP_SESS_ID_SIZE);

                        appHandle->mdDefault.pfCbFunction(
                            appHandle->mdDefault.pRefCon,
                            appHandle,
                            &theMessage, NULL, 0);
                    }

                    /* Close the socket */
                    err = (TRDP_ERR_T) vos_sockClose(appHandle->iface[index].sock);
                    if (err != TRDP_NO_ERR)
                    {
                        vos_printf(VOS_LOG_ERROR, "vos_sockClose() failed (Err:%d)\n", err);
                    }

                    /* Delete the socket from the iface */
                    vos_printf(VOS_LOG_INFO,
                               "Deleting socket (Num = %d) from the iface\n",
                               appHandle->iface[index].sock);
                    vos_printf(VOS_LOG_INFO, "Close socket iface index=%d\n", index);
                    appHandle->iface[index].sock = -1;
                    appHandle->iface[index].sendParam.qos       = 0;
                    appHandle->iface[index].sendParam.ttl       = 0;
                    appHandle->iface[index].usage               = 0;
                    appHandle->iface[index].bindAddr            = 0;
                    appHandle->iface[index].type                = (TRDP_SOCK_TYPE_T) 0;
                    appHandle->iface[index].rcvMostly           = FALSE;
                    appHandle->iface[index].tcpParams.cornerIp  = 0;
                    appHandle->iface[index].tcpParams.connectionTimeout.tv_sec  = 0;
                    appHandle->iface[index].tcpParams.connectionTimeout.tv_usec = 0;
                    appHandle->iface[index].tcpParams.addFileDesc = FALSE;
                }
            }
        }

    }

    trdp_closeMDSessions(appHandle);
}

/**********************************************************************************************************************/

TRDP_ERR_T trdp_mdCommonSend (
    const TRDP_MSG_T        msgType,
    TRDP_APP_SESSION_T      appHandle,
    const void              *pUserRef,
    TRDP_UUID_T             *pSessionId,
    UINT32                  comId,
    UINT32                  topoCount,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    UINT16                  userStatus,
    UINT32                  confirmTimeout,
    UINT32                  noOfRepliers,
    UINT32                  replyTimeout,
    TRDP_REPLY_STATUS_T     replyStatus,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    TRDP_ERR_T  errv = TRDP_NO_ERR;
    MD_ELE_T    *pSenderElement = NULL;
    UINT32      sequenceCounter = 0;
    BOOL        newSession      = FALSE;

    /*   TRDP_ADDRESSES  pubHandle = {comId, srcIpAddr, destIpAddr, 0}; */

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if ((pData == NULL) && (dataSize != 0))
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
                                /* TODO: Is this correct? Shall we get the former counter?  */
                                sequenceCounter = iterMD->pPacket->frameHead.sequenceCounter;
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
                /* vos_printf(VOS_LOG_INFO, "MD TRDP_MSG_MC\n"); */
                if (pSessionId)
                {
                    MD_ELE_T *iterMD;
                    for (iterMD = appHandle->pMDSndQueue; iterMD != NULL; iterMD = iterMD->pNext)
                    {
                        if (TRDP_ST_TX_REQ_W4AP_CONFIRM == iterMD->stateEle &&
                            0 == memcmp(iterMD->sessionID, pSessionId, TRDP_SESS_ID_SIZE))
                        {
                            pSenderElement = iterMD;
                            pSenderElement->dataSize    = 0;
                            pSenderElement->grossSize   = trdp_packetSizeMD(0);
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
                    pSenderElement->socketIdx   = -1;
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

        /**/
        {
            TRDP_TIME_T tv_interval;
            UINT32      tmo = 0;

            /* evaluate start time and timeout. For notify I use replyTimeout as sendTimeout */

            {
                switch (msgType)
                {
                    case TRDP_MSG_MP: /* reply without confirm */
                    case TRDP_MSG_MR: /* request with reply */
                    case TRDP_MSG_MN: /* notify (not reply)*/
                    case TRDP_MSG_ME: /* reply error */
                        tmo = replyTimeout ? replyTimeout : appHandle->mdDefault.replyTimeout; /* min time for delivery
                                                                                                 */
                        break;
                    case TRDP_MSG_MQ: /* reply with confirm */
                        tmo = confirmTimeout ? confirmTimeout : appHandle->mdDefault.confirmTimeout;
                        break;
                    default:
                        break;
                }

                tv_interval.tv_sec  = tmo / 1000000;
                tv_interval.tv_usec = tmo % 1000000;
            }

            trdp_mdSetSessionTimeout(pSenderElement, tmo);


            /* Prepare element data */
            pSenderElement->addr.comId      = comId;
            pSenderElement->addr.srcIpAddr  = srcIpAddr;
            pSenderElement->addr.destIpAddr = destIpAddr;
            pSenderElement->addr.mcGroup    = (vos_isMulticast(destIpAddr)) ? destIpAddr : 0;
            pSenderElement->privFlags       = TRDP_PRIV_NONE;
            pSenderElement->interval        = tv_interval;
            pSenderElement->dataSize        = dataSize;
            pSenderElement->grossSize       = trdp_packetSizeMD(dataSize);
            pSenderElement->numReplies      = 0;
            pSenderElement->numRetries      = 0;
            pSenderElement->pCachedDS       = NULL;
            pSenderElement->numRetriesMax   =
                (pSendParam != NULL) ? pSendParam->retries : (appHandle->mdDefault.sendParam.retries);

            /* vos_printf(VOS_LOG_ERROR, "numRetriesMax: %u, %u, %u, \n", pSenderElement->numRetriesMax, (pSendParam !=
             NULL) ? pSendParam->retries : 100000,  appHandle->mdDefault.sendParam.retries); */

            if ((pSenderElement->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                /* No multiple replies expected for TCP */
                pSenderElement->noOfRepliers = 1;

                /* No retries on TCP */
                pSenderElement->numRetriesMax = 0;
            }
            else if (vos_isMulticast(destIpAddr))
            {
                /* Multiple replies expected only for multicast */
                pSenderElement->noOfRepliers = noOfRepliers;

				if (noOfRepliers != 1)
				{
					/* No retries on UDP multicast with expected repliers =1 */
					pSenderElement->numRetriesMax = 0;
				}
            }
            else
            {
                /* No multiple response expected for unicast IP address */
                pSenderElement->noOfRepliers = 1;
            }

            if ((pSenderElement->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                if (pSenderElement->socketIdx == -1)
                {
                    /* socket to send TCP MD for request or notify only */
                    errv = trdp_requestSocket(
                            appHandle->iface,
                            appHandle->mdDefault.tcpPort,
                            (pSendParam != NULL) ? pSendParam : (&appHandle->mdDefault.sendParam),
                            srcIpAddr,
                            0,                 /* no TCP multicast possible */
                            TRDP_SOCK_MD_TCP,
                            TRDP_OPTION_NONE,
                            FALSE,
                            -1,
                            &pSenderElement->socketIdx,
                            destIpAddr);

                    if (TRDP_NO_ERR != errv)
                    {
                        /* Error getting socket */
                        break;
                    }
                }

                /* In the case that it is the first connection, do connect() */

                if(appHandle->iface[pSenderElement->socketIdx].usage > 1)
                {
                    pSenderElement->tcpParameters.doConnect = FALSE;
                }
                else
                {
                    pSenderElement->tcpParameters.doConnect = TRUE;
                }

            }
            else if (TRUE == newSession && -1 == pSenderElement->socketIdx)
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
                        -1,
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

            /* already exist session ID .. */
            if (0 != pSenderElement->sessionID[0])
            {
                vos_printf(VOS_LOG_INFO, "Using session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
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
                vos_printf(VOS_LOG_INFO, "Creating new sender session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                           pSenderElement->sessionID[0], pSenderElement->sessionID[1],
                           pSenderElement->sessionID[2], pSenderElement->sessionID[3],
                           pSenderElement->sessionID[4], pSenderElement->sessionID[5],
                           pSenderElement->sessionID[6], pSenderElement->sessionID[7]);

            }

            /* Add the pUserRef */
            switch (msgType)
            {
                case TRDP_MSG_MR:
                case TRDP_MSG_MC:
                    pSenderElement->pUserRef = pUserRef;
                    break;
                default:
                    break;
            }

            /* Prepare header */
            pSenderElement->pPacket->frameHead.sequenceCounter  = sequenceCounter;
            pSenderElement->pPacket->frameHead.protocolVersion  = vos_htons(TRDP_PROTO_VER);
            pSenderElement->pPacket->frameHead.msgType          = vos_htons((UINT16) msgType);
            pSenderElement->pPacket->frameHead.comId            = vos_htonl(comId);
            pSenderElement->pPacket->frameHead.topoCount        = vos_htonl(topoCount);
            pSenderElement->pPacket->frameHead.datasetLength    = vos_htonl(dataSize);
            pSenderElement->pPacket->frameHead.replyStatus      = vos_htonl(replyStatus);
            memcpy(pSenderElement->pPacket->frameHead.sessionID, pSenderElement->sessionID, TRDP_SESS_ID_SIZE);
            pSenderElement->pPacket->frameHead.replyTimeout = vos_htonl(tmo);

            if (sourceURI != NULL)
            {
                vos_strncpy((CHAR8 *)pSenderElement->pPacket->frameHead.sourceURI, sourceURI, TRDP_MAX_URI_USER_LEN);
            }

            if (destURI != NULL)
            {
                vos_strncpy((CHAR8 *)pSenderElement->pPacket->frameHead.destinationURI, destURI, TRDP_MAX_URI_USER_LEN);
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

            vos_printf(VOS_LOG_INFO,
                       "MD sender element state = %d msgType=x%04X.\n",
                       pSenderElement->stateEle,
                       msgType);
        }
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
        vos_printf(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return errv;
}
