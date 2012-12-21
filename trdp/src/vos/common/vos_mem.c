/**********************************************************************************************************************/
/**
 * @file            vos_mem.c
 *
 * @brief           Memory functions
 *
 * @details         OS abstraction of memory access and control
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

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#ifdef TRDP_OPTION_LADDER
#include <unistd.h>
#endif /* TRDP_OPTION_LADDER */

#include "vos_types.h"
#include "vos_utils.h"
#include "vos_mem.h"
#include "vos_thread.h"
#include "vos_private.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

typedef struct memBlock
{
    UINT32          size;           /* Size of the data part of the block */
    struct memBlock *pNext;         /* Pointer to next block in linked list */
                                    /* Data area follows here */
} MEM_BLOCK_T;

typedef struct
{
    UINT32  freeSize;             /* Size of free memory */
    UINT32  minFreeSize;          /* Size of free memory */
    UINT32  allocCnt;             /* No of allocated memory blocks */
    UINT32  allocErrCnt;          /* No of allocated memory errors */
    UINT32  freeErrCnt;           /* No of free memory errors */
    UINT32  blockCnt[VOS_MEM_NBLOCKSIZES];  /* D:o per block size */
    UINT32  preAlloc[VOS_MEM_NBLOCKSIZES];  /* Pre allocated per block size */

} MEM_STATISTIC_T;

typedef struct
{
    struct VOS_MUTEX    mutex;          /* Memory allocation semaphore */
    UINT8               *pArea;         /* Pointer to start of memory area */
    UINT8               *pFreeArea;     /* Pointer to start of free part of memory area */
    UINT32              memSize;        /* Size of memory area */
    UINT32              allocSize;      /* Size of allocated area */
    UINT32              noOfBlocks;     /* No of blocks */
    BOOL                wasMalloced;    /* needs to be freed in the end */

    /* Free block header array, one entry for each possible free block size */
    struct
    {
        UINT32      size;               /* Block size */
        MEM_BLOCK_T *pFirst;            /* Pointer to first free block */
    } freeBlock[VOS_MEM_NBLOCKSIZES];
    MEM_STATISTIC_T memCnt;             /* Statistic counters */
} MEM_CONTROL_T;

typedef struct
{
    UINT32  queueAllocated;      /* No of allocated queues */
    UINT32  queueMax;            /* Maximum number of allocated queues */
    UINT32  queuCreateErrCnt;    /* No of queue create errors */
    UINT32  queuDestroyErrCnt;   /* No of queue destroy errors */
    UINT32  queuWriteErrCnt;     /* No of queue write errors */
    UINT32  queuReadErrCnt;      /* No of queue read errors */
} VOS_STATISTIC;

struct VOS_QUEUE
{
    struct VOS_QUEUE    *pNext;
    void                *pData;
    size_t              size;
};

/***********************************************************************************************************************
 *  LOCALS
 */

static MEM_CONTROL_T gMem =
{
    {0, 0}, NULL, NULL, 0L, 0L, 0L, FALSE,
    {
        {0L, NULL}, {0L, NULL}, {0L, NULL}, {0L, NULL}, {0L, NULL}, {0L, NULL}, {0L, NULL},
        {0L, NULL}, {0L, NULL}, {0L, NULL}, {0L, NULL}, {0L, NULL}, {0L, NULL}, {0L, NULL}, {0L, NULL}
    },
    {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, VOS_MEM_PREALLOCATE}
};

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */


/**********************************************************************************************************************/
/*    Memory                                                                                                          */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Initialize the memory unit.
 *  Init a supplied block of memory and prepare it for use with vos_memAlloc and vos_memFree. The used block sizes can
 *  be supplied and will be preallocated.
 *  If half of the overall size of the requested memory area would be pre-allocated, either by the default
 *  pre-allocation table or a provided one, no pre-allocation takes place.
 *
 *  @param[in]      pMemoryArea        Pointer to memory area to use
 *  @param[in]      size               Size of provided memory area
 *  @param[in]      fragMem            Pointer to list of preallocated block sizes, used to fragment memory for large blocks
 *  @retval         VOS_NO_ERR         no error
 *  @retval         VOS_PARAM_ERR      parameter out of range/invalid
 *  @retval         VOS_MEM_ERR        no memory available
 *  @retval         VOS_MUTEX_ERR      no mutex available
 */

