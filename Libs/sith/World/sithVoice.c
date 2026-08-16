#include "sithVoice.h"
#include <j3dcore/j3dhook.h>

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithString.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithThing.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdFont.h>

#include <sound/Sound.h>

#include <std/General/stdConfig.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>

#define SITHVOICE_MSEC_SUBTITLE_HIDE_DURATION    1000
#define SITHVOICE_MSEC_SUBTITLE_REMOVE_DURATION  SITHVOICE_MSEC_SUBTITLE_HIDE_DURATION / 4
#define SITHVOICE_MSEC_SUBTITLE_REMOVE_THRESHOLD 25.0f
#define SITHVOICE_MAX_LINES_PER_DRAW             3    // number of lines to draw

#define SITHVOICE_TEXT_PADDING_X 24.0f
#define SITHVOICE_REF_WIDTH     (RD_REF_WIDTH * RD_REF_WIDTH)
static_assert(SITHVOICE_REF_WIDTH == 409600.0f, "SITHVOICE_REF_WIDTH == 409600.f");

#define SITHVOICE_TEXT_PADDING_Y 12.0f
#define SITHVOICE_REF_HEIGHT     (RD_REF_HEIGHT * RD_REF_HEIGHT)
static_assert(SITHVOICE_REF_HEIGHT == 230400.0f, "SITHVOICE_REF_WIDTH == 409600.f");

#define SITHVOICE_FONTFILENAME "mat\\jonesCalisto MT20.gcf"

static bool sithVoice_bStartup = false; // Added: Init to 0
static bool sithVoice_bOpen    = false; // Added: Init to 0

static bool sithVoice_bShowText;
static bool sithVoice_bThingHasSwapHead;

static int sithVoice_msecLipSyncOffset = 50; // lip-sync time lookup offset in milliseconds

static int sithVoice_curHeadHeight;
static int sithVoice_lastHeadHeight = -1;
static size_t sithVoice_sameHeadHightCounter;

static float sithVoice_secHeadSwapInterval = 0.1f;
static uint8_t sithVoice_aVoiceHeadHeights[4][4];
static float sithVoice_secNextHeadSwapTime;

static size_t sithVoice_curSubtitleDrawIndex;
static size_t sithVoice_numSubtitleInfos;
static SithVoiceSubtitleInfo sithVoice_aSubtitleInfos[25];

static rdFont* sithVoice_pTextFont;

#if defined(J3D_TEST)
static SithVoiceTestState sithVoice_testState;

static void J3DAPI sithVoice_TestStopChannel(tSoundChannelHandle hChannel)
{
    ++sithVoice_testState.numStopCalls;
    sithVoice_testState.hLastStoppedChannel = hChannel;
}

static int J3DAPI sithVoice_TestGenerateLipSync(tSoundChannelHandle hChannel, uint8_t* pMouthPosX, uint8_t* pMouthPosY, int msecLipSyncOffset)
{
    J3D_UNUSED(hChannel);
    J3D_UNUSED(msecLipSyncOffset);

    *pMouthPosX = sithVoice_testState.mouthPosX;
    *pMouthPosY = sithVoice_testState.mouthPosY;
    return sithVoice_testState.bLipSyncData;
}

static int J3DAPI sithVoice_TestAddSwapEntry(SithThing* pThing, int meshNum, rdModel3* pSrcModel, int meshNumSrc)
{
    ++sithVoice_testState.numAddSwapCalls;
    sithVoice_testState.pLastSwapThing       = pThing;
    sithVoice_testState.pLastAddedModel      = pSrcModel;
    sithVoice_testState.lastAddedMeshNum     = meshNum;
    sithVoice_testState.lastAddedSrcMeshNum  = meshNumSrc;
    return sithVoice_testState.addSwapResult;
}

static int J3DAPI sithVoice_TestRemoveSwapEntry(SithThing* pThing, int refNum)
{
    ++sithVoice_testState.numRemoveSwapCalls;
    sithVoice_testState.pLastSwapThing    = pThing;
    sithVoice_testState.lastRemovedRefNum = refNum;
    return 0;
}

// Added: Test seams expand to the original calls in non-test builds.
#   define SITHVOICE_STOP_CHANNEL(hChannel) \
        sithVoice_TestStopChannel((hChannel))
#   define SITHVOICE_GENERATE_LIP_SYNC(hChannel, pMouthPosX, pMouthPosY, msecLipSyncOffset) \
        sithVoice_TestGenerateLipSync((hChannel), (pMouthPosX), (pMouthPosY), (msecLipSyncOffset))
#   define SITHVOICE_ADD_SWAP_ENTRY(pThing, meshNum, pSrcModel, meshNumSrc) \
        sithVoice_TestAddSwapEntry((pThing), (meshNum), (pSrcModel), (meshNumSrc))
#   define SITHVOICE_REMOVE_SWAP_ENTRY(pThing, refNum) \
        sithVoice_TestRemoveSwapEntry((pThing), (refNum))
#   define SITHVOICE_SEC_GAME_TIME sithVoice_testState.secGameTime
#else
#   define SITHVOICE_STOP_CHANNEL(hChannel) \
        Sound_StopChannel((hChannel))
#   define SITHVOICE_GENERATE_LIP_SYNC(hChannel, pMouthPosX, pMouthPosY, msecLipSyncOffset) \
        Sound_GenerateLipSync((hChannel), (pMouthPosX), (pMouthPosY), (msecLipSyncOffset))
