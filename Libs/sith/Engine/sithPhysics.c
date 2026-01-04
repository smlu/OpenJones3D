#include "sithPhysics.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>

#include <sith/AI/sithAIAwareness.h>
#include <sith/Cog/sithCog.h>

#include <sith/Devices/sithControl.h>

#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>

#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>

#include <sith/World/sithMaterial.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>
#include <sith/RTI/symbols.h>

#include <std/General/stdConfig.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

//
// Physics constants
//
#define SITHPHYSICS_FIXED_FRAMERATE_UNCAP 150.0f
#define SITHPHYSICS_FIXED_FRAMERATE_DFLT  50.0f
#define SITHPHYSICS_FIXED_TIMESTEP_DFLT   (1.0f/SITHPHYSICS_FIXED_FRAMERATE_DFLT)

#define SITHPHYSICS_CROUCH_THRUST_SCALE 0.80000001f

#define sithPhysics_flt_538D04 J3D_DECL_FAR_VAR(sithPhysics_flt_538D04, float)
#define sithPhysics_dword_538D2C J3D_DECL_FAR_VAR(sithPhysics_dword_538D2C, int)
#define sithPhysics_dword_538D30 J3D_DECL_FAR_VAR(sithPhysics_dword_538D30, int)
#define sithPhysics_bJeepMoveFx J3D_DECL_FAR_VAR(sithPhysics_bJeepMoveFx, int)
#define sithPhysics_dword_538D38 J3D_DECL_FAR_VAR(sithPhysics_dword_538D38, int)
#define sithPhysics_dword_58540C J3D_DECL_FAR_VAR(sithPhysics_dword_58540C, int)
#define sithPhysics_flt_585410 J3D_DECL_FAR_VAR(sithPhysics_flt_585410, float)

// Added
static float sithPhysics_fixedFramerate = SITHPHYSICS_FIXED_FRAMERATE_DFLT;
static float sithPhysics_fixedTimestep  = SITHPHYSICS_FIXED_TIMESTEP_DFLT;

//
// MineCar fx vars
//
static float sithPhysics_mineCarEngineVolume       = 0.5f;
static float sithPhysics_mineCarEngineMinPitch     = 1.0f;
static float sithPhysics_mineCarEngineMaxPitch     = 1.15f;
static float sithPhysics_mineCarEngineMinVolume    = 0.5f;
static float sithPhysics_mineCarEngineVolIncrement = 0.5f;

static float sithPhysics_mineCarEngineAnimFPS   = 18.0f;
static float sithPhysics_mineCarEngineAnimScale = 10.0f;

static float sithPhysics_mineCarRumbleMinVolume = 0.050000001f;
static float sithPhysics_mineCarRumbleMaxVolume = 0.95f;
static float sithPhysics_mineCarRumbleFadeTime  = 0.60000002f;

static float sithPhysics_mineCarSparkMinSpeed      = 0.30000001f;
static float sithPhysics_mineCarSparkMinAngularVel = 20.0f; // in degrees
static float sithPhysics_mineCarSparkVolScale      = 0.5f;
static float sithPhysics_mineCarSparkMinVolume     = 0.050000001f;
static float sithPhysics_mineCarSparkVolIncrement  = 2.0f;
static float sithPhysics_mineCarSparkFadeVolume    = 0.5f;
static float sithPhysics_mineCarSparkInterval      = 0.050000001f;

static rdVector3 sithPhysics_mineCarSparkPosRight = { 0.050000001f, -0.039999999f, -0.090000004f };
static float sithPhysics_mineCarSparkIntervalLeft = 0.050000001f;
static rdVector3 sithPhysics_mineCarSparkPosLeft  = { -0.050000001f, -0.039999999f, -0.090000004f };

static float sithPhysics_mineCarClatterMinVolume  = 0.1f;
static float sithPhysics_mineCarClatterVolScale   = 1.0f;
static float sithPhysics_mineCarClatterFadeEndVol = 0.5f;
static float sithPhysics_mineCarClatterFadeTime   = 0.2f;

static float sithPhysics_mineCarRailClackMaxDist        = 3.0f;
static float sithPhysics_mineCarRailClackMinDist        = 0.1f;
static float sithPhysics_mineCarRailClackDistVariation  = 0.1f;
static float sithPhysics_mineCarRailClackVolScale       = 1.0f;
static float sithPhysics_mineCarRailClackMinVolume      = 0.1f;
static float sithPhysics_mineCarRailClackPitchVariation = 0.050000001f;

static bool sithPhysics_bMineCarEngineRunFx    = true;
static bool sithPhysics_bMineCarEngineRumbleFx = true;
static bool sithPhysics_bMineCarSparksFx       = true;
static bool sithPhysics_bMineCarClatterFx      = true;
static bool sithPhysics_bMineCarRailClackFx    = true;

// Track jeep fx vars
static bool sithPhysics_bUpdateTrackJeepEngineFx = true;
static float sithPhysics_trackJeepMinSndPitch    = 1.0f;
static float sithPhysics_trackJeepMaxSndPitch    = 2.0f;
static float sithPhysics_trackJeepMinSndVolume   = 0.60000002f;
static float sithPhysics_trackJeepMaxSndVolume   = 1.0f;
static float sithPhysics_trackJeepMaxSpeed       = 1.0f;

// Track truck fx vars
static bool sithPhysics_bUpdateTrackTruckEngineFx = true;
static float sithPhysics_trackTruckMinSndPitch    = 0.52f;
static float sithPhysics_trackTruckMaxSndPitch    = 1.0f;
static float sithPhysics_trackTruckMinSndVolume   = 0.5f;
static float sithPhysics_trackTruckMaxSndVolume   = 1.0f;
static float sithPhysics_trackTruckMaxSpeed       = 1.0f;

//
// MineCar physics vars
//
static float sithPhysics_mineCarMaxSpeed         = 1.25f;
static float sithPhysics_trackSearchHeightOffset = 0.2f;
static float sithPhysics_trackSearchDistance     = 0.5f;

//
// Raft physics vars
//
static float sithPhysics_raftWaterSurfaceSearchDist = 1.5f;
static float sithPhysics_waterSurfaceSearchOffset   = 0.018999999f;
static float sithPhysics_raftHeightAdjustSpeed      = 0.050000001f;
static float sithPhysics_raftSteepSlopeFactor       = 0.30000001f;
static float sithPhysics_raftSolidSurfaceDragFactor = 1.1f;

//
// Jeep physics vars
//
static bool sithPhysics_bJeepExhaust; // Added: new var

// Vehicle light on/off helper function
static void J3DAPI sithPhysics_SetVehicleLight(SithVehicleLight* pLight, bool bOn);
static void J3DAPI sithPhysics_SetVehicleLights(SithVehicleLights* pLights, bool bOn);

// MineCar fx init functions
void J3DAPI sithPhysics_InitMineCarFxState(SithThing* pThing, SithMineCarFxState* pFxState);
void J3DAPI sithPhysics_InitVehicleFxState(SithThing* pThing, SithVehicleEngineFxState* pFxState);

void J3DAPI sithPhysics_InitMineCarChassisDefault(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo);
void J3DAPI sithPhysics_InitMineCarChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo);
void J3DAPI sithPhysics_InitTrackTruckChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo);
void J3DAPI sithPhysics_InitTrackJeepChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo);

void J3DAPI sithPhysics_InitMineCarExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo);
void J3DAPI sithPhysics_InitJeepExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo);
void J3DAPI sithPhysics_InitTrackTruckExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo);
void J3DAPI sithPhysics_InitMineCarExhaustDefault(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo);

void J3DAPI sithPhysics_InitMineCarState(SithThing* pThing, SithMineCarState* pState);
void J3DAPI sithPhysics_InitTrackTruckState(SithThing* pThing, SithMineCarState* pState);

void J3DAPI sithPhysics_InitMineCarLights(SithThing* pThing, SithVehicleLights* pLights);

// MineCar fx update functions
void J3DAPI sithPhysics_UpdateTrackVehicleFx(SithThing* pThing, SithMineCarUserBlock* pMineCarUserBlock, float secDeltaTime);
void J3DAPI sithPhysics_UpdateMineCarFx(SithThing* pThing, SithMineCarFxState* pFxState, float secDeltaTime);
void J3DAPI sithPhysics_UpdateTrackJeepFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float secDeltaTime);
void J3DAPI sithPhysics_UpdateTrackTruckFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float secDeltaTime);
void J3DAPI sithPhysics_UpdateMineCarChassis(SithThing* pThing, const SithVehicleChassisInfo* pChasisInfo, float secDeltaTime);
void J3DAPI sithPhysics_UpdateExhaustFx(SithThing* pThing, const SithVehicleExhaustInfo* pExhaustInfo, float secDeltaTime);

// Minecar turn on/off functions (new)
void J3DAPI sithPhysics_PowerOnMineCar(SithThing* pThing);
void J3DAPI sithPhysics_PowerOffMineCar(SithThing* pThing);

// Raft physics
SithPhysicsWaterSurfaceType J3DAPI sithPhysics_CheckWaterSurfaceAtPos(const SithThing* pThing, float* pOutDistance, rdVector3* pOutNormal, const rdVector3* pPos, float secDeltaTime);
void J3DAPI sithPhysics_ApplyWaterThrust(SithThing* pThing, const rdVector3* pWaterSurfNormal, rdVector3* pThrust, float secDeltaTime);

void J3DAPI sithPhysics_GetThingAttachNormalAndDistance(const SithThing* pThing, rdVector3* pAttachNormal, float* pDistToAttach, float* pScale); // Added: from debug

// MineCar track physics helpers
void J3DAPI sithPhysics_ProcessMineCarTrackMove(SithThing* pThing, float secDeltaTime, float a3, rdVector3* a4);
int J3DAPI sithPhysics_ProcessTrackFace(SithThing* pThing, rdFace* pFace, rdVector3* a3, rdVector3* a4, rdVector3* a5, void* pData);
int J3DAPI sithPhysics_CheckForPointOnTrack(SithThing* pThing, const rdVector3* pPoint, rdFace** ppFoundFace, int bUpdateState, const rdFace* pPrevFace);
int J3DAPI sithPhysics_sub_487EC0(SithThing* pThing, void* pData, float* secDeltaTime, rdVector3* a4, rdVector3* a5, float a6);

// Jeep physics
int J3DAPI sithPhysics_Jeep_sub_4892E0(SithThing* pThing, void* a2, float secDeltaTime);
// function might check for jeep surface and thing face collision
void J3DAPI sithPhysics_sub_48A970(SithThing* pThing, void* a2, const rdVector3* a3);
int J3DAPI sithPhysics_sub_48AD20(SithThing* pThing, void* a2, rdVector3* a3, float secDeltaTime);
void J3DAPI sithPhysics_Jeep_sub_48B4D0(SithThing* pThing, float* a2);
void J3DAPI sithPhysics_UpdateJeepFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float a3);

void sithPhysics_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(sithPhysics_FindFloor);
    J3D_HOOKFUNC(sithPhysics_FindWaterSurface);
    J3D_HOOKFUNC(sithPhysics_UpdateThing);
    J3D_HOOKFUNC(sithPhysics_ApplyForce);
    J3D_HOOKFUNC(sithPhysics_SetThingLook);
    J3D_HOOKFUNC(sithPhysics_ApplyDrag);
    J3D_HOOKFUNC(sithPhysics_ParseArg);
    J3D_HOOKFUNC(sithPhysics_ResetThingMovement);
    J3D_HOOKFUNC(sithPhysics_GetThingHeight);
    J3D_HOOKFUNC(sithPhysics_UpdateDetachedThingPhysics);
    J3D_HOOKFUNC(sithPhysics_UpdateDetachedPlayerPhysics);
    J3D_HOOKFUNC(sithPhysics_UpdateUnderwaterThingPhysics);
    J3D_HOOKFUNC(sithPhysics_UpdateClimbingThingPhysics);
    J3D_HOOKFUNC(sithPhysics_UpdateAttachedThingPhysics);
    J3D_HOOKFUNC(sithPhysics_CreateMineCarUserBlock);
    J3D_HOOKFUNC(sithPhysics_InitMineCarFxState);
    J3D_HOOKFUNC(sithPhysics_InitVehicleFxState);
    J3D_HOOKFUNC(sithPhysics_InitMineCarChassisDefault);
    J3D_HOOKFUNC(sithPhysics_InitMineCarChassis);
    J3D_HOOKFUNC(sithPhysics_InitTrackTruckChassis);
    J3D_HOOKFUNC(sithPhysics_InitTrackJeepChassis);
    J3D_HOOKFUNC(sithPhysics_InitMineCarExhaust);
    J3D_HOOKFUNC(sithPhysics_InitJeepExhaust);
    J3D_HOOKFUNC(sithPhysics_InitTrackTruckExhaust);
    J3D_HOOKFUNC(sithPhysics_InitMineCarState);
    J3D_HOOKFUNC(sithPhysics_InitTrackTruckState);
    J3D_HOOKFUNC(sithPhysics_UpdateMineCarPhysics);
    J3D_HOOKFUNC(sithPhysics_UpdateTrackVehicleFx);
    J3D_HOOKFUNC(sithPhysics_UpdateMineCarFx);
    J3D_HOOKFUNC(sithPhysics_UpdateTrackJeepFx);
    J3D_HOOKFUNC(sithPhysics_UpdateTrackTruckFx);
    J3D_HOOKFUNC(sithPhysics_UpdateMineCarChassis);
    J3D_HOOKFUNC(sithPhysics_UpdateExhaustFx);
    J3D_HOOKFUNC(sithPhysics_UpdateRaftPhysics);
    J3D_HOOKFUNC(sithPhysics_CheckWaterSurfaceAtPos);
    J3D_HOOKFUNC(sithPhysics_ApplyWaterThrust);
    // J3D_HOOKFUNC(sithPhysics_ProcessMineCarTrackMove);
    // J3D_HOOKFUNC(sithPhysics_ProcessTrackFace);
    J3D_HOOKFUNC(sithPhysics_CheckForPointOnTrack);
    // J3D_HOOKFUNC(sithPhysics_sub_487EC0);
    // J3D_HOOKFUNC(sithPhysics_UpdateJeepPhysics);
    // J3D_HOOKFUNC(sithPhysics_Jeep_sub_4892E0);
    // J3D_HOOKFUNC(sithPhysics_sub_48A970);
    // J3D_HOOKFUNC(sithPhysics_sub_48AD20);
    // J3D_HOOKFUNC(sithPhysics_CreateJeepUserBlock);
    // J3D_HOOKFUNC(sithPhysics_Jeep_sub_48B4D0);
    // J3D_HOOKFUNC(sithPhysics_UpdateJeepFx);
}

void sithPhysics_ResetGlobals(void)
{
    float sithPhysics_flt_538D04_tmp = 0.60000002f;
    memcpy(&sithPhysics_flt_538D04, &sithPhysics_flt_538D04_tmp, sizeof(sithPhysics_flt_538D04));

    int sithPhysics_dword_538D2C_tmp = 1;
    memcpy(&sithPhysics_dword_538D2C, &sithPhysics_dword_538D2C_tmp, sizeof(sithPhysics_dword_538D2C));

    int sithPhysics_dword_538D30_tmp = 1;
    memcpy(&sithPhysics_dword_538D30, &sithPhysics_dword_538D30_tmp, sizeof(sithPhysics_dword_538D30));

    int sithPhysics_bJeepMoveFx_tmp = 1;
    memcpy(&sithPhysics_bJeepMoveFx, &sithPhysics_bJeepMoveFx_tmp, sizeof(sithPhysics_bJeepMoveFx));

    int sithPhysics_dword_538D38_tmp = 1;
    memcpy(&sithPhysics_dword_538D38, &sithPhysics_dword_538D38_tmp, sizeof(sithPhysics_dword_538D38));

    STD_ZEROMEM(&sithPhysics_dword_58540C, sizeof(sithPhysics_dword_58540C));
    STD_ZEROMEM(&sithPhysics_flt_585410, sizeof(sithPhysics_flt_585410));
}

void J3DAPI sithPhysics_Startup(void)
{
#ifdef J3D_QOL_IMPROVEMENTS
    // Fixed timestap
    sithPhysics_fixedFramerate = stdConfig_GetFloat(SITHPHYSICS_CFG_PHYSICS_FIXEDTIMESTEP, SITHPHYSICS_FIXED_FRAMERATE_UNCAP);
    sithPhysics_fixedTimestep  = 1.0f / sithPhysics_fixedFramerate;

    if ( !stdConfig_Contains(SITHPHYSICS_CFG_PHYSICS_FIXEDTIMESTEP) )
    {
        stdConfig_SetFloat(SITHPHYSICS_CFG_PHYSICS_FIXEDTIMESTEP, SITHPHYSICS_FIXED_FRAMERATE_UNCAP);
    }

    // Jeep exhaust
    sithPhysics_bJeepExhaust = true;

#else
    sithPhysics_bJeepExhaust = false;
#endif 
}

