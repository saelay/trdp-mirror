/**********************************************************************************************************************/
/**
 * @file	mdTest0002.c
 *
 * @brief	UDPMDCom teat application
 *
 * @details	Test application. In Testmode 1 start transactions, in Testmode 2 and 3 respond to transactions.
 *
 * @note	Project: TCNOpen TRDP prototype stack
 *		Version 0.0: s.pachera (FAR).
 *			First issues, derived from mdTest0001.c example application.
 *			Add basic multicast test for Notify, Request and Reply without confirmation
 *			Removed log 2 file support, no more needed
 *
 * @author	Simone Pachera  (FAR Systems)
 *
 * @remarks	All rights reserved. Reproduction, modification, use or disclosure
 *		to third parties without express authority is forbidden,
 *		FAR Systems spa, Italy, 2013.
 *
 */

 /* check if the needed functionality is present */
#if (MD_SUPPORT == 1)
/* the needed MD_SUPPORT was granted */
#else
#error "This test needs MD_SUPPORT with the value '1'"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <sys/select.h>
#include <mqueue.h>
#include <time.h>

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
#include "../api/trdp_types.h"

#define TRDP_IP4_ADDR(a,b,c,d) ( (am_big_endian()) ? \
	((UINT32)((a) & 0xFF) << 24) | ((UINT32)((b) & 0xFF) << 16) | ((UINT32)((c) & 0xFF) << 8) | ((UINT32)((d) & 0xFF)) : \
	((UINT32)((d) & 0xFF) << 24) | ((UINT32)((c) & 0xFF) << 16) | ((UINT32)((b) & 0xFF) << 8) | ((UINT32)((a) & 0xFF)))

#define HEAP_MEMORY_SIZE (2*1024*1024)

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
static TRDP_IP_ADDR_T x_ip4_mc_01;
static TRDP_IP_ADDR_T x_ip4_mc_02;
static int            x_period;		// Main loop period in ms
static int            x_testmode;	// Test mode: dev1 o dev 2


// Command line
int	cliPipeFd[2];	// Communication pipe
pid_t   cliConsolePid;	// Console server PID


// Command line test data structure
typedef struct
{
	int				cliChar;		// Command line char that identify test
	char			*tstName;		// Test Name
	char			*tstDescr;		// Test description
	int				sendType;		// Send type: 0 = notify, 1 = request
	int				comID;			// Test ComID (one for test)
	int				topoCnt;		// Topo counter value
	TRDP_IP_ADDR_T	dstIP;			// Destination IP address
	int 			noOfRepliers;	// Expected replyers: 0 = Unknown, >0 known (for Unicast is always considered 1)
} cli_test;



// Cli tests
// IP address set to tagg value, overrided during application init loaded from command line arguments
//   0 = Dev2 address
//   1 = Multicast address
cli_test cliTests[] =
{
	{'1', "TEST-0001", "Notify, Send Notify to Dev2 (no listener).", 0, 1001, 151, 0, 1},
	{'2', "TEST-0002", "Notify, Send Notify to Dev2.", 0, 1002, 151, 0, 1},
	{'3', "TEST-0003", "Notify, Send Notify to Dev2 (listener in different comID).", 0, 1003, 151, 0, 1},
	{'4', "TEST-0004", "Request-Reply, Send Request to Dev2 (no listener).", 1, 2001, 151, 0, 1},
	{'5', "TEST-0005", "Request-Reply, Send Request to Dev2.", 1, 2002, 151, 0, 1},
	{'6', "TEST-0006", "Request-Reply, Send Request to Dev2 (listener in different comID).", 1, 2003, 151, 0, 1},
	{'7', "TEST-0007", "Request-Reply-Confirm, Send Request to Dev2.", 1, 3001, 151, 0, 1},
	{'8', "TEST-0008", "Request-Reply-Confirm, Send Request to Dev2, no confirm sent.", 1, 3002, 151, 0, 1},
	{'9', "TEST-0009", "Multicast Notify, Send Multicast Notify.", 0, 4001, 151, 1, 0},
	{'a', "TEST-0010", "Multicast Request-Reply, 2 expected repliers, 0 reply.", 1, 5001, 151, 2, 2},
	{'b', "TEST-0011", "Multicast Request-Reply, 2 expected repliers, 1 reply.", 1, 5002, 151, 2, 2},
	{'c', "TEST-0012", "Multicast Request-Reply, 2 expected repliers, 2 reply.", 1, 5003, 151, 2, 2},
	{'d', "TEST-0013", "Multicast Request-Reply, unknown expected repliers, 0 reply.", 1, 6001, 151, 2, 0},
	{'e', "TEST-0014", "Multicast Request-Reply, unknown expected repliers, 1 reply.", 1, 6002, 151, 2, 0},
	{'f', "TEST-0015", "Multicast Request-Reply, unknown expected repliers, 2 reply.", 1, 6003, 151, 2, 0},
	{'g', "TEST-0013", "Multicast Request-Reply-Confirm, 2 expected repliers, 0 confirm sent.", 1, 7001, 151, 2, 2},
	{'i', "TEST-0014", "Multicast Request-Reply-Confirm, 2 expected repliers, 1 confirm sent.", 1, 7002, 151, 2, 2},
	{'l', "TEST-0015", "Multicast Request-Reply-Confirm, 2 expected repliers, 2 confirm sent.", 1, 7003, 151, 2, 2}
};


// Get cli_test element from comId
int cliTestGetElementFromComID(int comID)
{
	// Check if array is initialized
	if(cliTests == NULL)
	{
		// Initialization error
		return -1;
	}
	
	// Find element
	int n = 0;
	for(n = 0; n < sizeof(cliTests)/sizeof(cli_test); n++)
		if(cliTests[n].comID == comID)
			return n;
	
	// Not nound
	return -1;
}


// Reception FSM status, reset by command execution start, incremented and used in message queue handling function
static int rx_test_fsm_state = 0;



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

