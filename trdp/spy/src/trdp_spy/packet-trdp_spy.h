 /* By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


/** @fn void proto_register_trdp (void)
 *
 * @brief start also analyzing TRDP packets
 *
 * Register the protocol with Wireshark
 * this format is require because a script is used to build the C function
 *  that calls all the protocol registration.
 */
void proto_register_trdp (void);


/** @fn void proto_reg_handoff_trdp(void)
 *
 * @brief Called, if the analysis of TRDP packets is stopped.
 *
 * If this dissector uses sub-dissector registration add a registration routine.
 *  This exact format is required because a script is used to find these routines
 *  and create the code that calls these routines.
 *
 *  This function is also called by preferences whenever "Apply" is pressed
 *  (see prefs_register_protocol above) so it should accommodate being called
 *  more than once.
 */
void proto_reg_handoff_trdp(void);