void J3DAPI sithPhysics_FindFloor(SithThing* pThing, int bNoSurfaceImpactUpdate)
{
    if ( !pThing )
    {
        SITHLOG_ERROR("NULL thing pointer in FindFloor()");
        return;
    }

    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        SITHLOG_ERROR("Thing %s not physics type in FindFloor()", pThing->aName);
        return;
    }

    if ( !pThing->pInSector )
    {
        return;
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        if ( pThing->type == SITH_THING_PLAYER )
        {
            sithPhysics_FindWaterSurface(pThing);
        }

        return;
    }
    else if ( (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
    {
        if ( pThing->attach.flags )
        {
            sithThing_DetachThing(pThing);
        }

        return;
    }

    int searchFlags = 0x2800; // 0x800 - search for thing collision in all adjoined sectors (that were not searched yet) of already searched sectors

    const SithPhysicsFlags physflags = pThing->moveInfo.physics.flags;
    if ( pThing->type == SITH_THING_PLAYER && (physflags & (SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) == 0 )
    {
        searchFlags |= 0x200; // Check for adjoin SITH_ADJOIN_NOPLAYERMOVE flag is unset for collision ray to cross adjoin
    }

    rdVector3 moveNorm = RDVECTOR_NEG3(rdroid_g_zVector3);
    if ( (physflags & SITH_PF_WALLSTICK) != 0 )
    {
        searchFlags |= 0x02;// no adjoint touch?
        moveNorm = RDVECTOR_NEG3(pThing->orient.uvec);

    }
    else if ( (physflags & SITH_PF_RAFT) != 0 )
    {
        searchFlags |= 0x01; // Skip thing collisions in first pass
    }
    else
    {
        searchFlags |= 0x10; // Check for floor surface (i.e. surface with floor flag set or things with standon flag set)
    }

    float moveDist= sithPhysics_GetThingHeight(pThing);
    if ( bNoSurfaceImpactUpdate || pThing->attach.flags )
    {
        if ( (physflags & (SITH_PF_WALLSTICK | SITH_PF_FLOORSTICK)) != 0 )
        {
            moveDist *= 2.5f;
        }
        else
        {
            moveDist *= 1.1f;
        }
    }
    else
    {
        if ( moveDist == 0.0f )
        {
            moveDist = pThing->collide.movesize + 0.0049999999f;
        }
    }

    if ( moveDist > 0.0f )
    {
        float radius = 0.0f;
        while ( 1 )
        {
            sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, moveDist, radius, searchFlags);

            SithCollision* pCollision;
            while ( (pCollision = sithCollision_PopStack()) != NULL )
            {
                if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                {
                    if ( (physflags & SITH_PF_RAFT) == 0 )
                    {
                        sithCollision_DecreaseStackLevel();
                        sithThing_AttachThingToSurface(pThing, pCollision->pSurfaceCollided, bNoSurfaceImpactUpdate);
                        return;
                    }

                    if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATER) != 0
                        && (pCollision->pSurfaceCollided->pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
                    {
                        sithThing_AttachThingToSurface(pThing, pCollision->pSurfaceCollided, bNoSurfaceImpactUpdate);
                        sithCollision_DecreaseStackLevel();
                        return;
                    }
                }
                else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                {
                    SithThing* pHitThing = pCollision->pThingCollided;
                    SITH_ASSERTREL(pHitThing);

                    if ( pHitThing != pThing )
                    {
                        if ( !pCollision->pFaceCollided || !pCollision->pMeshCollided )
                        {
                            SITHLOG_ERROR("Hit standon object but not enough collide info to attach.\n");
                            sithCollision_DecreaseStackLevel();
                            return;
                        }

                        // Check if we should skip floor checks, or if surface is floor-like
                        bool bSkipFloorCheck = (searchFlags & 0x10) == 0;
                        bool bFlatSurface    = false;
                        if ( !bSkipFloorCheck )
                        {
                            rdVector3 faceNormal;
                            rdMatrix_TransformVector34(&faceNormal, &pCollision->pFaceCollided->normal, &pCollision->pThingCollided->orient);
                            bFlatSurface = rdVector_Dot3(&faceNormal, &rdroid_g_zVector3) >= 0.60000002f;
                        }

                        if ( bSkipFloorCheck || bFlatSurface )
                        {
                            sithThing_AttachThingToThingFace(
                                pThing,
                                pHitThing,
                                pCollision->pFaceCollided,
                                pCollision->pMeshCollided->apVertices,
                                bNoSurfaceImpactUpdate
                            );

                            sithCollision_DecreaseStackLevel();
                            return;
                        }
                    }
                }
                else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 && (physflags & SITH_PF_RAFT) != 0 )
                {
                    SITH_ASSERTREL(pCollision->pSurfaceCollided);
                    SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin);
                    if ( (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
                    {
                        sithThing_AttachThingToSurface(pThing, pCollision->pSurfaceCollided, bNoSurfaceImpactUpdate);
                        sithCollision_DecreaseStackLevel();
                        return;
                    }
                }
            }

            sithCollision_DecreaseStackLevel();

            if ( radius != 0.0f || pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
            {
                break;
            }

            if ( pThing->collide.movesize == 0.0f )
            {
                break;
            }

            if ( (physflags & SITH_PF_RAFT) != 0 )
            {
                radius   = 0.001f;
                moveDist *= 2.0f;
            }
            else if ( pThing->type == SITH_THING_PLAYER && pThing->moveStatus == SITHPLAYERMOVE_RUNNING && sithPuppet_g_bPlayerLeapForward )
            {
                radius = pThing->collide.movesize;
                moveDist -= radius;
                if ( moveDist <= 0.050000001f )
                {
                    moveDist = 0.050000001f;
                }
            }
            else
            {
                radius = 0.0049999999f;
                moveDist -= radius;
                if ( moveDist <= 0.050000001f )
                {
                    moveDist = 0.050000001f;
                }
            }
        }
    }

    //
    // No floor found, detach thing and make it fall
    //
    if ( pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);

        if ( (physflags & SITH_PF_JEEP) == 0
            && (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR)
            && !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            if ( pThing->type == SITH_THING_PLAYER )
            {
                if ( (physflags & SITH_PF_RAFT) != 0 || pThing->moveStatus != SITHPLAYERMOVE_CRAWLIDLE && pThing->moveInfo.physics.height >= 0.090000004f )
                {
                    if ( pThing->type == SITH_THING_PLAYER && pThing->moveStatus == SITHPLAYERMOVE_RUNNING && sithPuppet_g_bPlayerLeapForward == 1
                        || pThing->moveStatus == SITHPLAYERMOVE_LEAPFWD )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_LEAPFWD;
                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LEAPLEFT, /*pCallback=*/ NULL);
                        sithPuppet_g_bPlayerLeapForward = 0;
                        sithPlayerActions_LeapForward(pThing);
                        return;
                    }
                }
                else
                {
                    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
                    pThing->collide.movesize = 0.039999999f;

                    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
                    pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;

                    pThing->moveInfo.physics.height = 0.090000004f;
                    sithInventory_SetSwimmingInventory(pThing, 1);
                }
            }

            // Set falling move status
            pThing->moveStatus = SITHPLAYERMOVE_FALLING;
        }
    }
}

void J3DAPI sithPhysics_FindWaterSurface(SithThing* pThing)
{
    pThing->attach.distToWaterSurface = 0.1f;

    // Search 1m upwards for water surface
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &rdroid_g_zVector3, 0.1f, 0.0f, 0x01); // 0x01 = skip thing collisions

    for ( SithCollision* pCollision = sithCollision_PopStack(); pCollision; pCollision = sithCollision_PopStack() )
    {
        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
            SITH_ASSERTREL(pCollision->pSurfaceCollided);
            SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin);
            if ( (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
            {
                // Play exit water sound fx
                if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_EXITWATER);
                }

                pThing->moveInfo.physics.flags |= SITH_PF_ONWATERSURFACE;
                pThing->attach.distToWaterSurface = pCollision->distance;
                sithCollision_DecreaseStackLevel();
                return;
            }
        }
    }

    // No water surface found - thing is fully submerged or not in water
    sithCollision_DecreaseStackLevel();
    pThing->moveInfo.physics.flags &= ~SITH_PF_ONWATERSURFACE;
}

void J3DAPI sithPhysics_UpdateThing(SithThing* pThing, float secDeltaTime)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);

    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    if ( !pThing->pInSector
        || (pThing->type == SITH_THING_ACTOR && pThing->thingInfo.actorInfo.bForceMovePlay == 1)
        || (pPhysics->flags & SITH_PF_NOUPDATE) != 0 )
    {
        return; // Skip physics update
    }

    rdVector_Zero3(&pPhysics->deltaVelocity);
    rdVector_Zero3(&pPhysics->gravityForce);

    if ( (pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER) && (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        rdVector_Zero3(&pPhysics->thrust);
    }

    //
    // Update physics based on thing type and attachment state
    //

    if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 && pThing->moveStatus == SITHPLAYERMOVE_HANGING )
    {
        sithPhysics_UpdateClimbingThingPhysics(pThing, secDeltaTime);
    }
    // Update MineCar physics
    else if ( (pPhysics->flags & SITH_PF_JEEP) != 0 )
    {
        sithPhysics_UpdateJeepPhysics(pThing, secDeltaTime);
        if ( SITH_ISFRAMECYCLE(pThing->idx, 4) ) // every 4th frames
        {
            sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, 2, 4.0f, pThing);
        }
    }
    // Update mine car physics
    else if ( (pPhysics->flags & SITH_PF_MINECAR) != 0 && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
    {
        sithPhysics_UpdateMineCarPhysics(pThing, secDeltaTime);
        if ( SITH_ISFRAMECYCLE(pThing->idx, 8) ) // every 8th frames
        {
            sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, 2, 2.0f, pThing);
        }
    }
    // Update raft physics
    else if ( (pPhysics->flags & SITH_PF_RAFT) != 0 && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
    {
        sithPhysics_UpdateRaftPhysics(pThing, secDeltaTime);
    }
    // Update surface / thing attached physics
    else if ( (pThing->attach.flags & (SITH_ATTACH_THINGFACE | SITH_ATTACH_SURFACE)) != 0 )
    {
        sithPhysics_UpdateAttachedThingPhysics(pThing, secDeltaTime);
    }
    // Update climbing physics
    else if ( (pThing->attach.flags & SITH_ATTACH_CLIMBSURFACE) != 0 )
    {
        sithPhysics_UpdateClimbingThingPhysics(pThing, secDeltaTime);
    }
    // Update underwater physics
    else if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
    {
        if ( pThing->type == SITH_THING_PLAYER || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) == 0 )
        {
            sithPhysics_UpdateUnderwaterThingPhysics(pThing, secDeltaTime);

        }
        else
        {
            sithPhysics_UpdateDetachedThingPhysics(pThing, secDeltaTime);
        }
    }
    // Update detached physics
    else if ( pThing->type == SITH_THING_PLAYER )
    {
        sithPhysics_UpdateDetachedPlayerPhysics(pThing, secDeltaTime);
    }
    else
    {
        sithPhysics_UpdateDetachedThingPhysics(pThing, secDeltaTime);
    }
}

void J3DAPI sithPhysics_ApplyForce(SithThing* pThing, const rdVector3* force)
{
    SITH_ASSERTREL(pThing != NULL);
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);

    if ( pThing->moveType != SITH_MT_PHYSICS
        || pThing->moveInfo.physics.mass <= 0.0f
        || rdVector_IsZero3(force) )
    {
        return;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS && pThing->moveInfo.physics.mass > 0.0f && !rdVector_IsZero3(force) )
    {
        float invMass = 1.0f / pThing->moveInfo.physics.mass;
        if ( (force->z * invMass) > 0.5f )
        {
            sithThing_DetachThing(pThing);
        }

        rdVector_MultAcc3(&pThing->moveInfo.physics.velocity, force, invMass);
        pThing->moveInfo.physics.flags |= SITH_PF_FORCEAPPLIED;
    }
}

void J3DAPI sithPhysics_SetThingLook(SithThing* pThing, const rdVector3* look, float secDeltaTime)
{
    float upDiviation = 1.0f - (rdVector_Dot3(look, &pThing->orient.uvec));
    if ( stdMath_ClipNearZero(upDiviation) == 0.0f )
    {
        pThing->moveInfo.physics.flags |= SITH_PF_ALIGNED;
    }
    else if ( secDeltaTime == 0.0f )
    {
        pThing->orient.uvec = *look;

        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &pThing->orient.uvec); // right = left x up
        rdVector_Normalize3Acc(&pThing->orient.rvec);

        rdVector_Cross3(&pThing->orient.lvec, &pThing->orient.uvec, &pThing->orient.rvec); // left = up x right
        // TODO: normalize?

        pThing->moveInfo.physics.flags |= SITH_PF_ALIGNED;
    }
    else
    {
        rdVector_MultAcc3(&pThing->orient.uvec, look, secDeltaTime * 10.0f); // TODO: Calc smooth damping interp
        rdVector_Normalize3Acc(&pThing->orient.uvec);

        rdVector_Cross3(&pThing->orient.lvec, &pThing->orient.uvec, &pThing->orient.rvec); // left = up x right
        rdVector_Normalize3Acc(&pThing->orient.lvec);

        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &pThing->orient.uvec); // right = left x up
        // TODO: normalize?

        // Note: Not flagged as aligned cause it's transitioning to look vector
    }
}

void J3DAPI sithPhysics_ApplyDrag(rdVector3* pVelocity, float drag, float staticDrag, float secDeltaTime)
{
    // If staticDrag, check if current speed is below threshold
    if ( staticDrag != 0.0f )
    {
        float currentSpeed = rdVector_Len3(pVelocity);
        if ( currentSpeed < staticDrag )
        {
            rdVector_Zero3(pVelocity);
            return;
        }
    }

    if ( drag == 0.0f )
    {
        return;
    }

    float dragFactor = J3DMIN(drag * secDeltaTime, 1.0f);
    rdVector_MultAcc3(pVelocity, pVelocity, -dragFactor);
    rdMath_ClipVector3Acc(pVelocity, STDMATH_ZERO_EPSILON);
}

int J3DAPI sithPhysics_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int argNum)
{
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);
    switch ( argNum )
    {
        case SITHTHING_ARG_SURFDRAG:
        {
            float value = (float)atof(pArg->argValue);
            if ( value < 0.0f ) goto error;
            pThing->moveInfo.physics.surfDrag = value;
            return 1;
        }
        case SITHTHING_ARG_AIRDRAG:
        {
            float value = (float)atof(pArg->argValue);
            if ( value < 0.0f ) goto error;
            pThing->moveInfo.physics.airDrag = value;
            return 1;
        }
        case SITHTHING_ARG_STATICDRAG:
        {
            float value = (float)atof(pArg->argValue);
            if ( value < 0.0f ) goto error;
            pThing->moveInfo.physics.staticDrag = value;
            return 1;
        }
        case SITHTHING_ARG_MASS:
        {
            float value = (float)atof(pArg->argValue);
            if ( value < 0.0f ) goto error;
            pThing->moveInfo.physics.mass = value;
            return 1;
        }
        case SITHTHING_ARG_HEIGHT:
        {
            float value = (float)atof(pArg->argValue);
            if ( value < 0.0f ) goto error;
            pThing->moveInfo.physics.height = value;
            return 1;
        }
        case SITHTHING_ARG_PHYSFLAGS:
        {
            SithPhysicsFlags flags;
            if ( sscanf_s(pArg->argValue, "%x", &flags) != 1 )
            {
                goto error;
            }
            pThing->moveInfo.physics.flags = flags;
            return 1;
        }
        case SITHTHING_ARG_MAXROTVEL:
        {
            float value = (float)atof(pArg->argValue);
            if ( value < 0.0f ) goto error;
            pThing->moveInfo.physics.maxRotationVelocity = value;
            return 1;
        }
        case SITHTHING_ARG_MAXVEL:
        {
            float value = (float)atof(pArg->argValue);
            if ( value < 0.0f ) goto error;
            pThing->moveInfo.physics.maxVelocity = value;
            return 1;
        }
        case SITHTHING_ARG_VEL:
        {
            if ( sscanf_s(pArg->argValue, "(%f/%f/%f)",
                &pThing->moveInfo.physics.velocity.x,
                &pThing->moveInfo.physics.velocity.y,
                &pThing->moveInfo.physics.velocity.z) != 3 )
            {
                goto error;
            }
            return 1;
        }
        case SITHTHING_ARG_ANGVEL:
        {
            if ( sscanf_s(pArg->argValue, "(%f/%f/%f)",
                &pThing->moveInfo.physics.angularVelocity.x,
                &pThing->moveInfo.physics.angularVelocity.y,
                &pThing->moveInfo.physics.angularVelocity.z) != 3 )
            {
                goto error;
            }
            return 1;
        }
        case SITHTHING_ARG_ORIENTSPEED:
        {
            float value = (float)atof(pArg->argValue);
            if ( value < 0.0f ) goto error;
            pThing->moveInfo.physics.orientSpeed = value;
            return 1;
        }
        case SITHTHING_ARG_BUOYANCY:
        {
            float value = (float)atof(pArg->argValue);
            pThing->moveInfo.physics.buoyancy = value;
            return 1;
        }

        default:
            return 0;
    }

error:
    SITHLOG_ERROR("Bad argument %s=%s line %d.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber());
    return 0;
}

