#include <windows.h>
#include "wuRegistry.h"
#include <j3dcore/j3dhook.h>
#include <w32util/RTI/symbols.h>
#include <std/General/std.h>
#include <std/General/stdUtil.h>

int J3DAPI wuRegistry_Startup(HKEY hKey, LPCSTR lpSubKey);

static bool wuRegistry_bStarted = false;
static LPCSTR appSubKey = NULL;
static HKEY appRootKey  = NULL;

void wuRegistry_ResetGlobals(void)
{
    //memset(&wuRegistry_g_bStarted, 0, sizeof(wuRegistry_g_bStarted));
    //memset(&appSubKey, 0, sizeof(appSubKey));
    //memset(&appRootKey, 0, sizeof(appRootKey));
}

int J3DAPI wuRegistry_Startup(HKEY rootKey, LPCSTR subKey)
{
    STD_ASSERTREL(!wuRegistry_bStarted);

    // Added: Added HKEY_CURRENT_USER_LOCAL_SETTINGS & Removed HKEY_DYN_DATA 
    STD_ASSERTREL(
        rootKey == HKEY_CLASSES_ROOT
        || rootKey == HKEY_CURRENT_USER
        || rootKey == HKEY_LOCAL_MACHINE
        || rootKey == HKEY_USERS
        || rootKey == HKEY_PERFORMANCE_DATA
        || rootKey == HKEY_CURRENT_CONFIG
        || rootKey == HKEY_CURRENT_USER_LOCAL_SETTINGS
    );

    wuRegistry_bStarted = true;
    appSubKey  = subKey;
    appRootKey = rootKey;

    HKEY hkResult = 0;
    DWORD dwDisposition;
    if ( RegCreateKeyEx(
        rootKey,
        subKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE | KEY_READ, // 0x2001F -  KEY_WRITE | KEY_READ
        NULL,
        &hkResult,
        &dwDisposition) == ERROR_SUCCESS
        && RegCloseKey(hkResult) == ERROR_SUCCESS )
    {
        return 0;
    }

    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return 1;
}

// Added: Machine-wide root opener so callers above the platform layer need no HKEY.
int J3DAPI wuRegistry_StartupLocalMachine(const char* pSubKey)
{
    return wuRegistry_Startup(HKEY_LOCAL_MACHINE, pSubKey);
}

bool wuRegistry_HasStarted(void)
{
    return wuRegistry_bStarted;
}

void J3DAPI wuRegistry_Shutdown()
{
    appSubKey           = NULL;
    appRootKey          = NULL;
    wuRegistry_bStarted = false;
}

int J3DAPI wuRegistry_SaveInt(const char* pKey, int value)
{
    STD_ASSERTREL(wuRegistry_bStarted);

    // Added: Added HKEY_CURRENT_USER_LOCAL_SETTINGS & Removed HKEY_DYN_DATA 
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT
        || appRootKey == HKEY_CURRENT_USER
        || appRootKey == HKEY_LOCAL_MACHINE
        || appRootKey == HKEY_USERS
        || appRootKey == HKEY_PERFORMANCE_DATA
        || appRootKey == HKEY_CURRENT_CONFIG
        || appRootKey == HKEY_CURRENT_USER_LOCAL_SETTINGS
    );

    HKEY hkResult = 0;
    if ( RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult) == ERROR_SUCCESS
        && RegSetValueEx(hkResult, pKey, 0, REG_DWORD, (const BYTE*)&value, sizeof(value)) == ERROR_SUCCESS
        && RegCloseKey(hkResult) == ERROR_SUCCESS )
    {
        return 0;
    }

    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return 1;
}

