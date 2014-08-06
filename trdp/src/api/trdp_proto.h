/**********************************************************************************************************************/
/**
 * @file            trdp_proto.h
 *
 * @brief           Definitions for the TRDP protocol
 *
 * @details         TRDP internal type definitions
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
 */

#ifndef TRDP_PROTO_H
#define TRDP_PROTO_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"


/***********************************************************************************************************************
 * DEFINES
 */

#ifndef TRDP_PD_UDP_PORT
#define TRDP_PD_UDP_PORT  20548                                         /**< process data UDP port                  */
#endif
#ifndef TRDP_MD_UDP_PORT
#define TRDP_MD_UDP_PORT  20550                                         /**< message data UDP port                  */
#endif
#ifndef TRDP_MD_TCP_PORT
#define TRDP_MD_TCP_PORT  20550                                         /**< message data TCP port                  */
#endif

#define TRDP_PROTO_VER                      0x0100                      /**< Protocol version                       */
#define TRDP_PROTOCOL_VERSION_CHECK_MASK    0xFF00                      /**< Version check, two digits are relevant */
#define TRDP_SESS_ID_SIZE                   16                          /**< Session ID (UUID) size in MD header    */
#define TRDP_DEST_URI_SIZE                  32                          /**< max. Dest URI size in MD header        */


/*  Default MD communication parameters   */
#define TRDP_MD_DEFAULT_QOS     3
#define TRDP_MD_DEFAULT_TTL     64

/*  Default PD communication parameters   */
#define TRDP_PD_DEFAULT_QOS     5
#define TRDP_PD_DEFAULT_TTL     64

/*  PD packet properties    */
#define TRDP_MIN_PD_HEADER_SIZE     sizeof(PD_HEADER_T)                 /**< PD header size with FCS                */
#define TRDP_MAX_PD_DATA_SIZE       1432                                /**< PD data size without FCS               */
#define TRDP_MAX_PD_PACKET_SIZE     (TRDP_MAX_PD_DATA_SIZE + TRDP_MIN_PD_HEADER_SIZE + sizeof(UINT32))
#define TRDP_MAX_MD_DATA_SIZE       ((1024 * 64) - 8 - sizeof(MD_HEADER_T))
#define TRDP_MAX_MD_PACKET_SIZE     (TRDP_MAX_MD_DATA_SIZE + sizeof(MD_HEADER_T))

/** Maximum values    */

/**  A uri is a string of the following form:                                 */
/**  trdp://[user part]@[host part]                                           */
/**  trdp://instLabel.funcLabel@devLabel.carLabel.cstLabel.trainLabel         */
/**  Hence the exact max. uri length is:                                      */
/**  7 + (6 * 15) + 5 * (sizeof (separator)) + 1(terminating 0)               */
/**  to facilitate alignment the size will be increased by 1 byte             */

#define TRDP_MAX_LABEL_LEN      16                              /**< label length incl. terminating '0' */
#define TRDP_MAX_URI_USER_LEN   (2 * TRDP_MAX_LABEL_LEN)        /**< URI user part incl. terminating '0' */
#define TRDP_MAX_URI_HOST_LEN   (4 * TRDP_MAX_LABEL_LEN)        /**< URI host part length incl. terminating '0' */
#define TRDP_MAX_URI_LEN        ((6 * TRDP_MAX_LABEL_LEN) + 8)  /**< URI length incl. terminating '0' and 1 padding byte
                                                                  */
#define TRDP_MAX_FILE_NAME_LEN  128                             /**< path and file name length incl. terminating '0' */

#define TDRP_VAR_SIZE           0                               /**< Variable size dataset    */

/**********************************************************************************************************************/
/**                          TRDP reserved COMIDs in the range 1 ... 1000                                            */
/**********************************************************************************************************************/

#define TRDP_ETBCTRL_COMID                              1
#define TRDP_CSTINFO_COMID                              2
#define TRDP_CSTINFOCTRL_COMID                          3

#define TRDP_COMID_ECHO                                 10

#define TRDP_STATISTICS_REQUEST_COMID                   31
#define TRDP_GLOBAL_STATISTICS_COMID                    35
#define TRDP_SUBS_STATISTICS_COMID                      36
#define TRDP_PUB_STATISTICS_COMID                       37
#define TRDP_RED_STATISTICS_COMID                       38
#define TRDP_JOIN_STATISTICS_COMID                      39
#define TRDP_UDP_LIST_STATISTICS_COMID                  40
#define TRDP_TCP_LIST_STATISTICS_COMID                  41

#define TRDP_CONFTEST_COMID                             80
#define TRDP_CONFTEST_STATUS_COMID                      81
#define TRDP_CONFTEST_CONF_REQUEST_COMID                82
#define TRDP_CONFTEST_CONF_REPLY_COMID                  83
#define TRDP_CONFTEST_OPTRAIN_REQUEST_COMID             84
#define TRDP_CONFTEST_OPTRAIN_REPLY_COMID               85
#define TRDP_CONFTEST_ECHO_REQUEST_COMID                86
#define TRDP_CONFTEST_ECHO_REPLY_COMID                  87
#define TRDP_CONFTEST_REVERSE_ECHO_COMID                88

