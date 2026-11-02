// Synthetic world of the sith world-state vector harness: an arena with a world, sectors, things, AI control blocks,
// AI classes, an instinct registry, puppets and an allocation pool, its builder, the pure lookup stubs and the stubs
// of engine calls that many suites share (allocation, rand, sector search, thing collision move).
#ifndef SITH_TESTS_VEC_SITHVECWORLD_H
#define SITH_TESTS_VEC_SITHVECWORLD_H

#include "sithVec.h"

#include <sith/Engine/sithCollision.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMemory.h>

#include <math.h>

// Sizes: the first SITHVEC_NUM* elements are part of the world, the rest of each array holds the run's garbage fill
#define SITHVEC_NUMSECTORS   6u
#define SITHVEC_CAPSECTORS   8u
#define SITHVEC_NUMTHINGS    40u
#define SITHVEC_CAPTHINGS    44u
#define SITHVEC_CAPTEMPLATES 4u
#define SITHVEC_CAPAIBLOCKS  4u
#define SITHVEC_CAPAICLASSES 4u
#define SITHVEC_CAPAIFRAMES  8u
#define SITHVEC_CAPPUPPETS   4u
#define SITHVEC_CAPINSTINCTS 40u
#define SITHVEC_POOLSIZE     16384u
#define SITHVEC_USERSIZE     4096u

// Guid of thing i
#define SITHVEC_GUID(i) (0x1000 + (int)(i) * 0x11)

typedef struct
{
    SithWorld world;
    SithWorld staticWorld;
    SithSector sectors[SITHVEC_CAPSECTORS];
    SithThing things[SITHVEC_CAPTHINGS];
    SithThing templates[SITHVEC_CAPTEMPLATES];
    SithAIClass aiClasses[SITHVEC_CAPAICLASSES];
    SithAIControlBlock aiBlocks[SITHVEC_CAPAIBLOCKS];
    rdVector3 aiFrames[SITHVEC_CAPAIBLOCKS][SITHVEC_CAPAIFRAMES];
    rdPuppet puppets[SITHVEC_CAPPUPPETS];
    SithPuppetState puppetStates[SITHVEC_CAPPUPPETS];
    SithAIRegisteredInstinct instincts[SITHVEC_CAPINSTINCTS];
    uint8_t pool[SITHVEC_POOLSIZE]; // stdMemory_Malloc allocates from here
    uint32_t poolUsed;
    uint8_t user[SITHVEC_USERSIZE]; // suite specific state
    uint32_t tail[4];               // keeps the arena end distinct
} SithVecArena;

static SithVecArena sithVec_arena;

// Stable id of a pointer into the arena: kind << 24 | (index + 1); 0 for NULL
static uint32_t sithVec_Id(const void* p)
{
    SithVecArena* a = &sithVec_arena;
#define SITHVEC_ID(kind, arr) \
    if ( (const uint8_t*)p >= (const uint8_t*)&a->arr[0] && (const uint8_t*)p < (const uint8_t*)&a->arr[SITHVEC_ARRAYLEN(a->arr)] ) \
        return ((uint32_t)(kind) << 24) | (uint32_t)(((const uint8_t*)p - (const uint8_t*)&a->arr[0]) / sizeof(a->arr[0]) + 1)
    if ( !p )
    {
        return 0;
    }

    SITHVEC_ID(1u, things);
    SITHVEC_ID(2u, sectors);
    SITHVEC_ID(3u, templates);
    SITHVEC_ID(4u, aiBlocks);
    SITHVEC_ID(5u, aiClasses);
    SITHVEC_ID(6u, puppets);
    SITHVEC_ID(7u, instincts);
#undef SITHVEC_ID
    return sithVec_NormWord((uint32_t)(uintptr_t)p);
}

#define SITHVEC_P(p) ((uint32_t)sithVec_Id(p))

