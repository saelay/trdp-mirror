/******************************************************************************/
/**
 * @file            tau_xml.c
 *
 * @brief           Functions for XML file parsing
 *
 * @details         SOX parsing of XML configuration file
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          B. Loehr, NewTec GmbH, Tomas Svoboda, UniContorls a.s.
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright NewTec GmbH, 2016. All rights reserved.
 *
 * $Id$
 *
 *      BL 2016-03-04: Ticket #113: parsing of dataset element "type" always returns 0
 *      BL 2016-02-11: Ticket #111: unit, scale, offset added
 *      BL 2016-02-11: Ticket #102: Replacing libxml2
 *      BL 2016-01-25: Ticket #106: Callback can be ON, OFF, ALWAYS
 *
 */

/*******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "trdp_types.h"
#include "trdp_utils.h"
#include "tau_xml.h"
#include "trdp_xml.h"

/*******************************************************************************
 * DEFINES
 */

/*  Default SDT values  */
#ifndef TRDP_SDT_DEFAULT_SMI2
#define TRDP_SDT_DEFAULT_SMI2  0                                        /**< Default SDT safe message identifier    */
#endif
#ifndef TRDP_SDT_DEFAULT_NRXSAFE
#define TRDP_SDT_DEFAULT_NRXSAFE  3                                     /**< Default SDT timeout cycles             */
#endif
#ifndef TRDP_SDT_DEFAULT_NGUARD
#define TRDP_SDT_DEFAULT_NGUARD  100                                    /**< Default SDT initial timeout cycles     */
#endif
#ifndef TRDP_SDT_DEFAULT_CMTHR
#define TRDP_SDT_DEFAULT_CMTHR  10                                      /**< Default SDT chan. monitoring threshold */
#endif

/*******************************************************************************
 * TYPEDEFS
 */


/******************************************************************************
 *   Locals
 */

/*
 * Get type value
 */
TRDP_DATA_TYPE_T string2type (const CHAR8 *pTypeStr)
{
    CHAR8       *p;
    const CHAR8 tokenList[] =
        "01 BITSET8 01 BOOL8 01 ANTIVALENT8 02 CHAR8 02 UTF8 03 UTF16 04 INT8 05 INT16 06 INT32 07 INT64 08 UINT8"
        " 09 UINT16 10 UINT32 11 UINT64 12 REAL32 13 REAL64 14 TIMEDATE32 15 TIMEDATE48 16 TIMEDATE64";

    p = strstr(tokenList, pTypeStr);
    if (p != NULL)
    {
        return (TRDP_DATA_TYPE_T) strtol(p - 3, NULL, 10);
    }
    return TRDP_INVALID;
}


/*
 * Set default values to device parameters
 */
static void setDefaultDeviceValues (
    TRDP_MEM_CONFIG_T   *pMemConfig,
    TRDP_DBG_CONFIG_T   *pDbgConfig,
    UINT32              *pNumComPar,
    TRDP_COM_PAR_T      * *ppComPar,
    UINT32              *pNumIfConfig,
    TRDP_IF_CONFIG_T    * *ppIfConfig
    )
{
    /*  Memory configuration    */
    if (pMemConfig)
    {
        UINT32 defaultPrealloc[VOS_MEM_NBLOCKSIZES] = VOS_MEM_PREALLOCATE;
        pMemConfig->size    = 0;
        pMemConfig->p       = NULL;
        memcpy(pMemConfig->prealloc, defaultPrealloc, sizeof(defaultPrealloc));
    }
    /*  Default debug parameters*/
    if (pDbgConfig)
    {
        memset(pDbgConfig->fileName, 0, sizeof(TRDP_FILE_NAME_T));
        pDbgConfig->maxFileSize = TRDP_DEBUG_DEFAULT_FILE_SIZE;
        pDbgConfig->option      = TRDP_DBG_ERR;
    }

    /*  No Communication parameters*/
    if (pNumComPar)
    {
        *pNumComPar = 0;
    }
    if (ppComPar)
    {
        *ppComPar = NULL;
    }

    /*  No Interface configurations*/
    if (pNumIfConfig)
    {
        *pNumIfConfig = 0;
    }
    if (ppIfConfig)
    {
        *ppIfConfig = NULL;
    }
}

/*
* Set default values to interface (session) parameters
*/
static void setDefaultInterfaceValues (
    TRDP_PROCESS_CONFIG_T   *pProcessConfig,
    TRDP_PD_CONFIG_T        *pPdConfig,
    TRDP_MD_CONFIG_T        *pMdConfig
    )
{
    /*  Process configuration   */
    if (pProcessConfig)
    {
        memset(pProcessConfig->hostName, 0, sizeof(TRDP_LABEL_T));
        memset(pProcessConfig->leaderName, 0, sizeof(TRDP_LABEL_T));
        pProcessConfig->cycleTime   = TRDP_PROCESS_DEFAULT_CYCLE_TIME;
        pProcessConfig->options     = TRDP_PROCESS_DEFAULT_OPTIONS;
        pProcessConfig->priority    = TRDP_PROCESS_DEFAULT_PRIORITY;
    }

    /*  Default Pd configuration    */
    if (pPdConfig)
    {
        pPdConfig->pfCbFunction     = NULL;
        pPdConfig->pRefCon          = NULL;
        pPdConfig->flags            = TRDP_FLAGS_NONE;
        pPdConfig->port             = TRDP_PD_UDP_PORT;
        pPdConfig->sendParam.qos    = TRDP_PD_DEFAULT_QOS;
        pPdConfig->sendParam.ttl    = TRDP_PD_DEFAULT_TTL;
#ifdef TRDP_RETRIES
        pPdConfig->sendParam.retries = 0;
#endif
        pPdConfig->timeout      = TRDP_PD_DEFAULT_TIMEOUT;
        pPdConfig->toBehavior   = TRDP_TO_SET_TO_ZERO;
    }

    /*  Default Md configuration    */
    if (pMdConfig)
    {
        pMdConfig->pfCbFunction     = NULL;
        pMdConfig->pRefCon          = NULL;
        pMdConfig->confirmTimeout   = TRDP_MD_DEFAULT_CONFIRM_TIMEOUT;
        pMdConfig->connectTimeout   = TRDP_MD_DEFAULT_CONNECTION_TIMEOUT;
        pMdConfig->flags            = TRDP_FLAGS_NONE;
        pMdConfig->replyTimeout     = TRDP_MD_DEFAULT_REPLY_TIMEOUT;
        pMdConfig->sendParam.qos    = TRDP_MD_DEFAULT_QOS;
#ifdef TRDP_RETRIES
        pMdConfig->sendParam.retries = TRDP_MD_DEFAULT_RETRIES;
#endif
        pMdConfig->sendParam.ttl    = TRDP_MD_DEFAULT_TTL;
        pMdConfig->tcpPort          = TRDP_MD_TCP_PORT;
        pMdConfig->udpPort          = TRDP_MD_UDP_PORT;
        pMdConfig->maxNumSessions   = TRDP_MD_MAX_NUM_SESSIONS;
    }
}

