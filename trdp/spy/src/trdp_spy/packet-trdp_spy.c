/* packet-trdp_spy.c
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <epan/packet.h>
#include <epan/emem.h>
#include <epan/expert.h>
#include <epan/proto.h>
#include <epan/prefs.h>
#include <epan/conversation.h>
#include <epan/tap.h>
#include <epan/ipproto.h>
#include <epan/dissectors/packet-ip.h>
#include <epan/dissectors/packet-udp.h>
#include <epan/dissectors/packet-tcp.h>
#include <epan/strutil.h>
#include <epan/tvbuff-int.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include <locale.h>
#include "packet-trdp_spy.h"
#include "parsebody.h"
#include "trdp_env.h"

//To Debug
#ifdef PRINT_DEBUG
#define PRNT(a) a
#else
#define PRNT(a)
#endif

#define API_TRACE PRNT(printf("%s:%d : %s\n",__FILE__, __LINE__, __FUNCTION__))

/* Initialize the protocol and registered fields */
static int proto_trdp_spy = -1;
static int proto_trdp_spy_TCP = -1;

/*For All*/
static int hf_trdp_spy_sequencecounter = -1;    /*uint32*/
static int hf_trdp_spy_protocolversion = -1;    /*uint16*/
static int hf_trdp_spy_type = -1;               /*uint16*/
static int hf_trdp_spy_etb_topocount = -1;      /*uint32*/
static int hf_trdp_spy_op_trn_topocount = -1;   /*uint32*/
static int hf_trdp_spy_comid = -1;              /*uint32*/
static int hf_trdp_spy_datasetlength = -1;      /*uint16*/

/*For All (user data)*/
static int hf_trdp_spy_fcs_head = -1;           /*uint32*/
static int hf_trdp_spy_fcs_head_calc = -1;      /*uint32*/
static int hf_trdp_spy_fcs_head_data = -1;      /*uint32*/
static int hf_trdp_spy_userdata = -1;           /* userdata */

/*needed only for PD messages*/
static int hf_trdp_spy_reserved    = -1;        /*uint32*/
static int hf_trdp_spy_reply_comid = -1;        /*uint32*/   /*for MD-family only*/
static int hf_trdp_spy_reply_ipaddress = -1;    /*uint32*/
static int hf_trdp_spy_isPD = -1;               /* flag */

/* needed only for MD messages*/
static int hf_trdp_spy_replystatus = -1;        /*uint32*/
static int hf_trdp_spy_sessionid0 = -1;         /*uint32*/
static int hf_trdp_spy_sessionid1 = -1;         /*uint32*/
static int hf_trdp_spy_sessionid2 = -1;         /*uint32*/
static int hf_trdp_spy_sessionid3 = -1;         /*uint32*/
static int hf_trdp_spy_replytimeout = -1;       /*uint32*/
static int hf_trdp_spy_sourceURI = -1;          /*string*/
static int hf_trdp_spy_destinationURI = -1;     /*string*/
static int hf_trdp_spy_isMD 	= -1;           /*flag*/

static gboolean preference_changed = FALSE;

//possible PD - Substution transmission
static const value_string trdp_spy_subs_code_vals[] =
{
    {0, "No"},
    {1, "Yes"},
    {0, NULL},
};

/* Global sample preference ("controls" display of numbers) */
static const char *gbl_trdpDictionary_1 = NULL;	//XML Config Files String from ..Edit/Preference menu
static guint g_pd_port = TRDP_DEFAULT_UDP_PD_PORT;
static guint g_md_port = TRDP_DEFAULT_UDPTCP_MD_PORT;

/* Initialize the subtree pointers */
static gint ett_trdp_spy = -1;
static gint ett_trdp_spy_userdata = -1;
static gint ett_trdp_spy_app_data_fcs = -1;
static gint ett_trdp_proto_ver = -1;

/**
 * @internal
 * Compares the found CRC in a package with a calculated version
 *
 * @param tvb			dissected package
 * @param trdp_spy_tree	tree, where the information will be added as child
 * @param ref_fcs		name of the global dissect variable
 * @param ref_fcs_calc		name of the global dissect variable, when the calculation failed
 * @param offset		the offset in the package where the (32bit) CRC is stored
 * @param data_start	start where the data begins, the CRC should be calculated from
 * @param data_end		end where the data stops, the CRC should be calculated from
 * @param descr_text	description (normally "Header" or "Userdata")
 *
 * @return proto_item, that was appended
 */
