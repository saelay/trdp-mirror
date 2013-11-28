/**********************************************************************************************************************/
/**
 * @file            vtest.c
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

#include "vtest.h"

MEM_ERR_T L3_test_mem_init()
{
    TRDP_MEM_CONFIG_T   dynamicConfig = {NULL, RESERVED_MEMORY, {0}};
    TRDP_ERR_T res = TRDP_NO_ERR;
    MEM_ERR_T retVal = MEM_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[MEM_INIT] start...\n");
    res = vos_memInit(dynamicConfig.p,dynamicConfig.size,dynamicConfig.prealloc);
    if (res != 0)
    {
        retVal = MEM_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[MEM_INIT] vos_memInit() error\n");
    }
    printOut(OUTPUT_ADVANCED,"[MEM_INIT] finished with errcnt = %i\n",res);
    return retVal;
}

MEM_ERR_T L3_test_mem_alloc()
{
    UINT32 *ptr = NULL;
    TRDP_MEM_CONFIG_T   memConfig = {NULL, RESERVED_MEMORY, {0}};
    MEM_ERR_T retVal = MEM_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[MEM_ALLOC] start...\n");
    if (vos_memInit(memConfig.p,memConfig.size,memConfig.prealloc) != VOS_NO_ERR)
    {
        retVal = MEM_ALLOC_ERR;
        printOut(OUTPUT_ADVANCED,"[MEM_ALLOC] vos_memInit() error\n");
    }
    ptr = (UINT32*)vos_memAlloc(4);
    if (ptr == NULL)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_ALLOC] vos_memAlloc() error\n");
        retVal = MEM_ALLOC_ERR;
    }
    vos_memFree(ptr); /* undo mem_alloc */
    printOut(OUTPUT_ADVANCED,"[MEM_ALLOC] finished\n");
    return retVal;
}

MEM_ERR_T L3_test_mem_queue()
{
    VOS_QUEUE_T qHandle;
    MEM_ERR_T retVal = MEM_NO_ERR;
    VOS_ERR_T res = VOS_NO_ERR;
    UINT8 *pData;
    UINT32 size;
    VOS_TIME_T timeout = {0,20000};
    VOS_TIME_T startTime, endTime;

    printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] start...\n");
    res = vos_queueCreate(VOS_QUEUE_POLICY_FIFO,3,&qHandle);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] vos_queueCreate() ERROR: ret: %i, Handle: %x\n",res,(unsigned int)&qHandle);
        retVal = MEM_QUEUE_ERR;
    }
    res = vos_queueSend(qHandle,(UINT8*)0x0123,0x12);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] 1.queueSend() ERROR\n");
        retVal = MEM_QUEUE_ERR;
    }
    res = vos_queueSend(qHandle,(UINT8*)0x4567,0x34);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] 2.queueSend() ERROR\n");
        retVal = MEM_QUEUE_ERR;
    }
    res = vos_queueSend(qHandle,(UINT8*)0x89AB,0x56);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] 3.queueSend() ERROR\n");
        retVal = MEM_QUEUE_ERR;
    }
    res = vos_queueSend(qHandle,(UINT8*)0xCDEF,0x78); /* error expected because queue is full */
    if (res != VOS_QUEUE_FULL_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] 4.queueSend() ERROR\n");
        retVal = MEM_QUEUE_ERR;
    }
    res = vos_queueReceive(qHandle,&pData,&size,timeout.tv_usec);
    if ((res != VOS_NO_ERR) || (pData != (UINT8*)0x0123) || (size != 0x12))
    {
        printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] 1.queueReceive() ERROR\n");
        retVal = MEM_QUEUE_ERR;
    }
    res = vos_queueReceive(qHandle,&pData,&size,timeout.tv_usec);
    if ((res != VOS_NO_ERR) || (pData != (UINT8*)0x4567) || (size != 0x34))
    {
        printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] 2.queueReceive() ERROR\n");
        retVal = MEM_QUEUE_ERR;
    }
    res = vos_queueReceive(qHandle,&pData,&size,timeout.tv_usec);
    if ((res != VOS_NO_ERR) || (pData != (UINT8*)0x89AB) || (size != 0x56))
    {
        printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] 3.queueReceive() ERROR\n");
        retVal = MEM_QUEUE_ERR;
    }
    vos_getTime(&startTime);
    res = vos_queueReceive(qHandle,&pData,&size,timeout.tv_usec);
    vos_getTime(&endTime);
    vos_subTime(&endTime,&timeout);
    if ((res == VOS_NO_ERR) || (pData != (UINT8*)0x0) || (size != 0x0) || (!vos_cmpTime(&endTime,&startTime)))
    {
        printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] 4.queueReceive() ERROR\n");
        retVal = MEM_QUEUE_ERR;
    }
    res = vos_queueDestroy(qHandle);
    printOut(OUTPUT_ADVANCED,"[MEM_QUEUE] finished with errcnt = %i\n",res);
    return retVal;
}

int compareuints(const void * a, const void * b)
{
    return ( *(UINT8*)a - *(UINT8*)b);
}

MEM_ERR_T L3_test_mem_help()
{
    UINT8 *pItem = 0;
    UINT8 array2sort[5] = {3, 2, 4, 0, 1};
    CHAR8 str1[] = "string1";
    CHAR8 str2[] = "string2";
    UINT8 keyUINT = 0;
    MEM_ERR_T retVal = MEM_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[MEM_HELP] start...\n");
    /* qsort */
    vos_qsort(array2sort,5,sizeof(UINT8),compareuints);
    if ((array2sort[0] != 0) || (array2sort[1] != 1) || (array2sort[2] != 2)
            || (array2sort[3] != 3) || (array2sort[4] != 4))
    {
        retVal = MEM_HELP_ERR;
        printOut(OUTPUT_ADVANCED,"[MEM_HELP] vos_qsort() error\n");
    }
    printOut(OUTPUT_FULL,"[MEM_HELP] array = %u %u %u %u %u\n",array2sort[0],array2sort[1],array2sort[2],array2sort[3],array2sort[4]);

    /* bsearch */
    keyUINT = 3;
    pItem = vos_bsearch(&keyUINT,array2sort,5,sizeof(UINT8), compareuints);
    if ((*pItem != keyUINT) || (pItem == 0))
    {
        retVal = MEM_HELP_ERR;
        printOut(OUTPUT_ADVANCED,"[MEM_HELP] vos_bsearch() error\n");
    }
    keyUINT = 5;
    pItem = (UINT8*)vos_bsearch(&keyUINT,array2sort,4,sizeof(UINT8), compareuints);
    if (pItem != 0)
    {
        retVal = MEM_HELP_ERR;
        printOut(OUTPUT_ADVANCED,"[MEM_HELP] vos_bsearch() error\n");
    }

    /* strnicmp */
    if (vos_strnicmp(str1,str2,6) != 0)
    {
        retVal = MEM_HELP_ERR;
        printOut(OUTPUT_ADVANCED,"[MEM_HELP] vos_strnicmp() error\n");
    }
    if (vos_strnicmp(str1,str2,7) >= 0)
    {
        retVal = MEM_HELP_ERR;
        printOut(OUTPUT_ADVANCED,"[MEM_HELP] vos_strnicmp() error\n");
    }

    /* strncpy */
    vos_strncpy(str2,str1,4);
    if (vos_strnicmp(str2,"string1",6) != 0)
    {
        retVal = MEM_HELP_ERR;
        printOut(OUTPUT_ADVANCED,"[MEM_HELP] vos_strncpy() 1 error\n");
    }
    vos_strncpy(str2,str1,7);
    if (vos_strnicmp(str2,"string1",7) != 0)
    {
        retVal = MEM_HELP_ERR;
        printOut(OUTPUT_ADVANCED,"[MEM_HELP] vos_strncpy() 2 error\n");
    }
    printOut(OUTPUT_ADVANCED,"[MEM_HELP] finished...\n");
    return retVal;
}

MEM_ERR_T L3_test_mem_count()
{
    MEM_ERR_T retVal = MEM_NO_ERR;
    UINT8 *ptr1 = 0, *ptr2 = 0;
    UINT32  allocatedMemory = 0;
    UINT32  freeMemory = 0;
    UINT32  minFree = 0;
    UINT32  numAllocBlocks = 0;
    UINT32  numAllocErr = 0;
    UINT32  numFreeErr = 0;
    UINT32  blockSize[VOS_MEM_NBLOCKSIZES];
    UINT32  usedBlockSize[VOS_MEM_NBLOCKSIZES];

    printOut(OUTPUT_ADVANCED,"[MEM_COUNT] start...\n");
    vos_memCount(&allocatedMemory,&freeMemory,&minFree,&numAllocBlocks,&numAllocErr,&numFreeErr,blockSize,usedBlockSize);
    if (allocatedMemory != RESERVED_MEMORY
            || freeMemory != RESERVED_MEMORY
            || numAllocBlocks != 0
            || numAllocErr != 0
            || numFreeErr != 0)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_COUNT] Test 1 Error\n");
        retVal = MEM_COUNT_ERR;
    }
    ptr1 = (UINT8*)vos_memAlloc(8);
    vos_memCount(&allocatedMemory,&freeMemory,&minFree,&numAllocBlocks,&numAllocErr,&numFreeErr,blockSize,usedBlockSize);
    if (allocatedMemory != RESERVED_MEMORY
            || numAllocBlocks != 1
            || numAllocErr != 0
            || numFreeErr != 0)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_COUNT] Test 2 Error\n");
        retVal = MEM_COUNT_ERR;
    }
    ptr2 = (UINT8*)vos_memAlloc(1600);
    vos_memCount(&allocatedMemory,&freeMemory,&minFree,&numAllocBlocks,&numAllocErr,&numFreeErr,blockSize,usedBlockSize);
    if (allocatedMemory != RESERVED_MEMORY
            || numAllocBlocks != 2
            || numAllocErr != 0
            || numFreeErr != 0)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_COUNT] Test 3 Error\n");
        retVal = MEM_COUNT_ERR;
    }
    vos_memFree(ptr1);
    vos_memCount(&allocatedMemory,&freeMemory,&minFree,&numAllocBlocks,&numAllocErr,&numFreeErr,blockSize,usedBlockSize);
    if (allocatedMemory != RESERVED_MEMORY
            || numAllocBlocks != 1
            || numAllocErr != 0
            || numFreeErr != 0)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_COUNT] Test 4 Error\n");
        retVal = MEM_COUNT_ERR;
    }
    vos_memFree(ptr2);
    vos_memCount(&allocatedMemory,&freeMemory,&minFree,&numAllocBlocks,&numAllocErr,&numFreeErr,blockSize,usedBlockSize);
    if (allocatedMemory != RESERVED_MEMORY
            || freeMemory != RESERVED_MEMORY
            || numAllocBlocks != 0
            || numAllocErr != 0
            || numFreeErr != 0)
    {
        printOut(OUTPUT_ADVANCED,"[MEM_COUNT] Test 5 Error\n");
        retVal = MEM_COUNT_ERR;
    }
    printOut(OUTPUT_ADVANCED,"[MEM_COUNT] finished\n");
    return retVal;
}

MEM_ERR_T L3_test_mem_delete()
{
    /* tested with debugger, it seems to be ok */
    vos_memDelete(NULL);
    return MEM_NO_ERR;
}

VOS_THREAD_FUNC_T testSend(void* arguments)
{
    TEST_ARGS_THREAD *arg1 = (TEST_ARGS_THREAD*) arguments;
    VOS_QUEUE_T qHeader = arg1->queueHeader;
    VOS_SEMA_T queueSema = arg1->queueSema;
    VOS_SEMA_T printSema = arg1->printSema;
    VOS_SEMA_T helpSema = arg1->helpSema;
    pthread_t pthreadID = pthread_self();
    VOS_ERR_T res = VOS_UNKNOWN_ERR;
    VOS_ERR_T retVal = VOS_NO_ERR;

    res = vos_semaTake(printSema,0xffffffff);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] Could not take printSema in time\n");
        retVal = VOS_SEMA_ERR;
    }

#ifdef WIN32
    printOut(OUTPUT_FULL,"[SEND THREAD] Thread %ld start\n",(long int)pthreadID.p);
#else
    printOut(OUTPUT_FULL,"[SEND THREAD] Thread %ld start\n",(long int)pthreadID);
