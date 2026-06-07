#include "sithCog.h"
#include <j3dcore/j3dhook.h>

#include <sith/AI/sithAIClass.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Cog/sithCogFunction.h>
#include <sith/Cog/sithCogFunctionAI.h>
#include <sith/Cog/sithCogFunctionPlayer.h>
#include <sith/Cog/sithCogFunctionSector.h>
#include <sith/Cog/sithCogFunctionSound.h>
#include <sith/Cog/sithCogFunctionSurface.h>
#include <sith/Cog/sithCogFunctionThing.h>
#include <sith/Cog/sithCogParse.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Dss/sithDSSCog.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#include <stdlib.h>

#define SITHCOG_MAXGLOBALSYMBOLS 1024u
#define SITHCOG_MAXSCRIPTS       256u
#define SITHCOG_SYMVALUESTRLEN   64

static bool sithCog_bCogStartup = false; // Altered: Init to false
static bool sithCog_bCogOpen    = false; // Altered: Init to false

static tHashTable* sithCog_pHashtable = NULL;  // Altered: Init to NULL

static size_t numSectorLinks = 0u; // Altered: Init to 0
static SithCogSectorLink aCogLinkSectors[512] = { 0 }; // Added: Init to 0

static size_t numSurfLinks= 0u; // Altered: Init to 0
static SithCogSurfaceLink aCogLinkSurfaces[512]= { 0 }; // Added: Init to 0

static size_t numThingLinks = 0u; // Altered: Init to 0
static SithCogThingLink aThingLinks[1024]= { 0 }; // Added: Init to 0

int J3DAPI sithCog_LinkCog(const SithWorld* pWorld, SithCog* pCog, const SithCogSymbolRef* pRef, const SithCogSymbol* pSymbol);
void J3DAPI sithCog_GetSymbolRefInitializer(const SithWorld* pWorld, const SithCog* pCog, int symIdx, char* pOutString);

SithCogScript* J3DAPI sithCog_GetScript(const char* pName);
int J3DAPI sithCog_AddScript(SithCogScript* pScript);
int J3DAPI sithCog_RemoveScript(SithCogScript* pScript);

int J3DAPI sithCog_TimerEventTask(int msecTime, SithEventParams* pParams);
void J3DAPI sithCog_Initialize();

void sithCog_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCog_Startup);
    J3D_HOOKFUNC(sithCog_Shutdown);
    J3D_HOOKFUNC(sithCog_Open);
    J3D_HOOKFUNC(sithCog_Close);
    J3D_HOOKFUNC(sithCog_FreeWorldCogs);
    J3D_HOOKFUNC(sithCog_FreeScriptEntry);
    J3D_HOOKFUNC(sithCog_FreeEntry);
    J3D_HOOKFUNC(sithCog_UpdateThingTimer);
    J3D_HOOKFUNC(sithCog_BroadcastMessage);
    J3D_HOOKFUNC(sithCog_BroadcastMessageEx);
    J3D_HOOKFUNC(sithCog_SendMessage);
    J3D_HOOKFUNC(sithCog_SendMessageEx);
    J3D_HOOKFUNC(sithCog_AllocWorldCogScripts);
    J3D_HOOKFUNC(sithCog_AllocWorldCogs);
    J3D_HOOKFUNC(sithCog_GetSymbolRefInitializer);
    J3D_HOOKFUNC(sithCog_ReadCogsListText);
    J3D_HOOKFUNC(sithCog_WriteCogsListBinary);
    J3D_HOOKFUNC(sithCog_ReadCogsListBinary);
    J3D_HOOKFUNC(sithCog_ParseSymbolRef);
    J3D_HOOKFUNC(sithCog_LinkCog);
    J3D_HOOKFUNC(sithCog_Load);
    J3D_HOOKFUNC(sithCog_GetCogByIndex);
    J3D_HOOKFUNC(sithCog_GetCog);
    J3D_HOOKFUNC(sithCog_LinkCogToThing);
    J3D_HOOKFUNC(sithCog_LinkCogToSurface);
    J3D_HOOKFUNC(sithCog_LinkCogToSector);
    J3D_HOOKFUNC(sithCog_FixupLinksToThings);
    J3D_HOOKFUNC(sithCog_ThingSendMessage);
    J3D_HOOKFUNC(sithCog_ThingSendMessageEx);
    J3D_HOOKFUNC(sithCog_SectorSendMessage);
    J3D_HOOKFUNC(sithCog_SectorSendMessageEx);
    J3D_HOOKFUNC(sithCog_SurfaceSendMessage);
    J3D_HOOKFUNC(sithCog_SurfaceSendMessageEx);
    J3D_HOOKFUNC(sithCog_ReadCogScriptsListText);
    J3D_HOOKFUNC(sithCog_WriteCogScriptsListBinary);
    J3D_HOOKFUNC(sithCog_ReadCogScriptsListBinary);
    J3D_HOOKFUNC(sithCog_LoadScript);
    J3D_HOOKFUNC(sithCog_ProcessCog);
    J3D_HOOKFUNC(sithCog_ProcessCogs);
    J3D_HOOKFUNC(sithCog_AddIntSymbol);
    J3D_HOOKFUNC(sithCog_TimerEventTask);
    J3D_HOOKFUNC(sithCog_GetScript);
    J3D_HOOKFUNC(sithCog_AddScript);
    J3D_HOOKFUNC(sithCog_RemoveScript);
    J3D_HOOKFUNC(sithCog_Initialize);
    J3D_HOOKFUNC(sithCog_AddFloatSymbol);
    J3D_HOOKFUNC(sithCog_RegisterFunction);
    J3D_HOOKFUNC(sithCog_IsThingLinked);
}

void sithCog_ResetGlobals(void)
{
    STD_ZEROMEM(&sithCog_g_pSymbolTable, sizeof(sithCog_g_pSymbolTable));
    STD_ZEROMEM(&sithCog_g_pMasterCog, sizeof(sithCog_g_pMasterCog));
}

int sithCog_Startup(void)
{
    if ( sithCog_bCogStartup )
    {
        return 1;
    }

    sithCog_g_pSymbolTable = sithCogParse_AllocSymbolTable(SITHCOG_MAXGLOBALSYMBOLS);
    if ( !sithCog_g_pSymbolTable )
    {
        SITHLOG_ERROR("Could not allocate COG symboltable\n");
        return 1;
    }

    sithCog_pHashtable = stdHashtbl_New(SITHCOG_MAXSCRIPTS);
    if ( !sithCog_pHashtable )
    {
        SITHLOG_ERROR("Could not allocate COG hashtable.\n");
        return 1;
    }

    sithCog_g_pSymbolTable->firstId = SITHCOG_GLOBALSYMBOLSTARTID;

    sithCog_Initialize();
    sithEvent_RegisterTask(SITHCOG_TASKID, sithCog_TimerEventTask, 0, SITHEVENT_TASKONDEMAND);
    sithCog_bCogStartup = 1;
    return 0;
}

void sithCog_Shutdown(void)
{
    if ( sithCog_g_pSymbolTable )
    {
        sithCogParse_FreeSymbolTable(sithCog_g_pSymbolTable);
        sithCog_g_pSymbolTable = NULL;
    }

    if ( sithCog_pHashtable )
    {
        stdHashtbl_Free(sithCog_pHashtable);
        sithCog_pHashtable = NULL;
    }

    sithCogParse_FreeParseTree();
    sithCog_bCogStartup = 0;
}

