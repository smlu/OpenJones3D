// Test side of the sith world-state vector harness: fixture loading and record comparison. Included once by the test
// source of a suite, after the suite's case header.
//
// Fixture layout (little endian): char magic[4] = "SVTV", u32 version = 1, u32 numCases, case[numCases]
//   case: u8 nameLen, char name[nameLen], u32 recSize, u8 rec[recSize]   (record: see sithVec.h)
#ifndef SITH_TESTS_VEC_SITHVECTEST_H
#define SITH_TESTS_VEC_SITHVECTEST_H

#include <unity_fixture.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sithVecWorld.h"

// Globals the reconstructed code reaches through J3D_LOCAL_SITH_RTI
// Added: A standalone build (the native Null platform) already defines these in the sith library
// (Libs/sith/Main/sithMain.c, Libs/sith/World/sithWorld.c); only the injected test build needs fakes.
#if !defined(J3D_STANDALONE)
tHostServices* sith_g_pHS;
SithWorld* sithWorld_g_pCurrentWorld;
SithWorld* sithWorld_g_pStaticWorld;
SithWorld* sithWorld_g_pLastLoadedWorld;
int sithWorld_g_bLoading;
#endif

typedef struct
{
    char aName[256];
    uint32_t recSize;
    uint8_t* pRec;
} SithVecTestCase;

typedef struct
{
    SithVecTestCase* aCases;
    unsigned int numCases;
} SithVecTestFixture;

