/**********************************************************************************************************************/
/**
 * @file            trdp_ladder_app.h
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
 *          Copyright TOSHIBA, Japan, 2012.
 *
 *
 *
 */

#ifndef TRDP_LADDER_APP_H_
#define TRDP_LADDER_APP_H_

#ifdef TRDP_OPTION_LADDER
/*******************************************************************************
 * INCLUDES
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "vos_thread.h"
#include "trdp_utils.h"

/***********************************************************************************************************************
 * DEFINES
 */

/* PD Application Version */
#ifdef LITTLE_ENDIAN
#define PD_APP_VERSION	"V0.11"
#elif BIG_ENDIAN
#define PD_APP_VERSION	"V0.11"
#else
#define PD_APP_VERSION	"V0.11"
#endif

#define SUBNET2_NETMASK								0x00002000			/* The netmask for Subnet2 */
#define PDCOM_LADDER_THREAD_DELAY_TIME				10000				/* PDComLadder Thread starting Judge Cycle in us */
#define PDCOM_MULTICAST_GROUPING_DELAY_TIME		    10000000			/* PDComLadder Thread starting Wait Time in us */
#define PUT_DATASET_BUFFER_SIZE						1					/* putDataSet Backup Buffer */

#define PD_APP_THREAD_NOT_SUBSCRIBE	0x1				/* PDAppThread Not tlp_subscribe */
#define PD_APP_THREAD_NOT_PUBLISH		0x2				/* PDAppThread Not tlp_publish */
#define PD_APP_THREAD_NOT_SUB_PUB		PD_APP_THREAD_NOT_SUBSCRIBE | PD_APP_THREAD_NOT_PUBLISH	/* PDAppThread Not tlp_subscribe, tlp_publish */

/* Some sample comId definitions	*/
#define PD_DATA_SIZE_MAX			300					/* PD DATA MAX SIZE */
//#define LADDER_APP_CYCLE			500000				/* Ladder Application data put cycle in us */

/* Subscribe for Sub-network Id1 */
#define PD_COMID1_DATA_SIZE		32

/* We use static memory	*/
#define RESERVED_MEMORY  1024 * 1024

/* Convert into IP address from number */
#define TRDP_IP4_ADDR(a,b,c,d) ( (am_big_endian()) ? \
	((UINT32)((d) & 0xFF) << 24) | ((UINT32)((c) & 0xFF) << 16) | ((UINT32)((b) & 0xFF) << 8) | ((UINT32)((a) & 0xFF)) : \
	((UINT32)((a) & 0xFF) << 24) | ((UINT32)((b) & 0xFF) << 16) | ((UINT32)((c) & 0xFF) << 8) | ((UINT32)((d) & 0xFF)))
/*
	((UINT32)((a) & 0xFF) << 24) | ((UINT32)((b) & 0xFF) << 16) | ((UINT32)((c) & 0xFF) << 8) | ((UINT32)((d) & 0xFF)) : \
	((UINT32)((d) & 0xFF) << 24) | ((UINT32)((c) & 0xFF) << 16) | ((UINT32)((b) & 0xFF) << 8) | ((UINT32)((a) & 0xFF)))
*/
/* Input Command */
#define GET_COMMAND_MAX				1000			/* INPUT COMMAND MAX */
#define SPACE							 ' '			/* SPACE Character */

/* Valid ComId Type */
#define ENABLE_COMDID1				0x1				/* comId1 Enable */
#define ENABLE_COMDID2				0x2				/* comid2 Enable */

/* DATASET of COMID */
#define DATASET1_COMID				10001			/* DATASET1 of ComId : 10001 */
#define DATASET2_COMID				10002			/* DATASET2 of ComId : 10002 */

/***********************************************************************************************************************
 * TYPEDEFS
 */
/* PD Application Error Type definition */
typedef enum
{
    PD_APP_NO_ERR			= 0,			/**< PD Application No Error */
    PD_APP_ERR			= -1,			/**< PD Application Error */
    PD_APP_PARAM_ERR		= -2,			/**< PD Application Parameter Error */
    PD_APP_MEM_ERR		= -3,			/**< PD Application Memory Error */
    PD_APP_THREAD_ERR	= -4,			/**< PD Application Thread Error */
    PD_APP_MUTEX_ERR		= -5,			/**< PD Application Thread Mutex Error */
    PD_APP_COMMAND_ERR	= -6			/**< PD Application Command Error */
} PD_APP_ERR_TYPE;

