/**********************************************************************************************************************/
/**
 * @file            tau_tti.c
 *
 * @brief           Functions for train topology information access
 *
 * @details         The TTI subsystem maintains a pointer to the TAU_TTDB struct in the TRDP session struct.
 *                  That TAU_TTDB struct keeps the subscription and listener handles, the current TTDB directories and
 *                  a pointer list to consist infos (in network format). On init, most TTDB data is requested from the
 *                  ECSP plus the own consist info.
 *                  This data is automatically updated if an inauguration is detected. Additional consist infos are
 *                  requested on demand, only.
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          B. Loehr (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2015. All rights reserved.
 *
 * $Id$
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>
#include <stdio.h>

#include "trdp_if_light.h"
#include "trdp_utils.h"
#include "tau_marshall.h"
#include "tau_tti.h"
#include "iec61375-2-3.h"

#include "tau_cstinfo.c"

/***********************************************************************************************************************
 * DEFINES
 */

#define TTI_CACHED_CONSISTS  8          /**< We hold this number of consist infos (ca. 105kB) */

/***********************************************************************************************************************
 * TYPEDEFS
 */

typedef struct TAU_TTDB
{
    TRDP_SUB_T                      pd100SubHandle;
    TRDP_LIS_T                      md101Listener;
    TRDP_LIS_T                      md102Listener;
    TRDP_OP_TRAIN_DIR_STATUS_INFO_T opTrnState;
    TRDP_OP_TRAIN_DIR_T             opTrnDir;
    TRDP_TRAIN_DIR_T                trnDir;
    TRDP_TRAIN_NET_DIR_T            trnNetDir;
    UINT32                          noOfCachedCst;
    UINT32                          cstSize[TRDP_MAX_CST_CNT];
    TRDP_CONSIST_INFO_T             *cstInfo[TRDP_MAX_CST_CNT];
} TAU_TTDB_T;

/***********************************************************************************************************************
 *   Locals
 */

static void ttiRequestTTDBdata (TRDP_APP_SESSION_T  appHandle,
                                UINT32              comID,
                                const TRDP_UUID_T   cstUUID);

/**********************************************************************************************************************/
/**    Function returns the UUID for the given UIC ID
 *      We need to search in the OP_TRAIN_DIR the OP_VEHICLE where the vehicle is the
 *      first one in the consist and its name matches.
 *      Note: The first vehicle in a consist has the same ID as the consist it is belonging to (5.3.3.2.5)
 */
void ttiGetUUIDfromLabel (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_UUID_T         cstUUID,
    const TRDP_LABEL_T  cstLabel)
{
    UINT32 i, j;

    /* Search the vehicles in the OP_TRAIN_DIR for a matching vehID */

    for (i = 0; i < appHandle->pTTDB->opTrnDir.opVehCnt; i++)
    {
        if (vos_strnicmp(appHandle->pTTDB->opTrnDir.opVehList[i].vehId, cstLabel, sizeof(TRDP_LABEL_T)) == 0)
        {
            /* vehicle found, is it the first in the consist?   */
            UINT8 opCstNo = appHandle->pTTDB->opTrnDir.opVehList[i].ownOpCstNo;
            for (j = 0; j < appHandle->pTTDB->opTrnDir.opCstCnt; j++)
            {
                if (opCstNo == appHandle->pTTDB->opTrnDir.opCstList[j].opCstNo)
                {
                    memcpy(cstUUID, appHandle->pTTDB->opTrnDir.opCstList[j].cstUUID, sizeof(TRDP_UUID_T));
                    return;
                }
            }
        }
    }
    /* not found    */
    memset(cstUUID, 0, sizeof(TRDP_UUID_T));
}

BOOL8   ttiIsOwnCstInfo (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_CONSIST_INFO_T *pTelegram)
{
    UINT32 i;
    for (i = 0; i < appHandle->pTTDB->opTrnDir.opCstCnt; i++)
    {
        if (appHandle->pTTDB->opTrnState.ownTrnCstNo == appHandle->pTTDB->opTrnDir.opCstList[i].trnCstNo)
        {
            return memcmp(appHandle->pTTDB->opTrnDir.opCstList[i].cstUUID, pTelegram->cstUUID,
                          sizeof(TRDP_UUID_T)) == 0;
        }
    }
    return FALSE;
}

