#include "JonesDisplay.h"
#include <j3dcore/j3dhook.h>

#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/RTI/symbols.h>

#include <rdroid/Engine/rdCanvas.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Primitives/rdWallpaper.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/std.h>
#include <std/General/stdConfig.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>

#include <wkernel/wkernel.h>

static size_t JonesDisplay_primaryDisplayNum  = 1;
static uint32_t JonesDisplay_backBufferWidth  = 0; // Added: init to 0
static uint32_t JonesDisplay_backBufferHeight = 0;
static rdCanvas* JonesDisplay_pCanvas         = NULL;
static bool JonesDisplay_bDualMonitor         = false; // Added: init to false

// Load screen vars
static rdWallpaper* JonesDisplay_pWallpaper = NULL;
static rdWallLine* JonesDisplay_pWallLine   = NULL;
static tSoundHandle JonesDisplay_hSndLoadMusic;
static tSoundChannelHandle JonesDisplay_hSndChannelLoadMusic;

static size_t JonesDisplay_openLoadScreenCounter = 0;
static int JonesDisplay_bUpdateLoadScreen;
static int JonesDisplay_loadScreenState;
static bool JonesDisplay_bPrimaryLoadMusic = false;

rdVector4 JonesDisplay_wallLineColor = { 1.0f, 0.0f, 0.0f, 1.0f };

// Not used vars
static int JonesDisplay_dword_553328;
static int JonesDisplay_dword_55332C;

void JonesDisplay_InstallHooks(void)
{
    J3D_HOOKFUNC(JonesDisplay_Startup);
    J3D_HOOKFUNC(JonesDisplay_Shutdown);
    J3D_HOOKFUNC(JonesDisplay_Restart);
    J3D_HOOKFUNC(JonesDisplay_SetDefaultVideoMode);
    J3D_HOOKFUNC(JonesDisplay_Open);
    J3D_HOOKFUNC(JonesDisplay_Close);
    J3D_HOOKFUNC(JonesDisplay_EnableDualMonitor);
    J3D_HOOKFUNC(JonesDisplay_UpdateDualScreenWindowSize);
    J3D_HOOKFUNC(JonesDisplay_OpenLoadScreen);
    J3D_HOOKFUNC(JonesDisplay_CloseLoadScreen);
    J3D_HOOKFUNC(JonesDisplay_UpdateLoadProgress);
}

void JonesDisplay_ResetGlobals(void)
{
}

int J3DAPI JonesDisplay_Startup(JonesDisplaySettings* pSettings)
{
    if ( !stdDisplay_Startup() )
    {
        JonesDisplay_Shutdown();
        return 1;
    }

    JonesDisplay_UpdateDualScreenWindowSize(pSettings);
    if ( pSettings->bWindowMode )
    {
        JonesDisplay_bDualMonitor = false;
    }

    if ( !stdDisplay_Open(pSettings->displayDeviceNum) )
    {
        STDLOG_ERROR("Error opening display device.\n");
        JonesDisplay_Shutdown();
        return 1;
    }

    if ( pSettings->bWindowMode )
    {
        wkernel_SetWindowStyle(WS_VISIBLE | WS_OVERLAPPEDWINDOW); // 0x10CF0000
    }
    else
    {
        wkernel_SetWindowStyle(WS_POPUP | WS_VISIBLE);
    }

    JonesDisplay_primaryDisplayNum = pSettings->displayDeviceNum;
    stdDisplay_SetDefaultResolution(pSettings->width, pSettings->height);

    if ( stdDisplay_SetMode(pSettings->videoModeNum, pSettings->bWindowMode == 0, /*numBackBuffers*/pSettings->bBuffering ? 2u : 1u) )
    {
        STDLOG_ERROR("Error setting display mode.\n");
        JonesDisplay_Shutdown();
        return 1;
    }

    stdDisplay_GetBackBufferSize(&JonesDisplay_backBufferWidth, &JonesDisplay_backBufferHeight);

    // For OpenGL/SDL, setting correct window size needs to be handled in stdDisplay
#ifndef J3D_OPENGL
    if ( !JonesDisplay_UpdateDualScreenWindowSize(pSettings) )
    {
        wkernel_SetWindowSize(JonesDisplay_backBufferWidth, JonesDisplay_backBufferHeight);
    }
#endif

    if ( stdDisplay_BackBufferFill(/*color=*/0u, /*pRect=*/NULL) || stdDisplay_Update() )
    {
        JonesDisplay_Shutdown();
        return 1;
    }

    if ( !std3D_Startup() )
    {
        STDLOG_ERROR("std3D startup failed -- no 3D device?");
        JonesDisplay_Shutdown();
        return 1;
    }

    if ( !sithCamera_Startup() )
    {
        STDLOG_ERROR("Error Camera startup.\n");
        JonesDisplay_Shutdown();
        return 1;
    }

    if ( !std3D_Open(pSettings->device3DNum) )
    {
        STDLOG_STATUS("std3D open failed!!!!.\n");
        JonesDisplay_Shutdown();
        return 1;
    }

    if ( std3D_SetMipmapFilter(pSettings->filter) || !rdOpen() )
    {
        JonesDisplay_Shutdown();
        return 1;
    }

    rdSetGeometryMode(pSettings->geoMode);

    if ( sithWorld_g_pCurrentWorld )
    {
        sithWorld_g_pCurrentWorld->state |= SITH_WORLD_STATE_UPDATE_FOG;
    }

    JonesDisplay_bPrimaryLoadMusic    = false;
    JonesDisplay_loadScreenState      = 0;
    JonesDisplay_hSndChannelLoadMusic = 0;
    JonesDisplay_hSndLoadMusic        = 0;
    JonesDisplay_dword_55332C         = 0;
    JonesDisplay_dword_553328         = 0;
    return 0;
}