/* Command Value */
typedef struct PD_COMMAND_VALUE
{
//	BOOL ladderTopologyFlag;						/* Ladder Topology : TURE, Not Ladder Topology : FALSE */
	UINT16 OFFSET_ADDRESS1;						/* offsetAddress comId1 publish */
//	UINT16 OFFSET_ADDRESS2;						/* offsetAddress comId2 publish */
	UINT16 OFFSET_ADDRESS3;						/* offsetAddress comId1 subscribe */
//	UINT16 OFFSET_ADDRESS4;						/* offsetAddress comId2 subscribe */
	UINT32	LADDER_APP_CYCLE;						/* Ladder Application cycle in us */
	UINT32 PD_PUB_COMID1;						/* Publish ComId1 */
//	UINT32 PD_PUB_COMID2;						/* Publish ComId2 */
	UINT32 PD_SUB_COMID1;						/* Subscribe ComId1 */
//	UINT32 PD_SUB_COMID2;						/* Subscribe ComId2 */
	TRDP_IP_ADDR_T PD_COMID1_SUB_SRC_IP1;		/* Subscribe ComId1 Source IP1 */
	TRDP_IP_ADDR_T PD_COMID1_SUB_SRC_IP2;		/* Subscribe ComId1 Source IP2 */
	TRDP_IP_ADDR_T PD_COMID1_SUB_DST_IP1;		/* Subscribe ComId1 Destination IP1 */
	TRDP_IP_ADDR_T PD_COMID1_SUB_DST_IP2;		/* Subscribe ComId1 Destination IP2 */
//	TRDP_IP_ADDR_T PD_COMID2_SUB_SRC_IP1;		/* Subscribe ComId2 Source IP1 */
//	TRDP_IP_ADDR_T PD_COMID2_SUB_SRC_IP2;		/* Subscribe Comid2 Source IP2 */
//	TRDP_IP_ADDR_T PD_COMID2_SUB_DST_IP1;		/* Subscribe ComId2 Destination IP1 */
//	TRDP_IP_ADDR_T PD_COMID2_SUB_DST_IP2;		/* Subscribe ComId2 Destination IP2 */
	TRDP_IP_ADDR_T PD_COMID1_PUB_DST_IP1;		/* Publish ComID1 Destination IP1 */
	TRDP_IP_ADDR_T PD_COMID1_PUB_DST_IP2;		/* Publish ComID1 Destination IP2 */
//	TRDP_IP_ADDR_T PD_COMID2_PUB_DST_IP1;		/* Publish ComID2 Destination IP1 */
//	TRDP_IP_ADDR_T PD_COMID2_PUB_DST_IP2;		/* Publish ComID2 Destination IP2 */
	UINT32 PD_COMID1_TIMEOUT;				    /* Subscribe ComId1 Timeout : Macro second */
//	UINT32 PD_COMID2_TIMEOUT;				    /* Subscribe ComId2 Timeout : Macro second */
	UINT32 PD_COMID1_CYCLE;						/* Publish ComID1 Cycle TIme */
//	UINT32 PD_COMID2_CYCLE;						/* Publish ComID2 Cycle TIme */
	UINT32 TS_SUBNET;								/* Traffic Store Using Subnet */
	UINT32 subnet1ReceiveCount;					/* Subscribe subnet1 receive Count */
	UINT32 subnet2ReceiveCount;					/* Subscribe subnet2 receive Count */
	UINT32 subnet1TimeoutReceiveCount;			/* Subscribe subnet1 timeout receive Count */
	UINT32 subnet2TimeoutReceiveCount;			/* Subscribe subnet2 timeout receive Count */
	struct PD_COMMAND_VALUE *pNextPdCommandValue;	/* pointer to next PD_COMMAND_VALUE or NULL */
} PD_COMMAND_VALUE;

/* PD Thread Parameter */
typedef struct
{
	PD_COMMAND_VALUE *pPdCommandValue;
} PD_THREAD_PARAMETER;

