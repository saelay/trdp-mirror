/**********************************************************************************************************************/
/**
 * @file            trdp_types.h
 *
 * @brief           Typedefs for TRDP communication
 *
 * @details
 *F
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

#ifndef TRDP_TYPES_H
#define TRDP_TYPES_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#include "vos_mem.h"
//#include "vos_sock.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

/** Maximum values	*/

/**  A uri is a string of the following form:                                 */
/**  trdp://[user part]@[host part]                                           */
/**  trdp://instLabel.funcLabel@devLabel.carLabel.cstLabel.trainLabel         */
/**  Hence the exact max. uri length is:                                      */
/**  7 + (6 * 15) + 5 * (sizeof (separator)) + 1(terminating 0)               */
/**  to facilitate alignment the size will be increased by 1 byte             */
#define TRDP_MAX_LABEL_LEN      16                              /**< label length incl. terminating '0' */
#define TRDP_MAX_URI_USER_LEN   (2 * TRDP_MAX_LABEL_LEN)        /**< URI user part incl. terminating '0' */
#define TRDP_MAX_URI_HOST_LEN   (4 * TRDP_MAX_LABEL_LEN)        /**< URI host part length incl. terminating '0' */
#define TRDP_MAX_URI_LEN        ((6 * TRDP_MAX_LABEL_LEN) + 8)  /**< URI length incl. terminating '0' and 1 padding byte */
#define TRDP_MAX_FILE_NAME_LEN  128                             /**< path and file name length incl. terminating '0' */

#define TDRP_VAR_SIZE           0                               /**< Variable size dataset	*/


#ifndef USE_HEAP
    #define USE_HEAP  0                 /**< If this is set, we can allocate dynamically memory	*/
#endif

/*	Special handling for Windows DLLs	*/
#ifdef WIN32
    #ifdef DLL_EXPORT
        #define EXT_DECL    __declspec(dllexport)
    #elif DLL_IMPORT
        #define EXT_DECL    __declspec(dllimport)
    #else
        #define EXT_DECL
    #endif

#else

    #define EXT_DECL

#endif

/***********************************************************************************************************************
 * TYPEDEFS
 */

/**********************************************************************************************************************/
/**                          TRDP general type definitions.                                                           */
/**********************************************************************************************************************/

/* for example: IP-Addr 10.0.8.35 translated to (10 * << 24) + (0 * << 16) + (8 << 8) + 35 */
typedef UINT32 TRDP_IP_ADDR_T;

typedef CHAR8 TRDP_LABEL_T[TRDP_MAX_LABEL_LEN];
typedef CHAR8 TRDP_URI_T[TRDP_MAX_URI_LEN];
typedef CHAR8 TRDP_URI_HOST_T[TRDP_MAX_URI_HOST_LEN];
typedef CHAR8 TRDP_URI_USER_T[TRDP_MAX_URI_USER_LEN];

typedef CHAR8 TRDP_FILE_NAME_T[TRDP_MAX_FILE_NAME_LEN];


