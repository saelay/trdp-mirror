/******************************************************************************/
/**
 * @file            trdp_pdcom.h
 *
 * @brief           Functions for PD communication
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


#ifndef TRDP_PDCOM_H
#define TRDP_PDCOM_H

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

void        trdp_pdInit(PD_ELE_T *, TRDP_MSG_T, UINT32 topCount, UINT32 replyComId, UINT32 replyIpAddress);
void        trdp_pdUpdate (
    PD_ELE_T *);

TRDP_ERR_T  trdp_pdPut (
    PD_ELE_T *,
    TRDP_MARSHALL_T func,
    void            *refCon,
    const UINT8     *pData,
    UINT32          dataSize);

void        trdp_pdDataUpdate (
    PD_ELE_T *pPacket);

TRDP_ERR_T  trdp_pdCheck (
    PD_HEADER_T *pPacket,
    UINT32      packetSize);

TRDP_ERR_T  trdp_pdSend (
    INT32       pdSock,
    PD_ELE_T    *pPacket,
    UINT16      port);

TRDP_ERR_T trdp_pdGet (
    PD_ELE_T            *pPacket,
    TRDP_UNMARSHALL_T   unmarshall,
    void                *refCon,
    const UINT8         *pData,
    UINT32              *pDataSize);

TRDP_ERR_T  trdp_pdSendQueued (
    TRDP_SESSION_PT appHandle);

TRDP_ERR_T  trdp_pdReceive (
    TRDP_SESSION_PT pSessionHandle,
    INT32           sock);

void        trdp_pdCheckPending (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc);

void        trdp_pdHandleTimeOuts (
    TRDP_SESSION_PT appHandle);

TRDP_ERR_T  trdp_pdCheckListenSocks (
    TRDP_SESSION_PT appHandle,
    TRDP_FDS_T      *pRfds,
    INT32           *pCount);

TRDP_ERR_T trdp_pdDistribute (
    PD_ELE_T *pSndQueue);

#endif
