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
 *      BL 2017-02-08: Ticket #142: Compiler warnings /​ MISRA-C 2012 issues
 *      BL 2016-11-09: Default PD/MD parameter defines moved from trdp_private.h
 *      BL 2016-06-08: Ticket #120: ComIds for statistics changed to proposed 61375 errata
 *      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
 */

#ifndef TRDP_PROTO_H
#define TRDP_PROTO_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#include "iec61375-2-3.h"


/***********************************************************************************************************************
 * DEFINES
 */

#ifndef TRDP_PD_UDP_PORT
#define TRDP_PD_UDP_PORT  17224u                                         /**< process data UDP port                  */
#endif
#ifndef TRDP_MD_UDP_PORT
#define TRDP_MD_UDP_PORT  17225u                                         /**< message data UDP port                  */
#endif
#ifndef TRDP_MD_TCP_PORT
#define TRDP_MD_TCP_PORT  17225u                                         /**< message data TCP port                  */
#endif

#define TRDP_PROTO_VER                      0x0100u                      /**< Protocol version                       */
#define TRDP_PROTOCOL_VERSION_CHECK_MASK    0xFF00u                      /**< Version check, two digits are relevant */
#define TRDP_SESS_ID_SIZE                   16u                          /**< Session ID (UUID) size in MD header    */
#define TRDP_DEST_URI_SIZE                  32u                          /**< max. Dest URI size in MD header        */


/*  Definitions for time out behaviour accd. table A.17 */
#define TDRP_MD_INFINITE_TIME               0xFFFFFFFFu

/*  Default MD communication parameters   */
#define TRDP_MD_DEFAULT_REPLY_TIMEOUT       5000000u                      /**< default reply time out 5s             */
#define TRDP_MD_DEFAULT_CONFIRM_TIMEOUT     1000000u                      /**< default confirm time out 1s           */
#define TRDP_MD_DEFAULT_CONNECTION_TIMEOUT  60000000u                     /**< Socket connection time out 1 minute   */
#define TRDP_MD_DEFAULT_SENDING_TIMEOUT     5000000u                      /**< Socket sending time out 5s            */
#define TRDP_MD_DEFAULT_QOS                 3u
#define TRDP_MD_DEFAULT_TTL                 64u
#define TRDP_MD_DEFAULT_RETRIES             3u
#define TRDP_MD_DEFAULT_SEND_PARAM          {TRDP_MD_DEFAULT_QOS, TRDP_MD_DEFAULT_TTL, TRDP_MD_DEFAULT_RETRIES}
#define TRDP_MD_MAX_NUM_SESSIONS            1000u

/*  Default PD communication parameters   */
#define TRDP_PD_DEFAULT_QOS                 5u
#define TRDP_PD_DEFAULT_TTL                 64u
#define TRDP_PD_DEFAULT_TIMEOUT             100000u
#define TRDP_PD_DEFAULT_SEND_PARAM          {TRDP_PD_DEFAULT_QOS, TRDP_PD_DEFAULT_TTL, 0u}

/*  Default TRDP process options    */
#define TRDP_PROCESS_DEFAULT_CYCLE_TIME     10000u                       /**< Default cycle time for TRDP process    */
#define TRDP_PROCESS_DEFAULT_PRIORITY       64u                          /**< Default priority of TRDP process       */
#define TRDP_PROCESS_DEFAULT_OPTIONS        TRDP_OPTION_TRAFFIC_SHAPING /**< Default options for TRDP process       */

/*  PD packet properties    */
#define TRDP_MIN_PD_HEADER_SIZE     sizeof(PD_HEADER_T)                 /**< PD header size with FCS                */
#define TRDP_MAX_PD_DATA_SIZE       1432u                                /**< PD data                                */
#define TRDP_MAX_PD_PACKET_SIZE     (TRDP_MAX_PD_DATA_SIZE + TRDP_MIN_PD_HEADER_SIZE)
#define TRDP_MAX_MD_DATA_SIZE       65388u
#define TRDP_MAX_MD_PACKET_SIZE     (TRDP_MAX_MD_DATA_SIZE + sizeof(MD_HEADER_T))

