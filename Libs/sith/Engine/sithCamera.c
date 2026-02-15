#include "sithCamera.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

static size_t sithCamera_aCycleCamIndices[3] = { SITHCAMERA_INTCAMERANUM, SITHCAMERA_EXTCAMERANUM, SITHCAMERA_ORBCAMERANUM };

static rdVector3 sithCamera_vecExtCameraLookOffset             = { { 0.0f }, { 0.02f }, { 0.02f } };
static rdVector3 sithCamera_vecExtCameraPrevLookOffset;
static rdVector3 sithCamera_vecExtCameraOffset                 = { { 0.0f }, { -0.2f }, { 0.064999998f } };

static const rdVector3 sithCamera_vecExtCameraDefaultOffset      = { { 0.0f }, { -0.2f }, { 0.064999998f } };
static const rdVector3 sithCamera_vecExtCameraDefaultLookOffset  = { { 0.0f }, { 0.02f }, { 0.02f } };

static const rdVector3 sithCamera_vecCameraOffsetWeaponDrawn   = { { 0.0f }, { -0.22f }, { 0.15000001f } };
static const rdVector3 sithCamera_vecCameraOffsetJeep          = { { 0.0f }, { -0.2f }, { 0.14f } };
static const rdVector3 sithCamera_vecCameraOffsetClimbingUp    = { { 0.0f }, { -0.2f }, { -0.0099999998f } };
static const rdVector3 sithCamera_vecCameraOffsetClimbDown     = { { 0.0f }, { -0.2f }, { 0.16500001f } };
static const rdVector3 sithCamera_vecCameraOffsetClimbingLeft  = { { 0.1f }, { -0.2f }, { 0.064999998f } };
static const rdVector3 sithCamera_vecCameraOffsetClimbingRight = { { -0.1f }, { -0.2f }, { 0.064999998f } };
static const rdVector3 sithCamera_vecCameraOffsetSlideForward  = { { 0.16500001f }, { 0.0099999998f }, { 0.17f } };
static const rdVector3 sithCamera_vecCameraOffsetSlideBackward = { { 0.16500001f }, { -0.2f }, { 0.029999999f } };
static const rdVector3 sithCamera_vecCameraOffsetJumpRollback  = { { 0.0f }, { -0.30000001f }, { 0.064999998f } };

static rdVector3 sithCamera_vecOffsetCamType40  = { { 0.0f }, { 1.0f }, { 1.0f } };

static const rdVector3 sithCamera_idleCamOffset = { { 0.0f }, { -0.2f }, { 0.064999998f } };
static rdMatrix34 sithCamera_idleCamOrient;

static int sithCamera_bFocusThingInvisible;
static float sithCamera_secFocusThingFadeInTime;
static float sithCamera_secLookOffsetTranslateDelta;
static int sithCamera_bUpdateCameraOffset;

static int sithCamera_bStartup;
static int sithCamera_bOpen;

SithSector* J3DAPI sithCamera_SearchSectorInRadius(int a1, SithSector* pSector, const rdVector3* startPos, rdVector3* endPos, float unused, int flags);

void sithCamera_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCamera_Startup);
    J3D_HOOKFUNC(sithCamera_Shutdown);
    J3D_HOOKFUNC(sithCamera_Open);
    J3D_HOOKFUNC(sithCamera_Close);
    J3D_HOOKFUNC(sithCamera_ResetAllCameras);
    J3D_HOOKFUNC(sithCamera_NewEntry);
    J3D_HOOKFUNC(sithCamera_FreeEntry);
    J3D_HOOKFUNC(sithCamera_SetCameraCanvas);
    J3D_HOOKFUNC(sithCamera_SetCurrentCamera);
    J3D_HOOKFUNC(sithCamera_CycleCamera);
    J3D_HOOKFUNC(sithCamera_SetCurrentToCycleCamera);
    J3D_HOOKFUNC(sithCamera_SetCameraFocus);
    J3D_HOOKFUNC(sithCamera_SetCameraFOV);
    J3D_HOOKFUNC(sithCamera_SetCameraPosition);
    J3D_HOOKFUNC(sithCamera_GetCameraPosition);
    J3D_HOOKFUNC(sithCamera_SetExtCameraOffset);
    J3D_HOOKFUNC(sithCamera_SetExtCameraLookOffset);
    J3D_HOOKFUNC(sithCamera_RestoreExtCamera);
    J3D_HOOKFUNC(sithCamera_GetPrimaryFocus);
    J3D_HOOKFUNC(sithCamera_GetSecondaryFocus);
    J3D_HOOKFUNC(sithCamera_Update);
    J3D_HOOKFUNC(sithCamera_RenderScene);
    J3D_HOOKFUNC(sithCamera_SetPOVShake);
    J3D_HOOKFUNC(sithCamera_SearchSectorInRadius);
    J3D_HOOKFUNC(sithCamera_SetCameraStateFlags);
    J3D_HOOKFUNC(sithCamera_GetCameraStateFlags);
}

void sithCamera_ResetGlobals(void)
{
    STD_ZEROMEM(&sithCamera_g_pCurCamera, sizeof(sithCamera_g_pCurCamera));
    STD_ZEROMEM(&sithCamera_g_bExtCameraLookMode, sizeof(sithCamera_g_bExtCameraLookMode));

    STD_ZEROMEM(&sithCamera_g_camSpot, sizeof(sithCamera_g_camSpot));
    STD_ZEROMEM(&sithCamera_g_aCameras, sizeof(sithCamera_g_aCameras));
    STD_ZEROMEM(&sithCamera_g_bCurCameraSet, sizeof(sithCamera_g_bCurCameraSet));
    STD_ZEROMEM(&sithCamera_g_stateFlags, sizeof(sithCamera_g_stateFlags));
    STD_ZEROMEM(&sithCamera_g_curCycleCamNum, sizeof(sithCamera_g_curCycleCamNum));
    STD_ZEROMEM(&sithCamera_g_vecCameraPosOffset, sizeof(sithCamera_g_vecCameraPosOffset));
    STD_ZEROMEM(&sithCamera_g_vecCameraAngleOffset, sizeof(sithCamera_g_vecCameraAngleOffset));
    STD_ZEROMEM(&sithCamera_g_cameraPosDelta, sizeof(sithCamera_g_cameraPosDelta));
    STD_ZEROMEM(&sithCamera_g_cameraAngleDelta, sizeof(sithCamera_g_cameraAngleDelta));
    STD_ZEROMEM(&sithCamera_g_orbCamOrient, sizeof(sithCamera_g_orbCamOrient));
    STD_ZEROMEM(&sithCamera_g_camLookSpot, sizeof(sithCamera_g_camLookSpot));
}

int sithCamera_Startup(void)
{
    SITH_ASSERTREL(sithCamera_bStartup == 0);

    // 1st person camera
    sithCamera_NewEntry(&sithCamera_g_aCameras[SITHCAMERA_INTCAMERANUM], 0, SITHCAMERA_INTERNAL, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);

    // 3rd person camera
    sithCamera_NewEntry(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], 0, SITHCAMERA_EXTERNAL, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);
    sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM].offset      = sithCamera_vecExtCameraOffset;
    sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM].interpSpeed = 0.35f;

    sithCamera_NewEntry(&sithCamera_g_aCameras[SITHCAMERA_CINEMACAMERANUM], 0, SITHCAMERA_CINEMATIC, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);
    sithCamera_g_aCameras[SITHCAMERA_CINEMACAMERANUM].interpSpeed = 1.0f;

    // Idle camera
    sithCamera_NewEntry(&sithCamera_g_aCameras[SITHCAMERA_IDLECAMERANUM], 0, SITHCAMERA_IDLE, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);
    sithCamera_NewEntry(&sithCamera_g_aCameras[SITHCAMERA_UNKNONWCAMERANUM], 0, SITHCAMERA_UNKNOWN_40, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);
    sithCamera_NewEntry(&sithCamera_g_aCameras[SITHCAMERA_ORBCAMERANUM], 0, SITHCAMERA_ORBITAL, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);

    sithCamera_bStartup = 1;
    return 1;
}