#endif
    printOut(OUTPUT_FULL,"[SEND THREAD] Got print, give queue, take gelp, give print\n");
    vos_semaGive(queueSema);
    res = vos_semaTake(helpSema,0xffffffff);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] Could not take helpSema in time\n");
        retVal = VOS_SEMA_ERR;
    }
    vos_semaGive(printSema);
    vos_semaTake(queueSema,0xffffffff);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] Could not take queueSema in time\n");
        retVal = VOS_SEMA_ERR;
    }
    res = vos_queueSend(qHeader,(UINT8*)0x11111111,0xabababab);
    if (res == 0)
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] send success \n");
    }
    else
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] send error %i\n",res);
        retVal = VOS_QUEUE_ERR;
    }
    vos_semaTake(printSema,0xffffffff);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] Could not take printSema in time\n");
        retVal = VOS_SEMA_ERR;
    }
    printOut(OUTPUT_FULL,"[SEND THREAD] Got queue, queued in, got print\n");
#ifdef WIN32
    printOut(OUTPUT_FULL,"[SEND THREAD] Thread %ld finished\n",(long int)pthreadID.p);
#else
    printOut(OUTPUT_FULL,"[SEND THREAD] Thread %ld finished\n",(long int)pthreadID);
#endif
    vos_semaGive(printSema);
    vos_semaGive(helpSema);
    vos_semaGive(queueSema);
    arg1->result = retVal;
    return arguments;
}

VOS_THREAD_FUNC_T testRecv(void* arguments)
{
    UINT8 *pData = 0;
    UINT32 size = 0;
    TEST_ARGS_THREAD *arg1 = (TEST_ARGS_THREAD*) arguments;
    VOS_QUEUE_T qHeader = arg1->queueHeader;
    VOS_SEMA_T queueSema = arg1->queueSema;
    VOS_SEMA_T printSema = arg1->printSema;
    VOS_SEMA_T helpSema = arg1->helpSema;
    VOS_ERR_T res = VOS_UNKNOWN_ERR;
    VOS_ERR_T retVal = VOS_NO_ERR;
    pthread_t pthreadID = pthread_self();
    struct timespec waitTime = arg1->delay;

#ifdef WIN32
    printOut(OUTPUT_FULL,"[RECV THREAD] Thread %ld start\n",(long int)pthreadID.p);
#else
    printOut(OUTPUT_FULL,"[RECV THREAD] Thread %ld start\n",(long int)pthreadID);
#endif
    res = vos_threadDelay((UINT32)(waitTime.tv_sec*1000*1000)+((waitTime.tv_nsec)/1000));
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_FULL,"[RECV_THREAD] vos_threadDelay() error\n");
        retVal = VOS_THREAD_ERR;
    }
    vos_semaGive(printSema);
    res = vos_semaTake(queueSema,0xffffffff);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] Could not take queueSema in time\n");
        retVal = VOS_SEMA_ERR;
    }
    vos_semaGive(helpSema);
    res = vos_semaTake(printSema,0xffffffff);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] Could not take printSema in time\n");
        retVal = VOS_SEMA_ERR;
    }
    printOut(OUTPUT_FULL,"[RECV THREAD] gave print, got queue, gave help, got print, give queue, give print\n");
    vos_semaGive(queueSema);
    vos_semaGive(printSema);
    vos_semaTake(helpSema,0xffffffff);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] Could not take helpSema in time\n");
        retVal = VOS_SEMA_ERR;
    }
    vos_semaTake(queueSema,0xffffffff);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] Could not take queueSema in time\n");
        retVal = VOS_SEMA_ERR;
    }
    res = vos_queueReceive(qHeader,&pData,&size,2000000);
    if (res == 0)
    {
        printOut(OUTPUT_FULL,"[RECV THREAD] recv success \n");
        printOut(OUTPUT_FULL,"[RECV THREAD] pData: %x\n[RECV THREAD] size: %x\n",(unsigned int)pData,(unsigned int)size);
    }
    else
    {
        printOut(OUTPUT_FULL,"[RECV THREAD] recv error %i\n",res);
        retVal = VOS_QUEUE_ERR;
    }
    vos_semaGive(queueSema);
    vos_semaTake(printSema,0xffffff);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_FULL,"[SEND THREAD] Could not take printSema in time\n");
        retVal = VOS_SEMA_ERR;
    }
#ifdef VXWORKS
    printOut(OUTPUT_FULL,"[RECV THREAD] Thread %ld finished\n",(long int)pthreadID);
#endif
#ifdef WIN32
    printOut(OUTPUT_FULL,"[RECV THREAD] Thread %ld finished\n",(long int)pthreadID.p);
#endif
    vos_semaGive(printSema);
    vos_semaGive(helpSema);
    arg1->result = retVal;
    return arguments;
}

THREAD_ERR_T L3_test_thread_init()
{
    VOS_ERR_T res = VOS_NO_ERR;
    THREAD_ERR_T retVal = THREAD_NO_ERR;
    VOS_QUEUE_T queueHeader;
    VOS_THREAD_T sendThread = 0;
    VOS_THREAD_T recvThread = 0;
    TEST_ARGS_THREAD arg1;
    TEST_ARGS_THREAD arg2;
    VOS_SEMA_T sema;
    VOS_TIME_T startTime, endTime;
    BOOL8 both_finished = FALSE;

    printOut(OUTPUT_ADVANCED,"[THREAD_INIT] start...\n");
    /********************************************************************************************/
    /*  First Run                                                                               */
    /********************************************************************************************/
    /**********************/
    /*  vos_threadInit()  */
    /**********************/
    res = vos_threadInit();
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadInit() Error\n");
        retVal = THREAD_INIT_ERR;
    }
    vos_getTime(&startTime);
    res = vos_threadDelay(100000);
    vos_getTime(&endTime);
    if ((res != VOS_NO_ERR) && (vos_cmpTime(&endTime,&startTime)))
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadDelay() Error\n");
        retVal = THREAD_INIT_ERR;
    }
    /**********************/
    /*  vos_queueCreate() */
    /**********************/
    res = vos_queueCreate(VOS_QUEUE_POLICY_FIFO,5,&queueHeader);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_queueCreate() Error\n");
    }

    /************************/
    /* Make presets 1       */
    /************************/
    arg1.queueHeader = queueHeader;
    arg2.queueHeader = queueHeader;
    res = vos_semaCreate(&sema,VOS_SEMA_EMPTY);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_semaCreate() Error\n");
    }
    arg1.printSema = sema;
    arg2.printSema = sema;
    res = vos_semaCreate(&sema,VOS_SEMA_EMPTY);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_semaCreate() Error\n");
    }
    arg1.queueSema = sema;
    arg2.queueSema = sema;
    res = vos_semaCreate(&sema,VOS_SEMA_EMPTY);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_semaCreate() Error\n");
    }
    arg1.helpSema = sema;
    arg2.helpSema = sema;
    arg1.delay.tv_nsec = 0;
    arg1.delay.tv_sec = 0;
    arg2.delay.tv_nsec = 100000000;
    arg2.delay.tv_sec = 0;
    arg1.result = VOS_UNKNOWN_ERR;
    arg2.result = VOS_UNKNOWN_ERR;

    /************************/
    /*  vos_threadCreate()1 */
    /************************/
    printOut(OUTPUT_FULL,"[THREAD_INIT] First run start\n");
    res = vos_threadCreate(&sendThread,"sendThread",THREAD_POLICY,0,0,0,(void*)testSend,(void*)&arg1);
    if ((res != VOS_NO_ERR)||(sendThread == 0))
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadCreate() [1] sendThread Error\n");
    }
    res = vos_threadCreate(&recvThread,"recvThread",THREAD_POLICY,0,0,0,(void*)testRecv,(void*)&arg2);
    if ((res != VOS_NO_ERR)||(recvThread == 0))
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadCreate() [1] recvThread Error\n");
    }
    /**************************/
    /*  vos_threadIsActive()1 */
    /**************************/
    res = vos_threadIsActive(sendThread);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadIsActive() [1] sendThread Error\n");
    }
    res = vos_threadIsActive(recvThread);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadIsActive() [1] recvThread Error\n");
    }
    /***************************/
    /*  vos_threadTerminate()1 */
    /***************************/
    res = vos_threadTerminate(sendThread);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadTerminate() [1] sendThread Error\n");
    }
    res = vos_threadTerminate(recvThread);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadTerminate() [1] recvThread Error\n");
    }
    vos_threadDelay(10000);
    /*Threads are terminated before regular exit, therefore error is expected */
    if ((arg1.result != VOS_NO_ERR)||(arg2.result != VOS_NO_ERR))
    {
        printOut(OUTPUT_FULL,"[THREAD_INIT] First run terminated OK\n");
    }
    else
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] ERROR First run terminated with error(s) in thread(s)\n");
        retVal = THREAD_INIT_ERR;
    }
    /********************************************************************************************/
    /*  Second Run                                                                              */
    /********************************************************************************************/
    /************/
    /* clean up */
    /************/
    vos_semaDelete(arg1.printSema);
    vos_semaDelete(arg1.helpSema);
    vos_semaDelete(arg1.queueSema);
    /************************/
    /* Make presets 2       */
    /************************/
    res = vos_semaCreate(&sema,VOS_SEMA_EMPTY);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_semaCreate() Error\n");
    }
    arg1.printSema = sema;
    arg2.printSema = sema;
    res = vos_semaCreate(&sema,VOS_SEMA_EMPTY);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_semaCreate() Error\n");
    }
    arg1.queueSema = sema;
    arg2.queueSema = sema;
    res = vos_semaCreate(&sema,VOS_SEMA_EMPTY);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_semaCreate() Error\n");
    }
    arg1.helpSema = sema;
    arg2.helpSema = sema;
    arg1.result = VOS_UNKNOWN_ERR;
    arg2.result = VOS_UNKNOWN_ERR;
    arg2.delay.tv_nsec = 0;
    arg2.delay.tv_sec = 0;
    /************************/
    /*  vos_threadCreate()2 */
    /************************/
    printOut(OUTPUT_FULL,"[THREAD_INIT] Second run start\n");
    res = vos_threadCreate(&sendThread,"sendThread",THREAD_POLICY,0,0,0,(void*)testSend,(void*)&arg1);
    if ((res != VOS_NO_ERR)||(sendThread == 0))
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadCreate() [2] sendThread Error\n");
    }
    res = vos_threadCreate(&recvThread,"recvThread",THREAD_POLICY,0,0,0,(void*)testRecv,(void*)&arg2);
    if ((res != VOS_NO_ERR)||(recvThread == 0))
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadCreate() [2] recvThread Error\n");
    }
    /**************************/
    /*  vos_threadIsActive()2a */
    /**************************/
    res = vos_threadIsActive(sendThread);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadIsActive() [2a] sendThread Error\n");
    }
    res = vos_threadIsActive(recvThread);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadIsActive() [2a] recvThread Error\n");
    }
    /* wait until threads are finished */
    both_finished = FALSE;
    while (both_finished == FALSE)
    {
        vos_threadDelay(10000);
        res = vos_threadIsActive(sendThread);
        if (res == VOS_PARAM_ERR)
        {
            res = vos_threadIsActive(recvThread);
            if (res == VOS_PARAM_ERR)
            {
                both_finished = TRUE;
            }
        }
    }
    /*vos_threadDelay(20000);*/
    /***************************/
    /*  vos_threadTerminate()2 */
    /***************************/
    res = vos_threadTerminate(sendThread); /* should now return error because threads are finished */
    if (res == VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadTerminate() [2] sendThread Error\n");
    }
    res = vos_threadTerminate(recvThread);
    if (res == VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadTerminate() [2] recvThread Error\n");
    }
    vos_threadDelay(10000);
    /**************************/
    /*  vos_threadIsActive()2b */
    /**************************/
    res = vos_threadIsActive(sendThread); /*threads should not be active any longer */
    if (res == VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadIsActive() [2b] sendThread Error\n");
    }
    res = vos_threadIsActive(recvThread);
    if (res == VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadIsActive() [2b] recvThread Error\n");
    }
    if ((arg1.result == VOS_NO_ERR)||(arg2.result == VOS_NO_ERR))
    {
        printOut(OUTPUT_FULL,"[THREAD_INIT] Second run terminated OK\n");
    }
    else
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] ERROR Second run terminated with error(s) in thread(s)\n");
        retVal = THREAD_INIT_ERR;
    }
    /********************************************************************************************/
    /*  Third Run                                                                               */
    /********************************************************************************************/
    /************/
    /* clean up */
    /************/
    vos_semaDelete(arg1.printSema);
    vos_semaDelete(arg1.helpSema);
    vos_semaDelete(arg1.queueSema);
    /************************/
    /* Make presets 3       */
    /************************/
    res = vos_semaCreate(&sema,VOS_SEMA_EMPTY);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_semaCreate() Error\n");
    }
    arg1.printSema = sema;
    arg2.printSema = sema;
    res = vos_semaCreate(&sema,VOS_SEMA_EMPTY);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_semaCreate() Error\n");
    }
    arg1.queueSema = sema;
    arg2.queueSema = sema;
    res = vos_semaCreate(&sema,VOS_SEMA_EMPTY);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_semaCreate() Error\n");
    }
    arg1.helpSema = sema;
    arg2.helpSema = sema;
    arg2.delay.tv_sec = 0;
    arg2.delay.tv_nsec = 10000;
    arg1.result = VOS_UNKNOWN_ERR;
    arg2.result = VOS_UNKNOWN_ERR;
    /************************/
    /*  vos_threadCreate()3 */
    /************************/
    printOut(OUTPUT_FULL,"[THREAD_INIT] Third run start\n");
    res = vos_threadCreate(&sendThread,"sendThread",THREAD_POLICY,0,0,0,(void*)testSend,(void*)&arg1);
    if ((res != VOS_NO_ERR)||(sendThread == 0))
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadCreate() [3] sendThread Error\n");
    }
    res = vos_threadCreate(&recvThread,"recvThread",THREAD_POLICY,0,0,0,(void*)testRecv,(void*)&arg2);
    if ((res != VOS_NO_ERR)||(recvThread == 0))
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadCreate() [3] recvThread Error\n");
    }
    /*vos_threadDelay(10000);*/
    /**************************/
    /*  vos_threadIsActive()3a */
    /**************************/
    res = vos_threadIsActive(sendThread);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadIsActive() [3a] sendThread Error\n");
    }
    res = vos_threadIsActive(recvThread);
    if (res != VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadIsActive() [3a] recvThread Error\n");
    }
    both_finished = FALSE;
    while (both_finished == FALSE)
    {
        vos_threadDelay(10000);
        res = vos_threadIsActive(sendThread);
        if (res == VOS_PARAM_ERR)
        {
            res = vos_threadIsActive(recvThread);
            if (res == VOS_PARAM_ERR)
            {
                both_finished = TRUE;
            }
        }
    }
    /**************************/
    /*  vos_threadIsActive()3b */
    /**************************/
    res = vos_threadIsActive(sendThread); /*threads should not be active any longer */
    if (res == VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadIsActive() [3b] sendThread Error\n");
    }
    res = vos_threadIsActive(recvThread);
    if (res == VOS_NO_ERR)
    {
        retVal = THREAD_INIT_ERR;
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_threadIsActive() [3b] recvThread Error\n");
    }
    if ((arg1.result == VOS_NO_ERR)||(arg2.result == VOS_NO_ERR))
    {
        printOut(OUTPUT_FULL,"[THREAD_INIT] Third run terminated OK\n");
    }
    else
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] ERROR Third run terminated with error(s) in thread(s)\n");
        retVal = THREAD_INIT_ERR;
    }
    vos_threadTerm();
    vos_semaDelete(arg1.printSema);
    vos_semaDelete(arg1.helpSema);
    vos_semaDelete(arg1.queueSema);
    res = vos_queueDestroy(queueHeader);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_INIT] vos_queueDestroy() Error\n");
        retVal = THREAD_INIT_ERR;
    }

    printOut(OUTPUT_ADVANCED,"[THREAD_INIT] finished\n");
    return retVal;
}

