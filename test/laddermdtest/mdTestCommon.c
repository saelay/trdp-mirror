/**********************************************************************************************************************/
/**
 * @file	mdTestCommon.c
 *
 * @brief	Demo MD ladder application for TRDP
 *
 * @details	TRDP Ladder Topology Support MD Transmission Common
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


/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include <mqueue.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "trdp_if_light.h"
#include "vos_thread.h"
#include "trdp_private.h"

#ifdef __linux__
	#include <uuid/uuid.h>
#endif

#include "mdTestApp.h"

#define SOURCE_URI "user@host"
#define DEST_URI "user@host"


/* global vars */
static APP_THREAD_SESSION_HANDLE_MQ_DESCRIPTOR appThreadSessionHandleMqDescriptorTable[APP_SESSION_HANDLE_MQ_DESC_TABLE_MAX] = {{{{0}}}};
static COMID_MD_DATA_FILE_NAME comIdMdDataFileNameTable[] =
{
		{COMID_FIXED_DATA1, "mdLiteral1"},		/* Single Packet DATA */
		{COMID_FIXED_DATA2, "mdLiteral2"},		/* 3 Packet DATA */
		{COMID_FIXED_DATA3, "mdLiteral3"},		/* UDP MAX DATA */
		{COMID_FIXED_DATA4, "mdLiteral4"},		/* 128K Octet DATA */
		{COMID_FIXED_DATA5, "mdLiteral5"},		/* TCP MAX DATA */
		{COMID_FIXED_DATA6, "mdLiteral6"},		/* 512 Octet DATA */
		{COMID_ERROR_DATA_1, "mdErrMode1"},	/* For ErrMode1 : Transmission Error */
		{COMID_ERROR_DATA_2, "mdErrMode2"},	/* For ErrMode2 : Not Listener */
		{COMID_ERROR_DATA_3, "mdErrMode3"},	/* For ErrMode3 : ComId Error */
		{COMID_ERROR_DATA_4, "mdErrMode4"}		/* For ErrMode4 : Reply retry */
};

VOS_MUTEX_T pMdApplicationThreadMutex = NULL;					/* pointer to Mutex for MD Application Thread */

/* Mutex functions */
/**********************************************************************************************************************/
/** Get MD Application Thread accessibility.
 *
 *  @retval         MD_APP_NO_ERR			no error
 *  @retval         MD_APP_MUTEX_ERR		mutex error
 */