// ---------------------------------------------------------------------------------------------------------------
// Builder
// ---------------------------------------------------------------------------------------------------------------
static void sithVec_LinkThingToSector(SithThing* pThing, SithSector* pSector)
{
    pThing->pInSector          = pSector;
    pThing->pPrevThingInSector = NULL;
    pThing->pNextThingInSector = pSector->pFirstThingInSector;
    if ( pSector->pFirstThingInSector )
    {
        pSector->pFirstThingInSector->pPrevThingInSector = pThing;
    }

    pSector->pFirstThingInSector = pThing;
}

// Makes thing i a zeroed thing of the given type in sector sectorNum (identity orientation)
static SithThing* sithVec_NewThing(unsigned int i, SithThingType type, SithThingMoveType moveType, unsigned int sectorNum)
{
    SithThing* pThing = &sithVec_arena.things[i];
    memset(pThing, 0, sizeof(*pThing));
    pThing->idx         = (int)i;
    pThing->guid        = SITHVEC_GUID(i);
    pThing->type        = type;
    pThing->moveType    = moveType;
    pThing->signature   = 0x100u + i;
    pThing->orient.rvec.x = 1.0f;
    pThing->orient.lvec.y = 1.0f;
    pThing->orient.uvec.z = 1.0f;
    snprintf(pThing->aName, sizeof(pThing->aName), "thing%u", i);
    sithVec_LinkThingToSector(pThing, &sithVec_arena.sectors[sectorNum]);
    return pThing;
}

// Builds the world shared by all suites: SITHVEC_NUMSECTORS empty sectors without special flags and
// SITHVEC_NUMTHINGS free things; the unused tail of every array holds the run's fill byte
static void sithVec_BuildWorld(void)
{
    SithVecArena* a = &sithVec_arena;
    memset(a, 0, sizeof(*a));
    memset(a->sectors, (int)sithVec_fill, sizeof(a->sectors));
    memset(a->things, (int)sithVec_fill, sizeof(a->things));
    memset(a->templates, (int)sithVec_fill, sizeof(a->templates));
    memset(a->aiClasses, (int)sithVec_fill, sizeof(a->aiClasses));
    memset(a->aiBlocks, (int)sithVec_fill, sizeof(a->aiBlocks));
    memset(a->aiFrames, (int)sithVec_fill, sizeof(a->aiFrames));
    memset(a->puppets, (int)sithVec_fill, sizeof(a->puppets));
    memset(a->puppetStates, (int)sithVec_fill, sizeof(a->puppetStates));
    memset(a->instincts, (int)sithVec_fill, sizeof(a->instincts));
    memset(a->pool, (int)sithVec_fill, sizeof(a->pool));
    memset(a->user, (int)sithVec_fill, sizeof(a->user));

    SithWorld* pWorld = &a->world;
    sithWorld_g_pCurrentWorld = pWorld;
    sithWorld_g_pStaticWorld  = &a->staticWorld;

    pWorld->aSectors   = a->sectors;
    pWorld->numSectors = SITHVEC_NUMSECTORS;
    for ( unsigned int i = 0; i < SITHVEC_NUMSECTORS; ++i )
    {
        SithSector* pSector = &a->sectors[i];
        memset(pSector, 0, sizeof(*pSector));
        pSector->ambientLight.red   = sithVec_RndF();
        pSector->ambientLight.green = sithVec_RndF();
        pSector->ambientLight.blue  = sithVec_RndF();
        pSector->numSurfaces        = 0;
    }

    pWorld->aThings      = a->things;
    pWorld->numThings    = SITHVEC_NUMTHINGS;
    pWorld->lastThingIdx = (int)SITHVEC_NUMTHINGS - 1;
    for ( unsigned int i = 0; i < SITHVEC_NUMTHINGS; ++i )
    {
        memset(&a->things[i], 0, sizeof(a->things[i]));
        a->things[i].idx = (int)i;
    }

    pWorld->aThingTemplates   = a->templates;
    pWorld->numThingTemplates = 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Pure lookups. In the tests the calls of a tested source are renamed to these stubs (SITHVEC_LOOKUP in the seams
// file) when the original lookup reads engine globals the tests cannot reach; the harness leaves the original
// running on the synthetic world and verifies the stub against it (sithVec_VerifyLookups).
// ---------------------------------------------------------------------------------------------------------------
SITHVEC_STUB int J3DAPI sithVec_sithSector_ValidateSectorPointer(const SithWorld* pWorld, const SithSector* pSector)
{
    if ( !pWorld || !pSector || pSector < pWorld->aSectors || pSector >= &pWorld->aSectors[pWorld->numSectors] )
    {
        return 0;
    }

    return 1;
}

SITHVEC_STUB int J3DAPI sithVec_sithSector_GetSectorIndex(const SithSector* pSector)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( pSector >= pWorld->aSectors && pSector <= &pWorld->aSectors[pWorld->numSectors] )
    {
        return (int)(pSector - pWorld->aSectors);
    }

    return -1;
}

SITHVEC_STUB SithThing* J3DAPI sithVec_sithThing_GetThingByIndex(int idx)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( idx < 0 || idx >= (int)pWorld->numThings || !pWorld->aThings[idx].type )
    {
        return NULL;
    }

    return &pWorld->aThings[idx];
}

