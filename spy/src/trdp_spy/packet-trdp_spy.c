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
#include <epan/proto.h>
#include <epan/prefs.h>
#include <epan/conversation.h>
#include <epan/tap.h>
#include <epan/ipproto.h>
#include <epan/dissectors/packet-ip.h>
#include <epan/dissectors/packet-udp.h>
#include <epan/strutil.h>
#include <epan/tvbuff-int.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
//#include "packet-trdp_spy.h"
#include "parsebody.h"
#include <locale.h>

#define TRDP_SPY_DEFAULT_UDPTCP_MD_PORT 20550
#define TRDP_SPY_DEFAULT_UDP_PD_PORT 20548

#define FCS_LENGTH 4

//To Debug
#ifdef PRINT_DEBUG
#define PRNT(a) a
#else
#define PRNT(a)
#endif

#define API_TRACE PRNT(printf("%s:%d : %s\n",__FILE__, __LINE__, __FUNCTION__))


/**
 * Prototyp, implementation see below.
 * Needed to set default values
 * @param dataset_level is set to 0 for the beginning
 */
static guint32 dissect_trdp_generic_body(tvbuff_t *tvb, packet_info *pinfo, proto_tree *trdp_spy_tree, guint32 trdp_spy_comid, guint32 offset, guint32 length,guint8 flag_dataset, guint8 dataset_level);

/* Forward declaration we need below */
void proto_reg_handoff_trdp(void);

/* Initialize the protocol and registered fields */
static int proto_trdp_spy = -1;

/*For All*/
static int hf_trdp_spy_sequencecounter = -1;    				/*uint32*/
static int hf_trdp_spy_protocolversion = -1;      		/*uint16*/
static int hf_trdp_spy_type = -1;   						/*uint16*/
static int hf_trdp_spy_topocount = -1;      				/*uint32*/
static int hf_trdp_spy_comid = -1;   					/*uint32*/
static int hf_trdp_spy_datasetlength = -1;    			/*uint16*/

/*For All (user data)*/
static int hf_trdp_spy_fcs_head = -1;							/*uint32*/
static int hf_trdp_spy_fcs_head_calc = -1;							/*uint32*/
static int hf_trdp_spy_data = -1;						/*dyanmic*/
static int hf_trdp_spy_padding = -1;						/*uint16*/
static int hf_trdp_spy_fcs_head_data = -1;					/*uint32*/
static int hf_trdp_spy_userdata = -1;					/* userdata */

/*needed only for PD messages*/
static int hf_trdp_spy_subs = -1;						/*uint16*/

static int hf_trdp_spy_offset_address = -1;					/*uint16*/
static int hf_trdp_spy_reply_comid = -1;					/*uint16*/   /*for MD-family only*/
static int hf_trdp_spy_reply_ipaddress = -1; 				/*uint16*/

static int hf_trdp_spy_fcs_body = -1;					/*uint8*/
static int hf_trdp_spy_fcs_body_calc = -1;				/*uint8*/
static int hf_trdp_spy_desturilen = -1;					/*uint8*/
static int hf_trdp_spy_index = -1;						/*int16*/
static int hf_trdp_spy_sequencenumber = -1;				/*uint16*/
static int hf_trdp_spy_isPD = -1;				/* flag */

/* needed only for MD messages*/
static int hf_trdp_spy_replystatus = -1;	/*uint32*/
static int hf_trdp_spy_sessionid0 = -1;		/*uint32*/
static int hf_trdp_spy_sessionid1 = -1;		/*uint32*/
static int hf_trdp_spy_sessionid2 = -1;		/*uint32*/
static int hf_trdp_spy_sessionid3 = -1;		/*uint32*/
static int hf_trdp_spy_replytimeout = -1;	/*uint32*/
static int hf_trdp_spy_sourceURI = -1;		/*string*/
static int hf_trdp_spy_destinationURI = -1; /*string*/
static int hf_trdp_spy_isMD 	= -1;				/* flag*/

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
static guint g_pd_port = TRDP_SPY_DEFAULT_UDP_PD_PORT;
static guint g_md_port = TRDP_SPY_DEFAULT_UDPTCP_MD_PORT;

/* Initialize the subtree pointers */
static gint ett_trdp_spy = -1;
static gint ett_trdp_spy_userdata = -1;
static gint ett_trdp_spy_app_data_fcs = -1;
static gint ett_trdp_proto_ver = -1;

