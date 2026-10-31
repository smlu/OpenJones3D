// Cases of the sithAIMove vectors, shared by the capture harness and sithAIMoveVecTests. See
// tv/sithAIMove/README.md.
//
// A case builds a small AI world (an AI actor with its control block, a target, a goal thing, sectors, a puppet class
// with a chosen set of keyframes and, for the Quetzalcoatl functions, a user block) and calls one sithAIMove function
// (or a script of calls, "frames") with a list of arguments. After every call a checkpoint records the engine calls
// the stubs saw (sithAI_EmitEvent, the puppet, the sound class, the collision move, ...), the results of the call and
// every byte that changed in the arena.
//
// Replaced by stubs (sithAIMoveVecSeams.def): everything outside sithAIMove.c that has side effects or tables, and the
// non-inline math helpers (the stubs follow the helpers' structure but compute on their own).
#ifndef SITH_AI_TESTS_SITHAIMOVEVEC_H
#define SITH_AI_TESTS_SITHAIMOVEVEC_H

#include "sithVecWorld.h"

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithSurface.h>

#include <math.h>

// Defined in sithAIMove.c but not declared in its header
void J3DAPI sithAIMove_AIFinalizeSpecialMove(SithAIControlBlock* pLocal, SithActorSpecialMoveFlags moveFlags);

// ---------------------------------------------------------------------------------------------------------------
// Engine calls recorded by the stubs
// ---------------------------------------------------------------------------------------------------------------
enum
{
    SITHAIMOVEVEC_EV_RESULT = SITHVEC_EV_SUITE, // results of the call of a case: return value, outputs
    SITHAIMOVEVEC_EV_CHECKWPNTMOVEALIGN,
    SITHAIMOVEVEC_EV_SWEEPMOVETOPATHPOS,
    SITHAIMOVEVEC_EV_ADVANCETONEXTWPNT,
    SITHAIMOVEVEC_EV_CHECKPOSITION,
    SITHAIMOVEVEC_EV_CHECKPATHTOPOINT,
    SITHAIMOVEVEC_EV_APPLYFORCE,
    SITHAIMOVEVEC_EV_AISETMODE,
    SITHAIMOVEVEC_EV_AICLEARMODE,
    SITHAIMOVEVEC_EV_EMITEVENT,
    SITHAIMOVEVEC_EV_HASINSTINCT,
    SITHAIMOVEVEC_EV_HASWEAPONSELECTED,
    SITHAIMOVEVEC_EV_ISAIMING,
    SITHAIMOVEVEC_EV_PROCESSWEAPONAIM,
    SITHAIMOVEVEC_EV_JUMP,
    SITHAIMOVEVEC_EV_MESHINDEX,
    SITHAIMOVEVEC_EV_JOINTINDEX,
    SITHAIMOVEVEC_EV_DETACHTHING,
    SITHAIMOVEVEC_EV_CREATEQUETZBLOCK,
    SITHAIMOVEVEC_EV_PLAYMODE,
    SITHAIMOVEVEC_EV_CLEARMODE,
    SITHAIMOVEVEC_EV_FINDACTIVETRACK,
    SITHAIMOVEVEC_EV_SYNCHMODE,
    SITHAIMOVEVEC_EV_PUPPETSETSUBMODE,
    SITHAIMOVEVEC_EV_DEFAULTCALLBACK,
    SITHAIMOVEVEC_EV_FREETRACK,
    SITHAIMOVEVEC_EV_SOUNDFIRST,
    SITHAIMOVEVEC_EV_SOUNDRANDOM,
    SITHAIMOVEVEC_EV_SOUNDSTOP,
    SITHAIMOVEVEC_EV_SENDMESSAGE,
    SITHAIMOVEVEC_EV_SETHEADPYR,
    SITHAIMOVEVEC_EV_FINDFLOOR,
};

// ---------------------------------------------------------------------------------------------------------------
// Case description
// ---------------------------------------------------------------------------------------------------------------
#define SITHAIMOVEVEC_MAXKF    12u
#define SITHAIMOVEVEC_MAXCALLS 24u
#define SITHAIMOVEVEC_MAXSEQ   6u

// Things of the world
#define SITHAIMOVEVEC_OWNER  0u // the AI actor
#define SITHAIMOVEVEC_TARGET 1u // its target thing (SithAIControlBlock::pTargetThing)
#define SITHAIMOVEVEC_GOAL   2u // goalThing
#define SITHAIMOVEVEC_TAIL   3u // thing attached to the owner (tail segment)
#define SITHAIMOVEVEC_OTHER  4u // a bystander

// Function selected by a case (SithVecCase::var)
typedef enum
{
    SITHAIMOVEVEC_UPDATE,
    SITHAIMOVEVEC_UPDATEMINECAR,
    SITHAIMOVEVEC_UPDATETURN,
    SITHAIMOVEVEC_UPDATEMOVE,
    SITHAIMOVEVEC_CHECKMOVEALIGN,
    SITHAIMOVEVEC_GETGOALTOLERANCE,
    SITHAIMOVEVEC_CHECKWALKMOVE,
    SITHAIMOVEVEC_CHECKFLYMOVE,
    SITHAIMOVEVEC_CHECKSWIMMOVE,
    SITHAIMOVEVEC_GETMOVESTATE,
    SITHAIMOVEVEC_SPECIALTURN,
    SITHAIMOVEVEC_UPDATEAIMOVE,
    SITHAIMOVEVEC_GETMOVEMODES,
    SITHAIMOVEVEC_SETSUBMODE,
    SITHAIMOVEVEC_PUPPETCALLBACK,
    SITHAIMOVEVEC_SETLOOKTHING,
    SITHAIMOVEVEC_SETLOOKPOS,
    SITHAIMOVEVEC_SETLOOKPOSEYELEVEL,
    SITHAIMOVEVEC_SETMOVEPOS,
    SITHAIMOVEVEC_SETMOVETARGETPOS,
    SITHAIMOVEVEC_SPECIALMOVE,
    SITHAIMOVEVEC_FINALIZESPECIALMOVE,
    SITHAIMOVEVEC_JUMP,
    SITHAIMOVEVEC_STOP,
    SITHAIMOVEVEC_SETGOALREACHED,
    SITHAIMOVEVEC_UNREACHABLE,
    SITHAIMOVEVEC_STOPAIMOVEMENT,
    SITHAIMOVEVEC_RESETAILOOK,
    SITHAIMOVEVEC_UPDATEBOSS,
    SITHAIMOVEVEC_UPDATEHEADTRACKTARGET,
    SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS,
    SITHAIMOVEVEC_GETQUETZSTRIKEANGLES,
    SITHAIMOVEVEC_GETQUETZANIM2ANGLES,
    SITHAIMOVEVEC_UPDATEQUETZSTRIKE,
    SITHAIMOVEVEC_UPDATEQUETZANIM2,
    SITHAIMOVEVEC_UPDATEQUETZANIM3,
    SITHAIMOVEVEC_UPDATEQUETZANIM5,
    SITHAIMOVEVEC_UPDATEQUETZANIM4,
    SITHAIMOVEVEC_UPDATEQUETZTAIL,
    SITHAIMOVEVEC_STARTQUETZSTRIKE, // Start*, then the Update* of the same animation for every call
    SITHAIMOVEVEC_STARTQUETZANIM2,
    SITHAIMOVEVEC_STARTQUETZANIM3,
    SITHAIMOVEVEC_STARTQUETZANIM5,
    SITHAIMOVEVEC_STARTQUETZANIM4,
    SITHAIMOVEVEC_UPDATEMARDUKTAIL,
} SithAIMoveVecFunc;