void J3DAPI sithPhysics_ResetThingMovement(SithThing* pThing)
{
    SITH_ASSERTREL((pThing != NULL) && (pThing->moveType == SITH_MT_PHYSICS));
    rdVector_Zero3(&pThing->moveInfo.physics.velocity);
    rdVector_Zero3(&pThing->moveInfo.physics.angularVelocity);
    rdVector_Zero3(&pThing->moveInfo.physics.rotThrust);
    rdVector_Zero3(&pThing->moveInfo.physics.thrust);
    rdVector_Zero3(&pThing->moveInfo.physics.deltaVelocity);
    rdVector_Zero3(&pThing->moveDir);
}

float J3DAPI sithPhysics_GetThingHeight(const SithThing* pThing)
{
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);
    float height = pThing->moveInfo.physics.height;
    if ( height != 0.0f )
    {
        return height;
    }

    if ( pThing->renderData.type == RD_THING_MODEL3 )
    {
        height = pThing->renderData.data.pModel3->insertOffset.z;
    }

    if ( pThing->collide.movesize + 0.0049999999f >= height )
    {
        return pThing->collide.movesize + 0.0049999999f;
    }

    return height;
}

void J3DAPI sithPhysics_UpdateDetachedThingPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics  = &pThing->moveInfo.physics;
    SithPhysicsFlags physFlags = pPhysics->flags;

    rdVector_Zero3(&pPhysics->gravityForce);

    // Special handling for raft in air
    if ( (physFlags & SITH_PF_RAFT) != 0 )
    {
        // Note, raft node names are wrong and inraftfr is back raft side node
        // and inraftbk is fornt side node
        int raftBackIdx  = sithThing_GetThingJointIndex(pThing, "inraftfr");
        int raftFrontIdx = sithThing_GetThingJointIndex(pThing, "inraftbk");
        // TODO: Add check for invalid indices

        rdVector3* aAngles = pThing->renderData.apTweakedAngles;
        aAngles[raftBackIdx].pitch  -= stdMath_NormalizeAngleAcute(aAngles[raftBackIdx].pitch) * 0.5f;
        aAngles[raftFrontIdx].pitch -= stdMath_NormalizeAngleAcute(aAngles[raftFrontIdx].pitch) * 0.5f;

        float lookSpeed = 0.2f * secDeltaTime; // 0.2f = 1/50fps?
        sithPhysics_SetThingLook(pThing, &rdroid_g_zVector3, lookSpeed);
    }

    // Update angular velocity with thrust and drag
    if ( (physFlags & SITH_PF_USEANGULARTHRUST) != 0 )
    {
        // Apply air drag to angular velocity
        if ( !rdVector_IsZero3(&pPhysics->angularVelocity) )
        {
            float angularDrag = pPhysics->airDrag + 0.2f;
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, angularDrag, 0.0f, secDeltaTime);
        }

        // Apply angular thrust: angVel += rotThrust * dt
        rdVector_MultAcc3(&pPhysics->angularVelocity, &pPhysics->rotThrust, secDeltaTime);
        rdMath_ClampVector3Acc(&pPhysics->angularVelocity, -pPhysics->maxRotationVelocity, pPhysics->maxRotationVelocity); // Clamp angular velocity to max rotation velocity

        // Clip near-zero angular velocities
        rdMath_ClipVector3Acc(&pPhysics->angularVelocity, STDMATH_ZERO_EPSILON);
    }

    // Apply angular velocity to thing rotation
    rdVector3 angularDelta = { 0 };
    if ( !rdVector_IsZero3(&pPhysics->angularVelocity) )
    {
        rdVector_Scale3(&angularDelta, &pPhysics->angularVelocity, secDeltaTime);
    }

    if ( !rdVector_IsZero3(&angularDelta) )
    {
        rdMatrix34 rotMat;
        rdMatrix_BuildRotate34(&rotMat, &angularDelta);
        sithCollision_RotateThing(pThing, &rotMat);

        // For flying things, rotate velocity vector with the thing
        if ( (physFlags & SITH_PF_FLY) != 0 )
        {
            rdMatrix_TransformVector34Acc(&pPhysics->velocity, &rotMat);
        }

        // Periodically normalize orientation matrix to prevent drift
        if ( SITH_ISFRAMECYCLE(pThing->idx, 8) ) // every 8th frame
        {
            rdMatrix_Normalize34(&pThing->orient);
        }
    }

    // Apply air drag to velocity
    if ( pPhysics->airDrag != 0.0f )
    {
        sithPhysics_ApplyDrag(&pPhysics->velocity, pPhysics->airDrag, 0.0f, secDeltaTime);
    }

    // Apply thrust forces
    rdVector3 thrustDelta = { 0 };
    if ( (physFlags & SITH_PF_USETHRUST) != 0 )
    {
        // Water? things get reduced lateral thrust
        if ( (physFlags & SITH_PF_FLY) == 0 && pThing->moveStatus != SITHPLAYERMOVE_FALLING )
        {
            rdVector_Scale3Acc(&pPhysics->thrust, 0.30000001f);
        }

        // Transform thrust from local to world space
        rdVector_Scale3(&thrustDelta, &pPhysics->thrust, secDeltaTime);
        rdMatrix_TransformVector34Acc(&thrustDelta, &pThing->orient); // Transform to world space
    }

    // Apply sector thrust (e.g., wind, water current)
    if ( pPhysics->mass != 0.0f
        && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0
        && (physFlags & SITH_PF_NOTHRUST) == 0 )
    {
        rdVector_MultAcc3(&thrustDelta, &pThing->pInSector->thrust, secDeltaTime);
    }

    // Apply gravity
    if ( pPhysics->mass != 0.0f
        && (physFlags & SITH_PF_USEGRAVITY) != 0
        && (pThing->pInSector->flags & SITH_SECTOR_NOGRAVITY) == 0 )
    {
        float gravityDelta = sithWorld_g_pCurrentWorld->gravity * secDeltaTime;

        // Partial gravity for things in special states
        if ( (pPhysics->flags & SITH_PF_PARTIALGRAVITY) != 0 )
        {
            gravityDelta *= 0.5f;
        }

        thrustDelta.z -= gravityDelta;
        pPhysics->gravityForce.z = -gravityDelta;
    }

    // Update velocity with accumulated thrust/forces
    rdVector_Add3Acc(&pPhysics->velocity, &thrustDelta);

    // Clip near-zero velocity components
    rdMath_ClipVector3Acc(&pPhysics->velocity, STDMATH_ZERO_EPSILON);

    // Calculate position delta for this frame
    if ( !rdVector_IsZero3(&pPhysics->velocity) )
    {
        rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
    }
}

void J3DAPI sithPhysics_UpdateDetachedPlayerPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics  = &pThing->moveInfo.physics;
    SithPhysicsFlags physFlags = pPhysics->flags;

    rdVector_Zero3(&pPhysics->gravityForce);

    // Special handling for raft in air
    if ( (physFlags & SITH_PF_RAFT) != 0 )
    {
        int raftFrontIdx = sithThing_GetThingJointIndex(pThing, "inraftfr");
        int raftBackIdx  = sithThing_GetThingJointIndex(pThing, "inraftbk");

        rdVector3* pAngles = pThing->renderData.apTweakedAngles;
        pAngles[raftFrontIdx].pitch -= stdMath_NormalizeAngleAcute(pAngles[raftFrontIdx].pitch) * 0.5f;
        pAngles[raftBackIdx].pitch  -= stdMath_NormalizeAngleAcute(pAngles[raftBackIdx].pitch) * 0.5f;

        float lookSpeed = 0.2f * secDeltaTime;
        sithPhysics_SetThingLook(pThing, &rdroid_g_zVector3, lookSpeed);
    }

    // Handle player falling from slide
    if ( pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNFORWARD
        || pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNBACK )
    {
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        pThing->moveStatus = SITHPLAYERMOVE_FALLING;

        if ( pThing->pInSector
            && ((pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0
                || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0) )
        {
            pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
            if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_SWIM )
            {
                sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
            }
        }
    }

    // Update angular velocity with thrust and drag
    if ( (physFlags & SITH_PF_USEANGULARTHRUST) != 0 )
    {
        // Apply air drag to angular velocity
        if ( !rdVector_IsZero3(&pPhysics->angularVelocity) )
        {
            float angularDrag = pPhysics->airDrag + 0.2f;
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, angularDrag, 0.0f, secDeltaTime);
        }

        // Apply angular thrust: angVel += rotThrust * dt
        rdVector_MultAcc3(&pPhysics->angularVelocity, &pPhysics->rotThrust, secDeltaTime);

        // Clamp angular velocity to max rotation velocity
        rdMath_ClampVector3Acc(&pPhysics->angularVelocity, -pPhysics->maxRotationVelocity, pPhysics->maxRotationVelocity); // Clamp angular velocity to max rotation velocity

        // Clip near-zero angular velocities
        rdMath_ClipVector3Acc(&pPhysics->angularVelocity, STDMATH_ZERO_EPSILON);
    }

    // Apply angular velocity to thing rotation
    rdVector3 angularDelta = { 0 };
    if ( !rdVector_IsZero3(&pPhysics->angularVelocity) )
    {
        rdVector_Scale3(&angularDelta, &pPhysics->angularVelocity, secDeltaTime);
    }

    if ( !rdVector_IsZero3(&angularDelta) )
    {
        rdMatrix34 rotMat;
        rdMatrix_BuildRotate34(&rotMat, &angularDelta);
        sithCollision_RotateThing(pThing, &rotMat);

        // For flying things, rotate velocity vector with the thing
        if ( (physFlags & SITH_PF_FLY) != 0 )
        {
            rdMatrix_TransformVector34Acc(&pPhysics->velocity, &rotMat);
        }

        // Periodically normalize orientation matrix to prevent drift
        if ( SITH_ISFRAMECYCLE(pThing->idx, 8) ) // every 8th frame
        {
            rdMatrix_Normalize34(&pThing->orient);
        }
    }

    // Water? things get reduced lateral thrust
    if ( (physFlags & SITH_PF_FLY) == 0 && pThing->moveStatus != SITHPLAYERMOVE_FALLING )
    {
        rdVector_Scale3Acc(&pPhysics->thrust, 0.30000001f);
    }

    // Player physics runs at 50 FPS - accumulate time and process in fixed timesteps
    float totalTime = secDeltaTime + pPhysics->physicsRolloverFrames;
    size_t nFrames  = (size_t)truncf(totalTime * sithPhysics_fixedFramerate);
    pPhysics->physicsRolloverFrames = totalTime - (float)nFrames * sithPhysics_fixedTimestep;

    for ( size_t i = 0; i < nFrames; ++i )
    {
        rdVector3 thrustDelta = { 0 };

        // Apply air drag to velocity
        if ( pPhysics->airDrag != 0.0f )
        {
            sithPhysics_ApplyDrag(&pPhysics->velocity, pPhysics->airDrag, 0.0f, sithPhysics_fixedTimestep);
        }

        // Apply thrust forces in local space, then transform to world space
        if ( (physFlags & SITH_PF_USETHRUST) != 0 )
        {
            rdVector_Scale3(&thrustDelta, &pPhysics->thrust, sithPhysics_fixedTimestep);
            rdMatrix_TransformVector34Acc(&thrustDelta, &pThing->orient); // Transform thrust from local to world space
        }

        // Apply sector thrust
        if ( pPhysics->mass != 0.0f
            && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0
            && (physFlags & SITH_PF_NOTHRUST) == 0 )
        {
            rdVector_MultAcc3(&thrustDelta, &pThing->pInSector->thrust, sithPhysics_fixedTimestep);
        }

        // Apply gravity
        if ( pPhysics->mass != 0.0f
            && (physFlags & SITH_PF_USEGRAVITY) != 0
            && (pThing->pInSector->flags & SITH_SECTOR_NOGRAVITY) == 0 )
        {
            float gravityDelta = sithWorld_g_pCurrentWorld->gravity * sithPhysics_fixedTimestep;

            // Partial gravity for things in special states
            if ( (pPhysics->flags & SITH_PF_PARTIALGRAVITY) != 0 )
            {
                gravityDelta *= 0.5f;
            }

            thrustDelta.z -= gravityDelta;
            pPhysics->gravityForce.z = -gravityDelta;
        }

        // Update velocity with accumulated thrust/forces
        rdVector_Add3Acc(&pPhysics->velocity, &thrustDelta);

        // TODO: Clip near-zero velocity components like in sithPhysics_UpdateDetachedThingPhysics?

        // Accumulate position delta for this frame
        rdVector_MultAcc3(&pPhysics->deltaVelocity, &pPhysics->velocity, sithPhysics_fixedTimestep);
    }
}

void J3DAPI sithPhysics_UpdateUnderwaterThingPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics  = &pThing->moveInfo.physics;
    SithPhysicsFlags physFlags = pPhysics->flags;

    rdVector_Zero3(&pPhysics->gravityForce);

    // Update angular velocity with thrust and drag
    if ( (physFlags & SITH_PF_USEANGULARTHRUST) != 0 )
    {
        // Apply water drag to angular velocity
        if ( !rdVector_IsZero3(&pPhysics->angularVelocity) )
        {
            float angularDrag = pPhysics->airDrag + 0.2f;
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, angularDrag, 0.0f, secDeltaTime);
        }

        // Apply angular thrust: angVel += rotThrust * dt
        rdVector_MultAcc3(&pPhysics->angularVelocity, &pPhysics->rotThrust, secDeltaTime);
        rdMath_ClampVector3Acc(&pPhysics->angularVelocity, -pPhysics->maxRotationVelocity, pPhysics->maxRotationVelocity); // Clamp angular velocity to max rotation velocity
        rdMath_ClipVector3Acc(&pPhysics->angularVelocity, STDMATH_ZERO_EPSILON); // Clip near-zero angular velocities
    }

    // Apply angular velocity to thing rotation
    rdVector3 angularDelta = { 0 };
    if ( !rdVector_IsZero3(&pPhysics->angularVelocity) )
    {
        rdVector_Scale3(&angularDelta, &pPhysics->angularVelocity, secDeltaTime);
    }

    if ( !rdVector_IsZero3(&angularDelta) )
    {
        rdMatrix34 rotMat;
        rdMatrix_BuildRotate34(&rotMat, &angularDelta);
        sithCollision_RotateThing(pThing, &rotMat);

        // Every 8th frame normalize orientation matrix to prevent drift
        if ( SITH_ISFRAMECYCLE(pThing->idx, 8) )
        {
            rdMatrix_Normalize34(&pThing->orient);
        }
    }

    // Underwater has 4x water drag
    if ( pPhysics->airDrag != 0.0f )
    {
        float underwaterDrag = pPhysics->airDrag * 4.0f;
        sithPhysics_ApplyDrag(&pPhysics->velocity, underwaterDrag, 0.0f, secDeltaTime);
    }

    // Apply thrust forces - underwater thrust is damped to 60%
    rdVector3 thrustDelta = { 0 };
    if ( (physFlags & SITH_PF_USETHRUST) != 0 )
    {
        rdVector_Scale3Acc(&pPhysics->thrust, 0.60000002f);
        rdVector_Scale3(&thrustDelta, &pPhysics->thrust, secDeltaTime);
        rdMatrix_TransformVector34Acc(&thrustDelta, &pThing->orient); // Transform thrust from local to world space
    }

    // Apply sector thrust (e.g., water current)
    if ( pPhysics->mass != 0.0f
        && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0
        && (physFlags & SITH_PF_NOTHRUST) == 0 )
    {
        rdVector_MultAcc3(&thrustDelta, &pThing->pInSector->thrust, secDeltaTime);
    }

    // Apply buoyancy (gravity modified by buoyancy factor)
    if ( ((physFlags & SITH_PF_ONWATERSURFACE) == 0
        || (pThing->flags & SITH_TF_DYING) != 0) && (physFlags & SITH_PF_USEGRAVITY) != 0 ) // if dead sink it
    {
        // TODO: Add buoyancy to dead thing with no buoyancy
        /*if ( (pThing->flags & SITH_TF_DYING) != 0 && pPhysics->buoyancy == 0.0f )
        {
            pPhysics->buoyancy = 0.025f;
        }*/

        float gravityDelta  = sithWorld_g_pCurrentWorld->gravity * secDeltaTime;
        float buoyancyForce = pPhysics->buoyancy * gravityDelta;
        thrustDelta.z -= buoyancyForce;
        pPhysics->gravityForce.z -= buoyancyForce;
    }

    // Update velocity with accumulated thrust/forces
    rdVector_Add3Acc(&pPhysics->velocity, &thrustDelta);
    rdMath_ClipVector3Acc(&pPhysics->velocity, STDMATH_ZERO_EPSILON); // Clip near-zero velocity components

    // Calculate position delta for this frame
    if ( !rdVector_IsZero3(&pPhysics->velocity) )
    {
        rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
    }

    // Push thing to water surface if near it
    if ( (pPhysics->flags & SITH_PF_ONWATERSURFACE) != 0 && pPhysics->thrust.z >= 0.0f )
    {
        float distBelowSurface = pThing->attach.distToWaterSurface - 0.050999999f;
        float maxSurfacePush   = 0.0099999998f * secDeltaTime;

        // Cancel small upward velocities near surface
        if ( pPhysics->deltaVelocity.z > 0.0f && pPhysics->deltaVelocity.z < maxSurfacePush )
        {
            pPhysics->deltaVelocity.z = 0.0f;
        }

        // Push thing up to water surface
        if ( distBelowSurface != 0.0f )
        {
            float surfacePush = J3DMIN(distBelowSurface, maxSurfacePush);
            rdVector_MultAcc3(&pPhysics->deltaVelocity, &rdroid_g_zVector3, surfacePush);
            //TODO: Could just do pPhysics->deltaVelocity.z += surfacePush;
            sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_FIRE4, 0.0f);
        }
    }
}

