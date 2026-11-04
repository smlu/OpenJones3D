#ifndef SITH_SITHPHYSICS_H
#define SITH_SITHPHYSICS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

#include <rdroid/Math/rdVector.h>

#include <std/types.h>
#include <std/General/stdConffile.h>
#include <std/General/stdMath.h>

J3D_EXTERN_C_START

#define SITHPHYSICS_CFG_PHYSICS_FIXEDTIMESTEP "engine.physics.fixedTimestep" // fps

// Slope thresholds (cosine of angle)
#define SITHPHYSICS_SLIDE_SLOPE_MIN    0.69f       // ~46 deg - minimum slope for sliding
#define SITHPHYSICS_SLIDE_SLOPE_MAX    0.80000001f // ~37 deg - maximum slope for sliding

typedef enum eSithPhysicsWaterSurfaceType
{
    SITHPHYSICS_WATERSURFACE_NONE    = 0,
    SITHPHYSICS_WATERSURFACE_DEFAULT = 1,
    SITHPHYSICS_WATERSURFACE_ADJOIN  = 3,
    SITHPHYSICS_WATERSURFACE_WATER   = 4,
} SithPhysicsWaterSurfaceType;

void J3DAPI sithPhysics_Startup(void); // Added / New

void J3DAPI sithPhysics_FindFloor(SithThing* pThing, int bNoThingStateUpdate);
void J3DAPI sithPhysics_FindWaterSurface(SithThing* pThing);

// Main thing physics update function
void J3DAPI sithPhysics_UpdateThing(SithThing* pThing, float secDeltaTime);

// Force, drag and look functions
void J3DAPI sithPhysics_ApplyForce(SithThing* pThing, const rdVector3* force);
void J3DAPI sithPhysics_SetThingLook(SithThing* pThing, const rdVector3* look, float secDeltaTime);
void J3DAPI sithPhysics_ApplyDrag(rdVector3* pVelocity, float drag, float staticDrag, float secDeltaTime);

// Parse function
int J3DAPI sithPhysics_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum);

void J3DAPI sithPhysics_ResetThingMovement(SithThing* pThing);
float J3DAPI sithPhysics_GetThingHeight(const SithThing* pThing);

// Specific thing physics update functions.
// Note, these are called by sithPhysics_UpdateThing based on the state of the thing.
void J3DAPI sithPhysics_UpdateDetachedThingPhysics(SithThing* pThing, float secDeltaTime);
void J3DAPI sithPhysics_UpdateDetachedPlayerPhysics(SithThing* pThing, float secDeltaTime);
void J3DAPI sithPhysics_UpdateUnderwaterThingPhysics(SithThing* pThing, float secDeltaTime);
void J3DAPI sithPhysics_UpdateClimbingThingPhysics(SithThing* pThing, float secDeltaTime);
void J3DAPI sithPhysics_UpdateAttachedThingPhysics(SithThing* pThing, float secDeltaTime);

// MineCar physics
int J3DAPI sithPhysics_CreateMineCarUserBlock(SithThing* pThing);
void J3DAPI sithPhysics_UpdateMineCarPhysics(SithThing* pThing, float secDeltaTime);

// Raft physics
void J3DAPI sithPhysics_UpdateRaftPhysics(SithThing* pThing, float secDeltaTime);

// Jeep physics
int J3DAPI sithPhysics_CreateJeepUserBlock(SithThing* pThing);
void J3DAPI sithPhysics_UpdateJeepPhysics(SithThing* pThing, float secDeltaTime);

void sithPhysics_FreeUserBlockEntry(SithThing* pThing); // New

static inline bool J3DAPI sithPhysics_IsVehiclePhysics(const SithPhysicsInfo* pPhysics);
static inline bool J3DAPI sithPhysics_IsVehicleThing(const SithThing* pThing);

/**
 * Checks whether a given cosine-of-angle value corresponds to a valid slope angle.
 *
 * Assumes the input cosAngle was computed from two normalized vectors, e.g.:
 *   cosAngle = dot(normalizedVecA, normalizedVecB);
 * Thus cosAngle == cos(theta) where theta is the angle between the vectors.
 *
 * @param cosAngle - Cosine of the angle between two vectors (must be in range [-1.0, +1.0]).
 * @return true if theta (the angle itself) is within the configured slope angle limits; false otherwise.
 *
 * Example usage:
 *   float cosAngle = dot(forwardDir, moveDir);
 *   if (sithPhysics_CheckSlopeAngle(cosAngle)) {
 *       // is valid slope / alignment
 *   }
 */
static inline bool sithPhysics_CheckSlopeAngle(float cosAngle);  // new func

/**
 * Calculates the wheel rotation delta angle based on the thing's velocity and wheel radius.
 * @param pThing       - The vehicle thing whose wheel rotation angle is to be calculated.
 * @param wheelRadius  - The radius of the wheel.
 * @param secDeltaTime - The time delta in seconds.
 * @return The calculated wheel rotation angle in degrees at this frame.
 */
static inline float sithPhysics_CalcWheelRotationAngle(const SithThing* pThing, float wheelRadius, float secDeltaTime); // new func

// Helper hooking functions
void sithPhysics_ResetGlobals(void);


bool J3DAPI sithPhysics_IsVehiclePhysics(const SithPhysicsInfo* pPhysics)
{
    return (pPhysics->flags & (SITH_PF_UNKNOWN_8000000 | SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) != 0;
}

bool J3DAPI sithPhysics_IsVehicleThing(const SithThing* pThing)
{
    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        return sithPhysics_IsVehiclePhysics(&pThing->moveInfo.physics);
    }
    return false;
}

bool sithPhysics_CheckSlopeAngle(float cosAngle) // new func
{
    return  cosAngle < SITHPHYSICS_SLIDE_SLOPE_MAX && cosAngle > SITHPHYSICS_SLIDE_SLOPE_MIN;
}

float sithPhysics_CalcWheelRotationAngle(const SithThing* pThing, float wheelRadius, float secDeltaTime) // new func
{
    const float speed               = rdVector_Len3(&pThing->moveInfo.physics.velocity);
    const float wheelCircumference  = STDMATH_CIRCLE_CIRCUMF(wheelRadius);
    const float rotationsPerSec     = speed / wheelCircumference;

    // Reverse rotation if moving backward
    float rotDelta = stdMath_NormalizeAngle(360.0f * rotationsPerSec * secDeltaTime);
    if ( rdVector_Dot3(&pThing->moveInfo.physics.velocity, &pThing->orient.lvec) < 0.0f )
    {
        rotDelta = -rotDelta;
    }
    return rotDelta;
}

J3D_EXTERN_C_END
#endif // SITH_SITHPHYSICS_H
