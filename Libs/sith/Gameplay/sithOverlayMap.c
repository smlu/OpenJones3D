#include "sithOverlayMap.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdPrimit2.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/Devices/sithConsole.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdColor.h>
#include <std/General/stdConfig.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>


#define SITHOVERLAYMAP_NUMZLEVELS 9u

static bool sithOverlayMap_bOpened           = false; // Added: Init to false
static bool sithOverlayMap_bShowHints        = false; // Added: Init to false
static bool sithOverlayMap_bMapVisible       = false; // Added: Init to false
static bool sithOverlayMap_bNoSeenHintsCount = false; // Added: Init to false

static SithOverlayMapConfig sithOverlayMap_config;

static rdVector2 sithOverlayMap_canvasPos;
static rdMatrix34 sithOverlayMap_mapOrient;

static float sithOverlayMap_curScale = 75.0f;
static float sithOverlayMap_vertexScale;

static SithWorld* sithOverlayMap_pCurWorld;
static SithThing* sithOverlayMap_pLocalPlayer;

static int sithOverlayMap_curHintNum                = -1; // Added: Init to -1
static size_t sithOverlayMap_curHintIconNum;        // Is inited in open
static rdMaterial* sithOverlayMap_aHintIcons[10]    = { 0 }; // Added: Init to 0
static const char* sithOverlayMap_aHintIconNames[1] = { "hinticon.mat" };

static size_t sithOverlayMap_curIndyIconNum;
static rdMaterial* sithOverlayMap_aIndyIcons[10]    = { 0 }; // Added: Init to 0
static const char* sithOverlayMap_aMapIconNames[1]  = { "indyicon.mat" };

// Chalk mark thing icons related vars
static size_t sithOverlayMap_curMarkIconNum;
static rdMaterial* sithOverlayMap_aMarkIcons[10]         = { 0 }; // Added: Init to 0
static const char* sithOverlayMap_aChalkMarkIconNames[1] = { "gen_4icon_chalk_diamond.mat" };

// Local player icon related vars
static const rdVector2 sithOverlayMap_aPlayerIconVerts[4] = { { -0.1f, -0.1f }, { 0.1f, -0.1f }, { 0.1f, 0.1f }, { -0.1f, 0.1f } };
static const rdVector2 sithOverlayMap_aPlayerIconUVs[4]   = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

// Hint thing icon related vars
static const rdVector2 sithOverlayMap_aHintIconVerts[4] = { { -0.075f, -0.075f },  { 0.075f, -0.075f },  { 0.075f, 0.075f },  { -0.075f, 0.075f } };
static const rdVector2 sithOverlayMap_aMarkIconVerts[4] = { { -0.075f, -0.075f },  { 0.075f, -0.075f },  { 0.075f, 0.075f },  { -0.075f, 0.075f } };
static const rdVector2 sithOverlayMap_aThingIconUVs[4]  = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

// Vars used by sithOverlayMap_DrawThingIcon
static int sithOverlayMap_iconBlinkStep;
static uint32_t sithOverlayMap_msecLastIconBlinkUpdate;
static int sithOverlayMap_aIconBlinkUpdateIntervals[10] = { 135, 55, 40, 20, 8, 4, 2, 1, 0, 75 };

// Easter egg console command - "06/15/97"
static const char sithOverlayMap_aEncEasterEggCmd[8] = { '\0', '\x06', '\xFF', '\x01', '\x05', '\xFF', '\t', '\a' };

// Vars used by sithOverlayMap_DrawPlayerIcon
static const rdVector4 sithOverlayMap_playerLightColor = { { 1.0f }, { 1.0f }, { 1.0f }, { 1.0f } }; // Found in debug version, release version had it inlined

void J3DAPI sithOverlayMap_DrawSectors(SithSector* pSector);
int J3DAPI sithOverlayMap_DrawSector(SithSector* pSector);
void sithOverlayMap_DrawPlayerIcon(void);
void J3DAPI sithOverlayMap_DrawThingIcon(const SithThing* pThing, const rdMaterial* pIcon, const rdVector2* aVertices, int bZLevelColor, int bBlink);