THREAD_ERR_T L3_test_thread_getTime()
{
    VOS_TIME_T sysTime;

    printOut(OUTPUT_ADVANCED,"[THREAD_GETTIME] start...\n");
    vos_getTime(&sysTime);
    printOut(OUTPUT_FULL,"[THREAD_GETTIME] time is: %lu:%lu\n",(long unsigned int)sysTime.tv_sec, (long unsigned int)sysTime.tv_usec);
    printOut(OUTPUT_ADVANCED,"[THREAD_GETTIME] finished \n");
    return THREAD_NO_ERR;
}

THREAD_ERR_T L3_test_thread_getTimeStamp()
{
    CHAR8 pTimeString[32] = {0};

    printOut(OUTPUT_ADVANCED,"[THREAD_GETTIMESTAMP] start...\n");
    vos_strncpy(pTimeString,vos_getTimeStamp(),32);
    printOut(OUTPUT_FULL,"[THREAD_GETTIMESTAMP] Time Stamp: %s\n",pTimeString);
    printOut(OUTPUT_ADVANCED,"[THREAD_GETTIMESTAMP] finished \n");
    return THREAD_NO_ERR;
}

THREAD_ERR_T L3_test_thread_addTime()
{
    VOS_TIME_T time = { 1 /*sec */, 0 /* usec */ };
    VOS_TIME_T add =  { 0 /*sec */, 2 /* usec */ };
    THREAD_ERR_T retVal = THREAD_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[THREAD_ADDTIME] start...\n");
    vos_addTime(&time, &add);
    if (time.tv_sec != 1 || time.tv_usec != 2)
    {
        retVal = THREAD_ADDTIME_ERR;
    }

    time.tv_sec =  1 /*sec */;    time.tv_usec = 1 /* usec */;
    add.tv_sec = 1 /*sec */;    add.tv_usec = 2 /* usec */;
    vos_addTime(&time, &add);
    if (time.tv_sec != 2 || time.tv_usec != 3)
    {
        retVal = THREAD_ADDTIME_ERR;
    }

    time.tv_sec =  1 /*sec */;    time.tv_usec = 1 /* usec */;
    add.tv_sec = 1 /*sec */;    add.tv_usec = 999999 /* usec */;
    vos_addTime(&time, &add);
    if (time.tv_sec != 3 || time.tv_usec != 0)
    {
        retVal = THREAD_ADDTIME_ERR;
    }

    time.tv_sec =  2 /*sec */;    time.tv_usec = 999999 /* usec */;
    add.tv_sec = 1 /*sec */;    add.tv_usec = 999999 /* usec */;
    vos_addTime(&time, &add);
    if (time.tv_sec != 4 || time.tv_usec != 999998)
    {
        retVal = THREAD_ADDTIME_ERR;
    }
    printOut(OUTPUT_ADVANCED,"[THREAD_ADDTIME] finished \n");
    return retVal;
}

THREAD_ERR_T L3_test_thread_subTime()
{
    VOS_TIME_T time = { 1 /*sec */, 4 /* usec */ };
    VOS_TIME_T subs =  { 0 /*sec */, 2 /* usec */ };
    THREAD_ERR_T retVal = THREAD_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[THREAD_SUBTIME] start...\n");
    vos_subTime(&time, &subs);
    if (time.tv_sec != 1 || time.tv_usec != 2)
    {
        retVal = THREAD_SUBTIME_ERR;
    }

    time.tv_sec =  1 /*sec */;    time.tv_usec = 3 /* usec */;
    subs.tv_sec = 1 /*sec */;    subs.tv_usec = 2 /* usec */;
    vos_subTime(&time, &subs);
    if (time.tv_sec != 0 || time.tv_usec != 1)
    {
        retVal = THREAD_SUBTIME_ERR;
    }

    time.tv_sec =  3 /*sec */;    time.tv_usec = 1 /* usec */;
    subs.tv_sec = 1 /*sec */;    subs.tv_usec = 999998 /* usec */;
    vos_subTime(&time, &subs);
    if (time.tv_sec != 1 || time.tv_usec != 3)
    {
        retVal = THREAD_SUBTIME_ERR;
    }

    time.tv_sec =  3 /*sec */;    time.tv_usec = 0 /* usec */;
    subs.tv_sec = 1 /*sec */;    subs.tv_usec = 999999 /* usec */;
    vos_subTime(&time, &subs);
    if (time.tv_sec != 1 || time.tv_usec != 1)
    {
        retVal = THREAD_SUBTIME_ERR;
    }

    printOut(OUTPUT_ADVANCED,"[THREAD_SUBTIME] finished\n");
    return retVal;
}

THREAD_ERR_T L3_test_thread_mulTime()
{
    VOS_TIME_T time = { 2 /*sec */, 4 /* usec */ };
    UINT32 mul =  0;
    THREAD_ERR_T retVal = THREAD_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[THREAD_MULTIME] start...\n");
    vos_mulTime(&time,mul);
    if (time.tv_sec != 0 || time.tv_usec != 0)
    {
        retVal = THREAD_MULTIME_ERR;
    }

    time.tv_sec =  2 /*sec */;    time.tv_usec = 4 /* usec */;
    mul = 1;
    vos_mulTime(&time, mul);
    if (time.tv_sec != 2 || time.tv_usec != 4)
    {
        retVal = THREAD_MULTIME_ERR;
    }

    time.tv_sec =  2 /*sec */;    time.tv_usec = 4 /* usec */;
    mul = 2;
    vos_mulTime(&time, mul);
    if (time.tv_sec != 4 || time.tv_usec != 8)
    {
        retVal = THREAD_MULTIME_ERR;
    }

    time.tv_sec =  2 /*sec */;    time.tv_usec = 999999 /* usec */;
    mul = 2;
    vos_mulTime(&time, mul);
    if (time.tv_sec != 5 || time.tv_usec != 999998)
    {
        retVal = THREAD_MULTIME_ERR;
    }

    time.tv_sec =  2 /*sec */;    time.tv_usec = 500000 /* usec */;
    mul = 2;
    vos_mulTime(&time, mul);
    if (time.tv_sec != 5 || time.tv_usec != 0)
    {
        retVal = THREAD_MULTIME_ERR;
    }

    printOut(OUTPUT_ADVANCED,"[THREAD_MULTIME] finished\n");
    return retVal;
}

THREAD_ERR_T L3_test_thread_divTime()
{
    VOS_TIME_T time = { 5 /*sec */, 4 /* usec */ };
    UINT32 div = 1;
    THREAD_ERR_T retVal = THREAD_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[THREAD_DIVTIME] start...\n");
    vos_divTime(&time, div);
    if (time.tv_sec != 5 || time.tv_usec != 4)
    {
        retVal = THREAD_DIVTIME_ERR;
    }

    time.tv_sec =  5 /*sec */;    time.tv_usec = 0 /* usec */;
    div = 2;
    vos_divTime(&time, div);
    if (time.tv_sec != 2 || time.tv_usec != 500000)
    {
        retVal = THREAD_DIVTIME_ERR;
    }

    time.tv_sec =  5 /*sec */;    time.tv_usec = 0 /* usec */;
    div = 3;
    vos_divTime(&time, div);
    if (time.tv_sec != 1 || time.tv_usec != 666666)
    {
        retVal = THREAD_DIVTIME_ERR;
    }

    time.tv_sec =  3 /*sec */;    time.tv_usec = 2 /* usec */;
    div = 0;
    vos_divTime(&time, div);
    if (time.tv_sec != 3 || time.tv_usec != 2)
    {
        retVal = THREAD_DIVTIME_ERR;
    }

    printOut(OUTPUT_ADVANCED,"[THREAD_DIVTIME] finished\n");
    return retVal;
}