/**********************************************************************************************************************/
/**    Function called on reception of process data
 *
 *  Handle and process incoming data, update our data store
 *
 *  @param[in]      pRefCon         unused.
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      pMsg            Pointer to the message info (header etc.)
 *  @param[in]      pData           Pointer to the network buffer.
 *  @param[in]      dataSize        Size of the received data
 *
 *  @retval         none
 *
 */
static void ttiPDCallback (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    int         changed = 0;
    VOS_SEMA_T  waitForInaug = (VOS_SEMA_T) pRefCon;

    if (pMsg->comId == TTDB_STATUS_COMID)
    {
        if (pMsg->resultCode == TRDP_NO_ERR &&
            dataSize <= sizeof(TRDP_OP_TRAIN_DIR_STATUS_INFO_T))
        {
            TRDP_OP_TRAIN_DIR_STATUS_INFO_T *pTelegram = (TRDP_OP_TRAIN_DIR_STATUS_INFO_T *) pData;
            UINT32 crc;

            if (appHandle->etbTopoCnt != vos_ntohl(pTelegram->etbTopoCnt))
            {
                changed++;
                tlc_setETBTopoCount(appHandle, vos_ntohl(pTelegram->etbTopoCnt));
            }

            /* check the crc:   */
            crc = vos_crc32(0xFFFFFFFF, (const UINT8 *) &pTelegram->state, dataSize - 4);
            if (crc != MAKE_LE(pTelegram->state.crc))
            {
                vos_printLog(VOS_LOG_ERROR, "CRC error of received operational status info (%08x != %08x)!\n",
                             crc, vos_ntohl(pTelegram->state.crc))
                tlc_setOpTrainTopoCount(appHandle, 0);
                return;
            }
            memcpy(&appHandle->pTTDB->opTrnState, &pTelegram->state, dataSize);

            /* unmarshall manually:   */
            appHandle->pTTDB->opTrnState.state.opTrnTopoCnt = vos_ntohl(pTelegram->state.opTrnTopoCnt);
            
            if (appHandle->opTrnTopoCnt != vos_ntohl(pTelegram->state.opTrnTopoCnt))
            {
                changed++;
                tlc_setOpTrainTopoCount(appHandle, appHandle->pTTDB->opTrnState.state.opTrnTopoCnt);
            }

            tlc_setOpTrainTopoCount(appHandle, appHandle->pTTDB->opTrnState.state.opTrnTopoCnt);
            appHandle->pTTDB->opTrnState.state.crc = vos_ntohl(pTelegram->state.crc);
        }
        else if (pMsg->resultCode == TRDP_TIMED_OUT)
        {
            if (appHandle->etbTopoCnt != 0)
            {
                changed++;
                tlc_setETBTopoCount(appHandle, 0);
            }
            if (appHandle->opTrnTopoCnt != 0)
            {
                changed++;
                tlc_setOpTrainTopoCount(appHandle, 0);
            }
        }
        if (changed > 0 && waitForInaug != NULL)
        {
            vos_semaGive(waitForInaug);
        }
    }
}

/**********************************************************************************************************************/
/*  Functions to convert TTDB network packets into local (static) representation                                      */
/**********************************************************************************************************************/
static void ttiStoreOpTrnDir (
    TRDP_APP_SESSION_T  appHandle,
    UINT8               *pData)
{
    TRDP_OP_TRAIN_DIR_T *pTelegram = (TRDP_OP_TRAIN_DIR_T *) pData;
    UINT32 size;

    /* we have to unpack the data, copy up to OP_CONSIST */
    if (pTelegram->opCstCnt > TRDP_MAX_CST_CNT)
    {
        vos_printLog(VOS_LOG_ERROR, "Max count of consists of received operational dir exceeded (%d)!\n",
                     pTelegram->opCstCnt);
        return;
    }

    /* 8 Bytes up to opCstCnt plus number of Consists  */
    size = 8 + pTelegram->opCstCnt * sizeof(TRDP_OP_CONSIST_T);
    memcpy(&appHandle->pTTDB->opTrnDir, pData, size);
    pData   += size + 3;            /* jump to cnt  */
    size    = *pData++ *sizeof(TRDP_OP_VEHICLE_T) + sizeof(UINT32); /* copy opTrnTopoCnt as well    */
    memcpy(&appHandle->pTTDB->opTrnDir, pData, size);

    /* unmarshall manually and update the opTrnTopoCount   */
    appHandle->pTTDB->opTrnDir.opTrnTopoCnt = vos_ntohl(appHandle->pTTDB->opTrnDir.opTrnTopoCnt);
    tlc_setOpTrainTopoCount(appHandle, appHandle->pTTDB->opTrnDir.opTrnTopoCnt);
}

