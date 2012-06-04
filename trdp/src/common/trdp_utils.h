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
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define bswap_32(a)  OSSwapInt32(a)
#else
#include <byteswap.h>
#endif

#include "trdp_private.h"
#include "vos_utils.h"

/*******************************************************************************
 * DEFINES
 */

#define MAKE_LE(a)  ((am_big_endian()) ? bswap_32(a) : (a))

/*******************************************************************************
 * TYPEDEFS
 */
/*
   typedef enum trdp_log_level
   {
      TRDP_LOG_DBG	= 0x1,
      TRDP_LOG_INFO	= 0x2,
      TRDP_LOG_ERR	= 0x4
   } TRDP_LOG_T;
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

extern int          am_big_endian ();
extern TRDP_LOG_T   gDebugLevel;

PD_ELE_T            *trdp_util_getnext (
    PD_ELE_T                *pHead,
    const struct timeval    *pNow,
    PD_ELE_T                * *ppEle);
PD_ELE_T            *trdp_queue_find_comId (
    PD_ELE_T    * *pHead,
    UINT32      comId);
PD_ELE_T            *trdp_queue_find_addr (
    PD_ELE_T        *pHead,
    TRDP_ADDRESSES  *pAddr);
void                trdp_queue_del_element (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pDelete);
void                trdp_queue_app_last (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pNew);
void                trdp_queue_ins_first (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pNew);

void                trdp_initSockets(TRDP_SOCKETS_T iface[]);
TRDP_ERR_T          trdp_requestSocket(TRDP_SOCKETS_T iface[],
                                       const TRDP_SEND_PARAM_T * params,
                                       TRDP_IP_ADDR_T srcIP,
                                       TRDP_SOCK_TYPE_T usage,
                                       TRDP_OPTION_T options,
                                       INT32 * pIndex);
TRDP_ERR_T  trdp_releaseSocket(TRDP_SOCKETS_T iface[], INT32 index);

/*  Compute actual packet size from datasize    */
UINT32      trdp_packetSizePD (
    UINT32 dataSize);
void        trdp_print (
    TRDP_MSG_T  level,
    const char  *str);



#endif
