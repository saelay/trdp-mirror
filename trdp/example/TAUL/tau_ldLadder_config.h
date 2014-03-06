/**********************************************************************************************************************/
/**
 * @file            tau_ldLadder_config.h
 *
 * @brief           Global Variables for TRDP Ladder Topology Support
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, Toshiba Corporation
 *
 * @remarks This source code corresponds to TRDP_LADDER open source software.
 *          This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Toshiba Corporation, Japan, 2013. All rights reserved.
 *
 * $Id$
 *
 */

#ifndef TAU_LDLADDER_CONFIG_H_
#define TAU_LDLADDER_CONFIG_H_

#ifdef TRDP_OPTION_LADDER
/*******************************************************************************
 * INCLUDES
 */


#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
 *   Globals
 */
/* TRDP Config *****************************************************/
#ifdef XML_CONFIG_ENABLE
/* XML Config File : Enable */
TRDP_XML_DOC_HANDLE_T	xmlConfigHandle;			/* XML Config Handle */
TAU_LD_CONFIG_T			*pTaulConfig;				/* TAUL Config */
TAU_LD_CONFIG_T			taulConfig = {0};			/* TAUL Config */

/*  General parameters from xml configuration file */
TRDP_MEM_CONFIG_T			memoryConfig;
TRDP_DBG_CONFIG_T			debugConfig;
UINT32						numComPar = 0;
TRDP_COM_PAR_T			    *pComPar = NULL;
UINT32						numIfConfig = 0;
TRDP_IF_CONFIG_T			*pIfConfig = NULL;

/*  Dataset configuration from xml configuration file */
UINT32                  numComId = 0;
TRDP_COMID_DSID_MAP_T   *pComIdDsIdMap = NULL;
UINT32                  numDataset = 0;
apTRDP_DATASET_T        apDataset = NULL;

TRDP_APP_SESSION_T		appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
TRDP_APP_SESSION_T		appHandle2;				    /*	Sub-network Id2 identifier to the library instance	*/

/*  Session configurations  */
typedef struct
{
    TRDP_APP_SESSION_T      sessionHandle;
    TRDP_PD_CONFIG_T        pdConfig;
    TRDP_MD_CONFIG_T        mdConfig;
    TRDP_PROCESS_CONFIG_T   processConfig;
} sSESSION_CONFIG_T;
/*  Array of session configurations - one for each interface, only numIfConfig elements actually used  */
sSESSION_CONFIG_T          arraySessionConfig[MAX_SESSIONS];
/* Array of Exchange Parameter */
TRDP_EXCHG_PAR_T				*arrayExchgPar[LADDER_IF_NUMBER] = {0};
/*  Exchange Parameter from xml configuration file */
UINT32             			 numExchgPar = 0;				/* Number of Exchange Parameter */
//TRDP_EXCHG_PAR_T    			*pExchgPar = NULL;			/* Pointer to Exchange Parameter */

#else
/* XML Config File : disable */
/* The User needs to edit TRDP Config Parameter. */
TRDP_XML_DOC_HANDLE_T	xmlConfigHandle;			/* XML Config Handle */
TAU_LD_CONFIG_T			*pTaulConfig;				/* Pointer to TAUL Config */
TAU_LD_CONFIG_T			taulConfig = {0};			/* TAUL Config */

/*  General parameters from xml configuration file */
/* Memory Config ****************************************************/
/* size of static or allocated memory (Byte) : 1MB */
#define					TRDP_MEMORY_SIZE		10485760

TRDP_MEM_CONFIG_T			memoryConfig = {
							NULL,						/**< pointer to static or allocated memory */
							TRDP_MEMORY_SIZE,			/**< size of static or allocated memory (Byte) */
							{0, 0, 0, 0, 0, 0, 0, 10, 0, 2, 0, 0, 4, 0, 0}};
												/**< memory block structure
													Block Size Byte
											   		VOS_MEM_BLOCKSIZES  {48, 72, 128, 180, 256, 512, 1024, 1480, 2048, 4096, 11520, 16384, 32768, 65536, 131072} */

/* Debug Config *****************************************************/
/* Maximal Debug file size (Byte) : 1MB */
#define					TRDP_DEBUG_FILE_SIZE		10485760
/* Debug file name and path : ./trdp.log  */
//#define					TRDP_DEBUG_FILE_PATH_NAME	"trdp.log"
#define					TRDP_DEBUG_FILE_PATH_NAME	""

TRDP_DBG_CONFIG_T			debugConfig = {
							(TRDP_DBG_ERR | TRDP_DBG_TIME | TRDP_DBG_LOC | TRDP_DBG_CAT),		/**< Debug printout options for application use
							 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 		parameter format TRDP_DBG_OPTION_T */
							TRDP_DEBUG_FILE_SIZE,												/**< Maximal file size (Byte) */
							TRDP_DEBUG_FILE_PATH_NAME};											/**< Debug file name and path */

/* Communication Parameter ******************************************/
/* Number of Communication Parameter : 3 */
#define					NUM_COM_PAR	3
UINT32						numComPar = NUM_COM_PAR;

/* PD Parameter */
/* Communication Parameter Id 1 for PD */
#define					COMMUNICATION_PAR_ID_1_FOR_PD	1
/* MD Parameter */
/* Communication Parameter Id 2 for MD */
#define					COMMUNICATION_PAR_ID_2_FOR_MD	2
/* User Setting Parameter */
/* Communication Parameter Id 4 */
#define					COMMUNICATION_PAR_ID_4	4
/* QoS of Communication Parameter Id 4  */
#define					COMMUNICATION_PAR_ID_4_QOS	4
/* TTL of Communication Parameter Id 4  */
#define					COMMUNICATION_PAR_ID_4_TTL	32

/* Array Communication Parameter */
TRDP_COM_PAR_T			arrayComParConfig[NUM_COM_PAR] = {
//							{1, {5, 64}},		/* PD Parameter: {communication parameter identifier:1, {QoS:5 , TTL:64}} */
//							{2, {3, 64}},		/* MD Parameter: 	{communication parameter identifier:2, {QoS:3 , TTL:64}} */
//							{4, {4, 32}}};	    /* User Add Parameter: {communication parameter identifier:4, {QoS:4 , TTL:32}} */
							{COMMUNICATION_PAR_ID_1_FOR_PD, {TRDP_PD_DEFAULT_QOS, TRDP_PD_DEFAULT_TTL}},				/* PD Parameter: {communication parameter identifier:1, {QoS:5 , TTL:64}} */
							{COMMUNICATION_PAR_ID_2_FOR_MD, {TRDP_MD_DEFAULT_QOS, TRDP_MD_DEFAULT_TTL}},				/* MD Parameter: 	{communication parameter identifier:2, {QoS:3 , TTL:64}} */
							{COMMUNICATION_PAR_ID_4, {COMMUNICATION_PAR_ID_4_QOS, COMMUNICATION_PAR_ID_4_TTL}}};	/* User Add Parameter: {communication parameter identifier:4, {QoS:4 , TTL:32}} */
TRDP_COM_PAR_T			*pComPar = NULL;

/* IF Config ********************************************************/
/* Number of IF Config : 2 for Subnet1, Subnet2 */
#define					NUM_IF_CONFIG		2
UINT32						numIfConfig = NUM_IF_CONFIG;
/* Length of IP Address  (xxx.xxx.xxx.xxx) */
#define					TRDP_CHAR_IP_ADDR_LEN	16
/* Character String of IP Address */
typedef CHAR8 TRDP_CHAR_IP_ADDR_T[TRDP_CHAR_IP_ADDR_LEN];
/* Subnet1 Parameter */
/* IF Name of Subnet1 : eth0 */
#define					IF_NAME_SUBNET_1	"eth0"
/* Network ID of Subnet1 : 1 */
#define					NETWORK_ID_SUBNET_1	1
/* Host IP Address of Subnet1 : 10.0.1.19 */
#define					HOST_IP_ADDRESS_SUBNET_1	"10.0.1.19"
/* Leader IP Address of Subnet1 : 0.0.0.0 */
#define					LEADER_IP_ADDRESS_SUBNET_1	"0.0.0.0"
/* Subnet2 Parameter */
/* IF Name of Subnet2 : eth1 */
#define					IF_NAME_SUBNET_2	"eth1"
/* Network ID of Subnet2 : 2 */
#define					NETWORK_ID_SUBNET_2	2
/* IP Address of Subnet2 : 10.0.33.19 */
#define					HOST_IP_ADDRESS_SUBNET_2	"10.0.33.19"
/* Leader IP Address of Subnet2 : 0.0.0.0 */
#define					LEADER_IP_ADDRESS_SUBNET_2	"0.0.0.0"