MD_APP_ERR_TYPE  lockMdApplicationThread (
    void)
{
	extern VOS_MUTEX_T pMdApplicationThreadMutex;					/* pointer to Mutex for MD Application Thread */

	/* Lock MD Application Thread by Mutex */
	if ((vos_mutexTryLock(pMdApplicationThreadMutex)) != VOS_NO_ERR)
	{
		vos_printf(VOS_LOG_ERROR, "MD Application Thread Mutex Lock failed\n");
		return MD_APP_MUTEX_ERR;
	}
    return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Release MD Application Thread accessibility.
 *
 *  @retval         MD_APP_NO_ERR			no error
 *  @retval         MD_APP_MUTEX_ERR		mutex error
 *
  */
MD_APP_ERR_TYPE  unlockMdApplicationThread (
    void)
{
	extern VOS_MUTEX_T pMdApplicationThreadMutex;		/* pointer to Mutex for MD Application Thread */

	/* UnLock MD Application Thread by Mutex */
	vos_mutexUnlock(pMdApplicationThreadMutex);
	return MD_APP_NO_ERR;
}

/* queue functions */
MD_APP_ERR_TYPE queue_initialize(const char *pMqName, mqd_t *pMpDescriptor)
{
	struct mq_attr new_ma;
	struct mq_attr old_ma;
	struct mq_attr * pma;
	int rc;
	mqd_t trdp_mq;

	/* creation attributes */
	new_ma.mq_flags   = O_NONBLOCK;
	new_ma.mq_maxmsg  = TRDP_QUEUE_MAX_MESG;
	new_ma.mq_msgsize = TRDP_QUEUE_MAX_SIZE;
	new_ma.mq_curmsgs = 0;

	#ifdef __linux__
	pma = &new_ma;
	#endif

	#ifdef __QNXNTO__
	pma = &new_ma;
	#endif

	/* create a queue */
	trdp_mq = mq_open(pMqName, O_RDWR | O_CREAT, S_IWUSR|S_IRUSR, pma);
	if ((mqd_t)-1 == trdp_mq)
	{
		printf("mq_open() Error");
		return MD_APP_ERR;
	}
	/* get attributes */
	rc = mq_getattr(trdp_mq,&old_ma);
	if (-1 == rc)
	{
		printf("mq_getattr() Error");
		return MD_APP_ERR;
	}

	new_ma = old_ma;

	new_ma.mq_flags = O_NONBLOCK;

	/* change attributes */
	rc = mq_setattr(trdp_mq,&new_ma,&old_ma);
	if (-1 == rc)
	{
		printf("mq_setattr() Error");
		return MD_APP_ERR;
	}

	/* get attributes */
	rc = mq_getattr(trdp_mq,&old_ma);
	if (-1 == rc)
	{
		printf("mq_getattr() Error");
		return MD_APP_ERR;
	}

	/* Return Message Queue Descriptor */
	*pMpDescriptor = trdp_mq;
	return 0;
}

/* send message trough queue */
MD_APP_ERR_TYPE queue_sendMessage(trdp_apl_cbenv_t * msg, mqd_t mqDescriptor)
{
	int rc;
	char * p_bf = (char *)msg;
	int    l_bf = sizeof(*msg) - sizeof(msg->dummy);

	rc = mq_send(mqDescriptor,p_bf,l_bf,0);
	if (-1 == rc)
	{
		printf("mq_send() Error");
		return MD_APP_ERR;
	}
	else
	{
		return 0;
	}
}

/* receive message from queue */
MD_APP_ERR_TYPE queue_receiveMessage(trdp_apl_cbenv_t * msg, mqd_t mqDescriptor)
{
	ssize_t rc;
	char * p_bf = (char *)msg;
	int    l_bf = sizeof(*msg) - sizeof(msg->dummy);
	int    s_bf = sizeof(*msg) - 1;
	unsigned msg_prio;

	msg_prio = 0;
	rc = mq_receive(mqDescriptor,p_bf,s_bf,&msg_prio);
	if ((ssize_t)-1 == rc)
	{
		if (EAGAIN == errno)
		{
			return errno;
		}
		printf("mq_receive() Error");
		return MD_APP_ERR;
	}
	if (rc != l_bf)
	{
		printf("mq_receive() expected %d bytes, not %d\n",l_bf,(int)rc);
		return MD_APP_ERR;
	}
	printf("Received %d bytes\n",(int)rc);
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Set Message Queue Descriptor
 *
 *  @param[in]		pAppThreadSessionHandle		pointer to MD Application Thread Session Handle
 *  @param[in]		mqDescriptor					Message Queue Descriptor
 *
 */
MD_APP_ERR_TYPE setAppThreadSessionMessageQueueDescriptor(
		APP_THREAD_SESSION_HANDLE *pAppThreadSessionHandle,
		mqd_t mqDescriptor)
{
	UINT32 i = 0;
	for(i=0; i < APP_SESSION_HANDLE_MQ_DESC_TABLE_MAX; i++)
	{
		if(appThreadSessionHandleMqDescriptorTable[i].mqDescriptor == 0)
		{
			appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle = *pAppThreadSessionHandle;
			appThreadSessionHandleMqDescriptorTable[i].mqDescriptor = mqDescriptor;
			return MD_APP_NO_ERR;
		}
	}
	printf("Don't Set MQ Descriptor.\n");
	return MD_APP_ERR;
}

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
		mqd_t mqDescriptor)
{
	UINT32 i = 0;
	for(i = 0; i < APP_SESSION_HANDLE_MQ_DESC_TABLE_MAX; i++)
	{
		/* Matching ComId : equal */
		if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->comId == pAppThreadSessionHandle->pMdAppThreadListener->comId)
		{
			/* Matching Source IP Address : equal or nothing */
			if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->srcIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->srcIpAddr)
				||(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->srcIpAddr == IP_ADDRESS_NOTHING))
			{
				/* Matching Destination IP Address : equal */
				if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->destIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->destIpAddr)
				{
					/* Matching sessionId : equal */
//					if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.mdAppThreadSessionId == pAppThreadSessionHandle->mdAppThreadSessionId)
					if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.mdAppThreadSessionId == NULL)
					{
						continue;
					}
					else
					{
						if (strncmp((char *)appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.mdAppThreadSessionId,
									(char *)pAppThreadSessionHandle->mdAppThreadSessionId,
									sizeof(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.mdAppThreadSessionId)) == 0)
						{
							/* Clear mqDescriptor */
							appThreadSessionHandleMqDescriptorTable[i].mqDescriptor = 0;
							/* Clear appThreadSessionHandle */
							memset(&(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle), 0, sizeof(APP_THREAD_SESSION_HANDLE));
							/* Free Request Thread Session Handle Area */
							free(pAppThreadSessionHandle);
							return MD_APP_NO_ERR;
						}
					}
				}
			}
		}
	}
	return -1;
}

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
mqd_t getAppThreadSessionMessageQueueDescriptor(
		UINT32 *pLoopStartNumber,
		TRDP_MSG_T mdMsgType,
		APP_THREAD_SESSION_HANDLE *pAppThreadSessionHandle)
{
	UINT32 i = 0;
	for(i = *pLoopStartNumber; i < APP_SESSION_HANDLE_MQ_DESC_TABLE_MAX; i++)
	{
		/* Check Table Listener == NULL*/
		if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener == NULL)
		{
			/* Not Maching */
			return -1;
		}
		/* Matching ComId : equal */
		if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->comId == pAppThreadSessionHandle->pMdAppThreadListener->comId)
		{
			/* Matching Source IP Address : equal or nothing */
			if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->srcIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->srcIpAddr)
				||(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->srcIpAddr == IP_ADDRESS_NOTHING))
			{
				/* Matching Destination IP Address : equal */
				if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->destIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->destIpAddr)
					||(pAppThreadSessionHandle->pMdAppThreadListener->destIpAddr == IP_ADDRESS_NOTHING))
				{
					/* Matching mcGroup Address : equal */
/*					if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.comId == pAppThreadSessionHandle->comId)
					{ */
						/* Check msgType */
					    switch (mdMsgType)
					    {
					        case TRDP_MSG_MP:
					        	/* Matching sessionId : equal */
					        	if(memcmp(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.mdAppThreadSessionId,
					        			pAppThreadSessionHandle->mdAppThreadSessionId,
					        			sizeof(TRDP_UUID_T)) == 0)
					        	{
					    			*pLoopStartNumber = i;
					    			return appThreadSessionHandleMqDescriptorTable[i].mqDescriptor;
					        	}
					        break;
					        case TRDP_MSG_MN:
					        case TRDP_MSG_MR:
								*pLoopStartNumber = i;
								return appThreadSessionHandleMqDescriptorTable[i].mqDescriptor;
					        break;
					        default:
					        break;
					    }
/*					} */
				}
			}
		}
	}
	return -1;
}

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
 *
 */