// One call of the function of a case. Not every field is used by every function.
typedef struct
{
    float f;               // secDeltaTime, angle, moveSpeed or move speed of the function
    float g;               // second float argument
    int i;                 // integer argument: flags, move status, thing number ...
    int j;                 // second integer argument
    rdVector3 v;           // vector argument (position)
    rdVector3 w;           // second vector argument
    uint32_t addMsec;      // added to sithTime_g_msecGameTime before the call
    int newMoveStatus;     // sets the owner's moveStatus before the call when nonzero
    uint32_t xorMode;      // toggled in SithAIControlBlock::mode before the call
    uint32_t xorSubmode;   // toggled in SithAIControlBlock::submode before the call
    int newPuppetSubmode;  // sets the puppet state's submode before the call when nonzero
} SithAIMoveVecCall;

typedef struct
{
    // --- AI control block
    uint32_t mode, submode;
    rdVector3 goalLVec, lookPos, movePos, savedMovePos, homePos, toTarget;
    float moveSpeed, moveDistance, maxHomeDist;
    uint32_t msecPauseMoveUntil;
    uint8_t target; // pTargetThing = the target thing
    uint8_t goal;   // goalThing = the goal thing

    // --- Owner. Zero values of eyeZ, maxThrust, maxRot, maxHeadVel, maxHeadYaw, movesize, minPitch and maxPitch select
    // the defaults 1/16, 2, 90, 180, 70, 1/8, -30 and 40; zero health 100; heading (0,1,0) and up (0,0,1) when all of
    // their components are 0.
    uint32_t actorFlags, thingFlags, physFlags;
    int moveStatus;
    float health;
    uint8_t zeroHealth;
    int8_t hx, hy, hz, ux, uy, uz;
    rdVector3 pos, velocity, headPYR;
    uint8_t zeroHeadYaw;
    uint8_t zeroTweak; // joint angles (renderData.apTweakedAngles) are zero instead of random
    float eyeZ, maxThrust, maxRot, maxHeadVel, maxHeadYaw, minPitch, maxPitch, movesize;
    uint32_t attachFlags;     // SithAttach::flags
    uint8_t attachSurface;    // attach to a wall surface with the normal below
    rdVector3 surfaceNormal;
    uint8_t tail;             // 1: the tail thing is attached to the owner with SITH_ATTACH_TAIL, 2: attached without the flag
    const char* pName;        // default "ai"
    uint8_t sector;           // sector number of the owner (0..2)
    uint8_t type;             // thing type of the owner when nonzero
    uint8_t noLocal;          // controlInfo.aiControl.pLocal = NULL
    uint8_t noSector;         // pInSector = NULL
    uint8_t notPhysics;       // moveType SITH_MT_NONE instead of SITH_MT_PHYSICS
    uint8_t controlsDisabled, forceMovePlay; // actor info flags of the same names

    // --- Puppet
    uint8_t majorMode, numKf;
    uint8_t aKf[SITHAIMOVEVEC_MAXKF]; // submodes that have a keyframe in the owner's puppet class
    uint8_t puppetSubmode;            // SithPuppetState::submode
    uint8_t noPuppetClass, noPuppetState;
    uint8_t trackSubmode;             // submode of the track sithPuppet_FindActiveTrack returns

    // --- Target and goal things
    uint8_t targetType;     // thing type when nonzero (default actor)
    uint32_t targetActorFlags, targetPhysFlags;
    rdVector3 targetPos, targetEye;
    int8_t tx, ty, tz;      // target heading; all 0: (0,1,0)
    rdVector3 goalVelocity; // velocity of the goal thing

    // --- Sectors
    uint32_t sec0Flags, sec1Flags, sec2Flags;

    // --- Engine state
    uint32_t msecGameTime, frameNumber, debugModeFlags;

    // --- Stub behavior
    uint8_t hasWeapon, isAiming, hasInstinct;
    int meshIdx, jointIdx;
    int sweepRet, advanceRet, playModeRet, findTrack;
    uint8_t numCheckPos;
    int8_t aCheckPos[4];  // sithAIUtil_CheckPosition results by call number (the last one repeats); none: OK
    int8_t aCheckPath[4]; // sithAIUtil_CheckPathToPoint results by call number; none: 0
    int findSectorMode;   // sithVec_stubFindSectorMode
    float moveScale;      // sithVec_stubMoveScale (0: 1)
    int randSeed;         // sithVec_stubRandSeed when nonzero

    // --- Quetz user block
    uint8_t quetz;        // the owner has a block
    uint8_t quetzPreset;  // random strike state: mode, snapshot, current time and snapshots
    uint8_t strikeMode, strikeSnap, strikeNum;
    float strikeTime, snapStep;

    // --- Calls
    uint8_t numCalls;
    const SithAIMoveVecCall* aCalls;
} SithAIMoveVecParams;

// ---------------------------------------------------------------------------------------------------------------
// Synthetic state outside the arena that is part of the case (registered as region 1: addresses into it are
// normalized in the records)
// ---------------------------------------------------------------------------------------------------------------
typedef struct
{
    SithPuppetClass puppetClass;
    uint8_t aKeyframes[SITHAIMOVEVEC_MAXKF][16];
    SithPuppetTrack track;
} SithAIMoveVecPuppet;

static SithAIMoveVecPuppet sithAIMoveVec_puppet;

// Lives in SithVecArena::user
typedef struct
{
    rdVector3 aAngles[32];  // renderData.apTweakedAngles
    rdMatrix34 aJoints[32]; // renderData.paJointMatrices
    SithSurface surface;    // the wall surface of the owner (attach.attachedToStructure)
} SithAIMoveVecUser;

static_assert(sizeof(SithAIMoveVecUser) <= SITHVEC_USERSIZE, "SithAIMoveVecUser does not fit the arena");
static_assert(sizeof(SithQuetzUserBlock) <= SITHVEC_POOLSIZE / 2, "SithQuetzUserBlock does not fit the arena");

#define SITHAIMOVEVEC_USER ((SithAIMoveVecUser*)sithVec_arena.user)

// Stub configuration of the current run
static struct
{
    int hasWeapon, isAiming, hasInstinct;
    int meshIdx, jointIdx;
    int sweepRet, advanceRet, playModeRet, findTrack;
    unsigned int numCheckPos;
    int aCheckPos[4];
    int aCheckPath[4];
    unsigned int numCheckPosCalls, numCheckPathCalls;
} sithAIMoveVec_cfg;

// ---------------------------------------------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------------------------------------------
#define SITHAIMOVEVEC_F(f) sithVec_FloatBits(f)

SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_CheckWpntMoveAlign(SithAIControlBlock* pLocal)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_CHECKWPNTMOVEALIGN, 1, SITHVEC_P(pLocal));
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_SweepMoveToPathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float angle, float maxAngle, float deltaAngle, SithAIUtilMkPointFlags flags)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_SWEEPMOVETOPATHPOS, 5, SITHVEC_P(pLocal), SITHAIMOVEVEC_F(angle), SITHAIMOVEVEC_F(maxAngle), SITHAIMOVEVEC_F(deltaAngle), (uint32_t)flags);
    sithVec_EvWords(pPos, sizeof(*pPos));
    return sithAIMoveVec_cfg.sweepRet;
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_AIAdvanceToNextWpnt(SithAIControlBlock* pLocal, int bNotify)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_ADVANCETONEXTWPNT, 2, SITHVEC_P(pLocal), (uint32_t)bNotify);
    return sithAIMoveVec_cfg.advanceRet;
}

// Heading vectors: the orientation's look vector (XYZ) or its horizontal part, normalized (XY)
SITHVEC_STUB signed int J3DAPI sithVec_sithAIUtil_GetXYZHeadingVector(const SithThing* pThing, rdVector3* pDest)
{
    *pDest = pThing->orient.lvec;
    return 1;
}

SITHVEC_STUB signed int J3DAPI sithVec_sithAIUtil_GetXYHeadingVector(const SithThing* pThing, rdVector3* pDest)
{
    pDest->x = pThing->orient.lvec.x;
    pDest->y = pThing->orient.lvec.y;
    pDest->z = 0.0f;

    float len = sqrtf(pDest->x * pDest->x + pDest->y * pDest->y);
    if ( len != 0.0f )
    {
        pDest->x /= len;
        pDest->y /= len;
    }

    return 1;
}

SITHVEC_STUB SithAIUtilFloorCheck J3DAPI sithVec_sithAIUtil_CheckPosition(const SithAIControlBlock* pLocal, rdVector3* endPos, int* pbNoCollision)
{
    J3D_UNUSED(pbNoCollision);
    sithVec_Ev(SITHAIMOVEVEC_EV_CHECKPOSITION, 1, SITHVEC_P(pLocal));
    sithVec_EvWords(endPos, sizeof(*endPos));

    unsigned int n = sithAIMoveVec_cfg.numCheckPosCalls++;
    if ( sithAIMoveVec_cfg.numCheckPos == 0 )
    {
        return SITHAIUTIL_FLOOR_OK;
    }

    return (SithAIUtilFloorCheck)sithAIMoveVec_cfg.aCheckPos[n < sithAIMoveVec_cfg.numCheckPos ? n : sithAIMoveVec_cfg.numCheckPos - 1u];
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_CheckPathToPoint(SithThing* pViewer, const rdVector3* pTarget, float radius, float* pDistance, rdVector3* pHitNorm, int bDetectThings, int bSkipFloor)
{
    J3D_UNUSED(pHitNorm);
    sithVec_Ev(SITHAIMOVEVEC_EV_CHECKPATHTOPOINT, 5, SITHVEC_P(pViewer), SITHAIMOVEVEC_F(radius), SITHAIMOVEVEC_F(*pDistance), (uint32_t)bDetectThings, (uint32_t)bSkipFloor);
    sithVec_EvWords(pTarget, sizeof(*pTarget));

    unsigned int n = sithAIMoveVec_cfg.numCheckPathCalls++;
    return sithAIMoveVec_cfg.aCheckPath[n < SITHVEC_ARRAYLEN(sithAIMoveVec_cfg.aCheckPath) ? n : SITHVEC_ARRAYLEN(sithAIMoveVec_cfg.aCheckPath) - 1u];
}

SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_ApplyForce(SithAIControlBlock* pLocal, const rdVector3* pDirection, float force)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_APPLYFORCE, 2, SITHVEC_P(pLocal), SITHAIMOVEVEC_F(force));
    sithVec_EvWords(pDirection, sizeof(*pDirection));
}

SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_AISetMode(SithAIControlBlock* pLocal, SithAIMode mode)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_AISETMODE, 2, SITHVEC_P(pLocal), (uint32_t)mode);
    pLocal->mode |= mode;
}

SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_AIClearMode(SithAIControlBlock* pLocal, SithAIMode mode)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_AICLEARMODE, 2, SITHVEC_P(pLocal), (uint32_t)mode);
    pLocal->mode &= ~mode;
}

SITHVEC_STUB void J3DAPI sithVec_sithAI_EmitEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_EMITEVENT, 3, SITHVEC_P(pLocal), (uint32_t)event, SITHVEC_P(pObject));
}

SITHVEC_STUB int J3DAPI sithVec_sithAI_HasInstinct(const SithAIControlBlock* pLocal, const char* pName)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_HASINSTINCT, 2, SITHVEC_P(pLocal), sithVec_Hash(pName));
    return sithAIMoveVec_cfg.hasInstinct;
}

SITHVEC_STUB int J3DAPI sithVec_sithWeapon_HasWeaponSelected(const SithThing* pThing)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_HASWEAPONSELECTED, 1, SITHVEC_P(pThing));
    return sithAIMoveVec_cfg.hasWeapon;
}

SITHVEC_STUB int J3DAPI sithVec_sithWeapon_IsAiming(SithThing* pThing)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_ISAIMING, 1, SITHVEC_P(pThing));
    return sithAIMoveVec_cfg.isAiming;
}

SITHVEC_STUB void J3DAPI sithVec_sithPlayerControls_ProcessWeaponAim(SithThing* pThing, float secDeltaTime)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_PROCESSWEAPONAIM, 2, SITHVEC_P(pThing), SITHAIMOVEVEC_F(secDeltaTime));
}

SITHVEC_STUB void J3DAPI sithVec_sithPlayerActions_Jump(SithThing* pThing, float speedFactor, int jumpDirection)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_JUMP, 3, SITHVEC_P(pThing), SITHAIMOVEVEC_F(speedFactor), (uint32_t)jumpDirection);
}

SITHVEC_STUB int J3DAPI sithVec_sithThing_GetThingMeshIndex(const SithThing* pThing, const char* meshName)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_MESHINDEX, 2, SITHVEC_P(pThing), sithVec_Hash(meshName));
    return sithAIMoveVec_cfg.meshIdx;
}

SITHVEC_STUB int J3DAPI sithVec_sithThing_GetThingJointIndex(const SithThing* pThing, const char* pJointName)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_JOINTINDEX, 2, SITHVEC_P(pThing), sithVec_Hash(pJointName));
    return sithAIMoveVec_cfg.jointIdx;
}

SITHVEC_STUB void J3DAPI sithVec_sithThing_DetachThing(SithThing* pThing)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_DETACHTHING, 1, SITHVEC_P(pThing));
    pThing->attach.flags = 0;
}

// The block exists or not (the real function allocates it)
SITHVEC_STUB int J3DAPI sithVec_sithThing_CreateQuetzUserBlock(SithThing* pThing)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_CREATEQUETZBLOCK, 1, SITHVEC_P(pThing));
    return pThing->userblock.pQuetz != NULL;
}