#   define SITHVOICE_ADD_SWAP_ENTRY(pThing, meshNum, pSrcModel, meshNumSrc) \
        sithThing_AddSwapEntry((pThing), (meshNum), (pSrcModel), (meshNumSrc))
#   define SITHVOICE_REMOVE_SWAP_ENTRY(pThing, refNum) \
        sithThing_RemoveSwapEntry((pThing), (refNum))
#   define SITHVOICE_SEC_GAME_TIME sithTime_g_secGameTime
#endif

void J3DAPI sithVoice_PlayVoice(SithCog* pCog);
void J3DAPI sithVoice_SetThingVoiceHeads(SithCog* pCog);
void J3DAPI sithVoice_SetVoiceHeadHeight(SithCog* pCog);
void J3DAPI sithVoice_SetThingVoiceColor(SithCog* pCog);
void J3DAPI sithVoice_SetVoiceParams(SithCog* pCog);

void J3DAPI sithVoice_AddSubtitle(unsigned int msecSoundLen, const char* pVoiceSoundFileName, const char* pVoiceSubtitles, VGradiantColor* pVoiceSubtitleColor);
void J3DAPI sithVoice_RemoveSoundSubtitle(tSoundChannelHandle hSound); // Added
void sithVoice_PurgeDrawnSubtitles(void);

#if defined(J3D_TEST)
void sithVoice_TestResetState(void)
{
    sithVoice_bThingHasSwapHead     = false;
    sithVoice_msecLipSyncOffset     = 50;
    sithVoice_curHeadHeight         = 0;
    sithVoice_lastHeadHeight        = -1;
    sithVoice_sameHeadHightCounter  = 0;
    sithVoice_secHeadSwapInterval   = 0.1f;
    sithVoice_secNextHeadSwapTime   = 0.0f;
    STD_ZEROMEM(sithVoice_aVoiceHeadHeights, sizeof(sithVoice_aVoiceHeadHeights));

    STD_ZEROMEM(&sithVoice_testState, sizeof(sithVoice_testState));
    sithVoice_testState.addSwapResult       = 101;
    sithVoice_testState.lastAddedMeshNum    = -1;
    sithVoice_testState.lastAddedSrcMeshNum = -1;
    sithVoice_testState.lastRemovedRefNum   = -1;
}

void sithVoice_TestSetLipSyncResult(int bLipSyncData, uint8_t mouthPosX, uint8_t mouthPosY)
{
    sithVoice_testState.bLipSyncData = bLipSyncData;
    sithVoice_testState.mouthPosX    = mouthPosX;
    sithVoice_testState.mouthPosY    = mouthPosY;
}

void sithVoice_TestSetGameTime(float secGameTime)
{
    sithVoice_testState.secGameTime = secGameTime;
}

void sithVoice_TestSetHeadTableEntry(size_t mouthYLevel, size_t mouthXLevel, uint8_t headSlot)
{
    SITH_ASSERTREL(mouthYLevel < STD_ARRAYLEN(sithVoice_aVoiceHeadHeights));
    SITH_ASSERTREL(mouthXLevel < STD_ARRAYLEN(sithVoice_aVoiceHeadHeights[mouthYLevel]));
    SITH_ASSERTREL(headSlot < STD_ARRAYLEN(sithVoice_aVoiceHeadHeights));

    sithVoice_aVoiceHeadHeights[mouthYLevel][mouthXLevel] = headSlot;
}

void sithVoice_TestSetSelectionState(int lastHeadSlot, size_t sameHeadCounter, float secNextHeadSwapTime)
{
    sithVoice_lastHeadHeight        = lastHeadSlot;
    sithVoice_sameHeadHightCounter  = sameHeadCounter;
    sithVoice_secNextHeadSwapTime   = secNextHeadSwapTime;
}

void sithVoice_TestSetThingHasSwapHead(int bThingHasSwapHead)
{
    sithVoice_bThingHasSwapHead = bThingHasSwapHead != 0;
}

void sithVoice_TestGetState(SithVoiceTestState* pState)
{
    *pState = sithVoice_testState;

    pState->bThingHasSwapHead     = sithVoice_bThingHasSwapHead;
    pState->curHeadSlot           = sithVoice_curHeadHeight;
    pState->lastHeadSlot          = sithVoice_lastHeadHeight;
    pState->sameHeadCounter       = sithVoice_sameHeadHightCounter;
    pState->secNextHeadSwapTime   = sithVoice_secNextHeadSwapTime;
}
#endif

void sithVoice_InstallHooks(void)
{
    J3D_HOOKFUNC(sithVoice_Startup);
    J3D_HOOKFUNC(sithVoice_Open);
    J3D_HOOKFUNC(sithVoice_Shutdown);
    J3D_HOOKFUNC(sithVoice_Close);
    J3D_HOOKFUNC(sithVoice_RegisterCogFunctions);
    J3D_HOOKFUNC(sithVoice_PlayThingVoice);
    J3D_HOOKFUNC(sithVoice_UpdateLipSync);
    J3D_HOOKFUNC(sithVoice_PlayVoice);
    J3D_HOOKFUNC(sithVoice_SetThingVoiceHeads);
    J3D_HOOKFUNC(sithVoice_SetVoiceHeadHeight);
    J3D_HOOKFUNC(sithVoice_SetThingVoiceColor);
    J3D_HOOKFUNC(sithVoice_SetVoiceParams);
    J3D_HOOKFUNC(sithVoice_GetShowText);
    J3D_HOOKFUNC(sithVoice_ShowText);
    J3D_HOOKFUNC(sithVoice_AddSubtitle);
    J3D_HOOKFUNC(sithVoice_PurgeDrawnSubtitles);
    J3D_HOOKFUNC(sithVoice_Draw);
    J3D_HOOKFUNC(sithVoice_SyncVoiceState);
    J3D_HOOKFUNC(sithVoice_ProcessVoiceState);
    J3D_HOOKFUNC(sithVoice_GetTextFont);
}

