//**********************************************************************************************************************/
/**
 * @file            mdTestApp.h
 *
 * @brief           Define, Global Variables, ProtoType  for TRDP Ladder Topology Support
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, TOSHIBA
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright TOSHIBA, Japan, 2013.
 *
 *
 *
 */
#ifndef MDTESTAPP_H_
#define MDTESTAPP_H_

#ifdef TRDP_OPTION_LADDER
/*******************************************************************************
 * INCLUDES
 */
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <sys/select.h>
#include <mqueue.h>
#include <time.h>

#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <ifaddrs.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

// SP 30/10/2012
// Suppor for log library
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#ifdef __linux__
	#include <uuid/uuid.h>
#endif

#include "trdp_types.h"
#include "trdp_if_light.h"
#include "trdp_private.h"
#include "trdp_utils.h"
#include "trdp_ladder_app.h"
#include "trdp_mdcom.h"

/***********************************************************************************************************************
 * DEFINES
 */

/* MD Application Version */
#ifdef LITTLE_ENDIAN
#define MD_APP_VERSION	"V0.01"
#elif BIG_ENDIAN
#define MD_APP_VERSION	"V0.01"
#else
#define MD_APP_VERSION	"V0.01"
#endif

/* Application Session Handle - Message Queue Descriptor Table Size Max */
#define APP_SESSION_HANDLE_MQ_DESC_TABLE_MAX 1000

/* MD Request (Mr) sessionId(UUID) Table Size Max */
#define REQUEST_SESSIONID_TABLE_MAX 1000

/* Convert into IP address from number */
#define TRDP_IP4_ADDR(a,b,c,d) ( (am_big_endian()) ? \
	((UINT32)((d) & 0xFF) << 24) | ((UINT32)((c) & 0xFF) << 16) | ((UINT32)((b) & 0xFF) << 8) | ((UINT32)((a) & 0xFF)) : \
	((UINT32)((a) & 0xFF) << 24) | ((UINT32)((b) & 0xFF) << 16) | ((UINT32)((c) & 0xFF) << 8) | ((UINT32)((d) & 0xFF)))
/*
	((UINT32)((a) & 0xFF) << 24) | ((UINT32)((b) & 0xFF) << 16) | ((UINT32)((c) & 0xFF) << 8) | ((UINT32)((d) & 0xFF)) : \
	((UINT32)((d) & 0xFF) << 24) | ((UINT32)((c) & 0xFF) << 16) | ((UINT32)((b) & 0xFF) << 8) | ((UINT32)((a) & 0xFF)))
*/

/* MD Transfer Request ComId */
#define COMID_INCREMENT_DATA		0x200006
#define COMID_FIXED_DATA1			0x200001		/* Single Packet */
#define COMID_FIXED_DATA2			0x200002		/* Multi Packet */
#define COMID_FIXED_DATA3			0x200003		/* UDP Max Packet */
#define COMID_FIXED_DATA4			0x200004		/* TCP Packet (128K Octet) */
#define COMID_FIXED_DATA5			0x200005		/* TCP Max Packet */
#define COMID_FIXED_DATA6			0x300001		/* 512 Octet Packet */
#define COMID_ERROR_DATA_1			0x400001
#define COMID_ERROR_DATA_2			0x400002
#define COMID_ERROR_DATA_3			0x400003
#define COMID_ERROR_DATA_4			0x400004
/* MD Transfer Reply ComId */
#define COMID_INCREMENT_DATA_REPLY		0x2A0006
#define COMID_FIXED_DATA1_REPLY			0x2A0001		/* Single Packet */
#define COMID_FIXED_DATA2_REPLY			0x2A0002		/* Multi Packet */
#define COMID_FIXED_DATA3_REPLY			0x2A0003		/* UDP Max Packet */
#define COMID_FIXED_DATA4_REPLY			0x2A0004		/* TCP Packet (128K Octet) */
#define COMID_FIXED_DATA5_REPLY			0x2A0005		/* TCP Max Packet */
#define COMID_FIXED_DATA6_REPLY			0x3A0001		/* 512 Octet Packet */
#define COMID_ERROR_DATA_1_REPLY		0x4A0001
#define COMID_ERROR_DATA_2_REPLY		0x4A0002
#define COMID_ERROR_DATA_3_REPLY		0x4A0003
#define COMID_ERROR_DATA_4_REPLY		0x4A0004
/* MD Reply ComId Mask */
#define COMID_REPLY_MASK					0xA0000

