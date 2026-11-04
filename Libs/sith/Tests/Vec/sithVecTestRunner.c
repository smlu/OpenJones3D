#include <unity_fixture.h>

#include <stdio.h>

// Defined by the test source of the suite
void SithVecTest_RunAllTests(void);

// Added: the entry name is overridable so the Babylon column's test main can call this runner
// after it has installed the platform context (J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main); plain
// builds keep main.
#ifndef J3D_TEST_RUNNER_ENTRY
#define J3D_TEST_RUNNER_ENTRY main
#endif

int J3D_TEST_RUNNER_ENTRY(int argc, const char* argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    return UnityMain(argc, argv, SithVecTest_RunAllTests);
}
