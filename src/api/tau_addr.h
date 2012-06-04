/**********************************************************************************************************************/
/**
 * @file            tau_addr.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details			This module provides the interface to the following utilities
 *					- IP - URI address translation and train configuration access
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
#define TRDP_UIC_CST_PROPERTY_LEN   22
#define TRDP_UIC_IDENTIFIER_LEN     5
#define TRDP_UIC_CAR_PROPERTY_LEN   6
#define TRDP_UIC_CONF_POS_LEN       8

#ifndef __cdecl
#define __cdecl
#endif


/***********************************************************************************************************************
 * TYPEDEFS
 */


/** Types for address conversion and inauguration information */

/** inauguration states */
typedef enum
{
    TRDP_INAUGSTATE_FAULT       = 0 /**< DNS not or not yet available, no address transformation possible */
        TRDP_INAUGSTATE_INVALID = 1, /**< ongoing train inauguration,trainwide communication not possible */
    TRDP_INAUGSTATE_OK = 2,       /**< inauguration done, trainwide communication possible */
} TRDP_INAUGSTATE_T;


/** device information structure */
typedef struct
{
    TRDP_IP_ADDR    ipAddr;      /**< device IP address */
    TRDP_LABEL_T    id;          /**< device identifier (Label) / host name */
    TRDP_LABEL_T    type;        /**< device type */
    UINT8           orient;      /**< device orientation 0=opposite, 1=same rel. to car */
    UINT8           reserved1;   /**< reserved for alignment and future use */
    UINT16          length;      /**< length related to car */
    UINT16          width;       /**< width related to car */
    UINT16          hight;       /**< hight related to car */
} TRDP_DEVICE_INFO_T;


/** car information structure. */
typedef struct
{
    TRDP_LABEL_T        id;          /**< Unique car identifier (Label) / UIC identification nuber */
    TRDP_LABEL_T        type;        /**< Car type */
    UINT8               cstCarNo;    /**< sequence number of car in consist */
    UINT8               trnOrient;   /**< opposite(0) or same(1) orientation rel. to train */
    UINT8               cstOrient;   /**< opposite(0) or same(1) orientation rel. to consist */
    UINT8               reserved1;   /**< reserved for alignment and future use */
    UINT16              devCnt;      /**< number of devices in the car */
    TRDP_DEVICE_INFO_T  devData[1];  /**< device data list. The list size '1' is just a proxy */
                                     /**< definition for the real size (devCnt) in order to satisfy C-Language */
} TRDP_CAR_INFO_T;


#define TRDP_DEVICE_INFO_SIZE(cnt)  (UINT32) (cnt * sizeof (TRDP_DEVICE_INFO_T)))
#define TRDP_CAR_INFO_DEV_OFFSET()  (sizeof (TRDP_CAR_INFO_T) - \
                                     sizeof (TRDP_DEVICE_INFO_T))
#define TRDP_CAR_INFO_SIZE(cnt)     (TDRDP_CAR_INFO_DEV_OFFSET() + \
                                     TRDP_DEVICE_INFO_SIZE(cnt))


/** UIC train information structure. */
typedef struct
{
    UINT32  trnCarCnt;                                 /**< Total number of UIC cars */
    UINT8   confPos[TRDP_UIC_CONF_POS_LEN];            /**< confirmed position of unreachable cars */
    UINT8   confPosAvail;                              /**< 0 if conf. Position is not available */
    UINT8   operatAvail;                               /**< 0 if operator/owner is not available */
    UINT8   natApplAvail;                              /**< 0 if national Application/Version is not available */
    UINT8   cstPropAvail;                              /**< 0 if UIC consist properties are not available */
    UINT8   carPropAvail;                              /**< 0 if UIC car properties are not available */
    UINT8   seatResNoAvail;                            /**< 0 if if reservation number is not available */
    UINT8   inaugFrameVer;                             /**< inauguration frame version, s. Leaflet 556 Ann. C.3 */
    UINT8   rDataVer;                                  /**< supported R-Telegram Version, s. Leaflet 556 Ann. A */
    UINT8   inaugState;                                /**< UIC inaugaration state */
    UINT32  topoCnt;                                   /**< UIC (i.e. TCN) topography counter */
    UINT8   orient;                                    /**< 0 if UIC reference orientation is opposite to IPT */
    UINT8   notAllConf;                                /**< 0 if feature is not available */
    UINT8   confCancelled;                             /**< 0 if feature is not available */
} TRDP_UIC_TRAIN_INFO_T;


