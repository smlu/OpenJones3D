// Added: Unity Fixture suite for the Babylon column's simulation cvar seam: stdCvarBabylon.cpp,
// read through the StdBabylonContext's babylon::cvars::Registry (ADR-0020). The suite owns the
// registry and installs it in a fresh context, so the seam is tested exactly the way the
// composition roots wire it.
#include <unity_fixture.h>

#include <cstddef>
#include <cstring>
#include <string>

#include <babylon/cvars/Registry.h>
#include <babylon/platform/NullPlatform.h>
#include <babylon/time/NullClock.h>

#include <j3dcore/j3d.h>

#include <std/Babylon/stdBabylon.h>
#include <std/Babylon/stdBabylonCvars.h>
#include <std/Null/stdCvarNull.h>

TEST_GROUP(stdCvarBabylon);

namespace
{
    // The profile a default build plays, read from the same build switch the adapter uses and
    // written out here, so the tests name their expectation instead of calling production code.
#ifdef J3D_QOL_IMPROVEMENTS
    constexpr const char* kExpectedProfile = "enhanced";
#else
    constexpr const char* kExpectedProfile = "retail";
#endif

    constexpr size_t kNameSize = 64;
    constexpr size_t kValueSize = 1025;

    babylon::time::NullClock*        pNullClock     = nullptr;
    babylon::time::NullClock*        pVirtualClock  = nullptr;
    babylon::platform::NullPlatform* pPlatform      = nullptr;
    babylon::cvars::Registry         cvars;
    StdBabylonContext                context;

    // Registers one string cvar of this suite into the context's registry.
    void AddSuiteCvar(const char* pName, const char* pValue, babylon::cvars::Flag flags)
    {
        babylon::cvars::Desc desc;
        desc.name         = pName;
        desc.description  = "suite cvar";
        desc.defaultValue = std::string{ pValue };
        desc.flags        = flags;
        TEST_ASSERT_TRUE(cvars.add(desc));
    }

    // The simulation cvar at `index`, asserting the seam took it; the buffers are exactly big
    // enough for anything the registry can hand out here.
    void ReadSimulation(size_t index, char* pName, size_t nameSize, char* pValue, size_t valueSize)
    {
        TEST_ASSERT_TRUE(stdCvar_GetSimulation(index, pName, nameSize, pValue, valueSize));
    }
}

TEST_SETUP(stdCvarBabylon)
{
    cvars = babylon::cvars::Registry();

    pNullClock    = new babylon::time::NullClock();
    pVirtualClock = new babylon::time::NullClock();
    pPlatform     = new babylon::platform::NullPlatform();

    context.pClock          = pNullClock;
    context.pVirtualClock   = pVirtualClock;
    context.pFileSystem     = nullptr;
    context.pPlatform       = pPlatform;
    context.pScriptPlatform = pPlatform;
    context.pWindow         = nullptr;
    context.pCvars          = &cvars;
    stdBabylon_SetContext(&context);
}

TEST_TEAR_DOWN(stdCvarBabylon)
{
    stdBabylon_SetContext(nullptr);
    delete pPlatform;
    delete pVirtualClock;
    delete pNullClock;
    pPlatform     = nullptr;
    pVirtualClock = nullptr;
    pNullClock    = nullptr;
}

TEST(stdCvarBabylon, RegisterGameListsTheProfileCvar)
{
    TEST_ASSERT_TRUE(stdBabylonCvars_RegisterGame(cvars));
    cvars.finishStartup();

    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(stdCvar_GetNumSimulation()));

    char aName[kNameSize];
    char aValue[kValueSize];
    ReadSimulation(0, aName, sizeof(aName), aValue, sizeof(aValue));
    TEST_ASSERT_EQUAL_STRING("game.profile", aName);
    TEST_ASSERT_EQUAL_STRING(kExpectedProfile, aValue);
}

TEST(stdCvarBabylon, OutOfRangeIndexWritesNothing)
{
    TEST_ASSERT_TRUE(stdBabylonCvars_RegisterGame(cvars));
    cvars.finishStartup();

    char aName[kNameSize]  = "sentinel name";
    char aValue[kValueSize] = "sentinel value";
    TEST_ASSERT_FALSE(stdCvar_GetSimulation(1, aName, sizeof(aName), aValue, sizeof(aValue)));
    TEST_ASSERT_FALSE(stdCvar_GetSimulation(static_cast<size_t>(-1), aName, sizeof(aName),
        aValue, sizeof(aValue)));
    TEST_ASSERT_EQUAL_STRING("sentinel name", aName);
    TEST_ASSERT_EQUAL_STRING("sentinel value", aValue);
}

