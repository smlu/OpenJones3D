// Force-included into the sithDSS tests and the sources they test (see CMakeLists.txt).
//
// The reconstructed code reaches engine globals of the original executable through J3D_DECL_FAR_VAR at fixed
// addresses. Those addresses are not available in the test process (and cannot be mapped under every host), so the
// addresses of the globals that the DSS functions use are redirected to a static structure. The capture harness
// does not use this header: it runs inside the executable and uses the real addresses.
#ifndef SITHDSSTESTADDRS_H
#define SITHDSSTESTADDRS_H

#include <j3dcore/j3d.h>

#include <stddef.h>
#include <stdint.h>

#include <sith/types.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/RTI/addresses.h>

typedef struct
{
    SithMessage message;
    DPID serverId;
    uint32_t msecGameTime;
    size_t frameNumber;
    SithMode sithMode;
    SithCamera aCameras[7];
    SithCamera* pCurCamera;
    int bCurCameraSet;
    size_t curCycleCamNum;
    rdVector3 vecCameraPosOffset;
    rdVector3 vecCameraAngleOffset;
    float cameraPosDelta;
    float cameraAngleDelta;
    SithPlayer aPlayers[SITHPLAYER_MAX_PLAYERS];
    SithPlayer* pLocalPlayer;
    int bPlayerInPor;
    float impState;
    int bInAetheriumSector;
    int bGuybrush;
    int impFireType;
    int curLevelNum;
    int bBonusMapBought;
    SithPowerKeybind aInventoryPowerKeybinds[20];
    SithInventoryType aInventoryTypes[SITHINVENTORY_MAXTYPES];
    int bSendDeactivateMessage;
    int bInitInventory;
    int dword_56B750;
    int dword_56B754;
    SithSurface* pCurCollidedSurface;
    rdModel3* pCurCollidedModel;
    rdFace* pCurCollidedModelFace;
    SithThing* pPlasma;
    int jewelFlyingPuppetTrackNum;
    int bJewelFlying;
    int bPlayerInvisible;
    int bCutsceneMode;
    int bPlayerLeapForward;
    size_t numChalkMarks;
    int lastChalkMarkNum;
    SithThing* aChalkMarks[10];
    SithCog* pMasterCog;
} SithDSSTestGlobals;

extern SithDSSTestGlobals sithDSSTest_g;

#undef sithMulti_g_message_ADDR
#undef sithMulti_g_serverId_ADDR
#undef sithTime_g_msecGameTime_ADDR
#undef sithMain_g_frameNumber_ADDR
#undef sithMain_g_sith_mode_ADDR
#undef sithCamera_g_aCameras_ADDR
#undef sithCamera_g_pCurCamera_ADDR
#undef sithCamera_g_bCurCameraSet_ADDR
#undef sithCamera_g_curCycleCamNum_ADDR
#undef sithCamera_g_vecCameraPosOffset_ADDR
#undef sithCamera_g_vecCameraAngleOffset_ADDR
#undef sithCamera_g_cameraPosDelta_ADDR
#undef sithCamera_g_cameraAngleDelta_ADDR
#undef sithPlayer_g_aPlayers_ADDR
#undef sithPlayer_g_pLocalPlayer_ADDR
#undef sithPlayer_g_bPlayerInPor_ADDR
#undef sithPlayer_g_impState_ADDR
#undef sithPlayer_g_bInAetheriumSector_ADDR
#undef sithPlayer_g_bGuybrush_ADDR
#undef sithPlayer_g_impFireType_ADDR
#undef sithPlayer_g_curLevelNum_ADDR
#undef sithPlayer_g_bBonusMapBought_ADDR
#undef sithInventory_g_aPowerKeybinds_ADDR
#undef sithInventory_g_aTypes_ADDR
#undef sithInventory_g_bSendDeactivateMessage_ADDR
#undef sithInventory_g_bInitInventory_ADDR
#undef sithInventory_g_dword_56B750_ADDR
#undef sithInventory_g_dword_56B754_ADDR
#undef sithPlayerActions_g_pCurCollidedSurface_ADDR
#undef sithPlayerActions_g_pCurCollidedModel_ADDR
#undef sithPlayerActions_g_pCurCollidedModelFace_ADDR
#undef sithPlayerActions_g_pPlasma_ADDR
#undef sithPlayerActions_g_jewelFlyingPuppetTrackNum_ADDR
#undef sithPlayerActions_g_bJewelFlying_ADDR
#undef sithPlayerActions_g_bPlayerInvisible_ADDR
#undef sithPlayerControls_g_bCutsceneMode_ADDR
#undef sithPuppet_g_bPlayerLeapForward_ADDR
#undef sithFX_g_numChalkMarks_ADDR
#undef sithFX_g_lastChalkMarkNum_ADDR
#undef sithFX_g_aChalkMarks_ADDR
#undef sithCog_g_pMasterCog_ADDR

