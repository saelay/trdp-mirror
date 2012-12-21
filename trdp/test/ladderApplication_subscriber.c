/**********************************************************************************************************************/
/**
 * @file            ladderApplication_subscriber.c
 *
 * @brief           Demo ladder application for TRDP
 *
 * @details			TRDP Ladder Topology Support PD Transmission Subscriber
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, TOSHIBA
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright TOSHIBA, Japan, 2012.
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
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "vos_utils.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "trdp_pdcom.h"
#include "trdp_utils.h"
#include "trdp_if_light.h"
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
TRDP_ERR_T          err = TRDP_NO_ERR;
/* TRDP_PD_CONFIG_T    pdConfiguration = {tlp_recvPdDs, NULL, {0, 0}, TRDP_FLAGS_NONE,
                                       10000000, TRDP_TO_SET_TO_ZERO, 20548}; */
TRDP_PD_CONFIG_T    pdConfiguration = {tlp_recvPdDs, NULL, {0, 0}, TRDP_FLAGS_CALLBACK,
                                       10000000, TRDP_TO_SET_TO_ZERO, 20548};
TRDP_MEM_CONFIG_T   dynamicConfig = {NULL, RESERVED_MEMORY, {}};
TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};
//TRDP_MARSHALL_CONFIG_T	marshallConfig = {tau_marshall, tau_unmarshall, NULL};	/** Marshaling/unMarshalling configuration	*/

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
TRDP_IP_ADDR_T PD_COMID1_SRC_IP = 0x0A040110;		/* Source IP: 10.4.1.16 */
TRDP_IP_ADDR_T PD_COMID2_SRC_IP = 0x0A040110;		/* Source IP: 10.4.1.16 */
TRDP_IP_ADDR_T PD_COMID1_DST_IP = 0xe60000C0;	    /* multicast Destination IP: 230.0.0.192 */
TRDP_IP_ADDR_T PD_COMID2_DST_IP = 0xe60000C0;	    /* multicast Destination IP: 230.0.0.192 */
//TRDP_IP_ADDR_T PD_COMID1_DST_IP = 0x0A040110;		/* unicast Destination IP: 10.4.1.16 */
//TRDP_IP_ADDR_T PD_COMID2_DST_IP = 0x0A040110;		/* unicast Destination IP: 10.4.1.16 */
//TRDP_IP_ADDR_T PD_COMID1_DST_IP2 = 0x0A040110;	/* unicast Destination IP: 10.4.1.16 */
//TRDP_IP_ADDR_T PD_COMID2_DST_IP2 = 0x0A040110;	/* unicast Destination IP: 10.4.1.16 */
/* Subscribe for Sub-network Id2 */
TRDP_IP_ADDR_T PD_COMID1_SRC_IP2 = 0;
TRDP_IP_ADDR_T PD_COMID2_SRC_IP2 = 0;
/* Publish for Sub-network Id1 */
UINT32 PD_COMID1_CYCLE = 30000000;					/* ComID1 Publish Cycle TIme */
UINT32 PD_COMID2_CYCLE = 30000000;					/* ComID2 Publish Cycle TIme */
/* Ladder Topolpgy enable/disable */
BOOL ladderTopologyFlag = TRUE;						/* Ladder Topology : TURE, Not Ladder Topology : FALSE */
/* OFFSET ADDRESS */
UINT16 OFFSET_ADDRESS1 = 0x1100;					/* offsetAddress comId1 publish */
UINT16 OFFSET_ADDRESS2 = 0x1180;					/* offsetAddress comId2 publish */
UINT16 OFFSET_ADDRESS3 = 0x1300;					/* offsetAddress comId1 subscribe */
UINT16 OFFSET_ADDRESS4 = 0x1380;					/* offsetAddress comId2 subscribe */
/* Marshalling enable/disable */
BOOL marshallingFlag = FALSE;						/* Marshalling Enable : TURE, Marshalling Disable : FALSE */
UINT32	publisherAppCycle = 45000000;				/* Publisher Application cycle in us */
UINT32	subscriberAppCycle = 45000000;				/* Subscriber Application cycle in us */
/* Using Receive Subnet in order to Wirte PD in Traffic Store  */
UINT32 TS_SUBNET =1;

INT32     rv2 = 0;
UINT8   firstPutData[PD_DATA_SIZE_MAX] = "First Put";