/*Array IF Config */
typedef struct INTERNAL_CONFIG_IF_CONFIG
{
	TRDP_LABEL_T			ifName;			/**< interface name */
	UINT8					networkId;			/**< used network on the device (1..4) */
	TRDP_CHAR_IP_ADDR_T  dottedHostIp;		/**< host IP address (xxx.xxx.xxx.xxx) */
	TRDP_CHAR_IP_ADDR_T	dottedLeaderIp;	/**< Leader IP address dependant on redundancy concept (xxx.xxx.xxx.xxx) */
} INTERNAL_CONFIG_IF_CONFIG_T;

INTERNAL_CONFIG_IF_CONFIG_T			arrayInternalIfConfig[NUM_IF_CONFIG] = {
//										{"eth0", 1, "10.0.1.19", "0.0.0.0"},		/* Subnet1: {interface name: eth0, used network on the device: 1 , host IP address: 10.0.1.19,  Leader IP address dependant on redundancy concept: 0.0.0.0} */
//										{"eth1", 2, "10.0.33.19", "0.0.0.0"}};		/* Subnet2: {interface name: eth1, used network on the device: 2 , host IP address: 10.0.33.19, Leader IP address dependant on redundancy concept: 0.0.0.0} */
										{"IF_NAME_SUBNET_1", NETWORK_ID_SUBNET_1, HOST_IP_ADDRESS_SUBNET_1, LEADER_IP_ADDRESS_SUBNET_1},		/* Subnet1: {interface name: eth0, used network on the device: 1 , host IP address: 10.0.1.19,  Leader IP address dependant on redundancy concept: 0.0.0.0} */
										{"IF_NAME_SUBNET_2", NETWORK_ID_SUBNET_2, HOST_IP_ADDRESS_SUBNET_2, LEADER_IP_ADDRESS_SUBNET_2}};		/* Subnet2: {interface name: eth1, used network on the device: 2 , host IP address: 10.0.33.19, Leader IP address dependant on redundancy concept: 0.0.0.0} */
TRDP_IF_CONFIG_T			*pIfConfig = NULL;

/* Dataset Config ***************************************************/
/* Number of combination of comId and DatasetId  : 4 */
#define					NUM_COMID 8
UINT32						numComId = NUM_COMID;

/* comId - DatasetId Map */
/* Subnet1 Mr1 */
#define					SUBNET_1_MR_1_COMID_200003			200003		/* Subnet1 Mr1 comId 	: 200003 */
#define					SUBNET_1_MR_1_DATASETID_2003		2003		/* Subnet1 Mr1 DatasetId	: 2003 */
/* Subnet1 Mp1 */
#define					SUBNET_1_MP_1_COMID_210003			210003		/* Subnet1 Mp1 comId		: 210003 */
#define					SUBNET_1_MP_1_DATASETID_2103		2103		/* Subnet1 Mp1 DatasetId	: 2003 */
/* Subnet1 Pd1 */
#define					SUBNET_1_PD_1_COMID_10001			10001		/* Subnet1 Pd1 comId 	: 10001 */
#define					SUBNET_1_PD_1_DATASETID_1001		1001		/* Subnet1 Pd1 DatasetId	: 1001 */
/* Subnet1 Pd2 */
#define					SUBNET_1_PD_2_COMID_10002			10002		/* Subnet1 Pd2 comId 	: 10002 */
#define					SUBNET_1_PD_2_DATASETID_1002		1002		/* Subnet1 Pd2 DatasetId	: 1002 */
/* Subnet2 Mr1 */
#define					SUBNET_2_MR_1_COMID_200003			200003		/* Subnet2 Mr1 comId 	: 200003 */
#define					SUBNET_2_MR_1_DATASETID_2003		2003		/* Subnet2 Mr1 DatasetId	: 2003 */
/* Subnet2 Mp1 */
#define					SUBNET_2_MP_1_COMID_210003			210003		/* Subnet2 Mp1 comId		: 210003 */
#define					SUBNET_2_MP_1_DATASETID_2103		2103		/* Subnet2 Mp1 DatasetId	: 2003 */
/* Subnet2 Pd1 */
#define					SUBNET_2_PD_1_COMID_10001			10001		/* Subnet2 Pd1 comId 	: 10001 */
#define					SUBNET_2_PD_1_DATASETID_1001		1001		/* Subnet2 Pd1 DatasetId	: 1001 */
/* Subnet2 Pd2 */
#define					SUBNET_2_PD_2_COMID_10002			10002		/* Subnet2 Pd2 comId 	: 10002 */
#define					SUBNET_2_PD_2_DATASETID_1002		1002		/* Subnet2 Pd2 DatasetId	: 1002 */

/* Array comIdDatasetId Map Config */
TRDP_COMID_DSID_MAP_T	arrayComIdDsIdMapConfig[NUM_COMID] = {
//							{200003, 2003},		/* Subnet1 Mr1: {comId: 200003, corresponding datasetId: 2003} */
//							{210003, 2003},		/* Subnet1 Mp1: {comId: 210003, corresponding datasetId: 2003} */
//							{200003, 2003},		/* Subnet2 Mr1: {comId: 200003, corresponding datasetId: 2003} */
//							{210003, 2003}};		/* Subnet2 Mp1: {comId: 210003, corresponding datasetId: 2003} */
							{SUBNET_1_MR_1_COMID_200003,	SUBNET_1_MR_1_DATASETID_2003},		/* Subnet1 Mr1: {comId: 200003,	corresponding datasetId: 2003} */
							{SUBNET_1_MP_1_COMID_210003,	SUBNET_1_MP_1_DATASETID_2103},		/* Subnet1 Mp1: {comId: 210003,	corresponding datasetId: 2103} */
							{SUBNET_1_PD_1_COMID_10001,		SUBNET_1_PD_1_DATASETID_1001},		/* Subnet1 Pd1: {comId: 10001,	corresponding datasetId: 1001} */
							{SUBNET_1_PD_2_COMID_10002,		SUBNET_1_PD_2_DATASETID_1002},		/* Subnet1 Pd2: {comId: 10002,	corresponding datasetId: 1002} */
							{SUBNET_2_MR_1_COMID_200003,	SUBNET_2_MR_1_DATASETID_2003},		/* Subnet2 Mr1: {comId: 200003,	corresponding datasetId: 2003} */
							{SUBNET_2_MP_1_COMID_210003,	SUBNET_2_MP_1_DATASETID_2103},		/* Subnet2 Mp1: {comId: 210003,	corresponding datasetId: 203} */
							{SUBNET_2_PD_1_COMID_10001,		SUBNET_2_PD_1_DATASETID_1001},		/* Subnet2 Pd1: {comId: 10001,	corresponding datasetId: 1001} */
							{SUBNET_2_PD_2_COMID_10002,		SUBNET_2_PD_2_DATASETID_1002}};		/* Subnet2 Pd2: {comId: 10002,	corresponding datasetId: 1002} */
TRDP_COMID_DSID_MAP_T	*pComIdDsIdMap = NULL;

/* Number of DatasetId  : 4 */
#define					NUM_DATASET 4
UINT32						numDataset = NUM_DATASET;

/* Number of Element in DatasetId2003 : 2 */
#define					NUM_ELEMENT_DATASETID_2003	2
/* arrayDatasetConfig[0] Element Type : (DatasetId:2003) */
TRDP_DATASET_ELEMENT_T	arrayDatasetConfig1Element[NUM_ELEMENT_DATASETID_2003] =
							{
								{	/* pElement[0] */
									TRDP_INT8,		/* Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 : Signed integer, 8 bit*/
									1,				/* Number of items or TDRP_VAR_SIZE (0) : 1 */
									NULL			/* Used internally for marshalling speed-up */
								},
								{	/* pElement[1] */
									TRDP_INT16,	/* Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 : Signed integer, 16 bit*/
									2,				/* Number of items or TDRP_VAR_SIZE (0) : 2 */
									NULL			/* Used internally for marshalling speed-up */
								}
							};
