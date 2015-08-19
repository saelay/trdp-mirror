/**********************************************************************************************************************/
/**
 * @file            tau_dnr.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                  - IP - URI address translation 
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

#ifndef TAU_DNR_H
#define TAU_DNR_H

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


/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/**    Function to init DNR
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      ecspIpAddr      ECSP IP address.
 *  @param[in]      ecspPort        ECSP port number.
 *  @param[in]      hostFileName    Optional host file name as ECSP replacement.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initDnr (
	TRDP_APP_SESSION_T appHandle,
    TRDP_IP_ADDR_T     ecspIpAddr,
    UINT32             ecspPort,
    CHAR8             *hostFileName);


/**********************************************************************************************************************/
/**    Who am I ?.
 *  Realizes a kind of 'Who am I' function. It is used to determine the own identifiers (i.e. the own labels),
 *  which may be used as host part of the own fully qualified domain name.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     devId           Returns the device label (host name)
 *  @param[out]     vehId           Returns the vehicle label
 *  @param[out]     cstId           Returns the consist label
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getOwnIds (
   	TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         devId,       
    TRDP_LABEL_T         vehId,       
    TRDP_LABEL_T         cstId);
      

/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to get the own IP address.
 * 
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *
 *  @retval         own IP address
 *
 */
EXT_DECL  TRDP_IP_ADDR_T tau_getOwnAddr (
    	TRDP_APP_SESSION_T   appHandle);   


