/**********************************************************************************************************************/
/**
 * @file            getStatsMarshall.c
 *
 * @brief           Test application for TRDP statistics
 *
 * @details			Send PD Pull request for statistics and display them by unmarshalling them
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
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
#include <unistd.h>
#include <sys/select.h>

#include "trdp_if_light.h"
#include "tau_marshall.h"

#include "vos_thread.h"
#include "vos_sock.h"
#include "vos_utils.h"

/* Some sample comId definitions	*/

#define PD_OWN_IP               0 //vos_dottedIP("192.168.2.2")

/* Expect receiving:	*/
#define PD_COMID1               12
#define PD_COMID1_CYCLE         0
#define PD_COMID1_TIMEOUT       5000000
#define PD_COMID1_DATA_SIZE     sizeof(TRDP_STATISTICS_T)
//#define PD_COMID1_SRC_IP        vos_dottedIP("192.168.2.4")     /*	Sender's IP		*/
#define PD_COMID1_SRC_IP        vos_dottedIP("10.64.8.203")     /*	Sender's IP		*/

/* Send as request:	*/
#define PD_COMID2               11
#define PD_COMID2_DATA_SIZE     0
#define PD_COMID2_DST_IP        PD_COMID1_SRC_IP

/* We use dynamic memory	*/
#define RESERVED_MEMORY  64000
#define PREALLOCATE		{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0}

#define APP_VERSION  "0.0.0.3"

TRDP_STATISTICS_T   gBuffer;
BOOL                gKeepOnRunning = TRUE;

/**********************************************************************************************************************/
/*
 *  Dataset definitions
 */
#if 0
#define	TRDP_STATS_DS_ID		111
#define	TRDP_MEM_STATS_DS_ID	TRDP_STATS_DS_ID + 1
#define	TRDP_PD_STATS_DS_ID		TRDP_MEM_STATS_DS_ID + 1
#define	TRDP_MD_STATS_DS_ID		TRDP_PD_STATS_DS_ID + 1

/*	Statistics data set	*/

TRDP_DATASET_T gMemStatisticsDS =
{
    TRDP_MEM_STATS_DS_ID,		/*	dataset/com ID  */
    0,          				/*	reserved		*/
    3,          /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT32,
            6
        },
        {
            TRDP_UINT32,
            15
        },
        {
            TRDP_UINT32,
            15
        }
	}
};

TRDP_DATASET_T gPDStatisticsDS =
{
    TRDP_PD_STATS_DS_ID,		/*	dataset/com ID  */
    0,          				/*	reserved		*/
    1,          /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT32,
            13
        }
	}
};

TRDP_DATASET_T gMDStatisticsDS =
{
    TRDP_MD_STATS_DS_ID,		/*	dataset/com ID  */
    0,          				/*	reserved		*/
    1,          /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT32,
            13
        }
	}
};

TRDP_DATASET_T gStatisticsDS =
{
    TRDP_STATS_DS_ID,       /*	dataset/com ID  */
    0,          /*	reserved		*/
    8,          /*	No of elements, var size	*/
    {           /*	TRDP_DATASET_ELEMENT_T[]	*/
        {
            TRDP_UINT32,
            1
        },
        {
            TRDP_TIMEDATE64,
            1
        },
        {
            TRDP_TIMEDATE32,
            2
        },
        {
            TRDP_CHAR8,
            16
        },
        {
            TRDP_CHAR8,
            16
        },
        {
            TRDP_MEM_STATS_DS_ID,
            1
        },
        {
            TRDP_PD_STATS_DS_ID,
            1
        },
        {
            TRDP_MD_STATS_DS_ID,
            1
        },
    }
};

/*	Will be sorted by tau_initMarshall	*/
TRDP_DATASET_T*	gDataSets[] =
{
    &gStatisticsDS,
    &gMemStatisticsDS,
    &gPDStatisticsDS,
    &gMDStatisticsDS
};

#endif

#include "trdp_reserved.h"

extern TRDP_COMID_DSID_MAP_T	gComIdMap[];
extern TRDP_DATASET_T           *gDataSets[];
extern const UINT32             cNoOfDatasets;

