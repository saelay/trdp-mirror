/**********************************************************************************************************************/
/**
 * @file            trdp_marshall.c
 *
 * @brief           Marshalling functions for TRDP
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

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_types.h"
#include "trdp_if_light.h"
#include "trdp_utils.h"
#include "vos_mem.h"

#include "tau_marshall.h"

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Marshalling info, used to and from wire */
typedef struct
{
    INT32       level;
    UINT8       *pSrc;      /**< source pointer          */
    UINT8       *pDst;      /**< destination pointer     */
    UINT8       *pDstEnd;   /**< last destination        */
} TAU_MARSHALL_INFO_T;

/* structure type definitions for alignment calculation */ 
typedef struct 
{
    UINT8 a;
} STRUCT_T;

typedef struct 
{
    TIMEDATE48 a;
} TIMEDATE48_STRUCT_T;

typedef struct 
{
    TIMEDATE64 a;
} TIMEDATE64_STRUCT_T;


/***********************************************************************************************************************
 * LOCALS
 */

static TRDP_COMID_DSID_MAP_T   *sComIdDsIdMap  = NULL;
static UINT32                  sNumComId = 0;

static TRDP_DATASET_T          **sDataSets  = NULL;
static UINT32                  sNumEntries = 0;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

/**********************************************************************************************************************/
/**    Align a pointer to the next natural address.
 *
 *
 *  @param[in]      pSrc            Pointer to align
 *
 *  @retval         aligned pointer
 */
static INLINE UINT8 *alignePtr(
    const UINT8 *pSrc,
    UINT32      alignment)
{
    alignment--;

    return (UINT8 *) (((UINT32) pSrc + alignment) & ~alignment);
}

/**********************************************************************************************************************/
/**    Copy a variable to its natural address.
 *
 *
 *  @param[in,out]      ppSrc           Pointer to pointer to source variable
 *  @param[in,out]      ppDst           Pointer to pointer to destination variable
 *  @param[in]          noOfItems       Items to copy
 *
 *  @retval         none
 */
static INLINE void unpackedCopy64 (
    UINT8   * *ppSrc,
    UINT8   * *ppDst,
    UINT32  noOfItems)

#if __BIG_ENDIAN__
{
    UINT32  size    = noOfItems * sizeof(UINT64);
    UINT8   *pDst8  = alignePtr(*ppDst, ALIGNOF(UINT64));
    memcpy(pDst8, *ppSrc, size);
    *ppSrc  = (UINT8 *) *ppSrc + size;
    *ppDst  = (UINT8 *) pDst8 + size;
}
#elif __LITTLE_ENDIAN__
{
    UINT8   *pDst8 = (UINT8 *) alignePtr(*ppDst, ALIGNOF(UINT64));
    UINT8   *pSrc8 = *ppSrc;
    while (noOfItems--)
    {
        *pDst8++ = *(pSrc8 + 7);
        *pDst8++ = *(pSrc8 + 6);
        *pDst8++ = *(pSrc8 + 5);
        *pDst8++ = *(pSrc8 + 4);
        *pDst8++ = *(pSrc8 + 3);
        *pDst8++ = *(pSrc8 + 2);
        *pDst8++ = *(pSrc8 + 1);
        *pDst8++ = *pSrc8;
        pSrc8 += 8;
    }
    *ppSrc = (UINT8 *) pSrc8;
    *ppDst = (UINT8 *) pDst8;
}
#else
{
/*    #error "Endianess not defined" */
}
#endif

/**********************************************************************************************************************/
/**    Copy a variable from its natural address.
 *
 *
 *  @param[in,out]      ppSrc           Pointer to pointer to source variable
 *  @param[in,out]      ppDst           Pointer to pointer to destination variable
 *  @param[in]          noOfItems       Items to copy
 *
 *  @retval         none
 */

static INLINE void packedCopy64 (
    UINT8   * *ppSrc,
    UINT8   * *ppDst,
    UINT32  noOfItems)

{
    UINT64 *pSrc64 = (UINT64 *) alignePtr(*ppSrc, ALIGNOF(UINT64));
    while (noOfItems--)
    {
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 56);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 48);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 40);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 32);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 24);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 16);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 8);
        *(*ppDst)++ = (UINT8) (*pSrc64 & 0xFF);
        pSrc64++;
    }
    *ppSrc = (UINT8 *) pSrc64;
}