int J3DAPI sithCog_Open(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld);
    SITH_ASSERTREL(sithCog_bCogStartup == 1);

    if ( sithCog_bCogOpen )
    {
        SITHLOG_ERROR("Warning: System already open!\n");
        return 1;
    }

    SITHLOG_STATUS("Global symbol space %d, used %d.\n", SITHCOG_MAXGLOBALSYMBOLS, sithCog_g_pSymbolTable->numUsedSymbols);

    if ( sithWorld_g_pStaticWorld )
    {
        for ( size_t cogNum = 0; cogNum < sithWorld_g_pStaticWorld->numCogs; ++cogNum )
        {
            SithCog* pCog = &sithWorld_g_pStaticWorld->aCogs[cogNum];
            for ( size_t refNum = 0; refNum < pCog->pScript->numSymbolRefs; ++refNum )
            {
                SithCogSymbolRef* pRef = &pCog->pScript->aSymRefs[refNum];

                int symbolId = pRef->symbolId;
                SITH_ASSERTREL(symbolId >= 0);

                if ( strlen(pRef->aValue) )
                {
                    if ( sithCog_ParseSymbolRef(sithWorld_g_pStaticWorld, &pCog->pSymbolTable->aSymbols[symbolId], pRef, pRef->aValue) )
                    {
                        SITHLOG_ERROR("Cog %s, Invalid Reference.\n", pCog->aName);
                    }
                }
            }

            sithCog_SendMessage(pCog, SITHCOG_MSG_LOADING, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }

    for ( size_t cogNum = 0; cogNum < pWorld->numCogs; ++cogNum )
    {
        SithCog* pCog = &pWorld->aCogs[cogNum];
        size_t cogRefNum = 0;
        for ( size_t refNum = 0; refNum < pCog->pScript->numSymbolRefs; ++refNum )
        {
            SithCogSymbolRef* pRef = &pCog->pScript->aSymRefs[refNum];

            int symbolId = pRef->symbolId;
            SITH_ASSERTREL(symbolId >= 0);

            SithCogSymbol* pSymbol = &pCog->pSymbolTable->aSymbols[symbolId];
            if ( (pRef->bLocal & 1) != 0 ) // local symbol
            {
                if ( strlen(pRef->aValue) && sithCog_ParseSymbolRef(pWorld, pSymbol, pRef, pRef->aValue) )
                {
                    SITHLOG_ERROR("Cog %s: Invalid Reference %s.\n", pCog->aName, pRef->aValue);
                }
            }
            else
            {
                if ( strlen(pCog->aSymRefValues[cogRefNum]) )
                {
                    if ( sithCog_ParseSymbolRef(pWorld, pSymbol, pRef, pCog->aSymRefValues[cogRefNum]) )
                    {
                        SITHLOG_ERROR("Cog %s: Invalid Reference  %s.\n", pCog->aName, pRef->aValue);
                    }
                }

                else if ( strlen(pRef->aValue) && sithCog_ParseSymbolRef(pWorld, pSymbol, pRef, pRef->aValue) )
                {
                    SITHLOG_ERROR("Cog %s: Invalid Reference  %s.\n", pCog->aName, pRef->aValue);
                }

                ++cogRefNum;
                sithCog_LinkCog(pWorld, pCog, pRef, pSymbol);
            }
        }

        sithCog_SendMessage(pCog, SITHCOG_MSG_LOADING, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
    }

    sithCog_bCogOpen = true;
    return 0;
}

void sithCog_Close(void)
{
    if ( sithCog_bCogOpen )
    {
        sithCog_BroadcastMessage(SITHCOG_MSG_SHUTDOWN, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0);
    }

    numSectorLinks = 0;
    numSurfLinks   = 0;
    numThingLinks  = 0;
    sithCog_g_pMasterCog = NULL;

    sithCog_bCogOpen = false;
}

void J3DAPI sithCog_FreeWorldCogs(SithWorld* pWorld)
{
    if ( pWorld->aCogScripts )
    {
        for ( size_t i = 0; i < pWorld->numCogScripts; ++i )
        {
            sithCog_FreeScriptEntry(&pWorld->aCogScripts[i]);
            sithCog_RemoveScript(&pWorld->aCogScripts[i]);
        }

        STDFREE(pWorld->aCogScripts);
        pWorld->aCogScripts    = NULL;
        pWorld->sizeCogScripts = 0;
        pWorld->numCogScripts  = 0;
    }

    if ( pWorld->aCogs )
    {
        for ( size_t i = 0; i < pWorld->numCogs; ++i )
        {
            sithCog_FreeEntry(&pWorld->aCogs[i]);
        }

        STDFREE(pWorld->aCogs);
        pWorld->aCogs    = NULL;
        pWorld->sizeCogs = 0;
        pWorld->numCogs  = 0;
    }
}

void J3DAPI sithCog_FreeScriptEntry(SithCogScript* pScript)
{
    SITH_ASSERTREL(pScript != NULL);

    sithCogParse_FreeSymbolTable(pScript->pSymbolTable);
    pScript->pSymbolTable = NULL;

    for ( size_t i = 0; i < pScript->numSymbolRefs; ++i )
    {
        if ( pScript->aSymRefs[i].pDescription )
        {
            STDFREE(pScript->aSymRefs[i].pDescription);
            pScript->aSymRefs[i].pDescription = NULL;
        }
    }

    if ( pScript->pCode )
    {
        STDFREE(pScript->pCode);
        pScript->pCode = NULL;
    }
}

void J3DAPI sithCog_FreeEntry(SithCog* pCog)
{
    sithCogParse_FreeSymbolTable(pCog->pSymbolTable);
    pCog->pSymbolTable = NULL;

    if ( pCog->aHeap )
    {
        STDFREE(pCog->aHeap);
        pCog->heapSize = 0;
    }
}

void J3DAPI sithCog_UpdateThingTimer(SithThing* pThing)
{
    if ( (pThing->flags & SITH_TF_PULSESET) != 0 && pThing->msecNextPulseTime <= sithTime_g_msecGameTime )
    {
        pThing->msecNextPulseTime = pThing->msecPulseInterval + sithTime_g_msecGameTime;
        sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_PULSE);
    }

    if ( (pThing->flags & SITH_TF_TIMERSET) != 0 && pThing->msecTimerTime <= sithTime_g_msecGameTime )
    {
        pThing->flags &= ~SITH_TF_TIMERSET;
        sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_TIMER);
    }
}

void J3DAPI sithCog_BroadcastMessage(SithCogMsgType msgType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx)
{
    sithCog_BroadcastMessageEx(msgType, senderType, senderIdx, srcType, srcIdx, 0, 0, 0, 0);
}

void J3DAPI sithCog_BroadcastMessageEx(SithCogMsgType msgType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int param0, int param1, int param2, int param3)
{
    if ( sithWorld_g_pStaticWorld )
    {
        SithCog* pCog = sithWorld_g_pStaticWorld->aCogs;
        for ( size_t i = 0; i < sithWorld_g_pStaticWorld->numCogs; ++i )
        {
            sithCog_SendMessageEx(pCog++, msgType, senderType, senderIdx, srcType, srcIdx, 0, param0, param1, param2, param3);
        }
    }

    if ( sithWorld_g_pCurrentWorld )
    {
        SithCog* pCog = sithWorld_g_pCurrentWorld->aCogs;
        for ( size_t i = 0; i < sithWorld_g_pCurrentWorld->numCogs; ++i )
        {
            sithCog_SendMessageEx(pCog++, msgType, senderType, senderIdx, srcType, srcIdx, 0, param0, param1, param2, param3);
        }
    }
}

void J3DAPI sithCog_SendMessage(SithCog* pCog, SithCogMsgType messageType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int linkId)
{
    if ( !pCog )
    {
        return;
    }

    SithCogScript* pScript = pCog->pScript;
    SITH_ASSERTREL(pScript != NULL);
    SITH_ASSERTREL(messageType > 0);

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        SITHCONSOLE_PRINTF(
            "Cog %s: Message %d delivered, senderType=%d, senderIndex=%d, sourceType=%d, sourceIndex=%d, linkId=%d.\n",
            pCog->aName,
            messageType,
            senderType,
            senderIdx,
            srcType,
            srcIdx,
            linkId
        );
    }

    if ( (pCog->flags & SITHCOG_DISABLED) != 0 )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            SITHCONSOLE_PRINTF("Cog %s: Disabled, message ignored.\n", pCog->aName);
        }
        return;
    }

    // Find message handler
    size_t  handlerNum;
    for ( handlerNum = 0; handlerNum < pScript->numHandlers && messageType != pScript->aHandlers[handlerNum].type; ++handlerNum )
    {
        ;
    }

    if ( handlerNum == pScript->numHandlers )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            SITHCONSOLE_PRINTF("--Cog %s: Message %d received but ignored.  No handler.\n", pCog->aName, messageType);
        }
        return;
    }

    if ( (pCog->flags & SITHCOG_PAUSED) != 0 )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            SITHCONSOLE_PRINTF("--Cog %s: Message %d received but COG is paused.\n", pCog->aName, messageType);
        }
        return;
    }

    if ( messageType != SITHCOG_MSG_STARTUP
        && messageType != SITHCOG_MSG_SHUTDOWN
        && stdComm_IsGameActive()
        && !stdComm_IsGameHost()
        && (pCog->flags & SITHCOG_LOCAL) == 0 )
    {
        // Send message over net
        if ( messageType != SITHCOG_MSG_PULSE && messageType != SITHCOG_MSG_TIMER )
        {
            sithDSSCog_SendMessage(pCog, messageType, senderType, senderIdx, srcType, srcIdx, linkId, 0, 0, 0, 0, sithMulti_g_serverId);
        }
    }
    else
    {
        // Execute message locally
        pCog->linkId     = linkId;
        pCog->senderIdx  = senderIdx;
        pCog->senderType = senderType;
        pCog->sourceIdx  = srcIdx;
        pCog->sourceType = srcType;
        pCog->params[0]  = 0;
        pCog->params[1]  = 0;
        pCog->params[2]  = 0;
        pCog->params[3]  = 0;

        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            SITHCONSOLE_PRINTF("--Cog %s: Message %d received and accepted for execution.\n", pCog->aName, messageType);
        }

        sithCogExec_ExecuteMessage(pCog, handlerNum);
    }
}

