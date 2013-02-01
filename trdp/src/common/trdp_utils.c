/**********************************************************************************************************************/
/**
 * @file            trdp_utils.c
 *
 * @brief           Helper functions for TRDP communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2013.
 *
 *
 * $Id$
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_utils.h"
#include "trdp_if.h"

/***********************************************************************************************************************
 * DEFINES
 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 *   Locals
 */
static INT32 sCurrentMaxSocketCnt = 0;

/**********************************************************************************************************************/
/** Check if a mc group is in the list
 *
 *  @param[in]      mcList[]            List of multicast groups
 *  @param[in]      mcGroup             multicast group
 *
 *  @retval         1           if found
 *                  0           if not found
 */
BOOL trdp_SockIsJoined (
    const TRDP_IP_ADDR_T    mcList[VOS_MAX_MULTICAST_CNT],
    TRDP_IP_ADDR_T          mcGroup)
{
    int i = 0;

    for (i = 0; i < VOS_MAX_MULTICAST_CNT && mcList[i] != mcGroup; i++)
    {
        ;
    }

    return i < VOS_MAX_MULTICAST_CNT;
}

/**********************************************************************************************************************/
/** Add mc group to the list
 *
 *  @param[in]      mcList[]            List of multicast groups
 *  @param[in]      mcGroup             multicast group
 *
 *  @retval         1           if added
 *                  0           if list is full
 */
BOOL trdp_SockAddJoin (
    TRDP_IP_ADDR_T  mcList[VOS_MAX_MULTICAST_CNT],
    TRDP_IP_ADDR_T  mcGroup)
{
    int i = 0;

    for (i = 0; i < VOS_MAX_MULTICAST_CNT; i++)
    {
        if (0 == mcList[i] || mcGroup == mcList[i])
        {
            mcList[i] = mcGroup;
            return TRUE;
        }
    }

    return FALSE;
}

/**********************************************************************************************************************/
/** remove mc group from the list
 *
 *  @param[in]      mcList[]            List of multicast groups
 *  @param[in]      mcGroup             multicast group
 *
 *  @retval         1           if deleted
 *                  0           was not in list
 */
BOOL trdp_SockDelJoin (
    TRDP_IP_ADDR_T  mcList[VOS_MAX_MULTICAST_CNT],
    TRDP_IP_ADDR_T  mcGroup)
{
    int i = 0;

    for (i = 0; i < VOS_MAX_MULTICAST_CNT; i++)
    {
        if (mcGroup == mcList[i])
        {
            mcList[i] = 0;
            return TRUE;
        }
    }

    return FALSE;
}


/***********************************************************************************************************************
 *   Globals
 */

/**********************************************************************************************************************/
/** Determine if we are Big or Little endian
 *
 *  @retval         != 0        we are big endian
 *  @retval         0           we are little endian
 */
int am_big_endian ()
{
    int one = 1;
    return !(*((char *)(&one)));
}


/**********************************************************************************************************************/
/** Get the packet size from the raw data size
 *
 *  @param[in]      dataSize            net data size (without padding or FCS)
 *
 *  @retval         packet size         the size of the complete packet to
 *                                      be sent or received
 */
UINT32 trdp_packetSizePD (
    UINT32 dataSize)
{
    UINT32 packetSize = sizeof(PD_HEADER_T) + dataSize + sizeof(UINT32);

    /*  padding to 4 */
    if ((dataSize & 0x3) > 0)
    {
        packetSize += 4 - dataSize % 4;
    }

    return packetSize;
}


/**********************************************************************************************************************/
/** Return the element with same comId
 *
 *  @param[in]      pHead           pointer to head of queue
 *  @param[in]      comId           ComID to search for
 *
 *  @retval         != NULL         pointer to PD element
 *  @retval         NULL            No PD element found
 */
PD_ELE_T *trdp_queueFindComId (
    PD_ELE_T    *pHead,
    UINT32      comId)
{
    PD_ELE_T *iterPD;

    if (pHead == NULL || comId == 0)
    {
        return NULL;
    }

    for (iterPD = pHead; iterPD != NULL; iterPD = iterPD->pNext)
    {
        if (iterPD->addr.comId == comId)
        {
            return iterPD;
        }
    }
    return NULL;
}


