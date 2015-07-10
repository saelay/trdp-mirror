/******************************************************************************/
/**
 * @file            tau_xml.c
 *
 * @brief           Functions for XML file parsing
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Tomas Svoboda, UniContorls a.s.
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 */

/*******************************************************************************
 * INCLUDES
 */
#ifndef VXWORKS /*XML does not work yet on vxworks */
#include <string.h>
#include <stdio.h>

#include "trdp_types.h"
#include "trdp_utils.h"
#include "tau_xml.h"
#include "libxml/parser.h"
#include "libxml/xpath.h"

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

/**********************************************************************************************************************/
/**    Load XML file into DOM tree, prepare XPath context.
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
    xmlDoc      *pXmlDoc;
    xmlNodePtr  pRootElement;
    xmlXPathContextPtr pXPathCtx;

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

    /*  Initialize XML parser library (libxml2), test version   */
    LIBXML_TEST_VERSION

    /*  Parse XML file  */
        pXmlDoc = xmlReadFile((const char *)pFileName, NULL, 0);
    pDocHnd->pXmlDocument = (void *)pXmlDoc;

    /*  Check parsing result    */
    if (!pXmlDoc)
    {
        /*  Get parser error    */
        xmlError *pError = xmlGetLastError();

        vos_printLog(VOS_LOG_ERROR, "Prepare XML doc: failed to parsed XML file\n");
        if (pError)
        {
            vos_printLog(VOS_LOG_ERROR, "  line: %i: %s\n", pError->line, pError->message);
        }

        return TRDP_PARAM_ERR;
    }

    /*  Get root element    */
    pRootElement = xmlDocGetRootElement(pXmlDoc);
    pDocHnd->pRootElement = (void *)pRootElement;
    /*  Check result    */
    if (!pRootElement)
    {
        vos_printLog(VOS_LOG_ERROR, "Prepare XML doc: failed to get document root element\n");
        return TRDP_PARAM_ERR;
    }

    if (!pRootElement)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Prepare XPath context   */
    pXPathCtx = xmlXPathNewContext(pXmlDoc);
    pDocHnd->pXPathContext = (void *)pXPathCtx;
    /*  Check result    */
    if (!pXPathCtx)
    {
        /*  Get parser error    */
        xmlError *pError = xmlGetLastError();

        vos_printLog(VOS_LOG_ERROR, "Prepare XML doc: failed to prepare XPath context\n");
        if (pError)
        {
            vos_printLog(VOS_LOG_ERROR, "  line: %i: %s\n", pError->line, pError->message);
        }

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
    TRDP_XML_DOC_HANDLE_T *pDocHnd
    )
{
    /*  Check parameter */
    if (!pDocHnd)
    {
        return;
    }

    /*  Free XPath context  */
    if (pDocHnd->pXPathContext)
    {
        xmlXPathFreeContext((xmlXPathContextPtr)pDocHnd->pXPathContext);
        pDocHnd->pXPathContext = NULL;
    }

    /*  Free the document   */
    if (pDocHnd->pXmlDocument)
    {
        xmlFreeDoc((xmlDoc *)pDocHnd->pXmlDocument);
        pDocHnd->pXmlDocument = NULL;
    }
    pDocHnd->pRootElement = NULL;

    /* cleanup the parser   */
    xmlCleanupParser();
}

/*
*   Parse TRDP_LABEL from attribute of given XML elemnt.
*   Parsed value is copied into provided lable.
*   If XML attribute does not exist, label is not modified.
*/
static void parseLABEL (xmlNodePtr pXmlElem, const char *pAttrName, TRDP_LABEL_T label)
{
    xmlChar *pAttr;

    /*  Check for valid parameters  */
    if (!pXmlElem || !pAttrName || !label)
    {
        return;
    }

    /*  Get attribute   */
    pAttr = xmlGetProp(pXmlElem, BAD_CAST pAttrName);

    /*  Copy the value of the attribute */
    if (pAttr)
    {
        vos_strncpy(label, (const char *)pAttr, sizeof(TRDP_LABEL_T));
        xmlFree(pAttr);
    }
}

/*
* Parse UINT32 value from xml attribute into supplied variable.
* When attribute does not exist, value variable is not modified.
*/
static void parseUINT32 (xmlNodePtr pXmlElem, const char *pAttrName, UINT32 *pValue)
{
    xmlChar *pAttr;

    /*  Check for valid parameters  */
    if (!pXmlElem || !pAttrName || !pValue)
    {
        return;
    }

    /*  Get attribute   */
    pAttr = xmlGetProp(pXmlElem, BAD_CAST pAttrName);

    /*  Parse the value */
    if (pAttr)
    {
        *pValue = (UINT32)strtoul((const char *)pAttr, NULL, 10);
        xmlFree(pAttr);
    }
}

/*
* Parse UINT16 value from xml attribute into supplied variable.
* When attribute does not exist, value variable is not modified.
*/
static void parseUINT16 (xmlNodePtr pXmlElem, const char *pAttrName, UINT16 *pValue)
{
    xmlChar *pAttr;

    /*  Check for valid parameters  */
    if (!pXmlElem || !pAttrName || !pValue)
    {
        return;
    }

    /*  Get attribute   */
    pAttr = xmlGetProp(pXmlElem, BAD_CAST pAttrName);

    /*  Parse the value */
    if (pAttr)
    {
        *pValue = (UINT16)strtoul((const char *)pAttr, NULL, 10);
        xmlFree(pAttr);
    }
}

/*
* Parse UINT8 value from xml attribute into supplied variable.
* When attribute does not exist, value variable is not modified.
*/
static void parseUINT8 (xmlNodePtr pXmlElem, const char *pAttrName, UINT8 *pValue)
{
    xmlChar *pAttr;

    /*  Check for valid parameters  */
    if (!pXmlElem || !pAttrName || !pValue)
    {
        return;
    }

    /*  Get attribute   */
    pAttr = xmlGetProp(pXmlElem, BAD_CAST pAttrName);

    /*  Parse the value */
    if (pAttr)
    {
        *pValue = (UINT8)strtoul((const char *)pAttr, NULL, 10);
        xmlFree(pAttr);
    }
}

/*
* Parse IP ADDRESS value from xml attribute into supplied variable.
* When attribute does not exist, value variable is not modified.
*/
static void parseIPADDR (xmlNodePtr pXmlElem, const char *pAttrName, UINT32 *pValue)
{
    xmlChar *pAttr;

    /*  Check for valid parameters  */
    if (!pXmlElem || !pAttrName || !pValue)
    {
        return;
    }

    /*  Get attribute   */
    pAttr = xmlGetProp(pXmlElem, BAD_CAST pAttrName);

    /*  Parse the value */
    if (pAttr)
    {
        *pValue = vos_dottedIP((const CHAR8 *)pAttr);
        xmlFree(pAttr);
    }
}

