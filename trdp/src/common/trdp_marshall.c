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
    const UINT8 *pSrc;      /**< source pointer			*/
    UINT8       *pDst;      /**< destination pointer	*/
    UINT8       *pDstEnd;   /**< last destination		*/
} TAU_MARSHALL_INFO_T;

/***********************************************************************************************************************
 * LOCALS
 */
static TRDP_DATASET_T   *sDataSets  = NULL;
static UINT32           sNumEntries = 0;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

/**********************************************************************************************************************/
/**	Align a pointer to the next natural address.
 *
 *
 *  @param[in]      pSrc            Pointer to align
 *
 *  @retval         aligned pointer
 */
static inline const UINT8 *alignePtr (
    const UINT8 *pSrc,
    UINT32      alignment)
{
    alignment--;
    return ((UINT32) pSrc & alignment) ? pSrc + alignment : pSrc;
}

/**********************************************************************************************************************/
/**	Copy a variable to its natural address.
 *
 *
 *  @param[in,out]      ppSrc           Pointer to pointer to source variable
 *  @param[in,out]      ppDst           Pointer to pointer to destination variable
 *  @param[in]          noOfItems       Items to copy
 *
 *  @retval         none
 */
static inline void unpackedCopy64 (
    UINT8   * *ppSrc,
    UINT8   * *ppDst,
    UINT32  noOfItems)

#if __BIG_ENDIAN__
{
    UINT32  size    = noOfItems * sizeof(UINT64);
    UINT8   *pDst8  = (UINT8 *) alignePtr(*ppDst, ALIGNOF(UINT64));
    memcpy(pDst8, *ppSrc, size);
    *ppSrc  = (UINT8 *) *ppSrc + size;
    *ppDst  = (UINT8 *) pDst8 + size;
}
#elif __LITTLE_ENDIAN__
{
    UINT8 *pSrc8 = (UINT8 *) (((UINT64 *) alignePtr(*ppSrc, ALIGNOF(UINT64))) + 1);
    *ppSrc = (UINT8 *) pSrc8;
    while (noOfItems--)
    {
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
    }
}
#else
#endif

/**********************************************************************************************************************/
/**	Copy a variable from its natural address.
 *
 *
 *  @param[in,out]      ppSrc           Pointer to pointer to source variable
 *  @param[in,out]      ppDst           Pointer to pointer to destination variable
 *  @param[in]          noOfItems       Items to copy
 *
 *  @retval         none
 */
static inline void packedCopy64 (
    UINT8   * *ppSrc,
    UINT8   * *ppDst,
    UINT32  noOfItems)

#if __BIG_ENDIAN__
{
    UINT8 *pSrc8 = (UINT8 *) alignePtr(*ppSrc, ALIGNOF(UINT64));
    while (noOfItems--)
    {
        **ppDst++   = *pSrc8++;
        **ppDst++   = *pSrc8++;
        **ppDst++   = *pSrc8++;
        **ppDst++   = *pSrc8++;
        **ppDst++   = *pSrc8++;
        **ppDst++   = *pSrc8++;
        **ppDst++   = *pSrc8++;
        **ppDst++   = *pSrc8++;
    }
    *ppSrc = (UINT8 *) pSrc8;
}
#elif __LITTLE_ENDIAN__
{
    UINT8 *pSrc8 = (UINT8 *) (((UINT64 *) alignePtr(*ppSrc, ALIGNOF(UINT64))) + 1);
    *ppSrc = (UINT8 *) pSrc8;
    while (noOfItems--)
    {
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
        *(*ppDst)++ = *--pSrc8;
    }
}
#else
{
    UINT64 *pSrc64 = (UINT64 *) alignePtr(*ppSrc, ALIGNOF(UINT64));
    while (noOfItems--)
    {
        *pDst++ = (UINT8) (*pSrc64 >> 56);
        *pDst++ = (UINT8) (*pSrc64 >> 48);
        *pDst++ = (UINT8) (*pSrc64 >> 40);
        *pDst++ = (UINT8) (*pSrc64 >> 32);
        *pDst++ = (UINT8) (*pSrc64 >> 24);
        *pDst++ = (UINT8) (*pSrc64 >> 16);
        *pDst++ = (UINT8) (*pSrc64 >> 8);
        *pDst++ = (UINT8) (*pSrc64 & 0xFF);
        pSrc64++;
    }
    pSrc = (UINT8 *) pSrc64;
}
#endif

