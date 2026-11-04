// Cases of the sithAIInstinct function vectors, shared by the capture harness and sithAIInstinctFnVecTests. See
// tv/sithAIInstinctFn/README.md.
//
// A case builds an AI control block (thing 0 is the AI actor, things 1.. are the player, the target, the goal, the thing
// to flee from and other things the events refer to), an instinct with its arguments and state, the AI awareness
// sectors, the game time and the configuration of the engine calls the instinct makes, and then calls one instinct
// function (or one of the snake helpers) once or several times. Every engine call the instinct makes is replaced by a
// stub (sithAIInstinctFnVecSeams.def) that records its arguments and returns what the case configured; the instincts'
// own helpers (BasicFallow called by WallCrawl, SnakeFindMovePos, SnakeMungeTestCheck) run unchanged.
#ifndef SITH_AI_TESTS_SITHAIINSTINCTFNVEC_H
#define SITH_AI_TESTS_SITHAIINSTINCTFNVEC_H

#include "sithVecWorld.h"

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIAwareness.h>
#include <sith/AI/sithAIInstinct.h>
#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithWeapon.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdVector.h>

#include <stddef.h>

// Instinct functions of the module. The harness calls the original ones at their v1.0 addresses.
#ifdef SITHVEC_HARNESS
#define SITHAIFN_FUNC(fn) ((SithAIInstinctFunc)fn##_ADDR)
// File statics of sithAIInstinct.c at their v1.0 addresses
#define SITHAIFN_SNAKELASTDOT   (*(float*)sithAIInstinct_snakeLastDotFwd_ADDR)
#define SITHAIFN_SNAKESLITHER   (*(float*)sithAIInstinct_snakeSlitherDir_ADDR)
#define SITHAIFN_SNAKENEARRANGE (*(float*)sithAIInstinct_snakeFollowNearRange_ADDR)
#else
#define SITHAIFN_FUNC(fn) ((SithAIInstinctFunc)fn)
float* sithAIFnVecTest_GetSnakeLastDotFwd(void);
float* sithAIFnVecTest_GetSnakeSlitherDir(void);
float* sithAIFnVecTest_GetSnakeFollowNearRange(void);
#define SITHAIFN_SNAKELASTDOT   (*sithAIFnVecTest_GetSnakeLastDotFwd())
#define SITHAIFN_SNAKESLITHER   (*sithAIFnVecTest_GetSnakeSlitherDir())
#define SITHAIFN_SNAKENEARRANGE (*sithAIFnVecTest_GetSnakeFollowNearRange())
#endif

// ---------------------------------------------------------------------------------------------------------------
// Case description
// ---------------------------------------------------------------------------------------------------------------
typedef enum
{
    SITHAIFN_LISTEN,
    SITHAIFN_LOOKFORTARGET,
    SITHAIFN_SENSEDANGER,
    SITHAIFN_FEARGUNSHOT,
    SITHAIFN_HOP,
    SITHAIFN_HOVERDRIFT,
    SITHAIFN_TALK,
    SITHAIFN_RANDOMTURN,
    SITHAIFN_OPENDOORS,
    SITHAIFN_JUMP,
    SITHAIFN_LOBFIRE,
    SITHAIFN_PRIMARYFIRE,
    SITHAIFN_ALTERNATEFIRE,
    SITHAIFN_CIRCLESTRAFE,
    SITHAIFN_DODGE,
    SITHAIFN_HITANDRUN,
    SITHAIFN_RETREAT,
    SITHAIFN_RANDOMMOVE,
    SITHAIFN_HUMANCOMBATMOVE,
    SITHAIFN_ROAM,
    SITHAIFN_RETURNHOME,
    SITHAIFN_FLEE,
    SITHAIFN_BASICFALLOW,
    SITHAIFN_WALLCRAWL,
    SITHAIFN_SNAKEFOLLOW,
    SITHAIFN_NUMINSTINCTS,
    // Helpers of the snake instincts that are not instinct functions
    SITHAIFN_SNAKEFINDMOVEPOS = SITHAIFN_NUMINSTINCTS,
    SITHAIFN_SNAKEMUNGETESTCHECK,
    SITHAIFN_SNAKEPLAYSLITHERSOUND,
} SithAIFnId;

// Things of a case: the role selects the arena thing (role - 1)
typedef enum
{
    SITHAIFN_R_NONE = 0, // NULL
    SITHAIFN_R_OWNER,    // the AI actor
    SITHAIFN_R_PLAYER,   // the local player
    SITHAIFN_R_TARGET,
    SITHAIFN_R_GOAL,
    SITHAIFN_R_FLEE,
    SITHAIFN_R_OTHER,    // the thing an event refers to (touched thing, hit thing)
    SITHAIFN_R_SRC,      // a weapon thing in flight
    SITHAIFN_R_PARENT,   // the thing that fired the weapon
    SITHAIFN_R_NUM,
    SITHAIFN_OBJ_SURFACE, // argument is the case surface
    SITHAIFN_OBJ_RAW,     // argument is the value objVal (previous mode, sound class mode)
} SithAIFnRole;

typedef struct
{
    uint8_t type;       // SithThingType; 0: the role's default
    uint8_t moveType;   // SithThingMoveType; 0: SITH_MT_PHYSICS, SITHAIFN_MT_NONE: none
    uint8_t sector;     // sector index
    uint8_t controlAI;  // control type AI; its control block (block 1) has the thing of this role as target
    uint32_t flags;     // SithThingFlag
    uint32_t physFlags; // SithPhysicsFlag
    uint32_t actorFlags;
    uint32_t attachFlags;
    uint32_t damageType; // weapon things
    uint32_t weaponFlags;
    uint8_t bPos, bLook, bHp, bForceMovePlay;
    float pos[3];
    float look[3]; // lvec; rvec and uvec derived
    float vel[3];
    float health, maxHealth;
    float movesize; // 0: 0.25
    float eyeZ;
    int weaponId;
} SithAIFnThing;

#define SITHAIFN_MT_NONE 0xFFu

// Fields of the AI control block; zero unless given
typedef struct
{
    float toTarget[3], toGoal[3];
    float lastGoalSeenPos[3], lastTargetSeenPos[3];
    float homePos[3], homeOrient[3];
    float moveDirection[3], movePos[3], moveStartPos[3], alertPos[3];
    float distance, targetDistance, moveSpeed;
    uint8_t targetSight, targetSight2; // SithAISightState
    uint32_t msecAttackStart;
} SithAIFnLocal;

// Configuration of the engine call stubs. A sequence gives the 2 bit result of the first four calls of a case (repeated);
// SITHAIFN_ALL(v) the same result for every call, 0 the default.
typedef struct
{
    uint32_t canAttack;      // sithAIUtil_CanAttackTarget
    uint32_t sightPos;       // sithAIUtil_GetDistanceToTargetPos: SithAISightState
    uint32_t sightThing;     // sithAIUtil_GetDistanceToTarget: SithAISightState
    uint32_t nextWpnt;       // sithAIUtil_AIMoveToNextWpnt
    uint32_t lineOfFire;     // sithAIUtil_IsLineOfFireClear
    uint32_t fire;           // sithAIUtil_AIFire
    uint32_t sweep;          // sithAIUtil_SweepMoveToPathPos
    uint32_t pathPos;        // sithAIUtil_MakePathPos
    uint32_t roamPos;        // sithAIUtil_MakeRoamPos
    uint32_t getMovePos;     // sithAIUtil_AIGetMovePos
    uint32_t isMoving;       // sithAIUtil_IsThingMoving
    uint32_t checkPath;      // sithAIUtil_CheckPathToPos: 0 blocked, 1 clear, 2 world, 3 thing
    uint32_t checkPos;       // sithAIUtil_CheckPosition: 0 blocked, 1 ok, 2 wall, 3 thing
    uint32_t checkFloor;     // sithAIUtil_CheckFloorAtPos
    uint32_t setMovePos;     // sithAIMove_AISetMovePos
    uint32_t setMoveTarget;  // sithAIMove_AISetMoveTargetPos
    uint32_t special;        // sithAIMove_AISpecialMove
    uint32_t onTrack;        // sithPuppet_IsAnyModeOnTrack
    uint32_t hasWeapon;      // sithWeapon_HasWeaponSelected
    uint32_t isAiming;       // sithWeapon_IsAiming
    uint32_t inRange;        // sithAIAwareness_IsInTransmittingRange
    uint32_t hasInstinct;    // sithAI_HasInstinct
    uint32_t quetz;          // sithThing_CreateQuetzUserBlock
    uint32_t channel;        // sithSoundClass_PlayModeRandom/First: sound channel handle
    uint32_t parent;         // sithThing_GetThingParent: 0 NULL, 1 the case's parent thing, 2 the owner, 3 the player
    uint32_t noCollision;    // *pbNoCollision of sithAIUtil_CheckFloorAtPos/CheckPosition
    uint32_t hSound;         // sithSound_Load: sound handle
    int nullSectorFrom;      // sithCollision_FindSectorInRadius returns NULL from this call (1 based)
    float dist[4];           // *pDist of the sithAIUtil_GetDistanceToTarget(Pos) calls
    float toTarget[3];       // *pToTarget of the same calls
    float heading[3];        // sithAIUtil_GetXYHeadingVector
    float pathDest[3];       // destination of MakePathPos, MakeRoamPos, AIGetMovePos
    float hitPos[3];         // *pEndPos of sithAIUtil_CheckPathToPos
    float height;            // sithPhysics_GetThingHeight
    float maxAimDist;        // sithWeapon_GetWeaponMaxAimDistance
    uint32_t pathFlagsMask;  // bits of the sithAIUtil_CheckPathToPos flags that are recorded (0: all)
} SithAIFnStubs;