/*
* The FCS-32 generator polynomial:
*  x**0 + x**1 + x**2 + x**4 + x**5 + x**7 + x**8 + x**10 + x**11 + x**12 + x**16 + x**22 + x**23 + x**26 + x**32.
*/
static const guint32 fcstab[256] =
{
      0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
      0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
      0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
      0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
      0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
      0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
      0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
      0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
      0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
      0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
      0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
      0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
      0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
      0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
      0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
      0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
      0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
      0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
      0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
      0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
      0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
      0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
      0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
      0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
      0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
      0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
      0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
      0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
      0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
      0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
      0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
      0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
      0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
      0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
      0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
      0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
      0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
      0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
      0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
      0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
      0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
      0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
      0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
      0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
      0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
      0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
      0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
      0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
      0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
      0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
      0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
      0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
      0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
      0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
      0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
      0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
      0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
      0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
      0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
      0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
      0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
      0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
      0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
      0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/**
 * @internal
 * Calculates and returns a 32-bit FCS.
 *
 * @param buf   Input buffer
 * @param len   Length of input buffer
 * @param fcs   Initial (seed) value for the FCS calculation
 *
 * @return Calculated fcs value
 */
static guint32 fcs32(const guint8 buf[], guint32 len, guint32 fcs)
{
    guint32 i;

    for (i=0; i < len; i++)
    {
        fcs = (fcs >> 8)^fcstab[(fcs ^ buf[i]) & 0xff];
    }
   return fcs;
}

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
	calced_crc = g_ntohl(fcs32(pBuff, length,0xffffffff));

	package_crc = tvb_get_ntohl(tvb, offset);

	if (package_crc == calced_crc)
	{
		ti = proto_tree_add_uint_format(trdp_spy_tree, ref_fcs, tvb, offset, 4, calced_crc, "%s CRC: 0x%04x [correct]", descr_text, package_crc);
	}
	else
	{
		ti = proto_tree_add_uint_format(trdp_spy_tree, ref_fcs_calc, tvb, offset, 4, calced_crc, "%s CRC: 0x%04x [incorrect, should be 0x%04x]",
			descr_text, package_crc, calced_crc);
	}
	g_free(pBuff);
	return ti;
}

/**
 * @internal
 * The width of an element in bytes.
 * Extracted from table1 at TCN-TRDP2-D-BOM-011-11.
 *
 * @param type			the numeric representation of a type
 *
 * @return the width in byte of one element of the given type
 */