static proto_item* add_crc2tree(tvbuff_t *tvb, proto_tree *trdp_spy_tree, int ref_fcs, int ref_fcs_calc, guint32 offset, guint32 data_start, guint32 data_end, const char* descr_text)
{
	guint32 calced_crc, package_crc, length;
	guint8* pBuff;
	proto_item *ti;

	// this must always fit (if not, the programmer made a big mistake -> display nothing)
	if (data_start > data_end)
		return NULL;

	length = data_end - data_start;

	pBuff = g_malloc(length);
	if (pBuff == NULL) // big problem, could not allocate the needed memory
		return NULL;

	tvb_memcpy(tvb, pBuff, data_start, length);
	calced_crc = g_ntohl(trdp_fcs32(pBuff, length,0xffffffff));

	package_crc = tvb_get_ntohl(tvb, offset);

	if (package_crc == calced_crc)
	{
		ti = proto_tree_add_uint_format(trdp_spy_tree, ref_fcs, tvb, offset, 4, calced_crc, "%sCrc: 0x%04x [correct]", descr_text, package_crc);
	}
	else
	{
		ti = proto_tree_add_uint_format(trdp_spy_tree, ref_fcs_calc, tvb, offset, 4, calced_crc, "%s Crc: 0x%04x [incorrect, should be 0x%04x]",
			descr_text, package_crc, calced_crc);
	}
	g_free(pBuff);
	return ti;
}

/* @fn *static void checkPaddingAndOffset(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 start_offset, guint32 offset)
 *
 * @brief Check for correct padding and the calculate the CRC checksum
 *
 * @param[in]   tvb     Buffer with the captured data
 * @param[in]   pinfo   Necessary to mark status of this packet
 * @param[in]   tree    The information is appended
 * @param[in]   start_offset    Beginning of the user data, that is secured by the CRC
 * @param[in]   offset  Actual offset where the padding starts
 *
 * @return position in the buffer after the body CRC
 */
static gint32 checkPaddingAndOffset(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 start_offset, guint32 offset)
{
    gint32 remainingBytes;
    gint32 isPaddingZero;
    gint32 i;

    /* Jump to the last 4 byte and check the crc */
    remainingBytes = tvb_length_remaining(tvb, offset);
    PRNT(printf("The remaining bytes are %d (startoffset=%d, padding=%d)\n", remainingBytes, start_offset, (remainingBytes % 4)));

    if (remainingBytes < 0) /* There is no space for user data */
    {
        return offset;
    }
    else if (remainingBytes > 0)
    {
        isPaddingZero = 0; // flag, if all padding bytes are zero
        for(i = 0; i < remainingBytes; i++)
        {
            if (tvb_get_guint8(tvb, offset + i) != 0)
            {
                isPaddingZero = 1;
                break;
            }
        }
        proto_tree_add_text(tree, tvb, offset, remainingBytes, ( (isPaddingZero == 0) ? "padding" : "padding not zero") );
        offset += remainingBytes;

        /* Mark this packet in the statistics also as "not perfect" */
        if (isPaddingZero != 0)
        {
            expert_add_info_format(pinfo, NULL, PI_UNDECODED, PI_WARN, "Padding not zero");
        }
    }



    return remainingBytes + TRDP_FCS_LENGTH;
}

/**
 * @internal
 * Extract all information from the userdata (uses the parsebody module for unmarshalling)
 *
 * @param tvb				buffer
 * @param packet			info for tht packet
 * @param tree				to which the information are added
 * @param trdp_spy_comid	the already extracted comId
 * @param offset			where the userdata starts in the TRDP package
 *
 * @return nothing
 */
static void dissect_trdp_body(tvbuff_t *tvb, packet_info *pinfo, proto_tree *trdp_spy_tree, guint32 trdp_spy_comid, guint32 offset, guint32 length)
{
    dissect_trdp_generic_body(tvb, pinfo, trdp_spy_tree, trdp_spy_tree, trdp_spy_comid, offset, length, 0, 0/* level of cascaded datasets*/);
}


/**
 * Explanation in its prototype @see dissect_trdp_generic_body
 */
