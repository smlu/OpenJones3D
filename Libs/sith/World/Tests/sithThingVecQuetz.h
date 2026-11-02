// Cases of the sithThing Quetz tail vectors (sithThing_CreateQuetzUserBlock, sithThing_UpdateQuetzUserBlock),
// shared by the capture harness and sithThingQuetzVecTests. See tv/sithThingQuetz/README.md.
//
// A case builds a Quetz head (thing 0) with a chain of attached things (tail segments and other attachments) and
// either calls sithThing_CreateQuetzUserBlock once or runs a script of frames: before every frame the head is turned
// and moved, its velocity, distanceMoved and bResetSegment are set and the stubs are configured, then
// sithThing_UpdateQuetzUserBlock is called and a checkpoint records the calls and state changes of that frame.
#ifndef SITH_WORLD_TESTS_SITHTHINGVECQUETZ_H
#define SITH_WORLD_TESTS_SITHTHINGVECQUETZ_H

#include "sithVecWorld.h"

#include <math.h>

#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithThing.h>

// File statics of sithThing.c the Quetz functions keep between frames. The harness reaches them at their v1.0
// addresses (sithThing_bQuetzTurningSharp is an int there), the tests through sithThingVecTested.c.
#ifdef SITHVEC_HARNESS
#define SITHTHINGVEC_TURNINGSHARP (*(int*)sithThing_bQuetzTurningSharp_ADDR)
#define SITHTHINGVEC_PREVINFO     (*(SithQuetzAttachInfo**)sithThing_prevQuetzAttachInfo_ADDR)
#define SITHTHINGVEC_CURINFO      (*(SithQuetzAttachInfo**)sithThing_curQuetzAttachInfo_ADDR)
#else
bool* sithThingVecTest_GetQuetzTurningSharp(void);
SithQuetzAttachInfo** sithThingVecTest_GetPrevQuetzAttachInfo(void);
SithQuetzAttachInfo** sithThingVecTest_GetCurQuetzAttachInfo(void);
#define SITHTHINGVEC_TURNINGSHARP (*sithThingVecTest_GetQuetzTurningSharp())
#define SITHTHINGVEC_PREVINFO     (*sithThingVecTest_GetPrevQuetzAttachInfo())
#define SITHTHINGVEC_CURINFO      (*sithThingVecTest_GetCurQuetzAttachInfo())
#endif

#define SITHTHINGVEC_HEAD      0u
#define SITHTHINGVEC_MAXCHAIN  (SITHVEC_NUMTHINGS - 1u) // things 1.. can be attached to the head
#define SITHTHINGVEC_SEGSPACE  0.09f

// Head velocity relative to its orientation
typedef enum
{
    SITHTHINGVEC_VEL_FORWARD = 0, // along lvec: the full distance counts
    SITHTHINGVEC_VEL_ZERO,        // zero vector: Normalize3 keeps it, nothing counts
    SITHTHINGVEC_VEL_BACKWARD,    // against lvec: the dot is clamped to 0
    SITHTHINGVEC_VEL_SIDEWAYS,    // along rvec: dot 0
    SITHTHINGVEC_VEL_DIAGONAL,    // lvec + rvec: dot cos 45
    SITHTHINGVEC_VEL_UP,          // straight up: dot 0 for an unpitched head
} SithThingVecVelMode;

// Changes of the attachment chain before a frame
typedef enum
{
    SITHTHINGVEC_OP_NONE = 0,
    SITHTHINGVEC_OP_DETACHLAST, // unlinks the last attached thing from the chain
    SITHTHINGVEC_OP_UNTAIL1,    // clears the tail flag of the second tail segment
    SITHTHINGVEC_OP_RETAIL1,    // sets it again
    SITHTHINGVEC_OP_TAILALL,    // sets the tail flag on every attached thing (more segments than the block knows)
    SITHTHINGVEC_OP_SNAPSEG0,   // puts the first segment exactly where the previous frame wanted it to be
} SithThingVecOp;

// How the head's user block exists before the first frame
typedef enum
{
    SITHTHINGVEC_BLOCK_NONE = 0,  // NULL: the first sithThing_UpdateQuetzUserBlock creates it
    SITHTHINGVEC_BLOCK_CREATE,    // sithThing_CreateQuetzUserBlock is called first (own checkpoint)
    SITHTHINGVEC_BLOCK_ALLOCFAIL, // NULL and stdMemory_Malloc fails
    SITHTHINGVEC_BLOCK_PRESET,    // an existing block with random segment state (lengths, directions, rotations)
} SithThingVecBlockMode;

