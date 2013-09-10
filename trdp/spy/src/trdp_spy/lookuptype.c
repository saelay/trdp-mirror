/* lookuptype.c
 *
 * Functionality to find the corresponding type id for a name
 *
 * $Id: $
 *
 * Copyright 2012, Florian Weispfenning <florian.weispfenning@de.transport.bombardier.com>
 *
 *
 */

#include "lookuptype.h"


static GString* gName = NULL;
static guint    foundId = 0;

static void visit_lookupNamedType(gpointer       key,
                            gpointer       value,
                            gpointer       user_data)
{
    struct Dataset* pElement;
    if (value > 0)
    {
        pElement = (struct Dataset*) value;
        if (g_string_equal(gName, pElement->name) == TRUE
            && 0 == foundId)
        {
            foundId = pElement->datasetId;
        }
    }
}

gint trdp_lookupType(GHashTable *pTableDataset, GString* nameOfType)
{
    /* Check, that no search is in progress */
    if (gName != NULL)
        return 0;

    gName = nameOfType;
    foundId = 0;

    g_hash_table_foreach(pTableDataset, visit_lookupNamedType, NULL);

    /* Clean the global variable after usage */
    gName = NULL;
    return foundId;
}
