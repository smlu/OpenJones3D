#ifndef SITH_SITHPLAYERACTIONS_H
#define SITH_SITHPLAYERACTIONS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define sithPlayerActions_g_jewelFlyingPuppetTrackNum J3D_DECL_FAR_VAR(sithPlayerActions_g_jewelFlyingPuppetTrackNum, int)
// extern int sithPlayerActions_g_jewelFlyingPuppetTrackNum ;

#define sithPlayerActions_g_pCurCollidedSurface J3D_DECL_FAR_VAR(sithPlayerActions_g_pCurCollidedSurface, SithSurface*)
// extern SithSurface *sithPlayerActions_g_pCurCollidedSurface;

#define sithPlayerActions_g_pCurCollidedModelFace J3D_DECL_FAR_VAR(sithPlayerActions_g_pCurCollidedModelFace, rdFace*)
// extern rdFace *sithPlayerActions_g_pCurCollidedModelFace;

#define sithPlayerActions_g_pCurCollidedModel J3D_DECL_FAR_VAR(sithPlayerActions_g_pCurCollidedModel, rdModel3*)
// extern rdModel3 *sithPlayerActions_g_pCurCollidedModel;

#define sithPlayerActions_g_pPlasma J3D_DECL_FAR_VAR(sithPlayerActions_g_pPlasma, SithThing*)
// extern SithThing *sithPlayerActions_g_pPlasma;

#define sithPlayerActions_g_bJewelFlying J3D_DECL_FAR_VAR(sithPlayerActions_g_bJewelFlying, int)
// extern int sithPlayerActions_g_bJewelFlying;

#define sithPlayerActions_g_bPlayerInvisible J3D_DECL_FAR_VAR(sithPlayerActions_g_bPlayerInvisible, int)
// extern int sithPlayerActions_g_bPlayerInvisible;

#define sithPlayerActions_g_pChalkWriteSurf J3D_DECL_FAR_VAR(sithPlayerActions_g_pChalkWriteSurf, SithSurface*)
// extern SithSurface *sithPlayerActions_g_pChalkWriteSurf;

#define sithPlayerActions_g_chalkWritePos J3D_DECL_FAR_VAR(sithPlayerActions_g_chalkWritePos, rdVector3)
// extern rdVector3 sithPlayerActions_g_chalkWritePos;

#define sithPlayerActions_g_pChalkWriteSector J3D_DECL_FAR_VAR(sithPlayerActions_g_pChalkWriteSector, SithSector*)
// extern SithSector *sithPlayerActions_g_pChalkWriteSector;

int J3DAPI sithPlayerActions_Activate(SithThing* pPlayer);

void J3DAPI sithPlayerActions_MoveToPlayerPosition(SithThing* pThing, size_t playerNum);
int J3DAPI sithPlayerActions_HasActiveWeapon(SithThing* pThing);

void J3DAPI sithPlayerActions_PushItem(SithThing* pThing, SithThing* pItem, const rdVector3* pDirection);
void J3DAPI sithPlayerActions_PullItem(SithThing* pThing, SithThing* pItem, const rdVector3* pDirection);

int J3DAPI sithPlayerActions_CanPullUp(SithThing* pThing);
void J3DAPI sithPlayerActions_ClimbPullUp(SithThing* pThing);

void J3DAPI sithPlayerActions_CenterOnClimbSurface(SithThing* pThing, const SithSurface* pSurface); // Centers player on climb surface and moves player to climb surface sector. No thing attachment to climb surface is made 
int J3DAPI sithPlayerActions_FindAndAttachToClimbWall(SithThing* pThing); // Finds climb world surfaces, attaches to it and makes climb mount animation
void J3DAPI sithPlayerActions_ClimbMove(SithThing* pThing, SithSurface* pSurf, int direction);
void J3DAPI sithPlayerActions_UnmountWall(SithThing* pThing, int trackNum);

int J3DAPI sithPlayerActions_CanClimbOn1m(SithThing* pThing);
void J3DAPI sithPlayerActions_ClimbOn1m(SithThing* pThing);

int J3DAPI sithPlayerActions_CanClimbOn2m(SithThing* pThing);
void J3DAPI sithPlayerActions_ClimbOn2m(SithThing* pThing);

