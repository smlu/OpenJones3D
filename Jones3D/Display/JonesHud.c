#include "JonesHud.h"
#include "JonesHudConstants.h"

#include <j3dcore/j3dhook.h>

#include <Jones3D/Display/jonesConfig.h>
#include <Jones3D/Display/JonesReticle.h>
#include <Jones3D/Main/jonesLevel.h>
#include <Jones3D/Main/jonesMain.h>
#include <Jones3D/Main/jonesString.h>
#include <Jones3D/Play/jonesCog.h>
#include <Jones3D/Play/jonesInventory.h>
#include <Jones3D/RTI/symbols.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdCanvas.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Engine/rdThing.h>
#include <rdroid/Primitives/rdFont.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/Dss/sithGamesave.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>
#include <sith/World/sithVoice.h>

#include <sound/Sound.h>

#include <std/General/std.h>
#include <std/General/stdConfig.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include <math.h>
#include <stdbool.h>

#define JONESHUD_CREDITS_SPEEDFACTOR    0.5f // default 1.5

//
// Module state vars
//
static bool JonesHud_bStartup = false;
static bool JonesHud_bOpen    = false;

static int JonesHud_bCutsceneStart;
static int JonesHud_bInterfaceEnabled;

static int JonesHud_bIMPState;
static int JonesHud_bBonusMapBought; // what's the use of this var

//
// HUD vars
//
static int JonesHud_hudState = 0;

static float JonesHud_nearClipPlane  = 0.0f;
static rdCamera* JonesHud_pHudCamera = NULL;
static rdCanvas* JonesHud_pHudCanvas = NULL;
static rdMatrix34 JonesHud_camMatrix = { 0 }; // camera world matrix

static size_t JonesHud_msecTime = 0;
static size_t JonesHud_msecDeltaTime = 0;
static size_t JonesHud_msecGameOverWaitDeltaTime = 0;

//
// Inventory menu vars
//
static int JonesHud_bShowMenu;
static int JonesHud_bMenuEnabled;
static int JonesHud_bMapOpen;
static bool JonesHud_bMenuMusicEnabled       = true; // Added
static char JonesHud_aMenuMusicFilename[64]  = "mus_gen_maploadloop.wav"; // Added
static float JonesHud_menuMusicVolume        = 0.55f; // Added
static float JonesHud_menuMusicFadeDuratuion = 0.75f; // Added

static int JonesHud_bRestoreActivated;
static int JonesHud_bRestoreGameStatistics;
static int JonesHud_bItemActivated;
static int JonesHud_bExitActivated;

static float JonesHud_menuItemScale          = 0.75f;
static float JonesHud_menuItemTextPosY       = J3D_QOL_VALUE(-18.0f, 70.0f);
static float JonesHud_menuItemTextSize       = J3D_QOL_VALUE(11.0f, 14.0f);
static float JonesHud_menuItemMoveDurationMs = 250u;

static float JonesHud_menuColumnHeight             = J3D_QOL_VALUE(0.23f, 0.19499999f);
static float JonesHud_menuColumnSpacing            = J3D_QOL_VALUE(0.045f, 0.064999998f);
static int JonesHud_msecMenuColumnExpandDuration   = J3D_QOL_VALUE(50, 100);
static int JonesHud_msecMenuColumnCollapseDuration = J3D_QOL_VALUE(50, 100);
static float JonesHud_menuColumnHorizontalSpacing  = 0.064999998f;

static int JonesHud_msecMenuOpenStartSlideDuration = 100;
static int JonesHud_msecMenuOpenSlideDuration      = 100;

static int JonesHud_msecMenuCloseStartSlideDuration = 50;
static int JonesHud_msecMenuCloseSlideDuration      = 50;

static char JonesHud_aSlectedNdsFilePath[JONESCONFIG_GAMESAVE_FILEPATHSIZE];

// Original constant was -0.102f but this centers the item posiition with the health indicator better
static const float JonesHud_invMenuDefaultZ = -0.104f;
static float JonesHud_invMenuMinZ           = 0.0f;
static float JonesHud_invMenuMaxZ           = 0.0f;

static float JonesHud_itemAspectScaleSize     = 0.0f; // Added
static float JonesHud_itemAspectScalePosition = 0.0f; // Added
static float JonesHud_heightAspectRatioScale  = 0.0f;
static float JonesHud_widthAspectRatioScale   = 0.0f;

static int JonesHud_selectedWeaponID            = -1;
static int JonesHud_selectedWeaponMenuItemID    = -1;
static int JonesHud_selectedTreasuresMenuItemID = -1;
static int JonesHud_selectedItemsMenuItemID     = -1;
static int JonesHud_selectedSystemMenuItemID    = -1;
static int JonesHud_rootMenuItemId              = -1;

static int JonesHud_numFoundTreasures;
static int JonesHud_foundTreasureValue;
static int JonesHud_levelStartIQPoints;

// HUD font
static rdFont* JonesHud_pMenuItemTextFont          = NULL;
static char JonesHud_aMenuItemTextFontFilename[64] = "mat\\jonesComic Sans MS14.gcf";

static float JonesHud_healthIndScale            = 0.0;
static float JonesHud_enduranceIndScale         = 0.0; // Added
static float JonesHud_aCosSinTable[24][2]       = { 0 };
static float JonesHud_aCosSinTableScaled[24][2] = { 0 }; // Not used anymore

static int JonesHud_curWhoopsSndFxIdx = -1;

static size_t JonesHud_msecMenuItemMoveDuration = 250u;
static size_t JonesHud_msecMenuItemMoveCurDuration; // Altered: removed initialization

static JonesHudMenuItem* JonesHud_apMenuItems[JONESHUD_MAX_MENU_ITEMS] = { NULL };

static tSoundChannelHandle JonesHud_hCurSndChannel;
static tSoundChannelHandle JonesHud_hSndChannelMusic = SOUND_INVALIDHANDLE;
static tSoundHandle JonesHud_aSoundFxHandles[STD_ARRAYLEN(JonesHud_aSoundFxIdxs)] = { 0 };

static int JonesHud_item_flag_state_55514C;
static JonesHudMenuItem* JonesHud_pMenuItemLinkedList  = NULL;
static JonesHudMenuItem* JonesHud_pCloseMenuItem       = NULL;
static JonesHudMenuItem* JonesHud_pCurSelectedMenuItem = NULL;
static JonesHudMenuItem* JonesHud_pCurInvChangedItem   = NULL;

static rdMaterial* JonesHud_pArrowUp   = NULL; // unused
static rdMaterial* JonesHud_pArrowDown = NULL; // unused

static HANDLE JonesHud_hProcessHelp; // Unused anymore

static int JonesHud_msecItemChangedShowDuration  = 2000;
static float JonesHud_itemChangedSlideEaseInRate = 4000.0f;
static int JonesHud_itemChangedSlideSpeedRate    = 3;

//
// Keyboard state vars
//
static int JonesHud_bKeyStateUpdated;
static int JonesHud_curKeyId = -1;
static uint32_t JonesHud_msecLastKeyPressTime;
static size_t JonesHud_aActivateKeyIds[JONESCONTROL_ACTION_MAXBINDS] = { 0 };

//
// Health indicator vars
//
static int JonesHud_bHealthIndFadeSet;
static int JonesHud_bFadeHealthHUD;

static float JonesHud_healthIndAlpha;

static float JonesHud_curHealth;
static float JonesHud_curHealthIndFadeState;
static size_t JonesHud_msecHealthIndCurFadeTime;

static int JonesHud_bHealthIndFade;
static float JonesHud_maxHealthIndHitAlpha;
static float JonesHud_curHealthIndhitAlpha;

static JonesHudRect JonesHud_healthIndRect;
static rdVector4 JonesHud_healthIndBarPos;

static float JonesHud_healthIndSize       = J3D_QOL_VALUE(60.0f, 60.0f);
static float JonesHud_healthIndPosX       = 0.041379310f;
static float JonesHud_healthIndPosY       = 0.942857142f;
static float JonesHud_healthIndFade       = J3D_QOL_VALUE(0.65f, 0.2f);
static int JonesHud_msecHealthIndFadeTime = 3000;

static rdVector4 JonesHud_healthIndColorFull     = { 0.20f, 0.78f, 0.30f, 0.0f };
static rdVector4 JonesHud_healthIndColorMedium   = { 0.85f, 0.85f, 0.3f, 0.0f };
static rdVector4 JonesHud_healthIndColorLow      = { 0.69f , 0.07f , 0.15f , 0.0f };
static rdVector4 JonesHud_healthIndColorCritical = { 0.15f , 0.15f , 0.15f , 0.0f };

// Health indicator materials
static rdMaterial* JonesHud_pHealthIndBaseMat                 = NULL;
static char JonesHud_aHealthIndBaseMatFilename[64]            = "hud_health_base.mat";

static rdMaterial* JonesHud_pHealthIndOverlayMat              = NULL;
static char JonesHud_aHealthIndOverlayMatFilename[64]         = "hud_health_overlay.mat";

static rdMaterial* JonesHud_pHealthIndHitOverlayMat           = NULL;
static char JonesHud_aHealthIndHitOverlayMatFilename[64]      = "hud_health_overlay_hit.mat";

static rdMaterial* JonesHud_pHealthIndPoisonedOverlayMat      = NULL;
static char JonesHud_aHealthIndPoisonedOverlayMatFilename[64] = "hud_health_overlay_skull.mat";

//
// Endurance indicator vars
//
static JonesHudRect JonesHud_enduranceRect;
static int JonesHud_msecRaftIndicatorFadeTime;
static float JonesHud_curEnduranceIndAlpha;      // Altered: Moved initialization to startup function due to default value was made static variable
static float JonesHud_curEnduranceState          = -1.0f;
static rdVector4 JonesHud_enduranceIndBarPos     = { 0 };

static float JonesHud_enduranceIndSize   = J3D_QOL_VALUE(32.0f, 60.0f);
static float JonesHud_enduranceIndPosX   = J3D_QOL_VALUE(0.102f, 0.9586206896551f);
static float JonesHud_enduranceIndPosY   = J3D_QOL_VALUE(0.95f, 0.942857142f);
static float JonesHud_enduranceIndFade   = J3D_QOL_VALUE(0.70f, 0.2f);
static int JonesHud_msecEnduranceIndTime = 3000;

static rdVector4 JonesHud_enduranceIndColorOxygen    = { 0.40000001f, 0.69999999f, 0.89999998f,  0.0f };
static rdVector4 JonesHud_enduranceIndColorRaft      = { 0.95f , 0.5f, 0.2f, 0.0f };
static rdVector4 JonesHud_enduranceIndColorIMPEnergy = { 0.678f, 0.325f, 0.678f, 0.0f };

// Endurance indicator materials
static rdMaterial* JonesHud_pEnduranceIndOverlayMat      = NULL;
static char JonesHud_aEnduranceIndOverlayMatFilename[64] = "hud_breath_overlay.mat";

//
// End credit vars
//
static bool JonesHud_bEndingCredits     = false; // Added init to false
static bool JonesHud_bSkipUpdateCredits = false; // Added init to false

static int JonesHud_msecCreditsElapsedTime;
static int JonesHud_msecCreditsFadeStart;

float JonesHud_creditsAspectRatio;
float JonesHud_creditsIconPosOffset;
static float JonesHud_creditsSomeHeightRatio;

static size_t JonesHud_creditsCurIdx;
static size_t JonesHud_creditsCurMatIdx;
static size_t JonesHud_creditsCurEndIdx;

static rdFont* JonesHud_pCreditsFont1 = NULL;
static rdFont* JonesHud_pCreditsFont2 = NULL;

static uint32_t JonesHud_creditsCanvasWidth;
static uint32_t JonesHud_creditsCanvasHeight;

static int JonesHud_creditsCurCelNum;
static rdMaterial* JonesHud_apCreditsMats[STD_ARRAYLEN(JonesHud_aCredits)];

static float JonesHud_creditTextHeight;
static float JonesHud_aCreditsCurPosY[STD_ARRAYLEN(JonesHud_aCredits)];

// Variables that have values assigned but are not being used
static int JonesHud_dword_554FDC;
static float JonesHud_flt_554FE0;
static float JonesHud_flt_554FE4;
static float JonesHud_flt_555034;
static float JonesHud_flt_555038;
static float JonesHud_flt_55503C;
static int JonesHud_dword_55515C;

void J3DAPI JonesHud_Update(const SithWorld* pWorld);
int J3DAPI JonesHud_SetCanvasSize(int width, int height);
void JonesHud_UpdateSinCosTable(void);
void J3DAPI JonesHud_UpdateHUDLayout(uint32_t width, uint32_t height);

void JonesHud_MenuOpen(void);
void JonesHud_MenuClose(void);

void J3DAPI JonesHud_StartItemTranslation(JonesHudMenuItem* pItem, uint32_t msecDuration, float moveRate, int bMoveUp);
int J3DAPI JonesHud_HasTimeElapsed(unsigned int msecDuration, unsigned int msecDeltaTime, unsigned int msecTime);

void J3DAPI JonesHud_RenderEnduranceIndicator(float enduranceState);
void J3DAPI JonesHud_DrawEnduranceIndicator(float state, float alpha);

int J3DAPI JonesHud_RenderHealthIndicator(int bFade);
void J3DAPI JonesHud_DrawHealthIndicator(float hitDelta, float health, float alpha);
void J3DAPI JonesHud_DrawHealthHitOverlay(float hitDelta, float z, float rhw);
void J3DAPI JonesHud_RenderHealthIndicatorHit(float healthState);
void J3DAPI JonesHud_DrawHealthIndicatorBase(float state, float alpha, float z, float w);
void J3DAPI JonesHud_DrawHealthIndicatorBar(float health, float alpha, float z, float w);
void J3DAPI JonesHud_DrawIndicatorBar(const rdVector4* pPos, float scale, const rdVector4* pColor, float angle, int bArc); // Note, pos is in screen coords
void J3DAPI JonesHud_Draw(const rdMaterial* pMaterial, const JonesHudRect* rect, float z, float rhw, const rdVector4* pColor, int celNum, int bAlpha);

void JonesHud_ProcessInventoryMenu(void);
void JonesHud_MenuMoveLeft(void);
void JonesHud_MenuMoveRight(void);
void JonesHud_MenuMoveDown(void);
void J3DAPI JonesHud_SetSelectedMenuItem(int menuId, JonesHudMenuItem* pItem);
void JonesHud_MenuMoveUp(void);

int J3DAPI JonesHud_GetKey(unsigned int keyId);

int JonesHud_InitializeMenu(void);
void JonesHud_InitializeMenuSounds(void);

JonesHudMenuItem* J3DAPI JonesHud_NewMenuItem(rdModel3* pItemIcon3);
void J3DAPI JonesHud_FreeMenuItem(JonesHudMenuItem* pItem);
void J3DAPI JonesHud_UpdateItem(JonesHudMenuItem* pItem);
void J3DAPI JonesHud_RenderMenuItems(JonesHudMenuItem* pItem);
// Function formats and displays inventory items and its text
void J3DAPI JonesHud_RenderMenuItem(JonesHudMenuItem* pItem);
void JonesHud_MenuActivateItem(void);
void JonesHud_ResetMenuItems(void);
void J3DAPI JonesHud_SetLeftRightItems(JonesHudMenuItem* pItem1, JonesHudMenuItem* pItem2);
void J3DAPI JonesHud_SetDownUpItems(JonesHudMenuItem* pItem1, JonesHudMenuItem* pItem2);
void J3DAPI JonesHud_MenuExpandColumn(JonesHudMenuItem* pItem);
void J3DAPI JonesHud_MenuCollapseColumn(JonesHudMenuItem* pItem);

void JonesHud_RenderInventoryItemChange(void);
void J3DAPI JonesHud_RenderChangedItem(const JonesHudMenuItem* pItem, float scale);

void JonesHud_InitializeGameStatistics(void);

HANDLE J3DAPI JonesHud_OpenHelp(HANDLE process);

void JonesHud_InstallHooks(void)
{
    J3D_HOOKFUNC(JonesHud_Startup);
    J3D_HOOKFUNC(JonesHud_Shutdown);
    J3D_HOOKFUNC(JonesHud_Open);
    J3D_HOOKFUNC(JonesHud_Close);
    J3D_HOOKFUNC(JonesHud_ToggleMenu);
    J3D_HOOKFUNC(JonesHud_EnableMenu);
    J3D_HOOKFUNC(JonesHud_IsMenuEnabled);
    J3D_HOOKFUNC(JonesHud_Process);
    J3D_HOOKFUNC(JonesHud_Update);
    J3D_HOOKFUNC(JonesHud_SetCanvasSize);
    J3D_HOOKFUNC(JonesHud_UpdateSinCosTable);
    J3D_HOOKFUNC(JonesHud_UpdateHUDLayout);
    J3D_HOOKFUNC(JonesHud_MenuOpen);
    J3D_HOOKFUNC(JonesHud_MenuClose);
    J3D_HOOKFUNC(JonesHud_StartItemTranslation);
    J3D_HOOKFUNC(JonesHud_HasTimeElapsed);
    J3D_HOOKFUNC(JonesHud_RenderEnduranceIndicator);
    J3D_HOOKFUNC(JonesHud_DrawEnduranceIndicator);
    J3D_HOOKFUNC(JonesHud_SetFadeHealthHUD);
    J3D_HOOKFUNC(JonesHud_RenderHealthIndicator);
    J3D_HOOKFUNC(JonesHud_DrawHealthIndicator);
    J3D_HOOKFUNC(JonesHud_DrawHealthHitOverlay);
    J3D_HOOKFUNC(JonesHud_RenderHealthIndicatorHit);
    J3D_HOOKFUNC(JonesHud_DrawHealthIndicatorBase);
    J3D_HOOKFUNC(JonesHud_DrawHealthIndicatorBar);
    J3D_HOOKFUNC(JonesHud_DrawIndicatorBar);
    J3D_HOOKFUNC(JonesHud_Draw);
    J3D_HOOKFUNC(JonesHud_ProcessInventoryMenu);
    J3D_HOOKFUNC(JonesHud_MenuMoveLeft);
    J3D_HOOKFUNC(JonesHud_MenuMoveRight);
    J3D_HOOKFUNC(JonesHud_MenuMoveDown);
    J3D_HOOKFUNC(JonesHud_SetSelectedMenuItem);
    J3D_HOOKFUNC(JonesHud_MenuMoveUp);
    J3D_HOOKFUNC(JonesHud_BindActivateControlKeys);
    J3D_HOOKFUNC(JonesHud_GetKey);
    J3D_HOOKFUNC(JonesHud_InitializeMenu);
    J3D_HOOKFUNC(JonesHud_InitializeMenuSounds);
    J3D_HOOKFUNC(JonesHud_NewMenuItem);
    J3D_HOOKFUNC(JonesHud_FreeMenuItem);
    J3D_HOOKFUNC(JonesHud_UpdateItem);
    J3D_HOOKFUNC(JonesHud_RenderMenuItems);
    J3D_HOOKFUNC(JonesHud_RenderMenuItem);
    J3D_HOOKFUNC(JonesHud_MenuActivateItem);
    J3D_HOOKFUNC(JonesHud_ResetMenuItems);
    J3D_HOOKFUNC(JonesHud_SetLeftRightItems);
    J3D_HOOKFUNC(JonesHud_SetDownUpItems);
    J3D_HOOKFUNC(JonesHud_MenuExpandColumn);
    J3D_HOOKFUNC(JonesHud_MenuCollapseColumn);
    J3D_HOOKFUNC(JonesHud_InventoryItemChanged);
    J3D_HOOKFUNC(JonesHud_RenderInventoryItemChange);
    J3D_HOOKFUNC(JonesHud_RenderChangedItem);
    J3D_HOOKFUNC(JonesHud_ShowLevelCompleted);
    J3D_HOOKFUNC(JonesHud_OpenHelp);
    J3D_HOOKFUNC(JonesHud_ShowGameOverDialog);
    J3D_HOOKFUNC(JonesHud_GetHealthBarAlpha);
    J3D_HOOKFUNC(JonesHud_SetHealthBarAlpha);
    J3D_HOOKFUNC(JonesHud_CutsceneStart);
    J3D_HOOKFUNC(JonesHud_EnableInterface);
    J3D_HOOKFUNC(JonesHud_DrawCredits);
    J3D_HOOKFUNC(JonesHud_InitializeGameStatistics);
    J3D_HOOKFUNC(JonesHud_RestoreGameStatistics);
    J3D_HOOKFUNC(JonesHud_RestoreTreasuresStatistics);
}

void JonesHud_ResetGlobals(void)
{}

int JonesHud_Startup(void)
{
    if ( JonesHud_bStartup )
    {
        STDLOG_STATUS("Warning: System already initialized!\n");
        return 0;
    }

    jonesCog_g_bMenuVisible         = 0;
    jonesCog_g_bEnableGamesave      = 1;
    JonesHud_bRestoreActivated      = 0;
    JonesHud_bRestoreGameStatistics = 0;
    JonesHud_curEnduranceIndAlpha   = JonesHud_healthIndFade; // Added: Init endurance alpha to default fade value

    STD_ZEROMEM(JonesHud_aActivateKeyIds, sizeof(JonesHud_aActivateKeyIds)); // Fixed: 0 all elements of array
    STD_ZEROMEM(JonesHud_aCosSinTableScaled, sizeof(JonesHud_aCosSinTableScaled));
    STD_ZEROMEM(JonesHud_aCosSinTable, sizeof(JonesHud_aCosSinTable));

    // Init sin cos table
    size_t i = 0;
    for ( float x = 0.0f; x < 360.0f; x = x + 15.0f )
    {
        float cosv, sinv;
        stdMath_SinCos(x, &sinv, &cosv);
        JonesHud_aCosSinTable[i][1]   = sinv;
        JonesHud_aCosSinTable[i++][0] = cosv;
    }

    if ( jonesInventory_ResetGameStatistics() )
    {
        return 1;
    }

    JonesHud_bCutsceneStart    = 0;
    JonesHud_bInterfaceEnabled = 0;
    JonesHud_bStartup          = true;
    return 0;
}

void JonesHud_Shutdown(void)
{
    if ( !JonesHud_bStartup )
    {
        STDLOG_STATUS("Warning: System already shutdown!\n");
        return;
    }

    //nullsub_2(); // Called empty jonesInventory function
    JonesHud_pHealthIndBaseMat             = NULL;
    JonesHud_pHealthIndOverlayMat          = NULL;
    JonesHud_pHealthIndHitOverlayMat       = NULL;
    JonesHud_pHealthIndPoisonedOverlayMat  = NULL;
    JonesHud_pEnduranceIndOverlayMat       = NULL;

    JonesHud_pArrowUp             = NULL;
    JonesHud_pArrowDown           = NULL;

    JonesHud_pHudCanvas           = NULL;
    JonesHud_pHudCamera           = NULL;
    JonesHud_pMenuItemTextFont            = NULL;
    JonesHud_pMenuItemLinkedList  = NULL;
    JonesHud_pCurSelectedMenuItem = NULL;
    JonesHud_pCloseMenuItem       = NULL;
    JonesHud_pCurInvChangedItem   = NULL;

    JonesHud_widthAspectRatioScale   = 0.0f;
    JonesHud_heightAspectRatioScale  = 0.0f;
    JonesHud_hCurSndChannel          = 0;

    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_apMenuItems); ++i )
    {
        JonesHud_apMenuItems[i] = NULL;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_aSoundFxHandles); ++i )
    {
        JonesHud_aSoundFxHandles[i] = 0;
    }

    jonesConfig_Shutdown();
    JonesHud_bCutsceneStart    = 0;
    JonesHud_bInterfaceEnabled = 0;
    JonesHud_bStartup          = false;
}

