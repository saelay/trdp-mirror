/*
 *  crc-test.c
 *  trdp
 *
 *  Created by Bernd Löhr on 18.01.13.
 *  Copyright 2013 NewTec GmbH. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vos_utils.h"

UINT8	gSampleDATA[] =
{
	0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,		/*	Testdata	*/
    0x0, 0x0, 0x0, 0x0							/*	CRC	*/  
};

int main()
{
	// Compute CRC and store in little endian */
    UINT32	myCrc; 
    
    myCrc = vos_crc32 (0, gSampleDATA, 8);
    gSampleDATA[8] = myCrc;
    gSampleDATA[9] = myCrc >> 8;
    gSampleDATA[10] = myCrc >> 16;
    gSampleDATA[11] = myCrc >> 24;
    
    printf("CRC: %08x\n", myCrc);
    printf("LE in memory (from low to high): %02hhx %02hhx %02hhx %02hhx\n", gSampleDATA[8], gSampleDATA[9], gSampleDATA[10], gSampleDATA[11]);
    
	// CRC Check on Null
    
	// Verify CRC */
    myCrc = vos_crc32 (0, gSampleDATA, 12);
    printf("Checked CRC over all: %08x\n", myCrc);
    
    if (myCrc == 0)
    {
        printf(" = Correct behavior!\n");
    }
    else
    {
        printf(" = Wrong!!\n");
    }
    
    // Compute CRC and store as big endian
    myCrc = vos_crc32 (0, gSampleDATA, 8);
    gSampleDATA[8] = myCrc >> 24;
    gSampleDATA[9] = myCrc >> 16;
    gSampleDATA[10] = myCrc >> 8;
    gSampleDATA[11] = myCrc;
    
    printf("CRC: %08x\n", myCrc);
    printf("BE in memory (from low to high): %02hhx %02hhx %02hhx %02hhx\n", gSampleDATA[8], gSampleDATA[9], gSampleDATA[10], gSampleDATA[11]);
    
	// CRC Check on Null
    
	// Verify CRC */
    myCrc = vos_crc32 (0, gSampleDATA, 12);
    printf("Checked CRC over all: %08x\n", myCrc);
    
    if (myCrc == 0)
    {
        printf(" = Correct behavior!\n");
    }
    else
    {
        printf(" = Wrong!!\n");
    }
}
