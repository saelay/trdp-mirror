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
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      BL 2016-07-06: Ticket #122 64Bit compatibility (+ compiler warnings)
 *      BL 2016-03-01: Setting correct multicast TTL for PDs
 *      BL 2014-08-25: Ticket #57+58: Padding / zero bytes trailing MD & PD packets fixed
 *      BL 2014-06-02: Ticket #41: Sequence counter handling fixed
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_if.h"
#include "trdp_utils.h"

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
/** Debug socket usage output
 *
 *  @param[in]      iface[]            List of sockets
 */
void printSocketUsage (
    TRDP_SOCKETS_T iface[])
{
    INT32 lIndex = 0;
    vos_printLogStr(VOS_LOG_DBG, "------- Socket usage -------\n");
    for (lIndex = 0; lIndex < sCurrentMaxSocketCnt; lIndex++)
    {
        if (iface[lIndex].sock == -1)
        {
            continue;
        }
        vos_printLog(VOS_LOG_DBG, "iface[%u].sock = %u\n", lIndex, iface[lIndex].sock);
        vos_printLog(VOS_LOG_DBG, "iface[%u].bindAddr = %x\n", lIndex, iface[lIndex].bindAddr);
        vos_printLog(VOS_LOG_DBG, "iface[%u].type = %s \n", lIndex, (iface[lIndex].type == 0 ? "PD_UDP" :
                                                                     (iface[lIndex].type == 1 ? "MD_UDP" : "MD_TCP")));
        vos_printLog(VOS_LOG_DBG, "iface[%u].sendParam.qos = %u\n", lIndex, iface[lIndex].sendParam.qos);
        vos_printLog(VOS_LOG_DBG, "iface[%u].sendParam.ttl = %u\n", lIndex, iface[lIndex].sendParam.ttl);
        vos_printLog(VOS_LOG_DBG, "iface[%u].rcvMostly = %u\n", lIndex, iface[lIndex].rcvMostly);
    }
    vos_printLogStr(VOS_LOG_DBG, "----------------------------\n\n");
}

/**********************************************************************************************************************/
/** Check if a mc group is in the list
 *
 *  @param[in]      mcList[]            List of multicast groups
 *  @param[in]      mcGroup             multicast group
 *
 *  @retval         1           if found
 *                  0           if not found
 */
