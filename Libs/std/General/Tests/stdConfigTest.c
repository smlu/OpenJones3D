#include <unity_fixture.h>

#include <math.h>
#include <string.h>

#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdConfig.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdJSON.h>
#include <std/General/stdMemory.h>

#include "stdGeneralTest.h"

static const char* stdConfigTest_pPath = "stdConfigTest.json";

static void stdConfigTest_DeleteFile(void)
{
    StdGeneralTest_DeleteFile(stdConfigTest_pPath);
}

static void stdConfigTest_StartupConfig(void)
{
    TEST_ASSERT_TRUE(stdJSON_Startup());
    TEST_ASSERT_TRUE(stdConfig_Startup(stdConfigTest_pPath));
}

TEST_GROUP(stdConfig);

TEST_SETUP(stdConfig)
{
    StdGeneralTest_Startup();
    stdConfigTest_DeleteFile();
}

TEST_TEAR_DOWN(stdConfig)
{
    if ( stdConfig_HasStarted() )
    {
        stdConfig_Shutdown();
    }
    if ( stdJSON_HasStarted() )
    {
        stdJSON_Shutdown();
    }
    stdConfigTest_DeleteFile();
    StdGeneralTest_Shutdown();
}

TEST(stdConfig, TestStartupCreatesConfigFileAndVersion)
{
    char aVersion[16];

    stdConfigTest_StartupConfig();

    TEST_ASSERT_TRUE(stdConfig_HasStarted());
    TEST_ASSERT_TRUE(stdFileUtil_FileExists(stdConfigTest_pPath));
    TEST_ASSERT_TRUE(stdConfig_Contains("version"));
    TEST_ASSERT_TRUE(stdConfig_GetString("version", aVersion, sizeof(aVersion), NULL));
    TEST_ASSERT_EQUAL_STRING("1.0.0", aVersion);
}

