/**********************************************************************************************************************/
/**
 * @file            trdp_if.c
 *
 * @brief           Functions for ECN communication
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
 *      BL 2013-06-24: ID 125: Time-out handling and ready descriptors fixed
 *
 *      BL 2013-02-01: ID 53: Zero datset size fixed for PD
 *
 *      BL 2013-01-25: ID 20: Redundancy handling fixed
 *
 *      BL 2013-01-08: LADDER: Removed/Changed some ladder specific code in tlp_subscribe()
 *
 *      BL 2012-12-03: ID 1: "using uninitialized PD_ELE_T.pullIpAddress variable"
 *                     ID 2: "uninitialized PD_ELE_T newPD->pNext in tlp_subscribe()"
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_if_light.h"
#include "trdp_utils.h"
#include "trdp_pdcom.h"
#include "trdp_stats.h"
#include "vos_sock.h"
#include "vos_mem.h"

#if MD_SUPPORT
#include "trdp_mdcom.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 * LOCALS
 */

const TRDP_VERSION_T        trdpVersion = {TRDP_VERSION, TRDP_RELEASE, TRDP_UPDATE, TRDP_EVOLUTION};
static TRDP_APP_SESSION_T   sSession        = NULL;
static VOS_MUTEX_T          sSessionMutex   = NULL;
static BOOL sInited = FALSE;


/******************************************************************************
 * LOCAL FUNCTIONS
 */


/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Check if the session handle is valid
 *
 *
 *  @param[in]    pSessionHandle        pointer to packet data (dataset)
 *
 *  @retval       TRUE                  is valid
 *  @retval       FALSE                 is invalid
 */
BOOL    trdp_isValidSession (
    TRDP_APP_SESSION_T pSessionHandle)
{
    TRDP_SESSION_PT pSession = NULL;
    BOOL found = FALSE;

    if (pSessionHandle == NULL)
    {
        return FALSE;
    }

    if (vos_mutexLock(sSessionMutex) != VOS_NO_ERR)
    {
        return FALSE;
    }

    pSession = sSession;

    while (pSession)
    {
        if (pSession == (TRDP_SESSION_PT) pSessionHandle)
        {
            found = TRUE;
            break;
        }
        pSession = pSession->pNext;
    }

    if (vos_mutexUnlock(sSessionMutex) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return found;
}

/**********************************************************************************************************************/
/** Get the session queue head pointer
 *
 *    @retval            &sSession
 */
TRDP_APP_SESSION_T *trdp_sessionQueue (void)
{
    return (TRDP_APP_SESSION_T *)sSession;
}

/**********************************************************************************************************************/
/** Initialize the TRDP stack.
 *
 *  tlc_init returns in pAppHandle a unique handle to be used in further calls to the stack.
 *
 *  @param[in]      pPrintDebugString   Pointer to debug print function
 *  @param[in]      pMemConfig          Pointer to memory configuration
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_MEM_ERR        memory allocation failed
 *  @retval         TRDP_PARAM_ERR      initialization error
 */
EXT_DECL TRDP_ERR_T tlc_init (
    const TRDP_PRINT_DBG_T  pPrintDebugString,
    const TRDP_MEM_CONFIG_T *pMemConfig)
{
    TRDP_ERR_T ret = TRDP_NO_ERR;

    /*    Init memory subsystem and the session mutex */
    if (sInited == FALSE)
    {
        /*    Initialize VOS    */
        ret = (TRDP_ERR_T) vos_init(NULL, pPrintDebugString);

        if (ret != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "vos_init() failed (Err: %d)\n", ret);
        }
        else
        {
            if (pMemConfig == NULL)
            {
                ret = (TRDP_ERR_T) vos_memInit(NULL, 0, NULL);
            }
            else
            {
                ret = (TRDP_ERR_T) vos_memInit(pMemConfig->p, pMemConfig->size, pMemConfig->prealloc);
            }

            if (ret != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "vos_memInit() failed (Err: %d)\n", ret);
            }
            else
            {
                ret = (TRDP_ERR_T) vos_mutexCreate(&sSessionMutex);

                if (ret != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "vos_mutexCreate() failed (Err: %d)\n", ret);
                }
            }
        }

        if (ret == TRDP_NO_ERR)
        {
            sInited = TRUE;
            vos_printLog(VOS_LOG_INFO, "TRDP Stack Version %s: successfully initiated\n", tlc_getVersionString());
        }
    }
    else
    {
        vos_printLog(VOS_LOG_ERROR, "TRDP already initalised\n");

        ret = TRDP_INIT_ERR;
    }

    return ret;
}

/**********************************************************************************************************************/
/** Open a session with the TRDP stack.
 *
 *  tlc_openSession returns in pAppHandle a unique handle to be used in further calls to the stack.
 *
 *  @param[out]     pAppHandle          A handle for further calls to the trdp stack
 *  @param[in]      ownIpAddr           Own IP address, can be different for each process in multihoming systems,
 *                                      if zero, the default interface / IP will be used.
 *  @param[in]      leaderIpAddr        IP address of redundancy leader
 *  @param[in]      pMarshall           Pointer to marshalling configuration
 *  @param[in]      pPdDefault          Pointer to default PD configuration
 *  @param[in]      pMdDefault          Pointer to default MD configuration
 *  @param[in]      pProcessConfig      Pointer to process configuration
 *                                      only option parameter is used here to define session behavior
 *                                      all other parameters are only used to feed statistics
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       not yet inited
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_SOCK_ERR       socket error
 */
EXT_DECL TRDP_ERR_T tlc_openSession (
    TRDP_APP_SESSION_T              *pAppHandle,
    TRDP_IP_ADDR_T                  ownIpAddr,
    TRDP_IP_ADDR_T                  leaderIpAddr,
    const TRDP_MARSHALL_CONFIG_T    *pMarshall,
    const TRDP_PD_CONFIG_T          *pPdDefault,
    const TRDP_MD_CONFIG_T          *pMdDefault,
    const TRDP_PROCESS_CONFIG_T     *pProcessConfig)
{
    TRDP_ERR_T      ret         = TRDP_NO_ERR;
    TRDP_SESSION_PT pSession    = NULL;
    TRDP_PUB_T      dummyPubHndl;
    TRDP_SUB_T      dummySubHandle;

    if (pAppHandle == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "tlc_openSession() failed\n");
        return TRDP_PARAM_ERR;
    }

    /*    Check if we were inited */
    if (sInited == FALSE)
    {
        vos_printLog(VOS_LOG_ERROR, "tlc_openSession() called uninitialized\n");
        return TRDP_INIT_ERR;
    }

    pSession = (TRDP_SESSION_PT) vos_memAlloc(sizeof(TRDP_SESSION_T));
    if (pSession == NULL)
    {
        /* vos_memDelete(NULL); */
        vos_printLog(VOS_LOG_ERROR, "vos_memAlloc() failed\n");
        return TRDP_MEM_ERR;
    }

    memset(pSession, 0, sizeof(TRDP_SESSION_T));

    pSession->realIP    = ownIpAddr;
    pSession->virtualIP = leaderIpAddr;

    if (pProcessConfig != NULL)
    {
        pSession->option = pProcessConfig->options;
        pSession->stats.processCycle    = pProcessConfig->cycleTime;
        pSession->stats.processPrio     = pProcessConfig->priority;
        vos_strncpy(pSession->stats.hostName, pProcessConfig->hostName, TRDP_MAX_LABEL_LEN);
        vos_strncpy(pSession->stats.leaderName, pProcessConfig->leaderName, TRDP_MAX_LABEL_LEN);
    }

    if (pMarshall != NULL)
    {
        pSession->marshall = *pMarshall;
    }

    if (pPdDefault != NULL)
    {
        pSession->pdDefault = *pPdDefault;

        /* check whether default values needed or not */
        if (pSession->pdDefault.flags == TRDP_FLAGS_DEFAULT)
        {
            pSession->pdDefault.flags = TRDP_FLAGS_NONE;
        }

        if (pSession->pdDefault.port == 0)
        {
            pSession->pdDefault.port = TRDP_PD_UDP_PORT;
        }

        if (pSession->pdDefault.timeout == 0)
        {
            pSession->pdDefault.timeout = TRDP_PD_DEFAULT_TIMEOUT;
        }

        if (pSession->pdDefault.toBehavior == TRDP_TO_DEFAULT)
        {
            pSession->pdDefault.toBehavior = TRDP_TO_SET_TO_ZERO;
        }

        if (pSession->pdDefault.sendParam.ttl == 0)
        {
            pSession->pdDefault.sendParam.ttl = TRDP_PD_DEFAULT_TTL;
        }

        if (pSession->pdDefault.sendParam.qos == 0)
        {
            pSession->pdDefault.sendParam.qos = TRDP_PD_DEFAULT_QOS;
        }
    }
    else
    {
        pSession->pdDefault.pfCbFunction    = NULL;
        pSession->pdDefault.pRefCon         = NULL;
        pSession->pdDefault.flags           = TRDP_FLAGS_NONE;
        pSession->pdDefault.timeout         = TRDP_PD_DEFAULT_TIMEOUT;
        pSession->pdDefault.toBehavior      = TRDP_TO_SET_TO_ZERO;
        pSession->pdDefault.port            = TRDP_PD_UDP_PORT;
        pSession->pdDefault.sendParam.qos   = TRDP_PD_DEFAULT_QOS;
        pSession->pdDefault.sendParam.ttl   = TRDP_PD_DEFAULT_TTL;
        /*       pSession->pdDefault.sendParam.retries   = 0; */
    }