typedef struct
{
    int8_t hx, hy, hz;    // new head heading (normalized); all 0 keeps the heading
    float dist;           // distanceMoved; the head moves this far along its heading before the frame
    uint8_t velMode;      // SithThingVecVelMode
    uint8_t bReset;       // sets bResetSegment before the frame
    uint8_t bSectorStep;  // sithCollision_FindSectorInRadius returns the next sector (SetSector moves the segment)
    uint8_t moveMode;     // sithCollision_MoveThing: 0 moves the full distance, 1 half of it, 2 not at all
    uint8_t op;           // SithThingVecOp
} SithThingVecFrame;

typedef struct
{
    uint8_t numTail;          // tail segments
    uint8_t nonTailMask;      // other attached things: 1 before the first segment, 2 after the first, 4 after the last
    uint8_t bSharpInit;       // sithThing_bQuetzTurningSharp before the first frame
    uint8_t blockMode;        // SithThingVecBlockMode
    int8_t hx, hy, hz;        // initial heading of the head and the segments
    float fps;                // sithTime_g_fps
    uint8_t numFrames;
    const SithThingVecFrame* aFrames;
} SithThingVecQuetzParams;

// Mirrors of the file statics, watched with the arena
static uint32_t sithThingVec_aMirror[3];

static void sithThingVec_Sync(void)
{
    sithThingVec_aMirror[0] = SITHTHINGVEC_TURNINGSHARP ? 1u : 0u;
    sithThingVec_aMirror[1] = (uint32_t)(uintptr_t)SITHTHINGVEC_PREVINFO;
    sithThingVec_aMirror[2] = (uint32_t)(uintptr_t)SITHTHINGVEC_CURINFO;
}

static void sithThingVec_Watch(void)
{
    SITHVEC_WATCH(sithThingVec_aMirror);

    // sithThing_UpdateQuetzUserBlock copies its local orientation into the segment and only sets rvec, lvec and uvec
    // of it: dvec is an uninitialized stack value on both sides (and unused)
    for ( size_t i = 0; i < SITHVEC_CAPTHINGS; ++i )
    {
        sithVec_Ignore(&sithVec_arena.things[i].orient.dvec, sizeof(rdVector3));
    }
}

// Orientation with the given heading (exactly the same float operations on both sides: sqrt and division only)
static void sithThingVec_SetHeading(SithThing* pThing, int hx, int hy, int hz)
{
    float len = sqrtf((float)(hx * hx + hy * hy + hz * hz));
    rdVector3* l = &pThing->orient.lvec;
    l->x = (float)hx / len;
    l->y = (float)hy / len;
    l->z = (float)hz / len;

    float len2 = sqrtf((float)(hx * hx + hy * hy));
    rdVector3* r = &pThing->orient.rvec;
    r->x = (float)hy / len2;
    r->y = -(float)hx / len2;
    r->z = 0.0f;

    rdVector3* u = &pThing->orient.uvec;
    u->x = r->y * l->z - r->z * l->y;
    u->y = r->z * l->x - r->x * l->z;
    u->z = r->x * l->y - r->y * l->x;
}

static SithThing* sithThingVec_Head(void)
{
    return &sithVec_arena.things[SITHTHINGVEC_HEAD];
}