guint32 dissect_trdp_generic_body(tvbuff_t *tvb, packet_info *pinfo, proto_tree *trdp_spy_tree, proto_tree *trdpRootNode, guint32 trdp_spy_comid, guint32 offset, guint length, guint8 flag_dataset, guint8 dataset_level)
{
	guint32 start_offset;
	struct Dataset* pFound;
	proto_tree *trdp_spy_userdata;
	proto_tree *userdata_actual;
	proto_item *ti;
	TRDP_RET_t ret;
	struct Element* el;
	guint32 array_id;
	guint32 element_amount;
	gdouble formated_value;
	GSList *gActualNode;

	gint8 value8 = 0;
	gint16 value16 = 0;
	gint32 value32 = 0;
	gint64 value64 = 0;
	guint8 value8u = 0;
	guint16 value16u = 0;
	guint32 value32u = 0;
	guint64 value64u = 0;
	gfloat real32 = 0;
	gdouble real64 = 0;
	gchar  *text = NULL;
	GTimeVal time;

	start_offset = offset; /* mark the beginning of the userdata in the package */

	/* set the local environment to a "minimum version", so the separator in numbers is always a dot. */
	setlocale(LC_ALL,"C");

    /* make the userdata accessable for wireshark */
    ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_userdata, tvb, offset, length, FALSE);

	if (strcmp(gbl_trdpDictionary_1,"") == 0  ) /* No configuration file was set */
	{
	    offset += length;
        PRNT(printf("No Configuration, %d byte of userdata -> end offset is %d\n", length, offset));
	    return checkPaddingAndOffset(tvb, pinfo, trdp_spy_tree, start_offset, offset);
	}

	if ( preference_changed || !trdp_parsebody_isinited())
    {
		PRNT(printf("TRDP dictionary is '%s' (changed=%d, init=%d)\n", gbl_trdpDictionary_1, preference_changed, trdp_parsebody_isinited()));
		//proto_tree_add_text(trdp_spy_tree, tvb, 4, 4, "Found file %s", gbl_trdpDictionary_1); //output of the configuration file

		ret = trdp_parsebody_init(&gbl_trdpDictionary_1);

		if (ret != TRDP_PARSEBODY_OK)
		{
			expert_add_info_format(pinfo, NULL, PI_UNDECODED, PI_WARN, "Configuration could not be parsed, code: %d", ret);
			PRNT(printf("Configuration could not be parsed, code: %d", ret));
			return offset;
		}
		preference_changed = 0; // don't interpret this flag each time!
    }

    // Search for the comId or the datasetId, according to the set flag.
    if (flag_dataset) {
        pFound = trdp_parsebody_search(trdp_spy_comid);
    } else {
        pFound = trdp_parsebody_lookup(trdp_spy_comid);
	}

	if (pFound == NULL) /* No Configuration for this ComId available */
	{
	    /* Move position in front of CRC (padding included) */
        offset += length;
        PRNT(printf("No Dataset, %d byte of userdata -> end offset is %d\n", length, offset));
		return checkPaddingAndOffset(tvb, pinfo, trdp_spy_tree, start_offset, offset);
	}

    if (pFound > 0)
    {
        PRNT(printf("%s aka %d\n", (pFound->name) ? pFound->name->str : "", pFound->datasetId));
    } else {
        PRNT(printf("Could not find something %s : %d\n", (flag_dataset) ? "DATASET" : "COMID", trdp_spy_comid ));
    }

	ti = proto_tree_add_text(trdp_spy_tree, tvb, offset, length, "Dataset id : %d (%s)", pFound->datasetId, (pFound->name) ? pFound->name->str : "");
	trdp_spy_userdata = proto_item_add_subtree(ti, ett_trdp_spy_userdata);

	if (pFound->listOfElements <= 0)
	{
		expert_add_info_format(pinfo, NULL, PI_UNDECODED, PI_WARN, "Userdata should be empty.");
		return offset;
	}

	gActualNode = pFound->listOfElements;
	array_id = 0;
	formated_value = 0;
	while (gActualNode != NULL)
    {
		el = (struct Element *) gActualNode->data;

        PRNT(printf("[%d, %5x] Offset %5d ----> Element: type=%2d %s\tname=%s\tarray-size=%d\tunit=%s\tscale=%f\toffset=%d\n", dataset_level, (unsigned int) gActualNode /* FIXME debug has to be removed */,
                     offset, el->type, (el->typeName) ? el->typeName->str : "", el->name->str, el->array_size, el->unit, el->scale, el->offset));

        value8u = 0; // flag, if there was a dynamic list found

		// at startup of a new item, check if it is an array or not
		if (array_id == 0)
		{
			value32 = 0; // Use this value to fetch the width in bytes of one element
			// calculate the size of one element in bytes
			value32 = trdp_dissect_width(el->type);

			element_amount = el->array_size;

			if (element_amount == 0) // handle dynamic amount of content
			{
                value8u = 1;

                /* handle the special elements CHAR8 and UTF16: */
                if (el->type == TRDP_CHAR8 || el->type == TRDP_UTF16)
                {
                    value8u = 2;

                    /* Store the maximum possible length for the dynamic datastructure */
                    value64 = tvb_length_remaining(tvb, offset) - TRDP_FCS_LENGTH;

                    /* Search for a zero to determine the end of the dynamic length field */
                    for(value32u = 0; value32u < value64; value32u++)
                    {
                        if (tvb_get_guint8(tvb, offset + value32u) == 0)
                        {
                            break; /* found the end -> quit the search */
                        }
                    }
                    element_amount = value32u + 1; /* include the padding into the element */
                    value32u = 0; /* clear the borrowed variable */

                    ti = proto_tree_add_text(trdp_spy_userdata, tvb, offset, element_amount, "%s [%d]", el->name->str, element_amount);
                    userdata_actual = proto_item_add_subtree(ti, ett_trdp_spy_userdata);
                }
                else
                {
                    //Extract the amount of element from the first two bytes (standard way */
                    element_amount = tvb_get_ntohs(tvb, offset);
                    offset += 2;

                    // check if the specified amount could be found in the package
                    value64 = tvb_length_remaining(tvb, offset + element_amount * value32);
                    if (value64 > 0 && value64 < TRDP_FCS_LENGTH /* There will be always kept space for the FCS */)
                    {
                        element_amount = 0;
                    }

                    value16u = value32 * element_amount; // length in byte of the element
                    if (value8u)
                    {
                        offset -= 2; // jump before the number of element.
                        value16u += 2;
                    }
                }

			}

            // Appand a new node in the graphical dissector, tree (also the extracted dynamic information, see above are added)
            if ((element_amount > 1  || element_amount == 0) && value8u != 2)
            {

                ti = proto_tree_add_text(trdp_spy_userdata, tvb, offset, value16u, "%s [%d]", el->name->str, element_amount);
                userdata_actual = proto_item_add_subtree(ti, ett_trdp_spy_userdata);
            }
            else if (value8u != 2) /* check, that the dissector tree was not already modified handling dynamic datatypes */
            {
                userdata_actual = trdp_spy_userdata;
            }

            if (value8u == 1)
            {
                proto_tree_add_text(userdata_actual, tvb, offset, 2, "%d elements", element_amount);
                offset += 2;

                if (value64 > 0 && value64 < TRDP_FCS_LENGTH /*There will be always kept space for the FCS*/)
                {
                    PRNT(printf("The dynamic size is too large: %s : has %d elements [%d byte each], but only %d left",
                                el->name->str, element_amount, value32, tvb_length_remaining(tvb, offset)));
                    proto_tree_add_text(userdata_actual, tvb, offset, value32, "%s : has %d elements [%d byte each], but only %d left",
                                        el->name->str, element_amount, value32, tvb_length_remaining(tvb, offset));
                }
            }

            // jump to the next frame. (this is possible, when no elements are transmitted, or on problems)
            if (element_amount  == 0)
            {
                // pick the next element
                gActualNode = g_slist_next(gActualNode);
                continue;
            }
		}

		switch(el->type)
		{
		case TRDP_BOOL8: //	   1
			value32 = tvb_get_guint8(tvb, offset);
			proto_tree_add_text(userdata_actual, tvb, offset, 1, "%s : %s", el->name->str, (value32 == 0) ? "false" : "true");
			offset += 1;
			break;
		case TRDP_CHAR8:
			text = (gchar *) tvb_get_ephemeral_string(tvb, offset, element_amount);
			proto_tree_add_text(userdata_actual, tvb, offset, element_amount, "%s : %s %s", el->name->str, text, (el->unit != 0) ? el->unit->str : "");
			offset += element_amount;
            array_id = element_amount - 1; // Jump to the next element (remove one, because this will be added automatically later)
			break;
		case TRDP_UTF16:
			text = NULL;
			text = tvb_get_unicode_string(tvb, offset, element_amount * 2, 0);
			if (text != NULL)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, element_amount * 2, "%s : %s %s", el->name->str, text, (el->unit != 0) ? el->unit->str : "");
				g_free(text);
			}
			else
			{
				proto_tree_add_text(userdata_actual, tvb, offset, element_amount * 2, "%s could not extract UTF16 character", el->name->str);
			}
			offset +=(element_amount * 2);
			array_id = element_amount - 1; // Jump to the next element (remove one, because this will be added automatically later)
			break;
		case TRDP_INT8:
			value8 = (gint8) tvb_get_guint8(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 1, "%s : %d %s", el->name->str, value8 + el->offset, (el->unit != 0) ? el->unit->str : "");
			} else {
				formated_value = (gdouble) value8; // the value will be displayed in the bottom of the loop
			}
			offset += 1;
			break;
		case TRDP_INT16:
			value16 = (gint16) tvb_get_ntohs(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 2, "%s : %d %s", el->name->str, value16, (el->unit != 0) ? el->unit->str : "");
			} else {
				formated_value = (gdouble) value16; // the value will be displayed in the bottom of the loop
			}
			offset += 2;
			break;
		case TRDP_INT32:
			value32 = (gint32) tvb_get_ntohl(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 4, "%s : %d %s", el->name->str, value32, (el->unit != 0) ? el->unit->str : "");
			} else {
				formated_value = (gdouble) value32; // the value will be displayed in the bottom of the loop
			}
			offset += 4;
			break;
		case TRDP_INT64:
			value64 = (gint64) tvb_get_ntoh64(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 8, "%s : %lld %s", el->name->str, value64, (el->unit != 0) ? el->unit->str : "");
			} else {
				formated_value = (gdouble) value64; // the value will be displayed in the bottom of the loop
			}
			offset += 8;
			break;
		case TRDP_UINT8:
			value8u = tvb_get_guint8(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 1, "%s : %d %s", el->name->str, value8u, (el->unit != 0) ? el->unit->str : "");
			} else {
				formated_value = (gdouble) value8u; // the value will be displayed in the bottom of the loop
			}
			offset += 1;
			break;
		case TRDP_UINT16:
			value16u = tvb_get_ntohs(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 2, "%s : %d %s", el->name->str, value16u, (el->unit != 0) ? el->unit->str : "");
			} else {
				formated_value = (gdouble) value16u; // the value will be displayed in the bottom of the loop
			}
			offset += 2;
			break;
		case TRDP_UINT32:
			value32u = tvb_get_ntohl(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 4, "%s : %d %s", el->name->str, value32u, (el->unit != 0) ? el->unit->str : "");
			} else {
				formated_value = (gdouble) value32u; // the value will be displayed in the bottom of the loop
			}
			offset += 4;
			break;
		case TRDP_UINT64:
			value64u = tvb_get_ntoh64(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 8, "%s : %lld %s", el->name->str, value64u, (el->unit != 0) ? el->unit->str : "");
			} else {
				formated_value = (gdouble) value64u; // the value will be displayed in the bottom of the loop
			}
			offset += 8;
			break;
		case TRDP_REAL32:
			real32 = tvb_get_ntohieee_float(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 4, "%s : %f %s", el->name->str, real32, (el->unit != 0) ? el->unit->str : "");
			} else {
				formated_value = (gdouble) real32; // the value will be displayed in the bottom of the loop
			}
			offset += 4;
			break;
		case TRDP_REAL64:
			real64 = tvb_get_ntohieee_double(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 8, "%s : %lf %s", el->name->str, real64, (el->unit != 0) ? el->unit->str : "");
			}
			else
			{
				formated_value = (gdouble) real64; // the value will be displayed in the bottom of the loop
			}
			offset += 8;
			break;
		case TRDP_TIMEDATE32:
			memset(&time, 0, sizeof(time) );
			value32u = tvb_get_ntohl(tvb, offset);
			time.tv_sec = value32u;
			proto_tree_add_text(userdata_actual, tvb, offset, 4, "%s : %s %s", el->name->str, g_time_val_to_iso8601(&time), (el->unit != 0) ? el->unit->str : "");
			offset += 4;
			break;
		case TRDP_TIMEDATE48:
			memset(&time, 0, sizeof(time) );
			value32u = tvb_get_ntohl(tvb, offset);
			value16u = tvb_get_ntohs(tvb, offset + 4);
			time.tv_sec = value32u;
			//time.tv_usec TODO how are ticks calculated to microseconds
			proto_tree_add_text(userdata_actual, tvb, offset, 6, "%s : %s %s", el->name->str, g_time_val_to_iso8601(&time), (el->unit != 0) ? el->unit->str : "");
			offset += 6;
			break;
		case TRDP_TIMEDATE64:
			memset(&time, 0, sizeof(time) );
			value32u = tvb_get_ntohl(tvb, offset);
			time.tv_sec = value32u;
			value32u = tvb_get_ntohl(tvb, offset + 4);
			time.tv_usec = value32u;
			proto_tree_add_text(userdata_actual, tvb, offset, 8, "%s : %s %s", el->name->str, g_time_val_to_iso8601(&time), (el->unit != 0) ? el->unit->str : "");
			offset += 8;
			break;
		default:
			//proto_tree_add_text(userdata_actual, tvb, offset, 1, "Unkown type %d for %s", el->type, el->name);
            PRNT(printf("Unique type %d for %s\n", el->type, el->name));

			//FIXME check the dataset_level (maximum is 5!)

			offset = dissect_trdp_generic_body(tvb, pinfo, userdata_actual, trdpRootNode, el->type, offset, length, 1, dataset_level + 1);
			element_amount = 0;
			array_id = 0;
			break;
		}

		// the display of a formated value is the same for all types, so do it here.
		if (formated_value != 0)
		{
				formated_value = (formated_value * el->scale) + el->offset;
				value16 = trdp_dissect_width(el->type); // width of the element
				proto_tree_add_text(userdata_actual, tvb, offset - value16, value16, "%s : %lf %s", el->name->str, formated_value, (el->unit != 0) ? el->unit->str : "");
		}
		formated_value=0;


        if (element_amount == 0 && array_id == 0)
        {
            // handle dynamic size of zero (only the length is set with 0, but no data is transmitted)
            // jump to the next dataset element:
            gActualNode = g_slist_next(gActualNode);
            array_id = 0;
        }
        else
        {
            // handle arrays
            array_id++;
            if (element_amount - array_id <= 0)
            {
                // pick the next element
                gActualNode = g_slist_next(gActualNode);
                array_id = 0;
            }
	}
    }

    // When there is an dataset displayed, the FCS calculation is not necessary
    if (flag_dataset)
    {
        PRNT(printf("##### Display userdata END found (level %d) #######\n", dataset_level));
        return offset;
    }

    /* Check padding and CRC of the body */
    offset = checkPaddingAndOffset(tvb, pinfo, trdpRootNode, start_offset, offset);

    PRNT(printf("##### Display ComId END found (level %d) #######\n", dataset_level));
	return offset;
}

