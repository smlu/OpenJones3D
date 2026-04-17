#include "stdJSON.h"
#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <jansson.h>
#include <string.h>

// Internal JSON handle structure
typedef struct sStdJSON
{
    json_t* pRoot;
    StdJSONHandle pParent;
    char* pFilePath;
    bool bAutoSave;
    bool bModified;
    char** ppRequiredKeys;
    size_t numRequiredKeys;
} StdJSON;

// Module state
static bool stdJSON_bStarted = false;

static json_malloc_t stdJSON_pfJanssonAlloc = NULL;
static json_realloc_t stdJSON_pfJanssonRealloc = NULL;
static json_free_t stdJSON_pfJanssonFree = NULL;

// Internal helper functions
static StdJSONHandle stdJSON_NewChild(json_t* pJson, StdJSONHandle hParent);

static bool stdJSON_AutoSave(StdJSONHandle hJson);

static StdJSONType stdJSON_GetJSONType(const json_t* pJson);

static json_t* stdJSON_GetValue(StdJSONHandle hJson, const char* pKey);
static bool stdJSON_SetValue(StdJSONHandle hJson, const char* pKey, json_t* pValue);

static json_t* stdJSON_GetNestedValue(json_t* pRoot, const char* pPath);
static bool stdJSON_SetNestedValue(json_t* pRoot, const char* pPath, json_t* pValue);

static json_t* stdJSON_GetArrayValue(StdJSONHandle hJson, const char* pKey, size_t index);
static bool stdJSON_SetArrayValue(StdJSONHandle hJson, const char* pKey, size_t index, json_t* pValue);

static void stdJSON_FreeRequiredKeys(StdJSONHandle hJson);

static void* stdJSON_Alloc(size_t size)
{
    return STDMALLOC(size);
}

static void* stdJSON_Realloc(void* pData, size_t size)
{
    return STDREALLOC(pData, size);
}

static void stdJSON_FreeMem(void* pData)
{
    STDFREE(pData);
}

// Module initialization
bool J3DAPI stdJSON_Startup(void)
{
    if ( stdJSON_bStarted )
    {
        STDLOG_WARNING("stdJSON_Startup: Module already started.\n");
        return true;
    }

    if ( !std_g_pHS )
    {
        STDLOG_ERROR("stdJSON_Startup: Host services not available.\n");
        return false;
    }

    // Set custom memory allocators for Jansson
    json_get_alloc_funcs2(&stdJSON_pfJanssonAlloc, &stdJSON_pfJanssonRealloc, &stdJSON_pfJanssonFree);
    json_set_alloc_funcs2(stdJSON_Alloc, stdJSON_Realloc, stdJSON_FreeMem);

    stdJSON_bStarted = true;
    return true;
}

bool stdJSON_HasStarted(void)
{
    return stdJSON_bStarted;
}

void J3DAPI stdJSON_Shutdown(void)
{
    if ( !stdJSON_bStarted )
    {
        return;
    }

    json_set_alloc_funcs2(stdJSON_pfJanssonAlloc, stdJSON_pfJanssonRealloc, stdJSON_pfJanssonFree);
    stdJSON_bStarted = false;
}

StdJSONHandle J3DAPI stdJSON_New(void)
{
    if ( !stdJSON_bStarted )
    {
        STDLOG_ERROR("stdJSON_New: Module not started.\n");
        return NULL;
    }

    // Create JSON object
    StdJSONHandle hJson = (StdJSONHandle)STDMALLOC(sizeof(StdJSON));
    if ( !hJson )
    {
        STDLOG_ERROR("stdJSON_New: Handle allocation failed.\n");
        return NULL;
    }

    STD_ZEROMEM(hJson, sizeof(StdJSON));
    hJson->pRoot = json_object();
    return hJson;
}

static StdJSONHandle stdJSON_NewChild(json_t* pJson, StdJSONHandle hParent)
{
    if ( !pJson || !hParent )
    {
        return NULL;
    }

    StdJSONHandle pChildObj = stdJSON_New();
    if ( !pChildObj )
    {
        return NULL;
    }

    // Release initial empty object
    json_decref(pChildObj->pRoot);

    json_incref(pJson); // Increase ref count as we are assigning it to new handle to hold ownership independently of parent
    pChildObj->pRoot = pJson;

    stdJSON_SetParent(pChildObj, hParent);
    pChildObj->bAutoSave = hParent->bAutoSave;
    return pChildObj;
}

void J3DAPI stdJSON_FreeEntry(StdJSONHandle hJson)
{
    if ( !hJson )
    {
        return;
    }

    if ( hJson->pRoot )
    {
        json_decref(hJson->pRoot);
    #ifdef J3D_DEBUG
        if ( !hJson->pParent && hJson && *((uintptr_t*)hJson->pRoot) != (uintptr_t)STDMEMORY_FREEDPTR )
        {
            STDLOG_WARNING("stdJSON_FreeEntry: !!! POSSIBLE MEMORY LEAK ... JSON root was not deleted and refcount is %zu. Possibly undeleted child handle is holding it's reference !!!\n", hJson->pRoot->refcount);
        }
    #endif
        hJson->pRoot = NULL;
    }

    stdJSON_ClearParent(hJson);

    if ( hJson->pFilePath )
    {
        STDFREE(hJson->pFilePath);
        hJson->pFilePath = NULL;
    }

    stdJSON_FreeRequiredKeys(hJson);
}

void J3DAPI stdJSON_Free(StdJSONHandle hJson)
{
    if ( !hJson )
    {
        return;
    }

    stdJSON_FreeEntry(hJson);
    STDFREE(hJson);
}

bool J3DAPI stdJSON_Delete(StdJSONHandle hJson, const char* pKey)
{
    if ( !hJson || !hJson->pRoot || !pKey )
    {
        STDLOG_ERROR("stdJSON_Delete: Invalid parameters.\n");
        return false;
    }

    // Handle nested path deletion
    if ( strchr(pKey, '.') != NULL )
    {
        // For nested paths, we need to navigate to parent and delete key
        size_t pathSize = strlen(pKey) + 1;
        char* pathCopy = (char*)STDMALLOC(pathSize);
        if ( !pathCopy )
        {
            return false;
        }
        stdUtil_StringCopy(pathCopy, pathSize, pKey);

        // Find the last dot to separate parent path from key
        char* lastDot = strrchr(pathCopy, '.');
        if ( !lastDot )
        {
            STDFREE(pathCopy);
            return false;
        }

        *lastDot = '\0';
        const char* finalKey = lastDot + 1;
        const char* parentPath = pathCopy;

        json_t* parentObj = stdJSON_GetNestedValue(hJson->pRoot, parentPath);
        if ( json_is_object(parentObj) )
        {
            int result = json_object_del(parentObj, finalKey);
            if ( result == 0 )
            {
                STDFREE(pathCopy);
                hJson->bModified = true;
                stdJSON_AutoSave(hJson);
                return true;
            }
        }

        STDFREE(pathCopy);
        return false;
    }
    else
    {
        // Simple key deletion
        int result = json_object_del(hJson->pRoot, pKey);
        if ( result == 0 )
        {
            hJson->bModified = true;
            stdJSON_AutoSave(hJson);
            return true;
        }
    }

    return false;
}

bool J3DAPI stdJSON_Clear(StdJSONHandle hJson)
{
    if ( !hJson || !hJson->pRoot )
    {
        STDLOG_ERROR("stdJSON_Clear: Invalid handle.\n");
        return false;
    }

    int result = json_object_clear(hJson->pRoot);
    if ( result == 0 )
    {
        hJson->bModified = true;
        stdJSON_AutoSave(hJson);
        return true;
    }

    return false;
}

