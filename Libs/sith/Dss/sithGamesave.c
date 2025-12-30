#include "sithGamesave.h"
#include <j3dcore/j3dhook.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogParse.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSCog.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/Main/sithString.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdColor.h>
#include <std/General/stdConffile.h>
#include <std/General/stdConfig.h>
#include <std/General/stdFnames.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>

#define SITHSAVEGAME_THUMBCOLORFORMAT stdColor_cfRGB888 // Note, changed encoding format due to little-endian fix in stdColor_ColorConvertOneRow
#define SITHSAVEGAME_THUMBSIZE       (SITHSAVEGAME_THUMB_WIDTH * SITHSAVEGAME_THUMB_HEIGHT * SITHSAVEGAME_THUMBCOLORFORMAT.bpp) / 8
//static_assert(SITHSAVEGAME_THUMBSIZE == 0x9000, "SITHSAVEGAME_THUMBSIZE must be  0x9000 bytes");

#define SITHSAVEGAME_FILEVERSION  13
#define SITHSAVEGAME_ENDFILE      0x1000 

static int sithGamesave_state;

static char sithGamesave_aCurFilename[128];
static char sithGamesave_aLastFilename[128];
static int sithGamesave_aNdsSaveSectionSizes[15];

static char sithGamesave_aPrevLevelFilename[64];

static int sithGamesave_bStatisticsLocked;
static SithGameStatistics sithGamesave_gameStatistics;

static tVBuffer* sithGamesave_pThumbnailImage;
static int sithGamesave_bThumbnail;
static HBITMAP sithGamesave_hBmpThumbnail;

static SithGameSaveCallback sithGamesave_pfSaveGameCallback;


int J3DAPI sithGamesave_SaveCurrentWorld(SithMessageStream outstream);

int J3DAPI sithGamesave_RestoreFile(const char* pFilename, int bNotify);
int J3DAPI sithGamesave_ReadBlockTypeLength(uint16_t* pType, unsigned int* pLength);
int J3DAPI sithGamesave_SaveFile(const char* pFilename);

int J3DAPI sithGamesave_WriteThumbnail(tFileHandle fh);
int J3DAPI sithGamesave_SeekThumbnail(tFileHandle fh);

void sithGamesave_InstallHooks(void)
{
    J3D_HOOKFUNC(sithGamesave_GetGameStatistics);
    J3D_HOOKFUNC(sithGamesave_LockGameStatistics);
    J3D_HOOKFUNC(sithGamesave_UnlockGameStatistics);
    J3D_HOOKFUNC(sithGamesave_Save);
    J3D_HOOKFUNC(sithGamesave_Restore);
    J3D_HOOKFUNC(sithGamesave_SaveCurrentWorld);
    J3D_HOOKFUNC(sithGamesave_Process);
    J3D_HOOKFUNC(sithGamesave_RestoreFile);
    J3D_HOOKFUNC(sithGamesave_NotifyRestored);
    J3D_HOOKFUNC(sithGamesave_ReadBlockTypeLength);
    J3D_HOOKFUNC(sithGamesave_SaveFile);
    J3D_HOOKFUNC(sithGamesave_SetThumbnailImage);
    J3D_HOOKFUNC(sithGamesave_CreateThumbnail);
    J3D_HOOKFUNC(sithGamesave_WriteThumbnail);
    J3D_HOOKFUNC(sithGamesave_SeekThumbnail);
    J3D_HOOKFUNC(sithGamesave_LoadThumbnail);
    J3D_HOOKFUNC(sithGamesave_Startup);
    J3D_HOOKFUNC(sithGamesave_Shutdown);
    J3D_HOOKFUNC(sithGamesave_Open);
    J3D_HOOKFUNC(sithGamesave_CloseRestore);
    J3D_HOOKFUNC(sithGamesave_Close);
    J3D_HOOKFUNC(sithGamesave_SetPreviousLevelFilename);
    J3D_HOOKFUNC(sithGamesave_GetPreviousLevelFilename);
    J3D_HOOKFUNC(sithGamesave_GetLastFilename);
    J3D_HOOKFUNC(sithGamesave_LoadLevelFilename);
    J3D_HOOKFUNC(sithGamesave_GetState);
    J3D_HOOKFUNC(sithGamesave_SetSaveGameCallback);
}

void sithGamesave_ResetGlobals(void)
{}

SithGameStatistics* sithGamesave_GetGameStatistics(void)
{
    if ( sithGamesave_bStatisticsLocked )
    {
        return NULL;
    }
    return &sithGamesave_gameStatistics;
}

int sithGamesave_LockGameStatistics(void)
{
    if ( sithGamesave_bStatisticsLocked )
    {
        return 0;
    }

    sithGamesave_bStatisticsLocked = 1;
    return 1;
}

int sithGamesave_UnlockGameStatistics(void)
{
    sithGamesave_bStatisticsLocked = 0;
    return 1;
}

