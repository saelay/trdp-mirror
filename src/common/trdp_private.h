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
#include "vos_thread.h"


/***********************************************************************************************************************
 * DEFINES
 */

#define LIB_VERSION  "0.0.0.6"

#ifndef IP_PD_UDP_PORT
#define IP_PD_UDP_PORT  20548                   /**< process data UDP port      */
#endif
#ifndef IP_MD_UDP_PORT
#define IP_MD_UDP_PORT  20550                   /**< message data UDP port      */
#endif
#ifndef IP_MD_TCP_PORT
#define IP_MD_TCP_PORT  20550                   /**< message data TCP port      */
#endif


#define IP_PD_PROTO_VER             0x0100      /**< Protocol version           */
#define IP_MD_PROTO_VER             0x0100

#define ECHO_COMID                  110         /**< comid used for echo        */
#define TIMER_GRANULARITY           10000       /**< granularity in us          */

#define MD_DEFAULT_REPLY_TIMEOUT    10000000    /**< default reply time out 10s    */
#define MD_DEFAULT_CONFIRM_TIMEOUT  10000000    /**< default reply time out 10s    */

#define PD_DEFAULT_QOS              0
#define PD_DEFAULT_TTL              64

#define MD_DEFAULT_QOS              0
#define MD_DEFAULT_TTL              64

#define MIN_PD_HEADER_SIZE          sizeof(PD_HEADER_T)     /**< PD header size with FCS */
#define MAX_PD_DATA_SIZE            1436
#define MAX_PD_PACKET_SIZE          MAX_PD_DATA_SIZE + MIN_PD_HEADER_SIZE
#define MAX_MD_PACKET_SIZE          1024 * 64

/** Default value in milliseconds for waiting on acknowledge message */
#define ACK_TIME_OUT_VAL_DEF  500

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Internal flags for packets    */
typedef enum
{
    TRDP_PRIV_NONE      = 0,
    TRDP_MC_JOINT       = 0x1,
    TRDP_TIMED_OUT      = 0x2,              /**< if set, informed the user                              */
    TRDP_REQ_2B_SENT    = 0x4,              /**< if set, the request needs to be sent                   */
    TRDP_PULL_SUB       = 0x8               /**< if set, its a PULL subscription                        */
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
    UINT32              comId;              /**< comId for packets to send/receive                      */
    TRDP_IP_ADDR_T      srcIpAddr;          /**< source IP for PD                                       */
    TRDP_IP_ADDR_T      destIpAddr;         /**< destination IP for PD                                  */
    TRDP_IP_ADDR_T      mcGroup;            /**< multicast group to join for PD                         */
} TRDP_ADDRESSES, *TRDP_PUB_PT, *TRDP_SUB_PT;

/** Socket item    */
typedef struct TRDP_SOCKETS
{
    INT32               sock;               /**< vos socket descriptor to use                           */
    TRDP_IP_ADDR_T      bindAddr;           /**< Defines the interface to use                           */
    TRDP_SEND_PARAM_T   sendParam;          /**< Send parameters                                        */
    TRDP_SOCK_TYPE_T    type;               /**< Usage of this socket                                   */
    UINT16              usage;              /**< No. of current users of this socket                    */
} TRDP_SOCKETS_T;

#ifdef WIN32
#pragma pack(push, 1)
#endif

/** TRDP process data header - network order and alignment    */
typedef struct
{
    UINT32  sequenceCounter;                /**< Unique counter (autom incremented)                     */
    UINT16  protocolVersion;                /**< fix value for compatibility (set by the API)           */
    UINT16  msgType;                        /**< of datagram: PD Request (0x5072) or PD_MSG (0x5064)    */
    UINT32  comId;                          /**< set by user: unique id                                 */
    UINT32  topoCount;                      /**< set by user: ETB to use, '0' to deacticate             */
    UINT32  datasetLength;                  /**< length of the data to transmit 0...1436
                                                 without padding and FCS                                */
    UINT16  subsAndReserved;                /**< first bit (MSB): indicates substitution transmission   */
    UINT16  offsetAddress;                  /**< for process data in traffic store                      */
    UINT32  replyComId;                     /**< used in PD request                                     */
    UINT32  replyIpAddress;                 /**< used for PD request                                    */
    UINT32  frameCheckSum;                  /**< CRC32 of header                                        */
} GNU_PACKED PD_HEADER_T;

/** TRDP message data header - network order and alignment    */
typedef struct
{
    UINT32  sequenceCounter;                /**< Unique counter (autom incremented)             */
    UINT16  protocolVersion;                /**< fix value for compatibility                    */
    UINT16  msgType;                        /**< of datagram: Mn, Mr, Mp, Mq, Mc or Me          */
    UINT32  comId;                          /**< set by user: unique id                         */
    UINT32  topoCount;                      /**< set by user: ETB to use, '0' to deacticate     */
    UINT32  datasetLength;                  /**< defined by user: length of data to transmit    */
    INT32   replyStatus;                    /**< 0 = OK                                         */
    UINT8   sessionID[16];                  /**< UUID as a byte stream                          */
    UINT32  replyTimeout;                   /**< in us                                          */
    UINT8   sourceURI[32];                  /**<    User part of URI                            */
    UINT8   destinationURI[32];             /**<    User part of URI                            */
    UINT32  frameCheckSum;                  /**< CRC32 of header                                */
} GNU_PACKED MD_HEADER_T;