/* Number of Element in DatasetId2103 : 2 */
#define					NUM_ELEMENT_DATASETID_2103	2
/* arrayDatasetConfig[1] Element Type : (DatasetId:2103) */
TRDP_DATASET_ELEMENT_T	arrayDatasetConfig2Element[NUM_ELEMENT_DATASETID_2103] =
							{
								{	/* pElement[0] */
									SUBNET_1_MR_1_DATASETID_2003,		/* Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 : datasetId 2003 */
									2,				/* Number of items or TDRP_VAR_SIZE (0) : 1 */
									NULL			/* Used internally for marshalling speed-up */
								},
								{	/* pElement[1] */
									TRDP_INT16,	/* Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 : Signed integer, 16 bit*/
									1,				/* Number of items or TDRP_VAR_SIZE (0) : 1 */
									NULL			/* Used internally for marshalling speed-up */
								}
							};
/* Number of Element in DatasetId1001 : 3 */
#define					NUM_ELEMENT_DATASETID_1001	3
/* arrayDatasetConfig[2] Element Type : (DatasetId:1001) */
TRDP_DATASET_ELEMENT_T	arrayDatasetConfig3Element[NUM_ELEMENT_DATASETID_1001] =
							{
								{	/* pElement[0] */
									TRDP_INT32,		/* Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 : Signed integer, 32 bit*/
									1,				/* Number of items or TDRP_VAR_SIZE (0) : 1 */
									NULL			/* Used internally for marshalling speed-up */
								},
								{	/* pElement[1] */
									TRDP_UINT32,	/* Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 : UnSigned integer, 32 bit*/
									2,				/* Number of items or TDRP_VAR_SIZE (0) : 2 */
									NULL			/* Used internally for marshalling speed-up */
								},
								{	/* pElement[2] */
									TRDP_UINT16,	/* Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 : UnSigned integer, 16 bit*/
									3,				/* Number of items or TDRP_VAR_SIZE (0) : 3 */
									NULL			/* Used internally for marshalling speed-up */
								}
							};
/* Number of Element in DatasetId1002 : 3 */
#define					NUM_ELEMENT_DATASETID_1002	3
/* arrayDatasetConfig[3] Element Type : (DatasetId:1002) */
TRDP_DATASET_ELEMENT_T	arrayDatasetConfig4Element[NUM_ELEMENT_DATASETID_1002] =
							{
								{	/* pElement[0] */
									TRDP_INT32,		/* Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 : Signed integer, 32 bit*/
									1,				/* Number of items or TDRP_VAR_SIZE (0) : 1 */
									NULL			/* Used internally for marshalling speed-up */
								},
								{	/* pElement[1] */
									TRDP_UINT32,	/* Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 : UnSigned integer, 32 bit*/
									2,				/* Number of items or TDRP_VAR_SIZE (0) : 2 */
									NULL			/* Used internally for marshalling speed-up */
								},
								{	/* pElement[2] */
									TRDP_INT16,	/* Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000 : Signed integer, 16 bit*/
									3,				/* Number of items or TDRP_VAR_SIZE (0) : 3 */
									NULL			/* Used internally for marshalling speed-up */
								}
							};
/* Array Dataset of Internal Config */
INTERNAL_CONFIG_DATASET_T	arrayInternalDatasetConfig[NUM_DATASET] =
								{
									{	/* arraryInternalDatasetConfig[0] */
										SUBNET_1_MR_1_DATASETID_2003,			/* dataset identifier > 1000 : 2003 */
										0,											/* Reserved for future use, must be zero : 0 */
										NUM_ELEMENT_DATASETID_2003,				/* Number of elements : 2 */
										arrayDatasetConfig1Element				/* pElement[0] : arrayDatasetConfig1Element (DatasetId 2003)*/
									},
									{	/* arraryInternalDatasetConfig[1] */
										SUBNET_1_MP_1_DATASETID_2103,			/* dataset identifier > 1000 : 2103 */
										0,											/* Reserved for future use, must be zero : 0 */
										NUM_ELEMENT_DATASETID_2103,				/* Number of elements : 2 */
										arrayDatasetConfig2Element				/* pElement[0] : arrayDatasetConfig2Element (DatasetId 2103)*/
									},
									{	/* arraryInternalDatasetConfig[2] */
										SUBNET_1_PD_1_DATASETID_1001,			/* dataset identifier > 1000 : 1001 */
										0,											/* Reserved for future use, must be zero : 0 */
										NUM_ELEMENT_DATASETID_1001,				/* Number of elements : 3 */
										arrayDatasetConfig3Element				/* pElement[0] : arrayDatasetConfig3Element (DatasetId 1001)*/
									},

									{	/* arraryInternalDatasetConfig[3] */
										SUBNET_1_PD_2_DATASETID_1002,			/* dataset identifier > 1000 : 1002 */
										0,											/* Reserved for future use, must be zero : 0 */
										NUM_ELEMENT_DATASETID_1002,				/* Number of elements : 3 */
										arrayDatasetConfig4Element				/* pElement[0] : arrayDatasetConfig4Element (DatasetId 1002)*/
									}
								};
apTRDP_DATASET_T			apDataset = NULL;

/* IF Config Parameter **********************************************/
/* Subnet1 Parameter */
/* PD timeout in us of Subnet1 : 1sec */
#define					PD_TIMEOUT_SUBNET_1		1000000
/* Host Name of Subnet1 : ED1-1 */
#define					HOST_NAME_SUBNET_1	"ED1-1"
/* Leader Name of Subnet1 : */
#define					LEADER_NAME_SUBNET_1	""
/* TRDP main process cycle time in us of Subnet1 : 10ms */
#define					MAIN_PROCESS_CYCLE_TIME_SUBNET_1	10000
/* TRDP main process priority (0-255, 0=default, 255=highest) : 80 */
#define					MAIN_PROCESS_PRIORITY_SUBNET_1	80
/* Subnet2 Parameter */
/* PD timeout in us of Subnet2 : 1sec */
#define					PD_TIMEOUT_SUBNET_2		1000000
/* Host Name of Subnet2 : ED1-2 */
#define					HOST_NAME_SUBNET_2	"ED1-2"
/* Leader Name of Subnet2 : */
#define					LEADER_NAME_SUBNET_2	""
/* TRDP main process cycle time in us of Subnet2 : 10ms */
#define					MAIN_PROCESS_CYCLE_TIME_SUBNET_2	10000
/* TRDP main process priority (0-255, 0=default, 255=highest) of Subnet2 : 80 */
#define					MAIN_PROCESS_PRIORITY_SUBNET_2	80

