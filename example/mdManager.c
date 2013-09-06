/**********************************************************************************************************************/
/**
 * @file            mdManager.c
 *
 * @brief           Demo MD for TRDP
 *
 * @details         Receive and send process data, single threaded using select() and heap memory
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
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

#ifdef __linux__
        #include <uuid/uuid.h>
#endif

#include "trdp_types.h"
#include "trdp_if_light.h"
#include "trdp_private.h"
#include "trdp_utils.h"


#define TRDP_IP4_ADDR(a, b, c, d)    ((am_big_endian()) ?                                                                                                      \
                                      ((UINT32) ((a) & 0xFF) << 24) | ((UINT32) ((b) & 0xFF) << 16) | ((UINT32) ((c) & 0xFF) << 8) | ((UINT32) ((d) & 0xFF)) : \
                                      ((UINT32) ((d) & 0xFF) << 24) | ((UINT32) ((c) & 0xFF) << 16) | ((UINT32) ((b) & 0xFF) << 8) | ((UINT32) ((a) & 0xFF)))

#define HEAP_MEMORY_SIZE    (1 * 1024 * 1024)

#ifndef EOK
        #define EOK         0
#endif

/* message queue trdp to application */
typedef struct
{
    void           *pRefCon;
    TRDP_MD_INFO_T Msg;
    UINT8          *pData;
    UINT32         dataSize;
    int            dummy;
} trdp_apl_cbenv_t;

#define local_max(a, b)    (((a) > (b)) ? (a) : (b))

/* Q name */
#define TRDP_QUEUE_NAME        "/trdp_queue_4"
#define TRDP_QUEUE_MAX_SIZE    (sizeof(trdp_apl_cbenv_t) - 2)
#define TRDP_QUEUE_MAX_MESG    10


/* global vars */

static TRDP_APP_SESSION_T appHandle;
static TRDP_MD_CONFIG_T   md_config;
static TRDP_MEM_CONFIG_T  mem_config;

static TRDP_LIS_T         lisHandle;

static mqd_t              trdp_mq;


/* command line arguments */
static int            x_uuid;
static int            x_notify;
static int            x_request;
static int            x_reqconf;
static int            x_receiver;
static TRDP_IP_ADDR_T x_ip4_dest;

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
    fprintf(stderr, "r=%p c=%d t=%s f=%s l=%d m=%s", pRefCon, category, pTime, pFile, LineNumber, pMsgStr);
}

static void print_session(const UINT8 *p)
{
    int i;
    for (i = 0; i < 16; i++)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            printf("-");
        printf("%02X", p[i] & 0xFF);
    }
    printf("\n");
}

static void print_uri(const CHAR8 *p)
{
    int i;
    for (i = 0; i < 32; i++)
    {
        if (p[i] == 0)
            break;
        printf("%c", p[i] & 0xFF);
    }
}

static void print_memory(const void *p, const int l)
{
    if (p != NULL || l > 0)
    {
        int         i, j;
        const UINT8 *b = p;
        for (i = 0; i < l; i += 16)
        {
            printf("%04X ", i);
            /**/
            for (j = 0; j < 16; j++)
            {
                if (j == 8)
                    printf("- ");
                if ((i + j) < l)
                {
                    int ch = b[i + j];
                    printf("%02X ", ch);
                }
                else
                {
                    printf("   ");
                }
            }
            printf("   ");
            for (j = 0; j < 16 && (i + j) < l; j++)
            {
                int ch = b[i + j];
                printf("%c", (ch >= ' ' && ch <= '~') ? ch : '.');
            }
            printf("\n");
        }
    }
}

/* queue functions */