static uint32_t sithVecTest_Rd32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void sithVecTest_LoadFixture(SithVecTestFixture* pFix, const char* pDir, const char* pSuiteName)
{
    char aPath[1024];
    snprintf(aPath, sizeof(aPath), "%s/%s.tv", pDir, pSuiteName);

    FILE* pFile = fopen(aPath, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(pFile, aPath);
    fseek(pFile, 0, SEEK_END);
    long size = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    uint8_t* pData = (uint8_t*)malloc((size_t)size);
    TEST_ASSERT_NOT_NULL(pData);
    TEST_ASSERT_EQUAL_INT((int)size, (int)fread(pData, 1, (size_t)size, pFile));
    fclose(pFile);

    TEST_ASSERT_EQUAL_MEMORY("SVTV", pData, 4);
    TEST_ASSERT_EQUAL_UINT32(1, sithVecTest_Rd32(pData + 4));
    pFix->numCases = sithVecTest_Rd32(pData + 8);
    pFix->aCases   = (SithVecTestCase*)calloc(pFix->numCases ? pFix->numCases : 1, sizeof(SithVecTestCase));

    const uint8_t* p = pData + 12;
    for ( unsigned int i = 0; i < pFix->numCases; ++i )
    {
        SithVecTestCase* pCase = &pFix->aCases[i];
        unsigned int nameLen   = *p++;
        memcpy(pCase->aName, p, nameLen);
        p += nameLen;

        pCase->recSize = sithVecTest_Rd32(p);
        p += 4;
        pCase->pRec = (uint8_t*)malloc(pCase->recSize ? pCase->recSize : 1);
        memcpy(pCase->pRec, p, pCase->recSize);
        p += pCase->recSize;
    }

    TEST_ASSERT_EQUAL_PTR(pData + size, p);
    free(pData);
}

static void sithVecTest_FreeFixture(SithVecTestFixture* pFix)
{
    for ( unsigned int i = 0; i < pFix->numCases; ++i )
    {
        free(pFix->aCases[i].pRec);
    }

    free(pFix->aCases);
    memset(pFix, 0, sizeof(*pFix));
}

// Every case of the suite must be in the fixture, in the same order, and the other way around
static void sithVecTest_AssertFixtureCoversSuite(const SithVecTestFixture* pFix, const SithVecSuite* pSuite)
{
    TEST_ASSERT_EQUAL_UINT((unsigned int)pSuite->numCases, pFix->numCases);
    for ( size_t i = 0; i < pSuite->numCases; ++i )
    {
        TEST_ASSERT_EQUAL_STRING(pSuite->aCases[i].pName, pFix->aCases[i].aName);
    }
}

// Describes the first difference of two records: the section (header, checkpoint n events or state changes) and the
// bytes from there
static void sithVecTest_Describe(char* pOut, size_t outSize, const uint8_t* pExp, size_t expSize, const uint8_t* pAct, size_t actSize)
{
    size_t n = expSize < actSize ? expSize : actSize;
    size_t i = 0;
    while ( i < n && pExp[i] == pAct[i] )
    {
        ++i;
    }

    char aSection[96];
    snprintf(aSection, sizeof(aSection), "header (return value, host call counts)");
    if ( i >= 16 && expSize >= 16 )
    {
        // Walk the checkpoints of the expected record
        uint32_t numCheckpoints = sithVecTest_Rd32(pExp + 12);
        size_t off              = 16;
        for ( uint32_t c = 0; c < numCheckpoints && off + 4 <= expSize; ++c )
        {
            uint32_t evSize = sithVecTest_Rd32(pExp + off);
            if ( i < off + 4 + evSize )
            {
                snprintf(aSection, sizeof(aSection), "checkpoint %u engine calls", c);
                break;
            }

            off += 4 + evSize;
            uint32_t numDeltas = sithVecTest_Rd32(pExp + off);
            size_t end         = off + 4;
            for ( uint32_t d = 0; d < numDeltas && end + 6 <= expSize; ++d )
            {
                end += 6 + (pExp[end + 4] | (pExp[end + 5] << 8));
            }

            if ( i < end )
            {
                snprintf(aSection, sizeof(aSection), "checkpoint %u state changes", c);
                break;
            }

            off = end;
        }
    }

    int len = snprintf(pOut, outSize, "sizes expected %u actual %u, first difference at %u in %s: expected", (unsigned int)expSize, (unsigned int)actSize, (unsigned int)i, aSection);
    for ( size_t k = i; k < i + 24 && k < expSize && len < (int)outSize - 8; ++k )
    {
        len += snprintf(pOut + len, outSize - len, " %02x", pExp[k]);
    }

    len += snprintf(pOut + len, outSize - len, " actual");
    for ( size_t k = i; k < i + 24 && k < actSize && len < (int)outSize - 8; ++k )
    {
        len += snprintf(pOut + len, outSize - len, " %02x", pAct[k]);
    }
}

// Prints the checkpoints of a record (environment SITHVEC_VERBOSE=1); offsets are into the watched memory, the arena
// (SithVecArena) first
static void sithVecTest_PrintRecord(const char* pTitle, const uint8_t* pRec, size_t recSize)
{
    if ( recSize < 16 )
    {
        return;
    }

    uint32_t numCheckpoints = sithVecTest_Rd32(pRec + 12);
    printf("    %s: ret %08x asserts/errors/warnings/mallocs %u/%u/%u/%u, %u checkpoints\n", pTitle, sithVecTest_Rd32(pRec),
        pRec[4] | (pRec[5] << 8), pRec[6] | (pRec[7] << 8), pRec[8] | (pRec[9] << 8), pRec[10] | (pRec[11] << 8), numCheckpoints);

    const uint8_t* p = pRec + 16;
    for ( uint32_t c = 0; c < numCheckpoints && p + 4 <= pRec + recSize; ++c )
    {
        uint32_t evSize = sithVecTest_Rd32(p);
        printf("      checkpoint %u: events %u bytes:", c, evSize);
        for ( uint32_t k = 0; k < evSize && k < 48; ++k )
        {
            printf(" %02x", p[4 + k]);
        }

        p += 4 + evSize;
        uint32_t numDeltas = sithVecTest_Rd32(p);
        printf("\n      %u state changes\n", numDeltas);
        p += 4;
        for ( uint32_t d = 0; d < numDeltas && p + 6 <= pRec + recSize; ++d )
        {
            uint32_t off     = sithVecTest_Rd32(p);
            unsigned int len = p[4] | (p[5] << 8);
            printf("        +%-8u len %-4u:", off, len);
            for ( unsigned int k = 0; k < len && k < 16; ++k )
            {
                printf(" %02x", p[6 + k]);
            }

            printf("\n");
            p += 6 + len;
        }
    }
}

// Runs the cases of the suite whose name starts with pPrefix (each with both garbage fills, see sithVec_RunCase) and
// compares their records with the fixture. Returns the number of failed cases; their names and first differences are appended to pFailText.
static unsigned int sithVecTest_RunCases(const SithVecSuite* pSuite, const SithVecTestFixture* pFix, const char* pPrefix, char* pFailText, size_t failTextSize)
{
    // Added: the images are recorded from the 32-bit retail build; pointers and size_t are 8 bytes on a 64-bit
    // build, so no reconstructed record can ever match. Every TestOriginal... case starts here, so they all
    // ignore themselves here instead of failing. Windows 32-bit behavior is unchanged.
    if ( sizeof(void*) != 4 )
    {
        TEST_IGNORE_MESSAGE("32-bit state images; needs a 32-bit build");
    }

    unsigned int numFailed = 0;
    unsigned int numFound  = 0;
    pFailText[0]           = 0;
    sithVec_pSuite         = pSuite;

    for ( unsigned int i = 0; i < pFix->numCases; ++i )
    {
        const SithVecTestCase* pFixCase = &pFix->aCases[i];
        if ( strncmp(pFixCase->aName, pPrefix, strlen(pPrefix)) != 0 )
        {
            continue;
        }

        const SithVecCase* pCase = NULL;
        for ( size_t k = 0; k < pSuite->numCases; ++k )
        {
            if ( strcmp(pSuite->aCases[k].pName, pFixCase->aName) == 0 )
            {
                pCase = &pSuite->aCases[k];
                break;
            }
        }

        TEST_ASSERT_NOT_NULL_MESSAGE(pCase, pFixCase->aName);
        ++numFound;

        SithVecBuf rec = { 0 };
        char aWhy[128];
        int bStable = sithVec_RunCase(pCase, &rec, aWhy, sizeof(aWhy));
        if ( !bStable || rec.size != pFixCase->recSize || memcmp(rec.p, pFixCase->pRec, rec.size) != 0 )
        {
            ++numFailed;
            char aDesc[768];
            if ( bStable )
            {
                sithVecTest_Describe(aDesc, sizeof(aDesc), pFixCase->pRec, pFixCase->recSize, rec.p, rec.size);
            }
            else
            {
                snprintf(aDesc, sizeof(aDesc), "the runs with fill 0xA5 and 0x5A disagree: %s", aWhy);
            }

            size_t len = strlen(pFailText);
            snprintf(pFailText + len, failTextSize - len, "\n  %s: %s", pFixCase->aName, aDesc);
#ifdef _WIN32
            if ( GetEnvironmentVariableA("SITHVEC_VERBOSE", NULL, 0) != 0 )
#else
            // Altered: GetEnvironmentVariableA is the Windows-only way this harness asks whether
            // SITHVEC_VERBOSE is set; the native build reads the same variable with getenv().
            if ( getenv("SITHVEC_VERBOSE") != NULL )
#endif
            {
                printf("  %s\n", pFixCase->aName);
                sithVecTest_PrintRecord("expected", pFixCase->pRec, pFixCase->recSize);
                sithVecTest_PrintRecord("actual  ", rec.p, rec.size);
            }
        }

        sithVecB_Free(&rec);
    }

    TEST_ASSERT_TRUE_MESSAGE(numFound > 0, pPrefix);
    return numFailed;
}

// Cases in which the reconstruction is known to differ from the original (documented in the fixture README). Fails
// on any other difference; reports the test as ignored while the listed cases still differ.
static void sithVecTest_ExpectKnownDifferences(unsigned int numFailed, const char* pFailText, const char* const* apKnown, unsigned int numKnown, const char* pIgnoreMessage)
{
    unsigned int numKnownFailed = 0;
    for ( unsigned int i = 0; i < numKnown; ++i )
    {
        char aNeedle[300];
        snprintf(aNeedle, sizeof(aNeedle), "\n  %s:", apKnown[i]);
        if ( strstr(pFailText, aNeedle) )
        {
            ++numKnownFailed;
        }
    }

    TEST_ASSERT_EQUAL_UINT_MESSAGE(numKnownFailed, numFailed, pFailText);
    if ( numKnownFailed )
    {
        TEST_IGNORE_MESSAGE(pIgnoreMessage);
    }
}

// The records normalize addresses of the synthetic state; that needs the arena far from every generated value
static void sithVecTest_AssertArenaPlacement(void)
{
    uintptr_t lo = (uintptr_t)&sithVec_arena;
    uintptr_t hi = lo + sizeof(sithVec_arena);
    TEST_ASSERT_TRUE_MESSAGE(lo >= 0x10000000u && hi < 0x3C000000u, "test image not loaded at its fixed base");
}

#endif // SITH_TESTS_VEC_SITHVECTEST_H