static guint8 dissect_trdp_width(guint32 type)
{
	switch(type)
		{
		case 1: //BOOLEAN8	1	=UINT8, 1 bit relevant (equal to zero -> false, not equal to zero -> true)
		case 2: //CHAR8		2	char, can be used also as UTF8
		case 4: //INT8		4	Signed integer, 8 bit
		case 8: //UINT8		8	Unsigned integer, 8 bit
			return 1;
		case 3: //UTF16		3	Unicode UTF-16 character
		case 5: //INT16		5	Signed integer, 16 bit
		case 9: //UINT16		9	Unsigned integer, 16 bit
			return 2;
		case 6: //INT32		6	Signed integer, 32 bit
		case 10: //UINT32		10	Unsigned integer, 32 bit
		case 12: //REAL32		12	Floating point real, 32 bit
		case 14: //TIMEDATE32	15	32 bit UNIX time
			return 4;
		case 7: //INT64		7	Signed integer, 64 bit
		case 11: //UINT64		11	Unsigned integer, 64 bit
		case 13: //REAL64		13	Floating point real, 64 bit
		case 16: //TIMEDATE64	17	32 bit seconds and 32 bit microseconds
			return 8;
		case 15: //TIMEDATE48	16	48 bit TCN time (32 bit seconds and 16 bit ticks)
			return 6;
		default:
			return 0;
		}
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
    dissect_trdp_generic_body(tvb, pinfo, trdp_spy_tree, trdp_spy_comid, offset, length, 0, 0/* level of cascaded datasets*/);
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
 * @param flag_dataset		on 0, the comId will be searched, on > 0 trdp_spy_comid will be interpreted as a dataset id
 *
 * @return the actual offset in the package
 */
static guint32 dissect_trdp_generic_body(tvbuff_t *tvb, packet_info *pinfo, proto_tree *trdp_spy_tree, guint32 trdp_spy_comid, guint32 offset, guint length, guint8 flag_dataset, guint8 dataset_level)
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

	gint8 value8;
	gint16 value16;
	gint32 value32;
	gint64 value64;
	guint8 value8u;
	guint16 value16u;
	guint32 value32u;
	guint64 value64u;
	gfloat real32;
	gdouble real64;
	gchar  *text;
	GTimeVal time;
	
	start_offset = offset; /* mark the beginning of the userdata in the package */

	API_TRACE;

	// set the local environment to a "minimum version", so the separator in numbers is always a dot.
	setlocale(LC_ALL,"C");

	// make the userdata accessable for wireshark
	ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_userdata, tvb, offset, length, FALSE);

	if (strcmp(gbl_trdpDictionary_1,"") == 0  )
	{
		proto_tree_add_text(trdp_spy_tree, tvb, offset, length, "No Configuration available");
		/* Jump to the last 4 byte and check the crc */
		value32u = tvb_length_remaining(tvb, offset);
		PRNT(printf("The remaining is %d (startoffset=%d, padding=%d)\n", value32u, start_offset, 
				(value32u % 4)));
		if (value32u > FCS_LENGTH && value32u >= length) /* check if there is space for the header */
		{
			offset += length;
			ti = add_crc2tree(tvb,trdp_spy_tree, hf_trdp_spy_fcs_body, hf_trdp_spy_fcs_body_calc, start_offset + value32u - FCS_LENGTH, start_offset, offset, "Body");
		}
		return offset;
	}

	if ( preference_changed || !trdp_parsebody_isinited())
    {
		PRNT(printf("TRDP dictionary is '%s' (changed=%d, init=%d)\n", gbl_trdpDictionary_1, preference_changed, trdp_parsebody_isinited()));
		//proto_tree_add_text(trdp_spy_tree, tvb, 4, 4, "Found file %s", gbl_trdpDictionary_1); //output of the configuration file

		ret = trdp_parsebody_init(&gbl_trdpDictionary_1);

		if (ret != TRDP_PARSEBODY_OK)
		{
			proto_tree_add_text(trdp_spy_tree, tvb, offset, length, "Configuration could not be parsed, code: %d", ret);
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

	if (pFound == NULL)
	{
		ti = proto_tree_add_text(trdp_spy_tree, tvb, offset, -1, "The userdata could not be unmarshalled.");
		PRNT(printf("The userdata could for comId %d could not be unmarshalled.\n", trdp_spy_comid));
		/* Jump to the last 4 byte and check the crc */
		value32u = tvb_length_remaining(tvb, offset);
		PRNT(printf("The remaining is %d (startoffset=%d)\n", value32u, start_offset));
		if (value32u > FCS_LENGTH && value32u >= length)
		{
			offset += length;
			ti = add_crc2tree(tvb,trdp_spy_tree, hf_trdp_spy_fcs_body, hf_trdp_spy_fcs_body_calc, start_offset + value32u, start_offset, offset, "Body");
		}
		return offset;
	}

	ti = proto_tree_add_text(trdp_spy_tree, tvb, offset, length, "%s (dataset %d)", pFound->name, pFound->datasetId);
	trdp_spy_userdata = proto_item_add_subtree(ti, ett_trdp_spy_userdata);

	if (pFound->listOfElements <= 0)
	{
		ti = proto_tree_add_text(trdp_spy_userdata, tvb, offset, length, "Userdata should be empty.");
		return offset;
	}

	gActualNode = pFound->listOfElements;
	array_id = 0;
	formated_value = 0;
	while (gActualNode != NULL)
    {
		el = (struct Element *) gActualNode->data;

        PRNT(printf("[%d, %5x] Offset %5d ----> Element: type=%2d\tname=%s\tarray-size=%d\tunit=%s\tscale=%f\toffset=%d\n", dataset_level, gActualNode /* FIXME debug has to be removed */,
                     offset, el->type, el->name, el->array_size, el->unit, el->scale, el->offset));

        value8u = 0; // flag, if there was a dynamic list found

		// at startup of a new item, check if it is an array or not
		if (array_id == 0)
		{
			value32 = 0; // Use this value to fetch the width in bytes of one element
			//calculate the size of one element in bytes
			value32 = dissect_trdp_width(el->type);

			element_amount = el->array_size;

			if (element_amount == 0) // handle dynamic amount of content
			{
                value8u = 1;
			    //Extract the amount of element from the first two bytes
                element_amount = tvb_get_ntohs(tvb, offset);
                offset += 2;

                // check if the specified amount could be found in the package
                value64 = tvb_length_remaining(tvb, offset + element_amount * value32);
                if (value64 > 0 && value64 < FCS_LENGTH /*There will be always kept space for the FCS*/)
                {
                    element_amount = 0;
                }
			}


            value16u = value32 * element_amount; // length in byte of the element
            if (value8u)
            {
                offset -= 2; // jump before the number of element.
                value16u += 2;
            }

            // Appand a new node in the graphical dissector, tree (also the extracted dynamic information, see above are added)
            if (element_amount > 1  || element_amount == 0)
            {

                ti = proto_tree_add_text(trdp_spy_userdata, tvb, offset, value16u, "%s [%d]", el->name, element_amount);
                userdata_actual = proto_item_add_subtree(ti, ett_trdp_spy_userdata);
            } else {
                userdata_actual = trdp_spy_userdata;
            }

            if (value8u)
            {
                proto_tree_add_text(userdata_actual, tvb, offset, 2, "%d elements", element_amount);
                offset += 2;

                if (value64 > 0 && value64 < FCS_LENGTH /*There will be always kept space for the FCS*/)
                {
                    PRNT(printf("The dynamic size is too large: %s : has %d elements [%d byte each], but only %d left",
                                el->name, element_amount, value32, tvb_length_remaining(tvb, offset)));
                    proto_tree_add_text(userdata_actual, tvb, offset, value32, "%s : has %d elements [%d byte each], but only %d left",
                                        el->name, element_amount, value32, tvb_length_remaining(tvb, offset));
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
		case 1: //BOOLEAN8	1	=UINT8, 1 bit relevant (equal to zero -> false, not equal to zero -> true)
			value32 = tvb_get_guint8(tvb, offset);
			proto_tree_add_text(userdata_actual, tvb, offset, 1, "%s : %s", el->name, (value32 == 0) ? "false" : "true");
			offset += 1;
			break;
		case 2: //CHAR8		2	char, can be used also as UTF8
			text = tvb_get_ephemeral_string(tvb, offset, element_amount);
			proto_tree_add_text(userdata_actual, tvb, offset, element_amount, "%s : %s %s", el->name, text, (strlen(el->unit) > 0) ? el->unit : "");
			offset += element_amount;
            array_id = element_amount - 1; // Jump to the next element (remove one, because this will be added automatically later)
			break;
		case 3: //UTF16		3	Unicode UTF-16 character
			text = NULL;
			text = tvb_get_unicode_string(tvb, offset, element_amount * 2, 0);
			if (text != NULL)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, element_amount * 2, "%s : %s %s", el->name, text, (strlen(el->unit) > 0) ? el->unit : "");
				g_free(text);
			}
			else
			{
				proto_tree_add_text(userdata_actual, tvb, offset, element_amount * 2, "%s could not extract UTF16 character", el->name);
			}
			offset +=(element_amount * 2);
			array_id = element_amount - 1; // Jump to the next element (remove one, because this will be added automatically later)
			break;
		case 4: //INT8		4	Signed integer, 8 bit
			value8 = (gint8) tvb_get_guint8(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 1, "%s : %d %s", el->name, value8 + el->offset, (strlen(el->unit) > 0) ? el->unit : "");
			} else {
				formated_value = (gdouble) value8; // the value will be displayed in the bottom of the loop
			}
			offset += 1;
			break;
		case 5: //INT16		5	Signed integer, 16 bit
			value16 = (gint16) tvb_get_ntohs(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 2, "%s : %d %s", el->name, value16, (strlen(el->unit) > 0) ? el->unit : "");
			} else {
				formated_value = (gdouble) value16; // the value will be displayed in the bottom of the loop
			}
			offset += 2;
			break;
		case 6: //INT32		6	Signed integer, 32 bit
			value32 = (gint32) tvb_get_ntohl(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 4, "%s : %d %s", el->name, value32, (strlen(el->unit) > 0) ? el->unit : "");
			} else {
				formated_value = (gdouble) value32; // the value will be displayed in the bottom of the loop
			}
			offset += 4;
			break;
		case 7: //INT64		7	Signed integer, 64 bit
			value64 = (gint64) tvb_get_ntoh64(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 8, "%s : %lld %s", el->name, value64, (strlen(el->unit) > 0) ? el->unit : "");
			} else {
				formated_value = (gdouble) value64; // the value will be displayed in the bottom of the loop
			}
			offset += 8;
			break;
		case 8: //UINT8		8	Unsigned integer, 8 bit
			value8u = tvb_get_guint8(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 1, "%s : %d %s", el->name, value8u, (strlen(el->unit) > 0) ? el->unit : "");
			} else {
				formated_value = (gdouble) value8u; // the value will be displayed in the bottom of the loop
			}
			offset += 1;
			break;
		case 9: //UINT16		9	Unsigned integer, 16 bit
			value16u = tvb_get_ntohs(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 2, "%s : %d %s", el->name, value16u, (strlen(el->unit) > 0) ? el->unit : "");
			} else {
				formated_value = (gdouble) value16u; // the value will be displayed in the bottom of the loop
			}
			offset += 2;
			break;
		case 10: //UINT32		10	Unsigned integer, 32 bit
			value32u = tvb_get_ntohl(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 4, "%s : %d %s", el->name, value32u, (strlen(el->unit) > 0) ? el->unit : "");
			} else {
				formated_value = (gdouble) value32u; // the value will be displayed in the bottom of the loop
			}
			offset += 4;
			break;
		case 11: //UINT64		11	Unsigned integer, 64 bit
			value64u = tvb_get_ntoh64(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 8, "%s : %lld %s", el->name, value64u, (strlen(el->unit) > 0) ? el->unit : "");
			} else {
				formated_value = (gdouble) value64u; // the value will be displayed in the bottom of the loop
			}
			offset += 8;
			break;
		case 12: //REAL32		12	Floating point real, 32 bit
			real32 = tvb_get_ntohieee_float(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 4, "%s : %f %s", el->name, real32, (strlen(el->unit) > 0) ? el->unit : "");
			} else {
				formated_value = (gdouble) real32; // the value will be displayed in the bottom of the loop
			}
			offset += 4;
			break;
		case 13: //REAL64		13	Floating point real, 64 bit
			real64 = tvb_get_ntohieee_double(tvb, offset);
			if (el->scale == 0)
			{
				proto_tree_add_text(userdata_actual, tvb, offset, 8, "%s : %lf %d", el->name, real64, (strlen(el->unit) > 0) ? el->unit : "");
			}
			else
			{
				formated_value = (gdouble) real64; // the value will be displayed in the bottom of the loop
			}
			offset += 8;
			break;
		case 14: //TIMEDATE32	15	32 bit UNIX time
			memset(&time, 0, sizeof(time) );
			value32u = tvb_get_ntohl(tvb, offset);
			time.tv_sec = value32u;
			proto_tree_add_text(userdata_actual, tvb, offset, 4, "%s : %s %s", el->name, g_time_val_to_iso8601(&time), (strlen(el->unit) > 0) ? el->unit : "");
			offset += 4;
			break;
		case 15: //TIMEDATE48	16	48 bit TCN time (32 bit seconds and 16 bit ticks)
			memset(&time, 0, sizeof(time) );
			value32u = tvb_get_ntohl(tvb, offset);
			value16u = tvb_get_ntohs(tvb, offset + 4);
			time.tv_sec = value32u;
			//time.tv_usec TODO how are ticks calculated to microseconds
			proto_tree_add_text(userdata_actual, tvb, offset, 6, "%s : %s %s", el->name, g_time_val_to_iso8601(&time), (strlen(el->unit) > 0) ? el->unit : "");
			offset += 6;
			break;
		case 16: //TIMEDATE64	17	32 bit seconds and 32 bit microseconds
			memset(&time, 0, sizeof(time) );
			value32u = tvb_get_ntohl(tvb, offset);
			time.tv_sec = value32u;
			value32u = tvb_get_ntohl(tvb, offset + 4);
			time.tv_usec = value32u;
			proto_tree_add_text(userdata_actual, tvb, offset, 8, "%s : %s %s", el->name, g_time_val_to_iso8601(&time), (strlen(el->unit) > 0) ? el->unit : "");
			offset += 8;
			break;
		default:
			//proto_tree_add_text(userdata_actual, tvb, offset, 1, "Unkown type %d for %s", el->type, el->name);
            PRNT(printf("Unique type %d for %s\n", el->type, el->name));

			//FIXME check the dataset_level (maximum is 5!)

			offset = dissect_trdp_generic_body(tvb, pinfo, userdata_actual, el->type, offset, length, 1, dataset_level + 1);
			element_amount = 0;
			array_id = 0;
			break;
		}

		// the display of a formated value is the same for all types, so do it here.
		if (formated_value != 0)
		{
				formated_value = (formated_value * el->scale) + el->offset;
				value16 = dissect_trdp_width(el->type); // width of the element
				proto_tree_add_text(userdata_actual, tvb, offset - value16, value16, "%s : %lf %s", el->name, formated_value, (strlen(el->unit) > 0) ? el->unit : "");
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

	// display the padding:
	value32u = tvb_length_remaining(tvb, offset) - FCS_LENGTH;// number of padding bytes
	if (value32u > 0)
	{
		value8u = 0; // flag, if all padding bytes are zero
		for(value16u = 0; value16u < value32u; value16u++)
		{
			if (tvb_get_guint8(tvb, offset + value16u) != 0)
			{
				value8u = 1;
				break;
			}
		}
		proto_tree_add_text(trdp_spy_userdata, tvb, offset, value32u, ( (value8u == 0) ? "padding" : "padding not zero") );
		offset += value32u;
	}

	if (tvb_length_remaining(tvb, offset) != 4)
	{
		col_append_fstr(pinfo->cinfo, COL_INFO, " error at FCS for userdata ");
	}
	else
	{
	ti = add_crc2tree(tvb,trdp_spy_userdata, hf_trdp_spy_fcs_body, hf_trdp_spy_fcs_body_calc, offset, start_offset, offset - value32u /* do NOT calculate the CRC over the padding */, "Body");
	}

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
	if (tvb_length_remaining(tvb, 0) > 20) {
		// Fill the structure in the detail tree
		ti = proto_tree_add_item(tree, proto_trdp_spy, tvb, 0, -1, FALSE);
		trdp_spy_tree = proto_item_add_subtree(ti, ett_trdp_spy);

		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sequencecounter, tvb, 0, 4, FALSE);
		ti = proto_tree_add_text(trdp_spy_tree, tvb, 4, 2, "Protocol Version %d.%d", tvb_get_guint8(tvb, 4), tvb_get_guint8(tvb, 5));
		proto_ver_tree = proto_item_add_subtree(ti, ett_trdp_proto_ver);
		ti = proto_tree_add_item(proto_ver_tree, hf_trdp_spy_protocolversion, tvb, 4, 2, FALSE); /* add the raw version of the protocol version in a subtree */
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_type, tvb, 6, 2, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_comid, tvb, 8, 4, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_topocount, tvb, 12, 4, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_datasetlength, tvb, 16, 4, FALSE);
		datasetlength = tvb_get_ntohl(tvb, 16);
	}

	if (trdp_spy_string > 0)
		switch (trdp_spy_string[0])
		{
		case 'P':
			//PD specific stuff
			ti = proto_tree_add_text(trdp_spy_tree, tvb, 20, 4, "Reserved");
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_reply_comid, tvb, 24, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_reply_ipaddress, tvb, 28, 4, FALSE);
			ti = add_crc2tree(tvb,trdp_spy_tree, hf_trdp_spy_fcs_head, hf_trdp_spy_fcs_head_calc, 32, 0, 32, "Header");
			dissect_trdp_body(tvb, pinfo, trdp_spy_tree, trdp_spy_comid, 36, datasetlength);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_isPD, tvb, 6, 2, FALSE);
			break;
		case 'M':
			//MD specific stuff
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_replystatus, tvb, 20, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid0, tvb, 24, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid1, tvb, 28, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid2, tvb, 32, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid3, tvb, 36, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_replytimeout, tvb, 40, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sourceURI, tvb, 44, 32, ENC_ASCII);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_destinationURI, tvb, 76, 32, ENC_ASCII);
			ti = add_crc2tree(tvb,trdp_spy_tree, hf_trdp_spy_fcs_head, hf_trdp_spy_fcs_head_calc, 108, 0, 108, "Header");
			dissect_trdp_body(tvb, pinfo, trdp_spy_tree, trdp_spy_comid, 108 + FCS_LENGTH, datasetlength);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_isMD, tvb, 6, 2, FALSE);
			break;
		default:
			ti = proto_tree_add_text(trdp_spy_tree, tvb, 20, -1, "Unkown package format");
			break;
		}
}

