#include "sithAnimate.h"
#include <j3dcore/j3dhook.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

typedef struct sSithAnimateMoveFrame
{
    float distance;      // Move distance at frame
    float offsetFactor;  // Factor applied to frameDelta time to adjust frame distance
} SithAnimateMoveFrame;


static bool bOpen = false; // Added: Init. to false

static int lastUsedAnimNum; // can be -1
static SithAnimationSlot aAnims[512];

static size_t numFreeAnims;
static size_t aFreeAnimNums[STD_ARRAYLEN(aAnims)];

/**
* Animation data for the `in_pull_block.key` animation frames.
* These values precisely coordinate the movement of a pulled object with the player's pulling animation:
* - lastPullItemAnimFrameIdx: Maximum index for the aPullItemAnimFrames data
* - aPullItemAnimFrames: Specifies frame numbers in the in_pull_block.key animation where the pulled object
*   should be repositioned i.e.: pulled/moved
* - aPullItemDistances: Defines the exact distance the pulled object should be moved at each animation frame,
*   where distance is the target position relative to the starting point (negative values = pulled move),
*   and offsetFactor is used with frameDelta to adjust the movement between animation frames
*/
static const size_t lastPullItemAnimFrameIdx = 11u;
static const size_t aPullItemAnimFrames[12]  = { 0u, 0u, 29u, 38u, 54u, 72u, 110u, 120u, 128u, 135u, 155u, 163u };
static const SithAnimateMoveFrame aPullItemDistances[11] =
{
    { 0.0f, 0.0f },
    { 0.0f, -0.00190222f },
    { -0.01712f, -0.0030881299f },
    { -0.066529997f, 0.0f },
    { -0.066529997f, -0.00282553f },
    { -0.17390001f, 0.0f },
    { -0.17390001f, -0.00110625f },
    { -0.18275f, -0.0026557101f },
    { -0.20134f, 0.0f },
    { -0.20134f, 0.00033499999f },
    { -0.2f, 0.0f }
};

/**
* Animation data for the `in_push_block.key` animation frames.
* These values precisely coordinate the movement of a pushed object with the player's pushing animation:
* - lastPushItemAnimFrameIdx: Maximum index for the aPushItemAnimFrames data
* - aPushItemAnimFrames: Specifies frame numbers in the in_push_block.key animation where the pushed object
*   should be repositioned i.e.: pushed/moved
* - aPushItemDistances: Defines the exact distance the pushed object should be moved at each animation frame,
*   where distance is the target position relative to the starting point (positive values = pushed forward),
*   and offsetFactor is used with frameDelta to adjust the movement between animation frames
*/
static size_t lastPushItemAnimFrameIdx = 12u;
const size_t aPushItemAnimFrames[13]   = { 0u, 0u, 12u, 25u, 35u, 51u, 66u, 85u, 99u, 108u, 121u, 129u, 151u };
static SithAnimateMoveFrame aPushItemDistances[12] =
{
    { 0.0f, 0.0f },
    { 0.0f, 0.00167f },
    { 0.021709999f, 0.002697f },
    { 0.04868f, 0.0f },
    { 0.04868f, 0.0021413299f },
    { 0.080799997f, 0.0030821001f },
    { 0.13936f, 0.0f },
    { 0.13936f, 0.0021388901f },
    { 0.15861f, 0.0029861501f },
    { 0.19743f, 0.0f },
    { 0.19743f, 0.00012682f },
    { 0.20100001f, 0.0f }
};


void J3DAPI sithAnimate_BuildScrollFaceRotate(rdMatrix34* pOrient, const rdFace* pFace, SithSurfaceFlag surfflags);
float J3DAPI sithAnimate_ExtractUVAngle(float v, float u);

void J3DAPI sithAnimate_UpdateCameraZoomAnim(SithAnimationSlot* pAnim, float secTimeDelta);
void J3DAPI sithAnimate_UpdateSpriteSizeAnim(SithAnimationSlot* pAnim, float secDeltaTime);
void J3DAPI sithAnimate_UpdateLightAnim(SithAnimationSlot* pAnim, float secDeltaTime);
void J3DAPI sithAnimate_UpdateThingMoveAnim(SithAnimationSlot* pAnim, float secTimeDelta);
void J3DAPI sithAnimate_UpdateThingMoveToPosAnim(SithAnimationSlot* pAnim, float secDeltaTime);
void J3DAPI sithAnimate_UpdateThingFadeAnim(SithAnimationSlot* pAnim, float secDeltaTime);
void J3DAPI sithAnimate_UpdateSurfaceScrollAnim(SithAnimationSlot* pAnim, float secDeltaTime, int animNum);
void J3DAPI sithAnimate_UpdateSlideSkyAnim(SithAnimationSlot* pAnim, int skyType, float secDeltaTime, int animNum);
void J3DAPI sithAnimate_UpdatePageFlipAnim(SithAnimationSlot* pAnim);
void J3DAPI sithAnimate_UpdatePushItemAnim(SithAnimationSlot* pAnim);
void J3DAPI sithAnimate_UpdatePullItemAnim(SithAnimationSlot* pAnim);
void J3DAPI sithAnimate_UpdateThingQuickTurnAnim(SithAnimationSlot* pAnim, float secDeltaTime);

void J3DAPI sithAnimate_FreeAllAnims();
void J3DAPI sithAnimate_FreeAnim(SithAnimationSlot* pAnim);
SithAnimationSlot* J3DAPI sithAnimate_NewAnim();

size_t J3DAPI sithAnimate_GetAnimFrameIndex(float frame, size_t size, const size_t* aAnimFrames);
float J3DAPI sithAnimate_GetPuppetTrackCurFrame(SithThing* pThing, int trackNum);
SithSurface* J3DAPI sithAnimate_GetThingFloorSurface(const SithThing* pThing);
void J3DAPI sithAnimate_ExitFloorSurface(const SithThing* pThing, const SithSurface* pSurface);
void J3DAPI sithAnimate_FindAndEnterFloorSurface(const SithThing* pSrcThing, const SithSurface* pSurf);

void sithAnimate_InstallHooks(void)
{
    J3D_HOOKFUNC(sithAnimate_Startup);
    J3D_HOOKFUNC(sithAnimate_Open);
    J3D_HOOKFUNC(sithAnimate_Close);
    J3D_HOOKFUNC(sithAnimate_Reset);
    J3D_HOOKFUNC(sithAnimate_Stop);
    J3D_HOOKFUNC(sithAnimate_GetSurfaceAnimIndex);
    J3D_HOOKFUNC(sithAnimate_StartScrollSurfaceAnim);
    J3D_HOOKFUNC(sithAnimate_BuildScrollFaceRotate);
    J3D_HOOKFUNC(sithAnimate_ExtractUVAngle);
    J3D_HOOKFUNC(sithAnimate_StartSideSkyAnim);
    J3D_HOOKFUNC(sithAnimate_StartSpriteAnim);
    J3D_HOOKFUNC(sithAnimate_StartSurfaceAnim);
    J3D_HOOKFUNC(sithAnimate_StartMaterialAnim);
    J3D_HOOKFUNC(sithAnimate_StartSurfaceLightAnim);
    J3D_HOOKFUNC(sithAnimate_StartAnimateSpriteSize);
    J3D_HOOKFUNC(sithAnimate_StartSectorLightAnim);
    J3D_HOOKFUNC(sithAnimate_StartThingLightAnim);
    J3D_HOOKFUNC(sithAnimate_StartThingMove);
    J3D_HOOKFUNC(sithAnimate_StartThingMoveToPos);
    J3D_HOOKFUNC(sithAnimate_StartThingFadeAnim);
    J3D_HOOKFUNC(sithAnimate_CameraZoom);
    J3D_HOOKFUNC(sithAnimate_PushItem);
    J3D_HOOKFUNC(sithAnimate_PullItem);
    J3D_HOOKFUNC(sithAnimate_Update);
    J3D_HOOKFUNC(sithAnimate_UpdateThingQuickTurnAnim);
    J3D_HOOKFUNC(sithAnimate_GetSurfaceAnim);
    J3D_HOOKFUNC(sithAnimate_GetSurfaceScrollingDirection);
    J3D_HOOKFUNC(sithAnimate_GetAnim);
    J3D_HOOKFUNC(sithAnimate_GetMaterialAnim);
    J3D_HOOKFUNC(sithAnimate_Create);
    J3D_HOOKFUNC(sithAnimate_Save);
    J3D_HOOKFUNC(sithAnimate_UpdateCameraZoomAnim);
    J3D_HOOKFUNC(sithAnimate_UpdateSpriteSizeAnim);
    J3D_HOOKFUNC(sithAnimate_UpdateLightAnim);
    J3D_HOOKFUNC(sithAnimate_UpdateThingMoveAnim);
    J3D_HOOKFUNC(sithAnimate_UpdateThingMoveToPosAnim);
    J3D_HOOKFUNC(sithAnimate_UpdateThingFadeAnim);
    J3D_HOOKFUNC(sithAnimate_UpdateSurfaceScrollAnim);
    J3D_HOOKFUNC(sithAnimate_UpdateSlideSkyAnim);
    J3D_HOOKFUNC(sithAnimate_UpdatePageFlipAnim);
    J3D_HOOKFUNC(sithAnimate_FreeAllAnims);
    J3D_HOOKFUNC(sithAnimate_FreeAnim);
    J3D_HOOKFUNC(sithAnimate_NewAnim);
    J3D_HOOKFUNC(sithAnimate_UpdatePushItemAnim);
    J3D_HOOKFUNC(sithAnimate_UpdatePullItemAnim);
    J3D_HOOKFUNC(sithAnimate_GetAnimFrameIndex);
    J3D_HOOKFUNC(sithAnimate_GetPuppetTrackCurFrame);
    J3D_HOOKFUNC(sithAnimate_GetThingFloorSurface);
    J3D_HOOKFUNC(sithAnimate_ExitFloorSurface);
    J3D_HOOKFUNC(sithAnimate_FindAndEnterFloorSurface);
}

