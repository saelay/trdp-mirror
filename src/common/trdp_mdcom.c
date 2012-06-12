/******************************************************************************/
/**
 * @file            trdp_mdcom.c
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

/*******************************************************************************
 * INCLUDES
 */

#include "trdp_utils.h"
#include "trdp_mdcom.h"


/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */


/******************************************************************************
 *   Locals
 */

static TRDP_MD_STATS_T sMDComStats;


/******************************************************************************/
/** Send MD packet
 *
 *  @param[in]      mdSock			socket descriptor
 *  @param[in]      pPacket			pointer to packet to be sent
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_UNKNOWN_ERR	error
 */
TRDP_ERR_T  trdp_sendMD (
    int             mdSock,
    const MD_ELE_T  *pPacket)
{
    TRDP_ERR_T err = TRDP_UNKNOWN_ERR;


    if (err != TRDP_NO_ERR)
    {
        vos_printf(VOS_LOG_ERROR, "trdp_sendMD failed\n");
        return TRDP_IO_ERR;
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Receive MD packet
 *
 *  @param[in]      mdSock			socket descriptor
 *  @param[out]     ppPacket		pointer to pointer to received packet
 *  @param[out]     pSize			pointer to size of received packet
 *  @param[out]     pIPAddr			pointer to source IP address of packet
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_UNKNOWN_ERR	error
 */
TRDP_ERR_T  trdp_rcvMD (
    int         mdSock,
    MD_HEADER_T * *ppPacket,
    INT32       *pSize,
    UINT32      *pIPAddr)
{
    TRDP_ERR_T err = TRDP_UNKNOWN_ERR;


    return err;
}
