/******************************************************************************/
/**
 * @file            packet-trdp_spy.h
 *
 * @brief           Interface between Wireshark and the TRDP anaylsis module
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
 * @addtogroup Wireshark
 * @{
 */


/** @fn void proto_register_trdp (void)
 *
 * @brief start analyzing TRDP packets
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

/** @} */