/**********************************************************************************************************************/
/**
 * @file            vos_mem.c
 *
 * @brief           Memory functions
 *
 * @details			OS abstraction of memory access and control
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
    "You are trying to compile the POSIX implementation of vos_memory.c - either define POSIX or exclude this file!"
#endif

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "vos_types.h"
#include "vos_utils.h"
#include "vos_mem.h"
#include "vos_thread.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

struct memBlock
{
    UINT32          size;           /* Size of the data part of the block */
    struct memBlock *pNext;         /* Pointer to next block in linked list */
                                    /* Data area follows here */
};
typedef struct memBlock MEM_BLOCK;

typedef struct
{
    UINT32  freeSize;             /* Size of free memory */
    UINT32  minFreeSize;          /* Size of free memory */
    UINT32  allocCnt;             /* No of allocated memory blocks */
    UINT32  allocErrCnt;          /* No of allocated memory errors */
    UINT32  freeErrCnt;           /* No of free memory errors */
    UINT32  blockCnt[VOS_MEM_NBLOCKSIZES];  /* D:o per block size */

} MEM_STATISTIC;

typedef struct
{
    VOS_SEMA_T  sem;                 /* Memory allocation semaphore */
    UINT8       *pArea;            /* Pointer to start of memory area */
    UINT8       *pFreeArea;        /* Pointer to start of free part of memory area */
    UINT32      memSize;          /* Size of memory area */
    UINT32      allocSize;        /* Size of allocated area */
    UINT32      noOfBlocks;       /* No of blocks */

    /* Free block header array, one entry for each possible free block size */
    struct
    {
        UINT32      size;          /* Block size */
        MEM_BLOCK   *pFirst;       /* Pointer to first free block */
    } freeBlock[VOS_MEM_NBLOCKSIZES];
    MEM_STATISTIC memCnt;         /* Statistic counters */
} MEM_CONTROL;

typedef struct
{
    UINT32  queueAllocated;      /* No of allocated queues */
    UINT32  queueMax;            /* Maximum number of allocated queues */
    UINT32  queuCreateErrCnt;    /* No of queue create errors */
    UINT32  queuDestroyErrCnt;   /* No of queue destroy errors */
    UINT32  queuWriteErrCnt;     /* No of queue write errors */
    UINT32  queuReadErrCnt;      /* No of queue read errors */
} VOS_STATISTIC;

struct VOS_QUEUE_T
{
    struct VOS_QUEUE_T  *pNext;
    void                *pData;
    size_t              size;
};

/***********************************************************************************************************************
 *  LOCALS
 */

static UINT8        *gMemoryArea    = NULL;
static size_t       gMemorySize     = 0;
static MEM_CONTROL  gMem;

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */


/**********************************************************************************************************************/
/*	Memory																										      */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Initialize the memory unit.
 *  Init a supplied block of memory and prepare it for use with vos_alloc and vos_dealloc. The used block sizes can
 *	be supplied and will be preallocated.
 *
 *  @param[in]      pMemoryArea		Pointer to memory area to use
 *  @param[in]      size			Size of provided memory area
 *  @param[in]      fragMem			Pointer to list of preallocated block sizes, used to fragment memory for large blocks
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 *  @retval         VOS_MEM_ERR		no memory available
 */

