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
#include <stddef.h>
#include "vos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

extern VOS_PRINT_DBG_T gPDebugFunction;
extern void *gRefCon;

/** String size definitions for the debug output functions */
#define VOS_MAX_PRNT_STR_SIZE   256         /**< Max. size of the debug/error string of debug function */
#define VOS_MAX_FRMT_SIZE       64          /**< Max. size of the 'format' part */
#define VOS_MAX_ERR_STR_SIZE    (VOS_MAX_PRNT_STR_SIZE - VOS_MAX_FRMT_SIZE) /**< Max. size of the error part */

/** Safe printf function */
#ifdef WIN32
    #define vos_snprintf(str, size, format, ...)              \
        _snprintf_s(str, size, _TRUNCATE, format, __VA_ARGS__)
#else
    #define vos_snprintf(str, size, level, format, args ...)  \
        snprintf(str, size, format, ## args)
#endif

/** Debug output macro without formatting options */
#define vos_print(level, string)  {if(gPDebugFunction != NULL)          \
                                   {gPDebugFunction(gRefCon,            \
                                                    (level),            \
                                                    vos_getTimeStamp(), \
                                                    (__FILE__),         \
                                                    (UINT16)(__LINE__), \
                                                    (string)); }}

/** Debug output macro with formatting options */
#ifdef WIN32
    #define vos_printf(level, format, ...)                              \
    {if (gPDebugFunction != NULL)                                       \
     {   char str[VOS_MAX_PRNT_STR_SIZE];                               \
         (void) _snprintf_s(str, sizeof(str), _TRUNCATE, format, __VA_ARGS__); \
         vos_print(level, str);                                         \
     }                                                                  \
    }
#else
    #define vos_printf(level, format, args ...)       \
    {if (gPDebugFunction != NULL)                     \
     {   char str[VOS_MAX_PRNT_STR_SIZE];             \
         (void) snprintf(str, sizeof(str), format, ## args); \
         vos_print(level, str);                       \
     }                                                \
    }
#endif


/** Alignment macros  */
#ifdef WIN32
    #define ALIGNOF(type)  __alignof(type)
/* __alignof__() is broken in GCC since 3.3! It returns 8 for the alignement of long long instaed of 4!
#elif defined(__GNUC__)
    #define ALIGNOF(type)   __alignof__(type)
*/
#else
    #define ALIGNOF(type)  ((UINT32)offsetof(struct { char c; type member; }, member))
#endif

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
