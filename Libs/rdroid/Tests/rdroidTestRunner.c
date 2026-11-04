#include <unity_fixture.h>
#include <rdroid/Math/Tests/rdMathTestHost.h>

tHostServices* rdMathTest_pHS;

void TEST_rdVector_GROUP_RUNNER(void);
void TEST_rdMatrix_GROUP_RUNNER(void);

static void RdroidTest_RunAllTests(void)
{
    TEST_rdVector_GROUP_RUNNER();
    TEST_rdMatrix_GROUP_RUNNER();
}

// Added: the entry name is overridable so the Babylon column's test main can call this runner
// after it has installed the platform context (J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main); plain
// builds keep main.
#ifndef J3D_TEST_RUNNER_ENTRY
#define J3D_TEST_RUNNER_ENTRY main
#endif

int J3D_TEST_RUNNER_ENTRY(int argc, const char* argv[])
{
    return UnityMain(argc, argv, RdroidTest_RunAllTests);
}
