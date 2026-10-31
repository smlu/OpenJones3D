#include "jonesCog.h"
#include <j3dcore/j3dhook.h>

#include <Jones3D/Display/JonesConsole.h>
#include <Jones3D/Display/JonesHud.h>
#include <Jones3D/Main/JonesLevel.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Play/jonesInventory.h>
#include <Jones3D/RTI/symbols.h>

#include <sith/AI/sithAI.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdUtil.h>

#ifdef J3D_STANDALONE
// Added: Standalone builds have no retail executable to hold these. *_ResetGlobals sets the starting values.
int jonesCog_g_bShowHealthHUD;
int jonesCog_g_bMenuVisible;
int jonesCog_g_bEnableGamesave;
int jonesCog_g_dword_17EFFCC;
#endif

static int jonesCog_curCutsceneType     = -1;
static int jonesCog_bInterfaceEnabled   = -1;
static bool jonesCog_bOverlayMapVisible = false;
static bool jonesCog_bStartup           = false;

int J3DAPI jonesCog_RegisterFunctions();
void J3DAPI jonesCog_HideHealthDisplay(SithCog* pCog);
void J3DAPI jonesCog_ShowHealthDisplay(SithCog* pCog);
void J3DAPI jonesCog_EnableInterfaceFunc(SithCog* pCog);
void J3DAPI jonesCog_EndLevelFunc(SithCog* pCog);
void J3DAPI jonesCog_ExitToShell(SithCog* pCog);
void J3DAPI jonesCog_StartCutsceneFunc(SithCog* pCog);
void J3DAPI jonesCog_EndCutscene(SithCog* pCog);
void J3DAPI jonesCog_UpdateDifficulty(SithCog* pCog);
void J3DAPI jonesCog_InventoryItemChanged(SithCog* pCog);

void jonesCog_InstallHooks(void)
{
    J3D_HOOKFUNC(jonesCog_Startup);
    J3D_HOOKFUNC(jonesCog_Shutdown);
    J3D_HOOKFUNC(jonesCog_RegisterFunctions);
    J3D_HOOKFUNC(jonesCog_HideHealthDisplay);
    J3D_HOOKFUNC(jonesCog_ShowHealthDisplay);
    J3D_HOOKFUNC(jonesCog_InventoryItemChanged);
    J3D_HOOKFUNC(jonesCog_EndLevel);
    J3D_HOOKFUNC(jonesCog_EndLevelFunc);
    J3D_HOOKFUNC(jonesCog_ExitToShell);
    J3D_HOOKFUNC(jonesCog_StartCutsceneFunc);
    J3D_HOOKFUNC(jonesCog_EndCutscene);
    J3D_HOOKFUNC(jonesCog_UpdateDifficulty);
    J3D_HOOKFUNC(jonesCog_EnableInterfaceFunc);
    J3D_HOOKFUNC(jonesCog_StartCutscene);
    J3D_HOOKFUNC(jonesCog_ToggleInterface);
}

void jonesCog_ResetGlobals(void)
{
    memset(&jonesCog_g_bShowHealthHUD, 0, sizeof(jonesCog_g_bShowHealthHUD));
    memset(&jonesCog_g_bMenuVisible, 0, sizeof(jonesCog_g_bMenuVisible));
    memset(&jonesCog_g_bEnableGamesave, 0, sizeof(jonesCog_g_bEnableGamesave));
    memset(&jonesCog_g_dword_17EFFCC, 0, sizeof(jonesCog_g_dword_17EFFCC));
}

int jonesCog_Startup(void)
{
    if ( jonesCog_bStartup ) {
        return 1;
    }

    if ( jonesCog_RegisterFunctions() ) {
        return 1;
    }

    JonesMain_UpdateLevelNum();
    jonesCog_bOverlayMapVisible = false;
    jonesCog_curCutsceneType    = -1;
    jonesCog_bStartup           = true;
    return 0;
}

void jonesCog_Shutdown(void)
{
    jonesCog_bStartup = false;
}