bool J3DAPI stdJSON_Merge(StdJSONHandle hDst, StdJSONHandle hSrc, bool bOverwrite)
{
    if ( !hDst || !hDst->pRoot || !hSrc || !hSrc->pRoot )
    {
        STDLOG_ERROR("stdJSON_Merge: Invalid handles.\n");
        return false;
    }

    if ( !json_is_object(hDst->pRoot) || !json_is_object(hSrc->pRoot) )
    {
        STDLOG_ERROR("stdJSON_Merge: Both handles must contain JSON objects.\n");
        return false;
    }

    const char* key;
    json_t* value;
    json_object_foreach(hSrc->pRoot, key, value)
    {
        if ( !bOverwrite && json_object_get(hDst->pRoot, key) != NULL )
        {
            continue; // Skip existing keys if overwrite is disabled
        }

        if ( json_object_set_new(hDst->pRoot, key, json_deep_copy(value)) != 0 )
        {
            STDLOG_ERROR("stdJSON_Merge: Failed to merge key '%s'\n", key);
            return false;
        }
    }

    hDst->bModified = true;
    stdJSON_AutoSave(hDst);
    return true;
}

StdJSONHandle J3DAPI stdJSON_Duplicate(const StdJSONHandle hSrc)
{
    if ( !hSrc || !hSrc->pRoot )
    {
        STDLOG_ERROR("stdJSON_Duplicate: Invalid source handle.\n");
        return NULL;
    }

    json_t* pRootCopy = json_deep_copy(hSrc->pRoot);
    if ( !pRootCopy )
    {
        STDLOG_ERROR("stdJSON_Duplicate: Failed to copy JSON object.\n");
        return NULL;
    }

    StdJSONHandle hDst = stdJSON_New();
    if ( !hDst )
    {
        json_decref(pRootCopy);
        STDLOG_ERROR("stdJSON_Duplicate: Handle allocation failed.\n");
        return NULL;
    }

    hDst->pRoot           = pRootCopy;
    hDst->pFilePath       = NULL;
    hDst->bAutoSave       = hSrc->bAutoSave;
    hDst->bModified       = hSrc->bModified;
    hDst->ppRequiredKeys  = NULL;
    hDst->numRequiredKeys = 0;

    if ( hSrc->pParent )
    {
        stdJSON_SetParent(hDst, hSrc->pParent);
    }

    // Copy file path if it exists
    if ( hSrc->pFilePath )
    {
        size_t pathSize = strlen(hSrc->pFilePath) + 1;
        hDst->pFilePath = (char*)STDMALLOC(pathSize);
        if ( hDst->pFilePath )
        {
            stdUtil_StringCopy(hDst->pFilePath, pathSize, hSrc->pFilePath);
        }
    }

    // Copy required keys if they exist
    if ( hSrc->numRequiredKeys > 0 && hSrc->ppRequiredKeys )
    {
        stdJSON_SetRequiredKeys(hDst, hSrc->ppRequiredKeys, hSrc->numRequiredKeys);
    }

    return hDst;
}

bool J3DAPI stdJSON_IsValid(StdJSONHandle hJson)
{
    return hJson && json_is_object(hJson->pRoot);
}

StdJSONType stdJSON_GetJSONType(const json_t* pJson)
{
    if ( !pJson )
    {
        return STDJSON_TYPE_NULL;
    }

    if ( json_is_null(pJson) )    return STDJSON_TYPE_NULL;
    if ( json_is_boolean(pJson) ) return STDJSON_TYPE_BOOL;
    if ( json_is_integer(pJson) ) return STDJSON_TYPE_INTEGER;
    if ( json_is_real(pJson) )    return STDJSON_TYPE_REAL;
    if ( json_is_string(pJson) )  return STDJSON_TYPE_STRING;
    if ( json_is_array(pJson) )   return STDJSON_TYPE_ARRAY;
    if ( json_is_object(pJson) )  return STDJSON_TYPE_OBJECT;
    return STDJSON_TYPE_NULL;
}

StdJSONType J3DAPI stdJSON_GetType(StdJSONHandle hJson)
{
    return stdJSON_GetJSONType(hJson ? hJson->pRoot : NULL);
}

bool J3DAPI stdJSON_SetRequiredKeys(StdJSONHandle hJson, const char** ppKeys, size_t count)
{
    if ( !hJson || (!ppKeys && count > 0) )
    {
        STDLOG_ERROR("stdJSON_SetRequiredKeys: Invalid parameters.\n");
        return false;
    }

    // Free existing required keys
    stdJSON_FreeRequiredKeys(hJson);
    if ( count == 0 )
    {
        return true;
    }

    // Allocate array for required keys
    const size_t arraySize = count * sizeof(char*);
    hJson->ppRequiredKeys = (char**)STDMALLOC(arraySize);
    if ( !hJson->ppRequiredKeys )
    {
        STDLOG_ERROR("stdJSON_SetRequiredKeys: Memory allocation failed.\n");
        return false;
    }

    STD_ZEROMEM(hJson->ppRequiredKeys, arraySize);

    // Copy required key strings
    for ( size_t i = 0; i < count; i++ )
    {
        if ( !ppKeys[i] )
        {
            hJson->numRequiredKeys = i;
            stdJSON_FreeRequiredKeys(hJson);
            return false;
        }

        size_t keySize = strlen(ppKeys[i]) + 1;
        hJson->ppRequiredKeys[i] = (char*)STDMALLOC(keySize);
        if ( !hJson->ppRequiredKeys[i] )
        {
            // Free already allocated keys
            hJson->numRequiredKeys = i;
            stdJSON_FreeRequiredKeys(hJson);

            STDLOG_ERROR("stdJSON_SetRequiredKeys: Key allocation failed.\n");
            return false;
        }

        stdUtil_StringCopy(hJson->ppRequiredKeys[i], keySize, ppKeys[i]);
    }

    hJson->numRequiredKeys = count;
    return true;
}

bool J3DAPI stdJSON_ValidateRequired(StdJSONHandle hJson)
{
    if ( !hJson || !hJson->pRoot )
    {
        return false;
    }

    if ( hJson->numRequiredKeys == 0 )
    {
        return true; // No required keys means validation passes
    }

    for ( size_t i = 0; i < hJson->numRequiredKeys; i++ )
    {
        if ( !stdJSON_HasKey(hJson, hJson->ppRequiredKeys[i]) )
        {
            STDLOG_WARNING("stdJSON_ValidateRequired: Missing required key '%s'\n", hJson->ppRequiredKeys[i]);
            return false;
        }
    }

    return true;
}

StdJSONHandle J3DAPI stdJSON_Load(const char* pFilePath)
{
    if ( !stdJSON_bStarted )
    {
        STDLOG_ERROR("stdJSON_Load: Module not started.\n");
        return NULL;
    }

    if ( !pFilePath )
    {
        STDLOG_ERROR("stdJSON_Load: Invalid file path.\n");
        return NULL;
    }

    // Create object
    StdJSONHandle hJson = stdJSON_New();
    if ( !hJson )
    {
        return NULL;
    }

    if ( !stdJSON_LoadEntry(pFilePath, hJson) )
    {
        stdJSON_Free(hJson);
        return NULL;
    }

    return hJson;
}

