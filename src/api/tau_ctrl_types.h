/**********************************************************************************************************************/
/**
 * @file            tau_ctrl_types.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following
 *                  - ETB control type definitions acc. to IEC61375-2-3
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

#ifndef TAU_CTRL_TYPES_H
#define TAU_CTRL_TYPES_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#include "trdp_types.h"
#include "tau_tti.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

#ifdef WIN32
#pragma pack(push, 1)
#endif

/** Types for ETB control */

typedef struct
{
    UINT8                   trnVehNo;       /**< vehicle sequence number within the train
 	                                             with vehicle 01 being the first vehicle
                                                 in ETB reference direction 1 as defined in IEC61375-2-5
                                                 value range: 0..63
                                                 a value of 0 indicates that this vehicle
                                                 has been inserted by correction */
    ANTIVALENT8             isLead;         /**< vehicle is leading */
    UINT8                   leadDir;        /**< vehicle leading direction
                                                 0 = not relevant
                                                 1 = leading direction 1
                                                 2 = leading direction 2 */
    UINT8                   vehOrient;      /**< vehicle orientation
                                                 0 = not known (corrected vehicle)
                                                 1 = same as operational train direction
                                                 2 = inverse to operational train direction */
    
}  GNU_PACKED TRDP_CONF_VEHICLE_T;

typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< data structure version, parameter 'mainVersion' shall be set to 1. */ 
    UINT16                  reserved01;     /**< reserved (=0) */
    UINT8                   trnCstNo;       /**< own TCN consist number (= 1..32) */
    UINT8                   trnClTrNo;      /**< own TCN closed train sequence number (= 1..32) */
    UINT8                   ownOpCstNo;     /**< own operational address (= 1..32) = 0 if unknown (e.g. after Inauguration) */
    UINT8                   reserved02;     /**< reserved (=0) */
    UINT32                  cstTopoCount;   /**< Consist topology counter */
    UINT32                  trnTopoCount;   /**< Train directory topology counter */
    UINT32                  opTopoCount;    /**< Operational Train directory topology counter */
    ANTIVALENT8             wasLead;        /**< consist was leading, 
                                                 '01'B = false, '10'B = true */
    ANTIVALENT8             reqLead;        /**< leading request,
                                                 '01'B = false, '10'B = true */
    UINT8                   reqLeadDir;     /**< (request) leading direction, 
                                                 '01'B = consist direction 1,
                                                 '10'B = consist direction 2 */
    ANTIVALENT8             accLead;        /**< accept remote leading request,
                                                 '01'B = false/not accepted,
                                                 '10'B = true/accepted */
    ANTIVALENT8             isLead;         /**< consist contains leading vehicle, 
                                                 '01'B = false, '10'B = true */
    ANTIVALENT8             clearConfComp;  /**< clear confirmed composition,
                                                 '01'B = false, '10'B = true */
    ANTIVALENT8             compUnknown;    /**< request confirmation,
                                                 '01'B = false, '10'B = true */
    ANTIVALENT8             corrInfoSet;    /**< correction info set,
                                                 '01'B = false, '10'B = true */
    ANTIVALENT8             compStored;     /**< corrected composition stored,
                                                 '01'B = false, '10'B = true */
    ANTIVALENT8             sleepRequest;   /**< request sleep mode,
                                                 '01'B = false, '10'B = true */
    UINT8                   leadVehOfCst;   /**< position of leading vehicle in consist, 
                                                 0..31 (0: first vehicle in consist in Direction 1, 1: second vehicle, etc.) */
    UINT8                   reqCstInfo;     /**< rCi: request CSTINFO telegram exchange */
    UINT16                  reserved03;     /**< reserved (=0) */
    UINT16                  confVehCnt;     /**< number of confirmed vehicles in train (1..32) */
    TRDP_CONF_VEHICLE_T    *pConfVehList;   /**< dynamic ordered list of confirmed vehicles in train,
                                                 starting with vehicle at train head, see sub-clause 5.3.3.2.6 */
}  GNU_PACKED TRDP_ECSP_CTRL_T;


typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< data structure version, parameter 'mainVersion' shall be set to 1. */
    UINT16                  reserved01;     /**< reserved (=0) */
    UINT16                  lifesign;       /**<  wrap-around counter, incremented with each produced datagram. */
    UINT8                   ecspState;      /**<  ECSP state indication
                                                  0 = ECSP not operational(initial value)
                                                  1 = ECSP in operation */
    UINT8                   etbnState;      /**<  state indication of the (active) ETBN
                                                  0 = ETBN not operational(initial value)
                                                  1 = ETBN in operation */
    UINT8                   etbInhibit;     /**<  inauguration inhibit indication
                                                  0 = n/a (default)
                                                  1 = inhibit not requested on ETB
                                                  2 = inhibit set on local ETBN
                                                  3 = inhibit set on remote ETBN
                                                  4 = inhibit set on local and remote ETBN */
    UINT8                   etbLength;      /**<  indicates train lengthening in case train inauguration is inhibit
                                                  0 = no lengthening (default)
                                                  1 = lengthening detected */
    UINT8                   etbnPosition;   /**<  position of the ETBN
                                                  0 = unknown (default)
                                                  1 = single node
                                                  2 = middle node
                                                  3 = end node TCN direction 1
                                                  4 = end node TCN direction 2 */
    BITSET8                 etbLineState;   /**<  indication of ETB line status
                                                  bit0 = line A ETBN direction 1 trusted
                                                  bit1 = line B ETBN direction 1 trusted
                                                  bit2 = line A ETBN direction 2 trusted
                                                  bit3 = line B ETBN direction 2 trusted
                                                  bit4..7 = reserved (= 0)  */
    UINT8                   reserved02;     /**<  reserved (= 0) */
    UINT8                   etbLeadState;   /**<  indication of local consist leadership
                                                   5 = consist not leading (initial value)
                                                   6 = consist is leading requesting
                                                   9 = consist is leading
                                                  10 = leading conflict
                                                  other values are not allowed */
    UINT8                   etbLeadDir;     /**<  direction of the leading end car in the local consist
                                                   0 = unknown (default)
                                                   1 = TCN direction 1
                                                   2 = TCN direction 2
                                                   other values are not allowed */
    UINT8                   ttdbSrvState;   /**<  TTDB server state indication
                                                   0 = n/a (initial value)
                                                   1 = Leader (default)
                                                   2 = Follower
                                                   3 = Error */
    UINT8                   dnsSrvState;    /**<  DNS server state indication
                                                   0 = n/a (initial value)
                                                  1 = Leader (default)
                                                  2 = Follower
                                                  3 = Error */
    UINT8                   trainDirState;  /**<  train directory state
                                                  1 = unconfirmed
                                                  2 = confirmed
                                                  other values are not allowed */
    UINT16                  reserved03;     /**<  reserved (= 0)  */
    UINT32                  opTrTopoCnt;    /**<  operational train topology counter */
} GNU_PACKED TRDP_ECSP_STAT_T;

#ifdef WIN32
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* TAU_CTRL_TYPES_H */
