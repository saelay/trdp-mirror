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

/** ETB information */
typedef struct
{
    UINT8                   etbId;          /**< identification of train backbone; value range: 0..3 */
    UINT8                   ecnCnt;         /**< number of ECN's within consist connected to this ETB */
    UINT16                  reserved01;     /**< reserved for future use (= 0) */
} TRDP_ETB_INFO_T;

/** Closed train consists information */
typedef struct
{
    UINT8                   cltrCstUUID[16];    /**< closed train consist UUID */
    UINT8                   cltrCstOrient;      /**< closed train consist orientation
                                                     ´01`B = same as closed train direction
                                                     ´10`B = inverse to closed train direction */
    UINT8                   cltrCstNo;          /**< sequence number of the consist within the
                                                     closed train, value range 1..32 */
    UINT16                  reserved01;         /**< reserved for future use (= 0) */
} TRDP_CLTRCST_INFO_T;


/** Application defined properties */
typedef struct
{
    TRDP_SHORT_VERSION_T    ver;            /**< properties version information, application defined */
    UINT16                  len;            /**< properties length in number of octets,
                                                 application defined, must be a multiple
                                                 of 4 octets for alignment reasons 
                                                 value range: 0..32768  */
    UINT8                  *pProp;          /**< properties, application defined */
} TRDP_PROP_T;


/** function/device information structure */
typedef struct
{
    TRDP_LABEL_T            fctName;        /**< function device or group label */
    UINT16                  fctId;          /**< unique host identification of the function
                                                 device or group in the consist as defined in
                                                 IEC 61375-2-5, application defined. Value range: 1..16383 */
    BOOL8                   grp;            /**< is a function group and will be resolved as IP multicast address */ 
    UINT8                   reserved01;     /**< reserved for future use (= 0) */
    UINT8                   cstVehNo;       /**< Sequence number of the vehicle in the
                                                 consist the function belongs to. Value range: 1..16, 0 = not defined  */
    UINT8                   etbId;          /**< number of connected train backbone. Value range: 0..3 */
    UINT8                   ecnId;          /**< number of connected consist network in the consist, 
                                                 related to the etbId. Value range: 0..15 */
    UINT8                   reserved02;     /**< reserved for future use (= 0) */
    TRDP_PROP_T             fctProp;        /**< properties, application defined */
} TRDP_FUNCTION_INFO_T;


/** vehicle information structure */
typedef struct
{
    TRDP_LABEL_T            vehId;          /**< vehicle identifier label,application defined
                                                 (e.g. UIC vehicle identification number) */
    TRDP_LABEL_T            vehType;        /**< vehicle type,application defined */
    UINT8                   vehOrient;      /**< vehicle orientation
                                                 ´01`B = same as consist direction
                                                 ´10`B = inverse to consist direction */
    UINT8                   vehNo;          /**< Sequence number of vehicle in consist(1..16) */
    ANTIVALENT8             tracVeh;        /**< vehicle is a traction vehicle
                                                 ´01`B = vehicle is not a traction vehicle
                                                 ´10`B = vehicle is a traction vehicle */
    UINT8                   reserved01;     /**< for future use (= 0) */
    TRDP_PROP_T             vehProp;        /**< static vehicle properties */ 
} TRDP_VEHICLE_INFO_T;