/** UIC car information structure. */
typedef struct
{
    UINT8   cstProp[TRDP_UIC_CST_PROPERTY_LEN];          /**< consist properties */
    UINT8   carProp[TRDP_UIC_CAR_PROPERTY_LEN];          /**< car properties */
    UINT8   uicIdent[TRDP_UIC_IDENTIFIER_LEN];           /**< UIC identification number */
    UINT8   cstSeqNo;                                    /**< consist sequence number in UIC Train */
    UINT8   carSeqNo;                                    /**< car sequence number in UIC ref direction */
    UINT16  seatResNo;                                   /**< car number for seat reservation */
    INT8    contrCarCnt;                                 /**< total number of controlled cars in consist */
    UINT8   operat;                                      /**< consist operator type (s. UIC 556) */
    UINT8   owner;                                       /**< consist owner type (s. UIC 556) */
    UINT8   natAppl;                                     /**< national application type (s. UIC 556) */
    UINT8   natVer;                                      /**< national application version (s. UIC 556) */
    UINT8   trnOrient;                                   /**< 0 if car orientation is opposite to Train */
    UINT8   cstOrient;                                   /**< 0 if car orientation is opposite to Consist */
    UINT8   isLeading;                                   /**< 0 if car is not leading */
    UINT8   isLeadRequ;                                  /**< 0 if no leading request */
    UINT8   trnSwInCarCnt;                               /**< total number of train switches in car */
} TRDP_UIC_CAR_INFO_T;


/***********************************************************************************************************************
 * PROTOTYPES
 */


/**********************************************************************************************************************/
/*	IP <-> URI Address Translation  to be reworked                                                                    */
/**********************************************************************************************************************/