static void sithThingVec_Build(const SithVecCase* pCase)
{
    const SithThingVecQuetzParams* pParams = (const SithThingVecQuetzParams*)pCase->pParams;
    SithThing* pHead = sithVec_NewThing(SITHTHINGVEC_HEAD, SITH_THING_ACTOR, SITH_MT_PHYSICS, 0);
    sithThingVec_SetHeading(pHead, pParams->hx, pParams->hy, pParams->hz);
    pHead->pos.x = 64.0f;
    pHead->pos.y = 96.0f;
    pHead->pos.z = 32.0f;

    // Chain: optional other attachment, segment 0, optional other attachment, segments 1.., optional attachment
    SithThing* apChain[SITHTHINGVEC_MAXCHAIN];
    unsigned int numChain = 0;
    unsigned int thingNum = 1;
    for ( unsigned int seg = 0; seg <= pParams->numTail; ++seg )
    {
        int bOther = (seg == 0 && (pParams->nonTailMask & 1)) || (seg == 1 && (pParams->nonTailMask & 2)) || (seg == pParams->numTail && (pParams->nonTailMask & 4));
        if ( bOther )
        {
            SithThing* pOther = sithVec_NewThing(thingNum++, SITH_THING_ITEM, SITH_MT_NONE, 1);
            pOther->attach.flags     = SITH_ATTACH_THING;
            pOther->attach.posOffset.z = 0.25f;
            pOther->pos              = pHead->pos;
            apChain[numChain++]      = pOther;
        }

        if ( seg == pParams->numTail )
        {
            break;
        }

        SithThing* pSeg = sithVec_NewThing(thingNum++, SITH_THING_ACTOR, SITH_MT_PHYSICS, seg % 3u);
        pSeg->orient           = pHead->orient;
        pSeg->attach.flags     = SITH_ATTACH_TAIL;
        pSeg->attach.posOffset.x = 0.0f;
        pSeg->attach.posOffset.y = -SITHTHINGVEC_SEGSPACE * (float)(seg + 1);
        pSeg->attach.posOffset.z = -(float)seg / 64.0f;
        pSeg->pos.x            = pHead->pos.x - pHead->orient.lvec.x * SITHTHINGVEC_SEGSPACE * (float)(seg + 1);
        pSeg->pos.y            = pHead->pos.y - pHead->orient.lvec.y * SITHTHINGVEC_SEGSPACE * (float)(seg + 1);
        pSeg->pos.z            = pHead->pos.z + pSeg->attach.posOffset.z;
        apChain[numChain++]    = pSeg;
    }

    pHead->pAttachedThing = numChain ? apChain[0] : NULL;
    for ( unsigned int i = 0; i + 1 < numChain; ++i )
    {
        apChain[i]->pNextAttachedThing = apChain[i + 1];
    }

    SITHTHINGVEC_TURNINGSHARP = pParams->bSharpInit;
    SITHTHINGVEC_PREVINFO     = NULL;
    SITHTHINGVEC_CURINFO      = NULL;
    sithTime_g_fps            = pParams->fps;

    if ( pParams->blockMode == SITHTHINGVEC_BLOCK_PRESET )
    {
        // An existing block in the pool, as a long running Quetz would have it (not all values reachable)
        SithQuetzUserBlock* pBlock = (SithQuetzUserBlock*)sithVec_arena.pool;
        sithVec_arena.poolUsed     = (uint32_t)((sizeof(*pBlock) + 15u) & ~15u);
        memset(pBlock, 0, sizeof(*pBlock));
        for ( unsigned int i = 0; i < pParams->numTail; ++i )
        {
            SithQuetzAttachInfo* pInfo = &pBlock->aAttachInfos[i];
            pInfo->offsetPrevY = i ? -0.045f : 0.0f;
            pInfo->offsetY     = -0.045f;
            pInfo->offsetZ     = -1.0f / 64.0f;
            pInfo->length      = 0.0625f + (float)sithVec_RndI(16) / 64.0f;
            pInfo->remaining   = pInfo->length * (float)sithVec_RndI(17) / 16.0f;
            sithThingVec_SetHeading(pHead, sithVec_RndI(9) - 4, sithVec_RndI(9) - 4 + 9, 0); // via the head's orient
            pInfo->vecDirection = pHead->orient.lvec;
            sithThingVec_SetHeading(pHead, sithVec_RndI(9) - 4, sithVec_RndI(9) - 4 + 9, 0);
            pInfo->vecDirectionNextAttach = pHead->orient.lvec;
            pInfo->rotation.x  = (float)(sithVec_RndI(91) - 45);
            pInfo->rotation.y  = (float)(sithVec_RndI(31) - 15);
            pInfo->rotation.z  = (float)(sithVec_RndI(11) - 5);
            pInfo->uvecOffset  = (float)sithVec_RndI(9) / 64.0f;
        }

        pBlock->numAttachInfos = pParams->numTail;
        pBlock->bResetSegment  = sithVec_RndI(2);
        pHead->userblock.pQuetz = pBlock;
        sithThingVec_SetHeading(pHead, pParams->hx, pParams->hy, pParams->hz);
    }
}

static SithThing* sithThingVec_LastAttached(SithThing* pHead, SithThing** ppBefore)
{
    SithThing* pPrev = NULL;
    SithThing* pCur  = pHead->pAttachedThing;
    while ( pCur && pCur->pNextAttachedThing )
    {
        pPrev = pCur;
        pCur  = pCur->pNextAttachedThing;
    }

    *ppBefore = pPrev;
    return pCur;
}

// Segment num of the chain (the things built as segments, whether or not they have the tail flag now)
static SithThing* sithThingVec_TailSegment(SithThing* pHead, unsigned int num)
{
    for ( SithThing* pCur = pHead->pAttachedThing; pCur; pCur = pCur->pNextAttachedThing )
    {
        if ( pCur->attach.flags != SITH_ATTACH_THING && num-- == 0 )
        {
            return pCur;
        }
    }

    return NULL;
}