/* MD DATA SIZE */
#define MD_INCREMENT_DATA_MIN_SIZE	4				/* MD Increment DATA Minimum Size : 4B */
#define MD_INCREMENT_DATA_MAX_SIZE	58760			/* MD Increment DATA Max Size : 58760B */
#define MD_DATA_UDP_MAX_SIZE		65536			/* UDP MD DATA Max Size : 64KB */
#define MD_DATA_TCP_MAX_SIZE		65536			/* TCP MD DATA Max Size : 64KB */
#define MD_HEADER_SIZE				112				/* MD Header */
#define MD_FCS_SIZE					4				/* MD FCS Size */

/* MD DATA */
#define MD_DATA_INCREMENT_CYCLE			10			/* MD Increment DATA Cycle */
#define MD_DATA_FILE_NAME_MAX_SIZE		128			/* ComId-MdDataFileName Max Size */
#define MD_DATA_FIXED_FILE_NAME_SIZE	16			/* ComId-MdDataFixedFileName Max Size */

/* Input Command */
#define GET_COMMAND_MAX				1000			/* INPUT COMMAND MAX */
#define SPACE							 ' '			/* SPACE Character */

/* Message Queue */
#define MESSAGE_QUEUE_NAME_SIZE				24			/* Message Queue Name Size */
#define THREAD_COUNTER_CHARACTER_SIZE		10			/* Thread Counter Character Size */
#define TRDP_QUEUE_MAX_SIZE					(sizeof(trdp_apl_cbenv_t)-2)
#define TRDP_QUEUE_MAX_MESG 				10

/* LOG */
#define CALLER_LOG_BUFFER_SIZE		1024			/* Caller Log String Buffer Size : 1KB */
#define PIPE_BUFFER_SIZE				64 * 1024		/* LOG Pipe Message Buffer Size : 64KB */
//#define PIPE_BUFFER_SIZE				CALLER_LOG_BUFFER_SIZE		/* LOG Pipe Message Buffer Size : 64KB */
#define HALF_PIPE_BUFFER_SIZE		PIPE_BUFFER_SIZE / 2
#define LOG_OUTPUT_BUFFER_SIZE		12 * 1024		/* LOG Output Buffer Size : 12KB */
#define MD_OPERARTION_RESULT_LOG	0x1				/* MD Operation Result Log Enable */
#define MD_SEND_LOG					0x2				/* MD Send Log Enable */
#define MD_RECEIVE_LOG				0x4				/* MD Receive Log Enable */
#define MD_DUMP_ON					1				/* Log Display Dump Enable */
#define MD_DUMP_OFF					0				/* Log Display Dump Disable */

/* IP Address Nothing */
#define IP_ADDRESS_NOTHING			0				/* IP Address Nothing */

#define RECURSIVE_CALL_NOTHING		0				/* Recursive Call Count Nothing */

/***********************************************************************************************************************
 * TYPEDEFS
 */
/* Application Thread SessionId Handle */
typedef struct
{
	TRDP_UUID_T			mdAppThreadSessionId;
	TRDP_LIS_T				pMdAppThreadListener;
} APP_THREAD_SESSION_HANDLE;

/* Application Thread Session Handle - Message Queue Descriptor Table */
typedef struct
{
	APP_THREAD_SESSION_HANDLE appThreadSessionHandle;
	mqd_t mqDescriptor;
} APP_THREAD_SESSION_HANDLE_MQ_DESCRIPTOR;

