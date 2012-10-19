/*
//  test_server.h
//  trdp
//
//  Created by Bernd Löhr on 10.10.12.
//
*/

#ifndef trdp_test_server_h
#define trdp_test_server_h

/*  IP addresses:
 
    test_server <----.....----> test_client

*/

#define PD_TESTSERVER_IP        0x0a000064      /*	Sender's IP: 10.0.0.100         */
#define PD_TESTCLIENT_IP        0x0a000065      /*	Client's IP: 10.0.0.101         */
#define PD_TESTMULTICAST_IP     0xef000000      /*	Multicast Group: 239.0.0.0		*/

/* 
 Test dataset definitions
 */

/**********************************************************************************************************************/
/* PD unicast Test1                                                                                                   */
/**********************************************************************************************************************/

#define PD_TEST1_UNI_COMID      20001
#define PD_TEST1_UNI_CYCLE      10000000
#define PD_TEST1_UNI_TIMEOUT    5 * PD_TEST1_UNI_CYCLE
#define PD_TEST1_UNI_DST_IP     PD_TESTCLIENT_IP
#define PD_TEST1_UNI_SRC_IP     PD_TESTSERVER_IP

/*	Test data set	*/
TRDP_DATASET_T gDataSet20001 =
{
    PD_TEST1_UNI_COMID, 0,  11,
    {
        {TRDP_INT8, 1}, {TRDP_INT16, 1}, {TRDP_INT32, 1}, {TRDP_INT64, 1},
        {TRDP_UINT8, 1}, {TRDP_UINT16, 1}, {TRDP_UINT32, 1}, {TRDP_UINT64, 1},
        {TRDP_REAL32, 1}, {TRDP_REAL64, 1}, {TRDP_UINT8, 16}
    }
};

struct myDataSet20001
{
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
    char    string[16];
} gMyDataSet20001 =
{
    -1,
    4096,
    0x12345678,
    0x1234567890123456,
    255,
    65535,
    0x12345678,
    0x1234567890123456,
    0.12345678f,
    0.12345678,
    "Hello World"
};

/**********************************************************************************************************************/
/* PD multicast Test2                                                                                                 */
/**********************************************************************************************************************/
#define PD_TEST2_MC_COMID      20002
#define PD_TEST2_MC_CYCLE      5000000
#define PD_TEST2_MC_TIMEOUT    3 * PD_TEST2_MC_CYCLE
#define PD_TEST2_MC_DST_IP     PD_TESTMULTICAST_IP
#define PD_TEST2_MC_SRC_IP     PD_TESTSERVER_IP

/*	Test2 data set	*/
TRDP_DATASET_T gDataSet20002 =
{
    PD_TEST2_MC_COMID, 0,  11,
    {
        {TRDP_INT32, 1}, {TRDP_UINT8, 16}
    }
};

struct myDataSet20002
{
    INT32   counter;
    char    string[16];
} gMyDataSet20002 =
{
    0, "Hello World"
};


/**********************************************************************************************************************/
/* PD receive/echo Test3                                                                                              */
/**********************************************************************************************************************/
/* Receive and Send as echo:	*/
#define PD_TEST3_UNI_COMID      20003
#define PD_TEST3_UNI_CYCLE      1000000
#define PD_TEST3_UNI_TIMEOUT    30 * PD_TEST3_UNI_CYCLE
#define PD_TEST3_UNI_DST_IP     PD_TESTSERVER_IP
#define PD_TEST3_UNI_SRC_IP     PD_TESTCLIENT_IP

/*	Test2 data set	*/
TRDP_DATASET_T gDataSet20003 =
{
    PD_TEST3_UNI_COMID, 0,  11,
    {
        {TRDP_INT32, 1}, {TRDP_UINT8, 16}
    }
};

struct myDataSet20003
{
    INT32   counter;
    char    string[16];
} gMyDataSet20003 =
{
    0, "PULL Test"
};


/**********************************************************************************************************************/


/* We use dynamic memory	*/
#define RESERVED_MEMORY  10000


#endif
