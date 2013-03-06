/**********************************************************************************************************************/
/**
 * @file	mdTestMain.c
 *
 * @brief	Demo MD ladder application for TRDP
 *
 * @details	TRDP Ladder Topology Support MD Transmission Main
 *
 * @note	Project: TCNOpen TRDP prototype stack
 *
 * @author	Kazumasa Aiba, TOSHIBA
 *
 * @remarks	All rights reserved. Reproduction, modification, use or disclosure
 *		to third parties without express authority is forbidden,
 *		Copyright TOSHIBA, Japan, 2013.
 *
 */

 /* check if the needed functionality is present */
#if (MD_SUPPORT == 1)
/* the needed MD_SUPPORT was granted */
#else
/* #error "This test needs MD_SUPPORT with the value '1'" */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <sys/select.h>
#include <mqueue.h>
#include <time.h>
#include <getopt.h>
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

#include "mdTestApp.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

/* Thread Name */
CHAR8 mdReceiveManagerThreadName[] ="MDReceiveManagerThread";		/* Thread name is MDReceiveManager Thread. */
CHAR8 mdCallerThreadName[] ="MDCallerThread";							/* Thread name is MDCaller Thread. */
CHAR8 mdReplierThreadName[] ="MDReplierThread";						/* Thread name is MDReplier Thread. */
CHAR8 mdLogThreadName[] ="MDLogThread";								/* Thread name is MDlog Thread. */

/* Thread Handle */
//VOS_THREAD_T mdReceiveManagerThreadHandle = NULL;			/* MDReceiveManager Thread handle */
//VOS_THREAD_T mdCallerThreadHandle = NULL;						/* MDCaller Thread handle */
//VOS_THREAD_T mdReplierThreadHandle = NULL;					/* MDReplier Thread handle */
//VOS_THREAD_T mdLogThreadHandle = NULL;							/* MDLog Thread handle */

/* Create Thread Counter */
UINT32 callerThreadNoCount = 0;				/* Counter which Create Caller Thread */
UINT32 replierThreadNoCount = 0;			/* Counter which Create Replier Thread */

/* Message Queue Name Base */
CHAR8 callerThreadName[] ="/caller_mq";			/* Caller Message Queue Name Base */
CHAR8 replierThreadName[] ="/replier_mq";			/* Replier Message Queue Name Base */

COMMAND_VALUE				trdpInitializeParameter = {0};		/* Use to trdp_initialize for MDReceiveManager Thread */
//UINT32						*pMdData = NULL;						/* Create MD DATA */
//UINT32						*pMdDataSize = NULL;					/* Create MD DATA Size */
UINT32						sendMdTransferRequestCounter = 0;	/* Send MD Transfer Request Count */

/* MD DATA for Caller Thread */
UINT8 *pFirstCreateMdData = NULL;									/* pointer to First of Create MD DATA */
UINT32 *pFirstCreateMdDataSize = NULL;								/* pointer to First of Create MD DATA Size */

/* Subnet1 */
TRDP_APP_SESSION_T		appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
TRDP_MD_CONFIG_T			md_config;
TRDP_MEM_CONFIG_T			mem_config;
TRDP_PROCESS_CONFIG_T	processConfig = {"Subnet1", "", 0, 0, TRDP_OPTION_BLOCK};

/* Subnet2 */
TRDP_APP_SESSION_T		appHandle2;				/*	Sub-network Id2 identifier to the library instance	*/
TRDP_MD_CONFIG_T			md_config2;
TRDP_MEM_CONFIG_T			mem_config2;
TRDP_PROCESS_CONFIG_T	processConfig2 = {"Subnet2", "", 0, 0, TRDP_OPTION_BLOCK};

/* URI */
TRDP_URI_USER_T subnetId1URI = "Subnet1URI";		/* Subnet1 Network I/F URI */
TRDP_URI_USER_T subnetId2URI = "Subnet2URI";		/* Subnet2 Network I/F URI */
TRDP_URI_USER_T noneURI = "";						/* URI nothing */