THREAD_ERR_T L3_test_thread_cmpTime()
{
    VOS_TIME_T time1 = { 1 /*sec */, 2 /* usec */ };
    VOS_TIME_T time2 = { 1 /*sec */, 2 /* usec */ };
    THREAD_ERR_T retVal = THREAD_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[THREAD_CMPTIME] start...\n");
    /* time 1 and time 2 should be equal */
    if (vos_cmpTime(&time1, &time2) != 0)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    time1.tv_sec = 1; time1.tv_usec = 2;
    time2.tv_sec = 1; time2.tv_usec = 3;
    /* time 1 should be shorter than time 2 */
    if (vos_cmpTime(&time1, &time2) != -1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    time1.tv_sec = 1; time1.tv_usec = 2;
    time2.tv_sec = 2; time2.tv_usec = 4;
    /* time 1 should be shorter than time 2 */
    if (vos_cmpTime(&time1, &time2) != -1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    time1.tv_sec = 1; time1.tv_usec = 3;
    time2.tv_sec = 1; time2.tv_usec = 2;
    /* time 1 should be greater than time 2 */
    if (vos_cmpTime(&time1, &time2) != 1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    time1.tv_sec = 2; time1.tv_usec = 4;
    time2.tv_sec = 1; time2.tv_usec = 2;
    /* time 1 should be greater than time 2 */
    if (vos_cmpTime(&time1, &time2) != 1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    /* macro timercmp() */
    /* there is a problem with >= and <= in windows */
    time1.tv_sec = 1; time1.tv_usec = 1;
    time2.tv_sec = 2; time2.tv_usec = 2;
    if (timercmp(&time1, &time2, <=) != 1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    time1.tv_sec = 1; time1.tv_usec = 1;
    time2.tv_sec = 1; time2.tv_usec = 2;
    if (timercmp(&time1, &time2, <=) != 1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    time1.tv_sec = 2; time1.tv_usec = 999999;
    time2.tv_sec = 3; time2.tv_usec = 0;
    if (timercmp(&time1, &time2, <=) != 1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    /* test for equal */
    time1.tv_sec = 1; time1.tv_usec = 1;
    time2.tv_sec = 1; time2.tv_usec = 1;
    if (timercmp(&time1, &time2, <=) != 1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    time1.tv_sec = 2; time1.tv_usec = 2;
    time2.tv_sec = 1; time2.tv_usec = 1;
    if (timercmp(&time1, &time2, >=) != 1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    time1.tv_sec = 1; time1.tv_usec = 2;
    time2.tv_sec = 1; time2.tv_usec = 1;
    if (timercmp(&time1, &time2, >=) != 1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    time1.tv_sec = 2; time1.tv_usec = 0;
    time2.tv_sec = 1; time2.tv_usec = 999999;
    if (timercmp(&time1, &time2, >=) != 1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    /* test for equal */
    time1.tv_sec = 3; time1.tv_usec = 4;
    time2.tv_sec = 3; time2.tv_usec = 4;
    if (timercmp(&time1, &time2, >=) != 1)
    {
        retVal = THREAD_CMPTIME_ERR;
    }

    printOut(OUTPUT_ADVANCED,"[THREAD_CMPTIME] finished\n");
    return retVal;
}

THREAD_ERR_T L3_test_thread_clearTime()
{
    VOS_TIME_T timeVar;
    THREAD_ERR_T retVal = THREAD_NO_ERR;
    timeVar.tv_sec = 123;
    timeVar.tv_usec = 456;

    printOut(OUTPUT_ADVANCED,"[THREAD_CLEARTIME] start...\n");
    vos_clearTime(&timeVar);

    if ((timeVar.tv_sec != 0)||(timeVar.tv_usec != 0))
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_CLEARTIME] vos_clearTime() Error\n");
        retVal = THREAD_CLEARTIME_ERR;
    }
    printOut(OUTPUT_ADVANCED,"[THREAD_CLEARTIME] finished\n");
    return retVal;
}

THREAD_ERR_T L3_test_thread_getUUID()
{
    VOS_UUID_T uuid1 = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    VOS_UUID_T uuid2 = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    UINT16 cnt = 0;
    THREAD_ERR_T retVal = THREAD_UUID_ERR;

    printOut(OUTPUT_ADVANCED,"[THREAD_GETUUID] start...\n");
    vos_getUuid(uuid1);
    vos_getUuid(uuid2);
    for (cnt = 0; cnt < 16; cnt++)
    {
        if (uuid1[cnt] != uuid2[cnt])
        {
            retVal = THREAD_NO_ERR;
        }
    }
    printOut(OUTPUT_FULL,"[THREAD_GETUUID] UUID1 = %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",uuid1[0],uuid1[1],uuid1[2],uuid1[3],uuid1[4],uuid1[5],uuid1[6],uuid1[7],uuid1[8],uuid1[9],uuid1[10],uuid1[11],uuid1[12],uuid1[13],uuid1[14],uuid1[15]);
    printOut(OUTPUT_FULL,"[THREAD_GETUUID] UUID2 = %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",uuid2[0],uuid2[1],uuid2[2],uuid2[3],uuid2[4],uuid2[5],uuid2[6],uuid2[7],uuid2[8],uuid2[9],uuid2[10],uuid2[11],uuid2[12],uuid2[13],uuid2[14],uuid2[15]);
    printOut(OUTPUT_ADVANCED,"[THREAD_GETUUID] finished\n");
    return retVal;
}

THREAD_ERR_T L3_test_thread_mutex()
{
    /* create lock trylock unlock delete */
    VOS_MUTEX_T mutex;
    THREAD_ERR_T retVal = THREAD_NO_ERR;
    VOS_ERR_T res = VOS_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[THREAD_MUTEX] start...\n");
    res = vos_mutexCreate(&mutex);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_MUTEX] mutexCreate Error\n");
        retVal = THREAD_MUTEX_ERR;
    }
    res = vos_mutexTryLock(mutex);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_MUTEX] mutexCreate Error\n");
        retVal = THREAD_MUTEX_ERR;
    }
    res = vos_mutexUnlock(mutex);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_MUTEX] mutexCreate Error\n");
        retVal = THREAD_MUTEX_ERR;
    }
    res = vos_mutexLock(mutex);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_MUTEX] mutexCreate Error\n");
        retVal = THREAD_MUTEX_ERR;
    }
    res = vos_mutexTryLock(mutex);
    /*retVal = vos_mutexLock(mutex); This would cause a deadlock!
    if (retVal != 0)
        return THREAD_MUTEX_ERR;*/
    vos_mutexDelete(mutex);
    printOut(OUTPUT_ADVANCED,"[THREAD_MUTEX] finished\n");
    return retVal;
}

THREAD_ERR_T L3_test_thread_sema()
{
    /* create take give delete */
    VOS_SEMA_T sema;
    VOS_TIME_T startTime, endTime;
    VOS_ERR_T res = VOS_NO_ERR;
    THREAD_ERR_T retVal = THREAD_NO_ERR;
    VOS_TIME_T timeout = {0,20000};
    INT32 ret = 0;

    printOut(OUTPUT_ADVANCED,"[THREAD_SEMA] start...\n");
    res = vos_semaCreate(&sema,VOS_SEMA_FULL);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_SEMA] semaCreate Error\n");
        retVal = THREAD_SEMA_ERR;
    }
    res = vos_semaTake(sema,VOS_SEMA_EMPTY);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_SEMA] semaTake[1] Error\n");
        retVal = THREAD_SEMA_ERR;
    }
    vos_semaGive(sema); /* void */
    res = vos_semaTake(sema, VOS_SEMA_WAIT_FOREVER);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_SEMA] semaTake[2] Error\n");
        retVal = THREAD_SEMA_ERR;
    }
    vos_getTime(&startTime);
    res = vos_semaTake(sema,timeout.tv_usec);
    vos_getTime(&endTime);
    if (res == VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_SEMA] semaTake[3] Error\n");
        retVal = THREAD_SEMA_ERR;
    }
    vos_subTime(&endTime,&timeout);
    ret = vos_cmpTime(&endTime,&startTime);
    if (ret <= 0)
    {
        printOut(OUTPUT_ADVANCED,"[THREAD_SEMA] semaTake Timeout ERROR\n");
        retVal = THREAD_SEMA_ERR;
    }
    vos_semaDelete(sema);
    printOut(OUTPUT_ADVANCED,"[THREAD_SEMA] finished\n");
    return retVal;
}

SOCK_ERR_T L3_test_sock_helpFunctions()
{
    CHAR8 pDottedIP[15] = "";
    UINT32 ipAddr = 0;
    UINT32 i = 0;
    VOS_IF_REC_T    ifAddrs[VOS_MAX_NUM_IF];
    UINT32          ifCnt = sizeof(ifAddrs)/sizeof(VOS_IF_REC_T);
    VOS_ERR_T res = VOS_NO_ERR;
    UINT8 macAddr[6] = {0,0,0,0,0,0};
    SOCK_ERR_T retVal = SOCK_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[SOCK_HELP] start...\n");
    /**************************/
    /* Testing vos_dottedIP() */
    /**************************/
    vos_strncpy(pDottedIP,"12.34.56.78",11);
    ipAddr = vos_dottedIP(pDottedIP);
    if (ipAddr != 203569230)
    {
        retVal = SOCK_HELP_ERR;
    }
    /**************************/
    /* Testing vos_ipDotted() */
    /**************************/
    ipAddr = 3463778370u;   /* 'u' for unsigned to get rid of compiler warning */
    vos_strncpy(pDottedIP,vos_ipDotted(ipAddr),13);
    if (vos_strnicmp(pDottedIP,"206.117.16.66",13))
    {
        retVal = SOCK_HELP_ERR;
    }
    /*******************************/
    /* Testing vos_getInterfaces() */
    /*******************************/
    res = vos_getInterfaces(&ifCnt,ifAddrs);
    for (i=0;i<ifCnt;i++)
    {
        printOut(OUTPUT_FULL,"[SOCK_HELP] IP:\t%u = %s\n",ifAddrs[i].ipAddr,vos_ipDotted(ifAddrs[i].ipAddr));
        printOut(OUTPUT_FULL,"[SOCK_HELP] MAC:\t%x-%x-%x-%x-%x-%x\n",ifAddrs[i].mac[0],ifAddrs[i].mac[1],ifAddrs[i].mac[2],ifAddrs[i].mac[3],ifAddrs[i].mac[4],ifAddrs[i].mac[5]);
        printOut(OUTPUT_FULL,"[SOCK_HELP] Mask:\t%u = %s\n",ifAddrs[i].netMask,vos_ipDotted(ifAddrs[i].netMask));
        printOut(OUTPUT_FULL,"[SOCK_HELP] Name:\t%s\n",ifAddrs[i].name);
    }
    if (res != VOS_NO_ERR)
    {
        retVal = SOCK_HELP_ERR;
    }
    /****************************/
    /* Testing vos_sockGetMac() */
    /****************************/
    res = vos_sockGetMAC(macAddr);
    if (res != VOS_NO_ERR)
    {
        retVal = SOCK_HELP_ERR;
    }
    else
    {
        printOut(OUTPUT_FULL,"[SOCK_HELP] MAC = %x:%x:%x:%x:%x:%x\n",macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]);
    }
    printOut(OUTPUT_ADVANCED,"[SOCK_HELP] finished\n");
    return retVal;
}

SOCK_ERR_T L3_test_sock_init()
{
    VOS_ERR_T res = VOS_NO_ERR;
    SOCK_ERR_T retVal = SOCK_NO_ERR;
    printOut(OUTPUT_ADVANCED,"[SOCK_INIT] start...\n");
    res = vos_sockInit();
    if (res != VOS_NO_ERR)
    {
        retVal = SOCK_INIT_ERR;
    }
    printOut(OUTPUT_ADVANCED,"[SOCK_INIT] finished\n");
    return retVal;
}