static void ttiStoreTrnDir (
    TRDP_APP_SESSION_T  appHandle,
    UINT8               *pData)
{
    TRDP_TRAIN_DIR_T *pTelegram = (TRDP_TRAIN_DIR_T *) pData;
    UINT32 size, i;

    /* we have to unpack the data, copy up to CONSIST */
    if (pTelegram->cstCnt > TRDP_MAX_CST_CNT)
    {
        vos_printLog(VOS_LOG_ERROR, "Max count of consists of received train dir exceeded (%d)!\n",
                     pTelegram->cstCnt);
        return;
    }

    /* 4 Bytes up to cstCnt plus number of Consists  */
    size = 4 + pTelegram->cstCnt * sizeof(TRDP_CONSIST_T);
    memcpy(&appHandle->pTTDB->trnDir, pData, size);
    pData += size;              /* jump to trnTopoCount  */

    /* unmarshall manually and update the trnTopoCount   */
    appHandle->pTTDB->trnDir.trnTopoCnt = vos_ntohl((*(UINT32 *)pData));   /* copy trnTopoCnt as well    */

    /* swap the consist topoCnts    */
    for (i = 0; i < appHandle->pTTDB->trnDir.cstCnt; i++)
    {
        appHandle->pTTDB->trnDir.cstList[i].cstTopoCnt = vos_ntohl(appHandle->pTTDB->trnDir.cstList[i].cstTopoCnt);
    }
}

static void ttiStoreTrnNetDir (
    TRDP_APP_SESSION_T  appHandle,
    UINT8               *pData)
{
    TRDP_TRAIN_NET_DIR_T *pTelegram = (TRDP_TRAIN_NET_DIR_T *) pData;
    UINT32 size, i;

    /* we have to unpack the data, copy up to CONSIST */

    appHandle->pTTDB->trnNetDir.reserved01  = 0;
    appHandle->pTTDB->trnNetDir.entryCnt    = vos_ntohs(pTelegram->entryCnt);
    if (appHandle->pTTDB->trnNetDir.entryCnt > TRDP_MAX_CST_CNT)
    {
        vos_printLog(VOS_LOG_ERROR, "Max count of consists of received train net dir exceeded (%d)!\n",
                     vos_ntohs(pTelegram->entryCnt));
        return;
    }

    /* 4 Bytes up to cstCnt plus number of Consists  */
    size = appHandle->pTTDB->trnNetDir.entryCnt * sizeof(TRDP_TRAIN_NET_DIR_ENTRY_T);
    memcpy(&appHandle->pTTDB->trnNetDir.trnNetDir[0], pData, size);
    pData += 4 + size;              /* jump to etbTopoCnt  */

    /* unmarshall manually and update the etbTopoCount   */
    appHandle->pTTDB->trnNetDir.etbTopoCnt = vos_ntohl((*(UINT32 *)pData));   /* copy etbTopoCnt as well    */

    /* swap the consist network properties    */
    for (i = 0; i < appHandle->pTTDB->trnNetDir.entryCnt; i++)
    {
        appHandle->pTTDB->trnNetDir.trnNetDir[i].cstNetProp = vos_ntohl(
                appHandle->pTTDB->trnNetDir.trnNetDir[i].cstNetProp);
    }
}

/***********************************************************************************************************************
 * Find an appropriate location to store the received consist info
 */
