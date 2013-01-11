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

/***********************************************************************************************************************
 * DEFINES
 */

#define SUBNET2_NETMASK								0x00002000			/* The netmask for Subnet2 */
#define PDCOM_LADDER_THREAD_DELAY_TIME				10000				/* PDComLadder Thread starting Judge Cycle in us */
#define PDCOM_MULTICAST_GROUPING_DELAY_TIME		    10000000			/* PDComLadder Thread starting Wait Time in us */
#define PUT_DATASET_BUFFER_SIZE						1					/* putDataSet Backup Buffer */

/* Some sample comId definitions	*/
#define PD_DATA_SIZE_MAX			300					/* PD DATA MAX SIZE */
#define LADDER_APP_CYCLE			500000				/* Ladder Application data put cycle in us */

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

/***********************************************************************************************************************
 * TYPEDEFS
 */

/**	DataSet definition	*/
typedef struct
{
	CHAR8						character;		/**< char, can be used also as UTF8 */
	UTF16						utf16;			/**< Unicode UTF-16 character */
	INT8						integer8;		/**< Signed integer, 8 bit */
	INT16						integer16;		/**< Signed integer, 16 bit */
	INT32						integer32;		/**< Signed integer, 32 bit */
	INT64						integer64;		/**< Signed integer, 64 bit */
	UINT8						uInteger8;		/**< Unsigned integer, 8 bit */
	UINT16						uInteger16;	    /**< Unsigned integer, 16 bit */
	UINT32						uInteger32;	    /**< Unsigned integer, 32 bit */
	UINT64						uInteger64;	    /**< Unsigned integer, 64 bit */
	REAL32						real32;		    /**< Floating point real, 32 bit */
	CHAR8						string[16];	    /**< Zero-terminated array of CHAR8, fixed size */
	TIMEDATE32					timeDate32;	    /**< 32 bit UNIX time  */
	TIMEDATE48					timeDate48;	    /**< 48 bit TCN time (32 bit UNIX time and 16 bit ticks)  */
	TIMEDATE64					timeDate64;	    /**< 32 bit UNIX time + 32 bit miliseconds  */
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

/***********************************************************************************************************************
 * PROTOTYPES
 */
/**********************************************************************************************************************/
/** callback function PD receive
 *
 *  @param[in]		pRefCon			user supplied context pointer (offsetAddress)
 *  @param[in]		pPDInfo			pointer to PDInformation
 *  @param[in]		pData			pointer to receive PD Data
 *  @param[in]		dataSize        receive PD Data Size
 *
 */
void tlp_recvPdDs (
    void *pRefCon,
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

#endif	/* TRDP_OPTION_LADDER */

#ifdef __cplusplus
}
#endif

#endif /* TRDP_LADDER_APP_H_ */