/** Return codes for all API functions  */
typedef enum
{
    TRDP_NO_ERR             = 0,    /**< No error  */
    TRDP_PARAM_ERR          = -1,   /**< Parameter missing or out of range              */
    TRDP_INIT_ERR           = -2,   /**< Call without valid initialization              */
    TRDP_NOINIT_ERR         = -3,   /**< Call with invalid handle                       */
    TRDP_TIMEOUT_ERR        = -4,   /**< Timout                                         */
    TRDP_NODATA_ERR         = -5,   /**< Non blocking mode: no data received            */
    TRDP_SOCK_ERR           = -6,   /**< Socket error / option not supported            */
    TRDP_IO_ERR             = -7,   /**< Socket IO error, data can't be received/sent	*/
    TRDP_MEM_ERR            = -8,   /**< No more memory available           */
    TRDP_SEMA_ERR           = -9,   /**< Semaphore not available            */
    TRDP_QUEUE_ERR          = -10,  /**< Queue empty                        */
    TRDP_QUEUE_FULL_ERR     = -11,  /**< Queue full                         */
    TRDP_MUTEX_ERR          = -12,  /**< Mutex not available                */
    TRDP_NOSESSION_ERR      = -13,  /**< No such session                    */
    TRDP_SESSION_ABORT_ERR  = -14,  /**< Session aborted                    */
    TRDP_NOSUB_ERR          = -15,  /**< No subscriber                      */
    TRDP_NOPUB_ERR          = -16,  /**< No publisher                       */
    TRDP_NOLIST_ERR         = -17,  /**< No listener                        */
    TRDP_CRC_ERR            = -18,  /**< Wrong CRC                          */
    TRDP_WIRE_ERR           = -19,  /**<                                    */
    TRDP_TOPO_ERR           = -20,  /**< Invalid topo count                 */
    TRDP_COMID_ERR          = -21,  /**< Unknown ComId                      */
    TRDP_STATE_ERR          = -22,  /**< Call in wrong state				*/
    TRDP_UNKNOWN_ERR        = -99   /**< Unspecified error                  */
} TRDP_ERR_T;


/**	Timer value compatible with timeval / select.
 * Relative or absolute date, depending on usage
 */
typedef VOS_TIME_T TRDP_TIME_T;

/**	File descriptor set compatible with fd_set / select.
 */
/*typedef struct
   {
    UINT32  fd_count;
    INT32   fds_bits[VOS_MAX_SOCKET_CNT];
   } TRDP_FDS_T;
 */
typedef struct fd_set TRDP_FDS_T;
/*#define TRDP_FDS_T	fd_set */

/**********************************************************************************************************************/
/**                          TRDP data transfer type definitions.                                                     */
/**********************************************************************************************************************/

/** Message Types	*/
typedef enum
{
    TRDP_MSG_PD = 0x5064,     /**< 'Pd' PD Data (Reply)							*/
    TRDP_MSG_PR = 0x5072,     /**< 'Pr' PD Request								*/
    TRDP_MSG_PE = 0x5065,     /**< 'Pe' PD Error								*/
    TRDP_MSG_MN = 0x4D6E,     /**< 'Mn' MD Notification (Request without reply)	*/
    TRDP_MSG_MR = 0x4D72,     /**< 'Mr' MD Request with reply					*/
    TRDP_MSG_MP = 0x4D70,     /**< 'Mp' MD Reply without confirmation			*/
    TRDP_MSG_MQ = 0x4D71,     /**< 'Mq' MD Reply with confirmation				*/
    TRDP_MSG_MC = 0x4D63,     /**< 'Mc' MD Confirm								*/
    TRDP_MSG_ME = 0x4D65      /**< 'Me' MD Error								*/
} TRDP_MSG_T;

/** Reply status messages	*/
typedef enum
{
    TRDP_REPLY_OK = 0,
    TRDP_REPLY_APPL_TIMEOUT         = -1,
    TRDP_REPLY_SESSION_ABORT        = -2,
    TRDP_REPLY_NO_REPLIER_INST      = -3,
    TRDP_REPLY_NO_MEM_REPL          = -4,
    TRDP_REPLY_NO_MEM_LOCAL         = -5,
    TRDP_REPLY_NO_REPLY             = -6,
    TRDP_REPLY_NOT_ALL_REPLIES      = -7,
    TRDP_REPLY_NO_CONFIRM           = -8,
    TRDP_REPLY_WRONG_TOPO_COUNT     = -9,
    TRDP_REPLY_SENDING_FAILED       = -10,
    TRDP_REPLY_UNSPECIFIED_ERROR    = -99
} TRDP_REPLY_STATUS_T;


