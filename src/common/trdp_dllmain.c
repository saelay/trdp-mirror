/**********************************************************************************************************************/
/**
 * @file            trdp_dllmain.c
 *
 * @brief           Windows DLL main function
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss, Bombardier
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */
#ifdef WIN32
#ifdef DLL_EXPORT

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "trdp_if.h"
#include "trdp_private.h"
#include "vos_utils.h"


/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#endif
#endif