#define SITHAIFN_ALL(v)      ((uint32_t)(v) * 0x55555555u)
#define SITHAIFN_SEQ(a, b, c, d) (((uint32_t)(a) | ((uint32_t)(b) << 2) | ((uint32_t)(c) << 4) | ((uint32_t)(d) << 6)) * 0x01010101u)
#define SITHAIFN_T SITHAIFN_ALL(1)

typedef struct
{
    uint8_t fn;           // SithAIFnId
    uint8_t calls;        // number of calls, 0: 1
    uint8_t bTime;        // msec and sec are given
    uint8_t bSnake;       // snake statics are given
    uint8_t noRet;        // the return value is not recorded (see README)
    uint8_t obj;          // SithAIFnRole, SITHAIFN_OBJ_SURFACE, SITHAIFN_OBJ_RAW
    uint8_t targetRole, goalRole, fleeRole;
    uint8_t noPlayer;     // sithPlayer_g_pLocalPlayerThing is NULL
    uint8_t quetz;        // 1: owner has a Quetz block, 2: with posFudge
    uint8_t cutscene;
    uint32_t event;       // SithAIEventType
    uint32_t objVal;
    uint32_t dtMs;        // time between the calls
    uint32_t msec;
    float sec;
    uint32_t mode, submode;
    uint32_t debugFlags;  // sithMain_g_sith_mode.debugModeFlags
    uint32_t sectorFlags[2];
    const char* rnd;      // buckets of the first rand() results, see sithAIFnVec_FindSeed
    float args[24];       // instinct arguments (float and int)
    float st[4];          // instinct state parameters
    uint32_t stFlags, stNext;
    float cls[4];         // class: sightDistance, heardDistance, fov, rank
    SithAIFnLocal lb;
    SithAIFnThing thing[SITHAIFN_R_NUM];
    struct
    {
        float level[4];
        uint8_t thing[4]; // roles
        float start[3];
    } aw;
    float surfNormal[3];
    uint32_t surfFlags, attachSurfFlags;
    float quetzFudge;
    float snake[3];        // snake statics: lastDotFwd, slitherDir, followNearRange
    float vecA[3], vecB[3], fArg;
    SithAIFnStubs stubs;
} SithAIFnParams;

// ---------------------------------------------------------------------------------------------------------------
// Arena layout
// ---------------------------------------------------------------------------------------------------------------
typedef struct
{
    SithAIInstinct instinct;
    SithAIInstinctState state;
    SithAIAwarenessSector aw[8];
    SithSurface surf[2];
    SithQuetzUserBlock quetz;
    rdVector3 vec[3];
} SithAIFnUser;

static_assert(sizeof(SithAIFnUser) <= SITHVEC_USERSIZE, "SithAIFnUser must fit into the arena's user area");

#define SITHAIFN_USER ((SithAIFnUser*)sithVec_arena.user)
#define SITHAIFN_BLOCK (&sithVec_arena.aiBlocks[0])
#define SITHAIFN_ROLETHING(role) ((role) ? &sithVec_arena.things[(role) - 1u] : NULL)

// Mirrors of the file statics and of the variables the instincts read (watched with the arena)
static uint32_t sithAIFnVec_aMirror[3];

static void sithAIFnVec_Sync(void)
{
    memcpy(&sithAIFnVec_aMirror[0], &SITHAIFN_SNAKELASTDOT, 4);
    memcpy(&sithAIFnVec_aMirror[1], &SITHAIFN_SNAKESLITHER, 4);
    memcpy(&sithAIFnVec_aMirror[2], &SITHAIFN_SNAKENEARRANGE, 4);
}

static void sithAIFnVec_Watch(void)
{
    SITHVEC_WATCH(sithAIFnVec_aMirror);
}

// ---------------------------------------------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------------------------------------------
enum
{
    SITHAIFN_EV_RET = SITHVEC_EV_SUITE, // return value of a call of the instinct
    SITHAIFN_EV_LOOKTHING,
    SITHAIFN_EV_LOOKPOS,
    SITHAIFN_EV_LOOKPOSEYE,
    SITHAIFN_EV_SETMOVEPOS,
    SITHAIFN_EV_SETMOVETARGET,
    SITHAIFN_EV_SPECIALMOVE,
    SITHAIFN_EV_AIJUMP,
    SITHAIFN_EV_UNREACHABLE,
    SITHAIFN_EV_STOPMOVE,
    SITHAIFN_EV_SETMODE,
    SITHAIFN_EV_CLEARMODE,
    SITHAIFN_EV_PLAYSOUNDMODE,
    SITHAIFN_EV_CANATTACK,
    SITHAIFN_EV_UPDATETARGETSIGHT,
    SITHAIFN_EV_UPDATEGOALSIGHT,
    SITHAIFN_EV_MAKEPATHPOS,
    SITHAIFN_EV_GETDISTPOS,
    SITHAIFN_EV_GETDIST,
    SITHAIFN_EV_NEXTWPNT,
    SITHAIFN_EV_GETHEADING,
    SITHAIFN_EV_SETWEAPONFLAGS,
    SITHAIFN_EV_LINEOFFIRE,
    SITHAIFN_EV_AIFIRE,
    SITHAIFN_EV_SWEEP,
    SITHAIFN_EV_CHECKPATH,
    SITHAIFN_EV_PAUSEMOVE,
    SITHAIFN_EV_GETMOVEPOS,
    SITHAIFN_EV_ROAMPOS,
    SITHAIFN_EV_ISMOVING,
    SITHAIFN_EV_CHECKPOS,
    SITHAIFN_EV_CHECKFLOOR,
    SITHAIFN_EV_FORCEUPDATE,
    SITHAIFN_EV_HASINSTINCT,
    SITHAIFN_EV_ENABLEINSTINCT,
    SITHAIFN_EV_EMITEVENT,
    SITHAIFN_EV_TRANSMIT,
    SITHAIFN_EV_INRANGE,
    SITHAIFN_EV_PLAYRANDOM,
    SITHAIFN_EV_PLAYFIRST,
    SITHAIFN_EV_SOUNDLOAD,
    SITHAIFN_EV_PLAYSOUNDTHING,
    SITHAIFN_EV_STOPSOUNDS,
    SITHAIFN_EV_ONTRACK,
    SITHAIFN_EV_PUPPETPLAY,
    SITHAIFN_EV_APPLYFORCE,
    SITHAIFN_EV_GETPARENT,
    SITHAIFN_EV_DETACH,
    SITHAIFN_EV_CREATEQUETZ,
    SITHAIFN_EV_ATTACHSURFACE,
    SITHAIFN_EV_ACTIVATE,
    SITHAIFN_EV_HASWEAPON,
    SITHAIFN_EV_MAXAIM,
    SITHAIFN_EV_ISAIMING,
    SITHAIFN_EV_ROTATE3,
    SITHAIFN_EV_ROTATE3ACC,
};

// Stub configuration of the running case and call counters
static SithAIFnStubs sithAIFnVec_cfg;
static struct
{
    unsigned numLookEye, numFire, numPath;
    unsigned numCallbacks;
    uint32_t aCallbacks[8];
    rdVector3 lookEyePos, sightEndPos, pathStart, pathDirection;
    SithAIControlBlock* pLookLocal;
    SithAIUtilPathFlags pathFlags;
    float minMoveDist, moveDist, radius;
    int burstCount, weaponNum;
    float minFireDist, maxFireDist, fireDot, aimError;
    SithAIUtilFireFlags fireFlags;
    float waypointSpeed;
    int waypointMode;
} sithAIFnVec_observed;

