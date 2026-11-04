// Core of the sith world-state original-engine vector harness (see README.md in this directory).
//
// The headers of this directory and the case headers of a suite are compiled twice from the same source:
//  * by the capture harness (SITHVEC_HARNESS defined), a DLL injected into the retail v1.0 Indy3D.exe that runs the
//    ORIGINAL functions at their v1.0 addresses, and
//  * by the suite's test executable, which runs the reconstructed functions.
// Both build the same synthetic engine state, route the engine calls the tested functions make through the same
// stubs and produce a byte record per case. The harness stores the records in a .tv fixture, the tests compare them.
//
// Record layout of a case (all little endian):
//   u32 return value
//   u16 numAsserts, u16 numErrors, u16 numWarnings, u16 numMallocs   (host service and allocation calls)
//   u32 numCheckpoints, checkpoint[numCheckpoints]
//   checkpoint: u32 eventsSize, events[]   (calls of the stubbed engine functions, see sithVec_Ev)
//               u32 numDeltas, delta[]     (u32 offset, u16 size, bytes: changed bytes of the watched memory since
//                                           the previous checkpoint, addresses normalized, see sithVec_NormWord)
// Every case is run twice, with the unused tail of every array filled with 0xA5 and with 0x5A; the record holds the
// bytes that changed in either run and is only valid when both runs agree (sithVec_RunCase).
#ifndef SITH_TESTS_VEC_SITHVEC_H
#define SITH_TESTS_VEC_SITHVEC_H

#include <j3dcore/j3d.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rdroid/types.h>

#include <sith/types.h>

#include <std/types.h>

// Stubs are file local in the harness (it patches the original functions to jump to them) and external in the tests
// (the calls of the tested sources are renamed to them, see SithVecTest.cmake)
#ifdef SITHVEC_HARNESS
#define SITHVEC_STUB static
#define SITHVEC_FN(fn) ((fn##_TYPE)fn##_ADDR)
#else
#define SITHVEC_STUB
#define SITHVEC_FN(fn) fn
#endif

#define SITHVEC_ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))

// Diagnostics of the harness itself (the capture harness writes them to its log)
#ifndef SITHVEC_LOG
#define SITHVEC_LOG printf
#endif

// ---------------------------------------------------------------------------------------------------------------
// Growable byte buffer
// ---------------------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t* p;
    size_t size;
    size_t cap;
} SithVecBuf;

static void sithVecB_Put(SithVecBuf* b, const void* pData, size_t n)
{
    if ( b->size + n > b->cap )
    {
        b->cap = (b->size + n) * 2 + 256;
        b->p   = (uint8_t*)realloc(b->p, b->cap);
    }

    if ( pData )
    {
        memcpy(b->p + b->size, pData, n);
    }
    else
    {
        memset(b->p + b->size, 0, n);
    }

    b->size += n;
}

static void sithVecB_U8(SithVecBuf* b, uint32_t v)
{
    uint8_t x = (uint8_t)v;
    sithVecB_Put(b, &x, 1);
}

static void sithVecB_U16(SithVecBuf* b, uint32_t v)
{
    uint16_t x = (uint16_t)v;
    sithVecB_Put(b, &x, 2);
}

static void sithVecB_U32(SithVecBuf* b, uint32_t v)
{
    sithVecB_Put(b, &v, 4);
}

