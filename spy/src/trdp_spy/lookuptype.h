/* lookuptype.h
 *
 * Functionality to find the corresponding type id for a name
 *
 * $Id: $
 *
 * Copyright 2012, Florian Weispfenning <florian.weispfenning@de.transport.bombardier.com>
 *
 *
 */

#include <string.h>
#include <glib.h>

#ifndef TRDP_LOOKUPTYPE_H
#define TRDP_LOOKUPTYPE_H

#include "parsebody.h"
#include <glib.h>

gint trdp_lookupType(GHashTable *pTableDataset, GString* nameOfType);

#endif