SITHVEC_STUB int J3DAPI sithVec_sithThing_GetThingIndex(const SithThing* pThing)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( !pThing || pThing < pWorld->aThings || pThing >= &pWorld->aThings[pWorld->numThings] || pThing->type == SITH_THING_FREE )
    {
        return -1;
    }

    return pThing->idx;
}

// The original rejects indices from 2304 (the size of its free thing index list)
SITHVEC_STUB int J3DAPI sithVec_sithThing_ValidateThingPointer(const SithWorld* pWorld, const SithThing* pThing)
{
    if ( !pThing || pThing->idx != pThing - pWorld->aThings || pThing->idx >= 2304 )
    {
        return 0;
    }

    return 1;
}

#ifdef SITHVEC_HARNESS
// Compares a lookup stub with the original on the synthetic world for the pointers and indices the cases can produce
static int sithVec_numLookupMismatches;

static void sithVec_LookupCheck(int bEqual, const char* pWhat, int i)
{
    if ( !bEqual )
    {
        ++sithVec_numLookupMismatches;
        SITHVEC_LOG("lookup mismatch %s %d\n", pWhat, i);
    }
}

static void sithVec_VerifyLookups(void)
{
    SithVecArena* a   = &sithVec_arena;
    SithWorld* pWorld = &a->world;
    sithVec_fill      = 0xA5;
    sithVec_BuildWorld();
    for ( unsigned int i = 0; i < 12; ++i )
    {
        sithVec_NewThing(i, SITH_THING_ACTOR, SITH_MT_PHYSICS, i % SITHVEC_NUMSECTORS);
    }

    static const int aIdx[] = { (int)0x80000000u, -2, -1, 0, 1, 5, 11, 12, 39, 40, 41, 43, 2303, 2304, 0x7FFFFFFF };
    for ( unsigned int k = 0; k < SITHVEC_ARRAYLEN(aIdx); ++k )
    {
        sithVec_LookupCheck(SITHVEC_FN(sithThing_GetThingByIndex)(aIdx[k]) == sithVec_sithThing_GetThingByIndex(aIdx[k]), "GetThingByIndex", aIdx[k]);
    }

    for ( unsigned int i = 0; i <= SITHVEC_CAPTHINGS; ++i )
    {
        const SithThing* pThing = i < SITHVEC_CAPTHINGS ? &a->things[i] : NULL;
        sithVec_LookupCheck(SITHVEC_FN(sithThing_GetThingIndex)(pThing) == sithVec_sithThing_GetThingIndex(pThing), "GetThingIndex", (int)i);
        if ( pThing && i >= SITHVEC_NUMTHINGS )
        {
            continue; // garbage idx of the tail things
        }

        sithVec_LookupCheck(SITHVEC_FN(sithThing_ValidateThingPointer)(pWorld, pThing) == sithVec_sithThing_ValidateThingPointer(pWorld, pThing), "ValidateThingPointer", (int)i);
    }

    // Wrong idx, an idx past the original's thing limit
    SithThing saved = a->things[3];
    a->things[3].idx = 4;
    sithVec_LookupCheck(SITHVEC_FN(sithThing_ValidateThingPointer)(pWorld, &a->things[3]) == sithVec_sithThing_ValidateThingPointer(pWorld, &a->things[3]), "ValidateThingPointer idx", 3);
    a->things[3] = saved;

    for ( unsigned int i = 0; i <= SITHVEC_CAPSECTORS; ++i )
    {
        const SithSector* pSector = i < SITHVEC_CAPSECTORS ? &a->sectors[i] : NULL;
        sithVec_LookupCheck(SITHVEC_FN(sithSector_ValidateSectorPointer)(pWorld, pSector) == sithVec_sithSector_ValidateSectorPointer(pWorld, pSector), "ValidateSectorPointer", (int)i);
        sithVec_LookupCheck(SITHVEC_FN(sithSector_ValidateSectorPointer)(NULL, pSector) == sithVec_sithSector_ValidateSectorPointer(NULL, pSector), "ValidateSectorPointer NULL world", (int)i);
        if ( pSector )
        {
            sithVec_LookupCheck(SITHVEC_FN(sithSector_GetSectorIndex)(pSector) == sithVec_sithSector_GetSectorIndex(pSector), "GetSectorIndex", (int)i);
        }
    }
}
#endif // SITHVEC_HARNESS

