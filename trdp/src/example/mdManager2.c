/**********************************************************************************************************************/
/**
 * @file	mdManager2.c
 *
 * @brief	Demo UDPMDCom application for TRDP
 *
 * @details	Receive and send process data, single threaded polling, static memory
 *
 * @note	Project: TCNOpen TRDP prototype stack
 *		Version 0.0: d.quagreda (FAR). Initial version.
 *		Version 0.1: s.pachera (FAR). Add log to file (l2f) to help debug and integration test.
 *		Version 0.2: s.pachera (FAR). Add command line interface (cli), add main loop period handling, add test mode
 *
 * @author	Quagred Diego (FAR Systems), Simone Pachera  (FAR Systems)
 *
 * @remarks	All rights reserved. Reproduction, modification, use or disclosure
 *		to third parties without express authority is forbidden,
 *		FAR Systems spa, Italy, 2013.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <sys/select.h>
#include <mqueue.h>
#include <time.h>

// SP 30/10/2012
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

#define TRDP_IP4_ADDR(a,b,c,d) ( (am_big_endian()) ? \
	((UINT32)((a) & 0xFF) << 24) | ((UINT32)((b) & 0xFF) << 16) | ((UINT32)((c) & 0xFF) << 8) | ((UINT32)((d) & 0xFF)) : \
	((UINT32)((d) & 0xFF) << 24) | ((UINT32)((c) & 0xFF) << 16) | ((UINT32)((b) & 0xFF) << 8) | ((UINT32)((a) & 0xFF)))

#define HEAP_MEMORY_SIZE (1*1024*1024)

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

#define local_max(a,b) ( ((a) > (b)) ? (a) : (b) )

/* Q name */
#define TRDP_QUEUE_NAME "/trdp_queue_4"
#define TRDP_QUEUE_MAX_SIZE (sizeof(trdp_apl_cbenv_t)-2)
#define TRDP_QUEUE_MAX_MESG 10


/* global vars */

static TRDP_APP_SESSION_T appHandle;
static TRDP_MD_CONFIG_T   md_config;
static TRDP_MEM_CONFIG_T  mem_config;

static TRDP_LIS_T lisHandle;

static mqd_t trdp_mq;


typedef struct 
{ 
    UINT32 cnt;          /* lifesign  counter */ 
    char   testId[16];   /* test identification in ASCII */ 
} TRDP_MD_TEST_DS_T;


/* command line arguments */
static int            x_uuid;
static int            x_notify;
static int            x_request;
static int            x_reqconf;
static int            x_receiver;
static TRDP_IP_ADDR_T x_ip4_dest;
static int            x_l2f;		// Enable log 2 file
static int            x_period;		// Main loop period in ms
static int            x_testmode;	// Test mode: dev1 o dev 2


// Command line
int	cliPipeFd[2];	// Communication pipe
pid_t   cliConsolePid;	// Console server PID

// *** Log2file: Start ***

// Global variables
int	l2fPipeFd[2];	// Communication pipe
pid_t   l2fWriterPid;	// Writer server PID


// Write log to file
int l2fFlush(char *logMsg)
{
    // Open file in append mode
    FILE *flog = fopen("trdp.log", "a");
    
    // Append string to file
    fprintf(flog, "%s\n", logMsg);

    // Close file
    fclose(flog);
    
    return 0;
}

// Log file writer
void l2fWriterServer()
{
    // Arbitrary buffer size
    char pipeMsg[1025];
	
    // Readed message size
    int pipeMsgSize = 0;
    
    // Close client side pipe
    close(l2fPipeFd[1]);

    // Write messages to file
    while(1)
    {
	// Read message from file
	pipeMsgSize = read(l2fPipeFd[0], pipeMsg, sizeof(pipeMsg));
	
	// Check for pipe close
	if(pipeMsgSize < 0)
	    exit(0);
	
	pipeMsg[1024] = '\0'; // Tap, in case of problems
	
	// Write to file
	l2fFlush(pipeMsg);
    }
}


// Init writer task
int l2fInit(void)
{   
    // Init server pipe
    pipe(l2fPipeFd);
    
    // Create file writer server
    l2fWriterPid = fork();
    
    // Fork ok, start server
    if(l2fWriterPid == 0)
    {
	// Server start
	l2fWriterServer();
	
	// Abnormal termination, server never end before application
	perror("l2fInit(): server abnormal exit.");
	exit(1);
    }
    // Fork error
    else if(l2fWriterPid == -1)
    {
	perror("l2fInit(): server fork error.");
	exit(1);
    }
    
    // Close server side pipe
    close(l2fPipeFd[0]);
    
    // Parent, return only
    return 0;
}


// Send log string to server
int l2fLog(char *logString)
{
    struct tm *logtm;
    struct timeval tv; 
    long milliseconds;

    // Skip log if no init done
    if(l2fWriterPid == 0)
	return 1;

    gettimeofday (&tv, NULL); 
    milliseconds = tv.tv_usec / 1000; 
    logtm = localtime(&tv.tv_sec);

    // Prepare string
    char tmpString[1024];
    sprintf(
	tmpString,
	"%04d-%02d-%02d %02d:%02d:%02d.%03ld - %s",
	1900 + logtm->tm_year, logtm->tm_mon + 1,
	logtm->tm_mday, logtm->tm_hour, logtm->tm_min, logtm->tm_sec,
	milliseconds,
	logString);

    // Send to server using pipe
    write(l2fPipeFd[1], tmpString, (strlen(tmpString)+1));
    
    return 0;
}