SITHVEC_STUB int J3DAPI sithVec_sithPuppet_PlayMode(SithThing* pThing, SithPuppetSubMode submode, rdPuppetTrackCallback pfCallback)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_PLAYMODE, 3, SITHVEC_P(pThing), (uint32_t)submode, (uint32_t)(uintptr_t)pfCallback);
    return sithAIMoveVec_cfg.playModeRet;
}

SITHVEC_STUB void J3DAPI sithVec_sithPuppet_ClearMode(SithThing* pThing, SithPuppetSubMode mode)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_CLEARMODE, 2, SITHVEC_P(pThing), (uint32_t)mode);
}

SITHVEC_STUB SithPuppetTrack* J3DAPI sithVec_sithPuppet_FindActiveTrack(const SithThing* pThing, const rdKeyframe* pKfTrack)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_FINDACTIVETRACK, 2, SITHVEC_P(pThing), (uint32_t)(uintptr_t)pKfTrack);
    return sithAIMoveVec_cfg.findTrack ? &sithAIMoveVec_puppet.track : NULL;
}

SITHVEC_STUB int J3DAPI sithVec_sithPuppet_SynchMode(SithThing* pThing, SithPuppetSubMode oldMode, SithPuppetSubMode newMode, float a4, int bReverse)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_SYNCHMODE, 5, SITHVEC_P(pThing), (uint32_t)oldMode, (uint32_t)newMode, SITHAIMOVEVEC_F(a4), (uint32_t)bReverse);
    return 0;
}

SITHVEC_STUB int J3DAPI sithVec_sithPuppet_SetSubMode(SithThing* pThing, SithPuppetSubMode newSubMode, rdPuppetTrackCallback pfCallback)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_PUPPETSETSUBMODE, 3, SITHVEC_P(pThing), (uint32_t)newSubMode, (uint32_t)(uintptr_t)pfCallback);
    return 0;
}

SITHVEC_STUB void J3DAPI sithVec_sithPuppet_DefaultCallback(SithThing* pThing, int track, rdKeyMarkerType markerType)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_DEFAULTCALLBACK, 3, SITHVEC_P(pThing), (uint32_t)track, (uint32_t)markerType);
}

SITHVEC_STUB void J3DAPI sithVec_sithPuppet_FreeTrackByIndex(SithThing* pThing, int trackNum)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_FREETRACK, 2, SITHVEC_P(pThing), (uint32_t)trackNum);
}

SITHVEC_STUB tSoundChannelHandle J3DAPI sithVec_sithSoundClass_PlayModeFirst(SithThing* pThing, SithSoundClassMode mode)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_SOUNDFIRST, 2, SITHVEC_P(pThing), (uint32_t)mode);
    return 0;
}

SITHVEC_STUB tSoundChannelHandle J3DAPI sithVec_sithSoundClass_PlayModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_SOUNDRANDOM, 2, SITHVEC_P(pThing), (uint32_t)mode);
    return 0;
}

SITHVEC_STUB void J3DAPI sithVec_sithSoundClass_StopMode(SithThing* pThing, SithSoundClassMode mode)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_SOUNDSTOP, 2, SITHVEC_P(pThing), (uint32_t)mode);
}

SITHVEC_STUB int J3DAPI sithVec_sithCog_ThingSendMessage(const SithThing* pThing, const SithThing* pSrcThing, SithCogMsgType msgType)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_SENDMESSAGE, 3, SITHVEC_P(pThing), SITHVEC_P(pSrcThing), (uint32_t)msgType);
    return 0;
}

SITHVEC_STUB void J3DAPI sithVec_sithActor_SetHeadPYR(SithThing* pThing, const rdVector3* headAngles)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_SETHEADPYR, 1, SITHVEC_P(pThing));
    sithVec_EvWords(headAngles, sizeof(*headAngles));
    pThing->thingInfo.actorInfo.headPYR = *headAngles;
}

SITHVEC_STUB void J3DAPI sithVec_sithPhysics_FindFloor(SithThing* pThing, int bNoThingStateUpdate)
{
    sithVec_Ev(SITHAIMOVEVEC_EV_FINDFLOOR, 2, SITHVEC_P(pThing), (uint32_t)bNoThingStateUpdate);
}

// Math helpers (see sithVecWorld.h for the shared ones)
SITHVEC_STUB void J3DAPI sithVec_rdMatrix_TransformVector34Acc(rdVector3* dest, const rdMatrix34* mat)
{
    rdVector3 src = *dest;
    sithVec_rdMatrix_TransformVector34(dest, &src, mat);
}

// The inverse (transposed) transform
SITHVEC_STUB void J3DAPI sithVec_rdMatrix_TransformVectorOrtho34(rdVector3* dest, const rdVector3* src, const rdMatrix34* mat)
{
    rdVector3 s = *src;
    dest->x = s.x * mat->rvec.x + s.y * mat->rvec.y + s.z * mat->rvec.z;
    dest->y = s.x * mat->lvec.x + s.y * mat->lvec.y + s.z * mat->lvec.z;
    dest->z = s.x * mat->uvec.x + s.y * mat->uvec.y + s.z * mat->uvec.z;
}

SITHVEC_STUB void J3DAPI sithVec_rdMatrix_Normalize34(rdMatrix34* mat)
{
    sithVec_rdVector_Normalize3Acc(&mat->rvec);
    sithVec_rdVector_Normalize3Acc(&mat->lvec);
    sithVec_rdVector_Normalize3Acc(&mat->uvec);
}

SITHVEC_STUB void J3DAPI sithVec_rdVector_Rotate3Acc(rdVector3* vec, const rdVector3* pyr)
{
    rdMatrix34 m = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } };
    sithVec_rdMatrix_PreRotate34(&m, pyr);

    rdVector3 src = *vec;
    sithVec_rdMatrix_TransformVector34(vec, &src, &m);
}

SITHVEC_STUB void J3DAPI sithVec_rdVector_Rotate3(rdVector3* vec, const rdVector3* pivot, const rdVector3* pyr)
{
    *vec = *pivot;
    sithVec_rdVector_Rotate3Acc(vec, pyr);
}

SITHVEC_STUB float J3DAPI sithVec_stdMath_ArcSin3(float angle)
{
    return sithVec_AsinDeg(angle < -1.0f ? -1.0f : (angle > 1.0f ? 1.0f : angle));
}

SITHVEC_STUB float J3DAPI sithVec_stdMath_NormalizeAngle(float angle)
{
    return angle - 360.0f * floorf((angle + 180.0f) / 360.0f);
}

SITHVEC_STUB float J3DAPI sithVec_stdMath_Dist2D1(float x, float y)
{
    return sqrtf(x * x + y * y);
}

// ---------------------------------------------------------------------------------------------------------------
// Builder
// ---------------------------------------------------------------------------------------------------------------
static void sithAIMoveVec_SetOrient(rdMatrix34* pOrient, int lx, int ly, int lz, int ux, int uy, int uz)
{
    float ll = sqrtf((float)(lx * lx + ly * ly + lz * lz));
    pOrient->lvec.x = (float)lx / ll;
    pOrient->lvec.y = (float)ly / ll;
    pOrient->lvec.z = (float)lz / ll;

    float ul = sqrtf((float)(ux * ux + uy * uy + uz * uz));
    rdVector3 up = { (float)ux / ul, (float)uy / ul, (float)uz / ul };

    rdVector3 r;
    sithVec_Cross3(&r, &pOrient->lvec, &up);
    float rl = sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);
    pOrient->rvec.x = r.x / rl;
    pOrient->rvec.y = r.y / rl;
    pOrient->rvec.z = r.z / rl;
    sithVec_Cross3(&pOrient->uvec, &pOrient->rvec, &pOrient->lvec);
    pOrient->dvec.x = pOrient->dvec.y = pOrient->dvec.z = 0.0f;
}