/* Caller-Replier Type definition */
typedef enum
{
    CALLER          = 0,   /**< Caller */
    REPLIER         = 1    /**< Replier */
} CALLER_REPLIER_TYPE;

/* MD Message Kind definition */
typedef enum
{
    MD_MESSAGE_MN			= 0,	/**< Mn 		Message */
    MD_MESSAGE_MR_MP		= 1		/**< Mr-Mp		Message */
} MD_MESSAGE_KIND;

/* MD Telegram Type definition */
typedef enum
{
    INCREMENT_DATA	= 0,	/**< Increment DATA */
    FIXED_DATA_1		= 1,	/**< Fixed DATA 1 */
    FIXED_DATA_2		= 2,	/**< Fixed DATA 2 */
    FIXED_DATA_3		= 3,	/**< Fixed DATA 3 */
    FIXED_DATA_4		= 4,	/**< Fixed DATA 4 */
    FIXED_DATA_5		= 5,	/**< Fixed DATA 5 */
    FIXED_DATA_6		= 6,	/**< Fixed DATA 6 */
    ERROR_DATA_1		= 7,	/**< Error DATA 1 */
    ERROR_DATA_2		= 8,	/**< Error DATA 2 */
    ERROR_DATA_3		= 9,	/**< Error DATA 3 */
    ERROR_DATA_4		= 10	/**< Error DATA 4 */
} MD_TELEGRAM_TYPE;

/* MD Reply Error Type definition */
typedef enum
{
    MD_REPLY_ERROR_TYPE_1		= 1,	/**< MD Reply Error Type 1 : Reply Status:-1 */
    MD_REPLY_ERROR_TYPE_2		= 2,	/**< MD Reply Error Type 2 : No Memory */
    MD_REPLY_ERROR_TYPE_3		= 3,	/**< MD Reply Error Type 3 : Err ComId:0 */
    MD_REPLY_ERROR_TYPE_4		= 4,	/**< MD Reply Error Type 4 : dataSize:-1 */
    MD_REPLY_ERROR_TYPE_5		= 5,	/**< MD Reply Error Type 5 : Not Call tlm_reply() */
    MD_REPLY_ERROR_TYPE_6		= 6,	/**< MD Reply Error Type 6 : No Listener */
} MD_REPLY_ERROR_TYPE;


/* MD Transport Type definition */
typedef enum
{
    MD_TRANSPORT_UDP	= 0,	/**< UDP */
    MD_TRANSPORT_TCP	= 1		/**< TCP */
} MD_TRANSPORT_TYPE;

/* MD Application Error Type definition */
typedef enum
{
    MD_APP_NO_ERR			= 0,			/**< MD Application No Error */
    MD_APP_ERR			= -1,			/**< MD Application Error */
    MD_APP_PARAM_ERR		= -2,			/**< MD Application Parameter Error */
    MD_APP_MEM_ERR		= -3,			/**< MD Application Memory Error */
    MD_APP_THREAD_ERR	= -4,			/**< MD Application Thread Error */
    MD_APP_MUTEX_ERR		= -5			/**< MD Application Thread Mutex Error */
} MD_APP_ERR_TYPE;

/* MD Reply Error Type definition */
typedef enum
{
	MD_REPLY_NO_ERR				= 0,	/**< No Error */
	MD_REPLY_STATUS_ERR			= 1,	/**< ReplyStatus=1 Error */
	MD_REPLY_MEMORY_ERR			= 2,	/**< Memory Error */
	MD_REPLY_COMID_ERR			= 3,	/**< ComId Error */
	MD_REPLY_DATASIZE_ERR		= 4,	/**< DataSize=-1 Error */
	MD_REPLY_NOSEND_ERR			= 5,	/**< Not Send Error */
	MD_REPLY_NOLISTENER_ERR		= 6		/**< Not Add Listener Error */
} MD_REPLY_ERR_TYPE;


