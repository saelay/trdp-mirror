/**********************************************************************************************************************/
/**
 * @file            vos_utils.h
 *
 * @brief           Typedefs for OS abstraction
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

#ifndef VOS_UTILS_H
#define VOS_UTILS_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include "vos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

extern VOS_PRINT_DBG_T gPDebugFunction;
extern void *gRefCon;

/** Debug output macro without formatting options	*/
#define vos_print(level, string)  {if(gPDebugFunction != NULL)          \
                                   {gPDebugFunction(gRefCon,            \
                                                    (level),            \
                                                    vos_getTimeStamp(), \
                                                    (__FILE__),         \
                                                    (__LINE__),         \
                                                    (string)); }}

/** Debug output macro with formatting options	*/
#define vos_printf(level, format, args ...)           \
    {if (gPDebugFunction != NULL)                     \
     {   char str[256];                               \
         snprintf(str, sizeof(str), format, ## args); \
         vos_print(level, str);                       \
     }                                                \
    }

/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/** Calculate CRC for the given buffer and length.
 *  For TRDP FCS CRC calculation the CRC32 according to IEEE802.3 with start value 0xffffffff is used.
 *
 *  @param[in]          crc             Initial value.
 *  @param[in,out]      pData           Pointer to data.
 *  @param[in]          dataLen         length in bytes of data.
 *  @retval             crc32 according to IEEE802.3
 */

EXT_DECL UINT32 vos_crc32 (
    UINT32      crc,
    const UINT8 *pData,
    UINT32      dataLen);


#ifdef __cplusplus
}
#endif

#endif /* VOS_UTILS_H */