static void sithThingVec_ApplyFrame(const SithThingVecFrame* pFrame, const rdVector3* pSeg0PrevPos)
{
    SithThing* pHead = sithThingVec_Head();
    if ( pFrame->hx || pFrame->hy || pFrame->hz )
    {
        sithThingVec_SetHeading(pHead, pFrame->hx, pFrame->hy, pFrame->hz);
    }

    pHead->pos.x += pHead->orient.lvec.x * pFrame->dist;
    pHead->pos.y += pHead->orient.lvec.y * pFrame->dist;
    pHead->pos.z += pHead->orient.lvec.z * pFrame->dist;
    pHead->distanceMoved = pFrame->dist;

    rdVector3* pVel        = &pHead->moveInfo.physics.velocity;
    const rdMatrix34* pOri = &pHead->orient;
    switch ( pFrame->velMode )
    {
        case SITHTHINGVEC_VEL_FORWARD:
            pVel->x = pOri->lvec.x * 2.0f;
            pVel->y = pOri->lvec.y * 2.0f;
            pVel->z = pOri->lvec.z * 2.0f;
            break;

        case SITHTHINGVEC_VEL_ZERO:
            pVel->x = pVel->y = pVel->z = 0.0f;
            break;

        case SITHTHINGVEC_VEL_BACKWARD:
            pVel->x = -pOri->lvec.x;
            pVel->y = -pOri->lvec.y;
            pVel->z = -pOri->lvec.z;
            break;

        case SITHTHINGVEC_VEL_SIDEWAYS:
            *pVel = pOri->rvec;
            break;

        case SITHTHINGVEC_VEL_DIAGONAL:
            pVel->x = pOri->lvec.x + pOri->rvec.x;
            pVel->y = pOri->lvec.y + pOri->rvec.y;
            pVel->z = pOri->lvec.z + pOri->rvec.z;
            break;

        default:
            pVel->x = 0.0f;
            pVel->y = 0.0f;
            pVel->z = 3.0f;
            break;
    }

    if ( pFrame->bReset && pHead->userblock.pQuetz )
    {
        pHead->userblock.pQuetz->bResetSegment = 1;
    }

    sithVec_stubFindSectorMode = pFrame->bSectorStep;
    sithVec_stubMoveScale      = pFrame->moveMode == 0 ? 1.0f : (pFrame->moveMode == 1 ? 0.5f : 0.0f);

    SithThing* pBefore;
    SithThing* pSeg;
    switch ( pFrame->op )
    {
        case SITHTHINGVEC_OP_DETACHLAST:
            pSeg = sithThingVec_LastAttached(pHead, &pBefore);
            if ( pBefore )
            {
                pBefore->pNextAttachedThing = NULL;
            }
            else
            {
                pHead->pAttachedThing = NULL;
            }

            pSeg->attach.flags = 0;
            break;

        case SITHTHINGVEC_OP_UNTAIL1:
            sithThingVec_TailSegment(pHead, 1)->attach.flags &= ~SITH_ATTACH_TAIL;
            break;

        case SITHTHINGVEC_OP_RETAIL1:
            sithThingVec_TailSegment(pHead, 1)->attach.flags |= SITH_ATTACH_TAIL;
            break;

        case SITHTHINGVEC_OP_TAILALL:
            for ( SithThing* pCur = pHead->pAttachedThing; pCur; pCur = pCur->pNextAttachedThing )
            {
                pCur->attach.flags |= SITH_ATTACH_TAIL;
            }

            break;

        case SITHTHINGVEC_OP_SNAPSEG0:
            // moveDir of the previous frame is newPos - pos exactly (the components are close), so this is newPos
            pSeg = sithThingVec_TailSegment(pHead, 0);
            pSeg->pos.x = pSeg0PrevPos->x + pSeg->moveDir.x;
            pSeg->pos.y = pSeg0PrevPos->y + pSeg->moveDir.y;
            pSeg->pos.z = pSeg0PrevPos->z + pSeg->moveDir.z;
            break;

        default:
            break;
    }
}

// Runs the frame script of the case
static int sithThingVec_RunUpdate(SithVecCtx* pCtx)
{
    const SithThingVecQuetzParams* pParams = (const SithThingVecQuetzParams*)pCtx->pCase->pParams;
    SithThing* pHead = sithThingVec_Head();
    if ( pParams->blockMode == SITHTHINGVEC_BLOCK_ALLOCFAIL )
    {
        sithVec_stubAllocFail = 1;
    }

    sithVec_Begin(pCtx);

    int ret = 0;
    if ( pParams->blockMode == SITHTHINGVEC_BLOCK_CREATE )
    {
        ret = SITHVEC_FN(sithThing_CreateQuetzUserBlock)(pHead);
        sithVec_Checkpoint(pCtx);
    }

    rdVector3 seg0PrevPos = { 0 };
    for ( unsigned int i = 0; i < pParams->numFrames; ++i )
    {
        sithThingVec_ApplyFrame(&pParams->aFrames[i], &seg0PrevPos);
        SithThing* pSeg0 = sithThingVec_TailSegment(pHead, 0);
        if ( pSeg0 )
        {
            seg0PrevPos = pSeg0->pos;
        }

        SITHVEC_FN(sithThing_UpdateQuetzUserBlock)(pHead);
        sithVec_Checkpoint(pCtx);
    }

    return ret;
}