void sithAnimate_ResetGlobals(void)
{}

void sithAnimate_Startup(void)
{
    sithAnimate_FreeAllAnims();
}

void sithAnimate_Shutdown(void)
{}

int sithAnimate_Open(void)
{
    sithAnimate_FreeAllAnims();
    bOpen = true;
    return 0;
}

void J3DAPI sithAnimate_Close()
{
    sithAnimate_FreeAllAnims();
    bOpen = false;
}

void sithAnimate_Reset(void)
{
    sithAnimate_FreeAllAnims();
}

void J3DAPI sithAnimate_Stop(SithAnimationSlot* pAnim)
{
    SITH_ASSERTREL(pAnim);
    if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 && (pAnim->flags & SITHANIMATE_SCROLL) != 0 )
    {
        SITH_ASSERTREL(pAnim->pSurface);
        pAnim->pSurface->flags &= ~SITH_SURFACE_SCROLLING;

        pAnim->direction3 = rdroid_g_zeroVector3;
        pAnim->direction2 = rdroid_zeroVector2;
    }

    if ( (pAnim->flags & (SITHANIMATE_THING_MOVEPOS | SITHANIMATE_THING_MOVE)) != 0 && pAnim->pThing && pAnim->pThing->moveType == SITH_MT_PHYSICS )
    {
        pAnim->pThing->moveInfo.physics.flags &= ~SITH_PF_NOUPDATE;
    }

    sithAnimate_FreeAnim(pAnim);
}

int J3DAPI sithAnimate_GetSurfaceAnimIndex(const SithSurface* pSurf)
{
    SithAnimationSlot* pAnim = sithAnimate_GetSurfaceAnim(pSurf);
    if ( pAnim )
    {
        return pAnim - aAnims;
    }
    return -1;
}

SithAnimationSlot* J3DAPI sithAnimate_StartScrollSurfaceAnim(SithSurface* pSurf, const rdVector3* velocity)
{
    SITH_ASSERTREL(pSurf);

    if ( !pSurf->face.pMaterial )
    {
        return NULL;
    }

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    pAnim->flags      = SITHANIMATE_SCROLL | SITHANIMATE_SURFACE;
    pAnim->pSurface   = pSurf;
    pAnim->direction3 = *velocity;

    // Calculate 2D direction
    rdMatrix34 orient;
    sithAnimate_BuildScrollFaceRotate(&orient, &pSurf->face, pSurf->flags);

    rdVector3 dest;
    rdMatrix_TransformVector34(&dest, velocity, &orient);

    pAnim->direction2.x = (float)(-dest.x / (double)pSurf->face.pMaterial->width);
    pAnim->direction2.y = (float)(-dest.y / (double)pSurf->face.pMaterial->height);

    // Mark the surface as scrolling
    pSurf->flags |= SITH_SURFACE_SCROLLING;

    return pAnim;
}

void J3DAPI sithAnimate_BuildScrollFaceRotate(rdMatrix34* pOrient, const rdFace* pFace, SithSurfaceFlag surfflags)
{
    SITH_ASSERTREL(pFace != NULL);
    SITH_ASSERTREL(pFace->numVertices >= 3);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;

    rdMatrix34 mat = { 0 }; // Added: Init to 0
    mat.uvec = pFace->normal;;

    size_t vertNum = 1;
    rdVector3 vert = pWorld->aVertices[pFace->aVertices[0]];
    rdVector3 nextVert =  pWorld->aVertices[pFace->aVertices[vertNum]];

    rdVector2 texVert = pWorld->aTexVerticies[pFace->aTexVertices[0]];
    rdVector2 nextTexVert = pWorld->aTexVerticies[pFace->aTexVertices[vertNum]];

    rdVector3 dvert;
    rdVector_Sub3(&dvert, &vert, &nextVert);

    while ( rdVector_Len3(&dvert) < 0.0001f )
    {
        if ( ++vertNum == pFace->numVertices )
        {
            vertNum = 0;
        }

        nextVert    =  pWorld->aVertices[pFace->aVertices[vertNum]];
        nextTexVert = pWorld->aTexVerticies[pFace->aTexVertices[vertNum]];

        // Fixed: Recalculate dvert, was missing in the original code
        rdVector_Sub3(&dvert, &vert, &nextVert);
    }

    float u = nextTexVert.x - texVert.x;
    float v = nextTexVert.y - texVert.y;

    rdVector_Sub3(&dvert, &nextVert, &vert);

    // Set rvec and normalize 
    mat.rvec = dvert;
    rdVector_Normalize3Acc(&mat.uvec);
    rdVector_Normalize3Acc(&mat.rvec);

    // Calculate lvec and set dvec
    rdVector_Cross3(&mat.lvec, &mat.uvec, &mat.rvec);
    mat.dvec = vert; // set position

    // Calculate resulting orientation matrix
    rdMatrix_InvertOrtho34(pOrient, &mat);

    // Rotate matrix to UV angle 
    float angle = sithAnimate_ExtractUVAngle(v, u);

    // Altered: Moved this calculation before following if statements
    const rdVector3 vecPYR = { .pitch = 0.0f, .yaw = angle, .roll = 0.0f };
    rdMatrix_PostRotate34(pOrient, &vecPYR);

    // Scale the matrix
    float size = 320.0f;
    if ( (surfflags & SITH_SURFACE_QUARTERSIZE) != 0 )
    {
        size = size / 4.0f;
    }
    else if ( (surfflags & SITH_SURFACE_HALFSIZE) != 0 )
    {
        size = size / 2.0f;
    }
    else if ( (surfflags & SITH_SURFACE_DOUBLESIZE) != 0 )
    {
        size = size * 2.0f;
    }
    else if ( (surfflags & SITH_SURFACE_QUADRUPLESIZE) != 0 )
    {
        size = size * 4.0f;
    }
    else if ( (surfflags & SITH_SURFACE_EIGHTSIZE) != 0 )
    {
        size = size / 8.0f;
    }

    const rdVector3 vecScale = { size, size, 1.0f };
    rdMatrix_PostScale34(pOrient, &vecScale);
}

float J3DAPI sithAnimate_ExtractUVAngle(float v, float u)
{
    if ( u == 0.0f && v == 0.0f )
    {
        return 0.0f;
    }

    if ( u == 0.0f )
    {
        return (v <= 0.0f) ? -90.0f : 90.0f;
    }
    else if ( v == 0.0f )
    {
        return (u <= 0.0f) ? 180.0f : 0.0f;
    }

    float angle = STDMATH_TODEGREESF(atanf(v / u));
    if ( u < 0.0f && v > 0.0f )
    {
        SITH_ASSERTREL(angle < 0.0f);
        angle = angle + 180.0f;
    }

    if ( u < 0.0f && v < 0.0f )
    {
        SITH_ASSERTREL(angle > 0.0f);
        angle -= 180.0f;
    }

    SITH_ASSERTREL(angle >= -180.0f);
    SITH_ASSERTREL(angle <= 180.0f);
    return angle;
}