int JonesHud_Open(void)
{
    if ( JonesHud_bOpen )
    {
        STDLOG_STATUS("Warning: System already open!\n");
        return 0;
    }

    if ( jonesInventory_Open() )
    {
        return 1;
    }

    JonesHud_hudState       = 0;
    JonesHud_pCloseMenuItem = NULL;

    //
    // Added: Load HUD variables from config file
    //

    //
    // Inventory menu config
    //
    JonesHud_msecMenuOpenStartSlideDuration = stdConfig_GetInt(JONESHUD_CFG_MENU_OPEN_START_SLIDE_DURATION, JonesHud_msecMenuOpenStartSlideDuration);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_OPEN_START_SLIDE_DURATION) )
    {
        stdConfig_SetInt(JONESHUD_CFG_MENU_OPEN_START_SLIDE_DURATION, JonesHud_msecMenuOpenStartSlideDuration);
    }

    JonesHud_msecMenuOpenSlideDuration = stdConfig_GetInt(JONESHUD_CFG_MENU_OPEN_SLIDE_DURATION, JonesHud_msecMenuOpenSlideDuration);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_OPEN_SLIDE_DURATION) )
    {
        stdConfig_SetInt(JONESHUD_CFG_MENU_OPEN_SLIDE_DURATION, JonesHud_msecMenuOpenSlideDuration);
    }

    JonesHud_msecMenuCloseStartSlideDuration = stdConfig_GetInt(JONESHUD_CFG_MENU_CLOSE_START_SLIDE_DURATION, JonesHud_msecMenuCloseStartSlideDuration);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_CLOSE_START_SLIDE_DURATION) )
    {
        stdConfig_SetInt(JONESHUD_CFG_MENU_CLOSE_START_SLIDE_DURATION, JonesHud_msecMenuCloseStartSlideDuration);
    }

    JonesHud_msecMenuCloseSlideDuration = stdConfig_GetInt(JONESHUD_CFG_MENU_CLOSE_SLIDE_DURATION, JonesHud_msecMenuCloseSlideDuration);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_CLOSE_SLIDE_DURATION) )
    {
        stdConfig_SetInt(JONESHUD_CFG_MENU_CLOSE_SLIDE_DURATION, JonesHud_msecMenuCloseSlideDuration);
    }

    // Menu music config
    JonesHud_bMenuMusicEnabled = stdConfig_GetBool(JONESHUD_CFG_MENU_MUSICENABLED, JonesHud_bMenuMusicEnabled);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_MUSICENABLED) )
    {
        stdConfig_SetBool(JONESHUD_CFG_MENU_MUSICENABLED, JonesHud_bMenuMusicEnabled);
    }

    stdConfig_GetString(JONESHUD_CFG_MENU_MUSICFILENAME, JonesHud_aMenuMusicFilename,
        sizeof(JonesHud_aMenuMusicFilename), JonesHud_aMenuMusicFilename);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_MUSICFILENAME) )
    {
        stdConfig_SetString(JONESHUD_CFG_MENU_MUSICFILENAME, JonesHud_aMenuMusicFilename);
    }

    JonesHud_menuMusicVolume = stdConfig_GetFloat(JONESHUD_CFG_MENU_MUSICEVOLUME, JonesHud_menuMusicVolume);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_MUSICEVOLUME) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_MENU_MUSICEVOLUME, JonesHud_menuMusicVolume);
    }

    JonesHud_menuMusicFadeDuratuion = stdConfig_GetFloat(JONESHUD_CFG_MENU_FADEDURATION, JonesHud_menuMusicFadeDuratuion);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_FADEDURATION) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_MENU_FADEDURATION, JonesHud_menuMusicFadeDuratuion);
    }

    // Menu item config
    JonesHud_menuItemScale = stdConfig_GetFloat(JONESHUD_CFG_MENU_ITEM_SCALE, JonesHud_menuItemScale);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_ITEM_SCALE) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_MENU_ITEM_SCALE, JonesHud_menuItemScale);
    }

    JonesHud_menuItemTextPosY = stdConfig_GetFloat(JONESHUD_CFG_MENU_ITEM_TEXT_POS_Y, JonesHud_menuItemTextPosY);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_ITEM_TEXT_POS_Y) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_MENU_ITEM_TEXT_POS_Y, JonesHud_menuItemTextPosY);
    }

    JonesHud_menuItemTextSize = stdConfig_GetFloat(JONESHUD_CFG_MENU_ITEM_TEXT_SIZE, JonesHud_menuItemTextSize);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_ITEM_TEXT_SIZE) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_MENU_ITEM_TEXT_SIZE, JonesHud_menuItemTextSize);
    }

    stdConfig_GetString(JONESHUD_CFG_MENU_ITEM_FONT_FILENAME, JonesHud_aMenuItemTextFontFilename,
        sizeof(JonesHud_aMenuItemTextFontFilename), JonesHud_aMenuItemTextFontFilename);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_ITEM_FONT_FILENAME) )
    {
        stdConfig_SetString(JONESHUD_CFG_MENU_ITEM_FONT_FILENAME, JonesHud_aMenuItemTextFontFilename);
    }

    JonesHud_msecMenuItemMoveDuration = stdConfig_GetInt(JONESHUD_CFG_MENU_ITEM_MOVEDURATION, JonesHud_msecMenuItemMoveDuration);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_ITEM_MOVEDURATION) )
    {
        stdConfig_SetInt(JONESHUD_CFG_MENU_ITEM_MOVEDURATION, JonesHud_msecMenuItemMoveDuration);
    }

    // Added: Initialize current move duration to configured value
    JonesHud_msecMenuItemMoveCurDuration = JonesHud_msecMenuItemMoveDuration;

    // Menu column config
    JonesHud_menuColumnHeight = stdConfig_GetFloat(JONESHUD_CFG_MENU_COLUMN_HEIGHT, JonesHud_menuColumnHeight);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_COLUMN_HEIGHT) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_MENU_COLUMN_HEIGHT, JonesHud_menuColumnHeight);
    }

    JonesHud_menuColumnSpacing = stdConfig_GetFloat(JONESHUD_CFG_MENU_COLUMN_SPACING, JonesHud_menuColumnSpacing);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_COLUMN_SPACING) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_MENU_COLUMN_SPACING, JonesHud_menuColumnSpacing);
    }

    JonesHud_msecMenuColumnExpandDuration = stdConfig_GetInt(JONESHUD_CFG_MENU_COLUMN_EXPAND_DURATION, JonesHud_msecMenuColumnExpandDuration);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_COLUMN_EXPAND_DURATION) )
    {
        stdConfig_SetInt(JONESHUD_CFG_MENU_COLUMN_EXPAND_DURATION, JonesHud_msecMenuColumnExpandDuration);
    }

    JonesHud_msecMenuColumnCollapseDuration = stdConfig_GetInt(JONESHUD_CFG_MENU_COLUMN_COLLAPSE_DURATION, JonesHud_msecMenuColumnCollapseDuration);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_COLUMN_COLLAPSE_DURATION) )
    {
        stdConfig_SetInt(JONESHUD_CFG_MENU_COLUMN_COLLAPSE_DURATION, JonesHud_msecMenuColumnCollapseDuration);
    }

    JonesHud_menuColumnHorizontalSpacing = stdConfig_GetFloat(JONESHUD_CFG_MENU_COLUMN_HORIZONTAL_SPACING, JonesHud_menuColumnHorizontalSpacing);
    if ( !stdConfig_Contains(JONESHUD_CFG_MENU_COLUMN_HORIZONTAL_SPACING) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_MENU_COLUMN_HORIZONTAL_SPACING, JonesHud_menuColumnHorizontalSpacing);
    }

    //
    // Item changed config
    //
    JonesHud_msecItemChangedShowDuration = stdConfig_GetInt(JONESHUD_CFG_ITEM_CHANGED_SHOW_DURATION, JonesHud_msecItemChangedShowDuration);
    if ( !stdConfig_Contains(JONESHUD_CFG_ITEM_CHANGED_SHOW_DURATION) )
    {
        stdConfig_SetInt(JONESHUD_CFG_ITEM_CHANGED_SHOW_DURATION, JonesHud_msecItemChangedShowDuration);
    }

    JonesHud_itemChangedSlideEaseInRate = stdConfig_GetFloat(JONESHUD_CFG_ITEM_CHANGED_SLIDE_EASE_IN_RATE, JonesHud_itemChangedSlideEaseInRate);
    if ( !stdConfig_Contains(JONESHUD_CFG_ITEM_CHANGED_SLIDE_EASE_IN_RATE) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_ITEM_CHANGED_SLIDE_EASE_IN_RATE, JonesHud_itemChangedSlideEaseInRate);
    }

    JonesHud_itemChangedSlideSpeedRate = stdConfig_GetInt(JONESHUD_CFG_ITEM_CHANGED_SLIDE_SPEED_RATE, JonesHud_itemChangedSlideSpeedRate);
    if ( !stdConfig_Contains(JONESHUD_CFG_ITEM_CHANGED_SLIDE_SPEED_RATE) )
    {
        stdConfig_SetInt(JONESHUD_CFG_ITEM_CHANGED_SLIDE_SPEED_RATE, JonesHud_itemChangedSlideSpeedRate);
    }

    //
    // Health indicator config
    //
    JonesHud_healthIndSize = stdConfig_GetFloat(JONESHUD_CFG_HEALTH_IND_SIZE, JonesHud_healthIndSize);
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_SIZE) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_HEALTH_IND_SIZE, JonesHud_healthIndSize);
    }

    JonesHud_healthIndPosX = stdConfig_GetFloat(JONESHUD_CFG_HEALTH_IND_POS_X, JonesHud_healthIndPosX);
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_POS_X) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_HEALTH_IND_POS_X, JonesHud_healthIndPosX);
    }

    JonesHud_healthIndPosY = stdConfig_GetFloat(JONESHUD_CFG_HEALTH_IND_POS_Y, JonesHud_healthIndPosY);
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_POS_Y) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_HEALTH_IND_POS_Y, JonesHud_healthIndPosY);
    }

    JonesHud_healthIndFade = stdConfig_GetFloat(JONESHUD_CFG_HEALTH_IND_FADE, JonesHud_healthIndFade);
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_FADE) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_HEALTH_IND_FADE, JonesHud_healthIndFade);
    }

    JonesHud_msecHealthIndFadeTime = stdConfig_GetInt(JONESHUD_CFG_HEALTH_IND_FADE_TIME, JonesHud_msecHealthIndFadeTime);
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_FADE_TIME) )
    {
        stdConfig_SetInt(JONESHUD_CFG_HEALTH_IND_FADE_TIME, JonesHud_msecHealthIndFadeTime);
    }

    // Load health indicator colors
    tStdColor colorFull = stdConfig_GetColor(JONESHUD_CFG_HEALTH_IND_COLOR_FULL, JonesHud_RdVector4ToStdColor(&JonesHud_healthIndColorFull));
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_COLOR_FULL) )
    {
        stdConfig_SetColorRGB(JONESHUD_CFG_HEALTH_IND_COLOR_FULL, colorFull);
    }
    JonesHud_StdColorToRdVector4(colorFull, &JonesHud_healthIndColorFull);

    tStdColor colorMed = stdConfig_GetColor(JONESHUD_CFG_HEALTH_IND_COLOR_MED, JonesHud_RdVector4ToStdColor(&JonesHud_healthIndColorMedium));
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_COLOR_MED) )
    {
        stdConfig_SetColorRGB(JONESHUD_CFG_HEALTH_IND_COLOR_MED, colorMed);
    }
    JonesHud_StdColorToRdVector4(colorMed, &JonesHud_healthIndColorMedium);

    tStdColor colorLow = stdConfig_GetColor(JONESHUD_CFG_HEALTH_IND_COLOR_LOW, JonesHud_RdVector4ToStdColor(&JonesHud_healthIndColorLow));
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_COLOR_LOW) )
    {
        stdConfig_SetColorRGB(JONESHUD_CFG_HEALTH_IND_COLOR_LOW, colorLow);
    }
    JonesHud_StdColorToRdVector4(colorLow, &JonesHud_healthIndColorLow);

    tStdColor colorCritical = stdConfig_GetColor(JONESHUD_CFG_HEALTH_IND_COLOR_CRITICAL, JonesHud_RdVector4ToStdColor(&JonesHud_healthIndColorCritical));
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_COLOR_CRITICAL) )
    {
        stdConfig_SetColorRGB(JONESHUD_CFG_HEALTH_IND_COLOR_CRITICAL, colorCritical);
    }
    JonesHud_StdColorToRdVector4(colorCritical, &JonesHud_healthIndColorCritical);

    //
    // Endurance indicator config
    //
    JonesHud_enduranceIndSize = stdConfig_GetFloat(JONESHUD_CFG_ENDURANCE_IND_SIZE, JonesHud_enduranceIndSize);
    if ( !stdConfig_Contains(JONESHUD_CFG_ENDURANCE_IND_SIZE) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_ENDURANCE_IND_SIZE, JonesHud_enduranceIndSize);
    }

    JonesHud_enduranceIndPosX = stdConfig_GetFloat(JONESHUD_CFG_ENDURANCE_IND_POS_X, JonesHud_enduranceIndPosX);
    if ( !stdConfig_Contains(JONESHUD_CFG_ENDURANCE_IND_POS_X) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_ENDURANCE_IND_POS_X, JonesHud_enduranceIndPosX);
    }

    JonesHud_enduranceIndPosY = stdConfig_GetFloat(JONESHUD_CFG_ENDURANCE_IND_POS_Y, JonesHud_enduranceIndPosY);
    if ( !stdConfig_Contains(JONESHUD_CFG_ENDURANCE_IND_POS_Y) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_ENDURANCE_IND_POS_Y, JonesHud_enduranceIndPosY);
    }

    JonesHud_enduranceIndFade = stdConfig_GetFloat(JONESHUD_CFG_ENDURANCE_IND_FADE, JonesHud_enduranceIndFade);
    if ( !stdConfig_Contains(JONESHUD_CFG_ENDURANCE_IND_FADE) )
    {
        stdConfig_SetFloat(JONESHUD_CFG_ENDURANCE_IND_FADE, JonesHud_enduranceIndFade);
    }

    JonesHud_msecEnduranceIndTime = stdConfig_GetInt(JONESHUD_CFG_ENDURANCE_IND_FADE_TIME, JonesHud_msecEnduranceIndTime);
    if ( !stdConfig_Contains(JONESHUD_CFG_ENDURANCE_IND_FADE_TIME) )
    {
        stdConfig_SetInt(JONESHUD_CFG_ENDURANCE_IND_FADE_TIME, JonesHud_msecEnduranceIndTime);
    }

    stdConfig_GetString(JONESHUD_CFG_HEALTH_IND_BASE_MAT, JonesHud_aHealthIndBaseMatFilename,
        sizeof(JonesHud_aHealthIndBaseMatFilename), JonesHud_aHealthIndBaseMatFilename);
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_BASE_MAT) )
    {
        stdConfig_SetString(JONESHUD_CFG_HEALTH_IND_BASE_MAT, JonesHud_aHealthIndBaseMatFilename);
    }

    stdConfig_GetString(JONESHUD_CFG_HEALTH_IND_OVERLAY_MAT, JonesHud_aHealthIndOverlayMatFilename,
        sizeof(JonesHud_aHealthIndOverlayMatFilename), JonesHud_aHealthIndOverlayMatFilename);
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_OVERLAY_MAT) )
    {
        stdConfig_SetString(JONESHUD_CFG_HEALTH_IND_OVERLAY_MAT, JonesHud_aHealthIndOverlayMatFilename);
    }

    stdConfig_GetString(JONESHUD_CFG_HEALTH_IND_HIT_OVERLAY_MAT, JonesHud_aHealthIndHitOverlayMatFilename,
        sizeof(JonesHud_aHealthIndHitOverlayMatFilename), JonesHud_aHealthIndHitOverlayMatFilename);
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_HIT_OVERLAY_MAT) )
    {
        stdConfig_SetString(JONESHUD_CFG_HEALTH_IND_HIT_OVERLAY_MAT, JonesHud_aHealthIndHitOverlayMatFilename);
    }

    stdConfig_GetString(JONESHUD_CFG_HEALTH_IND_POISONED_OVERLAY_MAT, JonesHud_aHealthIndPoisonedOverlayMatFilename,
        sizeof(JonesHud_aHealthIndPoisonedOverlayMatFilename), JonesHud_aHealthIndPoisonedOverlayMatFilename);
    if ( !stdConfig_Contains(JONESHUD_CFG_HEALTH_IND_POISONED_OVERLAY_MAT) )
    {
        stdConfig_SetString(JONESHUD_CFG_HEALTH_IND_POISONED_OVERLAY_MAT, JonesHud_aHealthIndPoisonedOverlayMatFilename);
    }

    stdConfig_GetString(JONESHUD_CFG_ENDURANCE_IND_OVERLAY_MAT, JonesHud_aEnduranceIndOverlayMatFilename,
        sizeof(JonesHud_aEnduranceIndOverlayMatFilename), JonesHud_aEnduranceIndOverlayMatFilename);
    if ( !stdConfig_Contains(JONESHUD_CFG_ENDURANCE_IND_OVERLAY_MAT) )
    {
        stdConfig_SetString(JONESHUD_CFG_ENDURANCE_IND_OVERLAY_MAT, JonesHud_aEnduranceIndOverlayMatFilename);
    }

    // Load endurance indicator colors
    tStdColor colorOxygen = stdConfig_GetColor(JONESHUD_CFG_ENDURANCE_IND_COLOR_OXYGEN, JonesHud_RdVector4ToStdColor(&JonesHud_enduranceIndColorOxygen));
    if ( !stdConfig_Contains(JONESHUD_CFG_ENDURANCE_IND_COLOR_OXYGEN) )
    {
        stdConfig_SetColorRGB(JONESHUD_CFG_ENDURANCE_IND_COLOR_OXYGEN, colorOxygen);
    }
    JonesHud_StdColorToRdVector4(colorOxygen, &JonesHud_enduranceIndColorOxygen);

    tStdColor colorRaft = stdConfig_GetColor(JONESHUD_CFG_ENDURANCE_IND_COLOR_RAFT, JonesHud_RdVector4ToStdColor(&JonesHud_enduranceIndColorRaft));
    if ( !stdConfig_Contains(JONESHUD_CFG_ENDURANCE_IND_COLOR_RAFT) )
    {
        stdConfig_SetColorRGB(JONESHUD_CFG_ENDURANCE_IND_COLOR_RAFT, colorRaft);
    }
    JonesHud_StdColorToRdVector4(colorRaft, &JonesHud_enduranceIndColorRaft);

    tStdColor colorIMPEnergy = stdConfig_GetColor(JONESHUD_CFG_ENDURANCE_IND_COLOR_IMP_ENERGY, JonesHud_RdVector4ToStdColor(&JonesHud_enduranceIndColorIMPEnergy));
    if ( !stdConfig_Contains(JONESHUD_CFG_ENDURANCE_IND_COLOR_IMP_ENERGY) )
    {
        stdConfig_SetColorRGB(JONESHUD_CFG_ENDURANCE_IND_COLOR_IMP_ENERGY, colorIMPEnergy);
    }
    JonesHud_StdColorToRdVector4(colorIMPEnergy, &JonesHud_enduranceIndColorIMPEnergy);

    //
    // Load HUD materials
    //
    JonesHud_pHealthIndBaseMat = rdMaterial_Load(JonesHud_aHealthIndBaseMatFilename);
    if ( !JonesHud_pHealthIndBaseMat )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", JonesHud_aHealthIndBaseMatFilename);
        goto error;
    }

    JonesHud_pHealthIndOverlayMat = rdMaterial_Load(JonesHud_aHealthIndOverlayMatFilename);
    if ( !JonesHud_pHealthIndOverlayMat )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", JonesHud_aHealthIndOverlayMatFilename);
        goto error;
    }

    JonesHud_pHealthIndHitOverlayMat = rdMaterial_Load(JonesHud_aHealthIndHitOverlayMatFilename);
    if ( !JonesHud_pHealthIndHitOverlayMat )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", JonesHud_aHealthIndHitOverlayMatFilename);
        goto error;
    }

    JonesHud_pHealthIndPoisonedOverlayMat = rdMaterial_Load(JonesHud_aHealthIndPoisonedOverlayMatFilename);
    if ( !JonesHud_pHealthIndPoisonedOverlayMat )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", JonesHud_aHealthIndPoisonedOverlayMatFilename);
        goto error;
    }

    // Removed: Omitted loading of unused materials
#ifndef J3D_QOL_IMPROVEMENTS
    JonesHud_pArrowUp = rdMaterial_Load("gen_4arrow_up.mat");
    if ( !JonesHud_pArrowUp )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "gen_4arrow_up.mat");
        goto error;
    }

    // TODO: not needed?
    JonesHud_pArrowDown = rdMaterial_Load("gen_4arrow_down.mat");
    if ( !JonesHud_pArrowDown )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "gen_4arrow_down.mat");
        goto error;
    }
#endif

    JonesHud_pEnduranceIndOverlayMat = rdMaterial_Load(JonesHud_aEnduranceIndOverlayMatFilename);
    if ( !JonesHud_pEnduranceIndOverlayMat )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", JonesHud_aEnduranceIndOverlayMatFilename);
        goto error;
    }

    //
    // Initialize menu items
    //
    if ( !JonesHud_InitializeMenu() )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load initialize menu items.\n");
        goto error;
    }

    //
    // Initialize dialog data
    //
    if ( !jonesConfig_Startup() )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't initialize dialog data.\n"); // Fixed typo: 'intialize'
        goto error;
    }

    //
    // Initialize menu sounds
    //
    JonesHud_InitializeMenuSounds();

    //
    // Setup HUD camera
    //
    rdCamera_sub_4C60B0(true);

    JonesHud_pHudCamera = rdCamera_New(10.0f, 0, 1.0f, 100.0f, 1.0f);
    if ( !JonesHud_pHudCamera )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't create Hud camera.\n");
        goto error;
    }

    JonesHud_pHudCanvas = rdCanvas_New(
        0x0,// 00x0 flags to use buffer size instead of provided rect sizes
        &stdDisplay_g_backBuffer,
        64,// left
        (int)JonesHud_healthIndRect.y, // JonesHud_healthIndRect might not be initialized
        576,// right -> 640 - 64 = 576
        (int)(JonesHud_healthIndRect.y + 64.0f) // JonesHud_healthIndRect might not be initialized 
    );

    if ( !JonesHud_pHudCanvas )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't create Hud canvas.\n");
        goto error;
    }

    rdCamera_SetCanvas(JonesHud_pHudCamera, JonesHud_pHudCanvas);

    uint32_t height, width;
    stdDisplay_GetBackBufferSize(&width, &height);
    rdCamera_SetFrustrum(JonesHud_pHudCamera, JonesHud_pHudCamera->pFrustum, 0, 0, width - 1, height - 1);

    rdCamera* pCurCamera = rdCamera_g_pCurCamera;
    rdCamera_SetCurrent(JonesHud_pHudCamera);

    rdMatrix_Identity34(&JonesHud_camMatrix);
    rdMatrix_PostRotate34(&JonesHud_camMatrix, &JonesHud_camViewPyr);
    JonesHud_camMatrix.dvec = JonesHud_camViewPos;

    rdCamera_Update(&JonesHud_camMatrix);
    JonesHud_nearClipPlane = JonesHud_pHudCamera->pFrustum->nearPlane + 1.0f;// probably unused

    // Restore previous camera
    rdCamera_SetCurrent(pCurCamera);

    rdVector4 hudAmbientColor = { .red = 1.0f, .green = 1.0f, .blue = 1.0f, .alpha = 0.0f };
    rdCamera_SetAmbientLight(JonesHud_pHudCamera, &hudAmbientColor);

    rdCamera_sub_4C60B0(false);

    //
    // Initialize HUD state variables
    //
    JonesHud_bBonusMapBought = 0;
    JonesHud_bIMPState       = 0;
    JonesHud_hudState        = 0;

    // 
    // Load menu font
    //
    JonesHud_pMenuItemTextFont = rdFont_Load(JonesHud_aMenuItemTextFontFilename);
    if ( !JonesHud_pMenuItemTextFont )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", JonesHud_aMenuItemTextFontFilename);
        goto error;
    }

    //
    // Initialize auto aim reticle system
    // Added: New
    JonesReticle_Open();

    //
    // Success
    //
    JonesHud_bOpen             = true;
    JonesHud_bCutsceneStart    = 0;
    JonesHud_bInterfaceEnabled = 0;
    return 0;

error:
    STDLOG_ERROR(JonesMain_g_aErrorBuffer);

    JonesHud_Close();
    JonesHud_bOpen = false;
    return 1;
}

void JonesHud_Close(void)
{
    // Added
    JonesReticle_Close();

    JonesHud_pMenuItemLinkedList = 0;
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_apMenuItems); ++i )
    {
        if ( JonesHud_apMenuItems[i] )
        {
            JonesHud_FreeMenuItem(JonesHud_apMenuItems[i]);
            JonesHud_apMenuItems[i] = NULL;
        }
    }

    if ( JonesHud_pHudCamera )
    {
        rdCamera_Free(JonesHud_pHudCamera);
        JonesHud_pHudCamera = NULL;
    }

    if ( JonesHud_pHudCanvas )
    {
        rdCanvas_Free(JonesHud_pHudCanvas);
        JonesHud_pHudCanvas = NULL;
    }

    if ( JonesHud_pMenuItemTextFont )
    {
        rdFont_Free(JonesHud_pMenuItemTextFont);
        JonesHud_pMenuItemTextFont = NULL;
    }

    if ( JonesHud_pCurInvChangedItem )
    {
        JonesHud_FreeMenuItem(JonesHud_pCurInvChangedItem);
        JonesHud_pCurInvChangedItem = NULL;
    }

    JonesHud_pCurSelectedMenuItem = NULL;
    JonesHud_pCloseMenuItem       = NULL;
    JonesHud_pCurInvChangedItem   = NULL;

    JonesHud_bExitActivated         = 0;
    JonesHud_dword_55515C           = 0;
    JonesHud_bRestoreActivated      = 0;
    JonesHud_bRestoreGameStatistics = 0;
    JonesHud_bItemActivated         = 0;
    JonesHud_bMenuEnabled           = 0;
    JonesHud_bHealthIndFadeSet      = 0;
    JonesHud_bCutsceneStart         = 0;
    JonesHud_bInterfaceEnabled      = 0;

    jonesInventory_Close();

    // TODO: Why is this check so far down here? Maybe should be moved to the start of function
    if ( JonesHud_bOpen )
    {
        JonesHud_bOpen = false;
    }
    else
    {
        STDLOG_STATUS("Warning: System already closed!\n");
    }
}

void JonesHud_ToggleMenu(void)
{
    if ( sithPlayer_g_pLocalPlayerThing
        && ((sithPlayer_g_pLocalPlayerThing->flags & SITH_TF_DYING) != 0
            || (sithPlayer_g_pLocalPlayerThing->flags & SITH_TF_DESTROYED) != 0) )
    {
        jonesCog_g_bShowHealthHUD  = 0;
        jonesCog_g_bMenuVisible    = 0;
        jonesCog_g_bEnableGamesave = 0;
        jonesCog_g_dword_17EFFCC   = 0;

        JonesHud_hudState &= ~0x10;
        JonesHud_ShowGameOverDialog(1);
    }
    else if ( JonesHud_bMenuEnabled )
    {
        if ( (JonesHud_hudState & 0x01) != 0 ) // If menu already opened then close it
        {
            if ( (JonesHud_hudState & 0x04) == 0 && (JonesHud_hudState & 0x08) == 0 && (JonesHud_hudState & 0x01) != 0 )
            {
                jonesCog_g_bMenuVisible = 0;
                JonesHud_MenuClose();
            }
        }
        else // ... otherwise open menu here
        {
            // If item was picked up opening the menu is postponed until animation of picked-up item finish playing
            // TODO: Maybe we should discard the animation and just open the menu?
            if ( JonesHud_pCurInvChangedItem )
            {
                JonesHud_bShowMenu = 1;
            }
            else
            {
                sithGamesave_SetThumbnailImage(&stdDisplay_g_backBuffer);
                jonesCog_g_bMenuVisible = 1;
                JonesHud_bShowMenu      = 0;
            }

            JonesHud_healthIndAlpha = 1.0f;
            // Note, menu is opened on the next render tick
        }
    }
}

