/**********************************************************************************************************************/
/**
 * @file        	ladderApplication.c
 *
 * @brief           Demo ladder application for TRDP
 *
 * @details			TRDP Ladder Topology Support initialize and initial setting, write Traffic Store process data at a fixed cycle
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

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "vos_utils.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "trdp_pdcom.h"
#include "trdp_if_light.h"
#include "trdp_ladder.h"
#include "trdp_ladder_app.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

/* Thread Name */
CHAR8 pdThreadName[] ="PDThread";		/* Thread name is PD Thread. */
/* Thread Stack Size */
const size_t    pdThreadStackSize   = 256 * 1024;
VOS_MUTEX_T pPdApplicationThreadMutex = NULL;					/* pointer to Mutex for PD Application Thread */

/* Sub-network Id1 */
TRDP_APP_SESSION_T  appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
//TRDP_SUB_T          subHandleNet1ComId1;		/*	Sub-network Id1 ComID1 identifier to the subscription	*/
//TRDP_PUB_T          pubHandleNet1ComId1;		/*	Sub-network Id1 ComID2 identifier to the publication	*/
TRDP_ERR_T          err;
TRDP_PD_CONFIG_T    pdConfiguration = {tlp_recvPdDs, NULL, {0, 0}, TRDP_FLAGS_CALLBACK,
                                       10000000, TRDP_TO_SET_TO_ZERO, 20548};
TRDP_MEM_CONFIG_T   dynamicConfig = {NULL, RESERVED_MEMORY, {}};
TRDP_PROCESS_CONFIG_T	processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};

INT32     rv = 0;

/* Sub-network Id2 */
TRDP_APP_SESSION_T  appHandle2;					/*	Sub-network Id2 identifier to the library instance	*/
//TRDP_SUB_T          subHandleNet2ComId1;		/*	Sub-network Id2 ComID1 identifier to the subscription	*/
//TRDP_PUB_T          pubHandleNet2ComId1;		/*	Sub-network Id2 ComID2 identifier to the publication	*/
TRDP_ERR_T          err2;
TRDP_PD_CONFIG_T    pdConfiguration2 = {tlp_recvPdDs, NULL, {0, 0}, TRDP_FLAGS_CALLBACK,
                                       10000000, TRDP_TO_SET_TO_ZERO, 20548};	    /* Sub-network Id2 PDconfiguration */
TRDP_MEM_CONFIG_T   dynamicConfig2 = {NULL, RESERVED_MEMORY, {}};					/* Sub-network Id2 Structure describing memory */
TRDP_PROCESS_CONFIG_T   processConfig2   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};

TRDP_IP_ADDR_T subnetId1Address = 0;
TRDP_IP_ADDR_T subnetId2Address = 0;

INT32     rv2 = 0;
UINT16 OFFSET_ADDRESS1	= 0x1100;				/* offsetAddress comId1 */
UINT16 OFFSET_ADDRESS2	= 0x1180;				/* offsetAddress comId1 */

UINT8   firstPutData[PD_DATA_SIZE_MAX] = "First Put";

/***********************************************************************************************************************
 * DEFINES
 */
#define SUBNET2_NETMASK							0x00002000			/* The netmask for Subnet2 */
#define PDCOM_LADDER_THREAD_DELAY_TIME			10000				/* PDComLadder Thread starting Judge Cycle in us */
#define PDCOM_MULTICAST_GROUPING_DELAY_TIME	10000000			/* PDComLadder Thread starting Wait Time in us */

/* Some sample comId definitions	*/
#define PD_DATA_SIZE_MAX			300					/* PD DATA MAX SIZE */
//#define LADDER_APP_CYCLE			500000				/* Ladder Application data put cycle in us */

/* Subscribe for Sub-network Id1 */
//#define PD_COMID1				10001
//#define PD_COMID2				10002
//#define PD_COMID1_TIMEOUT		1200000
//#define PD_COMID2_TIMEOUT		1200000
#define PD_COMID1_DATA_SIZE		32
//#define PD_COMID1_SRC_IP        0x0A040111		/* Source IP: 10.4.1.17 */
//#define PD_COMID2_SRC_IP        0x0A040111		/* Source IP: 10.4.1.17 */
//#define PD_COMID1_DST_IP        0xe60000C0		/* Destination IP: 230.0.0.192 */
//#define PD_COMID2_DST_IP        0xe60000C0		/* Destination IP: 230.0.0.192 */