static void sithAIFnVec_ObserveCallback(uint32_t id)
{
    if ( sithAIFnVec_observed.numCallbacks < SITHVEC_ARRAYLEN(sithAIFnVec_observed.aCallbacks) )
    {
        sithAIFnVec_observed.aCallbacks[sithAIFnVec_observed.numCallbacks] = id;
    }
    ++sithAIFnVec_observed.numCallbacks;
}


static struct
{
    unsigned canAttack, sightPos, sightThing, nextWpnt, lineOfFire, fire, sweep, pathPos, roamPos, getMovePos, isMoving;
    unsigned checkPath, checkPos, checkFloor, setMovePos, setMoveTarget, special, onTrack, hasWeapon, isAiming, inRange;
    unsigned hasInstinct, quetz, channel, parent, dist;
} sithAIFnVec_ctr;

static unsigned sithAIFnVec_Seq(uint32_t seq, unsigned* pCounter)
{
    unsigned v = (seq >> ((*pCounter & 15u) * 2u)) & 3u;
    ++*pCounter;
    return v;
}

static void sithAIFnVec_EvVec(const rdVector3* pVec)
{
    sithVec_EvWords(pVec, sizeof(*pVec));
}

static void sithAIFnVec_SetVec(rdVector3* pDest, const float* pSrc)
{
    pDest->x = pSrc[0];
    pDest->y = pSrc[1];
    pDest->z = pSrc[2];
}

#define SF(f) sithVec_FloatBits(f)
#define SP(p) SITHVEC_P(p)

// sithAIMove
SITHVEC_STUB void J3DAPI sithVec_sithAIMove_AISetLookThing(SithAIControlBlock* pLocal, const SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_LOOKTHING, 2, SP(pLocal), SP(pThing));
}

SITHVEC_STUB void J3DAPI sithVec_sithAIMove_AISetLookPos(SithAIControlBlock* pLocal, const rdVector3* targetPos)
{
    sithVec_Ev(SITHAIFN_EV_LOOKPOS, 1, SP(pLocal));
    sithAIFnVec_EvVec(targetPos);
}

SITHVEC_STUB void J3DAPI sithVec_sithAIMove_AISetLookPosEyeLevel(SithAIControlBlock* pLocal, const rdVector3* targetPos)
{
    ++sithAIFnVec_observed.numLookEye;
    sithAIFnVec_observed.lookEyePos = *targetPos;
    sithAIFnVec_observed.pLookLocal = pLocal;
    sithVec_Ev(SITHAIFN_EV_LOOKPOSEYE, 1, SP(pLocal));
    sithAIFnVec_EvVec(targetPos);
}

SITHVEC_STUB int J3DAPI sithVec_sithAIMove_AISetMovePos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed)
{
    sithVec_Ev(SITHAIFN_EV_SETMOVEPOS, 2, SP(pLocal), SF(moveSpeed));
    sithAIFnVec_EvVec(moveToPos);
    pLocal->movePos   = *moveToPos;
    pLocal->moveSpeed = moveSpeed;
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.setMovePos, &sithAIFnVec_ctr.setMovePos);
}

SITHVEC_STUB int J3DAPI sithVec_sithAIMove_AISetMoveTargetPos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed)
{
    sithVec_Ev(SITHAIFN_EV_SETMOVETARGET, 2, SP(pLocal), SF(moveSpeed));
    sithAIFnVec_EvVec(moveToPos);
    pLocal->movePos   = *moveToPos;
    pLocal->moveSpeed = moveSpeed;
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.setMoveTarget, &sithAIFnVec_ctr.setMoveTarget);
}

SITHVEC_STUB int J3DAPI sithVec_sithAIMove_AISpecialMove(SithAIControlBlock* pLocal, SithActorSpecialMoveFlags moveFlags)
{
    sithVec_Ev(SITHAIFN_EV_SPECIALMOVE, 2, SP(pLocal), (uint32_t)moveFlags);
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.special, &sithAIFnVec_ctr.special);
}

SITHVEC_STUB void J3DAPI sithVec_sithAIMove_AIJump(SithAIControlBlock* pLocal, rdVector3* movePos, float speed)
{
    sithVec_Ev(SITHAIFN_EV_AIJUMP, 2, SP(pLocal), SF(speed));
    sithAIFnVec_EvVec(movePos);
}

SITHVEC_STUB void J3DAPI sithVec_sithAIMove_Unreachable(SithAIControlBlock* pLocal)
{
    sithVec_Ev(SITHAIFN_EV_UNREACHABLE, 1, SP(pLocal));
}

SITHVEC_STUB void J3DAPI sithVec_sithAIMove_StopAIMovement(SithAIControlBlock* pLocal)
{
    sithVec_Ev(SITHAIFN_EV_STOPMOVE, 1, SP(pLocal));
}

// sithAIUtil
SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_AISetMode(SithAIControlBlock* pLocal, SithAIMode mode)
{
    sithAIFnVec_ObserveCallback(SITHAIFN_EV_SETMODE);
    sithVec_Ev(SITHAIFN_EV_SETMODE, 2, SP(pLocal), (uint32_t)mode);
    pLocal->mode |= mode;
}

SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_AIClearMode(SithAIControlBlock* pLocal, SithAIMode mode)
{
    sithVec_Ev(SITHAIFN_EV_CLEARMODE, 2, SP(pLocal), (uint32_t)mode);
    pLocal->mode &= ~mode;
}

SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_AIPlaySoundMode(SithAIControlBlock* pLocal, SithSoundClassMode mode)
{
    sithAIFnVec_ObserveCallback(SITHAIFN_EV_PLAYSOUNDMODE);
    sithVec_Ev(SITHAIFN_EV_PLAYSOUNDMODE, 2, SP(pLocal), (uint32_t)mode);
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_CanAttackTarget(const SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_CANATTACK, 1, SP(pThing));
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.canAttack, &sithAIFnVec_ctr.canAttack);
}

SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_UpdateTargetSight(SithAIControlBlock* pLocal, SithAIUtilFireFlags flags)
{
    sithVec_Ev(SITHAIFN_EV_UPDATETARGETSIGHT, 2, SP(pLocal), (uint32_t)flags);
}

SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_UpdateGoalSight(SithAIControlBlock* pLocal)
{
    sithVec_Ev(SITHAIFN_EV_UPDATEGOALSIGHT, 1, SP(pLocal));
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_MakePathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float minDist, float moveDist, float angle, int flags, rdVector3* pDirection, rdVector3* pDestPos)
{
    sithVec_Ev(SITHAIFN_EV_MAKEPATHPOS, 5, SP(pLocal), SF(minDist), SF(moveDist), SF(angle), (uint32_t)flags);
    sithAIFnVec_EvVec(pPos);
    sithAIFnVec_EvVec(pDirection);
    int ret = (int)sithAIFnVec_Seq(sithAIFnVec_cfg.pathPos, &sithAIFnVec_ctr.pathPos);
    if ( ret )
    {
        sithAIFnVec_SetVec(pDestPos, sithAIFnVec_cfg.pathDest);
    }

    return ret;
}

static unsigned sithAIFnVec_DistCall(float* pDist, rdVector3* pToTarget)
{
    unsigned k = sithAIFnVec_ctr.dist++ & 3u;
    *pDist     = sithAIFnVec_cfg.dist[k];
    sithAIFnVec_SetVec(pToTarget, sithAIFnVec_cfg.toTarget);
    return k;
}

SITHVEC_STUB SithAISightState J3DAPI sithVec_sithAIUtil_GetDistanceToTargetPos(SithThing* pViewer, rdVector3* startPos, rdVector3* endPos, float fov, float sightDistance, float collisionSize, rdVector3* toTarget, float* pDist)
{
    sithVec_Ev(SITHAIFN_EV_GETDISTPOS, 4, SP(pViewer), SF(fov), SF(sightDistance), SF(collisionSize));
    sithAIFnVec_EvVec(startPos);
    sithAIFnVec_EvVec(endPos);
    sithAIFnVec_observed.sightEndPos = *endPos;
    sithAIFnVec_DistCall(pDist, toTarget);
    return (SithAISightState)sithAIFnVec_Seq(sithAIFnVec_cfg.sightPos, &sithAIFnVec_ctr.sightPos);
}