/* MD DATA CREATE FLAG Type definition */
typedef enum
{
    MD_DATA_CREATE_ENABLE	= 0,	/**< MD DATA_CREATE : ON */
    MD_DATA_CREATE_DISABLE	= 1		/**< MD DATA CREATE : OFF */
} MD_DATA_CREATE_FLAG;

/* MD DATA CREATE FLAG Type definition */
typedef enum
{
    MD_SEND_USE_SUBNET1	= 1,	/**< MD Mn/Mr Send I/F = Subnet1 */
    MD_SEND_USE_SUBNET2	= 2		/**< MD Mn/Mr Send I/F = Subnet2 */
} MD_SEND_USE_SUBNET;

/* Command Value */
typedef struct
{
/*	UINT8 mdSourceSinkType; */					/* -a --md-application-type Value */
	UINT8 mdCallerReplierType;					/* -b --md-caller-replier-type Value */
	UINT8 mdTransportType;						/* -c --md-transport-type Value */
	UINT8 mdMessageKind;							/* -d --md-message-kind Value */
	UINT8 mdTelegramType;						/* -e --md-telegram-type Value */
	UINT32 mdMessageSize;						/* -f --md-message-size Value */
	TRDP_IP_ADDR_T mdDestinationAddress;		/* -g --md-destination-address Value */
	UINT8 mdDump;									/* -i --md-dump Value */
	UINT8 mdReplierNumber;						/* -j --md-replier-number Value */
	UINT32 mdCycleNumber;						/* -k --md-cycle-number Value */
	UINT8 mdLog;									/* -l --md-log Value */
	UINT32 mdCycleTime;							/* -m --md-cycle-time Value */
	BOOL mdLadderTopologyFlag;					/* -n --md-topo Value */
	UINT8 mdReplyErr;								/* -o --md-reply-err Value */
	BOOL mdMarshallingFlag;						/* -p --md-marshall Value*/
	UINT32 mdAddListenerComId;					/* -q --md-listener-comid Value */
	UINT32 mdSendComId;							/* Caller Send comId */
	MD_DATA_CREATE_FLAG createMdDataFlag;		/* Caller use for a decision of MD create */
	UINT32 mdTimeoutReply;						/* -r --md-timeout-reply Value */
/*	UINT32 mdTimeoutConfirm;	*/					/* -s --md-timeout-confirm Value */
	UINT8 mdSendSubnet;							/* -t --md-send-subnet Value */
} COMMAND_VALUE;


/* ComId - MD Data File Name */
typedef struct
{
	UINT32 comId;
	CHAR8 mdDataFileName[MD_DATA_FIXED_FILE_NAME_SIZE];
} COMID_MD_DATA_FILE_NAME;

/* MDReceiveManager Thread Parameter */
typedef struct
{
	COMMAND_VALUE *pCommandValue;
} MD_RECEIVE_MANAGER_THREAD_PARAMETER;

/* Caller Thread Parameter */
typedef struct
{
	COMMAND_VALUE *pCommandValue;
	void *pMdData;
	UINT32 mdDataSize;
	char mqName[MESSAGE_QUEUE_NAME_SIZE];
} CALLER_THREAD_PARAMETER;

/* Replier Thread Parameter */
typedef struct
{
	COMMAND_VALUE *pCommandValue;
	char mqName[MESSAGE_QUEUE_NAME_SIZE];
} REPLIER_THREAD_PARAMETER;

/* message queue trdp to application */
typedef struct
{
	void                 *pRefCon;
	TRDP_MD_INFO_T        Msg;
	UINT8                *pData;
	UINT32               dataSize;
	CHAR8					timeStampString[64];
	int                  dummy;
} trdp_apl_cbenv_t;


/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */
/* Subnet1 */
extern TRDP_APP_SESSION_T		appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
extern TRDP_MD_CONFIG_T			md_config;
extern TRDP_MEM_CONFIG_T			mem_config;
extern TRDP_PROCESS_CONFIG_T	processConfig;
extern COMMAND_VALUE				trdpInitializeParameter;	/* Use to trdp_initialize */

