/**********************************************************************************************************************/
/**
 * @file            windows/vos_thread.c
 *
 * @brief           Multitasking functions
 *
 * @details         OS abstraction of thread-handling functions
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 *
 * @remarks         All rights reserved. Reproduction, modification, use or disclosure
 *                  to third parties without express authority is forbidden,
 *                  Copyright Bombardier Transportation GmbH, Germany, 2013.
 *                  vos_thread.c uses pthreads-w32 (http://sourceware.org/pthreads-win32/) under LGPL license
 *
 *
 * $Id$
 *
 */

#ifndef WIN32
#error \
    "You are trying to compile the WIN32 implementation of vos_thread.c - either define WIN32 or exclude this file!"
#endif

#define NSECS_PER_USEC  1000
#define USECS_PER_MSEC  1000
#define MSECS_PER_SEC   1000

/***********************************************************************************************************************
 * INCLUDES
 */
#include <errno.h>
#include <sys/timeb.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#include "vos_thread.h"
#include "vos_sock.h"
#include "vos_mem.h"
#include "vos_utils.h"
#include "vos_private.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

const size_t    cDefaultStackSize   = 16 * 1024;
const UINT32    cMutextMagic        = 0x1234FEDC;

UINT16          uuidCycle = 0;
BOOL            vosTreadInitialised = FALSE;
pthread_t       threadHandle[VOS_MAX_THREAD_CNT];

/***********************************************************************************************************************
 *  LOCALS
 */

/**********************************************************************************************************************/
/** Cyclic thread functions.
 *  Wrapper for cyclic threads. The thread function will be called cyclically with interval.
 *
 *  @param[in]      interval        Interval for cyclic threads in us (optional)
 *  @param[in]      pFunction       Pointer to the thread function
 *  @param[in]      pArguments      Pointer to the thread function parameters
 *  @retval         void
 */

void cyclicThread (
    UINT32              interval,
    VOS_THREAD_FUNC_T   pFunction,
    void                *pArguments)
{
    for (;; )
    {
        (void) vos_threadDelay(interval);
        pFunction(pArguments);
        pthread_testcancel();
    }
}


/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */


/**********************************************************************************************************************/
/* Threads
                                                                                                           */
/**********************************************************************************************************************/


/**********************************************************************************************************************/
/** Initialize the thread library.
 *  Must be called once before any other call
 *
 *  @retval         VOS_NO_ERR             no error
 *  @retval         VOS_INIT_ERR           threading not supported
 */