bool J3DAPI stdJSON_LoadEntry(const char* pFilePath, StdJSONHandle hJson)
{
    STD_ASSERT(hJson != NULL);

    if ( !stdJSON_bStarted )
    {
        STDLOG_ERROR("stdJSON_LoadEntry: Module not started.\n");
        return false;
    }

    if ( !pFilePath )
    {
        STDLOG_ERROR("stdJSON_LoadEntry: Invalid file path.\n");
        return false;
    }

    // Free any allocated data in handle
    stdJSON_FreeEntry(hJson);

    // Get file size
    size_t fileSize = std_g_pHS->pFileSize(pFilePath);
    if ( fileSize == 0 )
    {
        STDLOG_WARNING("stdJSON_LoadEntry: File '%s' is empty.\n", pFilePath);
        return false;
    }

    // Open file for reading
    tFileHandle hFile = std_g_pHS->pFileOpen(pFilePath, "rb");
    if ( !hFile )
    {
        STDLOG_WARNING("stdJSON_LoadEntry: Cannot open file '%s'\n", pFilePath);
        return false;
    }

    // Allocate buffer for file content
    char* pBuffer = (char*)STDMALLOC(fileSize);
    if ( !pBuffer )
    {
        std_g_pHS->pFileClose(hFile);
        STDLOG_ERROR("stdJSON_LoadEntry: Memory allocation failed.\n");
        return false;
    }

    // Read file content
    size_t bytesRead = std_g_pHS->pFileRead(hFile, pBuffer, fileSize);
    std_g_pHS->pFileClose(hFile);

    if ( bytesRead != fileSize )
    {
        STDFREE(pBuffer);
        STDLOG_ERROR("stdJSON_LoadEntry: File read failed.\n");
        return false;
    }

    // Parse JSON
    json_error_t error;
    json_t* pRoot = json_loadb(pBuffer, fileSize, 0, &error);
    STDFREE(pBuffer);

    if ( !pRoot )
    {
        STDLOG_ERROR("stdJSON_LoadEntry: JSON parse error at line %d: %s\n", error.line, error.text);
        return false;
    }

    if ( !json_is_object(pRoot) )
    {
        json_decref(pRoot);
        STDLOG_ERROR("stdJSON_LoadEntry: Root JSON element is not an object.\n");
        return false;
    }

    // Copy file path
    size_t pathSize = strlen(pFilePath) + 1;
    hJson->pFilePath = (char*)STDMALLOC(pathSize);
    if ( !hJson->pFilePath )
    {
        json_decref(pRoot);
        STDLOG_ERROR("stdJSON_LoadEntry: File path allocation failed.\n");
        return false;
    }

    stdUtil_StringCopy(hJson->pFilePath, pathSize, pFilePath);

    hJson->pRoot           = pRoot;
    hJson->pParent         = NULL;
    hJson->bAutoSave       = false;
    hJson->bModified       = false;
    hJson->ppRequiredKeys  = NULL;
    hJson->numRequiredKeys = 0;
    return true;
}

StdJSONHandle J3DAPI stdJSON_LoadFromString(const char* pJsonString)
{
    if ( !stdJSON_bStarted )
    {
        STDLOG_ERROR("stdJSON_LoadFromString: Module not started.\n");
        return NULL;
    }

    if ( !pJsonString )
    {
        STDLOG_ERROR("stdJSON_LoadFromString: Invalid JSON string.\n");
        return NULL;
    }

    StdJSONHandle hJson = stdJSON_New();
    if ( !hJson )
    {
        STDLOG_ERROR("stdJSON_LoadFromString: Handle allocation failed.\n");
        return NULL;
    }

    if ( !stdJSON_LoadEntryFromString(pJsonString, hJson) )
    {
        stdJSON_Free(hJson);
        return NULL;
    }

    return hJson;
}

StdJSONHandle J3DAPI stdJSON_LoadEntryFromString(const char* pJsonString, StdJSONHandle hJson)
{
    if ( !stdJSON_bStarted )
    {
        STDLOG_ERROR("stdJSON_LoadEntryFromString: Module not started.\n");
        return NULL;
    }

    STD_ASSERT(hJson != NULL);
    STD_ASSERT(pJsonString != NULL);

    // Free any existing data in handle
    stdJSON_FreeEntry(hJson);

    json_error_t error;
    json_t* pRoot = json_loads(pJsonString, 0, &error);
    if ( !pRoot )
    {
        STDLOG_ERROR("stdJSON_LoadEntryFromString: JSON parse error at line %d: %s\n", error.line, error.text);
        return NULL;
    }

    if ( !json_is_object(pRoot) )
    {
        json_decref(pRoot);
        STDLOG_ERROR("stdJSON_LoadEntryFromString: Root JSON element is not an object.\n");
        return NULL;
    }

    hJson->pRoot           = pRoot;
    hJson->pParent         = NULL;
    hJson->pFilePath       = NULL;
    hJson->bAutoSave       = false;
    hJson->bModified       = false;
    hJson->ppRequiredKeys  = NULL;
    hJson->numRequiredKeys = 0;

    return hJson;
}

char* J3DAPI stdJSON_ToString(StdJSONHandle hJson, bool bPretty)
{
    if ( !hJson || !hJson->pRoot )
    {
        STDLOG_ERROR("stdJSON_ToString: Invalid handle.\n");
        return NULL;
    }

    size_t flags = 0;
    if ( bPretty )
    {
        flags |= JSON_INDENT(2);
    }

    return json_dumps(hJson->pRoot, flags);
}

bool J3DAPI stdJSON_Save(StdJSONHandle hJson, const char* pFilePath)
{
    hJson = stdJSON_GetRoot(hJson);
    if ( !hJson || !hJson->pRoot )
    {
        STDLOG_ERROR("stdJSON_Save: Invalid handle.\n");
        return false;
    }

    const char* pPath = pFilePath ? pFilePath : hJson->pFilePath;
    if ( !pPath )
    {
        STDLOG_ERROR("stdJSON_Save: No file path specified.\n");
        return false;
    }

    // Generate JSON string
    char* pJsonString = json_dumps(hJson->pRoot, JSON_INDENT(2) | JSON_REAL_PRECISION(12));
    if ( !pJsonString )
    {
        STDLOG_ERROR("stdJSON_Save: Failed to serialize JSON.\n");
        return false;
    }

    // Open file for writing
    tFileHandle hFile = std_g_pHS->pFileOpen(pPath, "w");
    if ( !hFile )
    {
        STDFREE(pJsonString);
        STDLOG_ERROR("stdJSON_Save: Cannot open file '%s' for writing.\n", pPath);
        return false;
    }

    // Write JSON string to file
    size_t len = strlen(pJsonString);
    size_t written = std_g_pHS->pFileWrite(hFile, pJsonString, len);
    std_g_pHS->pFileClose(hFile);
    STDFREE(pJsonString);

    if ( written != len )
    {
        STDLOG_ERROR("stdJSON_Save: File write failed.\n");
        return false;
    }

    // Update file path if new path was provided
    if ( pFilePath && (!hJson->pFilePath || !streq(hJson->pFilePath, pFilePath)) )
    {
        if ( hJson->pFilePath )
        {
            STDFREE(hJson->pFilePath);
        }

        size_t pathSize = strlen(pFilePath) + 1;
        hJson->pFilePath = (char*)STDMALLOC(pathSize);
        if ( hJson->pFilePath )
        {
            stdUtil_StringCopy(hJson->pFilePath, pathSize, pFilePath);
        }
    }

    hJson->bModified = false;
    return true;
}

void J3DAPI stdJSON_SetAutoSave(StdJSONHandle hJson, bool bAutoSave)
{
    if ( !hJson )
    {
        STDLOG_ERROR("stdJSON_SetAutoSave: Invalid handle.\n");
        return;
    }

    hJson->bAutoSave = bAutoSave;
}

// Internal helper to auto-save if enabled
static bool stdJSON_AutoSave(StdJSONHandle hJson)
{
    if ( hJson && hJson->bAutoSave )
    {
        StdJSONHandle pParent = stdJSON_GetParent(hJson);
        if ( pParent ? !pParent->pFilePath : !hJson->pFilePath )
        {
            return false; // No path to save to
        }

        return stdJSON_Save(hJson, NULL); // path should be null to not try to make a copy
    }
    return true;
}

