#include "sithEngineTest.h"

#include <unity_fixture.h>
#include <stdio.h>

#include <j3dcore/Tests/j3dTest.h>
#include <rdroid/types.h>
#include <rdroid/RTI/addresses.h>
#include <sith/RTI/addresses.h>
#include <std/RTI/addresses.h>

tHostServices* sithEngineTest_pSithHostServices;
tHostServices* sithEngineTest_pStdHostServices;
tHostServices* sithEngineTest_pRdroidHostServices;

// Identity matrix redirected in sithEngineTestAddrs.h
const rdMatrix34 sithEngineTest_identMatrix34 = {
    .rvec = { .x = 1.0f, .y = 0.0f, .z = 0.0f },
    .lvec = { .x = 0.0f, .y = 1.0f, .z = 0.0f },
    .uvec = { .x = 0.0f, .y = 0.0f, .z = 1.0f },
    .dvec = { .x = 0.0f, .y = 0.0f, .z = 0.0f },
};

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