static void ttiStoreCstInfo (
    TRDP_APP_SESSION_T  appHandle,
    UINT8               *pData,
    UINT32              dataSize)
{
    TRDP_CONSIST_INFO_T *pTelegram = (TRDP_CONSIST_INFO_T *) pData;
    UINT32 curEntry = 0;

    if (ttiIsOwnCstInfo(appHandle, pTelegram) == TRUE)
    {
        if (appHandle->pTTDB->cstInfo[curEntry] != NULL)
        {
            vos_memFree(appHandle->pTTDB->cstInfo[curEntry]);
            appHandle->pTTDB->cstInfo[curEntry] = NULL;
        }
    }
    else
    {
        UINT32 index;
        curEntry = 1;
        /* check if already loaded */
        for (index = 1; index < TRDP_MAX_CST_CNT; index++)
        {
            if (appHandle->pTTDB->cstInfo[index] != NULL &&
                appHandle->pTTDB->cstInfo[index]->cstTopoCnt != 0 &&
                memcmp(appHandle->pTTDB->cstInfo[index]->cstUUID, pTelegram->cstUUID, sizeof(TRDP_UUID_T)) == 0)
            {
                vos_memFree(appHandle->pTTDB->cstInfo[index]);
                appHandle->pTTDB->cstInfo[index] = NULL;
                curEntry = index;
                break;
            }
        }
    }
    appHandle->pTTDB->cstInfo[curEntry] = (TRDP_CONSIST_INFO_T *) vos_memAlloc(dataSize);
    if (appHandle->pTTDB->cstInfo[curEntry] == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Consist info could not be stored!");
        return;
    }

    /* We do not convert but just copy the consist info slot   */

    memcpy(appHandle->pTTDB->cstInfo[curEntry], pTelegram, dataSize);
    appHandle->pTTDB->cstSize[curEntry] = dataSize;
}

/**********************************************************************************************************************/
/**    Function called on reception of message data
 *
 *  Handle and process incoming data, update our data store
 *
 *  @param[in]      pRefCon         unused.
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      pMsg            Pointer to the message info (header etc.)
 *  @param[in]      pData           Pointer to the network buffer.
 *  @param[in]      dataSize        Size of the received data
 *
 *  @retval         none
 *
 */
static void ttiMDCallback (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    VOS_SEMA_T  waitForInaug = (VOS_SEMA_T) pRefCon;

    if (pMsg->comId == TTDB_OP_DIR_INFO_COMID ||      /* TTDB notification */
        pMsg->comId == TTDB_OP_DIR_INFO_REP_COMID)
    {
        if (pMsg->resultCode == TRDP_NO_ERR &&
            dataSize <= sizeof(TRDP_OP_TRAIN_DIR_T))
        {
            ttiStoreOpTrnDir(appHandle, pData);
            if (waitForInaug != NULL)
            {
                vos_semaGive(waitForInaug);           /* Signal new inauguration    */
            }
        }
    }
    else if (pMsg->comId == TTDB_TRN_DIR_REP_COMID)
    {
        if (pMsg->resultCode == TRDP_NO_ERR &&
            dataSize <= sizeof(TRDP_TRAIN_DIR_T))
        {
            UINT32 i;
            ttiStoreTrnDir(appHandle, pData);
            /* Request consist infos now (fill cache)   */
            for (i = 0; i < TTI_CACHED_CONSISTS; i++)
            {
                /* invalidate entry */
                if (appHandle->pTTDB->cstInfo[i] != NULL)
                {
                    vos_memFree(appHandle->pTTDB->cstInfo[i]);
                    appHandle->pTTDB->cstInfo[i] = NULL;
                }
                ;
            }
            for (i = 0; i < TTI_CACHED_CONSISTS; i++)
            {
                if (appHandle->pTTDB->trnDir.cstList[i].cstTopoCnt == 0)
                {
                    break;  /* no of available consists reached   */
                }
                ttiRequestTTDBdata(appHandle, TTDB_STAT_CST_REQ_COMID, appHandle->pTTDB->trnDir.cstList[i].cstUUID);
            }
        }
    }
    else if (pMsg->comId == TTDB_NET_DIR_REP_COMID)
    {
        if (pMsg->resultCode == TRDP_NO_ERR &&
            dataSize <= sizeof(TRDP_TRAIN_NET_DIR_T))
        {
            ttiStoreTrnNetDir(appHandle, pData);
        }
    }
    else if (pMsg->comId == TTDB_READ_CMPLT_REP_COMID)
    {
        if (pMsg->resultCode == TRDP_NO_ERR &&
            dataSize <= sizeof(TRDP_READ_COMPLETE_REPLY_T))
        {
            TRDP_READ_COMPLETE_REPLY_T *pTelegram = (TRDP_READ_COMPLETE_REPLY_T *) pData;
            UINT32 crc;

            /* Handle the op_state  */

            /* check the crc:   */
            crc = vos_crc32(0xFFFFFFFF, (const UINT8 *) &pTelegram->state, dataSize - 4);
            if (crc != MAKE_LE(pTelegram->state.crc))
            {
                vos_printLog(VOS_LOG_ERROR, "CRC error of received operational status info (%08x != %08x)!\n",
                             crc, vos_ntohl(pTelegram->state.crc))
                tlc_setOpTrainTopoCount(appHandle, 0);
                return;
            }
            memcpy(&appHandle->pTTDB->opTrnState, &pTelegram->state, dataSize);

            /* unmarshall manually:   */
            appHandle->pTTDB->opTrnState.state.opTrnTopoCnt = vos_ntohl(pTelegram->state.opTrnTopoCnt);
            tlc_setOpTrainTopoCount(appHandle, appHandle->pTTDB->opTrnState.state.opTrnTopoCnt);
            appHandle->pTTDB->opTrnState.state.crc = vos_ntohl(pTelegram->state.crc);

            /* handle the other parts of the message    */
            ttiStoreOpTrnDir(appHandle, (UINT8 *) &pTelegram->opTrnDir);
            ttiStoreTrnDir(appHandle, (UINT8 *) &pTelegram->trnDir);
            ttiStoreTrnNetDir(appHandle, (UINT8 *) &pTelegram->trnNetDir);
        }
    }
    else if (pMsg->comId == TTDB_STAT_CST_REP_COMID)
    {

        if (pMsg->resultCode == TRDP_NO_ERR &&
            dataSize <= sizeof(TRDP_CONSIST_INFO_T))
        {
            /* find a free place in the cache, or overwrite oldest entry   */
            ttiStoreCstInfo(appHandle, pData, dataSize);
        }
    }
}