/**
 * @internal
 * Code to actually dissect the packets
 *
 * @param tvb				buffer
 * @param pinfo				info for tht packet
 * @param tree				to which the information are added
 *
 * @return nothing
 */
static void dissect_trdp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	guint32 trdp_spy_comid;
	gchar* trdp_spy_string;

	trdp_spy_string = 0;
	trdp_spy_comid = 0;

	PRNT(g_print("dissect_trdp_spy started\n"));
	API_TRACE;

	// when the package is big enough, then exract some data.
	if (tvb_length_remaining(tvb, 0) > 12) {
		// Read required values from the package:
		trdp_spy_string = tvb_get_ephemeral_string(tvb, 6, 2);
		trdp_spy_comid = tvb_get_ntohl(tvb, 8);
	}

    /* Make entries in Protocol column and Info column on summary display */
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
    {
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "TRDP");
    }

    /* This field shows up as the "Info" column in the display; you should use
   it, if possible, to summarize what's in the packet, so that a user looking
   at the list of packets can tell what type of packet it is. See section 1.5
   for more information.

   Before changing the contents of a column you should make sure the column is
   active by calling "check_col(pinfo->cinfo, COL_*)". If it is not active
   don't bother setting it.

   If you are setting the column to a constant string, use "col_set_str()",
   as it's more efficient than the other "col_set_XXX()" calls.

   If you're setting it to a string you've constructed, or will be
   appending to the column later, use "col_add_str()".

   "col_add_fstr()" can be used instead of "col_add_str()"; it takes
   "printf()"-like arguments.  Don't use "col_add_fstr()" with a format
   string of "%s" - just use "col_add_str()" or "col_set_str()", as it's
   more efficient than "col_add_fstr()".

   If you will be fetching any data from the packet before filling in
   the Info column, clear that column first, in case the calls to fetch
   data from the packet throw an exception because they're fetching data
   past the end of the packet, so that the Info column doesn't have data
   left over from the previous dissector; do
*/
    if (check_col(pinfo->cinfo, COL_INFO))
    {
        col_clear(pinfo->cinfo, COL_INFO);
    }

    /***************Cisco SM Protocol********************************
 if (check_col(pinfo->cinfo, COL_INFO))
  col_add_fstr(pinfo->cinfo, COL_INFO, "trdp_spy Packet (%s)",
   val_to_str(trdp_spy_timestamp, trdp_spy_message_type_value_info,"reserved"));
 ****************************************************************/

    /* A protocol dissector can be called in 2 different ways:

 (a) Operational dissection

  In this mode, Ethereal is only interested in the way protocols
  interact, protocol conversations are created, packets are reassembled
  and handed over to higher-level protocol dissectors.
  In this mode Ethereal does not build a so-called "protocol tree".

 (b) Detailed dissection

  In this mode, Ethereal is also interested in all details of a given
  protocol, so a "protocol tree" is created.

   Ethereal distinguishes between the 2 modes with the proto_tree pointer:
 (a) <=> tree == NULL
 (b) <=> tree != NULL

   In the interest of speed, if "tree" is NULL, avoid building a
   protocol tree and adding stuff to it, or even looking at any packet
   data needed only if you're building the protocol tree, if possible.

   Note, however, that you must fill in column information, create
   conversations, reassemble packets, build any other persistent state
   needed for dissection, and call subdissectors regardless of whether
   "tree" is NULL or not.  This might be inconvenient to do without
   doing most of the dissection work; the routines for adding items to
   the protocol tree can be passed a null protocol tree pointer, in
   which case they'll return a null item pointer, and
   "proto_item_add_subtree()" returns a null tree pointer if passed a
   null item pointer, so, if you're careful not to dereference any null
   tree or item pointers, you can accomplish this by doing all the
   dissection work.  This might not be as efficient as skipping that
   work if you're not building a protocol tree, but if the code would
   have a lot of tests whether "tree" is null if you skipped that work,
   you might still be better off just doing all that work regardless of
   whether "tree" is null or not. */

	if (tree != NULL)
		build_trdp_tree(tvb,pinfo,tree,trdp_spy_comid, trdp_spy_string);


	// Display a info line
	if (check_col(pinfo->cinfo, COL_INFO))
    {
        if ((!strcmp(trdp_spy_string,"Pr")))
        {
            col_set_str(pinfo->cinfo, COL_INFO, "PD Request");
        }
        else if ((!strcmp(trdp_spy_string,"Pp")))
        {
            col_set_str(pinfo->cinfo, COL_INFO, "PD Reply");
        }
        else if ((!strcmp(trdp_spy_string,"Pd")))
        {
            col_set_str(pinfo->cinfo, COL_INFO, "PD Data");
        }
        else if ((!strcmp(trdp_spy_string,"Mn")))
        {
            col_set_str(pinfo->cinfo, COL_INFO, "MD Notificatoin");
        }
        else if ((!strcmp(trdp_spy_string,"Mr")))
        {
            col_set_str(pinfo->cinfo, COL_INFO, "MD Request with reply");
        }
        else if ((!strcmp(trdp_spy_string,"Mp")))
        {
            col_set_str(pinfo->cinfo, COL_INFO, "MD Reply (without notification)");
        }
		else if ((!strcmp(trdp_spy_string,"Mq")))
        {
            col_set_str(pinfo->cinfo, COL_INFO, "MD Reply (with notification)");
        }
		else if ((!strcmp(trdp_spy_string,"Mc")))
        {
            col_set_str(pinfo->cinfo, COL_INFO, "MD Confirm");
        }
		else if ((!strcmp(trdp_spy_string,"Me")))
        {
            col_set_str(pinfo->cinfo, COL_INFO, "MD error");
        }
        else
        {
            col_set_str(pinfo->cinfo, COL_INFO, "Unknown TRDP Type");
        }
        col_append_fstr(pinfo->cinfo, COL_INFO, "\tComid: %d",trdp_spy_comid);
    }
}

