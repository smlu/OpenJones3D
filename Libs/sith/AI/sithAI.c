#include "sithAI.h"
#include <j3dcore/j3dhook.h>

#include <sith/Devices/sithConsole.h>
#include <sith/RTI/symbols.h>

#include <std/General/stdUtil.h>

#define sithAI_pRegisteredInstinctHashtbl J3D_DECL_FAR_VAR(sithAI_pRegisteredInstinctHashtbl, tHashTable*)
#define sithAI_numFreeAIs J3D_DECL_FAR_VAR(sithAI_numFreeAIs, int)
#define sithAI_aFreeAIIndices J3D_DECL_FAR_ARRAYVAR(sithAI_aFreeAIIndices, int(*)[256])
#define sithAI_bStartup J3D_DECL_FAR_VAR(sithAI_bStartup, int)
#define sithAI_aRegisteredInstincts J3D_DECL_FAR_VAR(sithAI_aRegisteredInstincts, SithAIRegisteredInstinct*)
#define sithAI_numRegisteredInstincts J3D_DECL_FAR_VAR(sithAI_numRegisteredInstincts, int)

void sithAI_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(sithAI_Startup);
    // J3D_HOOKFUNC(sithAI_Shutdown);
    // J3D_HOOKFUNC(sithAI_Open);
    // J3D_HOOKFUNC(sithAI_Close);
    // J3D_HOOKFUNC(sithAI_Create);
    // J3D_HOOKFUNC(sithAI_Free);
    // J3D_HOOKFUNC(sithAI_Process);
    // J3D_HOOKFUNC(sithAI_ProcessAIState);
    // J3D_HOOKFUNC(sithAI_InstinctUpdate);
    // J3D_HOOKFUNC(sithAI_ForceInstinctUpdate);
    // J3D_HOOKFUNC(sithAI_EmitEvent);
    // J3D_HOOKFUNC(sithAI_ProcessBlockedEvent);
    // J3D_HOOKFUNC(sithAI_ProcessUnhandledEvent);
    // J3D_HOOKFUNC(sithAI_ProcessUnhandledWpntEvent);
    // J3D_HOOKFUNC(sithAI_Stop);
    // J3D_HOOKFUNC(sithAI_StopAllAttackingAIs);
    // J3D_HOOKFUNC(sithAI_RegisterInstinct);
    // J3D_HOOKFUNC(sithAI_FindInstinct);
    // J3D_HOOKFUNC(sithAI_GetInstinctIndex);
    // J3D_HOOKFUNC(sithAI_HasInstinct);
    // J3D_HOOKFUNC(sithAI_EnableInstinct);
    // J3D_HOOKFUNC(sithAI_ParseArg);
    // J3D_HOOKFUNC(sithAI_AllocAIFrames);
    // J3D_HOOKFUNC(sithAI_CreateInstinctRegistry);
    // J3D_HOOKFUNC(sithAI_FreeAI);
    // J3D_HOOKFUNC(sithAI_CreateAI);
    // J3D_HOOKFUNC(sithAI_FreeAllAIs);
}

void sithAI_ResetGlobals(void)
{
    memset(&sithAI_pRegisteredInstinctHashtbl, 0, sizeof(sithAI_pRegisteredInstinctHashtbl));
    memset(&sithAI_numFreeAIs, 0, sizeof(sithAI_numFreeAIs));
    memset(&sithAI_aFreeAIIndices, 0, sizeof(sithAI_aFreeAIIndices));
    memset(&sithAI_g_bOpen, 0, sizeof(sithAI_g_bOpen));
    memset(&sithAI_bStartup, 0, sizeof(sithAI_bStartup));
    memset(&sithAI_aRegisteredInstincts, 0, sizeof(sithAI_aRegisteredInstincts));
    memset(&sithAI_numRegisteredInstincts, 0, sizeof(sithAI_numRegisteredInstincts));
    memset(&sithAI_g_aControlBlocks, 0, sizeof(sithAI_g_aControlBlocks));
    memset(&sithAI_g_lastUsedAIIndex, 0, sizeof(sithAI_g_lastUsedAIIndex));
}

