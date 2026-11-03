#ifndef SITH_SITHACTOR_H
#define SITH_SITHACTOR_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>

#include <std/types.h>
#include <std/General/stdConffile.h>

J3D_EXTERN_C_START

#define SITHACTOR_MAX_UNDERWATER_MSEC 60000
#define SITHACTOR_MAX_RAFT_DAMAGE     100000


void J3DAPI sithActor_SetDifficulty(SithThing* pActor);
void J3DAPI sithActor_Update(SithThing* pThing, unsigned int msecDeltaTime);

float J3DAPI sithActor_DamageActor(SithThing* pActor, SithThing* pThing, float damage, SithDamageType damageType);
float J3DAPI sithActor_DamageRaftActor(SithThing* pActor, SithThing* pPerpetrator, float damage, SithDamageType damageType);
void J3DAPI sithActor_KillActor(SithThing* pThing, SithThing* pSrcThing, SithDamageType damageType);

int J3DAPI sithActor_SurfaceCollisionHandler(SithThing* pThing, SithSurface* pSurface, SithCollision* pHitStack);
int J3DAPI sithActor_ActorCollisionHandler(SithThing* pSrcThing, SithThing* pThing, SithCollision* pCollision, int a4);

void J3DAPI sithActor_SetHeadPYR(SithThing* pThing, const rdVector3* headAngles);
void J3DAPI sithActor_RotateHead(SithThing* pThing, const rdVector3* headAngles, float secDeltaTime); // Added, new
void J3DAPI sithActor_UpdateAimJoints(SithThing* pThing); // Added

void J3DAPI sithActor_DestroyActor(SithThing* pActor);
void J3DAPI sithActor_DestroyCorpse(SithThing* pThing);
int J3DAPI sithActor_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum);

static inline void sithActor_ResetStateChange(SithThing* pActor) // Added
{
    if ( pActor->type == SITH_THING_ACTOR || pActor->type == SITH_THING_PLAYER ) // Just make sure we have actor
    {
        pActor->thingInfo.actorInfo.stateChange.type                 = SITHACTORSTATECHANGE_NONE;
        pActor->thingInfo.actorInfo.stateChange.params.moveFlags = (SithActorSpecialMoveFlags)-1;
    }
}

// Helper hooking functions
void sithActor_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHACTOR_H