void J3DAPI JonesHud_EnableMenu(int bEnable)
{
    JonesHud_bMenuEnabled = bEnable;
}

int JonesHud_IsMenuEnabled(void)
{
    return JonesHud_bMenuEnabled;
}

void JonesHud_Process(void) // maybe this function should be called something eles??
{
    if ( sithPlayer_g_pLocalPlayerThing )
    {
        // Update HUD screen vars
        JonesHud_Update(sithWorld_g_pCurrentWorld);

        if ( sithPlayer_g_pLocalPlayerThing->type
            && ((sithPlayer_g_pLocalPlayerThing->flags & SITH_TF_DYING) != 0
                || (sithPlayer_g_pLocalPlayerThing->flags & SITH_TF_DESTROYED) != 0) )
        {
            // Player died, wait 5 sec before then play anchor music then  show game over dialog

            sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health = 0.0f;
            jonesCog_g_bShowHealthHUD  = 0;
            jonesCog_g_bMenuVisible    = 0;
            jonesCog_g_bEnableGamesave = 0;
            jonesCog_g_dword_17EFFCC   = 0;

            if ( (JonesHud_hudState & 0x10) != 0 )
            {
                if ( JonesHud_HasTimeElapsed(5000u, JonesHud_msecGameOverWaitDeltaTime, JonesHud_msecTime) )
                {
                    JonesHud_ShowGameOverDialog(1);
                    return;
                }
            }
            else
            {
                // Wait 5 sec, then show gameover dialog
                JonesHud_hudState |= 0x10u;     // mark gameover 
                JonesHud_msecGameOverWaitDeltaTime = JonesHud_msecTime + JonesHud_msecGameOverWait;
            }
        }

        if ( JonesHud_bCutsceneStart && (JonesHud_hudState & 0x01) == 0 && !jonesCog_g_bMenuVisible )
        {
            // Menu is closed, start previously started cutscene
            JonesHud_bCutsceneStart = 0;
            jonesCog_StartCutscene(NULL);
        }

        if ( JonesHud_bInterfaceEnabled && (JonesHud_hudState & 0x01) == 0 && !jonesCog_g_bMenuVisible )
        {
            // Menu is closed show in game HUD
            JonesHud_bInterfaceEnabled = 0;
            jonesCog_ToggleInterface(NULL);
        }

        if ( JonesHud_bExitActivated && !JonesHud_hudState )
        {
            // Exit game
            JonesMain_CloseWindow();
        }
        else if ( JonesHud_bRestoreActivated && !JonesHud_hudState && strlen(JonesHud_aSlectedNdsFilePath) > 0 )
        {
            // Load game activated

            sithSoundMixer_StopAll();
            if ( sithCamera_g_pCurCamera )
            {
                sithCamera_ResetAllCameras();
            }

            sithGamesave_Restore(JonesHud_aSlectedNdsFilePath, /*bNotifyCog=*/1);
            STD_ZEROMEM(JonesHud_aSlectedNdsFilePath, sizeof(JonesHud_aSlectedNdsFilePath));
            JonesHud_bRestoreActivated = 0;
        }
        else
        {
            // Render Menu

            if ( JonesHud_bMapOpen && !sithPlayerControls_g_bCutsceneMode && (JonesHud_hudState & 0x01) == 0 )
            {
                JonesHud_bMapOpen = 0;
                sithOverlayMap_ToggleMap();
            }

            // Clear render cache before switching to rendering the HUD
            rdCache_FlushAlpha();
            rdCache_Flush();

            // Render reticle before the any other HUD component
            // Note, also has to be drawn before switching camera to HUD camera
            // because game camera matrix is required for transforming target position to screen space.
            //
            // Added: v0.4
            if ( (JonesHud_hudState & 0x10) == 0 && !sithPlayerControls_g_bCutsceneMode ) // if not game over and not in cutscene mode
            {
                JonesReticle_Render();
            }

            // Change cur camera to HUD camera
            rdCamera* pCurCam = rdCamera_g_pCurCamera;
            rdCamera_SetCurrent(JonesHud_pHudCamera);
            rdCamera_Update(&JonesHud_camMatrix);

            if ( !JonesHud_RenderHealthIndicator(/*bFade=*/JonesHud_bFadeHealthHUD) )
            {
                if ( !JonesHud_bHealthIndFadeSet || JonesHud_healthIndAlpha >= JonesHud_healthIndFade )
                {
                    float maxHealth = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.maxHealth;
                    float healthState = 0.0f;
                    if ( maxHealth > 0.0f )
                    {
                        healthState = (300.0f * sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health / maxHealth);
                    }
                    JonesHud_RenderHealthIndicatorHit(roundf(healthState));
                }

                // DRAW endurance indicator
                if ( sithPlayer_g_pLocalPlayerThing->pInSector
                    && (sithPlayer_g_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
                {
                    float breathState = (float)(100 * sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.endurance.msecUnderwater / SITHACTOR_MAX_UNDERWATER_MSEC);
                    JonesHud_RenderEnduranceIndicator(breathState);
                }
                else if ( (sithPlayer_g_pLocalPlayerThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
                {
                    float raftState = (float)sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.endurance.raftLeakDamage * 0.001f;
                    JonesHud_RenderEnduranceIndicator(raftState);
                }
                else
                {
                    sithInventory_GetCurrentWeapon(sithPlayer_g_pLocalPlayerThing);
                    float impState = sithPlayer_g_impState * 1000.0f / 1800.0f; // Convert to percentage
                    if ( impState <= 0.0f || impState > 100.0f )
                    {
                        JonesHud_bIMPState = 0;
                    }
                    else
                    {
                        JonesHud_bIMPState = 1;
                        JonesHud_RenderEnduranceIndicator(impState);
                    }
                }
            }

            if ( JonesHud_pCurInvChangedItem ) // Is item pickup animation playing?
            {
                // Continue rendering the item pickup animation
                JonesHud_RenderInventoryItemChange();
            }
            else
            {
                sithInventory_SetCurrentItem(sithPlayer_g_pLocalPlayerThing, 0);
                if ( (JonesHud_hudState & 0x01) != 0 ) // Is inventory menu opened?
                {
                    // Inventory menu opened

                    if ( (Sound_GetChannelFlags(JonesHud_hCurSndChannel) & SOUND_CHANNEL_PLAYING) == 0 ) // !playing?
                    {
                        JonesHud_hCurSndChannel = SOUND_INVALIDHANDLE;
                    }

                    // Process & render inventory menu items
                    // This is the main logic
                    JonesHud_ProcessInventoryMenu();
                }
                else if ( jonesCog_g_bMenuVisible )
                {
                    // Opening inventory menu here
                    if ( sithOverlayMap_IsMapVisible() )
                    {
                        JonesHud_bMapOpen = 1;
                        sithOverlayMap_ToggleMap();
                    }

                    stdControl_ResetMousePos();
                    JonesMain_PauseGame();
                    JonesReticle_Pause();
                    JonesHud_MenuOpen();
                }
                else if ( JonesHud_bShowMenu )
                {
                    // Initialize opening inventory menu
                    // Deferred from JonesHud_ToggleMenu when item pickup animation is playing
                    sithGamesave_SetThumbnailImage(&stdDisplay_g_backBuffer);
                    jonesCog_g_bMenuVisible = 1;
                    JonesHud_healthIndAlpha = 1.0f;
                    JonesHud_bShowMenu      = 0;
                }
            }

            if ( JonesHud_selectedWeaponID != -1 && !sithWeapon_IsMountingWeapon(sithPlayer_g_pLocalPlayerThing) )
            {
                sithWeapon_SelectWeapon(sithPlayer_g_pLocalPlayerThing, JonesHud_selectedWeaponID);
                JonesHud_selectedWeaponID = -1;
            }

            rdCache_FlushAlpha();
            rdCache_Flush();
            rdCamera_SetCurrent(pCurCam);
            sithWorld_g_pCurrentWorld->state &= ~SITH_WORLD_STATE_INIT_HUD;
        }
    }
}

void J3DAPI JonesHud_Update(const SithWorld* pWorld)
{
    float prevWidthScalar = JonesHud_widthAspectRatioScale;
    float prevHeighScalar = JonesHud_heightAspectRatioScale;

    uint32_t width, height;
    stdDisplay_GetBackBufferSize(&width, &height);

    float wratio = (float)width / RD_REF_WIDTH;
    JonesHud_widthAspectRatioScale = wratio;

    float hratio = (float)height / RD_REF_HEIGHT;
    JonesHud_heightAspectRatioScale = hratio;

    JonesHud_itemAspectScaleSize     = (RD_REF_APECTRATIO / (width / height)) * JonesHud_menuItemScale;
    JonesHud_itemAspectScalePosition = JonesHud_widthAspectRatioScale / JonesHud_heightAspectRatioScale;


    size_t curTime = stdPlatform_GetTimeMsec();
    int bCanvasUpdated = 0;

    if ( (pWorld->state & SITH_WORLD_STATE_INIT_HUD) != 0 )
    {
        JonesHud_msecMenuItemMoveCurDuration = JonesHud_msecMenuItemMoveDuration;
        JonesHud_hudState                 = 0;
        JonesHud_bHealthIndFadeSet        = 0;
        JonesHud_bExitActivated           = 0;
        JonesHud_dword_55515C             = 0;
        JonesHud_bItemActivated           = 0;
        JonesHud_bShowMenu                = 0;
        JonesHud_msecDeltaTime            = 1;
        JonesHud_bCutsceneStart           = 0;
        JonesHud_bInterfaceEnabled        = 0;

        JonesHud_InitializeGameStatistics();
        bCanvasUpdated = JonesHud_SetCanvasSize(width, height);
    }
    else if ( curTime <= JonesHud_msecTime )
    {
        // Fixed: Corrected delta time calculation on time wrap-around
        JonesHud_msecDeltaTime = (UINT32_MAX - JonesHud_msecTime + 1) + curTime;
    }
    else
    {
        JonesHud_msecDeltaTime = curTime - JonesHud_msecTime;
    }

    JonesHud_msecTime = curTime;
    if ( prevWidthScalar != JonesHud_widthAspectRatioScale || prevHeighScalar != JonesHud_heightAspectRatioScale )
    {
        JonesHud_UpdateHUDLayout(width, height);
        JonesHud_UpdateSinCosTable();
        if ( !bCanvasUpdated )
        {
            JonesHud_SetCanvasSize(width, height);
        }
    }

    // Added: Update aiming reticle
    JonesReticle_Update(JonesHud_msecDeltaTime / 1000.f);
}

int J3DAPI JonesHud_SetCanvasSize(int width, int height)
{
    if ( !JonesHud_pHudCamera || !JonesHud_pHudCamera->pFrustum )
    {
        return 0;
    }

    rdCamera_sub_4C60B0(true); // This changes camera z plane
    if ( JonesHud_pHudCanvas )
    {
        rdCanvas_Free(JonesHud_pHudCanvas);
        JonesHud_pHudCanvas = NULL;
        JonesHud_pHudCanvas = rdCanvas_New(
            0, // 00x0 flags to use buffer size instead of provided rect sizes
            &stdDisplay_g_backBuffer,
            64, // left
            (int32_t)JonesHud_healthIndRect.y, // top 
            576,  // right -> 640 - 64 = 576
            (int32_t)(JonesHud_healthIndRect.y + 64.0f) // bottom
        );
        rdCamera_SetCanvas(JonesHud_pHudCamera, JonesHud_pHudCanvas);
    }

    rdCamera_SetFrustrum(JonesHud_pHudCamera, JonesHud_pHudCamera->pFrustum, 0, 0, width - 1, height - 1);
    rdCamera_sub_4C60B0(false);
    return 1;
}

void JonesHud_UpdateSinCosTable(void)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_aCosSinTableScaled); ++i ) // was fixed to 24
    {
        JonesHud_aCosSinTableScaled[i][0] = JonesHud_aCosSinTable[i][0] * JonesHud_healthIndScale;// cos
        JonesHud_aCosSinTableScaled[i][1] = JonesHud_aCosSinTable[i][1] * JonesHud_healthIndScale;// sin
    }
}

void J3DAPI JonesHud_UpdateHUDLayout(uint32_t width, uint32_t height)
{
   // Fixed: Fixed HUD scaling of indicator by using only JonesHud_widthAspectRatioScale
   // These coords are all in screen size

    float aspectScale   =  JonesHud_heightAspectRatioScale;
    float healthIndSize = (aspectScale * JonesHud_healthIndSize); // Altered: OG: 60.0f

    if ( JonesHud_healthIndPosX <= 1.0f )
    {
        // Fixed anchored position to left
        JonesHud_healthIndRect.x = ((RD_REF_WIDTH - JonesHud_healthIndSize) * JonesHud_healthIndPosX) * aspectScale;  // Altered: OG: JonesHud_widthAspectRatioScale * 24.0f;
    }
    else
    {
        // Relative position to screen width
        JonesHud_healthIndRect.x = ((float)width - healthIndSize) * (JonesHud_healthIndPosX - 1.0f); // Altered: OG: JonesHud_widthAspectRatioScale * 24.0f;
    }

    JonesHud_healthIndRect.y      = ((RD_REF_HEIGHT - JonesHud_healthIndSize) * JonesHud_healthIndPosY) * aspectScale; // Altered: OG: (float)height - JonesHud_healthIndRect.x - JonesHud_heightAspectRatioScale * 60.0f;
    JonesHud_healthIndRect.width  = healthIndSize; // Altered: OG: JonesHud_widthAspectRatioScale * 60.0
    JonesHud_healthIndRect.height = healthIndSize; // Altered: OG: JonesHud_heightAspectRatioScale * 60.0

    JonesHud_healthIndBarPos.x = JonesHud_healthIndRect.x + JonesHud_healthIndRect.width / 2.0f;
    JonesHud_healthIndBarPos.y = JonesHud_healthIndRect.y + JonesHud_healthIndRect.height / 2.0f;
    JonesHud_healthIndBarPos.z = 0.0f; // z & w set later when drawing hud indicator
    JonesHud_healthIndBarPos.w = 0.0f;

    float enduranceInd = (aspectScale * JonesHud_enduranceIndSize);
    if ( JonesHud_enduranceIndPosX <= 1.0f )
    {
        // Fixed anchored position to left
        JonesHud_enduranceRect.x = ((RD_REF_WIDTH - JonesHud_enduranceIndSize) * JonesHud_enduranceIndPosX) * aspectScale;  // Altered: OG: JonesHud_healthIndRect.x + JonesHud_healthIndRect.width + JonesHud_widthAspectRatioScale * 8.0f;
    }
    else
    {
        // Relative position to screen width
        JonesHud_enduranceRect.x = ((float)width - enduranceInd) * (JonesHud_enduranceIndPosX - 1.0f); // Altered: OG: JonesHud_healthIndRect.x + JonesHud_healthIndRect.width + JonesHud_widthAspectRatioScale * 8.0f;
    }

    JonesHud_enduranceRect.y      = ((RD_REF_HEIGHT - JonesHud_enduranceIndSize) * JonesHud_enduranceIndPosY) * aspectScale; // Altered: OG: JonesHud_healthIndRect.y 
    JonesHud_enduranceRect.width  = enduranceInd; // Altered: OG: JonesHud_healthIndRect.width 
    JonesHud_enduranceRect.height = enduranceInd; // Altered: OG: JonesHud_healthIndRect.height

    JonesHud_enduranceIndBarPos.x = JonesHud_enduranceRect.x + JonesHud_enduranceRect.width / 2.0f;
    JonesHud_enduranceIndBarPos.y = JonesHud_enduranceRect.y + JonesHud_enduranceRect.height / 2.0f;
    JonesHud_enduranceIndBarPos.z = 0.0f;  // z & w set later when drawing hud indicator
    JonesHud_enduranceIndBarPos.w = 0.0f;

    JonesHud_healthIndScale    = ((JonesHud_healthIndRect.height - 2.0f) / 2.0f) - 8.5f * aspectScale; // Altered: Changed factor to 8.5f from 7.0f
    JonesHud_enduranceIndScale = ((JonesHud_enduranceRect.height - 2.0f) / 2.0f) - 4.0f * aspectScale; // Added: Added endurance indicator scale

    // Unused stuff
    JonesHud_flt_55503C = 16.0f * JonesHud_heightAspectRatioScale;
    JonesHud_flt_555038 = 16.0f * JonesHud_widthAspectRatioScale;
    JonesHud_flt_555034 = -JonesHud_flt_55503C;

    JonesHud_dword_554FDC = 0;

    JonesHud_flt_554FE4 = 16.0f * JonesHud_heightAspectRatioScale;
    JonesHud_flt_554FE0 = 16.0f * JonesHud_widthAspectRatioScale;

    // No need to adjust inventory height based on aspect ratio anymore
    JonesHud_invMenuMinZ = JonesHud_invMenuDefaultZ;
}

void JonesHud_MenuOpen(void)
{
    if ( sithPlayer_g_pLocalPlayerThing && (!JonesHud_pMenuItemLinkedList || (JonesHud_hudState & 0x01) == 0) )
    {
        JonesHud_selectedWeaponMenuItemID = 0;
        for ( size_t i = 0; i <= SITHWEAPON_GRENADE; ++i )
        {
            if ( JonesHud_apMenuItems[i]
                && JonesHud_apMenuItems[i]->inventoryID == sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->curWeaponID )
            {
                JonesHud_selectedWeaponMenuItemID = i;
                break;
            }
        }

        // Added: Play menu theme music if not already playing
        if ( JonesHud_bMenuMusicEnabled && JonesHud_aSoundFxHandles[0] )
        {
             // Note, must use playonce flag as loop play won't work due to sound module is stopped at this point
            JonesHud_hSndChannelMusic = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[0], JonesHud_menuMusicVolume, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        JonesHud_selectedTreasuresMenuItemID = JONESHUD_MENU_TREASURE_CHEST;
        JonesHud_selectedItemsMenuItemID     = JONESHUD_MENU_INVITEM_ZIPPO;
        JonesHud_selectedSystemMenuItemID    = JONESHUD_MENU_IQ;
        JonesHud_pCurSelectedMenuItem        = NULL;
        JonesHud_pMenuItemLinkedList         = NULL;

        JonesHud_ResetMenuItems();

        if ( JonesHud_pMenuItemLinkedList )
        {
            if ( JonesHud_aSoundFxHandles[3] )  // inv_expand.wav
            {
                JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[3], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
            }

            JonesHud_rootMenuItemId = JonesHud_pMenuItemLinkedList->id;

            JonesHud_pMenuItemLinkedList->flags |= 0x83;

            JonesHud_hudState |= 0x01; // mark menu opened;

            float moveRate = fabsf(JonesHud_pMenuItemLinkedList->endMovePos.x - JonesHud_pMenuItemLinkedList->startMovePos.x);
            JonesHud_StartItemTranslation(JonesHud_pMenuItemLinkedList, JonesHud_msecMenuOpenStartSlideDuration, moveRate, /*bMoveUp=*/0);
        }
    }
}

void JonesHud_MenuClose(void)
{
    JonesHud_pCurSelectedMenuItem = NULL;
    JonesHud_item_flag_state_55514C &= ~0xFF;

    JonesHudMenuItem* pItem = JonesHud_pMenuItemLinkedList;
    for ( size_t i = 0; i < 4; ++i ) // I think it collapses up to 4 items 
    {
        if ( pItem->nextUpItemId != -1
            && JonesHud_apMenuItems[pItem->nextUpItemId]
            && ((JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x01) != 0
                || (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x20) != 0) )
        {
            //(JonesHud_item_flag_state_55514C & 0xFF) = (1 << i) | JonesHud_item_flag_state_55514C;
            JonesHud_item_flag_state_55514C = (JonesHud_item_flag_state_55514C & ~0xFF) | (1 << i);
            if ( JonesHud_aSoundFxHandles[3] )  // inv_expand.wav
            {
                JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[3], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
            }

            JonesHud_MenuCollapseColumn(pItem);
        }

        pItem->flags |= i << 10;
        if ( pItem->nextRightItemId == -1
            || !JonesHud_apMenuItems[pItem->nextRightItemId]
            || JonesHud_apMenuItems[pItem->nextRightItemId]->id == JonesHud_rootMenuItemId
            || (JonesHud_apMenuItems[pItem->nextRightItemId]->flags & 0x01) == 0 )
        {
            break;
        }

        pItem = JonesHud_apMenuItems[pItem->nextRightItemId];
    }

    JonesHud_pCloseMenuItem = pItem;

    //if ( !(uint8_t)JonesHud_item_flag_state_55514C )
    if ( (JonesHud_item_flag_state_55514C & 0xFF) == 0 ) // And here if 4 items were collapsed move sideways to the left
    {
        if ( JonesHud_aSoundFxHandles[3] ) // inv_expand.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[3], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        JonesHud_pCloseMenuItem->flags |= 0x40u;
        JonesHud_pCloseMenuItem->flags &= ~0x80;

        JonesHud_pCloseMenuItem->startMovePos = JonesHud_pCloseMenuItem->pos;

        float moveRate = JonesHud_menuColumnHorizontalSpacing; // Fixed: Init to JonesHud_menuColumnHorizontalSpacing;
        if ( JonesHud_pCloseMenuItem == JonesHud_pMenuItemLinkedList )
        {
            JonesHud_pCloseMenuItem->endMovePos.x = -0.15800001f;
            moveRate = JonesHud_pMenuItemLinkedList->endMovePos.x - JonesHud_pMenuItemLinkedList->startMovePos.x;
        }
        else
        {
            JonesHud_pCloseMenuItem->endMovePos.x = JonesHud_apMenuItems[JonesHud_pCloseMenuItem->nextLeftItemId]->pos.x;
            if ( JonesHud_pCloseMenuItem->endMovePos.x == JonesHud_pCloseMenuItem->startMovePos.x )
            {
                if ( JonesHud_apMenuItems[JonesHud_pCloseMenuItem->nextLeftItemId] == JonesHud_pMenuItemLinkedList )
                {
                    JonesHud_pCloseMenuItem->endMovePos.x = -0.15800001f;
                    moveRate = JonesHud_pMenuItemLinkedList->endMovePos.x - JonesHud_pMenuItemLinkedList->startMovePos.x;
                }
                else
                {
                    JonesHud_pCloseMenuItem->endMovePos.x = JonesHud_pCloseMenuItem->endMovePos.x - JonesHud_menuColumnHorizontalSpacing;
                    moveRate = JonesHud_menuColumnHorizontalSpacing;
                }
            }
        }

        moveRate = fabsf(moveRate);
        JonesHud_StartItemTranslation(JonesHud_pCloseMenuItem, JonesHud_msecMenuCloseStartSlideDuration, moveRate, 0);
    }

    // Added: Stop menu music 
    if ( JonesHud_hSndChannelMusic != SOUND_INVALIDHANDLE )
    {
        sithSoundMixer_FadeVolume(JonesHud_hSndChannelMusic, 0.0f, JonesHud_menuMusicFadeDuratuion);
        JonesHud_hSndChannelMusic = SOUND_INVALIDHANDLE;
    }

    JonesHud_hudState |= 0x04; // Closing menu
}

void J3DAPI JonesHud_StartItemTranslation(JonesHudMenuItem* pItem, uint32_t msecDuration, float moveRate, int bMoveUp)
{
    // Fixed: Fixed division by 0
    if ( moveRate == 0 )
    {
        moveRate = 0.001f;
    }

    float delta;
    if ( bMoveUp )
    {
        delta = fabsf(pItem->endMovePos.z - pItem->startMovePos.z) / moveRate;
    }
    else
    {
        delta = fabsf(pItem->endMovePos.x - pItem->startMovePos.x) / moveRate;
    }

    pItem->msecMoveDuration = (int32_t)((double)msecDuration * delta);
    pItem->msecMoveEndTime  = pItem->msecMoveDuration + JonesHud_msecTime;
    pItem->flags |= 0x01; // Make visible
}

int J3DAPI JonesHud_HasTimeElapsed(unsigned int msecDuration, unsigned int msecDeltaTime, unsigned int msecTime)
{
    if ( msecDuration < msecDeltaTime )
    {
        return msecTime > msecDeltaTime;
    }

    return msecTime > msecDuration && msecTime >= msecDeltaTime;
}

void J3DAPI JonesHud_RenderEnduranceIndicator(float enduranceState)
{
    if ( (sithPlayer_g_pLocalPlayerThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
    {
        // Draw raft indicator
        if ( sithWorld_g_pCurrentWorld && (sithWorld_g_pCurrentWorld->state & SITH_WORLD_STATE_INIT_HUD) != 0 )
        {
            JonesHud_curEnduranceIndAlpha = JonesHud_enduranceIndFade;
            JonesHud_curEnduranceState    = -1.0f;
        }

        bool bFade = JonesHud_curEnduranceState == enduranceState && (JonesHud_hudState & 0x01) == 0; // Altered: Add check for menu not opened
        if ( bFade )
        {
            if ( JonesHud_curEnduranceIndAlpha > JonesHud_enduranceIndFade
                && JonesHud_curEnduranceState == enduranceState
                && JonesHud_HasTimeElapsed(JonesHud_msecEnduranceIndTime, JonesHud_msecRaftIndicatorFadeTime + JonesHud_msecEnduranceIndTime, JonesHud_msecTime) )
            {
                JonesHud_curEnduranceIndAlpha -= (float)JonesHud_msecDeltaTime * 0.8f / (float)JonesHud_msecEnduranceIndTime;
                if ( JonesHud_curEnduranceIndAlpha < JonesHud_enduranceIndFade )
                {
                    JonesHud_curEnduranceIndAlpha = JonesHud_enduranceIndFade;
                }
            }
        }
        else
        {
            JonesHud_curEnduranceIndAlpha      = 1.0f;
            JonesHud_msecRaftIndicatorFadeTime = JonesHud_msecTime;
        }

        // Draw indicator bar
        JonesHud_curEnduranceState = enduranceState;
        JonesHud_DrawEnduranceIndicator(enduranceState, JonesHud_curEnduranceIndAlpha * (JonesHud_curEnduranceIndAlpha == JonesHud_enduranceIndFade ? 0.7f : 1.0f)); // Altered: Reduced opacity by 30% to not overlap visually with overlay texture

        rdVector4 color = JonesHud_colorWhite;
        color.alpha     = JonesHud_curEnduranceIndAlpha;

        // Now draw indicator overlay texture
        JonesHud_Draw(
            JonesHud_pEnduranceIndOverlayMat,
            &JonesHud_enduranceRect,
            /*z=*/RD_FIXEDPOINT_RHW_SCALE_X1,
            /*rhw=*/RD_FIXEDPOINT_RHW_SCALE_X1,
            &color,
            /*celNum=*/0,
            /*bAlpha=*/1
        );
    }
    else
    {
        // Draw indicator bar
        JonesHud_DrawEnduranceIndicator(enduranceState, 1.0f);

        // Now draw indicator overlay texture
        rdVector4 color = JonesHud_colorWhite;
        color.alpha     = 1.0f;

        JonesHud_Draw(
            JonesHud_pEnduranceIndOverlayMat,
            &JonesHud_enduranceRect,
            /*z=*/RD_FIXEDPOINT_RHW_SCALE_X1,
            /*rhw=*/RD_FIXEDPOINT_RHW_SCALE_X1,
            &color,
            /*celNum=*/0,
            /*bAlpha=*/1
        );
    }
}

void J3DAPI JonesHud_DrawEnduranceIndicator(float state, float alpha)
{
    JonesHudRect rect;
    rect.x      = JonesHud_enduranceRect.x;
    rect.y      = JonesHud_enduranceRect.y;
    rect.width  = JonesHud_enduranceRect.width;
    rect.height = JonesHud_enduranceRect.height;

    // Draw piechart that indicates state
    int progress = 100;
    if ( state < 100.0f )
    {
        progress = (int32_t)state % 100;
    }

    float angle = (float)progress * 0.01f * 360.0f;
    rdVector4 maskColor = JonesHud_colorBlack;
    maskColor.alpha     = alpha;

    JonesHud_enduranceIndBarPos.z = RD_FIXEDPOINT_RHW_SCALE_X2; // this will make to draw_icon above base color (yellow, blue, pink)
    JonesHud_enduranceIndBarPos.w = RD_FIXEDPOINT_RHW_SCALE_X2;

    if ( angle != 0.0f )
    {
        // Draws indicator bar 
        JonesHud_DrawIndicatorBar(&JonesHud_enduranceIndBarPos, JonesHud_enduranceIndScale, &maskColor, angle, /*bClockWise=*/1);
    }

    // Now draw underlying bar color

    // Altered: Init to 0 and moved here from above
    rdVector4 indColor = { 0 };
    if ( (sithPlayer_g_pLocalPlayerThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
    {
        indColor = JonesHud_enduranceIndColorRaft;
    }
    else if ( sithPlayer_g_pLocalPlayerThing->pInSector
        && (sithPlayer_g_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        indColor = JonesHud_enduranceIndColorOxygen;
    }
    else if ( JonesHud_bIMPState )
    {
        indColor = JonesHud_enduranceIndColorIMPEnergy;
    }

    indColor.alpha = alpha;

    rect.x = JonesHud_enduranceRect.width / 2.0f - JonesHud_enduranceIndScale + rect.x;
    rect.y = JonesHud_enduranceRect.width / 2.0f - JonesHud_enduranceIndScale + rect.y;
    rect.width  = JonesHud_enduranceIndScale * 2.0f;
    rect.height = rect.width;

    JonesHud_Draw(JonesHud_pHealthIndBaseMat, &rect, RD_FIXEDPOINT_RHW_SCALE_X3, RD_FIXEDPOINT_RHW_SCALE_X3, &indColor, 0, 1); // will draw the base mat above previously drawn endurance indicator parts
}

void J3DAPI JonesHud_SetFadeHealthHUD(int bShow, int bFade)
{
    JonesHud_bFadeHealthHUD = bFade;
    if ( !bShow && !JonesHud_bFadeHealthHUD )
    {
        JonesHud_healthIndAlpha = 0.0f;
    }
}

int J3DAPI JonesHud_RenderHealthIndicator(int bFade)
{
    if ( bFade )
    {
        JonesHud_bHealthIndFade = 1;
        JonesHud_bFadeHealthHUD = 0;
    }

    if ( (JonesHud_hudState & 0x01) != 0 )
    {
        JonesHud_bHealthIndFade = 0;
    }

    if ( JonesHud_msecDeltaTime == 1 )
    {
        JonesHud_curHealthIndFadeState = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health;
    }
    else
    {
        if ( JonesHud_curHealthIndFadeState != sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health )
        {
            JonesHud_healthIndAlpha = 1.0f;
            JonesHud_bHealthIndFade = 0;
        }

        JonesHud_curHealthIndFadeState = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health;
    }

    if ( !JonesHud_bHealthIndFade )
    {
        return jonesCog_g_bShowHealthHUD == 0;
    }

    // Fade in or fade out health HUD
    float delta = (float)JonesHud_msecDeltaTime * 0.8f / (float)JonesHud_msecHealthIndFadeTime;
    if ( jonesCog_g_bShowHealthHUD )
    {
        if ( JonesHud_healthIndAlpha >= JonesHud_healthIndFade )
        {
            JonesHud_bHealthIndFade = 0;
        }
        else
        {
            JonesHud_healthIndAlpha += delta;
            if ( JonesHud_healthIndAlpha > JonesHud_healthIndFade )
            {
                JonesHud_healthIndAlpha = JonesHud_healthIndFade;
            }
        }
    }
    else if ( JonesHud_healthIndAlpha <= 0.0f )
    {
        JonesHud_bHealthIndFade = 0;
    }
    else
    {
        JonesHud_healthIndAlpha -= delta;
        if ( JonesHud_healthIndAlpha < 0.0f )
        {
            JonesHud_healthIndAlpha = 0.0f;
        }
    }

    if ( JonesHud_healthIndAlpha == 0.0f || JonesHud_healthIndAlpha == JonesHud_healthIndFade ) // 0.200000002980232f
    {
        JonesHud_bHealthIndFade = 0;
    }

    if ( JonesHud_healthIndAlpha <= 0.0f )
    {
        return 1;
    }

    float healthState = (float)((int)(300 * (int32_t)sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health)
        / (int32_t)sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.maxHealth);
    JonesHud_DrawHealthIndicator(0.0f, healthState, JonesHud_healthIndAlpha);
    return 1;
}

void J3DAPI JonesHud_DrawHealthIndicator(float hitDelta, float health, float alpha)
{
    rdVector4 color = JonesHud_colorWhite;
    if ( (sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.flags & SITH_AF_POISONED) != 0 )
    {
        // Draws poisoned mat above all components in health indicator
        // Altered: Moved behind endurance indicator
        // OG: X1
        color.alpha = 0.25f;
        JonesHud_Draw(JonesHud_pHealthIndPoisonedOverlayMat,
            &JonesHud_healthIndRect,
            RD_FIXEDPOINT_RHW_SCALE_X5, // sz 
            RD_FIXEDPOINT_RHW_SCALE_X5, // rhw
            &color,
            /*celNum=*/0,
            /*bAlpha=*/1
        );
    }

    // Draw hit flash effect
    if ( hitDelta > 0.0f )
    {
        // Altered: Moved behind endurance indicator
        // OG: X2
        JonesHud_DrawHealthHitOverlay(hitDelta, RD_FIXEDPOINT_RHW_SCALE_X6, RD_FIXEDPOINT_RHW_SCALE_X6); // will draw below poison overlay mat
    }

    // Draws overlay texture below previously drawn components
    // Altered: Moved behind endurance indicator
    // OG: X3
    color.alpha = alpha;
    JonesHud_Draw(JonesHud_pHealthIndOverlayMat, &JonesHud_healthIndRect, RD_FIXEDPOINT_RHW_SCALE_X7, RD_FIXEDPOINT_RHW_SCALE_X7, &color, 0, 1);

    // Draws health bar overlay "pie chart" of current health bar in either yellow, red or black color below previous components
    // Altered: Moved behind endurance indicator
    // OG: X4
    JonesHud_DrawHealthIndicatorBar(health, alpha, RD_FIXEDPOINT_RHW_SCALE_X8, RD_FIXEDPOINT_RHW_SCALE_X8);

    // Draws the health bar base circle color below previously drawn health pie chart in either color green, yellow, red color 
    // Altered: Moved behind endurance indicator
    // OG: X5
    JonesHud_DrawHealthIndicatorBase(health, alpha, RD_FIXEDPOINT_RHW_SCALE_X9, RD_FIXEDPOINT_RHW_SCALE_X9);
}

void J3DAPI JonesHud_DrawHealthHitOverlay(float hitDelta, float z, float rhw)
{
    if ( JonesHud_maxHealthIndHitAlpha == 0.0f && JonesHud_curHealthIndhitAlpha == 0.0f )
    {
        JonesHud_maxHealthIndHitAlpha = (hitDelta - (float)((int32_t)hitDelta % 20) + 20.0f) / 300.0f;
    }

    if ( JonesHud_curHealthIndhitAlpha >= (double)JonesHud_maxHealthIndHitAlpha )
    {
        JonesHud_curHealthIndhitAlpha -= (float)JonesHud_msecDeltaTime * JonesHud_maxHealthIndHitAlpha * 2.0f / 250.0f;
        if ( JonesHud_curHealthIndhitAlpha <= 0.0f )
        {
            JonesHud_curHealthIndhitAlpha = 0.0f;
            JonesHud_maxHealthIndHitAlpha = 0.0f;
            return;
        }
    }
    else
    {
        JonesHud_curHealthIndhitAlpha += (float)JonesHud_msecDeltaTime * JonesHud_maxHealthIndHitAlpha * 2.0f / 250.0f;
        if ( JonesHud_curHealthIndhitAlpha > JonesHud_maxHealthIndHitAlpha )
        {
            JonesHud_curHealthIndhitAlpha = JonesHud_maxHealthIndHitAlpha;
        }
    }

    rdVector4 color = JonesHud_colorWhite;
    color.alpha     = JonesHud_curHealthIndhitAlpha;
    JonesHud_Draw(JonesHud_pHealthIndHitOverlayMat, &JonesHud_healthIndRect, z, rhw, &color, 0, 1);
}

void J3DAPI JonesHud_RenderHealthIndicatorHit(float healthState)
{
    if ( sithPlayer_g_pLocalPlayerThing )
    {
        if ( !JonesHud_bHealthIndFadeSet )
        {
            JonesHud_curHealth = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health;
            JonesHud_msecHealthIndCurFadeTime = JonesHud_msecTime;
            JonesHud_bHealthIndFadeSet = 1;
        }

        float health = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health;
        if ( JonesHud_pCurInvChangedItem && JonesHud_pCurInvChangedItem->pos.x <= -0.13f
            || (JonesHud_hudState & 0x01) != 0
            || health == 0.0f
            || JonesHud_curHealth != health )
        {
            JonesHud_healthIndAlpha = 1.0f;
            JonesHud_msecHealthIndCurFadeTime = JonesHud_msecTime;
        }
        else if ( JonesHud_healthIndAlpha > JonesHud_healthIndFade
            && JonesHud_curHealth == health
            // TODO: check should probably be !JonesHud_HasTimeElapsed
            && JonesHud_HasTimeElapsed(JonesHud_msecHealthIndFadeTime, JonesHud_msecHealthIndCurFadeTime + JonesHud_msecHealthIndFadeTime, JonesHud_msecTime) )
        {
            JonesHud_healthIndAlpha -= (float)JonesHud_msecDeltaTime * 0.80000001f / (float)JonesHud_msecHealthIndFadeTime;
            if ( JonesHud_healthIndAlpha < JonesHud_healthIndFade )
            {
                JonesHud_healthIndAlpha = JonesHud_healthIndFade;
            }
        }

        float hitDelta = JonesHud_curHealth - health;
        JonesHud_DrawHealthIndicator(hitDelta, healthState, JonesHud_healthIndAlpha);
        JonesHud_curHealth = health;
    }
}

void J3DAPI JonesHud_DrawHealthIndicatorBase(float state, float alpha, float z, float w)
{
    rdVector4 color = { 0 };   // Added: Init to zero
    JonesHudRect rect = { 0 }; // Added: Init to zero

    rect.x = JonesHud_healthIndRect.x;
    rect.y = JonesHud_healthIndRect.y;
    rect.width  = JonesHud_healthIndRect.width;
    rect.height = JonesHud_healthIndRect.height;

    if ( state >= 100.01f )
    {
        if ( state >= 200.01f )
        {
            color =  JonesHud_healthIndColorFull;
        }
        else
        {
            color = JonesHud_healthIndColorMedium;
        }
    }
    else
    {
        color = JonesHud_healthIndColorLow;
    }

    color.alpha = alpha;

    rect.x = JonesHud_healthIndRect.width / 2.0f - JonesHud_healthIndScale + rect.x;
    rect.y = JonesHud_healthIndRect.width / 2.0f - JonesHud_healthIndScale + rect.y;
    rect.width  = JonesHud_healthIndScale * 2.0f;
    rect.height = rect.width;

    JonesHud_Draw(JonesHud_pHealthIndBaseMat, &rect, z, w, &color, 0, 1);
}

void J3DAPI JonesHud_DrawHealthIndicatorBar(float health, float alpha, float z, float w)
{
    rdVector4 color     = { 0 }; // Added: init to zero
    int32_t hlthPercent = (int32_t)health % 100;
    float angle         = (float)(100 - hlthPercent) * 0.01f * 360.0f;

    if ( (int32_t)roundf(angle) % 360 || health <= 0.0f )
    {
        if ( health >= 100.01f )
        {
            if ( health >= 200.01f )
            {
                color =  JonesHud_healthIndColorMedium;
            }
            else
            {
                color = JonesHud_healthIndColorLow;
            }
        }
        else
        {
            color = JonesHud_healthIndColorCritical;
        }

        JonesHud_healthIndBarPos.z = z;
        JonesHud_healthIndBarPos.w = w;

        color.alpha = alpha;
        JonesHud_DrawIndicatorBar(&JonesHud_healthIndBarPos, JonesHud_healthIndScale, &color, angle, /*bClockWise=*/0);
    }
}

void J3DAPI JonesHud_DrawIndicatorBar(const rdVector4* pPos, float scale, const rdVector4* pColor, float angle, int bClockWise)
{
    rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
    if ( pPoly )
    {
        pPoly->flags        = RD_FF_TEX_TRANSLUCENT;
        pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
        pPoly->pMaterial    = JonesHud_pHealthIndBaseMat;
        pPoly->matCelNum    = 0;
        pPoly->numVertices  = 0;

        D3DTLVERTEX vert;
        vert.sx  = pPos->x;
        vert.sy  = pPos->y;
        vert.sz  = pPos->z;
        vert.rhw = pPos->w;
        vert.tu  = 0.5f;
        vert.tv  = 0.5f;

        size_t vertNum = 0;

        LPD3DTLVERTEX pOutVert = pPoly->aVertices;
        *pOutVert = vert;

        ++pPoly->numVertices;
        ++vertNum;

        double startAngle = 0.0;
        double maxAngle    = 0.0;
        if ( bClockWise )
        {
            startAngle = angle + 270.0;
            maxAngle   = 270.0;
        }
        else
        {
            startAngle = 450.0;
            maxAngle   = 450.0 - angle;
        }

        bool bSkip        = false;
        double curAngle   = startAngle;
        double deltaAngle = fmod(startAngle, 15.0);

        while ( curAngle >= maxAngle )
        {
            pOutVert = &pPoly->aVertices[vertNum];
            *pOutVert = vert;

            if ( fmod(fmod(curAngle, 360.0), 30.0) == 0.0 )
            {
                size_t sinIdx = (size_t)(fmod(curAngle, 360.0) / 15.0);
                pOutVert->sx  = JonesHud_aCosSinTable[sinIdx][0] * scale + pOutVert->sx; // cos // Altered: Replaced JonesHud_aCosSinTableScaled with JonesHud_aCosSinTable so different scale can be used
                pOutVert->sy  = pOutVert->sy - JonesHud_aCosSinTable[sinIdx][1] * scale; // sin // Altered: Replaced JonesHud_aCosSinTableScaled with JonesHud_aCosSinTable so different scale can be used
            }
            else
            {
                float cos, sin;
                stdMath_SinCos((float)curAngle, &sin, &cos);
                pOutVert->sx =  cos * scale + pOutVert->sx;
                pOutVert->sy = -sin * scale + pOutVert->sy;
            }

            ++pPoly->numVertices;

            if ( startAngle - curAngle + 15.0 <= 180.0 || curAngle <= maxAngle || bSkip )
            {
                ++vertNum;
            }
            else
            {
                pPoly->extraLight.red   = pColor->red;
                pPoly->extraLight.green = pColor->green;
                pPoly->extraLight.blue  = pColor->blue;
                pPoly->extraLight.alpha = pColor->alpha;

                rdVector4* pVertColor = pPoly->aVertIntensities;
                pVertColor->red   = pColor->red;
                pVertColor->green = pColor->green;
                pVertColor->blue  = pColor->blue;
                pVertColor->alpha = pColor->alpha;

                rdCache_AddAlphaProcFace(pPoly->numVertices);

                rdCacheProcEntry* pNextPoly = rdCache_GetAlphaProcEntry();
                if ( !pNextPoly )
                {
                    return;
                }

                pNextPoly->flags        = RD_FF_TEX_TRANSLUCENT;
                pNextPoly->lightingMode = RD_LIGHTING_DIFFUSE;
                pNextPoly->pMaterial    = JonesHud_pHealthIndBaseMat;
                pNextPoly->matCelNum    = 0;
                pNextPoly->numVertices  = 0;

                pOutVert  = pNextPoly->aVertices;
                *pOutVert = vert;
                pNextPoly->aVertices[1] = pPoly->aVertices[vertNum];

                pPoly = pNextPoly;
                pPoly->numVertices = 2;
                vertNum = pPoly->numVertices;
                bSkip = true;
            }

            if ( curAngle == maxAngle )
            {
                break;
            }

            if ( curAngle != startAngle || deltaAngle == 0.0 )
            {
                curAngle = curAngle - 15.0;
            }
            else
            {
                curAngle = curAngle - deltaAngle;
            }

            if ( curAngle < maxAngle )
            {
                curAngle = maxAngle;
            }
        }

        pPoly->extraLight          = *pColor;
        pPoly->aVertIntensities[0] = *pColor;
        rdCache_AddAlphaProcFace(pPoly->numVertices);
    }
}

void J3DAPI JonesHud_Draw(const rdMaterial* pMaterial, const JonesHudRect* rect, float z, float rhw, const rdVector4* pColor, int celNum, int bAlpha)
{
    rdCacheProcEntry* pPoly = NULL;
    if ( bAlpha )
    {
        pPoly = rdCache_GetAlphaProcEntry();
        pPoly->flags = RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
    }
    else
    {
        pPoly = rdCache_GetProcEntry();
        pPoly->flags = RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X;
    }

    if ( pPoly )
    {
        pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
        pPoly->numVertices  = 4;
        pPoly->pMaterial    = (rdMaterial*)pMaterial;
        pPoly->matCelNum    = celNum;

        pPoly->extraLight.red   = pColor->red;
        pPoly->extraLight.green = pColor->green;
        pPoly->extraLight.blue  = pColor->blue;
        pPoly->extraLight.alpha = pColor->alpha;

        pPoly->aVertIntensities[0].red   = pColor->red;
        pPoly->aVertIntensities[0].green = pColor->green;
        pPoly->aVertIntensities[0].blue  = pColor->z;
        pPoly->aVertIntensities[0].alpha = pColor->alpha;

        LPD3DTLVERTEX pCurOutVert = pPoly->aVertices;
        pCurOutVert->sx = rect->x;
        pCurOutVert->sy = rect->y;
        pCurOutVert->sz = z;
        pCurOutVert->rhw = rhw;
        pCurOutVert->tu = 0.0f;
        pCurOutVert->tv = 0.0f;
        ++pCurOutVert;

        pCurOutVert->sx  = rect->x + rect->width;
        pCurOutVert->sy  = rect->y;
        pCurOutVert->sz  = z;
        pCurOutVert->rhw = rhw;
        pCurOutVert->tu  = 1.0f;
        pCurOutVert->tv  = 0.0f;
        ++pCurOutVert;

        pCurOutVert->sx  = rect->x + rect->width;
        pCurOutVert->sy  = rect->y + rect->height;
        pCurOutVert->sz  = z;
        pCurOutVert->rhw = rhw;
        pCurOutVert->tu  = 1.0f;
        pCurOutVert->tv  = 1.0f;
        ++pCurOutVert;

        pCurOutVert->sx  = rect->x;
        pCurOutVert->sy  = rect->y + rect->height;
        pCurOutVert->sz  = z;
        pCurOutVert->rhw = rhw;
        pCurOutVert->tu  = 0.0f;
        pCurOutVert->tv  = 1.0f;

        if ( bAlpha )
        {
            rdCache_AddAlphaProcFace(4);
        }
        else
        {
            rdCache_AddProcFace(4);
        }
    }
}

void JonesHud_ProcessInventoryMenu(void)
{
    // Added: Re start playing menu theme music if not already playing
    if ( JonesHud_bMenuMusicEnabled && JonesHud_aSoundFxHandles[0] )
    {
        if ( JonesHud_hSndChannelMusic != SOUND_INVALIDHANDLE && (Sound_GetChannelFlags(JonesHud_hSndChannelMusic) & SOUND_CHANNEL_PLAYING) == 0 )
        {
            // Note, must use playonce flag as loop play won't work due to sound module is stopped at this point
            JonesHud_hSndChannelMusic = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[0], JonesHud_menuMusicVolume, 0.0f, SOUNDPLAY_PLAYONCE);
        }
    }

    int bActivateKeyPressed;
    JonesHud_UpdateItem(JonesHud_pMenuItemLinkedList);

    if ( (JonesHud_hudState & 0x04) == 0 && (JonesHud_hudState & 0x08) == 0 )
    {
        if ( (JonesHud_hudState & 0x02) == 0 )
        {
            int v2 = 0;
            bActivateKeyPressed = 0;
            while ( JonesHud_aActivateKeyIds[v2] )
            {
                if ( JonesHud_GetKey(JonesHud_aActivateKeyIds[v2]) )
                {
                    bActivateKeyPressed = 1;
                    break;
                }

                ++v2;
            }

            bActivateKeyPressed |= JonesHud_GetKey(DIK_RETURN);
            if ( JonesHud_GetKey(DIK_NUMPADENTER) | bActivateKeyPressed )
            {
                JonesHud_MenuActivateItem();
            }

            else if ( JonesHud_GetKey(DIK_UP) )
            {
                JonesHud_MenuMoveUp();
            }

            else if ( JonesHud_GetKey(DIK_DOWN) )
            {
                JonesHud_MenuMoveDown();
            }
        }

        if ( JonesHud_GetKey(DIK_RIGHT) )
        {
            JonesHud_MenuMoveRight();
        }

        else if ( JonesHud_GetKey(DIK_LEFT) )
        {
            JonesHud_MenuMoveLeft();
        }
    }

    JonesHud_RenderMenuItems(JonesHud_pMenuItemLinkedList);
}

void JonesHud_MenuMoveLeft(void)
{
    JonesHudMenuItem* pLeftItem;
    JonesHudMenuItem* pCurItem;

    if ( JonesHud_pCurSelectedMenuItem )
    {
        pCurItem = JonesHud_pCurSelectedMenuItem;
    }
    else
    {
        pCurItem = JonesHud_pMenuItemLinkedList;
    }

    if ( pCurItem->nextLeftItemId != -1 )
    {
        if ( JonesHud_apMenuItems[pCurItem->nextLeftItemId] )
        {
            pLeftItem = JonesHud_apMenuItems[pCurItem->nextLeftItemId];
            if ( (pLeftItem->flags & 0x01) != 0
                && (pLeftItem->flags & 0x80) == 0
                && pLeftItem->pos.x == pLeftItem->endMovePos.x )
            {
                JonesHud_pCurSelectedMenuItem = JonesHud_apMenuItems[pLeftItem->id];
                JonesHud_MenuExpandColumn(JonesHud_apMenuItems[pLeftItem->id]);
                JonesHud_MenuCollapseColumn(pCurItem);

                if ( JonesHud_aSoundFxHandles[2] )// inv_move.wav
                {
                    JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                }
            }
        }
    }
}

void JonesHud_MenuMoveRight(void)
{
    JonesHudMenuItem* pRightItem;
    JonesHudMenuItem* pCurItem;

    if ( JonesHud_pCurSelectedMenuItem )
    {
        pCurItem = JonesHud_pCurSelectedMenuItem;
        if ( JonesHud_pCurSelectedMenuItem->nextRightItemId != -1 )
        {
            if ( JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextRightItemId] )
            {
                pRightItem = JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextRightItemId];
                if ( (pRightItem->flags & 0x01) != 0
                    && (pRightItem->flags & 0x80) == 0
                    && pRightItem->pos.x == pRightItem->endMovePos.x )
                {
                    JonesHud_pCurSelectedMenuItem = JonesHud_apMenuItems[pRightItem->id];
                    JonesHud_MenuExpandColumn(JonesHud_apMenuItems[pRightItem->id]);
                    JonesHud_MenuCollapseColumn(pCurItem);
                    if ( JonesHud_aSoundFxHandles[2] )
                    {
                        JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                    }
                }
            }
        }
    }

    else if ( (JonesHud_pMenuItemLinkedList->flags & 0x01) != 0
        && (JonesHud_pMenuItemLinkedList->flags & 0x80) == 0
        && JonesHud_pMenuItemLinkedList->pos.x == JonesHud_pMenuItemLinkedList->endMovePos.x )
    {
        JonesHud_pCurSelectedMenuItem = JonesHud_pMenuItemLinkedList;
        JonesHud_MenuExpandColumn(JonesHud_pMenuItemLinkedList);

        if ( JonesHud_aSoundFxHandles[2] )      // inv_move.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }
    }
}

void JonesHud_MenuMoveDown(void)
{
    if ( JonesHud_pCurSelectedMenuItem
        && JonesHud_pCurSelectedMenuItem->nextDownItemId != -1
        && JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextDownItemId] )
    {
        JonesHudMenuItem* pCurItem  = JonesHud_pCurSelectedMenuItem;
        JonesHudMenuItem* pDownItem = JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextDownItemId];
        JonesHud_pCurSelectedMenuItem = pDownItem;

        // Play move soundfx
        if ( JonesHud_aSoundFxHandles[2] ) // inv_move.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        JonesHud_SetSelectedMenuItem(pCurItem->id, pDownItem);

        pDownItem->flags |= 0x0A;
        pCurItem->flags &= ~0x02;

        pDownItem->nextLeftItemId = pCurItem->nextLeftItemId;
        pDownItem->nextRightItemId = pCurItem->nextRightItemId;

        pCurItem->nextRightItemId = -1;
        pCurItem->nextLeftItemId  = -1;

        if ( pDownItem->nextLeftItemId != -1 && JonesHud_apMenuItems[pDownItem->nextLeftItemId] )
        {
            JonesHud_apMenuItems[pDownItem->nextLeftItemId]->nextRightItemId = pCurItem->nextDownItemId;
        }

        if ( pDownItem->nextRightItemId != -1 && JonesHud_apMenuItems[pDownItem->nextRightItemId] )
        {
            JonesHud_apMenuItems[pDownItem->nextRightItemId]->nextLeftItemId = pCurItem->nextDownItemId;
        }

        while ( JonesHud_apMenuItems[pDownItem->nextUpItemId]->pos.z <= JonesHud_invMenuMaxZ
            && JonesHud_pCurSelectedMenuItem != JonesHud_apMenuItems[pDownItem->nextUpItemId] )
        {
            pDownItem = JonesHud_apMenuItems[pDownItem->nextUpItemId];
            pDownItem->startMovePos = pDownItem->pos;

            pDownItem->endMovePos    = pDownItem->pos;
            pDownItem->endMovePos.z += JonesHud_menuColumnSpacing;

            pDownItem->flags |= 0x08;

            JonesHud_StartItemTranslation(pDownItem, JonesHud_msecMenuItemMoveCurDuration, JonesHud_menuColumnSpacing, /*bMoveUp=*/1);

            if ( (JonesHud_apMenuItems[pDownItem->nextUpItemId]->flags & 0x01) == 0
                || pDownItem->id == JonesHud_selectedWeaponMenuItemID
                || pDownItem->id == JonesHud_selectedItemsMenuItemID
                || pDownItem->id == JonesHud_selectedSystemMenuItemID
                || pDownItem->id == JonesHud_selectedTreasuresMenuItemID )
            {
                pDownItem->flags |=  0x08;
                pDownItem->flags &= ~0x01;
                break;
            }
        }

        // Fixed: hide top item
        JonesHud_apMenuItems[pDownItem->nextUpItemId]->flags &= ~0x01; // Hide next up item

        // Move selected item to position without showing translation
        JonesHud_pCurSelectedMenuItem->pos = pCurItem->pos;
        JonesHud_pCurSelectedMenuItem->pos.z -= JonesHud_menuColumnSpacing;

        JonesHud_pCurSelectedMenuItem->startMovePos = pCurItem->pos;
        JonesHud_pCurSelectedMenuItem->startMovePos.z -= JonesHud_menuColumnSpacing;

        JonesHud_pCurSelectedMenuItem->endMovePos = pCurItem->pos;

        JonesHud_StartItemTranslation(JonesHud_pCurSelectedMenuItem, JonesHud_msecMenuItemMoveCurDuration, JonesHud_menuColumnSpacing, /*bMoveUp=*/1);
        JonesHud_pCurSelectedMenuItem->flags &= ~0x01; // Hide item being rendered during translation

        JonesHud_hudState |= 0x08;
    }
}

void J3DAPI JonesHud_SetSelectedMenuItem(int menuId, JonesHudMenuItem* pItem)
{
    if ( menuId == JonesHud_rootMenuItemId )
    {
        JonesHud_rootMenuItemId = pItem->id;
        JonesHud_pMenuItemLinkedList = pItem;
    }

    if ( menuId == JonesHud_selectedWeaponMenuItemID )
    {
        JonesHud_selectedWeaponMenuItemID = pItem->id;
    }

    else if ( menuId == JonesHud_selectedTreasuresMenuItemID )
    {
        JonesHud_selectedTreasuresMenuItemID = pItem->id;
    }

    else if ( menuId == JonesHud_selectedItemsMenuItemID )
    {
        JonesHud_selectedItemsMenuItemID = pItem->id;
    }

    else if ( menuId == JonesHud_selectedSystemMenuItemID )
    {
        JonesHud_selectedSystemMenuItemID = pItem->id;
    }
}

void JonesHud_MenuMoveUp(void)
{
    if ( JonesHud_pCurSelectedMenuItem
        && JonesHud_pCurSelectedMenuItem->nextUpItemId != -1
        && JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextUpItemId] )
    {

        JonesHudMenuItem* pCurItem = JonesHud_pCurSelectedMenuItem;
        int curItemId = JonesHud_pCurSelectedMenuItem->id;

        JonesHudMenuItem* pUpItem     = JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextUpItemId];
        JonesHud_pCurSelectedMenuItem = pUpItem;

        // Play move soundfx
        if ( JonesHud_aSoundFxHandles[2] ) // inv_move.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        JonesHud_SetSelectedMenuItem(pCurItem->id, pUpItem);

        pUpItem->flags  |= 0x02;
        pCurItem->flags &= ~0x02;

        pUpItem->nextLeftItemId   = pCurItem->nextLeftItemId;
        pUpItem->nextRightItemId  = pCurItem->nextRightItemId;

        pCurItem->nextRightItemId = -1;
        pCurItem->nextLeftItemId  = -1;

        if ( pUpItem->nextLeftItemId != -1 && JonesHud_apMenuItems[pUpItem->nextLeftItemId] )
        {
            JonesHud_apMenuItems[pUpItem->nextLeftItemId]->nextRightItemId = pCurItem->nextUpItemId;
        }

        if ( pUpItem->nextRightItemId != -1 && JonesHud_apMenuItems[pUpItem->nextRightItemId] )
        {
            JonesHud_apMenuItems[pUpItem->nextRightItemId]->nextLeftItemId = pCurItem->nextUpItemId;
        }

        while ( JonesHud_apMenuItems[pUpItem->nextDownItemId]->pos.z <= JonesHud_invMenuMaxZ )
        {
            pUpItem->pos    = JonesHud_apMenuItems[pUpItem->nextDownItemId]->pos;
            pUpItem->pos.z += JonesHud_menuColumnSpacing;

            pUpItem->startMovePos    = JonesHud_apMenuItems[pUpItem->nextDownItemId]->pos;
            pUpItem->startMovePos.z += JonesHud_menuColumnSpacing;

            pUpItem->endMovePos = JonesHud_apMenuItems[pUpItem->nextDownItemId]->pos;

            pUpItem->flags |= 0x08;
            JonesHud_StartItemTranslation(pUpItem, JonesHud_msecMenuItemMoveCurDuration, /*moveRate=*/JonesHud_menuColumnSpacing, /*bMoveUp=*/1);

            if ( pUpItem->pos.z > JonesHud_invMenuMaxZ || pUpItem->nextUpItemId == curItemId )
            {
                break;
            }

            pUpItem = JonesHud_apMenuItems[pUpItem->nextUpItemId];
        }

        pCurItem->flags |= 0x08;

        if ( pUpItem->pos.z <= JonesHud_invMenuMaxZ )
        {
            pCurItem->pos          = JonesHud_apMenuItems[pCurItem->nextDownItemId]->endMovePos;
            pCurItem->startMovePos = JonesHud_apMenuItems[pCurItem->nextDownItemId]->endMovePos;
            pCurItem->endMovePos   = JonesHud_apMenuItems[pCurItem->nextDownItemId]->pos;
            pCurItem->pyr          = JonesHud_aDfltMenuItemOrients[pCurItem->id].pyr;
        }
        else // up item is greater than 
        {
            pCurItem->startMovePos = pCurItem->pos;

            pCurItem->endMovePos    = pCurItem->pos;
            pCurItem->endMovePos.z -= JonesHud_menuColumnSpacing;

            // Set default orient and hide it
            pUpItem->pyr = JonesHud_aDfltMenuItemOrients[pUpItem->id].pyr;
            //pUpItem->flags &= ~0x01; // Fixed: Removed hiding the up item 
        }

        // Move to top and hide it from being rendered
        JonesHud_StartItemTranslation(pCurItem, JonesHud_msecMenuItemMoveCurDuration, /*moveRate=*/JonesHud_menuColumnSpacing, /*bMoveUp=*/1);
        pCurItem->flags &= ~0x01; // hide

        JonesHud_hudState |= 0x08;
    }
}