/**
 * @internal
 * Build the special header for PD and MD datasets
 * (and calls the function to extract the userdata)
 *
 * @param tvb				buffer
 * @param pinfo				info for tht packet
 * @param tree				to which the information are added
 * @param trdp_spy_comid	the already extracted comId
 * @param offset			where the userdata starts in the TRDP package
 *
 * @return nothing
 */
static void build_trdp_tree(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
	guint32 trdp_spy_comid,	gchar* trdp_spy_string)
{
	proto_item *ti;
	proto_tree *trdp_spy_tree;
	proto_tree *proto_ver_tree;

	guint32 datasetlength;

	ti = NULL;

	API_TRACE;

	// when the package is big enough exract some data.
	if (tvb_length_remaining(tvb, 0) > TRDP_HEADER_PD_OFFSET_RESERVED)
	{
		// Fill the structure in the detail tree
		ti = proto_tree_add_item(tree, proto_trdp_spy, tvb, 0, -1, FALSE);
		trdp_spy_tree = proto_item_add_subtree(ti, ett_trdp_spy);

		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sequencecounter, tvb, TRDP_HEADER_OFFSET_SEQCNT, 4, FALSE);
		ti = proto_tree_add_text(trdp_spy_tree, tvb, 4, 2, "protocolVersion: %d.%d", tvb_get_guint8(tvb, TRDP_HEADER_OFFSET_PROTOVER), tvb_get_guint8(tvb, TRDP_HEADER_OFFSET_PROTOVER + 1));
		proto_ver_tree = proto_item_add_subtree(ti, ett_trdp_proto_ver);
		ti = proto_tree_add_item(proto_ver_tree, hf_trdp_spy_protocolversion, tvb, TRDP_HEADER_OFFSET_PROTOVER, 2, FALSE); /* add the raw version of the protocol version in a subtree */
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_type, tvb, TRDP_HEADER_OFFSET_TYPE, 2, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_comid, tvb, TRDP_HEADER_OFFSET_COMID, 4, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_etb_topocount, tvb, TRDP_HEADER_OFFSET_ETB_TOPOCNT, 4, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_op_trn_topocount, tvb, TRDP_HEADER_OFFSET_OP_TRN_TOPOCNT, 4, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_datasetlength, tvb, TRDP_HEADER_OFFSET_DATASETLENGTH, 4, FALSE);
		datasetlength = tvb_get_ntohl(tvb, TRDP_HEADER_OFFSET_DATASETLENGTH);
	}
	else
	{
        expert_add_info_format(pinfo, NULL, PI_UNDECODED, PI_ERROR, "Packet too small for header information");
	}

	if (trdp_spy_string > 0)
	{
		switch (trdp_spy_string[0])
		{
		case 'P':
			/* PD specific stuff */
			proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_reserved, tvb, TRDP_HEADER_PD_OFFSET_RESERVED, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_reply_comid, tvb, TRDP_HEADER_PD_OFFSET_REPLY_COMID, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_reply_ipaddress, tvb, TRDP_HEADER_PD_OFFSET_REPLY_IPADDR, 4, FALSE);
			ti = add_crc2tree(tvb,trdp_spy_tree, hf_trdp_spy_fcs_head, hf_trdp_spy_fcs_head_calc, TRDP_HEADER_PD_OFFSET_FCSHEAD , 0, TRDP_HEADER_PD_OFFSET_FCSHEAD, "header");
			dissect_trdp_body(tvb, pinfo, trdp_spy_tree, trdp_spy_comid, TRDP_HEADER_PD_OFFSET_DATA, datasetlength);
			break;
		case 'M':
			/* MD specific stuff */
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_replystatus, tvb, TRDP_HEADER_MD_OFFSET_REPLY_STATUS, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid0, tvb, TRDP_HEADER_MD_SESSIONID0, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid1, tvb, TRDP_HEADER_MD_SESSIONID1, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid2, tvb, TRDP_HEADER_MD_SESSIONID2, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid3, tvb, TRDP_HEADER_MD_SESSIONID3, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_replytimeout, tvb, TRDP_HEADER_MD_REPLY_TIMEOUT, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sourceURI, tvb, TRDP_HEADER_MD_SRC_URI, 32, ENC_ASCII);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_destinationURI, tvb, TRDP_HEADER_MD_DEST_URI, 32, ENC_ASCII);
			ti = add_crc2tree(tvb,trdp_spy_tree, hf_trdp_spy_fcs_head, hf_trdp_spy_fcs_head_calc, TRDP_HEADER_MD_OFFSET_FCSHEAD, 0, TRDP_HEADER_MD_OFFSET_FCSHEAD, "header");
			dissect_trdp_body(tvb, pinfo, trdp_spy_tree, trdp_spy_comid, TRDP_HEADER_MD_OFFSET_DATA, datasetlength);
			break;
		default:
			expert_add_info_format(pinfo, NULL, PI_UNDECODED, PI_WARN, "Unkown package format");
			break;
		}
	}
}