#define TRDP_TTDB_OP_TRAIN_DIR_STATUS_INFO_COMID        100
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_COMID               101
#define TRDP_TTDB_TRAIN_DIR_INFO_REQUEST_COMID          102
#define TRDP_TTDB_TRAIN_DIR_INFO_REPLY_COMID            103
#define TRDP_TTDB_STATIC_CONSIST_INFO_REQUEST_COMID     104
#define TRDP_TTDB_STATIC_CONSIST_INFO_REPLY_COMID       105
#define TRDP_TTDB_TRAIN_NETWORK_DIR_INFO_REQUEST_COMID  106
#define TRDP_TTDB_TRAIN_NETWORK_DIR_INFO_REPLY_COMID    107
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_REQUEST_COMID       108
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_REPLY_COMID         109
#define TRDP_TTDB_READ_COMPLETE_REQUEST_COMID           110
#define TRDP_TTDB_READ_COMPLETE_REPLY_COMID             111

#define TRDP_ECSP_CTRL_COMID         	                120    /* Etb control message               */
#define TRDP_ECSP_STAT_COMID	                        121    /* Etb status message                */
#define TRDP_ECSP_CONF_REQUEST_COMID                    122    /* ECSP confirmation/correction request message */
#define TRDP_ECSP_CONF_REPLY_COMID                      123    /* ECSP confirmation/correction reply message */

#define TRDP_ETBN_CTRL_REQUEST_COMID         	        130
#define TRDP_ETBN_STATUS_REPLY_COMID	                131
#define TRDP_ETBN_TRAIN_NETWORK_DIR_INFO_REQUEST_COMID  132
#define TRDP_ETBN_TRAIN_NETWORK_DIR_INFO_REPLY_COMID    133

#define TRDP_DNS_REQUEST_COMID                          140
#define TRDP_DNS_REPLY_COMID                            141

#define TRDP_TEST_COMID                                 1000


/**********************************************************************************************************************/
/**                          TRDP reserved data set ids in the range 1 ... 1000                                      */
/**********************************************************************************************************************/

#define TRDP_ETBCTRL_DSID                               1
#define TRDP_CSTINFO_DSID                               2
#define TRDP_CSTINFOCTRL_DSID                           3

#define TRDP_STATISTICS_REQUEST_DSID                    31
#define TRDP_MEM_STATISTICS_DSID                        32
#define TRDP_PD_STATISTICS_DSID                         33
#define TRDP_MD_STATISTICS_DSID                         34
#define TRDP_GLOBAL_STATISTICS_DSID                     35
#define TRDP_SUBS_STATISTICS_DSID                       36
#define TRDP_SUBS_STATISTICS_ARRAY_DSID                 37
#define TRDP_PUB_STATISTICS_DSID                        38
#define TRDP_PUB_STATISTICS_ARRAY_DSID                  39
#define TRDP_RED_STATISTICS_DSID                        40
#define TRDP_RED_STATISTICS_ARRAY_DSID                  41
#define TRDP_JOIN_STATISTICS_DSID                       42
#define TRDP_JOIN_STATISTICS_ARRAY_DSID                 43
#define TRDP_LIST_STATISTIC_DSID                        44
#define TRDP_LIST_STATISTIC_ARRAY_DSID                  45

#define TRDP_CONFTEST_DSID                              80
#define TRDP_CONFTEST_STATUS_DSID                       81
#define TRDP_CONFTEST_CONF_REQUEST_DSID                 82
#define TRDP_CONFTEST_CONF_REPLY_DSID                   83
#define TRDP_CONFTEST_OPTRAIN_REQUEST_DSID              84
#define TRDP_CONFTEST_OPTRAIN_REPLY_DSID                85
#define TRDP_CONFTEST_ECHO_REQUEST_DSID                 86
#define TRDP_CONFTEST_ECHO_REPLY_DSID                   87
#define TRDP_CONFTEST_REVERSE_ECHO_DSID                 88

#define TRDP_TTDB_OP_TRAIN_DIR_STATUS_INFO_DSID         100
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_DSID                101
#define TRDP_TTDB_TRAIN_DIR_INFO_REQUEST_DSID           102
#define TRDP_TTDB_TRAIN_DIR_INFO_REPLY_DSID             103
#define TRDP_TTDB_STATIC_CONSIST_INFO_REQUEST_DSID      104
#define TRDP_TTDB_STATIC_CONSIST_INFO_REPLY_DSID        105
#define TRDP_TTDB_TRAIN_NETWORK_DIR_INFO_REQUEST_DSID   106
#define TRDP_TTDB_TRAIN_NETWORK_DIR_INFO_REPLY_DSID     107
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_REQUEST_DSID        108
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_REPLY_DSID          109
#define TRDP_TTDB_READ_COMPLETE_REQUEST_DSID            110
#define TRDP_TTDB_READ_COMPLETE_REPLY_DSID              111