// ---------------------------------------------------------------------------------------------------------------
// Shared engine call stubs (SITHVEC_SEAM in the seams file of a suite). Their behavior is set per case through the
// sithVec_stub* variables, which sithVec_RunCase resets before every case.
// ---------------------------------------------------------------------------------------------------------------
static int sithVec_stubAllocFail;            // stdMemory_Malloc returns NULL from this allocation number (1 based), 0: never
static int sithVec_stubFindSectorMode;       // sithCollision_FindSectorInRadius: 0 returns the start sector, 1 the next sector
static float sithVec_stubMoveScale;          // sithCollision_MoveThing moves pos by dir * dist * scale and returns dist * scale
static uint32_t sithVec_stubRandSeed;        // state of the rand stub
static int sithVec_stubFindSectorNullFrom;   // sithCollision_FindSectorInRadius returns NULL from this call number (1 based) of a case, 0: never
static int sithVec_numFindSectorCalls;       // calls of sithCollision_FindSectorInRadius in the current case

SITHVEC_STUB void* J3DAPI sithVec_stdMemory_Malloc(size_t size, const char* pFilename, size_t line)
{
    J3D_UNUSED(pFilename);
    J3D_UNUSED(line);
    ++sithVec_numMallocs;
    sithVec_Ev(SITHVEC_EV_MALLOC, 1, (uint32_t)size);

    SithVecArena* a = &sithVec_arena;
    if ( (sithVec_stubAllocFail && (int)sithVec_numMallocs >= sithVec_stubAllocFail) || a->poolUsed + size > sizeof(a->pool) )
    {
        return NULL;
    }

    void* p = &a->pool[a->poolUsed];
    a->poolUsed += (uint32_t)((size + 15u) & ~15u);
    return p;
}

SITHVEC_STUB void J3DAPI sithVec_stdMemory_Free(void* pBytes)
{
    sithVec_Ev(SITHVEC_EV_FREE, 1, (uint32_t)(uintptr_t)pBytes);
}

// The C runtime's rand (same sequence as the original's statically linked rand for the same seed)
SITHVEC_STUB int __cdecl sithVec_rand(void)
{
    sithVec_stubRandSeed = sithVec_stubRandSeed * 214013u + 2531011u;
    int r = (int)((sithVec_stubRandSeed >> 16) & 0x7FFFu);
    sithVec_Ev(SITHVEC_EV_RAND, 1, (uint32_t)r);
    return r;
}