static void sithVecB_Free(SithVecBuf* b)
{
    free(b->p);
    b->p    = NULL;
    b->size = 0;
    b->cap  = 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Deterministic data. The seed of a case is the hash of its name.
// ---------------------------------------------------------------------------------------------------------------
static uint32_t sithVec_rng;
static uint32_t sithVec_fill; // garbage byte of the current run (every case is run with 0xA5 and 0x5A)

static uint32_t sithVec_Hash(const char* pStr)
{
    uint32_t h = 2166136261u;
    while ( *pStr )
    {
        h = (h ^ (uint8_t)*pStr++) * 16777619u;
    }

    return h;
}

// 24 random bits
static uint32_t sithVec_Rnd(void)
{
    sithVec_rng = sithVec_rng * 1664525u + 1013904223u;
    return sithVec_rng >> 8;
}

static int sithVec_RndI(unsigned int n)
{
    return (int)(sithVec_Rnd() % n);
}

// Multiple of 1/16 in [-512, 512): exact, with a distinctive bit pattern
static float sithVec_RndF(void)
{
    return (float)((int)(sithVec_Rnd() % 16384u) - 8192) / 16.0f;
}

// Float given by its bit pattern (denormals, -0, +-FLT_MAX), exact on both sides
static float sithVec_BitsF(uint32_t bits)
{
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}

static void sithVec_RndVec3(rdVector3* pVec)
{
    pVec->x = sithVec_RndF();
    pVec->y = sithVec_RndF();
    pVec->z = sithVec_RndF();
}

// ---------------------------------------------------------------------------------------------------------------
// Host services (sith_g_pHS, std_g_pHS, rdroid_g_pHS). Calls are only counted, the text is not compared.
// ---------------------------------------------------------------------------------------------------------------
static unsigned int sithVec_numAsserts;
static unsigned int sithVec_numErrors;
static unsigned int sithVec_numWarnings;
static unsigned int sithVec_numMallocs;

static void J3DAPI sithVec_HsAssert(const char* pCond, const char* pFile, int line)
{
    J3D_UNUSED(pCond);
    J3D_UNUSED(pFile);
    J3D_UNUSED(line);
    ++sithVec_numAsserts;
}

static void sithVec_HsError(const char* pFmt, ...)
{
    J3D_UNUSED(pFmt);
    ++sithVec_numErrors;
}

static void sithVec_HsWarning(const char* pFmt, ...)
{
    J3D_UNUSED(pFmt);
    ++sithVec_numWarnings;
}

static void sithVec_HsIgnore(const char* pFmt, ...)
{
    J3D_UNUSED(pFmt);
}

static tHostServices sithVec_hostServices;

static tHostServices* sithVec_InitHostServices(void)
{
    memset(&sithVec_hostServices, 0, sizeof(sithVec_hostServices));
    sithVec_hostServices.pAssert       = (tAssertFunc)sithVec_HsAssert;
    sithVec_hostServices.pStatusPrint  = (tPrintfFunc)sithVec_HsIgnore;
    sithVec_hostServices.pMessagePrint = (tPrintfFunc)sithVec_HsIgnore;
    sithVec_hostServices.pWarningPrint = (tPrintfFunc)sithVec_HsWarning;
    sithVec_hostServices.pErrorPrint   = (tPrintfFunc)sithVec_HsError;
    sithVec_hostServices.pDebugPrint   = (tPrintfFunc)sithVec_HsIgnore;
    return &sithVec_hostServices;
}

// ---------------------------------------------------------------------------------------------------------------
// Address normalization. Words of the watched memory and of event arguments that are addresses of a registered
// region become (0xA0 + region) << 24 | offset, registered functions become 0xCB000000 | function number, so the
// records do not depend on where the state or the code lives. Generated integers stay below 2^24 and generated floats
// are far from the region addresses (the tests load their image at a fixed high base, the harness checks its own).
// ---------------------------------------------------------------------------------------------------------------
#define SITHVEC_MAXREGIONS 8
#define SITHVEC_MAXFUNCS   64

static struct
{
    uintptr_t base;
    size_t size;
} sithVec_aRegions[SITHVEC_MAXREGIONS];
static unsigned int sithVec_numRegions;

// Function i: { original v1.0 address, address of the reconstructed function (0 in the harness) }
static uintptr_t sithVec_aFuncs[SITHVEC_MAXFUNCS][2];
static unsigned int sithVec_numFuncs;

static void sithVec_AddRegion(const void* p, size_t size)
{
    sithVec_aRegions[sithVec_numRegions].base = (uintptr_t)p;
    sithVec_aRegions[sithVec_numRegions].size = size;
    ++sithVec_numRegions;
}

static void sithVec_AddFunc(uintptr_t origAddr, uintptr_t localAddr)
{
    sithVec_aFuncs[sithVec_numFuncs][0] = origAddr;
    sithVec_aFuncs[sithVec_numFuncs][1] = localAddr;
    ++sithVec_numFuncs;
}

#ifdef SITHVEC_HARNESS
#define SITHVEC_ADDFUNC(fn) sithVec_AddFunc(fn##_ADDR, 0)
#else
// Altered: Before the retail address tables were dropped the first argument was fn##_ADDR, a retail constant no
// reconstructed pointer can equal; recording the local address keeps the normalization index and every record byte
// the tests produce unchanged.
#define SITHVEC_ADDFUNC(fn) sithVec_AddFunc((uintptr_t)&fn, (uintptr_t)&fn)
#endif

static uint32_t sithVec_NormWord(uint32_t w)
{
    for ( unsigned int i = 0; i < sithVec_numRegions; ++i )
    {
        if ( w >= sithVec_aRegions[i].base && w < sithVec_aRegions[i].base + sithVec_aRegions[i].size )
        {
            return ((0xA0u + i) << 24) | (uint32_t)(w - sithVec_aRegions[i].base);
        }
    }

    for ( unsigned int i = 0; i < sithVec_numFuncs; ++i )
    {
        if ( w == sithVec_aFuncs[i][0] || (sithVec_aFuncs[i][1] && w == sithVec_aFuncs[i][1]) )
        {
            return 0xCB000000u | i;
        }
    }

    return w;
}

// ---------------------------------------------------------------------------------------------------------------
// Watched memory: everything the tested function may change. Captured as normalized words.
// ---------------------------------------------------------------------------------------------------------------
#define SITHVEC_MAXWATCH 64

static struct
{
    const void* p;
    size_t size;
} sithVec_aWatch[SITHVEC_MAXWATCH];
static unsigned int sithVec_numWatch;

static void sithVec_Watch(const void* p, size_t size)
{
    sithVec_aWatch[sithVec_numWatch].p    = p;
    sithVec_aWatch[sithVec_numWatch].size = size;
    ++sithVec_numWatch;
}

#define SITHVEC_WATCH(x) sithVec_Watch(&(x), sizeof(x))

// Watched bytes that are excluded from the comparison (written as 0 into every image), for state that is undefined
// on both sides, for example copies of uninitialized locals. Every use is documented in the suite's README.
#define SITHVEC_MAXIGNORE 64

static struct
{
    const void* p;
    size_t size;
} sithVec_aIgnore[SITHVEC_MAXIGNORE];
static unsigned int sithVec_numIgnore;

static void sithVec_Ignore(const void* p, size_t size)
{
    sithVec_aIgnore[sithVec_numIgnore].p    = p;
    sithVec_aIgnore[sithVec_numIgnore].size = size;
    ++sithVec_numIgnore;
}

static void sithVec_Capture(SithVecBuf* pOut)
{
    pOut->size = 0;
    for ( unsigned int i = 0; i < sithVec_numWatch; ++i )
    {
        const uint8_t* p = (const uint8_t*)sithVec_aWatch[i].p;
        size_t n         = sithVec_aWatch[i].size;
        size_t k         = 0;
        for ( ; k + 4 <= n; k += 4 )
        {
            uint32_t w;
            memcpy(&w, p + k, 4);
            w = sithVec_NormWord(w);
            sithVecB_Put(pOut, &w, 4);
        }

        if ( k < n )
        {
            sithVecB_Put(pOut, p + k, n - k);
        }
    }

    for ( unsigned int i = 0; i < sithVec_numIgnore; ++i )
    {
        size_t regionOffset = 0;
        for ( unsigned int r = 0; r < sithVec_numWatch; ++r )
        {
            const uint8_t* pBase = (const uint8_t*)sithVec_aWatch[r].p;
            const uint8_t* pIgn  = (const uint8_t*)sithVec_aIgnore[i].p;
            if ( pIgn >= pBase && pIgn + sithVec_aIgnore[i].size <= pBase + sithVec_aWatch[r].size )
            {
                memset(pOut->p + regionOffset + (size_t)(pIgn - pBase), 0, sithVec_aIgnore[i].size);
                break;
            }

            regionOffset += sithVec_aWatch[r].size;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------
// Engine calls recorded by the stubs: u8 event id followed by its u32 arguments (normalized)
// ---------------------------------------------------------------------------------------------------------------
static SithVecBuf sithVec_ev;

// Event ids of the shared stubs (sithVecWorld.h); suites number their own events from SITHVEC_EV_SUITE
typedef enum
{
    SITHVEC_EV_MALLOC = 1,
    SITHVEC_EV_FREE,
    SITHVEC_EV_RAND,
    SITHVEC_EV_FINDSECTORINRADIUS,
    SITHVEC_EV_MOVETHING,
    SITHVEC_EV_SUITE = 0x40
} SithVecEvent;

static void sithVec_Ev(unsigned int id, unsigned int numArgs, ...)
{
    va_list va;
    va_start(va, numArgs);
    sithVecB_U8(&sithVec_ev, id);
    for ( unsigned int i = 0; i < numArgs; ++i )
    {
        sithVecB_U32(&sithVec_ev, sithVec_NormWord(va_arg(va, uint32_t)));
    }

    va_end(va);
}

// Appends the words of a block (vectors, structs) to the last event
static void sithVec_EvWords(const void* p, size_t size)
{
    for ( size_t k = 0; k + 4 <= size; k += 4 )
    {
        uint32_t w;
        memcpy(&w, (const uint8_t*)p + k, 4);
        sithVecB_U32(&sithVec_ev, sithVec_NormWord(w));
    }
}

static uint32_t sithVec_FloatBits(float f)
{
    uint32_t w;
    memcpy(&w, &f, sizeof(w));
    return w;
}

// ---------------------------------------------------------------------------------------------------------------
// Cases and suites
// ---------------------------------------------------------------------------------------------------------------
struct sSithVecCtx;

typedef struct sSithVecCase
{
    const char* pName;
    int (*pfRun)(struct sSithVecCtx* pCtx); // sets up the case, calls sithVec_Begin, then the tested function(s)
    int var;                                 // variant selector for the case function
    const void* pParams;                     // suite-specific parameters of the case
} SithVecCase;

#define SITHVEC_MAXCHECKPOINTS 64

typedef struct sSithVecCtx
{
    const SithVecCase* pCase;
    int bBegun;
    uint32_t numCheckpoints;
    SithVecBuf aImages[SITHVEC_MAXCHECKPOINTS + 1]; // watched memory at sithVec_Begin and at every checkpoint
    SithVecBuf aEvents[SITHVEC_MAXCHECKPOINTS];     // engine calls up to every checkpoint
} SithVecCtx;

typedef struct sSithVecSuite
{
    const char* pName;                      // fixture file name without .tv
    const SithVecCase* aCases;
    size_t numCases;
    void (*pfBuild)(const SithVecCase* pCase); // builds the suite state on top of sithVec_BuildWorld (may be NULL)
    void (*pfWatch)(void);                     // watches suite memory on top of the world (may be NULL)
    void (*pfSync)(void);                      // refreshes watched mirrors of engine variables (may be NULL)
} SithVecSuite;

static const SithVecSuite* sithVec_pSuite;

// Called by a case function right before it calls the tested function: takes the reference image
static void sithVec_Begin(SithVecCtx* pCtx)
{
    if ( sithVec_pSuite->pfSync )
    {
        sithVec_pSuite->pfSync();
    }

    sithVec_Capture(&pCtx->aImages[0]);
    sithVec_ev.size = 0;
    pCtx->bBegun    = 1;
}

// Records the engine calls and the state since the previous checkpoint (or sithVec_Begin). Case functions that call
// the tested function several times (frames) call it after every call; the runner adds the last one.
static void sithVec_Checkpoint(SithVecCtx* pCtx)
{
    if ( pCtx->numCheckpoints >= SITHVEC_MAXCHECKPOINTS )
    {
        SITHVEC_LOG("sithVec: too many checkpoints in %s\n", pCtx->pCase->pName);
        return;
    }

    if ( sithVec_pSuite->pfSync )
    {
        sithVec_pSuite->pfSync();
    }

    ++pCtx->numCheckpoints;
    sithVec_Capture(&pCtx->aImages[pCtx->numCheckpoints]);
    sithVecB_Put(&pCtx->aEvents[pCtx->numCheckpoints - 1], sithVec_ev.p, sithVec_ev.size);
    sithVec_ev.size = 0;
}

static void sithVec_FreeCtx(SithVecCtx* pCtx)
{
    for ( size_t i = 0; i < SITHVEC_ARRAYLEN(pCtx->aImages); ++i )
    {
        sithVecB_Free(&pCtx->aImages[i]);
    }

    for ( size_t i = 0; i < SITHVEC_ARRAYLEN(pCtx->aEvents); ++i )
    {
        sithVecB_Free(&pCtx->aEvents[i]);
    }
}

// Appends the state changes of a checkpoint: every byte that changed in either run. Both runs built the same state
// apart from the garbage (fill) bytes, so a byte a function did not write differs between the runs only where it
// still holds the fill, and a byte it wrote must be equal in both. Returns 0 when the runs disagree (the function read
// or wrote outside the state it was given, or depends on something that is not part of the case).
static int sithVec_Delta(SithVecBuf* pRec, const SithVecBuf* pPreA, const SithVecBuf* pPostA, const SithVecBuf* pPreB, const SithVecBuf* pPostB, size_t* pBadOffset)
{
    if ( pPreA->size != pPostA->size || pPreB->size != pPostA->size || pPostB->size != pPostA->size )
    {
        *pBadOffset = 0;
        return 0;
    }

    int bStable        = 1;
    SithVecBuf d       = { 0 };
    uint32_t numDeltas = 0;
    for ( size_t i = 0; i < pPostA->size; )
    {
        if ( pPreA->p[i] == pPostA->p[i] && pPreB->p[i] == pPostB->p[i] )
        {
            if ( bStable && pPostA->p[i] != pPostB->p[i] && (pPostA->p[i] != 0xA5 || pPostB->p[i] != 0x5A) )
            {
                bStable     = 0;
                *pBadOffset = i;
            }

            ++i;
            continue;
        }

        size_t j = i;
        while ( j < pPostA->size && (pPreA->p[j] != pPostA->p[j] || pPreB->p[j] != pPostB->p[j]) && j - i < 0xFFF0 )
        {
            if ( bStable && pPostA->p[j] != pPostB->p[j] )
            {
                bStable     = 0;
                *pBadOffset = j;
            }

            ++j;
        }

        sithVecB_U32(&d, (uint32_t)i);
        sithVecB_U16(&d, (uint32_t)(j - i));
        sithVecB_Put(&d, pPostA->p + i, j - i);
        ++numDeltas;
        i = j;
    }

    sithVecB_U32(pRec, numDeltas);
    sithVecB_Put(pRec, d.p, d.size);
    sithVecB_Free(&d);
    return bStable;
}

#endif // SITH_TESTS_VEC_SITHVEC_H