// sithThing_CreateQuetzUserBlock alone; var 1: the block exists already, var 2: the allocation fails,
// var 3: the head has no attachments at all
static int sithThingVec_RunCreate(SithVecCtx* pCtx)
{
    SithThing* pHead = sithThingVec_Head();
    if ( pCtx->pCase->var == 1 )
    {
        pHead->userblock.pQuetz = (SithQuetzUserBlock*)&sithVec_arena.user[16];
    }
    else if ( pCtx->pCase->var == 2 )
    {
        sithVec_stubAllocFail = 1;
    }
    else if ( pCtx->pCase->var == 3 )
    {
        pHead->pAttachedThing = NULL;
    }

    sithVec_Begin(pCtx);
    return SITHVEC_FN(sithThing_CreateQuetzUserBlock)(pHead);
}

// ---------------------------------------------------------------------------------------------------------------
// Frame scripts. F(hx, hy, hz, dist, vel, reset, sectorStep, moveMode, op)
// ---------------------------------------------------------------------------------------------------------------
#define F(hx, hy, hz, dist, vel, reset, step, move, op) { hx, hy, hz, dist, vel, reset, step, move, op }
#define FWD SITHTHINGVEC_VEL_FORWARD

// Straight ahead in small steps: segments count down without resets until their ratio drops below 0.3
static const SithThingVecFrame sithThingVec_aStraight[] = {
    F(0, 0, 0, 0.02f, FWD, 0, 0, 0, 0), F(0, 0, 0, 0.02f, FWD, 0, 0, 0, 0), F(0, 0, 0, 0.02f, FWD, 0, 0, 0, 0),
    F(0, 0, 0, 0.02f, FWD, 0, 0, 0, 0), F(0, 0, 0, 0.02f, FWD, 0, 0, 0, 0), F(0, 0, 0, 0.02f, FWD, 0, 0, 0, 0),
    F(0, 0, 0, 0.02f, FWD, 0, 0, 0, 0), F(0, 0, 0, 0.02f, FWD, 0, 0, 0, 0),
};

// A slow left turn: consistent small angles, below the 25 degree sharp turn limit
static const SithThingVecFrame sithThingVec_aGentle[] = {
    F(-1, 16, 0, 0.03f, FWD, 0, 0, 0, 0), F(-2, 16, 0, 0.03f, FWD, 0, 0, 0, 0), F(-3, 16, 0, 0.03f, FWD, 0, 0, 0, 0),
    F(-4, 16, 0, 0.03f, FWD, 0, 0, 0, 0), F(-5, 16, 0, 0.03f, FWD, 0, 0, 0, 0), F(-6, 16, 0, 0.03f, FWD, 0, 0, 0, 0),
    F(-7, 16, 0, 0.03f, FWD, 0, 0, 0, 0), F(-8, 16, 0, 0.03f, FWD, 0, 0, 0, 0),
};

// 45 degrees right per frame: large consistent angles turn sharp turning on (segments get longer on reset)
static const SithThingVecFrame sithThingVec_aSharp[] = {
    F(1, 1, 0, 0.05f, FWD, 0, 0, 0, 0), F(1, 0, 0, 0.05f, FWD, 0, 0, 0, 0), F(1, -1, 0, 0.05f, FWD, 0, 0, 0, 0),
    F(0, -1, 0, 0.05f, FWD, 0, 0, 0, 0), F(-1, -1, 0, 0.05f, FWD, 0, 0, 0, 0), F(-1, 0, 0, 0.05f, FWD, 0, 0, 0, 0),
    F(-1, 1, 0, 0.05f, FWD, 0, 0, 0, 0), F(0, 1, 0, 0.05f, FWD, 0, 0, 0, 0), F(1, 1, 0, 0.05f, FWD, 0, 0, 0, 0),
    F(1, 0, 0, 0.05f, FWD, 0, 0, 0, 0),
};

// A U-turn in three frames, then straight on
static const SithThingVecFrame sithThingVec_aUTurn[] = {
    F(2, 1, 0, 0.04f, FWD, 0, 0, 0, 0), F(1, -2, 0, 0.04f, FWD, 0, 0, 0, 0), F(-1, -8, 0, 0.04f, FWD, 0, 0, 0, 0),
    F(0, -1, 0, 0.04f, FWD, 0, 0, 0, 0), F(0, -1, 0, 0.04f, FWD, 0, 0, 0, 0), F(0, -1, 0, 0.04f, FWD, 0, 0, 0, 0),
};

