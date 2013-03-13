/**********************************************************************************************************************/
/**
 * @file            ladderApplication_publisher.c
 *
 * @brief           Demo ladder application for TRDP
 *
 * @details			TRDP Ladder Topology Support PD Transmission Publisher
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, TOSHIBA
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright TOSHIBA, Japan, 2013.
 *
 */
#ifdef TRDP_OPTION_LADDER
/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <ifaddrs.h>
#include <limits.h>
#include <float.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "vos_utils.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "trdp_pdcom.h"
#include "trdp_utils.h"
#include "trdp_if_light.h"
#include "trdp_ladder.h"
#include "trdp_ladder_app.h"
#include "tau_marshall.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

/* Sub-network Id1 */
TRDP_APP_SESSION_T  appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
TRDP_SUB_T          subHandleNet1ComId1;		/*	Sub-network Id1 ComID1 identifier to the subscription	*/
TRDP_PUB_T          pubHandleNet1ComId1;		/*	Sub-network Id1 ComID2 identifier to the publication	*/
TRDP_SUB_T          subHandleNet1ComId2;		/*	Sub-network Id1 ComID1 identifier to the subscription	*/
TRDP_PUB_T          pubHandleNet1ComId2;		/*	Sub-network Id1 ComID2 identifier to the publication	*/
TRDP_ERR_T          err;
/* TRDP_PD_CONFIG_T    pdConfiguration = {tlp_recvPdDs, NULL, {0, 0}, TRDP_FLAGS_NONE,
                                       10000000, TRDP_TO_SET_TO_ZERO, 20548};*/
TRDP_PD_CONFIG_T    pdConfiguration = {tlp_recvPdDs, NULL, {0, 0}, TRDP_FLAGS_CALLBACK,
                                       10000000, TRDP_TO_SET_TO_ZERO, 20548};
TRDP_MEM_CONFIG_T   dynamicConfig = {NULL, RESERVED_MEMORY, {}};
TRDP_PROCESS_CONFIG_T	processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};
TRDP_MARSHALL_CONFIG_T	marshallConfig = {tau_marshall, tau_unmarshall, NULL};	/** Marshaling/unMarshalling configuration	*/

INT32     rv = 0;

/* Sub-network Id2 */
TRDP_APP_SESSION_T  appHandle2;					/*	Sub-network Id2 identifier to the library instance	*/
TRDP_SUB_T          subHandleNet2ComId1;		/*	Sub-network Id2 ComID1 identifier to the subscription	*/
TRDP_PUB_T          pubHandleNet2ComId1;		/*	Sub-network Id2 ComID2 identifier to the publication	*/
TRDP_SUB_T          subHandleNet2ComId2;		/*	Sub-network Id2 ComID1 identifier to the subscription	*/
TRDP_PUB_T          pubHandleNet2ComId2;		/*	Sub-network Id2 ComID2 identifier to the publication	*/
TRDP_ERR_T          err2;
/* TRDP_PD_CONFIG_T    pdConfiguration2 = {tlp_recvPdDs, NULL, {0, 0}, TRDP_FLAGS_NONE,
                                       10000000, TRDP_TO_SET_TO_ZERO, 20548};*/	/* Sub-network Id2 PDconfiguration */
TRDP_PD_CONFIG_T    pdConfiguration2 = {tlp_recvPdDs, NULL, {0, 0}, TRDP_FLAGS_CALLBACK,
                                       10000000, TRDP_TO_SET_TO_ZERO, 20548};
TRDP_MEM_CONFIG_T   dynamicConfig2 = {NULL, RESERVED_MEMORY, {}};					/* Sub-network Id2 Structure describing memory */
TRDP_PROCESS_CONFIG_T   processConfig2   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};

/* Initial value of TRDP parameter */
UINT32 PD_COMID1 = 10001;							/* ComId1 */
UINT32 PD_COMID2 = 10002;							/* ComId2 */
UINT32 PD_COMID1_TIMEOUT = 1200000;				    /* Macro second */
UINT32 PD_COMID2_TIMEOUT = 1200000;				    /* Macro second */
TRDP_IP_ADDR_T PD_COMID1_SRC_IP = 0x0A040111;		/* Source IP: 10.4.1.17 */
TRDP_IP_ADDR_T PD_COMID2_SRC_IP = 0x0A040111;		/* Source IP: 10.4.1.17 */
TRDP_IP_ADDR_T PD_COMID1_DST_IP = 0xefff0101;		/* multicast Destination IP: 239.255.1.1 */
TRDP_IP_ADDR_T PD_COMID2_DST_IP = 0xefff0101;		/* multicast Destination IP: 239.255.1.1 */
//TRDP_IP_ADDR_T PD_COMID1_DST_IP = 0x0A040111;		/* unicast Destination IP: 10.4.1.17 */
//TRDP_IP_ADDR_T PD_COMID2_DST_IP = 0x0A040111;		/* unicast Destination IP: 10.4.1.17 */
//TRDP_IP_ADDR_T PD_COMID1_DST_IP2 = 0;				/* unicast Destination IP: 10.4.1.17 */
//TRDP_IP_ADDR_T PD_COMID2_DST_IP2 = 0;				/* unicast Destination IP: 10.4.1.17 */

TRDP_IP_ADDR_T PD_COMID1_SUB_SRC_IP1 = 0x0A040111;		/* Subscribe Source IP: 10.4.1.17 */
TRDP_IP_ADDR_T PD_COMID2_SUB_SRC_IP1 = 0x0A040111;		/* Subscribe Source IP: 10.4.1.17 */
TRDP_IP_ADDR_T PD_COMID1_SUB_SRC_IP2 = 0x0A042111;		/* Subscribe Source IP: 10.4.33.17 */
TRDP_IP_ADDR_T PD_COMID2_SUB_SRC_IP2 = 0x0A042111;		/* Subscribe Source IP: 10.4.33.17 */

//TRDP_IP_ADDR_T PD_COMID1_SUB_DST_IP = 0x0A040110;		/* Subscribe Destination IP: 10.4.1.16 */
//TRDP_IP_ADDR_T PD_COMID2_SUB_DST_IP = 0x0A040110;		/* Subscribe Destination IP: 10.4.1.16 */
TRDP_IP_ADDR_T PD_COMID1_SUB_DST_IP1 = 0xefff0101;		/* Subscribe multicast Destination IP: 239.255.1.1 */
TRDP_IP_ADDR_T PD_COMID2_SUB_DST_IP1 = 0xefff0101;		/* Subscribe multicast Destination IP: 239.255.1.1 */
//TRDP_IP_ADDR_T PD_COMID1_SUB_DST_IP2 = 0x0A042110;		/* Subscribe Destination IP: 10.4.33.16 */
//TRDP_IP_ADDR_T PD_COMID2_SUB_DST_IP2 = 0x0A042110;		/* Subscribe Destination IP: 10.4.33.16 */
TRDP_IP_ADDR_T PD_COMID1_SUB_DST_IP2 = 0xefff0101;		/* Subscribe multicast Destination IP: 239.255.1.1 */
TRDP_IP_ADDR_T PD_COMID2_SUB_DST_IP2 = 0xefff0101;		/* Subscribe multicast Destination IP: 239.255.1.1 */

//TRDP_IP_ADDR_T PD_COMID1_PUB_DST_IP = 0x0A040111;		/* Publish Destination IP: 10.4.1.17 */
//TRDP_IP_ADDR_T PD_COMID2_PUB_DST_IP = 0x0A040111;		/* Publish Destination IP: 10.4.1.17 */
TRDP_IP_ADDR_T PD_COMID1_PUB_DST_IP1 = 0xefff0101;		/* Publish multicast Destination IP: 239.255.1.1 */
TRDP_IP_ADDR_T PD_COMID2_PUB_DST_IP1 = 0xefff0101;		/* Publish multicastDestination IP: 239.255.1.1 */
//TRDP_IP_ADDR_T PD_COMID1_PUB_DST_IP2 = 0x0A042111;		/* Publish Destination IP: 10.4.33.17 */
//TRDP_IP_ADDR_T PD_COMID2_PUB_DST_IP2 = 0x0A042111;		/* Publish Destination IP: 10.4.33.17 */
TRDP_IP_ADDR_T PD_COMID1_PUB_DST_IP2 = 0xefff0101;		/* Publish multicast Destination IP: 239.255.1.1 */
TRDP_IP_ADDR_T PD_COMID2_PUB_DST_IP2 = 0xefff0101;		/* Publish multicast Destination IP: 239.255.1.1 */