static void queue_initialize()
{
    struct mq_attr new_ma;
    struct mq_attr old_ma;
    struct mq_attr * pma;
    int            rc;

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

    printf("pma=%p\n", pma);

    /* create a queue */
    trdp_mq = mq_open(TRDP_QUEUE_NAME, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR, pma);
    if ((mqd_t) -1 == trdp_mq)
    {
        perror("mq_open()");
        exit(EXIT_FAILURE);
    }
    /* get attirubtes */
    rc = mq_getattr(trdp_mq, &old_ma);
    if (-1 == rc)
    {
        perror("mq_getattr()");
        exit(EXIT_FAILURE);
    }
    printf("mq_flags   = x%lX\n", old_ma.mq_flags);
    printf("mq_maxmsg  = %ld\n", old_ma.mq_maxmsg);
    printf("mq_msgsize = %ld\n", old_ma.mq_msgsize);
    printf("mq_curmsgs = %ld\n", old_ma.mq_curmsgs);

    new_ma = old_ma;

    new_ma.mq_flags = O_NONBLOCK;

    /* change attributes */
    rc = mq_setattr(trdp_mq, &new_ma, &old_ma);
    if (-1 == rc)
    {
        perror("mq_setattr()");
        exit(EXIT_FAILURE);
    }

    /* get attirubtes */
    rc = mq_getattr(trdp_mq, &old_ma);
    if (-1 == rc)
    {
        perror("mq_getattr()");
        exit(EXIT_FAILURE);
    }
    printf("mq_flags   = x%lX\n", old_ma.mq_flags);
    printf("mq_maxmsg  = %ld\n", old_ma.mq_maxmsg);
    printf("mq_msgsize = %ld\n", old_ma.mq_msgsize);
    printf("mq_curmsgs = %ld\n", old_ma.mq_curmsgs);
}

/* send message trough queue */
static void queue_sendmessage(trdp_apl_cbenv_t * msg)
{
    int  rc;
    char * p_bf = (char *) msg;
    int  l_bf   = sizeof(*msg) - sizeof(msg->dummy);
    rc = mq_send(trdp_mq, p_bf, l_bf, 0);
    if (-1 == rc)
    {
        perror("mq_send()");
        exit(EXIT_FAILURE);
    }
}

/* receive message from queue */
static int queue_receivemessage(trdp_apl_cbenv_t * msg)
{
    ssize_t  rc;
    char     * p_bf = (char *) msg;
    int      l_bf   = sizeof(*msg) - sizeof(msg->dummy);
    int      s_bf   = sizeof(*msg) - 1;
    unsigned msg_prio;

    msg_prio = 0;
    rc       = mq_receive(trdp_mq, p_bf, s_bf, &msg_prio);
    if ((ssize_t) -1 == rc)
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
        fprintf(stderr, "mq_receive() expected %d bytes, not %d\n", l_bf, (int) rc);
        exit(EXIT_FAILURE);
    }
    printf("Received %d bytes\n", (int) rc);
    return EOK;
}