int J3DAPI sithOverlayMap_EasterEggConsoleCallback(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithOverlayMap_CanDrawSurfaceEdge(SithSurface* pSurface, int vertIdx, int nextVertIdx);

void sithOverlayMap_InstallHooks(void)
{
    J3D_HOOKFUNC(sithOverlayMap_EasterEggConsoleCallback);
    J3D_HOOKFUNC(sithOverlayMap_Open);
    J3D_HOOKFUNC(sithOverlayMap_Close);
    J3D_HOOKFUNC(sithOverlayMap_IsMapVisible);
    J3D_HOOKFUNC(sithOverlayMap_ToggleMap);
    J3D_HOOKFUNC(sithOverlayMap_ZoomIn);
    J3D_HOOKFUNC(sithOverlayMap_ZoomOut);
    J3D_HOOKFUNC(sithOverlayMap_Draw);
    J3D_HOOKFUNC(sithOverlayMap_SetShowHints);
    J3D_HOOKFUNC(sithOverlayMap_EnableMapRotation);
    J3D_HOOKFUNC(sithOverlayMap_GetMapRotation);
    J3D_HOOKFUNC(sithOverlayMap_GetShowHints);
    J3D_HOOKFUNC(sithOverlayMap_UpdateHints);
    J3D_HOOKFUNC(sithOverlayMap_SetHintSolved);
    J3D_HOOKFUNC(sithOverlayMap_SetHintUnsolved);
    J3D_HOOKFUNC(sithOverlayMap_GetNumSeenHints);
    J3D_HOOKFUNC(sithOverlayMap_DrawSectors);
    J3D_HOOKFUNC(sithOverlayMap_DrawSector);
    J3D_HOOKFUNC(sithOverlayMap_DrawPlayerIcon);
    J3D_HOOKFUNC(sithOverlayMap_DrawThingIcon);
    J3D_HOOKFUNC(sithOverlayMap_CanDrawSurfaceEdge);
}

void sithOverlayMap_ResetGlobals(void)
{}

int J3DAPI sithOverlayMap_EasterEggConsoleCallback(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    size_t indyCelNum = (sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum]->curCelNum + 1) % sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum]->numCels;
    size_t markCelNum = (sithOverlayMap_aMarkIcons[sithOverlayMap_curMarkIconNum]->curCelNum + 1) % sithOverlayMap_aMarkIcons[sithOverlayMap_curMarkIconNum]->numCels;
    size_t hintCelNum = (sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum]->curCelNum + 1) % sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum]->numCels;

    sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum]->curCelNum = hintCelNum;
    sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum]->curCelNum = indyCelNum;
    sithOverlayMap_aMarkIcons[sithOverlayMap_curMarkIconNum]->curCelNum = markCelNum;
    return 1;
}

int J3DAPI sithOverlayMap_Open(SithOverlayMapConfig* pConfig)
{
    if ( sithOverlayMap_bOpened )
    {
        SITHLOG_ERROR("Warning: System already open!\n");
        return 0;
    }

    sithOverlayMap_bShowHints          = stdConfig_GetBool(SITHOVERLAYMAP_CFG_GAMEPLAY_SHOWHINTS, false);
    sithOverlayMap_config.bMapRotation = stdConfig_GetBool(SITHOVERLAYMAP_CFG_GAMEPLAY_MAPROTATION, false);

    sithOverlayMap_vertexScale = ((sithOverlayMap_curScale - 10.0f) / 290.0f) * 200.0f + 110.0f;

    sithOverlayMap_config.aZLevelBounds = NULL;
    sithOverlayMap_config.aZLevelColors = NULL;

    if ( pConfig )
    {
        STD_COPYMEM(&sithOverlayMap_config, pConfig, sizeof(sithOverlayMap_config));
    }
    else
    {
        // Init map geometry Z-levels

        sithOverlayMap_config.numZLevels    = SITHOVERLAYMAP_NUMZLEVELS;
        sithOverlayMap_config.aZLevelBounds = (float*)STDMALLOC(sizeof(float) * SITHOVERLAYMAP_NUMZLEVELS);
        if ( !sithOverlayMap_config.aZLevelBounds )
        {
            // TODO: maybe add log
            sithOverlayMap_Close();
            return 1;
        }

        sithOverlayMap_config.aZLevelBounds[0] = -1.0f;
        sithOverlayMap_config.aZLevelBounds[1] = -0.80000001f;
        sithOverlayMap_config.aZLevelBounds[2] = -0.40000001f;
        sithOverlayMap_config.aZLevelBounds[3] = -0.2f;
        sithOverlayMap_config.aZLevelBounds[4] = -0.1f;
        sithOverlayMap_config.aZLevelBounds[5] =  0.2f;
        sithOverlayMap_config.aZLevelBounds[6] =  0.40000001f;
        sithOverlayMap_config.aZLevelBounds[7] =  0.80000001f;
        sithOverlayMap_config.aZLevelBounds[8] =  1.0f;

        sithOverlayMap_config.aZLevelColors = (D3DCOLOR*)STDMALLOC(sizeof(*sithOverlayMap_config.aZLevelColors) * sithOverlayMap_config.numZLevels);
        if ( !sithOverlayMap_config.aZLevelColors )
        {
            sithOverlayMap_Close();
            return 1;
        }

        // Init geometry Z-level colors
        sithOverlayMap_config.aZLevelColors[0] = STD_RGB(0, 38, 176);    // 0xFF0026B0
        sithOverlayMap_config.aZLevelColors[1] = STD_RGB(18, 92, 170);   // 0xFF125CAA
        sithOverlayMap_config.aZLevelColors[2] = STD_RGB(51, 194, 158);  // 0xFF33C29E
        sithOverlayMap_config.aZLevelColors[3] = STD_RGB(145, 225, 92);  // 0xFF91E15C
        sithOverlayMap_config.aZLevelColors[4] = STD_RGB(255, 255, 30);  // 0xFFFFFF1E
        sithOverlayMap_config.aZLevelColors[5] = STD_RGB(232, 180, 21);  // 0xFFE8B415
        sithOverlayMap_config.aZLevelColors[6] = STD_RGB(210, 142, 24);  // 0xFFD28E18
        sithOverlayMap_config.aZLevelColors[7] = STD_RGB(201, 80, 15);   // 0xFFC9500F
        sithOverlayMap_config.aZLevelColors[8] = STD_RGB(194, 0, 0);     // 0xFFC20000

        // Init thing map color
        sithOverlayMap_config.playerBoundsColor    = STD_RGB(255, 15, 0);   // 0xFFFF0F00
        sithOverlayMap_config.playerDirectionColor = STD_RGB(0, 255, 0);    // 0xFF00FF00
        sithOverlayMap_config.actorBoundsColor     = STD_RGB(127, 127, 15); // 0xFF7F7F0F
        sithOverlayMap_config.actorDirectionColor  = STD_RGB(15, 127, 15);  // 0xFF0F7F0F
        sithOverlayMap_config.itemBoundsColor      = STD_RGB(0, 0, 255);    // 0xFF0000FF
        sithOverlayMap_config.weaponBoundsColor    = STD_RGB(0, 0, 119);    // 0xFF000077
        sithOverlayMap_config.defaultBoundsColor   = STD_RGB(51, 51, 255);  // 0xFF3333FF
    }

    STD_ZEROMEM(sithOverlayMap_aHintIcons, sizeof(sithOverlayMap_aHintIcons));
    STD_ZEROMEM(sithOverlayMap_aIndyIcons, sizeof(sithOverlayMap_aIndyIcons));

    for ( size_t i = 0; i < STD_ARRAYLEN(sithOverlayMap_aHintIconNames); i++ )
    {
        sithOverlayMap_aHintIcons[i] = sithMaterial_Load(sithOverlayMap_aHintIconNames[i]);
    }

    // Init map icons
    for ( size_t i = 0; i < STD_ARRAYLEN(sithOverlayMap_aMapIconNames); i++ )
    {
        sithOverlayMap_aIndyIcons[i] = sithMaterial_Load(sithOverlayMap_aMapIconNames[i]);
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(sithOverlayMap_aChalkMarkIconNames); i++ )
    {
        sithOverlayMap_aMarkIcons[i] = sithMaterial_Load(sithOverlayMap_aChalkMarkIconNames[i]);
    }

    sithOverlayMap_curHintIconNum = 0;
    sithOverlayMap_curIndyIconNum = 0;
    sithOverlayMap_curMarkIconNum = 0;

    sithOverlayMap_UpdateHints();

    sithOverlayMap_bOpened = true;

    // Init Easter egg command
    char aEasterEggCmd[STD_ARRAYLEN(sithOverlayMap_aEncEasterEggCmd) + 4] = { 0 }; // Added: Init to 0
    for ( size_t i = 0; i < STD_ARRAYLEN(sithOverlayMap_aEncEasterEggCmd); ++i )
    {
        aEasterEggCmd[i] = sithOverlayMap_aEncEasterEggCmd[i] + '0';
    }

    aEasterEggCmd[STD_ARRAYLEN(sithOverlayMap_aEncEasterEggCmd)] = 0;

    sithConsole_RegisterCommand(sithOverlayMap_EasterEggConsoleCallback, aEasterEggCmd, 0);
    return 0;
}