int J3DAPI jonesCog_RegisterFunctions()
{
    return sithCog_RegisterFunction(sithCog_g_pSymbolTable, jonesCog_HideHealthDisplay, "healthdisplayoff")
        || sithCog_RegisterFunction(sithCog_g_pSymbolTable, jonesCog_ShowHealthDisplay, "healthdisplayon")
        || sithCog_RegisterFunction(sithCog_g_pSymbolTable, jonesCog_InventoryItemChanged, "jonesinvitemchanged")
        || sithCog_RegisterFunction(sithCog_g_pSymbolTable, jonesCog_EndLevelFunc, "jonesendlevel")
        || sithCog_RegisterFunction(sithCog_g_pSymbolTable, jonesCog_ExitToShell, "exittoshell")
        || sithCog_RegisterFunction(sithCog_g_pSymbolTable, jonesCog_StartCutsceneFunc, "startcutscene")
        || sithCog_RegisterFunction(sithCog_g_pSymbolTable, jonesCog_EndCutscene, "endcutscene")
        || sithCog_RegisterFunction(sithCog_g_pSymbolTable, jonesCog_UpdateDifficulty, "updatedifficulty")
        || sithCog_RegisterFunction(sithCog_g_pSymbolTable, jonesCog_EnableInterfaceFunc, "enableinterface");
}

void J3DAPI jonesCog_HideHealthDisplay(SithCog* pCog)
{
    J3D_UNUSED(pCog);
    jonesCog_g_bShowHealthHUD = 0;
    JonesHud_SetFadeHealthHUD(0, 1);
}

void J3DAPI jonesCog_ShowHealthDisplay(SithCog* pCog)
{
    J3D_UNUSED(pCog);
    jonesCog_g_bShowHealthHUD = 1;
    JonesHud_SetFadeHealthHUD(1, 1);
}

void J3DAPI jonesCog_InventoryItemChanged(SithCog* pCog)
{
    int bin = sithCogExec_PopInt(pCog);
    JonesHud_InventoryItemChanged(bin);
}

void jonesCog_EndLevel(void)
{
    if ( JonesConsole_g_bVisible ) {
        JonesConsole_HideConsole();
    }

    jonesInventory_AdvanceStatistics();

    ++sithPlayer_g_curLevelNum;
    jonesCog_g_bShowHealthHUD = 0;

    if ( sithOverlayMap_IsMapVisible() ) {
        sithOverlayMap_ToggleMap();
    }

    sithSoundMixer_StopAll();

    if ( sithPlayer_g_curLevelNum >= JONESLEVEL_BONUSLEVELNUM ) { // If cur level is Peru or higher show end credits
        JonesMain_ShowEndCredits();
    }
    else if ( JonesHud_ShowLevelCompleted() )
    {
        JonesMain_SetBonusLevel();
        sithPlayer_g_bBonusMapBought = 1;
    }
    else if ( sithPlayer_g_curLevelNum != JONESLEVEL_LASTLEVELNUM || sithPlayer_g_bBonusMapBought ) {
        JonesMain_NextLevel();
    }
    else
    {
        sithSoundMixer_StopAll();
        JonesMain_ShowEndCredits();
    }
}

void J3DAPI jonesCog_EndLevelFunc(SithCog* pCog)
{
    J3D_UNUSED(pCog);
    jonesCog_EndLevel();
}

void J3DAPI jonesCog_ExitToShell(SithCog* pCog)
{
    J3D_UNUSED(pCog);
    JonesMain_ShowEndCredits();
}