int sithAI_Startup(void)
{
    return J3D_TRAMPOLINE_CALL(sithAI_Startup);
}

void J3DAPI sithAI_Shutdown()
{
    J3D_TRAMPOLINE_CALL(sithAI_Shutdown);
}

void J3DAPI sithAI_Open()
{
    J3D_TRAMPOLINE_CALL(sithAI_Open);
}

void J3DAPI sithAI_Close()
{
    J3D_TRAMPOLINE_CALL(sithAI_Close);
}

void J3DAPI sithAI_Create(SithThing* pThing)
{
    J3D_TRAMPOLINE_CALL(sithAI_Create, pThing);
}

void J3DAPI sithAI_Free(SithThing* pThing)
{
    J3D_TRAMPOLINE_CALL(sithAI_Free, pThing);
}

int J3DAPI sithAI_Process()
{
    return J3D_TRAMPOLINE_CALL(sithAI_Process);
}

void J3DAPI sithAI_ProcessAIState(SithAIControlBlock* pLocal)
{
    J3D_TRAMPOLINE_CALL(sithAI_ProcessAIState, pLocal);
}

unsigned int J3DAPI sithAI_InstinctUpdate(SithAIControlBlock* pLocal)
{
    return J3D_TRAMPOLINE_CALL(sithAI_InstinctUpdate, pLocal);
}

signed int J3DAPI sithAI_ForceInstinctUpdate(SithAIControlBlock* pLocal, const char* pInstinctName, int bSendModeChangeEvent)
{
    return J3D_TRAMPOLINE_CALL(sithAI_ForceInstinctUpdate, pLocal, pInstinctName, bSendModeChangeEvent);
}

void J3DAPI sithAI_EmitEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{
    J3D_TRAMPOLINE_CALL(sithAI_EmitEvent, pLocal, event, pObject);
}

int J3DAPI sithAI_ProcessBlockedEvent(SithAIControlBlock* pLocal, SithAIEventType aievent)
{
    return J3D_TRAMPOLINE_CALL(sithAI_ProcessBlockedEvent, pLocal, aievent);
}

int J3DAPI sithAI_ProcessUnhandledEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{
    return J3D_TRAMPOLINE_CALL(sithAI_ProcessUnhandledEvent, pLocal, event, pObject);
}

int J3DAPI sithAI_ProcessUnhandledWpntEvent(SithAIControlBlock* pLocal, SithAIEventType event)
{
    return J3D_TRAMPOLINE_CALL(sithAI_ProcessUnhandledWpntEvent, pLocal, event);
}

void J3DAPI sithAI_Stop(SithThing* pThing)
{
    J3D_TRAMPOLINE_CALL(sithAI_Stop, pThing);
}

void sithAI_StopAllAttackingAIs()
{
    J3D_TRAMPOLINE_CALL(sithAI_StopAllAttackingAIs);
}

int J3DAPI sithAI_RegisterInstinct(char* pName, SithAIInstinctFunc pfInstinct, SithAIMode updateModes, SithAIMode updateBlockModes, SithAIEventType triggerEvents)
{
    return J3D_TRAMPOLINE_CALL(sithAI_RegisterInstinct, pName, pfInstinct, updateModes, updateBlockModes, triggerEvents);
}

SithAIRegisteredInstinct* J3DAPI sithAI_FindInstinct(const char* pInstinctName)
{
    return J3D_TRAMPOLINE_CALL(sithAI_FindInstinct, pInstinctName);
}

int J3DAPI sithAI_GetInstinctIndex(const SithAIControlBlock* pLocal, const SithAIRegisteredInstinct* pRegInstinct)
{
    return J3D_TRAMPOLINE_CALL(sithAI_GetInstinctIndex, pLocal, pRegInstinct);
}

BOOL J3DAPI sithAI_HasInstinct(const SithAIControlBlock* pLocal, const char* pName)
{
    return J3D_TRAMPOLINE_CALL(sithAI_HasInstinct, pLocal, pName);
}