SITHVEC_STUB SithAISightState J3DAPI sithVec_sithAIUtil_GetDistanceToTarget(SithThing* pViewer, rdVector3* startPos, const SithThing* pTarget, float fov, float sightDistance, float collisionSize, rdVector3* toTarget, float* pDist)
{
    sithVec_Ev(SITHAIFN_EV_GETDIST, 5, SP(pViewer), SP(pTarget), SF(fov), SF(sightDistance), SF(collisionSize));
    sithAIFnVec_EvVec(startPos);
    sithAIFnVec_DistCall(pDist, toTarget);
    return (SithAISightState)sithAIFnVec_Seq(sithAIFnVec_cfg.sightThing, &sithAIFnVec_ctr.sightThing);
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_AIMoveToNextWpnt(SithAIControlBlock* pLocal, float moveSpeed, float degTurn, int mode)
{
    sithAIFnVec_ObserveCallback(SITHAIFN_EV_NEXTWPNT);
    sithAIFnVec_observed.waypointSpeed = moveSpeed;
    sithAIFnVec_observed.waypointMode  = mode;
    sithVec_Ev(SITHAIFN_EV_NEXTWPNT, 4, SP(pLocal), SF(moveSpeed), SF(degTurn), (uint32_t)mode);
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.nextWpnt, &sithAIFnVec_ctr.nextWpnt);
}

SITHVEC_STUB signed int J3DAPI sithVec_sithAIUtil_GetXYHeadingVector(const SithThing* pThing, rdVector3* pDest)
{
    sithVec_Ev(SITHAIFN_EV_GETHEADING, 1, SP(pThing));
    sithAIFnVec_SetVec(pDest, sithAIFnVec_cfg.heading);
    return 1;
}

SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_SetWeaponFireFlags(int weaponNum, SithAIUtilFireFlags* pFlags)
{
    sithVec_Ev(SITHAIFN_EV_SETWEAPONFLAGS, 2, (uint32_t)weaponNum, (uint32_t)*pFlags);
    *pFlags = (SithAIUtilFireFlags)((uint32_t)*pFlags | (((uint32_t)weaponNum & 0xFFu) << 16));
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_IsLineOfFireClear(const SithAIControlBlock* pLocal, const rdVector3* pPos)
{
    sithVec_Ev(SITHAIFN_EV_LINEOFFIRE, 1, SP(pLocal));
    sithAIFnVec_EvVec(pPos);
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.lineOfFire, &sithAIFnVec_ctr.lineOfFire);
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_AIFire(SithAIControlBlock* pLocal, float minDist, float maxDist, float fireDot, float aimError, int weaponNum, SithAIUtilFireFlags flags, int burstCount)
{
    ++sithAIFnVec_observed.numFire;
    sithAIFnVec_observed.burstCount = burstCount;
    sithAIFnVec_observed.weaponNum  = weaponNum;
    sithAIFnVec_observed.minFireDist = minDist;
    sithAIFnVec_observed.maxFireDist = maxDist;
    sithAIFnVec_observed.fireDot = fireDot;
    sithAIFnVec_observed.aimError = aimError;
    sithAIFnVec_observed.fireFlags = flags;
    sithVec_Ev(SITHAIFN_EV_AIFIRE, 8, SP(pLocal), SF(minDist), SF(maxDist), SF(fireDot), SF(aimError), (uint32_t)weaponNum, (uint32_t)flags, (uint32_t)burstCount);
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.fire, &sithAIFnVec_ctr.fire);
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_SweepMoveToPathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float angle, float maxAngle, float deltaAngle, SithAIUtilMkPointFlags flags)
{
    sithVec_Ev(SITHAIFN_EV_SWEEP, 5, SP(pLocal), SF(angle), SF(maxAngle), SF(deltaAngle), (uint32_t)flags);
    sithAIFnVec_EvVec(pPos);
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.sweep, &sithAIFnVec_ctr.sweep);
}

SITHVEC_STUB SithAIUtilPathResult J3DAPI sithVec_sithAIUtil_CheckPathToPos(SithAIControlBlock* pLocal, SithSector* pStartSector, rdVector3* startPos, SithAIUtilPathFlags flags, float minMoveDist, float moveDist, float radius, rdVector3* pDirection, rdVector3* pEndPos)
{
    ++sithAIFnVec_observed.numPath;
    sithAIFnVec_observed.pathFlags = flags;
    sithAIFnVec_observed.pathStart = *startPos;
    sithAIFnVec_observed.pathDirection = *pDirection;
    sithAIFnVec_observed.minMoveDist = minMoveDist;
    sithAIFnVec_observed.moveDist = moveDist;
    sithAIFnVec_observed.radius = radius;
    uint32_t mask = sithAIFnVec_cfg.pathFlagsMask ? sithAIFnVec_cfg.pathFlagsMask : 0xFFFFFFFFu;
    sithVec_Ev(SITHAIFN_EV_CHECKPATH, 6, SP(pLocal), SP(pStartSector), (uint32_t)flags & mask, SF(minMoveDist), SF(moveDist), SF(radius));
    sithAIFnVec_EvVec(startPos);
    sithAIFnVec_EvVec(pDirection);
    static const SithAIUtilPathResult aResults[4] = { SITHAIUTIL_PATH_BLOCKED, SITHAIUTIL_PATH_CLEAR, SITHAIUTIL_PATH_WORLD, SITHAIUTIL_PATH_THING };
    sithAIFnVec_SetVec(pEndPos, sithAIFnVec_cfg.hitPos);
    return aResults[sithAIFnVec_Seq(sithAIFnVec_cfg.checkPath, &sithAIFnVec_ctr.checkPath)];
}

SITHVEC_STUB void J3DAPI sithVec_sithAIUtil_AIPauseMove(SithAIControlBlock* pLocal, int msecPause)
{
    sithVec_Ev(SITHAIFN_EV_PAUSEMOVE, 2, SP(pLocal), (uint32_t)msecPause);
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_AIGetMovePos(const SithAIControlBlock* pLocal, SithAIUtilMkPointFlags flags, rdVector3* pDestPos)
{
    sithVec_Ev(SITHAIFN_EV_GETMOVEPOS, 2, SP(pLocal), (uint32_t)flags);
    sithAIFnVec_SetVec(pDestPos, sithAIFnVec_cfg.pathDest);
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.getMovePos, &sithAIFnVec_ctr.getMovePos);
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_MakeRoamPos(SithAIControlBlock* pLocal, rdVector3* pPos, float minDist, float moveDist, int bAllow3D, int maxRetries, rdVector3* pDestPos)
{
    sithVec_Ev(SITHAIFN_EV_ROAMPOS, 5, SP(pLocal), SF(minDist), SF(moveDist), (uint32_t)bAllow3D, (uint32_t)maxRetries);
    sithAIFnVec_EvVec(pPos);
    int ret = (int)sithAIFnVec_Seq(sithAIFnVec_cfg.roamPos, &sithAIFnVec_ctr.roamPos);
    if ( ret )
    {
        sithAIFnVec_SetVec(pDestPos, sithAIFnVec_cfg.pathDest);
    }

    return ret;
}

SITHVEC_STUB int J3DAPI sithVec_sithAIUtil_IsThingMoving(const SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_ISMOVING, 1, SP(pThing));
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.isMoving, &sithAIFnVec_ctr.isMoving);
}

static const SithAIUtilFloorCheck sithAIFnVec_aFloor[4] = { SITHAIUTIL_FLOOR_BLOCKED, SITHAIUTIL_FLOOR_OK, SITHAIUTIL_FLOOR_WALL, SITHAIUTIL_FLOOR_THING };

SITHVEC_STUB SithAIUtilFloorCheck J3DAPI sithVec_sithAIUtil_CheckPosition(const SithAIControlBlock* pLocal, rdVector3* endPos, int* pbNoCollision)
{
    sithVec_Ev(SITHAIFN_EV_CHECKPOS, 2, SP(pLocal), (uint32_t)(pbNoCollision != NULL));
    sithAIFnVec_EvVec(endPos);
    if ( pbNoCollision )
    {
        *pbNoCollision = (int)(sithAIFnVec_cfg.noCollision & 1u);
    }

    return sithAIFnVec_aFloor[sithAIFnVec_Seq(sithAIFnVec_cfg.checkPos, &sithAIFnVec_ctr.checkPos)];
}

