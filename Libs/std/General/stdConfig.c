#include "stdConfig.h"

#include <std/General/std.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdJSON.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <w32util/wuRegistry.h>

#include <string.h>

#define STDCONFIG_VERSION "1.0.0"

static bool stdConfig_bStarted = false;
static StdJSONHandle stdConfig_hJson = NULL;
static tHashTable* stdConfig_pKeyMappingTable = NULL;

// Internal helper functions
bool J3DAPI stdConfig_AddRegKeyMapping(const char* pConfigKey, const char* pRegistryKey);
static const char* stdConfig_GetRegistryKey(const char* pConfigKey);

void stdConfig_InitRegKeyMappings(void)
{
    // Map default key translation to registry keys
    stdConfig_AddRegKeyMapping("installPath", "Install Path");
    stdConfig_AddRegKeyMapping("sourcePath", "Source Dir");
    stdConfig_AddRegKeyMapping("graphics.allDevices", "AllDevices");
    stdConfig_AddRegKeyMapping("sound.volume", "Sound Volume");
    stdConfig_AddRegKeyMapping("gameplay.startLevel", "StartLevel");
    stdConfig_AddRegKeyMapping("graphics.window", "InWindow");
    stdConfig_AddRegKeyMapping("graphics.dualMonitor", "Dual Monitor");
    stdConfig_AddRegKeyMapping("graphics.buffering", "Buffering");
    stdConfig_AddRegKeyMapping("graphics.mipmapFilter", "Filter");
    stdConfig_AddRegKeyMapping("engine.renderer.fog.enabled", "Fog");
    stdConfig_AddRegKeyMapping("engine.renderer.fog.density", "Fog Density");
    stdConfig_AddRegKeyMapping("devMode", "DevMode");
    stdConfig_AddRegKeyMapping("startMode", "Start Mode");
    stdConfig_AddRegKeyMapping("log.mode", "Debug Mode");
    stdConfig_AddRegKeyMapping("log.level", "Verbosity");
    stdConfig_AddRegKeyMapping("graphics.performanceLevel", "Performance Level");
    stdConfig_AddRegKeyMapping("engine.renderer.geometry", "Geometry Mode");
    stdConfig_AddRegKeyMapping("engine.renderer.lighting", "Lighting Mode");
    stdConfig_AddRegKeyMapping("engine.renderer.hipoly", "HiPoly");
    stdConfig_AddRegKeyMapping("graphics.display", "Display");
    stdConfig_AddRegKeyMapping("graphics.device", "3D Device");
    stdConfig_AddRegKeyMapping("graphics.width", "Width");
    stdConfig_AddRegKeyMapping("graphics.height", "Height");
    stdConfig_AddRegKeyMapping("graphics.bpp", "bpp");
    stdConfig_AddRegKeyMapping("graphics.refreshRate", "Refresh Rate");
    stdConfig_AddRegKeyMapping("sound.hw", "Sound 3D");
    stdConfig_AddRegKeyMapping("sound.reverse", "ReverseSound");
    stdConfig_AddRegKeyMapping("gameplay.defaultRun", "Default Run");
    stdConfig_AddRegKeyMapping("controls.configFile", "Configuration");
    stdConfig_AddRegKeyMapping("controls.mouse", "Mouse Control");
    stdConfig_AddRegKeyMapping("controls.controller", "Joystick Control");
    stdConfig_AddRegKeyMapping("gameplay.showText", "Show Text");
    stdConfig_AddRegKeyMapping("gameplay.showHints", "Show Hints");
    stdConfig_AddRegKeyMapping("gameplay.mapRotation", "Map Rotation");
    stdConfig_AddRegKeyMapping("gameplay.difficulty", "Difficulty");
    stdConfig_AddRegKeyMapping("gameplay.lastSaveGame", "Last Save Game");
}