/*  Session configurations  */
typedef struct
{
    TRDP_APP_SESSION_T      sessionHandle;
    TRDP_PD_CONFIG_T        pdConfig;
    TRDP_MD_CONFIG_T        mdConfig;
    TRDP_PROCESS_CONFIG_T   processConfig;
} sSESSION_CONFIG_T;
/*  Array of session configurations - one for each interface, only numIfConfig elements actually used  */
sSESSION_CONFIG_T          arraySessionConfig[NUM_IF_CONFIG] =
{
	{	/* arraySessionConfig[0] : Subnet1 */
		/* Session Handle */
		NULL,
		{	/* PD Config */
			NULL,											/**< Pointer to PD callback function			    				*/
			NULL, 											/**< Pointer to user context for call back		    				*/
			{TRDP_PD_DEFAULT_QOS, TRDP_PD_DEFAULT_TTL},		/**< Default send parameters (QoS:5, TTL:64)						*/
			(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK),	/**< Default flags for PD packets (Marshalling:ON, Callback:ON		*/
			TRDP_PD_DEFAULT_TIMEOUT,						/**< Default timeout in us (100ms)				    				*/
			TRDP_TO_SET_TO_ZERO,							/**< Default timeout behaviour					    				*/
			TRDP_PD_UDP_PORT								/**< Port to be used for PD communication (20548)   				*/
		},
		{	/* MD Config */
			NULL,																	/**< Pointer to MD callback function											*/
			NULL,																	/**< Pointer to user context for call back									*/
			{TRDP_MD_DEFAULT_QOS, TRDP_MD_DEFAULT_QOS},						/**< Default send parameters (QoS:3, TTL:64)									*/
			(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP),	/**< Default flags for MD packets (Marshalling:ON, Callback:ON TCP:ON		*/
			TRDP_MD_DEFAULT_REPLY_TIMEOUT,										/**< Default reply timeout in us (5sec)										*/
			TRDP_MD_DEFAULT_CONFIRM_TIMEOUT,									/**< Default confirmation timeout in us (1sec)								*/
			TRDP_MD_DEFAULT_CONNECTION_TIMEOUT,								/**< Default connection timeout in us (60sec)									*/
			TRDP_MD_DEFAULT_SENDING_TIMEOUT,									/**< Default sending timeout in us (5sec)										*/
			TRDP_MD_UDP_PORT,														/**< Port to be used for UDP MD communication (20550)						*/
			TRDP_MD_TCP_PORT,														/**< Port to be used for TCP MD communication (20550)						*/
			TRDP_MD_MAX_NUM_SESSIONS												/**< Maximal number of replier sessions (1000)								*/
		},
		{	/* Process Config */
			HOST_NAME_SUBNET_1,							/* Host name : ED1-1 */
			LEADER_NAME_SUBNET_1,						/* Leader name dependant on redundancy concept : Nothing */
			TRDP_PROCESS_DEFAULT_CYCLE_TIME,			/* TRDP main process cycle time in us : 10ms */
			TRDP_PROCESS_DEFAULT_PRIORITY,				/* TRDP main process cycle time (0-255, 0=default, 255=highest) : 64 */
			TRDP_OPTION_TRAFFIC_SHAPING					/* TRDP options (Traffic Shaping: ON) */
		}
	},
	{	/* arraySessionConfig[1] : Subnet2 */
		/* Session Handle */
		NULL,
		{	/* PD Config */
			NULL,													/**< Pointer to PD callback function									*/
			NULL, 													/**< Pointer to user context for call back							*/
			{TRDP_PD_DEFAULT_QOS, TRDP_PD_DEFAULT_TTL},		/**< Default send parameters (QoS:5, TTL:64)							*/
			(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK),		/**< Default flags for PD packets (Marshalling:ON, Callback:ON		*/
			TRDP_PD_DEFAULT_TIMEOUT,								/**< Default timeout in us (100ms)										*/
			TRDP_TO_SET_TO_ZERO,									/**< Default timeout behaviour											*/
			TRDP_PD_UDP_PORT										/**< Port to be used for PD communication (20548)					*/
		},
		{	/* MD Config */
			NULL,																	/**< Pointer to MD callback function											*/
			NULL,																	/**< Pointer to user context for call back									*/
			{TRDP_MD_DEFAULT_QOS, TRDP_MD_DEFAULT_QOS},						/**< Default send parameters (QoS:3, TTL:64)									*/
			(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP),	/**< Default flags for MD packets (Marshalling:ON, Callback:ON TCP:ON		*/
			TRDP_MD_DEFAULT_REPLY_TIMEOUT,										/**< Default reply timeout in us (5sec)										*/
			TRDP_MD_DEFAULT_CONFIRM_TIMEOUT,									/**< Default confirmation timeout in us (1sec)								*/
			TRDP_MD_DEFAULT_CONNECTION_TIMEOUT,								/**< Default connection timeout in us (60sec)									*/
			TRDP_MD_DEFAULT_SENDING_TIMEOUT,									/**< Default sending timeout in us (5sec)										*/
			TRDP_MD_UDP_PORT,														/**< Port to be used for UDP MD communication (20550)						*/
			TRDP_MD_TCP_PORT,														/**< Port to be used for TCP MD communication (20550)						*/
			TRDP_MD_MAX_NUM_SESSIONS												/**< Maximal number of replier sessions (1000)								*/
		},
		{	/* Process Config */
			HOST_NAME_SUBNET_2,							/* Host name : ED1-2 */
			LEADER_NAME_SUBNET_2,						/* Leader name dependant on redundancy concept : Nothing */
			TRDP_PROCESS_DEFAULT_CYCLE_TIME,			/* TRDP main process cycle time in us : 10ms */
			TRDP_PROCESS_DEFAULT_PRIORITY,				/* TRDP main process cycle time (0-255, 0=default, 255=highest) : 64 */
			TRDP_OPTION_TRAFFIC_SHAPING					/* TRDP options (Traffic Shaping: ON) */
		}
	}
};

/* Number of Exchange Parameter : 2 */
#define					NUM_EXCHG_PAR 4
UINT32						numExchgPar = NUM_EXCHG_PAR;
//TRDP_EXCHG_PAR_T    			*pExchgPar = NULL;			/* Pointer to Exchange Parameter */

/* MD Parameter *****/
/* Subnet1 Exchange Parameter1 */
/* MD Confirm timeout in us of Subnet1 ExchgPar1 : 1sec */
#define					CONFIRM_TIMEOUT_SUBNET_1_EXCHG_PAR_1		1000000
/* MD Reply timeout in us of Subnet1 ExchgPar1 : 5sec */
#define					REPLY_TIMEOUT_SUBNET_1_EXCHG_PAR_1		5000000
/* MD Parameter of Sunet1 Exchange Parameter1 */
TRDP_MD_PAR_T				subnet1arrayExchgPar1MdPar =
							{
								CONFIRM_TIMEOUT_SUBNET_1_EXCHG_PAR_1,									/**< Confirmation timeout in us : 1sec  */
								REPLY_TIMEOUT_SUBNET_1_EXCHG_PAR_1,									/**< Reply timeout in us : 5sec         */
								(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP)		/**< Flags for MD packets               */
							};
/* Subnet1 Exchange Parameter2 */
/* MD Confirm timeout in us of Subnet1 ExchgPar2 : 1sec */
#define					CONFIRM_TIMEOUT_SUBNET_1_EXCHG_PAR_2		1000000
/* MD Reply timeout in us of Subnet1 ExchgPar2 : 5sec */
#define					REPLY_TIMEOUT_SUBNET_1_EXCHG_PAR_2		5000000
/* MD Parameter of Sunet1 Exchange Parameter2 */
TRDP_MD_PAR_T				subnet1arrayExchgPar2MdPar =
							{
								CONFIRM_TIMEOUT_SUBNET_1_EXCHG_PAR_2,									/**< Confirmation timeout in us : 1sec  */
								REPLY_TIMEOUT_SUBNET_1_EXCHG_PAR_2,									/**< Reply timeout in us : 5sec         */
								(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP)		/**< Flags for MD packets               */
							};
/* Subnet2 Exchange Parameter1 */
/* MD Confirm timeout in us of Subnet1 ExchgPar2 : 1sec */
#define					CONFIRM_TIMEOUT_SUBNET_2_EXCHG_PAR_1		1000000
/* MD Reply timeout in us of Subnet2 ExchgPar1 : 5sec */
#define					REPLY_TIMEOUT_SUBNET_2_EXCHG_PAR_1		5000000
/* MD Parameter of Sunet2 Exchange Parameter1 */
TRDP_MD_PAR_T				subnet2arrayExchgPar1MdPar =
							{
								CONFIRM_TIMEOUT_SUBNET_2_EXCHG_PAR_1,									/**< Confirmation timeout in us : 1sec  */
								REPLY_TIMEOUT_SUBNET_2_EXCHG_PAR_1,									/**< Reply timeout in us : 5sec         */
								(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP)		/**< Flags for MD packets               */
							};
/* Subnet2 Exchange Parameter2 */
/* MD Confirm timeout in us of Subnet2 ExchgPar2 : 1sec */
#define					CONFIRM_TIMEOUT_SUBNET_2_EXCHG_PAR_2		1000000
/* MD Reply timeout in us of Subnet2 ExchgPar2 : 5sec */
#define					REPLY_TIMEOUT_SUBNET_2_EXCHG_PAR_2		5000000
/* MD Parameter of Sunet2 Exchange Parameter2 */
TRDP_MD_PAR_T				subnet2arrayExchgPar2MdPar =
							{
								CONFIRM_TIMEOUT_SUBNET_2_EXCHG_PAR_2,									/**< Confirmation timeout in us : 1sec  */
								REPLY_TIMEOUT_SUBNET_2_EXCHG_PAR_2,									/**< Reply timeout in us : 5sec         */
								(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP)		/**< Flags for MD packets               */
							};

