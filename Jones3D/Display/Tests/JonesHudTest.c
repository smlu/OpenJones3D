#include <unity_fixture.h>

#include <Jones3D/Display/JonesHud.h>
#include <Jones3D/Display/jonesConfig.h>
#include <Jones3D/Display/JonesReticle.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Play/jonesCog.h>
#include <Jones3D/Play/jonesInventory.h>
#include <sith/Dss/sithGamesave.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sound/Sound.h>
#include <sound/Driver.h>
#include <std/Win95/stdWin95.h>
#include <string.h>

// Exercise the actual channel allocator, pause clock and fades. Only backend
// buffers and time are synthetic; the regression does not need a sound device.
typedef struct JonesHudTestBuffer
{
    int bAllocated;
    int bPlaying;
    float volume;
} JonesHudTestBuffer;

static JonesHudTestBuffer JonesHudTest_aBuffers[4];
static uint32_t JonesHudTest_msecTime;
static unsigned JonesHudTest_numCreates;
static unsigned JonesHudTest_numReleases;
static unsigned JonesHudTest_numResumes;
static int JonesHudTest_failCreate;
static int JonesHudTest_bGamePaused;
static int JonesHudTest_bThemeOwnedAtResume;
static float JonesHudTest_themeVolumeAfterResume;
static int JonesHudTest_bMenuVisible;
static int JonesHudTest_bCutsceneMode;
static int JonesHudTest_bEnableGamesave;
static int JonesHudTest_dialogResult;
static int JonesHudTest_playerNum;
static HWND J3DAPI JonesHudTest_GetWindow(void);
static void JonesHudTest_CloseRestore(void);
static SithThing JonesHudTest_playerThing;
static SithThing* JonesHudTest_pPlayerThing;
static SithPlayer JonesHudTest_player;
static SithInventoryType JonesHudTest_aInventoryTypes[SITHINVENTORY_MAXTYPES];

static uint32_t JonesHudTest_GetTimeMsec(void);
static int JonesHudTest_Use3DCaps(void);
static tSysSoundBuffer* J3DAPI JonesHudTest_CreateAndPlay(size_t sampleRate, size_t sampleBits, size_t numChannels, const uint8_t* pData, size_t dataSize, tSoundChannelFlag* pFlags, tSysSoundBuffer* pBuffer);
static void J3DAPI JonesHudTest_Release(tSysSoundBuffer* pBuffer);
static void J3DAPI JonesHudTest_Play(tSysSoundBuffer* pBuffer, int bLoop);
static void J3DAPI JonesHudTest_Stop(tSysSoundBuffer* pBuffer);
static void J3DAPI JonesHudTest_SetVolume(tSysSoundBuffer* pBuffer, float volume);
static float J3DAPI JonesHudTest_GetVolume(tSysSoundBuffer* pBuffer);
static void J3DAPI JonesHudTest_SetPan(tSysSoundBuffer* pBuffer, float pan);
static void J3DAPI JonesHudTest_SetFrequency(tSysSoundBuffer* pBuffer, float frequency);

#define SoundDriver_GetTimeMsec JonesHudTest_GetTimeMsec
#define SoundDriver_Use3DCaps JonesHudTest_Use3DCaps
#define SoundDriver_CreateAndPlay JonesHudTest_CreateAndPlay
#define SoundDriver_Release JonesHudTest_Release
#define SoundDriver_Play JonesHudTest_Play
#define SoundDriver_Stop JonesHudTest_Stop
#define SoundDriver_SetVolume JonesHudTest_SetVolume
#define SoundDriver_GetVolume JonesHudTest_GetVolume
#define SoundDriver_SetPan JonesHudTest_SetPan
#define SoundDriver_SetFrequency JonesHudTest_SetFrequency
#include "../../../Libs/sound/Sound.c"
// The mixer includes the local player number in channel GUIDs. Keep this RTI
// value local on both backends, irrespective of the executable load address.
#undef sithPlayer_g_playerNum
#define sithPlayer_g_playerNum JonesHudTest_playerNum
#include "../../../Libs/sith/Devices/sithSoundMixer.c"