bool J3DAPI stdJSON_SetBool(StdJSONHandle hJson, const char* pKey, bool value)
{
    if ( !hJson || !pKey )
    {
        STDLOG_ERROR("stdJSON_SetBool: Invalid parameters.\n");
        return false;
    }

    json_t* pValue = value ? json_true() : json_false();
    if ( !pValue )
    {
        STDLOG_ERROR("stdJSON_SetBool: Failed to create JSON boolean.\n");
        return false;
    }

    bool result = stdJSON_SetValue(hJson, pKey, pValue);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_SetInt(StdJSONHandle hJson, const char* pKey, int value)
{
    if ( !hJson || !pKey )
    {
        STDLOG_ERROR("stdJSON_SetInt: Invalid parameters.\n");
        return false;
    }

    json_t* pValue = json_integer(value);
    if ( !pValue )
    {
        STDLOG_ERROR("stdJSON_SetInt: Failed to create JSON integer.\n");
        return false;
    }

    bool result = stdJSON_SetValue(hJson, pKey, pValue);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_SetFloat(StdJSONHandle hJson, const char* pKey, float value)
{
    if ( !hJson || !pKey )
    {
        STDLOG_ERROR("stdJSON_SetFloat: Invalid parameters.\n");
        return false;
    }

    json_t* pValue = json_real((double)value);
    if ( !pValue )
    {
        STDLOG_ERROR("stdJSON_SetFloat: Failed to create JSON real.\n");
        return false;
    }

    bool result = stdJSON_SetValue(hJson, pKey, pValue);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_SetString(StdJSONHandle hJson, const char* pKey, const char* pStr)
{
    if ( !hJson || !pKey || !pStr )
    {
        STDLOG_ERROR("stdJSON_SetString: Invalid parameters.\n");
        return false;
    }

    json_t* pValue = json_string(pStr);
    if ( !pValue )
    {
        STDLOG_ERROR("stdJSON_SetString: Failed to create JSON string.\n");
        return false;
    }

    bool result = stdJSON_SetValue(hJson, pKey, pValue);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_GetBool(StdJSONHandle hJson, const char* pKey, bool defaultValue)
{
    json_t* pValue = stdJSON_GetValue(hJson, pKey);
    if ( !json_is_boolean(pValue) )
    {
        return defaultValue;
    }
    return json_is_true(pValue);
}

int J3DAPI stdJSON_GetInt(StdJSONHandle hJson, const char* pKey, int defaultValue)
{
    json_t* pValue = stdJSON_GetValue(hJson, pKey);
    if ( !json_is_integer(pValue) )
    {
        return defaultValue;
    }
    return (int)json_integer_value(pValue);
}

float J3DAPI stdJSON_GetFloat(StdJSONHandle hJson, const char* pKey, float defaultValue)
{
    json_t* pValue = stdJSON_GetValue(hJson, pKey);
    if ( !pValue || (!json_is_real(pValue) && !json_is_integer(pValue)) )
    {
        return defaultValue;
    }
    return json_is_real(pValue) ? (float)json_real_value(pValue) : (float)json_integer_value(pValue);
}

bool J3DAPI stdJSON_GetString(StdJSONHandle hJson, const char* pKey, char* pDstStr, size_t size, const char* pDefaultValue)
{
    if ( !pDstStr || size == 0 )
    {
        STDLOG_ERROR("stdJSON_GetString: Invalid output buffer.\n");
        return false;
    }

    json_t* pValue = stdJSON_GetValue(hJson, pKey);
    const char* pStr = NULL;
    if ( pValue )
    {
        pStr = json_string_value(pValue);
    }
    else
    {
        pStr = pDefaultValue;
    }

    if ( !pStr )
    {
        pDstStr[0] = '\0';
        return false;
    }

    stdUtil_StringCopy(pDstStr, size, pStr);
    return true;
}

StdJSONHandle J3DAPI stdJSON_GetObject(StdJSONHandle hJson, const char* pKey)
{
    if ( !hJson || !pKey )
    {
        STDLOG_ERROR("stdJSON_GetObject: Invalid parameters.\n");
        return NULL;
    }

    json_t* pObj = stdJSON_GetValue(hJson, pKey);
    if ( !json_is_object(pObj) )
    {
        return NULL;
    }
    return stdJSON_NewChild(pObj, hJson);
}

StdJSONHandle J3DAPI stdJSON_GetArray(StdJSONHandle hJson, const char* pKey)
{
    if ( !hJson || !pKey )
    {
        STDLOG_ERROR("stdJSON_GetArray: Invalid parameters.\n");
        return NULL;
    }

    json_t* pArray = stdJSON_GetValue(hJson, pKey);
    if ( !pArray || !json_is_array(pArray) )
    {
        return NULL;
    }
    return stdJSON_NewChild(pArray, hJson);
}

bool J3DAPI stdJSON_SetIntArray(StdJSONHandle hJson, const char* pKey, const int* pValues, size_t count)
{
    if ( !hJson || !pKey || (!pValues && count > 0) )
    {
        STDLOG_ERROR("stdJSON_SetIntArray: Invalid parameters.\n");
        return false;
    }

    json_t* pArray = json_array();
    if ( !pArray )
    {
        STDLOG_ERROR("stdJSON_SetIntArray: Failed to create JSON array.\n");
        return false;
    }

    for ( size_t i = 0; i < count; i++ )
    {
        json_t* pElement = json_integer(pValues[i]);
        if ( !pElement || json_array_append_new(pArray, pElement) != 0 )
        {
            json_decref(pArray);
            STDLOG_ERROR("stdJSON_SetIntArray: Failed to add array element.\n");
            return false;
        }
    }

    bool result = stdJSON_SetValue(hJson, pKey, pArray);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_SetFloatArray(StdJSONHandle hJson, const char* pKey, const float* pValues, size_t count)
{
    if ( !hJson || !pKey || (!pValues && count > 0) )
    {
        STDLOG_ERROR("stdJSON_SetFloatArray: Invalid parameters.\n");
        return false;
    }

    json_t* pArray = json_array();
    if ( !pArray )
    {
        STDLOG_ERROR("stdJSON_SetFloatArray: Failed to create JSON array.\n");
        return false;
    }

    for ( size_t i = 0; i < count; i++ )
    {
        json_t* pElement = json_real((double)pValues[i]);
        if ( !pElement || json_array_append_new(pArray, pElement) != 0 )
        {
            json_decref(pArray);
            STDLOG_ERROR("stdJSON_SetFloatArray: Failed to add array element.\n");
            return false;
        }
    }

    bool result = stdJSON_SetValue(hJson, pKey, pArray);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_SetBoolArray(StdJSONHandle hJson, const char* pKey, const bool* pValues, size_t count)
{
    if ( !hJson || !pKey || (!pValues && count > 0) )
    {
        STDLOG_ERROR("stdJSON_SetBoolArray: Invalid parameters.\n");
        return false;
    }

    json_t* pArray = json_array();
    if ( !pArray )
    {
        STDLOG_ERROR("stdJSON_SetBoolArray: Failed to create JSON array.\n");
        return false;
    }

    for ( size_t i = 0; i < count; i++ )
    {
        json_t* pElement = pValues[i] ? json_true() : json_false();
        if ( !pElement || json_array_append_new(pArray, pElement) != 0 )
        {
            json_decref(pArray);
            STDLOG_ERROR("stdJSON_SetBoolArray: Failed to add array element.\n");
            return false;
        }
    }

    bool result = stdJSON_SetValue(hJson, pKey, pArray);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_SetStringArray(StdJSONHandle hJson, const char* pKey, const char** ppValues, size_t count)
{
    if ( !hJson || !pKey || (!ppValues && count > 0) )
    {
        STDLOG_ERROR("stdJSON_SetStringArray: Invalid parameters.\n");
        return false;
    }

    json_t* pArray = json_array();
    if ( !pArray )
    {
        STDLOG_ERROR("stdJSON_SetStringArray: Failed to create JSON array.\n");
        return false;
    }

    for ( size_t i = 0; i < count; i++ )
    {
        const char* pStr = ppValues[i] ? ppValues[i] : "";
        json_t* pElement = json_string(pStr);
        if ( !pElement || json_array_append_new(pArray, pElement) != 0 )
        {
            json_decref(pArray);
            STDLOG_ERROR("stdJSON_SetStringArray: Failed to add array element.\n");
            return false;
        }
    }

    bool result = stdJSON_SetValue(hJson, pKey, pArray);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

size_t J3DAPI stdJSON_GetIntArray(StdJSONHandle hJson, const char* pKey, int* pValues, size_t maxCount)
{
    if ( !hJson || !pKey || !pValues || maxCount == 0 )
    {
        STDLOG_ERROR("stdJSON_GetIntArray: Invalid parameters.\n");
        return 0;
    }

    json_t* pArray = stdJSON_GetValue(hJson, pKey);
    if ( !json_is_array(pArray) )
    {
        return 0;
    }

    size_t arraySize = json_array_size(pArray);
    size_t count = (arraySize < maxCount) ? arraySize : maxCount;

    for ( size_t i = 0; i < count; i++ )
    {
        json_t* pElement = json_array_get(pArray, i);
        if ( json_is_integer(pElement) )
        {
            pValues[i] = (int)json_integer_value(pElement);
        }
        else
        {
            pValues[i] = 0;
        }
    }

    return count;
}

size_t J3DAPI stdJSON_GetFloatArray(StdJSONHandle hJson, const char* pKey, float* pValues, size_t maxCount)
{
    if ( !hJson || !pKey || !pValues || maxCount == 0 )
    {
        STDLOG_ERROR("stdJSON_GetFloatArray: Invalid parameters.\n");
        return 0;
    }

    json_t* pArray = stdJSON_GetValue(hJson, pKey);
    if ( !pArray || !json_is_array(pArray) )
    {
        return 0;
    }

    size_t arraySize = json_array_size(pArray);
    size_t count = (arraySize < maxCount) ? arraySize : maxCount;

    for ( size_t i = 0; i < count; i++ )
    {
        json_t* pElement = json_array_get(pArray, i);
        if ( pElement && (json_is_real(pElement) || json_is_integer(pElement)) )
        {
            pValues[i] = json_is_real(pElement) ? (float)json_real_value(pElement) : (float)json_integer_value(pElement);
        }
        else
        {
            pValues[i] = 0.0f;
        }
    }

    return count;
}

size_t J3DAPI stdJSON_GetBoolArray(StdJSONHandle hJson, const char* pKey, bool* pValues, size_t maxCount)
{
    if ( !hJson || !pKey || !pValues || maxCount == 0 )
    {
        STDLOG_ERROR("stdJSON_GetBoolArray: Invalid parameters.\n");
        return 0;
    }

    json_t* pArray = stdJSON_GetValue(hJson, pKey);
    if ( !pArray || !json_is_array(pArray) )
    {
        return 0;
    }

    size_t arraySize = json_array_size(pArray);
    size_t count = (arraySize < maxCount) ? arraySize : maxCount;

    for ( size_t i = 0; i < count; i++ )
    {
        json_t* pElement = json_array_get(pArray, i);
        if ( json_is_boolean(pElement) )
        {
            pValues[i] = json_is_true(pElement);
        }
        else
        {
            pValues[i] = false;
        }
    }

    return count;
}

size_t J3DAPI stdJSON_GetStringArray(StdJSONHandle hJson, const char* pKey, char** ppValues, size_t maxCount, size_t maxStrLen)
{
    if ( !hJson || !pKey || !ppValues || maxCount == 0 || maxStrLen == 0 )
    {
        STDLOG_ERROR("stdJSON_GetStringArray: Invalid parameters.\n");
        return 0;
    }

    json_t* pArray = stdJSON_GetValue(hJson, pKey);
    if ( !pArray || !json_is_array(pArray) )
    {
        return 0;
    }

    size_t arraySize = json_array_size(pArray);
    size_t count = (arraySize < maxCount) ? arraySize : maxCount;

    for ( size_t i = 0; i < count; i++ )
    {
        json_t* pElement = json_array_get(pArray, i);
        const char* pStr = "";

        if ( json_is_string(pElement) )
        {
            pStr = json_string_value(pElement);
        }

        if ( !pStr ) pStr = "";
        stdUtil_StringCopy(ppValues[i], maxStrLen, pStr);
    }

    return count;
}

size_t J3DAPI stdJSON_GetArraySize(StdJSONHandle hJson, const char* pKey)
{
    if ( !hJson || !pKey )
    {
        return 0;
    }

    json_t* pArray = stdJSON_GetValue(hJson, pKey);
    if ( !pArray || !json_is_array(pArray) )
    {
        return 0;
    }

    return json_array_size(pArray);
}

size_t J3DAPI stdJSON_ArrayGetSize(StdJSONHandle hArray)
{
    if ( !hArray || !hArray->pRoot )
    {
        return 0;
    }

    return json_array_size(hArray->pRoot);
}

bool J3DAPI stdJSON_ArrayAppendInt(StdJSONHandle hArray, int value)
{
    if ( !hArray || !hArray->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendInt: Invalid array handle.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendInt: Destination is not a JSON array.\n");
        return false;
    }

    json_t* pValue = json_integer(value);
    if ( !pValue )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendInt: Failed to create JSON integer.\n");
        return false;
    }

    int result = json_array_append_new(hArray->pRoot, pValue);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendInt: Failed to append array element.\n");
        return false;
    }

    return true;
}

bool J3DAPI stdJSON_ArraySetInt(StdJSONHandle hArray, size_t index, int value)
{
    if ( !hArray || !hArray->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArraySetInt: Invalid array handle.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetInt: Destination is not a JSON array.\n");
        return false;
    }

    if ( index >= json_array_size(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetInt: Failed to set integer at index %zu: index out of bounds.\n", index);
        return false;
    }

    json_t* pValue = json_integer(value);
    if ( !pValue )
    {
        STDLOG_ERROR("stdJSON_ArraySetInt: Failed to create JSON integer.\n");
        return false;
    }

    int result = json_array_set_new(hArray->pRoot, index, pValue);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArraySetInt: Failed to set array element.\n");
        return false;
    }

    return true;
}

int J3DAPI stdJSON_ArrayGetInt(StdJSONHandle hArray, size_t index, int defaultValue)
{
    if ( !hArray || !hArray->pRoot )
    {
        return defaultValue;
    }

    json_t* pElement = json_array_get(hArray->pRoot, index);
    if ( !pElement || !json_is_integer(pElement) )
    {
        return defaultValue;
    }

    return (int)json_integer_value(pElement);
}

bool J3DAPI stdJSON_ArrayAppendFloat(StdJSONHandle hArray, float value)
{
    if ( !hArray || !hArray->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendFloat: Invalid array handle.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendFloat: Destination is not a JSON array.\n");
        return false;
    }

    json_t* pValue = json_real((double)value);
    if ( !pValue )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendFloat: Failed to create JSON real.\n");
        return false;
    }

    int result = json_array_append_new(hArray->pRoot, pValue);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendFloat: Failed to append array element.\n");
        return false;
    }

    return true;
}

bool J3DAPI stdJSON_ArraySetFloat(StdJSONHandle hArray, size_t index, float value)
{
    if ( !hArray || !hArray->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArraySetFloat: Invalid array handle.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetFloat: Destination is not a JSON array.\n");
        return false;
    }

    if ( index >= json_array_size(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetFloat: Failed to set float at index %zu: index out of bounds.\n", index);
        return false;
    }

    json_t* pValue = json_real((double)value);
    if ( !pValue )
    {
        STDLOG_ERROR("stdJSON_ArraySetFloat: Failed to create JSON real.\n");
        return false;
    }

    int result = json_array_set_new(hArray->pRoot, index, pValue);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArraySetFloat: Failed to set array element.\n");
        return false;
    }

    return true;
}

float J3DAPI stdJSON_ArrayGetFloat(StdJSONHandle hArray, size_t index, float defaultValue)
{
    if ( !hArray || !hArray->pRoot )
    {
        return defaultValue;
    }

    json_t* pElement = json_array_get(hArray->pRoot, index);
    if ( !pElement || (!json_is_real(pElement) && !json_is_integer(pElement)) )
    {
        return defaultValue;
    }

    return json_is_real(pElement) ? (float)json_real_value(pElement) : (float)json_integer_value(pElement);
}

bool J3DAPI stdJSON_ArrayAppendBool(StdJSONHandle hArray, bool value)
{
    if ( !hArray || !hArray->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendBool: Invalid array handle.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendBool: Destination is not a JSON array.\n");
        return false;
    }

    json_t* pValue = value ? json_true() : json_false();
    if ( !pValue )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendBool: Failed to create JSON boolean.\n");
        return false;
    }

    int result = json_array_append_new(hArray->pRoot, pValue);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendBool: Failed to append array element.\n");
        return false;
    }

    return true;
}

bool J3DAPI stdJSON_ArraySetBool(StdJSONHandle hArray, size_t index, bool value)
{
    if ( !hArray || !hArray->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArraySetBool: Invalid array handle.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetBool: Destination is not a JSON array.\n");
        return false;
    }

    if ( index >= json_array_size(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetBool: Failed to set boolean at index %zu: index out of bounds.\n", index);
        return false;
    }

    json_t* pValue = value ? json_true() : json_false();
    if ( !pValue )
    {
        STDLOG_ERROR("stdJSON_ArraySetBool: Failed to create JSON boolean.\n");
        return false;
    }

    int result = json_array_set_new(hArray->pRoot, index, pValue);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArraySetBool: Failed to set array element.\n");
        return false;
    }

    return true;
}

bool J3DAPI stdJSON_ArrayGetBool(StdJSONHandle hArray, size_t index, bool defaultValue)
{
    if ( !hArray || !hArray->pRoot )
    {
        return defaultValue;
    }

    json_t* pElement = json_array_get(hArray->pRoot, index);
    if ( !pElement || !json_is_boolean(pElement) )
    {
        return defaultValue;
    }

    return json_is_true(pElement);
}

bool J3DAPI stdJSON_ArrayAppendString(StdJSONHandle hArray, const char* pValue)
{
    if ( !hArray || !hArray->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendString: Invalid array handle.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendString: Destination is not a JSON array.\n");
        return false;
    }

    if ( !pValue ) pValue = "";

    json_t* pJsonValue = json_string(pValue);
    if ( !pJsonValue )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendString: Failed to create JSON string.\n");
        return false;
    }

    int result = json_array_append_new(hArray->pRoot, pJsonValue);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendString: Failed to append array element.\n");
        return false;
    }

    return true;
}

