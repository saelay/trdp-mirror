/**********************************************************************************************************************/
/**
 * @file            tau_xml.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details            This module provides the interface to the following utilities
 *                    - read xml configuration interpreter
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
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
/** Configuration of TRDP main process.
 */
typedef struct
{
    TRDP_LABEL_T    hostName;   /**< Host name */
    TRDP_LABEL_T    leaderName; /**< Leader name dependant on redundanca concept */
    TRDP_IP_ADDR    hostIp;     /**< Host IP address */
    TRDP_IP_ADDR    leaderIp;   /**< Leader IP address dependant on redundancy concept */
    UINT32          cycleTime;  /**< TRDP main process cycle time in usec */
    UINT32          priority;   /**< TRDP main process priority */
    TRDP_OPTION_T   options;    /**< TRDP default options */
} TRDP_PROCESS_CONFIG_T;


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
    TRDP_TO_BEHAVIOR    toBehav;   /**< Behaviour when received process data is invalid/timed out. */
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
    TRDP_MD_PAR_T   *pMdPar;      /**< Pointer to optional SDT Parameters for this connection */
    TRDP_PD_PAR_T   *pPdPar;      /**< Pointer to optional SDT Parameters for this connection */
    UINT32          destCnt;      /**< number of destinations */
    TRDP_DEST_T     *pDest;       /**< Pointer to a destination handled as list */
    UINT32          srcCnt;       /**< number of sources */
    TRDP_SRC_T      *pSrc;        /**< Pointer to a source handled as list */
} TRDP_EXCHG_PAR_T;


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
    TRDP_DEBUG_OPTION_T option;        /**< Debug printout options for application use  */
    UINT32              maxFileSize;   /**< Maximal file size  */
    TRDP_FILE_NAME_T    fileName;      /**< Debug file name and path */
}TRDP_DBG_CONFIG_T;



/***********************************************************************************************************************
 * PROTOTYPES
 */


/**********************************************************************************************************************/
/*    XML Configuration                                                                                                  */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**    Function to read the TRDP configuration parameters out of the XML configuration file.
 *
 *
 *  @param[in]      pFileName         Path and filename of the xml configuration file
 *  @param[out]     pProcessConfig    TRDP main process configuration
 *  @param[out]     pMemConfig        Memory configuration
 *  @param[out]     pPdConfig         PD default configuration
 *  @param[out]     pMdConfig         MD default configuration
 *  @param[out]     pNumExchgPar      Number of configured telegrams
 *  @param[out]     ppExchgPar        Pointer to array of telegram configurations
 *  @param[out]     pNumComPar        Number of configured com parameters
 *  @param[out]     ppComPar          Pointer to array of com parameters
 *  @param[out]     pDbgPar           Debug printout options for application use
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlConfig (
    const CHAR8             *pFileName,
    TRDP_PROCESS_CONFIG_T   *pProcessConfig,
    TRDP_MEM_CONFIG_T       *pMemConfig,
    TRDP_PD_CONFIG_T        *pPdConfig,
    TRDP_MD_CONFIG_T        *pMdConfig,
    UINT32                  *pNumExchgPar,
    TRDP_EXCHG_PAR_T        * *ppExchgPar,
    UINT32                  *pNumComPar,
    TRDP_COM_PAR_T          * *ppComPar,
    TRDP_DBG_CONFIG_T       *pDbgPar
    );


/**********************************************************************************************************************/
/**    Function to read the DataSet configuration out of the XML configuration file.
 *
 *
 *  @param[in]      pFileName         Path and filename of the xml configuration file
 *  @param[out]     pNumDataset       Pointer to the number of datasets found in the configuration
 *  @param[out]     ppDataset         Pointer to an array of a structures of type TRDP_DATASET_T
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */

EXT_DECL TRDP_ERR_T tau_readXmlDatasetConfig (
    const CHAR8     *pFileName,
    UINT32          *pNumDataset,
    TRDP_DATASET_T  * *ppDataset);



#ifdef __cplusplus
}
#endif

#endif /* TAU_XML_H */