TRDP_DATASET_T DATASET1_TYPE =
{
	10001,		/* dataset/com ID */
	0,			/* reserved */
	15,			/* No of elements */
	{			/* TRDP_DATASET_ELEMENT_T [] */
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
					TRDP_CHAR8,		    /* data type < Zero-terminated array of CHAR8, fixed size */
					16					/* No of elements */
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
	10002,		/* dataset/com ID */
	0,			/* reserved */
	2,			/* No of elements */
	{
			{
					10001,		 		/* data type < dataset 10001  */
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
	BOOL ladderTopologyFlag = TRUE;							/* Ladder Topology : TURE, Not Ladder Topology : FALSE */
	PD_COMID1_SRC_IP2 = PD_COMID1_SRC_IP | SUBNET2_NETMASK;	/* Sender's IP: 10.4.33.17 (default) */
	PD_COMID2_SRC_IP2 = PD_COMID2_SRC_IP | SUBNET2_NETMASK;	/* Sender's IP: 10.4.33.17 (default) */

	DATASET1 getDataSet1 = {0};								/* get DataSet1 from Traffic Store */
	DATASET2 getDataSet2;									/* get DataSet2 from Traffic Store */
	memset(&getDataSet2, 0, sizeof(getDataSet2));

	DATASET1 dataSet1 = {0};								/* publish Dataset1 */
	DATASET2 dataSet2;										/* publish Dataset2 */
	size_t dataSet1Size = 0;								/* Dataset1 SIZE */
	size_t dataSet2Size = 0;								/* Dataset2 SIZE */
//	UINT16	initMarshallDatasetCount = 0;					/* Marshall Dataset Setting count */

	memset(&dataSet2, 0, sizeof(dataSet2));
	dataSet1Size = sizeof(dataSet1);
	dataSet2Size = sizeof(dataSet2);

/*	void *pRefConMarshallDataset1;
	void *pRefConMarshallDataset2;
*/
	/* Command analysis */
	struct option long_options[] = {		/* Command Option */
			{"topo",					required_argument,	NULL, 't'},
			{"offset1",				    required_argument,	NULL, '1'},
			{"offset2",				    required_argument,	NULL, '2'},
			{"offset3",				    required_argument,	NULL, '3'},
			{"offset4",				    required_argument,	NULL, '4'},
			{"sub-app-cycle",			required_argument,	NULL, 's'},
			{"marshall",				required_argument,	NULL, 'm'},
			{"comid1",					required_argument,	NULL, 'c'},
			{"comid2",					required_argument,	NULL, 'C'},
			{"src-ip1",				    required_argument,	NULL, 'a'},
			{"dst-ip1",				    required_argument,	NULL, 'b'},
			{"src-ip2",				    required_argument,	NULL, 'A'},
			{"dst-ip2",				    required_argument,	NULL, 'B'},
			{"timeout-comid1",		    required_argument,	NULL, 'o'},
			{"timeout-comid2",		    required_argument,	NULL, 'O'},
			{"send-comid1-cycle",	    required_argument,	NULL, 'd'},
			{"send-comid2-cycle",	    required_argument,	NULL, 'e'},
			{"traffic-store-subnet",	required_argument,	NULL, 'T'},
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
									"t:1:2:3:4:s:m:c:C:a:b:A:B:o:O:d:e:T:h",
									long_options,
									&option_index)) != -1)
	{
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
			case 's':
				/* Get subscriber application cycle from an option argument */
				if (strncmp(optarg, "-", 1) != 0)
				{
					sscanf(optarg, "%u", &uint32_value);
					/* Set subscriber application cycle */
					subscriberAppCycle = uint32_value;
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
					/* Get source IP address comid1 subnet1 from an option argument */
					if (sscanf(optarg, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set source IP address comid1 subnet1 */
						PD_COMID1_SRC_IP = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set source IP address comid1 subnet2 */
						PD_COMID1_SRC_IP2 = PD_COMID1_SRC_IP | SUBNET2_NETMASK;
					}
				}
			break;
			case 'b':
				if (strncmp(optarg, "-", 1) != 0)
				{
					/* Get destination IP address1 from an option argument */
					if (sscanf(optarg, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set destination IP address1 */
						PD_COMID1_DST_IP = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
					}
				}
			break;
			case 'A':
				if (strncmp(optarg, "-", 1) != 0)
				{
					/* Get source IP address comid2 subnet1 from an option argument */
					if (sscanf(optarg, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set source IP address comid2 subnet1 */
						PD_COMID2_SRC_IP = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set source IP address comid2 subnet2 */
						PD_COMID2_SRC_IP2 = PD_COMID2_SRC_IP | SUBNET2_NETMASK;
					}
				}
			break;
			case 'B':
				if (strncmp(optarg, "-", 1) != 0)
				{
					/* Get destination IP address2 from an option argument */
					if (sscanf(optarg, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4 )
					{
						/* Set destination IP address2 */
						PD_COMID2_DST_IP = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
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
			case 'h':
			case '?':
				printf("Unknown or required argument option -%c\n", optopt);
				printf("Usage: COMMAND [-t] [-1] [-2] [-3] [-4] [-s] [-m] [-c] [-C] [-a] [-b] [-A] [-B] [-o] [-O] [-d] [-e] [-T] [-h] \n");
				printf("-t,	--topo			Ladder:1, not Lader:0\n");
				printf("-1,	--offset1		OFFSET1 val hex\n");
				printf("-2,	--offset2		OFFSET2 val hex\n");
				printf("-3,	--offset3		OFFSET3 val hex\n");
				printf("-4,	--offset4		OFFSET4 val hex\n");
				printf("-s,	--sub-app-cycle		micro sec\n");
				printf("-m,	--marshall		Marshall:1, not Marshall:0\n");
				printf("-c,	--comid1		ComId1 val\n");
				printf("-C,	--comid2		ComId2 val\n");
				printf("-a,	--src-ip1		IP Address xxx.xxx.xxx.xxx\n");
				printf("-b,	- dst-ip1		IP Address xxx.xxx.xxx.xxx\n");
				printf("-A,	--src-ip2		IP Address xxx.xxx.xxx.xxx\n");
				printf("-B,	--dst-ip2		IP Address xxx.xxx.xxx.xxx\n");
				printf("-o,	--timeout-comid1	micro sec\n");
				printf("-O,	--timeout-comid2	micro sec\n");
				printf("-d,	--send-comid1-cycle	micro sec\n");
				printf("-e,	--send-comid2-cycle	micro sec\n");
				printf("-T,	--traffic-store-subnet	Subnet1:1,subnet2:2\n");
				printf("-h,	--help\n");
			break;
			default:
				printf("Unknown or required argument option -%c\n", optopt);
		}
	}

	/* Set dataSet in marshall table */
//	initMarshallDatasetCount ++;
//	tau_initMarshall(&pRefConMarshallDataset1, initMarshallDatasetCount, &DATASET1_TYPE);				/* dataSet1 */
//	initMarshallDatasetCount ++;
//	tau_initMarshall(&pRefConMarshallDataset2, initMarshallDatasetCount, &DATASET2_TYPE);				/* dataSet2 */

	/* argument2:ComId Number, argument4:DATASET Number */
/*	err = tau_initMarshall(pRefConMarshallDataset1, 2, gComIdMap, 2, gDataSets);
	if (err != TRDP_NO_ERR)
	{
    	printf("tau_initMarshall returns error = %d\n", err);
       return 1;
	}
*/
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
	if (vos_getIfAddrs(&ifa_list) != VOS_NO_ERR)
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
				printf("IP:%s\n", addrStr);
				subnetId1Address = inet_network(addrStr);
				break;
			}
		}
	}
	/* Release memory */
	vos_freeIfAddrs(ifa_list);

	/* Sub-network Id1 Init the library for callback operation	(PD only) */
	if (tlc_init(dbgOut,								/* actually printf	*/
				 &dynamicConfig						/* Use application supplied memory	*/
				) != TRDP_NO_ERR)
	{
		printf("Sub-network Initialization error (tlc_init)\n");
		return 1;
	}

	/*	Sub-network Id1 Open a session for callback operation	(PD only) */
	if (tlc_openSession(&appHandle,
							subnetId1Address, subnetId1Address,	/* Sub-net Id1 IP address/interface	*/
//							&marshallConfig,                   	/* Marshalling							*/
							NULL,                              	/* no Marshalling							*/
							&pdConfiguration, NULL,					/* system defaults for PD and MD		*/
							&processConfig) != TRDP_NO_ERR)
	{
		printf("Sub-network Id1 Initialization error (tlc_openSession)\n");
		return 1;
	}

	/* Is this Ladder Topology ? */
	if (ladderTopologyFlag == TRUE)
	{
		/* Sub-net Id2 Address */
		subnetId2Address = subnetId1Address | SUBNET2_NETMASK;

		/*	Sub-network Id2 Open a session for callback operation	(PD only) */
		if (tlc_openSession(&appHandle2,
								subnetId2Address, subnetId2Address,	/* Sub-net Id2 IP address/interface	*/
//								&marshallConfig,                     	/* Marshalling							*/
								NULL,                              	/* no Marshalling							*/
								&pdConfiguration2, NULL,					/* system defaults for PD and MD		*/
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
                         PD_COMID1_SRC_IP,         	/* Source IP filter */
                         0,                        	/* Source IP filter2 : no used */
                         PD_COMID1_DST_IP,        	/* Default destination	(or MC Group) */
                         PD_COMID1_TIMEOUT,         	/* Time out in us	*/
                         TRDP_TO_SET_TO_ZERO,       	/* delete invalid data on timeout */
                         dataSet1Size);	         	/* net data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep  Sub-network Id1 pd receive error\n");
        tlc_terminate();
        return 1;
    }

    /*	Sub-network Id1 ComID2 Subscribe */
    err = tlp_subscribe( appHandle,					/* our application identifier */
                         &subHandleNet1ComId2,		/* our subscription identifier */
                         &OFFSET_ADDRESS4,			/* user referece value = offsetAddress */
                         PD_COMID2,                	/* ComID */
                         0,                        	/* topocount: local consist only */
                         PD_COMID2_SRC_IP,         	/* Source IP filter */
                         0,                        	/* Source IP filter2 : no used */
                         PD_COMID2_DST_IP,        	/* Default destination	(or MC Group) */
                         PD_COMID2_TIMEOUT,         	/* Time out in us	*/
                         TRDP_TO_SET_TO_ZERO,       	/* delete invalid data on timeout */
                         dataSet2Size);  	       	/* net data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep  Sub-network Id1 pd receive error\n");
        tlc_terminate();
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
							 PD_COMID1_SRC_IP2,        	/* Source IP filter */
							 0,                        	/* Source IP filter2 : no used */
							 PD_COMID1_DST_IP,        	/* Default destination	(or MC Group) */
							 PD_COMID1_TIMEOUT,        	/* Time out in us	*/
							 TRDP_TO_SET_TO_ZERO,      	/* delete invalid data on timeout */
							 dataSet1Size);	        	/* net data size */
		if (err != TRDP_NO_ERR)
		{
			printf("prep  Sub-network Id2 pd receive error\n");
			tlc_terminate();
			return 1;
		}

		/*	Sub-network Id2 ComID2 Subscribe */
		err = tlp_subscribe( appHandle2,				/* our application identifier */
							 &subHandleNet2ComId2,		/* our subscription identifier */
							 &OFFSET_ADDRESS4,			/* user referece value = offsetAddress */
							 PD_COMID2,                	/* ComID */
							 0,                        	/* topocount: local consist only */
							 PD_COMID2_SRC_IP2,        	/* Source IP filter */
							 0,                        	/* Source IP filter2 : no used */
							 PD_COMID2_DST_IP,        	/* Default destination	(or MC Group) */
							 PD_COMID2_TIMEOUT,        	/* Time out in us	*/
							 TRDP_TO_SET_TO_ZERO,      	/* delete invalid data on timeout */
							 dataSet2Size);        	/* net data size */
		if (err != TRDP_NO_ERR)
		{
			printf("prep  Sub-network Id2 pd receive error\n");
			tlc_terminate();
			return 1;
		}
	}

    /*	Sub-network Id1 ComID1 Publish */
    err = tlp_publish(  appHandle,						/* our application identifier */
                        &pubHandleNet1ComId1,			/* our publication identifier */
                        PD_COMID1,						/* ComID to send */
                        0,								/* local consist only */
                        subnetId1Address,				/* default source IP */
                        PD_COMID1_DST_IP,				/* where to send to */
                        PD_COMID1_CYCLE,				/* Cycle time in ms */
                        0,								/* not redundant */
                        TRDP_FLAGS_NONE,				/* Don't use callback for errors */
                        NULL,							/* default qos and ttl */
                        (UINT8*)&dataSet1,			/* initial data */
                        dataSet1Size,					/* data size */
                        TRUE,							/* substitution : Ladder */
                        OFFSET_ADDRESS1);				/* offsetAddress */
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network Id1 pd publish error\n");
        tlc_terminate();
        return 1;
    }

    /*	Sub-network Id1 ComID2 Publish */
    err = tlp_publish(  appHandle,						/* our application identifier */
                        &pubHandleNet1ComId2,			/* our publication identifier */
                        PD_COMID2,						/* ComID to send */
                        0,								/* local consist only */
                        subnetId1Address,				/* default source IP */
                        PD_COMID2_DST_IP,				/* where to send to */
                        PD_COMID2_CYCLE,				/* Cycle time in ms */
                        0,								/* not redundant */
                        TRDP_FLAGS_NONE,				/* Don't use callback for errors */
                        NULL,							/* default qos and ttl */
                        (UINT8*)&dataSet2,			/* initial data */
                        dataSet2Size,					/* data size */
                        TRUE,							/* substitution : Ladder */
                        OFFSET_ADDRESS2);				/* offsetAddress */
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network Id1 pd publish error\n");
        tlc_terminate();
        return 1;
    }

	/* Is this Ladder Topology ? */
	if (ladderTopologyFlag == TRUE)
	{
    	/*	Sub-network Id2 ComID1 Publish */
		err = tlp_publish(  appHandle2,					/* our application identifier */
							&pubHandleNet2ComId1,		/* our publication identifier */
							PD_COMID1,						/* ComID to send */
							0,								/* local consist only */
							subnetId2Address,				/* default source IP */
							PD_COMID1_DST_IP,				/* where to send to */
							PD_COMID1_CYCLE,				/* Cycle time in ms */
							0,								/* not redundant */
							TRDP_FLAGS_NONE,				/* Don't use callback for errors */
							NULL,							/* default qos and ttl */
							(UINT8*)&dataSet1,			/* initial data */
							dataSet1Size,					/* data size */
							TRUE,							/* substitution : Ladder */
							OFFSET_ADDRESS1);				/* offsetAddress */

		if (err != TRDP_NO_ERR)
		{
			printf("prep Sub-network Id2 pd publish error\n");
			tlc_terminate();
			return 1;
		}

		/*	Sub-network Id2 ComID2 Publish */
		err = tlp_publish(  appHandle2,					/* our application identifier */
							&pubHandleNet2ComId2,		/* our publication identifier */
							PD_COMID2,						/* ComID to send */
							0,								/* local consist only */
							subnetId2Address,				/* default source IP */
							PD_COMID2_DST_IP,				/* where to send to */
							PD_COMID2_CYCLE,				/* Cycle time in ms */
							0,								/* not redundant */
							TRDP_FLAGS_NONE,				/* Don't use callback for errors */
							NULL,							/* default qos and ttl */
							(UINT8*)&dataSet2,			/* initial data */
							dataSet2Size,					/* data size */
							TRUE,							/* substitution : Ladder */
							OFFSET_ADDRESS2);				/* offsetAddress */
		if (err != TRDP_NO_ERR)
		{
			printf("prep Sub-network Id2 pd publish error\n");
			tlc_terminate();
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

    /*
        Enter the main processing loop.
     */
    while (1)
/* DEBUG
    int i;
    for(i=0; i>=1; i++)
*/
    {
      	/* Get access right to Traffic Store*/
    	err = tlp_lockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{
			/* Get Receive PD DataSet from Traffic Store */
			memcpy(&getDataSet1, (void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS3), dataSet1Size);
			vos_printf(VOS_LOG_DBG, "Get Traffic Store PD DATASET%d\n", DATASET_NO_1);
			memcpy(&getDataSet2, (void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS4), dataSet2Size);
			vos_printf(VOS_LOG_DBG, "Get Traffic Store PD DATASET%d\n", DATASET_NO_2);

			/* Display Get Receive PD DATASET from Traffic Store*/
			printf("Receive PD DATASET1\n");
			dumpMemory(&getDataSet1, dataSet1Size);
			printf("Receive PD DATASET2\n");
			dumpMemory(&getDataSet2, dataSet2Size);

    		/* Set PD DataSet1 in Traffic Store */
    		memcpy((void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS1), &getDataSet1, dataSet1Size);
    		/* Set PD DataSet2 in Traffic Store */
    		memcpy((void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS2), &getDataSet2, dataSet2Size);

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
			dumpMemory((void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS1), dataSet1Size);
			printf("tlp_put PD DATASET2\n");
			dumpMemory((void *)((int)pTrafficStoreAddr + OFFSET_ADDRESS2), dataSet2Size);

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

		/* Waits for a next to Traffic Store put/get cycle */
		vos_threadDelay(subscriberAppCycle);
    }   /*	Bottom of while-loop	*/

    /*
     *	We always clean up behind us!
     */

    tlp_unpublish(appHandle, pubHandleNet1ComId1);
    tlp_unsubscribe(appHandle, subHandleNet1ComId1);
	/* Is this Ladder Topology ? */
	if (ladderTopologyFlag == TRUE)
	{
		tlp_unpublish(appHandle, pubHandleNet1ComId2);
		tlp_unsubscribe(appHandle, subHandleNet1ComId2);
	}
    tlc_terminate();

    tlp_unpublish(appHandle2, pubHandleNet2ComId1);
    tlp_unsubscribe(appHandle2, subHandleNet2ComId1);
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