// Send a confirm
static int testConfirmSend(trdp_apl_cbenv_t msg)
{
    TRDP_ERR_T errv;

    // Send confirm
	errv = tlm_confirm (
		appHandle,
		(void *)0x1100CAFE,
		(const TRDP_UUID_T *)&msg.Msg.sessionId,
		msg.Msg.comId,
		msg.Msg.topoCount,
		msg.Msg.destIpAddr,
		msg.Msg.srcIpAddr,
		0, // pktFlags
		0, //  userStatus
		0, // replystatus
		NULL, // send param
		msg.Msg.destURI,
		msg.Msg.srcURI
	);

    //
    if (errv != TRDP_NO_ERR)
    {
		fprintf(stderr,"testConfirmSend(): error = %d\n",errv);
		exit(EXIT_FAILURE);
    }
    
    // LOG
    fprintf(stderr, "testConfirmSend(): comID = %u, topoCount = %u, dstIP = x%08X\n", msg.Msg.comId, msg.Msg.topoCount, msg.Msg.destIpAddr);
    
    return 0;
}

// Send a reply
static int testReplySend(trdp_apl_cbenv_t msg, TRDP_MD_TEST_DS_T mdTestData)
{
    TRDP_ERR_T errv;

    // Send reply
	errv = tlm_reply (
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
		(UINT8 *) &mdTestData,
		sizeof(mdTestData),
		msg.Msg.destURI,
		msg.Msg.srcURI
	);

    //
    if (errv != TRDP_NO_ERR)
    {
		fprintf(stderr,"testReplySend(): error = %d\n",errv);
		exit(EXIT_FAILURE);
    }
    
    // LOG
    fprintf(stderr, "testReplySend(): comID = %u, topoCount = %u, dstIP = x%08X\n", msg.Msg.comId, msg.Msg.topoCount, msg.Msg.destIpAddr);
    
    return 0;
}

// Send a replyQuery
static int testReplyQuerySend(trdp_apl_cbenv_t msg, TRDP_MD_TEST_DS_T mdTestData)
{
    TRDP_ERR_T errv;

    // Send reply query
	errv = tlm_replyQuery (
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
		(UINT8 *) &mdTestData,
		sizeof(mdTestData),
		msg.Msg.destURI,
		msg.Msg.srcURI
	);

    //
    if (errv != TRDP_NO_ERR)
    {
		fprintf(stderr,"testReplyQuerySend(): error = %d\n",errv);
		exit(EXIT_FAILURE);
    }
    
    // LOG
    fprintf(stderr, "testReplyQuerySend(): comID = %u, topoCount = %u, dstIP = x%08X\n", msg.Msg.comId, msg.Msg.topoCount, msg.Msg.destIpAddr);
    
    return 0;
}


