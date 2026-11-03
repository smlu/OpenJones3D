#ifndef SITH_SITHAI_H
#define SITH_SITHAI_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>

#include <std/types.h>
#include <std/General/stdConffile.h>

J3D_EXTERN_C_START

extern int sithAI_g_bOpen;
// extern int sithAI_g_bOpen;

extern SithAIControlBlock sithAI_g_aControlBlocks[256];
// extern SithAIControlBlock sithAI_g_aControlBlocks[256];

extern int sithAI_g_lastUsedAIIndex;
// extern int sithAI_g_lastUsedAIIndex;

int sithAI_Startup(void);
void sithAI_Shutdown(void);
void sithAI_Open(void);
void sithAI_Close(void);

void J3DAPI sithAI_Create(SithThing* pThing);
void J3DAPI sithAI_Free(SithThing* pThing);

void sithAI_Process(void);
int J3DAPI sithAI_ForceInstinctUpdate(SithAIControlBlock* pLocal, const char* pInstinctName, int bSendModeChangeEvent);

void J3DAPI sithAI_EmitEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject);

void J3DAPI sithAI_Stop(SithThing* pThing);
void sithAI_StopAllAttackingAIs(void);

int J3DAPI sithAI_RegisterInstinct(const char* pName, SithAIInstinctFunc pfInstinct, SithAIMode updateModes, SithAIMode updateBlockModes, SithAIEventType triggerEvents);
SithAIRegisteredInstinct* J3DAPI sithAI_FindInstinct(const char* pInstinctName);
int J3DAPI sithAI_GetInstinctIndex(const SithAIControlBlock* pLocal, const SithAIRegisteredInstinct* pRegInstinct);
int J3DAPI sithAI_HasInstinct(const SithAIControlBlock* pLocal, const char* pName);
int J3DAPI sithAI_EnableInstinct(SithAIControlBlock* pLocal, const char* pInstinctName, int bEnable);

int J3DAPI sithAI_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum);
int J3DAPI sithAI_AllocAIFrames(SithAIControlBlock* pLocal, size_t sizeFrames);
void J3DAPI sithAI_CreateAIFramesFomMarker(SithThing* pNewThing, const SithThing* pMarker, const rdVector3* pivot); // Added: From debug version

int J3DAPI sithAI_AIList(const SithConsoleCommand* pFunc, const char* pArg); //Added: From debug version
int J3DAPI sithAI_AIStatus(const SithConsoleCommand* pFunc, const char* pArg); //Added: From debug version

// Helper hooking functions
void sithAI_InstallHooks(void);
void sithAI_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHAI_H
