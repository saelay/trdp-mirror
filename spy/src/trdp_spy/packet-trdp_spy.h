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
 */

/**
 * @mainpage TRDP-SPY
 *
 * @section Introduction
 *
 * @subsection Purpose
 * As part of the IP-Train project, two new protocols namely TRDP-PD (Process Data) and
 * TRDP-MD (Message Data) are intended to be supported by the Wireshark tool. The support
 * is envisaged to be made available in the form of a plug-in.
 *
 * The existing GUI of the Wireshark V1.8.3 shall not be modified. The plug-in TRDP-SPY
 * shall be available as a DLL for Windows platform and shared library for TRDP-spy for Linux
 * platform.
 *
 * @subsection Intended Audience
 * The TRDP-SPY will be used primarily by TRDP Engineers.
 *
 *
 * @section Design Description
 *
 * @subsection System
 * TRDP Wire Protocol Analysis tool (TRDP-SPY) shall provide qualitative and quantitative
 * analysis of TRDP streams, in order to verify system behaviour during qualification tests (level
 * 2 and level 3) and provide help in problem analysis during train integration and debugging.
 *
 * @subsection Operational Environment
 * The plug-in shall be compatible with Windows XP and Linux implementation of Wireshark.
 * Standard behavior of Wireshark for all other protocols than WP shall not be influenced in
 * any way by the TRDPWP analysis plug-in.
 *
 * The plug-in shall be delivered as a DLL (Windows) along with the Wireshark-setup.exe and
 * shared Library (.la, .lai and .so files or Linux) along with the minimal source - Wireshark-1.8.3.
 *
 * @section Development Environment for Windows
 *
 * Following specifications are used for development of the TRDP PD and TRDP MD plug-in for Wireshark.
 * @li Operating System: Windows XP
 * @li Tool : Wireshark V1.8.3
 * @li Programming Language: C
 * @li TRDP Wire Protocol
 *
 * @subsection Steps to compile for Windows
 * Prerequisites:
 * @li Wireshark minimal source (wireshark-1.8.3.tar.bz2).
 * @li @c TRDP-SPY_src.zip source.
 * @li Follow the online guide http://www.wireshark.org/docs/wsdg_html_chunked/ChSetupWin32.html
 *
 * Steps:
 * @li Unzip @c wireshark-1.8.3.tar.bz2 to @c c:\ and rename it to @c wireshark.
 * @li Unzip @c TRDP-SPY_src.zip.
 * @li From @c TRDP-SPY_src source copy folders @ctrdp_spy to <tt>c:/wireshark/plugins</tt>.
 * @li Also copy @c config.nmake from @c TRDP-SPY to <tt>c:\\wireshark</tt> (overwrite the existing one).
 * @li Open a Terminal and navigate to the plugin location <tt>C:\\wireshark\\plugins\\trpd_spy\\</tt>.
 * @li First clean it using command <tt>nmake -f makefile.nmake distclean</tt> or run @c clean.bat.
 * @li Then compile using command <tt>nmake -f makefile.nmake</tt> or run @c build.bat.
 *
 * This will generate the @c TRDP_spy.dll.
 *
 * The Wireshark version containing TRDP can be started with: \n
 * <tt>C:\\wireshark\\wireshark-gtk2\\wireshark.exe</tt>
 *
 */


 /**
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