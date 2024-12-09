#include "JonesHud.h"
#include "JonesHudConstants.h"

#include <j3dcore/j3dhook.h>

#include <Jones3D/Display/jonesConfig.h>
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
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include <w32util/wuRegistry.h>

#include <math.h>
#include <stdbool.h>

#define JONESHUD_CREDITS_SPEEDFACTOR 0.5f // default 1.5

static bool JonesHud_bStartup = false;
static bool JonesHud_bOpen    = false;

static int JonesHud_bCutsceneStart;
static int JonesHud_bInterfaceEnabled;

static int JonesHud_bIMPState;
static int JonesHud_bBonusMapBought; // what's the use of this var

// HUD vars
static int JonesHud_hudState = 0;

static float JonesHud_nearClipPlane  = 0.0f;
static rdCamera* JonesHud_pHudCamera = NULL;
static rdCanvas* JonesHud_pHudCanvas = NULL;
static rdMatrix34 JonesHud_camViewMatrix = { 0 };

static size_t JonesHud_msecTime = 0;
static size_t JonesHud_msecDeltaTime = 0;
static size_t JonesHud_msecGameOverWaitDeltaTime = 0;

// Inventory menu vars
static int JonesHud_bShowMenu;
static int JonesHud_bMenuEnabled;
static int JonesHud_bMapOpen;

static int JonesHud_bRestoreActivated;
static int JonesHud_bRestoreGameStatistics;
static int JonesHud_bItemActivated;
static int JonesHud_bExitActivated;

static char JonesHud_aSlectedNdsFilePath[128];

static const float JonesHud_invMenuDefaultZ = -0.102f;
static float JonesHud_invMenuMinZ           = 0.0f;
static float JonesHud_invMenuMaxZ           = 0.0f;

static float JonesHud_aspectRatioScale       = 0.0f; // Added
static float JonesHud_heightAspectRatioScale = 0.0f;
static float JonesHud_widthAspectRatioScale  = 0.0f;


static int JonesHud_selectedWeaponID            = -1;
static int JonesHud_selectedWeaponMenuItemID    = -1;
static int JonesHud_selectedTreasuresMenuItemID = -1;
static int JonesHud_selectedItemsMenuItemID     = -1;
static int JonesHud_selectedSystemMenuItemID    = -1;
static int JonesHud_rootMenuItemId              = -1;

static int JonesHud_numFoundTreasures;
static int JonesHud_foundTreasureValue;
static int JonesHud_levelStartIQPoints;

static rdFont* JonesHud_pMenuFont = NULL;

static float JonesHud_healthIndScale    = 0.0;
static float JonesHud_enduranceIndScale = 0.0; // Added
static float JonesHud_aCosSinTable[24][2]= { 0 };
static float JonesHud_aCosSinTableScaled[24][2] = { 0 };

static int JonesHud_curWhoopsSndFxIdx = -1;

static size_t JonesHud_msecMenuItemMoveDuration = 250u;

static JonesHudMenuItem* JonesHud_apMenuItems[JONESHUD_MAX_MENU_ITEMS] = { NULL };

static tSoundChannelHandle JonesHud_hCurSndChannel;
static tSoundHandle JonesHud_aSoundFxHandles[STD_ARRAYLEN(JonesHud_aSoundFxIdxs)] = { 0 };

static int JonesHud_item_flag_state_55514C;
static JonesHudMenuItem* JonesHud_pMenuItemLinkedList  = NULL;
static JonesHudMenuItem* JonesHud_pCloseMenuItem       = NULL;
static JonesHudMenuItem* JonesHud_pCurSelectedMenuItem = NULL;
static JonesHudMenuItem* JonesHud_pCurInvChangedItem   = NULL;

static rdMaterial* JonesHud_pArrowUp   = NULL;
static rdMaterial* JonesHud_pArrowDown = NULL;

static HANDLE JonesHud_hProcessHelp;

// Keyboard state vars
static int JonesHud_bKeyStateUpdated;
static int JonesHud_curKeyId = -1;
static int JonesHud_msecLastKeyPressTime;
static int JonesHud_aActivateKeyIds[8] = { 0 };

// Health indicator vars
static int JonesHud_bHealthIndFadeSet;
static int JonesHud_bFadeHealthHUD;

static float JonesHud_healthIndAlpha;

static float JonesHud_curHealth;
static float JonesHud_curFadeHealthState;
static size_t JonesHud_msecHealthIndLastFadeUpdate;

static int JonesHud_bFadingHealthHUD;
static float JonesHud_hitOverlayAlphaState;
static float JonesHud_hitOverlayAlpha;

static rdMaterial* JonesHud_pHudBaseMat          = NULL;
static rdMaterial* JonesHud_pHealthOverlay       = NULL;
static rdMaterial* JonesHud_pHealthOverlayHitMat = NULL;
static rdMaterial* JonesHud_pPoisonedOverlayMat  = NULL;

static JonesHudRect JonesHud_healthIndRect;
static rdVector4 JonesHud_healthIndBarPos;

// Endurance indicator vars
static JonesHudRect JonesHud_enduranceRect;
static int JonesHud_msecRaftIndicatorFadeTime;
static float JonesHud_curEnduranceIndAlpha       = 0.2f;
static float JonesHud_curEnduranceState          = -1.0f;
static rdVector4 JonesHud_enduranceIndBarPos     = { 0 };
static rdMaterial* JonesHud_pEnduranceOverlayMat = NULL;

// End credit vars
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

static int JonesHud_curCelNum;
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

void JonesHud_InstallHooks(void)
{
    J3D_HOOKFUNC(JonesHud_Startup);
    J3D_HOOKFUNC(JonesHud_Shutdown);
    J3D_HOOKFUNC(JonesHud_Open);
    J3D_HOOKFUNC(JonesHud_Close);
    J3D_HOOKFUNC(JonesHud_ToggleMenu);
    J3D_HOOKFUNC(JonesHud_EnableMenu);
    J3D_HOOKFUNC(JonesHud_IsMenuEnabled);
    J3D_HOOKFUNC(JonesHud_Render);
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
    J3D_HOOKFUNC(JonesHud_RenderFadeHealthIndicator);
    J3D_HOOKFUNC(JonesHud_DrawHealthIndicator);
    J3D_HOOKFUNC(JonesHud_DrawHealthHitOverlay);
    J3D_HOOKFUNC(JonesHud_RenderHealthIndicator);
    J3D_HOOKFUNC(JonesHud_DrawHealthIndicatorBase);
    J3D_HOOKFUNC(JonesHud_DrawHealthIndicatorBar);
    J3D_HOOKFUNC(JonesHud_DrawIndicatorBar);
    J3D_HOOKFUNC(JonesHud_Draw);
    J3D_HOOKFUNC(JonesHud_ProcessMenuItems);
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
    J3D_HOOKFUNC(JonesHud_sub_4198E0);
    J3D_HOOKFUNC(JonesHud_sub_419B50);
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
{

}

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

    memset(JonesHud_aActivateKeyIds, 0, sizeof(JonesHud_aActivateKeyIds)); // Fixed: 0 all elements pf array

    memset(JonesHud_aCosSinTableScaled, 0, sizeof(JonesHud_aCosSinTableScaled));
    memset(JonesHud_aCosSinTable, 0, sizeof(JonesHud_aCosSinTable));

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

    //nullsub_2();
    JonesHud_pHudBaseMat          = NULL;
    JonesHud_pHealthOverlay       = NULL;
    JonesHud_pHealthOverlayHitMat = NULL;
    JonesHud_pPoisonedOverlayMat  = NULL;
    JonesHud_pEnduranceOverlayMat = NULL;

    JonesHud_pArrowUp             = NULL;
    JonesHud_pArrowDown           = NULL;

    JonesHud_pHudCanvas           = NULL;
    JonesHud_pHudCamera           = NULL;
    JonesHud_pMenuFont            = NULL;
    JonesHud_pMenuItemLinkedList  = NULL;
    JonesHud_pCurSelectedMenuItem = NULL;
    JonesHud_pCloseMenuItem       = NULL;
    JonesHud_pCurInvChangedItem   = NULL;

    JonesHud_widthAspectRatioScale    = 0.0f;
    JonesHud_heightAspectRatioScale   = 0.0f;
    JonesHud_hCurSndChannel       = 0;

    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_apMenuItems); ++i ) {
        JonesHud_apMenuItems[i] = NULL;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_aSoundFxHandles); ++i ) {
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

    JonesHud_pHudBaseMat = rdMaterial_Load("hud_health_base.mat");
    if ( !JonesHud_pHudBaseMat )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "hud_health_base.mat");
        goto error;
    }

    JonesHud_pHealthOverlay = rdMaterial_Load("hud_health_overlay.mat");
    if ( !JonesHud_pHealthOverlay )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "hud_health_overlay.mat");
        goto error;
    }

    JonesHud_pHealthOverlayHitMat = rdMaterial_Load("hud_health_overlay_hit.mat");
    if ( !JonesHud_pHealthOverlayHitMat )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "hud_health_overlay_hit.mat");
        goto error;
    }

    JonesHud_pPoisonedOverlayMat = rdMaterial_Load("hud_health_overlay_skull.mat");
    if ( !JonesHud_pPoisonedOverlayMat )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "hud_health_overlay_skull.mat");
        goto error;
    }

    JonesHud_pArrowUp = rdMaterial_Load("gen_4arrow_up.mat");
    if ( !JonesHud_pArrowUp )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "gen_4arrow_up.mat");
        goto error;
    }

    JonesHud_pArrowDown = rdMaterial_Load("gen_4arrow_down.mat");
    if ( !JonesHud_pArrowDown )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "gen_4arrow_down.mat");
        goto error;
    }

    JonesHud_pEnduranceOverlayMat = rdMaterial_Load("hud_breath_overlay.mat");
    if ( !JonesHud_pEnduranceOverlayMat )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "hud_breath_overlay.mat");
        goto error;
    }

    if ( !JonesHud_InitializeMenu() )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load initialize menu items.\n");
        goto error;
    }

    if ( !jonesConfig_Startup() )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't initialize dialog data.\n"); // Fixed typo: 'intialize'
        goto error;
    }

    JonesHud_InitializeMenuSounds();

    // Setup HUD camera
    rdCamera_sub_4C60B0(true);

    JonesHud_pHudCamera = rdCamera_New(10.0f, 0, 1.0f, 100.0f, 1.0f);
    if ( !JonesHud_pHudCamera )
    {
        STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't create Hud camera.\n");
        goto error;
    }

    JonesHud_pHudCanvas = rdCanvas_New(
        0,// 0 marks the rect args and instead the buffer size is used, ie: screen size
        &stdDisplay_g_backBuffer,
        64,// left
        (int)JonesHud_healthIndRect.y, // JonesHud_healthIndRect might not be initialized
        576,// right
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

    rdMatrix_Identity34(&JonesHud_camViewMatrix);
    rdMatrix_PostRotate34(&JonesHud_camViewMatrix, &JonesHud_camViewPyr);
    JonesHud_camViewMatrix.dvec = JonesHud_camViewPos;

    rdCamera_Update(&JonesHud_camViewMatrix);
    JonesHud_nearClipPlane = JonesHud_pHudCamera->pFrustum->nearPlane + 1.0f;// probably unused

    // Restore previous camera
    rdCamera_SetCurrent(pCurCamera);

    rdVector4 hudAmbientColor = { .red = 1.0f, .green = 1.0f, .blue = 1.0f, .alpha = 0.0f };
    rdCamera_SetAmbientLight(JonesHud_pHudCamera, &hudAmbientColor);

    rdCamera_sub_4C60B0(false);

    // Load last required fonts
    JonesHud_bBonusMapBought = 0;
    JonesHud_bIMPState       = 0;
    JonesHud_hudState        = 0;

    JonesHud_pMenuFont = rdFont_Load("mat\\jonesComic Sans MS14.gcf");
    if ( JonesHud_pMenuFont )
    {
        // Success
        JonesHud_bOpen             = true;
        JonesHud_bCutsceneStart    = 0;
        JonesHud_bInterfaceEnabled = 0;
        return 0;
    }

    STD_FORMAT(JonesMain_g_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "mat\\jonesComic Sans MS14.gcf");

error:
    STDLOG_ERROR(JonesMain_g_aErrorBuffer);

    JonesHud_Close();
    JonesHud_bOpen = false;
    return 1;
}