/** Various flags for PD and MD packets	*/
typedef enum
{
    TRDP_FLAGS_NONE         = 0,
    TRDP_FLAGS_REDUNDANT    = 0x1,      /**< Redundant                                          */
    TRDP_FLAGS_MARSHALL     = 0x2,      /**< Optional marshalling/unmarshalling in TRDP stack   */
    TRDP_FLAGS_CALLBACK     = 0x4,      /**< Use of callback function                           */
    TRDP_FLAGS_TCP          = 0x8       /**< Use TCP for message data                           */
} TRDP_FLAGS_T;


/** Redundancy states */
typedef enum
{
    TRDP_RED_FOLLOWER   = 0,     /**< Redundancy follower - redundant PD will be not sent out  */
    TRDP_RED_LEADER     = 1      /**< Redundancy leader - redundant PD will be sent out    */
} TRDP_RED_STATE_T;

/** How invalid PD shall be handled    */
typedef enum
{
    TRDP_TO_SET_TO_ZERO     = 1,      /**< If set, data will be reset to zero on time out         */
    TRDP_TO_KEEP_LAST_VALUE = 2       /**< If not set, last received values will be returned      */
} TRDP_TO_BEHAVIOR_T;

/**	Process data info from received telegram; allows the application to generate responses.
 *
 * Note: Not all fields are relevant for each message type!
 */
typedef struct
{
    TRDP_IP_ADDR_T  srcIpAddr;      /**< source IP address for filtering                */
    TRDP_IP_ADDR_T  destIpAddr;     /**< destination IP address for filtering           */
    UINT32          seqCount;       /**< sequence counter                               */
    UINT16          protVersion;    /**< Protocol version                               */
    TRDP_MSG_T      msgType;        /**< Protocol ('PD', 'MD', ...)                     */
    UINT32          comId;          /**< ComID                                          */
    UINT32          topoCount;      /**< received topocount                             */
    BOOL            subs;           /**< substitution                                   */
    UINT16          offsetAddr;     /**< offset address for ladder architecture         */
    UINT32          replyComId;     /**< ComID for reply (request only)                 */
    TRDP_IP_ADDR_T  replyIpAddr;    /**< IP address for reply (request only)            */
    const void      *pUserRef;      /**< User reference given with the local subscribe  */
    TRDP_ERR_T      resultCode;     /**< error code                                     */
} TRDP_PD_INFO_T;


/**	UUID definition reuses the VOS definition.
 */
typedef VOS_UUID_T TRDP_UUID_T;


/**	Message data info from received telegram; allows the application to generate responses.
 *
 * Note: Not all fields are relevant for each message type!
 */
typedef struct
{
    TRDP_IP_ADDR_T      srcIpAddr;    /**< source IP address for filtering      */
    TRDP_IP_ADDR_T      destIpAddr;   /**< destination IP address for filtering	*/
    UINT32              seqCount;     /**< sequence counter                     */
    UINT16              protVersion;  /**< Protocol version                     */
    TRDP_MSG_T          msgType;      /**< Protocol ('PD', 'MD', ...)			*/
    UINT32              comId;        /**< ComID								*/
    UINT32              topoCount;    /**< received topocount					*/
    UINT16              userStatus;   /**< error code, user stat                */
    TRDP_REPLY_STATUS_T replyStatus;  /**< reply status			                */
    TRDP_UUID_T         sessionId;    /**< for response							*/
    UINT32              replyTimeout; /**< reply timeout in us given with the request */
    TRDP_URI_USER_T     destURI;      /**< destination URI user part from MD header	  */
    TRDP_URI_USER_T     srcURI;       /**< source URI user part from MD header        */
    UINT32              noOfReplies;  /**< actual number of replies for the request   */
    const void          *pUserRef;    /**< User reference given with the local call   */
    TRDP_ERR_T          resultCode;   /**< error code								  */
} TRDP_MD_INFO_T;


/**	Quality/type of service and time to live	*/
typedef struct
{
    UINT8   qos;
    UINT8   ttl;
} TRDP_SEND_PARAM_T;