/*
* Checks if value of given attribute equals to supplied value, ignore case.
* Returns true if values are equal, false otherwise.
*/
static BOOL8 checkAttrValue (xmlNodePtr pXmlElem, const char *pAttrName, const char *pValue)
{
    xmlChar *pAttr;

    /*  Check for valid parameters  */
    if (!pXmlElem || !pAttrName || !pValue)
    {
        return FALSE;
    }

    /*  Get attribute   */
    pAttr = xmlGetProp(pXmlElem, BAD_CAST pAttrName);

    /*  Compare the value */
    if (pAttr)
    {
        if (!xmlStrcasecmp(pAttr, BAD_CAST pValue))
        {
            return TRUE;
        }
        xmlFree(pAttr);
    }

    return FALSE;
}

/*
 * Parse user and host part of the URI from given atribute in XML configuration. Syntax is [[user@]host].
 * URI parts are allocated dynamically (only if present in XML).
 * Pointers to allocated URIs are returned.
 */
static void parseURI (xmlNodePtr        pXmlElem,
                      const char        *pAttrName,
                      TRDP_URI_USER_T   * *ppUriUser,
                      TRDP_URI_HOST_T   * *ppUriHost)
{
    char    *pAttr;
    char    *pDelimiter;

    /*  Set empty */
    if (ppUriUser)
    {
        *ppUriUser = NULL;
    }
    if (ppUriHost)
    {
        *ppUriHost = NULL;
    }

    /*  Check for valid parameters  */
    if (!pXmlElem || !pAttrName)
    {
        return;
    }

    /*  Get attribute   */
    pAttr = (char *)xmlGetProp(pXmlElem, BAD_CAST pAttrName);
    if (!pAttr)
    {
        return;
    }

    /*  Get pointer to @ character    */
    pDelimiter = strchr(pAttr, '@');

    /*  Create host part of the URI */
    if (ppUriHost)
    {
        *ppUriHost = (TRDP_URI_HOST_T *) vos_memAlloc(sizeof(TRDP_URI_HOST_T));
        if (pDelimiter)
        {
            vos_strncpy((char *)*ppUriHost, pDelimiter + 1, sizeof(TRDP_URI_HOST_T));
        }
        else
        {
            vos_strncpy((char *)*ppUriHost, pAttr, sizeof(TRDP_URI_HOST_T));
        }
    }
    /*  Create user part of the URI */
    if (ppUriUser && pDelimiter && (pDelimiter > pAttr))
    {
        UINT32 size = (UINT32)(pDelimiter - pAttr);
        *ppUriUser = (TRDP_URI_USER_T *) vos_memAlloc(sizeof(TRDP_URI_USER_T));
        memset(*ppUriUser, 0, sizeof(TRDP_URI_USER_T));
        memcpy(*ppUriUser, pAttr, size < sizeof(TRDP_URI_USER_T) ? size : sizeof(TRDP_URI_USER_T));
    }
    xmlFree(pAttr);
}

/*
 * Parse type of dataset element.
 * Type may be specified either as string or number.
 */
static void parseDSElemType (xmlNodePtr pXmlElem, const char *pAttrName, UINT32 *pElemType)
{
    xmlChar *pAttr;
    UINT32  tmpType;

    /*  Check for valid parameters  */
    if (!pXmlElem || !pAttrName || !pElemType)
    {
        return;
    }

    *pElemType = 0;

    /*  Get attribute   */
    pAttr = xmlGetProp(pXmlElem, BAD_CAST pAttrName);
    if (!pAttr)
    {
        return;
    }

    /* Try to convert numerical value first */
    tmpType = strtoul((const char *)pAttr, NULL, 10);
    if (tmpType > 0)
    {
        *pElemType = tmpType;
    }
    /*  Try to convert string data type name    */
    else if (!xmlStrcmp(pAttr, BAD_CAST "BOOL8"))
    {
        *pElemType = TRDP_BOOL8;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "CHAR8"))
    {
        *pElemType = TRDP_CHAR8;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "UTF16"))
    {
        *pElemType = TRDP_UTF16;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "INT8"))
    {
        *pElemType = TRDP_INT8;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "INT16"))
    {
        *pElemType = TRDP_INT16;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "INT32"))
    {
        *pElemType = TRDP_INT32;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "INT64"))
    {
        *pElemType = TRDP_INT64;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "UINT8"))
    {
        *pElemType = TRDP_UINT8;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "UINT16"))
    {
        *pElemType = TRDP_UINT16;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "UINT32"))
    {
        *pElemType = TRDP_UINT32;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "UINT64"))
    {
        *pElemType = TRDP_UINT64;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "REAL32"))
    {
        *pElemType = TRDP_REAL32;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "REAL64"))
    {
        *pElemType = TRDP_REAL64;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "TIMEDATE32"))
    {
        *pElemType = TRDP_TIMEDATE32;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "TIMEDATE48"))
    {
        *pElemType = TRDP_TIMEDATE48;
    }
    else if (!xmlStrcmp(pAttr, BAD_CAST "TIMEDATE64"))
    {
        *pElemType = TRDP_TIMEDATE64;
    }

    xmlFree(pAttr);
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
        pPdConfig->pfCbFunction         = NULL;
        pPdConfig->pRefCon              = NULL;
        pPdConfig->flags                = TRDP_FLAGS_NONE;
        pPdConfig->port                 = TRDP_PD_UDP_PORT;
        pPdConfig->sendParam.qos        = TRDP_PD_DEFAULT_QOS;
        pPdConfig->sendParam.ttl        = TRDP_PD_DEFAULT_TTL;
#ifdef TRDP_RETRIES
        pPdConfig->sendParam.retries    = 0;
#endif
        pPdConfig->timeout              = TRDP_PD_DEFAULT_TIMEOUT;
        pPdConfig->toBehavior           = TRDP_TO_SET_TO_ZERO;
    }

    /*  Default Md configuration    */
    if (pMdConfig)
    {
        pMdConfig->pfCbFunction         = NULL;
        pMdConfig->pRefCon              = NULL;
        pMdConfig->confirmTimeout       = TRDP_MD_DEFAULT_CONFIRM_TIMEOUT;
        pMdConfig->connectTimeout       = TRDP_MD_DEFAULT_CONNECTION_TIMEOUT;
        pMdConfig->flags                = TRDP_FLAGS_NONE;
        pMdConfig->replyTimeout         = TRDP_MD_DEFAULT_REPLY_TIMEOUT;
        pMdConfig->sendParam.qos        = TRDP_MD_DEFAULT_QOS;
#ifdef TRDP_RETRIES
        pMdConfig->sendParam.retries    = TRDP_MD_DEFAULT_RETRIES;
#endif
        pMdConfig->sendParam.ttl        = TRDP_MD_DEFAULT_TTL;
        pMdConfig->tcpPort              = TRDP_MD_TCP_PORT;
        pMdConfig->udpPort              = TRDP_MD_UDP_PORT;
        pMdConfig->maxNumSessions       = TRDP_MD_MAX_NUM_SESSIONS;
    }
}


