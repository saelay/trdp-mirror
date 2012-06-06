/**********************************************************************************************************************/
/**
 * @file            tau_addr.h
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
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
 */

#ifndef TAU_ADDR_H
#define TAU_ADDR_H

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
/**	Who am I ?.
 *  Realizes a kind of ëWho am Ií function. It is used to determine the own identifiers (i.e. the own labels),
 *  which may be used as host part of the own fully qualified domain name.
 *
 *  @param[out] 	devId         Returns the device label (host name)
 *  @param[out] 	carId         Returns the car label
 *  @param[out] 	cstId         Returns the consist label
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getOwnIds (
    TRDP_LABEL_T          devId,       
    TRDP_LABEL_T          carId,       
    TRDP_LABEL_T          cstId);
      

/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to get the own IP address.
 * 
 *  @retval         own IP address
 *
 */
EXT_DECL  TRDP_IP_ADDR tau_getOwnAddr (void);   


/**********************************************************************************************************************/
/**	Function to convert a URI to an IP address.
 *  Receives a URI as input variable and translates this URI to an IP-Address. 
 *  The URI may specify either a unicast or a multicast IP-Address.
 *  The caller may specify a topographic counter, which will be checked.
 * 
 *  @param[out]	    pAddr            Pointer to return the IP address
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    uri              Pointer to a URI or an IP Address string, NULL==own URI
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_uri2Addr (
    TRDP_IP_ADDR      *pAddr,
    UINT32            *pTopoCnt,
    const TRDP_URI_T   uri);   


/**********************************************************************************************************************/
/**	Function to convert an IP address to a URI.
 *  Receives an IP-Address and translates it into the host part of the corresponding URI.
 *  Both unicast and multicast addresses are accepted.
 *
 *
 *  @param[out]	    uri              Pointer to a string to return the URI host part
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    addr             IP address, 0==own address
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2Uri (
    TRDP_URI_HOST_T    uri,
    UINT32            *pTopoCnt,
    TRDP_IP_ADDR       addr);  


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to retrieve the carId of the car with label carLabel in the consist with cstLabel.
 *  
 *
 *  @param[out]	    carId            Pointer to a label string to return the car id
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    carLabel         Pointer to the car label. NULL means own car if cstLabel == NULL. 
 *  @param[in]	    cstLabel         Pointer to the consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2CarId (
    TRDP_LABEL_T          carId,
    UINT32               *pTopoCnt,
    const TRDP_LABEL_T    carLabel,
    const TRDP_LABEL_T    cstLabel);    


/**********************************************************************************************************************/
/**	Function The function delivers the car number to the given label. 
 *  The first match of the table will be returned in case there is no unique label given. 
 *  
 *
 *  @param[out]	    pCarNo           Pointer to the car number to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    carLabel         Pointer to the car label. NULL means own car.
 *  @param[in]	    cstLabel         Pointer to the consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2CarNo (
    UINT8                *pCarNo,
    UINT32               *pTopoCnt,
    const TRDP_LABEL_T    carLabel,
    const TRDP_LABEL_T    cstLabel); 


/**********************************************************************************************************************/
/**	Function The function delivers the IEC car number to the given label. 
 *  The first match of the table will be returned in case there is no unique label given. 
 *  
 *
 *  @param[out]	    pIecCarNo        Pointer to the IEC car sequence number to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    carLabel         Pointer to a car label. NULL means own car.
 *  @param[in]	    cstLabel         Pointer to a consist label. NULL menas own consist.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2IecCarNo (
    UINT8               *pIecCarNo,
    UINT32              *pTopoCnt, 
    const TRDP_LABEL_T   carLabel, 
    const TRDP_LABEL_T   cstLabel);  


/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**	Function to retrieve the car and consist id of the car given with carNo and trnCstNo.
 *  
 *
 *  @param[out]	    carId            Pointer to the car id to be returned
 *  @param[out]	    cstId            Pointer to the consist id to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    carNo            Car number in consist. 0 means own car when trnCstNo == 0.
 *  @param[in]	    trnCstNo         Consist sequence number in train. 0 means own consist.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_carNo2Ids (
    TRDP_LABEL_T         carId,  
    TRDP_LABEL_T         cstId,  
    UINT32              *pTopoCnt,
    UINT8                carNo,
    UINT8                trnCstNo);



/**********************************************************************************************************************/
/**	Function to retrieve the car and consist id from a given IEC car sequence number.
 *  
 *
 *  @param[out]	    carId            Pointer to the car id to be returned
 *  @param[out]	    cstId            Pointer to the consist id to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    iecCarNo         Iec car sequence number. 0 means own car.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_iecCarNo2Ids (
    TRDP_LABEL_T     carId,  
    TRDP_LABEL_T     cstId,  
    UINT32          *pTopoCnt,
    UINT8            iecCarNo); 



/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**	Function to retrieve the carId of the car hosting a device with the IPAddress ipAddr.
 *  
 *
 *  @param[out]	    carId            Pointer to the car id to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    ipAddr           IP address. 0 means own address, so the own car id is returned.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2CarId (
    TRDP_LABEL_T         carId,  
    UINT32              *pTopoCnt,
    TRDP_IP_ADDR         ipAddr);


/**********************************************************************************************************************/
/**	Function to retrieve the car number in consist of the car hosting the device with the IP address ipAddr.
 *  
 *
 *  @param[out]	    pCarNo           Pointer to the car number in consist to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    ipAddr           IP address. 0 means own address, so the own car number is returned.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2CarNo (
    UINT8             *pCarNo, 
    UINT8             *pTopoCnt,
    TRDP_IP_ADDR      ipAddr);



/**********************************************************************************************************************/
/**	Function to retrieve the IEC car sequence number of the car hosting the device with the IP address ipAddr.
 *  
 *
 *  @param[out]	    pIecCarNo        Pointer to the IEC car sequence number to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    ipAddr           IP address. 0 means own address, so the own IEC car number is returned.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2IecCarNo (
    UINT8             *pIecCarNo, 
    UINT8             *pTopoCnt,
    TRDP_IP_ADDR      ipAddr);


/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**	Function to retrieve the consist identifier of the consist with train consist sequence number cstNo.
 *  
 *
 *  @param[out]	    cstId            Pointer to the consist id to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    cstNo            Consist sequence number based on IP reference direction. 0 means own consist.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_cstNo2CstId     (
    TRDP_LABEL_T      cstId,     
    UINT32            *pTopoCnt,   
    UINT8             cstNo);


/**********************************************************************************************************************/
/**	Function to retrieve the consist identifier of the consist with IEC sequence consist number iecCstNo.
 *  
 *
 *  @param[out]	    cstId            Pointer to the consist id to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    iecCstNo         Consist sequence number based on the leading car depending iec reference direction.
 *                                   0 means own consist.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_iecCstNo2CstId     (
    TRDP_LABEL_T      cstId,     
    UINT32            *pTopoCnt,   
    UINT8             iecCstNo);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to retrieve the consist identifier of the consist hosting a car with label carLabel.
 *  
 *
 *  @param[out]	    cstId            Pointer to the consist id to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    carLabel         Pointer to a car label. NULL means any car. 
 *  @param[in]	    cstLabel         Pointer to a consist label. NULL means own consist. 
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2CstId (
    TRDP_LABEL_T        cstId,
    UINT32              *pTopoCnt,
    const TRDP_LABEL_T  carLabel,
    const TRDP_LABEL_T  cstLabel); 


/**********************************************************************************************************************/
/**	Function to retrieve the consist sequence number of the consist hosting a car with label carLabel.
 *  
 *
 *  @param[out]	    pCstNo           Pointer to the train consist number to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    carLabel         Pointer to a car label, NULL means own car, so the own consist number is returned.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2CstNo (
    UINT8               *pCstNo,
    UINT32              *pTopoCnt, 
    const TRDP_LABEL_T  carLabel);


/**********************************************************************************************************************/
/**	Function to retrieve the leading car depending IEC consist sequence number of the consist hosting a car with label carLabel.
 *  
 *
 *  @param[out]	    pIecCstNo        Pointer to the iec consist number to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    carLabel         Pointer to a car label. NULL means own car, so the own IEC consist number is returned.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2IecCstNo (
    UINT8               *pIecCstNo,
    UINT32              *pTopoCnt, 
    const TRDP_LABEL_T  carLabel);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to retrieve the consist identifier of the consist hosting the device with the IP-Address ipAddr.
 *  
 *
 *  @param[out]	    cstId            Pointer to the consist id to be returned
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    ipAddr           IP address. 0 means own device, so the own consist id is returned.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2CstId      (
     TRDP_LABEL_T       cstId,     
     UINT32             *pTopoCnt,     
     TRDP_IP_ADDR       ipAddr);


/**********************************************************************************************************************/
/**	Function to retrieve the consist sequence number of the consist hosting the device with the IP-Address ipAddr.
 *  
 *
 *  @param[out]	    pCstNo           Pointer to the train consist number to be returned 
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    ipAddr           IP address. 0 means own device, so the own consist number is returned.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2CstNo (
    UINT8          *pCstNo,
    UINT32         *pTopoCnt, 
    TRDP_IP_ADDR   ipAddr);
  

/**********************************************************************************************************************/
/**	Function to retrieve the  leading car depending iec consist number of the consist hosting the device with the IP-Address addr.
 *  
 *
 *  @param[out]	    pIecCstNo        Pointer to the iec consist number to be returned 
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]	    ipAddr           IP address. 0 means own device, so the own IEC consist number is returned.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2IecCstNo (
    UINT8          *pIecCstNo,
    UINT32         *pTopoCnt, 
    TRDP_IP_ADDR   ipAddr);

/* ---------------------------------------------------------------------------- */



#ifdef __cplusplus
}
#endif

#endif /* TAU_ADDR_H */
