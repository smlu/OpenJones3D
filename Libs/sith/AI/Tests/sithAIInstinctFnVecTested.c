// sithAIInstinct.c as tested by the sithAIInstinctFn suite of sithTests: compiled with the redirected engine globals of the suite (see
// Libs/sith/Tests/Vec/sith_vec.cmake) and with access to its file statics.
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
