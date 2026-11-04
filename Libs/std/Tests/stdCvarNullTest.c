// Added: the Null column's simulation cvar seam (ADR-0020), stdCvarNull.c: the fixed table the
// headless state file starts with. The Babylon column covers its replacement, stdCvarBabylon.cpp,
// in stdBabylonTests.
#include <unity_fixture.h>

#include <string.h>

#include <j3dcore/j3d.h>

#include <std/Null/stdCvarNull.h>

#include "stdGeneralTest.h"

// The profile a default build plays, from the same switch the table reads, spelled out here so the
// test states its expectation.
#ifdef J3D_QOL_IMPROVEMENTS
#define STDCVARTEST_PROFILE "enhanced"
#else
#define STDCVARTEST_PROFILE "retail"
#endif

TEST_GROUP(stdCvarNull);

TEST_SETUP(stdCvarNull)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdCvarNull)
{
    StdGeneralTest_Shutdown();
}

TEST(stdCvarNull, ReportsTheBuildProfile)
{
    TEST_ASSERT_EQUAL_INT(1, (int)stdCvar_GetNumSimulation());

    char aName[64];
    char aValue[1025];
    TEST_ASSERT_TRUE(stdCvar_GetSimulation(0, aName, sizeof(aName), aValue, sizeof(aValue)));
    TEST_ASSERT_EQUAL_STRING("game.profile", aName);
    TEST_ASSERT_EQUAL_STRING(STDCVARTEST_PROFILE, aValue);
}

TEST(stdCvarNull, OutOfRangeIndexWritesNothing)
{
    char aName[64]   = "sentinel name";
    char aValue[1025] = "sentinel value";
    TEST_ASSERT_FALSE(stdCvar_GetSimulation(stdCvar_GetNumSimulation(), aName, sizeof(aName),
        aValue, sizeof(aValue)));
    TEST_ASSERT_FALSE(stdCvar_GetSimulation((size_t)-1, aName, sizeof(aName), aValue, sizeof(aValue)));
    TEST_ASSERT_EQUAL_STRING("sentinel name", aName);
    TEST_ASSERT_EQUAL_STRING("sentinel value", aValue);
}

TEST(stdCvarNull, NullBuffersWriteNothing)
{
    char aName[64]   = "sentinel name";
    char aValue[1025] = "sentinel value";
    TEST_ASSERT_FALSE(stdCvar_GetSimulation(0, NULL, sizeof(aName), aValue, sizeof(aValue)));
    TEST_ASSERT_FALSE(stdCvar_GetSimulation(0, aName, sizeof(aName), NULL, sizeof(aValue)));
    TEST_ASSERT_EQUAL_STRING("sentinel name", aName);
    TEST_ASSERT_EQUAL_STRING("sentinel value", aValue);
}

TEST(stdCvarNull, TooSmallBuffersWriteNothing)
{
    // One byte short of the NUL, for the name and for the value, each alone and together.
    char aName[64]   = "sentinel name";
    char aValue[1025] = "sentinel value";
    TEST_ASSERT_FALSE(stdCvar_GetSimulation(0, aName, strlen("game.profile"), aValue, sizeof(aValue)));
    TEST_ASSERT_EQUAL_STRING("sentinel name", aName);
    TEST_ASSERT_EQUAL_STRING("sentinel value", aValue);

    TEST_ASSERT_FALSE(stdCvar_GetSimulation(0, aName, sizeof(aName), aValue,
        strlen(STDCVARTEST_PROFILE)));
    TEST_ASSERT_EQUAL_STRING("sentinel name", aName);
    TEST_ASSERT_EQUAL_STRING("sentinel value", aValue);

    TEST_ASSERT_FALSE(stdCvar_GetSimulation(0, aName, 1, aValue, 1));
    TEST_ASSERT_EQUAL_STRING("sentinel name", aName);
    TEST_ASSERT_EQUAL_STRING("sentinel value", aValue);

    // The exact sizes do fit: 12 name bytes plus the NUL, and 24 for any profile text.
    char aTightName[13];
    char aTightValue[24];
    TEST_ASSERT_TRUE(stdCvar_GetSimulation(0, aTightName, sizeof(aTightName), aTightValue,
        sizeof(aTightValue)));
    TEST_ASSERT_EQUAL_STRING("game.profile", aTightName);
    TEST_ASSERT_EQUAL_STRING(STDCVARTEST_PROFILE, aTightValue);
}

TEST_GROUP_RUNNER(stdCvarNull)
{
    RUN_TEST_CASE(stdCvarNull, ReportsTheBuildProfile);
    RUN_TEST_CASE(stdCvarNull, OutOfRangeIndexWritesNothing);
    RUN_TEST_CASE(stdCvarNull, NullBuffersWriteNothing);
    RUN_TEST_CASE(stdCvarNull, TooSmallBuffersWriteNothing);
}