/* PD Parameter *****/
/* Subnet1 Exchange Parameter3 */
/* Process Cycle Time in us of Subnet1 ExchgPar3 : 10ms */
#define					PROCESS_CYCLE_TIME_SUBNET_1_EXCHG_PAR_3		10000
/* Redundancy Group of Subnet1 ExchgPar3 : Not Redundant */
#define					REDUNDANCY_GROUP_SUBNET_1_EXCHG_PAR_3		0
/* PD Receive Timeout in us of Subnet1 ExchgPar3 : 30ms */
#define					PD_RECEIVE_TIMEOUT_SUBNET_1_EXCHG_PAR_3		30000
/* Offset Address of Subnet1 ExchgPar3 : 4096(0x2000) */
#define					OFFSET_ADDRESS_SUBNET1_EXCHG_PAR_3			4096
/* PD Parameter of Sunet1 Exchange Parameter3 */
TRDP_PD_PAR_T				subnet1arrayExchgPar3PdPar =
							{
								PROCESS_CYCLE_TIME_SUBNET_1_EXCHG_PAR_3,			/**< cycle time for TRDP process */
								REDUNDANCY_GROUP_SUBNET_1_EXCHG_PAR_3,				/**< N0 = not redundant, >0 redundancy group */
								PD_RECEIVE_TIMEOUT_SUBNET_1_EXCHG_PAR_3,			/**< Timeout value in us, before considering received process data invalid */																/**< Reply timeout in us                */
								TRDP_TO_KEEP_LAST_VALUE,								/**< Behaviour when received process data is invalid/timed out. */
								(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK),		/**< Flags for PD packets */
								OFFSET_ADDRESS_SUBNET1_EXCHG_PAR_3					/**< Offset-address for PD in traffic store for ladder topology */
							};
/* Subnet1 Exchange Parameter4 */
/* Process Cycle Time in us of Subnet1 ExchgPar4 : 10ms */
#define					PROCESS_CYCLE_TIME_SUBNET_1_EXCHG_PAR_4		10000
/* Redundancy Group of Subnet1 ExchgPar4 : Not Redundant */
#define					REDUNDANCY_GROUP_SUBNET_1_EXCHG_PAR_4		0
/* PD Receive Timeout in us of Subnet1 ExchgPar4 : 30ms */
#define					PD_RECEIVE_TIMEOUT_SUBNET_1_EXCHG_PAR_4		30000
/* Offset Address of Subnet1 ExchgPar4 : 0x3000 */
#define					OFFSET_ADDRESS_SUBNET1_EXCHG_PAR_4			0X3000
/* PD Parameter of Sunet1 Exchange Parameter4 */
TRDP_PD_PAR_T				subnet1arrayExchgPar4PdPar =
							{
								PROCESS_CYCLE_TIME_SUBNET_1_EXCHG_PAR_4,			/**< cycle time for TRDP process */
								REDUNDANCY_GROUP_SUBNET_1_EXCHG_PAR_4,				/**< N0 = not redundant, >0 redundancy group */
								PD_RECEIVE_TIMEOUT_SUBNET_1_EXCHG_PAR_4,			/**< Timeout value in us, before considering received process data invalid */																/**< Reply timeout in us                */
								TRDP_TO_KEEP_LAST_VALUE,								/**< Behaviour when received process data is invalid/timed out. */
								(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK),		/**< Flags for PD packets */
								OFFSET_ADDRESS_SUBNET1_EXCHG_PAR_4													/**< Offset-address for PD in traffic store for ladder topology */
							};
/* Subnet2 Exchange Parameter3 */
/* Process Cycle Time in us of Subnet1 ExchgPar3 : 10ms */
#define					PROCESS_CYCLE_TIME_SUBNET_2_EXCHG_PAR_3		10000
/* Redundancy Group of Subnet2 ExchgPar3 : Not Redundant */
#define					REDUNDANCY_GROUP_SUBNET_2_EXCHG_PAR_3		0
/* PD Receive Timeout in us of Subnet2 ExchgPar3 : 30ms */
#define					PD_RECEIVE_TIMEOUT_SUBNET_2_EXCHG_PAR_3		30000
/* Offset Address of Subnet1 ExchgPar3 : 4096(0x2000) */
#define					OFFSET_ADDRESS_SUBNET2_EXCHG_PAR_3			0x2000
/* PD Parameter of Sunet2 Exchange Parameter3 */
TRDP_PD_PAR_T				subnet2arrayExchgPar3PdPar =
							{
								PROCESS_CYCLE_TIME_SUBNET_2_EXCHG_PAR_3,			/**< cycle time for TRDP process */
								REDUNDANCY_GROUP_SUBNET_2_EXCHG_PAR_3,				/**< N0 = not redundant, >0 redundancy group */
								PD_RECEIVE_TIMEOUT_SUBNET_2_EXCHG_PAR_3,			/**< Timeout value in us, before considering received process data invalid */																/**< Reply timeout in us                */
								TRDP_TO_KEEP_LAST_VALUE,								/**< Behaviour when received process data is invalid/timed out. */
								(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK),		/**< Flags for PD packets */
								OFFSET_ADDRESS_SUBNET2_EXCHG_PAR_3					/**< Offset-address for PD in traffic store for ladder topology */
							};
/* Subnet2 Exchange Parameter4 */
/* Process Cycle Time in us of Subnet1 ExchgPar4 : 10ms */
#define					PROCESS_CYCLE_TIME_SUBNET_2_EXCHG_PAR_4		10000
/* Redundancy Group of Subnet2 ExchgPar4 : Not Redundant */
#define					REDUNDANCY_GROUP_SUBNET_2_EXCHG_PAR_4		0
/* PD Receive Timeout in us of Subnet2 ExchgPar4 : 30ms */
#define					PD_RECEIVE_TIMEOUT_SUBNET_2_EXCHG_PAR_4		30000
/* Offset Address of Subnet1 ExchgPar4 : 4096(0x2000) */
#define					OFFSET_ADDRESS_SUBNET2_EXCHG_PAR_4			0x2000
/* PD Parameter of Sunet2 Exchange Parameter4 */
TRDP_PD_PAR_T				subnet2arrayExchgPar4PdPar =
							{
								PROCESS_CYCLE_TIME_SUBNET_2_EXCHG_PAR_4,			/**< cycle time for TRDP process */
								REDUNDANCY_GROUP_SUBNET_2_EXCHG_PAR_4,				/**< N0 = not redundant, >0 redundancy group */
								PD_RECEIVE_TIMEOUT_SUBNET_2_EXCHG_PAR_4,			/**< Timeout value in us, before considering received process data invalid */																/**< Reply timeout in us                */
								TRDP_TO_KEEP_LAST_VALUE,								/**< Behaviour when received process data is invalid/timed out. */
								(TRDP_FLAGS_MARSHALL | TRDP_FLAGS_CALLBACK),		/**< Flags for PD packets */
								OFFSET_ADDRESS_SUBNET2_EXCHG_PAR_4													/**< Offset-address for PD in traffic store for ladder topology */
							};

/* Destination Parameter *****/
/* Number of Destination Parameter */
#define					NUM_SUBNET_1_EXCHGPAR_1_DEST 0		/* Number of Destination Parameter in subnet1 Exchange Parameter1 */
#define					NUM_SUBNET_1_EXCHGPAR_2_DEST 0		/* Number of Destination Parameter in subnet1 Exchange Parameter2 */
#define					NUM_SUBNET_1_EXCHGPAR_3_DEST 1		/* Number of Destination Parameter in subnet1 Exchange Parameter3 */
#define					NUM_SUBNET_1_EXCHGPAR_4_DEST 1		/* Number of Destination Parameter in subnet1 Exchange Parameter4 */
#define					NUM_SUBNET_2_EXCHGPAR_1_DEST 0		/* Number of Destination Parameter in subnet2 Exchange Parameter1 */
#define					NUM_SUBNET_2_EXCHGPAR_2_DEST 0		/* Number of Destination Parameter in subnet2 Exchange Parameter2 */
#define					NUM_SUBNET_2_EXCHGPAR_3_DEST 1		/* Number of Destination Parameter in subnet2 Exchange Parameter3 */
#define					NUM_SUBNET_2_EXCHGPAR_4_DEST 1		/* Number of Destination Parameter in subnet2 Exchange Parameter4 */
/* Destination parameter of Sunet1 Exchange Parameter3 Destination1 */
#define					subnet1arrayExchgPar3Dest1Id		1					/* destination identifier */
TRDP_URI_USER_T			subnet1arrayExchgPar3Dest1UriUser = {""};				/* Destination URI User (nothing) */
TRDP_URI_HOST_T			subnet1arrayExchgPar3Dest1UriHost = {"239.255.1.1"};	/* Destination URI Host (Destination IP Address) */
/* Destination parameter of Sunet1 Exchange Parameter4 Destination1 */
#define					subnet1arrayExchgPar4Dest1Id		2					/* destination identifier */
TRDP_URI_USER_T			subnet1arrayExchgPar4Dest1UriUser = {""};				/* Destination URI User (nothing) */
TRDP_URI_HOST_T			subnet1arrayExchgPar4Dest1UriHost = {"239.255.1.1"};	/* Destination URI Host (Destination IP Address) */
/* Destination parameter of Sunet2 Exchange Parameter3 Destination1 */
#define					subnet2arrayExchgPar3Dest1Id		3					/* destination identifier */
TRDP_URI_USER_T			subnet2arrayExchgPar3Dest1UriUser = {""};				/* Destination URI User (nothing) */
TRDP_URI_HOST_T			subnet2arrayExchgPar3Dest1UriHost = {"239.255.1.1"};	/* Destination URI Host (Destination IP Address) */
/* Destination parameter of Sunet2 Exchange Parameter2 Destination2 */
#define					subnet2arrayExchgPar4Dest1Id		4					/* destination identifier */
TRDP_URI_USER_T			subnet2arrayExchgPar4Dest1UriUser = {""};				/* Destination URI User (nothing) */
TRDP_URI_HOST_T			subnet2arrayExchgPar4Dest1UriHost = {"239.255.1.1"};	/* Destination URI Host (Destination IP Address) */

