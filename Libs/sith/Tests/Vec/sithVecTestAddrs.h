// Force-included into the sources of a sith vector test executable (see SithVecTest.cmake), before the suite's own
// address header.
//
// The reconstructed code reaches engine globals of the original executable through J3D_DECL_FAR_VAR at fixed
// addresses, which are not mapped in the test process. A suite redirects the globals its tested functions use to
// static storage with:
//
//     SITHVEC_REDIRECT_DECL(float, sithTime_g_fps);
//     #undef sithTime_g_fps_ADDR
//     #define sithTime_g_fps_ADDR SITHVEC_REDIRECT_ADDR(sithTime_g_fps)
//
// in its address header and SITHVEC_REDIRECT_DEFINE(float, sithTime_g_fps); in its test source. The capture harness
// does not use these headers: it runs inside the executable and uses the real addresses.
#ifndef SITH_TESTS_VEC_SITHVECTESTADDRS_H
#define SITH_TESTS_VEC_SITHVECTESTADDRS_H

#include <j3dcore/j3d.h>

#include <stdint.h>
#include <stdlib.h>

#include <rdroid/RTI/addresses.h>
#include <sith/RTI/addresses.h>
#include <std/RTI/addresses.h>

#define SITHVEC_REDIRECT_DECL(type, name)   extern type sithVecTest_g_##name
#define SITHVEC_REDIRECT_DEFINE(type, name) type sithVecTest_g_##name
#define SITHVEC_REDIRECT_ADDR(name)         ((uintptr_t)&sithVecTest_g_##name)

// The C runtime's rand is replaced by the deterministic stub of sithVecWorld.h (the capture harness patches the
// original's rand at 0x004F7010 to the same stub). A macro instead of a seam rename because <stdlib.h> declares rand
// as a DLL import in the Debug CRT.
int __cdecl sithVec_rand(void);
#define rand sithVec_rand

#endif // SITH_TESTS_VEC_SITHVECTESTADDRS_H