static float sithAIMoveVec_Def(float v, float def)
{
    return v != 0.0f ? v : def;
}

static void sithAIMoveVec_Build(const SithVecCase* pCase)
{
    const SithAIMoveVecParams* p = (const SithAIMoveVecParams*)pCase->pParams;
    SithVecArena* a              = &sithVec_arena;

    memset(&sithAIMoveVec_cfg, 0, sizeof(sithAIMoveVec_cfg));
    sithAIMoveVec_cfg.hasWeapon   = p->hasWeapon;
    sithAIMoveVec_cfg.isAiming    = p->isAiming;
    sithAIMoveVec_cfg.hasInstinct = p->hasInstinct;
    sithAIMoveVec_cfg.meshIdx     = p->meshIdx;
    sithAIMoveVec_cfg.jointIdx    = p->jointIdx;
    sithAIMoveVec_cfg.sweepRet    = p->sweepRet;
    sithAIMoveVec_cfg.advanceRet  = p->advanceRet;
    sithAIMoveVec_cfg.playModeRet = p->playModeRet;
    sithAIMoveVec_cfg.findTrack   = p->findTrack;
    sithAIMoveVec_cfg.numCheckPos = p->numCheckPos;
    for ( unsigned int i = 0; i < SITHVEC_ARRAYLEN(p->aCheckPos); ++i )
    {
        sithAIMoveVec_cfg.aCheckPos[i]  = p->aCheckPos[i];
        sithAIMoveVec_cfg.aCheckPath[i] = p->aCheckPath[i];
    }

    sithVec_stubFindSectorMode = p->findSectorMode;
    sithVec_stubMoveScale      = p->moveScale != 0.0f ? p->moveScale : 1.0f;
    if ( p->randSeed )
    {
        sithVec_stubRandSeed = (uint32_t)p->randSeed;
    }

    sithTime_g_msecGameTime                 = p->msecGameTime;
    sithMain_g_frameNumber                  = p->frameNumber;
    sithMain_g_sith_mode.debugModeFlags     = p->debugModeFlags;

    memset(&sithAIMoveVec_puppet, 0, sizeof(sithAIMoveVec_puppet));
    sithVec_AddRegion(&sithAIMoveVec_puppet, sizeof(sithAIMoveVec_puppet));
    SITHVEC_ADDFUNC(sithAIMove_PuppetCallback);

    a->sectors[0].flags = p->sec0Flags;
    a->sectors[1].flags = p->sec1Flags;
    a->sectors[2].flags = p->sec2Flags;

    // User area: joint angles, joint matrices, wall surface
    SithAIMoveVecUser* pUser = SITHAIMOVEVEC_USER;
    memset(pUser, 0, sizeof(*pUser));
    for ( unsigned int i = 0; i < 32; ++i )
    {
        pUser->aAngles[i].x = sithVec_RndF() / 4.0f;
        pUser->aAngles[i].y = sithVec_RndF() / 4.0f;
        pUser->aAngles[i].z = sithVec_RndF() / 4.0f;
        if ( p->zeroTweak )
        {
            pUser->aAngles[i].x = pUser->aAngles[i].y = pUser->aAngles[i].z = 0.0f;
        }

        pUser->aJoints[i].rvec.x = 1.0f;
        pUser->aJoints[i].lvec.y = 1.0f;
        pUser->aJoints[i].uvec.z = 1.0f;
        pUser->aJoints[i].dvec.x = p->pos.x;
        pUser->aJoints[i].dvec.y = p->pos.y;
        pUser->aJoints[i].dvec.z = p->pos.z + 0.125f + (float)i / 64.0f; // joint positions in world space, near the owner
    }

    pUser->surface.face.normal = p->surfaceNormal;

    // Things
    SithThing* pOwner = sithVec_NewThing(SITHAIMOVEVEC_OWNER, p->type ? (SithThingType)p->type : SITH_THING_ACTOR, p->notPhysics ? SITH_MT_NONE : SITH_MT_PHYSICS, p->sector);
    if ( p->noSector )
    {
        pOwner->pInSector = NULL;
    }

    pOwner->controlType = SITH_CT_AI;
    pOwner->flags       = (SithThingFlag)p->thingFlags;
    pOwner->moveStatus  = (SithThingMoveStatus)p->moveStatus;
    pOwner->pos         = p->pos;
    snprintf(pOwner->aName, sizeof(pOwner->aName), "%s", p->pName ? p->pName : "ai");

    if ( p->hx || p->hy || p->hz || p->ux || p->uy || p->uz )
    {
        sithAIMoveVec_SetOrient(&pOwner->orient, p->hx, p->hy, p->hz, p->ux || p->uy || p->uz ? p->ux : 0, p->ux || p->uy || p->uz ? p->uy : 0, p->ux || p->uy || p->uz ? p->uz : 1);
    }

    SithActorInfo* pActor = &pOwner->thingInfo.actorInfo;
    pActor->flags         = (SithActorFlag)p->actorFlags;
    pActor->bControlsDisabled = p->controlsDisabled;
    pActor->bForceMovePlay    = p->forceMovePlay;
    pActor->health        = p->zeroHealth ? 0.0f : sithAIMoveVec_Def(p->health, 100.0f);
    pActor->maxThrust     = sithAIMoveVec_Def(p->maxThrust, 2.0f);
    pActor->maxRotVelocity = sithAIMoveVec_Def(p->maxRot, 90.0f);
    pActor->eyeOffset.z   = sithAIMoveVec_Def(p->eyeZ, 0.0625f);
    pActor->headPYR       = p->headPYR;
    pActor->minHeadPitch  = sithAIMoveVec_Def(p->minPitch, -30.0f);
    pActor->maxHeadPitch  = sithAIMoveVec_Def(p->maxPitch, 40.0f);
    pActor->maxHeadYaw    = p->zeroHeadYaw ? 0.0f : sithAIMoveVec_Def(p->maxHeadYaw, 70.0f);
    pActor->maxHeadVelocity = sithAIMoveVec_Def(p->maxHeadVel, 180.0f);

    pOwner->collide.movesize                = sithAIMoveVec_Def(p->movesize, 0.125f);
    pOwner->moveInfo.physics.flags          = (SithPhysicsFlags)p->physFlags;
    pOwner->moveInfo.physics.velocity       = p->velocity;
    pOwner->renderData.apTweakedAngles      = pUser->aAngles;
    pOwner->renderData.paJointMatrices      = pUser->aJoints;

    pOwner->attach.flags = (SithAttachFlag)p->attachFlags;
    if ( p->attachSurface )
    {
        pOwner->attach.flags |= SITH_ATTACH_SURFACE;
        pOwner->attach.attachedToStructure.pSurfaceAttached = &pUser->surface;
    }

    // Target
    SithThing* pTarget = sithVec_NewThing(SITHAIMOVEVEC_TARGET, p->targetType ? (SithThingType)p->targetType : SITH_THING_ACTOR, SITH_MT_PHYSICS, 1);
    pTarget->pos       = p->targetPos;
    pTarget->thingInfo.actorInfo.flags        = (SithActorFlag)p->targetActorFlags;
    pTarget->thingInfo.actorInfo.eyeOffset    = p->targetEye;
    pTarget->moveInfo.physics.flags           = (SithPhysicsFlags)p->targetPhysFlags;
    if ( p->tx || p->ty || p->tz )
    {
        sithAIMoveVec_SetOrient(&pTarget->orient, p->tx, p->ty, p->tz, 0, 0, 1);
    }

    SithThing* pGoal = sithVec_NewThing(SITHAIMOVEVEC_GOAL, SITH_THING_ACTOR, SITH_MT_PHYSICS, 2);
    pGoal->moveInfo.physics.velocity = p->goalVelocity;

    SithThing* pTail = sithVec_NewThing(SITHAIMOVEVEC_TAIL, SITH_THING_ACTOR, SITH_MT_PHYSICS, p->sector);
    if ( p->tail )
    {
        pTail->attach.flags   = p->tail == 1 ? SITH_ATTACH_TAIL : SITH_ATTACH_THING;
        pOwner->pAttachedThing = pTail;
    }

    sithVec_NewThing(SITHAIMOVEVEC_OTHER, SITH_THING_ITEM, SITH_MT_NONE, 2);

    // Puppet
    SithAIMoveVecPuppet* pPup = &sithAIMoveVec_puppet;
    if ( !p->noPuppetClass )
    {
        pOwner->pPuppetClass = &pPup->puppetClass;
    }

    if ( !p->noPuppetState )
    {
        SithPuppetState* pState = &a->puppetStates[0];
        memset(pState, 0, sizeof(*pState));
        pState->majorMode = p->majorMode;
        pState->submode   = (SithPuppetSubMode)p->puppetSubmode;
        pOwner->pPuppetState = pState;
    }

    for ( unsigned int i = 0; i < p->numKf; ++i )
    {
        pPup->puppetClass.aModes[p->majorMode][p->aKf[i]].pKeyframe = (rdKeyframe*)pPup->aKeyframes[i];
    }

    pPup->track.submode = (SithPuppetSubMode)p->trackSubmode;

    // AI control block
    SithAIControlBlock* pLocal = &a->aiBlocks[0];
    memset(pLocal, 0, sizeof(*pLocal));
    pLocal->pOwner             = pOwner;
    pLocal->pClass             = &a->aiClasses[0];
    memset(&a->aiClasses[0], 0, sizeof(a->aiClasses[0]));
    pLocal->mode               = (SithAIMode)p->mode;
    pLocal->submode            = (SithAISubMode)p->submode;
    pLocal->goalLVec           = p->goalLVec;
    pLocal->lookPos            = p->lookPos;
    pLocal->movePos            = p->movePos;
    pLocal->savedMovePos       = p->savedMovePos;
    pLocal->homePos            = p->homePos;
    pLocal->toTarget           = p->toTarget;
    pLocal->moveSpeed          = p->moveSpeed;
    pLocal->moveDistance       = p->moveDistance;
    pLocal->maxHomeDist        = p->maxHomeDist;
    pLocal->msecPauseMoveUntil = p->msecPauseMoveUntil;
    pLocal->pTargetThing       = p->target ? pTarget : NULL;
    pLocal->goalThing          = p->goal ? pGoal : NULL;

    pOwner->controlInfo.aiControl.pClass = pLocal->pClass;
    pOwner->controlInfo.aiControl.pLocal = p->noLocal ? NULL : pLocal;

    // Quetz user block
    if ( p->quetz )
    {
        SithQuetzUserBlock* pBlock = (SithQuetzUserBlock*)a->pool;
        a->poolUsed                = (uint32_t)((sizeof(*pBlock) + 15u) & ~15u);
        memset(pBlock, 0, sizeof(*pBlock));

        if ( p->quetzPreset )
        {
            SithQuetzStrike* pStrike = &pBlock->strike;
            pStrike->mode            = (SithQuetzStrikeMode)p->strikeMode;
            pStrike->snapShot        = p->strikeSnap;
            pStrike->numSnapShots    = p->strikeNum;
            pStrike->curTime         = p->strikeTime;

            float step = sithAIMoveVec_Def(p->snapStep, 0.125f);
            for ( unsigned int i = 0; i < SITHVEC_ARRAYLEN(pStrike->aSnapShots); ++i )
            {
                SithQuetzSnapShot* pSnap = &pStrike->aSnapShots[i];
                pSnap->time      = step * (float)i;
                pSnap->neckPitch = sithVec_RndF() / 16.0f;
                pSnap->neckYaw   = sithVec_RndF() / 16.0f;
                pSnap->headPitch = sithVec_RndF() / 16.0f;
                pSnap->headYaw   = sithVec_RndF() / 16.0f;
                pSnap->jawPitch  = sithVec_RndF() / 16.0f;
            }
        }

        pOwner->userblock.pQuetz = pBlock;
    }
}