/**********************************************************************************************************************/
/**	Dataset compare function
 *
 *  @param[in]      pArg1		Pointer to first element
 *  @param[in]      pArg2		Pointer to second element
 *
 *  @retval         -1 if arg1 < arg2
 *  @retval          0 if arg1 == arg2
 *  @retval          1 if arg1 > arg2
 */
static int dataset_compare (
    const void  *pArg1,
    const void  *pArg2)
{
    if (((TRDP_DATASET_T *)pArg1)->id < ((TRDP_DATASET_T *)pArg2)->id)
    {
        return -1;
    }
    else if (((TRDP_DATASET_T *)pArg1)->id > ((TRDP_DATASET_T *)pArg2)->id)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/**********************************************************************************************************************/
/**	Return the dataset for the comID
 *
 *
 *  @param[in]      comID           Pointer to align
 *
 *  @retval         NULL if not found
 *  @retval         pointer to dataset
 */
static TRDP_DATASET_T *find_DS (
    UINT32 comID)
{
#if 0
    UINT32 index;
    for (index = 0; index < sNumEntries; index++)
    {
        if (sDataSets[index].id == comID)
        {
            return &sDataSets[index];
        }
    }
#else
    TRDP_DATASET_T key = {comID, 0, 0};
    return vos_bsearch(&key, sDataSets, sNumEntries, sizeof(TRDP_DATASET_T), dataset_compare);
#endif
    return NULL;
}


/**********************************************************************************************************************/
/**	Marshall one dataset.
 *
 *  @param[in,out]  pInfo           Pointer with src & dest info
 *  @param[in]	    pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR		no error
 *  @retval         TRDP_MEM_ERR	provided buffer to small
 *  @retval         TRDP_PARAM_ERR	Parameter error
 *  @retval         TRDP_STATE_ERR	Too deep recursion
 *
 */

EXT_DECL TRDP_ERR_T marshall (
    TAU_MARSHALL_INFO_T *pInfo,
    TRDP_DATASET_T      *pDataset)
{
    TRDP_ERR_T  err;
    UINT16      index;
    UINT32      var_size    = 0;
    UINT8 const *pSrc       = pInfo->pSrc;
    UINT8       *pDst       = pInfo->pDst;

    /* Restrict recursion */
    pInfo->level++;
    if (pInfo->level > TAU_MAX_DS_LEVEL)
    {
        return TRDP_STATE_ERR;
    }

    /*	Loop over all datasets in the array	*/

    for (index = 0; index < pDataset->numElement; ++index)
    {
        UINT32 noOfItems = pDataset->pElement[index].size;

        if (TDRP_VAR_SIZE == noOfItems) /* variable size	*/
        {
            noOfItems = var_size;
        }

        /*	Is this a composite type?	*/
        if (pDataset->pElement[index].type > 1000)
        {
            while (noOfItems-- > 0)
            {
                /* Dataset, call ourself recursively */
                TRDP_DATASET_T *pLocalDataset = find_DS(pDataset->pElement[index].type);

                if (NULL == pLocalDataset)      /* Not in our DB	*/
                {
                    vos_printf(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDataset->pElement[index].type);
                    return TRDP_COMID_ERR;
                }

                err = marshall(pInfo, pLocalDataset);
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
                    while (noOfItems-- > 0)
                    {
                        *pDst++ = *pSrc++;
                    }
                    break;
                }
                case TRDP_STRING:
                {
                    strncpy((char *)pDst, (char *)pSrc, 16);
                    pDst    += 16;
                    pSrc    += 16;
                    break;
                }
                case TRDP_UTF16:
                case TRDP_INT16:
                case TRDP_UINT16:
                {
                    UINT16 *pSrc16 = (UINT16 *) alignePtr(pSrc, ALIGNOF(UINT16));

                    /*	possible variable source size	*/
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
                case TRDP_TIMEDATE48:
                    break;
                case TRDP_INT64:
                case TRDP_UINT64:
                case TRDP_REAL64:
                case TRDP_TIMEDATE64:
                    packedCopy64((UINT8 * *) &pSrc, &pDst, noOfItems);
                    break;
                default:
                    break;
            }
        }
    }
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**	Unmarshall one dataset.
 *
 *  @param[in,out]  pInfo           Pointer with src & dest info
 *  @param[in]	    pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR		no error
 *  @retval         TRDP_MEM_ERR	provided buffer to small
 *  @retval         TRDP_PARAM_ERR	Parameter error
 *  @retval         TRDP_STATE_ERR	Too deep recursion
 *
 */

EXT_DECL TRDP_ERR_T unmarshall (
    TAU_MARSHALL_INFO_T *pInfo,
    TRDP_DATASET_T      *pDataset)
{
    TRDP_ERR_T  err;
    UINT16      index;
    UINT32      var_size    = 0;
    UINT8 const *pSrc       = pInfo->pSrc;
    UINT8       *pDst       = pInfo->pDst;

    /* Restrict recursion */
    pInfo->level++;
    if (pInfo->level > TAU_MAX_DS_LEVEL)
    {
        return TRDP_STATE_ERR;
    }

    /*	Loop over all datasets in the array	*/

    for (index = 0; index < pDataset->numElement; ++index)
    {
        UINT32 noOfItems = pDataset->pElement[index].size;

        if (TDRP_VAR_SIZE == noOfItems) /* variable size	*/
        {
            noOfItems = var_size;
        }
        /*	Is this a composite type?	*/
        if (pDataset->pElement[index].type > 1000)
        {
            while (noOfItems-- > 0)
            {
                /* Dataset, call ourself recursively */
                TRDP_DATASET_T *pLocalDataset = find_DS(pDataset->pElement[index].type);

                if (NULL == pLocalDataset)      /* Not in our DB	*/
                {
                    vos_printf(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDataset->pElement[index].type);
                    return TRDP_COMID_ERR;
                }

                err = unmarshall(pInfo, pLocalDataset);
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
                    while (noOfItems-- > 0)
                    {
                        *pDst++ = *pSrc++;
                    }
                    break;
                }
                case TRDP_STRING:
                {
                    strncpy((char *)pDst, (char *)pSrc, 16);
                    pDst    += 16;
                    pSrc    += 16;
                    break;
                }
                case TRDP_UTF16:
                case TRDP_INT16:
                case TRDP_UINT16:
                {
                    UINT16 *pDst16 = (UINT16 *) alignePtr(pDst, ALIGNOF(UINT16));
                    while (noOfItems-- > 0)
                    {
                        *pDst16 = *pSrc++ >> 8;
                        *pDst16 += *pSrc++;
                        /*	possible variable source size	*/
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
                        *pDst32 = *pSrc++ >> 24;
                        *pDst32 += *pSrc++ >> 16;
                        *pDst32 += *pSrc++ >> 8;
                        *pDst32 += *pSrc++;
                        pDst32++;
                    }
                    pDst = (UINT8 *) pDst32;
                    break;
                }
                case TRDP_TIMEDATE48:
                    break;
                case TRDP_INT64:
                case TRDP_UINT64:
                case TRDP_REAL64:
                case TRDP_TIMEDATE64:
                    unpackedCopy64((UINT8 * *) &pSrc, &pDst, noOfItems);
                    break;
                default:
                    break;
            }
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/**	Function to initialise the marshalling/unmarshalling.
 *	The supplied array must be sorted by ComIds. The array must exist during the use of the marshalling
 *	functions (until tlc_terminate()).
 *
 *  @param[in,out]  ppRefCon         Returns a pointer to be used for the reference context of marshalling/unmarshalling
 *  @param[in]	    numDataSet       Number of datasets found in the configuration
 *  @param[in]	    pDataset         Pointer to an array of structures of type TRDP_DATASET_T
 *
 *  @retval         TRDP_NO_ERR		no error
 *  @retval         TRDP_MEM_ERR	provided buffer to small
 *  @retval         TRDP_PARAM_ERR	Parameter error
 *
 */

EXT_DECL TRDP_ERR_T tau_initMarshall (
    void            * *ppRefCon,
    UINT32          numDataSet,
    TRDP_DATASET_T  *pDataset)
{
    if (ppRefCon == NULL || pDataset == NULL || numDataSet == 0)
    {
        return TRDP_PARAM_ERR;
    }

    /*	Save the pointer to the table	*/
    sDataSets   = pDataset;
    sNumEntries = numDataSet;

    /* sort the table	*/
    vos_qsort(pDataset, numDataSet, sizeof(TRDP_DATASET_T), dataset_compare);

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**	marshall function.
 *
 *  @param[in]	    pRefCon    pointer to user context
 *  @param[in]      comId      ComId to identify the structure out of a configuration
 *  @param[in]      pSrc       pointer to received original message
 *  @param[in]	    pDest      pointer to a buffer for the treated message
 *  @param[in,out]  pDestSize  size of the provide buffer / size of the treated message
 *
 *  @retval         TRDP_NO_ERR	    no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR	comid not existing
 *  @retval         TRDP_PARAM_ERR	Parameter error
 *
 */

EXT_DECL TRDP_ERR_T tau_marshall (
    void        *pRefCon,
    UINT32      comId,
    const UINT8 *pSrc,
    UINT8       *pDest,
    UINT32      *pDestSize)
{
    TRDP_DATASET_T      *pDataset;
    TAU_MARSHALL_INFO_T info;

    if (0 == comId || NULL == pSrc || NULL == pDest || NULL == pDestSize || 0 == *pDestSize)
    {
        return TRDP_PARAM_ERR;
    }

    pDataset = find_DS(comId);

    if (NULL == pDataset)   /* Not in our DB	*/
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
/**	unmarshall function.
 *
 *  @param[in]	    pRefCon    pointer to user context
 *  @param[in]      comId      ComId to identify the structure out of a configuration
 *  @param[in]      pSrc       pointer to received original message
 *  @param[in]	    pDest      pointer to a buffer for the treated message
 *  @param[in,out]  pDestSize  size of the provide buffer / size of the treated message
 *
 *  @retval         TRDP_NO_ERR	    no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

EXT_DECL TRDP_ERR_T tau_unmarshall (
    void        *pRefCon,
    UINT32      comId,
    const UINT8 *pSrc,
    UINT8       *pDest,
    UINT32      *pDestSize)
{
    TRDP_DATASET_T      *pDataset;
    TAU_MARSHALL_INFO_T info;

    if (0 == comId || NULL == pSrc || NULL == pDest || NULL == pDestSize || 0 == *pDestSize)
    {
        vos_printf(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", comId);
        return TRDP_PARAM_ERR;
    }

    pDataset = find_DS(comId);

    if (NULL == pDataset)   /* Not in our DB	*/
    {
        return TRDP_COMID_ERR;
    }

    info.level      = 0;
    info.pSrc       = pSrc;
    info.pDst       = pDest;
    info.pDstEnd    = pDest + *pDestSize;

    return unmarshall(&info, pDataset);
}