TEST(stdCvarBabylon, NullBuffersWriteNothing)
{
    TEST_ASSERT_TRUE(stdBabylonCvars_RegisterGame(cvars));
    cvars.finishStartup();

    char aName[kNameSize]   = "sentinel name";
    char aValue[kValueSize] = "sentinel value";
    TEST_ASSERT_FALSE(stdCvar_GetSimulation(0, nullptr, sizeof(aName), aValue, sizeof(aValue)));
    TEST_ASSERT_FALSE(stdCvar_GetSimulation(0, aName, sizeof(aName), nullptr, sizeof(aValue)));
    TEST_ASSERT_EQUAL_STRING("sentinel name", aName);
    TEST_ASSERT_EQUAL_STRING("sentinel value", aValue);
}

TEST(stdCvarBabylon, TooSmallBuffersWriteNothing)
{
    TEST_ASSERT_TRUE(stdBabylonCvars_RegisterGame(cvars));
    cvars.finishStartup();

    // One byte short of the NUL, for the name and for the value, each alone and together.
    char aName[kNameSize]   = "sentinel name";
    char aValue[kValueSize] = "sentinel value";
    TEST_ASSERT_FALSE(stdCvar_GetSimulation(0, aName, strlen("game.profile"), aValue, kValueSize));
    TEST_ASSERT_EQUAL_STRING("sentinel name", aName);
    TEST_ASSERT_EQUAL_STRING("sentinel value", aValue);

    TEST_ASSERT_FALSE(stdCvar_GetSimulation(0, aName, kNameSize, aValue, strlen(kExpectedProfile)));
    TEST_ASSERT_EQUAL_STRING("sentinel name", aName);
    TEST_ASSERT_EQUAL_STRING("sentinel value", aValue);

    TEST_ASSERT_FALSE(stdCvar_GetSimulation(0, aName, 1, aValue, 1));
    TEST_ASSERT_EQUAL_STRING("sentinel name", aName);
    TEST_ASSERT_EQUAL_STRING("sentinel value", aValue);

    // The exact sizes do fit: 12 name bytes plus the NUL, and the profile text plus the NUL.
    char aTightName[13];
    char aTightValue[24];
    ReadSimulation(0, aTightName, sizeof(aTightName), aTightValue, sizeof(aTightValue));
    TEST_ASSERT_EQUAL_STRING("game.profile", aTightName);
    TEST_ASSERT_EQUAL_STRING(kExpectedProfile, aTightValue);
}

TEST(stdCvarBabylon, SimulationCvarsAreSortedByName)
{
    // Registered before game.profile on purpose, so the report order can only come from the names:
    // "alpha.sim" sorts ahead of "game.profile" in byte order.
    AddSuiteCvar("alpha.sim", "on", babylon::cvars::Flag::Simulation);
    TEST_ASSERT_TRUE(stdBabylonCvars_RegisterGame(cvars));
    cvars.finishStartup();

    TEST_ASSERT_EQUAL_INT(2, static_cast<int>(stdCvar_GetNumSimulation()));

    char aFirstName[kNameSize];
    char aFirstValue[kValueSize];
    ReadSimulation(0, aFirstName, sizeof(aFirstName), aFirstValue, sizeof(aFirstValue));
    TEST_ASSERT_EQUAL_STRING("alpha.sim", aFirstName);
    TEST_ASSERT_EQUAL_STRING("on", aFirstValue);

    char aSecondName[kNameSize];
    char aSecondValue[kValueSize];
    ReadSimulation(1, aSecondName, sizeof(aSecondName), aSecondValue, sizeof(aSecondValue));
    TEST_ASSERT_EQUAL_STRING("game.profile", aSecondName);
    TEST_ASSERT_EQUAL_STRING(kExpectedProfile, aSecondValue);
}

TEST(stdCvarBabylon, NonSimulationCvarsAreNotListed)
{
    AddSuiteCvar("presentation.mode", "vsync", babylon::cvars::Flag::Saved);
    AddSuiteCvar("presentation.gamma", "1.0", babylon::cvars::Flag::None);
    TEST_ASSERT_TRUE(stdBabylonCvars_RegisterGame(cvars));
    cvars.finishStartup();

    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(stdCvar_GetNumSimulation()));

    char aName[kNameSize];
    char aValue[kValueSize];
    ReadSimulation(0, aName, sizeof(aName), aValue, sizeof(aValue));
    TEST_ASSERT_EQUAL_STRING("game.profile", aName);

    TEST_ASSERT_FALSE(stdCvar_GetSimulation(1, aName, sizeof(aName), aValue, sizeof(aValue)));
}

TEST_GROUP_RUNNER(stdCvarBabylon)
{
    RUN_TEST_CASE(stdCvarBabylon, RegisterGameListsTheProfileCvar);
    RUN_TEST_CASE(stdCvarBabylon, OutOfRangeIndexWritesNothing);
    RUN_TEST_CASE(stdCvarBabylon, NullBuffersWriteNothing);
    RUN_TEST_CASE(stdCvarBabylon, TooSmallBuffersWriteNothing);
    RUN_TEST_CASE(stdCvarBabylon, SimulationCvarsAreSortedByName);
    RUN_TEST_CASE(stdCvarBabylon, NonSimulationCvarsAreNotListed);
}