/* Register the protocol with Wireshark */

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/
void proto_register_trdp(void)
{
    module_t *trdp_spy_module;

    /* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] =
    {
	// All the general fields for the header
        { &hf_trdp_spy_sequencecounter,      { "Sequence Counter",			"trdp.sequencecounter",		FT_UINT32, BASE_DEC, NULL,   0x0, "", HFILL } },
        { &hf_trdp_spy_protocolversion,      { "Protocol Version",           "trdp.protocolversion",    FT_UINT16, BASE_HEX, NULL,   0x0, "", HFILL } },
        { &hf_trdp_spy_type,                 { "Type",                       "trdp.type",               FT_STRING, BASE_NONE, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_comid,                { "Com ID",                     "trdp.comid",              FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
        { &hf_trdp_spy_topocount,            { "Topo Count",                 "trdp.topocount",          FT_UINT32, BASE_DEC, NULL,   0x0, "", HFILL } },
        { &hf_trdp_spy_datasetlength,        { "Dataset Length",             "trdp.datasetlength",      FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },

	// PD specific stuff
        { &hf_trdp_spy_reply_comid,         { "Requested ComId",             "trdp.replycomid",         FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } }, /* only used in a PD request */
        { &hf_trdp_spy_reply_ipaddress,     { "Reply IP address",            "trdp.replyip",			FT_IPv4, BASE_NONE, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_isPD,      	    { "Process data",  "trdp.pd",    FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },

        //MD specific stuff
	{ &hf_trdp_spy_replystatus,     { "Reply Status",  "trdp.replystatus",			FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_sessionid0,		{ "Session Id UINT0",  "trdp.sessionid0",				FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_sessionid1,		{ "Session Id UINT1",  "trdp.sessionid1",				FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_sessionid2,		{ "Session Id UINT2",  "trdp.sessionid2",				FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_sessionid3,		{ "Session Id UINT3",  "trdp.sessionid3",				FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_replytimeout,    { "Reply timeout",  "trdp.replytimeout",		FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_sourceURI,		{ "Source URI",  "trdp.sourceURI",				FT_STRING, BASE_NONE, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_destinationURI,  { "Destination URI",  "trdp.destinationURI",    FT_STRING, BASE_NONE, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_isMD,      	    { "Message data",  "trdp.md",    FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
	
	{ &hf_trdp_spy_userdata,             { "Dataset",                 "trdp.rawdata",		FT_BYTES, BASE_NONE, NULL, 0x0,     "", HFILL } },

	// The checksum for the header (the trdp.fcsheadcalc is ony set, if the calculated FCS differs)
	{ &hf_trdp_spy_fcs_head,             { "header FCS",                 "trdp.fcshead",		FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_fcs_head_calc,        { "calculated header FCS",      "trdp.fcsheadcalc",	FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_fcs_head_data,             { "FCS (DATA)",        "trdp.fcsheaddata",            FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },

	// The checksum for the body (the trdp.fcsbodycalc is only set, if calcuated FCS differs)
	{ &hf_trdp_spy_fcs_body,            { "body FCS",          "trdp.fcsbody",					FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
	{ &hf_trdp_spy_fcs_body_calc,       { "body FCS",          "trdp.fcsbodycalc",					FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
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
    proto_trdp_spy = proto_register_protocol("Train Real Time Data Protocol", "TRDP", "trdp");

    register_dissector("TRDP", dissect_trdp, proto_trdp_spy); /*By Cisco*/

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
    prefs_register_uint_preference(trdp_spy_module, "pd.udp.port", "PD message Port", "UDP port for PD messages (Default port is 20548)", 10 /*base */,  &g_pd_port);
    prefs_register_uint_preference(trdp_spy_module, "md.udptcp.port", "MD message Port", "UDP and TCP port for MD messages (Default port is 20550)", 10 /*base */,  &g_md_port);

/* TRDP_SPY_DEFAULT_UDP_PD_PORT */
}

/* If this dissector uses sub-dissector registration add a registration routine.
   This exact format is required because a script is used to find these routines
   and create the code that calls these routines.

   This function is also called by preferences whenever "Apply" is pressed
   (see prefs_register_protocol above) so it should accommodate being called
   more than once.
*/
void proto_reg_handoff_trdp(void)
{
    static gboolean inited = FALSE;
    static dissector_handle_t trdp_spy_handle;

    preference_changed = TRUE;

    API_TRACE;

    if(!inited )
    {
        trdp_spy_handle = create_dissector_handle(dissect_trdp, proto_trdp_spy);
        inited = TRUE;
    }
    else
    {
        dissector_delete("udp.port", g_md_port, trdp_spy_handle);
        dissector_delete("udp.port", g_pd_port, trdp_spy_handle);
		dissector_delete("tcp.port", g_md_port, trdp_spy_handle);
    }

    dissector_add("udp.port", g_md_port, trdp_spy_handle);
    dissector_add("udp.port", g_pd_port, trdp_spy_handle);
	dissector_add("tcp.port", g_md_port, trdp_spy_handle);

    /*
          If you perform registration functions which are dependant upon
          prefs the you should de-register everything which was associated
          with the previous settings and re-register using the new prefs settings
          here. In general this means you need to keep track of what value the
          preference had at the time you registered using a local static in this
          function. ie.
          static int currentPort = -1;
          if( -1 != currentPort ) {
              dissector_delete( "tcp.port", currentPort, trdp_spy_handle);
          }
          currentPort = gPortPref;
          dissector_add("tcp.port", currentPort, trdp_spy_handle);
      */
}