bool J3DAPI stdJSON_ArraySetString(StdJSONHandle hArray, size_t index, const char* pValue)
{
    if ( !hArray || !hArray->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArraySetString: Invalid array handle.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetString: Destination is not a JSON array.\n");
        return false;
    }

    if ( !pValue ) pValue = "";

    if ( index >= json_array_size(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetString: Failed to set string at index %zu: index out of bounds.\n", index);
        return false;
    }

    json_t* pJsonValue = json_string(pValue);
    if ( !pJsonValue )
    {
        STDLOG_ERROR("stdJSON_ArraySetString: Failed to create JSON string.\n");
        return false;
    }

    int result = json_array_set_new(hArray->pRoot, index, pJsonValue);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArraySetString: Failed to set array element.\n");
        return false;
    }

    return true;
}

bool J3DAPI stdJSON_ArrayGetString(StdJSONHandle hArray, size_t index, char* pDstStr, size_t size, const char* pDefaultValue)
{
    if ( !pDstStr || size == 0 )
    {
        STDLOG_ERROR("stdJSON_ArrayGetString: Invalid output buffer.\n");
        return false;
    }

    if ( !hArray || !hArray->pRoot )
    {
        if ( pDefaultValue )
        {
            stdUtil_StringCopy(pDstStr, size, pDefaultValue);
        }
        else
        {
            pDstStr[0] = '\0';
        }
        return false;
    }

    json_t* pElement = json_array_get(hArray->pRoot, index);
    const char* pStr = NULL;

    if ( json_is_string(pElement) )
    {
        pStr = json_string_value(pElement);
    }
    else
    {
        pStr = pDefaultValue;
    }

    if ( !pStr )
    {
        pDstStr[0] = '\0';
        return false;
    }

    stdUtil_StringCopy(pDstStr, size, pStr);
    return true;
}