void J3DAPI sithPhysics_UpdateClimbingThingPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;

    if ( rdVector_IsZero3(&pPhysics->thrust) )
    {
        return;
    }

    // Climbing has reduced thrust (10%)
    float climbingThrustScale = 0.1f;
    rdVector3 thrust;
    rdVector_Scale3(&thrust, &pPhysics->thrust, secDeltaTime * climbingThrustScale);

    // Clip near-zero thrust components
    rdMath_ClipVector3Acc(&thrust, STDMATH_ZERO_EPSILON);

    if ( !rdVector_IsZero3(&thrust) )
    {
        // Transform thrust from local to world space
        rdMatrix_TransformVector34Acc(&thrust, &pThing->orient);
    }

    // Apply sector thrust
    if ( pPhysics->mass != 0.0f
        && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0
        && (pPhysics->flags & SITH_PF_NOTHRUST) == 0 )
    {
        rdVector_MultAcc3(&thrust, &pThing->pInSector->thrust, secDeltaTime);
    }

    // For climbing, velocity equals thrust (no acceleration/momentum)
    rdVector_Copy3(&pPhysics->velocity, &thrust);
    rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
}

void J3DAPI sithPhysics_UpdateAttachedThingPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    rdFace* pAttachedFace = pThing->attach.pFace; // Leftover from sithPhysics_GetThingAttachNormalAndDistance?

    pPhysics->flags &= ~SITH_PF_UNKNOWN_200000;

    // Get attachment surface normal and distance
    rdVector3 floorNormal;
    float distToFloor, floorAttachScale;
    sithPhysics_GetThingAttachNormalAndDistance(pThing, &floorNormal, &distToFloor, &floorAttachScale);

    //
    // Calculate how aligned surface is with up vector (1.0 = flat, 0.0 = vertical)
    //
    float floorSlopeDot = rdVector_Dot3(&floorNormal, &rdroid_g_zVector3);

    // Adjust distance if aligning up vector
    if ( (pPhysics->flags & SITH_PF_ALIGNUP) != 0 && floorSlopeDot < 1.0f )
    {
        SITH_ASSERTREL(floorSlopeDot > 0.0f);
        distToFloor = distToFloor / floorSlopeDot; // Projected distance along up vector
    }

    //
    // Handle orientation alignment with surface or up vector
    //
    if ( (pPhysics->flags & SITH_PF_ALIGNED) == 0 )
    {
        if ( (pPhysics->flags & SITH_PF_ALIGNSURFACE) != 0 )
        {
            float orientSpeed = pPhysics->orientSpeed * secDeltaTime;
            sithPhysics_SetThingLook(pThing, &floorNormal, orientSpeed);
        }
        else if ( (pPhysics->flags & SITH_PF_ALIGNUP) != 0 )
        {
            float orientSpeed = pPhysics->orientSpeed * secDeltaTime;
            sithPhysics_SetThingLook(pThing, &rdroid_g_zVector3, orientSpeed);
        }
        else
        {
            // Nothing to be done, mark as aligned
            pPhysics->flags |= SITH_PF_ALIGNED;
        }
    }

    //
    // Handle steep slope sliding (37-43 degree range)
    //
    if ( pThing->moveStatus != SITHPLAYERMOVE_LEAPFWD
        && sithPhysics_CheckSlopeAngle(floorSlopeDot)
        && (pThing->type == SITH_THING_PLAYER || (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0) )
    {
        // Project look direction onto horizontal 2D plane
        rdVector3 horizontalLook= pThing->orient.lvec;
        horizontalLook.z = 0.0f;
        rdVector_Normalize3Acc(&horizontalLook);

        // Project surface normal onto horizontal 2D plane
        rdVector3 horizontalNormal = floorNormal;
        horizontalNormal.z = 0.0f;
        rdVector_Normalize3Acc(&horizontalNormal);

        // Determine slide direction based on facing
        pThing->moveStatus = SITHPLAYERMOVE_SLIDEDOWNFORWARD;
        float facingDot = rdVector_Dot3(&horizontalNormal, &horizontalLook);
        if ( facingDot < 0.0f )
        {
            rdVector_Neg3Acc(&horizontalNormal);
            pThing->moveStatus = SITHPLAYERMOVE_SLIDEDOWNBACK;
        }

        // Reorient thing to slide direction
        pThing->orient.lvec = horizontalNormal;

        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_g_zVector3);
        rdVector_Normalize3Acc(&pThing->orient.rvec);

        pThing->orient.uvec = rdroid_g_zVector3;

        // Calculate slide velocity
        rdVector3 thrustDelta  = floorNormal;
        thrustDelta.z *= -1.0f; // Invert normal to point downhill, i.e.: downward thrust
        rdVector_Normalize3Acc(&thrustDelta);
        pPhysics->velocity = thrustDelta;

        // Clamp and apply scale
        floorAttachScale = STDMATH_CLAMP(floorAttachScale, 0.25f, 1.0f);

        float slideDrag = pPhysics->surfDrag * floorAttachScale;
        sithPhysics_ApplyDrag(&pPhysics->velocity, slideDrag, 0.0f, secDeltaTime);

        rdVector_Scale3Acc(&pPhysics->velocity, 0.5f);
        rdMath_ClipVector3Acc(&pPhysics->velocity, STDMATH_ZERO_EPSILON);

        if ( !rdVector_IsZero3(&pPhysics->velocity) )
        {
            rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
        }

        // Early out since sliding overrides normal attached physics
        return;
    }

    //
    // Not sliding - handle normal attached physics
    //

    // Handle transition from sliding
    if ( pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNFORWARD
        || pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNBACK )
    {
        sithPhysics_ResetThingMovement(pThing);

        if ( (pThing == sithPlayer_g_pLocalPlayerThing
            || (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0)
            && !sithPhysics_IsVehiclePhysics(pPhysics) )
        {
            if ( pThing->pInSector
                && ((pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0
                    || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
                if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_SWIM )
                {
                    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
                }
            }
            else
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
            }
        }
    }

    //
    // Update angular velocity with thrust and drag
    //
    if ( (pPhysics->flags & SITH_PF_USEANGULARTHRUST) != 0 )
    {
        if ( !rdVector_IsZero3(&pPhysics->angularVelocity) )
        {
            float angularDrag = pPhysics->surfDrag + 0.2f;
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, angularDrag, 0.0f, secDeltaTime);
        }

        // Apply yaw thrust only
        // Note, in other cases e.g., detached physics, pitch and roll are also applied
        pPhysics->angularVelocity.yaw += pPhysics->rotThrust.yaw * secDeltaTime;
        rdMath_ClampVector3Acc(&pPhysics->angularVelocity, -pPhysics->maxRotationVelocity, pPhysics->maxRotationVelocity); // Clamp angular velocity to max rotation velocity
        rdMath_ClipVector3Acc(&pPhysics->angularVelocity, STDMATH_ZERO_EPSILON);
    }

    //
    // Apply angular velocity rotation to thing and things velocity
    //
    if ( pPhysics->angularVelocity.yaw != 0.0f )
    {
        rdVector3 angularDelta;
        rdVector_Scale3(&angularDelta, &pPhysics->angularVelocity, secDeltaTime);

        rdMatrix34 rotMat;
        rdMatrix_BuildRotate34(&rotMat, &angularDelta);
        sithCollision_RotateThing(pThing, &rotMat);

        // Rotate velocity with thing, but blend based on attachment scale
        if ( floorAttachScale >= 1.0f )
        {
            rdMatrix_TransformVector34Acc(&pPhysics->velocity, &rotMat);
        }
        else
        {
            rdVector3 rotatedVel;
            rdMatrix_TransformVector34(&rotatedVel, &pPhysics->velocity, &rotMat);

            float unrotatedScale = 1.0f - floorAttachScale;
            rdVector_Scale3Acc(&pPhysics->velocity, unrotatedScale);
            rdVector_MultAcc3(&pPhysics->velocity, &rotatedVel, floorAttachScale);
        }

        rdMatrix_Normalize34(&pThing->orient);
    }

    // Clamp attachment scale
    floorAttachScale = STDMATH_CLAMP(floorAttachScale, 0.25f, 1.0f);

    //
    // Apply surface drag
    //
    if ( !rdVector_IsZero3(&pPhysics->velocity) && pPhysics->surfDrag != 0.0f )
    {
        if ( (pPhysics->flags & SITH_PF_FORCEAPPLIED) != 0 )
        {
            pPhysics->flags &= ~SITH_PF_FORCEAPPLIED;
        }
        else if ( rdVector_IsZero3(&pPhysics->thrust)
            && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) == 0
            && floorAttachScale > 0.80000001f )
        {
            // Apply static drag when stationary on good attachment
            float staticDrag = pPhysics->staticDrag * floorAttachScale;
            float surfDrag   = pPhysics->surfDrag * floorAttachScale;
            sithPhysics_ApplyDrag(&pPhysics->velocity, surfDrag, staticDrag, secDeltaTime);
        }
        else
        {
            float surfDrag = pPhysics->surfDrag * floorAttachScale;
            sithPhysics_ApplyDrag(&pPhysics->velocity, surfDrag, 0.0f, secDeltaTime);
        }
    }

    //
    // Apply thrust forces
    //
    rdVector3 thrustDelta = { 0 };
    if ( (pPhysics->flags & SITH_PF_USETHRUST) != 0 && !rdVector_IsZero3(&pPhysics->thrust) )
    {
        float thrustScale = secDeltaTime * floorAttachScale;
        if ( (pPhysics->flags & SITH_PF_CROUCHING) != 0 )
        {
            thrustScale = secDeltaTime * SITHPHYSICS_CROUCH_THRUST_SCALE;
        }

        rdVector_Scale3(&thrustDelta, &pPhysics->thrust, thrustScale);
        rdMath_ClipVector3Acc(&thrustDelta, STDMATH_ZERO_EPSILON);

        if ( !rdVector_IsZero3(&thrustDelta) )
        {
            rdMatrix_TransformVector34Acc(&thrustDelta, &pThing->orient); // Transform thrust from local to world space
        }
    }

    //
    // Apply sector thrust
    //
    if ( pPhysics->mass != 0.0f
        && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0
        && (pPhysics->flags & SITH_PF_NOTHRUST) == 0 )
    {
        // Check if thrust uplifts i.e. detach thing
        // and in this case detach it and update as detached thing
        if ( (sithWorld_g_pCurrentWorld->gravity * pPhysics->mass) < pThing->pInSector->thrust.z )
        {
            sithThing_DetachThing(pThing);
            sithPhysics_UpdateDetachedThingPhysics(pThing, secDeltaTime);
            return;
        }

        rdVector_MultAcc3(&thrustDelta, &pThing->pInSector->thrust, secDeltaTime);
    }

    //
    // Update velocity
    // Updated velocity is then used by sithThing_UpdateMove to actually move the thing 
    //
    rdVector_Add3Acc(&pPhysics->velocity, &thrustDelta);

    //
    // Apply gravity on slopes for players
    //
    if ( pThing->type == SITH_THING_PLAYER
        && (pPhysics->flags & SITH_PF_USEGRAVITY) != 0
        && floorSlopeDot < 1.0f
        && (floorAttachScale < 0.80000001f || !rdVector_IsZero3(&pPhysics->velocity)) )
    {
        float slopeGravityFactor = STDMATH_CLAMP(1.0f - floorAttachScale, 0.2f, 0.80000001f);
        float slopeGravity       = sithWorld_g_pCurrentWorld->gravity * secDeltaTime * slopeGravityFactor;
        pPhysics->velocity.z    -= slopeGravity;
    }

    //
    // Remove velocity component into surface
    //
    if ( !rdVector_IsZero3(&pPhysics->velocity) )
    {
        float normalVelocity = rdVector_Dot3(&pPhysics->velocity, &floorNormal);
        normalVelocity       = stdMath_ClipNearZero(normalVelocity);

        // Only in water/aetherium
        if ( (pThing->pInSector
            && ((pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0
                || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0))
            && normalVelocity != 0.0f )
        {
            rdVector_MultAcc3(&pPhysics->velocity, &floorNormal, -normalVelocity);
        }
    }

    // Clip near-zero velocities
    rdMath_ClipVector3Acc(&pPhysics->velocity, STDMATH_ZERO_EPSILON);

    //
    // Calculate position delta
    //
    if ( !rdVector_IsZero3(&pPhysics->velocity) )
    {
        rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
    }

    //
    // Adjust for attachment distance based on thing height
    // i.e.: move thing up or down to maintain proper distance from surface, e.g., standing on ground
    //
    float thingHeight = sithPhysics_GetThingHeight(pThing);
    float heightDiff  = stdMath_ClipNearZero(distToFloor - thingHeight);

    if ( heightDiff != 0.0f )
    {
        float maxAdjust = 1.5f * secDeltaTime;

        if ( pThing->type == SITH_THING_PLAYER )
        {
            if ( fabsf(heightDiff) >= 0.03999999910593033f )
            {
                maxAdjust = 0.5f * secDeltaTime;
            }
        }

        heightDiff = STDMATH_CLAMP(heightDiff, -maxAdjust, maxAdjust);

        // Apply height adjustment along normal or up vector
        if ( (pPhysics->flags & SITH_PF_ALIGNUP) != 0 )
        {
            rdVector_MultAcc3(&pPhysics->deltaVelocity, &rdroid_g_zVector3, -heightDiff);
           // TODO: Could just do: pPhysics->deltaVelocity.z -= heightDiff;
        }
        else
        {
            rdVector_MultAcc3(&pPhysics->deltaVelocity, &floorNormal, -heightDiff);
        }
    }
}

int J3DAPI sithPhysics_CreateMineCarUserBlock(SithThing* pThing)
{
    if ( pThing->userblock.pMinecar )
    {
        return 1;
    }

    pThing->userblock.pMinecar = (SithMineCarUserBlock*)STDMALLOC(sizeof(SithMineCarUserBlock));
    if ( !pThing->userblock.pMinecar )
    {
        SITHLOG_ERROR("_CreateMineCarUserBlock failed to allocate.\n");
        return 0;
    }

    SithMineCarUserBlock* pMinecar = pThing->userblock.pMinecar;
    STD_ZEROMEM(pMinecar, sizeof(SithMineCarUserBlock));

    if ( !stdUtil_StrCmp(pThing->aName, "mineplayer") )
    {
        pMinecar->type = SITHMINECAR_PLAYER;
        sithPhysics_InitMineCarExhaust(pThing, &pThing->userblock.pMinecar->exhaustInfo);
        sithPhysics_InitMineCarChassis(pThing, &pThing->userblock.pMinecar->chassisInfo);
        sithPhysics_InitMineCarFxState(pThing, &pThing->userblock.pMinecar->fxstate);
        sithPhysics_InitMineCarState(pThing, &pThing->userblock.pMinecar->state);
        pThing->userblock.pMinecar->state.bEngineAnim = 1;

        // Added
        sithPhysics_InitMineCarLights(pThing, &pThing->userblock.pMinecar->state.lights);
    }
    else if ( !stdUtil_StrCmp(pThing->aName, "killtruk") )
    {
        pMinecar->type = SITHMINECAR_KILLTRUCK_AI;
        sithPhysics_InitTrackTruckExhaust(pThing, &pThing->userblock.pMinecar->exhaustInfo);
        sithPhysics_InitTrackTruckChassis(pThing, &pThing->userblock.pMinecar->chassisInfo);
        sithPhysics_InitTrackTruckState(pThing, &pThing->userblock.pMinecar->state);
        sithPhysics_InitVehicleFxState(pThing, &pThing->userblock.pMinecar->fxstate.engine);
    }
    else if ( !stdUtil_StrCmp(pThing->aName, "commietruck") )
    {
        pMinecar->type = SITHMINECAR_TRUCK_AI;
        sithPhysics_InitTrackTruckExhaust(pThing, &pThing->userblock.pMinecar->exhaustInfo);
        sithPhysics_InitTrackTruckChassis(pThing, &pThing->userblock.pMinecar->chassisInfo);
        sithPhysics_InitTrackTruckState(pThing, &pThing->userblock.pMinecar->state);
        sithPhysics_InitVehicleFxState(pThing, &pThing->userblock.pMinecar->fxstate.engine);
    }
    else if ( !stdUtil_StrCmp(pThing->aName, "jeep_cs") || !stdUtil_StrCmp(pThing->aName, "jeep_cs_pyr") )
    {
        pMinecar->type = SITHMINECAR_JEEP_AI;
        sithPhysics_InitJeepExhaust(pThing, &pThing->userblock.pMinecar->exhaustInfo);
        sithPhysics_InitTrackJeepChassis(pThing, &pThing->userblock.pMinecar->chassisInfo);
        sithPhysics_InitMineCarState(pThing, &pThing->userblock.pMinecar->state);
        sithPhysics_InitVehicleFxState(pThing, &pThing->userblock.pMinecar->fxstate.engine);
    }
    else if ( !stdUtil_StrCmp(pThing->aName, "mine_2commcar") )
    {
        pMinecar->type = SITHMINECAR_MINECAR_AI;
        sithPhysics_InitMineCarExhaust(pThing, &pThing->userblock.pMinecar->exhaustInfo);
        sithPhysics_InitMineCarChassis(pThing, &pThing->userblock.pMinecar->chassisInfo);
        sithPhysics_InitMineCarFxState(pThing, &pThing->userblock.pMinecar->fxstate);
        sithPhysics_InitMineCarState(pThing, &pThing->userblock.pMinecar->state);

        // Added
        sithPhysics_InitMineCarLights(pThing, &pThing->userblock.pMinecar->state.lights);
    }
    else
    {
        if ( !stdUtil_StrCmp(pThing->aName, "boulder_mc") )
        {
            pMinecar->type = SITHMINECAR_BOULDER;
        }
        else
        {
            pMinecar->type = SITHMINECAR_DEFAULT;
        }

        sithPhysics_InitMineCarExhaustDefault(pThing, &pThing->userblock.pMinecar->exhaustInfo);
        sithPhysics_InitMineCarChassisDefault(pThing, &pThing->userblock.pMinecar->chassisInfo);
        sithPhysics_InitMineCarState(pThing, &pThing->userblock.pMinecar->state);
    }

    return 1;
}