int J3DAPI wuRegistry_SaveFloat(const char* pKey, float value)
{
    STD_ASSERTREL(wuRegistry_bStarted);

    // Added: Added HKEY_CURRENT_USER_LOCAL_SETTINGS & Removed HKEY_DYN_DATA 
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT
        || appRootKey == HKEY_CURRENT_USER
        || appRootKey == HKEY_LOCAL_MACHINE
        || appRootKey == HKEY_USERS
        || appRootKey == HKEY_PERFORMANCE_DATA
        || appRootKey == HKEY_CURRENT_CONFIG
        || appRootKey == HKEY_CURRENT_USER_LOCAL_SETTINGS
    );

    HKEY hkResult = 0;
    if ( RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult) == ERROR_SUCCESS
        && RegSetValueEx(hkResult, pKey, 0, REG_BINARY, (const BYTE*)&value, sizeof(value)) == ERROR_SUCCESS
        && RegCloseKey(hkResult) == ERROR_SUCCESS )
    {
        return 0;
    }

    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return 1;
}

int J3DAPI wuRegistry_GetInt(const char* pKey, int defaultValue)
{
    STD_ASSERTREL(wuRegistry_bStarted);

    // Added: Added HKEY_CURRENT_USER_LOCAL_SETTINGS & Removed HKEY_DYN_DATA 
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT
        || appRootKey == HKEY_CURRENT_USER
        || appRootKey == HKEY_LOCAL_MACHINE
        || appRootKey == HKEY_USERS
        || appRootKey == HKEY_PERFORMANCE_DATA
        || appRootKey == HKEY_CURRENT_CONFIG
        || appRootKey == HKEY_CURRENT_USER_LOCAL_SETTINGS
    );

    HKEY hkResult = 0;
    if ( RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult) != ERROR_SUCCESS )
    {
        return defaultValue;
    }

    BYTE data[sizeof(int)] = { 0 };
    DWORD sizeData = sizeof(data);
    DWORD type;
    if ( RegQueryValueEx(hkResult, pKey, NULL, &type, data, &sizeData) == ERROR_SUCCESS )
    {
        RegCloseKey(hkResult);
        return *(uint32_t*)data;
    }

    RegCloseKey(hkResult);
    return defaultValue;
}

float J3DAPI wuRegistry_GetFloat(const char* pKey, float defaultValue)
{
    STD_ASSERTREL(wuRegistry_bStarted);

    // Added: Added HKEY_CURRENT_USER_LOCAL_SETTINGS & Removed HKEY_DYN_DATA 
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT
        || appRootKey == HKEY_CURRENT_USER
        || appRootKey == HKEY_LOCAL_MACHINE
        || appRootKey == HKEY_USERS
        || appRootKey == HKEY_PERFORMANCE_DATA
        || appRootKey == HKEY_CURRENT_CONFIG
        || appRootKey == HKEY_CURRENT_USER_LOCAL_SETTINGS
    );

    HKEY hkResult = 0; // Added: Initialized to 0
    if ( RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult) )
    {
        return defaultValue;
    }

    BYTE data[sizeof(int)] = { 0 };
    DWORD sizeData = sizeof(data);
    DWORD type;
    if ( RegQueryValueEx(hkResult, pKey, NULL, &type, data, &sizeData) == ERROR_SUCCESS )// TODO: make new int type var for 4th param (pKey
    {
        RegCloseKey(hkResult);
        return *(float*)data;
    }

    RegCloseKey(hkResult);
    return defaultValue;
}

int J3DAPI wuRegistry_SaveBool(const char* pKey, int defaultValue)
{
    STD_ASSERTREL(wuRegistry_bStarted);
    return wuRegistry_SaveInt(pKey, defaultValue);
}

int J3DAPI wuRegistry_GetBool(const char* pKey, int defaultValue)
{
    STD_ASSERTREL(wuRegistry_bStarted);
    return wuRegistry_GetInt(pKey, defaultValue);
}