void J3DAPI JonesHud_BindActivateControlKeys(const size_t* aKeyIds, size_t numKeys)
{
    // Added
    STD_ASSERT(aKeyIds);

    STD_ZEROMEM(JonesHud_aActivateKeyIds, sizeof(JonesHud_aActivateKeyIds)); // Fixed: 0 all elements pf array
    for ( size_t i = 0; i < J3DMIN(numKeys, STD_ARRAYLEN(JonesHud_aActivateKeyIds)); ++i ) // Added: Clamp to array size
    {
        JonesHud_aActivateKeyIds[i] = aKeyIds[i];
    }
}

int J3DAPI JonesHud_GetKey(unsigned int keyId)
{
    int bKeyPressed;

    int numPressed = 0;
    if ( JonesHud_bKeyStateUpdated
        && !JonesHud_HasTimeElapsed(JonesHud_msecMenuItemMoveCurDuration, JonesHud_msecMenuItemMoveCurDuration + JonesHud_msecLastKeyPressTime, JonesHud_msecTime) )
    {
        return 0;
    }

    JonesHud_bKeyStateUpdated = 0;
    switch ( keyId )
    {
        case DIK_UP:
            bKeyPressed = sithControl_GetKey(SITHCONTROL_FORWARD, &numPressed);
            break;

        case DIK_LEFT:
            bKeyPressed = sithControl_GetKey(SITHCONTROL_TURNLEFT, &numPressed);
            break;

        case DIK_RIGHT:
            bKeyPressed = sithControl_GetKey(SITHCONTROL_TURNRIGHT, &numPressed);
            break;

        case DIK_DOWN:
            bKeyPressed = sithControl_GetKey(SITHCONTROL_BACK, &numPressed);
            break;

        default:
            bKeyPressed = stdControl_ReadKey(keyId, &numPressed);
            break;
    }

    if ( !bKeyPressed )
    {
        JonesHud_bKeyStateUpdated = 0;
        return 0;
    }

    JonesHud_msecMenuItemMoveCurDuration = JonesHud_msecMenuItemMoveDuration;
    if ( keyId == JonesHud_curKeyId )
    {
        if ( JonesHud_msecMenuItemMoveCurDuration < JonesHud_msecMenuItemMoveDuration
            && JonesHud_HasTimeElapsed(JonesHud_msecMenuItemMoveDuration, JonesHud_msecLastKeyPressTime + JonesHud_msecMenuItemMoveDuration, JonesHud_msecTime) )
        {
            JonesHud_msecMenuItemMoveCurDuration = JonesHud_msecMenuItemMoveDuration;
        }
        else if ( JonesHud_msecMenuItemMoveCurDuration > 75 )
        {
            JonesHud_msecMenuItemMoveCurDuration = (int32_t)((double)JonesHud_msecMenuItemMoveCurDuration * 0.75f);
            if ( JonesHud_msecMenuItemMoveCurDuration < 75 )
            {
                JonesHud_msecMenuItemMoveCurDuration = 75;
            }
        }
    }

    JonesHud_msecLastKeyPressTime = JonesHud_msecTime;
    JonesHud_bKeyStateUpdated     = 1;
    JonesHud_curKeyId             = keyId;
    return 1;
}