void sithCamera_Shutdown(void)
{
    if ( sithCamera_bStartup )
    {
        sithCamera_Close(); // Added: Make sure system is closed
        for ( size_t i = 0; i < STD_ARRAYLEN(sithCamera_g_aCameras); ++i )
        {
            sithCamera_FreeEntry(&sithCamera_g_aCameras[i]);
        }

        sithCamera_g_pCurCamera = NULL;
        sithCamera_bStartup     = 0;
    }
}

int J3DAPI sithCamera_Open(rdCanvas* pCanvas, float aspect)
{
    SITH_ASSERTREL(sithCamera_bStartup == 1);
    if ( sithCamera_bOpen )
    {
        // TODO: Maybe add debug log
        return 0;
    }

    sithCamera_SetCameraCanvas(&sithCamera_g_aCameras[SITHCAMERA_INTCAMERANUM], aspect, pCanvas);
    sithCamera_SetCameraCanvas(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], aspect, pCanvas);
    sithCamera_SetCameraCanvas(&sithCamera_g_aCameras[SITHCAMERA_CINEMACAMERANUM], aspect, pCanvas);
    sithCamera_SetCameraCanvas(&sithCamera_g_aCameras[SITHCAMERA_IDLECAMERANUM], aspect, pCanvas);
    sithCamera_SetCameraCanvas(&sithCamera_g_aCameras[SITHCAMERA_UNKNONWCAMERANUM], aspect, pCanvas);
    sithCamera_SetCameraCanvas(&sithCamera_g_aCameras[SITHCAMERA_ORBCAMERANUM], aspect, pCanvas);

    sithCamera_ResetAllCameras();
    sithCamera_Update(sithCamera_g_pCurCamera);

    sithCamera_bOpen = 1;
    return 1;
}

void sithCamera_Close(void)
{
    if ( sithCamera_bOpen )
    {
        sithCamera_g_pCurCamera = NULL;
        sithCamera_bOpen        = 0;
    }
}

bool sithCamera_IsOpen(void)
{
    return sithCamera_bOpen != 0;
}

void sithCamera_ResetAllCameras(void)
{
    sithCamera_g_bCurCameraSet = 0;
    sithCamera_g_stateFlags &= ~SITHCAMERA_STATE_CUTSCENE;

    sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_INTCAMERANUM], sithWorld_g_pCurrentWorld->pCameraFocusThing, NULL);
    sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], sithWorld_g_pCurrentWorld->pCameraFocusThing, NULL);
    sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_IDLECAMERANUM], sithWorld_g_pCurrentWorld->pCameraFocusThing, sithWorld_g_pCurrentWorld->pCameraFocusThing);
    sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_UNKNONWCAMERANUM], sithWorld_g_pCurrentWorld->pCameraFocusThing, sithWorld_g_pCurrentWorld->pCameraFocusThing);
    sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_ORBCAMERANUM], sithWorld_g_pCurrentWorld->pCameraFocusThing, sithWorld_g_pCurrentWorld->pCameraFocusThing);

    sithCamera_vecExtCameraOffset     = sithCamera_vecExtCameraDefaultOffset;
    sithCamera_vecExtCameraLookOffset = sithCamera_vecExtCameraDefaultLookOffset;

    // Set default FOV for 3rd person camera
    sithCamera_SetCameraFOV(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], SITHCAMERA_FOVDEFAULT);

    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        // Set 1st person camera
        sithCamera_SetCurrentCamera(&sithCamera_g_aCameras[SITHCAMERA_INTCAMERANUM]);
    }
    else if ( sithPlayer_g_pLocalPlayerThing )
    {
        if ( !sithCamera_g_pCurCamera )
        {
            // Set 3rd person camera
            sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], sithPlayer_g_pLocalPlayerThing, NULL);
            sithCamera_SetCurrentCamera(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM]);
        }
    }
}

SithCamera* J3DAPI sithCamera_New(SithCameraType type, uint32_t a2, float fov, float aspect, rdCanvas* pCanvas, SithThing* pPrimaryFocus, SithThing* pSecondaryFocus)
{
    SithCamera* pCamera = (SithCamera*)STDMALLOC(sizeof(SithCamera));
    if ( !pCamera )
    {
        SITHLOG_ERROR("Error allocating memory for camera.\n");
        return NULL;
    }

    if ( !sithCamera_NewEntry(pCamera, a2, type, fov, aspect, pCanvas, pPrimaryFocus, pSecondaryFocus) )
    {
        return NULL;
    }

    return pCamera;
}

int J3DAPI sithCamera_NewEntry(SithCamera* pCamera, uint32_t a2, SithCameraType type, float fov, float aspect, rdCanvas* pCanvas, SithThing* pPrimaryFocus, SithThing* pSecondaryFocus)
{
    pCamera->type                 = type;
    pCamera->unknown1             = a2;
    pCamera->fov                  = fov;
    pCamera->aspectRatio          = aspect;
    pCamera->pPrimaryFocusThing   = pPrimaryFocus;
    pCamera->pSecondaryFocusThing = pSecondaryFocus;
    pCamera->posInterpState       = -1;
    pCamera->lookInterpState      = -1;
    pCamera->focusInterpState     = -1;

    rdCamera_NewEntry(&pCamera->rdCamera, fov, 0, 1.0f / 64.0f, 64.0f, aspect); // 0.015625f - 1 / 64
    rdCamera_SetAttenuation(&pCamera->rdCamera, SITHCAMERA_ATTENUATIONMIN, SITHCAMERA_ATTENUATIONMAX);

    if ( pCanvas )
    {
        rdCamera_SetCanvas(&pCamera->rdCamera, pCanvas);
    }

    rdVector_Zero3(&pCamera->lookPos);
    rdVector_Zero3(&pCamera->lookPYR);

    pCamera->orient = rdroid_g_identMatrix34;

    rdVector_Zero3(&pCamera->offset);
    rdVector_Zero3(&pCamera->vecUnknown1);

    pCamera->bLookInterp = 0;
    pCamera->bPosInterp  = 0;
    return 1;
}

void J3DAPI sithCamera_Free(SithCamera* pCamera)
{
    if ( !pCamera )
    {
        SITHLOG_ERROR("Warning: attempt to free NULL camera ptr.\n");
        return;
    }

    sithCamera_FreeEntry(pCamera);
    STDFREE(pCamera);
}

void J3DAPI sithCamera_FreeEntry(SithCamera* pCamera)
{
    SITH_ASSERTREL(pCamera != NULL);
    rdCamera_FreeEntry(&pCamera->rdCamera);
}

void J3DAPI sithCamera_SetCameraCanvas(SithCamera* pCamera, float aspect, rdCanvas* pCanvas)
{
    rdCamera_SetCanvas(&pCamera->rdCamera, pCanvas);
    rdCamera_SetAspectRatio(&pCamera->rdCamera, aspect);
    rdCamera_SetAttenuation(&pCamera->rdCamera, SITHCAMERA_ATTENUATIONMIN, SITHCAMERA_ATTENUATIONMAX);
}