SithAnimationSlot* J3DAPI sithAnimate_StartSideSkyAnim(SithSurfaceFlag skyType, const rdVector2* pDirection)
{
    SITH_ASSERTREL(skyType == SITH_SURFACE_HORIZONSKY || skyType == SITH_SURFACE_CEILINGSKY);

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    if ( skyType == SITH_SURFACE_HORIZONSKY )
    {
        pAnim->flags = SITHANIMATE_SKYHORIZON | SITHANIMATE_SCROLL;
    }
    else if ( skyType == SITH_SURFACE_CEILINGSKY )
    {
        pAnim->flags = SITHANIMATE_SKYCEILING | SITHANIMATE_SCROLL;
    }

    pAnim->direction2 = *pDirection;
    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartSpriteAnim(SithThing* pSprite, float fps, SithAnimateFlags flags)
{
    SITH_ASSERTREL(pSprite && (fps >= 0.0f));
    SITH_ASSERTREL(pSprite->renderData.type == RD_THING_SPRITE3);

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    pSprite->renderData.matCelNum = 0;
    if ( (flags & SITHANIMATE_START_THIRD_FRAME) != 0 )
    {
        pAnim->curFrame = 2;
    }
    else
    {
        pAnim->curFrame = (flags & SITHANIMATE_START_SECOND_FRAME) != 0 ? 1 : 0;
    }

    pAnim->flags             = (uint16_t)flags | SITHANIMATE_PAGEFLIP | SITHANIMATE_SPRITE; // (uint16_t)flags - clipped to 0xFFFF
    pAnim->pThing            = pSprite;
    pAnim->thingSignature    = pSprite->signature;
    pAnim->pMaterial         = pSprite->renderData.data.pSprite3->face.pMaterial;
    pAnim->msecPerFrame      = (uint32_t)(1000.0f / fps);
    pAnim->msecNextFrameTime = pAnim->msecPerFrame + sithTime_g_msecGameTime;
    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartSurfaceAnim(SithSurface* pSurf, float fps, SithAnimateFlags flags)
{
    SITH_ASSERTREL(pSurf);
    SITH_ASSERTREL(fps > 0.0f);

    if ( !pSurf->face.pMaterial )
    {
        return NULL;
    }

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        SITHLOG_ERROR("Out of animation slots.\n");
        return NULL;
    }

    if ( (flags & SITHANIMATE_START_THIRD_FRAME) != 0 )
    {
        pAnim->curFrame = 2;
    }
    else
    {
        pAnim->curFrame = (flags & SITHANIMATE_START_SECOND_FRAME) != 0 ? 1 : 0;
    }

    pAnim->flags        = (uint16_t)flags | SITHANIMATE_PAGEFLIP | SITHANIMATE_SURFACE; // (uint16_t)flags - clipped to 0xFFFF
    pAnim->pSurface     = pSurf;
    pAnim->pMaterial    = pSurf->face.pMaterial;
    pAnim->msecPerFrame = (uint32_t)(1000.0f / fps);

    if ( !pAnim->msecPerFrame )
    {
        SITHLOG_ERROR("Bad parameters to startSurfaceAnim.\n");
        sithAnimate_FreeAnim(pAnim);
        return NULL;
    }

    pAnim->msecNextFrameTime = pAnim->msecPerFrame + sithTime_g_msecGameTime;
    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartMaterialAnim(rdMaterial* pMat, float fps, SithAnimateFlags flags)
{
    SITH_ASSERTREL(pMat);
    SITH_ASSERTREL(fps > 0.0f);

    if ( !pMat->numCels )
    {
        return NULL;
    }

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        SITHLOG_ERROR("Out of animation slots.\n");
        return NULL;
    }

    if ( (flags & SITHANIMATE_START_THIRD_FRAME) != 0 )
    {
        pAnim->curFrame = 2;
    }
    else
    {
        pAnim->curFrame = (flags & SITHANIMATE_START_SECOND_FRAME) != 0 ? 1 : 0;
    }

    pAnim->pMaterial    = pMat;
    pAnim->flags        = (uint16_t)flags | SITHANIMATE_PAGEFLIP | SITHANIMATE_MATERIAL; // (uint16_t)flags - clipped to 0xFFFF
    pAnim->msecPerFrame = (uint32_t)(1000.0f / fps);
    if ( !pAnim->msecPerFrame )
    {
        SITHLOG_ERROR("Bad parameters to startMaterialAnim.\n");
        sithAnimate_FreeAnim(pAnim);
        return NULL;
    }

    pAnim->msecNextFrameTime = pAnim->msecPerFrame + sithTime_g_msecGameTime;
    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartSurfaceLightAnim(SithSurface* pSurface, const rdVector3* light, float timeDelta)
{
    SITH_ASSERTREL(pSurface && (timeDelta > 0.0f));

    float dRed   = light->red - pSurface->face.extraLight.red;
    float dGreen = light->green - pSurface->face.extraLight.green;
    float dBlue  = light->blue - pSurface->face.extraLight.blue;

    if ( dRed == 0.0f && dGreen == 0.0f && dBlue == 0.0f )
    {
        return NULL;
    }

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        SITHLOG_ERROR("Out of animation slots.\n");
        return NULL;
    }

    pAnim->pSurface      = pSurface;
    pAnim->flags         = SITHANIMATE_LIGHT | SITHANIMATE_SURFACE;
    pAnim->deltaVector.x = dRed / timeDelta;
    pAnim->deltaVector.y = dGreen / timeDelta;
    pAnim->deltaVector.z = dBlue / timeDelta;
    pAnim->curVector     = pSurface->face.extraLight;
    rdVector_Copy3((rdVector3*)&pAnim->endVector, light); // Note, alpha not set

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartThingQuickTurn(SithThing* pThing, int direction)
{
    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    pAnim->curValue = 0.0f;
    if ( direction != 1 ) // left
    {
        pAnim->endValue = 90.0f;
    }
    else
    {
        pAnim->endValue = -90.0f;
    }

    pAnim->deltaValue     = pAnim->endValue * 4.0f;
    pAnim->pThing         = pThing;
    pAnim->thingSignature = pThing->signature;
    pAnim->flags          = SITHANIMATE_THING_QUICKTURN | SITHANIMATE_THING;

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartAnimateSpriteSize(SithThing* pSprite, const rdVector3* start, const rdVector3* end, float timeDelta)
{
    SITH_ASSERTREL(pSprite);

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    pAnim->flags       = SITHANIMATE_SPRITE_SIZE | SITHANIMATE_THING;
    pAnim->endVector.x = end->x;    // width
    pAnim->endVector.y = end->y;    // height
    pAnim->endVector.z = end->z;    // alpha
    pAnim->endVector.w = 0.0f;

    pAnim->curVector.x = start->x;  // width
    pAnim->curVector.y = start->y;  // height
    pAnim->curVector.z = start->z;  // alpha
    pAnim->curVector.w = 0.0f;

    // Fixed: Added handling for zero timeDelta. Originally division by 0 didn't produced NAN values but 0 so the animation wasn't looping indefinitely.
    //        In other functions such case is handled by assertion but we need to let this one pass 
    //        because aet_sprpulse.cog script uses timeDelta = 0.0 to instantly set the sprite size to end value.
    if ( timeDelta == 0.0f )
    {
        timeDelta = 1.0f;
        pAnim->curVector.x = end->x;  // width
        pAnim->curVector.y = end->y;  // height
        pAnim->curVector.z = end->z;  // alpha
    }

    pAnim->deltaVector.x = (pAnim->endVector.x - pAnim->curVector.x) / timeDelta;
    pAnim->deltaVector.y = (pAnim->endVector.y - pAnim->curVector.y) / timeDelta;
    pAnim->deltaVector.z = (end->z - start->z) / timeDelta;

    pAnim->thingSignature = pSprite->signature;
    pAnim->pThing         = pSprite;

    pSprite->thingInfo.spriteInfo.width  = pAnim->curVector.x;
    pSprite->thingInfo.spriteInfo.height = pAnim->curVector.y;
    pSprite->thingInfo.spriteInfo.alpha  = pAnim->curVector.z;

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartSectorLightAnim(SithSector* pSector, const rdVector3* pNewLight, float timeDelta, SithAnimateFlags flags)
{
    SITH_ASSERTREL(pSector && (timeDelta > 0.0f));
    flags &= SITHANIMATE_LOOP; // clear all but loop flag

    rdVector3 colorDelta;
    rdVector_Sub3(&colorDelta, pNewLight, (const rdVector3*)&pSector->extraLight);

    if ( rdVector_IsZero3(&colorDelta) )
    {
        return NULL;
    }

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        SITHLOG_ERROR("Out of animation slots.\n");
        return NULL;
    }

    pAnim->pSector           = pSector;
    pAnim->flags             = flags | SITHANIMATE_SECTOR | SITHANIMATE_LIGHT;
    pAnim->deltaVector.red   = colorDelta.red / timeDelta;
    pAnim->deltaVector.green = colorDelta.green / timeDelta;
    pAnim->deltaVector.blue  = colorDelta.blue / timeDelta;
    pAnim->startVector       = pSector->extraLight; // Should it set vecCurrent instead?
    rdVector_Copy3((rdVector3*)&pAnim->endVector, pNewLight); // Note, alpha not set

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartThingLightAnim(SithThing* pThing, const rdVector4* light, float timeDelta, SithAnimateFlags flags)
{
    SITH_ASSERTREL(pThing && (timeDelta > 0.0f));

    flags &= SITHANIMATE_LOOP; // Clear all but loop flag

    rdVector4 lightDelta;
    lightDelta.red   = light->red - pThing->light.color.red;
    lightDelta.green = light->green - pThing->light.color.green;
    lightDelta.blue  = light->blue - pThing->light.color.blue;
    lightDelta.w     = light->w - pThing->light.minRadius; // range delta

    if ( rdVector_IsZero4(&lightDelta) )
    {
        return NULL;
    }

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        SITHLOG_ERROR("Out of animation slots.\n");
        return NULL;
    }

    pAnim->pThing         = pThing;
    pAnim->thingSignature = pThing->signature;
    pAnim->flags          = flags | SITHANIMATE_LIGHT | SITHANIMATE_THING;

    pAnim->deltaVector.red   = lightDelta.red / timeDelta;
    pAnim->deltaVector.green = lightDelta.green / timeDelta;
    pAnim->deltaVector.blue  = lightDelta.blue / timeDelta;
    pAnim->deltaVector.w     = lightDelta.w / timeDelta; // range

    pAnim->startVector   = pThing->light.color;
    pAnim->startVector.w = pThing->light.minRadius;

    pAnim->curVector   = pThing->light.color;
    pAnim->curVector.w = pAnim->startVector.w; // range

    pAnim->endVector = *light;

    // Flag the thing to emit light
    pThing->flags |= SITH_TF_EMITLIGHT;

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartThingMove(SithThing* pThing, const rdVector3* pDirection, float distance, float timeDelta)
{
    if ( !pThing )
    {
        SITHLOG_ERROR("Thing is NULL in sithAnimate_StartThingMove()\n");
        return NULL;
    }

    if ( rdVector_IsZero3(pDirection) )
    {
        SITHLOG_ERROR("Direction is 0 in sithAnimate_StartThingMove(), %s\n", pThing->aName);
        return NULL;
    }

    if ( distance < 0.0f )
    {
        SITHLOG_ERROR("Distance < 0, sithAnimate_StartThingMove(), %s\n", pThing->aName);
        return NULL;
    }

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    // Fixed: Added handling for zero timeDelta. Originally division by 0 didn't produced NAN values but 0 so the animation wasn't looping indefinitely.
    if ( timeDelta == 0.0f )
    {
        timeDelta = 1.0f;
    }

    pAnim->pThing         = pThing;
    pAnim->thingSignature = pThing->signature;
    pAnim->flags          = SITHANIMATE_THING_MOVE | SITHANIMATE_THING;
    pAnim->unknown24      = 0;
    pAnim->curValue       = 0.0f;
    pAnim->endValue       = distance;
    pAnim->direction3     = *pDirection;
    pAnim->deltaValue     = distance / timeDelta; // deltaDistance

    // Mark thing to not be affected by physics
    pThing->moveInfo.physics.flags |= SITH_PF_NOUPDATE;

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartThingMoveToPos(SithThing* pThing, const rdVector3* pPos, float time)
{
    if ( !pThing )
    {
        SITHLOG_ERROR("Thing is NULL in sithAnimate_StartThingMoveToPos()\n");
        return NULL;
    }

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        if ( (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR) && pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        }

        return NULL;
    }

    pAnim->pThing           = pThing;
    pAnim->thingSignature   = pThing->signature;
    pAnim->flags            = SITHANIMATE_THING_MOVEPOS | SITHANIMATE_THING;
    pAnim->secTimeRemaining = time;
    pAnim->thingEndPosition = *pPos;

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        // Mark thing to not be affected by physics
        pThing->moveInfo.physics.flags |= SITH_PF_NOUPDATE;
    }

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartThingFadeAnim(SithThing* pThing, const rdVector4* startColor, const rdVector4* endColor, float timeDelta, SithAnimateFlags flags)
{
    SITH_ASSERTREL(pThing && (timeDelta > 0.0f));

    flags &= SITHANIMATE_LOOP; // Clear all but loop flag

    rdVector4 colorDelta;
    rdVector_Sub4(&colorDelta, endColor, startColor);

    if ( rdVector_IsZero4(&colorDelta) )
    {
        return NULL;
    }

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        SITHLOG_ERROR("Out of animation slots.\n");
        return NULL;
    }

    pAnim->pThing         = pThing;
    pAnim->thingSignature = pThing->signature;
    pAnim->flags          = flags | SITHANIMATE_THING_FADE;

    pAnim->deltaVector.red   = colorDelta.red / timeDelta;
    pAnim->deltaVector.green = colorDelta.green / timeDelta;
    pAnim->deltaVector.blue  = colorDelta.blue / timeDelta;
    pAnim->deltaVector.alpha = colorDelta.alpha / timeDelta;

    pAnim->startVector = *startColor;
    pAnim->curVector   = *startColor;
    pAnim->endVector   = *endColor;

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_CameraZoom(SithCamera* pCamera, float fov, float timeDelta)
{
    SITH_ASSERTREL(timeDelta > 0.0f);

    float curFov = sithCamera_g_pCurCamera->fov;
    float dFov = fov - curFov;
    if ( dFov == 0.0f )
    {
        return NULL;
    }

    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        SITHLOG_ERROR("Out of animation slots, CameraZoom.\n");
        return NULL;
    }

    pAnim->pThing         = (SithThing*)pCamera;   // ?? huh, but thing is expected ?
    pAnim->thingSignature = 0;
    pAnim->flags          = SITHANIMATE_CAMERA_ZOOM | SITHANIMATE_THING;
    pAnim->deltaValue     = dFov / timeDelta;
    pAnim->endValue       = fov;
    pAnim->curValue       = curFov;

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_PushItem(SithThing* pActor, SithThing* pItem, const rdVector3* pDirection, int trackNum)
{
    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        SITHLOG_ERROR("Out of animation slots, PushItem.\n");
        return NULL;
    }

    SITH_ASSERTREL(pActor);
    pAnim->pThing = pActor;

    if ( pItem )
    {
        pAnim->pItemThing = pItem;

        // Fixed: Moved assignment into this block to avoid null pointer dereference
        pAnim->msecPerFrame = pItem->collide.type; // Note, msecPerFrame is valid name as it was found in debug symbols.. So might be that anim struct was union, or devs were just lazy and reuse this field instead of creating new one

        // Make item thing non-collidable
        pItem->collide.type = SITH_COLLIDE_NONE; // Fixed: Moved assignment into this block to avoid null pointer dereference
    }

    pAnim->direction3     = *pDirection;
    pAnim->deltaValue     = 0.0020000001f;       // ??
    pAnim->endValue       = 0.2f;                // 2m
    pAnim->curValue       = 0.0f;
    pAnim->thingSignature = pActor->signature;
    pAnim->flags          = SITHANIMATE_THING | SITHANIMATE_PUSHITEM;
    pAnim->curFrame       = 0;
    pAnim->deltaValue     = 0.0f;                // ?? it is set to current animate frame in update
    pAnim->startVector.x  = 0.0f;                // Holds current distance item was pushed for
    pAnim->trackNum       = trackNum;
    pAnim->pSurface       = sithAnimate_GetThingFloorSurface(pItem);

    if ( pAnim->pSurface )
    {
        sithAnimate_ExitFloorSurface(pItem, pAnim->pSurface);
    }

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_PullItem(SithThing* pActor, SithThing* pItem, const rdVector3* pDirection, int trackNum)
{
    SithAnimationSlot* pAnim  = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        SITHLOG_ERROR("Out of animation slots, PullItem.\n");
        return NULL;
    }

    SITH_ASSERTREL(pActor);
    pAnim->pThing = pActor;

    if ( pItem )
    {
        pAnim->pItemThing = pItem;

        // Fixed: Moved assignment into this block to avoid null pointer dereference
        pAnim->msecPerFrame = pItem->collide.type; // Note, msecPerFrame is valid name as it was found in debug symbols.. So might be that anim struct was union, or devs were just lazy and reuse this field instead of creating new one

        // Make item thing non-collidable
        pItem->collide.type = SITH_COLLIDE_NONE; // Fixed: Moved assignment into this block to avoid null pointer dereference
    }

    pAnim->direction3     = *pDirection;
    pAnim->deltaValue     = 0.0020000001f;       // ???
    pAnim->endValue       = 0.2f;                // 2m
    pAnim->curValue       = 0.0f;
    pAnim->thingSignature = pActor->signature;
    pAnim->flags          = SITHANIMATE_THING | SITHANIMATE_PULLITEM;
    pAnim->curFrame       = 0;
    pAnim->deltaValue     = 0.0f;                // ?? it is set to current animate frame in update
    pAnim->startVector.x  = 0.0f;
    pAnim->trackNum       = trackNum;
    pAnim->pSurface       = sithAnimate_GetThingFloorSurface(pItem);

    if ( pAnim->pSurface )
    {
        sithAnimate_ExitFloorSurface(pItem, pAnim->pSurface);
    }

    return pAnim;
}

void J3DAPI sithAnimate_Update(float secTimeDelta)
{
    int curLastUsedAnimNum = lastUsedAnimNum; // Note, the number has to be cached, as it can change during the loop, i.e.: anim can be freed in the process of updating
    for ( int animNum = 0; animNum <= curLastUsedAnimNum; animNum++ )
    {
        SithAnimationSlot* pAnim = &aAnims[animNum];

        if ( pAnim->flags )
        {
            // Do this check first, as pThing is actually SithCamera here
            if ( pAnim->pThing && (pAnim->flags & SITHANIMATE_CAMERA_ZOOM) != 0 )
            {
                sithAnimate_UpdateCameraZoomAnim(pAnim, secTimeDelta);
            }
            else if ( pAnim->pThing && (pAnim->pThing->type == SITH_THING_FREE || pAnim->pThing->signature != pAnim->thingSignature) )
            {
                // Thing was freed, remove thing anim
                sithAnimate_FreeAnim(pAnim);
            }
            else if ( (pAnim->flags & SITHANIMATE_THING_MOVE) != 0 )
            {
                sithAnimate_UpdateThingMoveAnim(pAnim, secTimeDelta);
            }
            else if ( (pAnim->flags & SITHANIMATE_THING_MOVEPOS) != 0 )
            {
                sithAnimate_UpdateThingMoveToPosAnim(pAnim, secTimeDelta);
            }
            else if ( (pAnim->flags & SITHANIMATE_PUSHITEM) != 0 )
            {
                sithAnimate_UpdatePushItemAnim(pAnim);
            }
            else if ( (pAnim->flags & SITHANIMATE_PULLITEM) != 0 )
            {
                sithAnimate_UpdatePullItemAnim(pAnim);
            }
            else if ( (pAnim->flags & SITHANIMATE_SPRITE_SIZE) != 0 )
            {
                sithAnimate_UpdateSpriteSizeAnim(pAnim, secTimeDelta);
            }
            else if ( (pAnim->flags & SITHANIMATE_THING_QUICKTURN) != 0 )
            {
                sithAnimate_UpdateThingQuickTurnAnim(pAnim, secTimeDelta);
            }
            else if ( (pAnim->flags & SITHANIMATE_SCROLL) != 0 )
            {
                if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 )
                {
                    sithAnimate_UpdateSurfaceScrollAnim(pAnim, secTimeDelta, animNum);
                }
                else if ( (pAnim->flags & SITHANIMATE_SKYHORIZON) != 0 )
                {
                    sithAnimate_UpdateSlideSkyAnim(pAnim, SITH_SURFACE_HORIZONSKY, secTimeDelta, animNum);
                }
                else if ( (pAnim->flags & SITHANIMATE_SKYCEILING) != 0 )
                {
                    sithAnimate_UpdateSlideSkyAnim(pAnim, SITH_SURFACE_CEILINGSKY, secTimeDelta, animNum);
                }
            }
            else if ( (pAnim->flags & SITHANIMATE_PAGEFLIP) != 0 && pAnim->msecNextFrameTime <= sithTime_g_msecGameTime )
            {
                sithAnimate_UpdatePageFlipAnim(pAnim);
            }
            else if ( (pAnim->flags & SITHANIMATE_LIGHT) != 0 )
            {
                sithAnimate_UpdateLightAnim(pAnim, secTimeDelta);
            }
            else if ( (pAnim->flags & SITHANIMATE_THING_FADE) != 0 )
            {
                sithAnimate_UpdateThingFadeAnim(pAnim, secTimeDelta);
            }
        }
    }
}

void J3DAPI sithAnimate_UpdateThingQuickTurnAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    float newRotAngle = pAnim->deltaValue * secDeltaTime;
    if ( newRotAngle > 0.0f )
    {
        if ( newRotAngle <= 90.0f )
        {
            if ( newRotAngle < 0.0f && newRotAngle < -90.0f )
            {
                newRotAngle = -90.0f;
            }
        }
        else
        {
            newRotAngle = 90.0f;
        }
    }

    bool bFinish = false;
    if ( pAnim->endValue > 0.0f && (pAnim->curValue + newRotAngle) >= (double)pAnim->endValue )
    {
        bFinish = true;
    }
    else if ( pAnim->endValue < 0.0f && (pAnim->curValue + newRotAngle) <= (double)pAnim->endValue )
    {
        bFinish = true;
    }

    pAnim->curValue = pAnim->curValue + newRotAngle;

    rdVector3 pyr = { 0 };
    pyr.yaw = newRotAngle;
    rdMatrix_PreRotate34(&pAnim->pThing->orient, &pyr);

    if ( bFinish )
    {
        pAnim->pThing->thingInfo.actorInfo.flags &= ~SITH_AF_CONTROLSDISABLED;
        pAnim->pThing->moveStatus                 = SITHPLAYERMOVE_STILL;
        sithAnimate_Stop(pAnim);
    }
}

SithAnimationSlot* J3DAPI sithAnimate_GetSurfaceAnim(const SithSurface* pSurf)
{
    SITH_ASSERTREL(pSurf);

    for ( int i = 0; i <= lastUsedAnimNum; ++i )
    {
        SithAnimationSlot* pAnim = &aAnims[i];
        if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 && pAnim->pSurface == pSurf )
        {
            return pAnim;
        }
    }

    return NULL;
}