/* Array Destination Parameter of Sunet1 Exchange Parameter3 */
TRDP_DEST_T				subnet1arrayExchgPar3Dest[NUM_SUBNET_1_EXCHGPAR_3_DEST] =
							{
								{	/* subnet1arrayExchgPar3Dest[0] */
									subnet1arrayExchgPar3Dest1Id,			/* destination identifier */
									NULL,										/* Pointer to optional SDT Parameters for this connection */
									&subnet1arrayExchgPar3Dest1UriUser,	/* Pointer to URI user part */
									&subnet1arrayExchgPar3Dest1UriHost		/* Pointer to URI host parts or IP */
								}
							};
/* Array Destination Parameter of Sunet1 Exchange Parameter4 */
TRDP_DEST_T				subnet1arrayExchgPar4Dest[NUM_SUBNET_1_EXCHGPAR_4_DEST] =
							{
								{	/* subnet1arrayExchgPar4Dest[0] */
									subnet1arrayExchgPar4Dest1Id,			/* destination identifier */
									NULL,										/* Pointer to optional SDT Parameters for this connection */
									&subnet1arrayExchgPar4Dest1UriUser,	/* Pointer to URI user part */
									&subnet1arrayExchgPar4Dest1UriHost		/* Pointer to URI host parts or IP */
								}
							};
/* Array Destination Parameter of Sunet2 Exchange Parameter3 */
TRDP_DEST_T				subnet2arrayExchgPar3Dest[NUM_SUBNET_2_EXCHGPAR_3_DEST] =
							{
								{	/* subnet2arrayExchgPar3Dest[0] */
									subnet2arrayExchgPar3Dest1Id,			/* destination identifier */
									NULL,										/* Pointer to optional SDT Parameters for this connection */
									&subnet2arrayExchgPar3Dest1UriUser,	/* Pointer to URI user part */
									&subnet2arrayExchgPar3Dest1UriHost		/* Pointer to URI host parts or IP */
								}
							};
/* Array Destination Parameter of Sunet2 Exchange Parameter4 */
TRDP_DEST_T				subnet2arrayExchgPar4Dest[NUM_SUBNET_2_EXCHGPAR_4_DEST] =
							{
								{	/* subnet2arrayExchgPar4Dest[0] */
									subnet2arrayExchgPar4Dest1Id,			/* destination identifier */
									NULL,										/* Pointer to optional SDT Parameters for this connection */
									&subnet2arrayExchgPar4Dest1UriUser,	/* Pointer to URI user part */
									&subnet2arrayExchgPar4Dest1UriHost		/* Pointer to URI host parts or IP */
								}
							};
/* Destination parameter of Internal Config */
typedef struct INTERNAL_CONFIG_DEST
{
	UINT32          destCnt;      /* number of destinations */
	TRDP_DEST_T     *pDest;       /* Pointer to array of destination descriptors */
} INITERNAL_CONFIG_DEST_T;

INITERNAL_CONFIG_DEST_T	arrayInternalDestinationConfig[NUM_IF_CONFIG][NUM_EXCHG_PAR] =
{
	{	/* Subnet1 Destination */
		{	/* arrayInternalDestinationConfig[0][0] (Subnet1 ExchgPar1)*/
			NUM_SUBNET_1_EXCHGPAR_1_DEST,	/* number of destinations */
			NULL                    		/* Pointer to array of destination descriptors */
		},
		{	/* arrayInternalDestinationConfig[0][1] (Subnet1 ExchgPar2)*/
			NUM_SUBNET_1_EXCHGPAR_2_DEST,	/* number of destinations */
			NULL                       		/* Pointer to array of destination descriptors */
		},
		{	/* arrayInternalDestinationConfig[0][2] (Subnet1 ExchgPar3)*/
			NUM_SUBNET_1_EXCHGPAR_3_DEST,	/* number of destinations */
			subnet1arrayExchgPar3Dest								/* Pointer to array of destination descriptors */
		},
		{	/* arrayInternalDestinationConfig[0][3] (Subnet1 ExchgPar4)*/
			NUM_SUBNET_1_EXCHGPAR_4_DEST,	/* number of destinations */
			subnet1arrayExchgPar4Dest		/* Pointer to array of destination descriptors */
		}
	},
	{	/* Subnet2 Destination */
		{	/* arrayInternalDestinationConfig[1][0] (Subnet2 ExchgPar1)*/
			NUM_SUBNET_2_EXCHGPAR_1_DEST,	/* number of destinations */
			NULL                    		/* Pointer to array of destination descriptors */
		},
		{	/* arrayInternalDestinationConfig[1][1] (Subnet2 ExchgPar2)*/
			NUM_SUBNET_2_EXCHGPAR_2_DEST,	/* number of destinations */
			NULL                    		/* Pointer to array of destination descriptors */
		},
		{	/* arrayInternalDestinationConfig[1][2] (Subnet2 ExchgPar3)*/
			NUM_SUBNET_2_EXCHGPAR_3_DEST,	/* number of destinations */
			subnet2arrayExchgPar3Dest		/* Pointer to array of destination descriptors */
		},
		{	/* arrayInternalDestinationConfig[1][3] (Subnet2 ExchgPar4)*/
			NUM_SUBNET_2_EXCHGPAR_4_DEST,	/* number of destinations */
			subnet2arrayExchgPar4Dest		/* Pointer to array of destination descriptors */
		}
	}
};