/** Maximum values    */

/**  A uri is a string of the following form:                                 */
/**  trdp://[user part]@[host part]                                           */
/**  trdp://instLabel.funcLabel@devLabel.carLabel.cstLabel.trainLabel         */
/**  Hence the exact max. uri length is:                                      */
/**  7 + (6 * 15) + 5 * (sizeof (separator)) + 1(terminating 0)               */
/**  to facilitate alignment the size will be increased by 1 byte             */

#define TRDP_MAX_LABEL_LEN      16u                              /**< label length incl. terminating '0' */
#define TRDP_MAX_URI_USER_LEN   (2u * TRDP_MAX_LABEL_LEN)        /**< URI user part incl. terminating '0' */
#define TRDP_MAX_URI_HOST_LEN   (4u * TRDP_MAX_LABEL_LEN)        /**< URI host part length incl. terminating '0' */
#define TRDP_MAX_URI_LEN        ((6u * TRDP_MAX_LABEL_LEN) + 8u)  /**< URI length incl. terminating '0' and 1 padding byte
                                                                  */
#define TRDP_MAX_FILE_NAME_LEN  128u                             /**< path and file name length incl. terminating '0' */

#define TDRP_VAR_SIZE           0u                               /**< Variable size dataset    */

/**********************************************************************************************************************/
/**                          TRDP reserved COMIDs in the range 1 ... 1000                                            */
/**********************************************************************************************************************/

#define TRDP_ETBCTRL_COMID                              ETB_CTRL_COMID
#define TRDP_CSTINFO_COMID                              CSTINFO_COMID
#define TRDP_CSTINFOCTRL_COMID                          CSTINFOCTRL_COMID

#define TRDP_COMID_ECHO                                 10u

#define TRDP_STATISTICS_PULL_COMID                      31u
#define TRDP_STATISTICS_REQUEST_COMID                   32u
#define TRDP_GLOBAL_STATISTICS_COMID                    35u
#define TRDP_SUBS_STATISTICS_COMID                      36u
#define TRDP_PUB_STATISTICS_COMID                       37u
#define TRDP_RED_STATISTICS_COMID                       38u
#define TRDP_JOIN_STATISTICS_COMID                      39u
#define TRDP_UDP_LIST_STATISTICS_COMID                  40u
#define TRDP_TCP_LIST_STATISTICS_COMID                  41u

#define TRDP_CONFTEST_COMID                             80u
#define TRDP_CONFTEST_STATUS_COMID                      81u
#define TRDP_CONFTEST_CONF_REQUEST_COMID                82u
#define TRDP_CONFTEST_CONF_REPLY_COMID                  83u
#define TRDP_CONFTEST_OPTRAIN_REQUEST_COMID             84u
#define TRDP_CONFTEST_OPTRAIN_REPLY_COMID               85u
#define TRDP_CONFTEST_ECHO_REQUEST_COMID                86u
#define TRDP_CONFTEST_ECHO_REPLY_COMID                  87u
#define TRDP_CONFTEST_REVERSE_ECHO_COMID                88u

#define TRDP_TTDB_OP_TRAIN_DIR_STATUS_INFO_COMID        TTDB_STATUS_COMID
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_COMID               TTDB_OP_DIR_INFO_COMID
#define TRDP_TTDB_TRAIN_DIR_INFO_REQUEST_COMID          TTDB_TRN_DIR_REQ_COMID
#define TRDP_TTDB_TRAIN_DIR_INFO_REPLY_COMID            TTDB_TRN_DIR_REP_COMID
#define TRDP_TTDB_STATIC_CONSIST_INFO_REQUEST_COMID     TTDB_STAT_CST_REQ_COMID
#define TRDP_TTDB_STATIC_CONSIST_INFO_REPLY_COMID       TTDB_STAT_CST_REP_COMID
#define TRDP_TTDB_TRAIN_NETWORK_DIR_INFO_REQUEST_COMID  TTDB_NET_DIR_REQ_COMID
#define TRDP_TTDB_TRAIN_NETWORK_DIR_INFO_REPLY_COMID    TTDB_NET_DIR_REP_COMID
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_REQUEST_COMID       TTDB_OP_DIR_INFO_REQ_COMID
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_REPLY_COMID         TTDB_OP_DIR_INFO_REP_COMID
#define TRDP_TTDB_READ_COMPLETE_REQUEST_COMID           TTDB_READ_CMPLT_REQ_COMID
#define TRDP_TTDB_READ_COMPLETE_REPLY_COMID             TTDB_READ_CMPLT_REP_COMID

