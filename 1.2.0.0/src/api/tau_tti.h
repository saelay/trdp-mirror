/**********************************************************************************************************************/
/**
 * @file            tau_tti.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                  - train topology information access
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2014. All rights reserved.
 *
 * $Id$
 *
 */

#ifndef TAU_TTI_H
#define TAU_TTI_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"
#include "tau_tti_types.h"

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
/*    Train configuration information access                                                                          */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**    Function to init TTI access
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initTtiAccess (void);


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory state.
 *
 *
 *  @param[out]     pOpTrDirState   Pointer to an operational train directory state structure to be returned.
 *  @param[out]     pOpTrDir        Pointer to an operational train directory structure to be returned.
 *  @param[in]      etbId           Identifier of the ETB the train directory state is is asked for.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getOpTrDirectory (
    TRDP_OP_TRAIN_DIR_STATE_T         *pOpTrDirState,
    TRDP_OP_TRAIN_DIR_T               *pOpTrDir,
    UINT8                       const  etbId);


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory.
 *
 *
 *  @param[out]     pTrDir          Pointer to a train directory structure to be returned.
 *  @param[in]      etbId           Identifier of the ETB the train directory is requested for.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrDirectory (
    TRDP_TRAIN_DIR_T                  *pTrDir,
    UINT8                       const  etbId);


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory.
 *
 *
 *  @param[out]     pCstInfo        Pointer to a consist info structure to be returned.
 *  @param[in]      cstUUID         UUID of the consist the consist info is rquested for.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getStaticCstInfo (
    TRDP_CONSIST_INFO_T                    *pCstInfo,
    TRDP_UUID_T                      const  cstUUID);


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory.
 *
 *
 *  @param[out]     pOpTrDirState   Pointer to an operational train directory state structure to be returned.
 *  @param[out]     pOpTrDir        Pointer to an operational train directory structure to be returned.
 *  @param[out]     pTrDir          Pointer to a train directory structure to be returned.
 *  @param[out]     pTrNetDir       Pointer to a train network directory structure to be returned.
 *  @param[in]      etbId           Identifier of the ETB the train directory state is requested for.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTTI (
    TRDP_OP_TRAIN_DIR_STATE_T         *pOpTrDirState,
    TRDP_OP_TRAIN_DIR_T               *pOpTrDir,
    TRDP_TRAIN_DIR_T                  *pTrDir,
    TRDP_TRAIN_NET_DIR_T              *pTrNetDir,
    UINT8                             const  etbId);