/** consist information structure */
typedef struct
{
    UINT32                  totalLength;    /**< total length of data structure in number of octets */
    TRDP_SHORT_VERSION_T    version;        /**< ConsistInfo data structure version, application defined */
    UINT8                   cstClass;       /**< consist info classification
                                                 0 = (single) consist
                                                 1 = closed train
                                                 2 = closed train consist */
    UINT8                   reserved01;     /**< reserved for future use (= 0) */
    TRDP_LABEL_T            cstId;          /**< consist identifier label, application defined
                                                 (e.g. UIC vehicle identification number
                                                 of the vehicle at extremity 1 of the consist) */
    TRDP_LABEL_T            cstType;        /**< consist type, application defined */
    TRDP_LABEL_T            cstOwner;       /**< consist owner, e.g. "trenitalia.it", "sncf.fr", "db.de" */
    TRDP_UUID_T             cstUuid;        /**< consist UUID  */
    UINT32                  reserved02;     /**< reserved for future use (= 0) */
    TRDP_PROP_T             cstProp;        /**< static consist properties */
    UINT16                  reserved03;     /**< reserved for future use (= 0) */
    UINT16                  etbCnt;         /**< number of ETB’s, range: 1..4 */
    TRDP_ETB_INFO_T        *pEtbInfoList;   /**< ETB information list for the consist */

    UINT16                  reserved04;     /**< reserved for future use (= 0) */
    UINT16                  vehCnt;         /**< number of vehicles in consist 1..12 */
    TRDP_VEHICLE_INFO_T    *pVehInfoList;   /**< vehicle info list for the vehicles in the consist 
                                                 value range: 0..32 */
    UINT16                  reserved05;     /**< reserved for future use (= 0) */
    UINT16                  fctCnt;         /**< number of consist functions
                                                 value range 0..1024 */
    TRDP_FUNCTION_INFO_T   *pFctInfoList;   /**< function info list for the functions in consist */
    UINT16                  reserved06;     /**< reserved for future use (= 0) */
    UINT16                  cltrCstCnt;     /**< number of original consists in closed train 
                                                 value range: 0..32, 0 = consist is no closed train */
    TRDP_CLTRCST_INFO_T    *pCltrCstInfoList;  /**< info on closed train composition */
    UINT32                 cstTopoCnt;      /**< sc-32 computed over record, seed value: 'FFFFFFFF'H */
} TRDP_CONSIST_INFO_T;


/* Consist info list */
typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< ConsistInfoList structure version  
                                                  parameter ‘mainVersion’ shall be set to 1. */
    UINT16                  cstInfoCnt;     /**< number of consists in train; range: 1..63 */
    TRDP_CONSIST_INFO_T    *pCstInfoList;   /**< consist info collection cstCnt elements */
} TRDP_CONSIST_INFO_LIST_T;


/** TCN consist structure */
typedef struct
{
    TRDP_UUID_T             cstUuid;        /**< Reference to static consist attributes,
                                                 0 if not available (e.g. correction) */
    UINT8                   trnCstNo;       /**< Sequence number of consist in train (1..63) */
    UINT8                   cstOrient;      /**< consist orientation
                                                 ´01`B = same as train direction
                                                 ´10`B = inverse to train direction */
    UINT8                   reserved01;     /**< reserved for future use (= 0) */
} TRDP_CONSIST_T;


/** TCN train directory */
typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< TrainDirectory data structure version  
                                                  parameter ‘mainVersion’ shall be set to 1. */
    BITSET8                 etbId;          /**< identification of the ETB the TTDB is computed for
                                                 bit0: ETB0 (operational network)
                                                 bit1: ETB1 (multimedia network)
                                                 bit2: ETB2 (other network)
                                                 bit3: ETB3 (other network) */
    UINT8                   reserved01;     /**< reserved for future use (= 0) */
    UINT16                  cstCnt;         /**< number of consists in train; range: 1..63 */
    TRDP_CONSIST_T         *pCstDirList;    /**< dynamic consist list ordered list starting with trnCstNo = 1 */
    UINT32                  trnTopoCnt;     /**< sc-32 computed over record (seed value: etbTopoCnt) */
} TRDP_TRAIN_DIRECTORY_T;


/** UIC train directory state */
typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< TrainDirectoryState data structure version  
                                                  parameter ‘mainVersion’ shall be set to 1. */
    UINT16                  reserved01;     /**< reserved for future use (= 0) */
    BITSET8                 etbId;          /**< identification of the ETB the TTDB is computed for
                                                 bit0: ETB0 (operational network)
                                                 bit1: ETB1 (multimedia network)
                                                 bit2: ETB2 (other network)
                                                 bit3: ETB3 (other network) */
    UINT8                   trnDirState;    /**< TTDB status: ´01`B == unconfirmed, ´10`B == confirmed */
    UINT8                   opTrnDirState;  /**< TTDB status: ´01`B == inalid, ´10`B == valid */
    UINT8                   reserved02;     /**< reserved for future use (= 0) */
    TRDP_LABEL_T            trnId;          /**< train identifier, application defined
                                                 (e.g. “ICE75”, “IC346”), informal */
    TRDP_LABEL_T            trnOperator;    /**< train operator, e.g. “trenitalia.it”, informal */
    UINT32                  opTrnTopoCnt;   /**< operational train topology counter
                                                 set to 0 if opTrnDirState == invalid */
    UINT32                  crc;            /**< sc-32 computed over record (seed value: ‘FFFFFFFF’H) */
} TRDP_OP_TRAIN_DIRECTORY_STATE_T;