void sithOverlayMap_Close(void)
{
    if ( !sithOverlayMap_bOpened )
    {
        SITHLOG_ERROR("Warning: System already closed!\n");
        return;
    }

    sithOverlayMap_bMapVisible = false;

    if ( sithOverlayMap_config.aZLevelBounds )
    {
        STDFREE(sithOverlayMap_config.aZLevelBounds);
    }
    sithOverlayMap_config.aZLevelBounds = NULL;

    if ( sithOverlayMap_config.aZLevelColors )
    {
        STDFREE(sithOverlayMap_config.aZLevelColors);
    }
    sithOverlayMap_config.aZLevelColors = NULL;

    sithOverlayMap_bOpened = false;
}

int sithOverlayMap_IsMapVisible(void)
{
    return sithOverlayMap_bMapVisible ? 1 : 0;
}

int sithOverlayMap_ToggleMap(void)
{
    bool bNotVisible = !sithOverlayMap_bMapVisible;
    sithOverlayMap_bMapVisible = bNotVisible;
    return bNotVisible ? 0 : 1; // Altered: Changed to return 1 if visible and 0 if not
}

void sithOverlayMap_ZoomIn(void)
{
    if ( sithOverlayMap_bMapVisible )
    {
        float dScale = 1.0f;
        if ( ((sithOverlayMap_curScale - 10.0f) / 290.0f * 20.0f) > 1.0f )
        {
            dScale = (sithOverlayMap_curScale - 10.0f) / 290.0f * 20.0f;
        }

        sithOverlayMap_curScale += dScale;
        if ( sithOverlayMap_curScale >= 300.0f )
        {
            sithOverlayMap_curScale = 300.0f;
        }
        sithOverlayMap_vertexScale = (sithOverlayMap_curScale - 10.0f) / 290.0f * 200.0f + 110.0f;
    }
}

void sithOverlayMap_ZoomOut(void)
{
    if ( sithOverlayMap_bMapVisible )
    {
        float dScale = 1.0f;
        if ( ((sithOverlayMap_curScale - 10.0f) / 290.0f * 20.0f) > 1.0f )
        {
            dScale = (sithOverlayMap_curScale - 10.0f) / 290.0f * 20.0f;
        }

        sithOverlayMap_curScale -= dScale;

        if ( sithOverlayMap_curScale <= 10.0f )
        {
            sithOverlayMap_curScale = 10.0f;
        }

        sithOverlayMap_vertexScale = (sithOverlayMap_curScale - 10.0f) / 290.0f * 200.0f + 110.0f;
    }
}

