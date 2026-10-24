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

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, RdroidTest_RunAllTests);
}