void JonesDisplay_Shutdown(void)
{
    rdClose();
    std3D_Close();
    stdDisplay_ClearMode();
    stdDisplay_Close();
    sithCamera_Shutdown();
    std3D_Shutdown();
    stdDisplay_Shutdown();

    JonesDisplay_pCanvas = NULL;
}

int J3DAPI JonesDisplay_Restart(JonesDisplaySettings* pSettings)
{
    JonesDisplay_Close();
    JonesDisplay_Shutdown();
    if ( JonesDisplay_Startup(pSettings) )
    {
        return 1;
    }

    return JonesDisplay_Open(pSettings);
}

void J3DAPI JonesDisplay_SetDefaultVideoMode(const StdDisplayEnvironment* pEnv, JonesDisplaySettings* pDisplaySettings)
{
    // TODO: should return error if videomode couldn't be selected

    pDisplaySettings->bWindowMode  = 0;
    pDisplaySettings->bDualMonitor = 0;
    pDisplaySettings->bBuffering   = 0;
    pDisplaySettings->bFog         = 1;
    pDisplaySettings->filter       = STD3D_MIPMAPFILTER_BILINEAR;
    pDisplaySettings->geoMode      = RD_GEOMETRY_FULL;
    pDisplaySettings->lightMode    = RD_LIGHTING_GOURAUD;

    size_t i = 0;
    for ( ; i < pEnv->numInfos; ++i )
    {
        if ( pEnv->aDisplayInfos[i].displayDevice.bHAL )
        {
            pDisplaySettings->displayDeviceNum = i;
            break; // Fixed: Added break
        }
    }

    if ( i < pEnv->numInfos )
    {
        StdDisplayInfo* pInfo = &pEnv->aDisplayInfos[pDisplaySettings->displayDeviceNum];
        if ( pInfo->numDevices )
        {
            // Search for hardware 3D device
            for ( i = 0; i < pInfo->numDevices; ++i )
            {
                if ( strstr(pInfo->aDevices[i].deviceDescription, "HAL") )
                {
                    pDisplaySettings->device3DNum = i;
                    break; // Fixed: Added break out of loop
                }
            }

            StdVideoMode videoMode;
            videoMode.aspectRatio                    = 1.0;
            videoMode.rasterInfo.width               = 640; // TODO: make 16:9 res
            videoMode.rasterInfo.height              = 480;
            videoMode.rasterInfo.colorInfo.bpp       = 16;  // TODO: make it 24/32 BPP
            videoMode.refreshRate                    = 60;  // Added
            videoMode.rasterInfo.colorInfo.colorMode = STDCOLOR_RGB;

            pDisplaySettings->videoModeNum = JonesMain_FindClosestVideoMode(pEnv, &videoMode, pDisplaySettings->displayDeviceNum);

            videoMode = pInfo->aModes[pDisplaySettings->videoModeNum];
            pDisplaySettings->width  = videoMode.rasterInfo.width;
            pDisplaySettings->height = videoMode.rasterInfo.height;
            pDisplaySettings->bClearBackBuffer = 0;

            stdConfig_SetString(JONESDISPLAY_CFG_GRAPHICS_DISPLAY, pEnv->aDisplayInfos[pDisplaySettings->displayDeviceNum].displayDevice.aDriverName);
            stdConfig_SetString(JONESDISPLAY_CFG_GRAPHICS_DEVICE, pEnv->aDisplayInfos[pDisplaySettings->displayDeviceNum].aDevices[pDisplaySettings->device3DNum].deviceDescription);
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_WIDTH, pDisplaySettings->width);
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_HEIGHT, pDisplaySettings->height);
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_BPP, pEnv->aDisplayInfos[pDisplaySettings->displayDeviceNum].aModes[pDisplaySettings->videoModeNum].rasterInfo.colorInfo.bpp);
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_REFRESHRATE, pEnv->aDisplayInfos[pDisplaySettings->displayDeviceNum].aModes[pDisplaySettings->videoModeNum].refreshRate);
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_MIPMAPFILTER, pDisplaySettings->filter);
            stdConfig_SetBool(JONESDISPLAY_CFG_GRAPHICS_WINDOW, pDisplaySettings->bWindowMode);
            stdConfig_SetBool(JONESDISPLAY_CFG_GRAPHICS_DUALMONITOR, pDisplaySettings->bDualMonitor);
            stdConfig_SetInt(JONESDISPLAY_CFG_ENGINE_RENDER_GEOMETRYMODE, pDisplaySettings->geoMode);
            stdConfig_SetInt(JONESDISPLAY_CFG_ENGINE_RENDER_LIGHTINGMODE, pDisplaySettings->lightMode);
        }
        else
        {
            pDisplaySettings->displayDeviceNum = -1;
        }
    }
    else
    {
        pDisplaySettings->displayDeviceNum = -1;
    }
}