/* Subscribe for Sub-network Id2 */
TRDP_IP_ADDR_T PD_COMID1_SRC_IP2 = 0;
TRDP_IP_ADDR_T PD_COMID2_SRC_IP2 = 0;
/* Publish for Sub-network Id1 */
UINT32 PD_COMID1_CYCLE = 100000;					/* ComID1 Publish Cycle TIme */
UINT32 PD_COMID2_CYCLE = 100000;					/* ComID2 Publish Cycle TIme */
/* Ladder Topolpgy enable/disable */
BOOL ladderTopologyFlag = TRUE;						/* Ladder Topology : TURE, Not Ladder Topology : FALSE */
/* OFFSET ADDRESS */
UINT16 OFFSET_ADDRESS1 = 0x1100;					/* offsetAddress comId1 publish */
UINT16 OFFSET_ADDRESS2 = 0x1180;					/* offsetAddress comId2 publish */
UINT16 OFFSET_ADDRESS3 = 0x1300;					/* offsetAddress comId1 subscribe */
UINT16 OFFSET_ADDRESS4 = 0x1380;					/* offsetAddress comId2 subscribe */
/* Marshalling enable/disable */
BOOL marshallingFlag = FALSE;						/* Marshalling Enable : TURE, Marshalling Disable : FALSE */
UINT32	publisherAppCycle = 200000;					/* Publisher Application cycle in us */
UINT32	subscriberAppCycle = 120000;				/* Subscriber Application cycle in us */
/* Using Receive Subnet in order to Wirte PD in Traffic Store  */
UINT32 TS_SUBNET =1;
/* PD return NG counter */
UINT32 PD_RETURN_NG_COUNTER = 0;

INT32	rv2 = 0;
CHAR8 	firstPutData[PD_DATA_SIZE_MAX] = "First Put";

/* PD DATASET */
TRDP_DATASET_T DATASET1_TYPE =
{
	1001,		/* datasetID */
	0,			/* reserved */
	16,			/* No of elements */
	{			/* TRDP_DATASET_ELEMENT_T [] */
			{
					TRDP_BOOLEAN, 	/**< =UINT8, 1 bit relevant (equal to zero = false, not equal to zero = true) */
					1					/* No of elements */
			},
			{
					TRDP_CHAR8, 		/* data type < char, can be used also as UTF8  */
					1					/* No of elements */
			},
			{
					TRDP_UTF16,   	    /* data type < Unicode UTF-16 character  */
					1					/* No of elements */
			},
			{
					TRDP_INT8,   		/* data type < Signed integer, 8 bit  */
					1					/* No of elements */
			},
			{
					TRDP_INT16,		    /* data type < Signed integer, 16 bit */
					1					/* No of elements */
			},
			{
					TRDP_INT32,		    /* data type < Signed integer, 32 bit */
					1					/* No of elements */
			},
			{
					TRDP_INT64,		    /* data type < Signed integer, 64 bit */
					1					/* No of elements */
			},
			{
					TRDP_UINT8,		    /* data type < Unsigned integer, 8 bit */
					1					/* No of elements */
			},
			{
					TRDP_UINT16,		/* data type < Unsigned integer, 16 bit */
					1					/* No of elements */
			},
			{
					TRDP_UINT32,		/* data type < Unsigned integer, 32 bit */
					1					/* No of elements */
			},
			{
					TRDP_UINT64,		/* data type < Unsigned integer, 64 bit */
					1					/* No of elements */
			},
			{
					TRDP_REAL32,		/* data type < Floating point real, 32 bit */
					1					/* No of elements */
			},
			{
					TRDP_REAL64,		/* data type < Floating point real, 64 bit */
					1					/* No of elements */
			},
			{
					TRDP_TIMEDATE32,	/* data type < 32 bit UNIX time  */
					1					/* No of elements */
			},
			{
					TRDP_TIMEDATE48,	/* data type *< 48 bit TCN time (32 bit UNIX time and 16 bit ticks)  */
					1					/* No of elements */
			},
			{
					TRDP_TIMEDATE64,	/* data type *< 32 bit UNIX time + 32 bit miliseconds  */
					1					/* No of elements */
			}
	}
};

TRDP_DATASET_T DATASET2_TYPE =
{
	1002,		/* datasetID */
	0,			/* reserved */
	2,			/* No of elements */
	{
			{
					1001,		 		/* data type < dataset 1001  */
					2					/* No of elements */
			},
			{
					TRDP_INT16,		    /* data type < Signed integer, 16 bit */
					64					/* No of elements */
			}
	}
};

/* Will be sorted by tau_initMarshall */
TRDP_DATASET_T *gDataSets[] =
{
	&DATASET1_TYPE,
	&DATASET2_TYPE,
};
/* ComId DATASETID Mapping */
TRDP_COMID_DSID_MAP_T gComIdMap[] =
{
	{10001, 1001},
	{10002, 1002}
};

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon			user supplied context pointer
 *  @param[in]		category		Log category (Error, Warning, Info etc.)
 *  @param[in]		pTime			pointer to NULL-terminated string of time stamp
 *  @param[in]		pFile			pointer to NULL-terminated string of source module
 *  @param[in]		LineNumber		line
 *  @param[in]		pMsgStr         pointer to NULL-terminated string
 *  @retval         none
 */
void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:"};
    printf("%s %s %s:%d %s",
           pTime,
           catStr[category],
           pFile,
           LineNumber,
           pMsgStr);
}

/**********************************************************************************************************************/
/** Compare PD DataSet
 *
 *  @param[in]		pSendPdDataset			    pointer to Send PD Dataset
 *  @param[in]		pReceivePdDataset			pointer to Receive PD Dataset
 *  @param[in]		sendPdDatasetSize			Send PD Dataset SIZE
 *  @param[in]		receivePdDatasetSize		Receive PD Dataset SIZE
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
INT8 comparePdDataset (
		const void *pSendPdDataset,
		const void *pReceivePdDataset,
		size_t sendPdDatasetSize,
		size_t receivePdDatasetSize)
{
	/* Compare Size */
	if (sendPdDatasetSize != receivePdDatasetSize)
	{
		printf("<NG> Compare Send PD with receive PD error. The size of is different.\n");
		return 1;
	}

	/* Compare Contents */
	if (memcmp(pSendPdDataset, pReceivePdDataset, sendPdDatasetSize) != 0)
	{
		printf("<NG> Compare Send PD with receive PD error. Contents is different.\n");
		return 1;
	}

	printf("<OK> Compare Send PD with receive PD OK!\n");
	return 0;
}