int J3DAPI sithCamera_SetCurrentCamera(SithCamera* pCamera)
{
    SITH_ASSERTREL(pCamera != NULL);

    sithCamera_g_pCurCamera   = pCamera;
    sithCamera_g_bCurCameraSet = 1;

    rdCamera_SetCurrent(&pCamera->rdCamera);
    switch ( pCamera->type )
    {
        case SITHCAMERA_INTERNAL:
            sithCamera_g_curCycleCamNum = 0;
            break;

        case SITHCAMERA_EXTERNAL:
            sithCamera_g_curCycleCamNum = 1;
            break;

        case SITHCAMERA_IDLE:
        {
            rdMatrix_Copy34(&sithCamera_idleCamOrient, &sithCamera_g_pCurCamera->pPrimaryFocusThing->orient);

            rdVector3 vecPYR = { 0 };
            vecPYR.y = -45.0f;
            rdMatrix_PostRotate34(&sithCamera_idleCamOrient, &vecPYR);
        } break;

        case SITHCAMERA_ORBITAL:
            sithCamera_g_curCycleCamNum = 2;
            break;

        default:
            break;
    }

    sithCamera_Update(sithCamera_g_pCurCamera);
    return 1;
}

void sithCamera_CycleCamera(void)
{
    if ( ++sithCamera_g_curCycleCamNum >= STD_ARRAYLEN(sithCamera_aCycleCamIndices) )
    {
        sithCamera_g_curCycleCamNum = 0;
    }

    sithCamera_SetCurrentCamera(&sithCamera_g_aCameras[sithCamera_aCycleCamIndices[sithCamera_g_curCycleCamNum]]);
}

int sithCamera_SetCurrentToCycleCamera(void)
{
    return sithCamera_SetCurrentCamera(&sithCamera_g_aCameras[sithCamera_aCycleCamIndices[sithCamera_g_curCycleCamNum]]);
}

// Found in dbg version
int J3DAPI SithCamera_sub_44ACCA(SithCamera* pTargetCamera, float time)
{
    SITH_ASSERTREL(pTargetCamera != NULL);
    SITH_ASSERTREL(time >= 0.0f);
    if ( sithCamera_g_pCurCamera && pTargetCamera->unknown1 < (unsigned int)sithCamera_g_pCurCamera->unknown1 )
    {
        return 0;
    }

    sithCamera_g_bCurCameraSet = 2;
    return 1;
}

void J3DAPI sithCamera_SetCameraFocus(SithCamera* pCamera, SithThing* pPrimaryFocusThing, SithThing* pSecondaryFocusThing)
{
    SITH_ASSERTREL(pCamera);
    if ( pPrimaryFocusThing && pCamera->pPrimaryFocusThing != pPrimaryFocusThing )
    {
        pCamera->pPrimaryFocusThing = pPrimaryFocusThing;
    }

    if ( pSecondaryFocusThing )
    {
        if ( pCamera->pSecondaryFocusThing != pSecondaryFocusThing )
        {
            pCamera->pSecondaryFocusThing = pSecondaryFocusThing;
        }
    }
}

void J3DAPI SithCamera_ZoomIn(SithCamera* pCamera)
{
    SITH_ASSERTREL(pCamera);
    pCamera->fov += 1.0f;
    if ( pCamera->fov > SITHCAMERA_FOVMAX )
    {
        pCamera->fov = SITHCAMERA_FOVMAX;
    }

    rdCamera_SetFOV(&pCamera->rdCamera, pCamera->fov);
}

void J3DAPI SithCamera_ZoomOut(SithCamera* pCamera)
{
    SITH_ASSERTREL(pCamera);
    pCamera->fov -= 1.0f;
    if ( pCamera->fov < SITHCAMERA_FOVMIN )
    {
        pCamera->fov = SITHCAMERA_FOVMIN;
    }

    rdCamera_SetFOV(&pCamera->rdCamera, pCamera->fov);
}

void J3DAPI sithCamera_SetCameraFOV(SithCamera* pCamera, float fov)
{
    SITH_ASSERTREL(pCamera);
    if ( fov >= SITHCAMERA_FOVMIN && fov <= SITHCAMERA_FOVMAX )
    {
        pCamera->fov = fov;
        rdCamera_SetFOV(&pCamera->rdCamera, pCamera->fov);
    }
}

void J3DAPI sithCamera_SetCameraPosition(SithCamera* pCamera, const rdVector3* pPos)
{
    SITH_ASSERTREL(pCamera);
    pCamera->pos = *pPos;
    pCamera->posInterpState   = 0;
    pCamera->focusInterpState = 0;
}


void J3DAPI sithCamera_GetCameraPosition(const SithCamera* pCamera, rdVector3* pDstPos)
{
    SITH_ASSERTREL(pCamera);
    *pDstPos = pCamera->orient.dvec;
}

void J3DAPI sithCamera_SetExtCameraOffset(const rdVector3* pOffset)
{
    if ( sithCamera_g_pCurCamera->type == SITHCAMERA_EXTERNAL )
    {
        sithCamera_vecExtCameraOffset  = *pOffset;
        sithCamera_bUpdateCameraOffset = 1;
    }
}

void J3DAPI sithCamera_SetExtCameraLookOffset(const rdVector3* pOffset)
{
    if ( sithCamera_g_pCurCamera->type == SITHCAMERA_EXTERNAL )
    {
        sithCamera_vecExtCameraPrevLookOffset = sithCamera_vecExtCameraLookOffset;

        sithCamera_secLookOffsetTranslateDelta = 0.5f;
        sithCamera_vecExtCameraLookOffset      = *pOffset;
    }
}

void sithCamera_RestoreExtCamera(void)
{
    if ( sithCamera_g_pCurCamera->type == SITHCAMERA_EXTERNAL )
    {
        sithCamera_vecExtCameraOffset     = sithCamera_vecExtCameraDefaultOffset;
        sithCamera_vecExtCameraLookOffset = sithCamera_vecExtCameraDefaultLookOffset;

        sithCamera_g_pCurCamera->interpSpeed = 0.34999999f;
        sithCamera_bUpdateCameraOffset = 0;

        if ( !sithCamera_g_bExtCameraLookMode )
        {
            sithCamera_bFocusThingInvisible = 0;
        }
    }
}

SithThing* J3DAPI sithCamera_GetPrimaryFocus(const SithCamera* pCamera)
{
    SITH_ASSERTREL(pCamera != NULL);
    return pCamera->pPrimaryFocusThing;
}

SithThing* J3DAPI sithCamera_GetSecondaryFocus(const SithCamera* pCamera)
{
    SITH_ASSERTREL(pCamera != NULL);
    return pCamera->pSecondaryFocusThing;
}

