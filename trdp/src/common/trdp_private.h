/**********************************************************************************************************************/
/**
 * @file            trdp_private.h
 *
 * @brief           Typedefs for TRDP communication
 *
 * @details         TRDP internal type definitions
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

#ifndef TRDP_PRIVATE_H
#define TRDP_PRIVATE_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"
#include "trdp_proto.h"
#include "vos_thread.h"
#include "vos_sock.h"


/***********************************************************************************************************************
 * DEFINES
 */

#define TRDP_VERSION            1
#define TRDP_RELEASE            0
#define TRDP_UPDATE             1
#define TRDP_EVOLUTION          0

#define TRDP_TIMER_GRANULARITY  10000                                   /**< granularity in us                      */
#define TRDP_TIMER_FOREVER      0xffffffff                              /**< granularity in us                      */

/*  Default MD communication parameters   */
#define TRDP_MD_DEFAULT_REPLY_TIMEOUT       5000000                      /**< default reply time out 5s             */
#define TRDP_MD_DEFAULT_CONFIRM_TIMEOUT     1000000                      /**< default confirm time out 1s           */
#define TRDP_MD_DEFAULT_CONNECTION_TIMEOUT  60000000                     /**< Socket connection time out 1 minute   */
#define TRDP_MD_DEFAULT_SENDING_TIMEOUT     5000000                      /**< Socket sending time out 5s            */
#define TRDP_MD_DEFAULT_QOS                 3
#define TRDP_MD_DEFAULT_TTL                 64
#define TRDP_MD_DEFAULT_SEND_PARAM          {TRDP_MD_DEFAULT_QOS, TRDP_MD_DEFAULT_TTL}
#define TRDP_MD_MAX_NUM_SESSIONS            1000

/*  Default PD communication parameters   */
#define TRDP_PD_DEFAULT_QOS         5
#define TRDP_PD_DEFAULT_TTL         64
#define TRDP_PD_DEFAULT_TIMEOUT     100000
#define TRDP_PD_DEFAULT_SEND_PARAM  {TRDP_PD_DEFAULT_QOS, TRDP_PD_DEFAULT_TTL, 0}

/*  Default TRDP process options    */
#define TRDP_PROCESS_DEFAULT_CYCLE_TIME     10000                       /**< Default cycle time for TRDP process    */
#define TRDP_PROCESS_DEFAULT_PRIORITY       64                          /**< Default priority of TRDP process       */
#define TRDP_PROCESS_DEFAULT_OPTIONS        TRDP_OPTION_TRAFFIC_SHAPING /**< Default options for TRDP process       */

#define TRDP_DEBUG_DEFAULT_FILE_SIZE        65536                       /**< Default maximum size of log file       */

#define TRDP_MAGIC_PUB_HNDL_VALUE           0xCAFEBABE
#define TRDP_MAGIC_SUB_HNDL_VALUE           0xBABECAFE

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Internal MD state */
typedef enum
{
    TRDP_ST_NONE = 0,                  /**< neutral value                                        */

    TRDP_ST_TX_NOTIFY_ARM       = 1,      /**< ready to send notify MD                              */
    TRDP_ST_TX_REQUEST_ARM      = 2,      /**< ready to send request MD                             */
    TRDP_ST_TX_REPLY_ARM        = 3,      /**< ready to send reply MD                               */
    TRDP_ST_TX_REPLYQUERY_ARM   = 4,      /**< ready to send reply with confirm request MD          */
    TRDP_ST_TX_CONFIRM_ARM      = 5,      /**< ready to send confirm MD                             */
    TRDP_ST_RX_READY = 6,                 /**< armed listener                                       */

    TRDP_ST_TX_REQUEST_W4REPLY  = 7,      /**< request sent, wait for reply                         */
    TRDP_ST_RX_REPLYQUERY_W4C   = 8,      /**< reply send, with confirm request MD                  */

    TRDP_ST_RX_REQ_W4AP_REPLY   = 9,      /**< request received, wait for application reply send    */
    TRDP_ST_TX_REQ_W4AP_CONFIRM = 10,     /**< reply conf. rq. tx, wait for application conf send   */
    TRDP_ST_RX_REPLY_SENT       = 11,     /**< reply sent    */

    TRDP_ST_RX_NOTIFY_RECEIVED  = 12,     /**< notification received, wait for application to accept    */
    TRDP_ST_TX_REPLY_RECEIVED   = 13,     /**< reply received    */
    TRDP_ST_RX_CONF_RECEIVED    = 14      /**< confirmation received  */
} TRDP_MD_ELE_ST_T;

