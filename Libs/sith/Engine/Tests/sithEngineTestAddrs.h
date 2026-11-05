#ifndef SITH_ENGINE_TEST_ADDRS_H
#define SITH_ENGINE_TEST_ADDRS_H

#include <stdint.h>
#include <j3dcore/j3d.h>
#include <rdroid/RTI/addresses.h>
#include <sith/RTI/addresses.h>
#include <std/RTI/addresses.h>

typedef struct sHostServices tHostServices;
typedef struct sSithWorld SithWorld;
extern SithWorld* sithEngineTest_pCurrentWorld;
extern tHostServices* sithEngineTest_pSithHostServices;
extern tHostServices* sithEngineTest_pStdHostServices;
extern tHostServices* sithEngineTest_pRdroidHostServices;

// Redirect only the assertion-service slots used by the tested modules. Real fixed
// addresses can overlap the test process heap; J3DTest binds these local slots instead.
#undef sith_g_pHS_ADDR
#define sith_g_pHS_ADDR ((uintptr_t)&sithEngineTest_pSithHostServices)
#undef std_g_pHS_ADDR
#define std_g_pHS_ADDR ((uintptr_t)&sithEngineTest_pStdHostServices)
#undef rdroid_g_pHS_ADDR
#define rdroid_g_pHS_ADDR ((uintptr_t)&sithEngineTest_pRdroidHostServices)

// Falling tests supply a complete synthetic world without occupying retail memory.
#undef sithWorld_g_pCurrentWorld_ADDR
#define sithWorld_g_pCurrentWorld_ADDR ((uintptr_t)&sithEngineTest_pCurrentWorld)

#endif