void print_stats(
    TRDP_STATISTICS_T*  pData)
{
    int i;

    printf("\n--------------------\n");
    printf("version:        %u\n", pData->version);
    printf("timestamp:      %u:%u\n", pData->timeStamp.tv_sec, pData->timeStamp.tv_usec);
    printf("upTime:         %u\n", pData->upTime);
    printf("statisticTime:  %u\n", pData->statisticTime);
    printf("ownIpAddr:      %u\n", pData->ownIpAddr);
    printf("leaderIpAddr:   %u\n", pData->leaderIpAddr);
    printf("processPrio:    %u\n", pData->processPrio);
    printf("processCycle:   %u\n", pData->processCycle);

    printf("mem.total:          %u\n", pData->mem.total);
    printf("mem.free:           %u\n", pData->mem.free);
    printf("mem.minFree:        %u\n", pData->mem.minFree);
    printf("mem.numAllocBlocks: %u\n", pData->mem.numAllocBlocks);
    printf("mem.numAllocErr:    %u\n", pData->mem.numAllocErr);
    printf("mem.numFreeErr:     %u\n", pData->mem.numFreeErr);

    printf("mem.allocBlockSizes: ");
    for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
    {
        printf("%u, ", i, pData->mem.allocBlockSize[i]);
    }
    
    printf("\nmem.usedBlockSize:   ");
    for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
    {
        printf("%u, ", i, pData->mem.usedBlockSize[i]);
    }

    /* Process data */
    printf("\npd.defQos:      %u\n", pData->pd.defQos);
    printf("pd.defTtl:      %u\n", pData->pd.defTtl);
    printf("pd.defTimeout:  %u\n", pData->pd.defTimeout);
    printf("pd.numSubs:     %u\n", pData->pd.numSubs);
    printf("pd.numPub:      %u\n", pData->pd.numPub);
    printf("pd.numRcv :     %u\n", pData->pd.numRcv);
    printf("pd.numCrcErr:   %u\n", pData->pd.numCrcErr);
    printf("pd.numProtErr:  %u\n", pData->pd.numProtErr);
    printf("pd.numTopoErr:  %u\n", pData->pd.numTopoErr);
    printf("pd.numNoSubs:   %u\n", pData->pd.numNoSubs);
    printf("pd.numNoPub:    %u\n", pData->pd.numNoPub);
    printf("pd.numTimeout:  %u\n", pData->pd.numTimeout);
    printf("pd.numSend:     %u\n", pData->pd.numSend);
    printf("--------------------\n");
}

/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool requests the general statistics from an ED.\n"
           "Arguments are:\n"
           "-t target IP address in dotted decimal\n"
           "-v print version and quit\n"
           );
}

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon			user supplied context pointer
 *  @param[in]		category		Log category (Error, Warning, Info etc.)
 *  @param[in]		pTime			pointer to NULL-terminated string of time stamp
 *  @param[in]		pFile			pointer to NULL-terminated string of source module
 *  @param[in]		LineNumber		line
 *  @param[in]		pMsgStr         pointer to NULL-terminated string
 *  @retval         none
 */
void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:"};
    printf("%s %s %s:%d %s",
           pTime,
           catStr[category],
           pFile,
           LineNumber,
           pMsgStr);
}


/**********************************************************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pRefCon			user supplied context pointer
 *  @param[in]      pMsg			pointer to header/packet infos
 *  @param[in]      pData			pointer to data block
 *  @param[in]      dataSize		pointer to data size
 *  @retval         none
 */