void dbgPrint (UINT32 num, TRDP_EXCHG_PAR_T *pArray)
{
    UINT32 i, j;

    printf( "---\nExchange parameters (ComId / parId / dataSetId / type:\n");
    for (i = 0; i < num; ++i)
    {
        printf( "%u:  %u / %u / %u / %u\n", i,
                pArray[i].comId,
                pArray[i].comParId,
                pArray[i].datasetId,
                pArray[i].type);
        if (pArray[i].pMdPar != NULL)
        {
            printf( "MD flags: %u reply timeout: %u\n",
                    pArray[i].pMdPar->flags,
                    pArray[i].pMdPar->replyTimeout);
        }

        if (pArray[i].pPdPar != NULL)
        {
            printf( "PD flags: %u cycle: %u timeout: %u\n",
                    pArray[i].pPdPar->flags,
                    pArray[i].pPdPar->cycle,
                    pArray[i].pPdPar->timeout);
        }
        for (j = 0; j < pArray[i].destCnt; ++j)
        {
            printf( "Dest Id: %u URI: %s\n",
                    pArray[i].pDest[j].id,
                    *pArray[i].pDest[j].pUriHost);
        }
        for (j = 0; j < pArray[i].srcCnt; ++j)
        {
            printf( "Src Id: %u URI1: %s URI2: %s\n",
                    pArray[i].pSrc[j].id,
                    *pArray[i].pSrc[j].pUriHost1,
                    (*pArray[i].pSrc[j].pUriHost2 != NULL) ? *pArray[i].pSrc[j].pUriHost2 : "---");
        }
    }
    printf( "---------------------------------\n");
}