// Stop log
int l2fStop(void)
{
    // Kill l2f writer
    if(l2fWriterPid != 0)
    {
	int l2fKillRet;
	l2fKillRet = kill(l2fWriterPid, SIGKILL);
	if(l2fKillRet < 0)
	{
	    perror("l2fStop(): unable to kill writer task.");
	}
	else
	{    
	    int exitStatus;
	    waitpid(l2fWriterPid,&exitStatus, 0);
	}
    }

    // Close pipe
    close(l2fPipeFd[1]);
    
    return 0;
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
	
// Convert Session to String
char *miscSession2String(const UINT8 *p)
{
    char *strTmp;
    strTmp = (char *)malloc(sizeof(char) * 64);
 
    if(strTmp == NULL)
    {
	fprintf(stderr, "miscSession2String(): Could not allocate memory.\n");
	exit(1);
    }

    sprintf(strTmp,"%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
	p[0] & 0xFF,
	p[1] & 0xFF,
	p[2] & 0xFF,
	p[3] & 0xFF,
	p[4] & 0xFF,
	p[5] & 0xFF,
	p[6] & 0xFF,
	p[7] & 0xFF,
	p[8] & 0xFF,
	p[9] & 0xFF,
	p[10] & 0xFF,
	p[11] & 0xFF,
	p[12] & 0xFF,
	p[13] & 0xFF,
	p[14] & 0xFF,
	p[15] & 0xFF
    );

    return strTmp;
}

// Convert URI to String
char *miscUriToString(const CHAR8 *p)
#include "../api/trdp_types.h"
{
    int i;
    char *strTmp;
    strTmp = (char *)malloc(sizeof(char) *128);
 
    if(strTmp == NULL)
    {
	fprintf(stderr, "miscUriToString(): Could not allocate memory.\n");
	exit(1);
    }
    
    for(i=0;i<32;i++)
    {
	if (p[i] == 0)
	{
	    strTmp[i] = 0;
	    break;
	}
	strTmp[i] = (char) p[i] & 0xFF;
    }

    return strTmp;
}


// memory to string
// TODO
char *miscMem2String(const void *p, const int l)
{
    char *strTmp;
    strTmp = (char *)malloc(sizeof(char) * 1024);
 
    if(strTmp == NULL)
    {
	fprintf(stderr, "miscMem2String(): Could not allocate memory.\n");
	exit(1);
    }
    
    /*
    if (p != NULL || l > 0)
    {
	int i,j;
	const UINT8 *b = p;
	for(i = 0; i < l; i += 16)
	{
	    fprintf(stderr,"%04X ", i );
			
	    for(j = 0; j < 16; j++)
	    {
		if (j == 8)
		    fprintf(stderr,"- ");
		if((i+j) < l)
		{
		    int ch = b[i+j];
		    fprintf(stderr,"%02X ",ch);
		}
		else
		{
		    fprintf(stderr,"   ");
		}
	    }
	    
	    fprintf(stderr,"   ");
	    
	    for(j = 0; j < 16 && (i+j) < l; j++)
	    {
		int ch = b[i+j];
		fprintf(stderr,"%c", (ch >= ' ' && ch <= '~') ? ch : '.');
	    }
	    fprintf(stderr,"\n");
	}
    }
    */
    
    return strTmp;
}

// Dump message content
char *miscEnv2String(trdp_apl_cbenv_t msg)
{
    char *strTmp;
    strTmp = (char *)malloc(sizeof(char) * 128);
 
    if(strTmp == NULL)
    {
	fprintf(stderr, "miscEnv2String(): Could not allocate memory.\n");
	exit(1);
    }

    sprintf(strTmp,"md_indication(r=%p d=%p l=%d)\n",msg.pRefCon,msg.pData,msg.dataSize);
    /*
    sprintf(str02,"srcIpAddr         = x%08X\n",msg.Msg.srcIpAddr);
    sprintf(str03,"destIpAddr        = x%08X\n",msg.Msg.destIpAddr);
    sprintf(str04,"seqCount          = %d\n"   ,msg.Msg.seqCount);
    sprintf(str05,"protVersion       = %d\n"   ,msg.Msg.protVersion);
    sprintf(str06,"msgType           = x%04X\n",msg.Msg.msgType);
    sprintf(str07,"comId             = %d\n"   ,msg.Msg.comId);
    sprintf(str08,"topoCount         = %d\n"   ,msg.Msg.topoCount);
    sprintf(str09,"userStatus        = %d\n"   ,msg.Msg.userStatus);
    sprintf(str10,"replyStatus       = %d\n"   ,msg.Msg.replyStatus);
    sprintf(str11,"sessionId         = %s\n"   ,miscSession2String(msg.Msg.sessionId));
    sprintf(str12,"replyTimeout      = %d\n"   ,msg.Msg.replyTimeout);
    sprintf(str13,"destURI           = %s\n"   ,miscUriToString(msg.Msg.destURI));
    sprintf(str14,"srcURI            = %s\n"   ,miscUriToString(msg.Msg.srcURI));
    sprintf(str15,"noOfReplies       = %d\n"   ,msg.Msg.noOfReplies);
    sprintf(str16,"pUserRef          = %p\n"   ,msg.Msg.pUserRef);
    sprintf(str17,"resultCode        = %d\n"   ,msg.Msg.resultCode);
    str18 = miscMem2String(msg.pData,msg.dataSize);
    */
    
    return strTmp;
}

// *** Log2file: End ***

char trdpLogString[1024];

/* code */

/* debug display function */
static void private_debug_printf(
	void *pRefCon,
	VOS_LOG_T category,
	const CHAR8 *pTime,
	const CHAR8 *pFile,
	UINT16 LineNumber,
	const CHAR8 *pMsgStr)
{
	fprintf(stderr,"r=%p c=%d t=%s f=%s l=%d m=%s",pRefCon,category,pTime,pFile,LineNumber,pMsgStr);
	//char strTmp[2047];
	//sprintf(strTmp,"r=%p c=%d t=%s f=%s l=%d m=%s",pRefCon,category,pTime,pFile,LineNumber,pMsgStr);
	//l2fLog(strTmp);
}


// SP 31/10/2012: all log to strerr because stdin/out are used for cli
static void print_session(const UINT8 *p)
{
	int i;
	for(i=0;i<16;i++)
	{
		if (i == 4 || i == 6 || i == 8 || i == 10) fprintf(stderr,"-");
		fprintf(stderr,"%02X",p[i] & 0xFF);
	}
	fprintf(stderr,"\n");
}

static void print_uri(const CHAR8 *p)
{
	int i;
	for(i=0;i<32;i++)
	{
		if (p[i] == 0) break;
		fprintf(stderr,"%c",p[i] & 0xFF);
	}
}

static void print_memory(const void *p, const int l)
{
	if (p != NULL || l > 0)
	{
		int i,j;
		const UINT8 *b = p;
		for(i = 0; i < l; i += 16)
		{
			fprintf(stderr,"%04X ", i );
			
			for(j = 0; j < 16; j++)
			{
				if (j == 8) fprintf(stderr,"- ");
				if ((i+j) < l)
				{
					int ch = b[i+j];
					fprintf(stderr,"%02X ",ch);
				}
				else
				{
					fprintf(stderr,"   ");
				}
			}
			fprintf(stderr,"   ");
			for(j = 0; j < 16 && (i+j) < l; j++)
			{
				int ch = b[i+j];
				fprintf(stderr,"%c", (ch >= ' ' && ch <= '~') ? ch : '.');
			}
			fprintf(stderr,"\n");
		}
	}
}

/* queue functions */

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
	
	fprintf(stderr,"pma=%p\n",pma);

	/* create a queue */
	trdp_mq = mq_open(TRDP_QUEUE_NAME, O_RDWR | O_CREAT, S_IWUSR|S_IRUSR, pma);
	if ((mqd_t)-1 == trdp_mq)
	{
		fprintf(stderr,"mq_open()");
		exit(EXIT_FAILURE);
	}
	/* get attirubtes */
	rc = mq_getattr(trdp_mq,&old_ma);
	if (-1 == rc)
	{
		fprintf(stderr,"mq_getattr()");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,"mq_flags   = x%lX\n",old_ma.mq_flags  );
	fprintf(stderr,"mq_maxmsg  = %ld\n",old_ma.mq_maxmsg );
	fprintf(stderr,"mq_msgsize = %ld\n",old_ma.mq_msgsize);
	fprintf(stderr,"mq_curmsgs = %ld\n",old_ma.mq_curmsgs);
	
	new_ma = old_ma;
	
	new_ma.mq_flags = O_NONBLOCK;
	
	/* change attributes */
	rc = mq_setattr(trdp_mq,&new_ma,&old_ma);
	if (-1 == rc)
	{
		fprintf(stderr,"mq_setattr()");
		exit(EXIT_FAILURE);
	}
	
	/* get attirubtes */
	rc = mq_getattr(trdp_mq,&old_ma);
	if (-1 == rc)
	{
		fprintf(stderr,"mq_getattr()");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,"mq_flags   = x%lX\n",old_ma.mq_flags  );
	fprintf(stderr,"mq_maxmsg  = %ld\n",old_ma.mq_maxmsg );
	fprintf(stderr,"mq_msgsize = %ld\n",old_ma.mq_msgsize);
	fprintf(stderr,"mq_curmsgs = %ld\n",old_ma.mq_curmsgs);
}

/* send message trough queue */
static void queue_sendmessage(trdp_apl_cbenv_t * msg)
{
	int rc;
	char * p_bf = (char *)msg;
	int    l_bf = sizeof(*msg) - sizeof(msg->dummy);
	rc = mq_send(trdp_mq,p_bf,l_bf,0);
	if (-1 == rc)
	{
		fprintf(stderr,"mq_send()");
		exit(EXIT_FAILURE);
	}
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
		fprintf(stderr,"mq_receive()");
		exit(EXIT_FAILURE);
	}
	if (rc != l_bf)
	{
		fprintf(stderr,"mq_receive() expected %d bytes, not %d\n",l_bf,(int)rc);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,"Received %d bytes\n",(int)rc);
	return EOK;
}


// Send counters
static int testReplySendID = 0;
static int testReplyQSendID = 0;
static int testConfirmSendID = 0;