SITHVEC_STUB SithAIUtilFloorCheck J3DAPI sithVec_sithAIUtil_CheckFloorAtPos(const SithAIControlBlock* pLocal, const rdVector3* startPos, SithSector* pStartSector, int* pbNoCollision)
{
    sithVec_Ev(SITHAIFN_EV_CHECKFLOOR, 3, SP(pLocal), SP(pStartSector), (uint32_t)(pbNoCollision != NULL));
    sithAIFnVec_EvVec(startPos);
    if ( pbNoCollision )
    {
        *pbNoCollision = (int)(sithAIFnVec_cfg.noCollision & 1u);
    }

    return sithAIFnVec_aFloor[sithAIFnVec_Seq(sithAIFnVec_cfg.checkFloor, &sithAIFnVec_ctr.checkFloor)];
}

// sithAI
SITHVEC_STUB int J3DAPI sithVec_sithAI_ForceInstinctUpdate(SithAIControlBlock* pLocal, const char* pInstinctName, int bSendModeChangeEvent)
{
    sithVec_Ev(SITHAIFN_EV_FORCEUPDATE, 3, SP(pLocal), sithVec_Hash(pInstinctName), (uint32_t)bSendModeChangeEvent);
    return 1;
}

SITHVEC_STUB int J3DAPI sithVec_sithAI_HasInstinct(const SithAIControlBlock* pLocal, const char* pName)
{
    sithVec_Ev(SITHAIFN_EV_HASINSTINCT, 2, SP(pLocal), sithVec_Hash(pName));
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.hasInstinct, &sithAIFnVec_ctr.hasInstinct);
}

SITHVEC_STUB int J3DAPI sithVec_sithAI_EnableInstinct(SithAIControlBlock* pLocal, const char* pInstinctName, int bEnable)
{
    sithVec_Ev(SITHAIFN_EV_ENABLEINSTINCT, 3, SP(pLocal), sithVec_Hash(pInstinctName), (uint32_t)bEnable);
    return 1;
}

SITHVEC_STUB void J3DAPI sithVec_sithAI_EmitEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{
    sithVec_Ev(SITHAIFN_EV_EMITEVENT, 3, SP(pLocal), (uint32_t)event, SP(pObject));
}

// sithAIAwareness
SITHVEC_STUB int J3DAPI sithVec_sithAIAwareness_CreateTransmittingEvent(SithSector* pSector, const rdVector3* pos, int type, float transmittingLevel, SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_TRANSMIT, 4, SP(pSector), (uint32_t)type, SF(transmittingLevel), SP(pThing));
    sithAIFnVec_EvVec(pos);
    return 1;
}

SITHVEC_STUB int J3DAPI sithVec_sithAIAwareness_IsInTransmittingRange(const SithAIAwarenessSector* pAISector, int type, const SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_INRANGE, 3, SP(pAISector), (uint32_t)type, SP(pThing));
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.inRange, &sithAIFnVec_ctr.inRange);
}

// Sound
SITHVEC_STUB tSoundChannelHandle J3DAPI sithVec_sithSoundClass_PlayModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    sithVec_Ev(SITHAIFN_EV_PLAYRANDOM, 2, SP(pThing), (uint32_t)mode);
    return (tSoundChannelHandle)sithAIFnVec_Seq(sithAIFnVec_cfg.channel, &sithAIFnVec_ctr.channel);
}

SITHVEC_STUB tSoundChannelHandle J3DAPI sithVec_sithSoundClass_PlayModeFirst(SithThing* pThing, SithSoundClassMode mode)
{
    sithVec_Ev(SITHAIFN_EV_PLAYFIRST, 2, SP(pThing), (uint32_t)mode);
    return (tSoundChannelHandle)sithAIFnVec_Seq(sithAIFnVec_cfg.channel, &sithAIFnVec_ctr.channel);
}

SITHVEC_STUB tSoundHandle J3DAPI sithVec_sithSound_Load(SithWorld* pWorld, const char* filename)
{
    sithVec_Ev(SITHAIFN_EV_SOUNDLOAD, 2, SP(pWorld), sithVec_Hash(filename));
    return (tSoundHandle)sithAIFnVec_cfg.hSound;
}

SITHVEC_STUB tSoundChannelHandle J3DAPI sithVec_sithSoundMixer_PlaySoundThing(tSoundHandle hSnd, const SithThing* pThing, float volume, float minRadius, float maxRadius, SoundPlayFlag playflags)
{
    sithVec_Ev(SITHAIFN_EV_PLAYSOUNDTHING, 6, (uint32_t)hSnd, SP(pThing), SF(volume), SF(minRadius), SF(maxRadius), (uint32_t)playflags);
    return 0;
}

SITHVEC_STUB void J3DAPI sithVec_sithSoundMixer_StopAllSoundsThing(const SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_STOPSOUNDS, 1, SP(pThing));
}

// Puppet, physics, thing, weapon
SITHVEC_STUB int J3DAPI sithVec_sithPuppet_IsAnyModeOnTrack(const SithThing* pThing, SithPuppetSubMode firstMode, SithPuppetSubMode lastMode)
{
    sithVec_Ev(SITHAIFN_EV_ONTRACK, 3, SP(pThing), (uint32_t)firstMode, (uint32_t)lastMode);
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.onTrack, &sithAIFnVec_ctr.onTrack);
}

SITHVEC_STUB int J3DAPI sithVec_sithPuppet_PlayMode(SithThing* pThing, SithPuppetSubMode submode, rdPuppetTrackCallback pfCallback)
{
    sithVec_Ev(SITHAIFN_EV_PUPPETPLAY, 3, SP(pThing), (uint32_t)submode, (uint32_t)(uintptr_t)pfCallback);
    return 0;
}

SITHVEC_STUB void J3DAPI sithVec_sithPhysics_ApplyForce(SithThing* pThing, const rdVector3* force)
{
    sithVec_Ev(SITHAIFN_EV_APPLYFORCE, 1, SP(pThing));
    sithAIFnVec_EvVec(force);
}

// A pure query that is not recorded: SnakeMungeTestCheck of the original calls it twice (it also computes an unused
// position, curFarPos) and the reconstruction once, with the same result (see the README)
SITHVEC_STUB float J3DAPI sithVec_sithPhysics_GetThingHeight(const SithThing* pThing)
{
    J3D_UNUSED(pThing);
    return sithAIFnVec_cfg.height;
}

SITHVEC_STUB SithThing* J3DAPI sithVec_sithThing_GetThingParent(const SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_GETPARENT, 1, SP(pThing));
    switch ( sithAIFnVec_Seq(sithAIFnVec_cfg.parent, &sithAIFnVec_ctr.parent) )
    {
        case 1:
            return SITHAIFN_ROLETHING(SITHAIFN_R_PARENT);
        case 2:
            return SITHAIFN_ROLETHING(SITHAIFN_R_OWNER);
        case 3:
            return SITHAIFN_ROLETHING(SITHAIFN_R_PLAYER);
        default:
            return NULL;
    }
}

SITHVEC_STUB void J3DAPI sithVec_sithThing_DetachThing(SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_DETACH, 1, SP(pThing));
    pThing->attach.flags = 0;
}

SITHVEC_STUB int J3DAPI sithVec_sithThing_CreateQuetzUserBlock(SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_CREATEQUETZ, 1, SP(pThing));
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.quetz, &sithAIFnVec_ctr.quetz);
}

SITHVEC_STUB void J3DAPI sithVec_sithThing_AttachThingToSurface(SithThing* pThing, SithSurface* pSurface, int bNoImpactUpdate)
{
    sithVec_Ev(SITHAIFN_EV_ATTACHSURFACE, 3, SP(pThing), SP(pSurface), (uint32_t)bNoImpactUpdate);
}

SITHVEC_STUB int J3DAPI sithVec_sithPlayerActions_Activate(SithThing* pPlayer)
{
    sithVec_Ev(SITHAIFN_EV_ACTIVATE, 1, SP(pPlayer));
    return 1;
}

SITHVEC_STUB int J3DAPI sithVec_sithWeapon_HasWeaponSelected(const SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_HASWEAPON, 1, SP(pThing));
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.hasWeapon, &sithAIFnVec_ctr.hasWeapon);
}

SITHVEC_STUB float J3DAPI sithVec_sithWeapon_GetWeaponMaxAimDistance(SithWeaponId weaponId)
{
    sithVec_Ev(SITHAIFN_EV_MAXAIM, 1, (uint32_t)weaponId);
    return sithAIFnVec_cfg.maxAimDist;
}

SITHVEC_STUB int J3DAPI sithVec_sithWeapon_IsAiming(SithThing* pThing)
{
    sithVec_Ev(SITHAIFN_EV_ISAIMING, 1, SP(pThing));
    return (int)sithAIFnVec_Seq(sithAIFnVec_cfg.isAiming, &sithAIFnVec_ctr.isAiming);
}

