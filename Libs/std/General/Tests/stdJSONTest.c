#include <unity_fixture.h>

#include <stdio.h>
#include <string.h>

#include <std/General/std.h>
#include <std/General/stdJSON.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

typedef struct stdJSONTestKeyCollector
{
    const char* apKeys[8];
    size_t numKeys;
    size_t stopAfter;
} stdJSONTestKeyCollector;

static bool stdJSONTest_HasKey(char** ppKeys, size_t numKeys, const char* pExpected)
{
    for ( size_t i = 0; i < numKeys; ++i )
    {
        if ( strcmp(ppKeys[i], pExpected) == 0 )
        {
            return true;
        }
    }

    return false;
}

static bool stdJSONTest_CollectKey(const char* pKey, void* pUserData)
{
    stdJSONTestKeyCollector* pCollector = (stdJSONTestKeyCollector*)pUserData;

    pCollector->apKeys[pCollector->numKeys++] = pKey;
    return !pCollector->stopAfter || pCollector->numKeys < pCollector->stopAfter;
}

static bool stdJSONTest_IsStructuralJsonChar(char ch)
{
    return ch == '{' || ch == '}' || ch == '[' || ch == ']' || ch == ':' || ch == ',' || ch == '"';
}

static void stdJSONTest_DeleteFile(const char* pPath)
{
    StdGeneralTest_DeleteFile(pPath);
}

TEST_GROUP(stdJSON);

TEST_SETUP(stdJSON)
{
    StdGeneralTest_Startup();
    TEST_ASSERT_TRUE(stdJSON_Startup());
}

TEST_TEAR_DOWN(stdJSON)
{
    stdJSONTest_DeleteFile("stdJSONTest_autosave.json");
    if ( stdJSON_HasStarted() )
    {
        stdJSON_Shutdown();
    }
    StdGeneralTest_Shutdown();
}