/**********************************************************************************************************************/
/**    Function called on reception of process data
 *
 *  Handle and process incoming data, update our data store
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      comID           Communication ID of request
 *  @param[in]      cstUUID         Pointer to the additional info
 *
 *  @retval         none
 *
 */
static void ttiRequestTTDBdata (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              comID,
    const TRDP_UUID_T   cstUUID)
{
    switch (comID)
    {
        case TTDB_OP_DIR_INFO_REQ_COMID:
        {
            UINT8 param = 0;
            tlm_request(appHandle, NULL, ttiMDCallback, NULL, TTDB_OP_DIR_INFO_REQ_COMID, appHandle->etbTopoCnt,
                        appHandle->opTrnTopoCnt, 0, TTDB_OP_DIR_INFO_REQ_IP, TRDP_FLAGS_CALLBACK, 1,
                        TTDB_OP_DIR_INFO_REQ_TO, 0, NULL, &param, sizeof(param), NULL, NULL);
            /* Make sure the request is sent: */
        }
        break;
        case TTDB_TRN_DIR_REQ_COMID:
        {
            UINT8 param = 0;        /* ETB0 */
            tlm_request(appHandle, NULL, ttiMDCallback, NULL, TTDB_TRN_DIR_REQ_COMID, appHandle->etbTopoCnt,
                        appHandle->opTrnTopoCnt, 0, TTDB_TRN_DIR_REQ_IP, TRDP_FLAGS_CALLBACK, 1,
                        TTDB_TRN_DIR_REQ_TO, 0, NULL, &param, sizeof(param), NULL, NULL);
        }
        break;
        case TTDB_NET_DIR_REQ_COMID:
        {
            UINT8 param = 0;        /* ETB0 */
            tlm_request(appHandle, NULL, ttiMDCallback, NULL, TTDB_NET_DIR_REQ_COMID, appHandle->etbTopoCnt,
                        appHandle->opTrnTopoCnt, 0, TTDB_NET_DIR_REQ_IP, TRDP_FLAGS_CALLBACK, 1,
                        TTDB_NET_DIR_REQ_TO, 0, NULL, &param, sizeof(param), NULL, NULL);
        }
        break;
        case TTDB_READ_CMPLT_REQ_COMID:
        {
            UINT8 param = 0;        /* ETB0 */
            tlm_request(appHandle, NULL, ttiMDCallback, NULL, TTDB_READ_CMPLT_REQ_COMID, appHandle->etbTopoCnt,
                        appHandle->opTrnTopoCnt, 0, TTDB_READ_CMPLT_REQ_IP, TRDP_FLAGS_CALLBACK, 1,
                        TTDB_READ_CMPLT_REQ_TO, 0, NULL, &param, sizeof(param), NULL, NULL);
        }
        break;
        case TTDB_STAT_CST_REQ_COMID:
        {
            tlm_request(appHandle, NULL, ttiMDCallback, NULL, TTDB_STAT_CST_REQ_COMID, appHandle->etbTopoCnt,
                        appHandle->opTrnTopoCnt, 0, TTDB_STAT_CST_REQ_IP, TRDP_FLAGS_CALLBACK, 1,
                        TTDB_STAT_CST_REQ_TO, 0, NULL, cstUUID, sizeof(TRDP_UUID_T), NULL, NULL);
        }
        break;

    }
    /* Make sure the request is sent: */
    tlc_process(appHandle, NULL, NULL);
}