bool J3DAPI stdConfig_Startup(const char* pConfigPath)
{
    if ( stdConfig_bStarted )
    {
        STDLOG_WARNING("stdConfig_Startup: Module already started");
        return true;
    }

    if ( !pConfigPath )
    {
        STDLOG_ERROR("stdConfig_Startup: Invalid config path");
        return false;
    }

    // Check dependencies
    if ( !stdJSON_HasStarted() || !wuRegistry_HasStarted() )
    {
        STDLOG_ERROR("stdConfig_Startup: stdJSON or wuRegistry module not started");
        return false;
    }

    // Create key mapping hash table
    stdConfig_pKeyMappingTable = stdHashtbl_New(64); // Start with 64 slots
    if ( !stdConfig_pKeyMappingTable )
    {
        STDLOG_ERROR("stdConfig_Startup: Failed to create key mapping table");
        goto error;
    }

    stdConfig_InitRegKeyMappings();

    // Try to load existing JSON config file
    if ( stdUtil_FileExists(pConfigPath) )
    {
        stdConfig_hJson = stdJSON_Load(pConfigPath);
        if ( !stdConfig_hJson )
        {
            STDLOG_ERROR("stdConfig_Startup: Failed to load JSON config from '%s'", pConfigPath);
            goto error;
        }
    }
    else
    {
        stdConfig_hJson = stdJSON_New();
        if ( !stdConfig_hJson )
        {
            STDLOG_ERROR("stdConfig_Startup: Failed to create JSON config");
            goto error;
        }

        if ( !stdJSON_SetString(stdConfig_hJson, "version", STDCONFIG_VERSION) )
        {
            STDLOG_ERROR("stdConfig_Startup: Failed to set config version");
            goto error;
        }

        // Save new empty config to file
        if ( !stdJSON_Save(stdConfig_hJson, pConfigPath) )
        {
            STDLOG_ERROR("stdConfig_Startup: Failed to save new JSON config to '%s'", pConfigPath);
            goto error;
        }
    }

    // Enable auto-save for JSON config
    stdJSON_SetAutoSave(stdConfig_hJson, true);

    stdConfig_bStarted = true;
    STDLOG_DEBUG("stdConfig_Startup: Module started successfully");
    return true;

error:
    if ( stdConfig_hJson )
    {
        // Fixed: Release a partially initialized JSON config when startup fails.
        stdJSON_Free(stdConfig_hJson);
        stdConfig_hJson = NULL;
    }

    if ( stdConfig_pKeyMappingTable )
    {
        stdHashtbl_Free(stdConfig_pKeyMappingTable);
        stdConfig_pKeyMappingTable = NULL;
    }

    return false;
}

bool stdConfig_HasStarted(void)
{
    return stdConfig_bStarted;
}

void J3DAPI stdConfig_Shutdown(void)
{
    if ( !stdConfig_bStarted )
    {
        return;
    }

    // Save config before shutdown
    if ( stdConfig_hJson )
    {
        stdJSON_Save(stdConfig_hJson, NULL);
    }

    // Free JSON handle
    if ( stdConfig_hJson )
    {
        stdJSON_Free(stdConfig_hJson);
        stdConfig_hJson = NULL;
    }

    // Free key mapping table
    if ( stdConfig_pKeyMappingTable )
    {
        stdHashtbl_Free(stdConfig_pKeyMappingTable);
        stdConfig_pKeyMappingTable = NULL;
    }

    stdConfig_bStarted = false;
}

bool J3DAPI stdConfig_AddRegKeyMapping(const char* pConfigKey, const char* pRegistryKey)
{
    STD_ASSERT(stdConfig_pKeyMappingTable);
    STD_ASSERTREL(pConfigKey);
    STD_ASSERTREL(pRegistryKey);

    if ( !stdHashtbl_Add(stdConfig_pKeyMappingTable, pConfigKey, pRegistryKey) )
    {
        return false;
    }

    return true;
}


bool stdConfig_Contains(const char* pKey)
{
    STD_ASSERT(stdConfig_bStarted);
    STD_ASSERT(stdConfig_hJson);
    STD_ASSERT(pKey);

    return stdJSON_HasKey(stdConfig_hJson, pKey);
}

