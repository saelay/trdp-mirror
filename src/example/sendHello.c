/**********************************************************************************************************************/
/**
 * @file            sendHello.c
 *
 * @brief           Demo application for TRDP
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr and Florian Weispfenning, NewTec GmbH
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#include "trdp_if_light.h"
#include "vos_thread.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION     "1.0"

#define DATA_MAX        1000

#define PD_COMID        1000
#define PD_COMID_CYCLE  1000000             /* in us (1000000 = 1 sec) */

/* We use dynamic memory	*/
#define RESERVED_MEMORY  10000

/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool sends PD messages to an ED.\n"
           "Arguments are:\n"
           "-t target IP address\n"
           "-c comId (default 1000)\n"
           "-d custom string to send (default: 'Hello World')\n"
           "-v print version and quit\n"
           );
}


/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (int argc, char *argv[])
{
    int                 ip[4];
    INT32               hugeCounter = 0;
    TRDP_APP_SESSION_T  appHandle;  /*	Our identifier to the library instance	*/
    TRDP_PUB_T          pubHandle;  /*	Our identifier to the publication	*/
    UINT32              comId = PD_COMID;
    TRDP_ERR_T          err;
    TRDP_PD_CONFIG_T    pdConfiguration = {NULL, NULL, {0, 64}, TRDP_FLAGS_NONE, 1000, TRDP_TO_SET_TO_ZERO};
    TRDP_MEM_CONFIG_T   dynamicConfig = {NULL, RESERVED_MEMORY, {}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};

    int                 rv = 0;


    UINT32              destIP = 0;

    /*	Generate some data, that we want to send, when nothing was specified. */
    UINT8               *outputBuffer;
    UINT8               exampleData[DATA_MAX]   = "Hello World";
    int                 outputBufferSize    = 32;

    UINT8               data[DATA_MAX];
    int                 ch;

    outputBuffer = exampleData;

    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "t:d:h?v")) != -1)
    {
        switch (ch)
        {
            case 'c':
            {   /*  read comId    */
                if (sscanf(optarg, "%u",
                           &comId) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                destIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
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
                destIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'd':
            {   /*  data    */
                char    c;
                int     dataSize = 0;
                do
                {
                    c = optarg[dataSize];
                    dataSize++;
                }
                while (c != '\0');

                if (dataSize >= DATA_MAX)
                {
                    fprintf(stderr, "The data is too long\n");
                    return 1;
                }
                memcpy(data, optarg, dataSize);
                outputBuffer        = data; /* move the pointer to the new data */
                outputBufferSize    = dataSize;
                break;
            }
            case 'v':   /*  version */
                printf("%s: Version %s\t(%s - %s)\n",
                       argv[0], APP_VERSION, __DATE__, __TIME__);
                exit(0);
                break;
            case 'h':
            case '?':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (destIP == 0)
    {
        fprintf(stderr, "No destination address given!\n");
        usage(argv[0]);
        return 1;
    }

    /*	Init the library  */
    if (tlc_init(NULL,                              /* no logging	*/
                 &dynamicConfig) != TRDP_NO_ERR)    /* Use application supplied memory	*/
        
    {
        printf("Initialization error\n");
        return 1;
    }

    /*	Open a session  */
    if (tlc_openSession(&appHandle,
                 0, 0,                      /* use default IP address */
                 NULL,                      /* no Marshalling	*/
                 &pdConfiguration, NULL,    /* system defaults for PD and MD	*/
                 &processConfig) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }
    
    /*	Copy the packet into the internal send queue, prepare for sending.	*/
    /*	If we change the data, just re-publish it	*/
    err = tlp_publish(  appHandle,                  /*	our application identifier	*/
                        &pubHandle,                 /*	our pulication identifier	*/
                        comId,                      /*	ComID to send				*/
                        0,                          /*	local consist only			*/
                        0,                          /*	default source IP			*/
                        destIP,                     /*	where to send to			*/
                        PD_COMID_CYCLE,             /*	Cycle time in us			*/
                        0,                          /*	not redundant				*/
                        TRDP_FLAGS_NONE,            /*	Use callback for errors		*/
                        NULL,                       /*	default qos and ttl			*/
                        (UINT8 *)outputBuffer,      /*	initial data                */
                        outputBufferSize,           /*	data size					*/
                        FALSE,                      /*	no ladder					*/
                        0);                         /*	no ladder					*/


    if (err != TRDP_NO_ERR)
    {
        printf("prep pd error\n");
        tlc_terminate();
        return 1;
    }

    /*
       Enter the main processing loop.
     */
    while (1)
    {
        fd_set  rfds;
        INT32   noDesc;
        struct timeval  tv;
        struct timeval  max_tv = {0, 100000};

        /*
           Prepare the file descriptor set for the select call.
           Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);
        /* FD_SET(pd_fd, &rfds); */

        /*
           Compute the min. timeout value for select.
           This way we can guarantee that PDs are sent in time
           with minimum CPU load and minimum jitter.
         */
        tlc_getInterval(appHandle, (TRDP_TIME_T *) &tv, (TRDP_FDS_T *) &rfds, &noDesc);

        /*
           The wait time for select must consider cycle times and timeouts of
           the PD packets received or sent.
           If we need to poll something faster than the lowest PD cycle,
           we need to set the maximum time out our self.
         */
        if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0)
        {
            tv = max_tv;
        }

        /*
           Select() will wait for ready descriptors or time out,
           what ever comes first.
         */
        rv = select((int)noDesc, &rfds, NULL, NULL, &tv);

        /*
           Check for overdue PDs (sending and receiving)
           Send any pending PDs if it's time...
           Detect missing PDs...
           'rv' will be updated to show the handled events, if there are
           more than one...
           The callback function will be called from within the tlc_process
           function (in it's context and thread)!
         */
        tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);

        /* Handle other ready descriptors... */
        if (rv > 0)
        {
            printf("other descriptors were ready\n");
        }
        else
        {
            printf(".");
			fflush(stdout);
        }

        sprintf((char *)outputBuffer, "Just a Counter: %08d", hugeCounter++);

        err = tlp_put(appHandle, pubHandle, outputBuffer, outputBufferSize);
        if (err != TRDP_NO_ERR)
        {
            printf("put pd error\n");
            rv = 1;
            break;
        }
    }

    /*
     *	We always clean up behind us!
     */
    tlp_unpublish(appHandle, pubHandle);

    tlc_terminate();
    return rv;
}