void J3DAPI sithCamera_Update(SithCamera* pCamera)
{
    double v1;
    double zDist;
    double v3;
    double v4;
    double v5;
    double v6;
    SithSector* pNewSector;
    double distToLook;
    double v9;
    double v10;
    float v12;
    float v13;
    float v14;
    float v15;
    float v16;
    float v17;
    float v18;
    float v19;
    float v20;
    float v21;
    float v22;
    float v23;
    float v24;
    float v25;
    SithCameraType type;
    rdVector4 startColor;
    rdVector4 endColor;
    float v30;
    rdVector3 lookDir;
    rdVector4 colorStart;
    rdVector4 colorEnd;
    float v34;
    rdVector3 newLookOffset;
    rdVector3 v36;
    float secLookOffsetDeltaTime;
    float zDistf;
    float v39;
    rdMatrix34 newOrient;
    rdVector3 newPYR;
    float zDelta;
    float secFrameTime;
    rdVector3 lookPos;
    int v45;
    float xDelta;
    float angleDelta;
    float yDelta;
    rdVector3 pos;
    rdVector3 newPos;
    rdVector3 newLookPos;
    float posDelta;
    SithThing* pThing2;
    float iterpDelta;
    int curWeaponId;
    rdVector3 curCamPos;
    float interpSpeed;
    float interpSpeedHalf;
    SithThing* pThing1;
    rdVector3 curLookPos;
    SithWorld* pWorld;
    rdVector3 vecPYR;

    secFrameTime = sithTime_g_frameTimeFlex;
    angleDelta = sithTime_g_frameTimeFlex * sithCamera_g_cameraAngleDelta;
    posDelta = sithTime_g_frameTimeFlex * sithCamera_g_cameraPosDelta;

    interpSpeed = pCamera->interpSpeed;
    interpSpeedHalf = interpSpeed / 2.0f;
    v45 = 0;
    pWorld = sithWorld_g_pCurrentWorld;

    SITH_ASSERTREL(pCamera != NULL);
    pThing1 = pCamera->pPrimaryFocusThing;
    pThing2 = pCamera->pSecondaryFocusThing;
    type = pCamera->type;

    if ( pCamera->type > SITHCAMERA_EXTERNAL )
    {
        switch ( type )
        {
            case SITHCAMERA_CINEMATIC:
                SITH_ASSERTREL(pThing1 != NULL);
                SITH_ASSERTREL(pThing2 != NULL);
                SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing1));
                SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing2));

                memcpy(&newPos, &pThing1->pos, sizeof(newPos));
                newPos.x = newPos.x + sithCamera_g_vecCameraPosOffset.x;
                newPos.y = newPos.y + sithCamera_g_vecCameraPosOffset.y;
                newPos.z = newPos.z + sithCamera_g_vecCameraPosOffset.z;

                memcpy(&lookPos, &pThing2->pos, sizeof(lookPos));

                if ( pCamera->bPosInterp )
                {
                    switch ( pCamera->posInterpState )
                    {
                        case 0xFFFFFFFF:
                        LABEL_145:
                            memcpy(&pCamera->pos, &newPos, sizeof(pCamera->pos));
                            pCamera->posInterpState = 0;
                            pCamera->secPosInterpDeltaTime = 0.0f;
                            break;

                        case 1:
                            pCamera->secPosInterpDeltaTime = pCamera->secPosInterpDeltaTime + secFrameTime;
                            v9 = interpSpeed - pCamera->secPosInterpDeltaTime;
                            iterpDelta = v9;
                            if ( v9 <= 0.0f )
                            {
                                goto LABEL_145;
                            }

                            iterpDelta = iterpDelta / interpSpeed;
                            xDelta = (newPos.x - pCamera->pos.x) * iterpDelta;
                            yDelta = (newPos.y - pCamera->pos.y) * iterpDelta;
                            zDelta = (newPos.z - pCamera->pos.z) * iterpDelta;
                            newPos.x = newPos.x - xDelta;
                            newPos.y = newPos.y - yDelta;
                            newPos.z = newPos.z - zDelta;
                            break;

                        case 0:
                            if ( newPos.x == pCamera->pos.x && newPos.y == pCamera->pos.y && newPos.z == pCamera->pos.z )
                            {
                                memcpy(&pCamera->pos, &newPos, sizeof(pCamera->pos));
                                pCamera->secPosInterpDeltaTime = 0.0f;
                            }
                            else
                            {
                                pCamera->posInterpState = 1;
                                pCamera->secPosInterpDeltaTime = 0.0f;
                                memcpy(&newPos, &pCamera->pos, sizeof(newPos));
                            }

                            break;
                    }
                }
                else
                {
                    memcpy(&pCamera->pos, &newPos, sizeof(pCamera->pos));
                }

                if ( pCamera->bLookInterp )
                {
                    if ( pCamera->lookInterpState == -1 )
                    {
                        goto LABEL_159;
                    }

                    if ( pCamera->lookInterpState != 1 )
                    {
                        if ( !pCamera->lookInterpState )
                        {
                            if ( lookPos.x == pCamera->curLookInterp.x && lookPos.y == pCamera->curLookInterp.y && lookPos.z == pCamera->curLookInterp.z )
                            {
                                memcpy(&pCamera->curLookInterp, &lookPos, sizeof(pCamera->curLookInterp));
                            }
                            else
                            {
                                pCamera->lookInterpState = 1;
                                pCamera->secLookInterpDeltaTime = 0.0f;
                                memcpy(&lookPos, &pCamera->curLookInterp, sizeof(lookPos));
                            }
                        }

                        goto LABEL_171;
                    }

                    pCamera->secLookInterpDeltaTime = pCamera->secLookInterpDeltaTime + secFrameTime;
                    v10 = interpSpeed - pCamera->secLookInterpDeltaTime;
                    iterpDelta = v10;
                    if ( v10 <= 0.0f )
                    {
                    LABEL_159:
                        memcpy(&pCamera->curLookInterp, &lookPos, sizeof(pCamera->curLookInterp));
                        pCamera->lookInterpState = 0;
                        pCamera->secLookInterpDeltaTime = 0.0f;
                    }
                    else
                    {
                        iterpDelta = iterpDelta / interpSpeed;
                        xDelta = (lookPos.x - pCamera->curLookInterp.x) * iterpDelta;
                        yDelta = (lookPos.y - pCamera->curLookInterp.y) * iterpDelta;
                        zDelta = (lookPos.z - pCamera->curLookInterp.z) * iterpDelta;
                        lookPos.x = lookPos.x - xDelta;
                        lookPos.y = lookPos.y - yDelta;
                        lookPos.z = lookPos.z - zDelta;
                    }
                }
                else
                {
                    memcpy(&pCamera->curLookInterp, &lookPos, sizeof(pCamera->curLookInterp));
                }

            LABEL_171:
                rdMatrix_LookAt(&pCamera->orient, &newPos, &lookPos, 0.0f);
                rdMatrix_PostRotate34(&pCamera->orient, &sithCamera_g_vecCameraAngleOffset);
                rdMatrix_Normalize34(&pCamera->orient);
                pCamera->pSector = sithCamera_SearchSectorInRadius(0, pThing1->pInSector, &pThing1->pos, &pCamera->orient.dvec, 0.0099999998f, 0x2200);
                goto LABEL_191;

            case SITHCAMERA_IDLE:
                SITH_ASSERTREL(pThing1 != NULL);
                SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing1));
                rdMatrix_TransformVector34(&pos, &sithCamera_idleCamOffset, &sithCamera_idleCamOrient);

                lookPos.x = 0.0f;
                lookPos.y = 0.0f;
                lookPos.z = 0.050000001f;

                lookPos.x = pThing1->pos.x + 0.0f;
                lookPos.y = pThing1->pos.y + 0.0f;
                lookPos.z = pThing1->pos.z + 0.050000001f;

                pos.x = pos.x + lookPos.x;
                pos.y = pos.y + lookPos.y;
                pos.z = pos.z + lookPos.z;
                rdMatrix_LookAt(&pCamera->orient, &pos, &lookPos, 0.0f);

                pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &lookPos, 0.0f);
                pCamera->pSector = sithCamera_SearchSectorInRadius(0, pCamera->pSector, &lookPos, &pCamera->orient.dvec, 0.0099999998f, 0x200);

                lookDir.x = pCamera->orient.dvec.x - lookPos.x;
                lookDir.y = pCamera->orient.dvec.y - lookPos.y;
                lookDir.z = pCamera->orient.dvec.z - lookPos.z;
                distToLook = rdVector_Len3(&lookDir);
                v30 = distToLook;
                if ( distToLook >= 0.098164998f )
                {
                    if ( sithCamera_bFocusThingInvisible && !sithPlayerActions_IsInvisible() )
                    {
                        memset(&endColor, 0, 12);
                        endColor.alpha = 1.0f;
                        memset(&startColor, 0, sizeof(startColor));
                        sithAnimate_StartThingFadeAnim(pThing1, &startColor, &endColor, 0.2f, (SithAnimateFlags)0);
                        sithCamera_bFocusThingInvisible = 0;
                    }
                }

                else if ( !sithCamera_bFocusThingInvisible && !sithPlayerActions_IsInvisible() )
                {
                    memset(&endColor, 0, sizeof(endColor));
                    memset(&startColor, 0, 12);
                    startColor.alpha = 1.0f;
                    sithAnimate_StartThingFadeAnim(pThing1, &startColor, &endColor, 0.2f, (SithAnimateFlags)0);
                    sithCamera_bFocusThingInvisible = 1;
                }

                vecPYR.x = 0.0f;
                vecPYR.y = sithTime_g_frameTimeFlex * 8.0f;
                vecPYR.z = 0.0f;
                rdMatrix_PostRotate34(&sithCamera_idleCamOrient, &vecPYR);
                rdMatrix_Normalize34(&sithCamera_idleCamOrient);
                goto LABEL_191;

            case SITHCAMERA_UNKNOWN_40:
                rdVector_Normalize3Acc(&sithCamera_vecOffsetCamType40);
                pCamera->orient.lvec.x = -sithCamera_vecOffsetCamType40.x;
                pCamera->orient.lvec.y = -sithCamera_vecOffsetCamType40.y;
                pCamera->orient.lvec.z = -sithCamera_vecOffsetCamType40.z;

                pCamera->orient.rvec.x = pCamera->orient.lvec.y * 1.0f - pCamera->orient.lvec.z * 0.0f;
                pCamera->orient.rvec.y = pCamera->orient.lvec.z * 0.0f - pCamera->orient.lvec.x * 1.0f;
                pCamera->orient.rvec.z = pCamera->orient.lvec.x * 0.0f - pCamera->orient.lvec.y * 0.0f;

                pCamera->orient.uvec.x = pCamera->orient.rvec.y * pCamera->orient.lvec.z - pCamera->orient.rvec.z * pCamera->orient.lvec.y;
                pCamera->orient.uvec.y = pCamera->orient.rvec.z * pCamera->orient.lvec.x - pCamera->orient.rvec.x * pCamera->orient.lvec.z;
                pCamera->orient.uvec.z = pCamera->orient.rvec.x * pCamera->orient.lvec.y - pCamera->orient.rvec.y * pCamera->orient.lvec.x;
                rdMatrix_Normalize34(&pCamera->orient);

                pCamera->orient.dvec.x = sithCamera_vecOffsetCamType40.x * 0.2f;
                pCamera->orient.dvec.y = sithCamera_vecOffsetCamType40.y * 0.2f;
                pCamera->orient.dvec.z = sithCamera_vecOffsetCamType40.z * 0.2f;
                rdMatrix_PostTranslate34(&pCamera->orient, &pThing1->pos);

                pCamera->pSector = sithCamera_SearchSectorInRadius(0, pThing1->pInSector, &pThing1->pos, &pCamera->orient.dvec, 0.01f, 0x2200);
                goto LABEL_191;

            case SITHCAMERA_ORBITAL:
                SITH_ASSERTREL(pThing1 != NULL);
                SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing1));
                memcpy(&pCamera->orient, &sithCamera_g_orbCamOrient, sizeof(pCamera->orient));
                rdMatrix_PostTranslate34(&pCamera->orient, &pThing1->pos);
                pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &pCamera->orient.dvec, 0.01f);
                goto LABEL_191;

            case SITHCAMERA_UNKNOWN_100:
                goto LABEL_35;

            default:
                goto LABEL_191;
        }
    }

    if ( type != SITHCAMERA_EXTERNAL )
    {
        if ( type == SITHCAMERA_INTERNAL )
        {
            SITH_ASSERTREL(pThing1 != NULL);
            SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing1));
            memcpy(&pCamera->orient, &pThing1->orient, sizeof(pCamera->orient));
            if ( pThing1->moveType == SITH_MT_PATH && pThing1->renderData.paJointMatrices )
            {
                memcpy(&pCamera->orient, pThing1->renderData.paJointMatrices, sizeof(pCamera->orient));
            }
            else
            {
                if ( pThing1->type == SITH_THING_ACTOR || pThing1->type == SITH_THING_PLAYER )
                {
                    memcpy(&newPYR, &pThing1->thingInfo.actorInfo.headPYR, sizeof(newPYR));
                }
                else
                {
                    memset(&newPYR, 0, sizeof(newPYR));
                }

                if ( pThing1->moveType == SITH_MT_PHYSICS )
                {
                    v1 = (rdVector_Dot3(&pThing1->orient.rvec, &pThing1->moveInfo.physics.velocity)) * 5.0f;

                    v39 = v1;
                    if ( v1 < -8.0f )
                    {
                        v24 = -8.0f;
                    }
                    else
                    {
                        if ( v39 > 8.0f )
                        {
                            v25 = 8.0f;
                        }
                        else
                        {
                            v25 = v39;
                        }

                        v24 = v25;
                    }

                    newPYR.roll = v24;
                }

                if ( pThing1 == sithPlayer_g_pLocalPlayerThing )
                {
                    newPYR.x = newPYR.x + sithCamera_g_vecCameraAngleOffset.x;
                    newPYR.y = newPYR.y + sithCamera_g_vecCameraAngleOffset.y;
                    newPYR.z = newPYR.z + sithCamera_g_vecCameraAngleOffset.z;
                }

                rdMatrix_PreRotate34(&pCamera->orient, &newPYR);
                rdMatrix_PostTranslate34(&pCamera->orient, &pThing1->pos);
                if ( pThing1->type == SITH_THING_ACTOR || pThing1->type == SITH_THING_PLAYER )
                {
                    rdMatrix_PreTranslate34(&pCamera->orient, &pThing1->thingInfo.actorInfo.eyeOffset);
                    if ( pThing1 == sithPlayer_g_pLocalPlayerThing )
                    {
                        rdMatrix_PreTranslate34(&pCamera->orient, &sithCamera_g_vecCameraPosOffset);
                    }
                }

                rdMatrix_Normalize34(&pCamera->orient);
            }

            pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &pCamera->orient.dvec, 0.0099999998f);
        }

        goto LABEL_191;
    }

