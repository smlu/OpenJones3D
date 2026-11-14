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

// rdMatrix_Identity34 copies the identity matrix from its retail address, which holds other data in the test process.
typedef struct srdMatrix34 rdMatrix34;
extern const rdMatrix34 sithEngineTest_identMatrix34;
#undef rdroid_g_identMatrix34_ADDR
#define rdroid_g_identMatrix34_ADDR ((uintptr_t)&sithEngineTest_identMatrix34)

// Frame-rate tests set the engine frame rate and the local player's IMP state for every simulated frame.
typedef struct sSithPlayer SithPlayer;
extern float sithEngineTest_fps;
extern SithPlayer* sithEngineTest_pLocalPlayer;
extern int sithEngineTest_impFireType;
extern float sithEngineTest_impState;
extern int sithEngineTest_bPlayerInPor;

extern uint32_t sithEngineTest_msecGameTime;
extern float sithEngineTest_secGameTime;
typedef struct sSithEvent SithEvent;
extern SithEvent* sithEngineTest_pFirstQueuedEvent;
extern uint32_t sithEngineTest_msecFrameTime;
extern float sithEngineTest_frameTimeFlex;
extern size_t sithEngineTest_frameNumber;

#undef sithTime_g_fps_ADDR
#define sithTime_g_fps_ADDR ((uintptr_t)&sithEngineTest_fps)
#undef sithTime_g_msecGameTime_ADDR
#define sithTime_g_msecGameTime_ADDR ((uintptr_t)&sithEngineTest_msecGameTime)
#undef sithTime_g_secGameTime_ADDR
#define sithTime_g_secGameTime_ADDR ((uintptr_t)&sithEngineTest_secGameTime)
#undef sithEvent_g_pFirstQueuedEvent_ADDR
#define sithEvent_g_pFirstQueuedEvent_ADDR ((uintptr_t)&sithEngineTest_pFirstQueuedEvent)
#undef sithTime_g_frameTime_ADDR
#define sithTime_g_frameTime_ADDR ((uintptr_t)&sithEngineTest_msecFrameTime)
#undef sithTime_g_frameTimeFlex_ADDR
#define sithTime_g_frameTimeFlex_ADDR ((uintptr_t)&sithEngineTest_frameTimeFlex)
#undef sithMain_g_frameNumber_ADDR
#define sithMain_g_frameNumber_ADDR ((uintptr_t)&sithEngineTest_frameNumber)
#undef sithPlayer_g_pLocalPlayer_ADDR
#define sithPlayer_g_pLocalPlayer_ADDR ((uintptr_t)&sithEngineTest_pLocalPlayer)
#undef sithPlayer_g_impFireType_ADDR
#define sithPlayer_g_impFireType_ADDR ((uintptr_t)&sithEngineTest_impFireType)
#undef sithPlayer_g_impState_ADDR
#define sithPlayer_g_impState_ADDR ((uintptr_t)&sithEngineTest_impState)
#undef sithPlayer_g_bPlayerInPor_ADDR
#define sithPlayer_g_bPlayerInPor_ADDR ((uintptr_t)&sithEngineTest_bPlayerInPor)

#endif
