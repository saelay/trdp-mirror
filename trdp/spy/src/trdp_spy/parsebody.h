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
 @dot
digraph Reference {
node [shape=record, fontname=Helvetica, fontsize=10];
c [ label="ComId" ];
d [ label="Dataset" ];
e [ label="Element" ];
c -> d [ ];
d -> d [ ];
d -> e [ ];
}
@enddot
 *
 *
 *  @brief This struct makes a mapping between one comId and one dataset.
 *  This is a separate structure, because there could be mappings from a dataset to another one.
 *  @var ComId::comId
 *  Communication Id
 *  @var ComId::datasetId
 *  Id for a dataset (see struct ComId)
 *
 */
struct ComId {
	guint32 comId;		// used as key
	guint datasetId;
};

/** @struct Dataset
 *  @brief Description of one dataset.
 *  @var Dataset::datasetId
 *  Unique identification of one dataset
 *  @var Dataset::name
 *  Description of the dataset
 *  @var Dataset::listOfElements
 *  All elements, this dataset consists of.
 */
struct Dataset {
    guint datasetId;
	GString *name;
	GSList *listOfElements;
};

/** @struct Element
 *  @brief description of one element, with a variable that is stored
 *  @var Element::name
 *  Name of the variable, that is stored
 *  @var Element::type
 *  Numeric type of the variable (see Usermanual, chapter 4.2)
 *  @var Element::array_size
 *  Amount this value occured. 1 is default; 0 indicates a dynamic list (the dynamic list starts with a 16bit value with the occurence)
 *  @var Element::unit
 *  Unit to display
 *  @var Element::scale
 *  A factor the given value is scaled
 *  @var Element::offset
 *  Offset that is added to the values. displayed value = scale * raw value + offset
 */
struct Element {
	GString* 	name;
	guint32 	type;
	GString* 	typeName;
	guint32 array_size;
	GString* 	unit;
	gfloat 	scale;
	gint32 offset;
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

TRDP_RET_t trdp_parsebody_init(const char ** xmlconfigFile);
void trdp_parsebody_clean(void);
int trdp_parsebody_isinited( void );

struct Dataset* trdp_parsebody_lookup(guint32 comId);
struct Dataset* trdp_parsebody_search(guint32 datasetId);

#endif
