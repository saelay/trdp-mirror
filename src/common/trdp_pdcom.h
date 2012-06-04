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

void        trdp_pdInit(PD_ELE_T *, TRDP_MSG_T, UINT32 topCount);
void        trdp_pdUpdate (
    PD_ELE_T *);
TRDP_ERR_T  trdp_pdPut (
    PD_ELE_T *,
    TRDP_MARSHALL_T func,
    void            *refCon,
    const UINT8     *pData,
    UINT32          dataSize);
TRDP_ERR_T  trdp_pdCheck (
    PD_HEADER_T *pPacket,
    INT32       packetSize);
TRDP_ERR_T  trdp_pdSend (
    INT32   sock,
    const   PD_ELE_T *);
TRDP_ERR_T  trdp_pdGet (
    PD_ELE_T            *pPacket,
    TRDP_UNMARSHALL_T   unmarshall,
    void                *refCon,
    const UINT8         *pData,
    UINT32              dataSize);
TRDP_ERR_T trdp_pdReceive (
    TRDP_SESSION_PT pSessionHandle,
    INT32           sock);


#endif