#pragma mark ----------------------- Public -----------------------------

/**********************************************************************************************************************/
/*    Train configuration information access                                                                          */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**    Function to init TTI access
 *
 *  Subscribe to necessary process data for correct ECSP handling
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      pUserAction     Semaphore to fire if inauguration took place.
 *  @param[in]      ecspIpAddr      ECSP IP address.
 *  @param[in]      hostFileName    Optional host file name as ECSP replacement.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initTTIaccess (
    TRDP_APP_SESSION_T  appHandle,
    VOS_SEMA_T          userAction,
    TRDP_IP_ADDR_T      ecspIpAddr,
    CHAR8               *hostFileName)
{
    if (appHandle == NULL || appHandle->pTTDB != NULL)
    {
        return TRDP_INIT_ERR;
    }

    appHandle->pTTDB = (TAU_TTDB_T *) vos_memAlloc(sizeof(TAU_TTDB_T));
    if (appHandle->pTTDB == NULL)
    {
        return TRDP_MEM_ERR;
    }

    /*  subscribe to PD 100 */

    if (tlp_subscribe(appHandle,
                      &appHandle->pTTDB->pd100SubHandle,
                      userAction, ttiPDCallback,
                      TRDP_TTDB_OP_TRAIN_DIR_STATUS_INFO_COMID,
                      0, 0,
                      VOS_INADDR_ANY,
                      vos_dottedIP(TTDB_STATUS_DEST_IP_ETB0),
                      TRDP_FLAGS_CALLBACK,
                      TTDB_STATUS_TO,
                      TRDP_TO_SET_TO_ZERO) != TRDP_NO_ERR)
    {
        vos_memFree(appHandle->pTTDB);
        return TRDP_INIT_ERR;
    }

    /*  Listen for MD 101 */

    if (tlm_addListener(appHandle,
                        &appHandle->pTTDB->md101Listener,
                        userAction,
                        ttiMDCallback,
                        TTDB_OP_DIR_INFO_COMID,
                        0,
                        0,
                        TTDB_OP_DIR_INFO_IP_ETB0,
                        TRDP_FLAGS_CALLBACK, NULL) != TRDP_NO_ERR)
    {
        vos_memFree(appHandle->pTTDB);
        return TRDP_INIT_ERR;
    }
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Release any resources allocated by TTI
 *  Must be called before closing the session.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *
 *  @retval         none
 *
 */
EXT_DECL void tau_deInitTTI (
    TRDP_APP_SESSION_T appHandle)
{
    if (appHandle->pTTDB != NULL)
    {
        UINT32 i;
        for (i = 0; i < appHandle->pTTDB->noOfCachedCst; i++)
        {
            if (appHandle->pTTDB->cstInfo[i] != NULL)
            {
                vos_memFree(appHandle->pTTDB->cstInfo[i]);
                appHandle->pTTDB->cstInfo[i] = NULL;
            }
        }
        tlm_delListener(appHandle, appHandle->pTTDB->md101Listener);
        tlp_unsubscribe(appHandle, appHandle->pTTDB->pd100SubHandle);
        vos_memFree(appHandle->pTTDB);
        appHandle->pTTDB = NULL;
    }
}

/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory state.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpTrnDirState  Pointer to an operational train directory state structure to be returned.
 *  @param[out]     pOpTrnDir       Pointer to an operational train directory structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Data currently not available, try again later
 *
 */
EXT_DECL TRDP_ERR_T tau_getOpTrDirectory (
    TRDP_APP_SESSION_T          appHandle,
    TRDP_OP_TRAIN_DIR_STATE_T   *pOpTrnDirState,
    TRDP_OP_TRAIN_DIR_T         *pOpTrnDir)
{
    if (appHandle == NULL ||
        appHandle->pTTDB == NULL)
    {
        return TRDP_PARAM_ERR;
    }
    if (appHandle->pTTDB->opTrnDir.opCstCnt == 0 ||
        appHandle->pTTDB->opTrnDir.opTrnTopoCnt != appHandle->opTrnTopoCnt)     /* need update? */
    {
        ttiRequestTTDBdata(appHandle, TTDB_OP_DIR_INFO_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }
    if (pOpTrnDirState != NULL)
    {
        *pOpTrnDirState = appHandle->pTTDB->opTrnState.state;
    }
    if (pOpTrnDir != NULL)
    {
        *pOpTrnDir = appHandle->pTTDB->opTrnDir;
    }
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTrnDir         Pointer to a train directory structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try later
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrDirectory (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_TRAIN_DIR_T    *pTrnDir)
{
    if (appHandle == NULL ||
        appHandle->pTTDB == NULL ||
        pTrnDir == NULL)
    {
        return TRDP_PARAM_ERR;
    }
    if (appHandle->pTTDB->trnDir.cstCnt == 0 ||
        appHandle->pTTDB->trnDir.trnTopoCnt != appHandle->etbTopoCnt)     /* need update? */
    {
        ttiRequestTTDBdata(appHandle, TTDB_TRN_DIR_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }
    *pTrnDir = appHandle->pTTDB->trnDir;
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstInfo        Pointer to a consist info structure to be returned.
 *  @param[in]      cstUUID         UUID of the consist the consist info is rquested for.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getStaticCstInfo (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_CONSIST_INFO_T *pCstInfo,
    TRDP_UUID_T const   cstUUID)
{
    UINT32 index;
    if (appHandle == NULL ||
        appHandle->pTTDB == NULL ||
        pCstInfo == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (cstUUID == NULL)
    {
        index = 0;
    }
    else
    {
        /* find the consist in our cache list */
        for (index = 0; index < TTI_CACHED_CONSISTS; index++)
        {
            if (appHandle->pTTDB->cstInfo[index] != NULL &&
                memcmp(appHandle->pTTDB->cstInfo[index]->cstUUID, cstUUID, sizeof(TRDP_UUID_T)) == 0)
            {
                break;
            }
        }
    }
    if (index < TTI_CACHED_CONSISTS &&
        appHandle->pTTDB->cstInfo[index] != NULL)
    {
        memcpy(pCstInfo, appHandle->pTTDB->cstInfo[index], appHandle->pTTDB->cstSize[index]);
    }
    else    /* not found, get it and return directly */
    {
        ttiRequestTTDBdata(appHandle, TTDB_STAT_CST_REQ_COMID, cstUUID);
        return TRDP_NODATA_ERR;
    }
    return TRDP_NO_ERR;
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpTrDirState   Pointer to an operational train directory state structure to be returned.
 *  @param[out]     pOpTrDir        Pointer to an operational train directory structure to be returned.
 *  @param[out]     pTrDir          Pointer to a train directory structure to be returned.
 *  @param[out]     pTrNetDir       Pointer to a train network directory structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTTI (
    TRDP_APP_SESSION_T          appHandle,
    TRDP_OP_TRAIN_DIR_STATE_T   *pOpTrDirState,
    TRDP_OP_TRAIN_DIR_T         *pOpTrDir,
    TRDP_TRAIN_DIR_T            *pTrDir,
    TRDP_TRAIN_NET_DIR_T        *pTrNetDir)
{
    if (appHandle == NULL ||
        appHandle->pTTDB == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pOpTrDirState != NULL)
    {
        *pTrNetDir = appHandle->pTTDB->trnNetDir;
    }
    if (pOpTrDir != NULL)
    {
        *pOpTrDir = appHandle->pTTDB->opTrnDir;
    }
    if (pTrDir != NULL)
    {
        *pTrDir = appHandle->pTTDB->trnDir;
    }
    if (pTrNetDir != NULL)
    {
        *pTrNetDir = appHandle->pTTDB->trnNetDir;
    }
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the total number of consists in the train.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTrnCstCnt      Pointer to the number of consists to be returned
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try again
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnCstCnt (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pTrnCstCnt)
{
    if (appHandle == NULL ||
        appHandle->pTTDB == NULL ||
        pTrnCstCnt == NULL)
    {
        return TRDP_PARAM_ERR;
    }
    if (appHandle->pTTDB->trnDir.cstCnt == 0 ||
        appHandle->pTTDB->opTrnState.etbTopoCnt != appHandle->etbTopoCnt)     /* need update? */
    {
        ttiRequestTTDBdata(appHandle, TTDB_TRN_DIR_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }

    *pTrnCstCnt = appHandle->pTTDB->trnDir.cstCnt;
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the total number of vehicles in the train.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTrnVehCnt      Pointer to the number of vehicles to be returned
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try again
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnVehCnt (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pTrnVehCnt)
{
    if (appHandle == NULL ||
        appHandle->pTTDB == NULL ||
        pTrnVehCnt == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (appHandle->pTTDB->trnDir.cstCnt == 0 ||
        appHandle->pTTDB->opTrnState.etbTopoCnt != appHandle->etbTopoCnt)     /* need update? */
    {
        ttiRequestTTDBdata(appHandle, TTDB_TRN_DIR_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }

    *pTrnVehCnt = appHandle->pTTDB->opTrnDir.opVehCnt;
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the total number of vehicles in a consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstVehCnt      Pointer to the number of vehicles to be returned
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try again
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstVehCnt (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pCstVehCnt,
    const TRDP_LABEL_T  pCstLabel)
{
    UINT32 index;
    if (appHandle == NULL ||
        appHandle->pTTDB == NULL ||
        pCstVehCnt == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pCstLabel == NULL)
    {
        index = 0;
    }
    else
    {
        /* find the consist in our cache list */
        for (index = 0; index < TTI_CACHED_CONSISTS; index++)
        {
            if (appHandle->pTTDB->cstInfo[index] != NULL &&
                vos_strnicmp(appHandle->pTTDB->cstInfo[index]->cstId, pCstLabel, sizeof(TRDP_LABEL_T)) == 0)
            {
                break;
            }
        }
    }
    if (index < TTI_CACHED_CONSISTS)
    {
        *pCstVehCnt = appHandle->pTTDB->cstInfo[index]->vehCnt;
    }
    else    /* not found, get it and return directly */
    {
        TRDP_UUID_T cstUUID;
        ttiGetUUIDfromLabel(appHandle, cstUUID, pCstLabel);
        ttiRequestTTDBdata(appHandle, TTDB_STAT_CST_REQ_COMID, cstUUID);
        return TRDP_NODATA_ERR;
    }
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the total number of functions in a consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstFctCnt      Pointer to the number of functions to be returned
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstFctCnt (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pCstFctCnt,
    const TRDP_LABEL_T  cstLabel)
{
    return TRDP_NO_ERR;
}


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to retrieve the function information of the consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pFctInfo        Pointer to function info list to be returned.
 *                                  Memory needs to be provided by application. Set NULL if not used.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *  @param[in]      maxFctCnt       Maximal number of functions to be returned in provided buffer.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstFctInfo (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_FUNCTION_INFO_T    *pFctInfo,
    const TRDP_LABEL_T      cstLabel,
    UINT16                  maxFctCnt)
{
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the vehicle information of a consist's vehicle.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pVehInfo        Pointer to the vehicle info to be returned.
 *  @param[in]      vehLabel        Pointer to a vehicle label. NULL means own vehicle  if cstLabel refers to own consist.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getVehInfo (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_VEHICLE_INFO_T *pVehInfo,
    const TRDP_LABEL_T  vehLabel,
    const TRDP_LABEL_T  cstLabel)
{
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the consist information of a train's consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstInfo        Pointer to the consist info to be returned.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstInfo (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_CONSIST_INFO_T *pCstInfo,
    const TRDP_LABEL_T  cstLabel)
{
    return TRDP_NO_ERR;
}


/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**    Function to retrieve the orientation of the given vehicle.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pVehOrient      Pointer to the vehicle orientation to be returned
 *                                   '00'B = not known (corrected vehicle)
 *                                   '01'B = same as operational train direction
 *                                   '10'B = inverse to operational train direction
 *  @param[out]     pCstOrient      Pointer to the consist orientation to be returned
 *                                   '00'B = not known (corrected vehicle)
 *                                   '01'B = same as operational train direction
 *                                   '10'B = inverse to operational train direction
 *  @param[in]      vehLabel        vehLabel = NULL means own vehicle if cstLabel == NULL
 *  @param[in]      cstLabel        cstLabel = NULL means own consist
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getVehOrient (
    TRDP_APP_SESSION_T  appHandle,
    UINT8               *pVehOrient,
    UINT8               *pCstOrient,
    TRDP_LABEL_T        vehLabel,
    TRDP_LABEL_T        cstLabel)
{
    return TRDP_NO_ERR;
}