/* Subscribe for Sub-network Id2 */
//#define PD_COMID1_SRC_IP2       PD_COMID1_SRC_IP | SUBNET2_NETMASK      /*	Sender's IP: 10.4.33.17		*/
//#define PD_COMID2_SRC_IP2       PD_COMID2_SRC_IP | SUBNET2_NETMASK      /*	Sender's IP: 10.4.33.17		*/

/* Publish for Sub-network Id1 */
//#define PD_COMID1_CYCLE         30000000			/* ComID1 Publish Cycle TIme */
//#define PD_COMID2_CYCLE         30000000			/* ComID2 Publish Cycle TIme */


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
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (int argc, char *argv[])
{
	PD_COMMAND_VALUE *pPdCommandValue = NULL;		/* PD Command Value */

    /* Traffic Store */
//	extern UINT8 *pTrafficStoreAddr;				/* pointer to pointer to Traffic Store Address */

	/* Get PD_COMMAND_VALUE Area */
	pPdCommandValue = (PD_COMMAND_VALUE *)malloc(sizeof(PD_COMMAND_VALUE));
	if (pPdCommandValue == NULL)
	{
		printf("PD_COMMAND_VALUE malloc Err\n");
		return PD_APP_MEM_ERR;
	}
	else
	{
		/* Decide Create Thread */
		err = decideCreatePdThread(argc, argv, pPdCommandValue);
		if (err !=  PD_APP_NO_ERR)
		{
			printf("Decide Create Thread Err\n");
			return PD_APP_THREAD_ERR;
		}
		else
		{
			/* Get Command, Create Application Thread Loop */
			pdCommand_main_proc();
		}
	}
	return 0;
}

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
PD_APP_ERR_TYPE decideCreatePdThread(int argc, char *argv[], PD_COMMAND_VALUE *pPdCommandValue)
{
	static BOOL firstTimeFlag = TRUE;
	PD_APP_ERR_TYPE err = PD_APP_NO_ERR;
	extern VOS_MUTEX_T pPdApplicationThreadMutex;

	/* Thread Parameter */
	PD_THREAD_PARAMETER *pPdThreadParameter = NULL;							/* PdThread Parameter */

	/* Analyze Command */
	err = analyzePdCommand(argc, argv, pPdCommandValue);
	if (err != PD_APP_NO_ERR)
	{
		printf("PD_COMMAND_VALUE Err\n");
		return PD_APP_ERR;
	}

	/* Only the First Time */
	if (firstTimeFlag == TRUE)
	{
		/* TRDP Initialize */
		err = trdp_pdInitialize();
		if (err != PD_APP_NO_ERR)
		{
			printf("TRDP PD Initialize Err\n");
			return 0;
		}

		/* Create PD Application Thread Mutex */
		if (vos_mutexCreate(&pPdApplicationThreadMutex) != VOS_NO_ERR)
		{
			printf("Create PD Application Thread Mutex Err\n");
			return PD_APP_THREAD_ERR;
		}
		firstTimeFlag = FALSE;
		/* Display PD Application Version */
		vos_printf(VOS_LOG_INFO,
	           "PD Application Version %s: TRDP Setting successfully\n",
	           PD_APP_VERSION);
	}

	/* Create PD Thread */
	/* Get Thread Parameter Area */
	pPdThreadParameter = (PD_THREAD_PARAMETER *)malloc(sizeof(PD_THREAD_PARAMETER));
	if (pPdThreadParameter == NULL)
	{
		printf("decideCreatePdThread Err. malloc callerThreadParameter Err\n");
		return PD_APP_MEM_ERR;
	}
	else
	{
		/* Set Thread Parameter */
		pPdThreadParameter->pPdCommandValue = pPdCommandValue;
		/* Lock PD Application Thread Mutex */
		lockPdApplicationThread();
		/* Create PD Thread */
		err = createPdThread(pPdThreadParameter);
		/* UnLock PD Application Thread Mutex */
		unlockPdApplicationThread();
		if (err != PD_APP_NO_ERR)
		{
			printf("Create PD Thread Err\n");
			return PD_APP_THREAD_ERR;
		}
	}
	return PD_APP_NO_ERR;
}