// MD application server
// Process queue elements pushed by call back function
static void queue_procricz()
{
    trdp_apl_cbenv_t msg;
    int rc = queue_receivemessage(&msg);
	
    if (rc != EOK) return;
    {
	// Message info
	fprintf(stderr,"md_indication(r=%p d=%p l=%d)\n",msg.pRefCon,msg.pData,msg.dataSize);
// 
	char strIp[16];
	fprintf(stderr,"srcIpAddr         = %s\n",  miscIpToString(msg.Msg.srcIpAddr, strIp));
	fprintf(stderr,"destIpAddr        = %s\n",  miscIpToString(msg.Msg.destIpAddr, strIp));
	fprintf(stderr,"seqCount          = %d\n"   ,msg.Msg.seqCount);
	fprintf(stderr,"protVersion       = %d\n"   ,msg.Msg.protVersion);
	fprintf(stderr,"msgType           = x%04X\n",msg.Msg.msgType);
	fprintf(stderr,"comId             = %d\n"   ,msg.Msg.comId);
	fprintf(stderr,"topoCount         = %d\n"   ,msg.Msg.topoCount);
	fprintf(stderr,"userStatus        = %d\n"   ,msg.Msg.userStatus);
	fprintf(stderr,"replyStatus       = %d\n"   ,msg.Msg.replyStatus);
	fprintf(stderr,"sessionId         = ");
	print_session(msg.Msg.sessionId);
	fprintf(stderr,"replyTimeout      = %d\n"   ,msg.Msg.replyTimeout);
	fprintf(stderr,"destURI           = ");      print_uri(msg.Msg.destURI); fprintf(stderr,"\n");
	fprintf(stderr,"srcURI            = ");      print_uri(msg.Msg.srcURI); fprintf(stderr,"\n");
	fprintf(stderr,"noOfReplies       = %d\n"   ,msg.Msg.noOfReplies);
	fprintf(stderr,"pUserRef          = %p\n"   ,msg.Msg.pUserRef);
	fprintf(stderr,"resultCode        = %d\n"   ,msg.Msg.resultCode);
	
	print_memory(msg.pData,msg.dataSize);
	
	// TODO
	//char *strTmp;
	//strTmp = miscEnv2String(msg);
	//l2fLog(strTmp);
	//free(strTmp);
	//strTmp = NULL;
    }

    // Dev 1
    if(x_testmode == 1)
    {
	switch(msg.Msg.comId)
	{
	    case 1001:
	    {
		// TEST-0001: no listener for comId 1001, non callback execution expected
		fprintf(stderr,"[TEST-0001: ERROR] queue_procricz(): no callback execution expected for comID 1001\n");
		l2fLog("[TEST-0001: ERROR] queue_procricz(): no callback execution expected for comID 1001.");
	    }
	    break;

	    case 1002:
	    {
		// TEST-0002: no listener for comId 1001, non callback execution expected
		fprintf(stderr,"[TEST-0002: ERROR] queue_procricz(): no callback execution expected for comID 1002\n");
		l2fLog("[TEST-0002: ERROR] queue_procricz(): no callback execution expected for comID 1002.");
	    }
	    break;
		
	    case 1003:
	    {
		// TEST-0003: listener for comId 1003, non callback execution expected
		fprintf(stderr,"[TEST-0003: ERROR] queue_procricz(): no callback execution expected for comID 1003\n");
		l2fLog("[TEST-0003: ERROR] queue_procricz(): no callback execution expected for comID 1003.");
	    }
	    break;
	    
	    case 2001:
	    {
		// TEST-0004: no Reply for comID 2001, timeout callback execution expected
		if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
		{    
		    fprintf(stderr,"[TEST-0004: OK] queue_procricz(): timeout callback execution for comID 2001\n");
		    l2fLog("[TEST-0004: OK] queue_procricz(): timeout callback execution for comID 2001.");
		}
		else
		{    
		    fprintf(stderr,"[TEST-0004: ERROR] queue_procricz(): callback result expected %u, found %u.\n", TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0004: ERROR] queue_procricz(): callback result expected %u, found %u.", TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
		    l2fLog(strTmp);
		}
	    }
	    break;

	    case 2002:
	    {
		// TEST-0005: listener for comId 2002, callback execution expected for Reply without confirmation request
		if(msg.Msg.resultCode != TRDP_NO_ERR)
		{
		    fprintf(stderr,"[TEST-0005: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.\n", TRDP_NO_ERR, msg.Msg.resultCode);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0005: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.", TRDP_NO_ERR, msg.Msg.resultCode);
		    l2fLog(strTmp);
		    break;
		}
		
		if(msg.Msg.msgType == TRDP_MSG_MP)
		{
		    // Get data
		    UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
		    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
		    mdTestData->cnt = vos_ntohl(mdTestData->cnt);
		    
		    //
		    fprintf(stderr,"[TEST-0005: OK] queue_procricz()\n  Callback executed for comID 2002\n  Reply without confirmation request\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0005: OK] queue_procricz()\n  Callback executed for comID 2002\n  Reply without confirmation request\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    l2fLog(strTmp);
		}
		else
		{
		    fprintf(stderr,"[TEST-0005: ERROR] queue_procricz()\n  Callback executed for comID 2002\n  Expected msgType %u but received %u\n", TRDP_MSG_MN, msg.Msg.msgType);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0005: ERROR] queue_procricz()\n  Callback executed for comID 2002\n  Expected msgType %u but received %u\n", TRDP_MSG_MN, msg.Msg.msgType);
		    l2fLog(strTmp);		
		} 
	    }
	    break;
		
	    case 2003:
	    {
		// TEST-0006: no Reply for comID 2003, timeout callback execution expected
		if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
		{    
		    fprintf(stderr,"[TEST-0006: OK] queue_procricz(): timeout callback execution for comID 2003\n");
		    l2fLog("[TEST-0006: OK] queue_procricz(): timeout callback execution for comID 2003.");
		}
		else
		{    
		    fprintf(stderr,"[TEST-0006: ERROR] queue_procricz(): callback result expected %u, found %u.\n", TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0006: ERROR] queue_procricz(): callback result expected %u, found %u.", TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
		    l2fLog(strTmp);
		}
	    }
	    break;

	    case 3001:
	    {
		// TEST-0007: listener for comId 3001 MD Request, callback execution expected for Reply with confirmation request
		if(msg.Msg.resultCode != TRDP_NO_ERR)
		{
		    fprintf(stderr,"[TEST-0007: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.\n", TRDP_NO_ERR, msg.Msg.resultCode);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0007: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.", TRDP_NO_ERR, msg.Msg.resultCode);
		    l2fLog(strTmp);
		    break;
		}
		
		if(msg.Msg.msgType == TRDP_MSG_MQ)
		{
		    // Get data
		    UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
		    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
		    mdTestData->cnt = vos_ntohl(mdTestData->cnt);
		    
		    //
		    fprintf(stderr,"[TEST-0007: OK] queue_procricz()\n  Callback executed for comID 3001 MD ReplyQuery reception\n  Reply with confirmation request\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0007: OK] queue_procricz()\n  Callback executed for comID 3001 MD ReplyQuery reception\n  Reply with confirmation request\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    l2fLog(strTmp);
		    
		    // Send confirm
		    testConfirmSendID++;
		    TRDP_ERR_T errv = tlm_confirm (
			appHandle,
			(void *)0x2000CAFE,
			(const TRDP_UUID_T *)&msg.Msg.sessionId,
			msg.Msg.comId,
			msg.Msg.topoCount,
			msg.Msg.destIpAddr,
			msg.Msg.srcIpAddr,
			0, /* pktFlags */
			0, /*  userStatus */
			0, /* replystatus */
			NULL, /* send param */
			msg.Msg.destURI,
			msg.Msg.srcURI
		    );
		    
		    if (errv != TRDP_NO_ERR)
		    {
			fprintf(stderr,"queue_procricz(): tlm_confirm() error = %d\n",errv);
			exit(EXIT_FAILURE);
		    }

		    //
		    fprintf(stderr,"[TEST-0007: OK] queue_procricz()\n  Callback executed for comID 3001\n  Confirm sent\n");
		    sprintf(strTmp, "[TEST-0007: OK] queue_procricz()\n  Callback executed for comID 3001\n  Confirm sent\n");
		    l2fLog(strTmp);
		}
		else
		{
		    fprintf(stderr,"[TEST-0007: ERROR] queue_procricz()\n  Callback executed for comID 3001\n  Expected msgType %u but received %u\n", TRDP_MSG_MN, msg.Msg.msgType);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0007: ERROR] queue_procricz()\n  Callback executed for comID 3001\n  Expected msgType %u but received %u\n", TRDP_MSG_MN, msg.Msg.msgType);
		    l2fLog(strTmp);		
		} 
	    }
	    break;
	    
	    case 3002:
	    {
		// TEST-0008: listener for comId 3002 MD Request, callback execution expected for Reply with confirmation request		
		if(msg.Msg.msgType == TRDP_MSG_MQ)
		{
		    if(msg.Msg.resultCode == TRDP_APPTIMEOUT_ERR)
		    {
			// Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
			fprintf(stderr,"[TEST-0008: OK] queue_procricz()\n  Callback executed for comID 3002 Application timeout\n");
			char strTmp[128];
			sprintf(strTmp, "[TEST-0008: OK] queue_procricz()\n  Callback executed for comID 3002 Application timeout\n");
			l2fLog(strTmp);
			
			// Test finished
			break;
		    }
		    else if(msg.Msg.resultCode != TRDP_NO_ERR)
		    {
			// Unexpected result code
			fprintf(stderr,"[TEST-0008: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.\n", TRDP_NO_ERR, msg.Msg.resultCode);
			char strTmp[128];
			sprintf(strTmp, "[TEST-0008: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.", TRDP_NO_ERR, msg.Msg.resultCode);
			l2fLog(strTmp);
			break;
		    }
		    
		    // Get data
		    UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
		    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
		    mdTestData->cnt = vos_ntohl(mdTestData->cnt);
		    
		    // No send confirm to check Confirm timeout in Dev2
		    fprintf(stderr,"[TEST-0008: OK] queue_procricz()\n  Callback executed for comID 3002 MD ReplyQuery reception\n  Reply with confirmation request\n  Cnt = %u\n  testId = %s\n  No send confirm to check Confirm timeout in Dev2\n", mdTestData->cnt, mdTestData->testId);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0008: OK] queue_procricz()\n  Callback executed for comID 3002 MD ReplyQuery reception\n  Reply with confirmation request\n  Cnt = %u\n  testId = %s\n No send confirm to check Confirm timeout in Dev2\n", mdTestData->cnt, mdTestData->testId);
		    l2fLog(strTmp);
		}
		else
		{
		    fprintf(stderr,"[TEST-0008: ERROR] queue_procricz()\n  Callback executed for comID 3002\n  Expected msgType %u but received %u\n", TRDP_MSG_MN, msg.Msg.msgType);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0008: ERROR] queue_procricz()\n  Callback executed for comID 3002\n  Expected msgType %u but received %u\n", TRDP_MSG_MN, msg.Msg.msgType);
		    l2fLog(strTmp);
		} 
	    }
	    break;
	    
	    default:
	    {
		// LOG
		fprintf(stderr,"[ERROR] queue_procricz(): Unexpected message with comID = x%04X\n", msg.Msg.comId);
		//sprintf(trdpLogString, "queue_procricz(): Message code = x%04X unhandled",msg.Msg.msgType);
		//l2fLog(trdpLogString);
	    }
	    break;
	}
    }
    
    // Dev 2
    if(x_testmode == 2)
    {
	switch(msg.Msg.comId)
	{
	    case 1001:
	    {
		// TEST-0001: no listener for comId 1001, non callback execution expected
		fprintf(stderr,"[TEST-0001: ERROR] queue_procricz(): no callback execution expected for comID 1001\n");
		l2fLog("[TEST-0001: ERROR] queue_procricz(): no callback execution expected for comID 1001.");
	    }
	    break;

	    case 1002:
	    {
		// TEST-0002: listener for comId 1002, callback execution expected
		if(msg.Msg.resultCode != TRDP_NO_ERR)
		{
		    fprintf(stderr,"[TEST-0002: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.\n", TRDP_NO_ERR, msg.Msg.resultCode);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0002: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.", TRDP_NO_ERR, msg.Msg.resultCode);
		    l2fLog(strTmp);
		    break;
		}
		
		// Check type
		if(msg.Msg.msgType == TRDP_MSG_MN)
		{
		    // Get data
		    UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
		    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
		    mdTestData->cnt = vos_ntohl(mdTestData->cnt);
		    
		    fprintf(stderr,"[TEST-0002: OK] queue_procricz()\n  Callback executed for comID 1002\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0002: OK] queue_procricz()\n  Callback executed for comID 1002\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    l2fLog(strTmp);
		}
		else
		{
		    fprintf(stderr,"[TEST-0002: ERROR] queue_procricz()\n  Callback executed for comID 1002\n  Expected msgType %u but received %u\n", TRDP_MSG_MN, msg.Msg.msgType);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0002: ERROR] queue_procricz()\n  Callback executed for comID 1002\n  Expected msgType %u but received %u\n", TRDP_MSG_MN, msg.Msg.msgType);
		    l2fLog(strTmp);		
		} 
	    }
	    break;
		
	    case 1003:
	    {
		// TEST-0003: listener for comId 1003, Notify sent to 1004, non callback execution expected
		fprintf(stderr,"[TEST-0003: ERROR] queue_procricz(): no callback execution expected for comID 1003\n");
		l2fLog("[TEST-0003: ERROR] queue_procricz(): no callback execution expected for comID 1003.");
	    }
	    break;
	    
	    case 2001:
	    {
		// TEST-0004: no listener for comId 2001, non callback execution expected
		fprintf(stderr,"[TEST-0004: ERROR] queue_procricz(): no callback execution expected for comID 2001.\n");
		l2fLog("[TEST-0004: ERROR] queue_procricz(): no callback execution expected for comID 2001.");
	    }
	    break;

	    case 2002:
	    {
		// TEST-0005: listener for comId 2002, callback execution expected for Reply
		if(msg.Msg.resultCode != TRDP_NO_ERR)
		{
		    fprintf(stderr,"[TEST-0005: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.\n", TRDP_NO_ERR, msg.Msg.resultCode);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0005: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.", TRDP_NO_ERR, msg.Msg.resultCode);
		    l2fLog(strTmp);
		    break;
		}
		
		// Check type
		if(msg.Msg.msgType == TRDP_MSG_MR)
		{
		    // Get data
		    UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
		    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
		    mdTestData->cnt = vos_ntohl(mdTestData->cnt);
		    
		    //
		    fprintf(stderr,"[TEST-0005: OK] queue_procricz()\n  Callback executed for comID 2002\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0005: OK] queue_procricz()\n  Callback executed for comID 2002\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    l2fLog(strTmp);
		    
		    // Send MD Reply
		    TRDP_MD_TEST_DS_T mdTestData1;    
		    testReplySendID++;
		    mdTestData1.cnt = vos_htonl(testReplySendID);
		    sprintf(mdTestData1.testId,"MD Reply test");
		    
		    TRDP_ERR_T errv = tlm_reply (
			appHandle,
			(void *)0x2000CAFE,
			&msg.Msg.sessionId,
			msg.Msg.topoCount,
			msg.Msg.comId,
			msg.Msg.destIpAddr,
			msg.Msg.srcIpAddr,
			0, /* pktFlags */
			0, /*  userStatus */
			NULL, /* send param */
			(UINT8 *) &mdTestData1,
			sizeof(mdTestData1),
			msg.Msg.destURI,
			msg.Msg.srcURI
		    );
		    
		    if (errv != TRDP_NO_ERR)
		    {
			fprintf(stderr,"queue_procricz(): tlm_reply() error = %d\n",errv);
			exit(EXIT_FAILURE);
		    }

		    //
		    fprintf(stderr,"[TEST-0005: OK] queue_procricz()\n  Callback executed for comID 2002\n  Reply sent\n");
		    sprintf(strTmp, "[TEST-0005: OK] queue_procricz()\n  Callback executed for comID 2002\n  Reply sent\n");
		    l2fLog(strTmp);
		}
		else
		{
		    fprintf(stderr,"[TEST-0005: ERROR] queue_procricz()\n  Callback executed for comID 2002\n  Expected msgType %u but received %u\n", TRDP_MSG_MR, msg.Msg.msgType);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0005: ERROR] queue_procricz()\n  Callback executed for comID 2002\n  Expected msgType %u but received %u\n", TRDP_MSG_MR, msg.Msg.msgType);
		    l2fLog(strTmp);		
		}
	    }
	    break;
		
	    case 2003:
	    {
		// TEST-0006: listener for comId 2003, Request sent to 1004, non callback execution expected
		fprintf(stderr,"[TEST-0006: ERROR] queue_procricz(): no callback execution expected for comID 2003.\n");
		l2fLog("[TEST-0006: ERROR] queue_procricz(): no callback execution expected for comID 2003.");
	    }
	    break;
	    
	    case 3001:
	    {
		// TEST-0007: listener for comId 3001, callback execution expected for Reply with confirmation request
		
		// Check type
		if(msg.Msg.msgType == TRDP_MSG_MR)
		{
		    if(msg.Msg.resultCode != TRDP_NO_ERR)
		    {
			fprintf(stderr,"[TEST-0007: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.\n", TRDP_NO_ERR, msg.Msg.resultCode);
			char strTmp[128];
			sprintf(strTmp, "[TEST-0007: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.", TRDP_NO_ERR, msg.Msg.resultCode);
			l2fLog(strTmp);
			break;
		    }
		    
		    // Get data
		    UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
		    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
		    mdTestData->cnt = vos_ntohl(mdTestData->cnt);
		    
		    //
		    fprintf(stderr,"[TEST-0007: OK] queue_procricz()\n  Callback executed for comID 3001 MD Request reception\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0007: OK] queue_procricz()\n  Callback executed for comID 3001 MD Request reception\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    l2fLog(strTmp);
		    
		    // Send MD Reply
		    TRDP_MD_TEST_DS_T mdTestData1;    
		    testReplyQSendID++;
		    mdTestData1.cnt = vos_htonl(testReplyQSendID);
		    sprintf(mdTestData1.testId,"MD ReplyQ test");
		    
		    TRDP_ERR_T errv = tlm_replyQuery (
			appHandle,
			(void *)0x2000CAFE,
			&msg.Msg.sessionId,
			msg.Msg.topoCount,
			msg.Msg.comId,
			msg.Msg.destIpAddr,
			msg.Msg.srcIpAddr,
			0, /* pktFlags */
			0, /*  userStatus */
			2*1000*1000, /* confirm timeout */
			NULL, /* send param */
			(UINT8 *) &mdTestData1,
			sizeof(mdTestData1),
			msg.Msg.destURI,
			msg.Msg.srcURI
		    );
		    
		    if (errv != TRDP_NO_ERR)
		    {
			fprintf(stderr,"queue_procricz(): tlm_replyQuery() error = %d\n",errv);
			exit(EXIT_FAILURE);
		    }

		    //
		    fprintf(stderr,"[TEST-0007: OK] queue_procricz()\n  Callback executed for comID 3001\n  ReplyQ sent\n");
		    sprintf(strTmp, "[TEST-0007: OK] queue_procricz()\n  Callback executed for comID 3001\n  ReplyQ sent\n");
		    l2fLog(strTmp);
		}
		else if(msg.Msg.msgType == TRDP_MSG_MC)
		{
		    if(msg.Msg.resultCode != TRDP_NO_ERR)
		    {
			fprintf(stderr,"[TEST-0007: ERROR] queue_procricz(): Callback executed for comID 3001 MD Confirm\n  msg.Msg.resultCode expected %u, found %u.\n", TRDP_NO_ERR, msg.Msg.resultCode);
			char strTmp[128];
			sprintf(strTmp, "[TEST-0007: ERROR] queue_procricz(): Callback executed for comID 3001 MD Confirm\n  msg.Msg.resultCode expected %u, found %u.", TRDP_NO_ERR, msg.Msg.resultCode);
			l2fLog(strTmp);
			break;
		    }
		    
		    //
		    fprintf(stderr,"[TEST-0007: OK] queue_procricz()\n  Callback executed for comID 3001 MD Confirm reception\n");
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0007: OK] queue_procricz()\n  Callback executed for comID 3001 MD Confirm reception\n");
		    l2fLog(strTmp);
		}
		else
		{
		    fprintf(stderr,"[TEST-0007: ERROR] queue_procricz()\n  Callback executed for comID 3001\n  Unexpected msgType %d and resultCode %d\n", msg.Msg.msgType, msg.Msg.resultCode);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0007: ERROR] queue_procricz()\n  Callback executed for comID 3001\n  Unexpected msgType %d and resultCode %d\n", msg.Msg.msgType, msg.Msg.resultCode);
		    l2fLog(strTmp);		
		}
	    }
	    break;
	    
	    case 3002:
	    {
		// TEST-0008: listener for comId 3002, callback execution expected for Reply with confirmation but with timeout
		
		// Check type
		if(msg.Msg.msgType == TRDP_MSG_MR)
		{
		    if(msg.Msg.resultCode != TRDP_NO_ERR)
		    {
			fprintf(stderr,"[TEST-0008: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.\n", TRDP_NO_ERR, msg.Msg.resultCode);
			char strTmp[128];
			sprintf(strTmp, "[TEST-0008: ERROR] queue_procricz(): msg.Msg.resultCode expected %u, found %u.", TRDP_NO_ERR, msg.Msg.resultCode);
			l2fLog(strTmp);
			break;
		    }
		    
		    // Get data
		    UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
		    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
		    mdTestData->cnt = vos_ntohl(mdTestData->cnt);
		    
		    //
		    fprintf(stderr,"[TEST-0008: OK] queue_procricz()\n  Callback executed for comID 3002 MD Request reception\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0008: OK] queue_procricz()\n  Callback executed for comID 3002 MD Request reception\n  Cnt = %u\n  testId = %s\n", mdTestData->cnt, mdTestData->testId);
		    l2fLog(strTmp);
		    
		    // Send MD Reply
		    TRDP_MD_TEST_DS_T mdTestData1;    
		    testReplyQSendID++;
		    mdTestData1.cnt = vos_htonl(testReplyQSendID);
		    sprintf(mdTestData1.testId,"MD ReplyQ test");
		    
		    TRDP_ERR_T errv = tlm_replyQuery (
			appHandle,
			(void *)0x2000CAFE,
			&msg.Msg.sessionId,
			msg.Msg.topoCount,
			msg.Msg.comId,
			msg.Msg.destIpAddr,
			msg.Msg.srcIpAddr,
			0, /* pktFlags */
			0, /*  userStatus */
			2*1000*1000, /* confirm timeout */
			NULL, /* send param */
			(UINT8 *) &mdTestData1,
			sizeof(mdTestData1),
			msg.Msg.destURI,
			msg.Msg.srcURI
		    );
		    
		    if (errv != TRDP_NO_ERR)
		    {
			fprintf(stderr,"queue_procricz(): tlm_replyQuery() error = %d\n",errv);
			exit(EXIT_FAILURE);
		    }

		    //
		    fprintf(stderr,"[TEST-0008: OK] queue_procricz()\n  Callback executed for comID 3002\n  ReplyQ sent\n");
		    sprintf(strTmp, "[TEST-0008: OK] queue_procricz()\n  Callback executed for comID 3002\n  ReplyQ sent\n");
		    l2fLog(strTmp);
		}
		else if(msg.Msg.msgType == TRDP_MSG_MQ)
		{
		    // Confirmation timeout is generated with ReplyQ msgType beacuse it is the one waiting the confirmation
		    if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
		    {
			fprintf(stderr,"[TEST-0008: OK] queue_procricz()\n  Callback executed for comID 3002 MD Confirm reception timeout\n");
			char strTmp[128];
			sprintf(strTmp, "[TEST-0008: OK] queue_procricz()\n  Callback executed for comID 3002 MD Confirm reception timeout\n");
			l2fLog(strTmp);
		    }
		    else
		    {
			fprintf(stderr,"[TEST-0008: ERROR] queue_procricz(): Callback executed for comID 3002 MD Confirm\n  msg.Msg.resultCode expected %u, found %u.\n", TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
			char strTmp[128];
			sprintf(strTmp, "[TEST-0008: ERROR] queue_procricz(): Callback executed for comID 3002 MD Confirm\n  msg.Msg.resultCode expected %u, found %u.", TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
			l2fLog(strTmp);
			break;
		    }
		}
		else
		{
		    fprintf(stderr,"[TEST-0008: ERROR] queue_procricz()\n  Callback executed for comID 3002\n  Unexpected msgType %d and resultCode %d\n", msg.Msg.msgType, msg.Msg.resultCode);
		    char strTmp[128];
		    sprintf(strTmp, "[TEST-0008: ERROR] queue_procricz()\n  Callback executed for comID 3002\n  Unexpected msgType %d and resultCode %d\n", msg.Msg.msgType, msg.Msg.resultCode);
		    l2fLog(strTmp);		
		}
	    }
	    break;	    
	    
	    default:
	    {
		// LOG
		fprintf(stderr,"[ERROR] queue_procricz(): Unexpected message with comID = x%04X\n", msg.Msg.comId);
		//sprintf(trdpLogString, "queue_procricz(): Message code = x%04X unhandled",msg.Msg.msgType);
		//l2fLog(trdpLogString);
	    }
	    break;
	}
    }

    #if 0
    switch(msg.Msg.msgType)
    {
	case TRDP_MSG_MN:
	{
	    // LOG
	    fprintf(stderr,"queue_procricz(): MD Notify Received\n");
	    //l2fLog("queue_procricz(): MD Notify Received");
	}
	break;
	
	case TRDP_MSG_MR:
	{
	    fprintf(stderr,"queue_procricz(): MD Request Received with reply\n");

	    // LOG
	    //l2fLog("queue_procricz(): MD Request Received with reply");

	    if (x_reqconf == 0)
	    {
		char outbuf[80];
		int szout = sprintf(outbuf,"tlm_reply %d",(int)time(0));
		    
		TRDP_ERR_T errv = tlm_reply (
		    appHandle,
		    (void *)0x2000CAFE,
		    &msg.Msg.sessionId,
		    msg.Msg.topoCount,
		    msg.Msg.comId,
		    msg.Msg.destIpAddr,
		    msg.Msg.srcIpAddr,
		    0, /* pktFlags */
		    0, /*  userStatus */
		    NULL, /* send param */
		    (UINT8 *)outbuf,
		    szout+1,
		    msg.Msg.destURI,
		    msg.Msg.srcURI);

		// LOG
		fprintf(stderr,"queue_procricz(): MD Reply Sent> %d\n",errv);
		//l2fLog("queue_procricz(): MD Reply Sent");
	    }
	    else
	    {
		char outbuf[80];
		int szout = sprintf(outbuf,"tlm_replyQuery %d",(int)time(0));
		    
		TRDP_ERR_T errv = tlm_replyQuery (
		    appHandle,
		    (void *)0x2000CAFE,
		    &msg.Msg.sessionId,
		    msg.Msg.topoCount,
		    msg.Msg.comId,
		    msg.Msg.destIpAddr,
		    msg.Msg.srcIpAddr,
		    0, /* pktFlags */
		    0, /*  userStatus */
		    5*1000*1000, /* confirm timeout */
		    NULL, /* send param */
		    (UINT8 *)outbuf,
		    szout+1,
		    msg.Msg.destURI,
		    msg.Msg.srcURI);

		// LOG
		fprintf(stderr,"queue_procricz(): MD Reply Query Sent: %d\n",errv);
		//l2fLog("queue_procricz(): MD Reply Query Sent");
	    }
	}
	break;

	case TRDP_MSG_MP:
	{
	    // LOG
	    fprintf(stderr,"queue_procricz(): MD Reply without confirmation Received\n");
	    //l2fLog("queue_procricz(): MD Reply without confirmation Received");
	}
	break;

	case TRDP_MSG_MQ:
	{
	    // LOG
	    fprintf(stderr,"queue_procricz(): MD Reply with confirmation Received\n");
	    //l2fLog("queue_procricz(): MD Reply with confirmation Received");
	
	    // Send confirm
	    TRDP_ERR_T errv = tlm_confirm (
		    appHandle,
		    (void *)0x2000CAFE,
		    (const TRDP_UUID_T *)&msg.Msg.sessionId,
		    msg.Msg.comId,
		    msg.Msg.topoCount,
		    msg.Msg.destIpAddr,
		    msg.Msg.srcIpAddr,
		    0, /* pktFlags */
		    0, /*  userStatus */
		    0, /* replystatus */
		    NULL, /* send param */
		    msg.Msg.destURI,
		    msg.Msg.srcURI);
	
	    // LOG
	    fprintf(stderr,"queue_procricz(): MD Confirm Sent: %d\n",errv);
	    //l2fLog("queue_procricz(): MD Confirm Sent");
	}
	break;

	case TRDP_MSG_MC:
	{
	    // LOG
	    fprintf(stderr,"queue_procricz(): MD Reply Confirm Received\n");
	    //l2fLog("queue_procricz(): MD Reply Confirm Received");
	}
	break;

	default:
	{
	    // LOG
	    fprintf(stderr,"queue_procricz(): Message code = x%04X unhandled\n",msg.Msg.msgType);
	    //sprintf(trdpLogString, "queue_procricz(): Message code = x%04X unhandled",msg.Msg.msgType);
	    //l2fLog(trdpLogString);
	}
	break;
    }
    #endif
}