EXT_DECL VOS_ERR_T vos_memInit (
    UINT8           *pMemoryArea,
    UINT32          size,
    const UINT32    fragMem[VOS_MEM_NBLOCKSIZES])
{
    UINT32  i, j, max;
    UINT32  minSize = 0;
    UINT32  blockSize[VOS_MEM_NBLOCKSIZES] = VOS_MEM_BLOCKSIZES;        /* Different block sizes */
    UINT8   *p[VOS_MEM_MAX_PREALLOCATE];

    /* Initialize memory */

    gMem.memSize            = size;
    gMem.allocSize          = 0;
    gMem.noOfBlocks         = 0;
    gMem.memCnt.freeSize    = size;
    gMem.memCnt.minFreeSize = size;
    gMem.memCnt.allocCnt    = 0;
    gMem.memCnt.allocErrCnt = 0;
    gMem.memCnt.freeErrCnt  = 0;

    /*  Create the memory mutex   */
    if (vos_mutexLocalCreate(&gMem.mutex) != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "vos_memInit Mutex creation failed\n");
        return VOS_MUTEX_ERR;
    }

    for (i = 0; i < VOS_MEM_MAX_PREALLOCATE; i++)
    {
        p[i] = NULL;
    }

    /*  Check if we should prealloc some memory */
    if (fragMem != NULL)
    {
        for (i = 0; i < (UINT32) VOS_MEM_NBLOCKSIZES; i++)
        {
            if (fragMem[i] != 0)
            {
                break;
            }
        }
        
        if (i < (UINT32) VOS_MEM_NBLOCKSIZES)
        {
            for (i = 0; i < (UINT32) VOS_MEM_NBLOCKSIZES; i++)
            {
                gMem.memCnt.preAlloc[i] = fragMem[i];
                minSize += gMem.memCnt.preAlloc[i] * blockSize[i];
            }
        }   
    }

    if (pMemoryArea == NULL && size == 0)       /* This means we will use standard malloc calls    */
    {
        gMem.noOfBlocks = 0;
        gMem.memSize    = 0;
        gMem.pArea      = NULL;
        return VOS_NO_ERR;
    }

    if (pMemoryArea == NULL && size != 0)       /* We must allocate memory from the heap once   */
    {
        gMem.pArea = (UINT8 *) malloc(size);
        if (gMem.pArea == NULL)
        {
            return VOS_MEM_ERR;
        }
        gMem.wasMalloced = TRUE;
    }

    /*  Can we pre-allocate the memory? If more than half of the memory would be occupied, we don't even try...  */
    if (minSize > size / 2)
    {
        for (i = 0; i < (UINT32) VOS_MEM_NBLOCKSIZES; i++)
        {
            gMem.memCnt.preAlloc[i] = 0;
        }
        vos_printf(VOS_LOG_INFO, "vos_memInit Pre-Allocation disabled\n");
    }

    minSize = 0;

    gMem.pFreeArea  = gMem.pArea;
    gMem.noOfBlocks = (UINT32) VOS_MEM_NBLOCKSIZES;
    gMem.memSize    = size;

    /* Initialize free block headers */
    for (i = 0; i < (UINT32) VOS_MEM_NBLOCKSIZES; i++)
    {
        gMem.freeBlock[i].pFirst    = (MEM_BLOCK_T *)NULL;
        gMem.freeBlock[i].size      = blockSize[i];
        max     = gMem.memCnt.preAlloc[i];
        minSize += blockSize[i];

        if (max > VOS_MEM_MAX_PREALLOCATE)
        {
            max = VOS_MEM_MAX_PREALLOCATE;
        }

        for (j = 0; j < max; j++)
        {
            p[j] = vos_memAlloc(blockSize[i]);
            if (p[j] == NULL)
            {
                vos_printf(VOS_LOG_ERROR, "vos_memInit Pre-Allocation size exceeds overall memory size!!! (%u > %u)\n",
                           minSize, size);
                break;
            }
        }

        for (j = 0; j < max; j++)
        {
            if (p[j])
            {
                vos_memFree(p[j]);
            }
        }
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete the memory area.
 *    This will eventually invalidate any previously allocated memory blocks! It should be called last before the
 *  application quits. No further access to the memory blocks is allowed after this call.
 *
 *  @param[in]      pMemoryArea        Pointer to memory area to use
 *  @retval         VOS_NO_ERR         no error
 *  @retval         VOS_INIT_ERR       module not initialised
 *  @retval         VOS_PARAM_ERR      parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_memDelete (
    UINT8 *pMemoryArea)
{
    if (pMemoryArea == gMem.pArea)
    {
        vos_mutexLocalDelete(&gMem.mutex);
        if (gMem.wasMalloced)
        {
            free(pMemoryArea);
        }
        memset(&gMem, 0, sizeof(gMem));
        return VOS_NO_ERR;
    }
    return VOS_PARAM_ERR;
}

/**********************************************************************************************************************/
/** Allocate a block of memory (from memory area above).
 *
 *  @param[in]      size            Size of requested block
 *
 *  @retval         Pointer to memory area
 *  @retval         NULL if no memory available
 */

EXT_DECL UINT8 *vos_memAlloc (
    UINT32 size)
{
    UINT32      i, blockSize;
    MEM_BLOCK_T *pBlock;

    if (size == 0)
    {
        gMem.memCnt.allocErrCnt++;
        vos_printf(VOS_LOG_ERROR, "vos_memAlloc Requested size = %u\n", size);
        return NULL;
    }

    /*    Use standard heap memory    */
    if (gMem.memSize == 0 && gMem.pArea == NULL)
    {
        UINT8 *p = malloc(size);
        if (p != NULL)
        {
            memset(p, 0, size);
            vos_printf(VOS_LOG_DBG, "vos_memAlloc %p Requested size = 0x%x\n", p, size);
        }
        return p;
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

        vos_printf(VOS_LOG_ERROR, "vos_memAlloc No block size big enough. Requested size=%d\n", size);

        return NULL; /* No block size big enough */
    }

    /* Get memory sempahore */
    if (vos_mutexLock(&gMem.mutex) != VOS_NO_ERR)
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
        if ((gMem.allocSize + blockSize + sizeof(MEM_BLOCK_T)) < gMem.memSize)
        {
            pBlock = (MEM_BLOCK_T *) gMem.pFreeArea; /*lint !e826 Allocation of MEM_BLOCK from free area*/

            gMem.pFreeArea  = (UINT8 *) gMem.pFreeArea + (sizeof(MEM_BLOCK_T) + blockSize);
            gMem.allocSize  += blockSize + sizeof(MEM_BLOCK_T);
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
    if (vos_mutexUnlock(&gMem.mutex) != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "IPTVosPutSemR ERROR\n");
    }

    if (pBlock != NULL)
    {
        /* Fill in size in memory header of the block. To be used when it is returned.*/
        pBlock->size            = blockSize;
        gMem.memCnt.freeSize    -= blockSize + sizeof(MEM_BLOCK_T);
        if (gMem.memCnt.freeSize < gMem.memCnt.minFreeSize)
        {
            gMem.memCnt.minFreeSize = gMem.memCnt.freeSize;
        }
        gMem.memCnt.allocCnt++;

        /* Clear returned memory area to be compliant with malloc'ed version */
        memset((UINT8 *) pBlock + sizeof(MEM_BLOCK_T), 0, blockSize);

        /* Return pointer to data area, not the memory block itself */
        return (UINT8 *) pBlock + sizeof(MEM_BLOCK_T);
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
 *  @param[in]      pMemBlock         Pointer to memory block to be freed
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_INIT_ERR      module not initialised
 *  @retval         VOS_PARAM_ERR     parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_memFree (
    void *pMemBlock)
{
    VOS_ERR_T   ret = VOS_NO_ERR;
    UINT32      i;
    UINT32      blockSize;
    MEM_BLOCK_T *pBlock;

    /* Param check */
    if (pMemBlock == NULL)
    {
        gMem.memCnt.freeErrCnt++;
        vos_printf(VOS_LOG_ERROR, "vos_memFree ERROR NULL pointer\n");
        return VOS_PARAM_ERR;
    }

    /*    Use standard heap memory    */
    if (gMem.memSize == 0 && gMem.pArea == NULL)
    {
        vos_printf(VOS_LOG_DBG, "vos_memFree %p\n", pMemBlock);
        free(pMemBlock);
        return VOS_NO_ERR;
    }

    /* Check that the returned memory is within the allocated area */
    if (((UINT8 *)pMemBlock < gMem.pArea) ||
        ((UINT8 *)pMemBlock >= (gMem.pArea + gMem.memSize)))
    {
        gMem.memCnt.freeErrCnt++;
        vos_printf(VOS_LOG_ERROR, "vos_memFree ERROR returned memory not within allocated memory\n");
        return VOS_PARAM_ERR;
    }

    /* Get memory sempahore */
    if (vos_mutexLock(&gMem.mutex) != VOS_NO_ERR)
    {
        gMem.memCnt.freeErrCnt++;

        vos_printf(VOS_LOG_ERROR, "vos_memFree can't get semaphore\n");
        return VOS_MEM_ERR;
    }

    /* Set block pointer to start of block, before the returned pointer */
    pBlock      = (MEM_BLOCK_T *) ((UINT8 *) pMemBlock - sizeof(MEM_BLOCK_T));
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
        gMem.memCnt.freeSize += blockSize + sizeof(MEM_BLOCK_T);
        gMem.memCnt.allocCnt--;

        /* Put the returned block first in the linked list */
        pBlock->pNext = gMem.freeBlock[i].pFirst;
        gMem.freeBlock[i].pFirst = pBlock;

        /* Destroy the size first in the block. If user tries to return same memory this will then fail. */
        pBlock->size = 0;
    }

    /* Release semaphore */
    if(vos_mutexUnlock(&gMem.mutex) != VOS_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "vos_memFree can't release semaphore\n");
    }

    return ret;
}



/**********************************************************************************************************************/
/** Return used and available memory (of memory area above).
 *
 *  @param[out]     pAllocatedMemory    Pointer to allocated memory size
 *  @param[out]     pFreeMemory         Pointer to free memory size
 *  @param[out]     pMinFree            Pointer to minimal free memory size in statistics interval
 *  @param[out]     pNumAllocBlocks     Pointer to number of allocated memory blocks
 *  @param[out]     pNumAllocErr        Pointer to number of allocation errors
 *  @param[out]     pNumFreeErr         Pointer to number of free errors
 *  @param[out]     allocBlockSize      Pointer to list of allocated memory blocks
 *  @param[out]     usedBlockSize       Pointer to list of used memoryblocks
 *  @retval         VOS_NO_ERR          no error
 *  @retval         VOS_INIT_ERR        module not initialised
 *  @retval         VOS_PARAM_ERR       parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_memCount (
    UINT32  *pAllocatedMemory,
    UINT32  *pFreeMemory,
    UINT32  *pMinFree,
    UINT32  *pNumAllocBlocks,
    UINT32  *pNumAllocErr,
    UINT32  *pNumFreeErr,
    UINT32  allocBlockSize[VOS_MEM_NBLOCKSIZES],
    UINT32  usedBlockSize[VOS_MEM_NBLOCKSIZES])
{
    UINT32 i;

    *pAllocatedMemory   = gMem.memSize;
    *pFreeMemory        = gMem.memCnt.freeSize;
    *pMinFree           = gMem.memCnt.minFreeSize;
    *pNumAllocBlocks    = gMem.memCnt.allocCnt;
    *pNumAllocErr       = gMem.memCnt.allocErrCnt;
    *pNumFreeErr        = gMem.memCnt.freeErrCnt;

    for (i = 0; i < (UINT32) VOS_MEM_NBLOCKSIZES; i++)
    {
        usedBlockSize[i]    = gMem.memCnt.blockCnt[i];
        allocBlockSize[i]   = gMem.memCnt.preAlloc[i];
    }

    return VOS_INIT_ERR;
}


/**********************************************************************************************************************/
/** Sort an array.
 *  This is just a wrapper for the standard qsort function.
 *
 *  @param[in,out]  pBuf            Pointer to the array to sort
 *  @param[in]      num             number of elements
 *  @param[in]      size            size of one element
 *  @param[in]      compare         Pointer to compare function
 *                                      return -n if arg1 < arg2,
 *                                      return 0  if arg1 == arg2,
 *                                      return +n if arg1 > arg2
 *                                  where n is an integer != 0
 *  @retval         none
 */

EXT_DECL void vos_qsort (
    void        *pBuf,
    UINT32      num,
    UINT32      size,
    int         (*compare)(
        const   void *,
        const   void *))
{
    qsort(pBuf, num, size, compare);
}

#ifdef TRDP_OPTION_LADDER
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
	VOS_ERR_T ret = VOS_MEM_ERR;
	mode_t PERMISSION = 0666;				/* Shared Memory permission is rw-rw-rw- */
	static INT32 fd;						/* Shared Memory file descriptor */
	struct	stat sharedMemoryStat;			/* Shared Memory Stat */

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
	(*pHandle)->fd = fd;

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
	if (shm_unlink(handle->semaphoreName) == -1)
	{
		vos_printf(VOS_LOG_ERROR, "Shared Memory unLink failed\n");
		return VOS_MEM_ERR;
	}
    return VOS_NO_ERR;
}
#endif /* TRDP_OPTION_LADDER */

/**********************************************************************************************************************/
/** Binary search in a sorted array.
 *  This is just a wrapper for the standard bsearch function.
 *
 *  @param[in]      pKey            Key to search for
 *  @param[in]      pBuf            Pointer to the array to sort
 *  @param[in]      num             number of elements
 *  @param[in]      size            size of one element
 *  @param[in]      compare         Pointer to compare function
 *                                      return -n if arg1 < arg2,
 *                                      return 0  if arg1 == arg2,
 *                                      return +n if arg1 > arg2
 *                                  where n is an integer != 0
 *  @retval         Pointer to found element or NULL
 */

EXT_DECL void *vos_bsearch (
    const void  *pKey,
    const void  *pBuf,
    UINT32      num,
    UINT32      size,
    int         (*compare)(
        const   void *,
        const   void *))
{
    return bsearch(pKey, pBuf, num, size, compare);
}