SOCK_ERR_T L3_test_sock_UDPMC(UINT8 sndBufStartVal, UINT8 rcvBufExpVal, TEST_ROLE_T role)
{
    SOCK_ERR_T retVal = SOCK_NO_ERR;
    VOS_ERR_T res = VOS_NO_ERR;
    INT32 sockDesc = 0;
    VOS_SOCK_OPT_T sockOpts;
    UINT32 mcIP = vos_dottedIP(MC_IP);
    UINT32 mcIF = vos_dottedIP(MC_IF);
    UINT32 destIP = vos_dottedIP(DEST_IP);
    UINT32 portPD = PORT_PD; /* according to IEC 61375-2-3 CDV A.2 */
    UINT8 sndBuf = sndBufStartVal;
    UINT8 rcvBuf = 0;
    UINT32 bufSize = cBufSize;
    BOOL8 received = FALSE;

    printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] start...\n");
    /*******************/
    /* open UDP socket */
    /*******************/
    res = vos_sockOpenUDP(&sockDesc,NULL);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockOpenUDP() ERROR!\n");
        retVal = SOCK_UDP_MC_ERR;
    }
    printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockOpenUDP() Open socket %i\n",sockDesc);
    /***************/
    /* set options */
    /***************/
    sockOpts.nonBlocking = FALSE;
    sockOpts.qos = 7;
    sockOpts.reuseAddrPort = TRUE;
    sockOpts.ttl_multicast = 63;
    printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockSetOptions()\n");
    res = vos_sockSetOptions(sockDesc,&sockOpts);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockSetOptions() ERROR!\n");
        retVal = SOCK_UDP_MC_ERR;
    }
   /* getsockopt(sockDesc,IPPROTO_IP,IP_MULTICAST_TTL,&value,(int*)&valueSize);
    printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_getsockopt() returned ip multicast ttl = %u\n",value);*/
    /********/
    /* bind */
    /********/
    res = vos_sockBind(sockDesc,mcIP,portPD);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockBind() ERROR!\n");
        retVal = SOCK_UDP_MC_ERR;
    }
    /***********/
    /* join mc */
    /***********/
    printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockJoinMC\n");
    res = vos_sockJoinMC(sockDesc,mcIP,mcIF);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockJoinMC() ERROR!\n");
        retVal = SOCK_UDP_MC_ERR;
    }
    /********************/
    /* set multicast if */
    /********************/
    printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockSetMulticastIF()\n");
    res = vos_sockSetMulticastIf(sockDesc,mcIF);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockSetMulticastIf() ERROR!\n");
        retVal = SOCK_UDP_MC_ERR;
    }
    if (role == ROLE_MASTER)
    {
        /**********************/
        /* send UDP Multicast */
        /**********************/
        vos_threadDelay(1000000);
        printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockSendUDP()\n");
        res = vos_sockSendUDP(sockDesc,&sndBuf,&bufSize,mcIP,portPD);
        if (res != VOS_NO_ERR)
        {
            printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockSendUDP() ERROR!\n");
            retVal = SOCK_UDP_MC_ERR;
        }
        /*************************/
        /* receive UDP Multicast */
        /*************************/
        /*ok here we first (re-)receive our own mc udp that was sent just above */
        /*TODO add check for own source ip and port to make sure */
        printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockReceive() retVal bisher = %u\n",retVal);
        res = vos_sockReceiveUDP(sockDesc,&rcvBuf,&bufSize,&gTestIP,&gTestPort,&destIP,FALSE);
        if (res != VOS_NO_ERR)
        {
            printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockreceiveUDP() ERROR!\n");
            retVal = SOCK_UDP_MC_ERR;
        }
        else
        {
            printOut(OUTPUT_FULL,"[SOCK_UDPMC] UDP MC received: %u\n",rcvBuf);
            printOut(OUTPUT_FULL,"[SOCK_UDPMC] Source: %s : %i\n",vos_ipDotted(gTestIP),gTestPort);
            printOut(OUTPUT_FULL,"[SOCK_UDPMC] Destination: %s\n",vos_ipDotted(destIP));
            received = TRUE;
        }
        /*and now here we get the response from our counterpart */
        printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockReceive() retVal bisher = %u\n",retVal);
        res = vos_sockReceiveUDP(sockDesc,&rcvBuf,&bufSize,&gTestIP,&gTestPort,&destIP,FALSE);
        if (res != VOS_NO_ERR)
        {
            printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockReceiveUDP() ERROR!\n");
            retVal = SOCK_UDP_MC_ERR;
        }
        else
        {
            if (rcvBuf != rcvBufExpVal)
            {
                printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] rcvBuf!= rcvBufExpVal ERROR!\n");
                retVal = SOCK_UDP_MC_ERR;
            }
            printOut(OUTPUT_FULL,"[SOCK_UDPMC] UDP MC received: %u\n",rcvBuf);
            printOut(OUTPUT_FULL,"[SOCK_UDPMC] Source: %s : %i\n",vos_ipDotted(gTestIP),gTestPort);
            printOut(OUTPUT_FULL,"[SOCK_UDPMC] Destination: %s\n",vos_ipDotted(destIP));
            received = TRUE;
        }
    }
    if(role == ROLE_SLAVE)
    {
        /*************************/
        /* receive UDP Multicast */
        /*************************/
        printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockReceive()\n");
        while(!received)
        {
            res = vos_sockReceiveUDP(sockDesc,&rcvBuf,&bufSize,&gTestIP,&gTestPort,&destIP,FALSE);
            if (res != VOS_NO_ERR)
            {
                printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockReceive() ERROR\n");
                retVal = SOCK_UDP_MC_ERR;
            }
            else
            {
                if(rcvBuf != rcvBufExpVal)
                {
                    retVal = SOCK_UDP_MC_ERR;
                    printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockReceive() Error: Received unexpected value\n");
                }
                printOut(OUTPUT_FULL,"[SOCK_UDPMC] UDP MC received: %u\n",rcvBuf);
                printOut(OUTPUT_FULL,"[SOCK_UDPMC] Source: %s : %i\n",vos_ipDotted(gTestIP),gTestPort);
                printOut(OUTPUT_FULL,"[SOCK_UDPMC] Destination: %s\n",vos_ipDotted(destIP));
                received = TRUE;
            }
        }
        /**********************/
        /* send UDP Multicast */
        /**********************/
        printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockSendUDP()\n");
        res = vos_sockSendUDP(sockDesc,&sndBuf,&bufSize,mcIP,portPD);
        if (res != VOS_NO_ERR)
        {
            printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockSendUDP() ERROR\n");
            retVal = SOCK_UDP_MC_ERR;
        }
    }
    /************/
    /* leave mc */
    /************/
    printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockLeaveMC()\n");
    res = vos_sockLeaveMC(sockDesc,mcIP,mcIF);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockLeaveMC() ERROR!\n");
        retVal = SOCK_UDP_MC_ERR;
    }
    /********************/
    /* close UDP socket */
    /********************/
    printOut(OUTPUT_FULL,"[SOCK_UDPMC] vos_sockClose()\n");
    res = vos_sockClose(sockDesc);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockClose() ERROR!\n");
        retVal = SOCK_UDP_MC_ERR;
    }
    printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] finished with %u\n",retVal);
    return retVal;
}

SOCK_ERR_T L3_test_sock_UDP(UINT8 sndBufStartVal, UINT8 rcvBufExpVal, TEST_ROLE_T role)
{
    VOS_ERR_T res = VOS_NO_ERR;
    SOCK_ERR_T retVal = SOCK_NO_ERR;
    VOS_SOCK_OPT_T sockOpts;
    INT32 sockDesc = 0;
    UINT32 ifIP = vos_dottedIP(IF_IP);
    UINT32 destIP = vos_dottedIP(DEST_IP);
    UINT32 portPD = PORT_PD; /* according to IEC 61375-2-3 CDV A.2 */
    UINT8 sndBuf = sndBufStartVal;
    UINT8 rcvBuf = 0;
    UINT8 rcvBufExp = rcvBufExpVal;
    UINT32 srcIP = (UINT32)NULL;
    UINT16 srcPort = 0;
    UINT32 bufSize = cBufSize;
    BOOL8 received = FALSE;
    UINT32 cnt = 0;

    printOut(OUTPUT_ADVANCED,"[SOCK_UDP] start...\n");
    /*******************/
    /* open UDP socket */
    /*******************/
    res = vos_sockOpenUDP(&sockDesc,NULL);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDPMC] vos_sockOpenUDP() ERROR!\n");
        retVal = SOCK_UDP_ERR;
    }
    printOut(OUTPUT_FULL,"[SOCK_UDP] vos_sockOpenUDP() Open socket %i\n",sockDesc);
    /***************/
    /* set options */
    /***************/
    sockOpts.nonBlocking = FALSE;
    sockOpts.qos = 7;
    sockOpts.reuseAddrPort = TRUE;
    sockOpts.ttl = 17;
    printOut(OUTPUT_FULL,"[SOCK_UDP] vos_sockSetOptions()\n");
    res = vos_sockSetOptions(sockDesc,&sockOpts);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDP] vos_sockSetOptions() ERROR\n");
        retVal = SOCK_UDP_ERR;
    }
    /*getsockopt(sockDesc,IPPROTO_IP,IP_MULTICAST_TTL,&value,(int*)&valueSize);
    printOut(OUTPUT_FULL,"[SOCK_UDP] vos_getsockopt() returned ip multicast ttl = %u\n",value);*/
    /********/
    /* bind */
    /********/
    printOut(OUTPUT_FULL,"[SOCK_UDP] vos_sockBind()\n");
    res = vos_sockBind(sockDesc,ifIP,portPD);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDP] vos_sockBind() ERROR!\n");
        retVal = SOCK_UDP_ERR;
    }
    if (role == ROLE_MASTER)
    {
        /************/
        /* send UDP */
        /************/
        printOut(OUTPUT_FULL,"[SOCK_UDP] vos_sockSendUDP() to %s:%u\n",vos_ipDotted(gTestIP),gTestPort);
        vos_threadDelay(500000);
        /* send with counter = 3 */
        res = vos_sockSendUDP(sockDesc,&sndBuf,&bufSize,gTestIP,gTestPort);
        if (res != VOS_NO_ERR)
        {
            printOut(OUTPUT_ADVANCED,"[SOCK_UDP] vos_sockSendUDP() ERROR!\n");
            retVal = SOCK_UDP_ERR;
        }
        /***************/
        /* receive UDP */
        /***************/
        printOut(OUTPUT_FULL,"[SOCK_UDP] vos_sockReceiveUDP()\n");
        /* expected received counter = 4*/
        res = vos_sockReceiveUDP(sockDesc,&rcvBuf,&bufSize,&srcIP,&srcPort,&destIP,FALSE);
        if (res != VOS_NO_ERR)
        {
            printOut(OUTPUT_ADVANCED,"[SOCK_UDP] UDP Receive Error\n");
            retVal = SOCK_UDP_ERR;
        }
        else
        {
            if ((rcvBuf != rcvBufExp) || (srcIP != gTestIP) || (srcPort != gTestPort))
            {
                retVal = SOCK_UDP_ERR;
                printOut(OUTPUT_ADVANCED,"[SOCK_UDP] UDP Receive Error\n");
                printOut(OUTPUT_FULL,"[SOCK_UDP] rcvBuf %u != %u\n",rcvBuf,rcvBufExpVal);
                printOut(OUTPUT_FULL,"[SOCK_UDP] srcIP %u != %u\n",srcIP,gTestIP);
                printOut(OUTPUT_FULL,"[SOCK_UDP] srcPort %u != %u\n",srcPort,gTestPort);
            }
            printOut(OUTPUT_FULL,"[SOCK_UDP] UDP received: %u\n",rcvBuf);
            printOut(OUTPUT_FULL,"[SOCK_UDP] Source: %s : %i\n",vos_ipDotted(srcIP),srcPort);
            printOut(OUTPUT_FULL,"[SOCK_UDP] Destination: %s\n",vos_ipDotted(destIP));
            printOut(OUTPUT_FULL,"[SOCK_UDP] Receive loops: %u\n",cnt);
            received = TRUE;
        }
    }
    if (role == ROLE_SLAVE)
    {
        /***************/
        /* receive UDP */
        /***************/
        printOut(OUTPUT_FULL,"[SOCK_UDP] vos_sockReceiveUDP()\n");
        res = vos_sockReceiveUDP(sockDesc,&rcvBuf,&bufSize,&srcIP,&srcPort,&destIP,FALSE);
        if (res != VOS_NO_ERR)
        {
            printOut(OUTPUT_FULL,"[SOCK_UDP] vos_sockReceiveUDP() ERROR\n");
            retVal = SOCK_UDP_ERR;
        }
        else
        {
            if ((rcvBuf != rcvBufExpVal) || (srcIP != gTestIP) || (srcPort != gTestPort))
            {
                retVal = SOCK_UDP_ERR;
                printOut(OUTPUT_FULL,"[SOCK_UDP] UDP Receive Error\n");
                printOut(OUTPUT_FULL,"[SOCK_UDP] srcIP(%s) != gTestIP(%s)\n",vos_ipDotted(srcIP),vos_ipDotted(gTestIP));
                printOut(OUTPUT_FULL,"[SOCK_UDP] srcPort(%s) != gTestPort(%s)\n",vos_ipDotted(srcPort),vos_ipDotted(gTestPort));
            }
            printOut(OUTPUT_FULL,"[SOCK_UDP] UDP received: %u\n",rcvBuf);
            printOut(OUTPUT_FULL,"[SOCK_UDP] Source: %s : %i\n",vos_ipDotted(srcIP),srcPort);
            printOut(OUTPUT_FULL,"[SOCK_UDP] Destination: %s\n",vos_ipDotted(destIP));
            received = TRUE;
        }
        /************/
        /* send UDP */
        /************/
        printOut(OUTPUT_FULL,"[SOCK_UDP] vos_sockSendUDP() to %s:%u\n",vos_ipDotted(gTestIP),gTestPort);
        vos_threadDelay(500000);
        /*Sleep(500);*/
        res = vos_sockSendUDP(sockDesc,&sndBuf,&bufSize,gTestIP,gTestPort);
        if (res != VOS_NO_ERR)
        {
            printOut(OUTPUT_ADVANCED,"[SOCK_UDP] vos_sockSendUDP() ERROR!\n");
            retVal = SOCK_UDP_ERR;
        }
    }
    /********************/
    /* close UDP socket */
    /********************/
    printOut(OUTPUT_FULL,"[SOCK_UDP] vos_sockClose()\n");
    res = vos_sockClose(sockDesc);
    if (res != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SOCK_UDP] vos_sockClose() ERROR!\n");
        retVal = SOCK_UDP_ERR;
    }
    printOut(OUTPUT_ADVANCED,"[SOCK_UDP] finished with %u\n",retVal);
    return retVal;
}