void sithVoice_ResetGlobals(void)
{}

int sithVoice_Startup(void)
{
    // Added:
    if ( sithVoice_bStartup )
    {
        // TODO: Maybe add log
        return 0;
    }

    sithVoice_pTextFont = NULL;
    sithVoice_bShowText = stdConfig_GetBool(SITHVOICE_CFG_GAMEPLAY_SHOWTEXT, false);
    sithVoice_bStartup  = true;
    return 1;
}

void sithVoice_Shutdown(void)
{
    // Added:
    if ( !sithVoice_bStartup )
    {
        // TODO: Maybe add log
        return;
    }

    sithVoice_Close(); // Added: Make sure the system was closed which will also release any allocated resources (prevents memory leak)
    sithVoice_pTextFont = NULL;
    sithVoice_bStartup  = false;
}

int sithVoice_Open(void)
{
    // Added:
    if ( sithVoice_bOpen )
    {
        // TODO: Maybe add log
        return 0;
    }

    sithVoice_pTextFont = rdFont_Load(SITHVOICE_FONTFILENAME);
    if ( !sithVoice_pTextFont )
    {
        sithVoice_Close();
        sithVoice_bOpen = false;
        return 0;
    }

    STD_ZEROMEM(sithVoice_aSubtitleInfos, sizeof(sithVoice_aSubtitleInfos));
    sithVoice_curSubtitleDrawIndex = 0;
    sithVoice_numSubtitleInfos   = 0;

    sithVoice_bThingHasSwapHead = false; // Added: Init to false
    sithVoice_bOpen = true;
    return 1;
}

void sithVoice_Close(void)
{
    // Added:
    if ( !sithVoice_bOpen )
    {
        // TODO: Maybe add log
        return;
    }

    if ( sithVoice_pTextFont )
    {
        rdFont_Free(sithVoice_pTextFont);
        sithVoice_pTextFont = NULL;
    }

    STD_ZEROMEM(sithVoice_aSubtitleInfos, sizeof(sithVoice_aSubtitleInfos));
    sithVoice_curSubtitleDrawIndex = 0;
    sithVoice_numSubtitleInfos     = 0;
    sithVoice_bOpen                = false;
}

int J3DAPI sithVoice_RegisterCogFunctions(SithCogSymbolTable* pTable)
{
    return sithCog_RegisterFunction(pTable, sithVoice_PlayVoice, "playvoice")
        || sithCog_RegisterFunction(pTable, sithVoice_SetThingVoiceHeads, "setthingvoiceheads")
        || sithCog_RegisterFunction(pTable, sithVoice_SetThingVoiceColor, "setthingvoicecolor")
        || sithCog_RegisterFunction(pTable, sithVoice_SetVoiceParams, "setvoiceparams")
        || sithCog_RegisterFunction(pTable, sithVoice_SetVoiceHeadHeight, "setvoiceheadheight");
}

int J3DAPI sithVoice_PlayThingVoice(SithThing* pThing, tSoundHandle hSnd, float volume)
{
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    SithActorVoiceInfo* pVoiceInfo = &pThing->thingInfo.actorInfo.voiceInfo;
    tSoundChannelHandle hSndChannel = pVoiceInfo->hSndChannel;
    if ( hSndChannel )
    {
        sithVoice_RemoveSoundSubtitle(hSndChannel); // Added: remove subtitle of a sound from pending array
        sithSoundMixer_StopSound(hSndChannel);
    }

    pVoiceInfo->voiceHeadInfo.headSwapRefNum = -1;
    sithVoice_secNextHeadSwapTime = 0.0f;

    if ( pThing->pSwapList )
    {
        for ( SithThingSwapEntry* pSwapEntry = pThing->pSwapList; pSwapEntry; pSwapEntry = pSwapEntry->pNextEntry )
        {
            if ( strneq(pSwapEntry->pSrcModel->aName, "head", 4u) )
            {
                sithVoice_bThingHasSwapHead = true;
                break;
            }
        }
    }

    // Set voice line subtitle text
    const char* pFilename = Sound_GetSoundFilename(hSnd);
    if ( pFilename )
    {
        char aFilename[64];
        STD_STRCPY(aFilename, pFilename);
        stdUtil_ToUpper(aFilename);
        // _strupr(aFilename);

        wchar_t* pwVoiceLine = sithString_GetString(aFilename);
        if ( pwVoiceLine )
        {
            char* pVoiceLine = stdUtil_ToAString(pwVoiceLine);
            if ( pVoiceLine )
            {
                unsigned int sndLength = Sound_GetLengthMsec(hSnd);
                sithVoice_AddSubtitle(sndLength, aFilename, pVoiceLine, &pVoiceInfo->voiceColor);
                stdMemory_Free(pVoiceLine);
            }
        }
    }

    // Reset head swap stuff as the lip sync works per single speaker only
    sithVoice_lastHeadHeight       = -1;
    sithVoice_sameHeadHightCounter = 0;

    // Play thing voice line
    pVoiceInfo->hSndChannel = sithSoundMixer_PlaySound(hSnd, volume, 0.0f, SOUNDPLAY_PLAYONCE | SOUNDPLAY_THING_POS);
    return pVoiceInfo->hSndChannel == SOUND_INVALIDHANDLE;
}