// Left and right alternately: the turn direction flips along the tail (bTurnFlipped resets the next segment)
static const SithThingVecFrame sithThingVec_aZigzag[] = {
    F(-1, 1, 0, 0.06f, FWD, 0, 0, 0, 0), F(1, 1, 0, 0.06f, FWD, 0, 0, 0, 0), F(-1, 1, 0, 0.06f, FWD, 0, 0, 0, 0),
    F(1, 1, 0, 0.06f, FWD, 0, 0, 0, 0), F(-1, 1, 0, 0.06f, FWD, 0, 0, 0, 0), F(1, 1, 0, 0.06f, FWD, 0, 0, 0, 0),
    F(-1, 1, 0, 0.06f, FWD, 0, 0, 0, 0), F(1, 1, 0, 0.06f, FWD, 0, 0, 0, 0),
};

// bResetSegment before frames 2 and 4 (it only resets the first segment and is cleared)
static const SithThingVecFrame sithThingVec_aReset[] = {
    F(0, 0, 0, 0.01f, FWD, 0, 0, 0, 0), F(-1, 4, 0, 0.01f, FWD, 1, 0, 0, 0), F(-1, 4, 0, 0.01f, FWD, 0, 0, 0, 0),
    F(1, 4, 0, 0.01f, FWD, 1, 0, 0, 0), F(1, 4, 0, 0.01f, FWD, 0, 0, 0, 0),
};

// Steps longer than a segment: every ratio clamps to 0 and every segment resets each frame
static const SithThingVecFrame sithThingVec_aBigSteps[] = {
    F(0, 0, 0, 1.0f, FWD, 0, 0, 0, 0), F(1, 3, 0, 1.0f, FWD, 0, 0, 0, 0), F(3, 1, 0, 1.0f, FWD, 0, 0, 0, 0),
    F(3, -1, 0, 1.0f, FWD, 0, 0, 0, 0),
};

// Velocity directions: backwards, zero, sideways, up (dot 0 or clamped: nothing counts) and diagonal (cos 45)
static const SithThingVecFrame sithThingVec_aVelocities[] = {
    F(0, 0, 0, 0.05f, SITHTHINGVEC_VEL_BACKWARD, 0, 0, 0, 0), F(0, 0, 0, 0.05f, SITHTHINGVEC_VEL_ZERO, 0, 0, 0, 0),
    F(0, 0, 0, 0.05f, SITHTHINGVEC_VEL_SIDEWAYS, 0, 0, 0, 0), F(0, 0, 0, 0.05f, SITHTHINGVEC_VEL_UP, 0, 0, 0, 0),
    F(0, 0, 0, 0.05f, SITHTHINGVEC_VEL_DIAGONAL, 0, 0, 0, 0), F(0, 0, 0, 0.05f, SITHTHINGVEC_VEL_DIAGONAL, 0, 0, 0, 0),
};

// distanceMoved below zero: remaining grows, the ratio clamps to 1
static const SithThingVecFrame sithThingVec_aNegative[] = {
    F(0, 0, 0, -0.05f, FWD, 0, 0, 0, 0), F(-1, 2, 0, -0.05f, FWD, 0, 0, 0, 0), F(0, 0, 0, -1.0f, FWD, 0, 0, 0, 0),
};

// Segment changes sector on every move and the collision move is half/blocked/full
static const SithThingVecFrame sithThingVec_aSectors[] = {
    F(0, 0, 0, 0.03f, FWD, 0, 1, 0, 0), F(-1, 3, 0, 0.03f, FWD, 0, 1, 1, 0), F(-1, 2, 0, 0.03f, FWD, 0, 0, 2, 0),
    F(-1, 1, 0, 0.03f, FWD, 0, 1, 2, 0), F(0, 1, 0, 0.03f, FWD, 0, 1, 0, 0),
};

// Attachment changes: the second segment loses and regains its tail flag, the last thing is detached
static const SithThingVecFrame sithThingVec_aDetach[] = {
    F(0, 0, 0, 0.03f, FWD, 0, 0, 0, 0), F(-1, 4, 0, 0.03f, FWD, 0, 0, 0, SITHTHINGVEC_OP_UNTAIL1),
    F(-1, 3, 0, 0.03f, FWD, 0, 0, 0, 0), F(-1, 2, 0, 0.03f, FWD, 0, 0, 0, SITHTHINGVEC_OP_RETAIL1),
    F(-1, 1, 0, 0.03f, FWD, 0, 0, 0, SITHTHINGVEC_OP_DETACHLAST), F(0, 1, 0, 0.03f, FWD, 0, 0, 0, 0),
};