SOCK_ERR_T L3_test_sock_TCPclient(UINT8 sndBufStartVal, UINT8 rcvBufExpVal, TEST_ROLE_T role)
{
    VOS_ERR_T res = VOS_NO_ERR;
    SOCK_ERR_T retVal = SOCK_NO_ERR;
    VOS_SOCK_OPT_T sockOpts;
    INT32 sockDesc = 0;
    UINT32 ifIP = vos_dottedIP(IF_IP);
    UINT32 destIP = vos_dottedIP(DEST_IP);
    UINT32 portPD = PORT_PD; /* according to IEC 61375-2-3 CDV A.2 */
    UINT8 sndBuf = sndBufStartVal;
    UINT8 rcvBuf = 0;
    UINT32 bufSize = cBufSize;
    UINT16 srcPort = 0;
    UINT32 srcIP = (UINT32)NULL;
    INT32 newSock = 0;
    BOOL8 connected = FALSE;

    printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] start...\n");
    /*******************/
    /* open tcp socket */
    /*******************/
    printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockOpenTCP()\n");
    res = vos_sockOpenTCP(&sockDesc,NULL);
    if (res != VOS_NO_ERR)
    {
        retVal = SOCK_TCP_CLIENT_ERR;
        printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] Sockopen() Error res = %i\n",res);
    }
    printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockOpenTCP() Open socket %i\n",sockDesc);
    /***************/
    /* set options */
    /***************/
    sockOpts.nonBlocking = FALSE;
    sockOpts.qos = 19;
    sockOpts.reuseAddrPort = TRUE;
    sockOpts.ttl = 28;
    sockOpts.ttl_multicast = 37;
    printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockSetOptions()\n");
    res = vos_sockSetOptions(sockDesc,&sockOpts);
    if (res != VOS_NO_ERR)
    {
        retVal = SOCK_TCP_CLIENT_ERR;
        printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockSetOptions() ERROR res = %i\n",res);
    }
    /********/
    /* bind */
    /********/
    printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockBind()\n");
    res = vos_sockBind(sockDesc,ifIP,portPD);
    if (res != VOS_NO_ERR)
    {
        retVal = SOCK_TCP_CLIENT_ERR;
        printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockBind() ERROR res = %i\n",res);
    }
    if (role == ROLE_MASTER)
    {
        /***********/
        /* connect */
        /***********/
        connected = FALSE;
        printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockConnect()\n");
        while(!connected)
        {
            vos_threadDelay(100000);
            res = vos_sockConnect(sockDesc,destIP,portPD);
            if (res != VOS_NO_ERR)
            {
                retVal = SOCK_TCP_CLIENT_ERR;
                printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockConnect() ERROR res = %i\n",res);
            }
            else
            {
                connected = TRUE;
            }
        }
        /************/
        /* send tcp */
        /************/
        printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockSendTCP()\n");
        res = vos_sockSendTCP(sockDesc,&sndBuf,&bufSize);
        if (res != VOS_NO_ERR)
        {
            retVal = SOCK_TCP_CLIENT_ERR;
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockSendTCP() ERROR res = %i\n",res);
        }
        /***************/
        /* receive tcp */
        /***************/
        printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockReceiveTCP()\n");
        res = vos_sockReceiveTCP(sockDesc,&rcvBuf,&bufSize);
        if (res != VOS_NO_ERR)
        {
            retVal = SOCK_TCP_CLIENT_ERR;
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockReceiveTCP() ERROR res = %i\n",res);
        }
        else
        {
            if (rcvBuf != rcvBufExpVal)
            {
                retVal = SOCK_TCP_CLIENT_ERR;
                printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockReceiveTCP() ERROR res = %i\n",res);
            }
            printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockReceiveTCP() received: %u\n",rcvBuf);
        }
    }
    if (role == ROLE_SLAVE)
    {
        /***********/
        /* listen  */
        /***********/
        printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockListen()\n");
        res = vos_sockListen(sockDesc,1000);
        if (res != VOS_NO_ERR)
        {
            retVal = SOCK_TCP_CLIENT_ERR;
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockListen() ERROR = %i\n",res);
        }
        /**********/
        /* accept */
        /**********/
        printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockAccept()\n");
        res = vos_sockAccept(sockDesc,&newSock,&srcIP,&srcPort);
        if (res == VOS_NO_ERR)
        {
            printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] accepted from: %s : %u\n",vos_ipDotted(srcIP),srcPort);
            /*if((srcIP != gTestIP) || (srcPort != gTestPort))
            {
                printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockAccept() Error accepted from invalid source\n");
                retVal = SOCK_TCP_CLIENT_ERR;
            }*/
        }
        else
        {
            retVal = SOCK_TCP_CLIENT_ERR;
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockAccept() Error\n");
        }
        printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockReceiveTCP()\n");
        res = vos_sockReceiveTCP(newSock,&rcvBuf,&bufSize);
        if (res != VOS_NO_ERR)
        {
            retVal = SOCK_TCP_CLIENT_ERR;
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockReceiveTCP() ERROR res = %i\n",res);
        }
        else
        {
            if (rcvBuf != rcvBufExpVal)
            {
                retVal = SOCK_TCP_CLIENT_ERR;
                printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockReceiveTCP() Error\n");
            }
            printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockReceiveTCP() received:%u\n",rcvBuf);
        }

        /************/
        /* send tcp */
        /************/
        printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockSendTCP()\n");
        res = vos_sockSendTCP(newSock,&sndBuf,&bufSize);
        if (res != VOS_NO_ERR)
        {
            retVal = SOCK_TCP_CLIENT_ERR;
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockSendTCP() ERROR\n");
        }
        /********************/
        /* close tcp socket */
        /********************/
        printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockClose() newSock\n");
        vos_sockClose(newSock);
        if (res != VOS_NO_ERR)
        {
            retVal = SOCK_TCP_CLIENT_ERR;
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockClose() newSock ERROR\n");
        }
    }
    /********************/
    /* close tcp socket */
    /********************/
    printOut(OUTPUT_FULL,"[SOCK_TCPCLIENT] vos_sockClose()\n");
    res = vos_sockClose(sockDesc);
    if (res != VOS_NO_ERR)
    {
        retVal = SOCK_TCP_CLIENT_ERR;
        printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] vos_sockClose() sockDesc ERROR\n");
    }
    printOut(OUTPUT_ADVANCED,"[SOCK_TCPCLIENT] finished with %u\n",retVal);
    return retVal;
}
SOCK_ERR_T L3_test_sock_TCPserver(UINT8 sndBufStartVal, UINT8 rcvBufExpVal, TEST_ROLE_T role)
{
    VOS_ERR_T res = VOS_UNKNOWN_ERR;
    SOCK_ERR_T retVal = SOCK_NO_ERR;
    VOS_SOCK_OPT_T sockOpts;
    INT32 sockDesc = 0;
    INT32 newSock = 0;
    UINT32 ifIP = vos_dottedIP(IF_IP);
    UINT32 portPD = PORT_MD; /* according to IEC 61375-2-3 CDV A.2 */
    UINT8 sndBuf = sndBufStartVal;
    UINT8 rcvBuf = 0;
    UINT32 srcIP = (UINT32)NULL;
    UINT16 srcPort = 0;
    UINT32 bufSize = cBufSize;
    BOOL8 received = FALSE;
    BOOL8 connected = FALSE;
    UINT32 destIP = vos_dottedIP(DEST_IP);

    printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] start...\n");
    /*******************/
    /* open tcp socket */
   /*******************/
    printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockOpenTCP()\n");
    res = vos_sockOpenTCP(&sockDesc,NULL);
     if (res != VOS_NO_ERR)
    {
        retVal = SOCK_TCP_SERVER_ERR;
        printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockOpenTCP() ERROR res = %i\n",res);
    }
    printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockOpenTCP() Open socket %i\n",sockDesc);
    /***************/
    /* set options */
    /***************/
    sockOpts.nonBlocking = FALSE;
    sockOpts.qos = 19;
    sockOpts.reuseAddrPort = TRUE;
    sockOpts.ttl = 28;
    sockOpts.ttl_multicast = 37;
    printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockSetOptions()\n");
    res = vos_sockSetOptions(sockDesc,&sockOpts);
    if (res != VOS_NO_ERR)
    {
        retVal = SOCK_TCP_SERVER_ERR;
         printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockSetOptions() ERROR res = %i\n",res);
    }
    /********/
    /* bind */
    /********/
    printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockBind()\n");
    res = vos_sockBind(sockDesc,ifIP,portPD);
    if (res != VOS_NO_ERR)
    {
        retVal = SOCK_TCP_SERVER_ERR;
        printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockBind() ERROR res = %i\n",res);
    }
    if (role == ROLE_MASTER)
    {
        /**********/
        /* listen */
        /**********/
        printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockListen()\n");
        res = vos_sockListen(sockDesc,1000);
        if (res != VOS_NO_ERR)
        {
            retVal = SOCK_TCP_SERVER_ERR;
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockListen() ERROR res = %i\n",res);
        }
        /**********/
        /* accept */
        /**********/
        printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockAccept()\n");
        res = vos_sockAccept(sockDesc,&newSock,&srcIP,&srcPort);
        if (res == VOS_NO_ERR)
        {
            printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockAccept() Connection accepted from %s : %u, Socket %i\n",vos_ipDotted(srcIP),srcPort,newSock);
            connected = TRUE;
        }
        if ((res != VOS_NO_ERR) || (srcIP != srcIP) || (srcPort != portPD))
        {
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockAccept() ERROR Accepted connection from invalid source\n");
            retVal = SOCK_TCP_SERVER_ERR;
        }
        /***************/
        /* receive tcp */
        /***************/
        printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockReceiveTCP()\n");
        res = vos_sockReceiveTCP(newSock,&rcvBuf,&bufSize);
        if (res != VOS_NO_ERR || rcvBuf != rcvBufExpVal)
        {
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockReceiveTCP() ERROR Received invalid value\n");
            retVal = SOCK_TCP_SERVER_ERR;
        }
        else
        {
            printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockReceiveTCP() received: %u\n",rcvBuf);
            received = TRUE;
        }
        /****************/
        /* reply to TCP */
        /****************/
        printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockSendTCP()\n");
        res = vos_sockSendTCP(newSock,&sndBuf,&bufSize);
        if (res != VOS_NO_ERR)
        {
            retVal = SOCK_TCP_SERVER_ERR;
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockSendTCP() Error\n");
        }
        /*********************/
        /* close tcp sockets */
        /*********************/
        printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockClose() newSock\n");
        res = vos_sockClose(newSock);
        if (res != VOS_NO_ERR)
        {
            retVal = SOCK_TCP_SERVER_ERR;
            printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockClose() newSock Error\n");
        }
    }
    if (role == ROLE_SLAVE)
    {
       /***********/
       /* connect */
       /***********/
        connected = FALSE;
        while (!connected)
        {
            vos_threadDelay(1000000);
            printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockConnect()\n");
            res = vos_sockConnect(sockDesc,destIP,portPD);
            if (res != VOS_NO_ERR)
            {
                retVal = SOCK_TCP_SERVER_ERR;
                printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] sockConnect() res = %i\n",res);
            }
            else
            {
                connected = TRUE;
            }
        }
       /************/
       /* send tcp */
       /************/
       printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockSendTCP()\n");
       res = vos_sockSendTCP(sockDesc,&sndBuf,&bufSize);
       if (res != VOS_NO_ERR)
       {
           retVal = SOCK_TCP_SERVER_ERR;
           printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockSendTCP() ERROR res = %i\n",res);
       }
       /***************/
       /* receive tcp */
       /***************/
       printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockReceiveTCP()\n");
       res = vos_sockReceiveTCP(sockDesc,&rcvBuf,&bufSize);
       if (res != VOS_NO_ERR)
       {
           retVal = SOCK_TCP_SERVER_ERR;
           printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockReceiveTCP() ERROR res = %i\n",res);
       }
       else
       {
           if (rcvBuf != rcvBufExpVal)
           {
               retVal = SOCK_TCP_SERVER_ERR;
               printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockReceiveTCP() ERROR res = %i\n",res);
           }
           printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockReceiveTCP() received: %u\n",rcvBuf);
       }
    }
    printOut(OUTPUT_FULL,"[SOCK_TCPSERVER] vos_sockClose() sockDesc\n");
    res = vos_sockClose(sockDesc);
    if (res != VOS_NO_ERR)
    {
        retVal = SOCK_TCP_SERVER_ERR;
        printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] vos_sockClose() sockDesc Error\n");
    }
    printOut(OUTPUT_ADVANCED,"[SOCK_TCPSERVER] finished\n");
    return retVal;
}