/*
*   Parse configuration of preallocated memory blocks
*/
static void parseMemBlockList (xmlNodePtr pMemBlockListElem, TRDP_MEM_CONFIG_T *pMemConfig)
{
    UINT32      blockSizes[VOS_MEM_NBLOCKSIZES] = VOS_MEM_BLOCKSIZES;
    xmlNodePtr  pMemBlockElem;
    UINT32      size;
    UINT32      i;

    if (!pMemConfig)
    {
        return;
    }

    /*  Clear the array of preallocated block sizes */
    memset(pMemConfig->prealloc, 0, sizeof(UINT32) * VOS_MEM_NBLOCKSIZES);

    /*  Iterate over all defined memory blocks  */
    pMemBlockElem = xmlFirstElementChild(pMemBlockListElem);
    while (pMemBlockElem != NULL)
    {
        /*  Parse size of the block */
        size = 0;
        parseUINT32(pMemBlockElem, "size", &size);
        if (size)
        {
            /*  Find memory block with corresponding size   */
            for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
            {
                if (blockSizes[i] == size)
                {
                    /*  Found the block, set number of preallocated blocks  */
                    pMemConfig->prealloc[i] = 0;
                    parseUINT32(pMemBlockElem, "preallocate", &pMemConfig->prealloc[i]);
                }
            }
        }

        pMemBlockElem = xmlNextElementSibling(pMemBlockElem);
    }
}

/*
*   Parse TRDP process configuration
*/
static void parseProcessConfig (xmlNodePtr pProcCfgElem, TRDP_PROCESS_CONFIG_T *pProcessConfig)
{
    if (!pProcessConfig)
    {
        return;
    }

    /*  Parse attribute values  */
    parseUINT32(pProcCfgElem, "cycle-time", &pProcessConfig->cycleTime);
    parseUINT32(pProcCfgElem, "priority", &pProcessConfig->priority);
    if (checkAttrValue(pProcCfgElem, "blocking", "yes"))
    {
        pProcessConfig->options |= TRDP_OPTION_BLOCK;
    }
    if (checkAttrValue(pProcCfgElem, "no-reuse-addr", "yes"))
    {
        pProcessConfig->options |= TRDP_OPTION_NO_REUSE_ADDR;
    }
    if (checkAttrValue(pProcCfgElem, "traffic-shaping", "off"))
    {
        pProcessConfig->options &= (TRDP_OPTION_T)(~TRDP_OPTION_TRAFFIC_SHAPING);
    }
}

/*
*   Parse default PD communication parameters
*/
static void parsePDDefaultParams (xmlNodePtr pPDParElem, TRDP_PD_CONFIG_T *pPdConfig)
{
    if (!pPdConfig)
    {
        return;
    }

    /*  Parse attribute values  */
    parseUINT32(pPDParElem, "timeout-value", &pPdConfig->timeout);
    if (checkAttrValue(pPDParElem, "validity-behavior", "keep"))
    {
        pPdConfig->toBehavior = TRDP_TO_KEEP_LAST_VALUE;
    }
    parseUINT8(pPDParElem, "ttl", &pPdConfig->sendParam.ttl);
    parseUINT8(pPDParElem, "qos", &pPdConfig->sendParam.qos);
    if (checkAttrValue(pPDParElem, "marshall", "on"))
    {
        pPdConfig->flags |= TRDP_FLAGS_MARSHALL;
    }
    if (checkAttrValue(pPDParElem, "callback", "on"))
    {
        pPdConfig->flags |= TRDP_FLAGS_CALLBACK;
    }
    parseUINT16(pPDParElem, "port", &pPdConfig->port);
}

/*
*   Parse default MD communication parameters
*/
static void parseMDDefaultParams (xmlNodePtr pMDParElem, TRDP_MD_CONFIG_T *pMdConfig)
{
    if (!pMdConfig)
    {
        return;
    }

    /*  Parse attribute values  */
    parseUINT32(pMDParElem, "confirm-timeout", &pMdConfig->confirmTimeout);
    parseUINT32(pMDParElem, "reply-timeout", &pMdConfig->replyTimeout);
    parseUINT32(pMDParElem, "connect-timeout", &pMdConfig->connectTimeout);
    parseUINT8(pMDParElem, "ttl", &pMdConfig->sendParam.ttl);
    parseUINT8(pMDParElem, "qos", &pMdConfig->sendParam.qos);
#ifdef TRDP_RETRIES
    parseUINT8(pMDParElem, "retries", &pMdConfig->sendParam.retries);
#endif
    if (checkAttrValue(pMDParElem, "protocol", "TCP"))
    {
        pMdConfig->flags |= TRDP_FLAGS_TCP;
    }
    if (checkAttrValue(pMDParElem, "marshall", "on"))
    {
        pMdConfig->flags |= TRDP_FLAGS_MARSHALL;
    }
    if (checkAttrValue(pMDParElem, "callback", "on"))
    {
        pMdConfig->flags |= TRDP_FLAGS_CALLBACK;
    }
    parseUINT16(pMDParElem, "udp-port", &pMdConfig->udpPort);
    parseUINT16(pMDParElem, "tcp-port", &pMdConfig->tcpPort);
    parseUINT32(pMDParElem, "num-sessions", &pMdConfig->maxNumSessions);
}

/*
*   Parse memory configuration from the the device-configuration element
*/
static void parseMemoryConfiguration (
    xmlNodePtr          pDevCfgElem,
    TRDP_MEM_CONFIG_T   *pMemConfig
    )
{
    xmlNodePtr pChildElement;

    /*  Get total TRDP memory size  */
    if (pMemConfig)
    {
        parseUINT32(pDevCfgElem, "memory-size", &pMemConfig->size);
    }

    /*  Iterate over all child elements, look for memory block list  */
    pChildElement = xmlFirstElementChild(pDevCfgElem);
    while (pChildElement != NULL)
    {
        /*  Memory blocks configuration  */
        if ((!xmlStrcmp(pChildElement->name, BAD_CAST "mem-block-list")))
        {
            parseMemBlockList(pChildElement, pMemConfig);
        }

        pChildElement = xmlNextElementSibling(pChildElement);
    }
}