/**********************************************************************************************************************/
/**                          TRDP dataset description definitions.                                                    */
/**********************************************************************************************************************/

/**	Dataset element definition	*/
typedef enum
{
    TRDP_BOOLEAN    = 1,   /**< =UINT8, 1 bit relevant (equal to zero = false, not equal to zero = true) */
    TRDP_CHAR8      = 2,   /**< char, can be used also as UTF8  */
    TRDP_UTF16      = 3,   /**< Unicode UTF-16 character  */
    TRDP_INT8       = 4,   /**< Signed integer, 8 bit  */
    TRDP_INT16      = 5,   /**< Signed integer, 16 bit  */
    TRDP_INT32      = 6,   /**< Signed integer, 32 bit  */
    TRDP_INT64      = 7,   /**< Signed integer, 64 bit  */
    TRDP_UINT8      = 8,   /**< Unsigned integer, 8 bit  */
    TRDP_UINT16     = 9,   /**< Unsigned integer, 16 bit  */
    TRDP_UINT32     = 10,  /**< Unsigned integer, 32 bit  */
    TRDP_UINT64     = 11,  /**< Unsigned integer, 64 bit  */
    TRDP_REAL32     = 12,  /**< Floating point real, 32 bit  */
    TRDP_REAL64     = 13,  /**< Floating point real, 64 bit  */
    TRDP_TIMEDATE32 = 14,  /**< 32 bit UNIX time  */
    TRDP_TIMEDATE48 = 15,  /**< 48 bit TCN time (32 bit UNIX time and 16 bit ticks)  */
    TRDP_TIMEDATE64 = 16,  /**< 32 bit UNIX time + 32 bit microseconds (== struct timeval) */
    TRDP_TYPE_MAX   = 30   /**< Values greater are considered nested datasets */
} TRDP_DATA_TYPE_T;

/**	Dataset element definition	*/
typedef struct
{
    UINT32  type;            /**< Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 */
    UINT32  size;            /**< Number of items or TDRP_VAR_SIZE (0) */
} TRDP_DATASET_ELEMENT_T;

/**	Dataset definition	*/
typedef struct
{
    UINT32                  id;           /**< dataset identifier > 1000 */
    UINT16                  reserved1;    /**< Reserved for future use, must be zero  */
    UINT16                  numElement;   /**< Number of elements */
    TRDP_DATASET_ELEMENT_T  pElement[];   /**< Pointer to a dataset element, used as array */
} TRDP_DATASET_T;

/**	Dataset element definition	*/
typedef struct
{
    UINT32  comId;              /**< comId */
    UINT32  datasetId;          /**< corresponding dataset Id */
} TRDP_COMID_DSID_MAP_T;



/**********************************************************************************************************************/
/**                          TRDP statistics type definitions.                                                        */
/**********************************************************************************************************************/

/** Statistical data
 * regarding the former info provided via SNMP the following information was left out/can be implemented additionally using MD:
 *   - PD subscr table:  ComId, sourceIpAddr, destIpAddr, cbFct?, timout, toBehaviour, counter
 *   - PD publish table: ComId, destIpAddr, redId, redState cycle, ttl, qos, counter
 *   - PD join table:    joined MC address table
 *   - MD listener table: ComId  destIpAddr, destUri, cbFct?, counter
 *   - Memory usage
 */


/** Structure containing all general memory statistics information. */
typedef struct
{
    UINT32  total;                              /**< total memory size */
    UINT32  free;                               /**< free memory size */
    UINT32  minFree;                            /**< minimal free memory size in statistics interval */
    UINT32  numAllocBlocks;                     /**< allocated memory blocks */
    UINT32  numAllocErr;                        /**< allocation errors */
    UINT32  numFreeErr;                         /**< free errors */
    UINT32  allocBlockSize[TRDP_MEM_BLK_524288 + 1];     /**< allocated memory blocks */
    UINT32  usedBlockSize[TRDP_MEM_BLK_524288 + 1];      /**< used memory blocks */
} TRDP_MEM_STATISTICS_T;


