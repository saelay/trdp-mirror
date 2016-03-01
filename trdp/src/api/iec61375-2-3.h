/**********************************************************************************************************************/
/**
 * @file            iec61375-2-3.h
 *
 * @brief           TTDB, CSTINFO Frame typedefs, Telegram definitions
 *
 * @note            Project: TCNOpen TRDP
 *
 * @author          Bernd Loehr, NewTec GmbH, 2015-09-11
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 */

#ifndef IEC61375_2_3_H
#define IEC61375_2_3_H

/***********************************************************************************************************************
 * DEFINITIONS
 */

/*
  TCN-URI (host part)               Scope       IP address      Description
￼
  grpAll.aVeh.lCst.lClTrn.lTrn      D           239.192.0.0     “broadcast” to all end devices
                                                                within the local consist.
                                                                    NOTE 1: 239.255/16 is defined as
                                                                    CN multicast range in IEC61375-2- 5)
  lDev.lVeh.lCst.lClTrn.lTrn        S,D         127.0.0.1       Own device (local loop-back)
  grpETBN.anyVeh.aCst.aClTrn.lTrn   D           239.192.0.129   Broadcast to all ETBN
  grpECSC.anyVeh.aCst.aClTrn.lTrn   D           239.192.0.131   Broadcast to all ECSC
  grpECSP.anyVeh.aCst.aClTrn.lTrn   D           239.192.0.130   Broadcast to all ECSP
  grpAll.aVeh.cstX.anyClTrnlTrn     D           239.192.0.X     Broadcast to all end devices in consist X.
                                                                    NOTE 2: this address is defined in IEC61375-2-5)
*/

#define MAX_NO_OF_VEHICLES          63              /* Ch. 4.2.2.1              */
#define MAX_NO_OF_CONSISTS          63              /* Ch. 4.2.2.1              */
#define MAX_NO_OF_CLOSED_CSTS       32              /* Ch. ??                   */
#define MAX_NO_OF_FUNCTIONS         1023            /* Ch. 5.3.3.1              */
#define MAX_SIZE_OF_PROPERTY        32 * 1024       /* Ch. ??                   */
#define MAX_SIZE_OF_CSTINFO         64 * 1024       /* Ch. ??                   */
#define MAX_NO_OF_ETBN              63              /* Ch. 6.4.2.3 IEC61375-2-5 */
#define MAX_NO_OF_ETB               4               /* Ch. 4.2.2.2              */

/* Time out values (in seconds)                                                                                 */

#define ETB_WAIT_TIMER_VALUE        5
#define TX_TIMER_VALUE              1

/** ETB Control telegram                                                                                        */

#define ETB_CTRL_COMID              1                                           /**<                            */
#define ETB_CTRL_CYC                500                                         /**< 0.5s                       */
#define ETB_CTRL_TO                 3000                                        /**< 3s                         */
#define ETB_CTRL_DEST_URI           "grpECSP.anyVeh.aCst.aClTrn.lTrn"
#define ETB_CTRL_DEST_IP            "239.192.0.130"

/** Consist Info telegram (Message data notification 'Mn') */

#define CSTINFO_COMID               2                                           /**<    */
#define CSTINFO_SOURCE_URI          "devECSP.aVeh.lCst"
#define CSTINFO_DEST_URI            "grpECSP.anyVeh.aCst.aClTrn.lTrn"
#define CSTINFO_DEST_IP             "239.192.0.130"

/** Consist Info control/request telegram (Message data notification 'Mn')                                      */

#define CSTINFOCTRL_COMID           3                                           /**<    */
#define CSTINFOCTRL_SOURCE_URI      "devECSP.aVeh.lCst"
#define CSTINFOCTRL_DEST_URI        "grpECSP.anyVeh.aCst.aClTrn.lTrn"
#define CSTINFOCTRL_DEST_IP         "239.192.0.130"

/** TTDB manager telegram PD                                                                                    */

#define TTDB_STATUS_COMID           100
#define TTDB_STATUS_CYC             1000                                        /**< Push                       */
#define TTDB_STATUS_TO              5000                                        /**< 5s                         */
#define TTDB_STATUS_SMI             100
#define TTDB_STATUS_USER_DATA_VER   0x0100
#define TTDB_STATUS_DEST_URI        "grpAll.aVeh.lCst.lClTrn.lTrn"
#define TTDB_STATUS_DEST_IP_ETB0    "239.194.0.0"
#define TTDB_STATUS_DEST_IP         "239.255.0.0"
#define TTDB_STATUS_INFO_DS         "TTDB_OP_TRAIN_DIRECTORY_STATUS_INFO"

/** TTDB manager telegram MD: Push the OP_TRAIN_DIRECTORY                                                       */

#define TTDB_OP_DIR_INFO_COMID      101                                         /**< MD notification */
#define TTDB_OP_DIR_INFO_URI        "grpAll.aVeh.lCst.lClTrn.lTrn"
#define TTDB_OP_DIR_INFO_IP_ETB0    "239.194.0.0"
#define TTDB_OP_DIR_INFO_IP         "239.255.0.0"
#define TTDB_OP_DIR_INFO_DS         "TTDB_OP_TRAIN_DIR_INFO"                    /**< OP_TRAIN_DIRECTORY */

/** TTDB manager telegram MD: Get the TRAIN_DIRECTORY                                                           */

#define TTDB_TRN_DIR_REQ_COMID      102                                         /**< MD request */
#define TTDB_TRN_DIR_REQ_URI        "devECSP.anyVeh.lCst"
#define TTDB_TRN_DIR_REQ_DS         "TTDB_TRAIN_DIRECTORY_INFO_REQUEST"
#define TTDB_TRN_DIR_REQ_TO         3000                                        /**< 3s timeout                 */

