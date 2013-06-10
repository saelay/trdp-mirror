/**********************************************************************************************************************/
/**
 * @file            windows/vos_shared_mem.c
 *
 * @brief           Shared Memory functions
 *
 * @details         OS abstraction of Shared memory access and control
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, TOSHIBA
 *
 *
 * @remarks         All rights reserved. Reproduction, modification, use or disclosure
 *                  to third parties without express authority is forbidden,
 *                  Copyright TOSHIBA, Japan, 2013.
 *
 *
 * $Id: vos_mem.h 282 2013-01-11 07:08:44Z 97029 $
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#ifndef POSIX
#error \
    "You are trying to compile the POSIX implementation of vos_sock.c - adapt the file to WIN32 or exclude this file!"
#endif
#ifndef WIN32
#error \
    "You are trying to compile the WIN32 implementation of vos_sock.c - either define WIN32 or exclude this file!"
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "vos_types.h"
#include "vos_utils.h"
#include "vos_shared_mem.h"
#include "vos_thread.h"
#include "vos_private.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */


/***********************************************************************************************************************
 *  LOCALS
 */


/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */


/**********************************************************************************************************************/
/*    Shared memory
                                                                                                               */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Create a shared memory area or attach to existing one.
 *  The first call with the a specified key will create a shared memory area with the supplied size and will return
 *  a handle and a pointer to that area. If the area already exists, the area will be attached.
 *    This function is not available in each target implementation.
 *
 *  @param[in]      pKey               Unique identifier (file name)
 *  @param[out]     pHandle            Pointer to returned handle
 *  @param[out]     ppMemoryArea       Pointer to pointer to memory area
 *  @param[in,out]  pSize              Pointer to size of area to allocate, on return actual size after attach
 *  @retval         VOS_NO_ERR         no error
 *  @retval         VOS_MEM_ERR        no memory available
 */
EXT_DECL VOS_ERR_T vos_sharedOpen (
    const CHAR8 *pKey,
    VOS_SHRD_T  *pHandle,
    UINT8       * *ppMemoryArea,
    UINT32      *pSize)
{
    VOS_ERR_T       ret         = VOS_MEM_ERR;
    mode_t          PERMISSION  = 0666;     /* Shared Memory permission is rw-rw-rw- */
    static INT32    fd;                     /* Shared Memory file descriptor */
    struct  stat    sharedMemoryStat;       /* Shared Memory Stat */

    /* Shared Memory Open */
    fd = shm_open(pKey, O_CREAT | O_RDWR, PERMISSION);
    if (fd == -1)
    {
        vos_printf(VOS_LOG_ERROR, "Shared Memory Create failed\n");
        return ret;
    }
    /* Shared Memory acquire */
    if (ftruncate(fd, (__off_t )*pSize) == -1)
    {
        vos_printf(VOS_LOG_ERROR, "Shared Memory Acquire failed\n");
        return ret;
    }
    /* Get Shared Memory Stats */
    fstat(fd, &sharedMemoryStat);
    if (sharedMemoryStat.st_size != (__off_t )*pSize)
    {
        vos_printf(VOS_LOG_ERROR, "Shared Memory Size failed\n");
        return ret;
    }

    /* Mapping Shared Memory */
    *ppMemoryArea = mmap(NULL, sharedMemoryStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*ppMemoryArea == MAP_FAILED)
    {
        vos_printf(VOS_LOG_ERROR, "Shared Memory memory-mapping failed\n");
        return ret;
    }
    /* Initialize Shared Memory */
    memset(*ppMemoryArea, 0, sharedMemoryStat.st_size);
    /* Handle */
    *pHandle = (VOS_SHRD_T) vos_memAlloc(sizeof (struct VOS_SHRD));
    if (*pHandle == NULL)
    {
        vos_printf(VOS_LOG_ERROR, "Shared Memory Handle create failed\n");
        return ret;
    }
    else
    {
        (*pHandle)->fd = fd;
    }

    ret = VOS_NO_ERR;
    return ret;
}

/**********************************************************************************************************************/
/** Close connection to the shared memory area.
 *  If the area was created by the calling process, the area will be closed (freed). If the area was attached,
 *  it will be detached.
 *    This function is not available in each target implementation.
 *
 *  @param[in]      handle             Returned handle
 *  @param[in]      pMemoryArea        Pointer to memory area
 *  @retval         VOS_NO_ERR         no error
 *  @retval         VOS_MEM_ERR        no memory available
 */

EXT_DECL VOS_ERR_T vos_sharedClose (
    VOS_SHRD_T  handle,
    const UINT8 *pMemoryArea)
{
    if (close(handle->fd) == -1)
    {
        vos_printf(VOS_LOG_ERROR, "Shared Memory file close failed\n");
        return VOS_MEM_ERR;
    }
    if (shm_unlink(handle->sharedMemoryName) == -1)
    {
        vos_printf(VOS_LOG_ERROR, "Shared Memory unLink failed\n");
        return VOS_MEM_ERR;
    }
    return VOS_NO_ERR;
}