void J3DAPI sithAnimate_GetSurfaceScrollingDirection(const SithSurface* pSurf, rdVector3* result)
{
    SITH_ASSERTREL(pSurf && result);

    SithAnimationSlot* pAnim = sithAnimate_GetSurfaceAnim(pSurf);
    if ( !pAnim )
    {
        *result = rdroid_g_zeroVector3;
    }
    else
    {
        *result = pAnim->direction3;
    }
}

SithAnimationSlot* J3DAPI sithAnimate_GetAnim(int animID)
{
    for ( int i = 0; i <= lastUsedAnimNum; ++i )
    {
        if ( aAnims[i].flags != 0 && aAnims[i].animID == animID )
        {
            return &aAnims[i];
        }
    }

    return NULL;
}

SithAnimationSlot* J3DAPI sithAnimate_GetMaterialAnim(const rdMaterial* pMaterial)
{
    for ( int i = 0; i <= lastUsedAnimNum; ++i )
    {
        if ( aAnims[i].pMaterial == pMaterial )
        {
            return &aAnims[i];
        }
    }

    return NULL;
}

SithAnimationSlot* sithAnimate_Create(void)
{
    return sithAnimate_NewAnim();
}

int J3DAPI sithAnimate_Save(unsigned int outstream)
{
    if ( (outstream & sithMessage_g_outputstream) == 0 )
    {
        return 1;
    }

    int bError = 0;
    for ( int i = 0; i <= lastUsedAnimNum; ++i )
    {
        if ( aAnims[i].flags != 0
            && (aAnims[i].flags & SITHANIMATE_NOSYNC) == 0 // Added: Don't save NOSYNC anims
            && ((aAnims[i].flags & (SITHANIMATE_SPRITE | SITHANIMATE_THING)) == 0
                || !aAnims[i].pThing
                || sithThing_CanSyncThing(aAnims[i].pThing)) )// Hmm pThing could also be SithCamera type
        {
            bError = sithDSS_AnimStatus(&aAnims[i], 0, outstream);
            if ( bError )
            {
                break;
            }
        }
    }

    return bError;
}