bool J3DAPI stdConfig_SetBool(const char* pKey, bool value)
{
    STD_ASSERT(stdConfig_bStarted);
    STD_ASSERT(stdConfig_hJson);
    STD_ASSERTREL(pKey);
    return stdJSON_SetBool(stdConfig_hJson, pKey, value);
}

bool J3DAPI stdConfig_GetBool(const char* pKey, bool defaultValue)
{
    STD_ASSERT(stdConfig_bStarted);
    STD_ASSERT(stdConfig_hJson);
    STD_ASSERTREL(pKey);

    // First try to get from JSON config
    if ( stdJSON_HasKey(stdConfig_hJson, pKey) )
    {
        return stdJSON_GetBool(stdConfig_hJson, pKey, defaultValue);
    }

    // Fallback to registry if available and key mapping exists
    if ( stdConfig_pKeyMappingTable )
    {
        const char* pRegistryKey = stdConfig_GetRegistryKey(pKey);
        if ( pRegistryKey )
        {
            bool value = !!wuRegistry_GetInt(pRegistryKey, defaultValue ? 1 : 0);
            stdJSON_SetBool(stdConfig_hJson, pKey, value);
            return value;
        }
    }

    return defaultValue;
}

bool J3DAPI stdConfig_SetInt(const char* pKey, int value)
{
    STD_ASSERT(stdConfig_bStarted);
    STD_ASSERT(stdConfig_hJson);
    STD_ASSERTREL(pKey);
    return stdJSON_SetInt(stdConfig_hJson, pKey, value);
}

int J3DAPI stdConfig_GetInt(const char* pKey, int defaultValue)
{
    STD_ASSERT(stdConfig_bStarted);
    STD_ASSERT(stdConfig_hJson);
    STD_ASSERTREL(pKey);

    // First try to get from JSON config
    if ( stdJSON_HasKey(stdConfig_hJson, pKey) )
    {
        int value = stdJSON_GetInt(stdConfig_hJson, pKey, defaultValue);
        return value;
    }

    // Fallback to registry if available and key mapping exists
    if ( stdConfig_pKeyMappingTable )
    {
        const char* pRegistryKey = stdConfig_GetRegistryKey(pKey);
        if ( pRegistryKey )
        {
            int value = wuRegistry_GetInt(pRegistryKey, defaultValue);
            stdJSON_SetInt(stdConfig_hJson, pKey, value);
            return value;
        }
    }

    return defaultValue;
}

bool J3DAPI stdConfig_SetFloat(const char* pKey, float value)
{
    STD_ASSERT(stdConfig_bStarted);
    STD_ASSERT(stdConfig_hJson);
    STD_ASSERTREL(pKey);
    return stdJSON_SetFloat(stdConfig_hJson, pKey, value);
}

float J3DAPI stdConfig_GetFloat(const char* pKey, float defaultValue)
{
    STD_ASSERT(stdConfig_bStarted);
    STD_ASSERT(stdConfig_hJson);
    STD_ASSERTREL(pKey);

    // First try to get from JSON config
    if ( stdJSON_HasKey(stdConfig_hJson, pKey) )
    {
        return stdJSON_GetFloat(stdConfig_hJson, pKey, defaultValue);
    }

    // Fallback to registry if available and key mapping exists
    if ( stdConfig_pKeyMappingTable )
    {
        const char* pRegistryKey = stdConfig_GetRegistryKey(pKey);
        if ( pRegistryKey )
        {
            float value = wuRegistry_GetFloat(pRegistryKey, defaultValue);
            stdJSON_SetFloat(stdConfig_hJson, pKey, value);
            return value;
        }
    }

    return defaultValue;
}

bool J3DAPI stdConfig_SetString(const char* pKey, const char* pStr)
{
    STD_ASSERT(stdConfig_bStarted);
    STD_ASSERT(stdConfig_hJson);
    STD_ASSERTREL(pKey);
    STD_ASSERTREL(pStr);
    return stdJSON_SetString(stdConfig_hJson, pKey, pStr);
}