/* Starting Input Command Argument :argv[0] = program name */
CHAR8 firstArgv[50] = {0};
CHAR8 **ppFirstArgv = NULL;

//TRDP_IP_ADDR_T subnetId1Address = 0;		/* Subnet1 Network I/F Address */
//TRDP_IP_ADDR_T subnetId2Address = 0;		/* Subnet2 Network I/F Address */

/* Thread Stack Size */
const size_t    threadStackSize   = 256 * 1024;

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (int argc, char *argv[])
{
	MD_APP_ERR_TYPE err = 0;						/* result */
	COMMAND_VALUE *pCommandValue = NULL;		/* Command Value */

	/* Get COMMAND_VALUE Area */
	pCommandValue = (COMMAND_VALUE *)malloc(sizeof(COMMAND_VALUE));
	if (pCommandValue == NULL)
	{
		printf("COMMAND_VALUE malloc Err\n");
		return MD_APP_MEM_ERR;
	}
	else
	{
		/* Decide Create Thread */
		err = decideCreateThread(argc, argv, pCommandValue);
		if (err !=  MD_APP_NO_ERR)
		{
			printf("Decide Create Thread Err\n");
			return MD_APP_THREAD_ERR;
		}
		else
		{
			/* Get Command, Create Application Thread Loop */
			command_main_proc();
		}
	}
	return 0;
}