int J3DAPI JonesDisplay_Open(JonesDisplaySettings* pSettings)
{
    if ( JonesDisplay_pCanvas )
    {
        JonesDisplay_Close();
    }

    uint32_t height, width;
    stdDisplay_GetBackBufferSize(&width, &height);
    JonesDisplay_pCanvas = rdCanvas_New(0x0, &stdDisplay_g_backBuffer, 0, 0, width, height);
    if ( !JonesDisplay_pCanvas )
    {
        return 1;
    }

    // Added: Load PVS enabled setting from config
    bool bPVSCull = stdConfig_GetBool(JONESDISPLAY_CFG_ENGINE_RENDER_PVSENABLED, true);
    if ( !stdConfig_Contains(JONESDISPLAY_CFG_ENGINE_RENDER_PVSENABLED) )
    {
        stdConfig_SetBool(JONESDISPLAY_CFG_ENGINE_RENDER_PVSENABLED, bPVSCull);
    }

    // Added: Load culled sector traversal mode from config
    SithRenderThingTraversal traversalMode = (SithRenderThingTraversal)stdConfig_GetInt(JONESDISPLAY_CFG_ENGINE_RENDER_CULLEDSECTORTRAVERSALMODE, SITHRENDER_CULLEDSECTOR_TRAVERSALMODE_DEFAULT);
    if ( !stdConfig_Contains(JONESDISPLAY_CFG_ENGINE_RENDER_CULLEDSECTORTRAVERSALMODE) )
    {
        stdConfig_SetInt(JONESDISPLAY_CFG_ENGINE_RENDER_CULLEDSECTORTRAVERSALMODE, traversalMode);
    }

    // Added: Load max thing/light collect distances from config
    float maxThingColectDist = stdConfig_GetFloat(JONESDISPLAY_CFG_ENGINE_RENDER_MAXTHINGCOLLECTDISTANCE, SITHRENDER_MAXTHINGCOLLECTDISTANCE_DEFAULT);
    if ( !stdConfig_Contains(JONESDISPLAY_CFG_ENGINE_RENDER_MAXTHINGCOLLECTDISTANCE) )
    {
        stdConfig_SetFloat(JONESDISPLAY_CFG_ENGINE_RENDER_MAXTHINGCOLLECTDISTANCE, maxThingColectDist);
    }

    float maxLightColectDist = stdConfig_GetFloat(JONESDISPLAY_CFG_ENGINE_RENDER_MAXLIGHTCOLLECTDISTANCE, SITHRENDER_MAXLIGHTCOLLECTDISTANCE_DEFAULT);
    if ( !stdConfig_Contains(JONESDISPLAY_CFG_ENGINE_RENDER_MAXLIGHTCOLLECTDISTANCE) )
    {
        stdConfig_SetFloat(JONESDISPLAY_CFG_ENGINE_RENDER_MAXLIGHTCOLLECTDISTANCE, maxLightColectDist);
    }

    rdSetGeometryMode(pSettings->geoMode);
    sithRender_SetLightingMode(pSettings->lightMode);
    sithRender_SetRenderFlags(sithRender_GetRenderFlags() | RDROID_USE_AMBIENT_CAMERA_LIGHT);

    // Added: Enable/disable PVS culling
    sithRender_EnablePVSCull(bPVSCull);

    // Added: Set culled sector traversal mode and max collect distances
    sithRender_SetCulledSectorTraversalMode(traversalMode);
    sithRender_SetMaxThingCollectDistance(maxThingColectDist);
    sithRender_SetMaxLightCollectDistance(maxLightColectDist);

    StdVideoMode displayMode;
    if ( stdDisplay_GetCurrentVideoMode(&displayMode) )
    {
        return 1;
    }

    sithCamera_Open(JonesDisplay_pCanvas, displayMode.aspectRatio);
    return 0;
}