/** Internal flags for packets    */
typedef enum
{
    TRDP_PRIV_NONE      = 0,
    TRDP_MC_JOINT       = 0x1,
    TRDP_TIMED_OUT      = 0x2,              /**< if set, inform the user                                */
    TRDP_INVALID_DATA   = 0x4,              /**< if set, inform the user                                */
    TRDP_REQ_2B_SENT    = 0x8,              /**< if set, the request needs to be sent                   */
    TRDP_PULL_SUB       = 0x10,             /**< if set, its a PULL subscription                        */
    TRDP_REDUNDANT      = 0x20              /**< if set, packet should not be sent (redundant           */
} TRDP_PRIV_FLAGS_T;

/** Socket usage    */
typedef enum
{
    TRDP_SOCK_PD        = 0,                /**< Socket is used for UDP process data                    */
    TRDP_SOCK_MD_UDP    = 1,                /**< Socket is used for UDP message data                    */
    TRDP_SOCK_MD_TCP    = 2                 /**< Socket is used for TCP message data                    */
} TRDP_SOCK_TYPE_T;

/** Hidden handle definition, used as unique addressing item    */
typedef struct TRDP_HANDLE
{
    UINT32          comId;                              /**< comId for packets to send/receive           */
    TRDP_IP_ADDR_T  srcIpAddr;                          /**< source IP for PD                            */
    TRDP_IP_ADDR_T  destIpAddr;                         /**< destination IP for PD                       */
    TRDP_IP_ADDR_T  mcGroup;                            /**< multicast group to join for PD              */
    UINT32          topoCount;                          /**< topocount belongs to addressing item        */
} TRDP_ADDRESSES_T /*, *TRDP_PUB_PT, *TRDP_SUB_PT*/;


/** TCP parameters    */
typedef struct TRDP_SOCKET_TCP
{
    TRDP_IP_ADDR_T  cornerIp;                           /**< The other TCP corner Ip                      */
    BOOL            notSend;                            /**< If the message has been sent uncompleted     */
    TRDP_TIME_T     connectionTimeout;                  /**< TCP socket connection Timeout                */
    BOOL            sendNotOk;                          /**< The sending timeout will be start            */
    TRDP_TIME_T     sendingTimeout;                     /**< The timeout sending the message              */
    BOOL            addFileDesc;                        /**< Ready to add the socket in the fd            */
    BOOL            morituri;                           /**< about to die                                 */
}TRDP_SOCKET_TCP_T;


/** Socket item    */
typedef struct TRDP_SOCKETS
{
    INT32               sock;                            /**< vos socket descriptor to use                */
    TRDP_IP_ADDR_T      bindAddr;                        /**< Defines the interface to use                */
    TRDP_SEND_PARAM_T   sendParam;                       /**< Send parameters                             */
    TRDP_SOCK_TYPE_T    type;                            /**< Usage of this socket                        */
    BOOL                rcvMostly;                       /**< Used for receiving                          */
    INT16               usage;                           /**< No. of current users of this socket         */
    TRDP_SOCKET_TCP_T   tcpParams;                       /**< Params used for TCP                         */
    TRDP_IP_ADDR_T      mcGroups[VOS_MAX_MULTICAST_CNT]; /**< List of multicast addresses for this socket */
} TRDP_SOCKETS_T;

#ifdef WIN32
#pragma pack(push, 1)
#endif

/** TRDP PD packet    */
typedef struct
{
    PD_HEADER_T frameHead;                      /**< Packet    header in network byte order                 */
    UINT8       data[TRDP_MAX_PD_PACKET_SIZE];  /**< data ready to be sent or received (with CRCs)          */
} GNU_PACKED PD_PACKET_T;

/** TRDP MD packet    */
typedef struct
{
    MD_HEADER_T frameHead;                      /**< Packet    header in network byte order                 */
    UINT8       data[TRDP_MAX_MD_PACKET_SIZE];  /**< data ready to be sent or received (with CRCs)          */
} GNU_PACKED MD_PACKET_T;