// call back function for message data
static void md_indication(
    void                 *pRefCon,
    const TRDP_MD_INFO_T *pMsg,
    UINT8                *pData,
    UINT32               dataSize)
{
    fprintf(stderr,"md_indication(r=%p m=%p d=%p l=%d comId=%d)\n",pRefCon,pMsg,pData,dataSize,pMsg->comId);
    
    #if 0

    printf("srcIpAddr         = x%08X\n",pMsg->srcIpAddr);
    printf("destIpAddr        = x%08X\n",pMsg->destIpAddr);
    printf("seqCount          = %d\n"   ,pMsg->seqCount);
    printf("protVersion       = %d\n"   ,pMsg->protVersion);
    printf("msgType           = x%04X\n",pMsg->msgType);
    printf("comId             = %d\n"   ,pMsg->comId);
    printf("topoCount         = %d\n"   ,pMsg->topoCount);
    printf("userStatus        = %d\n"   ,pMsg->userStatus);
    printf("replyStatus       = %d\n"   ,pMsg->replyStatus);
    printf("sessionId         = ");      print_session(pMsg->sessionId);
    printf("replyTimeout      = %d\n"   ,pMsg->replyTimeout);
    printf("destURI           = ");      print_uri(pMsg->destURI); printf("\n");
    printf("srcURI            = ");      print_uri(pMsg->srcURI); printf("\n");
    printf("noOfReplies       = %d\n"   ,pMsg->noOfReplies);
    printf("pUserRef          = %p\n"   ,pMsg->pUserRef);
    printf("resultCode        = %d\n"   ,pMsg->resultCode);
    
    print_memory(pData,dataSize);
    
    #endif
    
    {
	// ADd message to application event queue
	trdp_apl_cbenv_t fwd;
	
	fwd.pRefCon  = pRefCon;
	fwd.Msg      = * pMsg;
	fwd.pData    = pData;
	fwd.dataSize = dataSize;
	
	queue_sendmessage(&fwd);
    }
}