// Rotation of a vector by the yaw angle around the z axis (the other angles are not used by the instincts)
static void sithAIFnVec_RotateYaw(rdVector3* pDest, const rdVector3* pSrc, const rdVector3* pPyr)
{
    float s, c;
    sithVec_stdMath_SinCos(pPyr->y, &s, &c);
    rdVector3 v = *pSrc;
    pDest->x    = v.x * c + v.y * s;
    pDest->y    = -v.x * s + v.y * c;
    pDest->z    = v.z;
}

SITHVEC_STUB void J3DAPI sithVec_rdVector_Rotate3(rdVector3* vec, const rdVector3* pivot, const rdVector3* pyr)
{
    sithVec_Ev(SITHAIFN_EV_ROTATE3, 0);
    sithAIFnVec_EvVec(pivot);
    sithAIFnVec_EvVec(pyr);
    sithAIFnVec_RotateYaw(vec, pivot, pyr);
}

SITHVEC_STUB void J3DAPI sithVec_rdVector_Rotate3Acc(rdVector3* vec, const rdVector3* pyr)
{
    sithVec_Ev(SITHAIFN_EV_ROTATE3ACC, 0);
    sithAIFnVec_EvVec(vec);
    sithAIFnVec_EvVec(pyr);
    sithAIFnVec_RotateYaw(vec, vec, pyr);
}

// ---------------------------------------------------------------------------------------------------------------
// Builder
// ---------------------------------------------------------------------------------------------------------------
static SithAIInstinctFunc sithAIFnVec_apFuncs[SITHAIFN_NUMINSTINCTS];

#define SITHAIFN_ADDINSTINCT(i, fn) \
    SITHVEC_ADDFUNC(fn);            \
    sithAIFnVec_apFuncs[i] = SITHAIFN_FUNC(fn)

static void sithAIFnVec_AddFuncs(void)
{
    SITHAIFN_ADDINSTINCT(SITHAIFN_LISTEN, sithAIInstinct_Listen);
    SITHAIFN_ADDINSTINCT(SITHAIFN_LOOKFORTARGET, sithAIInstinct_LookForTarget);
    SITHAIFN_ADDINSTINCT(SITHAIFN_SENSEDANGER, sithAIInstinct_SenseDanger);
    SITHAIFN_ADDINSTINCT(SITHAIFN_FEARGUNSHOT, sithAIInstinct_FearGunshot);
    SITHAIFN_ADDINSTINCT(SITHAIFN_HOP, sithAIInstinct_Hop);
    SITHAIFN_ADDINSTINCT(SITHAIFN_HOVERDRIFT, sithAIInstinct_HoverDrift);
    SITHAIFN_ADDINSTINCT(SITHAIFN_TALK, sithAIInstinct_Talk);
    SITHAIFN_ADDINSTINCT(SITHAIFN_RANDOMTURN, sithAIInstinct_RandomTurn);
    SITHAIFN_ADDINSTINCT(SITHAIFN_OPENDOORS, sithAIInstinct_OpenDoors);
    SITHAIFN_ADDINSTINCT(SITHAIFN_JUMP, sithAIInstinct_Jump);
    SITHAIFN_ADDINSTINCT(SITHAIFN_LOBFIRE, sithAIInstinct_LobFire);
    SITHAIFN_ADDINSTINCT(SITHAIFN_PRIMARYFIRE, sithAIInstinct_PrimaryFire);
    SITHAIFN_ADDINSTINCT(SITHAIFN_ALTERNATEFIRE, sithAIInstinct_AlternateFire);
    SITHAIFN_ADDINSTINCT(SITHAIFN_CIRCLESTRAFE, sithAIInstinct_CircleStrafe);
    SITHAIFN_ADDINSTINCT(SITHAIFN_DODGE, sithAIInstinct_Dodge);
    SITHAIFN_ADDINSTINCT(SITHAIFN_HITANDRUN, sithAIInstinct_HitAndRun);
    SITHAIFN_ADDINSTINCT(SITHAIFN_RETREAT, sithAIInstinct_Retreat);
    SITHAIFN_ADDINSTINCT(SITHAIFN_RANDOMMOVE, sithAIInstinct_RandomMove);
    SITHAIFN_ADDINSTINCT(SITHAIFN_HUMANCOMBATMOVE, sithAIInstinct_HumanCombatMove);
    SITHAIFN_ADDINSTINCT(SITHAIFN_ROAM, sithAIInstinct_Roam);
    SITHAIFN_ADDINSTINCT(SITHAIFN_RETURNHOME, sithAIInstinct_ReturnHome);
    SITHAIFN_ADDINSTINCT(SITHAIFN_FLEE, sithAIInstinct_Flee);
    SITHAIFN_ADDINSTINCT(SITHAIFN_BASICFALLOW, sithAIInstinct_BasicFallow);
    SITHAIFN_ADDINSTINCT(SITHAIFN_WALLCRAWL, sithAIInstinct_WallCrawl);
    SITHAIFN_ADDINSTINCT(SITHAIFN_SNAKEFOLLOW, sithAIInstinct_SnakeFollow);
}

// Searches the seed of the rand stub whose first results fall into the buckets of pWant ('L' < 0.15, 'M' 0.4 to 0.6,
// 'H' > 0.85, 'm' 0.52 to 0.78, 'S' > 0.82, 'l' < 0.5, 'h' >= 0.5, '.' any; narrow windows: 'z' < 0.09, 'y' 0.11 to 0.14,
// 'c' 0.2 to 0.24, 'q' 0.22 to 0.28, 'd' 0.26 to 0.3, 'a' 0.42 to 0.48, 'b' 0.52 to 0.58, 'P' 0.62 to 0.68, 'A'/'B' around 0.39/0.41,
// 'C'/'D' around 0.49/0.51, 'E'/'F' around 0.79/0.81; the stub's results are (seed >> 16) & 0x7FFF)
static int sithAIFnVec_RndMatches(char bucket, int r)
{
    switch ( bucket )
    {
        case 'L':
            return r < 4900;
        case 'M':
            return r >= 13100 && r <= 19600;
        case 'm':
            return r >= 17000 && r <= 25500;
        case 'S':
            return r > 27000;
        case 'a':
            return r >= 13760 && r <= 15730;
        case 'b':
            return r >= 17040 && r <= 19000;
        case 'c':
            return r >= 6554 && r <= 7864;
        case 'd':
            return r >= 8520 && r <= 9830;
        case 'q':
            return r >= 7210 && r <= 9175;
        case 'z':
            return r < 2950;
        case 'y':
            return r >= 3600 && r <= 4590;
        case 'P':
            return r >= 20300 && r <= 22280;
        case 'A': // narrow windows around 0.39, 0.41, 0.49, 0.51, 0.79 and 0.81
            return r >= 12616 && r <= 12943;
        case 'B':
            return r >= 13271 && r <= 13598;
        case 'C':
            return r >= 15893 && r <= 16220;
        case 'D':
            return r >= 16548 && r <= 16876;
        case 'E':
            return r >= 25723 && r <= 26051;
        case 'F':
            return r >= 26378 && r <= 26706;
        case 'H':
            return r > 27850;
        case 'l':
            return r < 16384;
        case 'h':
            return r >= 16384;
        default:
            return 1;
    }
}

static uint32_t sithAIFnVec_FindSeed(const char* pWant)
{
    if ( !pWant )
    {
        return 1u;
    }

    for ( uint32_t seed = 1u; seed < 100000000u; ++seed )
    {
        uint32_t s = seed;
        int bMatch = 1;
        for ( const char* p = pWant; *p && bMatch; ++p )
        {
            s      = s * 214013u + 2531011u;
            bMatch = sithAIFnVec_RndMatches(*p, (int)((s >> 16) & 0x7FFFu));
        }

        if ( bMatch )
        {
            return seed;
        }
    }

    return 1u;
}

static void sithAIFnVec_Orient(SithThing* pThing, const float* pLook)
{
    rdVector3 look = { pLook[0], pLook[1], pLook[2] };
    sithVec_rdVector_Normalize3Acc(&look);
    rdVector3 z = { 0.0f, 0.0f, 1.0f };
    rdVector3 right;
    sithVec_Cross3(&right, &look, &z);
    sithVec_rdVector_Normalize3Acc(&right);
    rdVector3 up;
    sithVec_Cross3(&up, &right, &look);
    pThing->orient.lvec = look;
    pThing->orient.rvec = right;
    pThing->orient.uvec = up;
}