void J3DAPI sithVoice_UpdateLipSync(SithThing* pThing)
{
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));
    STD_GUARD_VOID((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR)); // Added: Guard to prevent handling non-actor/player things

    SithActorVoiceInfo* pVoiceInfo = &pThing->thingInfo.actorInfo.voiceInfo;
    if ( pVoiceInfo->hSndChannel != SOUND_INVALIDHANDLE )
    {
        if ( (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
        {
            SITHVOICE_STOP_CHANNEL(pVoiceInfo->hSndChannel);
        }

        uint8_t mouthPosX, mouthPosY;
        int bSyncData = SITHVOICE_GENERATE_LIP_SYNC(pVoiceInfo->hSndChannel, &mouthPosX, &mouthPosY, sithVoice_msecLipSyncOffset);
        if ( bSyncData )
        {
            if ( sithVoice_secNextHeadSwapTime < (double)SITHVOICE_SEC_GAME_TIME )
            {
                // Note: Sound returns four levels from each seven-bit AudioLib mouth coordinate. The shipped COG setup
                //       repeats each row value across all X columns, so mouth Y alone selects the M, A, AM, or O head.
                size_t mouthYLevel = SOUND_LIPSYNC_GETMOUTHLEVEL(mouthPosY);
                size_t mouthXLevel = SOUND_LIPSYNC_GETMOUTHLEVEL(mouthPosX);
                sithVoice_curHeadHeight = sithVoice_aVoiceHeadHeights[mouthYLevel][mouthXLevel];
                if ( sithVoice_curHeadHeight == sithVoice_lastHeadHeight )
                {
                    if ( ++sithVoice_sameHeadHightCounter >= 2 )
                    {
                        // Note: Repeated nonzero poses are varied deterministically. Head 0 represents the closed-mouth
                        //       M pose and is intentionally left unchanged so quiet or sustained M sounds keep closed lips.
                        switch ( sithVoice_curHeadHeight )
                        {
                            case 1:
                                sithVoice_curHeadHeight = 3;
                                break;

                            case 2:
                                sithVoice_curHeadHeight = 0;
                                break;

                            case 3:
                                sithVoice_curHeadHeight = 1;
                                break;

                            default:
                                break;
                        }

                        sithVoice_sameHeadHightCounter = 0;
                    }
                }
                else
                {
                    sithVoice_sameHeadHightCounter = 0;
                }

                sithVoice_lastHeadHeight = sithVoice_curHeadHeight;
                if ( pVoiceInfo->voiceHeadInfo.apSoundHeadModels[sithVoice_curHeadHeight] )
                {
                    int refNum = SITHVOICE_ADD_SWAP_ENTRY(pThing, pVoiceInfo->voiceHeadInfo.headMeshNum, pVoiceInfo->voiceHeadInfo.apSoundHeadModels[sithVoice_curHeadHeight], 0);
                    pVoiceInfo->voiceHeadInfo.headSwapRefNum = refNum;
                }
                else if ( pVoiceInfo->voiceHeadInfo.headSwapRefNum != -1 )
                {
                    SITHVOICE_REMOVE_SWAP_ENTRY(pThing, pVoiceInfo->voiceHeadInfo.headSwapRefNum);
                    pVoiceInfo->voiceHeadInfo.headSwapRefNum = -1;
                }

                sithVoice_secNextHeadSwapTime = SITHVOICE_SEC_GAME_TIME + sithVoice_secHeadSwapInterval;
            }
        }
        else
        {
            if ( pVoiceInfo->voiceHeadInfo.headSwapRefNum != -1 )
            {
                if ( sithVoice_bThingHasSwapHead && pVoiceInfo->voiceHeadInfo.apSoundHeadModels[0] ) // TODO: Note that sithVoice_bThingHasSwapHead might be set for other thing than the one passed to this function
                {
                    sithVoice_bThingHasSwapHead = false;
                    SITHVOICE_ADD_SWAP_ENTRY(pThing, pVoiceInfo->voiceHeadInfo.headMeshNum, pVoiceInfo->voiceHeadInfo.apSoundHeadModels[0], 0);
                }
                else
                {
                    SITHVOICE_REMOVE_SWAP_ENTRY(pThing, pVoiceInfo->voiceHeadInfo.headSwapRefNum);
                }

                pVoiceInfo->voiceHeadInfo.headSwapRefNum = -1;
            }

            pVoiceInfo->hSndChannel = SOUND_INVALIDHANDLE;
            sithVoice_secNextHeadSwapTime = 0.0f;
        }
    }
}

void J3DAPI sithVoice_PlayVoice(SithCog* pCog)
{
    int  bWait        = sithCogExec_PopInt(pCog);
    float volume      = sithCogExec_PopFlex(pCog);
    tSoundHandle hSnd = sithCogExec_PopSound(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !hSnd )
    {
        SITHLOG_ERROR("Cog %s: Bad params passed to PlayVoice().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return; // Fixed: Added missing return
    }

    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));
    SITH_ASSERTREL(pThing->renderData.data.pModel3 != NULL);

    int guid = -1; // Fixed: Init to -1;
    if ( streqi(pThing->renderData.data.pModel3->aName, "aet_gy.3do") )
    {
        tSoundChannelHandle hChannel = sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, SOUNDPLAY_PLAYTHINGONCE | SOUNDPLAY_THING_POS);
        if ( hChannel )
        {
            guid = Sound_GetChannelGUID(hChannel);
        }
    }
    else
    {
        if ( !sithVoice_PlayThingVoice(pThing, hSnd, volume) )
        {
            if ( pThing->thingInfo.actorInfo.voiceInfo.hSndChannel )
            {
                guid = Sound_GetChannelGUID(pThing->thingInfo.actorInfo.voiceInfo.hSndChannel);
            }
        }
    }

    if ( guid == -1 )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( bWait )
    {
        pCog->status = SITHCOG_STATUS_WAITING_SOUND_TO_STOP;
        pCog->statusParams[0] = guid;
    }

    sithCogExec_PushInt(pCog, guid);
}