VOS_THREAD_FUNC_T L3_test_sharedMem_write(void* arguments)
{
    TEST_ARGS_SHMEM *arg = (TEST_ARGS_SHMEM*) arguments;
    VOS_SHRD_T handle = 0;
    UINT8 *pMemArea = 0;
    UINT32 size = arg->size;
    UINT32 content = arg->content;
    VOS_ERR_T res = VOS_NO_ERR;
    VOS_ERR_T retVal = VOS_NO_ERR;
    VOS_SEMA_T sema = arg->sema;

    printOut(OUTPUT_FULL,"[SHMEM Write] start\n");
    res = vos_sharedOpen(arg->key,&handle,&pMemArea,&size);
    if (res != VOS_NO_ERR)
    {
        retVal = VOS_UNKNOWN_ERR;
        printOut(OUTPUT_ADVANCED,"[SHMEM Write] vos_sharedOpen() ERROR\n");
    }
    handle->sharedMemoryName = arg->key;
    printOut(OUTPUT_FULL,"handle->fd = %i\n",handle->fd);
    printOut(OUTPUT_FULL,"pMemArea = %x\n",(unsigned int)pMemArea);
    memcpy(pMemArea,&content,4);
    printOut(OUTPUT_FULL,"*pMemArea = %x\n",*pMemArea);
    res = vos_sharedClose(handle,pMemArea);
    if (res != VOS_NO_ERR)
    {
        retVal = VOS_UNKNOWN_ERR;
        printOut(OUTPUT_ADVANCED,"[SHMEM Write] vos_sharedClose() ERROR\n");
    }
    arg->result = retVal;
    vos_semaGive(sema);
    printOut(OUTPUT_FULL,"[SHMEM Write] finished\n");
    return 0;
}

VOS_THREAD_FUNC_T L3_test_sharedMem_read(void* arguments)
{
    TEST_ARGS_SHMEM *arg = (TEST_ARGS_SHMEM*) arguments;
    VOS_SHRD_T handle = 0;
    UINT8 *pMemArea = 0;
    UINT32 size = arg->size;
    UINT32 content = 0;
    VOS_ERR_T res = VOS_NO_ERR;
    VOS_ERR_T retVal = VOS_NO_ERR;
    VOS_SEMA_T sema = arg->sema;

    printOut(OUTPUT_FULL,"[SHMEM Read] start\n");
    res = vos_semaTake(sema,0xffffffff);

    res = vos_sharedOpen(arg->key,&handle,&pMemArea,&size);
    if (res != VOS_NO_ERR)
    {
        retVal = VOS_UNKNOWN_ERR;
        printOut(OUTPUT_ADVANCED,"[SHMEM Read] vos_sharedOpen() ERROR\n");
    }
    handle->sharedMemoryName = arg->key;
    memcpy(&content,pMemArea,4);
    printOut(OUTPUT_FULL,"pMemArea = %x\n",(unsigned int)pMemArea);
    printOut(OUTPUT_FULL,"*pMemArea = %x\n",*pMemArea);
    printOut(OUTPUT_FULL,"content = %x\n",content);
    res = vos_sharedClose(handle,pMemArea);
    if (content != arg->content)
    {
        retVal = VOS_UNKNOWN_ERR;
        printOut(OUTPUT_ADVANCED,"[SHMEM Read] vos_sharedClose() ERROR\n");
    }
    arg->result = retVal;
    vos_semaGive(sema);
    printOut(OUTPUT_FULL,"[SHMEM Read] finished\n");
    return 0;
}

SHMEM_ERR_T L3_test_sharedMem()/*--> improve / check test!*/
{
    VOS_THREAD_T writeThread = 0;
    VOS_THREAD_T readThread = 0;
    TEST_ARGS_SHMEM arg1, arg2;
    SHMEM_ERR_T retVal = SHMEM_NO_ERR;
    VOS_ERR_T ret = VOS_NO_ERR;
    VOS_ERR_T ret2 = VOS_NO_ERR;
    VOS_SHRD_T handle = 0;
    UINT8 *pMemArea = 0;
    VOS_SEMA_T sema = 0;

    arg1.content = 0x12345678;
    arg1.size = 4;
    arg2.content = arg1.content;
    arg2.size = arg1.size;
    vos_strncpy(arg1.key,"shmem1452",20);
    vos_strncpy(arg2.key,"shmem1452",20);

    ret = vos_semaCreate(&sema,VOS_SEMA_EMPTY);
    if (ret != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SHMEM] vos_semaCreate() ERROR\n");
        retVal = SHMEM_ALL_ERR;
    }
    printOut(OUTPUT_ADVANCED,"[SHMEM] start...\n");
    ret = vos_threadInit();
    if (ret != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SHMEM] vos_threadInit() ERROR\n");
        retVal = SHMEM_ALL_ERR;
    }
    ret = vos_sharedOpen(arg1.key,&handle,&pMemArea,&(arg1.size));
    if (ret != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SHMEM] vos_sharedOpen() ERROR\n");
        retVal = SHMEM_ALL_ERR;
    }
    handle->sharedMemoryName = arg1.key;
    arg1.sema = sema;
    arg2.sema = sema;
    /*vos thread error*/
    ret = vos_threadCreate(&writeThread,"writeThread",THREAD_POLICY,0,0,0,(void*)L3_test_sharedMem_write,(void*)&arg1);
    ret2 = vos_semaTake(sema,0xffffffff);
    if (ret2 != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SHMEM] vos_semaTake() ERROR\n");
        retVal = SHMEM_ALL_ERR;
    }
    if ((ret != VOS_NO_ERR)||(arg1.result != VOS_NO_ERR))
    {
        printOut(OUTPUT_ADVANCED,"[SHMEM] writeThread() ERROR\n");
        retVal = SHMEM_ALL_ERR;
    }
    vos_semaGive(sema);
    ret = vos_threadCreate(&readThread,"readThread",THREAD_POLICY,0,0,0,(void*)L3_test_sharedMem_read,(void*)&arg1);
    vos_threadDelay(50000);
    ret2 = vos_semaTake(sema,0xffffffff);
    if (ret2 != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SHMEM] vos_semaTake() ERROR\n");
        retVal = SHMEM_ALL_ERR;
    }
    if ((ret != VOS_NO_ERR)||(arg1.result != VOS_NO_ERR))
    {
        printOut(OUTPUT_ADVANCED,"[SHMEM] readThread() ERROR\n");
        retVal = SHMEM_ALL_ERR;
    }
    vos_semaDelete(sema);
    ret = vos_sharedClose(handle,pMemArea);
    if (ret != VOS_NO_ERR)
    {
        printOut(OUTPUT_ADVANCED,"[SHMEM] vos_sharedClose() ERROR\n");
        retVal = SHMEM_ALL_ERR;
    }
    printOut(OUTPUT_ADVANCED,"[SHMEM] finished \n");
    vos_threadTerm();
    return retVal;
}

UTILS_ERR_T L3_test_utils_init()
{
    VOS_ERR_T res = VOS_NO_ERR;
    UTILS_ERR_T retVal = UTILS_NO_ERR;

    printOut(OUTPUT_ADVANCED,"[UTILS_INIT] start...\n");
    res = vos_init(NULL,NULL);
    if (res != VOS_NO_ERR)
    {
        retVal = UTILS_INIT_ERR;
    }
    printOut(OUTPUT_ADVANCED,"[UTILS_INIT] finished\n");
    return retVal;
}

UTILS_ERR_T L3_test_utils_CRC()
{
    UTILS_ERR_T retVal = UTILS_NO_ERR;
    UINT8 testdata[1432];
    UINT32 length = sizeof(testdata);
    UINT32 crc = vos_crc32(0xffffffff, NULL, 0);
    UINT32 ix = 0; /*counter */

    printOut(OUTPUT_ADVANCED,"[UTILS_CRC] start...\n");
    /**********************************/
    /* init test data to get zero CRC */
    /**********************************/
    testdata[0] = 0x61;
    testdata[1] = 0x62;
    testdata[2] = 0x63;
    testdata[3] = 0xb3;
    testdata[4] = 0x99;
    testdata[5] = 0x75;
    testdata[6] = 0xca;
    testdata[7] = 0x0a;
    for (ix = 8; ix<sizeof(testdata); ix++)
    {
        testdata[ix] = 0;
    }
    crc = vos_crc32(crc, testdata, length);
    printOut(OUTPUT_FULL,"[UTILS_CRC] test memory\tCRC %x (length is %d)\n", crc, length);
    /********************/
    /* the CRC is zero! */
    /********************/
    if (crc != 0)
    {
        retVal = UTILS_CRC_ERR;
    }
    /**********************************/
    /* calculate crc for empty memory */
    /**********************************/
    memset(testdata, 0, length);
    crc = vos_crc32(0xffffffff, NULL, 0);
    crc = vos_crc32(crc, testdata, length);
    printOut(OUTPUT_FULL,"[UTILS_CRC] empty memory\tCRC %x (length is %d)\n", crc, length);
    if (crc != 0xA580609c)
    {
        retVal = UTILS_CRC_ERR;
    }
    printOut(OUTPUT_ADVANCED,"[UTILS_CRC] finished\n");
    return retVal;
}

UTILS_ERR_T L3_test_utils_terminate()
{
    /* tested with debugger, it's ok although vos_memDelete() has internal error, but that's because vos_memDelete() has been
     * executed in L2_test_mem() before and no 2nd vos_memInit() has been called. therefore memory is not initialised. */
    vos_terminate();
    return UTILS_NO_ERR;
}

MEM_ERR_T L2_test_mem()
{
    MEM_ERR_T errcnt = MEM_NO_ERR;
    printOut(OUTPUT_ADVANCED,"\n*********************************************************************\n");
    printOut(OUTPUT_ADVANCED,"*   [MEM] Test start...\n");
    printOut(OUTPUT_ADVANCED,"*********************************************************************\n");
    errcnt += L3_test_mem_init();
    errcnt += L3_test_mem_alloc();
    errcnt += L3_test_mem_queue();
    errcnt += L3_test_mem_help();
    errcnt += L3_test_mem_count();
    errcnt += L3_test_mem_delete();
    printOut(OUTPUT_ADVANCED,"\n*********************************************************************\n");
    printOut(OUTPUT_ADVANCED,"*   [MEM] Test finished with errcnt = %i\n",errcnt);
    printOut(OUTPUT_ADVANCED,"*********************************************************************\n");
    return errcnt;
}