TEST(stdJSON, TestCreateSetGetTypesSerializeDeleteAndClear)
{
    StdJSONHandle hJson = stdJSON_New();
    char aText[32];
    char* pJsonText;

    TEST_ASSERT_NOT_NULL(hJson);
    TEST_ASSERT_TRUE(stdJSON_IsValid(hJson));
    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_OBJECT, stdJSON_GetType(hJson));

    TEST_ASSERT_TRUE(stdJSON_SetBool(hJson, "graphics.window", true));
    TEST_ASSERT_TRUE(stdJSON_SetInt(hJson, "graphics.width", 1280));
    TEST_ASSERT_TRUE(stdJSON_SetFloat(hJson, "graphics.gamma", 1.25f));
    TEST_ASSERT_TRUE(stdJSON_SetString(hJson, "player.name", "Indy"));

    TEST_ASSERT_TRUE(stdJSON_GetBool(hJson, "graphics.window", false));
    TEST_ASSERT_EQUAL_INT(1280, stdJSON_GetInt(hJson, "graphics.width", 0));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.25f, stdJSON_GetFloat(hJson, "graphics.gamma", 0.0f));
    TEST_ASSERT_TRUE(stdJSON_GetString(hJson, "player.name", aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("Indy", aText);

    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_OBJECT, stdJSON_GetValueType(hJson, "graphics"));
    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_BOOL, stdJSON_GetValueType(hJson, "graphics.window"));
    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_INTEGER, stdJSON_GetValueType(hJson, "graphics.width"));
    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_REAL, stdJSON_GetValueType(hJson, "graphics.gamma"));
    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_STRING, stdJSON_GetValueType(hJson, "player.name"));
    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_NULL, stdJSON_GetValueType(hJson, "missing"));

    TEST_ASSERT_EQUAL_INT(77, stdJSON_GetInt(hJson, "graphics.window", 77));
    TEST_ASSERT_FALSE(stdJSON_GetString(hJson, "missing", aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("", aText);
    TEST_ASSERT_TRUE(stdJSON_GetString(hJson, "missing", aText, sizeof(aText), "default"));
    TEST_ASSERT_EQUAL_STRING("default", aText);

    pJsonText = stdJSON_ToString(hJson, true);
    TEST_ASSERT_NOT_NULL(pJsonText);
    TEST_ASSERT_EQUAL_STRING(
        "{\n  \"graphics\": {\n    \"window\": true,\n    \"width\": 1280,\n"
        "    \"gamma\": 1.25\n  },\n  \"player\": {\n    \"name\": \"Indy\"\n  }\n}",
        pJsonText
    );
    STDFREE(pJsonText);

    TEST_ASSERT_TRUE(stdJSON_Delete(hJson, "graphics.width"));
    TEST_ASSERT_FALSE(stdJSON_HasKey(hJson, "graphics.width"));
    TEST_ASSERT_FALSE(stdJSON_Delete(hJson, "graphics.width"));

    TEST_ASSERT_TRUE(stdJSON_Clear(hJson));
    TEST_ASSERT_FALSE(stdJSON_HasKey(hJson, "graphics.window"));

    stdJSON_Free(hJson);
}

TEST(stdJSON, TestLoadSaveLoadEntryDuplicateMergeAndRequiredKeys)
{
    const char* pPath = "stdJSONTest_save.json";
    const char* apRequiredKeys[] = { "name", "stats.health" };
    char aDocumentPath[512];
    char aMergePath[512];
    char aRootArrayPath[512];
    char aInvalidSyntaxPath[512];
    char aTrailingGarbagePath[512];
    char aUnclosedDeepObjectPath[512];
    StdJSONHandle hJson;
    StdJSONHandle hLoaded;
    StdJSONHandle hDuplicate;
    StdJSONHandle hMerge;

    STD_FORMAT(aDocumentPath, "%s", StdGeneralTest_GetTestVectorPath("stdJSON/document.json"));
    STD_FORMAT(aMergePath, "%s", StdGeneralTest_GetTestVectorPath("stdJSON/merge.json"));
    STD_FORMAT(aRootArrayPath, "%s", StdGeneralTest_GetTestVectorPath("stdJSON/root_array.json"));
    STD_FORMAT(aInvalidSyntaxPath, "%s", StdGeneralTest_GetTestVectorPath("stdJSON/invalid_syntax.json"));
    STD_FORMAT(aTrailingGarbagePath, "%s", StdGeneralTest_GetTestVectorPath("stdJSON/trailing_garbage.json"));
    STD_FORMAT(aUnclosedDeepObjectPath, "%s", StdGeneralTest_GetTestVectorPath("stdJSON/unclosed_deep_object.json"));
    stdJSONTest_DeleteFile(pPath);

    hJson = stdJSON_Load(aDocumentPath);
    TEST_ASSERT_NOT_NULL(hJson);
    TEST_ASSERT_TRUE(stdJSON_SetRequiredKeys(hJson, apRequiredKeys, STD_ARRAYLEN(apRequiredKeys)));
    TEST_ASSERT_TRUE(stdJSON_ValidateRequired(hJson));
    TEST_ASSERT_TRUE(stdJSON_Delete(hJson, "stats.health"));
    TEST_ASSERT_FALSE(stdJSON_ValidateRequired(hJson));
    TEST_ASSERT_TRUE(stdJSON_SetInt(hJson, "stats.health", 80));

    TEST_ASSERT_TRUE(stdJSON_Save(hJson, pPath));
    hLoaded = stdJSON_Load(pPath);
    TEST_ASSERT_NOT_NULL(hLoaded);
    TEST_ASSERT_EQUAL_INT(80, stdJSON_GetInt(hLoaded, "stats.health", 0));

    hDuplicate = stdJSON_Duplicate(hLoaded);
    TEST_ASSERT_NOT_NULL(hDuplicate);
    TEST_ASSERT_TRUE(stdJSON_SetString(hDuplicate, "name", "sophia"));
    TEST_ASSERT_TRUE(stdJSON_GetString(hDuplicate, "name", std_g_genBuffer, sizeof(std_g_genBuffer), NULL));
    TEST_ASSERT_EQUAL_STRING("sophia", std_g_genBuffer);
    TEST_ASSERT_TRUE(stdJSON_GetString(hLoaded, "name", std_g_genBuffer, sizeof(std_g_genBuffer), NULL));
    TEST_ASSERT_EQUAL_STRING("indy", std_g_genBuffer);

    hMerge = stdJSON_Load(aMergePath);
    TEST_ASSERT_NOT_NULL(hMerge);
    TEST_ASSERT_TRUE(stdJSON_Merge(hLoaded, hMerge, false));
    TEST_ASSERT_EQUAL_INT(1, stdJSON_GetInt(hLoaded, "keep", 0));
    TEST_ASSERT_EQUAL_INT(123, stdJSON_GetInt(hLoaded, "newValue", 0));
    TEST_ASSERT_TRUE(stdJSON_Merge(hLoaded, hMerge, true));
    TEST_ASSERT_EQUAL_INT(99, stdJSON_GetInt(hLoaded, "keep", 0));

    TEST_ASSERT_NULL(stdJSON_Load(aRootArrayPath));
    TEST_ASSERT_NULL(stdJSON_Load(aTrailingGarbagePath));
    TEST_ASSERT_NULL(stdJSON_Load(aUnclosedDeepObjectPath));
    TEST_ASSERT_FALSE(stdJSON_LoadEntry(aInvalidSyntaxPath, hLoaded));
    TEST_ASSERT_EQUAL_INT(99, stdJSON_GetInt(hLoaded, "keep", 0));

    stdJSON_Free(hMerge);
    stdJSON_Free(hDuplicate);
    stdJSON_Free(hLoaded);
    stdJSON_Free(hJson);
    stdJSONTest_DeleteFile(pPath);
}

TEST(stdJSON, TestNestedObjectVectorsAndPathCollisions)
{
    StdJSONHandle hJson = stdJSON_Load(StdGeneralTest_GetTestVectorPath("stdJSON/nested.json"));
    StdJSONHandle hRooms;
    char* apKeys[4] = { 0 };
    char aText[32];
    size_t numKeys;

    TEST_ASSERT_NOT_NULL(hJson);
    TEST_ASSERT_TRUE(stdJSON_GetString(hJson, "world.levels.temple.rooms.entry.name", aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("Entry Hall", aText);
    TEST_ASSERT_TRUE(stdJSON_GetBool(hJson, "world.levels.temple.rooms.entry.flags.hasPuzzle", false));
    TEST_ASSERT_FALSE(stdJSON_GetBool(hJson, "world.levels.temple.rooms.entry.flags.hasWater", true));
    TEST_ASSERT_EQUAL_INT(7, stdJSON_GetInt(hJson, "world.levels.temple.rooms.vault.stats.difficulty", 0));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.625f, stdJSON_GetFloat(hJson, "world.levels.temple.rooms.vault.stats.ambient", 0.0f));
    TEST_ASSERT_EQUAL_INT(4, stdJSON_GetInt(hJson, "matrix.row1.col1", 0));

    hRooms = stdJSON_GetObject(hJson, "world.levels.temple.rooms");
    TEST_ASSERT_NOT_NULL(hRooms);
    numKeys = stdJSON_GetKeys(hRooms, NULL, apKeys, STD_ARRAYLEN(apKeys));
    TEST_ASSERT_EQUAL_size_t(2u, numKeys);
    TEST_ASSERT_TRUE(stdJSONTest_HasKey(apKeys, numKeys, "entry"));
    TEST_ASSERT_TRUE(stdJSONTest_HasKey(apKeys, numKeys, "vault"));
    for ( size_t i = 0; i < numKeys; ++i )
    {
        STDFREE(apKeys[i]);
    }
    stdJSON_Free(hRooms);

    stdJSON_Free(hJson);
}

TEST(stdJSON, TestLoadFromStringRejectsBrokenOrNonObjectJson)
{
    StdJSONHandle hJson;

    TEST_ASSERT_NULL(stdJSON_LoadFromString("[1, 2, 3]"));
    TEST_ASSERT_NULL(stdJSON_LoadFromString("{\"bad\": "));
    TEST_ASSERT_NULL(stdJSON_LoadFromString("{\"ok\": true} []"));

    hJson = stdJSON_LoadFromString("{\"inline\":{\"nested\":{\"value\":5}}}");
    TEST_ASSERT_NOT_NULL(hJson);
    TEST_ASSERT_EQUAL_INT(5, stdJSON_GetInt(hJson, "inline.nested.value", 0));
    stdJSON_Free(hJson);
}

TEST(stdJSON, TestObjectsArraysParentsAndKeyEnumeration)
{
    StdJSONHandle hJson = stdJSON_Load(StdGeneralTest_GetTestVectorPath("stdJSON/document.json"));
    StdJSONHandle hObject;
    StdJSONHandle hArray;
    char* apKeys[4] = { 0 };
    stdJSONTestKeyCollector collector = { 0 };
    size_t numKeys;

    TEST_ASSERT_NOT_NULL(hJson);
    hObject = stdJSON_GetObject(hJson, "object");
    hArray = stdJSON_GetArray(hJson, "array");
    TEST_ASSERT_NOT_NULL(hObject);
    TEST_ASSERT_NOT_NULL(hArray);
    TEST_ASSERT_EQUAL_PTR(hJson, stdJSON_GetParent(hObject));
    TEST_ASSERT_EQUAL_PTR(hJson, stdJSON_GetRoot(hObject));
    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_OBJECT, stdJSON_GetType(hObject));
    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_ARRAY, stdJSON_GetType(hArray));
    TEST_ASSERT_NULL(stdJSON_GetObject(hJson, "array"));
    TEST_ASSERT_NULL(stdJSON_GetArray(hJson, "object"));

    numKeys = stdJSON_GetKeys(hJson, "group", apKeys, STD_ARRAYLEN(apKeys));
    TEST_ASSERT_EQUAL_size_t(2u, numKeys);
    TEST_ASSERT_TRUE(stdJSONTest_HasKey(apKeys, numKeys, "a"));
    TEST_ASSERT_TRUE(stdJSONTest_HasKey(apKeys, numKeys, "b"));
    for ( size_t i = 0; i < numKeys; ++i )
    {
        STDFREE(apKeys[i]);
    }

    TEST_ASSERT_EQUAL_size_t(2u, stdJSON_EnumerateKeys(hJson, "group", stdJSONTest_CollectKey, &collector));
    TEST_ASSERT_EQUAL_size_t(2u, collector.numKeys);
    TEST_ASSERT_EQUAL_STRING("a", collector.apKeys[0]);
    TEST_ASSERT_EQUAL_STRING("b", collector.apKeys[1]);

    collector.numKeys = 0;
    collector.stopAfter = 1;
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_EnumerateKeys(hJson, "array", stdJSONTest_CollectKey, &collector));
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_EnumerateKeys(hJson, "group", stdJSONTest_CollectKey, &collector));
    TEST_ASSERT_EQUAL_size_t(1u, collector.numKeys);
    TEST_ASSERT_EQUAL_STRING("a", collector.apKeys[0]);

    stdJSON_ClearParent(hObject);
    TEST_ASSERT_NULL(stdJSON_GetParent(hObject));
    TEST_ASSERT_EQUAL_PTR(hObject, stdJSON_GetRoot(hObject));

    stdJSON_Free(hObject);
    stdJSON_Free(hArray);
    stdJSON_Free(hJson);
}