/* Source Parameter *****/
/* Number of Source Parameter */
#define					NUM_SUBNET_1_EXCHGPAR_1_SRC 1		/* Number of Source Parameter in subnet1 Exchange Parameter1 */
#define					NUM_SUBNET_1_EXCHGPAR_2_SRC 0		/* Number of Source Parameter in subnet1 Exchange Parameter2 */
#define					NUM_SUBNET_1_EXCHGPAR_3_SRC 0		/* Number of Source Parameter in subnet1 Exchange Parameter3 */
#define					NUM_SUBNET_1_EXCHGPAR_4_SRC 1		/* Number of Source Parameter in subnet1 Exchange Parameter4 */
#define					NUM_SUBNET_2_EXCHGPAR_1_SRC 1		/* Number of Source Parameter in subnet2 Exchange Parameter1 */
#define					NUM_SUBNET_2_EXCHGPAR_2_SRC 0		/* Number of Source Parameter in subnet2 Exchange Parameter2 */
#define					NUM_SUBNET_2_EXCHGPAR_3_SRC 0		/* Number of Source Parameter in subnet2 Exchange Parameter3 */
#define					NUM_SUBNET_2_EXCHGPAR_4_SRC 1		/* Number of Source Parameter in subnet2 Exchange Parameter4 */
/* Source parameter of Sunet1 Exchange Parameter1 Source1 */
#define					subnet1arrayExchgPar1Src1Id		1						/* Source identifier */
TRDP_URI_USER_T			subnet1arrayExchgPar1Src1UriUser = {""};				/* Source URI user part (nothing) */
TRDP_URI_HOST_T			subnet1arrayExchgPar1Src1UriHost1 = {"10.0.1.19"};	/* Source device URI host or IP (Destination IP Address) */
TRDP_URI_HOST_T			subnet1arrayExchgPar1Src1UriHost2 = {""};	/* Source second device URI host parts or IP, used eg. for red. devices (Destination IP Address) */
/* Source parameter of Sunet1 Exchange Parameter4 Source1 */
#define					subnet1arrayExchgPar4Src1Id		3						/* Source identifier */
TRDP_URI_USER_T			subnet1arrayExchgPar4Src1UriUser = {""};				/* Source URI user part (nothing) */
TRDP_URI_HOST_T			subnet1arrayExchgPar4Src1UriHost1 = {"10.0.1.17"};	/* Source device URI host or IP (Destination IP Address) */
TRDP_URI_HOST_T			subnet1arrayExchgPar4Src1UriHost2 = {""};	/* Source second device URI host parts or IP, used eg. for red. devices (Destination IP Address) */
/* Source parameter of Sunet2 Exchange Parameter1 Source1 */
#define					subnet2arrayExchgPar1Src1Id		5						/* Source identifier */
TRDP_URI_USER_T			subnet2arrayExchgPar1Src1UriUser = {""};				/* Source URI user part (nothing) */
TRDP_URI_HOST_T			subnet2arrayExchgPar1Src1UriHost1 = {"10.0.33.19"};	/* Source device URI host or IP (Destination IP Address) */
TRDP_URI_HOST_T			subnet2arrayExchgPar1Src1UriHost2 = {""};	/* Source second device URI host parts or IP, used eg. for red. devices (Destination IP Address) */
/* Source parameter of Sunet2 Exchange Parameter4 Source1 */
#define					subnet2arrayExchgPar4Src1Id		7						/* Source identifier */
TRDP_URI_USER_T			subnet2arrayExchgPar4Src1UriUser = {""};				/* Source URI user part (nothing) */
TRDP_URI_HOST_T			subnet2arrayExchgPar4Src1UriHost1 = {"10.0.33.17"};	/* Source device URI host or IP (Destination IP Address) */
TRDP_URI_HOST_T			subnet2arrayExchgPar4Src1UriHost2 = {""};	/* Source second device URI host parts or IP, used eg. for red. devices (Destination IP Address) */
/* Array Source Parameter of Sunet1 Exchange Parameter1 */
TRDP_SRC_T					subnet1arrayExchgPar1Src[NUM_SUBNET_1_EXCHGPAR_1_SRC] =
							{
								{	/* subnet1arrayExchgPar1Src[0] */
									subnet1arrayExchgPar1Src1Id,			/* Source identifier */
									NULL,										/* Pointer to optional SDT Parameters for this connection */
									&subnet1arrayExchgPar1Src1UriUser,		/* Pointer to URI user part */
									&subnet1arrayExchgPar1Src1UriHost1,	/* Pointer to URI host parts or IP */
									&subnet1arrayExchgPar1Src1UriHost2		/* Pointer to a second device URI host parts or IP, used eg. for red. devices */
								}
							};
/* Array Source Parameter of Sunet1 Exchange Parameter4 */
TRDP_SRC_T					subnet1arrayExchgPar4Src[NUM_SUBNET_1_EXCHGPAR_4_SRC] =
							{
								{	/* subnet1arrayExchgPar4Src[0] */
									subnet1arrayExchgPar4Src1Id,			/* Source identifier */
									NULL,										/* Pointer to optional SDT Parameters for this connection */
									&subnet1arrayExchgPar4Src1UriUser,		/* Pointer to URI user part */
									&subnet1arrayExchgPar4Src1UriHost1,	/* Pointer to URI host parts or IP */
									&subnet1arrayExchgPar4Src1UriHost2		/* Pointer to a second device URI host parts or IP, used eg. for red. devices */
								}
							};
/* Array Source Parameter of Sunet2 Exchange Parameter1 */
TRDP_SRC_T					subnet2arrayExchgPar1Src[NUM_SUBNET_2_EXCHGPAR_1_SRC] =
							{
								{	/* subnet2arrayExchgPar1Src[0] */
									subnet2arrayExchgPar1Src1Id,			/* Source identifier */
									NULL,										/* Pointer to optional SDT Parameters for this connection */
									&subnet2arrayExchgPar1Src1UriUser,		/* Pointer to URI user part */
									&subnet2arrayExchgPar1Src1UriHost1,	/* Pointer to URI host parts or IP */
									&subnet2arrayExchgPar1Src1UriHost2		/* Pointer to a second device URI host parts or IP, used eg. for red. devices */
								}
							};
/* Array Source Parameter of Sunet2 Exchange Parameter4 */
TRDP_SRC_T				subnet2arrayExchgPar4Src[NUM_SUBNET_2_EXCHGPAR_4_SRC] =
							{
								{	/* subnet2arrayExchgPar4Src[0] */
									subnet2arrayExchgPar4Src1Id,			/* Source identifier */
									NULL,										/* Pointer to optional SDT Parameters for this connection */
									&subnet2arrayExchgPar4Src1UriUser,		/* Pointer to URI user part */
									&subnet2arrayExchgPar4Src1UriHost1,	/* Pointer to URI host parts or IP */
									&subnet2arrayExchgPar4Src1UriHost2		/* Pointer to a second device URI host parts or IP, used eg. for red. devices */
								}
							};
/* Source parameter of Internal Config */
typedef struct INTERNAL_CONFIG_SRC
{
	UINT32          srcCnt;      /* number of Sources */
	TRDP_SRC_T     *pSrc;       /* Pointer to array of Source descriptors */
} INITERNAL_CONFIG_SRC_T;

INITERNAL_CONFIG_SRC_T	arrayInternalSourceConfig[NUM_IF_CONFIG][NUM_EXCHG_PAR] =
{
	{	/* Subnet1 Source */
		{	/* arrayInternalSourceConfig[0][0] (Subnet1 ExchgPar1)*/
			NUM_SUBNET_1_EXCHGPAR_1_SRC,	/* number of Sources */
			subnet1arrayExchgPar1Src			/* Pointer to array of Source descriptors */
		},
		{	/* arrayInternalSourceConfig[0][1] (Subnet1 ExchgPar2)*/
			NUM_SUBNET_1_EXCHGPAR_2_SRC,	/* number of Sources */
			NULL								/* Pointer to array of Source descriptors */
		},
		{	/* arrayInternalSourceConfig[0][2] (Subnet1 ExchgPar3)*/
			NUM_SUBNET_1_EXCHGPAR_3_SRC,	/* number of Sources */
			NULL                    			/* Pointer to array of Source descriptors */
		},
		{	/* arrayInternalSourceConfig[0][3] (Subnet1 ExchgPar4)*/
			NUM_SUBNET_1_EXCHGPAR_4_SRC,	/* number of Sources */
			subnet1arrayExchgPar4Src			/* Pointer to array of Source descriptors */
		}
	},
	{	/* Subnet2 Source */
		{	/* arrayInternalDestinationConfig[1][0] (Subnet2 ExchgPar1)*/
			NUM_SUBNET_2_EXCHGPAR_1_SRC,	/* number of Sources */
			subnet2arrayExchgPar1Src			/* Pointer to array of Source descriptors */
		},
		{	/* arrayInternalSourceConfig[1][1] (Subnet2 ExchgPar2)*/
			NUM_SUBNET_2_EXCHGPAR_2_SRC,	/* number of Sources */
			NULL								/* Pointer to array of Source descriptors */
		},
		{	/* arrayInternalSourceConfig[1][2] (Subnet2 ExchgPar3)*/
			NUM_SUBNET_2_EXCHGPAR_3_SRC,	/* number of Sources */
			NULL                       			/* Pointer to array of Source descriptors */
		},
		{	/* arrayInternalSourceConfig[1][3] (Subnet2 ExchgPar4)*/
			NUM_SUBNET_2_EXCHGPAR_4_SRC,	/* number of Sources */
			subnet2arrayExchgPar4Src			/* Pointer to array of Source descriptors */
		}
	}
};

/* Exchange Parameter *****/
/* Array of Exchange Parameter */
TRDP_EXCHG_PAR_T			*arrayExchgPar[LADDER_IF_NUMBER] = {0};