bool J3DAPI stdConfig_GetString(const char* pKey, char* pDstStr, size_t size, const char* pDefaultValue)
{
    STD_ASSERT(stdConfig_bStarted);
    STD_ASSERT(stdConfig_hJson);
    STD_ASSERTREL(pKey);
    STD_ASSERTREL(size > 0);
    STD_ASSERTREL(pDstStr);

    // First try to get from JSON config
    if ( stdJSON_HasKey(stdConfig_hJson, pKey) )
    {
        return stdJSON_GetString(stdConfig_hJson, pKey, pDstStr, size, pDefaultValue);
    }

    // Fallback to registry if available and key mapping exists
    if ( stdConfig_pKeyMappingTable )
    {
        const char* pRegistryKey = stdConfig_GetRegistryKey(pKey);
        if ( pRegistryKey )
        {
            int result = wuRegistry_GetStr(pRegistryKey, pDstStr, size, pDefaultValue);

            // Store the value in JSON for future access
            stdJSON_SetString(stdConfig_hJson, pKey, pDstStr);
            return result == 0; // wuRegistry_GetStr returns 0 on success

        }
    }

    // Use default value
    if ( !pDefaultValue )
    {
        pDstStr[0] = '\0';
        return false;
    }

    stdUtil_StringCopy(pDstStr, size, pDefaultValue);
    return true;
}

// Internal helper functions
static const char* stdConfig_GetRegistryKey(const char* pConfigKey)
{
    if ( !stdConfig_pKeyMappingTable || !pConfigKey )
    {
        return NULL;
    }

    return (const char*)stdHashtbl_Find(stdConfig_pKeyMappingTable, pConfigKey);
}

bool J3DAPI stdConfig_SetColor(const char* pKey, tStdColor color)
{
    if ( !pKey )
    {
        return false;
    }

    // Format as #RRGGBBAA
    char hexStr[10];
    STD_FORMAT(hexStr, "#%02X%02X%02X%02X", STD_GETRED(color), STD_GETGREEN(color), STD_GETBLUE(color), STD_GETALPHA(color));
    return stdConfig_SetString(pKey, hexStr);
}

bool J3DAPI stdConfig_SetColorRGB(const char* pKey, tStdColor color)
{
    if ( !pKey )
    {
        return false;
    }

    // Format as #RRGGBB 
    char hexStr[8];
    STD_FORMAT(hexStr, "#%02X%02X%02X", STD_GETRED(color), STD_GETGREEN(color), STD_GETBLUE(color));
    return stdConfig_SetString(pKey, hexStr);
}

tStdColor J3DAPI stdConfig_GetColor(const char* pKey, tStdColor defaultColor)
{
    if ( !pKey )
    {
        return defaultColor;
    }

    char hexStr[16];
    if ( !stdConfig_GetString(pKey, hexStr, sizeof(hexStr), NULL) )
    {
        return defaultColor;
    }

    // Skip '#' if present
    const char* pHex = hexStr;
    if ( pHex[0] == '#' )
    {
        pHex++;
    }

    // Skip hex base prefix
    if ( hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X') )
    {
        pHex += 2;
    }

    // Parse hex string
    size_t len = strlen(pHex);
    uint32_t r = 0, g = 0, b = 0, a = 255;
    if ( len == 6 ) // #RRGGBB format (no alpha)
    {
        if ( sscanf_s(pHex, "%2x%2x%2x", &r, &g, &b) != 3 )
        {
            return defaultColor;
        }
    }
    else if ( len == 8 ) // #RRGGBBAA format
    {
        if ( sscanf_s(pHex, "%2x%2x%2x%2x", &r, &g, &b, &a) != 4 )
        {
            return defaultColor;
        }
    }
    else
    {
        // Invalid format
        return defaultColor;
    }

    return STD_RGBA(r, g, b, a);
}