int J3DAPI sithCog_SendMessageEx(SithCog* pCog, SithCogMsgType messageType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int linkId, int param0, int param1, int param2, int param3)
{
    SITH_ASSERTREL(pCog != NULL);

    SithCogScript* pScript = pCog->pScript;
    SITH_ASSERTREL(pScript != NULL);

    SITH_ASSERTREL(messageType > 0);

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        SITHCONSOLE_PRINTF(
            "Cog %s: MessageEx %d delivered, senderType=%d, senderIndex=%d, sourceType=%d, sourceIndex=%d, linkId=%d, param0=%d, param1=%d, param2=%d, param3=%d.\n",
            pCog->aName,
            messageType,
            senderType,
            senderIdx,
            srcType,
            srcIdx,
            linkId,
            param0,
            param1,
            param2,
            param3
        );
    }

    if ( (pCog->flags & SITHCOG_DISABLED) != 0 )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) == 0 )
        {
            return SITHCOG_NORETURNVALUE;
        }

        SITHCONSOLE_PRINTF("Cog %s: Disabled, MessageEx ignored.\n", pCog->aName);
        return SITHCOG_NORETURNVALUE;
    }

    // Find message handler
    size_t handlerNum;
    for ( handlerNum = 0; handlerNum < pScript->numHandlers && messageType != pScript->aHandlers[handlerNum].type; ++handlerNum )
    {
        ;
    }

    if ( handlerNum == pScript->numHandlers )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) == 0 )
        {
            return SITHCOG_NORETURNVALUE;
        }

        SITHCONSOLE_PRINTF("--Cog %s: MessageEx %d received but ignored.  No handler.\n", pCog->aName, messageType);
        return SITHCOG_NORETURNVALUE;
    }

    if ( (pCog->flags & SITHCOG_PAUSED) != 0 )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) == 0 )
        {
            return SITHCOG_NORETURNVALUE;
        }

        SITHCONSOLE_PRINTF("--Cog %s: MessageEx %d received but COG is paused.\n", pCog->aName, messageType);
        return SITHCOG_NORETURNVALUE;
    }

    if ( messageType != SITHCOG_MSG_STARTUP
        && messageType != SITHCOG_MSG_SHUTDOWN
        && stdComm_IsGameActive()
        && !stdComm_IsGameHost()
        && (pCog->flags & SITHCOG_LOCAL) == 0 )
    {
        // Send over net
        if ( messageType == SITHCOG_MSG_PULSE || messageType == SITHCOG_MSG_TIMER )
        {
            return 0;
        }
        else
        {
            sithDSSCog_SendMessage(pCog, messageType, senderType, senderIdx, srcType, srcIdx, linkId, param0, param1, param2, param3, sithMulti_g_serverId);
            return 0;
        }
    }
    else
    {
        // Execute locally
        pCog->linkId      = linkId;
        pCog->senderIdx   = senderIdx;
        pCog->senderType  = senderType;
        pCog->sourceIdx   = srcIdx;
        pCog->sourceType  = srcType;
        pCog->params[0]   = param0;
        pCog->params[1]   = param1;
        pCog->params[2]   = param2;
        pCog->params[3]   = param3;
        pCog->returnValue = SITHCOG_NORETURNVALUE;

        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            SITHCONSOLE_PRINTF("--Cog %s: MessageEx %d received and accepted for execution.\n", pCog->aName, messageType);
        }

        sithCogExec_ExecuteMessage(pCog, handlerNum);
        return pCog->returnValue;
    }
}

int J3DAPI sithCog_AllocWorldCogScripts(SithWorld* pWorld, size_t numCogScripts)
{
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(!pWorld->aCogScripts);

    pWorld->aCogScripts = (SithCogScript*)STDMALLOC(sizeof(SithCogScript) * numCogScripts);
    if ( !pWorld->aCogScripts )
    {
        SITHLOG_ERROR("Memory alloc failure initializing COG scripts.\n");
        return 1;
    }

    STD_ZEROMEM(pWorld->aCogScripts, sizeof(SithCogScript) * numCogScripts);
    pWorld->sizeCogScripts = numCogScripts;
    pWorld->numCogScripts  = 0;
    return 0;
}

int J3DAPI sithCog_AllocWorldCogs(SithWorld* pWorld, size_t sizeCogs)
{
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(!pWorld->aCogs);
    pWorld->aCogs = (SithCog*)STDMALLOC(sizeof(SithCog) * sizeCogs);
    if ( !pWorld->aCogs )
    {
        SITHLOG_ERROR("Memory alloc failure initializing COGs.\n");
        return 1;
    }

    STD_ZEROMEM(pWorld->aCogs, sizeof(SithCog) * sizeCogs);
    pWorld->sizeCogs = sizeCogs;
    pWorld->numCogs  = 0;
    return 0;
}

int J3DAPI sithCog_WriteCogsListText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("######### COG placement ########\n")
        || stdConffile_WriteLine("Section: cogs\n")
        || stdConffile_Printf("World cogs %d\n", pWorld->numCogs)
        || stdConffile_WriteLine("#Num\tScript          Symbol values\n") )
    {
        return 1;
    }

    size_t cogNum = 0;
    for ( size_t i = 0; i < pWorld->numCogs; ++i )
    {
        SithCog* pCog = &pWorld->aCogs[i];
        if ( (pCog->flags & SITHCOG_CLASS) == 0 )
        {
            stdConffile_Printf("%d:\t%s\t", cogNum++, pCog->pScript->aName);
            for ( size_t symNum = 0; symNum < pCog->pScript->numSymbolRefs; ++symNum )
            {
                if ( (pCog->pScript->aSymRefs[symNum].bLocal & 1) == 0 )
                {
                    char aValue[SITHCOG_SYMVALUESTRLEN];
                    sithCog_GetSymbolRefInitializer(pWorld, pCog, symNum, aValue);
                    if ( stdConffile_Printf("%s ", aValue) )
                    {
                        return 1;
                    }
                }
            }

            if ( stdConffile_WriteLine("\n") )
            {
                return 1;
            }
        }
    }

    return stdConffile_WriteLine("\nend\n") || stdConffile_WriteLine("################################\n\n\n");
}

void J3DAPI sithCog_GetSymbolRefInitializer(const SithWorld* pWorld, const SithCog* pCog, int symIdx, char* pOutString)
{
    SithCogSymbol* pSymbol = &pCog->pSymbolTable->aSymbols[pCog->pScript->aSymRefs[symIdx].symbolId];
    int objectIdx = pSymbol->value.val.intValue;
    int refIdx    = objectIdx;

    switch ( pCog->pScript->aSymRefs[symIdx].type )
    {
        case SITHCOG_SYM_REF_FLEX:
            stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%f", pSymbol->value.val.floatValue);
            break;

        case SITHCOG_SYM_REF_VECTOR:
            stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "(%f/%f/%f)", pSymbol->value.val.floatValue, pSymbol->value.val.vecValue.y, pSymbol->value.val.vecValue.z);
            break;

        case SITHCOG_SYM_REF_TEMPLATE:
            if ( (unsigned int)objectIdx < pWorld->numThingTemplates )
            {
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pWorld->aThingTemplates[objectIdx].aName);
            }
            else
            {
                objectIdx = 0;
                SITHLOG_ERROR("Invalid templates initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName);
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pSymbol->pName);
            }

            break;

        case SITHCOG_SYM_REF_KEYFRAME:
            if ( (size_t)objectIdx < pWorld->numKeyframes )
            {
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pWorld->aKeyframes[objectIdx].aName);
            }
            else
            {
                objectIdx = 0;
                SITHLOG_ERROR("Invalid keyframe initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName);
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pSymbol->pName);
            }

            break;

        case SITHCOG_SYM_REF_SOUND:
        {
            size_t numSounds;
            if ( (size_t)objectIdx > pWorld->numSounds )
            {
                numSounds = pWorld->numSounds;
            }
            else
            {
                numSounds = pSymbol->value.val.intValue;
            }

            objectIdx = numSounds;
            if ( numSounds < pWorld->numSounds )
            {
                tSoundHandle hSnd = Sound_GetSoundHandle(numSounds);
                const char* pFilename = Sound_GetSoundFilename(hSnd);
                if ( pFilename )
                {
                    stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pFilename);
                }
                else
                {
                    SITHLOG_ERROR("Unable to load sound initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName);
                    stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pSymbol->pName);
                }
            }
            else
            {
                objectIdx = 0;
                SITHLOG_ERROR("Invalid sound initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName);
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pSymbol->pName);
            }
        } break;

        case SITHCOG_SYM_REF_COG:
        {
            int numCogs;
            if ( objectIdx < -1 )
            {
                numCogs = -1;
            }
            else if ( (size_t)objectIdx > pWorld->numCogs )
            {
                numCogs = pWorld->numCogs;
            }
            else
            {
                numCogs = pSymbol->value.val.intValue;
            }

            objectIdx = numCogs;
            if ( numCogs >= 0 )
            {
                size_t cogNum = 0;
                for ( size_t i = 0; i < (size_t)numCogs; ++i )
                {
                    if ( (pWorld->aCogs[i].flags & SITHCOG_CLASS) == 0 )
                    {
                        ++cogNum;
                    }
                }

                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%d", cogNum);
            }
            else
            {
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%d", numCogs);
            }
        } break;

        case SITHCOG_SYM_REF_MATERIAL:
            if ( (size_t)objectIdx < pWorld->numMaterials && pWorld->aMaterials[objectIdx].aName[0] )
            {
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pWorld->aMaterials[objectIdx].aName);
            }
            else
            {
                objectIdx = 0;
                SITHLOG_ERROR("Invalid material initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName);
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pSymbol->pName);
            }
            break;

        case SITHCOG_SYM_REF_MODEL:
            if ( (size_t)objectIdx < pWorld->numModels )
            {
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pWorld->aModels[objectIdx].aName);
            }
            else
            {
                objectIdx = 0;
                SITHLOG_ERROR("Invalid AI Class initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName);
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pSymbol->pName);
            }
            break;

        case SITHCOG_SYM_REF_AICLASS:
            if ( (size_t)objectIdx < pWorld->numAIClasses )
            {
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pWorld->aAIClasses[objectIdx].aName);
            }
            else
            {
                objectIdx = 0;
                SITHLOG_ERROR("Invalid AI Class initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName);
                stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%s", pSymbol->pName);
            }

            break;

        default:
            stdUtil_Format(pOutString, SITHCOG_SYMVALUESTRLEN, "%d", objectIdx);
            break;
    }

    if ( objectIdx != refIdx )
    {
        SITHLOG_ERROR("Invalid Reference '%s' in COG '%s'", pCog->aName, pSymbol->pName); // Fixed typo
    }
}

