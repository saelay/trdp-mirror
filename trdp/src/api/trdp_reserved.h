/**********************************************************************************************************************/
/**
 * @file            trdp_reserved.h
 *
 * @brief           Definition of reserved COMID's and DSID's
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss, Bombardier
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id: trdp_reserverd.h 78 2012-10-25 08:23:28Z 97031 $
 *
 */

#ifndef TRDP_RESERVED_H
#define TRDP_RESERVED_H

/***********************************************************************************************************************
 * INCLUDES
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "trdp_types.h"


/***********************************************************************************************************************
 * DEFINES
 */

/**********************************************************************************************************************/
/**                          TRDP reserved COMID's in the range 1 ... 1000                                            */
/**********************************************************************************************************************/
#define TRDP_COMID_ECHO                   10

#define TRDP_STATISTICS_REQUEST_COMID     31
#define TRDP_GLOBAL_STATISTICS_COMID      35
#define TRDP_SUBS_STATISTICS_COMID        36
#define TRDP_PUB_STATISTICS_COMID         37
#define TRDP_RED_STATISTICS_COMID         38
#define TRDP_JOIN_STATISTICS_COMID        39
#define TRDP_UDP_LIST_STATISTICS_COMID    40
#define TRDP_TCP_LIST_STATISTICS_COMID    41


/* more reserved comIds:

            100	PD push Inauguration state and topo count telegram
            101	PD pull request telegram to retrieve dynamic train configuration information
            102	PD pull reply telegram with dynamic train configuration information
            103	MD request telegram to retrieve static consist and car information
            104	MD reply telegram with static consist and car information
            105	MD request telegram to retrieve device information for a given consist/car/device
            106	MD reply telegram with device information for a given consist/car/device
            107	MD request telegram to retrieve consist and car properties for a given consist/car
            108	MD reply telegram with consist and car properties for a given consist/car
            109	MD request telegram to retrieve device properties for a given consist/car/device
            110	MD reply telegram with device properties for a given consist/car/device
            111	MD request telegram for manual insertion of a given consist/car
            112	MD reply telegram for manual insertion of a given consist/car

            120..129	IPTSwitch Control&Monitoring Interface
            125	MD Data (Version) Request Telegram
            126	MD Counter Telegram
            127	MD Dynamic Configuration Telegram
            128	MD Dynamic Configuration Telegram Response
            129	PD Dynamic Configuration Telegram (redundant TS to TS IPC)

            400..415 	SDTv2 validation test
*/


/**********************************************************************************************************************/
/**                          TRDP reserved data set id's in the range 1 ... 1000                                      */
/**********************************************************************************************************************/
#define TRDP_STATISTICS_REQUEST_DSID  31
#define TRDP_MEM_STATISTICS_DSID      32
#define TRDP_PD_STATISTICS_DSID       33
#define TRDP_MD_STATISTICS_DSID       34
#define TRDP_GLOBAL_STATISTICS_DSID   35
#define TRDP_SUBS_STATISTICS_DSID     36
#define TRDP_PUB_STATISTICS_DSID      37
#define TRDP_RED_STATISTICS_DSID      38
#define TRDP_JOIN_STATISTICS_DSID     39
#define TRDP_LIST_STATISTIC_DSIDS     40

#define TRDP_NEST1_TEST_DSID          990
#define TRDP_NEST2_TEST_DSID          991
#define TRDP_NEST3_TEST_DSID          992
#define TRDP_NEST4_TEST_DSID          993
        
#define TRDP_TEST_DSID                1000


/***********************************************************************************************************************
 * TYPEDEFS
 */

/**********************************************************************************************************************/
/**                      TRDP statistics data set type definitions for above defined data set id's                    */
/**********************************************************************************************************************/

typedef struct 
{
    UINT32 comid;                   /**< comid of the requested statistics reply */
} TRDP_STATISTICS_REQUEST_DS_T;

typedef struct 
{
    UINT32 size;                    /**< length of the var size array */
    TRDP_SUBS_STATISTICS_T subs[1]; /**< subscriber, array length of 1 as placeholder for a variable length */
} TRDP_SUBS_STATISTICS_DS_T;