// The other attached things become tail segments: the block has no state for the extra segments (length 0, the
// ratio of a moving Quetz is -inf and clamps to 0, which resets the segment)
static const SithThingVecFrame sithThingVec_aLateTail[] = {
    F(0, 0, 0, 0.03f, FWD, 0, 0, 0, 0), F(0, 0, 0, 0.03f, FWD, 0, 0, 0, SITHTHINGVEC_OP_TAILALL),
    F(-1, 3, 0, 0.03f, FWD, 0, 0, 0, 0), F(-1, 2, 0, 0.03f, FWD, 0, 0, 0, 0),
};

// The head stands still for two frames and the first segment is put exactly where the first frame wanted it: the
// second frame finds no movement for it (moveDist == 0, no sector search and no collision move)
static const SithThingVecFrame sithThingVec_aStill[] = {
    F(0, 0, 0, 0.0f, FWD, 0, 0, 2, 0), F(0, 0, 0, 0.0f, FWD, 0, 0, 0, SITHTHINGVEC_OP_SNAPSEG0),
    F(0, 0, 0, 0.0f, FWD, 0, 0, 0, 0),
};

// Pitched head (lvec with z): the segment directions are flattened, offsets follow the pitched orientation
static const SithThingVecFrame sithThingVec_aPitched[] = {
    F(0, 4, 1, 0.03f, FWD, 0, 0, 0, 0), F(-1, 4, 2, 0.03f, FWD, 0, 0, 0, 0), F(-2, 4, -2, 0.03f, FWD, 0, 0, 0, 0),
    F(0, 0, 0, 0.03f, SITHTHINGVEC_VEL_UP, 0, 0, 0, 0),
};

// Frames for the allocation failure and other short runs
static const SithThingVecFrame sithThingVec_aShort[] = {
    F(0, 0, 0, 0.03f, FWD, 0, 0, 0, 0), F(-1, 2, 0, 0.03f, FWD, 1, 0, 0, 0),
};

#undef F
#undef FWD

#define SITHTHINGVEC_SCRIPT(a) (uint8_t)SITHVEC_ARRAYLEN(a), a
#define P(name, numTail, nonTail, sharp, block, hx, hy, hz, fps, script) \
    static const SithThingVecQuetzParams sithThingVec_p##name = { numTail, nonTail, sharp, block, hx, hy, hz, fps, SITHTHINGVEC_SCRIPT(script) };