int J3DAPI sithCog_ReadCogsListText(SithWorld* pWorld, int bSkip)
{
    if ( bSkip )
    {
        return 1;
    }

    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aCogs == NULL);

    stdConffile_ReadArgs();
    if ( !streq(stdConffile_g_entry.aArgs[0].argValue, "world") || !streq(stdConffile_g_entry.aArgs[1].argValue, "cogs") )
    {
        goto syntax_error;
    }

    size_t sizeCogs = atoi(stdConffile_g_entry.aArgs[2].argValue);
    if ( !sizeCogs )
    {
        return 0;
    }

    if ( sithCog_AllocWorldCogs(pWorld, sizeCogs) )
    {
        SITHLOG_ERROR("No memory to allocate COG symbol table, line %d.\n", stdConffile_GetLineNumber()); // Fixed: Fix typo
        return 1;
    }

    while ( stdConffile_ReadArgs() && !streq(stdConffile_g_entry.aArgs[0].argValue, "end") )
    {
        if ( stdConffile_g_entry.numArgs < 2u )
        {
            goto syntax_error;
        }

        size_t argNum = 2;
        SithCog* pCog = sithCog_Load(pWorld, stdConffile_g_entry.aArgs[1].argValue);
        if ( !pCog )
        {
            SITHLOG_ERROR("Placement of Cog %s failed.\n", stdConffile_g_entry.aArgs[1].argValue);
            continue;
        }

        SithCogScript* pScript = pCog->pScript;
        size_t curValIdx = 0;
        for ( size_t i = 0; i < pScript->numSymbolRefs; ++i )
        {
            if ( (pScript->aSymRefs[i].bLocal & 1) == 0 )
            {
                if ( stdConffile_g_entry.numArgs <= argNum )
                {
                    SITHLOG_STATUS("Cog %s: Not enough initializers to init referece #%d, line %d.\n", pCog->aName, i, stdConffile_GetLineNumber());
                    continue;
                }

                SITH_ASSERTREL(stdConffile_g_entry.aArgs[argNum].argValue);
                STD_STRCPY(pCog->aSymRefValues[curValIdx], stdConffile_g_entry.aArgs[argNum].argValue);

                ++argNum;
                ++curValIdx;
            }
        }
    }

    // Success
    return 0;

syntax_error:
    SITHLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, stdConffile_GetFilename());
    return 1;
}

int J3DAPI sithCog_WriteCogsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    uint32_t aSizes[2] = { 0 }; // leave uint32_t type
    for ( size_t i = 0; i < pWorld->numCogs; ++i )
    {
        SithCog* pCog = &pWorld->aCogs[i];
        if ( (pCog->flags & SITHCOG_CLASS) == 0 )// count only non-class cogs
        {
            ++aSizes[0]; // num cogs
            for ( size_t symIdx = 0; symIdx < pCog->pScript->numSymbolRefs; ++symIdx )
            {
                if ( (pCog->pScript->aSymRefs[symIdx].bLocal & 1) == 0 )
                {
                    ++aSizes[1]; // num values to write
                }
            }
        }
    }

    if ( sith_g_pHS->pFileWrite(fh, aSizes, sizeof(aSizes)) != sizeof(aSizes) )
    {
        static_assert(sizeof(aSizes) == 8, "sizeof((aSizes) == 8");
        return 1;
    }

    if ( !aSizes[0] )
    {
        return 0;
    }

    char (*aValues)[SITHCOG_SYMVALUESTRLEN] = NULL;

    uint32_t sizeNames = aSizes[0] * 64;
    char (*aNames)[64] = (char (*)[64])STDMALLOC(sizeNames);
    if ( !aNames )
    {
        goto error;
    }

    size_t sizeValues = aSizes[1] * SITHCOG_SYMVALUESTRLEN;
    aValues = (char (*)[SITHCOG_SYMVALUESTRLEN])STDMALLOC(sizeValues);
    if ( !aValues )
    {
        goto error;
    }

    char (*pName)[64] = aNames;
    char (*pValue)[SITHCOG_SYMVALUESTRLEN] = aValues;

    for ( size_t i = 0; i < pWorld->numCogs; ++i )
    {
        SithCog* pCog  = &pWorld->aCogs[i];
        if ( (pCog->flags & SITHCOG_CLASS) == 0 )
        {
            STD_STRCPY(*pName, pCog->aName);
            pName++;
            static_assert(STD_ARRAYLEN(*pName) == 64, "(STD_ARRAYLEN(*pName) == 64");

            for ( size_t symIdx = 0; symIdx < pCog->pScript->numSymbolRefs; ++symIdx )
            {
                if ( (pCog->pScript->aSymRefs[symIdx].bLocal & 1) == 0 )
                {
                    sithCog_GetSymbolRefInitializer(pWorld, pCog, symIdx, *pValue++);
                    static_assert(STD_ARRAYLEN(*pValue) == 64, "(STD_ARRAYLEN(*pValue) == 64");
                }
            }
        }
    }

    if ( (sith_g_pHS->pFileWrite(fh, aNames, sizeNames) == sizeNames)
        && (sith_g_pHS->pFileWrite(fh, aValues, sizeValues) == sizeValues) )
    {
        STDFREE(aNames);
        STDFREE(aValues);
        return 0;
    }

error:
    if ( aNames )
    {
        STDFREE(aNames);
    }

    if ( aValues )
    {
        STDFREE(aValues);
    }

    return 1;
}

int J3DAPI sithCog_ReadCogsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    char (*aNames)[64] = NULL;
    char (*aValues)[SITHCOG_SYMVALUESTRLEN] = NULL;

    uint32_t aSizes[2];
    if ( sith_g_pHS->pFileRead(fh, aSizes, sizeof(aSizes)) != sizeof(aSizes) || sithCog_AllocWorldCogs(pWorld, pWorld->sizeCogs) ) // Altered: Use pWorld->sizeCogs value instead of pWorld->numCogs for cogs allocation
    {
        static_assert(sizeof(aSizes) == 8, "sizeof((aSizes) == 8");
        goto error;
    }

    if ( !aSizes[0] )
    {
        return 0;
    }

    size_t sizeCogs = aSizes[0] * 64;
    aNames = (char (*)[64])STDMALLOC(sizeCogs);
    if ( !aNames )
    {
        goto error;
    }

    size_t sizeValues = aSizes[1] * SITHCOG_SYMVALUESTRLEN;
    aValues = (char (*)[SITHCOG_SYMVALUESTRLEN])STDMALLOC(sizeValues);
    if ( !aValues )
    {
        goto error;
    }

    if ( (sith_g_pHS->pFileRead(fh, aNames, sizeCogs) == sizeCogs)
        && (sith_g_pHS->pFileRead(fh, aValues, sizeValues) == sizeValues) )
    {
        const char (*pCurValue)[SITHCOG_SYMVALUESTRLEN] = aValues;
        for ( size_t i = 0; i < aSizes[0]; ++i )
        {
            // Load cog
            const char* pName = aNames[i];
            SithCog* pCog = sithCog_Load(pWorld, pName);
            if ( !pCog )
            {
                goto error;
            }

            // Parse cog's var values
            size_t valIdx = 0;
            for ( size_t j = 0; j < pCog->pScript->numSymbolRefs; ++j )
            {
                if ( (pCog->pScript->aSymRefs[j].bLocal & 1) == 0 ) // not local symbol
                {
                    STD_STRCPY(pCog->aSymRefValues[valIdx++], *pCurValue);
                    ++pCurValue;
                }
            }
        }

        STDFREE(aNames);
        STDFREE(aValues);
        return 0;
    }

error:
    SITHLOG_ERROR("Error Reading COGs\n");
    if ( aNames )
    {
        STDFREE(aNames);
    }

    if ( aValues )
    {
        STDFREE(aValues);
    }

    return 1;
}