void sithOverlayMap_Draw(void)
{
    // Draw level geometry and things
    if ( sithOverlayMap_bMapVisible )
    {
        // Prepare render state
        sithAdvanceRenderTick();
        rdPrimit2_Reset(); // Will switch to wireframe mode

        sithOverlayMap_pCurWorld    = sithWorld_g_pCurrentWorld;
        sithOverlayMap_pLocalPlayer = sithWorld_g_pCurrentWorld->pLocalPlayer;
        sithOverlayMap_canvasPos    = sithCamera_g_pCurCamera->rdCamera.pCanvas->center;

        // Update map orientation
        rdVector3 pyr = { 0 };
        pyr.yaw       = -stdMath_ArcTan4(sithPlayer_g_pLocalPlayerThing->orient.lvec.y, sithPlayer_g_pLocalPlayerThing->orient.lvec.x);
        rdMatrix_BuildRotate34(&sithOverlayMap_mapOrient, &pyr);

        // Draw sectors
        sithOverlayMap_DrawSectors(sithOverlayMap_pLocalPlayer->pInSector);

        // Draw map hints
        for ( int i = 0; i < sithFX_g_numChalkMarks; ++i )
        {
            sithOverlayMap_DrawThingIcon(
                sithFX_g_aChalkMarks[i],
                sithOverlayMap_aMarkIcons[sithOverlayMap_curMarkIconNum],
                sithOverlayMap_aMarkIconVerts,
                sithOverlayMap_aMarkIcons[sithOverlayMap_curMarkIconNum]->curCelNum == 0, // bZLevelColor
                /*bBlink=*/0
            );
        }

        // Set cur hint thing
        SithThing* pCurHintThing = NULL;
        if ( sithOverlayMap_curHintNum != -1 )
        {
            pCurHintThing = &sithWorld_g_pCurrentWorld->aThings[sithOverlayMap_curHintNum];
        }

        // Update & draw cur hint thing
        if ( sithOverlayMap_bShowHints && sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum] && pCurHintThing )
        {
            int32_t hintval = (int32_t)pCurHintThing->userval;
            hintval |= SITH_HINT_SEEN;
            if ( (hintval & SITH_HINT_SOLVED) != 0 )
            {
                sithOverlayMap_UpdateHints();

                // TODO: What's the point?
                hintval = (int32_t)pCurHintThing->userval;
                hintval |= SITH_HINT_SEEN;
            }

            pCurHintThing->userval = (float)hintval;
            sithThing_SyncThing(pCurHintThing, SITHTHING_SYNC_STATE);

            sithOverlayMap_DrawThingIcon(
                pCurHintThing,
                sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum],
                sithOverlayMap_aHintIconVerts,
                sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum]->curCelNum == 0, // bZLevelColor
                sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum]->curCelNum == 0  // bBlink
            );
        }

        // Now draw player on top
        sithOverlayMap_DrawPlayerIcon();
    }
}

void J3DAPI sithOverlayMap_SetShowHints(int bShow)
{
    sithOverlayMap_bShowHints = bShow != 0;
}

void J3DAPI sithOverlayMap_EnableMapRotation(int bEnable)
{
    sithOverlayMap_config.bMapRotation = bEnable;
}

int sithOverlayMap_GetMapRotation(void)
{
    return sithOverlayMap_config.bMapRotation;
}

int sithOverlayMap_GetShowHints(void)
{
    return sithOverlayMap_bShowHints ? 1 : 0;
}

int sithOverlayMap_UpdateHints(void)
{
    if ( !sithWorld_g_pCurrentWorld )
    {
        return 0;
    }

    int maxHintVal = -1;
    int curHintNum = -1;
    for ( int i = 0; i <= sithWorld_g_pCurrentWorld->lastThingIdx; ++i )
    {
        if ( sithWorld_g_pCurrentWorld->aThings[i].type == SITH_THING_HINT )
        {
            int32_t hintflags = (int32_t)sithWorld_g_pCurrentWorld->aThings[i].userval;
            if ( (hintflags & SITH_HINT_SOLVED) == 0 )
            {
                int32_t hintval = (int32_t)sithWorld_g_pCurrentWorld->aThings[i].userval;
                hintval &= ~(SITH_HINT_SOLVED | SITH_HINT_SEEN);
                if ( (size_t)hintval < (size_t)maxHintVal ) // Note: Don't remove casting
                {
                    maxHintVal = hintval;
                    curHintNum = i;
                }
            }
        }
    }

    sithOverlayMap_curHintNum = curHintNum;
    return curHintNum != -1;
}

int J3DAPI sithOverlayMap_SetHintSolved(SithThing* pThing)
{
    if ( !sithWorld_g_pCurrentWorld )
    {
        return 0;
    }

    if ( !pThing )
    {
        return sithOverlayMap_UpdateHints();
    }

    int32_t hintval = (int32_t)pThing->userval;
    hintval |= SITH_HINT_SOLVED;
    pThing->userval = (float)hintval;

    sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    return sithOverlayMap_UpdateHints();
}