/**********************************************************************************************************************/
/**	Function to retrieve the train backbone type.
 *
 *
 *  @param[out]	    pTbType          Pointer to return the train backbone type. 0=ETB, 1= WTB
 *  @param[out]	    pGatewayIpAddr   IP address of active gateway to train backbone.
 *                                   This parameter may be a NULL pointer if the caller is not interested in the address.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnBackboneType (
    UINT8           *pTbType,
    TRDP_IP_ADDR    *pGatewayIpAddr);


/**********************************************************************************************************************/
/**	Function to retrieve the inauguration state and the topography counter.
 *
 *
 *  @param[out]	    pInaugState      Pointer to an inauguration state variable.
 *  @param[out]	    pTopoCnt         Pointer to a topo counter variable.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getEtbState (
    TRDP_INAUGSTATE_T   *pInaugState,
    UINT32              *pTopoCnt);


/**********************************************************************************************************************/
/**	Who am I ?.
 *  Realizes a kind of Who am I function. It is used to determine the own identifiers (i.e. the own labels),
 *  which may be used as host part of the own fully qualified domain name.
 *
 *  @param[out]     devId         Returns the device label (host name)
 *  @param[out]     carId         Returns the car label
 *  @param[out]     cstId         Returns the consist label
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getOwnIds (
    TRDP_LABEL_T    devId,
    TRDP_LABEL_T    carId,
    TRDP_LABEL_T    cstId);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to convert a URI to an IP address.
 *  Receives a URI as input variable and translates this URI to an IP-Address.
 *  The URI may specify either a unicast or a multicast IP-Address.
 *  The caller may specify a topographic counter, which will be checked.
 *
 *  @param[in]	    uri              Pointer to a URI or an IP Address string
 *  @param[out]	    pIpAddr          Pointer to return the IP address
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getAddrByName (
    const TRDP_URI_T    uri,
    TRDP_IP_ADDR        *pIpAddr,
    UINT32              *pTopoCnt);


/**********************************************************************************************************************/
/**	Function to get the host part of an URI.
 *  Receives an IP-Address and translates it into the host part of the corresponding URI;
 *  both unicast and multicast addresses are accepted.
 *  The caller may specify a topographic counter, which will be checked.
 *
 *  @param[in]	    ipAddr           IP address
 *  @param[out]	    uri
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getUriHostPart (
    TRDP_IP_ADDR    ipAddr,
    TRDP_URI_HOST_T uri,
    UINT32          *pTopoCnt);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to convert a label to a carID
 *
 *
 *  @param[in]	    carLabel		Car label
 *  @param[in]	    cstLabel		Consist label
 *  @param[out]	    pCarId			Pointer to the carID returned
 *  @param[in,out]	pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2CarId (
    const TRDP_LABEL_T  carLabel,
    const TRDP_LABEL_T  cstLabel,
    TRDP_LABEL_T        pCarId,
    UINT32              *pTopoCnt);


/**********************************************************************************************************************/
/**	The function delivers the car number to the given label.
 *  The first match of the table will be returned in case there is no unique label given.
 *
 *
 *  @param[in]	    carLabel		Car label
 *  @param[in]	    cstLabel		Consist label
 *  @param[out]	    pCarNo			Pointer to the carNo returned
 *  @param[in,out]	pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2CarNo (
    const TRDP_LABEL_T  carLabel,
    const TRDP_LABEL_T  cstLabel,
    UINT8               *pCarNo,
    UINT32              *pTopoCnt);


/**********************************************************************************************************************/
/**	Function to get the carId from an IP address
 *
 *
 *  @param[in]	    ipAddr			IP address
 *  @param[out]	    carId			Car ID
 *  @param[in,out]	pTopoCnt		Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2CarId (
    TRDP_IP_ADDR    ipAddr,
    TRDP_LABEL_T    carId,
    UINT32          *pTopoCnt);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to
 *
 *
 *  @param[in]	    carLabel
 *  @param[out]	    cstId
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2CstId (
    const TRDP_LABEL_T  carLabel,
    TRDP_LABEL_T        cstId,
    UINT32              *pTopoCnt);


/**********************************************************************************************************************/
/**	Function to
 *
 *
 *  @param[in]	    ipAddr
 *  @param[out]	    cstId
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2CstId (
    TRDP_IP_ADDR    ipAddr,
    TRDP_LABEL_T    cstId,
    UINT32          *pTopoCnt);


/**********************************************************************************************************************/
/**	Function to
 *
 *
 *  @param[in]	    trnCstNo
 *  @param[out]	    cstId
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_cstNo2CstId (
    UINT8           trnCstNo,
    TRDP_LABEL_T    cstId,
    UINT32          *pTopoCnt);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to
 *
 *
 *  @param[in]	    carLabel
 *  @param[out]	    pTrnCstNo
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2TrnCstNo (
    const TRDP_LABEL_T  carLabel,
    UINT8               *pTrnCstNo,
    UINT32              *pTopoCnt);


/**********************************************************************************************************************/
/**	Function to
 *
 *
 *  @param[in]	    ipAddr           IP address
 *  @param[out]	    pTrnCstNo
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2TrnCstNo (
    TRDP_IP_ADDR    ipAddr,
    UINT8           *pTrnCstNo,
    UINT32          *pTopoCnt);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to
 *
 *
 *  @param[out]	    pCstCnt
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnCstCnt (
    UINT8   *pCstCnt,
    UINT32  *pTopoCnt);


/**********************************************************************************************************************/
/**	Function to
 *
 *
 *  @param[in]	    cstLabel
 *  @param[out]	    pCarCnt
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstCarCnt (
    const TRDP_LABEL_T  cstLabel,
    UINT8               *pCarCnt,
    UINT32              *pTopoCnt);


/**********************************************************************************************************************/
/**	Function to
 *
 *
 *  @param[in]	    carLabel
 *  @param[in]	    cstLabel
 *  @param[out]	    pDevCnt
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCarDevCnt (
    const TRDP_LABEL_T  carLabel,
    const TRDP_LABEL_T  cstLabel,
    UINT16              *pDevCnt,
    UINT32              *pTopoCnt);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to tbd
 *
 *
 *  @param[in]	    carLabel		tbd
 *  @param[in]	    cstLabel		tbd
 *  @param[in]	    maxDev			tbd
 *  @param[out]	    pCarData		tbd
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCarInfo (
    const TRDP_LABEL_T  carLabel,
    const TRDP_LABEL_T  cstLabel,
    UINT16              maxDev,
    TRDP_CAR_INFO_T     *pCarData,
    UINT32              *pTopoCnt);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to tbd
 *
 *
 *  @param[out]	    pInaugState		tbd
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getUicState (
    UINT8   * *pInaugState,
    UINT32  * *pTopoCnt);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to tbd
 *
 *
 *  @param[in]	    carSeqNo		tbd
 *  @param[out]	    pCarData		tbd
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getUicCarData (
    UINT8               carSeqNo,
    TRDP_UIC_CAR_DATA_T *pCarData,
    UINT32              *pTopoCnt);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to tbd
 *
 *
 *  @param[in]	    carLabel		tbd
 *  @param[in]	    cstLabel		tbd
 *  @param[out]	    pCarSeqNo		tbd
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_Label2UicCarSeqNo (
    const TRDP_LABEL_T  carLabel,
    const TRDP_LABEL_T  cstLabel,
    UINT8               *pCarSeqNo,
    UINT32              *pTopoCnt);


/**********************************************************************************************************************/
/**	Function to tbd
 *
 *
 *  @param[in]	    ipAddr			tbd
 *  @param[out]	    pCarSeqNo		tbd
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_Addr2UicCarSeqNo (
    TRDP_IP_ADDR    ipAddr,
    UINT8           *pCarSeqNo,
    UINT8           *pTopoCnt);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**	Function to tbd
 *
 *
 *  @param[in]	    carSeqNo		tbd
 *  @param[out]	    carId			tbd
 *  @param[out]	    cstId			tbd
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_UicCarSeqNo2Ids (
    UINT8           carSeqNo,
    TRDP_LABEL_T    carId,
    TRDP_LABEL_T    cstId,
    UINT32          *pTopoCnt);


/**********************************************************************************************************************/
/**	Function to retrieve the orientation of the given car.
 *
 *  @param[in]	    carId           carId = NULL means own car
 *  @param[in]	    cstId           cstId = NULL means own consist
 *  @param[out]	    pCarOrient		tbd
 *  @param[out]	    pCstOrient		tbd
 *  @param[out]	    pUicCarOrient	tbd
 *  @param[out]	    pUicCstOrient	tbd
 *  @param[in,out]	pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCarOrient (
    TRDP_LABEL_T    carId,
    TRDP_LABEL_T    cstId,
    UINT8           *pCarOrient,
    UINT8           *pCstOrient,
    UINT8           *pUicCarOrient,
    UINT8           *pUicCstOrient,
    UINT32          *pTopoCnt);


/**********************************************************************************************************************/
/**	Function to retrieve the orientation of the given consist.
 *
 *  @param[in]	    cstId            cstId = NULL means own consist
 *  @param[out]	    pCstOrient		tbd
 *  @param[out]	    pUicCstOrient	tbd
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCarOrient (
    TRDP_LABEL_T    cstId,
    UINT8           *pCstOrient,
    UINT8           *pUicCstOrient,
    UINT32          *pTopoCnt);


#ifdef __cplusplus
}
#endif

******************************************** /
/**	Function to retrieve the orientation of the given consist.
 *
 *  @param[in]	    cstId            cstId = NULL means own consist
 *  @param[out]	    pCstOrient      tbd
 *  @param[out]	    pUicCstOrient	tbd
 *  @param[in,out]	pTopoCnt         Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR	      no error
 *  @retval         TRDP_PARAM_ERR    Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCarOrient (
    TRDP_LABEL_T    cstId,
    UINT8           *pCstOrient,
    UINT8           *pUicCstOrient,
    UINT32          *pTopoCnt);


#ifdef __cplusplus
}
#endif

#endif /* TAU_ADDR_H */
