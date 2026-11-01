// sithAIInstinct.c as tested by sithAIInstinctFnVecTests: compiled with the redirected engine globals of the suite (see
// Libs/sith/Tests/Vec/CMakeLists.txt) and with access to its file statics.
#include "../sithAIInstinct.c"

float* sithAIFnVecTest_GetSnakeLastDotFwd(void)
{
    return &sithAIInstinct_snakeLastDotFwd;
}

float* sithAIFnVecTest_GetSnakeSlitherDir(void)
{
    return &sithAIInstinct_snakeSlitherDir;
}

float* sithAIFnVecTest_GetSnakeFollowNearRange(void)
{
    return &sithAIInstinct_snakeFollowNearRange;
}