SITHVEC_STUB SithSector* J3DAPI sithVec_sithCollision_FindSectorInRadius(SithSector* pStartSector, const rdVector3* pStartPos, rdVector3* pEndPos, float radius)
{
    sithVec_Ev(SITHVEC_EV_FINDSECTORINRADIUS, 2, SITHVEC_P(pStartSector), sithVec_FloatBits(radius));
    sithVec_EvWords(pStartPos, sizeof(*pStartPos));
    sithVec_EvWords(pEndPos, sizeof(*pEndPos));
    ++sithVec_numFindSectorCalls;
    if ( sithVec_stubFindSectorNullFrom && sithVec_numFindSectorCalls >= sithVec_stubFindSectorNullFrom )
    {
        return NULL;
    }

    if ( sithVec_stubFindSectorMode == 1 && pStartSector )
    {
        size_t next = ((size_t)(pStartSector - sithVec_arena.sectors) + 1u) % SITHVEC_NUMSECTORS;
        return &sithVec_arena.sectors[next];
    }

    return pStartSector;
}

SITHVEC_STUB float J3DAPI sithVec_sithCollision_MoveThing(SithThing* pThing, const rdVector3* pMoveNorm, float moveDist, int flags)
{
    sithVec_Ev(SITHVEC_EV_MOVETHING, 3, SITHVEC_P(pThing), sithVec_FloatBits(moveDist), (uint32_t)flags);
    sithVec_EvWords(pMoveNorm, sizeof(*pMoveNorm));
    const float dist = moveDist * sithVec_stubMoveScale;
    pThing->pos.x += pMoveNorm->x * dist;
    pThing->pos.y += pMoveNorm->y * dist;
    pThing->pos.z += pMoveNorm->z * dist;
    return dist;
}

// ---------------------------------------------------------------------------------------------------------------
// Math stubs. The original computes rdroid/stdMath results on the x87 (with intermediates kept in 53-bit precision),
// the reconstruction in single precision SSE, so results can differ in the last bits; and both read tables of their
// own. A suite that tests engine logic rather than these helpers seams them to the stubs below, which both sides
// compute with the same float operations (the trigonometry is a polynomial, not the engine's tables). They follow the
// structure of the original helpers (zero length checks, the degenerate look vector of BuildFromLook34, the matrix
// layout) closely enough for the cases, but they are a definition of their own and not under test.
// ---------------------------------------------------------------------------------------------------------------
#define SITHVEC_PI 3.14159265f

// sin of x in radians, |x| <= pi / 2
static float sithVec_SinRad(float x)
{
    float x2 = x * x;
    return x * (1.0f - x2 / 6.0f * (1.0f - x2 / 20.0f * (1.0f - x2 / 42.0f * (1.0f - x2 / 72.0f))));
}

// asin of x in degrees, |x| <= 1
static float sithVec_AsinDeg(float x)
{
    float a = x < 0.0f ? -x : x;
    float r;
    if ( a <= 0.5f )
    {
        float a2 = a * a;
        r        = a * (1.0f + a2 * (1.0f / 6.0f + a2 * (3.0f / 40.0f + a2 * (15.0f / 336.0f + a2 * (105.0f / 3456.0f)))));
    }
    else
    {
        float h  = sqrtf((1.0f - a) * 0.5f);
        float h2 = h * h;
        r        = SITHVEC_PI / 2.0f - 2.0f * h * (1.0f + h2 * (1.0f / 6.0f + h2 * (3.0f / 40.0f + h2 * (15.0f / 336.0f + h2 * (105.0f / 3456.0f)))));
    }

    r = r * (180.0f / SITHVEC_PI);
    return x < 0.0f ? -r : r;
}