/*
*   Parse content of the debug element
*/
static void parseDebugConfiguration (
    xmlNodePtr          pDbgCfgElem,
    TRDP_DBG_CONFIG_T   *pDbgConfig
    )
{
    xmlChar *pAttr;

    if (!pDbgConfig)
    {
        return;
    }

    /*  Parse attribute values  */
    parseUINT32(pDbgCfgElem, "file-size", &pDbgConfig->maxFileSize);
    pAttr = xmlGetProp(pDbgCfgElem, BAD_CAST "file-name");
    if (pAttr)
    {
        vos_strncpy(pDbgConfig->fileName, (const char *)pAttr, sizeof(TRDP_FILE_NAME_T));
        xmlFree(pAttr);
    }
    /*  Debug printout level    */
    pAttr = xmlGetProp(pDbgCfgElem, BAD_CAST "level");
    if (pAttr)
    {
        if (!xmlStrcasecmp(pAttr, BAD_CAST "") || !xmlStrcasecmp(pAttr, BAD_CAST " "))
        {
            pDbgConfig->option = TRDP_DBG_OFF;
        }
        else if (!xmlStrcasecmp(pAttr, BAD_CAST "e"))
        {
            pDbgConfig->option = TRDP_DBG_ERR;
        }
        else if (!xmlStrcasecmp(pAttr, BAD_CAST "w"))
        {
            pDbgConfig->option = TRDP_DBG_WARN;
        }
        else if (!xmlStrcasecmp(pAttr, BAD_CAST "i"))
        {
            pDbgConfig->option = TRDP_DBG_INFO;
        }
        else if (!xmlStrcasecmp(pAttr, BAD_CAST "d"))
        {
            pDbgConfig->option = TRDP_DBG_DBG;
        }
        xmlFree(pAttr);
    }
    /*  Debug info  */
    pAttr = xmlGetProp(pDbgCfgElem, BAD_CAST "info");
    if (pAttr)
    {
        if (!xmlStrcasecmp(pAttr, BAD_CAST "") || !xmlStrcasecmp(pAttr, BAD_CAST " "))
        {
            pDbgConfig->option &= (TRDP_DBG_OPTION_T) ~(TRDP_DBG_TIME | TRDP_DBG_LOC | TRDP_DBG_CAT);
        }
        else
        {
            if (xmlStrcasestr(pAttr, BAD_CAST "a"))
            {
                pDbgConfig->option |= TRDP_DBG_TIME | TRDP_DBG_LOC | TRDP_DBG_CAT;
            }
            if (xmlStrcasestr(pAttr, BAD_CAST "d") || xmlStrcasestr(pAttr, BAD_CAST "t"))
            {
                pDbgConfig->option |= TRDP_DBG_TIME;
            }
            if (xmlStrcasestr(pAttr, BAD_CAST "f"))
            {
                pDbgConfig->option |= TRDP_DBG_LOC;
            }
            if (xmlStrcasestr(pAttr, BAD_CAST "c"))
            {
                pDbgConfig->option |= TRDP_DBG_CAT;
            }
        }
        xmlFree(pAttr);
    }
}

/*
*   Parse list of communication parameters (com-parameter-list)
*/
static void parseComParameters (
    xmlNodePtr      pComParListElem,
    UINT32          *pNumComPar,
    TRDP_COM_PAR_T  * *ppComPar
    )
{
    xmlNodePtr  pChildElement;
    UINT32      lIndex;

    /*  Check parameters    */
    if (!pNumComPar || !ppComPar)
    {
        return;
    }

    /*  Get number of child elements    */
    *ppComPar   = NULL;
    *pNumComPar = (UINT32) xmlChildElementCount(pComParListElem);
    if (*pNumComPar == 0)
    {
        return;
    }

    /*  Allocate array of communication parameters  */
    *ppComPar = (TRDP_COM_PAR_T *) vos_memAlloc(sizeof(TRDP_COM_PAR_T) * *pNumComPar);

    /*  Iterate over all child elements (com-parameter), parse their attributes  */
    pChildElement   = xmlFirstElementChild(pComParListElem);
    lIndex          = 0;
    while (pChildElement != NULL && lIndex < *pNumComPar)
    {
        parseUINT32(pChildElement, "id", &(*ppComPar)[lIndex].id);
        parseUINT8(pChildElement, "qos", &(*ppComPar)[lIndex].sendParam.qos);
        (*ppComPar)[lIndex].sendParam.ttl = 64;
        parseUINT8(pChildElement, "ttl", &(*ppComPar)[lIndex].sendParam.ttl);
#ifdef TDRP_RETRIES
        (*ppComPar)[lIndex].sendParam.retries = 0;
        parseUINT8(pChildElement, "retries", &(*ppComPar)[lIndex].sendParam.retries);
#endif
        pChildElement = xmlNextElementSibling(pChildElement);
        lIndex++;
    }
}

/*
*   Parse list of bus interfaces (bus-interface-list)
*/
static void parseBusInterfaces (
    xmlNodePtr          pBusInterfaceListElem,
    UINT32              *pNumIfConfig,
    TRDP_IF_CONFIG_T    * *ppIfConfig
    )
{
    xmlNodePtr  pChildElement;
    UINT32      lIndex;

    /*  Check parameters    */
    if (!pNumIfConfig || !ppIfConfig)
    {
        return;
    }

    /*  Get number of child elements    */
    *ppIfConfig     = NULL;
    *pNumIfConfig   = (UINT32) xmlChildElementCount(pBusInterfaceListElem);
    if (*pNumIfConfig == 0)
    {
        return;
    }

    /*  Allocate array of interface configurations  */
    *ppIfConfig = (TRDP_IF_CONFIG_T *) vos_memAlloc(sizeof(TRDP_IF_CONFIG_T) * (*pNumIfConfig));

    /*  Iterate over all child elements (bus-interface), parse their attributes  */
    pChildElement   = xmlFirstElementChild(pBusInterfaceListElem);
    lIndex          = 0;
    while (pChildElement != NULL && lIndex < *pNumIfConfig)
    {
        parseUINT8(pChildElement, "network-id", &(*ppIfConfig)[lIndex].networkId);
        parseLABEL(pChildElement, "name", (*ppIfConfig)[lIndex].ifName);
        (*ppIfConfig)[lIndex].hostIp = 0;
        parseIPADDR(pChildElement, "host-ip", &(*ppIfConfig)[lIndex].hostIp);
        (*ppIfConfig)[lIndex].leaderIp = 0;
        parseIPADDR(pChildElement, "leader-ip", &(*ppIfConfig)[lIndex].leaderIp);

        pChildElement = xmlNextElementSibling(pChildElement);
        lIndex++;
    }
}


/**********************************************************************************************************************/
/**    Function to read the TRDP device configuration parameters out of the XML configuration file.
 *
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
    xmlNodePtr  pRootElement;
    xmlNodePtr  pChildElement;

    /*  Set default values to all parameters    */
    setDefaultDeviceValues(
        pMemConfig, pDbgConfig,
        pNumComPar, ppComPar, pNumIfConfig, ppIfConfig);

    /* Check document handle    */
    if (!pDocHnd)
    {
        return TRDP_PARAM_ERR;
    }
    pRootElement = (xmlNodePtr)pDocHnd->pRootElement;


    /*  Iterate over all child elements, call appropriate parser functions*/
    pChildElement = xmlFirstElementChild(pRootElement);
    while (pChildElement != NULL)
    {
        /*  Parse device-configuration element and all children  */
        if ((!xmlStrcmp(pChildElement->name, BAD_CAST "device-configuration")))
        {
            parseMemoryConfiguration(
                pChildElement, pMemConfig);
        }
        /*  Parse debug configuration  */
        else if ((!xmlStrcmp(pChildElement->name, BAD_CAST "debug")))
        {
            parseDebugConfiguration(pChildElement, pDbgConfig);
        }
        /*  Parse communication parameters  */
        else if ((!xmlStrcmp(pChildElement->name, BAD_CAST "com-parameter-list")))
        {
            parseComParameters(pChildElement, pNumComPar, ppComPar);
        }
        /*  Parse bus interfaces  */
        else if ((!xmlStrcmp(pChildElement->name, BAD_CAST "bus-interface-list")))
        {
            parseBusInterfaces(pChildElement, pNumIfConfig, ppIfConfig);
        }

        pChildElement = xmlNextElementSibling(pChildElement);
    }

    return TRDP_NO_ERR;
}