int J3DAPI sithCog_ParseSymbolRef(SithWorld* pWorld, SithCogSymbol* pSymbol, const SithCogSymbolRef* pRef, const char* pString)
{
    SITH_ASSERTREL(pSymbol && pRef && pString);

    switch ( pRef->type )
    {
        case SITHCOG_SYM_REF_FLEX:
            pSymbol->value.type = SITHCOG_VALUE_FLOAT;
            pSymbol->value.val.floatValue = strtof(pString, NULL);
            return 0;

        case SITHCOG_SYM_REF_VECTOR:
        {
            pSymbol->value.type = SITHCOG_VALUE_VECTOR;
            float x, y, z;
            if ( sscanf_s(pString, "(%f/%f/%f)", &x, &y, &z) != 3 )
            {
                SITHLOG_ERROR("Ref '%s' could not be read as a vector, line %d.\n", pString, stdConffile_GetLineNumber());
                STD_ZEROMEM(&pSymbol->value.val, sizeof(pSymbol->value.val));
                return 1;
            }

            pSymbol->value.val.vecValue.x = x;
            pSymbol->value.val.vecValue.y = y;
            pSymbol->value.val.vecValue.z = z;
            return 0;
        }

        case SITHCOG_SYM_REF_MATERIAL:
        {
            pSymbol->value.type = SITHCOG_VALUE_INT;
            rdMaterial* pMat = sithMaterial_Load(pString);
            if ( !pMat )
            {
                SITHLOG_ERROR("MaterialRef '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber());
                pSymbol->value.val.intValue = -1;
                return 1;
            }

            pSymbol->value.val.intValue = pMat->num;
            return 0;
        }

        case SITHCOG_SYM_REF_SOUND:
        {
            pSymbol->value.type = SITHCOG_VALUE_INT;
            tSoundHandle hSnd = sithSound_Load(pWorld, pString);
            if ( hSnd == 0 )
            {
                pSymbol->value.val.intValue = -1;
                SITHLOG_ERROR("SoundRef '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber());
                return 1;
            }

            pSymbol->value.val.intValue = Sound_GetSoundIndex(hSnd);
            return 0;
        }

        case SITHCOG_SYM_REF_TEMPLATE:
        {
            pSymbol->value.type = SITHCOG_VALUE_INT;
            SithThing* pTemplate = sithTemplate_GetTemplate(pString);
            if ( !pTemplate )
            {
                pSymbol->value.val.intValue = -1;
                SITHLOG_ERROR("TemplateRef '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber());
                return 1;
            }

            pSymbol->value.val.intValue = pTemplate->idx;
            return 0;
        }

        case SITHCOG_SYM_REF_MODEL:
        {
            pSymbol->value.type = SITHCOG_VALUE_INT;
            rdModel3* pModel = sithModel_Load(pString, 1);
            if ( !pModel )
            {
                pSymbol->value.val.intValue = -1;
                SITHLOG_ERROR("ModelRef '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber());
                return 1;
            }

            pSymbol->value.val.intValue = pModel->num;
            return 0;
        }

        case SITHCOG_SYM_REF_KEYFRAME:
        {
            pSymbol->value.type = SITHCOG_VALUE_INT;
            rdKeyframe* pKeyframe = sithPuppet_LoadKeyframe(pString);
            if ( !pKeyframe )
            {
                pSymbol->value.val.intValue = -1;
                SITHLOG_ERROR("KeyframeRef '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber());
                return 1;
            }

            pSymbol->value.val.intValue = pKeyframe->idx;
            return 0;
        }

        case SITHCOG_SYM_REF_AICLASS:
        {
            pSymbol->value.type = SITHCOG_VALUE_INT;
            SithAIClass* pAIClass = sithAIClass_Load(pWorld, pString);
            if ( !pAIClass )
            {
                pSymbol->value.val.intValue = -1;
                SITHLOG_ERROR("AIClass '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber());
                return 1;
            }

            pSymbol->value.val.intValue = pAIClass->num;
            return 0;
        }

        default:
            pSymbol->value.type = SITHCOG_VALUE_INT;
            pSymbol->value.val.intValue = atoi(pString);
            return 0;
    }
}

int J3DAPI sithCog_LinkCog(const SithWorld* pWorld, SithCog* pCog, const SithCogSymbolRef* pRef, const SithCogSymbol* pSymbol)
{
    SITH_ASSERTREL(pCog && pRef && pSymbol);

    int index = pSymbol->value.val.intValue;
    if ( index < 0 )
    {
        return 1;
    }

    switch ( pRef->type )
    {
        case SITHCOG_SYM_REF_THING:
        {
            if ( (size_t)index < pWorld->numThings )
            {
                return sithCog_LinkCogToThing(pWorld, pCog, &pWorld->aThings[index], pRef->linkId, pRef->mask);
            }
            else
            {
                SITHLOG_ERROR("Could not link Cog %s to thing %d, out of range.\n", pCog->aName, index);
                return 1;
            }
        }

        case SITHCOG_SYM_REF_SECTOR:
        {
            if ( (size_t)index < pWorld->numSectors )
            {
                return sithCog_LinkCogToSector(pWorld, pCog, &pWorld->aSectors[index], pRef->linkId, pRef->mask);
            }
            else
            {
                SITHLOG_ERROR("Could not link Cog %s to sector %d, out of range.\n", pCog->aName, index);
                return 1;
            }
        }

        case SITHCOG_SYM_REF_SURFACE:
        {
            if ( (size_t)index < pWorld->numSurfaces )
            {
                return sithCog_LinkCogToSurface(pCog, &pWorld->aSurfaces[index], pRef->linkId, pRef->mask);
            }
            else
            {
                SITHLOG_ERROR("Could not link Cog %s to surface %d, out of range.\n", pCog->aName, index);
                return 1;
            }
        }
        default:
            return 0;
    }
}

SithCog* J3DAPI sithCog_Load(SithWorld* pWorld, const char* pName)
{
    SITH_ASSERTREL(pWorld && pName);

    if ( pWorld->numCogs >= pWorld->sizeCogs )
    {
        SITHLOG_ERROR("No space to load Cog %s.\n", pName);
        return NULL;
    }

    SithCog* pCog = &pWorld->aCogs[pWorld->numCogs];
    pCog->idx = pWorld->numCogs;
    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        pCog->idx = SITHWORLD_STATICINDEX(pCog->idx);
    }

    SithCogScript* pScript = sithCog_LoadScript(pWorld, pName);
    if ( !pScript )
    {
        SITHLOG_ERROR("Load of cog script %s failed.\n", pName);
        return NULL;
    }

    STD_STRCPY(pCog->aName, pScript->aName);
    pCog->pScript = pScript;
    pCog->flags   = pScript->flags;

    pCog->pSymbolTable = sithCogParse_DuplicateSymbolTable(pScript->pSymbolTable);
    if ( !pCog->pSymbolTable )
    {
        SITHLOG_ERROR("Could not allocate symbol table for cog %s.\n", pCog->aName);
        return NULL;
    }

    // Success
    ++pWorld->numCogs;
    return pCog;
}

SithCog* J3DAPI sithCog_GetCogByIndex(int idx)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(idx) )
    {
        idx    = SITHWORLD_FROM_STATICINDEX(idx);
        pWorld = sithWorld_g_pStaticWorld;
    }

    if ( pWorld && idx >= 0 && idx < (signed int)pWorld->numCogs )
    {
        return &pWorld->aCogs[idx];
    }

    return NULL;
}

SithCog* J3DAPI sithCog_GetCog(const char* pName)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( !pName )
    {
        return NULL;
    }

    for ( size_t i = 0; i < 2; ++i )
    {
        if ( i == 1 )
        {
            pWorld = sithWorld_g_pStaticWorld;
        }

        if ( pWorld )
        {
            for ( size_t j = 0; j < pWorld->numCogs; ++j )
            {
                if ( streqi(pName, pWorld->aCogs[j].aName) )
                {
                    return &pWorld->aCogs[j];
                }
            }
        }
    }

    return NULL;
}

int J3DAPI sithCog_LinkCogToThing(const SithWorld* pWorld, SithCog* pCog, SithThing* pThing, int linkId, int mask)
{
    SITH_ASSERTREL(pCog && pThing);

    if ( !sithThing_ValidateThingPointer(pWorld, pThing) || pThing->type == SITH_THING_FREE )
    {
        return 1;
    }

    if ( linkId < 0 ) // not linked
    {
        return 0;
    }

    // TODO: maybe it should be handled some other way than crashing the engine
    SITH_ASSERTREL(numThingLinks < STD_ARRAYLEN(aThingLinks));

    pThing->flags |= SITH_TF_COGLINKED;
    aThingLinks[numThingLinks].pThing = pThing;
    aThingLinks[numThingLinks].pCog   = pCog;
    aThingLinks[numThingLinks].linkId = linkId;
    aThingLinks[numThingLinks].mask   = mask;
    aThingLinks[numThingLinks++].thingSignature = pThing->signature;
    return 0;
}

int J3DAPI sithCog_LinkCogToSurface(SithCog* pCog, SithSurface* pSurface, int linkId, int mask)
{
    SITH_ASSERTREL(pCog && pSurface);

    if ( !sithSurface_ValidateSurfacePointer(pSurface) )
    {
        return 1;
    }

    if ( linkId < 0 ) // nolink
    {
        return 0;
    }

    // TODO: maybe it should be handled some other way than crashing the engine
    SITH_ASSERTREL(numSurfLinks < STD_ARRAYLEN(aCogLinkSurfaces));

    pSurface->flags |= SITH_SURFACE_COGLINKED;
    aCogLinkSurfaces[numSurfLinks].pSurface = pSurface;
    aCogLinkSurfaces[numSurfLinks].pCog     = pCog;
    aCogLinkSurfaces[numSurfLinks].linkId   = linkId;
    aCogLinkSurfaces[numSurfLinks++].mask   = mask;
    return 0;
}

int J3DAPI sithCog_LinkCogToSector(const SithWorld* pWorld, SithCog* pCog, SithSector* pSector, int linkId, int mask)
{
    SITH_ASSERTREL(pCog && pSector);

    if ( !sithSector_ValidateSectorPointer(pWorld, pSector) )
    {
        return 1;
    }

    if ( linkId < 0 ) // nolink
    {
        return 0;
    }

    // TODO: maybe it should be handled some other way than crashing the engine
    SITH_ASSERTREL(numSectorLinks < STD_ARRAYLEN(aCogLinkSectors));

    pSector->flags |= SITH_SECTOR_COGLINKED;
    aCogLinkSectors[numSectorLinks].pSector = pSector;
    aCogLinkSectors[numSectorLinks].pCog    = pCog;
    aCogLinkSectors[numSectorLinks].linkId  = linkId;
    aCogLinkSectors[numSectorLinks++].mask  = mask;
    return 0;
}

void sithCog_FixupLinksToThings(void)
{
    for ( size_t i = 0; i < numThingLinks; ++i )
    {
        aThingLinks[i].thingSignature = aThingLinks[i].pThing->signature;
    }
}

int J3DAPI sithCog_ThingSendMessage(const SithThing* pThing, const SithThing* pSrcThing, SithCogMsgType msgType)
{
    return sithCog_ThingSendMessageEx(pThing, pSrcThing, msgType, 0, 0, 0, 0);
}

int J3DAPI sithCog_ThingSendMessageEx(const SithThing* pThing, const SithThing* pSrcThing, SithCogMsgType messageType, int param0, int param1, int param2, int param3)
{
    SITH_ASSERTREL(pThing);

    int retVal = 0;
    if ( messageType == SITHCOG_MSG_DAMAGED )
    {
        retVal = param0;
    }

    SithCogSymbolRefType srcType = SITHCOG_SYM_REF_NONE;
    int srcIdx = -1;
    int mask   = SITHTHING_TYPEMASK(SITH_THING_FREE);
    if ( pSrcThing )
    {
        srcIdx  = pSrcThing->idx;
        srcType = SITHCOG_SYM_REF_THING;
        mask    = SITHTHING_TYPEMASK(pSrcThing->type);
    }

    // 1. Send the message to zhing COG first
    if ( pThing->pCog )
    {
        if ( messageType == SITHCOG_MSG_DAMAGED )
        {
            int res = sithCog_SendMessageEx(pThing->pCog, SITHCOG_MSG_DAMAGED, SITHCOG_SYM_REF_THING, pThing->idx, srcType, srcIdx, 0, param0, param1, param2, param3);
            if ( res != SITHCOG_NORETURNVALUE )
            {
                retVal = res;
                param0 = res;
            }
        }
        else
        {
            int res = sithCog_SendMessageEx(pThing->pCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcType, srcIdx, 0, param0, param1, param2, param3);
            if ( res != SITHCOG_NORETURNVALUE )
            {
                retVal += res;
            }
        }
    }

    // 2. Send message to thing's captured COG
    if ( pThing->pCaptureCog )
    {
        if ( messageType == SITHCOG_MSG_DAMAGED )
        {
            int res = sithCog_SendMessageEx(pThing->pCaptureCog, SITHCOG_MSG_DAMAGED, SITHCOG_SYM_REF_THING, pThing->idx, srcType, srcIdx, 0, param0, param1, param2, param3);
            if ( res != SITHCOG_NORETURNVALUE )
            {
                retVal = res;
                param0 = res;
            }
        }
        else
        {
            int res = sithCog_SendMessageEx(pThing->pCaptureCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcType, srcIdx, 0, param0, param1, param2, param3);
            if ( res != SITHCOG_NORETURNVALUE )
            {
                retVal += res;
            }
        }
    }

    // 3. Send the message to all COGs that think is linked to
    for ( size_t i = 0; i < numThingLinks; i++ )
    {
        SithCogThingLink* pLink = &aThingLinks[i];
        if ( pLink->pThing == pThing && pLink->thingSignature == pThing->signature && (mask & pLink->mask) != 0 )
        {
            if ( messageType == SITHCOG_MSG_DAMAGED )
            {
                int res = sithCog_SendMessageEx(pLink->pCog, SITHCOG_MSG_DAMAGED, SITHCOG_SYM_REF_THING, pThing->idx, srcType, srcIdx, 0, param0, param1, param2, param3);
                if ( res != SITHCOG_NORETURNVALUE )
                {
                    retVal = res;
                    param0 = res;
                }
            }
            else
            {
                int res = sithCog_SendMessageEx(pLink->pCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcType, srcIdx, pLink->linkId, param0, param1, param2, param3);
                if ( res != SITHCOG_NORETURNVALUE )
                {
                    retVal += res;
                }
            }
        }
    }

    return retVal;
}

int J3DAPI sithCog_SectorSendMessage(const SithSector* pSector, const SithThing* pThing, SithCogMsgType msgType)
{
    return sithCog_SectorSendMessageEx(pSector, pThing, msgType, 0, 0, 0, 0);
}

int J3DAPI sithCog_SectorSendMessageEx(const SithSector* pSector, const SithThing* pThing, SithCogMsgType messageType, int param0, int param1, int param2, int param3)
{
    SITH_ASSERTREL(pSector);

    SithCogSymbolRefType srcType = SITHCOG_SYM_REF_NONE;
    int srcIdx = -1;
    int mask   = SITHTHING_TYPEMASK(SITH_THING_FREE);
    if ( pThing )
    {
        srcIdx  = pThing->idx;
        srcType = SITHCOG_SYM_REF_THING;
        mask    = SITHTHING_TYPEMASK(pThing->type);
    }

    int retVal = 0;
    for ( size_t i = 0; i < numSectorLinks; i++ )
    {
        SithCogSectorLink* pLink = &aCogLinkSectors[i];
        if ( pLink->pSector == pSector && (mask & pLink->mask) != 0 )
        {
            if ( messageType == SITHCOG_MSG_DAMAGED )
            {
                int secIdx = sithSector_GetSectorIndex(pSector);
                int res = sithCog_SendMessageEx(pLink->pCog, SITHCOG_MSG_DAMAGED, SITHCOG_SYM_REF_SECTOR, secIdx, srcType, srcIdx, pLink->linkId, param0, param1, param2, param3);
                if ( res == SITHCOG_NORETURNVALUE )
                {
                    retVal = param0;
                }
                else
                {
                    retVal = res;
                    param0 = res;
                }
            }
            else
            {
                int secIdx = sithSector_GetSectorIndex(pSector);
                int res = sithCog_SendMessageEx(pLink->pCog, messageType, SITHCOG_SYM_REF_SECTOR, secIdx, srcType, srcIdx, pLink->linkId, param0, param1, param2, param3);
                if ( res != SITHCOG_NORETURNVALUE )
                {
                    retVal += res;
                }
            }
        }
    }

    return retVal;
}

int J3DAPI sithCog_SurfaceSendMessage(const SithSurface* pSurf, const SithThing* pSrcThing, SithCogMsgType messageType)
{
    return sithCog_SurfaceSendMessageEx(pSurf, pSrcThing, messageType, 0, 0, 0, 0);
}

int J3DAPI sithCog_SurfaceSendMessageEx(const SithSurface* pSurf, const SithThing* pSrcThing, SithCogMsgType messageType, int param0, int param1, int param2, int param3)
{
    SITH_ASSERTREL(pSurf);

    SithCogSymbolRefType srcType = SITHCOG_SYM_REF_NONE;
    int srcIdx = -1;
    int mask   = SITHTHING_TYPEMASK(SITH_THING_FREE);
    if ( pSrcThing )
    {
        srcIdx  = pSrcThing->idx;
        srcType = SITHCOG_SYM_REF_THING;
        mask    = SITHTHING_TYPEMASK(pSrcThing->type);
    }

    int retVal = 0;
    for ( size_t i = 0; i < numSurfLinks; i++ )
    {
        SithCogSurfaceLink* pLink = &aCogLinkSurfaces[i];
        if ( pLink->pSurface == pSurf && (mask & pLink->mask) != 0 )
        {
            if ( messageType == SITHCOG_MSG_DAMAGED )
            {
                int surfIdx = sithSurface_GetSurfaceIndex(pSurf);
                int res = sithCog_SendMessageEx(pLink->pCog, SITHCOG_MSG_DAMAGED, SITHCOG_SYM_REF_SURFACE, surfIdx, srcType, srcIdx, pLink->linkId, param0, param1, param2, param3);
                if ( res == SITHCOG_NORETURNVALUE )
                {
                    retVal = param0;
                }
                else
                {
                    retVal = res;
                    param0 = res;
                }
            }
            else
            {
                int surfIdx = sithSurface_GetSurfaceIndex(pSurf);
                int res = sithCog_SendMessageEx(pLink->pCog, messageType, SITHCOG_SYM_REF_SURFACE, surfIdx, srcType, srcIdx, pLink->linkId, param0, param1, param2, param3);
                if ( res != SITHCOG_NORETURNVALUE )
                {
                    retVal += res;
                }
            }
        }
    }

    return retVal;
}

int J3DAPI sithCog_WriteCogScriptsListText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("########## COG scripts #########\n")
        || stdConffile_WriteLine("Section: cogscripts\n")
        || stdConffile_Printf("World scripts %d\n", pWorld->numCogScripts) )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numCogScripts; ++i )
    {
        if ( stdConffile_Printf("%d:\t%s\n", i, pWorld->aCogScripts[i].aName) )
        {
            return 1;
        }
    }

    return stdConffile_WriteLine("end\n") || stdConffile_WriteLine("################################\n\n\n");
}

