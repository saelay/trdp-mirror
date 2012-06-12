/**********************************************************************************************************************/
/**
 * @file            vos_types.h
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

#ifndef VOS_TYPES_H
#define VOS_TYPES_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stddef.h>

#ifdef WIN32
#include <windows.h>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#ifdef WIN32
#define UINT8   unsigned char
#define UINT16  unsigned short
#define UINT32  unsigned int
#define INT8    char
#define INT16   signed short
#define INT32   signed int
#define BOOL    int
#define CHAR8   char
#endif

#ifndef UINT8
    #define UINT8  uint8_t
#endif
#ifndef UINT16
    #define UINT16  uint16_t
#endif
#ifndef UINT32
    #define UINT32  uint32_t
#endif
#ifndef INT8
    #define INT8  int8_t
#endif
#ifndef INT16
    #define INT16  int16_t
#endif
#ifndef INT32
    #define INT32  int32_t
#endif
#ifndef BOOL
    #define BOOL  int
#endif
#ifndef CHAR8
    #define CHAR8  char
#endif

/*	Special handling for Windows DLLs	*/
#ifdef WIN32
    #ifdef DLL_EXPORT
        #define EXT_DECL    __declspec(dllexport)
    #elif DLL_IMPORT
        #define EXT_DECL    __declspec(dllimport)
    #else
        #define EXT_DECL
    #endif

#else

    #define EXT_DECL

#endif

#undef GNU_PACKED
#define GNU_PACKED

#if defined (__GNUC__)
   #if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 6))
      #undef GNU_PACKED
/* Assert Minimum alignment (packed) for structure elements for GNU Compiler. */
      #define GNU_PACKED  __attribute__ ((__packed__))
   #endif
#endif


/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Return codes for all VOS API functions  */
typedef enum
{
    VOS_NO_ERR          = 0,    /**< No error										*/
    VOS_PARAM_ERR       = -1,   /**< Necessary parameter missing or out of range	*/
    VOS_INIT_ERR        = -2,   /**< Call without valid initialization				*/
    VOS_NOINIT_ERR      = -3,   /**< The supplied handle/reference is not valid     */
    VOS_TIMEOUT_ERR     = -4,   /**< Timout                                         */
    VOS_NODATA_ERR      = -5,   /**< Non blocking mode: no data received			*/
    VOS_SOCK_ERR        = -6,   /**< Socket option not supported                    */
    VOS_IO_ERR          = -7,   /**< Socket IO error, data can't be received/sent	*/
    VOS_MEM_ERR         = -8,   /**< No more memory available           */
    VOS_SEMA_ERR        = -9,   /**< Semaphore not available            */
    VOS_QUEUE_ERR       = -10,  /**< Queue empty                        */
    VOS_QUEUE_FULL_ERR  = -11,  /**< Queue full                         */
    VOS_MUTEX_ERR       = -12,  /**< Mutex not available                */
    VOS_THREAD_ERR      = -13,  /**< Thread creation error			    */
    VOS_UNKNOWN_ERR     = -99   /**< Unknown error                      */
} VOS_ERR_T;

/** Categories for logging	*/
typedef enum
{
    VOS_LOG_ERROR   = 0,        /**< This is a critical error				*/
    VOS_LOG_WARNING = 1,        /**< This is a warning						*/
    VOS_LOG_INFO    = 2,        /**< This is an info						*/
    VOS_LOG_DBG     = 3,        /**< This is a debug info					*/
} VOS_LOG_T;

#ifdef _UUID_T
typedef uuid_t VOS_UUID_T;      /**< universal unique identifier according to RFC 4122, time based version */
#else
typedef UINT8 VOS_UUID_T[16];   /**< universal unique identifier according to RFC 4122, time based version */
#endif

/**	Timer value compatible with timeval / select.
 * Relative or absolute date, depending on usage
 */
typedef struct
{
    UINT32  tv_sec;         /**< full seconds                                   */
    UINT32  tv_usec;        /**< Micro seconds (max. value 999999)				*/
} VOS_TIME_T;

/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/**	Function definition for error/debug output.
 *  The function will be called for logging and error message output. The user can decide, what kind of info will
 *	be logged by filtering the category.
 *
 *  @param[in]	    *pRefCon    pointer to user context
 *  @param[in]		category	Log category (Error, Warning, Info etc.)
 *  @param[in]		pTime		pointer to NULL-terminated string of time stamp
 *  @param[in]		pFile		pointer to NULL-terminated string of source module
 *  @param[in]		LineNumber	Line number
 *  @param[in]		pMsgStr     pointer to NULL-terminated string
 *
 *  @retval         none
 *
 */

typedef void (*VOS_PRINT_DBG_T)(
    void        *pRefCon,
    VOS_LOG_T   category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr);

/**********************************************************************************************************************/
/** Initialize the vos library.
 *  This is used to set the output function for all VOS error and debug output.
 *
 *  @param[in]	    *pRefCon            user context
 *  @param[in]	    *pDebugOutput       pointer to debug output function
 *  @retval         VOS_NO_ERR			no error
 *  @retval         VOS_INIT_ERR		unsupported
 */

EXT_DECL VOS_ERR_T vos_init (
    void            *pRefCon,
    VOS_PRINT_DBG_T pDebugOutput);


#ifdef __cplusplus
}
#endif

#endif /* VOS_TYPES_H */