void J3DAPI sithVoice_SetThingVoiceHeads(SithCog* pCog)
{
    const char* aModelNames[4];
    for ( int i = 3; i >= 0; --i )
    {
        aModelNames[i] = sithCogExec_PopString(pCog); // Head Model Names
    }

    const char* pMeshName = sithCogExec_PopString(pCog); // thing model head mesh name
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));
        for ( size_t i = 0; i < STD_ARRAYLEN(pThing->thingInfo.actorInfo.voiceInfo.voiceHeadInfo.apSoundHeadModels); ++i )
        {
            pThing->thingInfo.actorInfo.voiceInfo.voiceHeadInfo.apSoundHeadModels[i] = sithModel_GetModel(aModelNames[i]);
        }

        pThing->thingInfo.actorInfo.voiceInfo.voiceHeadInfo.headMeshNum = sithThing_GetThingMeshIndex(pThing, pMeshName);
    }
}

void J3DAPI sithVoice_SetVoiceHeadHeight(SithCog* pCog)
{
    size_t aHeights[4];
    for ( int i = 3; i >= 0; --i )
    {
        aHeights[i] = sithCogExec_PopInt(pCog);
        if ( aHeights[i] > 3 )
        {
            aHeights[i] = 3;
        }
    }

    size_t idx = sithCogExec_PopInt(pCog);
    if ( idx > 3 )
    {
        idx = 3;
    }

    for ( size_t i = 0; i < 4; ++i )
    {
        sithVoice_aVoiceHeadHeights[idx][i] = (uint8_t)aHeights[i]; // It should be ok to cast since it's clamped to 3
    }
}

void J3DAPI sithVoice_SetThingVoiceColor(SithCog* pCog)
{
    rdVector3 aColors[4];
    for ( int i = 3; i >= 0; --i )
    {
        sithCogExec_PopVector(pCog, &aColors[i]);
    }

    SithThing* pThing = sithCogExec_PopThing(pCog);
    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR)));

    SithActorVoiceInfo* pVoiceInfo = &pThing->thingInfo.actorInfo.voiceInfo;
    for ( size_t i = 0; i < 4; ++i )
    {
        rdVector_Set4(&pVoiceInfo->voiceColor.top + i, aColors[i].red, aColors[i].green, aColors[i].blue, 1.0f);
    }
}

void J3DAPI sithVoice_SetVoiceParams(SithCog* pCog)
{
    sithVoice_msecLipSyncOffset = sithCogExec_PopInt(pCog);
    sithVoice_secHeadSwapInterval = sithCogExec_PopFlex(pCog);
}

int sithVoice_GetShowText(void)
{
    // TODO: refactor function to return bool
    return (int)sithVoice_bShowText;
}

void J3DAPI sithVoice_ShowText(int bShow)
{
    // TODO: refactor function to accept bool
    sithVoice_bShowText = bShow != 0;
}