int J3DAPI sithCog_ReadCogScriptsListText(SithWorld* pWorld, int bSkip)
{
    SITH_ASSERTREL(pWorld != NULL);

    if ( bSkip )
    {
        return 1;
    }

    stdConffile_ReadArgs();
    if ( !streq(stdConffile_g_entry.aArgs[0].argValue, "world") || !streq(stdConffile_g_entry.aArgs[1].argValue, "scripts") )
    {
        goto syntax_error;
    }

    size_t numScripts = atoi(stdConffile_g_entry.aArgs[2].argValue);
    if ( numScripts )
    {
        SITH_ASSERTREL(pWorld->aCogScripts == NULL);
        if ( sithCog_AllocWorldCogScripts(pWorld, numScripts) )
        {
            return 1;
        }

        while ( stdConffile_ReadArgs() && !streq(stdConffile_g_entry.aArgs[0].argValue, "end") )
        {
            if ( pWorld->numCogScripts >= pWorld->sizeCogScripts )
            {
                SITHLOG_ERROR("Not enough space to load COG scripts.  Script skipped.\n");
                continue; // TODO: shall we just break here, or make more specific log about which script wasn't loaded
            }

            if ( !stdConffile_g_entry.numArgs )
            {
                goto syntax_error;
            }

            if ( !sithCog_LoadScript(pWorld, stdConffile_g_entry.aArgs[1].argValue) )
            {
                SITHLOG_ERROR("Failed to load COG script %s.\n", stdConffile_g_entry.aArgs[1].argValue);
            }
        }
    }

    return 0;

syntax_error:
    SITHLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, stdConffile_GetFilename());
    return 1;
}