int JonesHud_InitializeMenu(void)
{
    if ( JonesHud_pMenuItemLinkedList )
    {
        STDLOG_ERROR("Menu already initialized.\n");
        return 0;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_apMenuIconModelNames); ++i )
    {
        if ( !JonesHud_apMenuItems[i] && JonesHud_apMenuIconModelNames[i] )
        {
            rdModel3* pItemIcon3 = sithModel_GetModel(JonesHud_apMenuIconModelNames[i]);
            if ( pItemIcon3 )
            {
                JonesHud_apMenuItems[i] = JonesHud_NewMenuItem(pItemIcon3);
                if ( !JonesHud_apMenuItems[i] )
                {
                    STDLOG_ERROR("Couldn't create menu item %s.\n", JonesHud_apMenuIconModelNames[i]);
                    return 0;
                }

                JonesHud_apMenuItems[i]->id = i;
            }
            else if ( i >= JONESHUD_MENU_IQ || i == JONESHUD_MENU_TREASURE_CHEST )
            {
                STDLOG_ERROR("Couldn't find model %s.\n", JonesHud_apMenuIconModelNames[i]);
                return 0;
            }
        }
    }

    // Initialize inventory menu items
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_apMenuItems); ++i )
    {
        JonesHudMenuItem* pItem = JonesHud_apMenuItems[i];
        for ( size_t typeId = 0; pItem && pItem->inventoryID == -1 && typeId < STD_ARRAYLEN(sithInventory_g_aTypes); ++typeId )
        {
            if ( sithInventory_g_aTypes[typeId].pInvIconModel == pItem->prdIcon->data.pModel3 )
            {
                pItem->inventoryID = typeId;
                break;
            }
        }
    }

    return 1;
}

void JonesHud_InitializeMenuSounds(void)
{
    // Altered: Changed start idx to 1 as 0 is reserved for ambient music
    for ( size_t i = 1; i < STD_ARRAYLEN(JonesHud_aSoundFxIdxs); ++i )
    {
        if ( JonesHud_aSoundFxIdxs[i] > -1 )
        {
            int sndIdx = SITHWORLD_STATICINDEX(JonesHud_aSoundFxIdxs[i]);
            JonesHud_aSoundFxHandles[i] = Sound_GetSoundHandle(sndIdx);
        }
    }

    if ( JonesHud_bMenuMusicEnabled && JonesHud_aMenuMusicFilename[0] )
    {
        JonesHud_aSoundFxHandles[0] = sithSound_Load(sithWorld_g_pStaticWorld, JonesHud_aMenuMusicFilename);
    }
}

JonesHudMenuItem* J3DAPI JonesHud_NewMenuItem(rdModel3* pItemIcon3)
{
    JonesHudMenuItem* pMenuItem = (JonesHudMenuItem*)STDMALLOC(sizeof(JonesHudMenuItem));
    if ( !pMenuItem )
    {
        return NULL;
    }

    pMenuItem->prdIcon = rdThing_New(NULL);
    rdThing_SetModel3(pMenuItem->prdIcon, pItemIcon3);

    pMenuItem->msecMoveDuration = 0;
    pMenuItem->msecMoveEndTime  = 0;
    pMenuItem->msecMoveDelta  = 0;

    pMenuItem->id              = -1;
    pMenuItem->inventoryID     = -1;
    pMenuItem->nextUpItemId    = -1;
    pMenuItem->nextDownItemId  = -1;
    pMenuItem->nextLeftItemId  = -1;
    pMenuItem->nextRightItemId = -1;
    pMenuItem->flags = 0;

    pMenuItem->startMovePos.x = 0.0f;
    pMenuItem->startMovePos.y = 0.0f;
    pMenuItem->startMovePos.z = 0.0f;

    pMenuItem->endMovePos.x = 0.0f;
    pMenuItem->endMovePos.y = 0.0f;
    pMenuItem->endMovePos.z = 0.0f;

    pMenuItem->pos.x = 0.0f;
    pMenuItem->pos.y = 0.0f;
    pMenuItem->pos.z = 0.0f;

    pMenuItem->pyr.x = 0.0f;
    pMenuItem->pyr.y = 0.0f;
    pMenuItem->pyr.z = 0.0f;

    return pMenuItem;
}

void J3DAPI JonesHud_FreeMenuItem(JonesHudMenuItem* pItem)
{
    rdThing_Free(pItem->prdIcon);
    stdMemory_Free(pItem);
}

