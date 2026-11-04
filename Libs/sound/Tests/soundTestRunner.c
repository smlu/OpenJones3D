#include <unity_fixture.h>

void TEST_AudioLib_GROUP_RUNNER(void);
void TEST_Sound_GROUP_RUNNER(void);

static void SoundTest_RunAllTests(void)
{
    TEST_AudioLib_GROUP_RUNNER();
    TEST_Sound_GROUP_RUNNER();
}

// Added: the entry name is overridable so the Babylon column's test main can call this runner
// after it has installed the platform context (J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main); plain
// builds keep main.
#ifndef J3D_TEST_RUNNER_ENTRY
#define J3D_TEST_RUNNER_ENTRY main
#endif

int J3D_TEST_RUNNER_ENTRY(int argc, const char* argv[])
{
    return UnityMain(argc, argv, SoundTest_RunAllTests);
}