/* MD application server */
static void queue_procricz()
{
    trdp_apl_cbenv_t msg;
    int              rc = queue_receivemessage(&msg);
    if (rc != EOK)
        return;

    {
        printf("md_indication(r=%p d=%p l=%d)\n", msg.pRefCon, msg.pData, msg.dataSize);

        printf("srcIpAddr         = x%08X\n", msg.Msg.srcIpAddr);
        printf("destIpAddr        = x%08X\n", msg.Msg.destIpAddr);
        printf("seqCount          = %d\n", msg.Msg.seqCount);
        printf("protVersion       = %d\n", msg.Msg.protVersion);
        printf("msgType           = x%04X\n", msg.Msg.msgType);
        printf("comId             = %d\n", msg.Msg.comId);
        printf("topoCount         = %d\n", msg.Msg.topoCount);
        printf("userStatus        = %d\n", msg.Msg.userStatus);
        printf("replyStatus       = %d\n", msg.Msg.replyStatus);
        printf("sessionId         = ");      print_session(msg.Msg.sessionId);
        printf("replyTimeout      = %d\n", msg.Msg.replyTimeout);
        printf("destURI           = ");      print_uri(msg.Msg.destURI); printf("\n");
        printf("srcURI            = ");      print_uri(msg.Msg.srcURI); printf("\n");
        printf("numReplies       = %d\n", msg.Msg.numReplies);
        printf("pUserRef          = %p\n", msg.Msg.pUserRef);
        printf("resultCode        = %d\n", msg.Msg.resultCode);

        print_memory(msg.pData, msg.dataSize);
    }

    switch (msg.Msg.msgType)
    {
    case TRDP_MSG_MN:
    {
        printf("MD: Notify\n");
    }
    break;
    case TRDP_MSG_MR:
    {
        printf("MD: Request with reply\n");
        if (x_reqconf == 0)
        {
            char outbuf[80];
            int  szout = sprintf(outbuf, "tlm_reply %d", (int) time(0));

            {
                TRDP_ERR_T errv = tlm_reply(
                    appHandle,
                    (void *) 0x2000CAFE,
                    (const TRDP_UUID_T*) &(msg.Msg.sessionId),
                    msg.Msg.topoCount,
                    msg.Msg.comId,
                    msg.Msg.destIpAddr,
                    msg.Msg.srcIpAddr,
                    0,                                     /* pktFlags */
                    0,                                     /*  userStatus */
                    NULL,                                  /* send param */
                    (UINT8 *) outbuf,
                    szout + 1,
                    msg.Msg.destURI,
                    msg.Msg.srcURI);
                printf("tlm_reply()=%d\n", errv);
            }
        }
        else
        {
            char outbuf[80];
            int  szout = sprintf(outbuf, "tlm_replyQuery %d", (int) time(0));

            {
                TRDP_ERR_T errv = tlm_replyQuery(
                    appHandle,
                    (void *) 0x2000CAFE,
                    (const TRDP_UUID_T*)&(msg.Msg.sessionId),
                    msg.Msg.topoCount,
                    msg.Msg.comId,
                    msg.Msg.destIpAddr,
                    msg.Msg.srcIpAddr,
                    0,                                     /* pktFlags */
                    0,                                     /*  userStatus */
                    5 * 1000 * 1000,                       /* confirm timeout */
                    NULL,                                  /* send param */
                    (UINT8 *) outbuf,
                    szout + 1,
                    msg.Msg.destURI,
                    msg.Msg.srcURI);
                printf("tlm_replyQuery()=%d\n", errv);
            }
        }
    }
    break;
    case TRDP_MSG_MP:
    {
        printf("MD: Reply without confirmation\n");
    }
    break;
    case TRDP_MSG_MQ:
    {
        printf("MD: Reply with confirmation request\n");
        {
            TRDP_ERR_T errv = tlm_confirm(
                appHandle,
                (void *) 0x2000CAFE,
                (const TRDP_UUID_T *) &msg.Msg.sessionId,
                msg.Msg.comId,
                msg.Msg.topoCount,
                msg.Msg.destIpAddr,
                msg.Msg.srcIpAddr,
                0,                                 /* pktFlags */
                0,                                 /*  userStatus */
                0,                                 /* replystatus */
                NULL,                              /* send param */
                msg.Msg.destURI,
                msg.Msg.srcURI);
            printf("tlm_confirm()=%d\n", errv);
        }
    }
    break;
    case TRDP_MSG_MC:
    {
        printf("MD: Reply Confirm\n");
    }
    break;
    default:
    {
        printf("Message code = x%04X unhandled\n", msg.Msg.msgType);
    }
    break;
    }
}


/* *****/

static void test_uuid()
{
#ifdef __linux__
    uuid_t uuid;
    int    i;

    uuid_generate_time(uuid);
    printf("uuid_generate_time(): ");
    for (i = 0; i < sizeof(uuid); i++)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            printf("-");
        printf("%02X", uuid[i] & 0xFF);
    }
    printf("\n");
#endif
}


/* call back function for message data */
static void md_indication(
    void                    *pRefCon,
    TRDP_APP_SESSION_T appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32 dataSize)
{
    printf("md_indication(r=%p m=%p d=%p l=%d)\n", pRefCon, pMsg, pData, dataSize);

        #if 0
    printf("srcIpAddr         = x%08X\n", pMsg->srcIpAddr);
    printf("destIpAddr        = x%08X\n", pMsg->destIpAddr);
    printf("seqCount          = %d\n", pMsg->seqCount);
    printf("protVersion       = %d\n", pMsg->protVersion);
    printf("msgType           = x%04X\n", pMsg->msgType);
    printf("comId             = %d\n", pMsg->comId);
    printf("topoCount         = %d\n", pMsg->topoCount);
    printf("userStatus        = %d\n", pMsg->userStatus);
    printf("replyStatus       = %d\n", pMsg->replyStatus);
    printf("sessionId         = ");      print_session(pMsg->sessionId);
    printf("replyTimeout      = %d\n", pMsg->replyTimeout);
    printf("destURI           = ");      print_uri(pMsg->destURI); printf("\n");
    printf("srcURI            = ");      print_uri(pMsg->srcURI); printf("\n");
    printf("numReplies        = %d\n", pMsg->numReplies);
    printf("pUserRef          = %p\n", pMsg->pUserRef);
    printf("resultCode        = %d\n", pMsg->resultCode);

    print_memory(pData, dataSize);
        #endif

    {
        trdp_apl_cbenv_t fwd;

        fwd.pRefCon  = pRefCon;
        fwd.Msg      = *pMsg;
        fwd.pData    = pData;
        fwd.dataSize = dataSize;

        queue_sendmessage(&fwd);
    }
}