/*
 * Parse mapping between ComId and DatasetId from all telegram elements
 * defined in all bus interfaces
 */
static void parseComIdDsIdMap (
    xmlXPathContextPtr      pXPathCtx,
    UINT32                  *pNumComId,
    TRDP_COMID_DSID_MAP_T   * *ppComIdDsIdMap
    )
{
    xmlXPathObjectPtr   pXPathObj;
    xmlNodePtr          pTlgElem;
    int lIndex;

    /*  Check parameters    */
    if (!pNumComId || !ppComIdDsIdMap)
    {
        return;
    }

    /* Execute XPath expression   */
    pXPathObj = xmlXPathEvalExpression(BAD_CAST "//telegram", pXPathCtx);
    if (!pXPathObj)
    {
        return;
    }

    /*  Check if any telegram elements were found   */
    if (pXPathObj->nodesetval && pXPathObj->nodesetval->nodeNr)
    {
        /*  Allocate array of map structures    */
        *pNumComId      = pXPathObj->nodesetval->nodeNr;
        *ppComIdDsIdMap = (TRDP_COMID_DSID_MAP_T *) vos_memAlloc(
                sizeof(TRDP_COMID_DSID_MAP_T) * (*pNumComId));

        /* Iterate over all found telegram elements */
        for (lIndex = 0; lIndex < pXPathObj->nodesetval->nodeNr; lIndex++)
        {
            pTlgElem = pXPathObj->nodesetval->nodeTab[lIndex];
            /*  Check for correct XML node type */
            if (pTlgElem->type != XML_ELEMENT_NODE)
            {
                continue;
            }

            /*  Parse telegram attributes   */
            parseUINT32(pTlgElem, "com-id", &(*ppComIdDsIdMap)[lIndex].comId);
            parseUINT32(pTlgElem, "data-set-id", &(*ppComIdDsIdMap)[lIndex].datasetId);
        }
    }

    /*  Free XPath result   */
    xmlXPathFreeObject(pXPathObj);
}

/*
 * Parse list of dataset configurations
 */