int J3DAPI sithCog_WriteCogScriptsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    for ( size_t i = 0; i < pWorld->numCogScripts; ++i )
    {
        if ( sith_g_pHS->pFileWrite(fh, pWorld->aCogScripts[i].aName, STD_ARRAYLEN(pWorld->aCogScripts[i].aName)) != STD_ARRAYLEN(pWorld->aCogScripts[i].aName) )
        {
            static_assert(STD_ARRAYLEN(pWorld->aCogScripts[i].aName) == 64, "STD_ARRAYLEN(pWorld->aCogScripts[i].aName) == 64"); // Just make sure the name size wasn't changed
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithCog_ReadCogScriptsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    const size_t numCogScripts = pWorld->numCogScripts; // Cache the num scripts as sithCog_AllocWorldCogScripts resets it to 0
    if ( sithCog_AllocWorldCogScripts(pWorld, pWorld->sizeCogScripts) )
    {
        return 1;
    }

    for ( size_t i = 0; i < numCogScripts; ++i )
    {
        char aStriptName[64];
        if ( sith_g_pHS->pFileRead(fh, aStriptName, STD_ARRAYLEN(aStriptName)) != STD_ARRAYLEN(aStriptName) || !sithCog_LoadScript(pWorld, aStriptName) )
        {
            return 1;
        }
    }

    return 0;
}

SithCogScript* J3DAPI sithCog_LoadScript(SithWorld* pWorld, const char* pName)
{
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(pWorld != NULL);

    char aPath[128];
    SITH_ASSERTREL(strlen(pName) + 32 < STD_ARRAYLEN(aPath));

    STD_FORMAT(aPath, "%s%c%s", "cog", '\\', pName);

    SithCogScript* pScript = sithCog_GetScript(pName);
    if ( pScript )
    {
        return pScript;
    }

    if ( pWorld->numCogScripts >= pWorld->sizeCogScripts )
    {
        SITHLOG_ERROR("No space left to load script %s.\n", pName);
        return NULL;
    }

    pScript = &pWorld->aCogScripts[pWorld->numCogScripts];
    if ( !sithCogParse_Load(aPath, pScript, /*bParseDescription=*/0) )
    {
        SITHLOG_ERROR("Load failed for script %s.\n", aPath);
        return NULL;
    }

    sithCog_AddScript(pScript);
    ++pWorld->numCogScripts;
    return pScript;
}

void J3DAPI sithCog_ProcessCog(SithCog* pCog)
{
    if ( (pCog->flags & SITHCOG_DISABLED) != 0 )
    {
        return;
    }

    if ( (pCog->flags & SITHCOG_PULSE_SET) != 0 && sithTime_g_msecGameTime >= pCog->msecNextPulseTime )
    {
        pCog->msecNextPulseTime = pCog->msecPulseInterval + sithTime_g_msecGameTime;
        sithCog_SendMessage(pCog, SITHCOG_MSG_PULSE, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
    }

    if ( (pCog->flags & SITHCOG_TIMER_SET) != 0 && sithTime_g_msecGameTime >= pCog->msecTimerTimeout )
    {
        pCog->flags  &= ~SITHCOG_TIMER_SET;
        pCog->msecTimerTimeout = 0;
        sithCog_SendMessage(pCog, SITHCOG_MSG_TIMER, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
    }

    switch ( pCog->status )
    {
        case SITHCOG_STATUS_SLEEPING:
        {
            if ( (unsigned int)pCog->statusParams[0] >= sithTime_g_msecGameTime ) // Fixed: Make sure pCog->statusParams[0]  is unsigned when compare it to current time
            {
                // Cog still needs to sleep
                return;
            }

            // Ok wake up COG now and continue execution
            if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
            {
                SITHCONSOLE_PRINTF("Cog %s: Waking up due to timer elapse.\n", pCog->aName);
            }

            sithCogExec_Execute(pCog);
            return;
        }
        case SITHCOG_STATUS_WAITING_THING_TO_STOP:
        {
            if ( (sithWorld_g_pCurrentWorld->aThings[pCog->statusParams[0]].moveInfo.pathMovement.mode & (SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE)) != 0 )
            {
                // Thing is still moving
                return;
            }

            // Ok thing stopped continue execution
            if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
            {
                SITHCONSOLE_PRINTF("Cog %s: Waking up due to movement completion.\n", pCog->aName);
            }

            sithCogExec_Execute(pCog);
            return;
        }
        case SITHCOG_STATUS_WAITING_KEYFRAME_TO_STOP:
        {
            SithThing* pThing = &sithWorld_g_pCurrentWorld->aThings[pCog->statusParams[1]];
            int trackNum = sithPuppet_GetTrackNumForGUID(pThing, pCog->statusParams[0]);
            if ( trackNum == -1 )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    SITHCONSOLE_PRINTF("Cog %s: waking up due to keyframe completion.\n", pCog->aName);
                }

                sithCogExec_Execute(pCog);
                return;
            }

            if ( (pThing->renderData.pPuppet->aTracks[trackNum].status & RDPUPPET_TRACK_PLAYING) != 0 )
            {
                // Keyframe is still playing
                return;
            }

            // Keyframe stopped playing, continue execution
            if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
            {
                SITHCONSOLE_PRINTF("Cog %s: waking up due to keyframe completion.\n", pCog->aName);
            }

            sithCogExec_Execute(pCog);
            return;
        }
        case SITHCOG_STATUS_WAITING_SOUND_TO_STOP:
        {
            tSoundChannelHandle channel = sithSoundMixer_GetChannelHandle(pCog->statusParams[0]);
            if ( (Sound_GetChannelFlags(channel) & SOUND_CHANNEL_PLAYING) != 0 )
            {
                // Sound is sill playing
                return;
            }

            // Sound stopped playing continue execution
            if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
            {
                SITHCONSOLE_PRINTF("Cog %s: waking up due to sound completion.\n", pCog->aName);
            }

            sithCogExec_Execute(pCog);
            return;
        }
        case SITHCOG_STATUS_WAITING_AI_TO_STOP:
        {
            SithThing* pThing = &sithWorld_g_pCurrentWorld->aThings[pCog->statusParams[0]];
            if ( !pThing || pThing->type == SITH_THING_FREE )
            {
                // Thing no longer exists
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    SITHCONSOLE_PRINTF("Cog %s: Was waiting on AI movement completion, but AI no longer exists!", pCog->aName);
                }

                sithCogExec_Execute(pCog);
                return;
            }

            if ( !pThing->controlInfo.aiControl.pLocal )
            {
                // AI no longer exists
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    SITHCONSOLE_PRINTF("Cog %s: Was waiting on AI %s movement completion, but AI control block no longer exists!", pCog->aName, pThing->aName);
                }

                sithCogExec_Execute(pCog);
                return;
            }

            if ( (pThing->controlInfo.aiControl.pLocal->mode & (SITHAI_MODE_TURNING | SITHAI_MODE_MOVING)) != 0 )
            {
                // AI is still moving
                return;
            }

            // AI stopped, continue execution
            if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
            {
                SITHCONSOLE_PRINTF("Cog %s: Waking up due to AI movement completion.\n", pCog->aName);
            }

            sithCogExec_Execute(pCog);
            return;
        }
        case SITHCOG_STATUS_WAITING_ACTOR_WEAPON_SELECT_FINISH:
        {
            if ( !sithWeapon_SelectWeapon(&sithWorld_g_pCurrentWorld->aThings[pCog->statusParams[0]], (SithWeaponId)pCog->statusParams[1]) )
            {
                // Still couldn't select weapon
                return;
            }

            // Weapon selected, continue execution
            if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
            {
                SITHCONSOLE_PRINTF("Weapon selection successful. Waking up cog %s.\n", pCog->aName);
            }

            sithCogExec_Execute(pCog);
            return;
        }
        case SITHCOG_STATUS_WAITING_ACTOR_WEAPON_DESELECT_FINISH:
        {
            if ( sithWorld_g_pCurrentWorld->aThings[pCog->statusParams[0]].thingInfo.actorInfo.weaponInfo.deselectedWeaponID != SITHWEAPON_NOWEAPONDESELECTED
                || sithWeapon_GetMountWait() > (double)sithTime_g_secGameTime )
            {
                // Weapon still not deselected
                return;
            }

            // Weapon deselected, continue execution
            if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
            {
                SITHCONSOLE_PRINTF("Weapon deselection complete. Waking up cog %s.\n", pCog->aName);
            }

            sithCogExec_Execute(pCog);
            return;
        }
        case SITHCOG_STATUS_WAITING_ANIMATION_TO_STOP:
        {
            if ( sithAnimate_GetAnim(pCog->statusParams[0]) )
            {
                // Animation is still playing
                return;
            }

            // Animation finished playing, continue execution
            if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
            {
                SITHCONSOLE_PRINTF("Cog %s: Waking up due to animation completion.\n", pCog->aName);
            }

            sithCogExec_Execute(pCog);
            return;
        }
        case SITHCOG_STATUS_WAITING_AI_HEAD_TRACK:
        {
            SithThing* pThing = &sithWorld_g_pCurrentWorld->aThings[pCog->statusParams[0]];
            if ( !pThing || pThing->type == SITH_THING_FREE )
            {
                // AI thing gone
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    SITHCONSOLE_PRINTF("Cog %s: Was waiting on AI head tracking, but AI no longer exists!", pCog->aName);
                }

                sithCogExec_Execute(pCog);
                return;
            }

            if ( !pThing->controlInfo.aiControl.pLocal )
            {
                // AI gone
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    SITHCONSOLE_PRINTF("Cog %s: Was waiting on AI %s head tracking, but AI control block no longer exists!", pCog->aName, pThing->aName);
                }

                sithCogExec_Execute(pCog);
                return;
            }

            if ( (pThing->controlInfo.aiControl.pLocal->submode & SITHAI_SUBMODE_HEADTRACKINGMOTION) == 0 )
            {
                // AI head tracking stopped
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    SITHCONSOLE_PRINTF("Cog %s: Waking up due to AI head tracking at max yaw.\n", pCog->aName);
                }

                sithCogExec_Execute(pCog);
            }
            return;
        }
        case SITHCOG_STATUS_WAITING_PLAYER_TO_STOP:
        {
            SithThing* pThing = sithPlayer_g_pLocalPlayerThing;
            if ( !sithPlayer_g_pLocalPlayerThing || sithPlayer_g_pLocalPlayerThing->type == SITH_THING_FREE )
            {
                SITHCONSOLE_PRINTF("Cog %s was waiting for player to stop, player is gone!", pCog->aName);
                sithCogExec_Execute(pCog);
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_STILL )
            {
                sithCogExec_Execute(pCog);
            }

            return;
        }

        default:
            return;
    }
}