SITHVEC_STUB void J3DAPI sithVec_stdMath_SinCos(float angle, float* pSinOut, float* pCosOut)
{
    // Reduce to [-180, 180), then use sin(180 - a) = sin(a) to stay within +-90
    float a = angle - 360.0f * floorf((angle + 180.0f) / 360.0f);
    float s = a > 90.0f ? 180.0f - a : (a < -90.0f ? -180.0f - a : a);
    float c = a + 90.0f;
    c       = c >= 180.0f ? c - 360.0f : c;
    c       = c > 90.0f ? 180.0f - c : (c < -90.0f ? -180.0f - c : c);
    *pSinOut = sithVec_SinRad(s * (SITHVEC_PI / 180.0f));
    *pCosOut = sithVec_SinRad(c * (SITHVEC_PI / 180.0f));
}

SITHVEC_STUB float J3DAPI sithVec_rdVector_Normalize3(rdVector3* pDest, const rdVector3* pSrc)
{
    float len = sqrtf(pSrc->x * pSrc->x + pSrc->y * pSrc->y + pSrc->z * pSrc->z);
    if ( len == 0.0f )
    {
        *pDest = *pSrc;
        return len;
    }

    float invLen = 1.0f / len;
    pDest->x = pSrc->x * invLen;
    pDest->y = pSrc->y * invLen;
    pDest->z = pSrc->z * invLen;
    return len;
}

SITHVEC_STUB float J3DAPI sithVec_rdVector_Normalize3Acc(rdVector3* pVec)
{
    float len = sqrtf(pVec->x * pVec->x + pVec->y * pVec->y + pVec->z * pVec->z);
    if ( len != 0.0f )
    {
        float invLen = 1.0f / len;
        pVec->x *= invLen;
        pVec->y *= invLen;
        pVec->z *= invLen;
    }

    return len;
}

static void sithVec_Cross3(rdVector3* pOut, const rdVector3* a, const rdVector3* b)
{
    pOut->x = a->y * b->z - a->z * b->y;
    pOut->y = a->z * b->x - a->x * b->z;
    pOut->z = a->x * b->y - a->y * b->x;
}

SITHVEC_STUB void J3DAPI sithVec_rdMatrix_TransformVector34(rdVector3* pDest, const rdVector3* pSrc, const rdMatrix34* pMat)
{
    pDest->x = pMat->rvec.x * pSrc->x + pMat->lvec.x * pSrc->y + pMat->uvec.x * pSrc->z;
    pDest->y = pMat->rvec.y * pSrc->x + pMat->lvec.y * pSrc->y + pMat->uvec.y * pSrc->z;
    pDest->z = pMat->rvec.z * pSrc->x + pMat->lvec.z * pSrc->y + pMat->uvec.z * pSrc->z;
}

// Like the original, dvec is left as it is
SITHVEC_STUB void J3DAPI sithVec_rdMatrix_BuildFromLook34(rdMatrix34* pMat, const rdVector3* pLook)
{
    static const rdVector3 zVec = { 0.0f, 0.0f, 1.0f };
    static const rdVector3 xVec = { 1.0f, 0.0f, 0.0f };
    pMat->lvec = *pLook;
    sithVec_Cross3(&pMat->rvec, &pMat->lvec, &zVec);
    if ( sithVec_rdVector_Normalize3Acc(&pMat->rvec) == 0.0f )
    {
        sithVec_Cross3(&pMat->uvec, &xVec, &pMat->lvec);
        sithVec_rdVector_Normalize3Acc(&pMat->uvec);
        sithVec_Cross3(&pMat->rvec, &pMat->lvec, &pMat->uvec);
    }
    else
    {
        sithVec_Cross3(&pMat->uvec, &pMat->rvec, &pMat->lvec);
    }
}

