#include <unity_fixture.h>

#include <j3dcore/j3d.h>

#if !defined(J3D_PLATFORM_NULL)
// Added: The smush Sound area mocks IDirectSound/IDirectSoundBuffer and drives the DirectSound
// driver (insane/Sound/Dsnd.c), so it is not built on the Null platform (see Tests/CMakeLists.txt).
void TEST_SmushSound_GROUP_RUNNER(void);
#endif
void TEST_SmushCodec_GROUP_RUNNER(void);
void TEST_SmushStream_GROUP_RUNNER(void);
void TEST_SmushCore_GROUP_RUNNER(void);
void TEST_SmushAsm_GROUP_RUNNER(void);
void TEST_SmushVec_GROUP_RUNNER(void);

static void SmushTest_RunAllTests(void)
{
#if !defined(J3D_PLATFORM_NULL)
    TEST_SmushSound_GROUP_RUNNER();
#endif
    TEST_SmushCodec_GROUP_RUNNER();
    TEST_SmushStream_GROUP_RUNNER();
    TEST_SmushCore_GROUP_RUNNER();
    TEST_SmushAsm_GROUP_RUNNER();
    TEST_SmushVec_GROUP_RUNNER();
}

// Added: the entry name is overridable so the Babylon column's test main can call this runner
// after it has installed the platform context (J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main); plain
// builds keep main.
#ifndef J3D_TEST_RUNNER_ENTRY
#define J3D_TEST_RUNNER_ENTRY main
#endif

int J3D_TEST_RUNNER_ENTRY(int argc, const char* argv[])
{
    return UnityMain(argc, argv, SmushTest_RunAllTests);
}
