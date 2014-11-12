/******************************************************************************/
/**
 * @file            lookuptype.c
 *
 * @brief           Functionality to find the corresponding type id for a name
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

/*******************************************************************************
 * INCLUDES
 */
#include "lookuptype.h"

/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */


/******************************************************************************
 *   Locals
 */
static GString* gName = NULL;   /** <- name to search fore */
static guint    foundId = 0;                /** <- found identifier for the searched name */


/******************************************************************************/
/** Iterate over the given hashTable containing all dataset elements.
 * There the correct name is searched and its id is stored.
 *
 *  @param[in]      key                     of the actual item
 *  @param[in]      value                 stored information of the key
 *  @param[in]      user_data       pointer to aditional parameters
 */
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

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

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