/**	DataSet definition	*/
typedef struct
{
	BOOL			boolean;		/**< =UINT8, 1 bit relevant (equal to zero = false, not equal to zero = true) */
	CHAR8			character;		/**< char, can be used also as UTF8 */
	UTF16			utf16;			/**< Unicode UTF-16 character */
	INT8			integer8;		/**< Signed integer, 8 bit */
	INT16			integer16;		/**< Signed integer, 16 bit */
	INT32			integer32;		/**< Signed integer, 32 bit */
	INT64			integer64;		/**< Signed integer, 64 bit */
	UINT8			uInteger8;		/**< Unsigned integer, 8 bit */
	UINT16			uInteger16;    /**< Unsigned integer, 16 bit */
	UINT32			uInteger32;    /**< Unsigned integer, 32 bit */
	UINT64			uInteger64;    /**< Unsigned integer, 64 bit */
	REAL32			real32;	    /**< Floating point real, 32 bit */
	REAL64			real64;	    /**< Floating point real, 64 bit */
//	CHAR8			string[16];    /**< Zero-terminated array of CHAR8, fixed size */
	TIMEDATE32		timeDate32;    /**< 32 bit UNIX time  */
	TIMEDATE48		timeDate48;    /**< 48 bit TCN time (32 bit UNIX time and 16 bit ticks)  */
	TIMEDATE64		timeDate64;    /**< 32 bit UNIX time + 32 bit miliseconds (== struct timeval) */
} DATASET1;

typedef struct
{
	DATASET1					dataset1[2];	/**< DATASET1 (73 octets)  */
	INT16						int16[64];		/**< int16 array */
} DATASET2;

/* Dataset Number definition */
typedef enum
{
    DATASET_NO_1		= 1,	/**< DATASET1 Number */
    DATASET_NO_2		= 2,	/**< DATASET2 Number */
} DATASET_NUMBER;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

/* Sub-network Id1 */
extern TRDP_APP_SESSION_T  appHandle;  				/*	Sub-network Id1 identifier to the library instance	*/
extern TRDP_SUB_T          subHandleNet1ComId1;		/*	Sub-network Id1 ComID1 identifier to the subscription	*/
extern TRDP_PUB_T          pubHandleNet1ComId1;		/*	Sub-network Id1 ComID2 identifier to the publication	*/
extern TRDP_SUB_T          subHandleNet1ComId2;		/*	Sub-network Id1 ComID1 identifier to the subscription	*/
extern TRDP_PUB_T          pubHandleNet1ComId2;		/*	Sub-network Id1 ComID2 identifier to the publication	*/
extern TRDP_ERR_T          err;
extern TRDP_PD_CONFIG_T    pdConfiguration;
extern TRDP_MEM_CONFIG_T   dynamicConfig;

extern INT32     rv;

/* Sub-network Id2 */
extern TRDP_APP_SESSION_T  appHandle2;  			/*	Sub-network Id2 identifier to the library instance	*/
extern TRDP_SUB_T          subHandleNet2ComId1;		/*	Sub-network Id2 ComID1 identifier to the subscription	*/
extern TRDP_PUB_T          pubHandleNet2ComId1;		/*	Sub-network Id2 ComID2 identifier to the publication	*/
extern TRDP_SUB_T          subHandleNet2ComId2;		/*	Sub-network Id2 ComID1 identifier to the subscription	*/
extern TRDP_PUB_T          pubHandleNet2ComId2;		/*	Sub-network Id2 ComID2 identifier to the publication	*/
extern TRDP_ERR_T          err2;
extern TRDP_PD_CONFIG_T    pdConfiguration2;       /* Sub-network Id2 PDconfiguration */
extern TRDP_MEM_CONFIG_T   dynamicConfig2;			/* Sub-network Id2 Structure describing memory */

extern INT32     rv2;

/* Configuration */
/* DATASET SIZE */
extern size_t dataSet1Size;						/* Dataset1 SIZE */
extern size_t dataSet2Size;						/* Dataset2 SIZE */
/* OffsetAddress */
extern INT32 dataSet1OffsetAddress;			        /* Dataset1 OffsetAddress */
extern INT32 dataset2OffsetAddress;			        /* Dataset2 OffsetAddress */

