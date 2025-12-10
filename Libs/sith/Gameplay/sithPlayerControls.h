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

#define sithPlayerControls_g_bCutsceneMode J3D_DECL_FAR_VAR(sithPlayerControls_g_bCutsceneMode, int)

#define sithPlayerControls_g_angularVel_fps  30.0f
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
inline float sithPlayerControls_CalculateThrust(SithActorInfo* pActor, float direction, float speedFactor)
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
inline float sithPlayerControls_CalculateAcceleratedThrust(SithActorInfo* pActor, float direction, float speedFactor, float secDeltaTime)
{
    // TODO: Would make sens to refactor this formula and remove frame time dependency altogether
    return (pActor->maxThrust * direction * speedFactor) + (direction * secDeltaTime);
}

/*
* Calculates angular velocity based on actor max rotation velocity
* @param pActor        - Actor info
* @param axisDirection - Axis direction. Should be negative for right turn
* @param keyDirection  - Key direction. Should be negative for right turn
* @param speedFactor   - Extra speed factor. Should be <= 1.0f
*/
inline float sithPlayerControls_CalculateAngularVelocity(SithActorInfo* pActor, float axisDirection, float keyDirection, float speedFactor)
{
    speedFactor = J3DMIN(speedFactor, 1.0f);

    return (axisDirection * sithPlayerControls_g_angularVel_fps) + (pActor->maxRotVelocity * keyDirection * speedFactor);
}

// Helper hooking functions
void sithPlayerControls_InstallHooks(void);
void sithPlayerControls_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHPLAYERCONTROLS_H
