/******************************************************************************/
/**
 * @file            parsebody.c
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

/*******************************************************************************
 * INCLUDES
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

#include "lookuptype.h"

/*******************************************************************************
 * DEFINES
 */
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

/******************************************************************************
 *   Locals
 */
static GHashTable *gTableComId = NULL;
static GHashTable *gTableDataset = NULL;

/******************************************************************************
 *   Locals
 */

#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED)

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

            text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_DATASET_ID));
            id = atoi(text);
            /* the stored information is a number */
            pWorkingDataset->datasetId = id;

            text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_DATASET_NAME));
            if (text > 0)
            {
                pWorkingDataset->name = g_string_new(text);
            }
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
                el->type = atoi( text ); /* try to stored information as a number */
                if (text > 0 && el->type == 0)
                {
                    el->typeName = g_string_new(text); /* store information as text */
                }

                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_ARRAYSIZE));
                if (text > 0)
                    el->array_size = atoi( text );
                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_UNIT));
                if (text > 0)
                {
                    el->unit = g_string_new(text);
                }
                //FIXME add further attributes
                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_SCALE));
                if (text > 0)
                    el->scale = (gfloat) atof(text);
                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_OFFSET));
                if (text > 0)
                    el->offset = atoi( text );
                text = (const char*) xmlGetProp(cur, xmlCharStrdup(ATTR_NAME));
                if (text > 0)
                {
                    el->name = g_string_new(text);
                }
                pWorkingDataset->listOfElements = g_slist_append (pWorkingDataset->listOfElements, el);
            }
        }

    }
}

/**
 * Parse and store the necessary information from the XML file.
 *  @param[in]   filename: the file name to parse
 *  @return TRDP_PARSEBODY_ERR_FILE_NOT_FOUND
  * @return TRDP_PARSEBODY_ERR_PARSE_XML
  * @return TRDP_PARSEBODY_OK
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

/******************************************************************************/
/** Iterate over the dataset elements.
 *  For each item, clean its allocated memory and the item itself.
 *
 *  @param[in]      value                 actual element in the list
 *  @param[in]      user_data       pointer to aditional parameters
 */
static void visit_list_free(gpointer data, gpointer user_data)
{
    struct Element* pItem = (struct Element*) data;
    if (data > 0)
    {
        if (pItem->typeName > 0)
            g_string_free(pItem->typeName, TRUE);
        if (pItem->name > 0)
            g_string_free(pItem->name, TRUE);
        if (pItem->unit > 0)
            g_string_free(pItem->unit, TRUE);

        free(data);
    }
}

/******************************************************************************/
/** Iterate over the given hashTable containing all dataset elements.
 *  Each elements memory is freed by calling  @see visit_list_free,
  * and the item itself is also removed from the memory.
 *
 *  @param[in]      key                     of the actual item
 *  @param[in]      value                 stored information of the key
 *  @param[in]      user_data       pointer to aditional parameters
 */
static void visit_map_free(gpointer       key,
                    gpointer       value,
                    gpointer       user_data)
{

    struct Dataset* pElement;
    if (value > 0)
    {
        pElement = (struct Dataset*) value;
        if (pElement->name > 0)
        {
            g_string_free(pElement->name, TRUE);
        }
        if (pElement->listOfElements > 0)
        {
            g_slist_foreach(pElement->listOfElements, visit_list_free, NULL);
            g_slist_free(pElement->listOfElements);
        }
    }
}

/******************************************************************************/
/** Iterate over the dataset elements.
 *  For each item, that has no numeric type look this up by using the function @see  trdp_lookupType
 *
 *  @param[in]      value                 actual element in the list
 *  @param[in]      user_data       pointer to aditional parameters
 */
static void visit_convert2number_element(gpointer value, gpointer userdata)
{
    guint id;
    struct Element* pItem = (struct Element*) value;
    if (value > 0 && pItem->type == 0 && pItem->typeName != 0)
    {
        id = trdp_lookupType(gTableDataset, pItem->typeName);
#ifdef PRINT_DEBUG
        printf("Replaced %s by %d (original %d)\n", pItem->typeName->str, id, pItem->type);
#endif
        pItem->type = id;
    }
}