/* Subnet2 */
extern TRDP_APP_SESSION_T		appHandle2;				/*	Sub-network Id2 identifier to the library instance	*/
extern TRDP_MD_CONFIG_T			md_config2;
extern TRDP_MEM_CONFIG_T			mem_config2;
extern TRDP_PROCESS_CONFIG_T	processConfig2;

/* IP Address */
extern TRDP_IP_ADDR_T subnetId1Address;		/* Subnet1 Network I/F Address */
extern TRDP_IP_ADDR_T subnetId2Address;		/* Subnet2 Network I/F Address */
/* URI */
extern TRDP_URI_USER_T subnetId1URI;			/* Subnet1 Network I/F URI */
extern TRDP_URI_USER_T subnetId2URI;			/* Subnet2 Network I/F URI */
extern TRDP_URI_USER_T noneURI;					/* URI nothing */

extern CHAR8 LOG_PIPE[];							/* named PIPE for log */

extern UINT32 sendMdTransferRequestCounter;	/* Send MD Transfer Request Count */

/***********************************************************************************************************************
 * PROTOTYPES
 */

/* Thread */
MD_APP_ERR_TYPE  lockMdApplicationThread (
    void);

MD_APP_ERR_TYPE  unlockMdApplicationThread (
    void);

/**********************************************************************************************************************/
/** MDReceiveManager Thread
 */
VOS_THREAD_FUNC_T MDReceiveManager (
	MD_RECEIVE_MANAGER_THREAD_PARAMETER *pMdReceiveManagerThreadParameter);
/**********************************************************************************************************************/
/** MDCaller Thread
 */
VOS_THREAD_FUNC_T MDCaller (
		CALLER_THREAD_PARAMETER *pCallerThreadParameter);

/**********************************************************************************************************************/
/** MDReplier Thread
 */
VOS_THREAD_FUNC_T MDReplier (
		REPLIER_THREAD_PARAMETER *pReplierThreadParameter);

/**********************************************************************************************************************/
/** MDLog Thread
 */
VOS_THREAD_FUNC_T MDLog (
		void);


/* Common */
/**********************************************************************************************************************/
/** Message Queue Initialize
 *
 *  @param[in]		pMqName			pointer to Message Queue Name
 *  @param[out]		pMqDescriptor		pointer to Message Queue Descriptor
 *
 */
MD_APP_ERR_TYPE queue_initialize (
		const char *pMqName,
		mqd_t *pMqDescriptor);

/**********************************************************************************************************************/
/** Send message trough message queue
 *
 *  @param[in]		msg					pointer to Message queue trdp to application
 *  @param[in]		mqDescriptor		Message Queue Descriptor
 *
 */
MD_APP_ERR_TYPE queue_sendMessage (
		trdp_apl_cbenv_t * msg,
		mqd_t mqDescriptor);

/**********************************************************************************************************************/
/** Receive message from message queue
 *
 *  @param[in,out]	msg					pointer to Message queue trdp to application
 *  @param[in]		mqDiscripter		Message Queue Descriptor
 *
 */
MD_APP_ERR_TYPE queue_receiveMessage (
		trdp_apl_cbenv_t * msg,
		mqd_t mqDescriptor);

/**********************************************************************************************************************/
/** Set Message Queue Descriptor
 *
 *  @param[in]		pAppThreadSessionHandle		pointer to MD Application Thread Session Handle
 *  @param[in]		mqDescriptor					Message Queue Descriptor
 *
 */
MD_APP_ERR_TYPE setAppThreadSessionMessageQueueDescriptor (
		APP_THREAD_SESSION_HANDLE *pAppThreadSessionHandle,
		mqd_t mqDescriptor);

