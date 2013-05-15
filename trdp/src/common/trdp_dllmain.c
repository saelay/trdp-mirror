#define WIN32_LEAN_AND_MEAN             // Selten verwendete Teile der Windows-Header nicht einbinden.

#include <windows.h>
#include "trdp_if.h"
#include "trdp_private.h"
#include "vos_utils.h"

#ifdef WIN32
#ifdef DLL_EXPORT
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            const TRDP_VERSION_T *pVersion;

            /* check lib version */
            pVersion = tlc_getVersion();

            if (   (pVersion->ver != TRDP_VERSION)
                || (pVersion->rel <  TRDP_RELEASE)
               )
            {
                vos_printf( VOS_LOG_ERROR, "ERROR: Wrong version of TRDP.DLL (expected: %d.%d.%d.%d - found: %d.%d.%d.%d)\n\n",
                        TRDP_VERSION, TRDP_RELEASE, TRDP_UPDATE, TRDP_EVOLUTION,
                        (int)pVersion->ver, (int)pVersion->rel, (int)pVersion->upd, (int)pVersion->evo );

                return FALSE;
            }
        }
        break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#endif
#endif