int J3DAPI sithOverlayMap_SetHintUnsolved(SithThing* pThing)
{
    if ( !sithWorld_g_pCurrentWorld )
    {
        return 0;
    }

    if ( !pThing )
    {
        return sithOverlayMap_UpdateHints();
    }

    int64_t hintval = (int32_t)pThing->userval;
    hintval &= ~SITH_HINT_SOLVED;
    pThing->userval = (float)hintval;

    sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    return sithOverlayMap_UpdateHints();
}

int J3DAPI sithOverlayMap_NoSeenHintsCountConsoleCallback(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    //  Found in debug version, this function if probably leftover.

    if ( !pArg )
    {
        return 0;
    }

    sithOverlayMap_bNoSeenHintsCount = streqi(pArg, "on");
    return 1;
}

void J3DAPI sithOverlayMap_GetNumSeenHints(size_t* pOutNumSeenHints, size_t* pOutNumHints)
{
    SITH_ASSERT(pOutNumSeenHints && pOutNumHints); // Added

    size_t numSeenHints  = 0;
    size_t totalNumHints = 0;
    if ( sithWorld_g_pCurrentWorld )
    {
        for ( int i = 0; i <= sithWorld_g_pCurrentWorld->lastThingIdx; ++i )
        {
            if ( sithWorld_g_pCurrentWorld->aThings[i].type == SITH_THING_HINT )
            {
                if ( (((int32_t)sithWorld_g_pCurrentWorld->aThings[i].userval) & SITH_HINT_SEEN) != 0 )
                {
                    ++numSeenHints;
                }

                ++totalNumHints;
            }
        }

        if ( sithOverlayMap_bNoSeenHintsCount )
        {
            numSeenHints = 0;
        }

    }

    // Altered: Moved out of if scope to make sure return vars are initialized
    *pOutNumSeenHints = numSeenHints;
    *pOutNumHints     = totalNumHints;
}

SithThing* sithOverlayMap_GetCurrentHintThing(void)
{
    if ( sithOverlayMap_curHintNum != -1 )
    {
        return &sithWorld_g_pCurrentWorld->aThings[sithOverlayMap_curHintNum];
    }

    return NULL;
}

void J3DAPI sithOverlayMap_DrawSectors(SithSector* pSector)
{
    if ( pSector->renderTick != sithMain_g_curRenderTick
        && ((pSector->flags & SITH_SECTOR_SEEN) != 0 || (sithMain_g_sith_mode.mapModeFlags & SITHMAPMODE_SHOWALLSECTORS) != 0) )
    {
        pSector->renderTick = sithMain_g_curRenderTick;

        int  bRenderAdjoinSectors = 1;
        if ( (pSector->flags & SITH_SECTOR_HIDEONMAP) == 0 )
        {
            bRenderAdjoinSectors = sithOverlayMap_DrawSector(pSector);
        }

        if ( bRenderAdjoinSectors )
        {
            for ( SithSurfaceAdjoin* pCurAdjoin = pSector->pFirstAdjoin; pCurAdjoin; pCurAdjoin = pCurAdjoin->pNextAdjoin )
            {
                sithOverlayMap_DrawSectors(pCurAdjoin->pAdjoinSector);
            }
        }
    }
}