void J3DAPI sithVoice_AddSubtitle(unsigned int msecSoundLen, const char* pSoundFilename, const char* pSubtitleText, VGradiantColor* pSubtitleColor)
{
    const char* pCurLine = pSubtitleText;

    if ( pSubtitleText && pSoundFilename && sithVoice_bShowText )
    {
        if ( sithVoice_numSubtitleInfos > 0 && streq(pSoundFilename, sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos - 1].aSoundFilename) )
        {
            size_t numSubtitles = sithVoice_numSubtitleInfos;

            char aCurVoiceLine[256] = { 0 };
            char aVoiceLines[256]   = { 0 };

            unsigned int curTime = stdPlatform_GetTimeMsec();
            while ( sithVoice_numSubtitleInfos > 0
                && streq(pSoundFilename, sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos - 1].aSoundFilename) // Fixed: swap numInfo check and sound filename check
                && curTime < sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos - 1].msecEndTime )
            {
                if ( strlen(aCurVoiceLine) == 0 )
                {
                    STD_STRCPY(aCurVoiceLine, sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos - 1].aSubtitleText);
                }

                STD_STRCAT(aVoiceLines, aCurVoiceLine);
                STD_ZEROMEM(aCurVoiceLine, sizeof(aCurVoiceLine));

                --sithVoice_numSubtitleInfos;
            }

            if ( streq(aVoiceLines, pSubtitleText) )
            {
                unsigned int msecStartTime = stdPlatform_GetTimeMsec();
                while ( sithVoice_numSubtitleInfos < numSubtitles )
                {
                    SithVoiceSubtitleInfo* pInfo = &sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos];
                    unsigned int msecDuration = pInfo->msecEndTime - pInfo->msecStartTime;
                    pInfo->msecStartTime   = msecStartTime;
                    pInfo->msecEndTime     = msecDuration + msecStartTime;
                    pInfo->msecShowEndTime = pInfo->msecEndTime + SITHVOICE_MSEC_SUBTITLE_HIDE_DURATION;
                    ++sithVoice_numSubtitleInfos;
                }

                return;
            }

            sithVoice_numSubtitleInfos = numSubtitles;
        }

        size_t numProcessedLines = 0; // TODO: Kind of useless var
        unsigned int msecEndTime = 0;
        size_t numLines          = 0; // Fixed: Init to 0

        // Changed: Moved the calculation of textWidthScalar outside while loop
        uint32_t width, height;
        stdDisplay_GetBackBufferSize(&width, &height);

        float fontWidth       = 1.0f - (float)((double)width * SITHVOICE_TEXT_PADDING_X / SITHVOICE_REF_WIDTH);
        float textWidthScalar = 2.0f * fontWidth - 1.0f;

        while ( pCurLine )
        {
            unsigned int msecStartTime = msecEndTime;
            if ( pCurLine == pSubtitleText )
            {
                rdFont_GetTextWidth(pSubtitleText, sithVoice_pTextFont); // TODO: ??

                numLines  = 0;

                const char* pText = pCurLine;
                while ( pText )
                {
                    const char* pInText = pText;
                    pText = rdFont_GetWrapLine(pInText, sithVoice_pTextFont, textWidthScalar);
                    ++numLines;

                    // Fixed: Fixed infinitive loop bay adding check for case where text contain word which is too long to fit onto the screen.
                    //        In this case pInText is returned which leads to infinitive loop.
                    if ( pText == pInText )
                    {
                        pText = NULL;
                    }
                }

                msecStartTime = stdPlatform_GetTimeMsec();
            }

            msecEndTime = msecStartTime + (msecSoundLen / numLines);

            ++numProcessedLines;

            const char* pLineEnd  = rdFont_GetWrapLine(pCurLine, sithVoice_pTextFont, textWidthScalar);
            if ( pLineEnd == pCurLine )
            {
                // Fixed: If line contains long word at the end and cannot fit onto the screen the function returns pCurLine, in this case pLineEnd should be NULL.
                pLineEnd = NULL;
            }

            if ( sithVoice_numSubtitleInfos >= STD_ARRAYLEN(sithVoice_aSubtitleInfos) ) // Fixed: Make sure the sithVoice_numSubtitleInfos is not greater than STD_ARRAYLEN(sithVoice_aSubtitleInfos)
            {
                sithVoice_PurgeDrawnSubtitles(); // resets to STD_ARRAYLEN(sithVoice_aSubtitleInfos) - sithVoice_curSubtitleDrawIndex
            }

            // Fixed: Added OB check to prevent out of bounds write
            if ( sithVoice_numSubtitleInfos >= STD_ARRAYLEN(sithVoice_aSubtitleInfos) )
            {
                SITHLOG_WARNING("sithVoice_AddSubtitle: Ran out of subtitle slots. Dropping subtitle for sound '%s'.\n", pSoundFilename);
                return;
            }

            SithVoiceSubtitleInfo* pInfo = &sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos];

            if ( pLineEnd )
            {
                STD_STRNCPY(pInfo->aSubtitleText, pCurLine, pLineEnd - pCurLine); // Fixed: By using STD_STNRCPY to ensure null termination
                pCurLine = pLineEnd;
            }
            else
            {
                rdFont_GetTextWidth(pCurLine, sithVoice_pTextFont); // TODO: ??
                STD_STRCPY(pInfo->aSubtitleText, pCurLine); // Fixed: By using STD_STNRCPY to ensure null termination
                pCurLine = NULL;
            }

            pInfo->msecStartTime   = msecStartTime;
            pInfo->msecEndTime     = msecEndTime;
            pInfo->msecShowEndTime = pInfo->msecEndTime + SITHVOICE_MSEC_SUBTITLE_HIDE_DURATION;

            STD_STRCPY(pInfo->aSoundFilename, pSoundFilename); // Fixed: By using STD_STRCPY ensures null termination

            if ( pSubtitleColor->top.red == -1.0f )
            {
                rdVector_Set4(&pInfo->aTextColors[0], 1.0f, 0.0f, 0.0f, 1.0f);
                rdVector_Set4(&pInfo->aTextColors[1], 1.0f, 0.0f, 0.0f, 1.0f);
                rdVector_Set4(&pInfo->aTextColors[2], 1.0f, 1.0f, 0.0f, 1.0f);
                rdVector_Set4(&pInfo->aTextColors[3], 1.0f, 1.0f, 0.0f, 1.0f);
            }
            else
            {
                for ( size_t i = 0; i < STD_ARRAYLEN(pInfo->aTextColors); ++i )
                {
                    static_assert(STD_ARRAYLEN(pInfo->aTextColors) == 4, "STD_ARRAYLEN(pInfo->aTextColors) == 4");
                    rdVector_Copy4(&pInfo->aTextColors[i], &pSubtitleColor->top + i);
                }
            }

            ++sithVoice_numSubtitleInfos;
        }
    }
}