/**********************************************************************************************************************/
/** Dump Memory
 *
 *  @param[in]		pDumpStartAddress			pointer to Dump Memory start address
 *  @param[in]		dumpSize					Dump Memory SIZE
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
INT8 dumpMemory (
		const void *pDumpStartAddress,
		size_t dumpSize)
{
	if (pDumpStartAddress != NULL && dumpSize > 0)
	{
		int i,j;
		const UINT8 *b = pDumpStartAddress;
		for(i = 0; i < dumpSize; i += 16)
		{
			printf("%04X ", i );
			/**/
			for(j = 0; j < 16; j++)
			{
				if (j == 8) printf("- ");
				if ((i+j) < dumpSize)
				{
					int ch = b[i+j];
					printf("%02X ",ch);
				}
				else
				{
					printf("   ");
				}
			}
			printf("   ");
			for(j = 0; j < 16 && (i+j) < dumpSize; j++)
			{
				int ch = b[i+j];
				printf("%c", (ch >= ' ' && ch <= '~') ? ch : '.');
			}
			printf("\n");
		}
	}
	else
	{
		return 1;
	}
	return 0;
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (int argc, char *argv[])
{
	TRDP_ERR_T err;
//	CHAR8 stringMaxData[16] = "STRING MAX SIZE";
	PD_COMID1_SRC_IP2 = PD_COMID1_SRC_IP | SUBNET2_NETMASK;	    /* Sender's IP: 10.4.33.17 (default) */
	PD_COMID2_SRC_IP2 = PD_COMID2_SRC_IP | SUBNET2_NETMASK;	    /* Sender's IP: 10.4.33.17 (default) */

	UINT32 pdDataset1ReturnNgCounter = 0;						/* Criterion of the failure PD DATASET1 Return */
	UINT32 pdDataset2ReturnNgCounter = 0;						/* Criterion of the failure PD DATASET2 Return */
	UINT32 pdDataset1CreateCount = 0;							/* Create PD DATASET1 Counter */
	UINT32 pdDataset2CreateCount = 0;							/* Create PD DATASET2 Counter */
	UINT32 pdDataset1ReturnFailureCount = 0;					/* PD DATASET1 Return NG Counter */
	UINT32 pdDataset2ReturnFailureCount = 0;					/* PD DATASET2 Return NG Counter */
	UINT16	i = 0;													/* Loop Counter for leading putDataSetBuffer*/

	DATASET1 putDataSet1 = {0};									/* put DataSet1 */
	DATASET2 putDataSet2;										/* put DataSet2 */
	DATASET1 getDataSet1 = {0};									/* get DataSet1 from Traffic Store */
	DATASET2 getDataSet2;										/* get DataSet2 from Traffic Store */
	memset(&putDataSet2, 0, sizeof(putDataSet2));
	memset(&getDataSet2, 0, sizeof(getDataSet2));

	DATASET1 dataSet1 = {0};									/* publish Dataset1 */
	DATASET2 dataSet2;											/* publish Dataset2 */
	size_t dataSet1Size = 0;									/* publish Dataset1 SIZE */
	size_t dataSet2Size = 0;									/* publish Dataset2 SIZE */
	UINT16 inputPosition = 0;									/* putDataSetBuffer position */
	DATASET1 putDataSet1Buffer[PUT_DATASET_BUFFER_SIZE];		/* putDataSet1Buffer for PD compare */
	DATASET2 putDataSet2Buffer[PUT_DATASET_BUFFER_SIZE];		/* putDataSet2Buffer for PD compare */

	memset(&dataSet1, 0, sizeof(dataSet1));
	memset(&dataSet2, 0, sizeof(dataSet2));
	memset(putDataSet1Buffer, 0, sizeof(putDataSet1Buffer));
	memset(putDataSet2Buffer, 0, sizeof(putDataSet2Buffer));

	/* First Put Data in Dataset1 */
//	strncpy(dataSet1.string, firstPutData, strlen(firstPutData));
	/* First Put Data in Dataset2 */
//	strncpy(dataSet2.dataset1[1].string, firstPutData, strlen(firstPutData));
	dataSet1Size = sizeof(dataSet1);
	dataSet2Size = sizeof(dataSet2);

	void *pRefConMarshallDataset;
	UINT32 usingComIdNumber = 2;							/* 2 = ComId:10001,10002 */
	UINT32 usingDatasetNumber =2;							/* 2 = DATASET1,DATASET2 */
	TRDP_MARSHALL_CONFIG_T	*pMarshallConfigPtr = NULL;	    /* Marshaling/unMarshalling configuration Pointer	*/

	/* Display PD Application Version */
	printf("PD Application Version %s: ladderApplication_publisher Start \n", PD_APP_VERSION);

	/* Command analysis */
	struct option long_options[] = {		/* Command Option */
			{"topo",					required_argument,	NULL, 't'},
			{"offset1",				    required_argument,	NULL, '1'},
			{"offset2",				    required_argument,	NULL, '2'},
			{"offset3",				    required_argument,	NULL, '3'},
			{"offset4",				    required_argument,	NULL, '4'},
			{"pub-app-cycle",			required_argument,	NULL, 'p'},
			{"marshall",				required_argument,	NULL, 'm'},
			{"comid1",					required_argument,	NULL, 'c'},
			{"comid2",					required_argument,	NULL, 'C'},
			{"comid1-sub-src-ip1",	    required_argument,	NULL, 'a'},
			{"comid1-sub-dst-ip1",	    required_argument,	NULL, 'b'},
			{"comid2-sub-src-ip1",	    required_argument,	NULL, 'A'},
			{"comid2-sub-dst-ip1",	    required_argument,	NULL, 'B'},
			{"comid1-pub-dst-ip1",	    required_argument,	NULL, 'f'},
			{"comid2-pub-dst-ip1",	    required_argument,	NULL, 'F'},
			{"timeout-comid1",		    required_argument,	NULL, 'o'},
			{"timeout-comid2",		    required_argument,	NULL, 'O'},
			{"send-comid1-cycle",	    required_argument,	NULL, 'd'},
			{"send-comid2-cycle",	    required_argument,	NULL, 'e'},
			{"traffic-store-subnet",	required_argument,	NULL, 'T'},
			{"pd-return-ng-counter",	required_argument,	NULL, 'r'},
			{"help",					no_argument,		NULL, 'h'},
			{0, 0, 0, 0}
	};
	INT32 option = 0;				/* getopt_long() result = command option */
	INT32 option_index = 0;		    /* command option index */
	INT32 int32_value;			    /* use variable number in order to get 32bit value */
	UINT16 uint16_value;			/* use variable number in order to get 16bit value */
	UINT32 uint32_value;			/* use variable number in order to get 32bit value */
	INT32 ip[4];					/* use variable number in order to get IP address value */

	while((option = getopt_long(argc, argv,
									"t:1:2:3:4:p:m:c:C:a:b:A:B:f:F:o:O:d:e:T:r:h",
									long_options,
									&option_index)) != -1){
		switch(option)
		{
		case 't':
			/* Get ladderTopologyFlag from an option argument */
			if (strncmp(optarg, "-", 1) != 0)
			{
				sscanf(optarg, "%1d", &int32_value);
				if ((int32_value == TRUE) || (int32_value == FALSE))
				{
					/* Set ladderTopologyFlag */
					ladderTopologyFlag = int32_value;
				}
			}
		break;
		case '1':
			/* Get OFFSET1 from an option argument */
			if (strncmp(optarg, "-", 1) != 0)
			{
				sscanf(optarg, "%hx", &uint16_value);
				if ((uint16_value >= 0) && (uint16_value <= TRAFFIC_STORE_SIZE))
				{
					/* Set OFFSET1 */
					OFFSET_ADDRESS1 = uint16_value;
				}
			}
		break;
		case '2':
			/* Get OFFSET2 from an option argument */
			if (strncmp(optarg, "-", 1) != 0)
			{
				sscanf(optarg, "%hx", &uint16_value);
				if ((uint16_value >= 0) || (uint16_value <= TRAFFIC_STORE_SIZE))
				{
					/* Set OFFSET2 */
					OFFSET_ADDRESS2 = uint16_value;
				}
			}
		break;
		case '3':
			/* Get OFFSET3 from an option argument */
			if (strncmp(optarg, "-", 1) != 0)
			{
				sscanf(optarg, "%hx", &uint16_value);
				if ((uint16_value >= 0) || (uint16_value <= TRAFFIC_STORE_SIZE))
				{
					/* Set OFFSET3 */
					OFFSET_ADDRESS3 = uint16_value;
				}
			}
		break;
		case '4':
			/* Get OFFSET4 from an option argument */
			if (strncmp(optarg, "-", 1) != 0)
			{
				sscanf(optarg, "%hx", &uint16_value);
				if ((uint16_value >= 0) || (uint16_value <= TRAFFIC_STORE_SIZE))
				{
					/* Set OFFSET4 */
					OFFSET_ADDRESS4 = uint16_value;
				}
			}
		break;
			case 'p':
				/* Get publisher application cycle from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%u", &uint32_value);
					/* Set publisher application cycle */
					publisherAppCycle = uint32_value;
				}
			break;
			case 'm':
				/* Get marshallingFlag from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%1d", &int32_value);
					if ((int32_value == TRUE) || (int32_value == FALSE))
					{
						/* Set marshallingFlag */
						marshallingFlag = int32_value;
					}
				}
			break;
			case 'c':
				/* Get PD ComId1 from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%u", &uint32_value);
					/* Set PD ComId1 */
					PD_COMID1 = uint32_value;
				}
			break;
			case 'C':
				/* Get PD ComId2 from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%u", &uint32_value);
					/* Set PD ComId2 */
					PD_COMID2 = uint32_value;
				}
			break;
			case 'a':
				if (strncmp(optarg, "-", 1) != 0)
				{
					/* Get ComId1 Subscribe source IP address comid1 subnet1 from an option argument */
					if (sscanf(optarg, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set source IP address comid1 subnet1 */
//						PD_COMID1_SRC_IP = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						PD_COMID1_SUB_SRC_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set source IP address comid1 subnet2 */
//						PD_COMID1_SRC_IP2 = PD_COMID1_SRC_IP | SUBNET2_NETMASK;
						PD_COMID1_SUB_SRC_IP2 = PD_COMID1_SUB_SRC_IP1 | SUBNET2_NETMASK;
					}
				}
			break;
			case 'b':
				if (strncmp(optarg, "-", 1) != 0)
				{
					/* Get ComId1 Subscribe destination IP address1 from an option argument */
					if (sscanf(optarg, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set destination IP address1 */
//						PD_COMID1_DST_IP = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						PD_COMID1_SUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set destination IP address2 */
						if (vos_isMulticast(PD_COMID1_SUB_DST_IP1))
						{
							/* Multicast Group */
							PD_COMID1_SUB_DST_IP2 = PD_COMID1_SUB_DST_IP1;
						}
						else
						{
							/* Unicast IP Address */
							PD_COMID1_SUB_DST_IP2 = PD_COMID1_SUB_DST_IP1 | SUBNET2_NETMASK;
						}
					}
				}
			break;
			case 'A':
				if (strncmp(optarg, "-", 1) != 0)
				{
					/* Get ComId2 Subscribe source IP address comid2 subnet1 from an option argument */
					if (sscanf(optarg, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set source IP address comid2 subnet1 */
//						PD_COMID2_SRC_IP = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						PD_COMID2_SUB_SRC_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set source IP address comid2 subnet2 */
//						PD_COMID2_SRC_IP2 = PD_COMID2_SRC_IP | SUBNET2_NETMASK;
						PD_COMID2_SUB_SRC_IP2 = PD_COMID2_SUB_SRC_IP1 | SUBNET2_NETMASK;
					}
				}
			break;
			case 'B':
				if (strncmp(optarg, "-", 1) != 0)
				{
					/* Get ComId2 Subscribe destination IP address2 from an option argument */
					if (sscanf(optarg, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4 )
					{
						/* Set destination IP address1 */
//						PD_COMID2_DST_IP = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						PD_COMID2_SUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set destination IP address2 */
						if (vos_isMulticast(PD_COMID2_SUB_DST_IP1))
						{
							/* Multicast Group */
							PD_COMID2_SUB_DST_IP2 = PD_COMID2_SUB_DST_IP1;
						}
						else
						{
							/* Unicast IP Address */
							PD_COMID2_SUB_DST_IP2 = PD_COMID2_SUB_DST_IP1 | SUBNET2_NETMASK;
						}
					}
				}
			break;
			case 'f':
				if (strncmp(optarg, "-", 1) != 0)
				{
					/* Get ComId1 Publish destination IP address1 from an option argument */
					if (sscanf(optarg, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4 )
					{
						/* Set destination IP address1 */
						PD_COMID1_PUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set destination IP address2 */
						if (vos_isMulticast(PD_COMID1_PUB_DST_IP1))
						{
							/* Multicast Group */
							PD_COMID1_PUB_DST_IP2 = PD_COMID1_PUB_DST_IP1;
						}
						else
						{
							/* Unicast IP Address */
							PD_COMID1_PUB_DST_IP2 = PD_COMID1_PUB_DST_IP1 | SUBNET2_NETMASK;
						}
					}
				}
			break;
			case 'F':
				if (strncmp(optarg, "-", 1) != 0)
				{
					/* Get ComId1 Publish destination IP address1 from an option argument */
					if (sscanf(optarg, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4 )
					{
						/* Set destination IP address1 */
						PD_COMID2_PUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set destination IP address2 */
						if (vos_isMulticast(PD_COMID2_PUB_DST_IP1))
						{
							/* Multicast Group */
							PD_COMID2_PUB_DST_IP2 = PD_COMID2_PUB_DST_IP1;
						}
						else
						{
							/* Unicast IP Address */
							PD_COMID2_PUB_DST_IP2 = PD_COMID2_PUB_DST_IP1 | SUBNET2_NETMASK;
						}
					}
				}
			break;
			case 'o':
				/* Get PD ComId1 timeout from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%u", &uint32_value);
					/* Set PD ComId1 timeout */
					PD_COMID1_TIMEOUT = uint32_value;
				}
			break;
			case 'O':
				/* Get PD ComId2 timeout from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%u", &uint32_value);
					/* Set PD ComId2 timeout */
					PD_COMID2_TIMEOUT = uint32_value;
				}
			break;
			case 'd':
				/* Get PD ComId1 send cycle time from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%u", &uint32_value);
					/* Set PD ComId1 send cycle time */
					PD_COMID1_CYCLE = uint32_value;
				}
			break;
			case 'e':
				/* Get PD ComId2 send cycle time from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%u", &uint32_value);
					/* Set PD ComId2 send cycle time */
					PD_COMID2_CYCLE = uint32_value;
				}
			break;
			case 'T':
				/* Get Traffic Store subnet from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%u", &uint32_value);
					/* Set Traffic Store subnet */
					TS_SUBNET = uint32_value;
				}
			break;
			case 'r':
				/* Get PD return NG counter from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%u", &uint32_value);
					/* Set PD return NG counter */
					PD_RETURN_NG_COUNTER = uint32_value;
				}
			break;
			case 'h':
			case '?':
				printf("Unknown or required argument option -%c\n", optopt);
				printf("Usage: COMMAND [-t] [-1] [-2] [-3] [-4] [-p] [-m] [-c] [-C] [-a] [-b] [-A] [-B] [-f] [-F] [-o] [-O] [-d] [-e] [-T] [-r] [-h] \n");
				printf("-t,	--topo			Ladder:1, not Lader:0\n");
				printf("-1,	--offset1		OFFSET1 for Publish val hex: 0xXXXX\n");
				printf("-2,	--offset2		OFFSET2 for Publish val hex: 0xXXXX\n");
				printf("-3,	--offset3		OFFSET3 for Subscribe val hex: 0xXXXX\n");
				printf("-4,	--offset4		OFFSET4 for Subscribe val hex: 0xXXXX\n");
				printf("-p,	--pub-app-cycle		Publisher tlp_put cycle time: micro sec\n");
				printf("-m,	--marshall		Marshall:1, not Marshall:0\n");
				printf("-c,	--publish-comid1	Publish ComId1 val\n");
				printf("-C,	--publish-comid2	Publish ComId2 val\n");