/**********************************************************************************************************************/
/**    Function to retrieve the total number of consists in the train.
 *
 *
 *  @param[out]     pTrnCstCnt      Pointer to the number of consists to be returned
 *  @param[in,out]  pOpTrTopoCnt    Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnCstCnt (
    UINT16  *pTrnCstCnt,
    UINT32  *pOpTrTopoCnt);


/**********************************************************************************************************************/
/**    Function to retrieve the total number of consists in the train.
 *
 *
 *  @param[out]     pTrnCarCnt      Pointer to the number of cars to be returned
 *  @param[in,out]  pOpTrTopoCnt    Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnCarCnt (
    UINT16  *pTrnCarCnt,
    UINT32  *pOpTrTopoCnt);


/**********************************************************************************************************************/
/**    Function to retrieve the total number of cars in a consist.
 *
 *
 *  @param[out]     pCstCarCnt      Pointer to the number of cars to be returned
 *  @param[in,out]  pOpTrTopoCnt    Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstCarCnt (
    UINT16              *pCstCarCnt,
    UINT32              *pOpTrTopoCnt,
    const TRDP_LABEL_T  cstLabel);


/**********************************************************************************************************************/
/**    Function to retrieve the total number of functions in a consist.
 *
 *
 *  @param[out]     pCstFctCnt      Pointer to the number of functions to be returned
 *  @param[in,out]  pOpTrTopoCnt    Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstFctCnt (
    UINT16              *pCstFctCnt,
    UINT32              *pOpTrTopoCnt,
    const TRDP_LABEL_T  cstLabel);


/**********************************************************************************************************************/
/**    Function to retrieve the total number of devices in a car.
 *
 *
 *  @param[out]     pDevCnt         Pointer to the device count to be returned
 *  @param[in,out]  pOpTrTopoCnt    Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      vehLabel        Pointer to a vehicle label. NULL means own vehicle if cstLabel == NULL.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCarDevCnt (
    UINT16              *pDevCnt,
    UINT32              *pOpTrTopoCnt,
    const TRDP_LABEL_T  vehLabel,
    const TRDP_LABEL_T  cstLabel);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to retrieve the function information of the consist.
 *
 *
 *  @param[out]     pFctInfo        Pointer to function info list to be returned.
 *                                  Memory needs to be provided by application. Set NULL if not used.
 *  @param[in,out]  pOpTrTopoCnt    Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *  @param[in]      maxFctCnt       Maximal number of functions to be returned in provided buffer.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstFctInfo (
    TRDP_FUNCTION_INFO_T    *pFctInfo,
    UINT32                  *pOpTrTopoCnt,
    const TRDP_LABEL_T      cstLabel,
    UINT16                  maxFctCnt);


/**********************************************************************************************************************/
/**    Function to retrieve the car information of a consist's car.
 *
 *
 *  @param[out]     pVehInfo        Pointer to the vehicle info to be returned. 
 *  @param[in,out]  pOpTrTopoCnt    Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      vehLabel        Pointer to a vehicle label. NULL means own vehicle  if cstLabel refers to own consist.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *  @param[in]      carPropLen      Size of properties
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getVehInfo (
    TRDP_VEHICLE_INFO_T     *pVehInfo,
    UINT32                  *pOpTrTopoCnt,
    const TRDP_LABEL_T      vehLabel,
    const TRDP_LABEL_T      cstLabel,
    UINT32                  carPropLen);


/**********************************************************************************************************************/
/**    Function to retrieve the consist information of a train's consist.
 *
 *
 *  @param[out]     pCstInfo        Pointer to the consist info to be returned.
 *  @param[in,out]  pOpTrTopoCnt    Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstInfo (
    TRDP_CONSIST_INFO_T     *pCstInfo,
    UINT32                  *pOpTrTopoCnt,
    const TRDP_LABEL_T      cstLabel);


/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**    Function to retrieve the orientation of the given vehicle.
 *
 *  @param[out]     pCarOrient      Pointer to the vehicle orientation to be returned
 *  @param[out]     pCstOrient      Pointer to the consist orientation to be returned
 *  @param[in,out]  pOpTrTopoCnt      Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      vehLabel        vehLabel = NULL means own vehicle if cstLabel == NULL
 *  @param[in]      cstLabel        cstLabel = NULL means own consist
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getVehOrient (
    UINT8           *pCarOrient,
    UINT8           *pCstOrient,
    UINT32          *pOpTrTopoCnt,
    TRDP_LABEL_T    vehLabel,
    TRDP_LABEL_T    cstLabel);


/**********************************************************************************************************************/
/**    Function to retrieve the leading car depending IEC orientation of the given consist.
 *
 *  @param[out]     pIecCarOrient   Pointer to the IEC car orientation to be returned
 *  @param[out]     pIecCstOrient   Pointer to the IEC consist orientation to be returned
 *  @param[in,out]  pOpTrTopoCnt    Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      vehLabel        vehLabel = NULL means own vehicle if cstLabel == NULL
 *  @param[in]      cstLabel        cstLabel = NULL means own consist
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getIecCarOrient (
    UINT8           *pIecCarOrient,
    UINT8           *pIecCstOrient,
    UINT32          *pOpTrTopoCnt,
    TRDP_LABEL_T    vehLabel,
    TRDP_LABEL_T    cstLabel);




#ifdef __cplusplus
}
#endif

#endif /* TAU_TTI_H */