// Keep the world state local to this executable instead of fixed RTI addresses.
#undef sithPlayer_g_pLocalPlayerThing
#define sithPlayer_g_pLocalPlayerThing JonesHudTest_pPlayerThing
#undef sithInventory_g_aTypes
#define sithInventory_g_aTypes JonesHudTest_aInventoryTypes
#undef jonesCog_g_bMenuVisible
#define jonesCog_g_bMenuVisible JonesHudTest_bMenuVisible
#undef sithPlayerControls_g_bCutsceneMode
#define sithPlayerControls_g_bCutsceneMode JonesHudTest_bCutsceneMode
#undef jonesCog_g_bEnableGamesave
#define jonesCog_g_bEnableGamesave JonesHudTest_bEnableGamesave
#define stdWin95_GetWindow JonesHudTest_GetWindow
#define sithGamesave_CloseRestore JonesHudTest_CloseRestore
#ifndef JONESHUD_TEST_SOURCE
#define JONESHUD_TEST_SOURCE "../JonesHud.c"
#endif
#include JONESHUD_TEST_SOURCE

static tHostServices JonesHudTest_hostServices;
static tSoundChannel JonesHudTest_aChannels[4];
static SoundInfo JonesHudTest_soundInfo;
static uint8_t JonesHudTest_aSoundData[32];
static JonesHudMenuItem JonesHudTest_aItems[JONESHUD_MAX_MENU_ITEMS];

static uint32_t JonesHudTest_GetTimeMsec(void)
{
    return JonesHudTest_msecTime;
}

static int JonesHudTest_Use3DCaps(void)
{
    return 0;
}

static tSysSoundBuffer* J3DAPI JonesHudTest_CreateAndPlay(size_t sampleRate, size_t sampleBits, size_t numChannels, const uint8_t* pData, size_t dataSize, tSoundChannelFlag* pFlags, tSysSoundBuffer* pBuffer)
{
    J3D_UNUSED(pBuffer);
    TEST_ASSERT_EQUAL_UINT(11025, sampleRate);
    TEST_ASSERT_EQUAL_UINT(16, sampleBits);
    TEST_ASSERT_EQUAL_UINT(1, numChannels);
    TEST_ASSERT_EQUAL_PTR(JonesHudTest_aSoundData, pData);
    TEST_ASSERT_EQUAL_UINT(sizeof(JonesHudTest_aSoundData), dataSize);
    TEST_ASSERT_EQUAL_UINT(SOUND_CHANNEL_PLAYING | SOUND_CHANNEL_PLAYONCE, *pFlags);
    ++JonesHudTest_numCreates;
    if ( JonesHudTest_failCreate )
    {
        return NULL;
    }
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHudTest_aBuffers); ++i )
    {
        if ( !JonesHudTest_aBuffers[i].bAllocated )
        {
            JonesHudTest_aBuffers[i].bAllocated = 1;
            JonesHudTest_aBuffers[i].bPlaying = 1;
            return (tSysSoundBuffer*)&JonesHudTest_aBuffers[i];
        }
    }
    TEST_FAIL_MESSAGE("Unexpected extra menu sound buffer");
}

static void J3DAPI JonesHudTest_Release(tSysSoundBuffer* pBuffer)
{
    if ( pBuffer )
    {
        JonesHudTestBuffer* pFake = (JonesHudTestBuffer*)pBuffer;
        TEST_ASSERT_TRUE(pFake->bAllocated);
        pFake->bAllocated = 0;
        pFake->bPlaying = 0;
        ++JonesHudTest_numReleases;
    }
}

static void J3DAPI JonesHudTest_Play(tSysSoundBuffer* pBuffer, int bLoop)
{
    TEST_ASSERT_NOT_NULL(pBuffer);
    TEST_ASSERT_FALSE(bLoop);
    ((JonesHudTestBuffer*)pBuffer)->bPlaying = 1;
}

static void J3DAPI JonesHudTest_Stop(tSysSoundBuffer* pBuffer)
{
    TEST_ASSERT_NOT_NULL(pBuffer);
    ((JonesHudTestBuffer*)pBuffer)->bPlaying = 0;
}

static void J3DAPI JonesHudTest_SetVolume(tSysSoundBuffer* pBuffer, float volume)
{
    TEST_ASSERT_NOT_NULL(pBuffer);
    // Retain the requested value so an extrapolated fade above 100% is visible.
    ((JonesHudTestBuffer*)pBuffer)->volume = volume;
}