//				printf("-g,	--subscribe-comid1	Subscribe ComId1 val\n");
//				printf("-G,	--subscribe-comid2	Subscribe ComId2 val\n");
				printf("-a,	--comid1-sub-src-ip1	Subscribe ComId1 Source IP Address: xxx.xxx.xxx.xxx\n");
				printf("-b,	--comid1-sub-dst-ip1	Subscribe COmId1 Destination IP Address: xxx.xxx.xxx.xxx\n");
				printf("-A,	--comid2-sub-src-ip1	Subscribe ComId2 Source IP Address: xxx.xxx.xxx.xxx\n");
				printf("-B,	--comid2-sub-dst-ip1	Subscribe COmId2 Destination IP Address: xxx.xxx.xxx.xxx\n");
				printf("-f,	--comid1-pub-dst-ip1	Publish ComId1 Destination IP Address: xxx.xxx.xxx.xxx\n");
				printf("-F,	--comid2-pub-dst-ip1	Publish ComId1 Destination IP Address: xxx.xxx.xxx.xxx\n");
				printf("-o,	--timeout-comid1	Subscribe Timeout: micro sec\n");
				printf("-O,	--timeout-comid2	Subscribe TImeout: micro sec\n");
				printf("-d,	--send-comid1-cycle	Publish Cycle TIme: micro sec\n");
				printf("-e,	--send-comid2-cycle	Publish Cycle TIme: micro sec\n");
				printf("-T,	--traffic-store-subnet	Write Traffic Store Receive Subnet1:1,subnet2:2\n");
				printf("-r,	--pd-return-ng-counter	PD Return NG = Continuation compare NG\n");
				printf("-h,	--help\n");
				return 1;
			break;
			default:
				printf("Unknown or required argument option -%c\n", optopt);
				return 1;
		}
	}

	/* marshall setting */
	if (marshallingFlag == TRUE)
	{
		pRefConMarshallDataset = vos_memAlloc(sizeof(UINT32));

		/* Set dataSet in marshall table */
		err = tau_initMarshall((void *)&pRefConMarshallDataset, usingComIdNumber, gComIdMap, usingDatasetNumber, gDataSets); /* 2nd argument:using ComId Number=2, 4th argument:using DATASET Number=2 */
		if (err != TRDP_NO_ERR)
		{
			printf("tau_initMarshall returns error = %d\n", err);
		   return 1;
		}
	}

	/* Initialize PD DataSet1 */
	dataSet1.boolean = 0;
	dataSet1.character = 1;
	dataSet1.utf16 = 2;
	dataSet1.integer8 = 3;
	dataSet1.integer16 = 4;
	dataSet1.integer32 = 5;
	dataSet1.integer64 = 6;
	dataSet1.uInteger8 = 7;
	dataSet1.uInteger16 = 8;
	dataSet1.uInteger32 = 9;
	dataSet1.uInteger64 = 10;
	dataSet1.real32 = 11;
	dataSet1.real64 = 12;