int J3DAPI sithGamesave_Save(const char* pFilename, int bOverWrite)
{
    SITH_ASSERTREL(pFilename);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    SITH_ASSERTREL(pWorld);

    // Multiplayer game?
    if ( (sithMain_g_sith_mode.subModeFlags & 1) != 0 )
    {
        return 1;
    }

    // Don't save game when player is dead/dying
    if ( (sithPlayer_g_pLocalPlayerThing->flags & SITH_TF_DYING) != 0 )
    {
        return 1;
    }

    // Ensure folder structure exists
    sithMakeDirs();

    if ( !bOverWrite && stdConffile_Open(pFilename) )
    {
        SITHLOG_ERROR("Cannot overwrite existing file without bOverWrite specified.\n");
        stdConffile_Close();
        return 1;
    }

    // Deferred save game
    sithGamesave_state = SITHGAMESAVE_SAVE;
    STD_STRCPY(sithGamesave_aCurFilename, pFilename);
    return 0;
}

int J3DAPI sithGamesave_Restore(const char* pFilename, int bNotifyCog)
{
    // Check restoring savegame file exists and can be opened
    SITH_ASSERTREL(pFilename);
    if ( !stdConffile_OpenMode(pFilename, "rb") )
    {
        SITHLOG_ERROR("Could not open savegame file %s.\n", pFilename);
        return 1;
    }

    stdConffile_Close();

    // If restoring from active game, defer restoring game from savegame file until current game is closed
    if ( sithWorld_g_pCurrentWorld )
    {
        sithGamesave_state = SITHGAMESAVE_RESTORE;
        STD_STRCPY(sithGamesave_aCurFilename, pFilename);
        return 0;
    }

    return sithGamesave_RestoreFile(pFilename, bNotifyCog);
}