static float J3DAPI JonesHudTest_GetVolume(tSysSoundBuffer* pBuffer)
{
    TEST_ASSERT_NOT_NULL(pBuffer);
    return ((JonesHudTestBuffer*)pBuffer)->volume;
}

static void J3DAPI JonesHudTest_SetPan(tSysSoundBuffer* pBuffer, float pan)
{
    TEST_ASSERT_NOT_NULL(pBuffer);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pan);
}

static void J3DAPI JonesHudTest_SetFrequency(tSysSoundBuffer* pBuffer, float frequency)
{
    J3D_UNUSED(pBuffer); J3D_UNUSED(frequency);
    TEST_FAIL_MESSAGE("Menu theme must not alter pitch");
}

static int J3DAPI JonesHudTest_Log(const char* pFormat, ...)
{
    J3D_UNUSED(pFormat);
    return 0;
}

void JonesMain_ResumeGame(void)
{
    if ( JonesHudTest_bGamePaused )
    {
        // Observe ordering at the application boundary, before Sound_Resume can
        // rewind its clock and resume the previously paused world channels.
        JonesHudTest_bThemeOwnedAtResume |= Sound_GetChannelBySoundHandle(1234u) != NULL;
        ++JonesHudTest_numResumes;
        Sound_Resume();
        Sound_UpdateFades();
        tSoundChannel* pTheme = Sound_GetChannelBySoundHandle(1234u);
        JonesHudTest_themeVolumeAfterResume = pTheme ? JonesHudTest_GetVolume(pTheme->pDSoundBuffer) : 0.0f;
        JonesHudTest_bGamePaused = 0;
    }
}

static void JonesHudTest_CloseRestore(void)
{
}

static HWND J3DAPI JonesHudTest_GetWindow(void)
{
    return NULL;
}

void JonesReticle_Resume(void)
{
}

void JonesReticle_Close(void)
{
}

void jonesConfig_Shutdown(void)
{
}

void jonesInventory_Close(void)
{
}

// Unused application paths fail if reached. The complete module is compiled so
// private menu state remains production-owned without adding a test API.
int jonesConfig_Startup(void)
{
    TEST_FAIL_MESSAGE("Unexpected config startup");
}

int J3DAPI jonesConfig_ShowMessageDialog(HWND h, const char* a, const char* b, int c)
{
    J3D_UNUSED(h);
    J3D_UNUSED(a);
    J3D_UNUSED(b);
    J3D_UNUSED(c);
    TEST_FAIL_MESSAGE("Unexpected message dialog");
}

int J3DAPI jonesConfig_GetSaveGameFilePath(HWND h, char* p)
{
    J3D_UNUSED(h);
    J3D_UNUSED(p);
    TEST_FAIL_MESSAGE("Unexpected save dialog");
}

int J3DAPI jonesConfig_GetLoadGameFilePath(HWND h, char* p)
{
    TEST_ASSERT_NULL(h);
    TEST_ASSERT_TRUE(JonesHudTest_bGamePaused);
    TEST_ASSERT_EQUAL_INT(1, Sound_pausedRefCount);
    TEST_ASSERT_NOT_NULL(Sound_GetChannelBySoundHandle(1234u));
    JonesHudTest_msecTime += 30000;
    if ( JonesHudTest_dialogResult == 1 )
    {
        memcpy(p, "test.nds", sizeof("test.nds"));
    }
    return JonesHudTest_dialogResult;
}

int J3DAPI jonesConfig_ShowGamePlayOptions(HWND h)
{
    J3D_UNUSED(h);
    TEST_FAIL_MESSAGE("Unexpected gameplay dialog");
}

int J3DAPI jonesConfig_ShowControlOptions(HWND h)
{
    J3D_UNUSED(h);
    TEST_FAIL_MESSAGE("Unexpected controls dialog");
}

int J3DAPI jonesConfig_ShowDisplaySettingsDialog(HWND h, StdDisplayEnvironment* e, JonesDisplaySettings* s)
{
    J3D_UNUSED(h);
    J3D_UNUSED(e);
    J3D_UNUSED(s);
    TEST_FAIL_MESSAGE("Unexpected display dialog");
}

int J3DAPI jonesConfig_ShowSoundSettingsDialog(HWND h, JonesSoundSettings* s)
{
    J3D_UNUSED(h);
    J3D_UNUSED(s);
    TEST_FAIL_MESSAGE("Unexpected sound dialog");
}