#define TRDP_ECSP_CTRL_COMID         	                ECSP_CTRL_COMID    /* Etb control message               */
#define TRDP_ECSP_STAT_COMID	                        ECSP_STATUS_COMID    /* Etb status message                */
#define TRDP_ECSP_CONF_REQUEST_COMID                    ECSP_CONF_REQ_COMID    /* ECSP confirmation/correction request message */
#define TRDP_ECSP_CONF_REPLY_COMID                      ECSP_CONF_REP_COMID    /* ECSP confirmation/correction reply message */

#define TRDP_ETBN_CTRL_REQUEST_COMID         	        ETBN_CTRL_REQ_COMID
#define TRDP_ETBN_STATUS_REPLY_COMID	                ETBN_CTRL_REP_COMID
#define TRDP_ETBN_TRAIN_NETWORK_DIR_INFO_REQUEST_COMID  ETBN_TRN_NET_DIR_REQ_COMID
#define TRDP_ETBN_TRAIN_NETWORK_DIR_INFO_REPLY_COMID    ETBN_TRN_NET_DIR_REP_COMID

#define TRDP_DNS_REQUEST_COMID                          TCN_DNS_REQ_COMID
#define TRDP_DNS_REPLY_COMID                            TCN_DNS_REP_COMID

#define TRDP_TEST_COMID                                 1000u


/**********************************************************************************************************************/
/**                          TRDP reserved data set ids in the range 1 ... 1000                                      */
/**********************************************************************************************************************/

#define TRDP_ETBCTRL_DSID                               1u
#define TRDP_CSTINFO_DSID                               2u
#define TRDP_CSTINFOCTRL_DSID                           3u

#define TRDP_STATISTICS_REQUEST_DSID                    31u
#define TRDP_MEM_STATISTICS_DSID                        32u
#define TRDP_PD_STATISTICS_DSID                         33u
#define TRDP_MD_STATISTICS_DSID                         34u
#define TRDP_GLOBAL_STATISTICS_DSID                     35u
#define TRDP_SUBS_STATISTICS_DSID                       36u
#define TRDP_SUBS_STATISTICS_ARRAY_DSID                 37u
#define TRDP_PUB_STATISTICS_DSID                        38u
#define TRDP_PUB_STATISTICS_ARRAY_DSID                  39u
#define TRDP_RED_STATISTICS_DSID                        40u
#define TRDP_RED_STATISTICS_ARRAY_DSID                  41u
#define TRDP_JOIN_STATISTICS_DSID                       42u
#define TRDP_JOIN_STATISTICS_ARRAY_DSID                 43u
#define TRDP_LIST_STATISTIC_DSID                        44u
#define TRDP_LIST_STATISTIC_ARRAY_DSID                  45u

#define TRDP_CONFTEST_DSID                              80u
#define TRDP_CONFTEST_STATUS_DSID                       81u
#define TRDP_CONFTEST_CONF_REQUEST_DSID                 82u
#define TRDP_CONFTEST_CONF_REPLY_DSID                   83u
#define TRDP_CONFTEST_OPTRAIN_REQUEST_DSID              84u
#define TRDP_CONFTEST_OPTRAIN_REPLY_DSID                85u
#define TRDP_CONFTEST_ECHO_REQUEST_DSID                 86u
#define TRDP_CONFTEST_ECHO_REPLY_DSID                   87u
#define TRDP_CONFTEST_REVERSE_ECHO_DSID                 88u