EXT_DECL VOS_ERR_T vos_threadInit (
    void)
{
    memset(threadHandle, 0, sizeof(threadHandle));
    vosTreadInitialised = TRUE;

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Search a free Handle place in the thread handle list.
 *
 *  @retval         pointer to a free thread handle or NULL if not available
 */

pthread_t *vos_getFreeThreadHandle (void)
{
    pthread_t *pHandle = NULL;

    if (vosTreadInitialised)
    {
        UINT32 i;

        for (i = 0; i < sizeof(threadHandle) / sizeof(pthread_t); i++)
        {
            if (threadHandle[i].p == NULL)
            {
                return (&threadHandle[i]);
            }
        }
    }

    return pHandle;
}


/**********************************************************************************************************************/
/** Create a thread.
 *  Create a thread and return a thread handle for further requests. Not each parameter may be supported by all
 *  target systems!
 *
 *  @param[out]     pThread         Pointer to returned thread handle
 *  @param[in]      pName           Pointer to name of the thread (optional)
 *  @param[in]      policy          Scheduling policy (FIFO, Round Robin or other)
 *  @param[in]      priority        Scheduling priority (1...255 (highest), default 0)
 *  @param[in]      interval        Interval for cyclic threads in us (optional)
 *  @param[in]      stackSize       Minimum stacksize, default 0: 16kB
 *  @param[in]      pFunction       Pointer to the thread function
 *  @param[in]      pArguments      Pointer to the thread function parameters
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_NOINIT_ERR  invalid handle
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_THREAD_ERR  thread creation error
 *  @retval         VOS_INIT_ERR    no threads available
 */

EXT_DECL VOS_ERR_T vos_threadCreate (
    VOS_THREAD_T            *pThread,
    const CHAR8             *pName,
    VOS_THREAD_POLICY_T     policy,
    VOS_THREAD_PRIORITY_T   priority,
    UINT32                  interval,
    UINT32                  stackSize,
    VOS_THREAD_FUNC_T       pFunction,
    void                    *pArguments)
{
    pthread_t           *pThreadHandle;
    pthread_attr_t      threadAttrib;
    struct sched_param  schedParam;  /* scheduling priority */
    int retCode;

    if (!vosTreadInitialised)
    {
        return VOS_INIT_ERR;
    }

    if ((pThreadHandle = vos_getFreeThreadHandle()) == NULL)
    {
        return VOS_THREAD_ERR;
    }

    if (interval > 0)
    {
        vos_printLog(VOS_LOG_ERROR,
                   "%s cyclic threads not implemented yet\n",
                   pName);
        return VOS_INIT_ERR;
    }

    /* Initialize thread attributes to default values */
    retCode = pthread_attr_init(&threadAttrib);
    if (retCode != 0)
    {
        vos_printLog(VOS_LOG_ERROR,
                   "%s pthread_attr_init() failed (Err:%d)\n",
                   pName,
                   retCode );
        return VOS_THREAD_ERR;
    }

    /* Set the stack size */
    if (stackSize > 0)
    {
        retCode = pthread_attr_setstacksize(&threadAttrib, (size_t) stackSize);
    }
    else
    {
        retCode = pthread_attr_setstacksize(&threadAttrib, cDefaultStackSize);
    }

    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_ERROR,
            "%s pthread_attr_setstacksize() failed (Err:%d)\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    /* Detached thread */
    retCode = pthread_attr_setdetachstate(&threadAttrib,
                                          PTHREAD_CREATE_DETACHED);
    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_ERROR,
            "%s pthread_attr_setdetachstate() failed (Err:%d)\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    /* Set the policy of the thread */
    retCode = pthread_attr_setschedpolicy(&threadAttrib, (int) policy);
    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_ERROR,
            "%s pthread_attr_setschedpolicy() failed (Err:%d)\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    /* Set the scheduling priority of the thread */
    schedParam.sched_priority = priority;
    retCode = pthread_attr_setschedparam(&threadAttrib, &schedParam);
    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_ERROR,
            "%s pthread_attr_setschedparam() failed (Err:%d)\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    /* Set inheritsched attribute of the thread */
    retCode = pthread_attr_setinheritsched(&threadAttrib,
                                           PTHREAD_EXPLICIT_SCHED);
    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_ERROR,
            "%s pthread_attr_setinheritsched() failed (Err:%d)\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    /* Create the thread */
    retCode = pthread_create( pThreadHandle,
                              &threadAttrib,
                              (void *(*)(void *))pFunction,
                              pArguments);
    if (retCode != 0)
    {
        vos_printLog(VOS_LOG_ERROR,
                   "%s pthread_create() failed (Err:%d)\n",
                   pName,
                   retCode );
        return VOS_THREAD_ERR;
    }

    *pThread = (VOS_THREAD_T) pThreadHandle;

    /* Destroy thread attributes */
    retCode = pthread_attr_destroy(&threadAttrib);
    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_ERROR,
            "%s pthread_attr_destroy() failed (Err:%d)\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Terminate a thread.
 *  This call will terminate the thread with the given threadId and release all resources. Depending on the
 *  underlying architectures, it may just block until the thread ran out.
 *
 *  @param[in]      thread          Thread handle (or NULL if current thread)
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_THREAD_ERR  cancel failed
 */

EXT_DECL VOS_ERR_T vos_threadTerminate (
    VOS_THREAD_T thread)
{
    int retCode;

    if (!vosTreadInitialised)
    {
        return VOS_INIT_ERR;
    }

    retCode = pthread_cancel(*(pthread_t *)thread);
    if (retCode != 0)
    {
        vos_printLog(VOS_LOG_ERROR,
                   "pthread_cancel() failed (Err:%d)\n",
                   retCode );
        return VOS_THREAD_ERR;
    }
    else
    {
        memset(threadHandle, 0, sizeof(pthread_t));
    }

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Is the thread still active?
 *  This call will return VOS_NO_ERR if the thread is still active, VOS_PARAM_ERR in case it ran out.
 *
 *  @param[in]      thread          Thread handle
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_threadIsActive (
    VOS_THREAD_T thread)
{
    int retValue;
    int policy;
    struct sched_param param;

    if (!vosTreadInitialised)
    {
        return VOS_INIT_ERR;
    }

    retValue = pthread_getschedparam(*(pthread_t *)thread, &policy, &param);

    return (retValue == 0 ? VOS_NO_ERR : VOS_PARAM_ERR);
}



/**********************************************************************************************************************/
/*  Timers                                                                                                            */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Delay the execution of the current thread by the given delay in us.
 *
 *
 *  @param[in]      delay           Delay in us
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_threadDelay (
    UINT32 delay)
{
    struct timespec timespec_delay;

    timespec_delay.tv_sec   = delay / 1000000;
    timespec_delay.tv_nsec  = (delay % 1000000) * 1000;

    if (pthread_delay_np(&timespec_delay) != 0)
    {
        return VOS_PARAM_ERR;
    }

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Return the current time in sec and us
 *
 *
 *  @param[out]     pTime           Pointer to time value
 */

EXT_DECL void vos_getTime (
    VOS_TIME_T *pTime)
{
    struct __timeb32 curTime;

    if (pTime == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        if (_ftime32_s( &curTime ) == 0)
        {
            pTime->tv_sec   = curTime.time;
            pTime->tv_usec  = curTime.millitm * 1000;
        }
        else
        {
            pTime->tv_sec   = 0;
            pTime->tv_usec  = 0;
        }
    }
}

/**********************************************************************************************************************/
/** Get a time-stamp string.
 *  Get a time-stamp string for debugging in the form "yyyymmdd-hh:mm:ss.ms"
 *  Depending on the used OS / hardware the time might not be a real-time stamp but relative from start of system.
 *
 *  @retval         timestamp   "yyyymmdd-hh:mm:ss.ms"
 */

EXT_DECL const CHAR8 *vos_getTimeStamp (void)
{
    static char         timeString[32];
    struct __timeb32    curTime;
    struct tm           curTimeTM;

    memset(timeString, 0, sizeof(timeString));

    if (_ftime32_s( &curTime ) == 0)
    {
        if (_localtime32_s(&curTimeTM, &curTime.time) == 0)
        {
            (void) sprintf_s(timeString,
                             sizeof(timeString),
                             "%04d%02d%02d-%02d:%02d:%02d.%03hd ",
                             1900 + curTimeTM.tm_year /* offset in years from 1900 */,
                             1 + curTimeTM.tm_mon /* january is zero */,
                             curTimeTM.tm_mday,
                             curTimeTM.tm_hour,
                             curTimeTM.tm_min,
                             curTimeTM.tm_sec,
                             curTime.millitm);
        }
    }

    return timeString;
}


/**********************************************************************************************************************/
/** Clear the time stamp
 *
 *
 *  @param[out]     pTime           Pointer to time value
 */

EXT_DECL void vos_clearTime (
    VOS_TIME_T *pTime)
{
    if (pTime == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        timerclear(pTime);
    }
}

/**********************************************************************************************************************/
/** Add the second to the first time stamp, return sum in first
 *
 *
 *  @param[in, out]     pTime           Pointer to time value
 *  @param[in]          pAdd            Pointer to time value
 */

EXT_DECL void vos_addTime (
    VOS_TIME_T          *pTime,
    const VOS_TIME_T    *pAdd)
{
    if (pTime == NULL || pAdd == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        pTime->tv_sec   += pAdd->tv_sec;
        pTime->tv_usec  += pAdd->tv_usec;

        while (pTime->tv_usec >= 1000000)
        {
            pTime->tv_sec++;
            pTime->tv_usec -= 1000000;
        }
    }
}

/**********************************************************************************************************************/
/** Subtract the second from the first time stamp, return diff in first
 *
 *
 *  @param[in, out]     pTime           Pointer to time value
 *  @param[in]          pSub            Pointer to time value
 */

EXT_DECL void vos_subTime (
    VOS_TIME_T          *pTime,
    const VOS_TIME_T    *pSub)
{
    if (pTime == NULL || pSub == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        /* handle carry over:
         * when the usec are too large at pSub, move one second from tv_sec to tv_usec */
        if (pSub->tv_usec > pTime->tv_usec)
        {
            pTime->tv_sec--;
            pTime->tv_usec += 1000000;
        }

        pTime->tv_usec  = pTime->tv_usec - pSub->tv_usec;
        pTime->tv_sec   = pTime->tv_sec - pSub->tv_sec;
    }
}

/**********************************************************************************************************************/
/** Divide the first time value by the second, return quotient in first
 *
 *
 *  @param[in, out]     pTime           Pointer to time value
 *  @param[in]          divisor         Divisor
 */

EXT_DECL void vos_divTime (
    VOS_TIME_T  *pTime,
    UINT32      divisor)
{
    if (pTime == NULL || divisor == 0)
    {
        vos_printLog(VOS_LOG_ERROR, "ERROR NULL pointer/parameter\n");
    }
    else
    {
        UINT32 temp;

        temp = pTime->tv_sec % divisor;
        pTime->tv_sec /= divisor;
        if (temp > 0)
        {
            pTime->tv_usec += temp * 1000000;
        }
        pTime->tv_usec /= divisor;  /*lint !e573 mix of signed and unsigned */
    }
}

/**********************************************************************************************************************/
/** Multiply the first time by the second, return product in first
 *
 *
 *  @param[in, out]     pTime           Pointer to time value
 *  @param[in]          mul             Factor
 */

EXT_DECL void vos_mulTime (
    VOS_TIME_T  *pTime,
    UINT32      mul)
{
    if (pTime == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "ERROR NULL pointer/parameter\n");
    }
    else
    {
        pTime->tv_sec   *= mul;
        pTime->tv_usec  *= mul;
        while (pTime->tv_usec >= 1000000)
        {
            pTime->tv_sec++;
            pTime->tv_usec -= 1000000;
        }
    }
}

/**********************************************************************************************************************/
/** Compare the second from the first time stamp, return diff in first
 *
 *
 *  @param[in, out]     pTime           Pointer to time value
 *  @param[in]          pCmp            Pointer to time value to compare
 *  @retval             0               pTime == pCmp
 *  @retval             -1              pTime < pCmp
 *  @retval             1               pTime > pCmp
 */

EXT_DECL INT32 vos_cmpTime (
    const VOS_TIME_T    *pTime,
    const VOS_TIME_T    *pCmp)
{
    if (pTime == NULL || pCmp == NULL)
    {
        return 0;
    }
    if (timercmp(pTime, pCmp, >))
    {
        return 1;
    }
    if (timercmp(pTime, pCmp, <))
    {
        return -1;
    }
    return 0;
}

/**********************************************************************************************************************/
/** Get a universal unique identifier according to RFC 4122 time based version.
 *
 *
 *  @param[out]     pUuID           Pointer to a universal unique identifier
 */

EXT_DECL void vos_getUuid (
    VOS_UUID_T pUuID)
{
    /*  Manually creating a UUID from time stamp and MAC address  */
    static UINT16   count = 1;
    VOS_TIME_T      current;
    VOS_ERR_T       ret;

    vos_getTime(&current);

    pUuID[0]    = current.tv_usec & 0xFF;
    pUuID[1]    = (current.tv_usec & 0xFF00) >> 8;
    pUuID[2]    = (current.tv_usec & 0xFF0000) >> 16;
    pUuID[3]    = (current.tv_usec & 0xFF000000) >> 24;
    pUuID[4]    = current.tv_sec & 0xFF;
    pUuID[5]    = (current.tv_sec & 0xFF00) >> 8;
    pUuID[6]    = (current.tv_sec & 0xFF0000) >> 16;
    pUuID[7]    = ((current.tv_sec & 0x0F000000) >> 24) | 0x4; /*  pseudo-random version   */

    /* we always increment these values, this definitely makes the UUID unique */
    pUuID[8]    = (UINT8) (count & 0xFF);
    pUuID[9]    = (UINT8) (count >> 8);
    count++;

    /*  Copy the mac address into the rest of the array */
    ret = vos_sockGetMAC(&pUuID[10]);
    if (ret != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_sockGetMAC() failed (Err:%d)\n", ret);
    }
}



/**********************************************************************************************************************/
/*    Mutex & Semaphores
                                                                                                 */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Create a recursive mutex.
 *  Return a mutex handle. The mutex will be available at creation.
 *
 *  @param[out]     pMutex          Pointer to mutex handle
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_PARAM_ERR   pMutex == NULL
 *  @retval         VOS_MUTEX_ERR   no mutex available
 */

EXT_DECL VOS_ERR_T vos_mutexCreate (
    VOS_MUTEX_T *pMutex)
{
    int err = 0;
    pthread_mutexattr_t attr;

    if (pMutex == NULL)
    {
        return VOS_PARAM_ERR;
    }

    *pMutex = (VOS_MUTEX_T) vos_memAlloc(sizeof (struct VOS_MUTEX));

    if (*pMutex == NULL)
    {
        return VOS_MEM_ERR;
    }

    err = pthread_mutexattr_init(&attr);
    if (err == 0)
    {
        err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        if (err == 0)
        {
            err = pthread_mutex_init(&(*pMutex)->mutexId, &attr);
        }
        (void) pthread_mutexattr_destroy(&attr);
    }

    if (err == 0)
    {
        (*pMutex)->magicNo = cMutextMagic;
    }
    else
    {
        vos_printLog(VOS_LOG_ERROR, "Can not create Mutex(pthread err=%d)\n", err);
        vos_memFree(*pMutex);
        *pMutex = NULL;
        return VOS_MUTEX_ERR;
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Create a recursive mutex.
 *  Fill in a mutex handle. The mutex storage must be already allocated.
 *
 *  @param[out]     pMutex          Pointer to mutex handle
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_PARAM_ERR   pMutex == NULL
 *  @retval         VOS_MUTEX_ERR   no mutex available
 */

VOS_ERR_T vos_mutexLocalCreate (
    struct VOS_MUTEX *pMutex)
{
    int err = 0;
    pthread_mutexattr_t attr;

    if (pMutex == NULL)
    {
        return VOS_PARAM_ERR;
    }

    err = pthread_mutexattr_init(&attr);
    if (err == 0)
    {
        err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        if (err == 0)
        {
            err = pthread_mutex_init(&pMutex->mutexId, &attr);
        }
        (void) pthread_mutexattr_destroy(&attr);
    }

    if (err == 0)
    {
        pMutex->magicNo = cMutextMagic;
    }
    else
    {
        vos_printLog(VOS_LOG_ERROR, "Can not create Mutex(pthread err=%d)\n", err);
        return VOS_MUTEX_ERR;
    }

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Delete a mutex.
 *  Release the resources taken by the mutex.
 *
 *  @param[in]      pMutex          mutex handle
 */

EXT_DECL void vos_mutexDelete (
    VOS_MUTEX_T pMutex)
{
    if (pMutex == NULL || pMutex->magicNo != cMutextMagic)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_mutexDelete() ERROR invalid parameter");
    }
    else
    {
        int err;

        err = pthread_mutex_destroy(&pMutex->mutexId);
        if (err == 0)
        {
            pMutex->magicNo = 0;
            vos_memFree(pMutex);
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR,
                       "Can not destroy Mutex (pthread err=%d)\n",
                       err);
        }
    }
}

/**********************************************************************************************************************/
/** Delete a mutex.
 *  Release the resources taken by the mutex.
 *
 *  @param[in]      pMutex          Pointer to mutex struct
 */

void vos_mutexLocalDelete (
    struct VOS_MUTEX *pMutex)
{
    if (pMutex == NULL || pMutex->magicNo != cMutextMagic)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_mutexLocalDelete() ERROR invalid parameter");
    }
    else
    {
        int err;

        err = pthread_mutex_destroy(&pMutex->mutexId);
        if (err == 0)
        {
            pMutex->magicNo = 0;
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "Can not destroy Mutex (pthread err=%d)\n", err);
        }
    }
}


/**********************************************************************************************************************/
/** Take a mutex.
 *  Wait for the mutex to become available (lock).
 *
 *  @param[in]      pMutex          mutex handle
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMutex == NULL or wrong type
 *  @retval         VOS_MUTEX_ERR   no such mutex
 */

EXT_DECL VOS_ERR_T vos_mutexLock (
    VOS_MUTEX_T pMutex)
{
    int err;

    if (pMutex == NULL || pMutex->magicNo != cMutextMagic)
    {
        return VOS_PARAM_ERR;
    }

    err = pthread_mutex_lock(&pMutex->mutexId);
    if (err != 0)
    {
        vos_printLog(VOS_LOG_ERROR,
                   "Unable to lock Mutex (pthread err=%d)\n",
                   err);
        return VOS_MUTEX_ERR;
    }

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Try to take a mutex.
 *  If mutex is can't be taken VOS_MUTEX_ERR is returned.
 *
 *  @param[in]      pMutex          mutex handle
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMutex == NULL or wrong type
 *  @retval         VOS_MUTEX_ERR   mutex not locked
 */

EXT_DECL VOS_ERR_T vos_mutexTryLock (
    VOS_MUTEX_T pMutex)
{
    int err;

    if (pMutex == NULL || pMutex->magicNo != cMutextMagic)
    {
        return VOS_PARAM_ERR;
    }

    err = pthread_mutex_trylock(&pMutex->mutexId);
    if (err == EBUSY)
    {
        return VOS_MUTEX_ERR;
    }
    if (err == EINVAL)
    {
        vos_printLog(VOS_LOG_ERROR,
                   "Unable to trylock Mutex (pthread err=%d)\n",
                   err);
        return VOS_MUTEX_ERR;
    }

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Release a mutex.
 *  Unlock the mutex.
 *
 *  @param[in]      pMutex           mutex handle
 */

EXT_DECL VOS_ERR_T vos_mutexUnlock (
    VOS_MUTEX_T pMutex)
{
    if (pMutex == NULL || pMutex->magicNo != cMutextMagic)
    {

        vos_printLog(VOS_LOG_ERROR, "vos_mutexUnlock() ERROR invalid parameter");
        return VOS_PARAM_ERR;
    }
    else
    {
        int err;

        err = pthread_mutex_unlock(&pMutex->mutexId);
        if (err != 0)
        {
            vos_printLog(VOS_LOG_ERROR,
                       "Unable to unlock Mutex (pthread err=%d)\n",
                       err);
            return VOS_MUTEX_ERR;
        }
    }
    return VOS_NO_ERR;
}



/**********************************************************************************************************************/
/** Create a semaphore.
 *  Return a semaphore handle. Depending on the initial state the semaphore will be available on creation or not.
 *
 *  @param[out]     pSema           Pointer to semaphore handle
 *  @param[in]      initialState    The initial state of the sempahore
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_SEMA_ERR    no semaphore available
 */

EXT_DECL VOS_ERR_T vos_semaCreate (
    VOS_SEMA_T          *pSema,
    VOS_SEMA_STATE_T    initialState)
{
    VOS_ERR_T   retVal  = VOS_SEMA_ERR;
    INT32       ret     = (INT32) NULL;

    /*Check parameters*/
    if (pSema == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_SemaCreate() ERROR invalid parameter pSema == NULL\n");
        retVal = VOS_PARAM_ERR;
    }
    else if ((initialState != VOS_SEMA_EMPTY) && (initialState != VOS_SEMA_FULL))
    {
        vos_printLog(VOS_LOG_ERROR, "vos_SemaCreate() ERROR invalid parameter initialState\n");
        retVal = VOS_PARAM_ERR;
    }
    else
    {
        /*Parameters are OK*/
        *pSema = (VOS_SEMA_T) vos_memAlloc(sizeof (VOS_SEMA_T));

        if (*pSema == NULL)
        {
            return VOS_MEM_ERR;
        }

        /*pThread Semaphore init*/
        ret = sem_init((sem_t *)*pSema, 0, (UINT8)initialState);
        if (ret != (int)NULL)
        {
            /*Semaphore init failed*/
            vos_printLog(VOS_LOG_ERROR, "vos_semaCreate() ERROR Semaphore could not be initialized\n");
            retVal = VOS_SEMA_ERR;
        }
        else
        {
            /*Semaphore init successful*/
            retVal = VOS_NO_ERR;
        }
    }
    return retVal;
}


/**********************************************************************************************************************/
/** Delete a semaphore.
 *  This will eventually release any processes waiting for the semaphore.
 *
 *  @param[in]      sema            semaphore handle
 */

EXT_DECL void vos_semaDelete (
    VOS_SEMA_T sema)
{
    INT32   ret = (INT32) NULL;
    INT32   err = (INT32) NULL;

    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_semaDelete() ERROR invalid parameter\n");
    }
    else
    {
        /* Check if this is a valid semaphore handle*/
        err = sem_getvalue((sem_t *)sema, &ret);
        if (err == (INT32) NULL)
        {
            ret = sem_destroy((sem_t *)sema);
            if (ret != (INT32) NULL)
            {
                /* Error destroying Semaphore */
                vos_printLog(VOS_LOG_ERROR, "vos_semaDelete() ERROR CloseHandle failed\n");
            }
            else
            {
                /* Semaphore deleted successfully, free allocated memory */
                vos_memFree(sema);
            }
        }
    }
    return;
}

/**********************************************************************************************************************/
/** Take a semaphore.
 *  Try to get (decrease) a semaphore.
 *
 *  @param[in]      sema            semaphore handle
 *  @param[in]      timeout         Max. time in us to wait, 0 means no wait
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_NOINIT_ERR  invalid handle
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_SEMA_ERR    could not get semaphore in time
 */

EXT_DECL VOS_ERR_T vos_semaTake (
    VOS_SEMA_T  sema,
    UINT32      timeout)
{
    INT32           err             = (INT32) NULL;
    VOS_ERR_T       retVal          = VOS_SEMA_ERR;
    VOS_TIME_T      waitTimeVos     = {(UINT32) NULL, (UINT32) NULL};
    struct timespec waitTimeSpec    = {(UINT32) NULL, (UINT32) NULL};

    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_semaTake() ERROR invalid parameter 'sema' == NULL\n");
        retVal = VOS_PARAM_ERR;
    }
    else if (timeout == (UINT32) NULL)
    {
        /* Take Semaphore, return ERROR if Semaphore cannot be taken immediately instead of blocking */
        err = sem_trywait((sem_t *)sema);
    }
    else if (timeout == VOS_SEMA_WAIT_FOREVER)
    {
        /* Take Semaphore, block until Semaphore becomes available */
         err = sem_wait((sem_t *)sema);
    }
    else
    {
        /* Take Semaphore with specified wait time, block until timeout expires */

        /* Get time since 01/01/1970 and convert it to timespec format */
        vos_getTime(&waitTimeVos);
        waitTimeSpec.tv_sec     = waitTimeVos.tv_sec;
        waitTimeSpec.tv_nsec    = waitTimeVos.tv_usec * NSECS_PER_USEC;

        /* add offset */
        if (timeout >= (USECS_PER_MSEC * MSECS_PER_SEC))
        {
            /* Timeout longer than 1 sec, add sec and nsec seperately */
            waitTimeSpec.tv_sec     += timeout / (USECS_PER_MSEC * MSECS_PER_SEC);
            waitTimeSpec.tv_nsec    += (timeout % (USECS_PER_MSEC * MSECS_PER_SEC)) * NSECS_PER_USEC;
        }
        else
        {
            /* Timeout shorter than 1 sec, only add nsecs */
            waitTimeSpec.tv_nsec += timeout * NSECS_PER_USEC;
        }

        /* Carry if tv_nsec > 1.000.000.000 */
        if (waitTimeSpec.tv_nsec >= NSECS_PER_USEC * USECS_PER_MSEC * MSECS_PER_SEC)
        {
            waitTimeSpec.tv_sec++;
            waitTimeSpec.tv_nsec -= NSECS_PER_USEC * USECS_PER_MSEC * MSECS_PER_SEC;
        }
        else
        {
            /* carry not necessary */
        }

        /* take semaphore with specified timeout */
        err = sem_timedwait((sem_t *)sema, &waitTimeSpec);
    }
    if (err != (INT32) NULL)
    {
        /* Could not take Semaphore in time */
        retVal = VOS_SEMA_ERR;
    }
    else
    {
        /* Semaphore take success */
        retVal = VOS_NO_ERR;
    }
    return retVal;
}


/**********************************************************************************************************************/
/** Give a semaphore.
 *  Release (increase) a semaphore.
 *
 *  @param[in]      sema            semaphore handle
 */

EXT_DECL void vos_semaGive (
    VOS_SEMA_T sema)
{
    INT32 err = (INT32) NULL;

    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_semaGive() ERROR invalid parameter 'sema' == NULL\n");
    }
    else
    {
        /* release semaphore */
        err = sem_post((sem_t *)sema);
        if (err == (INT32) NULL)
        {
            /* Semaphore released */
        }
        else
        {
            /* Could not release Semaphore */
            vos_printLog(VOS_LOG_ERROR, "vos_semaGive() ERROR could not release semaphore\n");
        }
    }
    return;
}