void J3DAPI sithAnimate_UpdateCameraZoomAnim(SithAnimationSlot* pAnim, float secTimeDelta)
{
    bool bFinish = false;

    float newFOV = pAnim->curValue + pAnim->deltaValue * secTimeDelta;
    if ( pAnim->deltaValue < 0.0f && newFOV < (double)pAnim->endValue ) // End FOV lt new FOV
    {
        newFOV = pAnim->endValue;
        bFinish = true;
    }
    else if ( pAnim->deltaValue > 0.0f && newFOV > (double)pAnim->endValue ) // End FOV lt new FOV
    {
        newFOV = pAnim->endValue;
        bFinish = true;
    }

    pAnim->curValue = newFOV;
    sithCamera_SetCameraFOV(sithCamera_g_pCurCamera, newFOV);

    if ( bFinish )
    {
        sithAnimate_Stop(pAnim);
    }
}

void J3DAPI sithAnimate_UpdateSpriteSizeAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    bool bWidthEnd  = false;
    bool bHeightEnd = false;
    bool bAlphaEnd  = false;

    if ( !pAnim->pThing || (pAnim->pThing->flags & SITH_TF_DESTROYED) != 0 || (pAnim->pThing->flags & SITH_TF_DYING) != 0 )
    {
        sithAnimate_Stop(pAnim);
        return;
    }

    // Calculate new width
    pAnim->curVector.x += pAnim->deltaVector.x * secDeltaTime;
    if ( pAnim->deltaVector.x < 0.0f && pAnim->curVector.x <= (double)pAnim->endVector.x )
    {
        pAnim->curVector.x = pAnim->endVector.x;
        bWidthEnd = true;
    }
    else if ( pAnim->deltaVector.x >= 0.0f && pAnim->curVector.x >= (double)pAnim->endVector.x )
    {
        pAnim->curVector.x = pAnim->endVector.x;
        bWidthEnd = true;
    }

    // Calculate new height
    pAnim->curVector.y += pAnim->deltaVector.y * secDeltaTime;
    if ( pAnim->deltaVector.y < 0.0f && pAnim->curVector.y <= (double)pAnim->endVector.y )
    {
        pAnim->curVector.y = pAnim->endVector.y;
        bHeightEnd = true;
    }
    else if ( pAnim->deltaVector.y >= 0.0f && pAnim->curVector.y >= (double)pAnim->endVector.y )
    {
        pAnim->curVector.y = pAnim->endVector.y;
        bHeightEnd = true;
    }

    // Calculate new alpha
    pAnim->curVector.z += pAnim->deltaVector.z * secDeltaTime;
    if ( pAnim->deltaVector.z < 0.0f && pAnim->curVector.z <= (double)pAnim->endVector.z )
    {
        pAnim->curVector.z = pAnim->endVector.z;
        bAlphaEnd = true;
    }
    else if ( pAnim->deltaVector.z >= 0.0f && pAnim->curVector.z >= (double)pAnim->endVector.z )
    {
        pAnim->curVector.z = pAnim->endVector.z;
        bAlphaEnd = true;
    }

    pAnim->pThing->thingInfo.spriteInfo.width  = pAnim->curVector.x;
    pAnim->pThing->thingInfo.spriteInfo.height = pAnim->curVector.y;
    pAnim->pThing->thingInfo.spriteInfo.alpha  = pAnim->curVector.z;

    if ( bWidthEnd && bHeightEnd && bAlphaEnd )
    {
        sithAnimate_Stop(pAnim);
    }
}

