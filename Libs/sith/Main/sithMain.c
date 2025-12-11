#include "sithMain.h"
#include "sithString.h"
#include <j3dcore/j3dhook.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIAwareness.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/AI/sithAIClass.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithGamesave.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithParticle.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSprite.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdPrimit3.h>

#include <std/General/stdColor.h>
#include <std/General/stdConfig.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdFnames.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#define SITH_PATHSIZE 128
#define SITH_DEFAULTGAMEDIFFICULTY 5

static bool sith_bStartup = false; // Added: Init to false
static bool sith_bOpen    = false; // Added: Init to false

SithOpenCallback sith_pfOpenCallback = NULL; // Added: Init to nil

static char sith_aLevelNdsFileNameBuf[512];
static char sith_aScreenShotsFileName[SITH_PATHSIZE];
static char sith_aNdsLevelFilenameBuf[512];
static char sith_aTmpQuickSavePrefix[SITH_PATHSIZE];
static char sith_aTmpSaveGameDirPath[SITH_PATHSIZE];
static char sith_aTmpAutoSaveFilePrefix[SITH_PATHSIZE];
static char sith_aTmpScreenShotDirPath[SITH_PATHSIZE];

static int sith_bDrawUnknown;
static bool sith_bDrawPlayerMoveBounds = false; // Added: Init to false
static bool sith_bDrawThingMoveBounds  = false; // Added: Init to false
static int sith_performanceLevel;

rdVector3 sith_unknownPos;
rdVector3 sith_unknownPos2;

float sith_unknownRadius;
float sith_unknownRadius2;

size_t sithMain_g_intendedFrameNumber = 0;    //new
static float sith_frameAccumulator    = 0.0f; // new

static const SithMainStartLevelNdsInfo sithMain_aLevelNdsInfos[17] =
{
    // Note all level file names must be of 6 chars; see sithGetCurrentWorldSaveName
    { "SITHSTRING_STARTCYN", "00_CYN" },
    { "SITHSTRING_STARTBAB", "01_BAB" },
    { "SITHSTRING_STARTRIV", "02_RIV" },
    { "SITHSTRING_STARTSHS", "03_SHS" },
    { "SITHSTRING_STARTLAG", "05_LAG" },
    { "SITHSTRING_STARTVOL", "06_VOL" },
    { "SITHSTRING_STARTTEM", "07_TEM" },
    { "SITHSTRING_STARTJEP", "16_JEP" },
    { "SITHSTRING_STARTTEO", "08_TEO" },
    { "SITHSTRING_STARTOLV", "09_OLV" },
    { "SITHSTRING_STARTSEA", "10_SEA" },
    { "SITHSTRING_STARTPYR", "11_PYR" },
    { "SITHSTRING_STARTSOL", "12_SOL" },
    { "SITHSTRING_STARTNUB", "13_NUB" },
    { "SITHSTRING_STARTINF", "14_INF" },
    { "SITHSTRING_STARTAET", "15_AET" },
    { "SITHSTRING_STARTPRU", "17_PRU" }
};

const char* J3DAPI sithGetPath(char* aOutPath, const char* pSithStrName, int bPrependInstallDirPath);

void sithMain_InstallHooks(void)
{
    J3D_HOOKFUNC(sithSetServices);
    J3D_HOOKFUNC(sithClearServices);
    J3D_HOOKFUNC(sithStartup);
    J3D_HOOKFUNC(sithShutdown);
    J3D_HOOKFUNC(sithSetPerformanceLevel);
    J3D_HOOKFUNC(sithGetPerformanceLevel);
    J3D_HOOKFUNC(sithOpenStatic);
    J3D_HOOKFUNC(sithCloseStatic);
    J3D_HOOKFUNC(sithOpenNormal);
    J3D_HOOKFUNC(sithOpenPostProcess);
    J3D_HOOKFUNC(sithOpen);
    J3D_HOOKFUNC(sithClose);
    J3D_HOOKFUNC(sithUpdate);
    J3D_HOOKFUNC(sithDrawScene);
    J3D_HOOKFUNC(sithSetGameDifficulty);
    J3D_HOOKFUNC(sithGetGameDifficulty);
    J3D_HOOKFUNC(sithGetHitAccuarancyScalar);
    J3D_HOOKFUNC(sithGetCombatDamageScalar);
    J3D_HOOKFUNC(sithGetIMPDamageScalar);
    J3D_HOOKFUNC(sithAdvanceRenderTick);
    J3D_HOOKFUNC(sithMakeDirs);
    J3D_HOOKFUNC(sithGetAutoSaveFilePrefix);
    J3D_HOOKFUNC(sithGetPath);
    J3D_HOOKFUNC(sithGetSaveGamesDir);
    J3D_HOOKFUNC(sithGetQuickSaveFilePrefix);
    J3D_HOOKFUNC(sithGetScreenShotsDir);
    J3D_HOOKFUNC(sithGetScreenShotFileTemplate);
    J3D_HOOKFUNC(sithSetOpenCallback);
    J3D_HOOKFUNC(sithGetCurrentWorldSaveName);
    J3D_HOOKFUNC(sithGetLevelSaveFilename);
}