int J3DAPI sithGamesave_SaveCurrentWorld(SithMessageStream outstream)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    SITH_ASSERTREL(pWorld);

    // If outstream is not set to required stream, return error
    if ( (outstream & sithMessage_g_outputstream) == 0 )
    {
        return 1;
    }

    if ( sithGamesave_pfSaveGameCallback )
    {
        int bError = sithGamesave_pfSaveGameCallback(DPID_ALLPLAYERS, outstream);
        if ( bError )
        {
            return bError;
        }
    }

    // Write things state
    for ( size_t i = 0; i < pWorld->numThings; i++ )
    {
        const SithThing* pThing = &pWorld->aThings[i];
        if ( sithThing_CanSync(pThing) )
        {
            int bError = sithDSSThing_ThingFullDescription(pThing, DPID_ALLPLAYERS, outstream);
            if ( bError )
            {
                return bError;
            }

            sithGamesave_aNdsSaveSectionSizes[1] += sithMulti_g_message.length;

            if ( pThing->renderData.pPuppet )
            {
                bError = sithDSS_PuppetStatus(pThing, DPID_ALLPLAYERS, outstream);
                if ( bError )
                {
                    return bError;
                }

                sithGamesave_aNdsSaveSectionSizes[2] += sithMulti_g_message.length;
            }
        }
    }

    // Write attachment things state
    for ( size_t i = 0; i < pWorld->numThings; ++i )
    {
        if ( sithThing_CanSync(&pWorld->aThings[i]) && pWorld->aThings[i].attach.flags )
        {
            int bError = sithDSSThing_Attachment(&pWorld->aThings[i], DPID_ALLPLAYERS, outstream, 1u);
            if ( bError )
            {
                return bError;
            }

            sithGamesave_aNdsSaveSectionSizes[3] += sithMulti_g_message.length;
        }
    }

    // Write AIs state
    for ( size_t i = 0; i < STD_ARRAYLEN(sithAI_g_aControlBlocks); ++i )
    {
        if ( sithAI_g_aControlBlocks[i].pClass )
        {
            int bError = sithDSS_AIStatus(&sithAI_g_aControlBlocks[i], DPID_ALLPLAYERS, outstream);
            if ( bError )
            {
                return bError;
            }

            sithGamesave_aNdsSaveSectionSizes[4] += sithMulti_g_message.length;
        }
    }

    // Write AI waypoints
    sithAIUtil_SyncWpnts(DPID_ALLPLAYERS, outstream);

    // Write level cogs state
    for ( size_t i = 0; i < pWorld->numCogs; ++i )
    {
        int bError = sithDSSCog_SyncCogState(&pWorld->aCogs[i], DPID_ALLPLAYERS, outstream);
        if ( bError )
        {
            return bError;
        }

        sithGamesave_aNdsSaveSectionSizes[7] += sithMulti_g_message.length;
    }

    // Write static cogs state
    if ( sithWorld_g_pStaticWorld )
    {
        for ( size_t i = 0; i < sithWorld_g_pStaticWorld->numCogs; ++i )
        {
            int bError = sithDSSCog_SyncCogState(&sithWorld_g_pStaticWorld->aCogs[i], DPID_ALLPLAYERS, outstream);
            if ( bError )
            {
                return bError;
            }

            sithGamesave_aNdsSaveSectionSizes[8] += sithMulti_g_message.length;
        }
    }

    // Write surfaces state
    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        if ( (pWorld->aSurfaces[i].flags & SITH_SURFACE_SYNC) != 0 )
        {
            int bError = sithDSS_SurfaceStatus(&pWorld->aSurfaces[i], DPID_ALLPLAYERS, outstream);
            if ( bError )
            {
                return bError;
            }

            sithGamesave_aNdsSaveSectionSizes[5] += sithMulti_g_message.length;
        }
    }

    // Write sectors state
    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        if ( (pWorld->aSectors[i].flags & SITH_SECTOR_SYNC) != 0 || (pWorld->aSectors[i].flags & SITH_SECTOR_ADJOINSOFF) != 0 )// Hmm is it ok to sync only marked sectors?? Note, There is limited number of sectors that can be synced in one batch
        {
            int bError = sithDSS_SectorStatus(&pWorld->aSectors[i], DPID_ALLPLAYERS, outstream);
            if ( bError )
            {
                return bError;
            }

            sithGamesave_aNdsSaveSectionSizes[6] += sithMulti_g_message.length;
        }
    }

    // Write inventory state
    for ( size_t i = 0; i < STD_ARRAYLEN(sithInventory_g_aTypes); ++i )
    {
        if ( (sithInventory_g_aTypes[i].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
        {
            int bError = sithDSS_Inventory(sithPlayer_g_pLocalPlayerThing, i, DPID_ALLPLAYERS, outstream);
            if ( bError )
            {
                return bError;
            }

            sithGamesave_aNdsSaveSectionSizes[9] += sithMulti_g_message.length;
        }
    }

    // Write state of animate module
    int bError = sithAnimate_Save(outstream);
    if ( bError )
    {
        return bError;
    }

    sithGamesave_aNdsSaveSectionSizes[10] += sithMulti_g_message.length;

    // Write event system state
    for ( SithEvent* pEvent = sithEvent_g_pFirstQueuedEvent; pEvent; pEvent = pEvent->pNextEvent )
    {
        bError = sithDSS_SyncTaskEvents(pEvent, DPID_ALLPLAYERS, outstream);
        if ( bError )
        {
            return bError;
        }

        sithGamesave_aNdsSaveSectionSizes[11] += sithMulti_g_message.length;
    }

    // Write state of all cameras
    bError = sithDSS_SyncCameras(DPID_ALLPLAYERS, outstream);
    if ( bError )
    {
        return bError;
    }

    sithGamesave_aNdsSaveSectionSizes[12] += sithMulti_g_message.length;

    // Write game state (player flying state, player cur weapon num, drawn chalk marks, current cel num of all world materials  etc...)
    bError = sithDSS_SyncGameState(DPID_ALLPLAYERS, outstream);
    if ( bError )
    {
        return bError;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_g_message.length;

    // Write voice system state
    bError = sithVoice_SyncVoiceState(DPID_ALLPLAYERS, outstream);
    if ( bError )
    {
        return bError;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_g_message.length;

    // Write vhicle control system state
    bError = sithDSS_SyncVehicleControlState(DPID_ALLPLAYERS, outstream);
    if ( bError )
    {
        return bError;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_g_message.length;

    // Write whip system state
    bError = sithWhip_Save(DPID_ALLPLAYERS, outstream);
    if ( bError )
    {
        return bError;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_g_message.length;

    // Write state of seen sectors
    bError = sithDSS_SyncSeenSectors(DPID_ALLPLAYERS, outstream);
    if ( bError )
    {
        return bError;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_g_message.length;

    // Write unknown section
    bError = sithDSS_sub_4B3760(DPID_ALLPLAYERS, outstream);
    if ( bError )
    {
        return bError;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_g_message.length;
    return 0;
}

int sithGamesave_Process(void)
{
    int bError;
    if ( sithGamesave_state == SITHGAMESAVE_SAVE )
    {
        if ( !sithGamesave_bThumbnail )
        {
#if defined (J3D_OPENGL)

            //Make sure back buffer fbo is created
            if ( stdDisplay_g_backBuffer.surface.fbo == 0 )
            {
                return 1;
            }

#else
            // Fixed: Make sure back buffer is inited (DirectX 9 could reset device in between calls)
            if ( !stdDisplay_g_backBuffer.surface.pSysSurface )
            {
                return 1;
            }
#endif


            sithGamesave_SetThumbnailImage(&stdDisplay_g_backBuffer);
            sithGamesave_hBmpThumbnail = sithGamesave_CreateThumbnail();
        }

        bError = sithGamesave_SaveFile(sithGamesave_aCurFilename);
        if ( !bError )
        {
            stdConfig_SetString(SITHSAVEGAME_CFG_GAMEPLAY_LASTSAVEGAME, sithGamesave_aCurFilename);
        }

        // If savegame is autosave i.e.: savegame at the start of a level (start_xxx.nds), notify master cog

        // TODO: Why not just call sithGamesave_NotifyRestored
        const char* pPrefix   = sithGetAutoSaveFilePrefix();
        const char* pFilename = stdFnames_FindMedName(sithGamesave_aCurFilename);
        if ( strneq(pFilename, pPrefix, strlen(pPrefix)) && sithCog_g_pMasterCog )
        {
            sithCog_SendMessage(sithCog_g_pMasterCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE,
                                0, 0);
        }
    }
    else
    {
        if ( sithGamesave_state != SITHGAMESAVE_RESTORE )
        {
            return 0;
        }

        sithGamesave_Close(); // Close

        // Restore game from savegame file, and notify master cog
        // Note, notifying via restoring file is not done as it won't do notification when restoring file is not auto save file
        bError = sithGamesave_RestoreFile(sithGamesave_aCurFilename, /*bNotify*/0);

        // Fixed: Added check for ineditor flag not being set.
        //        This fixes restoring savegame that was saved under ineditor flag to not play intro cutscene
        if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
        {
            if ( sithCog_g_pMasterCog )
            {
                sithCog_SendMessage(sithCog_g_pMasterCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
            }
        }
    }

    sithGamesave_state = SITHGAMESAVE_NONE;
    sithGamesave_Close();
    return bError;
}

int J3DAPI sithGamesave_RestoreFile(const char* pFilename, int bNotify)
{
    if ( !stdConffile_OpenMode(pFilename, "rb") )
    {
        SITHLOG_ERROR("RESTORE: could not open restore file!\n");
        goto error;
    }

    // Fixed: Originally, the file handle was not set to the file handle of the opened file but the error status stdConffile_OpenMode returned
    tFileHandle fh = stdConffile_GetFileHandle();

    // Read nds file header
    NdsHeader header;
    if ( !stdConffile_Read(&header, sizeof(NdsHeader)) )
    {
        SITHLOG_ERROR("RESTORE: header read failed!\n");
        goto error;
    }

    // Check header version
    if ( header.version != SITHSAVEGAME_FILEVERSION )
    {
        SITHLOG_ERROR("RESTORE: version numbers don't match!\n");
        goto error;
    }

    sithGamesave_bThumbnail = header.bThumbnail;
    sithSetPerformanceLevel(header.perflevel);

    STD_STRCPY(sithGamesave_aPrevLevelFilename, header.aPreviousLevelFilename);
    sithGamesave_aPrevLevelFilename[63] = 0;

    // Skip thumbnail section
    if ( sithGamesave_SeekThumbnail(fh) )
    {
        SITHLOG_ERROR("RESTORE: seekThumbNail failed!\n");
        goto error;
    }

    // Close current level
    if ( sithWorld_g_pCurrentWorld && streqi(sithWorld_g_pCurrentWorld->aName, header.aLevelFilename) ) // I guess when loading in game the same level.
    {
        // Wonder why this is good when current level is closed anyway.
        // Oh right, sithSound_FreeWorldSounds won't free sounds when we set here to skip restoring sounds.
        sithSound_SetSkipRestoringSounds(1);
    }

    if ( sithWorld_g_pCurrentWorld )
    {
        sithClose();
    }

    // Load level specified in header
    if ( sithOpenNormal(header.aLevelFilename, L"Indiana Jones") ) // Note, when game is started the name is set to computer name
    {
        SITHLOG_ERROR("Restore: sithOpenNormal failed!\n");
        goto error;
    }

    // Copy game statistics from header 
    sithGamesave_gameStatistics = header.gameStatistics;

    // Copy the state of COG global symbols from header
    for ( size_t i = 0; i < STD_ARRAYLEN(header.aCogGlobalValues); ++i )
    {
        char aText[64];
        STD_FORMAT(aText, "%s%d", "global", i);

        SithCogSymbol* pSymbol = sithCogParse_GetSymbol(sithCog_g_pSymbolTable, aText);
        if ( pSymbol )
        {
            pSymbol->value = header.aCogGlobalValues[i];
        }
    }

    // Set local player number
    sithPlayer_g_playerNum = header.localPlayerNum;

    // Reset system and clear all things
    sithAnimate_Reset();
    sithEvent_Reset();
    sithFX_ClearChalkMarks();
    sithThing_RemoveWorldThings(sithWorld_g_pCurrentWorld); // TODO: This will remove all things which moveType was changed via thing arg but is not preserved in savegame file; Fix this

    // Parse & restore gamesave file sections
    int hresult = sithGamesave_ReadBlockTypeLength(&sithMulti_g_message.type, &sithMulti_g_message.length);
    while ( !hresult )
    {
        if ( sithMulti_g_message.length && !stdConffile_Read(sithMulti_g_message.data, sithMulti_g_message.length) )
        {
            SITHLOG_ERROR("stdConfile_Read failed, last type %d.\n", sithMulti_g_message.type);
            goto error;
        }

        if ( !sithMessage_Process(&sithMulti_g_message) )
        {
            SITHLOG_ERROR("sithMessage_Process() failed for DSS message type %d.\n", sithMulti_g_message.type);
            goto error;
        }

        hresult = sithGamesave_ReadBlockTypeLength(&sithMulti_g_message.type, &sithMulti_g_message.length);
        if ( hresult && hresult != SITHSAVEGAME_ENDFILE ) // Reading error
        {
            SITHLOG_ERROR("ReadBlockTypeLength failed, type %d.\n", sithMulti_g_message.type);
        }
    }

    if ( hresult != SITHSAVEGAME_ENDFILE )
    {
        SITHLOG_ERROR("hresult != ENDFILE, last type %d\n", sithMulti_g_message.type);
        goto error;
    }

    // Restore sound mixer state from savegame file
    if ( sithSoundMixer_GameRestore(fh) )
    {
        SITHLOG_ERROR("RESTORE: sithSoundMixer_GameRestore failed!\n");
        goto error;
    }

    // Restore weapon system state from savegame file
    if ( sithWeapon_Restore(fh) )
    {
        SITHLOG_ERROR("RESTORE: sithWeapon_Restore failed!\n");
        return 1;
    }

    // Load post process
    sithThing_LoadPostProcess(sithWorld_g_pCurrentWorld);
    sithPlayer_SetLocalPlayer(header.localPlayerNum);

    // Close savegame file
    stdConffile_Close();
    fh = 0;

    STD_STRCPY(sithGamesave_aLastFilename, pFilename);

    // Fix COG links to things
    // Altered: Change log level to status from error
    SITHLOG_STATUS("RESTORE: calling sithCog_FixupLinksToThings\n");
    sithCog_FixupLinksToThings();

    // Set game time from savegame file
    // Altered: Change log level to status from error
    sithTime_SetGameTime(header.msecGameTime);
    SITHLOG_STATUS("RESTORE: returning SUCCESS!\n");

    // Fixed: Added check for ineditor flag not being set.
    //        This fixes restoring savegame that was saved under ineditor flag to not play intro cutscene
    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        if ( bNotify == 1 )
        {
            sithGamesave_NotifyRestored(pFilename);
        }
    }

    return 0; // Restore succeeded

error:
    stdConffile_Close();
    sithClose();
    SITHLOG_ERROR("Savegame parsing failed for %s.\n", pFilename);
    return 1;
}

void J3DAPI sithGamesave_NotifyRestored(const char* pFilePath)
{
    // If restoring from autosave i.e.: savegame at the start of a level (start_xxx.nds), notify master cog
    const char* pPrefix   = sithGetAutoSaveFilePrefix();
    const char* pFilename = stdFnames_FindMedName(pFilePath);
    if ( strneq(pFilename, pPrefix, strlen(pPrefix)) )
    {
        if ( sithCog_g_pMasterCog )
        {
            sithCog_SendMessage(sithCog_g_pMasterCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }
}

int J3DAPI sithGamesave_ReadBlockTypeLength(uint16_t* pType, uint32_t* pLength)
{
    if ( !stdConffile_Read(pType, sizeof(uint16_t)) || !stdConffile_Read(pLength, sizeof(uint32_t)) )
    {
        *pType   = SITHDSS_UNKNOWN_44;
        *pLength = 0;
        return 1; // error
    }
    else if ( *pType == SITHDSS_UNKNOWN_44 )
    {
        return SITHSAVEGAME_ENDFILE; // end of savegame file section
    }

    return 0; // success
}

int J3DAPI sithGamesave_SaveFile(const char* pFilename)
{
    SITH_ASSERTREL(sithPlayer_g_pLocalPlayerThing);

    // Don't make savegame file when player is dying
    if ( (sithPlayer_g_pLocalPlayerThing->flags & SITH_TF_DYING) != 0 )
    {
        return 1;
    }

    memset(sithGamesave_aNdsSaveSectionSizes, 0, sizeof(sithGamesave_aNdsSaveSectionSizes));

    if ( !stdConffile_OpenWrite(pFilename) )
    {
        SITHLOG_ERROR("Error opening file %s for writing.\n", pFilename);
        return 1;
    }

    // Fixed: Originally, the file handle was not set to the file handle of the opened write file but the error status stdConffile_OpenWrite returned
    tFileHandle fh = stdConffile_GetWriteFileHandle();

    // Change output stream to file
    SithMessageStream curStream = sithMessage_g_outputstream;
    sithMessage_g_outputstream  = SITHMESSAGE_STREAM_FILE;

    // Init NDS file header
    NdsHeader header = { 0 };
    header.version = SITHSAVEGAME_FILEVERSION;
    STD_STRCPY(header.aDate, "Oct 28 1999"); // TODO: use current date
    STD_STRCPY(header.aLevelFilename, sithWorld_g_pCurrentWorld->aName);
    STD_STRCPY(header.aPreviousLevelFilename, sithGamesave_aPrevLevelFilename);

    header.msecGameTime   = sithTime_g_msecGameTime;
    header.gameStatistics = sithGamesave_gameStatistics;
    header.bThumbnail     = sithGamesave_bThumbnail;
    header.perflevel      = sithGetPerformanceLevel();

    // Copy COG global symbols to header
    for ( size_t i = 0; i < STD_ARRAYLEN(header.aCogGlobalValues); ++i )
    {
        char aGlobalSym[64];
        STD_FORMAT(aGlobalSym, "%s%d", "global", i);

        SithCogSymbol* pCogSym = sithCogParse_GetSymbol(sithCog_g_pSymbolTable, aGlobalSym);
        if ( pCogSym )
        {
            header.aCogGlobalValues[i] = pCogSym->value;
            if ( pCogSym->value.type >= SITHCOG_VALUE_POINTER && (pCogSym->value.type <= SITHCOG_VALUE_SYMBOLID || pCogSym->value.type == SITHCOG_VALUE_STRING) )
            {
                // For some reason reset symbol if it is a string, pointer or symbol id
                memset(&header.aCogGlobalValues[i].val, 0, sizeof(header.aCogGlobalValues[i].val));
            }
        }
        else
        {
            memset(&header.aCogGlobalValues[i], 0, sizeof(header.aCogGlobalValues[i]));
        }
    }

    header.localPlayerNum = sithPlayer_g_playerNum;

    // Write header to file
    int bError = stdConffile_Write(&header, sizeof(NdsHeader));
    if ( bError )
    {
        return 1;
    }

    sithGamesave_aNdsSaveSectionSizes[0] = sizeof(NdsHeader);

    // Write thumbnail
    bError = sithGamesave_WriteThumbnail(fh);
    if ( bError )
    {
        return 1;
    }

    // Save world state to file
    bError = sithGamesave_SaveCurrentWorld(SITHMESSAGE_STREAM_FILE);
    if ( bError )
    {
        return 1;
    }

    // Write sound mixer state
    bError = sithSoundMixer_GameSave(fh);
    if ( bError )
    {
        return 1;
    }

    // Write weapon system state to file
    bError = sithWeapon_Save(fh);
    if ( bError )
    {
        return 1;
    }

    // Writing savegame succeeded
    stdConffile_CloseWrite();
    fh = 0;

    STD_STRCPY(sithGamesave_aLastFilename, pFilename);

    // Print game saved to console
    // Note, the "GAME_SAVED" string does not exist, might be just a remnant of the original sith engine
    const wchar_t* pText = sithString_GetString("GAME_SAVED");
    sithConsole_PrintWString(pText);

    sithMessage_g_outputstream = curStream;
    return 0; // success
}

void J3DAPI sithGamesave_SetThumbnailImage(tVBuffer* pVBuffer)
{
    if ( pVBuffer )
    {
        if ( sithGamesave_pThumbnailImage )
        {
            stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
            sithGamesave_pThumbnailImage = NULL;
        }

        if ( sithGamesave_hBmpThumbnail )
        {
            DeleteObject(sithGamesave_hBmpThumbnail);
            sithGamesave_hBmpThumbnail = NULL;
        }

        // Clone the thumbnail image to sithGamesave_pThumbnailImage
        tRasterInfo rasterInfo = pVBuffer->rasterInfo;
        sithGamesave_pThumbnailImage = stdDisplay_VBufferNew(&rasterInfo, /*bUseVSurface=*/0, /*bUseVideoMemory=*/0);
        if ( sithGamesave_pThumbnailImage )
        {
            size_t pixelSize = sithGamesave_pThumbnailImage->rasterInfo.rowSize / (unsigned int)sithGamesave_pThumbnailImage->rasterInfo.rowWidth;
            stdDisplay_VBufferLock(pVBuffer);
            for ( size_t i = 0; i < rasterInfo.height; ++i )
            {
                memcpy(
                    &sithGamesave_pThumbnailImage->pPixels[sithGamesave_pThumbnailImage->rasterInfo.rowSize * i],
                    &pVBuffer->pPixels[pVBuffer->rasterInfo.rowSize * i],
                    pixelSize * pVBuffer->rasterInfo.width
                );
            }

            stdDisplay_VBufferUnlock(pVBuffer);
        }
    }
}

HBITMAP sithGamesave_CreateThumbnail(void)
{
    HDC hdc       = NULL;
    HDC hdcThumb  = NULL;
    HBITMAP hBmp  = NULL;
    void* pPixmap = NULL;

    if ( sithGamesave_hBmpThumbnail )
    {
        return sithGamesave_hBmpThumbnail;
    }

    if ( sithGamesave_pThumbnailImage )
    {
        sithGamesave_pThumbnailImage = stdDisplay_VBufferConvertColorFormat(&SITHSAVEGAME_THUMBCOLORFORMAT, sithGamesave_pThumbnailImage, 0, NULL);
        const LONG imageSize = (sithGamesave_pThumbnailImage->rasterInfo.width * sithGamesave_pThumbnailImage->rasterInfo.height * sithGamesave_pThumbnailImage->rasterInfo.colorInfo.bpp) / 8;

        BITMAPINFO bmpInfo = { 0 };
        bmpInfo.bmiHeader.biSize        = sizeof(bmpInfo.bmiHeader);
        bmpInfo.bmiHeader.biWidth       = sithGamesave_pThumbnailImage->rasterInfo.width;
        bmpInfo.bmiHeader.biHeight      = -(int32_t)sithGamesave_pThumbnailImage->rasterInfo.height;
        bmpInfo.bmiHeader.biPlanes      = 1;
        bmpInfo.bmiHeader.biBitCount    = sithGamesave_pThumbnailImage->rasterInfo.colorInfo.bpp;
        bmpInfo.bmiHeader.biCompression = BI_RGB;
        bmpInfo.bmiHeader.biSizeImage   = imageSize;

        hdc = CreateCompatibleDC(NULL);
        if ( !hdc )
        {
            goto error;
        }

        // Create bmp
        hBmp = CreateDIBSection(hdc, &bmpInfo, DIB_RGB_COLORS, &pPixmap, NULL, 0);
        if ( !hBmp )
        {
            goto error;
        }

        // Copy pixels to bmp
        memcpy(pPixmap, sithGamesave_pThumbnailImage->pPixels, bmpInfo.bmiHeader.biSizeImage);

        // Now resize bmp to thumbnail size
        bmpInfo.bmiHeader.biSize        = sizeof(bmpInfo.bmiHeader);
        bmpInfo.bmiHeader.biWidth       = SITHSAVEGAME_THUMB_WIDTH;
        bmpInfo.bmiHeader.biHeight      = -(int32_t)SITHSAVEGAME_THUMB_HEIGHT;
        bmpInfo.bmiHeader.biPlanes      = 1;
        bmpInfo.bmiHeader.biBitCount    = SITHSAVEGAME_THUMBCOLORFORMAT.bpp;
        bmpInfo.bmiHeader.biCompression = BI_RGB;
        bmpInfo.bmiHeader.biSizeImage   = SITHSAVEGAME_THUMBSIZE;
        memset(&bmpInfo.bmiHeader.biXPelsPerMeter, 0, 16);

        hdcThumb = CreateCompatibleDC(NULL);
        if ( !hdcThumb )
        {
            goto error;
        }
        sithGamesave_hBmpThumbnail = CreateDIBSection(hdcThumb, &bmpInfo, 0, &pPixmap, NULL, 0);
        if ( sithGamesave_hBmpThumbnail )
        {
            // Resize bmp
            SelectObject(hdc, hBmp);
            SelectObject(hdcThumb, sithGamesave_hBmpThumbnail);
            SetStretchBltMode(hdcThumb, STRETCH_HALFTONE);

            StretchBlt(
                hdcThumb,
                0,
                0,
                SITHSAVEGAME_THUMB_WIDTH,
                SITHSAVEGAME_THUMB_HEIGHT,
                hdc,
                0,
                0,
                sithGamesave_pThumbnailImage->rasterInfo.width,
                sithGamesave_pThumbnailImage->rasterInfo.height,
                SRCCOPY
            );

            stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
            sithGamesave_pThumbnailImage = NULL;

            DeleteDC(hdc);
            DeleteObject(hBmp);
            DeleteDC(hdcThumb);

            sithGamesave_bThumbnail = 1;
            return sithGamesave_hBmpThumbnail;
        }
    }

error:
    if ( sithGamesave_pThumbnailImage )
    {
        stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
        sithGamesave_pThumbnailImage = NULL;
    }

    if ( hBmp )
    {
        DeleteObject(hBmp);
    }

    if ( hdc )
    {
        DeleteDC(hdc);
    }

    if ( hdcThumb )
    {
        DeleteDC(hdcThumb);
    }

    sithGamesave_bThumbnail = 0;
    return 0;
}

int J3DAPI sithGamesave_WriteThumbnail(tFileHandle fh)
{
    if ( !sithGamesave_bThumbnail )
    {
        return 0;
    }

    BITMAP bmp = { 0 }; // Fixed: Initialized to 0
    if ( !GetObject(sithGamesave_hBmpThumbnail, sizeof(BITMAP), &bmp) )
    {
        return 1;
    }

    const size_t imgSize = bmp.bmWidthBytes * bmp.bmHeight; // Altered: Changed to use bmp.bmWidthBytes instead of calculating (bmp.bmBitsPixel/8 * bmp.bmWidth)
    if ( imgSize != sith_g_pHS->pFileWrite(fh, bmp.bmBits, imgSize) )
    {
        return 1;
    }

    // Success
    sithGamesave_bThumbnail = 0;
    return 0;
}

int J3DAPI sithGamesave_SeekThumbnail(tFileHandle fh)
{
    // Skips thumbnail section
    return sithGamesave_bThumbnail && sith_g_pHS->pFileSeek(fh, SITHSAVEGAME_THUMBSIZE, 1);// 0x9000 - 128 * 96 * 3
}

HBITMAP J3DAPI sithGamesave_LoadThumbnail(const char* pFilename)
{
    HDC hdc = NULL;
    if ( !stdConffile_OpenMode(pFilename, "rb") )
    {
        return 0;
    }

    // Read nds file header
    NdsHeader header;
    if ( !stdConffile_Read(&header, sizeof(NdsHeader)) )
    {
        goto error;
    }

    if ( header.version != SITHSAVEGAME_FILEVERSION )
    {
        goto error;
    }

    if ( !header.bThumbnail )
    {
        goto error;
    }

    // Create bitmap info & DC
    BITMAPINFO bmi              = { 0 }; // Fixed: Initialized to 0
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = SITHSAVEGAME_THUMB_WIDTH;
    bmi.bmiHeader.biHeight      = -SITHSAVEGAME_THUMB_HEIGHT;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = SITHSAVEGAME_THUMBCOLORFORMAT.bpp;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = SITHSAVEGAME_THUMBSIZE;
    //memset(&bmi.bmiHeader.biXPelsPerMeter, 0, 16);

    hdc = CreateCompatibleDC(NULL);
    if ( !hdc )
    {
        goto error;
    }

    // Create bitmap and read thumb pixels
    void* pPixels;
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pPixels, NULL, 0);
    if ( hBitmap && stdConffile_Read(pPixels, SITHSAVEGAME_THUMBSIZE) )
    {
        DeleteDC(hdc);
        stdConffile_Close();
        return hBitmap;
    }

error:
    if ( hdc )
    {
        DeleteDC(hdc);
    }

    stdConffile_Close();
    return NULL;
}

void sithGamesave_Startup(void)
{
    sithGamesave_hBmpThumbnail = NULL;
    sithGamesave_bThumbnail    = 0;
    memset(sithGamesave_aPrevLevelFilename, 0, sizeof(sithGamesave_aPrevLevelFilename));
}

void sithGamesave_Shutdown(void)
{
    sithGamesave_Close();
    memset(sithGamesave_aPrevLevelFilename, 0, sizeof(sithGamesave_aPrevLevelFilename));
}

void sithGamesave_Open(void)
{
    sithGamesave_hBmpThumbnail = NULL;
    sithGamesave_bThumbnail    = 0;
}

void sithGamesave_CloseRestore(void)
{
    if ( sithGamesave_state != SITHGAMESAVE_SAVE )
    {
        sithGamesave_Close();
    }
}

void sithGamesave_Close(void)
{
    if ( sithGamesave_hBmpThumbnail )
    {
        DeleteObject((HGDIOBJ)sithGamesave_hBmpThumbnail);
        sithGamesave_hBmpThumbnail = NULL;
        sithGamesave_bThumbnail    = 0;
    }

    if ( sithGamesave_pThumbnailImage )
    {
        stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
        sithGamesave_pThumbnailImage = NULL;
    }
}

void J3DAPI sithGamesave_SetPreviousLevelFilename(const char* pFilename)
{
    STD_STRCPY(sithGamesave_aPrevLevelFilename, pFilename);
}

const char* sithGamesave_GetPreviousLevelFilename(void)
{
    return sithGamesave_aPrevLevelFilename;
}

const char* sithGamesave_GetLastFilename(void)
{
    return sithGamesave_aLastFilename;
}

int J3DAPI sithGamesave_LoadLevelFilename(const char* pNdsFilePath, char* pDestFilename)
{
    NdsHeader header;
    if ( !stdConffile_OpenMode(pNdsFilePath, "rb") || !stdConffile_Read(&header, sizeof(NdsHeader)) || (stdConffile_Close(), header.version != SITHSAVEGAME_FILEVERSION) )
    {
        stdConffile_Close();
        return 1;
    }

    if ( !pDestFilename )
    {
        return 0;
    }

    // TODO: ensure pDestFilename is large enough
    stdUtil_StringCopy(pDestFilename, 64u, header.aLevelFilename);
    return 0;
}

int J3DAPI sithGamesave_GetState(const char** ppCurFilename)
{
    *ppCurFilename = sithGamesave_aCurFilename;
    return sithGamesave_state;
}

void J3DAPI sithGamesave_SetSaveGameCallback(SithGameSaveCallback pfCallback)
{
    sithGamesave_pfSaveGameCallback = pfCallback;
}