int J3DAPI sithOverlayMap_DrawSector(SithSector* pSector)
{
    int bSectorDrawn = 0;
    bool bDrawing = false;

    // Draw level sectors

    SithSurface* pSurf = pSector->pFirstSurface;
    for ( size_t surfNum = 0; surfNum < pSector->numSurfaces; ++surfNum )
    {
        if ( (pSurf->flags & SITH_SURFACE_ISFLOOR) != 0 )
        {
            for ( size_t vecNum = 0; vecNum < pSurf->face.numVertices; ++vecNum )
            {
                int vertIdx     = pSurf->face.aVertices[vecNum];
                int nextVertIdx = pSurf->face.aVertices[(vecNum + 1) % pSurf->face.numVertices];

                int edgeClipState = sithOverlayMap_CanDrawSurfaceEdge(pSurf, vertIdx, nextVertIdx);
                if ( edgeClipState )
                {
                    bDrawing = true;

                    rdVector3 vert     = sithOverlayMap_pCurWorld->aVertices[vertIdx];
                    rdVector3 nextVert = sithOverlayMap_pCurWorld->aVertices[nextVertIdx];

                    rdVector_Sub3Acc(&vert, &sithOverlayMap_pLocalPlayer->pos);
                    rdVector_Sub3Acc(&nextVert, &sithOverlayMap_pLocalPlayer->pos);

                    if ( sithOverlayMap_config.bMapRotation )
                    {
                        rdMatrix_TransformVector34Acc(&vert, &sithOverlayMap_mapOrient);
                        rdMatrix_TransformVector34Acc(&nextVert, &sithOverlayMap_mapOrient);
                    }

                    bool bVertInBounds = false;
                    size_t layer = 0;
                    for ( ; layer < sithOverlayMap_config.numZLevels - 1; ++layer )
                    {
                        if ( vert.z >= sithOverlayMap_config.aZLevelBounds[layer]
                            && vert.z < sithOverlayMap_config.aZLevelBounds[layer + 1] )
                        {
                            bVertInBounds = true;
                            break;
                        }
                    }

                    uint32_t color;
                    if ( bVertInBounds )
                    {
                        color = sithOverlayMap_config.aZLevelColors[layer];
                        //v17 = 0xFFFFFFFF;
                    }
                    else
                    {
                        if ( edgeClipState == 2 )
                        {
                            continue;
                        }

                        layer = 0;
                        if ( vert.z >= sithOverlayMap_config.aZLevelBounds[0] )
                        {
                            layer = sithOverlayMap_config.numZLevels - 1;
                        }

                        color = sithOverlayMap_config.aZLevelColors[layer];
                        //v17 = 0xCCCCCCCC;
                    }

                    rdVector_Scale3Acc(&vert, sithOverlayMap_curScale);
                    rdVector_Scale3Acc(&nextVert, sithOverlayMap_curScale);

                    float x1 = vert.x + sithOverlayMap_canvasPos.x;
                    float y1 = -vert.y + sithOverlayMap_canvasPos.y;

                    float x2 = nextVert.x + sithOverlayMap_canvasPos.x;
                    float y2 = -nextVert.y + sithOverlayMap_canvasPos.y;

                    if ( rdPrimit2_DrawClippedLine2(x1, y1, x2, y2, color) )
                    {
                        bSectorDrawn = 1;
                    }
                }
            }
        }

        pSurf->renderTick = sithMain_g_curRenderTick;
        ++pSurf;
    }

    if ( !bSectorDrawn )
    {
        rdVector3 secCenter = pSector->center;
        rdVector_Scale3Acc(&secCenter, sithOverlayMap_curScale);

        float x1 = secCenter.x + sithOverlayMap_canvasPos.x;
        float y1 = -secCenter.y + sithOverlayMap_canvasPos.y;

        float radius = pSector->radius * sithOverlayMap_curScale;

        if ( (sithCamera_g_pCurCamera->rdCamera.pCanvas->rect.left - radius <= x1
            && sithCamera_g_pCurCamera->rdCamera.pCanvas->rect.right + radius >= x1)
            || (sithCamera_g_pCurCamera->rdCamera.pCanvas->rect.top - radius <= y1
                && sithCamera_g_pCurCamera->rdCamera.pCanvas->rect.bottom + radius >= y1) )
        {
            bSectorDrawn = 1;
        }
    }

    // Draw Things
    if ( (sithMain_g_sith_mode.mapModeFlags & (SITHMAPMODE_SHOWALLTHINGS | SITHMAPMODE_SHOWACTORS | SITHMAPMODE_SHOWPLAYERS)) != 0 )
    {
        for ( SithThing* pCurThing = pSector->pFirstThingInSector; pCurThing; pCurThing = pCurThing->pNextThingInSector )
        {
            if ( pCurThing != sithWorld_g_pCurrentWorld->pCameraFocusThing && (pCurThing->flags & (SITH_TF_DISABLED | SITH_TF_INVISIBLE | SITH_TF_DESTROYED)) == 0 )
            {
                bool bDrawThing = false;
                uint32_t color, dirLineColor = 0;

                if ( (sithMain_g_sith_mode.mapModeFlags & SITHMAPMODE_SHOWALLTHINGS) != 0 )
                {
                    bDrawThing = true;
                    color = sithOverlayMap_config.defaultBoundsColor;
                }

                switch ( pCurThing->type )
                {
                    case SITH_THING_PLAYER:
                        if ( (sithMain_g_sith_mode.mapModeFlags & (SITHMAPMODE_SHOWACTORS | SITHMAPMODE_SHOWPLAYERS)) != 0 )
                        {
                            bDrawThing = true;
                        }

                        color = sithOverlayMap_config.playerBoundsColor;
                        dirLineColor = sithOverlayMap_config.playerDirectionColor;
                        break;

                    case SITH_THING_ACTOR:
                        if ( (sithMain_g_sith_mode.mapModeFlags & SITHMAPMODE_SHOWACTORS) != 0 )
                        {
                            bDrawThing = true;
                        }

                        color = sithOverlayMap_config.actorBoundsColor;
                        dirLineColor = sithOverlayMap_config.actorDirectionColor;
                        break;

                    case SITH_THING_ITEM:
                        if ( (sithMain_g_sith_mode.mapModeFlags & SITHMAPMODE_SHOWITEMS) != 0 )
                        {
                            bDrawThing = true;
                        }

                        color = sithOverlayMap_config.itemBoundsColor;
                        break;

                    case SITH_THING_WEAPON:
                        if ( (sithMain_g_sith_mode.mapModeFlags & SITHMAPMODE_SHOWWEAPONS) != 0 )
                        {
                            bDrawThing = true;
                        }

                        color = sithOverlayMap_config.weaponBoundsColor;
                        break;

                    default:
                        color = sithOverlayMap_config.defaultBoundsColor;
                        break;
                }

                if ( bDrawThing )
                {
                    rdVector3 vert;
                    rdVector_Sub3(&vert, &pCurThing->pos, &sithOverlayMap_pLocalPlayer->pos);

                    if ( sithOverlayMap_config.bMapRotation )
                    {
                        rdMatrix_TransformVector34Acc(&vert, &sithOverlayMap_mapOrient);
                    }

                    rdVector_Scale3Acc(&vert, sithOverlayMap_curScale);

                    // Draw thing on map
                    float x1 = vert.x + sithOverlayMap_canvasPos.x;
                    float y1 = -vert.y + sithOverlayMap_canvasPos.y;

                    float radius = pCurThing->collide.movesize * sithOverlayMap_curScale;

                    rdPrimit2_DrawClippedCircle2(x1, y1, radius, 20.0f, color);

                    // Draw actor / player direction line
                    if ( pCurThing->type == SITH_THING_ACTOR || pCurThing->type == SITH_THING_PLAYER )
                    {
                        rdVector3 nextVert;
                        rdVector_Scale3(&nextVert, &pCurThing->orient.lvec, pCurThing->collide.movesize * 2.0f);
                        rdVector_Add3Acc(&nextVert, &pCurThing->pos);

                        rdVector_Sub3(&vert, &nextVert, &sithOverlayMap_pLocalPlayer->pos);
                        if ( sithOverlayMap_config.bMapRotation )
                        {
                            rdMatrix_TransformVector34Acc(&vert, &sithOverlayMap_mapOrient);
                        }
                        rdVector_Scale3Acc(&vert, sithOverlayMap_curScale);

                        float x2 = vert.x + sithOverlayMap_canvasPos.x;
                        float y2 = -vert.y + sithOverlayMap_canvasPos.y;

                        rdPrimit2_DrawClippedLine2(x1, y1, x2, y2, dirLineColor);
                    }
                }
            }
        }
    }

    if ( bDrawing )
    {
        return bSectorDrawn;
    }
    else
    {
        return 1;
    }
}

