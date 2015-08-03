/******************************************************************************/
/**
 * @file            parsebody.h
 *
 * @brief           Loading of the XML description
 *
 * @details
 *
 * @note            Project: TRDP SPY
 *
 * @author          Florian Weispfenning, Bombardier Transportation
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 * @addtogroup Parsing
 * @{
 */

#ifndef TRDP_PARESEBODY_H
#define TRDP_PARESEBODY_H

/*******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include <glib.h>

/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */

/** @struct ComId
 *
 *  @brief This struct makes a mapping between one comId and one dataset.
 *
 * The following overview visualizes the relation between one comId and an element of a dataset:
 * @dot
 * digraph Reference {
 *      rankdir=LR;
 *      node [shape=record];
 *      c [ label="ComId" ];
 *      d [ label="Dataset" ];
 *      e [ label="Element" ];
 *      c -> d [ label="1..1" ];
 *      d -> d [ label="0..*" ];
 *      d -> e [ label="1..*" ];
 * }
 * @enddot
 * There is a separate structure for datasets necessary, because the dataset itself can be packed recursively into each other.
 */
struct ComId {
	guint32 comId;      /**< Communication Id, used as key*/
	guint datasetId;    /**< Id for a dataset ( @link #Dataset see Dataset structure @endlink) */
};

/** @struct Dataset
 *  @brief Description of one dataset.
 */
struct Dataset {
    guint datasetId;        /**< Unique identification of one dataset */
	GString *name;          /**< Description of the dataset */
	GSList *listOfElements; /**< All elements, this dataset consists of. */
};

/** @struct Element
 *  @brief description of one element
 *
 * All persisted information can be seen in this diagram:
 * @dot
 * digraph ERdetails {
 *      node [shape=record ];
 *      c [ label="ComId" fontsize=18 ];
 *      cdDatasetId [ label="datasetId" shape=diamond ];
 *      cComID [ label="ComId" shape=ellipse ];
 *      d [ label="Dataset" fontsize=18 ];
 *      deList [ label="listOfElements" shape=diamond ];
 *      dName [ label="name" shape=ellipse ];
 *      e [ label="Element" fontsize=18 ];
 *      eName [ label="name" shape=ellipse ];
 *      eType [ label="type" shape=ellipse ];
 *      eTypeName [ label="typeName" shape=ellipse ];
 *      eArray [ label="array_size" shape=ellipse ];
 *      eUnit [ label="unit" shape=ellipse ];
 *      eScale [ label="scale" shape=ellipse ];
 *      eOffset [ label="offset" shape=ellipse ];
 *
 *      c -> cComID [ arrowhead=none ];
 *      c -> cdDatasetId [ arrowhead=none label="1" ];
 *      cdDatasetId -> d [ arrowhead=none label="1" ];
 *      d -> deList [ arrowhead=none label="1" ];
 *      d -> dName [ arrowhead=none ];
 *      deList -> e [ arrowhead=none label="N" ];
 *      e -> eName  [ arrowhead=none ];
 *      e -> eType [ arrowhead=none ];
 *      e -> eTypeName [ arrowhead=none ];
 *      e -> eArray [ arrowhead=none ];
 *      e -> eUnit [ arrowhead=none ];
 *      e -> eScale [ arrowhead=none ];
 *      e -> eOffset [ arrowhead=none ];
 * }
 * @enddot
 */
struct Element {
	GString*    name; /**< Name of the variable, that is stored */
	guint32     type; /**< Numeric type of the variable (see Usermanual, chapter 4.2) or defined at ::TRDP_BOOL8, ::TRDP_UINT8, ::TRDP_UINT16 and so on.*/
	GString*    typeName;   /**< Textual representation of the type (necessary for own datasets, packed recursively) */
	guint32     array_size; /**< Amount this value occurred. 1 is default; 0 indicates a dynamic list (the dynamic list starts with a 16bit value with the occurrence) */
	GString*    unit;       /**< Unit to display */
	gfloat      scale;      /**< A factor the given value is scaled */
	gint32      offset;     /**< Offset that is added to the values. displayed value = scale * raw value + offset */
};

enum TRDP_RET {
	TRDP_PARSEBODY_OK = 0,
	TRDP_PARSEBODY_ERR,
	TRDP_PARSEBODY_ERR_IO_ERROR,
	TRDP_PARSEBODY_ERR_PARSE_XML,
	TRDP_PARSEBODY_ERR_FILE_NOT_FOUND,
};

typedef enum TRDP_RET TRDP_RET_t;

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */


/** @fn TRDP_RET_t trdp_parsebody_init(const char ** xmlconfigFile)
 * @brief Create the module and extract all the needed information from the configuration file.
 * @param[in]  xmlconfigFile   path to the file containing the XML description of the TRDP packets.
 * @return TRDP_PARSEBODY_OK when no errors occured
 */
TRDP_RET_t trdp_parsebody_init(const char ** xmlconfigFile);


/** @fn void trdp_parsebody_clean(void)
 * @brief Clean used memory
 *
 * Release all the allocated memory, needed to store the given information.
 * @return nothing
 */
void trdp_parsebody_clean(void);

/** @fn int trdp_parsebody_isinited( void )
 * Show the status, if the library is ready to use.
 * @return > 0 if the library is initialized, 0 if uninitialized
 */
int trdp_parsebody_isinited( void );

/** @fn struct Dataset* trdp_parsebody_lookup(guint32 comId)
 * Looks up the dataset for a given ComId.
 * @param[in] comId    to search for.
 * @return NULL, when nothing was found.
 */
struct Dataset* trdp_parsebody_lookup(guint32 comId);

/** @fn struct Dataset* trdp_parsebody_search(guint32 datasetId)
 * Uses the second hashmap to find the struct Dataset for a given datasetid
 * @param[in]   datasetId       the dataset we are searching for
 * @return NULL, when nothing was found.
 */
struct Dataset* trdp_parsebody_search(guint32 datasetId);

#endif

/** @} */