int J3DAPI jonesConfig_ShowGameOverDialog(HWND h, char* p, tSoundHandle s, tSoundChannelHandle* c)
{
    J3D_UNUSED(h);
    J3D_UNUSED(p);
    J3D_UNUSED(s);
    J3D_UNUSED(c);
    TEST_FAIL_MESSAGE("Unexpected gameover dialog");
}

int J3DAPI jonesConfig_ShowStatisticsDialog(HWND h, SithGameStatistics* p)
{
    J3D_UNUSED(h);
    J3D_UNUSED(p);
    TEST_FAIL_MESSAGE("Unexpected statistics dialog");
}

int J3DAPI jonesConfig_ShowLevelCompletedDialog(HWND h, int* a, int* b, int c, int d, int e, int f, int g, int i)
{
    J3D_UNUSED(h);
    J3D_UNUSED(a);
    J3D_UNUSED(b);
    J3D_UNUSED(c);
    J3D_UNUSED(d);
    J3D_UNUSED(e);
    J3D_UNUSED(f);
    J3D_UNUSED(g);
    J3D_UNUSED(i);
    TEST_FAIL_MESSAGE("Unexpected completed dialog");
}

int J3DAPI jonesConfig_ShowExitGameDialog(HWND h, char* p)
{
    J3D_UNUSED(h);
    J3D_UNUSED(p);
    TEST_FAIL_MESSAGE("Unexpected exit dialog");
}

void jonesConfig_UpdateCurrentLevelNum(void)
{
    TEST_FAIL_MESSAGE("Unexpected level update");
}

void JonesReticle_Open(void)
{
    TEST_FAIL_MESSAGE("Unexpected reticle open");
}

void J3DAPI JonesReticle_Update(float deltaTime)
{
    J3D_UNUSED(deltaTime);
    TEST_FAIL_MESSAGE("Unexpected reticle update");
}

void JonesReticle_Render(void)
{
    TEST_FAIL_MESSAGE("Unexpected reticle render");
}

void JonesReticle_Pause(void)
{
    TEST_FAIL_MESSAGE("Unexpected reticle pause");
}

int JonesMain_HasStarted(void)
{
    return 1;
}

void JonesMain_PauseGame(void)
{
    TEST_FAIL_MESSAGE("Unexpected application pause");
}

int JonesMain_ProcessGamesaveState(void)
{
    TEST_FAIL_MESSAGE("Unexpected gamesave processing");
}

size_t JonesMain_GetCurrentLevelNum(void)
{
    TEST_FAIL_MESSAGE("Unexpected level query");
}

StdDisplayEnvironment* JonesMain_GetDisplayEnvironment(void)
{
    TEST_FAIL_MESSAGE("Unexpected display query");
}

JonesDisplaySettings* JonesMain_GetDisplaySettings(void)
{
    TEST_FAIL_MESSAGE("Unexpected settings query");
}

int JonesMain_CloseWindow(void)
{
    TEST_FAIL_MESSAGE("Unexpected window close");
}

void JonesMain_RefreshDisplayDevice(void)
{
    TEST_FAIL_MESSAGE("Unexpected display refresh");
}

void J3DAPI JonesMain_LogErrorToFile(const char* p)
{
    J3D_UNUSED(p);
    TEST_FAIL_MESSAGE("Unexpected application error");
}

void J3DAPI jonesCog_StartCutscene(SithCog* c)
{
    J3D_UNUSED(c);
    TEST_FAIL_MESSAGE("Unexpected cutscene");
}

void J3DAPI jonesCog_ToggleInterface(SithCog* c)
{
    J3D_UNUSED(c);
    TEST_FAIL_MESSAGE("Unexpected interface toggle");
}

int jonesInventory_Open(void)
{
    TEST_FAIL_MESSAGE("Unexpected inventory open");
}

int J3DAPI jonesInventory_ActivateItem(SithThing* p, int i)
{
    J3D_UNUSED(p);
    J3D_UNUSED(i);
    TEST_FAIL_MESSAGE("Unexpected inventory activation");
}

int J3DAPI jonesInventory_UseItem(SithThing* p, unsigned i)
{
    J3D_UNUSED(p);
    J3D_UNUSED(i);
    TEST_FAIL_MESSAGE("Unexpected inventory use");
}