TEST(stdJSON, TestFreeEntryAutoSaveAndExplicitParent)
{
    const char* pPath            = "stdJSONTest_autosave.json";
    const char* apRequiredKeys[] = { "value" };
    StdJSONHandle hJson        = stdJSON_New();
    StdJSONHandle hParent      = stdJSON_New();
    StdJSONHandle hOtherParent = stdJSON_New();
    StdJSONHandle hLoaded;

    TEST_ASSERT_NOT_NULL(hJson);
    TEST_ASSERT_NOT_NULL(hParent);
    TEST_ASSERT_NOT_NULL(hOtherParent);
    TEST_ASSERT_TRUE(stdJSON_SetRequiredKeys(hJson, apRequiredKeys, STD_ARRAYLEN(apRequiredKeys)));
    TEST_ASSERT_TRUE(stdJSON_SetInt(hJson, "value", 1));
    TEST_ASSERT_TRUE(stdJSON_Save(hJson, pPath));

    stdJSON_SetAutoSave(hJson, true);
    TEST_ASSERT_TRUE(stdJSON_SetInt(hJson, "value", 2));
    hLoaded = stdJSON_Load(pPath);
    TEST_ASSERT_NOT_NULL(hLoaded);
    TEST_ASSERT_EQUAL_INT(2, stdJSON_GetInt(hLoaded, "value", 0));
    stdJSON_Free(hLoaded);

    stdJSON_SetAutoSave(hJson, false);
    TEST_ASSERT_TRUE(stdJSON_SetInt(hJson, "value", 3));
    hLoaded = stdJSON_Load(pPath);
    TEST_ASSERT_NOT_NULL(hLoaded);
    TEST_ASSERT_EQUAL_INT(2, stdJSON_GetInt(hLoaded, "value", 0));
    stdJSON_Free(hLoaded);

    stdJSON_SetParent(hJson, hParent);
    TEST_ASSERT_EQUAL_PTR(hParent, stdJSON_GetParent(hJson));
    TEST_ASSERT_EQUAL_PTR(hParent, stdJSON_GetRoot(hJson));
    stdJSON_SetParent(hJson, hParent);
    TEST_ASSERT_EQUAL_PTR(hParent, stdJSON_GetParent(hJson));

    stdJSON_SetParent(hJson, hOtherParent);
    TEST_ASSERT_EQUAL_PTR(hOtherParent, stdJSON_GetParent(hJson));
    TEST_ASSERT_EQUAL_PTR(hOtherParent, stdJSON_GetRoot(hJson));

    stdJSON_SetParent(NULL, hParent);
    stdJSON_SetParent(hJson, NULL);
    TEST_ASSERT_EQUAL_PTR(hOtherParent, stdJSON_GetParent(hJson));
    stdJSON_SetAutoSave(NULL, true);
    stdJSON_FreeEntry(NULL);

    stdJSON_FreeEntry(hJson);
    TEST_ASSERT_FALSE(stdJSON_IsValid(hJson));
    TEST_ASSERT_NULL(stdJSON_GetParent(hJson));
    TEST_ASSERT_EQUAL_PTR(hJson, stdJSON_GetRoot(hJson));

    stdJSON_Free(hJson);
    stdJSON_Free(hOtherParent);
    stdJSON_Free(hParent);
}