// Test initialization
static int test_initialize()
{
    TRDP_ERR_T errv;
    TRDP_MEM_CONFIG_T       dynamicConfig = {NULL, HEAP_MEMORY_SIZE, {}};

    memset(&md_config,0,sizeof(md_config));
    memset(&mem_config,0,sizeof(mem_config));
    
    /* memory allocator config */
    mem_config.p    = NULL;
    mem_config.size = HEAP_MEMORY_SIZE;
	    
    /* MD config */
    md_config.pfCbFunction = md_indication;
    md_config.pRefCon      = (void *)0x12345678;
    md_config.sendParam.qos = MD_DEFAULT_QOS;
    md_config.sendParam.ttl = MD_DEFAULT_TTL;
    md_config.flags = 0
	| TRDP_FLAGS_NONE      * 0
	| TRDP_FLAGS_REDUNDANT * 0
	| TRDP_FLAGS_MARSHALL  * 0
	| TRDP_FLAGS_CALLBACK  * 1
	| TRDP_FLAGS_TCP       * 0 /* 1=TCP, 0=UDP */
	;
    md_config.replyTimeout   = MD_DEFAULT_REPLY_TIMEOUT;
    md_config.confirmTimeout = MD_DEFAULT_CONFIRM_TIMEOUT;
    md_config.udpPort        = IP_MD_UDP_PORT;
    md_config.tcpPort        = IP_MD_UDP_PORT;
    
    /* Inizialize TRDP */
    /*  Init the library for non-blocking operation     */
    errv = tlc_init(&private_debug_printf,                            /* actually printf                                 */
		 &dynamicConfig);
    if (errv != TRDP_NO_ERR)
    {
	fprintf(stderr,"tlc_init() error = %d\n",errv);
	exit(EXIT_FAILURE);
    }

    /* set network topo counter */
    tlc_setTopoCount(appHandle, 151);

    return 0;
}