#define TRDP_ECSP_CTRL_DSID         	                120    /* Etb control message               */
#define TRDP_ECSP_STAT_DSID	                            121    /* Etb status message                */
#define TRDP_ECSP_CONF_REQUEST_DSID                     122    /* ECSP confirmation/correction request message */
#define TRDP_ECSP_CONF_REPLY_DSID                       123    /* ECSP confirmation/correction reply message */

#define TRDP_ETBN_CTRL_REQUEST_DSID         	        130
#define TRDP_ETBN_STATUS_REPLY_DSID	                    131
#define TRDP_ETBN_TRAIN_NETWORK_DIR_INFO_REQUEST_DSID   132
#define TRDP_ETBN_TRAIN_NETWORK_DIR_INFO_REPLY_DSID     133

#define TRDP_DNS_REQUEST_DSID                           140
#define TRDP_DNS_REPLY_DSID                             141

#define TRDP_NEST1_TEST_DSID                            990
#define TRDP_NEST2_TEST_DSID                            991
#define TRDP_NEST3_TEST_DSID                            992
#define TRDP_NEST4_TEST_DSID                            993

#define TRDP_TEST_DSID                                  1000


/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Message Types    */
typedef enum
{
    TRDP_MSG_PD = 0x5064,     /**< 'Pd' PD Data                                    */
    TRDP_MSG_PP = 0x5070,     /**< 'Pp' PD Data (Pull Reply)                       */
    TRDP_MSG_PR = 0x5072,     /**< 'Pr' PD Request                                 */
    TRDP_MSG_PE = 0x5065,     /**< 'Pe' PD Error                                   */
    TRDP_MSG_MN = 0x4D6E,     /**< 'Mn' MD Notification (Request without reply)    */
    TRDP_MSG_MR = 0x4D72,     /**< 'Mr' MD Request with reply                      */
    TRDP_MSG_MP = 0x4D70,     /**< 'Mp' MD Reply without confirmation              */
    TRDP_MSG_MQ = 0x4D71,     /**< 'Mq' MD Reply with confirmation                 */
    TRDP_MSG_MC = 0x4D63,     /**< 'Mc' MD Confirm                                 */
    TRDP_MSG_ME = 0x4D65      /**< 'Me' MD Error                                   */
} TRDP_MSG_T;

#ifdef WIN32
#pragma pack(push, 1)
#endif

/** TRDP process data header - network order and alignment    */
typedef struct
{
    UINT32  sequenceCounter;                    /**< Unique counter (autom incremented)                     */
    UINT16  protocolVersion;                    /**< fix value for compatibility (set by the API)           */
    UINT16  msgType;                            /**< of datagram: PD Request (0x5072) or PD_MSG (0x5064)    */
    UINT32  comId;                              /**< set by user: unique id                                 */
    UINT32  etbTopoCnt;                         /**< set by user: ETB to use, '0' for consist local traffic */
    UINT32  opTrnTopoCnt;                       /**< set by user: direction/side critical, '0' if ignored   */
    UINT32  datasetLength;                      /**< length of the data to transmit 0...1436
                                                    without padding and FCS                                 */
    UINT32  reserved;                           /**< before used for ladder support                         */
    UINT32  replyComId;                         /**< used in PD request                                     */
    UINT32  replyIpAddress;                     /**< used for PD request                                    */
    UINT32  frameCheckSum;                      /**< CRC32 of header                                        */
} GNU_PACKED PD_HEADER_T;

/** TRDP message data header - network order and alignment    */
typedef struct
{
    UINT32  sequenceCounter;                    /**< Unique counter (autom incremented)                     */
    UINT16  protocolVersion;                    /**< fix value for compatibility                            */
    UINT16  msgType;                            /**< of datagram: Mn, Mr, Mp, Mq, Mc or Me                  */
    UINT32  comId;                              /**< set by user: unique id                                 */
    UINT32  etbTopoCnt;                         /**< set by user: ETB to use, '0' for consist local traffic */
    UINT32  opTrnTopoCnt;                       /**< set by user: direction/side critical, '0' if ignored   */
    UINT32  datasetLength;                      /**< defined by user: length of data to transmit            */
    INT32   replyStatus;                        /**< 0 = OK                                                 */
    UINT8   sessionID[16];                      /**< UUID as a byte stream                                  */
    UINT32  replyTimeout;                       /**< in us                                                  */
    UINT8   sourceURI[32];                      /**< User part of URI                                       */
    UINT8   destinationURI[32];                 /**< User part of URI                                       */
    UINT32  frameCheckSum;                      /**< CRC32 of header                                        */
} GNU_PACKED MD_HEADER_T;

#ifdef WIN32
#pragma pack(pop)
#endif

#endif