// MD application server
// Process queue elements pushed by call back function
static void queue_procricz()
{
	char strIp[16];
	char strTstName[128];	// Debug string
	
	trdp_apl_cbenv_t msg;
	
    int rc = queue_receivemessage(&msg);
	
	if (rc != EOK)
		return;

    {
		// Message info
		fprintf(stderr,"md_indication(r=%p d=%p l=%d)\n",msg.pRefCon,msg.pData,msg.dataSize);
		fprintf(stderr,"rx_test_fsm_state = %d\n"   ,rx_test_fsm_state);
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
		fprintf(stderr,"noOfRepliers      = %d\n"   ,msg.Msg.noOfRepliers);
		fprintf(stderr,"numReplies        = %d\n"   ,msg.Msg.numReplies);
		fprintf(stderr,"numRetriesMax     = %d\n"   ,msg.Msg.numRetriesMax);
		fprintf(stderr,"numRetries        = %d\n"   ,msg.Msg.numRetries);
		fprintf(stderr,"pUserRef          = %p\n"   ,msg.Msg.pUserRef);
		fprintf(stderr,"resultCode        = %d\n"   ,msg.Msg.resultCode);
		
		print_memory(msg.pData,msg.dataSize);
    }

	// Get test id
	int tstId = cliTestGetElementFromComID(msg.Msg.comId);
	if(tstId < 0)
	{
		// Error
		fprintf(stderr, "[ERROR] queue_procricz()\n  Test undefined for comId %u\n", msg.Msg.comId);
		sprintf(strTstName, "Callback ERROR [%u, UNDEFINED TEST, %u]", x_testmode, msg.Msg.comId);
	}
	else
	{
		sprintf(strTstName, "Callback [%u, %s, %u]", x_testmode, cliTests[tstId].tstName, cliTests[tstId].comID);
	}
	
    // Dev 1
    if(x_testmode == 1)
    {
		switch(msg.Msg.comId)
		{
			case 2001:
			{
				if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
				{
					if(rx_test_fsm_state == 0)
					{
						fprintf(stderr, "%s: timeout 1.\n", strTstName);
					}
					else if(rx_test_fsm_state == 1)
					{
						fprintf(stderr, "%s: timeout 2.\n", strTstName);
					}
					else
					{
						fprintf(stderr, "%s ERROR: unexpected rx fsm state %u\n", strTstName, rx_test_fsm_state);
					}
				}
				else
				{
					fprintf(stderr, "%s ERROR: resultCode expected %u, found %u.\n", strTstName, TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 2002:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					if(msg.Msg.msgType == TRDP_MSG_MP)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);

						//
						fprintf(stderr, "%s: Reply payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
					}
					else
					{
						fprintf(stderr, "%s ERROR: Expected msgType %u but received %u\n", strTstName, TRDP_MSG_MN, msg.Msg.msgType);
					}
				}
				else
				{
					fprintf(stderr, "%s ERROR: resultCode expected %u, found %u.\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 2003:
			{
				if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
				{    
					if(rx_test_fsm_state == 0)
					{
						fprintf(stderr, "%s: timeout 1.\n", strTstName);
					}
					else if(rx_test_fsm_state == 1)
					{
						fprintf(stderr, "%s: timeout 2.\n", strTstName);
					}
					else
					{
						fprintf(stderr, "%s ERROR: unexpected rx fsm state %u\n", strTstName, rx_test_fsm_state);
					}
				}
				else
				{    
					fprintf(stderr, "%s ERROR: resultCode expected %u, found %u.\n", strTstName, TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 3001:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					if(msg.Msg.msgType == TRDP_MSG_MQ)
					{
						// Get payload
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						fprintf(stderr, "%s: MD ReplyQuery, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);

						// Send Confirm
						testConfirmSend(msg);
						
						fprintf(stderr, "%s: Confirm sent\n", strTstName);
					}
					else
					{
						// Error
						fprintf(stderr, "%s ERROR: Expected msgType %u, received %u\n", strTstName, TRDP_MSG_MN, msg.Msg.msgType);
					} 
				}
				else
				{
					// Error
					fprintf(stderr, "%s ERROR: resultCode expected %u, found %u\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 3002:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					if(msg.Msg.msgType == TRDP_MSG_MQ)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						// No send confirm to check Confirm timeout in Dev2
						fprintf(stderr, "%s: MD ReplyQuery reception, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
					}
					else
					{
						// Error
						fprintf(stderr, "%s ERROR: Expected msgType %u but received %u\n", strTstName, TRDP_MSG_MQ, msg.Msg.msgType);
					}
				}
				else if(msg.Msg.resultCode == TRDP_APPTIMEOUT_ERR)
				{
					// Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
					fprintf(stderr, "%s: Application timeout on not sended confirm.\n", strTstName);
				}
				else
				{
					// Error
					fprintf(stderr, "%s ERROR: resultCode expected %u or %u, found %u\n", strTstName, TRDP_NO_ERR, TRDP_APPTIMEOUT_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 5001:
				if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
				{
					if(msg.Msg.numReplies == 0)
					{
						fprintf(stderr, "%s: timeout, numReplies = %u\n", strTstName, msg.Msg.numReplies);
					}
					else
					{
						fprintf(stderr, "%s ERROR: timeout, expected %d replies, found %d.\n", strTstName, 0, msg.Msg.numReplies);
					}
				}
				else
				{
					fprintf(stderr,"%s ERROR: resultCode expected %d, found %d.\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			break;
			
			case 5002:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// 1) Reception
					if(rx_test_fsm_state != 0)
					{
						fprintf(stderr, "%s ERROR: expected rx fsm state %u, found %u.\n", strTstName, 0, rx_test_fsm_state);
						break;
					}

					// 2 replies expected, 1 received
					if(msg.Msg.msgType == TRDP_MSG_MP)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						//
						if(msg.Msg.numReplies == 1)
						{
							fprintf(stderr,"%s: Reply from %s, payload Cnt = %u\n testId = %s\n", strTstName, miscIpToString(msg.Msg.srcIpAddr, strIp), mdTestData->cnt, mdTestData->testId);
						}
						else
						{
							fprintf(stderr, "%s ERROR: expected %u replies, found %u.\n", strTstName, 1, msg.Msg.numReplies);
						}
					}
				}
				else if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
				{
					// 2) Timeout
					if(rx_test_fsm_state != 1)
					{
						fprintf(stderr, "%s ERROR: expected rx fsm state %u, found %u.\n", strTstName, 1, rx_test_fsm_state);
						break;
					}

					// 2 replies expected, 1 received
					if(msg.Msg.numReplies == 1)
					{
						fprintf(stderr, "%s: timeout, numReplies = %u\n", strTstName, msg.Msg.numReplies);
					}
					else
					{
						fprintf(stderr, "%s ERROR: timeout, expected %u replies, found %u.\n", strTstName, 1, msg.Msg.numReplies);
					}
				}
				else
				{
					// Unexpected  result code
					fprintf(stderr, "%s ERROR: resultCode expected %d or %d, found %d.\n", strTstName, TRDP_NO_ERR, TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 5003:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// 2 replies expected, 2 received
					if(msg.Msg.msgType == TRDP_MSG_MP)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						// Reply 1
						if(rx_test_fsm_state == 0)
						{
							if(msg.Msg.numReplies == 1)
							{
								fprintf(stderr,"%s: Reply from %s, payload Cnt = %u\n testId = %s\n", strTstName, miscIpToString(msg.Msg.srcIpAddr, strIp), mdTestData->cnt, mdTestData->testId);
							}
							else
							{
								fprintf(stderr, "%s ERROR: expected 1 replies, found %u\n", strTstName, msg.Msg.numReplies);
							}
						}
						else if(rx_test_fsm_state == 1)
						{
							if(msg.Msg.numReplies == 2)
							{
								fprintf(stderr,"%s: Reply from %s, payload Cnt = %u\n testId = %s\n", strTstName, miscIpToString(msg.Msg.srcIpAddr, strIp), mdTestData->cnt, mdTestData->testId);
							}
							else
							{
								fprintf(stderr, "%s ERROR: expected 2 replies, found %u\n", strTstName, msg.Msg.numReplies);
							}
						}
						else
						{
							fprintf(stderr, "%s ERROR: unexpected rx fsm state %u\n", strTstName, rx_test_fsm_state);
						}
					}
				}
				else
				{
					// Unexpected  result code
					fprintf(stderr, "%s ERROR: resultCode expected %d or %d, found %d\n", strTstName, TRDP_NO_ERR, TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 6001:
				if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
				{
					if(rx_test_fsm_state != 0)
					{
						fprintf(stderr, "%s ERROR: expected rx fsm state %u, found %u\n", strTstName, 0, rx_test_fsm_state);
						break;
					}

					if(msg.Msg.numReplies == 0)
					{
						fprintf(stderr, "%s: timeout, numReplies = %u\n", strTstName, msg.Msg.numReplies);
					}
					else
					{
						fprintf(stderr, "%s ERROR: timeout, expected %d replies, found %d\n", strTstName, 0, msg.Msg.numReplies);
					}
				}
				else
				{
					fprintf(stderr, "%s ERROR: resultCode expected %d, found %d\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			break;

			case 6002:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// 1) Reception
					if(rx_test_fsm_state != 0)
					{
						fprintf(stderr, "%s ERROR: expected rx fsm state %u, found %u\n", strTstName, 0, rx_test_fsm_state);
						break;
					}

					// Unknown replies expected, 1 received			
					if(msg.Msg.msgType == TRDP_MSG_MP)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						//
						if(msg.Msg.numReplies == 1)
						{
							fprintf(stderr, "%s: Reply, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
						}
						else
						{
							fprintf(stderr, "%s ERROR: expected %u replies, found %u\n", strTstName, 1, msg.Msg.numReplies);
						}
					}
				}
				else if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
				{
					// 2) Timeout
					if(rx_test_fsm_state != 1)
					{
						fprintf(stderr, "%s ERROR: expected rx fsm state %u, found %u\n", strTstName, 1, rx_test_fsm_state);
						break;
					}

					// Unknown replies expected, 1 received
					if(msg.Msg.numReplies == 1)
					{
						fprintf(stderr, "%s: timeout, numReplies = %u\n", strTstName, msg.Msg.numReplies);
					}
					else
					{
						fprintf(stderr, "%s ERROR: timeout, expected %u replies, found %u\n", strTstName, 1, msg.Msg.numReplies);
					}
				}
				else
				{
					// Unexpected  result code
					fprintf(stderr, "%s ERROR: resultCode expected %d or %d, found %d\n", strTstName, TRDP_NO_ERR, TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 6003:
			{
				// TEST-0015: listener for comId 6003, callback execution expected for Reply without confirmation request
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// 1) Reception, unknown replies expected, 1 received			
					if(msg.Msg.msgType == TRDP_MSG_MP)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);

						if(rx_test_fsm_state == 0)
						{
							if(msg.Msg.numReplies == 1)
							{
								fprintf(stderr, "%s: Reply, payload Cnt = %u\n, testId = %s; numReplies = %u\n", strTstName, mdTestData->cnt, mdTestData->testId, msg.Msg.numReplies);
							}
							else
							{
								fprintf(stderr, "%s ERROR: expected 1 replies, found %u\n", strTstName, msg.Msg.numReplies);
							}
						}
						else if(rx_test_fsm_state == 1)
						{
							if(msg.Msg.numReplies == 2)
							{
								fprintf(stderr, "%s: Reply, payload Cnt = %u\n, testId = %s; numReplies = %u\n", strTstName, mdTestData->cnt, mdTestData->testId, msg.Msg.numReplies);
							}
							else
							{
								fprintf(stderr, "%s ERROR: expected 2 replies, found %u\n", strTstName, msg.Msg.numReplies);
							}
						}
						else
						{
							fprintf(stderr, "%s ERROR: unexpected rx fsm state %u\n", strTstName, rx_test_fsm_state);
						}
					}
				}
				else if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
				{
					if(rx_test_fsm_state != 2)
					{
						fprintf(stderr, "%s ERROR: expected rx fsm state %u, found %u\n", strTstName, 2, rx_test_fsm_state);
						break;
					}

					// Timeout, unknown replies expected, 2 received
					if(msg.Msg.numReplies == 2)
					{
						fprintf(stderr, "%s: timeout, numReplies = %u\n", strTstName, msg.Msg.numReplies);
					}
					else
					{
						fprintf(stderr, "%s ERROR: timeout, expected %u replies, found %u\n", strTstName, 2, msg.Msg.numReplies);
					}
				}
				else
				{
					// Unexpected  result code
					fprintf(stderr, "%s ERROR: resultCode expected %d or %d, found %d\n", strTstName, TRDP_NO_ERR, TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 7001:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					if(msg.Msg.msgType == TRDP_MSG_MQ)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						// No send confirm to check Confirm timeout in Dev2
						fprintf(stderr, "%s: MD ReplyQuery reception, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
					}
					else
					{
						// Error
						fprintf(stderr, "%s ERROR: Expected msgType %u but received %u\n", strTstName, TRDP_MSG_MQ, msg.Msg.msgType);
					}
				}
				else if(msg.Msg.resultCode == TRDP_APPTIMEOUT_ERR)
				{
					// Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
					fprintf(stderr, "%s: Application timeout on not sended confirm.\n", strTstName);
				}
				else
				{
					// Error
					fprintf(stderr, "%s ERROR: resultCode expected %u or %u, found %u\n", strTstName, TRDP_NO_ERR, TRDP_APPTIMEOUT_ERR, msg.Msg.resultCode);
				}
			}
			break;

			default:
			{
				// Error
				fprintf(stderr, "%s ERROR]: Unexpected message with comID = %d\n", strTstName, msg.Msg.comId);
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
			case 1003:
			case 1004:
			case 2001:
			case 2003:
			{
				fprintf(stderr, "%s ERROR: no callback execution expected\n", strTstName);
			}
			break;

			case 1002:
			{
				// TEST-0002: listener for comId 1002, callback execution expected
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// Check type
					if(msg.Msg.msgType == TRDP_MSG_MN)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						fprintf(stderr, "%s: notify recived, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
					}
					else
					{
						fprintf(stderr, "%s ERROR: Expected msgType %u, received %u\n", strTstName, TRDP_MSG_MN, msg.Msg.msgType);
					} 
				}
				else
				{
					fprintf(stderr, "%s ERROR: resultCode expected %u, found %u\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			}
			break;
			
			case 2002:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// Check type
					if(msg.Msg.msgType == TRDP_MSG_MR)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						fprintf(stderr, "%s: request received, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
						
						// Send MD Reply
						TRDP_MD_TEST_DS_T mdTestData1;    
						testReplySendID++;
						mdTestData1.cnt = vos_htonl(testReplySendID);
						sprintf(mdTestData1.testId,"MD Reply test");
						
						testReplySend(msg, mdTestData1);
						
						fprintf(stderr, "%s: Reply sent\n", strTstName);
					}
					else
					{
						fprintf(stderr, "%s ERROR: Expected msgType %u but received %u\n", strTstName, TRDP_MSG_MR, msg.Msg.msgType);
					}
				}
				else
				{
					fprintf(stderr, "%s ERROR: resultCode expected %u, found %u\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 3001:
			case 7002:
			case 7003:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// Check type
					if(msg.Msg.msgType == TRDP_MSG_MR)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						//
						fprintf(stderr, "%s: request received, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
						
						// Send MD Reply
						TRDP_MD_TEST_DS_T mdTestData1;    
						testReplyQSendID++;
						mdTestData1.cnt = vos_htonl(testReplyQSendID);
						sprintf(mdTestData1.testId,"MD ReplyQ test");
						
						testReplyQuerySend(msg, mdTestData1);
					
						fprintf(stderr, "%s: ReplyQuery sent\n", strTstName);
					}
					else if(msg.Msg.msgType == TRDP_MSG_MC)
					{
						//
						fprintf(stderr, "%s: MD Confirm received\n", strTstName);
					}
					else
					{
						fprintf(stderr, "%s ERROR: Unexpected msgType %d and resultCode %d\n", strTstName, msg.Msg.msgType, msg.Msg.resultCode);
					}
				}
				else
				{
					fprintf(stderr, "%s ERROR: resultCode expected %u, found %u\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			}
			break;
			
			case 3002:
			case 7001:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// Check type
					if(msg.Msg.msgType == TRDP_MSG_MR)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						//
						fprintf(stderr, "%s: request received payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
						
						// Send MD Reply
						TRDP_MD_TEST_DS_T mdTestData1;    
						testReplyQSendID++;
						mdTestData1.cnt = vos_htonl(testReplyQSendID);
						sprintf(mdTestData1.testId,"MD ReplyQ test");

						testReplyQuerySend(msg, mdTestData1);
					
						fprintf(stderr, "%s: ReplyQuery sent\n", strTstName);
					}
				}
				else if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
				{
					if(msg.Msg.msgType == TRDP_MSG_MQ)
					{
						// Confirmation timeout is generated with ReplyQ msgType because it is the one waiting the confirmation
						fprintf(stderr, "%s: confirm reception timeout\n", strTstName);
					}
					else
					{
						// Error
						fprintf(stderr, "%s ERROR: Unexpected msgType %d\n", strTstName, msg.Msg.msgType);
					}
				}
				else
				{
					// Error
					fprintf(stderr, "%s ERROR: resultCode expected %u or %u, found %u\n", strTstName, TRDP_NO_ERR, TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 4001:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// Check type
					if(msg.Msg.msgType == TRDP_MSG_MN)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						fprintf(stderr, "%s: notify received, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
					}
					else
					{
						fprintf(stderr, "%s ERROR: Expected msgType %u, received %u\n", strTstName, TRDP_MSG_MN, msg.Msg.msgType);
					} 
				}
				else
				{
					// Error
					fprintf(stderr, "%s ERROR:resultCode expected %u, found %u\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 5002:
			case 5003:
			case 6002:
			case 6003:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// Check type
					if(msg.Msg.msgType == TRDP_MSG_MR)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						//
						fprintf(stderr, "%s: request received, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
						
						// Send MD Reply
						TRDP_MD_TEST_DS_T mdTestData1;    
						testReplySendID++;
						mdTestData1.cnt = vos_htonl(testReplySendID);
						sprintf(mdTestData1.testId,"MD Reply test");
						
						testReplySend(msg, mdTestData1);
						fprintf(stderr, "%s: Reply sent\n", strTstName);
					}
					else
					{
						// Error
						fprintf(stderr, "%s ERROR: Expected msgType %u, received %u\n", strTstName, TRDP_MSG_MR, msg.Msg.msgType);
					}
				}
				else
				{
					// Error
					fprintf(stderr, "%s ERROR: resultCode expected %u, found %u\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			}
			break;

			default:
			{
				fprintf(stderr, "%s ERROR: Unexpected message with comID = %d\n", strTstName, msg.Msg.comId);
			}
			break;
		}
    }
	
    // Dev 3
    if(x_testmode == 3)
    {
		switch(msg.Msg.comId)
		{
			case 4001:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// Check type
					if(msg.Msg.msgType == TRDP_MSG_MN)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						fprintf(stderr, "%s: notify received, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
					}
					else
					{
						fprintf(stderr, "%s ERROR: Expected msgType %u, received %u\n", strTstName, TRDP_MSG_MN, msg.Msg.msgType);
					} 
				}
				else
				{
					// Error
					fprintf(stderr, "%s ERROR:resultCode expected %u, found %u\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 5003:
			case 6003:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// Check type
					if(msg.Msg.msgType == TRDP_MSG_MR)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						//
						fprintf(stderr, "%s: request received, payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
						
						// Send MD Reply
						TRDP_MD_TEST_DS_T mdTestData1;    
						testReplySendID++;
						mdTestData1.cnt = vos_htonl(testReplySendID);
						sprintf(mdTestData1.testId,"MD Reply test");
						
						testReplySend(msg, mdTestData1);
						fprintf(stderr, "%s: Reply sent\n", strTstName);
					}
					else
					{
						// Error
						fprintf(stderr, "%s ERROR: Expected msgType %u, received %u\n", strTstName, TRDP_MSG_MR, msg.Msg.msgType);
					}
				}
				else
				{
					// Error
					fprintf(stderr, "%s ERROR: resultCode expected %u, found %u\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
				}
			}
			break;

			case 7001:
			case 7002:
			{
				if(msg.Msg.resultCode == TRDP_NO_ERR)
				{
					// Check type
					if(msg.Msg.msgType == TRDP_MSG_MR)
					{
						// Get data
						UINT8 *pPayload = &msg.pData[sizeof(MD_HEADER_T)];
						TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;
						mdTestData->cnt = vos_ntohl(mdTestData->cnt);
						
						//
						fprintf(stderr, "%s: request received payload Cnt = %u, testId = %s\n", strTstName, mdTestData->cnt, mdTestData->testId);
						
						// Send MD Reply
						TRDP_MD_TEST_DS_T mdTestData1;    
						testReplyQSendID++;
						mdTestData1.cnt = vos_htonl(testReplyQSendID);
						sprintf(mdTestData1.testId,"MD ReplyQ test");

						testReplyQuerySend(msg, mdTestData1);
					
						fprintf(stderr, "%s: ReplyQuery sent\n", strTstName);
					}
				}
				else if(msg.Msg.resultCode == TRDP_TIMEOUT_ERR)
				{
					if(msg.Msg.msgType == TRDP_MSG_MQ)
					{
						// Confirmation timeout is generated with ReplyQ msgType because it is the one waiting the confirmation
						fprintf(stderr, "%s: confirm reception timeout\n", strTstName);
					}
					else
					{
						// Error
						fprintf(stderr, "%s ERROR: Unexpected msgType %d\n", strTstName, msg.Msg.msgType);
					}
				}
				else
				{
					// Error
					fprintf(stderr, "%s ERROR: resultCode expected %u or %u, found %u\n", strTstName, TRDP_NO_ERR, TRDP_TIMEOUT_ERR, msg.Msg.resultCode);
				}
			}
			break;

			default:
			{
				fprintf(stderr, "%s ERROR: Unexpected message with comID = %d\n", strTstName, msg.Msg.comId);
			}
			break;
		}
	}
	
	// Increment current FSM state
	rx_test_fsm_state++;
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
	TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};
	
	TRDP_ERR_T errv;

	memset(&mem_config,0,sizeof(mem_config));
	
	// Memory allocator config
	mem_config.p    = NULL;
	mem_config.size = HEAP_MEMORY_SIZE;
		
	//	MD config
	memset(&md_config,0,sizeof(md_config));
	md_config.pfCbFunction = md_indication;
	md_config.pRefCon      = (void *)0x12345678;
	md_config.sendParam.qos = TRDP_MD_DEFAULT_QOS;
	md_config.sendParam.ttl = TRDP_MD_DEFAULT_TTL;
	md_config.sendParam.retries = TRDP_MD_DEFAULT_RETRIES;
	md_config.flags = 0
		| TRDP_FLAGS_NONE      * 0
		| TRDP_FLAGS_REDUNDANT * 0
		| TRDP_FLAGS_MARSHALL  * 0
		| TRDP_FLAGS_CALLBACK  * 1
		| TRDP_FLAGS_TCP       * 0 /* 1=TCP, 0=UDP */
		;
	md_config.replyTimeout   = TRDP_MD_DEFAULT_REPLY_TIMEOUT;
	md_config.confirmTimeout = TRDP_MD_DEFAULT_CONFIRM_TIMEOUT;
	md_config.udpPort        = TRDP_MD_UDP_PORT;
	md_config.tcpPort        = TRDP_MD_UDP_PORT;

	/*	Init the library  */
	errv = tlc_init(
	    private_debug_printf,		/* debug print function */
	    &mem_config				/* Use application supplied memory	*/
	    );
	
	if (errv != TRDP_NO_ERR)
	{
	    fprintf(stderr,"tlc_init() error = %d\n",errv);
	    exit(EXIT_FAILURE);
	}


	/*	Open a session  */
	errv = tlc_openSession(
		&appHandle,			// TRDP_APP_SESSION_T			*pAppHandle
		0,					// TRDP_IP_ADDR_T				ownIpAddr
		0,					// TRDP_IP_ADDR_T				leaderIpAddr
		NULL,           	// TRDP_MARSHALL_CONFIG_T		*pMarshall
        NULL,				// const TRDP_PD_CONFIG_T		*pPdDefault
		&md_config,			// const TRDP_MD_CONFIG_T		*pMdDefault
        &processConfig		// const TRDP_PROCESS_CONFIG_T	*pProcessConfig
	);
	
	if (errv != TRDP_NO_ERR)
	{
	    fprintf(stderr,"tlc_openSession() error = %d\n",errv);
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
	TRDP_IP_ADDR_T ipDst,
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
	vos_htonl(ipDst),
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
    
    return 0;
}


// TEST: Send a MD Request
static int testRequestSendID = 0;

static int testRequestSend(
    UINT32 comId,
    UINT32 topoCount,
	TRDP_IP_ADDR_T ipDst,
	UINT32 noOfRepliers,
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
	vos_htonl(ipDst),
	0, /* flags */
	noOfRepliers, /* # of repliers */
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
    
    return 0;
}


// Test main loop
static int test_main_proc()
{
    TRDP_ERR_T errv;

    /* main process loop */
    int run = 1;
    int cliCmd = 0;
	int n = 0;
   
    while(run)
    {
		// * Test cli command execution
		ssize_t r = read(cliPipeFd[0], &cliCmd, sizeof(cliCmd));
		if (r == -1 && errno == EAGAIN)
		{
			// No commands
		}
		else if (r > 0)
		{
			// Cli command
			fprintf(stderr,"test_main_proc(): cliCmd = %c.\n", (char) (cliCmd & 0xff));

			// Command execution
			for(n = 0; n < sizeof(cliTests)/sizeof(cli_test); n++)
			{
				// Check command
				if(cliTests[n].cliChar == cliCmd)
				{
					// Reset test FSM
					rx_test_fsm_state = 0;

					// Send Notify
					if(cliTests[n].sendType == 0)
					{
						testNotifySend(cliTests[n].comID, cliTests[n].topoCnt, cliTests[n].dstIP, "", "");
						break;
					}
					
					// Send Request
					if(cliTests[n].sendType == 1)
					{
						testRequestSend(cliTests[n].comID, cliTests[n].topoCnt, cliTests[n].dstIP, cliTests[n].noOfRepliers, "", "");
						break;
					}
				}
			}
		}
		else
		{
			fprintf(stderr,"test_main_proc(): pipe read error\n");
			exit(EXIT_FAILURE);
		}


		// * TRDP handling
		fd_set rfds;
		INT32 noDesc = -1;
		struct timeval tv = {0,0};

		// cleanup
		FD_ZERO(&rfds);
		//printf("sizeof(fd_set)=%d\n",(int)sizeof(fd_set));

		errv = tlc_getInterval(
			appHandle,
			(TRDP_TIME_T *)&tv,
			(TRDP_FDS_T *)&rfds,
			&noDesc);

		// INFO
		//double sec = (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-9;
		//fprintf(stderr,"tlc_getInterval()=%d, time = %g, noDesc=%d\n",errv, sec, noDesc);

		// Polling Mode
		errv = tlc_process(appHandle,NULL,NULL);
		//fprintf(stderr,"tlc_process()=%d\n",errv);


		// call back process
		queue_procricz();

		// Loop delay
		usleep(x_period*1000);
    }
    
    return(0);
}


// *** Command line - Start ***

// TEST: add a single listener
static int testAddListener(
    const void              *pUserRef,
    UINT32                  comId,
	TRDP_IP_ADDR_T          destIpAddr,
    const TRDP_URI_USER_T   destURI)
{
    TRDP_ERR_T errv;
    errv = tlm_addListener(
	appHandle,
	&lisHandle,
	pUserRef,
	comId,
	0,
	vos_htonl(destIpAddr),
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
static const int test0002_lenv  = 10001002; // Dev2 Listener, comID 1002 Notify
static const int test0003_lenv  = 10001004; // Dev2 Listener, ComID 1003 Notify
static const int test0004_lenv1 = 10002001; // Dev1 Listener, comID 2001 Reply
static const int test0005_lenv1 = 10002002; // Dev1 Listener, comID 2002 Reply
static const int test0005_lenv2 = 20002002; // Dev1 Listener, comID 2002 Request
static const int test0006_lenv1 = 10002003; // Dev1 Listener, comID 2003 Reply
static const int test0006_lenv2 = 20002004; // Dev1 Listener, comID 2002 Request
static const int test0007_lenv1 = 10003001; // Dev1 Listener, comID 2002 Reply
static const int test0007_lenv2 = 20003001; // Dev1 Listener, comID 2002 Request, Confirm
static const int test0008_lenv1 = 10003002; // Dev1 Listener, comID 2003 Reply
static const int test0008_lenv2 = 20003002; // Dev1 Listener, comID 2002 Request, Confirm
static const int test0009_lenv2 = 20004001; // Dev2 Listener, comID 2002 Multicast Notify
static const int test0010_lenv1 = 10005001; // Dev1 Listener, comID 5001 Reply, 2 expected 0 received
static const int test0010_lenv2 = 20005001; // Dev2 Listener, comID 5001 Multicast Request
static const int test0011_lenv1 = 10005002; // Dev1 Listener, comID 5002 Reply, 2 expected 1 received
static const int test0011_lenv2 = 20005002; // Dev2 Listener, comID 5002 Multicast Request
static const int test0012_lenv1 = 10005003; // Dev1 Listener, comID 5003 Reply, 2 expected 2 received
static const int test0012_lenv2 = 20005003; // Dev2 Listener, comID 5003 Multicast Request
static const int test0012_lenv3 = 30005003; // Dev3 Listener, comID 5003 Multicast Request
static const int test0013_lenv1 = 10006001; // Dev1 Listener, comID 6001 Reply, unknown expected 0 received
static const int test0014_lenv1 = 10006002; // Dev1 Listener, comID 6002 Reply, unknown expected 1 received
static const int test0014_lenv2 = 20006002; // Dev2 Listener, comID 6002 Multicast Request
static const int test0015_lenv1 = 10006003; // Dev1 Listener, comID 6003 Reply, unknown expected 2 received
static const int test0015_lenv2 = 20006003; // Dev2 Listener, comID 6003 Multicast Request
static const int test0015_lenv3 = 30006003; // Dev3 Listener, comID 6003 Multicast Request
static const int test0016_lenv1 = 10007001; // Dev1 Listener, comID 7001 Reply, 2 expected 2 received, 0 confirm sent
static const int test0016_lenv2 = 20007001; // Dev2 Listener, comID 7001 Multicast Request, Singlecast Confirm
static const int test0016_lenv3 = 30007001; // Dev3 Listener, comID 7001 Multicast Request, Singlecast Confirm
static const int test0017_lenv1 = 10007002; // Dev1 Listener, comID 7002 Reply, 2 expected 2 received
static const int test0017_lenv2 = 20007002; // Dev2 Listener, comID 7002 Multicast Request, Singlecast Confirm
static const int test0017_lenv3 = 30007002; // Dev3 Listener, comID 7002 Multicast Request, Singlecast Confirm
static const int test0018_lenv1 = 10007003; // Dev1 Listener, comID 7003 Reply, 2 expected 2 received
static const int test0018_lenv2 = 20007003; // Dev2 Listener, comID 7003 Multicast Request, Singlecast Confirm
static const int test0018_lenv3 = 30007003; // Dev3 Listener, comID 7003 Multicast Request, Singlecast Confirm


// Init listeners
static int testInitListeners()
{
    // Dev1 mode listeners
    if(x_testmode == 1)
    {
		// TEST-0004: listener for comID 2001 Reply
		testAddListener(&test0004_lenv1, 2001, 0, "");
		
		// TEST-0005: listener for comID 2002 Reply
		testAddListener(&test0005_lenv1, 2002, 0, "");

		// TEST-0006: listener for comID 2003 Reply
		testAddListener(&test0006_lenv1, 2003, 0, "");
		
		// TEST-0007: listener for comID 3001 Reply
		testAddListener(&test0007_lenv1, 3001, 0, "");

		// TEST-0008: listener for comID 3002 Reply
		testAddListener(&test0008_lenv1, 3002, 0, "");
				
		// TEST-0010: listener for comID 5001 Reply 
		testAddListener(&test0010_lenv1, 5001, 0, "");
		
		// TEST-0011: listener for comID 5002 Reply 
		testAddListener(&test0011_lenv1, 5002, 0, "");
		
		// TEST-0012: listener for comID 5003 Reply 
		testAddListener(&test0012_lenv1, 5003, 0, "");
		
		// TEST-0013: listener for comID 6001 Reply 
		testAddListener(&test0013_lenv1, 6001, 0, "");
		
		// TEST-0014: listener for comID 6002 Reply 
		testAddListener(&test0014_lenv1, 6002, 0, "");
		
		// TEST-0015: listener for comID 6003 Reply 
		testAddListener(&test0015_lenv1, 6003, 0, "");
		
		// TEST-0016: listener for comID 7001 Reply 
		testAddListener(&test0016_lenv1, 7001, 0, "");
		
		// TEST-0017: listener for comID 7002 Reply 
		testAddListener(&test0017_lenv1, 7002, 0, "");
		
		// TEST-0018: listener for comID 7003 Reply 
		testAddListener(&test0018_lenv1, 7003, 0, "");
    }
    
    // Dev2 mode listeners
    if(x_testmode == 2)
    {
		// TEST-0002: listener
		testAddListener(&test0002_lenv, 1002, 0, "");

		// TEST-0003: listener
		testAddListener(&test0003_lenv, 1004, 0, "");
		
		// TEST-0005: listener for comID 2002 Request
		testAddListener(&test0005_lenv2, 2002, 0, "");

		// TEST-0006: listener for comID 2004 Request
		testAddListener(&test0006_lenv2, 2004, 0, "");
		
		// TEST-0007: listener for comID 3001 Request and Confirm
		testAddListener(&test0007_lenv2, 3001, 0, "");

		// TEST-0008: listener for comID 3002 Request and Confirm
		testAddListener(&test0008_lenv2, 3002, 0, "");

		// TEST-0009: listener for comID 4001 Multicast Notify
		testAddListener(&test0009_lenv2, 4001, x_ip4_mc_01, "");
		
		// TEST-0011: comID 5002 Multicast Request, Singlecast Confirm
		testAddListener(&test0011_lenv2, 5002, x_ip4_mc_02, "");
		
		// TEST-0012: comID 5003 Multicast Request, Singlecast Confirm
		testAddListener(&test0012_lenv2, 5003, x_ip4_mc_02, "");
		
		// TEST-0014: comID 6002 Multicast Request, Singlecast Confirm
		testAddListener(&test0014_lenv2, 6002, x_ip4_mc_02, "");
		
		// TEST-0015: comID 6003 Multicast Request, Singlecast Confirm
		testAddListener(&test0015_lenv2, 6003, x_ip4_mc_02, "");
		
		// TEST-0016: comID 7001 Multicast Request, Singlecast Confirm
		testAddListener(&test0016_lenv2, 7001, x_ip4_mc_02, "");

		// TEST-0017: comID 7002 Multicast Request, Singlecast Confirm
		testAddListener(&test0017_lenv2, 7002, x_ip4_mc_02, "");
		
		// TEST-0018: comID 7003 Multicast Request, Singlecast Confirm
		testAddListener(&test0018_lenv2, 7003, x_ip4_mc_02, "");
    }

    // Dev3 mode listeners
    if(x_testmode == 3)
    {
		// TEST-0009: listener for comID 4001 Multicast Notify
		testAddListener(&test0009_lenv2, 4001, x_ip4_mc_01, "");
		
		// TEST-0012: comID 5003 Multicast Request, Singlecast Confirm
		testAddListener(&test0012_lenv2, 5003, x_ip4_mc_02, "");
		
		// TEST-0015: comID 6003 Multicast Request, Singlecast Confirm
		testAddListener(&test0015_lenv2, 6003, x_ip4_mc_02, "");
		
		// TEST-0016: comID 7001 Multicast Request, Singlecast Confirm
		testAddListener(&test0016_lenv3, 7001, x_ip4_mc_02, "");

		// TEST-0017: comID 7002 Multicast Request, Singlecast Confirm
		testAddListener(&test0017_lenv3, 7002, x_ip4_mc_02, "");
		
		// TEST-0018: comID 7003 Multicast Request, Singlecast Confirm
		testAddListener(&test0018_lenv3, 7003, x_ip4_mc_02, "");
    }

    // LOG
    fprintf(stderr,"testInitListeners(): done.\n");
    return 0;
}


// Cli interactive console
void cliInteractiveCli()
{
    close(cliPipeFd[0]);
    
    // Command line handler
	int n = 0;
    char ch;
    
    int cmdRun = 1;
    while(cmdRun == 1)
    {
		// Check if parent task is diede
		if(getppid() < 2)
		{
			fprintf(stderr,"cliInteractiveCli(): error, parent task is died\n");
			exit(EXIT_FAILURE);
		}

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
				// Help
				printf("Commands:\n");
				printf("h) Print menu comands\n");
				for(n = 0; n < sizeof(cliTests)/sizeof(cli_test); n++)
				{
					printf("%c) %s [ComID %u] : %s\n", cliTests[n].cliChar, cliTests[n].tstName, cliTests[n].comID, cliTests[n].tstDescr);
				}
			}
			else
			{
				// Command trigger
				for(n = 0; n < sizeof(cliTests)/sizeof(cli_test); n++)
				{
					if(ch == cliTests[n].cliChar)
					{
						// Informations
						printf("\n");
						printf("%c) %s [ComID %u] : %s\n", cliTests[n].cliChar, cliTests[n].tstName, cliTests[n].comID, cliTests[n].tstDescr);
						
						// Send command to trdp test task
						int cliCmd = ch;
						write(cliPipeFd[1], &cliCmd, sizeof(cliCmd));
						
						// Exit loop
						break;
					}
				}
			}
		}
    }
}


// Init writer task
int cliInit()
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
    fprintf(stderr,"usage: %s [--dest a.b.c.d] [--period <perido in ms>]  [--testmode <1/2>]\n",argv[0]);
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
	x_ip4_mc_01 = TRDP_IP4_ADDR(225,0,0,5);
	x_ip4_mc_02 = TRDP_IP4_ADDR(225,0,0,6);
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

	//fprintf(stderr, "MD_HEADER_T size: %u\n", sizeof(MD_HEADER_T));
	
	// Log test mode
	fprintf(stderr, "main: start with testmode %u.", x_testmode);

	
	// Init test patterns destination IP
	int n = 0;
	for(n = 0; n < sizeof(cliTests)/sizeof(cli_test); n++)
	{
		// IP Dev2 destination
		if(cliTests[n].dstIP == 0)
		{
			cliTests[n].dstIP = x_ip4_dest;
		}
		
		// IP Multicat destination 1
		else if(cliTests[n].dstIP == 1)
		{
			cliTests[n].dstIP = x_ip4_mc_01;
		}

		// IP Multicat destination 2
		else if(cliTests[n].dstIP == 2)
		{
			cliTests[n].dstIP = x_ip4_mc_02;
		}
		
		// Not expected
		else
		{
			fprintf(stderr, "Error in %s: Unexpected destination type %u\n", cliTests[n].tstName, cliTests[n].dstIP);
			exit(EXIT_FAILURE);
		}
	}

	
	// Init queue
	queue_initialize();

	// Init 4 test
	test_initialize();

	// Init command line interface
	cliInit();
	
	// Add listeners
	testInitListeners();

	// Main loop
	test_main_proc();

	return(0);
}