void sithMain_ResetGlobals(void)
{
    memset(&sith_g_pHS, 0, sizeof(sith_g_pHS));
    memset(&sithMain_g_frameNumber, 0, sizeof(sithMain_g_frameNumber));
    memset(&sithMain_g_intendedFrameNumber, 0, sizeof(sithMain_g_intendedFrameNumber));
    memset(&sithMain_g_sith_mode, 0, sizeof(sithMain_g_sith_mode));
    memset(&sithMain_g_curRenderTick, 0, sizeof(sithMain_g_curRenderTick));
}

void J3DAPI sithSetServices(tHostServices* pHS)
{
    sith_g_pHS = pHS;
}

void sithClearServices(void)
{
    sith_g_pHS = NULL;
}

int sithStartup(void)
{
    //TODO: Add check for system already being started

    sithGamesave_Startup();
    int bSuccess = sithEvent_Startup() & 1;
    bSuccess &= sithWorld_Startup();
    bSuccess &= sithRender_Startup();
    sithCollision_Startup();
    sithPhysics_Startup(); // Added
    bSuccess &= sithThing_Startup();
    sithMessage_Startup();

    if ( sithCog_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithAI_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithSprite_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithParticle_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithPuppet_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithAIClass_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithMaterial_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithTemplate_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithModel_Startup() )
    {
        bSuccess = 0;
    }

    sithAnimate_Startup();
    sithFX_Reset();

    bSuccess &= sithVoice_Startup();

    memset(&sithMain_g_sith_mode, 0, sizeof(sithMain_g_sith_mode));

    int difficulty = stdConfig_GetInt(SITH_CFG_GAMEPLAY_DIFFICULTY, SITH_DEFAULTGAMEDIFFICULTY);
    sithSetGameDifficulty(difficulty);

    memset(sith_aTmpAutoSaveFilePrefix, 0, sizeof(sith_aTmpAutoSaveFilePrefix));
    memset(sith_aTmpSaveGameDirPath, 0, sizeof(sith_aTmpSaveGameDirPath));
    memset(sith_aTmpQuickSavePrefix, 0, sizeof(sith_aTmpQuickSavePrefix));
    memset(sith_aTmpScreenShotDirPath, 0, sizeof(sith_aTmpScreenShotDirPath));
    memset(sith_aScreenShotsFileName, 0, sizeof(sith_aScreenShotsFileName));

    if ( !bSuccess )
    {
        SITHLOG_ERROR("Sith startup failed.\n");
        return 1;
    }

    sith_bStartup = true;
    return 0;
}