/**********************************************************************************************************************/
/** Delete Message Queue Descriptor
 *
 *  @param[in]		pAppThreadSessionHandle		pointer to MD Application Thread Session Handle
 *  @param[in]		mqDescriptor					Message Queue Descriptor
 *
 *  @retval         mqDescriptor		no error
 *  @retval         -1					error
 */
mqd_t deleteAppThreadSessionMessageQueueDescriptor(
		APP_THREAD_SESSION_HANDLE *pAppThreadSessionHandle,
		mqd_t mqDescriptor);

/**********************************************************************************************************************/
/** Get Message Queue Descriptor
 *
 *  @param[in,out]	pLoopStartNumber				pointer to Loop Starting Number
 *  @param[in]		mdMsgType						MD Message Type
 *  @param[in]		pAppThreadSessionHandle		pointer to MD Application Thread Session Handle
 *
 *  @retval         mqDescriptor		no error
 *  @retval         -1					error
 */
mqd_t getAppThreadSessionMessageQueueDescriptor (
		UINT32 *pLoopStartNumber,
		TRDP_MSG_T mdMsgType,
		APP_THREAD_SESSION_HANDLE *pAppThreadSessionHandle);

/**********************************************************************************************************************/
/** Decide MD Transmission Success or Failure
 *
 *  @param[in]		receiveComId				Receive ComId
 *  @param[in]		pReceiveMdData			pointer to Receive MD DATA
 *  @param[in]		pReceiveMdDataSize		pointer to Receive MD DATA Size
 *  @param[in,out]	pLogString					pointer to Log String
 *
 *  @retval         MD_APP_NO_ERR				Success
 *  @retval         MD_APP_ERR					Failure
 *
 */
MD_APP_ERR_TYPE decideMdTransmissionResult(
		UINT32 receiveComId,
		UINT8 *pReceiveMdData,
		UINT32 *pReceiveMdDataSize,
		CHAR8 *pLogString);

/**********************************************************************************************************************/
/** Create MD Increment DATA
 *
 *  @param[in]		mdSendCount				MD Request Send Count Number
 *  @param[in]		mdDataSize					Create Message Data Size
 *  @param[out]		pMdData					pointer to Created Message Data
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *  @retval         MD_APP_MEM_ERR				Memory error
 *
 */
MD_APP_ERR_TYPE createMdIncrementData (
		UINT32 mdSendCount,
		UINT32 mdDataSize,
		UINT8 ***pppMdData);

/**********************************************************************************************************************/
/** Create MD Fixed DATA
 *
 *  @param[in]		comId						Create Message ComId
 *  @param[out]		pMdData					pointer to Created Message Data
 *  @param[out]		pMdDataSize				pointer to Created Message Data Size
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *  @retval         MD_APP_MEM_ERR				Memory error
 *
 */
MD_APP_ERR_TYPE createMdFixedData (
		UINT32 comId,
		UINT8 ***pppMdData,
		UINT32 *pMdDataSize);

/**********************************************************************************************************************/
/** Get MD DATA File Name from ComId
 *
 *  @param[in]		comId						Create Message ComId
 *  @param[out]		pMdDataFileName			pointer to MD DATA FILE NAME
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *  @retval         MD_APP_MEM_ERR				Memory error
 *
 */
MD_APP_ERR_TYPE getMdDataFileNameFromComId (
		UINT32 comId,
		char *pMdDataFileName);

/**********************************************************************************************************************/
/** Get MD DATA from ComId
 *
 *  @param[in]		receiveComId				Receive ComId
 *  @param[in]		pReceiveMdData			pointer to Receive MD DATA
 *  @param[in]		pReceiveMdDataSize		pointer to Receive MD DATA Size
 *  @param[out]		pCheckMdData				pointer to Create for Check MD DATA
 *  @param[out]		pCheckMdDataSize			pointer to Create for Check MD DATA Size
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *  @retval         MD_APP_MEM_ERR				Memory error
 *
 */
