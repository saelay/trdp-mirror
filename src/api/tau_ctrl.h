/**********************************************************************************************************************/
/**
 * @file            tau_ctrl.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                  - ETB control
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 */

#ifndef TAU_CTRL_H
#define TAU_CTRL_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#include "trdp_types.h"
#include "tau_tti.h"
#include "tau_ctrl_types.h"

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


/***********************************************************************************************************************
 * PROTOTYPES
 */


/**********************************************************************************************************************/
/*    Train switch control                                                                                            */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**    Function to init  ECSP control interface
 *
 *  @param[in]      TRDP_APP_SESSION_T  appHndl
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initEcspCtrl ( TRDP_APP_SESSION_T appHndl );



/**********************************************************************************************************************/
/**    Function to close  ECSP control interface
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_UNKNOWN_ERR    undefined error
 *
 */

EXT_DECL TRDP_ERR_T tau_terminateEcspCtrl (void);


/**********************************************************************************************************************/
/**    Function to set ECSP control information
 *
 *
 *  @param[in]      pEcspCtrl       Pointer to the ECSP control structure
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_setEcspCtrl (TRDP_ECSP_CTRL_T  *pEcspCtrl);


/**********************************************************************************************************************/
/**    Function to get ECSP status information
 *
 *
 *  @param[in/out]   pEcspStat       Pointer to the ECSP status structure
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getEcspStat (TRDP_ECSP_STAT_T  *pEcspStat);


/**********************************************************************************************************************/
/**    Function for ECSP confirmation/correction request
 *
 *
 *  @param[in]      pEcspCtrl       Pointer to the ECSP control structure
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_requestEcspConfirm (TRDP_ECSP_CONF_REQUEST_T  *pEcspConfRequest, 
                                            TRDP_ECSP_CONF_REPLY_T    *pEcspConfReply);


#ifdef __cplusplus
}
#endif

#endif /* TAU_CTRL_H */
