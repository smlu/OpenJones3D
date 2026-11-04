// Added: the Babylon column's wkernel test runner. Same entry pattern as the other module
// runners: the Babylon column's test main renames the entry and calls it after it installed the
// adapter context.
#include <unity_fixture.h>

#include <j3dcore/j3d.h>

void TEST_wkernelBabylon_GROUP_RUNNER(void);

static void WkernelTest_RunAllTests(void)
{
    TEST_wkernelBabylon_GROUP_RUNNER();
}

// The entry name is overridable so the Babylon column's test main can call this runner after it
// has installed the platform context (J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main); plain builds
// keep main.
#ifndef J3D_TEST_RUNNER_ENTRY
#define J3D_TEST_RUNNER_ENTRY main
#endif

int J3D_TEST_RUNNER_ENTRY(int argc, const char* argv[])
{
    return UnityMain(argc, argv, WkernelTest_RunAllTests);
}