/*
MD_APP_ERR_TYPE getMdDataFromComId (
		UINT32 receiveComId,
		UINT8 *pReceiveMdData,
		UINT32 *pReceiveMdDataSize,
		UINT8 *pCheckMdData,
		UINT32 *pCheckMdDataSize);
*/
MD_APP_ERR_TYPE getMdDataFromComId(
		UINT32 receiveComId,
		UINT8 *pReceiveMdData,
		UINT32 *pReceiveMdDataSize,
		UINT8 **ppCheckMdData,
		UINT32 **ppCheckMdDataSize);

/**********************************************************************************************************************/
/** String Conversion to memory (for MD DATA)
 *
 *  @param[in]		pStartAddress				pointer to Start Memory Address
 *  @param[in]		memorySize					convert Memory Size
 *  @param[in]		logKind					Log Kind
 *  @param[in]		dumpOnOff					Dump Enable or Disable
 *  @param[in]		callCount					miscMemory2String Recursive Call Count
 *
 *  @retval         String Start Address		no error
 *  @retval         NULL							error
 *
 */
MD_APP_ERR_TYPE miscMemory2String (
		const void *pStartAddress,
		size_t memorySize,
		int logKind,
		int dumpOnOff,
		int callCount);

// Convert an IP address to string
char * miscIpToString(
		int ipAdd,
		char *strTmp);

/**********************************************************************************************************************/
/** Send log string to server
 *
 *  @param[in]		logString					pointer to Log String
 *  @param[in]		logKind					Log Kind
 *  @param[in]		dumpOnOff					Dump Enable or Disable
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *
 */
int l2fLog (
		char *logString,
		int logKind,
		int dumpOnOff);

/**********************************************************************************************************************/
/** Decide MD Result code
 *
 *  @param[in]		mdResultCode				Result Code
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *  @retval         MD_APP_MEM_ERR				Memory error
 *
 */
MD_APP_ERR_TYPE decideResultCode(
		TRDP_ERR_T mdResultCode);

/* debug display function */
void private_debug_printf (
	void *pRefCon,
	VOS_LOG_T category,
	const CHAR8 *pTime,
	const CHAR8 *pFile,
	UINT16 LineNumber,
	const CHAR8 *pMsgStr);


/* main */
/**********************************************************************************************************************/
/** Create MdLog Thread
 *
 *  @param[in]		void
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdLogThread (
		void);

/**********************************************************************************************************************/
/** Create MdReceiveManager Thread
 *
 *  @param[in]		pMdReceiveManagerThreadParameter			pointer to MdReceiveManagerThread Parameter
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdReceiveManagerThread (
		MD_RECEIVE_MANAGER_THREAD_PARAMETER *pMdReceiveManagerThreadParameter);

/**********************************************************************************************************************/
/** Create MdCaller Thread
 *
 *  @param[in]		pCallerThreadParameter			pointer to CallerThread Parameter
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdCallerThread (
		CALLER_THREAD_PARAMETER *pCallerThreadParameter);

/**********************************************************************************************************************/
/** Create MdReplier Thread
 *
 *  @param[in]		pReplierThreadParameter			pointer to ReplierThread Parameter
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdReplierThread (
		REPLIER_THREAD_PARAMETER *pReplierThreadParameter);

/**********************************************************************************************************************/
/** Analyze Command
 *
 *  @param[in]		argc
 *  @param[in]		argv
 *  @param[in,out]	pCommandValue		pointer to Command Value
 *
 *  @retval         0					no error
 *  @retval         -1					error
 */
MD_APP_ERR_TYPE analyzeCommand (
		int argc,
		char *argv[],
		COMMAND_VALUE *pCommandValue);

/** decide MD Transfer Pattern
 *
 *  @param[in,out]		pCommandValue		pointer to Command Value
 *  @param[out]			pMdData			pointer to Create MD DATA
 *  @param[out]			pMdDataSize		pointer to Create MD DATA Size
 *
 *  @retval         0					no error
 *  @retval         -1					error
 */
