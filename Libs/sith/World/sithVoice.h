#ifndef SITH_SITHVOICE_H
#define SITH_SITHVOICE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHVOICE_CFG_GAMEPLAY_SHOWTEXT "gameplay.showText"

int sithVoice_Startup(void);
void sithVoice_Shutdown(void);

int sithVoice_Open(void);
void sithVoice_Close(void);

int J3DAPI sithVoice_RegisterCogFunctions(SithCogSymbolTable* pTbl);

int J3DAPI sithVoice_PlayThingVoice(SithThing* pThing, tSoundHandle hSnd, float volume);
void J3DAPI sithVoice_UpdateLipSync(SithThing* pThing);

int sithVoice_GetShowText(void);
void J3DAPI sithVoice_ShowText(int bShow);

void sithVoice_Draw(void);

int J3DAPI sithVoice_SyncVoiceState(DPID idTo, unsigned int outstream);
int J3DAPI sithVoice_ProcessVoiceState(const SithMessage* pMsg);

const rdFont* sithVoice_GetTextFont(void);

#if defined(J3D_TEST)
typedef struct SithVoiceTestState
{
    int bLipSyncData;
    uint8_t mouthPosX;
    uint8_t mouthPosY;
    float secGameTime;
    int addSwapResult;
    size_t numStopCalls;
    size_t numAddSwapCalls;
    size_t numRemoveSwapCalls;
    tSoundChannelHandle hLastStoppedChannel;
    SithThing* pLastSwapThing;
    rdModel3* pLastAddedModel;
    int lastAddedMeshNum;
    int lastAddedSrcMeshNum;
    int lastRemovedRefNum;
    int bThingHasSwapHead;
    int curHeadSlot;
    int lastHeadSlot;
    size_t sameHeadCounter;
    float secNextHeadSwapTime;
} SithVoiceTestState;

void sithVoice_TestResetState(void);
void sithVoice_TestSetLipSyncResult(int bLipSyncData, uint8_t mouthPosX, uint8_t mouthPosY);
void sithVoice_TestSetGameTime(float secGameTime);
void sithVoice_TestSetHeadTableEntry(size_t mouthYLevel, size_t mouthXLevel, uint8_t headSlot);
void sithVoice_TestSetSelectionState(int lastHeadSlot, size_t sameHeadCounter, float secNextHeadSwapTime);
void sithVoice_TestSetThingHasSwapHead(int bThingHasSwapHead);
void sithVoice_TestGetState(SithVoiceTestState* pState);
#endif

// Helper hooking functions
void sithVoice_InstallHooks(void);
void sithVoice_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHVOICE_H