// Rotation by pitch (x), yaw (y) and roll (z) degrees applied before the matrix (rdMatrix_BuildRotate34 and
// rdMatrix_PreMultiply34 of the engine)
SITHVEC_STUB void J3DAPI sithVec_rdMatrix_PreRotate34(rdMatrix34* pMat, const rdVector3* pPyr)
{
    float xs, xc, ys, yc, zs, zc;
    sithVec_stdMath_SinCos(pPyr->x, &xs, &xc);
    sithVec_stdMath_SinCos(pPyr->y, &ys, &yc);
    sithVec_stdMath_SinCos(pPyr->z, &zs, &zc);

    rdMatrix34 r;
    r.rvec.x = -(ys * zs) * xs + yc * zc;
    r.rvec.y = (yc * zs) * xs + ys * zc;
    r.rvec.z = -zs * xc;
    r.lvec.x = -ys * xc;
    r.lvec.y = yc * xc;
    r.lvec.z = xs;
    r.uvec.x = (ys * zc) * xs + yc * zs;
    r.uvec.y = -(yc * zc) * xs + ys * zs;
    r.uvec.z = zc * xc;

    rdMatrix34 a = *pMat;
    pMat->rvec.x = a.rvec.x * r.rvec.x + a.lvec.x * r.rvec.y + a.uvec.x * r.rvec.z;
    pMat->rvec.y = a.rvec.y * r.rvec.x + a.lvec.y * r.rvec.y + a.uvec.y * r.rvec.z;
    pMat->rvec.z = a.rvec.z * r.rvec.x + a.lvec.z * r.rvec.y + a.uvec.z * r.rvec.z;
    pMat->lvec.x = a.rvec.x * r.lvec.x + a.lvec.x * r.lvec.y + a.uvec.x * r.lvec.z;
    pMat->lvec.y = a.rvec.y * r.lvec.x + a.lvec.y * r.lvec.y + a.uvec.y * r.lvec.z;
    pMat->lvec.z = a.rvec.z * r.lvec.x + a.lvec.z * r.lvec.y + a.uvec.z * r.lvec.z;
    pMat->uvec.x = a.rvec.x * r.uvec.x + a.lvec.x * r.uvec.y + a.uvec.x * r.uvec.z;
    pMat->uvec.y = a.rvec.y * r.uvec.x + a.lvec.y * r.uvec.y + a.uvec.y * r.uvec.z;
    pMat->uvec.z = a.rvec.z * r.uvec.x + a.lvec.z * r.uvec.y + a.uvec.z * r.uvec.z;
}

// Signed angle in degrees between two unit vectors around pZ
SITHVEC_STUB float J3DAPI sithVec_rdMath_DeltaAngleNormalized(const rdVector3* pX, const rdVector3* pY, const rdVector3* pZ)
{
    float dot = pX->x * pY->x + pX->y * pY->y + pX->z * pY->z;
    dot       = dot < -1.0f ? -1.0f : (dot > 1.0f ? 1.0f : dot);
    if ( dot == 1.0f )
    {
        return 0.0f;
    }

    if ( dot == -1.0f )
    {
        return 180.0f;
    }

    float angle = 90.0f - sithVec_AsinDeg(dot);
    rdVector3 cross;
    sithVec_Cross3(&cross, pX, pY);
    float side = cross.x * pZ->x + cross.y * pZ->y + cross.z * pZ->z;
    return side <= 0.0f ? -angle : angle;
}

static void sithVec_ResetStubs(void)
{
    sithVec_stubAllocFail      = 0;
    sithVec_stubFindSectorMode = 0;
    sithVec_stubFindSectorNullFrom = 0;
    sithVec_numFindSectorCalls = 0;
    sithVec_stubMoveScale      = 1.0f;
    sithVec_stubRandSeed       = 1u;
}

// ---------------------------------------------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t ret;
    uint16_t aCounts[4]; // asserts, errors, warnings, mallocs
    SithVecCtx ctx;
} SithVecRun;

