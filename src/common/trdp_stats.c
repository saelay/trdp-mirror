/******************************************************************************/
/**
 * @file            trdp_stats.c
 *
 * @brief           Statistics functions for TRDP communication
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

/*******************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <string.h>

#include "trdp_stats.h"
#include "trdp_if_light.h"
#include "trdp_if.h"

/*******************************************************************************
 * DEFINES
 */

/*******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 *   Locals
 */

/*
   TRDP_STATISTICS_T	gStats;
 */

/******************************************************************************
 *   Globals
 */

/**********************************************************************************************************************/
/** Return statistics.
 *  Memory for statistics information will be reserved by tlc layer and needs to be freed by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[out]     ppStatistics        Statistics for this application session
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR		handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_getStatistics (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_STATISTICS_T   * *ppStatistics)
{
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*	TBD	*/
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return PD subscription statistics.
 *  Memory for statistics information will be reserved by tlc layer and needs to be freed by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[out]     pNumSubs            Pointer to the number of subscriptions
 *  @param[out]     ppStatistics         Pointer to a list with the subscription statistics information
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR		handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_getSubsStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumSubs,
    TRDP_SUBS_STATISTICS_T  * *ppStatistics)
{
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*	TBD	*/
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return PD publish statistics.
 *  Memory for statistics information will be reserved by tlc layer and needs to be freed by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[out]     pNumPub             Pointer to the number of publishers
 *  @param[out]     ppStatistics        Pointer to a list with the publish statistics information
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR		handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_getPubStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumPub,
    TRDP_PUB_STATISTICS_T   * *ppStatistics)
{
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*	TBD	*/
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return MD listener statistics.
 *  Memory for statistics information will be reserved by tlc layer and needs to be freed by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[out]     pNumList            Pointer to the number of listeners
 *  @param[out]     ppStatistics        Pointer to a list with the listener statistics information
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR		handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_getListStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumList,
    TRDP_LIST_STATISTICS_T  * *ppStatistics)
{
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*	TBD	*/
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return redundancy group statistics.
 *  Memory for statistics information will be reserved by tlc layer and needs to be freed by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[out]     pNumRed             Pointer to the number of redundancy groups
 *  @param[out]     ppStatistics        Pointer to a list with the redundancy group information
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR		handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_getRedStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumRed,
    TRDP_RED_STATISTICS_T   * *ppStatistics)
{
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*	TBD	*/
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return join statistics.
 *  Memory for statistics information will be reserved by tlc layer and needs to be freed by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[out]     pNumJoin            Pointer to the number of joined IP Adresses
 *  @param[out]     ppIpAddr            Pointer to a list with the joined IP adresses
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR		handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_getJoinStatistics (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pNumJoin,
    UINT32              * *ppIpAddr)
{
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*	TBD	*/
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Reset statistics.
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @retval         TRDP_NO_ERR	        no error
 *  @retval         TRDP_NOINIT_ERR		handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_resetStatistics (
    TRDP_APP_SESSION_T appHandle)
{
    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    memset(&appHandle->stats, 0, sizeof(appHandle->stats));

    return TRDP_NO_ERR;
}