TEST(stdJSON, TestBulkArraysAndSparseElementAccessors)
{
    StdJSONHandle hJson = stdJSON_New();
    StdJSONHandle hObject = stdJSON_New();
    StdJSONHandle hGotObject;
    const int aInts[] = { 1, 2, 3 };
    const float aFloats[] = { 1.5f, 2.5f };
    const bool aBools[] = { true, false, true };
    const char* apStrings[] = { "alpha", NULL, "gamma" };
    int aOutInts[4] = { 0 };
    float aOutFloats[4] = { 0 };
    bool aOutBools[4] = { 0 };
    char aString0[16];
    char aString1[16];
    char aString2[16];
    char* apOutStrings[] = { aString0, aString1, aString2 };

    TEST_ASSERT_NOT_NULL(hJson);
    TEST_ASSERT_NOT_NULL(hObject);
    TEST_ASSERT_TRUE(stdJSON_SetString(hObject, "name", "artifact"));

    TEST_ASSERT_TRUE(stdJSON_SetIntArray(hJson, "arrays.ints", aInts, STD_ARRAYLEN(aInts)));
    TEST_ASSERT_TRUE(stdJSON_SetFloatArray(hJson, "arrays.floats", aFloats, STD_ARRAYLEN(aFloats)));
    TEST_ASSERT_TRUE(stdJSON_SetBoolArray(hJson, "arrays.bools", aBools, STD_ARRAYLEN(aBools)));
    TEST_ASSERT_TRUE(stdJSON_SetStringArray(hJson, "arrays.strings", apStrings, STD_ARRAYLEN(apStrings)));

    TEST_ASSERT_EQUAL_size_t(3u, stdJSON_GetArraySize(hJson, "arrays.ints"));
    TEST_ASSERT_EQUAL_size_t(2u, stdJSON_GetIntArray(hJson, "arrays.ints", aOutInts, 2u));
    TEST_ASSERT_EQUAL_INT(1, aOutInts[0]);
    TEST_ASSERT_EQUAL_INT(2, aOutInts[1]);
    TEST_ASSERT_EQUAL_size_t(2u, stdJSON_GetFloatArray(hJson, "arrays.floats", aOutFloats, STD_ARRAYLEN(aOutFloats)));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.5f, aOutFloats[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.5f, aOutFloats[1]);
    TEST_ASSERT_EQUAL_size_t(3u, stdJSON_GetBoolArray(hJson, "arrays.bools", aOutBools, STD_ARRAYLEN(aOutBools)));
    TEST_ASSERT_TRUE(aOutBools[0]);
    TEST_ASSERT_FALSE(aOutBools[1]);
    TEST_ASSERT_TRUE(aOutBools[2]);
    TEST_ASSERT_EQUAL_size_t(3u, stdJSON_GetStringArray(hJson, "arrays.strings", apOutStrings, STD_ARRAYLEN(apOutStrings), sizeof(aString0)));
    TEST_ASSERT_EQUAL_STRING("alpha", aString0);
    TEST_ASSERT_EQUAL_STRING("", aString1);
    TEST_ASSERT_EQUAL_STRING("gamma", aString2);

    TEST_ASSERT_TRUE(stdJSON_SetIntArrayElement(hJson, "sparse.ints", 3u, 99));
    TEST_ASSERT_EQUAL_size_t(4u, stdJSON_GetArraySize(hJson, "sparse.ints"));
    TEST_ASSERT_EQUAL_INT(-1, stdJSON_GetIntArrayElement(hJson, "sparse.ints", 0u, -1));
    TEST_ASSERT_EQUAL_INT(99, stdJSON_GetIntArrayElement(hJson, "sparse.ints", 3u, -1));
    TEST_ASSERT_TRUE(stdJSON_SetFloatArrayElement(hJson, "sparse.floats", 1u, 4.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.5f, stdJSON_GetFloatArrayElement(hJson, "sparse.floats", 1u, 0.0f));
    TEST_ASSERT_TRUE(stdJSON_SetBoolArrayElement(hJson, "sparse.bools", 2u, true));
    TEST_ASSERT_TRUE(stdJSON_GetBoolArrayElement(hJson, "sparse.bools", 2u, false));
    TEST_ASSERT_TRUE(stdJSON_SetStringArrayElement(hJson, "sparse.strings", 1u, "map"));
    TEST_ASSERT_TRUE(stdJSON_GetStringArrayElement(hJson, "sparse.strings", 1u, aString0, sizeof(aString0), NULL));
    TEST_ASSERT_EQUAL_STRING("map", aString0);

    TEST_ASSERT_TRUE(stdJSON_SetObjectArrayElement(hJson, "sparse.objects", 1u, hObject));
    hGotObject = stdJSON_GetObjectArrayElement(hJson, "sparse.objects", 1u);
    TEST_ASSERT_NOT_NULL(hGotObject);
    TEST_ASSERT_TRUE(stdJSON_GetString(hGotObject, "name", aString0, sizeof(aString0), NULL));
    TEST_ASSERT_EQUAL_STRING("artifact", aString0);
    stdJSON_Free(hGotObject);

    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_NULL, stdJSON_GetValueType(NULL, "x"));
    TEST_ASSERT_FALSE(stdJSON_SetIntArray(NULL, "x", aInts, 1u));
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_GetIntArray(hJson, "missing", aOutInts, STD_ARRAYLEN(aOutInts)));

    stdJSON_Free(hObject);
    stdJSON_Free(hJson);
}