void dissect_trdp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	guint32 trdp_spy_comid = 0;
	gchar* trdp_spy_string = NULL;

	API_TRACE;

    /* Make entries in Protocol column ... */
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
    {
        col_set_str(pinfo->cinfo, COL_PROTOCOL, PROTO_TAG_TRDP);
    }

    /* and "info" column on summary display */
    if (check_col(pinfo->cinfo, COL_INFO))
    {
        col_clear(pinfo->cinfo, COL_INFO);
    }

    // Read required values from the package:
    trdp_spy_string = (gchar *) tvb_get_ephemeral_string(tvb, TRDP_HEADER_OFFSET_TYPE, 2);
    trdp_spy_comid = tvb_get_ntohl(tvb, TRDP_HEADER_OFFSET_COMID);

    /* Telegram that fits into one packet, or the header of huge telegram, that was reassembled */
    if (tree != NULL)
    {
        build_trdp_tree(tvb,pinfo,tree,trdp_spy_comid, trdp_spy_string);
    }

    /* Display a info line */
    col_set_str(pinfo->cinfo, COL_INFO, "");
    col_append_fstr(pinfo->cinfo, COL_INFO, "comId: %10d ",trdp_spy_comid);
    /* Append the packet type into the information description */
    if (check_col(pinfo->cinfo, COL_INFO))
    {
        if ((!strcmp(trdp_spy_string,"Pr")))
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, "(PD Request)");
        }
        else if ((!strcmp(trdp_spy_string,"Pp")))
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, "(PD Reply)");
        }
        else if ((!strcmp(trdp_spy_string,"Pd")))
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, "(PD Data)");
        }
        else if ((!strcmp(trdp_spy_string,"Mn")))
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, "(MD Notification (Request without reply))");
        }
        else if ((!strcmp(trdp_spy_string,"Mr")))
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, "(MD Request with reply)");
        }
        else if ((!strcmp(trdp_spy_string,"Mp")))
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, "(MD Reply (without confirmation))");
        }
        else if ((!strcmp(trdp_spy_string,"Mq")))
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, "(MD Reply (with confirmation))");
        }
        else if ((!strcmp(trdp_spy_string,"Mc")))
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, "(MD Confirm)");
        }
        else if ((!strcmp(trdp_spy_string,"Me")))
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, "(MD error)");
        }
        else
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, "Unknown TRDP Type");
            expert_add_info_format(pinfo, NULL, PI_UNDECODED, PI_WARN, "Unknown TRDP Type");
        }
    }
}