void J3DAPI JonesHud_UpdateItem(JonesHudMenuItem* pItem)
{
    int32_t v9;
    int v16;
    int v17;
    int v18;
    int v19;
    int v20;

    if ( pItem )
    {
        if ( (pItem->flags & 0x01) != 0 )  // Item visible
        {
            if ( pItem != JonesHud_pCurSelectedMenuItem
                || ((sithInventory_g_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) == 0
                    ? (v9 = 0)
                    : (v9 = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[pItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED),
                    v9) )
            {
                if ( JonesHud_aDfltMenuItemOrients[pItem->id].pyr.yaw != pItem->pyr.yaw )
                {
                    float fyaw = fmodf(pItem->pyr.yaw, 360.0f);
                    float dyaw = fyaw - JonesHud_aDfltMenuItemOrients[pItem->id].pyr.yaw;
                    if ( dyaw < 0.0f )
                    {
                        dyaw = dyaw + 360.0f;
                    }

                    if ( dyaw + 1.0f < 360.0f )
                    {
                        pItem->pyr.yaw = (360.0f - dyaw) / 2.0f + fyaw; // Fixed: Added bounded yaw to 360 degrees. OG: Unbounded pItem->pyr.yaw was added
                    }
                    else
                    {
                        pItem->pyr.yaw = JonesHud_aDfltMenuItemOrients[pItem->id].pyr.yaw;
                    }
                }
            }
            else if ( JonesHud_bItemActivated )
            {
                JonesHud_bItemActivated = 0;
            }
            else
            {
                JonesHud_pCurSelectedMenuItem->pyr.yaw += (float)JonesHud_msecDeltaTime * 16.0f / 50.0f;
            }
        }

        if ( (pItem->flags & 0x08) != 0
            || (pItem->flags & 0x10) != 0
            || (pItem->flags & 0x20) != 0
            || (pItem->flags & 0x80) != 0
            || (pItem->flags & 0x40) != 0 )
        {
            if ( JonesHud_HasTimeElapsed(pItem->msecMoveDuration, pItem->msecMoveEndTime, JonesHud_msecTime) )
            {
                v16 = pItem->flags & 0x02;
                v18 = pItem->flags & 0x01; // visible
                v19 = ((pItem->flags >> 10) & 3) << 10;
                v20 = pItem->flags & 0x100;
                v17 = pItem->flags & 0x200;

                pItem->msecMoveDelta = 0;
                pItem->pos           = pItem->endMovePos;
                pItem->startMovePos  = pItem->endMovePos;

                switch ( (v19 | v17 | v20 | v16) ^ v18 ^ pItem->flags )
                {
                    case 8:
                        if ( !v20 && !v17 )
                        {
                            if ( pItem->id == JonesHud_selectedWeaponMenuItemID
                                || pItem->id == JonesHud_selectedItemsMenuItemID
                                || pItem->id == JonesHud_selectedSystemMenuItemID
                                || pItem->id == JonesHud_selectedTreasuresMenuItemID
                                || pItem->pos.z == JonesHud_invMenuMaxZ
                                || (JonesHud_apMenuItems[pItem->nextUpItemId]->id == JonesHud_selectedWeaponMenuItemID
                                    || JonesHud_apMenuItems[pItem->nextUpItemId]->id == JonesHud_selectedItemsMenuItemID
                                    || JonesHud_apMenuItems[pItem->nextUpItemId]->id == JonesHud_selectedSystemMenuItemID
                                    || JonesHud_apMenuItems[pItem->nextUpItemId]->id == JonesHud_selectedTreasuresMenuItemID)
                                && pItem->pos.z <= (JonesHud_invMenuMaxZ)
                                && pItem->pos.z >= JonesHud_invMenuMinZ ) // Changed: from fixed constant -0.102f
                            {
                                pItem->flags |= 0x01; // make item visible
                            }

                            pItem->flags &= ~0x08;

                            JonesHud_hudState &= ~0x08u;
                        }

                        break;

                    case 0x10:
                        pItem->flags &= ~0x11u;
                        JonesHud_MenuCollapseColumn(JonesHud_apMenuItems[pItem->nextDownItemId]);
                        break;

                    case 0x20:
                        pItem->flags &= ~0x20u;
                        JonesHud_MenuExpandColumn(pItem);
                        break;

                    case 0x40:
                        pItem->flags &= ~0x41u;
                        if ( pItem->nextLeftItemId == -1
                            || !JonesHud_apMenuItems[pItem->nextLeftItemId]
                            || pItem->id == JonesHud_rootMenuItemId )
                        {
                            // Closing inventory menu here and resuming the game
                            JonesHud_hudState       = 0;
                            jonesCog_g_bMenuVisible = 0;
                            JonesHud_pCloseMenuItem = NULL;

                            sithGamesave_CloseRestore();
                            JonesMain_ResumeGame();
                            JonesReticle_Resume();

                            if ( !sithPlayerControls_g_bCutsceneMode && JonesHud_bMapOpen )
                            {
                                JonesHud_bMapOpen = 0;
                                sithOverlayMap_ToggleMap();
                            }
                        }
                        else
                        {
                            JonesHudMenuItem* pLeftItem = JonesHud_apMenuItems[pItem->nextLeftItemId];
                            float moveRate = JonesHud_menuColumnHorizontalSpacing;
                            if ( pLeftItem == JonesHud_pMenuItemLinkedList )
                            {
                                pLeftItem->endMovePos.x = -0.15800001f;
                                moveRate = pLeftItem->endMovePos.x - pLeftItem->startMovePos.x;
                            }
                            else
                            {
                                pLeftItem->endMovePos.x -= JonesHud_menuColumnHorizontalSpacing;
                            }

                            pLeftItem->flags |= 0x40u;
                            pLeftItem->flags &= ~0x80;
                            moveRate = fabsf(moveRate);
                            JonesHud_StartItemTranslation(pLeftItem, JonesHud_msecMenuCloseSlideDuration, moveRate, 0);
                        }

                        break;

                    case 0x80:
                        pItem->flags &= ~0x80;
                        pItem->flags |= 0x02u;
                        if ( (JonesHud_hudState & 0x04) == 0
                            && pItem->nextRightItemId != -1
                            && JonesHud_apMenuItems[pItem->nextRightItemId]
                            && JonesHud_apMenuItems[pItem->nextRightItemId]->id != JonesHud_rootMenuItemId )
                        {
                            JonesHudMenuItem* pRightItem = JonesHud_apMenuItems[pItem->nextRightItemId];
                            pRightItem->pos          = pItem->endMovePos;
                            pRightItem->startMovePos = pItem->endMovePos;

                            pRightItem->endMovePos = pItem->endMovePos;
                            pRightItem->endMovePos.x +=  JonesHud_menuColumnHorizontalSpacing;

                            pRightItem->flags |= 0x81;
                            JonesHud_StartItemTranslation(pRightItem, JonesHud_msecMenuOpenSlideDuration, /*moveRate=*/JonesHud_menuColumnHorizontalSpacing, 0);
                        }

                        break;

                    default:
                        break;
                }
            }
            else
            {
                // Scope updates item translation
                pItem->msecMoveDelta += JonesHud_msecDeltaTime;
                if ( pItem->msecMoveDelta < pItem->msecMoveDuration )
                {
                    rdVector_SmoothDamp3(&pItem->pos,
                        &pItem->startMovePos,
                        &pItem->endMovePos,
                        4.0f,
                        (float)pItem->msecMoveDelta / (float)(int)pItem->msecMoveDuration
                    );
                }
                else
                {
                    pItem->pos = pItem->endMovePos;
                }
            }
        }

        if ( pItem->nextRightItemId != -1
            && JonesHud_apMenuItems[pItem->nextRightItemId]
            && JonesHud_apMenuItems[pItem->nextRightItemId]->id != JonesHud_rootMenuItemId )
        {
            JonesHud_UpdateItem(JonesHud_apMenuItems[pItem->nextRightItemId]);
        }

        if ( pItem->nextUpItemId != -1
            && JonesHud_apMenuItems[pItem->nextUpItemId]
            && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedWeaponMenuItemID
            && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedItemsMenuItemID
            && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedSystemMenuItemID
            && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedTreasuresMenuItemID )
        {
            JonesHud_UpdateItem(JonesHud_apMenuItems[pItem->nextUpItemId]);
        }
    }
}

void J3DAPI JonesHud_RenderMenuItems(JonesHudMenuItem* pItem)
{
    if ( (pItem->flags & 0x01) != 0 )
    {
        JonesHud_RenderMenuItem(pItem);
    }

    if ( pItem->nextRightItemId != -1
        && JonesHud_apMenuItems[pItem->nextRightItemId]
        && JonesHud_apMenuItems[pItem->nextRightItemId]->id != JonesHud_rootMenuItemId )
    {
        JonesHud_RenderMenuItems(JonesHud_apMenuItems[pItem->nextRightItemId]);
    }

    if ( pItem->nextUpItemId != -1
        && JonesHud_apMenuItems[pItem->nextUpItemId]
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedWeaponMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedItemsMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedSystemMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedTreasuresMenuItemID )
    {
        JonesHud_RenderMenuItems(JonesHud_apMenuItems[pItem->nextUpItemId]);
    }
}

void J3DAPI JonesHud_RenderMenuItem(JonesHudMenuItem* pItem)
{
    rdMatrix34 placement = rdroid_g_identMatrix34;

    if ( pItem )
    {
        rdVector3 vecIconRadiusScale;
        vecIconRadiusScale.z = 0.018204151f / pItem->prdIcon->data.pModel3->size;
        vecIconRadiusScale.y = vecIconRadiusScale.z;
        vecIconRadiusScale.x = vecIconRadiusScale.z;
        rdMatrix_PostScale34(&placement, &vecIconRadiusScale);

        rdVector3 vecScale;
        vecScale.z = JonesHud_aDfltMenuItemOrients[pItem->id].scale * JonesHud_itemAspectScaleSize; // Added: Multiply scale by aspect ratio scale
        vecScale.y = vecScale.z;
        vecScale.x = vecScale.z;
        rdMatrix_PostScale34(&placement, &vecScale);

        if ( pItem == JonesHud_pCurSelectedMenuItem )
        {
            if ( pItem->pos.z == pItem->endMovePos.z )
            {
                // Scope draws selected item text 
                // 
                //float v26 = 20.0f * JonesHud_heightAspectRatioScale; // Unused

                rdVector3 pos = pItem->pos;

                rdMatrix34 orient = rdroid_g_identMatrix34;
                orient.dvec.x = pos.x;
                orient.dvec.y = pos.y;
                orient.dvec.z = pos.z;

                // Transform position to view space (rotated for orient [world space])
                rdMatrix34 tmat;
                rdMatrix_Multiply34(&tmat, &orient, &rdCamera_g_pCurCamera->viewMatrix);
                rdCamera_PerspProject(&pos, &tmat.dvec);

                rdFontColor fontColor;
                rdVector_Set4(&fontColor[0], 1.0f, 0.0f, 0.0f, 1.0f);
                rdVector_Set4(&fontColor[1], 1.0f, 0.0f, 0.0f, 1.0f);
                rdVector_Set4(&fontColor[2], 1.0f, 1.0f, 0.0f, 1.0f);
                rdVector_Set4(&fontColor[3], 1.0f, 1.0f, 0.0f, 1.0f);
                rdFont_SetFontColor(fontColor);

                bool bDisabled = (sithInventory_g_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
                    ? sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[pItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED
                    : 0;

                const char* pItemName = bDisabled
                    ? jonesString_GetString("JONES_STR_INV_NOUSE")
                    : jonesString_GetString(JonesHud_apItemNames[pItem->id]);

                if ( pItemName )
                {
                    char aItemName[512] = { 0 };
                    STD_STRCPY(aItemName, pItemName);

                    float amount = 0.0f;
                    if ( (sithInventory_g_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
                    {
                        amount = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[pItem->inventoryID].amount;
                    }

                    char aItemText[256] = { 0 };
                    if ( pItem->inventoryID == -1 || pItem->id > JONESHUD_MENU_INVITEM_BONUSMAP )
                    {
                        STD_FORMAT(aItemText, "%s", aItemName);
                    }
                    else
                    {
                        switch ( pItem->id )
                        {
                            case JONESHUD_MENU_WEAP_GLOVES:
                            case JONESHUD_MENU_WEAP_MACHETE:
                                STD_FORMAT(aItemText, "%s", aItemName);
                                break;

                            case JONESHUD_MENU_WEAP_WHIP:
                                if ( (sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.flags & SITH_AF_ELECTRICWHIP) == 0 )
                                {
                                    STD_FORMAT(aItemText, "%s", aItemName);
                                    break;
                                }

                                STD_FORMAT(aItemText, "%s  %i\n", aItemName, (int)amount);
                                break;

                            case JONESHUD_MENU_TREASURE_CHEST:
                                STD_FORMAT(aItemText, "%s  %i ($%i)\n", aItemName, JonesHud_numFoundTreasures, (int)amount);
                                break;

                            case JONESHUD_MENU_INVITEM_PATCHKIT:
                                STD_FORMAT(aItemText, "%s  %i\n", aItemName, (int)amount);
                                break;

                            default:
                                if ( (unsigned int)(int32_t)amount <= 1 )
                                {
                                    STD_FORMAT(aItemText, "%s", aItemName);
                                    break;
                                }

                                STD_FORMAT(aItemText, "%s  %i\n", aItemName, (int)amount);
                                break;
                        }
                    }

                    float textX = pos.x;
                    float textY = pos.y - (JonesHud_menuItemTextPosY * JonesHud_heightAspectRatioScale);
                    textX /= (JonesHud_widthAspectRatioScale * RD_REF_WIDTH);
                    textY /= (JonesHud_heightAspectRatioScale * RD_REF_HEIGHT);
                    rdFont_DrawTextLineClippedEx(aItemText, textX, textY, RD_FIXEDPOINT_RHW_SCALE_X1, JonesHud_pMenuItemTextFont, RDFONT_ALIGNCENTER, JonesHud_menuItemTextSize); // Altered: Replaced call with font size scale
                }
            }

            // Highlight selected item by scaling it up a little bit
            bool bDisabled = false;
            if ( (sithInventory_g_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
            {
                bDisabled = (sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[pItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED) != 0;
            }

            if ( !bDisabled )
            {
                rdMatrix_PostScale34(&placement, &JonesHud_vecSelectedMenuItemScale);
            }
        }

        // Rotate itme matrix by RPY
        rdVector3 pyrOrient = { 0 };
        pyrOrient.roll = pItem->pyr.roll;
        rdMatrix_PostRotate34(&placement, &pyrOrient);

        rdVector_Zero3(&pyrOrient);
        pyrOrient.pitch = pItem->pyr.pitch;
        rdMatrix_PostRotate34(&placement, &pyrOrient);

        rdVector_Zero3(&pyrOrient);
        pyrOrient.yaw = pItem->pyr.yaw;
        rdMatrix_PostRotate34(&placement, &pyrOrient);

        placement.dvec = pItem->pos;

        rdVector4 color;
        rdModel3_GetThingColor(pItem->prdIcon, &color);
        float oalpha = color.alpha;
        color.alpha  = pItem->alpha;

        if ( (pItem->flags & 0x08) == 0
            && ((pItem->flags & 0x10) != 0
                || (pItem->flags & 0x20) != 0
                || (pItem->flags & 0x80) != 0
                || (pItem->flags & 0x40) != 0) )
        {
            float v14 = 1.0f, v15 = 1.0f;// Added Init to 1
            switch ( ((((pItem->flags >> 10) & 3) << 10) | pItem->flags & 0x302) ^ pItem->flags & 0x01 ^ pItem->flags )
            {
                case 0x10:
                    v15 = pItem->startMovePos.z - pItem->endMovePos.z;
                    v14 = pItem->pos.z - pItem->endMovePos.z;
                    break;

                case 0x20:
                    v15 = pItem->startMovePos.z - pItem->endMovePos.z;
                    v14 = pItem->startMovePos.z - pItem->pos.z;
                    break;

                case 0x40:
                    v15 = pItem->startMovePos.x - pItem->endMovePos.x;
                    v14 = pItem->pos.x - pItem->endMovePos.x;
                    break;

                case 0x80:
                    v15 = pItem->startMovePos.x - pItem->endMovePos.x;
                    v14 = pItem->startMovePos.x - pItem->pos.x;
                    break;

                default:
                    break;
            }

            float alpha = v14 / v15;
            if ( alpha > 0.66666669f )
            {
                alpha = (alpha - 0.66666669f) * 0.60000002f / 0.33333334f + 0.40000001f;
            }
            else
            {
                alpha = 0.60000002f * alpha;
            }

            color.alpha = alpha;

            bool bDisabled = false;
            if ( (sithInventory_g_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
            {
                bDisabled = (sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[pItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED) != 0;
            }

            if ( bDisabled )
            {
                if ( color.alpha >= 0.40000001f )
                {
                    color.alpha  = 0.40000001f;
                }
            }
            else
            {
                if ( color.alpha >= 0.94999999f )
                {
                    color.alpha = 0.94999999f;
                }
            }

            if ( (pItem->flags & 0x10) != 0 || (pItem->flags & 0x40) != 0 )
            {
                if ( color.alpha <= 0.0f )
                {
                    color.alpha  = 0.0f;
                }
            }
        }
        else
        {
            bool bDisabled = false;
            if ( (sithInventory_g_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
            {
                bDisabled = (sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[pItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED) != 0;
            }

            if ( bDisabled )
            {
                color.alpha = 0.40000001f;
            }
            else
            {
                color.alpha = 0.94999999f;
            }
        }

        rdModel3_SetThingColor(pItem->prdIcon, &color);
        rdModel3_Draw(pItem->prdIcon, &placement);

        pItem->alpha = color.alpha;
        color.alpha  = oalpha;
        rdModel3_SetThingColor(pItem->prdIcon, &color);
    }
}

void JonesHud_MenuActivateItem(void)
{
    if ( !JonesHud_pCurSelectedMenuItem || !sithPlayer_g_pLocalPlayerThing )
    {
        return;
    }

    int32_t bItemDisable = 0;
    if ( (sithInventory_g_aTypes[JonesHud_pCurSelectedMenuItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
    {
        bItemDisable = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_pCurSelectedMenuItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED;
    }

    if ( bItemDisable )
    {
    close_menu:
        sithGamesave_CloseRestore();
        JonesHud_MenuClose();
        return;
    }

    if ( JonesHud_pCurSelectedMenuItem->inventoryID != -1 // i.e.: if system menu is activated. Note the system menus don't have inventory ID (e.g. settings, load/save, exit etc..)
        && JonesHud_pCurSelectedMenuItem->id != JONESHUD_MENU_TREASURE_CHEST )
    {
        if ( JonesHud_pCurSelectedMenuItem->id <= JONESHUD_MENU_WEAP_BAZOOKA
            || JonesHud_pCurSelectedMenuItem->id == JONESHUD_MENU_INVITEM_WEAP_MIRROR
            || JonesHud_pCurSelectedMenuItem->id == JONESHUD_MENU_INVITEM_ZIPPO
            || JonesHud_pCurSelectedMenuItem->id >= JONESHUD_MENU_INVITEM_IMP1
            && JonesHud_pCurSelectedMenuItem->id <= JONESHUD_MENU_INVITEM_IMP5 )
        {
            if ( sithWeapon_IsMountingWeapon(sithPlayer_g_pLocalPlayerThing) )
            {
                JonesHud_selectedWeaponID = JonesHud_pCurSelectedMenuItem->inventoryID;
            }
            else
            {
                sithWeapon_SelectWeapon(sithPlayer_g_pLocalPlayerThing, (SithWeaponId)JonesHud_pCurSelectedMenuItem->inventoryID);
                JonesHud_selectedWeaponID = -1;
            }
        }
        else
        {
            sithInventory_SetCurrentItem(sithPlayer_g_pLocalPlayerThing, JonesHud_pCurSelectedMenuItem->inventoryID);

            if ( JonesHud_pCurSelectedMenuItem->id >= JONESHUD_MENU_HEALTH_SMALL
                && JonesHud_pCurSelectedMenuItem->id <= JONESHUD_MENU_HEALTH_POISONKIT
                && sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health < (double)sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.maxHealth )
            {
                // Sends cog msg user0 to health items and poison kit
                jonesInventory_UseItem(sithPlayer_g_pLocalPlayerThing, JonesHud_pCurSelectedMenuItem->inventoryID);
            }

            else if ( jonesInventory_ActivateItem(sithPlayer_g_pLocalPlayerThing, JonesHud_pCurSelectedMenuItem->inventoryID)
                || !sithPlayer_g_pLocalPlayerThing->pInSector
                || (sithPlayer_g_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 ) // This last 2 sector & underwater checks should be moved to else scope
            {
                if ( JonesHud_pCurSelectedMenuItem->id == JONESHUD_MENU_INVITEM_PATCHKIT )
                {
                    if ( sithWorld_g_pCurrentWorld )
                    {
                        tSoundHandle hSndInfo = sithSound_Load(sithWorld_g_pCurrentWorld, "riv_raft_repair.wav"); // inflate sound fx
                        if ( hSndInfo )
                        {
                            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(hSndInfo, 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                        }
                    }
                }
            }
            else
            {
                // Item couldn't be activated, play whoops indy voice
                int sndIdx = 0;
                int v23 = 0;
                do
                {
                    do
                    {
                        sndIdx = rand() % 4;
                        v23 = sndIdx + 7;       // from idx 7 - 11
                    } while ( !JonesHud_aSoundFxHandles[sndIdx + 7] );
                } while ( sndIdx == JonesHud_curWhoopsSndFxIdx );

                JonesHud_curWhoopsSndFxIdx = sndIdx;
                sithVoice_PlayThingVoice(sithPlayer_g_pLocalPlayerThing, JonesHud_aSoundFxHandles[v23], 1.0f);
            }
        }

        goto close_menu;
    }

    // CHandle activation of system menu item

    int dlgResult = 0;
    JonesHud_bItemActivated = 1;
    switch ( JonesHud_pCurSelectedMenuItem->id )
    {
        case JONESHUD_MENU_UNKNOWN_124:
        case JONESHUD_MENU_IQ: // Show game statistics
        {
            jonesInventory_UpdateSolvedHintsStatistics();
            jonesInventory_UpdateGameTimeStatistics(sithTime_g_msecGameTime);
            SithGameStatistics* pStatistics = sithGamesave_GetGameStatistics();
            if ( pStatistics && sithGamesave_LockGameStatistics() )
            {
                pStatistics->aLevelStatistic[pStatistics->curLevelNum].iqPoints = jonesInventory_GetTotalIQPoints() - JonesHud_levelStartIQPoints;
                HWND hwnd = stdWin95_GetWindow();
                jonesConfig_ShowStatisticsDialog(hwnd, pStatistics);
                sithGamesave_UnlockGameStatistics();
            }

            dlgResult = 2;
        } break;

        case JONESHUD_MENU_HELP: // help
            // TODO: Help menu item is not put in place in ResetMenuitems
            //       so this scope could be skipped
            if ( JonesHud_hProcessHelp )
            {
                DWORD exitCode;
                GetExitCodeProcess(JonesHud_hProcessHelp, &exitCode);
                if ( exitCode != STILL_ACTIVE )
                    //if ( exitCode != 259 )
                {
                    JonesHud_hProcessHelp = NULL;
                }
            }

            JonesHud_hProcessHelp = JonesHud_OpenHelp(JonesHud_hProcessHelp);
            dlgResult = 2;
            break;

        case JONESHUD_MENU_SAVE_GAME: // save game
        {
            if ( jonesCog_g_bEnableGamesave )
            {
                JonesHud_RestoreTreasuresStatistics();
                char aFilePath[JONESCONFIG_GAMESAVE_FILEPATHSIZE] = { 0 };
                HWND hwnd = stdWin95_GetWindow();
                dlgResult = jonesConfig_GetSaveGameFilePath(hwnd, aFilePath);
                if ( dlgResult == 1 )
                {
                    jonesInventory_UpdateGameTimeStatistics(sithTime_g_msecGameTime);
                    sithGamesave_Save(aFilePath, 1);
                }
            }

        } break;

        case JONESHUD_MENU_LOAD_GAME:
        {
            if ( jonesCog_g_bEnableGamesave )   // load game
            {
                STD_ZEROMEM(JonesHud_aSlectedNdsFilePath, sizeof(JonesHud_aSlectedNdsFilePath));
                HWND hwnd = stdWin95_GetWindow();
                dlgResult = jonesConfig_GetLoadGameFilePath(hwnd, JonesHud_aSlectedNdsFilePath);
                if ( dlgResult == 1 )
                {
                    if ( strlen(JonesHud_aSlectedNdsFilePath) )
                    {
                        if ( JonesMain_HasStarted() )
                        {
                            JonesHud_bRestoreActivated = 1;
                        }

                        JonesHud_bRestoreGameStatistics = 1;
                    }
                }
            }

        } break;

        case JONESHUD_MENU_GAME_SETTINGS: // game play options
        {
            HWND hwnd = stdWin95_GetWindow();
            dlgResult = jonesConfig_ShowGamePlayOptions(hwnd);
        } break;

        case JONESHUD_MENU_CONTROLS_SETTINGS:   // control options
        {
            HWND hwnd = stdWin95_GetWindow();
            dlgResult = jonesConfig_ShowControlOptions(hwnd);
        } break;

        case JONESHUD_MENU_DISPLAY_SETTINGS:    // display settings
        {
            JonesDisplaySettings* pDSettings   = JonesMain_GetDisplaySettings();
            StdDisplayEnvironment* pDisplayEnv = JonesMain_GetDisplayEnvironment();
            HWND hwnd = stdWin95_GetWindow();
            dlgResult = jonesConfig_ShowDisplaySettingsDialog(hwnd, pDisplayEnv, pDSettings);
            if ( dlgResult == 1 )
            {
                JonesMain_RefreshDisplayDevice();
            }

        } break;

        case JONESHUD_MENU_SOUND_SETTINGS: // sound settings
        {
            JonesSoundSettings sndSettings;
            sndSettings.maxSoundVolume = Sound_GetMaxVolume();
            sndSettings.b3DHWSupport   = Sound_Get3DHWState();
            sndSettings.bReverseSound  = stdConfig_GetBool(JONESCONFIG_CFG_SOUND_REVERSE, false);

            HWND hwnd = stdWin95_GetWindow();
            dlgResult = jonesConfig_ShowSoundSettingsDialog(hwnd, &sndSettings);
            if ( dlgResult == 1 )
            {
                Sound_SetMaxVolume(sndSettings.maxSoundVolume);
                Sound_Set3DHWState(sndSettings.b3DHWSupport);
                Sound_SetReverseSound(sndSettings.bReverseSound);
            }

        } break;

        case JONESHUD_MENU_EXIT: // exit
        {
            char aSaveGameFilename[JONESCONFIG_GAMESAVE_FILEPATHSIZE] = { 0 };
            HWND hwnd = stdWin95_GetWindow();
            dlgResult = jonesConfig_ShowExitGameDialog(hwnd, aSaveGameFilename);
            if ( dlgResult == 2 )
            {
                JonesHud_bExitActivated = 0;
            }
            else
            {
                JonesHud_bExitActivated = 1;
                if ( dlgResult == 1187 )
                {
                    sithGamesave_Save(aSaveGameFilename, 1);
                    if ( sithGamesave_Process() )
                    {
                        const char* pNoSaveFormat = jonesString_GetString("JONES_STR_NOSAVE");
                        if ( pNoSaveFormat )
                        {
                            char aMsg[256] = { 0 };
                            STD_FORMAT(aMsg, pNoSaveFormat, aSaveGameFilename);
                            hwnd = stdWin95_GetWindow();
                            jonesConfig_ShowMessageDialog(hwnd, "JONES_STR_EXIT", aMsg, 141);
                        }

                        dlgResult = 2;
                        JonesHud_bExitActivated = 0;
                    }
                    else
                    {
                        dlgResult = 1;
                    }
                }
            }

        } break;

        default:
            break;
    }

    if ( dlgResult == 1 )
    {
        if ( JonesHud_pCurSelectedMenuItem->id != JONESHUD_MENU_SAVE_GAME )
        {
            sithGamesave_CloseRestore();
        }

        JonesHud_MenuClose();
    }
}

void JonesHud_ResetMenuItems(void)
{
    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return;
    }

    JonesHud_hudState = 0;

    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_apMenuItems); ++i )
    {
        if ( JonesHud_apMenuItems[i] )
        {
            JonesHud_apMenuItems[i]->msecMoveDuration = 0;
            JonesHud_apMenuItems[i]->msecMoveEndTime  = 0;
            JonesHud_apMenuItems[i]->msecMoveDelta    = 0;
            JonesHud_apMenuItems[i]->flags            = 0;
            JonesHud_apMenuItems[i]->alpha            = 1.0f;
            JonesHud_apMenuItems[i]->nextDownItemId   = -1;
            JonesHud_apMenuItems[i]->nextRightItemId  = -1;
            JonesHud_apMenuItems[i]->nextUpItemId     = -1;
            JonesHud_apMenuItems[i]->nextLeftItemId   = -1;
            JonesHud_apMenuItems[i]->pyr              = JonesHud_aDfltMenuItemOrients[JonesHud_apMenuItems[i]->id].pyr;

            JonesHud_apMenuItems[i]->startMovePos.x = 0.0f;
            JonesHud_apMenuItems[i]->startMovePos.y = 0.0f;
            JonesHud_apMenuItems[i]->startMovePos.z = 0.0f;

            JonesHud_apMenuItems[i]->endMovePos.x = 0.0f;
            JonesHud_apMenuItems[i]->endMovePos.y = 0.0f;
            JonesHud_apMenuItems[i]->endMovePos.z = 0.0f;

            JonesHud_apMenuItems[i]->pos.x = 0.0f;
            JonesHud_apMenuItems[i]->pos.y = 0.0f;
            JonesHud_apMenuItems[i]->pos.z = 0.0f;
        }
    }

    JonesHudMenuItem* pLastSelectedItem = NULL;
    if ( JonesHud_selectedSystemMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedSystemMenuItemID] )
    {
        pLastSelectedItem = JonesHud_apMenuItems[JonesHud_selectedSystemMenuItemID];
    }

    if ( JonesHud_selectedItemsMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedItemsMenuItemID] )
    {
        if ( pLastSelectedItem )
        {
            JonesHud_SetLeftRightItems(JonesHud_apMenuItems[JonesHud_selectedItemsMenuItemID], pLastSelectedItem);
        }

        pLastSelectedItem = JonesHud_apMenuItems[JonesHud_selectedItemsMenuItemID];
    }

    if ( JonesHud_selectedTreasuresMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedTreasuresMenuItemID] )
    {
        if ( pLastSelectedItem )
        {
            JonesHud_SetLeftRightItems(JonesHud_apMenuItems[JonesHud_selectedTreasuresMenuItemID], pLastSelectedItem);
        }

        pLastSelectedItem = JonesHud_apMenuItems[JonesHud_selectedTreasuresMenuItemID];
    }

    if ( JonesHud_selectedWeaponMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedWeaponMenuItemID] )
    {
        if ( pLastSelectedItem )
        {
            JonesHud_SetLeftRightItems(JonesHud_apMenuItems[JonesHud_selectedWeaponMenuItemID], pLastSelectedItem);
        }

        pLastSelectedItem = JonesHud_apMenuItems[JonesHud_selectedWeaponMenuItemID];
    }

    // TODO: Add check for JonesHud_selectedSystemMenuItemID >= 0
    JonesHud_SetLeftRightItems(JonesHud_apMenuItems[JonesHud_selectedSystemMenuItemID], pLastSelectedItem);
    JonesHud_pMenuItemLinkedList = pLastSelectedItem;

    // Setup the 3 inventory menus
    for ( size_t i = 0; i < 3; ++i )
    {
        // Added: Default init.
        int selectedItemId           = -1;
        JonesHudMenuType lastItemId  = JONESHUD_MENU_WEAP_GLOVES;
        JonesHudMenuType firstItemId = JONESHUD_MENU_WEAP_GLOVES;

        if ( i == 0 ) // Weapons menu
        {
            if ( JonesHud_selectedWeaponMenuItemID == -1 || !JonesHud_apMenuItems[JonesHud_selectedWeaponMenuItemID] )
            {
                continue;
            }

            selectedItemId = JonesHud_selectedWeaponMenuItemID;
            firstItemId = JONESHUD_MENU_WEAP_GLOVES;
            lastItemId = JONESHUD_MENU_WEAP_BAZOOKA;
        }
        else if ( i == 1 ) // Treasury menu
        {
            if ( JonesHud_selectedTreasuresMenuItemID == -1
                || !JonesHud_apMenuItems[JonesHud_selectedTreasuresMenuItemID] )
            {
                continue;
            }

            selectedItemId = JonesHud_selectedTreasuresMenuItemID;
            firstItemId = JONESHUD_MENU_HEALTH_SMALL;
            lastItemId = JONESHUD_MENU_TREASURE_CHEST;
        }
        else if ( i == 2 ) // inventory items menu
        {
            if ( JonesHud_selectedItemsMenuItemID == -1 || !JonesHud_apMenuItems[JonesHud_selectedItemsMenuItemID] )
            {
                continue;
            }

            selectedItemId = JonesHud_selectedItemsMenuItemID;
            firstItemId = JONESHUD_MENU_INVITEM_ZIPPO;
            lastItemId = JONESHUD_MENU_INVITEM_BONUSMAP;
        }


        JonesHudMenuType curItemId = selectedItemId + 1;
        JonesHudMenuItem* pCurItem = JonesHud_apMenuItems[selectedItemId];
        for ( int j = 0; j < lastItemId - firstItemId; ++j )
        {
            if ( curItemId > lastItemId )
            {
                curItemId = firstItemId;
            }

            if ( curItemId != -1 && JonesHud_apMenuItems[curItemId] && JonesHud_apMenuItems[curItemId]->inventoryID != -1 )
            {
                float amount = (sithInventory_g_aTypes[JonesHud_apMenuItems[curItemId]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
                    ? sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[curItemId]->inventoryID].amount
                    : 0.0f;

                int32_t v5;
                if ( (int32_t)amount
                    || curItemId == JONESHUD_MENU_TREASURE_CHEST
                    || JonesHud_apMenuItems[curItemId]->id <= 11 // 11 - is num player weapons TODO: make global constant or use existing one
                    && ((sithInventory_g_aTypes[JonesHud_apMenuItems[curItemId]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) == 0
                        ? (v5 = 0)
                        : (v5 = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[curItemId]->inventoryID].status & SITHINVENTORY_ITEM_FOUND),
                        v5) )
                {
                    JonesHud_SetDownUpItems(JonesHud_apMenuItems[curItemId], pCurItem);
                    pCurItem = JonesHud_apMenuItems[curItemId];
                }
            }

            ++curItemId;
        }

        if ( pCurItem != JonesHud_apMenuItems[selectedItemId] )
        {
            JonesHud_SetDownUpItems(JonesHud_apMenuItems[selectedItemId], pCurItem);
        }
    }

    // Setup the system menu items
    if ( JonesHud_selectedSystemMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedSystemMenuItemID] )
    {
        int curItemId = JonesHud_selectedSystemMenuItemID + 1;
        JonesHudMenuItem* pCurItem = JonesHud_apMenuItems[JonesHud_selectedSystemMenuItemID];
        for ( size_t i = 0; i < 8; ++i ) // 8 = 9 sys items - 1 
        {
            // Added: Skip help menu option, i.e. remove it
            if ( curItemId == JONESHUD_MENU_HELP )
            {
                ++curItemId;
            }

            if ( curItemId > JONESHUD_MENU_EXIT )
            {
                curItemId = JONESHUD_MENU_IQ;
            }

            if ( pCurItem != JonesHud_apMenuItems[curItemId] )
            {
                JonesHud_SetDownUpItems(JonesHud_apMenuItems[curItemId], pCurItem);
                pCurItem = JonesHud_apMenuItems[curItemId];
            }

            ++curItemId;
        }

        if ( pCurItem != JonesHud_apMenuItems[JonesHud_selectedSystemMenuItemID] )
        {
            JonesHud_SetDownUpItems(JonesHud_apMenuItems[JonesHud_selectedSystemMenuItemID], pCurItem);
        }
    }

    // TODO: Make sure JonesHud_pMenuItemLinkedList is not null here
    JonesHud_pMenuItemLinkedList->startMovePos.x = -0.167f;
    JonesHud_pMenuItemLinkedList->startMovePos.y = 0.0099999998f;
    JonesHud_pMenuItemLinkedList->startMovePos.z = JonesHud_invMenuMinZ; // Changed: from fixed constant -0.102f

    JonesHud_pMenuItemLinkedList->startMovePos.x = -0.15800001f; // TODO: Why assigning again to x with different value??

    JonesHud_pMenuItemLinkedList->pos = JonesHud_pMenuItemLinkedList->startMovePos;

    JonesHud_pMenuItemLinkedList->endMovePos.x = -0.167f;
    JonesHud_pMenuItemLinkedList->endMovePos.y = 0.0099999998f;
    JonesHud_pMenuItemLinkedList->endMovePos.z = JonesHud_invMenuMinZ; // Altered: Using JonesHud_invMenuMinZ instead of fixed constant -0.102f

    JonesHud_pMenuItemLinkedList->endMovePos.x +=  JonesHud_menuColumnHorizontalSpacing;
    JonesHud_invMenuMaxZ = JonesHud_invMenuMinZ + JonesHud_menuColumnHeight; // Altered: Using JonesHud_invMenuMinZ instead of fixed constant -0.102f
}

void J3DAPI JonesHud_SetLeftRightItems(JonesHudMenuItem* pItem1, JonesHudMenuItem* pItem2)
{
    pItem1->nextRightItemId = pItem2->id;
    pItem2->nextLeftItemId  = pItem1->id;
}

void J3DAPI JonesHud_SetDownUpItems(JonesHudMenuItem* pItem1, JonesHudMenuItem* pItem2)
{
    pItem1->nextDownItemId = pItem2->id;
    pItem2->nextUpItemId   = pItem1->id;
}

void J3DAPI JonesHud_MenuExpandColumn(JonesHudMenuItem* pItem)
{
    // This function has to do with logic that shows up to 4 item in the column of currently selected item

    if ( pItem->nextUpItemId != -1
        && JonesHud_apMenuItems[pItem->nextUpItemId]
        && JonesHud_apMenuItems[pItem->nextUpItemId] != pItem )
    {
        JonesHud_hudState |= 0x02;
        while ( pItem->nextUpItemId != -1
            && JonesHud_apMenuItems[pItem->nextUpItemId]
            && pItem->pos.z < JonesHud_invMenuMaxZ
            && (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x01) != 0
            && pItem->pos.z < JonesHud_apMenuItems[pItem->nextUpItemId]->pos.z )
        {
            pItem = JonesHud_apMenuItems[pItem->nextUpItemId];
        }

        // If column was collapsing but now triggered expand,
        // the item is the top element of collapsing column
        if ( (pItem->flags & 0x08) != 0
            || (pItem->flags & 0x10) != 0
            || (pItem->flags & 0x20) != 0
            || (pItem->flags & 0x80) != 0
            || (pItem->flags & 0x40) != 0 )
        {
            pItem->startMovePos  = pItem->pos;
            pItem->endMovePos.z += JonesHud_menuColumnSpacing;

            pItem->flags &= ~0x10u;
            pItem->flags |= 0x20u;
            JonesHud_StartItemTranslation(pItem, JonesHud_msecMenuColumnExpandDuration, /*moveRate=*/JonesHud_menuColumnSpacing, /*bMoveUp=*/1);
        }
        // Check if stop expanding column
        else if ( pItem->nextUpItemId == -1
            || !JonesHud_apMenuItems[pItem->nextUpItemId]
            || (pItem->pos.z + JonesHud_menuColumnSpacing) > JonesHud_invMenuMaxZ // Fixed: Altered z value check by adding Z spacing. This fixes to always display correct number of items on expand
            || (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x01) != 0 )
        {
            JonesHud_hudState &= ~0x02; // flag end of opening column item row animation
        }
        else
        {
            JonesHudMenuItem* pUpItem = JonesHud_apMenuItems[pItem->nextUpItemId];
            pUpItem->startMovePos = pItem->pos;
            pUpItem->endMovePos   = pItem->pos;

            pUpItem->pos           = pItem->pos;
            pUpItem->endMovePos.z += JonesHud_menuColumnSpacing;

            pUpItem->flags |= 0x20;
            JonesHud_StartItemTranslation(pUpItem, JonesHud_msecMenuColumnExpandDuration, /*moveRate=*/JonesHud_menuColumnSpacing, /*bMoveUp=*/1);
        }
    }
}

void J3DAPI JonesHud_MenuCollapseColumn(JonesHudMenuItem* pItem)
{
    while ( pItem->nextUpItemId != -1
        && JonesHud_apMenuItems[pItem->nextUpItemId]
        && pItem->pos.z < JonesHud_invMenuMaxZ
        && (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x01) != 0 // item visible
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedWeaponMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedItemsMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedSystemMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedTreasuresMenuItemID
        && pItem->pos.z <= JonesHud_apMenuItems[pItem->nextUpItemId]->pos.z )
    {
        pItem = JonesHud_apMenuItems[pItem->nextUpItemId];
    }

    if ( (pItem->flags & 0x02) != 0 || (pItem->flags & 0x10) != 0 )
    {
        if ( JonesHud_pCloseMenuItem )
        {
            //(JonesHud_item_flag_state_55514C & 0xFF) = ~(1 << ((pItem->flags >> 10) & 3)) & JonesHud_item_flag_state_55514C;
            //JonesHud_item_flag_state_55514C = (JonesHud_item_flag_state_55514C & 0xFF) & ~(1 << ((pItem->flags >> 10) & 3));
            JonesHud_item_flag_state_55514C &= ~(1 << ((pItem->flags >> 10) & 0x03));

            //if ( !(uint8_t)JonesHud_item_flag_state_55514C )
            if ( (JonesHud_item_flag_state_55514C & 0xFF) == 0 )
            {
                // Play close iv menu sound
                if ( JonesHud_aSoundFxHandles[3] )// inv_expand.wav
                {
                    JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[3], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                }

                JonesHud_pCloseMenuItem->flags |= 0x40u;
                JonesHud_pCloseMenuItem->flags &= ~0x80;

                float moveRate = JonesHud_menuColumnHorizontalSpacing;
                if ( JonesHud_pCloseMenuItem == JonesHud_pMenuItemLinkedList )
                {
                    JonesHud_pCloseMenuItem->endMovePos.x = -0.15800001f;
                    moveRate = JonesHud_pCloseMenuItem->endMovePos.x - JonesHud_pCloseMenuItem->startMovePos.x;
                }
                else
                {
                    JonesHud_pCloseMenuItem->endMovePos.x = JonesHud_pCloseMenuItem->endMovePos.x - JonesHud_menuColumnHorizontalSpacing;
                }

                moveRate = fabsf(moveRate);
                JonesHud_StartItemTranslation(JonesHud_pCloseMenuItem, JonesHud_msecMenuCloseSlideDuration, moveRate, /*bMoveUp=*/0);
            }
        }
    }
    else
    {
        JonesHudMenuItem* pDownPos = JonesHud_apMenuItems[pItem->nextDownItemId];
        pItem->startMovePos = pItem->pos;
        pItem->endMovePos   = pDownPos->pos;

        pItem->flags &= ~0x20u;
        pItem->flags |= 0x10u;
        JonesHud_StartItemTranslation(pItem, JonesHud_msecMenuColumnCollapseDuration, /*moveRate=*/JonesHud_menuColumnSpacing, /*bMoveUp=*/1);
    }
}

void J3DAPI JonesHud_InventoryItemChanged(int typeId)
{
    ;
    int32_t bFound;
    JonesHudMenuItem* pMenuItem;
    int itemID;

    pMenuItem = 0;
    if ( sithPlayer_g_pLocalPlayerThing && sithPlayer_g_pLocalPlayerThing->pInSector )
    {
        if ( JonesHud_pCurInvChangedItem )
        {
            JonesHud_FreeMenuItem(JonesHud_pCurInvChangedItem);
            JonesHud_pCurInvChangedItem = NULL;
        }

        for ( itemID = 0; itemID < STD_ARRAYLEN(JonesHud_apMenuItems); ++itemID )
        {
            if ( itemID != -1 && JonesHud_apMenuItems[itemID] && JonesHud_apMenuItems[itemID]->inventoryID == typeId )
            {
                if ( (sithInventory_g_aTypes[JonesHud_apMenuItems[itemID]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
                {
                    bFound = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[itemID]->inventoryID].status & SITHINVENTORY_ITEM_FOUND;
                }
                else
                {
                    bFound = 0;
                }

                if ( bFound )
                {
                    switch ( itemID )
                    {
                        case JONESHUD_MENU_WEAP_TOKAREV:
                            itemID = JONESHUD_MENU_AMMO_TOKAREV;
                            break;

                        case JONESHUD_MENU_WEAP_MAUSER:
                            itemID = JONESHUD_MENU_AMMO_MAUSER;
                            break;

                        case JONESHUD_MENU_WEAP_SIMONOV:
                            itemID = JONESHUD_MENU_AMMO_SIMONOV;
                            break;

                        case JONESHUD_MENU_WEAP_SUBMACHINE:
                            itemID = JONESHUD_MENU_AMMO_SUBMACHINE;
                            break;

                        case JONESHUD_MENU_WEAP_SHOTGUN:
                            itemID = JONESHUD_MENU_AMMO_SHOTGUN;
                            break;

                        case JONESHUD_MENU_WEAP_BAZOOKA:
                            itemID = JONESHUD_MENU_AMMO_BAZOOKA;
                            break;

                        default:
                            break;
                    }
                }

                pMenuItem = JonesHud_apMenuItems[itemID];
                break;
            }
        }

        JonesHud_pCurInvChangedItem = NULL;
        if ( pMenuItem )
        {
            JonesHud_pCurInvChangedItem = JonesHud_NewMenuItem(JonesHud_apMenuItems[itemID]->prdIcon->data.pModel3);
            JonesHud_pCurInvChangedItem->id          = pMenuItem->id;
            JonesHud_pCurInvChangedItem->inventoryID = pMenuItem->inventoryID;

            // TODO: make global vars for below assignment of constant values

            JonesHud_pCurInvChangedItem->endMovePos.x = -0.167f * JonesHud_itemAspectScalePosition;
            JonesHud_pCurInvChangedItem->endMovePos.y = 0.0099999998f * JonesHud_itemAspectScalePosition;
            JonesHud_pCurInvChangedItem->endMovePos.z = JonesHud_invMenuMinZ; // Changed: from fixed constant -0.102f

            JonesHud_pCurInvChangedItem->endMovePos.x = -0.15800001f * JonesHud_itemAspectScalePosition;// ?? few lines up was set to -0.167f // Changed: Fix end position by multiplying it with aspect scale

            JonesHud_pCurInvChangedItem->pos.x = -0.167f;      // Note not really necessary to init x here as it's assigned few lines lower
            JonesHud_pCurInvChangedItem->pos.y = 0.0099999998f * JonesHud_itemAspectScalePosition;
            JonesHud_pCurInvChangedItem->pos.z = JonesHud_invMenuMinZ; // Changed: from fixed constant -0.102f

            JonesHud_pCurInvChangedItem->startMovePos.x = -0.167f * JonesHud_itemAspectScalePosition;     // Note not really necessary to init x here as it's assigned few lines lower
            JonesHud_pCurInvChangedItem->startMovePos.y = 0.0099999998f * JonesHud_itemAspectScalePosition;
            JonesHud_pCurInvChangedItem->startMovePos.z = JonesHud_invMenuMinZ; // Changed: from fixed constant -0.102f
            JonesHud_pCurInvChangedItem->pyr            = JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr;

            if ( JonesHud_pCurInvChangedItem->id >= JONESHUD_MENU_TREASURE_COINS_GOLD
                && JonesHud_pCurInvChangedItem->id <= JONESHUD_MENU_TREASURE_CASHBOX )
            {
                // Found Treasure
                jonesInventory_AdvanceFoundTreasuresStatistics();
                ++JonesHud_numFoundTreasures;
            }

            if ( sithPlayer_g_pLocalPlayerThing->pInSector
                && (sithPlayer_g_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0
                || JonesHud_bIMPState )
            {
                JonesHud_pCurInvChangedItem->pos.x = 0.1026f * JonesHud_itemAspectScalePosition; // Changed: Fix current position by multiplying it with aspect scale
                JonesHud_pCurInvChangedItem->startMovePos.x = 0.1026f * JonesHud_itemAspectScalePosition; // Changed: Fix start position by multiplying with aspect scale
            }
            else
            {
                JonesHud_pCurInvChangedItem->pos.x = 0.15260001f * JonesHud_itemAspectScalePosition; // Changed: Fix current position by multiplying it with aspect scale
                JonesHud_pCurInvChangedItem->startMovePos.x = 0.15260001f * JonesHud_itemAspectScalePosition; // Changed: Fix start position by multiplying with aspect scale
            }

            if ( JonesHud_pCurInvChangedItem->id >= JONESHUD_MENU_TREASURE_COINS_GOLD
                && JonesHud_pCurInvChangedItem->id <= JONESHUD_MENU_TREASURE_CASHBOX )
            {
                if ( JonesHud_aSoundFxHandles[12] )// inv_treasure.wav
                {
                    JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[12], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                }
            }
            else if ( JonesHud_aSoundFxHandles[5] )// mus_find.wav
            {
                JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[5], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
            }

            float moveRate = fabsf(JonesHud_pCurInvChangedItem->pos.x - -0.15800001f);// -0.15800001f = endMOvePos.x
            JonesHud_StartItemTranslation(JonesHud_pCurInvChangedItem, JonesHud_msecItemChangedShowDuration, moveRate, /*moveUp=*/0);
        }
    }
}

void JonesHud_RenderInventoryItemChange(void)
{
    // Rotate icon for the duration time
    if ( !JonesHud_HasTimeElapsed(JonesHud_pCurInvChangedItem->msecMoveDuration, JonesHud_pCurInvChangedItem->msecMoveEndTime, JonesHud_msecTime) )
    {
        JonesHud_pCurInvChangedItem->pyr.yaw += (float)JonesHud_msecDeltaTime * 16.0f / 50.0f;

    draw_icon:
        JonesHud_RenderChangedItem(JonesHud_pCurInvChangedItem, 1.0f);
        return;
    }

    // Rotate item to final orientation
    if ( (JonesHud_pCurInvChangedItem->flags & 0x40) == 0 )
    {
        if ( JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr.yaw == JonesHud_pCurInvChangedItem->pyr.yaw ) // Is it at final orientation
        {
            JonesHud_pCurInvChangedItem->flags |= 0x40u;
            JonesHud_pCurInvChangedItem->msecMoveDelta = 0;
        }
        else
        {
            float fyaw = fmodf(JonesHud_pCurInvChangedItem->pyr.yaw, 360.0f);
            float dyaw = fyaw - JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr.yaw;
            if ( dyaw < 0.0f )
            {
                dyaw = dyaw + 360.0f;
            }

            if ( dyaw + 20.0f < 360.0f )
            {
                if ( dyaw > 0.0f )
                {
                    // Fixed: Added bounded yaw to 360 degrees (fyaw) of JonesHud_pCurInvChangedItem to prevent infinite loop.
                    // The original code was unbounded, by adding JonesHud_pCurInvChangedItem->pyr.yaw, which could cause an infinite loop if the yaw value
                    // exceeded 360 degrees. This change ensures that the yaw value is always within the
                    // range of 0 to 360 degrees.
                    JonesHud_pCurInvChangedItem->pyr.yaw = (360.0f - dyaw) / 2.0f + fyaw;
                }
            }
            else
            {
                JonesHud_pCurInvChangedItem->pyr.yaw       = JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr.yaw;
                JonesHud_pCurInvChangedItem->msecMoveDelta = 0;

                /*  flags = JonesHud_pCurInvChangedItem->flags;
                  (flags & 0xFF) = flags | 0x40;
                  JonesHud_pCurInvChangedItem->flags = flags;*/
                JonesHud_pCurInvChangedItem->flags |= 0x40;
            }
        }

        goto draw_icon;
    }

    // Item finished rotating, now translate item

    if ( JonesHud_pCurInvChangedItem->endMovePos.x == JonesHud_pCurInvChangedItem->pos.x )
    {
        // Item translated to the end position
        if ( !JonesHud_bMenuEnabled )
        {
            JonesHud_healthIndAlpha = 1.0f;
            JonesHud_bFadeHealthHUD = 1;
        }

        JonesHud_FreeMenuItem(JonesHud_pCurInvChangedItem);
        JonesHud_pCurInvChangedItem = NULL;
        return;
    }

    JonesHud_pCurInvChangedItem->msecMoveDelta += JonesHud_msecDeltaTime;
    JonesHud_pCurInvChangedItem->msecMoveDelta *= JonesHud_itemChangedSlideSpeedRate;


    if ( JonesHud_pCurInvChangedItem->startMovePos.x == JonesHud_pCurInvChangedItem->pos.x )
    {
        // Begin translation
        if ( JonesHud_aSoundFxHandles[6] ) // inv_slide_to.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[6], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        rdVector_Lerp3(&JonesHud_pCurInvChangedItem->pos,
            &JonesHud_pCurInvChangedItem->startMovePos,
            &JonesHud_pCurInvChangedItem->endMovePos,
            (float)JonesHud_pCurInvChangedItem->msecMoveDelta / JonesHud_itemChangedSlideEaseInRate
        );
    }
    else
    {
        rdVector_Lerp3(&JonesHud_pCurInvChangedItem->pos,
            &JonesHud_pCurInvChangedItem->startMovePos,
            &JonesHud_pCurInvChangedItem->endMovePos,
            (float)JonesHud_pCurInvChangedItem->msecMoveDelta / JonesHud_itemChangedSlideEaseInRate
        );

        if ( JonesHud_pCurInvChangedItem->pos.x < (float)JonesHud_pCurInvChangedItem->endMovePos.x )
        {
            JonesHud_pCurInvChangedItem->pos.x = JonesHud_pCurInvChangedItem->endMovePos.x;
        }
    }

    if ( JonesHud_pCurInvChangedItem->pos.x == JonesHud_pCurInvChangedItem->endMovePos.x )
    {
        JonesHud_RenderChangedItem(JonesHud_pCurInvChangedItem, 0.0f);
    }
    else
    {
        float dx  = JonesHud_pCurInvChangedItem->endMovePos.x - JonesHud_pCurInvChangedItem->pos.x;
        float desx = JonesHud_pCurInvChangedItem->endMovePos.x - JonesHud_pCurInvChangedItem->startMovePos.x;
        JonesHud_RenderChangedItem(JonesHud_pCurInvChangedItem, dx / desx);
    }

}

void J3DAPI JonesHud_RenderChangedItem(const JonesHudMenuItem* pItem, float scale)
{
    float v5;
    float v6;
    float v7;

    if ( !pItem )
    {
        return;
    }

    rdMatrix34 placement = rdroid_g_identMatrix34;
    //memcpy(&placement, &rdroid_g_identMatrix34, sizeof(placement));

    rdVector3 svec;
    svec.z = 0.018204151f / pItem->prdIcon->data.pModel3->size;// 0.018204151f - 1 / 54.9325261f
    svec.y = svec.z;
    svec.x = svec.z;
    rdMatrix_PostScale34(&placement, &svec);
    rdMatrix_PostScale34(&placement, &JonesHud_vecSelectedMenuItemScale);

    svec.z = JonesHud_aDfltMenuItemOrients[pItem->id].scale * JonesHud_itemAspectScaleSize; // Added: Multiply scale by aspect ratio scale
    svec.y = svec.z;
    svec.x = svec.z;
    rdMatrix_PostScale34(&placement, &svec);

    v7 = 0.018204151f * 1.25f * svec.x;
    if ( v7 > 0.0154452f )
    {
        v5 = (v7 - 0.0154452f) * scale;
        v6 = (v5 + 0.0154452f) / v7;

        rdVector3 vec;
        vec.z = v6;
        vec.y = v6;
        vec.x = v6;

        v7 = v7 * v6; // ??
        rdMatrix_PostScale34(&placement, &vec);
    }

    // RPY rotate
    rdVector3 tvec = { 0 };
    tvec.roll = pItem->pyr.roll;
    rdMatrix_PostRotate34(&placement, &tvec);

    STD_ZEROMEM(&tvec, sizeof(tvec));
    tvec.pitch = pItem->pyr.pitch;
    rdMatrix_PostRotate34(&placement, &tvec);

    STD_ZEROMEM(&tvec, sizeof(tvec));
    tvec.yaw = pItem->pyr.yaw;
    rdMatrix_PostRotate34(&placement, &tvec);

    tvec = pItem->pos;
    tvec.x -= (svec.x - 1.0f) * 0.75f * 0.0242722f;

    placement.dvec.x = tvec.x;
    placement.dvec.y = tvec.y;
    placement.dvec.z = tvec.z;
    rdModel3_Draw(pItem->prdIcon, &placement);
}

int JonesHud_ShowLevelCompleted(void)
{
    int elapsedTime        = 0;
    int iqPoints           = 0;
    int foundTreasureValue = 0;
    int totalTrasureValue  = 0;
    int numFoundTreasures  = 0;

    SithGameStatistics* pStatistics = sithGamesave_GetGameStatistics();
    if ( pStatistics && sithGamesave_LockGameStatistics() )
    {
        elapsedTime = pStatistics->aLevelStatistic[pStatistics->curLevelNum - 1].elapsedTime;
        iqPoints = pStatistics->totalIQPoints - JonesHud_levelStartIQPoints;
        numFoundTreasures = pStatistics->aLevelStatistic[pStatistics->curLevelNum - 1].numFoundTreasures;

        float amount = sithInventory_GetInventory(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID); // Changed: To use sithInventory_GetInventory instead of manually do what function does
        totalTrasureValue = (int32_t)amount;
        foundTreasureValue = totalTrasureValue - JonesHud_foundTreasureValue;
        sithGamesave_UnlockGameStatistics();
    }

    int balance = (int)sithInventory_GetInventory(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID); // Changed: To use sithInventory_GetInventory instead of manually do what function does

    int aItemsState[14] = { 0 }; // Added: init to 0
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_aStoreItems); ++i )
    {
        if ( JonesHud_aStoreItems[i].menuID == JONESHUD_MENU_INVITEM_BONUSMAP
            && !sithPlayer_g_bBonusMapBought // TODO: in debug version JonesHud_bBonusMapBought is used
            && JonesMain_GetCurrentLevelNum() != JONESLEVEL_BONUSLEVELNUM
            && JonesMain_GetCurrentLevelNum() != 10
            && JonesMain_GetCurrentLevelNum() != 15
            || JonesHud_aStoreItems[i].menuID >= JONESHUD_MENU_HEALTH_SMALL
            && JonesHud_aStoreItems[i].menuID <= JONESHUD_MENU_HEALTH_POISONKIT )
        {
            float amount = sithInventory_GetInventory(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[JonesHud_aStoreItems[i].menuID]->inventoryID); // Changed: To use sithInventory_GetInventory instead of manually do what function does
            aItemsState[i] = (16 * (int32_t)amount) | (JonesHud_aStoreItems[i].menuID << 16) | 1;
        }
        else
        {
            int menuID = JonesHud_aStoreItems[i].menuID;
            bool bItemAvailable = false;
            if ( JonesHud_apMenuItems[menuID] )
            {
                if ( (sithInventory_g_aTypes[JonesHud_apMenuItems[menuID]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
                    ? sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[menuID]->inventoryID].status & SITHINVENTORY_ITEM_FOUND
                    : 0 )
                {
                    bItemAvailable = true;
                }
            }

            if ( bItemAvailable )
            {
                float amount = sithInventory_GetInventory(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[JonesHud_aStoreItems[i].menuID]->inventoryID); // Changed: To use sithInventory_GetInventory instead of manually do what function does
                aItemsState[i] = bItemAvailable | (16 * (int32_t)amount) | (menuID << 16);
            }
            else
            {
                aItemsState[i] = menuID << 16;
            }
        }
    }

    int bBoughtBounusMap = 0;
    if ( jonesConfig_ShowLevelCompletedDialog(
        stdWin95_GetWindow(),
        &balance,
        aItemsState,
        11,
        elapsedTime,
        iqPoints,
        numFoundTreasures,
        foundTreasureValue,
        totalTrasureValue) == 1 )
    {
        for ( size_t i = 0; i < STD_ARRAYLEN(aItemsState); ++i )
        {
            int menuID = (aItemsState[i] & ~0xFFFFu) >> 16;
            if ( (uint16_t)aItemsState[i] )
            {
                if ( menuID == JONESHUD_MENU_INVITEM_BONUSMAP )
                {
                    jonesConfig_UpdateCurrentLevelNum();
                    bBoughtBounusMap = 1;
                    JonesHud_bBonusMapBought = 1;
                }
                else if ( JonesHud_apMenuItems[menuID] )
                {
                    float amount = (float)(JonesHud_aStoreItems[i].unitsPerItem * (uint16_t)aItemsState[i]);
                    sithInventory_ChangeInventory(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[menuID]->inventoryID, amount);
                    sithInventory_SetInventoryAvailable(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[menuID]->inventoryID, 1);
                }
            }
        }

        sithInventory_SetInventory(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID, (float)balance);
    }

    SithGameStatistics* pGameStatistics = sithGamesave_GetGameStatistics();
    if ( !pGameStatistics || !sithGamesave_LockGameStatistics() )
    {
        return bBoughtBounusMap;
    }

    pGameStatistics->foundTreasureValue = (int32_t)sithInventory_GetInventory(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID); // Changed: To use sithInventory_GetInventory instead of manually do what function does
    pGameStatistics->numFoundTreasures  = 0;

    sithGamesave_UnlockGameStatistics();
    return bBoughtBounusMap;
}

HANDLE J3DAPI JonesHud_OpenHelp(HANDLE process)
{
    J3D_UNUSED(process);

    wchar_t* pwString = sithString_GetString("SITHSTRING_HELPFILE");
    HANDLE hProcess = NULL;
    if ( !pwString )
    {
        return hProcess;
    }

    char* pFilename = stdUtil_ToAString(pwString);
    char aPath[128] = { 0 };
    stdConfig_GetString(SITH_CFG_INSTALLPATH, aPath, sizeof(aPath), "");
    if ( pFilename && strlen(aPath) )
    {
        SHELLEXECUTEINFOA execInfo = { 0 };
        execInfo.cbSize      = 60;
        execInfo.fMask       = 80;
        execInfo.lpVerb      = "open";
        execInfo.lpFile      = pFilename;
        execInfo.lpDirectory = aPath;
        execInfo.nShow       = 1;
        ShellExecuteExA(&execInfo);
        hProcess = execInfo.hProcess;
    }

    if ( pFilename )
    {
        STDFREE(pFilename);
    }

    return hProcess;
}

void J3DAPI JonesHud_ShowGameOverDialog(int bPlayDiedMusic)
{
    HWND hwnd;
    int btnId;
    const char* pErrorText;
    int hSnd;
    tSoundChannelHandle hSndChannel;
    char aFilename[JONESCONFIG_GAMESAVE_FILEPATHSIZE] = { 0 }; // Altered: Init to null OG: memset to zero few lines lower

    hSndChannel = 0;
    hSnd = 0;
    sithSoundMixer_StopAll();

    if ( bPlayDiedMusic )
    {
        hSnd = JonesHud_aSoundFxHandles[11];    // mus_gen_indydies1.wav
    }

    hwnd = stdWin95_GetWindow();
    btnId = jonesConfig_ShowGameOverDialog(hwnd, aFilename, hSnd, &hSndChannel);
    if ( btnId < 1177 || btnId > 1178 )         // if not buttons restart or load game were clicked
    {
        if ( (int)hSndChannel > 0 )
        {
            sithSoundMixer_StopSound(hSndChannel);
        }
    }
    else
    {
        if ( (int)hSndChannel > 0 )
        {
            sithSoundMixer_StopSound(JonesHud_hCurSndChannel);
        }

        if ( strlen(aFilename) > 0 )
        {
            JonesHud_bRestoreGameStatistics = 1;
            sithSoundMixer_StopAll();
            if ( sithCamera_g_pCurCamera )
            {
                sithCamera_ResetAllCameras();
            }

            sithGamesave_Restore(aFilename, 1);

            if ( !JonesMain_ProcessGamesaveState() )
            {
                JonesHud_hudState       = 0;
                jonesCog_g_bMenuVisible = 0;

                sithGamesave_CloseRestore();
                JonesMain_ResumeGame();
                JonesReticle_Resume();
                return;
            }

            pErrorText = jonesString_GetString("JONES_STR_CTRLERROR");
            if ( pErrorText )
            {
                JonesMain_LogErrorToFile(pErrorText);
            }
        }
    }

    JonesMain_CloseWindow();
}

float JonesHud_GetHealthBarAlpha(void)
{
    return JonesHud_healthIndAlpha;
}

void J3DAPI JonesHud_SetHealthBarAlpha(float a1)
{
    JonesHud_healthIndAlpha = a1;
}

void J3DAPI JonesHud_CutsceneStart(int a1)
{
    JonesHud_bCutsceneStart = a1;
}

int J3DAPI JonesHud_EnableInterface(int bEnable)
{
    int result;

    result = bEnable;
    JonesHud_bInterfaceEnabled = bEnable;
    return result;
}

void JonesHud_InitializeGameStatistics(void)
{
    float amount;
    SithGameStatistics* pStatistics;

    pStatistics = sithGamesave_GetGameStatistics();
    if ( JonesHud_bRestoreGameStatistics )
    {
        if ( pStatistics && sithGamesave_LockGameStatistics() )
        {
            JonesHud_levelStartIQPoints = pStatistics->aLevelStatistic[pStatistics->curLevelNum].levelStartIQPoints;
            JonesHud_foundTreasureValue = pStatistics->foundTreasureValue;
            JonesHud_numFoundTreasures = pStatistics->numFoundTreasures;
            sithGamesave_UnlockGameStatistics();
        }

        JonesHud_bRestoreGameStatistics = 0;
    }
    else
    {
        JonesHud_numFoundTreasures = 0;
        if ( (sithInventory_g_aTypes[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
        {
            amount = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].amount;
        }
        else
        {
            amount = 0.0f;
        }

        JonesHud_foundTreasureValue = (int32_t)amount;
        if ( pStatistics && sithGamesave_LockGameStatistics() )
        {
            if ( pStatistics->curLevelNum )
            {
                JonesHud_levelStartIQPoints = pStatistics->totalIQPoints;
            }
            else
            {
                JonesHud_levelStartIQPoints = 150;
            }

            sithGamesave_UnlockGameStatistics();
        }
    }

    jonesInventory_ResetStatisticsGameTime();
}

void JonesHud_RestoreGameStatistics(void)
{
    JonesHud_bRestoreGameStatistics = 1;
}

void JonesHud_RestoreTreasuresStatistics(void)
{
    SithGameStatistics* pStatistics;

    pStatistics = sithGamesave_GetGameStatistics();
    if ( pStatistics )
    {
        if ( sithGamesave_LockGameStatistics() )
        {
            pStatistics->foundTreasureValue = JonesHud_foundTreasureValue;
            pStatistics->numFoundTreasures = JonesHud_numFoundTreasures;
            sithGamesave_UnlockGameStatistics();
        }
    }
}

int J3DAPI JonesHud_DrawCredits(int bEndCredits, tSoundChannelHandle hSndChannel)
{
    uint32_t msecCurTime = stdPlatform_GetTimeMsec();

    float lineScalar = rdFont_GetNormY((float)JonesHud_creditsCanvasHeight);
    if ( lineScalar < 1.0f )
    {
        lineScalar = 1.0f;
    }

    // Update credits
    if ( !bEndCredits && !JonesHud_bSkipUpdateCredits )
    {
        if ( JonesHud_msecCreditsElapsedTime == 0 ) // initial init.
        {
            JonesHud_bEndingCredits       = false;
            JonesHud_msecCreditsFadeStart = 0;
            JonesHud_creditTextHeight     = 0.0f;
            JonesHud_creditsCurMatIdx     = 0;
            stdDisplay_GetBackBufferSize(&JonesHud_creditsCanvasWidth, &JonesHud_creditsCanvasHeight);

            JonesHud_creditsSomeHeightRatio = (float)(int)JonesHud_creditsCanvasHeight / 8000.0f;
            JonesHud_creditsAspectRatio     = ((float)JonesHud_creditsCanvasHeight * (4.0f / 3.0f)) / (float)JonesHud_creditsCanvasWidth;
            JonesHud_creditsIconPosOffset   = 48.0f / JonesHud_creditsAspectRatio;

            JonesHud_pCreditsFont1 = rdFont_Load("mat\\jonesComic Sans MS14.gcf");
            JonesHud_pCreditsFont2 = rdFont_Load("mat\\jonesCalisto MT20.gcf");
            JonesHud_creditsCurCelNum = 0;

            STD_ZEROMEM(JonesHud_apCreditsMats, sizeof(JonesHud_apCreditsMats));

            for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_aCredits); ++i )
            {
                int v34 = JonesHud_aCredits[i].flags & 0x08;
                if ( (v34 ^ JonesHud_aCredits[i].flags) == 0x04 )
                {
                    if ( v34 ) // If icon
                    {
                        if ( !JonesHud_creditsCurMatIdx )
                        {
                            JonesHud_creditsCurMatIdx = i;
                            JonesHud_creditTextHeight =
                                ((float)JonesHud_pCreditsFont2->lineSpacing
                                    + 48.0f / JonesHud_creditsAspectRatio
                                    + (float)JonesHud_pCreditsFont1->lineSpacing
                                    + (float)(2 * JonesHud_pCreditsFont1->fontSize)) * lineScalar;
                        }

                        JonesHud_apCreditsMats[i] = (rdMaterial*)STDMALLOC(sizeof(rdMaterial));
                        rdMaterial* pMat = JonesHud_apCreditsMats[i];
                        if ( pMat )
                        {
                            if ( rdMaterial_LoadEntry(JonesHud_aCredits[i].aText, pMat) )
                            {
                                JonesHud_aCredits[i].flags &= ~0x08;
                                rdMaterial_FreeEntry(pMat);
                                pMat = NULL;
                            }
                        }
                    }
                    else
                    {
                        JonesHud_apCreditsMats[i] = rdMaterial_Load(JonesHud_aCredits[i].aText);
                    }
                }
            }

            STD_ZEROMEM(JonesHud_aCreditsCurPosY, sizeof(JonesHud_aCreditsCurPosY));

            JonesHud_creditsCurIdx      = 0;
            JonesHud_creditsCurEndIdx   = 0;
            JonesHud_aCreditsCurPosY[0] = (float)(JonesHud_creditsCanvasHeight + 64); // init position, since 64 is the size of mat it should probably be scaled

            if ( JonesHud_pCreditsFont1 && JonesHud_pCreditsFont2 )
            {
                JonesHud_msecCreditsElapsedTime = stdPlatform_GetTimeMsec();
                return 0;
            }
            else
            {
                JonesHud_msecCreditsElapsedTime = 0;
                JonesHud_bSkipUpdateCredits     = false;
                if ( JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] )
                {
                    rdMaterial_FreeEntry(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]);
                    stdMemory_Free(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]);
                    JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] = 0;
                }

                if ( JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] )
                {
                    rdMaterial_FreeEntry(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1]);
                    stdMemory_Free(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1]);
                    JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] = 0;
                }

                if ( JonesHud_pCreditsFont1 )
                {
                    rdFont_Free(JonesHud_pCreditsFont1);
                    JonesHud_pCreditsFont1 = NULL;
                }

                if ( !JonesHud_pCreditsFont2 )
                {
                    return 1;
                }

                rdFont_Free(JonesHud_pCreditsFont2);
                JonesHud_pCreditsFont2 = NULL;
                return 1;
            }
        } // initial initi.

        //v31 = (float)(msecCurTime - JonesHud_msecCreditsElapsedTime) * JonesHud_creditsSomeHeightRatio; // Unused

        // Update credits movement progress and sound fx
        for ( size_t j = JonesHud_creditsCurIdx; j < STD_ARRAYLEN(JonesHud_aCredits) && j <= JonesHud_creditsCurEndIdx; ++j )
        {
            int v30 = JonesHud_aCredits[JonesHud_creditsCurIdx].flags & 0x08;
            JonesHud_aCreditsCurPosY[j] -= lineScalar * JONESHUD_CREDITS_SPEEDFACTOR; // Here we move text up

            if ( j >= JonesHud_creditsCurMatIdx
                && JonesHud_creditsCurMatIdx > 0
                && JonesHud_aCreditsCurPosY[JonesHud_creditsCurMatIdx] > 0.0f
                && JonesHud_aCreditsCurPosY[JonesHud_creditsCurMatIdx + 1] > 0.0f
                && ((float)(int)JonesHud_creditsCanvasHeight - JonesHud_creditTextHeight) / 2.0f >= JonesHud_aCreditsCurPosY[JonesHud_creditsCurMatIdx] )
            {
                // At the end credits, stop LEC logo and copy right at the center
                switch ( j - JonesHud_creditsCurMatIdx )
                {
                    case 0:
                    case 1: // LEC icon
                        JonesHud_aCreditsCurPosY[j] = ((float)(int)JonesHud_creditsCanvasHeight - JonesHud_creditTextHeight) / 2.0f;
                        break;

                    case 2: //LEC  text
                        JonesHud_aCreditsCurPosY[j] = ((float)JonesHud_pCreditsFont1->lineSpacing + 48.0f / JonesHud_creditsAspectRatio) * lineScalar + JonesHud_aCreditsCurPosY[j - 1]; // Fixed: position by applying lineScalar and aspect ration
                        break;

                    case 3: // Copy right text
                        JonesHud_aCreditsCurPosY[j] = (float)JonesHud_pCreditsFont1->lineSpacing * lineScalar + JonesHud_aCreditsCurPosY[j - 1];
                        break;

                    default:
                        break;
                }
            }

            if ( j == JonesHud_creditsCurIdx )
            {
                switch ( v30 ^ JonesHud_aCredits[JonesHud_creditsCurIdx].flags )
                {
                    case 0x01:
                        if ( (double)-(JonesHud_pCreditsFont2->fontSize * lineScalar) > JonesHud_aCreditsCurPosY[JonesHud_creditsCurIdx] )
                        {
                            ++JonesHud_creditsCurIdx;
                        }

                        break;

                    case 0x02:
                    case 0x10:
                        if ( (double)-(JonesHud_pCreditsFont1->fontSize * lineScalar) > JonesHud_aCreditsCurPosY[JonesHud_creditsCurIdx] )
                        {
                            ++JonesHud_creditsCurIdx;
                        }

                        break;

                    case 0x04: // icon image 
                        if ( JonesHud_aCreditsCurPosY[JonesHud_creditsCurIdx] < (-64.0f * JonesHud_widthAspectRatioScale) )
                        {
                            ++JonesHud_creditsCurIdx;
                        }

                        if ( JonesHud_creditsCurIdx == JonesHud_creditsCurMatIdx && JonesHud_creditsCurMatIdx > 0 )
                        {
                            if ( JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]
                                && JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] )
                            {

                                if ( JonesHud_msecCreditsFadeStart == 0 )
                                {
                                    JonesHud_msecCreditsFadeStart = msecCurTime;
                                }

                                int prevCelNum = JonesHud_creditsCurCelNum;
                                JonesHud_creditsCurCelNum = (msecCurTime - JonesHud_msecCreditsFadeStart) * JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]->numCels / 1000;

                                if ( prevCelNum == 12 && JonesHud_creditsCurCelNum == 13 )// gg_a.mat whip fire cell 12 & 13
                                {
                                    tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(3));// gen_whip_fire.wav
                                    if ( hSnd )
                                    {
                                        JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                                    }
                                }

                                if ( JonesHud_creditsCurCelNum == JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]->numCels
                                    && !JonesHud_bSkipUpdateCredits )
                                {
                                    JonesHud_bSkipUpdateCredits   = true;
                                    JonesHud_msecCreditsFadeStart = msecCurTime;
                                }
                            }
                            else
                            {
                                JonesHud_bSkipUpdateCredits = true;
                            }
                        }

                        break;

                    default:
                        continue;
                }
            }
        }
    } // update credits

    // End credits?
    if ( bEndCredits && !JonesHud_bEndingCredits )
    {
        if ( hSndChannel )
        {
            sithSoundMixer_FadeVolume(hSndChannel, 0.0f, 2.0f); // Changed: Changed to 2 sec, was 500 sec
        }

        JonesHud_msecCreditsFadeStart = msecCurTime;
        JonesHud_bEndingCredits       = true;
    }
    else if ( JonesHud_bSkipUpdateCredits // At credits end
        && !JonesHud_bEndingCredits
        && (!JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]
            || !JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1]
            || JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]
            && JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1]
            && (float)(msecCurTime - JonesHud_msecCreditsFadeStart) >= 1000.0f) )
    {
        if ( hSndChannel )
        {
            sithSoundMixer_FadeVolume(hSndChannel, 0.0f, 2.0f); // Changed: Changed to 2 sec, was 500 sec
        }

        JonesHud_msecCreditsFadeStart = msecCurTime;
        JonesHud_bEndingCredits       = true;
    }

    float fontAlpha = 1.0f;
    if ( JonesHud_bEndingCredits )
    {
        fontAlpha = 1.0f - (float)(msecCurTime - JonesHud_msecCreditsFadeStart) / 2000.0f; // Changed: Changed fadeout to 2 sec from 1 sec
        if ( fontAlpha <= 0.0f && (Sound_GetChannelFlags(hSndChannel) & SOUND_CHANNEL_PLAYING) == 0 )
        {
            // Finished fade out

            JonesHud_msecCreditsElapsedTime = 0;
            JonesHud_bSkipUpdateCredits     = false;

            fontAlpha = 0.0f;
            rdFont_Free(JonesHud_pCreditsFont1);
            rdFont_Free(JonesHud_pCreditsFont2);
            JonesHud_pCreditsFont1 = NULL;
            JonesHud_pCreditsFont2 = NULL;

            if ( JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] )
            {
                rdMaterial_FreeEntry(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]);
                stdMemory_Free(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]);
                JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] = NULL;
            }

            if ( !JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] )
            {
                return 1;
            }

            rdMaterial_FreeEntry(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1]);
            stdMemory_Free(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1]);
            JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] = NULL;
            return 1;
        }
    }

    // Credit draw section

    // Fixed: Changed order so the i is first compared to len and JonesHud_creditsCurEndIdx  before indexing into array
    for ( size_t i = JonesHud_creditsCurIdx;
        i < STD_ARRAYLEN(JonesHud_aCreditsCurPosY)
        && i <= JonesHud_creditsCurEndIdx
        && ((double)(JonesHud_creditsCanvasHeight + 64) >= JonesHud_aCreditsCurPosY[i]); ++i )
    {
        int v27 = JonesHud_aCredits[i].flags & 0x08;
        switch ( v27 ^ JonesHud_aCredits[i].flags )
        {
            case 1: // Ascii text
            {
                rdFontColor aFontColor;
                for ( size_t k = 0; k < STD_ARRAYLEN(aFontColor); ++k )
                {
                    aFontColor[k] = JonesHud_colorWhite;
                    aFontColor[k].alpha = fontAlpha;
                }
                rdFont_SetFontColor(aFontColor);

                float posY = JonesHud_aCreditsCurPosY[i] / (float)(int)JonesHud_creditsCanvasHeight;
                rdFont_DrawTextLine(JonesHud_aCredits[i].aText, 0.5f, posY, RD_FIXEDPOINT_RHW_SCALE_X1, JonesHud_pCreditsFont2, RDFONT_ALIGNCENTER);

                if ( i + 1 < STD_ARRAYLEN(JonesHud_aCreditsCurPosY)
                    && !bEndCredits
                    && i + 1 > JonesHud_creditsCurEndIdx
                    && JonesHud_aCreditsCurPosY[i + 1] == 0.0f )
                {
                    int v11 = JonesHud_aCredits[i + 1].flags & 0x08 ^ JonesHud_aCredits[i + 1].flags;
                    if ( v11 == 0x04 )
                    {
                        JonesHud_aCreditsCurPosY[i + 1] = ((float)JonesHud_pCreditsFont1->lineSpacing + 48.0f) * lineScalar
                            + JonesHud_aCreditsCurPosY[i];
                    }
                    else
                    {
                        float lineSize;
                        if ( v11 == 0x10 )
                        {
                            lineSize = (float)JonesHud_pCreditsFont2->lineSpacing * 2.25f * lineScalar;
                        }
                        else
                        {
                            lineSize = (float)JonesHud_pCreditsFont2->lineSpacing * 1.25f * lineScalar;
                        }

                        JonesHud_aCreditsCurPosY[i + 1] = lineSize + JonesHud_aCreditsCurPosY[i];
                    }

                    JonesHud_creditsCurEndIdx = i + 1;
                }

            } break;

            // jones string text or ascii text
            case 2:    // colored
            case 0x10: // white color
            {
                rdFontColor textColor;
                size_t m;
                for ( m = 0; m < STD_ARRAYLEN(textColor); ++m )
                {
                    if ( JonesHud_aCredits[i].flags == 0x10 )
                    {
                        textColor[m] = JonesHud_colorWhite;
                    }
                    else
                    {
                        textColor[m] = JonesHud_aCreditFontColors[JonesHud_aCredits[i].fontColorNum];
                    }

                    textColor[m].alpha  = fontAlpha;
                }
                rdFont_SetFontColor(textColor);

                const char* pText = jonesString_GetString(JonesHud_aCredits[i].aText);
                float posY = JonesHud_aCreditsCurPosY[i] / (float)(int)JonesHud_creditsCanvasHeight;
                if ( pText )
                {
                    rdFont_DrawTextLine(pText, 0.5f, posY, RD_FIXEDPOINT_RHW_SCALE_X1, JonesHud_pCreditsFont1, RDFONT_ALIGNCENTER);
                }
                else
                {
                    rdFont_DrawTextLine(JonesHud_aCredits[i].aText, 0.5f, posY, RD_FIXEDPOINT_RHW_SCALE_X1, JonesHud_pCreditsFont1, RDFONT_ALIGNCENTER);
                }

                if ( i + 1 < STD_ARRAYLEN(JonesHud_aCreditsCurPosY)
                    && i + 1 > JonesHud_creditsCurEndIdx
                    && !bEndCredits
                    && JonesHud_aCreditsCurPosY[i + 1] == 0.0f )
                {
                    if ( JonesHud_aCredits[i].flags == 0x10 )
                    {
                        JonesHud_aCreditsCurPosY[i + 1] = (float)JonesHud_pCreditsFont2->lineSpacing * lineScalar + JonesHud_aCreditsCurPosY[i];
                    }
                    else
                    {
                        int v18 = JonesHud_aCredits[i + 1].flags & 0x08;
                        switch ( v18 ^ JonesHud_aCredits[i + 1].flags )
                        {
                            case 1:
                            case 4:
                            case 0x10:
                                if ( i == 317 || i == 314 )
                                {
                                    JonesHud_aCreditsCurPosY[i + 1] = (float)JonesHud_pCreditsFont2->lineSpacing * lineScalar + JonesHud_aCreditsCurPosY[i];
                                }
                                else
                                {
                                    JonesHud_aCreditsCurPosY[i + 1] = ((float)JonesHud_pCreditsFont1->lineSpacing + 24.0f) * lineScalar + JonesHud_aCreditsCurPosY[i];
                                }

                                break;

                            case 2:
                                JonesHud_aCreditsCurPosY[i + 1] = (float)JonesHud_pCreditsFont1->lineSpacing * lineScalar + JonesHud_aCreditsCurPosY[i];
                                break;

                            default:
                                break;
                        }
                    }

                    JonesHud_creditsCurEndIdx = i + 1;
                }

            } break;

            case 4: // mat icon
            {
                const rdMaterial* pIconMat = JonesHud_apCreditsMats[i];
                if ( pIconMat )
                {
                    JonesHudRect rect;
                    rect.y = JonesHud_aCreditsCurPosY[i];
                    rect.width  = 64.0f * JonesHud_widthAspectRatioScale;
                    rect.height = 64.0f * JonesHud_widthAspectRatioScale;
                    if ( i == JonesHud_creditsCurMatIdx + 1 && JonesHud_creditsCurMatIdx > 0 )
                    {
                        rect.x = (float)((JonesHud_creditsCanvasWidth - rect.height) / 2 + rect.height);
                    }
                    else
                    {
                        rect.x = (float)((JonesHud_creditsCanvasWidth - rect.height) / 2);
                    }

                    rdVector4 fontColor = JonesHud_aCreditFontColors[JonesHud_aCredits[i].fontColorNum];
                    fontColor.alpha = fontAlpha;

                    JonesHud_Draw(pIconMat, &rect, RD_FIXEDPOINT_RHW_SCALE_X1, RD_FIXEDPOINT_RHW_SCALE_X1, &fontColor, JonesHud_creditsCurCelNum, 1);
                }
                else
                {
                    JonesHud_aCredits[i].flags &= ~0x08;
                }

                if ( i + 1 < STD_ARRAYLEN(JonesHud_aCreditsCurPosY)
                    && i + 1 > JonesHud_creditsCurEndIdx
                    && !bEndCredits
                    && JonesHud_aCreditsCurPosY[i + 1] == 0.0f )
                {
                    if ( i == JonesHud_creditsCurMatIdx && JonesHud_creditsCurMatIdx > 0 )
                    {
                        JonesHud_aCreditsCurPosY[i + 1] = JonesHud_aCreditsCurPosY[i];
                    }
                    else
                    {
                        JonesHud_aCreditsCurPosY[i + 1] = ((float)JonesHud_pCreditsFont1->lineSpacing + JonesHud_creditsIconPosOffset) * lineScalar + JonesHud_aCreditsCurPosY[i];
                    }

                    JonesHud_creditsCurEndIdx = i + 1;
                }

            } break;

            default:
                continue;
        }
    }

    JonesHud_msecCreditsElapsedTime = msecCurTime;
    return 0;
}

const JonesHudMenuItem* J3DAPI JonesHud_GetMenuItem(size_t index)
{
    if ( index < JONESHUD_MAX_MENU_ITEMS )
    {
        return JonesHud_apMenuItems[index];
    }
    return NULL;
}