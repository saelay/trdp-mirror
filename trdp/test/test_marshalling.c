/*
 *  test_marshalling.c
 *  trdp
 *
 *  Created by Bernd Löhr on 13.08.12.
 *  Copyright 2012 NewTec GmbH. All rights reserved.
 *
 *	$ID:$
 */

#include <stdio.h>
#include <string.h>
#include "tau_marshall.h"

/*	Test data sets	*/
TRDP_DATASET_T gDataSet990 =
{
    990,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    2,         /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT8,
            1,
            NULL
        },
        {
            TRDP_CHAR8,
            16,
            NULL
        }
    }
};

TRDP_DATASET_T gDataSet991 =
{
    991,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    2,         /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT8,
            1,
            NULL
        },
        {
            990,
            1,
            NULL
        }
    }
};

TRDP_DATASET_T gDataSet992 =
{
    992,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    2,         /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT8,
            1,
            NULL
        },
        {
            991,
            1,
            NULL
        }
    }
};

TRDP_DATASET_T gDataSet993 =
{
    993,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    2,         /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT8,
            1,
            NULL
        },
        {
            992,
            1,
            NULL
        }
    }
};

TRDP_DATASET_T gDataSet1000 =
{
    1000,        /*	dataset/com ID  */
    0,          /*	reserved		*/
    65,         /*	No of elements	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_BOOLEAN,  /*	data type		*/
            1,               /*	no of elements	*/
            NULL
        },
        {
            TRDP_CHAR8,
            1,
            NULL
        },
       {
            TRDP_UTF16,
           1,
           NULL
        },
        {
            TRDP_INT8,
            1,
            NULL           
        },
        {
            TRDP_INT16,
            1,
            NULL
        },
        {
            TRDP_INT32,
            1,
            NULL
        },
        {
            TRDP_INT64,
            1,
            NULL
        },
        {
            TRDP_UINT8,
            1,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_UINT32,
            1,
            NULL
        },
        {
            TRDP_UINT64,	/* 10	*/
            1,
            NULL
        },
        {
            TRDP_REAL32,
            1,
            NULL
        },
        {
            TRDP_REAL64,
            1,
            NULL
        },
        {
            TRDP_TIMEDATE32,
            1,
            NULL
        },
        {
            TRDP_TIMEDATE48,
            1,
            NULL
        },
        {
            TRDP_TIMEDATE64,		/* 15	*/
            1,
            NULL
        },
        {	/* 16	*/
            TRDP_BOOLEAN,  /*	data type		*/
            4,
            NULL               /*	no of elements	*/
        },
        {
            TRDP_CHAR8,
            16,
            NULL
        },
       {
            TRDP_UTF16,
           16,
           NULL
        },
        {
            TRDP_INT8,
            4,
            NULL           
        },
        {
            TRDP_INT16,	/* 20	*/
            4,
            NULL
        },
        {
            TRDP_INT32,
            4,
            NULL
        },
        {
            TRDP_INT64,
            4,
            NULL
        },
        {
            TRDP_UINT8,
            4,
            NULL
        },
        {
            TRDP_UINT16,
            4,
            NULL
        },
        {
            TRDP_UINT32,		/* 25	*/
            4,
            NULL
        },
        {
            TRDP_UINT64,
            4,
            NULL
        },
        {
            TRDP_REAL32,
            4,
            NULL
        },
        {
            TRDP_REAL64,
            4,
            NULL
        },
        {
            TRDP_TIMEDATE32,
            4,
            NULL
        },
        {
            TRDP_TIMEDATE48,	/* 30	*/
            4,
            NULL
        },
        {
            TRDP_TIMEDATE64,
            4,
            NULL
        },
        {
            TRDP_UINT16,	/* 32	*/
            1,
            NULL
        },
        {
            TRDP_BOOLEAN,  /*	data type		*/
            0,               /*	no of elements	*/
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_CHAR8,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_UTF16,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_INT8,
            0,
            NULL           
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_INT16,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_INT32,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_INT64,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_UINT8,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_UINT16,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_UINT32,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_UINT64,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_REAL32,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_REAL64,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_TIMEDATE32,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_TIMEDATE48,
            0,
            NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL
        },
        {
            TRDP_TIMEDATE64,
            0,
            NULL
        },
        {
            993,
            1,
            NULL
        }
    }
};