THREAD_ERR_T L2_test_thread()
{
    THREAD_ERR_T errcnt = THREAD_NO_ERR;
    printOut(OUTPUT_ADVANCED,"\n*********************************************************************\n");
    printOut(OUTPUT_ADVANCED,"*   [THREAD] Test start...\n");
    printOut(OUTPUT_ADVANCED,"*********************************************************************\n");
    errcnt += L3_test_thread_init();
    errcnt += L3_test_thread_getTime();
    errcnt += L3_test_thread_getTimeStamp();
    errcnt += L3_test_thread_addTime();
    errcnt += L3_test_thread_subTime();
    errcnt += L3_test_thread_mulTime();
    errcnt += L3_test_thread_divTime();
    errcnt += L3_test_thread_cmpTime();
    errcnt += L3_test_thread_clearTime();
    errcnt += L3_test_thread_getUUID();
    errcnt += L3_test_thread_mutex();
    errcnt += L3_test_thread_sema();
    printOut(OUTPUT_ADVANCED,"\n*********************************************************************\n");
    printOut(OUTPUT_ADVANCED,"*   [THREAD] Test finished with errcnt = %i\n",errcnt);
    printOut(OUTPUT_ADVANCED,"*********************************************************************\n");
    return errcnt;
}

SOCK_ERR_T L2_test_sock(TEST_ROLE_T role)
{
    SOCK_ERR_T errcnt = SOCK_NO_ERR;
    UINT8 sndBufStartVal = 0x12;
    UINT8 rcvBufExpVal = 0x12;
    printOut(OUTPUT_ADVANCED,"\n*********************************************************************\n");
    printOut(OUTPUT_ADVANCED,"*   [SOCK] Test start...\n");
    printOut(OUTPUT_ADVANCED,"*********************************************************************\n");
    errcnt += L3_test_sock_init();
    errcnt += L3_test_sock_helpFunctions();
    errcnt += L3_test_sock_UDPMC(sndBufStartVal,rcvBufExpVal,role);      /* 0,1 */
    sndBufStartVal = 0x34;
    rcvBufExpVal = 0x34;
    errcnt += L3_test_sock_UDP(sndBufStartVal,rcvBufExpVal,role);        /* 2,3*/
    sndBufStartVal = 0x56;
    rcvBufExpVal = 0x56;
    errcnt += L3_test_sock_TCPclient(sndBufStartVal,rcvBufExpVal,role);  /* 4,5 */
    sndBufStartVal = 0x78;
    rcvBufExpVal = 0x78;
    errcnt += L3_test_sock_TCPserver(sndBufStartVal,rcvBufExpVal,role);  /* 7,6 */
    printOut(OUTPUT_ADVANCED,"\n*********************************************************************\n");
    printOut(OUTPUT_ADVANCED,"*   [SOCK] Test finished with errcnt = %i\n",errcnt);
    printOut(OUTPUT_ADVANCED,"*********************************************************************\n");
    return errcnt;
}

SHMEM_ERR_T L2_test_sharedMem()
{
    SHMEM_ERR_T errcnt = SHMEM_NO_ERR;
    printOut(OUTPUT_ADVANCED,"\n*********************************************************************\n");
    printOut(OUTPUT_ADVANCED,"*   [SHMEM] Test start...\n");
    printOut(OUTPUT_ADVANCED,"*********************************************************************\n");
    errcnt += L3_test_sharedMem();
    printOut(OUTPUT_ADVANCED,"\n*********************************************************************\n");
    printOut(OUTPUT_ADVANCED,"*   [SHMEM] Test finished with errcnt = %i\n",errcnt);
    printOut(OUTPUT_ADVANCED,"*********************************************************************\n");
    return errcnt;
}

UTILS_ERR_T L2_test_utils()
{
    UTILS_ERR_T errcnt = UTILS_NO_ERR;
    printOut(OUTPUT_ADVANCED,"\n*********************************************************************\n");
    printOut(OUTPUT_ADVANCED,"*   [UTILS] Test start...\n");
    printOut(OUTPUT_ADVANCED,"*********************************************************************\n");
    errcnt += L3_test_utils_init();
    errcnt += L3_test_utils_CRC();
    errcnt += L3_test_utils_terminate();
    printOut(OUTPUT_ADVANCED,"\n*********************************************************************\n");
    printOut(OUTPUT_ADVANCED,"*   [UTILS] Test finished with errcnt = %i\n",errcnt);
    printOut(OUTPUT_ADVANCED,"*********************************************************************\n");
    return errcnt;
}

void L1_testEvaluation(MEM_ERR_T memErr, THREAD_ERR_T threadErr, SOCK_ERR_T sockErr, SHMEM_ERR_T shMemErr, UTILS_ERR_T utilsErr)
{
    printOut(OUTPUT_BASIC,"\n\n\n");
    printOut(OUTPUT_BASIC,"*********************************************************************\n");
    printOut(OUTPUT_BASIC,"*                       Dev Test Evaluation                         *\n");
    printOut(OUTPUT_BASIC,"*********************************************************************\n");

    /********************************************/
    /*           vos_mem functionality          */
    /********************************************/

    printOut(OUTPUT_BASIC,"\n-> L2_test_mem err = %i\n",memErr);
    if (memErr & MEM_INIT_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," MEM_INIT\n");
    if (memErr & MEM_ALLOC_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," MEM_ALLOC\n");
    if (memErr & MEM_QUEUE_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," MEM_QUEUE\n");
    if (memErr & MEM_HELP_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," MEM_HELP\n");
    if (memErr & MEM_COUNT_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," MEM_COUNT\n");
    if (memErr & MEM_DELETE_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," MEM_DELETE\n");
    /********************************************/
    /*          vos_thread functionality        */
    /********************************************/
    printOut(OUTPUT_BASIC,"\n-> L2_test_thread err = %i\n",threadErr);
    if (threadErr & THREAD_INIT_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_INIT\n");
    if (threadErr & THREAD_GETTIME_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_GETTIME\n");
    if (threadErr & THREAD_GETTIMESTAMP_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_GETTIMESTAMP\n");
    if (threadErr & THREAD_ADDTIME_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_ADDTIME\n");
    if (threadErr & THREAD_SUBTIME_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_SUBTIME\n");
    if (threadErr & THREAD_MULTIME_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_MULTIME\n");
    if (threadErr & THREAD_DIVTIME_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_DIVTIME\n");
    if (threadErr & THREAD_CMPTIME_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_CMPTIME\n");
    if (threadErr & THREAD_CLEARTIME_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_CLEARTIME\n");
    if (threadErr & THREAD_UUID_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_UUID\n");
    if (threadErr & THREAD_MUTEX_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_MUTEX\n");
    if (threadErr & THREAD_SEMA_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," THREAD_SEMA\n");

    /********************************************/
    /*          vos_sock functionality          */
    /********************************************/

    printOut(OUTPUT_BASIC,"\n-> L2_test_sock err = %i\n",sockErr);
    if (sockErr & SOCK_INIT_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," SOCK_INIT\n");
    if (sockErr & SOCK_HELP_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," SOCK_HELPFUNCTIONS\n");
    if (sockErr & SOCK_UDP_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," SOCK_UDP\n");
    if (sockErr & SOCK_UDP_MC_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," SOCK_UDP_MC\n");
    if (sockErr & SOCK_TCP_CLIENT_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," SOCK_TCP_CLIENT\n");
    if (sockErr & SOCK_TCP_SERVER_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," SOCK_TCP_SERVER\n");

    /********************************************/
    /*       vos_sharedMem functionality        */
    /********************************************/

    printOut(OUTPUT_BASIC,"\n-> L2_test_sharedMem err = %i\n",shMemErr);
    if (shMemErr & SHMEM_ALL_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," SHMEM\n");

    /********************************************/
    /*       vos_utils functionality        */
    /********************************************/

    printOut(OUTPUT_BASIC,"\n-> L2_test_utils err = %i\n",utilsErr);
    if (utilsErr& UTILS_INIT_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," UTILS_INIT\n");
    if (utilsErr& UTILS_CRC_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," UTILS_CRC\n");
    if (utilsErr& UTILS_TERMINATE_ERR)
    {
        printOut(OUTPUT_BASIC,"[ERR]");
    }
    else
    {
        printOut(OUTPUT_BASIC,"[OK] ");
    }
    printOut(OUTPUT_BASIC," UTILS_TERMINATE\n");

    printOut(OUTPUT_BASIC,"\n");
    printOut(OUTPUT_BASIC,"*********************************************************************\n");
    printOut(OUTPUT_BASIC,"*                   Dev Test Evaluation Finished                    *\n");
    printOut(OUTPUT_BASIC,"*********************************************************************\n");
}

UINT32 L1_test_basic(UINT32 testCnt)
{
    BOOL8 mem                = TRUE;
    BOOL8 thread             = TRUE;
    BOOL8 sock               = FALSE;
    BOOL8 shMem              = FALSE;
    BOOL8 utils              = TRUE;
    MEM_ERR_T memErr        = MEM_ALL_ERR;
    THREAD_ERR_T threadErr  = THREAD_ALL_ERR;
    SOCK_ERR_T sockErr      = SOCK_ALL_ERR;
    SHMEM_ERR_T shMemErr    = SHMEM_ALL_ERR;
    UTILS_ERR_T utilsErr    = UTILS_ALL_ERR;
    UINT32 errcnt = 0;
    TEST_ROLE_T role = TEST_ROLE;

    errcnt = VOS_NO_ERR;
    printOut(OUTPUT_BASIC,"Test start\n");
    printOut(OUTPUT_BASIC,"\n\n\n");
    printOut(OUTPUT_BASIC,"#####################################################################\n");
    printOut(OUTPUT_BASIC,"#####################################################################\n");
    printOut(OUTPUT_BASIC,"#                                                                   #\n");
    printOut(OUTPUT_BASIC,"#                         Dev Test %i Start...                     #\n",testCnt);
    printOut(OUTPUT_BASIC,"#                                                                   #\n");
    printOut(OUTPUT_BASIC,"#####################################################################\n");
    printOut(OUTPUT_BASIC,"#####################################################################\n");
    if(mem == TRUE)
    {
        memErr = L2_test_mem();
        errcnt += memErr;
    };
    if(thread == TRUE)
    {
        threadErr = L2_test_thread();
        errcnt += threadErr;
    };
    if(sock == TRUE)
    {
        sockErr = L2_test_sock(role);
        errcnt += sockErr;
    };
    if(shMem == TRUE)
    {
        shMemErr = L2_test_sharedMem();
        errcnt += shMemErr;
    };
    if(utils == TRUE)
    {
        utilsErr = L2_test_utils();
        errcnt += utilsErr;
    };
    /*errcnt = (VOS_ERR_T) memErr + threadErr + sockErr + shMemErr + utilsErr;*/
    L1_testEvaluation(memErr, threadErr, sockErr, shMemErr, utilsErr);
    printOut(OUTPUT_BASIC,"\n\n\n");
    printOut(OUTPUT_BASIC,"#####################################################################\n");
    printOut(OUTPUT_BASIC,"#####################################################################\n");
    printOut(OUTPUT_BASIC,"#                                                                   #\n");
    printOut(OUTPUT_BASIC,"#                         Dev Test %i Finished                     #\n",testCnt);
    printOut(OUTPUT_BASIC,"#                                                                   #\n");
    printOut(OUTPUT_BASIC,"#####################################################################\n");
    printOut(OUTPUT_BASIC,"#####################################################################\n");
    return errcnt;
}

int main()
{
    UINT32 testCnt = 0;
    UINT32 totalErrors = 0;
    for (testCnt=0; testCnt<N_ITERATIONS; testCnt++)
    {
        totalErrors += L1_test_basic(testCnt);
    }
    printf("\n\nTOTAL ERRORS = %i\n",totalErrors);
    return 0;
}