EXT_DECL VOS_ERR_T vos_memInit (
    UINT8           *pMemoryArea,
    UINT32          size,
    const UINT32    fragMem[VOS_MEM_NBLOCKSIZES])
{
    int     i, j, max;
    UINT32  blockSize[VOS_MEM_NBLOCKSIZES]      = VOS_MEM_BLOCKSIZES;   /* Different block sizes */
    UINT32  preAllocate[VOS_MEM_NBLOCKSIZES]    = VOS_MEM_PREALLOCATE;  /* No of blocks that should be pre-allocated */
    unsigned char *p[VOS_MEM_MAX_PREALLOCATE];

    /* TBD: This code is not working, use heap memory allocation */
    return VOS_NO_ERR;

    if (fragMem != NULL)
    {
        for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
        {
            preAllocate[i] = fragMem[i];
        }
    }

    if (pMemoryArea == NULL && size == 0)       /* This means we will use standard malloc calls	*/
    {
        gMem.noOfBlocks = 0;
        gMemorySize     = 0;
        gMemoryArea     = NULL;
        return VOS_NO_ERR;
    }

    if (pMemoryArea == NULL && size != 0)       /* We must allocate memory from the heap	*/
    {
        gMemoryArea = (UINT8 *) malloc(size);
        if (gMemoryArea == NULL)
        {
            return VOS_MEM_ERR;
        }
    }

    gMem.noOfBlocks = VOS_MEM_NBLOCKSIZES;
    gMemorySize     = size;

    /* Initialize free block headers */
    for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
    {
        gMem.freeBlock[i].pFirst    = (MEM_BLOCK *)NULL;
        gMem.freeBlock[i].size      = blockSize[i];
        max = preAllocate[i];

        if (max > VOS_MEM_MAX_PREALLOCATE)
        {
            max = VOS_MEM_MAX_PREALLOCATE;
        }

        for (j = 0; j < max; j++)
        {
            p[j] = vos_memAlloc(blockSize[i]);
        }

        for (j = 0; j < max; j++)
        {
            vos_memFree(p[j]);
        }
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete the memory area.
 *	This will eventually invalidate any previously allocated memory blocks! It should be called last before the
 *  application quits. No further access to the memory blocks is allowed after this call.
 *
 *  @param[in]      pMemoryArea		Pointer to memory area to use
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_memDelete (
    UINT8 *pMemoryArea)
{
    /* TBD: This code is not working, use heap memory allocation */
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Allocate a block of memory (from memory area above).
 *
 *  @param[in]      size			Size of requested block
 *
 *  @retval         Pointer to memory area
 *  @retval         NULL if no memory available
 */

EXT_DECL UINT8 *vos_memAlloc (
    UINT32 size)
{
    UINT32      i, blockSize;
    MEM_BLOCK   *pBlock;

    /* TBD: This code is not working, use heap memory allocation */
    /*	Use standard heap memory	*/
    if (gMemorySize == 0 && gMemoryArea == NULL)
    {
        UINT8 *p = malloc(size);
        if (p != NULL)
        {
            memset(p, 0, size);
        }
        return p;
    }

    if (!size)
    {
        return (UINT8 *)NULL; /* No block size big enough */
    }

    /* Adjust size to get one which is a multiple of UINT32's */
    size = ((size + sizeof(UINT32) - 1) / sizeof(UINT32)) * sizeof(UINT32);

    /* Find appropriate blocksize */
    for (i = 0; i < gMem.noOfBlocks; i++)
    {
        if (size <= gMem.freeBlock[i].size)
        {
            break;
        }
    }

    if (i >= gMem.noOfBlocks)
    {
        gMem.memCnt.allocErrCnt++;

        vos_printf(VOS_LOG_ERROR,
                   "vos_memAlloc No block size big enough. Requested size=%d\n",
                   size);

        return NULL; /* No block size big enough */
    }

    /* Get memory sempahore */
    if (vos_semaTake(gMem.sem, 0) != VOS_NO_ERR)
    {
        gMem.memCnt.allocErrCnt++;

        vos_printf(VOS_LOG_ERROR, "vos_memAlloc can't get semaphore\n");

        return NULL;
    }

    blockSize   = gMem.freeBlock[i].size;
    pBlock      = gMem.freeBlock[i].pFirst;

    /* Check if there is a free block ready */
    if (pBlock != NULL)
    {
        /* There is, get it. */
        /* Set start pointer to next free block in the linked list */
        gMem.freeBlock[i].pFirst = pBlock->pNext;
    }
    else
    {
        /* There was no suitable free block, create one from the free area */

        /* Enough free memory left ? */
        if ((gMem.allocSize + blockSize + sizeof(MEM_BLOCK)) < gMem.memSize)
        {
            pBlock = (MEM_BLOCK *) gMem.pFreeArea; /*lint !e826 Allocation of MEM_BLOCK from free area*/

            gMem.pFreeArea = (UINT8 *) gMem.pFreeArea +
                (sizeof(MEM_BLOCK) + blockSize);
            gMem.allocSize += blockSize + sizeof(MEM_BLOCK);
            gMem.memCnt.blockCnt[i]++;
        }
        else
        {
            while ((++i < gMem.noOfBlocks) && (pBlock == NULL))
            {
                pBlock = gMem.freeBlock[i].pFirst;
                if (pBlock != NULL)
                {
                    vos_printf(
                        VOS_LOG_ERROR,
                        "IPTVosMalloc Used a bigger buffer size=%d asked size=%d\n",
                        gMem.freeBlock[i].size,
                        size);
                    /* There is, get it. */
                    /* Set start pointer to next free block in the linked list */
                    gMem.freeBlock[i].pFirst = pBlock->pNext;

                    blockSize = gMem.freeBlock[i].size;

                }
            }
            if (pBlock == NULL)
            {
                /* Not enough memory */
                vos_printf(VOS_LOG_ERROR, "IPTVosMalloc Not enough memory\n");
            }
        }
    }

    /* Release semaphore */
    if (vos_semaGive(gMem.sem) != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "IPTVosPutSemR ERROR\n");
    }

    if (pBlock != NULL)
    {
        /* Fill in size in memory header of the block. To be used when it is returned.*/
        pBlock->size            = blockSize;
        gMem.memCnt.freeSize    -= blockSize + sizeof(MEM_BLOCK);
        if (gMem.memCnt.freeSize < gMem.memCnt.minFreeSize)
        {
            gMem.memCnt.minFreeSize = gMem.memCnt.freeSize;
        }
        gMem.memCnt.allocCnt++;

        /* Return pointer to data area, not the memory block itself */
        return (UINT8 *) pBlock + sizeof(MEM_BLOCK);
    }
    else
    {
        gMem.memCnt.allocErrCnt++;
        return NULL;
    }
}


/**********************************************************************************************************************/
/** Deallocate a block of memory (from memory area above).
 *
 *  @param[in]      pMemBlock		Pointer to memory block to be freed
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_memFree (
    void *pMemBlock)
{
    int         ret = VOS_NO_ERR;
    UINT32      i;
    UINT32      blockSize;
    MEM_BLOCK   *pBlock;

    /* TBD: This code is not working, use heap memory allocation */
    /*	Use standard heap memory	*/
    if (gMemorySize == 0 && gMemoryArea == NULL)
    {
        free(pMemBlock);
        return VOS_NO_ERR;
    }

    if (pMemBlock == NULL)
    {
        gMem.memCnt.freeErrCnt++;

        vos_printf(VOS_LOG_ERROR, "vos_memFree ERROR NULL pointer\n");
        return VOS_PARAM_ERR;
    }

    /* Check that the returned memory is within the allocated area */
    if (((UINT8 *)pMemBlock < gMem.pArea) ||
        ((UINT8 *)pMemBlock >= (gMem.pArea + gMem.memSize)))
    {
        gMem.memCnt.freeErrCnt++;

        vos_printf(
            VOS_LOG_ERROR,
            "vos_memFree ERROR returned memory not within allocated memory\n");
        return VOS_PARAM_ERR;
    }

    /* Get memory sempahore */
    if (vos_semaTake(gMem.sem, 0) != VOS_NO_ERR)
    {
        gMem.memCnt.freeErrCnt++;

        vos_printf(VOS_LOG_ERROR, "vos_memFree can't get semaphore\n");
        return VOS_MEM_ERR;
    }

    /* Set block pointer to start of block, before the returned pointer */
    pBlock      = (MEM_BLOCK *) ((UINT8 *) pMemBlock - sizeof(MEM_BLOCK));
    blockSize   = pBlock->size;

    /* Find appropriate free block item */
    for (i = 0; i < gMem.noOfBlocks; i++)
    {
        if (blockSize == gMem.freeBlock[i].size)
        {
            break;
        }
    }

    if (i >= gMem.noOfBlocks)
    {
        gMem.memCnt.freeErrCnt++;

        vos_printf(VOS_LOG_ERROR, "vos_memFree illegal sized memory\n");
        ret = VOS_MEM_ERR;   /* Try to return a illegal sized memory */
    }
    else
    {
        gMem.memCnt.freeSize += blockSize + sizeof(MEM_BLOCK);
        gMem.memCnt.allocCnt--;
        /* Put the returned block first in the linked list */
        pBlock->pNext = gMem.freeBlock[i].pFirst;
        gMem.freeBlock[i].pFirst = pBlock;

        /* Destroy the size first in the block. If user tries to return same memory this will then fail. */
        pBlock->size = 0;
    }

    /* Release semaphore */
    if(vos_semaGive(gMem.sem) != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "vos_memFree can't release semaphore\n");
    }

    return ret;
}