int jonesInventory_ResetGameStatistics(void)
{
    TEST_FAIL_MESSAGE("Unexpected statistics reset");
}

void jonesInventory_UpdateSolvedHintsStatistics(void)
{
    TEST_FAIL_MESSAGE("Unexpected hints update");
}

int jonesInventory_GetTotalIQPoints(void)
{
    TEST_FAIL_MESSAGE("Unexpected IQ query");
}

void jonesInventory_AdvanceFoundTreasuresStatistics(void)
{
    TEST_FAIL_MESSAGE("Unexpected treasures update");
}

void J3DAPI jonesInventory_UpdateGameTimeStatistics(uint32_t t)
{
    J3D_UNUSED(t);
    TEST_FAIL_MESSAGE("Unexpected game time statistics");
}

void jonesInventory_ResetStatisticsGameTime(void)
{
    TEST_FAIL_MESSAGE("Unexpected statistics reset");
}

const char* J3DAPI jonesString_GetString(const char* p)
{
    J3D_UNUSED(p);
    TEST_FAIL_MESSAGE("Unexpected localization query");
}

static void JonesHudTest_OpenMenu(void)
{
    if ( !JonesHudTest_bGamePaused )
    {
        Sound_Pause();
        JonesHudTest_bGamePaused = 1;
    }
    JonesHud_MenuOpen();
    TEST_ASSERT_TRUE(JonesHud_hudState & 0x01);
    // Finish the root's opening translation before asking the menu to close.
    JonesHudTest_msecTime += 101;
    JonesHud_msecTime = JonesHudTest_msecTime;
    JonesHud_UpdateItem(JonesHud_pMenuItemLinkedList);
}

static void JonesHudTest_FinishClose(void)
{
    JonesHudTest_msecTime += 1000;
    JonesHud_msecTime = JonesHudTest_msecTime;
    JonesHud_UpdateItem(JonesHud_pMenuItemLinkedList);
    TEST_ASSERT_EQUAL_INT(0, JonesHud_hudState);
    TEST_ASSERT_FALSE(JonesHudTest_bGamePaused);
    TEST_ASSERT_EQUAL_UINT(SOUND_INVALIDHANDLE, JonesHud_hSndChannelMusic);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, JonesHudTest_themeVolumeAfterResume, "Menu theme must be inaudible after the sound clock resumes");
    TEST_ASSERT_FALSE_MESSAGE(JonesHudTest_bThemeOwnedAtResume, "Menu theme must be released before gameplay sound resumes");
    Sound_UpdateFades();
    TEST_ASSERT_NULL(Sound_GetChannelBySoundHandle(1234u));
}

TEST_GROUP(JonesHud);