static int test_initialize()
{
    TRDP_ERR_T            errv;
    TRDP_MEM_CONFIG_T     dynamicConfig = { NULL, HEAP_MEMORY_SIZE, {} };
    TRDP_PROCESS_CONFIG_T processConfig = { "Me", "", 0, 0, TRDP_OPTION_BLOCK };
    memset(&md_config, 0, sizeof(md_config));
    memset(&mem_config, 0, sizeof(mem_config));

    /* memory allocator config */
    mem_config.p    = NULL;
    mem_config.size = HEAP_MEMORY_SIZE;

    /* MD config */
    md_config.pfCbFunction  = md_indication;
    md_config.pRefCon       = (void *) 0x12345678;
    md_config.sendParam.qos = TRDP_MD_DEFAULT_QOS;
    md_config.sendParam.ttl = TRDP_MD_DEFAULT_TTL;
    md_config.flags         = 0
                              | TRDP_FLAGS_NONE * 0
                              | TRDP_FLAGS_MARSHALL * 0
                              | TRDP_FLAGS_CALLBACK * 1
                              | TRDP_FLAGS_TCP * 0 /* 1=TCP, 0=UDP */
    ;
    md_config.replyTimeout   = TRDP_MD_DEFAULT_REPLY_TIMEOUT;
    md_config.confirmTimeout = TRDP_MD_DEFAULT_CONFIRM_TIMEOUT;
    md_config.udpPort        = TRDP_MD_UDP_PORT;
    md_config.tcpPort        = TRDP_MD_UDP_PORT;


    /*  Init the library for non-blocking operation     */
    errv = tlc_init(&private_debug_printf, &dynamicConfig);
    if (errv != TRDP_NO_ERR)
    {
        fprintf(stderr, "tlc_init() error = %d\n", errv);
        return EXIT_FAILURE;
    }
    if (errv != TRDP_NO_ERR)
    {
        fprintf(stderr, "tlc_init() error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    /*	Open a session for callback operation */
    errv = tlc_openSession(&appHandle,
                           0, 0,                      /* use default IP addresses */
                           NULL,                      /* no Marshalling */
                           NULL, &md_config,          /* system defaults for PD and MD	*/
                           &processConfig);
    if (errv != TRDP_NO_ERR)
    {
        fprintf(stderr, "tlc_openSession() error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    /* set network topo counter */
    tlc_setTopoCount(appHandle, 151);

    return 0;
}

static int test_notify()
{
    TRDP_ERR_T errv;

    /* notify MD */
    char outbuf[80];
    int  szout = sprintf(outbuf, "tlm_notify %d", (int) time(0));
    errv = tlm_notify(
        appHandle,
        NULL,      /* user ref */
        123,       /* comId */
        150,       /* topoCount */
        0,         /* own IP address from trdp stack */
        vos_htonl(x_ip4_dest),
        0,         /* flags */
        NULL,      /* default send param */
        (UINT8 *) &outbuf[0],
        szout + 1,
        "a.b.c.d",
        "e.f.g.h"
        );
    if (errv != TRDP_NO_ERR)
    {
        fprintf(stderr, "tlm_notify() error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    return 0;
}

static int test_request()
{
    TRDP_ERR_T errv;

    /* notify MD */
    char        outbuf[80];
    int         szout = sprintf(outbuf, "tlm_request %d", (int) time(0));

    TRDP_UUID_T session;

    errv = tlm_request(
        appHandle,
        (void *) 0x1000CAFE, /* user ref */
        &session,
        123,                 /* comId */
        150,                 /* topoCount */
        0,                   /* own IP address from trdp stack */
        vos_htonl(x_ip4_dest),
        0,                   /* flags */
        1,                   /* # of repliers */
        5 * 1000 * 1000,     /* reply timeout */
        NULL,                /* default send param */
        (UINT8 *) &outbuf[0],
        szout + 1,
        "a.b.c.d",
        "e.f.g.h"
        );
    if (errv != TRDP_NO_ERR)
    {
        fprintf(stderr, "tlm_request() error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    return 0;
}

static const int rice_env = 12345678;

static int test_receiver()
{
    TRDP_ERR_T errv;

    errv = tlm_addListener(
        appHandle,
        &lisHandle,
        &rice_env,
        123,
        0,
        vos_htonl(0),
        0,
        "ciao");
    if (errv != TRDP_NO_ERR)
    {
        fprintf(stderr, "tlm_addListener() error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    printf("tlm_addListener() lisHandle = x%p\n", lisHandle);

    return(0);
}

static int test_main_proc()
{
    int loop;

    /* main process loop */
    for (loop = 0; loop < 1000; loop++)
    {
        fd_set         rfds;
        INT32          noDesc = -1;
        struct timeval tv     = { 0, 0 };

        /* cleanup */
        FD_ZERO(&rfds);

        /* printf("sizeof(fd_set)=%d\n",(int)sizeof(fd_set)); */

        tlc_getInterval(
            appHandle,
            (TRDP_TIME_T *) &tv,
            (TRDP_FDS_T *) &rfds,
            &noDesc);
        //printf("tlc_getInterval()=%d\n",errv);

        {
            //double sec = (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-9;
            //printf("time = %g\n", sec);
        }
        //printf("noDesc=%d\n",noDesc);


        /* Polling Mode */
        tlc_process(appHandle, NULL, NULL);
        //printf("tlc_process()=%d\n",errv);

        /* call back process */
        queue_procricz();

        /* small delay */
        usleep(500 * 1000);

        printf("."); fflush(stdout);
    }

    return(0);
}

static void cmdlinerr(int argc, char * argv[])
{
    fprintf(stderr, "usage: %s [--uuid] [--notify] [--request] [--reqconf] [--listener] [--dest a.b.c.d]\n", argv[0]);
}

#define needargs(n)    { if ((n) > (argc - i)) { cmdlinerr(argc, argv); exit(EXIT_FAILURE); } }

int main(int argc, char * argv[])
{
    int i;

    /**/
    x_uuid     = 0;
    x_notify   = 0;
    x_request  = 0;
    x_reqconf  = 0;
    x_receiver = 0;
    x_ip4_dest = TRDP_IP4_ADDR(192, 168, 190, 129);
    /**/

    for (i = 1; i < argc; i++)
    {
        if (0 == strcmp(argv[i], "--uuid"))
        {
            x_uuid = 1;
            continue;
        }
        if (0 == strcmp(argv[i], "--notify"))
        {
            x_notify = 1;
            continue;
        }
        if (0 == strcmp(argv[i], "--request"))
        {
            x_request = 1;
            continue;
        }
        if (0 == strcmp(argv[i], "--reqconf"))
        {
            x_reqconf = 1;
            continue;
        }
        if (0 == strcmp(argv[i], "--listener"))
        {
            x_receiver = 1;
            continue;
        }
        if (0 == strcmp(argv[i], "--dest"))
        {
            needargs(2);
            i++;
            int ip[4];
            if (4 != sscanf(argv[i], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]))
            {
                cmdlinerr(argc, argv);
                exit(EXIT_FAILURE);
            }
            x_ip4_dest = TRDP_IP4_ADDR(ip[0], ip[1], ip[2], ip[3]);
            continue;
        }

        cmdlinerr(argc, argv);
        exit(EXIT_FAILURE);
    }


    /* init queue */
    queue_initialize();

    /* Init 4 test */
    test_initialize();


    if (x_uuid)
    {
        test_uuid();
    }

    if (x_receiver)
    {
        test_receiver();
    }

    if (x_notify)
    {
        test_notify();
    }

    if (x_request)
    {
        test_request();
    }

    if (x_notify || x_request || x_receiver)
    {
        test_main_proc();
    }

    printf("\nThat's All Folks!\n");

    return(0);
}