/** Structure containing all general PD statistics information. */
typedef struct
{
    UINT32  defQos;           /**< default QoS for PD */
    UINT32  defTtl;           /**< default TTL for PD */
    UINT32  defTimeout;       /**< default timeout in us for PD */
    UINT32  numSubs;          /**< number of subscribed ComId's */
    UINT32  numPub;           /**< number of published ComId's */
    UINT32  numRcv;           /**< number of received PD packets */
    UINT32  numCrcErr;        /**< number of received PD packets with CRC err */
    UINT32  numProtErr;       /**< number of received PD packets with protocol err */
    UINT32  numTopoErr;       /**< number of received PD packets with wrong topo count */
    UINT32  numNoSubs;        /**< number of received PD push packets without subscription */
    UINT32  numNoPub;         /**< number of received PD pull packets without publisher */
    UINT32  numTimeout;       /**< number of PD timeouts */
    UINT32  numSend;          /**< number of sent PD  packets */
} TRDP_PD_STATISTICS_T;


/** Structure containing all general MD statistics information. */
typedef struct
{
    UINT32  defQos;                /**< default QoS for MD */
    UINT32  defTtl;                /**< default TTL for MD */
    UINT32  defReplyTimeout;       /**< default reply timeout in us for MD */
    UINT32  defConfirmTimeout;     /**< default confirm timeout in us for MD */
    UINT32  numList;               /**< number of listeners */
    UINT32  numRcv;                /**< number of received MD packets */
    UINT32  numCrcErr;             /**< number of received MD packets with CRC err */
    UINT32  numProtErr;            /**< number of received MD packets with protocol err */
    UINT32  numTopoErr;            /**< number of received MD packets with wrong topo count */
    UINT32  numNoListener;         /**< number of received MD packets without listener */
    UINT32  numReplyTimeout;       /**< number of reply timeouts */
    UINT32  numConfirmTimeout;     /**< number of confirm timeouts */
    UINT32  numSend;               /**< number of sent MD packets */
} TRDP_MD_STATISTICS_T;


/** Structure containing all general memory, PD and MD statistics information. */
typedef struct
{
    UINT32                  version;      /**< TRDP version  */
    TIMEDATE64              timeStamp;    /**< actual time stamp */
    TIMEDATE32              upTime;       /**< time in sec since last initialisation */
    TIMEDATE32              statisticTime; /**< time in sec since last reset of statistics */
    TRDP_LABEL_T            hostName;     /**< host name */
    TRDP_LABEL_T            leaderName;   /**< leader host name */
    TRDP_IP_ADDR_T          ownIpAddr;    /**< own IP address */
    TRDP_IP_ADDR_T          leaderIpAddr; /**< leader IP address */
    UINT32                  processPrio;  /**< priority of TRDP process */
    UINT32                  processCycle; /**< cycle time of TRDP process in microseconds */
    UINT32                  numJoin;      /**< number of joins */
    UINT32                  numRed;       /**< number of redundancy groups */
    TRDP_MEM_STATISTICS_T   mem;          /**< memory statistics */
    TRDP_PD_STATISTICS_T    pd;           /**< pd statistics */
    TRDP_MD_STATISTICS_T    udpMd;        /**< UDP md statistics */
    TRDP_MD_STATISTICS_T    tcpMd;        /**< TCP md statistics */
} TRDP_STATISTICS_T;

