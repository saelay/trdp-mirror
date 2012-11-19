/*
//  test_server.h
//  trdp
//
//  Created by Bernd Löhr on 10.10.12.
//
*/

#ifndef trdp_test_server_h
#define trdp_test_server_h



#include "tau_marshall.h"

/*  IP addresses:

    test_server <----.....----> test_client

*/

#define PD_TESTSERVER_IP        0x0a000066      /*	Sender's IP: 10.0.0.102         */
#define PD_TESTCLIENT_IP        0x0a000065      /*	Client's IP: 10.0.0.101         */
#define PD_TESTMULTICAST_IP     0xef000000      /*	Multicast Group: 239.0.0.0		*/

/*
 Test dataset definitions
 */

/**********************************************************************************************************************/
/* PD unicast test with generic test dataset                                                                          */
/**********************************************************************************************************************/

#define PD_TEST_GEN_UNI_COMID       999
#define PD_TEST_PULL_COMID       	997 /* Separate ComId for a request [testing the PULL logic] */
#define PD_TEST_PULL_REPLY_COMID    995 /* Separate ComId for a request [testing the PULL logic] */
#define PD_TEST_ECHO_UNI_COMID      998
#define	PD_TESTSERVER_MESSAGE_MAX_SIZE 256
#define PD_TEST_GIGANTIC_PD      	996

#define MD_TEST_COMID       		901

/*	Test data sets	*/
TRDP_DATASET_T gDataSet990 =
{
    990,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    1,         /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT8,
            1
        },
        {
            TRDP_CHAR8,
            16
        }
    }
};

TRDP_DATASET_T gDataSet991 =
{
    991,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    1,         /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT8,
            1
        },
        {
            990,
            1
        }
    }
};

TRDP_DATASET_T gDataSet992 =
{
    992,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    1,         /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT8,
            1
        },
        {
            991,
            1
        }
    }
};

TRDP_DATASET_T gDataSet993 =
{
    993,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    1,         /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT8,
            1
        },
        {
            992,
            1
        }
    }
};

TRDP_DATASET_T gDataSet999 =
{
    999,        /*	dataset/com ID  */
    0,          /*	reserved		*/
    65,         /*	No of elements	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_BOOLEAN,  /*	data type		*/
            1               /*	no of elements	*/
        },
        {
            TRDP_CHAR8,
            1
        },
       {
            TRDP_UTF16,
            1
        },
        {
            TRDP_INT8,
            1
        },
        {
            TRDP_INT16,
            1
        },
        {
            TRDP_INT32,
            1
        },
        {
            TRDP_INT64,
            1
        },
        {
            TRDP_UINT8,
            1
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_UINT32,
            1
        },
        {
            TRDP_UINT64,
            1
        },
        {
            TRDP_REAL32,
            1
        },
        {
            TRDP_REAL64,
            1
        },
        {
            TRDP_TIMEDATE32,
            1
        },
        {
            TRDP_TIMEDATE48,
            1
        },
        {
            TRDP_TIMEDATE64,
            1
        },
        {
            TRDP_BOOLEAN,  /*	data type		*/
            4               /*	no of elements	*/
        },
        {
            TRDP_CHAR8,
            16
        },
       {
            TRDP_UTF16,
            16
        },
        {
            TRDP_INT8,
            4
        },
        {
            TRDP_INT16,
            4
        },
        {
            TRDP_INT32,
            4
        },
        {
            TRDP_INT64,
            4
        },
        {
            TRDP_UINT8,
            4
        },
        {
            TRDP_UINT16,
            4
        },
        {
            TRDP_UINT32,
            4
        },
        {
            TRDP_UINT64,
            4
        },
        {
            TRDP_REAL32,
            4
        },
        {
            TRDP_REAL64,
            4
        },
        {
            TRDP_TIMEDATE32,
            4
        },
        {
            TRDP_TIMEDATE48,
            4
        },
        {
            TRDP_TIMEDATE64,
            4
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_BOOLEAN,  /*	data type		*/
            0               /*	no of elements	*/
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_CHAR8,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_UTF16,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_INT8,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_INT16,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_INT32,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_INT64,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_UINT8,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_UINT16,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_UINT32,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_UINT64,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_REAL32,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_REAL64,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_TIMEDATE32,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_TIMEDATE48,
            0
        },
        {
            TRDP_UINT16,
            1
        },
        {
            TRDP_TIMEDATE64,
            0
        },
        {
            993,
            1
        }
    }
};


/**
 * Echo-dataset, that is returned by the server:
 * - cycletime
 * - returncode (status, of the last received package)
 * - values of dataset 999
 */