static TRDP_ERR_T parseDatasets (
    xmlXPathContextPtr  pXPathCtx,
    UINT32              *pNumDataset,
    papTRDP_DATASET_T   papDataset
    )
{
    xmlXPathObjectPtr   pXPathObj;
    xmlNodePtr          pDatasetElem;
    xmlNodePtr          pElementElem;
    int idxDataset;
    UINT16 idxElem;
    UINT16 numElement;
    apTRDP_DATASET_T    apDataset;
    pTRDP_DATASET_T     pDataset;

    /*  Check parameters    */
    if (!pNumDataset || !papDataset)
    {
        return TRDP_NO_ERR;
    }

    /*  Set default values  */
    *pNumDataset    = 0;
    apDataset       = NULL;
    *papDataset     = NULL;

    /* Execute XPath expression   */
    pXPathObj = xmlXPathEvalExpression(BAD_CAST "/device/data-set-list/data-set", pXPathCtx);

    if (!pXPathObj)
    {
        return TRDP_NO_ERR;
    }

    /*  Check if any data-set elements were found   */
    if (pXPathObj->nodesetval && pXPathObj->nodesetval->nodeNr)
    {
        /*  Allocate array of pointers to dataset structures    */
        *pNumDataset    = pXPathObj->nodesetval->nodeNr;
        apDataset       = (apTRDP_DATASET_T) vos_memAlloc(
                sizeof(TRDP_DATASET_T *) * (*pNumDataset));
        if (apDataset == NULL)
        {
            return TRDP_MEM_ERR;
        }
        *papDataset = apDataset;

        /* Iterate over all found data-set elements */
        for (idxDataset = 0; idxDataset < pXPathObj->nodesetval->nodeNr; idxDataset++)
        {
            pDatasetElem = pXPathObj->nodesetval->nodeTab[idxDataset];
            /*  Check for correct XML node type */
            if (pDatasetElem->type != XML_ELEMENT_NODE)
            {
                continue;
            }

            /*  Get number of elements in the dataset   */
            numElement = (UINT16) xmlChildElementCount(pDatasetElem);

            /*  Allocate dataset structure with elements    */
            pDataset = (TRDP_DATASET_T *)vos_memAlloc(
                    sizeof(TRDP_DATASET_T) + sizeof(TRDP_DATASET_ELEMENT_T) * numElement);
            if (pDataset == NULL)
            {
                return TRDP_MEM_ERR;
            }
            apDataset[idxDataset] = pDataset;

            /*  Parse dataset attributes   */
            parseUINT32(pDatasetElem, "id", &pDataset->id);
            pDataset->numElement = numElement;

            /*  Iterate over all dataset elements   */
            idxElem         = 0;
            pElementElem    = xmlFirstElementChild(pDatasetElem);
            while (pElementElem != NULL && idxElem < numElement)
            {
                parseDSElemType(pElementElem, "type", &(pDataset->pElement[idxElem].type));
                pDataset->pElement[idxElem].size = 1;
                parseUINT32(pElementElem, "array-size", &(pDataset->pElement[idxElem].size));

                pElementElem = xmlNextElementSibling(pElementElem);
                idxElem++;
            }
        }
    }

    /*  Free XPath result   */
    xmlXPathFreeObject(pXPathObj);
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to read the DataSet configuration out of the XML configuration file.
 *
 *
 *  @param[in]      pDocHnd           Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[out]     pNumComId         Pointer to the number of entries in the ComId DatasetId mapping list
 *  @param[out]     ppComIdDsIdMap    Pointer to an array of a structures of type TRDP_COMID_DSID_MAP_T
 *  @param[out]     pNumDataset       Pointer to the number of datasets found in the configuration
 *  @param[out]     papDataset        Pointer to an array of pointers to a structures of type TRDP_DATASET_T
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlDatasetConfig (
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    UINT32                      *pNumComId,
    TRDP_COMID_DSID_MAP_T       * *ppComIdDsIdMap,
    UINT32                      *pNumDataset,
    papTRDP_DATASET_T           papDataset
    )
{
    xmlXPathContextPtr  pXPathCtx;
    TRDP_ERR_T          result;

    /*  Set empty values  */
    if (pNumComId)
    {
        *pNumComId = 0;
    }
    if (ppComIdDsIdMap)
    {
        *ppComIdDsIdMap = NULL;
    }
    if (pNumDataset)
    {
        *pNumDataset = 0;
    }
    if (papDataset)
    {
        *papDataset = NULL;
    }

    /* Check document handle    */
    if (!pDocHnd)
    {
        return TRDP_PARAM_ERR;
    }
    pXPathCtx = (xmlXPathContextPtr)pDocHnd->pXPathContext;

    /*  Parse mapping between ComId and DatasetId   */
    parseComIdDsIdMap(pXPathCtx, pNumComId, ppComIdDsIdMap);

    /*  Parse dataset definitions   */
    result = parseDatasets(pXPathCtx, pNumDataset, papDataset);

    return result;
}

/*
 * Parse message data communication parameters configured for one telegram
 * Default values are used for properties which are not configured
 */
static void parseMdParameter (
    xmlXPathContextPtr      pXPathCtx,
    const TRDP_MD_CONFIG_T  *pMdConfig,
    TRDP_EXCHG_PAR_T        *pExchgPar
    )
{
    xmlXPathObjectPtr   pXPathObj;
    xmlNodePtr          pMdParElem;

    /*  Try to find the md-parameter element    */
    pXPathObj = xmlXPathEvalExpression(BAD_CAST "./md-parameter", pXPathCtx);
    if (!pXPathObj)
    {
        return;
    }

    /*  Check if element was found  */
    if (pXPathObj->nodesetval && pXPathObj->nodesetval->nodeNr)
    {
        /*  Get the element */
        pMdParElem = pXPathObj->nodesetval->nodeTab[0];
        /*  Allocate the structure  */
        pExchgPar->pMdPar = (TRDP_MD_PAR_T *) vos_memAlloc(sizeof(TRDP_MD_PAR_T));

        /*  Set default values  */
        pExchgPar->pMdPar->confirmTimeout   = pMdConfig->confirmTimeout;
        pExchgPar->pMdPar->replyTimeout     = pMdConfig->replyTimeout;
        pExchgPar->pMdPar->flags = pMdConfig->flags;

        /*  Parse attributes    */
        parseUINT32(pMdParElem, "confirm-timeout", &pExchgPar->pMdPar->confirmTimeout);
        parseUINT32(pMdParElem, "reply-timeout", &pExchgPar->pMdPar->replyTimeout);
        if (checkAttrValue(pMdParElem, "protocol", "TCP"))
        {
            pExchgPar->pMdPar->flags |= TRDP_FLAGS_TCP;
        }
        if (checkAttrValue(pMdParElem, "protocol", "UDP"))
        {
            pExchgPar->pMdPar->flags &= (TRDP_FLAGS_T) ~TRDP_FLAGS_TCP;
        }
        if (checkAttrValue(pMdParElem, "marshall", "on"))
        {
            pExchgPar->pMdPar->flags |= TRDP_FLAGS_MARSHALL;
        }
        if (checkAttrValue(pMdParElem, "marshall", "off"))
        {
            pExchgPar->pMdPar->flags &= (TRDP_FLAGS_T) ~TRDP_FLAGS_MARSHALL;
        }
        if (checkAttrValue(pMdParElem, "callback", "on"))
        {
            pExchgPar->pMdPar->flags |= TRDP_FLAGS_CALLBACK;
        }
        if (checkAttrValue(pMdParElem, "callback", "off"))
        {
            pExchgPar->pMdPar->flags &= (TRDP_FLAGS_T) ~TRDP_FLAGS_CALLBACK;
        }
    }

    /*  Free XPath result   */
    xmlXPathFreeObject(pXPathObj);
}

/*
 * Parse process data communication parameters configured for one telegram
 * Default values are used for properties which are not configured
 */
static void parsePdParameter (
    xmlXPathContextPtr      pXPathCtx,
    const TRDP_PD_CONFIG_T  *pPdConfig,
    TRDP_EXCHG_PAR_T        *pExchgPar
    )
{
    xmlXPathObjectPtr   pXPathObj;
    xmlNodePtr          pPdParElem;

    /*  Try to find the pd-parameter element    */
    pXPathObj = xmlXPathEvalExpression(BAD_CAST "./pd-parameter", pXPathCtx);
    if (!pXPathObj)
    {
        return;
    }

    /*  Check if element was found  */
    if (pXPathObj->nodesetval && pXPathObj->nodesetval->nodeNr)
    {
        /*  Get the element */
        pPdParElem = pXPathObj->nodesetval->nodeTab[0];

        /*  Allocate the structure  */
        pExchgPar->pPdPar = (TRDP_PD_PAR_T *) vos_memAlloc(sizeof(TRDP_PD_PAR_T));

        /*  Set default values  */
        pExchgPar->pPdPar->flags        = pPdConfig->flags;
        pExchgPar->pPdPar->redundant    = 0;
        pExchgPar->pPdPar->timeout      = pPdConfig->timeout;
        pExchgPar->pPdPar->toBehav      = pPdConfig->toBehavior;
        pExchgPar->pPdPar->offset       = 0;

        /*  Parse attributes    */
        parseUINT32(pPdParElem, "timeout", &pExchgPar->pPdPar->timeout);
        parseUINT32(pPdParElem, "cycle", &pExchgPar->pPdPar->cycle);
        parseUINT32(pPdParElem, "redundant", &pExchgPar->pPdPar->redundant);
        parseUINT16(pPdParElem, "offset-address", &pExchgPar->pPdPar->offset);
        if (checkAttrValue(pPdParElem, "validity-behavior", "keep"))
        {
            pExchgPar->pPdPar->toBehav = TRDP_TO_KEEP_LAST_VALUE;
        }
        if (checkAttrValue(pPdParElem, "validity-behavior", "zero"))
        {
            pExchgPar->pPdPar->toBehav = TRDP_TO_SET_TO_ZERO;
        }
        if (checkAttrValue(pPdParElem, "marshall", "on"))
        {
            pExchgPar->pPdPar->flags |= TRDP_FLAGS_MARSHALL;
        }
        if (checkAttrValue(pPdParElem, "marshall", "off"))
        {
            pExchgPar->pPdPar->flags &= (TRDP_FLAGS_T) ~TRDP_FLAGS_MARSHALL;
        }
        if (checkAttrValue(pPdParElem, "callback", "on"))
        {
            pExchgPar->pPdPar->flags |= TRDP_FLAGS_CALLBACK;
        }
        if (checkAttrValue(pPdParElem, "callback", "off"))
        {
            pExchgPar->pPdPar->flags &= (TRDP_FLAGS_T) ~TRDP_FLAGS_CALLBACK;
        }
    }

    /*  Free XPath result   */
    xmlXPathFreeObject(pXPathObj);
}

/*
 *  Parse Safe Data Transmission parameters for given source or destination element.
 *  Looks for sdt-parameter child element. If it exists, new SDT_PAR structure is
 *  allocated and pointer is returned.
 */
static TRDP_ERR_T parseSDTParameters (xmlNodePtr pParentElem, TRDP_SDT_PAR_T * *ppSdtPar)
{
    xmlNodePtr      pSDTElem;
    TRDP_SDT_PAR_T  *pSdtPar;

    /*  Set empty   */
    *ppSdtPar = NULL;

    /*  Look for sdt-parameter element  */
    pSDTElem = xmlFirstElementChild(pParentElem);
    if (!pSDTElem)
    {
        return TRDP_NO_ERR;
    }
    if (xmlStrcmp(pSDTElem->name, BAD_CAST "sdt-parameter"))
    {
        return TRDP_NO_ERR;
    }

    /*  Allocate new SDT_PAR structure, set default values  */
    pSdtPar = (TRDP_SDT_PAR_T *) vos_memAlloc(sizeof(TRDP_SDT_PAR_T));
    if (pSdtPar == NULL)
    {
        return TRDP_MEM_ERR;
    }
    *ppSdtPar = pSdtPar;
    pSdtPar->smi2       = TRDP_SDT_DEFAULT_SMI2;
    pSdtPar->nrxSafe    = TRDP_SDT_DEFAULT_NRXSAFE;
    pSdtPar->nGuard     = TRDP_SDT_DEFAULT_NGUARD;
    pSdtPar->cmThr      = TRDP_SDT_DEFAULT_CMTHR;

    /*  Parse SDT attributes    */
    parseUINT32(pSDTElem, "smi1", &pSdtPar->smi1);
    parseUINT32(pSDTElem, "smi2", &pSdtPar->smi2);
    parseUINT16(pSDTElem, "udv", &pSdtPar->udv);
    parseUINT16(pSDTElem, "rx-period", &pSdtPar->rxPeriod);
    parseUINT16(pSDTElem, "tx-period", &pSdtPar->txPeriod);
    parseUINT8(pSDTElem, "n-rxsafe", &pSdtPar->nrxSafe);
    parseUINT16(pSDTElem, "n-guard", &pSdtPar->nGuard);
    parseUINT32(pSDTElem, "cm-thr", &pSdtPar->cmThr);
    return TRDP_NO_ERR;
}

/*
 *  Parse configuration of all sources for one telegram
 */
static TRDP_ERR_T parseSources (
    xmlXPathContextPtr  pXPathCtx,
    TRDP_EXCHG_PAR_T    *pExchgPar
    )
{
    TRDP_ERR_T  result;
    xmlXPathObjectPtr   pXPathObj;
    xmlNodePtr          pSourceElem;
    int         lIndex;

    /*  Try to find all source elements    */
    pXPathObj = xmlXPathEvalExpression(BAD_CAST "./source", pXPathCtx);
    if (!pXPathObj)
    {
        return TRDP_NO_ERR;
    }

    /*  Check if any sources were found  */
    if (!pXPathObj->nodesetval || !pXPathObj->nodesetval->nodeNr)
    {
        /*  Free XPath result   */
        xmlXPathFreeObject(pXPathObj);
        return TRDP_NO_ERR;
    }

    /*  Allocate array of SRC structures    */
    pExchgPar->srcCnt   = pXPathObj->nodesetval->nodeNr;
    pExchgPar->pSrc     = (TRDP_SRC_T *) vos_memAlloc(sizeof(TRDP_SRC_T) * pExchgPar->srcCnt);

    /*  Iterate over all found source elements  */
    for (lIndex = 0; lIndex < pXPathObj->nodesetval->nodeNr; lIndex++)
    {
        pSourceElem = pXPathObj->nodesetval->nodeTab[lIndex];
        /*  Check for correct XML node type */
        if (pSourceElem->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        /*  Parse the source element    */
        parseUINT32(pSourceElem, "id", &(pExchgPar->pSrc[lIndex].id));
        parseURI(pSourceElem, "uri1",
                 &(pExchgPar->pSrc[lIndex].pUriUser), &(pExchgPar->pSrc[lIndex].pUriHost1));
        parseURI(pSourceElem, "uri2",
                 NULL, &(pExchgPar->pSrc[lIndex].pUriHost2));

        /*  Parse SDT parameters   */
        result = parseSDTParameters(pSourceElem, &(pExchgPar->pSrc[lIndex].pSdtPar));
        if (result != TRDP_NO_ERR)
        {
            return result;
        }
    }

    /*  Free XPath result   */
    xmlXPathFreeObject(pXPathObj);
    return TRDP_NO_ERR;
}

/*
 *  Parse configuration of all destinations for one telegram
 */
static TRDP_ERR_T parseDestinations (
    xmlXPathContextPtr  pXPathCtx,
    TRDP_EXCHG_PAR_T    *pExchgPar
    )
{
    TRDP_ERR_T  result;
    xmlXPathObjectPtr   pXPathObj;
    xmlNodePtr          pDestElem;
    int         lIndex;

    /*  Try to find all destination elements    */
    pXPathObj = xmlXPathEvalExpression(BAD_CAST "./destination", pXPathCtx);
    if (!pXPathObj)
    {
        return TRDP_NO_ERR;
    }

    /*  Check if any destinations were found  */
    if (!pXPathObj->nodesetval || !pXPathObj->nodesetval->nodeNr)
    {
        /*  Free XPath result   */
        xmlXPathFreeObject(pXPathObj);
        return TRDP_NO_ERR;
    }

    /*  Allocate array of DEST structures    */
    pExchgPar->destCnt  = pXPathObj->nodesetval->nodeNr;
    pExchgPar->pDest    = (TRDP_DEST_T *) vos_memAlloc(sizeof(TRDP_DEST_T) * pExchgPar->destCnt);

    /*  Iterate over all found destination elements  */
    for (lIndex = 0; lIndex < pXPathObj->nodesetval->nodeNr; lIndex++)
    {
        pDestElem = pXPathObj->nodesetval->nodeTab[lIndex];
        /*  Check for correct XML node type */
        if (pDestElem->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        /*  Parse the destination element    */
        parseUINT32(pDestElem, "id", &(pExchgPar->pDest[lIndex].id));
        parseURI(pDestElem, "uri",
                 &(pExchgPar->pDest[lIndex].pUriUser), &(pExchgPar->pDest[lIndex].pUriHost));

        /*  Parse SDT parameters   */
        result = parseSDTParameters(pDestElem, &(pExchgPar->pDest[lIndex].pSdtPar));
        if (result != TRDP_NO_ERR)
        {
            return result;
        }
    }

    /*  Free XPath result   */
    xmlXPathFreeObject(pXPathObj);
    return TRDP_NO_ERR;
}

/*
 * Parse configuration of one telegram into provided structure
 */
static TRDP_ERR_T parseTelegram (
    xmlNodePtr              pTlgElem,
    xmlXPathContextPtr      pXPathCtx,
    const TRDP_PD_CONFIG_T  *pPdConfig,
    const TRDP_MD_CONFIG_T  *pMdConfig,
    TRDP_EXCHG_PAR_T        *pExchgPar
    )
{
    TRDP_ERR_T result;

    /*  Check parameters    */
    if (!pTlgElem || !pExchgPar)
    {
        return TRDP_NO_ERR;
    }

    /*  Set default values  */
    memset(pExchgPar, 0, sizeof(TRDP_EXCHG_PAR_T));

    /*  Parse telegram attributes   */
    parseUINT32(pTlgElem, "com-id", &pExchgPar->comId);
    parseUINT32(pTlgElem, "data-set-id", &pExchgPar->datasetId);
    parseUINT32(pTlgElem, "com-parameter-id", &pExchgPar->comParId);

    if (checkAttrValue(pTlgElem, "type", "source-sink"))
    {
        pExchgPar->type = TRDP_EXCHG_SOURCESINK;
    }
    else if (checkAttrValue(pTlgElem, "type", "source"))
    {
        pExchgPar->type = TRDP_EXCHG_SOURCE;
    }
    else if (checkAttrValue(pTlgElem, "type", "sink"))
    {
        pExchgPar->type = TRDP_EXCHG_SINK;
    }
    else
    {
        pExchgPar->type = TRDP_EXCHG_UNSET;
    }

    /*  Set current node to XPath context   */
    pXPathCtx->node = pTlgElem;

    /*  Call parser functions for nested elements   */
    parseMdParameter(pXPathCtx, pMdConfig, pExchgPar);
    parsePdParameter(pXPathCtx, pPdConfig, pExchgPar);
    result = parseSources(pXPathCtx, pExchgPar);
    if (result != TRDP_NO_ERR)
    {
        return result;
    }
    result = parseDestinations(pXPathCtx, pExchgPar);
    if (result != TRDP_NO_ERR)
    {
        return result;
    }
    return TRDP_NO_ERR;
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
    TRDP_ERR_T  result;
    xmlNodePtr          pRootElement;
    xmlXPathContextPtr  pXPathCtx;
    xmlXPathObjectPtr   pXPathObj;
    CHAR8       strXPath[256];
    xmlNodePtr          pIfElem;
    xmlNodePtr          pChildElement;
    xmlNodePtr          pTlgElem;
    int lIndex;
    int length;

    /*  Check parameters    */
    if (!pDocHnd || !pIfName || !pNumExchgPar || !ppExchgPar)
    {
        return TRDP_PARAM_ERR;
    }
    if (!pPdConfig || !pMdConfig)
    {
        return TRDP_PARAM_ERR;
    }

    pRootElement    = (xmlNodePtr)pDocHnd->pRootElement;
    pXPathCtx       = (xmlXPathContextPtr)pDocHnd->pXPathContext;

    /* Set default values */
    *pNumExchgPar   = 0;
    *ppExchgPar     = NULL;
    setDefaultInterfaceValues(pProcessConfig, pPdConfig, pMdConfig);

    /*  Read host names */
    if (pProcessConfig)
    {
        /*  Read host name attribute    */
        parseLABEL(pRootElement, "host-name", pProcessConfig->hostName);
        /*  Read    leader name attribute   */
        parseLABEL(pRootElement, "leader-name", pProcessConfig->leaderName);
    }

    /* Execute XPath expression - find given interface element  */
    {
        length = vos_snprintf(strXPath, sizeof(strXPath),
                              "/device/bus-interface-list/bus-interface[@name='%s']", pIfName);
    }

    if (length < 0 || length > (int) sizeof(strXPath))
    {
        return TRDP_PARAM_ERR;
    }
    pXPathObj = xmlXPathEvalExpression(BAD_CAST strXPath, pXPathCtx);
    /*  Check if bus-interface element was found    */
    if (pXPathObj)
    {
        if (pXPathObj->nodesetval && pXPathObj->nodesetval->nodeNr)
        {
            /*  Get the first found interface with given name - there should be only one!   */
            pIfElem = pXPathObj->nodesetval->nodeTab[0];

            /*  Iterate over all child elements, call parser functions for PROCESS, PD and MD config  */
            pChildElement = xmlFirstElementChild(pIfElem);
            while (pChildElement != NULL)
            {
                /*  TRDP process (session) configuration  */
                if ((!xmlStrcmp(pChildElement->name, BAD_CAST "trdp-process")))
                {
                    parseProcessConfig(pChildElement, pProcessConfig);
                }
                /*  PD default parameters  */
                else if ((!xmlStrcmp(pChildElement->name, BAD_CAST "pd-com-parameter")))
                {
                    parsePDDefaultParams(pChildElement, pPdConfig);
                }
                /*  MD default parameters  */
                else if ((!xmlStrcmp(pChildElement->name, BAD_CAST "md-com-parameter")))
                {
                    parseMDDefaultParams(pChildElement, pMdConfig);
                }

                pChildElement = xmlNextElementSibling(pChildElement);
            }
        }
        /*  Free XPath result   */
        xmlXPathFreeObject(pXPathObj);
    }


    /* Execute XPath expression - find all telegram elemments for given interface  */
    {
        length = vos_snprintf(strXPath, sizeof(strXPath),
                              "/device/bus-interface-list/bus-interface[@name='%s']/telegram", pIfName);
    }

    if (length < 0 || length > (int) sizeof(strXPath))
    {
        return TRDP_PARAM_ERR;
    }
    pXPathObj = xmlXPathEvalExpression(BAD_CAST strXPath, pXPathCtx);

    /*  Check if any telegrams were found   */
    if (pXPathObj)
    {
        if (pXPathObj->nodesetval && pXPathObj->nodesetval->nodeNr)
        {
            /*  Allocate array of exchange parameters   */
            *pNumExchgPar   = pXPathObj->nodesetval->nodeNr;
            *ppExchgPar     = (TRDP_EXCHG_PAR_T *) vos_memAlloc(
                    sizeof(TRDP_EXCHG_PAR_T) * (*pNumExchgPar));

            /*  Iterate over all found telegram definitions and parse them   */
            for (lIndex = 0; lIndex < pXPathObj->nodesetval->nodeNr; lIndex++)
            {
                pTlgElem = pXPathObj->nodesetval->nodeTab[lIndex];
                /*  Check for correct XML node type */
                if (pTlgElem->type != XML_ELEMENT_NODE)
                {
                    continue;
                }

                /*  Parse the telegram definition   */
                result = parseTelegram(pTlgElem, pXPathCtx, pPdConfig, pMdConfig, &((*ppExchgPar)[lIndex]));
                if (result != TRDP_NO_ERR)
                {
                    return result;
                }
            }
        }
        /*  Free XPath result   */
        xmlXPathFreeObject(pXPathObj);
    }

    return TRDP_NO_ERR;
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
    TRDP_EXCHG_PAR_T    *pExchgPar
    )
{
    UINT32 idxEP, i;

    /*  Check parameters    */
    if (!numExchgPar || !pExchgPar)
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
            /*  Iterate ove all sources    */
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
#endif
