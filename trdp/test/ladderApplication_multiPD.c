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

//PD_COMMAND_VALUE *pFirstPdCommandValue = NULL;		/* First PD Command Value */

UINT32 logCategoryOnOffType = 0x0;						/* 0x0 is disable TRDP vos_printf. for dbgOut */


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
    BOOL logPrintOnFlag = FALSE;	/* FALSE is not print */

    switch(category)
    {
    	case VOS_LOG_ERROR:
			/* logCategoryOnOffType : ERROR */
			if ((logCategoryOnOffType & LOG_CATEGORY_ERROR) == LOG_CATEGORY_ERROR)
			{
				logPrintOnFlag = TRUE;
			}
		break;
    	case VOS_LOG_WARNING:
			/* logCategoryOnOffType : WARNING */
    		if((logCategoryOnOffType & LOG_CATEGORY_WARNING) == LOG_CATEGORY_WARNING)
			{
				logPrintOnFlag = TRUE;
			}
    	break;
    	case VOS_LOG_INFO:
			/* logCategoryOnOffType : INFO */
    		if((logCategoryOnOffType & LOG_CATEGORY_INFO) == LOG_CATEGORY_INFO)
			{
				logPrintOnFlag = TRUE;
			}
    	break;
    	case VOS_LOG_DBG:
			/* logCategoryOnOffType : DEBUG */
			if((logCategoryOnOffType & LOG_CATEGORY_DEBUG) == LOG_CATEGORY_DEBUG)
			{
				logPrintOnFlag = TRUE;
			}
		break;
    	default:
    	break;
    }

    /* Check log Print */
    if (logPrintOnFlag == TRUE)
    {
		printf("%s %s %s:%d %s",
			   pTime,
			   catStr[category],
			   pFile,
			   LineNumber,
			   pMsgStr);
    }
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (int argc, char *argv[])
{
//	PD_COMMAND_VALUE *pPdCommandValue = NULL;		/* PD Command Value */

	/* Display PD Application Version */
	printf("PD Application Version %s: ladderApplication_multiPD Start \n", PD_APP_VERSION);

	/* Get PD_COMMAND_VALUE Area */
	pFirstPdCommandValue = (PD_COMMAND_VALUE *)malloc(sizeof(PD_COMMAND_VALUE));
	if (pFirstPdCommandValue == NULL)
	{
		vos_printf(VOS_LOG_ERROR,"PD_COMMAND_VALUE malloc Err\n");
		return PD_APP_MEM_ERR;
	}
	else
	{
		/* Decide Create Thread */
		err = decideCreatePdThread(argc, argv, pFirstPdCommandValue);
		if (err !=  PD_APP_NO_ERR)
		{
			/* command -h = PD_APP_COMMAND_ERR */
			if (err == PD_APP_COMMAND_ERR)
			{
				/* Get Command, Create Application Thread Loop */
				pdCommand_main_proc();
			}
			else
			{
				/* command err */
				/* Get Command, Create Application Thread Loop */
				pdCommand_main_proc();
			}
		}
		else
		{
			/* command OK */
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
		/* command -h */
		if (err == PD_APP_COMMAND_ERR)
		{
			/* Continue Input Command */
			return PD_APP_COMMAND_ERR;
		}
		else
		{
			/* command err */
			printf("PD_COMMAND_VALUE Err\n");
			return PD_APP_ERR;
		}
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

	/* Search New Command in CommandList */
	err = serachPdCommandValueToCommand (pFirstPdCommandValue, pPdCommandValue);
	if (err== PD_APP_COMMAND_ERR)
	{
		printf("decideCreatePdThread Err. There is already Command Err\n");
		return PD_APP_PARAM_ERR;
	}
	else if (err == PD_APP_PARAM_ERR)
	{
		printf("decideCreatePdThread Err. Not ComId Command Value Err\n");
		return PD_APP_PARAM_ERR;
	}

	/* First Command NG ? */
	if ((pFirstPdCommandValue->PD_PUB_COMID1 == 0)	&& (pFirstPdCommandValue->PD_SUB_COMID1 == 0))
	{
		free(pFirstPdCommandValue);
		pFirstPdCommandValue = NULL;
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
		/* Initialize Thread Parameter */
		memset(pPdThreadParameter, 0, sizeof(PD_THREAD_PARAMETER));
		/* Set Thread Parameter */
		pPdThreadParameter->pPdCommandValue = pPdCommandValue;
		/* set tlp_publish tlp_subscribe */
		err = trdp_pdApplicationInitialize(pPdThreadParameter);
		if (err != PD_APP_NO_ERR)
		{
			printf("decideCreatePdThread Err. trdp_pdApplicationInitialize Err\n");
			return PD_APP_ERR;
		}
		if(pPdThreadParameter->subPubValidFlag == PD_APP_THREAD_NOT_PUBLISH)
		{
			/* not publisher */
			return PD_APP_NO_ERR;
		}
		else
		{
			/* Create PD Thread for publisher */
			err = createPdThread(pPdThreadParameter);
			if (err != PD_APP_NO_ERR)
			{
				printf("Create PD Thread Err\n");
				return PD_APP_THREAD_ERR;
			}
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
		vos_printf(VOS_LOG_ERROR, "PD Thread Create Err\n");
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
			vos_printf(VOS_LOG_ERROR, "PD_COMMAND_VALUE malloc Err\n");
			return PD_APP_MEM_ERR;
		}
		else
		{
			/* Decide Create Thread */
			err = decideCreatePdThread(operand+1, argvCommand, pPdCommandValue);
			if (err !=  PD_APP_NO_ERR)
			{
				/* command -h = PD_APP_COMMAND_ERR */
				if (err == PD_APP_COMMAND_ERR)
				{
					continue;
				}
				else
				{
					/* command err */
					vos_printf(VOS_LOG_ERROR, "Decide Create Thread Err\n");
				}
				free(pPdCommandValue);
				pPdCommandValue = NULL;
			}
			else
			{
				/* Set pPdCommandValue List */
				appendPdComamndValueList(&pFirstPdCommandValue, pPdCommandValue);
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
//				if (argv[i+1] != NULL)
//				{
					/* Get ladderTopologyFlag from an option argument */
	//				sscanf(argv[i+1], "%1d", &int32_value);
	//				if ((int32_value == TRUE) || (int32_value == FALSE))
	//				{
						/* Set ladderTopologyFlag */
	//					getPdCommandValue.ladderTopologyFlag = int32_value;
	//				}
//			}
//			break;
			case '1':
				if (argv[i+1] != NULL)
				{
					/* Get OFFSET1 from an option argument */
					sscanf(argv[i+1], "%hx", &uint16_value);
					if ((uint16_value >= 0) && (uint16_value <= TRAFFIC_STORE_SIZE))
					{
						/* Set OFFSET1 */
						getPdCommandValue.OFFSET_ADDRESS1 = uint16_value;
					}
				}
			break;
//			case '2':
//				if (argv[i+1] != NULL)
//				{
				/* Get OFFSET2 from an option argument */
	//				sscanf(argv[i+1], "%hx", &uint16_value);
	//				if ((uint16_value >= 0) || (uint16_value <= TRAFFIC_STORE_SIZE))
	//				{
						/* Set OFFSET2 */
	//					getPdCommandValue.OFFSET_ADDRESS2 = uint16_value;
	//				}
//			}
//			break;
			case '3':
				if (argv[i+1] != NULL)
				{
					/* Get OFFSET3 from an option argument */
					sscanf(argv[i+1], "%hx", &uint16_value);
					if ((uint16_value >= 0) || (uint16_value <= TRAFFIC_STORE_SIZE))
					{
						/* Set OFFSET3 */
						getPdCommandValue.OFFSET_ADDRESS3 = uint16_value;
					}
				}
			break;
//			case '4':
//				if (argv[i+1] != NULL)
//				{
					/* Get OFFSET4 from an option argument */
	//				sscanf(argv[i+1], "%hx", &uint16_value);
	//				if ((uint16_value >= 0) || (uint16_value <= TRAFFIC_STORE_SIZE))
	//				{
						/* Set OFFSET4 */
	//					getPdCommandValue.OFFSET_ADDRESS4 = uint16_value;
	//				}
//			}
//			break;
			case 'p':
				if (argv[i+1] != NULL)
				{
					/* Get ladder application cycle from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set ladder application cycle */
					getPdCommandValue.LADDER_APP_CYCLE = uint32_value;
				}
			break;
//			case 'm':
//				if (argv[i+1] != NULL)
//				{
					/* Get marshallingFlag from an option argument */
	//				sscanf(argv[i+1], "%1d", &int32_value);
	//				if ((int32_value == TRUE) || (int32_value == FALSE))
	//				{
					/* Set marshallingFlag */
	//					getPdCommandValue.marshallingFlag = int32_value;
	//				}
//			}
			break;
			case 'c':
				if (argv[i+1] != NULL)
				{
					/* Get PD Publish ComId1 from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId1 */
					getPdCommandValue.PD_PUB_COMID1 = uint32_value;
				}
			break;
//			case 'C':
//				if (argv[i+1] != NULL)
//				{
					/* Get PD Publish ComId2 from an option argument */
	//				sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId2 */
	//				getPdCommandValue.PD_PUB_COMID2 = uint32_value;
//			}
//			break;
			case 'g':
				if (argv[i+1] != NULL)
				{
					/* Get PD Subscribe ComId1 from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId1 */
					getPdCommandValue.PD_SUB_COMID1 = uint32_value;
				}
			break;
//			case 'G':
//				if (argv[i+1] != NULL)
//				{
					/* Get PD Subscribe ComId2 from an option argument */
	//				sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId2 */
	//				getPdCommandValue.PD_SUB_COMID2 = uint32_value;
//			}
//			break;
			case 'a':
				if (argv[i+1] != NULL)
				{
					/* Get ComId1 Subscribe source IP address comid1 subnet1 from an option argument */
					if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set source IP address comid1 subnet1 */
						getPdCommandValue.PD_COMID1_SUB_SRC_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set source IP address comid1 subnet2 */
						getPdCommandValue.PD_COMID1_SUB_SRC_IP2 = getPdCommandValue.PD_COMID1_SUB_SRC_IP1 | SUBNET2_NETMASK;
					}
				}
			break;
			case 'b':
				if (argv[i+1] != NULL)
				{
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
				}
			break;
//			case 'A':
//				if (argv[i+1] != NULL)
//				{
					/* Get ComId2 Subscribe source IP address comid2 subnet1 from an option argument */
	//				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
	//				{
						/* Set source IP address comid2 subnet1 */
	//					getPdCommandValue.PD_COMID2_SUB_SRC_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set source IP address comid2 subnet2 */
	//					getPdCommandValue.PD_COMID2_SUB_SRC_IP2 = getPdCommandValue.PD_COMID2_SUB_SRC_IP1 | SUBNET2_NETMASK;
	//				}
//			}
//			break;
//			case 'B':
//				if (argv[i+1] != NULL)
//				{
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
//			}
//			break;
			case 'f':
				if (argv[i+1] != NULL)
				{
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
				}
			break;
//			case 'F':
//				if (argv[i+1] != NULL)
//				{
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
//			}
//			break;
			case 'o':
				if (argv[i+1] != NULL)
				{
					/* Get PD ComId1 Subscribe timeout from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId1 timeout */
					getPdCommandValue.PD_COMID1_TIMEOUT = uint32_value;
				}
			break;
//			case 'O':
//				if (argv[i+1] != NULL)
//				{
					/* Get PD ComId2 Subscribe timeout from an option argument */
	//				sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId2 timeout */
	//				getPdCommandValue.PD_COMID2_TIMEOUT = uint32_value;
//			}
//			break;
			case 'd':
				if (argv[i+1] != NULL)
				{
					/* Get PD ComId1 send cycle time from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId1 send cycle time */
					getPdCommandValue.PD_COMID1_CYCLE = uint32_value;
				}
			break;
//			case 'e':
//				if (argv[i+1] != NULL)
//				{
					/* Get PD ComId2 send cycle time from an option argument */
	//				sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId2 send cycle time */
	//				getPdCommandValue.PD_COMID2_CYCLE = uint32_value;
//			}
//			break;
			case 'T':
				if (argv[i+1] != NULL)
				{
					/* Get Traffic Store subnet from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set Traffic Store subnet */
					getPdCommandValue.TS_SUBNET = uint32_value;
				}
			break;
			case 's':
				if (printPdCommandValue(pFirstPdCommandValue) != PD_APP_NO_ERR)
				{
					printf("PD Command Value Dump Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'S':
				if (printPdStatistics(appHandle) != PD_APP_NO_ERR)
				{
					printf("Application Handle1 PD Statistics Dump Err\n");
				}
				if (printPdStatistics(appHandle2) != PD_APP_NO_ERR)
				{
					printf("Application Handle2 PD Statistics Dump Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'D':
				if (printPdSubscribeResult(pFirstPdCommandValue) != PD_APP_NO_ERR)
				{
					printf("Subscriber Receive Count Dump Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'l':
				if (argv[i+1] != NULL)
				{
					/* Get Log Category OnOff Type from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set MD Log */
					logCategoryOnOffType = uint32_value;
				}
			break;
			case 'h':
			case '?':
				printf("Unknown or required argument option -%c\n", optopt);
				printf("Usage: COMMAND [-1 offset1] [-3 offset3] [-p publisherCycleTiem] [-c publishComid1Number]\n"
						"[-g subscribeComid1] [-a subscribeComid1SorceIP] [-b subscribeComid1DestinationIP] [-f publishComid1DestinationIP]\n"
						"[-o subscribeComid1Timeout] [-d publishComid1CycleTime] [-T writeTrafficStoreSubnetType]\n"
						"[-l logCategoryOnOffType]\n"
						"[-s] [-S] [-D] [-h] \n");
//				printf("-t,	--topo			Ladder:1, not Lader:0\n");
				printf("-1,	--offset1		OFFSET1 for Publish val hex: 0xXXXX\n");
//				printf("-2,	--offset2		OFFSET2 for Publish val hex: 0xXXXX\n");
				printf("-3,	--offset3		OFFSET3 for Subscribe val hex: 0xXXXX\n");
//				printf("-4,	--offset4		OFFSET4 for Subscribe val hex: 0xXXXX\n");
				printf("-p,	--pub-app-cycle		Publisher tlp_put cycle time: micro sec\n");
//				printf("-m,	--marshall		Marshall:1, not Marshall:0\n");
				printf("-c,	--publish-comid1	Publish ComId1 val\n");
//				printf("-C,	--publish-comid2	Publish ComId2 val\n");
				printf("-g,	--subscribe-comid1	Subscribe ComId1 val\n");
//				printf("-G,	--subscribe-comid2	Subscribe ComId2 val\n");
				printf("-a,	--comid1-sub-src-ip1	Subscribe ComId1 Source IP Address: xxx.xxx.xxx.xxx\n");
				printf("-b,	--comid1-sub-dst-ip1	Subscribe ComId1 Destination IP Address: xxx.xxx.xxx.xxx\n");
//				printf("-A,	--comid2-sub-src-ip1	Subscribe ComId2 Source IP Address: xxx.xxx.xxx.xxx\n");
//				printf("-B,	--comid2-sub-dst-ip1	Subscribe COmId2 Destination IP Address: xxx.xxx.xxx.xxx\n");
				printf("-f,	--comid1-pub-dst-ip1	Publish ComId1 Destination IP Address: xxx.xxx.xxx.xxx\n");
//				printf("-F,	--comid2-pub-dst-ip1	Publish ComId1 Destination IP Address: xxx.xxx.xxx.xxx\n");
				printf("-o,	--timeout-comid1	Subscribe Timeout: micro sec\n");
//				printf("-O,	--timeout-comid2	Subscribe TImeout: micro sec\n");
				printf("-d,	--send-comid1-cycle	Publish Cycle TIme: micro sec\n");
//				printf("-e,	--send-comid2-cycle	Publish Cycle TIme: micro sec\n");
				printf("-T,	--traffic-store-subnet	Write Traffic Store Receive Subnet1:1,subnet2:2\n");
				printf("-l,	--log-type-onoff	LOG Category OnOff Type Log On:1, Log Off:0, 0bit:ERROR, 1bit:WARNING, 2bit:INFO, 3bit:DBG\n");
				printf("-s,	--show-set-command	Display Setup Command until now\n");
				printf("-S,	--show-pd-statistics	Display PD Statistics\n");
				printf("-D,	--show-subscribe-result	Display subscribe-result\n");
				printf("-h,	--help\n");
				return PD_APP_COMMAND_ERR;
			break;
			default:
				printf("Unknown or required argument option -%c\n", optopt);
				return PD_APP_PARAM_ERR;
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
		vos_printf(VOS_LOG_ERROR, "getifaddrs error. errno=%d\n", errno);
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
				vos_printf(VOS_LOG_INFO, "ip:%s\n", addrStr);
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
		vos_printf(VOS_LOG_ERROR, "Sub-network Initialization error (tlc_init)\n");
		return PD_APP_ERR;
	}

	/*	Sub-network Id1 Open a session for callback operation	(PD only) */
	if (tlc_openSession(&appHandle,
							subnetId1Address, subnetId1Address,	    /* Sub-net Id1 IP address/interface	*/
							NULL,                   				/* no Marshalling		*/
							&pdConfiguration, NULL,					/* system defaults for PD and MD		*/
							&processConfig) != TRDP_NO_ERR)
	{
		vos_printf(VOS_LOG_ERROR, "Sub-network Id1 Initialization error (tlc_openSession)\n");
		return PD_APP_ERR;
	}

	/* TRDP Ladder support initialize */
	if (trdp_ladder_init() != TRDP_NO_ERR)
	{
		vos_printf(VOS_LOG_ERROR, "TRDP Ladder Support Initialize failed\n");
		return PD_APP_ERR;
	}

        /*	Sub-network Id2 Open a session for callback operation	(PD only) */
	if (tlc_openSession(&appHandle2,
							subnetId2Address, subnetId2Address,	    /* Sub-net Id2 IP address/interface	*/
							NULL,				                   	/* no Marshalling		*/
							&pdConfiguration2, NULL,				/* system defaults for PD and MD		*/
							&processConfig2) != TRDP_NO_ERR)
	{
		vos_printf(VOS_LOG_ERROR, "Sub-network Id2 Initialization error (tlc_openSession)\n");
		return PD_APP_ERR;
	}
	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** TRDP PD Application initialization
 *
 *  @param[in]		pPDThreadParameter			pointer to PDThread Parameter
 *
 *  @retval			PD_APP_NO_ERR					no error
 *  @retval			PD_APP_ERR						some error
 */
PD_APP_ERR_TYPE trdp_pdApplicationInitialize (PD_THREAD_PARAMETER *pPdThreadParameter)
{
    /* Traffic Store */
	extern UINT8 *pTrafficStoreAddr;				/* pointer to pointer to Traffic Store Address */

	UINT8 *pPdDataSet = NULL;						/* pointer to PD DATASET */
	size_t pdDataSetSize = 0;						/* subscirbe or publish DATASET SIZE */

	/*	Sub-network Id1 Subscribe */
	/* Check Subscribe ComId */
	if (pPdThreadParameter->pPdCommandValue->PD_SUB_COMID1 == 0)
	{
		/* Set not tlp_subscreibe flag */
		pPdThreadParameter->subPubValidFlag = PD_APP_THREAD_NOT_SUBSCRIBE;
	}
	else
	{
		/* Get PD DATASET */
		if (pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1 == DATASET1_COMID)
		{
			/* DATASET1 Size */
			pdDataSetSize = sizeof(DATASET1);
		}
		else
		{
			/* DATASET2 Size */
			pdDataSetSize = sizeof(DATASET2);
		}

		err = tlp_subscribe( appHandle,															/* our application identifier */
							 &pPdThreadParameter->subHandleNet1ComId1,												/* our subscription identifier */
							 &pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS3,			/* user referece value = offsetAddress */
							 pPdThreadParameter->pPdCommandValue->PD_SUB_COMID1,             	/* ComID */
							 0,                        											/* topocount: local consist only */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_SRC_IP1,		/* Source IP filter */
							 0,                        											/* Source IP filter2 : no used */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_DST_IP1,		/* Default destination	(or MC Group) */
							 0,																		/* Option */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_TIMEOUT,			/* Time out in us	*/
							 TRDP_TO_SET_TO_ZERO,       											/* delete invalid data on timeout */
							 pdDataSetSize);													/* net data size */
		if (err != TRDP_NO_ERR)
		{
			vos_printf(VOS_LOG_ERROR, "prep  Sub-network Id1 pd receive error\n");
			return PD_APP_ERR;
		}

		/*	Sub-network Id2 Subscribe */
		err = tlp_subscribe( appHandle2,															/* our application identifier */
							 &pPdThreadParameter->subHandleNet2ComId1,												/* our subscription identifier */
							 &pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS3,			/* user referece value = offsetAddress */
							 pPdThreadParameter->pPdCommandValue->PD_SUB_COMID1,              /* ComID */
							 0,                        											/* topocount: local consist only */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_SRC_IP2,		/* Source IP filter */
							 0,                        											/* Source IP filter2 : no used */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_DST_IP2,      /* Default destination	(or MC Group) */
							 0,																		/* Option */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_TIMEOUT,         /* Time out in us	*/
							 TRDP_TO_SET_TO_ZERO,       											/* delete invalid data on timeout */
							 pdDataSetSize);    											    /* net data size */

		if (err != TRDP_NO_ERR)
		{
			vos_printf(VOS_LOG_ERROR, "prep  Sub-network Id2 pd receive error\n");
			return PD_APP_ERR;
		}
	}

	/* Check Publish Destination IP Address */
	if (pPdThreadParameter->pPdCommandValue->PD_COMID1_PUB_DST_IP1 == 0)
	{
		/* Set not tlp_publish flag */
		pPdThreadParameter->subPubValidFlag = pPdThreadParameter->subPubValidFlag | PD_APP_THREAD_NOT_PUBLISH;
	}
	else
	{
		/* Create PD DATASET */
		if (pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1 == DATASET1_COMID)
		{
			/* DATASET1 Size */
			pdDataSetSize = sizeof(DATASET1);
			/* Get PD DATASET1 Area */
			pPdDataSet = (UINT8 *)malloc(pdDataSetSize);
			if (pPdDataSet == NULL)
			{
				vos_printf(VOS_LOG_ERROR, "Create PD DATASET1 ERROR. malloc Err\n");
				return PD_APP_MEM_ERR;
			}
			else
			{
				/* Initialize PD DTASET1 */
				if ((createPdDataSet1(TRUE, (DATASET1 *)pPdDataSet)) != PD_APP_NO_ERR)
				{
					vos_printf(VOS_LOG_ERROR, "Create PD DATASET1 ERROR. Initialize Err\n");
					return PD_APP_ERR;
				}
			}
		}
		else
		{
			/* DATASET1 Size */
			pdDataSetSize = sizeof(DATASET2);
			/* Get PD DATASET2 Area */
			pPdDataSet = (UINT8 *)malloc(pdDataSetSize);
			if (pPdDataSet == NULL)
			{
				vos_printf(VOS_LOG_ERROR, "Create PD DATASET2 ERROR. malloc Err\n");
				return PD_APP_MEM_ERR;
			}
			else
			{
				/* Initialize PD DTASET1 */
				if ((createPdDataSet2(TRUE, (DATASET2 *)pPdDataSet)) != PD_APP_NO_ERR)
				{
					vos_printf(VOS_LOG_ERROR, "Create PD DATASET2 ERROR. Initialize Err\n");
					return PD_APP_ERR;
				}
			}
		}

		/*	Sub-network Id1 Publish */
		err = tlp_publish(  appHandle,															/* our application identifier */
							&pPdThreadParameter->pubHandleNet1ComId1,												/* our pulication identifier */
							pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1,				/* ComID to send */
							0,																	/* local consist only */
							subnetId1Address,													/* default source IP */
							pPdThreadParameter->pPdCommandValue->PD_COMID1_PUB_DST_IP1,	/* where to send to */
							pPdThreadParameter->pPdCommandValue->PD_COMID1_CYCLE,			/* Cycle time in ms */
							0,																	/* not redundant */
							TRDP_FLAGS_NONE,													/* Don't use callback for errors */
							NULL,																/* default qos and ttl */
							pPdDataSet,														/* initial data */
							pdDataSetSize);													/* data size */
		if (err != TRDP_NO_ERR)
		{
			vos_printf(VOS_LOG_ERROR, "prep Sub-network Id1 pd publish error\n");
			return PD_APP_ERR;
		}

		/* Set PD Data in Traffic Store */
		memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1), pPdDataSet, pdDataSetSize);

		/*	Sub-network Id2 Publish */
		err = tlp_publish(  appHandle2,					    								/* our application identifier */
							&pPdThreadParameter->pubHandleNet2ComId1,												/* our pulication identifier */
							pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1,				/* ComID to send */
							0,																	/* local consist only */
							subnetId2Address,			    									/* default source IP */
							pPdThreadParameter->pPdCommandValue->PD_COMID1_PUB_DST_IP2,	/* where to send to */
							pPdThreadParameter->pPdCommandValue->PD_COMID1_CYCLE,			/* Cycle time in ms */
							0,																	/* not redundant */
							TRDP_FLAGS_NONE,													/* Don't use callback for errors */
							NULL,																/* default qos and ttl */
							pPdDataSet,														/* initial data */
							pdDataSetSize);														/* data size */
		if (err != TRDP_NO_ERR)
		{
			vos_printf(VOS_LOG_ERROR, "prep Sub-network Id2 pd publish error\n");
			return PD_APP_ERR;
		}
	}

    /* Using Sub-Network : SUBNET1 */
    err = tlp_setNetworkContext(SUBNET1);
    if (err != TRDP_NO_ERR)
    {
    	vos_printf(VOS_LOG_ERROR, "prep Sub-network error\n");
        return PD_APP_ERR;
    }

	/* Check Not tlp_subscribe and Not tlp_publish */
	if (pPdThreadParameter->subPubValidFlag == (PD_APP_THREAD_NOT_SUB_PUB))
	{
    	return PD_APP_THREAD_ERR;
	}

	/* Start PdComLadderThread */
	trdp_setPdComLadderThreadStartFlag(TRUE);

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
	INT32 putCounter = 0;							/* put counter */

    /*
        Enter the main processing loop.
     */
    while (1)
    {
      	/* Get access right to Traffic Store*/
    	err = tlp_lockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{

#if 0
/* Don't Update PD DATASET for Literal Data */
    		/* Create PD DATASET */
    		if (pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1 == DATASET1_COMID)
    		{
				/* Update PD DTASET1 */
				if ((createPdDataSet1(FALSE, (DATASET1 *)pPdDataSet)) != PD_APP_NO_ERR)
				{
					vos_printf(VOS_LOG_ERROR, "Create PD DATASET1 ERROR. Update Err\n");
					return PD_APP_ERR;
				}
				else
				{
					/* Set DATASET1 Size */
					pdDataSetSize = sizeof(DATASET1);
				}
    		}
    		else
    		{
				/* Update PD DTASET1 */
				if ((createPdDataSet2(FALSE, (DATASET2 *)pPdDataSet)) != PD_APP_NO_ERR)
				{
					vos_printf(VOS_LOG_ERROR, "Create PD DATASET2 ERROR. Update Err\n");
					return PD_APP_ERR;
				}
				else
				{
					/* Set DATASET2 Size */
					pdDataSetSize = sizeof(DATASET2);
				}
    		}

    		/* Set PD Data in Traffic Store */
    		memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1), pPdDataSet, pdDataSetSize);
#endif /* if 0 */

			/* First TRDP instance in TRDP publish buffer */
			tlp_put(appHandle,
					pPdThreadParameter->pubHandleNet1ComId1,
					(void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1),
					appHandle->pSndQueue->dataSize);
			/* Second TRDP instance in TRDP publish buffer */
			tlp_put(appHandle2,
					pPdThreadParameter->pubHandleNet2ComId1,
					(void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1),
					appHandle2->pSndQueue->dataSize);
			/* put count up */
			putCounter++;

          	/* Release access right to Traffic Store*/
    		err = tlp_unlockTrafficStore();
    		if (err != TRDP_NO_ERR)
    		{
    			vos_printf(VOS_LOG_ERROR, "Release Traffic Store accessibility Failed\n");
    		}
    	}
    	else
    	{
    		vos_printf(VOS_LOG_ERROR, "Get Traffic Store accessibility Failed\n");
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

/**********************************************************************************************************************/
/** Append an pdCommandValue at end of List
 *
 *  @param[in]      ppHeadPdCommandValue			pointer to pointer to head of List
 *  @param[in]      pNewPdCommandValue				pointer to pdCommandValue to append
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE appendPdComamndValueList(
		PD_COMMAND_VALUE    * *ppHeadPdCommandValue,
		PD_COMMAND_VALUE    *pNewPdCommandValue)
{
	PD_COMMAND_VALUE *iterPdCommandValue;

    if (ppHeadPdCommandValue == NULL || pNewPdCommandValue == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

    /* Ensure this List is last! */
    pNewPdCommandValue->pNextPdCommandValue = NULL;

    if (*ppHeadPdCommandValue == NULL)
    {
        *ppHeadPdCommandValue = pNewPdCommandValue;
        return PD_APP_NO_ERR;
    }

    for (iterPdCommandValue = *ppHeadPdCommandValue;
    	  iterPdCommandValue->pNextPdCommandValue != NULL;
    	  iterPdCommandValue = iterPdCommandValue->pNextPdCommandValue)
    {
        ;
    }
    iterPdCommandValue->pNextPdCommandValue = pNewPdCommandValue;
    return PD_APP_NO_ERR;
}

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
		PD_COMMAND_VALUE	*pNewPdCommandValue)
{
	PD_COMMAND_VALUE *iterPdCommandValue;

	/* Check Parameter */
    if (pHeadPdCommandValue == NULL
    	|| pNewPdCommandValue == NULL
    	|| (pNewPdCommandValue->PD_SUB_COMID1 == 0 && pNewPdCommandValue->PD_PUB_COMID1 == 0))	/* Check comid:0 */
    {
        return PD_APP_PARAM_ERR;
    }

	/* Check publish comid:10001,10002 */
	if ((pNewPdCommandValue->PD_PUB_COMID1 != 0)
		&& !(pNewPdCommandValue->PD_PUB_COMID1 == DATASET1_COMID)
		&& !(pNewPdCommandValue->PD_PUB_COMID1 == DATASET2_COMID))
	{
		return PD_APP_PARAM_ERR;
	}

    if (pHeadPdCommandValue == pNewPdCommandValue)
    {
    	return PD_APP_NO_ERR;
    }

    for (iterPdCommandValue = pHeadPdCommandValue;
    	  iterPdCommandValue != NULL;
    	  iterPdCommandValue = iterPdCommandValue->pNextPdCommandValue)
    {
        /*  Subscribe Command: We match if src/dst address is zero or matches */
        if ((iterPdCommandValue->PD_SUB_COMID1 == pNewPdCommandValue->PD_SUB_COMID1 && pNewPdCommandValue->PD_SUB_COMID1 != 0 ) &&
            (iterPdCommandValue->PD_COMID1_SUB_SRC_IP1 == 0 || iterPdCommandValue->PD_COMID1_SUB_SRC_IP1 == pNewPdCommandValue->PD_COMID1_SUB_SRC_IP1) &&
            (iterPdCommandValue->PD_COMID1_SUB_DST_IP1 == 0 || iterPdCommandValue->PD_COMID1_SUB_DST_IP1 == pNewPdCommandValue->PD_COMID1_SUB_DST_IP1)
            )
        {
            return PD_APP_COMMAND_ERR;
        }
        /*  Publish Command: We match if dst address is zero or matches */
        else if ((iterPdCommandValue->PD_PUB_COMID1 == pNewPdCommandValue->PD_PUB_COMID1 && pNewPdCommandValue->PD_PUB_COMID1 != 0) &&
        		(iterPdCommandValue->PD_COMID1_PUB_DST_IP1 == 0 || iterPdCommandValue->PD_COMID1_PUB_DST_IP1 == pNewPdCommandValue->PD_COMID1_PUB_DST_IP1)
        		)
        {
        	return PD_APP_COMMAND_ERR;
        }
        else
        {
        	continue;
        }

    }
    return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display PdCommandValue
 *
 *  @param[in]      pHeadPdCommandValue	pointer to head of queue
 *
 *  @retval         != NULL         		pointer to PdCommandValue
 *  @retval         NULL            		No PD PdCommandValue found
 */
PD_APP_ERR_TYPE printPdCommandValue (
		PD_COMMAND_VALUE	*pHeadPdCommandValue)
{
	PD_COMMAND_VALUE *iterPdCommandValue;
	UINT16 pdCommnadValueNumber = 1;
	char strIp[16] = {0};

    if (pHeadPdCommandValue == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

    /* Check Valid First Command */
    /* Publish ComId = 0 && Subscribe ComId = 0 */
    if ((pHeadPdCommandValue->PD_PUB_COMID1 == 0) && (pHeadPdCommandValue->PD_SUB_COMID1 == 0))
    {
    	printf("Valid First PD Command isn't Set up\n");
    	return PD_APP_NO_ERR;
    }

    for (iterPdCommandValue = pHeadPdCommandValue;
    	  iterPdCommandValue != NULL;
    	  iterPdCommandValue = iterPdCommandValue->pNextPdCommandValue)
    {
    	/*  Dump PdCommandValue */
		printf("PD Command Value Thread No.%u\n", pdCommnadValueNumber);
		printf("-1,	OFFSET1 for Publish val hex: 0x%x\n", iterPdCommandValue->OFFSET_ADDRESS1);
		printf("-3,	OFFSET3 for Subscribe val hex: 0x%x\n", iterPdCommandValue->OFFSET_ADDRESS3);
		printf("-p,	Publisher tlp_put cycle time: %u micro sec\n", iterPdCommandValue->LADDER_APP_CYCLE);
		printf("-c,	Publish ComId1: %u\n", iterPdCommandValue->PD_PUB_COMID1);
		printf("-g,	Subscribe ComId1: %u\n", iterPdCommandValue->PD_SUB_COMID1);
		miscIpToString(iterPdCommandValue->PD_COMID1_SUB_SRC_IP1, strIp);
		printf("-a,	Subscribe ComId1 Source IP Address: %s\n", strIp);
		miscIpToString(iterPdCommandValue->PD_COMID1_SUB_DST_IP1, strIp);
		printf("-b,	Subscribe ComId1 Destination IP Address: %s\n", strIp);
		miscIpToString(iterPdCommandValue->PD_COMID1_PUB_DST_IP1, strIp);
		printf("-f,	Publish ComId1 Destination IP Address: %s\n", strIp);
		printf("-o,	Subscribe Timeout: %u micro sec\n", iterPdCommandValue->PD_COMID1_TIMEOUT);
		printf("-d,	Publish Cycle TIme: %u micro sec\n", iterPdCommandValue->PD_COMID1_CYCLE);
		printf("-T,	Write Traffic Store Receive Subnet: %u\n", iterPdCommandValue->TS_SUBNET);
		pdCommnadValueNumber++;
    }
    return PD_APP_NO_ERR;
}

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
		TRDP_APP_SESSION_T  appHandle)
{
	TRDP_ERR_T err;
	TRDP_STATISTICS_T   pdStatistics;
	char strIp[16] = {0};

	if (appHandle == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

	/* Get PD Statistics */
	err = tlc_getStatistics(appHandle, &pdStatistics);
	if (err == TRDP_NO_ERR)
	{
		/*  Dump PD Statistics */
		printf("===   PD Statistics   ===\n");
		miscIpToString(appHandle->realIP, strIp);
		printf("Application Handle RealIP(Network I/F Address): %s\n", strIp);
		printf("Default Timeout in us for PD: %u micro sec\n", pdStatistics.pd.defTimeout);
		printf("Number of subscribed ComId's: %u\n", pdStatistics.pd.numSubs);
		printf("Number of published ComId's: %u\n", pdStatistics.pd.numPub);
		printf("Number of received PD packets with No err: %u\n", pdStatistics.pd.numRcv);
		printf("Number of received PD packets with CRC err: %u\n", pdStatistics.pd.numCrcErr);
		printf("Number of received PD packets with protocol err: %u\n", pdStatistics.pd.numProtErr);
		printf("Number of received PD packets with wrong topo count: %u\n", pdStatistics.pd.numTopoErr);
//		printf("Number of received PD push packets without subscription: %u\n", pdStatistics.pd.numNoSubs);
//		printf("Number of received PD pull packets without publisher: %u\n", pdStatistics.pd.numNoPub);
		printf("Number of PD timeouts: %u\n", pdStatistics.pd.numTimeout);
		printf("Number of sent PD packets: %u\n", pdStatistics.pd.numSend);
	}
	else
	{
		return PD_APP_ERR;
	}
	return PD_APP_NO_ERR;
}

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
		PD_COMMAND_VALUE	*pHeadPdCommandValue)
{
	PD_COMMAND_VALUE *iterPdCommandValue;
	UINT16 pdCommnadValueNumber = 1;
	char strIp[16] = {0};

    if (pHeadPdCommandValue == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

    for (iterPdCommandValue = pHeadPdCommandValue;
    	  iterPdCommandValue != NULL;
    	  iterPdCommandValue = iterPdCommandValue->pNextPdCommandValue)
    {
		/* Check Valid Subscriber */
		/* Subscribe ComId != 0 */
		if (iterPdCommandValue->PD_SUB_COMID1 != 0)
		{
	    	/*  Dump PdCommandValue */
			printf("Subscriber No.%u\n", pdCommnadValueNumber);
			printf("-3,	OFFSET3 for Subscribe val hex: 0x%x\n", iterPdCommandValue->OFFSET_ADDRESS3);
			printf("-g,	Subscribe ComId1: %u\n", iterPdCommandValue->PD_SUB_COMID1);
			miscIpToString(iterPdCommandValue->PD_COMID1_SUB_SRC_IP1, strIp);
			printf("-a,	Subscribe ComId1 Source IP Address: %s\n", strIp);
			miscIpToString(iterPdCommandValue->PD_COMID1_SUB_DST_IP1, strIp);
			printf("-b,	Subscribe ComId1 Destination IP Address: %s\n", strIp);
			printf("-o,	Subscribe Timeout: %u micro sec\n", iterPdCommandValue->PD_COMID1_TIMEOUT);
			printf("Subnet1 Receive PD Count: %u\n", iterPdCommandValue->subnet1ReceiveCount);
			printf("Subnet1 Receive PD Timeout Count: %u\n", iterPdCommandValue->subnet1TimeoutReceiveCount);
			printf("Subnet2 Receive PD Count: %u\n", iterPdCommandValue->subnet2ReceiveCount);
			printf("Subnet2 Receive PD Timeout Count: %u\n", iterPdCommandValue->subnet2TimeoutReceiveCount);
			pdCommnadValueNumber++;
		}
    }

    if (pdCommnadValueNumber == 1 )
    {
    	printf("Valid Subscriber PD Command isn't Set up\n");
    }

    return PD_APP_NO_ERR;
}

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
		DATASET1 *pPdDataSet1)
{
	/* Parameter Check */
	if (pPdDataSet1 == NULL)
	{
		vos_printf(VOS_LOG_ERROR, "create PD DATASET1 error\n");
		return PD_APP_PARAM_ERR;
	}

	if (firstCreateFlag == TRUE)
	{
		/* DATASET1 Zero Clear */
		memset(pPdDataSet1, 0, sizeof(DATASET1));
		/* Set Initial PD DataSet1 */
		pPdDataSet1->boolean = 1;
		pPdDataSet1->character = 2;
		pPdDataSet1->utf16 = 3;
		pPdDataSet1->integer8 = 4;
		pPdDataSet1->integer16 = 5;
		pPdDataSet1->integer32 = 6;
		pPdDataSet1->integer64 = 7;
		pPdDataSet1->uInteger8 = 8;
		pPdDataSet1->uInteger16 = 9;
		pPdDataSet1->uInteger32 = 10;
		pPdDataSet1->uInteger64 = 11;
		pPdDataSet1->real32 = 12;
		pPdDataSet1->real64 = 13;
		pPdDataSet1->timeDate32 = 14;
		pPdDataSet1->timeDate48.sec = 15;
		pPdDataSet1->timeDate48.ticks = 16;
		pPdDataSet1->timeDate64.tv_sec = 17;
		pPdDataSet1->timeDate64.tv_usec = 18;
	}
	else
	{
		/* Increment PD DataSet1 */
		pPdDataSet1->boolean++;
		pPdDataSet1->character++;
		pPdDataSet1->utf16++;
		pPdDataSet1->integer8++;
		pPdDataSet1->integer16++;
		pPdDataSet1->integer32++;
		pPdDataSet1->integer64++;
		pPdDataSet1->uInteger8++;
		pPdDataSet1->uInteger16++;
		pPdDataSet1->uInteger32++;
		pPdDataSet1->uInteger64++;
		pPdDataSet1->real32++;
		pPdDataSet1->real64++;
		pPdDataSet1->timeDate32++;
		pPdDataSet1->timeDate48.sec++;
		pPdDataSet1->timeDate48.ticks++;
		pPdDataSet1->timeDate64.tv_sec++;
		pPdDataSet1->timeDate64.tv_usec++;
	}
	return PD_APP_NO_ERR;
}

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
		DATASET2 *pPdDataSet2)
{
	int i = 0;

	/* Parameter Check */
	if (pPdDataSet2 == NULL)
	{
		vos_printf(VOS_LOG_ERROR, "create PD DATASET2 error\n");
		return PD_APP_PARAM_ERR;
	}

	if (firstCreateFlag == TRUE)
	{
		/* DATASET2 Zero Clear */
		memset(pPdDataSet2, 0, sizeof(DATASET2));
		/* Set Initial PD DataSet2 */
		pPdDataSet2->dataset1[0].boolean = 1;
		pPdDataSet2->dataset1[0].character = 2;
		pPdDataSet2->dataset1[0].utf16 = 3;
		pPdDataSet2->dataset1[0].integer8 = 4;
		pPdDataSet2->dataset1[0].integer16 = 5;
		pPdDataSet2->dataset1[0].integer32 = 6;
		pPdDataSet2->dataset1[0].integer64 = 7;
		pPdDataSet2->dataset1[0].uInteger8 = 8;
		pPdDataSet2->dataset1[0].uInteger16 = 9;
		pPdDataSet2->dataset1[0].uInteger32 = 10;
		pPdDataSet2->dataset1[0].uInteger64 = 11;
		pPdDataSet2->dataset1[0].real32 = 12;
		pPdDataSet2->dataset1[0].real64 = 13;
		pPdDataSet2->dataset1[0].timeDate32 = 14;
		pPdDataSet2->dataset1[0].timeDate48.sec = 15;
		pPdDataSet2->dataset1[0].timeDate48.ticks = 16;
		pPdDataSet2->dataset1[0].timeDate64.tv_sec = 17;
		pPdDataSet2->dataset1[0].timeDate64.tv_usec = 18;

/* Max -1 */
/*		pPdDataSet2->dataset1[1].boolean = 1;
		pPdDataSet2->dataset1[1].character = 127-1;
		pPdDataSet2->dataset1[1].utf16 = 0xFFFFFFFF-1;
		pPdDataSet2->dataset1[1].integer8 = 127-1;
		pPdDataSet2->dataset1[1].integer16 = 32767-1;
		pPdDataSet2->dataset1[1].integer32 = 2147483647-1;
		pPdDataSet2->dataset1[1].integer64 = 9223372036854775807ll-1;
		pPdDataSet2->dataset1[1].uInteger8 = 0xFF-1;
		pPdDataSet2->dataset1[1].uInteger16 = 0xFFFF-1;
		pPdDataSet2->dataset1[1].uInteger32 = 0xFFFFFFFF-1;
		pPdDataSet2->dataset1[1].uInteger64 = 0xFFFFFFFFFFFFFFFFull-1;
		pPdDataSet2->dataset1[1].real32 = FLT_MAX_EXP - 1;
		pPdDataSet2->dataset1[1].real64 = DBL_MAX_EXP - 1;
		pPdDataSet2->dataset1[1].timeDate32 = 2147483647-1;
		pPdDataSet2->dataset1[1].timeDate48.sec = 2147483647-1;
		pPdDataSet2->dataset1[1].timeDate48.ticks = 0xFFFF-1;
		pPdDataSet2->dataset1[1].timeDate64.tv_sec =2147483647-1;
		pPdDataSet2->dataset1[1].timeDate64.tv_usec = 0xFFFFFFFF-1;
*/

		pPdDataSet2->dataset1[1].boolean = 1;
		pPdDataSet2->dataset1[1].character = 2;
		pPdDataSet2->dataset1[1].utf16 = 3;
		pPdDataSet2->dataset1[1].integer8 = 4;
		pPdDataSet2->dataset1[1].integer16 = 5;
		pPdDataSet2->dataset1[1].integer32 = 6;
		pPdDataSet2->dataset1[1].integer64 = 7;
		pPdDataSet2->dataset1[1].uInteger8 = 8;
		pPdDataSet2->dataset1[1].uInteger16 = 9;
		pPdDataSet2->dataset1[1].uInteger32 = 10;
		pPdDataSet2->dataset1[1].uInteger64 = 11;
		pPdDataSet2->dataset1[1].real32 = 12;
		pPdDataSet2->dataset1[1].real64 = 13;
		pPdDataSet2->dataset1[1].timeDate32 = 14;
		pPdDataSet2->dataset1[1].timeDate48.sec = 15;
		pPdDataSet2->dataset1[1].timeDate48.ticks = 16;
		pPdDataSet2->dataset1[1].timeDate64.tv_sec = 17;
		pPdDataSet2->dataset1[1].timeDate64.tv_usec = 18;
		for(i = 0; i < 64; i++)
		{
			pPdDataSet2->int16[i] = i;
		}
	}
	else
	{
		/* Set Initial PD DataSet2 */
		pPdDataSet2->dataset1[0].boolean++;
		pPdDataSet2->dataset1[0].character++;
		pPdDataSet2->dataset1[0].utf16++;
		pPdDataSet2->dataset1[0].integer8++;
		pPdDataSet2->dataset1[0].integer16++;
		pPdDataSet2->dataset1[0].integer32++;
		pPdDataSet2->dataset1[0].integer64++;
		pPdDataSet2->dataset1[0].uInteger8++;
		pPdDataSet2->dataset1[0].uInteger16++;
		pPdDataSet2->dataset1[0].uInteger32++;
		pPdDataSet2->dataset1[0].uInteger64++;
		pPdDataSet2->dataset1[0].real32 ++;
		pPdDataSet2->dataset1[0].real64++;
		pPdDataSet2->dataset1[0].timeDate32++;
		pPdDataSet2->dataset1[0].timeDate48.sec++;
		pPdDataSet2->dataset1[0].timeDate48.ticks++;
		pPdDataSet2->dataset1[0].timeDate64.tv_sec++;
		pPdDataSet2->dataset1[0].timeDate64.tv_usec++;

		pPdDataSet2->dataset1[1].boolean++;
		pPdDataSet2->dataset1[1].character++;
		pPdDataSet2->dataset1[1].utf16++;
		pPdDataSet2->dataset1[1].integer8++;
		pPdDataSet2->dataset1[1].integer16++;
		pPdDataSet2->dataset1[1].integer32++;
		pPdDataSet2->dataset1[1].integer64++;
		pPdDataSet2->dataset1[1].uInteger8++;
		pPdDataSet2->dataset1[1].uInteger16++;
		pPdDataSet2->dataset1[1].uInteger32++;
		pPdDataSet2->dataset1[1].uInteger64++;
		pPdDataSet2->dataset1[1].real32++;
		pPdDataSet2->dataset1[1].real64++;
		pPdDataSet2->dataset1[1].timeDate32++;
		pPdDataSet2->dataset1[1].timeDate48.sec++;
		pPdDataSet2->dataset1[1].timeDate48.ticks++;
		pPdDataSet2->dataset1[1].timeDate64.tv_sec++;
		pPdDataSet2->dataset1[1].timeDate64.tv_usec++;
		for(i = 0; i < 64; i++)
		{
			pPdDataSet2->int16[i]++;
		}
	}
	return PD_APP_NO_ERR;
}

// Convert an IP address to string
char * miscIpToString(int ipAdd, char *strTmp)
{
    int ipVal1 = (ipAdd >> 24) & 0xff;
    int ipVal2 = (ipAdd >> 16) & 0xff;
    int ipVal3 = (ipAdd >>  8) & 0xff;
    int ipVal4 = (ipAdd >>  0) & 0xff;

    int strSize = sprintf(strTmp,"%u.%u.%u.%u", ipVal1, ipVal2, ipVal3, ipVal4);
    strTmp[strSize] = 0;

    return strTmp;
}

#endif /* TRDP_OPTION_LADDER */
