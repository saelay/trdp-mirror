/**********************************************************************************************************************/
/**
 * @file            trdp_if.h
 *
 * @brief           Typedefs for TRDP communication
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

#ifndef TRDP_IF_H
#define TRDP_IF_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_if_light.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */


/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/** Check if the session handle is valid
 *
 *
 *  @param[in]      pSessionHandle    pointer to packet data (dataset)
 *  @retval         TRUE              is valid
 *  @retval         FALSE             is invalid
 */
BOOL trdp_isValidSession (
    TRDP_APP_SESSION_T pSessionHandle);

/**********************************************************************************************************************/
/** Get the session queue head pointer
 *
 *  @retval	        &sSession
 */
TRDP_APP_SESSION_T *trdp_sessionQueue (
    void);

#ifdef __cplusplus
}
#endif

#endif  /* TRDP_IF_LIGHT_H	*/
