/* parsebody.c
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

#include "parsebody.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


#define XPATH_EXPR			"//telegram | //element | //data-set"
#define TAG_ELEMENT			"element"
#define TAG_DATA_SET		"data-set"
#define TAG_TELEGRAM		"telegram"
#define ATTR_DATA_SET_ID	"data-set-id"
#define ATTR_COM_ID			"com-id"
#define ATTR_NAME			"name"
#define ATTR_TYPE			"type"
#define ATTR_ARRAYSIZE		"array-size"

#define ATTR_DATASET_ID		"id"
#define ATTR_DATASET_NAME	"name"
#define ATTR_UNIT			"unit"
#define ATTR_SCALE			"scale"
#define ATTR_OFFSET			"offset"
static GHashTable *gTableComId = NULL;
static GHashTable *gTableDataset = NULL;

struct SearchDataset
{
    guint32* pSearchdatasetId;
    struct Dataset* pdatasetId;
};

#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED)

void datasetId_iterator(gpointer key, gpointer value, gpointer user_data)
{
    struct SearchDataset* pSD = (struct SearchDataset *) user_data;
    struct Dataset *pElement = (struct Dataset *) value;
    if (pElement->datasetId == *(pSD->pSearchdatasetId))
    {
        pSD->pdatasetId = pElement;
        //printf("found: comId=%d (%s) datasetId=%d \t against %d\n", pElement->comId, pElement->name, pElement->datasetId, *(pSD->pSearchdatasetId));
    }
}

/**
 * processNode:
 * @reader: the xmlReader
 *
 * Dump information about the current node
 */
static void
processNodes(xmlNodeSetPtr nodes)
{
    gint i, size;
    xmlNodePtr cur;
    guint32 id;
    struct ComId *myComIdMapping = NULL;
    static struct Dataset *pWorkingDataset = NULL;
    struct Element *el;
    const char* text;

    size = (nodes) ? nodes->nodeNr : 0;
	id = 0;
	
    /* handle each result separatly */
    for(i = 0; i < size; ++i)
    {
        assert(nodes->nodeTab[i]);

        if(nodes->nodeTab[i]->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        cur = nodes->nodeTab[i];

        if (xmlStrEqual(cur->name, xmlCharStrdup(TAG_TELEGRAM)))
        {
            if (cur->properties <= 0)
                continue;
            /* Found a new comId, add that to the hash map */
            myComIdMapping = (struct ComId *) g_malloc(sizeof *myComIdMapping);
            memset(myComIdMapping, 0, sizeof *myComIdMapping);
            text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_COM_ID));
            if (text > 0)
                myComIdMapping->comId = atoi( text );
            text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_DATA_SET_ID));
            if (text > 0)
                myComIdMapping->datasetId = atoi( text );
            g_hash_table_insert(gTableComId, GINT_TO_POINTER(myComIdMapping->comId), myComIdMapping);
        }
        else if (xmlStrEqual(cur->name, xmlCharStrdup(TAG_DATA_SET)))
        {
            if (cur->properties <= 0)
                continue;
            // create the memory to store the custom datatype
            pWorkingDataset = (struct Dataset *) g_malloc(sizeof *pWorkingDataset);
            memset(pWorkingDataset, 0, sizeof *pWorkingDataset);

            id = atoi((const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_DATASET_ID)));
            pWorkingDataset->datasetId=id;
            text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_DATASET_NAME));
            if (text > 0)
                strncpy(pWorkingDataset->name, text, sizeof(pWorkingDataset->name));
            g_hash_table_insert(gTableDataset, GINT_TO_POINTER(pWorkingDataset->datasetId), pWorkingDataset);
        }
        else if (xmlStrEqual(cur->name, xmlCharStrdup(TAG_ELEMENT)))
        {
            if (pWorkingDataset > 0)
            {
                el = (struct Element *) g_malloc(sizeof *el);
                memset(el, 0, sizeof *el);
                el->array_size = 1; // default value

                /******* check if tags are present before trying to store them *************/
                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_TYPE));
                if (text > 0)
                    el->type = atoi( text );
                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_ARRAYSIZE));
                if (text > 0)
                    el->array_size = atoi( text );
                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_UNIT));
                if (text > 0)
                    strncpy(el->unit, text, sizeof(el->unit) );
                //FIXME add further attributes
                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_SCALE));
                if (text > 0)
                    el->scale = (gfloat) atof(text);
                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_OFFSET));
                if (text > 0)
                    el->offset = atoi( text );
                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_NAME));
                if (text > 0)
                    strncpy(el->name, text, sizeof(el->name) );
                pWorkingDataset->listOfElements = g_slist_append (pWorkingDataset->listOfElements, el);
            }
        }

    }
}

/**
 * streamFile:
 * @filename: the file name to parse
 * Parse and store the necessary information from the XML file.
 */