#ifdef WIN32
#pragma pack(pop)
#endif

/** Queue element for PD packets to send or receive    */
typedef struct PD_ELE
{
    struct PD_ELE       *pNext;                 /**< pointer to next element or NULL                        */
    UINT32              magic;                  /**< prevent acces through dangeling pointer                */
    TRDP_ADDRESSES_T    addr;                   /**< handle of publisher/subscriber                         */
    TRDP_IP_ADDR_T      pullIpAddress;          /**< In case of pulling a PD this is the requested Ip       */
    UINT32              redId;                  /**< Redundancy group ID or zero                            */
    UINT32              curSeqCnt;              /**< the last sent or received sequence counter             */
    UINT32              curSeqCnt4Pull;         /**< the last sent sequence counter for PULL                */
    UINT32              numRxTx;                /**< Counter for received packets (statistics)              */
    UINT32              updPkts;                /**< Counter for updated packets (statistics)               */
    UINT32              getPkts;                /**< Counter for read packets (statistics)                  */
    TRDP_ERR_T          lastErr;                /**< Last error (timeout)                                   */
    TRDP_PRIV_FLAGS_T   privFlags;              /**< private flags                                          */
    TRDP_FLAGS_T        pktFlags;               /**< flags                                                  */
    TRDP_TIME_T         interval;               /**< time out value for received packets or
                                                     interval for packets to send (set from ms)             */
    TRDP_TIME_T         timeToGo;               /**< next time this packet must be sent/rcv                 */
    TRDP_TO_BEHAVIOR_T  toBehavior;             /**< timeout behavior for packets                           */
    UINT32              dataSize;               /**< net data size                                          */
    UINT32              grossSize;              /**< complete packet size (header, data, padding, FCS)      */
    UINT32              sendSize;               /**< data size sent out                                     */
    TRDP_DATASET_T      *pCachedDS;             /**< Pointer to dataset element if known                    */
    INT32               socketIdx;              /**< index into the socket list                             */
    const void          *userRef;               /**< from subscribe()                                       */
    PD_PACKET_T         *pFrame;                /**< header ... data + FCS...                               */
} PD_ELE_T, *TRDP_PUB_PT, *TRDP_SUB_PT;

/** Queue element for MD listeners (UDP and TCP)   */
typedef struct MD_LIS_ELE
{
    struct MD_LIS_ELE   *pNext;                 /**< pointer to next element or NULL                        */
    TRDP_ADDRESSES_T    addr;                   /**< addressing values                                      */
    TRDP_PRIV_FLAGS_T   privFlags;              /**< private flags                                          */
    TRDP_FLAGS_T        pktFlags;               /**< flags                                                  */
    const void          *pUserRef;              /**< user reference for call_back from tlm_request()        */
    TRDP_URI_USER_T     destURI;
    INT32               socketIdx;              /**< index into the socket list                             */
} MD_LIS_ELE_T;

/** Tcp connection parameters    */
typedef struct TRDP_MD_TCP
{
    BOOL    doConnect;                          /**< TCP connection state                                   */
    BOOL    msgUncomplete;                      /**< The receive message is uncomplete                   */
} TRDP_MD_TCP_T;