// Creation only (numFrames 0)
static const SithThingVecQuetzParams sithThingVec_pCreate0 = { 0, 0, 0, 0, 0, 1, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreate0Other = { 0, 1, 0, 0, 0, 1, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreate1 = { 1, 0, 0, 0, 0, 1, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreate2 = { 2, 0, 0, 0, 1, 2, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreate3 = { 3, 0, 0, 0, -3, 4, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreate4 = { 4, 0, 0, 0, 0, 1, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreate8Other = { 8, 7, 0, 0, 5, -2, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreate25 = { 25, 0, 0, 0, 0, 1, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreate26 = { 26, 0, 0, 0, 0, 1, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreate27 = { 27, 0, 0, 0, 0, 1, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreate34 = { 34, 0, 0, 0, 1, 1, 0, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreatePitched = { 3, 0, 0, 0, 1, 2, 3, 30.0f, 0, NULL };
static const SithThingVecQuetzParams sithThingVec_pCreateVertical = { 3, 0, 0, 0, 0, 0, 1, 30.0f, 0, NULL };

P(Straight1, 1, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aStraight)
P(Straight3, 3, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aStraight)
P(Straight8, 8, 0, 0, SITHTHINGVEC_BLOCK_CREATE, 1, 1, 0, 30.0f, sithThingVec_aStraight)
P(Straight26, 26, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aStraight)
P(Gentle4, 4, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aGentle)
P(Gentle12, 12, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 60.0f, sithThingVec_aGentle)
P(Sharp2, 2, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aSharp)
P(Sharp4, 4, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aSharp)
P(Sharp8, 8, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aSharp)
P(Sharp26, 26, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aSharp)
P(UTurn6, 6, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aUTurn)
P(Zigzag5, 5, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aZigzag)
P(Zigzag1, 1, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aZigzag)
P(Reset3, 3, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aReset)
P(SharpInit3, 3, 0, 1, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aBigSteps)
P(SharpInitStraight3, 3, 0, 1, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aStraight)
P(BigSteps4, 4, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aBigSteps)
P(Velocities3, 3, 0, 0, SITHTHINGVEC_BLOCK_NONE, 1, 2, 0, 30.0f, sithThingVec_aVelocities)
P(Negative3, 3, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aNegative)
P(Sectors3, 3, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aSectors)
P(OtherFirst3, 3, 1, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aGentle)
P(OtherAll4, 4, 7, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aSharp)
P(Detach4, 4, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aDetach)
P(LateTail2, 2, 7, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aLateTail)
P(Still3, 3, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 30.0f, sithThingVec_aStill)
P(Pitched3, 3, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 4, 1, 30.0f, sithThingVec_aPitched)
P(AllocFail3, 3, 0, 0, SITHTHINGVEC_BLOCK_ALLOCFAIL, 0, 1, 0, 30.0f, sithThingVec_aShort)
P(Preset3, 3, 0, 0, SITHTHINGVEC_BLOCK_PRESET, 0, 1, 0, 30.0f, sithThingVec_aGentle)
P(Preset6, 6, 0, 0, SITHTHINGVEC_BLOCK_PRESET, 1, 3, 0, 30.0f, sithThingVec_aZigzag)
P(Preset10, 10, 0, 1, SITHTHINGVEC_BLOCK_PRESET, -2, 3, 0, 30.0f, sithThingVec_aSharp)
P(Fps0, 3, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 0.0f, sithThingVec_aGentle)
P(Fps1000, 3, 0, 0, SITHTHINGVEC_BLOCK_NONE, 0, 1, 0, 1000.0f, sithThingVec_aSharp)

#undef P
#undef SITHTHINGVEC_SCRIPT

#define C(name, var, params) { name, sithThingVec_RunCreate, var, &sithThingVec_p##params }
#define U(name, params)      { name, sithThingVec_RunUpdate, 0, &sithThingVec_p##params }

static const SithVecCase sithThingVec_aQuetzCases[] = {
    // sithThing_CreateQuetzUserBlock: segment counts up to and past the 26 entries of the block (from 26 on it stops
    // and numAttachInfos stays one short), other attachments skipped, an existing block, failed allocation, no
    // attachments, a pitched and a vertical head (the flattened direction of a vertical lvec is a zero vector)
    C("Create_tail0", 0, Create0),
    C("Create_tail0_other", 0, Create0Other),
    C("Create_noattach", 3, Create0),
    C("Create_tail1", 0, Create1),
    C("Create_tail2", 0, Create2),
    C("Create_tail3", 0, Create3),
    C("Create_tail4", 0, Create4),
    C("Create_tail8_other", 0, Create8Other),
    C("Create_tail25", 0, Create25),
    C("Create_edge_tail26", 0, Create26),
    C("Create_edge_tail27", 0, Create27),
    C("Create_edge_tail34", 0, Create34),
    C("Create_pitched", 0, CreatePitched),
    C("Create_edge_vertical", 0, CreateVertical),
    C("Create_existing", 1, Create3),
    C("Create_allocfail", 2, Create3),

    // sithThing_UpdateQuetzUserBlock over frame scripts
    U("Update_straight_tail1", Straight1),
    U("Update_straight_tail3", Straight3),
    U("Update_straight_tail8_created", Straight8),
    U("Update_edge_straight_tail26", Straight26),
    U("Update_gentle_tail4", Gentle4),
    U("Update_gentle_tail12_fps60", Gentle12),
    U("Update_sharp_tail2", Sharp2),
    U("Update_sharp_tail4", Sharp4),
    U("Update_sharp_tail8", Sharp8),
    U("Update_edge_sharp_tail26", Sharp26),
    U("Update_uturn_tail6", UTurn6),
    U("Update_zigzag_tail5", Zigzag5),
    U("Update_zigzag_tail1", Zigzag1),
    U("Update_reset_tail3", Reset3),
    U("Update_sharpinit_bigsteps_tail3", SharpInit3),
    U("Update_sharpinit_straight_tail3", SharpInitStraight3),
    U("Update_bigsteps_tail4", BigSteps4),
    U("Update_velocities_tail3", Velocities3),
    U("Update_negativedist_tail3", Negative3),
    U("Update_sectors_moves_tail3", Sectors3),
    U("Update_otherfirst_tail3", OtherFirst3),
    U("Update_otherall_tail4", OtherAll4),
    U("Update_detach_tail4", Detach4),
    U("Update_edge_latetail_tail2", LateTail2),
    U("Update_still_tail3", Still3),
    U("Update_pitched_tail3", Pitched3),
    U("Update_allocfail_tail3", AllocFail3),
    U("Update_preset_tail3", Preset3),
    U("Update_preset_tail6", Preset6),
    U("Update_preset_sharp_tail10", Preset10),
    U("Update_edge_fps0_tail3", Fps0),
    U("Update_edge_fps1000_tail3", Fps1000),
};

#undef C
#undef U

static const SithVecSuite sithThingVec_quetzSuite = {
    "sithThingQuetz",
    sithThingVec_aQuetzCases,
    SITHVEC_ARRAYLEN(sithThingVec_aQuetzCases),
    sithThingVec_Build,
    sithThingVec_Watch,
    sithThingVec_Sync,
};

#define SITHVEC_SUITE sithThingVec_quetzSuite

#endif // SITH_WORLD_TESTS_SITHTHINGVECQUETZ_H