/******************************************************************************/
/** Iterate over the given hashTable containing all dataset elements.
 *  For each element list iterate over the items by using @see  visit_convert2number_element
 *
 *  @param[in]      key                     of the actual item
 *  @param[in]      value                 stored information of the key
 *  @param[in]      user_data       pointer to aditional parameters
 */
static void visit_convert2number(gpointer       key,
                            gpointer       value,
                            gpointer       user_data)
{
    struct Dataset* pElement;
    if (value > 0)
    {
        pElement = (struct Dataset*) value;
        if (pElement->listOfElements > 0)
        {
            g_slist_foreach(pElement->listOfElements, visit_convert2number_element, NULL);
        }
    }
}

/******************************************************************************/
/** insert a new dataset identified by its unique id.
 * Next to the identifier, also a textual description is stored in the name attribute.
 *
 *  @param[in]      id                  unique identifier
 *  @param[in]      textdescr   textual descrption
 */
static void insertStandardType(guint id, char* textdescr)
{
    // create the memory to store the custom datatype
    struct Dataset * pWorkingDataset = (struct Dataset *) g_malloc(sizeof (struct Dataset));
    memset(pWorkingDataset, 0, sizeof *pWorkingDataset);

    pWorkingDataset->datasetId = id;
    pWorkingDataset->name = g_string_new(textdescr);

#ifdef PRINT_DEBUG
    printf("%s -> %d\n", textdescr, id);
#endif
    g_hash_table_insert(gTableDataset, GINT_TO_POINTER(pWorkingDataset->datasetId), pWorkingDataset);
}

/********************* public accessable functions **********************************/


int trdp_parsebody_isinited( void )
{
    return (gTableComId > 0);
}

/** @fn TRDP_RET_t trdp_parsebody_init(const char ** xmlconfigFile)
 * @brief Create the module and extract all the needed information from the configuration file.
  * @param[in]  xmlconfigFile   path to the file containing the XML description of the TRDP packets.
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

#ifdef PRINT_DEBUG
        printf("================ Insert default types ================\n");
#endif

    /* Insert all default types */
    insertStandardType(1,  "BOOL8");
    insertStandardType(2,  "CHAR8");
    insertStandardType(3,  "UTF16");
    insertStandardType(4,  "INT8");
    insertStandardType(5,  "INT16");
    insertStandardType(6,  "INT32");
    insertStandardType(7,  "INT64");
    insertStandardType(8,  "UINT8");
    insertStandardType(9,  "UINT16");
    insertStandardType(10, "UINT32");
    insertStandardType(11, "UINT64");
    insertStandardType(12, "REAL32");
    insertStandardType(13, "REAL64");
    insertStandardType(14, "TIMEDATE32");
    insertStandardType(15, "TIMEDATE48");
    insertStandardType(16, "TIMEDATE64");

#ifdef PRINT_DEBUG
        printf("================ Convert textual identifier with the correct numerical one ================\n");
#endif

    /* convert all textual mapping of elements to numbers (to increase the performance) */
    if (gTableDataset > 0)
    {
        g_hash_table_foreach(gTableDataset, visit_convert2number, NULL);
    }

    return ret;
}

/**
 * Looks up the dataset for a given ComId.
 * @param[in] comId    to search for.
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
 * @param[in]   datasetId       the dataset we are searching for
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
    }


    return pFound;
}

/**
 * Release all the allocated memory, needed to store the given information.
 */
void trdp_parsebody_clean(void)
{
    /* Clean lookuptable for the comIds */
     if (gTableComId > 0)
    {
        g_hash_table_destroy(gTableComId);
        gTableComId = NULL;
    }

    /* Clean lookuptable for the datasets */
    if (gTableDataset > 0)
    {
        g_hash_table_foreach(gTableDataset, visit_map_free, NULL);
        g_hash_table_destroy(gTableDataset);
        gTableDataset = NULL;
    }
}