TRDP_DATASET_T	gDataSet1001 = 
{
    1001,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    3,         /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_TIMEDATE64,	/*	Array	*/
            4,
            NULL
        },
        {
            TRDP_UINT16,		/*	Size of variable dataset	*/
            1,
            NULL
        },
        {
            TRDP_UINT8,
            0,
            NULL
        }
    }
};

/*	Will be sorted by tau_initMarshall	*/
TRDP_DATASET_T*	gDataSets[] =
{
    &gDataSet1001,
    &gDataSet1000,
    &gDataSet990,
    &gDataSet991,
    &gDataSet992,
    &gDataSet993
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
    struct myDataSet993 ds;
} gMyDataSet1000 =
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
    {0x12345678, 0x9ABC},			/* 14	*/
    {0x12345678, 0x9ABCDEF0},
    {1,0,1,0},                      /* BOOL8 array fixed size */
    "Hello old World",
    {0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0},
    {0x12, 0x34, 0x55, 0x78},
    {0x1234, 0x5678, 0x9ABC, 0xDEF0},	/* index == 20	*/
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
    4,								/* 32	*/
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
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},	/* 43	*/
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

struct myDataSet1001
{
    TIMEDATE64  timedate64_4[4];
	UINT16		size;
	UINT8		array[4];
} gMyDataSet1001 =
{
	{{0x12345671, 0x89ABCDE1},{0x12345672, 0x89ABCDE2},{0x12345673, 0x89ABCDE3}, {0x12345674, 0x89ABCDE4}},
    4,
    {1,0,1,0}                       /* UINT8 array var size */
};


TRDP_COMID_DSID_MAP_T	gComIdMap[] = {
    {1000, 1000},
    {1001, 1001}
};

UINT8 					gDstDataBuffer[1500];
struct myDataSet1000	gMyDataSet1000Copy;
struct myDataSet1001	gMyDataSet1001Copy;

/***********************************************************************************************************************
	Test marshalling and unmarshalling of test dataset
***********************************************************************************************************************/
int main ()
{
    /*INT32       index;*/
    UINT32      *refCon;
    TRDP_ERR_T  err;
    UINT32      bufSize;

    err = tau_initMarshall((void *)&refCon, 2, gComIdMap, 6, gDataSets);

	if (err != TRDP_NO_ERR)
    {
        printf("tau_initMarshall returns error %d\n", err);
        return 1;
    }

    bufSize = sizeof(gDstDataBuffer);
    memset(gDstDataBuffer, 0, bufSize);

    err = tau_marshall (refCon, 1000, (UINT8 *) &gMyDataSet1000, gDstDataBuffer, &bufSize, NULL);

	if (err != TRDP_NO_ERR)
    {
        printf("tau_marshall returns error %d\n", err);
        return 1;
    }

	bufSize = sizeof(gMyDataSet1000Copy);
    memset(&gMyDataSet1000Copy, 0, bufSize);

    err = tau_unmarshall (refCon, 1000, gDstDataBuffer, (UINT8 *) &gMyDataSet1000Copy, &bufSize, NULL);

	if (err != TRDP_NO_ERR)
    {
        printf("tau_unmarshall returns error %d\n", err);
        return 1;
    }
    
	if (memcmp(&gMyDataSet1000, &gMyDataSet1000Copy, sizeof(gMyDataSet1000)) != 0)
	{
        printf("Something's wrong in the state of Marshalling!\n");
        return 1;
        
    }
    else
    {
        printf("Marshalling and Unmarshalling data matched!\n");
    }

	/*
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
    printf(" float %f, double %lf\n",
           gMyDataSet1000.float32_1,
           gMyDataSet1000.float64_1);

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
	*/

    return 0;
}