// Returns
// 0 - none or unable to move (due to weapon other surfaces/things blocking the move)
// 1 - climb wall
// 2 - hang ledge
// 3 - angled climb wall 
int J3DAPI sithPlayerActions_CheckClimbDownWall(SithThing* pThing);
void J3DAPI sithPlayerActions_ClimbDownToClimb(SithThing* pThing, int bLedge); // Climb down to climb surface
void J3DAPI sithPlayerActions_ClimbDownToHang(SithThing* pThing); // Climb down to hang surface

float J3DAPI sithPlayerActions_FindLedge(SithThing* pThing, const rdVector3* pPYR, SithSurface** ppLedgeSurf, rdFace** ppLedgeThingFace, rdModel3Mesh** ppLedgeThingMesh, SithThing** ppLedgeThing);
int J3DAPI sithPlayerActions_GrabLedge(SithThing* pThing, float distance, SithSurface* pLedgeSurf, rdFace* pLedgeThingFace, const rdModel3Mesh* pLedgeThingMesh, SithThing* pLedgeThing);

void J3DAPI sithPlayerActions_Jump(SithThing* pThing, float speedFactor, int jumpDirection);
void J3DAPI sithPlayerActions_LeapForward(SithThing* pThing);
void J3DAPI sithPlayerActions_JumpForward(SithThing* pThing);
void J3DAPI sithPlayerActions_HopLeft(SithThing* pThing);
void J3DAPI sithPlayerActions_HopRight(SithThing* pThing);
void J3DAPI sithPlayerActions_JumpRollBack(SithThing* pThing);
void J3DAPI sithPlayerActions_JumpRollForward(SithThing* pThing);
void J3DAPI sithPlayerActions_StrafeLeft(SithThing* pThing);
void J3DAPI sithPlayerActions_StrafeRight(SithThing* pThing);
void J3DAPI sithPlayerActions_HopBack(SithThing* pThing);
void J3DAPI sithPlayerActions_JumpStart(SithThing* pThing);

void J3DAPI sithPlayerActions_Stand2Crawl(SithThing* pThing);
void J3DAPI sithPlayerActions_Crawl2Stand(SithThing* pThing);
void J3DAPI sithPlayerActions_MoveToCrawlPosition(SithThing* pThing);

/**
 * Checks if there is a valid walkable floor/surface or standable thing at the specified position that the player can move to.
 * Performs collision detection and surface angle validation to determine movement feasibility.
 *
 * @param pThing - The player/actor thing attempting to move
 * @param pPos   - The target position to check for valid floor
 * @param pbSurfaceChange (Optional) -  Output parameter: set to 1 if moving would change the surface/standable thing
 *                       the player is currently attached to, 0 if no change
 *
 * @return floor status:
 *         0 - No valid floor surface found (no world surface found, no standable thing found, slope or steep floor, blocked, lava, cliff wall, running in direction away of descending surface/ slope / wall)
 *         1 - Valid floor found  (walkable world surface or standable thing)
 *         2 - Non-floor world surface found (no sector found or tilted surface < 51 deg from vertical)
 *         3 - Ascending floor slope surface or non-floor steep surface (wall, cliff) found in direction of running movement (only returned when player is running)
 *             Note when running in the same direction as surface (away) e.g.: descending slope, 0 is returned instead
 */
int J3DAPI sithPlayerActions_CheckFloorAtPos(SithThing* pThing, rdVector3* pPos, int* pbSurfaceWillChange);

void sithPlayerActions_StartInvisibility(void);
void sithPlayerActions_EndInvisibility(void);
int sithPlayerActions_IsInvisible(void);

int sithPlayerActions_StartJewelFlying(void);
void J3DAPI sithPlayerActions_EnableJewelFlying(SithThing* pPlasma);
void J3DAPI sithPlayerActions_DisableJewelFlying();

void J3DAPI sithPlayerActions_QuickTurnLeft(SithThing* pThing); // Added from debug
void J3DAPI sithPlayerActions_QuickTurnRight(SithThing* pThing); // Added from debug

// Helper hooking functions
void sithPlayerActions_InstallHooks(void);
void sithPlayerActions_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHPLAYERACTIONS_H
