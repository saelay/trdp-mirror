/**********************************************************************************************************************/
/**
 * @file            tau_marshall.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details			This module provides the interface to the following utilities
 *					- marshalling/unmarshalling
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
 */

#ifndef TAU_MARSHALL_H
#define TAU_MARSHALL_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#include "trdp_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#ifndef __cdecl
#define __cdecl
#endif


/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Types for marshalling / unmarshalling	*/

/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/*	Marshalling/Unmarshalling                                                                                         */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**	Function to initialise the marshalling/unmarshalling.
 *
 *
 *  @param[in,out]	ppRefCon         Returns a pointer to be used for the reference context of marshalling/unmarshalling
 *  @param[in]	    numDataSet       Number of datasets found in the configuration
 *  @param[in]	    pDataset         Pointer to an array of a structures of type TRDP_DATASET_T
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */

EXT_DECL TRDP_ERR_T tau_initMarshall (
    void            * *ppRefCon,
    UINT32          numDataSet,
    TRDP_DATASET_T  *pDataset);

/**********************************************************************************************************************/
/**	marshall function.
 *
 *  @param[in]	    pRefCon    pointer to user context
 *  @param[in]		comId      ComId to identify the structure out of a configuration
 *  @param[in]		pSrc       pointer to received original message
 *  @param[in]	    pDest      pointer to a buffer for the treated message
 *  @param[in,out]	pDestSize  size of the provide buffer / size of the treated message
 *
 *  @retval         TRDP_NO_ERR	    no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

typedef TRDP_ERR_T tau_marshall (
    void *pRefCon,
    UINT32 comId,
    const UINT8 * pSrc,
    UINT8       * pDest,
    UINT32      * pDestSize);


/**********************************************************************************************************************/
/**	unmarshall function.
 *
 *  @param[in]	    pRefCon    pointer to user context
 *  @param[in]		comId      ComId to identify the structure out of a configuration
 *  @param[in]		pSrc       pointer to received original message
 *  @param[in]	    pDest      pointer to a buffer for the treated message
 *  @param[in,out]	pDestSize  size of the provide buffer / size of the treated message
 *
 *  @retval         TRDP_NO_ERR	    no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

typedef TRDP_ERR_T tau_unmarshall (
    void *pRefCon,
    UINT32 comId,
    const UINT8 * pSrc,
    UINT8       * pDest,
    UINT32      * pDestSize);



#ifdef __cplusplus
}
#endif

#endif /* TAU_MARSHALL_H */
