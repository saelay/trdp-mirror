/**********************************************************************************************************************/
/**
 * @file            tau_tci.h
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
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id: tau_tci.h 274 2013-01-10 11:00:43Z aweiss $
 *
 */

#ifndef TAU_TTI_H
#define TAU_TTI_H

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


/** Types for train configuration information */

/** inauguration states */
typedef enum
{
    TRDP_INAUG_INVALID       = 0    /**< Ongoing inauguration, DNS not yet available, no address transformation possible */
    TRDP_INAUG_FAULT         = 1,   /**< Error in train inauguration, DNS not available, trainwide communication not possible */
    TRDP_INAUG_NOLEAD_UNCONF = 2,   /**< inauguration done, no leading vehicle set, inauguration unconfirmed */
    TRDP_INAUG_LEAD_UNCONF   = 3,   /**< inauguration done, leading vehicle set, inauguration unconfirmed */
    TRDP_INAUG_LEAD_CONF     = 4,   /**< inauguration done, leading vehicle set, inauguration confirmed */
} TRDP_INAUG_STATE_T;


/** function types */
typedef enum
{
    TRDP_FCT_INVALID         = 0    /**< Invalid type */
    TRDP_FCT_LOCAL           = 1,   /**< Device local function */
    TRDP_FCT_CAR             = 2,   /**< Car control function */
    TRDP_FCT_CST             = 3,   /**< Consist control function */
    TRDP_FCT_TRAIN           = 4,   /**< Train control function */
} TRDP_FCT_T;


/** device information structure */
typedef struct
{
    TRDP_LABEL_T        id;         /**< function identifier (name) */
    TRDP_FCT_T          type;       /**< function type */
    UINT32              no;         /**< unique function number in consist, should be the list index number */
    TRDP_IP_ADDR        addr;       /**< Device IP address/multicast address */
    UINT8               ecnId;      /**< Consist network id the device is connected to  */
    UINT8               etbId;      /**< Ethernet train backbone id */

} TRDP_FCT_INFO_T;


/** properties information structure */
typedef struct
{
    UINT32              crc;        /**< property CRC */
    UINT16              len;        /**< function type */
    UINT8               ver;        /**< property version */
    UINT8               rel;        /**< property release */
    UINT8               data[1];    /**< dummy field for data access */

} TRDP_PROP_INFO_T;


/** device information structure */
typedef struct
{
    TRDP_IP_ADDR        addr1;      /**< First device IP address */
    TRDP_IP_ADDR        addr2;      /**< Second device IP address */
    TRDP_LABEL_T        id;         /**< consist unique device identifier (Label) / host name */
    TRDP_LABEL_T        type;       /**< device type (reserved key words ETBN, ETBR, FCT)  */
    UINT8               orient;     /**< device orientation 0=opposite, 1=same rel. to car */
    TRDP_LABEL_T        redId;      /**< redundant device Id if available */
    UINT8               ecnId1;     /**< First consist network id the device is connected to */
    UINT8               ecnId2;     /**< Second consist network id the device is connected to  */
    UINT8               etbId1;     /**< First Ethernet train backbone id */
    UINT8               etbId2;     /**< Second Ethernet train backbone id */
    UINT16              fctCnt;     /**< number of public functions on the device */
    UINT32             *pFctNo;     /**< Pointer to function number list for application use and convenience */
    UINT16              propLen;    /**< device property length */
    UINT8              *pProp;      /**< Pointer to device properties for application use and convenience */
} TRDP_DEVICE_INFO_T;


/** car information structure. */
typedef struct
{
    TRDP_LABEL_T        id;         /**< Unique car identifier (Label) / IEC identification number */
    TRDP_LABEL_T        type;       /**< car type */
    UINT8               orient;     /**< 0 == opposite, 1 == same orientation rel. to consist */
    UINT8               lead;       /**< 0 == car is not leading */
    UINT8               leadDir;    /**< 0 == leading direction 1, 1 == leading direction 2 */
    UINT8               no;         /**< sequence number of car in consist */
    UINT8               iecNo;      /**< IEC sequence number of car in train */
    UINT8               reachable;  /**< 0 == car not reachable, inserted manually  */
    UINT16              devCnt;     /**< number of devices in the car */
    TRDP_DEVICE_INFO_T *pDevInfo;   /**< Pointer to device info list for application use and convenience. */
    UINT16              propLen;    /**< car property length */
    UINT8              *pProp;      /**< Pointer to car properties for application use and convenience */
} TRDP_CAR_INFO_T;