void sithShutdown(void)
{
    if ( sith_bStartup )
    {
        // TODO: Shall we also try to close our module?

        sithVoice_Shutdown();
        sithAnimate_Shutdown();
        sithModel_Shutdown();
        sithTemplate_Shutdown();
        sithMaterial_Shutdown();
        sithAIClass_Shutdown();
        sithPuppet_Shutdown();
        sithParticle_Shutdown();
        sithSprite_Shutdown();

        sithAI_Shutdown();
        sithCog_Shutdown();
        sithMessage_Shutdown();
        sithThing_Shutdown();
        sithCollision_Shutdown();
        sithRender_Shutdown();
        sithWorld_Shutdown();
        sithEvent_Shutdown();
        sithGamesave_Shutdown();

        memset(sith_aTmpAutoSaveFilePrefix, 0, sizeof(sith_aTmpAutoSaveFilePrefix));
        memset(sith_aTmpSaveGameDirPath, 0, sizeof(sith_aTmpSaveGameDirPath));
        memset(sith_aTmpQuickSavePrefix, 0, sizeof(sith_aTmpQuickSavePrefix));
        memset(sith_aTmpScreenShotDirPath, 0, sizeof(sith_aTmpScreenShotDirPath));
        memset(sith_aScreenShotsFileName, 0, sizeof(sith_aScreenShotsFileName));

        sith_bStartup = false;
    }
}

void J3DAPI sithSetPerformanceLevel(size_t level)
{
    sith_performanceLevel = level;
}

int sithGetPerformanceLevel(void)
{
    return sith_performanceLevel;
}

int J3DAPI sithOpenStatic(const char* pFilename)
{
    sithCloseStatic();

    sithWorld_g_pStaticWorld = sithWorld_NewEntry();
    if ( !sithWorld_g_pStaticWorld )
    {
        return 1;
    }

    sithWorld_g_pStaticWorld->state |= SITH_WORLD_STATE_STATIC;

    // Load world from file
    char aPath[128];
    stdFnames_MakePath(aPath, STD_ARRAYLEN(aPath), "ndy", pFilename);
    if ( !sithWorld_Load(sithWorld_g_pStaticWorld, aPath) )
    {
        // Success
        return 0;
    }

    // Failed to load world from cwd path, try from another path
    sithWorld_Free(sithWorld_g_pStaticWorld);
    sithWorld_g_pStaticWorld = sithWorld_NewEntry();
    if ( !sithWorld_g_pStaticWorld )
    {
        return 1;
    }

    sithWorld_g_pStaticWorld->state |= SITH_WORLD_STATE_STATIC; // Fixed: Added static flag

    stdFnames_MakePath(aPath, STD_ARRAYLEN(aPath), "\\Jones3D\\Resource\\Ndy", pFilename);
    if ( !sithWorld_Load(sithWorld_g_pStaticWorld, aPath) )
    {
        // Success
        return 0;
    }

    sithWorld_Free(sithWorld_g_pStaticWorld);
    sithWorld_g_pStaticWorld = NULL;
    return 1;
}

void sithCloseStatic(void)
{
    if ( sithWorld_g_pStaticWorld )
    {
        sithWorld_Free(sithWorld_g_pStaticWorld);
        sithWorld_g_pStaticWorld = NULL;
    }
}

int J3DAPI sithOpenNormal(const char* pWorldName, const wchar_t* pwPlayerName)
{
    sithWorld_g_pCurrentWorld = sithWorld_NewEntry();
    if ( !sithWorld_g_pCurrentWorld )
    {
        return 1;
    }

    sithAIUtil_AIResetWaypoints();

    char aPath[128];
    stdFnames_MakePath(aPath, STD_ARRAYLEN(aPath), "ndy", pWorldName);
    if ( sithWorld_Load(sithWorld_g_pCurrentWorld, aPath) || (sithWorld_InitPlayers(sithWorld_g_pCurrentWorld), sithOpen(pwPlayerName)) )
    {
        SITHLOG_ERROR("Error opening world: %s.\n", pWorldName);
        if ( sithWorld_g_pCurrentWorld )
        {
            sithWorld_Free(sithWorld_g_pCurrentWorld);
        }

        sithWorld_g_pCurrentWorld = NULL;
        return 1;
    }

    // Success
    sithMain_g_sith_mode.masterMode = SITH_MODE_OPENED;
    sithSetGameDifficulty(sithMain_g_sith_mode.difficulty);
    return 0;
}