void J3DAPI sithPhysics_InitMineCarFxState(SithThing* pThing, SithMineCarFxState* pFxState)
{
    J3D_UNUSED(pThing);
    pFxState->engine.secUpdateInterval = 0.2f;
    pFxState->engine.secUpdateTimer    = 0.0f;
    pFxState->railClackDistance        = 2.0f;
    pFxState->railClackState           = 2.0f;
}

void J3DAPI sithPhysics_InitVehicleFxState(SithThing* pThing, SithVehicleEngineFxState* pFxState)
{
    J3D_UNUSED(pThing);
    pFxState->secUpdateInterval = 0.050000001f;
    pFxState->secUpdateTimer    = 0.0f;
}

void J3DAPI sithPhysics_InitMineCarChassisDefault(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    J3D_UNUSED(pThing);
    pChassisInfo->numNodes = 0;
}

void J3DAPI sithPhysics_InitMineCarChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    J3D_UNUSED(pThing);
    pChassisInfo->numNodes = 2;

    STD_STRCPY(pChassisInfo->aWheelMeshNames[0], "bkwheel");
    STD_STRCPY(pChassisInfo->aWheelMeshNames[1], "frwheel");

    pChassisInfo->aWheelNodeNums[0] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[0]);
    pChassisInfo->aWheelNodeNums[1] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[1]);
    pChassisInfo->wheelRadius = 0.015f;
}

void J3DAPI sithPhysics_InitTrackTruckChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    J3D_UNUSED(pThing);
    pChassisInfo->numNodes = 2;

    STD_STRCPY(pChassisInfo->aWheelMeshNames[0], "bkwheel");
    STD_STRCPY(pChassisInfo->aWheelMeshNames[1], "frwheel");

    pChassisInfo->aWheelNodeNums[0] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[0]);
    pChassisInfo->aWheelNodeNums[1] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[1]);
    pChassisInfo->wheelRadius       = 0.037500001f;
}

void J3DAPI sithPhysics_InitTrackJeepChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    J3D_UNUSED(pThing);
    pChassisInfo->numNodes = 4;

    STD_STRCPY(pChassisInfo->aWheelMeshNames[0], "brwheel");
    STD_STRCPY(pChassisInfo->aWheelMeshNames[1], "flwheel");
    STD_STRCPY(pChassisInfo->aWheelMeshNames[2], "blwheel");
    STD_STRCPY(pChassisInfo->aWheelMeshNames[3], "frwheel");

    pChassisInfo->aWheelNodeNums[0] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[0]);
    pChassisInfo->aWheelNodeNums[1] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[1]);
    pChassisInfo->aWheelNodeNums[2] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[2]);
    pChassisInfo->aWheelNodeNums[3] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[3]);

    pChassisInfo->wheelRadius = 0.037500001f;
}

void J3DAPI sithPhysics_InitMineCarExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    J3D_UNUSED(pThing);

    pExhaustInfo->bEnabled = 1;
    pExhaustInfo->spacing  = 0.0080000004f;

    pExhaustInfo->posY = -0.090000004f;
    pExhaustInfo->posZ = -0.034000002f;
    pExhaustInfo->posX = -0.02f;

    pExhaustInfo->lifeFactor     = 0.2f;
    pExhaustInfo->lifeRandFactor = 0.2f;
    pExhaustInfo->life           = 0.15000001f;

    pExhaustInfo->sizeStart      = 0.01f;
    pExhaustInfo->sizeEnd        = 0.1f;
    pExhaustInfo->sizeRandFactor = 0.01f;

    pExhaustInfo->alphaStart      = 0.15000001f;
    pExhaustInfo->alphaEnd        = 0.0f;
    pExhaustInfo->alphaRandFactor = 0.01f;

    pExhaustInfo->scatterX = 0.1f;
    pExhaustInfo->dirX     = -0.1f;

    pExhaustInfo->scatterZ = 0.1f;
    pExhaustInfo->dirZ     = 0.0f;

    STD_STRCPY(pExhaustInfo->aTemplate, "+exhaust");
}

void J3DAPI sithPhysics_InitJeepExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    J3D_UNUSED(pThing);

    pExhaustInfo->bEnabled = sithPhysics_bJeepExhaust; // Altered: assign value via var
    pExhaustInfo->spacing  = 0.0080000004f;

    pExhaustInfo->posY = -0.16f;
    pExhaustInfo->posZ = -0.034000002f;
    pExhaustInfo->posX = 0.02f;

    pExhaustInfo->life           = 0.050000001f;
    pExhaustInfo->lifeFactor     = 0.02f;
    pExhaustInfo->lifeRandFactor = 0.2f;

    pExhaustInfo->sizeStart      = 0.01f;
    pExhaustInfo->sizeEnd        = 0.1f;
    pExhaustInfo->sizeRandFactor = 0.01f;

    pExhaustInfo->alphaStart      = 0.15000001f;
    pExhaustInfo->alphaEnd        = 0.0f;
    pExhaustInfo->alphaRandFactor = 0.01f;

    pExhaustInfo->scatterX = 0.1f;
    pExhaustInfo->dirX     = 0.0f;

    pExhaustInfo->scatterZ = 0.1f;
    pExhaustInfo->dirZ     = 0.0f;

    STD_STRCPY(pExhaustInfo->aTemplate, "+exhaust");
}

void J3DAPI sithPhysics_InitTrackTruckExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    J3D_UNUSED(pThing);

    pExhaustInfo->bEnabled = 1;
    pExhaustInfo->spacing  = 0.0080000004f;

    pExhaustInfo->posY = -0.30000001f;
    pExhaustInfo->posZ = -0.1f;
    pExhaustInfo->posX = 0.06f;

    pExhaustInfo->lifeFactor     = 0.2f;
    pExhaustInfo->lifeRandFactor = 0.2f;
    pExhaustInfo->life           = 0.1f;

    pExhaustInfo->sizeStart     = 0.01f;
    pExhaustInfo->sizeEnd        = 0.1f;
    pExhaustInfo->sizeRandFactor = 0.01f;

    pExhaustInfo->alphaStart      = 0.15000001f;
    pExhaustInfo->alphaEnd        = 0.0f;
    pExhaustInfo->alphaRandFactor = 0.01f;

    pExhaustInfo->scatterX = 0.1f;
    pExhaustInfo->dirX     = 0.0f;

    pExhaustInfo->scatterZ = 0.1f;
    pExhaustInfo->dirZ     = 0.0f;

    STD_STRCPY(pExhaustInfo->aTemplate, "+exhaust");
}

void J3DAPI sithPhysics_InitMineCarExhaustDefault(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    J3D_UNUSED(pThing);
    pExhaustInfo->bEnabled = 0; // disable
}

void J3DAPI sithPhysics_InitMineCarState(SithThing* pThing, SithMineCarState* pState)
{
    J3D_UNUSED(pThing);

    pState->maybeMoveSate      = 0;
    pState->bOnAdjoinTrack     = 0;
    pState->pEngineAnim        = NULL;
    pState->bEngineAnim        = 0;
    pState->bUpdateSparksRight = 0;
    pState->bUpdateSparksLeft  = 0;
    pState->secTimeSparksRight = 0.0f;
    pState->secTimeSparksLeft  = 0.0f;
    pState->surfDrag           = pThing->moveInfo.physics.surfDrag + 1.0f;
    pState->unknown10          = 0.2f;
    pState->unknown9           = 0.30000001f;
    pState->bBraking           = 0;
}

void J3DAPI sithPhysics_InitTrackTruckState(SithThing* pThing, SithMineCarState* pState)
{
    J3D_UNUSED(pThing);

    pState->maybeMoveSate      = 0;
    pState->bOnAdjoinTrack     = 0;
    pState->pEngineAnim        = NULL;
    pState->bEngineAnim        = 0;
    pState->bUpdateSparksRight = 0;
    pState->bUpdateSparksLeft  = 0;
    pState->secTimeSparksRight = 0.0f;
    pState->secTimeSparksLeft  = 0.0f;
    pState->surfDrag           = pThing->moveInfo.physics.surfDrag + 1.0f;
    pState->unknown10          = 0.60000002f;
    pState->unknown9           = 0.80000001f;
    pState->bBraking           = 0;
}

void J3DAPI sithPhysics_InitMineCarLights(SithThing* pThing, SithVehicleLights* pLights)
{
    // TODO: A better solution would be to flag thing not to sync to savegame files
    // First remove & destroy any attached thing
    // This is required due to savegame preserves light things
    SithThing* pNextAttach = NULL;
    for ( SithThing* pAttach = pThing->pAttachedThing; pAttach; pAttach = pNextAttach )
    {
        pNextAttach = pAttach->pNextAttachedThing;
        sithThing_DetachThing(pAttach);
        sithThing_DestroyThing(pAttach);
    }

    // Init front light
    rdModel3HNode* pFrLampJoint = rdModel3_FindNamedNode("frlamp", pThing->renderData.data.pModel3);
    if ( pFrLampJoint )
    {
        SithVehicleLight* pLight = &pLights->aFrontLights[0];
        pLights->numFrontLights  = 1;

        // Set lamp mesh
        pLight->prdLightMesh = &pThing->renderData.data.pModel3->aGeos[0].aMeshes[pFrLampJoint->meshIdx];
    }

   // Init rear light
    rdModel3HNode* pBkLampJoint = rdModel3_FindNamedNode("bklamp", pThing->renderData.data.pModel3);
    if ( pBkLampJoint )
    {
        SithVehicleLight* pLight = &pLights->aRearLights[0];
        pLights->numRearLights   = 1;

        // Set lamp mesh
        pLight->prdLightMesh = &pThing->renderData.data.pModel3->aGeos[0].aMeshes[pBkLampJoint->meshIdx];

        // Creat dynamic red light
        SithThing* pLightTmpl = sithTemplate_GetTemplate("ghost");

        if ( pLightTmpl ) // Fixed: Added null check
        {
            // Get lamp world position
            // Lamp light is put slightly down and to the left of lamp mesh.
            // That's due to the issues with dynamic lights and world surfaces, and hopefully all world surface won't be lit by this light.
            rdVector3 lampPos = { 0.023807, -0.08997, 0.103777 - sithPhysics_GetThingHeight(pThing) }; // local space
            rdMatrix_TransformVector34Acc(&lampPos, &pThing->orient);
            rdVector_Add3Acc(&lampPos, &pThing->pos); // no make it world space

            SithThing* pLightThing = sithThing_CreateThingAtPos(pLightTmpl, &lampPos, &pThing->orient, pThing->pInSector, pThing->pParent);
            if ( pLightThing )
            {
                pLightThing->flags |= SITH_TF_EMITLIGHT;

                pLightThing->light.color.red   = 1.0f;
                pLightThing->light.color.green = 0.0f;
                pLightThing->light.color.blue  = 0.0f;
                pLightThing->light.color.alpha = 0.08f;

                pLightThing->light.minRadius   = 0.08f;
                pLightThing->light.maxRadius   = 0.08f;

                pLight->pLightThing = pLightThing;

                sithThing_AttachThingToThing(pLightThing, pThing);
                pLightThing->attach.flags |= SITH_ATTACH_NOMOVE; // Make sure light won't swing around minecar
            }
        }
    }

    // Turn off all lights
    sithPhysics_SetVehicleLights(pLights, /*bOn=*/false);
}

void J3DAPI sithPhysics_UpdateMineCarPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;

    // Clamp delta time
    if ( secDeltaTime > 0.1f )
    {
        secDeltaTime = 0.1f;
    }

    if ( !sithPhysics_CreateMineCarUserBlock(pThing) )
    {
        return;
    }

    SithMineCarState* pCarState = &pThing->userblock.pMinecar->state;

    pPhysics->flags &= ~SITH_PF_UNKNOWN_200000;

    // Get attachment info
    rdVector3 floorNormal;
    float distToFloor;
    float floorAttachScale;
    sithPhysics_GetThingAttachNormalAndDistance(pThing, &floorNormal, &distToFloor, &floorAttachScale);
    floorAttachScale = STDMATH_CLAMP(floorAttachScale, 0.25f, 1.0f);

    //
    // Apply drag
    //
    if ( !rdVector_IsZero3(&pPhysics->velocity) && pPhysics->surfDrag != 0.0f )
    {
        if ( (pPhysics->flags & SITH_PF_FORCEAPPLIED) != 0 )
        {
            pPhysics->flags &= ~SITH_PF_FORCEAPPLIED;
        }
        else if ( rdVector_IsZero3(&pPhysics->thrust)
            && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) == 0
            && floorAttachScale > 0.80000001f )
        {
            // Apply static drag when dying or normal operation
            float staticDrag, surfDrag;
            if ( (pThing->flags & SITH_TF_DYING) != 0 )
            {
                staticDrag = 0.30000001f * floorAttachScale;
                surfDrag   = 3.0f * floorAttachScale;
            }
            else
            {
                staticDrag = pPhysics->staticDrag * floorAttachScale;
                surfDrag   = pPhysics->surfDrag * floorAttachScale;
            }

            sithPhysics_ApplyDrag(&pPhysics->velocity, surfDrag, staticDrag, secDeltaTime);
        }
        else // we have thrust here
        {
            // Calculate slope-based drag
            float slopeZ = pThing->orient.lvec.z;
            if ( pPhysics->thrust.y < 0.0f )
            {
                slopeZ = -slopeZ;
            }

            float targetDrag = pPhysics->surfDrag + J3DMAX(slopeZ * 6.0f + 1.0f, 0.0f);
            //targetDrag = J3DMAX(targetDrag, 0.0f); // TODO: that J3DMAX seems redundant since slopeZ should be positive. Do another J3DMAX here?

            // Smoothly transition drag
            float dragDiff = targetDrag - pCarState->surfDrag;
            float dragChangeRate;

            if ( fabsf(slopeZ) >= 0.1f )
            {
                dragChangeRate = 4.0f * secDeltaTime;
            }
            else
            {
                dragChangeRate = (dragDiff >= 0.0f ? 0.2f : 1.0f) * secDeltaTime;
            }

            if ( fabsf(dragDiff) > dragChangeRate )
            {
                pCarState->surfDrag += (dragDiff >= 0.0f ? 1.0f : -1.0f) * dragChangeRate;
            }
            else
            {
                pCarState->surfDrag = targetDrag;
            }

            float drag = pCarState->surfDrag * floorAttachScale;
            sithPhysics_ApplyDrag(&pPhysics->velocity, drag, 0.0f, secDeltaTime);
        }
    }

    //
    // Calculate speed and thrust
    //
    float speed           = rdVector_Len3(&pPhysics->velocity);
    float thrustDelta     = 0.0f;
    float scaledDeltaTime = secDeltaTime * floorAttachScale;

    if ( rdVector_IsZero3(&pPhysics->velocity) )
    {
        thrustDelta = fabsf(pPhysics->thrust.y) * scaledDeltaTime;
    }
    else
    {
        float velocityDot = rdVector_Dot3(&pPhysics->velocity, &pThing->orient.lvec);
        if ( velocityDot > 0.0f ) // Moving forward
        {
            if ( pPhysics->thrust.y < 0.0f ) // Backward thrust
            {
                // Backward thrust while moving forward - brake
                thrustDelta = -pPhysics->thrust.y * scaledDeltaTime;
                if ( fabsf(thrustDelta) > speed )
                {
                    rdVector_Zero3(&pPhysics->velocity);
                    thrustDelta = 0.0f;
                }
            }
            else
            {
                // Forward thrust while moving forward - accelerate
                thrustDelta = pPhysics->thrust.y * scaledDeltaTime;
            }
        }
        else // Moving backward
        {
            if ( pPhysics->thrust.y >= 0.0f ) // if forward thrust
            {
                // Forward thrust while moving backward - brake
                thrustDelta = -pPhysics->thrust.y * scaledDeltaTime;
                if ( fabsf(thrustDelta) > speed )
                {
                    rdVector_Zero3(&pPhysics->velocity);
                    thrustDelta = 0.0f;
                }
            }
            else
            {
                // Backward thrust while moving backward - accelerate
                thrustDelta = -pPhysics->thrust.y * scaledDeltaTime;
            }
        }
    }

    //
    // Process track movement
    //
    thrustDelta = stdMath_ClipNearZero(thrustDelta);
    speed += thrustDelta;
    speed = J3DMIN(speed, sithPhysics_mineCarMaxSpeed); // Max speed cap
    sithPhysics_ProcessMineCarTrackMove(pThing, secDeltaTime, speed, &pPhysics->deltaVelocity);

    // Apply angular velocity
    if ( pPhysics->angularVelocity.yaw != 0.0f || pPhysics->angularVelocity.pitch != 0.0 )
    {
        rdVector3 angularDelta;
        rdVector_Scale3(&angularDelta, &pPhysics->angularVelocity, secDeltaTime);

        rdMatrix34 rotMat;
        rdMatrix_BuildRotate34(&rotMat, &angularDelta);
        sithCollision_RotateThing(pThing, &rotMat);
        rdMatrix_Normalize34(&pThing->orient);
    }

    // Update chassis if not forcing sparks
    if ( !pCarState->bBraking )
    {
        sithPhysics_UpdateMineCarChassis(pThing, &pThing->userblock.pMinecar->chassisInfo, secDeltaTime);
    }

    // Update effects
    sithPhysics_UpdateTrackVehicleFx(pThing, pThing->userblock.pMinecar, secDeltaTime);
    sithPhysics_UpdateExhaustFx(pThing, &pThing->userblock.pMinecar->exhaustInfo, secDeltaTime);
}