bool J3DAPI stdJSON_ArrayAppendObject(StdJSONHandle hArray, StdJSONHandle hObjectToAppend)
{
    if ( !hArray || !hArray->pRoot || !hObjectToAppend || !hObjectToAppend->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendObject: Invalid parameters.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendObject: Destination is not a JSON array.\n");
        return false;
    }

    if ( !json_is_object(hObjectToAppend->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendObject: Source is not a JSON object.\n");
        return false;
    }

    // Create a deep copy of the object
    json_t* pObjectCopy = json_deep_copy(hObjectToAppend->pRoot);
    if ( !pObjectCopy )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendObject: Failed to copy JSON object.\n");
        return false;
    }

    int result = json_array_append_new(hArray->pRoot, pObjectCopy);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendObject: Failed to append array element.\n");
        return false;
    }

    return true;
}

bool J3DAPI stdJSON_ArraySetObject(StdJSONHandle hArray, size_t index, StdJSONHandle hObjectToSet)
{
    if ( !hArray || !hArray->pRoot || !hObjectToSet || !hObjectToSet->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArraySetObject: Invalid parameters.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetObject: Destination array is not a JSON array.\n");
        return false;
    }

    if ( !json_is_object(hObjectToSet->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetObject: Value is not a JSON object.\n");
        return false;
    }

    if ( index >= json_array_size(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetObject: Failed to set object at index %zu: index out of bounds.\n", index);
        return false;
    }

    // Create a deep copy of the object to avoid reference issues
    json_t* pObjectCopy = json_deep_copy(hObjectToSet->pRoot);
    if ( !pObjectCopy )
    {
        STDLOG_ERROR("stdJSON_ArraySetObject: Failed to copy JSON object.\n");
        return false;
    }

    int result = json_array_set_new(hArray->pRoot, index, pObjectCopy);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArraySetObject: Failed to set array element.\n");
        return false;
    }

    return true;
}

StdJSONHandle J3DAPI stdJSON_ArrayGetObject(StdJSONHandle hArray, size_t index)
{
    if ( !hArray || !hArray->pRoot )
    {
        return NULL;
    }

    json_t* pElement = json_array_get(hArray->pRoot, index);
    if ( !pElement || !json_is_object(pElement) )
    {
        return NULL;
    }

    return stdJSON_NewChild(pElement, hArray);
}

bool J3DAPI stdJSON_ArrayAppendArray(StdJSONHandle hArray, StdJSONHandle hArrayToAppend)
{
    if ( !hArray || !hArray->pRoot || !hArrayToAppend || !hArrayToAppend->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendArray: Invalid parameters.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendArray: Destination is not a JSON array.\n");
        return false;
    }

    // Create a deep copy of the array
    json_t* pArrayCopy = json_deep_copy(hArrayToAppend->pRoot);
    if ( !pArrayCopy )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendArray: Failed to copy JSON array.\n");
        return false;
    }

    int result = json_array_append_new(hArray->pRoot, pArrayCopy);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArrayAppendArray: Failed to append array element.\n");
        return false;
    }

    return true;
}

