#ifndef SITH_SITHCOLLISION_H
#define SITH_SITHCOLLISION_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

void sithCollision_Startup(void);
void sithCollision_Shutdown(void);
void J3DAPI sithCollision_AddCollisionHandler(SithThingType type1, SithThingType type2, CollisionHandlerThing pProcessFunc, int (J3DAPI* pUnknownFunc)(SithThing*, SithThing*));
void J3DAPI sithCollision_AddSurfaceCollisionHandler(SithThingType type, CollisionHandlerSurface fpHandler);
SithCollision* sithCollision_PopStack(void);

SithSector* J3DAPI sithCollision_FindSectorInRadius(SithSector* pStartSector, const rdVector3* startPos, rdVector3* endPos, float radius);
SithSector* J3DAPI sithCollision_FindSectorAtThing(SithThing* pThing, SithSector* pStartSector, const rdVector3* startPos, rdVector3* endPos, float radius);
SithSector* J3DAPI sithCollision_FindWaterSector(SithSector* pStartSector, const rdVector3* startPos, rdVector3* endPos, float radius);
SithSurface* J3DAPI sithCollision_FindWaterSurface(SithSector* pStartSector, const rdVector3* startPos, rdVector3* endPos, float radius); // Added new

int J3DAPI sithCollision_CheckLOS(SithSector* pStartSector, const rdVector3* startPos, const rdVector3* endPos, float radius);
int J3DAPI sithCollision_CheckThingLOS(const SithThing* pViewer, const SithThing* pTarget, int a3);
void J3DAPI sithCollision_RotateThing(SithThing* pThing, const rdMatrix34* pOrient);
void J3DAPI sithCollision_ApplyThingOrient(SithThing* pThing, rdMatrix34* pOrient);
// Function returns distance the thing has travelled
float J3DAPI sithCollision_MoveThing(SithThing* pThing, const rdVector3* moveNorm, float moveDist, int flags);
float J3DAPI sithCollision_SearchForCollisions(SithSector* pStartSector, SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, int searchFlags);
void sithCollision_DecreaseStackLevel(void);
float J3DAPI sithCollision_SearchForThingCollisions(const SithSector* pSector, SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, int searchFlags);
// flags:
// 0x100 - skip check for adjoin flag SITH_ADJOIN_MOVE
// 0x200 - skip check for adjoin flag SITH_ADJOIN_NO_PLAYER_MOVE
void J3DAPI sithCollision_SearchForSurfaceCollisions(const SithSector* pSector, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, int flags);
int J3DAPI sithCollision_HandleThingHitSurface(SithThing* pThing, SithSurface* pSurface, SithCollision* pCollision);
// Note: Function handles collsions between 2 things.
// It returns True if there was collision otherwise False
int J3DAPI sithCollision_ThingCollisionHandler(SithThing* pSrcThing, SithThing* pThingCollided, SithCollision* pCollision, int bSecondThingIsSource);
void J3DAPI sithCollision_VehicleCollisionHandler(SithThing* pThing, SithThing* pHitThing, rdVector3* pForce, rdVector3* pInpactForce, int* a5, const rdVector3* hitNormal, float impactSpeed, float hitImpactSpeed);
int J3DAPI sithCollision_CollideHurt(SithThing* pThing, const rdVector3* pHitNorm, float distance, int a4, const SithSurface* pHitSurf);
SithThing* J3DAPI sithCollision_FindItemThing(SithThing* pThing, int* pbFoundFloorItem);
void J3DAPI sithCollision_PushThingCollision(SithThing* pHitThing, float distance, SithCollisionType hitType, rdModel3Mesh* pHitMesh, rdFace* pHitFace, rdVector3* hitNorm);
void J3DAPI sithCollision_PushSurfaceCollision(SithSurface* pSurf, float distance, SithCollisionType hitType, rdVector3* pHitNorm);
int J3DAPI sithCollision_ParticleAndActorCollisionHandler(SithThing* pSrcThing, SithThing* pThingCollided, SithCollision* pCollision, int bSecondThingIsSource);
float J3DAPI sithCollision_CheckFloorDistance(SithThing* pThing, const rdVector3* moveNorm);
int J3DAPI sithCollision_ActorPlayerSearchFilter(SithThing* pThing1, SithThing* pThing2);

// Helper hooking functions
void sithCollision_InstallHooks(void);
void sithCollision_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCOLLISION_H
