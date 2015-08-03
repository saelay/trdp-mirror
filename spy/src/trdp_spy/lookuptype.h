/******************************************************************************/
/**
 * @file            lookuptype.h
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
 * @addtogroup Parsing
 * @{
 */

#ifndef TRDP_LOOKUPTYPE_H
#define TRDP_LOOKUPTYPE_H

/*******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include <glib.h>

#include "parsebody.h"
#include <glib.h>

/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

/** @fn gint trdp_lookupType(GHashTable *pTableDataset, GString* nameOfType)
 * @brief Search in the given table at the names and return the found id.
 *
 *
 *
 *  @param[in]      pTableDataset             table containing all types
 *  @param[in]      nameOfType                 textual description of a type, searching for
 *  @return         found identifier, or zero on errors
 */
gint trdp_lookupType(GHashTable *pTableDataset, GString* nameOfType);

#endif

/** @} */