/** UIC operational vehicle structure */
typedef struct
{
    TRDP_LABEL_T            vehId;          /**< Unique vehicle identifier, application defined (e.g. UIC Identifier) */
    UINT8                   opVehNo;        /**< operational number in train (1..63) */
    ANTIVALENT8             isLead;         /**< vehicle is leading */
    UINT8                   leadDir;        /**< ´01`B = leading direction 1, ´10`B = leading direction 2 */
    ANTIVALENT8             reachable;      /**< vehicle reachable over ETB */
    UINT8                   vehOrient;      /**< vehicle orientation, 
                                                 ´01`B = same as operational train direction
                                                 ´10`B = inverse to operational train direction */
    UINT8                   ownCstNo;       /**< operational consist number the vehicle belongs to */
    UINT16                  reserved01;     /**< reserved for future use (= 0) */
} TRDP_OP_VEHICLE_T;


/** UIC operational consist structure */
typedef struct
{
    TRDP_UUID_T             cstUuid;        /**< Reference to static consist attributes, 
                                                 0 if not available (e.g. correction) */
    UINT8                   cstIndex;       /**< Index of consist in consist info list, only for performance reason
                                                 in any case cstUUID needs to be checked in parallel */
    UINT8                   opCstNo;        /**< operational consist number in train (1..63) */
    UINT8                   opCstOrient;    /**< consist orientation
                                                 ´01`B = same as operational train direction
                                                 ´10`B = inverse to operational train direction */
    UINT8                   reserved01;     /*< reserved for future use (= 0) */
} TRDP_OP_CONSIST_T;


/** UIC operational train structure */
typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< Train info structure version */
    BITSET8                 etbId;          /**< identification of the ETB the TTDB is computed for
                                                 bit0: ETB0 (operational network)
                                                 bit1: ETB1 (multimedia network)
                                                 bit2: ETB2 (other network)
                                                 bit3: ETB3 (other network) */
    UINT8                   opTrnOrient;    /**< operational train orientation
                                                 ‘00’B = unknown
                                                 ´01`B = same as train direction
                                                 ´10`B = inverse to train direction */
    UINT16                  reserved01;     /**< reserved for future use (= 0) */
    UINT16                  opCstCnt;       /**< number of consists in train (1..63) */
    TRDP_OP_CONSIST_T      *pOpCstList;     /**< Pointer to operational consist list starting with op. consist #1 */
    UINT16                  reserved02;     /**< reserved for future use (= 0) */
    UINT16                  opVehCnt;       /**< number of vehicles in train (1..63) */
    TRDP_OP_VEHICLE_T      *pOpVehList;     /**< Pointer to operational vehicle list starting with op. vehicle #1 */
    UINT32                  opTrnTopoCnt;   /**< operational train topology counter 
                                                 SC-32 computed over record (seed value : trnTopoCnt) */
} TRDP_OP_TRAIN_DIRECTORY_T;


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
 *  @param[out]     pTrDirState     Pointer to an operational train directory state structure to be returned.
 *  @param[out]     pOpTrDir        Pointer to an operational train directory structure to be returned.
 *  @param[in]      etbId           Identifier of the ETB the train directory state is is asked for.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getOpTrDirectory (
    TRDP_OP_TRAIN_DIRECTORY_STATE_T         *pOpTrDirState,
    TRDP_OP_TRAIN_DIRECTORY_T               *pOpTrDir,
    UINT8                            const  etbId);


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
    TRDP_TRAIN_DIRECTORY_T                  *pTrDir,
    UINT8                            const  etbId);


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
/**    Function to retrieve the operational train directory state.
 *
 *
 *  @param[out]     pTrDirState     Pointer to an operational train directory state structure to be returned.
 *  @param[out]     pOpTrDir        Pointer to an operational train directory structure to be returned.
 *  @param[out]     pTrDir          Pointer to a train directory structure to be returned.
 *  @param[out]     pCstInfoList    Pointer to a consist info list structure to be returned.
 *  @param[in]      etbId           Identifier of the ETB the train directory state is requested for.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTTI (
    TRDP_OP_TRAIN_DIRECTORY_STATE_T         *pOpTrDirState,
    TRDP_OP_TRAIN_DIRECTORY_T               *pOpTrDir,
    TRDP_TRAIN_DIRECTORY_T                  *pTrDir,
    TRDP_CONSIST_INFO_LIST_T                *pCstInfoList,
    UINT8                            const  etbId);


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
    UINT32              carPropLen);


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
 *  @param[out]     pVehOrient      Pointer to the vehicle orientation to be returned
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
