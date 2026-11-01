// Cases of the sithAIUtil vectors, shared by the capture harness and sithAIUtilVecTests. See
// tv/sithAIUtil/README.md.
//
// The cases build an AI actor (thing 0) with its control block and class, a target, a goal and a flee-from thing,
// waypoint ghosts and sectors in the shared arena, set the file statics of sithAIUtil.c (waypoint tables, point
// creation state, things-in-view state) and the engine globals, and call the sithAIUtil functions. The engine calls
// of the functions (collision searches, line of sight, weapon, cog, AI move and sound calls) are replaced by stubs
// that record their arguments and return what the case configured (collision searches pop scripted collisions).
#ifndef SITH_AI_TESTS_SITHAIUTILVEC_H
#define SITH_AI_TESTS_SITHAIUTILVEC_H

#include "sithVecWorld.h"

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithWeapon.h>

#include <rdroid/Engine/rdLight.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <std/General/stdMath.h>

#include "sithAIUtilVecStatics.h"

#include <limits.h>
#include <stddef.h>

// ---------------------------------------------------------------------------------------------------------------
// File statics of sithAIUtil.c
// ---------------------------------------------------------------------------------------------------------------
typedef struct
{
    SithAIWaypoint* aWpnts;
    SithAIWaypointOwner* aOwners;
    SithAIWaypointDistance* aDists;
    SithAIWaypointLayerFlag* pLayer;
    float* pSecLastUpdate;
    SithAIUtilMkPointFlags* pMkFlags;
    SithAIControlBlock** ppMkLocal;
    rdVector3* pMkDir;
    rdVector3* pMkPyr;
    rdVector3* pMkRVec;
    float* pFovX;
    float* pFovY;
    size_t* pNumVisited;
    size_t* pNumInView;
    size_t* pSizeInView;
    SithThing*** ppThingsInView;
    float* pMaxDist;
    int* pTypeMask;
} SithAIUtilVecStatics;

static SithAIUtilVecStatics sithAIUtilVec_st;

#ifdef SITHVEC_HARNESS
static void sithAIUtilVec_InitStatics(void)
{
    SithAIUtilVecStatics* s = &sithAIUtilVec_st;
    s->aWpnts         = (SithAIWaypoint*)sithAIUtil_aAIWpnts_ADDR;
    s->aOwners        = (SithAIWaypointOwner*)sithAIUtil_aWpntOwners_ADDR;
    s->aDists         = (SithAIWaypointDistance*)sithAIUtil_aWpntDistances_ADDR;
    s->pLayer         = (SithAIWaypointLayerFlag*)sithAIUtil_activeWpntLayer_ADDR;
    s->pSecLastUpdate = (float*)sithAIUtil_secLastUpdate_ADDR;
    s->pMkFlags       = (SithAIUtilMkPointFlags*)sithAIUtil_mkPointCurFlags_ADDR;
    s->ppMkLocal      = (SithAIControlBlock**)sithAIUtil_pMkPointCurLocal_ADDR;
    s->pMkDir         = (rdVector3*)sithAIUtil_mkPointCurDir_ADDR;
    s->pMkPyr         = (rdVector3*)sithAIUtil_mkPointCurPYR_ADDR;
    s->pMkRVec        = (rdVector3*)sithAIUtil_mkPointCurRVec_ADDR;
    s->pFovX          = (float*)sithAIUtil_thingsInViewFovX_ADDR;
    s->pFovY          = (float*)sithAIUtil_thingsInViewFovY_ADDR;
    s->pNumVisited    = (size_t*)sithAIUtil_numVisitedSectors_ADDR;
    s->pNumInView     = (size_t*)sithAIUtil_numThingsInView_ADDR;
    s->pSizeInView    = (size_t*)sithAIUtil_sizeThingsInView_ADDR;
    s->ppThingsInView = (SithThing***)sithAIUtil_aThingsInView_ADDR;
    s->pMaxDist       = (float*)sithAIUtil_maxDistanceToThingsInView_ADDR;
    s->pTypeMask      = (int*)sithAIUtil_thingInViewTypeMask_ADDR;
}
#else
void sithAIUtilVecTest_GetStatics(void** apOut);

static void sithAIUtilVec_InitStatics(void)
{
    void* apStatics[SITHAIUTILVEC_ST_COUNT];
    sithAIUtilVecTest_GetStatics(apStatics);

    SithAIUtilVecStatics* s = &sithAIUtilVec_st;
    s->aWpnts         = (SithAIWaypoint*)apStatics[SITHAIUTILVEC_ST_WPNTS];
    s->aOwners        = (SithAIWaypointOwner*)apStatics[SITHAIUTILVEC_ST_OWNERS];
    s->aDists         = (SithAIWaypointDistance*)apStatics[SITHAIUTILVEC_ST_DISTANCES];
    s->pLayer         = (SithAIWaypointLayerFlag*)apStatics[SITHAIUTILVEC_ST_LAYER];
    s->pSecLastUpdate = (float*)apStatics[SITHAIUTILVEC_ST_SECLASTUPDATE];
    s->pMkFlags       = (SithAIUtilMkPointFlags*)apStatics[SITHAIUTILVEC_ST_MKFLAGS];
    s->ppMkLocal      = (SithAIControlBlock**)apStatics[SITHAIUTILVEC_ST_MKLOCAL];
    s->pMkDir         = (rdVector3*)apStatics[SITHAIUTILVEC_ST_MKDIR];
    s->pMkPyr         = (rdVector3*)apStatics[SITHAIUTILVEC_ST_MKPYR];
    s->pMkRVec        = (rdVector3*)apStatics[SITHAIUTILVEC_ST_MKRVEC];
    s->pFovX          = (float*)apStatics[SITHAIUTILVEC_ST_FOVX];
    s->pFovY          = (float*)apStatics[SITHAIUTILVEC_ST_FOVY];
    s->pNumVisited    = (size_t*)apStatics[SITHAIUTILVEC_ST_NUMVISITED];
    s->pNumInView     = (size_t*)apStatics[SITHAIUTILVEC_ST_NUMINVIEW];
    s->pSizeInView    = (size_t*)apStatics[SITHAIUTILVEC_ST_SIZEINVIEW];
    s->ppThingsInView = (SithThing***)apStatics[SITHAIUTILVEC_ST_THINGSINVIEW];
    s->pMaxDist       = (float*)apStatics[SITHAIUTILVEC_ST_MAXDIST];
    s->pTypeMask      = (int*)apStatics[SITHAIUTILVEC_ST_TYPEMASK];
}
#endif

// Mirror of the statics and of the engine globals the functions write, watched with the arena
typedef struct
{
    SithAIWaypoint aWpnts[SITHAI_MAXWPNTS];
    SithAIWaypointOwner aOwners[SITHAI_MAXWPNTOWNERS];
    SithAIWaypointDistance aDists[SITHAI_MAXWPNTS];
    uint32_t layer;
    float secLastUpdate;
    uint32_t mkFlags;
    uint32_t pMkLocal;
    rdVector3 mkDir;
    rdVector3 mkPyr;
    rdVector3 mkRVec;
    float fovX;
    float fovY;
    uint32_t numVisited;
    uint32_t numInView;
    uint32_t sizeInView;
    uint32_t pThingsInView;
    float maxDist;
    int32_t typeMask;
    int32_t bRenderWpnts;
    uint32_t curRenderTick;
    uint32_t playerThing;
    uint32_t pad;
} SithAIUtilVecMirror;

static SithAIUtilVecMirror sithAIUtilVec_mirror;

static void sithAIUtilVec_Sync(void)
{
    SithAIUtilVecStatics* s     = &sithAIUtilVec_st;
    SithAIUtilVecMirror* m      = &sithAIUtilVec_mirror;
    memcpy(m->aWpnts, s->aWpnts, sizeof(m->aWpnts));
    memcpy(m->aOwners, s->aOwners, sizeof(m->aOwners));
    memcpy(m->aDists, s->aDists, sizeof(m->aDists));
    m->layer         = (uint32_t)*s->pLayer;
    m->secLastUpdate = *s->pSecLastUpdate;
    m->mkFlags       = (uint32_t)*s->pMkFlags;
    m->pMkLocal      = (uint32_t)SITHVEC_P(*s->ppMkLocal);
    m->mkDir         = *s->pMkDir;
    m->mkPyr         = *s->pMkPyr;
    m->mkRVec        = *s->pMkRVec;
    m->fovX          = *s->pFovX;
    m->fovY          = *s->pFovY;
    m->numVisited    = (uint32_t)*s->pNumVisited;
    m->numInView     = (uint32_t)*s->pNumInView;
    m->sizeInView    = (uint32_t)*s->pSizeInView;
    m->pThingsInView = (uint32_t)(uintptr_t)*s->ppThingsInView;
    m->maxDist       = *s->pMaxDist;
    m->typeMask      = *s->pTypeMask;
    m->bRenderWpnts  = sithAIUtil_g_bRenderAIWpnts;
    m->curRenderTick = (uint32_t)sithMain_g_curRenderTick;
    m->playerThing   = SITHVEC_P(sithPlayer_g_pLocalPlayerThing);
}

// ---------------------------------------------------------------------------------------------------------------
// Case configuration and scripted collision searches
// ---------------------------------------------------------------------------------------------------------------
#define SITHAIUTILVEC_NUMBLOCKS 8u
#define SITHAIUTILVEC_MAXCOLL   16u
#define SITHAIUTILVEC_MAXSEARCH 16u
#define SITHAIUTILVEC_GHOST0    10u // first waypoint ghost thing
#define SITHAIUTILVEC_NUMGHOSTS 14u

#define UV_BLOCK(i)  (&sithAI_g_aControlBlocks[i])
#define UV_THING(i)  (&sithVec_arena.things[i])
#define UV_SECTOR(i) (&sithVec_arena.sectors[i])
#define UV_CLASS(i)  (&sithVec_arena.aiClasses[i])
#define UV_OWNER     UV_THING(0)
#define UV_TARGET    UV_THING(1)
#define UV_GOAL      UV_THING(2)
#define UV_FLEE      UV_THING(3)
#define UV_LOCAL     UV_BLOCK(0)
#define UV_GHOST(k)  UV_THING(SITHAIUTILVEC_GHOST0 + (k))

// One scripted collision of a collision search
typedef struct
{
    uint32_t type;       // SithCollisionType
    int8_t thing;        // arena thing collided, -1: none
    uint32_t surfFlags;  // SithSurfaceFlag of the surface collided
    uint32_t adjFlags;   // flags of the adjoin of the surface, 0: no adjoin
    uint8_t adjSector;   // arena sector of the adjoin
    float dist;
    rdVector3 hitNorm;
    float faceNz;        // z of the surface face normal (x = sqrt(1 - z^2))
} SithAIUtilVecColl;

// Collisions returned by the collision searches of a case, in the order of the calls
typedef struct
{
    uint8_t numSearches;
    uint8_t aCount[SITHAIUTILVEC_MAXSEARCH];
    SithAIUtilVecColl aColl[SITHAIUTILVEC_MAXCOLL];
} SithAIUtilVecScript;

typedef struct
{
    // Results of the stubs
    uint32_t losMask;     // bit n: result of the n-th CheckLOS call (calls from the 32nd on use bit 31)
    uint32_t movePosMask; // bit n: result of the n-th sithAIMove_AISetMovePos call
    int hasWeapon;
    int isAiming;
    int isMounting;
    int hasInstinct;
    int bulletThing;      // arena thing returned by sithWeapon_WeaponFire, -1: NULL
    int invType;          // sithInventory_GetType: 0 NULL, 1 type without cog, 2 type with cog
    float weaponSize;
    float hitAccuracy;
    float combatDamage;
    float thingHeight;

    // Call counters
    unsigned int numLos;
    unsigned int numMovePos;
    unsigned int numSearches;
    unsigned int popBase;
    unsigned int popCount;
    unsigned int popPos;
    unsigned int nextColl;
    SithAIUtilVecScript script;
} SithAIUtilVecCfg;

static SithAIUtilVecCfg sithAIUtilVec_cfg;

// SITH_RAND of the original computes (float)rand() * (1.0f / RAND_MAX) in single precision, the reconstruction's macro
// (double)rand() * (1.0 / RAND_MAX) in double precision and rounds to float; the results differ by one ulp for some
// values. The rand values of a case that exposes the float itself are therefore taken from seeds whose numbers are exact
// in both forms (sithAIUtil_RandExact); the difference itself is covered by the cases with a raw seed.
static int sithAIUtilVec_RandExact(uint32_t value)
{
    volatile float single = (float)value * (1.0f / 32767.0f);
    volatile float widened = (float)((double)value * (1.0 / 32767.0));
    return single == widened;
}

// Seeds of the rand stub for the cases that depend on the random numbers: index 0..15 selects one of 16 seeds whose
// first three numbers cover every combination of "below 0.25" and "above 0.75" (the sequence of a small seed starts
// with tiny numbers), moved on to the next seed whose first 12 numbers are exact. An index from 16 on is a raw seed.
static uint32_t sithAIUtilVec_Seed(uint32_t index)
{
    static const uint32_t aSeeds[16] = {
        0xE9C329A8u, 0x8A04F74Fu, 0x4AD48431u, 0xA282BBFAu, 0xA091250Eu, 0xC555663Fu, 0x00DA5A80u, 0xCDF91845u,
        0x5E503A67u, 0x6980A81Fu, 0xD64A3CE0u, 0xA0CE08BBu, 0x141CB03Eu, 0xEBAA7701u, 0x94820A06u, 0x600027E8u,
    };

    if ( index >= 16u )
    {
        return index;
    }

    uint32_t seed = aSeeds[index];
    for ( ;; )
    {
        uint32_t state = seed;
        int bExact     = 1;
        for ( unsigned int i = 0; i < 12; ++i )
        {
            state = state * 214013u + 2531011u;
            bExact &= sithAIUtilVec_RandExact((state >> 16) & 0x7FFFu);
        }

        if ( bExact )
        {
            return seed;
        }

        ++seed;
    }
}

// Seed whose first random number is value (0..32767)
static uint32_t sithAIUtilVec_SeedForRand(uint32_t value)
{
    uint32_t inverse = 214013u; // modular inverse of the multiplier of the generator
    for ( unsigned int i = 0; i < 5; ++i )
    {
        inverse *= 2u - 214013u * inverse;
    }

    return ((value << 16) - 2531011u) * inverse;
}

// Storage the scripted collisions point to (not part of the compared state)
static SithCollision sithAIUtilVec_aCollisions[SITHAIUTILVEC_MAXCOLL];
static SithSurface sithAIUtilVec_aSurfaces[SITHAIUTILVEC_MAXCOLL];
static SithSurfaceAdjoin sithAIUtilVec_aAdjoins[SITHAIUTILVEC_MAXCOLL];
static SithInventoryType sithAIUtilVec_invType;
static SithThing* sithAIUtilVec_apViewResults[16]; // result array of GetThingsInView

enum
{
    SITHAIUTILVEC_EV_OUT = SITHVEC_EV_SUITE, // an output of a case function: tag, values
    SITHAIUTILVEC_EV_SEARCH,
    SITHAIUTILVEC_EV_POP,
    SITHAIUTILVEC_EV_DECSTACK,
    SITHAIUTILVEC_EV_LOS,
    SITHAIUTILVEC_EV_HASWEAPON,
    SITHAIUTILVEC_EV_WEAPONSIZE,
    SITHAIUTILVEC_EV_WEAPONFIRE,
    SITHAIUTILVEC_EV_ISAIMING,
    SITHAIUTILVEC_EV_ISMOUNTING,
    SITHAIUTILVEC_EV_AIEMIT,
    SITHAIUTILVEC_EV_HASINSTINCT,
    SITHAIUTILVEC_EV_COGMSGEX,
    SITHAIUTILVEC_EV_COGMSG,
    SITHAIUTILVEC_EV_COGSENDEX,
    SITHAIUTILVEC_EV_AISTOP,
    SITHAIUTILVEC_EV_SETMOVEPOS,
    SITHAIUTILVEC_EV_SETLOOKPOS,
    SITHAIUTILVEC_EV_QUETZ,
    SITHAIUTILVEC_EV_STOPMOVE,
    SITHAIUTILVEC_EV_SETMOVETARGET,
    SITHAIUTILVEC_EV_PLAYMODE,
    SITHAIUTILVEC_EV_PLAYSOUND,
    SITHAIUTILVEC_EV_HEADPYR,
    SITHAIUTILVEC_EV_LASER,
    SITHAIUTILVEC_EV_THINGHEIGHT,
    SITHAIUTILVEC_EV_APPLYFORCE,
    SITHAIUTILVEC_EV_INVTYPE,
    SITHAIUTILVEC_EV_HITACC,
    SITHAIUTILVEC_EV_COMBATDMG,
    SITHAIUTILVEC_EV_ADVANCETICK,
    SITHAIUTILVEC_EV_INTENSITY,
};

// Records an output of the called function that is not part of the watched memory (a local or an out parameter)
static void sithAIUtilVec_Out(uint32_t tag, uint32_t value)
{
    sithVec_Ev(SITHAIUTILVEC_EV_OUT, 2, tag, value);
}

static void sithAIUtilVec_OutF(uint32_t tag, float value)
{
    sithVec_Ev(SITHAIUTILVEC_EV_OUT, 2, tag, sithVec_FloatBits(value));
}

static void sithAIUtilVec_OutVec(uint32_t tag, const rdVector3* pVec)
{
    sithVec_Ev(SITHAIUTILVEC_EV_OUT, 1, tag);
    sithVec_EvWords(pVec, sizeof(*pVec));
}

// ---------------------------------------------------------------------------------------------------------------
// Stubs of the engine calls
// ---------------------------------------------------------------------------------------------------------------
SITHVEC_STUB float J3DAPI sithVec_sithCollision_SearchForCollisions(SithSector* pStartSector, SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, int searchFlags)
{
    SithAIUtilVecCfg* c = &sithAIUtilVec_cfg;
    sithVec_Ev(SITHAIUTILVEC_EV_SEARCH, 5, SITHVEC_P(pStartSector), SITHVEC_P(pThing), sithVec_FloatBits(moveDist), sithVec_FloatBits(radius), (uint32_t)searchFlags);
    sithVec_EvWords(startPos, sizeof(*startPos));
    sithVec_EvWords(moveNorm, sizeof(*moveNorm));

    c->popBase  = c->nextColl;
    c->popPos   = 0;
    c->popCount = c->numSearches < c->script.numSearches ? c->script.aCount[c->numSearches] : 0;
    c->nextColl += c->popCount;
    ++c->numSearches;
    return 0.0f;
}

SITHVEC_STUB SithCollision* sithVec_sithCollision_PopStack(void)
{
    SithAIUtilVecCfg* c = &sithAIUtilVec_cfg;
    if ( c->popPos < c->popCount )
    {
        unsigned int idx = c->popBase + c->popPos++;
        sithVec_Ev(SITHAIUTILVEC_EV_POP, 1, idx + 1u);
        return &sithAIUtilVec_aCollisions[idx];
    }

    sithVec_Ev(SITHAIUTILVEC_EV_POP, 1, 0u);
    return NULL;
}

SITHVEC_STUB void sithVec_sithCollision_DecreaseStackLevel(void)
{
    sithVec_Ev(SITHAIUTILVEC_EV_DECSTACK, 0);
}

SITHVEC_STUB int J3DAPI sithVec_sithCollision_CheckLOS(SithSector* pStartSector, const rdVector3* startPos, const rdVector3* endPos, float radius)
{
    SithAIUtilVecCfg* c = &sithAIUtilVec_cfg;
    int ret = (int)((c->losMask >> (c->numLos < 31u ? c->numLos : 31u)) & 1u);
    ++c->numLos;
    sithVec_Ev(SITHAIUTILVEC_EV_LOS, 3, SITHVEC_P(pStartSector), sithVec_FloatBits(radius), (uint32_t)ret);
    sithVec_EvWords(startPos, sizeof(*startPos));
    sithVec_EvWords(endPos, sizeof(*endPos));
    return ret;
}

SITHVEC_STUB int J3DAPI sithVec_sithWeapon_HasWeaponSelected(const SithThing* pThing)
{
    sithVec_Ev(SITHAIUTILVEC_EV_HASWEAPON, 1, SITHVEC_P(pThing));
    return sithAIUtilVec_cfg.hasWeapon;
}

SITHVEC_STUB float J3DAPI sithVec_sithWeapon_GetWeaponCollideSize(SithWeaponId typeId)
{
    sithVec_Ev(SITHAIUTILVEC_EV_WEAPONSIZE, 1, (uint32_t)typeId);
    return sithAIUtilVec_cfg.weaponSize;
}

SITHVEC_STUB SithThing* J3DAPI sithVec_sithWeapon_WeaponFire(SithThing* pShooter, const SithThing* pProjectileTemplate, const rdVector3* pFireDir, rdVector3* pFirePos, tSoundHandle hFireSnd, SithPuppetSubMode submode, float extra, SithFireProjectileFlags projectileFlags, float secDeltaTime)
{
    sithVec_Ev(SITHAIUTILVEC_EV_WEAPONFIRE, 7, SITHVEC_P(pShooter), SITHVEC_P(pProjectileTemplate), (uint32_t)hFireSnd, (uint32_t)submode, sithVec_FloatBits(extra), (uint32_t)projectileFlags, sithVec_FloatBits(secDeltaTime));
    sithVec_EvWords(pFireDir, sizeof(*pFireDir));
    sithVec_EvWords(pFirePos, sizeof(*pFirePos));
    return sithAIUtilVec_cfg.bulletThing >= 0 ? &sithVec_arena.things[sithAIUtilVec_cfg.bulletThing] : NULL;
}

SITHVEC_STUB int J3DAPI sithVec_sithWeapon_IsAiming(SithThing* pThing)
{
    sithVec_Ev(SITHAIUTILVEC_EV_ISAIMING, 1, SITHVEC_P(pThing));
    return sithAIUtilVec_cfg.isAiming;
}

SITHVEC_STUB int J3DAPI sithVec_sithWeapon_IsMountingWeapon(const SithThing* pThing)
{
    sithVec_Ev(SITHAIUTILVEC_EV_ISMOUNTING, 1, SITHVEC_P(pThing));
    return sithAIUtilVec_cfg.isMounting;
}

SITHVEC_STUB void J3DAPI sithVec_sithAI_EmitEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{
    sithVec_Ev(SITHAIUTILVEC_EV_AIEMIT, 3, SITHVEC_P(pLocal), (uint32_t)event, (uint32_t)(uintptr_t)pObject);
}

SITHVEC_STUB int J3DAPI sithVec_sithAI_HasInstinct(const SithAIControlBlock* pLocal, const char* pName)
{
    sithVec_Ev(SITHAIUTILVEC_EV_HASINSTINCT, 2, SITHVEC_P(pLocal), sithVec_Hash(pName));
    return sithAIUtilVec_cfg.hasInstinct;
}

SITHVEC_STUB int J3DAPI sithVec_sithCog_ThingSendMessageEx(const SithThing* pThing, const SithThing* pSrcThing, SithCogMsgType messageType, int param0, int param1, int param2, int param3)
{
    sithVec_Ev(SITHAIUTILVEC_EV_COGMSGEX, 7, SITHVEC_P(pThing), SITHVEC_P(pSrcThing), (uint32_t)messageType, (uint32_t)param0, (uint32_t)param1, (uint32_t)param2, (uint32_t)param3);
    return 0;
}

SITHVEC_STUB int J3DAPI sithVec_sithCog_ThingSendMessage(const SithThing* pThing, const SithThing* pSrcThing, SithCogMsgType msgType)
{
    sithVec_Ev(SITHAIUTILVEC_EV_COGMSG, 3, SITHVEC_P(pThing), SITHVEC_P(pSrcThing), (uint32_t)msgType);
    return 0;
}

SITHVEC_STUB int J3DAPI sithVec_sithCog_SendMessageEx(SithCog* pCog, SithCogMsgType messageType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int linkId, int param0, int param1, int param2, int param3)
{
    sithVec_Ev(SITHAIUTILVEC_EV_COGSENDEX, 11, (uint32_t)(uintptr_t)pCog, (uint32_t)messageType, (uint32_t)senderType, (uint32_t)senderIdx, (uint32_t)srcType, (uint32_t)srcIdx, (uint32_t)linkId, (uint32_t)param0, (uint32_t)param1, (uint32_t)param2, (uint32_t)param3);
    return 0;
}

SITHVEC_STUB void J3DAPI sithVec_sithAIMove_AIStop(SithAIControlBlock* pLocal)
{
    sithVec_Ev(SITHAIUTILVEC_EV_AISTOP, 1, SITHVEC_P(pLocal));
}

SITHVEC_STUB int J3DAPI sithVec_sithAIMove_AISetMovePos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed)
{
    SithAIUtilVecCfg* c = &sithAIUtilVec_cfg;
    int ret = (int)((c->movePosMask >> (c->numMovePos < 31u ? c->numMovePos : 31u)) & 1u);
    ++c->numMovePos;
    sithVec_Ev(SITHAIUTILVEC_EV_SETMOVEPOS, 3, SITHVEC_P(pLocal), sithVec_FloatBits(moveSpeed), (uint32_t)ret);
    sithVec_EvWords(moveToPos, sizeof(*moveToPos));
    return ret;
}

SITHVEC_STUB void J3DAPI sithVec_sithAIMove_AISetLookPosEyeLevel(SithAIControlBlock* pLocal, const rdVector3* targetPos)
{
    sithVec_Ev(SITHAIUTILVEC_EV_SETLOOKPOS, 1, SITHVEC_P(pLocal));
    sithVec_EvWords(targetPos, sizeof(*targetPos));
}

SITHVEC_STUB void J3DAPI sithVec_sithAIMove_StartQuetzStrike(SithAIControlBlock* pLocal)
{
    sithVec_Ev(SITHAIUTILVEC_EV_QUETZ, 1, SITHVEC_P(pLocal));
}

SITHVEC_STUB void J3DAPI sithVec_sithAIMove_StopAIMovement(SithAIControlBlock* pLocal)
{
    sithVec_Ev(SITHAIUTILVEC_EV_STOPMOVE, 1, SITHVEC_P(pLocal));
}

SITHVEC_STUB int J3DAPI sithVec_sithAIMove_AISetMoveTargetPos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed)
{
    sithVec_Ev(SITHAIUTILVEC_EV_SETMOVETARGET, 2, SITHVEC_P(pLocal), sithVec_FloatBits(moveSpeed));
    sithVec_EvWords(moveToPos, sizeof(*moveToPos));
    return 1;
}

SITHVEC_STUB int J3DAPI sithVec_sithPuppet_PlayMode(SithThing* pThing, SithPuppetSubMode submode, rdPuppetTrackCallback pfCallback)
{
    sithVec_Ev(SITHAIUTILVEC_EV_PLAYMODE, 3, SITHVEC_P(pThing), (uint32_t)submode, (uint32_t)(uintptr_t)pfCallback);
    return 0;
}

SITHVEC_STUB tSoundChannelHandle J3DAPI sithVec_sithSoundClass_PlayModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    sithVec_Ev(SITHAIUTILVEC_EV_PLAYSOUND, 2, SITHVEC_P(pThing), (uint32_t)mode);
    return 0;
}

SITHVEC_STUB void J3DAPI sithVec_sithActor_SetHeadPYR(SithThing* pThing, const rdVector3* headAngles)
{
    sithVec_Ev(SITHAIUTILVEC_EV_HEADPYR, 1, SITHVEC_P(pThing));
    sithVec_EvWords(headAngles, sizeof(*headAngles));
}

SITHVEC_STUB SithThing* J3DAPI sithVec_sithFX_CreateLaserThing(const SithThing* pSourceThing, const rdVector3* vecOffset, const rdVector3* vecEnd, float baseRadius, float tipRadius, float duration)
{
    sithVec_Ev(SITHAIUTILVEC_EV_LASER, 4, SITHVEC_P(pSourceThing), sithVec_FloatBits(baseRadius), sithVec_FloatBits(tipRadius), sithVec_FloatBits(duration));
    sithVec_EvWords(vecOffset, sizeof(*vecOffset));
    sithVec_EvWords(vecEnd, sizeof(*vecEnd));
    return NULL;
}

SITHVEC_STUB float J3DAPI sithVec_sithPhysics_GetThingHeight(const SithThing* pThing)
{
    sithVec_Ev(SITHAIUTILVEC_EV_THINGHEIGHT, 1, SITHVEC_P(pThing));
    return sithAIUtilVec_cfg.thingHeight;
}

SITHVEC_STUB void J3DAPI sithVec_sithPhysics_ApplyForce(SithThing* pThing, const rdVector3* force)
{
    sithVec_Ev(SITHAIUTILVEC_EV_APPLYFORCE, 1, SITHVEC_P(pThing));
    sithVec_EvWords(force, sizeof(*force));
}

SITHVEC_STUB SithInventoryType* J3DAPI sithVec_sithInventory_GetType(size_t typeId)
{
    sithVec_Ev(SITHAIUTILVEC_EV_INVTYPE, 1, (uint32_t)typeId);
    return sithAIUtilVec_cfg.invType ? &sithAIUtilVec_invType : NULL;
}

SITHVEC_STUB float sithVec_sithGetHitAccuarancyScalar(void)
{
    sithVec_Ev(SITHAIUTILVEC_EV_HITACC, 0);
    return sithAIUtilVec_cfg.hitAccuracy;
}

SITHVEC_STUB float sithVec_sithGetCombatDamageScalar(void)
{
    sithVec_Ev(SITHAIUTILVEC_EV_COMBATDMG, 0);
    return sithAIUtilVec_cfg.combatDamage;
}

SITHVEC_STUB void sithVec_sithAdvanceRenderTick(void)
{
    ++sithMain_g_curRenderTick;
    sithVec_Ev(SITHAIUTILVEC_EV_ADVANCETICK, 1, (uint32_t)sithMain_g_curRenderTick);
}

SITHVEC_STUB float J3DAPI sithVec_rdLight_GetIntensity(const rdVector4* pLight)
{
    float intensity = (pLight->red + pLight->green + pLight->blue) / 3.0f;
    sithVec_Ev(SITHAIUTILVEC_EV_INTENSITY, 1, sithVec_FloatBits(intensity));
    return intensity;
}

SITHVEC_STUB void J3DAPI sithVec_rdVector_Rotate3(rdVector3* vec, const rdVector3* pivot, const rdVector3* pyr)
{
    rdMatrix34 mat = { 0 };
    mat.rvec.x     = 1.0f;
    mat.lvec.y     = 1.0f;
    mat.uvec.z     = 1.0f;
    sithVec_rdMatrix_PreRotate34(&mat, pyr);

    rdVector3 src = *pivot;
    sithVec_rdMatrix_TransformVector34(vec, &src, &mat);
}

SITHVEC_STUB void J3DAPI sithVec_rdVector_Rotate3Acc(rdVector3* vec, const rdVector3* pyr)
{
    rdVector3 src = *vec;
    sithVec_rdVector_Rotate3(vec, &src, pyr);
}

// ---------------------------------------------------------------------------------------------------------------
// World of the cases
// ---------------------------------------------------------------------------------------------------------------
static void sithAIUtilVec_ResetTables(void)
{
    SithAIUtilVecStatics* s = &sithAIUtilVec_st;
    memset(s->aWpnts, 0, SITHAI_MAXWPNTS * sizeof(SithAIWaypoint));
    memset(s->aOwners, 0, SITHAI_MAXWPNTOWNERS * sizeof(SithAIWaypointOwner));
    memset(s->aDists, 0, SITHAI_MAXWPNTS * sizeof(SithAIWaypointDistance));
    for ( size_t i = 0; i < SITHAI_MAXWPNTS; ++i )
    {
        s->aWpnts[i].thingNum = -1;
        s->aWpnts[i].flags    = SITH_AIWPNT_LAYER0;
    }

    for ( size_t i = 0; i < SITHAI_MAXWPNTOWNERS; ++i )
    {
        s->aOwners[i].thingNum = -1;
    }

    *s->pLayer = SITH_AIWPNT_LAYER0;
}

static SithThing* sithAIUtilVec_Ghost(unsigned int k, float x, float y, float z, unsigned int sector)
{
    SithThing* pGhost = sithVec_NewThing(SITHAIUTILVEC_GHOST0 + k, SITH_THING_GHOST, SITH_MT_NONE, sector);
    pGhost->pos.x     = x;
    pGhost->pos.y     = y;
    pGhost->pos.z     = z;
    return pGhost;
}

// Waypoint k uses ghost k
static void sithAIUtilVec_SetWpnt(unsigned int k, SithAIWaypointLayerFlag flags, unsigned int rank)
{
    SithAIWaypoint* pWpnt = &sithAIUtilVec_st.aWpnts[k];
    pWpnt->thingNum       = (int)(SITHAIUTILVEC_GHOST0 + k);
    pWpnt->flags          = flags;
    pWpnt->rank           = rank;
}

static void sithAIUtilVec_Link(unsigned int a, unsigned int b)
{
    SithAIWaypoint* pWpnt = &sithAIUtilVec_st.aWpnts[a];
    pWpnt->aWpntLinks[pWpnt->numUsedLinks++] = (int)b;
}

static void sithAIUtilVec_Connect(unsigned int a, unsigned int b)
{
    sithAIUtilVec_Link(a, b);
    sithAIUtilVec_Link(b, a);
}

static void sithAIUtilVec_ResetCfg(void)
{
    SithAIUtilVecCfg* c = &sithAIUtilVec_cfg;
    memset(c, 0, sizeof(*c));
    c->hasWeapon   = 0;
    c->bulletThing = -1;
    c->weaponSize  = 0.5f;
    c->hitAccuracy = 1.0f;
    c->combatDamage = 1.0f;
    c->thingHeight = 1.0f;

    memset(sithAIUtilVec_aCollisions, 0, sizeof(sithAIUtilVec_aCollisions));
    memset(sithAIUtilVec_aSurfaces, 0, sizeof(sithAIUtilVec_aSurfaces));
    memset(sithAIUtilVec_aAdjoins, 0, sizeof(sithAIUtilVec_aAdjoins));
    memset(&sithAIUtilVec_invType, 0, sizeof(sithAIUtilVec_invType));
}

// Instantiates the script of the case: the collisions of the searches in the order of the calls
static void sithAIUtilVec_LoadScript(void)
{
    SithAIUtilVecScript* pScript = &sithAIUtilVec_cfg.script;
    unsigned int total           = 0;
    for ( unsigned int s = 0; s < pScript->numSearches; ++s )
    {
        total += pScript->aCount[s];
    }

    for ( unsigned int i = 0; i < total && i < SITHAIUTILVEC_MAXCOLL; ++i )
    {
        const SithAIUtilVecColl* pSpec = &pScript->aColl[i];
        SithCollision* pColl           = &sithAIUtilVec_aCollisions[i];
        SithSurface* pSurface          = &sithAIUtilVec_aSurfaces[i];
        pColl->type                    = (SithCollisionType)pSpec->type;
        pColl->pThingCollided          = pSpec->thing >= 0 ? &sithVec_arena.things[pSpec->thing] : NULL;
        pColl->pSurfaceCollided        = pSurface;
        pColl->hitNorm                 = pSpec->hitNorm;
        pColl->distance                = pSpec->dist;

        pSurface->flags = (SithSurfaceFlag)pSpec->surfFlags;
        pSurface->face.normal.x = sqrtf(1.0f - pSpec->faceNz * pSpec->faceNz);
        pSurface->face.normal.z = pSpec->faceNz;
        if ( pSpec->adjFlags )
        {
            SithSurfaceAdjoin* pAdjoin = &sithAIUtilVec_aAdjoins[i];
            pAdjoin->flags             = (SithSurfaceAdjoinFlag)pSpec->adjFlags;
            pAdjoin->pAdjoinSector     = &sithVec_arena.sectors[pSpec->adjSector];
            pSurface->pAdjoin          = pAdjoin;
        }
    }
}

// Scripted search helpers: a search returning the given collisions (up to 4), appended to the case script
typedef struct
{
    uint8_t num;
    SithAIUtilVecColl aColl[4];
} SithAIUtilVecSearch;

static void sithAIUtilVec_AddSearch(const SithAIUtilVecSearch* pSearch)
{
    SithAIUtilVecScript* pScript = &sithAIUtilVec_cfg.script;
    unsigned int first           = 0;
    for ( unsigned int s = 0; s < pScript->numSearches; ++s )
    {
        first += pScript->aCount[s];
    }

    pScript->aCount[pScript->numSearches++] = pSearch->num;
    for ( unsigned int i = 0; i < pSearch->num; ++i )
    {
        pScript->aColl[first + i] = pSearch->aColl[i];
    }
}

// Prepares the owner as a walking actor with its control block
static void sithAIUtilVec_Build(const SithVecCase* pCase)
{
    sithVec_stubRandSeed = sithVec_Hash(pCase->pName); // cases that depend on the random numbers set a seed of their own
    sithAIUtilVec_InitStatics();
    sithAIUtilVec_ResetCfg();

    SithAIUtilVecStatics* s = &sithAIUtilVec_st;
    sithAIUtilVec_ResetTables();
    memset(s->aDists, 0, SITHAI_MAXWPNTS * sizeof(SithAIWaypointDistance));
    *s->pSecLastUpdate = 0.0f;
    *s->pMkFlags       = 0;
    *s->ppMkLocal      = NULL;
    memset(s->pMkDir, 0, sizeof(rdVector3));
    memset(s->pMkPyr, 0, sizeof(rdVector3));
    memset(s->pMkRVec, 0, sizeof(rdVector3));
    *s->pFovX       = 0.0f;
    *s->pFovY       = 0.0f;
    *s->pNumVisited = 0;
    *s->pNumInView  = 0;
    *s->pSizeInView = 0;
    *s->ppThingsInView = NULL;
    *s->pMaxDist    = 0.0f;
    *s->pTypeMask   = 0;

    sithMain_g_frameNumber  = 1000;
    sithMain_g_curRenderTick = 77;
    memset(&sithMain_g_sith_mode, 0, sizeof(sithMain_g_sith_mode));
    sithTime_g_msecGameTime = 100000;
    sithTime_g_secGameTime  = 50.0f;
    sithTime_g_frameTimeFlex = 0.0625f;
    sithPlayer_g_pLocalPlayerThing = NULL;
    sithAI_g_lastUsedAIIndex = 0;
    sithAIUtil_g_bRenderAIWpnts = 0;

    // The AI control blocks are kept in the engine's array: the garbage fill, then block 0 of the owner
    for ( unsigned int i = 0; i < SITHAIUTILVEC_NUMBLOCKS; ++i )
    {
        memset(UV_BLOCK(i), (int)sithVec_fill, sizeof(*UV_BLOCK(i)));
    }

    sithVec_AddRegion(UV_BLOCK(0), SITHAIUTILVEC_NUMBLOCKS * sizeof(SithAIControlBlock));
    sithVec_AddRegion(sithAIUtilVec_apViewResults, sizeof(sithAIUtilVec_apViewResults));

    SithAIClass* pClass = UV_CLASS(0);
    memset(pClass, 0, sizeof(*pClass));
    pClass->num           = 1;
    pClass->maxStep       = 0.25f;
    pClass->sightDistance = 20.0f;
    pClass->heardDistance = 10.0f;
    pClass->fov           = 0.5f;
    pClass->accurancy     = 0.5f;
    pClass->degTurnAlign  = 30.0f;

    SithAIControlBlock* pLocal = UV_LOCAL;
    memset(pLocal, 0, sizeof(*pLocal));
    pLocal->pOwner = UV_OWNER;
    pLocal->pClass = pClass;

    SithThing* pOwner = sithVec_NewThing(0, SITH_THING_ACTOR, SITH_MT_PHYSICS, 0);
    pOwner->controlType = SITH_CT_AI;
    pOwner->collide.movesize = 0.25f;
    pOwner->collide.size     = 0.2f;
    pOwner->moveInfo.physics.mass = 2.0f;
    pOwner->controlInfo.aiControl.pClass = pClass;
    pOwner->controlInfo.aiControl.pLocal = pLocal;

    SithThing* pTarget = sithVec_NewThing(1, SITH_THING_ACTOR, SITH_MT_PHYSICS, 0);
    pTarget->pos.x     = 5.0f;
    pTarget->collide.size = 0.5f;

    SithThing* pGoal = sithVec_NewThing(2, SITH_THING_ACTOR, SITH_MT_PHYSICS, 1);
    pGoal->pos.x     = -4.0f;
    pGoal->pos.y     = 3.0f;
    pGoal->collide.size = 0.25f;

    SithThing* pFlee = sithVec_NewThing(3, SITH_THING_ACTOR, SITH_MT_PHYSICS, 0);
    pFlee->pos.x     = 2.0f;
    pFlee->pos.y     = 2.0f;
    pFlee->collide.size = 0.25f;
}

static void sithAIUtilVec_Watch(void)
{
    SITHVEC_WATCH(sithAIUtilVec_mirror);
    sithVec_Watch(UV_BLOCK(0), SITHAIUTILVEC_NUMBLOCKS * sizeof(SithAIControlBlock));
}

// ---------------------------------------------------------------------------------------------------------------
// Cases: small state functions
// ---------------------------------------------------------------------------------------------------------------
typedef struct
{
    int weaponNum;
    uint32_t initFlags;
} SithAIUtilVecWeaponFlags;

static int sithAIUtilVec_RunSetWeaponFireFlags(SithVecCtx* pCtx)
{
    const SithAIUtilVecWeaponFlags* p = (const SithAIUtilVecWeaponFlags*)pCtx->pCase->pParams;
    SithAIUtilFireFlags flags         = (SithAIUtilFireFlags)p->initFlags;
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_SetWeaponFireFlags)(p->weaponNum, &flags);
    sithAIUtilVec_Out(0, (uint32_t)flags);
    return 0;
}

// CanAttackTarget: pThing NULL or thing 1 with the given type, thing flags and actor flags
typedef struct
{
    int bNull;
    int type;
    uint32_t flags;
    uint32_t actorFlags;
} SithAIUtilVecCanAttack;

static int sithAIUtilVec_RunCanAttackTarget(SithVecCtx* pCtx)
{
    const SithAIUtilVecCanAttack* p = (const SithAIUtilVecCanAttack*)pCtx->pCase->pParams;
    SithThing* pThing               = UV_TARGET;
    pThing->type                    = (SithThingType)p->type;
    pThing->flags                   = (SithThingFlag)p->flags;
    pThing->thingInfo.actorInfo.flags = (SithActorFlag)p->actorFlags;
    sithVec_Begin(pCtx);
    return SITHVEC_FN(sithAIUtil_CanAttackTarget)(p->bNull ? NULL : pThing);
}

// IsThingMoving: thing 1 of the given type and move type
typedef struct
{
    int type;
    int moveType;
    int bHasLocal;
    uint32_t mode;
    rdVector3 vel;
    rdVector3 angVel;
    uint32_t pathMode;
    int bNull;
} SithAIUtilVecMoving;

static int sithAIUtilVec_RunIsThingMoving(SithVecCtx* pCtx)
{
    const SithAIUtilVecMoving* p = (const SithAIUtilVecMoving*)pCtx->pCase->pParams;
    SithThing* pThing            = UV_TARGET;
    pThing->type                 = (SithThingType)p->type;
    pThing->moveType             = (SithThingMoveType)p->moveType;
    if ( p->moveType == SITH_MT_PATH )
    {
        pThing->moveInfo.pathMovement.mode = (SithPathMoveMode)p->pathMode;
    }
    else
    {
        pThing->moveInfo.physics.velocity        = p->vel;
        pThing->moveInfo.physics.angularVelocity = p->angVel;
    }

    pThing->controlInfo.aiControl.pLocal = p->bHasLocal ? UV_LOCAL : NULL;
    UV_LOCAL->mode                       = (SithAIMode)p->mode;
    sithVec_Begin(pCtx);
    return SITHVEC_FN(sithAIUtil_IsThingMoving)(p->bNull ? NULL : pThing);
}

typedef struct
{
    uint32_t msecTime;
    int msecPause;
} SithAIUtilVecPause;

static int sithAIUtilVec_RunAIPauseMove(SithVecCtx* pCtx)
{
    const SithAIUtilVecPause* p = (const SithAIUtilVecPause*)pCtx->pCase->pParams;
    sithTime_g_msecGameTime     = p->msecTime;
    UV_LOCAL->msecPauseMoveUntil = 0x5A5A5A5Au;
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_AIPauseMove)(UV_LOCAL, p->msecPause);
    return 0;
}

// AISetMode and AIClearMode with the given initial mode and submode. owner: the owner is registered in the waypoint
// owner table (slot 2), with waypoint number 0x0305.
typedef struct
{
    uint32_t aiMode;
    uint32_t initMode;
    uint32_t initSubmode;
    int bOwnerRegistered;
} SithAIUtilVecMode;

static void sithAIUtilVec_SetupModeCase(const SithAIUtilVecMode* p)
{
    UV_LOCAL->mode    = (SithAIMode)p->initMode;
    UV_LOCAL->submode = (SithAISubMode)p->initSubmode;
    if ( p->bOwnerRegistered )
    {
        SithAIWaypointOwner* pOwner = &sithAIUtilVec_st.aOwners[2];
        pOwner->thingNum            = UV_OWNER->idx;
        pOwner->waypointNum         = 0x0305u;
        pOwner->cosTurnAlignAngle   = 0.5f;
    }
}

static int sithAIUtilVec_RunAISetMode(SithVecCtx* pCtx)
{
    const SithAIUtilVecMode* p = (const SithAIUtilVecMode*)pCtx->pCase->pParams;
    sithAIUtilVec_SetupModeCase(p);
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_AISetMode)(UV_LOCAL, (SithAIMode)p->aiMode);
    return 0;
}

static int sithAIUtilVec_RunAIClearMode(SithVecCtx* pCtx)
{
    const SithAIUtilVecMode* p = (const SithAIUtilVecMode*)pCtx->pCase->pParams;
    sithAIUtilVec_SetupModeCase(p);
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_AIClearMode)(UV_LOCAL, (SithAIMode)p->aiMode);
    return 0;
}

// AIGetMovePos: the flags select the reference position. Positions: owner (1,2,3), movePos (4,5,6), homePos (7,8,9),
// goal thing (-4,3,0), goalPos (10,11,12), flee-from thing (2,2,0) with velocity (1,0,0.5).
typedef struct
{
    uint32_t flags;
    int bGoalThing;
    int bFleeThing;
} SithAIUtilVecMovePos;

static int sithAIUtilVec_RunAIGetMovePos(SithVecCtx* pCtx)
{
    const SithAIUtilVecMovePos* p = (const SithAIUtilVecMovePos*)pCtx->pCase->pParams;
    UV_OWNER->pos.x               = 1.0f;
    UV_OWNER->pos.y               = 2.0f;
    UV_OWNER->pos.z               = 3.0f;
    UV_LOCAL->movePos.x           = 4.0f;
    UV_LOCAL->movePos.y           = 5.0f;
    UV_LOCAL->movePos.z           = 6.0f;
    UV_LOCAL->homePos.x           = 7.0f;
    UV_LOCAL->homePos.y           = 8.0f;
    UV_LOCAL->homePos.z           = 9.0f;
    UV_LOCAL->goalPos.x           = 10.0f;
    UV_LOCAL->goalPos.y           = 11.0f;
    UV_LOCAL->goalPos.z           = 12.0f;
    UV_LOCAL->goalThing           = p->bGoalThing ? UV_GOAL : NULL;
    UV_LOCAL->pFleeFromThing      = p->bFleeThing ? UV_FLEE : NULL;
    UV_FLEE->moveInfo.physics.velocity.x = 1.0f;
    UV_FLEE->moveInfo.physics.velocity.z = 0.5f;

    rdVector3 dest = { 99.0f, 98.0f, 97.0f };
    sithVec_Begin(pCtx);
    int ret = SITHVEC_FN(sithAIUtil_AIGetMovePos)(UV_LOCAL, (SithAIUtilMkPointFlags)p->flags, &dest);
    sithAIUtilVec_OutVec(0, &dest);
    return ret;
}

// Heading vectors of thing 1: velocity and orientation look vector, NULL arguments
typedef struct
{
    int bNullThing;
    int bNullDest;
    rdVector3 vel;
    rdVector3 lvec;
} SithAIUtilVecHeading;

static int sithAIUtilVec_RunHeading(SithVecCtx* pCtx)
{
    const SithAIUtilVecHeading* p = (const SithAIUtilVecHeading*)pCtx->pCase->pParams;
    SithThing* pThing             = UV_TARGET;
    pThing->moveInfo.physics.velocity = p->vel;
    pThing->orient.lvec               = p->lvec;
    rdVector3 dest                    = { 99.0f, 98.0f, 97.0f };
    sithVec_Begin(pCtx);
    int ret;
    if ( pCtx->pCase->var == 0 )
    {
        ret = SITHVEC_FN(sithAIUtil_GetXYHeadingVector)(p->bNullThing ? NULL : pThing, p->bNullDest ? NULL : &dest);
    }
    else
    {
        ret = SITHVEC_FN(sithAIUtil_GetXYZHeadingVector)(p->bNullThing ? NULL : pThing, p->bNullDest ? NULL : &dest);
    }

    sithAIUtilVec_OutVec(0, &dest);
    return ret;
}

typedef struct
{
    float mass;
    rdVector3 dir;
    float force;
} SithAIUtilVecForce;

static int sithAIUtilVec_RunApplyForce(SithVecCtx* pCtx)
{
    const SithAIUtilVecForce* p = (const SithAIUtilVecForce*)pCtx->pCase->pParams;
    UV_OWNER->moveInfo.physics.mass = p->mass;
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_ApplyForce)(UV_LOCAL, &p->dir, p->force);
    return 0;
}

typedef struct
{
    rdVector3 pyr;
    float scalar;
} SithAIUtilVecRotate;

static int sithAIUtilVec_RunRandomRotate(SithVecCtx* pCtx)
{
    const SithAIUtilVecRotate* p = (const SithAIUtilVecRotate*)pCtx->pCase->pParams;
    rdVector3 pyr                = p->pyr;
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_RandomRotate)(&pyr, p->scalar);
    sithAIUtilVec_OutVec(0, &pyr);
    return 0;
}

// AIPlaySoundMode: a NULL control block, a block without owner, and modes around the talk range
typedef struct
{
    int bNullLocal;
    int bNullOwner;
    int hasInstinct;
    int mode;
} SithAIUtilVecSound;

static int sithAIUtilVec_RunAIPlaySoundMode(SithVecCtx* pCtx)
{
    const SithAIUtilVecSound* p = (const SithAIUtilVecSound*)pCtx->pCase->pParams;
    sithAIUtilVec_cfg.hasInstinct = p->hasInstinct;
    if ( p->bNullOwner )
    {
        UV_LOCAL->pOwner = NULL;
    }

    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_AIPlaySoundMode)(p->bNullLocal ? NULL : UV_LOCAL, (SithSoundClassMode)p->mode);
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Cases: waypoint tables
// ---------------------------------------------------------------------------------------------------------------

// Waypoint graphs. Waypoint k uses ghost k at (3k, 1.5 * (k & 1), 0); all are in layer 0 with rank 0 unless noted.
enum
{
    SITHAIUTILVEC_GRAPH_NONE,     // no waypoints
    SITHAIUTILVEC_GRAPH_CHAIN,    // 0-1-2-3-4-5
    SITHAIUTILVEC_GRAPH_DISABLED, // chain, waypoint 2 disabled
    SITHAIUTILVEC_GRAPH_OTHERLAYER, // chain, waypoint 3 only in layer 1
    SITHAIUTILVEC_GRAPH_STAR,     // hub 0 linked with 1..5
    SITHAIUTILVEC_GRAPH_RING,     // ring 0-1-2-3-0 and tail 3-4-5
    SITHAIUTILVEC_GRAPH_SPLIT,    // 0-1-2 and 3-4-5
    SITHAIUTILVEC_GRAPH_HIGHRANK, // chain, waypoint 2 already holds the rank 250
    SITHAIUTILVEC_GRAPH_ONEWAY,   // chain 0>1>2>3 (links in one direction only), 4 and 5 unlinked
};

static void sithAIUtilVec_Graph(int graph)
{
    if ( graph == SITHAIUTILVEC_GRAPH_NONE )
    {
        return;
    }

    for ( unsigned int k = 0; k < 6; ++k )
    {
        sithAIUtilVec_Ghost(k, 3.0f * (float)k, 1.5f * (float)(k & 1u), 0.0f, k % 3u);
        sithAIUtilVec_SetWpnt(k, SITH_AIWPNT_LAYER0, 0);
    }

    switch ( graph )
    {
        case SITHAIUTILVEC_GRAPH_STAR:
            for ( unsigned int k = 1; k < 6; ++k )
            {
                sithAIUtilVec_Connect(0, k);
            }
            break;

        case SITHAIUTILVEC_GRAPH_RING:
            sithAIUtilVec_Connect(0, 1);
            sithAIUtilVec_Connect(1, 2);
            sithAIUtilVec_Connect(2, 3);
            sithAIUtilVec_Connect(3, 0);
            sithAIUtilVec_Connect(3, 4);
            sithAIUtilVec_Connect(4, 5);
            break;

        case SITHAIUTILVEC_GRAPH_SPLIT:
            sithAIUtilVec_Connect(0, 1);
            sithAIUtilVec_Connect(1, 2);
            sithAIUtilVec_Connect(3, 4);
            sithAIUtilVec_Connect(4, 5);
            break;

        case SITHAIUTILVEC_GRAPH_ONEWAY:
            sithAIUtilVec_Link(0, 1);
            sithAIUtilVec_Link(1, 2);
            sithAIUtilVec_Link(2, 3);
            break;

        default:
            for ( unsigned int k = 0; k < 5; ++k )
            {
                sithAIUtilVec_Connect(k, k + 1);
            }
            break;
    }

    if ( graph == SITHAIUTILVEC_GRAPH_DISABLED )
    {
        sithAIUtilVec_st.aWpnts[2].flags = SITH_AIWPNT_LAYER0 | SITH_AIWPNT_DISABLED;
    }
    else if ( graph == SITHAIUTILVEC_GRAPH_OTHERLAYER )
    {
        sithAIUtilVec_st.aWpnts[3].flags = SITH_AIWPNT_LAYER1;
    }
    else if ( graph == SITHAIUTILVEC_GRAPH_HIGHRANK )
    {
        sithAIUtilVec_st.aWpnts[2].rank = 250;
    }
}

static void sithAIUtilVec_Dirty(void* p, size_t size)
{
    uint32_t* pWords = (uint32_t*)p;
    for ( size_t i = 0; i < size / 4; ++i )
    {
        pWords[i] = sithVec_Rnd();
    }
}

static int sithAIUtilVec_RunResetWaypoints(SithVecCtx* pCtx)
{
    SithAIUtilVecStatics* s = &sithAIUtilVec_st;
    sithAIUtilVec_Dirty(s->aWpnts, SITHAI_MAXWPNTS * sizeof(SithAIWaypoint));
    sithAIUtilVec_Dirty(s->aOwners, SITHAI_MAXWPNTOWNERS * sizeof(SithAIWaypointOwner));
    *s->pLayer                  = (SithAIWaypointLayerFlag)0x2A5A;
    sithAIUtil_g_bRenderAIWpnts = pCtx->pCase->var;
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_AIResetWaypoints)();
    return 0;
}

// AISetWpnt: wpntIdx; thing: 0 ghost 11, 1 actor, 2 NULL, 3 ghost 10, 4 a free thing
typedef struct
{
    unsigned int wpntIdx;
    int thing;
} SithAIUtilVecSetWpnt;

static int sithAIUtilVec_RunAISetWpnt(SithVecCtx* pCtx)
{
    const SithAIUtilVecSetWpnt* p = (const SithAIUtilVecSetWpnt*)pCtx->pCase->pParams;
    sithAIUtilVec_Ghost(0, 1.0f, 0.0f, 0.0f, 0);
    sithAIUtilVec_Ghost(1, 2.0f, 0.0f, 0.0f, 0);
    SithAIWaypoint* pWpnt = &sithAIUtilVec_st.aWpnts[p->wpntIdx < SITHAI_MAXWPNTS ? p->wpntIdx : 0];
    pWpnt->thingNum       = 99;
    pWpnt->flags          = (SithAIWaypointLayerFlag)0x3333;
    pWpnt->numUsedLinks   = 5;
    pWpnt->rank           = 7;
    pWpnt->aWpntLinks[0]  = 4;

    SithThing* apThings[] = { UV_GHOST(1), UV_TARGET, NULL, UV_GHOST(0), UV_THING(30) };
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_AISetWpnt)(apThings[p->thing], p->wpntIdx);
    return 0;
}

typedef struct
{
    unsigned int wpntIdx;
    uint32_t value; // rank or flags
} SithAIUtilVecWpntValue;

static int sithAIUtilVec_RunWpntValue(SithVecCtx* pCtx)
{
    const SithAIUtilVecWpntValue* p = (const SithAIUtilVecWpntValue*)pCtx->pCase->pParams;
    SithAIWaypoint* pWpnt           = &sithAIUtilVec_st.aWpnts[p->wpntIdx < SITHAI_MAXWPNTS ? p->wpntIdx : 0];
    pWpnt->thingNum                 = 12;
    pWpnt->flags                    = (SithAIWaypointLayerFlag)0x12F0AA55u;
    sithVec_Begin(pCtx);
    switch ( pCtx->pCase->var )
    {
        case 0:
            SITHVEC_FN(sithAIUtil_AISetWpntRank)(p->wpntIdx, p->value);
            break;

        case 1:
            SITHVEC_FN(sithAIUtil_AISetWpntFlags)(p->wpntIdx, (SithAIWaypointLayerFlag)p->value);
            break;

        default:
            SITHVEC_FN(sithAIUtil_AIClearWpntFlags)(p->wpntIdx, (SithAIWaypointLayerFlag)p->value);
            break;
    }

    return 0;
}

static int sithAIUtilVec_RunSetActiveLayer(SithVecCtx* pCtx)
{
    const uint32_t* pLayer = (const uint32_t*)pCtx->pCase->pParams;
    *sithAIUtilVec_st.pLayer = (SithAIWaypointLayerFlag)0x4000;
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_AISetActiveWpntLayer)((SithAIWaypointLayerFlag)*pLayer);
    return 0;
}

// AIConnectWpnts: waypoints 0..5 are valid (waypoint 6 and up have no thing); fullA and fullB: the waypoint already
// has all 8 links
typedef struct
{
    unsigned int a;
    unsigned int b;
    int both;
    int fullA;
    int fullB;
} SithAIUtilVecConnect;

static int sithAIUtilVec_RunAIConnectWpnts(SithVecCtx* pCtx)
{
    const SithAIUtilVecConnect* p = (const SithAIUtilVecConnect*)pCtx->pCase->pParams;
    sithAIUtilVec_Graph(SITHAIUTILVEC_GRAPH_CHAIN);
    for ( unsigned int k = 0; k < 2; ++k )
    {
        unsigned int idx = k == 0 ? p->a : p->b;
        if ( (k == 0 ? p->fullA : p->fullB) && idx < SITHAI_MAXWPNTS )
        {
            sithAIUtilVec_st.aWpnts[idx].numUsedLinks = 8;
            for ( unsigned int j = 0; j < 8; ++j )
            {
                sithAIUtilVec_st.aWpnts[idx].aWpntLinks[j] = (int)(j % 6);
            }
        }
    }

    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_AIConnectWpnts)(p->a, p->b, p->both);
    return 0;
}

// Waypoint owner table layouts of CreateAIWaypoint, ClearAIWaypoint and GetAIWaypointNum. The owner (thing 0) is
// "own", free slots hold thingNum -1, other slots those of other AIs.
typedef struct
{
    uint16_t occupied;  // bit i: slot i holds another AI
    int8_t ownSlot;     // slot of the owner, -1: none
    float degTurn;
} SithAIUtilVecOwners;

static int sithAIUtilVec_RunOwners(SithVecCtx* pCtx)
{
    const SithAIUtilVecOwners* p = (const SithAIUtilVecOwners*)pCtx->pCase->pParams;
    for ( unsigned int i = 0; i < SITHAI_MAXWPNTOWNERS; ++i )
    {
        SithAIWaypointOwner* pOwner = &sithAIUtilVec_st.aOwners[i];
        if ( p->occupied & (1u << i) )
        {
            pOwner->thingNum          = 20 + (int)i;
            pOwner->waypointNum       = 0x0102u + i;
            pOwner->cosTurnAlignAngle = 0.25f * (float)i;
        }
    }

    if ( p->ownSlot >= 0 )
    {
        SithAIWaypointOwner* pOwner = &sithAIUtilVec_st.aOwners[p->ownSlot];
        pOwner->thingNum            = UV_OWNER->idx;
        pOwner->waypointNum         = 0x0706u;
        pOwner->cosTurnAlignAngle   = 0.75f;
    }

    sithVec_Begin(pCtx);
    int ret;
    switch ( pCtx->pCase->var )
    {
        case 0:
            ret = SITHVEC_FN(sithAIUtil_CreateAIWaypoint)(UV_LOCAL, p->degTurn);
            break;

        case 1:
            SITHVEC_FN(sithAIUtil_ClearAIWaypoint)(UV_LOCAL);
            ret = 0;
            break;

        default:
            ret = SITHVEC_FN(sithAIUtil_GetAIWaypointNum)(UV_LOCAL);
            break;
    }

    return ret;
}

typedef struct
{
    int bNull1;
    int bNull2;
    float dist1;
    float dist2;
} SithAIUtilVecCompare;

static int sithAIUtilVec_RunCompare(SithVecCtx* pCtx)
{
    const SithAIUtilVecCompare* p = (const SithAIUtilVecCompare*)pCtx->pCase->pParams;
    SithAIWaypointDistance d1     = { 1, p->dist1 };
    SithAIWaypointDistance d2     = { 2, p->dist2 };
    sithVec_Begin(pCtx);
    return SITHVEC_FN(sithAIUtil_CompareAIWpntDistances)(p->bNull1 ? NULL : &d1, p->bNull2 ? NULL : &d2);
}

// AIGetWpntThing: waypoint 3 holds thing number thingNum
static int sithAIUtilVec_RunAIGetWpntThing(SithVecCtx* pCtx)
{
    const int* pThingNum = (const int*)pCtx->pCase->pParams;
    sithAIUtilVec_st.aWpnts[3].thingNum = *pThingNum;
    sithVec_Begin(pCtx);
    SithThing* pThing = SITHVEC_FN(sithAIUtil_AIGetWpntThing)(3);
    return (int)SITHVEC_P(pThing);
}

// RankWpntsFromThing and RankWpntsFromPlayer. thing: 0 = ghost of waypoint 0, 1 = ghost of waypoint 2, 2 = ghost
// of waypoint 5, 3 = a thing that is no waypoint (actor), 4 = thing 0 (owner)
typedef struct
{
    int graph;
    int thing;
    uint32_t layer;     // active layer, 0: layer 0
    uint32_t playerFlags;
    int player;         // RankWpntsFromPlayer: 0 NULL player, 1 player thing 1, 2 invisible player, 3 player at a waypoint
    float px, py, pz;   // player position
} SithAIUtilVecRank;

static void sithAIUtilVec_DirtyRanks(void)
{
    for ( unsigned int k = 0; k < SITHAI_MAXWPNTS; ++k )
    {
        sithAIUtilVec_st.aWpnts[k].rank = (size_t)(k * 7u % 40u);
    }
}

static int sithAIUtilVec_RunRankFromThing(SithVecCtx* pCtx)
{
    const SithAIUtilVecRank* p = (const SithAIUtilVecRank*)pCtx->pCase->pParams;
    sithAIUtilVec_Graph(p->graph);
    sithAIUtilVec_DirtyRanks();
    if ( p->layer )
    {
        *sithAIUtilVec_st.pLayer = (SithAIWaypointLayerFlag)p->layer;
    }

    SithThing* apThings[] = { UV_GHOST(0), UV_GHOST(2), UV_GHOST(5), UV_TARGET, UV_OWNER };
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_RankWpntsFromThing)(apThings[p->thing]);
    return 0;
}

static int sithAIUtilVec_RunRankFromPlayer(SithVecCtx* pCtx)
{
    const SithAIUtilVecRank* p = (const SithAIUtilVecRank*)pCtx->pCase->pParams;
    sithAIUtilVec_Graph(p->graph);
    sithAIUtilVec_DirtyRanks();
    if ( p->layer )
    {
        *sithAIUtilVec_st.pLayer = (SithAIWaypointLayerFlag)p->layer;
    }

    if ( p->player )
    {
        SithThing* pPlayer = UV_TARGET;
        pPlayer->type      = SITH_THING_PLAYER;
        pPlayer->pos.x     = p->px;
        pPlayer->pos.y     = p->py;
        pPlayer->pos.z     = p->pz;
        pPlayer->thingInfo.actorInfo.flags = (SithActorFlag)p->playerFlags;
        pPlayer->collide.movesize          = 0.25f;
        sithPlayer_g_pLocalPlayerThing     = pPlayer;
    }

    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_RankWpntsFromPlayer)();
    return 0;
}

// SelectWpntLink on a star graph (hub 0, leaves 1..5; waypoint k is at (3k, 1.5 * (k & 1), 0)). The low byte of the
// flags is the "priority" of a waypoint (rank mask), the rank field the flee/hunt rank.
typedef struct
{
    uint32_t mode;       // mode of the control block
    int flee;            // flee-from thing: 0 none, 1 actor (thing 3), 2 ghost of waypoint 4
    int bTarget;         // target thing set (thing 1)
    int player;          // local player thing set (thing 1, at 0,0,0): the ranking from the player runs
    uint8_t aPriority[6];
    uint8_t aRank[6];
    uint8_t disabled;    // bit k: waypoint k disabled
    uint8_t otherLayer;  // bit k: waypoint k only in layer 1
    uint8_t numLinks;
    int8_t aLinks[6];
    int linkNum;
    int ownerPrev;       // -1: owner number -1; otherwise the owner is registered with this previous waypoint
    float ownerX, ownerY;
    float fleeX, fleeY;
    uint32_t seed;
} SithAIUtilVecSelect;

static int sithAIUtilVec_RunSelectWpntLink(SithVecCtx* pCtx)
{
    const SithAIUtilVecSelect* p = (const SithAIUtilVecSelect*)pCtx->pCase->pParams;
    sithAIUtilVec_Graph(SITHAIUTILVEC_GRAPH_STAR);
    for ( unsigned int k = 0; k < 6; ++k )
    {
        SithAIWaypoint* pWpnt = &sithAIUtilVec_st.aWpnts[k];
        pWpnt->flags          = (SithAIWaypointLayerFlag)(((p->otherLayer >> k) & 1u ? SITH_AIWPNT_LAYER1 : SITH_AIWPNT_LAYER0) | p->aPriority[k] | ((p->disabled >> k) & 1u ? SITH_AIWPNT_DISABLED : 0));
        pWpnt->rank           = p->aRank[k];
    }

    UV_LOCAL->mode = (SithAIMode)p->mode;
    sithVec_stubRandSeed = sithAIUtilVec_Seed(p->seed);
    UV_OWNER->pos.x = p->ownerX;
    UV_OWNER->pos.y = p->ownerY;
    UV_FLEE->pos.x  = p->fleeX;
    UV_FLEE->pos.y  = p->fleeY;
    if ( p->flee == 1 )
    {
        UV_LOCAL->pFleeFromThing = UV_FLEE;
    }
    else if ( p->flee == 2 )
    {
        UV_LOCAL->pFleeFromThing = UV_GHOST(4);
    }

    if ( p->bTarget )
    {
        UV_LOCAL->pTargetThing = UV_TARGET;
    }

    if ( p->player )
    {
        UV_TARGET->type = SITH_THING_PLAYER;
        UV_TARGET->pos  = (rdVector3){ 0.0f, 0.0f, 0.0f };
        UV_TARGET->collide.movesize = 0.25f;
        sithPlayer_g_pLocalPlayerThing = UV_TARGET;
    }

    int ownerNum = -1;
    if ( p->ownerPrev >= 0 )
    {
        ownerNum = 4;
        sithAIUtilVec_st.aOwners[4].thingNum    = UV_OWNER->idx;
        sithAIUtilVec_st.aOwners[4].waypointNum = (unsigned int)p->ownerPrev | 0x0300u;
    }

    int aLinks[6];
    for ( unsigned int i = 0; i < 6; ++i )
    {
        aLinks[i] = p->aLinks[i];
    }

    sithVec_Begin(pCtx);
    int var = pCtx->pCase->var;
    if ( var == 0 )
    {
        return SITHVEC_FN(sithAIUtil_SelectWpntLink)(UV_LOCAL, p->numLinks, aLinks, p->linkNum, ownerNum);
    }

    // Sweeps: 1 the first random number, 2 the priority and 3 the rank of the second candidate (the first one holds
    // 15). The results show where the decision changes.
    SithAIWaypoint* pLead = &sithAIUtilVec_st.aWpnts[aLinks[p->linkNum]];
    SithAIWaypoint* pNext = &sithAIUtilVec_st.aWpnts[aLinks[p->linkNum + 1 < p->numLinks ? p->linkNum + 1 : p->linkNum]];
    if ( var == 2 )
    {
        pLead->flags = (SithAIWaypointLayerFlag)((pLead->flags & ~SITHAI_WPNT_RANKMASK) | 15u);
    }
    else if ( var == 3 )
    {
        pLead->rank = 15;
    }

    for ( unsigned int k = 0; k < (var == 1 ? 16u : 31u); ++k )
    {
        if ( var == 1 )
        {
            sithVec_stubRandSeed = sithAIUtilVec_SeedForRand(k * 2184u);
        }
        else if ( var == 2 )
        {
            pNext->flags = (SithAIWaypointLayerFlag)((pNext->flags & ~SITHAI_WPNT_RANKMASK) | k);
        }
        else
        {
            pNext->rank = k;
        }

        int ret = SITHVEC_FN(sithAIUtil_SelectWpntLink)(UV_LOCAL, p->numLinks, aLinks, p->linkNum, ownerNum);
        sithAIUtilVec_Out(k, (uint32_t)ret);
    }

    return 0;
}

// AIFindNearestWpnt
typedef struct
{
    int graph;
    int who;                // 0: the owner (thing 0), 1: the local player (thing 1), 2: thing 2 as a non-AI actor
    float x, y, z;          // position of the thing
    int bCheckPath;
    uint32_t layer;         // active layer, 0: layer 0
    uint32_t mode;          // mode of the control block
    uint32_t damage;        // bit 0: waypoint 1 holds thing 45, 1: waypoint 2 holds the actor, 2: waypoint 0 has no thing
    float movesize;
    const SithAIUtilVecSearch* pSearches;
    uint8_t numSearches;
} SithAIUtilVecNearest;

static void sithAIUtilVec_LoadSearches(const SithAIUtilVecSearch* pSearches, unsigned int num)
{
    for ( unsigned int i = 0; i < num; ++i )
    {
        sithAIUtilVec_AddSearch(&pSearches[i]);
    }

    sithAIUtilVec_LoadScript();
}

static int sithAIUtilVec_RunFindNearest(SithVecCtx* pCtx)
{
    const SithAIUtilVecNearest* p = (const SithAIUtilVecNearest*)pCtx->pCase->pParams;
    sithAIUtilVec_Graph(p->graph);
    if ( p->layer )
    {
        *sithAIUtilVec_st.pLayer = (SithAIWaypointLayerFlag)p->layer;
    }

    if ( p->graph != SITHAIUTILVEC_GRAPH_NONE )
    {
        if ( p->damage & 1u )
        {
            sithAIUtilVec_st.aWpnts[1].thingNum = 45;
        }

        if ( p->damage & 2u )
        {
            sithAIUtilVec_st.aWpnts[2].thingNum = 1;
        }

        if ( p->damage & 4u )
        {
            sithAIUtilVec_st.aWpnts[0].thingNum = -1;
        }
    }

    SithThing* pThing = UV_OWNER;
    if ( p->who == 1 )
    {
        pThing = UV_TARGET;
        pThing->type = SITH_THING_PLAYER;
        sithPlayer_g_pLocalPlayerThing = pThing;
    }
    else if ( p->who == 2 )
    {
        pThing = UV_GOAL;
    }

    pThing->pos.x           = p->x;
    pThing->pos.y           = p->y;
    pThing->pos.z           = p->z;
    pThing->collide.movesize = p->movesize;
    UV_LOCAL->mode          = (SithAIMode)p->mode;
    sithAIUtilVec_LoadSearches(p->pSearches, p->numSearches);

    sithVec_Begin(pCtx);
    return SITHVEC_FN(sithAIUtil_AIFindNearestWpnt)(pThing, p->bCheckPath);
}

// AIMoveToNextWpnt, AITraverseWpnts, AIAdvanceToNextWpnt
typedef struct
{
    int graph;
    float x, y, z;           // owner position
    uint32_t mode, submode;  // control block
    int wpntIdx;             // AITraverseWpnts: waypoint
    float moveSpeed;
    float degTurn;
    int modeArg;
    uint32_t movePosMask;
    int ownerSlot;           // owner registered at this slot of the owner table, -1: not
    uint32_t ownerWpntNum;   // its waypointNum
    uint16_t othersFull;     // owner table slots held by other AIs
    uint32_t layer;
    float classDegTurn;
    int bNotify;
    uint32_t damage;         // bit 0: waypoint 1 holds thing 45, bit 1: waypoint 2 holds the actor, bit 2: no world
    const SithAIUtilVecSearch* pSearches;
    uint8_t numSearches;
} SithAIUtilVecWpntMove;

static int sithAIUtilVec_RunWpntMove(SithVecCtx* pCtx)
{
    const SithAIUtilVecWpntMove* p = (const SithAIUtilVecWpntMove*)pCtx->pCase->pParams;
    sithAIUtilVec_Graph(p->graph);
    if ( p->layer )
    {
        *sithAIUtilVec_st.pLayer = (SithAIWaypointLayerFlag)p->layer;
    }

    if ( p->graph != SITHAIUTILVEC_GRAPH_NONE )
    {
        if ( p->damage & 1u )
        {
            sithAIUtilVec_st.aWpnts[1].thingNum = 45;
        }

        if ( p->damage & 2u )
        {
            sithAIUtilVec_st.aWpnts[2].thingNum = 1;
        }
    }

    for ( unsigned int i = 0; i < SITHAI_MAXWPNTOWNERS; ++i )
    {
        if ( p->othersFull & (1u << i) )
        {
            sithAIUtilVec_st.aOwners[i].thingNum    = 20 + (int)i;
            sithAIUtilVec_st.aOwners[i].waypointNum = 0x0102u;
        }
    }

    if ( p->ownerSlot >= 0 )
    {
        sithAIUtilVec_st.aOwners[p->ownerSlot].thingNum          = UV_OWNER->idx;
        sithAIUtilVec_st.aOwners[p->ownerSlot].waypointNum       = p->ownerWpntNum;
        sithAIUtilVec_st.aOwners[p->ownerSlot].cosTurnAlignAngle = 0.5f;
    }

    UV_OWNER->pos.x          = p->x;
    UV_OWNER->pos.y          = p->y;
    UV_OWNER->pos.z          = p->z;
    UV_LOCAL->mode           = (SithAIMode)p->mode;
    UV_LOCAL->submode        = (SithAISubMode)p->submode;
    UV_LOCAL->moveSpeed      = 1.5f;
    UV_CLASS(0)->degTurnAlign = p->classDegTurn;
    sithAIUtilVec_cfg.movePosMask = p->movePosMask;
    sithAIUtilVec_LoadSearches(p->pSearches, p->numSearches);

    SithWorld* pSavedWorld = sithWorld_g_pCurrentWorld;
    if ( p->damage & 4u )
    {
        sithWorld_g_pCurrentWorld = NULL;
    }

    sithVec_Begin(pCtx);
    int ret = 0;
    switch ( pCtx->pCase->var )
    {
        case 0:
            ret = SITHVEC_FN(sithAIUtil_AIMoveToNextWpnt)(UV_LOCAL, p->moveSpeed, p->degTurn, p->modeArg);
            break;

        case 1:
            SITHVEC_FN(sithAIUtil_AITraverseWpnts)(UV_LOCAL, p->wpntIdx, p->moveSpeed, p->degTurn, p->modeArg);
            break;

        default:
            ret = SITHVEC_FN(sithAIUtil_AIAdvanceToNextWpnt)(UV_LOCAL, p->bNotify);
            break;
    }

    sithWorld_g_pCurrentWorld = pSavedWorld;
    return ret;
}

// CheckWpntMoveAlign: the owner is registered in slot 6 with the given turn cosine
typedef struct
{
    int bRegistered;
    uint32_t mode;
    uint32_t submode;
    float cosAngle;
    rdVector3 goalLVec;
    rdVector3 lvec;
    rdVector3 uvec;
} SithAIUtilVecAlign;

static int sithAIUtilVec_RunCheckWpntMoveAlign(SithVecCtx* pCtx)
{
    const SithAIUtilVecAlign* p = (const SithAIUtilVecAlign*)pCtx->pCase->pParams;
    if ( p->bRegistered )
    {
        sithAIUtilVec_st.aOwners[6].thingNum          = UV_OWNER->idx;
        sithAIUtilVec_st.aOwners[6].waypointNum       = 0x0102u;
        sithAIUtilVec_st.aOwners[6].cosTurnAlignAngle = p->cosAngle;
    }

    UV_LOCAL->mode                   = (SithAIMode)p->mode;
    UV_LOCAL->submode                = (SithAISubMode)p->submode;
    UV_LOCAL->goalLVec               = p->goalLVec;
    UV_OWNER->orient.lvec            = p->lvec;
    UV_OWNER->orient.uvec            = p->uvec;
    UV_OWNER->moveInfo.physics.velocity.x = 1.0f;
    UV_OWNER->moveInfo.physics.velocity.y = 2.0f;
    UV_OWNER->moveInfo.physics.velocity.z = 3.0f;
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_CheckWpntMoveAlign)(UV_LOCAL);
    return 0;
}

// RenderAIWaypoints
typedef struct
{
    int bRender;
    float secGameTime;
    float secLastUpdate;
    int graph;
    uint32_t layer;
} SithAIUtilVecRender;

static int sithAIUtilVec_RunRenderWaypoints(SithVecCtx* pCtx)
{
    const SithAIUtilVecRender* p = (const SithAIUtilVecRender*)pCtx->pCase->pParams;
    sithAIUtilVec_Graph(p->graph);
    if ( p->layer )
    {
        *sithAIUtilVec_st.pLayer = (SithAIWaypointLayerFlag)p->layer;
    }

    sithAIUtil_g_bRenderAIWpnts        = p->bRender;
    sithTime_g_secGameTime             = p->secGameTime;
    *sithAIUtilVec_st.pSecLastUpdate   = p->secLastUpdate;
    sithVec_Begin(pCtx);
    SITHVEC_FN(sithAIUtil_RenderAIWaypoints)();
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Cases: sight
// ---------------------------------------------------------------------------------------------------------------

// GetDistanceToTarget (var 0) and GetDistanceToTargetPos (var 1). The viewer is thing 0 at (0,0,0) looking along +y
// (right vector +x), the target thing 1.
typedef struct
{
    uint32_t viewerActorFlags;
    uint32_t viewerThingFlags;
    int targetType;
    uint32_t targetThingFlags;
    uint32_t targetActorFlags;
    uint32_t targetPhysFlags;
    int targetMoveType;
    rdVector3 targetPos;
    float targetSize;
    float fov;
    float sight;
    uint32_t losMask;
    rdVector3 startPos;
} SithAIUtilVecDist;

static void sithAIUtilVec_SetupTarget(const SithAIUtilVecDist* p)
{
    UV_OWNER->thingInfo.actorInfo.flags = (SithActorFlag)p->viewerActorFlags;
    UV_OWNER->flags                     = (SithThingFlag)p->viewerThingFlags;
    SithThing* pTarget                  = UV_TARGET;
    pTarget->type                       = (SithThingType)p->targetType;
    pTarget->flags                      = (SithThingFlag)p->targetThingFlags;
    pTarget->thingInfo.actorInfo.flags  = (SithActorFlag)p->targetActorFlags;
    pTarget->moveType                   = (SithThingMoveType)p->targetMoveType;
    pTarget->moveInfo.physics.flags     = (SithPhysicsFlags)p->targetPhysFlags;
    pTarget->pos                        = p->targetPos;
    pTarget->collide.size               = p->targetSize;
    sithAIUtilVec_cfg.losMask           = p->losMask;
}

static const float sithAIUtilVec_aSweepSin[24] = { 0.0f, 0.258819044f, 0.5f, 0.707106769f, 0.866025388f, 0.965925813f, 1.0f, 0.965925813f, 0.866025388f, 0.707106769f, 0.5f, 0.258819044f, 0.0f, -0.258819044f, -0.5f, -0.707106769f, -0.866025388f, -0.965925813f, -1.0f, -0.965925813f, -0.866025388f, -0.707106769f, -0.5f, -0.258819044f };
static const float sithAIUtilVec_aSweepCos[24] = { 1.0f, 0.965925813f, 0.866025388f, 0.707106769f, 0.5f, 0.258819044f, 0.0f, -0.258819044f, -0.5f, -0.707106769f, -0.866025388f, -0.965925813f, -1.0f, -0.965925813f, -0.866025388f, -0.707106769f, -0.5f, -0.258819044f, 0.0f, 0.258819044f, 0.5f, 0.707106769f, 0.866025388f, 0.965925813f };

// var 0: GetDistanceToTarget, 1: GetDistanceToTargetPos; 2 and 3: the target runs around the viewer in 15 degree steps
// (2: thing, 3: position); 4 and 5: the target distance runs from 17 to 22.75 (4: thing, 5: position)
static int sithAIUtilVec_RunDistance(SithVecCtx* pCtx)
{
    const SithAIUtilVecDist* p = (const SithAIUtilVecDist*)pCtx->pCase->pParams;
    sithAIUtilVec_SetupTarget(p);

    int var            = pCtx->pCase->var;
    rdVector3 startPos = p->startPos;
    rdVector3 toTarget = { 91.0f, 92.0f, 93.0f };
    float dist         = 94.0f;
    sithVec_Begin(pCtx);

    unsigned int numSteps = var < 2 ? 1u : 24u;
    int ret               = 0;
    for ( unsigned int k = 0; k < numSteps; ++k )
    {
        if ( var == 2 || var == 3 )
        {
            UV_TARGET->pos.x = 5.0f * sithAIUtilVec_aSweepSin[k];
            UV_TARGET->pos.y = 5.0f * sithAIUtilVec_aSweepCos[k];
            UV_TARGET->pos.z = p->targetPos.z;
        }
        else if ( var >= 4 )
        {
            UV_TARGET->pos.x = p->targetPos.x;
            UV_TARGET->pos.y = 17.0f + 0.25f * (float)k;
            UV_TARGET->pos.z = p->targetPos.z;
        }

        toTarget = (rdVector3){ 91.0f, 92.0f, 93.0f };
        dist     = 94.0f;
        if ( (var & 1) == 0 )
        {
            ret = SITHVEC_FN(sithAIUtil_GetDistanceToTarget)(UV_OWNER, &startPos, UV_TARGET, p->fov, p->sight, 0.25f, &toTarget, &dist);
        }
        else
        {
            ret = SITHVEC_FN(sithAIUtil_GetDistanceToTargetPos)(UV_OWNER, &startPos, &UV_TARGET->pos, p->fov, p->sight, 0.25f, &toTarget, &dist);
        }

        if ( numSteps > 1 )
        {
            sithAIUtilVec_Out(100u + k, (uint32_t)ret);
        }

        sithAIUtilVec_OutF(0, dist);
    }

    sithAIUtilVec_OutVec(1, &toTarget);
    return ret;
}

// CanSeeTarget
typedef struct
{
    int bNullTarget;
    int targetType;
    float distance;
    uint32_t mode;
    uint32_t ownerActorFlags;
    uint32_t targetActorFlags;
    int unknownFlags;
    float ambient;             // ambient light (all colors) of the sector of the target
    int moveType;
    uint32_t physFlags;
    int moving;
    uint32_t seed;
} SithAIUtilVecSee;

static int sithAIUtilVec_RunCanSeeTarget(SithVecCtx* pCtx)
{
    const SithAIUtilVecSee* p = (const SithAIUtilVecSee*)pCtx->pCase->pParams;
    SithThing* pTarget        = UV_TARGET;
    pTarget->type             = (SithThingType)p->targetType;
    pTarget->thingInfo.actorInfo.flags = (SithActorFlag)p->targetActorFlags;
    pTarget->unknownFlags              = p->unknownFlags;
    pTarget->moveType                  = (SithThingMoveType)p->moveType;
    pTarget->moveInfo.physics.flags    = (SithPhysicsFlags)p->physFlags;
    pTarget->moveInfo.physics.velocity.x = p->moving ? 1.0f : 0.0f;
    UV_SECTOR(0)->ambientLight.red     = p->ambient;
    UV_SECTOR(0)->ambientLight.green   = p->ambient;
    UV_SECTOR(0)->ambientLight.blue    = p->ambient;
    UV_OWNER->thingInfo.actorInfo.flags = (SithActorFlag)p->ownerActorFlags;
    UV_LOCAL->mode                     = (SithAIMode)p->mode;
    sithVec_stubRandSeed               = sithAIUtilVec_Seed(p->seed);
    sithVec_Begin(pCtx);
    if ( pCtx->pCase->var == 1 )
    {
        // The first random number runs through the range, the results show the chance of being seen
        for ( uint32_t value = 0; value <= 32767u; value += 128u )
        {
            sithVec_stubRandSeed = sithAIUtilVec_SeedForRand(value);
            int ret              = SITHVEC_FN(sithAIUtil_CanSeeTarget)(UV_LOCAL, p->bNullTarget ? NULL : pTarget, p->distance);
            sithAIUtilVec_Out(value, (uint32_t)ret);
        }

        return 0;
    }

    return SITHVEC_FN(sithAIUtil_CanSeeTarget)(UV_LOCAL, p->bNullTarget ? NULL : pTarget, p->distance);
}

// UpdateTargetSight (var 0) and UpdateGoalSight (var 1)
typedef struct
{
    uint32_t fireFlags;
    int sameFrame;            // the sight frame of the block is the current frame
    int hasWeapon;
    int weaponTemplate;       // the owner has a weapon template (collision size 0.75)
    uint32_t submode;
    uint32_t mode;
    uint32_t ownerActorFlags;
    rdVector3 eyeOffset;
    rdVector3 fireOffset;
    int targetKind;           // 0 no target thing (target position), 1 target thing actor, 2 invisible actor, 3 non-actor
    int prevSight;            // previous sight state
    rdVector3 targetPos;
    uint32_t losMask;
    float fov;
    float sight;
    uint32_t seed;
    float ambient;
} SithAIUtilVecUpdate;

static int sithAIUtilVec_RunUpdateSight(SithVecCtx* pCtx)
{
    const SithAIUtilVecUpdate* p = (const SithAIUtilVecUpdate*)pCtx->pCase->pParams;
    SithAIControlBlock* pLocal   = UV_LOCAL;
    UV_OWNER->thingInfo.actorInfo.flags      = (SithActorFlag)p->ownerActorFlags;
    UV_OWNER->thingInfo.actorInfo.eyeOffset  = p->eyeOffset;
    UV_OWNER->thingInfo.actorInfo.fireOffset = p->fireOffset;
    UV_OWNER->thingInfo.actorInfo.weaponInfo.curWeaponID = 3;
    if ( p->weaponTemplate )
    {
        SithThing* pTemplate = &sithVec_arena.templates[0];
        memset(pTemplate, 0, sizeof(*pTemplate));
        pTemplate->collide.movesize = 0.75f;
        UV_OWNER->thingInfo.actorInfo.pWeaponTemplate = pTemplate;
    }

    pLocal->submode       = (SithAISubMode)p->submode;
    pLocal->mode          = (SithAIMode)p->mode;
    pLocal->pClass->fov   = p->fov;
    pLocal->pClass->sightDistance = p->sight;
    sithAIUtilVec_cfg.hasWeapon   = p->hasWeapon;
    sithAIUtilVec_cfg.weaponSize  = 0.5f;
    sithAIUtilVec_cfg.losMask     = p->losMask;
    sithVec_stubRandSeed          = sithAIUtilVec_Seed(p->seed);
    UV_SECTOR(0)->ambientLight.red   = p->ambient;
    UV_SECTOR(0)->ambientLight.green = p->ambient;
    UV_SECTOR(0)->ambientLight.blue  = p->ambient;
    UV_SECTOR(1)->ambientLight.red   = p->ambient;
    UV_SECTOR(1)->ambientLight.green = p->ambient;
    UV_SECTOR(1)->ambientLight.blue  = p->ambient;

    SithThing* pThing = p->targetKind == 3 ? UV_THING(4) : UV_TARGET;
    if ( p->targetKind == 3 )
    {
        sithVec_NewThing(4, SITH_THING_ITEM, SITH_MT_NONE, 0);
    }

    pThing->pos = p->targetPos;
    if ( p->targetKind == 2 )
    {
        pThing->thingInfo.actorInfo.flags = SITH_AF_INVISIBLE;
    }

    if ( pCtx->pCase->var == 0 )
    {
        pLocal->targetSightFrame = p->sameFrame ? (int)sithMain_g_frameNumber : 7;
        pLocal->targetSightState = (SithAISightState)p->prevSight;
        pLocal->pTargetThing     = p->targetKind ? pThing : NULL;
        pLocal->targetPos        = p->targetPos;
        pLocal->toTarget.x       = 0.5f;
        pLocal->distance         = 123.0f;
    }
    else
    {
        pLocal->goalSightFrame    = p->sameFrame ? (int)sithMain_g_frameNumber : 7;
        pLocal->targetSightState2 = (SithAISightState)p->prevSight;
        pLocal->goalThing         = p->targetKind ? pThing : NULL;
        pLocal->movePos           = p->targetPos;
    }

    sithVec_Begin(pCtx);
    if ( pCtx->pCase->var == 0 )
    {
        SITHVEC_FN(sithAIUtil_UpdateTargetSight)(pLocal, (SithAIUtilFireFlags)p->fireFlags);
    }
    else
    {
        SITHVEC_FN(sithAIUtil_UpdateGoalSight)(pLocal);
    }

    return 0;
}

// IsLineOfFireClear. Blocks 1 and 2 are other AI actors (things 4 and 5).
typedef struct
{
    int bNullLocal;
    int bNullOwner;
    int ownerControlType;
    int hasWeapon;
    int bTarget;
    int lastUsed;
    struct
    {
        int bOwner;              // block has an owner
        int controlType;
        uint32_t actorFlags;
        uint32_t mode;
        rdVector3 toTarget;
        float distance;
    } aOthers[3];                // blocks 1, 2, 3
    rdVector3 pos;
    int selfHuman;           // the owner itself is a human with a visible target
} SithAIUtilVecLineOfFire;

static int sithAIUtilVec_RunIsLineOfFireClear(SithVecCtx* pCtx)
{
    const SithAIUtilVecLineOfFire* p = (const SithAIUtilVecLineOfFire*)pCtx->pCase->pParams;
    UV_OWNER->controlType            = (SithControlType)p->ownerControlType;
    sithAIUtilVec_cfg.hasWeapon      = p->hasWeapon;
    UV_LOCAL->pTargetThing           = p->bTarget ? UV_TARGET : NULL;
    UV_LOCAL->pOwner                 = p->bNullOwner ? NULL : UV_OWNER;
    sithAI_g_lastUsedAIIndex         = p->lastUsed;
    for ( unsigned int i = 0; i < 3; ++i )
    {
        SithAIControlBlock* pBlock = UV_BLOCK(i + 1);
        memset(pBlock, 0, sizeof(*pBlock));
        if ( p->aOthers[i].bOwner )
        {
            SithThing* pThing = sithVec_NewThing(4 + i, SITH_THING_ACTOR, SITH_MT_PHYSICS, 0);
            pThing->controlType = (SithControlType)p->aOthers[i].controlType;
            pThing->thingInfo.actorInfo.flags = (SithActorFlag)p->aOthers[i].actorFlags;
            pBlock->pOwner = pThing;
        }

        pBlock->mode     = (SithAIMode)p->aOthers[i].mode;
        pBlock->toTarget = p->aOthers[i].toTarget;
        pBlock->distance = p->aOthers[i].distance;
    }

    if ( p->selfHuman )
    {
        UV_OWNER->thingInfo.actorInfo.flags = SITH_AF_HUMAN;
        UV_LOCAL->mode                      = SITHAI_MODE_TARGETVISIBLE;
        UV_LOCAL->toTarget                  = (rdVector3){ 1.0f, 0.0f, 0.0f };
        UV_LOCAL->distance                  = 100.0f;
    }

    UV_TARGET->pos.x = 8.0f;
    rdVector3 pos    = p->pos;
    sithVec_Begin(pCtx);
    return SITHVEC_FN(sithAIUtil_IsLineOfFireClear)(p->bNullLocal ? NULL : UV_LOCAL, &pos);
}

// ---------------------------------------------------------------------------------------------------------------
// Cases: AIFire
// ---------------------------------------------------------------------------------------------------------------

// The owner (thing 0) at the origin looking along +y fires at the target (thing 1). With sameFrame and fire flags
// without LOB/PRIMARY/ALT the sight of the block is used as it is (toTarget, distance, sight state); otherwise the
// sight is updated (line of sight stub, losMask).
typedef struct
{
    uint32_t debugFlags;
    uint32_t ownerThingFlags;
    uint32_t sectorFlags;
    uint32_t ownerActorFlags;
    int bTarget;
    int bTemplate;            // the owner has a weapon template (shot speed templateSpeed)
    int curWeaponId;
    float templateSpeed;
    uint32_t mode;
    uint32_t submode;
    uint32_t msecFireWaitTime;
    int sameFrame;
    rdVector3 toTarget;       // sight state of the block (sameFrame)
    float distance;
    int sightState;
    rdVector3 targetPos;      // target position (sight update)
    rdVector3 targetVel;      // target physics velocity
    int targetMoveType;
    uint32_t losMask;
    float minDist, maxDist, fireDot, aimError;
    int weaponNum;
    uint32_t fireFlags;
    int burstCount;
    int hasWeapon, isAiming, isMounting;
    int invType;              // 0 none, 1 without cog, 2 with cog
    int bulletThing;          // -1: weapon fire fails
    int tailAttached;
    float hitAccuracy;
    float combatDamage;
    float accuracy;           // class accuracy
    float gravity;
    uint32_t seed;
    rdVector3 prevVecUnknown; // weaponInfo.vecUnknown0 before the call
} SithAIUtilVecFire;

static int sithAIUtilVec_RunAIFire(SithVecCtx* pCtx)
{
    const SithAIUtilVecFire* p = (const SithAIUtilVecFire*)pCtx->pCase->pParams;
    SithAIControlBlock* pLocal = UV_LOCAL;
    SithThing* pOwner          = UV_OWNER;
    sithMain_g_sith_mode.debugModeFlags = (SithDebugModeFlag)p->debugFlags;
    pOwner->flags                       = (SithThingFlag)p->ownerThingFlags;
    UV_SECTOR(0)->flags                 = (SithSectorFlag)p->sectorFlags;
    pOwner->thingInfo.actorInfo.flags   = (SithActorFlag)p->ownerActorFlags;
    pOwner->thingInfo.actorInfo.weaponInfo.curWeaponID  = p->curWeaponId;
    pOwner->thingInfo.actorInfo.weaponInfo.vecUnknown0  = p->prevVecUnknown;
    pOwner->thingInfo.actorInfo.fireOffset.y = 0.5f;
    if ( p->bTemplate )
    {
        SithThing* pTemplate = &sithVec_arena.templates[0];
        memset(pTemplate, 0, sizeof(*pTemplate));
        pTemplate->moveInfo.physics.velocity.y     = p->templateSpeed;
        pTemplate->thingInfo.weaponInfo.damage     = 12.5f;
        pTemplate->thingInfo.weaponInfo.damageType = (SithDamageType)3;
        pOwner->thingInfo.actorInfo.pWeaponTemplate = pTemplate;
    }

    if ( p->tailAttached )
    {
        SithThing* pAttached = UV_THING(6);
        sithVec_NewThing(6, SITH_THING_ACTOR, SITH_MT_NONE, 0);
        pAttached->attach.flags = SITH_ATTACH_TAIL;
        pOwner->pAttachedThing  = pAttached;
    }

    SithThing* pTarget = UV_TARGET;
    pTarget->pos       = p->targetPos;
    pTarget->moveType  = (SithThingMoveType)p->targetMoveType;
    pTarget->moveInfo.physics.velocity = p->targetVel;
    pTarget->collide.size = 0.5f;

    pLocal->pTargetThing = p->bTarget ? pTarget : NULL;
    pLocal->mode         = (SithAIMode)p->mode;
    pLocal->submode      = (SithAISubMode)p->submode;
    pLocal->msecFireWaitTime = p->msecFireWaitTime;
    pLocal->pClass->accurancy = p->accuracy;
    pLocal->targetSightFrame  = p->sameFrame ? (int)sithMain_g_frameNumber : 7;
    pLocal->toTarget          = p->toTarget;
    pLocal->distance          = p->distance;
    pLocal->targetSightState  = (SithAISightState)p->sightState;
    pLocal->weaponFirePos     = (rdVector3){ 0.0f, 0.5f, 0.0f };
    sithWorld_g_pCurrentWorld->gravity = p->gravity;

    SithAIUtilVecCfg* c = &sithAIUtilVec_cfg;
    c->losMask      = p->losMask;
    c->hasWeapon    = p->hasWeapon;
    c->isAiming     = p->isAiming;
    c->isMounting   = p->isMounting;
    c->invType      = p->invType;
    c->bulletThing  = p->bulletThing;
    c->hitAccuracy  = p->hitAccuracy;
    c->combatDamage = p->combatDamage;
    sithAIUtilVec_invType.pCog = p->invType == 2 ? (SithCog*)(uintptr_t)0x1234u : NULL;
    if ( p->bulletThing >= 0 )
    {
        sithVec_NewThing((unsigned int)p->bulletThing, SITH_THING_WEAPON, SITH_MT_PHYSICS, 0);
    }

    sithVec_stubRandSeed = sithAIUtilVec_Seed(p->seed);
    sithVec_Begin(pCtx);
    int ret = SITHVEC_FN(sithAIUtil_AIFire)(pLocal, p->minDist, p->maxDist, p->fireDot, p->aimError, p->weaponNum, (SithAIUtilFireFlags)p->fireFlags, p->burstCount);
    sithAIUtilVec_Out(0, (uint32_t)sithWorld_g_pCurrentWorld->gravity != 0u);
    return ret;
}

// ---------------------------------------------------------------------------------------------------------------
// Cases: things in view
// ---------------------------------------------------------------------------------------------------------------

// GetThingsInView and GetNextThingsInView. Sectors 0..3 are chained by adjoins (adjoin s leads from sector s to
// s + 1, adjoin 4 + s from sector s back to s - 1), each sector holds the listed things.
typedef struct
{
    uint8_t sector;
    uint8_t type;
    uint32_t flags;
    rdVector3 pos;
} SithAIUtilVecViewThing;

typedef struct
{
    float fovX, fovY;
    int sizeResult;
    int typeMask;
    float maxDistance;
    uint8_t startSector;
    uint32_t adjoinFlags;      // flags of all adjoins
    int surfaceKind;           // 0 no material, 1 opaque rgb material with geometry mode, 2 translucent, 3 non-rgb material, 4 material without geometry mode
    float normalY;             // y of the adjoin face normals (-1: facing against the view direction)
    float adjoinDistance;      // distance of the adjoins (and of their mirrors)
    uint32_t sectorTicks;      // bit s: sector s was already visited in this render tick (77 + 1 after the advance)
    uint8_t numThings;
    const SithAIUtilVecViewThing* pThings;
    rdVector3 orientPos;
    rdVector3 lvec;
    rdVector3 rvec;
    rdVector3 uvec;
    int bDirect;               // call GetNextThingsInView with the given distance and prepared state
    float distance;
    uint32_t directVisited;
} SithAIUtilVecView;

static rdMaterial sithAIUtilVec_aMaterials[2];

static int sithAIUtilVec_RunThingsInView(SithVecCtx* pCtx)
{
    const SithAIUtilVecView* p = (const SithAIUtilVecView*)pCtx->pCase->pParams;
    for ( unsigned int i = 0; i < p->numThings; ++i )
    {
        const SithAIUtilVecViewThing* pT = &p->pThings[i];
        SithThing* pThing                = sithVec_NewThing(14 + i, (SithThingType)pT->type, SITH_MT_NONE, pT->sector);
        pThing->flags                    = (SithThingFlag)pT->flags;
        pThing->pos                      = pT->pos;
    }

    // Adjoins: forward 0..2 (sector s to s + 1), back 4..6 (sector s + 1 to s)
    for ( unsigned int a = 0; a < 7; ++a )
    {
        if ( a == 3 )
        {
            continue;
        }

        unsigned int from = a < 3 ? a : a - 4u + 1u;
        unsigned int to   = a < 3 ? a + 1u : a - 4u;
        SithSurfaceAdjoin* pAdjoin = &sithAIUtilVec_aAdjoins[a];
        SithSurface* pSurface      = &sithAIUtilVec_aSurfaces[a];
        memset(pAdjoin, 0, sizeof(*pAdjoin));
        memset(pSurface, 0, sizeof(*pSurface));
        pAdjoin->flags          = (SithSurfaceAdjoinFlag)p->adjoinFlags;
        pAdjoin->pAdjoinSector  = UV_SECTOR(to);
        pAdjoin->pAdjoinSurface = pSurface;
        pAdjoin->distance       = p->adjoinDistance;
        pAdjoin->pMirrorAdjoin  = &sithAIUtilVec_aAdjoins[a < 3 ? a + 4u : a - 4u];
        pSurface->pSector       = UV_SECTOR(from);
        pSurface->pAdjoin       = pAdjoin;
        pSurface->face.normal.y = p->normalY;
        switch ( p->surfaceKind )
        {
            case 1:
                pSurface->face.pMaterial    = &sithAIUtilVec_aMaterials[0];
                pSurface->face.geometryMode = (rdGeometryMode)3;
                break;

            case 2:
                pSurface->face.pMaterial    = &sithAIUtilVec_aMaterials[0];
                pSurface->face.geometryMode = (rdGeometryMode)3;
                pSurface->face.flags        = RD_FF_TEX_TRANSLUCENT;
                break;

            case 3:
                pSurface->face.pMaterial    = &sithAIUtilVec_aMaterials[1];
                pSurface->face.geometryMode = (rdGeometryMode)3;
                break;

            case 4:
                pSurface->face.pMaterial    = &sithAIUtilVec_aMaterials[0];
                pSurface->face.geometryMode = (rdGeometryMode)0;
                break;

            default:
                break;
        }
    }

    memset(sithAIUtilVec_aMaterials, 0, sizeof(sithAIUtilVec_aMaterials));
    sithAIUtilVec_aMaterials[0].formatType = STDCOLOR_FORMAT_RGB;
    sithAIUtilVec_aMaterials[1].formatType = (StdColorFormatType)1;

    // Sector 0: adjoin 0 then (back adjoins) ...; sector s: forward adjoin s then back adjoin 4 + s - 1
    UV_SECTOR(0)->pFirstAdjoin = &sithAIUtilVec_aAdjoins[0];
    for ( unsigned int s = 1; s < 4; ++s )
    {
        SithSurfaceAdjoin* pBack = &sithAIUtilVec_aAdjoins[4 + s - 1];
        UV_SECTOR(s)->pFirstAdjoin = pBack;
        if ( s < 3 )
        {
            pBack->pNextAdjoin = &sithAIUtilVec_aAdjoins[s];
        }
    }

    for ( unsigned int s = 0; s < 4; ++s )
    {
        if ( p->sectorTicks & (1u << s) )
        {
            UV_SECTOR(s)->renderTick = sithMain_g_curRenderTick + (p->bDirect ? 0u : 1u);
        }
    }

    for ( unsigned int i = 0; i < 16; ++i )
    {
        sithAIUtilVec_apViewResults[i] = (SithThing*)(uintptr_t)0xDEAD0000u;
    }

    rdMatrix34 orient;
    memset(&orient, 0, sizeof(orient));
    orient.rvec = p->rvec;
    orient.lvec = p->lvec;
    orient.uvec = p->uvec;
    orient.dvec = p->orientPos;

    sithVec_Begin(pCtx);
    int ret = 0;
    if ( p->bDirect )
    {
        // Prepared state of a running search
        *sithAIUtilVec_st.pFovX       = p->fovX;
        *sithAIUtilVec_st.pFovY       = p->fovY;
        *sithAIUtilVec_st.pNumVisited = p->directVisited;
        *sithAIUtilVec_st.pNumInView  = 0;
        *sithAIUtilVec_st.pSizeInView = (size_t)p->sizeResult;
        *sithAIUtilVec_st.ppThingsInView = sithAIUtilVec_apViewResults;
        *sithAIUtilVec_st.pMaxDist    = p->maxDistance;
        *sithAIUtilVec_st.pTypeMask   = p->typeMask;
        SITHVEC_FN(sithAIUtil_GetNextThingsInView)(UV_SECTOR(p->startSector), &orient, p->distance);
        ret = (int)*sithAIUtilVec_st.pNumInView;
    }
    else
    {
        ret = (int)SITHVEC_FN(sithAIUtil_GetThingsInView)(UV_SECTOR(p->startSector), &orient, p->fovX, p->fovY, p->sizeResult, sithAIUtilVec_apViewResults, p->typeMask, p->maxDistance);
    }

    for ( unsigned int i = 0; i < 16; ++i )
    {
        sithAIUtilVec_Out(i, SITHVEC_P(sithAIUtilVec_apViewResults[i]));
    }

    return ret;
}

// ---------------------------------------------------------------------------------------------------------------
// Cases: collision based position checks
// ---------------------------------------------------------------------------------------------------------------

// CheckPathToPoint (var 0): the owner at the origin checks the path to a position
typedef struct
{
    int bDetectThings;
    int bSkipFloor;
    rdVector3 target;
    float radius;
    const SithAIUtilVecSearch* pSearches;
    uint8_t numSearches;
} SithAIUtilVecPoint;

static int sithAIUtilVec_RunCheckPathToPoint(SithVecCtx* pCtx)
{
    const SithAIUtilVecPoint* p = (const SithAIUtilVecPoint*)pCtx->pCase->pParams;
    sithAIUtilVec_LoadSearches(p->pSearches, p->numSearches);

    rdVector3 target  = p->target;
    float distance    = 77.0f;
    rdVector3 hitNorm = { 71.0f, 72.0f, 73.0f };
    sithVec_Begin(pCtx);
    int ret = SITHVEC_FN(sithAIUtil_CheckPathToPoint)(UV_OWNER, &target, p->radius, &distance, &hitNorm, p->bDetectThings, p->bSkipFloor);
    sithAIUtilVec_OutF(0, distance);
    sithAIUtilVec_OutVec(1, &hitNorm);
    return ret;
}

// CheckFloorAtPos (var 0) and CheckPosition (var 1)
typedef struct
{
    uint32_t ownerMode;
    rdVector3 ownerUvec;
    uint32_t ownerThingFlags;
    uint32_t ownerSectorFlags;
    uint32_t foundSectorFlags;   // CheckPosition: flags of the sector found at the end position
    int sectorMode;              // CheckPosition: 0 the end position is in the start sector, 1 in the next sector, 2 not in any sector
    uint32_t ownerPhysFlags;
    uint32_t ownerAttachFlags;
    int ownerAttachedSurface;    // index of the collision whose surface the owner is attached to, -1: none
    int ownerAttachedThing;      // 0 none, 1 thing 5 (the thing collided)
    int ownerTail;               // the owner has an attached tail thing
    uint32_t allowedSurfaceTypes;
    float maxStep;
    uint32_t thingAttachFlags;   // thing 5
    int thingAttachedToOwner;
    uint32_t thingFlags;
    int thingType;
    uint32_t adjoinSectorFlags;  // flags of sector 2, the sector of the collided adjoins
    int bNoCollision;            // a pbNoCollision pointer is passed
    const SithAIUtilVecSearch* pSearches;
    uint8_t numSearches;
} SithAIUtilVecFloor;

static int sithAIUtilVec_RunFloor(SithVecCtx* pCtx)
{
    const SithAIUtilVecFloor* p = (const SithAIUtilVecFloor*)pCtx->pCase->pParams;
    SithThing* pOwner           = UV_OWNER;
    pOwner->orient.uvec         = p->ownerUvec;
    pOwner->flags               = (SithThingFlag)p->ownerThingFlags;
    UV_SECTOR(0)->flags         = (SithSectorFlag)p->ownerSectorFlags;
    UV_SECTOR(1)->flags         = (SithSectorFlag)p->foundSectorFlags;
    UV_SECTOR(2)->flags         = (SithSectorFlag)p->adjoinSectorFlags;
    pOwner->moveInfo.physics.flags = (SithPhysicsFlags)p->ownerPhysFlags;
    pOwner->attach.flags        = (SithAttachFlag)p->ownerAttachFlags;
    UV_LOCAL->mode              = (SithAIMode)p->ownerMode;
    UV_LOCAL->allowedSurfaceTypes = (int)p->allowedSurfaceTypes;
    UV_CLASS(0)->maxStep        = p->maxStep;
    sithAIUtilVec_cfg.thingHeight = 1.0f;
    sithVec_stubFindSectorMode  = p->sectorMode == 1 ? 1 : 0;

    SithThing* pCollided = sithVec_NewThing(5, (SithThingType)p->thingType, SITH_MT_NONE, 0);
    pCollided->flags       = (SithThingFlag)p->thingFlags;
    pCollided->attach.flags = (SithAttachFlag)p->thingAttachFlags;
    pCollided->attach.attachedToStructure.pThingAttached = p->thingAttachedToOwner ? pOwner : UV_THING(6);
    if ( p->ownerTail )
    {
        SithThing* pTail = sithVec_NewThing(7, SITH_THING_ACTOR, SITH_MT_NONE, 0);
        pTail->attach.flags  = SITH_ATTACH_TAIL;
        pOwner->pAttachedThing = pTail;
    }

    sithAIUtilVec_LoadSearches(p->pSearches, p->numSearches);
    if ( p->ownerAttachedSurface >= 0 )
    {
        pOwner->attach.attachedToStructure.pSurfaceAttached = &sithAIUtilVec_aSurfaces[p->ownerAttachedSurface];
    }
    else if ( p->ownerAttachedThing )
    {
        pOwner->attach.attachedToStructure.pThingAttached = pCollided;
    }

    // The collided thing of every scripted thing collision is thing 5
    for ( unsigned int i = 0; i < SITHAIUTILVEC_MAXCOLL; ++i )
    {
        if ( sithAIUtilVec_aCollisions[i].pThingCollided )
        {
            sithAIUtilVec_aCollisions[i].pThingCollided = pCollided;
        }
    }

    rdVector3 pos = { 1.0f, 2.0f, 3.0f };
    int noCollision = 77;
    SithSector* pStartSector = p->sectorMode == 2 ? NULL : UV_SECTOR(0);
    if ( p->sectorMode == 2 )
    {
        pOwner->pInSector = NULL;
    }

    sithVec_Begin(pCtx);
    int ret;
    if ( pCtx->pCase->var == 0 )
    {
        ret = SITHVEC_FN(sithAIUtil_CheckFloorAtPos)(UV_LOCAL, &pos, pStartSector ? pStartSector : UV_SECTOR(0), p->bNoCollision ? &noCollision : NULL);
    }
    else
    {
        ret = SITHVEC_FN(sithAIUtil_CheckPosition)(UV_LOCAL, &pos, p->bNoCollision ? &noCollision : NULL);
    }

    sithAIUtilVec_Out(0, (uint32_t)noCollision);
    return ret;
}

// ---------------------------------------------------------------------------------------------------------------
// Cases: point creation and path checks
// ---------------------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t flags;          // SithAIUtilMkPointFlags | SithAIUtilPathFlags
    float angle, maxAngle, deltaAngle;
    float minDist, moveDist, radius;
    float downCos;
    rdVector3 pos;           // start position (the owner is moved there)
    rdVector3 dir;
    rdVector3 moveDirection;
    rdVector3 ownerUvec;
    rdVector3 movePos, homePos, goalPos;
    int goalThing;           // goal thing (thing 2) set, 2: at goalPos
    int fleeThing;           // flee-from thing (thing 3) set
    int targetThing;         // target thing (thing 1) set
    uint32_t mode;
    uint32_t losMask;
    uint32_t seed;
    int sectorMode;          // sithCollision_FindSectorInRadius: 0 start sector, 1 next sector
    int bNullPos, bNullDir, bNullDest, bNullEnd;
    int maxRetries;
    int bAllow3D;
    uint8_t retries;         // MakeRandPoint: this many RetryMakeRandPoint calls follow
    uint8_t retryOther;      // the retries use another control block
    const SithAIUtilVecSearch* pSearches;
    uint8_t numSearches;
} SithAIUtilVecPath;

static void sithAIUtilVec_SetupPath(const SithAIUtilVecPath* p)
{
    UV_OWNER->pos                = p->pos;
    UV_OWNER->orient.uvec        = p->ownerUvec;
    UV_LOCAL->moveDirection      = p->moveDirection;
    UV_LOCAL->movePos            = p->movePos;
    UV_LOCAL->homePos            = p->homePos;
    UV_LOCAL->goalPos            = p->goalPos;
    UV_LOCAL->goalThing          = p->goalThing ? UV_GOAL : NULL;
    if ( p->goalThing == 2 )
    {
        UV_GOAL->pos = p->goalPos; // the goal thing stands at goalPos
    }

    UV_LOCAL->pFleeFromThing     = p->fleeThing ? UV_FLEE : NULL;
    UV_LOCAL->pTargetThing       = p->targetThing ? UV_TARGET : NULL;
    UV_LOCAL->mode               = (SithAIMode)p->mode;
    UV_LOCAL->moveSpeed          = 1.5f;
    sithAIUtilVec_cfg.losMask    = p->losMask;
    sithVec_stubRandSeed         = sithAIUtilVec_Seed(p->seed);
    sithVec_stubFindSectorMode   = p->sectorMode;
    UV_FLEE->moveInfo.physics.velocity.x = 2.0f;
    UV_FLEE->moveInfo.physics.velocity.y = 1.0f;
    sithAIUtilVec_LoadSearches(p->pSearches, p->numSearches);
}

// var 0: MakeRandPoint, then retries RetryMakeRandPoint calls (one checkpoint per call)
static int sithAIUtilVec_RunMakeRandPoint(SithVecCtx* pCtx)
{
    const SithAIUtilVecPath* p = (const SithAIUtilVecPath*)pCtx->pCase->pParams;
    sithAIUtilVec_SetupPath(p);

    rdVector3 pos      = p->pos;
    rdVector3 dir      = p->dir;
    rdVector3 dest     = { 61.0f, 62.0f, 63.0f };
    float downCos      = p->downCos;
    sithVec_Begin(pCtx);
    int ret = SITHVEC_FN(sithAIUtil_MakeRandPoint)(UV_LOCAL, p->bNullPos ? NULL : &pos, (SithAIUtilMkPointFlags)p->flags, p->angle, &downCos, p->bNullDir ? NULL : &dir, p->bNullDest ? NULL : &dest);
    sithAIUtilVec_OutF(0, downCos);
    sithAIUtilVec_OutVec(1, &dir);
    sithAIUtilVec_OutVec(2, &dest);
    sithVec_Checkpoint(pCtx);

    for ( unsigned int i = 0; i < p->retries; ++i )
    {
        SithAIControlBlock* pOther = p->retryOther ? UV_BLOCK(1) : UV_LOCAL;
        rdVector3 retryDir  = { 51.0f, 52.0f, 53.0f };
        rdVector3 retryDest = { 41.0f, 42.0f, 43.0f };
        int retryRet        = SITHVEC_FN(sithAIUtil_RetryMakeRandPoint)(pOther, &pos, downCos, &retryDir, &retryDest);
        sithAIUtilVec_Out(10, (uint32_t)retryRet);
        sithAIUtilVec_OutVec(11, &retryDir);
        sithAIUtilVec_OutVec(12, &retryDest);
        if ( i + 1 < p->retries )
        {
            sithVec_Checkpoint(pCtx);
        }
    }

    return ret;
}

// A retry without a preceding MakeRandPoint call (the statics are in their initial state), var 0: no
// MakeRandPoint; var 1: stale state of another control block; var 2: stale state of this control block
static int sithAIUtilVec_RunRetryAlone(SithVecCtx* pCtx)
{
    const SithAIUtilVecPath* p = (const SithAIUtilVecPath*)pCtx->pCase->pParams;
    sithAIUtilVec_SetupPath(p);
    if ( pCtx->pCase->var )
    {
        *sithAIUtilVec_st.pMkFlags  = (SithAIUtilMkPointFlags)p->flags;
        *sithAIUtilVec_st.ppMkLocal = pCtx->pCase->var == 2 ? UV_LOCAL : UV_BLOCK(1);
        *sithAIUtilVec_st.pMkDir    = p->dir;
        *sithAIUtilVec_st.pMkPyr    = p->movePos;
        *sithAIUtilVec_st.pMkRVec   = p->homePos;
    }

    rdVector3 pos       = p->pos;
    rdVector3 retryDir  = { 51.0f, 52.0f, 53.0f };
    rdVector3 retryDest = { 41.0f, 42.0f, 43.0f };
    sithVec_Begin(pCtx);
    int ret = SITHVEC_FN(sithAIUtil_RetryMakeRandPoint)(p->retryOther ? NULL : UV_LOCAL, p->bNullPos ? NULL : &pos, p->downCos, p->bNullDir ? NULL : &retryDir, p->bNullDest ? NULL : &retryDest);
    sithAIUtilVec_OutVec(0, &retryDir);
    sithAIUtilVec_OutVec(1, &retryDest);
    return ret;
}

static int sithAIUtilVec_RunCheckPathToPos(SithVecCtx* pCtx)
{
    const SithAIUtilVecPath* p = (const SithAIUtilVecPath*)pCtx->pCase->pParams;
    sithAIUtilVec_SetupPath(p);
    sithVec_NewThing(4, SITH_THING_PLAYER, SITH_MT_PHYSICS, 0);
    sithVec_NewThing(5, SITH_THING_ACTOR, SITH_MT_PHYSICS, 0);

    rdVector3 startPos = p->pos;
    rdVector3 dir      = p->dir;
    rdVector3 endPos   = { 61.0f, 62.0f, 63.0f };
    UV_LOCAL->toGoal   = (rdVector3){ 5.0f, 6.0f, 7.0f };
    UV_LOCAL->targetDistance = 8.0f;
    sithVec_Begin(pCtx);
    int ret = SITHVEC_FN(sithAIUtil_CheckPathToPos)(UV_LOCAL, UV_SECTOR(0), &startPos, (SithAIUtilPathFlags)p->flags, p->minDist, p->moveDist, p->radius, p->bNullDir ? NULL : &dir, p->bNullEnd ? NULL : &endPos);
    sithAIUtilVec_OutVec(0, &endPos);
    return ret;
}

static int sithAIUtilVec_RunSweepMoveToPathPos(SithVecCtx* pCtx)
{
    const SithAIUtilVecPath* p = (const SithAIUtilVecPath*)pCtx->pCase->pParams;
    sithAIUtilVec_SetupPath(p);

    rdVector3 pos = p->goalPos;
    sithVec_Begin(pCtx);
    return SITHVEC_FN(sithAIUtil_SweepMoveToPathPos)(UV_LOCAL, &pos, p->angle, p->maxAngle, p->deltaAngle, (SithAIUtilMkPointFlags)p->flags);
}

static int sithAIUtilVec_RunMakeRoamPos(SithVecCtx* pCtx)
{
    const SithAIUtilVecPath* p = (const SithAIUtilVecPath*)pCtx->pCase->pParams;
    sithAIUtilVec_SetupPath(p);

    rdVector3 pos  = p->pos;
    rdVector3 dest = { 61.0f, 62.0f, 63.0f };
    sithVec_Begin(pCtx);
    int ret = SITHVEC_FN(sithAIUtil_MakeRoamPos)(UV_LOCAL, &pos, p->minDist, p->moveDist, p->bAllow3D, p->maxRetries, &dest);
    sithAIUtilVec_OutVec(0, &dest);
    return ret;
}

static int sithAIUtilVec_RunMakePathPos(SithVecCtx* pCtx)
{
    const SithAIUtilVecPath* p = (const SithAIUtilVecPath*)pCtx->pCase->pParams;
    sithAIUtilVec_SetupPath(p);

    rdVector3 pos  = p->pos;
    rdVector3 dir  = p->dir;
    rdVector3 dest = { 61.0f, 62.0f, 63.0f };
    sithVec_Begin(pCtx);
    int ret = SITHVEC_FN(sithAIUtil_MakePathPos)(UV_LOCAL, &pos, p->minDist, p->moveDist, p->angle, (int)p->flags, &dir, &dest);
    sithAIUtilVec_OutVec(0, &dir);
    sithAIUtilVec_OutVec(1, &dest);
    return ret;
}

// ---------------------------------------------------------------------------------------------------------------
// Parameters and case table
// ---------------------------------------------------------------------------------------------------------------
#define UVP(type, name, ...) static const type name = { __VA_ARGS__ }
#define V3(x, y, z)          { x, y, z }

// ---- SetWeaponFireFlags
UVP(SithAIUtilVecWeaponFlags, uvp_wf_0, 0, 0);
UVP(SithAIUtilVecWeaponFlags, uvp_wf_1, 1, 0);
UVP(SithAIUtilVecWeaponFlags, uvp_wf_2, 2, 0);
UVP(SithAIUtilVecWeaponFlags, uvp_wf_3, 3, 0);
UVP(SithAIUtilVecWeaponFlags, uvp_wf_0all, 0, 0xFFFFFFFFu);
UVP(SithAIUtilVecWeaponFlags, uvp_wf_3some, 3, 0x155u);
UVP(SithAIUtilVecWeaponFlags, uvp_wf_m1, -1, 0x155u);
UVP(SithAIUtilVecWeaponFlags, uvp_wf_4, 4, 0x155u);
UVP(SithAIUtilVecWeaponFlags, uvp_wf_100, 100, 0x2u);
UVP(SithAIUtilVecWeaponFlags, uvp_wf_min, (int)0x80000000u, 0x2u);
UVP(SithAIUtilVecWeaponFlags, uvp_wf_max, 0x7FFFFFFF, 0x2u);

// ---- CanAttackTarget: { bNull, type, thing flags, actor flags }
UVP(SithAIUtilVecCanAttack, uvp_ca_null, 1, SITH_THING_ACTOR, 0, 0);
UVP(SithAIUtilVecCanAttack, uvp_ca_plain, 0, SITH_THING_ACTOR, 0, 0);
UVP(SithAIUtilVecCanAttack, uvp_ca_disabled, 0, SITH_THING_ACTOR, SITH_TF_DISABLED, 0);
UVP(SithAIUtilVecCanAttack, uvp_ca_dying, 0, SITH_THING_ACTOR, SITH_TF_DYING, 0);
UVP(SithAIUtilVecCanAttack, uvp_ca_destroyed, 0, SITH_THING_ACTOR, SITH_TF_DESTROYED, 0);
UVP(SithAIUtilVecCanAttack, uvp_ca_all3, 0, SITH_THING_ACTOR, SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED, 0);
UVP(SithAIUtilVecCanAttack, uvp_ca_otherflags, 0, SITH_THING_ACTOR, (uint32_t)~(SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED), 0);
UVP(SithAIUtilVecCanAttack, uvp_ca_player, 0, SITH_THING_PLAYER, 0, 0);
UVP(SithAIUtilVecCanAttack, uvp_ca_playerdisabledcontrols, 0, SITH_THING_PLAYER, 0, SITH_AF_CONTROLSDISABLED);
UVP(SithAIUtilVecCanAttack, uvp_ca_playerotheractorflags, 0, SITH_THING_PLAYER, 0, (uint32_t)~SITH_AF_CONTROLSDISABLED);
UVP(SithAIUtilVecCanAttack, uvp_ca_actordisabledcontrols, 0, SITH_THING_ACTOR, 0, SITH_AF_CONTROLSDISABLED);
UVP(SithAIUtilVecCanAttack, uvp_ca_playerdyingcontrols, 0, SITH_THING_PLAYER, SITH_TF_DYING, SITH_AF_CONTROLSDISABLED);

// ---- IsThingMoving: { type, moveType, bHasLocal, mode, vel, angVel, pathMode, bNull }
UVP(SithAIUtilVecMoving, uvp_mv_null, SITH_THING_ACTOR, SITH_MT_PHYSICS, 1, 0, V3(0, 0, 0), V3(0, 0, 0), 0, 1);
UVP(SithAIUtilVecMoving, uvp_mv_free, SITH_THING_FREE, SITH_MT_PHYSICS, 1, SITHAI_MODE_MOVING, V3(1, 0, 0), V3(0, 0, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_actornolocal, SITH_THING_ACTOR, SITH_MT_PHYSICS, 0, SITHAI_MODE_MOVING, V3(1, 0, 0), V3(0, 0, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_actormoving, SITH_THING_ACTOR, SITH_MT_PHYSICS, 1, SITHAI_MODE_MOVING, V3(0, 0, 0), V3(0, 0, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_actoridle, SITH_THING_ACTOR, SITH_MT_PHYSICS, 1, 0, V3(1, 1, 1), V3(1, 1, 1), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_actorothermodes, SITH_THING_ACTOR, SITH_MT_PHYSICS, 1, (uint32_t)~SITHAI_MODE_MOVING, V3(1, 0, 0), V3(0, 0, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_actorallmodes, SITH_THING_ACTOR, SITH_MT_PHYSICS, 1, 0xFFFFFFFFu, V3(0, 0, 0), V3(0, 0, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_physicsstill, SITH_THING_ITEM, SITH_MT_PHYSICS, 1, 0, V3(0, 0, 0), V3(0, 0, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_physicsvel, SITH_THING_ITEM, SITH_MT_PHYSICS, 1, 0, V3(0, 0, 0.5f), V3(0, 0, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_physicsangvel, SITH_THING_PLAYER, SITH_MT_PHYSICS, 1, 0, V3(0, 0, 0), V3(0, 2, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_physicsboth, SITH_THING_WEAPON, SITH_MT_PHYSICS, 1, 0, V3(1, 2, 3), V3(4, 5, 6), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_physicsnegzero, SITH_THING_ITEM, SITH_MT_PHYSICS, 1, 0, V3(-0.0f, -0.0f, -0.0f), V3(-0.0f, 0, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_physicstiny, SITH_THING_ITEM, SITH_MT_PHYSICS, 1, 0, V3(1e-30f, 0, 0), V3(0, 0, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_path0, SITH_THING_ITEM, SITH_MT_PATH, 1, 0, V3(0, 0, 0), V3(0, 0, 0), 0, 0);
UVP(SithAIUtilVecMoving, uvp_mv_path1, SITH_THING_ITEM, SITH_MT_PATH, 1, 0, V3(0, 0, 0), V3(0, 0, 0), 1, 0);
UVP(SithAIUtilVecMoving, uvp_mv_path2, SITH_THING_ITEM, SITH_MT_PATH, 1, 0, V3(0, 0, 0), V3(0, 0, 0), 2, 0);
UVP(SithAIUtilVecMoving, uvp_mv_path3, SITH_THING_GHOST, SITH_MT_PATH, 1, 0, V3(0, 0, 0), V3(0, 0, 0), 3, 0);
UVP(SithAIUtilVecMoving, uvp_mv_none, SITH_THING_ITEM, SITH_MT_NONE, 1, 0, V3(1, 0, 0), V3(1, 0, 0), 0, 0);

// ---- AIPauseMove: { game time, pause }
UVP(SithAIUtilVecPause, uvp_pm_basic, 100000u, 500);
UVP(SithAIUtilVecPause, uvp_pm_zero, 100000u, 0);
UVP(SithAIUtilVecPause, uvp_pm_negative, 100000u, -250);
UVP(SithAIUtilVecPause, uvp_pm_wrap, 0xFFFFFF00u, 0x200);
UVP(SithAIUtilVecPause, uvp_pm_max, 5u, 0x7FFFFFFF);
UVP(SithAIUtilVecPause, uvp_pm_min, 5u, (int)0x80000000u);
UVP(SithAIUtilVecPause, uvp_pm_timezero, 0u, 1);

// ---- AISetMode / AIClearMode: { ai mode, initial mode, initial submode, owner registered }
#define UVMODE(id, aiMode)                                                                   \
    UVP(SithAIUtilVecMode, uvp_mode_##id##_zero, aiMode, 0, 0, 1);                           \
    UVP(SithAIUtilVecMode, uvp_mode_##id##_ones, aiMode, 0xFFFFFFFFu, 0xFFFFFFFFu, 1);       \
    UVP(SithAIUtilVecMode, uvp_mode_##id##_nocont, aiMode, 0xFFFFFFFFu, (uint32_t)~SITHAI_SUBMODE_CONTINUOUSWPNTMOTION, 1); \
    UVP(SithAIUtilVecMode, uvp_mode_##id##_unreg, aiMode, 0xFFFFFFFFu, 0xFFFFFFFFu, 0);      \
    UVP(SithAIUtilVecMode, uvp_mode_##id##_unregnocont, aiMode, 0x0A0A0A0Au, 0x50505050u & (uint32_t)~SITHAI_SUBMODE_CONTINUOUSWPNTMOTION, 0)

UVMODE(moving, SITHAI_MODE_MOVING);
UVMODE(searching, SITHAI_MODE_SEARCHING);
UVMODE(attacking, SITHAI_MODE_ATTACKING);
UVMODE(hunting, SITHAI_MODE_HUNTING);
UVMODE(fleeingtowaypoint, SITHAI_MODE_FLEEINGTOWAYPOINT);
UVMODE(fleeing, SITHAI_MODE_FLEEING);
UVMODE(block, SITHAI_MODE_BLOCK);
UVMODE(wallcrawling, SITHAI_MODE_WALLCRAWLING);
UVMODE(turning, SITHAI_MODE_TURNING);
UVMODE(traversewpnts, SITHAI_MODE_TRAVERSEWPNTS);
UVMODE(none, 0);
UVMODE(combined, SITHAI_MODE_MOVING | SITHAI_MODE_ATTACKING);
UVMODE(highbit, 0x40000000u);
UVMODE(allbits, 0xFFFFFFFFu);
UVMODE(dodging, SITHAI_MODE_DODGING);

// ---- AIGetMovePos: { flags, goal thing, flee thing }
UVP(SithAIUtilVecMovePos, uvp_gm_none, 0, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_owner, SITHAIUTIL_MKPOINT_REFOWNER, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_movepos, SITHAIUTIL_MKPOINT_TOWARDMOVEPOS, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_home, SITHAIUTIL_MKPOINT_REFHOME, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_goalthing, SITHAIUTIL_MKPOINT_TOWARDGOAL, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_goalpos, SITHAIUTIL_MKPOINT_TOWARDGOAL, 0, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_awaygoalthing, SITHAIUTIL_MKPOINT_AWAYFROMGOAL, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_awaygoalpos, SITHAIUTIL_MKPOINT_AWAYFROMGOAL, 0, 0);
UVP(SithAIUtilVecMovePos, uvp_gm_flee, SITHAIUTIL_MKPOINT_AWAYFROMFLEE, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_noflee, SITHAIUTIL_MKPOINT_AWAYFROMFLEE, 1, 0);
UVP(SithAIUtilVecMovePos, uvp_gm_ownerfirst, SITHAIUTIL_MKPOINT_REFOWNER | SITHAIUTIL_MKPOINT_TOWARDMOVEPOS | SITHAIUTIL_MKPOINT_REFHOME, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_moveposfirst, SITHAIUTIL_MKPOINT_TOWARDMOVEPOS | SITHAIUTIL_MKPOINT_REFHOME | SITHAIUTIL_MKPOINT_TOWARDGOAL, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_homefirst, SITHAIUTIL_MKPOINT_REFHOME | SITHAIUTIL_MKPOINT_TOWARDGOAL | SITHAIUTIL_MKPOINT_AWAYFROMFLEE, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_goalfirst, SITHAIUTIL_MKPOINT_TOWARDGOAL | SITHAIUTIL_MKPOINT_AWAYFROMFLEE, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_unrelated, SITHAIUTIL_MKPOINT_YAW | SITHAIUTIL_MKPOINT_PITCH | SITHAIUTIL_MKPOINT_RANDANGLE | SITHAIUTIL_MKPOINT_TARGETDIST, 1, 1);
UVP(SithAIUtilVecMovePos, uvp_gm_all, 0xFFFFFFFFu, 1, 1);

// ---- Heading vectors: { bNullThing, bNullDest, vel, lvec }
UVP(SithAIUtilVecHeading, uvp_hd_nullthing, 1, 0, V3(1, 0, 0), V3(0, 1, 0));
UVP(SithAIUtilVecHeading, uvp_hd_nulldest, 0, 1, V3(1, 0, 0), V3(0, 1, 0));
UVP(SithAIUtilVecHeading, uvp_hd_bothnull, 1, 1, V3(1, 0, 0), V3(0, 1, 0));
UVP(SithAIUtilVecHeading, uvp_hd_still, 0, 0, V3(0, 0, 0), V3(0.6f, 0.8f, 0));
UVP(SithAIUtilVecHeading, uvp_hd_stilltilted, 0, 0, V3(0, 0, 0), V3(0, 0.6f, 0.8f));
UVP(SithAIUtilVecHeading, uvp_hd_stillnegzero, 0, 0, V3(-0.0f, 0, -0.0f), V3(0, 1, 0));
UVP(SithAIUtilVecHeading, uvp_hd_vel, 0, 0, V3(3, 4, 5), V3(0, 1, 0));
UVP(SithAIUtilVecHeading, uvp_hd_velflat, 0, 0, V3(3, 4, 0), V3(0, 1, 0));
UVP(SithAIUtilVecHeading, uvp_hd_velvertical, 0, 0, V3(0, 0, 7), V3(0, 1, 0));
UVP(SithAIUtilVecHeading, uvp_hd_velneg, 0, 0, V3(-2, -6, -3), V3(0, 1, 0));
UVP(SithAIUtilVecHeading, uvp_hd_veltiny, 0, 0, V3(0.000001f, 0.000002f, 0.000003f), V3(0, 1, 0));
UVP(SithAIUtilVecHeading, uvp_hd_velmixedtiny, 0, 0, V3(0.0000001f, 5, 0.000002f), V3(0, 1, 0));
UVP(SithAIUtilVecHeading, uvp_hd_velhuge, 0, 0, V3(1e20f, -1e20f, 1e20f), V3(0, 1, 0));

// ---- ApplyForce: { mass, dir, force }
UVP(SithAIUtilVecForce, uvp_af_basic, 2.5f, V3(1, 2, 3), 3.0f);
UVP(SithAIUtilVecForce, uvp_af_zeromass, 0.0f, V3(1, 2, 3), 3.0f);
UVP(SithAIUtilVecForce, uvp_af_zeroforce, 2.5f, V3(1, 2, 3), 0.0f);
UVP(SithAIUtilVecForce, uvp_af_negative, -2.0f, V3(-1, 0, 0.5f), -4.0f);
UVP(SithAIUtilVecForce, uvp_af_zerodir, 2.5f, V3(0, 0, 0), 3.0f);

// ---- RandomRotate: { pyr, scalar }
UVP(SithAIUtilVecRotate, uvp_rr_basic, V3(1, 2, 3), 2.0f);
UVP(SithAIUtilVecRotate, uvp_rr_zeroscalar, V3(1, 2, 3), 0.0f);
UVP(SithAIUtilVecRotate, uvp_rr_zerovec, V3(0, 0, 0), 10.0f);
UVP(SithAIUtilVecRotate, uvp_rr_negscalar, V3(0, 1, 0), -3.0f);
UVP(SithAIUtilVecRotate, uvp_rr_zerobothzero, V3(0, 0, 0), 0.0f);
UVP(SithAIUtilVecRotate, uvp_rr_large, V3(100, -50, 25), 1000.0f);

// ---- AIPlaySoundMode: { bNullLocal, bNullOwner, hasInstinct, mode }
UVP(SithAIUtilVecSound, uvp_ps_nulllocal, 1, 0, 1, SITHSOUNDCLASS_CURIOUS);
UVP(SithAIUtilVecSound, uvp_ps_nullowner, 0, 1, 1, SITHSOUNDCLASS_CURIOUS);
UVP(SithAIUtilVecSound, uvp_ps_talkcurious, 0, 0, 1, SITHSOUNDCLASS_CURIOUS);
UVP(SithAIUtilVecSound, uvp_ps_talkalert, 0, 0, 1, SITHSOUNDCLASS_ALERT);
UVP(SithAIUtilVecSound, uvp_ps_talkreserved8, 0, 0, 1, SITHSOUNDCLASS_RESERVED8);
UVP(SithAIUtilVecSound, uvp_ps_nottalk, 0, 0, 0, SITHSOUNDCLASS_ALERT);
UVP(SithAIUtilVecSound, uvp_ps_belowrange, 0, 0, 1, SITHSOUNDCLASS_CURIOUS - 1);
UVP(SithAIUtilVecSound, uvp_ps_aboverange, 0, 0, 1, SITHSOUNDCLASS_RESERVED8 + 1);
UVP(SithAIUtilVecSound, uvp_ps_fire1, 0, 0, 1, SITHSOUNDCLASS_FIRE1);
UVP(SithAIUtilVecSound, uvp_ps_negative, 0, 0, 1, -1);
UVP(SithAIUtilVecSound, uvp_ps_huge, 0, 0, 1, (int)0x80000000u);
UVP(SithAIUtilVecSound, uvp_ps_nulllocalnull, 1, 1, 0, 0);

// ---- AISetWpnt: { wpntIdx, thing: 0 ghost 1, 1 actor, 2 NULL, 3 ghost 0, 4 free thing }
UVP(SithAIUtilVecSetWpnt, uvp_sw_ghost, 5, 0);
UVP(SithAIUtilVecSetWpnt, uvp_sw_ghost0, 0, 3);
UVP(SithAIUtilVecSetWpnt, uvp_sw_last, 59, 0);
UVP(SithAIUtilVecSetWpnt, uvp_sw_outofrange, 60, 0);
UVP(SithAIUtilVecSetWpnt, uvp_sw_wayoutofrange, 0xFFFFFFFFu, 0);
UVP(SithAIUtilVecSetWpnt, uvp_sw_actor, 5, 1);
UVP(SithAIUtilVecSetWpnt, uvp_sw_null, 5, 2);
UVP(SithAIUtilVecSetWpnt, uvp_sw_free, 5, 4);
UVP(SithAIUtilVecSetWpnt, uvp_sw_outofrangeactor, 70, 1);

// ---- AISetWpntRank / AISetWpntFlags / AIClearWpntFlags: { wpntIdx, value }
UVP(SithAIUtilVecWpntValue, uvp_wv_0, 3, 0);
UVP(SithAIUtilVecWpntValue, uvp_wv_1, 3, 1);
UVP(SithAIUtilVecWpntValue, uvp_wv_5, 59, 5);
UVP(SithAIUtilVecWpntValue, uvp_wv_254, 3, 254);
UVP(SithAIUtilVecWpntValue, uvp_wv_255, 3, 255);
UVP(SithAIUtilVecWpntValue, uvp_wv_256, 3, 256);
UVP(SithAIUtilVecWpntValue, uvp_wv_1000, 3, 1000);
UVP(SithAIUtilVecWpntValue, uvp_wv_max, 3, 0xFFFFFFFFu);
UVP(SithAIUtilVecWpntValue, uvp_wv_outofrange, 60, 7);
UVP(SithAIUtilVecWpntValue, uvp_wv_huge, 0xFFFFFFFFu, 7);
UVP(SithAIUtilVecWpntValue, uvp_wv_disabled, 3, SITH_AIWPNT_DISABLED);
UVP(SithAIUtilVecWpntValue, uvp_wv_layer1, 3, SITH_AIWPNT_LAYER1);
UVP(SithAIUtilVecWpntValue, uvp_wv_layers, 0, SITH_AIWPNT_ALLLAYERS);
UVP(SithAIUtilVecWpntValue, uvp_wv_flagsmask, 3, 0xFFFFFF00u);
UVP(SithAIUtilVecWpntValue, uvp_wv_lowbits, 3, 0x7Fu);
UVP(SithAIUtilVecWpntValue, uvp_wv_belowdisabled, 3, 0xFFu);
UVP(SithAIUtilVecWpntValue, uvp_wv_lowandhigh, 3, 0x1234u);
UVP(SithAIUtilVecWpntValue, uvp_wv_allones, 3, 0xFFFFFFFFu);
UVP(SithAIUtilVecWpntValue, uvp_wv_zeroflags, 3, 0u);
UVP(SithAIUtilVecWpntValue, uvp_wv_flagsoutofrange, 61, SITH_AIWPNT_LAYER2);

// ---- AISetActiveWpntLayer
static const uint32_t uvp_ly_0 = 0;
static const uint32_t uvp_ly_layer0 = SITH_AIWPNT_LAYER0;
static const uint32_t uvp_ly_layer3 = SITH_AIWPNT_LAYER3;
static const uint32_t uvp_ly_layer5 = SITH_AIWPNT_LAYER5;
static const uint32_t uvp_ly_all = SITH_AIWPNT_ALLLAYERS;
static const uint32_t uvp_ly_two = SITH_AIWPNT_LAYER1 | SITH_AIWPNT_LAYER4;
static const uint32_t uvp_ly_invalidlow = 0x1;
static const uint32_t uvp_ly_invalidhigh = 0x40000;
static const uint32_t uvp_ly_disabled = SITH_AIWPNT_DISABLED;
static const uint32_t uvp_ly_allones = 0xFFFFFFFFu;
static const uint32_t uvp_ly_layerplusinvalid = SITH_AIWPNT_LAYER2 | 0x7;

// ---- AIConnectWpnts: { a, b, both, fullA, fullB }
UVP(SithAIUtilVecConnect, uvp_cw_oneway, 0, 3, 0, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_both, 0, 3, 1, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_bothvalue2, 1, 4, 2, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_bothneg, 2, 5, -1, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_self, 2, 2, 1, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_aoutofrange, 60, 3, 1, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_boutofrange, 3, 60, 1, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_bothoutofrange, 100, 200, 1, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_ainvalid, 8, 3, 1, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_binvalid, 3, 8, 1, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_bothinvalid, 8, 9, 1, 0, 0);
UVP(SithAIUtilVecConnect, uvp_cw_fulla, 0, 3, 0, 1, 0);
UVP(SithAIUtilVecConnect, uvp_cw_fulla_both, 0, 3, 1, 1, 0);
UVP(SithAIUtilVecConnect, uvp_cw_fullb_both, 0, 3, 1, 0, 1);
UVP(SithAIUtilVecConnect, uvp_cw_fullb_oneway, 0, 3, 0, 0, 1);
UVP(SithAIUtilVecConnect, uvp_cw_fullboth, 0, 3, 1, 1, 1);

// ---- Waypoint owner table: { occupied bits, own slot, degTurn }
UVP(SithAIUtilVecOwners, uvp_ow_empty, 0, -1, 45.0f);
UVP(SithAIUtilVecOwners, uvp_ow_emptydeg0, 0, -1, 0.0f);
UVP(SithAIUtilVecOwners, uvp_ow_owninslot3, 0x0007, 3, 90.0f);
UVP(SithAIUtilVecOwners, uvp_ow_owninslot0, 0x0000, 0, 5.0f);
UVP(SithAIUtilVecOwners, uvp_ow_ownlast, 0x01FF, 9, 180.0f);
UVP(SithAIUtilVecOwners, uvp_ow_full, 0x03FF, -1, 45.0f);
UVP(SithAIUtilVecOwners, uvp_ow_freefirstthenown, 0x03E0, 7, 30.0f);
UVP(SithAIUtilVecOwners, uvp_ow_firstfreethenmatch, 0x00F1, 7, 200.0f);
UVP(SithAIUtilVecOwners, uvp_ow_onefreelast, 0x01FF, -1, 4.0f);
UVP(SithAIUtilVecOwners, uvp_ow_middlefree, 0x03EF, -1, -10.0f);
UVP(SithAIUtilVecOwners, uvp_ow_degtiny, 0x0003, -1, 4.999f);
UVP(SithAIUtilVecOwners, uvp_ow_deghuge, 0x0003, 2, 1000.0f);
UVP(SithAIUtilVecOwners, uvp_ow_fullown, 0x01FF, 9, 60.0f);

// ---- CompareAIWpntDistances: { bNull1, bNull2, dist1, dist2 }
UVP(SithAIUtilVecCompare, uvp_cd_less, 0, 0, 1.0f, 2.0f);
UVP(SithAIUtilVecCompare, uvp_cd_greater, 0, 0, 5.0f, 2.0f);
UVP(SithAIUtilVecCompare, uvp_cd_equal, 0, 0, 3.0f, 3.0f);
UVP(SithAIUtilVecCompare, uvp_cd_null1, 1, 0, 3.0f, 3.0f);
UVP(SithAIUtilVecCompare, uvp_cd_null2, 0, 1, 3.0f, 4.0f);
UVP(SithAIUtilVecCompare, uvp_cd_bothnull, 1, 1, 3.0f, 4.0f);
UVP(SithAIUtilVecCompare, uvp_cd_negative, 0, 0, -1.0f, -2.0f);
UVP(SithAIUtilVecCompare, uvp_cd_negzero, 0, 0, -0.0f, 0.0f);
UVP(SithAIUtilVecCompare, uvp_cd_tiny, 0, 0, 1.0000001f, 1.0f);
UVP(SithAIUtilVecCompare, uvp_cd_denormal, 0, 0, 1e-45f, 0.0f);

// ---- AIGetWpntThing: thing number of the waypoint
static const int uvp_gt_0 = 0;
static const int uvp_gt_ghost = 12;
static const int uvp_gt_last = 39;
static const int uvp_gt_outside = 44;
static const int uvp_gt_neg1 = -1;

// ---- RankWpntsFromThing / RankWpntsFromPlayer: { graph, thing, layer, player flags, player, px, py, pz }
#define UVRANK(id, graph, thing, layer, pflags, player, x, y, z) \
    UVP(SithAIUtilVecRank, uvp_rank_##id, graph, thing, layer, pflags, player, x, y, z)

UVRANK(chain_first, SITHAIUTILVEC_GRAPH_CHAIN, 0, 0, 0, 0, 0, 0, 0);
UVRANK(chain_mid, SITHAIUTILVEC_GRAPH_CHAIN, 1, 0, 0, 0, 0, 0, 0);
UVRANK(chain_last, SITHAIUTILVEC_GRAPH_CHAIN, 2, 0, 0, 0, 0, 0, 0);
UVRANK(chain_nowpnt, SITHAIUTILVEC_GRAPH_CHAIN, 3, 0, 0, 0, 0, 0, 0);
UVRANK(chain_owner, SITHAIUTILVEC_GRAPH_CHAIN, 4, 0, 0, 0, 0, 0, 0);
UVRANK(disabled_first, SITHAIUTILVEC_GRAPH_DISABLED, 0, 0, 0, 0, 0, 0, 0);
UVRANK(disabled_last, SITHAIUTILVEC_GRAPH_DISABLED, 2, 0, 0, 0, 0, 0, 0);
UVRANK(otherlayer_first, SITHAIUTILVEC_GRAPH_OTHERLAYER, 0, 0, 0, 0, 0, 0, 0);
UVRANK(otherlayer_layer1, SITHAIUTILVEC_GRAPH_OTHERLAYER, 0, SITH_AIWPNT_LAYER1, 0, 0, 0, 0, 0);
UVRANK(otherlayer_alllayers, SITHAIUTILVEC_GRAPH_OTHERLAYER, 1, SITH_AIWPNT_ALLLAYERS, 0, 0, 0, 0, 0);
UVRANK(star_hub, SITHAIUTILVEC_GRAPH_STAR, 0, 0, 0, 0, 0, 0, 0);
UVRANK(star_leaf, SITHAIUTILVEC_GRAPH_STAR, 2, 0, 0, 0, 0, 0, 0);
UVRANK(ring_first, SITHAIUTILVEC_GRAPH_RING, 0, 0, 0, 0, 0, 0, 0);
UVRANK(ring_tail, SITHAIUTILVEC_GRAPH_RING, 2, 0, 0, 0, 0, 0, 0);
UVRANK(split_first, SITHAIUTILVEC_GRAPH_SPLIT, 0, 0, 0, 0, 0, 0, 0);
UVRANK(split_second, SITHAIUTILVEC_GRAPH_SPLIT, 2, 0, 0, 0, 0, 0, 0);
UVRANK(highrank_first, SITHAIUTILVEC_GRAPH_HIGHRANK, 0, 0, 0, 0, 0, 0, 0);
UVRANK(oneway_first, SITHAIUTILVEC_GRAPH_ONEWAY, 0, 0, 0, 0, 0, 0, 0);
UVRANK(oneway_last, SITHAIUTILVEC_GRAPH_ONEWAY, 2, 0, 0, 0, 0, 0, 0);
UVRANK(none_any, SITHAIUTILVEC_GRAPH_NONE, 3, 0, 0, 0, 0, 0, 0);

UVRANK(player_none, SITHAIUTILVEC_GRAPH_CHAIN, 0, 0, 0, 0, 0, 0, 0);
UVRANK(player_chain, SITHAIUTILVEC_GRAPH_CHAIN, 0, 0, 0, 1, 4.5f, 0.5f, 0);
UVRANK(player_chain_far, SITHAIUTILVEC_GRAPH_CHAIN, 0, 0, 0, 1, 40.0f, 0.5f, 0);
UVRANK(player_invisible, SITHAIUTILVEC_GRAPH_CHAIN, 0, 0, SITH_AF_INVISIBLE, 2, 4.5f, 0.5f, 0);
UVRANK(player_onwaypoint, SITHAIUTILVEC_GRAPH_CHAIN, 0, 0, 0, 3, 6.0f, 0.1f, 0);
UVRANK(player_star, SITHAIUTILVEC_GRAPH_STAR, 0, 0, 0, 1, 10.0f, 0.5f, 0);
UVRANK(player_ring, SITHAIUTILVEC_GRAPH_RING, 0, 0, 0, 1, 7.5f, 1.0f, 0);
UVRANK(player_ring_layer1, SITHAIUTILVEC_GRAPH_RING, 0, SITH_AIWPNT_LAYER1, 0, 1, 7.5f, 1.0f, 0);
UVRANK(player_disabled, SITHAIUTILVEC_GRAPH_DISABLED, 0, 0, 0, 1, 6.2f, 0.2f, 0);
UVRANK(player_split, SITHAIUTILVEC_GRAPH_SPLIT, 0, 0, 0, 1, 11.0f, 0.5f, 0);
UVRANK(player_nowpnts, SITHAIUTILVEC_GRAPH_NONE, 0, 0, 0, 1, 1.0f, 0.5f, 0);
UVRANK(player_highrank, SITHAIUTILVEC_GRAPH_HIGHRANK, 0, 0, 0, 1, 1.0f, 0.5f, 0);
UVRANK(player_invisibleflags, SITHAIUTILVEC_GRAPH_CHAIN, 0, 0, (uint32_t)~SITH_AF_INVISIBLE, 1, 4.5f, 0.5f, 0);

// ---- SelectWpntLink: { mode, flee, target, player, priorities, ranks, disabled, otherLayer, numLinks, links, linkNum,
//                        ownerPrev, owner x y, flee x y, seed }
#define UVSEL(id, mode, flee, tgt, plr, prio, rank, dis, lay, n, links, linkNum, prev, ox, oy, fx, fy, seed) \
    UVP(SithAIUtilVecSelect, uvp_sel_##id, mode, flee, tgt, plr, prio, rank, dis, lay, n, links, linkNum, prev, ox, oy, fx, fy, seed)
#define P6(a, b, c, d, e, f) { a, b, c, d, e, f }

// Plain selection by the priority in the rank bits of the flags
UVSEL(plain_ties, 0, 0, 0, 0, P6(0, 3, 3, 1, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(plain_ties_seed2, 0, 0, 0, 0, P6(0, 3, 3, 1, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 7);
UVSEL(plain_ties_seed3, 0, 0, 0, 0, P6(0, 3, 3, 3, 3, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 0, 0, 19);
UVSEL(plain_higher, 0, 0, 0, 0, P6(0, 1, 5, 2, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(plain_lower, 0, 0, 0, 0, P6(0, 5, 1, 1, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(plain_linknum1, 0, 0, 0, 0, P6(0, 1, 2, 3, 4, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 1, -1, 0, 0, 0, 0, 1);
UVSEL(plain_disabled, 0, 0, 0, 0, P6(0, 1, 9, 2, 0, 0), P6(0, 0, 0, 0, 0, 0), 0x04, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(plain_otherlayer, 0, 0, 0, 0, P6(0, 1, 2, 9, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0x08, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(plain_single, 0, 0, 0, 0, P6(0, 1, 9, 2, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 1, P6(1, 0, 0, 0, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(plain_lastlink, 0, 0, 0, 0, P6(0, 1, 9, 2, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 2, -1, 0, 0, 0, 0, 1);
UVSEL(plain_leaddisabled, 0, 0, 0, 0, P6(0, 9, 3, 2, 0, 0), P6(0, 0, 0, 0, 0, 0), 0x02, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(plain_attackingtargetless, 0, 0, 0, 0, P6(0, 1, 2, 3, 4, 5), P6(0, 0, 0, 0, 0, 0), 0, 0, 5, P6(1, 2, 3, 4, 5, 0), 0, 2, 0, 0, 0, 0, 1);

// Attacking and searching: the waypoint the AI came from is penalized, the random flip
UVSEL(attack_backtrack, SITHAI_MODE_ATTACKING, 0, 0, 0, P6(0, 3, 3, 1, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, 2, 0, 0, 0, 0, 1);
UVSEL(attack_backtrack_lead, SITHAI_MODE_ATTACKING, 0, 0, 0, P6(0, 3, 3, 1, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, 1, 0, 0, 0, 0, 1);
UVSEL(attack_lower_seed1, SITHAI_MODE_ATTACKING, 0, 0, 0, P6(0, 6, 2, 6, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(attack_lower_seed2, SITHAI_MODE_ATTACKING, 0, 0, 0, P6(0, 6, 5, 6, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 12345);
UVSEL(attack_lower_seed3, SITHAI_MODE_ATTACKING, 0, 0, 0, P6(0, 6, 5, 6, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 777);
UVSEL(attack_lower_wide, SITHAI_MODE_ATTACKING, 0, 0, 0, P6(0, 12, 0, 12, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 55);
UVSEL(attack_higher, SITHAI_MODE_ATTACKING, 0, 0, 0, P6(0, 1, 5, 2, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(search_ties, SITHAI_MODE_SEARCHING, 0, 0, 0, P6(0, 4, 4, 4, 4, 4), P6(0, 0, 0, 0, 0, 0), 0, 0, 5, P6(1, 2, 3, 4, 5, 0), 0, 3, 0, 0, 0, 0, 3);
UVSEL(search_lower, SITHAI_MODE_SEARCHING, 0, 0, 0, P6(0, 9, 1, 8, 2, 7), P6(0, 0, 0, 0, 0, 0), 0, 0, 5, P6(1, 2, 3, 4, 5, 0), 0, -1, 0, 0, 0, 0, 31);
UVSEL(search_lower_seed2, SITHAI_MODE_SEARCHING, 0, 0, 0, P6(0, 9, 1, 8, 2, 7), P6(0, 0, 0, 0, 0, 0), 0, 0, 5, P6(1, 2, 3, 4, 5, 0), 0, -1, 0, 0, 0, 0, 4242);
UVSEL(attack_flee_nothing, SITHAI_MODE_ATTACKING | SITHAI_MODE_FLEEING, 0, 0, 0, P6(0, 3, 3, 1, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, 2, 0, 0, 0, 0, 1);

// Hunting: the ranks come from the player (thing 1 at the origin, or at the waypoint 3 position)
UVSEL(hunt_target_playernear0, SITHAI_MODE_HUNTING, 0, 1, 1, P6(0, 0, 0, 0, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(hunt_target_backtrack, SITHAI_MODE_HUNTING, 0, 1, 1, P6(0, 0, 0, 0, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, 1, 0, 0, 0, 0, 1);
UVSEL(hunt_target_backtrack_lead, SITHAI_MODE_HUNTING, 0, 1, 1, P6(0, 0, 0, 0, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, 1, 0, 0, 0, 0, 1);
UVSEL(hunt_target_noplayer, SITHAI_MODE_HUNTING, 0, 1, 0, P6(0, 0, 0, 0, 0, 0), P6(0, 7, 9, 2, 9, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(hunt_target_noplayer_ranklower, SITHAI_MODE_HUNTING, 0, 1, 0, P6(0, 0, 0, 0, 0, 0), P6(0, 9, 2, 2, 1, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(hunt_target_noplayer_ranktie, SITHAI_MODE_HUNTING, 0, 1, 0, P6(0, 0, 0, 0, 0, 0), P6(0, 5, 5, 5, 5, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 0, 0, 9);
UVSEL(hunt_notarget, SITHAI_MODE_HUNTING, 0, 0, 1, P6(0, 3, 3, 1, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(hunt_attacking_notarget, SITHAI_MODE_HUNTING | SITHAI_MODE_ATTACKING, 0, 0, 1, P6(0, 3, 3, 1, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 3, P6(1, 2, 3, 0, 0, 0), 0, 2, 0, 0, 0, 0, 1);
UVSEL(hunt_flee_toplayer, SITHAI_MODE_HUNTING | SITHAI_MODE_FLEEING, 1, 1, 1, P6(0, 0, 0, 0, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 1.0f, 0, 1);

// Fleeing from a thing (not to a waypoint): max rank penalty, facing the flee-from thing, rank steps
UVSEL(flee_ranks, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 0, 0, 0, 0, 0), P6(0, 10, 12, 8, 6, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 9.0f, 0, 1);
UVSEL(flee_ranks_leadmax, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 0, 0, 0, 0, 0), P6(0, 255, 12, 8, 6, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 9.0f, 0, 1);
UVSEL(flee_ranks_nextmax, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 4, 4, 4, 4, 0), P6(0, 10, 255, 8, 6, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 9.0f, 0, 1);
UVSEL(flee_ranks_bothmax, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 4, 4, 4, 4, 0), P6(0, 255, 255, 8, 6, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 9.0f, 0, 1);
UVSEL(flee_facing_close, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 5, 5, 5, 5, 0), P6(0, 3, 3, 3, 3, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 1.0f, 0, 1);
UVSEL(flee_facing_far, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 5, 5, 5, 5, 0), P6(0, 3, 3, 3, 3, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 6.0f, 1.0f, 1);
UVSEL(flee_facing_mid, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 5, 5, 5, 5, 0), P6(0, 3, 3, 3, 3, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 0.75f, 0.3f, 1);
UVSEL(flee_facing_behind, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 5, 5, 5, 5, 0), P6(0, 3, 3, 3, 3, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, -3.0f, 0.0f, 1);
UVSEL(flee_backtrack, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 5, 5, 5, 5, 0), P6(0, 3, 3, 3, 3, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, 2, 0, 0, -3.0f, 0.0f, 1);
UVSEL(flee_backtrack_lead, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 5, 5, 5, 5, 0), P6(0, 3, 3, 3, 3, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, 1, 0, 0, -3.0f, 0.0f, 1);
UVSEL(flee_ranksteps, SITHAI_MODE_FLEEING, 1, 0, 0, P6(0, 5, 5, 5, 5, 0), P6(0, 3, 9, 1, 12, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, -3.0f, 0.0f, 1);
UVSEL(flee_playerranked, SITHAI_MODE_FLEEING, 1, 0, 1, P6(0, 5, 5, 5, 5, 0), P6(0, 3, 9, 1, 12, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 7.0f, 0.5f, 1);
UVSEL(flee_noflee, SITHAI_MODE_FLEEING, 0, 0, 0, P6(0, 2, 6, 5, 5, 0), P6(0, 3, 9, 1, 12, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 7.0f, 0.5f, 1);
UVSEL(flee_noflee_attacking, SITHAI_MODE_FLEEING | SITHAI_MODE_ATTACKING, 0, 0, 0, P6(0, 2, 6, 5, 5, 0), P6(0, 3, 9, 1, 12, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, 3, 0, 0, 7.0f, 0.5f, 1);

// Fleeing to a waypoint: the ranks come from the flee-from thing (a waypoint ghost)
UVSEL(fleewpnt_actorthing, SITHAI_MODE_FLEEING | SITHAI_MODE_FLEEINGTOWAYPOINT, 1, 0, 0, P6(0, 0, 0, 0, 0, 0), P6(0, 3, 9, 1, 12, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 7.0f, 0.5f, 1);
UVSEL(fleewpnt_ghost, SITHAI_MODE_FLEEING | SITHAI_MODE_FLEEINGTOWAYPOINT, 2, 0, 0, P6(0, 0, 0, 0, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(fleewpnt_ghost_backtrack, SITHAI_MODE_FLEEING | SITHAI_MODE_FLEEINGTOWAYPOINT, 2, 0, 0, P6(0, 0, 0, 0, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, 3, 0, 0, 0, 0, 1);
UVSEL(fleewpnt_ghost_backtrack_lead, SITHAI_MODE_FLEEING | SITHAI_MODE_FLEEINGTOWAYPOINT, 2, 0, 0, P6(0, 0, 0, 0, 0, 0), P6(0, 0, 0, 0, 0, 0), 0, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, 1, 0, 0, 0, 0, 1);
UVSEL(fleewpnt_ghost_disabled, SITHAI_MODE_FLEEING | SITHAI_MODE_FLEEINGTOWAYPOINT, 2, 0, 0, P6(0, 0, 0, 0, 0, 0), P6(0, 0, 0, 0, 0, 0), 0x08, 0, 4, P6(1, 2, 3, 4, 0, 0), 0, -1, 0, 0, 0, 0, 1);
UVSEL(plain_manylinks, 0, 0, 0, 0, P6(0, 2, 4, 4, 6, 6), P6(0, 0, 0, 0, 0, 0), 0x04, 0x10, 5, P6(1, 2, 3, 4, 5, 0), 0, 4, 0, 0, 0, 0, 5);

// ---- Scripted collision searches. A search returns up to 4 collisions to the popping loop of the called function.
#define COLL_WALL(d)         { SITHCOLLISION_WORLD, -1, 0, 0, 0, d, { 0.0f, -1.0f, 0.0f }, 0.0f }
#define COLL_FLOOR(d)        { SITHCOLLISION_WORLD, -1, SITH_SURFACE_ISFLOOR, 0, 0, d, { 0.0f, 0.0f, 1.0f }, 1.0f }
#define COLL_SURF(d, f, nz)  { SITHCOLLISION_WORLD, -1, f, 0, 0, d, { 0.0f, 0.0f, 1.0f }, nz }
#define COLL_THING(d)        { SITHCOLLISION_THING, 5, 0, 0, 0, d, { 1.0f, 0.0f, 0.0f }, 0.0f }
#define COLL_ADJ(d, af, sec, sf) { SITHCOLLISION_ADJOINTOUCH, -1, sf, af, sec, d, { 0.0f, 1.0f, 0.0f }, 0.0f }
#define COLL_ADJCROSS(d, af) { SITHCOLLISION_ADJOINCROSS, -1, 0, af, 2, d, { 0.0f, 1.0f, 0.0f }, 0.0f }
#define COLL_ADJWORLD(d, af, sec, sf, nz) { SITHCOLLISION_ADJOINTOUCH | SITHCOLLISION_WORLD, -1, sf, af, sec, d, { 0.0f, 0.0f, 1.0f }, nz }
#define SEARCH(...)          { sizeof((SithAIUtilVecColl[]){ __VA_ARGS__ }) / sizeof(SithAIUtilVecColl), { __VA_ARGS__ } }

#define SEARCH_NONE         { 0, { { 0 } } }
#define UVSEARCH(name, ...) static const SithAIUtilVecSearch name = SEARCH(__VA_ARGS__)
static const SithAIUtilVecSearch uvs_none = { 0, { { 0 } } };
UVSEARCH(uvs_wall, COLL_WALL(1.0f));
UVSEARCH(uvs_wallnear, COLL_WALL(0.05f));
UVSEARCH(uvs_wallzero, COLL_WALL(0.0f));
UVSEARCH(uvs_wallmid, COLL_WALL(0.6f));
UVSEARCH(uvs_floor, COLL_FLOOR(1.0f));
UVSEARCH(uvs_floorthenwall, COLL_FLOOR(0.5f), COLL_WALL(1.5f));
UVSEARCH(uvs_thing, COLL_THING(2.0f));
UVSEARCH(uvs_thingnear, COLL_THING(0.1f));
UVSEARCH(uvs_adjmove, COLL_ADJ(0.5f, SITH_ADJOIN_MOVE, 2, 0));
UVSEARCH(uvs_adjmovethenwall, COLL_ADJ(0.5f, SITH_ADJOIN_MOVE, 2, 0), COLL_WALL(1.25f));
UVSEARCH(uvs_adjvisible, COLL_ADJ(0.5f, SITH_ADJOIN_VISIBLE, 2, 0));
UVSEARCH(uvs_adjcross, COLL_ADJCROSS(0.75f, SITH_ADJOIN_VISIBLE));
UVSEARCH(uvs_adjcrossmove, COLL_ADJCROSS(0.75f, SITH_ADJOIN_MOVE));

// ---- CheckPathToPoint: { bDetectThings, bSkipFloor, target, radius, searches }
#define UVPOINT(id, things, skip, tx, ty, tz, radius, ...) \
    static const SithAIUtilVecSearch uvq_pt_##id[] = { __VA_ARGS__ };    \
    UVP(SithAIUtilVecPoint, uvp_pt_##id, things, skip, V3(tx, ty, tz), radius, uvq_pt_##id, sizeof(uvq_pt_##id) / sizeof(uvq_pt_##id[0]))

UVPOINT(clear, 1, 0, 5, 0, 0, 0.25f, SEARCH_NONE);
UVPOINT(clear_nothings, 0, 0, 5, 0, 0, 0.25f, SEARCH_NONE);
UVPOINT(wall, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_WALL(1.0f)));
UVPOINT(floor_ignored, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_FLOOR(1.0f)));
UVPOINT(floor_blocks_skipfloor, 1, 1, 5, 0, 0, 0.25f, SEARCH(COLL_FLOOR(1.0f)));
UVPOINT(floor_then_wall, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_FLOOR(0.5f), COLL_WALL(1.5f)));
UVPOINT(floor_then_wall_skipfloor, 1, 1, 5, 0, 0, 0.25f, SEARCH(COLL_FLOOR(0.5f), COLL_WALL(1.5f)));
UVPOINT(adjoin_move_ignored, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_ADJ(0.5f, SITH_ADJOIN_MOVE, 2, 0)));
UVPOINT(adjoin_move_then_wall, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_ADJ(0.5f, SITH_ADJOIN_MOVE, 2, 0), COLL_WALL(1.25f)));
UVPOINT(adjoin_visible_blocks, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_ADJ(0.5f, SITH_ADJOIN_VISIBLE, 2, 0)));
UVPOINT(adjoin_cross_blocks, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_ADJCROSS(0.75f, SITH_ADJOIN_VISIBLE)));
UVPOINT(adjoin_cross_move_ignored, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_ADJCROSS(0.75f, SITH_ADJOIN_MOVE)));
UVPOINT(adjoin_move_floor_adjoin, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_ADJWORLD(0.5f, SITH_ADJOIN_MOVE, 2, SITH_SURFACE_ISFLOOR, 1.0f), COLL_ADJWORLD(0.9f, SITH_ADJOIN_VISIBLE, 2, 0, 0.0f)));
UVPOINT(thing, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_THING(2.0f)));
UVPOINT(thing_nodetect, 0, 0, 5, 0, 0, 0.5f, SEARCH(COLL_THING(2.0f)));
UVPOINT(onself, 1, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVPOINT(onself_wall, 1, 0, 0, 0, 0, 0.25f, SEARCH(COLL_WALL(0.0f)));
UVPOINT(diagonal_wall, 1, 0, 3, 4, 12, 1.0f, SEARCH(COLL_WALL(8.5f)));
UVPOINT(second_ignored_first_blocks, 1, 0, 5, 0, 0, 0.25f, SEARCH(COLL_FLOOR(0.1f), COLL_ADJ(0.2f, SITH_ADJOIN_MOVE, 2, 0), COLL_THING(0.4f), COLL_WALL(0.8f)));

// ---- CheckFloorAtPos / CheckPosition. Fields: { ownerMode, ownerUvec, ownerThingFlags, ownerSectorFlags,
//   foundSectorFlags, sectorMode, ownerPhysFlags, ownerAttachFlags, ownerAttachedSurface, ownerAttachedThing,
//   ownerTail, allowedSurfaceTypes, maxStep, thingAttachFlags, thingAttachedToOwner, thingFlags, thingType,
//   adjoinSectorFlags, bNoCollision, searches }
#define UVFLOOR(id, mode, uvec, tflags, sflags, fflags, smode, phys, attach, asurf, athing, tail, allowed, step, tattach, tonowner, thflags, thtype, adjflags, nocoll, ...) \
    static const SithAIUtilVecSearch uvq_fl_##id[] = { __VA_ARGS__ };                                                                                                     \
    UVP(SithAIUtilVecFloor, uvp_fl_##id, mode, uvec, tflags, sflags, fflags, smode, phys, attach, asurf, athing, tail, allowed, step, tattach, tonowner, thflags, thtype, adjflags, nocoll, uvq_fl_##id, sizeof(uvq_fl_##id) / sizeof(uvq_fl_##id[0]))

#define UP V3(0.0f, 0.0f, 1.0f)
#define DOWN V3(0.0f, 0.0f, -1.0f)
#define PLAIN(id, ...) UVFLOOR(id, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, __VA_ARGS__)

PLAIN(nothing, SEARCH_NONE);
PLAIN(floor, SEARCH(COLL_FLOOR(0.5f)));
PLAIN(floor_nocoll_null, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(floor_nocollptr_off, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 0, SEARCH(COLL_FLOOR(0.5f)));
PLAIN(wall, SEARCH(COLL_WALL(0.5f)));
PLAIN(noaimove, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_NOAIMOVE | SITH_SURFACE_ISFLOOR, 1.0f)));
PLAIN(lava, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_LAVA | SITH_SURFACE_ISFLOOR, 1.0f)));
UVFLOOR(lava_allowed, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, SITH_SURFACE_LAVA | SITH_SURFACE_METAL, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_LAVA | SITH_SURFACE_ISFLOOR, 1.0f)));
UVFLOOR(allowed_metal_ok, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, SITH_SURFACE_METAL, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_METAL | SITH_SURFACE_ISFLOOR, 1.0f)));
UVFLOOR(allowed_not_matching, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, SITH_SURFACE_METAL, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_EARTH | SITH_SURFACE_ISFLOOR, 1.0f)));
UVFLOOR(allowed_lava_not_listed, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, SITH_SURFACE_EARTH, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_LAVA | SITH_SURFACE_EARTH | SITH_SURFACE_ISFLOOR, 1.0f)));
PLAIN(adjoin_noenter, SEARCH(COLL_ADJ(0.5f, SITH_ADJOIN_VISIBLE, 2, 0)));
UVFLOOR(adjoin_noenter_flag, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, SITH_SECTOR_NOACTORENTER, 1, SEARCH(COLL_ADJ(0.5f, SITH_ADJOIN_VISIBLE, 2, 0), COLL_FLOOR(1.0f)));
UVFLOOR(adjoin_water_destroyed, 0, UP, SITH_TF_WATERDESTROYED, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, SITH_SECTOR_UNDERWATER, 1, SEARCH(COLL_ADJCROSS(0.5f, SITH_ADJOIN_VISIBLE), COLL_FLOOR(1.0f)));
UVFLOOR(adjoin_water_not_destroyed, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, SITH_SECTOR_UNDERWATER, 1, SEARCH(COLL_ADJCROSS(0.5f, SITH_ADJOIN_VISIBLE), COLL_FLOOR(1.0f)));
UVFLOOR(adjoin_water_already_in, 0, UP, SITH_TF_WATERDESTROYED, SITH_SECTOR_UNDERWATER, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, SITH_SECTOR_UNDERWATER, 1, SEARCH(COLL_ADJCROSS(0.5f, SITH_ADJOIN_VISIBLE), COLL_FLOOR(1.0f)));
UVFLOOR(adjoin_plain_then_floor, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_ADJ(0.25f, SITH_ADJOIN_VISIBLE, 2, 0), COLL_FLOOR(1.0f)));
UVFLOOR(adjoin_world_noaimove, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_ADJWORLD(0.25f, SITH_ADJOIN_VISIBLE, 2, SITH_SURFACE_NOAIMOVE, 0.0f)));
PLAIN(wall_then_floor, SEARCH(COLL_WALL(0.5f), COLL_FLOOR(1.0f)));
UVFLOOR(alignup_attached_same, 0, UP, 0, 0, 0, 0, SITH_PF_ALIGNUP, SITH_ATTACH_SURFACE, 0, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(alignup_attached_other_steep, 0, UP, 0, 0, 0, 0, SITH_PF_ALIGNUP, SITH_ATTACH_SURFACE, 1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_ISFLOOR, 0.5f)));
UVFLOOR(alignup_attached_other_flat, 0, UP, 0, 0, 0, 0, SITH_PF_ALIGNUP, SITH_ATTACH_SURFACE, 1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_ISFLOOR, 0.9f)));
UVFLOOR(alignup_attached_other_edge, 0, UP, 0, 0, 0, 0, SITH_PF_ALIGNUP, SITH_ATTACH_SURFACE, 1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_ISFLOOR, 0.788f)));
UVFLOOR(alignup_not_attached, 0, UP, 0, 0, 0, 0, SITH_PF_ALIGNUP, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_ISFLOOR, 0.5f)));
UVFLOOR(noalignup_attached_other, 0, UP, 0, 0, 0, 0, 0, SITH_ATTACH_SURFACE, 1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_SURF(0.5f, SITH_SURFACE_ISFLOOR, 0.5f)));
UVFLOOR(attached_same_nocoll, 0, UP, 0, 0, 0, 0, 0, SITH_ATTACH_SURFACE, 0, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_WALL(0.5f)));
UVFLOOR(attached_flag_only, 0, UP, 0, 0, 0, 0, 0, SITH_ATTACH_SURFACE, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_WALL(0.5f)));
UVFLOOR(upside_down, 0, DOWN, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(tilted_threshold, 0, V3(0.0f, 0.7f, -0.7f), 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(tilted_threshold_exact, 0, V3(0.0f, 0.707106781f, -0.707f), 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(tilted_threshold_below, 0, V3(0.0f, 0.7071f, -0.7071f), 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(wallcrawl, SITHAI_MODE_WALLCRAWLING, V3(0.0f, 1.0f, 0.0f), 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(thing_attached_to_owner, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, SITH_ATTACH_THING, 1, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_attached_other, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, SITH_ATTACH_THING, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_tail, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 1, 0, 0.25f, 0, 0, 0, SITH_THING_ITEM, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_standon, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, SITH_TF_STANDON, SITH_THING_ITEM, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_blocks_item, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ITEM, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_type_player_bits, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_PLAYER, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_type_actor_bit, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_type_corpse_bits, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_CORPSE, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_type_ghost_bits, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_GHOST, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_type_weapon_bits, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_WEAPON, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_type_camera_bits, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_CAMERA, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_type_hint_bits, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_HINT, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_facing_attached, 0, UP, 0, 0, 0, 0, 0, SITH_ATTACH_THINGFACE, -1, 1, 0, 0, 0.25f, 0, 0, SITH_TF_STANDON, SITH_THING_ITEM, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_facing_other, 0, UP, 0, 0, 0, 0, 0, SITH_ATTACH_THINGFACE, -1, 0, 0, 0, 0.25f, 0, 0, SITH_TF_STANDON, SITH_THING_ITEM, 0, 1, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_nocollptr, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, SITH_TF_STANDON, SITH_THING_ITEM, 0, 0, SEARCH(COLL_THING(0.5f)));
UVFLOOR(thing_then_floor, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ITEM, 0, 1, SEARCH(COLL_THING(0.5f), COLL_FLOOR(1.0f)));
UVFLOOR(thing_ok_then_wall, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, SITH_TF_STANDON, SITH_THING_ITEM, 0, 1, SEARCH(COLL_THING(0.5f), COLL_WALL(1.0f)));
UVFLOOR(unknown_collision_type, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ITEM, 0, 1, SEARCH({ SITHCOLLISION_FACE, -1, 0, 0, 0, 0.5f, { 0.0f, 0.0f, 1.0f }, 0.0f }, COLL_FLOOR(1.0f)));
UVFLOOR(maxstep_large, 0, UP, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 100.0f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(50.0f)));

// CheckPosition: the end position is in the next sector (flags of sector 1), the start sector, or in no sector
UVFLOOR(pos_samesector, 0, UP, 0, 0, SITH_SECTOR_NOACTORENTER, 0, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(pos_nosector, 0, UP, 0, 0, 0, 2, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(pos_noenter, 0, UP, 0, 0, SITH_SECTOR_NOACTORENTER, 1, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(pos_water_destroyed, 0, UP, SITH_TF_WATERDESTROYED, 0, SITH_SECTOR_UNDERWATER, 1, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(pos_water_not_destroyed, 0, UP, 0, 0, SITH_SECTOR_UNDERWATER, 1, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(pos_water_from_water, 0, UP, SITH_TF_WATERDESTROYED, SITH_SECTOR_UNDERWATER, SITH_SECTOR_UNDERWATER, 1, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(pos_ok_floor, 0, UP, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_FLOOR(0.5f)));
UVFLOOR(pos_ok_cliff, 0, UP, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 0, SEARCH_NONE);
UVFLOOR(pos_ok_wall, 0, UP, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0.25f, 0, 0, 0, SITH_THING_ACTOR, 0, 1, SEARCH(COLL_WALL(0.5f)));

// ---- AIFindNearestWpnt: { graph, who, x, y, z, bCheckPath, layer, mode, damage, movesize, searches }
// Chain waypoints are at (0,0) (3,1.5) (6,0) (9,1.5) (12,0) (15,1.5): from (4.5, 0.5) the order is 2, 1, 0, 3, 4, 5.
#define UVNEAR(id, graph, who, x, y, z, check, layer, mode, damage, moveSize, ...)                                        \
    static const SithAIUtilVecSearch uvq_nr_##id[] = { __VA_ARGS__ };                                                    \
    UVP(SithAIUtilVecNearest, uvp_nr_##id, graph, who, x, y, z, check, layer, mode, damage, moveSize, uvq_nr_##id, sizeof(uvq_nr_##id) / sizeof(uvq_nr_##id[0]))

#define CHAIN SITHAIUTILVEC_GRAPH_CHAIN
UVNEAR(nowpnts, SITHAIUTILVEC_GRAPH_NONE, 0, 4.5f, 0.5f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(nowpnts_player, SITHAIUTILVEC_GRAPH_NONE, 1, 4.5f, 0.5f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(onwaypoint, CHAIN, 0, 6.0f, 0.0f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(nearwaypoint_edge, CHAIN, 0, 6.28f, 0.0f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(nearwaypoint_outside, CHAIN, 0, 6.3f, 0.0f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(nearwaypoint_bigsize, CHAIN, 0, 6.8f, 0.0f, 0, 0, 0, 0, 0, 1.0f, SEARCH_NONE);
UVNEAR(owner_nocheck, CHAIN, 0, 4.5f, 0.5f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(owner_nocheck_hunting, CHAIN, 0, 4.5f, 0.5f, 0, 0, 0, SITHAI_MODE_HUNTING, 0, 0.25f, SEARCH_NONE);
UVNEAR(owner_check_clear, CHAIN, 0, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(owner_check_firstblocked, CHAIN, 0, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH(COLL_WALL(1.0f)), SEARCH_NONE);
UVNEAR(owner_check_firstsecondblocked, CHAIN, 0, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_THING(1.0f)), SEARCH_NONE);
UVNEAR(owner_check_floorignored, CHAIN, 0, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH(COLL_FLOOR(1.0f)), SEARCH_NONE);
UVNEAR(owner_check_allblocked, CHAIN, 0, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)));
UVNEAR(owner_check_linkblocked, CHAIN, 0, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH_NONE, SEARCH(COLL_WALL(1.0f)));
UVNEAR(owner_check_lastblocked, CHAIN, 0, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH_NONE);
UVNEAR(player_nocheck, CHAIN, 1, 4.5f, 0.5f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(player_check_clear, CHAIN, 1, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(player_check_firstblocked, CHAIN, 1, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH(COLL_WALL(1.0f)), SEARCH_NONE);
UVNEAR(player_check_allblocked, CHAIN, 1, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)));
UVNEAR(disabled_wpnt, SITHAIUTILVEC_GRAPH_DISABLED, 0, 4.5f, 0.5f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(disabled_wpnt_player, SITHAIUTILVEC_GRAPH_DISABLED, 1, 4.5f, 0.5f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(otherlayer_wpnt, SITHAIUTILVEC_GRAPH_OTHERLAYER, 0, 10.0f, 0.5f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(otherlayer_active, SITHAIUTILVEC_GRAPH_OTHERLAYER, 0, 10.0f, 0.5f, 0, 0, SITH_AIWPNT_LAYER1, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(otherlayer_all, SITHAIUTILVEC_GRAPH_OTHERLAYER, 0, 10.0f, 0.5f, 0, 0, SITH_AIWPNT_ALLLAYERS, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(invalid_thing_45, CHAIN, 0, 4.5f, 0.5f, 0, 0, 0, 0, 1, 0.25f, SEARCH_NONE);
UVNEAR(invalid_nonghost, CHAIN, 0, 4.5f, 0.5f, 0, 0, 0, 0, 2, 0.25f, SEARCH_NONE);
UVNEAR(invalid_nothing, CHAIN, 0, 4.5f, 0.5f, 0, 0, 0, 0, 4, 0.25f, SEARCH_NONE);
UVNEAR(invalid_all3, CHAIN, 0, 4.5f, 0.5f, 0, 0, 0, 0, 7, 0.25f, SEARCH_NONE);
UVNEAR(star_hub_owner, SITHAIUTILVEC_GRAPH_STAR, 0, 1.0f, 0.25f, 0, 1, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(star_hub_owner_blocked, SITHAIUTILVEC_GRAPH_STAR, 0, 1.0f, 0.25f, 0, 1, 0, 0, 0, 0.25f, SEARCH_NONE, SEARCH(COLL_WALL(1.0f)), SEARCH_NONE, SEARCH(COLL_WALL(1.0f)));
UVNEAR(star_leaf_owner, SITHAIUTILVEC_GRAPH_STAR, 0, 8.0f, 1.0f, 0, 0, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(star_leaf_owner_check, SITHAIUTILVEC_GRAPH_STAR, 0, 8.0f, 1.0f, 0, 1, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(ring_owner, SITHAIUTILVEC_GRAPH_RING, 0, 4.0f, 0.0f, 0, 1, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(ring_owner_blocked_first, SITHAIUTILVEC_GRAPH_RING, 0, 4.0f, 0.0f, 0, 1, 0, 0, 0, 0.25f, SEARCH(COLL_WALL(1.0f)));
UVNEAR(ring_owner_attacking, SITHAIUTILVEC_GRAPH_RING, 0, 4.0f, 0.0f, 0, 1, 0, SITHAI_MODE_ATTACKING, 0, 0.25f, SEARCH_NONE);
UVNEAR(oneway_owner, SITHAIUTILVEC_GRAPH_ONEWAY, 0, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(split_owner, SITHAIUTILVEC_GRAPH_SPLIT, 0, 4.5f, 0.5f, 0, 1, 0, 0, 0, 0.25f, SEARCH_NONE);
UVNEAR(highrank_owner, SITHAIUTILVEC_GRAPH_HIGHRANK, 0, 4.5f, 0.5f, 0, 1, 0, SITHAI_MODE_ATTACKING, 0, 0.25f, SEARCH_NONE);

// ---- AIMoveToNextWpnt (var 0), AITraverseWpnts (var 1), AIAdvanceToNextWpnt (var 2): { graph, x, y, z, mode, submode,
//   wpntIdx, moveSpeed, degTurn, modeArg, movePosMask, ownerSlot, ownerWpntNum, othersFull, layer, classDegTurn,
//   bNotify, damage, searches }
#define UVMOVE(id, graph, x, y, z, mode, submode, wpnt, speed, deg, modeArg, mask, slot, wnum, others, layer, cdeg, notify, damage, ...) \
    static const SithAIUtilVecSearch uvq_mv_##id[] = { __VA_ARGS__ };                                                                   \
    UVP(SithAIUtilVecWpntMove, uvp_wm_##id, graph, x, y, z, mode, submode, wpnt, speed, deg, modeArg, mask, slot, wnum, others, layer, cdeg, notify, damage, uvq_mv_##id, sizeof(uvq_mv_##id) / sizeof(uvq_mv_##id[0]))

// MoveToNext: the nearest waypoint is found with a path check, then traversed
UVMOVE(next_nowpnts, SITHAIUTILVEC_GRAPH_NONE, 4.5f, 0.5f, 0, 0, 0, 0, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(next_basic, CHAIN, 4.5f, 0.5f, 0, 0, 0, 0, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(next_basic_movefails, CHAIN, 4.5f, 0.5f, 0, 0, 0, 0, 2.0f, 45.0f, 0, 0, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(next_moveforward, CHAIN, 4.5f, 0.5f, 0, 0, SITHAI_SUBMODE_MOVEFORWARD, 0, 2.0f, 45.0f, 1, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(next_mode1, CHAIN, 4.5f, 0.5f, 0, 0, 0, 0, 1.0f, 0.0f, 1, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(next_mode2, CHAIN, 4.5f, 0.5f, 0, 0, 0, 0, 1.0f, 0.0f, 2, 1, -1, 0, 0, 0, 0.0f, 0, 0, SEARCH_NONE);
UVMOVE(next_modeneg, CHAIN, 4.5f, 0.5f, 0, 0, 0, 0, 1.0f, 20.0f, -1, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(next_ownerfull, CHAIN, 4.5f, 0.5f, 0, 0, 0, 0, 2.0f, 45.0f, 0, 1, -1, 0, 0x3FF, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(next_blocked, CHAIN, 4.5f, 0.5f, 0, 0, 0, 0, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)), SEARCH(COLL_WALL(1.0f)));
UVMOVE(next_stationary, CHAIN, 6.0f, 0.0f, 0, 0, 0, 0, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);

// Traverse
UVMOVE(trav_basic, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_basic_movefails, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 45.0f, 0, 0, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_movefails_star, SITHAIUTILVEC_GRAPH_STAR, 0, 0, 0, 0, 0, 0, 2.0f, 45.0f, 0, 0x6, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_idx60, CHAIN, 0, 0, 0, 0, 0, 60, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_idxneg, CHAIN, 0, 0, 0, 0, 0, -1, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_noworld, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 4, SEARCH_NONE);
UVMOVE(trav_thing45, CHAIN, 0, 0, 0, 0, 0, 1, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 1, SEARCH_NONE);
UVMOVE(trav_nonghost, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 2, SEARCH_NONE);
UVMOVE(trav_deg0_class30, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 0.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_deg0_class0, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 0.0f, 0, 1, -1, 0, 0, 0, 0.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_deg3, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 3.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_deg400, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 400.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_mode1, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 45.0f, 1, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_mode2, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 45.0f, 2, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_mode100, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 45.0f, 100, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_modeneg, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 45.0f, -5, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_owneralready, CHAIN, 0, 0, 0, 0, 0, 3, 2.0f, 45.0f, 0, 1, 4, 0x0102, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_ownerfull, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 45.0f, 0, 1, -1, 0, 0x3FF, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_ownerfull_withown, CHAIN, 0, 0, 0, 0, 0, 2, 2.0f, 45.0f, 0, 1, 9, 0x0102, 0x1FF, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_wallcrawl_aligned, CHAIN, 3, 0, 0, SITHAI_MODE_WALLCRAWLING, 0, 2, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(trav_turning, CHAIN, 0, 0, 0, SITHAI_MODE_TURNING | SITHAI_MODE_MOVING, SITHAI_SUBMODE_ALIGNING, 2, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);

// Advance: the owner is registered in slot 1; waypointNum = next << 8 | previous
UVMOVE(adv_notregistered, CHAIN, 0, 0, 0, 0, 0, 0, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(adv_notregistered_notify, CHAIN, 0, 0, 0, 0, 0, 0, 2.0f, 45.0f, 0, 1, -1, 0, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_midchain, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0201, 0, 0, 30.0f, 0, 0, SEARCH_NONE);
UVMOVE(adv_midchain_notify, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0201, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_midchain_movefails, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 0, 1, 0x0201, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_midchain_prev3, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0203, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_chainend_single_back, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0504, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_chainend_single_back_cont, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, SITHAI_SUBMODE_CONTINUOUSWPNTMOTION, 0, 2.0f, 45.0f, 0, 1, 1, 0x0504, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_chainend_single_back_semi, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION, 0, 2.0f, 45.0f, 0, 1, 1, 0x0504, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_chainend_single_other, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0503, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_nolinks, SITHAIUTILVEC_GRAPH_ONEWAY, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0402, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_next_outofrange, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x3C01, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_next_255, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0xFF01, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_next_unused_wpnt, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0A01, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_star_hub, SITHAIUTILVEC_GRAPH_STAR, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0003, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_star_hub_hunting, SITHAIUTILVEC_GRAPH_STAR, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS | SITHAI_MODE_ATTACKING, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0003, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_star_hub_nobacktrack, SITHAIUTILVEC_GRAPH_STAR, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x00FF, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_ring, SITHAIUTILVEC_GRAPH_RING, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0301, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_disabled_link, SITHAIUTILVEC_GRAPH_DISABLED, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0101, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_otherlayer_link, SITHAIUTILVEC_GRAPH_OTHERLAYER, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0203, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_otherlayer_active, SITHAIUTILVEC_GRAPH_OTHERLAYER, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0203, 0, SITH_AIWPNT_LAYER1, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_otherlayer_two, SITHAIUTILVEC_GRAPH_OTHERLAYER, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0402, 0, SITH_AIWPNT_ALLLAYERS, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_hunting_mode, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS | SITHAI_MODE_HUNTING, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0201, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_fleewpnt_mode, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS | SITHAI_MODE_FLEEINGTOWAYPOINT, 0, 0, 2.0f, 45.0f, 0, 1, 1, 0x0201, 0, 0, 30.0f, 1, 0, SEARCH_NONE);
UVMOVE(adv_continuous_cleared_on_arrive, CHAIN, 0, 0, 0, SITHAI_MODE_TRAVERSEWPNTS | SITHAI_MODE_HUNTING | SITHAI_MODE_FLEEINGTOWAYPOINT, SITHAI_SUBMODE_CONTINUOUSWPNTMOTION | SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION, 0, 2.0f, 45.0f, 0, 0, 1, 0x0504, 0, 0, 30.0f, 1, 0, SEARCH_NONE);

// ---- CheckWpntMoveAlign: { registered, mode, submode, cos, goalLVec, lvec, uvec }
UVP(SithAIUtilVecAlign, uvp_al_notreg, 0, SITHAI_MODE_TURNING | SITHAI_MODE_TRAVERSEWPNTS | SITHAI_MODE_HUNTING, 0xFFFFFFFFu, 0.5f, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_aligned, 1, SITHAI_MODE_TURNING, 0xFFFFFFFFu, 0.5f, V3(0, 1, 0), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_aligned_notturning, 1, 0, 0xFFFFFFFFu, 0.5f, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_turning_off, 1, SITHAI_MODE_TURNING, 0xFFFFFFFFu, 0.5f, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_turning_edge, 1, SITHAI_MODE_TURNING, SITHAI_SUBMODE_ALIGNING, 0.5f, V3(0.5f, 0.5f, 0), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_turning_justoff, 1, SITHAI_MODE_TURNING, SITHAI_SUBMODE_ALIGNING, 0.5f, V3(0.25f, 0.75f, 0), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_cos_negative, 1, SITHAI_MODE_TURNING, SITHAI_SUBMODE_ALIGNING, -0.5f, V3(0, -1, 0), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_cos_negative_ok, 1, SITHAI_MODE_TURNING, SITHAI_SUBMODE_ALIGNING, -1.0f, V3(0, -1, 0), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_wallcrawl, 1, SITHAI_MODE_TURNING | SITHAI_MODE_WALLCRAWLING, SITHAI_SUBMODE_ALIGNING, 0.8f, V3(0, 0.6f, 0.8f), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_wallcrawl_aligned, 1, SITHAI_MODE_TURNING | SITHAI_MODE_WALLCRAWLING, SITHAI_SUBMODE_ALIGNING, 0.5f, V3(0, 0.6f, 0.8f), V3(0, 0.6f, 0.8f), V3(0, -0.8f, 0.6f));
UVP(SithAIUtilVecAlign, uvp_al_wallcrawl_notturning, 1, SITHAI_MODE_WALLCRAWLING, SITHAI_SUBMODE_ALIGNING, 0.99f, V3(0, 0.6f, 0.8f), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_flat_ignores_z, 1, SITHAI_MODE_TURNING, SITHAI_SUBMODE_ALIGNING, 0.9f, V3(0, 0.6f, 0.8f), V3(0, 1, 0), V3(0, 0, 1));
UVP(SithAIUtilVecAlign, uvp_al_tilted_up, 1, SITHAI_MODE_TURNING, SITHAI_SUBMODE_ALIGNING, 0.7f, V3(0.6f, 0.6f, 0), V3(0, 1, 0), V3(0, 0.5f, 0.866f));
UVP(SithAIUtilVecAlign, uvp_al_zerogoal, 1, SITHAI_MODE_TURNING, SITHAI_SUBMODE_ALIGNING, 0.5f, V3(0, 0, 0), V3(0, 1, 0), V3(0, 0, 1));

// ---- RenderAIWaypoints: { bRender, secGameTime, secLastUpdate, graph, layer }
UVP(SithAIUtilVecRender, uvp_rw_off, 0, 50.0f, 7.0f, CHAIN, 0);
UVP(SithAIUtilVecRender, uvp_rw_early, 1, 50.0f, 49.5f, CHAIN, 0);
UVP(SithAIUtilVecRender, uvp_rw_interval, 1, 50.0f, 49.0f, CHAIN, 0);
UVP(SithAIUtilVecRender, uvp_rw_late, 1, 50.0f, 10.0f, CHAIN, 0);
UVP(SithAIUtilVecRender, uvp_rw_nowpnts, 1, 50.0f, 10.0f, SITHAIUTILVEC_GRAPH_NONE, 0);
UVP(SithAIUtilVecRender, uvp_rw_disabled, 1, 50.0f, 10.0f, SITHAIUTILVEC_GRAPH_DISABLED, 0);
UVP(SithAIUtilVecRender, uvp_rw_otherlayer, 1, 50.0f, 10.0f, SITHAIUTILVEC_GRAPH_OTHERLAYER, 0);
UVP(SithAIUtilVecRender, uvp_rw_otherlayer_active, 1, 50.0f, 10.0f, SITHAIUTILVEC_GRAPH_OTHERLAYER, SITH_AIWPNT_LAYER1);
UVP(SithAIUtilVecRender, uvp_rw_star, 1, 50.0f, 10.0f, SITHAIUTILVEC_GRAPH_STAR, 0);
UVP(SithAIUtilVecRender, uvp_rw_ring, 1, 50.0f, 10.0f, SITHAIUTILVEC_GRAPH_RING, 0);
UVP(SithAIUtilVecRender, uvp_rw_oneway, 1, 50.0f, 10.0f, SITHAIUTILVEC_GRAPH_ONEWAY, 0);
UVP(SithAIUtilVecRender, uvp_rw_split, 1, 50.0f, 10.0f, SITHAIUTILVEC_GRAPH_SPLIT, 0);
UVP(SithAIUtilVecRender, uvp_rw_timeearly, 1, 0.5f, 0.0f, CHAIN, 0);

// ---- GetDistanceToTarget / GetDistanceToTargetPos: { viewer actor flags, viewer thing flags, target type, target thing
//   flags, target actor flags, target physics flags, target move type, target pos, target size, fov, sight, los mask,
//   start pos }. The viewer looks along +y.
#define UVDIST(id, vaf, vtf, ttype, ttf, taf, tpf, tmt, tx, ty, tz, tsize, fov, sight, los, sx, sy, sz) \
    UVP(SithAIUtilVecDist, uvp_ds_##id, vaf, vtf, ttype, ttf, taf, tpf, tmt, V3(tx, ty, tz), tsize, fov, sight, los, V3(sx, sy, sz))

#define ACT SITH_THING_ACTOR
#define PHYS SITH_MT_PHYSICS
UVDIST(visible, 0, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(visible_nolos, 0, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 0, 0, 0, 0);
UVDIST(visible_startoffset, 0, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 1.5f);
UVDIST(toofar, 0, 0, ACT, 0, 0, 0, PHYS, 0, 30, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(toofar_edge, 0, 0, ACT, 0, 0, 0, PHYS, 0, 21, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(toofar_edge_inside, 0, 0, ACT, 0, 0, 0, PHYS, 0, 20.5f, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(toofar_size0, 0, 0, ACT, 0, 0, 0, PHYS, 0, 20, 0, 0.0f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(sizebigger_than_distance, 0, 0, ACT, 0, 0, 0, PHYS, 0, 1, 0, 3.0f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(ontop, 0, 0, ACT, 0, 0, 0, PHYS, 0, 0, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(invisible_target, 0, 0, ACT, 0, SITH_AF_INVISIBLE, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(invisible_target_player, 0, 0, SITH_THING_PLAYER, 0, SITH_AF_INVISIBLE, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(invisible_target_other_type, 0, 0, SITH_THING_ITEM, 0, SITH_AF_INVISIBLE, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(invisible_seeinvisible, SITH_AF_SEEINVISIBLE, 0, ACT, 0, SITH_AF_INVISIBLE, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 0, 0, 0, 0);
UVDIST(blind_viewer, SITH_AF_BLIND, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(blind_seeinvisible, SITH_AF_BLIND | SITH_AF_SEEINVISIBLE, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 0, 0, 0, 0);
UVDIST(seeinvisible_toofar, SITH_AF_SEEINVISIBLE, 0, ACT, 0, 0, 0, PHYS, 0, 50, 0, 0.5f, -1.0f, 20.0f, 0, 0, 0, 0);
UVDIST(seeinvisible_outoffov, SITH_AF_SEEINVISIBLE, 0, ACT, 0, 0, 0, PHYS, 0, -5, 0, 0.5f, 0.5f, 20.0f, 0, 0, 0, 0);
UVDIST(target_submerged_viewer_dry, 0, 0, ACT, SITH_TF_SUBMERGED, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(target_submerged_onsurface, 0, 0, ACT, SITH_TF_SUBMERGED, 0, SITH_PF_ONWATERSURFACE, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(target_submerged_nonphysics, 0, 0, ACT, SITH_TF_SUBMERGED, 0, SITH_PF_ONWATERSURFACE, SITH_MT_NONE, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(target_submerged_both, 0, SITH_TF_SUBMERGED, ACT, SITH_TF_SUBMERGED, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(viewer_submerged_target_dry, 0, SITH_TF_SUBMERGED, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(viewer_submerged_target_dry_seeinvisible, SITH_AF_SEEINVISIBLE, SITH_TF_SUBMERGED, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_front, 0, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, 0.5f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_behind, 0, 0, ACT, 0, 0, 0, PHYS, 0, -5, 0, 0.5f, 0.5f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_side, 0, 0, ACT, 0, 0, 0, PHYS, 5, 0.1f, 0, 0.5f, 0.5f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_side_edge, 0, 0, ACT, 0, 0, 0, PHYS, 5, 5, 0, 0.0f, 0.5f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_zero_front, 0, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, 0.0f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_zero_slightside, 0, 0, ACT, 0, 0, 0, PHYS, 0.5f, 5, 0, 0.0f, 0.0f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_one_straight, 0, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, 1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_zero_exact_side, 0, 0, ACT, 0, 0, 0, PHYS, 5, 0, 0, 0.0f, 0.0f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_one, 0, 0, ACT, 0, 0, 0, PHYS, 0.5f, 5, 0, 0.0f, 1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_negative_behind_side, 0, 0, ACT, 0, 0, 0, PHYS, 1, -5, 0, 0.0f, -0.5f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_negative_behind_center, 0, 0, ACT, 0, 0, 0, PHYS, 0, -5, 0, 0.0f, -0.5f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_negative_behind_wide, 0, 0, ACT, 0, 0, 0, PHYS, 5, -1, 0, 0.0f, -0.5f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_minus1_behind, 0, 0, ACT, 0, 0, 0, PHYS, 0, -5, 0, 0.5f, -1.0f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_minus099_behind, 0, 0, ACT, 0, 0, 0, PHYS, 0, -5, 0, 0.5f, -0.99f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_big, 0, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, 2.0f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_vertical, 0, 0, ACT, 0, 0, 0, PHYS, 0, 0, 5, 0.5f, 0.5f, 20.0f, 1, 0, 0, 0);
UVDIST(fov_above_front, 0, 0, ACT, 0, 0, 0, PHYS, 0, 5, 5, 0.5f, 0.5f, 20.0f, 1, 0, 0, 0);
UVDIST(sight0, 0, 0, ACT, 0, 0, 0, PHYS, 0, 0.4f, 0, 0.2f, -1.0f, 0.0f, 1, 0, 0, 0);
UVDIST(sight_negative, 0, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, -1.0f, 1, 0, 0, 0);
UVDIST(startpos_far_from_viewer, 0, 0, ACT, 0, 0, 0, PHYS, 0, 5, 0, 0.5f, -1.0f, 20.0f, 1, 10, 10, 10);
UVDIST(startpos_equals_target, 0, 0, ACT, 0, 0, 0, PHYS, 3, 3, 3, 0.5f, -1.0f, 20.0f, 1, 3, 3, 3);
UVDIST(diagonal, 0, 0, ACT, 0, 0, 0, PHYS, 3, 4, 12, 0.5f, -1.0f, 20.0f, 0, 0, 0, 0);

// ---- CanSeeTarget: { bNullTarget, target type, distance, mode, owner actor flags, target actor flags, unknownFlags,
//   ambient, move type, physics flags, moving, seed }
#define UVSEE(id, bnull, ttype, dist, mode, oaf, taf, unk, amb, mt, pf, moving, seed) \
    UVP(SithAIUtilVecSee, uvp_cs_##id, bnull, ttype, dist, mode, oaf, taf, unk, amb, mt, pf, moving, seed)

UVSEE(nulltarget, 1, ACT, 10.0f, 0, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(itemtarget, 0, SITH_THING_ITEM, 10.0f, 0, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(ghosttarget, 0, SITH_THING_GHOST, 10.0f, 0, 0, 0, 0, 0.0f, PHYS, 0, 1, 1);
UVSEE(close_active, 0, ACT, 1.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(close_inactive, 0, ACT, 1.0f, 0, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(close_edge, 0, ACT, 2.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(close_edge_inactive, 0, ACT, 2.0f, 0, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(far_active_lit, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(far_active_lit_seed2, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 1, 2);
UVSEE(far_active_lit_seed3, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 1, 3);
UVSEE(far_inactive_lit, 0, ACT, 10.0f, 0, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(far_inactive_lit_seed5, 0, ACT, 10.0f, 0, 0, 0, 0, 1.0f, PHYS, 0, 1, 5);
UVSEE(very_far, 0, ACT, 100.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(very_far_seed9, 0, ACT, 100.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 1, 9);
UVSEE(very_far_seed100, 0, ACT, 100.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 1, 100);
UVSEE(mid3_active, 0, ACT, 3.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(mid4_inactive, 0, ACT, 4.0f, 0, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(mid5_active_dark, 0, ACT, 5.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 0.3f, PHYS, 0, 1, 1);
UVSEE(mid6_active_crouching, 0, ACT, 6.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, SITH_PF_CROUCHING, 1, 1);
UVSEE(mid5_active_still, 0, ACT, 5.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 0, 1);
UVSEE(mid5_active_crouching_still, 0, ACT, 5.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, SITH_PF_CROUCHING, 0, 1);
UVSEE(mid5_active_invisible, 0, ACT, 5.0f, SITHAI_MODE_ACTIVE, 0, SITH_AF_INVISIBLE, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(mid7_inactive_blind_dark, 0, ACT, 7.0f, 0, SITH_AF_BLIND, 0, 0, 0.2f, PHYS, SITH_PF_CROUCHING, 0, 1);
UVSEE(dark, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 0.1f, PHYS, 0, 1, 1);
UVSEE(dark_seed7, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 0.1f, PHYS, 0, 1, 7);
UVSEE(dark_seeindark, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, SITH_AF_SEEINDARK, 0, 0, 0.1f, PHYS, 0, 1, 1);
UVSEE(dark_headlight, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, SITH_AF_HEADLIGHT, 0, 0.1f, PHYS, 0, 1, 1);
UVSEE(dark_unknownflag, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 1, 0.1f, PHYS, 0, 1, 1);
UVSEE(dark_unknownflag_other, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 2, 0.1f, PHYS, 0, 1, 1);
UVSEE(half_light, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 0.5f, PHYS, 0, 1, 1);
UVSEE(lit_just_below, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 0.49f, PHYS, 0, 1, 1);
UVSEE(crouching, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, SITH_PF_CROUCHING, 1, 1);
UVSEE(crouching_seed4, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, SITH_PF_CROUCHING, 1, 4);
UVSEE(standing_still, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 0, 1);
UVSEE(standing_still_seed4, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 0, 4);
UVSEE(crouching_still, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, SITH_PF_CROUCHING, 0, 1);
UVSEE(crouching_still_seed8, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, SITH_PF_CROUCHING, 0, 8);
UVSEE(nonphysics_target, 0, ACT, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, SITH_MT_NONE, SITH_PF_CROUCHING, 0, 1);
UVSEE(headlight_crouching_ignored, 0, ACT, 10.0f, 0, 0, SITH_AF_HEADLIGHT, 0, 0.0f, PHYS, SITH_PF_CROUCHING, 0, 1);
UVSEE(invisible_target, 0, ACT, 1.0f, SITHAI_MODE_ACTIVE, 0, SITH_AF_INVISIBLE, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(invisible_target_seed2, 0, ACT, 1.0f, SITHAI_MODE_ACTIVE, 0, SITH_AF_INVISIBLE, 0, 1.0f, PHYS, 0, 1, 2);
UVSEE(invisible_seeinvisible, 0, ACT, 1.0f, SITHAI_MODE_ACTIVE, SITH_AF_SEEINVISIBLE, SITH_AF_INVISIBLE, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(blind, 0, ACT, 1.0f, SITHAI_MODE_ACTIVE, SITH_AF_BLIND, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(blind_seed3, 0, ACT, 1.0f, SITHAI_MODE_ACTIVE, SITH_AF_BLIND, 0, 0, 1.0f, PHYS, 0, 1, 3);
UVSEE(blind_invisible, 0, ACT, 1.0f, SITHAI_MODE_ACTIVE, SITH_AF_BLIND, SITH_AF_INVISIBLE, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(blind_invisible_seed6, 0, ACT, 1.0f, SITHAI_MODE_ACTIVE, SITH_AF_BLIND, SITH_AF_INVISIBLE, 0, 1.0f, PHYS, 0, 1, 6);
UVSEE(blind_seeinvisible, 0, ACT, 1.0f, SITHAI_MODE_ACTIVE, SITH_AF_BLIND | SITH_AF_SEEINVISIBLE, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(everything_bad, 0, ACT, 100.0f, 0, 0, SITH_AF_INVISIBLE, 0, 0.0f, PHYS, SITH_PF_CROUCHING, 0, 1);
UVSEE(everything_bad_seed20, 0, ACT, 100.0f, 0, SITH_AF_BLIND, SITH_AF_INVISIBLE, 0, 0.0f, PHYS, SITH_PF_CROUCHING, 0, 20);
UVSEE(negative_distance, 0, ACT, -5.0f, 0, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(edge_nan_distance, 0, ACT, NAN, 0, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);
UVSEE(player_target, 0, SITH_THING_PLAYER, 10.0f, SITHAI_MODE_ACTIVE, 0, 0, 0, 1.0f, PHYS, 0, 1, 1);

// ---- UpdateTargetSight / UpdateGoalSight: { fire flags, same frame, weapon selected, weapon template, submode, mode,
//   owner actor flags, eye offset, fire offset, target kind (0 none, 1 actor, 2 invisible actor, 3 item), previous
//   sight, target position, los mask, fov, sight, seed, ambient }
#define UVUPD(id, ff, same, hw, wt, sub, mode, oaf, eye, fire, kind, prev, tpos, los, fov, sight, seed, amb) \
    UVP(SithAIUtilVecUpdate, uvp_us_##id, ff, same, hw, wt, sub, mode, oaf, eye, fire, kind, prev, tpos, los, fov, sight, seed, amb)

#define Z3 V3(0, 0, 0)
UVUPD(sameframe, 0, 1, 0, 0, 0, 0, 0, Z3, Z3, 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(sameframe_primary, SITHAIUTIL_FIRE_PRIMARY, 1, 0, 0, 0, 0, 0, Z3, Z3, 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(sameframe_lob, SITHAIUTIL_FIRE_LOB, 1, 0, 0, 0, 0, 0, Z3, Z3, 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(sameframe_alt, SITHAIUTIL_FIRE_ALT, 1, 0, 0, 0, 0, 0, Z3, Z3, 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(sameframe_other_flags, SITHAIUTIL_FIRE_LEAD | SITHAIUTIL_FIRE_DELAYED | SITHAIUTIL_FIRE_WEAPON1, 1, 0, 0, 0, 0, 0, Z3, Z3, 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(visible_target, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 1, SITHAI_SIGHT_VISIBLE, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(visible_target_prevhidden_cansee, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 1, SITHAI_SIGHT_HIDDEN, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(visible_target_prevhidden_cantsee, 0, 0, 0, 0, 0, 0, SITHAI_MODE_ACTIVE, Z3, Z3, 1, SITHAI_SIGHT_HIDDEN, V3(0, 50, 0), 1, -1.0f, 100.0f, 1, 0.0f);
UVUPD(visible_target_prevtoofar, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 1, SITHAI_SIGHT_TOOFAR, V3(0, 15, 0), 1, -1.0f, 20.0f, 7, 0.2f);
UVUPD(visible_target_prevhidden_dark_seed2, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 1, SITHAI_SIGHT_HIDDEN, V3(0, 15, 0), 1, -1.0f, 20.0f, 2, 0.2f);
UVUPD(hidden_los, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 1, 0, V3(0, 5, 0), 0, -1.0f, 20.0f, 1, 1.0f);
UVUPD(toofar, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 1, 0, V3(0, 50, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(outoffov, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 1, 0, V3(0, -5, 0), 1, 0.5f, 20.0f, 1, 1.0f);
UVUPD(invisible_target, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 2, SITHAI_SIGHT_VISIBLE, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(invisible_seeinvisible, 0, 0, 0, 0, 0, 0, SITH_AF_SEEINVISIBLE, Z3, Z3, 2, SITHAI_SIGHT_VISIBLE, V3(0, 5, 0), 0, -1.0f, 20.0f, 1, 1.0f);
UVUPD(blind_owner, 0, 0, 0, 0, 0, 0, SITH_AF_BLIND, Z3, Z3, 1, SITHAI_SIGHT_VISIBLE, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(blind_seeinvisible, 0, 0, 0, 0, 0, 0, SITH_AF_BLIND | SITH_AF_SEEINVISIBLE, Z3, Z3, 1, SITHAI_SIGHT_VISIBLE, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(item_target, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 3, SITHAI_SIGHT_HIDDEN, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(weapon_selected, 0, 0, 1, 0, 0, 0, 0, Z3, Z3, 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(weapon_template, 0, 0, 0, 1, 0, 0, 0, Z3, Z3, 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(weapon_selected_and_template, 0, 0, 1, 1, 0, 0, 0, Z3, Z3, 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(fire_offset, 0, 0, 0, 0, 0, 0, 0, Z3, V3(0.25f, 0.5f, 0.75f), 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(eye_flag_off, 0, 0, 0, 0, 0, 0, 0, V3(0, 0, 2.0f), Z3, 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(eye_flag_on_zero_eye, 0, 0, 0, 0, SITHAI_SUBMODE_FIREADDEYEOFFSET, 0, 0, Z3, V3(0, 0.5f, 0), 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(eye_flag_on, 0, 0, 0, 0, SITHAI_SUBMODE_FIREADDEYEOFFSET, 0, 0, V3(0, 0, 2.0f), V3(0, 0.5f, 0), 1, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(eye_flag_on_nothing, 0, 0, 0, 0, SITHAI_SUBMODE_FIREADDEYEOFFSET, 0, 0, V3(0, 0, 2.0f), V3(0, 0.5f, 0), 0, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(eye_flag_on_target_close, 0, 0, 0, 0, SITHAI_SUBMODE_FIREADDEYEOFFSET, 0, 0, V3(0, 0, 2.0f), V3(0, 0.5f, 0), 1, 0, V3(0, 0.6f, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(eye_flag_on_target_ontop, 0, 0, 0, 0, SITHAI_SUBMODE_FIREADDEYEOFFSET, 0, 0, V3(0, 0, 2.0f), V3(0, 0.5f, 0), 1, 0, V3(0, 0.5f, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(eye_flag_on_hidden, 0, 0, 0, 0, SITHAI_SUBMODE_FIREADDEYEOFFSET, 0, 0, V3(0, 0, 2.0f), V3(0, 0.5f, 0), 1, 0, V3(0, 5, 0), 0, -1.0f, 20.0f, 1, 1.0f);
UVUPD(notarget_visible, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 0, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(notarget_visible_prevmode, 0, 0, 0, 0, 0, SITHAI_MODE_TARGETVISIBLE | SITHAI_MODE_ACTIVE, 0, Z3, Z3, 0, 0, V3(0, 5, 0), 0, -1.0f, 20.0f, 1, 1.0f);
UVUPD(notarget_hidden, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 0, 0, V3(0, 5, 0), 0, -1.0f, 20.0f, 1, 1.0f);
UVUPD(notarget_toofar, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 0, 0, V3(0, 50, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(notarget_outoffov, 0, 0, 0, 0, 0, 0, 0, Z3, Z3, 0, 0, V3(0, -5, 0), 1, 0.5f, 20.0f, 1, 1.0f);
UVUPD(notarget_eye_flag_on, 0, 0, 0, 0, SITHAI_SUBMODE_FIREADDEYEOFFSET, 0, 0, V3(0, 0, 2.0f), V3(0, 0.5f, 0), 0, 0, V3(0, 5, 0), 1, -1.0f, 20.0f, 1, 1.0f);
UVUPD(notarget_eye_flag_on_ontop, 0, 0, 0, 0, SITHAI_SUBMODE_FIREADDEYEOFFSET, 0, 0, V3(0, 0, 2.0f), V3(0, 0.5f, 0), 0, 0, V3(0, 0.5f, 0), 1, -1.0f, 20.0f, 1, 1.0f);

// Goal sight uses the same table: the goal thing is thing 1 (kind 1..3) or the position movePos (kind 0)

// ---- IsLineOfFireClear: { bNullLocal, bNullOwner, ownerControl, hasWeapon, bTarget, lastUsed, others[3] { bOwner,
//   control, actor flags, mode, toTarget, distance }, pos }
#define OTHER_NONE { 0, 0, 0, 0, { 0, 0, 0 }, 0 }
#define OTHER_FIRING(dist, ...) { 1, SITH_CT_AI, SITH_AF_HUMAN, SITHAI_MODE_TARGETVISIBLE, __VA_ARGS__, dist }
#define DIR_TO_TARGET { 1.0f, 0.0f, 0.0f }
UVP(SithAIUtilVecLineOfFire, uvp_lf_nulllocal, 1, 0, SITH_CT_AI, 1, 1, 3, { OTHER_NONE, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_nullowner, 0, 1, SITH_CT_AI, 1, 1, 3, { OTHER_NONE, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_notai, 0, 0, SITH_CT_PLAYER, 1, 1, 3, { OTHER_NONE, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_noweapon, 0, 0, SITH_CT_AI, 0, 1, 3, { OTHER_NONE, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_notarget, 0, 0, SITH_CT_AI, 1, 0, 3, { OTHER_NONE, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_noothers, 0, 0, SITH_CT_AI, 1, 1, 0, { OTHER_NONE, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_blocked, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_FIRING(2.0f, DIR_TO_TARGET), OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_blocked_second, 0, 0, SITH_CT_AI, 1, 1, 2, { OTHER_NONE, OTHER_FIRING(2.0f, DIR_TO_TARGET), OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_blocked_third, 0, 0, SITH_CT_AI, 1, 1, 3, { OTHER_NONE, OTHER_NONE, OTHER_FIRING(2.0f, DIR_TO_TARGET) }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_blocked_beyond_lastused, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_NONE, OTHER_NONE, OTHER_FIRING(2.0f, DIR_TO_TARGET) }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_otherfurther, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_FIRING(9.0f, DIR_TO_TARGET), OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_otherdistequal, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_FIRING(8.0f, DIR_TO_TARGET), OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_otherdifferentdir, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_FIRING(2.0f, { 0.0f, 1.0f, 0.0f }), OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_otherdotedge, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_FIRING(2.0f, { 0.966f, 0.2588f, 0.0f }), OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_otherdotjustbelow, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_FIRING(2.0f, { 0.9659f, 0.2589f, 0.0f }), OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_othernotvisible, 0, 0, SITH_CT_AI, 1, 1, 1, { { 1, SITH_CT_AI, SITH_AF_HUMAN, 0, DIR_TO_TARGET, 2.0f }, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_othernothuman, 0, 0, SITH_CT_AI, 1, 1, 1, { { 1, SITH_CT_AI, 0, SITHAI_MODE_TARGETVISIBLE, DIR_TO_TARGET, 2.0f }, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_othernotai, 0, 0, SITH_CT_AI, 1, 1, 1, { { 1, SITH_CT_PLAYER, SITH_AF_HUMAN, SITHAI_MODE_TARGETVISIBLE, DIR_TO_TARGET, 2.0f }, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_othernoowner, 0, 0, SITH_CT_AI, 1, 1, 1, { { 0, SITH_CT_AI, SITH_AF_HUMAN, SITHAI_MODE_TARGETVISIBLE, DIR_TO_TARGET, 2.0f }, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_posoffset, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_FIRING(2.0f, DIR_TO_TARGET), OTHER_NONE, OTHER_NONE }, V3(-4, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_posoffset_far, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_FIRING(2.0f, DIR_TO_TARGET), OTHER_NONE, OTHER_NONE }, V3(7, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_posoffset_sideways, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_FIRING(2.0f, DIR_TO_TARGET), OTHER_NONE, OTHER_NONE }, V3(8, 6, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_all_clear_mixed, 0, 0, SITH_CT_AI, 1, 1, 3, { OTHER_FIRING(9.0f, DIR_TO_TARGET), { 1, SITH_CT_AI, SITH_AF_HUMAN, SITHAI_MODE_TARGETVISIBLE, { 0.0f, 1.0f, 0.0f }, 2.0f }, OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_first_clear_second_blocks, 0, 0, SITH_CT_AI, 1, 1, 3, { OTHER_FIRING(9.0f, DIR_TO_TARGET), OTHER_FIRING(2.0f, DIR_TO_TARGET), OTHER_NONE }, V3(0, 0, 0));
UVP(SithAIUtilVecLineOfFire, uvp_lf_lastused_negative, 0, 0, SITH_CT_AI, 1, 1, -1, { OTHER_FIRING(2.0f, DIR_TO_TARGET), OTHER_NONE, OTHER_NONE }, V3(0, 0, 0));

UVP(SithAIUtilVecLineOfFire, uvp_lf_selfhuman, 0, 0, SITH_CT_AI, 1, 1, 1, { OTHER_FIRING(2.0f, DIR_TO_TARGET), OTHER_NONE, OTHER_NONE }, V3(0, 0, 0), 1);
UVP(SithAIUtilVecLineOfFire, uvp_lf_selfhuman_only, 0, 0, SITH_CT_AI, 1, 1, 0, { OTHER_NONE, OTHER_NONE, OTHER_NONE }, V3(0, 0, 0), 1);

// ---- AIFire. The base scenario: the owner shoots its weapon template at the visible target 5 units ahead.
#define UVFIRE_BASE                                                                                              \
    .debugFlags = 0, .ownerThingFlags = 0, .sectorFlags = 0, .ownerActorFlags = 0, .bTarget = 1, .bTemplate = 1,  \
    .curWeaponId = 0, .templateSpeed = 10.0f, .mode = 0, .submode = 0, .msecFireWaitTime = 0, .sameFrame = 1,    \
    .toTarget = { 0.0f, 1.0f, 0.0f }, .distance = 5.0f, .sightState = SITHAI_SIGHT_VISIBLE,                       \
    .targetPos = { 0.0f, 5.0f, 0.0f }, .targetVel = { 0.0f, 0.0f, 0.0f }, .targetMoveType = SITH_MT_PHYSICS,     \
    .losMask = 1, .minDist = 1.0f, .maxDist = 10.0f, .fireDot = 0.5f, .aimError = 0.0f, .weaponNum = 0,          \
    .fireFlags = SITHAIUTIL_FIRE_WEAPON1, .burstCount = 1, .hasWeapon = 0, .isAiming = 0, .isMounting = 0,       \
    .invType = 0, .bulletThing = 8, .tailAttached = 0, .hitAccuracy = 1.0f, .combatDamage = 1.0f, .accuracy = 0.5f, \
    .gravity = 1.0f, .seed = 1, .prevVecUnknown = { 0.0f, 0.0f, 0.0f }

#define UVFIRE(id, ...) static const SithAIUtilVecFire uvp_fire_##id = { UVFIRE_BASE, __VA_ARGS__ }
#define UVFIRE0(id)     static const SithAIUtilVecFire uvp_fire_##id = { UVFIRE_BASE }

// Gates
UVFIRE0(base);
UVFIRE(aidisabled, .debugFlags = SITHDEBUG_AIDISABLED);
UVFIRE(aidisabled_other_debug, .debugFlags = (uint32_t)~SITHDEBUG_AIDISABLED);
UVFIRE(destroyed, .ownerThingFlags = SITH_TF_DESTROYED);
UVFIRE(dying, .ownerThingFlags = SITH_TF_DYING);
UVFIRE(disabledflag_only, .ownerThingFlags = SITH_TF_DISABLED);
UVFIRE(notarget, .bTarget = 0);
UVFIRE(underwater_nofire, .sectorFlags = SITH_SECTOR_UNDERWATER, .ownerActorFlags = SITH_AF_NOUNDERWATERFIRE);
UVFIRE(underwater_canfire, .sectorFlags = SITH_SECTOR_UNDERWATER);
UVFIRE(dry_nounderwaterfire_flag, .ownerActorFlags = SITH_AF_NOUNDERWATERFIRE);

// Waiting, visibility, distance and field of view
UVFIRE(firewait, .msecFireWaitTime = 100001u);
UVFIRE(firewait_equal, .msecFireWaitTime = 100000u);
UVFIRE(firewait_delayed_ignored, .msecFireWaitTime = 100001u, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_DELAYED);
UVFIRE(sight_toofar, .sightState = SITHAI_SIGHT_TOOFAR);
UVFIRE(sight_hidden, .sightState = SITHAI_SIGHT_HIDDEN);
UVFIRE(sight_outoffov, .sightState = SITHAI_SIGHT_OUTOFFOV);
UVFIRE(sight_delayed_ignored, .sightState = SITHAI_SIGHT_HIDDEN, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_DELAYED);
UVFIRE(dist_below_min, .distance = 0.5f);
UVFIRE(dist_at_min, .distance = 1.0f);
UVFIRE(dist_at_max, .distance = 10.0f);
UVFIRE(dist_above_max, .distance = 10.5f);
UVFIRE(dist_delayed_ignored, .distance = 50.0f, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_DELAYED);
UVFIRE(fov_outside, .toTarget = { 1.0f, 0.0f, 0.0f });
UVFIRE(fov_edge, .toTarget = { 0.8660254f, 0.5f, 0.0f });
UVFIRE(fov_outside_skipcheck, .toTarget = { 1.0f, 0.0f, 0.0f }, .submode = SITHAI_SUBMODE_SKIPCHECKFIREFOV);
UVFIRE(fov_outside_delayed, .toTarget = { 1.0f, 0.0f, 0.0f }, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_DELAYED);
UVFIRE(firedot_negative, .toTarget = { 0.0f, -1.0f, 0.0f }, .fireDot = -2.0f);

// Weapon sub mode
UVFIRE(weapon2, .weaponNum = 1, .fireFlags = SITHAIUTIL_FIRE_WEAPON2);
UVFIRE(weapon3, .weaponNum = 2, .fireFlags = SITHAIUTIL_FIRE_WEAPON3);
UVFIRE(weapon4, .weaponNum = 3, .fireFlags = SITHAIUTIL_FIRE_WEAPON4);
UVFIRE(weapon_none_flag, .weaponNum = 0, .fireFlags = 0);
UVFIRE(weapon_flags_all, .weaponNum = 3, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_WEAPON2 | SITHAIUTIL_FIRE_WEAPON3 | SITHAIUTIL_FIRE_WEAPON4);
UVFIRE(weapon_flags_23, .weaponNum = 2, .fireFlags = SITHAIUTIL_FIRE_WEAPON2 | SITHAIUTIL_FIRE_WEAPON3);
UVFIRE(weaponnum_negative, .weaponNum = -1);
UVFIRE(weaponnum_negative_delayfire, .weaponNum = -1, .ownerActorFlags = SITH_AF_DELAYFIRE);
UVFIRE(weaponnum_large, .weaponNum = 7, .fireFlags = SITHAIUTIL_FIRE_WEAPON4);
UVFIRE(stop_on_fire, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_STOP_ON_FIRE);
UVFIRE(stop_on_fire_delayed, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_STOP_ON_FIRE | SITHAIUTIL_FIRE_DELAYED);

// Delayed fire
UVFIRE(delayfire_actor, .ownerActorFlags = SITH_AF_DELAYFIRE);
UVFIRE(delayfire_actor_weapon3, .ownerActorFlags = SITH_AF_DELAYFIRE, .weaponNum = 2, .fireFlags = SITHAIUTIL_FIRE_WEAPON3, .aimError = 0.75f);
UVFIRE(delayfire_second_phase, .ownerActorFlags = SITH_AF_DELAYFIRE, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_DELAYED);
UVFIRE(delayed_flag_noactorflag, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_DELAYED);

// Sight update (the flags force an update)
UVFIRE(update_primary, .sameFrame = 1, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_PRIMARY, .toTarget = { 1.0f, 0.0f, 0.0f }, .distance = 99.0f);
UVFIRE(update_alt, .sameFrame = 1, .fireFlags = SITHAIUTIL_FIRE_WEAPON2 | SITHAIUTIL_FIRE_ALT, .weaponNum = 1, .toTarget = { 1.0f, 0.0f, 0.0f }, .distance = 99.0f);
UVFIRE(update_primary_hidden, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_PRIMARY, .losMask = 0);
UVFIRE(update_newframe, .sameFrame = 0, .toTarget = { 1.0f, 0.0f, 0.0f }, .distance = 99.0f);
UVFIRE(update_newframe_hidden, .sameFrame = 0, .losMask = 0);
UVFIRE(update_lob, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LOB);
UVFIRE(update_toofar, .sameFrame = 0, .targetPos = { 0.0f, 80.0f, 0.0f });

// Lead
UVFIRE(lead_stationary, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LEAD);
UVFIRE(lead_moving_sideways, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LEAD, .targetVel = { 2.0f, 0.0f, 0.0f });
UVFIRE(lead_moving_fast, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LEAD, .targetVel = { 30.0f, 0.0f, 0.0f });
UVFIRE(lead_moving_toward, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LEAD, .targetVel = { 0.0f, -4.0f, 0.0f });
UVFIRE(lead_moving_away, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LEAD, .targetVel = { 0.0f, 4.0f, 0.0f });
UVFIRE(lead_nonphysics_target, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LEAD, .targetVel = { 2.0f, 0.0f, 0.0f }, .targetMoveType = SITH_MT_NONE);
UVFIRE(lead_slow_shot, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LEAD, .targetVel = { 2.0f, 0.0f, 0.0f }, .templateSpeed = 1.0f);
UVFIRE(lead_fast_shot, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LEAD, .targetVel = { 2.0f, 0.0f, 0.0f }, .templateSpeed = 400.0f);
UVFIRE(lead_edge_dot, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LEAD, .targetVel = { 17.32f, 0.0f, 0.0f });

// Lob
UVFIRE(lob, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LOB, .gravity = -10.0f);
UVFIRE(lob_flat, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LOB, .gravity = 0.0f);
UVFIRE(lob_up, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LOB, .gravity = 10.0f, .toTarget = { 0.0f, 0.6f, 0.8f });
UVFIRE(lob_slow_shot, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LOB, .gravity = -10.0f, .templateSpeed = 2.0f);
UVFIRE(lob_lead, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LOB | SITHAIUTIL_FIRE_LEAD, .gravity = -10.0f, .targetVel = { 2.0f, 0.0f, 0.0f });

// Aim error and accuracy
UVFIRE(aimerror_hit, .aimError = 0.5f, .seed = 1, .accuracy = 0.0f);
UVFIRE(aimerror_miss, .aimError = 0.5f, .seed = 1, .accuracy = 1.0f);
UVFIRE(aimerror_roll_a, .aimError = 0.5f, .seed = 11, .accuracy = 0.5f);
UVFIRE(aimerror_roll_b, .aimError = 0.5f, .seed = 12, .accuracy = 0.5f);
UVFIRE(aimerror_roll_c, .aimError = 0.5f, .seed = 13, .accuracy = 0.5f);
UVFIRE(aimerror_scaled, .aimError = 0.5f, .seed = 11, .accuracy = 0.5f, .hitAccuracy = 0.1f, .combatDamage = 3.0f);
UVFIRE(aimerror_zero_distance, .aimError = 0.5f, .distance = 0.0f, .minDist = 0.0f, .accuracy = 0.0f);
UVFIRE(aimerror_zero, .aimError = 0.0f, .accuracy = 0.0f);
UVFIRE(aimerror_negative, .aimError = -0.5f, .seed = 11, .accuracy = 0.0f);

// Weapon template shot: tail attached, bullet created or not, burst
UVFIRE(tail_attached, .tailAttached = 1);
UVFIRE(fire_failed, .bulletThing = -1);
UVFIRE(fire_failed_delayed, .bulletThing = -1, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_DELAYED);
UVFIRE(shot_delayed_flag, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_DELAYED);

// No weapon template: the weapon is a human weapon (COMFISTS..COMSHOTGUN, shot speed 5), the weapon inventory type
// runs a cog
UVFIRE(notemplate_weapon0, .bTemplate = 0, .curWeaponId = 0);
UVFIRE(notemplate_weapon18, .bTemplate = 0, .curWeaponId = 18, .invType = 2);
UVFIRE(notemplate_weapon25, .bTemplate = 0, .curWeaponId = 25, .invType = 2);
UVFIRE(notemplate_weaponneg, .bTemplate = 0, .curWeaponId = -1, .invType = 2);
UVFIRE(notemplate_comfists_nocog, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMFISTS, .invType = 1);
UVFIRE(notemplate_comfists_notype, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMFISTS, .invType = 0);
UVFIRE(notemplate_comfists_cog, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMFISTS, .invType = 2);
UVFIRE(notemplate_comshotgun_cog_burst, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMSHOTGUN, .invType = 2, .burstCount = 5);
UVFIRE(notemplate_cog_delayed, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMSUBMACHINE, .invType = 2, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_DELAYED);
UVFIRE(notemplate_cog_selected_notaiming, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMTOKAREV, .invType = 2, .hasWeapon = 1, .isAiming = 0);
UVFIRE(notemplate_cog_selected_aiming, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMTOKAREV, .invType = 2, .hasWeapon = 1, .isAiming = 1);
UVFIRE(notemplate_cog_selected_aiming_mounting, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMTOKAREV, .invType = 2, .hasWeapon = 1, .isAiming = 1, .isMounting = 1);
UVFIRE(notemplate_cog_notselected_mounting, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMMAUSER, .invType = 2, .hasWeapon = 0, .isMounting = 1);
UVFIRE(notemplate_cog_lead, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMSIMONOV, .invType = 2, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LEAD, .targetVel = { 2.0f, 0.0f, 0.0f });
UVFIRE(notemplate_cog_lob, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMSIMONOV, .invType = 2, .fireFlags = SITHAIUTIL_FIRE_WEAPON1 | SITHAIUTIL_FIRE_LOB, .gravity = -10.0f);
UVFIRE(notemplate_cog_aimerror, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMSIMONOV, .invType = 2, .aimError = 0.5f, .accuracy = 0.0f, .seed = 5);
UVFIRE(notemplate_delayfire, .bTemplate = 0, .curWeaponId = SITHWEAPON_COMSIMONOV, .invType = 2, .ownerActorFlags = SITH_AF_DELAYFIRE, .prevVecUnknown = { 7.0f, 8.0f, 9.0f });
UVFIRE(notemplate_not_in_range_delayfire, .bTemplate = 0, .curWeaponId = 3, .ownerActorFlags = SITH_AF_DELAYFIRE);

// ---- GetThingsInView / GetNextThingsInView. Things (sector, type, flags, pos); the viewer looks along +y from the origin.
static const SithAIUtilVecViewThing uvp_vt_all[] = {
    { 0, SITH_THING_ACTOR, 0, { 0.0f, 5.0f, 0.0f } },          // ahead
    { 0, SITH_THING_ACTOR, 0, { 5.0f, 5.0f, 0.0f } },          // 45 degrees to the right
    { 0, SITH_THING_ACTOR, 0, { 0.0f, -5.0f, 0.0f } },         // behind
    { 1, SITH_THING_PLAYER, 0, { 0.0f, 10.0f, 0.0f } },        // sector 1
    { 2, SITH_THING_ACTOR, 0, { 0.0f, 15.0f, 0.0f } },         // sector 2
    { 3, SITH_THING_ACTOR, 0, { 0.0f, 20.0f, 0.0f } },         // sector 3
    { 0, SITH_THING_ACTOR, SITH_TF_DESTROYED, { 0.0f, 6.0f, 0.0f } },
    { 0, SITH_THING_ITEM, 0, { 0.0f, 7.0f, 0.0f } },
    { 0, SITH_THING_ACTOR, 0, { 0.0f, 8.0f, 5.0f } },          // above
    { 0, SITH_THING_ACTOR, SITH_TF_DYING, { 0.0f, 9.0f, 0.0f } },
    { 0, SITH_THING_ACTOR, SITH_TF_DISABLED, { 0.0f, 9.5f, 0.0f } },
    { 1, SITH_THING_ACTOR, 0, { 0.0f, 11.0f, 0.0f } },
};

static const SithAIUtilVecViewThing uvp_vt_dense[] = {
    { 0, SITH_THING_ACTOR, 0, { 0.0f, 1.0f, 0.0f } },
    { 0, SITH_THING_ACTOR, 0, { 0.0f, 2.0f, 0.0f } },
    { 0, SITH_THING_ACTOR, 0, { 0.0f, 3.0f, 0.0f } },
    { 0, SITH_THING_ACTOR, 0, { 0.0f, 4.0f, 0.0f } },
    { 0, SITH_THING_ACTOR, 0, { 0.0f, 5.0f, 0.0f } },
    { 0, SITH_THING_ACTOR, 0, { 0.0f, 6.0f, 0.0f } },
};

#define UVVIEW_BASE                                                                                              \
    .fovX = 90.0f, .fovY = 90.0f, .sizeResult = 16, .typeMask = 0x404, .maxDistance = 100.0f, .startSector = 0,   \
    .adjoinFlags = SITH_ADJOIN_VISIBLE, .surfaceKind = 0, .normalY = -1.0f, .adjoinDistance = 1.0f,               \
    .sectorTicks = 0, .numThings = (uint8_t)SITHVEC_ARRAYLEN(uvp_vt_all), .pThings = uvp_vt_all,                   \
    .orientPos = { 0.0f, 0.0f, 0.0f }, .lvec = { 0.0f, 1.0f, 0.0f }, .rvec = { 1.0f, 0.0f, 0.0f },               \
    .uvec = { 0.0f, 0.0f, 1.0f }, .bDirect = 0, .distance = 0.0f, .directVisited = 0

#define UVVIEW(id, ...) static const SithAIUtilVecView uvp_view_##id = { UVVIEW_BASE, __VA_ARGS__ }
#define UVVIEW0(id)     static const SithAIUtilVecView uvp_view_##id = { UVVIEW_BASE }

UVVIEW0(basic);
UVVIEW(fov_negative_x, .fovX = -1.0f);
UVVIEW(fov_negative_y, .fovY = -0.001f);
UVVIEW(fov_zero, .fovX = 0.0f, .fovY = 0.0f);
UVVIEW(fov_narrow, .fovX = 20.0f, .fovY = 20.0f);
UVVIEW(fov_narrow_x_wide_y, .fovX = 20.0f, .fovY = 180.0f);
UVVIEW(fov_wide_x_narrow_y, .fovX = 180.0f, .fovY = 20.0f);
UVVIEW(fov_180, .fovX = 180.0f, .fovY = 180.0f);
UVVIEW(fov_360, .fovX = 360.0f, .fovY = 360.0f);
UVVIEW(size_zero, .sizeResult = 0);
UVVIEW(size_one, .sizeResult = 1);
UVVIEW(size_two, .sizeResult = 2);
UVVIEW(size_three, .sizeResult = 3);
UVVIEW(size_exact, .sizeResult = 5);
UVVIEW(mask_none, .typeMask = 0);
UVVIEW(mask_actor, .typeMask = 1 << SITH_THING_ACTOR);
UVVIEW(mask_player, .typeMask = 1 << SITH_THING_PLAYER);
UVVIEW(mask_item, .typeMask = 1 << SITH_THING_ITEM);
UVVIEW(mask_all, .typeMask = 0x7FFF);
UVVIEW(maxdist_zero, .maxDistance = 0.0f);
UVVIEW(maxdist_small, .maxDistance = 0.5f);
UVVIEW(maxdist_one_adjoin, .maxDistance = 2.0f);
UVVIEW(maxdist_two_adjoins, .maxDistance = 4.0f);
UVVIEW(maxdist_negative, .maxDistance = -1.0f);
UVVIEW(adjoin_notvisible, .adjoinFlags = 0);
UVVIEW(adjoin_move_only, .adjoinFlags = SITH_ADJOIN_MOVE);
UVVIEW(adjoin_visible_move, .adjoinFlags = SITH_ADJOIN_VISIBLE | SITH_ADJOIN_MOVE);
UVVIEW(adjoin_far, .adjoinDistance = 30.0f, .maxDistance = 50.0f);
UVVIEW(adjoin_far_toofar, .adjoinDistance = 30.0f, .maxDistance = 40.0f);
UVVIEW(surface_opaque, .surfaceKind = 1);
UVVIEW(surface_translucent, .surfaceKind = 2);
UVVIEW(surface_nonrgb, .surfaceKind = 3);
UVVIEW(surface_nogeometry, .surfaceKind = 4);
UVVIEW(facing_away, .normalY = 1.0f);
UVVIEW(facing_side, .normalY = 0.0f);
UVVIEW(facing_slightly, .normalY = -0.001f);
UVVIEW(visited_sector1, .sectorTicks = 0x2);
UVVIEW(visited_sector0, .sectorTicks = 0x1);
UVVIEW(visited_sector2, .sectorTicks = 0x4);
UVVIEW(start_sector1, .startSector = 1);
UVVIEW(start_sector2, .startSector = 2);
UVVIEW(start_sector3, .startSector = 3);
UVVIEW(orient_looking_back, .lvec = { 0.0f, -1.0f, 0.0f }, .rvec = { -1.0f, 0.0f, 0.0f });
UVVIEW(orient_moved, .orientPos = { 0.0f, 12.0f, 0.0f });
UVVIEW(orient_moved_side, .orientPos = { 4.0f, 3.0f, 0.0f }, .fovX = 60.0f, .fovY = 60.0f);
UVVIEW(orient_unnormalized, .lvec = { 0.0f, 2.0f, 0.0f }, .rvec = { 3.0f, 0.0f, 0.0f }, .uvec = { 0.0f, 0.0f, 0.5f });
UVVIEW(orient_tilted, .lvec = { 0.0f, 0.7071f, 0.7071f }, .uvec = { 0.0f, -0.7071f, 0.7071f });
UVVIEW(orient_zero_vectors, .lvec = { 0.0f, 0.0f, 0.0f }, .rvec = { 0.0f, 0.0f, 0.0f }, .uvec = { 0.0f, 0.0f, 0.0f });
UVVIEW(dense_size3, .numThings = (uint8_t)SITHVEC_ARRAYLEN(uvp_vt_dense), .pThings = uvp_vt_dense, .sizeResult = 3);
UVVIEW(dense_size6, .numThings = (uint8_t)SITHVEC_ARRAYLEN(uvp_vt_dense), .pThings = uvp_vt_dense, .sizeResult = 6);
UVVIEW(dense_size7, .numThings = (uint8_t)SITHVEC_ARRAYLEN(uvp_vt_dense), .pThings = uvp_vt_dense, .sizeResult = 7);
UVVIEW(nothings, .numThings = 0);

// Direct calls of GetNextThingsInView with a prepared search state
UVVIEW(next_basic, .bDirect = 1, .distance = 0.0f);
UVVIEW(next_distance_over_max, .bDirect = 1, .distance = 150.0f);
UVVIEW(next_distance_at_max, .bDirect = 1, .distance = 100.0f, .maxDistance = 100.0f);
UVVIEW(next_distance_adjoin_cutoff, .bDirect = 1, .distance = 99.0f);
UVVIEW(next_already_visited, .bDirect = 1, .sectorTicks = 0x1);
UVVIEW(next_visited_limit, .bDirect = 1, .directVisited = 128);
UVVIEW(next_visited_limit_minus1, .bDirect = 1, .directVisited = 127);
UVVIEW(next_visited_limit_over, .bDirect = 1, .directVisited = 1000);
UVVIEW(next_size_full_midway, .bDirect = 1, .sizeResult = 1);
UVVIEW(next_size_zero, .bDirect = 1, .sizeResult = 0);
UVVIEW(next_start_sector2, .bDirect = 1, .startSector = 2, .distance = 3.0f);

// ---- Point creation and path checks
#define COLL_THINGP(d)       { SITHCOLLISION_THING, 4, 0, 0, 0, d, { 1.0f, 0.0f, 0.0f }, 0.0f }

#define UVPATH_BASE                                                                                              \
    .flags = 0, .angle = 0.0f, .maxAngle = 0.0f, .deltaAngle = 0.0f, .minDist = 0.0f, .moveDist = 5.0f,          \
    .radius = 0.0f, .downCos = 5.0f, .pos = { 1.0f, 2.0f, 0.5f }, .dir = { 0.0f, 1.0f, 0.0f },                   \
    .moveDirection = { 1.0f, 0.0f, 0.0f }, .ownerUvec = { 0.0f, 0.0f, 1.0f }, .movePos = { 10.0f, 0.0f, 0.5f },   \
    .homePos = { -10.0f, 0.0f, 0.5f }, .goalPos = { 0.0f, 10.0f, 0.5f }, .goalThing = 0, .fleeThing = 0,         \
    .targetThing = 0, .mode = 0, .losMask = 0xFFFFFFFFu, .seed = 1, .sectorMode = 0, .bNullPos = 0,              \
    .bNullDir = 0, .bNullDest = 0, .bNullEnd = 0, .maxRetries = 3, .bAllow3D = 0, .retries = 0, .retryOther = 0,  \
    .pSearches = NULL, .numSearches = 0

#define COMMA ,
#define UVPATH(id, ...)  static const SithAIUtilVecPath uvp_pa_##id = { UVPATH_BASE, __VA_ARGS__ }
#define UVPATH0(id)      static const SithAIUtilVecPath uvp_pa_##id = { UVPATH_BASE }
#define UVPATHS(id, searches, ...)                                                                              \
    static const SithAIUtilVecSearch uvq_pa_##id[] = { searches };                                              \
    static const SithAIUtilVecPath uvp_pa_##id = { UVPATH_BASE, .pSearches = uvq_pa_##id, .numSearches = (uint8_t)SITHVEC_ARRAYLEN(uvq_pa_##id), __VA_ARGS__ }

#define MK_YAW        ((uint32_t)SITHAIUTIL_MKPOINT_YAW)
#define MK_PITCH      ((uint32_t)SITHAIUTIL_MKPOINT_PITCH)
#define MK_SIDE       ((uint32_t)SITHAIUTIL_MKPOINT_SIDEWAYS)
#define MK_RANDANGLE  ((uint32_t)SITHAIUTIL_MKPOINT_RANDANGLE)
#define MK_RANDSIDE   ((uint32_t)SITHAIUTIL_MKPOINT_RANDSIDE)

// MakeRandPoint
UVPATH(mk_noflags, .flags = 0, .dir = { 0.0f, 1.0f, 0.0f });
UVPATH(mk_noflags_dirz, .flags = 0, .dir = { 0.0f, 3.0f, 4.0f });
UVPATH(mk_yaw30, .flags = MK_YAW, .angle = 30.0f);
UVPATH(mk_yaw90, .flags = MK_YAW, .angle = 90.0f);
UVPATH(mk_yaw180, .flags = MK_YAW, .angle = 180.0f);
UVPATH(mk_yaw_negative, .flags = MK_YAW, .angle = -45.0f);
UVPATH(mk_yaw0, .flags = MK_YAW, .angle = 0.0f);
UVPATH(mk_yaw_dirz, .flags = MK_YAW, .angle = 30.0f, .dir = { 0.0f, 3.0f, 4.0f });
UVPATH(mk_pitch20, .flags = MK_PITCH, .angle = 20.0f);
UVPATH(mk_pitch_dirz, .flags = MK_PITCH, .angle = 20.0f, .dir = { 0.0f, 3.0f, 4.0f });
UVPATH(mk_yaw_pitch, .flags = MK_YAW | MK_PITCH, .angle = 25.0f);
UVPATH(mk_yaw_randangle_s1, .flags = MK_YAW | MK_RANDANGLE, .angle = 60.0f, .seed = 1);
UVPATH(mk_yaw_randangle_s2, .flags = MK_YAW | MK_RANDANGLE, .angle = 60.0f, .seed = 2);
UVPATH(mk_yaw_randangle_zero, .flags = MK_YAW | MK_RANDANGLE, .angle = 0.0f);
UVPATH(mk_edge_rand_ulp, .flags = MK_PITCH | MK_RANDANGLE, .angle = 60.0f, .seed = 0xA282BBFAu);
UVPATH(mk_pitch_randangle, .flags = MK_PITCH | MK_RANDANGLE, .angle = 60.0f, .seed = 3);
UVPATH(mk_yawpitch_randangle, .flags = MK_YAW | MK_PITCH | MK_RANDANGLE, .angle = 60.0f, .seed = 4);
UVPATH(mk_yaw_randside_s1, .flags = MK_YAW | MK_RANDSIDE, .angle = 40.0f, .seed = 1);
UVPATH(mk_yaw_randside_s2, .flags = MK_YAW | MK_RANDSIDE, .angle = 40.0f, .seed = 2);
UVPATH(mk_yaw_randside_s3, .flags = MK_YAW | MK_RANDSIDE, .angle = 40.0f, .seed = 3);
UVPATH(mk_yaw_randangle_randside, .flags = MK_YAW | MK_RANDANGLE | MK_RANDSIDE, .angle = 90.0f, .seed = 5);
UVPATH(mk_yaw_randangle_randside_s6, .flags = MK_YAW | MK_RANDANGLE | MK_RANDSIDE, .angle = 90.0f, .seed = 6);
UVPATH(mk_sideways, .flags = MK_SIDE);
UVPATH(mk_sideways_dirz, .flags = MK_SIDE, .dir = { 0.0f, 3.0f, 4.0f });
UVPATH(mk_sideways_alongmove_same, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_ALONGMOVEDIR, .moveDirection = { -1.0f, 0.0f, 0.0f });
UVPATH(mk_sideways_alongmove_opposite, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_ALONGMOVEDIR, .moveDirection = { 1.0f, 0.0f, 0.0f });
UVPATH(mk_sideways_alongmove_perp, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_ALONGMOVEDIR, .moveDirection = { 0.0f, 1.0f, 0.0f });
UVPATH(mk_sideways_alongmove_ignores_rand, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_ALONGMOVEDIR | MK_RANDSIDE | MK_RANDANGLE, .angle = 30.0f, .moveDirection = { 1.0f, 0.0f, 0.0f });
UVPATH(mk_sideways_randside_s1, .flags = MK_SIDE | MK_RANDSIDE, .seed = 1);
UVPATH(mk_sideways_randside_s2, .flags = MK_SIDE | MK_RANDSIDE, .seed = 2);
UVPATH(mk_sideways_randside_s3, .flags = MK_SIDE | MK_RANDSIDE, .seed = 3);
UVPATH(mk_sideways_randangle_s1, .flags = MK_SIDE | MK_RANDANGLE, .angle = 30.0f, .seed = 1);
UVPATH(mk_sideways_randangle_s2, .flags = MK_SIDE | MK_RANDANGLE, .angle = 30.0f, .seed = 2);
UVPATH(mk_sideways_randangle_randside, .flags = MK_SIDE | MK_RANDANGLE | MK_RANDSIDE, .angle = 30.0f, .seed = 7);
UVPATH(mk_sideways_yaw_wins, .flags = MK_SIDE | MK_YAW, .angle = 30.0f);
UVPATH(mk_downcos_zero, .flags = MK_YAW, .angle = 30.0f, .downCos = 0.0f);
UVPATH(mk_downcos_tiny, .flags = MK_YAW, .angle = 30.0f, .downCos = 0.0000001f);
UVPATH(mk_downcos_negative, .flags = MK_YAW, .angle = 30.0f, .downCos = -3.0f);
UVPATH(mk_downcos_epsilon_edge, .flags = MK_YAW, .angle = 30.0f, .downCos = 0.00001f);
UVPATH(mk_nullpos, .flags = MK_YAW, .angle = 30.0f, .bNullPos = 1);
UVPATH(mk_nulldir, .flags = MK_YAW, .angle = 30.0f, .bNullDir = 1);
UVPATH(mk_nulldest, .flags = MK_YAW, .angle = 30.0f, .bNullDest = 1);
UVPATH(mk_towardgoal_front, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 30.0f);
UVPATH(mk_towardgoal_behind, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 30.0f, .goalPos = { 0.0f, -10.0f, 0.5f });
UVPATH(mk_towardgoal_behind_nonyaw, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .goalPos = { 5.0f, 0.0f, 0.5f });
UVPATH(mk_towardgoal_front_nonyaw, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .goalPos = { -5.0f, 0.0f, 0.5f });
UVPATH(mk_towardgoal_thing, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 30.0f, .goalThing = 1);
UVPATH(mk_awayfromgoal_front, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_AWAYFROMGOAL, .angle = 30.0f);
UVPATH(mk_awayfromgoal_behind, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_AWAYFROMGOAL, .angle = 30.0f, .goalPos = { 0.0f, -10.0f, 0.5f });
UVPATH(mk_awayfromgoal_front_nonyaw, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_AWAYFROMGOAL, .goalPos = { -5.0f, 0.0f, 0.5f });
UVPATH(mk_awayfromgoal_behind_nonyaw, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_AWAYFROMGOAL, .goalPos = { 5.0f, 0.0f, 0.5f });
UVPATH(mk_towardmovepos_front, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDMOVEPOS, .angle = 30.0f, .movePos = { 0.0f, 10.0f, 0.5f });
UVPATH(mk_towardmovepos_behind, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDMOVEPOS, .angle = 30.0f, .movePos = { 0.0f, -10.0f, 0.5f });
UVPATH(mk_refowner, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_REFOWNER, .angle = 30.0f);
UVPATH(mk_refowner_nonyaw, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_REFOWNER);
UVPATH(mk_awayfromflee, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_AWAYFROMFLEE, .angle = 30.0f, .fleeThing = 1);
UVPATH(mk_awayfromflee_nonyaw, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_AWAYFROMFLEE, .fleeThing = 1);
UVPATH(mk_targetdist_towardmovepos, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TARGETDIST | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDMOVEPOS, .angle = 30.0f, .movePos = { 0.0f, 10.0f, 0.5f });
UVPATH(mk_targetdist_towardgoal, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TARGETDIST | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 30.0f);
UVPATH(mk_targetdist_behind, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TARGETDIST | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 30.0f, .goalPos = { 0.0f, -10.0f, 0.5f });
UVPATH(mk_targetdist_zero, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TARGETDIST | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDMOVEPOS, .angle = 90.0f, .movePos = { 1.0f, 2.0f, 0.5f });
UVPATH(mk_targetdist_ignored_noref, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TARGETDIST, .angle = 30.0f);
UVPATH(mk_refhome_not_ref, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_REFHOME, .angle = 30.0f);
UVPATH(mk_rand_towardgoal_randsides, .flags = MK_YAW | MK_RANDANGLE | MK_RANDSIDE | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 90.0f, .seed = 8);
UVPATH(mk_rand_awaygoal_randsides, .flags = MK_YAW | MK_RANDANGLE | MK_RANDSIDE | (uint32_t)SITHAIUTIL_MKPOINT_AWAYFROMGOAL, .angle = 90.0f, .seed = 9);

// Retry after MakeRandPoint
UVPATH(retry_yaw_1, .flags = MK_YAW, .angle = 30.0f, .retries = 1);
UVPATH(retry_yaw_2, .flags = MK_YAW, .angle = 30.0f, .retries = 2);
UVPATH(retry_yaw_pitch, .flags = MK_YAW | MK_PITCH, .angle = 30.0f, .retries = 2);
UVPATH(retry_pitch_only, .flags = MK_PITCH, .angle = 30.0f, .retries = 2);
UVPATH(retry_sideways_1, .flags = MK_SIDE, .retries = 1);
UVPATH(retry_sideways_2, .flags = MK_SIDE, .retries = 2);
UVPATH(retry_sideways_randangle, .flags = MK_SIDE | MK_RANDANGLE | MK_RANDSIDE, .angle = 40.0f, .seed = 3, .retries = 2);
UVPATH(retry_noretry_targetdist, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TARGETDIST | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 30.0f, .retries = 2);
UVPATH(retry_noretry_towardmovepos, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDMOVEPOS, .angle = 30.0f, .retries = 2);
UVPATH(retry_towardgoal_ok, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 30.0f, .retries = 2);
UVPATH(retry_after_invalid_point, .flags = MK_YAW, .angle = 30.0f, .downCos = 0.0f, .retries = 1);
UVPATH(retry_after_noflags, .flags = 0, .retries = 1);
UVPATH(retry_other_block, .flags = MK_YAW, .angle = 30.0f, .retries = 1, .retryOther = 1);
UVPATH(retry_yaw_zero_angle, .flags = MK_YAW, .angle = 0.0f, .retries = 2);
UVPATH(retry_3, .flags = MK_YAW, .angle = 45.0f, .retries = 3);

// RetryMakeRandPoint on its own: { var 0: no state; var 1: other control block; var 2: this control block }
UVPATH(ra_nostate, .flags = 0);
UVPATH(ra_nostate_nulllocal, .flags = 0, .retryOther = 1);
UVPATH(ra_stale_otherlocal, .flags = MK_YAW);
UVPATH(ra_stale_nulllocal, .flags = MK_YAW, .retryOther = 1);
UVPATH(ra_stale_yaw, .flags = MK_YAW, .movePos = { 10.0f, 20.0f, 30.0f }, .dir = { 0.0f, 1.0f, 0.0f });
UVPATH(ra_stale_yawpitch, .flags = MK_YAW | MK_PITCH | MK_RANDANGLE, .movePos = { 0.0f, 20.0f, 30.0f });
UVPATH(ra_stale_sideways, .flags = MK_SIDE, .homePos = { 1.0f, 2.0f, 3.0f });
UVPATH(ra_stale_noretry, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TARGETDIST);
UVPATH(ra_stale_noretry_movepos, .flags = MK_SIDE | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDMOVEPOS);
UVPATH(ra_stale_otherflags, .flags = (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL | MK_RANDSIDE);
UVPATH(ra_nullpos, .flags = MK_YAW, .bNullPos = 1);
UVPATH(ra_nulldir, .flags = MK_YAW, .bNullDir = 1);
UVPATH(ra_nulldest, .flags = MK_YAW, .bNullDest = 1);
UVPATH(ra_downcos, .flags = MK_SIDE, .downCos = 3.5f, .pos = { 4.0f, 5.0f, 6.0f }, .homePos = { 0.0f, 0.0f, 1.0f });
UVPATH(ra_downcos_neg, .flags = MK_YAW, .downCos = -2.0f, .pos = { 4.0f, 5.0f, 6.0f }, .movePos = { 0.0f, 90.0f, 0.0f });

// CheckPathToPos, straight: { flags STRAIGHT | ..., minDist, moveDist, radius, dir }
#define PF_STRAIGHT ((uint32_t)SITHAIUTIL_PATHFLAG_STRAIGHT)
#define PF_SIDES    ((uint32_t)SITHAIUTIL_PATHFLAG_SIDES)
UVPATH(cp_neither, .flags = 0, .moveDist = 5.0f);
UVPATH(cp_nulldir, .flags = PF_STRAIGHT, .bNullDir = 1);
UVPATH(cp_straight_clear, .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f);
UVPATH(cp_straight_clear_noend, .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f, .bNullEnd = 1);
UVPATH(cp_straight_clear_radius, .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f, .radius = 0.75f);
UVPATH(cp_straight_clear_dir, .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 4.0f, .dir = { 0.6f, 0.8f, 0.0f });
UVPATHS(cp_straight_wall_near, SEARCH(COLL_WALL(0.1f)), .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_wall_at_min, SEARCH(COLL_WALL(0.5f)), .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_wall_far, SEARCH(COLL_WALL(2.5f)), .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_wall_far_noend, SEARCH(COLL_WALL(2.5f)), .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f, .bNullEnd = 1);
UVPATHS(cp_straight_thing, SEARCH(COLL_THING(2.5f)), .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_thing_near, SEARCH(COLL_THING(0.25f)), .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_player_blocks, SEARCH(COLL_THINGP(2.5f)), .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_player_ignored, SEARCH(COLL_THINGP(2.5f)), .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_IGNOREPLAYER, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_actor_not_ignored_as_player, SEARCH(COLL_THING(2.5f)), .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_IGNOREPLAYER, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_ignorethings, SEARCH(COLL_THING(2.5f)), .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_IGNORETHINGS, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_ignorethings_then_wall, SEARCH(COLL_THING(1.0f)), .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_IGNORETHINGS, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_floor_blocks, SEARCH(COLL_FLOOR(2.5f)), .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_floor_ignored, SEARCH(COLL_FLOOR(2.5f)), .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_IGNOREFLOORS, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_wall_not_floor_ignored, SEARCH(COLL_WALL(2.5f)), .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_IGNOREFLOORS, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_floor_then_wall, SEARCH(COLL_FLOOR(1.0f)) COMMA SEARCH(COLL_WALL(3.0f)), .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_IGNOREFLOORS, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_all_ignored, SEARCH(COLL_FLOOR(1.0f)) COMMA SEARCH(COLL_THING(2.0f)) COMMA SEARCH(COLL_THINGP(3.0f)), .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_IGNOREFLOORS | (uint32_t)SITHAIUTIL_PATHFLAG_IGNORETHINGS, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_adjoin_world, SEARCH(COLL_ADJWORLD(1.0f, SITH_ADJOIN_VISIBLE, 2, SITH_SURFACE_ISFLOOR, 1.0f)), .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_IGNOREFLOORS, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(cp_straight_adjoin_touch, SEARCH(COLL_ADJ(1.0f, SITH_ADJOIN_VISIBLE, 2, 0)), .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f);

// Straight path ends must be visible from the goal or target
UVPATH(cp_goalvisible_ok, .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_GOALVISIBLE, .minDist = 0.5f, .moveDist = 5.0f, .goalThing = 1, .losMask = 1);
UVPATH(cp_goalvisible_hidden, .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_GOALVISIBLE, .minDist = 0.5f, .moveDist = 5.0f, .goalThing = 1, .losMask = 0);
UVPATH(cp_goalvisible_nogoal, .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_GOALVISIBLE, .minDist = 0.5f, .moveDist = 5.0f, .goalThing = 0, .losMask = 0);
UVPATH(cp_goalvisible_noflag, .flags = PF_STRAIGHT, .minDist = 0.5f, .moveDist = 5.0f, .goalThing = 1, .losMask = 0);
UVPATH(cp_targetvisible_ok, .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_TARGETVISIBLE, .minDist = 0.5f, .moveDist = 5.0f, .targetThing = 1, .losMask = 1);
UVPATH(cp_targetvisible_hidden, .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_TARGETVISIBLE, .minDist = 0.5f, .moveDist = 5.0f, .targetThing = 1, .losMask = 0);
UVPATH(cp_targetvisible_notarget, .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_TARGETVISIBLE, .minDist = 0.5f, .moveDist = 5.0f, .targetThing = 0, .losMask = 0);
UVPATH(cp_both_visible_target_wins, .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_GOALVISIBLE | (uint32_t)SITHAIUTIL_PATHFLAG_TARGETVISIBLE, .minDist = 0.5f, .moveDist = 5.0f, .goalThing = 1, .targetThing = 1, .losMask = 2);
UVPATH(cp_both_visible_goal_only, .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_GOALVISIBLE | (uint32_t)SITHAIUTIL_PATHFLAG_TARGETVISIBLE, .minDist = 0.5f, .moveDist = 5.0f, .goalThing = 1, .targetThing = 0, .losMask = 0);
UVPATH(cp_visible_toofar, .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_TARGETVISIBLE, .minDist = 0.5f, .moveDist = 5.0f, .targetThing = 1, .losMask = 1, .pos = { 100.0f, 0.0f, 0.0f });
UVPATHS(cp_visible_but_wall_blocks, SEARCH(COLL_WALL(0.1f)), .flags = PF_STRAIGHT | (uint32_t)SITHAIUTIL_PATHFLAG_TARGETVISIBLE, .minDist = 0.5f, .moveDist = 5.0f, .targetThing = 1, .losMask = 0);

// Sides
UVPATH(cp_sides_clear, .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f);
UVPATH(cp_sides_clear_noend, .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f, .bNullEnd = 1);
UVPATH(cp_sides_movedist_equal, .flags = PF_SIDES, .minDist = 1.0f, .moveDist = 1.03f);
UVPATH(cp_sides_dir_parallel_up, .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f, .dir = { 0.0f, 0.0f, 1.0f });
UVPATH(cp_sides_dir_parallel_down, .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f, .dir = { 0.0f, 0.0f, -1.0f });
UVPATH(cp_sides_dir_parallel_up_rvec, .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f, .dir = { 0.0f, 0.0f, 1.0f }, .ownerUvec = { 0.0f, 0.0f, 1.0f });
UVPATH(cp_sides_dir_tiny_cross, .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f, .dir = { 0.0000001f, 0.0f, 1.0f });
UVPATH(cp_sides_dir_diagonal, .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f, .dir = { 0.6f, 0.0f, 0.8f });
UVPATH(cp_sides_nosector, .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f, .sectorMode = 1);
UVPATHS(cp_sides_first_hit_wall, SEARCH(COLL_WALL(0.3f)), .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f);
UVPATHS(cp_sides_first_hit_thing, SEARCH(COLL_THING(0.3f)), .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f);
UVPATHS(cp_sides_first_hit_floor, SEARCH(COLL_FLOOR(0.3f)), .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f);
UVPATHS(cp_sides_first_hit_adjoin, SEARCH(COLL_ADJ(0.3f, SITH_ADJOIN_VISIBLE, 2, 0)), .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f);
UVPATHS(cp_sides_both_walls, SEARCH(COLL_WALL(0.3f)) COMMA SEARCH_NONE COMMA SEARCH(COLL_WALL(0.4f)), .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f);
UVPATH(cp_sides_goal_visible_both, .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 1, .losMask = 3);
UVPATH(cp_sides_goal_visible_none, .flags = PF_SIDES, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 1, .losMask = 0);
UVPATH(cp_sides_goal_visible_none_required, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_GOALVISIBLE, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 1, .losMask = 0);
UVPATH(cp_sides_goal_visible_one, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_GOALVISIBLE, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 1, .losMask = 1);
UVPATH(cp_sides_goal_visible_other, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_GOALVISIBLE, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 1, .losMask = 2);
UVPATH(cp_sides_prefer_goal, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_PREFERGOAL, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 1, .losMask = 3);
UVPATH(cp_sides_avoid_goal, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_AVOIDGOAL, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 1, .losMask = 3);
UVPATH(cp_sides_prefer_and_avoid, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_PREFERGOAL | (uint32_t)SITHAIUTIL_PATHFLAG_AVOIDGOAL, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 1, .losMask = 3);
UVPATH(cp_sides_prefer_goal_far_side, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_PREFERGOAL, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 1, .losMask = 3, .pos = { -3.0f, 0.0f, 0.5f });
UVPATH(cp_sides_avoid_goal_far_side, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_AVOIDGOAL, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 1, .losMask = 3, .pos = { -3.0f, 0.0f, 0.5f });
UVPATH(cp_sides_prefer_goal_equidistant, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_PREFERGOAL, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 2, .losMask = 3, .goalPos = { 1.0f, 10.0f, 0.5f });
UVPATH(cp_sides_avoid_goal_equidistant, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_AVOIDGOAL, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 2, .losMask = 3, .goalPos = { 1.0f, 10.0f, 0.5f });
UVPATH(cp_sides_prefer_goal_nogoal, .flags = PF_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_PREFERGOAL, .minDist = 0.5f, .moveDist = 0.65f, .goalThing = 0);
UVPATH(cp_sides_both_flags_straight_wins, .flags = PF_SIDES | PF_STRAIGHT, .minDist = 0.5f, .moveDist = 0.65f);

// SweepMoveToPathPos: the target position is goalPos; { angle, maxAngle, deltaAngle, flags }
UVPATH(sw_noiter, .angle = 100.0f, .maxAngle = 50.0f, .deltaAngle = 10.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATHS(sw_blocked_first, SEARCH(COLL_WALL(1.0f)), .angle = 0.0f, .maxAngle = 90.0f, .deltaAngle = 30.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATHS(sw_blocked_first_angle30, SEARCH(COLL_WALL(1.0f)), .angle = 30.0f, .maxAngle = 90.0f, .deltaAngle = 30.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATH(sw_clear_all, .angle = 0.0f, .maxAngle = 90.0f, .deltaAngle = 30.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATH(sw_clear_all_angle45, .angle = 45.0f, .maxAngle = 180.0f, .deltaAngle = 45.0f, .goalPos = { 5.0f, 10.0f, 0.5f }, .seed = 2);
UVPATH(sw_pos_at_owner, .angle = 0.0f, .maxAngle = 90.0f, .deltaAngle = 30.0f, .goalPos = { 1.0f, 2.0f, 0.5f });
UVPATH(sw_pos_above_owner, .angle = 0.0f, .maxAngle = 90.0f, .deltaAngle = 30.0f, .goalPos = { 1.0f, 2.0f, 9.5f });
UVPATHS(sw_blocked_retry_free, SEARCH_NONE COMMA SEARCH(COLL_WALL(1.0f)), .angle = 30.0f, .maxAngle = 30.0f, .deltaAngle = 30.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATHS(sw_flags_masked, SEARCH(COLL_WALL(1.0f)), .flags = 0xFFFFFFFFu, .angle = 0.0f, .maxAngle = 0.0f, .deltaAngle = 30.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATHS(sw_flags_pitch, SEARCH(COLL_WALL(1.0f)), .flags = MK_PITCH | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 20.0f, .maxAngle = 20.0f, .deltaAngle = 30.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATHS(sw_towardgoal, SEARCH(COLL_WALL(1.0f)), .flags = (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 20.0f, .maxAngle = 20.0f, .deltaAngle = 30.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATHS(sw_zero_delta_limit, SEARCH(COLL_WALL(1.0f)), .angle = 90.0f, .maxAngle = 90.0f, .deltaAngle = 0.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATHS(sw_blocked_second_iteration, SEARCH_NONE COMMA SEARCH(COLL_WALL(1.0f)), .angle = 0.0f, .maxAngle = 90.0f, .deltaAngle = 90.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATHS(sw_angle_180_no_retry, SEARCH_NONE, .angle = 180.0f, .maxAngle = 180.0f, .deltaAngle = 30.0f, .goalPos = { 0.0f, 10.0f, 0.5f });
UVPATHS(sw_blocked_at_180, SEARCH(COLL_WALL(1.0f)), .angle = 180.0f, .maxAngle = 180.0f, .deltaAngle = 30.0f, .goalPos = { 0.0f, 10.0f, 0.5f });

// MakeRoamPos: { minDist, moveDist, bAllow3D, maxRetries, seed }
UVPATH(ro_clear, .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 3);
UVPATH(ro_clear_3d, .minDist = 0.5f, .moveDist = 5.0f, .bAllow3D = 1, .maxRetries = 3, .seed = 2);
UVPATHS(ro_blocked_once, SEARCH(COLL_WALL(0.1f)), .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 3);
UVPATHS(ro_edge_rand_ulp, SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)), .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 3, .seed = 0xA282BBFAu);
UVPATHS(ro_blocked_twice, SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)), .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 3, .seed = 3);
UVPATH(ro_movedist_zero, .minDist = 0.5f, .moveDist = 0.0f, .maxRetries = 3);
UVPATH(ro_maxretries_zero, .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 0);
UVPATH(ro_maxretries_negative, .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = -1);
UVPATH(ro_floor_ignored_in_2d, .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 2);
UVPATHS(ro_floor_2d, SEARCH(COLL_FLOOR(1.0f)), .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 2);
UVPATHS(ro_floor_3d, SEARCH(COLL_FLOOR(1.0f)), .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 2, .bAllow3D = 1);
UVPATHS(ro_thing_blocked_all, SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)), .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 1);
UVPATHS(ro_thing_blocked_all_retries3, SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)), .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 3);
UVPATH(ro_seed_variation, .minDist = 0.5f, .moveDist = 5.0f, .maxRetries = 3, .seed = 77);

// MakePathPos: { flags, minDist, moveDist, angle, dir }
UVPATH(mp_noflags, .flags = 0, .minDist = 0.5f, .moveDist = 5.0f);
UVPATH(mp_yaw_clear, .flags = MK_YAW, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATH(mp_yaw_pitch_clear, .flags = MK_YAW | MK_PITCH, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATH(mp_pitch_clear, .flags = MK_PITCH, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(mp_yaw_blocked_retry_ok, SEARCH(COLL_WALL(0.1f)), .flags = MK_YAW, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(mp_yaw_blocked_both, SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)), .flags = MK_YAW, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(mp_yaw_blocked_noretry_flags, SEARCH(COLL_WALL(0.1f)), .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TARGETDIST | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(mp_sideways_blocked_retry_ok, SEARCH(COLL_WALL(0.1f)), .flags = MK_SIDE, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(mp_sideways_blocked_both, SEARCH(COLL_WALL(0.1f)) COMMA SEARCH(COLL_WALL(0.1f)), .flags = MK_SIDE, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(mp_pathflags_ignoreplayer, SEARCH(COLL_THINGP(2.0f)), .flags = MK_YAW | (uint32_t)SITHAIUTIL_PATHFLAG_IGNOREPLAYER, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(mp_pathflags_ignorethings, SEARCH(COLL_THING(2.0f)), .flags = MK_YAW | (uint32_t)SITHAIUTIL_PATHFLAG_IGNORETHINGS, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(mp_pathflags_thing_blocks, SEARCH(COLL_THING(2.0f)), .flags = MK_YAW, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(mp_pathflags_floor_2d_ignored, SEARCH(COLL_FLOOR(1.0f)), .flags = MK_YAW, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATHS(mp_pathflags_floor_3d_blocks, SEARCH(COLL_FLOOR(1.0f)), .flags = MK_YAW | MK_PITCH, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATH(mp_goalvisible, .flags = MK_YAW | (uint32_t)SITHAIUTIL_PATHFLAG_GOALVISIBLE, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f, .goalThing = 1, .losMask = 0);
UVPATH(mp_goalvisible_ok, .flags = MK_YAW | (uint32_t)SITHAIUTIL_PATHFLAG_GOALVISIBLE, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f, .goalThing = 1, .losMask = 0xFFFFFFFFu);
UVPATH(mp_targetvisible, .flags = MK_YAW | (uint32_t)SITHAIUTIL_PATHFLAG_TARGETVISIBLE, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f, .targetThing = 1, .losMask = 0);
UVPATH(mp_other_pathflags_dropped, .flags = MK_YAW | (uint32_t)SITHAIUTIL_PATHFLAG_SIDES | (uint32_t)SITHAIUTIL_PATHFLAG_PREFERGOAL | (uint32_t)SITHAIUTIL_PATHFLAG_AVOIDGOAL, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f);
UVPATH(mp_downcos_invalid, .flags = MK_YAW, .angle = 30.0f, .minDist = 0.5f, .moveDist = 0.0f);
UVPATH(mp_randangle_seed, .flags = MK_YAW | MK_RANDANGLE | MK_RANDSIDE, .angle = 90.0f, .minDist = 0.5f, .moveDist = 5.0f, .seed = 9);
UVPATH(mp_towardgoal, .flags = MK_YAW | (uint32_t)SITHAIUTIL_MKPOINT_TOWARDGOAL, .angle = 30.0f, .minDist = 0.5f, .moveDist = 5.0f, .goalPos = { 0.0f, -10.0f, 0.5f });

// ---- GENERATED CASE TABLE

#define C(name, fn, var, params) { name, sithAIUtilVec_Run##fn, var, params }

static const SithVecCase sithAIUtilVec_aCases[] = {
    C("SetWeaponFireFlags_0", SetWeaponFireFlags, 0, &uvp_wf_0),
    C("SetWeaponFireFlags_1", SetWeaponFireFlags, 0, &uvp_wf_1),
    C("SetWeaponFireFlags_2", SetWeaponFireFlags, 0, &uvp_wf_2),
    C("SetWeaponFireFlags_3", SetWeaponFireFlags, 0, &uvp_wf_3),
    C("SetWeaponFireFlags_0all", SetWeaponFireFlags, 0, &uvp_wf_0all),
    C("SetWeaponFireFlags_3some", SetWeaponFireFlags, 0, &uvp_wf_3some),
    C("SetWeaponFireFlags_m1", SetWeaponFireFlags, 0, &uvp_wf_m1),
    C("SetWeaponFireFlags_4", SetWeaponFireFlags, 0, &uvp_wf_4),
    C("SetWeaponFireFlags_100", SetWeaponFireFlags, 0, &uvp_wf_100),
    C("SetWeaponFireFlags_min", SetWeaponFireFlags, 0, &uvp_wf_min),
    C("SetWeaponFireFlags_max", SetWeaponFireFlags, 0, &uvp_wf_max),
    C("CanAttackTarget_null", CanAttackTarget, 0, &uvp_ca_null),
    C("CanAttackTarget_plain", CanAttackTarget, 0, &uvp_ca_plain),
    C("CanAttackTarget_disabled", CanAttackTarget, 0, &uvp_ca_disabled),
    C("CanAttackTarget_dying", CanAttackTarget, 0, &uvp_ca_dying),
    C("CanAttackTarget_destroyed", CanAttackTarget, 0, &uvp_ca_destroyed),
    C("CanAttackTarget_all3", CanAttackTarget, 0, &uvp_ca_all3),
    C("CanAttackTarget_otherflags", CanAttackTarget, 0, &uvp_ca_otherflags),
    C("CanAttackTarget_player", CanAttackTarget, 0, &uvp_ca_player),
    C("CanAttackTarget_playerdisabledcontrols", CanAttackTarget, 0, &uvp_ca_playerdisabledcontrols),
    C("CanAttackTarget_playerotheractorflags", CanAttackTarget, 0, &uvp_ca_playerotheractorflags),
    C("CanAttackTarget_actordisabledcontrols", CanAttackTarget, 0, &uvp_ca_actordisabledcontrols),
    C("CanAttackTarget_playerdyingcontrols", CanAttackTarget, 0, &uvp_ca_playerdyingcontrols),
    C("IsThingMoving_null", IsThingMoving, 0, &uvp_mv_null),
    C("IsThingMoving_free", IsThingMoving, 0, &uvp_mv_free),
    C("IsThingMoving_actornolocal", IsThingMoving, 0, &uvp_mv_actornolocal),
    C("IsThingMoving_actormoving", IsThingMoving, 0, &uvp_mv_actormoving),
    C("IsThingMoving_actoridle", IsThingMoving, 0, &uvp_mv_actoridle),
    C("IsThingMoving_actorothermodes", IsThingMoving, 0, &uvp_mv_actorothermodes),
    C("IsThingMoving_actorallmodes", IsThingMoving, 0, &uvp_mv_actorallmodes),
    C("IsThingMoving_physicsstill", IsThingMoving, 0, &uvp_mv_physicsstill),
    C("IsThingMoving_physicsvel", IsThingMoving, 0, &uvp_mv_physicsvel),
    C("IsThingMoving_physicsangvel", IsThingMoving, 0, &uvp_mv_physicsangvel),
    C("IsThingMoving_physicsboth", IsThingMoving, 0, &uvp_mv_physicsboth),
    C("IsThingMoving_physicsnegzero", IsThingMoving, 0, &uvp_mv_physicsnegzero),
    C("IsThingMoving_physicstiny", IsThingMoving, 0, &uvp_mv_physicstiny),
    C("IsThingMoving_path0", IsThingMoving, 0, &uvp_mv_path0),
    C("IsThingMoving_path1", IsThingMoving, 0, &uvp_mv_path1),
    C("IsThingMoving_path2", IsThingMoving, 0, &uvp_mv_path2),
    C("IsThingMoving_path3", IsThingMoving, 0, &uvp_mv_path3),
    C("IsThingMoving_none", IsThingMoving, 0, &uvp_mv_none),
    C("AIPauseMove_basic", AIPauseMove, 0, &uvp_pm_basic),
    C("AIPauseMove_zero", AIPauseMove, 0, &uvp_pm_zero),
    C("AIPauseMove_negative", AIPauseMove, 0, &uvp_pm_negative),
    C("AIPauseMove_wrap", AIPauseMove, 0, &uvp_pm_wrap),
    C("AIPauseMove_max", AIPauseMove, 0, &uvp_pm_max),
    C("AIPauseMove_min", AIPauseMove, 0, &uvp_pm_min),
    C("AIPauseMove_timezero", AIPauseMove, 0, &uvp_pm_timezero),
    C("AISetMode_moving_zero", AISetMode, 0, &uvp_mode_moving_zero),
    C("AIClearMode_moving_zero", AIClearMode, 0, &uvp_mode_moving_zero),
    C("AISetMode_moving_ones", AISetMode, 0, &uvp_mode_moving_ones),
    C("AIClearMode_moving_ones", AIClearMode, 0, &uvp_mode_moving_ones),
    C("AISetMode_moving_nocont", AISetMode, 0, &uvp_mode_moving_nocont),
    C("AIClearMode_moving_nocont", AIClearMode, 0, &uvp_mode_moving_nocont),
    C("AISetMode_moving_unreg", AISetMode, 0, &uvp_mode_moving_unreg),
    C("AIClearMode_moving_unreg", AIClearMode, 0, &uvp_mode_moving_unreg),
    C("AISetMode_moving_unregnocont", AISetMode, 0, &uvp_mode_moving_unregnocont),
    C("AIClearMode_moving_unregnocont", AIClearMode, 0, &uvp_mode_moving_unregnocont),
    C("AISetMode_searching_zero", AISetMode, 0, &uvp_mode_searching_zero),
    C("AIClearMode_searching_zero", AIClearMode, 0, &uvp_mode_searching_zero),
    C("AISetMode_searching_ones", AISetMode, 0, &uvp_mode_searching_ones),
    C("AIClearMode_searching_ones", AIClearMode, 0, &uvp_mode_searching_ones),
    C("AISetMode_searching_nocont", AISetMode, 0, &uvp_mode_searching_nocont),
    C("AIClearMode_searching_nocont", AIClearMode, 0, &uvp_mode_searching_nocont),
    C("AISetMode_searching_unreg", AISetMode, 0, &uvp_mode_searching_unreg),
    C("AIClearMode_searching_unreg", AIClearMode, 0, &uvp_mode_searching_unreg),
    C("AISetMode_searching_unregnocont", AISetMode, 0, &uvp_mode_searching_unregnocont),
    C("AIClearMode_searching_unregnocont", AIClearMode, 0, &uvp_mode_searching_unregnocont),
    C("AISetMode_attacking_zero", AISetMode, 0, &uvp_mode_attacking_zero),
    C("AIClearMode_attacking_zero", AIClearMode, 0, &uvp_mode_attacking_zero),
    C("AISetMode_attacking_ones", AISetMode, 0, &uvp_mode_attacking_ones),
    C("AIClearMode_attacking_ones", AIClearMode, 0, &uvp_mode_attacking_ones),
    C("AISetMode_attacking_nocont", AISetMode, 0, &uvp_mode_attacking_nocont),
    C("AIClearMode_attacking_nocont", AIClearMode, 0, &uvp_mode_attacking_nocont),
    C("AISetMode_attacking_unreg", AISetMode, 0, &uvp_mode_attacking_unreg),
    C("AIClearMode_attacking_unreg", AIClearMode, 0, &uvp_mode_attacking_unreg),
    C("AISetMode_attacking_unregnocont", AISetMode, 0, &uvp_mode_attacking_unregnocont),
    C("AIClearMode_attacking_unregnocont", AIClearMode, 0, &uvp_mode_attacking_unregnocont),
    C("AISetMode_hunting_zero", AISetMode, 0, &uvp_mode_hunting_zero),
    C("AIClearMode_hunting_zero", AIClearMode, 0, &uvp_mode_hunting_zero),
    C("AISetMode_hunting_ones", AISetMode, 0, &uvp_mode_hunting_ones),
    C("AIClearMode_hunting_ones", AIClearMode, 0, &uvp_mode_hunting_ones),
    C("AISetMode_hunting_nocont", AISetMode, 0, &uvp_mode_hunting_nocont),
    C("AIClearMode_hunting_nocont", AIClearMode, 0, &uvp_mode_hunting_nocont),
    C("AISetMode_hunting_unreg", AISetMode, 0, &uvp_mode_hunting_unreg),
    C("AIClearMode_hunting_unreg", AIClearMode, 0, &uvp_mode_hunting_unreg),
    C("AISetMode_hunting_unregnocont", AISetMode, 0, &uvp_mode_hunting_unregnocont),
    C("AIClearMode_hunting_unregnocont", AIClearMode, 0, &uvp_mode_hunting_unregnocont),
    C("AISetMode_fleeingtowaypoint_zero", AISetMode, 0, &uvp_mode_fleeingtowaypoint_zero),
    C("AIClearMode_fleeingtowaypoint_zero", AIClearMode, 0, &uvp_mode_fleeingtowaypoint_zero),
    C("AISetMode_fleeingtowaypoint_ones", AISetMode, 0, &uvp_mode_fleeingtowaypoint_ones),
    C("AIClearMode_fleeingtowaypoint_ones", AIClearMode, 0, &uvp_mode_fleeingtowaypoint_ones),
    C("AISetMode_fleeingtowaypoint_nocont", AISetMode, 0, &uvp_mode_fleeingtowaypoint_nocont),
    C("AIClearMode_fleeingtowaypoint_nocont", AIClearMode, 0, &uvp_mode_fleeingtowaypoint_nocont),
    C("AISetMode_fleeingtowaypoint_unreg", AISetMode, 0, &uvp_mode_fleeingtowaypoint_unreg),
    C("AIClearMode_fleeingtowaypoint_unreg", AIClearMode, 0, &uvp_mode_fleeingtowaypoint_unreg),
    C("AISetMode_fleeingtowaypoint_unregnocont", AISetMode, 0, &uvp_mode_fleeingtowaypoint_unregnocont),
    C("AIClearMode_fleeingtowaypoint_unregnocont", AIClearMode, 0, &uvp_mode_fleeingtowaypoint_unregnocont),
    C("AISetMode_fleeing_zero", AISetMode, 0, &uvp_mode_fleeing_zero),
    C("AIClearMode_fleeing_zero", AIClearMode, 0, &uvp_mode_fleeing_zero),
    C("AISetMode_fleeing_ones", AISetMode, 0, &uvp_mode_fleeing_ones),
    C("AIClearMode_fleeing_ones", AIClearMode, 0, &uvp_mode_fleeing_ones),
    C("AISetMode_fleeing_nocont", AISetMode, 0, &uvp_mode_fleeing_nocont),
    C("AIClearMode_fleeing_nocont", AIClearMode, 0, &uvp_mode_fleeing_nocont),
    C("AISetMode_fleeing_unreg", AISetMode, 0, &uvp_mode_fleeing_unreg),
    C("AIClearMode_fleeing_unreg", AIClearMode, 0, &uvp_mode_fleeing_unreg),
    C("AISetMode_fleeing_unregnocont", AISetMode, 0, &uvp_mode_fleeing_unregnocont),
    C("AIClearMode_fleeing_unregnocont", AIClearMode, 0, &uvp_mode_fleeing_unregnocont),
    C("AISetMode_block_zero", AISetMode, 0, &uvp_mode_block_zero),
    C("AIClearMode_block_zero", AIClearMode, 0, &uvp_mode_block_zero),
    C("AISetMode_block_ones", AISetMode, 0, &uvp_mode_block_ones),
    C("AIClearMode_block_ones", AIClearMode, 0, &uvp_mode_block_ones),
    C("AISetMode_block_nocont", AISetMode, 0, &uvp_mode_block_nocont),
    C("AIClearMode_block_nocont", AIClearMode, 0, &uvp_mode_block_nocont),
    C("AISetMode_block_unreg", AISetMode, 0, &uvp_mode_block_unreg),
    C("AIClearMode_block_unreg", AIClearMode, 0, &uvp_mode_block_unreg),
    C("AISetMode_block_unregnocont", AISetMode, 0, &uvp_mode_block_unregnocont),
    C("AIClearMode_block_unregnocont", AIClearMode, 0, &uvp_mode_block_unregnocont),
    C("AISetMode_wallcrawling_zero", AISetMode, 0, &uvp_mode_wallcrawling_zero),
    C("AIClearMode_wallcrawling_zero", AIClearMode, 0, &uvp_mode_wallcrawling_zero),
    C("AISetMode_wallcrawling_ones", AISetMode, 0, &uvp_mode_wallcrawling_ones),
    C("AIClearMode_wallcrawling_ones", AIClearMode, 0, &uvp_mode_wallcrawling_ones),
    C("AISetMode_wallcrawling_nocont", AISetMode, 0, &uvp_mode_wallcrawling_nocont),
    C("AIClearMode_wallcrawling_nocont", AIClearMode, 0, &uvp_mode_wallcrawling_nocont),
    C("AISetMode_wallcrawling_unreg", AISetMode, 0, &uvp_mode_wallcrawling_unreg),
    C("AIClearMode_wallcrawling_unreg", AIClearMode, 0, &uvp_mode_wallcrawling_unreg),
    C("AISetMode_wallcrawling_unregnocont", AISetMode, 0, &uvp_mode_wallcrawling_unregnocont),
    C("AIClearMode_wallcrawling_unregnocont", AIClearMode, 0, &uvp_mode_wallcrawling_unregnocont),
    C("AISetMode_turning_zero", AISetMode, 0, &uvp_mode_turning_zero),
    C("AIClearMode_turning_zero", AIClearMode, 0, &uvp_mode_turning_zero),
    C("AISetMode_turning_ones", AISetMode, 0, &uvp_mode_turning_ones),
    C("AIClearMode_turning_ones", AIClearMode, 0, &uvp_mode_turning_ones),
    C("AISetMode_turning_nocont", AISetMode, 0, &uvp_mode_turning_nocont),
    C("AIClearMode_turning_nocont", AIClearMode, 0, &uvp_mode_turning_nocont),
    C("AISetMode_turning_unreg", AISetMode, 0, &uvp_mode_turning_unreg),
    C("AIClearMode_turning_unreg", AIClearMode, 0, &uvp_mode_turning_unreg),
    C("AISetMode_turning_unregnocont", AISetMode, 0, &uvp_mode_turning_unregnocont),
    C("AIClearMode_turning_unregnocont", AIClearMode, 0, &uvp_mode_turning_unregnocont),
    C("AISetMode_traversewpnts_zero", AISetMode, 0, &uvp_mode_traversewpnts_zero),
    C("AIClearMode_traversewpnts_zero", AIClearMode, 0, &uvp_mode_traversewpnts_zero),
    C("AISetMode_traversewpnts_ones", AISetMode, 0, &uvp_mode_traversewpnts_ones),
    C("AIClearMode_traversewpnts_ones", AIClearMode, 0, &uvp_mode_traversewpnts_ones),
    C("AISetMode_traversewpnts_nocont", AISetMode, 0, &uvp_mode_traversewpnts_nocont),
    C("AIClearMode_traversewpnts_nocont", AIClearMode, 0, &uvp_mode_traversewpnts_nocont),
    C("AISetMode_traversewpnts_unreg", AISetMode, 0, &uvp_mode_traversewpnts_unreg),
    C("AIClearMode_traversewpnts_unreg", AIClearMode, 0, &uvp_mode_traversewpnts_unreg),
    C("AISetMode_traversewpnts_unregnocont", AISetMode, 0, &uvp_mode_traversewpnts_unregnocont),
    C("AIClearMode_traversewpnts_unregnocont", AIClearMode, 0, &uvp_mode_traversewpnts_unregnocont),
    C("AISetMode_none_zero", AISetMode, 0, &uvp_mode_none_zero),
    C("AIClearMode_none_zero", AIClearMode, 0, &uvp_mode_none_zero),
    C("AISetMode_none_ones", AISetMode, 0, &uvp_mode_none_ones),
    C("AIClearMode_none_ones", AIClearMode, 0, &uvp_mode_none_ones),
    C("AISetMode_none_nocont", AISetMode, 0, &uvp_mode_none_nocont),
    C("AIClearMode_none_nocont", AIClearMode, 0, &uvp_mode_none_nocont),
    C("AISetMode_none_unreg", AISetMode, 0, &uvp_mode_none_unreg),
    C("AIClearMode_none_unreg", AIClearMode, 0, &uvp_mode_none_unreg),
    C("AISetMode_none_unregnocont", AISetMode, 0, &uvp_mode_none_unregnocont),
    C("AIClearMode_none_unregnocont", AIClearMode, 0, &uvp_mode_none_unregnocont),
    C("AISetMode_combined_zero", AISetMode, 0, &uvp_mode_combined_zero),
    C("AIClearMode_combined_zero", AIClearMode, 0, &uvp_mode_combined_zero),
    C("AISetMode_combined_ones", AISetMode, 0, &uvp_mode_combined_ones),
    C("AIClearMode_combined_ones", AIClearMode, 0, &uvp_mode_combined_ones),
    C("AISetMode_combined_nocont", AISetMode, 0, &uvp_mode_combined_nocont),
    C("AIClearMode_combined_nocont", AIClearMode, 0, &uvp_mode_combined_nocont),
    C("AISetMode_combined_unreg", AISetMode, 0, &uvp_mode_combined_unreg),
    C("AIClearMode_combined_unreg", AIClearMode, 0, &uvp_mode_combined_unreg),
    C("AISetMode_combined_unregnocont", AISetMode, 0, &uvp_mode_combined_unregnocont),
    C("AIClearMode_combined_unregnocont", AIClearMode, 0, &uvp_mode_combined_unregnocont),
    C("AISetMode_highbit_zero", AISetMode, 0, &uvp_mode_highbit_zero),
    C("AIClearMode_highbit_zero", AIClearMode, 0, &uvp_mode_highbit_zero),
    C("AISetMode_highbit_ones", AISetMode, 0, &uvp_mode_highbit_ones),
    C("AIClearMode_highbit_ones", AIClearMode, 0, &uvp_mode_highbit_ones),
    C("AISetMode_highbit_nocont", AISetMode, 0, &uvp_mode_highbit_nocont),
    C("AIClearMode_highbit_nocont", AIClearMode, 0, &uvp_mode_highbit_nocont),
    C("AISetMode_highbit_unreg", AISetMode, 0, &uvp_mode_highbit_unreg),
    C("AIClearMode_highbit_unreg", AIClearMode, 0, &uvp_mode_highbit_unreg),
    C("AISetMode_highbit_unregnocont", AISetMode, 0, &uvp_mode_highbit_unregnocont),
    C("AIClearMode_highbit_unregnocont", AIClearMode, 0, &uvp_mode_highbit_unregnocont),
    C("AISetMode_allbits_zero", AISetMode, 0, &uvp_mode_allbits_zero),
    C("AIClearMode_allbits_zero", AIClearMode, 0, &uvp_mode_allbits_zero),
    C("AISetMode_allbits_ones", AISetMode, 0, &uvp_mode_allbits_ones),
    C("AIClearMode_allbits_ones", AIClearMode, 0, &uvp_mode_allbits_ones),
    C("AISetMode_allbits_nocont", AISetMode, 0, &uvp_mode_allbits_nocont),
    C("AIClearMode_allbits_nocont", AIClearMode, 0, &uvp_mode_allbits_nocont),
    C("AISetMode_allbits_unreg", AISetMode, 0, &uvp_mode_allbits_unreg),
    C("AIClearMode_allbits_unreg", AIClearMode, 0, &uvp_mode_allbits_unreg),
    C("AISetMode_allbits_unregnocont", AISetMode, 0, &uvp_mode_allbits_unregnocont),
    C("AIClearMode_allbits_unregnocont", AIClearMode, 0, &uvp_mode_allbits_unregnocont),
    C("AISetMode_dodging_zero", AISetMode, 0, &uvp_mode_dodging_zero),
    C("AIClearMode_dodging_zero", AIClearMode, 0, &uvp_mode_dodging_zero),
    C("AISetMode_dodging_ones", AISetMode, 0, &uvp_mode_dodging_ones),
    C("AIClearMode_dodging_ones", AIClearMode, 0, &uvp_mode_dodging_ones),
    C("AISetMode_dodging_nocont", AISetMode, 0, &uvp_mode_dodging_nocont),
    C("AIClearMode_dodging_nocont", AIClearMode, 0, &uvp_mode_dodging_nocont),
    C("AISetMode_dodging_unreg", AISetMode, 0, &uvp_mode_dodging_unreg),
    C("AIClearMode_dodging_unreg", AIClearMode, 0, &uvp_mode_dodging_unreg),
    C("AISetMode_dodging_unregnocont", AISetMode, 0, &uvp_mode_dodging_unregnocont),
    C("AIClearMode_dodging_unregnocont", AIClearMode, 0, &uvp_mode_dodging_unregnocont),
    C("AIGetMovePos_none", AIGetMovePos, 0, &uvp_gm_none),
    C("AIGetMovePos_owner", AIGetMovePos, 0, &uvp_gm_owner),
    C("AIGetMovePos_movepos", AIGetMovePos, 0, &uvp_gm_movepos),
    C("AIGetMovePos_home", AIGetMovePos, 0, &uvp_gm_home),
    C("AIGetMovePos_goalthing", AIGetMovePos, 0, &uvp_gm_goalthing),
    C("AIGetMovePos_goalpos", AIGetMovePos, 0, &uvp_gm_goalpos),
    C("AIGetMovePos_awaygoalthing", AIGetMovePos, 0, &uvp_gm_awaygoalthing),
    C("AIGetMovePos_awaygoalpos", AIGetMovePos, 0, &uvp_gm_awaygoalpos),
    C("AIGetMovePos_flee", AIGetMovePos, 0, &uvp_gm_flee),
    C("AIGetMovePos_noflee", AIGetMovePos, 0, &uvp_gm_noflee),
    C("AIGetMovePos_ownerfirst", AIGetMovePos, 0, &uvp_gm_ownerfirst),
    C("AIGetMovePos_moveposfirst", AIGetMovePos, 0, &uvp_gm_moveposfirst),
    C("AIGetMovePos_homefirst", AIGetMovePos, 0, &uvp_gm_homefirst),
    C("AIGetMovePos_goalfirst", AIGetMovePos, 0, &uvp_gm_goalfirst),
    C("AIGetMovePos_unrelated", AIGetMovePos, 0, &uvp_gm_unrelated),
    C("AIGetMovePos_all", AIGetMovePos, 0, &uvp_gm_all),
    C("GetXYHeadingVector_nullthing", Heading, 0, &uvp_hd_nullthing),
    C("GetXYZHeadingVector_nullthing", Heading, 1, &uvp_hd_nullthing),
    C("GetXYHeadingVector_nulldest", Heading, 0, &uvp_hd_nulldest),
    C("GetXYZHeadingVector_nulldest", Heading, 1, &uvp_hd_nulldest),
    C("GetXYHeadingVector_bothnull", Heading, 0, &uvp_hd_bothnull),
    C("GetXYZHeadingVector_bothnull", Heading, 1, &uvp_hd_bothnull),
    C("GetXYHeadingVector_still", Heading, 0, &uvp_hd_still),
    C("GetXYZHeadingVector_still", Heading, 1, &uvp_hd_still),
    C("GetXYHeadingVector_stilltilted", Heading, 0, &uvp_hd_stilltilted),
    C("GetXYZHeadingVector_stilltilted", Heading, 1, &uvp_hd_stilltilted),
    C("GetXYHeadingVector_stillnegzero", Heading, 0, &uvp_hd_stillnegzero),
    C("GetXYZHeadingVector_stillnegzero", Heading, 1, &uvp_hd_stillnegzero),
    C("GetXYHeadingVector_vel", Heading, 0, &uvp_hd_vel),
    C("GetXYZHeadingVector_vel", Heading, 1, &uvp_hd_vel),
    C("GetXYHeadingVector_velflat", Heading, 0, &uvp_hd_velflat),
    C("GetXYZHeadingVector_velflat", Heading, 1, &uvp_hd_velflat),
    C("GetXYHeadingVector_velvertical", Heading, 0, &uvp_hd_velvertical),
    C("GetXYZHeadingVector_velvertical", Heading, 1, &uvp_hd_velvertical),
    C("GetXYHeadingVector_velneg", Heading, 0, &uvp_hd_velneg),
    C("GetXYZHeadingVector_velneg", Heading, 1, &uvp_hd_velneg),
    C("GetXYHeadingVector_veltiny", Heading, 0, &uvp_hd_veltiny),
    C("GetXYZHeadingVector_veltiny", Heading, 1, &uvp_hd_veltiny),
    C("GetXYHeadingVector_velmixedtiny", Heading, 0, &uvp_hd_velmixedtiny),
    C("GetXYZHeadingVector_velmixedtiny", Heading, 1, &uvp_hd_velmixedtiny),
    C("GetXYHeadingVector_velhuge", Heading, 0, &uvp_hd_velhuge),
    C("GetXYZHeadingVector_velhuge", Heading, 1, &uvp_hd_velhuge),
    C("ApplyForce_basic", ApplyForce, 0, &uvp_af_basic),
    C("ApplyForce_zeromass", ApplyForce, 0, &uvp_af_zeromass),
    C("ApplyForce_zeroforce", ApplyForce, 0, &uvp_af_zeroforce),
    C("ApplyForce_negative", ApplyForce, 0, &uvp_af_negative),
    C("ApplyForce_zerodir", ApplyForce, 0, &uvp_af_zerodir),
    C("RandomRotate_basic", RandomRotate, 0, &uvp_rr_basic),
    C("RandomRotate_zeroscalar", RandomRotate, 0, &uvp_rr_zeroscalar),
    C("RandomRotate_zerovec", RandomRotate, 0, &uvp_rr_zerovec),
    C("RandomRotate_negscalar", RandomRotate, 0, &uvp_rr_negscalar),
    C("RandomRotate_zerobothzero", RandomRotate, 0, &uvp_rr_zerobothzero),
    C("RandomRotate_large", RandomRotate, 0, &uvp_rr_large),
    C("AIPlaySoundMode_nulllocal", AIPlaySoundMode, 0, &uvp_ps_nulllocal),
    C("AIPlaySoundMode_nullowner", AIPlaySoundMode, 0, &uvp_ps_nullowner),
    C("AIPlaySoundMode_talkcurious", AIPlaySoundMode, 0, &uvp_ps_talkcurious),
    C("AIPlaySoundMode_talkalert", AIPlaySoundMode, 0, &uvp_ps_talkalert),
    C("AIPlaySoundMode_talkreserved8", AIPlaySoundMode, 0, &uvp_ps_talkreserved8),
    C("AIPlaySoundMode_nottalk", AIPlaySoundMode, 0, &uvp_ps_nottalk),
    C("AIPlaySoundMode_belowrange", AIPlaySoundMode, 0, &uvp_ps_belowrange),
    C("AIPlaySoundMode_aboverange", AIPlaySoundMode, 0, &uvp_ps_aboverange),
    C("AIPlaySoundMode_fire1", AIPlaySoundMode, 0, &uvp_ps_fire1),
    C("AIPlaySoundMode_negative", AIPlaySoundMode, 0, &uvp_ps_negative),
    C("AIPlaySoundMode_huge", AIPlaySoundMode, 0, &uvp_ps_huge),
    C("AIPlaySoundMode_nulllocalnull", AIPlaySoundMode, 0, &uvp_ps_nulllocalnull),
    C("AISetWpnt_ghost", AISetWpnt, 0, &uvp_sw_ghost),
    C("AISetWpnt_ghost0", AISetWpnt, 0, &uvp_sw_ghost0),
    C("AISetWpnt_last", AISetWpnt, 0, &uvp_sw_last),
    C("AISetWpnt_outofrange", AISetWpnt, 0, &uvp_sw_outofrange),
    C("AISetWpnt_wayoutofrange", AISetWpnt, 0, &uvp_sw_wayoutofrange),
    C("AISetWpnt_actor", AISetWpnt, 0, &uvp_sw_actor),
    C("AISetWpnt_null", AISetWpnt, 0, &uvp_sw_null),
    C("AISetWpnt_free", AISetWpnt, 0, &uvp_sw_free),
    C("AISetWpnt_outofrangeactor", AISetWpnt, 0, &uvp_sw_outofrangeactor),
    C("AISetWpntRank_0", WpntValue, 0, &uvp_wv_0),
    C("AISetWpntFlags_0", WpntValue, 1, &uvp_wv_0),
    C("AIClearWpntFlags_0", WpntValue, 2, &uvp_wv_0),
    C("AISetWpntRank_1", WpntValue, 0, &uvp_wv_1),
    C("AISetWpntFlags_1", WpntValue, 1, &uvp_wv_1),
    C("AIClearWpntFlags_1", WpntValue, 2, &uvp_wv_1),
    C("AISetWpntRank_5", WpntValue, 0, &uvp_wv_5),
    C("AISetWpntFlags_5", WpntValue, 1, &uvp_wv_5),
    C("AIClearWpntFlags_5", WpntValue, 2, &uvp_wv_5),
    C("AISetWpntRank_254", WpntValue, 0, &uvp_wv_254),
    C("AISetWpntFlags_254", WpntValue, 1, &uvp_wv_254),
    C("AIClearWpntFlags_254", WpntValue, 2, &uvp_wv_254),
    C("AISetWpntRank_255", WpntValue, 0, &uvp_wv_255),
    C("AISetWpntFlags_255", WpntValue, 1, &uvp_wv_255),
    C("AIClearWpntFlags_255", WpntValue, 2, &uvp_wv_255),
    C("AISetWpntRank_256", WpntValue, 0, &uvp_wv_256),
    C("AISetWpntFlags_256", WpntValue, 1, &uvp_wv_256),
    C("AIClearWpntFlags_256", WpntValue, 2, &uvp_wv_256),
    C("AISetWpntRank_1000", WpntValue, 0, &uvp_wv_1000),
    C("AISetWpntFlags_1000", WpntValue, 1, &uvp_wv_1000),
    C("AIClearWpntFlags_1000", WpntValue, 2, &uvp_wv_1000),
    C("AISetWpntRank_max", WpntValue, 0, &uvp_wv_max),
    C("AISetWpntFlags_max", WpntValue, 1, &uvp_wv_max),
    C("AIClearWpntFlags_max", WpntValue, 2, &uvp_wv_max),
    C("AISetWpntRank_outofrange", WpntValue, 0, &uvp_wv_outofrange),
    C("AISetWpntFlags_outofrange", WpntValue, 1, &uvp_wv_outofrange),
    C("AIClearWpntFlags_outofrange", WpntValue, 2, &uvp_wv_outofrange),
    C("AISetWpntRank_huge", WpntValue, 0, &uvp_wv_huge),
    C("AISetWpntFlags_huge", WpntValue, 1, &uvp_wv_huge),
    C("AIClearWpntFlags_huge", WpntValue, 2, &uvp_wv_huge),
    C("AISetWpntRank_disabled", WpntValue, 0, &uvp_wv_disabled),
    C("AISetWpntFlags_disabled", WpntValue, 1, &uvp_wv_disabled),
    C("AIClearWpntFlags_disabled", WpntValue, 2, &uvp_wv_disabled),
    C("AISetWpntRank_layer1", WpntValue, 0, &uvp_wv_layer1),
    C("AISetWpntFlags_layer1", WpntValue, 1, &uvp_wv_layer1),
    C("AIClearWpntFlags_layer1", WpntValue, 2, &uvp_wv_layer1),
    C("AISetWpntRank_layers", WpntValue, 0, &uvp_wv_layers),
    C("AISetWpntFlags_layers", WpntValue, 1, &uvp_wv_layers),
    C("AIClearWpntFlags_layers", WpntValue, 2, &uvp_wv_layers),
    C("AISetWpntRank_flagsmask", WpntValue, 0, &uvp_wv_flagsmask),
    C("AISetWpntFlags_flagsmask", WpntValue, 1, &uvp_wv_flagsmask),
    C("AIClearWpntFlags_flagsmask", WpntValue, 2, &uvp_wv_flagsmask),
    C("AISetWpntRank_lowbits", WpntValue, 0, &uvp_wv_lowbits),
    C("AISetWpntFlags_lowbits", WpntValue, 1, &uvp_wv_lowbits),
    C("AIClearWpntFlags_lowbits", WpntValue, 2, &uvp_wv_lowbits),
    C("AISetWpntRank_belowdisabled", WpntValue, 0, &uvp_wv_belowdisabled),
    C("AISetWpntFlags_belowdisabled", WpntValue, 1, &uvp_wv_belowdisabled),
    C("AIClearWpntFlags_belowdisabled", WpntValue, 2, &uvp_wv_belowdisabled),
    C("AISetWpntRank_lowandhigh", WpntValue, 0, &uvp_wv_lowandhigh),
    C("AISetWpntFlags_lowandhigh", WpntValue, 1, &uvp_wv_lowandhigh),
    C("AIClearWpntFlags_lowandhigh", WpntValue, 2, &uvp_wv_lowandhigh),
    C("AISetWpntRank_allones", WpntValue, 0, &uvp_wv_allones),
    C("AISetWpntFlags_allones", WpntValue, 1, &uvp_wv_allones),
    C("AIClearWpntFlags_allones", WpntValue, 2, &uvp_wv_allones),
    C("AISetWpntRank_zeroflags", WpntValue, 0, &uvp_wv_zeroflags),
    C("AISetWpntFlags_zeroflags", WpntValue, 1, &uvp_wv_zeroflags),
    C("AIClearWpntFlags_zeroflags", WpntValue, 2, &uvp_wv_zeroflags),
    C("AISetWpntRank_flagsoutofrange", WpntValue, 0, &uvp_wv_flagsoutofrange),
    C("AISetWpntFlags_flagsoutofrange", WpntValue, 1, &uvp_wv_flagsoutofrange),
    C("AIClearWpntFlags_flagsoutofrange", WpntValue, 2, &uvp_wv_flagsoutofrange),
    C("AISetActiveWpntLayer_0", SetActiveLayer, 0, &uvp_ly_0),
    C("AISetActiveWpntLayer_layer0", SetActiveLayer, 0, &uvp_ly_layer0),
    C("AISetActiveWpntLayer_layer3", SetActiveLayer, 0, &uvp_ly_layer3),
    C("AISetActiveWpntLayer_layer5", SetActiveLayer, 0, &uvp_ly_layer5),
    C("AISetActiveWpntLayer_all", SetActiveLayer, 0, &uvp_ly_all),
    C("AISetActiveWpntLayer_two", SetActiveLayer, 0, &uvp_ly_two),
    C("AISetActiveWpntLayer_invalidlow", SetActiveLayer, 0, &uvp_ly_invalidlow),
    C("AISetActiveWpntLayer_invalidhigh", SetActiveLayer, 0, &uvp_ly_invalidhigh),
    C("AISetActiveWpntLayer_disabled", SetActiveLayer, 0, &uvp_ly_disabled),
    C("AISetActiveWpntLayer_allones", SetActiveLayer, 0, &uvp_ly_allones),
    C("AISetActiveWpntLayer_layerplusinvalid", SetActiveLayer, 0, &uvp_ly_layerplusinvalid),
    C("AIConnectWpnts_oneway", AIConnectWpnts, 0, &uvp_cw_oneway),
    C("AIConnectWpnts_both", AIConnectWpnts, 0, &uvp_cw_both),
    C("AIConnectWpnts_bothvalue2", AIConnectWpnts, 0, &uvp_cw_bothvalue2),
    C("AIConnectWpnts_bothneg", AIConnectWpnts, 0, &uvp_cw_bothneg),
    C("AIConnectWpnts_self", AIConnectWpnts, 0, &uvp_cw_self),
    C("AIConnectWpnts_aoutofrange", AIConnectWpnts, 0, &uvp_cw_aoutofrange),
    C("AIConnectWpnts_boutofrange", AIConnectWpnts, 0, &uvp_cw_boutofrange),
    C("AIConnectWpnts_bothoutofrange", AIConnectWpnts, 0, &uvp_cw_bothoutofrange),
    C("AIConnectWpnts_ainvalid", AIConnectWpnts, 0, &uvp_cw_ainvalid),
    C("AIConnectWpnts_binvalid", AIConnectWpnts, 0, &uvp_cw_binvalid),
    C("AIConnectWpnts_bothinvalid", AIConnectWpnts, 0, &uvp_cw_bothinvalid),
    C("AIConnectWpnts_fulla", AIConnectWpnts, 0, &uvp_cw_fulla),
    C("AIConnectWpnts_fulla_both", AIConnectWpnts, 0, &uvp_cw_fulla_both),
    C("AIConnectWpnts_fullb_both", AIConnectWpnts, 0, &uvp_cw_fullb_both),
    C("AIConnectWpnts_fullb_oneway", AIConnectWpnts, 0, &uvp_cw_fullb_oneway),
    C("AIConnectWpnts_fullboth", AIConnectWpnts, 0, &uvp_cw_fullboth),
    C("CreateAIWaypoint_empty", Owners, 0, &uvp_ow_empty),
    C("ClearAIWaypoint_empty", Owners, 1, &uvp_ow_empty),
    C("GetAIWaypointNum_empty", Owners, 2, &uvp_ow_empty),
    C("CreateAIWaypoint_emptydeg0", Owners, 0, &uvp_ow_emptydeg0),
    C("ClearAIWaypoint_emptydeg0", Owners, 1, &uvp_ow_emptydeg0),
    C("GetAIWaypointNum_emptydeg0", Owners, 2, &uvp_ow_emptydeg0),
    C("CreateAIWaypoint_owninslot3", Owners, 0, &uvp_ow_owninslot3),
    C("ClearAIWaypoint_owninslot3", Owners, 1, &uvp_ow_owninslot3),
    C("GetAIWaypointNum_owninslot3", Owners, 2, &uvp_ow_owninslot3),
    C("CreateAIWaypoint_owninslot0", Owners, 0, &uvp_ow_owninslot0),
    C("ClearAIWaypoint_owninslot0", Owners, 1, &uvp_ow_owninslot0),
    C("GetAIWaypointNum_owninslot0", Owners, 2, &uvp_ow_owninslot0),
    C("CreateAIWaypoint_ownlast", Owners, 0, &uvp_ow_ownlast),
    C("ClearAIWaypoint_ownlast", Owners, 1, &uvp_ow_ownlast),
    C("GetAIWaypointNum_ownlast", Owners, 2, &uvp_ow_ownlast),
    C("CreateAIWaypoint_full", Owners, 0, &uvp_ow_full),
    C("ClearAIWaypoint_full", Owners, 1, &uvp_ow_full),
    C("GetAIWaypointNum_full", Owners, 2, &uvp_ow_full),
    C("CreateAIWaypoint_freefirstthenown", Owners, 0, &uvp_ow_freefirstthenown),
    C("ClearAIWaypoint_freefirstthenown", Owners, 1, &uvp_ow_freefirstthenown),
    C("GetAIWaypointNum_freefirstthenown", Owners, 2, &uvp_ow_freefirstthenown),
    C("CreateAIWaypoint_firstfreethenmatch", Owners, 0, &uvp_ow_firstfreethenmatch),
    C("ClearAIWaypoint_firstfreethenmatch", Owners, 1, &uvp_ow_firstfreethenmatch),
    C("GetAIWaypointNum_firstfreethenmatch", Owners, 2, &uvp_ow_firstfreethenmatch),
    C("CreateAIWaypoint_onefreelast", Owners, 0, &uvp_ow_onefreelast),
    C("ClearAIWaypoint_onefreelast", Owners, 1, &uvp_ow_onefreelast),
    C("GetAIWaypointNum_onefreelast", Owners, 2, &uvp_ow_onefreelast),
    C("CreateAIWaypoint_middlefree", Owners, 0, &uvp_ow_middlefree),
    C("ClearAIWaypoint_middlefree", Owners, 1, &uvp_ow_middlefree),
    C("GetAIWaypointNum_middlefree", Owners, 2, &uvp_ow_middlefree),
    C("CreateAIWaypoint_degtiny", Owners, 0, &uvp_ow_degtiny),
    C("ClearAIWaypoint_degtiny", Owners, 1, &uvp_ow_degtiny),
    C("GetAIWaypointNum_degtiny", Owners, 2, &uvp_ow_degtiny),
    C("CreateAIWaypoint_deghuge", Owners, 0, &uvp_ow_deghuge),
    C("ClearAIWaypoint_deghuge", Owners, 1, &uvp_ow_deghuge),
    C("GetAIWaypointNum_deghuge", Owners, 2, &uvp_ow_deghuge),
    C("CreateAIWaypoint_fullown", Owners, 0, &uvp_ow_fullown),
    C("ClearAIWaypoint_fullown", Owners, 1, &uvp_ow_fullown),
    C("GetAIWaypointNum_fullown", Owners, 2, &uvp_ow_fullown),
    C("CompareAIWpntDistances_less", Compare, 0, &uvp_cd_less),
    C("CompareAIWpntDistances_greater", Compare, 0, &uvp_cd_greater),
    C("CompareAIWpntDistances_equal", Compare, 0, &uvp_cd_equal),
    C("CompareAIWpntDistances_null1", Compare, 0, &uvp_cd_null1),
    C("CompareAIWpntDistances_null2", Compare, 0, &uvp_cd_null2),
    C("CompareAIWpntDistances_bothnull", Compare, 0, &uvp_cd_bothnull),
    C("CompareAIWpntDistances_negative", Compare, 0, &uvp_cd_negative),
    C("CompareAIWpntDistances_negzero", Compare, 0, &uvp_cd_negzero),
    C("CompareAIWpntDistances_tiny", Compare, 0, &uvp_cd_tiny),
    C("CompareAIWpntDistances_denormal", Compare, 0, &uvp_cd_denormal),
    C("AIGetWpntThing_0", AIGetWpntThing, 0, &uvp_gt_0),
    C("AIGetWpntThing_ghost", AIGetWpntThing, 0, &uvp_gt_ghost),
    C("AIGetWpntThing_last", AIGetWpntThing, 0, &uvp_gt_last),
    C("AIGetWpntThing_outside", AIGetWpntThing, 0, &uvp_gt_outside),
    C("AIGetWpntThing_neg1", AIGetWpntThing, 0, &uvp_gt_neg1),
    C("RankWpntsFromThing_chain_first", RankFromThing, 0, &uvp_rank_chain_first),
    C("RankWpntsFromThing_chain_mid", RankFromThing, 0, &uvp_rank_chain_mid),
    C("RankWpntsFromThing_chain_last", RankFromThing, 0, &uvp_rank_chain_last),
    C("RankWpntsFromThing_chain_nowpnt", RankFromThing, 0, &uvp_rank_chain_nowpnt),
    C("RankWpntsFromThing_chain_owner", RankFromThing, 0, &uvp_rank_chain_owner),
    C("RankWpntsFromThing_disabled_first", RankFromThing, 0, &uvp_rank_disabled_first),
    C("RankWpntsFromThing_disabled_last", RankFromThing, 0, &uvp_rank_disabled_last),
    C("RankWpntsFromThing_otherlayer_first", RankFromThing, 0, &uvp_rank_otherlayer_first),
    C("RankWpntsFromThing_otherlayer_layer1", RankFromThing, 0, &uvp_rank_otherlayer_layer1),
    C("RankWpntsFromThing_otherlayer_alllayers", RankFromThing, 0, &uvp_rank_otherlayer_alllayers),
    C("RankWpntsFromThing_star_hub", RankFromThing, 0, &uvp_rank_star_hub),
    C("RankWpntsFromThing_star_leaf", RankFromThing, 0, &uvp_rank_star_leaf),
    C("RankWpntsFromThing_ring_first", RankFromThing, 0, &uvp_rank_ring_first),
    C("RankWpntsFromThing_ring_tail", RankFromThing, 0, &uvp_rank_ring_tail),
    C("RankWpntsFromThing_split_first", RankFromThing, 0, &uvp_rank_split_first),
    C("RankWpntsFromThing_split_second", RankFromThing, 0, &uvp_rank_split_second),
    C("RankWpntsFromThing_highrank_first", RankFromThing, 0, &uvp_rank_highrank_first),
    C("RankWpntsFromThing_oneway_first", RankFromThing, 0, &uvp_rank_oneway_first),
    C("RankWpntsFromThing_oneway_last", RankFromThing, 0, &uvp_rank_oneway_last),
    C("RankWpntsFromThing_none_any", RankFromThing, 0, &uvp_rank_none_any),
    C("RankWpntsFromPlayer_none", RankFromPlayer, 0, &uvp_rank_player_none),
    C("RankWpntsFromPlayer_chain", RankFromPlayer, 0, &uvp_rank_player_chain),
    C("RankWpntsFromPlayer_chain_far", RankFromPlayer, 0, &uvp_rank_player_chain_far),
    C("RankWpntsFromPlayer_invisible", RankFromPlayer, 0, &uvp_rank_player_invisible),
    C("RankWpntsFromPlayer_onwaypoint", RankFromPlayer, 0, &uvp_rank_player_onwaypoint),
    C("RankWpntsFromPlayer_star", RankFromPlayer, 0, &uvp_rank_player_star),
    C("RankWpntsFromPlayer_ring", RankFromPlayer, 0, &uvp_rank_player_ring),
    C("RankWpntsFromPlayer_ring_layer1", RankFromPlayer, 0, &uvp_rank_player_ring_layer1),
    C("RankWpntsFromPlayer_disabled", RankFromPlayer, 0, &uvp_rank_player_disabled),
    C("RankWpntsFromPlayer_split", RankFromPlayer, 0, &uvp_rank_player_split),
    C("RankWpntsFromPlayer_nowpnts", RankFromPlayer, 0, &uvp_rank_player_nowpnts),
    C("RankWpntsFromPlayer_highrank", RankFromPlayer, 0, &uvp_rank_player_highrank),
    C("RankWpntsFromPlayer_invisibleflags", RankFromPlayer, 0, &uvp_rank_player_invisibleflags),
    C("SelectWpntLink_plain_ties", SelectWpntLink, 0, &uvp_sel_plain_ties),
    C("SelectWpntLink_sweeprand_plain_ties", SelectWpntLink, 1, &uvp_sel_plain_ties),
    C("SelectWpntLink_sweeppriority_plain_ties", SelectWpntLink, 2, &uvp_sel_plain_ties),
    C("SelectWpntLink_sweeprank_plain_ties", SelectWpntLink, 3, &uvp_sel_plain_ties),
    C("SelectWpntLink_plain_ties_seed2", SelectWpntLink, 0, &uvp_sel_plain_ties_seed2),
    C("SelectWpntLink_sweeprand_plain_ties_seed2", SelectWpntLink, 1, &uvp_sel_plain_ties_seed2),
    C("SelectWpntLink_sweeppriority_plain_ties_seed2", SelectWpntLink, 2, &uvp_sel_plain_ties_seed2),
    C("SelectWpntLink_sweeprank_plain_ties_seed2", SelectWpntLink, 3, &uvp_sel_plain_ties_seed2),
    C("SelectWpntLink_plain_ties_seed3", SelectWpntLink, 0, &uvp_sel_plain_ties_seed3),
    C("SelectWpntLink_sweeprand_plain_ties_seed3", SelectWpntLink, 1, &uvp_sel_plain_ties_seed3),
    C("SelectWpntLink_sweeppriority_plain_ties_seed3", SelectWpntLink, 2, &uvp_sel_plain_ties_seed3),
    C("SelectWpntLink_sweeprank_plain_ties_seed3", SelectWpntLink, 3, &uvp_sel_plain_ties_seed3),
    C("SelectWpntLink_plain_higher", SelectWpntLink, 0, &uvp_sel_plain_higher),
    C("SelectWpntLink_sweeprand_plain_higher", SelectWpntLink, 1, &uvp_sel_plain_higher),
    C("SelectWpntLink_sweeppriority_plain_higher", SelectWpntLink, 2, &uvp_sel_plain_higher),
    C("SelectWpntLink_sweeprank_plain_higher", SelectWpntLink, 3, &uvp_sel_plain_higher),
    C("SelectWpntLink_plain_lower", SelectWpntLink, 0, &uvp_sel_plain_lower),
    C("SelectWpntLink_sweeprand_plain_lower", SelectWpntLink, 1, &uvp_sel_plain_lower),
    C("SelectWpntLink_sweeppriority_plain_lower", SelectWpntLink, 2, &uvp_sel_plain_lower),
    C("SelectWpntLink_sweeprank_plain_lower", SelectWpntLink, 3, &uvp_sel_plain_lower),
    C("SelectWpntLink_plain_linknum1", SelectWpntLink, 0, &uvp_sel_plain_linknum1),
    C("SelectWpntLink_sweeprand_plain_linknum1", SelectWpntLink, 1, &uvp_sel_plain_linknum1),
    C("SelectWpntLink_sweeppriority_plain_linknum1", SelectWpntLink, 2, &uvp_sel_plain_linknum1),
    C("SelectWpntLink_sweeprank_plain_linknum1", SelectWpntLink, 3, &uvp_sel_plain_linknum1),
    C("SelectWpntLink_plain_disabled", SelectWpntLink, 0, &uvp_sel_plain_disabled),
    C("SelectWpntLink_sweeprand_plain_disabled", SelectWpntLink, 1, &uvp_sel_plain_disabled),
    C("SelectWpntLink_sweeppriority_plain_disabled", SelectWpntLink, 2, &uvp_sel_plain_disabled),
    C("SelectWpntLink_sweeprank_plain_disabled", SelectWpntLink, 3, &uvp_sel_plain_disabled),
    C("SelectWpntLink_plain_otherlayer", SelectWpntLink, 0, &uvp_sel_plain_otherlayer),
    C("SelectWpntLink_sweeprand_plain_otherlayer", SelectWpntLink, 1, &uvp_sel_plain_otherlayer),
    C("SelectWpntLink_sweeppriority_plain_otherlayer", SelectWpntLink, 2, &uvp_sel_plain_otherlayer),
    C("SelectWpntLink_sweeprank_plain_otherlayer", SelectWpntLink, 3, &uvp_sel_plain_otherlayer),
    C("SelectWpntLink_plain_single", SelectWpntLink, 0, &uvp_sel_plain_single),
    C("SelectWpntLink_sweeprand_plain_single", SelectWpntLink, 1, &uvp_sel_plain_single),
    C("SelectWpntLink_sweeppriority_plain_single", SelectWpntLink, 2, &uvp_sel_plain_single),
    C("SelectWpntLink_sweeprank_plain_single", SelectWpntLink, 3, &uvp_sel_plain_single),
    C("SelectWpntLink_plain_lastlink", SelectWpntLink, 0, &uvp_sel_plain_lastlink),
    C("SelectWpntLink_sweeprand_plain_lastlink", SelectWpntLink, 1, &uvp_sel_plain_lastlink),
    C("SelectWpntLink_sweeppriority_plain_lastlink", SelectWpntLink, 2, &uvp_sel_plain_lastlink),
    C("SelectWpntLink_sweeprank_plain_lastlink", SelectWpntLink, 3, &uvp_sel_plain_lastlink),
    C("SelectWpntLink_plain_leaddisabled", SelectWpntLink, 0, &uvp_sel_plain_leaddisabled),
    C("SelectWpntLink_sweeprand_plain_leaddisabled", SelectWpntLink, 1, &uvp_sel_plain_leaddisabled),
    C("SelectWpntLink_sweeppriority_plain_leaddisabled", SelectWpntLink, 2, &uvp_sel_plain_leaddisabled),
    C("SelectWpntLink_sweeprank_plain_leaddisabled", SelectWpntLink, 3, &uvp_sel_plain_leaddisabled),
    C("SelectWpntLink_plain_attackingtargetless", SelectWpntLink, 0, &uvp_sel_plain_attackingtargetless),
    C("SelectWpntLink_sweeprand_plain_attackingtargetless", SelectWpntLink, 1, &uvp_sel_plain_attackingtargetless),
    C("SelectWpntLink_sweeppriority_plain_attackingtargetless", SelectWpntLink, 2, &uvp_sel_plain_attackingtargetless),
    C("SelectWpntLink_sweeprank_plain_attackingtargetless", SelectWpntLink, 3, &uvp_sel_plain_attackingtargetless),
    C("SelectWpntLink_attack_backtrack", SelectWpntLink, 0, &uvp_sel_attack_backtrack),
    C("SelectWpntLink_sweeprand_attack_backtrack", SelectWpntLink, 1, &uvp_sel_attack_backtrack),
    C("SelectWpntLink_sweeppriority_attack_backtrack", SelectWpntLink, 2, &uvp_sel_attack_backtrack),
    C("SelectWpntLink_sweeprank_attack_backtrack", SelectWpntLink, 3, &uvp_sel_attack_backtrack),
    C("SelectWpntLink_attack_backtrack_lead", SelectWpntLink, 0, &uvp_sel_attack_backtrack_lead),
    C("SelectWpntLink_sweeprand_attack_backtrack_lead", SelectWpntLink, 1, &uvp_sel_attack_backtrack_lead),
    C("SelectWpntLink_sweeppriority_attack_backtrack_lead", SelectWpntLink, 2, &uvp_sel_attack_backtrack_lead),
    C("SelectWpntLink_sweeprank_attack_backtrack_lead", SelectWpntLink, 3, &uvp_sel_attack_backtrack_lead),
    C("SelectWpntLink_attack_lower_seed1", SelectWpntLink, 0, &uvp_sel_attack_lower_seed1),
    C("SelectWpntLink_sweeprand_attack_lower_seed1", SelectWpntLink, 1, &uvp_sel_attack_lower_seed1),
    C("SelectWpntLink_sweeppriority_attack_lower_seed1", SelectWpntLink, 2, &uvp_sel_attack_lower_seed1),
    C("SelectWpntLink_sweeprank_attack_lower_seed1", SelectWpntLink, 3, &uvp_sel_attack_lower_seed1),
    C("SelectWpntLink_attack_lower_seed2", SelectWpntLink, 0, &uvp_sel_attack_lower_seed2),
    C("SelectWpntLink_sweeprand_attack_lower_seed2", SelectWpntLink, 1, &uvp_sel_attack_lower_seed2),
    C("SelectWpntLink_sweeppriority_attack_lower_seed2", SelectWpntLink, 2, &uvp_sel_attack_lower_seed2),
    C("SelectWpntLink_sweeprank_attack_lower_seed2", SelectWpntLink, 3, &uvp_sel_attack_lower_seed2),
    C("SelectWpntLink_attack_lower_seed3", SelectWpntLink, 0, &uvp_sel_attack_lower_seed3),
    C("SelectWpntLink_sweeprand_attack_lower_seed3", SelectWpntLink, 1, &uvp_sel_attack_lower_seed3),
    C("SelectWpntLink_sweeppriority_attack_lower_seed3", SelectWpntLink, 2, &uvp_sel_attack_lower_seed3),
    C("SelectWpntLink_sweeprank_attack_lower_seed3", SelectWpntLink, 3, &uvp_sel_attack_lower_seed3),
    C("SelectWpntLink_attack_lower_wide", SelectWpntLink, 0, &uvp_sel_attack_lower_wide),
    C("SelectWpntLink_sweeprand_attack_lower_wide", SelectWpntLink, 1, &uvp_sel_attack_lower_wide),
    C("SelectWpntLink_sweeppriority_attack_lower_wide", SelectWpntLink, 2, &uvp_sel_attack_lower_wide),
    C("SelectWpntLink_sweeprank_attack_lower_wide", SelectWpntLink, 3, &uvp_sel_attack_lower_wide),
    C("SelectWpntLink_attack_higher", SelectWpntLink, 0, &uvp_sel_attack_higher),
    C("SelectWpntLink_sweeprand_attack_higher", SelectWpntLink, 1, &uvp_sel_attack_higher),
    C("SelectWpntLink_sweeppriority_attack_higher", SelectWpntLink, 2, &uvp_sel_attack_higher),
    C("SelectWpntLink_sweeprank_attack_higher", SelectWpntLink, 3, &uvp_sel_attack_higher),
    C("SelectWpntLink_search_ties", SelectWpntLink, 0, &uvp_sel_search_ties),
    C("SelectWpntLink_sweeprand_search_ties", SelectWpntLink, 1, &uvp_sel_search_ties),
    C("SelectWpntLink_sweeppriority_search_ties", SelectWpntLink, 2, &uvp_sel_search_ties),
    C("SelectWpntLink_sweeprank_search_ties", SelectWpntLink, 3, &uvp_sel_search_ties),
    C("SelectWpntLink_search_lower", SelectWpntLink, 0, &uvp_sel_search_lower),
    C("SelectWpntLink_sweeprand_search_lower", SelectWpntLink, 1, &uvp_sel_search_lower),
    C("SelectWpntLink_sweeppriority_search_lower", SelectWpntLink, 2, &uvp_sel_search_lower),
    C("SelectWpntLink_sweeprank_search_lower", SelectWpntLink, 3, &uvp_sel_search_lower),
    C("SelectWpntLink_search_lower_seed2", SelectWpntLink, 0, &uvp_sel_search_lower_seed2),
    C("SelectWpntLink_sweeprand_search_lower_seed2", SelectWpntLink, 1, &uvp_sel_search_lower_seed2),
    C("SelectWpntLink_sweeppriority_search_lower_seed2", SelectWpntLink, 2, &uvp_sel_search_lower_seed2),
    C("SelectWpntLink_sweeprank_search_lower_seed2", SelectWpntLink, 3, &uvp_sel_search_lower_seed2),
    C("SelectWpntLink_attack_flee_nothing", SelectWpntLink, 0, &uvp_sel_attack_flee_nothing),
    C("SelectWpntLink_sweeprand_attack_flee_nothing", SelectWpntLink, 1, &uvp_sel_attack_flee_nothing),
    C("SelectWpntLink_sweeppriority_attack_flee_nothing", SelectWpntLink, 2, &uvp_sel_attack_flee_nothing),
    C("SelectWpntLink_sweeprank_attack_flee_nothing", SelectWpntLink, 3, &uvp_sel_attack_flee_nothing),
    C("SelectWpntLink_hunt_target_playernear0", SelectWpntLink, 0, &uvp_sel_hunt_target_playernear0),
    C("SelectWpntLink_sweeprand_hunt_target_playernear0", SelectWpntLink, 1, &uvp_sel_hunt_target_playernear0),
    C("SelectWpntLink_sweeppriority_hunt_target_playernear0", SelectWpntLink, 2, &uvp_sel_hunt_target_playernear0),
    C("SelectWpntLink_sweeprank_hunt_target_playernear0", SelectWpntLink, 3, &uvp_sel_hunt_target_playernear0),
    C("SelectWpntLink_hunt_target_backtrack", SelectWpntLink, 0, &uvp_sel_hunt_target_backtrack),
    C("SelectWpntLink_sweeprand_hunt_target_backtrack", SelectWpntLink, 1, &uvp_sel_hunt_target_backtrack),
    C("SelectWpntLink_sweeppriority_hunt_target_backtrack", SelectWpntLink, 2, &uvp_sel_hunt_target_backtrack),
    C("SelectWpntLink_sweeprank_hunt_target_backtrack", SelectWpntLink, 3, &uvp_sel_hunt_target_backtrack),
    C("SelectWpntLink_hunt_target_backtrack_lead", SelectWpntLink, 0, &uvp_sel_hunt_target_backtrack_lead),
    C("SelectWpntLink_sweeprand_hunt_target_backtrack_lead", SelectWpntLink, 1, &uvp_sel_hunt_target_backtrack_lead),
    C("SelectWpntLink_sweeppriority_hunt_target_backtrack_lead", SelectWpntLink, 2, &uvp_sel_hunt_target_backtrack_lead),
    C("SelectWpntLink_sweeprank_hunt_target_backtrack_lead", SelectWpntLink, 3, &uvp_sel_hunt_target_backtrack_lead),
    C("SelectWpntLink_hunt_target_noplayer", SelectWpntLink, 0, &uvp_sel_hunt_target_noplayer),
    C("SelectWpntLink_sweeprand_hunt_target_noplayer", SelectWpntLink, 1, &uvp_sel_hunt_target_noplayer),
    C("SelectWpntLink_sweeppriority_hunt_target_noplayer", SelectWpntLink, 2, &uvp_sel_hunt_target_noplayer),
    C("SelectWpntLink_sweeprank_hunt_target_noplayer", SelectWpntLink, 3, &uvp_sel_hunt_target_noplayer),
    C("SelectWpntLink_hunt_target_noplayer_ranklower", SelectWpntLink, 0, &uvp_sel_hunt_target_noplayer_ranklower),
    C("SelectWpntLink_sweeprand_hunt_target_noplayer_ranklower", SelectWpntLink, 1, &uvp_sel_hunt_target_noplayer_ranklower),
    C("SelectWpntLink_sweeppriority_hunt_target_noplayer_ranklower", SelectWpntLink, 2, &uvp_sel_hunt_target_noplayer_ranklower),
    C("SelectWpntLink_sweeprank_hunt_target_noplayer_ranklower", SelectWpntLink, 3, &uvp_sel_hunt_target_noplayer_ranklower),
    C("SelectWpntLink_hunt_target_noplayer_ranktie", SelectWpntLink, 0, &uvp_sel_hunt_target_noplayer_ranktie),
    C("SelectWpntLink_sweeprand_hunt_target_noplayer_ranktie", SelectWpntLink, 1, &uvp_sel_hunt_target_noplayer_ranktie),
    C("SelectWpntLink_sweeppriority_hunt_target_noplayer_ranktie", SelectWpntLink, 2, &uvp_sel_hunt_target_noplayer_ranktie),
    C("SelectWpntLink_sweeprank_hunt_target_noplayer_ranktie", SelectWpntLink, 3, &uvp_sel_hunt_target_noplayer_ranktie),
    C("SelectWpntLink_hunt_notarget", SelectWpntLink, 0, &uvp_sel_hunt_notarget),
    C("SelectWpntLink_sweeprand_hunt_notarget", SelectWpntLink, 1, &uvp_sel_hunt_notarget),
    C("SelectWpntLink_sweeppriority_hunt_notarget", SelectWpntLink, 2, &uvp_sel_hunt_notarget),
    C("SelectWpntLink_sweeprank_hunt_notarget", SelectWpntLink, 3, &uvp_sel_hunt_notarget),
    C("SelectWpntLink_hunt_attacking_notarget", SelectWpntLink, 0, &uvp_sel_hunt_attacking_notarget),
    C("SelectWpntLink_sweeprand_hunt_attacking_notarget", SelectWpntLink, 1, &uvp_sel_hunt_attacking_notarget),
    C("SelectWpntLink_sweeppriority_hunt_attacking_notarget", SelectWpntLink, 2, &uvp_sel_hunt_attacking_notarget),
    C("SelectWpntLink_sweeprank_hunt_attacking_notarget", SelectWpntLink, 3, &uvp_sel_hunt_attacking_notarget),
    C("SelectWpntLink_hunt_flee_toplayer", SelectWpntLink, 0, &uvp_sel_hunt_flee_toplayer),
    C("SelectWpntLink_sweeprand_hunt_flee_toplayer", SelectWpntLink, 1, &uvp_sel_hunt_flee_toplayer),
    C("SelectWpntLink_sweeppriority_hunt_flee_toplayer", SelectWpntLink, 2, &uvp_sel_hunt_flee_toplayer),
    C("SelectWpntLink_sweeprank_hunt_flee_toplayer", SelectWpntLink, 3, &uvp_sel_hunt_flee_toplayer),
    C("SelectWpntLink_flee_ranks", SelectWpntLink, 0, &uvp_sel_flee_ranks),
    C("SelectWpntLink_sweeprand_flee_ranks", SelectWpntLink, 1, &uvp_sel_flee_ranks),
    C("SelectWpntLink_sweeppriority_flee_ranks", SelectWpntLink, 2, &uvp_sel_flee_ranks),
    C("SelectWpntLink_sweeprank_flee_ranks", SelectWpntLink, 3, &uvp_sel_flee_ranks),
    C("SelectWpntLink_flee_ranks_leadmax", SelectWpntLink, 0, &uvp_sel_flee_ranks_leadmax),
    C("SelectWpntLink_sweeprand_flee_ranks_leadmax", SelectWpntLink, 1, &uvp_sel_flee_ranks_leadmax),
    C("SelectWpntLink_sweeppriority_flee_ranks_leadmax", SelectWpntLink, 2, &uvp_sel_flee_ranks_leadmax),
    C("SelectWpntLink_sweeprank_flee_ranks_leadmax", SelectWpntLink, 3, &uvp_sel_flee_ranks_leadmax),
    C("SelectWpntLink_flee_ranks_nextmax", SelectWpntLink, 0, &uvp_sel_flee_ranks_nextmax),
    C("SelectWpntLink_sweeprand_flee_ranks_nextmax", SelectWpntLink, 1, &uvp_sel_flee_ranks_nextmax),
    C("SelectWpntLink_sweeppriority_flee_ranks_nextmax", SelectWpntLink, 2, &uvp_sel_flee_ranks_nextmax),
    C("SelectWpntLink_sweeprank_flee_ranks_nextmax", SelectWpntLink, 3, &uvp_sel_flee_ranks_nextmax),
    C("SelectWpntLink_flee_ranks_bothmax", SelectWpntLink, 0, &uvp_sel_flee_ranks_bothmax),
    C("SelectWpntLink_sweeprand_flee_ranks_bothmax", SelectWpntLink, 1, &uvp_sel_flee_ranks_bothmax),
    C("SelectWpntLink_sweeppriority_flee_ranks_bothmax", SelectWpntLink, 2, &uvp_sel_flee_ranks_bothmax),
    C("SelectWpntLink_sweeprank_flee_ranks_bothmax", SelectWpntLink, 3, &uvp_sel_flee_ranks_bothmax),
    C("SelectWpntLink_flee_facing_close", SelectWpntLink, 0, &uvp_sel_flee_facing_close),
    C("SelectWpntLink_sweeprand_flee_facing_close", SelectWpntLink, 1, &uvp_sel_flee_facing_close),
    C("SelectWpntLink_sweeppriority_flee_facing_close", SelectWpntLink, 2, &uvp_sel_flee_facing_close),
    C("SelectWpntLink_sweeprank_flee_facing_close", SelectWpntLink, 3, &uvp_sel_flee_facing_close),
    C("SelectWpntLink_flee_facing_far", SelectWpntLink, 0, &uvp_sel_flee_facing_far),
    C("SelectWpntLink_sweeprand_flee_facing_far", SelectWpntLink, 1, &uvp_sel_flee_facing_far),
    C("SelectWpntLink_sweeppriority_flee_facing_far", SelectWpntLink, 2, &uvp_sel_flee_facing_far),
    C("SelectWpntLink_sweeprank_flee_facing_far", SelectWpntLink, 3, &uvp_sel_flee_facing_far),
    C("SelectWpntLink_flee_facing_mid", SelectWpntLink, 0, &uvp_sel_flee_facing_mid),
    C("SelectWpntLink_sweeprand_flee_facing_mid", SelectWpntLink, 1, &uvp_sel_flee_facing_mid),
    C("SelectWpntLink_sweeppriority_flee_facing_mid", SelectWpntLink, 2, &uvp_sel_flee_facing_mid),
    C("SelectWpntLink_sweeprank_flee_facing_mid", SelectWpntLink, 3, &uvp_sel_flee_facing_mid),
    C("SelectWpntLink_flee_facing_behind", SelectWpntLink, 0, &uvp_sel_flee_facing_behind),
    C("SelectWpntLink_sweeprand_flee_facing_behind", SelectWpntLink, 1, &uvp_sel_flee_facing_behind),
    C("SelectWpntLink_sweeppriority_flee_facing_behind", SelectWpntLink, 2, &uvp_sel_flee_facing_behind),
    C("SelectWpntLink_sweeprank_flee_facing_behind", SelectWpntLink, 3, &uvp_sel_flee_facing_behind),
    C("SelectWpntLink_flee_backtrack", SelectWpntLink, 0, &uvp_sel_flee_backtrack),
    C("SelectWpntLink_sweeprand_flee_backtrack", SelectWpntLink, 1, &uvp_sel_flee_backtrack),
    C("SelectWpntLink_sweeppriority_flee_backtrack", SelectWpntLink, 2, &uvp_sel_flee_backtrack),
    C("SelectWpntLink_sweeprank_flee_backtrack", SelectWpntLink, 3, &uvp_sel_flee_backtrack),
    C("SelectWpntLink_flee_backtrack_lead", SelectWpntLink, 0, &uvp_sel_flee_backtrack_lead),
    C("SelectWpntLink_sweeprand_flee_backtrack_lead", SelectWpntLink, 1, &uvp_sel_flee_backtrack_lead),
    C("SelectWpntLink_sweeppriority_flee_backtrack_lead", SelectWpntLink, 2, &uvp_sel_flee_backtrack_lead),
    C("SelectWpntLink_sweeprank_flee_backtrack_lead", SelectWpntLink, 3, &uvp_sel_flee_backtrack_lead),
    C("SelectWpntLink_flee_ranksteps", SelectWpntLink, 0, &uvp_sel_flee_ranksteps),
    C("SelectWpntLink_sweeprand_flee_ranksteps", SelectWpntLink, 1, &uvp_sel_flee_ranksteps),
    C("SelectWpntLink_sweeppriority_flee_ranksteps", SelectWpntLink, 2, &uvp_sel_flee_ranksteps),
    C("SelectWpntLink_sweeprank_flee_ranksteps", SelectWpntLink, 3, &uvp_sel_flee_ranksteps),
    C("SelectWpntLink_flee_playerranked", SelectWpntLink, 0, &uvp_sel_flee_playerranked),
    C("SelectWpntLink_sweeprand_flee_playerranked", SelectWpntLink, 1, &uvp_sel_flee_playerranked),
    C("SelectWpntLink_sweeppriority_flee_playerranked", SelectWpntLink, 2, &uvp_sel_flee_playerranked),
    C("SelectWpntLink_sweeprank_flee_playerranked", SelectWpntLink, 3, &uvp_sel_flee_playerranked),
    C("SelectWpntLink_flee_noflee", SelectWpntLink, 0, &uvp_sel_flee_noflee),
    C("SelectWpntLink_sweeprand_flee_noflee", SelectWpntLink, 1, &uvp_sel_flee_noflee),
    C("SelectWpntLink_sweeppriority_flee_noflee", SelectWpntLink, 2, &uvp_sel_flee_noflee),
    C("SelectWpntLink_sweeprank_flee_noflee", SelectWpntLink, 3, &uvp_sel_flee_noflee),
    C("SelectWpntLink_flee_noflee_attacking", SelectWpntLink, 0, &uvp_sel_flee_noflee_attacking),
    C("SelectWpntLink_sweeprand_flee_noflee_attacking", SelectWpntLink, 1, &uvp_sel_flee_noflee_attacking),
    C("SelectWpntLink_sweeppriority_flee_noflee_attacking", SelectWpntLink, 2, &uvp_sel_flee_noflee_attacking),
    C("SelectWpntLink_sweeprank_flee_noflee_attacking", SelectWpntLink, 3, &uvp_sel_flee_noflee_attacking),
    C("SelectWpntLink_fleewpnt_actorthing", SelectWpntLink, 0, &uvp_sel_fleewpnt_actorthing),
    C("SelectWpntLink_sweeprand_fleewpnt_actorthing", SelectWpntLink, 1, &uvp_sel_fleewpnt_actorthing),
    C("SelectWpntLink_sweeppriority_fleewpnt_actorthing", SelectWpntLink, 2, &uvp_sel_fleewpnt_actorthing),
    C("SelectWpntLink_sweeprank_fleewpnt_actorthing", SelectWpntLink, 3, &uvp_sel_fleewpnt_actorthing),
    C("SelectWpntLink_fleewpnt_ghost", SelectWpntLink, 0, &uvp_sel_fleewpnt_ghost),
    C("SelectWpntLink_sweeprand_fleewpnt_ghost", SelectWpntLink, 1, &uvp_sel_fleewpnt_ghost),
    C("SelectWpntLink_sweeppriority_fleewpnt_ghost", SelectWpntLink, 2, &uvp_sel_fleewpnt_ghost),
    C("SelectWpntLink_sweeprank_fleewpnt_ghost", SelectWpntLink, 3, &uvp_sel_fleewpnt_ghost),
    C("SelectWpntLink_fleewpnt_ghost_backtrack", SelectWpntLink, 0, &uvp_sel_fleewpnt_ghost_backtrack),
    C("SelectWpntLink_sweeprand_fleewpnt_ghost_backtrack", SelectWpntLink, 1, &uvp_sel_fleewpnt_ghost_backtrack),
    C("SelectWpntLink_sweeppriority_fleewpnt_ghost_backtrack", SelectWpntLink, 2, &uvp_sel_fleewpnt_ghost_backtrack),
    C("SelectWpntLink_sweeprank_fleewpnt_ghost_backtrack", SelectWpntLink, 3, &uvp_sel_fleewpnt_ghost_backtrack),
    C("SelectWpntLink_fleewpnt_ghost_backtrack_lead", SelectWpntLink, 0, &uvp_sel_fleewpnt_ghost_backtrack_lead),
    C("SelectWpntLink_sweeprand_fleewpnt_ghost_backtrack_lead", SelectWpntLink, 1, &uvp_sel_fleewpnt_ghost_backtrack_lead),
    C("SelectWpntLink_sweeppriority_fleewpnt_ghost_backtrack_lead", SelectWpntLink, 2, &uvp_sel_fleewpnt_ghost_backtrack_lead),
    C("SelectWpntLink_sweeprank_fleewpnt_ghost_backtrack_lead", SelectWpntLink, 3, &uvp_sel_fleewpnt_ghost_backtrack_lead),
    C("SelectWpntLink_fleewpnt_ghost_disabled", SelectWpntLink, 0, &uvp_sel_fleewpnt_ghost_disabled),
    C("SelectWpntLink_sweeprand_fleewpnt_ghost_disabled", SelectWpntLink, 1, &uvp_sel_fleewpnt_ghost_disabled),
    C("SelectWpntLink_sweeppriority_fleewpnt_ghost_disabled", SelectWpntLink, 2, &uvp_sel_fleewpnt_ghost_disabled),
    C("SelectWpntLink_sweeprank_fleewpnt_ghost_disabled", SelectWpntLink, 3, &uvp_sel_fleewpnt_ghost_disabled),
    C("SelectWpntLink_plain_manylinks", SelectWpntLink, 0, &uvp_sel_plain_manylinks),
    C("SelectWpntLink_sweeprand_plain_manylinks", SelectWpntLink, 1, &uvp_sel_plain_manylinks),
    C("SelectWpntLink_sweeppriority_plain_manylinks", SelectWpntLink, 2, &uvp_sel_plain_manylinks),
    C("SelectWpntLink_sweeprank_plain_manylinks", SelectWpntLink, 3, &uvp_sel_plain_manylinks),
    C("CheckPathToPoint_clear", CheckPathToPoint, 0, &uvp_pt_clear),
    C("CheckPathToPoint_clear_nothings", CheckPathToPoint, 0, &uvp_pt_clear_nothings),
    C("CheckPathToPoint_wall", CheckPathToPoint, 0, &uvp_pt_wall),
    C("CheckPathToPoint_floor_ignored", CheckPathToPoint, 0, &uvp_pt_floor_ignored),
    C("CheckPathToPoint_floor_blocks_skipfloor", CheckPathToPoint, 0, &uvp_pt_floor_blocks_skipfloor),
    C("CheckPathToPoint_floor_then_wall", CheckPathToPoint, 0, &uvp_pt_floor_then_wall),
    C("CheckPathToPoint_floor_then_wall_skipfloor", CheckPathToPoint, 0, &uvp_pt_floor_then_wall_skipfloor),
    C("CheckPathToPoint_adjoin_move_ignored", CheckPathToPoint, 0, &uvp_pt_adjoin_move_ignored),
    C("CheckPathToPoint_adjoin_move_then_wall", CheckPathToPoint, 0, &uvp_pt_adjoin_move_then_wall),
    C("CheckPathToPoint_adjoin_visible_blocks", CheckPathToPoint, 0, &uvp_pt_adjoin_visible_blocks),
    C("CheckPathToPoint_adjoin_cross_blocks", CheckPathToPoint, 0, &uvp_pt_adjoin_cross_blocks),
    C("CheckPathToPoint_adjoin_cross_move_ignored", CheckPathToPoint, 0, &uvp_pt_adjoin_cross_move_ignored),
    C("CheckPathToPoint_adjoin_move_floor_adjoin", CheckPathToPoint, 0, &uvp_pt_adjoin_move_floor_adjoin),
    C("CheckPathToPoint_thing", CheckPathToPoint, 0, &uvp_pt_thing),
    C("CheckPathToPoint_thing_nodetect", CheckPathToPoint, 0, &uvp_pt_thing_nodetect),
    C("CheckPathToPoint_onself", CheckPathToPoint, 0, &uvp_pt_onself),
    C("CheckPathToPoint_onself_wall", CheckPathToPoint, 0, &uvp_pt_onself_wall),
    C("CheckPathToPoint_diagonal_wall", CheckPathToPoint, 0, &uvp_pt_diagonal_wall),
    C("CheckPathToPoint_second_ignored_first_blocks", CheckPathToPoint, 0, &uvp_pt_second_ignored_first_blocks),
    C("CheckFloorAtPos_nothing", Floor, 0, &uvp_fl_nothing),
    C("CheckFloorAtPos_floor", Floor, 0, &uvp_fl_floor),
    C("CheckPosition_floor", Floor, 1, &uvp_fl_floor),
    C("CheckFloorAtPos_floor_nocoll_null", Floor, 0, &uvp_fl_floor_nocoll_null),
    C("CheckFloorAtPos_floor_nocollptr_off", Floor, 0, &uvp_fl_floor_nocollptr_off),
    C("CheckFloorAtPos_wall", Floor, 0, &uvp_fl_wall),
    C("CheckPosition_wall", Floor, 1, &uvp_fl_wall),
    C("CheckFloorAtPos_noaimove", Floor, 0, &uvp_fl_noaimove),
    C("CheckPosition_noaimove", Floor, 1, &uvp_fl_noaimove),
    C("CheckFloorAtPos_lava", Floor, 0, &uvp_fl_lava),
    C("CheckPosition_lava", Floor, 1, &uvp_fl_lava),
    C("CheckFloorAtPos_lava_allowed", Floor, 0, &uvp_fl_lava_allowed),
    C("CheckFloorAtPos_allowed_metal_ok", Floor, 0, &uvp_fl_allowed_metal_ok),
    C("CheckFloorAtPos_allowed_not_matching", Floor, 0, &uvp_fl_allowed_not_matching),
    C("CheckFloorAtPos_allowed_lava_not_listed", Floor, 0, &uvp_fl_allowed_lava_not_listed),
    C("CheckFloorAtPos_adjoin_noenter", Floor, 0, &uvp_fl_adjoin_noenter),
    C("CheckPosition_adjoin_noenter", Floor, 1, &uvp_fl_adjoin_noenter),
    C("CheckFloorAtPos_adjoin_noenter_flag", Floor, 0, &uvp_fl_adjoin_noenter_flag),
    C("CheckFloorAtPos_adjoin_water_destroyed", Floor, 0, &uvp_fl_adjoin_water_destroyed),
    C("CheckFloorAtPos_adjoin_water_not_destroyed", Floor, 0, &uvp_fl_adjoin_water_not_destroyed),
    C("CheckFloorAtPos_adjoin_water_already_in", Floor, 0, &uvp_fl_adjoin_water_already_in),
    C("CheckFloorAtPos_adjoin_plain_then_floor", Floor, 0, &uvp_fl_adjoin_plain_then_floor),
    C("CheckFloorAtPos_adjoin_world_noaimove", Floor, 0, &uvp_fl_adjoin_world_noaimove),
    C("CheckFloorAtPos_wall_then_floor", Floor, 0, &uvp_fl_wall_then_floor),
    C("CheckFloorAtPos_alignup_attached_same", Floor, 0, &uvp_fl_alignup_attached_same),
    C("CheckFloorAtPos_alignup_attached_other_steep", Floor, 0, &uvp_fl_alignup_attached_other_steep),
    C("CheckFloorAtPos_alignup_attached_other_flat", Floor, 0, &uvp_fl_alignup_attached_other_flat),
    C("CheckFloorAtPos_alignup_attached_other_edge", Floor, 0, &uvp_fl_alignup_attached_other_edge),
    C("CheckFloorAtPos_alignup_not_attached", Floor, 0, &uvp_fl_alignup_not_attached),
    C("CheckFloorAtPos_noalignup_attached_other", Floor, 0, &uvp_fl_noalignup_attached_other),
    C("CheckFloorAtPos_attached_same_nocoll", Floor, 0, &uvp_fl_attached_same_nocoll),
    C("CheckFloorAtPos_attached_flag_only", Floor, 0, &uvp_fl_attached_flag_only),
    C("CheckFloorAtPos_upside_down", Floor, 0, &uvp_fl_upside_down),
    C("CheckFloorAtPos_tilted_threshold", Floor, 0, &uvp_fl_tilted_threshold),
    C("CheckFloorAtPos_tilted_threshold_exact", Floor, 0, &uvp_fl_tilted_threshold_exact),
    C("CheckFloorAtPos_tilted_threshold_below", Floor, 0, &uvp_fl_tilted_threshold_below),
    C("CheckFloorAtPos_wallcrawl", Floor, 0, &uvp_fl_wallcrawl),
    C("CheckFloorAtPos_thing_attached_to_owner", Floor, 0, &uvp_fl_thing_attached_to_owner),
    C("CheckFloorAtPos_thing_attached_other", Floor, 0, &uvp_fl_thing_attached_other),
    C("CheckFloorAtPos_thing_tail", Floor, 0, &uvp_fl_thing_tail),
    C("CheckFloorAtPos_thing_standon", Floor, 0, &uvp_fl_thing_standon),
    C("CheckPosition_thing_standon", Floor, 1, &uvp_fl_thing_standon),
    C("CheckFloorAtPos_thing_blocks_item", Floor, 0, &uvp_fl_thing_blocks_item),
    C("CheckFloorAtPos_thing_type_player_bits", Floor, 0, &uvp_fl_thing_type_player_bits),
    C("CheckFloorAtPos_thing_type_actor_bit", Floor, 0, &uvp_fl_thing_type_actor_bit),
    C("CheckFloorAtPos_thing_type_corpse_bits", Floor, 0, &uvp_fl_thing_type_corpse_bits),
    C("CheckFloorAtPos_thing_type_ghost_bits", Floor, 0, &uvp_fl_thing_type_ghost_bits),
    C("CheckFloorAtPos_thing_type_weapon_bits", Floor, 0, &uvp_fl_thing_type_weapon_bits),
    C("CheckFloorAtPos_thing_type_camera_bits", Floor, 0, &uvp_fl_thing_type_camera_bits),
    C("CheckFloorAtPos_thing_type_hint_bits", Floor, 0, &uvp_fl_thing_type_hint_bits),
    C("CheckFloorAtPos_thing_facing_attached", Floor, 0, &uvp_fl_thing_facing_attached),
    C("CheckFloorAtPos_thing_facing_other", Floor, 0, &uvp_fl_thing_facing_other),
    C("CheckFloorAtPos_thing_nocollptr", Floor, 0, &uvp_fl_thing_nocollptr),
    C("CheckFloorAtPos_thing_then_floor", Floor, 0, &uvp_fl_thing_then_floor),
    C("CheckFloorAtPos_thing_ok_then_wall", Floor, 0, &uvp_fl_thing_ok_then_wall),
    C("CheckFloorAtPos_unknown_collision_type", Floor, 0, &uvp_fl_unknown_collision_type),
    C("CheckFloorAtPos_maxstep_large", Floor, 0, &uvp_fl_maxstep_large),
    C("CheckPosition_samesector", Floor, 1, &uvp_fl_pos_samesector),
    C("CheckPosition_nosector", Floor, 1, &uvp_fl_pos_nosector),
    C("CheckPosition_noenter", Floor, 1, &uvp_fl_pos_noenter),
    C("CheckPosition_water_destroyed", Floor, 1, &uvp_fl_pos_water_destroyed),
    C("CheckPosition_water_not_destroyed", Floor, 1, &uvp_fl_pos_water_not_destroyed),
    C("CheckPosition_water_from_water", Floor, 1, &uvp_fl_pos_water_from_water),
    C("CheckPosition_ok_floor", Floor, 1, &uvp_fl_pos_ok_floor),
    C("CheckPosition_ok_cliff", Floor, 1, &uvp_fl_pos_ok_cliff),
    C("CheckPosition_ok_wall", Floor, 1, &uvp_fl_pos_ok_wall),
    C("AIFindNearestWpnt_nowpnts", FindNearest, 0, &uvp_nr_nowpnts),
    C("AIFindNearestWpnt_nowpnts_player", FindNearest, 0, &uvp_nr_nowpnts_player),
    C("AIFindNearestWpnt_onwaypoint", FindNearest, 0, &uvp_nr_onwaypoint),
    C("AIFindNearestWpnt_nearwaypoint_edge", FindNearest, 0, &uvp_nr_nearwaypoint_edge),
    C("AIFindNearestWpnt_nearwaypoint_outside", FindNearest, 0, &uvp_nr_nearwaypoint_outside),
    C("AIFindNearestWpnt_nearwaypoint_bigsize", FindNearest, 0, &uvp_nr_nearwaypoint_bigsize),
    C("AIFindNearestWpnt_owner_nocheck", FindNearest, 0, &uvp_nr_owner_nocheck),
    C("AIFindNearestWpnt_owner_nocheck_hunting", FindNearest, 0, &uvp_nr_owner_nocheck_hunting),
    C("AIFindNearestWpnt_owner_check_clear", FindNearest, 0, &uvp_nr_owner_check_clear),
    C("AIFindNearestWpnt_owner_check_firstblocked", FindNearest, 0, &uvp_nr_owner_check_firstblocked),
    C("AIFindNearestWpnt_owner_check_firstsecondblocked", FindNearest, 0, &uvp_nr_owner_check_firstsecondblocked),
    C("AIFindNearestWpnt_owner_check_floorignored", FindNearest, 0, &uvp_nr_owner_check_floorignored),
    C("AIFindNearestWpnt_owner_check_allblocked", FindNearest, 0, &uvp_nr_owner_check_allblocked),
    C("AIFindNearestWpnt_owner_check_linkblocked", FindNearest, 0, &uvp_nr_owner_check_linkblocked),
    C("AIFindNearestWpnt_owner_check_lastblocked", FindNearest, 0, &uvp_nr_owner_check_lastblocked),
    C("AIFindNearestWpnt_player_nocheck", FindNearest, 0, &uvp_nr_player_nocheck),
    C("AIFindNearestWpnt_player_check_clear", FindNearest, 0, &uvp_nr_player_check_clear),
    C("AIFindNearestWpnt_player_check_firstblocked", FindNearest, 0, &uvp_nr_player_check_firstblocked),
    C("AIFindNearestWpnt_player_check_allblocked", FindNearest, 0, &uvp_nr_player_check_allblocked),
    C("AIFindNearestWpnt_disabled_wpnt", FindNearest, 0, &uvp_nr_disabled_wpnt),
    C("AIFindNearestWpnt_disabled_wpnt_player", FindNearest, 0, &uvp_nr_disabled_wpnt_player),
    C("AIFindNearestWpnt_otherlayer_wpnt", FindNearest, 0, &uvp_nr_otherlayer_wpnt),
    C("AIFindNearestWpnt_otherlayer_active", FindNearest, 0, &uvp_nr_otherlayer_active),
    C("AIFindNearestWpnt_otherlayer_all", FindNearest, 0, &uvp_nr_otherlayer_all),
    C("AIFindNearestWpnt_invalid_thing_45", FindNearest, 0, &uvp_nr_invalid_thing_45),
    C("AIFindNearestWpnt_invalid_nonghost", FindNearest, 0, &uvp_nr_invalid_nonghost),
    C("AIFindNearestWpnt_invalid_nothing", FindNearest, 0, &uvp_nr_invalid_nothing),
    C("AIFindNearestWpnt_invalid_all3", FindNearest, 0, &uvp_nr_invalid_all3),
    C("AIFindNearestWpnt_star_hub_owner", FindNearest, 0, &uvp_nr_star_hub_owner),
    C("AIFindNearestWpnt_star_hub_owner_blocked", FindNearest, 0, &uvp_nr_star_hub_owner_blocked),
    C("AIFindNearestWpnt_star_leaf_owner", FindNearest, 0, &uvp_nr_star_leaf_owner),
    C("AIFindNearestWpnt_star_leaf_owner_check", FindNearest, 0, &uvp_nr_star_leaf_owner_check),
    C("AIFindNearestWpnt_ring_owner", FindNearest, 0, &uvp_nr_ring_owner),
    C("AIFindNearestWpnt_ring_owner_blocked_first", FindNearest, 0, &uvp_nr_ring_owner_blocked_first),
    C("AIFindNearestWpnt_ring_owner_attacking", FindNearest, 0, &uvp_nr_ring_owner_attacking),
    C("AIFindNearestWpnt_oneway_owner", FindNearest, 0, &uvp_nr_oneway_owner),
    C("AIFindNearestWpnt_split_owner", FindNearest, 0, &uvp_nr_split_owner),
    C("AIFindNearestWpnt_highrank_owner", FindNearest, 0, &uvp_nr_highrank_owner),
    C("AIMoveToNextWpnt_nowpnts", WpntMove, 0, &uvp_wm_next_nowpnts),
    C("AIMoveToNextWpnt_basic", WpntMove, 0, &uvp_wm_next_basic),
    C("AIMoveToNextWpnt_basic_movefails", WpntMove, 0, &uvp_wm_next_basic_movefails),
    C("AIMoveToNextWpnt_moveforward", WpntMove, 0, &uvp_wm_next_moveforward),
    C("AIMoveToNextWpnt_mode1", WpntMove, 0, &uvp_wm_next_mode1),
    C("AIMoveToNextWpnt_mode2", WpntMove, 0, &uvp_wm_next_mode2),
    C("AIMoveToNextWpnt_modeneg", WpntMove, 0, &uvp_wm_next_modeneg),
    C("AIMoveToNextWpnt_ownerfull", WpntMove, 0, &uvp_wm_next_ownerfull),
    C("AIMoveToNextWpnt_blocked", WpntMove, 0, &uvp_wm_next_blocked),
    C("AIMoveToNextWpnt_stationary", WpntMove, 0, &uvp_wm_next_stationary),
    C("AITraverseWpnts_basic", WpntMove, 1, &uvp_wm_trav_basic),
    C("AITraverseWpnts_basic_movefails", WpntMove, 1, &uvp_wm_trav_basic_movefails),
    C("AITraverseWpnts_movefails_star", WpntMove, 1, &uvp_wm_trav_movefails_star),
    C("AITraverseWpnts_idx60", WpntMove, 1, &uvp_wm_trav_idx60),
    C("AITraverseWpnts_idxneg", WpntMove, 1, &uvp_wm_trav_idxneg),
    C("AITraverseWpnts_noworld", WpntMove, 1, &uvp_wm_trav_noworld),
    C("AITraverseWpnts_thing45", WpntMove, 1, &uvp_wm_trav_thing45),
    C("AITraverseWpnts_nonghost", WpntMove, 1, &uvp_wm_trav_nonghost),
    C("AITraverseWpnts_deg0_class30", WpntMove, 1, &uvp_wm_trav_deg0_class30),
    C("AITraverseWpnts_deg0_class0", WpntMove, 1, &uvp_wm_trav_deg0_class0),
    C("AITraverseWpnts_deg3", WpntMove, 1, &uvp_wm_trav_deg3),
    C("AITraverseWpnts_deg400", WpntMove, 1, &uvp_wm_trav_deg400),
    C("AITraverseWpnts_mode1", WpntMove, 1, &uvp_wm_trav_mode1),
    C("AITraverseWpnts_mode2", WpntMove, 1, &uvp_wm_trav_mode2),
    C("AITraverseWpnts_mode100", WpntMove, 1, &uvp_wm_trav_mode100),
    C("AITraverseWpnts_modeneg", WpntMove, 1, &uvp_wm_trav_modeneg),
    C("AITraverseWpnts_owneralready", WpntMove, 1, &uvp_wm_trav_owneralready),
    C("AITraverseWpnts_ownerfull", WpntMove, 1, &uvp_wm_trav_ownerfull),
    C("AITraverseWpnts_ownerfull_withown", WpntMove, 1, &uvp_wm_trav_ownerfull_withown),
    C("AITraverseWpnts_wallcrawl_aligned", WpntMove, 1, &uvp_wm_trav_wallcrawl_aligned),
    C("AITraverseWpnts_turning", WpntMove, 1, &uvp_wm_trav_turning),
    C("AIAdvanceToNextWpnt_notregistered", WpntMove, 2, &uvp_wm_adv_notregistered),
    C("AIAdvanceToNextWpnt_notregistered_notify", WpntMove, 2, &uvp_wm_adv_notregistered_notify),
    C("AIAdvanceToNextWpnt_midchain", WpntMove, 2, &uvp_wm_adv_midchain),
    C("AIAdvanceToNextWpnt_midchain_notify", WpntMove, 2, &uvp_wm_adv_midchain_notify),
    C("AIAdvanceToNextWpnt_midchain_movefails", WpntMove, 2, &uvp_wm_adv_midchain_movefails),
    C("AIAdvanceToNextWpnt_midchain_prev3", WpntMove, 2, &uvp_wm_adv_midchain_prev3),
    C("AIAdvanceToNextWpnt_chainend_single_back", WpntMove, 2, &uvp_wm_adv_chainend_single_back),
    C("AIAdvanceToNextWpnt_chainend_single_back_cont", WpntMove, 2, &uvp_wm_adv_chainend_single_back_cont),
    C("AIAdvanceToNextWpnt_chainend_single_back_semi", WpntMove, 2, &uvp_wm_adv_chainend_single_back_semi),
    C("AIAdvanceToNextWpnt_chainend_single_other", WpntMove, 2, &uvp_wm_adv_chainend_single_other),
    C("AIAdvanceToNextWpnt_nolinks", WpntMove, 2, &uvp_wm_adv_nolinks),
    C("AIAdvanceToNextWpnt_next_outofrange", WpntMove, 2, &uvp_wm_adv_next_outofrange),
    C("AIAdvanceToNextWpnt_next_255", WpntMove, 2, &uvp_wm_adv_next_255),
    C("AIAdvanceToNextWpnt_next_unused_wpnt", WpntMove, 2, &uvp_wm_adv_next_unused_wpnt),
    C("AIAdvanceToNextWpnt_star_hub", WpntMove, 2, &uvp_wm_adv_star_hub),
    C("AIAdvanceToNextWpnt_star_hub_hunting", WpntMove, 2, &uvp_wm_adv_star_hub_hunting),
    C("AIAdvanceToNextWpnt_star_hub_nobacktrack", WpntMove, 2, &uvp_wm_adv_star_hub_nobacktrack),
    C("AIAdvanceToNextWpnt_ring", WpntMove, 2, &uvp_wm_adv_ring),
    C("AIAdvanceToNextWpnt_disabled_link", WpntMove, 2, &uvp_wm_adv_disabled_link),
    C("AIAdvanceToNextWpnt_otherlayer_link", WpntMove, 2, &uvp_wm_adv_otherlayer_link),
    C("AIAdvanceToNextWpnt_otherlayer_active", WpntMove, 2, &uvp_wm_adv_otherlayer_active),
    C("AIAdvanceToNextWpnt_otherlayer_two", WpntMove, 2, &uvp_wm_adv_otherlayer_two),
    C("AIAdvanceToNextWpnt_hunting_mode", WpntMove, 2, &uvp_wm_adv_hunting_mode),
    C("AIAdvanceToNextWpnt_fleewpnt_mode", WpntMove, 2, &uvp_wm_adv_fleewpnt_mode),
    C("AIAdvanceToNextWpnt_continuous_cleared_on_arrive", WpntMove, 2, &uvp_wm_adv_continuous_cleared_on_arrive),
    C("CheckWpntMoveAlign_notreg", CheckWpntMoveAlign, 0, &uvp_al_notreg),
    C("CheckWpntMoveAlign_aligned", CheckWpntMoveAlign, 0, &uvp_al_aligned),
    C("CheckWpntMoveAlign_aligned_notturning", CheckWpntMoveAlign, 0, &uvp_al_aligned_notturning),
    C("CheckWpntMoveAlign_turning_off", CheckWpntMoveAlign, 0, &uvp_al_turning_off),
    C("CheckWpntMoveAlign_turning_edge", CheckWpntMoveAlign, 0, &uvp_al_turning_edge),
    C("CheckWpntMoveAlign_turning_justoff", CheckWpntMoveAlign, 0, &uvp_al_turning_justoff),
    C("CheckWpntMoveAlign_cos_negative", CheckWpntMoveAlign, 0, &uvp_al_cos_negative),
    C("CheckWpntMoveAlign_cos_negative_ok", CheckWpntMoveAlign, 0, &uvp_al_cos_negative_ok),
    C("CheckWpntMoveAlign_wallcrawl", CheckWpntMoveAlign, 0, &uvp_al_wallcrawl),
    C("CheckWpntMoveAlign_wallcrawl_aligned", CheckWpntMoveAlign, 0, &uvp_al_wallcrawl_aligned),
    C("CheckWpntMoveAlign_wallcrawl_notturning", CheckWpntMoveAlign, 0, &uvp_al_wallcrawl_notturning),
    C("CheckWpntMoveAlign_flat_ignores_z", CheckWpntMoveAlign, 0, &uvp_al_flat_ignores_z),
    C("CheckWpntMoveAlign_tilted_up", CheckWpntMoveAlign, 0, &uvp_al_tilted_up),
    C("CheckWpntMoveAlign_zerogoal", CheckWpntMoveAlign, 0, &uvp_al_zerogoal),
    C("RenderAIWaypoints_off", RenderWaypoints, 0, &uvp_rw_off),
    C("RenderAIWaypoints_early", RenderWaypoints, 0, &uvp_rw_early),
    C("RenderAIWaypoints_interval", RenderWaypoints, 0, &uvp_rw_interval),
    C("RenderAIWaypoints_late", RenderWaypoints, 0, &uvp_rw_late),
    C("RenderAIWaypoints_nowpnts", RenderWaypoints, 0, &uvp_rw_nowpnts),
    C("RenderAIWaypoints_disabled", RenderWaypoints, 0, &uvp_rw_disabled),
    C("RenderAIWaypoints_otherlayer", RenderWaypoints, 0, &uvp_rw_otherlayer),
    C("RenderAIWaypoints_otherlayer_active", RenderWaypoints, 0, &uvp_rw_otherlayer_active),
    C("RenderAIWaypoints_star", RenderWaypoints, 0, &uvp_rw_star),
    C("RenderAIWaypoints_ring", RenderWaypoints, 0, &uvp_rw_ring),
    C("RenderAIWaypoints_oneway", RenderWaypoints, 0, &uvp_rw_oneway),
    C("RenderAIWaypoints_split", RenderWaypoints, 0, &uvp_rw_split),
    C("RenderAIWaypoints_timeearly", RenderWaypoints, 0, &uvp_rw_timeearly),
    C("GetDistanceToTarget_visible", Distance, 0, &uvp_ds_visible),
    C("GetDistanceToTargetPos_visible", Distance, 1, &uvp_ds_visible),
    C("GetDistanceToTarget_sweepangle_visible", Distance, 2, &uvp_ds_visible),
    C("GetDistanceToTargetPos_sweepangle_visible", Distance, 3, &uvp_ds_visible),
    C("GetDistanceToTarget_sweepdistance_visible", Distance, 4, &uvp_ds_visible),
    C("GetDistanceToTargetPos_sweepdistance_visible", Distance, 5, &uvp_ds_visible),
    C("GetDistanceToTarget_visible_nolos", Distance, 0, &uvp_ds_visible_nolos),
    C("GetDistanceToTargetPos_visible_nolos", Distance, 1, &uvp_ds_visible_nolos),
    C("GetDistanceToTarget_sweepangle_visible_nolos", Distance, 2, &uvp_ds_visible_nolos),
    C("GetDistanceToTargetPos_sweepangle_visible_nolos", Distance, 3, &uvp_ds_visible_nolos),
    C("GetDistanceToTarget_sweepdistance_visible_nolos", Distance, 4, &uvp_ds_visible_nolos),
    C("GetDistanceToTargetPos_sweepdistance_visible_nolos", Distance, 5, &uvp_ds_visible_nolos),
    C("GetDistanceToTarget_visible_startoffset", Distance, 0, &uvp_ds_visible_startoffset),
    C("GetDistanceToTargetPos_visible_startoffset", Distance, 1, &uvp_ds_visible_startoffset),
    C("GetDistanceToTarget_sweepangle_visible_startoffset", Distance, 2, &uvp_ds_visible_startoffset),
    C("GetDistanceToTargetPos_sweepangle_visible_startoffset", Distance, 3, &uvp_ds_visible_startoffset),
    C("GetDistanceToTarget_sweepdistance_visible_startoffset", Distance, 4, &uvp_ds_visible_startoffset),
    C("GetDistanceToTargetPos_sweepdistance_visible_startoffset", Distance, 5, &uvp_ds_visible_startoffset),
    C("GetDistanceToTarget_toofar", Distance, 0, &uvp_ds_toofar),
    C("GetDistanceToTargetPos_toofar", Distance, 1, &uvp_ds_toofar),
    C("GetDistanceToTarget_sweepangle_toofar", Distance, 2, &uvp_ds_toofar),
    C("GetDistanceToTargetPos_sweepangle_toofar", Distance, 3, &uvp_ds_toofar),
    C("GetDistanceToTarget_sweepdistance_toofar", Distance, 4, &uvp_ds_toofar),
    C("GetDistanceToTargetPos_sweepdistance_toofar", Distance, 5, &uvp_ds_toofar),
    C("GetDistanceToTarget_toofar_edge", Distance, 0, &uvp_ds_toofar_edge),
    C("GetDistanceToTargetPos_toofar_edge", Distance, 1, &uvp_ds_toofar_edge),
    C("GetDistanceToTarget_sweepangle_toofar_edge", Distance, 2, &uvp_ds_toofar_edge),
    C("GetDistanceToTargetPos_sweepangle_toofar_edge", Distance, 3, &uvp_ds_toofar_edge),
    C("GetDistanceToTarget_sweepdistance_toofar_edge", Distance, 4, &uvp_ds_toofar_edge),
    C("GetDistanceToTargetPos_sweepdistance_toofar_edge", Distance, 5, &uvp_ds_toofar_edge),
    C("GetDistanceToTarget_toofar_edge_inside", Distance, 0, &uvp_ds_toofar_edge_inside),
    C("GetDistanceToTargetPos_toofar_edge_inside", Distance, 1, &uvp_ds_toofar_edge_inside),
    C("GetDistanceToTarget_sweepangle_toofar_edge_inside", Distance, 2, &uvp_ds_toofar_edge_inside),
    C("GetDistanceToTargetPos_sweepangle_toofar_edge_inside", Distance, 3, &uvp_ds_toofar_edge_inside),
    C("GetDistanceToTarget_sweepdistance_toofar_edge_inside", Distance, 4, &uvp_ds_toofar_edge_inside),
    C("GetDistanceToTargetPos_sweepdistance_toofar_edge_inside", Distance, 5, &uvp_ds_toofar_edge_inside),
    C("GetDistanceToTarget_toofar_size0", Distance, 0, &uvp_ds_toofar_size0),
    C("GetDistanceToTargetPos_toofar_size0", Distance, 1, &uvp_ds_toofar_size0),
    C("GetDistanceToTarget_sweepangle_toofar_size0", Distance, 2, &uvp_ds_toofar_size0),
    C("GetDistanceToTargetPos_sweepangle_toofar_size0", Distance, 3, &uvp_ds_toofar_size0),
    C("GetDistanceToTarget_sweepdistance_toofar_size0", Distance, 4, &uvp_ds_toofar_size0),
    C("GetDistanceToTargetPos_sweepdistance_toofar_size0", Distance, 5, &uvp_ds_toofar_size0),
    C("GetDistanceToTarget_sizebigger_than_distance", Distance, 0, &uvp_ds_sizebigger_than_distance),
    C("GetDistanceToTargetPos_sizebigger_than_distance", Distance, 1, &uvp_ds_sizebigger_than_distance),
    C("GetDistanceToTarget_sweepangle_sizebigger_than_distance", Distance, 2, &uvp_ds_sizebigger_than_distance),
    C("GetDistanceToTargetPos_sweepangle_sizebigger_than_distance", Distance, 3, &uvp_ds_sizebigger_than_distance),
    C("GetDistanceToTarget_sweepdistance_sizebigger_than_distance", Distance, 4, &uvp_ds_sizebigger_than_distance),
    C("GetDistanceToTargetPos_sweepdistance_sizebigger_than_distance", Distance, 5, &uvp_ds_sizebigger_than_distance),
    C("GetDistanceToTarget_ontop", Distance, 0, &uvp_ds_ontop),
    C("GetDistanceToTargetPos_ontop", Distance, 1, &uvp_ds_ontop),
    C("GetDistanceToTarget_sweepangle_ontop", Distance, 2, &uvp_ds_ontop),
    C("GetDistanceToTargetPos_sweepangle_ontop", Distance, 3, &uvp_ds_ontop),
    C("GetDistanceToTarget_sweepdistance_ontop", Distance, 4, &uvp_ds_ontop),
    C("GetDistanceToTargetPos_sweepdistance_ontop", Distance, 5, &uvp_ds_ontop),
    C("GetDistanceToTarget_invisible_target", Distance, 0, &uvp_ds_invisible_target),
    C("GetDistanceToTargetPos_invisible_target", Distance, 1, &uvp_ds_invisible_target),
    C("GetDistanceToTarget_sweepangle_invisible_target", Distance, 2, &uvp_ds_invisible_target),
    C("GetDistanceToTargetPos_sweepangle_invisible_target", Distance, 3, &uvp_ds_invisible_target),
    C("GetDistanceToTarget_sweepdistance_invisible_target", Distance, 4, &uvp_ds_invisible_target),
    C("GetDistanceToTargetPos_sweepdistance_invisible_target", Distance, 5, &uvp_ds_invisible_target),
    C("GetDistanceToTarget_invisible_target_player", Distance, 0, &uvp_ds_invisible_target_player),
    C("GetDistanceToTargetPos_invisible_target_player", Distance, 1, &uvp_ds_invisible_target_player),
    C("GetDistanceToTarget_sweepangle_invisible_target_player", Distance, 2, &uvp_ds_invisible_target_player),
    C("GetDistanceToTargetPos_sweepangle_invisible_target_player", Distance, 3, &uvp_ds_invisible_target_player),
    C("GetDistanceToTarget_sweepdistance_invisible_target_player", Distance, 4, &uvp_ds_invisible_target_player),
    C("GetDistanceToTargetPos_sweepdistance_invisible_target_player", Distance, 5, &uvp_ds_invisible_target_player),
    C("GetDistanceToTarget_invisible_target_other_type", Distance, 0, &uvp_ds_invisible_target_other_type),
    C("GetDistanceToTargetPos_invisible_target_other_type", Distance, 1, &uvp_ds_invisible_target_other_type),
    C("GetDistanceToTarget_sweepangle_invisible_target_other_type", Distance, 2, &uvp_ds_invisible_target_other_type),
    C("GetDistanceToTargetPos_sweepangle_invisible_target_other_type", Distance, 3, &uvp_ds_invisible_target_other_type),
    C("GetDistanceToTarget_sweepdistance_invisible_target_other_type", Distance, 4, &uvp_ds_invisible_target_other_type),
    C("GetDistanceToTargetPos_sweepdistance_invisible_target_other_type", Distance, 5, &uvp_ds_invisible_target_other_type),
    C("GetDistanceToTarget_invisible_seeinvisible", Distance, 0, &uvp_ds_invisible_seeinvisible),
    C("GetDistanceToTargetPos_invisible_seeinvisible", Distance, 1, &uvp_ds_invisible_seeinvisible),
    C("GetDistanceToTarget_sweepangle_invisible_seeinvisible", Distance, 2, &uvp_ds_invisible_seeinvisible),
    C("GetDistanceToTargetPos_sweepangle_invisible_seeinvisible", Distance, 3, &uvp_ds_invisible_seeinvisible),
    C("GetDistanceToTarget_sweepdistance_invisible_seeinvisible", Distance, 4, &uvp_ds_invisible_seeinvisible),
    C("GetDistanceToTargetPos_sweepdistance_invisible_seeinvisible", Distance, 5, &uvp_ds_invisible_seeinvisible),
    C("GetDistanceToTarget_blind_viewer", Distance, 0, &uvp_ds_blind_viewer),
    C("GetDistanceToTargetPos_blind_viewer", Distance, 1, &uvp_ds_blind_viewer),
    C("GetDistanceToTarget_sweepangle_blind_viewer", Distance, 2, &uvp_ds_blind_viewer),
    C("GetDistanceToTargetPos_sweepangle_blind_viewer", Distance, 3, &uvp_ds_blind_viewer),
    C("GetDistanceToTarget_sweepdistance_blind_viewer", Distance, 4, &uvp_ds_blind_viewer),
    C("GetDistanceToTargetPos_sweepdistance_blind_viewer", Distance, 5, &uvp_ds_blind_viewer),
    C("GetDistanceToTarget_blind_seeinvisible", Distance, 0, &uvp_ds_blind_seeinvisible),
    C("GetDistanceToTargetPos_blind_seeinvisible", Distance, 1, &uvp_ds_blind_seeinvisible),
    C("GetDistanceToTarget_sweepangle_blind_seeinvisible", Distance, 2, &uvp_ds_blind_seeinvisible),
    C("GetDistanceToTargetPos_sweepangle_blind_seeinvisible", Distance, 3, &uvp_ds_blind_seeinvisible),
    C("GetDistanceToTarget_sweepdistance_blind_seeinvisible", Distance, 4, &uvp_ds_blind_seeinvisible),
    C("GetDistanceToTargetPos_sweepdistance_blind_seeinvisible", Distance, 5, &uvp_ds_blind_seeinvisible),
    C("GetDistanceToTarget_seeinvisible_toofar", Distance, 0, &uvp_ds_seeinvisible_toofar),
    C("GetDistanceToTargetPos_seeinvisible_toofar", Distance, 1, &uvp_ds_seeinvisible_toofar),
    C("GetDistanceToTarget_sweepangle_seeinvisible_toofar", Distance, 2, &uvp_ds_seeinvisible_toofar),
    C("GetDistanceToTargetPos_sweepangle_seeinvisible_toofar", Distance, 3, &uvp_ds_seeinvisible_toofar),
    C("GetDistanceToTarget_sweepdistance_seeinvisible_toofar", Distance, 4, &uvp_ds_seeinvisible_toofar),
    C("GetDistanceToTargetPos_sweepdistance_seeinvisible_toofar", Distance, 5, &uvp_ds_seeinvisible_toofar),
    C("GetDistanceToTarget_seeinvisible_outoffov", Distance, 0, &uvp_ds_seeinvisible_outoffov),
    C("GetDistanceToTargetPos_seeinvisible_outoffov", Distance, 1, &uvp_ds_seeinvisible_outoffov),
    C("GetDistanceToTarget_sweepangle_seeinvisible_outoffov", Distance, 2, &uvp_ds_seeinvisible_outoffov),
    C("GetDistanceToTargetPos_sweepangle_seeinvisible_outoffov", Distance, 3, &uvp_ds_seeinvisible_outoffov),
    C("GetDistanceToTarget_sweepdistance_seeinvisible_outoffov", Distance, 4, &uvp_ds_seeinvisible_outoffov),
    C("GetDistanceToTargetPos_sweepdistance_seeinvisible_outoffov", Distance, 5, &uvp_ds_seeinvisible_outoffov),
    C("GetDistanceToTarget_target_submerged_viewer_dry", Distance, 0, &uvp_ds_target_submerged_viewer_dry),
    C("GetDistanceToTargetPos_target_submerged_viewer_dry", Distance, 1, &uvp_ds_target_submerged_viewer_dry),
    C("GetDistanceToTarget_sweepangle_target_submerged_viewer_dry", Distance, 2, &uvp_ds_target_submerged_viewer_dry),
    C("GetDistanceToTargetPos_sweepangle_target_submerged_viewer_dry", Distance, 3, &uvp_ds_target_submerged_viewer_dry),
    C("GetDistanceToTarget_sweepdistance_target_submerged_viewer_dry", Distance, 4, &uvp_ds_target_submerged_viewer_dry),
    C("GetDistanceToTargetPos_sweepdistance_target_submerged_viewer_dry", Distance, 5, &uvp_ds_target_submerged_viewer_dry),
    C("GetDistanceToTarget_target_submerged_onsurface", Distance, 0, &uvp_ds_target_submerged_onsurface),
    C("GetDistanceToTargetPos_target_submerged_onsurface", Distance, 1, &uvp_ds_target_submerged_onsurface),
    C("GetDistanceToTarget_sweepangle_target_submerged_onsurface", Distance, 2, &uvp_ds_target_submerged_onsurface),
    C("GetDistanceToTargetPos_sweepangle_target_submerged_onsurface", Distance, 3, &uvp_ds_target_submerged_onsurface),
    C("GetDistanceToTarget_sweepdistance_target_submerged_onsurface", Distance, 4, &uvp_ds_target_submerged_onsurface),
    C("GetDistanceToTargetPos_sweepdistance_target_submerged_onsurface", Distance, 5, &uvp_ds_target_submerged_onsurface),
    C("GetDistanceToTarget_target_submerged_nonphysics", Distance, 0, &uvp_ds_target_submerged_nonphysics),
    C("GetDistanceToTargetPos_target_submerged_nonphysics", Distance, 1, &uvp_ds_target_submerged_nonphysics),
    C("GetDistanceToTarget_sweepangle_target_submerged_nonphysics", Distance, 2, &uvp_ds_target_submerged_nonphysics),
    C("GetDistanceToTargetPos_sweepangle_target_submerged_nonphysics", Distance, 3, &uvp_ds_target_submerged_nonphysics),
    C("GetDistanceToTarget_sweepdistance_target_submerged_nonphysics", Distance, 4, &uvp_ds_target_submerged_nonphysics),
    C("GetDistanceToTargetPos_sweepdistance_target_submerged_nonphysics", Distance, 5, &uvp_ds_target_submerged_nonphysics),
    C("GetDistanceToTarget_target_submerged_both", Distance, 0, &uvp_ds_target_submerged_both),
    C("GetDistanceToTargetPos_target_submerged_both", Distance, 1, &uvp_ds_target_submerged_both),
    C("GetDistanceToTarget_sweepangle_target_submerged_both", Distance, 2, &uvp_ds_target_submerged_both),
    C("GetDistanceToTargetPos_sweepangle_target_submerged_both", Distance, 3, &uvp_ds_target_submerged_both),
    C("GetDistanceToTarget_sweepdistance_target_submerged_both", Distance, 4, &uvp_ds_target_submerged_both),
    C("GetDistanceToTargetPos_sweepdistance_target_submerged_both", Distance, 5, &uvp_ds_target_submerged_both),
    C("GetDistanceToTarget_viewer_submerged_target_dry", Distance, 0, &uvp_ds_viewer_submerged_target_dry),
    C("GetDistanceToTargetPos_viewer_submerged_target_dry", Distance, 1, &uvp_ds_viewer_submerged_target_dry),
    C("GetDistanceToTarget_sweepangle_viewer_submerged_target_dry", Distance, 2, &uvp_ds_viewer_submerged_target_dry),
    C("GetDistanceToTargetPos_sweepangle_viewer_submerged_target_dry", Distance, 3, &uvp_ds_viewer_submerged_target_dry),
    C("GetDistanceToTarget_sweepdistance_viewer_submerged_target_dry", Distance, 4, &uvp_ds_viewer_submerged_target_dry),
    C("GetDistanceToTargetPos_sweepdistance_viewer_submerged_target_dry", Distance, 5, &uvp_ds_viewer_submerged_target_dry),
    C("GetDistanceToTarget_viewer_submerged_target_dry_seeinvisible", Distance, 0, &uvp_ds_viewer_submerged_target_dry_seeinvisible),
    C("GetDistanceToTargetPos_viewer_submerged_target_dry_seeinvisible", Distance, 1, &uvp_ds_viewer_submerged_target_dry_seeinvisible),
    C("GetDistanceToTarget_sweepangle_viewer_submerged_target_dry_seeinvisible", Distance, 2, &uvp_ds_viewer_submerged_target_dry_seeinvisible),
    C("GetDistanceToTargetPos_sweepangle_viewer_submerged_target_dry_seeinvisible", Distance, 3, &uvp_ds_viewer_submerged_target_dry_seeinvisible),
    C("GetDistanceToTarget_sweepdistance_viewer_submerged_target_dry_seeinvisible", Distance, 4, &uvp_ds_viewer_submerged_target_dry_seeinvisible),
    C("GetDistanceToTargetPos_sweepdistance_viewer_submerged_target_dry_seeinvisible", Distance, 5, &uvp_ds_viewer_submerged_target_dry_seeinvisible),
    C("GetDistanceToTarget_fov_front", Distance, 0, &uvp_ds_fov_front),
    C("GetDistanceToTargetPos_fov_front", Distance, 1, &uvp_ds_fov_front),
    C("GetDistanceToTarget_sweepangle_fov_front", Distance, 2, &uvp_ds_fov_front),
    C("GetDistanceToTargetPos_sweepangle_fov_front", Distance, 3, &uvp_ds_fov_front),
    C("GetDistanceToTarget_sweepdistance_fov_front", Distance, 4, &uvp_ds_fov_front),
    C("GetDistanceToTargetPos_sweepdistance_fov_front", Distance, 5, &uvp_ds_fov_front),
    C("GetDistanceToTarget_fov_behind", Distance, 0, &uvp_ds_fov_behind),
    C("GetDistanceToTargetPos_fov_behind", Distance, 1, &uvp_ds_fov_behind),
    C("GetDistanceToTarget_sweepangle_fov_behind", Distance, 2, &uvp_ds_fov_behind),
    C("GetDistanceToTargetPos_sweepangle_fov_behind", Distance, 3, &uvp_ds_fov_behind),
    C("GetDistanceToTarget_sweepdistance_fov_behind", Distance, 4, &uvp_ds_fov_behind),
    C("GetDistanceToTargetPos_sweepdistance_fov_behind", Distance, 5, &uvp_ds_fov_behind),
    C("GetDistanceToTarget_fov_side", Distance, 0, &uvp_ds_fov_side),
    C("GetDistanceToTargetPos_fov_side", Distance, 1, &uvp_ds_fov_side),
    C("GetDistanceToTarget_sweepangle_fov_side", Distance, 2, &uvp_ds_fov_side),
    C("GetDistanceToTargetPos_sweepangle_fov_side", Distance, 3, &uvp_ds_fov_side),
    C("GetDistanceToTarget_sweepdistance_fov_side", Distance, 4, &uvp_ds_fov_side),
    C("GetDistanceToTargetPos_sweepdistance_fov_side", Distance, 5, &uvp_ds_fov_side),
    C("GetDistanceToTarget_fov_side_edge", Distance, 0, &uvp_ds_fov_side_edge),
    C("GetDistanceToTargetPos_fov_side_edge", Distance, 1, &uvp_ds_fov_side_edge),
    C("GetDistanceToTarget_sweepangle_fov_side_edge", Distance, 2, &uvp_ds_fov_side_edge),
    C("GetDistanceToTargetPos_sweepangle_fov_side_edge", Distance, 3, &uvp_ds_fov_side_edge),
    C("GetDistanceToTarget_sweepdistance_fov_side_edge", Distance, 4, &uvp_ds_fov_side_edge),
    C("GetDistanceToTargetPos_sweepdistance_fov_side_edge", Distance, 5, &uvp_ds_fov_side_edge),
    C("GetDistanceToTarget_fov_zero_front", Distance, 0, &uvp_ds_fov_zero_front),
    C("GetDistanceToTargetPos_fov_zero_front", Distance, 1, &uvp_ds_fov_zero_front),
    C("GetDistanceToTarget_sweepangle_fov_zero_front", Distance, 2, &uvp_ds_fov_zero_front),
    C("GetDistanceToTargetPos_sweepangle_fov_zero_front", Distance, 3, &uvp_ds_fov_zero_front),
    C("GetDistanceToTarget_sweepdistance_fov_zero_front", Distance, 4, &uvp_ds_fov_zero_front),
    C("GetDistanceToTargetPos_sweepdistance_fov_zero_front", Distance, 5, &uvp_ds_fov_zero_front),
    C("GetDistanceToTarget_fov_zero_slightside", Distance, 0, &uvp_ds_fov_zero_slightside),
    C("GetDistanceToTargetPos_fov_zero_slightside", Distance, 1, &uvp_ds_fov_zero_slightside),
    C("GetDistanceToTarget_sweepangle_fov_zero_slightside", Distance, 2, &uvp_ds_fov_zero_slightside),
    C("GetDistanceToTargetPos_sweepangle_fov_zero_slightside", Distance, 3, &uvp_ds_fov_zero_slightside),
    C("GetDistanceToTarget_sweepdistance_fov_zero_slightside", Distance, 4, &uvp_ds_fov_zero_slightside),
    C("GetDistanceToTargetPos_sweepdistance_fov_zero_slightside", Distance, 5, &uvp_ds_fov_zero_slightside),
    C("GetDistanceToTarget_fov_one_straight", Distance, 0, &uvp_ds_fov_one_straight),
    C("GetDistanceToTargetPos_fov_one_straight", Distance, 1, &uvp_ds_fov_one_straight),
    C("GetDistanceToTarget_sweepangle_fov_one_straight", Distance, 2, &uvp_ds_fov_one_straight),
    C("GetDistanceToTargetPos_sweepangle_fov_one_straight", Distance, 3, &uvp_ds_fov_one_straight),
    C("GetDistanceToTarget_sweepdistance_fov_one_straight", Distance, 4, &uvp_ds_fov_one_straight),
    C("GetDistanceToTargetPos_sweepdistance_fov_one_straight", Distance, 5, &uvp_ds_fov_one_straight),
    C("GetDistanceToTarget_fov_zero_exact_side", Distance, 0, &uvp_ds_fov_zero_exact_side),
    C("GetDistanceToTargetPos_fov_zero_exact_side", Distance, 1, &uvp_ds_fov_zero_exact_side),
    C("GetDistanceToTarget_sweepangle_fov_zero_exact_side", Distance, 2, &uvp_ds_fov_zero_exact_side),
    C("GetDistanceToTargetPos_sweepangle_fov_zero_exact_side", Distance, 3, &uvp_ds_fov_zero_exact_side),
    C("GetDistanceToTarget_sweepdistance_fov_zero_exact_side", Distance, 4, &uvp_ds_fov_zero_exact_side),
    C("GetDistanceToTargetPos_sweepdistance_fov_zero_exact_side", Distance, 5, &uvp_ds_fov_zero_exact_side),
    C("GetDistanceToTarget_fov_one", Distance, 0, &uvp_ds_fov_one),
    C("GetDistanceToTargetPos_fov_one", Distance, 1, &uvp_ds_fov_one),
    C("GetDistanceToTarget_sweepangle_fov_one", Distance, 2, &uvp_ds_fov_one),
    C("GetDistanceToTargetPos_sweepangle_fov_one", Distance, 3, &uvp_ds_fov_one),
    C("GetDistanceToTarget_sweepdistance_fov_one", Distance, 4, &uvp_ds_fov_one),
    C("GetDistanceToTargetPos_sweepdistance_fov_one", Distance, 5, &uvp_ds_fov_one),
    C("GetDistanceToTarget_fov_negative_behind_side", Distance, 0, &uvp_ds_fov_negative_behind_side),
    C("GetDistanceToTargetPos_fov_negative_behind_side", Distance, 1, &uvp_ds_fov_negative_behind_side),
    C("GetDistanceToTarget_sweepangle_fov_negative_behind_side", Distance, 2, &uvp_ds_fov_negative_behind_side),
    C("GetDistanceToTargetPos_sweepangle_fov_negative_behind_side", Distance, 3, &uvp_ds_fov_negative_behind_side),
    C("GetDistanceToTarget_sweepdistance_fov_negative_behind_side", Distance, 4, &uvp_ds_fov_negative_behind_side),
    C("GetDistanceToTargetPos_sweepdistance_fov_negative_behind_side", Distance, 5, &uvp_ds_fov_negative_behind_side),
    C("GetDistanceToTarget_fov_negative_behind_center", Distance, 0, &uvp_ds_fov_negative_behind_center),
    C("GetDistanceToTargetPos_fov_negative_behind_center", Distance, 1, &uvp_ds_fov_negative_behind_center),
    C("GetDistanceToTarget_sweepangle_fov_negative_behind_center", Distance, 2, &uvp_ds_fov_negative_behind_center),
    C("GetDistanceToTargetPos_sweepangle_fov_negative_behind_center", Distance, 3, &uvp_ds_fov_negative_behind_center),
    C("GetDistanceToTarget_sweepdistance_fov_negative_behind_center", Distance, 4, &uvp_ds_fov_negative_behind_center),
    C("GetDistanceToTargetPos_sweepdistance_fov_negative_behind_center", Distance, 5, &uvp_ds_fov_negative_behind_center),
    C("GetDistanceToTarget_fov_negative_behind_wide", Distance, 0, &uvp_ds_fov_negative_behind_wide),
    C("GetDistanceToTargetPos_fov_negative_behind_wide", Distance, 1, &uvp_ds_fov_negative_behind_wide),
    C("GetDistanceToTarget_sweepangle_fov_negative_behind_wide", Distance, 2, &uvp_ds_fov_negative_behind_wide),
    C("GetDistanceToTargetPos_sweepangle_fov_negative_behind_wide", Distance, 3, &uvp_ds_fov_negative_behind_wide),
    C("GetDistanceToTarget_sweepdistance_fov_negative_behind_wide", Distance, 4, &uvp_ds_fov_negative_behind_wide),
    C("GetDistanceToTargetPos_sweepdistance_fov_negative_behind_wide", Distance, 5, &uvp_ds_fov_negative_behind_wide),
    C("GetDistanceToTarget_fov_minus1_behind", Distance, 0, &uvp_ds_fov_minus1_behind),
    C("GetDistanceToTargetPos_fov_minus1_behind", Distance, 1, &uvp_ds_fov_minus1_behind),
    C("GetDistanceToTarget_sweepangle_fov_minus1_behind", Distance, 2, &uvp_ds_fov_minus1_behind),
    C("GetDistanceToTargetPos_sweepangle_fov_minus1_behind", Distance, 3, &uvp_ds_fov_minus1_behind),
    C("GetDistanceToTarget_sweepdistance_fov_minus1_behind", Distance, 4, &uvp_ds_fov_minus1_behind),
    C("GetDistanceToTargetPos_sweepdistance_fov_minus1_behind", Distance, 5, &uvp_ds_fov_minus1_behind),
    C("GetDistanceToTarget_fov_minus099_behind", Distance, 0, &uvp_ds_fov_minus099_behind),
    C("GetDistanceToTargetPos_fov_minus099_behind", Distance, 1, &uvp_ds_fov_minus099_behind),
    C("GetDistanceToTarget_sweepangle_fov_minus099_behind", Distance, 2, &uvp_ds_fov_minus099_behind),
    C("GetDistanceToTargetPos_sweepangle_fov_minus099_behind", Distance, 3, &uvp_ds_fov_minus099_behind),
    C("GetDistanceToTarget_sweepdistance_fov_minus099_behind", Distance, 4, &uvp_ds_fov_minus099_behind),
    C("GetDistanceToTargetPos_sweepdistance_fov_minus099_behind", Distance, 5, &uvp_ds_fov_minus099_behind),
    C("GetDistanceToTarget_fov_big", Distance, 0, &uvp_ds_fov_big),
    C("GetDistanceToTargetPos_fov_big", Distance, 1, &uvp_ds_fov_big),
    C("GetDistanceToTarget_sweepangle_fov_big", Distance, 2, &uvp_ds_fov_big),
    C("GetDistanceToTargetPos_sweepangle_fov_big", Distance, 3, &uvp_ds_fov_big),
    C("GetDistanceToTarget_sweepdistance_fov_big", Distance, 4, &uvp_ds_fov_big),
    C("GetDistanceToTargetPos_sweepdistance_fov_big", Distance, 5, &uvp_ds_fov_big),
    C("GetDistanceToTarget_fov_vertical", Distance, 0, &uvp_ds_fov_vertical),
    C("GetDistanceToTargetPos_fov_vertical", Distance, 1, &uvp_ds_fov_vertical),
    C("GetDistanceToTarget_sweepangle_fov_vertical", Distance, 2, &uvp_ds_fov_vertical),
    C("GetDistanceToTargetPos_sweepangle_fov_vertical", Distance, 3, &uvp_ds_fov_vertical),
    C("GetDistanceToTarget_sweepdistance_fov_vertical", Distance, 4, &uvp_ds_fov_vertical),
    C("GetDistanceToTargetPos_sweepdistance_fov_vertical", Distance, 5, &uvp_ds_fov_vertical),
    C("GetDistanceToTarget_fov_above_front", Distance, 0, &uvp_ds_fov_above_front),
    C("GetDistanceToTargetPos_fov_above_front", Distance, 1, &uvp_ds_fov_above_front),
    C("GetDistanceToTarget_sweepangle_fov_above_front", Distance, 2, &uvp_ds_fov_above_front),
    C("GetDistanceToTargetPos_sweepangle_fov_above_front", Distance, 3, &uvp_ds_fov_above_front),
    C("GetDistanceToTarget_sweepdistance_fov_above_front", Distance, 4, &uvp_ds_fov_above_front),
    C("GetDistanceToTargetPos_sweepdistance_fov_above_front", Distance, 5, &uvp_ds_fov_above_front),
    C("GetDistanceToTarget_sight0", Distance, 0, &uvp_ds_sight0),
    C("GetDistanceToTargetPos_sight0", Distance, 1, &uvp_ds_sight0),
    C("GetDistanceToTarget_sweepangle_sight0", Distance, 2, &uvp_ds_sight0),
    C("GetDistanceToTargetPos_sweepangle_sight0", Distance, 3, &uvp_ds_sight0),
    C("GetDistanceToTarget_sweepdistance_sight0", Distance, 4, &uvp_ds_sight0),
    C("GetDistanceToTargetPos_sweepdistance_sight0", Distance, 5, &uvp_ds_sight0),
    C("GetDistanceToTarget_sight_negative", Distance, 0, &uvp_ds_sight_negative),
    C("GetDistanceToTargetPos_sight_negative", Distance, 1, &uvp_ds_sight_negative),
    C("GetDistanceToTarget_sweepangle_sight_negative", Distance, 2, &uvp_ds_sight_negative),
    C("GetDistanceToTargetPos_sweepangle_sight_negative", Distance, 3, &uvp_ds_sight_negative),
    C("GetDistanceToTarget_sweepdistance_sight_negative", Distance, 4, &uvp_ds_sight_negative),
    C("GetDistanceToTargetPos_sweepdistance_sight_negative", Distance, 5, &uvp_ds_sight_negative),
    C("GetDistanceToTarget_startpos_far_from_viewer", Distance, 0, &uvp_ds_startpos_far_from_viewer),
    C("GetDistanceToTargetPos_startpos_far_from_viewer", Distance, 1, &uvp_ds_startpos_far_from_viewer),
    C("GetDistanceToTarget_sweepangle_startpos_far_from_viewer", Distance, 2, &uvp_ds_startpos_far_from_viewer),
    C("GetDistanceToTargetPos_sweepangle_startpos_far_from_viewer", Distance, 3, &uvp_ds_startpos_far_from_viewer),
    C("GetDistanceToTarget_sweepdistance_startpos_far_from_viewer", Distance, 4, &uvp_ds_startpos_far_from_viewer),
    C("GetDistanceToTargetPos_sweepdistance_startpos_far_from_viewer", Distance, 5, &uvp_ds_startpos_far_from_viewer),
    C("GetDistanceToTarget_startpos_equals_target", Distance, 0, &uvp_ds_startpos_equals_target),
    C("GetDistanceToTargetPos_startpos_equals_target", Distance, 1, &uvp_ds_startpos_equals_target),
    C("GetDistanceToTarget_sweepangle_startpos_equals_target", Distance, 2, &uvp_ds_startpos_equals_target),
    C("GetDistanceToTargetPos_sweepangle_startpos_equals_target", Distance, 3, &uvp_ds_startpos_equals_target),
    C("GetDistanceToTarget_sweepdistance_startpos_equals_target", Distance, 4, &uvp_ds_startpos_equals_target),
    C("GetDistanceToTargetPos_sweepdistance_startpos_equals_target", Distance, 5, &uvp_ds_startpos_equals_target),
    C("GetDistanceToTarget_diagonal", Distance, 0, &uvp_ds_diagonal),
    C("GetDistanceToTargetPos_diagonal", Distance, 1, &uvp_ds_diagonal),
    C("GetDistanceToTarget_sweepangle_diagonal", Distance, 2, &uvp_ds_diagonal),
    C("GetDistanceToTargetPos_sweepangle_diagonal", Distance, 3, &uvp_ds_diagonal),
    C("GetDistanceToTarget_sweepdistance_diagonal", Distance, 4, &uvp_ds_diagonal),
    C("GetDistanceToTargetPos_sweepdistance_diagonal", Distance, 5, &uvp_ds_diagonal),
    C("CanSeeTarget_nulltarget", CanSeeTarget, 0, &uvp_cs_nulltarget),
    C("CanSeeTarget_sweeprand_nulltarget", CanSeeTarget, 1, &uvp_cs_nulltarget),
    C("CanSeeTarget_itemtarget", CanSeeTarget, 0, &uvp_cs_itemtarget),
    C("CanSeeTarget_sweeprand_itemtarget", CanSeeTarget, 1, &uvp_cs_itemtarget),
    C("CanSeeTarget_ghosttarget", CanSeeTarget, 0, &uvp_cs_ghosttarget),
    C("CanSeeTarget_sweeprand_ghosttarget", CanSeeTarget, 1, &uvp_cs_ghosttarget),
    C("CanSeeTarget_close_active", CanSeeTarget, 0, &uvp_cs_close_active),
    C("CanSeeTarget_sweeprand_close_active", CanSeeTarget, 1, &uvp_cs_close_active),
    C("CanSeeTarget_close_inactive", CanSeeTarget, 0, &uvp_cs_close_inactive),
    C("CanSeeTarget_sweeprand_close_inactive", CanSeeTarget, 1, &uvp_cs_close_inactive),
    C("CanSeeTarget_close_edge", CanSeeTarget, 0, &uvp_cs_close_edge),
    C("CanSeeTarget_sweeprand_close_edge", CanSeeTarget, 1, &uvp_cs_close_edge),
    C("CanSeeTarget_close_edge_inactive", CanSeeTarget, 0, &uvp_cs_close_edge_inactive),
    C("CanSeeTarget_sweeprand_close_edge_inactive", CanSeeTarget, 1, &uvp_cs_close_edge_inactive),
    C("CanSeeTarget_far_active_lit", CanSeeTarget, 0, &uvp_cs_far_active_lit),
    C("CanSeeTarget_sweeprand_far_active_lit", CanSeeTarget, 1, &uvp_cs_far_active_lit),
    C("CanSeeTarget_far_active_lit_seed2", CanSeeTarget, 0, &uvp_cs_far_active_lit_seed2),
    C("CanSeeTarget_sweeprand_far_active_lit_seed2", CanSeeTarget, 1, &uvp_cs_far_active_lit_seed2),
    C("CanSeeTarget_far_active_lit_seed3", CanSeeTarget, 0, &uvp_cs_far_active_lit_seed3),
    C("CanSeeTarget_sweeprand_far_active_lit_seed3", CanSeeTarget, 1, &uvp_cs_far_active_lit_seed3),
    C("CanSeeTarget_far_inactive_lit", CanSeeTarget, 0, &uvp_cs_far_inactive_lit),
    C("CanSeeTarget_sweeprand_far_inactive_lit", CanSeeTarget, 1, &uvp_cs_far_inactive_lit),
    C("CanSeeTarget_far_inactive_lit_seed5", CanSeeTarget, 0, &uvp_cs_far_inactive_lit_seed5),
    C("CanSeeTarget_sweeprand_far_inactive_lit_seed5", CanSeeTarget, 1, &uvp_cs_far_inactive_lit_seed5),
    C("CanSeeTarget_very_far", CanSeeTarget, 0, &uvp_cs_very_far),
    C("CanSeeTarget_sweeprand_very_far", CanSeeTarget, 1, &uvp_cs_very_far),
    C("CanSeeTarget_very_far_seed9", CanSeeTarget, 0, &uvp_cs_very_far_seed9),
    C("CanSeeTarget_sweeprand_very_far_seed9", CanSeeTarget, 1, &uvp_cs_very_far_seed9),
    C("CanSeeTarget_very_far_seed100", CanSeeTarget, 0, &uvp_cs_very_far_seed100),
    C("CanSeeTarget_sweeprand_very_far_seed100", CanSeeTarget, 1, &uvp_cs_very_far_seed100),
    C("CanSeeTarget_mid3_active", CanSeeTarget, 0, &uvp_cs_mid3_active),
    C("CanSeeTarget_sweeprand_mid3_active", CanSeeTarget, 1, &uvp_cs_mid3_active),
    C("CanSeeTarget_mid4_inactive", CanSeeTarget, 0, &uvp_cs_mid4_inactive),
    C("CanSeeTarget_sweeprand_mid4_inactive", CanSeeTarget, 1, &uvp_cs_mid4_inactive),
    C("CanSeeTarget_mid5_active_dark", CanSeeTarget, 0, &uvp_cs_mid5_active_dark),
    C("CanSeeTarget_sweeprand_mid5_active_dark", CanSeeTarget, 1, &uvp_cs_mid5_active_dark),
    C("CanSeeTarget_mid6_active_crouching", CanSeeTarget, 0, &uvp_cs_mid6_active_crouching),
    C("CanSeeTarget_sweeprand_mid6_active_crouching", CanSeeTarget, 1, &uvp_cs_mid6_active_crouching),
    C("CanSeeTarget_mid5_active_still", CanSeeTarget, 0, &uvp_cs_mid5_active_still),
    C("CanSeeTarget_sweeprand_mid5_active_still", CanSeeTarget, 1, &uvp_cs_mid5_active_still),
    C("CanSeeTarget_mid5_active_crouching_still", CanSeeTarget, 0, &uvp_cs_mid5_active_crouching_still),
    C("CanSeeTarget_sweeprand_mid5_active_crouching_still", CanSeeTarget, 1, &uvp_cs_mid5_active_crouching_still),
    C("CanSeeTarget_mid5_active_invisible", CanSeeTarget, 0, &uvp_cs_mid5_active_invisible),
    C("CanSeeTarget_sweeprand_mid5_active_invisible", CanSeeTarget, 1, &uvp_cs_mid5_active_invisible),
    C("CanSeeTarget_mid7_inactive_blind_dark", CanSeeTarget, 0, &uvp_cs_mid7_inactive_blind_dark),
    C("CanSeeTarget_sweeprand_mid7_inactive_blind_dark", CanSeeTarget, 1, &uvp_cs_mid7_inactive_blind_dark),
    C("CanSeeTarget_dark", CanSeeTarget, 0, &uvp_cs_dark),
    C("CanSeeTarget_sweeprand_dark", CanSeeTarget, 1, &uvp_cs_dark),
    C("CanSeeTarget_dark_seed7", CanSeeTarget, 0, &uvp_cs_dark_seed7),
    C("CanSeeTarget_sweeprand_dark_seed7", CanSeeTarget, 1, &uvp_cs_dark_seed7),
    C("CanSeeTarget_dark_seeindark", CanSeeTarget, 0, &uvp_cs_dark_seeindark),
    C("CanSeeTarget_sweeprand_dark_seeindark", CanSeeTarget, 1, &uvp_cs_dark_seeindark),
    C("CanSeeTarget_dark_headlight", CanSeeTarget, 0, &uvp_cs_dark_headlight),
    C("CanSeeTarget_sweeprand_dark_headlight", CanSeeTarget, 1, &uvp_cs_dark_headlight),
    C("CanSeeTarget_dark_unknownflag", CanSeeTarget, 0, &uvp_cs_dark_unknownflag),
    C("CanSeeTarget_sweeprand_dark_unknownflag", CanSeeTarget, 1, &uvp_cs_dark_unknownflag),
    C("CanSeeTarget_dark_unknownflag_other", CanSeeTarget, 0, &uvp_cs_dark_unknownflag_other),
    C("CanSeeTarget_sweeprand_dark_unknownflag_other", CanSeeTarget, 1, &uvp_cs_dark_unknownflag_other),
    C("CanSeeTarget_half_light", CanSeeTarget, 0, &uvp_cs_half_light),
    C("CanSeeTarget_sweeprand_half_light", CanSeeTarget, 1, &uvp_cs_half_light),
    C("CanSeeTarget_lit_just_below", CanSeeTarget, 0, &uvp_cs_lit_just_below),
    C("CanSeeTarget_sweeprand_lit_just_below", CanSeeTarget, 1, &uvp_cs_lit_just_below),
    C("CanSeeTarget_crouching", CanSeeTarget, 0, &uvp_cs_crouching),
    C("CanSeeTarget_sweeprand_crouching", CanSeeTarget, 1, &uvp_cs_crouching),
    C("CanSeeTarget_crouching_seed4", CanSeeTarget, 0, &uvp_cs_crouching_seed4),
    C("CanSeeTarget_sweeprand_crouching_seed4", CanSeeTarget, 1, &uvp_cs_crouching_seed4),
    C("CanSeeTarget_standing_still", CanSeeTarget, 0, &uvp_cs_standing_still),
    C("CanSeeTarget_sweeprand_standing_still", CanSeeTarget, 1, &uvp_cs_standing_still),
    C("CanSeeTarget_standing_still_seed4", CanSeeTarget, 0, &uvp_cs_standing_still_seed4),
    C("CanSeeTarget_sweeprand_standing_still_seed4", CanSeeTarget, 1, &uvp_cs_standing_still_seed4),
    C("CanSeeTarget_crouching_still", CanSeeTarget, 0, &uvp_cs_crouching_still),
    C("CanSeeTarget_sweeprand_crouching_still", CanSeeTarget, 1, &uvp_cs_crouching_still),
    C("CanSeeTarget_crouching_still_seed8", CanSeeTarget, 0, &uvp_cs_crouching_still_seed8),
    C("CanSeeTarget_sweeprand_crouching_still_seed8", CanSeeTarget, 1, &uvp_cs_crouching_still_seed8),
    C("CanSeeTarget_nonphysics_target", CanSeeTarget, 0, &uvp_cs_nonphysics_target),
    C("CanSeeTarget_sweeprand_nonphysics_target", CanSeeTarget, 1, &uvp_cs_nonphysics_target),
    C("CanSeeTarget_headlight_crouching_ignored", CanSeeTarget, 0, &uvp_cs_headlight_crouching_ignored),
    C("CanSeeTarget_sweeprand_headlight_crouching_ignored", CanSeeTarget, 1, &uvp_cs_headlight_crouching_ignored),
    C("CanSeeTarget_invisible_target", CanSeeTarget, 0, &uvp_cs_invisible_target),
    C("CanSeeTarget_sweeprand_invisible_target", CanSeeTarget, 1, &uvp_cs_invisible_target),
    C("CanSeeTarget_invisible_target_seed2", CanSeeTarget, 0, &uvp_cs_invisible_target_seed2),
    C("CanSeeTarget_sweeprand_invisible_target_seed2", CanSeeTarget, 1, &uvp_cs_invisible_target_seed2),
    C("CanSeeTarget_invisible_seeinvisible", CanSeeTarget, 0, &uvp_cs_invisible_seeinvisible),
    C("CanSeeTarget_sweeprand_invisible_seeinvisible", CanSeeTarget, 1, &uvp_cs_invisible_seeinvisible),
    C("CanSeeTarget_blind", CanSeeTarget, 0, &uvp_cs_blind),
    C("CanSeeTarget_sweeprand_blind", CanSeeTarget, 1, &uvp_cs_blind),
    C("CanSeeTarget_blind_seed3", CanSeeTarget, 0, &uvp_cs_blind_seed3),
    C("CanSeeTarget_sweeprand_blind_seed3", CanSeeTarget, 1, &uvp_cs_blind_seed3),
    C("CanSeeTarget_blind_invisible", CanSeeTarget, 0, &uvp_cs_blind_invisible),
    C("CanSeeTarget_sweeprand_blind_invisible", CanSeeTarget, 1, &uvp_cs_blind_invisible),
    C("CanSeeTarget_blind_invisible_seed6", CanSeeTarget, 0, &uvp_cs_blind_invisible_seed6),
    C("CanSeeTarget_sweeprand_blind_invisible_seed6", CanSeeTarget, 1, &uvp_cs_blind_invisible_seed6),
    C("CanSeeTarget_blind_seeinvisible", CanSeeTarget, 0, &uvp_cs_blind_seeinvisible),
    C("CanSeeTarget_sweeprand_blind_seeinvisible", CanSeeTarget, 1, &uvp_cs_blind_seeinvisible),
    C("CanSeeTarget_everything_bad", CanSeeTarget, 0, &uvp_cs_everything_bad),
    C("CanSeeTarget_sweeprand_everything_bad", CanSeeTarget, 1, &uvp_cs_everything_bad),
    C("CanSeeTarget_everything_bad_seed20", CanSeeTarget, 0, &uvp_cs_everything_bad_seed20),
    C("CanSeeTarget_sweeprand_everything_bad_seed20", CanSeeTarget, 1, &uvp_cs_everything_bad_seed20),
    C("CanSeeTarget_negative_distance", CanSeeTarget, 0, &uvp_cs_negative_distance),
    C("CanSeeTarget_sweeprand_negative_distance", CanSeeTarget, 1, &uvp_cs_negative_distance),
    C("CanSeeTarget_edge_nan_distance", CanSeeTarget, 0, &uvp_cs_edge_nan_distance),
    C("CanSeeTarget_sweeprand_edge_nan_distance", CanSeeTarget, 1, &uvp_cs_edge_nan_distance),
    C("CanSeeTarget_player_target", CanSeeTarget, 0, &uvp_cs_player_target),
    C("CanSeeTarget_sweeprand_player_target", CanSeeTarget, 1, &uvp_cs_player_target),
    C("UpdateTargetSight_sameframe", UpdateSight, 0, &uvp_us_sameframe),
    C("UpdateGoalSight_sameframe", UpdateSight, 1, &uvp_us_sameframe),
    C("UpdateTargetSight_sameframe_primary", UpdateSight, 0, &uvp_us_sameframe_primary),
    C("UpdateGoalSight_sameframe_primary", UpdateSight, 1, &uvp_us_sameframe_primary),
    C("UpdateTargetSight_sameframe_lob", UpdateSight, 0, &uvp_us_sameframe_lob),
    C("UpdateGoalSight_sameframe_lob", UpdateSight, 1, &uvp_us_sameframe_lob),
    C("UpdateTargetSight_sameframe_alt", UpdateSight, 0, &uvp_us_sameframe_alt),
    C("UpdateGoalSight_sameframe_alt", UpdateSight, 1, &uvp_us_sameframe_alt),
    C("UpdateTargetSight_sameframe_other_flags", UpdateSight, 0, &uvp_us_sameframe_other_flags),
    C("UpdateGoalSight_sameframe_other_flags", UpdateSight, 1, &uvp_us_sameframe_other_flags),
    C("UpdateTargetSight_visible_target", UpdateSight, 0, &uvp_us_visible_target),
    C("UpdateGoalSight_visible_target", UpdateSight, 1, &uvp_us_visible_target),
    C("UpdateTargetSight_visible_target_prevhidden_cansee", UpdateSight, 0, &uvp_us_visible_target_prevhidden_cansee),
    C("UpdateGoalSight_visible_target_prevhidden_cansee", UpdateSight, 1, &uvp_us_visible_target_prevhidden_cansee),
    C("UpdateTargetSight_visible_target_prevhidden_cantsee", UpdateSight, 0, &uvp_us_visible_target_prevhidden_cantsee),
    C("UpdateGoalSight_visible_target_prevhidden_cantsee", UpdateSight, 1, &uvp_us_visible_target_prevhidden_cantsee),
    C("UpdateTargetSight_visible_target_prevtoofar", UpdateSight, 0, &uvp_us_visible_target_prevtoofar),
    C("UpdateGoalSight_visible_target_prevtoofar", UpdateSight, 1, &uvp_us_visible_target_prevtoofar),
    C("UpdateTargetSight_visible_target_prevhidden_dark_seed2", UpdateSight, 0, &uvp_us_visible_target_prevhidden_dark_seed2),
    C("UpdateGoalSight_visible_target_prevhidden_dark_seed2", UpdateSight, 1, &uvp_us_visible_target_prevhidden_dark_seed2),
    C("UpdateTargetSight_hidden_los", UpdateSight, 0, &uvp_us_hidden_los),
    C("UpdateGoalSight_hidden_los", UpdateSight, 1, &uvp_us_hidden_los),
    C("UpdateTargetSight_toofar", UpdateSight, 0, &uvp_us_toofar),
    C("UpdateGoalSight_toofar", UpdateSight, 1, &uvp_us_toofar),
    C("UpdateTargetSight_outoffov", UpdateSight, 0, &uvp_us_outoffov),
    C("UpdateGoalSight_outoffov", UpdateSight, 1, &uvp_us_outoffov),
    C("UpdateTargetSight_invisible_target", UpdateSight, 0, &uvp_us_invisible_target),
    C("UpdateGoalSight_invisible_target", UpdateSight, 1, &uvp_us_invisible_target),
    C("UpdateTargetSight_invisible_seeinvisible", UpdateSight, 0, &uvp_us_invisible_seeinvisible),
    C("UpdateGoalSight_invisible_seeinvisible", UpdateSight, 1, &uvp_us_invisible_seeinvisible),
    C("UpdateTargetSight_blind_owner", UpdateSight, 0, &uvp_us_blind_owner),
    C("UpdateGoalSight_blind_owner", UpdateSight, 1, &uvp_us_blind_owner),
    C("UpdateTargetSight_blind_seeinvisible", UpdateSight, 0, &uvp_us_blind_seeinvisible),
    C("UpdateGoalSight_blind_seeinvisible", UpdateSight, 1, &uvp_us_blind_seeinvisible),
    C("UpdateTargetSight_item_target", UpdateSight, 0, &uvp_us_item_target),
    C("UpdateGoalSight_item_target", UpdateSight, 1, &uvp_us_item_target),
    C("UpdateTargetSight_weapon_selected", UpdateSight, 0, &uvp_us_weapon_selected),
    C("UpdateGoalSight_weapon_selected", UpdateSight, 1, &uvp_us_weapon_selected),
    C("UpdateTargetSight_weapon_template", UpdateSight, 0, &uvp_us_weapon_template),
    C("UpdateGoalSight_weapon_template", UpdateSight, 1, &uvp_us_weapon_template),
    C("UpdateTargetSight_weapon_selected_and_template", UpdateSight, 0, &uvp_us_weapon_selected_and_template),
    C("UpdateGoalSight_weapon_selected_and_template", UpdateSight, 1, &uvp_us_weapon_selected_and_template),
    C("UpdateTargetSight_fire_offset", UpdateSight, 0, &uvp_us_fire_offset),
    C("UpdateGoalSight_fire_offset", UpdateSight, 1, &uvp_us_fire_offset),
    C("UpdateTargetSight_eye_flag_off", UpdateSight, 0, &uvp_us_eye_flag_off),
    C("UpdateGoalSight_eye_flag_off", UpdateSight, 1, &uvp_us_eye_flag_off),
    C("UpdateTargetSight_eye_flag_on_zero_eye", UpdateSight, 0, &uvp_us_eye_flag_on_zero_eye),
    C("UpdateGoalSight_eye_flag_on_zero_eye", UpdateSight, 1, &uvp_us_eye_flag_on_zero_eye),
    C("UpdateTargetSight_eye_flag_on", UpdateSight, 0, &uvp_us_eye_flag_on),
    C("UpdateGoalSight_eye_flag_on", UpdateSight, 1, &uvp_us_eye_flag_on),
    C("UpdateTargetSight_eye_flag_on_nothing", UpdateSight, 0, &uvp_us_eye_flag_on_nothing),
    C("UpdateGoalSight_eye_flag_on_nothing", UpdateSight, 1, &uvp_us_eye_flag_on_nothing),
    C("UpdateTargetSight_eye_flag_on_target_close", UpdateSight, 0, &uvp_us_eye_flag_on_target_close),
    C("UpdateGoalSight_eye_flag_on_target_close", UpdateSight, 1, &uvp_us_eye_flag_on_target_close),
    C("UpdateTargetSight_eye_flag_on_target_ontop", UpdateSight, 0, &uvp_us_eye_flag_on_target_ontop),
    C("UpdateGoalSight_eye_flag_on_target_ontop", UpdateSight, 1, &uvp_us_eye_flag_on_target_ontop),
    C("UpdateTargetSight_eye_flag_on_hidden", UpdateSight, 0, &uvp_us_eye_flag_on_hidden),
    C("UpdateGoalSight_eye_flag_on_hidden", UpdateSight, 1, &uvp_us_eye_flag_on_hidden),
    C("UpdateTargetSight_notarget_visible", UpdateSight, 0, &uvp_us_notarget_visible),
    C("UpdateGoalSight_notarget_visible", UpdateSight, 1, &uvp_us_notarget_visible),
    C("UpdateTargetSight_notarget_visible_prevmode", UpdateSight, 0, &uvp_us_notarget_visible_prevmode),
    C("UpdateGoalSight_notarget_visible_prevmode", UpdateSight, 1, &uvp_us_notarget_visible_prevmode),
    C("UpdateTargetSight_notarget_hidden", UpdateSight, 0, &uvp_us_notarget_hidden),
    C("UpdateGoalSight_notarget_hidden", UpdateSight, 1, &uvp_us_notarget_hidden),
    C("UpdateTargetSight_notarget_toofar", UpdateSight, 0, &uvp_us_notarget_toofar),
    C("UpdateGoalSight_notarget_toofar", UpdateSight, 1, &uvp_us_notarget_toofar),
    C("UpdateTargetSight_notarget_outoffov", UpdateSight, 0, &uvp_us_notarget_outoffov),
    C("UpdateGoalSight_notarget_outoffov", UpdateSight, 1, &uvp_us_notarget_outoffov),
    C("UpdateTargetSight_notarget_eye_flag_on", UpdateSight, 0, &uvp_us_notarget_eye_flag_on),
    C("UpdateGoalSight_notarget_eye_flag_on", UpdateSight, 1, &uvp_us_notarget_eye_flag_on),
    C("UpdateTargetSight_notarget_eye_flag_on_ontop", UpdateSight, 0, &uvp_us_notarget_eye_flag_on_ontop),
    C("UpdateGoalSight_notarget_eye_flag_on_ontop", UpdateSight, 1, &uvp_us_notarget_eye_flag_on_ontop),
    C("IsLineOfFireClear_nulllocal", IsLineOfFireClear, 0, &uvp_lf_nulllocal),
    C("IsLineOfFireClear_nullowner", IsLineOfFireClear, 0, &uvp_lf_nullowner),
    C("IsLineOfFireClear_notai", IsLineOfFireClear, 0, &uvp_lf_notai),
    C("IsLineOfFireClear_noweapon", IsLineOfFireClear, 0, &uvp_lf_noweapon),
    C("IsLineOfFireClear_notarget", IsLineOfFireClear, 0, &uvp_lf_notarget),
    C("IsLineOfFireClear_noothers", IsLineOfFireClear, 0, &uvp_lf_noothers),
    C("IsLineOfFireClear_blocked", IsLineOfFireClear, 0, &uvp_lf_blocked),
    C("IsLineOfFireClear_blocked_second", IsLineOfFireClear, 0, &uvp_lf_blocked_second),
    C("IsLineOfFireClear_blocked_third", IsLineOfFireClear, 0, &uvp_lf_blocked_third),
    C("IsLineOfFireClear_blocked_beyond_lastused", IsLineOfFireClear, 0, &uvp_lf_blocked_beyond_lastused),
    C("IsLineOfFireClear_otherfurther", IsLineOfFireClear, 0, &uvp_lf_otherfurther),
    C("IsLineOfFireClear_otherdistequal", IsLineOfFireClear, 0, &uvp_lf_otherdistequal),
    C("IsLineOfFireClear_otherdifferentdir", IsLineOfFireClear, 0, &uvp_lf_otherdifferentdir),
    C("IsLineOfFireClear_otherdotedge", IsLineOfFireClear, 0, &uvp_lf_otherdotedge),
    C("IsLineOfFireClear_otherdotjustbelow", IsLineOfFireClear, 0, &uvp_lf_otherdotjustbelow),
    C("IsLineOfFireClear_othernotvisible", IsLineOfFireClear, 0, &uvp_lf_othernotvisible),
    C("IsLineOfFireClear_othernothuman", IsLineOfFireClear, 0, &uvp_lf_othernothuman),
    C("IsLineOfFireClear_othernotai", IsLineOfFireClear, 0, &uvp_lf_othernotai),
    C("IsLineOfFireClear_othernoowner", IsLineOfFireClear, 0, &uvp_lf_othernoowner),
    C("IsLineOfFireClear_posoffset", IsLineOfFireClear, 0, &uvp_lf_posoffset),
    C("IsLineOfFireClear_posoffset_far", IsLineOfFireClear, 0, &uvp_lf_posoffset_far),
    C("IsLineOfFireClear_posoffset_sideways", IsLineOfFireClear, 0, &uvp_lf_posoffset_sideways),
    C("IsLineOfFireClear_all_clear_mixed", IsLineOfFireClear, 0, &uvp_lf_all_clear_mixed),
    C("IsLineOfFireClear_first_clear_second_blocks", IsLineOfFireClear, 0, &uvp_lf_first_clear_second_blocks),
    C("IsLineOfFireClear_lastused_negative", IsLineOfFireClear, 0, &uvp_lf_lastused_negative),
    C("IsLineOfFireClear_selfhuman", IsLineOfFireClear, 0, &uvp_lf_selfhuman),
    C("IsLineOfFireClear_selfhuman_only", IsLineOfFireClear, 0, &uvp_lf_selfhuman_only),
    C("AIFire_base", AIFire, 0, &uvp_fire_base),
    C("AIFire_aidisabled", AIFire, 0, &uvp_fire_aidisabled),
    C("AIFire_aidisabled_other_debug", AIFire, 0, &uvp_fire_aidisabled_other_debug),
    C("AIFire_destroyed", AIFire, 0, &uvp_fire_destroyed),
    C("AIFire_dying", AIFire, 0, &uvp_fire_dying),
    C("AIFire_disabledflag_only", AIFire, 0, &uvp_fire_disabledflag_only),
    C("AIFire_notarget", AIFire, 0, &uvp_fire_notarget),
    C("AIFire_underwater_nofire", AIFire, 0, &uvp_fire_underwater_nofire),
    C("AIFire_underwater_canfire", AIFire, 0, &uvp_fire_underwater_canfire),
    C("AIFire_dry_nounderwaterfire_flag", AIFire, 0, &uvp_fire_dry_nounderwaterfire_flag),
    C("AIFire_firewait", AIFire, 0, &uvp_fire_firewait),
    C("AIFire_firewait_equal", AIFire, 0, &uvp_fire_firewait_equal),
    C("AIFire_firewait_delayed_ignored", AIFire, 0, &uvp_fire_firewait_delayed_ignored),
    C("AIFire_sight_toofar", AIFire, 0, &uvp_fire_sight_toofar),
    C("AIFire_sight_hidden", AIFire, 0, &uvp_fire_sight_hidden),
    C("AIFire_sight_outoffov", AIFire, 0, &uvp_fire_sight_outoffov),
    C("AIFire_sight_delayed_ignored", AIFire, 0, &uvp_fire_sight_delayed_ignored),
    C("AIFire_dist_below_min", AIFire, 0, &uvp_fire_dist_below_min),
    C("AIFire_dist_at_min", AIFire, 0, &uvp_fire_dist_at_min),
    C("AIFire_dist_at_max", AIFire, 0, &uvp_fire_dist_at_max),
    C("AIFire_dist_above_max", AIFire, 0, &uvp_fire_dist_above_max),
    C("AIFire_dist_delayed_ignored", AIFire, 0, &uvp_fire_dist_delayed_ignored),
    C("AIFire_fov_outside", AIFire, 0, &uvp_fire_fov_outside),
    C("AIFire_fov_edge", AIFire, 0, &uvp_fire_fov_edge),
    C("AIFire_fov_outside_skipcheck", AIFire, 0, &uvp_fire_fov_outside_skipcheck),
    C("AIFire_fov_outside_delayed", AIFire, 0, &uvp_fire_fov_outside_delayed),
    C("AIFire_firedot_negative", AIFire, 0, &uvp_fire_firedot_negative),
    C("AIFire_weapon2", AIFire, 0, &uvp_fire_weapon2),
    C("AIFire_weapon3", AIFire, 0, &uvp_fire_weapon3),
    C("AIFire_weapon4", AIFire, 0, &uvp_fire_weapon4),
    C("AIFire_weapon_none_flag", AIFire, 0, &uvp_fire_weapon_none_flag),
    C("AIFire_weapon_flags_all", AIFire, 0, &uvp_fire_weapon_flags_all),
    C("AIFire_weapon_flags_23", AIFire, 0, &uvp_fire_weapon_flags_23),
    C("AIFire_weaponnum_negative", AIFire, 0, &uvp_fire_weaponnum_negative),
    C("AIFire_weaponnum_negative_delayfire", AIFire, 0, &uvp_fire_weaponnum_negative_delayfire),
    C("AIFire_weaponnum_large", AIFire, 0, &uvp_fire_weaponnum_large),
    C("AIFire_stop_on_fire", AIFire, 0, &uvp_fire_stop_on_fire),
    C("AIFire_stop_on_fire_delayed", AIFire, 0, &uvp_fire_stop_on_fire_delayed),
    C("AIFire_delayfire_actor", AIFire, 0, &uvp_fire_delayfire_actor),
    C("AIFire_delayfire_actor_weapon3", AIFire, 0, &uvp_fire_delayfire_actor_weapon3),
    C("AIFire_delayfire_second_phase", AIFire, 0, &uvp_fire_delayfire_second_phase),
    C("AIFire_delayed_flag_noactorflag", AIFire, 0, &uvp_fire_delayed_flag_noactorflag),
    C("AIFire_update_primary", AIFire, 0, &uvp_fire_update_primary),
    C("AIFire_update_alt", AIFire, 0, &uvp_fire_update_alt),
    C("AIFire_update_primary_hidden", AIFire, 0, &uvp_fire_update_primary_hidden),
    C("AIFire_update_newframe", AIFire, 0, &uvp_fire_update_newframe),
    C("AIFire_update_newframe_hidden", AIFire, 0, &uvp_fire_update_newframe_hidden),
    C("AIFire_update_lob", AIFire, 0, &uvp_fire_update_lob),
    C("AIFire_update_toofar", AIFire, 0, &uvp_fire_update_toofar),
    C("AIFire_lead_stationary", AIFire, 0, &uvp_fire_lead_stationary),
    C("AIFire_lead_moving_sideways", AIFire, 0, &uvp_fire_lead_moving_sideways),
    C("AIFire_lead_moving_fast", AIFire, 0, &uvp_fire_lead_moving_fast),
    C("AIFire_lead_moving_toward", AIFire, 0, &uvp_fire_lead_moving_toward),
    C("AIFire_lead_moving_away", AIFire, 0, &uvp_fire_lead_moving_away),
    C("AIFire_lead_nonphysics_target", AIFire, 0, &uvp_fire_lead_nonphysics_target),
    C("AIFire_lead_slow_shot", AIFire, 0, &uvp_fire_lead_slow_shot),
    C("AIFire_lead_fast_shot", AIFire, 0, &uvp_fire_lead_fast_shot),
    C("AIFire_lead_edge_dot", AIFire, 0, &uvp_fire_lead_edge_dot),
    C("AIFire_lob", AIFire, 0, &uvp_fire_lob),
    C("AIFire_lob_flat", AIFire, 0, &uvp_fire_lob_flat),
    C("AIFire_lob_up", AIFire, 0, &uvp_fire_lob_up),
    C("AIFire_lob_slow_shot", AIFire, 0, &uvp_fire_lob_slow_shot),
    C("AIFire_lob_lead", AIFire, 0, &uvp_fire_lob_lead),
    C("AIFire_aimerror_hit", AIFire, 0, &uvp_fire_aimerror_hit),
    C("AIFire_aimerror_miss", AIFire, 0, &uvp_fire_aimerror_miss),
    C("AIFire_aimerror_roll_a", AIFire, 0, &uvp_fire_aimerror_roll_a),
    C("AIFire_aimerror_roll_b", AIFire, 0, &uvp_fire_aimerror_roll_b),
    C("AIFire_aimerror_roll_c", AIFire, 0, &uvp_fire_aimerror_roll_c),
    C("AIFire_aimerror_scaled", AIFire, 0, &uvp_fire_aimerror_scaled),
    C("AIFire_aimerror_zero_distance", AIFire, 0, &uvp_fire_aimerror_zero_distance),
    C("AIFire_aimerror_zero", AIFire, 0, &uvp_fire_aimerror_zero),
    C("AIFire_aimerror_negative", AIFire, 0, &uvp_fire_aimerror_negative),
    C("AIFire_tail_attached", AIFire, 0, &uvp_fire_tail_attached),
    C("AIFire_fire_failed", AIFire, 0, &uvp_fire_fire_failed),
    C("AIFire_fire_failed_delayed", AIFire, 0, &uvp_fire_fire_failed_delayed),
    C("AIFire_shot_delayed_flag", AIFire, 0, &uvp_fire_shot_delayed_flag),
    C("AIFire_notemplate_weapon0", AIFire, 0, &uvp_fire_notemplate_weapon0),
    C("AIFire_notemplate_weapon18", AIFire, 0, &uvp_fire_notemplate_weapon18),
    C("AIFire_notemplate_weapon25", AIFire, 0, &uvp_fire_notemplate_weapon25),
    C("AIFire_notemplate_weaponneg", AIFire, 0, &uvp_fire_notemplate_weaponneg),
    C("AIFire_notemplate_comfists_nocog", AIFire, 0, &uvp_fire_notemplate_comfists_nocog),
    C("AIFire_notemplate_comfists_notype", AIFire, 0, &uvp_fire_notemplate_comfists_notype),
    C("AIFire_notemplate_comfists_cog", AIFire, 0, &uvp_fire_notemplate_comfists_cog),
    C("AIFire_notemplate_comshotgun_cog_burst", AIFire, 0, &uvp_fire_notemplate_comshotgun_cog_burst),
    C("AIFire_notemplate_cog_delayed", AIFire, 0, &uvp_fire_notemplate_cog_delayed),
    C("AIFire_notemplate_cog_selected_notaiming", AIFire, 0, &uvp_fire_notemplate_cog_selected_notaiming),
    C("AIFire_notemplate_cog_selected_aiming", AIFire, 0, &uvp_fire_notemplate_cog_selected_aiming),
    C("AIFire_notemplate_cog_selected_aiming_mounting", AIFire, 0, &uvp_fire_notemplate_cog_selected_aiming_mounting),
    C("AIFire_notemplate_cog_notselected_mounting", AIFire, 0, &uvp_fire_notemplate_cog_notselected_mounting),
    C("AIFire_notemplate_cog_lead", AIFire, 0, &uvp_fire_notemplate_cog_lead),
    C("AIFire_notemplate_cog_lob", AIFire, 0, &uvp_fire_notemplate_cog_lob),
    C("AIFire_notemplate_cog_aimerror", AIFire, 0, &uvp_fire_notemplate_cog_aimerror),
    C("AIFire_notemplate_delayfire", AIFire, 0, &uvp_fire_notemplate_delayfire),
    C("AIFire_notemplate_not_in_range_delayfire", AIFire, 0, &uvp_fire_notemplate_not_in_range_delayfire),
    C("GetThingsInView_basic", ThingsInView, 0, &uvp_view_basic),
    C("GetThingsInView_fov_negative_x", ThingsInView, 0, &uvp_view_fov_negative_x),
    C("GetThingsInView_fov_negative_y", ThingsInView, 0, &uvp_view_fov_negative_y),
    C("GetThingsInView_fov_zero", ThingsInView, 0, &uvp_view_fov_zero),
    C("GetThingsInView_fov_narrow", ThingsInView, 0, &uvp_view_fov_narrow),
    C("GetThingsInView_fov_narrow_x_wide_y", ThingsInView, 0, &uvp_view_fov_narrow_x_wide_y),
    C("GetThingsInView_fov_wide_x_narrow_y", ThingsInView, 0, &uvp_view_fov_wide_x_narrow_y),
    C("GetThingsInView_fov_180", ThingsInView, 0, &uvp_view_fov_180),
    C("GetThingsInView_fov_360", ThingsInView, 0, &uvp_view_fov_360),
    C("GetThingsInView_size_zero", ThingsInView, 0, &uvp_view_size_zero),
    C("GetThingsInView_size_one", ThingsInView, 0, &uvp_view_size_one),
    C("GetThingsInView_size_two", ThingsInView, 0, &uvp_view_size_two),
    C("GetThingsInView_size_three", ThingsInView, 0, &uvp_view_size_three),
    C("GetThingsInView_size_exact", ThingsInView, 0, &uvp_view_size_exact),
    C("GetThingsInView_mask_none", ThingsInView, 0, &uvp_view_mask_none),
    C("GetThingsInView_mask_actor", ThingsInView, 0, &uvp_view_mask_actor),
    C("GetThingsInView_mask_player", ThingsInView, 0, &uvp_view_mask_player),
    C("GetThingsInView_mask_item", ThingsInView, 0, &uvp_view_mask_item),
    C("GetThingsInView_mask_all", ThingsInView, 0, &uvp_view_mask_all),
    C("GetThingsInView_maxdist_zero", ThingsInView, 0, &uvp_view_maxdist_zero),
    C("GetThingsInView_maxdist_small", ThingsInView, 0, &uvp_view_maxdist_small),
    C("GetThingsInView_maxdist_one_adjoin", ThingsInView, 0, &uvp_view_maxdist_one_adjoin),
    C("GetThingsInView_maxdist_two_adjoins", ThingsInView, 0, &uvp_view_maxdist_two_adjoins),
    C("GetThingsInView_maxdist_negative", ThingsInView, 0, &uvp_view_maxdist_negative),
    C("GetThingsInView_adjoin_notvisible", ThingsInView, 0, &uvp_view_adjoin_notvisible),
    C("GetThingsInView_adjoin_move_only", ThingsInView, 0, &uvp_view_adjoin_move_only),
    C("GetThingsInView_adjoin_visible_move", ThingsInView, 0, &uvp_view_adjoin_visible_move),
    C("GetThingsInView_adjoin_far", ThingsInView, 0, &uvp_view_adjoin_far),
    C("GetThingsInView_adjoin_far_toofar", ThingsInView, 0, &uvp_view_adjoin_far_toofar),
    C("GetThingsInView_surface_opaque", ThingsInView, 0, &uvp_view_surface_opaque),
    C("GetThingsInView_surface_translucent", ThingsInView, 0, &uvp_view_surface_translucent),
    C("GetThingsInView_surface_nonrgb", ThingsInView, 0, &uvp_view_surface_nonrgb),
    C("GetThingsInView_surface_nogeometry", ThingsInView, 0, &uvp_view_surface_nogeometry),
    C("GetThingsInView_facing_away", ThingsInView, 0, &uvp_view_facing_away),
    C("GetThingsInView_facing_side", ThingsInView, 0, &uvp_view_facing_side),
    C("GetThingsInView_facing_slightly", ThingsInView, 0, &uvp_view_facing_slightly),
    C("GetThingsInView_visited_sector1", ThingsInView, 0, &uvp_view_visited_sector1),
    C("GetThingsInView_visited_sector0", ThingsInView, 0, &uvp_view_visited_sector0),
    C("GetThingsInView_visited_sector2", ThingsInView, 0, &uvp_view_visited_sector2),
    C("GetThingsInView_start_sector1", ThingsInView, 0, &uvp_view_start_sector1),
    C("GetThingsInView_start_sector2", ThingsInView, 0, &uvp_view_start_sector2),
    C("GetThingsInView_start_sector3", ThingsInView, 0, &uvp_view_start_sector3),
    C("GetThingsInView_orient_looking_back", ThingsInView, 0, &uvp_view_orient_looking_back),
    C("GetThingsInView_orient_moved", ThingsInView, 0, &uvp_view_orient_moved),
    C("GetThingsInView_orient_moved_side", ThingsInView, 0, &uvp_view_orient_moved_side),
    C("GetThingsInView_orient_unnormalized", ThingsInView, 0, &uvp_view_orient_unnormalized),
    C("GetThingsInView_orient_tilted", ThingsInView, 0, &uvp_view_orient_tilted),
    C("GetThingsInView_orient_zero_vectors", ThingsInView, 0, &uvp_view_orient_zero_vectors),
    C("GetThingsInView_dense_size3", ThingsInView, 0, &uvp_view_dense_size3),
    C("GetThingsInView_dense_size6", ThingsInView, 0, &uvp_view_dense_size6),
    C("GetThingsInView_dense_size7", ThingsInView, 0, &uvp_view_dense_size7),
    C("GetThingsInView_nothings", ThingsInView, 0, &uvp_view_nothings),
    C("GetNextThingsInView_basic", ThingsInView, 0, &uvp_view_next_basic),
    C("GetNextThingsInView_distance_over_max", ThingsInView, 0, &uvp_view_next_distance_over_max),
    C("GetNextThingsInView_distance_at_max", ThingsInView, 0, &uvp_view_next_distance_at_max),
    C("GetNextThingsInView_distance_adjoin_cutoff", ThingsInView, 0, &uvp_view_next_distance_adjoin_cutoff),
    C("GetNextThingsInView_already_visited", ThingsInView, 0, &uvp_view_next_already_visited),
    C("GetNextThingsInView_visited_limit", ThingsInView, 0, &uvp_view_next_visited_limit),
    C("GetNextThingsInView_visited_limit_minus1", ThingsInView, 0, &uvp_view_next_visited_limit_minus1),
    C("GetNextThingsInView_visited_limit_over", ThingsInView, 0, &uvp_view_next_visited_limit_over),
    C("GetNextThingsInView_size_full_midway", ThingsInView, 0, &uvp_view_next_size_full_midway),
    C("GetNextThingsInView_size_zero", ThingsInView, 0, &uvp_view_next_size_zero),
    C("GetNextThingsInView_start_sector2", ThingsInView, 0, &uvp_view_next_start_sector2),
    C("MakeRandPoint_noflags", MakeRandPoint, 0, &uvp_pa_mk_noflags),
    C("MakeRandPoint_noflags_dirz", MakeRandPoint, 0, &uvp_pa_mk_noflags_dirz),
    C("MakeRandPoint_yaw30", MakeRandPoint, 0, &uvp_pa_mk_yaw30),
    C("MakeRandPoint_yaw90", MakeRandPoint, 0, &uvp_pa_mk_yaw90),
    C("MakeRandPoint_yaw180", MakeRandPoint, 0, &uvp_pa_mk_yaw180),
    C("MakeRandPoint_yaw_negative", MakeRandPoint, 0, &uvp_pa_mk_yaw_negative),
    C("MakeRandPoint_yaw0", MakeRandPoint, 0, &uvp_pa_mk_yaw0),
    C("MakeRandPoint_yaw_dirz", MakeRandPoint, 0, &uvp_pa_mk_yaw_dirz),
    C("MakeRandPoint_pitch20", MakeRandPoint, 0, &uvp_pa_mk_pitch20),
    C("MakeRandPoint_pitch_dirz", MakeRandPoint, 0, &uvp_pa_mk_pitch_dirz),
    C("MakeRandPoint_yaw_pitch", MakeRandPoint, 0, &uvp_pa_mk_yaw_pitch),
    C("MakeRandPoint_yaw_randangle_s1", MakeRandPoint, 0, &uvp_pa_mk_yaw_randangle_s1),
    C("MakeRandPoint_yaw_randangle_s2", MakeRandPoint, 0, &uvp_pa_mk_yaw_randangle_s2),
    C("MakeRandPoint_yaw_randangle_zero", MakeRandPoint, 0, &uvp_pa_mk_yaw_randangle_zero),
    C("MakeRandPoint_edge_rand_ulp", MakeRandPoint, 0, &uvp_pa_mk_edge_rand_ulp),
    C("MakeRandPoint_pitch_randangle", MakeRandPoint, 0, &uvp_pa_mk_pitch_randangle),
    C("MakeRandPoint_yawpitch_randangle", MakeRandPoint, 0, &uvp_pa_mk_yawpitch_randangle),
    C("MakeRandPoint_yaw_randside_s1", MakeRandPoint, 0, &uvp_pa_mk_yaw_randside_s1),
    C("MakeRandPoint_yaw_randside_s2", MakeRandPoint, 0, &uvp_pa_mk_yaw_randside_s2),
    C("MakeRandPoint_yaw_randside_s3", MakeRandPoint, 0, &uvp_pa_mk_yaw_randside_s3),
    C("MakeRandPoint_yaw_randangle_randside", MakeRandPoint, 0, &uvp_pa_mk_yaw_randangle_randside),
    C("MakeRandPoint_yaw_randangle_randside_s6", MakeRandPoint, 0, &uvp_pa_mk_yaw_randangle_randside_s6),
    C("MakeRandPoint_sideways", MakeRandPoint, 0, &uvp_pa_mk_sideways),
    C("MakeRandPoint_sideways_dirz", MakeRandPoint, 0, &uvp_pa_mk_sideways_dirz),
    C("MakeRandPoint_sideways_alongmove_same", MakeRandPoint, 0, &uvp_pa_mk_sideways_alongmove_same),
    C("MakeRandPoint_sideways_alongmove_opposite", MakeRandPoint, 0, &uvp_pa_mk_sideways_alongmove_opposite),
    C("MakeRandPoint_sideways_alongmove_perp", MakeRandPoint, 0, &uvp_pa_mk_sideways_alongmove_perp),
    C("MakeRandPoint_sideways_alongmove_ignores_rand", MakeRandPoint, 0, &uvp_pa_mk_sideways_alongmove_ignores_rand),
    C("MakeRandPoint_sideways_randside_s1", MakeRandPoint, 0, &uvp_pa_mk_sideways_randside_s1),
    C("MakeRandPoint_sideways_randside_s2", MakeRandPoint, 0, &uvp_pa_mk_sideways_randside_s2),
    C("MakeRandPoint_sideways_randside_s3", MakeRandPoint, 0, &uvp_pa_mk_sideways_randside_s3),
    C("MakeRandPoint_sideways_randangle_s1", MakeRandPoint, 0, &uvp_pa_mk_sideways_randangle_s1),
    C("MakeRandPoint_sideways_randangle_s2", MakeRandPoint, 0, &uvp_pa_mk_sideways_randangle_s2),
    C("MakeRandPoint_sideways_randangle_randside", MakeRandPoint, 0, &uvp_pa_mk_sideways_randangle_randside),
    C("MakeRandPoint_sideways_yaw_wins", MakeRandPoint, 0, &uvp_pa_mk_sideways_yaw_wins),
    C("MakeRandPoint_downcos_zero", MakeRandPoint, 0, &uvp_pa_mk_downcos_zero),
    C("MakeRandPoint_downcos_tiny", MakeRandPoint, 0, &uvp_pa_mk_downcos_tiny),
    C("MakeRandPoint_downcos_negative", MakeRandPoint, 0, &uvp_pa_mk_downcos_negative),
    C("MakeRandPoint_downcos_epsilon_edge", MakeRandPoint, 0, &uvp_pa_mk_downcos_epsilon_edge),
    C("MakeRandPoint_nullpos", MakeRandPoint, 0, &uvp_pa_mk_nullpos),
    C("MakeRandPoint_nulldir", MakeRandPoint, 0, &uvp_pa_mk_nulldir),
    C("MakeRandPoint_nulldest", MakeRandPoint, 0, &uvp_pa_mk_nulldest),
    C("MakeRandPoint_towardgoal_front", MakeRandPoint, 0, &uvp_pa_mk_towardgoal_front),
    C("MakeRandPoint_towardgoal_behind", MakeRandPoint, 0, &uvp_pa_mk_towardgoal_behind),
    C("MakeRandPoint_towardgoal_behind_nonyaw", MakeRandPoint, 0, &uvp_pa_mk_towardgoal_behind_nonyaw),
    C("MakeRandPoint_towardgoal_front_nonyaw", MakeRandPoint, 0, &uvp_pa_mk_towardgoal_front_nonyaw),
    C("MakeRandPoint_towardgoal_thing", MakeRandPoint, 0, &uvp_pa_mk_towardgoal_thing),
    C("MakeRandPoint_awayfromgoal_front", MakeRandPoint, 0, &uvp_pa_mk_awayfromgoal_front),
    C("MakeRandPoint_awayfromgoal_behind", MakeRandPoint, 0, &uvp_pa_mk_awayfromgoal_behind),
    C("MakeRandPoint_awayfromgoal_front_nonyaw", MakeRandPoint, 0, &uvp_pa_mk_awayfromgoal_front_nonyaw),
    C("MakeRandPoint_awayfromgoal_behind_nonyaw", MakeRandPoint, 0, &uvp_pa_mk_awayfromgoal_behind_nonyaw),
    C("MakeRandPoint_towardmovepos_front", MakeRandPoint, 0, &uvp_pa_mk_towardmovepos_front),
    C("MakeRandPoint_towardmovepos_behind", MakeRandPoint, 0, &uvp_pa_mk_towardmovepos_behind),
    C("MakeRandPoint_refowner", MakeRandPoint, 0, &uvp_pa_mk_refowner),
    C("MakeRandPoint_refowner_nonyaw", MakeRandPoint, 0, &uvp_pa_mk_refowner_nonyaw),
    C("MakeRandPoint_awayfromflee", MakeRandPoint, 0, &uvp_pa_mk_awayfromflee),
    C("MakeRandPoint_awayfromflee_nonyaw", MakeRandPoint, 0, &uvp_pa_mk_awayfromflee_nonyaw),
    C("MakeRandPoint_targetdist_towardmovepos", MakeRandPoint, 0, &uvp_pa_mk_targetdist_towardmovepos),
    C("MakeRandPoint_targetdist_towardgoal", MakeRandPoint, 0, &uvp_pa_mk_targetdist_towardgoal),
    C("MakeRandPoint_targetdist_behind", MakeRandPoint, 0, &uvp_pa_mk_targetdist_behind),
    C("MakeRandPoint_targetdist_zero", MakeRandPoint, 0, &uvp_pa_mk_targetdist_zero),
    C("MakeRandPoint_targetdist_ignored_noref", MakeRandPoint, 0, &uvp_pa_mk_targetdist_ignored_noref),
    C("MakeRandPoint_refhome_not_ref", MakeRandPoint, 0, &uvp_pa_mk_refhome_not_ref),
    C("MakeRandPoint_rand_towardgoal_randsides", MakeRandPoint, 0, &uvp_pa_mk_rand_towardgoal_randsides),
    C("MakeRandPoint_rand_awaygoal_randsides", MakeRandPoint, 0, &uvp_pa_mk_rand_awaygoal_randsides),
    C("RetryMakeRandPoint_after_yaw_1", MakeRandPoint, 0, &uvp_pa_retry_yaw_1),
    C("RetryMakeRandPoint_after_yaw_2", MakeRandPoint, 0, &uvp_pa_retry_yaw_2),
    C("RetryMakeRandPoint_after_yaw_pitch", MakeRandPoint, 0, &uvp_pa_retry_yaw_pitch),
    C("RetryMakeRandPoint_after_pitch_only", MakeRandPoint, 0, &uvp_pa_retry_pitch_only),
    C("RetryMakeRandPoint_after_sideways_1", MakeRandPoint, 0, &uvp_pa_retry_sideways_1),
    C("RetryMakeRandPoint_after_sideways_2", MakeRandPoint, 0, &uvp_pa_retry_sideways_2),
    C("RetryMakeRandPoint_after_sideways_randangle", MakeRandPoint, 0, &uvp_pa_retry_sideways_randangle),
    C("RetryMakeRandPoint_after_noretry_targetdist", MakeRandPoint, 0, &uvp_pa_retry_noretry_targetdist),
    C("RetryMakeRandPoint_after_noretry_towardmovepos", MakeRandPoint, 0, &uvp_pa_retry_noretry_towardmovepos),
    C("RetryMakeRandPoint_after_towardgoal_ok", MakeRandPoint, 0, &uvp_pa_retry_towardgoal_ok),
    C("RetryMakeRandPoint_after_after_invalid_point", MakeRandPoint, 0, &uvp_pa_retry_after_invalid_point),
    C("RetryMakeRandPoint_after_after_noflags", MakeRandPoint, 0, &uvp_pa_retry_after_noflags),
    C("RetryMakeRandPoint_after_other_block", MakeRandPoint, 0, &uvp_pa_retry_other_block),
    C("RetryMakeRandPoint_after_yaw_zero_angle", MakeRandPoint, 0, &uvp_pa_retry_yaw_zero_angle),
    C("RetryMakeRandPoint_after_3", MakeRandPoint, 0, &uvp_pa_retry_3),
    C("RetryMakeRandPoint_nostate", RetryAlone, 0, &uvp_pa_ra_nostate),
    C("RetryMakeRandPoint_nostate_nulllocal", RetryAlone, 0, &uvp_pa_ra_nostate_nulllocal),
    C("RetryMakeRandPoint_stale_otherlocal", RetryAlone, 1, &uvp_pa_ra_stale_otherlocal),
    C("RetryMakeRandPoint_stale_nulllocal", RetryAlone, 2, &uvp_pa_ra_stale_nulllocal),
    C("RetryMakeRandPoint_stale_yaw", RetryAlone, 2, &uvp_pa_ra_stale_yaw),
    C("RetryMakeRandPoint_stale_yawpitch", RetryAlone, 2, &uvp_pa_ra_stale_yawpitch),
    C("RetryMakeRandPoint_stale_sideways", RetryAlone, 2, &uvp_pa_ra_stale_sideways),
    C("RetryMakeRandPoint_stale_noretry", RetryAlone, 2, &uvp_pa_ra_stale_noretry),
    C("RetryMakeRandPoint_stale_noretry_movepos", RetryAlone, 2, &uvp_pa_ra_stale_noretry_movepos),
    C("RetryMakeRandPoint_stale_otherflags", RetryAlone, 2, &uvp_pa_ra_stale_otherflags),
    C("RetryMakeRandPoint_nullpos", RetryAlone, 2, &uvp_pa_ra_nullpos),
    C("RetryMakeRandPoint_nulldir", RetryAlone, 2, &uvp_pa_ra_nulldir),
    C("RetryMakeRandPoint_nulldest", RetryAlone, 2, &uvp_pa_ra_nulldest),
    C("RetryMakeRandPoint_downcos", RetryAlone, 2, &uvp_pa_ra_downcos),
    C("RetryMakeRandPoint_downcos_neg", RetryAlone, 2, &uvp_pa_ra_downcos_neg),
    C("CheckPathToPos_neither", CheckPathToPos, 0, &uvp_pa_cp_neither),
    C("CheckPathToPos_nulldir", CheckPathToPos, 0, &uvp_pa_cp_nulldir),
    C("CheckPathToPos_straight_clear", CheckPathToPos, 0, &uvp_pa_cp_straight_clear),
    C("CheckPathToPos_straight_clear_noend", CheckPathToPos, 0, &uvp_pa_cp_straight_clear_noend),
    C("CheckPathToPos_straight_clear_radius", CheckPathToPos, 0, &uvp_pa_cp_straight_clear_radius),
    C("CheckPathToPos_straight_clear_dir", CheckPathToPos, 0, &uvp_pa_cp_straight_clear_dir),
    C("CheckPathToPos_straight_wall_near", CheckPathToPos, 0, &uvp_pa_cp_straight_wall_near),
    C("CheckPathToPos_straight_wall_at_min", CheckPathToPos, 0, &uvp_pa_cp_straight_wall_at_min),
    C("CheckPathToPos_straight_wall_far", CheckPathToPos, 0, &uvp_pa_cp_straight_wall_far),
    C("CheckPathToPos_straight_wall_far_noend", CheckPathToPos, 0, &uvp_pa_cp_straight_wall_far_noend),
    C("CheckPathToPos_straight_thing", CheckPathToPos, 0, &uvp_pa_cp_straight_thing),
    C("CheckPathToPos_straight_thing_near", CheckPathToPos, 0, &uvp_pa_cp_straight_thing_near),
    C("CheckPathToPos_straight_player_blocks", CheckPathToPos, 0, &uvp_pa_cp_straight_player_blocks),
    C("CheckPathToPos_straight_player_ignored", CheckPathToPos, 0, &uvp_pa_cp_straight_player_ignored),
    C("CheckPathToPos_straight_actor_not_ignored_as_player", CheckPathToPos, 0, &uvp_pa_cp_straight_actor_not_ignored_as_player),
    C("CheckPathToPos_straight_ignorethings", CheckPathToPos, 0, &uvp_pa_cp_straight_ignorethings),
    C("CheckPathToPos_straight_ignorethings_then_wall", CheckPathToPos, 0, &uvp_pa_cp_straight_ignorethings_then_wall),
    C("CheckPathToPos_straight_floor_blocks", CheckPathToPos, 0, &uvp_pa_cp_straight_floor_blocks),
    C("CheckPathToPos_straight_floor_ignored", CheckPathToPos, 0, &uvp_pa_cp_straight_floor_ignored),
    C("CheckPathToPos_straight_wall_not_floor_ignored", CheckPathToPos, 0, &uvp_pa_cp_straight_wall_not_floor_ignored),
    C("CheckPathToPos_straight_floor_then_wall", CheckPathToPos, 0, &uvp_pa_cp_straight_floor_then_wall),
    C("CheckPathToPos_straight_all_ignored", CheckPathToPos, 0, &uvp_pa_cp_straight_all_ignored),
    C("CheckPathToPos_straight_adjoin_world", CheckPathToPos, 0, &uvp_pa_cp_straight_adjoin_world),
    C("CheckPathToPos_straight_adjoin_touch", CheckPathToPos, 0, &uvp_pa_cp_straight_adjoin_touch),
    C("CheckPathToPos_goalvisible_ok", CheckPathToPos, 0, &uvp_pa_cp_goalvisible_ok),
    C("CheckPathToPos_goalvisible_hidden", CheckPathToPos, 0, &uvp_pa_cp_goalvisible_hidden),
    C("CheckPathToPos_goalvisible_nogoal", CheckPathToPos, 0, &uvp_pa_cp_goalvisible_nogoal),
    C("CheckPathToPos_goalvisible_noflag", CheckPathToPos, 0, &uvp_pa_cp_goalvisible_noflag),
    C("CheckPathToPos_targetvisible_ok", CheckPathToPos, 0, &uvp_pa_cp_targetvisible_ok),
    C("CheckPathToPos_targetvisible_hidden", CheckPathToPos, 0, &uvp_pa_cp_targetvisible_hidden),
    C("CheckPathToPos_targetvisible_notarget", CheckPathToPos, 0, &uvp_pa_cp_targetvisible_notarget),
    C("CheckPathToPos_both_visible_target_wins", CheckPathToPos, 0, &uvp_pa_cp_both_visible_target_wins),
    C("CheckPathToPos_both_visible_goal_only", CheckPathToPos, 0, &uvp_pa_cp_both_visible_goal_only),
    C("CheckPathToPos_visible_toofar", CheckPathToPos, 0, &uvp_pa_cp_visible_toofar),
    C("CheckPathToPos_visible_but_wall_blocks", CheckPathToPos, 0, &uvp_pa_cp_visible_but_wall_blocks),
    C("CheckPathToPos_sides_clear", CheckPathToPos, 0, &uvp_pa_cp_sides_clear),
    C("CheckPathToPos_sides_clear_noend", CheckPathToPos, 0, &uvp_pa_cp_sides_clear_noend),
    C("CheckPathToPos_sides_movedist_equal", CheckPathToPos, 0, &uvp_pa_cp_sides_movedist_equal),
    C("CheckPathToPos_sides_dir_parallel_up", CheckPathToPos, 0, &uvp_pa_cp_sides_dir_parallel_up),
    C("CheckPathToPos_sides_dir_parallel_down", CheckPathToPos, 0, &uvp_pa_cp_sides_dir_parallel_down),
    C("CheckPathToPos_sides_dir_parallel_up_rvec", CheckPathToPos, 0, &uvp_pa_cp_sides_dir_parallel_up_rvec),
    C("CheckPathToPos_sides_dir_tiny_cross", CheckPathToPos, 0, &uvp_pa_cp_sides_dir_tiny_cross),
    C("CheckPathToPos_sides_dir_diagonal", CheckPathToPos, 0, &uvp_pa_cp_sides_dir_diagonal),
    C("CheckPathToPos_sides_nosector", CheckPathToPos, 0, &uvp_pa_cp_sides_nosector),
    C("CheckPathToPos_sides_first_hit_wall", CheckPathToPos, 0, &uvp_pa_cp_sides_first_hit_wall),
    C("CheckPathToPos_sides_first_hit_thing", CheckPathToPos, 0, &uvp_pa_cp_sides_first_hit_thing),
    C("CheckPathToPos_sides_first_hit_floor", CheckPathToPos, 0, &uvp_pa_cp_sides_first_hit_floor),
    C("CheckPathToPos_sides_first_hit_adjoin", CheckPathToPos, 0, &uvp_pa_cp_sides_first_hit_adjoin),
    C("CheckPathToPos_sides_both_walls", CheckPathToPos, 0, &uvp_pa_cp_sides_both_walls),
    C("CheckPathToPos_sides_goal_visible_both", CheckPathToPos, 0, &uvp_pa_cp_sides_goal_visible_both),
    C("CheckPathToPos_sides_goal_visible_none", CheckPathToPos, 0, &uvp_pa_cp_sides_goal_visible_none),
    C("CheckPathToPos_sides_goal_visible_none_required", CheckPathToPos, 0, &uvp_pa_cp_sides_goal_visible_none_required),
    C("CheckPathToPos_sides_goal_visible_one", CheckPathToPos, 0, &uvp_pa_cp_sides_goal_visible_one),
    C("CheckPathToPos_sides_goal_visible_other", CheckPathToPos, 0, &uvp_pa_cp_sides_goal_visible_other),
    C("CheckPathToPos_sides_prefer_goal", CheckPathToPos, 0, &uvp_pa_cp_sides_prefer_goal),
    C("CheckPathToPos_sides_avoid_goal", CheckPathToPos, 0, &uvp_pa_cp_sides_avoid_goal),
    C("CheckPathToPos_sides_prefer_and_avoid", CheckPathToPos, 0, &uvp_pa_cp_sides_prefer_and_avoid),
    C("CheckPathToPos_sides_prefer_goal_far_side", CheckPathToPos, 0, &uvp_pa_cp_sides_prefer_goal_far_side),
    C("CheckPathToPos_sides_avoid_goal_far_side", CheckPathToPos, 0, &uvp_pa_cp_sides_avoid_goal_far_side),
    C("CheckPathToPos_sides_prefer_goal_equidistant", CheckPathToPos, 0, &uvp_pa_cp_sides_prefer_goal_equidistant),
    C("CheckPathToPos_sides_avoid_goal_equidistant", CheckPathToPos, 0, &uvp_pa_cp_sides_avoid_goal_equidistant),
    C("CheckPathToPos_sides_prefer_goal_nogoal", CheckPathToPos, 0, &uvp_pa_cp_sides_prefer_goal_nogoal),
    C("CheckPathToPos_sides_both_flags_straight_wins", CheckPathToPos, 0, &uvp_pa_cp_sides_both_flags_straight_wins),
    C("SweepMoveToPathPos_noiter", SweepMoveToPathPos, 0, &uvp_pa_sw_noiter),
    C("SweepMoveToPathPos_blocked_first", SweepMoveToPathPos, 0, &uvp_pa_sw_blocked_first),
    C("SweepMoveToPathPos_blocked_first_angle30", SweepMoveToPathPos, 0, &uvp_pa_sw_blocked_first_angle30),
    C("SweepMoveToPathPos_clear_all", SweepMoveToPathPos, 0, &uvp_pa_sw_clear_all),
    C("SweepMoveToPathPos_clear_all_angle45", SweepMoveToPathPos, 0, &uvp_pa_sw_clear_all_angle45),
    C("SweepMoveToPathPos_pos_at_owner", SweepMoveToPathPos, 0, &uvp_pa_sw_pos_at_owner),
    C("SweepMoveToPathPos_pos_above_owner", SweepMoveToPathPos, 0, &uvp_pa_sw_pos_above_owner),
    C("SweepMoveToPathPos_blocked_retry_free", SweepMoveToPathPos, 0, &uvp_pa_sw_blocked_retry_free),
    C("SweepMoveToPathPos_flags_masked", SweepMoveToPathPos, 0, &uvp_pa_sw_flags_masked),
    C("SweepMoveToPathPos_flags_pitch", SweepMoveToPathPos, 0, &uvp_pa_sw_flags_pitch),
    C("SweepMoveToPathPos_towardgoal", SweepMoveToPathPos, 0, &uvp_pa_sw_towardgoal),
    C("SweepMoveToPathPos_zero_delta_limit", SweepMoveToPathPos, 0, &uvp_pa_sw_zero_delta_limit),
    C("SweepMoveToPathPos_blocked_second_iteration", SweepMoveToPathPos, 0, &uvp_pa_sw_blocked_second_iteration),
    C("SweepMoveToPathPos_angle_180_no_retry", SweepMoveToPathPos, 0, &uvp_pa_sw_angle_180_no_retry),
    C("SweepMoveToPathPos_blocked_at_180", SweepMoveToPathPos, 0, &uvp_pa_sw_blocked_at_180),
    C("MakeRoamPos_clear", MakeRoamPos, 0, &uvp_pa_ro_clear),
    C("MakeRoamPos_clear_3d", MakeRoamPos, 0, &uvp_pa_ro_clear_3d),
    C("MakeRoamPos_blocked_once", MakeRoamPos, 0, &uvp_pa_ro_blocked_once),
    C("MakeRoamPos_edge_rand_ulp", MakeRoamPos, 0, &uvp_pa_ro_edge_rand_ulp),
    C("MakeRoamPos_blocked_twice", MakeRoamPos, 0, &uvp_pa_ro_blocked_twice),
    C("MakeRoamPos_movedist_zero", MakeRoamPos, 0, &uvp_pa_ro_movedist_zero),
    C("MakeRoamPos_maxretries_zero", MakeRoamPos, 0, &uvp_pa_ro_maxretries_zero),
    C("MakeRoamPos_maxretries_negative", MakeRoamPos, 0, &uvp_pa_ro_maxretries_negative),
    C("MakeRoamPos_floor_ignored_in_2d", MakeRoamPos, 0, &uvp_pa_ro_floor_ignored_in_2d),
    C("MakeRoamPos_floor_2d", MakeRoamPos, 0, &uvp_pa_ro_floor_2d),
    C("MakeRoamPos_floor_3d", MakeRoamPos, 0, &uvp_pa_ro_floor_3d),
    C("MakeRoamPos_thing_blocked_all", MakeRoamPos, 0, &uvp_pa_ro_thing_blocked_all),
    C("MakeRoamPos_thing_blocked_all_retries3", MakeRoamPos, 0, &uvp_pa_ro_thing_blocked_all_retries3),
    C("MakeRoamPos_seed_variation", MakeRoamPos, 0, &uvp_pa_ro_seed_variation),
    C("MakePathPos_noflags", MakePathPos, 0, &uvp_pa_mp_noflags),
    C("MakePathPos_yaw_clear", MakePathPos, 0, &uvp_pa_mp_yaw_clear),
    C("MakePathPos_yaw_pitch_clear", MakePathPos, 0, &uvp_pa_mp_yaw_pitch_clear),
    C("MakePathPos_pitch_clear", MakePathPos, 0, &uvp_pa_mp_pitch_clear),
    C("MakePathPos_yaw_blocked_retry_ok", MakePathPos, 0, &uvp_pa_mp_yaw_blocked_retry_ok),
    C("MakePathPos_yaw_blocked_both", MakePathPos, 0, &uvp_pa_mp_yaw_blocked_both),
    C("MakePathPos_yaw_blocked_noretry_flags", MakePathPos, 0, &uvp_pa_mp_yaw_blocked_noretry_flags),
    C("MakePathPos_sideways_blocked_retry_ok", MakePathPos, 0, &uvp_pa_mp_sideways_blocked_retry_ok),
    C("MakePathPos_sideways_blocked_both", MakePathPos, 0, &uvp_pa_mp_sideways_blocked_both),
    C("MakePathPos_pathflags_ignoreplayer", MakePathPos, 0, &uvp_pa_mp_pathflags_ignoreplayer),
    C("MakePathPos_pathflags_ignorethings", MakePathPos, 0, &uvp_pa_mp_pathflags_ignorethings),
    C("MakePathPos_pathflags_thing_blocks", MakePathPos, 0, &uvp_pa_mp_pathflags_thing_blocks),
    C("MakePathPos_pathflags_floor_2d_ignored", MakePathPos, 0, &uvp_pa_mp_pathflags_floor_2d_ignored),
    C("MakePathPos_pathflags_floor_3d_blocks", MakePathPos, 0, &uvp_pa_mp_pathflags_floor_3d_blocks),
    C("MakePathPos_goalvisible", MakePathPos, 0, &uvp_pa_mp_goalvisible),
    C("MakePathPos_goalvisible_ok", MakePathPos, 0, &uvp_pa_mp_goalvisible_ok),
    C("MakePathPos_targetvisible", MakePathPos, 0, &uvp_pa_mp_targetvisible),
    C("MakePathPos_other_pathflags_dropped", MakePathPos, 0, &uvp_pa_mp_other_pathflags_dropped),
    C("MakePathPos_downcos_invalid", MakePathPos, 0, &uvp_pa_mp_downcos_invalid),
    C("MakePathPos_randangle_seed", MakePathPos, 0, &uvp_pa_mp_randangle_seed),
    C("MakePathPos_towardgoal", MakePathPos, 0, &uvp_pa_mp_towardgoal),
    C("AIResetWaypoints_flagoff", ResetWaypoints, 0, NULL),
    C("AIResetWaypoints_renderon", ResetWaypoints, 1, NULL),
};

#undef C

static const SithVecSuite sithAIUtilVec_suite = {
    "sithAIUtil",
    sithAIUtilVec_aCases,
    SITHVEC_ARRAYLEN(sithAIUtilVec_aCases),
    sithAIUtilVec_Build,
    sithAIUtilVec_Watch,
    sithAIUtilVec_Sync,
};

#define SITHVEC_SUITE sithAIUtilVec_suite

#endif // SITH_AI_TESTS_SITHAIUTILVEC_H