// TEST: Send a MD Notify
static int testNotifySendID = 0;

static int testNotifySend(
    UINT32 comId,
    UINT32 topoCount,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    TRDP_ERR_T errv;

    // Prepare data buffer
    TRDP_MD_TEST_DS_T mdTestData;    
    testNotifySendID++;
    mdTestData.cnt = vos_htonl(testNotifySendID);
    sprintf(mdTestData.testId,"MD Notify test");
    
    // Send MD Notify 
    errv = tlm_notify(
	appHandle,
	(void *)0x1000CAFE, /* user ref */
	comId,
	topoCount,
	0,   /* own IP address from trdp stack */
	vos_htonl(x_ip4_dest),
	0, /* flags */
	NULL, /* default send param */
	(UINT8 *)&mdTestData,
	sizeof(mdTestData),
	sourceURI,
	destURI
    );
    
    //
    if (errv != TRDP_NO_ERR)
    {
	fprintf(stderr,"testNotifySend() error = %d\n",errv);
	exit(EXIT_FAILURE);
    }

    // LOG
    fprintf(stderr,"testNotifySend(): comID = %u, topoCount = %u, dstIP = x%08X\n", comId, topoCount, x_ip4_dest);
    char strTmp[128];
    sprintf(strTmp, "testNotifySend(): comID = %u, topoCount = %u, dstIP = x%08X", comId, topoCount, x_ip4_dest);
    l2fLog(strTmp);
    
    return 0;
}


