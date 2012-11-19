/**********************************************************************************************************************/
/**
 * @file            echoMD.c
 *
 * @brief           Demo TRDP Message Data
 *
 * @details			Receive and send message data
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Gari Oiarbide, CAF
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright CAF, Spain, 2012.
 *
 */


/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/time.h>
#include <sys/select.h>

#include <mqueue.h>
#include <errno.h>

#include "trdp_if_light.h"
//#include "vos_thread.h"
#include "trdp_private.h"

#ifdef __linux__
	#include <uuid/uuid.h>
#endif


#define MD_DEFAULT_IP_1        0xC0A866B3 /* Sender's IP: 192.168.102.179	Decimal: 3232261811	/ 10.0.0.203 - 0xA0000CB*/
#define MD_DEFAULT_IP_2        0xC0A866B4 /* Caller's IP: 192.168.102.180	Decimal: 3232261811	/ 10.0.0.204 - 0xA0000CC*/

#define SOURCE_URI "user@host"
#define DEST_URI "user@host"


/* We use dynamic memory	*/
#define HEAP_MEMORY_SIZE (1*1024*1024)

/* Q name */
#define TRDP_QUEUE_NAME "/trdp_queue"
#define TRDP_QUEUE_MAX_SIZE (sizeof(trdp_apl_cbenv_t)-2)
#define TRDP_QUEUE_MAX_MESG 10


#ifndef EOK
	#define EOK 0
#endif

/* message queue trdp to application */
typedef struct
{
	void                 *pRefCon;
	TRDP_MD_INFO_T        Msg;
	UINT8                *pData;
	UINT32               dataSize;
	int                  dummy;
} trdp_apl_cbenv_t;


typedef enum
{
    TRDP_SENT    		= 1,
    TRDP_RECEIVED	    = 2
} TRDP_SENT_T;

typedef struct
{
	TRDP_MSG_T msgType;
	TRDP_SENT_T sent_recv_prep;
}TRDP_SESSION_STATE;


typedef struct
{
	TRDP_UUID_T pSessionId;
	TRDP_IP_ADDR_T srcIp;
	UINT32 comId;
	TRDP_SESSION_STATE sessionState;
}APP_SESSION_TEST_T;

typedef struct
{
    UINT32 cnt;          /* lifesign  counter */
    char   testId[16];   /* test identification in ASCII */
} TRDP_MD_TEST_DS_T;

/* global vars */

static TRDP_MD_CONFIG_T   md_config;
static TRDP_MEM_CONFIG_T  mem_config;

static APP_SESSION_TEST_T APP_SESSION_TEST[TRDP_QUEUE_MAX_MESG];// = { -1 };

static UINT32 count_session = 0;

static TRDP_APP_SESSION_T  appHandle;  /*	Our identifier to the library instance	*/


static int x_reqconf ;
static mqd_t trdp_mq;

static TRDP_IP_ADDR_T	MD_COMID1_SRC_IP;
static TRDP_IP_ADDR_T	MD_COMID1_DST_IP;

BOOL app_queue_used;
CHAR8 gBuffer[32];



void get_IPs()
{
	INT32 read_data;

	printf("What is your PC?  PC-1[1] / PC-2[2] \n");
	scanf("%d", &read_data);
	if(read_data == 1)
	{
		MD_COMID1_SRC_IP = MD_DEFAULT_IP_1;
		MD_COMID1_DST_IP = MD_DEFAULT_IP_2;
	}else
	{
		MD_COMID1_SRC_IP = MD_DEFAULT_IP_2;
		MD_COMID1_DST_IP = MD_DEFAULT_IP_1;
	}

	printf(" The IPs configured correctly\n");
}



/* QUEUE FUNCTIONS */