/** Session queue element for MD (UDP and TCP)  */
typedef struct MD_ELE
{
    struct MD_ELE       *pNext;                 /**< pointer to next element or NULL                        */
    TRDP_ADDRESSES_T    addr;                   /**< handle of publisher/subscriber                         */
    UINT32              curSeqCnt;              /**< the last sent or received sequence counter             */
    TRDP_PRIV_FLAGS_T   privFlags;              /**< private flags                                          */
    TRDP_FLAGS_T        pktFlags;               /**< flags                                                  */
    BOOL                morituri;               /**< about to die                                           */
    TRDP_TIME_T         interval;               /**< time out value for received packets or
                                                     interval for packets to send (set from ms)             */
    TRDP_TIME_T         timeToGo;               /**< next time this packet must be sent/rcv                 */
    UINT32              dataSize;               /**< net data size                                          */
    UINT32              grossSize;              /**< complete packet size (header, data, padding, FCS)      */
    UINT32              sendSize;               /**< data size sent out                                     */
    TRDP_DATASET_T      *pCachedDS;             /**< Pointer to dataset element if known                    */
    INT32               socketIdx;              /**< index into the socket list                             */
    UINT16              replyPort;              /**< replies are sent to the requesters source port         */
    TRDP_MD_ELE_ST_T    stateEle;               /**< internal status                                        */
    UINT8               sessionID[16];          /**< UUID as a byte stream                                  */
    UINT32              numExpReplies;          /**< number of expected repliers, 0 if unknown              */
    UINT32              numReplies;             /**< actual number of replies for the request               */
    UINT32              numRetriesMax;          /**< maximun number of retries for request to a know dev    */
    UINT32              numRetries;             /**< actual number of retries for request to a know dev     */
    UINT32              numRepliesQuery;        /**< number of ReplyQuery received, used to count nuomber
                                                     of expected Confirm sent                               */
    UINT32              numConfirmSent;         /**< number of Confirm sent                                 */
    UINT32              numConfirmTimeout;      /**< number of Confirm Timeouts (incremented by listeners   */
    const void          *pUserRef;              /**< user reference for call_back from tlm_request()        */
    TRDP_URI_USER_T     destURI;                /**< filter on incoming MD by destination URI               */
    TRDP_MD_TCP_T       tcpParameters;          /**< Tcp connection parameters                              */
    MD_PACKET_T         *pPacket;               /**< Packet    header in network byte order                 */
                                                /**< data ready to be sent (with CRCs)                      */
} MD_ELE_T;

/**    TCP file descriptor parameters   */
typedef struct
{
    INT32   listen_sd;          /**< TCP general socket listening connection requests   */
    INT32   max_sd;             /**< Maximum socket number in the file descriptor   */
    /* fd_set  master_set;         / **< Local file descriptor   * / */
} TRDP_TCP_FD_T;


/** Session/application variables store */
typedef struct TRDP_SESSION
{
    struct TRDP_SESSION     *pNext;             /**< Pointer to next session                                */
    VOS_MUTEX_T             mutex;              /**< protect this session                                   */
    TRDP_IP_ADDR_T          realIP;             /**< Real IP address                                        */
    TRDP_IP_ADDR_T          virtualIP;          /**< Virtual IP address                                     */
    BOOL                    beQuiet;            /**< if set, only react on ownIP requests                   */
    UINT32                  redID;              /**< redundant comId                                        */
    UINT32                  topoCount;          /**< current valid topocount or zero                        */
    TRDP_TIME_T             interval;           /**< Store for next select interval                         */
    TRDP_TIME_T             nextJob;
    TRDP_PRINT_DBG_T        pPrintDebugString;
    TRDP_MARSHALL_CONFIG_T  marshall;
    TRDP_PD_CONFIG_T        pdDefault;          /**< Default configuration for process data                 */
    TRDP_MEM_CONFIG_T       memConfig;
    TRDP_OPTION_T           option;
    TRDP_SOCKETS_T          iface[VOS_MAX_SOCKET_CNT];  /**< Collection of sockets to use                   */
    PD_ELE_T                *pSndQueue;         /**< pointer to first element of send queue                 */
    PD_ELE_T                *pRcvQueue;         /**< pointer to first element of rcv queue                  */
    TRDP_TIME_T             initTime;           /**< initialization time of session                         */
    TRDP_STATISTICS_T       stats;              /**< statistics of this session                             */
#if MD_SUPPORT
    TRDP_TCP_FD_T           tcpFd;              /**< TCP file descriptor parameters   */
    TRDP_MD_CONFIG_T        mdDefault;
    MD_LIS_ELE_T            *pMDListenQueue;    /**< pointer to first element of listeners queue            */
    MD_ELE_T                *pMDSndQueue;       /**< pointer to first element of send MD queue (caller)     */
    MD_ELE_T                *pMDRcvQueue;       /**< pointer to first element of recv MD queue (replier)    */
    MD_ELE_T                *pMDRcvEle;         /**< pointer to received MD element                         */
    MD_ELE_T                *uncompletedTCP[VOS_MAX_SOCKET_CNT];     /**< uncompleted TCP messages buffer   */
#endif
} TRDP_SESSION_T, *TRDP_SESSION_PT;


#endif