// Runs a case of the current suite (sithVec_pSuite) once with the given garbage fill
static void sithVec_RunOnce(const SithVecCase* pCase, uint32_t fill, SithVecRun* pRun)
{
    memset(pRun, 0, sizeof(*pRun));
    SithVecCtx* pCtx = &pRun->ctx;
    pCtx->pCase      = pCase;

    sithVec_fill        = fill;
    sithVec_rng         = sithVec_Hash(pCase->pName);
    sithVec_numAsserts  = 0;
    sithVec_numErrors   = 0;
    sithVec_numWarnings = 0;
    sithVec_numMallocs  = 0;
    sithVec_ev.size     = 0;
    sithVec_ResetStubs();

    sithVec_numRegions = 0;
    sithVec_numFuncs   = 0;
    sithVec_AddRegion(&sithVec_arena, sizeof(sithVec_arena));

    sithVec_BuildWorld();
    if ( sithVec_pSuite->pfBuild )
    {
        sithVec_pSuite->pfBuild(pCase);
    }

    sithVec_numWatch  = 0;
    sithVec_numIgnore = 0;
    SITHVEC_WATCH(sithVec_arena);
    if ( sithVec_pSuite->pfWatch )
    {
        sithVec_pSuite->pfWatch();
    }

    pRun->ret = (uint32_t)pCase->pfRun(pCtx);
    if ( !pCtx->bBegun )
    {
        SITHVEC_LOG("sithVec: case %s did not call sithVec_Begin\n", pCase->pName);
        sithVec_Begin(pCtx);
    }

    sithVec_Checkpoint(pCtx);

    pRun->aCounts[0] = (uint16_t)sithVec_numAsserts;
    pRun->aCounts[1] = (uint16_t)sithVec_numErrors;
    pRun->aCounts[2] = (uint16_t)sithVec_numWarnings;
    pRun->aCounts[3] = (uint16_t)sithVec_numMallocs;
}

// Runs a case of the current suite with both garbage fills and stores its record in pRec. Returns 0 (with the reason
// in pWhy) when the two runs disagree.
static int sithVec_RunCase(const SithVecCase* pCase, SithVecBuf* pRec, char* pWhy, size_t whySize)
{
    static SithVecRun runA;
    static SithVecRun runB;
    sithVec_RunOnce(pCase, 0xA5, &runA);
    sithVec_RunOnce(pCase, 0x5A, &runB);

    int bStable = 1;
    pWhy[0]     = 0;
    if ( runA.ret != runB.ret || memcmp(runA.aCounts, runB.aCounts, sizeof(runA.aCounts)) != 0 || runA.ctx.numCheckpoints != runB.ctx.numCheckpoints )
    {
        snprintf(pWhy, whySize, "return value, host call counts or checkpoints differ");
        bStable = 0;
    }

    pRec->size = 0;
    sithVecB_U32(pRec, runA.ret);
    for ( size_t i = 0; i < SITHVEC_ARRAYLEN(runA.aCounts); ++i )
    {
        sithVecB_U16(pRec, runA.aCounts[i]);
    }

    sithVecB_U32(pRec, runA.ctx.numCheckpoints);
    for ( uint32_t c = 0; bStable && c < runA.ctx.numCheckpoints; ++c )
    {
        const SithVecBuf* pEvA = &runA.ctx.aEvents[c];
        const SithVecBuf* pEvB = &runB.ctx.aEvents[c];
        if ( pEvA->size != pEvB->size || (pEvA->size && memcmp(pEvA->p, pEvB->p, pEvA->size) != 0) )
        {
            snprintf(pWhy, whySize, "engine calls of checkpoint %u differ", c);
            bStable = 0;
            break;
        }

        sithVecB_U32(pRec, (uint32_t)pEvA->size);
        sithVecB_Put(pRec, pEvA->p, pEvA->size);

        size_t badOffset = 0;
        if ( !sithVec_Delta(pRec, &runA.ctx.aImages[c], &runA.ctx.aImages[c + 1], &runB.ctx.aImages[c], &runB.ctx.aImages[c + 1], &badOffset) )
        {
            snprintf(pWhy, whySize, "state of checkpoint %u differs at watched offset %u", c, (unsigned int)badOffset);
            bStable = 0;
        }
    }

    sithVec_FreeCtx(&runA.ctx);
    sithVec_FreeCtx(&runB.ctx);
    return bStable;
}

#endif // SITH_TESTS_VEC_SITHVECWORLD_H