void myPDcallBack (
    void                    *pRefCon,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{

    /*	Check why we have been called	*/
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            printf("ComID %d received\n", pMsg->comId);
            if (pData)
            {
                if (pMsg->comId == 12)
                {
            		tau_unmarshall(NULL, pMsg->comId, pData, (UINT8*) &gBuffer, &dataSize);
                    print_stats(&gBuffer);
                    gKeepOnRunning = FALSE;
                }
            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept	*/
            printf("Packet timed out (ComID %d, SrcIP: %s)\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->srcIpAddr));
            memset(&gBuffer, 0, sizeof(gBuffer));
            break;
        default:
            printf("Error on packet received (ComID %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
            break;
    }
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (int argc, char * *argv)
{
    TRDP_APP_SESSION_T  	appHandle;  /*	Our identifier to the library instance	*/
    TRDP_SUB_T          	subHandle;  /*	Our identifier to the subscription	*/
    TRDP_ERR_T          	err;
    TRDP_PD_CONFIG_T    	pdConfiguration = {myPDcallBack, NULL, {0, 0},
                                        	TRDP_FLAGS_CALLBACK, 10000000, TRDP_TO_SET_TO_ZERO, 20548};
    TRDP_MEM_CONFIG_T   	dynamicConfig = {NULL, RESERVED_MEMORY, {}};
    TRDP_PROCESS_CONFIG_T	processConfig = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};
    TRDP_MARSHALL_CONFIG_T	marshall = {NULL, tau_unmarshall, 0};

    int                 rv = 0;
    int                 ch;
    int                 ip[4];
    UINT32              destIP = PD_COMID2_DST_IP;
    UINT32      		*refCon;

    while ((ch = getopt(argc, argv, "t:h?v")) != -1)
    {
        switch (ch)
        {
            case 't':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u", &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                destIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'v':   /*  version */
                printf("%s: Version %s\t(%s - %s)\n", argv[0], APP_VERSION, __DATE__, __TIME__);
                exit(0);
                break;
            case 'h':
            case '?':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    /*	Init the library for callback operation	(PD only) */
    if (tlc_init(dbgOut,                            /* actually printf	*/
                 &dynamicConfig                    /* Use application supplied memory	*/
                ) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

	if (tau_initMarshall((void *)&refCon, 1, gComIdMap, cNoOfDatasets, gDataSets) != TRDP_NO_ERR)
    {
        printf("Marshalling initialization error\n");
        return 1;
    }
    
    /*	Open a session for callback operation	(PD only) */
    if (tlc_openSession(&appHandle,
                 PD_OWN_IP, 0,                      /* use default IP addresses */
                 &marshall,                         /* marshalling	*/
                 &pdConfiguration, NULL,            /* system defaults for PD and MD	*/
                 &processConfig) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*	Subscribe to control PD		*/

    memset(&gBuffer, 0, sizeof(gBuffer));

    err = tlp_subscribe( appHandle,                 /*	our application identifier			*/
                         &subHandle,                /*	our subscription identifier			*/
                         NULL,
                         PD_COMID1,                 /*	ComID								*/
                         0,                         /*	topocount: local consist only		*/
                         0,                         /*	Source IP filter					*/
                         0,
                         0xEF000000,/*PD_OWN_IP, */                /*	Default destination	(or MC Group)   */
                         PD_COMID1_TIMEOUT,         /*	Time out in us						*/
                         TRDP_TO_SET_TO_ZERO,       /*  delete invalid data	on timeout      */
                         sizeof(gBuffer));          /*	net data size						*/

    if (err != TRDP_NO_ERR)
    {
        printf("prep pd receive error\n");
        tlc_terminate();
        return 1;
    }

    /*	Request statistics PD		*/

    err = tlp_request(appHandle, subHandle, PD_COMID2, 0, 0, destIP, 0, TRDP_FLAGS_NONE, 0, NULL, 0, 12, 0xEF000000, 0, 0);

    if (err != TRDP_NO_ERR)
    {
        printf("prep pd publish error\n");
        tlc_terminate();
        return 1;
    }


    /*
        Enter the main processing loop.
     */
    while (gKeepOnRunning)
    {
        fd_set  rfds;
        INT32   noOfDesc;
        struct timeval  tv;
        struct timeval  max_tv = {5, 0};

        /*
            Prepare the file descriptor set for the select call.
            Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);
        /*
            Compute the min. timeout value for select and return descriptors to wait for.
            This way we can guarantee that PDs are sent in time...
         */
        tlc_getInterval(appHandle,
                        (TRDP_TIME_T *) &tv,
                        (TRDP_FDS_T *) &rfds,
                        &noOfDesc);

        /*
            The wait time for select must consider cycle times and timeouts of
            the PD packets received or sent.
            If we need to poll something faster than the lowest PD cycle,
            we need to set the maximum timeout ourselfs
         */

        if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0)
        {
            tv = max_tv;
        }

        /*
            Select() will wait for ready descriptors or timeout,
            what ever comes first.
         */

        rv = select((int)noOfDesc, &rfds, NULL, NULL, &tv);

        /*
            Check for overdue PDs (sending and receiving)
            Send any PDs if it's time...
            Detect missing PDs...
            'rv' will be updated to show the handled events, if there are
            more than one...
            The callback function will be called from within the trdp_work
            function (in it's context and thread)!
         */

        tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);

        /*
           Handle other ready descriptors...
         */
        if (rv > 0)
        {
            printf("other descriptors were ready\n");
        }
        else
        {
            printf(".");
            fflush(stdout);
        }

    }   /*	Bottom of while-loop	*/

    /*
     *	We always clean up behind us!
     */
    tlp_unsubscribe(appHandle, subHandle);

    tlc_terminate();

    return rv;
}