void J3DAPI jonesCog_StartCutsceneFunc(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    STD_ASSERTREL(pWorld != NULL);

    int type;
    if ( pCog )
    {
        type = sithCogExec_PopInt(pCog);
        if ( jonesCog_g_bMenuVisible )
        {
            JonesHud_CutsceneStart(1);
            jonesCog_curCutsceneType = type;
            return;
        }
    }
    else
    {
        type = jonesCog_curCutsceneType;
        if ( jonesCog_curCutsceneType <= -1 )
        {
            return;
        }

        jonesCog_curCutsceneType = -1;
    }

    jonesCog_bOverlayMapVisible = sithOverlayMap_IsMapVisible();
    if ( jonesCog_bOverlayMapVisible ) {
        sithOverlayMap_ToggleMap();
    }

    // Added
    if ( JonesConsole_g_bVisible ) {
        JonesConsole_HideConsole();
    }

    if ( type )
    {
        if ( type == 1 )
        {
            jonesCog_g_bShowHealthHUD = 0;
            JonesHud_SetFadeHealthHUD(0, 1);
        }

        else if ( type == 2 )
        {
            jonesCog_g_bShowHealthHUD = 0;
            JonesHud_SetFadeHealthHUD(0, 0);
        }
    }
    else
    {
        jonesCog_g_bShowHealthHUD = 1;
        JonesHud_SetFadeHealthHUD(1, 0);
    }

    JonesHud_EnableMenu(0);

    jonesCog_g_bEnableGamesave = 0;
    sithCamera_g_stateFlags |= SITHCAMERA_STATE_CUTSCENE;

    if ( sithPlayer_g_pLocalPlayerThing )
    {
        SithThing* pPlayerThing = sithPlayer_g_pLocalPlayerThing;
        sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
        if ( pPlayerThing->pCog )
        {
            sithCog_SendMessage(pPlayerThing->pCog, SITHCOG_MSG_USER2, SITHCOG_SYM_REF_THING, pPlayerThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }

    // Destroy all flying bullets
    SithThing* pCurThing = pWorld->aThings;
    int thingNum = 0;
    while ( thingNum <= pWorld->lastThingIdx )
    {
        if ( pCurThing->type == SITH_THING_WEAPON )
        {
            sithThing_DestroyThing(pCurThing);
        }

        ++thingNum;
        ++pCurThing;
    }

    sithPlayerControls_g_bCutsceneMode = 1;
    sithAI_StopAllAttackingAIs();
}

void J3DAPI jonesCog_EndCutscene(SithCog* pCog)
{
    J3D_UNUSED(pCog);

    if ( jonesCog_bOverlayMapVisible ) {
        sithOverlayMap_ToggleMap();
    }

    jonesCog_g_bShowHealthHUD = 1;
    JonesHud_SetFadeHealthHUD(1, 1);
    JonesHud_EnableMenu(1);

    jonesCog_g_bEnableGamesave = 1;

    if ( sithPlayer_g_pLocalPlayerThing )
    {
        if ( !sithPlayer_g_bPlayerInvulnerable ) {
            sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_INVULNERABLE;
        }

        if ( sithPlayer_g_pLocalPlayerThing->pCog ) {
            sithCog_SendMessage(sithPlayer_g_pLocalPlayerThing->pCog, SITHCOG_MSG_USER3, SITHCOG_SYM_REF_THING, sithPlayer_g_pLocalPlayerThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }

    sithCamera_g_stateFlags &= ~SITHCAMERA_STATE_CUTSCENE;
    sithPlayerControls_g_bCutsceneMode = 0;
}

void J3DAPI jonesCog_UpdateDifficulty(SithCog* pCog)
{
    unsigned int difficulty = sithCogExec_PopInt(pCog);
    if ( difficulty > 5 ) {
        difficulty = 5;
    }

    jonesInventory_UpdateIQPoints(difficulty);
}

void J3DAPI jonesCog_EnableInterfaceFunc(SithCog* pCog)
{
    int bEnable = 0; // Added: init to 0
    if ( pCog )
    {
        bEnable = sithCogExec_PopInt(pCog);
        if ( jonesCog_g_bMenuVisible )
        {
            JonesHud_EnableInterface(1);
            jonesCog_bInterfaceEnabled = bEnable;
            return;
        }
    }
    else
    {
        bEnable = jonesCog_bInterfaceEnabled;
        if ( jonesCog_bInterfaceEnabled <= -1 )
        {
            return;
        }

        jonesCog_bInterfaceEnabled = -1;
    }

    JonesHud_EnableMenu(bEnable);
    jonesCog_g_bEnableGamesave = bEnable;
}

void J3DAPI jonesCog_StartCutscene(SithCog* pCog)
{
    jonesCog_StartCutsceneFunc(pCog);
}

void J3DAPI jonesCog_ToggleInterface(SithCog* pCog)
{
    jonesCog_EnableInterfaceFunc(pCog);
}

void J3DAPI jonesCog_EnableInterface(bool bEnable)
{
    if ( jonesCog_g_bMenuVisible )
    {
        JonesHud_EnableInterface(1);
        jonesCog_bInterfaceEnabled = bEnable ? 1 : 0;
        return;
    }

    JonesHud_EnableMenu(bEnable);
    jonesCog_g_bEnableGamesave = bEnable ? 1 : 0;
}

void J3DAPI jonesCog_EnableIndicatrors(bool bEnable)
{
    if ( bEnable )
    {
        jonesCog_g_bShowHealthHUD = 1;
        JonesHud_SetFadeHealthHUD(1, 1);
    }
    else
    {
        jonesCog_g_bShowHealthHUD = 0;
        JonesHud_SetFadeHealthHUD(0, 0);
    }
}
