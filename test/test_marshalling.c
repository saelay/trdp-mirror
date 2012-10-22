/*
 *  test_marshalling.c
 *  trdp
 *
 *  Created by Bernd Löhr on 13.08.12.
 *  Copyright 2012 NewTec GmbH. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include "tau_marshall.h"

/*	Test data set	*/
TRDP_DATASET_T gDataSet1000 =
{
    1000,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    16,         /*	No of elements	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_BOOLEAN8,  /*	data type		*/
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
            TRDP_TIMEDATE64,
            1
        },
        {
            TRDP_CHAR8,
            16
        }
    }
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
    UINT32  timedate32_1;
    UINT64  timedate64_1;
    char    string[16];
} gMyDataSet1000 =
{
    1,
    'A',
    0xaa55;
    -1,
    4096,
    0x12345678,
    0x1234567890123456,
    255,
    65535,
    0x12345678,
    0x1234567890123456,
    0.12345,
    0.12345678,
    0x01234567;
    0x0123456789ABCDEF;
    "Hello World"
};

UINT8 gDstDataBuffer[1500];

int main ()
{
    INT32       index;
    UINT32      *refCon;
    TRDP_ERR_T  err;
    UINT32      bufSize;

    err = tau_initMarshall((void *)&refCon, 1, &gDataSet1000);

    bufSize = sizeof(gDstDataBuffer);
    memset(gDstDataBuffer, 0, bufSize);

    err = tau_marshall(refCon, 1000, (UINT8 *) &gMyDataSet1000, gDstDataBuffer, &bufSize);

    printf("ComID: 0x%0X\n", gDataSet1000.id);
    printf("  INT8 0x%02hhx,  INT16 0x%04hx,  INT32 0x%04x,  INT64 0x%08llx\n",
           gMyDataSet1000.int8_1,
           gMyDataSet1000.int16_1,
           gMyDataSet1000.int32_1,
           gMyDataSet1000.int64_1);
    printf(" UINT8 0x%02hhx, UINT16 0x%04hx, UINT32 0x%04x, UINT64 0x%08llx\n",
           gMyDataSet1000.uint8_1,
           gMyDataSet1000.uint16_1,
           gMyDataSet1000.uint32_1,
           gMyDataSet1000.uint64_1);
    printf(" float %f, double %lf String '%s'\n",
           gMyDataSet1000.float32_1,
           gMyDataSet1000.float64_1,
           gMyDataSet1000.string);

    for (index = 0; index < 8; index++)
    {
        printf("%02hhx, %02hhx, %02hhx, %02hhx, %02hhx, %02hhx, %02hhx, %02hhx, \n",
               gDstDataBuffer[index * 8 + 0],
               gDstDataBuffer[index * 8 + 1],
               gDstDataBuffer[index * 8 + 2],
               gDstDataBuffer[index * 8 + 3],
               gDstDataBuffer[index * 8 + 4],
               gDstDataBuffer[index * 8 + 5],
               gDstDataBuffer[index * 8 + 6],
               gDstDataBuffer[index * 8 + 7]
               );
    }


    return err;
}
