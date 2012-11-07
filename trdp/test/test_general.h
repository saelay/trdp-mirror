/*
//  test_general.h
//  trdp
//
//  Created by Bernd Löhr on 10.10.12.
//
*/

#ifndef trdp_test_general_h
#define trdp_test_general_h

#define APP_VERSION     "0.09"

#include "vos_types.h"
#include "trdp_types.h"

int trdp_test_general_parseCommandLineArguments(int argc, char * *argv, 
			UINT32* destIP, UINT32* cycletime, UINT32* timeout, UINT32* srcIP);

TRDP_ERR_T test_general_calcTimediff(UINT32 *difference, VOS_TIME_T time1, VOS_TIME_T time2);

int trdp_general_min(int number1, int number2);

/**
 * Dynamic size
 * structure:
 * - length : UINT16
 * - data
 */
 typedef struct {
	 UINT16 length;
	 void*	pData;
} TRDP_TEST_DYNAMIC_SIZE_T;

#endif