TEST_SETUP(JonesHud)
{
    memset(JonesHudTest_aBuffers, 0, sizeof(JonesHudTest_aBuffers));
    memset(JonesHudTest_aChannels, 0, sizeof(JonesHudTest_aChannels));
    memset(JonesHudTest_aItems, 0, sizeof(JonesHudTest_aItems));
    memset(JonesHudTest_aInventoryTypes, 0, sizeof(JonesHudTest_aInventoryTypes));
    memset(&JonesHudTest_playerThing, 0, sizeof(JonesHudTest_playerThing));
    memset(&JonesHudTest_player, 0, sizeof(JonesHudTest_player));
    memset(&JonesHudTest_hostServices, 0, sizeof(JonesHudTest_hostServices));
    JonesHudTest_hostServices.pErrorPrint = JonesHudTest_Log;
    JonesHudTest_hostServices.pStatusPrint = JonesHudTest_Log;
    JonesHudTest_hostServices.pDebugPrint = JonesHudTest_Log;
    Sound_pHS = &JonesHudTest_hostServices;
    Sound_TestSetChannels(JonesHudTest_aChannels, STD_ARRAYLEN(JonesHudTest_aChannels));
    Sound_TestSetOpen(1);
    memset(&JonesHudTest_soundInfo, 0, sizeof(JonesHudTest_soundInfo));
    JonesHudTest_soundInfo.hSnd = 1234u;
    JonesHudTest_soundInfo.sampleRate = 11025;
    JonesHudTest_soundInfo.sempleBitSize = 16;
    JonesHudTest_soundInfo.numChannels = 1;
    JonesHudTest_soundInfo.dataSize = sizeof(JonesHudTest_aSoundData);
    Sound_TestSetSoundBank(0, &JonesHudTest_soundInfo, 1, JonesHudTest_aSoundData, sizeof(JonesHudTest_aSoundData));
    memset(Sound_aFades, 0, sizeof(Sound_aFades));
    Sound_pausedRefCount = 0;
    Sound_msecElapsed = 0;
    Sound_handleEntropy = 3;
    Sound_maxVolume = 0.8f;
    sithSoundMixer_bStartup = true;
    sithSoundMixer_channelGUIDSeed = 0;
    JonesHudTest_msecTime = JonesHud_msecTime = 1000;
    JonesHudTest_numCreates = JonesHudTest_numReleases = JonesHudTest_numResumes = 0;
    JonesHudTest_themeVolumeAfterResume = 0.0f;
    JonesHudTest_failCreate = JonesHudTest_bGamePaused = JonesHudTest_bThemeOwnedAtResume = 0;
    JonesHudTest_bMenuVisible = 1;
    JonesHudTest_bCutsceneMode = 0;
    JonesHudTest_bEnableGamesave = 1;
    JonesHudTest_dialogResult = 2;
    JonesHudTest_playerNum = 0;
    JonesHudTest_pPlayerThing = &JonesHudTest_playerThing;
    JonesHudTest_playerThing.thingInfo.actorInfo.pPlayer = &JonesHudTest_player;
    memset(JonesHud_apMenuItems, 0, sizeof(JonesHud_apMenuItems));
    memset(JonesHud_aSoundFxHandles, 0, sizeof(JonesHud_aSoundFxHandles));
    // Only the system column is needed; all of its authored entries exist.
    for ( int i = JONESHUD_MENU_IQ; i <= JONESHUD_MENU_EXIT; ++i )
    {
        JonesHudTest_aItems[i].id = i;
        JonesHudTest_aItems[i].inventoryID = -1;
        JonesHud_apMenuItems[i] = &JonesHudTest_aItems[i];
    }
    JonesHud_aSoundFxHandles[0] = 1234u;
    JonesHud_hSndChannelMusic = SOUND_INVALIDHANDLE;
    JonesHud_bMenuMusicEnabled = true;
    JonesHud_menuMusicVolume = 0.55f;
    JonesHud_menuMusicFadeDuratuion = 0.75f;
    JonesHud_hudState = 0;
    JonesHud_pMenuItemLinkedList = JonesHud_pCloseMenuItem = JonesHud_pCurSelectedMenuItem = NULL;
    JonesHud_pCurInvChangedItem = NULL;
    JonesHud_pHudCamera = NULL;
    JonesHud_pHudCanvas = NULL;
    JonesHud_pMenuItemTextFont = NULL;
    JonesHud_bMapOpen = 0;
    JonesHud_bRestoreActivated = JonesHud_bRestoreGameStatistics = JonesHud_bItemActivated = 0;
    JonesHud_bStartup = JonesHud_bOpen = true;
}

TEST_TEAR_DOWN(JonesHud)
{
    Sound_StopAllSounds();
    memset(JonesHud_apMenuItems, 0, sizeof(JonesHud_apMenuItems));
    JonesHud_pMenuItemLinkedList = JonesHud_pCloseMenuItem = JonesHud_pCurSelectedMenuItem = NULL;
    JonesHudTest_pPlayerThing = NULL;
    JonesHud_hSndChannelMusic = SOUND_INVALIDHANDLE;
    Sound_TestResetLipSyncState();
}

TEST(JonesHud, LongDialogCancelThenCloseReleasesThemeBeforeClockResume)
{
    JonesHudTest_OpenMenu();
    TEST_ASSERT_EQUAL_UINT(1, JonesHudTest_numCreates);
    TEST_ASSERT_EQUAL_FLOAT(0.44f, JonesHudTest_aBuffers[0].volume);
    // The real menu action receives the cancelled dialog result after a long
    // synthetic dwell. It must leave the inventory and its theme active.
    JonesHud_pCurSelectedMenuItem = &JonesHudTest_aItems[JONESHUD_MENU_LOAD_GAME];
    JonesHud_MenuActivateItem();
    TEST_ASSERT_TRUE(JonesHud_hudState & 0x01);
    TEST_ASSERT_FALSE(JonesHud_hudState & 0x04);
    TEST_ASSERT_EQUAL_UINT(1, JonesHudTest_numCreates);
    JonesHud_pCurSelectedMenuItem = NULL;
    JonesHud_msecTime = JonesHudTest_msecTime;
    JonesHud_MenuClose();
    JonesHudTest_FinishClose();
    TEST_ASSERT_EQUAL_UINT(1, JonesHudTest_numReleases);
    TEST_ASSERT_EQUAL_UINT(1, JonesHudTest_numResumes);
    TEST_ASSERT_EQUAL_UINT(1000, Sound_GetDeltaTime());
}