void sithCog_ProcessCogs(void)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    SITH_ASSERTREL(pWorld != NULL);

    if ( sithMain_g_sith_mode.masterMode != SITH_MODE_UNKNOWN_2 )
    {
        SithCog* pCog = pWorld->aCogs;
        for ( size_t i = 0; i < pWorld->numCogs; ++i )
        {
            sithCog_ProcessCog(pCog++);
        }

        if ( sithWorld_g_pStaticWorld )
        {
            pWorld = sithWorld_g_pStaticWorld;
            pCog = sithWorld_g_pStaticWorld->aCogs;
            for ( size_t i = 0; i < pWorld->numCogs; ++i )
            {
                sithCog_ProcessCog(pCog++);
            }
        }
    }
}

int J3DAPI sithCog_CogStatus(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    size_t index;
    SithCog* pCog;
    if ( !pWorld
        || !pArg
        || sscanf_s(pArg, "%d", &index) != 1
        || index > pWorld->numCogs
        || (pCog = &pWorld->aCogs[index], !pCog->pScript)
        || !pCog->pSymbolTable )
    {
        sithConsole_PrintString("Error, bad parameters.\n");
        return 0;
    }

    SITHCONSOLE_PRINTF("Cog #%d: Name:%s  Script %s\n", index, pCog->aName, pCog->pScript->aName);

    for ( size_t i = 0; i < pCog->pSymbolTable->numUsedSymbols; i++ )
    {
        SithCogSymbol* pSymbol = &pCog->pSymbolTable->aSymbols[i];
        if ( pSymbol->pName )
        {
            STD_FORMAT(std_g_genBuffer, "  Symbol %d: '%s' ", pSymbol->id, pSymbol->pName);
        }
        else
        {
            STD_FORMAT(std_g_genBuffer, "  Symbol %d: '%s' ", pSymbol->id, "<null>");
        }

        size_t curPos = strlen(std_g_genBuffer);
        if ( pSymbol->value.type == SITHCOG_VALUE_FLOAT )
        {
            stdUtil_Format(&std_g_genBuffer[curPos], STD_ARRAYLEN(std_g_genBuffer) - curPos, " = %f\n", pSymbol->value.val.floatValue);
        }
        else
        {
            stdUtil_Format(&std_g_genBuffer[curPos], STD_ARRAYLEN(std_g_genBuffer) - curPos, " = %d\n", pSymbol->value.val.intValue);
        }

        sithConsole_PrintString(std_g_genBuffer);
    }

    return 1;
}

void J3DAPI sithCog_AddIntSymbol(SithCogSymbolTable* pTbl, int val, const char* pName)
{
    SithCogSymbol* pSymbol = sithCogParse_AddSymbol(pTbl, pName);
    if ( !pSymbol )
    {
        SITHLOG_ERROR("Could not add symbol %s to table.\n", pName);
        return;
    }

    SithCogSymbolValue value;
    value.type = SITHCOG_VALUE_INT;
    value.val.intValue = val;
    sithCogParse_SetSymbolValue(pSymbol, &value);
}

int J3DAPI sithCog_TimerEventTask(int msecTime, SithEventParams* pParams)
{
    J3D_UNUSED(msecTime);
    SITH_ASSERTREL(pParams);
    SithCog* pCog = sithCog_GetCogByIndex(pParams->idx);
    if ( pCog )
    {
        sithCog_SendMessageEx(pCog, SITHCOG_MSG_TIMER, SITHCOG_SYM_REF_COG, pCog->idx, SITHCOG_SYM_REF_NONE, 0, pParams->param1, pParams->param2, pParams->param3, 0, 0);
    }
    return 1;
}

SithCogScript* J3DAPI sithCog_GetScript(const char* pName)
{
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(sithCog_pHashtable != NULL);
    return (SithCogScript*)stdHashtbl_Find(sithCog_pHashtable, pName);
}

int J3DAPI sithCog_AddScript(SithCogScript* pScript)
{
    SITH_ASSERTREL(pScript != NULL);
    SITH_ASSERTREL(strlen(pScript->aName) > 0);
    SITH_ASSERTREL(sithCog_pHashtable != NULL);
    return stdHashtbl_Add(sithCog_pHashtable, pScript->aName, pScript);
}

int J3DAPI sithCog_RemoveScript(SithCogScript* pScript)
{
    SITH_ASSERTREL(pScript != NULL);
    SITH_ASSERTREL(strlen(pScript->aName) > 0);
    SITH_ASSERTREL(sithCog_pHashtable != NULL);
    return stdHashtbl_Remove(sithCog_pHashtable, pScript->aName);
}

void sithCog_Initialize(void)
{
    SithCogSymbolTable* pTable = sithCog_g_pSymbolTable;

    sithCogFunction_RegisterFunctions(pTable);
    sithCogFunctionThing_RegisterFunctions(pTable);
    sithCogFunctionAI_RegisterFunctions(pTable);
    sithCogFunctionSurface_RegisterFunctions(pTable);
    sithCogFunctionSound_RegisterFunctions(pTable);
    sithCogFunctionSector_RegisterFunctions(pTable);
    sithCogFunctionPlayer_RegisterFunctions(pTable);
    sithVoice_RegisterCogFunctions(pTable);

    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_ACTIVATE, "activate");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_ACTIVATE, "activated");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_STARTUP, "startup");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_TIMER, "timer");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_BLOCKED, "blocked");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_ENTERED, "entered");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_EXITED, "exited");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_CROSSED, "crossed");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_SIGHTED, "sighted");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_DAMAGED, "damaged");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_ARRIVED, "arrived");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_KILLED, "killed");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_PULSE, "pulse");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_TOUCHED, "touched");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_CREATED, "created");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_LOADING, "loading");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_SELECTED, "selected");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_DESELECTED, "deselected");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_CHANGED, "changed");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_DEACTIVATED, "deactivated");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_SHUTDOWN, "shutdown");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_RESPAWN, "respawn");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_REMOVED, "removed");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_AIM, "aim");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_AIEVENT, "aievent");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_CALLBACK, "callback");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_TAKEN, "taken");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER0, "user0");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER1, "user1");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER2, "user2");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER3, "user3");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER4, "user4");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER5, "user5");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER6, "user6");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER7, "user7");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_NEWPLAYER, "newplayer");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_FIRE, "fire");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_JOIN, "join");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_LEAVE, "leave");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_SPLASH, "splash");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_TRIGGER, "trigger");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_STATECHANGE, "statechange");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_MISSED, "missed");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_BOARDED, "boarded");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_UNBOARDED, "unboarded");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_ARRIVEDWPNT, "arrivedwpnt");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_INITIALIZED, "initialized");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_UPDATEWPNTS, "updatewpnts");

    sithCog_AddFloatSymbol(pTable, "global0", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global1", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global2", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global3", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global4", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global5", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global6", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global7", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global8", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global9", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global10", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global11", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global12", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global13", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global14", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global15", 0.0f);
}

void J3DAPI sithCog_AddFloatSymbol(SithCogSymbolTable* pTbl, const char* pName, float value)
{
    SithCogSymbol* pSymbol = sithCogParse_AddSymbol(pTbl, pName);
    if ( !pSymbol )
    {
        SITHLOG_ERROR("Could not add symbol %s to table.\n", pName);
        return;
    }

    SithCogSymbolValue val;
    val.type = SITHCOG_VALUE_FLOAT;
    val.val.floatValue = value;
    sithCogParse_SetSymbolValue(pSymbol, &val);
}

int J3DAPI sithCog_RegisterFunction(SithCogSymbolTable* pTable, SithCogFunctionType pFunction, const char* pName)
{
    SithCogSymbol* pSymbol = sithCogParse_AddSymbol(pTable, pName);
    if ( !pSymbol )
    {
        SITHLOG_ERROR("Could not add symbol %s to table.\n", pName);
        return 1;
    }

    SithCogSymbolValue value;
    value.type = SITHCOG_VALUE_POINTER;
    value.val.pointerValue = (void*)pFunction;
    sithCogParse_SetSymbolValue(pSymbol, &value);
    return 0;
}

int J3DAPI sithCog_IsThingLinked(const SithThing* pThing)
{
    for ( SithCogThingLink* pLink = aThingLinks; pLink < &aThingLinks[numThingLinks]; ++pLink )
    {
        if ( pLink->pThing == pThing )
        {
            return 1;
        }
    }

    return 0;
}