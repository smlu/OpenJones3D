#include <unity_fixture.h>

#include <stdio.h>

void TEST_sithDSS_GROUP_RUNNER(void);
void TEST_sithDSSThing_GROUP_RUNNER(void);

static void SithDSSTest_RunAllTests(void)
{
    TEST_sithDSS_GROUP_RUNNER();
    TEST_sithDSSThing_GROUP_RUNNER();
}

// Added: the entry name is overridable so the Babylon column's test main can call this runner
// after it has installed the platform context (J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main); plain
// builds keep main.
#ifndef J3D_TEST_RUNNER_ENTRY
#define J3D_TEST_RUNNER_ENTRY main
#endif

int J3D_TEST_RUNNER_ENTRY(int argc, const char* argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    return UnityMain(argc, argv, SithDSSTest_RunAllTests);
}