// TEST: Send a MD Request
static int testRequestSendID = 0;

static int testRequestSend(
    UINT32 comId,
    UINT32 topoCount,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    TRDP_ERR_T errv;

    // Prepare data buffer
    TRDP_MD_TEST_DS_T mdTestData;    
    testRequestSendID++;
    mdTestData.cnt = vos_htonl(testRequestSendID);
    sprintf(mdTestData.testId,"MD Request test");
    
    TRDP_UUID_T session;
    
    //
    errv = tlm_request(
	appHandle,
	(void *)0x1000CAFE, /* user ref */
	&session,
	comId, /* comId */
	topoCount, /* topoCount */
	0,   /* own IP address from trdp stack */
	vos_htonl(x_ip4_dest),
	0, /* flags */
	1, /* # of repliers */
	2*1000*1000, /* reply timeout */
	NULL, /* default send param */
	(UINT8 *)&mdTestData,
	sizeof(mdTestData),
	sourceURI,
	destURI
    );

    //
    if (errv != TRDP_NO_ERR)
    {
	fprintf(stderr,"testRequestSend(): error = %d\n",errv);
	exit(EXIT_FAILURE);
    }
    
    // LOG
    fprintf(stderr,"testRequestSend(): comID = %u, topoCount = %u, dstIP = x%08X\n", comId, topoCount, x_ip4_dest);
    char strTmp[128];
    sprintf(strTmp, "testRequestSend(): comID = %u, topoCount = %u, dstIP = x%08X", comId, topoCount, x_ip4_dest);
    l2fLog(strTmp);
    
    
    return 0;
}

// Test main loop
static int test_main_proc()
{
    /* main process loop */
    int run = 1;
    int cliCmd = 0;
    while(run)
    {
	// Test cli command execution
	ssize_t r = read(cliPipeFd[0], &cliCmd, sizeof(cliCmd));
	if (r == -1 && errno == EAGAIN)
	{
	    // No commands
	}
	else if (r > 0)
	{
	    // Cli command
	    fprintf(stderr,"test_main_proc(): cliCmd = %u\n", cliCmd);
	    
	    if(cliCmd == 1)
	    {
		// TEST-0001: Send MD Notify to 1001
		testNotifySend(1001, 151, "", "");
	    }
	    else if(cliCmd == 2)
	    {
		// TEST-0002: Send MD Notify to 1002
		testNotifySend(1002, 151, "", "");
	    }
	    else if(cliCmd == 3)
	    {
		// TEST-0003: Send MD Notify to 1004
		testNotifySend(1003, 151, "", "");
	    }
	    else if(cliCmd == 4)
	    {
		// TEST-0004: Send MD Request to 2001
		testRequestSend(2001, 151, "", "");
	    }
	    else if(cliCmd == 5)
	    {
		// TEST-0005: Send MD Request to 2002
		testRequestSend(2002, 151, "", "");
	    }
	    else if(cliCmd == 6)
	    {
		// TEST-0006: Send MD Request to 2003
		testRequestSend(2003, 151, "", "");
	    }
	    else if(cliCmd == 7)
	    {
		// TEST-0007: Send MD Request to 3001
		testRequestSend(3001, 151, "", "");
	    }
	    else if(cliCmd == 8)
	    {
		// TEST-0008: Send MD Request to 3002
		testRequestSend(3002, 151, "", "");
	    }
	}
	else
	{
	    fprintf(stderr,"test_main_proc(): pipe read error\n");
	    exit(EXIT_FAILURE);
	}
	
	// TRDP stuff
	fd_set rfds;
	INT32 noDesc = -1;
	struct timeval tv = {0,0};
	
	// cleanup
	FD_ZERO(&rfds);
	
	// printf("sizeof(fd_set)=%d\n",(int)sizeof(fd_set));
	
	tlc_getInterval(
	    appHandle,
	    (TRDP_TIME_T *)&tv,
	    (TRDP_FDS_T *)&rfds,
	    &noDesc);
	//fprintf(stderr,"tlc_getInterval()=%d\n",errv);
		
	{
	    //double sec = (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-9;
	    //printf("time = %g\n", sec);
	}
	//fprintf(stderr,"noDesc=%d\n",noDesc);
	
	
	// Polling Mode
	tlc_process(appHandle,NULL,NULL);
	//fprintf(stderr,"tlc_process()=%d\n",errv);
	
	// call back process
	queue_procricz();
	
	// Loop delay
	usleep(x_period*1000);

	//printf("."); fflush(stdout);
    }
    
    return(0);
}


// *** Command line - Start ***

// TEST: add a single listener
static int testAddListener(
    const void              *pUserRef,
    UINT32                  comId,
    const TRDP_URI_USER_T   destURI)
{
    TRDP_ERR_T errv;
    errv = tlm_addListener(
	appHandle,
	&lisHandle,
	pUserRef,
	comId,
	0,
	vos_htonl(0),
	0,
	destURI);
    if (errv != TRDP_NO_ERR)
    {
	    fprintf(stderr,"testAddListener() comID = %d error = %d\n", comId, errv);
	    exit(EXIT_FAILURE);
    }

    fprintf(stderr,"testAddListener(): comID = %d, lisHandle = x%p\n", comId, lisHandle);

    return 0;
}

// Listeners env
static const int test0001_lenv  = 10001001; // Not used
static const int test0002_lenv  = 10001002; // Dev2 Listener, comID 1002 Notify
static const int test0003_lenv  = 10001004; // Dev2 Listener, ComID 1003 Notify
static const int test0004_lenv1 = 10002001; // Dev1 Listener, comID 2001 Reply
static const int test0004_lenv2 = 20002001; // Not used
static const int test0005_lenv1 = 10002002; // Dev1 Listener, comID 2002 Reply
static const int test0005_lenv2 = 20002002; // Dev1 Listener, comID 2002 Request
static const int test0006_lenv1 = 10002003; // Dev1 Listener, comID 2003 Reply
static const int test0006_lenv2 = 20002004; // Dev1 Listener, comID 2002 Request
static const int test0007_lenv1 = 10003001; // Dev1 Listener, comID 2002 Reply
static const int test0007_lenv2 = 20003001; // Dev1 Listener, comID 2002 Request, Confirm
static const int test0008_lenv1 = 10003002; // Dev1 Listener, comID 2003 Reply
static const int test0008_lenv2 = 20003002; // Dev1 Listener, comID 2002 Request, Confirm

// Init listeners
static int testInitListeners()
{
    //TRDP_ERR_T errv;
    
    // Dev1 mode listeners
    if(x_testmode == 1)
    {
	// TEST-0001: no listener
	
	// TEST-0002: no listener

	// TEST-0003: no listener

	// TEST-0004: listener for comID 2001 Reply
	testAddListener(&test0004_lenv1, 2001, "");
	
	// TEST-0005: listener for comID 2002 Reply
	testAddListener(&test0005_lenv1, 2002, "");

	// TEST-0006: listener for comID 2003 Reply
	testAddListener(&test0006_lenv1, 2003, "");
	
	// TEST-0007: listener for comID 3001 Reply
	testAddListener(&test0007_lenv1, 3001, "");

	// TEST-0008: listener for comID 3002 Reply
	testAddListener(&test0008_lenv1, 3002, "");
    }
    
    // Dev2 mode listeners
    if(x_testmode == 2)
    {
	// TEST-0001: no listener
	
	// TEST-0002: listener
	testAddListener(&test0002_lenv, 1002, "");

	// TEST-0003: listener
	testAddListener(&test0003_lenv, 1004, "");

	// TEST-0004: no listener
	
	// TEST-0005: listener for comID 2002 Request
	testAddListener(&test0005_lenv2, 2002, "");

	// TEST-0006: listener for comID 2004 Request
	testAddListener(&test0006_lenv2, 2004, "");
	
	// TEST-0007: listener for comID 3001 Request and Confirm
	testAddListener(&test0007_lenv2, 3001, "");

	// TEST-0008: listener for comID 3002 Request and Confirm
	testAddListener(&test0008_lenv2, 3002, "");
    }

    // LOG
    fprintf(stderr,"testInitListeners(): done.");
    l2fLog("testInitListeners(): done.");

    return 0;
}