void J3DAPI sithAnimate_UpdateLightAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    // Calculate new light color
    rdVector_MultAcc4(&pAnim->curVector, &pAnim->deltaVector, secDeltaTime);

    bool bRedEnd   = false;
    if ( pAnim->deltaVector.red == 0.0f )
    {
        bRedEnd = true;
    }

    bool bGreenEnd = false;
    if ( pAnim->deltaVector.green == 0.0f )
    {
        bGreenEnd = true;
    }

    bool bBlueEnd  = false;
    if ( pAnim->deltaVector.blue == 0.0f )
    {
        bBlueEnd = true;
    }

    bool bRangeEnd = false;
    if ( pAnim->deltaVector.w == 0.0f )
    {
        bRangeEnd = true;
    }

    if ( pAnim->deltaVector.red < 0.0f && pAnim->curVector.red < (double)pAnim->endVector.red )
    {
        pAnim->curVector.red = pAnim->endVector.red;
        bRedEnd = true;
    }
    else if ( pAnim->deltaVector.red > 0.0f && pAnim->curVector.red > (double)pAnim->endVector.red )
    {
        pAnim->curVector.red = pAnim->endVector.red;
        bRedEnd = true;
    }

    if ( pAnim->deltaVector.green < 0.0f && pAnim->curVector.green < (double)pAnim->endVector.green )
    {
        pAnim->curVector.green = pAnim->endVector.green;
        bGreenEnd = true;
    }
    else if ( pAnim->deltaVector.green > 0.0f && pAnim->curVector.green > (double)pAnim->endVector.green )
    {
        pAnim->curVector.green = pAnim->endVector.green;
        bGreenEnd = true;
    }

    if ( pAnim->deltaVector.blue < 0.0f && pAnim->curVector.blue < (double)pAnim->endVector.blue )
    {
        pAnim->curVector.blue = pAnim->endVector.blue;
        bBlueEnd = true;
    }
    else if ( pAnim->deltaVector.blue > 0.0f && pAnim->curVector.blue > (double)pAnim->endVector.blue )
    {
        pAnim->curVector.blue = pAnim->endVector.blue;
        bBlueEnd = true;
    }

    // Check if light range is in range
    if ( pAnim->deltaVector.w < 0.0f && pAnim->curVector.w < (double)pAnim->endVector.w )
    {
        pAnim->curVector.w = pAnim->endVector.w;
        bRangeEnd = true;
    }
    else if ( pAnim->deltaVector.w > 0.0f && pAnim->curVector.w > (double)pAnim->endVector.w )
    {
        pAnim->curVector.w = pAnim->endVector.w;
        bRangeEnd = true;
    }

    if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 && pAnim->pSurface )
    {
        pAnim->pSurface->face.extraLight = pAnim->curVector;
    }
    else if ( (pAnim->flags & SITHANIMATE_THING) != 0 && pAnim->pThing )
    {
        pAnim->pThing->light.color     = pAnim->curVector;
        pAnim->pThing->light.minRadius = pAnim->curVector.w;
        pAnim->pThing->light.maxRadius = 1.1f * pAnim->curVector.w;
    }
    else if ( (pAnim->flags & SITHANIMATE_SECTOR) != 0 && pAnim->pSector )
    {
        pAnim->pSector->extraLight = pAnim->curVector;
    }

    if ( bRedEnd && bGreenEnd && bBlueEnd && bRangeEnd )
    {
        if ( (pAnim->flags & SITHANIMATE_LOOP) != 0 )
        {
            rdVector_Neg4Acc(&pAnim->deltaVector);

            rdVector4 newStartVec = pAnim->endVector;
            pAnim->endVector      = pAnim->startVector;
            pAnim->startVector    = newStartVec;
        }
        else
        {
            sithAnimate_Stop(pAnim);
        }
    }
}