void sithOpenPostProcess(void)
{
    sithTime_Reset();
    sithInventory_ResetInventory(sithPlayer_g_pLocalPlayerThing);
    sithCog_BroadcastMessage(SITHCOG_MSG_STARTUP, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0);

    // Send initialized & created message to every thing COG
    for ( size_t i = 0; i < sithWorld_g_pCurrentWorld->numThings; ++i )
    {
        SithThing* pThing = &sithWorld_g_pCurrentWorld->aThings[i];
        if ( pThing->pCog )
        {
            sithCog_SendMessage(pThing->pCog, SITHCOG_MSG_INITIALIZED, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
            sithCog_SendMessage(pThing->pCog, SITHCOG_MSG_CREATED, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }

    if ( stdComm_IsGameActive() )
    {
        sithPlayer_NewPlayer(sithPlayer_g_pLocalPlayerThing);
        sithMulti_SendWelcome(sithMessage_g_localPlayerId, sithPlayer_g_playerNum, SITHMESSAGE_SENDTOJOINEDPLAYERS);
        sithMulti_SendWelcome(sithMessage_g_localPlayerId, sithPlayer_g_playerNum, SITHMESSAGE_SENDTOJOINEDPLAYERS); // TODO: Why same message sent twice
    }
    else if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        // Make savegame at the start of the level
        const char* pWorldName      = sithGetCurrentWorldSaveName();
        const char* pSaveNamePrefix = sithGetAutoSaveFilePrefix();

        char aFilename[128];
        STD_FORMAT(aFilename, "%s%s", pSaveNamePrefix, pWorldName);
        stdFnames_ChangeExt(aFilename, "nds");

        const char* pSaveGameDir = sithGetSaveGamesDir();
        char aFilepath[128];
        stdFnames_MakePath(aFilepath, STD_ARRAYLEN(aFilepath), pSaveGameDir, aFilename);

        sithGamesave_Save(aFilepath, 1);
    }

    sithTime_Reset(); // TODO: Why time is reset 2nd time?
    sithFX_Reset();
}

int J3DAPI sithOpenMulti(const char* pFilename, const wchar_t* pwPlayerName)
{
    sithWorld_g_pCurrentWorld = sithWorld_NewEntry();
    if ( !sithWorld_g_pCurrentWorld )
    {
        return 0;
    }

    char aPath[128];
    stdFnames_MakePath(aPath, STD_ARRAYLEN(aPath), "ndy", pFilename);
    if ( sithWorld_Load(sithWorld_g_pCurrentWorld, aPath) || sithOpen(pwPlayerName) )
    {
        SITHLOG_ERROR("Error opening multiplayer level %s.\n", pFilename);
        if ( sithWorld_g_pCurrentWorld )
        {
            sithWorld_Free(sithWorld_g_pCurrentWorld);
        }

        sithWorld_g_pCurrentWorld = NULL;
        return 1;
    }

    // Success
    sithTime_Reset();
    sithMulti_OpenGame();
    sithMain_g_sith_mode.masterMode = SITH_MODE_OPENED;
    return 0;
}

int J3DAPI sithOpen(const wchar_t* pwPlayerName)
{
    // TODO: Add check for the system already being open

    if ( sith_pfOpenCallback )
    {
        sith_pfOpenCallback();
    }

    sithMain_g_frameNumber         = 0;
    sithMain_g_intendedFrameNumber = 0;
    sith_frameAccumulator          = 0.0f;
    sithMain_g_curRenderTick       = 1;
    sithWorld_ResetRenderState(sithWorld_g_pCurrentWorld);

    sithEvent_Open();

    if ( sithAnimate_Open() )
    {
        return 1;
    }

    if ( !sithVoice_Open() )
    {
        return 1;
    }

    sithAI_Open();

    if ( sithCog_Open(sithWorld_g_pCurrentWorld) )
    {
        return 1;
    }

    sithControl_Open();

    // Added
    sithPlayerControls_Reset();

    if ( sithAIAwareness_Open() )
    {
        return 1;
    }

    if ( sithRender_Open() )
    {
        return 1;
    }

    sithPlayer_Open(pwPlayerName);
    sithWeapon_Open();

    if ( sithOverlayMap_Open(0) )
    {
        return 1;
    }

    sithGamesave_Open();

    sith_bOpen = true;
    return 0;
}

void sithClose(void)
{
    if ( sith_bStartup && sith_bOpen )
    {
        if ( (sithMain_g_sith_mode.subModeFlags & 1) != 0 )
        {
            sithMulti_CloseGame();
        }

        sithGamesave_Close();
        sithOverlayMap_Close();
        sithWeapon_Close();
        sithPlayer_Close();
        sithRender_Close();
        sithAIAwareness_Close();

        sithControl_Close();
        sithCog_Close();

        sithWorld_Close(sithWorld_g_pCurrentWorld);
        sithWorld_g_pCurrentWorld = NULL;

        sithAI_Close();
        sithVoice_Close();
        sithAnimate_Close();
        sithEvent_Close();

        if ( sithCamera_g_pCurCamera )
        {
            sithCamera_g_pCurCamera->pSector              = NULL;
            sithCamera_g_pCurCamera->pPrimaryFocusThing   = NULL;
            sithCamera_g_pCurCamera->pSecondaryFocusThing = NULL;
        }

        sithMain_g_sith_mode.masterMode   = SITH_MODE_CLOSED;
        sithMain_g_sith_mode.subModeFlags = 0;

        sith_bOpen = false;
    }
}

void sithUpdate(void)
{
    SITH_ASSERTREL(sithMain_g_sith_mode.masterMode != SITH_MODE_CLOSED);
    if ( (sithMain_g_sith_mode.subModeFlags & 8) != 0 ) // Game host?
    {
        sithTime_Advance();
        sithMessage_ProcessMessages();
        sithAnimate_Update(sithTime_g_frameTimeFlex);
        sithThing_Update(sithTime_g_frameTimeFlex, sithTime_g_frameTime);
    }
    else
    {
        sithSoundMixer_Update();

        if ( sithTime_IsPaused() )
        {
            if ( sithMain_g_sith_mode.masterMode != SITH_MODE_UNKNOWN_2 )
            {
                sithControl_Update(sithTime_g_frameTimeFlex, sithTime_g_frameTime);
            }
        }
        else
        {
            ++sithMain_g_frameNumber;

            sithAdvanceRenderTick();
            sithTime_Advance();

            // update intended frame counter
            sith_frameAccumulator += sithTime_g_frameTimeFlex;
            const float targetFrameTime = 1.0f / SITH_INTENDED_FPS;

            const size_t steps = (size_t)floorf(sith_frameAccumulator / targetFrameTime);

            if ( steps > 0 )
            {
                // subtract the consumed time
                sith_frameAccumulator -= (float)steps * targetFrameTime;
                // increment the virtual frame counter
                sithMain_g_intendedFrameNumber += steps;
            }

            sithEvent_Process();

            if ( sithMessage_g_inputstream )
            {
                sithMessage_ProcessMessages();
            }

            if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_AIEVENTS_DISABLED) == 0 )
            {
                sithAI_Process();
            }

            sithAnimate_Update(sithTime_g_frameTimeFlex);

            if ( sithMain_g_sith_mode.masterMode != SITH_MODE_UNKNOWN_2 )
            {
                sithControl_Update(sithTime_g_frameTimeFlex, sithTime_g_frameTime);
            }

            sithThing_Update(sithTime_g_frameTimeFlex, sithTime_g_frameTime);
            sithCog_ProcessCogs();
            sithMulti_Update(sithTime_g_frameTime);
        }
    }
}

