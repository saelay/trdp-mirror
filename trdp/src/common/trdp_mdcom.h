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

void trdp_closeMDSessions(
    TRDP_SESSION_PT appHandle);

TRDP_ERR_T  trdp_mdSendPacket (
    INT32           pdSock,
    UINT32          port,
    const MD_ELE_T  *pPacket);

void trdp_mdUpdatePacket (
    MD_ELE_T *pPacket);

TRDP_ERR_T  trdp_mdRecv (
    TRDP_SESSION_PT appHandle,
    INT32           sock,
    MD_ELE_T        *pElement);

TRDP_ERR_T  trdp_mdSend (
    TRDP_SESSION_PT appHandle);

void trdp_mdCheckListenSocks (
    TRDP_SESSION_PT appHandle,
    TRDP_FDS_T          *pRfds,
    INT32               *pCount);

void trdp_mdCheckTimeouts (
    TRDP_SESSION_PT appHandle);

#endif