typedef struct 
{
    UINT32 size;                    /**< length of the var size array */
    TRDP_PUBS_STATISTICS_T pub[1];  /**< publisher, array length of 1 as placeholder for a variable length */
} TRDP_PUB_STATISTICS_DS_T;

typedef struct 
{
    UINT32 size;                    /**< length of the var size array */
    TRDP_LIST_STATISTICS_T list[1]; /**< listener, array length of 1 as placeholder for a variable length */
} TRDP_LIST_STATISTICS_DS_T;


typedef struct 
{
    UINT32 size;                    /**< length of the var size array */
    TRDP_RED_STATISTICS_T red[1];   /**< redundancy group, array length of 1 as placeholder for a variable length */
} TRDP_RED_STATISTICS_DS_T;

typedef struct 
{
    UINT32 size;                    /**< length of the var size array */
    UINT32 addr[1];                 /**< joined address, array length of 1 as placeholder for a variable length */
} TRDP_JOIN_STATISTICS_DS_T;


/**********************************************************************************************************************/
/**                     TRDP test data set definitions for above defined data set id's p_types.h                      */
/**********************************************************************************************************************/

typedef struct 
{
    UINT32 cnt;
    char   testId[16];
} TRDP_MD_TEST_DS_T;

typedef struct 
{
    UINT32 cnt;
    char   testId[16];
} TRDP_MD_TEST_DS_T;

typedef struct
{
    UINT8   level;
    char    string[16];
} TRDP_NEST1_TEST_DS_T;

typedef struct
{
    UINT8   level;
    TRDP_NEST1_TEST_DS_T ds;
}TRDP_NEST2_TEST_DS_T;

typedef struct
{
    UINT8   level;
    TRDP_NEST2_TEST_DS_T ds;
}TRDP_NEST3_TEST_DS_T;

typedef struct
{
    UINT8   level;
    TRDP_NEST3_TEST_DS_T ds;
} TRDP_NEST4_TEST_DS_T;

typedef struct 
{
    UINT8   bool8_1;
    char    char8_1;
    INT16   utf16_1;
    INT8    int8_1;
    INT16   int16_1;
    INT32   int32_1;
    INT64   int64_1;
    UINT8   uint8_1;
    UINT16  uint16_1;
    UINT32  uint32_1;
    UINT64  uint64_1;
    float   float32_1;
    double  float64_1;
    TIMEDATE32  timedate32_1;
    TIMEDATE48  timedate48_1;
    TIMEDATE64  timedate64_1;
    UINT8   bool8_4[4];
    char    char8_16[16];
    INT16   utf16_4[16];
    INT8    int8_4[4];
    INT16   int16_4[4];
    INT32   int32_4[4];
    INT64   int64_4[4];
    UINT8   uint8_4[4];
    UINT16  uint16_4[4];
    UINT32  uint32_4[4];
    UINT64  uint64_4[4];
    float   float32_4[4];
    double  float64_4[4];
    TIMEDATE32  timedate32_4[4];
    TIMEDATE48  timedate48_4[4];
    TIMEDATE64  timedate64_4[4];
    UINT16  size_bool8;
    UINT8   bool8_0[4];
    UINT16  size_char8;
    char    char8_0[16];
    UINT16  size_utf16;
    INT16   utf16_0[16];
    UINT16  size_int8;
    INT8    int8_0[4];
    UINT16  size_int16;
    INT16   int16_0[4];
    UINT16  size_int32;
    INT32   int32_0[4];
    UINT16  size_int64;
    INT64   int64_0[4];
    UINT16  size_uint8;
    UINT8   uint8_0[4];
    UINT16  size_uint16;
    UINT16  uint16_0[4];
    UINT16  size_uint32;
    UINT32  uint32_0[4];
    UINT16  size_uint64;
    UINT64  uint64_0[4];
    UINT16  size_float32;
    float   float32_0[4];
    UINT16  size_float64;
    double  float64_0[4];
    UINT16  size_timedate32;
    TIMEDATE32  timedate32_0[4];
    UINT16  size_timedate48;
    TIMEDATE48  timedate48_0[4];
    UINT16  size_timedate64;
    TIMEDATE64  timedate64_0[4];
    TRDP_NEST4_TEST_DS_T ds;
} TRDP_TEST_DS_T; 


#ifdef __cplusplus
}
#endif

#endif /* TRDP_RESERVED_H */ 