/* send message trough queue */
static void queue_sendmessage(trdp_apl_cbenv_t * msg)
{
	int rc;
	char * p_bf = (char *)msg;
	int    l_bf = sizeof(*msg) - sizeof(msg->dummy);
	rc = mq_send(trdp_mq,p_bf,l_bf,0);
	if (-1 == rc)
	{
		perror("mq_send()");
		exit(EXIT_FAILURE);
	}else
	{
		printf("Message (comId = %d) added in the application queue\n"
				, msg->Msg.comId);
	}
}


/* call back function for message data */
static void md_indication(
    void                 *pRefCon,
    const TRDP_MD_INFO_T *pMsg,
    UINT8                *pData,
    UINT32               dataSize)
{
	//printf("md_indication(r=%p m=%p d=%p l=%d)\n",pRefCon,pMsg,pData,dataSize);

	trdp_apl_cbenv_t fwd;

	fwd.pRefCon  = pRefCon;
	fwd.Msg      = * pMsg;
	fwd.pData    = pData;
	fwd.dataSize = dataSize;

	queue_sendmessage(&fwd);

}


static void queue_initialize()
{
	struct mq_attr new_ma;
	struct mq_attr old_ma;
	struct mq_attr * pma;
	int rc;

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

	printf("pma=%p\n",pma);

	/* create a queue */
	trdp_mq = mq_open(TRDP_QUEUE_NAME, O_RDWR | O_CREAT, S_IWUSR|S_IRUSR, pma);
	if ((mqd_t)-1 == trdp_mq)
	{
		perror("mq_open()");
		exit(EXIT_FAILURE);
	}
	/* get attirubtes */
	rc = mq_getattr(trdp_mq,&old_ma);
	if (-1 == rc)
	{
		perror("mq_getattr()");
		exit(EXIT_FAILURE);
	}
	printf("mq_flags   = x%lX\n",old_ma.mq_flags  );
	printf("mq_maxmsg  = %ld\n",old_ma.mq_maxmsg );
	printf("mq_msgsize = %ld\n",old_ma.mq_msgsize);
	printf("mq_curmsgs = %ld\n",old_ma.mq_curmsgs);

	new_ma = old_ma;

	new_ma.mq_flags = O_NONBLOCK;

	/* change attributes */
	rc = mq_setattr(trdp_mq,&new_ma,&old_ma);
	if (-1 == rc)
	{
		perror("mq_setattr()");
		exit(EXIT_FAILURE);
	}

	/* get attirubtes */
	rc = mq_getattr(trdp_mq,&old_ma);
	if (-1 == rc)
	{
		perror("mq_getattr()");
		exit(EXIT_FAILURE);
	}
	printf("mq_flags   = x%lX\n",old_ma.mq_flags  );
	printf("mq_maxmsg  = %ld\n",old_ma.mq_maxmsg );
	printf("mq_msgsize = %ld\n",old_ma.mq_msgsize);
	printf("mq_curmsgs = %ld\n",old_ma.mq_curmsgs);
}


/* receive message from queue */
static int queue_receivemessage(trdp_apl_cbenv_t * msg)
{
	ssize_t rc;
	char * p_bf = (char *)msg;
	int    l_bf = sizeof(*msg) - sizeof(msg->dummy);
	int    s_bf = sizeof(*msg) - 1;
	unsigned msg_prio;

	msg_prio = 0;
	rc = mq_receive(trdp_mq,p_bf,s_bf,&msg_prio);
	if ((ssize_t)-1 == rc)
	{
		if (EAGAIN == errno)
		{
			return errno;
		}
		perror("mq_receive()");
		exit(EXIT_FAILURE);
	}
	if (rc != l_bf)
	{
		fprintf(stderr,"mq_receive() expected %d bytes, not %d\n",l_bf,(int)rc);
		exit(EXIT_FAILURE);
	}
	printf("Received %d bytes\n",(int)rc);
	return EOK;
}

/* MD application server */
static void queue_procricz()
{
	trdp_apl_cbenv_t msg;
	int rc = queue_receivemessage(&msg);
	if (rc != EOK) return;

	{
		myMDcallBack(msg.Msg.pUserRef, msg.Msg,	msg.pData,
								msg.dataSize);
	}
}



