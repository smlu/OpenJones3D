// Force-included into the sources of sithThingQuetzVecTests after Libs/sith/Tests/Vec/sithVecTestAddrs.h: redirects
// the engine globals the Quetz functions use to static storage (defined in sithThingQuetzVecTest.c).
#ifndef SITH_WORLD_TESTS_SITHTHINGQUETZVECADDRS_H
#define SITH_WORLD_TESTS_SITHTHINGQUETZVECADDRS_H

SITHVEC_REDIRECT_DECL(float, sithTime_g_fps);
#undef sithTime_g_fps_ADDR
#define sithTime_g_fps_ADDR SITHVEC_REDIRECT_ADDR(sithTime_g_fps)

#endif // SITH_WORLD_TESTS_SITHTHINGQUETZVECADDRS_H