void J3DAPI sithPhysics_UpdateTrackVehicleFx(SithThing* pThing, SithMineCarUserBlock* pMineCarUserBlock, float secDeltaTime)
{
    switch ( pMineCarUserBlock->type )
    {
        case SITHMINECAR_PLAYER:
        case SITHMINECAR_MINECAR_AI:
            sithPhysics_UpdateMineCarFx(pThing, &pMineCarUserBlock->fxstate, secDeltaTime);
            break;

        case SITHMINECAR_KILLTRUCK_AI:
        case SITHMINECAR_TRUCK_AI:
            sithPhysics_UpdateTrackTruckFx(pThing, &pMineCarUserBlock->fxstate.engine, secDeltaTime);
            break;

        case SITHMINECAR_JEEP_AI:
            sithPhysics_UpdateTrackJeepFx(pThing, &pMineCarUserBlock->fxstate.engine, secDeltaTime);
            break;

        default:
            break;
    }
}

void J3DAPI sithPhysics_UpdateMineCarFx(SithThing* pThing, SithMineCarFxState* pFxState, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics   = &pThing->moveInfo.physics;
    SithMineCarState* pCarState = &pThing->userblock.pMinecar->state;

    float speed = rdVector_Len3(&pPhysics->velocity);

    // Check if attached to water surface
    bool bOnWater = false;
    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0
        && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_SHALLOWWATER) != 0 )
    {
        bOnWater = true;
    }

    // Update engine sound timer
    bool bUpdateEngineSndFx = false;
    pFxState->engine.secUpdateTimer -= secDeltaTime;
    if ( pFxState->engine.secUpdateTimer < 0.0f )
    {
        bUpdateEngineSndFx = true;
        pFxState->engine.secUpdateTimer = pFxState->engine.secUpdateInterval;
    }

    //
    // Engine run fx
    //
    if ( sithPhysics_bMineCarEngineRunFx )
    {
        if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_BOARDING )
        {
      /*      sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
            SITH_ASSERTREL(!pCarState->pEngineAnim);
            pCarState->pEngineAnim = NULL;*/

            sithPhysics_PowerOffMineCar(pThing);
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT
            || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT )
        {
      /*      sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
            if ( pCarState->pEngineAnim && pCarState->bEngineAnim )
            {
                sithAnimate_Stop(pCarState->pEngineAnim);
                pCarState->pEngineAnim = NULL;
            }*/

            sithPhysics_PowerOffMineCar(pThing);

        }
        else
        {
            sithPhysics_PowerOnMineCar(pThing);

            //sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_LWALKMETAL, sithPhysics_mineCarEngineVolume); // sol_minecar_motor_run.wav. Note indymine.snd flags this mode as play thing once so it's ok to call it multiple times

            //if ( !pCarState->pEngineAnim && pCarState->bEngineAnim )
            //{
            //    rdMaterial* pEngineMat = sithMaterial_Load("minecar_a_ngenfrnt.mat");
            //    if ( pEngineMat )
            //    {
            //        // Fixed: Added flag SITHANIMATE_NOSYNC to prevent engine animation from being written to savegame or over network.
            //        //        This prevents dangling engine mat animation on load since minecar state is not synced to savegame file.
            //        pCarState->pEngineAnim = sithAnimate_StartMaterialAnim(pEngineMat, sithPhysics_mineCarEngineAnimFPS, SITHANIMATE_LOOP | SITHANIMATE_NOSYNC);
            //    }
            //}

            if ( bUpdateEngineSndFx )
            {
                float volume = speed * sithPhysics_mineCarEngineVolIncrement + sithPhysics_mineCarEngineVolume;
                sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_LWALKMETAL, volume);

                float pitchRange = sithPhysics_mineCarEngineMaxPitch - sithPhysics_mineCarEngineMinPitch;
                float pitchScale = pitchRange / pPhysics->maxVelocity;
                float pitch      = STDMATH_CLAMP(speed * pitchScale + sithPhysics_mineCarEngineMinPitch,
                    sithPhysics_mineCarEngineMinPitch, sithPhysics_mineCarEngineMaxPitch
                );

                sithSoundClass_SetModePitch(pThing, SITHSOUNDCLASS_LWALKMETAL, pitch);

                if ( pCarState->pEngineAnim && pCarState->bEngineAnim )
                {
                    float animSpeed = (pitch - 1.0f) * sithPhysics_mineCarEngineAnimScale + 1.0f;
                    pCarState->pEngineAnim->msecPerFrame = (int)(1000.0f / (animSpeed * sithPhysics_mineCarEngineAnimFPS));
                }
            }
        }
    }
    else // !sithPhysics_bMineCarEngineRunFx
    {
        sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
        if ( pCarState->pEngineAnim && (pThing->type & SITH_THING_PLAYER) != 0 )
        {
            sithAnimate_Stop(pCarState->pEngineAnim);
            pCarState->pEngineAnim = NULL;
        }
    }

    //
    // Car rumble sound
    //
    if ( sithPhysics_bMineCarEngineRumbleFx && !bOnWater )
    {
        if ( speed == 0.0f )
        {
            if ( !sithSoundClass_IsModeFadingVol(pThing, SITHSOUNDCLASS_RWALKMETAL) )
            {
                sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_RWALKMETAL, 0.0f, sithPhysics_mineCarRumbleFadeTime);
            }
        }
        else
        {
            sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_RWALKMETAL, sithPhysics_mineCarRumbleMinVolume); // sol_minecar_rumble.wav
            if ( bUpdateEngineSndFx )
            {
                // TODO: Clamping?
                float volume = speed * sithPhysics_mineCarRumbleMaxVolume + sithPhysics_mineCarRumbleMinVolume;
                sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_RWALKMETAL, volume);
            }
        }
    }
    else
    {
        sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_RWALKMETAL);
    }

    //
    // Sparks effects
    //
    if ( sithPhysics_bMineCarSparksFx && !bOnWater )
    {
        float halfSpeed = speed * sithPhysics_mineCarSparkVolScale;

        if ( pCarState->maybeMoveSate == 3 || pCarState->maybeMoveSate == 1 ) // Leaning states or turn state
        {
            float absAngularVel = fabsf(pPhysics->angularVelocity.yaw);
            if ( absAngularVel > sithPhysics_mineCarSparkMinAngularVel
                && speed > sithPhysics_mineCarSparkMinSpeed
                && SITH_RANDF() < halfSpeed )
            {
                float volume = (speed - sithPhysics_mineCarSparkMinSpeed) * sithPhysics_mineCarSparkVolIncrement
                    + sithPhysics_mineCarSparkMinVolume;
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RWALKHARD); // sol_minecar_screech_rail1.wav
                sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_RWALKHARD, volume);

                if ( pCarState->maybeMoveSate == 1 )
                {
                    pCarState->bUpdateSparksRight = 1;
                    pCarState->secTimeSparksRight = 0.0f;
                }
                else if ( pCarState->maybeMoveSate == 3 )
                {
                    pCarState->bUpdateSparksLeft = 1;
                    pCarState->secTimeSparksLeft = 0.0f;
                }
            }
        }
        else if ( pCarState->maybeMoveSate == 5 ) // Centered state
        {
            sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_RWALKHARD, 0.0f, sithPhysics_mineCarSparkFadeVolume);
            pCarState->bUpdateSparksRight = 0;
            pCarState->bUpdateSparksLeft  = 0;
        }

        //
        // Set sparks if braking
        //
        if ( pCarState->bBraking )
        {
            if ( !pCarState->bUpdateSparksRight )
            {
                pCarState->bUpdateSparksRight = 1;
                pCarState->secTimeSparksRight = 0.0f;
            }

            if ( !pCarState->bUpdateSparksLeft )
            {
                pCarState->bUpdateSparksLeft = 1;
                pCarState->secTimeSparksLeft = 0.0f;
            }
        }

        // Update right sparks
        if ( pCarState->bUpdateSparksRight )
        {
            pCarState->secTimeSparksRight -= secDeltaTime;
            if ( pCarState->secTimeSparksRight <= 0.0f )
            {
                rdVector3 sparkPos = sithPhysics_mineCarSparkPosRight; // TODO: no need for temp variable
                sithFX_CreateMineCarSparks(pThing, &sparkPos);
                pCarState->secTimeSparksRight = sithPhysics_mineCarSparkInterval;
            }
        }

        // Update left sparks
        if ( pCarState->bUpdateSparksLeft )
        {
            pCarState->secTimeSparksLeft -= secDeltaTime;
            if ( pCarState->secTimeSparksLeft <= 0.0f )
            {
                rdVector3 sparkPos = sithPhysics_mineCarSparkPosLeft; // TODO: no need for temp variable
                sithFX_CreateMineCarSparks(pThing, &sparkPos);
                pCarState->secTimeSparksLeft = sithPhysics_mineCarSparkIntervalLeft;
            }
        }
    }

    //
    // Clatter sound like when on wooden bridge
    //
    if ( sithPhysics_bMineCarClatterFx && !bOnWater )
    {
        if ( pCarState->bOnAdjoinTrack && speed != 0.0f )
        {
            // TODO: Clamping?
            float volume = speed * sithPhysics_mineCarClatterVolScale + sithPhysics_mineCarClatterMinVolume;
            sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_LRUNMETAL, volume); // sol_minecar_clatter.wav
        }
        else if ( !sithSoundClass_IsModeFadingVol(pThing, SITHSOUNDCLASS_LRUNMETAL) )
        {
            sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_LRUNMETAL, 0.0f, sithPhysics_mineCarClatterFadeEndVol);
        }
    }
    else
    {
        sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_LRUNMETAL, 0.0f, sithPhysics_mineCarClatterFadeTime);
    }

    // Rail clack sound
    if ( sithPhysics_bMineCarRailClackFx && !bOnWater )
    {
        if ( speed == 0.0f )
        {
            pFxState->railClackState = pFxState->railClackDistance;
        }
        else
        {
            float distRange = sithPhysics_mineCarRailClackMaxDist - sithPhysics_mineCarRailClackMinDist;
            float distScale = distRange / pPhysics->maxVelocity;
            float distDelta = (speed * distScale + sithPhysics_mineCarRailClackMinDist) * secDeltaTime;

            float prevState = pFxState->railClackState;
            if ( prevState <= 0.0f )
            {
                pFxState->railClackState += distDelta;
            }
            else
            {
                pFxState->railClackState -= distDelta;
            }

            // Check for zero crossing (clack trigger)
            int prevSign = (prevState >= 0.0f) ? 1 : -1;
            int curSign = (pFxState->railClackState >= 0.0f) ? 1 : -1;

            if ( curSign != prevSign || pFxState->railClackState == 0.0f )
            {
                // Play clack sound
                if ( pFxState->railClackState < 0.0f )
                {
                    sithSoundClass_PlayMode(pThing, SITHSOUNDCLASS_LWALKHARD, 0.0f); // sol_minecar_rail_clack.wav
                }
                else
                {
                    sithSoundClass_PlayMode(pThing, SITHSOUNDCLASS_LWALKHARD, 1.0f); // sol_minecar_rail_clack.wav
                }

                float volume = speed * sithPhysics_mineCarRailClackVolScale + sithPhysics_mineCarRailClackMinVolume;
                float basePitch = speed * sithPhysics_mineCarRailClackPitchVariation + 1.0f;
                float pitch = basePitch + (SITH_RANDF() * 0.01f - 0.005f);

                // TODO: clamp volume and pitch?
                sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_LWALKHARD, volume);
                sithSoundClass_SetModePitch(pThing, SITHSOUNDCLASS_LWALKHARD, pitch);

                // Reset state with random variation
                float newDist = SITH_RANDF() * 0.2f + pFxState->railClackDistance;
                pFxState->railClackState = -prevSign * newDist;
            }
        }
    }
}

void J3DAPI sithPhysics_UpdateTrackJeepFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float secDeltaTime)
{
    if ( (pThing->flags & SITH_TF_DYING) != 0 )
    {
        return;
    }

    // Update engine sound timer
    bool bUpdateEngine = false;
    pFxState->secUpdateTimer -= secDeltaTime;
    if ( pFxState->secUpdateTimer < 0.0f )
    {
        bUpdateEngine = true;
        pFxState->secUpdateTimer = pFxState->secUpdateInterval;
    }

    if ( !sithPhysics_bUpdateTrackJeepEngineFx )
    {
        sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
        return;
    }

    sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_LWALKMETAL, sithPhysics_trackJeepMinSndVolume);

    if ( bUpdateEngine )
    {
        const float speed = rdVector_Len3(&pThing->moveInfo.physics.velocity);

        // Calculate volume based on speed
        const float volRange = sithPhysics_trackJeepMaxSndVolume - sithPhysics_trackJeepMinSndVolume;
        const float volScale = volRange / sithPhysics_trackJeepMaxSpeed;
        const float volume = STDMATH_CLAMP(speed * volScale + sithPhysics_trackJeepMinSndVolume,
            sithPhysics_trackJeepMinSndVolume, sithPhysics_trackJeepMaxSndVolume
        );

        sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_LWALKMETAL, volume);

        // Calculate pitch based on speed
        const float pitchRange = sithPhysics_trackJeepMaxSndPitch - sithPhysics_trackJeepMinSndPitch;
        const float pitchScale = pitchRange / sithPhysics_trackJeepMaxSpeed;
        const float pitch = STDMATH_CLAMP(speed * pitchScale + sithPhysics_trackJeepMinSndPitch,
            sithPhysics_trackJeepMinSndPitch, sithPhysics_trackJeepMaxSndPitch
        );

        sithSoundClass_SetModePitch(pThing, SITHSOUNDCLASS_LWALKMETAL, pitch);
    }
}

void J3DAPI sithPhysics_UpdateTrackTruckFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float secDeltaTime)
{
    if ( (pThing->flags & SITH_TF_DYING) != 0 )
    {
        return;
    }

    // Update engine sound timer
    bool bUpdateEngine = false;
    pFxState->secUpdateTimer -= secDeltaTime;
    if ( pFxState->secUpdateTimer < 0.0f )
    {
        bUpdateEngine = true;
        pFxState->secUpdateTimer = pFxState->secUpdateInterval;
    }

    if ( !sithPhysics_bUpdateTrackTruckEngineFx )
    {
        sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
        return;
    }

    sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_LWALKMETAL, sithPhysics_trackTruckMinSndVolume);

    if ( bUpdateEngine )
    {
        const float speed = rdVector_Len3(&pThing->moveInfo.physics.velocity);

        // Calculate volume based on speed
        const float volRange = sithPhysics_trackTruckMaxSndVolume - sithPhysics_trackTruckMinSndVolume;
        const float volScale = volRange / sithPhysics_trackTruckMaxSpeed;
        const float volume   = STDMATH_CLAMP(speed * volScale + sithPhysics_trackTruckMinSndVolume,
            sithPhysics_trackTruckMinSndVolume, sithPhysics_trackTruckMaxSndVolume
        );

        sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_LWALKMETAL, volume);

        // Calculate pitch based on speed
        const float pitchRange = sithPhysics_trackTruckMaxSndPitch - sithPhysics_trackTruckMinSndPitch;
        const float pitchScale = pitchRange / sithPhysics_trackTruckMaxSpeed;
        const float pitch      = speed * pitchScale + sithPhysics_trackTruckMinSndPitch;
        // TODO: do clamping as in sithPhysics_UpdateTrackJeepFx function

        sithSoundClass_SetModePitch(pThing, SITHSOUNDCLASS_LWALKMETAL, pitch);
    }
}
void J3DAPI sithPhysics_UpdateMineCarChassis(SithThing* pThing, const SithVehicleChassisInfo* pChassisInfo, float secDeltaTime)
{
    if ( pChassisInfo->numNodes == 0 || rdVector_IsZero3(&pThing->moveInfo.physics.velocity) )
    {
        return;
    }

    // Calculate wheel rotation based on speed
    float deltaAngle = sithPhysics_CalcWheelRotationAngle(pThing, pChassisInfo->wheelRadius, secDeltaTime);

    // Update all wheel joint angles
    for ( size_t i = 0; i < pChassisInfo->numNodes; i++ )
    {
        if ( pChassisInfo->aWheelNodeNums[i] != -1 )
        {
            pThing->renderData.apTweakedAngles[pChassisInfo->aWheelNodeNums[i]].pitch -= deltaAngle;
        }
    }
}