/* Command analysis */
MD_APP_ERR_TYPE analyzeCommand(int argc, char *argv[], COMMAND_VALUE *pCommandValue)
{
	static BOOL firstAnalyzeCommand = TRUE;
	INT32 int32_value =0;					/* use variable number in order to get 32bit value */
	UINT32 uint32_value = 0;					/* use variable number in order to get 32bit value */
	INT32 ip[4] = {0};						/* use variable number in order to get IP address value */
	COMMAND_VALUE getCommandValue = {0}; 	/* command value */
	INT32 i = 0;

	/* Back up argv[0](program name) for after next command */
	if (firstAnalyzeCommand == TRUE)
	{
		//strncpy(firstArgv ,argv[0], sizeof(firstArgv));
		ppFirstArgv = &argv[0];
	}

	/* Command analysis */
	for(i=1; i < argc ; i++)
	{
		if (argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
			case 'b':
				/* Get CallerReplierType(Caller or Replier) from an option argument */
				sscanf(argv[i+1], "%1u", &int32_value);
				/* Set CallerReplierType(Caller or Replier) */
				getCommandValue.mdCallerReplierType = int32_value;
				break;
			case 'c':
				/* Get TransportType(UDP or TCP) from an option argument */
				sscanf(argv[i+1], "%1u", &int32_value);
				/* Set TransportType(UDP or TCP) */
				getCommandValue.mdTransportType = int32_value;
				break;
			case 'd':
				/* Get MD Message Kind from an option argument */
				sscanf(argv[i+1], "%1u", &int32_value);
				/* Set MD Message Kind */
				getCommandValue.mdMessageKind = int32_value;
				break;
			case 'e':
				/* Get MD Telegram Type from an option argument */
				sscanf(argv[i+1], "%2u", &int32_value);
				/* Set MD Telegram Type */
				getCommandValue.mdTelegramType = int32_value;
				break;
			case 'f':
				/* Get MD Message Size from an option argument */
				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set MD Message Size */
				getCommandValue.mdMessageSize = uint32_value;
			break;
			case 'g':
				/* Get Destination IP Address from an option argument */
				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
				{
					/* Set Destination Address */
					getCommandValue.mdDestinationAddress = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
				}
				break;
			case 'i':
				/* Get MD dump from an option argument */
				sscanf(argv[i+1], "%1u", &int32_value);
				/* Set MD dump */
				getCommandValue.mdDump = int32_value;
				break;
			case 'j':
				/* Get MD Replier Number from an option argument */
				sscanf(argv[i+1], "%u", &int32_value);
				/* Set MD Replier Number */
				getCommandValue.mdReplierNumber = int32_value;
				break;
			case 'k':
				/* Get MD Request Send Cycle Number from an option argument */
				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set MD Request Send Cycle Number */
				getCommandValue.mdCycleNumber = uint32_value;
				break;
			case 'l':
				/* Get MD Log from an option argument */
				sscanf(argv[i+1], "%1u", &int32_value);
				/* Set MD Log */
				getCommandValue.mdLog = int32_value;
				break;
			case 'm':
				/* Get MD Request Send Cycle Time from an option argument */
				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set MD Request Send Cycle Time */
				getCommandValue.mdCycleTime = uint32_value;
				break;
			case 'n':
				/* Get Ladder TopologyFlag from an option argument */
				sscanf(argv[i+1], "%1d", &int32_value);
				if ((int32_value == TRUE) || (int32_value == FALSE))
				{
					/* Set ladderTopologyFlag */
					getCommandValue.mdLadderTopologyFlag = int32_value;
				}
				break;
			case 'o':
				/* Get MD Reply err type from an option argument */
				sscanf(argv[i+1], "%1u", &int32_value);
				/* Set MD Reply err type */
				getCommandValue.mdReplyErr = int32_value;
				break;
			case 'p':
				/* Get Marshalling Flag from an option argument */
				sscanf(argv[i+1], "%1d", &int32_value);
				if ((int32_value == TRUE) || (int32_value == FALSE))
				{
					/* Set Marshalling Flag */
					getCommandValue.mdMarshallingFlag = int32_value;
				}
				break;
			case 'q':
				/* Get Add Listener ComId from an option argument */
				sscanf(argv[i+1], "%x", &uint32_value);
				/* Set Add Listener ComId */
				getCommandValue.mdAddListenerComId = uint32_value;
				break;
			case 'r':
				/* Get Reply Timeout from an option argument */
				sscanf(argv[i+1], "%u", &uint32_value);
				/* Set Reply Timeout */
				getCommandValue.mdTimeoutReply = uint32_value;
				break;
			case 't':
				/* Get MD Sender Subnet from an option argument */
				sscanf(argv[i+1], "%1u", &int32_value);
				/* Set MD Sender Subnet */
				getCommandValue.mdSendSubnet = int32_value;
				break;
			case 'h':
			case '?':
				printf("Unknown or required argument option -%c\n", optopt);
				printf("Usage: COMMAND [-b] [-c] [-d] [-e] [-f] [-g] [-i] [-j] [-k] [-l] [-m] [-n] [-o] [-p] [-q] [-r] [-t] [-h] \n");
				printf("long option(--) Not Support \n");
				printf("-b,	--md-caller-replier-type		Caller:0, Replier:1\n");
				printf("-c,	--md-transport-type			UDP:0, TCP:1\n");
				printf("-d,	--md-message-kind			Mn:0, Mr-Mp:1\n");
				printf("-e,	--md-telegram-type			Increment:0, Fixed:1-6, Error:7-10\n");
				printf("-f,	--md-message-size			MD Message Size Byte\n");
				printf("-g,	--md-destination-address		IP Address xxx.xxx.xxx.xxx\n");
				printf("-i,	--md-dump				DumpOn:1, DumpOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log\n");
				printf("-j,	--md-replier-number			MD Replier Number\n");
				printf("-k,	--md-cycle-number			MD Request Send Cycle Number\n");
				printf("-l,	--md-log				LogFileOn:1, LogFileOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log\n");
				printf("-m,	--md-cycle-time				MD Request Send Cycle Time micro sec\n");
				printf("-n,	--md-topo				Ladder:1, not Lader:0\n");
				printf("-o,	--md-reply-err				MD Reply Error Type(1-6)\n");
				printf("-p,	--md-marshall				Marshall:1, not Marshall:0\n");
				printf("-q,	--md-listener-comid			Add Listener ComId val\n");
				printf("-r,	--md-timeout-reply			micro sec\n");
				printf("-t,	--md-send-subnet			Subnet1:1,subnet2:2\n");
				printf("-h,	--help\n");
			break;
			default:
				printf("Unknown or required argument option -%c\n", optopt);
			}
		}
	}

	/* Return Command Value */
	*pCommandValue = getCommandValue;
	/* First Command Analyze ? */
	if(firstAnalyzeCommand == TRUE)
	{
		/* Set TRDP Initialize Parameter */
		trdpInitializeParameter = getCommandValue;
	}
	/* Set finishing the first command Analyze */
	firstAnalyzeCommand = FALSE;
	return 0;
}

