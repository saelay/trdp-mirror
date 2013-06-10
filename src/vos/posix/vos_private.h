/**********************************************************************************************************************/
/**
 * @file            posix/vos_private.h
 *
 * @brief           Private definitions for the OS abstraction layer
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
 */

#ifndef VOS_PRIVATE_H
#define VOS_PRIVATE_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include <pthread.h>
#include <sys/types.h>

#include "vos_types.h"
#include "vos_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

struct VOS_MUTEX
{
    UINT32          magicNo;
    pthread_mutex_t mutexId;
};

struct VOS_SHRD
{
    INT32   fd;                     /* File descriptor */
    CHAR8   *sharedMemoryName;      /* shared memory Name */
};

VOS_ERR_T   vos_mutexLocalCreate (struct VOS_MUTEX *pMutex);
void        vos_mutexLocalDelete (struct VOS_MUTEX *pMutex);

#if (((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE) || __APPLE__)
#   define STRING_ERR(pStrBuf)  (void)strerror_r(errno, pStrBuf, VOS_MAX_ERR_STR_SIZE);
#else
#   define STRING_ERR(pStrBuf)                                         \
    {                                                                  \
        char *pStr = strerror_r(errno, pStrBuf, VOS_MAX_ERR_STR_SIZE); \
        if (pStr != NULL)                                              \
        {                                                              \
            strncpy(pStrBuf, pStr, VOS_MAX_ERR_STR_SIZE);              \
        }                                                              \
    }
#endif

#ifdef __cplusplus
}
#endif

#endif /* VOS_UTILS_H */