TRDP_DATASET_T gDataSet998 =
{
    998,        /*	dataset/com ID  */
    0,          /*	reserved		*/
    3,         /*	No of elements	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT32,    /*	data type		*/
            1               /*	no of elements	*/
        },
        {
            TRDP_UINT32,    /*	data type 		*/
            1               /*	no of elements	*/
        },
        {
            TRDP_CHAR8,
            0
        },
        {
            999,
            1
        }
    }
};

struct myDataSet990
{
    UINT8   level;
    char    string[16];
};

struct myDataSet991
{
    UINT8   level;
    struct myDataSet990 ds;
};

struct myDataSet992
{
    UINT8   level;
    struct myDataSet991 ds;
};

struct myDataSet993
{
    UINT8   level;
    struct myDataSet992 ds;
};

struct myDataSet1000
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
    UINT16  size_int32_1; /* again an value without real description */
    INT64   int64_0[4];
    UINT16  size_int64;
    UINT8   uint8_0[4];
    UINT16  size_uint8;
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
    struct myDataSet993 ds;
} gMyDataSet999 =
{
    1,                               /* BOOL8 */
    'A',
    0x0030,
    0x12,
    0x1234,
    0x12345678,
    0x123456789ABCDEF0,
    0x12,
    0x1234,
    0x12345678,
    0x123456789ABCDEF0,
    0.12345,
    0.12345678,
    0x12345678,
    {0x12345678, 0x9ABC},
    {0x12345678, 0x9ABCDEF0},
    {1,0,1,0},                      /* BOOL8 array fixed size */
    "Hello old World",
    {0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0},
    {0x12, 0x34, 0x55, 0x78},
    {0x1234, 0x5678, 0x9ABC, 0xDEF0},
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    {0x123456789ABCDEF1, 0x123456789ABCDEF2, 0x123456789ABCDEF3, 0x123456789ABCDEF4},
    {0x01, 0x23, 0x45, 0x67},
    {0x1234, 0x5678, 0x9ABC, 0xDEF0},
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    {0x123456789ABCDEF1, 0x123456789ABCDEF2, 0x123456789ABCDEF3, 0x123456789ABCDEF4},
    {0.12341, 0x12342, 0x12343, 0x12344},
    {0.12345671, 0.12345672, 0.12345673, 0.12345674},
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    {{0x12345671, 0x89A1},{0x12345672, 0x89A2},{0x12345673, 0x89A3},{0x12345674, 0x89A4}},
    {{0x12345671, 0x89ABCDE1},{0x12345672, 0x89ABCDE2},{0x12345673, 0x89ABCDE3}, {0x12345674, 0x89ABCDE4}},
    4,
    {1,0,1,0},                       /* BOOL8 array var size */
    16,
    "Hello old World",
    16,
    {0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0},
    4,
    {0x12, 0x34, 0x55, 0x78},
    4,
    {0x1234, 0x5678, 0x9ABC, 0xDEF0},
    4,
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    4,
    {0x123456789ABCDEF1, 0x123456789ABCDEF2, 0x123456789ABCDEF3, 0x123456789ABCDEF4},
    4,
    {0x12, 0x34, 0x55, 0x78},
    4,
    {0x1234, 0x5678, 0x9ABC, 0xDEF0},
    4,
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    4,
    {0x123456789ABCDEF1, 0x123456789ABCDEF2, 0x123456789ABCDEF3, 0x123456789ABCDEF4},
    4,
    {0.12341, 0.12342, 0.12343, 0x12344},
    4,
    {0.12345671, 0.12345672, 0.12345673, 0.12345674},
    4,
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    4,
    {{0x12345671, 0x89A1},{0x12345672, 0x89A2},{0x12345673, 0x89A3},{0x12345674, 0x89A4}},
    4,
    {{0x12345671, 0x89ABCDE1},{0x12345672, 0x89ABCDE2},{0x12345673, 0x89ABCDE3}, {0x12345674, 0x89ABCDE4}},
    {1, {2, {3, {4, "Nested Datasets"}}}}
};


struct myDataSet998
{
    UINT32  cycletime;
    UINT32	resultcode;
    
    /* the first two bytes are needed for the length of the dynamic size: */
    char	custom_message[PD_TESTSERVER_MESSAGE_MAX_SIZE + 2];

    struct myDataSet1000 echoDataset;
} gMyDataSet998;

/**********************************************************************************************************************/


/* We use dynamic memory	*/
#define RESERVED_MEMORY  900000


#endif