bool J3DAPI stdJSON_ArraySetArray(StdJSONHandle hArray, size_t index, StdJSONHandle hArrayToSet)
{
    if ( !hArray || !hArray->pRoot || !hArrayToSet || !hArrayToSet->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArraySetArray: Invalid parameters.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetArray: Destination is not a JSON array.\n");
        return false;
    }

    if ( !json_is_array(hArrayToSet->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetArray: Value is not a JSON array.\n");
        return false;
    }

    if ( index >= json_array_size(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArraySetArray: Failed to set array object at index %zu: index out of bounds.\n", index);
        return false;
    }

    // Create a deep copy of the array to avoid reference issues
    json_t* pArrayCopy = json_deep_copy(hArrayToSet->pRoot);
    if ( !pArrayCopy )
    {
        STDLOG_ERROR("stdJSON_ArraySetArray: Failed to copy JSON array.\n");
        return false;
    }

    int result = json_array_set_new(hArray->pRoot, index, pArrayCopy);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArraySetArray: Failed to set array element.\n");
        return false;
    }

    return true;
}

StdJSONHandle J3DAPI stdJSON_ArrayGetArray(StdJSONHandle hArray, size_t index)
{
    if ( !hArray || !hArray->pRoot )
    {
        return NULL;
    }

    json_t* pElement = json_array_get(hArray->pRoot, index);
    if ( !pElement || !json_is_array(pElement) )
    {
        return NULL;
    }

    return stdJSON_NewChild(pElement, hArray);
}

StdJSONType J3DAPI stdJSON_ArrayGetElementType(StdJSONHandle hArray, size_t index)
{
    if ( !hArray || !hArray->pRoot )
    {
        return STDJSON_TYPE_NULL;
    }

    json_t* pElement = json_array_get(hArray->pRoot, index);
    return stdJSON_GetJSONType(pElement);
}

bool J3DAPI stdJSON_ArrayRemoveElement(StdJSONHandle hArray, size_t index)
{
    if ( !hArray || !hArray->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArrayRemoveElement: Invalid array handle.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArrayRemoveElement: Destination is not a JSON array.\n");
        return false;
    }

    size_t arraySize = json_array_size(hArray->pRoot);
    if ( index >= arraySize )
    {
        STDLOG_WARNING("stdJSON_ArrayRemoveElement: Index %zu out of bounds (size: %zu).\n", index, arraySize);
        return false;
    }

    int result = json_array_remove(hArray->pRoot, index);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArrayRemoveElement: Failed to remove array element.\n");
        return false;
    }

    return true;
}

bool J3DAPI stdJSON_ArrayClear(StdJSONHandle hArray)
{
    if ( !hArray || !hArray->pRoot )
    {
        STDLOG_ERROR("stdJSON_ArrayClear: Invalid array handle.\n");
        return false;
    }

    if ( !json_is_array(hArray->pRoot) )
    {
        STDLOG_ERROR("stdJSON_ArrayClear: Destination is not a JSON array.\n");
        return false;
    }

    int result = json_array_clear(hArray->pRoot);
    if ( result != 0 )
    {
        STDLOG_ERROR("stdJSON_ArrayClear: Failed to clear array.\n");
        return false;
    }

    return true;
}

