/**********************************************************************************************************************/
/**
 * @file            vxworks/vos_shared_mem.c
 *
 * @brief           Shared Memory functions
 *
 * @details         OS abstraction of Shared memory access and control
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Christoph Schneider, Bombardier Transportationt GmbH
 *
 *
 * @remarks         All rights reserved. Reproduction, modification, use or disclosure
 *                  to third parties without express authority is forbidden,
 *                  Copyright Bombardier Transportation GmbH, Germany, 2013.
 *
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#ifndef VXWORKS
#error \
    "You are trying to compile the VXWORKS implementation of vos_shared_mem.c - either define VXWORKS or exclude this file!"
#endif

#include <stdio.h>
#include <stddef.h>
#ifndef VXWORKS
#include <stdint.h>
#endif
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "vos_types.h"
#include "vos_utils.h"
#include "vos_shared_mem.h"
#include "vos_thread.h"
#include "vos_private.h"

/************************************************************************************************************************
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
   
}