/* Changing TRDP parameter by Command Option */
/* Subscribe for Sub-network Id1 */
extern UINT32 PD_COMID1;
extern UINT32 PD_COMID2;
extern UINT32 PD_COMID1_TIMEOUT;
extern UINT32 PD_COMID2_TIMEOUT;
extern TRDP_IP_ADDR_T PD_COMID1_SRC_IP;
extern TRDP_IP_ADDR_T PD_COMID2_SRC_IP;
extern TRDP_IP_ADDR_T PD_COMID1_DST_IP;
extern TRDP_IP_ADDR_T PD_COMID2_DST_IP;
/* Subscribe for Sub-network Id2 */
extern TRDP_IP_ADDR_T PD_COMID1_SRC_IP2;
extern TRDP_IP_ADDR_T PD_COMID2_SRC_IP2;
/* Publish for Sub-network Id1 */
extern UINT32 PD_COMID1_CYCLE;
extern UINT32 PD_COMID2_CYCLE;
/* Ladder Topolpgy enable/disable */
extern BOOL ladderTopologyFlag;			/* Ladder Topology : TURE, Not Ladder Topology : FALSE */
/* OFFSET ADDRESS */
extern UINT16 OFFSET_ADDRESS1;				/* offsetAddress comId1 publish */
extern UINT16 OFFSET_ADDRESS2;				/* offsetAddress comId2 publish */
extern UINT16 OFFSET_ADDRESS3;				/* offsetAddress comId1 subscribe */
extern UINT16 OFFSET_ADDRESS4;				/* offsetAddress comId2 subscribe */
/* Marshalling enable/disable */
extern BOOL marshallingFlag;				/* Marshalling Enable : TURE, Marshalling Disable : FALSE */
/* publisher Application Cycle Time */
extern UINT32	publisherAppCycle;			/* Publisher Application cycle in us */

/* pointer to first command value for ladderApplication_multiPD */
extern PD_COMMAND_VALUE *pFirstPdCommandValue;		/* First PD Command Value */

/***********************************************************************************************************************
 * PROTOTYPES
 */
/**********************************************************************************************************************/
/** callback function PD receive
 *
 *  @param[in]		pRefCon			user supplied context pointer (offsetAddress)
 *  @param[in]		appHandle			application handle returned by tlc_opneSession
 *  @param[in]		pPDInfo			pointer to PDInformation
 *  @param[in]		pData			pointer to receive PD Data
 *  @param[in]		dataSize        receive PD Data Size
 *
 */
void tlp_recvPdDs (
    void *pRefCon,
    TRDP_APP_SESSION_T appHandle,
    const TRDP_PD_INFO_T *pPDInfo,
    UINT8 *pData,
    UINT32 dataSize);

/**********************************************************************************************************************/
/** Compare PD DataSet
 *
 */
INT8 comparePdDataset (
		const void *pSendPdDataset,
		const void *pReceivePdDataset,
		size_t sendPdDatasetSize,
		size_t receivePdDatasetSize);

/**********************************************************************************************************************/
/** Dump Memory
 *
 */
INT8 dumpMemory (
		const void *pDumpStartAddress,
		size_t dumpSize);

/**********************************************************************************************************************/
/** PDComLadder Thread
 *
 *
 */
VOS_THREAD_FUNC_T PDComLadder (
		void);

/**********************************************************************************************************************/
/** Decide Create Thread
 *
 *  @param[in]		argc
 *  @param[in]		argv
 *  @param[in]		pPdCommandValue			pointer to Command Value
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_ERR						error
 *  @retval         PD_APP_THREAD_ERR				error
 *
 */
PD_APP_ERR_TYPE decideCreatePdThread (
		int argc,
		char *argv[],
		PD_COMMAND_VALUE *pPdCommandValue);

/**********************************************************************************************************************/
/** Get PD Application Thread accessibility.
 *
 *  @retval         PD_APP_NO_ERR			no error
 *  @retval         PD_APP_MUTEX_ERR		mutex error
 */
PD_APP_ERR_TYPE  lockPdApplicationThread (
    void);

/**********************************************************************************************************************/
/** Release PD Application Thread accessibility.
 *
 *  @retval         PD_APP_NO_ERR			no error
 *  @retval         PD_APP_MUTEX_ERR		mutex error
 *
  */
PD_APP_ERR_TYPE  unlockPdApplicationThread (
    void);

/**********************************************************************************************************************/
/** Create PD Thread
 *
 *  @param[in]		pPdThreadParameter			pointer to PDThread Parameter
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_THREAD_ERR				Thread error
 *
 */
PD_APP_ERR_TYPE createPdThread (
		PD_THREAD_PARAMETER *pPdThreadParameter);

/**********************************************************************************************************************/
/** main thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
PD_APP_ERR_TYPE pdCommand_main_proc(
		void);

/**********************************************************************************************************************/
/** analyze command
 *
 *  @param[in]		void
 *
 *  @retval         PD_APP_NO_ERR			no error
 *  @retval         PD_APP_ERR				error
 */
PD_APP_ERR_TYPE analyzePdCommand(
		int argc,
		char *argv[],
		PD_COMMAND_VALUE *pPdCommandValue);