void sithDrawScene(void)
{
    if ( (sithMain_g_sith_mode.subModeFlags & 8) == 0 )
    {
        // Render scene
        sithAdvanceRenderTick();
        sithCamera_Update(sithCamera_g_pCurCamera);
        sithCamera_RenderScene();

        // Draw debug stuff

        if ( sith_bDrawPlayerMoveBounds && sithWorld_g_pCurrentWorld && sithPlayer_g_pLocalPlayerThing )
        {
            rdPrimit3_DrawClippedCircle(&sithPlayer_g_pLocalPlayerThing->pos, sithPlayer_g_pLocalPlayerThing->collide.movesize, 20.0f, STD_RGB(58, 231, 123), 0xFFFFFFFF);
            // Altered: color encoding to 32 bpp, was RGB565 (0xFFFF3F2F)
        }

        if ( sith_bDrawUnknown )
        {
            rdPrimit3_DrawClippedCircle(&sith_unknownPos, sith_unknownRadius, 20.0f, STD_RGB(58, 231, 123), 0xFFFFFFFF);   // Altered: color encoding to 32 bpp, was RGB565 (0xFFFF3F2F)
            rdPrimit3_DrawClippedCircle(&sith_unknownPos2, sith_unknownRadius2, 20.0f, STD_RGB(58, 231, 123), 0xFFFFFFFF); // Altered: color encoding to 32 bpp, was RGB565 (0xFFFF3F2F)
        }

        if ( sith_bDrawThingMoveBounds && sithWorld_g_pCurrentWorld )
        {
            for ( int thNum = 0; thNum <= sithWorld_g_pCurrentWorld->lastThingIdx; ++thNum )
            {
                SithThing* pThing = &sithWorld_g_pCurrentWorld->aThings[thNum];
                if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_WEAPON )
                {
                    rdPrimit3_DrawClippedCircle(&pThing->pos, pThing->collide.movesize, 20.0f, STD_RGB(58, 231, 123), 0xFFFFFFFF); // Altered: color encoding to 32 bpp, was RGB565 (0xFFFF3F2F)
                }
            }
        }
    }
}

