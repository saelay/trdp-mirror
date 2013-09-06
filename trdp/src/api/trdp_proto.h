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

#define TRDP_COMID_ECHO                 10

#define TRDP_STATISTICS_REQUEST_COMID   31
#define TRDP_GLOBAL_STATISTICS_COMID    35
#define TRDP_SUBS_STATISTICS_COMID      36
#define TRDP_PUB_STATISTICS_COMID       37
#define TRDP_RED_STATISTICS_COMID       38
#define TRDP_JOIN_STATISTICS_COMID      39
#define TRDP_UDP_LIST_STATISTICS_COMID  40
#define TRDP_TCP_LIST_STATISTICS_COMID  41
#define TRDP_TEST_COMID                 1000

/* more reserved comIds:

 100    PD push Inauguration state and topo count telegram
 101    PD pull request telegram to retrieve dynamic train configuration information
 102    PD pull reply telegram with dynamic train configuration information
 103    MD request telegram to retrieve static consist and car information
 104    MD reply telegram with static consist and car information
 105    MD request telegram to retrieve device information for a given consist/car/device
 106    MD reply telegram with device information for a given consist/car/device
 107    MD request telegram to retrieve consist and car properties for a given consist/car
 108    MD reply telegram with consist and car properties for a given consist/car
 109    MD request telegram to retrieve device properties for a given consist/car/device
 110    MD reply telegram with device properties for a given consist/car/device
 111    MD request telegram for manual insertion of a given consist/car
 112    MD reply telegram for manual insertion of a given consist/car

 120..129    IPTSwitch Control&Monitoring Interface
 125    MD Data (Version) Request Telegram
 126    MD Counter Telegram
 127    MD Dynamic Configuration Telegram
 128    MD Dynamic Configuration Telegram Response
 129    PD Dynamic Configuration Telegram (redundant TS to TS IPC)

 400..415     SDTv2 validation test
 */


/**********************************************************************************************************************/
/**                          TRDP reserved data set ids in the range 1 ... 1000                                      */
/**********************************************************************************************************************/

#define TRDP_STATISTICS_REQUEST_DSID        31
#define TRDP_MEM_STATISTICS_DSID            32
#define TRDP_PD_STATISTICS_DSID             33
#define TRDP_MD_STATISTICS_DSID             34
#define TRDP_GLOBAL_STATISTICS_DSID         35
#define TRDP_SUBS_STATISTICS_DSID           36
#define TRDP_SUBS_STATISTICS_ARRAY_DSID     37
#define TRDP_PUB_STATISTICS_DSID            38
#define TRDP_PUB_STATISTICS_ARRAY_DSID      39
#define TRDP_RED_STATISTICS_DSID            40
#define TRDP_RED_STATISTICS_ARRAY_DSID      41
#define TRDP_JOIN_STATISTICS_DSID           42
#define TRDP_JOIN_STATISTICS_ARRAY_DSID     43
#define TRDP_LIST_STATISTIC_DSID            44
#define TRDP_LIST_STATISTIC_ARRAY_DSID      45

#define TRDP_NEST1_TEST_DSID                990
#define TRDP_NEST2_TEST_DSID                991
#define TRDP_NEST3_TEST_DSID                992
#define TRDP_NEST4_TEST_DSID                993

#define TRDP_TEST_DSID                      1000


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
    UINT32  topoCount;                          /**< set by user: ETB to use, '0' to deacticate             */
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
    UINT32  topoCount;                          /**< set by user: ETB to use, '0' to deacticate             */
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