#define					DEFAULT_NUM_DESTINATION			0			/* Default Number of Destination */
#define					DEFAULT_DESTINATION_POINTER		NULL		/* Default Destination Pointer */
#define					DEFAULT_NUM_SOURCE				0			/* Default Number of Source */
#define					DEFAULT_SOURCE_POINTER			NULL	 	/* Default Source Pointer */
/* Array of Exchange Parameter of Internal Config */
TRDP_EXCHG_PAR_T			arrayInternalConfigExchgPar[NUM_IF_CONFIG][NUM_EXCHG_PAR] =
{
	{	/* Subnet1 Exchange Parameter */
		{	/* arrayInternalConfigExchgPar[0][0] (Subnet1 ExchgPar1) */
			SUBNET_1_MR_1_COMID_200003,								/* comId : 200003 */
			SUBNET_1_MR_1_DATASETID_2003,							/* DatasetId : 2003 */
			COMMUNICATION_PAR_ID_2_FOR_MD,							/* communication parameter id : 2 for MD */
			&subnet1arrayExchgPar1MdPar,      						/* Pointer to MD Parameters for this connection */
			NULL,							      						/* Pointer to PD Parameters for this connection */
			DEFAULT_NUM_DESTINATION,									/* number of destinations : 0 */
			DEFAULT_DESTINATION_POINTER,							/* Pointer to array of destination descriptors : NULL */
			DEFAULT_NUM_SOURCE,										/* number of sources : 0 */
			DEFAULT_SOURCE_POINTER									/* Pointer to array of source descriptors : NULL */
		},
		{	/* arrayInternalConfigExchgPar[0][1] (Subnet1 ExchgPar2)*/
			SUBNET_1_MP_1_COMID_210003,								/* comId : 210003 */
			SUBNET_1_MP_1_DATASETID_2103,							/* DatasetId : 2103 */
			COMMUNICATION_PAR_ID_2_FOR_MD,							/* communication parameter id :2 for MD */
			&subnet1arrayExchgPar2MdPar,      						/* Pointer to MD Parameters for this connection */
			NULL,							      						/* Pointer to PD Parameters for this connection */
			DEFAULT_NUM_DESTINATION,									/* number of destinations : 0 */
			DEFAULT_DESTINATION_POINTER,							/* Pointer to array of destination descriptors : NULL */
			DEFAULT_NUM_SOURCE,										/* number of sources : 0 */
			DEFAULT_SOURCE_POINTER									/* Pointer to array of source descriptors : NULL */
		},
		{	/* arrayInternalConfigExchgPar[0][2] (Subnet1 ExchgPar3)*/
			SUBNET_1_PD_1_COMID_10001,								/* comId : 10001 */
			SUBNET_1_PD_1_DATASETID_1001,							/* DatasetId : 1001 */
			COMMUNICATION_PAR_ID_1_FOR_PD,							/* communication parameter id : 1 for PD */
			NULL,							      						/* Pointer to MD Parameters for this connection */
			&subnet1arrayExchgPar3PdPar,      						/* Pointer to PD Parameters for this connection */
			DEFAULT_NUM_DESTINATION,									/* number of destinations : 0 */
			DEFAULT_DESTINATION_POINTER,							/* Pointer to array of destination descriptors : NULL */
			DEFAULT_NUM_SOURCE,										/* number of sources : 0 */
			DEFAULT_SOURCE_POINTER									/* Pointer to array of source descriptors : NULL */
		},
		{	/* arrayInternalConfigExchgPar[0][3] (Subnet1 ExchgPar4)*/
			SUBNET_1_PD_2_COMID_10002,								/* comId : 10002 */
			SUBNET_1_PD_2_DATASETID_1002,							/* DatasetId : 1002 */
			COMMUNICATION_PAR_ID_1_FOR_PD,							/* communication parameter id : 1 for PD */
			NULL,							      						/* Pointer to MD Parameters for this connection */
			&subnet1arrayExchgPar4PdPar,      						/* Pointer to PD Parameters for this connection */
			DEFAULT_NUM_DESTINATION,									/* number of destinations : 0 */
			DEFAULT_DESTINATION_POINTER,							/* Pointer to array of destination descriptors : NULL */
			DEFAULT_NUM_SOURCE,										/* number of sources : 0 */
			DEFAULT_SOURCE_POINTER									/* Pointer to array of source descriptors : NULL */
		},
	},
	{	/* Subnet1 Exchange Parameter */
		{	/* arrayInternalConfigExchgPar[1][0] (Subnet2 ExchgPar1) */
			SUBNET_2_MR_1_COMID_200003,								/* comId : 200003*/
			SUBNET_2_MR_1_DATASETID_2003,							/* DatasetId : 2003 */
			COMMUNICATION_PAR_ID_2_FOR_MD,							/* communication parameter id :2 for MD*/
			&subnet2arrayExchgPar1MdPar,      						/* Pointer to MD Parameters for this connection */
			NULL,							      						/* Pointer to PD Parameters for this connection */
			DEFAULT_NUM_DESTINATION,									/* number of destinations : 0 */
			DEFAULT_DESTINATION_POINTER,							/* Pointer to array of destination descriptors : NULL */
			DEFAULT_NUM_SOURCE,										/* number of sources : 0 */
			DEFAULT_SOURCE_POINTER									/* Pointer to array of source descriptors : NULL */
		},
		{	/* arrayInternalConfigExchgPar[1][1] (Subnet2 ExchgPar2)*/
			SUBNET_2_MP_1_COMID_210003,								/* comId : 210003 */
			SUBNET_2_MP_1_DATASETID_2103,							/* DatasetId : 2103 */
			COMMUNICATION_PAR_ID_2_FOR_MD,							/* communication parameter id : 2 for MD */
			&subnet2arrayExchgPar2MdPar,      						/* Pointer to MD Parameters for this connection */
			NULL,							      						/* Pointer to PD Parameters for this connection */
			DEFAULT_NUM_DESTINATION,									/* number of destinations : 0 */
			DEFAULT_DESTINATION_POINTER,							/* Pointer to array of destination descriptors : NULL */
			DEFAULT_NUM_SOURCE,										/* number of sources : 0 */
			DEFAULT_SOURCE_POINTER									/* Pointer to array of source descriptors : NULL */
		},
		{	/* arrayInternalConfigExchgPar[1][2] (Subnet2 ExchgPar3)*/
			SUBNET_2_PD_1_COMID_10001,													/* comId */
			SUBNET_2_PD_1_DATASETID_1001,														/* DatasetId */
			COMMUNICATION_PAR_ID_1_FOR_PD,															/* communication parameter id */
			NULL,							      						/* Pointer to MD Parameters for this connection */
			&subnet2arrayExchgPar3PdPar,      						/* Pointer to PD Parameters for this connection */
			DEFAULT_NUM_DESTINATION,									/* number of destinations : 0 */
			DEFAULT_DESTINATION_POINTER,							/* Pointer to array of destination descriptors : NULL */
			DEFAULT_NUM_SOURCE,										/* number of sources : 0 */
			DEFAULT_SOURCE_POINTER									/* Pointer to array of source descriptors : NULL */
		},
		{	/* arrayInternalConfigExchgPar[1][3] (Subnet2 ExchgPar4)*/
			SUBNET_2_PD_2_COMID_10002,													/* comId */
			SUBNET_2_PD_2_DATASETID_1002,														/* DatasetId */
			COMMUNICATION_PAR_ID_1_FOR_PD,															/* communication parameter id */
			NULL,							      						/* Pointer to MD Parameters for this connection */
			&subnet2arrayExchgPar4PdPar,      						/* Pointer to PD Parameters for this connection */
			DEFAULT_NUM_DESTINATION,									/* number of destinations : 0 */
			DEFAULT_DESTINATION_POINTER,							/* Pointer to array of destination descriptors : NULL */
			DEFAULT_NUM_SOURCE,										/* number of sources : 0 */
			DEFAULT_SOURCE_POINTER									/* Pointer to array of source descriptors : NULL */
		}
	}
};

/* Application Handle */
TRDP_APP_SESSION_T		appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
TRDP_APP_SESSION_T		appHandle2;				/*	Sub-network Id2 identifier to the library instance	*/

#endif /* ifdef XML_CONFIG_ENABLE */
#endif	/* TRDP_OPTION_LADDER */

#ifdef __cplusplus
}
#endif
#endif /* TAU_LDLADDER_CONFIG_H_ */