/**********************************************************************************************************************/
/** TRDP PD initialization
 *
 *  @retval         PD_APP_NO_ERR		no error
 *  @retval         PD_APP_ERR			some error
 */
PD_APP_ERR_TYPE trdp_pdInitialize (
		void);

/**********************************************************************************************************************/
/** PD Application main
 *
 *  @param[in]		pPDThreadParameter			pointer to PDThread Parameter
 *
 *  @retval         PD_APP_NO_ERR		no error
 *  @retval         PD_APP_ERR			some error
 */
PD_APP_ERR_TYPE PDApplication (
		PD_THREAD_PARAMETER *pPdThreadParameter);

/**********************************************************************************************************************/
/** Append an pdCommandValue at end of List
 *
 *  @param[in]      ppHead          pointer to pointer to head of List
 *  @param[in]      pNew            pointer to pdCommandValue to append
 *
 *  @retval         PD_APP_NO_ERR			no error
 *  @retval         PD_APP_ERR				error
 */
PD_APP_ERR_TYPE appendPdComamndValueList(
		PD_COMMAND_VALUE    * *ppHeadPdCommandValue,
		PD_COMMAND_VALUE    *pNewPdCommandValue);

/**********************************************************************************************************************/
/** Return the PdCommandValue with same comId and IP addresses
 *
 *  @param[in]      pHeadPdCommandValue	pointer to head of queue
 *  @param[in]      pNewPdCommandValue		Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *
 *  @retval         != NULL         		pointer to PdCommandValue
 *  @retval         NULL            		No PD PdCommandValue found
 */
PD_APP_ERR_TYPE serachPdCommandValueToCommand (
		PD_COMMAND_VALUE	*pHeadPdCommandValue,
		PD_COMMAND_VALUE	*pNewPdCommandValue);

/**********************************************************************************************************************/
/** Return the PdCommandValue with same comId and IP addresses
 *
 *  @param[in]		pNewPdCommandValue		Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *  @param[out]		pMatchSubnet            	pointer to match subnet number
 *
 *  @retval         != NULL         		pointer to PdCommandValue
 *  @retval         NULL            		No PD PdCommandValue found
 */
PD_COMMAND_VALUE *serachPdCommandValueToAddr (
		TRDP_ADDRESSES_T    *addr,
		UINT32	*pMatchSubnet);

/**********************************************************************************************************************/
/** Return the PdCommandValue with same comId and IP addresses
 *
 *  @param[in]      pHeadPdCommandValue	pointer to head of queue
 *
 *  @retval         != NULL         		pointer to PdCommandValue
 *  @retval         NULL            		No PD PdCommandValue found
 */
PD_APP_ERR_TYPE printPdCommandValue (
		PD_COMMAND_VALUE	*pHeadPdCommandValue);

/**********************************************************************************************************************/
/** Display PD Statistics
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_PARAM_ERR					parameter	error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE printPdStatistics (
		TRDP_APP_SESSION_T  appHandle);

/**********************************************************************************************************************/
/** Display PD Subscriber Receive Count / Receive Timeout Count
 *
 *  @param[in]      pHeadPdCommandValue	pointer to head of queue
 *  @param[in]      addr						Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_PARAM_ERR					parameter	error
 *
 */
PD_APP_ERR_TYPE printPdSubscribeResult (
		PD_COMMAND_VALUE	*pHeadPdCommandValue);

/**********************************************************************************************************************/
/** Create PD DataSet1
 *
 *  @param[in]		firstCreateFlag			First : TRUE, Not First : FALSE
 *  @param[out]		pPdDataSet1				pointer to Created PD DATASET1
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *
 */
PD_APP_ERR_TYPE createPdDataSet1 (
		BOOL firstCreateFlag,
		DATASET1 *pPdDataSet1);

/**********************************************************************************************************************/
/** Create PD DataSet2
 *
 *  @param[in]		fristCreateFlag			First : TRUE, Not First : FALSE
 *  @param[out]		pPdDataSet2				pointer to Created PD DATASET2
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *
 */
PD_APP_ERR_TYPE createPdDataSet2 (
		BOOL firstCreateFlag,
		DATASET2 *pPdDataSet2);

// Convert an IP address to string
char * miscIpToString(
		int ipAdd,
		char *strTmp);

#endif	/* TRDP_OPTION_LADDER */

#ifdef __cplusplus
}
#endif

#endif /* TRDP_LADDER_APP_H_ */
