/**********************************************************************************************************************/
/**
 * @file            test_general.c
 *
 * @brief           Test sender/receiver/responder for TRDP
 *
 * @details	        Generic functionality, that could be used by the client and the server
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


/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_general.h"


#define DEFAULT_TIMEOUTFACTOR	2
#define TRDP_TEST_GENERAL_MICROSECOND (1000000)

/**********************************************************************************************************************/
/** Print a sensible usage message */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool sends PD messages to an ED and displayes received PD packages.\n"
           "Arguments are:\n"
           "-t target IP address\n"
           "-c cycle time in us\n"
           "-o timeout, also in us (must be larger than the cycle time) [optional, default: 2x cycle time]\n"
           "-s source IP address (of local interface to bind on)\n"
           "-v print version and quit\n"
           );
}

/**********************************************************************************************************************/
/** main entry
 * @param[out]      destIP		given IP address (could be used as target IP address)
 * @param[out]      cycletime	specify the time in us, the package will be sent again	
 * @param[out]      timeout		Time after receiving, that has to pass until a connection is declared as dead.
 * @param[out]      srcIP		IP address of local interface, to use [optional]
 * 
 *  @retval         0       no error
 *  @retval         1       some error
 */
int trdp_test_general_parseCommandLineArguments(int argc, char * *argv, 
			UINT32* destIP, UINT32* cycletime, UINT32* timeout, UINT32* srcIP)
{
    int                 ip[4];
    int                 ch;

    /* check if there is memory available */
    if (destIP == 0 || cycletime == 0 || timeout == 0)
    {
        return 1;
    }
    
    /* reset the timeout */
    *timeout = 0;
    
    /****** Parsing the command line arguments */
    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "t:c:o:s:h?v")) != -1)
    {
        switch (ch)
        {
			case 'c':
            {   /*  read cycle time */
                if (sscanf(optarg, "%u",
                           cycletime) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            }
            case 'o':            
            {
				/*  read timeout */
                if (sscanf(optarg, "%u",
                           timeout) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            }
            case 't':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                *destIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'v':   /*  version */
                printf("%s: Version %s\t(%s - %s)\n",
                       argv[0], APP_VERSION, __DATE__, __TIME__);
                exit(0);
                break;
            case 's':
            {   /*  read ip    */            
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
					usage(argv[0]);
					return 1;
                }
                *srcIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'h':
            case '?':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (*destIP == 0)
    {
        fprintf(stderr, "No destination address given!\n");
        usage(argv[0]);
        return 1;
    }
    
    /* set the default timeout, if it was not specified by the user */
    if (*timeout == 0)
    {
		*timeout = DEFAULT_TIMEOUTFACTOR * (*cycletime);
	}
    
    /* Check if there is an cycletime set */
    if (*cycletime == 0)
    {
        fprintf(stderr, "No time for each cycle is given!\n");
        usage(argv[0]);
        return 1;
    }
    
    return 0;
}


/**********************************************************************************************************************/
/** Calculate the difference between two time values
 * @param[out]     difference   the calculated time difference in microseconds
 * @param[in]      time1        the first time (the newer one)
 * @param[in]      time2        the other time must be older or equal to 'time1'
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  the difference parameter points to dead memory
 */
TRDP_ERR_T test_general_calcTimediff(UINT32 *difference, VOS_TIME_T time1, VOS_TIME_T time2)
{
    INT32 diff_sec = 0;

    if (difference == 0)
    {
        return TRDP_PARAM_ERR;
    }
    diff_sec = time1.tv_sec - time2.tv_sec;

    /* fetch a carry from seconds, when needed */
    /* one second has 1,000,000 microseconds */
    if (time1.tv_usec < time2.tv_usec)
    {
        diff_sec--;
        time1.tv_usec += TRDP_TEST_GENERAL_MICROSECOND;
    }
    (*difference) = time1.tv_usec - time2.tv_usec;
    (*difference) += (TRDP_TEST_GENERAL_MICROSECOND * diff_sec);

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** calculate the minium of two given number
 * @param[in]      number1
 * @param[in]      number2
 *
 *  @retval         number1, if this is the smaller one, else number2 is returned
 */
int trdp_general_min(int number1, int number2)
{
    if (number1 < number2)
        return number1;
    else
        return number2;
}