/** Table containing particular PD subscription information. */
typedef struct
{
    UINT32              comId;    /**< Subscribed ComId */
    TRDP_IP_ADDR_T      joinedAddr; /**< Joined IP address */
    TRDP_IP_ADDR_T      filterAddr; /**< Filter IP address, i.e IP address of the sender for this subscription, 0.0.0.0 in case all senders. */
    UINT32              callBack; /**< Reference for call back function if used */
    UINT32              timeout;  /**< Time-out value in us. 0 = No time-out supervision */
    TRDP_ERR_T          status;   /**< Receive status information TRDP_NO_ERR, TRDP_TIMEOUT_ERR */
    TRDP_TO_BEHAVIOR_T  toBehav;  /**< Behaviour at time-out. Set data to zero / keep last value */
    UINT32              numRecv;  /**< Number of packets received for this subscription. */
} TRDP_SUBS_STATISTICS_T;

/** Table containing particular PD publishing information. */
typedef struct
{
    UINT32          comId;      /**< Published ComId  */
    TRDP_IP_ADDR_T  destAddr;   /**< IP address of destination for this publishing. */
    UINT32          cycle;      /**< Publishing cycle in us */
    UINT32          redId;      /**< Redundancy group id */
    UINT32          redState;   /**< Redundant state.Leader or Follower */
    UINT32          numPut;     /**< Number of packet updates */
    UINT32          numSend;    /**< Number of packets sent out */
} TRDP_PUB_STATISTICS_T;


/** Information about a particular MD listener */
typedef struct
{
    UINT32          comId;      /**< ComId to listen to */
    TRDP_URI_USER_T uri;        /**< URI user part to listen to */
    TRDP_IP_ADDR_T  joinedAddr; /**< Joined IP address */
    UINT32          callBack;   /**< Call back function reference if used */
    UINT32          queue;      /**< Queue reference if used */
    UINT32          userRef;    /**< User reference if used */
    UINT32          numRecv;    /**< Number of received packets */
} TRDP_LIST_STATISTICS_T;


/** A table containing PD redundant group information */
typedef struct
{
    UINT32              id;    /**< Redundant Id */
    TRDP_RED_STATE_T    state; /**< Redundant state.Leader or Follower */
} TRDP_RED_STATISTICS_T;



/**********************************************************************************************************************/
/**                          TRDP configuration type definitions.                                                     */
/**********************************************************************************************************************/


/**********************************************************************************************************************/
/**	Callback function definition for error/debug output, reuse of the VOS defined function.
 */

typedef VOS_PRINT_DBG_T TRDP_PRINT_DBG_T;


/**********************************************************************************************************************/
/** Categories for logging, reuse of the VOS definition
 */

typedef VOS_LOG_T TRDP_LOG_T;



/**********************************************************************************************************************/
/**	Function type for marshalling .
 * The function must know about the dataset's alignment etc.
 *
 *  @param[in]	    *pRefCon   pointer to user context
 *  @param[in]		comId      ComId to identify the structure out of a configuration
 *  @param[in]		*pSrc      pointer to received original message
 *  @param[in]	    *pDst      pointer to a buffer for the treated message
 *  @param[in,out]	*pDstSize  size of the provide buffer / size of the treated message
 *
 *  @retval         TRDP_NO_ERR	    no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

typedef TRDP_ERR_T (*TRDP_MARSHALL_T)(
    void        *pRefCon,
    UINT32      comId,
    UINT8 		*pSrc,
    UINT8       *pDst,
    UINT32      *pDstSize);


/**********************************************************************************************************************/
/**	Function type for unmarshalling.
 * The function must know about the dataset's alignment etc.
 *
 *  @param[in]	    *pRefCon   pointer to user context
 *  @param[in]		comId      ComId to identify the structure out of a configuration
 *  @param[in]		*pSrc      pointer to received original message
 *  @param[in]	    *pDst      pointer to a buffer for the treated message
 *  @param[in,out]	*pDstSize  size of the provide buffer / size of the treated message
 *
 *  @retval         TRDP_NO_ERR	    no error
 *  @retval         TRDP_MEM_ERR    provide buffer to small
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

typedef TRDP_ERR_T (*TRDP_UNMARSHALL_T)(
    void        *pRefCon,
    UINT32      comId,
    UINT8 		*pSrc,
    UINT8       *pDst,
    UINT32      *pDstSize);


/**********************************************************************************************************************/
/** Marshaling/unmarshalling configuration	*/
typedef struct
{
    TRDP_MARSHALL_T     pfCbMarshall;           /**< Pointer to marshall callback function      */
    TRDP_UNMARSHALL_T   pfCbUnmarshall;         /**< Pointer to unmarshall callback function    */
    void                *pRefCon;               /**< Pointer to user context for call back      */
} TRDP_MARSHALL_CONFIG_T;