void JonesDisplay_Close(void)
{
    sithCamera_Close();
    if ( JonesDisplay_pCanvas )
    {
        rdCanvas_Free(JonesDisplay_pCanvas);
    }

    JonesDisplay_pCanvas = NULL;
}

void J3DAPI JonesDisplay_EnableDualMonitor(int bEnable)
{
    JonesDisplay_bDualMonitor = bEnable;
}

int J3DAPI JonesDisplay_UpdateDualScreenWindowSize(const JonesDisplaySettings* pSettings)
{
    if ( !pSettings->bWindowMode || !JonesDisplay_bDualMonitor || !JonesDisplay_primaryDisplayNum )
    {
        return 0;
    }

    wkernel_SetWindowSize(200, 20);
    return 1;
}

void J3DAPI JonesDisplay_OpenLoadScreen(const char* pMatFilePath, float wlStartX, float wlStartY, float wlEndX, float wlEndY, int bPrimaryMusicTheme)
{
    if ( JonesDisplay_openLoadScreenCounter > 2 ) // Greater than 2 is when loading saved game within existing game
    {
        sithSoundMixer_StopAll();
    }
    else
    {
        ++JonesDisplay_openLoadScreenCounter;
    }

    stdDisplay_DisableVSync(true); // Added: This will speed up loading since it will not sync to screen refresh rate allowing higher fps
    JonesDisplay_pWallpaper = rdWallpaper_New(pMatFilePath);
    JonesDisplay_pWallLine  = rdWallpaper_NewWallLine(wlStartX, wlStartY, wlEndX, wlEndY, &JonesDisplay_wallLineColor);

    sithWorld_SetLoadProgressCallback(JonesDisplay_UpdateLoadProgress);
    JonesDisplay_bPrimaryLoadMusic = bPrimaryMusicTheme;
    JonesDisplay_bUpdateLoadScreen = 0;
    sithTime_Reset();
}

void JonesDisplay_CloseLoadScreen(void)
{
    if ( JonesDisplay_pWallpaper )
    {
        rdWallpaper_Free(JonesDisplay_pWallpaper);
    }
    JonesDisplay_pWallpaper = NULL;

    if ( JonesDisplay_pWallLine )
    {
        rdWallpaper_FreeWallLine(JonesDisplay_pWallLine);
    }
    JonesDisplay_pWallLine = NULL;

    if ( JonesDisplay_openLoadScreenCounter > 1 )
    {
        stdDisplay_DisableVSync(false); // Added, re-enable VSync after finish loading level (state 2 or 3)
    }
}