/**********************************************************************************************************************/
/** Return the element with same comId and IP addresses
 *
 *  @param[in]      pHead           pointer to head of queue
 *  @param[in]      addr            Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *
 *  @retval         != NULL         pointer to PD element
 *  @retval         NULL            No PD element found
 */
PD_ELE_T *trdp_queueFindPubAddr (
    PD_ELE_T            *pHead,
    TRDP_ADDRESSES_T    *addr)
{
    PD_ELE_T *iterPD;

    if (pHead == NULL || addr == NULL)
    {
        return NULL;
    }

    for (iterPD = pHead; iterPD != NULL; iterPD = iterPD->pNext)
    {
        /*  We match if src/dst/mc address is zero or matches */
        if (iterPD->addr.comId == addr->comId &&
            (iterPD->addr.srcIpAddr == 0 || iterPD->addr.srcIpAddr == addr->srcIpAddr) &&
            (iterPD->addr.destIpAddr == 0 || iterPD->addr.destIpAddr == addr->destIpAddr) &&
            (iterPD->addr.mcGroup == 0 || iterPD->addr.mcGroup == addr->mcGroup)
            )
        {
            return iterPD;
        }
    }
    return NULL;
}


/**********************************************************************************************************************/
/** Return the element with same comId and IP addresses
 *
 *  @param[in]      pHead           pointer to head of queue
 *  @param[in]      addr            Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *
 *  @retval         != NULL         pointer to PD element
 *  @retval         NULL            No PD element found
 */
PD_ELE_T *trdp_queueFindSubAddr (
    PD_ELE_T            *pHead,
    TRDP_ADDRESSES_T    *addr)
{
    PD_ELE_T *iterPD;

    if (pHead == NULL || addr == NULL)
    {
        return NULL;
    }

    for (iterPD = pHead; iterPD != NULL; iterPD = iterPD->pNext)
    {
        /*  We match if src/dst/mc address is zero or matches */
        if (iterPD->addr.comId == addr->comId &&
            (iterPD->addr.srcIpAddr == 0 || iterPD->addr.srcIpAddr == addr->srcIpAddr)
            )
        {
            return iterPD;
        }
    }
    return NULL;
}


/**********************************************************************************************************************/
/** Return the element with same comId from MD queue
 *
 *  @param[in]      pHead           pointer to head of queue
 *  @param[in]      addr            Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *
 *  @retval         != NULL         pointer to PD element
 *  @retval         NULL            No PD element found
 */
MD_ELE_T *trdp_MDqueueFindAddr (
    MD_ELE_T            *pHead,
    TRDP_ADDRESSES_T    *addr)
{
    MD_ELE_T *iterMD;

    if (pHead == NULL || addr == NULL)
    {
        return NULL;
    }

    for (iterMD = pHead; iterMD != NULL; iterMD = iterMD->pNext)
    {
        /*  We match if src/dst address is zero or found */
        if (iterMD->addr.comId == addr->comId &&
            (addr->srcIpAddr == 0 || iterMD->addr.srcIpAddr ==
             addr->srcIpAddr) &&
            (addr->destIpAddr == 0 || iterMD->addr.destIpAddr ==
             addr->destIpAddr))
        {
            return iterMD;
        }
    }
    return NULL;
}


/**********************************************************************************************************************/
/** Delete an element
 *
 *  @param[in]      ppHead          pointer to pointer to head of queue
 *  @param[in]      pDelete         pointer to element to delete
 */
void    trdp_queueDelElement (
    PD_ELE_T    * *ppHead,
    PD_ELE_T    *pDelete)
{
    PD_ELE_T *iterPD;

    if (ppHead == NULL || *ppHead == NULL || pDelete == NULL)
    {
        return;
    }

    /* handle removal of first element */
    if (pDelete == *ppHead)
    {
        *ppHead = pDelete->pNext;
        return;
    }

    for (iterPD = *ppHead; iterPD != NULL; iterPD = iterPD->pNext)
    {
        if (iterPD->pNext && iterPD->pNext == pDelete)
        {
            iterPD->pNext = pDelete->pNext;
            return;
        }
    }
}


/**********************************************************************************************************************/
/** Delete an element from MD queue
 *
 *  @param[in]      ppHead          pointer to pointer to head of queue
 *  @param[in]      pDelete         pointer to element to delete
 */