LABEL_35:

    // Here cam type should be SITHCAMERA_EXTERNAL or SITHCAMERA_UNKNOWN_100
    SITH_ASSERTREL(pThing1 != NULL);
    SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing1));

        // What's the point of this check?
    if ( pCamera->type != SITHCAMERA_EXTERNAL && pCamera->type != SITHCAMERA_UNKNOWN_100 )
    {
        goto LABEL_102;
    }

    if ( (pThing1->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        sithCamera_g_bExtCameraLookMode = 0;
        pThing1->alpha = 1.0f;
        sithCamera_RestoreExtCamera();
        rdMatrix_LookAt(&pCamera->orient, &pCamera->lookPos, &pThing1->pos, 0.0f);

        if ( pThing1->pInSector )
        {
            pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &pCamera->lookPos, 0.0f);
        }

        goto LABEL_191;
    }

    memcpy(&curLookPos, &pCamera->lookPos, sizeof(curLookPos));
    if ( pThing1->type == SITH_THING_ACTOR || pThing1->type == SITH_THING_PLAYER )
    {
        memcpy(&newPYR, &pThing1->thingInfo.actorInfo.headPYR, sizeof(newPYR));
    }
    else
    {
        memset(&newPYR, 0, sizeof(newPYR));
    }

    if ( pThing1 == sithPlayer_g_pLocalPlayerThing )
    {
        newPYR.x = newPYR.x + sithCamera_g_vecCameraAngleOffset.pitch;
        newPYR.y = newPYR.y + sithCamera_g_vecCameraAngleOffset.yaw;
        newPYR.z = newPYR.z + sithCamera_g_vecCameraAngleOffset.roll;
    }

    if ( pThing1 != sithPlayer_g_pLocalPlayerThing || sithCamera_g_bExtCameraLookMode || sithCamera_bUpdateCameraOffset )
    {
    LABEL_75:
        memcpy(&pCamera->offset, &sithCamera_vecExtCameraOffset, sizeof(pCamera->offset));
        goto LABEL_76;
    }

    if ( pThing1->moveStatus == SITHPLAYERMOVE_SWIMIDLE
        || (pThing1->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0
        || (pThing1->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
    {
        if ( pThing1->moveStatus == SITHPLAYERMOVE_SWIMIDLE && (pThing1->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
            memcpy(&pCamera->offset, &sithCamera_vecCameraOffsetClimbDown, sizeof(pCamera->offset));
            goto LABEL_76;
        }

        goto LABEL_75;
    }

    if ( (pThing1->moveInfo.physics.flags & SITH_PF_JEEP) == 0 || pThing1->moveStatus == SITHPLAYERMOVE_JEEP_BOARDING )
    {
        switch ( pThing1->moveStatus )
        {
            case SITHPLAYERMOVE_CLIMBING_UP:
                memcpy(&pCamera->offset, &sithCamera_vecCameraOffsetClimbingUp, sizeof(pCamera->offset));
                break;

            case SITHPLAYERMOVE_CLIMBING_DOWN:
                memcpy(&pCamera->offset, &sithCamera_vecCameraOffsetClimbDown, sizeof(pCamera->offset));
                break;

            case SITHPLAYERMOVE_CLIMBING_LEFT:
                memcpy(&pCamera->offset, &sithCamera_vecCameraOffsetClimbingLeft, sizeof(pCamera->offset));
                break;

            case SITHPLAYERMOVE_CLIMBING_RIGHT:
                memcpy(&pCamera->offset, &sithCamera_vecCameraOffsetClimbingRight, sizeof(pCamera->offset));
                break;

            case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
                memcpy(&pCamera->offset, &sithCamera_vecCameraOffsetSlideForward, sizeof(pCamera->offset));
                break;

            case SITHPLAYERMOVE_JUMPROLLBACK:
                memcpy(&pCamera->offset, &sithCamera_vecCameraOffsetJumpRollback, sizeof(pCamera->offset));
                break;

            case SITHPLAYERMOVE_SLIDEDOWNBACK:
                memcpy(&pCamera->offset, &sithCamera_vecCameraOffsetSlideBackward, sizeof(pCamera->offset));
                break;

            default:
                curWeaponId = sithInventory_GetCurrentWeapon(pThing1);
                if ( curWeaponId == SITHWEAPON_NO_WEAPON || curWeaponId == SITHWEAPON_SATCHEL || curWeaponId == SITHWEAPON_ZIPPO )
                {
                    goto LABEL_75;
                }

                memcpy(&pCamera->offset, &sithCamera_vecCameraOffsetWeaponDrawn, sizeof(pCamera->offset));
                break;
        }
    }
    else
    {
        interpSpeed = 0.2f;
        memcpy(&pCamera->offset, &sithCamera_vecCameraOffsetJeep, sizeof(pCamera->offset));
    }

LABEL_76:
    memcpy(&newOrient, &pThing1->orient, sizeof(newOrient));

    switch ( pCamera->focusInterpState )
    {
        case 0xFFFFFFFF:
        LABEL_77:
            memcpy(&pCamera->focusPos, &pThing1->pos, sizeof(pCamera->focusPos));
            pCamera->focusInterpState = 0;
            break;

        case 0:
            zDist = pCamera->focusPos.z - pThing1->pos.z;
            zDistf = zDist;
            if ( zDist > 0.0099999998f && pThing1->collide.movesize * 10.0f >= zDistf )
            {
                pCamera->focusInterpState = 1;
                pCamera->focusPos.x = pThing1->pos.x;
                pCamera->focusPos.y = pThing1->pos.y;
                pCamera->secFocusInterpDeltaTime = secFrameTime;

                iterpDelta = (interpSpeedHalf - secFrameTime) / interpSpeedHalf;
                zDelta = (pThing1->pos.z - pCamera->focusPos.z) * iterpDelta;
                pCamera->focusPos.z = pThing1->pos.z - zDelta;
            }
            else
            {
                memcpy(&pCamera->focusPos, &pThing1->pos, sizeof(pCamera->focusPos));
            }

            break;

        case 1:
            pCamera->secFocusInterpDeltaTime = pCamera->secFocusInterpDeltaTime + secFrameTime;
            v3 = interpSpeedHalf - pCamera->secFocusInterpDeltaTime;
            iterpDelta = v3;
            if ( v3 > 0.0f )
            {
                if ( pCamera->focusPos.z != pThing1->pos.z )
                {
                    pCamera->secFocusInterpDeltaTime = secFrameTime;
                    iterpDelta = interpSpeedHalf - pCamera->secFocusInterpDeltaTime;
                    pCamera->focusPos.x = pThing1->pos.x;
                    pCamera->focusPos.y = pThing1->pos.y;
                }

                iterpDelta = iterpDelta / interpSpeedHalf;
                zDelta = (pThing1->pos.z - pCamera->focusPos.z) * iterpDelta;
                pCamera->focusPos.z = pThing1->pos.z - zDelta;
                break;
            }

            goto LABEL_77;
    }


    // Offset camera new pos
    memcpy(&newPos, &pCamera->focusPos, sizeof(newPos));
    newPos.x = sithCamera_g_vecCameraPosOffset.x * 3.0f + newPos.x;
    newPos.y = sithCamera_g_vecCameraPosOffset.y * 3.0f + newPos.y;
    newPos.z = sithCamera_g_vecCameraPosOffset.z * 3.0f + newPos.z;

    // Set camera new orient
    rdMatrix_PreRotate34(&newOrient, &newPYR);
    rdMatrix_PostTranslate34(&newOrient, &newPos);
    memcpy(&pCamera->orient, &newOrient, sizeof(pCamera->orient));

    // Set orient offset
    if ( sithCamera_secLookOffsetTranslateDelta == 0.0f )
    {
        rdMatrix_PreTranslate34(&newOrient, &sithCamera_vecExtCameraLookOffset);
    }
    else
    {
        // Translate orient offset
        // This happens when switching to first person look
        v4 = sithCamera_secLookOffsetTranslateDelta - secFrameTime;
        secLookOffsetDeltaTime = v4;
        if ( v4 > 0.0f )
        {
            secLookOffsetDeltaTime = secLookOffsetDeltaTime / 0.5f;// half time?
        }

        v5 = sithCamera_secLookOffsetTranslateDelta - secFrameTime;
        sithCamera_secLookOffsetTranslateDelta = v5;

        if ( v5 >= 0.0f )
        {
            v36.x = (sithCamera_vecExtCameraLookOffset.x - sithCamera_vecExtCameraPrevLookOffset.x) * (1.0f - secLookOffsetDeltaTime);
            v36.y = (sithCamera_vecExtCameraLookOffset.z - sithCamera_vecExtCameraPrevLookOffset.y) * (1.0f - secLookOffsetDeltaTime);
            v36.z = (sithCamera_vecExtCameraLookOffset.z - sithCamera_vecExtCameraPrevLookOffset.z) * (1.0f - secLookOffsetDeltaTime);
            newLookOffset.x = sithCamera_vecExtCameraPrevLookOffset.x + v36.x;
            newLookOffset.y = sithCamera_vecExtCameraPrevLookOffset.y + v36.y;
            newLookOffset.z = sithCamera_vecExtCameraPrevLookOffset.z + v36.z;
            rdMatrix_PreTranslate34(&newOrient, &newLookOffset);
        }
        else
        {
            rdMatrix_PreTranslate34(&newOrient, &sithCamera_vecExtCameraLookOffset);
            sithCamera_secLookOffsetTranslateDelta = 0.0f;
        }
    }

    // Offset camera orient
    rdMatrix_PreTranslate34(&pCamera->orient, &pCamera->offset);

    // Find camera new sector
    pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &newOrient.dvec, 0.0f);
    memcpy(&newLookPos, &pCamera->orient.dvec, sizeof(newLookPos));

    if ( sithCamera_vecExtCameraLookOffset.x == sithCamera_vecExtCameraDefaultLookOffset.x
        && sithCamera_vecExtCameraLookOffset.y == sithCamera_vecExtCameraDefaultLookOffset.y
        && sithCamera_vecExtCameraLookOffset.z == sithCamera_vecExtCameraDefaultLookOffset.z )
    {
        pCamera->pSector = sithCamera_SearchSectorInRadius(0, pCamera->pSector, &newOrient.dvec, &pCamera->orient.dvec, 0.0099999998f, 0x200);
    }
    else
    {
        pCamera->pSector = sithCollision_FindSectorInRadius(pCamera->pSector, &newOrient.dvec, &pCamera->orient.dvec, 0.0f);
        v45 = 1;
    }

    memcpy(&curCamPos, &pCamera->orient.dvec, sizeof(curCamPos));

LABEL_102:
    switch ( pCamera->posInterpState )
    {
        case -1:
            memcpy(&pCamera->pos, &curCamPos, sizeof(pCamera->pos));
            memcpy(&newPos, &curCamPos, sizeof(newPos));
            pCamera->posInterpState = 0;
            pCamera->secPosInterpDeltaTime = 0.0f;
            break;

        case 1:
            memcpy(&newPos, &curCamPos, sizeof(newPos));
            pCamera->secPosInterpDeltaTime = pCamera->secPosInterpDeltaTime + secFrameTime;
            v6 = interpSpeed - pCamera->secPosInterpDeltaTime;
            iterpDelta = v6;
            if ( v6 > 0.0f )
            {
                if ( newPos.x != pCamera->pos.x || newPos.y != pCamera->pos.y || newPos.z != pCamera->pos.z )
                {
                    pCamera->secPosInterpDeltaTime = secFrameTime;
                    iterpDelta = interpSpeed - pCamera->secPosInterpDeltaTime;
                    memcpy(&pCamera->pos, &curLookPos, sizeof(pCamera->pos));
                }

                iterpDelta = iterpDelta / interpSpeed;
                xDelta = (newPos.x - pCamera->pos.x) * iterpDelta;
                yDelta = (newPos.y - pCamera->pos.y) * iterpDelta;
                zDelta = (newPos.z - pCamera->pos.z) * iterpDelta;

                if ( pThing1->moveStatus != SITHPLAYERMOVE_PULLINGUP )
                {
                    newPos.x = newPos.x - xDelta;
                    newPos.y = newPos.y - yDelta;
                }

                newPos.z = newPos.z - zDelta;
            }
            else
            {
                memcpy(&pCamera->pos, &newPos, sizeof(pCamera->pos));
                pCamera->posInterpState = 0;
            }

            break;

        case 0:
            memcpy(&newPos, &curCamPos, sizeof(newPos));

            if ( newPos.x == pCamera->pos.x && newPos.y == pCamera->pos.y && newPos.z == pCamera->pos.z )
            {
                memcpy(&pCamera->pos, &curCamPos, sizeof(pCamera->pos));
            }
            else
            {
                pCamera->posInterpState = 1;
                pCamera->secPosInterpDeltaTime = secFrameTime;
                iterpDelta = (interpSpeed - secFrameTime) / interpSpeed;
                xDelta = (newPos.x - pCamera->pos.x) * iterpDelta;
                yDelta = (newPos.y - pCamera->pos.y) * iterpDelta;
                zDelta = (newPos.z - pCamera->pos.z) * iterpDelta;

                if ( pThing1->moveStatus != SITHPLAYERMOVE_PULLINGUP )
                {
                    newPos.x = newPos.x - xDelta;
                    newPos.y = newPos.y - yDelta;
                }

                newPos.z = newPos.z - zDelta;
            }

            break;
    }

// Found in debug build
// Code in if scope is always executed, there might be special preprocessor macro here
#if 1
    memcpy(&newLookPos, &newOrient.dvec, sizeof(newLookPos));
    pCamera->pSector = sithCollision_FindSectorInRadius(pCamera->pSector, &pCamera->orient.dvec, &newLookPos, 0.0f);
    if ( v45 )
    {
        pNewSector = sithCollision_FindSectorInRadius(pCamera->pSector, &newOrient.dvec, &newPos, 0.0f);
    }
    else
    {
        pNewSector = sithCamera_SearchSectorInRadius(0, pCamera->pSector, &newLookPos, &newPos, 0.0099999998f, 0x200);
    }

    pCamera->pSector = pNewSector;

    // Set camera orient based on new pos and look pos
    rdMatrix_LookAt(&pCamera->orient, &newPos, &newLookPos, 0.0f);
    rdMatrix_Normalize34(&pCamera->orient);

    memcpy(&sithCamera_g_camSpot, &newPos, sizeof(sithCamera_g_camSpot));
    memcpy(&sithCamera_g_camLookSpot, &newLookPos, sizeof(sithCamera_g_camLookSpot));

    // Now check if camera is too close to focus thing, and in this cane make focused thing transparent
    lookDir.x = newLookPos.x - newPos.x;
    lookDir.y = newLookPos.y - newPos.y;
    lookDir.z = newLookPos.z - newPos.z;
    distToLook = rdVector_Len3(&lookDir);

    v34 = distToLook;
    if ( distToLook < 0.11f )
    {
        // Dist too close, translate focus thing to invisible
        if ( !sithCamera_bFocusThingInvisible && !sithPlayerActions_IsInvisible() )
        {
            memset(&colorEnd, 0, sizeof(colorEnd));
            memset(&colorStart, 0, sizeof(rdVector3));
            colorStart.alpha = 1.0f;

            sithAnimate_StartThingFadeAnim(pThing1, &colorStart, &colorEnd, 0.2f, (SithAnimateFlags)0);
            sithCamera_bFocusThingInvisible = 1;
        }
    }
    else
    {
    #ifndef J3D_DEBUG
        // Following code was found only in release build but not in debug build
        sithCamera_secFocusThingFadeInTime = sithCamera_secFocusThingFadeInTime + sithTime_g_frameTimeFlex;
    #endif

        if ( sithCamera_bFocusThingInvisible && !sithPlayerActions_IsInvisible() && !sithCamera_g_bExtCameraLookMode )
        {
            // Translate focus thing to visible as cam is far away enough
            memset(&colorEnd, 0, sizeof(rdVector3));
            colorEnd.alpha = 1.0f;
            memset(&colorStart, 0, sizeof(colorStart));

            sithAnimate_StartThingFadeAnim(pThing1, &colorStart, &colorEnd, 0.2f, (SithAnimateFlags)0);
            sithCamera_bFocusThingInvisible = 0;
        }

    #ifndef J3D_DEBUG
        // Following code was found only in release build in debug build
        if ( pThing1->alpha != 1.0f && !sithCamera_g_bExtCameraLookMode )
        {
            if ( (pThing1->moveStatus == SITHPLAYERMOVE_STILL || pThing1->moveStatus == SITHPLAYERMOVE_SWIMIDLE)
                && sithCamera_secFocusThingFadeInTime > 0.5
                && !sithPlayerActions_IsInvisible()
                && sithInventory_GetCurrentWeapon(pThing1) != SITHWEAPON_IMP2 ) // Note: Note if pThing1 is not Actor or Player the call to sithInventory_GetCurrentWeapon function crashes app
            {
                sithCamera_secFocusThingFadeInTime = 0.0;
                pThing1->alpha                     = 1.0;
            }
        }
    #endif
    }
#elif 0
    // TODO: [DEAD] This code was found in debug build and was never executed
    //       It's buggy code, don't know what's it's real purpose
    pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &pCamera->orient.dvec, 0.0);
    rdMatrix_LookAt(&pCamera->orient, &pCamera->orient.dvec, &newPos, 0.0);
    memcpy(&pCamera->curLookInterp, &newPos, sizeof(pCamera->curLookInterp));
#endif

LABEL_191:
    // Update global pos & rot delta states
    memcpy(&pCamera->lookPos, &pCamera->orient.dvec, sizeof(pCamera->lookPos));
    rdMatrix_ExtractAngles34(&pCamera->orient, &pCamera->lookPYR);
    if ( sithCamera_g_vecCameraPosOffset.x <= 0.0f )
    {
        if ( sithCamera_g_vecCameraPosOffset.x < 0.0f )
        {
            if ( sithCamera_g_vecCameraPosOffset.x + posDelta < 0.0f )
            {
                v22 = sithCamera_g_vecCameraPosOffset.x + posDelta;
            }
            else
            {
                v22 = 0.0f;
            }

            sithCamera_g_vecCameraPosOffset.x = v22;
        }
    }
    else
    {
        if ( sithCamera_g_vecCameraPosOffset.x - posDelta > 0.0f )
        {
            v23 = sithCamera_g_vecCameraPosOffset.x - posDelta;
        }
        else
        {
            v23 = 0.0f;
        }

        sithCamera_g_vecCameraPosOffset.x = v23;
    }

    if ( sithCamera_g_vecCameraPosOffset.y <= 0.0f )
    {
        if ( sithCamera_g_vecCameraPosOffset.y < 0.0f )
        {
            if ( sithCamera_g_vecCameraPosOffset.y + posDelta < 0.0f )
            {
                v20 = sithCamera_g_vecCameraPosOffset.y + posDelta;
            }
            else
            {
                v20 = 0.0f;
            }

            sithCamera_g_vecCameraPosOffset.y = v20;
        }
    }
    else
    {
        if ( sithCamera_g_vecCameraPosOffset.y - posDelta > 0.0f )
        {
            v21 = sithCamera_g_vecCameraPosOffset.y - posDelta;
        }
        else
        {
            v21 = 0.0f;
        }

        sithCamera_g_vecCameraPosOffset.y = v21;
    }

    if ( sithCamera_g_vecCameraPosOffset.z <= 0.0f )
    {
        if ( sithCamera_g_vecCameraPosOffset.z < 0.0f )
        {
            if ( sithCamera_g_vecCameraPosOffset.z + posDelta < 0.0f )
            {
                v18 = sithCamera_g_vecCameraPosOffset.z + posDelta;
            }
            else
            {
                v18 = 0.0f;
            }

            sithCamera_g_vecCameraPosOffset.z = v18;
        }
    }
    else
    {
        if ( sithCamera_g_vecCameraPosOffset.z - posDelta > 0.0f )
        {
            v19 = sithCamera_g_vecCameraPosOffset.z - posDelta;
        }
        else
        {
            v19 = 0.0f;
        }

        sithCamera_g_vecCameraPosOffset.z = v19;
    }

    if ( sithCamera_g_vecCameraAngleOffset.x <= 0.0f )
    {
        if ( sithCamera_g_vecCameraAngleOffset.x < 0.0f )
        {
            if ( sithCamera_g_vecCameraAngleOffset.x + angleDelta < 0.0f )
            {
                v16 = sithCamera_g_vecCameraAngleOffset.x + angleDelta;
            }
            else
            {
                v16 = 0.0f;
            }

            sithCamera_g_vecCameraAngleOffset.x = v16;
        }
    }
    else
    {
        if ( sithCamera_g_vecCameraAngleOffset.x - angleDelta > 0.0f )
        {
            v17 = sithCamera_g_vecCameraAngleOffset.x - angleDelta;
        }
        else
        {
            v17 = 0.0f;
        }

        sithCamera_g_vecCameraAngleOffset.x = v17;
    }

    if ( sithCamera_g_vecCameraAngleOffset.y <= 0.0f )
    {
        if ( sithCamera_g_vecCameraAngleOffset.y < 0.0f )
        {
            if ( sithCamera_g_vecCameraAngleOffset.y + angleDelta < 0.0f )
            {
                v14 = sithCamera_g_vecCameraAngleOffset.y + angleDelta;
            }
            else
            {
                v14 = 0.0f;
            }

            sithCamera_g_vecCameraAngleOffset.y = v14;
        }
    }
    else
    {
        if ( sithCamera_g_vecCameraAngleOffset.y - angleDelta > 0.0f )
        {
            v15 = sithCamera_g_vecCameraAngleOffset.y - angleDelta;
        }
        else
        {
            v15 = 0.0f;
        }

        sithCamera_g_vecCameraAngleOffset.y = v15;
    }

    if ( sithCamera_g_vecCameraAngleOffset.z <= 0.0f )
    {
        if ( sithCamera_g_vecCameraAngleOffset.z < 0.0f )
        {
            if ( sithCamera_g_vecCameraAngleOffset.z + angleDelta < 0.0f )
            {
                v12 = sithCamera_g_vecCameraAngleOffset.z + angleDelta;
            }
            else
            {
                v12 = 0.0f;
            }

            sithCamera_g_vecCameraAngleOffset.z = v12;
        }
    }
    else
    {
        if ( sithCamera_g_vecCameraAngleOffset.z - angleDelta > 0.0f )
        {
            v13 = sithCamera_g_vecCameraAngleOffset.z - angleDelta;
        }
        else
        {
            v13 = 0.0f;
        }

        sithCamera_g_vecCameraAngleOffset.z = v13;
    }
}

void sithCamera_RenderScene(void)
{
    if ( sithCamera_g_pCurCamera )
    {
        rdCamera_SetCurrent(&sithCamera_g_pCurCamera->rdCamera);
        rdCamera_Update(&sithCamera_g_pCurCamera->orient);
        sithRender_RenderScene();
    }
}

void J3DAPI sithCamera_SetPOVShake(const rdVector3* posOffset, const rdVector3* angleOffset, float posDelta, float angleDelta)
{
    sithCamera_g_vecCameraPosOffset   = *posOffset;
    sithCamera_g_vecCameraAngleOffset = *angleOffset;

    sithCamera_g_cameraPosDelta   = posDelta;
    sithCamera_g_cameraAngleDelta = angleDelta;
}

SithSector* J3DAPI sithCamera_SearchSectorInRadius(int a1, SithSector* pSector, const rdVector3* startPos, rdVector3* endPos, float unused, int flags)
{
    /* Function searches for collision that blocks camera movement */

    J3D_UNUSED(a1);
    J3D_UNUSED(unused);

    rdVector3 moveNorm;
    rdVector_Sub3(&moveNorm, endPos, startPos);
    float moveDist = rdVector_Normalize3Acc(&moveNorm);

    SithSector* pFoundSector = pSector;
    sithCollision_SearchForCollisions(pSector, NULL, startPos, &moveNorm, moveDist, 0.014f, flags | 0x800);

    SithCollision* pCollision = NULL;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
            SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin != NULL);
            pFoundSector = pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector;
        }

        else if ( ((pCollision->type & SITHCOLLISION_THING) == 0
            || pCollision->pThingCollided->type != SITH_THING_ITEM
            && pCollision->pThingCollided->type != SITH_THING_CORPSE
            && pCollision->distance != 0.0f
            && pCollision->pThingCollided->type != SITH_THING_WEAPON)
            && ((pCollision->type & SITHCOLLISION_THING) == 0 || pCollision->pThingCollided != sithPlayer_g_pLocalPlayerThing) )
        {
            rdVector_ScaleAdd3(endPos, &moveNorm, pCollision->distance, startPos);
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return pFoundSector;
}

void J3DAPI sithCamera_SetCameraStateFlags(SithCameraState flags)
{
    sithCamera_g_stateFlags = flags;
}

SithCameraState sithCamera_GetCameraStateFlags(void)
{
    return sithCamera_g_stateFlags;
}