/* Delete MD session */
void delete_session(const TRDP_MD_INFO_T *pMsg)
{
	INT32 index;
	INT32 found_index;

	for(index=0; index < count_session; index++)
	{
		if(memcmp(APP_SESSION_TEST[index].pSessionId, pMsg->sessionId, sizeof(TRDP_UUID_T)) == 0)
		{
			found_index = index;
			break;
		}
	}

	memset(&APP_SESSION_TEST[found_index], 0, sizeof(APP_SESSION_TEST_T));

	for(index = found_index; found_index < (count_session - 1); found_index++)
	{
		memcpy(&APP_SESSION_TEST[index], &APP_SESSION_TEST[index+1], sizeof(APP_SESSION_TEST_T));
	}

	count_session--;
}

/* Update the session state */
void tlm_digest_session(const TRDP_MD_INFO_T *pMsg)
{
	UINT32 index;
	BOOL session_exist = FALSE;
	TRDP_IP_ADDR_T tmp_ip;


	/* Request message received */
	if(pMsg->msgType == TRDP_MSG_MR)
	{
		printf("A new Request message (comId=%d) received\n", pMsg->comId);
		APP_SESSION_TEST[count_session].sessionState.sent_recv_prep = TRDP_RECEIVED;
		APP_SESSION_TEST[count_session].sessionState.msgType = pMsg->msgType;
		APP_SESSION_TEST[count_session].comId = pMsg->comId;
		memcpy(APP_SESSION_TEST[count_session].pSessionId, pMsg->sessionId, sizeof(TRDP_UUID_T));
		APP_SESSION_TEST[count_session].srcIp = pMsg->srcIpAddr;
		count_session++;
	}



	for(index=0; index < count_session; index++)
	{

		if(memcmp(APP_SESSION_TEST[index].pSessionId, pMsg->sessionId, sizeof(TRDP_UUID_T)) == 0)
		{

			if(pMsg->msgType == TRDP_MSG_MP)
			{
				/* Reply message has been sent */
				printf("Session-Handling - Reply without confirm message (comId=%d) sent\n", pMsg->comId);
				APP_SESSION_TEST[index].sessionState.sent_recv_prep = TRDP_SENT;
				APP_SESSION_TEST[index].sessionState.msgType = pMsg->msgType;
				delete_session(pMsg);

			}else if(pMsg->msgType == TRDP_MSG_ME)
			{
				/* Reply Error message has been sent */
				printf("Session-Handling - Reply Error message (comId=%d) sent\n", pMsg->comId);
				APP_SESSION_TEST[index].sessionState.sent_recv_prep = TRDP_SENT;
				APP_SESSION_TEST[index].sessionState.msgType = pMsg->msgType;
				delete_session(pMsg);

			}else if(pMsg->msgType == TRDP_MSG_MQ)
			{
				/* Reply with confirm message has been sent */
				printf("Session-Handling - Reply with confirm (comId=%d) sent\n", pMsg->comId);
				APP_SESSION_TEST[index].sessionState.sent_recv_prep = TRDP_SENT;
				APP_SESSION_TEST[index].sessionState.msgType = pMsg->msgType;

			}else if(pMsg->msgType == TRDP_MSG_MC)
			{
				/* Confirm message has been sent */
				printf("Session-Handling - Confirm message (comId=%d) sent\n", APP_SESSION_TEST[index].comId);
				APP_SESSION_TEST[index].sessionState.sent_recv_prep = TRDP_SENT;
				APP_SESSION_TEST[index].sessionState.msgType = pMsg->msgType;
				delete_session(pMsg);

			}else
			{
				/* Wrong message received */
				printf("Session-Handling - Wrong message.\n");
				printf("The session will be aborted.\n");
				delete_session(pMsg);
			}

			break;

		}

	}

}