MD_APP_ERR_TYPE createMdIncrementData (UINT32 mdSendCount, UINT32 mdDataSize, UINT8 ***pppMdData)
{
	char firstCharacter = 0;
	UINT8 *pFirstMdData = NULL;
	UINT32 i = 0;

	/* Parameter Check */
	if ((mdDataSize >= MD_INCREMENT_DATA_MIN_SIZE)
			&& (mdDataSize <= MD_INCREMENT_DATA_MAX_SIZE))
	{
		/* Get MD DATA Area */
		**pppMdData = (UINT8 *)malloc(mdDataSize);
		if (**pppMdData == NULL)
		{
			printf("createMdIncrement DataERROR. malloc Err\n");
			return MD_APP_MEM_ERR;
		}
		else
		{
			/* Store pMdData Address */
			pFirstMdData = **pppMdData;
			/* Set DATA Size */
			sprintf((char *)**pppMdData, "%4u", mdDataSize);
			**pppMdData = **pppMdData + 4;

			/* Create Top Character */
			firstCharacter = (char)(mdSendCount % MD_DATA_INCREMENT_CYCLE);
			/* Create MD Increment Data */
			for(i=0; i <= mdDataSize - 4; i++)
			{
				***pppMdData = (char)((firstCharacter + i) % MD_DATA_INCREMENT_CYCLE);
				**pppMdData = **pppMdData + 1;
			}
		}
		/* Return start pMdaData Address */
		**pppMdData = pFirstMdData;
		}
	else
	{
		printf("createMdIncrementData ERROR. parameter Err\n");
		return MD_APP_PARAM_ERR;
	}
	return MD_APP_NO_ERR;
}

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
MD_APP_ERR_TYPE createMdFixedData (UINT32 comId, UINT8 ***pppMdData, UINT32 *pMdDataSize)
{
	MD_APP_ERR_TYPE err = MD_APP_NO_ERR;
	char mdDataFileName[MD_DATA_FILE_NAME_MAX_SIZE] = {0};
	FILE *fpMdDataFile = NULL;
	long mdDataFileSize = 0;
	struct stat fileStat = {0};

	/* Parameter Check */
	if ((pppMdData == NULL)
			&& (pMdDataSize == NULL))
	{
		printf("createMdFixedDataERROR. parameter Err\n");
		return MD_APP_PARAM_ERR;
	}
	else
	{
		/* Get ComId-MD DATA File Name */
		err = getMdDataFileNameFromComId(comId, mdDataFileName);
		if (err != MD_APP_NO_ERR)
		{
			printf("createMdFixedData ERROR. comId:%d Err\n", comId);
			return MD_APP_PARAM_ERR;
		}
		else
		{
			/* Get MD Fixed DATA */
			/* Open MdDataFile */
			fpMdDataFile = fopen(mdDataFileName, "rb");
			if (fpMdDataFile == NULL)
			{
				printf("createMdFixedData ERROR. MdDataFile Open Err\n");
				return MD_APP_PARAM_ERR;
			}
			/* Get MdDataFile Size */
/*
			fseek(fpMdDataFile, 0, SEEK_END);
			mdDataFileSize = ftell(fpMdDataFile);
			fseek(fpMdDataFile, 0, SEEK_SET);
*/
			if ((fstat(fileno(fpMdDataFile), &fileStat)) == -1)
			{
				printf("createMdFixedData ERROR. MdDataFile Stat Err\n");
				return MD_APP_PARAM_ERR;
			}
			mdDataFileSize = fileStat.st_size;
			*pMdDataSize = mdDataFileSize;

			/* Get MD DATA Area */
			**pppMdData = (UINT8 *)malloc(mdDataFileSize);
			if (**pppMdData == NULL)
			{
				printf("createMdFixedData ERROR. malloc Err\n");
				return MD_APP_MEM_ERR;
			}

			/* Read MdDataFile */
			fread(**pppMdData, sizeof(char), mdDataFileSize, fpMdDataFile);
			/* Close MdDataFile */
			fclose(fpMdDataFile);
		}
	}
	return MD_APP_NO_ERR;
}

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
MD_APP_ERR_TYPE getMdDataFileNameFromComId (UINT32 comId, CHAR8 *pMdDataFileName)
{
	int i = 0;										/* loop counter */
	int comIdMdDataFileNameTableNumber = 0;	/* ComId-MdDataFileName Table Number */

	/* Parameter Check */
	if (pMdDataFileName == NULL)
	{
		printf("getMdDataFileNameFromComIdERROR. parameter Err\n");
		return MD_APP_PARAM_ERR;
	}
	else
	{
		/* Get Number of comIdMdDataFileNameTable Member */
		comIdMdDataFileNameTableNumber = (sizeof(comIdMdDataFileNameTable)) / (sizeof(comIdMdDataFileNameTable[0]));
		/* Search ComId to MdDataFileName */
		for(i=0; i < comIdMdDataFileNameTableNumber; i++)
		{
			if (comId == comIdMdDataFileNameTable[i].comId)
			{
				/* Set MD DATA FILE NAME */
				strncpy(pMdDataFileName, comIdMdDataFileNameTable[i].mdDataFileName, sizeof(comIdMdDataFileNameTable[i].mdDataFileName));
				return MD_APP_NO_ERR;
			}
		}
	}
	printf("getMdDataFileNameFromComId ERROR. Unmatch ComId:%d Err\n", comId);
	return MD_APP_PARAM_ERR;
}

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
int l2fLog(char *logString, int logKind, int dumpOnOff)
{
	static int writeLogFifoFd = 0;
	char writeFifoBuffer[PIPE_BUFFER_SIZE] = {0};
	ssize_t writeFifoBufferSize = 0;
	ssize_t writeFifoSize = 0;

	/* Get fifoBuffer Size */
	writeFifoBufferSize = sizeof(writeFifoBuffer);

	/* Create Log String */
	/* Set Log Kind */
	sprintf(writeFifoBuffer, "%1u", logKind);
	/* Set Dump OnOff */
	sprintf(writeFifoBuffer + 1, "%1u", dumpOnOff);
	/* Set String */
	sprintf(writeFifoBuffer +2, "%s", logString);

	/* Check FIFO Open */
	if (writeLogFifoFd <= 0)
	{
		/* FIFO Open */
		writeLogFifoFd = open(LOG_PIPE, O_WRONLY);
		if (writeLogFifoFd == -1)
		{
			/* Log FIFO(named pipe) Open Error */
			printf("Write Log FIFO Open ERROR\n");
			return MD_APP_ERR;
		}
	}

	// Send to server using named pipe(FIFO)
	writeFifoSize = write(writeLogFifoFd, writeFifoBuffer, writeFifoBufferSize);
	if (writeFifoSize == -1)
	{
		printf("l2fLogERROR. write FIFO Err\n");
		return MD_APP_ERR;
	}
	return MD_APP_NO_ERR;
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

/**********************************************************************************************************************/
/** String Conversion
 *
 *  @param[in]		pStartAddress				pointer to Start Memory Address
 *  @param[in]		memorySize					convert Memory Size
 *  @param[in]		logKind					Log Kind
 *  @param[in]		dumpOnOff					Dump Enable or Disable
 *  @param[in]		callCount					miscMemory2String Recursive Call Count
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *
 */
MD_APP_ERR_TYPE miscMemory2String (
		const void *pStartAddress,
		size_t memorySize,
		int logKind,
		int dumpOnOff,
		int callCount)
{
	char *strTmpStartAddress = NULL;
	char *strTmp = NULL;
	size_t remainderMemorySize = 0;
	int recursiveCallCount = 0;

	/* Get String Area */
	strTmp = (char *)malloc(PIPE_BUFFER_SIZE);
	if (strTmp == NULL)
	{
		printf("miscMemory2String(): Could not allocate memory.\n");
		return MD_APP_MEM_ERR;
	}
	strTmpStartAddress = strTmp;

//	strTmp = strTmpBuffer;
	if (pStartAddress != NULL && memorySize > 0)
	{
		int i,j;
		const UINT8 *b = pStartAddress;

		/* Recursive Call ? */
		if (callCount > 0)
		{
			recursiveCallCount = callCount;
		}

		/* Don't Display at once ? */
		if (memorySize > LOG_OUTPUT_BUFFER_SIZE)
		{
			/* Hold remaining Log String */
			remainderMemorySize = memorySize - LOG_OUTPUT_BUFFER_SIZE;
			memorySize = LOG_OUTPUT_BUFFER_SIZE;
		}

		/* Create Log String */
		for(i = 0; i < memorySize; i += 16)
		{
			sprintf(strTmp, "%04X ", i + (recursiveCallCount * LOG_OUTPUT_BUFFER_SIZE));
			strTmp = strTmp + 5;
			/**/
			for(j = 0; j < 16; j++)
			{
				if (j == 8)
				{
					sprintf(strTmp, "- ");
					strTmp = strTmp + 2;
				}
				if ((i+j) < memorySize)
				{
					int ch = b[i+j];
					sprintf(strTmp, "%02X ",ch);
					strTmp = strTmp + 3;
				}
				else
				{
					sprintf(strTmp, "   ");
					strTmp = strTmp + 3;
				}
			}
			sprintf(strTmp, "   ");
			strTmp = strTmp + 3;

			for(j = 0; j < 16 && (i+j) < memorySize; j++)
			{
				int ch = b[i+j];
				sprintf(strTmp, "%c", (ch >= ' ' && ch <= '~') ? ch : '.');
				strTmp = strTmp + 1;
			}
			sprintf(strTmp, "\n");
			strTmp = strTmp + 1;
		}
		/* LOG OUTPUT */
		l2fLog(strTmpStartAddress, logKind, dumpOnOff);
		/* Release String Area */
		free(strTmpStartAddress);

		/* Remaining Log String ? */
		if (remainderMemorySize > 0)
		{
			/* Recursive Call : miscMemory2String */
			miscMemory2String(
					pStartAddress + LOG_OUTPUT_BUFFER_SIZE,
					remainderMemorySize,
					logKind,
					dumpOnOff,
					recursiveCallCount + 1);
		}
	}
	else
	{
		/* Release String Area */
		free(strTmpStartAddress);
		return MD_APP_ERR;
	}
	return MD_APP_NO_ERR;
}

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
MD_APP_ERR_TYPE getMdDataFromComId(
		UINT32 receiveComId,
		UINT8 *pReceiveMdData,
		UINT32 *pReceiveMdDataSize,
		UINT8 **ppCheckMdData,
		UINT32 **ppCheckMdDataSize)
{
	UINT32 startCharacter = 0;
	MD_APP_ERR_TYPE err = MD_APP_NO_ERR;

	/* Get Check MD DATA Size Area */
	*ppCheckMdDataSize = (UINT32 *)malloc(sizeof(UINT32));
	if (*ppCheckMdDataSize == NULL)
	{
		printf("getMdDataFromId ERROR. malloc Err\n");
		return MD_APP_MEM_ERR;
	}

	/* Decide MD DATA Type*/
	switch(receiveComId)
	{
		case COMID_INCREMENT_DATA:
		case COMID_INCREMENT_DATA_REPLY:
			/* Get Increment Start Byte of Receive MD DATA */
			memcpy(&startCharacter, pReceiveMdData + 4, sizeof(char));
			/* Create Increment DATA */
			err = createMdIncrementData(startCharacter, *pReceiveMdDataSize, &ppCheckMdData);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Increment DATA */
				printf("Create Increment DATA ERROR\n");
			}
			else
			{
				**ppCheckMdDataSize = *pReceiveMdDataSize;
			}
		break;
		case COMID_FIXED_DATA1:
		case COMID_FIXED_DATA1_REPLY:
			/* Create Fixed DATA1 */
			err = createMdFixedData(COMID_FIXED_DATA1, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Fixed DATA1 */
				printf("Create Fixed DATA1 ERROR\n");
			}
		break;
		case COMID_FIXED_DATA2:
		case COMID_FIXED_DATA2_REPLY:
			/* Create Fixed DATA2 */
			err = createMdFixedData(COMID_FIXED_DATA2, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Fixed DATA2 */
				printf("Create Fixed DATA2 ERROR\n");
			}
		break;
		case COMID_FIXED_DATA3:
		case COMID_FIXED_DATA3_REPLY:
			/* Create Fixed DATA3 */
			err = createMdFixedData(COMID_FIXED_DATA3, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Fixed DATA3 */
				printf("Create Fixed DATA3 ERROR\n");
			}
		break;
		case COMID_FIXED_DATA4:
		case COMID_FIXED_DATA4_REPLY:
			/* Create Fixed DATA4 */
			err = createMdFixedData(COMID_FIXED_DATA4, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Fixed DATA4 */
				printf("Create Fixed DATA4 ERROR\n");
			}
		break;
		case COMID_FIXED_DATA5:
		case COMID_FIXED_DATA5_REPLY:
			/* Create Fixed DATA5 */
			err = createMdFixedData(COMID_FIXED_DATA5, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Fixed DATA5 */
				printf("Create Fixed DATA5 ERROR\n");
			}
		break;
		case COMID_FIXED_DATA6:
		case COMID_FIXED_DATA6_REPLY:
			/* Create Fixed DATA6 */
			err = createMdFixedData(COMID_FIXED_DATA6, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Fixed DATA6 */
				printf("Create Fixed DATA6 ERROR\n");
			}
		break;
		case COMID_ERROR_DATA_1:
		case COMID_ERROR_DATA_1_REPLY:
			/* Create Error DATA1 */
			err = createMdFixedData(COMID_ERROR_DATA_1, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Error DATA1 */
				printf("Create Error DATA1 ERROR\n");
			}
		break;
		case COMID_ERROR_DATA_2:
		case COMID_ERROR_DATA_2_REPLY:
			/* Create Error DATA2 */
			err = createMdFixedData(COMID_ERROR_DATA_2, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Error DATA2 */
				printf("Create Error DATA2 ERROR\n");
			}
		break;
		case COMID_ERROR_DATA_3:
		case COMID_ERROR_DATA_3_REPLY:
			/* Create Error DATA3 */
			err = createMdFixedData(COMID_ERROR_DATA_3, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Error DATA3 */
				printf("Create Error DATA3 ERROR\n");
			}
		break;
		case COMID_ERROR_DATA_4:
		case COMID_ERROR_DATA_4_REPLY:
			/* Create Error DATA4 */
			err = createMdFixedData(COMID_ERROR_DATA_4, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Error DATA4 */
				printf("Create Error DATA4 ERROR\n");
			}
		break;
		default:
				/* MD DATA TYPE NOTHING */
				printf("Receive ComId ERROR. receiveComId = %d\n", receiveComId);
				err = MD_APP_ERR;
		break;
	}
	return err;
}

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
		CHAR8 *pLogString)
{
	/* Lock MD Application Thread Mutex */
	lockMdApplicationThread();

	MD_APP_ERR_TYPE err = MD_APP_ERR;		/* MD Application Result Code */
	UINT8 *pCheckMdData = NULL;
	UINT32 *pCheckMdDataSize = NULL;
	UINT32 receiveMdDataSetSize = 0;

	/* Get MD DATASET Size (MD DATA = MD HEADER + MD DATASET + FCR) */
	memcpy(&receiveMdDataSetSize, pReceiveMdDataSize, sizeof(UINT32));
	receiveMdDataSetSize = receiveMdDataSetSize - MD_HEADER_SIZE - MD_FCS_SIZE;

	/* Create for check MD DATA */
	err = getMdDataFromComId(
			receiveComId,
			pReceiveMdData + MD_HEADER_SIZE,
			&receiveMdDataSetSize,
			&pCheckMdData,
			&pCheckMdDataSize);
	if (err != MD_APP_NO_ERR)
	{
		/* Error : Create Increment DATA */
		sprintf(pLogString, "<NG> Receive MD DATA error. Create Check MD DATA Err.\n");
		if (pCheckMdDataSize != NULL)
		{
			/* Free check MD DATA Size Area */
			free(pCheckMdDataSize);
		}
		if (pCheckMdData != NULL)
		{
			/* Free check MD DATA Area */
			free(pCheckMdData);
		}
		return err;
	}
	else
	{
		/* Check compare size*/
		if (receiveMdDataSetSize != *pCheckMdDataSize)
		{
			sprintf(pLogString, "<NG> Receive MD DATA error. The size of is different.\n");
			if (pCheckMdDataSize != NULL)
			{
				/* Free check MD DATA Size Area */
				free(pCheckMdDataSize);
			}
			if (pCheckMdData != NULL)
			{
				/* Free check MD DATA Area */
				free(pCheckMdData);
			}
			return MD_APP_ERR;
		}
		else
		{
			/* Compare Contents */
			if (memcmp(pReceiveMdData + MD_HEADER_SIZE, pCheckMdData, receiveMdDataSetSize) != 0)
			{
				sprintf(pLogString, "<NG> Receive MD error. Contents is different.\n");
				if (pCheckMdDataSize != NULL)
				{
					/* Free check MD DATA Size Area */
					free(pCheckMdDataSize);
				}
				if (pCheckMdData != NULL)
				{
					/* Free check MD DATA Area */
					free(pCheckMdData);
				}
				return MD_APP_ERR;
			}
			else
			{
				sprintf(pLogString,"<OK> Receive MD DATA normal.\n");
				if (pCheckMdDataSize != NULL)
				{
					/* Free check MD DATA Size Area */
					free(pCheckMdDataSize);
				}
				if (pCheckMdData != NULL)
				{
					/* Free check MD DATA Area */
					free(pCheckMdData);
				}
			}
		}
	}
	/* UnLock MD Application Thread Mutex */
	unlockMdApplicationThread();
	return MD_APP_NO_ERR;
}

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
		TRDP_ERR_T mdResultCode)
{
	/*    Check why we have been called    */
	switch (mdResultCode)
	{
		case TRDP_NO_ERR:
			return MD_APP_NO_ERR;
		break;

		case TRDP_TIMEOUT_ERR:
		case TRDP_REPLYTO_ERR:
		case TRDP_CONFIRMTO_ERR:
		case TRDP_REQCONFIRMTO_ERR:
			return mdResultCode;
			break;

		case TRDP_APP_TIMEOUT_ERR:
		case TRDP_APP_REPLYTO_ERR:
		case TRDP_APP_CONFIRMTO_ERR:
			return mdResultCode;
		   break;

		default:
		   printf("Error on packet err = %d\n",mdResultCode);
		   return mdResultCode;
		   break;
	}
}