MD_APP_ERR_TYPE decideMdPattern (
		COMMAND_VALUE *pCommandValue,
		 UINT8 **ppMdData,
		 UINT32 **ppMdDataSize);

/**********************************************************************************************************************/
/** main thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
MD_APP_ERR_TYPE command_main_proc (
		void);

/**********************************************************************************************************************/
/** Decide Create Thread
 *
 *  @param[in]		argc
 *  @param[in]		argv
 *  @param[in]		pCommandValue			pointer to Command Value
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE decideCreateThread (
		int argc,
		char *argv[],
		COMMAND_VALUE *pCommandValue);


/* MDReceiveManager */
/**********************************************************************************************************************/
/** TRDP initialization
 *
 *  @param[in]		void
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
MD_APP_ERR_TYPE trdp_initialize(void);

/**********************************************************************************************************************/
/** call back function for message data receive
 *
 *  @param[in]		pRefCon		user supplied context pointer
 *  @param[in]		appHandle 		handle returned also by tlc_init
 *  @param[in]		pMsg			pointer to MDInformation
 *  @param[in]		pData			pointer to receive MD Data
 *  @param[in]		dataSize		receive MD Data Size
 *
 */
void md_indication (
    void                 *pRefCon,
    TRDP_APP_SESSION_T	appHandle,
    const TRDP_MD_INFO_T *pMsg,
    UINT8                *pData,
    UINT32               dataSize);

/**********************************************************************************************************************/
/** MDReceiveManager thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
MD_APP_ERR_TYPE mdReceive_main_proc (
		void);

/* Caller */
/**********************************************************************************************************************/
/** Caller thread main loop process
 *
 *  @param[in]		mqDescriptor						Message Queue Descriptor
 *  @param[in]		replierComId						Replier ComId
 *  @param[in]		pCallerThreadParameter			pointer to Caller Thread parameter
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
MD_APP_ERR_TYPE caller_main_proc (
		mqd_t mqDescriptor,
		UINT32 replierComId,
		CALLER_THREAD_PARAMETER *pCallerThreadParameter);


/* Replier */
/**********************************************************************************************************************/
/** Replier thread main loop process
 *
 *  @param[in]		mqDescriptor						Message Queue Descriptor
 *  @param[in]		replierComId						Replier ComId
 *  @param[in]		pReplierThreadParameter			pointer to Replier Thread parameter
 *
 *  @retval         0					no error
 *  @retval         1					error
 *
 */
MD_APP_ERR_TYPE replier_main_proc (
		mqd_t mqDescriptor,
		UINT32 replierComId,
		REPLIER_THREAD_PARAMETER *pReplierThreadParameter);

/**********************************************************************************************************************/
/** Decide Receive MD DATA
 *
 *  @param[in]		pReceiveMsg						pointer to Receive MD Message
 *  @param[in]		pReplierThreadParameter			pointer to Replier Thread parameter
 *
 *  @retval         0					no error
 *  @retval         1					error
 *
 */
MD_APP_ERR_TYPE decideReceiveMdDataToReplier (
		trdp_apl_cbenv_t *pReceiveMsg,
		REPLIER_THREAD_PARAMETER *pReplierThreadParameter);


/* Log */
/**********************************************************************************************************************/
/** Log FIFO Receive
 *
 *
 */
MD_APP_ERR_TYPE l2fWriterServer (
		void);

/**********************************************************************************************************************/
/** Write Log File and Dump
 *
 *  @param[in]		logMsg								Log Message
 *  @param[in]		logFilePath						Log File Path
 *  @param[in]		dumpOnOff							Dump Enable/Disable Flag
 *
 *  @retval         0					no error
 *  @retval         1					error
 *
 */
int l2fFlash(
		char *logMsg,
		const char *logFilePath,
		int dumpOnOff);


#endif	/* TRDP_OPTION_LADDER */

#ifdef __cplusplus
}
#endif

#endif /* MDTESTAPP_H_ */