TRDP_RET_t loadXML(const char *filename)
{
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;

    LIBXML_TEST_VERSION

    /* Load XML document */
    doc = xmlParseFile(filename);
    if (doc == NULL)
    {
        return TRDP_PARSEBODY_ERR_FILE_NOT_FOUND;
    }

    /* Create xpath evaluation context */
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL)
    {
        xmlFreeDoc(doc);
        return TRDP_PARSEBODY_ERR_PARSE_XML;
    }


    /* Evaluate xpath expression */
    xpathObj = xmlXPathEvalExpression(xmlCharStrdup(XPATH_EXPR), xpathCtx);
    if(xpathObj == NULL)
    {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return TRDP_PARSEBODY_ERR_PARSE_XML;
    }

    /* Evaluate xpath expression */
    xpathObj = xmlXPathEvalExpression(xmlCharStrdup(XPATH_EXPR), xpathCtx);
    if(xpathObj == NULL)
    {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return TRDP_PARSEBODY_ERR_PARSE_XML;
    }

    /* fill the results into the hash map */
    processNodes(xpathObj->nodesetval);

    /* Cleanup */
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return TRDP_PARSEBODY_OK;
}

#else

#error "Could not find the library for XML parsing"

#endif

/********************* public access able functions **********************************/

static void visit_list_item(gpointer data, gpointer user_data)
{
    if (data > 0)
    {
        struct Element* pData = (struct Element*) data;
        printf("Element: type=%2d\tname=%s\tarray-size=%d\tunit=%s\tscale=%f\toffset=%d\n", pData->type, pData->name, pData->array_size, pData->unit, pData->scale, pData->offset);
    }
}

static void visit_list_free(gpointer data, gpointer user_data)
{
    if (data > 0)
    {
        free(data);
    }
}

void visit_map_free(gpointer       key,
                    gpointer       value,
                    gpointer       user_data)
{

    struct Dataset* pElement;
    if (value > 0)
    {
        pElement = (struct Dataset*) value;
        if (pElement->listOfElements > 0)
        {
            g_slist_foreach(pElement->listOfElements, visit_list_free, NULL);
            g_slist_free(pElement->listOfElements);
        }
    }
}

int trdp_parsebody_isinited( void )
{
    return (gTableComId > 0);
}

/**
 * Create the module and extract all the needed information from the configuration file.
 * @return TRDP_PARSEBODY_OK when no errors occured
 */
TRDP_RET_t trdp_parsebody_init(const char ** xmlconfigFile)
{
    TRDP_RET_t ret;

    // When the function is called another time, first cleanup the old stuff.
    if (gTableComId > 0)
    {
        trdp_parsebody_clean();
    }
    gTableComId = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    gTableDataset = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);

    ret = loadXML(*xmlconfigFile);
    if (ret != TRDP_PARSEBODY_OK)
        trdp_parsebody_clean(); // there were problems -> clean the internal structure
    return ret;
}

/**
 * Looks up the dataset for a given ComId.
 * @return NULL, when nothing was found.
 */
struct Dataset * trdp_parsebody_lookup(guint32 comId)
{
    struct ComId *pFound = NULL;
    gpointer mempos_found;

    if (gTableComId == NULL)
        return NULL;

    // search in the first hashmap for the corresponding comId
    mempos_found = g_hash_table_lookup(gTableComId, GINT_TO_POINTER(comId));
    if (mempos_found > 0)
    {
        //printf("We found something at the position %x\n", mempos_found);
        pFound = (struct ComId *) mempos_found;
        //FIXME DEBUG printf("comId=%d -> datasetId=%d\n", pFound->comId, pFound->datasetId);

        return trdp_parsebody_search(pFound->datasetId);
    } else {
        return NULL;
    }
}

/**
 * Uses the second hashmap to find the struct Dataset for a given datasetid
 * @param datasetId the dataset we are searching for
 * @return NULL, when nothing was found.
 */
struct Dataset* trdp_parsebody_search(guint32 datasetId)
{
    struct Dataset * pFound = NULL;
    gpointer mempos_found;

    if (gTableDataset == NULL)
        return pFound;

    mempos_found = g_hash_table_lookup(gTableDataset, GINT_TO_POINTER(datasetId));

    if (mempos_found > 0)
    {
        pFound = (struct Dataset *) mempos_found;
        //FIXME DEBUG printf("%s datasetId=%d %s\n", pFound->name, pFound->datasetId, (pFound->listOfElements > 0) ? " and a list" : "");

//        if (pFound->listOfElements > 0)
//        {
//            g_slist_foreach(pFound->listOfElements, visit_list_item, NULL);
//        }
    }


    return pFound;
}

/**
 * Release all the allocated memory, needed to store the given information.
 */
void trdp_parsebody_clean(void)
{
    if (gTableComId > 0)
    {
        g_hash_table_destroy(gTableComId);
        gTableComId = NULL;
    }


    if (gTableDataset > 0)
    {
        g_hash_table_foreach(gTableDataset, visit_map_free, NULL);
        g_hash_table_destroy(gTableDataset);
        gTableDataset = NULL;
    }
}