//	strncpy(dataSet1.string, stringMaxData, sizeof(stringMaxData));
	dataSet1.timeDate32 = 13;
	dataSet1.timeDate48.sec = 14;
	dataSet1.timeDate48.ticks = 15;
	dataSet1.timeDate64.tv_sec = 16;
	dataSet1.timeDate64.tv_usec = 17;

	/* Initialize PD DataSet2 */
	dataSet2.dataset1[0].boolean = 0;
	dataSet2.dataset1[0].character = 1;
	dataSet2.dataset1[0].utf16 = 2;
	dataSet2.dataset1[0].integer8 = 3;
	dataSet2.dataset1[0].integer16 = 4;
	dataSet2.dataset1[0].integer32 = 5;
	dataSet2.dataset1[0].integer64 = 6;
	dataSet2.dataset1[0].uInteger8 = 7;
	dataSet2.dataset1[0].uInteger16 = 8;
	dataSet2.dataset1[0].uInteger32 = 9;
	dataSet2.dataset1[0].uInteger64 = 10;
	dataSet2.dataset1[0].real32 = 11;
	dataSet2.dataset1[0].real64 = 12;
//	strncpy(dataSet2.dataset1[0].string, stringMaxData, sizeof(stringMaxData));
	dataSet2.dataset1[0].timeDate32 = 13;
	dataSet2.dataset1[0].timeDate48.sec = 14;
	dataSet2.dataset1[0].timeDate48.ticks = 15;
	dataSet2.dataset1[0].timeDate64.tv_sec = 16;
	dataSet2.dataset1[0].timeDate64.tv_usec = 17;
	dataSet2.dataset1[1].boolean = 1;
	dataSet2.dataset1[1].character = 127-1;
	dataSet2.dataset1[1].utf16 = 0xFFFFFFFF-1;
	dataSet2.dataset1[1].integer8 = 127-1;
	dataSet2.dataset1[1].integer16 = 32767-1;
	dataSet2.dataset1[1].integer32 = 2147483647-1;
	dataSet2.dataset1[1].integer64 = 9223372036854775807ll-1;
	dataSet2.dataset1[1].uInteger8 = 0xFF-1;
	dataSet2.dataset1[1].uInteger16 = 0xFFFF-1;
	dataSet2.dataset1[1].uInteger32 = 0xFFFFFFFF-1;
	dataSet2.dataset1[1].uInteger64 = 0xFFFFFFFFFFFFFFFFull-1;
//	dataSet2.dataset1[1].real32 = 2147483647-1;
	dataSet2.dataset1[1].real32 = FLT_MAX_EXP - 1;
//	dataSet2.dataset1[1].real64 = 1.79769313486232e308 - 1;
	dataSet2.dataset1[1].real64 = DBL_MAX_EXP - 1;