/**********************************************************************************************************************/
/**    Function to convert a URI to an IP address.
 *  Receives a URI as input variable and translates this URI to an IP-Address. 
 *  The URI may specify either a unicast or a multicast IP-Address.
 *  The caller may specify a topographic counter, which will be checked.
 * 
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pAddr           Pointer to return the IP address
 *  @param[in]      uri             Pointer to a URI or an IP Address string, NULL==own URI
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_uri2Addr (
	TRDP_APP_SESSION_T   appHandle,
	TRDP_IP_ADDR_T      *pAddr,
    const TRDP_URI_T     uri);   


/**********************************************************************************************************************/
/**    Function to convert an IP address to a URI.
 *  Receives an IP-Address and translates it into the host part of the corresponding URI.
 *  Both unicast and multicast addresses are accepted.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     uri             Pointer to a string to return the URI host part
 *  @param[in]      addr            IP address, 0==own address
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2Uri (
	TRDP_APP_SESSION_T   appHandle,
    TRDP_URI_HOST_T      uri,
    TRDP_IP_ADDR_T       addr);  


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to retrieve the vehId of the car with label vehLabel in the consist with cstLabel.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     vehId           Pointer to a label string to return the vehicle id
 *  @param[in]      vehLabel        Pointer to the vehicle label. NULL means own vehicle if cstLabel == NULL. 
 *  @param[in]      cstLabel        Pointer to the consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2VehId (
	TRDP_APP_SESSION_T    appHandle,
    TRDP_LABEL_T          vehId,
    const TRDP_LABEL_T    vehLabel,
    const TRDP_LABEL_T    cstLabel);    


/**********************************************************************************************************************/
/**    Function The function delivers the TCN vehicle number to the given label. 
 *  The first match of the table will be returned in case there is no unique label given. 
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTcnVehNo       Pointer to the TCN vehicle number to be returned
 *  @param[in]      vehLabel        Pointer to the vehicle label. NULL means own vehicle.
 *  @param[in]      cstLabel        Pointer to the consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2TcnVehNo (
	TRDP_APP_SESSION_T    appHandle,
    UINT8                *pTcnVehNo,
    const TRDP_LABEL_T    vehLabel,
    const TRDP_LABEL_T    cstLabel); 


/**********************************************************************************************************************/
/**    Function The function delivers the operational veheicle sequence number to the given label. 
 *  The first match of the table will be returned in case there is no unique label given. 
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpVehNo        Pointer to the operational vehicle sequence number to be returned
 *  @param[in]      vehLabel        Pointer to a vehicle label. NULL means own vehicle.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL menas own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2OpVehNo (
	TRDP_APP_SESSION_T   appHandle,
    UINT8               *pOpVehNo,
    const TRDP_LABEL_T   vehLabel, 
    const TRDP_LABEL_T   cstLabel);  


/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**    Function to retrieve the car and consist id of the car given with carNo and trnCstNo.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     vehId           Pointer to the vehicle id to be returned
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      tcnVehNo        Vehicle number in consist. 0 means own vehicle when trnCstNo == 0.
 *  @param[in]      tcnCstNo        TCN consist sequence number in train. 0 means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_tcnVehNo2Ids (
	TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         vehId,  
    TRDP_LABEL_T         cstId,  
    UINT8                tcnVehNo,
    UINT8                tcnCstNo);



/**********************************************************************************************************************/
/**    Function to retrieve the vehicle and consist id from a given operational vehicle sequence number.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     vehId           Pointer to the vehicle id to be returned
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      opVehNo         Operational vehicle sequence number. 0 means own vehicle.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_opVehNo2Ids (
	TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         vehId,  
    TRDP_LABEL_T         cstId,  
    UINT8                opVehNo); 



/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**    Function to retrieve the vehId of the car hosting a device with the IPAddress ipAddr.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     vehId           Pointer to the vehicle id to be returned
 *  @param[in]      ipAddr          IP address. 0 means own address, so the own vehicle id is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2VehId (
	TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         vehId,  
    TRDP_IP_ADDR_T       ipAddr);


/**********************************************************************************************************************/
/**    Function to retrieve the TCN vehicle number in consist of the car hosting the device with the given IP address
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTcnVehNo       Pointer to the TCN vehicle number in consist to be returned
 *  @param[in]      ipAddr          IP address. 0 means own address, so the own vehicle number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2TcnVehNo (
	TRDP_APP_SESSION_T   appHandle,
    UINT8               *pTcnVehNo, 
    TRDP_IP_ADDR_T       ipAddr);



/**********************************************************************************************************************/
/**    Function to retrieve the operational vehicle number of the vehicle hosting the device with the given IP address
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpVehNo        Pointer to the operational vehicle number to be returned
 *  @param[in]      ipAddr          IP address. 0 means own address, so the own operational vehicle number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2OpVehNo (
	TRDP_APP_SESSION_T   appHandle,
    UINT8               *pOpVehNo, 
    TRDP_IP_ADDR_T       ipAddr);


/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**    Function to retrieve the consist identifier of the consist with train consist sequence number cstNo.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      tcnCstNo        Consist sequence number based on IP reference direction. 0 means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_tcnCstNo2CstId     (
	TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         cstId,     
    UINT8                tcnCstNo);


/**********************************************************************************************************************/
/**    Function to retrieve the consist identifier of the consist with IEC sequence consist number iecCstNo.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      opCstNo         Operational consist sequence number based on the leading car.
 *                                  0 means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_iecCstNo2CstId     (
	TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         cstId,     
    UINT8                opCstNo);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to retrieve the consist identifier of the consist hosting a car with label vehLabel.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      vehLabel        Pointer to a vehicle label. NULL means any car. 
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist. 
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2CstId (
	TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         cstId,
    const TRDP_LABEL_T   vehLabel,
    const TRDP_LABEL_T   cstLabel); 


/**********************************************************************************************************************/
/**    Function to retrieve the TCN consist sequence number of the consist hosting a vehicle with label vehLabel.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTcnCstNo       Pointer to the TCN consist number to be returned
 *  @param[in]      vehLabel        Pointer to a vehicle label, NULL means own vehicle, so the own consist number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2TcnCstNo (
	TRDP_APP_SESSION_T   appHandle,
    UINT8               *pTcnCstNo,
    const TRDP_LABEL_T   vehLabel);


/**********************************************************************************************************************/
/**    Function to retrieve the operational consist sequence number of the consist hosting a vehicle with label vehLabel.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpCstNo        Pointer to the operational consist number to be returned
 *  @param[in]      vehLabel        Pointer to a vehicle label. NULL means own vehicle, so the own IEC consist number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2OpCstNo (
	TRDP_APP_SESSION_T   appHandle,
    UINT8               *pOpCstNo,
    const TRDP_LABEL_T   vehLabel);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to retrieve the consist identifier of the consist hosting the device with the IP-Address ipAddr.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      ipAddr          IP address. 0 means own device, so the own consist id is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2CstId      (
	TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         cstId,     
    TRDP_IP_ADDR_T       ipAddr);


/**********************************************************************************************************************/
/**    Function to retrieve the TCN consist number of the consist hosting the device with the IP-Address ipAddr.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTcnCstNo       Pointer to the TCN consist number to be returned 
 *  @param[in]      ipAddr          IP address. 0 means own device, so the own consist number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2TcnCstNo (
	TRDP_APP_SESSION_T   appHandle,
    UINT8               *pTcnCstNo,
    TRDP_IP_ADDR_T       ipAddr);
  

/**********************************************************************************************************************/
/**    Function to retrieve the operational consist number of the consist hosting the device with the IP-Address ipAddr.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpCstNo        Pointer to the operational consist number to be returned 
 *  @param[in]      ipAddr          IP address. 0 means own device, so the own IEC consist number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2OpCstNo (
	TRDP_APP_SESSION_T   appHandle,
	UINT8				*pOpCstNo,
    TRDP_IP_ADDR_T		 ipAddr);

/* ---------------------------------------------------------------------------- */



#ifdef __cplusplus
}
#endif

#endif /* TAU_DNR_H */
