// Force-included into the sithDSS tests and the sources they test (see CMakeLists.txt).
//
// The vector cases seed synthetic puppet tracks with the "vanilla" callbacks of the original executable through
// the historical <function>_ADDR macros, and the reconstructed sithDSS.c save path recognizes those values; the
// redirects below give the seven callbacks distinct static sentinel addresses, standing in for the retail
// function addresses. They must differ from the address of the callback function itself: the load cases record
// the change of the seeded value. The engine-global redirects at the end of this header are no longer read (the
// cases and the tested sources use the module globals of the reconstruction directly) and are kept only until
// they are removed in a separate cleanup.
#ifndef SITHDSSTESTADDRS_H
#define SITHDSSTESTADDRS_H

#include <j3dcore/j3d.h>

#include <stddef.h>
#include <stdint.h>

#include <sith/types.h>
#include <sith/Gameplay/sithPlayer.h>

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
    void* aPuppetCallbackSentinels[7];
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
#undef sithPlayerActions_g_pCurHitLedgeSurface_ADDR
#undef sithPlayerActions_g_pCurHitLedgeModel_ADDR
#undef sithPlayerActions_g_pCurHitLedgeModelFace_ADDR
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
#define sithPlayerActions_g_pCurHitLedgeSurface_ADDR ((uintptr_t)&sithDSSTest_g.pCurCollidedSurface)
#define sithPlayerActions_g_pCurHitLedgeModel_ADDR ((uintptr_t)&sithDSSTest_g.pCurCollidedModel)
#define sithPlayerActions_g_pCurHitLedgeModelFace_ADDR ((uintptr_t)&sithDSSTest_g.pCurCollidedModelFace)
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

// Sentinels for the seven puppet track callbacks the DSS and Multi vector cases seed and the reconstructed
// sithDSS.c save path names by index (see the VANILLACB macro in sithDSS.c). Distinct from every function
// address, so the seeded value and the reloaded function pointer produce the same record as before.
#define sithPuppet_DefaultCallback_ADDR           ((uintptr_t)&sithDSSTest_g.aPuppetCallbackSentinels[0])
#define sithPlayerControls_PuppetCallback_ADDR    ((uintptr_t)&sithDSSTest_g.aPuppetCallbackSentinels[1])
#define sithVehicleControls_PuppetCallback_ADDR   ((uintptr_t)&sithDSSTest_g.aPuppetCallbackSentinels[2])
#define sithAIMove_PuppetCallback_ADDR            ((uintptr_t)&sithDSSTest_g.aPuppetCallbackSentinels[3])
#define sithWhip_WhipClimbPuppetCallback_ADDR     ((uintptr_t)&sithDSSTest_g.aPuppetCallbackSentinels[4])
#define sithWhip_ClimbDismountPuppetCallback_ADDR ((uintptr_t)&sithDSSTest_g.aPuppetCallbackSentinels[5])
#define sithWhip_WhipFirePuppetCallback_ADDR      ((uintptr_t)&sithDSSTest_g.aPuppetCallbackSentinels[6])

#endif // SITHDSSTESTADDRS_H