//	strncpy(dataSet2.dataset1[1].string, stringMaxData, sizeof(stringMaxData));
	dataSet2.dataset1[1].timeDate32 = 2147483647-1;
	dataSet2.dataset1[1].timeDate48.sec = 2147483647-1;
	dataSet2.dataset1[1].timeDate48.ticks = 0xFFFF-1;
	dataSet2.dataset1[1].timeDate64.tv_sec =2147483647-1;
	dataSet2.dataset1[1].timeDate64.tv_usec = 0xFFFFFFFF-1;
	dataSet2.int16[0] = 0-1;
	dataSet2.int16[63] = 63-1;

	/* Set Criterion of the failure PD Return */
	pdDataset1ReturnNgCounter = PD_RETURN_NG_COUNTER;
	pdDataset2ReturnNgCounter = PD_RETURN_NG_COUNTER;
	/* Traffic Store */
	extern UINT8 *pTrafficStoreAddr;				/* pointer to pointer to Traffic Store Address */

	/* Get IP Address */
	struct ifaddrs *ifa_list;
	struct ifaddrs *ifa;
	TRDP_IP_ADDR_T subnetId1Address = 0;
	TRDP_IP_ADDR_T subnetId2Address = 0;
	CHAR8 SUBNETWORK_ID1_IF_NAME[] = "eth0";
	CHAR8 addrStr[256] = {0};

	/* Get I/F address */
	if (getifaddrs(&ifa_list) != VOS_NO_ERR)
	{
    	printf("getifaddrs error. errno=%d\n", errno);
       return 1;
	}
	/* Get All I/F List */
	for(ifa = ifa_list; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (strncmp(ifa->ifa_name, SUBNETWORK_ID1_IF_NAME, sizeof(SUBNETWORK_ID1_IF_NAME)) == 0)
		{
			/* IPv4 */
			if (ifa->ifa_addr->sa_family == AF_INET)
			{
				/* Get Sub-net Id1 Address */
				inet_ntop(AF_INET,
							&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,
							addrStr,
							sizeof(addrStr));
				printf("ip:%s\n",addrStr);
				subnetId1Address = inet_network(addrStr);
				break;
			}
		}
	}
	/* Release memory */
	freeifaddrs(ifa_list);

	/* Sub-network Id1 Init the library for callback operation	(PD only) */
	if (tlc_init(dbgOut,							/* actually printf	*/
				 &dynamicConfig						/* Use application supplied memory	*/
				) != TRDP_NO_ERR)
	{
		printf("Sub-network Initialization error (tlc_init)\n");
		return 1;
	}

	/*	Sub-network Id1 Open a session for callback operation	(PD only) */
	if (marshallingFlag == TRUE)
	{
		pMarshallConfigPtr = &marshallConfig;
	}

	if (tlc_openSession(&appHandle,
							subnetId1Address, subnetId1Address, 	/* Sub-net Id1 IP address/interface	*/
							pMarshallConfigPtr,                   	/* Marshalling or no Marshalling		*/
							&pdConfiguration, NULL,					/* system defaults for PD and MD		*/
							&processConfig) != TRDP_NO_ERR)
	{
		printf("Sub-network Id1 Initialization error (tlc_openSession)\n");
		return 1;
	}

	/* TRDP Ladder support initialize */
	if (trdp_ladder_init() != TRDP_NO_ERR)
	{
		printf("TRDP Ladder Support Initialize failed\n");
		return 1;
	}

	/* Is this Ladder Topology ? */
	if (ladderTopologyFlag == TRUE)
	{
		/* Sub-net Id2 Address */
		subnetId2Address = subnetId1Address | SUBNET2_NETMASK;

		/*	Sub-network Id2 Open a session for callback operation	(PD only) */
		if (tlc_openSession(&appHandle2,
								subnetId2Address, subnetId2Address,	    /* Sub-net Id2 IP address/interface	*/
								pMarshallConfigPtr,                   	/* Marshalling or no Marshalling		*/
								&pdConfiguration2, NULL,				/* system defaults for PD and MD		*/
								&processConfig2) != TRDP_NO_ERR)
		{
			printf("Sub-network Id2 Initialization error (tlc_openSession)\n");
			return 1;
		}
	}

    /*	Sub-network Id1 ComID1 Subscribe */
    err = tlp_subscribe( appHandle,					/* our application identifier */
                         &subHandleNet1ComId1,		/* our subscription identifier */
                         &OFFSET_ADDRESS3,			/* user referece value = offsetAddress */
                         PD_COMID1,                	/* ComID */
                         0,                        	/* topocount: local consist only */
                         PD_COMID1_SUB_SRC_IP1,     	/* Source IP filter */
                         0,                        	/* Source IP filter2 : no used */
                         PD_COMID1_SUB_DST_IP1,     	/* Default destination	(or MC Group) */
                         0,								/* Option */
                         PD_COMID1_TIMEOUT,         /* Time out in us	*/
                         TRDP_TO_SET_TO_ZERO,       /* delete invalid data on timeout */
                         dataSet1Size);         	/* net data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep  Sub-network Id1 pd receive error\n");
        tlc_terminate();
        return 1;
    }

    /* Start PdComLadderThread */
    trdp_setPdComLadderThreadStartFlag(TRUE);

    /*	Sub-network Id1 ComID2 Subscribe */
    err = tlp_subscribe( appHandle,					/* our application identifier */
                         &subHandleNet1ComId2,		/* our subscription identifier */
                         &OFFSET_ADDRESS4,			/* user referece value = offsetAddress */
                         PD_COMID2,                	/* ComID */
                         0,                        	/* topocount: local consist only */
                         PD_COMID2_SUB_SRC_IP1,         	/* Source IP filter */
                         0,                        	/* Source IP filter2 : no used */
                         PD_COMID2_SUB_DST_IP1,        	/* Default destination	(or MC Group) */
                         0,								/* Option */
                         PD_COMID2_TIMEOUT,         /* Time out in us	*/
                         TRDP_TO_SET_TO_ZERO,       /* delete invalid data on timeout */
                         dataSet2Size);       	  	/* net data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep  Sub-network Id1 pd receive error\n");
        tlc_terminate();
        trdp_ladder_terminate();
        return 1;
    }

	/* Is this Ladder Topology ? */
	if (ladderTopologyFlag == TRUE)
	{
		/*	Sub-network Id2 ComID1 Subscribe */
		err = tlp_subscribe( appHandle2,				/* our application identifier */
							 &subHandleNet2ComId1,		/* our subscription identifier */
							 &OFFSET_ADDRESS3,			/* user referece value = offsetAddress */
							 PD_COMID1,                	/* ComID */
							 0,                        	/* topocount: local consist only */
							 PD_COMID1_SUB_SRC_IP2,        	/* Source IP filter */
							 0,                        	/* Source IP filter2 : no used */
							 PD_COMID1_SUB_DST_IP2,        	/* Default destination	(or MC Group) */
							 0,								/* Option */
							 PD_COMID1_TIMEOUT,        	/* Time out in us	*/
							 TRDP_TO_SET_TO_ZERO,      	/* delete invalid data on timeout */
							 dataSet1Size);        		/* net data size */
		if (err != TRDP_NO_ERR)
		{
			printf("prep  Sub-network Id2 pd receive error\n");
			tlc_terminate();
			trdp_ladder_terminate();
			return 1;
		}

		/*	Sub-network Id2 ComID2 Subscribe */
		err = tlp_subscribe( appHandle2,				/* our application identifier */
							 &subHandleNet2ComId2,		/* our subscription identifier */
							 &OFFSET_ADDRESS4,			/* user referece value = offsetAddress */
							 PD_COMID2,                	/* ComID */
							 0,                        	/* topocount: local consist only */
							 PD_COMID2_SUB_SRC_IP2,        	/* Source IP filter */
							 0,                        	/* Source IP filter2 : no used */
							 PD_COMID2_SUB_DST_IP2,        	/* Default destination	(or MC Group) */
							 0,								/* Option */
							 PD_COMID2_TIMEOUT,        	/* Time out in us	*/
							 TRDP_TO_SET_TO_ZERO,      	/* delete invalid data on timeout */
							 dataSet2Size);     		/* net data size */
		if (err != TRDP_NO_ERR)
		{
			printf("prep  Sub-network Id2 pd receive error\n");
			tlc_terminate();
			trdp_ladder_terminate();
			return 1;
		}
	}

    /*	Sub-network Id1 ComID1 Publish */
    err = tlp_publish(  appHandle,						/* our application identifier */
                        &pubHandleNet1ComId1,			/* our pulication identifier */
                        PD_COMID1,						/* ComID to send */
                        0,								/* local consist only */
                        subnetId1Address,				/* default source IP */
                        PD_COMID1_PUB_DST_IP1,				/* where to send to */
                        PD_COMID1_CYCLE,				/* Cycle time in ms */
                        0,								/* not redundant */
                        TRDP_FLAGS_NONE,				/* Don't use callback for errors */
                        NULL,							/* default qos and ttl */
                        (UINT8*)&dataSet1,			    /* initial data */
                        dataSet1Size);					/* data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network Id1 pd publish error\n");
        tlc_terminate();
        trdp_ladder_terminate();
        return 1;
    }

    /*	Sub-network Id1 ComID2 Publish */
    err = tlp_publish(  appHandle,						/* our application identifier */
                        &pubHandleNet1ComId2,			/* our pulication identifier */
                        PD_COMID2,						/* ComID to send */
                        0,								/* local consist only */
                        subnetId1Address,				/* default source IP */
                        PD_COMID2_PUB_DST_IP1,				/* where to send to */
                        PD_COMID2_CYCLE,				/* Cycle time in ms */
                        0,								/* not redundant */
                        TRDP_FLAGS_NONE,				/* Don't use callback for errors */
                        NULL,							/* default qos and ttl */
                        (UINT8*)&dataSet2,			    /* initial data */
                        dataSet2Size);					/* data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network Id1 pd publish error\n");
        tlc_terminate();
        trdp_ladder_terminate();
        return 1;
    }

	/* Is this Ladder Topology ? */
	if (ladderTopologyFlag == TRUE)
	{
    	/*	Sub-network Id2 ComID1 Publish */
		err = tlp_publish(  appHandle2,					/* our application identifier */
							&pubHandleNet2ComId1,		/* our pulication identifier */
							PD_COMID1,					/* ComID to send */
							0,							/* local consist only */
							subnetId2Address,			/* default source IP */
							PD_COMID1_PUB_DST_IP2,			/* where to send to */
							PD_COMID1_CYCLE,			/* Cycle time in ms */
							0,							/* not redundant */
							TRDP_FLAGS_NONE,			/* Don't use callback for errors */
							NULL,						/* default qos and ttl */
							(UINT8*)&dataSet1,			/* initial data */
							dataSet1Size);				/* data size */
		if (err != TRDP_NO_ERR)
		{
			printf("prep Sub-network Id2 pd publish error\n");
			tlc_terminate();
			trdp_ladder_terminate();
			return 1;
		}

		/*	Sub-network Id2 ComID2 Publish */
		err = tlp_publish(  appHandle2,					/* our application identifier */
							&pubHandleNet2ComId2,		/* our pulication identifier */
							PD_COMID2,					/* ComID to send */
							0,							/* local consist only */
							subnetId2Address,			/* default source IP */
							PD_COMID2_PUB_DST_IP2,			/* where to send to */
							PD_COMID2_CYCLE,			/* Cycle time in ms */
							0,							/* not redundant */
							TRDP_FLAGS_NONE,			/* Don't use callback for errors */
							NULL,						/* default qos and ttl */
							 (UINT8*)&dataSet2,			/* initial data */
							dataSet2Size);				/* data size */
		if (err != TRDP_NO_ERR)
		{
			printf("prep Sub-network Id2 pd publish error\n");
			tlc_terminate();
			trdp_ladder_terminate();
			return 1;
		}
	}

    /* Using Sub-Network : TS_SUBNET */
	if (TS_SUBNET == 1)
	{
		TS_SUBNET = SUBNET1;
	}
	else if (TS_SUBNET == 2)
	{
		TS_SUBNET = SUBNET2;
	}
	else
	{
        printf("prep Sub-network error\n");
        return 1;
	}
    err = tlp_setNetworkContext(TS_SUBNET);
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network tlp_setNetworkContext error\n");
        return 1;
    }