void J3DAPI sithAnimate_UpdateThingMoveAnim(SithAnimationSlot* pAnim, float secTimeDelta)
{
    float moveDist = pAnim->deltaValue * secTimeDelta;
    if ( (pAnim->curValue + moveDist) > (double)pAnim->endValue )
    {
        moveDist = pAnim->endValue - pAnim->curValue;
    }

    float distanceMoved = 0.0f;
    bool bFinish = pAnim->deltaValue == 0.0f;
    if ( stdMath_ClipNearZero(moveDist) == 0.0f )
    {
        bFinish = true;
    }
    else
    {
        distanceMoved = sithCollision_MoveThing(pAnim->pThing, &pAnim->direction3, moveDist, 0);
        if ( distanceMoved == 0.0f )
        {
            // Thing couldn't move to the new location, stop animation
            sithAnimate_Stop(pAnim);

            if ( (pAnim->pThing->flags & SITH_TF_COGLINKED) != 0 )
            {
                sithCog_ThingSendMessage(pAnim->pThing, 0, SITHCOG_MSG_BLOCKED);
            }

            if ( pAnim->pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
            {
                pAnim->pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            }

            return;
        }
    }

    // Update current distance
    pAnim->curValue += distanceMoved;

    if ( pAnim->curValue >= (double)pAnim->endValue || bFinish )
    {
        sithAnimate_Stop(pAnim);

        if ( (pAnim->pThing->flags & SITH_TF_COGLINKED) != 0 )
        {
            sithCog_ThingSendMessage(pAnim->pThing, 0, SITHCOG_MSG_ARRIVED);
        }

        if ( pAnim->pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
        {
            pAnim->pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        }
    }
}

void J3DAPI sithAnimate_UpdateThingMoveToPosAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    // Update current distance
    rdVector_Sub3(&pAnim->direction3, &pAnim->thingEndPosition, &pAnim->pThing->pos);

    float distance = rdVector_Normalize3Acc(&pAnim->direction3);

    if ( stdMath_ClipNearZero(distance) == 0.0f || stdMath_ClipNearZero(pAnim->secTimeRemaining) == 0.0f )
    {
        // Thing arrived at position, finish animation

        if ( (pAnim->pThing->flags & SITH_TF_COGLINKED) != 0 )
        {
            sithCog_ThingSendMessage(pAnim->pThing, 0, SITHCOG_MSG_ARRIVED);
        }

        if ( (pAnim->pThing->type == SITH_THING_PLAYER || pAnim->pThing->type == SITH_THING_ACTOR) && pAnim->pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
        {
            pAnim->pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        }

        sithAnimate_Stop(pAnim);
        return;
    }

    float moveDist = (float)((double)distance / (double)pAnim->secTimeRemaining * secDeltaTime);
    if ( sithCollision_MoveThing(pAnim->pThing, &pAnim->direction3, moveDist, 0) == 0.0f )
    {
        // Thing couldn't move to the new position, stop animation

        if ( (pAnim->pThing->flags & SITH_TF_COGLINKED) != 0 )
        {
            sithCog_ThingSendMessage(pAnim->pThing, 0, SITHCOG_MSG_BLOCKED);
        }

        if ( (pAnim->pThing->type == SITH_THING_PLAYER || pAnim->pThing->type == SITH_THING_ACTOR) && pAnim->pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
        {
            pAnim->pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        }

        sithAnimate_Stop(pAnim);
        return;
    }

    // Update remaining time
    pAnim->secTimeRemaining = J3DMAX(pAnim->secTimeRemaining - secDeltaTime, 0.0f);
}

void J3DAPI sithAnimate_UpdateThingFadeAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    rdVector4 newColor;
    if ( pAnim->pThing->type == SITH_THING_POLYLINE )
    {
        newColor = pAnim->startVector;
    }

    bool bFinish = false;
    newColor.alpha = pAnim->deltaVector.alpha * secDeltaTime + pAnim->curVector.alpha;
    if ( pAnim->deltaVector.alpha == 0.0f )
    {
        bFinish = true;
    }

    if ( pAnim->deltaVector.alpha < 0.0f && newColor.alpha <= (double)pAnim->endVector.alpha )
    {
        newColor.alpha = pAnim->endVector.alpha;
        bFinish = true;
    }
    else if ( pAnim->deltaVector.alpha > 0.0f && newColor.alpha >= (double)pAnim->endVector.alpha )
    {
        newColor.alpha = pAnim->endVector.alpha;
        bFinish = true;
    }

    // Update cur fade state
    pAnim->curVector = newColor;

    // Set thing new alpha
    pAnim->pThing->alpha = newColor.alpha;

    if ( bFinish )
    {
        if ( (pAnim->flags & SITHANIMATE_LOOP) != 0 )
        {
            // Loop the animation around
            pAnim->deltaVector.alpha = -pAnim->deltaVector.alpha;

            rdVector4 newStartVec = pAnim->endVector;
            pAnim->endVector      = pAnim->startVector;
            pAnim->startVector    = newStartVec;
        }
        else
        {
            sithAnimate_Stop(pAnim);
        }
    }
}

void J3DAPI sithAnimate_UpdateSurfaceScrollAnim(SithAnimationSlot* pAnim, float secDeltaTime, int animNum)
{
    SITH_ASSERTREL(pAnim && (pAnim->pSurface));

    if ( pAnim->pSurface )
    {
        // Shift the texture coordinates
        rdVector_MultAcc2(&pAnim->pSurface->face.texVertOffset, &pAnim->direction2, secDeltaTime);
        if ( SITH_ISFRAMECYCLE(animNum, 16) ) // every 16th frame
        {
            // Wrap texture coordinates
            pAnim->pSurface->face.texVertOffset.x = fmodf(pAnim->pSurface->face.texVertOffset.x, 1024.0f);
            pAnim->pSurface->face.texVertOffset.y = fmodf(pAnim->pSurface->face.texVertOffset.y, 1024.0f);
        }
    }
}

void J3DAPI sithAnimate_UpdateSlideSkyAnim(SithAnimationSlot* pAnim, int skyType, float secDeltaTime, int animNum)
{
    SITH_ASSERTREL(pAnim);
    SITH_ASSERTREL(skyType == SITH_SURFACE_HORIZONSKY || skyType == SITH_SURFACE_CEILINGSKY);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;

    if ( skyType == SITH_SURFACE_HORIZONSKY )
    {
        pWorld->horizonSkyOffset.x = pAnim->direction2.x * secDeltaTime + pWorld->horizonSkyOffset.x;
        pWorld->horizonSkyOffset.y = pAnim->direction2.y * secDeltaTime + pWorld->horizonSkyOffset.y;

        rdVector_MultAcc2(&pWorld->horizonSkyOffset, &pAnim->direction2, secDeltaTime);
        if ( SITH_ISFRAMECYCLE(animNum, 16) )
        {
            // Wrap texture coordinates
            pWorld->horizonSkyOffset.x = fmodf(pWorld->horizonSkyOffset.x, 1024.0f);
            pWorld->horizonSkyOffset.y = fmodf(pWorld->horizonSkyOffset.y, 1024.0f);
        }
    }
    else
    {
        rdVector_MultAcc2(&pWorld->ceilingSkyOffset, &pAnim->direction2, secDeltaTime);
        if ( SITH_ISFRAMECYCLE(animNum, 16) )
        {
            // Wrap texture coordinates
            pWorld->ceilingSkyOffset.x = fmodf(pWorld->ceilingSkyOffset.x, 1024.0f);
            pWorld->ceilingSkyOffset.y = fmodf(pWorld->ceilingSkyOffset.y, 1024.0f);
        }
    }
}

void J3DAPI sithAnimate_UpdatePageFlipAnim(SithAnimationSlot* pAnim)
{
    SITH_ASSERTREL(pAnim);
    SITH_ASSERTREL(pAnim->msecPerFrame && pAnim->pMaterial);

    uint32_t msecDeltaTime = sithTime_g_msecGameTime - pAnim->msecNextFrameTime;

    bool bFinish = false;
    if ( pAnim->msecPerFrame != 0 && pAnim->pMaterial )
    {
        pAnim->curFrame += msecDeltaTime / pAnim->msecPerFrame + 1;
        pAnim->msecNextFrameTime = pAnim->msecPerFrame + sithTime_g_msecGameTime - msecDeltaTime % pAnim->msecPerFrame;

        if ( pAnim->curFrame >= pAnim->pMaterial->numCels )
        {
            if ( (pAnim->flags & SITHANIMATE_LOOP) != 0 )
            {
                size_t startFrame = 0;
                if ( (pAnim->flags & SITHANIMATE_START_SECOND_FRAME) != 0 )
                {
                    startFrame = 1;
                }
                else if ( (pAnim->flags & SITHANIMATE_START_THIRD_FRAME) != 0 )
                {
                    startFrame = 2;
                }

                pAnim->curFrame = startFrame + (pAnim->curFrame - startFrame) % (pAnim->pMaterial->numCels - startFrame);
                if ( pAnim->curFrame > pAnim->pMaterial->numCels - 1 )
                {
                    pAnim->curFrame = pAnim->pMaterial->numCels - 1;
                }
            }
            else
            {
                pAnim->curFrame = pAnim->pMaterial->numCels - 1;
                if ( (pAnim->flags & SITHANIMATE_FINISH_FIRST_FRAME) != 0 )
                {
                    pAnim->curFrame = 0;
                }

                bFinish = true;
            }
        }

        if ( (pAnim->flags & SITHANIMATE_SPRITE) != 0 )
        {
            SITH_ASSERTREL(pAnim->pThing);
            pAnim->pThing->renderData.matCelNum = pAnim->curFrame;
        }
        else if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 )
        {
            SITH_ASSERTREL(pAnim->pSurface);
            SITH_ASSERTREL(pAnim->curFrame < pAnim->pSurface->face.pMaterial->numCels);

            pAnim->pSurface->flags         |= SITH_SURFACE_SYNC;
            pAnim->pSurface->face.matCelNum = pAnim->curFrame;
        }
        else if ( (pAnim->flags & SITHANIMATE_MATERIAL) != 0 )
        {
            SITH_ASSERTREL(pAnim->pMaterial);
            SITH_ASSERTREL(pAnim->curFrame < pAnim->pMaterial->numCels);
            pAnim->pMaterial->curCelNum = pAnim->curFrame;
        }
        else
        {
            SITHLOG_ERROR("Unexpected flags for pageflip animation, 0x%x.\n", pAnim->flags);
        }

        if ( bFinish )
        {
            sithAnimate_FreeAnim(pAnim);
        }
    }
}

void sithAnimate_FreeAllAnims(void)
{
    memset(aAnims, 0, sizeof(aAnims));

    for ( int i = STD_ARRAYLEN(aFreeAnimNums) - 1; i >= 0; --i )
    {
        aFreeAnimNums[i] = STD_ARRAYLEN(aFreeAnimNums) - i - 1;
    }

    numFreeAnims    = STD_ARRAYLEN(aFreeAnimNums);
    lastUsedAnimNum = 0;
}

void J3DAPI sithAnimate_FreeAnim(SithAnimationSlot* pAnim)
{
    size_t animNum = pAnim - &aAnims[0];
    SITH_ASSERTREL(animNum < STD_ARRAYLEN(aAnims));

    SITH_ASSERTREL(numFreeAnims < STD_ARRAYLEN(aAnims));

    pAnim->flags = 0;
    aFreeAnimNums[numFreeAnims++] = animNum;

    if ( (int)animNum == lastUsedAnimNum )
    {
        int i;
        for ( i = (int)animNum - 1; i >= 0 && !aAnims[i].flags; --i )
        {
            ;
        }

        lastUsedAnimNum = i;
    }
}

SithAnimationSlot* sithAnimate_NewAnim(void)
{
    if ( numFreeAnims == 0 )
    {
        return NULL;
    }

    size_t animNum = aFreeAnimNums[--numFreeAnims];
    if ( (int)animNum > lastUsedAnimNum )
    {
        lastUsedAnimNum = aFreeAnimNums[numFreeAnims];
    }

    // Reset animation
    SithAnimationSlot* pAnim = &aAnims[animNum];
    memset(pAnim, 0, sizeof(SithAnimationSlot));

    // Set animation ID
    pAnim->animID = (uint16_t)animNum | ((sithPlayer_g_playerNum + 1) << 16);

    return pAnim;
}

void J3DAPI sithAnimate_UpdatePushItemAnim(SithAnimationSlot* pAnim)
{
    float curFrame = sithAnimate_GetPuppetTrackCurFrame(pAnim->pThing, pAnim->trackNum);

    bool bFinish = false;
    float moveDist = (float)((double)pAnim->endValue - (double)pAnim->endVector.x); // endVector.x holds current distance item was pushed for 
    if ( curFrame < (double)pAnim->deltaValue && moveDist > 0.0f ) // deltaValue holds last processed keyframe frame
    {
        // Animation completed

        // Make item pushable by removing it's surface drag
        float surfDrag = pAnim->pItemThing->moveInfo.physics.surfDrag;
        pAnim->pItemThing->moveInfo.physics.surfDrag = 0.0f;

        // One final item push
        rdVector3 moveDir = { 0 }; // Fixed: Init to zero; TODO: this must be a bug as it should be a copy of pAnim->direction3
        sithCollision_MoveThing(pAnim->pItemThing, &moveDir, moveDist, 0);

        // Restore item surface drag, and mark animation as finished
        pAnim->pItemThing->moveInfo.physics.surfDrag = surfDrag;
        bFinish = true;
    }
    else
    {
        pAnim->deltaValue = curFrame;
        size_t frameIdx = sithAnimate_GetAnimFrameIndex(curFrame, lastPushItemAnimFrameIdx, &aPushItemAnimFrames[1]);

        if ( (double)aPushItemAnimFrames[lastPushItemAnimFrameIdx] < curFrame )
        {
            // Animation completed
            bFinish = true;
        }
        else
        {
            double frameDelta = curFrame - (double)aPushItemAnimFrames[frameIdx + 1]; // curFrame - nextPushFrame
            double frameDistance = aPushItemDistances[frameIdx].offsetFactor * frameDelta;
            frameDistance += aPushItemDistances[frameIdx].distance;

            float distDelta = (float)(frameDistance - pAnim->endVector.x); // endVector.x holds current distance item was pushed for 

            rdVector3 moveDir = pAnim->direction3;
            if ( distDelta > 0.0f )
            {
                if ( distDelta + pAnim->endVector.x <= pAnim->endValue ) // endVector.x holds current distance item was pushed for 
                {
                    moveDist = distDelta;
                }
                else
                {
                    // Animation completed, but we need to push item one last time to it's final position
                    bFinish = true;
                    moveDist = (float)((double)distDelta + (double)pAnim->endVector.x - (double)pAnim->endValue);
                }

                // Push item
                if ( pAnim->pItemThing )
                {
                    // Make item pushable by removing it's surface drag
                    float surfDrag = pAnim->pItemThing->moveInfo.physics.surfDrag;
                    pAnim->pItemThing->moveInfo.physics.surfDrag = 0.0f;

                    double distPushed = sithCollision_MoveThing(pAnim->pItemThing, &moveDir, moveDist, 0);
                    pAnim->endVector.x = (float)(pAnim->endVector.x + distPushed); // endVector.x holds current distance item was pushed for 

                    // Restore item surface drag
                    pAnim->pItemThing->moveInfo.physics.surfDrag = surfDrag;
                }
            }
        }
    }

    if ( pAnim->endVector.x >= (double)pAnim->endValue || bFinish )// endVector.x holds current distance item was pushed for 
    {
        // Animation completed

        // Fixed: Added check for pAnim->pItemThing
        if ( pAnim->pItemThing )
        {
            pAnim->pItemThing->collide.type = pAnim->msecPerFrame;
            sithAnimate_FindAndEnterFloorSurface(pAnim->pItemThing, pAnim->pSurface);// pSurface is item thing start surface
        }

        sithAnimate_Stop(pAnim);
    }
}

void J3DAPI sithAnimate_UpdatePullItemAnim(SithAnimationSlot* pAnim)
{
    float frame = sithAnimate_GetPuppetTrackCurFrame(pAnim->pThing, pAnim->trackNum);

    bool bFinish = false;
    float moveDist = (float)((double)pAnim->endValue - (double)pAnim->endVector.x); // endVector.x holds current distance item was pulled for 
    if ( frame < (double)pAnim->deltaValue && moveDist > 0.0f ) // deltaValue holds current keyframe animation frame
    {
        // Animation completed

        // Make item pushable by removing it's surface drag
        float surfdrag = pAnim->pItemThing->moveInfo.physics.surfDrag;
        pAnim->pItemThing->moveInfo.physics.surfDrag = 0.0f;

        // One final item pull
        rdVector3 moveDir = { 0 }; // Fixed: Init to zero; TODO: this must be a bug as it should be a copy of pAnim->direction3
        sithCollision_MoveThing(pAnim->pItemThing, &moveDir, moveDist, 0);

        // Restore item surface drag, and mark animation as finished
        pAnim->pItemThing->moveInfo.physics.surfDrag = surfdrag;
        bFinish = true;
    }
    else
    {
        pAnim->deltaValue = frame;
        size_t frameIdx = sithAnimate_GetAnimFrameIndex(frame, lastPullItemAnimFrameIdx, &aPullItemAnimFrames[1]);
        if ( (double)aPullItemAnimFrames[lastPullItemAnimFrameIdx] < frame )
        {
            // Animation completed
            bFinish = true;
        }
        else
        {
            double frameDelta = frame - (double)aPullItemAnimFrames[frameIdx + 1]; // cur frame - next pull anim frame
            double frameDistance = aPullItemDistances[frameIdx].offsetFactor * frameDelta;
            frameDistance = fabs(aPullItemDistances[frameIdx].distance + frameDistance); // offsets can be negative

            float distDelta = (float)(frameDistance - pAnim->endVector.x); // endVector.x holds current distance item was pulled for 

            rdVector3 moveDir = pAnim->direction3;
            if ( distDelta > 0.0f )
            {
                if ( distDelta + pAnim->endVector.x <= pAnim->endValue )// endVector.x holds current distance item was pulled for 
                {
                    moveDist = distDelta;
                }
                else
                {
                    bFinish = 1;
                    moveDist = (float)((double)distDelta + (double)pAnim->endVector.x - (double)pAnim->endValue);
                }

                // Pull item
                if ( pAnim->pItemThing )
                {
                    // Make item pullable by removing it's surface drag
                    float surfdrag = pAnim->pItemThing->moveInfo.physics.surfDrag;
                    pAnim->pItemThing->moveInfo.physics.surfDrag = 0.0f;

                    double distPulled = sithCollision_MoveThing(pAnim->pItemThing, &moveDir, moveDist, 0);
                    pAnim->endVector.x = (float)(pAnim->endVector.x + distPulled); // endVector.x holds current distance item was pulled for 

                    // Restore item surface drag
                    pAnim->pItemThing->moveInfo.physics.surfDrag = surfdrag;
                }
            }
        }
    }

    if ( pAnim->endVector.x >= (double)pAnim->endValue || bFinish ) // endVector.x holds current distance item was pulled for 
    {
        // Animation completed

        // Fixed: Added check for pAnim->pItemThing
        if ( pAnim->pItemThing )
        {
            pAnim->pItemThing->collide.type = pAnim->msecPerFrame;
            sithAnimate_FindAndEnterFloorSurface(pAnim->pItemThing, pAnim->pSurface); // pSurface is item thing start surface
        }

        sithAnimate_Stop(pAnim);
    }
}

size_t J3DAPI sithAnimate_GetAnimFrameIndex(float frame, size_t size, const size_t* aAnimFrames)
{
    size_t index = 0;
    for ( size_t i = 1; i < size && aAnimFrames[i] <= (size_t)frame; ++i )
    {
        index = i;
    }

    return index;
}

float J3DAPI sithAnimate_GetPuppetTrackCurFrame(SithThing* pThing, int trackNum)
{
    return pThing->renderData.pPuppet->aTracks[trackNum].curFrame;
}

SithSurface* J3DAPI sithAnimate_GetThingFloorSurface(const SithThing* pThing)
{
    rdVector3 moveNorm;
    rdVector_Neg3(&moveNorm, &rdroid_g_zVector3);

    float moveDist = pThing->collide.movesize * 4.0f;
    float radius   = pThing->collide.movesize / 2.0f;

    sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, moveDist, radius, 0x2812);

    while ( 1 )
    {
        SithCollision* pCollision = sithCollision_PopStack();
        if ( !pCollision )
        {
            break;
        }

        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            SithSurface* pSurfaceCollided = pCollision->pSurfaceCollided;
            sithCollision_DecreaseStackLevel();
            return pSurfaceCollided;
        }
    }

    sithCollision_DecreaseStackLevel();

    return NULL;
}