/**********************************************************************************************************************/
TRDP_ERR_T readTelegramDef (
    XML_HANDLE_T        *pXML,
    TRDP_EXCHG_PAR_T    *pExchgParam)
{
    CHAR8       tag[MAX_TAG_LEN];
    CHAR8       attribute[MAX_TOK_LEN];
    CHAR8       value[MAX_TOK_LEN];
    UINT32      valueInt;
    UINT32      countSrc, countDst;
    TRDP_SRC_T  *pSrc;
    TRDP_DEST_T *pDest;

    /* Get the attributes */

    while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE)
    {
        if (vos_strnicmp(attribute, "com-id", MAX_TOK_LEN) == 0)
        {
            pExchgParam->comId = valueInt;
        }
        else if (vos_strnicmp(attribute, "data-set-id", MAX_TOK_LEN) == 0)
        {
            pExchgParam->datasetId = valueInt;
        }
        else if (vos_strnicmp(attribute, "com-parameter-id", MAX_TOK_LEN) == 0)
        {
            pExchgParam->comParId = valueInt;
        }
        else if (vos_strnicmp(attribute, "type", MAX_TOK_LEN) == 0)
        {
            if (vos_strnicmp("sink", value, TRDP_MAX_LABEL_LEN) == 0)
            {
                pExchgParam->type = TRDP_EXCHG_SINK;
            }
            else if (vos_strnicmp("source", value, TRDP_MAX_LABEL_LEN) == 0)
            {
                pExchgParam->type = TRDP_EXCHG_SOURCE;
            }
            else if (vos_strnicmp("source-sink", value, TRDP_MAX_LABEL_LEN) == 0)
            {
                pExchgParam->type = TRDP_EXCHG_SOURCESINK;
            }
        }
        else if (vos_strnicmp(attribute, "create", MAX_TOK_LEN) == 0)
        {
            if (vos_strnicmp("on", value, TRDP_MAX_LABEL_LEN) == 0)
            {
                pExchgParam->create = TRUE;
            }
        }
    }

    if (pExchgParam->comId == 10004)
    {
        printf("10004");
    }

    /* find out how many sources are defined before hand */
    countSrc    = trdp_XMLCountStartTag(pXML, "source");
    pSrc        = NULL;
    countDst    = trdp_XMLCountStartTag(pXML, "destination");
    pDest       = NULL;

    /* Iterate thru <telegram> */

    while (trdp_XMLSeekStartTagAny(pXML, tag, MAX_TAG_LEN) == 0)
    {
        if (vos_strnicmp(tag, "md-parameter", MAX_TAG_LEN) == 0)
        {
            pExchgParam->pMdPar = (TRDP_MD_PAR_T *) vos_memAlloc(sizeof(TRDP_MD_PAR_T));

            if (pExchgParam->pMdPar != NULL)
            {
                while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE)
                {
                    if (vos_strnicmp(attribute, "reply-timeout", MAX_TOK_LEN) == 0)
                    {
                        pExchgParam->pMdPar->replyTimeout = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "confirm-timeout", MAX_TOK_LEN) == 0)
                    {
                        pExchgParam->pMdPar->confirmTimeout = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "marshall", MAX_TOK_LEN) == 0)
                    {
                        if (vos_strnicmp("on", value, TRDP_MAX_LABEL_LEN) == 0)
                        {
                            pExchgParam->pMdPar->flags  |= TRDP_FLAGS_MARSHALL;
                            pExchgParam->pMdPar->flags  &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                        }
                    }
                    else if (vos_strnicmp(attribute, "callback", MAX_TOK_LEN) == 0)
                    {
                        if (vos_strnicmp("on", value, TRDP_MAX_LABEL_LEN) == 0)
                        {
                            pExchgParam->pMdPar->flags  |= TRDP_FLAGS_CALLBACK;
                            pExchgParam->pMdPar->flags  &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                        }
                        else if (vos_strnicmp("always", value, TRDP_MAX_LABEL_LEN) == 0)
                        {
                            pExchgParam->pMdPar->flags  |= TRDP_FLAGS_FORCE_CB;
                            pExchgParam->pMdPar->flags  &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                        }
                    }
                    else if (vos_strnicmp(attribute, "protocol", MAX_TOK_LEN) == 0)
                    {
                        if (vos_strnicmp("TCP", value, TRDP_MAX_LABEL_LEN) == 0)
                        {
                            pExchgParam->pMdPar->flags |= TRDP_FLAGS_TCP;
                        }
                        else
                        {
                            pExchgParam->pMdPar->flags &= (TRDP_FLAGS_T) ~TRDP_FLAGS_TCP;
                        }
                    }
                }
            }
            else
            {
                return TRDP_MEM_ERR;
            }
        }
        else if (vos_strnicmp(tag, "pd-parameter", MAX_TAG_LEN) == 0)
        {
            pExchgParam->pPdPar = (TRDP_PD_PAR_T *) vos_memAlloc(sizeof(TRDP_PD_PAR_T));

            if (pExchgParam->pPdPar != NULL)
            {
                while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE)
                {
                    if (vos_strnicmp(attribute, "cycle", MAX_TOK_LEN) == 0)
                    {
                        pExchgParam->pPdPar->cycle = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "timeout", MAX_TOK_LEN) == 0)
                    {
                        pExchgParam->pPdPar->timeout = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "marshall", MAX_TOK_LEN) == 0)
                    {
                        if (vos_strnicmp("on", value, TRDP_MAX_LABEL_LEN) == 0)
                        {
                            pExchgParam->pPdPar->flags  |= TRDP_FLAGS_MARSHALL;
                            pExchgParam->pPdPar->flags  &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                        }
                    }
                    else if (vos_strnicmp(attribute, "callback", MAX_TOK_LEN) == 0)
                    {
                        if (vos_strnicmp("on", value, TRDP_MAX_LABEL_LEN) == 0)
                        {
                            pExchgParam->pPdPar->flags  |= TRDP_FLAGS_CALLBACK;
                            pExchgParam->pPdPar->flags  &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                        }
                        else if (vos_strnicmp("always", value, TRDP_MAX_LABEL_LEN) == 0)
                        {
                            pExchgParam->pPdPar->flags  |= TRDP_FLAGS_FORCE_CB;
                            pExchgParam->pPdPar->flags  &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                        }
                    }
                    else if (vos_strnicmp(attribute, "redundant", MAX_TOK_LEN) == 0)
                    {
                        pExchgParam->pPdPar->redundant = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "validity-behavior", MAX_TOK_LEN) == 0)
                    {
                        if (vos_strnicmp("keep", value, TRDP_MAX_LABEL_LEN) == 0)
                        {
                            pExchgParam->pPdPar->toBehav |= TRDP_TO_KEEP_LAST_VALUE;
                        }
                        else
                        {
                            pExchgParam->pPdPar->toBehav |= TRDP_TO_SET_TO_ZERO;
                        }
                    }
                    else if (vos_strnicmp(attribute, "offset-address", MAX_TOK_LEN) == 0)
                    {
                        pExchgParam->pPdPar->offset = valueInt;
                    }
                }
            }
        }
        else if (vos_strnicmp(tag, "source", MAX_TAG_LEN) == 0)
        {
            if (countSrc > 0)
            {
                pExchgParam->pSrc = (TRDP_SRC_T *)vos_memAlloc(countSrc * sizeof(TRDP_SRC_T));

                if (pExchgParam->pSrc == NULL)
                {
                    vos_printLog(VOS_LOG_ERROR, "%lu Bytes failed to allocate while reading XML source definitions!\n",
                                 countSrc * sizeof(TRDP_SRC_T));
                    return TRDP_MEM_ERR;
                }
                pExchgParam->srcCnt = countSrc;
                countSrc    = 0;
                pSrc        = pExchgParam->pSrc;
            }

            while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE && pSrc != NULL)
            {
                if (vos_strnicmp(attribute, "id", MAX_TOK_LEN) == 0)
                {
                    pSrc->id = valueInt;
                }
                else if (vos_strnicmp(attribute, "uri1", MAX_TOK_LEN) == 0)
                {
                    char *p = strchr(value, '@');   /* Get host part only */
                    if (p != NULL)
                    {
                        pSrc->pUriUser = (TRDP_URI_USER_T *) vos_memAlloc(TRDP_MAX_URI_USER_LEN + 1);
                        if (pSrc->pUriUser == NULL)
                        {
                            vos_printLog(VOS_LOG_ERROR,
                                         "%u Bytes failed to allocate while reading XML source definitions!\n",
                                         TRDP_MAX_URI_USER_LEN + 1);
                            return TRDP_MEM_ERR;
                        }
                        memcpy(pSrc->pUriUser, p, p - value);  /* Trailing zero by vos_memAlloc    */
                        p++;
                    }
                    else
                    {
                        p = value;
                    }

                    pSrc->pUriHost1 = (TRDP_URI_HOST_T *) vos_memAlloc(strlen(p) + 1);
                    if (pSrc->pUriHost1 == NULL)
                    {
                        vos_printLog(VOS_LOG_ERROR,
                                     "%lu Bytes failed to allocate while reading XML source definitions!\n",
                                     strlen(p) + 1);
                        return TRDP_MEM_ERR;
                    }
                    vos_strncpy((char *)pSrc->pUriHost1, p, TRDP_MAX_URI_HOST_LEN - 1);
                }
                else if (vos_strnicmp(attribute, "uri2", MAX_TOK_LEN) == 0)
                {
                    char *p = strchr(value, '@');   /* Get host part only */
                    p = (p == NULL) ? value : p + 1;

                    pSrc->pUriHost2 = (TRDP_URI_HOST_T *) vos_memAlloc(strlen(p) + 1);
                    if (pSrc->pUriHost2 == NULL)
                    {
                        vos_printLog(VOS_LOG_ERROR,
                                     "%lu Bytes failed to allocate while reading XML source definitions!\n",
                                     strlen(p) + 1);
                        return TRDP_MEM_ERR;
                    }
                    vos_strncpy((char *)pSrc->pUriHost2, p, TRDP_MAX_URI_HOST_LEN - 1);
                }
            }
            trdp_XMLEnter(pXML);
            if (trdp_XMLCountStartTag(pXML, "sdt-parameter") > 0 &&
                trdp_XMLSeekStartTag(pXML, "sdt-parameter") == 0 &&
                pSrc != NULL)
            {
                pSrc->pSdtPar = (TRDP_SDT_PAR_T *)vos_memAlloc(sizeof(TRDP_SDT_PAR_T));

                if (pSrc->pSdtPar == NULL)
                {
                    vos_printLog(VOS_LOG_ERROR, "%lu Bytes failed to allocate while reading XML source definitions!\n",
                                 sizeof(TRDP_SDT_PAR_T));
                    return TRDP_MEM_ERR;
                }

                while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE)
                {
                    if (vos_strnicmp(attribute, "smi1", MAX_TOK_LEN) == 0)
                    {
                        pSrc->pSdtPar->smi1 = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "smi2", MAX_TOK_LEN) == 0)
                    {
                        pSrc->pSdtPar->smi2 = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "udv", MAX_TOK_LEN) == 0)
                    {
                        pSrc->pSdtPar->udv = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "rx-period", MAX_TOK_LEN) == 0)
                    {
                        pSrc->pSdtPar->rxPeriod = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "tx-period", MAX_TOK_LEN) == 0)
                    {
                        pSrc->pSdtPar->txPeriod = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "n-rxsafe", MAX_TOK_LEN) == 0)
                    {
                        pSrc->pSdtPar->nrxSafe = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "n-guard", MAX_TOK_LEN) == 0)
                    {
                        pSrc->pSdtPar->nGuard = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "cm-thr", MAX_TOK_LEN) == 0)
                    {
                        pSrc->pSdtPar->cmThr = valueInt;
                    }
                }
            }
            trdp_XMLLeave(pXML);
            if (pSrc != NULL)
            {
                pSrc++;
            }
        }
        else if (vos_strnicmp(tag, "destination", MAX_TAG_LEN) == 0)
        {
            if (countDst > 0)
            {
                pExchgParam->pDest = (TRDP_DEST_T *)vos_memAlloc(countDst * sizeof(TRDP_DEST_T));

                if (pExchgParam->pDest == NULL)
                {
                    vos_printLog(VOS_LOG_ERROR, "%lu Bytes failed to allocate while reading XML source definitions!\n",
                                 countDst * sizeof(TRDP_DEST_T));
                    return TRDP_MEM_ERR;
                }
                pExchgParam->destCnt = countDst;
                countDst    = 0;
                pDest       = pExchgParam->pDest;
            }

            while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE && pDest != NULL)
            {
                if (vos_strnicmp(attribute, "id", MAX_TOK_LEN) == 0)
                {
                    pDest->id = valueInt;
                }
                else if (vos_strnicmp(attribute, "uri", MAX_TOK_LEN) == 0)
                {
                    char *p = strchr(value, '@');   /* Get host part only */
                    if (p != NULL)
                    {
                        pDest->pUriUser = (TRDP_URI_USER_T *) vos_memAlloc(TRDP_MAX_URI_USER_LEN + 1);
                        if (pDest->pUriUser == NULL)
                        {
                            vos_printLog(VOS_LOG_ERROR,
                                         "%u Bytes failed to allocate while reading XML source definitions!\n",
                                         TRDP_MAX_URI_USER_LEN + 1);
                            return TRDP_MEM_ERR;
                        }
                        memcpy(pDest->pUriUser, p, p - value);  /* Trailing zero by vos_memAlloc    */
                        p++;
                    }
                    else
                    {
                        p = value;
                    }

                    pDest->pUriHost = (TRDP_URI_HOST_T *) vos_memAlloc(strlen(p) + 1);
                    if (pDest->pUriHost == NULL)
                    {
                        vos_printLog(VOS_LOG_ERROR,
                                     "%lu Bytes failed to allocate while reading XML source definitions!\n",
                                     strlen(p) + 1);
                        return TRDP_MEM_ERR;
                    }
                    vos_strncpy((char *)pDest->pUriHost, p, TRDP_MAX_URI_HOST_LEN - 1);
                }
            }
            trdp_XMLEnter(pXML);
            if (trdp_XMLCountStartTag(pXML, "sdt-parameter") > 0 &&
                trdp_XMLSeekStartTag(pXML, "sdt-parameter") == 0 &&
                pDest != NULL)
            {
                pDest->pSdtPar = (TRDP_SDT_PAR_T *)vos_memAlloc(sizeof(TRDP_SDT_PAR_T));

                if (pDest->pSdtPar == NULL)
                {
                    vos_printLog(VOS_LOG_ERROR, "%lu Bytes failed to allocate while reading XML source definitions!\n",
                                 sizeof(TRDP_SDT_PAR_T));
                    return TRDP_MEM_ERR;
                }

                while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE)
                {
                    if (vos_strnicmp(attribute, "smi1", MAX_TOK_LEN) == 0)
                    {
                        pDest->pSdtPar->smi1 = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "smi2", MAX_TOK_LEN) == 0)
                    {
                        pDest->pSdtPar->smi2 = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "udv", MAX_TOK_LEN) == 0)
                    {
                        pDest->pSdtPar->udv = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "rx-period", MAX_TOK_LEN) == 0)
                    {
                        pDest->pSdtPar->rxPeriod = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "tx-period", MAX_TOK_LEN) == 0)
                    {
                        pDest->pSdtPar->txPeriod = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "n-rxsafe", MAX_TOK_LEN) == 0)
                    {
                        pDest->pSdtPar->nrxSafe = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "n-guard", MAX_TOK_LEN) == 0)
                    {
                        pDest->pSdtPar->nGuard = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "cm-thr", MAX_TOK_LEN) == 0)
                    {
                        pDest->pSdtPar->cmThr = valueInt;
                    }
                }
            }
            trdp_XMLLeave(pXML);
            if (pDest != NULL)
            {
                pDest++;
            }
        }
    }
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
TRDP_ERR_T readXmlDatasetMap (
    XML_HANDLE_T            *pXML,
    UINT32                  *pNumComId,
    TRDP_COMID_DSID_MAP_T   * *ppComIdDsIdMap)
{
    /* CHAR8   tag[MAX_TAG_LEN]; */
    CHAR8   attribute[MAX_TOK_LEN];
    CHAR8   value[MAX_TOK_LEN];
    UINT32  valueInt;
    UINT32  count = 0, idx = 0;

    trdp_XMLRewind(pXML);

    trdp_XMLEnter(pXML);

    if (trdp_XMLSeekStartTag(pXML, "device") == 0) /* Optional */
    {
        trdp_XMLEnter(pXML);

        if (trdp_XMLSeekStartTag(pXML, "bus-interface-list") == 0)
        {
            trdp_XMLEnter(pXML);

            while (trdp_XMLSeekStartTag(pXML, "bus-interface") == 0)
            {
                trdp_XMLEnter(pXML);
                count += trdp_XMLCountStartTag(pXML, "telegram");
                trdp_XMLLeave(pXML);
            }
            trdp_XMLLeave(pXML);
        }

        if (count > 0)
        {
            *ppComIdDsIdMap = (TRDP_COMID_DSID_MAP_T *) vos_memAlloc(count * sizeof(TRDP_COMID_DSID_MAP_T));
            if (*ppComIdDsIdMap == NULL)
            {
                vos_printLog(VOS_LOG_ERROR, "%lu Bytes failed to allocate while creating XML dataset map!\n",
                             count * sizeof(TRDP_COMID_DSID_MAP_T));
                return TRDP_MEM_ERR;
            }
            *pNumComId = count;
        }
    }

    trdp_XMLLeave(pXML);

    trdp_XMLRewind(pXML);

    trdp_XMLEnter(pXML);

    if (trdp_XMLSeekStartTag(pXML, "device") == 0) /* Optional */
    {
        trdp_XMLEnter(pXML);

        if (trdp_XMLSeekStartTag(pXML, "bus-interface-list") == 0)
        {
            trdp_XMLEnter(pXML);

            while (trdp_XMLSeekStartTag(pXML, "bus-interface") == 0)
            {
                trdp_XMLEnter(pXML);
                while (trdp_XMLSeekStartTag(pXML, "telegram") == 0)
                {
                    while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE)
                    {
                        if (vos_strnicmp(attribute, "com-id", MAX_TOK_LEN) == 0)
                        {
                            (*ppComIdDsIdMap)[idx].comId = valueInt;
                        }
                        else if (vos_strnicmp(attribute, "data-set-id", MAX_TOK_LEN) == 0)
                        {
                            (*ppComIdDsIdMap)[idx].datasetId = valueInt;
                        }
                    }
                    idx++;
                }
                trdp_XMLLeave(pXML);
            }
            trdp_XMLLeave(pXML);
        }
    }
    trdp_XMLLeave(pXML);
    return TRDP_NO_ERR;

}