void sithOverlayMap_DrawPlayerIcon(void)
{
    if ( sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum] )
    {
        rdCache_FlushAlpha();
        rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
        if ( pPoly )
        {
            pPoly->flags        = RD_FF_TEX_TRANSLUCENT;
            pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
            pPoly->pMaterial    = sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum];
            pPoly->matCelNum    = sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum]->curCelNum;
            pPoly->numVertices  = STD_ARRAYLEN(sithOverlayMap_aPlayerIconVerts);

            for ( size_t i = 0; i < pPoly->numVertices; ++i )
            {
                LPD3DTLVERTEX pVert = &pPoly->aVertices[i];

                rdVector3 vert;
                vert.x = sithOverlayMap_aPlayerIconVerts[i].x;
                vert.y = sithOverlayMap_aPlayerIconVerts[i].y;
                rdVector_Scale3Acc(&vert, sithOverlayMap_vertexScale);

                if ( !sithOverlayMap_config.bMapRotation )
                {
                    rdMatrix_TransformVector34Acc(&vert, &sithOverlayMap_mapOrient);
                }

                pVert->sx  = vert.x + sithOverlayMap_canvasPos.x;
                pVert->sy  = vert.y + sithOverlayMap_canvasPos.y;
                pVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X1; // Altered: Changed to RD_FIXEDPOINT_RHW_SCALE_X1 from RD_FIXEDPOINT_RHW_SCALE_X5 to be drawn above geometry wireframe and thing icon
                pVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X1; // Altered: Changed to RD_FIXEDPOINT_RHW_SCALE_X1 from RD_FIXEDPOINT_RHW_SCALE_X5 to be drawn above geometry wireframe and thing icon

                pVert->tu = sithOverlayMap_aPlayerIconUVs[i].x;
                pVert->tv = sithOverlayMap_aPlayerIconUVs[i].y;

                pPoly->aVertIntensities[i] = sithOverlayMap_playerLightColor;
            }

            pPoly->extraLight = sithOverlayMap_playerLightColor;
            rdCache_AddAlphaProcFace(pPoly->numVertices);
            rdCache_FlushAlpha();
        }
    }
}