TEST(stdJSON, TestArrayHandleOperations)
{
    StdJSONHandle hJson = stdJSON_Load(StdGeneralTest_GetTestVectorPath("stdJSON/document.json"));
    StdJSONHandle hArray;
    StdJSONHandle hSourceArray;
    StdJSONHandle hObject;
    StdJSONHandle hReplacement;
    StdJSONHandle hGotObject;
    StdJSONHandle hGotArray;
    char aText[16];

    TEST_ASSERT_NOT_NULL(hJson);
    hArray = stdJSON_GetArray(hJson, "array");
    hSourceArray = stdJSON_GetArray(hJson, "source");
    hObject = stdJSON_GetObject(hJson, "object");
    hReplacement = stdJSON_GetObject(hJson, "replacement");
    TEST_ASSERT_NOT_NULL(hArray);
    TEST_ASSERT_NOT_NULL(hSourceArray);
    TEST_ASSERT_NOT_NULL(hObject);
    TEST_ASSERT_NOT_NULL(hReplacement);

    TEST_ASSERT_EQUAL_size_t(2u, stdJSON_ArrayGetSize(hArray));
    TEST_ASSERT_TRUE(stdJSON_ArrayAppendInt(hArray, 3));
    TEST_ASSERT_TRUE(stdJSON_ArraySetInt(hArray, 0u, 10));
    TEST_ASSERT_EQUAL_INT(10, stdJSON_ArrayGetInt(hArray, 0u, 0));
    TEST_ASSERT_FALSE(stdJSON_ArraySetInt(hArray, 99u, 1));

    TEST_ASSERT_TRUE(stdJSON_ArrayAppendFloat(hArray, 4.5f));
    TEST_ASSERT_TRUE(stdJSON_ArraySetFloat(hArray, 3u, 5.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.5f, stdJSON_ArrayGetFloat(hArray, 3u, 0.0f));

    TEST_ASSERT_TRUE(stdJSON_ArrayAppendBool(hArray, true));
    TEST_ASSERT_TRUE(stdJSON_ArraySetBool(hArray, 4u, false));
    TEST_ASSERT_FALSE(stdJSON_ArrayGetBool(hArray, 4u, true));

    TEST_ASSERT_TRUE(stdJSON_ArrayAppendString(hArray, "raft"));
    TEST_ASSERT_TRUE(stdJSON_ArraySetString(hArray, 5u, "map"));
    TEST_ASSERT_TRUE(stdJSON_ArrayGetString(hArray, 5u, aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("map", aText);

    TEST_ASSERT_TRUE(stdJSON_ArrayAppendObject(hArray, hObject));
    hGotObject = stdJSON_ArrayGetObject(hArray, 6u);
    TEST_ASSERT_NOT_NULL(hGotObject);
    TEST_ASSERT_TRUE(stdJSON_GetString(hGotObject, "name", aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("indy", aText);
    stdJSON_Free(hGotObject);

    TEST_ASSERT_TRUE(stdJSON_ArraySetObject(hArray, 6u, hReplacement));
    hGotObject = stdJSON_ArrayGetObject(hArray, 6u);
    TEST_ASSERT_NOT_NULL(hGotObject);
    TEST_ASSERT_TRUE(stdJSON_GetString(hGotObject, "name", aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("sophia", aText);
    stdJSON_Free(hGotObject);

    TEST_ASSERT_TRUE(stdJSON_ArrayAppendArray(hArray, hSourceArray));
    TEST_ASSERT_TRUE(stdJSON_ArraySetArray(hArray, 7u, hSourceArray));
    hGotArray = stdJSON_ArrayGetArray(hArray, 7u);
    TEST_ASSERT_NOT_NULL(hGotArray);
    TEST_ASSERT_EQUAL_INT(7, stdJSON_ArrayGetInt(hGotArray, 0u, 0));
    stdJSON_Free(hGotArray);

    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_ARRAY, stdJSON_ArrayGetElementType(hArray, 7u));
    TEST_ASSERT_TRUE(stdJSON_ArrayRemoveElement(hArray, 1u));
    TEST_ASSERT_FALSE(stdJSON_ArrayRemoveElement(hArray, 99u));
    TEST_ASSERT_TRUE(stdJSON_ArrayClear(hArray));
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_ArrayGetSize(hArray));

    TEST_ASSERT_FALSE(stdJSON_ArrayAppendInt(hJson, 1));
    TEST_ASSERT_FALSE(stdJSON_ArrayClear(hJson));
    TEST_ASSERT_EQUAL_INT(-1, stdJSON_ArrayGetInt(NULL, 0u, -1));

    stdJSON_Free(hReplacement);
    stdJSON_Free(hObject);
    stdJSON_Free(hSourceArray);
    stdJSON_Free(hArray);
    stdJSON_Free(hJson);
}

TEST(stdJSON, TestStartupAndHandleFailureBranches)
{
    StdJSONHandle hJson;
    tHostServices* pSavedHostServices;

    stdJSON_Shutdown();
    TEST_ASSERT_FALSE(stdJSON_HasStarted());
    TEST_ASSERT_NULL(stdJSON_New());

    pSavedHostServices = std_g_pHS;
    std_g_pHS = NULL;
    TEST_ASSERT_FALSE(stdJSON_Startup());
    std_g_pHS = pSavedHostServices;
    TEST_ASSERT_TRUE(stdJSON_Startup());

    StdGeneralTest_FailNextAllocations(0);
    hJson = stdJSON_New();
    StdGeneralTest_ClearAllocationFailures();
    TEST_ASSERT_NULL(hJson);
}

TEST(stdJSON, TestLoadSaveAndRequiredKeyFailureBranches)
{
    const char* pDocumentPath = StdGeneralTest_GetTestVectorPath("stdJSON/document.json");
    const char* pSavePath = "stdJSONTest_failure_save.json";
    const char* apRequiredKeys[] = { "name", "stats.health" };
    StdJSONHandle hJson = stdJSON_New();
    StdJSONHandle hLoaded;

    TEST_ASSERT_NOT_NULL(hJson);
    stdJSONTest_DeleteFile(pSavePath);

    StdGeneralTest_FailNextFileReads(0);
    TEST_ASSERT_FALSE(stdJSON_LoadEntry(pDocumentPath, hJson));
    StdGeneralTest_ClearFileFailures();

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_FALSE(stdJSON_LoadEntry(pDocumentPath, hJson));
    StdGeneralTest_ClearAllocationFailures();

    hLoaded = stdJSON_Load(pDocumentPath);
    TEST_ASSERT_NOT_NULL(hLoaded);

    TEST_ASSERT_FALSE(stdJSON_Save(hJson, NULL));

    StdGeneralTest_FailNextFileOpens(0);
    TEST_ASSERT_FALSE(stdJSON_Save(hLoaded, pSavePath));
    StdGeneralTest_ClearFileFailures();

    StdGeneralTest_FailNextFileWrites(0);
    TEST_ASSERT_FALSE(stdJSON_Save(hLoaded, pSavePath));
    StdGeneralTest_ClearFileFailures();

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_FALSE(stdJSON_SetRequiredKeys(hLoaded, apRequiredKeys, STD_ARRAYLEN(apRequiredKeys)));
    StdGeneralTest_ClearAllocationFailures();

    StdGeneralTest_FailNextAllocations(1);
    TEST_ASSERT_FALSE(stdJSON_SetRequiredKeys(hLoaded, apRequiredKeys, STD_ARRAYLEN(apRequiredKeys)));
    StdGeneralTest_ClearAllocationFailures();

    TEST_ASSERT_FALSE(stdJSON_SetRequiredKeys(hLoaded, NULL, 1u));
    TEST_ASSERT_TRUE(stdJSON_SetRequiredKeys(hLoaded, apRequiredKeys, 0u));

    stdJSON_Free(hLoaded);
    stdJSON_Free(hJson);
    stdJSONTest_DeleteFile(pSavePath);
}

TEST(stdJSON, TestInvalidParametersAndEnumerationAllocationFailures)
{
    StdJSONHandle hJson = stdJSON_Load(StdGeneralTest_GetTestVectorPath("stdJSON/document.json"));
    char* apKeys[2] = { 0 };
    char aText[8];
    int aInts[2];
    float aFloats[2];
    bool aBools[2];
    char* apStrings[1] = { aText };

    TEST_ASSERT_NOT_NULL(hJson);

    TEST_ASSERT_FALSE(stdJSON_Delete(NULL, "name"));
    TEST_ASSERT_FALSE(stdJSON_Clear(NULL));
    TEST_ASSERT_FALSE(stdJSON_Merge(NULL, hJson, true));
    TEST_ASSERT_NULL(stdJSON_Duplicate(NULL));
    TEST_ASSERT_FALSE(stdJSON_IsValid(NULL));
    TEST_ASSERT_EQUAL_INT(STDJSON_TYPE_NULL, stdJSON_GetType(NULL));
    TEST_ASSERT_NULL(stdJSON_Load(NULL));
    TEST_ASSERT_FALSE(stdJSON_LoadEntry(NULL, hJson));
    TEST_ASSERT_NULL(stdJSON_LoadFromString(NULL));
    TEST_ASSERT_NULL(stdJSON_ToString(NULL, false));
    TEST_ASSERT_FALSE(stdJSON_SetBool(NULL, "x", true));
    TEST_ASSERT_FALSE(stdJSON_SetInt(hJson, NULL, 1));
    TEST_ASSERT_FALSE(stdJSON_SetFloat(NULL, "x", 1.0f));
    TEST_ASSERT_FALSE(stdJSON_SetString(hJson, "x", NULL));
    TEST_ASSERT_FALSE(stdJSON_GetString(hJson, "name", NULL, sizeof(aText), NULL));
    TEST_ASSERT_FALSE(stdJSON_GetString(hJson, "name", aText, 0u, NULL));
    TEST_ASSERT_NULL(stdJSON_GetObject(NULL, "object"));
    TEST_ASSERT_NULL(stdJSON_GetArray(hJson, NULL));

    TEST_ASSERT_FALSE(stdJSON_SetIntArray(hJson, "bad", NULL, 1u));
    TEST_ASSERT_FALSE(stdJSON_SetFloatArray(hJson, "bad", NULL, 1u));
    TEST_ASSERT_FALSE(stdJSON_SetBoolArray(hJson, "bad", NULL, 1u));
    TEST_ASSERT_FALSE(stdJSON_SetStringArray(hJson, "bad", NULL, 1u));
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_GetIntArray(NULL, "array", aInts, STD_ARRAYLEN(aInts)));
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_GetFloatArray(hJson, NULL, aFloats, STD_ARRAYLEN(aFloats)));
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_GetBoolArray(hJson, "array", NULL, STD_ARRAYLEN(aBools)));
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_GetStringArray(hJson, "array", apStrings, STD_ARRAYLEN(apStrings), 0u));
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_GetKeys(hJson, "group", apKeys, 0u));
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_EnumerateKeys(hJson, "group", NULL, NULL));

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_EQUAL_size_t(0u, stdJSON_GetKeys(hJson, "group", apKeys, STD_ARRAYLEN(apKeys)));
    StdGeneralTest_ClearAllocationFailures();

    stdJSON_Free(hJson);
}