TEST(JonesHud, AcceptedLoadDialogClosesAndReleasesTheTheme)
{
    JonesHudTest_OpenMenu();
    JonesHudTest_dialogResult = 1;
    JonesHud_pCurSelectedMenuItem = &JonesHudTest_aItems[JONESHUD_MENU_LOAD_GAME];
    JonesHud_MenuActivateItem();
    TEST_ASSERT_TRUE(JonesHud_hudState & 0x04);
    TEST_ASSERT_TRUE(JonesHud_bRestoreActivated);
    TEST_ASSERT_EQUAL_STRING("test.nds", JonesHud_aSlectedNdsFilePath);
    JonesHudTest_FinishClose();
}

TEST(JonesHud, FailedLoadDialogLeavesTheMenuThemeOwnedUntilClose)
{
    JonesHudTest_OpenMenu();
    JonesHudTest_dialogResult = -1;
    JonesHud_pCurSelectedMenuItem = &JonesHudTest_aItems[JONESHUD_MENU_LOAD_GAME];
    JonesHud_MenuActivateItem();
    TEST_ASSERT_FALSE(JonesHud_hudState & 0x04);
    TEST_ASSERT_NOT_EQUAL(SOUND_INVALIDHANDLE, JonesHud_hSndChannelMusic);
    JonesHud_MenuClose();
    JonesHudTest_FinishClose();
}

TEST(JonesHud, FinishedThemeRepeatsOnlyWhileTheMenuIsOpen)
{
    JonesHudTest_OpenMenu();
    tSoundChannelHandle hFirst = JonesHud_hSndChannelMusic;
    Sound_StopChannel(hFirst); // production update releases a completed one-shot
    JonesHud_UpdateMenuMusic();
    TEST_ASSERT_NOT_EQUAL(hFirst, JonesHud_hSndChannelMusic);
    TEST_ASSERT_EQUAL_UINT(2, JonesHudTest_numCreates);
    JonesHud_MenuClose();
    JonesHudTest_FinishClose();
    TEST_ASSERT_EQUAL_UINT(2, JonesHudTest_numReleases);
}

TEST(JonesHud, RepeatedMenuSessionsNeverLeaveAnAudibleThemeInGameplay)
{
    for ( unsigned i = 0; i < 4; ++i )
    {
        JonesHudTest_OpenMenu();
        JonesHudTest_msecTime += i * 10000;
        JonesHud_MenuClose();
        JonesHudTest_FinishClose();
        TEST_ASSERT_EQUAL_UINT(i + 1, JonesHudTest_numCreates);
        TEST_ASSERT_EQUAL_UINT(i + 1, JonesHudTest_numReleases);
    }
}

TEST(JonesHud, ReopenDuringClosingDoesNotDuplicateTheTheme)
{
    JonesHudTest_OpenMenu();
    tSoundChannelHandle hFirst = JonesHud_hSndChannelMusic;
    JonesHud_MenuClose();
    // An open request during the close animation keeps the existing session;
    // it must neither duplicate the track nor lose the still-owned fade.
    JonesHud_MenuOpen();
    TEST_ASSERT_EQUAL_UINT(hFirst, JonesHud_hSndChannelMusic);
    TEST_ASSERT_NOT_NULL(Sound_GetChannel(hFirst));
    TEST_ASSERT_EQUAL_UINT(1, JonesHudTest_numCreates);
    TEST_ASSERT_EQUAL_UINT(0, JonesHudTest_numReleases);
    JonesHudTest_FinishClose();
}

TEST(JonesHud, FinishedFadeDoesNotRestartWhileMenuIsClosing)
{
    JonesHudTest_OpenMenu();
    JonesHud_MenuClose();
    JonesHudTest_msecTime += 751;
    Sound_UpdateFades();
    TEST_ASSERT_NULL(Sound_GetChannelBySoundHandle(1234u));
    JonesHud_UpdateMenuMusic();
    TEST_ASSERT_EQUAL_UINT(1, JonesHudTest_numCreates);
    JonesHudTest_FinishClose();
}

