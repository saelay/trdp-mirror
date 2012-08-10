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
   INT32		level;
   UINT8 		*pSrc;		/**< source pointer			*/
   UINT8 		*pDst;		/**< destination pointer	*/
   UINT8		*pDstEnd;	/**< last destination		*/
} TAU_MARSHALL_INFO_T;

/***********************************************************************************************************************
 * LOCALS
 */
static TRDP_DATASET_T	(*sDataSets)[] = NULL;
static UINT32			sNumEntries = 0;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

/**********************************************************************************************************************/
/**	Align a pointer to the next natural address.
 *
 *	
 *  @param[in]  	pSrc      		Pointer to align
 *
 *  @retval         aligned pointer
 */
UINT8*	alignePtr(
	UINT8*	pSrc,
    UINT32	alignment)
{
	alignment--;
	return ((UINT32) pSrc & alignment)? pSrc + alignment : pSrc;
}

/**********************************************************************************************************************/
/**	Marshall one dataset.
 *	
 *  @param[in,out]  pInfo      		Pointer with src & dest info
 *  @param[in]	    pDataset    	Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR		no error
 *  @retval         TRDP_MEM_ERR	provided buffer to small
 *  @retval         TRDP_PARAM_ERR	Parameter error
 *  @retval         TRDP_STATE_ERR	Too deep recursion
 *
 */

EXT_DECL TRDP_ERR_T marshall (
    TAU_MARSHALL_INFO_T	*pInfo,
    TRDP_DATASET_T		*pDataset)
{
	UINT16	index;
    UINT32	var_size = 0;
    UINT8	*pSrc = pInfo->pSrc;
    UINT8	*pDst = pInfo->pDst;

    /* Restrict recursion */
    pInfo->level++;
    if (pInfo->level > TAU_MAX_DS_LEVEL)
    {
    	return TRDP_STATE_ERR;
    }

	/*	Loop over all datasets in the array	*/

	for (index = 0; index < pDataset->numElement; ++index)
    {
    	UINT32	noOfItems = pDataset->pElement[index].size;
        
        if (TDRP_VAR_SIZE == noOfItems)	/* variable size	*/
        {
			/*	TBD: Where does the var_size come from?	*/
        	memcpy(pDst, pSrc, var_size);
        }
        else
        {
        	switch (pDataset->pElement[index].type)
            {
            	case TRDP_BOOLEAN:
                case TRDP_CHAR8:
                case TRDP_INT8:
                case TRDP_UINT8:
                case TRDP_STRING:
                {
                	while (noOfItems--)
                    {
	                	*pDst++ = *pSrc++;
                    }
                }
	                break;
                case TRDP_UTF16:
                case TRDP_INT16:
                case TRDP_UINT16:
                {
                	UINT16 *pSrc16 = (UINT16 *) alignePtr(pSrc, ALIGNOF(UINT16)); 
                	while (noOfItems--)
                    {
 	                	*pDst++ = (UINT8) (*pSrc16 >> 8);
	                	*pDst++ = (UINT8) (*pSrc16 & 0xFF);
                        pSrc16++;
                    }
                    pSrc = (UINT8*) pSrc16;
                }
                	break;
                case TRDP_INT32:
                case TRDP_UINT32:
                case TRDP_REAL32:
                case TRDP_TIMEDATE32:
                {
                	UINT32 *pSrc32 = (UINT32 *) alignePtr(pSrc, ALIGNOF(UINT32)); 
                	while (noOfItems--)
                    {
 	                	*pDst++ = (UINT8) (*pSrc32 >> 24);
	                	*pDst++ = (UINT8) (*pSrc32 >> 16);
 	                	*pDst++ = (UINT8) (*pSrc32 >> 8);
	                	*pDst++ = (UINT8) (*pSrc32 & 0xFF);
                        pSrc32++;
                    }
                     pSrc = (UINT8*) pSrc32;
                }
                	break;
                case TRDP_TIMEDATE48:
                	break;
                case TRDP_INT64:
                case TRDP_UINT64:
                case TRDP_REAL64:
                case TRDP_TIMEDATE64:
#if __BIG_ENDIAN__
                {
                	UINT8 *pSrc8 = (UINT8 *) alignePtr(pSrc, ALIGNOF(UINT64)); 
                	while (noOfItems--)
                    {
 	                	*pDst++ = *pSrc8++;
 	                	*pDst++ = *pSrc8++;
 	                	*pDst++ = *pSrc8++;
 	                	*pDst++ = *pSrc8++;
 	                	*pDst++ = *pSrc8++;
	                	*pDst++ = *pSrc8++;
 	                	*pDst++ = *pSrc8++;
	                	*pDst++ = *pSrc8++;
                        pSrc64++;
                    }
                    pSrc = (UINT8*) pSrc8;
                }
#elif __LITTLE_ENDIAN__
                {
                	UINT8 *pSrc8 = (UINT8 *) (((UINT64 *) alignePtr(pSrc, ALIGNOF(UINT64))) + 1); 
                	pSrc = (UINT8*) pSrc8;
                    while (noOfItems--)
                    {
 	                	*pDst++ = --*pSrc8;
 	                	*pDst++ = --*pSrc8;
 	                	*pDst++ = --*pSrc8;
 	                	*pDst++ = --*pSrc8;
 	                	*pDst++ = --*pSrc8;
	                	*pDst++ = --*pSrc8;
 	                	*pDst++ = --*pSrc8;
	                	*pDst++ = --*pSrc8;
                    }
                }
#else
                {
                	UINT64 *pSrc64 = (UINT64 *) alignePtr(pSrc, ALIGNOF(UINT64)); 
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
                    pSrc = (UINT8*) pSrc64;
                }
#endif
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
    TRDP_DATASET_T  (*pDataset)[])
{
    if (ppRefCon == NULL || pDataset == NULL || numDataSet == 0)
    {
        return TRDP_PARAM_ERR;
    }

	/*	Save the pointer to the table	*/
    sDataSets = pDataset;
	sNumEntries = numDataSet;
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
    void 		*pRefCon,
    UINT32 		comId,
    const UINT8 * pSrc,
    UINT8       * pDest,
    UINT32      * pDestSize)
{
	TRDP_DATASET_T	*pDataset;
    
    if (0 == comId || NULL == pSrc || NULL == pSrc || NULL == pDestSize || 0 == *pDestSize)
    {
        return TRDP_PARAM_ERR;
    }

	pDataset = find_DS(comId);

    if (NULL == pDataset)	/* Not in our DB	*/
    {
    	return TRDP_COMID_ERR;
    }
    
    
	return TRDP_NO_ERR;
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

EXT_DECL  TRDP_ERR_T tau_unmarshall (
    void *pRefCon,
    UINT32 comId,
    const UINT8 * pSrc,
    UINT8       * pDest,
    UINT32      * pDestSize)
{
	return TRDP_NO_ERR;
}