/**********************************************************************************************************************/
TRDP_ERR_T readXmlDatasets (
    XML_HANDLE_T        *pXML,
    UINT32              *pNumDataset,
    papTRDP_DATASET_T   papDataset)
{
    /* CHAR8   tag[MAX_TAG_LEN]; */
    CHAR8   attribute[MAX_TOK_LEN];
    CHAR8   value[MAX_TOK_LEN];
    UINT32  valueInt;

    trdp_XMLRewind(pXML);

    trdp_XMLEnter(pXML);

    if (trdp_XMLSeekStartTag(pXML, "device") == 0) /* Optional */
    {
        trdp_XMLEnter(pXML);

        if (trdp_XMLSeekStartTag(pXML, "data-set-list") == 0)
        {
            UINT32  count = 0;
            UINT32  idx;

            trdp_XMLEnter(pXML);

            count = trdp_XMLCountStartTag(pXML, "data-set");

            /* Allocate an array of pointers */
            *papDataset = (apTRDP_DATASET_T) vos_memAlloc(count * sizeof(apTRDP_DATASET_T));

            if (papDataset == NULL)
            {
                vos_printLog(VOS_LOG_ERROR, "%lu Bytes failed to allocate while reading XML telegram definitions!\n",
                             count * sizeof(apTRDP_DATASET_T));
                return TRDP_MEM_ERR;
            }

            *pNumDataset = count;

            /* Read the interface params */
            for (idx = 0; idx < *pNumDataset && trdp_XMLSeekStartTag(pXML, "data-set") == 0; idx++)
            {
                UINT32 i = 0;
                trdp_XMLEnter(pXML);
                count = trdp_XMLCountStartTag(pXML, "element");

                /* Allocate the dataset element */
                (*papDataset)[idx] =
                    (TRDP_DATASET_T *)vos_memAlloc(count * sizeof(TRDP_DATASET_ELEMENT_T) + sizeof(TRDP_DATASET_T));

                if ((*papDataset)[idx] == NULL)
                {
                    vos_printLog(VOS_LOG_ERROR,
                                 "%lu Bytes failed to allocate while reading XML telegram definitions!\n",
                                 count * sizeof(TRDP_DATASET_ELEMENT_T) + sizeof(TRDP_DATASET_T));
                    return TRDP_MEM_ERR;
                }

                while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE)
                {
                    if (vos_strnicmp(attribute, "id", MAX_TOK_LEN) == 0)
                    {
                        (*papDataset)[idx]->id = valueInt;
                    }
                }

                while (trdp_XMLSeekStartTag(pXML, "element") == 0)
                {
                    (*papDataset)[idx]->pElement[i].size = 1;   /* default  */
                    while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE)
                    {
                        if (vos_strnicmp(attribute, "type", MAX_TOK_LEN) == 0)
                        {
                            if (valueInt == 0)
                            {
                                (*papDataset)[idx]->pElement[i].type = string2type(value);
                            }
                            else
                            {
                                (*papDataset)[idx]->pElement[i].type = valueInt;
                            }
                        }
                        else if (vos_strnicmp(attribute, "array-size", MAX_TOK_LEN) == 0)
                        {
                            (*papDataset)[idx]->pElement[i].size = valueInt;
                        }
                        else if (vos_strnicmp(attribute, "unit", MAX_TOK_LEN) == 0)
                        {
                            (*papDataset)[idx]->pElement[i].unit = (CHAR8 *) vos_memAlloc(strlen(value));
                            if ((*papDataset)[idx]->pElement[i].unit == NULL)
                            {
                                return TRDP_MEM_ERR;
                            }
                            vos_strncpy((*papDataset)[idx]->pElement[i].unit, value, strlen(value));
                        }
                        else if (vos_strnicmp(attribute, "scale", MAX_TOK_LEN) == 0)
                        {
                            (*papDataset)[idx]->pElement[i].scale = (REAL32) strtod(value, NULL);
                        }
                        else if (vos_strnicmp(attribute, "offset", MAX_TOK_LEN) == 0)
                        {
                            (*papDataset)[idx]->pElement[i].offset = valueInt;
                        }
                    }
                    (*papDataset)[idx]->numElement++;
                    i++;
                }
                trdp_XMLLeave(pXML);
            }
        }
        trdp_XMLLeave(pXML);
    }
    trdp_XMLLeave(pXML);
    return TRDP_NO_ERR;
}