void J3DAPI sithVoice_RemoveSoundSubtitle(tSoundChannelHandle hSound)
{
    if ( !hSound )
    {
        return;
    }

    const char* pFilename = Sound_GetSoundFilename(hSound);
    if ( !pFilename )
    {
        return;
    }

    // If sound is not playing or progress is less then threshold remove it immediately,
    // otherwise hide it with fast transition
    bool bRemove = false;
    float progress = 0.0;
    if ( !Sound_GetChannelPlayProgress(hSound, &progress) || progress < SITHVOICE_MSEC_SUBTITLE_REMOVE_THRESHOLD )
    {
        bRemove = true;
    }

    // Find all slots with pFilename
    unsigned int curTime = !bRemove ? stdPlatform_GetTimeMsec() : 0;
    for ( size_t i = sithVoice_curSubtitleDrawIndex; i < sithVoice_numSubtitleInfos; i++ )
    {
        if ( streqi(pFilename, sithVoice_aSubtitleInfos[i].aSoundFilename) )
        {
            if ( bRemove )
            {
                // Note: don't clear aSubtitleText or the list will get corrupted
                //       and draw function won't render anything
                STD_ZEROMEM(sithVoice_aSubtitleInfos[i].aSoundFilename, sizeof(sithVoice_aSubtitleInfos[i].aSoundFilename));
                sithVoice_aSubtitleInfos[i].msecStartTime   = 0;
            }
            sithVoice_aSubtitleInfos[i].msecEndTime     = curTime;
            sithVoice_aSubtitleInfos[i].msecShowEndTime = curTime + SITHVOICE_MSEC_SUBTITLE_REMOVE_DURATION;
        }
    }
}

void sithVoice_PurgeDrawnSubtitles(void)
{
    unsigned int curTime = stdPlatform_GetTimeMsec();

    STD_ZEROMEM(sithVoice_aSubtitleInfos, sizeof(SithVoiceSubtitleInfo) * sithVoice_curSubtitleDrawIndex);
    sithVoice_numSubtitleInfos = 0;

    // Copy range from sithVoice_curSubtitleDrawIndex to end of array to the beginning of the array
    for ( size_t i = sithVoice_curSubtitleDrawIndex; i < STD_ARRAYLEN(sithVoice_aSubtitleInfos); ++i )
    {
        const SithVoiceSubtitleInfo* pSrcInfo = &sithVoice_aSubtitleInfos[i];

        // Fixed: Added check to skip empty subtitle lines and lines which have expired
        if ( strlen(pSrcInfo->aSubtitleText) == 0 ||
            pSrcInfo->msecShowEndTime < curTime )
        {
            continue;
        }

        SithVoiceSubtitleInfo* pDestInfo = &sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos];
        pDestInfo->msecStartTime   = pSrcInfo->msecStartTime;
        pDestInfo->msecEndTime     = pSrcInfo->msecEndTime;
        pDestInfo->msecShowEndTime = pSrcInfo->msecShowEndTime;

        STD_STRCPY(pDestInfo->aSubtitleText, pSrcInfo->aSubtitleText);   // Fixed: By using STD_STRCPY it ensures null termination
        STD_STRCPY(pDestInfo->aSoundFilename, pSrcInfo->aSoundFilename); // Fixed: By using STD_STRCPY it ensures null termination

        for ( size_t j = 0; j < STD_ARRAYLEN(pDestInfo->aTextColors); ++j )
        {
            rdVector_Copy4(&pDestInfo->aTextColors[j], &pSrcInfo->aTextColors[j]);
        }

        ++sithVoice_numSubtitleInfos;
    }

    // Fixed: Fixed clearing of correct part of the array to prevent clearing of valid subtitle lines
    //        Original code used sithVoice_curSubtitleDrawIndex as start index and range calculation which is wrong
    //        as index could be smaller than sithVoice_numSubtitleInfos and could lead to clearing of valid subtitle lines.
    //        In addition this introduced a bug where entry at index would be cleared and nothing would be then drawn to the screen (due to check in sithVoice_Draw)
    if ( sithVoice_numSubtitleInfos < STD_ARRAYLEN(sithVoice_aSubtitleInfos) ) // Also added this check to prevent out of bounds access
    {
        STD_ZEROMEM(&sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos], sizeof(SithVoiceSubtitleInfo) * (STD_ARRAYLEN(sithVoice_aSubtitleInfos) - sithVoice_numSubtitleInfos));
    }

    sithVoice_curSubtitleDrawIndex = 0;
}