/** decide MD Transfer Pattern
 *
 *  @param[in,out]		pCommandValue		pointer to Command Value
 *  @param[out]			pMdData			pointer to Create MD DATA
 *  @param[out]			pMdDataSize		pointer to Create MD DATA Size
 *
 *  @retval         0					no error
 *  @retval         -1					error
 */
MD_APP_ERR_TYPE decideMdPattern(COMMAND_VALUE *pCommandValue, UINT8 **ppMdData, UINT32 **ppMdDataSize)
{
	MD_APP_ERR_TYPE err = MD_APP_ERR;		/* MD Application Result Code */
//	UINT32 *pCreateMdDatazSize = NULL;		/* Create MD Data Size */

	/* Set createMdDataFlag : OFF */
	pCommandValue->createMdDataFlag = MD_DATA_CREATE_DISABLE;

	/* Decide Caller or Replier */
	switch(pCommandValue->mdCallerReplierType)
	{
		case CALLER:
			/* Get MD DATA Size Area */
			*ppMdDataSize = (UINT32 *)malloc(sizeof(UINT32));
			if (*ppMdDataSize == NULL)
			{
				printf("createMdIncrement DataERROR. malloc Err\n");
				return MD_APP_MEM_ERR;
			}
			/* Decide MD DATA Type*/
			switch(pCommandValue->mdTelegramType)
			{
				case INCREMENT_DATA:
					/* Create Increment DATA */
					err = createMdIncrementData(sendMdTransferRequestCounter, pCommandValue->mdMessageSize, &ppMdData);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Increment DATA */
						printf("Create Increment DATA ERROR\n");
					}
					else
					{
						/* Set MD DATA Size */
						**ppMdDataSize = pCommandValue->mdMessageSize;
						/* Set createMdDataFlag : On */
						pCommandValue->createMdDataFlag = MD_DATA_CREATE_ENABLE;
						/* Set send comId */
						pCommandValue->mdSendComId = COMID_INCREMENT_DATA;
					}
				break;
				case FIXED_DATA_1:
					/* Create Fixed DATA1 */
					err = createMdFixedData(COMID_FIXED_DATA1, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Fixed DATA1 */
						printf("Create Fixed DATA1 ERROR\n");
					}
					else
					{
						/* Set send comId */
						pCommandValue->mdSendComId = COMID_FIXED_DATA1;
					}
				break;
				case FIXED_DATA_2:
					/* Create Fixed DATA2 */
					err = createMdFixedData(COMID_FIXED_DATA2, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Fixed DATA2 */
						printf("Create Fixed DATA2 ERROR\n");
					}
					else
					{
						/* Set send comId */
						pCommandValue->mdSendComId = COMID_FIXED_DATA2;
					}
				break;
				case FIXED_DATA_3:
					/* Transport type : UDP */
					if (pCommandValue->mdTransportType == MD_TRANSPORT_UDP)
					{
						/* Create Fixed DATA3 */
						err = createMdFixedData(COMID_FIXED_DATA3, &ppMdData, *ppMdDataSize);
						if (err != MD_APP_NO_ERR)
						{
							/* Error : Create Fixed DATA3 */
							printf("Create Fixed DATA3 ERROR\n");
							break;
						}
					}
					else
					{
						/* Error : Transport  */
						printf("Create Fixed DATA3 ERROR. Because Transport is not UDP.\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_FIXED_DATA3;
				break;
				case FIXED_DATA_4:
					/* Transport type : TCP */
					if (pCommandValue->mdTransportType == MD_TRANSPORT_TCP)
					{
						/* Create Fixed DATA4 */
						err = createMdFixedData(COMID_FIXED_DATA4, &ppMdData, *ppMdDataSize);
						if (err != MD_APP_NO_ERR)
						{
							/* Error : Create Fixed DATA4 */
							printf("Create Fixed DATA4 ERROR\n");
							break;
						}
					}
					else
					{
						/* Error : Transport  */
						printf("Create Fixed DATA4 ERROR. Because Transport is not TCP.\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_FIXED_DATA4;
				break;
				case FIXED_DATA_5:
					/* Transport type : TCP */
					if (pCommandValue->mdTransportType == MD_TRANSPORT_TCP)
					{
						/* Create Fixed DATA5 */
						err = createMdFixedData(COMID_FIXED_DATA5, &ppMdData, *ppMdDataSize);
						if (err != MD_APP_NO_ERR)
						{
							/* Error : Create Fixed DATA5 */
							printf("Create Fixed DATA5 ERROR\n");
							break;
						}
					}
					else
					{
						/* Error : Transport  */
						printf("Create Fixed DATA5 ERROR. Because Transport is not TCP.\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_FIXED_DATA5;
				break;
				case FIXED_DATA_6:
					/* Create Fixed DATA6 */
					err = createMdFixedData(COMID_FIXED_DATA6, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Fixed DATA6 */
						printf("Create Fixed DATA6 ERROR\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_FIXED_DATA6;
				break;
				case ERROR_DATA_1:
					/* Create Error DATA1 */
					err = createMdFixedData(COMID_ERROR_DATA_1, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Error DATA1 */
						printf("Create Error DATA1 ERROR\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_ERROR_DATA_1;
				break;
				case ERROR_DATA_2:
					/* Create Error DATA2 */
					err = createMdFixedData(COMID_ERROR_DATA_2, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Error DATA2 */
						printf("Create Error DATA2 ERROR\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_ERROR_DATA_2;
				break;
				case ERROR_DATA_3:
					/* Create Error DATA3 */
					err = createMdFixedData(COMID_ERROR_DATA_3, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Error DATA3 */
						printf("Create Error DATA3 ERROR\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_ERROR_DATA_3;
				break;
				case ERROR_DATA_4:
					/* Create Error DATA4 */
					err = createMdFixedData(COMID_ERROR_DATA_4, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Error DATA4 */
						printf("Create Error DATA4 ERROR\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_ERROR_DATA_4;
				break;
				default:
						/* MD DATA TYPE NOTHING */
						printf("MD DATA Telegram Type ERROR. mdTelegramType = %d\n", pCommandValue->mdTelegramType);
				break;
			}
		break;
		case REPLIER:
			/* Decide MD Reply Error Type */
			if ((pCommandValue->mdReplyErr >= MD_REPLY_NO_ERR) && (pCommandValue->mdReplyErr <= MD_REPLY_NOLISTENER_ERR))
			{
				/* Set Replier Error */
				err = MD_APP_NO_ERR;
			}
			else
			{
				/* MD Reply Error Type Error*/
				printf("MD Reply Error Type ERROR. mdReplyErr = %d\n", pCommandValue->mdReplyErr);
			}
		break;
		default:
			/* Other than Caller and Replier */
			printf("Caller Replier Type ERROR. mdCallerReplierType = %d\n", pCommandValue->mdCallerReplierType);
		break;
	}
	/* MD DATA Create OK ? */
/*	if (err == MD_APP_NO_ERR)
	{	*/
		/* Decide MD Request Type */
/*		switch(pCommandValue->mdMessageKind)
		{
			case MD_MESSAGE_MN:

			break;
			case MD_MESSAGE_MR_MP:

			break;
			default:	*/
					/* MD Message Kind NOTHING */
/*					printf("MD Message kind ERROR. mdMessageKind = %d\n", pCommandValue->mdMessageKind);
					err = MD_APP_ERR;
			break;
		}
	}
*/
	return err;
}

/**********************************************************************************************************************/
/** main thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
MD_APP_ERR_TYPE command_main_proc(void)
{
	MD_APP_ERR_TYPE err = 0;
	int getCommandLength = 0;					/* Input Command Length */
	int i = 0;										/* loop counter */
	UINT8 operand = 0;							/* Input Command Operand Number */
	char getCommand[GET_COMMAND_MAX];			/* Input Command */
	char argvGetCommand[GET_COMMAND_MAX];		/* Input Command for argv */
	static char *argvCommand[100];				/* Command argv */
	int startPoint;								/* Copy Start Point */
	int endPoint;									/* Copy End Point */
	COMMAND_VALUE *pCommandValue = NULL;		/* Command Value */

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

		/* Set argvGetCommand[0] as program name */
//		strncpy(&argvGetCommand[0], firstArgv, sizeof(firstArgv));
//		argvCommand[operand] = &argvGetCommand[0];

//		argvCommand[operand] = firstArgv;
		argvCommand[operand] = *ppFirstArgv;
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

		/* Get COMMAND_VALUE Area */
		pCommandValue = (COMMAND_VALUE *)malloc(sizeof(COMMAND_VALUE));
		if (pCommandValue == NULL)
		{
			printf("COMMAND_VALUE malloc Err\n");
			return MD_APP_MEM_ERR;
		}
		else
		{
			/* Decide Create Thread */
			err = decideCreateThread(operand+1, argvCommand, pCommandValue);
			if (err !=  MD_APP_NO_ERR)
			{
				printf("Decide Create Thread Err\n");
				return MD_APP_THREAD_ERR;
			}
		}
	}
}

/**********************************************************************************************************************/
/** Create MdLog Thread
 *
 *  @param[in]		void
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdLogThread(void)
{
	/* Thread Handle */
	VOS_THREAD_T mdLogThreadHandle = NULL;							/* MDLog Thread handle */

	/* First Thread Create */
	vos_threadInit();
	/*  Create MdLog Thread */
	if (vos_threadCreate(&mdLogThreadHandle,
							mdLogThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							threadStackSize,
							(void *)MDLog,
							NULL) == VOS_NO_ERR)
	{
		return MD_APP_NO_ERR;
	}
	else
	{
		printf("MDLog Thread Create Err\n");
		return MD_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Create MdReceiveManager Thread
 *
 *  @param[in]		pMdReceiveManagerThreadParameter			pointer to MdReceiveManagerThread Parameter
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdReceiveManagerThread(MD_RECEIVE_MANAGER_THREAD_PARAMETER *pMdReceiveManagerThreadParameter)
{
	/* Thread Handle */
	VOS_THREAD_T mdReceiveManagerThreadHandle = NULL;			/* MDReceiveManager Thread handle */

	/*  Create MdReceiveManager Thread */
	if (vos_threadCreate(&mdReceiveManagerThreadHandle,
							mdReceiveManagerThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							threadStackSize,
							(void *)MDReceiveManager,
							(void *)pMdReceiveManagerThreadParameter) == VOS_NO_ERR)
	{
		return MD_APP_NO_ERR;
	}
	else
	{
		printf("MDReceiveManager Thread Create Err\n");
		return MD_APP_THREAD_ERR;
	}
}


/**********************************************************************************************************************/
/** Create MdCaller Thread
 *
 *  @param[in]		pCallerThreadParameter			pointer to CallerThread Parameter
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdCallerThread(
		CALLER_THREAD_PARAMETER *pCallerThreadParameter)
{
	/* Thread Handle */
	VOS_THREAD_T mdCallerThreadHandle = NULL;						/* MDCaller Thread handle */

	CHAR8 callerThreadCounterCharacter[THREAD_COUNTER_CHARACTER_SIZE] = {0};

	/* Caller Thread Number Count up */
	callerThreadNoCount++;

	/* Set Message Queue Name */
	/* Set Base Name */
	strncpy(pCallerThreadParameter->mqName, callerThreadName, sizeof(callerThreadName));
	/* String Conversion */
	sprintf(callerThreadCounterCharacter, "%u", callerThreadNoCount);
	/* Connect Base Name and Thread Counter */
	strcat(pCallerThreadParameter->mqName, callerThreadCounterCharacter);

	/*  Create Caller Thread */
	if (vos_threadCreate(&mdCallerThreadHandle,
							mdCallerThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							threadStackSize,
							(void *)MDCaller,
							(void *)pCallerThreadParameter) == VOS_NO_ERR)
	{
		return MD_APP_NO_ERR;
	}
	else
	{
		/* Caller Thread Number Count down */
		callerThreadNoCount--;
		printf("Caller Thread Create Err\n");
		return MD_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Create MdReplier Thread
 *
 *  @param[in]		pReplierThreadParameter			pointer to ReplierThread Parameter
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdReplierThread(
		REPLIER_THREAD_PARAMETER *pReplierThreadParameter)
{
	/* Thread Handle */
	VOS_THREAD_T mdReplierThreadHandle = NULL;					/* MDReplier Thread handle */

	CHAR8 replierThreadCounterCharacter[THREAD_COUNTER_CHARACTER_SIZE] = {0};

	/* Replier Thread Number Count up */
	replierThreadNoCount++;

	/* Set Message Queue Name */
	/* Set Base Name */
	strncpy(pReplierThreadParameter->mqName, replierThreadName, sizeof(replierThreadName));
	/* String Conversion */
	sprintf(replierThreadCounterCharacter, "%u", replierThreadNoCount);
	/* Connect Base Name and Thread Counter */
	strcat(pReplierThreadParameter->mqName, replierThreadCounterCharacter);

	/*  Create Replier Thread */
	if (vos_threadCreate(&mdReplierThreadHandle,
							mdReplierThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							threadStackSize,
							(void *)MDReplier,
							(void *)pReplierThreadParameter) == VOS_NO_ERR)
	{
		return MD_APP_NO_ERR;
	}
	else
	{
		printf("Replier Thread Create Err\n");
		/* Replier Thread Number Count down */
		replierThreadNoCount--;
		return MD_APP_THREAD_ERR;
	}
}


/**********************************************************************************************************************/
/** Decide Create Thread
 *
 *  @param[in]		argc
 *  @param[in]		argv
 *  @param[in]		pCommandValue			pointer to Command Value
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_ERR						error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE decideCreateThread(int argc, char *argv[], COMMAND_VALUE *pCommandValue)
{
	static BOOL firstTimeFlag = TRUE;
	MD_APP_ERR_TYPE err = MD_APP_NO_ERR;
	extern VOS_MUTEX_T pMdApplicationThreadMutex;

	/* Thread Parameter */
	MD_RECEIVE_MANAGER_THREAD_PARAMETER mdReceiveManagerThreadParameter = {0};	/* MDReceiveManagerThread Parameter */
	CALLER_THREAD_PARAMETER *pCallerThreadParameter = NULL;							/* CallerThread Parameter */
	REPLIER_THREAD_PARAMETER *pReplierThreadParameter = NULL;						/* ReplierThread Parameter */

	/* Analyze Command */
	err = analyzeCommand(argc, argv, pCommandValue);
	if (err != MD_APP_NO_ERR)
	{
		printf("COMMAND_VALUE Err\n");
		return MD_APP_ERR;
	}

/* Debug start */
//	return MD_APP_NO_ERR;
/* Debug end */

	/* Decide MD Transmission Pattern */
	err = decideMdPattern(pCommandValue, &pFirstCreateMdData, &pFirstCreateMdDataSize);
	if (err != MD_APP_NO_ERR)
	{
		printf("MD Transmission Pattern Err\n");
		return MD_APP_ERR;
	}

	/* Only the First Time */
	if (firstTimeFlag == TRUE)
	{
		/* Create MD Application Thread Mutex */
		if (vos_mutexCreate(&pMdApplicationThreadMutex) != VOS_NO_ERR)
		{
			printf("Create MD Application Thread Mutex Err\n");
			return MD_APP_THREAD_ERR;
		}
		/* Lock MD Application Thread Mutex */
		lockMdApplicationThread();
		/* Create Log File Thread */
		err = createMdLogThread();
		/* UnLock MD Application Thread Mutex */
		unlockMdApplicationThread();
		if (err != MD_APP_NO_ERR)
		{
			printf("Create MdLogThread Err\n");
			return MD_APP_THREAD_ERR;
		}

		/* Create MD Receive Manager Thread */
		/* Set Thread Parameter */
		mdReceiveManagerThreadParameter.pCommandValue = pCommandValue;
    	/* Waits for LOG Setting */
		vos_threadDelay(1000000);
		/* Lock MD Application Thread Mutex */
		lockMdApplicationThread();
		err = createMdReceiveManagerThread(&mdReceiveManagerThreadParameter);
		/* UnLock MD Application Thread Mutex */
		unlockMdApplicationThread();
		if (err != MD_APP_NO_ERR)
		{
			printf("Create MdReceiveManagerThread Err\n");
			return MD_APP_THREAD_ERR;
		}
		firstTimeFlag = FALSE;
	}

/* Debug start */
//	return MD_APP_NO_ERR;
/* Debug end */

	/* Create Application (Caller or Replier) Thread */
	if (pCommandValue->mdCallerReplierType == CALLER)
	{
		/* Get Thread Parameter Area */
		pCallerThreadParameter = (CALLER_THREAD_PARAMETER *)malloc(sizeof(CALLER_THREAD_PARAMETER));
		if (pCallerThreadParameter == NULL)
		{
			printf("decideCreateThread Err. malloc callerThreadParameter Err\n");
			return MD_APP_MEM_ERR;
		}
		else
		{
			/* Set Thread Parameter */
			pCallerThreadParameter->pCommandValue = pCommandValue;
			/* Set MD DATA */
			pCallerThreadParameter->pMdData = pFirstCreateMdData;
			pCallerThreadParameter->mdDataSize = *pFirstCreateMdDataSize;

	    	/* Waits for TRDP Setting */
			vos_threadDelay(1000000);
			/* Lock MD Application Thread Mutex */
			lockMdApplicationThread();
			/* Create Caller Thread */
			err = createMdCallerThread(pCallerThreadParameter);
			/* UnLock MD Application Thread Mutex */
			unlockMdApplicationThread();
			if (err != MD_APP_NO_ERR)
			{
				printf("Create CallerThread Err\n");
				return MD_APP_THREAD_ERR;
			}
		}
	}
	else if (pCommandValue->mdCallerReplierType == REPLIER)
	{
		/* Get Thread Parameter Area */
		pReplierThreadParameter = (REPLIER_THREAD_PARAMETER *)malloc(sizeof(REPLIER_THREAD_PARAMETER));
		if (pReplierThreadParameter == NULL)
		{
			printf("decideCreateThread Err. malloc ReplierThreadParameter Err\n");
			return MD_APP_MEM_ERR;
		}
		else
		{
			/* Set Thread Parameter */
			pReplierThreadParameter->pCommandValue = pCommandValue;

	    	/* Waits for TRDP Setting */
			vos_threadDelay(1000000);
			/* Lock MD Application Thread Mutex */
			lockMdApplicationThread();
			/* Create Replier Thread */
			err = createMdReplierThread(pReplierThreadParameter);
			/* unLock MD Application Thread Mutex */
			lockMdApplicationThread();
			if (err != MD_APP_NO_ERR)
			{
				printf("Create ReplierThread Err\n");
				return MD_APP_THREAD_ERR;
			}
		}
	}
	else
	{
		printf("MD Application Thread Create Err\n");
		return MD_APP_ERR;
	}
	return MD_APP_NO_ERR;
}