#define TRDP_TTDB_OP_TRAIN_DIR_STATUS_INFO_DSID         100u
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_DSID                101u
#define TRDP_TTDB_TRAIN_DIR_INFO_REQUEST_DSID           102u
#define TRDP_TTDB_TRAIN_DIR_INFO_REPLY_DSID             103u
#define TRDP_TTDB_STATIC_CONSIST_INFO_REQUEST_DSID      104u
#define TRDP_TTDB_STATIC_CONSIST_INFO_REPLY_DSID        105u
#define TRDP_TTDB_TRAIN_NETWORK_DIR_INFO_REQUEST_DSID   106u
#define TRDP_TTDB_TRAIN_NETWORK_DIR_INFO_REPLY_DSID     107u
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_REQUEST_DSID        108u
#define TRDP_TTDB_OP_TRAIN_DIR_INFO_REPLY_DSID          109u
#define TRDP_TTDB_READ_COMPLETE_REQUEST_DSID            110u
#define TRDP_TTDB_READ_COMPLETE_REPLY_DSID              111u

#define TRDP_ECSP_CTRL_DSID         	                120u    /* Etb control message               */
#define TRDP_ECSP_STAT_DSID	                            121u    /* Etb status message                */
#define TRDP_ECSP_CONF_REQUEST_DSID                     122u    /* ECSP confirmation/correction request message */
#define TRDP_ECSP_CONF_REPLY_DSID                       123u    /* ECSP confirmation/correction reply message */

#define TRDP_ETBN_CTRL_REQUEST_DSID         	        130u
#define TRDP_ETBN_STATUS_REPLY_DSID	                    131u
#define TRDP_ETBN_TRAIN_NETWORK_DIR_INFO_REQUEST_DSID   132u
#define TRDP_ETBN_TRAIN_NETWORK_DIR_INFO_REPLY_DSID     133u

#define TRDP_DNS_REQUEST_DSID                           140u
#define TRDP_DNS_REPLY_DSID                             141u

#define TRDP_NEST1_TEST_DSID                            990u
#define TRDP_NEST2_TEST_DSID                            991u
#define TRDP_NEST3_TEST_DSID                            992u
#define TRDP_NEST4_TEST_DSID                            993u

#define TRDP_TEST_DSID                                  1000u


/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Message Types    */
typedef enum
{
    TRDP_MSG_PD = 0x5064u,      /**< 'Pd' PD Data                                    */
    TRDP_MSG_PP = 0x5070u,      /**< 'Pp' PD Data (Pull Reply)                       */
    TRDP_MSG_PR = 0x5072u,      /**< 'Pr' PD Request                                 */
    TRDP_MSG_PE = 0x5065u,      /**< 'Pe' PD Error                                   */
    TRDP_MSG_MN = 0x4D6Eu,      /**< 'Mn' MD Notification (Request without reply)    */
    TRDP_MSG_MR = 0x4D72u,      /**< 'Mr' MD Request with reply                      */
    TRDP_MSG_MP = 0x4D70u,      /**< 'Mp' MD Reply without confirmation              */
    TRDP_MSG_MQ = 0x4D71u,      /**< 'Mq' MD Reply with confirmation                 */
    TRDP_MSG_MC = 0x4D63u,      /**< 'Mc' MD Confirm                                 */
    TRDP_MSG_ME = 0x4D65u       /**< 'Me' MD Error                                   */
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
    UINT32  datasetLength;                      /**< length of the data to transmit 0...1432                */
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
    UINT8   sessionID[16u];                     /**< UUID as a byte stream                                  */
    UINT32  replyTimeout;                       /**< in us                                                  */
    UINT8   sourceURI[32u];                     /**< User part of URI                                       */
    UINT8   destinationURI[32u];                /**< User part of URI                                       */
    UINT32  frameCheckSum;                      /**< CRC32 of header                                        */
} GNU_PACKED MD_HEADER_T;

#ifdef WIN32
#pragma pack(pop)
#endif

#endif
