// sithAI.c as tested by sithAIInstinctVecTests: compiled with the redirected engine globals of the suite (see
// Libs/sith/Tests/Vec/CMakeLists.txt) and with access to the instinct registry, a file static of sithAI.c.
#include "../sithAI.c"

SithAIRegisteredInstinct** sithAIVecTest_GetRegisteredInstincts(void)
{
    return &aRegisteredInstincts;
}

size_t* sithAIVecTest_GetNumRegisteredInstincts(void)
{
    return &numRegisteredInstincts;
}