#if MD_SUPPORT

    if (pMdDefault != NULL)
    {
        pSession->mdDefault = *pMdDefault;

        /* check whether default values needed or not */
        if (pSession->mdDefault.tcpPort == 0)
        {
            pSession->mdDefault.tcpPort = TRDP_MD_TCP_PORT;
        }

        if (pSession->mdDefault.udpPort == 0)
        {
            pSession->mdDefault.udpPort = TRDP_MD_UDP_PORT;
        }

        if (pSession->mdDefault.confirmTimeout == 0)
        {
            pSession->mdDefault.confirmTimeout = TRDP_MD_DEFAULT_CONFIRM_TIMEOUT;
        }

        if (pSession->mdDefault.connectTimeout == 0)
        {
            pSession->mdDefault.connectTimeout = TRDP_MD_DEFAULT_CONNECTION_TIMEOUT;
        }

        if (pSession->mdDefault.sendingTimeout == 0)
        {
            pSession->mdDefault.sendingTimeout = TRDP_MD_DEFAULT_SENDING_TIMEOUT;
        }

        if (pSession->mdDefault.replyTimeout == 0)
        {
            pSession->mdDefault.replyTimeout = TRDP_MD_DEFAULT_REPLY_TIMEOUT;
        }

        if (pSession->mdDefault.sendParam.ttl == 0)
        {
            pSession->mdDefault.sendParam.ttl = TRDP_MD_DEFAULT_TTL;
        }

        if (pSession->mdDefault.sendParam.qos == 0)
        {
            pSession->mdDefault.sendParam.qos = TRDP_MD_DEFAULT_QOS;
        }

        if (pSession->mdDefault.maxNumSessions == 0)
        {
            pSession->mdDefault.maxNumSessions = TRDP_MD_MAX_NUM_SESSIONS;
        }
    }
    else
    {
        pSession->mdDefault.pfCbFunction    = NULL;
        pSession->mdDefault.pRefCon         = NULL;
        pSession->mdDefault.confirmTimeout  = TRDP_MD_DEFAULT_CONFIRM_TIMEOUT;
        pSession->mdDefault.connectTimeout  = TRDP_MD_DEFAULT_CONNECTION_TIMEOUT;
        pSession->mdDefault.replyTimeout    = TRDP_MD_DEFAULT_REPLY_TIMEOUT;
        pSession->mdDefault.flags           = TRDP_FLAGS_NONE;
        pSession->mdDefault.udpPort         = TRDP_MD_UDP_PORT;
        pSession->mdDefault.tcpPort         = TRDP_MD_TCP_PORT;
        pSession->mdDefault.sendParam.qos   = TRDP_MD_DEFAULT_QOS;
        pSession->mdDefault.sendParam.ttl   = TRDP_MD_DEFAULT_TTL;
#ifdef TRDP_RETRIES
        pSession->mdDefault.sendParam.retries = TRDP_MD_DEFAULT_RETRIES;
#endif
    }

    /* zero is a valid file/socket descriptor! */
    pSession->tcpFd.listen_sd = -1;

#endif

    ret = (TRDP_ERR_T) vos_mutexCreate(&pSession->mutex);

    if (ret != TRDP_NO_ERR)
    {
        vos_memFree(pSession);
        vos_printLog(VOS_LOG_ERROR, "vos_mutexCreate() failed (Err: %d)\n", ret);
        return ret;
    }

    vos_clearTime(&pSession->nextJob);
    vos_getTime(&pSession->initTime);

    /*    Clear the socket pool    */
    trdp_initSockets(pSession->iface);

#if MD_SUPPORT
    /* Initialize pointers to Null in the incomplete message structure */
    trdp_initUncompletedTCP(pSession);
