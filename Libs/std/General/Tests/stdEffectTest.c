#include <unity_fixture.h>

#include <std/General/stdEffect.h>

#include "stdGeneralTest.h"

TEST_GROUP(stdEffect);

TEST_SETUP(stdEffect)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdEffect)
{
    StdGeneralTest_Shutdown();
}

TEST(stdEffect, TestStartupResetAndShutdown)
{
    const tStdFadeFactor* pFade;

    stdEffect_Startup();
    pFade = stdEffect_GetFadeFactor();
    TEST_ASSERT_NOT_NULL(pFade);
    TEST_ASSERT_EQUAL_INT(0, pFade->bEnabled);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 1.0f, pFade->factor);

    stdEffect_SetFadeFactor(1, 0.25f);
    TEST_ASSERT_EQUAL_INT(1, pFade->bEnabled);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.25f, pFade->factor);

    stdEffect_Reset();
    TEST_ASSERT_EQUAL_INT(0, pFade->bEnabled);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 1.0f, pFade->factor);

    stdEffect_SetFadeFactor(1, 0.5f);
    stdEffect_Shutdown();
    TEST_ASSERT_EQUAL_INT(0, pFade->bEnabled);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 1.0f, pFade->factor);
}

TEST(stdEffect, TestSetPreservesInputValues)
{
    const tStdFadeFactor* pFade;

    stdEffect_SetFadeFactor(-7, -2.0f);
    pFade = stdEffect_GetFadeFactor();

    TEST_ASSERT_EQUAL_INT(-7, pFade->bEnabled);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, -2.0f, pFade->factor);
}

TEST_GROUP_RUNNER(stdEffect)
{
    RUN_TEST_CASE(stdEffect, TestStartupResetAndShutdown);
    RUN_TEST_CASE(stdEffect, TestSetPreservesInputValues);
}