static void sithAIMoveVec_Watch(void)
{
}

// ---------------------------------------------------------------------------------------------------------------
// Running a case
// ---------------------------------------------------------------------------------------------------------------
static void sithAIMoveVec_Result(unsigned int numWords, ...)
{
    va_list va;
    va_start(va, numWords);
    sithVecB_U8(&sithVec_ev, SITHAIMOVEVEC_EV_RESULT);
    for ( unsigned int i = 0; i < numWords; ++i )
    {
        sithVecB_U32(&sithVec_ev, sithVec_NormWord(va_arg(va, uint32_t)));
    }

    va_end(va);
}

static void sithAIMoveVec_ResultVec(const rdVector3* pVec)
{
    sithVecB_U8(&sithVec_ev, SITHAIMOVEVEC_EV_RESULT);
    sithVec_EvWords(pVec, sizeof(*pVec));
}

// Applies the pre-call changes of a call
static void sithAIMoveVec_Pre(const SithAIMoveVecCall* pCall)
{
    SithAIControlBlock* pLocal = &sithVec_arena.aiBlocks[0];
    SithThing* pOwner          = &sithVec_arena.things[SITHAIMOVEVEC_OWNER];

    sithTime_g_msecGameTime += pCall->addMsec;
    pLocal->mode            ^= pCall->xorMode;
    pLocal->submode         ^= pCall->xorSubmode;
    if ( pCall->newMoveStatus )
    {
        pOwner->moveStatus = (SithThingMoveStatus)pCall->newMoveStatus;
    }

    if ( pCall->newPuppetSubmode && pOwner->pPuppetState )
    {
        pOwner->pPuppetState->submode = (SithPuppetSubMode)pCall->newPuppetSubmode;
    }
}