/** consist information structure. */
typedef struct
{
    TRDP_LABEL_T        id;         /**< Unique consist identifier (Label) / IEC identification number taken from 1st car in consist */
    TRDP_LABEL_T        owner;      /**< consist owner, e.g. "trenitalia.it", "sncf.fr", "db.de" */
    TRDP_UUID_T         uuid;       /**< consist UUID for inauguration purposes */
    UINT8               orient;     /**< opposite(0) or same(1) orientation rel. to train */
    UINT8               lead;       /**< 0 == consist is not leading */
    UINT8               leadDir;    /**< 0 == leading direction 1, 1 == leading direction 2 */
    UINT8               tcnNo;      /**< sequence number of consist in train */
    UINT8               iecNo;      /**< IEC sequence number of consist in train */
    UINT8               reachable;  /**< 0 == consist not reachable, inserted manually */
    UINT8               ecnCnt;     /**< number of cars in the consist */
    UINT8               etbCnt;     /**< number of cars in the consist */
    UINT16              fctCnt;     /**< number of public functions in the consist */
    TRDP_FCT_INFO_T    *pFctInfo;   /**< Pointer to function info list for application use and convenience. */
    UINT16              carCnt;     /**< number of cars in the consist */
    TRDP_CAR_INFO_T    *pCarInfo;   /**< Pointer to car info list for application use and convenience. */
    UINT16              propLen;    /**< consist property length */
    UINT8              *pProp;      /**< Pointer to consist properties for application use and convenience */
} TRDP_CST_INFO_T;


/** train information structure. */
typedef struct
{
    UINT32             version;     /**< Train info structure version */ 
    TRDP_LABEL_T       id;          /**< Train identifier */
    TRDP_LABEL_T       operator;    /**< Train operator e.g. "trenitalia.it", "sncf.fr", "db.de" */
    TRDP_INAUG_STATE_T inaugState;  /**< inaugaration state */
    UINT32             topoCnt;     /**< IEC (i.e. TCN) topography counter */
    UINT8              iecOrient;   /**< 0 == IEC reference orientation is opposite to TCN */
    UINT16             carCnt;      /**< Total number of cars in train */
    UINT32             cstCnt;      /**< Total number of consists in train */
    TRDP_CST_INFO_T   *pCstInfo;    /**< Pointer to consist info list for application use and convenience. */
} TRDP_TRAIN_INFO_T;




/***********************************************************************************************************************
 * PROTOTYPES
 */


/**********************************************************************************************************************/
/*    Train configuration information access                                                                            */
/**********************************************************************************************************************/


