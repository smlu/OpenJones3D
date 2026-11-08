#ifndef SITH_AI_TESTS_SITHAIFIXTEST_H
#define SITH_AI_TESTS_SITHAIFIXTEST_H

// Fixed-behavior tests reuse the deterministic world and engine seams, but make
// explicit assertions instead of comparing against undefined original outputs.
static void sithAIFixTest_Prepare(const SithVecSuite* pSuite, const SithVecCase* pCase)
{
    sithVec_fill        = 0xA5;
    sithVec_rng         = 12345;
    sithVec_numAsserts  = 0;
    sithVec_numErrors   = 0;
    sithVec_numWarnings = 0;
    sithVec_numMallocs  = 0;
    sithVec_ev.size     = 0;
    sithVec_ResetStubs();
    sithVec_numRegions = 0;
    sithVec_numFuncs   = 0;
    sithVec_AddRegion(&sithVec_arena, sizeof(sithVec_arena));
    sithVec_BuildWorld();
    pSuite->pfBuild(pCase);
}

static void sithAIFixTest_Shutdown(void)
{
    unsigned numAsserts = sithVec_numAsserts;
    unsigned numErrors  = sithVec_numErrors;
    sithVecB_Free(&sithVec_ev);
    sith_g_pHS = NULL;
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numAsserts, "Unexpected engine assertion");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numErrors, "Unexpected engine error");
}

#endif
