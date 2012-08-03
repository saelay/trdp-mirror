/**********************************************************************************************************************/
/**
 * @file            vos_thread.c
 *
 * @brief           Multitasking functions
 *
 * @details			OS abstraction of thread-handling functions
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 *
 * @remarks         All rights reserved. Reproduction, modification, use or disclosure
 *                  to third parties without express authority is forbidden,
 *                  Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
 */

#ifndef POSIX
#error \
    "You are trying to compile the POSIX implementation of vos_thread.c - either define POSIX or exclude this file!"
#endif

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

#ifdef __APPLE__
#include <uuid/uuid.h>
#else
#include "vos_sock.h"
#endif

#include "vos_types.h"
#include "vos_thread.h"
#include "vos_mem.h"
#include "vos_utils.h"


/***********************************************************************************************************************
 * DEFINITIONS
 */

const size_t    cDefaultStackSize   = 16 * 1024;
const uint32_t  cMutextMagic        = 0x1234FEDC;

BOOL            vosThreadInitialised = FALSE;

struct VOS_MUTEX_T
{
    UINT32          magicNo;
    pthread_mutex_t mutexId;
};

/***********************************************************************************************************************
 *  LOCALS
 */

/**********************************************************************************************************************/
/** Cyclic thread functions.
 *  Wrapper for cyclic threads. The thread function will be called cyclically with interval.
 *
 *  @param[in]      interval		Interval for cyclic threads in us (optional)
 *  @param[in]      pFunction	    Pointer to the thread function
 *  @param[in]      pArguments		Pointer to the thread function parameters
 *  @retval         void
 */

void cyclicThread (
    UINT32              interval,
    VOS_THREAD_FUNC_T   pFunction,
    void                *pArguments)
{
    for(;; )
    {
        vos_threadDelay(interval);
        pFunction(pArguments);
        pthread_testcancel();
    }
}


/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */


/**********************************************************************************************************************/
/*	Threads																										      */
/**********************************************************************************************************************/


/**********************************************************************************************************************/
/** Initialize the thread library.
 *  Must be called once before any other call
 *
 *  @retval         VOS_NO_ERR			no error
 *  @retval         VOS_INIT_ERR		threading not supported
 */