/**********************************************************************************************************************/
/**    Function to retrieve the inauguration state and the topography counter.
 * 
 *
 *  @param[out]     pInaugState     Pointer to an inauguration state variable to be returned.
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getEtbState (
    TRDP_INAUG_STATE_T  *pInaugState,
    UINT32              *pTopoCnt); 
 
 
/**********************************************************************************************************************/
/**    Function to retrieve the total number of consists in the train.
 *  
 *
 *  @param[out]     pTrnCstCnt      Pointer to the number of consists to be returned 
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnCstCnt (
    UINT16         *pTrnCstCnt,
    UINT32         *pTopoCnt); 


/**********************************************************************************************************************/
/**    Function to retrieve the total number of consists in the train.
 *  
 *
 *  @param[out]     pTrnCarCnt      Pointer to the number of cars to be returned 
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnCarCnt (
    UINT16         *pTrnCarCnt,
    UINT32         *pTopoCnt); 


/**********************************************************************************************************************/
/**    Function to retrieve the total number of cars in a consist.
 *  
 *
 *  @param[out]     pCstCarCnt      Pointer to the number of cars to be returned
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstCarCnt (
    UINT16               *pCstCarCnt,
    UINT32               *pTopoCnt,
    const TRDP_LABEL_T    cstLabel);


/**********************************************************************************************************************/
/**    Function to retrieve the total number of functions in a consist.
 *  
 *
 *  @param[out]     pCstFctCnt      Pointer to the number of functions to be returned
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstFctCnt (
    UINT16               *pCstFctCnt,
    UINT32               *pTopoCnt,
    const TRDP_LABEL_T    cstLabel);

                                         
/**********************************************************************************************************************/
/**    Function to retrieve the total number of devices in a car.
 *  
 *
 *  @param[out]     pDevCnt         Pointer to the device count to be returned
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      carLabel        Pointer to a car label. NULL means own car if cstLabel == NULL.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCarDevCnt (
    UINT16               *pDevCnt,
    UINT32               *pTopoCnt, 
    const TRDP_LABEL_T    carLabel,
    const TRDP_LABEL_T    cstLabel); 


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to retrieve the function information of the consist. 
 *  
 *
 *  @param[out]     pFctInfo        Pointer to function info list to be returned. Memory needs to be provided by application. 
 *                                  Memory needs to be provided by application. Set NULL if not used.
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *  @param[in]      maxFctCnt       Maximal number of functions to be returned in provided buffer.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstFctInfo (
    TRDP_FCT_INFO_T     *pFctInfo,
    UINT32              *pTopoCnt,  
    const TRDP_LABEL_T   cstLabel,
    UINT16               maxFctCnt);  


/**********************************************************************************************************************/
/**    Function to retrieve the device information of a car's device. 
 *  
 *
 *  @param[out]     pDevInfo        Pointer to device infos to be returned. Memory needs to be provided by application. 
 *  @param[out]     pDevProp        Pointer to application specific device properties to be returned.
 *                                  Memory needs to be provided by application. Set NULL if not used.
 *  @param[out]     pDevFctNo       Pointer to device function number list to be returned. 
 *                                  Memory needs to be provided by application. Set NULL if not used.
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      devLabel        Pointer to a device label. NULL means own device if carLabel ist referring to own car.
 *                                  "devxxx" possible, with xxx = 001...999
 *  @param[in]      carLabel        Pointer to a car label. NULL means own car if cstLabel refers to the own consist.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *  @param[in]      devPropLen      Length of provided buffer for device properties.
 *  @param[in]      devFctCnt       Maximal number of functions to be returned in provided buffer pDevFctNo.
 *  
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getDevInfo (
    TRDP_DEV_INFO_T     *pDevInfo,
    UINT8               *pDevProp,
    UINT32              *pDevFctNo,
    UINT32              *pTopoCnt,  
    const TRDP_LABEL_T   devLabel, 
    const TRDP_LABEL_T   carLabel, 
    const TRDP_LABEL_T   cstLabel,
    UINT32               devPropLen,
    UINT16               devFctCnt);  


/**********************************************************************************************************************/
/**    Function to retrieve the car information of a consist's car. 
 *  
 *
 *  @param[out]     pCarInfo        Pointer to the car info to be returned. Memory needs to be provided by application. 
 *  @param[out]     pCarProp        Pointer to application specific car properties to be returned.
 *                                  Memory needs to be provided by application. Set NULL if not used.
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      carLabel        Pointer to a car label. NULL means own car  if cstLabel refers to own consist.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *  @param[in]      carPropLen      Length of provided buffer for car properties.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCarInfo (
    TRDP_CAR_INFO_T     *pCarInfo,
    UINT8               *pCarProp,
    UINT32              *pTopoCnt,  
    const TRDP_LABEL_T   carLabel, 
    const TRDP_LABEL_T   cstLabel,
    UINT32               carPropLen);  


/**********************************************************************************************************************/
/**    Function to retrieve the consist information of a train's consist. 
 *  
 *
 *  @param[out]     pCstInfo        Pointer to the consist info to be returned. Memory needs to be provided by application. 
 *  @param[out]     pCstProp        Pointer to application specific consist properties to be returned.
 *                                  Memory needs to be provided by application. Set NULL if not used.
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *  @param[in]      cstPropLen      Length of provided buffer for consist properties.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstInfo (
    TRDP_CST_INFO_T     *pCstInfo,
    UINT8               *pCstProp,
    UINT32              *pTopoCnt,
    const TRDP_LABEL_T   cstLabel,
    UINT32               cstPropLen);  


/**********************************************************************************************************************/
/**    Function to retrieve the train information. 
 *  
 *
 *  @param[out]     pTrnInfo        Pointer to the train info to be returned. Memory needs to be provided by application. 
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnInfo (
    TRDP_CST_INFO_T    *pTrnInfo,
    UINT32             *pTopoCnt);  


/* ---------------------------------------------------------------------------- */


**********************************************************************************************************************/
/**    Function to retrieve the orientation of the given car. 
 *  
 *  @param[out]     pCarOrient      Pointer to the car orientation to be returned
 *  @param[out]     pCstOrient      Pointer to the consist orientation to be returned
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      carLabel        carLabel = NULL means own car if cstLabel == NULL
 *  @param[in]      cstLabel        cstLabel = NULL means own consist
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCarOrient (
    UINT8           *pCarOrient,
    UINT8           *pCstOrient,
    UINT32          *pTopoCnt,
    TRDP_LABEL_T     carLabel,  
    TRDP_LABEL_T     cstLabel);


/**********************************************************************************************************************/
/**    Function to retrieve the leading car depending IEC orientation of the given consist. 
 *  
 *  @param[out]     pIecCarOrient   Pointer to the IEC car orientation to be returned
 *  @param[out]     pIecCstOrient   Pointer to the IEC consist orientation to be returned
 *  @param[in,out]  pTopoCnt        Pointer to the actual topo count. If !=0 will be checked. Returns the actual one.
 *  @param[in]      carLabel        carLabel = NULL means own car if cstLabel == NULL
 *  @param[in]      cstLabel        cstLabel = NULL means own consist
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getIecCarOrient (
    UINT8           *pIecCarOrient,
    UINT8           *pIecCstOrient,
    UINT32          *pTopoCnt,
    TRDP_LABEL_T     carLabel,  
    TRDP_LABEL_T     cstLabel);




#ifdef __cplusplus
}
#endif

#endif /* TAU_ADDR_H */
