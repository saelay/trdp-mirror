/*
 * $Id: $
 */
/******************************************************************************/
/**
 * @file            pdsend.c
 *
 * @brief           Demo application for TRDP
 *
 * @note            Project: Sending Demo application for TRDP
 *
 * @author          Bernd Lhr on 11.11.11, 2011 NewTec GmbH.
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2011.
 *
 */

/*******************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "trdp_if_light.h"
#include "vos_sock.h"
#include "pdsend.h"

/*******************************************************************************
 * DEFINITIONS
 */

#define MAX_PAYLOAD_SIZE  1024

#if BYTE_ORDER == LITTLE_ENDIAN
#define MAKE_LE(a)  (a)
#else
#define MAKE_LE(a)  (bswap_32(a))
#endif

/*******************************************************************************
 * GLOBALS
 */

pd_receive_packet_t gRec[] =
{
    {0, PD_COMID1, PD_COMID1_TIMEOUT, "10.0.0.200", 0, "", 0, 1},
    {0, PD_COMID1, PD_COMID1_TIMEOUT, "10.0.0.201", 0, "", 0, 1},
    {0, PD_COMID2, PD_COMID2_TIMEOUT, "10.0.0.202", 0, "", 0, 1},
    {0, PD_COMID2, PD_COMID2_TIMEOUT, "10.0.0.203", 0, "", 0, 1},
    {0, PD_COMID3, PD_COMID3_TIMEOUT, "10.0.0.204", 0, "", 0, 1},
    {0, 0, 0, "", 0, "", 0, 1}
};

UINT8       gDataBuffer[MAX_PAYLOAD_SIZE] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};      /*	Buffer for our PD data	*/

size_t      gDataSize   = 32;       /* Size of test data			*/
uint32_t    gComID      = PD_COMID0;
uint32_t    gInterval   = PD_COMID0_CYCLE;
char        gTargetIP[16];
int         gDataChanged    = 1;
int         gIsActive       = 1;
int32_t     gRecFD          = 0;

TRDP_APP_SESSION_T gAppHandle;   /*	Our identifier to the library instance	*/
TRDP_PUB_T  gPubHandle;          /*	Our identifier to the publication	*/

/*******************************************************************************
 * LOCAL
 */
static void callBack (void                  *pRefCon,
                      TRDP_APP_SESSION_T    appHandle,
                      const TRDP_PD_INFO_T  *pMsg,
                      UINT8                 *pData,
                      UINT32                dataSize);

/*******************************************************************************/

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

/******************************************************************************/
void setIP (const char *ipAddr)
{
    strcpy(gTargetIP, ipAddr);
}

/******************************************************************************/
void setComID (uint32_t comID)
{
    gComID = comID;
}

/******************************************************************************/
void setInterval (uint32_t interval)
{
    gInterval = interval * 1000;
}

/******************************************************************************/
void setIPRec (int index, const char *ipAddr)
{
    strcpy(gRec[index].srcIP, ipAddr);
}

/******************************************************************************/
void setComIDRec (int index, uint32_t comID)
{
    gRec[index].comID = comID;
}

void pd_updateSubscriber (int index)
{
    pd_sub(&gRec[index]);
}


/******************************************************************************/
void pd_stop (BOOL redundant)
{
    tlp_setRedundant(gAppHandle, 0, redundant);
}