/**********************************************************************************************************************/
/** Return used and available memory (of memory area above).
 *
 *  @param[out]     pAllocatedMemory	Pointer to allocated memory size
 *  @param[out]     pFreeMemory			Pointer to free memory size
 *  @param[out]     pFragMem			Pointer to list of used memoryblocks
 *  @retval         VOS_NO_ERR			no error
 *  @retval         VOS_INIT_ERR	    module not initialised
 *  @retval         VOS_PARAM_ERR		parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_memCount (
    UINT32  *pAllocatedMemory,
    UINT32  *pFreeMemory,
    UINT32  *pFragMem[VOS_MEM_NBLOCKSIZES])
{
    /* TBD: This code is not working, use heap memory allocation */
    vos_printf(VOS_LOG_ERROR,
               "vos_memCount not implemented\n");
    return VOS_INIT_ERR;
}


/**********************************************************************************************************************/
/*	Queues																										      */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Initialize a message queue.
 *  Returns a handle for further calls
 *
 *  @param[in]      pKey			Unique identifier (file name)
 *  @param[out]     pQueueID		Pointer to returned queue handle
 *  @param[in]      maxNoMsg		maximum number of messages
 *  @param[in]      maxLength		maximum size of one messages
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_NOINIT_ERR	invalid handle
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 *  @retval         VOS_INIT_ERR	not supported
 *  @retval         VOS_QUEUE_ERR	error creating queue
 */