TEST(JonesHud, FailedPlaybackCanRetryAndStillClosesCleanly)
{
    JonesHudTest_failCreate = 1;
    JonesHudTest_OpenMenu();
    TEST_ASSERT_EQUAL_UINT(SOUND_INVALIDHANDLE, JonesHud_hSndChannelMusic);
    JonesHudTest_failCreate = 0;
    JonesHud_UpdateMenuMusic();
    TEST_ASSERT_NOT_EQUAL(SOUND_INVALIDHANDLE, JonesHud_hSndChannelMusic);
    JonesHud_MenuClose();
    JonesHudTest_FinishClose();
    TEST_ASSERT_EQUAL_UINT(1, JonesHudTest_numReleases);
}

TEST(JonesHud, DisabledOrMissingThemeDoesNotAllocateAudio)
{
    JonesHud_bMenuMusicEnabled = false;
    JonesHudTest_OpenMenu();
    JonesHud_MenuClose();
    JonesHudTest_FinishClose();
    JonesHud_bMenuMusicEnabled = true;
    JonesHud_aSoundFxHandles[0] = SOUND_INVALIDHANDLE;
    JonesHudTest_OpenMenu();
    JonesHud_MenuClose();
    JonesHudTest_FinishClose();
    TEST_ASSERT_EQUAL_UINT(0, JonesHudTest_numCreates);
}

TEST(JonesHud, HudCloseReleasesActiveAndClosingTheme)
{
    JonesHudTest_OpenMenu();
    JonesHud_MenuClose();
    // Menu entries here are static test storage, so leave them outside HUD's
    // heap-owned cleanup while exercising the real HUD close audio ownership.
    memset(JonesHud_apMenuItems, 0, sizeof(JonesHud_apMenuItems));
    JonesHud_Close();
    TEST_ASSERT_EQUAL_UINT(SOUND_INVALIDHANDLE, JonesHud_hSndChannelMusic);
    TEST_ASSERT_NULL(Sound_GetChannelBySoundHandle(1234u));
    TEST_ASSERT_EQUAL_UINT(1, JonesHudTest_numReleases);
}

TEST(JonesHud, ShutdownReleasesThemeAndClearsItsOwnership)
{
    JonesHudTest_OpenMenu();
    JonesHud_Shutdown();
    TEST_ASSERT_EQUAL_UINT(SOUND_INVALIDHANDLE, JonesHud_hSndChannelMusic);
    TEST_ASSERT_NULL(Sound_GetChannelBySoundHandle(1234u));
    TEST_ASSERT_EQUAL_UINT(1, JonesHudTest_numReleases);
}

TEST_GROUP_RUNNER(JonesHud)
{
    RUN_TEST_CASE(JonesHud, LongDialogCancelThenCloseReleasesThemeBeforeClockResume);
    RUN_TEST_CASE(JonesHud, AcceptedLoadDialogClosesAndReleasesTheTheme);
    RUN_TEST_CASE(JonesHud, FailedLoadDialogLeavesTheMenuThemeOwnedUntilClose);
    RUN_TEST_CASE(JonesHud, FinishedThemeRepeatsOnlyWhileTheMenuIsOpen);
    RUN_TEST_CASE(JonesHud, RepeatedMenuSessionsNeverLeaveAnAudibleThemeInGameplay);
    RUN_TEST_CASE(JonesHud, ReopenDuringClosingDoesNotDuplicateTheTheme);
    RUN_TEST_CASE(JonesHud, FinishedFadeDoesNotRestartWhileMenuIsClosing);
    RUN_TEST_CASE(JonesHud, FailedPlaybackCanRetryAndStillClosesCleanly);
    RUN_TEST_CASE(JonesHud, DisabledOrMissingThemeDoesNotAllocateAudio);
    RUN_TEST_CASE(JonesHud, HudCloseReleasesActiveAndClosingTheme);
    RUN_TEST_CASE(JonesHud, ShutdownReleasesThemeAndClearsItsOwnership);
}

static void JonesHudTest_RunAllTests(void)
{
    RUN_TEST_GROUP(JonesHud);
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, JonesHudTest_RunAllTests);
}