signed int J3DAPI sithAI_EnableInstinct(SithAIControlBlock* pLocal, const char* pInstinctName, int bEnable)
{
    return J3D_TRAMPOLINE_CALL(sithAI_EnableInstinct, pLocal, pInstinctName, bEnable);
}

signed int J3DAPI sithAI_ParseArg(StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    return J3D_TRAMPOLINE_CALL(sithAI_ParseArg, pArg, pThing, adjNum);
}

signed int J3DAPI sithAI_AllocAIFrames(SithAIControlBlock* pLocal, int sizeFrames)
{
    return J3D_TRAMPOLINE_CALL(sithAI_AllocAIFrames, pLocal, sizeFrames);
}

int J3DAPI sithAI_CreateInstinctRegistry(unsigned int maxInstincts)
{
    return J3D_TRAMPOLINE_CALL(sithAI_CreateInstinctRegistry, maxInstincts);
}

int J3DAPI sithAI_FreeAI(unsigned int aiNum)
{
    return J3D_TRAMPOLINE_CALL(sithAI_FreeAI, aiNum);
}

int J3DAPI sithAI_CreateAI()
{
    return J3D_TRAMPOLINE_CALL(sithAI_CreateAI);
}

void J3DAPI sithAI_FreeAllAIs()
{
    J3D_TRAMPOLINE_CALL(sithAI_FreeAllAIs);
}

int J3DAPI sithAI_AIList(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithAI_g_bOpen )
    {
        sithConsole_PrintString("AI system not open.\n");
        return 0;
    }

    sithConsole_PrintString("Active AI things:\n");

    for ( int i = 0; i <= sithAI_g_lastUsedAIIndex; ++i )
    {
        SithAIControlBlock* pLocal = &sithAI_g_aControlBlocks[i];
        if ( pLocal->pClass && pLocal->pOwner )
        {
            SITHCONSOLE_PRINTF(
                "Block %2d: Class '%s', Owner '%s' (%d), Flags 0x%x\n",
                i,
                pLocal->pClass->aName,
                pLocal->pOwner->aName,
                pLocal->pOwner->idx,
                pLocal->mode
            );
        }
    }

    return 1;
}

int J3DAPI sithAI_AIStatus(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    int idx;
    if ( !pArg || !sithAI_g_bOpen || sscanf_s(pArg, "%d", &idx) != 1 || idx > sithAI_g_lastUsedAIIndex )
    {
        sithConsole_PrintString("Cannot process AIStatus command.\n");
        return 0;
    }

    SithAIControlBlock* pLocal = &sithAI_g_aControlBlocks[idx];
    if ( !pLocal->pOwner )
    {
        sithConsole_PrintString("That AI block is not currently active.\n");
        return 1;
    }

    SITHCONSOLE_PRINTF("AI Status dump for thing %d (%s).\n", pLocal->pOwner->idx, pLocal->pOwner->aName);
    SITHCONSOLE_PRINTF(
        "Class '%s', Flags=0x%x, Moods %d/%d/%d, NextUpdate=%d\n",
        pLocal->pClass->aName,
        pLocal->mode,
        pLocal->aMoods[0],
        pLocal->aMoods[1],
        pLocal->aMoods[2],
        pLocal->msecNextUpdate
    );

    sithConsole_PrintString("Current instincts:\n");
    for ( int i = 0; i < pLocal->numInstincts; ++i )
    {
        SITHCONSOLE_PRINTF(
            "Instinct %d: Params: %f/%f/%f/%f, nextUpdate=%d, mask=0x%x, mode=0x%x.\n",
            i,
            pLocal->aInstinctStates[i].aParams[0],
            pLocal->aInstinctStates[i].aParams[1],
            pLocal->aInstinctStates[i].aParams[2],
            pLocal->aInstinctStates[i].aParams[3],
            pLocal->aInstinctStates[i].msecNextUpdate,
            pLocal->pClass->aInstincts[i].triggerEvents,
            pLocal->pClass->aInstincts[i].updateModes
        );
    }

    return 1;
}