TEST(stdJSON, TestGeneratedMalformedJsonCorpusRejects)
{
    static const char* const apMalformedJson[] =
    {
        "",
        " ",
        "null",
        "[]",
        "{",
        "{\"a\":",
        "{\"a\":1",
        "{\"a\":1,}",
        "{\"a\":[1,2,]}",
        "{\"a\":\"\\u12\"}",
        "{\"a\":\"\\x\"}",
        "{\"a\":01}",
        "{\"a\":1} trailing",
        "{\"a\": /*comment*/ 1}"
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(apMalformedJson); ++i )
    {
        TEST_ASSERT_NULL_MESSAGE(stdJSON_LoadFromString(apMalformedJson[i]), apMalformedJson[i]);
    }

    {
        const char* pValidJson = "{\"a\":1,\"b\":true,\"c\":\"text\",\"d\":[1,2,3]}";

        for ( size_t i = 0; pValidJson[i]; ++i )
        {
            if ( stdJSONTest_IsStructuralJsonChar(pValidJson[i]) )
            {
                char aMutated[96];

                stdUtil_StringCopy(aMutated, sizeof(aMutated), pValidJson);
                aMutated[i] = '#';
                TEST_ASSERT_NULL_MESSAGE(stdJSON_LoadFromString(aMutated), aMutated);
            }
        }
    }

    {
        StdJSONHandle hJson = stdJSON_New();

        TEST_ASSERT_NOT_NULL(hJson);
        TEST_ASSERT_TRUE(stdJSON_SetInt(hJson, "sentinel", 77));
        TEST_ASSERT_NULL(stdJSON_LoadEntryFromString("{\"broken\":", hJson));
        TEST_ASSERT_EQUAL_INT(77, stdJSON_GetInt(hJson, "sentinel", 0));
        stdJSON_Free(hJson);
    }
}