// Log file writer
void cliInteractiveCli()
{
    close(cliPipeFd[0]);
    
    // Command line handler
    char ch;
    
    int cmdRun = 1;
    while(cmdRun == 1)
    {
	// Prompt
	printf("\n");
	printf("Test mode: Dev%u\n", x_testmode);
	printf("Press a key: ");

	// Get command
	ch = getchar();
	printf("\n");

	// Dev1
	if(x_testmode == 1)
	{
	    // Execute command
	    if(ch == 'h')
	    {
		printf("Commands:\n");
		printf("h) Print menu comands\n");
		printf("1) TST-0001: Notify, Send Notify (ComID = 1001) to Dev2.\n");
		printf("2) TST-0002: Notify, Send Notify (ComID = 1002) to Dev2.\n");
		printf("3) TST-0003: Notify, Send Notify (ComID = 1004) to Dev2.\n");
		printf("4) TST-0004: Request-Reply, Send Request (ComID = 2001) to Dev2, no receive Reply.\n");
		printf("5) TST-0005: Request-Reply, Send Request (ComID = 2002) to Dev2, receive Reply.\n");
		printf("6) TST-0006: Request-Reply, Send Request (ComID = 2003) to Dev2, no receive Reply.\n");
		printf("7) TST-0007: Request-Reply-Confirm, Send Request (ComID = 3001) to Dev2, receive Reply and send Confirm.\n");
		printf("8) TST-0008: Request-Reply-Confirm, Send Request (ComID = 3002) to Dev2, receive Reply but not send Confirm.\n");
	    }
	    else if(ch == '1')
	    {
		// 
		printf("\n");
		printf("1) TST-0001: Notify, Send Notify (ComID = 1001) to Dev2.\n");
		
		// Send to server using pipe
		int cliCmd = 1;
		write(cliPipeFd[1], &cliCmd, sizeof(cliCmd));
	    }
	    else if(ch == '2')
	    {
		// 
		printf("\n");
		printf("2) TST-0002: Notify, Send Notify (ComID = 1002) to Dev2.\n");
		
		// Send to server using pipe
		int cliCmd = 2;
		write(cliPipeFd[1], &cliCmd, sizeof(cliCmd));
	    }
	    else if(ch == '3')
	    {
		// 
		printf("\n");
		printf("3) TST-0003: Notify, Send Notify (ComID = 1004) to Dev2.\n");
		
		// Send to server using pipe
		int cliCmd = 3;
		write(cliPipeFd[1], &cliCmd, sizeof(cliCmd));
	    }
	    else if(ch == '4')
	    {
		// 
		printf("\n");
		printf("4) TST-0004: Request-Reply, Send Request (ComID = 2001) to Dev2, no receive Reply.\n");
		
		// Send to server using pipe
		int cliCmd = 4;
		write(cliPipeFd[1], &cliCmd, sizeof(cliCmd));
	    }
	    else if(ch == '5')
	    {
		// 
		printf("\n");
		printf("5) TST-0005: Request-Reply, Send Request (ComID = 2002) to Dev2, receive Reply.\n");
		
		// Send to server using pipe
		int cliCmd = 5;
		write(cliPipeFd[1], &cliCmd, sizeof(cliCmd));
	    }
	    else if(ch == '6')
	    {
		// 
		printf("\n");
		printf("6) TST-0006: Request-Reply, Send Request (ComID = 2003) to Dev2, no receive Reply.\n");
		
		// Send to server using pipe
		int cliCmd = 6;
		write(cliPipeFd[1], &cliCmd, sizeof(cliCmd));
	    }
	    else if(ch == '7')
	    {
		// 
		printf("\n");
		printf("7) TST-0007: Request-Reply-Confirm, Send Request (ComID = 3001) to Dev2, receive Reply and send Confirm.\n");
		
		// Send to server using pipe
		int cliCmd = 7;
		write(cliPipeFd[1], &cliCmd, sizeof(cliCmd));
	    }
	    else if(ch == '8')
	    {
		// 
		printf("\n");
		printf("8) TST-0008: Request-Reply-Confirm, Send Request (ComID = 3002) to Dev2, receive Reply but not send Confirm.\n");
		
		// Send to server using pipe
		int cliCmd = 8;
		write(cliPipeFd[1], &cliCmd, sizeof(cliCmd));
	    }
	    else if(ch == 'q')
	    {
		// Reset local TX stats
		printf("EXIT: Todo");
	    }	    
	}
    }
}


// Init writer task
int cliInit(void)
{   
    // Init server pipe
    pipe(cliPipeFd);

    // Set pipe as non blocking, use it as event drive in main test loop to execute test commands
    if(fcntl(cliPipeFd[0], F_SETFL, O_NONBLOCK))
    {
	// Error
	close(cliPipeFd[0]);
	close(cliPipeFd[1]);
	
	fprintf(stderr,"cliInit(): set cliPipeFd as O_NONBLOCK failed.");
	exit(1);
    }
   
    // Create file writer server
    cliConsolePid = fork();
    
    // Fork ok, start server
    if(cliConsolePid == 0)
    {
	// Server start
	cliInteractiveCli();
	
	// Abnormal termination, server never end before application
	fprintf(stderr,"cliInit(): server abnormal exit.");
	exit(1);
    }
    // Fork error
    else if(cliConsolePid == -1)
    {
	fprintf(stderr,"cliInit(): server fork error.");
	exit(1);
    }
    
    // Close server side pipe
    close(cliPipeFd[1]);
    
    // Parent, return only
    return 0;
}

// *** Command line - END ***



static void cmdlinerr(int argc, char * argv[])
{
    fprintf(stderr,"usage: %s [--dest a.b.c.d] [--l2f] [--period <perido in ms>]  [--testmode <1/2>]\n",argv[0]);
}

#define needargs(n) { if ((n) > (argc - i)) { cmdlinerr(argc,argv); exit(EXIT_FAILURE); } }

int main(int argc, char * argv[])
{
	int i;

	// Basic command line parameters value
	x_uuid     = 0;
	x_notify   = 0;
	x_request  = 0;
	x_reqconf  = 0;
	x_receiver = 0;
	x_ip4_dest = TRDP_IP4_ADDR(192,168,190,129);
	x_l2f 	   = 0;
	x_period   = 100; // Defaul main loop period
	x_testmode = 1;
	
	// Process command line parameters
	for(i = 1; i < argc; i++)
	{
		if (0 == strcmp(argv[i],"--dest"))
		{
			needargs(1);
			i++;
			int ip[4];
			if (4 != sscanf(argv[i],"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]))
			{
				cmdlinerr(argc,argv);
				exit(EXIT_FAILURE);
			}
			x_ip4_dest = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
			continue;
		}
		if (0 == strcmp(argv[i],"--l2f"))
		{
			x_l2f = 1;
			continue;
		}
		if (0 == strcmp(argv[i],"--period"))
		{
			needargs(2);
			i++;
			if (1 != sscanf(argv[i],"%d",&x_period))
			{
				cmdlinerr(argc,argv);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if (0 == strcmp(argv[i],"--testmode"))
		{
			needargs(2);
			i++;
			if (1 != sscanf(argv[i],"%d",&x_testmode))
			{
				cmdlinerr(argc,argv);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		
		cmdlinerr(argc,argv);
		exit(EXIT_FAILURE);
	}

	// Log system init
	if(x_l2f == 1)
	{
	    l2fInit();
	    // Sllep to wait server start (workaound becaus einit return before server start)
	    usleep(2*1000*1000);
	}

	//fprintf(stderr, "MD_HEADER_T size: %u\n", sizeof(MD_HEADER_T));
	
	// Log test mode
	char strTmp[128];
	sprintf(strTmp, "main: start with testmode %u.", x_testmode);
	fprintf(stderr, strTmp);
	//l2fLog(strTmp);

	// Init queue
	queue_initialize();
	//l2fLog("main: queue_initialize() done.");

	// Init 4 test
	test_initialize();
	//l2fLog("main: test_initialize() done.");

	// Init command line interface
	cliInit();
	//l2fLog("main: cliInit() done.");
	
	// Add listeners
	testInitListeners();
	
	// Main loop
	char tmpStr[100];
	sprintf(tmpStr, "main: test_main_proc() with period %ums.", x_period);
	//l2fLog(tmpStr);
	test_main_proc();

	//l2fLog("main: end.");

	// Only to get last l2f messages logged
	//usleep(2*1000*1000);
	
	// Stop log
	//l2fStop();

	return(0);

}