/**********************************************************************************************************************/
/**	Callback for receiving indications, timeouts, releases, responses.
 *
 *  @param[in]	*pRefCon   pointer to user context
 *  @param[in]	*pMsg      pointer to received message information
 *  @param[in]	*pData     pointer to received data
 *  @param[in]	dataSize   size of received data pointer to received data excl. padding and FCS !!!!
 */
typedef void (*TRDP_PD_CALLBACK_T)(
    void                    *pRefCon,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize);


/**********************************************************************************************************************/
/** Default PD configuration	*/
typedef struct
{
    TRDP_PD_CALLBACK_T  pfCbFunction;           /**< Pointer to PD callback function			*/
    void                *pRefCon;               /**< Pointer to user context for call back      */
    TRDP_SEND_PARAM_T   sendParam;              /**< Default send parameters			*/
    TRDP_FLAGS_T        flags;                  /**< Default flags for PD packets				*/
    UINT32              timeout;                /**< Default timeout in us                      */
    TRDP_TO_BEHAVIOR_T  toBehavior;             /**< Default timeout behaviour                  */
    UINT32              port;                   /**< Port to be used for PD communication       */
} TRDP_PD_CONFIG_T;


/**********************************************************************************************************************/
/**	Callback for receiving indications, timeouts, releases, responses.
 *
 *  @param[in]	*pRefCon   pointer to user context
 *  @param[in]	*pMsg      pointer to received message information
 *  @param[in]	*pData     pointer to received data
 *  @param[in]	dataSize   size of received data pointer to received data excl. padding and FCS !!!!
 */
typedef void (*TRDP_MD_CALLBACK_T)(
    void                    *pRefCon,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize);


/**********************************************************************************************************************/
/** Default MD configuration
 */
typedef struct
{
    TRDP_MD_CALLBACK_T  pfCbFunction;           /**< Pointer to MD callback function			*/
    void                *pRefCon;               /**< Pointer to user context for call back      */
    TRDP_SEND_PARAM_T   sendParam;              /**< Default send parameters			*/
    TRDP_FLAGS_T        flags;                  /**< Default flags for MD packets				*/
    UINT32              replyTimeout;           /**< Default timeout in us                      */
    UINT32              confirmTimeout;         /**< Default timeout in us                      */
    UINT32              udpPort;                /**< Port to be used for UDP MD communication   */
    UINT32              tcpPort;                /**< Port to be used for TCP MD communication   */
} TRDP_MD_CONFIG_T;



/**********************************************************************************************************************/
/** Enumeration type for memory pre-fragmentation, reuse of VOS definition.
 */
typedef VOS_MEM_BLK_T TRDP_MEM_BLK_T;


/** Structure describing memory	(and its pre-fragmentation)	*/
typedef struct
{
    UINT8   *p;                                     /**< pointer to static or allocated memory  */
    UINT32  size;                                   /**< size of static or allocated memory     */
    UINT32  prealloc[TRDP_MEM_BLK_524288 + 1];      /**< memory block structure                 */
} TRDP_MEM_CONFIG_T;



/**********************************************************************************************************************/
/** Various flags/general TRDP options for library initialization
 */