#define TTDB_TRN_DIR_REP_COMID      103                                         /**< MD reply                   */
#define TTDB_TRN_DIR_REP_DS         "TTDB_TRAIN_DIRECTORY_INFO_REPLY"           /**< TRAIN_DIRECTORY            */

/** TTDB manager telegram MD: Get the static consist information                                                */
#define TTDB_STAT_CST_REQ_COMID     104                                         /**< MD request                 */
#define TTDB_STAT_CST_REQ_URI       "devECSP.anyVeh.lCst.lClTrn.lTrn"
#define TTDB_STAT_CST_REQ_DS        "TTDB_STATIC_CONSIST_INFO_REQUEST"
#define TTDB_STAT_CST_REQ_TO        3000                                        /**< 3s timeout                 */

#define TTDB_STAT_CST_REP_COMID     105
#define TTDB_STAT_CST_REP_DS        "TTDB_STATIC_CONSIST_INFO_REPLY"            /**< CONSIST_INFO               */

/** TTDB manager telegram MD: Get the NETWORK_TRAIN_DIRECTORY                                                   */

#define TTDB_NET_DIR_REQ_COMID      106                                         /**< MD request                 */
#define TTDB_NET_DIR_REQ_URI        "devECSP.anyVeh.lCst"
#define TTDB_NET_DIR_REQ_DS         "TTDB_TRAIN_NETWORK_DIRECTORY_INFO_REQUEST"
#define TTDB_NET_DIR_REQ_TO         3000                                        /**< 3s timeout                 */
#define TTDB_NET_DIR_REP_COMID      107                                         /**< MD reply                   */
#define TTDB_NET_DIR_REP_DS         "TTDB_TRAIN_NETWORK_DIRECTORY_INFO_REPLY"   /**< TRAIN_NETWORK_DIRECTORY    */

/** TTDB manager telegram MD: Get the OP_TRAIN_DIRECTORY                                                        */

#define TTDB_OP_DIR_INFO_REQ_COMID  108
#define TTDB_OP_DIR_INFO_REQ_URI    "devECSP.anyVeh.lCst"
#define TTDB_OP_DIR_INFO_REQ_TO     3000                                        /**< 3s timeout                 */
#define TTDB_OP_DIR_INFO_REP_COMID  109
#define TTDB_OP_DIR_INFO_REP_DS     "TTDB_OP_TRAIN_DIR_INFO"                    /**< OP_TRAIN_DIRECTORY         */

/** TTDB manager telegram MD: Get the TTDB                                                                      */

#define TTDB_READ_CMPLT_REQ_COMID   110
#define TTDB_READ_CMPLT_REQ_URI     "devECSP.anyVeh.lCst"
#define TTDB_READ_CMPLT_REQ_DS      "TTDB_READ_COMPLETE_REQUEST"                /**< ETBx                       */
#define TTDB_READ_CMPLT_REQ_TO      3000                                        /**< 3s timeout                 */

#define TTDB_READ_CMPLT_REP_COMID   111                                         /**< MD reply                   */
#define TTDB_READ_CMPLT_REP_DS      "TTDB_READ_COMPLETE_REPLY"                  /**< TRDP_READ_COMPLETE_REPLY_T */

/** ECSP Control telegram                                                                                       */

#define ECSP_CTRL_COMID             120
#define ECSP_CTRL_CYC               1000                                        /**< 1s                         */
#define ECSP_CTRL_TO                5000                                        /**< 5s                         */
#define ECSP_CTRL_DEST_URI          "devECSP.anyVeh.lCst"                       /**< 10.0.0.1                   */

/** ECSP status telegram                                                                                        */

#define ECSP_STATUS_COMID           121
#define ECSP_STATUS_CYC             1000                                        /**< 1s                         */
#define ECSP_STATUS_TO              5000                                        /**< 5s                         */
#define ECSP_STATUS_DEST_URI        "devECSC.anyVeh.lCst"                       /**< 10.0.0.100                 */

/** ETBN STATUS Telegram PD                                                                                     */

#define ETBN_STATUS_COMID           122                                         /**< tbd!                       */
#define ETBN_STATUS_CYC             1000                                        /**< 1s cycle time              */
#define ETBN_STATUS_TO              5000                                        /**< 5s timeout                 */
#define ETBN_STATUS_DEST_URI        "grpAll.aVeh.lCst"
#define ETBN_STATUS_DEST_IP         "239.255.0.0"

/** ETBN Control Telegram MD                                                                                    */

#define ETBN_CTRL_REQ_COMID         130
#define ETBN_CTRL_REQ_DS            "ETBN_CTRL"                                 /**< ETBx                       */
#define ETBN_CTRL_REQ_TO            3000                                        /**< 3s timeout                 */
#define ETBN_CTRL_REP_COMID         131
#define ETBN_CTRL_REP_DS            "ETBN_STATUS"                               /**< ETBN status reply          */

/** ETBN Control Telegram MD                                                                                    */

#define ETBN_TRN_NET_DIR_REQ_COMID  132
#define ETBN_TRN_NET_DIR_REQ_TO     3000                                        /**< 3s timeout                 */
#define ETBN_TRN_NET_DIR_REP_COMID  133
#define ETBN_TRN_NET_DIR_REP_DS     "ETBN_TRAIN_NETWORK_DIRECTORY_INFO_REPLY"   /**< ETBx                       */

/*******************************************************************************
 *  TYPEDEFS
 */


#endif
