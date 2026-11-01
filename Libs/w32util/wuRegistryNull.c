// Added: Null version of Libs/w32util/wuRegistry.c. There is no registry on this platform,
// so the service behaves like an empty registry: Startup succeeds, every Get answers with
// the caller's default, and Save succeeds without storing. Same header contracts.
#include <j3dcore/j3d.h>

#include <string.h>

#include <w32util/wuRegistry.h>

#include <std/General/std.h>
#include <std/General/stdUtil.h>

static bool wuRegistry_bStarted = false;
static char wuRegistry_aSubKey[128] = { 0 };

int J3DAPI wuRegistry_StartupLocalMachine(const char* pSubKey)
{
    STD_ASSERTREL(!wuRegistry_bStarted);

    if ( pSubKey )
    {
        stdUtil_StringCopy(wuRegistry_aSubKey, STD_ARRAYLEN(wuRegistry_aSubKey), pSubKey);
    }

    wuRegistry_bStarted = true;
    // Returns 0 like the Windows RegCreateKeyEx success path in wuRegistry_Startup.
    return 0;
}

bool wuRegistry_HasStarted(void)
{
    return wuRegistry_bStarted;
}

void J3DAPI wuRegistry_Shutdown()
{
    wuRegistry_aSubKey[0] = '\0';
    wuRegistry_bStarted   = false;
}

int J3DAPI wuRegistry_GetInt(const char* pKey, int defaultValue)
{
    J3D_UNUSED(pKey);
    STD_ASSERTREL(wuRegistry_bStarted);
    return defaultValue;
}

float J3DAPI wuRegistry_GetFloat(const char* pKey, float defaultValue)
{
    J3D_UNUSED(pKey);
    STD_ASSERTREL(wuRegistry_bStarted);
    return defaultValue;
}

// Fixed: On a storage miss the Windows GetStr returns 1 after copying the default. The Null
// registry never stores anything, and the config-reader fallback (stdConfig_GetString)
// treats a 0 result as "value served", so 0 is returned here to keep defaults propagating
// as the QOL config JSON does. Copies the default exactly like the Windows GetStr does.
int J3DAPI wuRegistry_GetStr(const char* pKey, char* pDstStr, size_t size, const char* pDefaultValue)
{
    J3D_UNUSED(pKey);
    STD_ASSERTREL(wuRegistry_bStarted);

    if ( pDefaultValue )
    {
        stdUtil_StringCopy(pDstStr, size, pDefaultValue);
    }

    return 0;
}