typedef enum
{
    TRDP_OPTION_NONE    = 0,
    TRDP_OPTION_BLOCK   = 0x01,         /**< Default: Use nonblocking I/O calls, polling necessary
                                             Set: Read calls will block, use select()	*/
    TRDP_OPTION_TRAFFIC_SHAPING = 0x02  /**< Use traffic shaping - distribute packet sending    */
} TRDP_OPTION_T;

/**********************************************************************************************************************/
/** Various flags/general TRDP options for library initialization
 */
typedef struct {
    TRDP_LABEL_T  hostName;         /**< Host name  */
    TRDP_LABEL_T  leaderName;       /**< Leader name dependant on redundancy concept   */
    UINT32        cycleTime;        /**< TRDP main process cycle time in us  */
    UINT32        priority;         /**< TRDP main process cycle time (0-255, 0=default, 255=highest)   */
    TRDP_OPTION_T options;          /**< TRDP options */
} TRDP_PROCESS_CONFIG_T;


#ifdef __cplusplus
}
#endif

/**
   \mainpage The TRDP Light Library API Specification

   \image html TCNOpen.png
   \image latex TCNOpen.png

   \section general_sec General Information

   \subsection purpose_sec Purpose

   The TRDP protocol has been defined as the standard communication protocol in
   IP-enabled trains. It allows communication via process data (periodically transmitted
   data using UDP/IP) and message data (client - server messaging using UDP/IP or TCP/IP)
   This document describes the light API of the TRDP Library.

   \subsection scope_sec Scope

   The intended audience of this document is the developers and project members of
   the TRDP project. TRDP Client Applications are programs using the TRDP protocol library to
   access the services of TRDP. Programmers developing such applications are the
   main target audience for this documentation.

   \subsection document_sec Related documents

   TCN-TRDP2-D-BOM-004-01 IEC61375-2-3_CD_ANNEXA  Protocol definition of the TRDP standard

   \subsection abbreviation_sec Abbreviations and Definitions

   -<em> API </em>	Application Programming Interface

   -<em> ECN </em>	Ethernet Consist Network

   -<em> TRDP </em> Train Real-time Data Protocol

   -<em> TCMS </em>	Train Control Management System

   \section terminology_sec Terminology

   The API documented here is mainly concerned with three
   bodies of code:

   - <em> TRDP Client Applications </em> (or 'client applications'
   for short): These are programs using the API to access the
   services of TRDP.  Programmers developing such applications are
   the main target audience for this documentation.

   - <em> TRDP Light Implementations </em> (or just 'TRDP
   implementation'): These are libraries realising the API
   as documented here.  Programmers developing such implementations
   will find useful definitions about syntax and semantics of the
   API wihtin this documentation.

   - <em> VOS Subsystem </em> (Virtual Operating System): An OS and hardware
   abstraction layer which offers memory, networking, threading, queues and debug functions.
   The VOS API is documented here.

   The following diagram shows how these pieces of software are
   interrelated.

   \image html SingleThreadedWorkflowPD.pdf "Sample client workflow"
   \image latex SingleThreadedWorkflowPD.eps "Sample client workflow" width=\textwidth


   \section api_conventions_sec Conventions of the API

   The API comprises a set of C header files that can also be used
   from client applications written in C++.  These header files are
   contained in a directory named \c trdp/api and a subdirectory called
   \c trdp/vos/api with declarations not topical to TRDP but needed by
   the stack.  Client applications shall include these header files
   like:

   \code #include "trdp_if_light.h" \endcode
   and, if VOS functions are needed, also the corresponding headers:
   \code #include "vos_thread.h" \endcode for example.


   The subdirectory \c trdp/doc contains files needed for the API
   documentation.

   Generally client application source code including API headers will
   only compile if the parent directory of the \c trdp directory is
   part of the include path of the used compiler.  No other
   subdirectories of the API should be added to the compiler's include
   path.

   The client API doesn't support a "catch-all" header file that
   includes all declarations in one step; rather the client
   application has to include individual headers for each feature set
   it wants to use.




 */
#endif