TEST(stdJSON, TestGeneratedRoundTripPreservesNestedValuesAndEscapes)
{
    StdJSONHandle hJson   = stdJSON_New();
    StdJSONHandle hLoaded = NULL;
    char* pJsonText       = NULL;

    TEST_ASSERT_NOT_NULL(hJson);

    for ( size_t i = 0; i < 24u; ++i )
    {
        char aKey[64];
        char aValue[64];

        STD_FORMAT(aKey, "generated.group%02u.index", (unsigned)i);
        TEST_ASSERT_TRUE(stdJSON_SetInt(hJson, aKey, (int)i * 3 - 17));

        STD_FORMAT(aKey, "generated.group%02u.scale", (unsigned)i);
        TEST_ASSERT_TRUE(stdJSON_SetFloat(hJson, aKey, (float)i + 0.25f));

        STD_FORMAT(aKey, "generated.group%02u.enabled", (unsigned)i);
        TEST_ASSERT_TRUE(stdJSON_SetBool(hJson, aKey, (i & 1u) == 0));

        STD_FORMAT(aKey, "generated.group%02u.label", (unsigned)i);
        STD_FORMAT(aValue, "name_%02u quote=\" backslash=\\ line=\n", (unsigned)i);
        TEST_ASSERT_TRUE(stdJSON_SetString(hJson, aKey, aValue));
    }

    {
        const int aInts[]       = { -3, -1, 0, 1, 3, 9 };
        const char* apStrings[] = { "alpha", "two words", "quote=\"", "slash=\\", "" };

        TEST_ASSERT_TRUE(stdJSON_SetIntArray(hJson, "generated.arrays.ints", aInts, STD_ARRAYLEN(aInts)));
        TEST_ASSERT_TRUE(stdJSON_SetStringArray(hJson, "generated.arrays.strings", apStrings, STD_ARRAYLEN(apStrings)));
    }

    pJsonText = stdJSON_ToString(hJson, false);
    TEST_ASSERT_NOT_NULL(pJsonText);

    hLoaded = stdJSON_LoadFromString(pJsonText);
    TEST_ASSERT_NOT_NULL(hLoaded);

    for ( size_t i = 0; i < 24u; ++i )
    {
        char aKey[64];
        char aExpected[64];
        char aActual[64];

        STD_FORMAT(aKey, "generated.group%02u.index", (unsigned)i);
        TEST_ASSERT_EQUAL_INT((int)i * 3 - 17, stdJSON_GetInt(hLoaded, aKey, 0));

        STD_FORMAT(aKey, "generated.group%02u.scale", (unsigned)i);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, (float)i + 0.25f, stdJSON_GetFloat(hLoaded, aKey, 0.0f));

        STD_FORMAT(aKey, "generated.group%02u.enabled", (unsigned)i);
        TEST_ASSERT_EQUAL_INT((i & 1u) == 0, stdJSON_GetBool(hLoaded, aKey, (i & 1u) != 0));

        STD_FORMAT(aKey, "generated.group%02u.label", (unsigned)i);
        STD_FORMAT(aExpected, "name_%02u quote=\" backslash=\\ line=\n", (unsigned)i);
        TEST_ASSERT_TRUE(stdJSON_GetString(hLoaded, aKey, aActual, sizeof(aActual), NULL));
        TEST_ASSERT_EQUAL_STRING(aExpected, aActual);
    }

    {
        int aOutInts[6];
        char aString0[16];
        char aString1[16];
        char aString2[16];
        char aString3[16];
        char aString4[16];
        char* apStrings[] = { aString0, aString1, aString2, aString3, aString4 };

        TEST_ASSERT_EQUAL_size_t(
            STD_ARRAYLEN(aOutInts),
            stdJSON_GetIntArray(hLoaded, "generated.arrays.ints", aOutInts, STD_ARRAYLEN(aOutInts))
        );
        TEST_ASSERT_EQUAL_INT(-3, aOutInts[0]);
        TEST_ASSERT_EQUAL_INT(9, aOutInts[5]);

        TEST_ASSERT_EQUAL_size_t(
            STD_ARRAYLEN(apStrings),
            stdJSON_GetStringArray(hLoaded, "generated.arrays.strings", apStrings, STD_ARRAYLEN(apStrings), sizeof(aString0))
        );
        TEST_ASSERT_EQUAL_STRING("alpha", aString0);
        TEST_ASSERT_EQUAL_STRING("two words", aString1);
        TEST_ASSERT_EQUAL_STRING("quote=\"", aString2);
        TEST_ASSERT_EQUAL_STRING("slash=\\", aString3);
        TEST_ASSERT_EQUAL_STRING("", aString4);
    }

    STDFREE(pJsonText);
    stdJSON_Free(hLoaded);
    stdJSON_Free(hJson);
}