bool J3DAPI stdJSON_SetObjectArrayElement(StdJSONHandle hJson, const char* pKey, size_t index, StdJSONHandle value)
{
    if ( !hJson || !value || !value->pRoot || !json_is_object(value->pRoot) )
    {
        return false;
    }

    json_incref(value->pRoot); // Increase ref count as stdJSON_SetArrayValue might decref on failure
    bool result = stdJSON_SetArrayValue(hJson, pKey, index, value->pRoot);
    if ( result )
    {
        json_decref(value->pRoot); // Decrease ref count as function succeeded
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_SetIntArrayElement(StdJSONHandle hJson, const char* pKey, size_t index, int value)
{
    json_t* pElement = json_integer(value);
    if ( !pElement )
    {
        return false;
    }

    bool result = stdJSON_SetArrayValue(hJson, pKey, index, pElement);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_SetFloatArrayElement(StdJSONHandle hJson, const char* pKey, size_t index, float value)
{
    json_t* pElement = json_real((double)value);
    if ( !pElement )
    {
        return false;
    }

    bool result = stdJSON_SetArrayValue(hJson, pKey, index, pElement);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_SetBoolArrayElement(StdJSONHandle hJson, const char* pKey, size_t index, bool value)
{
    json_t* pElement = value ? json_true() : json_false();
    if ( !pElement )
    {
        return false;
    }

    bool result = stdJSON_SetArrayValue(hJson, pKey, index, pElement);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

bool J3DAPI stdJSON_SetStringArrayElement(StdJSONHandle hJson, const char* pKey, size_t index, const char* pValue)
{
    if ( !pValue )
    {
        return false;
    }

    json_t* pElement = json_string(pValue);
    if ( !pElement )
    {
        return false;
    }

    bool result = stdJSON_SetArrayValue(hJson, pKey, index, pElement);
    if ( result )
    {
        stdJSON_AutoSave(hJson);
    }
    return result;
}

StdJSONHandle J3DAPI stdJSON_GetObjectArrayElement(StdJSONHandle hJson, const char* pKey, size_t index)
{
    json_t* pObj = stdJSON_GetArrayValue(hJson, pKey, index);
    if ( !pObj || !json_is_object(pObj) )
    {
        return NULL;
    }

    return stdJSON_NewChild(pObj, hJson);
}

int J3DAPI stdJSON_GetIntArrayElement(StdJSONHandle hJson, const char* pKey, size_t index, int defaultValue)
{
    json_t* pElement = stdJSON_GetArrayValue(hJson, pKey, index);
    if ( !pElement || !json_is_integer(pElement) )
    {
        return defaultValue;
    }
    return (int)json_integer_value(pElement);
}

float J3DAPI stdJSON_GetFloatArrayElement(StdJSONHandle hJson, const char* pKey, size_t index, float defaultValue)
{
    json_t* pElement = stdJSON_GetArrayValue(hJson, pKey, index);
    if ( !pElement || (!json_is_real(pElement) && !json_is_integer(pElement)) )
    {
        return defaultValue;
    }
    return json_is_real(pElement) ? (float)json_real_value(pElement) : (float)json_integer_value(pElement);
}

bool J3DAPI stdJSON_GetBoolArrayElement(StdJSONHandle hJson, const char* pKey, size_t index, bool defaultValue)
{
    json_t* pElement = stdJSON_GetArrayValue(hJson, pKey, index);
    if ( !pElement || !json_is_boolean(pElement) )
    {
        return defaultValue;
    }
    return json_is_true(pElement);
}

bool J3DAPI stdJSON_GetStringArrayElement(StdJSONHandle hJson, const char* pKey, size_t index, char* pDstStr, size_t size, const char* pDefaultValue)
{
    if ( !pDstStr || size == 0 )
    {
        STDLOG_ERROR("stdJSON_GetStringArrayElement: Invalid output buffer.\n");
        return false;
    }

    json_t* pElement = stdJSON_GetArrayValue(hJson, pKey, index);
    const char* pStr = NULL;

    if ( json_is_string(pElement) )
    {
        pStr = json_string_value(pElement);
    }
    else
    {
        pStr = pDefaultValue;
    }

    if ( !pStr )
    {
        pDstStr[0] = '\0';
        return false;
    }

    stdUtil_StringCopy(pDstStr, size, pStr);
    return true;
}

// Type checking
StdJSONType J3DAPI stdJSON_GetValueType(StdJSONHandle hJson, const char* pKey)
{
    json_t* pValue = stdJSON_GetValue(hJson, pKey);
    return stdJSON_GetJSONType(pValue);
}

bool J3DAPI stdJSON_HasKey(StdJSONHandle hJson, const char* pKey)
{
    return stdJSON_GetValue(hJson, pKey) != NULL;
}

size_t J3DAPI stdJSON_EnumerateKeys(StdJSONHandle hJson, const char* pPath, StdJSONEnumKeyCallback callback, void* pUserData)
{
    if ( !hJson || !callback )
    {
        STDLOG_ERROR("stdJSON_EnumerateKeys: Invalid parameters.\n");
        return 0;
    }

    json_t* pObject = hJson->pRoot;
    if ( pPath && strlen(pPath) > 0 )
    {
        pObject = stdJSON_GetNestedValue(hJson->pRoot, pPath);
    }

    if ( !pObject || !json_is_object(pObject) )
    {
        return 0;
    }

    const char* key;
    json_t* value;
    size_t count = 0;
    json_object_foreach(pObject, key, value)
    {
        if ( !callback(key, pUserData) )
        {
            break;
        }
        count++;
    }

    return count;
}

size_t J3DAPI stdJSON_GetKeys(StdJSONHandle hJson, const char* pPath, char** ppKeys, size_t maxKeys)
{
    if ( !hJson || !ppKeys || maxKeys == 0 )
    {
        STDLOG_ERROR("stdJSON_GetKeys: Invalid parameters.\n");
        return 0;
    }

    json_t* pObject = hJson->pRoot;
    if ( pPath && strlen(pPath) > 0 )
    {
        pObject = stdJSON_GetNestedValue(hJson->pRoot, pPath);
    }

    if ( !pObject || !json_is_object(pObject) )
    {
        return 0;
    }

    const char* key;
    json_t* value;
    size_t count = 0;
    json_object_foreach(pObject, key, value)
    {
        if ( count >= maxKeys )
        {
            break;
        }

        size_t keySize = strlen(key) + 1;
        ppKeys[count] = (char*)STDMALLOC(keySize);
        if ( ppKeys[count] )
        {
            stdUtil_StringCopy(ppKeys[count], keySize, key);
            count++;
        }
    }

    return count;
}

static json_t* stdJSON_GetValue(StdJSONHandle hJson, const char* pKey)
{
    if ( !hJson || !hJson->pRoot || !pKey )
    {
        return NULL;
    }

    // Check if path contains dots (nested access)
    if ( strchr(pKey, '.') != NULL )
    {
        return stdJSON_GetNestedValue(hJson->pRoot, pKey);
    }

    return json_object_get(hJson->pRoot, pKey);
}

static bool stdJSON_SetValue(StdJSONHandle hJson, const char* pKey, json_t* pValue)
{
    if ( !hJson || !hJson->pRoot || !pKey || !pValue )
    {
        if ( pValue ) json_decref(pValue);
        return false;
    }

    bool bSuccess;

    // Check if path contains dots (nested access)
    if ( strchr(pKey, '.') != NULL )
    {
        bSuccess = stdJSON_SetNestedValue(hJson->pRoot, pKey, pValue);
    }
    else
    {
        bSuccess = json_object_set_new(hJson->pRoot, pKey, pValue) == 0;
    }

    if ( !bSuccess )
    {
        STDLOG_ERROR("stdJSON_SetValue: Failed to set key '%s'\n", pKey);
        return false;
    }

    hJson->bModified = true;
    return true;
}

// Helper function to get nested value using dot notation
static json_t* stdJSON_GetNestedValue(json_t* pRoot, const char* pPath)
{
    if ( !pRoot || !pPath )
    {
        return NULL;
    }

    // Make a copy of path for tokenization
    size_t pathSize = strlen(pPath) + 1;
    char* pathCopy = (char*)STDMALLOC(pathSize);
    if ( !pathCopy )
    {
        return NULL;
    }
    stdUtil_StringCopy(pathCopy, pathSize, pPath);

    json_t* current = pRoot;
    char* token = pathCopy;
    char* nextDot;

    while ( token && current )
    {
// Find next dot
        nextDot = strchr(token, '.');
        if ( nextDot )
        {
            *nextDot = '\0';
        }

        if ( !json_is_object(current) )
        {
            current = NULL;
            break;
        }

        current = json_object_get(current, token);

        if ( nextDot )
        {
            token = nextDot + 1;
        }
        else
        {
            break;
        }
    }

    STDFREE(pathCopy);
    return current;
}

// Helper function to set nested value using dot notation
static bool stdJSON_SetNestedValue(json_t* pRoot, const char* pPath, json_t* pValue)
{
    if ( !pRoot || !pPath || !pValue )
    {
        return false;
    }

    // Make a copy of path for tokenization
    size_t pathSize = strlen(pPath) + 1;
    char* pathCopy = (char*)STDMALLOC(pathSize);
    if ( !pathCopy )
    {
        return false;
    }
    stdUtil_StringCopy(pathCopy, pathSize, pPath);

    json_t* current = pRoot;
    char* token = pathCopy;
    char* nextDot;

    // Navigate to the parent object
    while ( token )
    {
        nextDot = strchr(token, '.');
        if ( nextDot )
        {
            *nextDot = '\0';
        }

        if ( !nextDot )
        {
            // This is the final key, set the value
            int result = json_object_set_new(current, token, pValue);
            STDFREE(pathCopy);
            return result == 0;
        }

        // Navigate to next level
        if ( !json_is_object(current) )
        {
            STDFREE(pathCopy);
            return false;
        }

        json_t* next = json_object_get(current, token);
        if ( !next )
        {
            // Create intermediate object
            next = json_object();
            if ( !next || json_object_set_new(current, token, next) != 0 )
            {
                STDFREE(pathCopy);
                return false;
            }
        }
        else if ( !json_is_object(next) )
        {
            STDFREE(pathCopy);
            return false;
        }

        current = next;
        token = nextDot + 1;
    }

    STDFREE(pathCopy);
    return false;
}

static json_t* stdJSON_GetArrayValue(StdJSONHandle hJson, const char* pKey, size_t index)
{
    if ( !hJson || !pKey )
    {
        return NULL;
    }

    json_t* pArray = stdJSON_GetValue(hJson, pKey);
    if ( !pArray || !json_is_array(pArray) )
    {
        return NULL;
    }

    if ( index >= json_array_size(pArray) )
    {
        return NULL;
    }

    return json_array_get(pArray, index);
}

static bool stdJSON_SetArrayValue(StdJSONHandle hJson, const char* pKey, size_t index, json_t* pValue)
{
    if ( !hJson || !pKey || !pValue )
    {
        if ( pValue ) json_decref(pValue);
        return false;
    }

    json_t* pArray = stdJSON_GetValue(hJson, pKey);
    if ( !json_is_array(pArray) )
    {
        // Create array if it doesn't exist
        pArray = json_array();
        if ( !pArray )
        {
            json_decref(pValue);
            return false;
        }

        int setResult = stdJSON_SetValue(hJson, pKey, pArray);
        if ( !setResult )
        {
            json_decref(pValue);
            return false;
        }
        pArray = stdJSON_GetValue(hJson, pKey); // Get the reference after setting
    }

    // Expand array if necessary
    size_t currentSize = json_array_size(pArray);
    while ( currentSize <= index )
    {
        if ( json_array_append(pArray, json_null()) != 0 )
        {
            json_decref(pValue);
            return false;
        }
        currentSize++;
    }

    int result = json_array_set_new(pArray, index, pValue);
    if ( result == 0 )
    {
        hJson->bModified = true;
        return true;
    }

    return false;
}

void J3DAPI stdJSON_SetParent(StdJSONHandle hJson, StdJSONHandle hParent)
{
    if ( !hJson )
    {
        STDLOG_ERROR("stdJSON_SetParent: Invalid child handle.\n");
        return;
    }

    if ( !hParent || !hParent->pRoot )
    {
        STDLOG_ERROR("stdJSON_SetParent: Invalid parent handle.\n");
        return;
    }

    if ( hJson->pParent == hParent )
    {
        return; // No change
    }
    // Clear existing parent
    stdJSON_ClearParent(hJson);

    json_incref(hParent->pRoot); // Increase ref count for parent
    hJson->pParent = hParent;
}

StdJSONHandle J3DAPI stdJSON_GetParent(StdJSONHandle hJson)
{
    if ( !hJson )
    {
        return NULL;
    }
    return hJson->pParent;
}

void J3DAPI stdJSON_ClearParent(StdJSONHandle hJson)
{
    if ( !hJson || !hJson->pParent )
    {
        return;
    }

    if ( hJson->pParent->pRoot )
    {
        json_decref(hJson->pParent->pRoot); // Decrease ref count for current root
    }
    hJson->pParent = NULL;
}

StdJSONHandle stdJSON_GetRoot(StdJSONHandle hJson)
{
    StdJSONHandle pCur = hJson;
    while ( pCur && pCur->pParent )
    {
        pCur = pCur->pParent;
    }
    return pCur;
}

static void stdJSON_FreeRequiredKeys(StdJSONHandle hJson)
{
    if ( !hJson || !hJson->ppRequiredKeys )
    {
        return;
    }

    for ( size_t i = 0; i < hJson->numRequiredKeys; i++ )
    {
        if ( hJson->ppRequiredKeys[i] )
        {
            STDFREE(hJson->ppRequiredKeys[i]);
        }
    }

    STDFREE(hJson->ppRequiredKeys);
    hJson->ppRequiredKeys  = NULL;
    hJson->numRequiredKeys = 0;
}