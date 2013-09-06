/**********************************************************************************************************************/
/**
 * @file            tau_xml.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                  - read xml configuration interpreter
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 */

#ifndef TAU_XML_H
#define TAU_XML_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#include "trdp_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#ifndef __cdecl
#define __cdecl
#endif


/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Types to read out the XML configuration    */
typedef struct
{
    UINT32  smi1;        /**< Safe message identifier - unique for this message at consist level */
    UINT32  smi2;        /**< Safe message identifier - unique for this message at consist level */
    UINT32  cmThr;       /**< Channel monitoring threshold */
    UINT16  udv;         /**< User data version */
    UINT16  rxPeriod;    /**< Sink cycle time */
    UINT16  txPeriod;    /**< Source cycle time */
    UINT16  nGuard;      /**< Initial timeout cycles */
    UINT8   nrxSafe;     /**< Timout cycles */
    UINT8   reserved1;   /**< Reserved for future use */
    UINT16  reserved2;   /**< Reserved for future use */
} TRDP_SDT_PAR_T;

typedef struct
{
    UINT32              cycle;     /**< dataset identifier */
    UINT32              redundant; /**< N0 = not redundant, >0 redundancy group */
    UINT32              timeout;   /**< Timeout value in us, before considering received process data invalid */
    TRDP_TO_BEHAVIOR_T  toBehav;   /**< Behaviour when received process data is invalid/timed out. */
    TRDP_FLAGS_T        flags;     /**< TRDP_FLAGS_MARSHALL, TRDP_FLAGS_REDUNDANT */
} TRDP_PD_PAR_T;

typedef struct
{
    UINT32          confirmTimeout; /**< dataset identifier */
    UINT32          replyTimeout;  /**< Number of elements */
    TRDP_FLAGS_T    flags;         /**< Pointer to URI host part or IP */
} TRDP_MD_PAR_T;

typedef struct
{
    UINT32          id;            /**< destination identifier */
    TRDP_SDT_PAR_T  *pSdtPar;      /**< Pointer to optional SDT Parameters for this connection */
    TRDP_URI_USER_T *pUriUser;     /**< Pointer to URI user part */
    TRDP_URI_HOST_T *pUriHost;     /**< Pointer to URI host parts or IP */
} TRDP_DEST_T;

typedef struct
{
    UINT32          id;            /**< source filter identifier */
    TRDP_SDT_PAR_T  *pSdtPar;      /**< Pointer to optional SDT Parameters for this connection */
    TRDP_URI_USER_T *pUriUser;     /**< Pointer to URI user part */
    TRDP_URI_HOST_T *pUriHost1;    /**< Pointer to device URI host or IP */
    TRDP_URI_HOST_T *pUriHost2;    /**< Pointer to a second device URI host parts or IP, used eg. for red. devices */
} TRDP_SRC_T;

typedef struct
{
    UINT32          comId;        /**< source filter identifier */
    UINT32          datasetId;    /**< data set identifier */
    UINT32          comParId;     /**< communication parameter id */
    TRDP_MD_PAR_T   *pMdPar;      /**< Pointer to MD Parameters for this connection */
    TRDP_PD_PAR_T   *pPdPar;      /**< Pointer to PD Parameters for this connection */
    UINT32          destCnt;      /**< number of destinations */
    TRDP_DEST_T     *pDest;       /**< Pointer to array of destination descriptors */
    UINT32          srcCnt;       /**< number of sources */
    TRDP_SRC_T      *pSrc;        /**< Pointer to array of source descriptors */
} TRDP_EXCHG_PAR_T;

typedef struct
{
    TRDP_LABEL_T    ifName;       /**< interface name   */
    UINT8           networkId;    /**< used network on the device (1…4)   */
    TRDP_IP_ADDR_T  hostIp;       /**< host IP address   */
    TRDP_IP_ADDR_T  leaderIp;     /**< Leader IP address dependant on redundancy concept   */
} TRDP_IF_CONFIG_T;

typedef struct
{
    UINT32            id;         /**< communication parameter identifier */
    TRDP_SEND_PARAM_T sendParam;  /**< Send parameter (TTL, QoS, retries) */
} TRDP_COM_PAR_T;

/** Control for debug output format on application level.
 */
typedef enum
{
    TRDP_DBG_DEFAULT    = 0,   /**< Printout default  */
    TRDP_DBG_OFF        = 0x01, /**< Printout off  */
    TRDP_DBG_ERR        = 0x02, /**< Printout error */
    TRDP_DBG_WARN       = 0x04, /**< Printout warning and error */
    TRDP_DBG_INFO       = 0x08, /**< Printout info, warning and error */
    TRDP_DBG_DBG        = 0x10, /**< Printout debug, info, warning and error */
    TRDP_DBG_TIME       = 0x20, /**< Printout timestamp */
    TRDP_DBG_LOC        = 0x40, /**< Printout file name and line */
    TRDP_DBG_CAT        = 0x80 /**< Printout category (DBG, INFO, WARN, ERR) */
} TRDP_DBG_OPTION_T;


/** Control for debug output device/file on application level.
 */
typedef struct
{
    TRDP_DBG_OPTION_T   option;        /**< Debug printout options for application use  */
    UINT32              maxFileSize;   /**< Maximal file size  */
    TRDP_FILE_NAME_T    fileName;      /**< Debug file name and path */
} TRDP_DBG_CONFIG_T;

/** Parsed XML document handle
 */
typedef struct
{
    void               *pXmlDocument;   /**< Pointer to parsed XML document */
    void               *pRootElement;   /**< Pointer to the document root element   */
    void               *pXPathContext;  /**< Pointer to prepared XPath context  */
} TRDP_XML_DOC_HANDLE_T;


/***********************************************************************************************************************
 * PROTOTYPES
 */


/**********************************************************************************************************************/
/*    XML Configuration                                                                                                  */
/**********************************************************************************************************************/

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
    );

/**********************************************************************************************************************/
/**    Free all the memory allocated by tau_prepareXmlDoc
 *
 *
 *  @param[in]        pDocHnd           Handle of the parsed XML file
 *
 */
EXT_DECL void tau_freeXmlDoc (
    TRDP_XML_DOC_HANDLE_T   *pDocHnd
    );

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
    TRDP_COM_PAR_T             **ppComPar,
    UINT32                      *pNumIfConfig,
    TRDP_IF_CONFIG_T           **ppIfConfig
    );

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
    TRDP_EXCHG_PAR_T           **ppExchgPar
    );

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
    TRDP_COMID_DSID_MAP_T      **ppComIdDsIdMap,        
    UINT32                      *pNumDataset,
    papTRDP_DATASET_T            papDataset);


/**********************************************************************************************************************/
/**    Free array of telegram configurations allocated by tau_readXmlInterfaceConfig
 *
 *  @param[in]      numExchgPar       Number of telegram configurations in the array
 *  @param[in]      pExchgPar         Pointer to array of telegram configurations
 *
 */
EXT_DECL void tau_freeTelegrams(
    UINT32                  numExchgPar,
    TRDP_EXCHG_PAR_T       *pExchgPar);

#ifdef __cplusplus
}
#endif

#endif /* TAU_XML_H */