/******************************************************************************/
/** pd_init
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int pd_init (
    const char  *pDestAddress,
    uint32_t    comID,
    uint32_t    interval)
{
    TRDP_PD_CONFIG_T        pdConfiguration = {callBack, NULL, {0, 0},
                                               TRDP_FLAGS_CALLBACK, 10000000, TRDP_TO_SET_TO_ZERO, 20548};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, 100000, {}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};


    printf("pd_init\n");

    strcpy(gTargetIP, pDestAddress);
    gComID      = comID;
    gInterval   = interval;


    /*	Init the library for dynamic operation	(PD only) */
    if (tlc_init(dbgOut,                            /* actually printf	*/
                 &dynamicConfig                    /* Use application supplied memory	*/
                 ) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*	Open a session for callback operation	(PD only) */
    if (tlc_openSession(&gAppHandle,
                        0, 0,                              /* use default IP addresses */
                        NULL,                              /* no Marshalling	*/
                        &pdConfiguration, NULL,            /* system defaults for PD and MD	*/
                        &processConfig) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*	Subscribe to control PDs		*/

    pd_sub(&gRec[0]);
    pd_sub(&gRec[1]);
    pd_sub(&gRec[2]);
    pd_sub(&gRec[3]);
    pd_sub(&gRec[4]);

    if (tlp_publish(gAppHandle, &gPubHandle, gComID, 0, 0, vos_dottedIP(gTargetIP), gInterval, 0,
                    TRDP_FLAGS_NONE, NULL, gDataBuffer, gDataSize) != TRDP_NO_ERR)
    {
        printf("Publish error\n");
        return 1;
    }
    return 0;
}

/******************************************************************************/
void pd_deinit ()
{
    /*
     *	We always clean up behind us!
     */

    tlp_unpublish(gAppHandle, gPubHandle);
    tlc_closeSession(gAppHandle);
    tlc_terminate();
    printf("pd_deinit\n");
}

/******************************************************************************/
void pd_updatePublisher (BOOL stop)
{
    TRDP_ERR_T err;
    err = tlp_unpublish(gAppHandle, gPubHandle);
    if (err != TRDP_NO_ERR)
    {
        printf("tlp_unpublish error %d\n", err);
    }
    gPubHandle = NULL;
    if (stop == FALSE)
    {
        err = tlp_publish(gAppHandle, &gPubHandle, gComID, 0, 0, vos_dottedIP(gTargetIP), gInterval, 0,
                          TRDP_FLAGS_NONE, NULL, gDataBuffer, gDataSize);
        if (err != TRDP_NO_ERR)
        {
            printf("tlp_publish error %d\n", err);
        }
    }
}

/******************************************************************************/
void pd_updateData (
    uint8_t *pData,
    size_t  dataSize)
{
    memcpy(gDataBuffer, pData, dataSize);
    gDataSize = dataSize;
    gDataChanged++;
    tlp_setRedundant(gAppHandle, 0, !gIsActive);
}

/******************************************************************************/
uint32_t  gray2hex (uint32_t in)
{
    static uint32_t last    = 0;
    uint32_t        ar[]    = {2, 0, 8, 0xc, 4, 6, 0xE};

    for (int i = 0; i < 7; i++)
    {
        if(ar[i] == in)
        {
            if (in == 2 && last > 3)
            {
                last = 7;
                return 7;
            }
            last = i;
            return i;
        }
    }
    return 0;
}

/******************************************************************************/
void pd_sub (
    pd_receive_packet_t *recPacket)
{
    if (recPacket->subHandle != 0)
    {
        tlp_unsubscribe(gAppHandle, recPacket->subHandle);
        recPacket->subHandle = NULL;
    }

    TRDP_ERR_T err = tlp_subscribe(
            gAppHandle,                                 /*	our application identifier			*/
            &recPacket->subHandle,                      /*	our subscription identifier			*/
            NULL,
            recPacket->comID,                           /*	ComID								*/
            0,                                          /*	topocount: local consist only		*/
            vos_dottedIP(recPacket->srcIP),             /*	Source IP filter 1					*/
            0,                                          /*	Source IP filter 2					*/
            0,
            0x0,                                        /*	Default destination	(or MC Group)   */
            recPacket->timeout,                         /*	Time out in us						*/
            TRDP_TO_SET_TO_ZERO,                        /*  delete invalid data	on timeout      */
            sizeof(recPacket->message) + sizeof(recPacket->counter));               /*	net data size   */

    if (err != TRDP_NO_ERR)
    {
        printf("trdp_subscribe error\n");
    }
}

/******************************************************************************/
pd_receive_packet_t *pd_get (
    int index)
{
    if (index < 0 || index >= sizeof(gRec) / sizeof(pd_receive_packet_t))
    {
        return NULL;
    }
    return &gRec[index];
}

/******************************************************************************/
/** Kind of specialized marshalling!
 *
 *  @param[in]      index           into our subscription array
 *  @param[in]      data			pointer to received data
 *  @param[in]      valid			flag for timeouts
 *  @retval         none
 */
void pd_getData (int index, uint8_t *data, int invalid)
{
    gRec[index].invalid = invalid;
    if (!invalid && data != NULL)
    {
        memcpy(&gRec[index].counter, data, sizeof(uint32_t));
        gRec[index].counter = ntohl(gRec[index].counter);
        strncpy((char *)(gRec[index].message), (char *)(data + 4), sizeof(gRec[index].message));
    }
    gRec[index].changed = 1;
}

/******************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pCallerRef		user supplied context pointer
 *  @param[in]      pMsg			pointer to message block
 *  @retval         none
 */
void callBack (
    void                    *pCallerRef,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    /*	Check why we have been called	*/
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            switch (pMsg->comId)
            {
                case 100:
                    printf("PD 100 received\n");
                    break;
                case 1000:
                    printf("PD 1000 received\n");
                    break;
                case PD_COMID1:
                    if (pMsg->srcIpAddr == ntohl(inet_addr(gRec[0].srcIP)))
                    {
                        pd_getData(0, pData, 0);
                    }
                    else
                    {
                        pd_getData(1, pData, 0);
                    }
                    break;
                case PD_COMID2:
                    if (pMsg->srcIpAddr == ntohl(inet_addr(gRec[2].srcIP)))
                    {
                        pd_getData(2, pData, 0);
                    }
                    else
                    {
                        pd_getData(3, pData, 0);
                    }
                    break;
                case PD_COMID3:
                    if (pData != NULL)
                    {
                        memcpy(&gRec[4].counter, pData, sizeof(uint32_t));
                        if (gRec[4].counter == 0x0a000000)
                        {
                            break;
                        }
                        gRec[4].counter = gray2hex(ntohl(gRec[4].counter));

                        gRec[4].changed = 1;
                        gRec[4].invalid = 0;
                    }
                    break;
                default:
                    break;
            }
            printf("ComID %d received (%d Bytes)\n", pMsg->comId, dataSize);
            if (pData && dataSize > 0)
            {
                printf("Msg: %s\n", pData);
            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept	*/
            printf("Packet timed out (ComID %d, SrcIP: %s)\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));

            switch (pMsg->comId)
            {
                case PD_COMID1:
                    if (pMsg->srcIpAddr == ntohl(inet_addr(gRec[0].srcIP)))
                    {
                        pd_getData(0, NULL, 1);
                    }
                    else
                    {
                        pd_getData(1, NULL, 1);
                    }
                    break;

                case PD_COMID2:
                    if (pMsg->srcIpAddr == ntohl(inet_addr(gRec[2].srcIP)))
                    {
                        pd_getData(2, NULL, 1);
                    }
                    else
                    {
                        pd_getData(3, NULL, 1);
                    }
                    /* [rec1Message setStringValue:@"Timed out"]; */
                    break;
                case PD_COMID3:
                    gRec[4].invalid = 1;
                    gRec[4].changed = 1;
                    break;
                default:
                    break;
            }

        default:
            /*
               printf("Error on packet received (ComID %d), err = %d\n",
                     pMsg->comId,
                     pMsg->resultCode);
             */
            break;
    }
}

/******************************************************************************/
int pd_loop2 ()
{
    /* INT32           pd_fd = 0; */
    TRDP_ERR_T  err;
    int         rv = 0;

    printf("pd_init\n");

    /*
     Enter the main processing loop.
     */
    while (1)
    {
        fd_set  rfds;
        INT32   noDesc;
        struct timeval  tv;
        struct timeval  max_tv = {0, 100000};

        if (gDataChanged)
        {
            /*	Copy the packet into the internal send queue, prepare for sending.
                If we change the data, just re-publish it	*/

            err = tlp_put(gAppHandle, gPubHandle, gDataBuffer, gDataSize);


            if (err != TRDP_NO_ERR)
            {
                printf("put pd error\n");
                /*
                   tlc_terminate();
                   return 1;
                 */
            }
            gDataChanged = 0;
        }

        /*
             Prepare the file descriptor set for the select call.
             Additional descriptors can be added here.
         */

        FD_ZERO(&rfds);
        FD_SET(gRecFD, &rfds);

        /*
             Compute the min. timeout value for select.
             This way we can guarantee that PDs are sent in time...
             (A requirement of the TRDP Protocol)
         */

        tlc_getInterval(gAppHandle, (TRDP_TIME_T *) &tv, (TRDP_FDS_T *) &rfds, &noDesc);

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
        rv = select((int)noDesc + 1, &rfds, NULL, NULL, &tv);

        /*
             Check for overdue PDs (sending and receiving)
             Send any PDs if it's time...
             Detect missing PDs...
             'rv' will be updated to show the handled events, if there are
             more than one...
             The callback function will be called from within the trdp_work
             function (in it's context and thread)!
         */

        tlc_process(gAppHandle, (TRDP_FDS_T *) &rfds, &rv);

        /* Handle other ready descriptors... */

        if (rv > 0)
        {
            printf("%sother descriptors were ready\n", vos_getTimeStamp());
        }
        else
        {
            /* printf("%slooping...\n", trdp_getTimeStamp()); */
        }

    }

    return rv;
}

#if 0
int pd_loop ()
{
    INT32       pd_fd = 0;
    TRDP_ERR_T  err;
    int         rv = 0;

    printf("pd_init\n");

    if (gDataChanged)
    {

        /*	Copy the packet into the internal send queue, prepare for sending.
            If we change the data, just re-publish it	*/

        err = trdp_publish(gComID,                  /*	ComID				*/
                           gTargetIP,                /*	Destination IP		*/
                           gInterval,               /*	Cycle time in ms	*/
                           TRDP_NONE,               /*	Is redundant		*/
                           gDataBuffer,             /*	pointer to data		*/
                           gDataSize);              /*	net data size		*/


        if (err != TRDP_NO_ERR)
        {
            printf("prep pd error\n");
            trdp_terminate();
            return 1;
        }
        gDataChanged = 0;
    }

    /*
     Enter the main processing loop.
     */
    if (1)
    {
        fd_set  rfds;
        struct timeval tv;
        struct timeval max_tv   = {0, 100};
        UINT8   sentPackages    = 0;

        /*
             Prepare the file descriptor set for the select call.
             Additional descriptors can be added here.
         */

        FD_ZERO(&rfds);
        FD_SET(pd_fd, &rfds);

        /*
             Compute the min. timeout value for select.
             This way we can guarantee that PDs are sent in time...
             (A requirement of the TRDP Protocol)
         */

        trdp_getInterval(&tv);

        /*
             The wait time for select must consider cycle times and timeouts of
             the PD packets received or sent.
             If we need to poll something faster than the lowest PD cycle,
             we need to set the maximum time out ourself
         */

        if (timercmp(&tv, &max_tv, >))
        {
            tv = max_tv;
        }

        /*
             Select() will wait for ready descriptors or time out,
             what ever comes first.
         */

        rv = select(pd_fd + 1, &rfds, NULL, NULL, &tv);

        /*
             Check for overdue PDs (sending and receiving)
             Send any PDs if it's time...
             Detect missing PDs...
             'rv' will be updated to show the handled events, if there are
             more than one...
             The callback function will be called from within the trdp_work
             function (in it's context and thread)!
         */

        trdp_process(&rfds, &rv, &sentPackages);

        /* Handle other ready descriptors... */

        if (rv > 0)
        {
            printf("%sother descriptors were ready\n", trdp_getTimeStamp());
        }
        else
        {
            printf("%slooping...\n", trdp_getTimeStamp());
        }

    }

    return rv;
}
#endif