void J3DAPI sithOverlayMap_DrawThingIcon(const SithThing* pThing, const rdMaterial* pIcon, const rdVector2* aVertices, int bZLevelColor, int bBlink)
{
    rdVector4 lightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    if ( pThing && pIcon && aVertices )
    {
        rdVector3 pos = pThing->pos;
        rdVector_Sub3Acc(&pos, &sithOverlayMap_pLocalPlayer->pos);

        if ( bZLevelColor )
        {
            bool bFoundLayer = false;
            size_t layer;
            for ( layer = 0; layer < sithOverlayMap_config.numZLevels - 1; ++layer )
            {
                if ( pos.z >= sithOverlayMap_config.aZLevelBounds[layer] && pos.z < sithOverlayMap_config.aZLevelBounds[layer + 1] )
                {
                    bFoundLayer = true;
                    break;
                }
            }

            if ( !bFoundLayer )
            {
                layer = 0;
                if ( pos.z >= sithOverlayMap_config.aZLevelBounds[0] )
                {
                    layer = sithOverlayMap_config.numZLevels - 1;
                }
            }

            D3DCOLOR layerColor = sithOverlayMap_config.aZLevelColors[layer];
            lightColor.red   = (float)STD_GETRED(layerColor) / 255.0f;
            lightColor.green = (float)STD_GETGREEN(layerColor) / 255.0f;
            lightColor.blue  = (float)STD_GETBLUE(layerColor) / 255.0f;
        }

        if ( bBlink )
        {
            if ( stdPlatform_GetTimeMsec() > sithOverlayMap_aIconBlinkUpdateIntervals[sithOverlayMap_iconBlinkStep] + sithOverlayMap_msecLastIconBlinkUpdate )
            {
                sithOverlayMap_msecLastIconBlinkUpdate = stdPlatform_GetTimeMsec();
                sithOverlayMap_iconBlinkStep = (sithOverlayMap_iconBlinkStep + 1) % 10;
            }

            lightColor.alpha = 1.0f - (float)sithOverlayMap_iconBlinkStep * 0.1f;
            if ( lightColor.alpha <= 0.1f )
            {
                lightColor.alpha = 0.0f;
            }
        }

        // Removed: Redundant
        // rdVector_Sub3(&pos, &pThing->pos, &sithOverlayMap_pLocalPlayer->pos);

        if ( sithOverlayMap_config.bMapRotation )
        {
            rdMatrix_TransformVector34Acc(&pos, &sithOverlayMap_mapOrient);
        }

        rdVector_Scale3Acc(&pos, sithOverlayMap_curScale);

        rdCache_FlushAlpha();
        rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
        if ( pPoly )
        {
            pPoly->flags        = RD_FF_TEX_TRANSLUCENT;
            pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
            pPoly->pMaterial    = (rdMaterial*)pIcon;
            pPoly->matCelNum    = pIcon->curCelNum;
            pPoly->numVertices  = 4;

            float posX = pos.x + sithOverlayMap_canvasPos.x;
            float posY = -pos.y + sithOverlayMap_canvasPos.y;
            for ( size_t i = 0; i < pPoly->numVertices; ++i )
            {
                LPD3DTLVERTEX pVert = &pPoly->aVertices[i];
                pVert->sx  = posX + aVertices[i].x * sithOverlayMap_vertexScale;
                pVert->sy  = posY + aVertices[i].y * sithOverlayMap_vertexScale;
                pVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X2; // Altered: Changed to RD_FIXEDPOINT_RHW_SCALE_X2 from RD_FIXEDPOINT_RHW_SCALE_X5 to be drawn above geometry wireframe
                pVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X2; // Altered: Changed to RD_FIXEDPOINT_RHW_SCALE_X2 from RD_FIXEDPOINT_RHW_SCALE_X5 to be drawn above geometry wireframe

                pVert->tu = sithOverlayMap_aThingIconUVs[i].x;
                pVert->tv = sithOverlayMap_aThingIconUVs[i].y;

                pPoly->aVertIntensities[i] = lightColor;
            }

            pPoly->extraLight = lightColor;

            rdCache_AddAlphaProcFace(pPoly->numVertices);
            rdCache_FlushAlpha();
        }
    }
}

int J3DAPI sithOverlayMap_CanDrawSurfaceEdge(SithSurface* pSurface, int vertIdx, int nextVertIdx)
{
    SithSector* pSec = pSurface->pSector;
    SithSurface* pSecSurf = pSec->pFirstSurface;
    for ( size_t i = 0; i < pSec->numSurfaces; ++i )
    {
        if ( pSecSurf != pSurface )
        {
            if ( pSecSurf->pAdjoin )
            {
                SithSurfaceAdjoin* pAdjoin = pSecSurf->pAdjoin;
                if ( pAdjoin->pAdjoinSurface->face.pMaterial )
                {
                    if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) == 0
                        || pAdjoin->pAdjoinSurface->face.geometryMode
                        && (pAdjoin->pAdjoinSurface->face.flags & RD_FF_TEX_TRANSLUCENT) == 0
                        && pAdjoin->pAdjoinSurface->face.pMaterial->formatType == STDCOLOR_FORMAT_RGB )
                    {
                        for ( size_t j = 0; j < pAdjoin->pAdjoinSurface->face.numVertices; ++j )
                        {
                            if ( pAdjoin->pAdjoinSurface->face.aVertices[j] == nextVertIdx
                                && pAdjoin->pAdjoinSurface->face.aVertices[(j + 1) % pAdjoin->pAdjoinSurface->face.numVertices] == vertIdx )
                            {
                                return (pSecSurf->flags & SITH_SURFACE_ISFLOOR) == 0;
                            }
                        }
                    }
                }
            }
        }

        if ( pSecSurf != pSurface && !pSecSurf->pAdjoin )
        {
            for ( size_t j = 0; j < pSecSurf->face.numVertices; ++j )
            {
                if ( pSecSurf->face.aVertices[j] == nextVertIdx && pSecSurf->face.aVertices[(j + 1) % pSecSurf->face.numVertices] == vertIdx )
                {
                    return (pSecSurf->flags & SITH_SURFACE_ISFLOOR) == 0;
                }
            }
        }

        ++pSecSurf;
    }

    for ( SithSurfaceAdjoin* pAdjoin = pSec->pFirstAdjoin; pAdjoin; pAdjoin = pAdjoin->pNextAdjoin )
    {
        SithSector* pAdjoinSec = pAdjoin->pAdjoinSector;
        pSecSurf = pAdjoinSec->pFirstSurface;
        for ( size_t i = 0; i < pAdjoinSec->numSurfaces; ++i )
        {
            if ( (pSecSurf->flags & SITH_SURFACE_ISFLOOR) != 0 )
            {
                for ( size_t j = 0; j < pSecSurf->face.numVertices; ++j )
                {
                    if ( pSecSurf->face.aVertices[j] == nextVertIdx && pSecSurf->face.aVertices[(j + 1) % pSecSurf->face.numVertices] == vertIdx )
                    {
                        return 0;
                    }
                }
            }

            ++pSecSurf;
        }
    }

    return 2;
}