/* Mutex functions */
/**********************************************************************************************************************/
/** Get PD Application Thread accessibility.
 *
 *  @retval         PD_APP_NO_ERR			no error
 *  @retval         PD_APP_MUTEX_ERR		mutex error
 */
PD_APP_ERR_TYPE  lockPdApplicationThread (
    void)
{
	extern VOS_MUTEX_T pPdApplicationThreadMutex;					/* pointer to Mutex for PD Application Thread */

	/* Lock PD Application Thread by Mutex */
	if ((vos_mutexTryLock(pPdApplicationThreadMutex)) != VOS_NO_ERR)
	{
		vos_printf(VOS_LOG_ERROR, "PD Application Thread Mutex Lock failed\n");
		return PD_APP_MUTEX_ERR;
	}
    return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Release PD Application Thread accessibility.
 *
 *  @retval         PD_APP_NO_ERR			no error
 *  @retval         PD_APP_MUTEX_ERR		mutex error
 *
  */
PD_APP_ERR_TYPE  unlockPdApplicationThread (
    void)
{
	extern VOS_MUTEX_T pPdApplicationThreadMutex;		/* pointer to Mutex for PD Application Thread */

	/* UnLock PD Application Thread by Mutex */
	vos_mutexUnlock(pPdApplicationThreadMutex);
	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Create PD Thread
 *
 *  @param[in]		pPdThreadParameter			pointer to PDThread Parameter
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_THREAD_ERR				Thread error
 *
 */
PD_APP_ERR_TYPE createPdThread (PD_THREAD_PARAMETER *pPdThreadParameter)
{
	/* Thread Handle */
	VOS_THREAD_T pdThreadHandle = NULL;			/* PD Thread handle */

	/*  Create MdReceiveManager Thread */
	if (vos_threadCreate(&pdThreadHandle,
							pdThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							pdThreadStackSize,
							(void *)PDApplication,
							(void *)pPdThreadParameter) == VOS_NO_ERR)
	{
		return PD_APP_NO_ERR;
	}
	else
	{
		printf("PD Thread Create Err\n");
		return PD_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** main thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
PD_APP_ERR_TYPE pdCommand_main_proc(void)
{
	PD_APP_ERR_TYPE err = 0;
	int getCommandLength = 0;					/* Input Command Length */
	int i = 0;										/* loop counter */
	UINT8 operand = 0;							/* Input Command Operand Number */
	char getCommand[GET_COMMAND_MAX];			/* Input Command */
	char argvGetCommand[GET_COMMAND_MAX];		/* Input Command for argv */
	static char *argvCommand[100];				/* Command argv */
	int startPoint;								/* Copy Start Point */
	int endPoint;									/* Copy End Point */
	PD_COMMAND_VALUE *pPdCommandValue = NULL;		/* Command Value */

	/* Decide Command */
	for(;;)
	{
		/* Initialize */
		memset(getCommand, 0, sizeof(getCommand));
		memset(argvGetCommand, 0, sizeof(argvGetCommand));
		memset(argvCommand, 0, sizeof(argvCommand));
		operand = 0;
		startPoint = 0;
		endPoint = 0;

		printf("Input Command\n");

		/* Input Command */
		fgets(getCommand, sizeof(getCommand), stdin);

		/* Get Command Length */
		getCommandLength = strlen(getCommand);
		operand++;

		/* Create argvCommand and Operand Number */
		for(i=0; i < getCommandLength; i++)
		{
			/* Check SPACE */
			if(getCommand[i] == SPACE )
			{
				/* Get argvCommand */
				strncpy(&argvGetCommand[startPoint], &getCommand[startPoint], i-startPoint);
				argvCommand[operand] = &argvGetCommand[startPoint];
				startPoint = i+1;
				operand++;
			}
		}

		/* Copy Last Operand */
		strncpy(&argvGetCommand[startPoint], &getCommand[startPoint], getCommandLength-startPoint-1);
		argvCommand[operand] = &argvGetCommand[startPoint];

		/* Get PD_COMMAND_VALUE Area */
		pPdCommandValue = (PD_COMMAND_VALUE *)malloc(sizeof(PD_COMMAND_VALUE));
		if (pPdCommandValue == NULL)
		{
			printf("PD_COMMAND_VALUE malloc Err\n");
			return PD_APP_MEM_ERR;
		}
		else
		{
			/* Decide Create Thread */
			err = decideCreatePdThread(operand+1, argvCommand, pPdCommandValue);
			if (err !=  PD_APP_NO_ERR)
			{
				printf("Decide Create Thread Err\n");
				return PD_APP_THREAD_ERR;
			}
		}
	}
}

/**********************************************************************************************************************/
/** analyze command
 *
 *  @param[in]		void
 *
 *  @retval         PD_APP_NO_ERR			no error
 *  @retval         PD_APP_ERR				error
 */
PD_APP_ERR_TYPE analyzePdCommand(int argc, char *argv[], PD_COMMAND_VALUE *pPdCommandValue)
{
	UINT16 uint16_value = 0;					/* use variable number in order to get 16bit value */
//	INT32 int32_value =0;					/* use variable number in order to get 32bit value */
	UINT32 uint32_value = 0;					/* use variable number in order to get 32bit value */
	INT32 ip[4] = {0};						/* use variable number in order to get IP address value */
	PD_COMMAND_VALUE getPdCommandValue = {0}; 	/* command value */
	INT32 i = 0;

	/* Command analysis */
	for(i=1; i < argc ; i++)
	{
		if (argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
//			case 't':
				/* Get ladderTopologyFlag from an option argument */
//				sscanf(argv[i+1], "%1d", &int32_value);
//				if ((int32_value == TRUE) || (int32_value == FALSE))
//				{
					/* Set ladderTopologyFlag */
//					getPdCommandValue.ladderTopologyFlag = int32_value;
//				}
//			break;
			case '1':
				/* Get OFFSET1 from an option argument */
				sscanf(argv[i+1], "%hx", &uint16_value);
				if ((uint16_value >= 0) && (uint16_value <= TRAFFIC_STORE_SIZE))
				{
					/* Set OFFSET1 */
					getPdCommandValue.OFFSET_ADDRESS1 = uint16_value;
				}
			break;
//			case '2':
				/* Get OFFSET2 from an option argument */
//				sscanf(argv[i+1], "%hx", &uint16_value);
//				if ((uint16_value >= 0) || (uint16_value <= TRAFFIC_STORE_SIZE))
//				{
					/* Set OFFSET2 */
//					getPdCommandValue.OFFSET_ADDRESS2 = uint16_value;
//				}
//			break;
			case '3':
				/* Get OFFSET3 from an option argument */
				sscanf(argv[i+1], "%hx", &uint16_value);
				if ((uint16_value >= 0) || (uint16_value <= TRAFFIC_STORE_SIZE))
				{
					/* Set OFFSET3 */
					getPdCommandValue.OFFSET_ADDRESS3 = uint16_value;
				}
			break;
//			case '4':
				/* Get OFFSET4 from an option argument */
//				sscanf(argv[i+1], "%hx", &uint16_value);
//				if ((uint16_value >= 0) || (uint16_value <= TRAFFIC_STORE_SIZE))
//				{
					/* Set OFFSET4 */
//					getPdCommandValue.OFFSET_ADDRESS4 = uint16_value;
//				}
//			break;
			case 'p':
				/* Get ladder application cycle from an option argument */
				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set ladder application cycle */
				getPdCommandValue.LADDER_APP_CYCLE = uint32_value;
			break;
//			case 'm':
				/* Get marshallingFlag from an option argument */
//				sscanf(argv[i+1], "%1d", &int32_value);
//				if ((int32_value == TRUE) || (int32_value == FALSE))
//				{
					/* Set marshallingFlag */
//					getPdCommandValue.marshallingFlag = int32_value;
//				}
			break;
			case 'c':
				/* Get PD Publish ComId1 from an option argument */
				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set PD ComId1 */
				getPdCommandValue.PD_PUB_COMID1 = uint32_value;
			break;
//			case 'C':
				/* Get PD Publish ComId2 from an option argument */
//				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set PD ComId2 */
//				getPdCommandValue.PD_PUB_COMID2 = uint32_value;
//			break;
			case 'g':
				/* Get PD Subscribe ComId1 from an option argument */
				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set PD ComId1 */
				getPdCommandValue.PD_SUB_COMID1 = uint32_value;
			break;
//			case 'G':
				/* Get PD Subscribe ComId2 from an option argument */
//				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set PD ComId2 */
//				getPdCommandValue.PD_SUB_COMID2 = uint32_value;
//			break;
			case 'a':
				/* Get ComId1 Subscribe source IP address comid1 subnet1 from an option argument */
				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
				{
					/* Set source IP address comid1 subnet1 */
					getPdCommandValue.PD_COMID1_SUB_SRC_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
					/* Set source IP address comid1 subnet2 */
					getPdCommandValue.PD_COMID1_SUB_SRC_IP2 = getPdCommandValue.PD_COMID1_SUB_SRC_IP1 | SUBNET2_NETMASK;
				}
			break;
			case 'b':
				/* Get ComId1 Subscribe destination IP address1 from an option argument */
				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
				{
					/* Set destination IP address1 */
					getPdCommandValue.PD_COMID1_SUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
					/* Set destination IP address2 */
					if (vos_isMulticast(getPdCommandValue.PD_COMID1_SUB_DST_IP1))
					{
						/* Multicast Group */
						getPdCommandValue.PD_COMID1_SUB_DST_IP2 = getPdCommandValue.PD_COMID1_SUB_DST_IP1;
					}
					else
					{
						/* Unicast IP Address */
						getPdCommandValue.PD_COMID1_SUB_DST_IP2 = getPdCommandValue.PD_COMID1_SUB_DST_IP1 | SUBNET2_NETMASK;
					}
				}
			break;
//			case 'A':
				/* Get ComId2 Subscribe source IP address comid2 subnet1 from an option argument */
//				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
//				{
					/* Set source IP address comid2 subnet1 */
//					getPdCommandValue.PD_COMID2_SUB_SRC_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
					/* Set source IP address comid2 subnet2 */
//					getPdCommandValue.PD_COMID2_SUB_SRC_IP2 = getPdCommandValue.PD_COMID2_SUB_SRC_IP1 | SUBNET2_NETMASK;
//				}
//			break;
//			case 'B':
				/* Get ComId2 Subscribe destination IP address2 from an option argument */
//				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4 )
//				{
					/* Set destination IP address1 */
//					getPdCommandValue.PD_COMID2_SUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
					/* Set destination IP address2 */
//					if (vos_isMulticast(getPdCommandValue.PD_COMID2_SUB_DST_IP1))
//					{
						/* Multicast Group */
//						getPdCommandValue.PD_COMID2_SUB_DST_IP2 = getPdCommandValue.PD_COMID2_SUB_DST_IP1;
//					}
//					else
//					{
						/* Unicast IP Address */
//						getPdCommandValue.PD_COMID2_SUB_DST_IP2 = getPdCommandValue.PD_COMID2_SUB_DST_IP1 | SUBNET2_NETMASK;
//					}
//				}
//			break;
			case 'f':
				/* Get ComId1 Publish destination IP address1 from an option argument */
				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4 )
				{
					/* Set destination IP address1 */
					getPdCommandValue.PD_COMID1_PUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
					/* Set destination IP address2 */
					if (vos_isMulticast(getPdCommandValue.PD_COMID1_PUB_DST_IP1))
					{
						/* Multicast Group */
						getPdCommandValue.PD_COMID1_PUB_DST_IP2 = getPdCommandValue.PD_COMID1_PUB_DST_IP1;
					}
					else
					{
						/* Unicast IP Address */
						getPdCommandValue.PD_COMID1_PUB_DST_IP2 = getPdCommandValue.PD_COMID1_PUB_DST_IP1 | SUBNET2_NETMASK;
					}
				}
			break;
//			case 'F':
				/* Get ComId1 Publish destination IP address1 from an option argument */
//				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4 )
//				{
					/* Set destination IP address1 */
//					getPdCommandValue.PD_COMID2_PUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
					/* Set destination IP address2 */
//					if (vos_isMulticast(PD_COMID2_PUB_DST_IP1))
//					{
						/* Multicast Group */
//						getPdCommandValue.PD_COMID2_PUB_DST_IP2 = getPdCommandValue.PD_COMID2_PUB_DST_IP1;
//					}
//					else
//					{
						/* Unicast IP Address */
//						getPdCommandValue.PD_COMID2_PUB_DST_IP2 = getPdCommandValue.PD_COMID2_PUB_DST_IP1 | SUBNET2_NETMASK;
//					}
//				}
//			break;
			case 'o':
				/* Get PD ComId1 Subscribe timeout from an option argument */
				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set PD ComId1 timeout */
				getPdCommandValue.PD_COMID1_TIMEOUT = uint32_value;
			break;
//			case 'O':
				/* Get PD ComId2 Subscribe timeout from an option argument */
//				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set PD ComId2 timeout */
//				getPdCommandValue.PD_COMID2_TIMEOUT = uint32_value;
//			break;
			case 'd':
				/* Get PD ComId1 send cycle time from an option argument */
				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set PD ComId1 send cycle time */
				getPdCommandValue.PD_COMID1_CYCLE = uint32_value;
			break;
//			case 'e':
				/* Get PD ComId2 send cycle time from an option argument */
//				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set PD ComId2 send cycle time */
//				getPdCommandValue.PD_COMID2_CYCLE = uint32_value;
//			break;
			case 'T':
				/* Get Traffic Store subnet from an option argument */
				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set Traffic Store subnet */
				getPdCommandValue.TS_SUBNET = uint32_value;
			break;
			case 'h':
			case '?':
				printf("Unknown or required argument option -%c\n", optopt);
				printf("Usage: COMMAND [-t] [-1] [-2] [-3] [-4] [-p] [-m] [-c] [-C] [-g] [-G] [-a] [-b] [-A] [-B] [-o] [-O] [-d] [-e] [-T] [-r] [-h] \n");
//				printf("-t,	--topo			Ladder:1, not Lader:0\n");
				printf("-1,	--offset1		OFFSET1 for Publish val hex\n");
//				printf("-2,	--offset2		OFFSET2 for Publish val hex\n");
				printf("-3,	--offset3		OFFSET3 for Subscribe val hex\n");
//				printf("-4,	--offset4		OFFSET4 for Subscribe val hex\n");
				printf("-p,	--pub-app-cycle		micro sec\n");
//				printf("-m,	--marshall		Marshall:1, not Marshall:0\n");
				printf("-c,	--publish-comid1		Publish ComId1 val\n");
//				printf("-C,	--publish-comid2		Publish ComId2 val\n");
				printf("-g,	--subscribe-comid1		Subscribe ComId1 val\n");
//				printf("-G,	--subscribe-comid2		Subscribe ComId2 val\n");
				printf("-a,	--comid1-sub-src-ip1		IP Address xxx.xxx.xxx.xxx\n");
				printf("-b,	--comid1-sub-dst-ip1		IP Address xxx.xxx.xxx.xxx\n");
//				printf("-A,	--comid2-sub-src-ip1		IP Address xxx.xxx.xxx.xxx\n");
//				printf("-B,	--comid2-sub-dst-ip1		IP Address xxx.xxx.xxx.xxx\n");
				printf("-f,	--comid1-pub-dst-ip1		IP Address xxx.xxx.xxx.xxx\n");
//				printf("-F,	--comid2-pub-dst-ip1		IP Address xxx.xxx.xxx.xxx\n");
				printf("-o,	--timeout-comid1	Subscribe Timeout micro sec\n");
//				printf("-O,	--timeout-comid2	Subscribe TImeout micro sec\n");
				printf("-d,	--send-comid1-cycle	Publish Cycle TIme micro sec\n");
//				printf("-e,	--send-comid2-cycle	Publish Cycle TIme micro sec\n");
				printf("-T,	--traffic-store-subnet	Subnet1:1,subnet2:2\n");
				printf("-h,	--help\n");
				return PD_APP_NO_ERR;
			break;
			default:
				printf("Unknown or required argument option -%c\n", optopt);
			}
		}
	}

	/* Return Command Value */
	*pPdCommandValue = getPdCommandValue;
	return PD_APP_NO_ERR;
}


/**********************************************************************************************************************/
/** TRDP PD initialization
 *
 *  @retval         PD_APP_NO_ERR		no error
 *  @retval         PD_APP_ERR			some error
 */
PD_APP_ERR_TYPE trdp_pdInitialize (void)
{
	/* Get IP Address */
	struct ifaddrs *ifa_list;
	struct ifaddrs *ifa;
//	TRDP_IP_ADDR_T subnetId1Address = 0;
//	TRDP_IP_ADDR_T subnetId2Address = 0;
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

	/* Sub-net Id2 Address */
	subnetId2Address = subnetId1Address | SUBNET2_NETMASK;

	/* Sub-network Init the library for callback operation	(PD only) */
	if (tlc_init(dbgOut,							/* actually printf	*/
				 &dynamicConfig						/* Use application supplied memory	*/
				) != TRDP_NO_ERR)
	{
		printf("Sub-network Initialization error (tlc_init)\n");
		return PD_APP_ERR;
	}

	/*	Sub-network Id1 Open a session for callback operation	(PD only) */
	if (tlc_openSession(&appHandle,
							subnetId1Address, subnetId1Address,	    /* Sub-net Id1 IP address/interface	*/
							NULL,                   				/* no Marshalling		*/
							&pdConfiguration, NULL,					/* system defaults for PD and MD		*/
							&processConfig) != TRDP_NO_ERR)
	{
		printf("Sub-network Id1 Initialization error (tlc_openSession)\n");
		return PD_APP_ERR;
	}

	/* TRDP Ladder support initialize */
	if (trdp_ladder_init() != TRDP_NO_ERR)
	{
		printf("TRDP Ladder Support Initialize failed\n");
		return PD_APP_ERR;
	}

        /*	Sub-network Id2 Open a session for callback operation	(PD only) */
	if (tlc_openSession(&appHandle2,
							subnetId2Address, subnetId2Address,	    /* Sub-net Id2 IP address/interface	*/
							NULL,				                   	/* no Marshalling		*/
							&pdConfiguration2, NULL,				/* system defaults for PD and MD		*/
							&processConfig2) != TRDP_NO_ERR)
	{
		printf("Sub-network Id2 Initialization error (tlc_openSession)\n");
		return PD_APP_ERR;
	}
	return PD_APP_NO_ERR;
}


/**********************************************************************************************************************/
/** PD Application main
 *
 *  @param[in]		pPDThreadParameter			pointer to PDThread Parameter
 *
 *  @retval         PD_APP_NO_ERR		no error
 *  @retval         PD_APP_ERR			some error
 */
PD_APP_ERR_TYPE PDApplication (PD_THREAD_PARAMETER *pPdThreadParameter)
{
	TRDP_SUB_T          subHandleNet1ComId1;		/*	Sub-network Id1 ComID1 identifier to the subscription	*/
	TRDP_PUB_T          pubHandleNet1ComId1;		/*	Sub-network Id1 ComID2 identifier to the publication	*/
	TRDP_SUB_T          subHandleNet2ComId1;		/*	Sub-network Id2 ComID1 identifier to the subscription	*/
	TRDP_PUB_T          pubHandleNet2ComId1;		/*	Sub-network Id2 ComID2 identifier to the publication	*/

    /* Traffic Store */
	extern UINT8 *pTrafficStoreAddr;				/* pointer to pointer to Traffic Store Address */
	CHAR8 putData[PD_DATA_SIZE_MAX];				/* put DATA */
	INT32 putCounter = 0;							/* put counter */

	/*	Sub-network Id1 Subscribe */
    err = tlp_subscribe( appHandle,															/* our application identifier */
                         &subHandleNet1ComId1,												/* our subscription identifier */
                         &pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS3,			/* user referece value = offsetAddress */
                         pPdThreadParameter->pPdCommandValue->PD_SUB_COMID1,             	/* ComID */
                         0,                        											/* topocount: local consist only */
                         pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_SRC_IP1,		/* Source IP filter */
                         0,                        											/* Source IP filter2 : no used */
                         pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_DST_IP1,		/* Default destination	(or MC Group) */
                         0,																		/* Option */
                         pPdThreadParameter->pPdCommandValue->PD_COMID1_TIMEOUT,			/* Time out in us	*/
                         TRDP_TO_SET_TO_ZERO,       											/* delete invalid data on timeout */
                         PD_DATA_SIZE_MAX);													/* net data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep  Sub-network Id1 pd receive error\n");
        tlc_terminate();
        trdp_ladder_terminate();
        return PD_APP_ERR;
    }

    /* Start PdComLadderThread */
    trdp_setPdComLadderThreadStartFlag(TRUE);

    /*	Sub-network Id2 Subscribe */
    err = tlp_subscribe( appHandle2,															/* our application identifier */
                         &subHandleNet2ComId1,												/* our subscription identifier */
                         &pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS3,			/* user referece value = offsetAddress */
                         pPdThreadParameter->pPdCommandValue->PD_SUB_COMID1,              /* ComID */
                         0,                        											/* topocount: local consist only */
                         pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_SRC_IP2,		/* Source IP filter */
                         0,                        											/* Source IP filter2 : no used */
                         pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_DST_IP2,      /* Default destination	(or MC Group) */
                         0,																		/* Option */
                         pPdThreadParameter->pPdCommandValue->PD_COMID1_TIMEOUT,         /* Time out in us	*/
                         TRDP_TO_SET_TO_ZERO,       											/* delete invalid data on timeout */
                         PD_DATA_SIZE_MAX);    											    /* net data size */

    if (err != TRDP_NO_ERR)
    {
        printf("prep  Sub-network Id2 pd receive error\n");
        tlc_terminate();
        trdp_ladder_terminate();
        return 1;
    }

    /*	Sub-network Id1 Publish */
    err = tlp_publish(  appHandle,															/* our application identifier */
                        &pubHandleNet1ComId1,												/* our pulication identifier */
                        pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1,				/* ComID to send */
                        0,																	/* local consist only */
                        subnetId1Address,													/* default source IP */
                        pPdThreadParameter->pPdCommandValue->PD_COMID1_PUB_DST_IP1,	/* where to send to */
                        pPdThreadParameter->pPdCommandValue->PD_COMID1_CYCLE,			/* Cycle time in ms */
                        0,																	/* not redundant */
                        TRDP_FLAGS_NONE,													/* Don't use callback for errors */
                        NULL,																/* default qos and ttl */
                        firstPutData,														/* initial data */
                        PD_DATA_SIZE_MAX);												/* data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network Id1 pd publish error\n");
        tlc_terminate();
        trdp_ladder_terminate();
        return 1;
    }

    /*	Sub-network Id2 Publish */
    err = tlp_publish(  appHandle2,					    								/* our application identifier */
                        &pubHandleNet2ComId1,												/* our pulication identifier */
                        pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1,				/* ComID to send */
                        0,																	/* local consist only */
                        subnetId2Address,			    									/* default source IP */
                        pPdThreadParameter->pPdCommandValue->PD_COMID1_PUB_DST_IP2,	/* where to send to */
                        pPdThreadParameter->pPdCommandValue->PD_COMID1_CYCLE,			/* Cycle time in ms */
                        0,																	/* not redundant */
                        TRDP_FLAGS_NONE,													/* Don't use callback for errors */
                        NULL,																/* default qos and ttl */
                        firstPutData,														/* initial data */
                        PD_DATA_SIZE_MAX);												/* data size */
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network Id2 pd publish error\n");
        tlc_terminate();
        trdp_ladder_terminate();
        return 1;
    }

    /* Using Sub-Network : SUBNET1 */
    err = tlp_setNetworkContext(SUBNET1);
    if (err != TRDP_NO_ERR)
    {
        printf("prep Sub-network error\n");
        return 1;
    }

    /*
        Enter the main processing loop.
     */
    while (1)
    {
      	/* Get access right to Traffic Store*/
    	err = tlp_lockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{
        	/* Create PD Data */
    		memset(putData, 0, sizeof(putData));
    		sprintf(putData, "Put Counter %d", putCounter++);
    		/* Set PD Data in Traffic Store */
    		memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1), putData, sizeof(putData));

			/* First TRDP instance in TRDP publish buffer */
			tlp_put(appHandle,
					pubHandleNet1ComId1,
					(void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1),
					appHandle->pSndQueue->dataSize);
			/* Second TRDP instance in TRDP publish buffer */
			tlp_put(appHandle2,
					pubHandleNet2ComId1,
					(void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1),
					appHandle2->pSndQueue->dataSize);

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

    	/* Waits for a next creation cycle */
		vos_threadDelay(pPdThreadParameter->pPdCommandValue->LADDER_APP_CYCLE);

    }   /*	Bottom of while-loop	*/

    /*
     *	We always clean up behind us!
     */

    tlp_unpublish(appHandle, pubHandleNet1ComId1);
    tlp_unsubscribe(appHandle, subHandleNet1ComId1);

    tlc_terminate();
    trdp_ladder_terminate();

    tlp_unpublish(appHandle2, pubHandleNet2ComId1);
    tlp_unsubscribe(appHandle2, subHandleNet2ComId1);

    tlc_terminate();

    return rv;
}
#endif /* TRDP_OPTION_LADDER */