/******************************************************************************
 *   Globals
 */

/**********************************************************************************************************************/
/**    Open XML file, prepare XPath context.
 *
 *
 *  @param[in]      pFileName         Path and filename of the xml configuration file
 *  @param[out]     pDocHnd           Handle of the parsed XML file
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_PARAM_ERR    File does not exist
 *
 */
EXT_DECL TRDP_ERR_T tau_prepareXmlDoc (
    const CHAR8             *pFileName,
    TRDP_XML_DOC_HANDLE_T   *pDocHnd
    )
{
    /*  Check file name */
    if (!pFileName || strlen(pFileName) == 0)
    {
        return TRDP_PARAM_ERR;
    }

    /* Check parameters */
    if (!pDocHnd)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Set handle pointers to NULL */
    memset(pDocHnd, 0, sizeof(TRDP_XML_DOC_HANDLE_T));

    pDocHnd->pXmlDocument = (XML_HANDLE_T *) vos_memAlloc(sizeof(XML_HANDLE_T));
    if (pDocHnd->pXmlDocument == NULL)
    {
        return TRDP_MEM_ERR;
    }

    /*  Read XML file  */
    if (trdp_XMLOpen(pDocHnd->pXmlDocument, pFileName))
    {
        vos_printLog(VOS_LOG_ERROR, "Prepare XML doc: failed to open XML file\n");
        return TRDP_PARAM_ERR;
    }

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Free all the memory allocated by tau_prepareXmlDoc
 *
 *
 *  @param[in]     pDocHnd           Handle of the parsed XML file
 *
 */
EXT_DECL void tau_freeXmlDoc (
    TRDP_XML_DOC_HANDLE_T *pDocHnd)
{
    /*  Check parameter */
    if (!pDocHnd)
    {
        return;
    }

    trdp_XMLClose(pDocHnd->pXmlDocument);

    vos_memFree(pDocHnd->pXmlDocument);

    pDocHnd->pXmlDocument = NULL;
}

/**********************************************************************************************************************/
/**    Read the interface relevant telegram parameters (except data set configuration) out of the configuration file .
 *
 *
 *  @param[in]      pDocHnd           Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[in]      pIfName           Interface name
 *  @param[out]     pProcessConfig    TRDP process (session) configuration for the interface
 *  @param[out]     pPdConfig         PD default configuration for the interface
 *  @param[out]     pMdConfig         MD default configuration for the interface
 *  @param[out]     pNumExchgPar      Number of configured telegrams
 *  @param[out]     ppExchgPar        Pointer to array of telegram configurations
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlInterfaceConfig (
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    const CHAR8                 *pIfName,
    TRDP_PROCESS_CONFIG_T       *pProcessConfig,
    TRDP_PD_CONFIG_T            *pPdConfig,
    TRDP_MD_CONFIG_T            *pMdConfig,
    UINT32                      *pNumExchgPar,
    TRDP_EXCHG_PAR_T            * *ppExchgPar
    )
{
    CHAR8       tag[MAX_TAG_LEN];
    CHAR8       attribute[MAX_TOK_LEN];
    CHAR8       value[MAX_TOK_LEN];
    UINT32      valueInt;
    TRDP_ERR_T  result = TRDP_NO_ERR;

    /*  Check parameters    */
    if (!pDocHnd || !pIfName || !pNumExchgPar || !ppExchgPar)
    {
        return TRDP_PARAM_ERR;
    }
    if (!pPdConfig || !pMdConfig)
    {
        return TRDP_PARAM_ERR;
    }

    trdp_XMLRewind(pDocHnd->pXmlDocument);

    /* Set default values */
    *pNumExchgPar   = 0;
    *ppExchgPar     = NULL;

    setDefaultInterfaceValues(pProcessConfig, pPdConfig, pMdConfig);

    trdp_XMLEnter(pDocHnd->pXmlDocument);

    if (trdp_XMLSeekStartTag(pDocHnd->pXmlDocument, "device") == 0) /* Optional */
    {
        if (pProcessConfig != NULL)
        {
            while (trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &valueInt, value) == TOK_ATTRIBUTE)
            {
                if (vos_strnicmp(attribute, "host-name", MAX_TOK_LEN) == 0)
                {
                    vos_strncpy(pProcessConfig->hostName, value, TRDP_MAX_LABEL_LEN - 1);
                }
                else if (vos_strnicmp(attribute, "leader-name", MAX_TOK_LEN) == 0)
                {
                    vos_strncpy(pProcessConfig->leaderName, value, TRDP_MAX_LABEL_LEN - 1);
                }
            }
        }

        trdp_XMLEnter(pDocHnd->pXmlDocument);

        /* Iterate thru <device> */
        while (trdp_XMLSeekStartTagAny(pDocHnd->pXmlDocument, tag, MAX_TAG_LEN) == 0)
        {
            if (vos_strnicmp(tag, "bus-interface-list", MAX_TAG_LEN) == 0)
            {
                int foundIdx = 0;

                trdp_XMLEnter(pDocHnd->pXmlDocument);

                /* Iterate thru <bus-interface-list>,
                    find the first that matches ifName, if set */

                while (trdp_XMLSeekStartTag(pDocHnd->pXmlDocument, "bus-interface") == 0)
                {
                    UINT32 idx = 0, count = 0;

                    /* find the interface, if its name was supplied, otherwise take the first one which was defined */
                    if (pIfName != NULL && strlen(pIfName))
                    {
                        while (trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &valueInt,
                                                    value) == TOK_ATTRIBUTE)
                        {
                            if (vos_strnicmp(attribute, "name", MAX_TOK_LEN) == 0 &&
                                vos_strnicmp(pIfName, value, TRDP_MAX_LABEL_LEN) == 0)
                            {
                                foundIdx = 1;
                            }
                        }
                        if (foundIdx == 0)
                        {
                            continue;
                        }
                    }

                    trdp_XMLEnter(pDocHnd->pXmlDocument);

                    /* find out how many telegrams are defined before hand */

                    count = trdp_XMLCountStartTag(pDocHnd->pXmlDocument, "telegram");

                    if (count > 0)
                    {
                        *ppExchgPar = (TRDP_EXCHG_PAR_T *)vos_memAlloc(count * sizeof(TRDP_EXCHG_PAR_T));

                        if (*ppExchgPar == NULL)
                        {
                            vos_printLog(VOS_LOG_ERROR,
                                         "%lu Bytes failed to allocate while reading XML telegram definitions!\n",
                                         count * sizeof(TRDP_EXCHG_PAR_T));
                            return TRDP_MEM_ERR;
                        }
                    }

                    while (trdp_XMLSeekStartTagAny(pDocHnd->pXmlDocument, tag, MAX_TAG_LEN) == 0)
                    {
                        if (vos_strnicmp(tag, "pd-com-parameter", MAX_TOK_LEN) == 0)
                        {
                            while (trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &valueInt,
                                                        value) == TOK_ATTRIBUTE)
                            {
                                if (vos_strnicmp(attribute, "marshall", MAX_TOK_LEN) == 0)
                                {
                                    if (vos_strnicmp("on", value, TRDP_MAX_LABEL_LEN) == 0)
                                    {
                                        pPdConfig->flags    |= TRDP_FLAGS_MARSHALL;
                                        pPdConfig->flags    &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                                    }
                                }
                                else if (vos_strnicmp(attribute, "validity-behavior", MAX_TOK_LEN) == 0)
                                {
                                    if (vos_strnicmp("keep", value, TRDP_MAX_LABEL_LEN) == 0)
                                    {
                                        pPdConfig->toBehavior |= TRDP_TO_KEEP_LAST_VALUE;
                                    }
                                    else
                                    {
                                        pPdConfig->toBehavior |= TRDP_TO_SET_TO_ZERO;
                                    }
                                }
                                else if (vos_strnicmp(attribute, "callback", MAX_TOK_LEN) == 0)
                                {
                                    if (vos_strnicmp("on", value, TRDP_MAX_LABEL_LEN) == 0)
                                    {
                                        pPdConfig->flags    |= TRDP_FLAGS_CALLBACK;
                                        pPdConfig->flags    &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                                    }
                                    else if (vos_strnicmp("always", value, TRDP_MAX_LABEL_LEN) == 0)
                                    {
                                        pPdConfig->flags    |= TRDP_FLAGS_FORCE_CB;
                                        pPdConfig->flags    &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                                    }
                                }
                                else if (vos_strnicmp(attribute, "port", MAX_TOK_LEN) == 0)
                                {
                                    pPdConfig->port = valueInt;
                                }
                                else if (vos_strnicmp(attribute, "ttl", MAX_TOK_LEN) == 0)
                                {
                                    pPdConfig->sendParam.ttl = valueInt;
                                }
                                else if (vos_strnicmp(attribute, "qos", MAX_TOK_LEN) == 0)
                                {
                                    pPdConfig->sendParam.qos = valueInt;
                                }
                            }
                        }

                        if (vos_strnicmp(tag, "md-com-parameter", MAX_TAG_LEN) == 0)
                        {
                            while (trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &valueInt,
                                                        value) == TOK_ATTRIBUTE)
                            {
                                if (vos_strnicmp(attribute, "marshall", MAX_TOK_LEN) == 0)
                                {
                                    if (vos_strnicmp("on", value, TRDP_MAX_LABEL_LEN) == 0)
                                    {
                                        pMdConfig->flags    |= TRDP_FLAGS_MARSHALL;
                                        pMdConfig->flags    &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                                    }
                                }
                                else if (vos_strnicmp(attribute, "protocol", MAX_TOK_LEN) == 0)
                                {
                                    if (vos_strnicmp("TCP", value, TRDP_MAX_LABEL_LEN) == 0)
                                    {
                                        pMdConfig->flags    |= TRDP_FLAGS_TCP;
                                        pMdConfig->flags    &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                                    }
                                }
                                else if (vos_strnicmp(attribute, "callback", MAX_TOK_LEN) == 0)
                                {
                                    if (vos_strnicmp("on", value, TRDP_MAX_LABEL_LEN) == 0)
                                    {
                                        pMdConfig->flags    |= TRDP_FLAGS_CALLBACK;
                                        pMdConfig->flags    &= (TRDP_FLAGS_T) ~TRDP_FLAGS_NONE;
                                    }
                                }
                                else if (vos_strnicmp(attribute, "udp-port", MAX_TOK_LEN) == 0)
                                {
                                    pMdConfig->udpPort = valueInt;
                                }
                                else if (vos_strnicmp(attribute, "tcp-port", MAX_TOK_LEN) == 0)
                                {
                                    pMdConfig->tcpPort = valueInt;
                                }
    #ifdef TRDP_RETRIES
                                else if (vos_strnicmp(attribute, "retries", MAX_TOK_LEN) == 0)
                                {
                                    pMdConfig->sendParam.retries = valueInt;
                                }
    #endif
                                else if (vos_strnicmp(attribute, "ttl", MAX_TOK_LEN) == 0)
                                {
                                    pMdConfig->sendParam.ttl = valueInt;
                                }
                                else if (vos_strnicmp(attribute, "qos", MAX_TOK_LEN) == 0)
                                {
                                    pMdConfig->sendParam.qos = valueInt;
                                }
                                else if (vos_strnicmp(attribute, "num-sessions", MAX_TOK_LEN) == 0)
                                {
                                    pMdConfig->maxNumSessions = valueInt;
                                }
                                else if (vos_strnicmp(attribute, "confirm-timeout", MAX_TOK_LEN) == 0)
                                {
                                    pMdConfig->confirmTimeout = valueInt;
                                }
                                else if (vos_strnicmp(attribute, "connect-timeout", MAX_TOK_LEN) == 0)
                                {
                                    pMdConfig->connectTimeout = valueInt;
                                }
                                else if (vos_strnicmp(attribute, "reply-timeout", MAX_TOK_LEN) == 0)
                                {
                                    pMdConfig->replyTimeout = valueInt;
                                }
                            }
                        }

                        if (vos_strnicmp(tag, "trdp-process", MAX_TAG_LEN) == 0 && pProcessConfig != NULL)
                        {
                            while (trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &valueInt,
                                                        value) == TOK_ATTRIBUTE)
                            {
                                if (vos_strnicmp(attribute, "blocking", MAX_TOK_LEN) == 0)
                                {
                                    if (vos_strnicmp("yes", value, TRDP_MAX_LABEL_LEN) == 0)
                                    {
                                        pProcessConfig->options |= TRDP_OPTION_BLOCK;
                                    }
                                }
                                else if (vos_strnicmp(attribute, "traffic-shaping", MAX_TOK_LEN) == 0)
                                {
                                    if (vos_strnicmp("off", value, TRDP_MAX_LABEL_LEN) == 0)
                                    {
                                        pProcessConfig->options &= (TRDP_OPTION_T) ~TRDP_OPTION_TRAFFIC_SHAPING;
                                    }
                                }
                                else if (vos_strnicmp(attribute, "priority", MAX_TOK_LEN) == 0)
                                {
                                    pProcessConfig->priority = valueInt;
                                }
                                else if (vos_strnicmp(attribute, "cycle-time", MAX_TOK_LEN) == 0)
                                {
                                    pProcessConfig->cycleTime = valueInt;
                                }
                            }
                        }
                        /* read the n-th telegram / exchange parameters */
                        if (count > 0 && count > idx && vos_strnicmp(tag, "telegram", MAX_TAG_LEN) == 0)
                        {
                            trdp_XMLEnter(pDocHnd->pXmlDocument);
                            result = readTelegramDef(pDocHnd->pXmlDocument, &(*ppExchgPar)[idx]);
                            //dbgPrint(1, &(*ppExchgPar)[idx]);
                            trdp_XMLLeave(pDocHnd->pXmlDocument);
                            if (result != TRDP_NO_ERR)
                            {
                                return result;
                            }
                            idx++;
                        }
                    }
                    *pNumExchgPar = count;
                }
                trdp_XMLLeave(pDocHnd->pXmlDocument);
            }
        }
        trdp_XMLLeave(pDocHnd->pXmlDocument);
    }
    trdp_XMLLeave(pDocHnd->pXmlDocument);

    return result;
}