#define sithMulti_g_message_ADDR ((uintptr_t)&sithDSSTest_g.message)
#define sithMulti_g_serverId_ADDR ((uintptr_t)&sithDSSTest_g.serverId)
#define sithTime_g_msecGameTime_ADDR ((uintptr_t)&sithDSSTest_g.msecGameTime)
#define sithMain_g_frameNumber_ADDR ((uintptr_t)&sithDSSTest_g.frameNumber)
#define sithMain_g_sith_mode_ADDR ((uintptr_t)&sithDSSTest_g.sithMode)
#define sithCamera_g_aCameras_ADDR ((uintptr_t)&sithDSSTest_g.aCameras)
#define sithCamera_g_pCurCamera_ADDR ((uintptr_t)&sithDSSTest_g.pCurCamera)
#define sithCamera_g_bCurCameraSet_ADDR ((uintptr_t)&sithDSSTest_g.bCurCameraSet)
#define sithCamera_g_curCycleCamNum_ADDR ((uintptr_t)&sithDSSTest_g.curCycleCamNum)
#define sithCamera_g_vecCameraPosOffset_ADDR ((uintptr_t)&sithDSSTest_g.vecCameraPosOffset)
#define sithCamera_g_vecCameraAngleOffset_ADDR ((uintptr_t)&sithDSSTest_g.vecCameraAngleOffset)
#define sithCamera_g_cameraPosDelta_ADDR ((uintptr_t)&sithDSSTest_g.cameraPosDelta)
#define sithCamera_g_cameraAngleDelta_ADDR ((uintptr_t)&sithDSSTest_g.cameraAngleDelta)
#define sithPlayer_g_aPlayers_ADDR ((uintptr_t)&sithDSSTest_g.aPlayers)
#define sithPlayer_g_pLocalPlayer_ADDR ((uintptr_t)&sithDSSTest_g.pLocalPlayer)
#define sithPlayer_g_bPlayerInPor_ADDR ((uintptr_t)&sithDSSTest_g.bPlayerInPor)
#define sithPlayer_g_impState_ADDR ((uintptr_t)&sithDSSTest_g.impState)
#define sithPlayer_g_bInAetheriumSector_ADDR ((uintptr_t)&sithDSSTest_g.bInAetheriumSector)
#define sithPlayer_g_bGuybrush_ADDR ((uintptr_t)&sithDSSTest_g.bGuybrush)
#define sithPlayer_g_impFireType_ADDR ((uintptr_t)&sithDSSTest_g.impFireType)
#define sithPlayer_g_curLevelNum_ADDR ((uintptr_t)&sithDSSTest_g.curLevelNum)
#define sithPlayer_g_bBonusMapBought_ADDR ((uintptr_t)&sithDSSTest_g.bBonusMapBought)
#define sithInventory_g_aPowerKeybinds_ADDR ((uintptr_t)&sithDSSTest_g.aInventoryPowerKeybinds)
#define sithInventory_g_aTypes_ADDR ((uintptr_t)&sithDSSTest_g.aInventoryTypes)
#define sithInventory_g_bSendDeactivateMessage_ADDR ((uintptr_t)&sithDSSTest_g.bSendDeactivateMessage)
#define sithInventory_g_bInitInventory_ADDR ((uintptr_t)&sithDSSTest_g.bInitInventory)
#define sithInventory_g_dword_56B750_ADDR ((uintptr_t)&sithDSSTest_g.dword_56B750)
#define sithInventory_g_dword_56B754_ADDR ((uintptr_t)&sithDSSTest_g.dword_56B754)
#define sithPlayerActions_g_pCurCollidedSurface_ADDR ((uintptr_t)&sithDSSTest_g.pCurCollidedSurface)
#define sithPlayerActions_g_pCurCollidedModel_ADDR ((uintptr_t)&sithDSSTest_g.pCurCollidedModel)
#define sithPlayerActions_g_pCurCollidedModelFace_ADDR ((uintptr_t)&sithDSSTest_g.pCurCollidedModelFace)
#define sithPlayerActions_g_pPlasma_ADDR ((uintptr_t)&sithDSSTest_g.pPlasma)
#define sithPlayerActions_g_jewelFlyingPuppetTrackNum_ADDR ((uintptr_t)&sithDSSTest_g.jewelFlyingPuppetTrackNum)
#define sithPlayerActions_g_bJewelFlying_ADDR ((uintptr_t)&sithDSSTest_g.bJewelFlying)
#define sithPlayerActions_g_bPlayerInvisible_ADDR ((uintptr_t)&sithDSSTest_g.bPlayerInvisible)
#define sithPlayerControls_g_bCutsceneMode_ADDR ((uintptr_t)&sithDSSTest_g.bCutsceneMode)
#define sithPuppet_g_bPlayerLeapForward_ADDR ((uintptr_t)&sithDSSTest_g.bPlayerLeapForward)
#define sithFX_g_numChalkMarks_ADDR ((uintptr_t)&sithDSSTest_g.numChalkMarks)
#define sithFX_g_lastChalkMarkNum_ADDR ((uintptr_t)&sithDSSTest_g.lastChalkMarkNum)
#define sithFX_g_aChalkMarks_ADDR ((uintptr_t)&sithDSSTest_g.aChalkMarks)
#define sithCog_g_pMasterCog_ADDR ((uintptr_t)&sithDSSTest_g.pMasterCog)

#endif // SITHDSSTESTADDRS_H
