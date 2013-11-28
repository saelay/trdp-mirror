/**********************************************************************************************************************/
/**
 * @file            vtest.h
 *
 * @brief           Test VOS functions
 *
 * @details         -
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Christoph Schneider, Bombardier Transportation GmbH
 *
 *
 * @remarks         All rights reserved. Reproduction, modification, use or disclosure
 *                  to third parties without express authority is forbidden,
 *                  Copyright Bombardier Transportation GmbH, Germany, 2013.
 *
 * $Id$
 */
/**************************/
/* error flags definition */
/**************************/
#define WIN32
/* We use dynamic memory    */
#define RESERVED_MEMORY  200000
#define MAXKEYSIZE 25
#define cBufSize 1
/* define output detail level */
#define OUTPUT_DETAILS OUTPUT_FULL
#define N_ITERATIONS 1

#define printOut(lvl,format, ...) {if (lvl <= OUTPUT_DETAILS) \
    {(void) printf(format, __VA_ARGS__);}};

#define IF_IP "192.168.64.3"
#define DEST_IP "192.168.64.117"
#define MC_IP "238.0.0.1"
#define MC_IF "192.168.64.3"
#define PORT_PD 20548
#define PORT_MD 20550
#ifdef WIN32
#define THREAD_POLICY VOS_THREAD_POLICY_OTHER
#else 
#ifdef VXWORKS
#define THREAD_POLICY VOS_THREAD_POLICY_RR
#endif
#endif

#include "stdio.h"
#include "vos_types.h"
#include "trdp_types.h"
#include "trdp_if_light.h"
#include "vos_shared_mem.h"
#include "vos_utils.h"

typedef enum
{
    MEM_NO_ERR      =  0,
    MEM_INIT_ERR    =  1,
    MEM_ALLOC_ERR   =  2,
    MEM_QUEUE_ERR   =  4,
    MEM_HELP_ERR    =  8,
    MEM_COUNT_ERR   = 16,
    MEM_DELETE_ERR  = 32,
    MEM_ALL_ERR     = 63
} MEM_ERR_T;

typedef enum
{
    THREAD_NO_ERR               =    0,
    THREAD_INIT_ERR             =    1,
    THREAD_GETTIME_ERR          =    2,
    THREAD_GETTIMESTAMP_ERR     =    4,
    THREAD_ADDTIME_ERR          =    8,
    THREAD_SUBTIME_ERR          =   16,
    THREAD_MULTIME_ERR          =   32,
    THREAD_DIVTIME_ERR          =   64,
    THREAD_CMPTIME_ERR          =  128,
    THREAD_CLEARTIME_ERR        =  256,
    THREAD_UUID_ERR             =  512,
    THREAD_MUTEX_ERR            = 1024,
    THREAD_SEMA_ERR             = 2048,
    THREAD_ALL_ERR              = 4095
} THREAD_ERR_T;

typedef enum
{
    SOCK_NO_ERR         =  0,
    SOCK_HELP_ERR       =  1,
    SOCK_INIT_ERR       =  2,
    SOCK_UDP_ERR        =  4,
    SOCK_TCP_CLIENT_ERR =  8,
    SOCK_UDP_MC_ERR     = 16,
    SOCK_TCP_SERVER_ERR = 32,
    SOCK_ALL_ERR        = 63
} SOCK_ERR_T;

typedef enum
{
    SHMEM_NO_ERR    = 0,
    SHMEM_ALL_ERR   = 1,
} SHMEM_ERR_T;

typedef enum
{
    UTILS_NO_ERR        = 0,
    UTILS_INIT_ERR      = 1,
    UTILS_CRC_ERR       = 2,
    UTILS_TERMINATE_ERR = 4,
    UTILS_ALL_ERR       = 7
} UTILS_ERR_T;

typedef enum
{
    OUTPUT_BASIC    = 0,    /* print basic information only */
    OUTPUT_ADVANCED = 1,    /* print more / deeper information */
    OUTPUT_FULL     = 2     /* print all available information */
} OUTPUT_DETAILS_T;

typedef enum
{
    ROLE_MASTER     = 0,
    ROLE_SLAVE      = 1,
} TEST_ROLE_T;

const CHAR8 *cDefaultIface = "fec0";
/* define communication test identity (Master / Slave) */
const TEST_ROLE_T TEST_ROLE = ROLE_MASTER;
UINT32 gTestIP = 0;
UINT16 gTestPort = 0;

typedef struct arg_struct_thread {
    VOS_QUEUE_T queueHeader;
    VOS_SEMA_T queueSema;
    VOS_SEMA_T printSema;
    VOS_SEMA_T helpSema;
    struct timespec delay;
    VOS_ERR_T result;
}TEST_ARGS_THREAD;

typedef struct arg_struct_shmem {
    UINT32 size;
    UINT32 content;
    CHAR8 key[MAXKEYSIZE];
    VOS_SEMA_T sema;
    VOS_ERR_T result;
}TEST_ARGS_SHMEM;

/*
void dbgOut (
             void        *pRefCon,
             TRDP_LOG_T  category,
             const CHAR8 *pTime,
             const CHAR8 *pFile,
             UINT16      LineNumber,
             const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:"};
    if (category == VOS_LOG_DBG)
    {
        return;
    }

    printf("%s %s %s:%d %s",
           pTime,
           catStr[category],
           pFile,
           LineNumber,
           pMsgStr);
}*/