static void sithAIFnVec_BuildThing(unsigned role, const SithAIFnThing* pCfg)
{
    static const uint8_t aTypes[SITHAIFN_R_NUM] = { 0, SITH_THING_ACTOR, SITH_THING_PLAYER, SITH_THING_ACTOR, SITH_THING_ACTOR, SITH_THING_ACTOR, SITH_THING_ACTOR, SITH_THING_WEAPON, SITH_THING_ACTOR };
    unsigned idx          = role - 1u;
    SithThingType type    = (SithThingType)(pCfg->type == 0xFFu ? 0 : (pCfg->type ? pCfg->type : aTypes[role]));
    SithThingMoveType mt  = (SithThingMoveType)(pCfg->moveType == SITHAIFN_MT_NONE ? 0 : (pCfg->moveType ? pCfg->moveType : SITH_MT_PHYSICS));
    SithThing* pThing     = sithVec_NewThing(idx, type, mt, pCfg->sector);

    pThing->pos.x = (float)idx * 1.5f;
    pThing->pos.y = (float)idx * 0.25f;
    pThing->pos.z = 0.0f;
    if ( pCfg->bPos )
    {
        sithAIFnVec_SetVec(&pThing->pos, pCfg->pos);
    }

    if ( pCfg->bLook )
    {
        sithAIFnVec_Orient(pThing, pCfg->look);
    }

    sithAIFnVec_SetVec(&pThing->moveInfo.physics.velocity, pCfg->vel);
    pThing->flags                         = (SithThingFlag)pCfg->flags;
    pThing->moveInfo.physics.flags        = (SithPhysicsFlags)pCfg->physFlags;
    pThing->attach.flags                  = (SithAttachFlag)pCfg->attachFlags;
    pThing->collide.movesize              = pCfg->movesize != 0.0f ? pCfg->movesize : 0.25f;
    if ( type == SITH_THING_WEAPON )
    {
        pThing->thingInfo.weaponInfo.damageType = (SithDamageType)pCfg->damageType;
        pThing->thingInfo.weaponInfo.flags      = (SithWeaponFlag)pCfg->weaponFlags;
    }
    else
    {
        pThing->thingInfo.actorInfo.flags         = (SithActorFlag)pCfg->actorFlags;
        pThing->thingInfo.actorInfo.health        = pCfg->bHp ? pCfg->health : 100.0f;
        pThing->thingInfo.actorInfo.maxHealth     = pCfg->bHp ? pCfg->maxHealth : 100.0f;
        pThing->thingInfo.actorInfo.bForceMovePlay = pCfg->bForceMovePlay;
        pThing->thingInfo.actorInfo.eyeOffset.z   = pCfg->eyeZ;
        pThing->thingInfo.actorInfo.weaponInfo.curWeaponID = (SithWeaponId)pCfg->weaponId;
    }

    if ( pCfg->controlAI )
    {
        pThing->controlType                = SITH_CT_AI;
        pThing->controlInfo.aiControl.pLocal = &sithVec_arena.aiBlocks[1];
    }
}

static void sithAIFnVec_Build(const SithVecCase* pCase)
{
    const SithAIFnParams* pP = (const SithAIFnParams*)pCase->pParams;
    SithVecArena* a          = &sithVec_arena;
    SithAIFnUser* pUser      = SITHAIFN_USER;
    sithAIFnVec_AddFuncs();

    // Sectors
    a->sectors[0].flags = (SithSectorFlag)pP->sectorFlags[0];
    a->sectors[1].flags = (SithSectorFlag)pP->sectorFlags[1];

    // Things (roles 1..R_NUM - 1)
    for ( unsigned role = SITHAIFN_R_OWNER; role < SITHAIFN_R_NUM; ++role )
    {
        sithAIFnVec_BuildThing(role, &pP->thing[role]);
    }

    // The firer block (used by Dodge through controlInfo.aiControl.pLocal)
    for ( unsigned role = SITHAIFN_R_OWNER; role < SITHAIFN_R_NUM; ++role )
    {
        if ( pP->thing[role].controlAI )
        {
            memset(&a->aiBlocks[1], 0, sizeof(a->aiBlocks[1]));
            a->aiBlocks[1].pOwner       = SITHAIFN_ROLETHING(role);
            a->aiBlocks[1].pClass       = &a->aiClasses[0];
            a->aiBlocks[1].pTargetThing = SITHAIFN_ROLETHING(pP->thing[role].controlAI);
        }
    }

    // Class
    SithAIClass* pClass = &a->aiClasses[0];
    memset(pClass, 0, sizeof(*pClass));
    pClass->sightDistance = pP->cls[0] != 0.0f ? pP->cls[0] : 10.0f;
    pClass->heardDistance = pP->cls[1] != 0.0f ? pP->cls[1] : 8.0f;
    pClass->fov           = pP->cls[2] != 0.0f ? pP->cls[2] : -1.0f;
    pClass->rank          = pP->cls[3];

    // Control block
    SithAIControlBlock* pLocal = SITHAIFN_BLOCK;
    memset(pLocal, 0, sizeof(*pLocal));
    pLocal->pOwner       = SITHAIFN_ROLETHING(SITHAIFN_R_OWNER);
    pLocal->pClass       = pClass;
    pLocal->mode         = (SithAIMode)pP->mode;
    pLocal->submode      = (SithAISubMode)pP->submode;
    pLocal->pTargetThing = SITHAIFN_ROLETHING(pP->targetRole);
    pLocal->goalThing    = SITHAIFN_ROLETHING(pP->goalRole);
    pLocal->pFleeFromThing = SITHAIFN_ROLETHING(pP->fleeRole);
    sithAIFnVec_SetVec(&pLocal->toTarget, pP->lb.toTarget);
    sithAIFnVec_SetVec(&pLocal->toGoal, pP->lb.toGoal);
    sithAIFnVec_SetVec(&pLocal->lastGoalSeenPos, pP->lb.lastGoalSeenPos);
    sithAIFnVec_SetVec(&pLocal->lastTargetSeenPos, pP->lb.lastTargetSeenPos);
    sithAIFnVec_SetVec(&pLocal->homePos, pP->lb.homePos);
    sithAIFnVec_SetVec(&pLocal->homeOrient, pP->lb.homeOrient);
    sithAIFnVec_SetVec(&pLocal->moveDirection, pP->lb.moveDirection);
    sithAIFnVec_SetVec(&pLocal->movePos, pP->lb.movePos);
    sithAIFnVec_SetVec(&pLocal->moveStartPos, pP->lb.moveStartPos);
    sithAIFnVec_SetVec(&pLocal->alertPos, pP->lb.alertPos);
    pLocal->distance          = pP->lb.distance;
    pLocal->targetDistance    = pP->lb.targetDistance;
    pLocal->moveSpeed         = pP->lb.moveSpeed;
    pLocal->targetSightState  = (SithAISightState)pP->lb.targetSight;
    pLocal->targetSightState2 = (SithAISightState)pP->lb.targetSight2;
    pLocal->msecAttackStart   = pP->lb.msecAttackStart;

    // Instinct and its state
    memset(pUser, 0, sizeof(*pUser));
    for ( unsigned i = 0; i < 24; ++i )
    {
        pUser->instinct.fltArg[i] = pP->args[i];
        pUser->instinct.intArg[i] = (int)pP->args[i];
    }

    if ( pP->fn < SITHAIFN_NUMINSTINCTS )
    {
        pUser->instinct.pfInstinct = sithAIFnVec_apFuncs[pP->fn];
    }

    pUser->state.flags          = (SithAIInstinctFlag)pP->stFlags;
    pUser->state.msecNextUpdate = pP->stNext;
    for ( unsigned i = 0; i < 4; ++i )
    {
        pUser->state.aParams[i] = pP->st[i];
    }

    // Awareness sectors: the owner stands in sector 0
    for ( unsigned i = 0; i < 8; ++i )
    {
        pUser->aw[i].processID = 0;
    }

    SithAIAwarenessSector* pAw = &pUser->aw[pP->thing[SITHAIFN_R_OWNER].sector];
    for ( unsigned i = 0; i < 4; ++i )
    {
        pAw->aLevelAtTransmittingPos[i] = pP->aw.level[i];
        pAw->aTransmittingThing[i]      = SITHAIFN_ROLETHING(pP->aw.thing[i]);
        sithAIFnVec_SetVec(&pAw->aStartPos[i], pP->aw.start);
    }

    // Surfaces: the argument surface and the one the owner is attached to
    sithAIFnVec_SetVec(&pUser->surf[0].face.normal, pP->surfNormal);
    pUser->surf[0].flags = (SithSurfaceFlag)pP->surfFlags;
    pUser->surf[1].flags = (SithSurfaceFlag)pP->attachSurfFlags;
    SithThing* pOwner = SITHAIFN_ROLETHING(SITHAIFN_R_OWNER);
    if ( (pOwner->attach.flags & SITH_ATTACH_SURFACE) != 0 )
    {
        pOwner->attach.attachedToStructure.pSurfaceAttached = &pUser->surf[1];
    }

    // Quetz block of the snake
    if ( pP->quetz )
    {
        pUser->quetz.bPosFudged = pP->quetz == 2;
        pUser->quetz.posFudge   = pP->quetzFudge;
        pOwner->userblock.pQuetz = &pUser->quetz;
    }

    sithAIFnVec_SetVec(&pUser->vec[0], pP->vecA);
    sithAIFnVec_SetVec(&pUser->vec[1], pP->vecB);

    // Engine variables
    sithTime_g_msecGameTime = pP->bTime ? pP->msec : 100000u;
    sithTime_g_secGameTime  = pP->bTime ? pP->sec : 100.0f;
    sithMain_g_frameNumber  = 4242;
    memset(&sithMain_g_sith_mode, 0, sizeof(sithMain_g_sith_mode));
    sithMain_g_sith_mode.debugModeFlags = (SithDebugModeFlag)pP->debugFlags;
    sithPlayerControls_g_bCutsceneMode  = pP->cutscene;
    sithPlayer_g_pLocalPlayerThing      = pP->noPlayer ? NULL : SITHAIFN_ROLETHING(SITHAIFN_R_PLAYER);
    sithAIAwareness_g_aSectors          = pUser->aw;

    SITHAIFN_SNAKELASTDOT   = pP->bSnake ? pP->snake[0] : 1.0f;
    SITHAIFN_SNAKESLITHER   = pP->bSnake ? pP->snake[1] : 0.0f;
    SITHAIFN_SNAKENEARRANGE = pP->bSnake ? pP->snake[2] : 0.0f;

    // Stubs
    sithAIFnVec_cfg = pP->stubs;
    memset(&sithAIFnVec_observed, 0, sizeof(sithAIFnVec_observed));
    memset(&sithAIFnVec_ctr, 0, sizeof(sithAIFnVec_ctr));
    sithVec_stubFindSectorNullFrom = pP->stubs.nullSectorFrom;
    sithVec_stubRandSeed           = sithAIFnVec_FindSeed(pP->rnd);
}