void J3DAPI JonesDisplay_UpdateLoadProgress(float progress)
{
    std3D_ClearZBuffer();
    rdCache_AdvanceFrame();
    sithTime_Advance();
    sithSoundMixer_Update();

    if ( JonesDisplay_openLoadScreenCounter == 1 ) // loading static world
    {
        progress = progress / 2.0f;
    }
    else if ( JonesDisplay_openLoadScreenCounter == 2 ) // loading normal world
    {
        progress = progress / 2.0f + 50.0f;
    }

    if ( std3D_StartScene() )
    {
        // Error starting scene
        std3D_EndScene();
        stdDisplay_Update();
        return;
    }

    // Play loading music
    if ( !JonesDisplay_hSndLoadMusic && progress < 50.0f && !JonesDisplay_hSndChannelLoadMusic )
    {
        JonesDisplay_hSndLoadMusic = JonesDisplay_bPrimaryLoadMusic ? Sound_GetSoundHandle(SITHWORLD_STATICINDEX(0)) : Sound_GetSoundHandle(SITHWORLD_STATICINDEX(1));
        if ( JonesDisplay_hSndLoadMusic )
        {
            sithSoundMixer_StopAll();
            JonesDisplay_hSndChannelLoadMusic = sithSoundMixer_PlaySound(JonesDisplay_hSndLoadMusic, 1.0f, 0.0, SOUNDPLAY_LOOP);
        }
    }

    // Draw background images
    if ( JonesDisplay_pWallpaper )
    {
        if ( progress == 100.0f && (!JonesDisplay_loadScreenState || JonesDisplay_loadScreenState > 0 && JonesDisplay_bUpdateLoadScreen) )
        {
            // Progress completed, increment background image mats cells to show load completed state
            for ( size_t i = 0; i < 6; ++i )
            {
                if ( JonesDisplay_pWallpaper->aMatCelNums[i] < JonesDisplay_pWallpaper->aMaterials[i].numCels - 1 )
                {
                    ++JonesDisplay_pWallpaper->aMatCelNums[i];
                }
            }
        }

        rdWallpaper_Draw(JonesDisplay_pWallpaper);
    }

    // Draw progress bar line
    if ( !JonesDisplay_pWallLine )
    {
        rdCache_Flush();
        rdCache_FlushAlpha();

        std3D_EndScene();
        stdDisplay_Update();
        return;
    }

    rdWallpaper_DrawWallLine(JonesDisplay_pWallLine, progress);

    if ( progress != 100.0 || JonesDisplay_loadScreenState && (JonesDisplay_loadScreenState <= 0 || !JonesDisplay_bUpdateLoadScreen) )
    {
        rdCache_Flush();
        rdCache_FlushAlpha();

        std3D_EndScene();
        stdDisplay_Update();
        return;
    }

    // Loading completed

    size_t msecCurTime   = stdPlatform_GetTimeMsec();
    size_t msecEndUpdate = msecCurTime + 250;

    JonesDisplay_bUpdateLoadScreen = 0;

    if ( ++JonesDisplay_loadScreenState == 1 )
    {
        if ( JonesDisplay_hSndChannelLoadMusic )
        {
            if ( (Sound_GetChannelFlags(JonesDisplay_hSndChannelLoadMusic) & SOUND_CHANNEL_PLAYING) != 0 )
            {
                sithSoundMixer_StopSound(JonesDisplay_hSndChannelLoadMusic);
            }

            JonesDisplay_hSndChannelLoadMusic = 0;
            JonesDisplay_hSndLoadMusic        = 0;
        }

        // Play load finish sound fx
        JonesDisplay_hSndLoadMusic = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(55)); // 55 - alt_arrive.wav
        if ( JonesDisplay_hSndLoadMusic )
        {
            sithSoundMixer_PlaySound(JonesDisplay_hSndLoadMusic, 1.0f, 0.0, SOUNDPLAY_PLAYONCE);
        }

        msecEndUpdate = msecCurTime + 250;
    }

    if ( JonesDisplay_loadScreenState == 2 )
    {
        msecEndUpdate = msecCurTime + 1000;
    }

    rdCache_Flush();
    rdCache_FlushAlpha();
    std3D_EndScene();

    while ( stdPlatform_GetTimeMsec() < msecEndUpdate )
    {
        stdDisplay_Update();
        JonesDisplay_UpdateLoadProgress(100.0f);
    }

    if ( JonesDisplay_loadScreenState >= 3 )
    {
        JonesDisplay_loadScreenState   = 0;
        JonesDisplay_bPrimaryLoadMusic = 0;
        JonesDisplay_dword_553328      = 0; // ???
        sithSoundMixer_StopAll();
        JonesDisplay_hSndLoadMusic        = 0;
        JonesDisplay_hSndChannelLoadMusic = 0;
        JonesDisplay_dword_55332C         = 0; // ???

        rdCache_Flush();
        rdCache_FlushAlpha();
        stdDisplay_Update();
    }
    else
    {
        JonesDisplay_bUpdateLoadScreen = 1;
        rdCache_Flush();
        rdCache_FlushAlpha();
        stdDisplay_Update();
        JonesDisplay_UpdateLoadProgress(100.0f); // Update to show background image in load completed state
    }
}