/* Wait */
vos_threadDelay(publisherAppCycle);

	/* Display PD Application Version */
	vos_printf(VOS_LOG_INFO,
           "PD Application Version %s: TRDP Setting successfully\n",
           PD_APP_VERSION);

    /*
        Enter the main processing loop.
     */
    while (1)
/*DEBUG    for(i=0; i>=1; i++) */
    {
      	/* Get access right to Traffic Store*/
    	err = tlp_lockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{
    		/* Judge PD DATASET1 Create */
    		if (pdDataset1ReturnNgCounter == PD_RETURN_NG_COUNTER)
    		{
				/* Create PD DataSet1 */
    			dataSet1.boolean++;
				dataSet1.character++;
				dataSet1.utf16++;
				dataSet1.integer8++;
				dataSet1.integer16++;
				dataSet1.integer32++;
				dataSet1.integer64++;
				dataSet1.uInteger8++;
				dataSet1.uInteger16++;
				dataSet1.uInteger32++;
				dataSet1.uInteger64++;
				dataSet1.real32++;
				dataSet1.real64++;
//				strncpy(dataSet1.string, stringMaxData, sizeof(stringMaxData));
				dataSet1.timeDate32++;
				dataSet1.timeDate48.sec++;
				dataSet1.timeDate48.ticks++;
				dataSet1.timeDate64.tv_sec++;
				dataSet1.timeDate64.tv_usec++;
				/* Count up Create PD DATASET */
				pdDataset1CreateCount++;
    		}

			/* Judge PD DATASET2 Create */
    		if (pdDataset2ReturnNgCounter == PD_RETURN_NG_COUNTER)
	 		{
				/* Create PD DataSet2 */
    			dataSet2.dataset1[0].boolean++;
				dataSet2.dataset1[0].character++;
				dataSet2.dataset1[0].utf16++;
				dataSet2.dataset1[0].integer8++;
				dataSet2.dataset1[0].integer16++;
				dataSet2.dataset1[0].integer32++;
				dataSet2.dataset1[0].integer64++;
				dataSet2.dataset1[0].uInteger8++;
				dataSet2.dataset1[0].uInteger16++;
				dataSet2.dataset1[0].uInteger32++;
				dataSet2.dataset1[0].uInteger64++;
				dataSet2.dataset1[0].real32++;
				dataSet2.dataset1[0].real64++;
//				strncpy(dataSet2.dataset1[0].string, stringMaxData, sizeof(stringMaxData));
				dataSet2.dataset1[0].timeDate32++;
				dataSet2.dataset1[0].timeDate48.sec++;
				dataSet2.dataset1[0].timeDate48.ticks++;
				dataSet2.dataset1[0].timeDate64.tv_sec++;
				dataSet2.dataset1[0].timeDate64.tv_usec++;
				dataSet2.dataset1[1].boolean++;
				dataSet2.dataset1[1].character++;
				dataSet2.dataset1[1].utf16++;
				dataSet2.dataset1[1].integer8++;
				dataSet2.dataset1[1].integer16++;
				dataSet2.dataset1[1].integer32++;
				dataSet2.dataset1[1].integer64++;
				dataSet2.dataset1[1].uInteger8++;
				dataSet2.dataset1[1].uInteger16++;
				dataSet2.dataset1[1].uInteger32++;
				dataSet2.dataset1[1].uInteger64++;
				dataSet2.dataset1[1].real32++;
				dataSet2.dataset1[1].real64++;
//				strncpy(dataSet2.dataset1[1].string, stringMaxData, sizeof(stringMaxData));
				dataSet2.dataset1[1].timeDate32++;
				dataSet2.dataset1[1].timeDate48.sec++;
				dataSet2.dataset1[1].timeDate48.ticks++;
				dataSet2.dataset1[1].timeDate64.tv_sec++;
				dataSet2.dataset1[1].timeDate64.tv_usec++;
				dataSet2.int16[0]++;
				dataSet2.int16[63]++;
				/* Count up Create PD DATASET */
				pdDataset2CreateCount++;
    		}

    		/* Set PD DataSet1 in Traffic Store */
    		dataSet1Size = sizeof(dataSet1);
    		memcpy((void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS1), &dataSet1, dataSet1Size);
    		memcpy(&putDataSet1, &dataSet1, dataSet1Size);
    		/* Set DataSet1 in putDataSetBuffer */
			putDataSet1Buffer[inputPosition % PUT_DATASET_BUFFER_SIZE] = putDataSet1;

    		/* Set PD DataSet2 in Traffic Store */
    		dataSet2Size = sizeof(dataSet2);
    		memcpy((void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS2), &dataSet2, dataSet2Size);
    		memcpy(&putDataSet2, &dataSet2, dataSet2Size);
    		/* Set DataSet2 in putDataSetBuffer */
    		putDataSet2Buffer[inputPosition % PUT_DATASET_BUFFER_SIZE] = putDataSet2;
    		/* Set next Buffer area */
    		inputPosition++;

    		/* First TRDP instance in TRDP publish buffer (put DataSet1) */
			tlp_put(appHandle,
					pubHandleNet1ComId1,
					(void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS1),
					dataSet1Size);
			vos_printf(VOS_LOG_DBG, "Ran tlp_put PD DATASET%d subnet1\n", DATASET_NO_1);

    		/* First TRDP instance in TRDP publish buffer (put DataSet2) */
			tlp_put(appHandle,
					pubHandleNet1ComId2,
					(void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS2),
					dataSet2Size);
			vos_printf(VOS_LOG_DBG, "Ran tlp_put PD DATASET%d subnet1\n", DATASET_NO_2);

			/* Is this Ladder Topology ? */
			if (ladderTopologyFlag == TRUE)
			{
				/* Second TRDP instance in TRDP publish buffer (put DatSet1) */
				tlp_put(appHandle2,
						pubHandleNet2ComId1,
						(void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS1),
						dataSet1Size);
				vos_printf(VOS_LOG_DBG, "Ran tlp_put PD DATASET%d subnet2\n", DATASET_NO_1);

				/* Second TRDP instance in TRDP publish buffer (put DatSet2) */
				tlp_put(appHandle2,
						pubHandleNet2ComId2,
						(void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS2),
						dataSet2Size);
				vos_printf(VOS_LOG_DBG, "Ran tlp_put PD DATASET%d subnet2\n", DATASET_NO_2);
			}

			/* Display tlp_put PD DATASET */
			printf("tlp_put PD DATASET1\n");
//    		dumpMemory(&putDataSet1, dataSet1Size);
    		printf("tlp_put PD DATASET2\n");
//    		dumpMemory(&putDataSet2, dataSet2Size);

			/* Release access right to Traffic Store*/
			err = tlp_unlockTrafficStore();
			if (err != TRDP_NO_ERR)
			{
				printf("Release Traffic Store accessibility Failed\n");
			}

	    	/* Waits for a next to Traffic Store put/get cycle */
			vos_threadDelay(publisherAppCycle);

	      	/* Get access right to Traffic Store*/
	    	err = tlp_lockTrafficStore();
	    	if (err == TRDP_NO_ERR)
	    	{
				/* Get Receive PD DataSet from Traffic Store */
/*
	    		memcpy(&getDataSet1, (void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS3), dataSet1Size);
				vos_printf(VOS_LOG_DBG, "Get Traffic Store PD DATASET%d\n", DATASET_NO_1);
				memcpy(&getDataSet2, (void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS4), dataSet2Size);
				vos_printf(VOS_LOG_DBG, "Get Traffic Store PD DATASET%d\n", DATASET_NO_2);
*/
	    		if (marshallingFlag == TRUE)
	    		{
	    			/* unmarshalling ComId1 */
	    			err = tau_unmarshall (pRefConMarshallDataset,
	    									PD_COMID1,
	    									(UINT8 *)((int)pTrafficStoreAddr + OFFSET_ADDRESS3),
	    									(UINT8 *) &getDataSet1,
	    									&dataSet1Size,
	    									NULL);
	    			vos_printf(VOS_LOG_DBG, "Get Traffic Store PD DATASET%d\n", DATASET_NO_1);
	    			if (err != TRDP_NO_ERR)
	    			{
	    		        printf("tau_unmarshall PD DATASET%d returns error %d\n", DATASET_NO_1, err);
	    		        return 1;
	    			}
	    			/* unmarshalling ComId2 */
	    			err = tau_unmarshall (pRefConMarshallDataset,
	    									PD_COMID2,
	    									(UINT8 *)((int)pTrafficStoreAddr + OFFSET_ADDRESS4),
	    									(UINT8 *) &getDataSet2,
	    									&dataSet2Size,
	    									NULL);
	    			vos_printf(VOS_LOG_DBG, "Get Traffic Store PD DATASET%d\n", DATASET_NO_2);
	    			if (err != TRDP_NO_ERR)
	    			{
	    		        printf("tau_unmarshall PD DATASET%d returns error %d\n", DATASET_NO_2, err);
	    		        return 1;
	    			}
	    		}
	    		else
	    		{
	    			memcpy(&getDataSet1, (void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS3), dataSet1Size);
	    			vos_printf(VOS_LOG_DBG, "Get Traffic Store PD DATASET%d\n", DATASET_NO_1);
	    			memcpy(&getDataSet2, (void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS4), dataSet2Size);
	    			vos_printf(VOS_LOG_DBG, "Get Traffic Store PD DATASET%d\n", DATASET_NO_2);
	    		}

				/* Display Get Receive PD DATASET from Traffic Store*/
				printf("Receive PD DATASET1\n");
//	    		dumpMemory(&getDataSet1, dataSet1Size);
	    		printf("Receive PD DATASET2\n");
//	    		dumpMemory(&getDataSet2, dataSet2Size);

				for(i=0; i <= PUT_DATASET_BUFFER_SIZE - 1; i++)
				{
					/* Compare dataset1 Send PD with receive PD */
					if (comparePdDataset(&putDataSet1Buffer[i], &getDataSet1, dataSet1Size, dataSet1Size) ==1)
					{
						if (pdDataset1ReturnNgCounter <= 0)
						{
							/* PD DATASET1 Return NG */
							printf("*** NG *** PD DATASET1 Return Error.\n");
							/* Set Criterion of the failure PD Return */
							pdDataset1ReturnNgCounter = PD_RETURN_NG_COUNTER;
							pdDataset1ReturnFailureCount++;
							printf("PD DATASET1 Create Count = %d\n", pdDataset1CreateCount);
							printf("PD DATASET1 RETURN NG Count = %d\n", pdDataset1ReturnFailureCount);
						}
						else
						{
							/* During a judgement PD DATASET1 Return */
							pdDataset1ReturnNgCounter--;
						}
					}
					else
					{
						/* PD DATASET1 Return OK */
						/* Set Criterion of the failure PD DATASET1 Return */
						pdDataset1ReturnNgCounter = PD_RETURN_NG_COUNTER;
						printf("PD DATASET1 Create Count = %d\n", pdDataset1CreateCount);
						printf("PD DATASET1 RETURN NG Count = %d\n", pdDataset1ReturnFailureCount);
					}

					/* Compare dataset2 Send PD with receive PD */
					if (comparePdDataset(&putDataSet2Buffer[i], &getDataSet2, dataSet2Size, dataSet2Size) == 1)
					{
						if (pdDataset2ReturnNgCounter <= 0)
						{
							/* PD DATASET2 Return NG */
							printf("*** NG *** PD DATASET2 Return Error.\n");
							/* Set Criterion of the failure PD DATASET2 Return */
							pdDataset2ReturnNgCounter = PD_RETURN_NG_COUNTER;
							pdDataset2ReturnFailureCount++;
							printf("PD DATASET2 Create Count = %d\n", pdDataset2CreateCount);
							printf("PD DATASET2 RETURN NG Count = %d\n", pdDataset2ReturnFailureCount);
						}
						else
						{
							/* During a judgement PD DATASET2 Return */
							pdDataset2ReturnNgCounter--;
						}
					}
					else
					{
						/* PD DATASET2 Return OK */
						/* Set Criterion of the failure PD DATASET2 Return */
						pdDataset2ReturnNgCounter = PD_RETURN_NG_COUNTER;
						printf("PD DATASET2 Create Count = %d\n", pdDataset2CreateCount);
						printf("PD DATASET2 RETURN NG Count = %d\n", pdDataset2ReturnFailureCount);
					}
				}

				/* Release access right to Traffic Store*/
				err = tlp_unlockTrafficStore();
				if (err != TRDP_NO_ERR)
				{
					printf("Release Traffic Store accessibility Failed\n");
				}
	    	}
	    	else
	    	{
	    		printf("Get Traffic Store accessibility Failed\n");
	    	}
    	}
    	else
    	{
    		printf("Get Traffic Store accessibility Failed\n");
	    	/* Waits for a next to Traffic Store put/get cycle */
			vos_threadDelay(publisherAppCycle);
    	}
    }   /*	Bottom of while-loop	*/

    /*
     *
     *	We always clean up behind us!
     */

    tlp_unpublish(appHandle, pubHandleNet1ComId1);
    tlp_unsubscribe(appHandle, subHandleNet1ComId1);
	/* Is this Ladder Topology ? */
	if (ladderTopologyFlag == TRUE)
	{
		tlp_unpublish(appHandle2, pubHandleNet2ComId1);
		tlp_unsubscribe(appHandle2, subHandleNet2ComId1);
	}
    tlc_terminate();
    trdp_ladder_terminate();

    tlp_unpublish(appHandle, pubHandleNet1ComId2);
    tlp_unsubscribe(appHandle, subHandleNet1ComId2);
	/* Is this Ladder Topology ? */
	if (ladderTopologyFlag == TRUE)
	{
		tlp_unpublish(appHandle2, pubHandleNet2ComId2);
		tlp_unsubscribe(appHandle2, subHandleNet2ComId2);
	}
    tlc_terminate();

    return rv;
}
#endif /* TRDP_OPTION_LADDER */