EXT_DECL VOS_ERR_T vos_queueCreate (
    const CHAR8 *pKey,
    VOS_QUEUE_T *pQueueID,
    UINT32      maxNoMsg,
    UINT32      maxLength)
{
    /* TBD      */
    vos_printf(VOS_LOG_ERROR,
               "vos_queueCreate not implemented\n");
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Destroy a message queue.
 *  Free all resources used by this queue
 *
 *  @param[in]      queueID			Queue handle
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_NOINIT_ERR	invalid handle
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_queueDestroy (
    VOS_QUEUE_T queueID)
{
    /* TBD      */
    vos_printf(VOS_LOG_ERROR,
               "vos_queue function not implemented\n");
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Send a message.
 *
 *
 *  @param[in]      queueID			Queue handle
 *  @param[in]      pMsg			Pointer to message to be sent
 *  @param[in]      size			Message size
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_NOINIT_ERR	invalid handle
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 *  @retval         VOS_QUEUE_FULL	queue is full
 */

EXT_DECL VOS_ERR_T vos_queueSend (
    VOS_QUEUE_T queueID,
    const UINT8 *pMsg,
    UINT32      size)
{
    /* TBD      */
    vos_printf(VOS_LOG_ERROR,
               "vos_queue function not implemented\n");
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Get a message.
 *
 *
 *  @param[in]      queueID			Queue handle
 *  @param[out]     pMsg			Pointer to message to be received
 *  @param[in,out]  pSize			Pointer to max. message size on entry, actual size on exit
 *  @param[in]      usTimeout		Maximum time to wait for a message in usec
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_NOINIT_ERR	invalid handle
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 *  @retval         VOS_QUEUE_ERR	queue is empty
 */

EXT_DECL VOS_ERR_T vos_queueReceive (
    VOS_QUEUE_T queueID,
    UINT8       *pMsg,
    UINT32      *pSize,
    UINT32      usTimeout)
{
    /* TBD      */
    vos_printf(VOS_LOG_ERROR,
               "vos_queue function not implemented\n");
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/*	Shared memory																										      */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Create a shared memory area or attach to existing one.
 *  The first call with the a specified key will create a shared memory area with the supplied size and will return
 *  a handle and a pointer to that area. If the area already exists, the area will be attached.
 *	This function is not available in each target implementation.
 *
 *  @param[in]      pKey			Unique identifier (file name)
 *  @param[out]     pHandle			Pointer to returned handle
 *  @param[out]     ppMemoryArea	Pointer to pointer to memory area
 *  @param[in,out]  pSize			Pointer to size of area to allocate, on return actual size after attach
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_NOINIT_ERR	invalid handle
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 *  @retval         VOS_MEM_ERR		no memory available
 */

EXT_DECL VOS_ERR_T vos_sharedOpen (
    const CHAR8 *pKey,
    VOS_SHRD_T  *pHandle,
    UINT8       * *ppMemoryArea,
    UINT32      *pSize)
{
    /* TBD      */
    vos_printf(VOS_LOG_ERROR,
               "vos_shared function not implemented\n");
    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Close connection to the shared memory area.
 *  If the area was created by the calling process, the area will be closed (freed). If the area was attached,
 *  it will be detached.
 *	This function is not available in each target implementation.
 *
 *  @param[in]      handle			Returned handle
 *  @param[in]      pMemoryArea		Pointer to memory area
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_INIT_ERR	module not initialised
 *  @retval         VOS_NOINIT_ERR	invalid handle
 *  @retval         VOS_PARAM_ERR	parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_sharedClose (
    VOS_SHRD_T  handle,
    const UINT8 *pMemoryArea)
{
    /* TBD      */
    vos_printf(VOS_LOG_ERROR,
               "vos_shared function not implemented\n");
    return VOS_NO_ERR;
}
