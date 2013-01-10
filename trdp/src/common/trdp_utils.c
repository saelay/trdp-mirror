/******************************************************************************/
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
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
 */

/*******************************************************************************
 * INCLUDES
 */

#include "trdp_utils.h"
#include "trdp_if.h"

#ifdef TRDP_OPTION_LADDER
#include "vos_thread.h"
#include "trdp_ladder.h"
#include "trdp_ladder_app.h"

#endif /* TRDP_OPTION_LADDER */

/*******************************************************************************
 * DEFINES
 */

/*******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 *   Locals
 */
static INT32 sCurrentMaxSocketCnt = 0;

/******************************************************************************
 *   Globals
 */

#ifdef TRDP_OPTION_LADDER
/* Traffic Store Semaphore */
CHAR8 SEMAPHORE_TRAFFIC_STORE[] = "/semaphore_ts";        /* Traffic Store semaphore name */
mode_t PERMISSION_SEMAPHORE = 0777;                        /* Traffic Store semaphore permission is rwxrwxrwx */
struct VOS_SEMA trafficStoreSemaphore;                    /* Semaphore for Traffic Store */
UINT32 semaphoreTimeout = 1000;                            /* semaphore take timeout : Max. time in us to wait, 0 means forever */
sem_t *pSemaphore;                                        /* pointer to Traffic Store semaphore */

/* Traffic Store */
CHAR8 TRAFFIC_STORE[] = "/ladder_ts";                    /* Traffic Store shared memory name */
mode_t PERMISSION     = 0666;                            /* Traffic Store permission is rw-rw-rw- */
UINT8 *pTrafficStoreAddr;                                /* pointer to pointer to Traffic Store Address */
struct VOS_SHRD  trafficStoreHandle;                    /* Traffic Store Handle */

/* PDComLadderThread */
CHAR8 pdComLadderThreadName[] ="PDComLadderThread";        /* Thread name is PDComLadder Thread. */
BOOL pdComLadderThreadActiveFlag = FALSE;                /* PDComLaader Thread active/noactive Flag :active=TRUE, nonActive=FALSE */
BOOL pdComLadderThreadStartFlag = FALSE;                /* PDComLadder Thread instruction start up Flag :start=TRUE, stop=FALSE */

/* Sub-net */
UINT32 usingSubnetId;                                    /* Using SubnetId */

#endif /* TRDP_OPTION_LADDER */
/******************************************************************************/
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


/******************************************************************************/
/** Get the packet size from the raw data size
 *
 *  @param[in]      dataSize            net data size (without padding or FCS)
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

/******************************************************************************/
/** Return the element with same comId
 *
 *  @param[in]      pHead           pointer to head of queue
 *  @param[in]      comId           ComID to search for
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

/******************************************************************************/
/** Return the element with same comId and IP addresses
 *
 *  @param[in]      pHead           pointer to head of queue
 *  @param[in]      addr            Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
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

/******************************************************************************/
/** Return the element with same comId and IP addresses
 *
 *  @param[in]      pHead           pointer to head of queue
 *  @param[in]      addr            Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
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

/******************************************************************************/
/** Return the element with same comId from MD queue
 *
 *  @param[in]      pHead           pointer to head of queue
 *  @param[in]      addr            Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
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

/******************************************************************************/
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

/******************************************************************************/
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

/******************************************************************************/
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


/******************************************************************************/
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


/******************************************************************************/
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

/******************************************************************************/
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


/******************************************************************************/
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

/******************************************************************************/
/** Handle the socket pool: Request a socket from our socket pool
 *
 *  @param[in,out]  iface           socket pool
 *  @param[in]      params          parameters to use
 *  @param[in]      srcIP           IP to bind to (0 = any address)
 *  @param[in]      usage           type and port to bind to
 *  @param[in]      options         blocking/nonblocking
 *  @param[in]      rcvOnly         only used for receiving
 *  @param[out]     pIndex          returned index of socket pool
 *  @retval            TRDP_NO_ERR
 *  @retval            TRDP_PARAM_ERR
 */