EXT_DECL VOS_ERR_T vos_threadInit (
    void)
{
    vosThreadInitialised = TRUE;

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Create a thread.
 *  Create a thread and return a thread handle for further requests. Not each parameter may be supported by all
 *  target systems!
 *
 *  @param[out]     pThread         Pointer to returned thread handle
 *  @param[in]      pName			Pointer to name of the thread (optional)
 *  @param[in]      policy			Scheduling policy (FIFO, Round Robin or other)
 *  @param[in]      priority		Scheduling priority (1...255 (highest), default 0)
 *  @param[in]      interval		Interval for cyclic threads in us (optional)
 *  @param[in]      stackSize		Minimum stacksize, default 0: 16kB
 *  @param[in]      pFunction	    Pointer to the thread function
 *  @param[in]      pArguments		Pointer to the thread function parameters
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_NOINIT_ERR	invalid handle
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 *  @retval         VOS_THREAD_ERR	thread creation error
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
    pthread_t           hThread;
    pthread_attr_t      threadAttrib;
    struct sched_param  schedParam;  /* scheduling priority */
    int retCode;

    if (!vosThreadInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (interval > 0)
    {
        vos_printf(VOS_LOG_ERROR,
                   "%s vos_threadCreate cyclic threads not implemented yet\n",
                   pName);
        return VOS_INIT_ERR;
    }

    /* Initialize thread attributes to default values */
    retCode = pthread_attr_init(&threadAttrib);
    if (retCode != 0)
    {
        vos_printf(VOS_LOG_ERROR,
                   "%s vos_threadCreate pthread_attr_init failed return=%d\n",
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
        vos_printf(
            VOS_LOG_ERROR,
            "%s vos_threadCreate pthread_attr_setstacksize failed return=%d\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    /* Detached thread */
    retCode = pthread_attr_setdetachstate(&threadAttrib,
                                          PTHREAD_CREATE_DETACHED);
    if (retCode != 0)
    {
        vos_printf(
            VOS_LOG_ERROR,
            "%s vos_threadCreate pthread_attr_setdetachstate failed return=%d\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    /* Set the policy of the thread */
    retCode = pthread_attr_setschedpolicy(&threadAttrib, policy);
    if (retCode != 0)
    {
        vos_printf(
            VOS_LOG_ERROR,
            "%s vos_threadCreate pthread_attr_setschedpolicy failed return=%d\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    /* Set the scheduling priority of the thread */
    schedParam.sched_priority = priority;
    retCode = pthread_attr_setschedparam(&threadAttrib, &schedParam);
    if (retCode != 0)
    {
        vos_printf(
            VOS_LOG_ERROR,
            "%s vos_threadCreate pthread_attr_setschedparam failed return=%d\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    /* Set inheritsched attribute of the thread */
    retCode = pthread_attr_setinheritsched(&threadAttrib,
                                           PTHREAD_EXPLICIT_SCHED);
    if (retCode != 0)
    {
        vos_printf(
            VOS_LOG_ERROR,
            "%s vos_threadCreate pthread_attr_setinheritsched failed return=%d\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    /* Create the thread */
    retCode = pthread_create(&hThread, &threadAttrib, (void *(*)(
                                                           void *))pFunction,
                             pArguments);
    if (retCode != 0)
    {
        vos_printf(VOS_LOG_ERROR,
                   "%s vos_threadCreate pthread_create failed return=%d\n",
                   pName,
                   retCode );
        return VOS_THREAD_ERR;
    }

    *pThread = (VOS_THREAD_T) hThread;

    /* Destroy thread attributes */
    retCode = pthread_attr_destroy(&threadAttrib);
    if (retCode != 0)
    {
        vos_printf(
            VOS_LOG_ERROR,
            "%s vos_threadCreate pthread_attr_destroy failed return=%d\n",
            pName,
            retCode );
        return VOS_THREAD_ERR;
    }

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Terminate a thread.
 *  This call will terminate the thread with the given threadId and release all resources. Depending on the
 *	underlying architectures, it may just block until the thread ran out.
 *
 *  @param[in]      thread          Thread handle (or NULL if current thread)
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_THREAD_ERR	cancel failed
 */

EXT_DECL VOS_ERR_T vos_threadTerminate (
    VOS_THREAD_T thread)
{
    int retCode;

    retCode = pthread_cancel((pthread_t)thread);
    if (retCode != 0)
    {
        vos_printf(VOS_LOG_ERROR,
                   "vos_threadTerminate pthread_cancel failed return=%d\n",
                   retCode );
        return VOS_THREAD_ERR;
    }
    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Is the thread still active?
 *  This call will return VOS_NO_ERR if the thread is still active, VOS_PARAM_ERR in case it ran out.
 *
 *  @param[in]      thread          Thread handle
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_threadIsActive (
    VOS_THREAD_T thread)
{
    int retValue;
    int policy;
    struct sched_param param;

    retValue = pthread_getschedparam((pthread_t)thread, &policy, &param);

    return (retValue == 0 ? VOS_NO_ERR : VOS_PARAM_ERR);
}



/**********************************************************************************************************************/
/*	Timers																										      */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Delay the execution of the current thread by the given delay in us.
 *
 *
 *  @param[in]      delay           Delay in us
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_threadDelay (
    UINT32 delay)
{
    struct timespec wanted_delay;
    struct timespec remaining_delay;
    int ret;

    if (delay == 0)
    {
        /*	yield cpu to other processes   */
        if (sched_yield() != 0)
        {
            return VOS_PARAM_ERR;
        }
        return VOS_NO_ERR;
    }

    wanted_delay.tv_sec     = delay / 1000000;
    wanted_delay.tv_nsec    = (delay % 1000000) * 1000;
    do
    {
        ret = nanosleep(&wanted_delay, &remaining_delay);
        if (ret == -1 && errno == EINTR)
        {
            wanted_delay = remaining_delay;
        }
    }
    while (errno == EINTR);

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Return the current time in sec and us
 *
 *
 *  @param[out]     pTime			Pointer to time value
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_getTime (
    VOS_TIME_T *pTime)
{
    struct timeval myTime;

    if (pTime == NULL)
    {
        return VOS_PARAM_ERR;
    }

#ifndef _POSIX_TIMERS

    /*	On systems without monotonic clock support,
        changing the system clock during operation
        might interrupt process data packet transmissions!	*/

    gettimeofday(&myTime, NULL);

#else

    struct timespec currentTime;

    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    TIMESPEC_TO_TIMEVAL(&myTime, &currentTime);

#endif

    pTime->tv_sec   = myTime.tv_sec;
    pTime->tv_usec  = myTime.tv_usec;

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Get a time-stamp string.
 *	Get a time-stamp string for debugging in the form "yyyymmdd-hh:mm:ss.ms"
 *	Depending on the used OS / hardware the time might not be a real-time stamp but relative from start of system.
 *
 *  @retval         timestamp		"yyyymmdd-hh:mm:ss.ms"
 */

EXT_DECL const CHAR8 *vos_getTimeStamp (void)
{
    static char     pTimeString[32];
    struct timeval  curTime;
    struct tm       *curTimeTM;

    gettimeofday(&curTime, NULL);
    curTimeTM = localtime(&curTime.tv_sec);
    sprintf(pTimeString, "%04d%02d%02d-%02d:%02d:%02d.%03ld ",
            curTimeTM->tm_year,
            curTimeTM->tm_mon,
            curTimeTM->tm_mday,
            curTimeTM->tm_hour,
            curTimeTM->tm_min,
            curTimeTM->tm_sec,
            (long) curTime.tv_usec / 1000L);

    return pTimeString;
}


/**********************************************************************************************************************/
/** Clear the time stamp
 *
 *
 *  @param[out]     pTime			Pointer to time value
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	parameter must not be NULL
 */

EXT_DECL VOS_ERR_T vos_clearTime (
    VOS_TIME_T *pTime)
{
    if (pTime == NULL)
    {
        return VOS_PARAM_ERR;
    }

    timerclear(pTime);

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Add the second to the first time stamp, return sum in first
 *
 *
 *  @param[in, out]     pTime			Pointer to time value
 *  @param[in]          pAdd			Pointer to time value
 *  @retval             VOS_NO_ERR		no error
 *  @retval             VOS_PARAM_ERR	parameter must not be NULL
 */

EXT_DECL VOS_ERR_T vos_addTime (
    VOS_TIME_T          *pTime,
    const VOS_TIME_T    *pAdd)
{
    VOS_TIME_T ltime;

    if (pTime == NULL || pAdd == NULL)
    {
        return VOS_PARAM_ERR;
    }
    timeradd(pTime, pAdd, &ltime);
    *pTime = ltime;
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Subtract the second from the first time stamp, return diff in first
 *
 *
 *  @param[in, out]     pTime			Pointer to time value
 *  @param[in]          pSub			Pointer to time value
 *  @retval             VOS_NO_ERR		no error
 *  @retval             VOS_PARAM_ERR	parameter must not be NULL
 */

EXT_DECL VOS_ERR_T vos_subTime (
    VOS_TIME_T          *pTime,
    const VOS_TIME_T    *pSub)
{
    VOS_TIME_T ltime;

    if (pTime == NULL || pSub == NULL)
    {
        return VOS_PARAM_ERR;
    }
    timersub(pTime, pSub, &ltime);
    *pTime = ltime;
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Compare the second from the first time stamp, return diff in first
 *
 *
 *  @param[in, out]     pTime			Pointer to time value
 *  @param[in]          pCmp			Pointer to time value to compare
 *  @retval             0				pTime == pCmp
 *  @retval             -1				pTime < pCmp
 *  @retval             1				pTime > pCmp
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
 *  @param[out]     pUuID			Pointer to a universal unique identifier
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_UNKNOWN_ERR	Could not create UUID
 */

EXT_DECL VOS_ERR_T vos_getUuid (
    VOS_UUID_T pUuID)
{
#ifdef __APPLE__
    uuid_generate_time(pUuID);
#else
    /*	Manually creating a UUID from time stamp and MAC address	*/
    static UINT16   count = 1;
    VOS_TIME_T      current;
    
    vos_getTime(&current);

    pUuID[0]    = current.tv_usec & 0xFF;
    pUuID[1]    = (current.tv_usec & 0xFF00) >> 8;
    pUuID[2]    = (current.tv_usec & 0xFF0000) >> 16;
    pUuID[3]    = (current.tv_usec & 0xFF000000) >> 24;
    pUuID[4]    = current.tv_sec & 0xFF;
    pUuID[5]    = (current.tv_sec & 0xFF00) >> 8;
    pUuID[6]    = (current.tv_sec & 0xFF0000) >> 16;
    pUuID[7]    = ((current.tv_sec & 0x0F000000) >> 24) | 0x4; /*	pseudo-random version	*/

    /* we always increment these values, this definitely makes the UUID unique */
    pUuID[8]    = (UINT8) (count & 0xFF);
    pUuID[9]    = (UINT8) (count >> 8);
    count++;

    /*	Copy the mac address into the rest of the array	*/
    if (vos_sockGetMAC(&pUuID[10]) != VOS_NO_ERR)
    {
        return VOS_UNKNOWN_ERR;
    }
#endif

    return VOS_NO_ERR;
}



/**********************************************************************************************************************/
/*	Mutex & Semaphores																								  */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Create a recursive mutex.
 *  Return a mutex handle. The mutex will be available at creation.
 *
 *  @param[out]     pMutex			Pointer to mutex handle
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_PARAM_ERR	pMutex == NULL
 *  @retval         VOS_MUTEX_ERR	no mutex available
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

    *pMutex = (VOS_MUTEX_T) vos_memAlloc(sizeof (struct VOS_MUTEX_T));

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
        pthread_mutexattr_destroy(&attr);
    }

    if (err == 0)
    {
        (*pMutex)->magicNo = cMutextMagic;
    }
    else
    {
        vos_printf(VOS_LOG_ERROR, "Can not create Mutex(pthread err=%d)\n", err);
        vos_memFree(*pMutex);
        *pMutex = NULL;
        return VOS_MUTEX_ERR;
    }

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Delete a mutex.
 *  Release the resources taken by the mutex.
 *
 *  @param[in]      mutex			mutex handle
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	pMutex == NULL or wrong type
 *  @retval         VOS_MUTEX_ERR	no such mutex
 */

EXT_DECL VOS_ERR_T vos_mutexDelete (
    VOS_MUTEX_T mutex)
{
    int err;

    if (mutex == NULL || mutex->magicNo != cMutextMagic)
    {
        return VOS_PARAM_ERR;
    }

    err = pthread_mutex_destroy(&mutex->mutexId);
    if (err == 0)
    {
        vos_memFree(mutex);
        mutex->magicNo = 0;
    }
    else
    {
        vos_printf(VOS_LOG_ERROR,
                   "Can not destroy Mutex (pthread err=%d)\n",
                   err);
        return VOS_MUTEX_ERR;
    }

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Take a mutex.
 *  Wait for the mutex to become available (lock).
 *
 *  @param[in]      mutex			mutex handle
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	pMutex == NULL or wrong type
 *  @retval         VOS_MUTEX_ERR	no such mutex
 */

EXT_DECL VOS_ERR_T vos_mutexLock (
    VOS_MUTEX_T mutex)
{
    int err;

    if (mutex == NULL || mutex->magicNo != cMutextMagic)
    {
        return VOS_PARAM_ERR;
    }

    err = pthread_mutex_lock(&mutex->mutexId);
    if (err != 0)
    {
        vos_printf(VOS_LOG_ERROR,
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
 *  @param[in]      mutex			mutex handle
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	pMutex == NULL or wrong type
 *  @retval         VOS_MUTEX_ERR	mutex not locked
 */

EXT_DECL VOS_ERR_T vos_mutexTryLock (
    VOS_MUTEX_T mutex)
{
    int err;

    if (mutex == NULL || mutex->magicNo != cMutextMagic)
    {
        return VOS_PARAM_ERR;
    }

    err = pthread_mutex_trylock(&mutex->mutexId);
    if (err == EBUSY)
    {
        return VOS_MUTEX_ERR;
    }
    if (err == EINVAL)
    {
        vos_printf(VOS_LOG_ERROR,
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
 *  @param[in]      mutex			mutex handle
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	pMutex == NULL or wrong type
 *  @retval         VOS_MUTEX_ERR	no such mutex
 */

EXT_DECL VOS_ERR_T vos_mutexUnlock (
    VOS_MUTEX_T mutex)
{
    int err;

    if (mutex == NULL || mutex->magicNo != cMutextMagic)
    {
        return VOS_PARAM_ERR;
    }

    err = pthread_mutex_unlock(&mutex->mutexId);
    if (err != 0)
    {
        vos_printf(VOS_LOG_ERROR,
                   "Unable to unlock Mutex (pthread err=%d)\n",
                   err);
        return VOS_MUTEX_ERR;
    }

    return VOS_NO_ERR;
}



/**********************************************************************************************************************/
/** Create a semaphore.
 *  Return a semaphore handle. Depending on the initial state the semaphore will be available on creation or not.
 *
 *  @param[out]     pSema           Pointer to semaphore handle
 *  @param[in]      initialState	The initial state of the sempahore
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 *  @retval         VOS_SEMA_ERR	no semaphore available
 */

EXT_DECL VOS_ERR_T vos_semaCreate (
    VOS_SEMA_T          *pSema,
    VOS_SEMA_STATE_T    initialState)
{
    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Delete a semaphore.
 *  This will eventually release any processes waiting for the semaphore.
 *
 *  @param[in]      sema            semaphore handle
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_NOINIT_ERR	invalid handle
 */

EXT_DECL VOS_ERR_T vos_semaDelete (
    VOS_SEMA_T sema)
{
    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Take a semaphore.
 *  Try to get (decrease) a semaphore.
 *
 *  @param[in]      sema            semaphore handle
 *  @param[in]      timeout         Max. time in us to wait, 0 means forever
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_NOINIT_ERR	invalid handle
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 *  @retval         VOS_SEMA_ERR	could not get semaphore in time
 */

EXT_DECL VOS_ERR_T vos_semaTake (
    VOS_SEMA_T  sema,
    UINT32      timeout)
{
    return VOS_NO_ERR;
}



/**********************************************************************************************************************/
/** Give a semaphore.
 *  Release (increase) a semaphore.
 *
 *  @param[in]      sema            semaphore handle
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_NOINIT_ERR	invalid handle
 *  @retval         VOS_SEM_ERR		could not release semaphore
 */

EXT_DECL VOS_ERR_T vos_semaGive (
    VOS_SEMA_T sema)
{
    return VOS_NO_ERR;
}
