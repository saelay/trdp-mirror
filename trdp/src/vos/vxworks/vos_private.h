/**********************************************************************************************************************/
/**
 * @file            vxworks/vos_private.h
 *
 * @brief           Private definitions for the OS abstraction layer
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Christoph Schneider, Bombardier Transportation GmbH
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id: vos_private.h 253 2013-01-07 13:48:40Z aweiss $
 *
 */

#ifndef VOS_PRIVATE_H
#define VOS_PRIVATE_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include <pthread.h>

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

//VOS_ERR_T   vos_mutexLocalCreate (struct VOS_MUTEX *pMutex);
//void        vos_mutexLocalDelete (struct VOS_MUTEX *pMutex);

#ifdef __cplusplus
}
#endif

#endif /* VOS_UTILS_H */