/* determine PDU length of protocol foo */

/** @fn static guint get_trdp_tcp_message_len(packet_info *pinfo, tvbuff_t *tvb, int offset)
 * @internal
 * @brief retrieve the expected size of the transmitted packet.
 */
static guint get_trdp_tcp_message_len(packet_info *pinfo, tvbuff_t *tvb, int offset)
{
    guint datasetlength = (guint) tvb_get_ntohl(tvb, offset+16);
    return datasetlength + TRDP_MD_HEADERLENGTH + TRDP_FCS_LENGTH /* add padding, FIXME must be calculated */;
}

/**
 * @internal
 * Code to analyze the actual TRDP packet, transmitted via TCP
 *
 * @param tvb       buffer
 * @param pinfo     info for the packet
 * @param tree      to which the information are added
 * @param data      Collected information
 *
 * @return nothing
 */
static void dissect_trdp_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    tcp_dissect_pdus(tvb, pinfo, tree, TRUE, TRDP_MD_HEADERLENGTH,
                     get_trdp_tcp_message_len, dissect_trdp);
}

void proto_register_trdp(void)
{
    module_t *trdp_spy_module;

    /* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] =
    {
        /* All the general fields for the header */
        { &hf_trdp_spy_sequencecounter,      { "sequenceCounter",      "trdp.sequencecounter",     FT_UINT32, BASE_DEC, NULL,   0x0, "", HFILL } },
        { &hf_trdp_spy_protocolversion,      { "protocolVersion",      "trdp.protocolversion",     FT_UINT16, BASE_HEX, NULL,   0x0, "", HFILL } },
        { &hf_trdp_spy_type,                 { "msgtype",                  "trdp.type",                FT_STRING, BASE_NONE, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_comid,                { "comId",                "trdp.comid",               FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_etb_topocount,        { "etbTopoCnt",        "trdp.etbtopocnt",          FT_UINT32, BASE_DEC, NULL,   0x0, "", HFILL } },
        { &hf_trdp_spy_op_trn_topocount,     { "opTrnTopoCnt",     "trdp.optrntopocnt",        FT_UINT32, BASE_DEC, NULL,   0x0, "", HFILL } },
        { &hf_trdp_spy_datasetlength,        { "datasetLength",        "trdp.datasetlength",       FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },

        /* PD specific stuff */
        { &hf_trdp_spy_reserved,            { "reserved",               "trdp.reserved",    FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_reply_comid,         { "replyComId",        "trdp.replycomid",  FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } }, /* only used in a PD request */
        { &hf_trdp_spy_reply_ipaddress,     { "replyIpAddress",       "trdp.replyip",     FT_IPv4, BASE_NONE, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_isPD,                { "processData",           "trdp.pd",          FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },

        /* MD specific stuff */
        { &hf_trdp_spy_replystatus,     { "replyStatus",  "trdp.replystatus",      FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_sessionid0,		{ "sessionId0",  "trdp.sessionid0",   FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_sessionid1,		{ "sessionId1",  "trdp.sessionid1",   FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_sessionid2,		{ "sessionId2",  "trdp.sessionid2",   FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_sessionid3,		{ "sessionId3",  "trdp.sessionid3",   FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_replytimeout,    { "replyTimeout",  "trdp.replytimeout",    FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_sourceURI,		{ "sourceUri",  "trdp.sourceUri",          FT_STRING, BASE_NONE, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_destinationURI,  { "destinationURI",  "trdp.destinationUri",    FT_STRING, BASE_NONE, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_isMD,      	    { "messageData",  "trdp.md",               FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
        { &hf_trdp_spy_userdata,        { "dataset",   "trdp.rawdata",         FT_BYTES, BASE_NONE, NULL, 0x0,     "", HFILL } },

        /* The checksum for the header (the trdp.fcsheadcalc is only set, if the calculated FCS differs) */
        { &hf_trdp_spy_fcs_head,             { "headerFcs",                "trdp.fcshead",     FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_fcs_head_calc,        { "calculatedHeaderFcs",     "trdp.fcsheadcalc", FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_fcs_head_data,        { "FCS (DATA)",                "trdp.fcsheaddata", FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },

    };
    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_trdp_spy,
        &ett_trdp_spy_userdata,
        &ett_trdp_spy_app_data_fcs,
        &ett_trdp_proto_ver,
    };

    PRNT(printf("\n\n"));
    API_TRACE;

    /* Register the protocol name and description */
    proto_trdp_spy = proto_register_protocol(PROTO_NAME_TRDP, PROTO_TAG_TRDP, PROTO_FILTERNAME_TRDP);

    register_dissector(PROTO_TAG_TRDP, dissect_trdp, proto_trdp_spy);

    /* Required function calls to register the header fields and subtrees used */
    proto_register_field_array(proto_trdp_spy, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    /* Register preferences module (See Section 2.6 for more on preferences) */
    trdp_spy_module = prefs_register_protocol(proto_trdp_spy, proto_reg_handoff_trdp);

    /* Register the preference */
    prefs_register_filename_preference(trdp_spy_module, "configfile",
                                     "TRDP configuration file",
                                     "TRDP configuration file",
                                     &gbl_trdpDictionary_1);
    prefs_register_uint_preference(trdp_spy_module, "pd.udp.port",
                                   "PD message Port",
                                   "UDP port for PD messages (Default port is 17224)", 10 /*base */,  &g_pd_port);
    prefs_register_uint_preference(trdp_spy_module, "md.udptcp.port",
                                   "MD message Port",
                                   "UDP and TCP port for MD messages (Default port is 17225)", 10 /*base */,  &g_md_port);
}

void proto_reg_handoff_trdp(void)
{
    static gboolean inited = FALSE;
    static dissector_handle_t trdp_spy_handle;
    static dissector_handle_t trdp_spy_TCP_handle;

    preference_changed = TRUE;

    API_TRACE;

    if(!inited )
    {
        trdp_spy_handle     = create_dissector_handle(dissect_trdp, proto_trdp_spy);
        trdp_spy_TCP_handle = create_dissector_handle(dissect_trdp_tcp, proto_trdp_spy_TCP);
        inited = TRUE;
    }
    else
    {
#if VERSION_MAJOR >= 1 && VERSION_MICRO >= 6
        dissector_delete_uint("udp.port", g_pd_port, trdp_spy_handle);
        dissector_delete_uint("udp.port", g_md_port, trdp_spy_handle);
	dissector_delete_uint("tcp.port", g_md_port, trdp_spy_TCP_handle);
#else /* old API is used */
        dissector_delete("udp.port", g_pd_port, trdp_spy_handle);
        dissector_delete("udp.port", g_md_port, trdp_spy_handle);
	dissector_delete("tcp.port", g_md_port, trdp_spy_TCP_handle);
#endif
    }
#if VERSION_MAJOR >= 1 && VERSION_MICRO >= 6
    dissector_add_uint("udp.port", g_pd_port, trdp_spy_handle);
    dissector_add_uint("udp.port", g_md_port, trdp_spy_handle);
    dissector_add_uint("tcp.port", g_md_port, trdp_spy_TCP_handle);
#else /* old API is used */
    dissector_add("udp.port", g_pd_port, trdp_spy_handle);
    dissector_add("udp.port", g_md_port, trdp_spy_handle);
    dissector_add("tcp.port", g_md_port, trdp_spy_TCP_handle);
#endif
}
