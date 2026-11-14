// Force-included into the sources of the sithAIMove suite (sithTests) after Libs/sith/Tests/Vec/sithVecTestAddrs.h: redirects the
// engine globals the sithAIMove functions use to static storage (defined in sithAIMoveVecTest.c).
#ifndef SITH_AI_TESTS_SITHAIMOVEVECADDRS_H
#define SITH_AI_TESTS_SITHAIMOVEVECADDRS_H

SITHVEC_REDIRECT_DECL(uint32_t, sithTime_g_msecGameTime);
SITHVEC_REDIRECT_DECL(uint32_t, sithTime_g_frameTime); // read by sithTime_CalcDeadline
#undef sithTime_g_msecGameTime_ADDR
#define sithTime_g_msecGameTime_ADDR SITHVEC_REDIRECT_ADDR(sithTime_g_msecGameTime)
#undef sithTime_g_frameTime_ADDR
#define sithTime_g_frameTime_ADDR SITHVEC_REDIRECT_ADDR(sithTime_g_frameTime)

SITHVEC_REDIRECT_DECL(size_t, sithMain_g_frameNumber);
#undef sithMain_g_frameNumber_ADDR
#define sithMain_g_frameNumber_ADDR SITHVEC_REDIRECT_ADDR(sithMain_g_frameNumber)

SITHVEC_REDIRECT_DECL(SithMode, sithMain_g_sith_mode);
#undef sithMain_g_sith_mode_ADDR
#define sithMain_g_sith_mode_ADDR SITHVEC_REDIRECT_ADDR(sithMain_g_sith_mode)

#endif // SITH_AI_TESTS_SITHAIMOVEVECADDRS_H
