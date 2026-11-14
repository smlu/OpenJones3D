#ifndef SITH_SITHPLAYERCONTROLS_H
#define SITH_SITHPLAYERCONTROLS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#ifdef J3D_QOL_IMPROVEMENTS
/**
 * Fixed frame rate used by the key turn, fly pitch and jewel fly thrust controls.
 * The original engine scaled these controls by the current frame rate, so they got faster as FPS went up (issue #10).
 * The controls behave as at this frame rate at any FPS; change this value to tune their speed.
 * Also used in the speedrun build, so the control speed doesn't depend on FPS there either.
 */
#  define SITHPLAYERCONTROLS_FIXEDFPS   60.0f
#  define SITHPLAYERCONTROLS_CONTROLFPS SITHPLAYERCONTROLS_FIXEDFPS
#else
#  define SITHPLAYERCONTROLS_CONTROLFPS sithTime_g_fps
#endif

#define sithPlayerControls_g_bCutsceneMode J3D_DECL_FAR_VAR(sithPlayerControls_g_bCutsceneMode, int)
// extern int sithPlayerControls_g_bCutsceneMode;

extern bool sithPlayerControls_bActionKeyActive;

/**
 * Resets internal state
 * @note new function
 */
void sithPlayerControls_Reset(void);

void J3DAPI sithPlayerControls_PuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType);
int J3DAPI sithPlayerControls_Process(SithThing* pPlayerThing, float secDeltaTime);

int J3DAPI sithPlayerControls_BoardVehicle(SithThing* pThing, int bBoard);
void J3DAPI sithPlayerControls_ExitVehicle(SithThing* pThing);

void J3DAPI sithPlayerControls_RotateAimJoints(SithThing* pThing, float pitch, float yaw);
void J3DAPI sithPlayerControls_RotateAimJointsEx(SithThing* pThing, float pitch, float yaw, float secDeltaTime); // New

void J3DAPI sithPlayerControls_ResetAimJoints(SithThing* pThing);
void J3DAPI sithPlayerControls_ResetAimJointsEx(SithThing* pThing, float secDeltaTime); //New

void J3DAPI sithPlayerControls_ProcessWeaponAim(SithThing* pThing, float secDeltaTime);

SithThing* sithPlayerControls_GetTargetThing(void);

SithThing* J3DAPI sithPlayerControls_GetVehicleBoardedThing();
void J3DAPI sithPlayerControls_SetVehicleBoardedThing(SithThing* pThing);

/**
 * Calculates linear move thrust based on actor max thrust and extra speed
 * @param pActor      - Pointer to actor info
 * @param direction   - Direction factor. Should be negative for backward movement
 * @param speedFactor - Extra speed factor.
 * @return
 */
static inline float sithPlayerControls_CalculateThrust(SithActorInfo* pActor, float direction, float speedFactor)
{
    return (pActor->maxThrust + pActor->extraSpeed) * direction * speedFactor;
}

/**
* Calculates accelerated linear move thrust based on actor max thrust and time delta
* @param pActor       - Pointer to actor info
* @param direction    - Direction factor. Should be negative for backward movement
* @param speedFactor  - Extra speed factor.
* @param secDeltaTime - Frame time delta
*/
static inline float sithPlayerControls_CalculateAcceleratedThrust(SithActorInfo* pActor, float direction, float speedFactor, float secDeltaTime)
{
    // TODO: Would make sens to refactor this formula and remove frame time dependency altogether
    return (pActor->maxThrust * direction * speedFactor) + (direction * secDeltaTime);
}

/*
* Calculates key turn angular velocity based on actor max rotation velocity
* @param pActor        - Actor info
* @param axisDirection - Axis direction. Should be negative for right turn
* @param keyDirection  - Key direction. Should be negative for right turn
* @param speedFactor   - Extra speed factor. Should be <= 1.0f
*/
static inline float sithPlayerControls_CalculateAngularVelocity(SithActorInfo* pActor, float axisDirection, float keyDirection, float speedFactor)
{
    speedFactor = J3DMIN(speedFactor, 1.0f);
    // TODO: Would probably make sense to remove left part of the formula altogether and rely only on right part.
    // Fixed: Use the fixed control frame rate, so the key turn speed doesn't grow with FPS
    return (axisDirection * SITHPLAYERCONTROLS_CONTROLFPS) + (pActor->maxRotVelocity * keyDirection * speedFactor);
}

/*
* Calculates mouse turn angular velocity based on actor max rotation velocity
* @param pActor      - Actor info
* @param axisValue   - Mouse turn axis value read this frame. Should be negative for right turn
* @param keyValue    - Mouse turn key axis value. Should be negative for right turn
* @param speedFactor - Extra speed factor. Should be <= 1.0f
* @note Keeps the original current frame rate scale, which converts the per-frame axis value to a rate.
*/
static inline float sithPlayerControls_CalculateMouseAngularVelocity(SithActorInfo* pActor, float axisValue, float keyValue, float speedFactor)
{
    speedFactor = J3DMIN(speedFactor, 1.0f);
    return (axisValue * sithTime_g_fps) + (pActor->maxRotVelocity * keyValue * speedFactor);
}

/*
* Calculates the jewel fly up/down thrust step, which is added to the thrust every frame while the key is held
* @param secDeltaTime - Frame time delta
*/
static inline float sithPlayerControls_CalculateJewelFlyThrustStep(float secDeltaTime)
{
    const float thrustIncrement = 0.00019999999f;
#ifdef SITHPLAYERCONTROLS_FIXEDFPS
    // Fixed: Scale the step by frame time at the fixed control frame rate. The original step grew with FPS
    //        and was added every frame, so full thrust was reached much sooner at high FPS.
    return thrustIncrement * SITHPLAYERCONTROLS_FIXEDFPS * SITHPLAYERCONTROLS_FIXEDFPS * secDeltaTime;
#else
    J3D_UNUSED(secDeltaTime);
    return thrustIncrement * sithTime_g_fps;
#endif
}

// Helper hooking functions
void sithPlayerControls_InstallHooks(void);
void sithPlayerControls_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHPLAYERCONTROLS_H
