/**********************************************************************************************************************/
/**
 * @file            trdp_ladder.h
 *
 * @brief           Global Variables for TRDP Ladder Topology Support
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, TOSHIBA
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright TOSHIBA, Japan, 2012.
 *
 *
 *
 */

#ifndef TRDP_LADDER_H_
#define TRDP_LADDER_H_

#ifdef TRDP_OPTION_LADDER

#include "trdp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

/* Traffic Store */
extern CHAR8 SEMAPHORE_TRAFFIC_STORE[];	    /* Traffic Store semaphore name */
extern CHAR8 TRAFFIC_STORE[];				/* Traffic Store shared memory name */
extern mode_t PERMISSION_SEMAPHORE;		    /* Traffic Store semaphore permission is rwxrwxrwx */
extern mode_t PERMISSION	;				/* Traffic Store permission is rw-rw-rw- */

extern UINT8 *pTrafficStoreAddr;			/* pointer to pointer to Traffic Store Address */

/* PDComLadderThread */
extern CHAR8 pdComLadderThreadName[];		/* Thread name is PDComLadder Thread. */
extern BOOL pdComLadderThreadActiveFlag;	/* PDComLaader Thread active/noactive Flag :active=TRUE, nonActive=FALSE */
extern BOOL pdComLadderThreadStartFlag;	/* PDComLadder Thread instruction start up Flag :start=TRUE, stop=FALSE */

/* Sub-net */
extern UINT32 usingSubnetId;				/* Using SubnetId */

#endif	/* TRDP_OPTION_LADDER */

#ifdef __cplusplus
}
#endif
#endif /* TRDP_LADDER_H_ */