void J3DAPI sithPhysics_UpdateExhaustFx(SithThing* pThing, const SithVehicleExhaustInfo* pExhaustInfo, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;

    if ( !pExhaustInfo->bEnabled
        || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_BOARDING
        || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT
        || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT )
    {
        return;
    }

    // Calculate exhaust spawn position
    rdVector3 exhaustPos = pThing->pos;
    rdVector_MultAcc3(&exhaustPos, &pThing->orient.lvec, pExhaustInfo->posY);
    rdVector_MultAcc3(&exhaustPos, &pThing->orient.uvec, pExhaustInfo->posZ);
    rdVector_MultAcc3(&exhaustPos, &pThing->orient.rvec, pExhaustInfo->posX);
    rdVector_Add3Acc(&exhaustPos, &pPhysics->deltaVelocity);

    // Calculate exhaust direction
    rdVector3 exhaustDir;
    rdVector_Normalize3(&exhaustDir, &pPhysics->deltaVelocity);

    // Reverse direction if moving backward
    if ( rdVector_Dot3(&pThing->orient.lvec, &pPhysics->velocity) < 0.0f )
    {
        rdVector_Scale3Acc(&exhaustDir, -0.5f);
    }

    // Calculate speeds
    float deltaSpeed = rdVector_Len3(&pPhysics->deltaVelocity);
    float speed = rdVector_Len3(&pPhysics->velocity);

    // Calculate number of smoke puffs to spawn
    size_t numSmokes = (size_t)(deltaSpeed / pExhaustInfo->spacing);
    numSmokes = STDMATH_CLAMP(numSmokes, 1, 10);

    float smokeDeltaTime = secDeltaTime / (float)numSmokes;

    SithThing* pSmokeTmpl = sithTemplate_GetTemplate(pExhaustInfo->aTemplate);
    if ( !pSmokeTmpl ) // Fixed: move this check out of for loop
    {
        return;
    }

    const float maxLifeTime = pPhysics->maxVelocity * pExhaustInfo->lifeFactor + pExhaustInfo->life;

    for ( size_t i = 0; i < numSmokes; i++ )
    {
        float smokeTime = (float)i * smokeDeltaTime;

        // Calculate random scatter
        rdVector3 smokeVel = { 0 };

        float scatterX = (float)((SITH_RAND() - 0.5f) * pExhaustInfo->scatterX) + pExhaustInfo->dirX;
        rdVector_Scale3(&smokeVel, &pThing->orient.rvec, scatterX * speed);

        float scatterZ = (float)(SITH_RAND() * (pExhaustInfo->scatterZ - pExhaustInfo->dirZ)) + pExhaustInfo->dirZ;
        rdVector_MultAcc3(&smokeVel, &pThing->orient.uvec, scatterZ * speed);

        // Move exhaust position back along exhaust direction
        rdVector_MultAcc3(&exhaustPos, &exhaustDir, -pExhaustInfo->spacing);

        rdVector3 smokePos = exhaustPos;
        rdVector_MultAcc3(&smokePos, &pThing->orient.rvec, scatterX * smokeTime);
        rdVector_MultAcc3(&smokePos, &pThing->orient.uvec, scatterZ * smokeTime);

        // Create exhaust sprite
        SithThing* pSmoke = sithThing_CreateThingAtPos(pSmokeTmpl, &smokePos, &pThing->orient, pThing->pInSector, NULL);
        if ( !pSmoke )
        {
            continue;
        }

        // Calculate lifetime
        float life = pExhaustInfo->lifeFactor * speed + pExhaustInfo->life - smokeTime;
        life += (SITH_RANDF() - 0.5f) * pExhaustInfo->lifeRandFactor * life;
        pSmoke->msecLifeLeft = (uint32_t)(life * 1000.0f);

        // Calculate sprite sizes
        float animProgress = smokeTime / maxLifeTime;

        float startMinSize = pExhaustInfo->sizeStart + (pExhaustInfo->sizeEnd - pExhaustInfo->sizeStart) * animProgress;
        float startAlpha   = pExhaustInfo->alphaStart + (pExhaustInfo->alphaEnd - pExhaustInfo->alphaStart) * animProgress;

        rdVector3 startSize;
        startSize.x = startMinSize + (float)((SITH_RAND() - 0.5f) * pExhaustInfo->sizeRandFactor);
        startSize.y = startMinSize + (float)((SITH_RAND() - 0.5f) * pExhaustInfo->sizeRandFactor);
        startSize.z = startAlpha + (float)((SITH_RAND() - 0.5f) * pExhaustInfo->alphaRandFactor);

        rdVector3 endSize;
        endSize.x = pExhaustInfo->sizeEnd + (float)((SITH_RAND() - 0.5f) * pExhaustInfo->sizeRandFactor);
        endSize.y = pExhaustInfo->sizeEnd + (float)((SITH_RAND() - 0.5f) * pExhaustInfo->sizeRandFactor);
        endSize.z = pExhaustInfo->alphaEnd;

        sithAnimate_StartAnimateSpriteSize(pSmoke, &startSize, &endSize, life);
        pSmoke->moveInfo.physics.velocity = smokeVel;
    }
}

void J3DAPI sithPhysics_SetVehicleLight(SithVehicleLight* pLight, bool bOn)
{
    if ( pLight->prdLightMesh )
    {
        pLight->prdLightMesh->lightMode = bOn ? RD_LIGHTING_NONE : RD_LIGHTING_GOURAUD;
    }

    if ( pLight->pLightThing )
    {
        if ( bOn )
        {
            pLight->pLightThing->flags |= SITH_TF_EMITLIGHT;
            pLight->pLightThing->flags &= ~SITH_TF_DISABLED;
        }
        else
        {
            pLight->pLightThing->flags |= SITH_TF_DISABLED;
            pLight->pLightThing->flags &= ~SITH_TF_EMITLIGHT;
        }
    }
}

void J3DAPI sithPhysics_SetVehicleLights(SithVehicleLights* pLights, bool bOn)
{
    for ( size_t i = 0; i < pLights->numFrontLights; i++ )
    {
        sithPhysics_SetVehicleLight(&pLights->aFrontLights[i], bOn);
    }

    for ( size_t i = 0; i < pLights->numRearLights; i++ )
    {
        sithPhysics_SetVehicleLight(&pLights->aRearLights[i], bOn);
    }
}

void J3DAPI sithPhysics_PowerOnMineCar(SithThing* pThing)
{
    // Play motor run sound fx. The sound has to be repeatedly played as cog script might stop it.
    // Note the indymine.snd flags this mode as play thing once so it's ok to call it multiple times
    sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_LWALKMETAL, sithPhysics_mineCarEngineVolume); // sol_minecar_motor_run.wav. 

    SithMineCarState* pCarState = &pThing->userblock.pMinecar->state;
    if ( !pCarState->bEngineRunning ) // Added
    {


        if ( !pCarState->pEngineAnim && pCarState->bEngineAnim )
        {
            rdMaterial* pEngineMat = sithMaterial_Load("minecar_a_ngenfrnt.mat");
            if ( pEngineMat )
            {
                // Fixed: Added flag SITHANIMATE_NOSYNC to prevent engine animation from being written to savegame or over network.
                //        This prevents dangling engine mat animation on load since minecar state is not synced to savegame file.
                pCarState->pEngineAnim = sithAnimate_StartMaterialAnim(pEngineMat, sithPhysics_mineCarEngineAnimFPS, SITHANIMATE_LOOP | SITHANIMATE_NOSYNC);
            }
        }

        // Added: Falowing is new code
        pThing->flags |= SITH_TF_EMITLIGHT; // Added: Turn on rear red light
        pThing->thingInfo.actorInfo.flags |= SITH_AF_HEADLIGHT;
        pThing->thingInfo.actorInfo.headLightIntensity.alpha = 2.0f; // light range

        sithPhysics_SetVehicleLights(&pCarState->lights, /*bOn=*/ true);
        pCarState->bEngineRunning = true;
    }
}

void J3DAPI sithPhysics_PowerOffMineCar(SithThing* pThing)
{
    SithMineCarState* pCarState = &pThing->userblock.pMinecar->state;
    if ( pCarState->bEngineRunning ) // Added
    {
        // Stop engine sound fx
        sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);

        // Play engine stop sound fx
        // Added: Moved from sithVehicleControls module
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STOPMOVE);// engine stop sol_minecar_motor_stop.wav

        if ( pCarState->pEngineAnim && pCarState->bEngineAnim )
        {
            sithAnimate_Stop(pCarState->pEngineAnim);
            pCarState->pEngineAnim = NULL;
        }

        // Added: Falowing is new code
        pThing->flags &= SITH_TF_EMITLIGHT; // Added: Turn on rear red light
        pThing->thingInfo.actorInfo.flags &= ~SITH_AF_HEADLIGHT;
        sithPhysics_SetVehicleLights(&pCarState->lights, /*bOn=*/ false);
        pCarState->bEngineRunning = false;
    }

}

void J3DAPI sithPhysics_UpdateRaftPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    pPhysics->flags &= ~SITH_PF_UNKNOWN_200000;

    //
    // Get attachment surface info
    //
    rdVector3 floorNormal;
    float distToFloor;
    float floorAttachScale;
    sithPhysics_GetThingAttachNormalAndDistance(pThing, &floorNormal, &distToFloor, &floorAttachScale);

    float floorSlopeDot = rdVector_Dot3(&floorNormal, &rdroid_g_zVector3);

    //
    // Get surrounding water surface normals and distances
    // based on corners of raft
    //

    // Note, OG code had all loops unrooled

    // Sample water surface at 4 corners of raft
    rdVector3 cornerOffsets[4] =
    {
        { 0.059999999f,  0.090000004f, 0.0f },  // Front right
        {-0.059999999f,  0.090000004f, 0.0f },  // Front left
        {-0.059999999f, -0.090000004f, 0.0f },  // Back left
        { 0.059999999f, -0.090000004f, 0.0f }   // Back right
    };

    rdVector3 cornerPos[STD_ARRAYLEN(cornerOffsets)];
    rdVector3 cornerSurfNormals[STD_ARRAYLEN(cornerOffsets)];
    SithPhysicsWaterSurfaceType cornerSurfTypes[STD_ARRAYLEN(cornerOffsets)];

    for ( size_t i = 0; i < STD_ARRAYLEN(cornerOffsets); i++ )
    {
        // Transform corner offset to world position
        cornerPos[i] = cornerOffsets[i];
        rdMatrix_TransformPoint34Acc(&cornerPos[i], &pThing->orient);
        rdVector_Add3Acc(&cornerPos[i], &pThing->pos);

        float dist;
        cornerSurfTypes[i] = sithPhysics_CheckWaterSurfaceAtPos(pThing, &dist, &cornerSurfNormals[i], &cornerPos[i], secDeltaTime);
        cornerPos[i].z -= dist;
    }

    //
    // Get center water surface normal and distance
    //
    rdVector3 centerPos = pThing->pos;
    rdVector3 centerSurfNormal;
    float centerDist;
    SithPhysicsWaterSurfaceType centerSurfType = sithPhysics_CheckWaterSurfaceAtPos(pThing, &centerDist, &centerSurfNormal, &centerPos, secDeltaTime);
    centerPos.z -= centerDist;

    // Store center water surface type in userval
    pThing->userval = (float)centerSurfType;

    //
    // Calculate corner normals and new look direction
    //
    rdVector3 newLookDir = { 0 };
    rdVector3 cornerNormals[STD_ARRAYLEN(cornerOffsets)]; // front-right, front-left, back-left, back-right 
    for ( size_t i = 0; i < STD_ARRAYLEN(cornerPos); i++ )
    {
        int nextIdx = (i + 1) % STD_ARRAYLEN(cornerPos);

        rdVector3 edge1, edge2;
        rdVector_Sub3(&edge1, &cornerPos[nextIdx], &cornerPos[i]);
        rdVector_Sub3(&edge2, &centerPos, &cornerPos[nextIdx]);

        rdVector_Cross3(&cornerNormals[nextIdx], &edge1, &edge2);
        rdVector_Normalize3Acc(&cornerNormals[nextIdx]);

        rdVector_Add3Acc(&newLookDir, &cornerNormals[nextIdx]);
    }

    rdVector_Normalize3Acc(&newLookDir);

    //
    // Twist raft front & back joints angles based on surface corner heights
    //
    // Note, raft node names are wrong and inraftfr is raft back side
    // and inraftbk is raft front side
    int raftBackIdx  = sithThing_GetThingJointIndex(pThing, "inraftfr");
    int raftFrontIdx = sithThing_GetThingJointIndex(pThing, "inraftbk");

    if ( raftBackIdx != -1 || raftFrontIdx != -1 )
    {
        // Project average water surface normal onto plane perpendicular to raft's right vector
        // This removes the lateral (left-right) component, keeping only forward-up component
        rdVector3 newLookDirProjected;
        rdMath_ProjectPointOntoPlaneNormalized(&newLookDirProjected, &newLookDir, &pThing->orient.rvec, &rdroid_g_zeroVector3);

        // Create reference up vector in the raft's pitch plane (perpendicular to right vector)
        rdVector3 pitchPlaneUp;
        rdVector_Cross3(&pitchPlaneUp, &newLookDirProjected, &pThing->orient.rvec);

        // TODO: the normals below are switched.
        // You could use correct one according to the side is being twisted with negative dots

        // Update back raft twist angle
        if ( raftBackIdx != -1 )
        {
            // Use front left corner normal and remove right vector component
            rdVector3 frontNormalProjected;
            rdMath_ProjectPointOntoPlaneNormalized(&frontNormalProjected, &cornerNormals[1], &pThing->orient.rvec, &rdroid_g_zeroVector3);

            float frontAngleDot = rdVector_Dot3(&frontNormalProjected, &pitchPlaneUp);
            float frontAngle    = stdMath_ArcSin1(frontAngleDot);
            pThing->renderData.apTweakedAngles[raftBackIdx].pitch = frontAngle * 1.0f;
        }

        // Update front raft twist angle
        if ( raftFrontIdx != -1 )
        {
            // Use back right corner normal and remove right vector component 
            rdVector3 backNormalProjected;
            rdMath_ProjectPointOntoPlaneNormalized(&backNormalProjected, &cornerNormals[3], &pThing->orient.rvec, &rdroid_g_zeroVector3);

            float backAngleDot = rdVector_Dot3(&backNormalProjected, &pitchPlaneUp);
            float backAngle    = stdMath_ArcSin1(backAngleDot);
            pThing->renderData.apTweakedAngles[raftFrontIdx].pitch = backAngle * 1.0f;
        }
    }

    //
    // Orient raft to average surface normal
    //
    float orientSpeed = pPhysics->orientSpeed * secDeltaTime;
    sithPhysics_SetThingLook(pThing, &newLookDir, orientSpeed);

    //
    // Update angular velocity
    //
    if ( (pPhysics->flags & SITH_PF_USEANGULARTHRUST) != 0 )
    {
        if ( !rdVector_IsZero3(&pPhysics->angularVelocity) )
        {
            float angularDrag = pPhysics->surfDrag + 0.4f;
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, angularDrag, 0.0f, secDeltaTime);
        }

        // Clamp angular velocity when on water
        if ( centerSurfType >= SITHPHYSICS_WATERSURFACE_ADJOIN ) // adjoin or water surface
        {
            rdMath_ClampVector3Acc(&pPhysics->angularVelocity, -pPhysics->maxRotationVelocity, pPhysics->maxRotationVelocity); // Clamp angular velocity to max rotation velocity
        }

        // Apply yaw thrust
        pPhysics->angularVelocity.yaw += pPhysics->rotThrust.yaw * secDeltaTime;
        rdMath_ClipVector3Acc(&pPhysics->angularVelocity, STDMATH_ZERO_EPSILON);
    }

    //
    // Apply angular velocity
    //
    if ( pPhysics->angularVelocity.yaw != 0.0f )
    {
        rdVector3 angularDelta;
        rdVector_Scale3(&angularDelta, &pPhysics->angularVelocity, secDeltaTime);

        rdMatrix34 rotMat;
        rdMatrix_BuildRotate34(&rotMat, &angularDelta);
        sithCollision_RotateThing(pThing, &rotMat);
        rdMatrix_Normalize34(&pThing->orient);
    }


    // Clamp attachment scale
    floorAttachScale = STDMATH_CLAMP(floorAttachScale, 0.25f, 1.0f);

    //
    // Apply drag
    //
    if ( !rdVector_IsZero3(&pPhysics->velocity) && pPhysics->surfDrag != 0.0f )
    {
        if ( (pPhysics->flags & SITH_PF_FORCEAPPLIED) != 0 )
        {
            // Skip applying drag if force was applied in current frame
            pPhysics->flags &= ~SITH_PF_FORCEAPPLIED;
        }
        // Apply surf drag with static drag if no thrust 
        else if ( rdVector_IsZero3(&pPhysics->thrust)
            && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) == 0
            && floorAttachScale > 0.80000001f )
        {
            float staticDrag = pPhysics->staticDrag * floorAttachScale;
            float surfDrag   = pPhysics->surfDrag * floorAttachScale;
            sithPhysics_ApplyDrag(&pPhysics->velocity, surfDrag, staticDrag, secDeltaTime);
        }
        // Applay only surface drag
        else
        {
            float surfDrag = pPhysics->surfDrag * floorAttachScale;
            sithPhysics_ApplyDrag(&pPhysics->velocity, surfDrag, 0.0f, secDeltaTime);
        }
    }

    //
    // Apply thrust
    //
    rdVector3 thrustDelta = { 0 };
    if ( (pPhysics->flags & SITH_PF_USETHRUST) != 0
        && !rdVector_IsZero3(&pPhysics->thrust)
        && centerSurfType >= SITHPHYSICS_WATERSURFACE_ADJOIN )
    {
        float thrustScale = secDeltaTime * floorAttachScale;
        rdVector_Scale3(&thrustDelta, &pPhysics->thrust, thrustScale);
        rdMath_ClipVector3Acc(&thrustDelta, STDMATH_ZERO_EPSILON);

        if ( !rdVector_IsZero3(&thrustDelta) )
        {
            rdMatrix_TransformVector34Acc(&thrustDelta, &pThing->orient); // Transform to world space
        }
    }

    //
    // Apply down slope thrust
    //
    if ( floorSlopeDot < 1.0f )
    {
        // Calculate slope direction (downhill)
        rdVector3 slopeRDir;
        rdVector_Cross3(&slopeRDir, &floorNormal, &rdroid_g_zVector3);

        rdVector3 slopeDir;
        rdVector_Cross3(&slopeDir, &floorNormal, &slopeRDir);
        rdVector_Normalize3Acc(&slopeDir);

        float slopeThrust = (1.0f - floorSlopeDot) * sithPhysics_raftSteepSlopeFactor * secDeltaTime;
        rdVector_MultAcc3(&thrustDelta, &slopeDir, slopeThrust);
    }

    //
    // Apply water thrust
    //
    sithPhysics_ApplyWaterThrust(pThing, &floorNormal, &thrustDelta, secDeltaTime);

    //
    // Apply thrust to velocity
    //
    rdVector_Add3Acc(&pPhysics->velocity, &thrustDelta);

    //
    // Bounce off walls at water surface corners
    //
    for ( size_t i = 0; i < STD_ARRAYLEN(cornerSurfNormals); i++ )
    {
        if ( cornerSurfTypes[i] == SITHPHYSICS_WATERSURFACE_NONE )
        {
            // Project corner normal onto plane, ie. remove up-down component
            rdVector3 wallNormal;
            rdMath_ProjectPointOntoPlaneNormalized(&wallNormal, &cornerSurfNormals[i], &pThing->orient.uvec, &rdroid_g_zeroVector3);

            float velDot = rdVector_Dot3(&pPhysics->velocity, &wallNormal);
            if ( velDot < 0.0f )
            {
                rdVector_MultAcc3(&pPhysics->velocity, &wallNormal, -velDot * sithPhysics_raftSolidSurfaceDragFactor);
            }
        }
    }

    // Project velocity onto water surface plane, removing up component
    // TODO: this projection should be probably a function.
    //       Same code can be found in sithPhysics_ApplyWaterThrust
    float velNormalDot = rdMath_DistancePointToPlane(&pPhysics->velocity, &floorNormal, &rdroid_g_zeroVector3); // Thrust along water surface normal... could also just use dot productrdVector_Dot3(&pPhysics->velocity, &floorNormal);
    float normalUpDot  = rdVector_Dot3(&floorNormal, &rdroid_g_zVector3); // TODO: could reuse floorSlopeDot
    float projectedVelScale = velNormalDot / normalUpDot;

    rdVector3 surfaceVelocity;
    rdVector_ScaleAdd3(&surfaceVelocity, &rdroid_g_zVector3, -projectedVelScale, &pPhysics->velocity);
    rdVector_Normalize3Acc(&surfaceVelocity);

    float speed = rdVector_Len3(&pPhysics->velocity);
    rdVector_Scale3(&pPhysics->velocity, &surfaceVelocity, speed);

    // 
    // Remove velocity into surface if moving into it
    //
    if ( !rdVector_IsZero3(&pPhysics->velocity) )
    {
        float velIntoSurface = rdVector_Dot3(&pPhysics->velocity, &floorNormal);
        velIntoSurface = stdMath_ClipNearZero(velIntoSurface);
        if ( velIntoSurface < 0.0f )
        {
            rdVector_MultAcc3(&pPhysics->velocity, &floorNormal, -velIntoSurface);
        }
    }

    // Clip near-zero velocities
    rdMath_ClipVector3Acc(&pPhysics->velocity, STDMATH_ZERO_EPSILON);

    //
    // Calculate position delta
    //
    if ( !rdVector_IsZero3(&pPhysics->velocity) )
    {
        rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
    }

    //
    // Adjust height to match water surface
    //
    float thingHeight = sithPhysics_GetThingHeight(pThing);
    float heightDiff  = stdMath_ClipNearZero(distToFloor - thingHeight);
    if ( heightDiff != 0.0f )
    {
        float maxAdjust = sithPhysics_raftHeightAdjustSpeed * secDeltaTime;
        heightDiff = STDMATH_CLAMP(heightDiff, -maxAdjust, maxAdjust);

        rdVector_MultAcc3(&pPhysics->deltaVelocity, &floorNormal, -heightDiff);
    }
}

