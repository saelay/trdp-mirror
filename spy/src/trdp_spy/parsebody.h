/* parsebody.h
 * Routines for Train Real Time Data Protocol
 * Copyright 2012, Florian Weispfenning <florian.weispfenning@de.transport.bombardier.com>
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from WHATEVER_FILE_YOU_USED (where "WHATEVER_FILE_YOU_USED"
 * is a dissector file; if you just copied this from README.developer,
 * don't bother with the "Copied from" - you don't even need to put
 * in a "Copied from" if you copied an existing dissector, especially
 * if the bulk of the code in the new dissector is your code)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <glib.h>

#ifndef TRDP_PARESEBODY_H
#define TRDP_PARESEBODY_H

/** @struct ComId
 *  @brief This struct makes a mapping between one comId and one dataset.
 *  This is a separate structure, because there could be mappings from a dataset to another one.
 *  @var ComId::comId
 *  Communication Id
 *  @var ComId::datasetId
 *  Id for a dataset (see struct ComId)
 */
struct ComId {
	guint32 comId;		// used as key
	guint datasetId;
};

/** @struct Dataset
 *  @brief Description of one dataset.
 *  @var Dataset:datsetId
 *  Unique identification of one dataset
 *  @var Dataset::name
 *  Description of the dataset
 */
struct Dataset {
    guint datasetId;
	GString *name;
	GSList *listOfElements;
};

/** @struct Element
 *  @brief description of one element, with a variable that is stored
 *  @var Element:name
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

TRDP_RET_t trdp_parsebody_init(const char ** xmlconfigFile);
void trdp_parsebody_clean(void);
int trdp_parsebody_isinited( void );

struct Dataset* trdp_parsebody_lookup(guint32 comId);
struct Dataset* trdp_parsebody_search(guint32 ComId);

#endif
