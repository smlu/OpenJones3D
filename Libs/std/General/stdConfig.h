#ifndef STDCONFIG_H
#define STDCONFIG_H

#include <j3dcore/j3d.h>

#include <std/General/stdColor.h>
#include <std/types.h>

#include <stddef.h>
#include <stdbool.h>

J3D_EXTERN_C_START

// Added: Reads settings stored by an older version, used when a key isn't in the JSON config.
typedef struct sStdConfigFallback
{
    bool (*pfHasStarted)(void);
    int (J3DAPI* pfGetInt)(const char* pKey, int defaultValue);
    float (J3DAPI* pfGetFloat)(const char* pKey, float defaultValue);
    int (J3DAPI* pfGetStr)(const char* pKey, char* pDstStr, size_t size, const char* pDefaultValue); // Returns 0 on success
} StdConfigFallback;

/**
 * Sets the fallback reader. The struct is copied. Call before stdConfig_Startup, which fails while no
 * fallback is set or pfHasStarted returns false. Stays set across stdConfig_Shutdown.
 */
void stdConfig_SetFallback(const StdConfigFallback* pFallback);

bool J3DAPI stdConfig_Startup(const char* pConfigPath);
bool stdConfig_HasStarted(void);
void J3DAPI stdConfig_Shutdown(void);

/**
 * @brief Checks if key is stored in config
 * @param pKey - key to check
 * @return true if key is in config otherwise false
 */
bool stdConfig_Contains(const char* pKey);

/**
 * Adds a mapping between a config key and a registry key.
 * This allows falling back to the registry if the config key does not exist in the JSON config.
 * The key supports nested keys using dot notation (e.g. "graphics.fullscreen").
 *
 * @param pConfigKey - Config key to map.
 * @param pRegistryKey - Registry key to map to.
 * @return true if successful, false on error (e.g. module not started).
 */
bool J3DAPI stdConfig_SetBool(const char* pKey, bool value);

/**
 * Gets bool value for given key.
 * The key supports nested keys using dot notation (e.g. "graphics.fullscreen").
 * If the key does not exist, the defaultValue is returned.
 *
 * @param pKey - Key to get value for.
 * @param defaultValue - Value to return if key does not exist.
 * @return Value associated with key, or defaultValue if key does not exist or on error (e.g. module not started).
 */
bool J3DAPI stdConfig_GetBool(const char* pKey, bool defaultValue);

/**
 * Sets int value for given key.
 * The key supports nested keys using dot notation (e.g. "graphics.resolution.width").
 *
 * @param pKey - Key to set value for.
 * @param value - Value to set.
 * @return true if successful, false on error (e.g. module not started).
 */
bool J3DAPI stdConfig_SetInt(const char* pKey, int value);

/**
 * Gets int value for given key.
 * The key supports nested keys using dot notation (e.g. "graphics.resolution.width").
 * If the key does not exist, the defaultValue is returned.
 *
 * @param pKey - Key to get value for.
 * @param defaultValue - Value to return if key does not exist.
 * @return Value associated with key, or defaultValue if key does not exist or on error (e.g. module not started).
 */
int J3DAPI stdConfig_GetInt(const char* pKey, int defaultValue);

/**
 * Sets float value for given key.
 * The key supports nested keys using dot notation (e.g. "graphics.resolution.width").
 *
 * @param pKey - Key to set value for.
 * @param value - Value to set.
 * @return true if successful, false on error (e.g. module not started).
 */
bool J3DAPI stdConfig_SetFloat(const char* pKey, float value);

/**
 * Gets float value for given key.
 * The key supports nested keys using dot notation (e.g. "graphics.resolution.width").
 * If the key does not exist, the defaultValue is returned.
 *
 * @param pKey - Key to get value for.
 * @param defaultValue - Value to return if key does not exist.
 * @return Value associated with key, or defaultValue if key does not exist or on error (e.g. module not started).
 */
float J3DAPI stdConfig_GetFloat(const char* pKey, float defaultValue);

/**
 * Sets string value for given key.
 * The key supports nested keys using dot notation (e.g. "key.subkey").
 *
 * @param pKey - Key to set value for.
 * @param pStr - String value to set.
 * @return true if successful, false on error (e.g. module not started).
 */
bool J3DAPI stdConfig_SetString(const char* pKey, const char* pStr);

/**
 * Gets string value for given key.
 * The key supports nested keys using dot notation (e.g. "key.subkey").
 * If the key does not exist, the pDefaultValue is used if provided, otherwise an empty string is returned.
 *
 * @param pKey - Key to get value for.
 * @param pDstStr - Buffer to store the retrieved string.
 * @param size - Size of the buffer pointed to by pDstStr.
 * @param pDefaultValue - Default string to use if key does not exist. Can be NULL.
 * @return true if the string was successfully retrieved and copied into pDstStr; otherwise, false.
 *         If false is returned, pDstStr will contain an empty string if pDefaultValue is NULL, or the default value if provided.
 */
bool J3DAPI stdConfig_GetString(const char* pKey, char* pDstStr, size_t size, const char* pDefaultValue);

/**
 * Sets color value for given key using stdColor format.
 * Color is stored as hex string in format "#RRGGBBAA" for readability.
 *
 * @param pKey  - Key to set value for.
 * @param color - Color value encoded using STD_RGBA(r, g, b, a).
 * @return true if successful, false on error.
 */
bool J3DAPI stdConfig_SetColor(const char* pKey, tStdColor color);

/**
 * Sets RGB color value without alpha for given key using stdColor format.
 * Color is stored as hex string in format "#RRGGBB" for readability.
 *
 * @param pKey  - Key to set value for.
 * @param color - Color value encoded using STD_RGB(r, g, b) or STD_RGBA(r, g, b, a).
 * @return true if successful, false on error.
 */
bool J3DAPI stdConfig_SetColorRGB(const char* pKey, tStdColor color);

/**
 * Gets color value for given key in stdColor format.
 *
 * @param pKey - Key to get value for.
 * @param defaultColor - Default color (use STD_RGBA(r, g, b, a)).
 * @return Color value that can be decoded with STD_GETALPHA, STD_GETRED, etc.
 */
tStdColor J3DAPI stdConfig_GetColor(const char* pKey, tStdColor defaultColor);

J3D_EXTERN_C_END

#endif // STDCONFIG_H