int JonesDisplay_ShowSplashDemo(void)
{
    rdWallpaper* pWallpaper = rdWallpaper_New("mat\\splashdemo");

    sithSoundMixer_StopAll();
    JonesDisplay_hSndLoadMusic = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(2));
    if ( JonesDisplay_hSndLoadMusic )
    {
        sithSoundMixer_PlaySound(JonesDisplay_hSndLoadMusic, 1.0, 0.0, (SoundPlayFlag)0);
    }

    bool bFinish = false;
    while ( !bFinish && pWallpaper )
    {
        int numPressed;
        stdControl_ReadControls();
        if ( stdControl_ReadKey(DIK_ESCAPE, &numPressed)
            || stdControl_ReadKey(DIK_SPACE, &numPressed)
            || stdControl_ReadKey(DIK_F1, &numPressed)
            || stdControl_ReadKey(DIK_F2, &numPressed)
            || stdControl_ReadKey(DIK_F3, &numPressed)
            || stdControl_ReadKey(DIK_F4, &numPressed)
            || stdControl_ReadKey(DIK_F5, &numPressed)
            || stdControl_ReadKey(DIK_F6, &numPressed)
            || stdControl_ReadKey(DIK_F7, &numPressed)
            || stdControl_ReadKey(DIK_F8, &numPressed)
            || stdControl_ReadKey(DIK_F9, &numPressed)
            || stdControl_ReadKey(DIK_F10, &numPressed)
            //|| stdControl_ReadKey(DIK_F11, &numPressed)
            || stdControl_ReadKey(DIK_F12, &numPressed)
            || stdControl_ReadKey(DIK_RETURN, &numPressed)
            || stdControl_ReadKey(DIK_NUMPADENTER, &numPressed)
            || stdControl_ReadKey(DIK_RCONTROL, &numPressed)
            || stdControl_ReadKey(DIK_LCONTROL, &numPressed)
            || stdControl_ReadKey(DIK_RMENU, &numPressed)
            || stdControl_ReadKey(DIK_LMENU, &numPressed)
            || stdControl_ReadKey(DIK_Q, &numPressed)
            || stdControl_ReadKey(DIK_X, &numPressed)
            || stdControl_ReadKey(DIK_W, &numPressed) )
        {
            bFinish = 1;
        }

        else if ( stdControl_ReadKey(DIK_R, &numPressed) )
        {
            JonesMain_RestartGame();
            if ( !pWallpaper )
            {
                return 0;
            }

            rdWallpaper_Free(pWallpaper);
            pWallpaper = NULL;
            return 0;
        }

        std3D_ClearZBuffer();
        rdCache_AdvanceFrame();
        if ( !std3D_StartScene() )
        {
            rdWallpaper_Draw(pWallpaper);
            rdCache_Flush();
            rdCache_FlushAlpha();
            std3D_EndScene();
            stdDisplay_Update();
        }
    }

    if ( !pWallpaper )
    {
        return 1;
    }

    rdWallpaper_Free(pWallpaper);
    pWallpaper = NULL;
    return 1;
}