void    trdp_MDqueueDelElement (
    MD_ELE_T    * *ppHead,
    MD_ELE_T    *pDelete)
{
    MD_ELE_T *iterMD;

    if (ppHead == NULL || *ppHead == NULL || pDelete == NULL)
    {
        return;
    }

    /*    handle removal of first element    */
    if (pDelete == *ppHead)
    {
        *ppHead = pDelete->pNext;
        return;
    }

    for (iterMD = *ppHead; iterMD != NULL; iterMD = iterMD->pNext)
    {
        if (iterMD->pNext && iterMD->pNext == pDelete)
        {
            iterMD->pNext = pDelete->pNext;
            return;
        }
    }
}


/**********************************************************************************************************************/
/** Append an element at end of queue
 *
 *  @param[in]      ppHead          pointer to pointer to head of queue
 *  @param[in]      pNew            pointer to element to append
 */
void    trdp_queueAppLast (
    PD_ELE_T    * *ppHead,
    PD_ELE_T    *pNew)
{
    PD_ELE_T *iterPD;

    if (ppHead == NULL || pNew == NULL)
    {
        return;
    }

    /* Ensure this element is last! */
    pNew->pNext = NULL;

    if (*ppHead == NULL)
    {
        *ppHead = pNew;
        return;
    }

    for (iterPD = *ppHead; iterPD->pNext != NULL; iterPD = iterPD->pNext)
    {
        ;
    }
    iterPD->pNext = pNew;
}


/**********************************************************************************************************************/
/** Append an element at end of queue
 *
 *  @param[in]      ppHead          pointer to pointer to head of queue
 *  @param[in]      pNew            pointer to element to append
 */
void    trdp_MDqueueAppLast (
    MD_ELE_T    * *ppHead,
    MD_ELE_T    *pNew)
{
    MD_ELE_T *iterMD;

    if (ppHead == NULL || pNew == NULL)
    {
        return;
    }

    /* Ensure this element is last! */
    pNew->pNext = NULL;

    if (*ppHead == NULL)
    {
        *ppHead = pNew;
        return;
    }

    for (iterMD = *ppHead; iterMD->pNext != NULL; iterMD = iterMD->pNext)
    {
        ;
    }
    iterMD->pNext = pNew;
}


/**********************************************************************************************************************/
/** Insert an element at front of queue
 *
 *  @param[in]      ppHead          pointer to pointer to head of queue
 *  @param[in]      pNew            pointer to element to insert
 */
void    trdp_queueInsFirst (
    PD_ELE_T    * *ppHead,
    PD_ELE_T    *pNew)
{
    if (ppHead == NULL || pNew == NULL)
    {
        return;
    }

    pNew->pNext = *ppHead;
    *ppHead     = pNew;
}


/**********************************************************************************************************************/
/** Insert an element at front of MD queue
 *
 *  @param[in]      ppHead          pointer to pointer to head of queue
 *  @param[in]      pNew            pointer to element to insert
 */
void    trdp_MDqueueInsFirst (
    MD_ELE_T    * *ppHead,
    MD_ELE_T    *pNew)
{
    if (ppHead == NULL || pNew == NULL)
    {
        return;
    }

    pNew->pNext = *ppHead;
    *ppHead     = pNew;
}


/**********************************************************************************************************************/
/** Handle the socket pool: Initialize it
 *
 *  @param[in]      iface          pointer to the socket pool
 */
void trdp_initSockets (TRDP_SOCKETS_T iface[])
{
    int index;
    /* Clear the socket pool */
    for (index = 0; index < VOS_MAX_SOCKET_CNT; index++)
    {
        iface[index].sock = -1;
    }
}


/**********************************************************************************************************************/
/** Handle the socket pool: Request a socket from our socket pool
 *  First we loop through the socket pool and check if there is already a socket
 *  which would suit us. If a multicast group should be joined, we do that on an otherwise suitable socket - up to 20
 *  multicast goups can be joined per socket.
 *  If a socket for multicast publishing is requested, we also use the source IP to determine the interface for outgoing
 *  multicast traffic.
 *
 *  @param[in,out]  iface           socket pool
 *  @param[in]      port            port to use
 *  @param[in]      params          parameters to use
 *  @param[in]      srcIP           IP to bind to (0 = any address)
 *  @param[in]      mcGroup         MC group to join (0 = do not join)
 *  @param[in]      usage           type and port to bind to (PD, MD/UDP, MD/TCP)
 *  @param[in]      options         blocking/nonblocking
 *  @param[in]      rcvOnly         only used for receiving
 *  @param[out]     pIndex          returned index of socket pool
 *  @param[in]      cornerIp        only used for receiving
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_PARAM_ERR
 */