TEST(stdJSON, TestRfc8259UnicodeEscapesAndExactSerialization)
{
    // Independently specified escapes: RFC 8259 sections 7 and 8.3.
    // https://www.rfc-editor.org/rfc/rfc8259.html#section-7
    const char* pDocument = "{\"slash\":\"\\u005C\",\"music\":\"\\uD834\\uDD1E\"}";
    StdJSONHandle hJson = stdJSON_LoadFromString(pDocument);
    TEST_ASSERT_NOT_NULL(hJson);

    char aText[16];
    TEST_ASSERT_TRUE(stdJSON_GetString(hJson, "slash", aText, sizeof(aText), NULL));
    // Check decoded UTF-8 bytes and the complete compact serialization, including escaping.
    TEST_ASSERT_EQUAL_STRING("\\", aText);
    TEST_ASSERT_TRUE(stdJSON_GetString(hJson, "music", aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("\xF0\x9D\x84\x9E", aText);

    char* pSerialized = stdJSON_ToString(hJson, false);
    TEST_ASSERT_NOT_NULL(pSerialized);
    TEST_ASSERT_EQUAL_STRING("{\"slash\": \"\\\\\", \"music\": \"\xF0\x9D\x84\x9E\"}", pSerialized);
    STDFREE(pSerialized);
    stdJSON_Free(hJson);

    // Unpaired or incorrectly ordered surrogate escapes are rejected by Jansson.
    static const char* apInvalid[] = {
        "{\"s\":\"\\uD834\"}", "{\"s\":\"\\uDD1E\"}",
        "{\"s\":\"\\uDD1E\\uD834\"}", "{\"s\":\"\\uD834x\"}"
    };
    size_t initialAllocs = stdMemory_g_curState.totalAllocs;
    size_t initialBytes  = stdMemory_g_curState.totalBytes;
    for ( size_t i = 0u; i < STD_ARRAYLEN(apInvalid); ++i )
    {
        TEST_ASSERT_NULL(stdJSON_LoadFromString(apInvalid[i]));
        TEST_ASSERT_EQUAL_size_t(initialAllocs, stdMemory_g_curState.totalAllocs);
        TEST_ASSERT_EQUAL_size_t(initialBytes, stdMemory_g_curState.totalBytes);
    }
}

TEST_GROUP_RUNNER(stdJSON)
{
    RUN_TEST_CASE(stdJSON, TestRfc8259UnicodeEscapesAndExactSerialization);
    RUN_TEST_CASE(stdJSON, TestCreateSetGetTypesSerializeDeleteAndClear);
    RUN_TEST_CASE(stdJSON, TestLoadSaveLoadEntryDuplicateMergeAndRequiredKeys);
    RUN_TEST_CASE(stdJSON, TestNestedObjectVectorsAndPathCollisions);
    RUN_TEST_CASE(stdJSON, TestLoadFromStringRejectsBrokenOrNonObjectJson);
    RUN_TEST_CASE(stdJSON, TestObjectsArraysParentsAndKeyEnumeration);
    RUN_TEST_CASE(stdJSON, TestFreeEntryAutoSaveAndExplicitParent);
    RUN_TEST_CASE(stdJSON, TestBulkArraysAndSparseElementAccessors);
    RUN_TEST_CASE(stdJSON, TestArrayHandleOperations);
    RUN_TEST_CASE(stdJSON, TestStartupAndHandleFailureBranches);
    RUN_TEST_CASE(stdJSON, TestLoadSaveAndRequiredKeyFailureBranches);
    RUN_TEST_CASE(stdJSON, TestInvalidParametersAndEnumerationAllocationFailures);
    RUN_TEST_CASE(stdJSON, TestGeneratedMalformedJsonCorpusRejects);
    RUN_TEST_CASE(stdJSON, TestGeneratedRoundTripPreservesNestedValuesAndEscapes);
}