void sithVoice_Draw(void)
{
    if ( !sithVoice_bShowText )
    {
        return;
    }

    unsigned int curTime = stdPlatform_GetTimeMsec();

    bool bDraw = true;

    // Skip lines that expired
    while ( (strlen(sithVoice_aSubtitleInfos[sithVoice_curSubtitleDrawIndex].aSubtitleText) > 0)
        && (curTime > sithVoice_aSubtitleInfos[sithVoice_curSubtitleDrawIndex].msecShowEndTime) )
    {
        ++sithVoice_curSubtitleDrawIndex;

        // Fixed: Added OB check and loop break, which fixes potential out of bounds read/write
        if ( sithVoice_curSubtitleDrawIndex >= STD_ARRAYLEN(sithVoice_aSubtitleInfos) )
        {
            sithVoice_curSubtitleDrawIndex = STD_ARRAYLEN(sithVoice_aSubtitleInfos) - 1;
            bDraw = false;
            break;
        }
    }

    if ( bDraw && strlen(sithVoice_aSubtitleInfos[sithVoice_curSubtitleDrawIndex].aSubtitleText) > 0 )
    {
        uint32_t width, height;
        stdDisplay_GetBackBufferSize(&width, &height);

        float x = (float)((double)width * SITHVOICE_TEXT_PADDING_X / SITHVOICE_REF_WIDTH);
        float y = (float)((double)height * SITHVOICE_TEXT_PADDING_Y / SITHVOICE_REF_HEIGHT);
        float lineHeight = (float)((double)sithVoice_pTextFont->lineSpacing / RD_REF_HEIGHT);

        SithVoiceSubtitleInfo* pInfo = &sithVoice_aSubtitleInfos[sithVoice_curSubtitleDrawIndex];
        if ( curTime >= pInfo->msecEndTime
            && (sithVoice_curSubtitleDrawIndex < sithVoice_numSubtitleInfos - 1 && strlen(sithVoice_aSubtitleInfos[sithVoice_curSubtitleDrawIndex + 1].aSubtitleText) > 0) ) // Fixed: Added OB check
        {
            y = y - (float)((double)(curTime - pInfo->msecEndTime) * lineHeight) / (pInfo->msecShowEndTime - pInfo->msecEndTime);
        }

        // Print subtitle text to the screen
        // Fixed: Add OB check for lineCount + sithVoice_numSubtitleInfos
        for ( size_t lineCount = 0; lineCount < SITHVOICE_MAX_LINES_PER_DRAW && (lineCount + sithVoice_curSubtitleDrawIndex) < sithVoice_numSubtitleInfos; lineCount++ )
        {
            pInfo = &sithVoice_aSubtitleInfos[lineCount + sithVoice_curSubtitleDrawIndex];
            if ( strlen(pInfo->aSubtitleText) == 0 || curTime <= pInfo->msecStartTime )
            {
                break;
            }

            // Added: Check for case when info is considered removed, i.e.: msecShowEndTime == 0
            if ( curTime > pInfo->msecShowEndTime )
            {
                continue;
            }

            rdFont_SetFontColor(pInfo->aTextColors);
            rdFont_DrawTextLine(pInfo->aSubtitleText, x, y, rdCamera_g_pCurCamera->pFrustum->nearPlane, sithVoice_pTextFont, RDFONT_ALIGNLEFT);

            y = y + lineHeight;
        }
    }
}

int J3DAPI sithVoice_SyncVoiceState(DPID idTo, unsigned int outstream)
{
    SITHDSS_STARTOUT(SITHDSS_VOICESTATE);
    SITHDSS_PUSHINT8(sithVoice_bShowText);

    //sithMulti_g_message.data[0] = sithVoice_bShowText;
   // uint8_t* pCurOut = &sithMulti_g_message.data[1];

    static_assert(STD_ARRAYLEN(sithVoice_aVoiceHeadHeights) == 4, "");
    static_assert(STD_ARRAYLEN(sithVoice_aVoiceHeadHeights[0]) == 4, "");
    for ( size_t i = 0; i < STD_ARRAYLEN(sithVoice_aVoiceHeadHeights); ++i )
    {
        for ( size_t j = 0; j < STD_ARRAYLEN(sithVoice_aVoiceHeadHeights[i]); ++j )
        {
            SITHDSS_PUSHINT8(sithVoice_aVoiceHeadHeights[i][j]);
            //pCurOut = sithVoice_aVoiceHeadHeights[i][j];
            //pCurOut = (float*)((char*)pCurOut + 1);
        }
    }

    SITHDSS_PUSHFLOAT(sithVoice_secHeadSwapInterval);
    //*(float*)pCurOut = sithVoice_secHeadSwapInterval;
    // pCurOut += 4;

    SITHDSS_PUSHINT32(sithVoice_msecLipSyncOffset);
    //*(uint32_t*)pCurOut = sithVoice_msecLipSyncOffset;

    SITHDSS_ENDOUT;
    //sithMulti_g_message.type = SITHDSS_VOICESTATE;
    //sithMulti_g_message.length = (char*)(pCurOut + 1) - (char*)sithMulti_g_message.data;

    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, 1u);
}

int J3DAPI sithVoice_ProcessVoiceState(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);

    SITHDSS_STARTIN(pMsg);

    sithVoice_bShowText = SITHDSS_POPUINT8();
    //sithVoice_bShowText = pMsg->data[0];

    //pCurIn = &pMsg->data[1];
    for ( size_t i = 0; i < 4; ++i )
    {
        for ( size_t j = 0; j < 4; ++j )
        {
            sithVoice_aVoiceHeadHeights[i][j] = SITHDSS_POPUINT8();
            //sithVoice_aVoiceHeadHeights[i][j] = *pCurIn++;
        }
    }

    sithVoice_secHeadSwapInterval = SITHDSS_POPFLOAT();
    //sithVoice_secHeadSwapInterval = *(float*)pCurIn;

    sithVoice_msecLipSyncOffset = SITHDSS_POPUINT32();
    //sithVoice_msecLipSyncOffset = *((uint32_t*)pCurIn + 1);

    SITHDSS_ENDIN;
    return 1;
}

const rdFont* sithVoice_GetTextFont(void)
{
    return sithVoice_pTextFont;
}