#ifdef WIN32
#pragma pack(pop)
#endif

/** Queue element for PD packets to send or receive    */
typedef struct PD_ELE
{
    struct PD_ELE       *pNext;                 /**< pointer to next element or NULL                    */
    TRDP_ADDRESSES      addr;                   /**< handle of publisher/subscriber                     */
    TRDP_IP_ADDR_T      pullIpAddress;          /**< In case of pulling a PD this is the requested Ip   */
    UINT32              curSeqCnt;              /**< the last sent or received sequence counter         */
    TRDP_PRIV_FLAGS_T   privFlags;              /**< private flags                                      */
    TRDP_FLAGS_T        pktFlags;               /**< flags                                              */
    TRDP_TIME_T         interval;               /**< time out value for received packets or
                                                     interval for packets to send (set from ms)         */
    TRDP_TIME_T         timeToGo;               /**< next time this packet must be sent/rcv             */
    TRDP_TO_BEHAVIOR_T  toBehavior;             /**< timeout behavior for packets                       */
    UINT32              dataSize;               /**< net data size                                      */
    UINT32              grossSize;              /**< complete packet size (header, data, padding, FCS)  */
    INT32               socketIdx;              /**< index into the socket list                         */
    const void          *userRef;               /**< from subscribe()                                   */
    PD_HEADER_T         frameHead;              /**< Packet    header in network byte order             */
    UINT8               data[MAX_PD_PACKET_SIZE]; /**< data ready to be sent or received (with CRCs)    */
    /*    ... data + FCS... */
} PD_ELE_T;

/** Queue element for MD packets to send or receive or acknowledge    */
typedef struct MD_ELE
{
    struct MD_ELE       *pNext;                 /**< pointer to next element or NULL                    */
    TRDP_ADDRESSES      addr;                   /**< handle of publisher/subscriber                     */
    TRDP_PRIV_FLAGS_T   privFlags;              /**< private flags                                      */
    TRDP_TIME_T         interval;               /**< time out value for received packets or
                                                    interval for packets to send (set from ms)          */
    TRDP_TIME_T         timeToGo;               /**< next time this packet must be sent/rcv             */
    INT32               dataSize;               /**< net data size                                      */
    INT32               socketIdx;              /**< index into the socket list                         */
    MD_HEADER_T         frameHead;              /**< Packet    header in network byte order             */
    UINT8               data[0];                /**< data ready to be sent (with CRCs)                  */
    /*    ... data + FCS ... */
} MD_ELE_T;

/** Session/application variables store */
typedef struct TRDP_SESSION
{
    struct TRDP_SESSION     *pNext;     /**< Pointer to next session                    */
    VOS_MUTEX_T             mutex;      /**< protect this session                       */
    TRDP_IP_ADDR_T          realIP;     /**< Real IP address                            */
    TRDP_IP_ADDR_T          virtualIP;  /**< Virtual IP address                         */
    BOOL                    beQuiet;    /**< if set, only react on ownIP requests       */
    UINT32                  redID;      /**< redundant comId                            */
    UINT32                  topoCount;  /**< current valid topocount or zero            */
    TRDP_TIME_T             interval;   /**< Store for next select interval             */
    TRDP_TIME_T             nextJob;
    TRDP_PRINT_DBG_T        pPrintDebugString;
    TRDP_MARSHALL_CONFIG_T  marshall;
    TRDP_PD_CONFIG_T        pdDefault;  /**< Default configuration for process data     */
    TRDP_MD_CONFIG_T        mdDefault;
    TRDP_MEM_CONFIG_T       memConfig;
    TRDP_OPTION_T           option;
    TRDP_SOCKETS_T          iface[VOS_MAX_SOCKET_CNT];  /**< Collection of sockets to use    */
    PD_ELE_T                *pSndQueue; /**< pointer to first element of send queue       */
    PD_ELE_T                *pRcvQueue; /**< pointer to first element of rcv queue        */
    MD_ELE_T                *pMDQueue;  /**< pointer to first element of MD session       */
    TRDP_STATISTICS_T       stats;      /**< statistics of this session                   */
} TRDP_SESSION_T, *TRDP_SESSION_PT;

/** Process data statistics */
typedef struct TRDP_PD_STATISTICS
{
    UINT32  headerInPackets;            /**< Incoming packets                 */
    UINT32  headerInCRCErr;             /**< Incoming CRC errors              */
    UINT32  headerInProtoErr;           /**< Incoming protocol errors         */
    UINT32  headerInTimeOuts;           /**< Incoming timing errors           */
    UINT32  headerInFrameErr;           /**< Incoming timing errors           */
    UINT32  headerOutPackets;           /**< Outgoing packets                 */
} TRDP_PD_STATS_T;

/** Message data statistics */
typedef struct TRDP_MD_STATISTICS
{
    UINT32  headerInPackets;            /**< Incoming packets                 */
    UINT32  headerInCRCErr;             /**< Incoming CRC errors              */
    UINT32  headerInProtoErr;           /**< Incoming protocol errors         */
    UINT32  headerInTimeOuts;           /**< Incoming timing errors           */
    UINT32  headerInFrameErr;           /**< Incoming timing errors           */
    UINT32  headerOutPackets;           /**< Outgoing packets                 */
    UINT32  headerAckErr;               /**< Missing acknowledge              */
} TRDP_MD_STATS_T;

#endif