static int sithAIMoveVec_Run(SithVecCtx* pCtx)
{
    const SithAIMoveVecParams* p = (const SithAIMoveVecParams*)pCtx->pCase->pParams;
    SithAIControlBlock* pLocal   = &sithVec_arena.aiBlocks[0];
    SithThing* pOwner            = &sithVec_arena.things[SITHAIMOVEVEC_OWNER];
    sithVec_Begin(pCtx);

    for ( unsigned int c = 0; c < p->numCalls; ++c )
    {
        const SithAIMoveVecCall* pCall = &p->aCalls[c];
        sithAIMoveVec_Pre(pCall);
        ++sithMain_g_frameNumber;

        switch ( (SithAIMoveVecFunc)pCtx->pCase->var )
        {
            case SITHAIMOVEVEC_UPDATE:
                SITHVEC_FN(sithAIMove_Update)(pOwner, pCall->f);
                break;

            case SITHAIMOVEVEC_UPDATEMINECAR:
                SITHVEC_FN(sithAIMove_UpdateMineCar)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_UPDATETURN:
                SITHVEC_FN(sithAIMove_UpdateTurn)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_UPDATEMOVE:
                SITHVEC_FN(sithAIMove_UpdateMove)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_CHECKMOVEALIGN:
                SITHVEC_FN(sithAIMove_CheckMoveAlign)(pLocal);
                break;

            case SITHAIMOVEVEC_GETGOALTOLERANCE:
            {
                rdVector3 pos     = pCall->v;
                rdVector3 movePos = pCall->w;
                float tolerance   = SITHVEC_FN(sithAIMove_GetGoalTolerance)(pLocal, &pos, &movePos);
                sithAIMoveVec_Result(1, SITHAIMOVEVEC_F(tolerance));
                sithAIMoveVec_ResultVec(&pos);
                sithAIMoveVec_ResultVec(&movePos);
                break;
            }

            case SITHAIMOVEVEC_CHECKWALKMOVE:
                sithAIMoveVec_Result(1, (uint32_t)SITHVEC_FN(sithAIMove_CheckWalkMove)(pLocal, pCall->f));
                break;

            case SITHAIMOVEVEC_CHECKFLYMOVE:
                sithAIMoveVec_Result(1, (uint32_t)SITHVEC_FN(sithAIMove_CheckFlyMove)(pLocal, pCall->f));
                break;

            case SITHAIMOVEVEC_CHECKSWIMMOVE:
                sithAIMoveVec_Result(1, (uint32_t)SITHVEC_FN(sithAIMove_CheckSwimMove)(pLocal, pCall->f));
                break;

            case SITHAIMOVEVEC_GETMOVESTATE:
                // Every move status from i to j
                for ( int status = pCall->i; status <= pCall->j; ++status )
                {
                    pOwner->moveStatus = (SithThingMoveStatus)status;
                    sithAIMoveVec_Result(2, (uint32_t)status, (uint32_t)SITHVEC_FN(sithAIMove_AIGetMoveState)(pLocal));
                }

                break;

            case SITHAIMOVEVEC_SPECIALTURN:
                // Every angle starts from the move status of the case (a started turn changes it)
                pOwner->moveStatus = (SithThingMoveStatus)p->moveStatus;
                sithAIMoveVec_Result(1, (uint32_t)SITHVEC_FN(sithAIMove_AISpecialTurn)(pLocal, pCall->f));
                break;

            case SITHAIMOVEVEC_UPDATEAIMOVE:
                sithAIMoveVec_Result(1, SITHAIMOVEVEC_F(SITHVEC_FN(sithAIMove_UpdateAIMove)(pLocal)));
                break;

            case SITHAIMOVEVEC_GETMOVEMODES:
            {
                // Every speed of the table along every axis; the outputs start with a marker value
                static const float aSpeeds[] = {
                    0.0f, 0.0005f, -0.0005f, 0.001f, -0.001f, 0.00105f, 0.0011f, 0.00115f, 0.0012f, -0.0012f, 0.0049f, 0.005f, 0.0052f, 0.0055f,
                    0.00575f, 0.006f, -0.006f, 0.1f, -0.1f, 0.2699f, 0.27f, 0.2701f, 0.297f, 0.31f, 0.3105f, 0.32f, -0.3f, 1.0f
                };
                // The thresholds of the function (walk, run and turn speeds, scaled for the fast-move flags) and their float neighbors
                static const float aBases[] = { 0.001f, 0.001f * 1.1f, 0.001f * 1.15f, 0.27000001f, 0.27000001f * 1.1f, 0.27000001f * 1.15f, 0.0049999999f, 0.0049999999f * 1.1f, 0.0049999999f * 1.15f };
                static const SithThingMoveAxis aAxes[] = { SITHTHING_MOVEAXIS_X, SITHTHING_MOVEAXIS_Y, SITHTHING_MOVEAXIS_Z, SITHTHING_MOVEAXIS_NONE };
                for ( unsigned int s = 0; s < SITHVEC_ARRAYLEN(aSpeeds) + 6u * SITHVEC_ARRAYLEN(aBases); ++s )
                {
                    float speed = 0.0f;
                    if ( s < SITHVEC_ARRAYLEN(aSpeeds) )
                    {
                        speed = aSpeeds[s];
                    }
                    else
                    {
                        float base = aBases[(s - SITHVEC_ARRAYLEN(aSpeeds)) / 6u];
                        switch ( (s - SITHVEC_ARRAYLEN(aSpeeds)) % 6u )
                        {
                            case 0: speed = base; break;
                            case 1: speed = nextafterf(base, 1.0f); break;
                            case 2: speed = nextafterf(base, -1.0f); break;
                            case 3: speed = -base; break;
                            case 4: speed = -nextafterf(base, 1.0f); break;
                            default: speed = -nextafterf(base, -1.0f); break;
                        }
                    }

                    for ( unsigned int axis = 0; axis < SITHVEC_ARRAYLEN(aAxes); ++axis )
                    {
                        SithThingMoveStatus status = (SithThingMoveStatus)0x7F;
                        SithPuppetSubMode submode  = (SithPuppetSubMode)0x7F;
                        SITHVEC_FN(sithAIMove_GetAIMoveModes)(pOwner, speed, aAxes[axis], &status, &submode);
                        sithAIMoveVec_Result(2, (uint32_t)status, (uint32_t)submode);
                    }
                }

                break;
            }

            case SITHAIMOVEVEC_SETSUBMODE:
                // Every new submode, starting from the same puppet submode (newPuppetSubmode) and move status each time
                for ( int submode = 1; submode < (int)SITH_PUPPET_NUMSUBMODES; ++submode )
                {
                    if ( pOwner->pPuppetState )
                    {
                        pOwner->pPuppetState->submode = (SithPuppetSubMode)pCall->newPuppetSubmode;
                    }

                    pOwner->moveStatus = SITHPLAYERMOVE_STILL;
                    SITHVEC_FN(sithAIMove_SetSubMode)(pLocal, (SithThingMoveStatus)pCall->i, (SithPuppetSubMode)submode);
                    sithAIMoveVec_Result(3, (uint32_t)submode, pOwner->pPuppetState ? (uint32_t)pOwner->pPuppetState->submode : 0xFFu, (uint32_t)pOwner->moveStatus);
                }

                break;

            case SITHAIMOVEVEC_PUPPETCALLBACK:
                SITHVEC_FN(sithAIMove_PuppetCallback)(pOwner, pCall->i, (rdKeyMarkerType)pCall->j);
                break;

            case SITHAIMOVEVEC_SETLOOKTHING:
                SITHVEC_FN(sithAIMove_AISetLookThing)(pLocal, &sithVec_arena.things[pCall->i]);
                break;

            case SITHAIMOVEVEC_SETLOOKPOS:
                SITHVEC_FN(sithAIMove_AISetLookPos)(pLocal, &pCall->v);
                break;

            case SITHAIMOVEVEC_SETLOOKPOSEYELEVEL:
                SITHVEC_FN(sithAIMove_AISetLookPosEyeLevel)(pLocal, &pCall->v);
                break;

            case SITHAIMOVEVEC_SETMOVEPOS:
                sithAIMoveVec_Result(1, (uint32_t)SITHVEC_FN(sithAIMove_AISetMovePos)(pLocal, &pCall->v, pCall->f));
                break;

            case SITHAIMOVEVEC_SETMOVETARGETPOS:
                sithAIMoveVec_Result(1, (uint32_t)SITHVEC_FN(sithAIMove_AISetMoveTargetPos)(pLocal, &pCall->v, pCall->f));
                break;

            case SITHAIMOVEVEC_SPECIALMOVE:
                sithAIMoveVec_Result(1, (uint32_t)SITHVEC_FN(sithAIMove_AISpecialMove)(pLocal, (SithActorSpecialMoveFlags)pCall->i));
                break;

            case SITHAIMOVEVEC_FINALIZESPECIALMOVE:
                SITHVEC_FN(sithAIMove_AIFinalizeSpecialMove)(pLocal, (SithActorSpecialMoveFlags)pCall->i);
                break;

            case SITHAIMOVEVEC_JUMP:
            {
                rdVector3 movePos = pCall->v;
                SITHVEC_FN(sithAIMove_AIJump)(pLocal, &movePos, pCall->f);
                break;
            }

            case SITHAIMOVEVEC_STOP:
                SITHVEC_FN(sithAIMove_AIStop)(pLocal);
                break;

            case SITHAIMOVEVEC_SETGOALREACHED:
                SITHVEC_FN(sithAIMove_SetGoalReached)(pLocal);
                break;

            case SITHAIMOVEVEC_UNREACHABLE:
                SITHVEC_FN(sithAIMove_Unreachable)(pLocal);
                break;

            case SITHAIMOVEVEC_STOPAIMOVEMENT:
                SITHVEC_FN(sithAIMove_StopAIMovement)(pLocal);
                break;

            case SITHAIMOVEVEC_RESETAILOOK:
                SITHVEC_FN(sithAIMove_ResetAILook)(pLocal);
                break;

            case SITHAIMOVEVEC_UPDATEBOSS:
                SITHVEC_FN(sithAIMove_UpdateBoss)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_UPDATEHEADTRACKTARGET:
                SITHVEC_FN(sithAIMove_UpdateHeadTrackTarget)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS:
                SITHVEC_FN(sithAIMove_UpdateHeadTrackLookPos)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_GETQUETZSTRIKEANGLES:
            case SITHAIMOVEVEC_GETQUETZANIM2ANGLES:
            {
                float yaw = 0.0f, pitch = 0.0f, headYaw = 0.0f, headPitch = 0.0f;
                if ( pCtx->pCase->var == SITHAIMOVEVEC_GETQUETZSTRIKEANGLES )
                {
                    SITHVEC_FN(sithAIMove_GetQuetzStrikeAngles)(pLocal, &yaw, &pitch, &headYaw, &headPitch);
                }
                else
                {
                    SITHVEC_FN(sithAIMove_GetQuetzAnim2Angles)(pLocal, &yaw, &pitch, &headYaw, &headPitch);
                }

                sithAIMoveVec_Result(4, SITHAIMOVEVEC_F(yaw), SITHAIMOVEVEC_F(pitch), SITHAIMOVEVEC_F(headYaw), SITHAIMOVEVEC_F(headPitch));
                break;
            }

            case SITHAIMOVEVEC_UPDATEQUETZSTRIKE:
                SITHVEC_FN(sithAIMove_UpdateQuetzStrike)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_UPDATEQUETZANIM2:
                SITHVEC_FN(sithAIMove_UpdateQuetzAnim2)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_UPDATEQUETZANIM3:
                SITHVEC_FN(sithAIMove_UpdateQuetzAnim3)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_UPDATEQUETZANIM5:
                SITHVEC_FN(sithAIMove_UpdateQuetzAnim5)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_UPDATEQUETZANIM4:
                SITHVEC_FN(sithAIMove_UpdateQuetzAnim4)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_UPDATEQUETZTAIL:
                SITHVEC_FN(sithAIMove_UpdateQuetzTail)(pLocal, pCall->f);
                break;

            case SITHAIMOVEVEC_STARTQUETZSTRIKE:
                if ( c == 0 )
                {
                    SITHVEC_FN(sithAIMove_StartQuetzStrike)(pLocal);
                }

                if ( pCall->f != 0.0f )
                {
                    SITHVEC_FN(sithAIMove_UpdateQuetzStrike)(pLocal, pCall->f);
                }

                break;

            case SITHAIMOVEVEC_STARTQUETZANIM2:
                if ( c == 0 )
                {
                    SITHVEC_FN(sithAIMove_StartQuetzAnim2)(pLocal);
                }

                if ( pCall->f != 0.0f )
                {
                    SITHVEC_FN(sithAIMove_UpdateQuetzAnim2)(pLocal, pCall->f);
                }

                break;

            case SITHAIMOVEVEC_STARTQUETZANIM3:
                if ( c == 0 )
                {
                    SITHVEC_FN(sithAIMove_StartQuetzAnim3)(pLocal);
                }

                if ( pCall->f != 0.0f )
                {
                    SITHVEC_FN(sithAIMove_UpdateQuetzAnim3)(pLocal, pCall->f);
                }

                break;

            case SITHAIMOVEVEC_STARTQUETZANIM5:
                if ( c == 0 )
                {
                    SITHVEC_FN(sithAIMove_StartQuetzAnim5)(pLocal);
                }

                if ( pCall->f != 0.0f )
                {
                    SITHVEC_FN(sithAIMove_UpdateQuetzAnim5)(pLocal, pCall->f);
                }

                break;

            case SITHAIMOVEVEC_STARTQUETZANIM4:
                if ( c == 0 )
                {
                    SITHVEC_FN(sithAIMove_StartQuetzAnim4)(pLocal);
                }

                if ( pCall->f != 0.0f )
                {
                    SITHVEC_FN(sithAIMove_UpdateQuetzAnim4)(pLocal, pCall->f);
                }

                break;

            case SITHAIMOVEVEC_UPDATEMARDUKTAIL:
                SITHVEC_FN(sithAIMove_UpdateMardukTail)(pOwner, pCall->f);
                break;
        }

        sithVec_Checkpoint(pCtx);
    }

    return 0;
}

#include "sithAIMoveVecCases.h"

#endif // SITH_AI_TESTS_SITHAIMOVEVEC_H