void JonesHud_Close(void)
{
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

    if ( JonesHud_pMenuFont )
    {
        rdFont_Free(JonesHud_pMenuFont);
        JonesHud_pMenuFont = NULL;
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
        if ( (JonesHud_hudState & 1) != 0 ) // If menu already opened then close it
        {
            if ( (JonesHud_hudState & 4) == 0 && (JonesHud_hudState & 8) == 0 && (JonesHud_hudState & 1) != 0 )
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
                sithGamesave_ScreenShot(&stdDisplay_g_backBuffer);
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

void JonesHud_Render(void) // maybe this function should be called something eles??
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

        if ( JonesHud_bCutsceneStart && (JonesHud_hudState & 1) == 0 && !jonesCog_g_bMenuVisible )
        {
            // Menu is closed, start previously started cutscene
            JonesHud_bCutsceneStart = 0;
            jonesCog_StartCutscene(NULL);
        }

        if ( JonesHud_bInterfaceEnabled && (JonesHud_hudState & 1) == 0 && !jonesCog_g_bMenuVisible )
        {
            // Menu is closed show in game HUD
            JonesHud_bInterfaceEnabled = 0;
            jonesCog_EnableInterface(NULL);
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

            sithGamesave_Restore(JonesHud_aSlectedNdsFilePath, 1);
            memset(JonesHud_aSlectedNdsFilePath, 0, sizeof(JonesHud_aSlectedNdsFilePath));
            JonesHud_bRestoreActivated = 0;
        }
        else
        {
            // Render Menu

            if ( JonesHud_bMapOpen && !sithPlayerControls_g_bCutsceneMode && (JonesHud_hudState & 1) == 0 )
            {
                JonesHud_bMapOpen = 0;
                sithOverlayMap_ToggleMap();
            }

            // Clear render cache before switching to rendering the HUD
            rdCache_FlushAlpha();
            rdCache_Flush();

            // Change cur camera to HUD camera
            rdCamera* pCurCam = rdCamera_g_pCurCamera;
            rdCamera_SetCurrent(JonesHud_pHudCamera);
            rdCamera_Update(&JonesHud_camViewMatrix);

            if ( !JonesHud_RenderFadeHealthIndicator(JonesHud_bFadeHealthHUD) )
            {
                if ( !JonesHud_bHealthIndFadeSet || JonesHud_healthIndAlpha >= 0.2f )
                {
                    float maxHealth = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.maxHealth;
                    float healthState = 0.0f;
                    if ( maxHealth > 0.0f )
                    {
                        healthState = (300.0f * sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health / maxHealth);
                    }
                    JonesHud_RenderHealthIndicator(roundf(healthState));
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
                    float impState = sithPlayer_g_impState * 1000.0f / 1800.0f;
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
                if ( (JonesHud_hudState & 1) != 0 ) // Is inventory menu opened?
                {
                    if ( (Sound_GetChannelFlags(JonesHud_hCurSndChannel) & SOUND_CHANNEL_PLAYING) == 0 ) // !playing?
                    {
                        JonesHud_hCurSndChannel = 0;
                    }

                    // Process & render inventory menu items
                    // This is the main logic
                    JonesHud_ProcessMenuItems();
                }
                else if ( jonesCog_g_bMenuVisible )
                {
                    // Closing menu so resume the game here
                    if ( sithOverlayMap_IsMapVisible() )
                    {
                        JonesHud_bMapOpen = 1;
                        sithOverlayMap_ToggleMap();
                    }

                    stdControl_ResetMousePos();
                    JonesMain_PauseGame();
                    JonesHud_MenuOpen();
                }
                else if ( JonesHud_bShowMenu )
                {
                    // Deferred from JonesHud_ToggleMenu when item pickup animation is playing
                    sithGamesave_ScreenShot(&stdDisplay_g_backBuffer);
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

    float wratio = (float)width / RD_REF_WIDTH; // NOTE: fixed screen height 640
    JonesHud_widthAspectRatioScale = wratio;

    float hratio = (float)height / RD_REF_HEIGHT; // NOTE: fixed screen height 480
    JonesHud_heightAspectRatioScale = hratio;

    JonesHud_aspectRatioScale = JonesHud_widthAspectRatioScale / JonesHud_heightAspectRatioScale;

    size_t curTime = stdPlatform_GetTimeMsec();
    int bCanvasUpdated = 0;

    if ( (pWorld->state & SITH_WORLD_STATE_INIT_HUD) != 0 )
    {
        JonesHud_msecMenuItemMoveDuration = 250;
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
        JonesHud_msecDeltaTime = curTime + -1 - JonesHud_msecTime;
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
        JonesHud_pHudCanvas = 0;
        JonesHud_pHudCanvas = rdCanvas_New(
            0, // 0 here makrks that back bufer size should be used and the provided rect sizes are not used
            &stdDisplay_g_backBuffer,
            64,
            (int32_t)JonesHud_healthIndRect.y,
            576,
            (int32_t)(JonesHud_healthIndRect.y + 64.0f)
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
    JonesHud_healthIndAlpha = 1.0f;

    // Fixed: Fixed HUD scaling of indicator by using only JonesHud_widthAspectRatioScale
    // These coords are all in screen size
    JonesHud_healthIndRect.x      = JonesHud_widthAspectRatioScale * 24.0f;
    JonesHud_healthIndRect.y      = (float)height - JonesHud_healthIndRect.x - JonesHud_widthAspectRatioScale * 60.0f; // Changed: Was using JonesHud_heightAspectRatioScale
    JonesHud_healthIndRect.width  = JonesHud_widthAspectRatioScale * 60.0f;
    JonesHud_healthIndRect.height = JonesHud_widthAspectRatioScale * 60.0f; // Changed: Was using JonesHud_heightAspectRatioScale

    JonesHud_healthIndBarPos.x = JonesHud_healthIndRect.x + JonesHud_healthIndRect.width / 2.0f;
    JonesHud_healthIndBarPos.y = JonesHud_healthIndRect.y + JonesHud_healthIndRect.height / 2.0f;
    JonesHud_healthIndBarPos.z = 0.0f; // z & w set later when drawing hud indicator
    JonesHud_healthIndBarPos.w = 0.0f;

    JonesHud_enduranceRect.x      = (float)width - JonesHud_healthIndRect.x - JonesHud_healthIndRect.width;
    JonesHud_enduranceRect.y      = JonesHud_healthIndRect.y;
    JonesHud_enduranceRect.width  = JonesHud_healthIndRect.width;
    JonesHud_enduranceRect.height = JonesHud_healthIndRect.height;

    JonesHud_enduranceIndBarPos.x = JonesHud_enduranceRect.x + JonesHud_enduranceRect.width / 2.0f;
    JonesHud_enduranceIndBarPos.y = JonesHud_healthIndBarPos.y;
    JonesHud_enduranceIndBarPos.z = 0.0f;  // z & w set later when drawing hud indicator
    JonesHud_enduranceIndBarPos.w = 0.0f;

    float hheight = (JonesHud_healthIndRect.height - 2.0f) / 2.0f;
    JonesHud_healthIndScale = hheight - 7.0f * JonesHud_widthAspectRatioScale;

    JonesHud_enduranceIndScale = ((JonesHud_enduranceRect.height - 2.0f) / 2.0f) - 7.0f * JonesHud_widthAspectRatioScale; // Added

    // Unused stuff
    JonesHud_flt_55503C = 16.0f * JonesHud_heightAspectRatioScale;
    JonesHud_flt_555038 = 16.0f * JonesHud_widthAspectRatioScale;
    JonesHud_flt_555034 = -JonesHud_flt_55503C;

    JonesHud_dword_554FDC = 0;

    JonesHud_flt_554FE4 = 16.0f * JonesHud_heightAspectRatioScale;
    JonesHud_flt_554FE0 = 16.0f * JonesHud_widthAspectRatioScale;

    // Fixed: Adjusted inventory menu position for wide screen resolutions. OG: invMenuBottomOffset was set to invMenuDefaultOffset
    float adjustedAspect = ((float)height * RD_REF_APECTRATIO) / (float)width;
    float adjustedZ      = JonesHud_invMenuDefaultZ * adjustedAspect;
    float offset         = -0.05f * (1 - adjustedAspect); // Add a small offset to move it slightly up
    JonesHud_invMenuMinZ = adjustedZ + offset;
}

void JonesHud_MenuOpen(void)
{
    if ( sithPlayer_g_pLocalPlayerThing && (!JonesHud_pMenuItemLinkedList || (JonesHud_hudState & 1) == 0) )
    {
        JonesHud_selectedWeaponMenuItemID = 0;
        for ( size_t i = 0; i <= 11; ++i ) // 11 - num player weapons TODO: make constant or use existing one
        {
            if ( JonesHud_apMenuItems[i]
              && JonesHud_apMenuItems[i]->inventoryID == sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->curWeaponID )
            {
                JonesHud_selectedWeaponMenuItemID = i;
                break;
            }
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

            /* flags = JonesHud_pMenuItemLinkedList->flags;
             (flags & 0xFF) = flags | 0x83;
             JonesHud_pMenuItemLinkedList->flags = flags;*/
            JonesHud_pMenuItemLinkedList->flags |= 0x83;

            //v1 = JonesHud_hudState;
            //(v1 & 0xFF) = JonesHud_hudState | 1; // mark menu opened
            //JonesHud_hudState = v1;
            JonesHud_hudState |= 1; // mark menu opened;

            float moveRate = fabsf(JonesHud_pMenuItemLinkedList->endMovePos.x - JonesHud_pMenuItemLinkedList->startMovePos.x);
            JonesHud_StartItemTranslation(JonesHud_pMenuItemLinkedList, /*msecDuration=*/100, moveRate, 0);
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
          && ((JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 1) != 0
              || (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x20) != 0) )
        {
            //(JonesHud_item_flag_state_55514C & 0xFF) = (1 << i) | JonesHud_item_flag_state_55514C;
            JonesHud_item_flag_state_55514C = (JonesHud_item_flag_state_55514C & ~0xFF) | (1 << i);
            if ( JonesHud_aSoundFxHandles[3] )  // inv_expand.wav
            {
                JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[3], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
            }

            JonesHud_sub_419B50(pItem);
        }

        pItem->flags |= i << 10;
        if ( pItem->nextRightItemId == -1
          || !JonesHud_apMenuItems[pItem->nextRightItemId]
          || JonesHud_apMenuItems[pItem->nextRightItemId]->id == JonesHud_rootMenuItemId
          || (JonesHud_apMenuItems[pItem->nextRightItemId]->flags & 1) == 0 )
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

        /* flags = JonesHud_pCloseMenuItem->flags;
        (flags & 0xFF) = flags & ~0x80;
        JonesHud_pCloseMenuItem->flags = flags;*/
        JonesHud_pCloseMenuItem->flags &= ~0x80;

        rdVector_Copy3(&JonesHud_pCloseMenuItem->startMovePos, &JonesHud_pCloseMenuItem->pos);

        float moveRate = 0.064999998f; // Fixed: Init to 0.064999998f;
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
                    JonesHud_pCloseMenuItem->endMovePos.x = JonesHud_pCloseMenuItem->endMovePos.x - 0.064999998f;
                    moveRate = 0.064999998f;
                }
            }
        }

        moveRate = fabsf(moveRate);
        JonesHud_StartItemTranslation(JonesHud_pCloseMenuItem, 50, moveRate, 0);
    }

    //v1 = JonesHud_hudState;
    //(v1 & 0xFF) = JonesHud_hudState | 4;
    //JonesHud_hudState = v1;
    JonesHud_hudState |= 0x04; // Closing menu
}

void J3DAPI JonesHud_StartItemTranslation(JonesHudMenuItem* pItem, uint32_t msecDuration, float moveRate, int bMoveUp)
{
    // Fixed: Fixed division by 0
    if ( moveRate == 0 ) {
        moveRate = 0.001f;
    }

    float delta;
    if ( bMoveUp ) {
        delta = fabsf(pItem->endMovePos.z - pItem->startMovePos.z) / moveRate;
    }
    else {
        delta = fabsf(pItem->endMovePos.x - pItem->startMovePos.x) / moveRate;
    }

    pItem->msecMoveDuration = (int32_t)((double)msecDuration * delta);
    pItem->msecMoveEndTime  = pItem->msecMoveDuration + JonesHud_msecTime;
    pItem->flags |= 0x01; // enable item rotation anim
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
            JonesHud_curEnduranceIndAlpha = 0.2f;
            JonesHud_curEnduranceState = -1.0f;
        }

        if ( JonesHud_curEnduranceState == enduranceState )
        {
            if ( JonesHud_curEnduranceIndAlpha > 0.2f
              && JonesHud_curEnduranceState == enduranceState
              && JonesHud_HasTimeElapsed(3000u, JonesHud_msecRaftIndicatorFadeTime + 3000, JonesHud_msecTime) )
            {
                JonesHud_curEnduranceIndAlpha -= (float)JonesHud_msecDeltaTime * 0.80000001f / 3000.0f;
                if ( JonesHud_curEnduranceIndAlpha < 0.2f )
                {
                    JonesHud_curEnduranceIndAlpha = 0.2f;
                }
            }
        }
        else
        {
            JonesHud_curEnduranceIndAlpha = 1.0f;
            JonesHud_msecRaftIndicatorFadeTime = JonesHud_msecTime;
        }

        JonesHud_curEnduranceState = enduranceState;
        JonesHud_DrawEnduranceIndicator(enduranceState, JonesHud_curEnduranceIndAlpha);

        rdVector4 color = JonesHud_colorWhite;
        color.alpha     = JonesHud_curEnduranceIndAlpha;

        JonesHud_Draw(
            JonesHud_pEnduranceOverlayMat,
            &JonesHud_enduranceRect,
            /*z=*/RD_FIXEDPOINT_RHW_SCALE, // 0.000030518044f - 1 / 32767 = 1/ (128 * 256 -1) ;  256 could be w
            /*rhw=*/RD_FIXEDPOINT_RHW_SCALE,
            &color,
            /*celNum=*/0,
            /*bAlpha=*/1
        );
    }
    else
    {
        JonesHud_DrawEnduranceIndicator(enduranceState, 1.0f);

        rdVector4 color = JonesHud_colorWhite;
        color.alpha = 1.0f;

        JonesHud_Draw(
            JonesHud_pEnduranceOverlayMat,
            &JonesHud_enduranceRect,
            /*z=*/RD_FIXEDPOINT_RHW_SCALE, // z; 0.000030518044f - 1 / 32767 = 1/ (128 * 256 -1) ;  256 could be w
            /*rhw=*/RD_FIXEDPOINT_RHW_SCALE,
            &color,
            /*celNum=*/0,
            /*bAlpha=*/1
        );
    }
}

void J3DAPI JonesHud_DrawEnduranceIndicator(float state, float alpha)
{
    JonesHudRect rect;
    rect.x = JonesHud_enduranceRect.x;
    rect.y = JonesHud_enduranceRect.y;
    rect.width  = JonesHud_enduranceRect.width;
    rect.height = JonesHud_enduranceRect.height;

    int progress;
    if ( state >= 100.0f )
    {
        progress = 100;
    }
    else
    {
        progress = (int32_t)state % 100;
    }

    float angle = (float)progress * 0.01f * 360.0f;
    rdVector4 arcBarColor = JonesHud_colorBlack;
    arcBarColor.alpha = alpha;

    JonesHud_enduranceIndBarPos.z = 0.000061036088f;// 1 / 16384 = 1 / (64*256) - this will make to draw_icon above base color (yellow, blue, pink)
    JonesHud_enduranceIndBarPos.w = 0.000061036088f;


    rdVector4 barColor = { 0 }; // Added: Init to zero
    if ( angle != 0.0f )
    {
        // Draws indicator bar arc
        JonesHud_DrawIndicatorBar(&JonesHud_enduranceIndBarPos, JonesHud_enduranceIndScale, &arcBarColor, angle, /*bArc=*/1);
    }

    if ( (sithPlayer_g_pLocalPlayerThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
    {
        barColor = JonesHud_colorYellow;
    }
    else if ( sithPlayer_g_pLocalPlayerThing->pInSector
           && (sithPlayer_g_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        rdVector_Copy4(&barColor, &JonesHud_skyBlue);
    }
    else if ( JonesHud_bIMPState )
    {
        // IM parts bar color
        barColor = JonesHud_colorPink;
    }

    barColor.alpha = alpha;

    rect.x = JonesHud_enduranceRect.width / 2.0f - JonesHud_enduranceIndScale + rect.x;
    rect.y = JonesHud_enduranceRect.width / 2.0f - JonesHud_enduranceIndScale + rect.y;
    rect.width = JonesHud_enduranceIndScale * 2.0f;
    rect.height = rect.width;

    JonesHud_Draw(JonesHud_pHudBaseMat, &rect, 0.000091554131f, 0.000091554131f, &barColor, 0, 1);// 0.000091554131f = 1 / 10922.5f - 1 / (42.666051f * 256)
}

void J3DAPI JonesHud_SetFadeHealthHUD(int bShow, int bFade)
{
    JonesHud_bFadeHealthHUD = bFade;
    if ( !bShow && !JonesHud_bFadeHealthHUD )
    {
        JonesHud_healthIndAlpha = 0.0f;
    }
}

int J3DAPI JonesHud_RenderFadeHealthIndicator(int bFade)
{
    if ( bFade )
    {
        JonesHud_bFadingHealthHUD = 1;
        JonesHud_bFadeHealthHUD = 0;
    }

    if ( (JonesHud_hudState & 1) != 0 )
    {
        JonesHud_bFadingHealthHUD = 0;
    }

    if ( JonesHud_msecDeltaTime == 1 )
    {
        JonesHud_curFadeHealthState = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health;
    }
    else
    {
        if ( JonesHud_curFadeHealthState != sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health )
        {
            JonesHud_healthIndAlpha = 1.0f;
            JonesHud_bFadingHealthHUD = 0;
        }

        JonesHud_curFadeHealthState = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health;
    }

    if ( !JonesHud_bFadingHealthHUD )
    {
        return jonesCog_g_bShowHealthHUD == 0;
    }

    // Fade in or fade out health HUD
    float delta = (float)JonesHud_msecDeltaTime * 0.80000001f / 3000.0f;
    if ( jonesCog_g_bShowHealthHUD )
    {
        if ( JonesHud_healthIndAlpha >= 0.2f )
        {
            JonesHud_bFadingHealthHUD = 0;
        }
        else
        {
            JonesHud_healthIndAlpha += delta;
            if ( JonesHud_healthIndAlpha > 0.2f )
            {
                JonesHud_healthIndAlpha = 0.2f;
            }
        }
    }

    else if ( JonesHud_healthIndAlpha <= 0.0f )
    {
        JonesHud_bFadingHealthHUD = 0;
    }
    else
    {
        JonesHud_healthIndAlpha -= delta;
        if ( JonesHud_healthIndAlpha < 0.0f )
        {
            JonesHud_healthIndAlpha = 0.0f;
        }
    }

    if ( JonesHud_healthIndAlpha == 0.0f || JonesHud_healthIndAlpha == 0.200000002980232f )
    {
        JonesHud_bFadingHealthHUD = 0;
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
        color.alpha = 0.25f;
        JonesHud_Draw(
            JonesHud_pPoisonedOverlayMat,
            &JonesHud_healthIndRect,
            RD_FIXEDPOINT_RHW_SCALE,               // z; 0.00003051844f = 1/ 32767 - 1 / (128*256-1)
            RD_FIXEDPOINT_RHW_SCALE,               // rhw
            &color,
            /*celNum=*/0,
            /*bAlpha=*/1
        );
    }

    if ( hitDelta > 0.0f ) {
        JonesHud_DrawHealthHitOverlay(hitDelta, 0.000061036088f, 0.000061036088f);// 0.000061036088f = 1 / 16384 - 1 /(64*256)
    }

    color.alpha = alpha;
    JonesHud_Draw(JonesHud_pHealthOverlay, &JonesHud_healthIndRect, 0.000091554131f, 0.000091554131f, &color, 0, 1);// 0.000091554131f - 1 / 10922.5f - 1 / (42.666015f*256)

    // Draws health bar in a style of a "pie chart", in colors yellow, red or black
    // Then draw_icon base beneath underneath with health bar circle in color green, yellow, red  
    JonesHud_DrawHealthIndicatorBar(health, alpha, 0.00012207218f, 0.00012207218f);// 0.00012207218f - 1 / 8192 = 1/(32*256); 256 could be w
    JonesHud_DrawHealthIndicatorBase(health, alpha, 0.00015259022f, 0.00015259022f);// 0.00015259022f - 1/ 6554 - 1(25.599602f*256)
}

void J3DAPI JonesHud_DrawHealthHitOverlay(float hitDelta, float z, float rhw)
{
    if ( JonesHud_hitOverlayAlphaState == 0.0f && JonesHud_hitOverlayAlpha == 0.0f )
    {
        JonesHud_hitOverlayAlphaState = (hitDelta - (float)((int32_t)hitDelta % 20) + 20.0f) / 300.0f;
    }

    if ( JonesHud_hitOverlayAlpha >= (double)JonesHud_hitOverlayAlphaState )
    {
        JonesHud_hitOverlayAlpha -= (float)JonesHud_msecDeltaTime * JonesHud_hitOverlayAlphaState * 2.0f / 250.0f;
        if ( JonesHud_hitOverlayAlpha <= 0.0f )
        {
            JonesHud_hitOverlayAlpha = 0.0f;
            JonesHud_hitOverlayAlphaState = 0.0f;
            return;
        }
    }
    else
    {
        JonesHud_hitOverlayAlpha += (float)JonesHud_msecDeltaTime * JonesHud_hitOverlayAlphaState * 2.0f / 250.0f;
        if ( JonesHud_hitOverlayAlpha > JonesHud_hitOverlayAlphaState )
        {
            JonesHud_hitOverlayAlpha = JonesHud_hitOverlayAlphaState;
        }
    }

    rdVector4 color = JonesHud_colorWhite;
    color.alpha     = JonesHud_hitOverlayAlpha;
    JonesHud_Draw(JonesHud_pHealthOverlayHitMat, &JonesHud_healthIndRect, z, rhw, &color, 0, 1);
}

void J3DAPI JonesHud_RenderHealthIndicator(float healthState)
{
    if ( sithPlayer_g_pLocalPlayerThing )
    {
        if ( !JonesHud_bHealthIndFadeSet )
        {
            JonesHud_curHealth = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health;
            JonesHud_msecHealthIndLastFadeUpdate = JonesHud_msecTime;
            JonesHud_bHealthIndFadeSet = 1;
        }

        float health = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.health;
        if ( JonesHud_pCurInvChangedItem && JonesHud_pCurInvChangedItem->pos.x <= -0.13f
          || (JonesHud_hudState & 1) != 0
          || health == 0.0f
          || JonesHud_curHealth != health )
        {
            JonesHud_healthIndAlpha = 1.0f;
            JonesHud_msecHealthIndLastFadeUpdate = JonesHud_msecTime;
        }

        else if ( JonesHud_healthIndAlpha > 0.2f
               && JonesHud_curHealth == health
               && JonesHud_HasTimeElapsed(3000u, JonesHud_msecHealthIndLastFadeUpdate + 3000, JonesHud_msecTime) )
        {
            JonesHud_healthIndAlpha -= (float)JonesHud_msecDeltaTime * 0.80000001f / 3000.0f;
            if ( JonesHud_healthIndAlpha < 0.2f )
            {
                JonesHud_healthIndAlpha = 0.2f;
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
    rect.width = JonesHud_healthIndRect.width;
    rect.height = JonesHud_healthIndRect.height;

    if ( state >= 100.01f )
    {
        if ( state >= 200.01f )
        {
            color.red   = JonesHud_colorGreen.red;
            color.green = JonesHud_colorGreen.green;
            color.blue  = JonesHud_colorGreen.blue;
            color.alpha = JonesHud_colorGreen.alpha;
        }
        else
        {
            color.red   = JonesHud_colorYellow.red;
            color.green = JonesHud_colorYellow.green;
            color.blue  = JonesHud_colorYellow.blue;
            color.alpha = JonesHud_colorYellow.alpha;
        }
    }
    else
    {
        color.red   = JonesHud_colorRed.red;
        color.green = JonesHud_colorRed.green;
        color.blue  = JonesHud_colorRed.blue;
        color.alpha = JonesHud_colorRed.alpha;
    }

    color.alpha = alpha;

    rect.x = JonesHud_healthIndRect.width / 2.0f - JonesHud_healthIndScale + rect.x;
    rect.y = JonesHud_healthIndRect.width / 2.0f - JonesHud_healthIndScale + rect.y;
    rect.width = JonesHud_healthIndScale * 2.0f;
    rect.height = rect.width;

    JonesHud_Draw(JonesHud_pHudBaseMat, &rect, z, w, &color, 0, 1);
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
                color.red = JonesHud_colorYellow.red;
                color.green = JonesHud_colorYellow.green;
                color.blue = JonesHud_colorYellow.blue;
                color.alpha = JonesHud_colorYellow.alpha;
            }
            else
            {
                color.red = JonesHud_colorRed.red;
                color.green = JonesHud_colorRed.green;
                color.blue = JonesHud_colorRed.blue;
                color.alpha = JonesHud_colorRed.alpha;
            }
        }
        else
        {
            color.red = JonesHud_colorBlack.red;
            color.green = JonesHud_colorBlack.green;
            color.blue = JonesHud_colorBlack.blue;
            color.alpha = JonesHud_colorBlack.alpha;
        }

        JonesHud_healthIndBarPos.z = z;
        JonesHud_healthIndBarPos.w = w;

        color.alpha = alpha;
        JonesHud_DrawIndicatorBar(&JonesHud_healthIndBarPos, JonesHud_healthIndScale, &color, angle, 0);
    }
}

void J3DAPI JonesHud_DrawIndicatorBar(const rdVector4* pPos, float scale, const rdVector4* pColor, float angle, int bArc)
{
    rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
    if ( pPoly )
    {
        pPoly->flags        = RD_FF_TEX_TRANSLUCENT;
        pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
        pPoly->pMaterial    = JonesHud_pHudBaseMat;
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
        memcpy(pOutVert, &vert, sizeof(D3DTLVERTEX));

        ++pPoly->numVertices;
        ++vertNum;

        double startAngle = 0.0;
        double maxAngle    = 0.0;
        if ( bArc )
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
            memcpy(pOutVert, &vert, sizeof(D3DTLVERTEX));

            if ( fmod(fmod(curAngle, 360.0), 30.0) == 0.0 )
            {
                size_t sinIdx = (size_t)(fmod(curAngle, 360.0) / 15.0);
                pOutVert->sx  = JonesHud_aCosSinTableScaled[sinIdx][0] + pOutVert->sx; // cos
                pOutVert->sy  = pOutVert->sy - JonesHud_aCosSinTableScaled[sinIdx][1]; // sin
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
                pNextPoly->pMaterial    = JonesHud_pHudBaseMat;
                pNextPoly->matCelNum    = 0;
                pNextPoly->numVertices  = 0;

                pOutVert = pNextPoly->aVertices;
                memcpy(pOutVert, &vert, sizeof(D3DTLVERTEX));
                memcpy(&pNextPoly->aVertices[1], &pPoly->aVertices[vertNum], sizeof(pNextPoly->aVertices[1]));

                pPoly = pNextPoly;
                pPoly->numVertices = 2;
                vertNum = pPoly->numVertices;
                bSkip = true;
            }

            if ( curAngle == maxAngle )
            {
                break;
            }

            if ( curAngle != startAngle || deltaAngle == 0.0 ) {
                curAngle = curAngle - 15.0;
            }
            else {
                curAngle = curAngle - deltaAngle;
            }

            if ( curAngle < maxAngle ) {
                curAngle = maxAngle;
            }
        }

        rdVector_Copy4(&pPoly->extraLight, pColor);
        rdVector_Copy4(&pPoly->aVertIntensities[0], pColor);
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

        pPoly->extraLight.red = pColor->red;
        pPoly->extraLight.green = pColor->green;
        pPoly->extraLight.blue = pColor->blue;
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

        if ( bAlpha ) {
            rdCache_AddAlphaProcFace(4);
        }
        else {
            rdCache_AddProcFace(4);
        }
    }
}

void JonesHud_ProcessMenuItems(void)
{
    int bActivateKeyPressed;
    int v2;

    JonesHud_UpdateItem(JonesHud_pMenuItemLinkedList);

    if ( (JonesHud_hudState & 4) == 0 && (JonesHud_hudState & 8) == 0 )
    {
        if ( (JonesHud_hudState & 2) == 0 )
        {
            v2 = 0;
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
                JonesHud_sub_4198E0(JonesHud_apMenuItems[pLeftItem->id]);
                JonesHud_sub_419B50(pCurItem);

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
                    JonesHud_sub_4198E0(JonesHud_apMenuItems[pRightItem->id]);
                    JonesHud_sub_419B50(pCurItem);
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
        JonesHud_sub_4198E0(JonesHud_pMenuItemLinkedList);

        if ( JonesHud_aSoundFxHandles[2] )      // inv_move.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }
    }
}

void JonesHud_MenuMoveDown(void)
{
    JonesHudMenuItem* pCurItem;
    JonesHudMenuItem* pDownItem;

    if ( JonesHud_pCurSelectedMenuItem
      && JonesHud_pCurSelectedMenuItem->nextDownItemId != -1
      && JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextDownItemId] )
    {
        pCurItem = JonesHud_pCurSelectedMenuItem;
        pDownItem = JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextDownItemId];
        JonesHud_pCurSelectedMenuItem = pDownItem;

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

            pDownItem->startMovePos.x = pDownItem->pos.x;
            pDownItem->startMovePos.y = pDownItem->pos.y;
            pDownItem->startMovePos.z = pDownItem->pos.z;

            pDownItem->endMovePos.x = pDownItem->pos.x;
            pDownItem->endMovePos.y = pDownItem->pos.y;
            pDownItem->endMovePos.z = pDownItem->pos.z;
            pDownItem->endMovePos.z = pDownItem->endMovePos.z + 0.064999998f;

            /*flags = pDownItem->flags;
            (flags & 0xFF) = flags | 8;
            pDownItem->flags = flags;*/
            pDownItem->flags |= 0x8;

            JonesHud_StartItemTranslation(pDownItem, JonesHud_msecMenuItemMoveDuration, 0.064999998f, 1);// 0.064999998f - 1 / 15.384f
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

        rdVector_Copy3(&JonesHud_pCurSelectedMenuItem->startMovePos, &pCurItem->pos);
        rdVector_Copy3(&JonesHud_pCurSelectedMenuItem->pos, &pCurItem->pos);
        rdVector_Copy3(&JonesHud_pCurSelectedMenuItem->endMovePos, &pCurItem->pos);
        JonesHud_pCurSelectedMenuItem->startMovePos.z = JonesHud_pCurSelectedMenuItem->startMovePos.z - 0.064999998f;
        JonesHud_pCurSelectedMenuItem->pos.z = JonesHud_pCurSelectedMenuItem->pos.z - 0.064999998f;

        JonesHud_StartItemTranslation(JonesHud_pCurSelectedMenuItem, JonesHud_msecMenuItemMoveDuration, 0.064999998f, 1);
        JonesHud_pCurSelectedMenuItem->flags &= ~0x01;// dont rotate anim
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
    //int flags;
    //int v1;
    rdVector3* pDownItemPos;
    //int v5;
    rdVector3* pDownItemEndPos;
    //int firstItemId;
    int curItemId;
    JonesHudMenuItem* pCurItem;
    JonesHudMenuItem* pUpItem;

    if ( JonesHud_pCurSelectedMenuItem
      && JonesHud_pCurSelectedMenuItem->nextUpItemId != -1
      && JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextUpItemId] )
    {
        pUpItem = JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextUpItemId];
        pCurItem = JonesHud_pCurSelectedMenuItem;
        curItemId = JonesHud_pCurSelectedMenuItem->id;
        JonesHud_pCurSelectedMenuItem = pUpItem;

        if ( JonesHud_aSoundFxHandles[2] )      // inv_move.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        JonesHud_SetSelectedMenuItem(pCurItem->id, pUpItem);

        /*flags = pUpItem->flags;
        (flags & 0xFF) = flags | 2;
        pUpItem->flags = flags;*/
        pUpItem->flags |= 0x2;

        /*v1 = pCurItem->flags;
        (v1 & 0xFF) = v1 & ~2;
        pCurItem->flags = v1;*/
        pCurItem->flags &= ~0x2;

        pUpItem->nextLeftItemId = pCurItem->nextLeftItemId;
        pUpItem->nextRightItemId = pCurItem->nextRightItemId;
        pCurItem->nextRightItemId = -1;
        pCurItem->nextLeftItemId = -1;

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
            pDownItemPos = &JonesHud_apMenuItems[pUpItem->nextDownItemId]->pos;
            pUpItem->startMovePos.x = pDownItemPos->x;
            pUpItem->startMovePos.y = pDownItemPos->y;
            pUpItem->startMovePos.z = pDownItemPos->z;

            pDownItemPos = &JonesHud_apMenuItems[pUpItem->nextDownItemId]->pos;
            pUpItem->pos.x = pDownItemPos->x;
            pUpItem->pos.y = pDownItemPos->y;
            pUpItem->pos.z = pDownItemPos->z;

            pDownItemPos = &JonesHud_apMenuItems[pUpItem->nextDownItemId]->pos;
            pUpItem->endMovePos.x = pDownItemPos->x;
            pUpItem->endMovePos.y = pDownItemPos->y;
            pUpItem->endMovePos.z = pDownItemPos->z;

            pUpItem->startMovePos.z = pUpItem->startMovePos.z + 0.064999998f;
            pUpItem->pos.z = pUpItem->pos.z + 0.064999998f;
            pUpItem->flags |= 0x08;
            JonesHud_StartItemTranslation(pUpItem, JonesHud_msecMenuItemMoveDuration, /*moveRate=*/0.064999998f, /*bMoveUp=*/1);

            if ( pUpItem->pos.z > JonesHud_invMenuMaxZ || pUpItem->nextUpItemId == curItemId )
            {
                break;
            }

            pUpItem = JonesHud_apMenuItems[pUpItem->nextUpItemId];
        }

        /*v5 = pCurItem->flags;
        (v5 & 0xFF) = v5 | 8;
        pCurItem->flags = v5;*/
        pCurItem->flags |= 0x8;

        if ( pUpItem->pos.z <= JonesHud_invMenuMaxZ )
        {
            pDownItemEndPos = &JonesHud_apMenuItems[pCurItem->nextDownItemId]->endMovePos;
            pCurItem->startMovePos.x = pDownItemEndPos->x;
            pCurItem->startMovePos.y = pDownItemEndPos->y;
            pCurItem->startMovePos.z = pDownItemEndPos->z;

            pDownItemEndPos = &JonesHud_apMenuItems[pCurItem->nextDownItemId]->endMovePos;
            pCurItem->pos.x = pDownItemEndPos->x;
            pCurItem->pos.y = pDownItemEndPos->y;
            pCurItem->pos.z = pDownItemEndPos->z;

            pDownItemPos = &JonesHud_apMenuItems[pCurItem->nextDownItemId]->pos;
            pCurItem->endMovePos.x = pDownItemPos->x;
            pCurItem->endMovePos.y = pDownItemPos->y;
            pCurItem->endMovePos.z = pDownItemPos->z;

            pCurItem->pyr.pitch = JonesHud_aDfltMenuItemOrients[pCurItem->id].pyr.pitch;
            pCurItem->pyr.roll  = JonesHud_aDfltMenuItemOrients[pCurItem->id].pyr.roll;
            pCurItem->pyr.yaw   = JonesHud_aDfltMenuItemOrients[pCurItem->id].pyr.yaw;
        }
        else
        {
            pCurItem->startMovePos.x = pCurItem->pos.x;
            pCurItem->startMovePos.y = pCurItem->pos.y;
            pCurItem->startMovePos.z = pCurItem->pos.z;

            pCurItem->endMovePos.x = pCurItem->pos.x;
            pCurItem->endMovePos.y = pCurItem->pos.y;
            pCurItem->endMovePos.z = pCurItem->pos.z;
            pCurItem->endMovePos.z = pCurItem->endMovePos.z - 0.064999998f;

            pUpItem->pyr.pitch = JonesHud_aDfltMenuItemOrients[pUpItem->id].pyr.x;
            pUpItem->pyr.roll  = JonesHud_aDfltMenuItemOrients[pUpItem->id].pyr.roll;
            pUpItem->pyr.yaw   = JonesHud_aDfltMenuItemOrients[pUpItem->id].pyr.yaw;
            pUpItem->flags &= ~0x01;
        }

        JonesHud_StartItemTranslation(pCurItem, JonesHud_msecMenuItemMoveDuration, /*moveRate=*/0.064999998f, /*bMoveUp=*/1);
        pCurItem->flags &= ~0x01; // disable updating item rotation

        /*firstItemId = JonesHud_hudState;
        (firstItemId & 0xFF) = JonesHud_hudState | 8;
        JonesHud_hudState = firstItemId;*/
        JonesHud_hudState |= 0x8;
    }
}

void J3DAPI JonesHud_BindActivateControlKeys(int* aKeyIds, int numKeys)
{
    memset(JonesHud_aActivateKeyIds, 0, sizeof(JonesHud_aActivateKeyIds)); // Fixed: 0 all elements pf array
    for ( int i = 0; i < numKeys; ++i )
    {
        JonesHud_aActivateKeyIds[i] = aKeyIds[i];
    }
}

int J3DAPI JonesHud_GetKey(unsigned int keyId)
{
    int bKeyPressed;

    int numPressed = 0;
    if ( JonesHud_bKeyStateUpdated
      && !JonesHud_HasTimeElapsed(JonesHud_msecMenuItemMoveDuration, JonesHud_msecMenuItemMoveDuration + JonesHud_msecLastKeyPressTime, JonesHud_msecTime) )
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

    JonesHud_msecMenuItemMoveDuration = 250;
    if ( keyId == JonesHud_curKeyId )
    {
        if ( JonesHud_msecMenuItemMoveDuration < 250
          && JonesHud_HasTimeElapsed(250u, JonesHud_msecLastKeyPressTime + 250, JonesHud_msecTime) )
        {
            JonesHud_msecMenuItemMoveDuration = 250;
        }

        else if ( JonesHud_msecMenuItemMoveDuration > 75 )
        {
            JonesHud_msecMenuItemMoveDuration = (int32_t)((double)JonesHud_msecMenuItemMoveDuration * 0.75f);
            if ( JonesHud_msecMenuItemMoveDuration < 75 )
            {
                JonesHud_msecMenuItemMoveDuration = 75;
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
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_aSoundFxIdxs); ++i )
    {
        if ( JonesHud_aSoundFxIdxs[i] > -1 )
        {
            int sndIdx = SITHWORLD_STATICINDEX(JonesHud_aSoundFxIdxs[i]);
            JonesHud_aSoundFxHandles[i] = Sound_GetSoundHandle(sndIdx);
        }
    }
}

JonesHudMenuItem* J3DAPI JonesHud_NewMenuItem(rdModel3* pItemIcon3)
{
    JonesHudMenuItem* pMenuItem = (JonesHudMenuItem*)STDMALLOC(sizeof(JonesHudMenuItem));
    if ( !pMenuItem ) {
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
    JonesHudMenuItem* pRightItem;
    JonesHudMenuItem* a1;
    int v16;
    int v17;
    int v18;
    int v19;
    int v20;

    if ( pItem )
    {
        if ( (pItem->flags & 1) != 0 )          // Update rotation anim
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

        if ( (pItem->flags & 8) != 0
          || (pItem->flags & 0x10) != 0
          || (pItem->flags & 0x20) != 0
          || (pItem->flags & 0x80) != 0
          || (pItem->flags & 0x40) != 0 )
        {
            if ( JonesHud_HasTimeElapsed(pItem->msecMoveDuration, pItem->msecMoveEndTime, JonesHud_msecTime) )
            {
                v16 = pItem->flags & 2;
                v18 = pItem->flags & 1;
                v19 = ((pItem->flags >> 10) & 3) << 10;
                v20 = pItem->flags & 0x100;
                v17 = pItem->flags & 0x200;

                pItem->msecMoveDelta = 0;
                rdVector_Copy3(&pItem->startMovePos, &pItem->endMovePos);
                rdVector_Copy3(&pItem->pos, &pItem->endMovePos);

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
                              && pItem->pos.z <= JonesHud_invMenuMaxZ
                              && pItem->pos.z >= JonesHud_invMenuMinZ ) // Changed: from fixed constant -0.102f
                            {
                                /*flags = pItem->flags;
                                (flags & 0xFF) = flags | 1;
                                pItem->flags = flags;*/
                                pItem->flags |= 1;
                            }

                            /*v3 = pItem->flags;
                            (v3 & 0xFF) = v3 & ~8;
                            pItem->flags = v3;*/
                            pItem->flags &= ~0x08;

                            JonesHud_hudState &= ~0x08u;
                        }

                        break;

                    case 0x10:
                        pItem->flags &= ~0x11u;
                        JonesHud_sub_419B50(JonesHud_apMenuItems[pItem->nextDownItemId]);
                        break;

                    case 0x20:
                        pItem->flags &= ~0x20u;
                        JonesHud_sub_4198E0(pItem);
                        break;

                    case 0x40:
                        pItem->flags &= ~0x41u;
                        if ( pItem->nextLeftItemId == -1
                          || !JonesHud_apMenuItems[pItem->nextLeftItemId]
                          || pItem->id == JonesHud_rootMenuItemId )
                        {
                            JonesHud_hudState = 0;
                            jonesCog_g_bMenuVisible = 0;
                            JonesHud_pCloseMenuItem = 0;
                            sithGamesave_CloseRestore();
                            JonesMain_ResumeGame();

                            if ( !sithPlayerControls_g_bCutsceneMode && JonesHud_bMapOpen )
                            {
                                JonesHud_bMapOpen = 0;
                                sithOverlayMap_ToggleMap();
                            }
                        }
                        else
                        {
                            a1 = JonesHud_apMenuItems[pItem->nextLeftItemId];
                            float moveRate = 0.064999998f;
                            if ( a1 == JonesHud_pMenuItemLinkedList )
                            {
                                a1->endMovePos.x = -0.15800001f;
                                moveRate = a1->endMovePos.x - a1->startMovePos.x;
                            }
                            else {
                                a1->endMovePos.x = a1->endMovePos.x - 0.064999998f;
                            }

                            a1->flags |= 0x40u;
                            /*  v4 = a1->flags;
                              (v4 & 0xFF) = v4 & ~0x80;
                              a1->flags = v4;*/
                            a1->flags &= ~0x80;
                            moveRate = fabsf(moveRate);
                            JonesHud_StartItemTranslation(a1, 50, moveRate, 0);
                        }

                        break;

                    case 0x80:
                        /* v5 = pItem->flags;
                         (v5 & 0xFF) = v5 & ~0x80;
                         pItem->flags = v5;*/
                        pItem->flags &= ~0x80;
                        pItem->flags |= 2u;
                        if ( (JonesHud_hudState & 4) == 0
                          && pItem->nextRightItemId != -1
                          && JonesHud_apMenuItems[pItem->nextRightItemId]
                          && JonesHud_apMenuItems[pItem->nextRightItemId]->id != JonesHud_rootMenuItemId )
                        {
                            pRightItem = JonesHud_apMenuItems[pItem->nextRightItemId];
                            rdVector_Copy3(&pRightItem->startMovePos, &pItem->endMovePos);
                            rdVector_Copy3(&pRightItem->pos, &pItem->endMovePos);
                            rdVector_Copy3(&pRightItem->endMovePos, &pItem->endMovePos);
                            pRightItem->endMovePos.x = pRightItem->endMovePos.x + 0.064999998f;

                            /*amount = pRightItem->flags;
                            (amount & 0xFF) = amount | 0x81;
                            pRightItem->flags = amount;*/
                            pRightItem->flags |= 0x81;
                            JonesHud_StartItemTranslation(pRightItem, 100, /*moveRate=*/0.064999998f, 0);
                        }

                        break;

                    default:
                        break;
                }
            }
            else
            {
                // Scope updates item translation
                float dTransX = pItem->endMovePos.x - pItem->startMovePos.x;
                float dTransY = pItem->endMovePos.y - pItem->startMovePos.y;
                float dTransZ = pItem->endMovePos.z - pItem->startMovePos.z;
                pItem->msecMoveDelta += JonesHud_msecDeltaTime;

                if ( pItem->msecMoveDelta < pItem->msecMoveDuration )
                {
                    pItem->pos.x = (float)pItem->msecMoveDelta / (float)(int)pItem->msecMoveDuration * dTransX + pItem->startMovePos.x;
                    pItem->pos.y = (float)pItem->msecMoveDelta / (float)(int)pItem->msecMoveDuration * dTransY + pItem->startMovePos.y;
                    pItem->pos.z = (float)pItem->msecMoveDelta / (float)(int)pItem->msecMoveDuration * dTransZ + pItem->startMovePos.z;
                }
                else
                {
                    rdVector_Copy3(&pItem->pos, &pItem->endMovePos);
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
    if ( (pItem->flags & 1) != 0 )
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
        vecScale.z = JonesHud_aDfltMenuItemOrients[pItem->id].scale * JonesHud_aspectRatioScale; // Added: Multiply scale by aspect ratio scale
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

                rdVector3 pos;
                rdVector_Copy3(&pos, &pItem->pos);

                rdMatrix34 orient = rdroid_g_identMatrix34;
                orient.dvec.x = pos.x;
                orient.dvec.y = pos.y;
                orient.dvec.z = pos.z;

                rdMatrix34 tmat;
                rdMatrix_Multiply34(&tmat, &orient, &rdCamera_g_pCurCamera->orient);
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
                    float textY = pos.y - (70.0f * JonesHud_heightAspectRatioScale);
                    textX /= (JonesHud_widthAspectRatioScale * RD_REF_WIDTH);
                    textY /= (JonesHud_heightAspectRatioScale * RD_REF_HEIGHT);
                    rdFont_DrawTextLineClipped(aItemText, textX, textY, RD_FIXEDPOINT_RHW_SCALE, JonesHud_pMenuFont, RDFONT_ALIGNCENTER);
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

        memset(&pyrOrient, 0, sizeof(pyrOrient));
        pyrOrient.pitch = pItem->pyr.pitch;
        rdMatrix_PostRotate34(&placement, &pyrOrient);

        memset(&pyrOrient, 0, sizeof(pyrOrient));
        pyrOrient.yaw = pItem->pyr.yaw;
        rdMatrix_PostRotate34(&placement, &pyrOrient);

        rdVector_Copy3(&placement.dvec, &pItem->pos);

        rdVector4 color;
        rdModel3_GetThingColor(pItem->prdIcon, &color);
        float oalpha = color.alpha;
        color.alpha  = pItem->alpha;

        if ( (pItem->flags & 8) == 0
          && ((pItem->flags & 0x10) != 0
              || (pItem->flags & 0x20) != 0
              || (pItem->flags & 0x80) != 0
              || (pItem->flags & 0x40) != 0) )
        {
            float v14 = 1.0f, v15 = 1.0f;// Added Init to 1
            switch ( ((((pItem->flags >> 10) & 3) << 10) | pItem->flags & 0x302) ^ pItem->flags & 1 ^ pItem->flags )
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

void J3DAPI JonesHud_MenuActivateItem()
{
    HWND hwnd;
    int32_t bItemDisable;
    SithGameStatistics* pStatistics;
    DWORD exitCode;
    float volume;
    int bNo3DSound;
    int bReverseSound;
    StdDisplayEnvironment* pDisplayEnv;
    JonesDisplaySettings* pDSettings;
    char aFilePath[128];
    char aMsg[256];
    const char* pNoSaveFormat;
    char aSaveGameFilename[128];
    tSoundHandle hSndInfo;
    int v23;

    if ( !JonesHud_pCurSelectedMenuItem || !sithPlayer_g_pLocalPlayerThing )
    {
        return;
    }

    bItemDisable = 0;
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
                        hSndInfo = sithSound_Load(sithWorld_g_pCurrentWorld, "riv_raft_repair.wav"); // inflate sound fx
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
        case JONESHUD_MENU_IQ:                  // Show game statistics
            jonesInventory_UpdateSolvedHintsStatistics();
            jonesInventory_UpdateGameTimeStatistics(sithTime_g_msecGameTime);
            pStatistics = sithGamesave_GetGameStatistics();
            if ( pStatistics && sithGamesave_LockGameStatistics() )
            {
                pStatistics->aLevelStatistic[pStatistics->curLevelNum].iqPoints = jonesInventory_GetTotalIQPoints() - JonesHud_levelStartIQPoints;
                hwnd = stdWin95_GetWindow();
                jonesConfig_ShowStatisticsDialog(hwnd, pStatistics);
                sithGamesave_UnlockGameStatistics();
            }

            dlgResult = 2;
            break;

        case JONESHUD_MENU_HELP:                // help
            // TODO: Help menu item is not put in place in ResetMenuitems
            //       so this scope could be skipped
            if ( JonesHud_hProcessHelp )
            {
                GetExitCodeProcess(JonesHud_hProcessHelp, &exitCode);
                if ( exitCode != STILL_ACTIVE )
                    //if ( exitCode != 259 )
                {
                    JonesHud_hProcessHelp = 0;
                }
            }

            JonesHud_hProcessHelp = JonesHud_OpenHelp(JonesHud_hProcessHelp);
            dlgResult = 2;
            break;

        case JONESHUD_MENU_SAVE_GAME:           // save game
            if ( jonesCog_g_bEnableGamesave )
            {
                JonesHud_RestoreTreasuresStatistics();
                memset(aFilePath, 0, sizeof(aFilePath));
                hwnd = stdWin95_GetWindow();
                dlgResult = jonesConfig_GetSaveGameFilePath(hwnd, aFilePath);
                if ( dlgResult == 1 )
                {
                    jonesInventory_UpdateGameTimeStatistics(sithTime_g_msecGameTime);
                    sithGamesave_Save(aFilePath, 1);
                }
            }

            break;

        case JONESHUD_MENU_LOAD_GAME:
            if ( jonesCog_g_bEnableGamesave )   // load game
            {
                memset(JonesHud_aSlectedNdsFilePath, 0, sizeof(JonesHud_aSlectedNdsFilePath));
                hwnd = stdWin95_GetWindow();
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

            break;

        case JONESHUD_MENU_GAME_SETTINGS:       // game play options
            hwnd = stdWin95_GetWindow();
            dlgResult = jonesConfig_ShowGamePlayOptions(hwnd);
            break;

        case JONESHUD_MENU_CONTROLS_SETTINGS:   // control options
            hwnd = stdWin95_GetWindow();
            dlgResult = jonesConfig_ShowControlOptions(hwnd);
            break;

        case JONESHUD_MENU_DISPLAY_SETTINGS:    // display settings
            pDSettings = JonesMain_GetDisplaySettings();
            pDisplayEnv = JonesMain_GetDisplayEnvironment();
            hwnd = stdWin95_GetWindow();
            dlgResult = jonesConfig_ShowDisplaySettingsDialog(hwnd, pDisplayEnv, pDSettings);
            if ( dlgResult == 1 )
            {
                JonesMain_RefreshDisplayDevice();
            }

            break;

        case JONESHUD_MENU_SOUND_SETTINGS:      // sound settings
            volume = Sound_GetMaxVolume();
            bNo3DSound = Sound_Get3DHWState();
            bReverseSound = wuRegistry_GetIntEx("ReverseSound", 0);
            hwnd = stdWin95_GetWindow();
            dlgResult = jonesConfig_ShowSoundSettingsDialog(hwnd, &volume);
            if ( dlgResult == 1 )
            {
                Sound_SetMaxVolume(volume);
                Sound_Set3DHWState(bNo3DSound);
                Sound_SetReverseSound(bReverseSound);
            }

            break;

        case JONESHUD_MENU_EXIT:                // exit
            memset(aSaveGameFilename, 0, sizeof(aSaveGameFilename));
            hwnd = stdWin95_GetWindow();
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
                        pNoSaveFormat = jonesString_GetString("JONES_STR_NOSAVE");
                        if ( pNoSaveFormat )
                        {
                            memset(aMsg, 0, sizeof(aMsg));
                            STD_FORMAT(aMsg, pNoSaveFormat, aSaveGameFilename);
                            //sprintf(aMsg, pNoSaveFormat, aSaveGameFilename);
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

            break;

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

void J3DAPI JonesHud_ResetMenuItems()
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

            rdVector_Copy3(&JonesHud_apMenuItems[i]->pyr, &JonesHud_aDfltMenuItemOrients[JonesHud_apMenuItems[i]->id].pyr);

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
            if ( curItemId == JONESHUD_MENU_HELP ) {
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

    rdVector_Copy3(&JonesHud_pMenuItemLinkedList->pos, &JonesHud_pMenuItemLinkedList->startMovePos);

    JonesHud_pMenuItemLinkedList->endMovePos.x = -0.167f;
    JonesHud_pMenuItemLinkedList->endMovePos.y = 0.0099999998f;
    JonesHud_pMenuItemLinkedList->endMovePos.z = JonesHud_invMenuMinZ; // Changed: from fixed constant -0.102f

    JonesHud_pMenuItemLinkedList->endMovePos.x = JonesHud_pMenuItemLinkedList->endMovePos.x + 0.064999998f;
    JonesHud_invMenuMaxZ = JonesHud_invMenuMinZ + 0.19499999f; // Changed: from fixed constant -0.102f
}

void J3DAPI JonesHud_SetLeftRightItems(JonesHudMenuItem* pItem1, JonesHudMenuItem* pItem2)
{
    pItem1->nextRightItemId = pItem2->id;
    pItem2->nextLeftItemId = pItem1->id;
}

void J3DAPI JonesHud_SetDownUpItems(JonesHudMenuItem* pItem1, JonesHudMenuItem* pItem2)
{
    pItem1->nextDownItemId = pItem2->id;
    pItem2->nextUpItemId = pItem1->id;
}

void J3DAPI JonesHud_sub_4198E0(JonesHudMenuItem* pItem)
{
    // This function has to do with logic that shows up to 4 item in the column of currently selected item
    //int hudstate;
    //int flags;
    //int v3;
    JonesHudMenuItem* pUpItem;

    if ( pItem->nextUpItemId != -1
      && JonesHud_apMenuItems[pItem->nextUpItemId]
      && JonesHud_apMenuItems[pItem->nextUpItemId] != pItem )
    {
        /*hudstate = JonesHud_hudState;
        (hudstate & 0xFF) = JonesHud_hudState | 2;
        JonesHud_hudState = hudstate;*/
        JonesHud_hudState |= 2;
        while ( pItem->nextUpItemId != -1
             && JonesHud_apMenuItems[pItem->nextUpItemId]
             && pItem->pos.z < JonesHud_invMenuMaxZ
             && (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x01) != 0
             && pItem->pos.z < JonesHud_apMenuItems[pItem->nextUpItemId]->pos.z )
        {
            pItem = JonesHud_apMenuItems[pItem->nextUpItemId];
        }

        if ( (pItem->flags & 8) != 0
          || (pItem->flags & 0x10) != 0
          || (pItem->flags & 0x20) != 0
          || (pItem->flags & 0x80) != 0
          || (pItem->flags & 0x40) != 0 )
        {
            rdVector_Copy3(&pItem->startMovePos, &pItem->pos);
            pItem->endMovePos.z = pItem->endMovePos.z + 0.064999998f;
            pItem->flags &= ~0x10u;
            pItem->flags |= 0x20u;
            JonesHud_StartItemTranslation(pItem, 100, /*moveRate=*/0.064999998f, /*bMoveUp=*/1);
        }

        else if ( pItem->nextUpItemId == -1
               || !JonesHud_apMenuItems[pItem->nextUpItemId]
               || pItem->pos.z >= JonesHud_invMenuMaxZ
               || (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x01) != 0 )
        {
            /*v3 = JonesHud_hudState;
            (v3 & 0xFF) = JonesHud_hudState & 0xFD;
            JonesHud_hudState = v3;*/
            JonesHud_hudState &= ~0x02;
        }
        else
        {
            pUpItem = JonesHud_apMenuItems[pItem->nextUpItemId];
            rdVector_Copy3(&pUpItem->startMovePos, &pItem->pos);
            rdVector_Copy3(&pUpItem->endMovePos, &pItem->pos);
            rdVector_Copy3(&pUpItem->pos, &pItem->pos);
            pUpItem->endMovePos.z = pUpItem->endMovePos.z + 0.064999998f;

            /* flags = pUpItem->flags;
             (flags & 0xFF) = flags | 0x20;
             pUpItem->flags = flags;*/
            pUpItem->flags |= 0x20;
            JonesHud_StartItemTranslation(pUpItem, 100, /*moveRate=*/0.064999998f, /*bMoveUp=*/1);
        }
    }
}

void J3DAPI JonesHud_sub_419B50(JonesHudMenuItem* pItem)
{
    while ( pItem->nextUpItemId != -1
         && JonesHud_apMenuItems[pItem->nextUpItemId]
         && pItem->pos.z < JonesHud_invMenuMaxZ
         && (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x01) != 0 // update rotation anim
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
                if ( JonesHud_aSoundFxHandles[3] )// inv_expand.wav
                {
                    JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[3], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                }

                JonesHud_pCloseMenuItem->flags |= 0x40u;
                JonesHud_pCloseMenuItem->flags &= ~0x80;

                float moveRate = 0.064999998f;
                if ( JonesHud_pCloseMenuItem == JonesHud_pMenuItemLinkedList )
                {
                    JonesHud_pCloseMenuItem->endMovePos.x = -0.15800001f;
                    moveRate = JonesHud_pCloseMenuItem->endMovePos.x - JonesHud_pCloseMenuItem->startMovePos.x;
                }
                else {
                    JonesHud_pCloseMenuItem->endMovePos.x = JonesHud_pCloseMenuItem->endMovePos.x - 0.064999998f;
                }

                moveRate = fabsf(moveRate);
                JonesHud_StartItemTranslation(JonesHud_pCloseMenuItem, /*msecDuration=*/50, moveRate, /*bMoveUp=*/0);
            }
        }
    }
    else
    {
        JonesHudMenuItem* pDownPos = JonesHud_apMenuItems[pItem->nextDownItemId];
        rdVector_Copy3(&pItem->startMovePos, &pItem->pos);
        rdVector_Copy3(&pItem->endMovePos, &pDownPos->pos); // TODO: make sure pDownPos is not null
        pItem->flags &= ~0x20u;
        pItem->flags |= 0x10u;
        JonesHud_StartItemTranslation(pItem, 100, /*moveRate=*/0.064999998f, /*bMoveUp=*/1);
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

            JonesHud_pCurInvChangedItem->endMovePos.x = -0.167f;
            JonesHud_pCurInvChangedItem->endMovePos.y = 0.0099999998f * JonesHud_aspectRatioScale;
            JonesHud_pCurInvChangedItem->endMovePos.z = JonesHud_invMenuMinZ; // Changed: from fixed constant -0.102f

            JonesHud_pCurInvChangedItem->endMovePos.x = -0.15800001f * JonesHud_aspectRatioScale;// ?? few lines up was set to -0.167f // Changed: Fix end position by multiplying it with aspect scale

            JonesHud_pCurInvChangedItem->pos.x = -0.167f;      // Note not really necessary to init x here as it's assigned few lines lower
            JonesHud_pCurInvChangedItem->pos.y = 0.0099999998f;
            JonesHud_pCurInvChangedItem->pos.z = JonesHud_invMenuMinZ; // Changed: from fixed constant -0.102f

            JonesHud_pCurInvChangedItem->startMovePos.x = -0.167f;     // Note not really necessary to init x here as it's assigned few lines lower
            JonesHud_pCurInvChangedItem->startMovePos.y = 0.0099999998f;
            JonesHud_pCurInvChangedItem->startMovePos.z = JonesHud_invMenuMinZ; // Changed: from fixed constant -0.102f

            rdVector_Copy3(&JonesHud_pCurInvChangedItem->pyr, &JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr);

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
                JonesHud_pCurInvChangedItem->pos.x = 0.1026f * JonesHud_aspectRatioScale; // Changed: Fix current position by multiplying it with aspect scale
                JonesHud_pCurInvChangedItem->startMovePos.x = 0.1026f * JonesHud_aspectRatioScale; // Changed: Fix start position by multiplying with aspect scale
            }
            else
            {
                JonesHud_pCurInvChangedItem->pos.x = 0.15260001f * JonesHud_aspectRatioScale; // Changed: Fix current position by multiplying it with aspect scale
                JonesHud_pCurInvChangedItem->startMovePos.x = 0.15260001f * JonesHud_aspectRatioScale; // Changed: Fix start position by multiplying with aspect scale
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
            JonesHud_StartItemTranslation(JonesHud_pCurInvChangedItem, /*msecDuration=*/2000, moveRate, /*moveUp=*/0);
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
    JonesHud_pCurInvChangedItem->msecMoveDelta *= 3;// ??

    if ( JonesHud_pCurInvChangedItem->startMovePos.x == JonesHud_pCurInvChangedItem->pos.x )
    {
        // Begin translation
        if ( JonesHud_aSoundFxHandles[6] ) // inv_slide_to.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[6], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        float dx = JonesHud_pCurInvChangedItem->endMovePos.x - JonesHud_pCurInvChangedItem->startMovePos.x;
        float dy = JonesHud_pCurInvChangedItem->endMovePos.y - JonesHud_pCurInvChangedItem->startMovePos.y;
        float dz = JonesHud_pCurInvChangedItem->endMovePos.z - JonesHud_pCurInvChangedItem->startMovePos.z;

        JonesHud_pCurInvChangedItem->pos.x = (float)JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f * dx
            + JonesHud_pCurInvChangedItem->startMovePos.x;

        JonesHud_pCurInvChangedItem->pos.y = (float)JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f * dy
            + JonesHud_pCurInvChangedItem->startMovePos.y;

        JonesHud_pCurInvChangedItem->pos.z = (float)JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f * dz
            + JonesHud_pCurInvChangedItem->startMovePos.z;
    }
    else
    {
        float dx = JonesHud_pCurInvChangedItem->endMovePos.x - JonesHud_pCurInvChangedItem->startMovePos.x;
        float dy = JonesHud_pCurInvChangedItem->endMovePos.y - JonesHud_pCurInvChangedItem->startMovePos.y;
        float dz = JonesHud_pCurInvChangedItem->endMovePos.z - JonesHud_pCurInvChangedItem->startMovePos.z;

        JonesHud_pCurInvChangedItem->pos.x = (float)JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f * dx
            + JonesHud_pCurInvChangedItem->startMovePos.x;

        JonesHud_pCurInvChangedItem->pos.y = (float)JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f * dy
            + JonesHud_pCurInvChangedItem->startMovePos.y;

        JonesHud_pCurInvChangedItem->pos.z = (float)JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f * dz
            + JonesHud_pCurInvChangedItem->startMovePos.z;

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

    if ( !pItem ) {
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

    svec.z = JonesHud_aDfltMenuItemOrients[pItem->id].scale * JonesHud_aspectRatioScale; // Added: Multiply scale by aspect ratio scale
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
    rdVector3 tvec;
    memset(&tvec, 0, sizeof(tvec));
    tvec.roll = pItem->pyr.roll;
    rdMatrix_PostRotate34(&placement, &tvec);

    memset(&tvec, 0, sizeof(tvec));
    tvec.pitch = pItem->pyr.pitch;
    rdMatrix_PostRotate34(&placement, &tvec);

    memset(&tvec, 0, sizeof(tvec));
    tvec.yaw = pItem->pyr.yaw;
    rdMatrix_PostRotate34(&placement, &tvec);

    memset(&tvec, 0, sizeof(tvec));
    rdVector_Copy3(&tvec, &pItem->pos);

    tvec.x = tvec.x - (svec.x - 1.0f) * 0.75f * 0.0242722f;
    placement.dvec.x = tvec.x;
    placement.dvec.y = tvec.y;
    placement.dvec.z = tvec.z;
    rdModel3_Draw(pItem->prdIcon, &placement);
}

int JonesHud_ShowLevelCompleted(void)
{
    HWND hwnd;
    float amount_1;
    float amountb;
    float v8;
    float v9;
    int bItemAvailable;
    float amount_2;
    float v13;
    float amount;
    unsigned int menuID_1;
    int menuID;
    SithGameStatistics* pGameStatistics;
    int aItemsState[14];
    int balance;

    int v18 = 0;
    int elapsedTime = 0;
    int iqPoints = 0;
    int foundTreasureValue = 0;
    int totalTrasureValue = 0;
    int numFoundTreasures = 0;

    SithGameStatistics* pStatistics = sithGamesave_GetGameStatistics();
    if ( pStatistics && sithGamesave_LockGameStatistics() )
    {
        elapsedTime = pStatistics->aLevelStatistic[pStatistics->curLevelNum - 1].elapsedTime;
        iqPoints = pStatistics->totalIQPoints - JonesHud_levelStartIQPoints;
        numFoundTreasures = pStatistics->aLevelStatistic[pStatistics->curLevelNum - 1].numFoundTreasures;
        if ( (sithInventory_g_aTypes[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
        {
            amount = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].amount;
        }
        else
        {
            amount = 0.0f;
        }

        totalTrasureValue = (int32_t)amount;
        foundTreasureValue = totalTrasureValue - JonesHud_foundTreasureValue;
        sithGamesave_UnlockGameStatistics();
    }

    if ( (sithInventory_g_aTypes[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
    {
        v13 = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].amount;
    }
    else
    {
        v13 = 0.0f;
    }

    balance = (int32_t)v13;
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_aStoreItems); ++i )
    {
        if ( JonesHud_aStoreItems[i].menuID == JONESHUD_MENU_INVITEM_BONUSMAP
          && !sithPlayer_g_bBonusMapBought
          && JonesMain_GetCurrentLevelNum() != 17
          && JonesMain_GetCurrentLevelNum() != 10
          && JonesMain_GetCurrentLevelNum() != 15
          || JonesHud_aStoreItems[i].menuID >= JONESHUD_MENU_HEALTH_SMALL
          && JonesHud_aStoreItems[i].menuID <= JONESHUD_MENU_HEALTH_POISONKIT )
        {
            if ( (sithInventory_g_aTypes[JonesHud_apMenuItems[JonesHud_aStoreItems[i].menuID]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
            {
                amount_2 = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[JonesHud_aStoreItems[i].menuID]->inventoryID].amount;
            }
            else
            {
                amount_2 = 0.0f;
            }

            aItemsState[i] = (16 * (int32_t)amount_2) | (JonesHud_aStoreItems[i].menuID << 16) | 1;
        }
        else
        {
            menuID = JonesHud_aStoreItems[i].menuID;
            bItemAvailable = 0;
            if ( JonesHud_apMenuItems[menuID] )
            {
                if ( (sithInventory_g_aTypes[JonesHud_apMenuItems[menuID]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
                   ? sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[menuID]->inventoryID].status & SITHINVENTORY_ITEM_FOUND
                   : 0 )
                {
                    bItemAvailable = 1;
                }
            }

            if ( bItemAvailable )
            {
                if ( (sithInventory_g_aTypes[JonesHud_apMenuItems[JonesHud_aStoreItems[i].menuID]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
                {
                    v9 = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[JonesHud_aStoreItems[i].menuID]->inventoryID].amount;
                }
                else
                {
                    v9 = 0.0f;
                }

                aItemsState[i] = bItemAvailable | (16 * (int32_t)v9) | (menuID << 16);
            }
            else
            {
                aItemsState[i] = menuID << 16;
            }
        }
    }

    hwnd = stdWin95_GetWindow();
    if ( jonesConfig_ShowLevelCompletedDialog(
        hwnd,
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
            menuID_1 = (aItemsState[i] & ~0xFFFFu) >> 16;
            if ( (uint16_t)aItemsState[i] )
            {
                if ( menuID_1 == JONESHUD_MENU_INVITEM_BONUSMAP )
                {
                    jonesConfig_UpdateCurrentLevelNum();
                    v18 = 1;
                    JonesHud_bBonusMapBought = 1;
                }

                else if ( JonesHud_apMenuItems[menuID_1] )
                {
                    amount_1 = (float)(JonesHud_aStoreItems[i].unknown132 * (uint16_t)aItemsState[i]);
                    sithInventory_ChangeInventory(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[menuID_1]->inventoryID, amount_1);
                    sithInventory_SetInventoryAvailable(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[menuID_1]->inventoryID, 1);
                }
            }
        }

        amountb = (float)balance;
        sithInventory_SetInventory(sithPlayer_g_pLocalPlayerThing, JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID, amountb);
    }

    pGameStatistics = sithGamesave_GetGameStatistics();
    if ( !pGameStatistics || !sithGamesave_LockGameStatistics() )
    {
        return v18;
    }

    if ( (sithInventory_g_aTypes[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
    {
        v8 = sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->aItems[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].amount;
    }
    else
    {
        v8 = 0.0f;
    }

    pGameStatistics->foundTreasureValue = (int32_t)v8;
    pGameStatistics->numFoundTreasures = 0;

    sithGamesave_UnlockGameStatistics();
    return v18;
}

HANDLE J3DAPI JonesHud_OpenHelp(HANDLE process)
{
    J3D_UNUSED(process);
    struct _SHELLEXECUTEINFOA execInfo;
    const char* pFilename;
    char aPath[128];
    wchar_t* pwString;
    HANDLE hProcess;

    pwString = sithString_GetString("SITHSTRING_HELPFILE");
    hProcess = 0;
    if ( !pwString )
    {
        return hProcess;
    }

    pFilename = stdUtil_ToAString(pwString);
    memset(aPath, 0, sizeof(aPath));
    wuRegistry_GetStr("Install Path", aPath, sizeof(aPath), std_g_aEmptyString);
    if ( pFilename && strlen(aPath) )
    {
        memset(&execInfo, 0, sizeof(execInfo));
        execInfo.cbSize = 60;
        execInfo.fMask  = 80;
        execInfo.lpVerb = "open";
        execInfo.lpFile = pFilename;
        execInfo.lpDirectory = aPath;
        execInfo.nShow = 1;
        ShellExecuteExA(&execInfo);
        hProcess = execInfo.hProcess;
    }


    if ( pFilename )
    {
        stdMemory_Free((void*)pFilename);
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
    char aFilename[128];

    hSndChannel = 0;
    hSnd = 0;
    sithSoundMixer_StopAll();
    memset(aFilename, 0, sizeof(aFilename));
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

        if ( strlen(aFilename) )
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
                JonesHud_hudState = 0;
                jonesCog_g_bMenuVisible = 0;
                sithGamesave_CloseRestore();
                JonesMain_ResumeGame();
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
            JonesHud_curCelNum = 0;

            memset(JonesHud_apCreditsMats, 0, sizeof(JonesHud_apCreditsMats));

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

            memset(JonesHud_aCreditsCurPosY, 0, sizeof(JonesHud_aCreditsCurPosY));

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

                                int prevCelNum = JonesHud_curCelNum;
                                JonesHud_curCelNum = (msecCurTime - JonesHud_msecCreditsFadeStart) * JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]->numCels / 1000;

                                if ( prevCelNum == 12 && JonesHud_curCelNum == 13 )// gg_a.mat whip fire cell 12 & 13
                                {
                                    tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(3));// gen_whip_fire.wav
                                    if ( hSnd )
                                    {
                                        JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                                    }
                                }

                                if ( JonesHud_curCelNum == JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]->numCels
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
                    rdVector_Copy4(&aFontColor[k], &JonesHud_colorWhite);
                    aFontColor[k].alpha = fontAlpha;
                }
                rdFont_SetFontColor(aFontColor);

                float posY = JonesHud_aCreditsCurPosY[i] / (float)(int)JonesHud_creditsCanvasHeight;
                rdFont_DrawTextLine(JonesHud_aCredits[i].aText, 0.5f, posY, RD_FIXEDPOINT_RHW_SCALE, JonesHud_pCreditsFont2, RDFONT_ALIGNCENTER);

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
                        rdVector_Copy4(&textColor[m], &JonesHud_colorWhite);
                    }
                    else
                    {
                        rdVector_Copy4(&textColor[m], &JonesHud_aCreditFontColors[JonesHud_aCredits[i].fontColorNum]);
                    }

                    textColor[m].alpha  = fontAlpha;
                }
                rdFont_SetFontColor(textColor);

                const char* pText = jonesString_GetString(JonesHud_aCredits[i].aText);
                float posY = JonesHud_aCreditsCurPosY[i] / (float)(int)JonesHud_creditsCanvasHeight;
                if ( pText )
                {
                    rdFont_DrawTextLine(pText, 0.5f, posY, RD_FIXEDPOINT_RHW_SCALE, JonesHud_pCreditsFont1, RDFONT_ALIGNCENTER);
                }
                else
                {
                    rdFont_DrawTextLine(JonesHud_aCredits[i].aText, 0.5f, posY, RD_FIXEDPOINT_RHW_SCALE, JonesHud_pCreditsFont1, RDFONT_ALIGNCENTER);
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

                    rdVector4 fontColor;
                    rdVector_Copy4(&fontColor, &JonesHud_aCreditFontColors[JonesHud_aCredits[i].fontColorNum]);
                    fontColor.alpha = fontAlpha;

                    JonesHud_Draw(pIconMat, &rect, RD_FIXEDPOINT_RHW_SCALE, RD_FIXEDPOINT_RHW_SCALE, &fontColor, JonesHud_curCelNum, 1);
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