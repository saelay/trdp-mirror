/******************************************************************************/
/**
 * @file            trdp_utils.h
 *
 * @brief           Common utilities for TRDP communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
 */


#ifndef TRDP_UTILS_H
#define TRDP_UTILS_H

/*******************************************************************************
 * INCLUDES
 */

#include <stdio.h>

#include "trdp_private.h"
#include "vos_utils.h"
#include "vos_sock.h"

/*******************************************************************************
 * DEFINES
 */

#define Swap32(val)     (UINT32)(((0xFF000000 & (UINT32)val) >> 24) | \
                                 ((0x00FF0000 & (UINT32)val) >> 8)  | \
                                 ((0x0000FF00 & (UINT32)val) << 8)  | \
                                 ((0x000000FF & (UINT32)val) << 24))
#define MAKE_LE(a)      ((am_big_endian()) ? Swap32(a) : (a))

/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

extern int          am_big_endian ();
extern TRDP_LOG_T   gDebugLevel;

PD_ELE_T            *trdp_queueFindComId (
    PD_ELE_T    * *pHead,
    UINT32      comId);

PD_ELE_T            *trdp_queueFindAddr (
    PD_ELE_T        *pHead,
    TRDP_ADDRESSES  *pAddr);

void                trdp_queueDelElement (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pDelete);

void                trdp_queueAppLast (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pNew);

void                trdp_queueInsFirst (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pNew);

void                trdp_initSockets(
    TRDP_SOCKETS_T iface[]);

TRDP_ERR_T          trdp_requestSocket(
    TRDP_SOCKETS_T iface[],
    const TRDP_SEND_PARAM_T * params,
    TRDP_IP_ADDR_T srcIP,
    TRDP_SOCK_TYPE_T usage,
    TRDP_OPTION_T options,
    INT32                   * pIndex);

TRDP_ERR_T trdp_releaseSocket(
    TRDP_SOCKETS_T iface[],
    INT32 index);

/*  Compute actual packet size from datasize    */
UINT32  trdp_packetSizePD (
    UINT32 dataSize);

void    trdp_print (
    TRDP_MSG_T  level,
    const char  *str);

/*  Get initial Sequence No	*/
UINT32 trdp_getSeqCnt (
    UINT32          comID,
    TRDP_MSG_T      msgType,
    TRDP_IP_ADDR_T  srcIP);

#endif