void J3DAPI sithAnimate_ExitFloorSurface(const SithThing* pThing, const SithSurface* pSurface)
{
    if ( pSurface && pThing && (pSurface->flags & SITH_SURFACE_COGLINKED) != 0 )
    {
        sithCog_SurfaceSendMessage(pSurface, pThing, SITHCOG_MSG_EXITED);
    }
}

void J3DAPI sithAnimate_FindAndEnterFloorSurface(const SithThing* pThing, const SithSurface* pPrevSurf)
{
    rdVector3 moveNorm;
    rdVector_Neg3(&moveNorm, &rdroid_g_zVector3);

    float moveDist = pThing->collide.movesize * 4.0f;
    float radius   = pThing->collide.movesize / 2.0f;

    sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, moveDist, radius, 0x2812);

    SithSurface* pSurfaceCollided = NULL;
    while ( 1 )
    {
        SithCollision* pCollision = sithCollision_PopStack();
        if ( !pCollision )
        {
            break;
        }

        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            pSurfaceCollided = pCollision->pSurfaceCollided;
            if ( pSurfaceCollided != pPrevSurf )
            {
                break;
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    // Send entered message to found surface
    if ( pSurfaceCollided )
    {
        if ( (pSurfaceCollided->flags & SITH_SURFACE_COGLINKED) != 0 )
        {
            sithCog_SurfaceSendMessage(pSurfaceCollided, pThing, SITHCOG_MSG_ENTERED);
        }
    }
}