BOOL8 trdp_SockIsJoined (
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
BOOL8 trdp_SockAddJoin (
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
BOOL8 trdp_SockDelJoin (
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
/** Get the packet size from the raw data size
 *
 *  @param[in]      dataSize            net data size
 *
 *  @retval         packet size         the size of the complete packet to
 *                                      be sent or received
 */
UINT32 trdp_packetSizePD (
    UINT32 dataSize)
{
    return sizeof(PD_HEADER_T) + dataSize;
}


/**********************************************************************************************************************/
/** Get the packet size from the raw data size
 *
 *  @param[in]      dataSize            net data size
 *
 *  @retval         packet size         the size of the complete packet to
 *                                      be sent or received
 */
UINT32 trdp_packetSizeMD (
    UINT32 dataSize)
{
    return sizeof(MD_HEADER_T) + dataSize;
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
        if ((iterPD->addr.comId == addr->comId)
            && ((iterPD->addr.srcIpAddr == 0) || (iterPD->addr.srcIpAddr == addr->srcIpAddr))
            && ((iterPD->addr.destIpAddr == 0) || (iterPD->addr.destIpAddr == addr->destIpAddr))
            && ((iterPD->addr.mcGroup == 0) || (iterPD->addr.mcGroup == addr->mcGroup)))
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
        if ((iterPD->addr.comId == addr->comId)
            && ((iterPD->addr.srcIpAddr == 0) || (iterPD->addr.srcIpAddr == addr->srcIpAddr)))
        {
            /*  We check for local communication
                or if etbTopoCnt and opTrnTopoCnt of the subscription are zero or match */
            if (
                ((addr->etbTopoCnt == 0) && (addr->opTrnTopoCnt == 0))
                || trdp_validTopoCounters( addr->etbTopoCnt,
                                           addr->opTrnTopoCnt,
                                           iterPD->addr.etbTopoCnt,
                                           iterPD->addr.opTrnTopoCnt))
            {
                return iterPD;
            }
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
/** Check topography counters
 *  The applied conformance pattern follows Table A.5/A.21 (positive match):
 *  Telegram to be sent   Locally stored value (appSession)
 *  Case etbTopoCnt opTrnTopoCnt etbTopoCntFilter opTrnTopoCntFilter
 *  1    any        any          0                0
 *  2    any        equal        0                equal
 *  3    equal      any          equal            0
 *  4    equal      equal        equal            equal
 *
 *  @param[in]      etbTopoCnt              ETB topography counter to be checked
 *  @param[in]      opTrnTopoCnt            Operational topography counter to be checked
 *  @param[in]      etbTopoCntFilter        ETB topography counter filter value
 *  @param[in]      opTrnTopoCntFilter      Operational topography counter filter value
 *
 *  @retval         TRUE           Filter criteria matched
 *                  FALSE          Filter criteria not matched
 */
BOOL8 trdp_validTopoCounters (
    UINT32  etbTopoCnt,
    UINT32  opTrnTopoCnt,
    UINT32  etbTopoCntFilter,
    UINT32  opTrnTopoCntFilter)
{
    if (((etbTopoCntFilter == 0) || (etbTopoCnt == etbTopoCntFilter))
        &&
        ((opTrnTopoCntFilter == 0) || (opTrnTopoCnt == opTrnTopoCntFilter)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#if MD_SUPPORT
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
        if ((iterMD->addr.comId == addr->comId)
            && ((addr->srcIpAddr == 0) || (iterMD->addr.srcIpAddr == addr->srcIpAddr))
            && ((addr->destIpAddr == 0) || (iterMD->addr.destIpAddr == addr->destIpAddr)))
        {
            return iterMD;
        }
    }
    return NULL;
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
/** Initialize the UncompletedTCP pointers to null
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 */
void trdp_initUncompletedTCP (TRDP_APP_SESSION_T appHandle)
{
    int lIndex;
    /* Initialize the pointers to Null */
    for (lIndex = 0; lIndex < VOS_MAX_SOCKET_CNT; lIndex++)
    {
        appHandle->uncompletedTCP[lIndex] = NULL;
    }
}
#endif

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
/** Handle the socket pool: Initialize it
 *
 *  @param[in]      iface          pointer to the socket pool
 */
void trdp_initSockets (TRDP_SOCKETS_T iface[])
{
    int lIndex;
    /* Clear the socket pool */
    for (lIndex = 0; lIndex < VOS_MAX_SOCKET_CNT; lIndex++)
    {
        iface[lIndex].sock = VOS_INVALID_SOCKET;
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
 *  @param[in]      rcvMostly       primarily used for receiving (tbd: bind on sender, too?)
 *  @param[out]     useSocket       socket to use, do not open a new one
 *  @param[out]     pIndex          returned index of socket pool
 *  @param[in]      cornerIp        only used for receiving
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_PARAM_ERR
 */
TRDP_ERR_T  trdp_requestSocket (
    TRDP_SOCKETS_T          iface[],
    UINT16                  port,
    const TRDP_SEND_PARAM_T *params,
    TRDP_IP_ADDR_T          srcIP,
    TRDP_IP_ADDR_T          mcGroup,
    TRDP_SOCK_TYPE_T        usage,
    TRDP_OPTION_T           options,
    BOOL8                   rcvMostly,
    INT32                   useSocket,
    INT32                   *pIndex,
    TRDP_IP_ADDR_T          cornerIp)
{
    VOS_SOCK_OPT_T  sock_options;
    INT32           lIndex, emptySock = VOS_INVALID_SOCKET;
    TRDP_ERR_T      err         = TRDP_NO_ERR;
    TRDP_IP_ADDR_T  bindAddr    = vos_determineBindAddr(srcIP, mcGroup, rcvMostly);

    if (iface == NULL || params == NULL || pIndex == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /*  We loop through the table of open/used sockets,
     if we find a usable one (with the same socket options) we take it.
     if we search for a multicast group enabled socket, we also search the list of mc groups (max. 20)
     and possibly add that group, if everything else fits.
     We remember already closed sockets on the way to be able to fill up gaps  */

    for (lIndex = 0; lIndex < sCurrentMaxSocketCnt; lIndex++)
    {
        /*  Check if the wanted socket is already in our list; if yes, increment usage */
        if (useSocket != VOS_INVALID_SOCKET &&
            useSocket == iface[lIndex].sock)
        {
            /* Use that socket */
            *pIndex = lIndex;
            iface[lIndex].usage++;
            return TRDP_NO_ERR;
        }
        else if ((iface[lIndex].sock != VOS_INVALID_SOCKET)
                 && (iface[lIndex].bindAddr == bindAddr)
                 && (iface[lIndex].type == usage)
                 && (iface[lIndex].sendParam.qos == params->qos)
                 && (iface[lIndex].sendParam.ttl == params->ttl)
                 && (iface[lIndex].rcvMostly == rcvMostly)
                 && ((usage != TRDP_SOCK_MD_TCP)
                     || ((usage == TRDP_SOCK_MD_TCP) && (iface[lIndex].tcpParams.cornerIp == cornerIp))))
        {
            /*  Did this socket join the required multicast group?  */
            if (mcGroup != 0 && trdp_SockIsJoined(iface[lIndex].mcGroups, mcGroup) == FALSE)
            {
                /*  No, but can we add it? */
                if (trdp_SockAddJoin(iface[lIndex].mcGroups, mcGroup) == FALSE)
                {
                    continue;   /* No, socket cannot join more MC groups */
                }
                else
                {
                    if (vos_sockJoinMC(iface[lIndex].sock, mcGroup, srcIP) != VOS_NO_ERR)
                    {
                        if (trdp_SockDelJoin(iface[lIndex].mcGroups, mcGroup) == FALSE)
                        {
                            vos_printLogStr(VOS_LOG_ERROR, "trdp_SockDelJoin() failed!\n");
                        }
                        continue;   /* No, socket cannot join more MC groups */
                    }
                }
            }

/* add_start TOSHIBA 0306 */
            if ((usage != TRDP_SOCK_MD_TCP)
                && (iface[lIndex].bindAddr != 0))
            {
                err = (TRDP_ERR_T) vos_sockSetMulticastIf(iface[lIndex].sock, iface[lIndex].bindAddr);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "vos_sockSetMulticastIf() for UDP snd failed! (Err: %d)\n", err);
                }
            }
/* add_end TOSHIBA */

            /* Use that socket */
            *pIndex = lIndex;

            if ((usage != TRDP_SOCK_MD_TCP)
                || ((usage == TRDP_SOCK_MD_TCP) && (iface[lIndex].usage > -1)))
            {
                iface[lIndex].usage++;
            }

            return err;
        }
        else if (iface[lIndex].sock == VOS_INVALID_SOCKET && emptySock == VOS_INVALID_SOCKET)
        {
            /* Remember the first empty slot */
            emptySock = lIndex;
        }
    }

    /* Not found, create a new socket entry */
    if (lIndex < VOS_MAX_SOCKET_CNT)
    {
        if ((emptySock != VOS_INVALID_SOCKET)
            && (lIndex != emptySock))
        {
            lIndex = emptySock;
        }
        else
        {
            sCurrentMaxSocketCnt = lIndex + 1;
        }

        iface[lIndex].sock          = VOS_INVALID_SOCKET;
        iface[lIndex].bindAddr      = bindAddr /* was srcIP (ID #125) */;
        iface[lIndex].type          = usage;
        iface[lIndex].sendParam.qos = params->qos;
        iface[lIndex].sendParam.ttl = params->ttl;
        iface[lIndex].rcvMostly     = rcvMostly;
        iface[lIndex].tcpParams.connectionTimeout.tv_sec    = 0;
        iface[lIndex].tcpParams.connectionTimeout.tv_usec   = 0;
        iface[lIndex].tcpParams.cornerIp    = cornerIp;
        iface[lIndex].tcpParams.sendNotOk   = FALSE;

        /* Add to the file desc only if it's an accepted socket */
        if (rcvMostly == TRUE)
        {
            iface[lIndex].tcpParams.addFileDesc = TRUE;
        }
        else
        {
            iface[lIndex].tcpParams.addFileDesc = FALSE;
        }

        memset(iface[lIndex].mcGroups, 0, sizeof(iface[lIndex].mcGroups));

        /* if a socket descriptor was supplied, take that one (for the TCP connection)   */
        if (useSocket != VOS_INVALID_SOCKET)
        {
            iface[lIndex].sock  = useSocket;
            iface[lIndex].usage = 0;         /* Mark as used */
            *pIndex = lIndex;
            return err;
        }

        sock_options.qos    = params->qos;
        sock_options.ttl    = params->ttl;
        sock_options.reuseAddrPort  = (options & TRDP_OPTION_NO_REUSE_ADDR) ? FALSE : TRUE;
        sock_options.nonBlocking    = (options & TRDP_OPTION_BLOCK) ? FALSE : TRUE;
        /* sock_options.ttl_multicast  = (usage != TRDP_SOCK_MD_TCP) ? VOS_TTL_MULTICAST : 0; */
        sock_options.ttl_multicast  = params->ttl;
        sock_options.no_mc_loop     = ((usage != TRDP_SOCK_MD_TCP) && (options & TRDP_OPTION_NO_MC_LOOP_BACK)) ? 1 : 0;
        sock_options.no_udp_crc     = ((usage != TRDP_SOCK_MD_TCP) && (options & TRDP_OPTION_NO_UDP_CHK)) ? 1 : 0;

        switch (usage)
        {
            case TRDP_SOCK_MD_UDP:
                sock_options.nonBlocking = TRUE; /* MD UDP sockets are always non blocking because they are polled */
            case TRDP_SOCK_PD:
                err = (TRDP_ERR_T) vos_sockOpenUDP(&iface[lIndex].sock, &sock_options);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "vos_sockOpenUDP failed! (Err: %d)\n", err);
                    *pIndex = TRDP_INVALID_SOCKET_INDEX;
                }
                else
                {
                    iface[lIndex].usage = 1;
                    *pIndex = lIndex;

                    if (rcvMostly)
                    {
                        /*  Only bind to local IP if we are not a multicast listener  */
                        if (0 == mcGroup)
                        {
                            err = (TRDP_ERR_T) vos_sockBind(iface[lIndex].sock, iface[lIndex].bindAddr, port);
                        }
                        else
                        {
                            err = (TRDP_ERR_T) vos_sockBind(iface[lIndex].sock, 0 /*mcGroup*/, port);
                        }

                        if (err != TRDP_NO_ERR)
                        {
                            vos_printLog(VOS_LOG_ERROR, "vos_sockBind() for UDP rcv failed! (Err: %d)\n", err);
                            *pIndex = TRDP_INVALID_SOCKET_INDEX;
                            break;
                        }

                        if (0 != mcGroup)
                        {

                            err = (TRDP_ERR_T) vos_sockJoinMC(iface[lIndex].sock, mcGroup, srcIP);
                            if (err != TRDP_NO_ERR)
                            {
                                vos_printLog(VOS_LOG_ERROR, "vos_sockJoinMC() for UDP rcv failed! (Err: %d)\n", err);
                                *pIndex = TRDP_INVALID_SOCKET_INDEX;
                                break;
                            }
                            else
                            {
                                if (trdp_SockAddJoin(iface[lIndex].mcGroups, mcGroup) == FALSE)
                                {
                                    vos_printLogStr(VOS_LOG_ERROR, "trdp_SockAddJoin() failed!\n");
                                }
                            }
                        }
                    }
                    else if (iface[lIndex].bindAddr != 0)
                    {
                        (void) vos_sockBind(iface[lIndex].sock, iface[lIndex].bindAddr, 0);
                    }

                    /*    Multicast sender shall be bound to an interface    */
                    if (iface[lIndex].bindAddr != 0)
                    {
                        err = (TRDP_ERR_T) vos_sockSetMulticastIf(iface[lIndex].sock, iface[lIndex].bindAddr);
                        if (err != TRDP_NO_ERR)
                        {
                            vos_printLog(VOS_LOG_ERROR, "vos_sockSetMulticastIf() for UDP snd failed! (Err: %d)\n", err);
                            *pIndex = TRDP_INVALID_SOCKET_INDEX;
                            break;
                        }
                    }

                }
                break;
            case TRDP_SOCK_MD_TCP:
                err = (TRDP_ERR_T) vos_sockOpenTCP(&iface[lIndex].sock, &sock_options);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "vos_sockOpenTCP() failed! (Err: %d)\n", err);
                    *pIndex = TRDP_INVALID_SOCKET_INDEX;
                }
                else
                {
                    iface[lIndex].usage = 0;
                    *pIndex = lIndex;
                }

                if (iface[lIndex].bindAddr != 0)
                {
                    (void) vos_sockBind(iface[lIndex].sock, iface[lIndex].bindAddr, 0);
                }
                break;
            default:
                *pIndex = TRDP_INVALID_SOCKET_INDEX;
                err     = TRDP_SOCK_ERR;
                break;
        }
    }
    else
    {
        err = TRDP_MEM_ERR;
    }

    printSocketUsage(iface);

    return err;
}


/**********************************************************************************************************************/
/** Handle the socket pool: if a received TCP socket is unused, the socket connection timeout is started.
 *  In Udp, Release a socket from our socket pool
 *  @param[in,out]  iface           socket pool
 *  @param[in]      lIndex          index of socket to release
 *  @param[in]      connectTimeout  time out
 *  @param[in]      checkAll        release all TCP pending sockets
 *
 */
void  trdp_releaseSocket (
    TRDP_SOCKETS_T  iface[],
    INT32           lIndex,
    UINT32          connectTimeout,
    BOOL8           checkAll)
{
    TRDP_ERR_T err = TRDP_PARAM_ERR;

    if (iface == NULL)
    {
        return;
    }

#if MD_SUPPORT
    if (checkAll == TRUE)
    {
        /* Check all the sockets */
        /* Close the morituri = TRUE sockets */
        for (lIndex = 0; lIndex < VOS_MAX_SOCKET_CNT; lIndex++)
        {
            if (iface[lIndex].tcpParams.morituri == TRUE)
            {
                vos_printLog(VOS_LOG_INFO, "The socket (Num = %d) will be closed\n", iface[lIndex].sock);

                err = (TRDP_ERR_T) vos_sockClose(iface[lIndex].sock);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "vos_sockClose() failed (Err:%d)\n", err);
                }

                /* Delete the socket from the iface */
                vos_printLog(VOS_LOG_INFO,
                             "Deleting socket from the iface (Sock: %d, lIndex: %d)\n",
                             iface[lIndex].sock, lIndex);
                iface[lIndex].sock = TRDP_INVALID_SOCKET_INDEX;
                iface[lIndex].sendParam.qos = 0;
                iface[lIndex].sendParam.ttl = 0;
                iface[lIndex].usage         = 0;
                iface[lIndex].bindAddr      = 0;
                iface[lIndex].type      = (TRDP_SOCK_TYPE_T) 0;
                iface[lIndex].rcvMostly = FALSE;
                iface[lIndex].tcpParams.cornerIp = 0;
                iface[lIndex].tcpParams.connectionTimeout.tv_sec    = 0;
                iface[lIndex].tcpParams.connectionTimeout.tv_usec   = 0;
                iface[lIndex].tcpParams.addFileDesc = FALSE;
                iface[lIndex].tcpParams.morituri    = FALSE;
            }
        }

    }
    else
#endif
    {
        /* Handle a specified socket */
        if (iface[lIndex].sock != VOS_INVALID_SOCKET &&
            (iface[lIndex].type == TRDP_SOCK_MD_UDP ||
             iface[lIndex].type == TRDP_SOCK_PD))
        {
            vos_printLog(VOS_LOG_DBG,
                         "Decrement the socket %d usage = %d\n",
                         iface[lIndex].sock,
                         iface[lIndex].usage);
            iface[lIndex].usage--;

            if (iface[lIndex].sock != VOS_INVALID_SOCKET &&
                iface[lIndex].usage <= 0)
            {
                /* Close that socket, nobody uses it anymore */
                err = (TRDP_ERR_T) vos_sockClose(iface[lIndex].sock);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLogStr(VOS_LOG_DBG, "Trying to close socket again?\n");
                }
                else
                {
                    vos_printLog(VOS_LOG_DBG, "Closed socket %d\n", iface[lIndex].sock);
                }
                iface[lIndex].sock = VOS_INVALID_SOCKET;
            }
        }
#if MD_SUPPORT
        else /* TCP socket */
        {
            if (iface[lIndex].sock != VOS_INVALID_SOCKET &&
                iface[lIndex].rcvMostly == FALSE)
            {
                vos_printLog(VOS_LOG_DBG,
                             "Decrement the socket %d usage = %d\n",
                             iface[lIndex].sock,
                             iface[lIndex].usage);

                iface[lIndex].usage--;

                if (iface[lIndex].usage <= 0)
                {
                    /* Start the socket connection timeout */
                    TRDP_TIME_T tmpt_interval, tmpt_now;

                    vos_printLog(VOS_LOG_INFO,
                                 "The Socket (Num = %d usage=0) ConnectionTimeout will be started\n",
                                 iface[lIndex].sock);

                    tmpt_interval.tv_sec    = connectTimeout / 1000000;
                    tmpt_interval.tv_usec   = connectTimeout % 1000000;

                    vos_getTime(&tmpt_now);
                    vos_addTime(&tmpt_now, &tmpt_interval);

                    memcpy(&iface[lIndex].tcpParams.connectionTimeout,
                           &tmpt_now,
                           sizeof(TRDP_TIME_T));
                }
            }
        }
#endif
    }
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

    if (0 == comId)
    {
        return 0;
    }

    /*    For process data look at the PD send queue only    */
    if (TRDP_MSG_PD == msgType ||
        TRDP_MSG_PP == msgType ||
        TRDP_MSG_PR == msgType)
    {
        /*    Loop thru all sessions    */
        while (pSession)
        {
            pSendElement = pSession->pSndQueue;
            while (pSendElement)
            {
                if (pSendElement->addr.comId == comId &&
                    (srcIpAddr == 0 || pSendElement->addr.srcIpAddr != srcIpAddr))
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
/** remove the sequence counter for the comID/source IP.
 *  The sequence counter should be reset if there was a packet time out.
 *
 *
 *  @param[in]      pElement            subscription element
 *  @param[in]      srcIP               Source IP address
 *  @param[in]      msgType             message type
 *
 *  @retval         none
 */

void trdp_resetSequenceCounter (
    PD_ELE_T        *pElement,
    TRDP_IP_ADDR_T  srcIP,
    TRDP_MSG_T      msgType)
{
    int index;

    if (pElement == NULL || pElement->pSeqCntList == NULL)
    {
        return;
    }
    /* Loop over entries */
    for (index = 0; index < pElement->pSeqCntList->curNoOfEntries; ++index)
    {
        if (srcIP == pElement->pSeqCntList->seq[index].srcIpAddr &&
            msgType == pElement->pSeqCntList->seq[index].msgType)
        {
            pElement->pSeqCntList->seq[index].lastSeqCnt = 0;
        }
    }
}

/**********************************************************************************************************************/
/** check and update the sequence counter for the comID/source IP.
 *  If the comID/srcIP is not found, update it and return 0 -
 *  else if already received, return 1
 *  On memory error, return -1
 *
 *  @param[in]      pElement            subscription element
 *  @param[in]      sequenceCounter     sequence counter to check
 *  @param[in]      srcIP               Source IP address
 *  @param[in]      msgType             type of the message
 *
 *  @retval         0 - no duplicate
 *                  1 - duplicate sequence counter
 *                 -1 - memory error
 */

int trdp_checkSequenceCounter (
    PD_ELE_T        *pElement,
    UINT32          sequenceCounter,
    TRDP_IP_ADDR_T  srcIP,
    TRDP_MSG_T      msgType)
{
    int index;

    if (pElement == NULL)
    {
        vos_printLogStr(VOS_LOG_DBG, "Parameter error\n");
        return -1;
    }

    if (pElement->pSeqCntList == NULL)
    {
        /* Allocate some space */
        pElement->pSeqCntList = (TRDP_SEQ_CNT_LIST_T *) vos_memAlloc(TRDP_SEQ_CNT_START_ARRAY_SIZE *
                                                                     sizeof(TRDP_SEQ_CNT_ENTRY_T) +
                                                                     sizeof(TRDP_SEQ_CNT_LIST_T));
        if (pElement->pSeqCntList == NULL)
        {
            return -1;
        }
        pElement->pSeqCntList->maxNoOfEntries   = TRDP_SEQ_CNT_START_ARRAY_SIZE;
        pElement->pSeqCntList->curNoOfEntries   = 0;
    }
    /* Loop over entries */
    for (index = 0; index < pElement->pSeqCntList->curNoOfEntries; ++index)
    {
        if (srcIP == pElement->pSeqCntList->seq[index].srcIpAddr &&
            msgType == pElement->pSeqCntList->seq[index].msgType)
        {
            /*        Is this packet a duplicate?    */
            if (pElement->pSeqCntList->seq[index].lastSeqCnt == 0 ||    /* first time after timeout */
                sequenceCounter != pElement->pSeqCntList->seq[index].lastSeqCnt)
            {
/*
                  vos_printLog(VOS_LOG_DBG,
                               "Rcv sequence: %u    last seq: %u\n",
                               sequenceCounter,
                               pElement->pSeqCntList->seq[index].lastSeqCnt);
                  vos_printLog(VOS_LOG_DBG, "-> new PD data found (SrcIp: %s comId %u)\n", vos_ipDotted(
                                   srcIP), pElement->addr.comId);
 */
                pElement->pSeqCntList->seq[index].lastSeqCnt = sequenceCounter;
                return 0;
            }
            else
            {
                vos_printLog(VOS_LOG_DBG,
                             "Rcv sequence: %u    last seq: %u\n",
                             sequenceCounter,
                             pElement->pSeqCntList->seq[index].lastSeqCnt);
                vos_printLog(VOS_LOG_DBG, "-> duplicated PD data ignored (SrcIp: %s comId %u)\n", vos_ipDotted(
                                 srcIP), pElement->addr.comId);
                return 1;
            }
        }
    }

    /* Not found in list, add new entry */
    if (pElement->pSeqCntList->curNoOfEntries >= pElement->pSeqCntList->maxNoOfEntries)
    {
        /* Allocate some more space */
        UINT16 newSize = 2 * pElement->pSeqCntList->curNoOfEntries;
        TRDP_SEQ_CNT_LIST_T *newList = (TRDP_SEQ_CNT_LIST_T *) vos_memAlloc(newSize * sizeof(TRDP_SEQ_CNT_ENTRY_T) +
                                                                            sizeof(TRDP_SEQ_CNT_LIST_T));
        if (newList == NULL)
        {
            return -1;
        }

        /* Copy old data into new, larger area */
        memcpy(newList, pElement->pSeqCntList, pElement->pSeqCntList->maxNoOfEntries *
               sizeof(TRDP_SEQ_CNT_ENTRY_T) +
               sizeof(TRDP_SEQ_CNT_LIST_T));
        vos_memFree(pElement->pSeqCntList);     /* Free old area */
        pElement->pSeqCntList = newList;
        pElement->pSeqCntList->maxNoOfEntries = newSize;
    }
    pElement->pSeqCntList->seq[pElement->pSeqCntList->curNoOfEntries].lastSeqCnt    = sequenceCounter;
    pElement->pSeqCntList->seq[pElement->pSeqCntList->curNoOfEntries].srcIpAddr     = srcIP;
    pElement->pSeqCntList->seq[pElement->pSeqCntList->curNoOfEntries].msgType       = msgType;
    pElement->pSeqCntList->curNoOfEntries++;
    vos_printLog(VOS_LOG_DBG, "Rcv sequence: %u\n", sequenceCounter);
    vos_printLog(VOS_LOG_DBG, "*** new sequence entry (SrcIp: %s comId %u)\n", vos_ipDotted(
                     srcIP), pElement->addr.comId);

    return 0;
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

BOOL8 trdp_isAddressed (const TRDP_URI_USER_T listUri, const TRDP_URI_USER_T destUri)
{
    return (vos_strnicmp(listUri, destUri, TRDP_DEST_URI_SIZE) == 0);
}