TRDP_ERR_T  trdp_requestSocket (
    TRDP_SOCKETS_T          iface[],
    const TRDP_SEND_PARAM_T *params,
    TRDP_IP_ADDR_T          srcIP,
    TRDP_SOCK_TYPE_T        usage,
    TRDP_OPTION_T           options,
    BOOL                    rcvOnly,
    INT32                   *pIndex,
    TRDP_IP_ADDR_T            cornerIp)
{
    VOS_SOCK_OPT_T  sock_options;
    INT32           index, emptySock = -1;
    TRDP_ERR_T      err     = TRDP_NO_ERR;
    UINT32          port    = TRDP_MD_TCP_PORT; /* port, where the server listens on FIXME (this could be found somewhere in the
                                   configuration file)*/

    if (iface == NULL || params == NULL || pIndex == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /*  We loop through the table of open/used sockets,
        if we find a usable one (with the same socket options) we take it.
        We remember already closed sockets on the way to be able to fill up gaps  */
    for (index = 0; index < sCurrentMaxSocketCnt; index++)
    {
        if (iface[index].sock != -1 &&
            iface[index].bindAddr == srcIP &&
            iface[index].type == usage &&
            iface[index].sendParam.qos == params->qos &&
            iface[index].sendParam.ttl == params->ttl &&
            iface[index].rcvOnly == rcvOnly &&
            ((usage != TRDP_SOCK_MD_TCP) ||
            ((usage == TRDP_SOCK_MD_TCP) && (iface[index].tcpParams.cornerIp == cornerIp))))
        {
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
        iface[index].tcpParams.connectionTimeout.tv_sec = 0;
        iface[index].tcpParams.connectionTimeout.tv_usec = 0;
        iface[index].tcpParams.cornerIp = cornerIp;

        sock_options.qos    = params->qos;
        sock_options.ttl    = params->ttl;
        sock_options.ttl_multicast  = VOS_TTL_MULTICAST;
        sock_options.reuseAddrPort  = TRUE;
        sock_options.nonBlocking    = (options == TRDP_OPTION_BLOCK) ? FALSE : TRUE;

        if((usage == TRDP_SOCK_MD_TCP) && (rcvOnly == TRUE))
        {
            iface[index].sock       = *pIndex;
            *pIndex                    = index;
            iface[index].usage         = 0;
            return err;
        }

        switch (usage)
        {
            case TRDP_SOCK_PD:
                port = TRDP_PD_UDP_PORT; /* FIXME configuration file! */
            case TRDP_SOCK_MD_UDP:
                if (port <= 0) /* only set the port, when this is a MD communication */
                {
                    port = TRDP_MD_UDP_PORT; /* FIXME configuration file! */
                }
                sock_options.nonBlocking = TRUE; /* MD UDP sockets are always non blocking because they are polled */
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
                        err = (TRDP_ERR_T) vos_sockBind(iface[index].sock, iface[index].bindAddr, port);
                        if (err != TRDP_NO_ERR)
                        {
                            vos_printf(VOS_LOG_ERROR, "vos_sockBind() for UDP rcv failed! (Err: %d)\n", err);
                            *pIndex = -1;
                            break;
                        }
                    }
                    err = (TRDP_ERR_T) vos_sockSetMulticastIf(iface[index].sock, iface[index].bindAddr);
                    if (err != TRDP_NO_ERR)
                    {
                        vos_printf(VOS_LOG_ERROR, "vos_sockSetMulticastIf() failed! (Err: %d)\n", err);
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

/******************************************************************************/
/** Handle the socket pool: Release a socket from our socket pool
 *
 *  @param[in,out]  iface           socket pool
 *  @param[in]      index           index of socket to release
 *  @retval            TRDP_NO_ERR
 *  @retval            TRDP_PARAM_ERR
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

/******************************************************************************/
/** Get the initial sequence counter for the comID/message type and subnet (source IP).
 *  If the comID/srcIP is not found elsewhere, return 0 -
 *  else return its current sequence number (the redundant packet needs the same seqNo)
 *
 *  Note: The standard demands that sequenceCounter is managed per comID/msgType at each publisher,
 *        but shall be the same for redundant telegrams (subnet/srcIP).
 *
 *  @param[in]      comId           comID to look for
 *  @param[in]      msgType            PD/MD type
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
 *  @retval            return the sequence number
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

#ifdef TRDP_OPTION_LADDER
/******************************************************************************/
/** Initialize TRDP Ladder Support
 *  Create Traffic Store semaphore, Traffic Store, PDComLadderThread.
 *
 *    Note:
 *
 *    @retval            TRDP_NO_ERR
 *    @retval            TRDP_SEMA_ERR
 */
TRDP_ERR_T trdp_ladder_init (void)
{
    /* Traffic Store */
    extern CHAR8 SEMAPHORE_TRAFFIC_STORE[];            /* Traffic Store semaphore name */
    extern CHAR8 TRAFFIC_STORE[];                    /* Traffic Store shared memory name */
    extern mode_t PERMISSION_SEMAPHORE;                /* Traffic Store semaphore permission is rwxrwxrwx */
    extern struct VOS_SHRD  trafficStoreHandle;    /* Traffic Store Handle */
    VOS_SHRD_T  pTrafficStoreHandle;                /* Pointer to Traffic Store Handle */
    extern UINT8 *pTrafficStoreAddr;                /* pointer to pointer to Traffic Store Address */
    UINT32 trafficStoreSize = TRAFFIC_STORE_SIZE;    /* Traffic Store Size : 64KB */

    /* PDComLadderThread */
    extern CHAR8 pdComLadderThreadName[];            /* Thread name is PDComLadder Thread. */
    extern BOOL pdComLadderThreadActiveFlag;        /* PDComLaader Thread active/non-active Flag :active=TRUE, nonActive=FALSE */
    VOS_THREAD_T pdComLadderThread = NULL;            /* Thread handle */

    /* Traffic Store Semaphore */
    extern struct VOS_SEMA trafficStoreSemaphore;    /* Semaphore for Traffic Store */
    extern UINT32 semaphoreTimeout;                    /* semaphore take timeout : Max. time in us to wait, 0 means forever */
    VOS_SEMA_T pTrafficStoreSemaphore;                /* Pointer to Semaphore for Traffic Store */


    /* Traffic Store Create */
    /* Traffic Store Semaphore Create */
    TRDP_ERR_T ret = TRDP_SEMA_ERR;

    /*    PDComLadder Thread Active ? */
    if (pdComLadderThreadActiveFlag == TRUE)
    {
        return TRDP_NO_ERR;
    }

    /* Set Traffic Store Semaphore parameter */
    trafficStoreSemaphore.semaphoreName = SEMAPHORE_TRAFFIC_STORE;
    trafficStoreSemaphore.oflag = (O_CREAT | O_EXCL);
    trafficStoreSemaphore.permission = PERMISSION_SEMAPHORE;
    trafficStoreSemaphore.pSemaphore = NULL;

    pTrafficStoreSemaphore = &trafficStoreSemaphore;

    if (vos_semaCreate(&pTrafficStoreSemaphore, VOS_SEMA_FULL) != VOS_NO_ERR)
    {
        if (pdComLadderThreadActiveFlag == FALSE)
        {
            vos_threadInit();
            if (vos_threadCreate(&pdComLadderThread,
                                    pdComLadderThreadName,
                                    VOS_THREAD_POLICY_OTHER,
                                    0,
                                    0,
                                    0,
                                    (void *)PDComLadder,
                                    NULL) == VOS_NO_ERR)
            {
                pdComLadderThreadActiveFlag = TRUE;
                return TRDP_NO_ERR;
            }
            else
            {
                vos_printf(VOS_LOG_ERROR, "TRDP PDComLadderThread Create failed\n");
                return ret;
            }
        }
    }

    /* Lock Traffic Store Semaphore */
    if ((vos_semaTake(&trafficStoreSemaphore, semaphoreTimeout)) != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "TRDP Traffic Store semaphore Lock failed\n");
        return ret;
    }

    /* Create the Traffic Store */
    trafficStoreHandle.semaphoreName = TRAFFIC_STORE;
    pTrafficStoreHandle = &trafficStoreHandle;
    if ((vos_sharedOpen(TRAFFIC_STORE, &pTrafficStoreHandle, &pTrafficStoreAddr, &trafficStoreSize)) != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "TRDP Traffic Store Creat failed\n");
        ret = TRDP_MEM_ERR;
        return ret;
    }

    /* Traffic Store semaphore unlock */
    if ((vos_semaGive(&trafficStoreSemaphore)) != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "TRDP Traffic Store semaphore Unlock failed\n");
        return ret;
    }

    /*    PDComLadder Thread Create */
    if (pdComLadderThreadActiveFlag == FALSE)
    {
        vos_threadInit();
        if (vos_threadCreate(&pdComLadderThread,
                                pdComLadderThreadName,
                                VOS_THREAD_POLICY_OTHER,
                                0,
                                0,
                                0,
                                (void *)PDComLadder,
                                NULL) == TRDP_NO_ERR)
        {
            pdComLadderThreadActiveFlag = TRUE;
            ret = TRDP_NO_ERR;
        }
        else
        {
            vos_printf(VOS_LOG_ERROR, "TRDP PDComLadderThread Create failed\n");
            return ret;
        }
    }
    else
    {
        ret = TRDP_NO_ERR;
    }

    return ret;    /* TRDP_NO_ERR */
}

#endif /* TRDP_OPTION_LADDER */