bool J3DAPI wuRegistry_SaveBinary(const char* pKey, const uint8_t* pData, size_t size)
{
    STD_ASSERTREL(wuRegistry_bStarted);

    // Added: Added HKEY_CURRENT_USER_LOCAL_SETTINGS & Removed HKEY_DYN_DATA 
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT
        || appRootKey == HKEY_CURRENT_USER
        || appRootKey == HKEY_LOCAL_MACHINE
        || appRootKey == HKEY_USERS
        || appRootKey == HKEY_PERFORMANCE_DATA
        || appRootKey == HKEY_CURRENT_CONFIG
        || appRootKey == HKEY_CURRENT_USER_LOCAL_SETTINGS
    );

    HKEY hkResult = 0;
    if ( RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult) == ERROR_SUCCESS
        && RegSetValueEx(hkResult, pKey, 0, REG_BINARY, pData, size) == ERROR_SUCCESS
        && RegCloseKey(hkResult) == ERROR_SUCCESS )
    {
        return true;
    }

    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return false;
}

bool J3DAPI wuRegistry_GetBinary(const char* pKey, uint8_t* pData, size_t size)
{
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT
        || appRootKey == HKEY_CURRENT_USER
        || appRootKey == HKEY_LOCAL_MACHINE
        || appRootKey == HKEY_USERS
        || appRootKey == HKEY_PERFORMANCE_DATA
        || appRootKey == HKEY_CURRENT_CONFIG
        || appRootKey == HKEY_CURRENT_USER_LOCAL_SETTINGS
    );

    HKEY hkResult = 0;
    if ( RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult) != ERROR_SUCCESS )
    {
        return false;
    }

    DWORD type;
    if ( RegQueryValueEx(hkResult, pKey, NULL, &type, pData, (LPDWORD)&size) == ERROR_SUCCESS )
    {
        RegCloseKey(hkResult);
        return true;
    }

    RegCloseKey(hkResult);
    return false;
}

int J3DAPI wuRegistry_SaveStr(const char* pKey, const char* pStr)
{
    STD_ASSERTREL(wuRegistry_bStarted);

    // Added: Added HKEY_CURRENT_USER_LOCAL_SETTINGS & Removed HKEY_DYN_DATA 
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT
        || appRootKey == HKEY_CURRENT_USER
        || appRootKey == HKEY_LOCAL_MACHINE
        || appRootKey == HKEY_USERS
        || appRootKey == HKEY_PERFORMANCE_DATA
        || appRootKey == HKEY_CURRENT_CONFIG
        || appRootKey == HKEY_CURRENT_USER_LOCAL_SETTINGS
    );

    HKEY hkResult = 0;
    if ( RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult) == ERROR_SUCCESS
        && RegSetValueEx(hkResult, pKey, 0, REG_SZ, (const uint8_t*)pStr, strlen(pStr)) == ERROR_SUCCESS
        && RegCloseKey(hkResult) == ERROR_SUCCESS )
    {
        return 0;
    }

    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return 1;
}

int J3DAPI wuRegistry_GetStr(const char* pKey, char* pDstStr, size_t size, const char* pDefaultValue)
{
    STD_ASSERTREL(wuRegistry_bStarted);

    // Added: Added HKEY_CURRENT_USER_LOCAL_SETTINGS & Removed HKEY_DYN_DATA 
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT
        || appRootKey == HKEY_CURRENT_USER
        || appRootKey == HKEY_LOCAL_MACHINE
        || appRootKey == HKEY_USERS
        || appRootKey == HKEY_PERFORMANCE_DATA
        || appRootKey == HKEY_CURRENT_CONFIG
        || appRootKey == HKEY_CURRENT_USER_LOCAL_SETTINGS
    );

    HKEY hkResult = 0;
    if ( RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult) == ERROR_SUCCESS )
    {
        // Altered: Changed call RegQueryValueEx -> RegGetValue to handle null termination
        DWORD dataSize = size;
        if ( RegGetValue(hkResult, NULL, pKey, RRF_RT_REG_SZ, NULL, (LPBYTE)pDstStr, &dataSize) == ERROR_SUCCESS
            && RegCloseKey(hkResult) == ERROR_SUCCESS )
        {
            return 0;
        }
    }

    if ( pDefaultValue )
    {
        stdUtil_StringCopy(pDstStr, size, pDefaultValue);
    }

    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return 1;
}