/* Check if there is any received message that is waiting the response */
BOOL prep_toSend(TRDP_MSG_T type)
{
	INT32 index;

	for(index = 0; index < count_session;index++)
	{
		if(type == TRDP_MSG_MR)
		{
			//Reply

			if((APP_SESSION_TEST[index].sessionState.msgType == type)
				&& (APP_SESSION_TEST[index].sessionState.sent_recv_prep == TRDP_RECEIVED))
			{
				return TRUE;
			}
		}else if(type == TRDP_MSG_MQ)
		{
			//Confirm
			if((APP_SESSION_TEST[index].sessionState.msgType == type)
				&& (APP_SESSION_TEST[index].sessionState.sent_recv_prep == TRDP_RECEIVED))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}



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
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pRefCon			user supplied context pointer
 *  @param[in]      pMsg			pointer to header/packet infos
 *  @param[in]      pData			pointer to data block
 *  @param[in]      dataSize		pointer to data size
 *  @retval         none
 */
void myMDcallBack (
    void                    *pRefCon,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{

	CHAR8 not_ini[TRDP_QUEUE_MAX_MESG];
	memset(not_ini, 0, sizeof(not_ini));
	UINT32 index;
	UINT32 found_index;
	BOOL session_exist = FALSE;

    /*	Check why we have been called	*/
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
        	switch (pMsg->msgType)
        	{

        		case TRDP_MSG_MN:
					printf("ComID %d Notify\n", pMsg->comId);
					if (pData)
					{
						memcpy(gBuffer, pData,
							   ((sizeof(gBuffer) <
								 dataSize) ? sizeof(gBuffer) : dataSize));
					}
					break;

        		case TRDP_MSG_MR:
					printf("ComID %d Request\n", pMsg->comId);
					if (pData)
					{
						memcpy(gBuffer, pData,
							   ((sizeof(gBuffer) <
								 dataSize) ? sizeof(gBuffer) : dataSize));

					}
					tlm_digest_session(pMsg);
					break;

        		case TRDP_MSG_MP:
					printf("ComID %d Reply without confirmation\n", pMsg->comId);
					if (pData)
					{
						memcpy(gBuffer, pData,
							   ((sizeof(gBuffer) <
								 dataSize) ? sizeof(gBuffer) : dataSize));

					}
					tlm_digest_session(pMsg);
					break;

        		case TRDP_MSG_ME:
					printf("ComID %d Reply Error\n", pMsg->comId);
					if (pData)
					{
						memcpy(gBuffer, pData,
							   ((sizeof(gBuffer) <
								 dataSize) ? sizeof(gBuffer) : dataSize));

					}
					tlm_digest_session(pMsg);
					break;


        		case TRDP_MSG_MQ:
					printf("ComID %d Reply with confirmation\n", pMsg->comId);
					if (pData)
					{
						memcpy(gBuffer, pData,
							   ((sizeof(gBuffer) <
								 dataSize) ? sizeof(gBuffer) : dataSize));
					}
					tlm_digest_session(pMsg);
					break;

        		case TRDP_MSG_MC:
					printf("ComID %d Confirm\n", pMsg->comId);
					if (pData)
					{
						memcpy(gBuffer, pData,
							   ((sizeof(gBuffer) <
								 dataSize) ? sizeof(gBuffer) : dataSize));
					}
					tlm_digest_session(pMsg);
					break;

        		default:
        			printf("Error on packet (ComID %d), err = %d\n",
						   pMsg->comId,
						   pMsg->resultCode);
					break;
    		}
        	break;

			default:
				printf("Error on packet (ComID %d), err = %d\n",
					   pMsg->comId,
					   pMsg->resultCode);
				break;
    }
    if(memcmp(gBuffer, not_ini, sizeof(not_ini)) != 0)
    {
    	printf("Data: %s\n\n", gBuffer);
    }

    memset(gBuffer, 0, sizeof(gBuffer));
}


TRDP_ERR_T init_trdp(TRDP_LIS_T *listenHandle, UINT32 *listeners_count, fd_set*  rfds)
{
	UINT32 read_data = 1;
	UINT32 MD_listen_COMID[TRDP_QUEUE_MAX_MESG];
	TRDP_ERR_T err;
	TRDP_PROCESS_CONFIG_T pProcessConfig;

	pProcessConfig.options = TRDP_OPTION_NONE;

	/* memory allocator config */
		mem_config.p    = NULL;
		mem_config.size = HEAP_MEMORY_SIZE;

		md_config.pRefCon      = (void *)0x12345678;
		md_config.sendParam.qos = MD_DEFAULT_QOS;
		md_config.sendParam.ttl = MD_DEFAULT_TTL;
		md_config.flags = 0
			| TRDP_FLAGS_NONE      * 0
			| TRDP_FLAGS_REDUNDANT * 0
			| TRDP_FLAGS_MARSHALL  * 0
			| TRDP_FLAGS_CALLBACK  * 1
			| TRDP_FLAGS_TCP       * 1 /* 1=TCP, 0=UDP */
			;
		md_config.replyTimeout   = MD_DEFAULT_REPLY_TIMEOUT;
		md_config.confirmTimeout = MD_DEFAULT_CONFIRM_TIMEOUT;
		md_config.udpPort        = IP_MD_UDP_PORT;
		md_config.tcpPort        = IP_MD_TCP_PORT;


		printf("Do you want to use the application queue to process the msgs? Yes[1] / No[0] \n");
		scanf("%d", &read_data);

		/* MD config */
		if(read_data == 1)
		{
			/* App queue will be used */
			app_queue_used = TRUE;
			queue_initialize();
			md_config.pfCbFunction = md_indication;
		}else
		{
			app_queue_used = FALSE;
			/* Msgs will be processed in the callback call */
			md_config.pfCbFunction = myMDcallBack;
		}

	  /*	Init the library for callback operation	(PD only) */

		if(tlc_init(dbgOut, &mem_config) != TRDP_NO_ERR)
		{
			 printf("Initialization error\n");
			 return TRDP_INIT_ERR;
		}


	    if (tlc_openSession(
	    		&appHandle,
	    		MD_COMID1_SRC_IP, 	  /* use default IP addresses */
	    		MD_COMID1_SRC_IP,     /* use default IP addresses */
	            NULL,                 /* no Marshalling	*/
	            NULL,
	            &md_config,            /* system defaults for PD and MD	*/
	            &pProcessConfig) != TRDP_NO_ERR)
	    {
	        printf("Initialization error\n");
	        return TRDP_INIT_ERR;
	    }else
	    {
	    	printf("-------- Initialization OK ------\n");
	    }

	    /*	AddListener		*/

	    memset(gBuffer, 0, sizeof(gBuffer));

	    printf("Do you want to add the Listener?\n Yes[1] / No[0]\n");
	    scanf("%d", &read_data);

	    while(read_data == 1)
	    {

			printf("Which is the comId that you want to add the Listener?\n");
			scanf("%d", &MD_listen_COMID[*listeners_count]);

			err = tlm_addListener( appHandle,                 				/*	our application identifier			*/
			                         &(listenHandle[*listeners_count]), 	/*	our subscription identifier			*/
			                         NULL,
			                         MD_listen_COMID[*listeners_count],     /*	ComID								*/
			                         0,                         			/*	topocount: local consist only		*/
			                         //MD_COMID1_SRC_IP,
			                         MD_COMID1_DST_IP,                      /*	Destination IP   					*/
			                         //sizeof(gBuffer),						/*	maxDataSize */
			                         TRDP_FLAGS_TCP,          				/*	TRDP_FLAGS_T						*/
			    					 "");									/*	destURI*/

			if (err != TRDP_NO_ERR)
			{
				printf("prep md receive error (comId = %d)\n", MD_listen_COMID[*listeners_count]);
				return err;
			}else
			{
				printf("The listener comId = %d has been added correctly.\n", MD_listen_COMID[*listeners_count]);
			}

			printf("Do you want to continue adding listeners?\n Yes[1] / No[0]\n");
			scanf("%d", &read_data);

			(*listeners_count) ++;

	    }

		FD_ZERO(rfds);
		return err;
}

TRDP_ERR_T notifies_requests()
{
	UINT32 read_data = 1;
	UINT32 MD_COMID=0;
	TRDP_ERR_T err = TRDP_NO_ERR;
    UINT32	noOfRepliers = 0;

	read_data = 0;
	printf("Do you want to send any notify or request msg? Yes[1] / No[0]\n");
	scanf("%d", &read_data);

	while(read_data == 1)
	{
		read_data=0;
		TRDP_UUID_T pSessionId;
		memset(gBuffer, 0, sizeof(gBuffer));
		printf("Specify the kind of message you want to send. ComId:\n");
		scanf("%d",&MD_COMID);
		printf("Enter: 1 for notify, 2 for request\n");
		scanf("%d", &read_data);

		printf("Enter the message data to send (Maximum 32 characters):\n");
		scanf("%s", gBuffer);

		err = vos_getUuid(pSessionId);

		if(err != TRDP_NO_ERR)
		{
			return TRDP_INIT_ERR;
		}

		if(read_data == 1)
		{
			err = tlm_notify(appHandle,
									NULL,
									MD_COMID,
									0,
									MD_COMID1_SRC_IP,
									MD_COMID1_DST_IP,
									TRDP_FLAGS_TCP,
									NULL,
									(UINT8 *)gBuffer,
									sizeof(gBuffer),
									SOURCE_URI,
									DEST_URI);

		}else if(read_data == 2)
		{
			err = tlm_request(appHandle,
								NULL,
								pSessionId,
								MD_COMID,
								0,
								MD_COMID1_SRC_IP,
								MD_COMID1_DST_IP,
								TRDP_FLAGS_TCP,
								noOfRepliers,
								MD_DEFAULT_REPLY_TIMEOUT,
								NULL,
								(UINT8 *)gBuffer,
								sizeof(gBuffer),
								SOURCE_URI,
								DEST_URI);
		}

		if (err != TRDP_NO_ERR)
		{
			printf("MD prepare error (ComId = %d)\n",MD_COMID);
			return err;
		}

		if(read_data == 2)
		{
			printf("A new Request message (comId=%d) prepared\n", MD_COMID);
			APP_SESSION_TEST[count_session].sessionState.sent_recv_prep = TRDP_SENT;
			APP_SESSION_TEST[count_session].sessionState.msgType = TRDP_MSG_MR;
			APP_SESSION_TEST[count_session].comId = MD_COMID;
			memcpy(APP_SESSION_TEST[count_session].pSessionId, pSessionId, sizeof(TRDP_UUID_T));
			APP_SESSION_TEST[count_session].srcIp = MD_COMID1_DST_IP;
			count_session++;
			printf("The request saved in the - App Session Handling -\n");
		}

		read_data=0;
		MD_COMID=0;
		printf("Do you want to send more messages? Yes[1] / No[0]\n");
		scanf("%d", &read_data);
	}
	return err;
}


TRDP_ERR_T reply_msgs()
{
	BOOL to_reply;
	UINT32 replies_count=0;
	UINT32 comid_num;
	UINT32 read_data = 1;
    UINT32 found_index;
    UINT32 answer_kind;
    UINT32 session_num;
	TRDP_ERR_T err = TRDP_NO_ERR;

	to_reply = prep_toSend(TRDP_MSG_MR);
	read_data=0;
	session_num =0;

	if(to_reply == TRUE)
	{
		printf("You have those connections (comId's) to reply.\n");
		for(found_index = 0; found_index < count_session;found_index++)
		{
			if((APP_SESSION_TEST[found_index].sessionState.msgType == (TRDP_MSG_MR))
				&& (APP_SESSION_TEST[found_index].sessionState.sent_recv_prep == TRDP_RECEIVED))
			{
				printf("%d.ComId=%d  |  ", (session_num), APP_SESSION_TEST[found_index].comId);
				replies_count++;
			}
			session_num ++;
		}
		printf("\n Do you want to reply any of those? Yes[1] / No[0]\n");
		scanf("%d", &read_data);
	}

	while(read_data)
	{
		printf("Which one? Enter the number (1,2...): \n");
		scanf("%d", &session_num);

		printf("Which kind of reply? 0-REPLY | 1-REPLY QUERY | 2-REPLY ERR\n");
		scanf("%d", &answer_kind);

		if((answer_kind == 0) || (answer_kind == 1))
		{
			printf("Enter the message data to send (Maximum 32 characters):\n");
			scanf("%s", gBuffer);
		}

		switch (answer_kind)
		{
			case 0:
				err = tlm_reply(appHandle,
							NULL,
							&(APP_SESSION_TEST[(session_num)].pSessionId),
							0,
							APP_SESSION_TEST[(session_num)].comId,
							MD_COMID1_SRC_IP,
							MD_COMID1_DST_IP,
							TRDP_FLAGS_TCP,
							NULL, //UINT16                  userStatus,
							NULL,
							(UINT8 *)gBuffer,
							sizeof(gBuffer),
							SOURCE_URI,
							DEST_URI);

				break;

			case 1:
				err = tlm_replyQuery(appHandle,
							NULL,
							&(APP_SESSION_TEST[(session_num)].pSessionId),
							0,
							APP_SESSION_TEST[(session_num)].comId,
							MD_COMID1_SRC_IP,
							MD_COMID1_DST_IP,
							TRDP_FLAGS_TCP,
							NULL, //UINT16                  userStatus,
							MD_DEFAULT_CONFIRM_TIMEOUT,
							NULL,
							(UINT8 *)gBuffer,
							sizeof(gBuffer),
							SOURCE_URI,
							DEST_URI);

				break;

			case 2:
				err = tlm_replyErr(appHandle,
							&(APP_SESSION_TEST[(session_num)].pSessionId),
							0,
							APP_SESSION_TEST[(session_num)].comId,
							MD_COMID1_SRC_IP,
							MD_COMID1_DST_IP,
							TRDP_REPLY_NO_REPLY,
							NULL,
							SOURCE_URI,
							DEST_URI);

				break;

			default:
				printf("You have entered bad value.\n");
				break;
		}

		if (err != TRDP_NO_ERR)
		{
			printf("MD reply error\n");
			return err;
		}

		replies_count--;

		if(replies_count > 0)
		{
			printf("Do you want to reply another one? Yes[1] / No[0]\n");
			scanf("%d", &read_data);
		}else
		{
			read_data = 0;
		}
	}

	return err;
}


TRDP_ERR_T confirm_msgs()
{
	BOOL to_reply;
	UINT32 read_data = 1;
	UINT32 comid_num;
    UINT32 found_index;
    UINT32 confirms_count=0;
	TRDP_ERR_T err = TRDP_NO_ERR;
	TRDP_UUID_T pSessionId;
	UINT32 session_num;

	to_reply = prep_toSend(TRDP_MSG_MQ);
	read_data=0;
	session_num=0;

	if(to_reply == TRUE)
	{
		printf("You have those connections (comId's) to send the confirm.\n");
		for(found_index = 0; found_index < count_session;found_index++)
		{
			if((APP_SESSION_TEST[found_index].sessionState.msgType == (TRDP_MSG_MQ))
				&& (APP_SESSION_TEST[found_index].sessionState.sent_recv_prep == TRDP_RECEIVED))
			{
				printf("%d.ComId=%d  |  ", (session_num), APP_SESSION_TEST[found_index].comId);
				confirms_count++;
			}
			session_num++;
		}
		printf("\nDo you want to send a confirm of any of those? Yes[1] / No[0]\n");
		scanf("%d", &read_data);
	}

	while(read_data)
	{
		printf("Which one? Enter the number (1,2...): \n");
		scanf("%d", &session_num);

		if(err != TRDP_NO_ERR)
		{
			return TRDP_INIT_ERR;
		}

		err = tlm_confirm(appHandle,
				NULL,
				&(APP_SESSION_TEST[(session_num)].pSessionId),
				APP_SESSION_TEST[(session_num)].comId,
				0,
				MD_COMID1_SRC_IP,
				MD_COMID1_DST_IP,
				TRDP_FLAGS_TCP,
				NULL, //UINT16                  userStatus,
				TRDP_REPLY_NO_REPLY,
				NULL,
				SOURCE_URI,
				DEST_URI);

		if (err != TRDP_NO_ERR)
		{
			printf("MD confirm error\n");
			return err;
		}

		confirms_count--;

		if(confirms_count > 0)
		{
			printf("Do you want to send another confirm message? Yes[1] / No[0]\n");
			scanf("%d", &read_data);
		}else
		{
			read_data = 0;
		}

	}
	return err;
}


/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (int argc, char * *argv)
{
    TRDP_LIS_T          listenHandle[TRDP_QUEUE_MAX_MESG];  /*	Array of identifier to the listeners	*/
    TRDP_PUB_T          pubHandle;  /*	Our identifier to the publication	*/
    TRDP_ERR_T          err;
    int rv = 0;
    fd_set  rfds;
    INT32   noOfDesc;
    UINT32 continue_looping = 1;
    struct timeval  tv;
    struct timeval  max_tv = {0, 100000};

    UINT32 listeners_count = 0;

    /* Configure the Ip */
    get_IPs();

    /*	Init the TRDP and addListeners	*/
    err = init_trdp(listenHandle, &listeners_count, &rfds);

    if(err != TRDP_NO_ERR)
    {
    	return 1;
    }


    while (continue_looping == 1)
    {
    	/* Send notify or request messages */
    	err = notifies_requests();
    	if(err != TRDP_NO_ERR)
		{
			return 1;
		}


    	/* Send reply messages */
    	reply_msgs();
    	if(err != TRDP_NO_ERR)
		{
			return 1;
		}

    	/* Send confirm messages */
    	confirm_msgs();
    	if(err != TRDP_NO_ERR)
    	{
    				return 1;
    	}



		tlc_getInterval(appHandle,
							   (TRDP_TIME_T *) &tv,
							   (TRDP_FDS_T *) &rfds,
							   &noOfDesc);


		if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0)
		{
				  tv = max_tv;
		}



		/*
		   Select() will wait for ready descriptors or timeout,
		   what ever comes first.
		*/
		rv = select((int)noOfDesc, &rfds, NULL, NULL, &tv);


		tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);


		if(app_queue_used == TRUE)
		{
			/* call back process */
			queue_procricz();
		}

		/* small delay */
		usleep(500*1000);

		/*
		  Handle other ready descriptors...
		*/
		if (rv > 0)
		{
		   printf("other descriptors were ready\n");
		}
		else
		{
		   printf("Do you want to continue? (looping) Yes[1] / No[0]\n");
		   scanf("%d",&continue_looping);
		}
    }

    printf("\n --- Cleaning the listeners and closing the session ---\n");

    int i = 0;
    for(i=0;i<listeners_count;i++)
    {
    	err = tlm_delListener(appHandle, &(listenHandle[i]));

    	if(err != TRDP_NO_ERR)
    	{
    		printf("Error deleting the listener %d\n", i);
    	}
    }

    printf("The TRDP session will be terminated\n");

    err = tlc_closeSession(appHandle);
    if(err != TRDP_NO_ERR)
	{
		printf("Error deleting the listener %d\n", i);
		return 1;
	}

    err = tlc_terminate();
    if(err != TRDP_NO_ERR)
	{
		printf("Error deleting the listener %d\n", i);
		return 1;
	}

    printf("TRDP session OK terminated\n");

	return 0;

}