TEST(stdConfig, TestStartupLoadsExistingVectorConfig)
{
    char aText[32];

    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile("stdConfig/existing_config.json", stdConfigTest_pPath));
    stdConfigTest_StartupConfig();

    TEST_ASSERT_TRUE(stdConfig_HasStarted());
    TEST_ASSERT_TRUE(stdConfig_GetBool("graphics.window", false));
    TEST_ASSERT_EQUAL_INT(1024, stdConfig_GetInt("graphics.width", 0));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.25f, stdConfig_GetFloat("engine.renderer.fog.density", 0.0f));
    TEST_ASSERT_TRUE(stdConfig_GetString("controls.configFile", aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("vector_controls.cfg", aText);
    TEST_ASSERT_EQUAL_UINT32(STD_RGBA(0x11, 0x22, 0x33, 0x44), stdConfig_GetColor("ui.tint", STD_RGBA(1, 2, 3, 4)));
}

TEST(stdConfig, TestStartupRejectsMalformedConfigAndCanRetry)
{
    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile("stdConfig/invalid_config.json", stdConfigTest_pPath));
    TEST_ASSERT_TRUE(stdJSON_Startup());

    size_t initialTotalBytes  = stdMemory_g_curState.totalBytes;
    size_t initialTotalAllocs = stdMemory_g_curState.totalAllocs;

    TEST_ASSERT_FALSE(stdConfig_Startup(stdConfigTest_pPath));
    TEST_ASSERT_FALSE(stdConfig_HasStarted());
    TEST_ASSERT_EQUAL_size_t(initialTotalBytes, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(initialTotalAllocs, stdMemory_g_curState.totalAllocs);

    stdConfigTest_DeleteFile();
    TEST_ASSERT_TRUE(stdConfig_Startup(stdConfigTest_pPath));
    TEST_ASSERT_TRUE(stdConfig_HasStarted());
    TEST_ASSERT_TRUE(stdConfig_Contains("version"));
}

TEST(stdConfig, TestSetGetPrimitivesAndContains)
{
    char aText[32];

    stdConfigTest_StartupConfig();

    TEST_ASSERT_TRUE(stdConfig_SetBool("graphics.window", true));
    TEST_ASSERT_TRUE(stdConfig_SetInt("graphics.width", 1280));
    TEST_ASSERT_TRUE(stdConfig_SetFloat("engine.renderer.fog.density", 0.45f));
    TEST_ASSERT_TRUE(stdConfig_SetString("controls.configFile", "keyboard.cfg"));

    TEST_ASSERT_TRUE(stdConfig_Contains("graphics.window"));
    TEST_ASSERT_TRUE(stdConfig_GetBool("graphics.window", false));
    TEST_ASSERT_EQUAL_INT(1280, stdConfig_GetInt("graphics.width", 0));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.45f, stdConfig_GetFloat("engine.renderer.fog.density", 0.0f));
    TEST_ASSERT_TRUE(stdConfig_GetString("controls.configFile", aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("keyboard.cfg", aText);

    TEST_ASSERT_FALSE(stdConfig_GetBool("missing.bool", false));
    TEST_ASSERT_EQUAL_INT(77, stdConfig_GetInt("missing.int", 77));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.25f, stdConfig_GetFloat("missing.float", 1.25f));
    TEST_ASSERT_TRUE(stdConfig_GetString("missing.string", aText, sizeof(aText), "default"));
    TEST_ASSERT_EQUAL_STRING("default", aText);
    TEST_ASSERT_FALSE(stdConfig_GetString("missing.emptyString", aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("", aText);
    TEST_ASSERT_FALSE(stdConfig_Contains("missing.key"));
}

TEST(stdConfig, TestRegistryFallbacksAreMappedAndCachedIntoJson)
{
    char aText[64];

    StdGeneralTest_SetRegistryInt("InWindow", 1);
    StdGeneralTest_SetRegistryFloat("Fog Density", 0.75f);
    StdGeneralTest_SetRegistryString("Install Path", "C:\\Games\\InfernalMachine");

    stdConfigTest_StartupConfig();

    TEST_ASSERT_TRUE(stdConfig_GetBool("graphics.window", false));
    StdGeneralTest_SetRegistryInt("Sound Volume", 55);
    TEST_ASSERT_EQUAL_INT(55, stdConfig_GetInt("sound.volume", 10));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.75f, stdConfig_GetFloat("engine.renderer.fog.density", 0.1f));
    TEST_ASSERT_TRUE(stdConfig_GetString("installPath", aText, sizeof(aText), "missing"));
    TEST_ASSERT_EQUAL_STRING("C:\\Games\\InfernalMachine", aText);

    StdGeneralTest_ClearRegistryValues();
    TEST_ASSERT_TRUE(stdConfig_GetBool("graphics.window", false));
    TEST_ASSERT_EQUAL_INT(55, stdConfig_GetInt("sound.volume", 10));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.75f, stdConfig_GetFloat("engine.renderer.fog.density", 0.1f));
    TEST_ASSERT_TRUE(stdConfig_GetString("installPath", aText, sizeof(aText), "missing"));
    TEST_ASSERT_EQUAL_STRING("C:\\Games\\InfernalMachine", aText);
}

TEST(stdConfig, TestColorRoundTripAndAcceptedFormats)
{
    tStdColor color;

    stdConfigTest_StartupConfig();

    TEST_ASSERT_TRUE(stdConfig_SetColor("ui.tint", STD_RGBA(0x11, 0x22, 0x33, 0x44)));
    color = stdConfig_GetColor("ui.tint", STD_RGBA(1, 2, 3, 4));
    TEST_ASSERT_EQUAL_UINT32(STD_RGBA(0x11, 0x22, 0x33, 0x44), color);

    TEST_ASSERT_TRUE(stdConfig_SetColorRGB("ui.rgb", STD_RGBA(0xAA, 0xBB, 0xCC, 0x12)));
    color = stdConfig_GetColor("ui.rgb", STD_RGBA(1, 2, 3, 4));
    TEST_ASSERT_EQUAL_UINT32(STD_RGBA(0xAA, 0xBB, 0xCC, 0xFF), color);

    TEST_ASSERT_TRUE(stdConfig_SetString("ui.hexPrefix", "0x01020304"));
    color = stdConfig_GetColor("ui.hexPrefix", STD_RGBA(9, 9, 9, 9));
    TEST_ASSERT_EQUAL_UINT32(STD_RGBA(1, 2, 3, 4), color);

    TEST_ASSERT_TRUE(stdConfig_SetString("ui.upperHexPrefix", "0X10203040"));
    color = stdConfig_GetColor("ui.upperHexPrefix", STD_RGBA(9, 9, 9, 9));
    TEST_ASSERT_EQUAL_UINT32(STD_RGBA(0x10, 0x20, 0x30, 0x40), color);

    TEST_ASSERT_TRUE(stdConfig_SetString("ui.badColor", "bad"));
    color = stdConfig_GetColor("ui.badColor", STD_RGBA(9, 8, 7, 6));
    TEST_ASSERT_EQUAL_UINT32(STD_RGBA(9, 8, 7, 6), color);

    TEST_ASSERT_TRUE(stdConfig_SetString("ui.badRgb", "GG2233"));
    color = stdConfig_GetColor("ui.badRgb", STD_RGBA(8, 7, 6, 5));
    TEST_ASSERT_EQUAL_UINT32(STD_RGBA(8, 7, 6, 5), color);

    TEST_ASSERT_TRUE(stdConfig_SetString("ui.badRgba", "112233GG"));
    color = stdConfig_GetColor("ui.badRgba", STD_RGBA(7, 6, 5, 4));
    TEST_ASSERT_EQUAL_UINT32(STD_RGBA(7, 6, 5, 4), color);

    TEST_ASSERT_FALSE(stdConfig_SetColor(NULL, STD_RGBA(1, 2, 3, 4)));
    TEST_ASSERT_FALSE(stdConfig_SetColorRGB(NULL, STD_RGBA(1, 2, 3, 4)));
    TEST_ASSERT_EQUAL_UINT32(STD_RGBA(6, 5, 4, 3), stdConfig_GetColor(NULL, STD_RGBA(6, 5, 4, 3)));
}

TEST(stdConfig, TestValuesPersistAcrossRestart)
{
    char aText[32];

    stdConfigTest_StartupConfig();
    TEST_ASSERT_TRUE(stdConfig_SetBool("graphics.window", true));
    TEST_ASSERT_TRUE(stdConfig_SetInt("graphics.width", 1600));
    TEST_ASSERT_TRUE(stdConfig_SetFloat("engine.renderer.fog.density", 0.625f));
    TEST_ASSERT_TRUE(stdConfig_SetString("controls.configFile", "persistent.cfg"));
    TEST_ASSERT_TRUE(stdConfig_SetColor("ui.tint", STD_RGBA(0x12, 0x34, 0x56, 0x78)));

    stdConfig_Shutdown();
    TEST_ASSERT_FALSE(stdConfig_HasStarted());
    TEST_ASSERT_TRUE(stdConfig_Startup(stdConfigTest_pPath));

    TEST_ASSERT_TRUE(stdConfig_GetBool("graphics.window", false));
    TEST_ASSERT_EQUAL_INT(1600, stdConfig_GetInt("graphics.width", 0));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.625f, stdConfig_GetFloat("engine.renderer.fog.density", 0.0f));
    TEST_ASSERT_TRUE(stdConfig_GetString("controls.configFile", aText, sizeof(aText), NULL));
    TEST_ASSERT_EQUAL_STRING("persistent.cfg", aText);
    TEST_ASSERT_EQUAL_UINT32(STD_RGBA(0x12, 0x34, 0x56, 0x78), stdConfig_GetColor("ui.tint", 0u));
}