/**********************************************************************************************************************/
/**    Free array of telegram configurations allocated by tau_readXmlInterfaceConfig
 *
 *  @param[in]      numExchgPar       Number of telegram configurations in the array
 *  @param[in]      pExchgPar         Pointer to array of telegram configurations
 *
 */
EXT_DECL void tau_freeTelegrams (
    UINT32              numExchgPar,
    TRDP_EXCHG_PAR_T    *pExchgPar)
{
    UINT32 idxEP, i;

    /*  Check parameters    */
    if (numExchgPar == 0 || pExchgPar == NULL)
    {
        return;
    }

    /*  Iterate over all exchange parameters    */
    for (idxEP = 0; idxEP < numExchgPar; idxEP++)
    {
        /*  Free MD and PD parameters   */
        if (pExchgPar[idxEP].pMdPar)
        {
            vos_memFree(pExchgPar[idxEP].pMdPar);
        }

        if (pExchgPar[idxEP].pPdPar)
        {
            vos_memFree(pExchgPar[idxEP].pPdPar);
        }

        /*  vos_memFree array of destinations  */
        if (pExchgPar[idxEP].destCnt)
        {
            /*  Iterate ove all destinations    */
            for (i = 0; i < pExchgPar[idxEP].destCnt; i++)
            {
                /*  vos_free URIs   */
                if (pExchgPar[idxEP].pDest[i].pUriHost)
                {
                    vos_memFree(pExchgPar[idxEP].pDest[i].pUriHost);
                }
                if (pExchgPar[idxEP].pDest[i].pUriUser)
                {
                    vos_memFree(pExchgPar[idxEP].pDest[i].pUriUser);
                }
                /*  Free SDT parameters */
                if (pExchgPar[idxEP].pDest[i].pSdtPar)
                {
                    vos_memFree(pExchgPar[idxEP].pDest[i].pSdtPar);
                }
            }
            /*  Free destinations array  */
            vos_memFree(pExchgPar[idxEP].pDest);
        }
        /*  Free array of sources  */
        if (pExchgPar[idxEP].srcCnt)
        {
            /*  Iterate over all sources    */
            for (i = 0; i < pExchgPar[idxEP].srcCnt; i++)
            {
                /*  Free URIs   */
                if (pExchgPar[idxEP].pSrc[i].pUriHost1)
                {
                    vos_memFree(pExchgPar[idxEP].pSrc[i].pUriHost1);
                }
                if (pExchgPar[idxEP].pSrc[i].pUriHost2)
                {
                    vos_memFree(pExchgPar[idxEP].pSrc[i].pUriHost2);
                }
                if (pExchgPar[idxEP].pSrc[i].pUriUser)
                {
                    vos_memFree(pExchgPar[idxEP].pSrc[i].pUriUser);
                }
                /*  Free SDT parameters */
                if (pExchgPar[idxEP].pSrc[i].pSdtPar)
                {
                    vos_memFree(pExchgPar[idxEP].pSrc[i].pSdtPar);
                }
            }
            /*  Free sources array  */
            vos_memFree(pExchgPar[idxEP].pSrc);
        }

    }

    /*  Free array of TRDP_EXCHG_PAR_T structures   */
    vos_memFree(pExchgPar);
}

