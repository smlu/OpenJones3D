// Force-included into the sources of sithAIInstinctVecTests after Libs/sith/Tests/Vec/sithVecTestAddrs.h: redirects
// the engine globals the tested functions use to static storage (defined in sithAIInstinctVecTest.c).
// The scan-restart regression exercises the real instinct-update caller.
#ifndef SITH_AI_TESTS_SITHAIINSTINCTVECADDRS_H
#define SITH_AI_TESTS_SITHAIINSTINCTVECADDRS_H

SITHVEC_REDIRECT_DECL(uint32_t, sithTime_g_msecGameTime);
SITHVEC_REDIRECT_DECL(uint32_t, sithTime_g_frameTime); // read by sithTime_CalcDeadline
#undef sithTime_g_msecGameTime_ADDR
#define sithTime_g_msecGameTime_ADDR SITHVEC_REDIRECT_ADDR(sithTime_g_msecGameTime)
#undef sithTime_g_frameTime_ADDR
#define sithTime_g_frameTime_ADDR SITHVEC_REDIRECT_ADDR(sithTime_g_frameTime)

#endif // SITH_AI_TESTS_SITHAIINSTINCTVECADDRS_H