TEST(stdConfig, TestStartupRejectsInvalidPathOrMissingDependencies)
{
    TEST_ASSERT_FALSE(stdConfig_Startup(NULL));
    TEST_ASSERT_FALSE(stdConfig_Startup(stdConfigTest_pPath));

    TEST_ASSERT_TRUE(stdJSON_Startup());
    TEST_ASSERT_TRUE(stdConfig_Startup(stdConfigTest_pPath));
    TEST_ASSERT_TRUE(stdConfig_Startup(stdConfigTest_pPath));
}

TEST(stdConfig, TestStartupCleansUpAfterAllocationFailure)
{
    TEST_ASSERT_TRUE(stdJSON_Startup());

    size_t initialTotalBytes  = stdMemory_g_curState.totalBytes;
    size_t initialTotalAllocs = stdMemory_g_curState.totalAllocs;

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_FALSE(stdConfig_Startup(stdConfigTest_pPath));
    StdGeneralTest_ClearAllocationFailures();

    TEST_ASSERT_FALSE(stdConfig_HasStarted());
    TEST_ASSERT_EQUAL_size_t(initialTotalBytes, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(initialTotalAllocs, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_TRUE(stdConfig_Startup(stdConfigTest_pPath));
}

TEST(stdConfig, TestStartupCleansUpAfterFileOpenFailure)
{
    TEST_ASSERT_TRUE(stdJSON_Startup());

    size_t initialTotalBytes  = stdMemory_g_curState.totalBytes;
    size_t initialTotalAllocs = stdMemory_g_curState.totalAllocs;

    StdGeneralTest_FailNextFileOpens(0);
    TEST_ASSERT_FALSE(stdConfig_Startup(stdConfigTest_pPath));
    StdGeneralTest_ClearFileFailures();

    TEST_ASSERT_FALSE(stdConfig_HasStarted());
    TEST_ASSERT_EQUAL_size_t(initialTotalBytes, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(initialTotalAllocs, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_TRUE(stdConfig_Startup(stdConfigTest_pPath));
}

TEST(stdConfig, TestStartupCleansUpAfterFileWriteFailure)
{
    TEST_ASSERT_TRUE(stdJSON_Startup());

    size_t initialTotalBytes  = stdMemory_g_curState.totalBytes;
    size_t initialTotalAllocs = stdMemory_g_curState.totalAllocs;

    StdGeneralTest_FailNextFileWrites(0);
    TEST_ASSERT_FALSE(stdConfig_Startup(stdConfigTest_pPath));
    StdGeneralTest_ClearFileFailures();

    TEST_ASSERT_FALSE(stdConfig_HasStarted());
    TEST_ASSERT_EQUAL_size_t(initialTotalBytes, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(initialTotalAllocs, stdMemory_g_curState.totalAllocs);

    stdConfigTest_DeleteFile();
    TEST_ASSERT_TRUE(stdConfig_Startup(stdConfigTest_pPath));
}

TEST_GROUP_RUNNER(stdConfig)
{
    RUN_TEST_CASE(stdConfig, TestStartupCreatesConfigFileAndVersion);
    RUN_TEST_CASE(stdConfig, TestStartupLoadsExistingVectorConfig);
    RUN_TEST_CASE(stdConfig, TestStartupRejectsMalformedConfigAndCanRetry);
    RUN_TEST_CASE(stdConfig, TestSetGetPrimitivesAndContains);
    RUN_TEST_CASE(stdConfig, TestRegistryFallbacksAreMappedAndCachedIntoJson);
    RUN_TEST_CASE(stdConfig, TestColorRoundTripAndAcceptedFormats);
    RUN_TEST_CASE(stdConfig, TestValuesPersistAcrossRestart);
    RUN_TEST_CASE(stdConfig, TestStartupRejectsInvalidPathOrMissingDependencies);
    RUN_TEST_CASE(stdConfig, TestStartupCleansUpAfterAllocationFailure);
    RUN_TEST_CASE(stdConfig, TestStartupCleansUpAfterFileOpenFailure);
    RUN_TEST_CASE(stdConfig, TestStartupCleansUpAfterFileWriteFailure);
}
