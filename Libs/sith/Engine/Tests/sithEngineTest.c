#include "sithEngineTest.h"

#include <unity_fixture.h>
#include <stdio.h>

#include <j3dcore/Tests/j3dTest.h>
#include <rdroid/RTI/addresses.h>
#include <sith/RTI/addresses.h>
#include <std/RTI/addresses.h>

tHostServices* sithEngineTest_pSithHostServices;
tHostServices* sithEngineTest_pStdHostServices;
tHostServices* sithEngineTest_pRdroidHostServices;

void SithEngineTest_Startup(void)
{
    J3DTest_Startup();
    J3DTest_BindHostServicesGlobal(sith_g_pHS_ADDR);
    J3DTest_BindHostServicesGlobal(std_g_pHS_ADDR);
    J3DTest_BindHostServicesGlobal(rdroid_g_pHS_ADDR);

    J3DTest_ResetAssertCapture();
}

void SithEngineTest_Shutdown(void)
{
    int assertCount = J3DTest_GetAssertCount();
    J3DTest_Shutdown();
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, assertCount, "Unexpected engine assertion");
}

void TEST_sithIntersect_GROUP_RUNNER(void);
void TEST_sithPathMove_GROUP_RUNNER(void);
void TEST_sithPhysics_GROUP_RUNNER(void);

static void SithEngineTest_RunAllTests(void)
{
    TEST_sithIntersect_GROUP_RUNNER();
    TEST_sithPathMove_GROUP_RUNNER();
    TEST_sithPhysics_GROUP_RUNNER();
}

int main(int argc, const char* argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    return UnityMain(argc, argv, SithEngineTest_RunAllTests);
}