/**********************************************************************************************************************/
/**    Function to read the TRDP device configuration parameters out of the XML configuration file.
 *  The user must release the memory for ppComPar and ppIfConfig (using vos_memFree)
 *
 *  @param[in]      pDocHnd           Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[out]     pMemConfig        Memory configuration
 *  @param[out]     pDbgConfig        Debug printout configuration for application use
 *  @param[out]     pNumComPar        Number of configured com parameters
 *  @param[out]     ppComPar          Pointer to array of com parameters
 *  @param[out]     pNumIfConfig      Number of configured interfaces
 *  @param[out]     ppIfConfig        Pointer to an array of interface parameter sets
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlDeviceConfig (
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    TRDP_MEM_CONFIG_T           *pMemConfig,
    TRDP_DBG_CONFIG_T           *pDbgConfig,
    UINT32                      *pNumComPar,
    TRDP_COM_PAR_T              * *ppComPar,
    UINT32                      *pNumIfConfig,
    TRDP_IF_CONFIG_T            * *ppIfConfig
    )
{
    CHAR8   tag[MAX_TAG_LEN];
    CHAR8   attribute[MAX_TOK_LEN];
    CHAR8   value[MAX_TOK_LEN];
    UINT32  valueInt;

    trdp_XMLRewind(pDocHnd->pXmlDocument);

    /*  Default all parameters    */
    setDefaultDeviceValues(pMemConfig, pDbgConfig, pNumComPar, ppComPar, pNumIfConfig, ppIfConfig);

    trdp_XMLEnter(pDocHnd->pXmlDocument);

    if (trdp_XMLSeekStartTag(pDocHnd->pXmlDocument, "device") == 0) /* Optional */
    {

        trdp_XMLEnter(pDocHnd->pXmlDocument);

        while (trdp_XMLSeekStartTagAny(pDocHnd->pXmlDocument, tag, MAX_TAG_LEN) == 0)
        {
            if (vos_strnicmp(tag, "device-configuration", MAX_TAG_LEN) == 0)
            {
                /* Get attribute data */
                if (trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &valueInt, value) == TOK_ATTRIBUTE)
                {
                    if (vos_strnicmp(attribute, "memory-size", MAX_TOK_LEN) == 0)
                    {
                        pMemConfig->size = (UINT32) valueInt;
                    }
                }
                trdp_XMLEnter(pDocHnd->pXmlDocument);
                if (trdp_XMLSeekStartTag(pDocHnd->pXmlDocument, "mem-block-list") == 0)
                {
                    /* Read the memory configuration */
                    trdp_XMLEnter(pDocHnd->pXmlDocument);
                    while (trdp_XMLSeekStartTag(pDocHnd->pXmlDocument, "mem-block") == 0)
                    {
                        const UINT32    mem_list[] =
                        {32, 72, 128, 256, 512, 1024, 1480, 2048, 4096, 11520, 16384, 32768, 65536, 131072};
                        UINT32          sizeValue   = 0;
                        UINT32          preAlloc    = 0;
                        int             i;
                        XML_TOKEN_T     found;

                        found = trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &sizeValue, value);
                        if (found == TOK_ATTRIBUTE && vos_strnicmp(attribute, "size", MAX_TOK_LEN) == 0)
                        {
                            found = trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &preAlloc, value);
                            if (found == TOK_ATTRIBUTE && vos_strnicmp(attribute, "preallocate", MAX_TOK_LEN) == 0)
                            {
                                /* Find the slot to store the value in  */
                                for (i = 0; sizeValue > mem_list[i]; i++)
                                {
                                    ;
                                }
                                if (i < 15)
                                {
                                    pMemConfig->prealloc[i] = preAlloc;
                                }
                            }
                        }
                    }
                    trdp_XMLLeave(pDocHnd->pXmlDocument);
                }
                trdp_XMLLeave(pDocHnd->pXmlDocument);
            }
            else if (pDbgConfig != NULL && vos_strnicmp(tag, "debug", MAX_TAG_LEN) == 0)
            {
                /* Get attribute data */
                while (trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &valueInt, value) == TOK_ATTRIBUTE)
                {
                    if (vos_strnicmp(attribute, "file-name", MAX_TOK_LEN) == 0)
                    {
                        vos_strncpy(pDbgConfig->fileName, value, TRDP_MAX_FILE_NAME_LEN - 1);
                    }
                    else if (vos_strnicmp(attribute, "file-size", MAX_TOK_LEN) == 0)
                    {
                        pDbgConfig->maxFileSize = valueInt;
                    }
                    else if (vos_strnicmp(attribute, "level", MAX_TOK_LEN) == 0)
                    {
                        if (strpbrk(value, "Dd") != NULL)
                        {
                            pDbgConfig->option = TRDP_DBG_DBG | TRDP_DBG_WARN | TRDP_DBG_INFO | TRDP_DBG_ERR;
                        }
                        if (strpbrk(value, "Ww") != NULL)
                        {
                            pDbgConfig->option |= TRDP_DBG_WARN | TRDP_DBG_ERR;
                        }
                        if (strpbrk(value, "Ee") != NULL)
                        {
                            pDbgConfig->option |= TRDP_DBG_ERR | TRDP_DBG_WARN | TRDP_DBG_ERR;
                        }
                        if (strpbrk(value, "Ii") != NULL)
                        {
                            pDbgConfig->option |= TRDP_DBG_INFO;
                        }
                    }
                }
            }
            else if (vos_strnicmp(tag, "com-parameter-list", MAX_TAG_LEN) == 0)
            {
                UINT32 count = 0;
                trdp_XMLEnter(pDocHnd->pXmlDocument);

                count = trdp_XMLCountStartTag(pDocHnd->pXmlDocument, "com-parameter");

                *ppComPar = (TRDP_COM_PAR_T *)vos_memAlloc(count * sizeof(TRDP_COM_PAR_T));

                if (*ppComPar != NULL)
                {
                    UINT32 i;
                    *pNumComPar = count;

                    /* Read the com params */
                    for (i = 0; i < count && trdp_XMLSeekStartTag(pDocHnd->pXmlDocument, "com-parameter") == 0; i++)
                    {
                        while (trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &valueInt,
                                                    value) == TOK_ATTRIBUTE)
                        {
                            if (vos_strnicmp(attribute, "id", MAX_TOK_LEN) == 0)
                            {
                                (*ppComPar)[i].id = valueInt;
                            }
                            else if (vos_strnicmp(attribute, "qos", MAX_TOK_LEN) == 0)
                            {
                                (*ppComPar)[i].sendParam.qos = valueInt;
                            }
                            else if (vos_strnicmp(attribute, "ttl", MAX_TOK_LEN) == 0)
                            {
                                (*ppComPar)[i].sendParam.ttl = valueInt;
                            }
#ifdef TRDP_RETRIES
                            else if (vos_strnicmp(attribute, "retries", MAX_TOK_LEN) == 0)
                            {
                                (*ppComPar)[i].sendParam.retries = valueInt;
                            }
#endif
                        }
                    }
                }
                trdp_XMLLeave(pDocHnd->pXmlDocument);
            }
            else if (vos_strnicmp(tag, "bus-interface-list", MAX_TAG_LEN) == 0)
            {
                UINT32 count = 0;
                trdp_XMLEnter(pDocHnd->pXmlDocument);

                count = trdp_XMLCountStartTag(pDocHnd->pXmlDocument, "bus-interface");

                *ppIfConfig = (TRDP_IF_CONFIG_T *)vos_memAlloc(count * sizeof(TRDP_IF_CONFIG_T));

                if (*ppIfConfig != NULL)
                {
                    UINT32 i;
                    *pNumIfConfig = count;

                    /* Read the interface params */
                    for (i = 0; i < count && trdp_XMLSeekStartTag(pDocHnd->pXmlDocument, "bus-interface") == 0; i++)
                    {
                        while (trdp_XMLGetAttribute(pDocHnd->pXmlDocument, attribute, &valueInt,
                                                    value) == TOK_ATTRIBUTE)
                        {
                            if (vos_strnicmp(attribute, "network-id", MAX_TOK_LEN) == 0)
                            {
                                (*ppIfConfig)[i].networkId = valueInt;
                            }
                            else if (vos_strnicmp(attribute, "name", MAX_TOK_LEN) == 0)
                            {
                                vos_strncpy((*ppIfConfig)[i].ifName, value, TRDP_MAX_LABEL_LEN - 1);
                            }
                            else if (vos_strnicmp(attribute, "host-ip", MAX_TOK_LEN) == 0)
                            {
                                (*ppIfConfig)[i].hostIp = vos_dottedIP(value);
                            }
                            else if (vos_strnicmp(attribute, "leader-ip", MAX_TOK_LEN) == 0)
                            {
                                (*ppIfConfig)[i].leaderIp = vos_dottedIP(value);
                            }
                        }
                    }
                }
                trdp_XMLLeave(pDocHnd->pXmlDocument);
            }
        }
        trdp_XMLLeave(pDocHnd->pXmlDocument);
    }

    trdp_XMLLeave(pDocHnd->pXmlDocument);

    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to read the DataSet configuration out of the XML configuration file.
 *
 *
 *  @param[in]      pDocHnd             Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[out]     pNumComId           Pointer to the number of entries in the ComId DatasetId mapping list
 *  @param[out]     ppComIdDsIdMap      Pointer to an array of a structures of type TRDP_COMID_DSID_MAP_T
 *  @param[out]     pNumDataset         Pointer to the number of datasets found in the configuration
 *  @param[out]     apDataset           Pointer to an array of pointers to a structure of type TRDP_DATASET_T
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_MEM_ERR        provided buffer to small
 *  @retval         TRDP_PARAM_ERR      File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlDatasetConfig (
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    UINT32                      *pNumComId,
    TRDP_COMID_DSID_MAP_T       * *ppComIdDsIdMap,
    UINT32                      *pNumDataset,
    apTRDP_DATASET_T            *apDataset
    )
{
    TRDP_ERR_T err = readXmlDatasetMap(pDocHnd->pXmlDocument, pNumComId, ppComIdDsIdMap);
    if (err == TRDP_NO_ERR)
    {
        err = readXmlDatasets(pDocHnd->pXmlDocument, pNumDataset, apDataset);
    }
    return err;
}