/**********************************************************************************************************************/
/** Display CommandValue
 *
 *  @param[in]      pHeadCommandValue	pointer to head of queue
 *
 *  @retval         != NULL         		pointer to CommandValue
 *  @retval         NULL            		No MD CommandValue found
 */
MD_APP_ERR_TYPE printCommandValue (
		COMMAND_VALUE	*pHeadCommandValue)
{
	COMMAND_VALUE *iterCommandValue;
	UINT16 commnadValueNumber = 1;
	char strIp[16] = {0};

    if (pHeadCommandValue == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

    /* Check Valid First Command */
    /* Caller: destination IP = 0 */
    /* Replier: listener comId = 0 */
    if (((pHeadCommandValue->mdCallerReplierType == CALLER) && (pHeadCommandValue->mdDestinationAddress == 0))
    	|| ((pHeadCommandValue->mdCallerReplierType == REPLIER) && (pHeadCommandValue->mdAddListenerComId == 0)))
    {
    	printf("Valid First MD Command isn't Set up\n");
    	return MD_APP_NO_ERR;
    }

    for (iterCommandValue = pHeadCommandValue;
    	  iterCommandValue != NULL;
    	  iterCommandValue = iterCommandValue->pNextCommandValue)
    {
    	/*  Dump CommandValue */
		printf("MD Command Value Thread No.%u\n", commnadValueNumber);
		printf("-b,	Application Type (Caller:0, Replier:1): %u\n", iterCommandValue->mdCallerReplierType);
		printf("-c,	Transport Type (UDP:0, TCP:1): %u\n", iterCommandValue->mdTransportType);
		printf("-d,	Caller Request Message Type (Mn:0, Mr-Mp:1): %u\n", iterCommandValue->mdMessageKind);
		printf("-e,	Caller Send MD DATASET Telegram Type (Increment:0, Fixed:1-6, Error:7-10): %u\n", iterCommandValue->mdTelegramType);
		printf("-f,	MD Increment Message Size Byte: %u\n", iterCommandValue->mdMessageSize);
		miscIpToString(iterCommandValue->mdDestinationAddress, strIp);
		printf("-g,	Caller MD Send Destination IP Address: %s\n", strIp);
		printf("-i,	Dump Type (DumpOn:1, DumpOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdDump);
		printf("-j,	Caller known MD Replier Number: %u\n", iterCommandValue->mdReplierNumber);
		printf("-k,	Caller MD Request Send Cycle Number: %u\n", iterCommandValue->mdCycleNumber);
		printf("-l,	Log Type (LogFileOn:1, LogFileOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdLog);
		printf("-m,	Caller MD Request Send Cycle Time: %u micro sec\n", iterCommandValue->mdCycleTime);
		printf("-n,	Topology TYpe (Ladder:1, not Lader:0): %u\n", iterCommandValue->mdLadderTopologyFlag);
		printf("-o,	Replier MD Reply Error Type(1-6): %u\n", iterCommandValue->mdReplyErr);
		printf("-p,	Marshalling Type (Marshall:1, not Marshall:0): %u\n", iterCommandValue->mdMarshallingFlag);
		printf("-q,	Replier Add Listener ComId: %u\n", iterCommandValue->mdReplierNumber);
		printf("-r,	Reply TImeout: %u micro sec\n", iterCommandValue->mdTimeoutReply);
		printf("-t,	Caller Using Network I/F (Subnet1:1,subnet2:2): %u\n", iterCommandValue->mdSendSubnet);
		commnadValueNumber++;
    }
    return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display MD Statistics
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_PARAM_ERR					parameter	error
 *  @retval         MD_APP_ERR						error
 */
MD_APP_ERR_TYPE printMdStatistics (
		TRDP_APP_SESSION_T  appHandle)
{
	TRDP_ERR_T err;
	TRDP_STATISTICS_T   mdStatistics;
	char strIp[16] = {0};

	if (appHandle == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

	/* Get MD Statistics */
	err = tlc_getStatistics(appHandle, &mdStatistics);
	if (err == TRDP_NO_ERR)
	{
		/*  Dump MD Statistics */
		printf("===   MD Statistics  ===\n");
		miscIpToString(appHandle->realIP, strIp);
		printf("Application Handle RealIP(Network I/F Address): %s\n", strIp);
		printf("===   UDP  ===\n");
		printf("Default QoS for MD: %u \n", mdStatistics.udpMd.defQos);
		printf("Default TTL for MD: %u \n", mdStatistics.udpMd.defTtl);
		printf("Default reply timeout in us for MD: %u micro sec\n", mdStatistics.udpMd.defReplyTimeout);
		printf("Default confirm timeout in us for MD: %u micro sec\n", mdStatistics.udpMd.defConfirmTimeout);
		printf("Number of listeners: %u \n", mdStatistics.udpMd.numList);
		printf("Number of received MD packets: %u \n", mdStatistics.udpMd.numRcv);
		printf("Number of received MD packets with CRC err: %u \n", mdStatistics.udpMd.numCrcErr);
		printf("Number of received MD packets with protocol err: %u \n", mdStatistics.udpMd.numProtErr);
		printf("Number of received MD packets with wrong topo count : %u \n", mdStatistics.udpMd.numTopoErr);
		printf("Number of received MD packets without listener: %u \n", mdStatistics.udpMd.numNoListener);
		printf("Number of reply timeouts: %u \n", mdStatistics.udpMd.numReplyTimeout);
		printf("Number of confirm timeouts: %u \n", mdStatistics.udpMd.numConfirmTimeout);
		printf("Number of sent MD packets: %u \n", mdStatistics.udpMd.numSend);
		printf("===   TCP  ===\n");
		printf("Default QoS for MD: %u \n", mdStatistics.tcpMd.defQos);
		printf("Default TTL for MD: %u \n", mdStatistics.tcpMd.defTtl);
		printf("Default reply timeout in us for MD: %u micro sec\n", mdStatistics.tcpMd.defReplyTimeout);
		printf("Default confirm timeout in us for MD: %u micro sec\n", mdStatistics.tcpMd.defConfirmTimeout);
		printf("Number of listeners: %u \n", mdStatistics.tcpMd.numList);
		printf("Number of received MD packets: %u \n", mdStatistics.tcpMd.numRcv);
		printf("Number of received MD packets with CRC err: %u \n", mdStatistics.tcpMd.numCrcErr);
		printf("Number of received MD packets with protocol err: %u \n", mdStatistics.tcpMd.numProtErr);
		printf("Number of received MD packets with wrong topo count : %u \n", mdStatistics.tcpMd.numTopoErr);
		printf("Number of received MD packets without listener: %u \n", mdStatistics.tcpMd.numNoListener);
		printf("Number of reply timeouts: %u \n", mdStatistics.tcpMd.numReplyTimeout);
		printf("Number of confirm timeouts: %u \n", mdStatistics.tcpMd.numConfirmTimeout);
		printf("Number of sent MD packets: %u \n", mdStatistics.tcpMd.numSend);
	}
	else
	{
		return MD_APP_ERR;
	}
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display MD Caller Receive Count
 *
 *  @param[in]      pHeadCommandValue	pointer to head of queue
 *  @param[in]      addr						Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_PARAM_ERR					parameter	error
 *
 */
MD_APP_ERR_TYPE printCallerResult (
		COMMAND_VALUE	*pHeadCommandValue)
{
	COMMAND_VALUE *iterCommandValue;
	UINT16 commnadValueNumber = 1;
	char strIp[16] = {0};

    if (pHeadCommandValue == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

    for (iterCommandValue = pHeadCommandValue;
    	  iterCommandValue != NULL;
    	  iterCommandValue = iterCommandValue->pNextCommandValue)
    {
		/* Check Valid Caller */
		if ((iterCommandValue->mdCallerReplierType == CALLER) && (pHeadCommandValue->mdDestinationAddress != 0))
		{
	    	/*  Dump CommandValue */
			printf("Caller No.%u\n", commnadValueNumber);
			printf("-c,	Transport Type (UDP:0, TCP:1): %u\n", iterCommandValue->mdTransportType);
			printf("-d,	Caller Request Message Type (Mn:0, Mr-Mp:1): %u\n", iterCommandValue->mdMessageKind);
			printf("-e,	Caller Send MD DATASET Telegram Type (Increment:0, Fixed:1-6, Error:7-10): %u\n", iterCommandValue->mdTelegramType);
			printf("-f,	MD Increment Message Size Byte: %u\n", iterCommandValue->mdMessageSize);
			miscIpToString(iterCommandValue->mdDestinationAddress, strIp);
			printf("-g,	Caller MD Send Destination IP Address: %s\n", strIp);
//			printf("-i,	Dump Type (DumpOn:1, DumpOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdDump);
			printf("-j,	Caller known MD Replier Number: %u\n", iterCommandValue->mdReplierNumber);
			printf("-k,	Caller MD Request Send Cycle Number: %u\n", iterCommandValue->mdCycleNumber);
//			printf("-l,	Log Type (LogFileOn:1, LogFileOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdLog);
			printf("-m,	Caller MD Request Send Cycle Time: %u micro sec\n", iterCommandValue->mdCycleTime);
			printf("-n,	Topology TYpe (Ladder:1, not Lader:0): %u\n", iterCommandValue->mdLadderTopologyFlag);
			printf("-p,	Marshalling Type (Marshall:1, not Marshall:0): %u\n", iterCommandValue->mdMarshallingFlag);
			printf("-r,	Reply TImeout: %u micro sec\n", iterCommandValue->mdTimeoutReply);
			printf("-t,	Caller Using Network I/F (Subnet1:1,subnet2:2): %u\n", iterCommandValue->mdSendSubnet);
			printf("Caller Receive MD Count: %u\n", iterCommandValue->callerMdReceiveCounter);
			printf("Caller Receive MD Success Count: %u\n", iterCommandValue->callerMdReceiveSuccessCounter);
			printf("Caller Receive MD Failure Count: %u\n", iterCommandValue->callerMdReceiveFailureCounter);
			printf("Caller Retry Count: %u\n", iterCommandValue->callerMdRetryCounter);
			commnadValueNumber++;
		}
    }

    if (commnadValueNumber == 1 )
    {
    	printf("Valid Subscriber MD Command isn't Set up\n");
    }

    return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display MD Replier Receive Count
 *
 *  @param[in]      pHeadCommandValue	pointer to head of queue
 *  @param[in]      addr						Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_PARAM_ERR					parameter	error
 *
 */
MD_APP_ERR_TYPE printReplierResult (
		COMMAND_VALUE	*pHeadCommandValue)
{
	COMMAND_VALUE *iterCommandValue;
	UINT16 commnadValueNumber = 1;
	char strIp[16] = {0};

    if (pHeadCommandValue == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

    for (iterCommandValue = pHeadCommandValue;
    	  iterCommandValue != NULL;
    	  iterCommandValue = iterCommandValue->pNextCommandValue)
    {
		/* Check Valid Replier */
		if ((iterCommandValue->mdCallerReplierType == REPLIER) && (pHeadCommandValue->mdAddListenerComId != 0))
		{
	    	/*  Dump CommandValue */
			printf("Replier No.%u\n", commnadValueNumber);
			printf("-c,	Transport Type (UDP:0, TCP:1): %u\n", iterCommandValue->mdTransportType);
			miscIpToString(iterCommandValue->mdDestinationAddress, strIp);
			printf("-g,	Replier MD Receive Destination IP Address: %s\n", strIp);
//			printf("-i,	Dump Type (DumpOn:1, DumpOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdDump);
			printf("-k,	Replier MD Request Receive Cycle Number: %u\n", iterCommandValue->mdCycleNumber);
//			printf("-l,	Log Type (LogFileOn:1, LogFileOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdLog);
			printf("-n,	Topology TYpe (Ladder:1, not Lader:0): %u\n", iterCommandValue->mdLadderTopologyFlag);
			printf("-o,	Replier MD Reply Error Type(1-6): %u\n", iterCommandValue->mdReplyErr);
			printf("-p,	Marshalling Type (Marshall:1, not Marshall:0): %u\n", iterCommandValue->mdMarshallingFlag);
			printf("-q,	Replier Add Listener ComId: %u\n", iterCommandValue->mdReplierNumber);
//			printf("-r,	Reply TImeout: %u micro sec\n", iterCommandValue->mdTimeoutReply);
			printf("Replier Receive MD Count: %u\n", iterCommandValue->callerMdReceiveCounter);
			printf("Replier Receive MD Success Count: %u\n", iterCommandValue->callerMdReceiveSuccessCounter);
			printf("Replier Receive MD Failure Count: %u\n", iterCommandValue->callerMdReceiveFailureCounter);
			printf("Replier Retry Count: %u\n", iterCommandValue->callerMdRetryCounter);
			commnadValueNumber++;
		}
    }

    if (commnadValueNumber == 1 )
    {
    	printf("Valid Subscriber MD Command isn't Set up\n");
    }

    return MD_APP_NO_ERR;
}