void J3DAPI sithSetGameDifficulty(int difficulty)
{
    sithMain_g_sith_mode.difficulty = difficulty;
    if ( sithWorld_g_pCurrentWorld )
    {
        for ( size_t i = 0; i < sithWorld_g_pCurrentWorld->numThings; ++i )
        {
            SithThing* pActor = &sithWorld_g_pCurrentWorld->aThings[i];
            if ( pActor->type == SITH_THING_ACTOR )
            {
                sithActor_SetDifficulty(pActor);
            }
        }
    }
}

int sithGetGameDifficulty(void)
{
    return sithMain_g_sith_mode.difficulty;
}

float sithGetHitAccuarancyScalar(void)
{
    float scalar = 1.0f;
    switch ( sithMain_g_sith_mode.difficulty )
    {
        case 0:
            scalar = 0.5f;
            break;

        case 1:
            scalar = 0.67000002f;
            break;

        case 2:
            scalar = 0.82999998f;
            break;

        case 3:
            scalar = 1.0f;
            break;

        case 4:
            scalar = 1.17f;
            break;

        case 5:
            scalar = 1.33f;
            break;

        default:
            return scalar;
    }

    return scalar;
}

float sithGetCombatDamageScalar(void)
{
    float scalar = 1.0f;
    switch ( sithMain_g_sith_mode.difficulty )
    {
        case 0:
            scalar = 1.5f;
            break;

        case 1:
            scalar = 1.33f;
            break;

        case 2:
            scalar = 1.17f;
            break;

        case 3:
            scalar = 1.0f;
            break;

        case 4:
            scalar = 0.82999998f;
            break;

        case 5:
            scalar = 0.67000002f;
            break;

        default:
            return scalar;
    }

    return scalar;
}

float sithGetIMPDamageScalar(void)
{
    float scalar = 1.0f;
    switch ( sithMain_g_sith_mode.difficulty )
    {
        case 0:
            scalar = 0.5f;
            break;

        case 1:
            scalar = 0.60000002f;
            break;

        case 2:
            scalar = 0.69999999f;
            break;

        case 3:
            scalar = 0.80000001f;
            break;

        case 4:
            scalar = 0.89999998f;
            break;

        case 5:
            scalar = 1.0f;
            break;

        default:
            return scalar;
    }

    return scalar;
}

void sithAdvanceRenderTick(void)
{
    if ( !++sithMain_g_curRenderTick )
    {
        sithWorld_ResetRenderState(sithWorld_g_pCurrentWorld);
        sithMain_g_curRenderTick = 1;
    }
}

bool sithToggleDrawPlayerRadius(void)
{
    sith_bDrawPlayerMoveBounds = !sith_bDrawPlayerMoveBounds;
    return sith_bDrawPlayerMoveBounds;
}

void sithMakeDirs(void)
{
    stdFileUtil_MkDir(sithGetSaveGamesDir());
    stdFileUtil_MkDir(sithGetScreenShotsDir());
}

const char* sithGetAutoSaveFilePrefix(void)
{
    return sithGetPath(sith_aTmpAutoSaveFilePrefix, "SITHSTRING_AUTO", 0);
}

