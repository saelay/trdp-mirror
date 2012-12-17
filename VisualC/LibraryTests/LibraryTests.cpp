/******************************************************************************/
/**
 * @file            LibraryTests.cpp
 *
 * @brief           Some static tests of the TRDP library
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Florian Weispfenning
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id: $
 *
 */

/*******************************************************************************
 * INCLUDES
 */
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
//#include <sys/time.h>
//#include <sys/select.h>

#include <trdp_if_light.h>
#include "vos_thread.h"
#include "vos_utils.h"
#include "vos_sock.h"

// include also stuff, needed for windows:
#ifdef WIN32
#include <winsock2.h>
#endif

#include "stdafx.h"

int testTimeCompare()
{
	VOS_TIME_T time1 = { 1 /*sec */, 2 /* usec */ };
	VOS_TIME_T time2 = { 1 /*sec */, 2 /* usec */ };
	/* time 1 and time 2 should be equal */
	if (vos_cmpTime(&time1, &time2) != 0)
		return 1;

	time1.tv_sec = 1; time1.tv_usec = 2;
	time2.tv_sec = 1; time2.tv_usec = 3;
	/* time 1 should be shorter than time 2 */
	if (vos_cmpTime(&time1, &time2) != -1)
		return 1;

	time1.tv_sec = 1; time1.tv_usec = 2;
	time2.tv_sec = 2; time2.tv_usec = 4;
	/* time 1 should be shorter than time 2 */
	if (vos_cmpTime(&time1, &time2) != -1)
		return 1;

	time1.tv_sec = 1; time1.tv_usec = 3;
	time2.tv_sec = 1; time2.tv_usec = 2;
	/* time 1 should be greater than time 2 */
	if (vos_cmpTime(&time1, &time2) != 1)
		return 1;

	time1.tv_sec = 2; time1.tv_usec = 4;
	time2.tv_sec = 1; time2.tv_usec = 2;
	/* time 1 should be greater than time 2 */
	if (vos_cmpTime(&time1, &time2) != 1)
		return 1;


	/* macro timercmp() */
	/* there is a problem with >= and <= in windows */
	time1.tv_sec = 1; time1.tv_usec = 1;
	time2.tv_sec = 2; time2.tv_usec = 2;
	if (timercmp(&time1, &time2, <=) != 1)
		return 1;

	time1.tv_sec = 1; time1.tv_usec = 1;
	time2.tv_sec = 1; time2.tv_usec = 2;
	if (timercmp(&time1, &time2, <=) != 1)
		return 1;

	time1.tv_sec = 2; time1.tv_usec = 999999;
	time2.tv_sec = 3; time2.tv_usec = 0;
	if (timercmp(&time1, &time2, <=) != 1)
		return 1;

	/* test for equal */
	time1.tv_sec = 1; time1.tv_usec = 1;
	time2.tv_sec = 1; time2.tv_usec = 1;
	if (timercmp(&time1, &time2, <=) != 1)
		return 1; 

	time1.tv_sec = 2; time1.tv_usec = 2;
	time2.tv_sec = 1; time2.tv_usec = 1;
	if (timercmp(&time1, &time2, >=) != 1)
		return 1;

	time1.tv_sec = 1; time1.tv_usec = 2;
	time2.tv_sec = 1; time2.tv_usec = 1;
	if (timercmp(&time1, &time2, >=) != 1)
		return 1;

	time1.tv_sec = 2; time1.tv_usec = 0;
	time2.tv_sec = 1; time2.tv_usec = 999999;
	if (timercmp(&time1, &time2, >=) != 1)
		return 1;

	/* test for equal */
	time1.tv_sec = 3; time1.tv_usec = 4;
	time2.tv_sec = 3; time2.tv_usec = 4;
	if (timercmp(&time1, &time2, >=) != 1)
		return 1; 

	return 0; /* all time tests succeeded */
}

int testTimeAdd()
{
	VOS_TIME_T time = { 1 /*sec */, 0 /* usec */ };
	VOS_TIME_T add =  { 0 /*sec */, 2 /* usec */ };

	if (vos_addTime(&time, &add) != VOS_NO_ERR)
		return 1;
	if (time.tv_sec != 1 || time.tv_usec != 2)
		return 1;

	time.tv_sec =  1 /*sec */;	time.tv_usec = 1 /* usec */;
	add.tv_sec = 1 /*sec */;	add.tv_usec = 2 /* usec */;

	if (vos_addTime(&time, &add) != VOS_NO_ERR)
		return 1;
	if (time.tv_sec != 2 || time.tv_usec != 3)
		return 1;

	time.tv_sec =  1 /*sec */;	time.tv_usec = 1 /* usec */;
	add.tv_sec = 1 /*sec */;	add.tv_usec = 999999 /* usec */;

	if (vos_addTime(&time, &add) != VOS_NO_ERR)
		return 1;
	if (time.tv_sec != 3 || time.tv_usec != 0)
		return 1;

	time.tv_sec =  2 /*sec */;	time.tv_usec = 999999 /* usec */;
	add.tv_sec = 1 /*sec */;	add.tv_usec = 999999 /* usec */;

	if (vos_addTime(&time, &add) != VOS_NO_ERR)
		return 1;
	if (time.tv_sec != 4 || time.tv_usec != 999998)
		return 1;

	return 0; /* all time tests succeeded */
}


int testTimeSubs()
{
	VOS_TIME_T time = { 1 /*sec */, 4 /* usec */ };
	VOS_TIME_T subs =  { 0 /*sec */, 2 /* usec */ };

    if (vos_subTime(&time, &subs) != VOS_NO_ERR)
		return 1;
	if (time.tv_sec != 1 || time.tv_usec != 2)
		return 1;

	time.tv_sec =  1 /*sec */;	time.tv_usec = 3 /* usec */;
	subs.tv_sec = 1 /*sec */;	subs.tv_usec = 2 /* usec */;

	if (vos_subTime(&time, &subs) != VOS_NO_ERR)
		return 1;
	if (time.tv_sec != 0 || time.tv_usec != 1)
		return 1;

	time.tv_sec =  3 /*sec */;	time.tv_usec = 1 /* usec */;
	subs.tv_sec = 1 /*sec */;	subs.tv_usec = 999998 /* usec */;

	if (vos_subTime(&time, &subs) != VOS_NO_ERR)
		return 1;
	if (time.tv_sec != 1 || time.tv_usec != 3)
		return 1;

	time.tv_sec =  3 /*sec */;	time.tv_usec = 0 /* usec */;
	subs.tv_sec = 1 /*sec */;	subs.tv_usec = 999999 /* usec */;

	if (vos_subTime(&time, &subs) != VOS_NO_ERR)
		return 1;
	if (time.tv_sec != 1 || time.tv_usec != 1)
		return 1;

	return 0; /* all time tests succeeded */
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf("Starting tests\n");
	if (testTimeCompare())
	{
		printf("Time COMPARE test failed\n");
		return 1;
	}

	if (testTimeAdd())
	{
		printf("Time ADD test failed\n");
		return 1;
	}

    if (testTimeSubs())
	{
		printf("Time SUBSCRACT test failed\n");
		return 1;
	}


	printf("All tests successfully finished.\n");
	return 0;
}

