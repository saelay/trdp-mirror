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
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
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

TRDP_ERR_T trdp_getTCPSocket (
    TRDP_SESSION_PT pSession);

void trdp_closeMDSessions(
    TRDP_SESSION_PT appHandle);

void trdp_mdFreeSession(
    MD_ELE_T    *pMDSession);

void trdp_mdSetSessionTimeout(
    MD_ELE_T    *pMDSession,
    UINT32      usTimeOut);

TRDP_ERR_T  trdp_mdSendPacket (
    INT32           pdSock,
    UINT32          port,
    MD_ELE_T        *pPacket);

void trdp_mdUpdatePacket (
    MD_ELE_T *pPacket);

TRDP_ERR_T  trdp_mdRecv (
    TRDP_SESSION_PT 	appHandle,
    UINT32           	sock/*,
    TRDP_SOCK_TYPE_T	sockType*/);

TRDP_ERR_T  trdp_mdSend (
    TRDP_SESSION_PT appHandle);

void trdp_mdCheckListenSocks (
    TRDP_SESSION_PT 	appHandle,
    TRDP_FDS_T          *pRfds,
    INT32               *pCount);

void trdp_mdCheckTimeouts (
    TRDP_SESSION_PT appHandle);

TRDP_ERR_T trdp_mdCommonSend (
                              const TRDP_MSG_T        msgType,
                              TRDP_APP_SESSION_T      appHandle,
                              const void              *pUserRef,
                              TRDP_UUID_T             *pSessionId,
                              UINT32                  comId,
                              UINT32                  topoCount,
                              TRDP_IP_ADDR_T          srcIpAddr,
                              TRDP_IP_ADDR_T          destIpAddr,
                              TRDP_FLAGS_T            pktFlags,
                              UINT16                  userStatus,
                              UINT32                  confirmTimeout,
                              UINT32                  noOfRepliers,
                              UINT32                  replyTimeout,
                              TRDP_REPLY_STATUS_T     replyStatus,
                              const TRDP_SEND_PARAM_T *pSendParam,
                              const UINT8             *pData,
                              UINT32                  dataSize,
                              const TRDP_URI_USER_T   sourceURI,
                              const TRDP_URI_USER_T   destURI);

#endif