const char* J3DAPI sithGetPath(char* aOutPath, const char* pSithStrName, int bPrependInstallDirPath)
{
    if ( strlen(aOutPath) != 0 ) // if not empty, i.e. inadvertently the string gets cached because the functions that call this functions use static global var to store path
    {
        return aOutPath;
    }

    const wchar_t* pwName = sithString_GetString(pSithStrName);
    if ( !pwName )
    {
        return aOutPath;
    }

    char* pName = stdUtil_ToAString(pwName);

    char aInstallPath[128] = { 0 };
    stdConfig_GetString(SITH_CFG_INSTALLPATH, aInstallPath, STD_ARRAYLEN(aInstallPath), "");

    if ( strlen(pName) )
    {
        if ( strlen(aInstallPath) && bPrependInstallDirPath )
        {
            char* pFilename  = pName;
            char* pStripName = strchr(pName, '\\');
            if ( pStripName )
            {
                pFilename = pStripName + 1;
            }

            while ( pFilename && *pFilename == '\\' )
            {
                ++pFilename;
            }

            stdFnames_MakePath(aOutPath, SITH_PATHSIZE, aInstallPath, pFilename);
        }
        else
        {
            stdUtil_StringCopy(aOutPath, SITH_PATHSIZE, pName);
        }
    }
    else
    {
        stdUtil_StringCopy(aOutPath, SITH_PATHSIZE, pSithStrName);
    }

    if ( pName )
    {
        stdMemory_Free(pName);
    }

    return aOutPath;
}

const char* sithGetSaveGamesDir(void)
{
    return sithGetPath(sith_aTmpSaveGameDirPath, "SITHSTRING_SAVEGAMES", 1);
}

const char* sithGetQuickSaveFilePrefix(void)
{
    return sithGetPath(sith_aTmpQuickSavePrefix, "SITHSTRING_QUICK", 0);
}

const char* sithGetScreenShotsDir(void)
{
    return sithGetPath(sith_aTmpScreenShotDirPath, "SITHSTRING_SHOTS", 1);
}

const char* sithGetScreenShotFileTemplate(void)
{
    return sithGetPath(sith_aScreenShotsFileName, "SITHSTRING_JONES_SHOTS", 0);
}

void J3DAPI sithSetOpenCallback(SithOpenCallback pfCallback)
{
    sith_pfOpenCallback = pfCallback;
}

const char* sithGetCurrentWorldSaveName(void)
{
    if ( !sithWorld_g_pCurrentWorld )
    {
        return 0;
    }

    size_t i;
    for ( i = 0; ; ++i )
    {
        if ( i >= STD_ARRAYLEN(sithMain_aLevelNdsInfos) )
        {
            return sithWorld_g_pCurrentWorld->aName;
        }

        if ( strneqi(sithWorld_g_pCurrentWorld->aName, sithMain_aLevelNdsInfos[i].aFilename, 6u) )
        {
            break;
        }
    }

    const wchar_t* pwName = sithString_GetString(sithMain_aLevelNdsInfos[i].aName);
    if ( !pwName )
    {
        return sithWorld_g_pCurrentWorld->aName;
    }

    char* pName = stdUtil_ToAString(pwName);
    if ( !pName )
    {
        return sithWorld_g_pCurrentWorld->aName;
    }

    STD_STRCPY(sith_aNdsLevelFilenameBuf, pName);
    stdMemory_Free(pName);
    return sith_aNdsLevelFilenameBuf;
}

const char* J3DAPI sithGetLevelSaveFilename(size_t levelNum)
{
    if ( levelNum >= STD_ARRAYLEN(sithMain_aLevelNdsInfos) )
    {
        return 0;
    }

    const wchar_t* pwLevelName = sithString_GetString(sithMain_aLevelNdsInfos[levelNum].aName);
    if ( !pwLevelName )
    {
        return 0;
    }

    char* pLevelName = stdUtil_ToAString(pwLevelName);
    if ( !pLevelName )
    {
        return 0;
    }

    STD_STRCPY(sith_aLevelNdsFileNameBuf, pLevelName);
    stdMemory_Free(pLevelName);
    return sith_aLevelNdsFileNameBuf;
}
