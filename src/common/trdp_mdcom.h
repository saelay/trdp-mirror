/******************************************************************************/
/**
 * @file            trdp_mdcom.h
 *
 * @brief           Functions for MD communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
 *                     Ticket #47: Protocol change: no FCS for data part of telegrams
 */


#ifndef TRDP_MDCOM_H
#define TRDP_MDCOM_H

/*******************************************************************************
 * INCLUDES
 */

#include "trdp_private.h"

/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

TRDP_ERR_T  trdp_mdGetTCPSocket (
    TRDP_SESSION_PT pSession);

void        trdp_mdCloseSessions (
    TRDP_SESSION_PT appHandle,
    INT32 socketIndex,
    INT32 newSocket,
    BOOL8 checkAllSockets);

void        trdp_mdFreeSession (
    MD_ELE_T *pMDSession);

void        trdp_mdSetSessionTimeout (
    MD_ELE_T    *pMDSession,
    UINT32      usTimeOut);

TRDP_ERR_T  trdp_mdSendPacket (
    INT32       mdSock,
    UINT32      port,
    MD_ELE_T    *pPacket);

void        trdp_mdUpdatePacket (
    MD_ELE_T *pPacket);

TRDP_ERR_T  trdp_mdRecv (
    TRDP_SESSION_PT appHandle,
    UINT32          sock/*,
    TRDP_SOCK_TYPE_T    sockType*/);

TRDP_ERR_T  trdp_mdSend (
    TRDP_SESSION_PT appHandle);

void        trdp_mdCheckPending (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc);

void trdp_mdCheckListenSocks (
    TRDP_SESSION_PT appHandle,
    TRDP_FDS_T      *pRfds,
    INT32           *pCount);

void        trdp_mdCheckTimeouts (
    TRDP_SESSION_PT appHandle);

TRDP_ERR_T  trdp_mdCommonSend (
    const TRDP_MSG_T        msgType,
    TRDP_APP_SESSION_T      appHandle,
    const void              *pUserRef,
    TRDP_MD_CALLBACK_T      pfCbFunction,
    TRDP_UUID_T             *pSessionId,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    UINT32                  confirmTimeout,
    UINT32                  noOfRepliers,
    UINT32                  replyTimeout,
    INT32                   replyStatus,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   srcURI,
    const TRDP_URI_USER_T   destURI);

#endif
