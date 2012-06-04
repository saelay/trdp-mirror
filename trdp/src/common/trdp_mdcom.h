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


TRDP_ERR_T  trdp_sendMD (
    int     sock,
    const   MD_ELE_T *);
TRDP_ERR_T  trdp_rcvMD (
    int         sock,
    MD_HEADER_T * *pPacket,
    ssize_t     *pSize,
    uint32_t    *pIPAddr);

#endif
