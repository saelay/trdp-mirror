/******************************************************************************/
/**
 * @file            trdp_stats.h
 *
 * @brief           Statistics for TRDP communication
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


#ifndef TRDP_STATS_H
#define TRDP_STATS_H

/*******************************************************************************
 * INCLUDES
 */

#include "trdp_if_light.h"
#include "trdp_private.h"
#include "vos_utils.h"

/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

void    trdp_initStats(TRDP_APP_SESSION_T appHandle);
void    trdp_pdPrepareStats (TRDP_APP_SESSION_T appHandle, PD_ELE_T *pPacket);


#endif