TRDP_ERR_T  trdp_requestSocket (
    TRDP_SOCKETS_T          iface[],
    UINT32                  port,
    const TRDP_SEND_PARAM_T *params,
    TRDP_IP_ADDR_T          srcIP,
    TRDP_IP_ADDR_T          mcGroup,
    TRDP_SOCK_TYPE_T        usage,
    TRDP_OPTION_T           options,
    BOOL                    rcvOnly,
    INT32                   *pIndex,
    TRDP_IP_ADDR_T          cornerIp)
{
    VOS_SOCK_OPT_T  sock_options;
    INT32           index, emptySock = -1;
    TRDP_ERR_T      err         = TRDP_NO_ERR;
    TRDP_IP_ADDR_T  bindAddr    = srcIP;

    if (iface == NULL || params == NULL || pIndex == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /*  We loop through the table of open/used sockets,
     if we find a usable one (with the same socket options) we take it.
     if we search for a multicast group enabled socket, we also search the list of mc groups (max. 20)
     and possibly add that group, if everything else fits.
     We remember already closed sockets on the way to be able to fill up gaps  */

    if (vos_isMulticast(mcGroup) && rcvOnly)
    {
        bindAddr = 0;
    }

    for (index = 0; index < sCurrentMaxSocketCnt; index++)
    {
        if (iface[index].sock != -1 &&
            iface[index].bindAddr == bindAddr &&
            iface[index].type == usage &&
            iface[index].sendParam.qos == params->qos &&
            iface[index].sendParam.ttl == params->ttl &&
            iface[index].rcvOnly == rcvOnly &&
            ((usage != TRDP_SOCK_MD_TCP) ||
             ((usage == TRDP_SOCK_MD_TCP) && (iface[index].tcpParams.cornerIp == cornerIp))))
        {
            /*  Did this socket join the required multicast group?  */
            if (mcGroup != 0 && trdp_SockIsJoined(iface[index].mcGroups, mcGroup) == FALSE)
            {
                /*  No, but can we add it? */
                if (trdp_SockAddJoin(iface[index].mcGroups, mcGroup) == FALSE)
                {
                    continue;   /* No, socket cannot join more MC groups */
                }
                else
                {
                    if (vos_sockJoinMC(iface[index].sock, mcGroup, iface[index].bindAddr) != VOS_NO_ERR)
                    {
                        if (trdp_SockDelJoin(iface[index].mcGroups, mcGroup)==FALSE)
                        {
                            vos_printf(VOS_LOG_ERROR, "trdp_SockDelJoin() failed!\n");
                        }
                        continue;   /* No, socket cannot join more MC groups */
                    }
                }
            }

            /* Use that socket */
            *pIndex = index;

            if(((usage != TRDP_SOCK_MD_TCP)) || ((usage == TRDP_SOCK_MD_TCP) && (iface[index].usage > 0)))
            {
                iface[index].usage++;
            }

            return err;
        }
        else if (iface[index].sock == -1 && emptySock == -1)
        {
            /* Remember the first empty slot */
            emptySock = index;
        }
    }

    /* Not found, create a new socket */
    if (index < VOS_MAX_SOCKET_CNT)
    {
        if (emptySock != -1 && index != emptySock)
        {
            index = emptySock;
        }
        else
        {
            sCurrentMaxSocketCnt = index + 1;
        }

        iface[index].sock           = -1;
        iface[index].bindAddr       = srcIP;
        iface[index].type           = usage;
        iface[index].sendParam.qos  = params->qos;
        iface[index].sendParam.ttl  = params->ttl;
        iface[index].rcvOnly        = rcvOnly;
        iface[index].tcpParams.connectionTimeout.tv_sec     = 0;
        iface[index].tcpParams.connectionTimeout.tv_usec    = 0;
        iface[index].tcpParams.cornerIp = cornerIp;
        memset(iface[index].mcGroups, 0, sizeof(iface[index].mcGroups));

        sock_options.qos    = params->qos;
        sock_options.ttl    = params->ttl;
        sock_options.ttl_multicast  = VOS_TTL_MULTICAST;
        sock_options.reuseAddrPort  = TRUE;
        sock_options.nonBlocking    = (options == TRDP_OPTION_BLOCK) ? FALSE : TRUE;

        if((usage == TRDP_SOCK_MD_TCP) && (rcvOnly == TRUE))
        {
            iface[index].sock = *pIndex;
            *pIndex = index;
            iface[index].usage = 0;
            return err;
        }

        switch (usage)
        {
            case TRDP_SOCK_MD_UDP:
                sock_options.nonBlocking = TRUE; /* MD UDP sockets are always non blocking because they are polled */
            case TRDP_SOCK_PD:
                err = (TRDP_ERR_T) vos_sockOpenUDP(&iface[index].sock, &sock_options);
                if (err != TRDP_NO_ERR)
                {
                    vos_printf(VOS_LOG_ERROR, "vos_sockOpenUDP failed! (Err: %d)\n", err);
                    *pIndex = -1;
                }
                else
                {
                    iface[index].usage = 1;
                    *pIndex = index;
                    if (rcvOnly)
                    {
                        /*  Only bind to local IP if we are not a multicast listener  */
                        if (0 == mcGroup)
                        {
                            err = (TRDP_ERR_T) vos_sockBind(iface[index].sock, iface[index].bindAddr, port);
                        }
                        else
                        {
                            err = (TRDP_ERR_T) vos_sockBind(iface[index].sock, mcGroup, port);
                        }

                        if (err != TRDP_NO_ERR)
                        {
                            vos_printf(VOS_LOG_ERROR, "vos_sockBind() for UDP rcv failed! (Err: %d)\n", err);
                            *pIndex = -1;
                            break;
                        }

                        if (0 != mcGroup)
                        {

                            err = (TRDP_ERR_T) vos_sockJoinMC(iface[index].sock, mcGroup, iface[index].bindAddr);
                            if (err != TRDP_NO_ERR)
                            {
                                vos_printf(VOS_LOG_ERROR, "vos_sockJoinMC() for UDP rcv failed! (Err: %d)\n", err);
                                *pIndex = -1;
                                break;
                            }
                            else
                            {
                                if (trdp_SockAddJoin(iface[index].mcGroups, mcGroup)==FALSE)
                                {
                                    vos_printf(VOS_LOG_ERROR, "trdp_SockAddJoin() failed!\n");
                                }
                            }
                        }
                    }
                    else if (0 != mcGroup)      /*	Multicast sender shall be bound to an interface	*/
                    {
                        err = (TRDP_ERR_T) vos_sockSetMulticastIf(iface[index].sock, iface[index].bindAddr);
                        if (err != TRDP_NO_ERR)
                        {
                            vos_printf(VOS_LOG_ERROR, "vos_sockSetMulticastIf() for UDP snd failed! (Err: %d)\n", err);
                            *pIndex = -1;
                            break;
                        }
                    }

                }
                break;
            case TRDP_SOCK_MD_TCP:
                err = (TRDP_ERR_T) vos_sockOpenTCP(&iface[index].sock, &sock_options);
                if (err != TRDP_NO_ERR)
                {
                    vos_printf(VOS_LOG_ERROR, "vos_sockOpenTCP() failed! (Err: %d)\n", err);
                    *pIndex = -1;
                }
                else
                {
                    iface[index].usage = 1;
                    *pIndex = index;
                }
                break;
            default:
                *pIndex = -1;
                err     = TRDP_SOCK_ERR;
                break;
        }
    }
    else
    {
        err = TRDP_MEM_ERR;
    }

    return err;
}


/**********************************************************************************************************************/
/** Handle the socket pool: Release a socket from our socket pool
 *
 *  @param[in,out]  iface           socket pool
 *  @param[in]      index           index of socket to release
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_PARAM_ERR
 */
TRDP_ERR_T  trdp_releaseSocket (
    TRDP_SOCKETS_T  iface[],
    INT32           index)
{
    TRDP_ERR_T err = TRDP_PARAM_ERR;

    if (iface != NULL)
    {
        if (iface[index].sock > -1)
        {
            if (--iface[index].usage == 0)
            {
                /* Close that socket, nobody uses it anymore */
                err = (TRDP_ERR_T) vos_sockClose(iface[index].sock);
                iface[index].sock = -1;
            }
        }
    }

    return err;
}


/**********************************************************************************************************************/
/** Get the initial sequence counter for the comID/message type and subnet (source IP).
 *  If the comID/srcIP is not found elsewhere, return 0 -
 *  else return its current sequence number (the redundant packet needs the same seqNo)
 *
 *  Note: The standard demands that sequenceCounter is managed per comID/msgType at each publisher,
 *        but shall be the same for redundant telegrams (subnet/srcIP).
 *
 *  @param[in]      comId           comID to look for
 *  @param[in]      msgType         PD/MD type
 *  @param[in]      srcIpAddr       Source IP address
 *
 *  @retval            return the sequence number
 */

UINT32  trdp_getSeqCnt (
    UINT32          comId,
    TRDP_MSG_T      msgType,
    TRDP_IP_ADDR_T  srcIpAddr)
{
    TRDP_SESSION_PT pSession        = (TRDP_SESSION_PT)trdp_sessionQueue();
    PD_ELE_T        *pSendElement   = NULL;

    if (0 == comId || 0 == srcIpAddr)
    {
        return 0;
    }

    /*    For process data look at the PD send queue only    */
    if (TRDP_MSG_PD == msgType ||
        TRDP_MSG_PP == msgType ||
        TRDP_MSG_PR == msgType ||
        TRDP_MSG_PE == msgType)
    {
        /*    Loop thru all sessions    */
        while (pSession)
        {
            pSendElement = pSession->pSndQueue;
            while (pSendElement)
            {
                if (pSendElement->addr.comId == comId &&
                    pSendElement->addr.srcIpAddr != srcIpAddr)
                {
                    return pSendElement->curSeqCnt;
                }
                pSendElement = pSendElement->pNext;
            }
            pSession = pSession->pNext;
        }
    }
#if MD_SUPPORT
    else
    {
        /* #error */
    }
#endif
    return 0;   /*    Not found, initial value is zero    */
}


/**********************************************************************************************************************/
/** Check if the sequence counter for the comID/message type and subnet (source IP)
 *  has already been received.
 *
 *  Note: The standard demands that sequenceCounter is managed per comID/msgType at each publisher,
 *         but shall be the same for redundant telegrams (subnet/srcIP).
 *
 *  @param[in]      seqCnt          sequence counter received
 *  @param[in]      comId           comID to look for
 *  @param[in]      msgType         PD/MD type
 *  @param[in]      srcIP           Source IP address
 *
 *  @retval         return the sequence number
 */

BOOL  trdp_isRcvSeqCnt (
    UINT32          seqCnt,
    UINT32          comId,
    TRDP_MSG_T      msgType,
    TRDP_IP_ADDR_T  srcIP)
{
    TRDP_SESSION_PT pSession        = (TRDP_SESSION_PT)trdp_sessionQueue();
    PD_ELE_T        *pRcvElement    = NULL;

    if (0 == comId || 0 == srcIP)
    {
        return FALSE;
    }

    /*    For process data look at the PD recv queue only    */
    if (TRDP_MSG_PD == msgType ||
        TRDP_MSG_PP == msgType ||
        TRDP_MSG_PR == msgType ||
        TRDP_MSG_PE == msgType)
    {
        /*    Loop thru all sessions    */
        while (pSession)
        {
            pRcvElement = pSession->pRcvQueue;
            while (pRcvElement)
            {
                if (pRcvElement->addr.comId == comId &&
                    pRcvElement->addr.srcIpAddr != srcIP)
                {
                    if (pRcvElement->curSeqCnt == seqCnt)
                    {
                        return TRUE;
                    }
                    else
                    {
                        pRcvElement->curSeqCnt = seqCnt;
                        return FALSE;
                    }

                }
                pRcvElement = pRcvElement->pNext;
            }
            pSession = pSession->pNext;
        }
    }
#if MD_SUPPORT
    else
    {
        /* #error */
    }
#endif
    return FALSE;   /* Not found, initial value is zero */
}


/**********************************************************************************************************************/
/** Check if listener URI is in addressing range of destination URI.
 *
 *  @param[in]      listUri       Null terminated listener URI string to compare
 *  @param[in]      destUri       Null terminated destination URI string to compare
 *
 *  @retval         FALSE - not in addressing range
 *  @retval         TRUE  - listener URI is in addressing range of destination URI
 */

BOOL trdp_isAddressed (const TRDP_URI_USER_T listUri, const TRDP_URI_USER_T destUri)
{
    return (vos_strnicmp(listUri, destUri, sizeof(TRDP_URI_USER_T)) == 0);
}