// ---------------------------------------------------------------------------------------------------------------
// Run
// ---------------------------------------------------------------------------------------------------------------
static int sithAIFnVec_Run(SithVecCtx* pCtx)
{
    const SithAIFnParams* pP   = (const SithAIFnParams*)pCtx->pCase->pParams;
    SithAIControlBlock* pLocal = SITHAIFN_BLOCK;
    SithAIFnUser* pUser        = SITHAIFN_USER;
    void* pObj                 = NULL;
    if ( pP->obj == SITHAIFN_OBJ_SURFACE )
    {
        pObj = &pUser->surf[0];
    }
    else if ( pP->obj == SITHAIFN_OBJ_RAW )
    {
        pObj = (void*)(uintptr_t)pP->objVal;
    }
    else
    {
        pObj = SITHAIFN_ROLETHING(pP->obj);
    }

    sithVec_Begin(pCtx);

    unsigned numCalls = pP->calls ? pP->calls : 1u;
    for ( unsigned c = 0; c < numCalls; ++c )
    {
        if ( c )
        {
            sithTime_g_msecGameTime += pP->dtMs;
            sithTime_g_secGameTime += (float)pP->dtMs / 1000.0f;
        }

        int ret = 0;
        switch ( pP->fn )
        {
            case SITHAIFN_SNAKEFINDMOVEPOS:
                SITHVEC_FN(sithAIInstinct_SnakeFindMovePos)(pLocal, &pUser->vec[0], &pUser->vec[1]);
                break;

            case SITHAIFN_SNAKEMUNGETESTCHECK:
                ret = SITHVEC_FN(sithAIInstinct_SnakeMungeTestCheck)(pLocal, &pUser->vec[0], pP->fArg, &pUser->vec[2]);
                break;

            case SITHAIFN_SNAKEPLAYSLITHERSOUND:
                SITHVEC_FN(sithAIInstinct_SnakePlaySlitherSound)(SITHAIFN_ROLETHING(SITHAIFN_R_OWNER), pP->fArg);
                break;

            default:
                ret = sithAIFnVec_apFuncs[pP->fn](pLocal, &pUser->instinct, &pUser->state, (SithAIEventType)pP->event, pObj);
                break;
        }

        if ( !pP->noRet )
        {
            sithVec_Ev(SITHAIFN_EV_RET, 1, (uint32_t)ret);
        }

        sithVec_Checkpoint(pCtx);
    }

    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Cases (sithAIInstinctFnVecCases.inc, included twice: the parameters of every case, then the case table)
// ---------------------------------------------------------------------------------------------------------------
// Short names for the case file
#define FN(n)      .fn = SITHAIFN_##n
#define EV(n)      .event = SITHAI_EVENT_##n
#define M(n)       SITHAI_MODE_##n
#define SM(n)      SITHAI_SUBMODE_##n
#define ARGS(...)  .args = { __VA_ARGS__ }
#define ST(...)    .st = { __VA_ARGS__ }
#define CLS(...)   .cls = { __VA_ARGS__ }
#define LB(...)    .lb = { __VA_ARGS__ }
#define STUB(...)  .stubs = { __VA_ARGS__ }
#define TIME(m, s) .bTime = 1, .msec = (m), .sec = (s)
#define POS(x, y, z)  .bPos = 1, .pos = { x, y, z }
#define LOOK(x, y, z) .bLook = 1, .look = { x, y, z }
#define HP(h, m)   .bHp = 1, .health = (h), .maxHealth = (m)
#define TH(r, ...) .thing[SITHAIFN_R_##r] = { __VA_ARGS__ }
#define AW(l0, l1, l2, l3, t0, t1, t2, t3, sx, sy, sz) .aw = { .level = { l0, l1, l2, l3 }, .thing = { SITHAIFN_R_##t0, SITHAIFN_R_##t1, SITHAIFN_R_##t2, SITHAIFN_R_##t3 }, .start = { sx, sy, sz } }
#define OBJ(r)     .obj = SITHAIFN_R_##r
#define TARGET(r)  .targetRole = SITHAIFN_R_##r
#define GOAL(r)    .goalRole = SITHAIFN_R_##r
#define FLEEFROM(r) .fleeRole = SITHAIFN_R_##r
#define RAW(v)     .obj = SITHAIFN_OBJ_RAW, .objVal = (uint32_t)(v)
#define SURF       .obj = SITHAIFN_OBJ_SURFACE
#define SEQ        SITHAIFN_SEQ
#define ALL        SITHAIFN_ALL
#define T          SITHAIFN_T
#define UNDERWATER SITH_SECTOR_UNDERWATER
#define NONE       SITHAIFN_MT_NONE
#define FREE       0xFFu
#define VEH        SITH_PF_MINECAR
#define HUMAN      SITH_AF_HUMAN

#define CASE(id, ...) static const SithAIFnParams sithAIFnVec_p_##id = { __VA_ARGS__ };
#include "sithAIInstinctFnVecCases.inc"
#undef CASE

#define CASE(id, ...) { #id, sithAIFnVec_Run, 0, &sithAIFnVec_p_##id },
static const SithVecCase sithAIFnVec_aCases[] = {
#include "sithAIInstinctFnVecCases.inc"
};
#undef CASE

#undef FN
#undef EV
#undef M
#undef SM
#undef ARGS
#undef ST
#undef CLS
#undef LB
#undef STUB
#undef TIME
#undef POS
#undef LOOK
#undef HP
#undef TH
#undef AW
#undef OBJ
#undef TARGET
#undef GOAL
#undef FLEEFROM
#undef RAW
#undef SURF
#undef SEQ
#undef ALL
#undef T
#undef UNDERWATER
#undef NONE
#undef FREE
#undef VEH
#undef HUMAN

static const SithVecSuite sithAIFnVec_suite = {
    "sithAIInstinctFn",
    sithAIFnVec_aCases,
    SITHVEC_ARRAYLEN(sithAIFnVec_aCases),
    sithAIFnVec_Build,
    sithAIFnVec_Watch,
    sithAIFnVec_Sync,
};

#define SITHVEC_SUITE sithAIFnVec_suite

#endif // SITH_AI_TESTS_SITHAIINSTINCTFNVEC_H