#endif

    /*    Clear the statistics for this session */
    trdp_initStats(pSession);

    pSession->stats.ownIpAddr       = ownIpAddr;
    pSession->stats.leaderIpAddr    = leaderIpAddr;


    /*    Queue the session in    */
    ret = (TRDP_ERR_T) vos_mutexLock(sSessionMutex);

    if (ret != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_mutexLock() failed (Err: %d)\n", ret);
    }
    else
    {
        pSession->pNext = sSession;
        sSession        = pSession;
        *pAppHandle     = pSession;

        /*  Publish our statistics packet   */
        ret = tlp_publish(pSession,                     /*    our application identifier    */
                          &dummyPubHndl,                /*    our pulication identifier     */
                          TRDP_GLOBAL_STATISTICS_COMID, /*    ComID to send                 */
                          0,                            /*    local consist only            */
                          0,                            /*    default source IP             */
                          0,                            /*    where to send to              */
                          0,                            /*    Cycle time in ms              */
                          0,                            /*    not redundant                 */
                          TRDP_FLAGS_NONE,              /*    No callbacks                  */
                          NULL,                         /*    default qos and ttl           */
                          NULL,                         /*    initial data                  */
                          sizeof(TRDP_STATISTICS_T));

        /*  Subscribe our request packet   */
        if (ret == TRDP_NO_ERR)
        {
            ret = tlp_subscribe(pSession,               /*    our application identifier    */
                                &dummySubHandle,        /*    our subscription identifier   */
                                NULL,
                                TRDP_STATISTICS_REQUEST_COMID, /*    ComID                         */
                                0,                      /*    topocount: local consist only */
                                0,                      /*    Source IP filter              */
                                0,
                                0,                      /*    Default destination (or MC Group)   */
                                TRDP_FLAGS_NONE,        /*    packet flags                  */
                                0,                      /*    Time out in us                */
                                TRDP_TO_DEFAULT,        /*    delete invalid data on timeout  */
                                0);                     /*    net data size                 */
        }
        if (ret == TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "TRDP session opened successfully\n");
        }
        if (vos_mutexUnlock(sSessionMutex) != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Close a session.
 *  Clean up and release all resources of that session
 *
 *  @param[in]      appHandle             The handle returned by tlc_openSession
 *
 *  @retval         TRDP_NO_ERR           no error
 *  @retval         TRDP_NOINIT_ERR       handle invalid
 *  @retval         TRDP_PARAM_ERR        handle NULL
 */

EXT_DECL TRDP_ERR_T tlc_closeSession (
    TRDP_APP_SESSION_T appHandle)
{
    TRDP_SESSION_PT pSession = NULL;
    BOOL found = FALSE;
    TRDP_ERR_T      ret = TRDP_NOINIT_ERR;

    /*    Find the session    */
    if (appHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    ret = (TRDP_ERR_T) vos_mutexLock(sSessionMutex);

    if (ret != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_mutexLock() failed (Err: %d)\n", ret);
    }
    else
    {
        pSession = sSession;

        if (sSession == (TRDP_SESSION_PT) appHandle)
        {
            sSession    = sSession->pNext;
            found       = TRUE;
        }
        else
        {
            while (pSession)
            {
                if (pSession->pNext == (TRDP_SESSION_PT) appHandle)
                {
                    pSession->pNext = pSession->pNext->pNext;
                    found = TRUE;
                    break;
                }
                pSession = pSession->pNext;
            }
        }

        /*    At this point we removed the session from the queue    */
        if (found)
        {
            pSession = (TRDP_SESSION_PT) appHandle;

            /*    Take the session mutex to prevent someone sitting on the branch while we cut it    */
            ret = (TRDP_ERR_T) vos_mutexLock(pSession->mutex);

            if (ret != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "vos_mutexLock() failed (Err: %d)\n", ret);
            }
            else
            {
                /*    Release all allocated sockets and memory    */
                while (pSession->pSndQueue != NULL)
                {
                    PD_ELE_T *pNext = pSession->pSndQueue->pNext;

                    /*    Only close socket if not used anymore    */
                    trdp_releaseSocket(pSession->iface, pSession->pSndQueue->socketIdx, 0, FALSE);
                    vos_memFree(pSession->pSndQueue);
                    pSession->pSndQueue = pNext;
                }

#if MD_SUPPORT
                if (pSession->pMDRcvEle != NULL)
                {
                    if (pSession->pMDRcvEle->pPacket != NULL)
                    {
                        vos_memFree(pSession->pMDRcvEle->pPacket);
                    }
                    vos_memFree(pSession->pMDRcvEle);
                    pSession->pMDRcvEle = NULL;
                }

                /*    Release all allocated sockets and memory    */
                while (pSession->pMDSndQueue != NULL)
                {
                    MD_ELE_T *pNext = pSession->pMDSndQueue->pNext;

                    /*    Only close socket if not used anymore    */
                    trdp_releaseSocket(pSession->iface,
                                       pSession->pMDSndQueue->socketIdx,
                                       pSession->mdDefault.connectTimeout,
                                       FALSE);
                    trdp_mdFreeSession(pSession->pMDSndQueue);
                    pSession->pMDSndQueue = pNext;
                }
                /*    Release all allocated sockets and memory    */
                while (pSession->pMDRcvQueue != NULL)
                {
                    MD_ELE_T *pNext = pSession->pMDRcvQueue->pNext;

                    /*    Only close socket if not used anymore    */
                    trdp_releaseSocket(pSession->iface,
                                       pSession->pMDRcvQueue->socketIdx,
                                       pSession->mdDefault.connectTimeout,
                                       FALSE);
                    trdp_mdFreeSession(pSession->pMDRcvQueue);
                    pSession->pMDRcvQueue = pNext;
                }
                /*    Release all allocated sockets and memory    */
                while (pSession->pMDListenQueue != NULL)
                {
                    MD_LIS_ELE_T *pNext = pSession->pMDListenQueue->pNext;

                    /*    Only close socket if not used anymore    */
                    if (pSession->pMDListenQueue->socketIdx != -1)
                    {
                        trdp_releaseSocket(pSession->iface,
                                           pSession->pMDListenQueue->socketIdx,
                                           pSession->mdDefault.connectTimeout,
                                           FALSE);
                    }
                    vos_memFree(pSession->pMDListenQueue);
                    pSession->pMDListenQueue = pNext;
                }
#endif
                if (vos_mutexUnlock(pSession->mutex) != VOS_NO_ERR)
                {
                    vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
                }

                vos_mutexDelete(pSession->mutex);
                vos_memFree(pSession);
            }
        }
        if (vos_mutexUnlock(sSessionMutex) != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Un-Initialize.
 *  Clean up and close all sessions. Mainly used for debugging/test runs. No further calls to library allowed
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       no error
 *  @retval         TRDP_MEM_ERR        TrafficStore nothing
 *  @retval         TRDP_MUTEX_ERR      TrafficStore mutex err
 */
EXT_DECL TRDP_ERR_T tlc_terminate (void)
{
    TRDP_ERR_T ret = TRDP_NO_ERR;

    if (sInited == TRUE)
    {
        /*    Close all sessions    */
        while (sSession != NULL)
        {
            TRDP_ERR_T err;

            err = tlc_closeSession(sSession);
            if (err != TRDP_NO_ERR)
            {
                /* save the error code in case of an error */
                ret = err;
                vos_printLog(VOS_LOG_ERROR, "tlc_closeSession() failed (Err: %d)\n", ret);
            }
        }

        /* Close stop timers, release memory  */
        vos_terminate();
        sInited = FALSE;
    }
    else
    {
        ret = TRDP_NOINIT_ERR;
    }
    return ret;
}

/**********************************************************************************************************************/
/** Re-Initialize.
 *  Should be called by the application when a link-down/link-up event has occured during normal operation.
 *  We need to re-join the multicast groups...
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      handle NULL
 */
EXT_DECL TRDP_ERR_T tlc_reinitSession (
    TRDP_APP_SESSION_T appHandle)
{
    PD_ELE_T    *iterPD;
    TRDP_ERR_T  ret;

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
        if (ret == TRDP_NO_ERR)
        {
            /*    Walk over the registered PDs */
            for (iterPD = appHandle->pSndQueue; iterPD != NULL; iterPD = iterPD->pNext)
            {
                if (iterPD->privFlags & TRDP_MC_JOINT &&
                    iterPD->socketIdx != -1)
                {
                    /*    Join the MC group again    */
                    ret = (TRDP_ERR_T) vos_sockJoinMC(appHandle->iface[iterPD->socketIdx].sock,
                                                      iterPD->addr.mcGroup,
                                                      appHandle->realIP);
                }
            }

            if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
            {
                vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }
    else
    {
        ret = TRDP_NOINIT_ERR;
    }
    return ret;
}

/**********************************************************************************************************************/
/** Return a human readable version representation.
 *    Return string in the form 'v.r.u.b'
 *
 *  @retval            const string
 */
const char *tlc_getVersionString (void)
{
    static CHAR8 version[16];

    (void) vos_snprintf(version,
                        sizeof(version),
                        "%d.%d.%d.%d",
                        TRDP_VERSION,
                        TRDP_RELEASE,
                        TRDP_UPDATE,
                        TRDP_EVOLUTION);

    return version;
}

/**********************************************************************************************************************/
/** Return version.
 *    Return pointer to version structure
 *
 *  @retval            TRDP_VERSION_T
 */
EXT_DECL const TRDP_VERSION_T *tlc_getVersion (void)
{
    return &trdpVersion;
}

/**********************************************************************************************************************/
/** Do not send non-redundant PDs when we are follower.
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      redId               will be set for all ComID's with the given redId, 0 to change for all redId
 *  @param[in]      leader              TRUE if we send
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error / redId not existing
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlp_setRedundant (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              redId,
    BOOL                leader)
{
    TRDP_ERR_T  ret = TRDP_NOINIT_ERR;
    PD_ELE_T    *iterPD;
    BOOL        found = FALSE;

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
        if (TRDP_NO_ERR == ret)
        {
            /*    Set the redundancy flag for every PD with the specified ID */
            for (iterPD = appHandle->pSndQueue; NULL != iterPD; iterPD = iterPD->pNext)
            {
                if (iterPD->redId == redId ||       /* packet redundant ID matches                      */
                    (iterPD->redId && 0 == redId))  /* all set redundant ID are targeted if redID == 0  */
                {
                    if (TRUE == leader)
                    {
                        iterPD->privFlags = (TRDP_PRIV_FLAGS_T) (iterPD->privFlags & ~TRDP_REDUNDANT);
                    }
                    else
                    {
                        iterPD->privFlags |= TRDP_REDUNDANT;
                    }
                    found = TRUE;
                }
            }

            /*  It would lead to an error, if the user tries to change the redundancy on a non-existant group, because
             the leadership state is recorded in the PD send queue! If there is no published comID with a certain
             redID, it would never be set... */
            if (FALSE == found)
            {
                vos_printLog(VOS_LOG_ERROR, "Redundant ID not found\n");
                ret = TRDP_PARAM_ERR;
            }

            if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
            {
                vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Get status of redundant ComIds.
 *  Only the status of the first redundancy group entry is returned will be returned!
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      redId               will be returned for all ComID's with the given redId
 *  @param[in,out]  pLeader             TRUE if we're sending this redundancy group (leader)
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error / redId not existing
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_getRedundant (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              redId,
    BOOL                *pLeader)
{
    TRDP_ERR_T  ret = TRDP_NOINIT_ERR;
    PD_ELE_T    *iterPD;

    if (pLeader == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
        if (ret == TRDP_NO_ERR)
        {
            /*    Search the redundancy flag for every PD with the specified ID */
            for (iterPD = appHandle->pSndQueue; NULL != iterPD; iterPD = iterPD->pNext)
            {
                if (iterPD->redId == redId)         /* packet redundant ID matches                      */
                {
                    if (iterPD->privFlags & TRDP_REDUNDANT)
                    {
                        *pLeader = FALSE;
                    }
                    else
                    {
                        *pLeader = TRUE;
                    }
                    break;
                }
            }

            if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
            {
                vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Set new topocount for trainwide communication
 *
 *    This value is used for validating outgoing and incoming packets only!
 *
 *  @param[in]      appHandle        the handle returned by tlc_openSession
 *  @param[in]      topoCount        New topoCount value
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlc_setTopoCount (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              topoCount)
{
    TRDP_ERR_T ret;

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
        if (ret == TRDP_NO_ERR)
        {
            /*  Set the topoCount for each session  */
            appHandle->topoCount = topoCount;

            if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
            {
                vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }
    else
    {
        ret = TRDP_NOINIT_ERR;
    }

    return ret;
}

/**********************************************************************************************************************/
/** Prepare for sending PD messages.
 *  Queue a PD message, it will be send when trdp_work has been called
 *
 *  @param[in]      appHandle            the handle returned by tlc_openSession
 *  @param[out]     pPubHandle           returned handle for related unprepare
 *  @param[in]      comId                comId of packet to send
 *  @param[in]      topoCount            valid topocount, 0 for local consist
 *  @param[in]      srcIpAddr            own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr           where to send the packet to
 *  @param[in]      interval             frequency of PD packet (>= 10ms) in usec, 0 if PD PULL
 *  @param[in]      redId                0 - Non-redundant, > 0 valid redundancy group
 *  @param[in]      pktFlags             OPTION:
 *                                       TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pSendParam           optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData                pointer to packet data / dataset
 *  @param[in]      dataSize             size of packet data <= 1436 without FCS
 *
 *  @retval         TRDP_NO_ERR          no error
 *  @retval         TRDP_PARAM_ERR       parameter error
 *  @retval         TRDP_MEM_ERR         could not insert (out of memory)
 *  @retval         TRDP_NOINIT_ERR      handle invalid
 *  @retval         TRDP_NOPUB_ERR       Already published
 */
EXT_DECL TRDP_ERR_T tlp_publish (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_PUB_T              *pPubHandle,
    UINT32                  comId,
    UINT32                  topoCount,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    UINT32                  interval,
    UINT32                  redId,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize)
{
    PD_ELE_T    *pNewElement = NULL;
    TRDP_TIME_T nextTime;
    TRDP_TIME_T tv_interval;
    TRDP_ERR_T  ret = TRDP_NO_ERR;

    /*    Check params    */
    if ((comId == 0)
        || (pData != NULL && dataSize == 0)
        || (dataSize > TRDP_MAX_PD_DATA_SIZE)
        || (interval != 0 && interval < TRDP_TIMER_GRANULARITY)
        || (pPubHandle == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
    if (ret == TRDP_NO_ERR)
    {
        TRDP_ADDRESSES_T pubHandle;

        /* initialize pubHandle */
        pubHandle.comId         = comId;
        pubHandle.destIpAddr    = destIpAddr;
        pubHandle.mcGroup       = vos_isMulticast(destIpAddr) ? destIpAddr : 0;
        pubHandle.srcIpAddr     = srcIpAddr;

        /*    Look for existing element    */
        if (trdp_queueFindPubAddr(appHandle->pSndQueue, &pubHandle) != NULL)
        {
            /*  Already published! */
            ret = TRDP_NOPUB_ERR;
        }
        else
        {
            pNewElement = (PD_ELE_T *) vos_memAlloc(sizeof(PD_ELE_T));
            if (pNewElement == NULL)
            {
                ret = TRDP_MEM_ERR;
            }
            else
            {
                /*
                 Compute the overal packet size
                 */
                if (dataSize == 0)
                {
                    /* mark data as invalid, data will be set valid with tlp_put */
                    pNewElement->privFlags |= TRDP_INVALID_DATA;
                    /* dataSize = TRDP_MAX_PD_DATA_SIZE; */
                }

                pNewElement->dataSize   = dataSize;
                pNewElement->grossSize  = trdp_packetSizePD(dataSize);

                /*    Get a socket    */
                ret = trdp_requestSocket(
                        appHandle->iface,
                        appHandle->pdDefault.port,
                        (pSendParam != NULL) ? pSendParam : &appHandle->pdDefault.sendParam,
                        (srcIpAddr == 0) ? appHandle->realIP : srcIpAddr,
                        pubHandle.mcGroup,
                        TRDP_SOCK_PD,
                        appHandle->option,
                        FALSE,
                        -1,
                        &pNewElement->socketIdx,
                        0);

                if (ret != TRDP_NO_ERR)
                {
                    vos_memFree(pNewElement);
                    pNewElement = NULL;
                }
                else
                {
                    /*  Alloc the corresponding data buffer  */
                    pNewElement->pFrame = (PD_PACKET_T *) vos_memAlloc(pNewElement->grossSize);
                    if (pNewElement->pFrame == NULL)
                    {
                        vos_memFree(pNewElement);
                        pNewElement = NULL;
                    }
                }
            }
        }

        /*    Get the current time and compute the next time this packet should be sent.    */
        if ((ret == TRDP_NO_ERR)
            && (pNewElement != NULL))
        {
            /* PD PULL?    Packet will be sent on request only    */
            if (0 == interval)
            {
                vos_clearTime(&pNewElement->interval);
                vos_clearTime(&pNewElement->timeToGo);
            }
            else
            {
                vos_getTime(&nextTime);
                tv_interval.tv_sec  = interval / 1000000;
                tv_interval.tv_usec = interval % 1000000;
                vos_addTime(&nextTime, &tv_interval);
                pNewElement->interval   = tv_interval;
                pNewElement->timeToGo   = nextTime;
            }

            /*    Update the internal data */
            pNewElement->addr           = pubHandle;
            pNewElement->pktFlags       = (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->pdDefault.flags : pktFlags;
            pNewElement->privFlags      = TRDP_PRIV_NONE;
            pNewElement->pullIpAddress  = 0;
            pNewElement->redId          = redId;
            pNewElement->pCachedDS      = NULL;
            pNewElement->magic          = TRDP_MAGIC_PUB_HNDL_VALUE;

            /*  Find a possible redundant entry in one of the other sessions and sync the sequence counter!
             curSeqCnt holds the last sent sequence counter, therefore set the value initially to -1,
             it will be incremented when sending...    */

            pNewElement->curSeqCnt = trdp_getSeqCnt(pNewElement->addr.comId, TRDP_MSG_PD,
                                                    pNewElement->addr.srcIpAddr) - 1;

            /*  Get a second sequence counter in case this packet is requested as PULL. This way we will not
             disturb the monotonic sequence for PDs  */
            pNewElement->curSeqCnt4Pull = trdp_getSeqCnt(pNewElement->addr.comId, TRDP_MSG_PP,
                                                         pNewElement->addr.srcIpAddr) - 1;

            /*    Check if the redundancy group is already set as follower; if set, we need to mark this one also!
             This will only happen, if publish() is called while we are in redundant mode */
            if (0 != redId)
            {
                BOOL isLeader = TRUE;

                ret = tlp_getRedundant(appHandle, redId, &isLeader);
                if (ret == TRDP_NO_ERR && FALSE == isLeader)
                {
                    pNewElement->privFlags |= TRDP_REDUNDANT;
                }
            }

            /*    Compute the header fields */
            trdp_pdInit(pNewElement, TRDP_MSG_PD, topoCount, 0, 0);

            /*    Insert at front    */
            trdp_queueInsFirst(&appHandle->pSndQueue, pNewElement);

            *pPubHandle = (TRDP_PUB_T) pNewElement;
            ret         = tlp_put(appHandle, *pPubHandle, pData, dataSize);

            if ((ret == TRDP_NO_ERR) && (appHandle->option & TRDP_OPTION_TRAFFIC_SHAPING))
            {
                ret = trdp_pdDistribute(appHandle->pSndQueue);
            }
        }

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Stop sending PD messages.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pubHandle           the handle returned by prepare
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_NOPUB_ERR      not published
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */

TRDP_ERR_T  tlp_unpublish (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle)
{
    PD_ELE_T    *pElement = (PD_ELE_T *)pubHandle;
    TRDP_ERR_T  ret;

    if (pElement == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_PUB_HNDL_VALUE)
    {
        return TRDP_NOPUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
    if (ret == TRDP_NO_ERR)
    {
        /*    Remove from queue?    */
        trdp_queueDelElement(&appHandle->pSndQueue, pElement);
        pElement->magic = 0;
        vos_memFree(pElement->pFrame);
        vos_memFree(pElement);
        ret = TRDP_NO_ERR;

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;      /*    Not found    */
}

/**********************************************************************************************************************/
/** Update the process data to send.
 *  Update previously published data. The new telegram will be sent earliest when tlc_process is called.
 *
 *  @param[in]      appHandle          the handle returned by tlc_openSession
 *  @param[in]      pubHandle          the handle returned by publish
 *  @param[in,out]  pData              pointer to application's data buffer
 *  @param[in,out]  dataSize           size of data
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_PARAM_ERR     parameter error on uninitialized parameter or changed dataSize compared to published one
 *  @retval         TRDP_NOPUB_ERR     not published
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 *  @retval         TRDP_COMID_ERR     ComID not found when marshalling
 */
TRDP_ERR_T tlp_put (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    const UINT8         *pData,
    UINT32              dataSize)
{
    PD_ELE_T    *pElement   = (PD_ELE_T *)pubHandle;
    TRDP_ERR_T  ret         = TRDP_NO_ERR;

    if (pElement == NULL ||
        dataSize > TRDP_MAX_PD_DATA_SIZE )
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_PUB_HNDL_VALUE)
    {
        return TRDP_NOPUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
    if ( ret == TRDP_NO_ERR)
    {
        /*    Find the published queue entry    */
        /* pElement = trdp_queueFindPubAddr(appHandle->pSndQueue, pubHandle); */
        if (pElement != NULL)
        {
            ret = trdp_pdPut(pElement,
                             appHandle->marshall.pfCbMarshall,
                             appHandle->marshall.pRefCon,
                             pData,
                             dataSize);
        }

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Get the lowest time interval for PDs.
 *  Return the maximum time interval suitable for 'select()' so that we
 *    can send due PD packets in time.
 *    If the PD send queue is empty, return zero time
 *
 *  @param[in]      appHandle          The handle returned by tlc_openSession
 *  @param[out]     pInterval          pointer to needed interval
 *  @param[in,out]  pFileDesc          pointer to file descriptor set
 *  @param[out]     pNoDesc            pointer to put no of highest used descriptors (for select())
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
EXT_DECL TRDP_ERR_T tlc_getInterval (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_TIME_T         *pInterval,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc)
{
    TRDP_TIME_T now;
    TRDP_ERR_T  ret = TRDP_NOINIT_ERR;

    if (trdp_isValidSession(appHandle))
    {
        if ((pInterval == NULL)
            || (pFileDesc == NULL)
            || (pNoDesc == NULL))
        {
            ret = TRDP_PARAM_ERR;
        }
        else
        {
            ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);

            if (ret == TRDP_NO_ERR)
            {
                /*    Get the current time    */
                vos_getTime(&now);
                vos_clearTime(&appHandle->nextJob);

                trdp_pdCheckPending(appHandle, pFileDesc, pNoDesc);

#if MD_SUPPORT
                trdp_mdCheckPending(appHandle, pFileDesc, pNoDesc);
#endif

                /*    if next job time is known, return the time-out value to the caller   */
                if (timerisset(&appHandle->nextJob) &&
                    timercmp(&now, &appHandle->nextJob, <))
                {
                    vos_subTime(&appHandle->nextJob, &now);
                    *pInterval = appHandle->nextJob;

                }
                else    /* if unknown, set minimum poll time to 10ms   */
                {
                    pInterval->tv_sec   = 0;
                    pInterval->tv_usec  = TRDP_PROCESS_DEFAULT_CYCLE_TIME;      /* Minimum poll time 10ms    */
                }

                if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
                {
                    vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
                }
            }
        }
    }
    return ret;
}

/**********************************************************************************************************************/
/** Work loop of the TRDP handler.
 *    Search the queue for pending PDs to be sent
 *    Search the receive queue for pending PDs (time out)
 *
 *
 *  @param[in]      appHandle          The handle returned by tlc_openSession
 *  @param[in]      pRfds              pointer to set of ready descriptors
 *  @param[in,out]  pCount             pointer to number of ready descriptors
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
EXT_DECL TRDP_ERR_T tlc_process (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pRfds,
    INT32               *pCount)
{
    TRDP_ERR_T  result = TRDP_NO_ERR;
    TRDP_ERR_T  err;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }
    else
    {
        vos_clearTime(&appHandle->nextJob);

        /******************************************************
         Find and send the packets which have to be sent next:
         ******************************************************/

        err = trdp_pdSendQueued(appHandle);

        if (err != TRDP_NO_ERR)
        {
            /*  We do not break here, only report error */
            result = err;
            vos_printLog(VOS_LOG_ERROR, "trdp_pdSendQueued failed (Err: %d)\n", err);
        }

        /******************************************************
         Find packets which are pending/overdue
         ******************************************************/
        trdp_pdHandleTimeOuts(appHandle);

#if MD_SUPPORT

        err = trdp_mdSend(appHandle);
        if (err != TRDP_NO_ERR)
        {
            if (err == TRDP_IO_ERR)
            {
                vos_printLog(VOS_LOG_INFO, "trdp_mdSend() incomplete \n");

            }
            else
            {
                result = err;
                vos_printLog(VOS_LOG_ERROR, "trdp_mdSend() failed (Err: %d)\n", err);
            }
        }

#endif

        /******************************************************
         Find packets which are to be received
         ******************************************************/
        err = trdp_pdCheckListenSocks(appHandle, pRfds, pCount);
        if (err != TRDP_NO_ERR)
        {
            /*  We do not break here */
            result = err;
        }

#if MD_SUPPORT

        trdp_mdCheckListenSocks(appHandle, pRfds, pCount);

        trdp_mdCheckTimeouts(appHandle);

#endif

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return result;
}

/**********************************************************************************************************************/
/** Initiate sending PD messages (PULL).
 *  Send a PD request message
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           handle from related subscribe
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      topoCount           valid topocount, 0 for local consist
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      redId               0 - Non-redundant, > 0 valid redundancy group
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      replyComId          comId of reply
 *  @param[in]      replyIpAddr         IP for reply
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not insert (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_NOSUB_ERR      no matching subscription found
 */
EXT_DECL TRDP_ERR_T tlp_request (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_SUB_T              subHandle,
    UINT32                  comId,
    UINT32                  topoCount,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    UINT32                  redId,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    UINT32                  replyComId,
    TRDP_IP_ADDR_T          replyIpAddr)
{
    TRDP_ERR_T  ret             = TRDP_NO_ERR;
    PD_ELE_T    *pSubPD         = (PD_ELE_T *) subHandle;
    PD_ELE_T    *pReqElement    = NULL;

    /*    Check params    */
    if ((appHandle == 0)
        || (subHandle == 0)
        || (comId == 0)
        || (dataSize > TRDP_MAX_PD_PACKET_SIZE)
        || (destIpAddr == 0))
    {
        return TRDP_PARAM_ERR;
    }

    if (pSubPD->magic != TRDP_MAGIC_SUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);

    if ( ret == TRDP_NO_ERR)
    {
        {
            /*    Look for former request element    */
            pReqElement = trdp_queueFindComId(appHandle->pSndQueue, comId);

            if ( pReqElement == NULL)
            {
                /*  This is the first time, get a new element   */
                pReqElement = (PD_ELE_T *) vos_memAlloc(sizeof(PD_ELE_T));

                if (pReqElement == NULL)
                {
                    ret = TRDP_MEM_ERR;
                }
                else
                {
                    /*
                     Compute the overal packet size
                     Add padding bytes to align to 32 bits
                     */
                    pReqElement->dataSize   = dataSize;
                    pReqElement->grossSize  = trdp_packetSizePD(dataSize);
                    pReqElement->pFrame     = (PD_PACKET_T *) vos_memAlloc(pReqElement->grossSize);

                    if (pReqElement->pFrame == NULL)
                    {
                        vos_memFree(pReqElement);
                        pReqElement = NULL;
                    }
                    else
                    {
                        /*    Get a socket    */
                        ret = trdp_requestSocket(appHandle->iface,
                                                 appHandle->pdDefault.port,
                                                 (pSendParam != NULL) ? pSendParam : &appHandle->pdDefault.sendParam,
                                                 srcIpAddr,
                                                 0,
                                                 TRDP_SOCK_PD,
                                                 appHandle->option,
                                                 FALSE,
                                                 -1,
                                                 &pReqElement->socketIdx,
                                                 0);

                        if (ret != TRDP_NO_ERR)
                        {
                            vos_memFree(pReqElement->pFrame);
                            vos_memFree(pReqElement);
                            pReqElement = NULL;
                        }
                        else
                        {
                            /*  Mark this element as a PD PULL.  Request will be sent on tlc_process time.    */
                            vos_clearTime(&pReqElement->interval);
                            vos_clearTime(&pReqElement->timeToGo);

                            /*  Update the internal data */
                            pReqElement->addr.comId         = comId;
                            pReqElement->addr.destIpAddr    = destIpAddr;
                            pReqElement->addr.srcIpAddr     = srcIpAddr;
                            pReqElement->addr.mcGroup       = 0;
                            pReqElement->pktFlags           =
                                (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->pdDefault.flags : pktFlags;
                            pReqElement->magic = TRDP_MAGIC_PUB_HNDL_VALUE;
                            /*    Enter this request into the send queue.    */
                            trdp_queueInsFirst(&appHandle->pSndQueue, pReqElement);
                        }
                    }
                }
            }

            if (ret == TRDP_NO_ERR && pReqElement != NULL)
            {
                /*  Find a possible redundant entry in one of the other sessions and sync the sequence counter!
                 curSeqCnt holds the last sent sequence counter, therefore set the value initially to -1,
                 it will be incremented when sending...                                                          */
                pReqElement->curSeqCnt = trdp_getSeqCnt(pReqElement->addr.comId,
                                                        TRDP_MSG_PR, pReqElement->addr.srcIpAddr) - 1;

                /*    Compute the header fields */
                trdp_pdInit(pReqElement, TRDP_MSG_PR, topoCount, replyComId, replyIpAddr);

                ret = tlp_put(appHandle, (TRDP_PUB_T) pReqElement, pData, dataSize);

                /*  This flag triggers sending in tlc_process (one shot)  */
                pReqElement->privFlags |= TRDP_REQ_2B_SENT;

                /*    Set the current time and start time out of subscribed packet  */
                if (timerisset(&pSubPD->interval))
                {
                    vos_getTime(&pSubPD->timeToGo);
                    vos_addTime(&pSubPD->timeToGo, &pSubPD->interval);
                }
            }
        }

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Prepare for receiving PD messages.
 *  Subscribe to a specific PD ComID and source IP.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pSubHandle          return a handle for these messages
 *  @param[in]      pUserRef            user supplied value returned within the info structure
 *  @param[in]      comId               comId of packet to receive
 *  @param[in]      topoCount           valid topocount, 0 for local consist
 *  @param[in]      srcIpAddr1          IP for source filtering, set 0 if not used
 *  @param[in]      srcIpAddr2          Second source IP address for source filtering, set to zero if not used.
 *                                      Used e.g. for source filtering of redundant devices.
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      destIpAddr          IP address to join
 *  @param[in]      timeout             timeout (>= 10ms) in usec
 *  @param[in]      toBehavior          timeout behavior
 *  @param[in]      maxDataSize         expected max. size of packet data
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not reserve memory (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_subscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          *pSubHandle,
    const void          *pUserRef,
    UINT32              comId,
    UINT32              topoCount,
    TRDP_IP_ADDR_T      srcIpAddr1,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      destIpAddr,
    TRDP_FLAGS_T        pktFlags,
    UINT32              timeout,
    TRDP_TO_BEHAVIOR_T  toBehavior,
    UINT32              maxDataSize)
{
    PD_ELE_T            *newPD = NULL;
    TRDP_TIME_T         now;
    TRDP_ERR_T          ret = TRDP_NO_ERR;
    TRDP_ADDRESSES_T    subHandle;
    INT32 lIndex;

    /*    Check params    */
    if (comId == 0 || pSubHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Check params    */
    if (comId == 0 || maxDataSize > TRDP_MAX_PD_DATA_SIZE)
    {
        return TRDP_PARAM_ERR;
    }

    if (timeout == 0)
    {
        timeout = appHandle->pdDefault.timeout;
    }
    else if (timeout < TRDP_TIMER_GRANULARITY)
    {
        timeout = TRDP_TIMER_GRANULARITY;
    }

    /*    Reserve mutual access    */
    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /*  Create an addressing item   */
    subHandle.comId         = comId;
    subHandle.srcIpAddr     = srcIpAddr1;
    subHandle.destIpAddr    = destIpAddr;

    if (vos_isMulticast(destIpAddr))
    {
        subHandle.mcGroup = destIpAddr;
    }
    else
    {
        subHandle.mcGroup = 0;
    }

    /*    Get the current time    */
    vos_getTime(&now);

    /*    Look for existing element    */
    if (trdp_queueFindSubAddr(appHandle->pRcvQueue, &subHandle) != NULL)
    {
        ret = TRDP_NOSUB_ERR;
    }
    else
    {
        /*    Find a (new) socket    */
        ret = trdp_requestSocket(appHandle->iface,
                                 appHandle->pdDefault.port,
                                 &appHandle->pdDefault.sendParam,
                                 appHandle->realIP,
                                 subHandle.mcGroup,
                                 TRDP_SOCK_PD,
                                 appHandle->option,
                                 TRUE,
                                 -1,
                                 &lIndex,
                                 0);

        if (ret == TRDP_NO_ERR)
        {
            /*    buffer size is PD_ELEMENT plus max. payload size plus padding & framecheck    */

            /*    Allocate a buffer for this kind of packets    */
            newPD = (PD_ELE_T *) vos_memAlloc(sizeof(PD_ELE_T));

            if (newPD == NULL)
            {
                ret = TRDP_MEM_ERR;
            }
            else
            {
                /*  Alloc the corresponding data buffer  */
                newPD->pFrame = (PD_PACKET_T *) vos_memAlloc(TRDP_MAX_PD_PACKET_SIZE);
                if (newPD->pFrame == NULL)
                {
                    vos_memFree(newPD);
                    newPD   = NULL;
                    ret     = TRDP_MEM_ERR;
                }
                else
                {
                    /*    Initialize some fields    */
                    if (vos_isMulticast(destIpAddr))
                    {
                        newPD->addr.mcGroup = destIpAddr;
                        newPD->privFlags    |= TRDP_MC_JOINT;
                    }
                    else
                    {
                        newPD->addr.mcGroup = 0;
                    }

                    newPD->addr.comId       = comId;
                    newPD->addr.srcIpAddr   = srcIpAddr1;
                    newPD->addr.destIpAddr  = destIpAddr;
                    newPD->interval.tv_sec  = timeout / 1000000;
                    newPD->interval.tv_usec = timeout % 1000000;
                    newPD->timeToGo         = newPD->interval;
                    newPD->toBehavior       =
                        (toBehavior == TRDP_TO_DEFAULT) ? appHandle->pdDefault.toBehavior : toBehavior;
                    newPD->grossSize    = TRDP_MAX_PD_PACKET_SIZE;
                    newPD->userRef      = pUserRef;
                    newPD->socketIdx    = lIndex;
                    newPD->privFlags    |= TRDP_INVALID_DATA;
                    newPD->pktFlags     = (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->pdDefault.flags : pktFlags;
                    newPD->pCachedDS    = NULL;
                    newPD->magic        = TRDP_MAGIC_SUB_HNDL_VALUE;

                    if (timeout == TRDP_TIMER_FOREVER)
                    {
                        vos_clearTime(&newPD->timeToGo);
                    }
                    else
                    {
                        vos_getTime(&newPD->timeToGo);
                        vos_addTime(&newPD->timeToGo, &newPD->interval);
                    }

                    /*  append this subscription to our receive queue */
                    trdp_queueAppLast(&appHandle->pRcvQueue, newPD);

                    *pSubHandle = (TRDP_SUB_T) newPD;
                }
            }
        }
    }

    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return ret;
}

/**********************************************************************************************************************/
/** Stop receiving PD messages.
 *  Unsubscribe to a specific PD ComID
 *
 *  @param[in]      appHandle            the handle returned by tlc_openSession
 *  @param[in]      subHandle            the handle returned by subscription
 *
 *  @retval         TRDP_NO_ERR          no error
 *  @retval         TRDP_PARAM_ERR       parameter error
 *  @retval         TRDP_NOSUB_ERR       not subscribed
 *  @retval         TRDP_NOINIT_ERR      handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_unsubscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle)
{
    PD_ELE_T    *pElement = (PD_ELE_T *) subHandle;
    TRDP_ERR_T  ret;

    if (pElement == NULL )
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_SUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
    if (ret == TRDP_NO_ERR)
    {
        /*    Remove from queue?    */
        trdp_queueDelElement(&appHandle->pRcvQueue, pElement);
        pElement->magic = 0;
        vos_memFree(pElement->pFrame);
        vos_memFree(pElement);
        ret = TRDP_NO_ERR;
        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;      /*    Not found    */
}

/**********************************************************************************************************************/
/** Get the last valid PD message.
 *  This allows polling of PDs instead of event driven handling by callbacks
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           the handle returned by subscription
 *  @param[in,out]  pPdInfo             pointer to application's info buffer
 *  @param[in,out]  pData               pointer to application's data buffer
 *  @param[in,out]  pDataSize           in: size of buffer, out: size of data
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_SUB_ERR        not subscribed
 *  @retval         TRDP_TIMEOUT_ERR    packet timed out
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_COMID_ERR      ComID not found when marshalling
 */
EXT_DECL TRDP_ERR_T tlp_get (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle,
    TRDP_PD_INFO_T      *pPdInfo,
    UINT8               *pData,
    UINT32              *pDataSize)
{
    PD_ELE_T    *pElement   = (PD_ELE_T *) subHandle;
    TRDP_ERR_T  ret         = TRDP_NOSUB_ERR;
    TRDP_TIME_T now;

    if (pElement == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_SUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
    if (ret == TRDP_NO_ERR)
    {
        /*    Call the receive function if we are in non blocking mode    */
        if (!(appHandle->option & TRDP_OPTION_BLOCK))
        {
            /* read all you can get, return value is not interesting */
            do
            {}
            while (trdp_pdReceive(appHandle, appHandle->iface[pElement->socketIdx].sock) == TRDP_NO_ERR);
        }

        /*    Get the current time    */
        vos_getTime(&now);

        /*    Check time out    */
        if (timerisset(&pElement->interval) &&
            timercmp(&pElement->timeToGo, &now, <))
        {
            /*    Packet is late    */
            if (pElement->toBehavior == TRDP_TO_SET_TO_ZERO)
            {
                memset(pData, 0, *pDataSize);
            }
            else /* TRDP_TO_KEEP_LAST_VALUE */
            {
                ;
            }
            ret = TRDP_TIMEOUT_ERR;
        }
        else
        {
            ret = trdp_pdGet(pElement,
                             appHandle->marshall.pfCbUnmarshall,
                             appHandle->marshall.pRefCon,
                             pData,
                             pDataSize);
        }

        if (pPdInfo != NULL && pElement != NULL)
        {
            pPdInfo->comId          = pElement->addr.comId;
            pPdInfo->srcIpAddr      = pElement->addr.srcIpAddr;
            pPdInfo->destIpAddr     = pElement->addr.destIpAddr;
            pPdInfo->topoCount      = vos_ntohl(pElement->pFrame->frameHead.topoCount);
            pPdInfo->msgType        = (TRDP_MSG_T) vos_ntohs(pElement->pFrame->frameHead.msgType);
            pPdInfo->seqCount       = pElement->curSeqCnt;
            pPdInfo->protVersion    = vos_ntohs(pElement->pFrame->frameHead.protocolVersion);
            pPdInfo->replyComId     = vos_ntohl(pElement->pFrame->frameHead.replyComId);
            pPdInfo->replyIpAddr    = vos_ntohl(pElement->pFrame->frameHead.replyIpAddress);
            pPdInfo->pUserRef       = pElement->userRef;        /* TBD: User reference given with the local subscribe?
                                                                 */
            pPdInfo->resultCode     = TRDP_NO_ERR;
        }

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

#if MD_SUPPORT
/**********************************************************************************************************************/
/** Initiate sending MD notification message.
 *  Send a MD notification message
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      pUserRef            user supplied value returned with reply
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      topoCount           topocount to use
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      sourceURI           only functional group of source URI
 *  @param[in]      destURI             only functional group of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_notify (
    TRDP_APP_SESSION_T      appHandle,
    const void              *pUserRef,
    UINT32                  comId,
    UINT32                  topoCount,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    return trdp_mdCommonSend(
               TRDP_MSG_MN,                            /* notify without reply */
               appHandle,
               pUserRef,
               NULL,                                   /* no return session id?
                                                          useful to abort send while waiting of output queue */
               comId,
               topoCount,
               srcIpAddr,
               destIpAddr,
               pktFlags,
               0,                                      /* no user status */
               0,                                      /* confirm timeout */
               0,                                      /* numbber of repliers for notify */
               0,                                      /* reply timeout for notify */
               TRDP_REPLY_OK,                          /* reply state */
               pSendParam,
               pData,
               dataSize,
               sourceURI,
               destURI
               );
}

/**********************************************************************************************************************/
/** Initiate sending MD request message.
 *  Send a MD request message
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      pUserRef            user supplied value returned with reply
 *  @param[out]     pSessionId          return session ID
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      topoCount           topocount to use
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL
 *  @param[in]      numReplies          number of expected replies, 0 if unknown
 *  @param[in]      replyTimeout        timeout for reply
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      sourceURI           only functional group of source URI
 *  @param[in]      destURI             only functional group of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_request (
    TRDP_APP_SESSION_T      appHandle,
    const void              *pUserRef,
    TRDP_UUID_T             *pSessionId,
    UINT32                  comId,
    UINT32                  topoCount,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    UINT32                  numReplies,
    UINT32                  replyTimeout,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    return trdp_mdCommonSend(
               TRDP_MSG_MR,                            /* request with reply */
               appHandle,
               pUserRef,
               pSessionId,
               comId,
               topoCount,
               srcIpAddr,
               destIpAddr,
               pktFlags,
               0,                                      /* no user status */
               0,                                      /* confirm timeout */
               numReplies,
               replyTimeout,
               TRDP_REPLY_OK,                          /* reply state */
               pSendParam,
               pData,
               dataSize,
               sourceURI,
               destURI
               );
}


/**********************************************************************************************************************/
/** Subscribe to MD messages.
 *  Add a listener to TRDP to get notified when messages are received
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[out]     pListenHandle       Listener ID returned
 *  @param[in]      pUserRef            user supplied value returned with reply
 *  @param[in]      comId               comId to be observed
 *  @param[in]      topoCount           topocount to use
 *  @param[in]      mcDestIpAddr        multicast group to listen on
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_MARSHALL
 *  @param[in]      destURI             only functional group of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_addListener (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_LIS_T              *pListenHandle,
    const void              *pUserRef,
    UINT32                  comId,
    UINT32                  topoCount,
    TRDP_IP_ADDR_T          mcDestIpAddr,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_URI_USER_T   destURI)
{
    TRDP_ERR_T      errv            = TRDP_NO_ERR;
    MD_LIS_ELE_T    *pNewElement    = NULL;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /* lock mutex */

    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /* mutex protected */
    {
        /* Make sure that there is a TCP listener socket */
        if ((pktFlags & TRDP_FLAGS_TCP) != 0)
        {
            errv = trdp_getTCPSocket(appHandle);
        }

        /* no error... */
        if (errv == TRDP_NO_ERR)
        {
            /* Room for MD element */
            pNewElement = (MD_LIS_ELE_T *) vos_memAlloc(sizeof(MD_LIS_ELE_T));
            if (NULL == pNewElement)
            {
                errv = TRDP_MEM_ERR;
            }
            else
            {
                pNewElement->pNext = NULL;

                /* caller parameters saved into instance */
                pNewElement->pUserRef           = pUserRef;
                pNewElement->addr.comId         = comId;
                pNewElement->addr.topoCount     = topoCount;
                pNewElement->addr.destIpAddr    = 0;
                pNewElement->pktFlags           = pktFlags;

                /* 2013-02-06 BL: Check for zero pointer  */
                if (NULL != destURI)
                {
                    vos_strncpy(pNewElement->destURI, destURI, TRDP_MAX_URI_USER_LEN);
                }

                if (vos_isMulticast(mcDestIpAddr))
                {
                    pNewElement->addr.mcGroup   = mcDestIpAddr;     /* Set multicast group address */
                    pNewElement->privFlags      |= TRDP_MC_JOINT;   /* Set multicast flag */
                }
                else
                {
                    pNewElement->addr.mcGroup = 0;
                }

                if ((pNewElement->pktFlags & TRDP_FLAGS_TCP) == 0)
                {
                    /* socket to receive UDP MD */
                    errv = trdp_requestSocket(
                            appHandle->iface,
                            appHandle->mdDefault.udpPort,
                            &appHandle->mdDefault.sendParam,
                            appHandle->realIP,
                            pNewElement->addr.mcGroup,
                            TRDP_SOCK_MD_UDP,
                            appHandle->option,
                            TRUE,
                            -1,
                            &pNewElement->socketIdx,
                            0);
                }
                else
                {
                    pNewElement->socketIdx = -1;
                }

                if (TRDP_NO_ERR != errv)
                {
                    /* Error getting socket */
                }
                else
                {
                    /* Insert into list */
                    pNewElement->pNext          = appHandle->pMDListenQueue;
                    appHandle->pMDListenQueue   = pNewElement;

                    /* Statistics */
                    if ((pNewElement->pktFlags & TRDP_FLAGS_TCP) != 0)
                    {
                        appHandle->stats.tcpMd.numList++;
                    }
                    else
                    {
                        appHandle->stats.udpMd.numList++;
                    }
                }
            }
        }
    }

    /* Error and allocated element ! */
    if (TRDP_NO_ERR != errv && NULL != pNewElement)
    {
        vos_memFree(pNewElement);
        pNewElement = NULL;
    }

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    /* return listener reference to caller */
    if (pListenHandle != NULL)
    {
        *pListenHandle = (TRDP_LIS_T)pNewElement;
    }

    return errv;
}


/**********************************************************************************************************************/
/** Remove Listener.
 *
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[out]     listenHandle        Listener ID returned
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_delListener (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_LIS_T          listenHandle)
{
    TRDP_ERR_T      errv        = TRDP_NO_ERR;
    MD_LIS_ELE_T    *iterLis    = NULL;
    MD_LIS_ELE_T    *pDelete    = (MD_LIS_ELE_T *) listenHandle;
    BOOL            dequeued    = FALSE;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /* lock mutex */

    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /* mutex protected */
    if (NULL != pDelete)
    {
        /* handle removal of first element */
        if (pDelete == appHandle->pMDListenQueue)
        {
            appHandle->pMDListenQueue = pDelete->pNext;
            dequeued = TRUE;
        }
        else
        {
            for (iterLis = appHandle->pMDListenQueue; iterLis != NULL; iterLis = iterLis->pNext)
            {
                if (iterLis->pNext == pDelete)
                {
                    iterLis->pNext  = pDelete->pNext;
                    dequeued        = TRUE;
                    break;
                }
            }
        }

        if (TRUE == dequeued)
        {
            /* cleanup instance */
            if (pDelete->socketIdx != -1)
            {
                trdp_releaseSocket(appHandle->iface, pDelete->socketIdx, appHandle->mdDefault.connectTimeout, FALSE);
            }
            /* free memory space for element */
            vos_memFree(pDelete);
        }
    }

    /* Statistics */
    if (errv == TRDP_NO_ERR)
    {
        if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0)
        {
            appHandle->stats.tcpMd.numList--;
        }
        else
        {
            appHandle->stats.udpMd.numList--;
        }
    }

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return errv;
}


/**********************************************************************************************************************/
/** Send a MD reply message.
 *  Send a MD reply message after receiving an request
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      pUserRef            user supplied value returned with reply
 *  @param[in]      pSessionId          Session ID returned by indication
 *  @param[in]      topoCount           topocount to use
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      pktFlags            optional marshalling
 *  @param[in]      userStatus          Info for requester about application errors
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      sourceURI           only user part of source URI
 *  @param[in]      destURI             only user part of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        Out of memory
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_reply (
    TRDP_APP_SESSION_T      appHandle,
    void                    *pUserRef,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  topoCount,
    UINT32                  comId,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    UINT16                  userStatus,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    return trdp_mdCommonSend(
               TRDP_MSG_MP,                            /* reply without confirm */
               appHandle,
               pUserRef,
               (TRDP_UUID_T *)pSessionId,
               comId,
               topoCount,
               srcIpAddr,
               destIpAddr,
               pktFlags,
               userStatus,
               0,                                      /* confirm timeout */
               0,                                      /* noOfRepliers */
               0,                                      /* replyTimeout */
               TRDP_REPLY_OK,                          /* reply state */
               pSendParam,
               pData,
               dataSize,
               sourceURI,
               destURI
               );
}

/**********************************************************************************************************************/
/** Send a MD reply message.
 *  Send a MD reply message after receiving a request and ask for confirmation.
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      pUserRef            user supplied value returned with reply
 *  @param[in]      pSessionId          Session ID returned by indication
 *  @param[in]      topoCount           topocount to use
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      pktFlags            optional marshalling
 *  @param[in]      userStatus          Info for requester about application errors
 *  @param[in]      confirmTimeout      timeout for confirmation
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      sourceURI           only user part of source URI
 *  @param[in]      destURI             only user part of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_replyQuery (
    TRDP_APP_SESSION_T      appHandle,
    void                    *pUserRef,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  topoCount,
    UINT32                  comId,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    UINT16                  userStatus,
    UINT32                  confirmTimeout,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    return trdp_mdCommonSend(
               TRDP_MSG_MQ,                            /* reply with confirm request */
               appHandle,
               pUserRef,
               (TRDP_UUID_T *)pSessionId,
               comId,
               topoCount,
               srcIpAddr,
               destIpAddr,
               pktFlags,
               userStatus,
               confirmTimeout,
               0,                                      /* number of repliers */
               0,                                      /* reply timeout */
               TRDP_REPLY_OK,                          /* reply state */
               pSendParam,
               pData,
               dataSize,
               sourceURI,
               destURI
               );
}

/**********************************************************************************************************************/
/** Send a MD reply message.
 *  Send a MD error reply message after receiving an request
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      pSessionId          Session ID returned by indication
 *  @param[in]      topoCount           topocount to use
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      replyState          Info for requester about stack errors
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      sourceURI           only user part of source URI
 *  @param[in]      destURI             only user part of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_replyErr (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  topoCount,
    UINT32                  comId,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_REPLY_STATUS_T     replyState,
    const TRDP_SEND_PARAM_T *pSendParam,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    return trdp_mdCommonSend(
               TRDP_MSG_ME,                             /* reply with error */
               appHandle,
               NULL,                                    /* user ref */
               (TRDP_UUID_T *)pSessionId,
               comId,
               topoCount,
               srcIpAddr,
               destIpAddr,
               TRDP_FLAGS_NONE,                         /* pktflags */
               0,                                       /* user status */
               0,                                       /* confirm timeout */
               0,                                       /* num of repliers */
               0,                                       /* reply timeout */
               replyState,
               pSendParam,
               NULL,                                    /* pData */
               0,                                       /* dataSize */
               sourceURI,
               destURI
               );
}


/**********************************************************************************************************************/
/** Initiate sending MD confirm message.
 *  Send a MD confirmation message
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      pUserRef            user supplied value returned with reply
 *  @param[in]      pSessionId          Session ID returned by request
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      topoCount           topocount to use
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      pktFlags            OPTION: TRDP_FLAGS_CALLBACK
 *  @param[in]      userStatus          Info for requester about application errors
 *  @param[in]      replyStatus         Info for requester about stack errors
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      sourceURI           only functional group of source URI
 *  @param[in]      destURI             only functional group of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOSESSION_ERR  no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_confirm (
    TRDP_APP_SESSION_T      appHandle,
    const void              *pUserRef,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  comId,
    UINT32                  topoCount,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    UINT16                  userStatus,
    TRDP_REPLY_STATUS_T     replyStatus,
    const TRDP_SEND_PARAM_T *pSendParam,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    return trdp_mdCommonSend(
               TRDP_MSG_MC,                            /* reply confirm */
               appHandle,
               pUserRef,
               (TRDP_UUID_T *)pSessionId,
               comId,
               topoCount,
               srcIpAddr,
               destIpAddr,
               pktFlags,
               userStatus,
               0,                                      /* confirm timeout */
               0,                                      /* num of repliers */
               0,                                      /* reply timeout */
               replyStatus,
               pSendParam,
               NULL,                                   /* pData */
               0,                                      /* dataSize */
               sourceURI,
               destURI
               );
}

/**********************************************************************************************************************/
/** Cancel an open session.
 *  Abort an open session; any pending messages will be dropped
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      pSessionId          Session ID returned by request
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOSESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_abortSession (
    TRDP_APP_SESSION_T  appHandle,
    const TRDP_UUID_T   *pSessionId)
{
    MD_ELE_T    *iterMD     = appHandle->pMDSndQueue;
    BOOL        firstLoop   = TRUE;
    TRDP_ERR_T  err         = TRDP_NOSESSION_ERR;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pSessionId == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* lock mutex */

    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /*  Find the session which needs to be killed. Actual release will be done in tlc_process().
        Note: We must also check the receive queue for pending replies! */
    do
    {
        /*  Switch to receive queue */
        if (NULL == iterMD && TRUE == firstLoop)
        {
            iterMD      = appHandle->pMDRcvQueue;
            firstLoop   = FALSE;
        }

        if (NULL != iterMD)
        {
            if (memcmp(iterMD->sessionID, pSessionId, TRDP_SESS_ID_SIZE) == 0)
            {
                iterMD->morituri = TRUE;
                err = TRDP_NO_ERR;
            }
            iterMD = iterMD->pNext;
        }
    }
    while (iterMD != NULL);

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return err;
}

#endif

#ifdef __cplusplus
}
#endif