/**********************************************************************************************************************/
/**    Function to free the memory for the DataSet configuration
 *
 *  Free the memory for the DataSet configuration which was allocated when parsing the XML configuration file.
 *
 *
 *  @param[in]      numComId            The number of entries in the ComId DatasetId mapping list
 *  @param[in]      pComIdDsIdMap       Pointer to an array of structures of type TRDP_COMID_DSID_MAP_T
 *  @param[in]      numDataset          The number of datasets found in the configuration
 *  @param[in]      ppDataset           Pointer to an array of pointers to a structures of type TRDP_DATASET_T
 *
 *  @retval         none
 *
 */
EXT_DECL void tau_freeXmlDatasetConfig (
    UINT32                  numComId,
    TRDP_COMID_DSID_MAP_T   *pComIdDsIdMap,
    UINT32                  numDataset,
    TRDP_DATASET_T          * *ppDataset)
{
    UINT32 i, j;

    /*  Mapping between ComId and DatasetId   */
    if (numComId > 0 && pComIdDsIdMap != NULL)
    {
        vos_memFree(pComIdDsIdMap);
    }

    /*  Dataset definitions   */
    if (numDataset > 0 && ppDataset != NULL)
    {
        for (i = 0; i < numDataset; ++i)
        {
            for (j = 0; j < ppDataset[i]->numElement; ++j)
            {
                if (ppDataset[i]->pElement[j].unit != NULL)
                {
                    vos_memFree(ppDataset[i]->pElement[j].unit);
                }
            }
            vos_memFree(ppDataset[i]);
        }
        vos_memFree(ppDataset);
    }
}