/**********************************************************************************************************************/
/**    Dataset compare function
 *
 *  @param[in]      pArg1        Pointer to first element
 *  @param[in]      pArg2        Pointer to second element
 *
 *  @retval         -1 if arg1 < arg2
 *  @retval          0 if arg1 == arg2
 *  @retval          1 if arg1 > arg2
 */
static int dataset_compare (
    const void  *pArg1,
    const void  *pArg2)
{
    TRDP_DATASET_T    *p1 = *(TRDP_DATASET_T **)pArg1;
    TRDP_DATASET_T    *p2 = *(TRDP_DATASET_T **)pArg2;

    if (p1->id < p2->id)
    {
        return -1;
    }
    else if (p1->id > p2->id)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**********************************************************************************************************************/
/**    Dataset compare function
 *
 *  @param[in]      pArg1        Pointer to key
 *  @param[in]      pArg2        Pointer to array element
 *
 *  @retval         -1 if arg1 < arg2
 *  @retval          0 if arg1 == arg2
 *  @retval          1 if arg1 > arg2
 */
static int dataset_compare_deref (
    const void  *pArg1,
    const void  *pArg2)
{
    TRDP_DATASET_T    *p1 = (TRDP_DATASET_T *)pArg1;
    TRDP_DATASET_T    *p2 = *(TRDP_DATASET_T **)pArg2;

    if (p1->id < p2->id)
    {
        return -1;
    }
    else if (p1->id > p2->id)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**********************************************************************************************************************/
/**    ComId/dataset mapping compare function
 *
 *  @param[in]      pArg1        Pointer to first element
 *  @param[in]      pArg2        Pointer to second element
 *
 *  @retval         -1 if arg1 < arg2
 *  @retval          0 if arg1 == arg2
 *  @retval          1 if arg1 > arg2
 */
static int comId_compare (
    const void  *pArg1,
    const void  *pArg2)
{
    if (((TRDP_COMID_DSID_MAP_T *)pArg1)->comId < ((TRDP_COMID_DSID_MAP_T *)pArg2)->comId)
    {
        return -1;
    }
    else if (((TRDP_COMID_DSID_MAP_T *)pArg1)->comId > ((TRDP_COMID_DSID_MAP_T *)pArg2)->comId)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/**********************************************************************************************************************/
/**    Return the dataset for the comID
 *
 *
 *  @param[in]      comID           
 *
 *  @retval         NULL if not found
 *  @retval         pointer to dataset
 */
static TRDP_DATASET_T *find_DS_from_ComId (
    UINT32 comId)
{
    TRDP_COMID_DSID_MAP_T    key1 = {0};
    TRDP_DATASET_T           **key3;    
    TRDP_COMID_DSID_MAP_T    *key2 = (TRDP_COMID_DSID_MAP_T *) vos_bsearch(&key1, sComIdDsIdMap, sNumComId, sizeof(TRDP_COMID_DSID_MAP_T), comId_compare);
    
    key1.comId = comId;
    
    if (key2 != NULL)
    {
        TRDP_DATASET_T    key22 = {0};

        key22.id = key2->datasetId; 
        key3 = (TRDP_DATASET_T **) vos_bsearch(&key22, sDataSets, sNumEntries, sizeof(TRDP_DATASET_T*), dataset_compare_deref);
        if (key3 != NULL)
        {
            return *key3;
        }
    }

    return NULL;
}

/**********************************************************************************************************************/
/**    Return the dataset for the datasetID
 *
 *
 *  @param[in]      datasetID           dataset ID to find
 *
 *  @retval         NULL if not found
 *  @retval         pointer to dataset
 */
static TRDP_DATASET_T *find_DS (
    UINT32 datasetId)
{
    TRDP_DATASET_T  key2 = {0};
    TRDP_DATASET_T  **key3;

    key2.id = datasetId;
    key3 = vos_bsearch(&key2, sDataSets, sNumEntries, sizeof(TRDP_DATASET_T*), dataset_compare_deref);
    if (key3 != NULL)
    {
        return *key3;
    }

    return NULL;
}


/**********************************************************************************************************************/
/**    Marshall one dataset.
 *
 *  @param[in,out]  pInfo           Pointer with src & dest info
 *  @param[in]      pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_STATE_ERR  Too deep recursion
 *
 */

EXT_DECL TRDP_ERR_T marshall (
    TAU_MARSHALL_INFO_T *pInfo,
    TRDP_DATASET_T      *pDataset)
{
    TRDP_ERR_T  err;
    UINT16      index;
    UINT32      var_size    = 0;
    UINT8       *pSrc;
    UINT8       *pDst       = pInfo->pDst;

    /* Restrict recursion */
    pInfo->level++;
    if (pInfo->level > TAU_MAX_DS_LEVEL)
    {
        return TRDP_STATE_ERR;
    }

    /*    Align on struct boundary first    */
    pSrc = alignePtr(pInfo->pSrc, ALIGNOF(STRUCT_T));

    /*    Loop over all datasets in the array    */
    for (index = 0; index < pDataset->numElement; ++index)
    {
        UINT32 noOfItems = pDataset->pElement[index].size;

        if (TDRP_VAR_SIZE == noOfItems) /* variable size    */
        {
            noOfItems = var_size;
        }

        /*    Is this a composite type?    */
        if (pDataset->pElement[index].type > (UINT32) TRDP_TYPE_MAX)
        {
            while (noOfItems-- > 0)
            {
                /* Dataset, call ourself recursively */

				/* Never used before?  */                
                if (NULL == pDataset->pElement[index].pCachedDS)
                {
                	/* Look for it   */
                    pDataset->pElement[index].pCachedDS = find_DS(pDataset->pElement[index].type);
                }

                if (NULL == pDataset->pElement[index].pCachedDS)      /* Not in our DB    */
                {
                    vos_printf(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDataset->pElement[index].type);
                    return TRDP_COMID_ERR;
                }

                err = marshall(pInfo, pDataset->pElement[index].pCachedDS);
                if (err != TRDP_NO_ERR)
                {
                    return err;
                }
            }
        }
        else
        {
            switch (pDataset->pElement[index].type)
            {
                case TRDP_BOOLEAN:
                case TRDP_CHAR8:
                case TRDP_INT8:
                case TRDP_UINT8:
                {
                    /*    possible variable source size    */
                    var_size = *pSrc;

                    while (noOfItems-- > 0)
                    {
                        *pDst++ = *pSrc++;
                    }
                    break;
                }
                case TRDP_UTF16:
                case TRDP_INT16:
                case TRDP_UINT16:
                {
                    UINT16 *pSrc16 = (UINT16 *) alignePtr(pSrc, ALIGNOF(UINT16));

                    /*    possible variable source size    */
                    var_size = *pSrc16;

                    while (noOfItems-- > 0)
                    {
                        *pDst++ = (UINT8) (*pSrc16 >> 8);
                        *pDst++ = (UINT8) (*pSrc16 & 0xFF);
                        pSrc16++;
                    }
                    pSrc = (UINT8 *) pSrc16;
                    break;
                }
                case TRDP_INT32:
                case TRDP_UINT32:
                case TRDP_REAL32:
                case TRDP_TIMEDATE32:
                {
                     UINT32 *pSrc32 = (UINT32 *) alignePtr(pSrc, ALIGNOF(UINT32));

                    /*    possible variable source size    */
                    var_size = *pSrc32;

                    while (noOfItems-- > 0)
                    {
                        *pDst++ = (UINT8) (*pSrc32 >> 24);
                        *pDst++ = (UINT8) (*pSrc32 >> 16);
                        *pDst++ = (UINT8) (*pSrc32 >> 8);
                        *pDst++ = (UINT8) (*pSrc32 & 0xFF);
                        pSrc32++;
                    }
                    pSrc = (UINT8 *) pSrc32;
                    break;
                }
                case TRDP_TIMEDATE64:
                {
                    UINT32 *pSrc32 = (UINT32 *) alignePtr(pSrc, ALIGNOF(TIMEDATE64_STRUCT_T));

                    while (noOfItems-- > 0)
                    {
                        *pDst++ = (UINT8) (*pSrc32 >> 24);
                        *pDst++ = (UINT8) (*pSrc32 >> 16);
                        *pDst++ = (UINT8) (*pSrc32 >> 8);
                        *pDst++ = (UINT8) (*pSrc32 & 0xFF);
                        pSrc32++;
                        *pDst++ = (UINT8) (*pSrc32 >> 24);
                        *pDst++ = (UINT8) (*pSrc32 >> 16);
                        *pDst++ = (UINT8) (*pSrc32 >> 8);
                        *pDst++ = (UINT8) (*pSrc32 & 0xFF);
                        pSrc32++;
                    }
                    pSrc = (UINT8 *) pSrc32;
                    break;
                }
                case TRDP_TIMEDATE48:
                {
                    /*    This is not a base type but a structure    */
                    UINT32 *pSrc32;
                    UINT16 *pSrc16;

                    while (noOfItems-- > 0)
                    {
                        pSrc32 = (UINT32 *) alignePtr(pSrc, ALIGNOF(TIMEDATE48_STRUCT_T));
                        *pDst++ = (UINT8) (*pSrc32 >> 24);
                        *pDst++ = (UINT8) (*pSrc32 >> 16);
                        *pDst++ = (UINT8) (*pSrc32 >> 8);
                        *pDst++ = (UINT8) (*pSrc32 & 0xFF);
                        pSrc32++;
                        pSrc16 = (UINT16 *) alignePtr((UINT8*) pSrc32, ALIGNOF(UINT16));
                        *pDst++ = (UINT8) (*pSrc16 >> 8);
                        *pDst++ = (UINT8) (*pSrc16 & 0xFF);
                        pSrc32++;
                        pSrc = (UINT8 *) pSrc32;
                    }
                    break;
                }
                case TRDP_INT64:
                case TRDP_UINT64:
                case TRDP_REAL64:
                    packedCopy64( &pSrc, &pDst, noOfItems);
                    break;
                default:
                    break;
            }
        }
        pInfo->pDst = pDst;
        pInfo->pSrc = pSrc;
    }
    
    
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Unmarshall one dataset.
 *
 *  @param[in,out]  pInfo           Pointer with src & dest info
 *  @param[in]      pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_STATE_ERR  Too deep recursion
 *
 */

EXT_DECL TRDP_ERR_T unmarshall (
    TAU_MARSHALL_INFO_T *pInfo,
    TRDP_DATASET_T      *pDataset)
{
    TRDP_ERR_T  err;
    UINT16      index;
    UINT32      var_size    = 0;
    UINT8       *pSrc       = pInfo->pSrc;
    UINT8       *pDst       = pInfo->pDst;

    /* Restrict recursion */
    pInfo->level++;
    if (pInfo->level > TAU_MAX_DS_LEVEL)
    {
        return TRDP_STATE_ERR;
    }

    /*    Loop over all datasets in the array    */

    for (index = 0; index < pDataset->numElement; ++index)
    {
        UINT32 noOfItems = pDataset->pElement[index].size;

        if (TDRP_VAR_SIZE == noOfItems) /* variable size    */
        {
            noOfItems = var_size;
        }
        /*    Is this a composite type?    */
        if (pDataset->pElement[index].type > (UINT32) TRDP_TYPE_MAX)
        {
            while (noOfItems-- > 0)
            {
                /* Dataset, call ourself recursively */
				/* Never used before?  */                
                if (NULL == pDataset->pElement[index].pCachedDS)
                {
                	/* Look for it   */
                    pDataset->pElement[index].pCachedDS = find_DS(pDataset->pElement[index].type);
                }
                
                if (NULL == pDataset->pElement[index].pCachedDS)      /* Not in our DB    */
                {
                    vos_printf(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDataset->pElement[index].type);
                    return TRDP_COMID_ERR;
                }

                err = unmarshall(pInfo, pDataset->pElement[index].pCachedDS);
                if (err != TRDP_NO_ERR)
                {
                    return err;
                }
                pDst = pInfo->pDst;
                pSrc = pInfo->pSrc;

            }
        }
        else
        {
            switch (pDataset->pElement[index].type)
            {
                case TRDP_BOOLEAN:
                case TRDP_CHAR8:
                case TRDP_INT8:
                case TRDP_UINT8:
                {
                    while (noOfItems-- > 0)
                    {
                        var_size = *pSrc++;
                        *pDst++ = var_size;
                    }
                    break;
                }
                case TRDP_UTF16:
                case TRDP_INT16:
                case TRDP_UINT16:
                {
                    UINT16 *pDst16 = (UINT16 *) alignePtr(pDst, ALIGNOF(UINT16));
                    while (noOfItems-- > 0)
                    {
                        *pDst16 = *pSrc++ << 8;
                        *pDst16 += *pSrc++;
                        /*    possible variable source size    */
                        var_size = *pDst16;
                        pDst16++;
                    }
                    pDst = (UINT8 *) pDst16;
                    break;
                }
                case TRDP_INT32:
                case TRDP_UINT32:
                case TRDP_REAL32:
                case TRDP_TIMEDATE32:
                {
                    UINT32 *pDst32 = (UINT32 *) alignePtr(pDst, ALIGNOF(UINT32));
                    while (noOfItems-- > 0)
                    {
                        *pDst32 = *pSrc++ << 24;
                        *pDst32 += *pSrc++ << 16;
                        *pDst32 += *pSrc++ << 8;
                        *pDst32 += *pSrc++;
                        var_size = *pDst32;
                        pDst32++;
                    }
                    pDst = (UINT8 *) pDst32;
                    break;
                }
                case TRDP_TIMEDATE48:
                {
                    /*    This is not a base type but a structure    */
                    UINT32 *pDst32;
                    UINT16 *pDst16;
                    while (noOfItems-- > 0)
                    {
                        pDst32 = (UINT32 *) alignePtr(pDst, ALIGNOF(TIMEDATE48_STRUCT_T));
                        *pDst32 = *pSrc++ << 24;
                        *pDst32 += *pSrc++ << 16;
                        *pDst32 += *pSrc++ << 8;
                        *pDst32 += *pSrc++;
                        pDst32++;
                        pDst16 = (UINT16 *) alignePtr((UINT8*)pDst32, ALIGNOF(UINT16));
                        *pDst16 = *pSrc++ << 8;
                        *pDst16 += *pSrc++;
                        pDst32++;
                        pDst = (UINT8 *) pDst32;
                    }
                    break;
                }
                case TRDP_TIMEDATE64:
                {
                    /*    This is not a base type but a structure    */
                    UINT32 *pDst32 = (UINT32 *) alignePtr(pDst, ALIGNOF(TIMEDATE64_STRUCT_T));
                    while (noOfItems-- > 0)
                    {
                        *pDst32 = *pSrc++ << 24;
                        *pDst32 += *pSrc++ << 16;
                        *pDst32 += *pSrc++ << 8;
                        *pDst32 += *pSrc++;
                        pDst32++;
                        pDst32 = (UINT32 *) alignePtr((UINT8*)pDst32, ALIGNOF(UINT32));
                        *pDst32 = *pSrc++ << 24;
                        *pDst32 += *pSrc++ << 16;
                        *pDst32 += *pSrc++ << 8;
                        *pDst32 += *pSrc++;
                        pDst32++;
                    }
                    pDst = (UINT8 *) pDst32;
                    break;
                }
                case TRDP_INT64:
                case TRDP_UINT64:
                case TRDP_REAL64:
                    unpackedCopy64((UINT8 * *) &pSrc, &pDst, noOfItems);
                    break;
                default:
                    break;
            }
        }
        pInfo->pDst = pDst;
        pInfo->pSrc = pSrc;
    }

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/**    Function to initialise the marshalling/unmarshalling.
 *    The supplied array must be sorted by ComIds. The array must exist during the use of the marshalling
 *    functions (until tlc_terminate()).
 *
 *  @param[in,out]  ppRefCon         Returns a pointer to be used for the reference context of marshalling/unmarshalling
 *  @param[in]      numComId         Number of datasets found in the configuration
 *  @param[in]      pComIdDsIdMap    Pointer to an array of structures of type TRDP_DATASET_T
 *  @param[in]      numDataSet       Number of datasets found in the configuration
 *  @param[in]      pDataset         Pointer to an array of pointers to structures of type TRDP_DATASET_T
 *
 *  @retval         TRDP_NO_ERR      no error
 *  @retval         TRDP_MEM_ERR     provided buffer to small
 *  @retval         TRDP_PARAM_ERR   Parameter error
 *
 */

EXT_DECL TRDP_ERR_T tau_initMarshall (
    void                    **ppRefCon,
    UINT32                  numComId,
    TRDP_COMID_DSID_MAP_T   *pComIdDsIdMap,        
    UINT32                  numDataSet,
    TRDP_DATASET_T          *pDataset[])
{
	UINT32	i,j;
    
    if (ppRefCon == NULL || pDataset == NULL || numDataSet == 0 || numComId == 0 || pComIdDsIdMap == 0)
    {
        return TRDP_PARAM_ERR;
    }

    /*    Save the pointer to the comId mapping table    */
    sComIdDsIdMap   = pComIdDsIdMap;
    sNumComId = numComId;
    
    /* sort the table    */
    vos_qsort(pComIdDsIdMap, numComId, sizeof(TRDP_COMID_DSID_MAP_T), comId_compare);

    /*    Save the pointer to the table    */
    sDataSets   = pDataset;
    sNumEntries = numDataSet;

    /* invalidate the cache */
    for (i = 0; i < numDataSet; i++)
    {
    	for (j = 0; j < pDataset[i]->numElement; j++)
        {
            pDataset[i]->pElement[j].pCachedDS = NULL;
        }
    }
    /* sort the table    */
    vos_qsort(pDataset, numDataSet, sizeof(TRDP_DATASET_T*), dataset_compare);

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    marshall function.
 *
 *  @param[in]      pRefCon         pointer to user context
 *  @param[in]      comId           ComId to identify the structure out of a configuration
 *  @param[in]      pSrc            pointer to received original message
 *  @param[in]      pDest           pointer to a buffer for the treated message
 *  @param[in,out]  pDestSize       size of the provide buffer / size of the treated message
 *  @param[in,out]  ppDSPointer     pointer to pointer to cached datasett
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR  comid not existing
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */

EXT_DECL TRDP_ERR_T tau_marshall (
    void        	*pRefCon,
    UINT32      	comId,
    UINT8       	*pSrc,
    UINT8       	*pDest,
    UINT32      	*pDestSize,
    TRDP_DATASET_T	**ppDSPointer)
{
    TRDP_DATASET_T      *pDataset;
    TAU_MARSHALL_INFO_T info;

    if (0 == comId || NULL == pSrc || NULL == pDest || NULL == pDestSize || 0 == *pDestSize)
    {
        return TRDP_PARAM_ERR;
    }

	/* Can we use the formerly cached value? */
	if (NULL != ppDSPointer)
    {
        if (NULL == *ppDSPointer)
        {
        	*ppDSPointer = find_DS_from_ComId(comId);
        }
        pDataset = *ppDSPointer;
    }
    else
    {
        pDataset = find_DS_from_ComId(comId);
    }

    if (NULL == pDataset)   /* Not in our DB    */
    {
        vos_printf(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", comId);
        return TRDP_COMID_ERR;
    }

    info.level      = 0;
    info.pSrc       = pSrc;
    info.pDst       = pDest;
    info.pDstEnd    = pDest + *pDestSize;

    return marshall(&info, pDataset);
}

/**********************************************************************************************************************/
/**    unmarshall function.
 *
 *  @param[in]      pRefCon         pointer to user context
 *  @param[in]      comId           ComId to identify the structure out of a configuration
 *  @param[in]      pSrc            pointer to received original message
 *  @param[in]      pDest           pointer to a buffer for the treated message
 *  @param[in,out]  pDestSize       size of the provide buffer / size of the treated message
 *  @param[in,out]  ppDSPointer     pointer to pointer to cached dataset
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

EXT_DECL TRDP_ERR_T tau_unmarshall (
    void        	*pRefCon,
    UINT32      	comId,
    UINT8       	*pSrc,
    UINT8      		*pDest,
    UINT32      	*pDestSize,
    TRDP_DATASET_T	**ppDSPointer)
{
    TRDP_DATASET_T      *pDataset;
    TAU_MARSHALL_INFO_T info;

    if (0 == comId || NULL == pSrc || NULL == pDest || NULL == pDestSize || 0 == *pDestSize)
    {
        vos_printf(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", comId);
        return TRDP_PARAM_ERR;
    }

	/* Can we use the formerly cached value? */
	if (NULL != ppDSPointer)
    {
        if (NULL == *ppDSPointer)
        {
        	*ppDSPointer = find_DS_from_ComId(comId);
        }
        pDataset = *ppDSPointer;
    }
    else
    {
        pDataset = find_DS_from_ComId(comId);
    }
    
    if (NULL == pDataset)   /* Not in our DB    */
    {
        return TRDP_COMID_ERR;
    }

    info.level      = 0;
    info.pSrc       = pSrc;
    info.pDst       = pDest;
    info.pDstEnd    = pDest + *pDestSize;

    return unmarshall(&info, pDataset);
}