SithPhysicsWaterSurfaceType J3DAPI sithPhysics_CheckWaterSurfaceAtPos(const SithThing* pThing, float* pOutDistance, rdVector3* pOutNormal, const rdVector3* pPos, float secDeltaTime)
{
    float moveDist = sithPhysics_GetThingHeight(pThing)
        + sithPhysics_raftWaterSurfaceSearchDist * secDeltaTime
        + sithPhysics_waterSurfaceSearchOffset;

    rdVector3 startPos = *pPos;
    startPos.z += sithPhysics_waterSurfaceSearchOffset;

    rdVector3 moveNorm = RDVECTOR_NEG3(rdroid_g_zVector3);

    SithSector* pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.001f);
    if ( !pStartSector )
    {
        // TODO: is this correct?
        //       Should we set pOutDistance and pOutNormal here?
        return SITHPHYSICS_WATERSURFACE_NONE;
    }

    bool bFoundSolid = false;

    sithCollision_SearchForCollisions(pStartSector, NULL, &startPos, &moveNorm, moveDist, 0.0f, 0x01);

    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        // Check for adjoin with underwater sector (water surface from above)
        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
            {
                *pOutNormal   = pCollision->pSurfaceCollided->face.normal;
                *pOutDistance = pCollision->distance - sithPhysics_waterSurfaceSearchOffset;
                sithCollision_DecreaseStackLevel();
                return SITHPHYSICS_WATERSURFACE_ADJOIN;
            }
        }
        // Check for "solid" water surface (i.e. cold water)
        else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATER) != 0
            && (pCollision->pSurfaceCollided->pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
        {
            if ( pCollision->pSurfaceCollided->face.normal.z >= 0.44999999f ) // ~63 deg from vertical
            {
                *pOutNormal   = pCollision->pSurfaceCollided->face.normal;
                *pOutDistance = pCollision->distance - sithPhysics_waterSurfaceSearchOffset;
                sithCollision_DecreaseStackLevel();
                return SITHPHYSICS_WATERSURFACE_WATER;
            }
        }
        // Remember first solid surface found
        else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && !bFoundSolid )
        {
            *pOutNormal = pCollision->pSurfaceCollided->face.normal;
            bFoundSolid = true;

            if ( pCollision->pSurfaceCollided->face.normal.z >= 0.44999999f ) // ~63 deg from vertical
            {
                *pOutDistance = pCollision->distance - sithPhysics_waterSurfaceSearchOffset;
            }
            else
            {
                *pOutDistance = sithPhysics_GetThingHeight(pThing);
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bFoundSolid )
    {
        return SITHPHYSICS_WATERSURFACE_NONE;
    }

    // No surface found - assume flat water surface at search distance
    *pOutDistance = moveDist - sithPhysics_waterSurfaceSearchOffset;
    *pOutNormal   = rdroid_g_zVector3;
    return SITHPHYSICS_WATERSURFACE_DEFAULT;
}

void J3DAPI sithPhysics_GetThingAttachNormalAndDistance(const SithThing* pThing, rdVector3* pAttachNormal, float* pDistToAttach, float* pScale)
{
    SITH_ASSERTREL(pThing->attach.flags);

    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
    {
        *pAttachNormal = pThing->attach.pFace->normal;
        *pDistToAttach = rdMath_DistancePointToPlane(&pThing->pos, pAttachNormal, &pThing->attach.attachedFaceFirstVert); // Calculate distance from thing to attached surface
        *pScale = 1.0f;
    }
    else if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 )
    {
        SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;

        // Transform face normal to world space
        rdMatrix_TransformVector34(pAttachNormal, &pThing->attach.pFace->normal, &pThingAttached->orient);

        // Transform face vertex to world space
        // Note could set dvec with position and use rdMatrix_TransformPoint34
        rdVector3 worldVert;
        rdMatrix_TransformVector34(&worldVert, &pThing->attach.attachedFaceFirstVert, &pThingAttached->orient);
        rdVector_Add3Acc(&worldVert, &pThingAttached->pos);

        *pDistToAttach = rdMath_DistancePointToPlane(&pThing->pos, pAttachNormal, &worldVert); // Calculate distance from thing to attached surface
        *pScale = 1.0f;
    }
    else if ( (pThing->attach.flags & SITH_ATTACH_CLIMBSURFACE) != 0 )
    {
        *pAttachNormal = pThing->attach.pFace->normal;
        *pDistToAttach = rdMath_DistancePointToPlane(&pThing->pos, pAttachNormal, &pThing->attach.attachedFaceFirstVert); // Calculate distance from thing to attached surface
        *pScale = 1.0f;
    }
}

void J3DAPI sithPhysics_ApplyWaterThrust(SithThing* pThing, const rdVector3* pWaterSurfNormal, rdVector3* pThrust, float secDeltaTime)
{
    if ( (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    // Search downward for water surface or underwater sector
    float moveDist     = sithPhysics_GetThingHeight(pThing) + sithPhysics_raftHeightAdjustSpeed;
    rdVector3 moveNorm = RDVECTOR_NEG3(rdroid_g_zVector3);

    sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, moveDist, 0.001f, 0x01);

    SithSector* pWaterSector = NULL;
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
            pWaterSector = pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector;
            if ( (pWaterSector->flags & SITH_SECTOR_UNDERWATER) != 0
                || (pWaterSector->flags & SITH_SECTOR_USETHRUST) != 0 )
            {
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATER) != 0
            && (pCollision->pSurfaceCollided->pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
        {
            pWaterSector = pCollision->pSurfaceCollided->pSector;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !pWaterSector || (pWaterSector->flags & SITH_SECTOR_USETHRUST) == 0 )
    {
        return;
    }

    // Project sector thrust onto water surface plane
    float thrustAlongNormal    = rdMath_DistancePointToPlane(&pWaterSector->thrust, pWaterSurfNormal, &rdroid_g_zeroVector3); // Thrust along water surface normal... could also just use dot product
    float normalAlongUp        = rdVector_Dot3(pWaterSurfNormal, &rdroid_g_zVector3);
    float projectedThrustScale = thrustAlongNormal / normalAlongUp;

    rdVector3 surfaceThrust;
    rdVector_ScaleAdd3(&surfaceThrust, &rdroid_g_zVector3, -projectedThrustScale, &pWaterSector->thrust);
    rdVector_Normalize3Acc(&surfaceThrust);

    // Scale by original thrust magnitude
    // Note: OG did it really inefficiently by calculating magnitude 3 times to multiply each component separately
    float thrustMagnitude = rdVector_Len3(&pWaterSector->thrust);
    rdVector_Scale3Acc(&surfaceThrust, thrustMagnitude);

    // Apply to thrust delta
    rdVector_MultAcc3(pThrust, &surfaceThrust, secDeltaTime);
}

void J3DAPI sithPhysics_ProcessMineCarTrackMove(SithThing* pThing, float secDeltaTime, float a3, rdVector3* a4)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_ProcessMineCarTrackMove, pThing, secDeltaTime, a3, a4);
}

int J3DAPI sithPhysics_ProcessTrackFace(SithThing* pThing, rdFace* pFace, rdVector3* a3, rdVector3* a4, rdVector3* a5, void* pData)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_ProcessTrackFace, pThing, pFace, a3, a4, a5, pData);
}

int J3DAPI sithPhysics_CheckForPointOnTrack(SithThing* pThing, const rdVector3* pPoint, rdFace** ppFoundFace, int bUpdateState, const rdFace* pPrevFace)
{
    // Search downward from point for track surface
    rdVector3 startPos = *pPoint;
    rdVector_MultAcc3(&startPos, &rdroid_g_zVector3, sithPhysics_trackSearchHeightOffset);

    rdVector3 moveNorm  = RDVECTOR_NEG3(rdroid_g_zVector3);
    SithSector* pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    SithSurface* pFoundSurf = NULL;
    int bFound = 0;

    sithCollision_SearchForCollisions(pSector, NULL, &startPos, &moveNorm, sithPhysics_trackSearchDistance, 0.001f, 0x01); // 0x01 = skip thing collisions

    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_TRACK) != 0 )
            {
                bFound     = 1;
                pFoundSurf = pCollision->pSurfaceCollided;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINCROSS) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_TRACK) != 0 )
        {
            bFound     = 1;
            pFoundSurf = pCollision->pSurfaceCollided;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !bFound || !ppFoundFace )
    {
        return bFound;
    }

    if ( pPrevFace && pPrevFace == &pFoundSurf->face )
    {
        SITHLOG_ERROR("Holy Smokes! CheckForPointOnTrack found prev face.\n");
    }

    *ppFoundFace = &pFoundSurf->face;

    if ( bUpdateState )
    {
        SithMineCarState* pState = &pThing->userblock.pMinecar->state;
        pState->bOnAdjoinTrack = (pCollision->pSurfaceCollided->pAdjoin != NULL) ? 1 : 0;
    }

    return bFound;
}

void J3DAPI sithPhysics_FreeVehicleLightEntry(SithVehicleLight* pLight)
{
    if ( pLight && pLight->pLightThing )
    {
        sithThing_DestroyThing(pLight->pLightThing);
        pLight->pLightThing = NULL;
    }
}

void J3DAPI sithPhysics_FreeUserBlockEntry(SithThing* pThing)
{
    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_MINECAR) != 0 )
        {
            if ( pThing->userblock.pMinecar )
            {
                SithVehicleLights* pLights = &pThing->userblock.pMinecar->state.lights;
                for ( size_t i = 0; i < pLights->numFrontLights; i++ )
                {
                    sithPhysics_FreeVehicleLightEntry(&pLights->aFrontLights[i]);
                }

                for ( size_t i = 0; i < pLights->numRearLights; i++ )
                {
                    sithPhysics_FreeVehicleLightEntry(&pLights->aRearLights[i]);
                }
            }
        }
    }
}

int J3DAPI sithPhysics_sub_487EC0(SithThing* pThing, void* pData, float* secDeltaTime, rdVector3* a4, rdVector3* a5, float a6)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_sub_487EC0, pThing, pData, secDeltaTime, a4, a5, a6);
}

void J3DAPI sithPhysics_UpdateJeepPhysics(SithThing* pThing, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateJeepPhysics, pThing, secDeltaTime);
}

int J3DAPI sithPhysics_Jeep_sub_4892E0(SithThing* pThing, void* a2, float secDeltaTime)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_Jeep_sub_4892E0, pThing, a2, secDeltaTime);
}

// function might check for jeep surface and thing face collision
void J3DAPI sithPhysics_sub_48A970(SithThing* pThing, void* a2, const rdVector3* a3)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_sub_48A970, pThing, a2, a3);
}

int J3DAPI sithPhysics_sub_48AD20(SithThing* pThing, void* a2, rdVector3* a3, float secDeltaTime)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_sub_48AD20, pThing, a2, a3, secDeltaTime);
}

int J3DAPI sithPhysics_CreateJeepUserBlock(SithThing* pThing)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_CreateJeepUserBlock, pThing);
}

void J3DAPI sithPhysics_Jeep_sub_48B4D0(SithThing* pThing, float* a2)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_Jeep_sub_48B4D0, pThing, a2);
}

void J3DAPI sithPhysics_UpdateJeepFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float a3)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateJeepFx, pThing, pFxState, a3);
}
