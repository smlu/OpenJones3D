#include <unity_fixture.h>

#if defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sithDSSVecThing.h"

// Original-engine test vectors for the sithDSS and sithDSSThing functions. The fixtures in tv/sithDSS hold, for every case of
// dssv_aDSSCases, the result of the retail v1.0 function (return value, the messages and engine calls it made and the
// bytes it changed in the synthetic state). The tests rebuild the same state, run the reconstructed functions and compare.
// See tv/sithDSS/README.md.

// Globals the reconstructed code reaches through J3D_LOCAL_SITH_RTI
tHostServices* sith_g_pHS;
SithWorld* sithWorld_g_pCurrentWorld;
SithWorld* sithWorld_g_pStaticWorld;
SithWorld* sithWorld_g_pLastLoadedWorld;
int sithWorld_g_bLoading;

typedef struct
{
    char aName[96];
    int bHasIn;
    uint16_t inType;
    uint32_t inLength;
    uint8_t* pInData;
    uint32_t recSize;
    uint8_t* pRec;
} SithDSSTestFixCase;

typedef struct
{
    SithDSSTestFixCase* aCases;
    unsigned numCases;
} SithDSSTestFixture;

#if defined(_DEBUG)
static size_t sithDSSTest_previousDebugFillThreshold;
#endif

static SithDSSTestFixture sithDSSTest_dssFixture;
static SithDSSTestFixture sithDSSTest_thingFixture;
static unsigned sithDSSTest_numRun;

static uint32_t sithDSSTest_Rd32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void sithDSSTest_LoadFixture(SithDSSTestFixture* pFix, const char* pFile)
{
    char aPath[1024];
    snprintf(aPath, sizeof(aPath), "%s/%s", SITH_DSS_TV_DIR, pFile);

    FILE* f = fopen(aPath, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, aPath);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* pData = (uint8_t*)malloc((size_t)size);
    TEST_ASSERT_EQUAL_INT((int)size, (int)fread(pData, 1, (size_t)size, f));
    fclose(f);

    TEST_ASSERT_EQUAL_MEMORY("DSTV", pData, 4);
    TEST_ASSERT_EQUAL_UINT32(1, sithDSSTest_Rd32(pData + 4));
    pFix->numCases = sithDSSTest_Rd32(pData + 8);
    pFix->aCases = (SithDSSTestFixCase*)calloc(pFix->numCases, sizeof(SithDSSTestFixCase));

    const uint8_t* p = pData + 12;
    for ( unsigned i = 0; i < pFix->numCases; ++i )
    {
        SithDSSTestFixCase* c = &pFix->aCases[i];
        unsigned nameLen = *p++;
        memcpy(c->aName, p, nameLen);
        p += nameLen;
        c->bHasIn = *p++;
        if ( c->bHasIn )
        {
            c->inType   = (uint16_t)(p[0] | (p[1] << 8));
            c->inLength = sithDSSTest_Rd32(p + 2);
            p += 6;
            c->pInData = (uint8_t*)malloc(c->inLength ? c->inLength : 1);
            memcpy(c->pInData, p, c->inLength);
            p += c->inLength;
        }
        c->recSize = sithDSSTest_Rd32(p);
        p += 4;
        c->pRec = (uint8_t*)malloc(c->recSize);
        memcpy(c->pRec, p, c->recSize);
        p += c->recSize;
    }

    TEST_ASSERT_EQUAL_PTR(pData + size, p);
    free(pData);
}

static void sithDSSTest_FreeFixture(SithDSSTestFixture* pFix)
{
    for ( unsigned i = 0; i < pFix->numCases; ++i )
    {
        free(pFix->aCases[i].pInData);
        free(pFix->aCases[i].pRec);
    }
    free(pFix->aCases);
    memset(pFix, 0, sizeof(*pFix));
}

// Engine globals of the original executable, see sithDSSTestAddrs.h
SithDSSTestGlobals sithDSSTest_g;

static void sithDSSTest_Describe(char* pOut, size_t outSize, const uint8_t* pExp, size_t expSize, const uint8_t* pAct, size_t actSize)
{
    size_t n = expSize < actSize ? expSize : actSize;
    size_t i = 0;
    while ( i < n && pExp[i] == pAct[i] ) { ++i; }

    const char* pSection = "header (return value or host call counts)";
    if ( i >= 12 )
    {
        uint32_t evSize = sithDSSTest_Rd32(pExp + 12);
        pSection = i < 16 + evSize ? "engine calls/messages" : "state changes";
    }

    int len = snprintf(pOut, outSize, "sizes expected %u actual %u, first difference at %u in %s: expected", (unsigned)expSize, (unsigned)actSize, (unsigned)i, pSection);

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

// Names an offset in the watched image (see dssv_WatchAll): arena members first, then the engine globals
static void sithDSSTest_NameOffset(uint32_t off, char* pOut, size_t outSize)
{
#define SITHDSSTEST_MEMBER(member, count) { #member, offsetof(DssvArena, member), sizeof(((DssvArena*)0)->member[0]), count }
    static const struct { const char* pName; size_t offset; size_t elemSize; size_t count; } aMembers[] = {
        { "world", offsetof(DssvArena, world), sizeof(SithWorld), 1 }, { "staticWorld", offsetof(DssvArena, staticWorld), sizeof(SithWorld), 1 },
        SITHDSSTEST_MEMBER(sectors, DSSV_CAPSECTORS), SITHDSSTEST_MEMBER(surfaces, DSSV_CAPSURFACES), SITHDSSTEST_MEMBER(adjoins, 4),
        SITHDSSTEST_MEMBER(intens, DSSV_CAPSURFACES), SITHDSSTEST_MEMBER(mats, DSSV_CAPMATS), SITHDSSTEST_MEMBER(things, DSSV_CAPTHINGS),
        SITHDSSTEST_MEMBER(templates, DSSV_CAPTEMPLATES), SITHDSSTEST_MEMBER(models, DSSV_CAPMODELS), SITHDSSTEST_MEMBER(keys, DSSV_CAPKEYS),
        SITHDSSTEST_MEMBER(cogs, DSSV_CAPCOGS), SITHDSSTEST_MEMBER(aiClasses, DSSV_CAPAICLASSES), SITHDSSTEST_MEMBER(aiBlocks, 2),
        SITHDSSTEST_MEMBER(puppets, DSSV_CAPTHINGS), SITHDSSTEST_MEMBER(puppetStates, DSSV_CAPTHINGS), SITHDSSTEST_MEMBER(puppetTracks, DSSV_CAPTHINGS),
        { "puppetClass", offsetof(DssvArena, puppetClass), sizeof(SithPuppetClass), 1 }, SITHDSSTEST_MEMBER(swaps, DSSV_CAPTHINGS),
        SITHDSSTEST_MEMBER(anims, DSSV_NUMANIMS + 1), { "pool", offsetof(DssvArena, pool), 1, sizeof(((DssvArena*)0)->pool) },
        { "event", offsetof(DssvArena, event), sizeof(SithEvent), 1 }, { "mineCar", offsetof(DssvArena, mineCar), sizeof(SithMineCarControlsState), 1 },
        { "raft", offsetof(DssvArena, raft), sizeof(SithRaftControlsState), 1 }, { "aiFrames", offsetof(DssvArena, aiFrames), sizeof(rdVector3), 4 },
    };
#undef SITHDSSTEST_MEMBER
    if ( off >= sizeof(DssvArena) )
    {
        snprintf(pOut, outSize, "globals+%u", (unsigned)(off - (uint32_t)sizeof(DssvArena)));
        return;
    }

    const char* pBest = "arena";
    size_t bestOff = 0, bestIdx = 0, bestElem = 0;
    for ( size_t i = 0; i < sizeof(aMembers) / sizeof(aMembers[0]); ++i )
    {
        if ( off >= aMembers[i].offset && off < aMembers[i].offset + aMembers[i].elemSize * aMembers[i].count )
        {
            pBest   = aMembers[i].pName;
            bestOff = (off - aMembers[i].offset) % aMembers[i].elemSize;
            bestIdx = (off - aMembers[i].offset) / aMembers[i].elemSize;
            bestElem = aMembers[i].elemSize;
            break;
        }
    }
    if ( bestElem ) { snprintf(pOut, outSize, "%s[%u]+%u", pBest, (unsigned)bestIdx, (unsigned)bestOff); }
    else { snprintf(pOut, outSize, "arena+%u", off); }
}

// Prints the state changes of a record (SITHDSS_VERBOSE=1)
static void sithDSSTest_PrintDeltas(const char* pTitle, const uint8_t* pRec, size_t recSize)
{
    uint32_t evSize = sithDSSTest_Rd32(pRec + 12);
    const uint8_t* p = pRec + 16 + evSize;
    uint32_t numDeltas = sithDSSTest_Rd32(p);
    p += 4;
    printf("    %s: ret %08x asserts/errors/status/malloc %u/%u/%u/%u, events %u bytes, %u state changes\n", pTitle, sithDSSTest_Rd32(pRec),
        pRec[4] | (pRec[5] << 8), pRec[6] | (pRec[7] << 8), pRec[8] | (pRec[9] << 8), pRec[10] | (pRec[11] << 8), evSize, numDeltas);

    for ( uint32_t i = 0; i < numDeltas && p < pRec + recSize; ++i )
    {
        uint32_t off = sithDSSTest_Rd32(p);
        unsigned len = p[4] | (p[5] << 8);
        char aName[64];
        sithDSSTest_NameOffset(off, aName, sizeof(aName));
        printf("      %-28s len %u:", aName, len);
        for ( unsigned k = 0; k < len && k < 16; ++k ) { printf(" %02x", p[6 + k]); }
        printf("\n");
        p += 6 + len;
    }
}

// Runs the cases of aCases that are present in the fixture and whose name starts with one of the two prefixes
static unsigned sithDSSTest_RunCases(const DssvCase* aCases, unsigned numCases, const SithDSSTestFixture* pFix,
    const char* pPrefixA, const char* pPrefixB, char* pFailText, size_t failTextSize)
{
    unsigned numFailed = 0;
    unsigned numCasesFound = 0;
    pFailText[0] = 0;

    for ( unsigned i = 0; i < pFix->numCases; ++i )
    {
        const SithDSSTestFixCase* fc = &pFix->aCases[i];
        if ( strncmp(fc->aName, pPrefixA, strlen(pPrefixA)) != 0 && (!pPrefixB || strncmp(fc->aName, pPrefixB, strlen(pPrefixB)) != 0) )
        {
            continue;
        }

        const DssvCase* c = NULL;
        for ( unsigned k = 0; k < numCases; ++k )
        {
            if ( strcmp(aCases[k].name, fc->aName) == 0 ) { c = &aCases[k]; break; }
        }
        TEST_ASSERT_NOT_NULL_MESSAGE(c, fc->aName);
        ++numCasesFound;
        ++sithDSSTest_numRun;

        DssvMsgIn in;
        in.type   = fc->inType;
        in.length = fc->inLength;
        in.data   = fc->pInData;

        // Every case is run with both fill patterns the capture used to reject out of bounds accesses
        for ( uint32_t fill = 0xA5; fill != 0; fill = (fill == 0xA5 ? 0x5A : 0) )
        {
            DssvBuf rec = { 0 };
            dssv_RunCase(c, fill, fc->bHasIn ? &in : NULL, &rec);
            if ( rec.size != fc->recSize || memcmp(rec.p, fc->pRec, rec.size) != 0 )
            {
                ++numFailed;
                char aDesc[768];
                sithDSSTest_Describe(aDesc, sizeof(aDesc), fc->pRec, fc->recSize, rec.p, rec.size);
                size_t len = strlen(pFailText);
                snprintf(pFailText + len, failTextSize - len, "\n  %s (fill %02x): %s", fc->aName, fill, aDesc);
                if ( GetEnvironmentVariableA("SITHDSS_VERBOSE", NULL, 0) != 0 )
                {
                    printf("  %s\n", fc->aName);
                    sithDSSTest_PrintDeltas("expected", fc->pRec, fc->recSize);
                    sithDSSTest_PrintDeltas("actual  ", rec.p, rec.size);
                }
                dssvB_free(&rec);
                break;
            }
            dssvB_free(&rec);
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(numCasesFound > 0, pPrefixA);
    return numFailed;
}

// Cases in which the reconstruction differs from the original (documented in tv/sithDSS/README.md). The test fails on
// any other difference and is reported as ignored while the listed cases still differ.
static void sithDSSTest_ExpectKnownDifferences(unsigned numFailed, const char* pFailText, const char* const* apKnown, unsigned numKnown, const char* pIgnoreMessage)
{
    unsigned numKnownFailed = 0;
    for ( unsigned i = 0; i < numKnown; ++i )
    {
        if ( strstr(pFailText, apKnown[i]) ) { ++numKnownFailed; }
    }

    TEST_ASSERT_EQUAL_UINT_MESSAGE(numKnownFailed, numFailed, pFailText);
    if ( numKnownFailed )
    {
        TEST_IGNORE_MESSAGE(pIgnoreMessage);
    }
}

TEST_GROUP(sithDSS);

TEST_SETUP(sithDSS)
{
#if defined(_DEBUG)
    // The original zero pads fixed size names; suppress the Debug CRT's 0xFE tail fill of strncpy_s
    sithDSSTest_previousDebugFillThreshold = _CrtSetDebugFillThreshold(0u);
#endif
    // Addresses of the synthetic state must be far from the values the state is filled with (sithDSSVec.h)
    TEST_ASSERT_TRUE_MESSAGE((uintptr_t)&dssv_arena >= 0x10000000u && (uintptr_t)&dssv_arena + sizeof(dssv_arena) < 0x3C000000u, "test image not loaded at its fixed base");
    dssv_InitHostServices();
    sith_g_pHS = &dssv_hostServices;
    sithDSSTest_LoadFixture(&sithDSSTest_dssFixture, "sithDSS.tv");
}

TEST_TEAR_DOWN(sithDSS)
{
#if defined(_DEBUG)
    _CrtSetDebugFillThreshold(sithDSSTest_previousDebugFillThreshold);
#endif
    sithDSSTest_FreeFixture(&sithDSSTest_dssFixture);
    sith_g_pHS = NULL;
}

static char sithDSSTest_aFailText[8192];

#define SITHDSSTEST_FAMILY(name, prefixA, prefixB) \
    TEST(sithDSS, TestOriginal##name) \
    { \
        unsigned numFailed = sithDSSTest_RunCases(dssv_aDSSCases, (unsigned)DSSV_NUMDSSCASES, &sithDSSTest_dssFixture, prefixA, prefixB, sithDSSTest_aFailText, sizeof(sithDSSTest_aFailText)); \
        TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithDSSTest_aFailText); \
    }

SITHDSSTEST_FAMILY(SurfaceStatus, "SurfaceStatus_", "ProcessSurfaceStatus_")
SITHDSSTEST_FAMILY(SectorStatus, "SectorStatus_", "ProcessSectorStatus_")
SITHDSSTEST_FAMILY(SectorFlags, "SectorFlags_", "ProcessSectorFlags_")
SITHDSSTEST_FAMILY(SyncSeenSectors, "SyncSeenSectors", "ProcessSyncSeenSecors")
SITHDSSTEST_FAMILY(AIStatus, "AIStatus_", "ProcessAIStatus_")
SITHDSSTEST_FAMILY(Inventory, "Inventory_", "ProcessInventory_")
SITHDSSTEST_FAMILY(AnimStatus, "AnimStatus_", "ProcessAnimStatus_")
SITHDSSTEST_FAMILY(SyncTaskEvents, "SyncTaskEvents_", "ProcessSyncTaskEvents_")
SITHDSSTEST_FAMILY(SyncCameras, "SyncCameras_", "ProcessSyncCameras_")
// The synthetic state of the vectors holds a last chalk mark number up to 99, which the reconstruction rejects when the
// game state is read (marked Fixed: in sithDSS_ProcessSyncGameState; the original stores it unchecked). The six
// reader cases therefore differ on purpose, see tv/sithDSS/README.md; the writer cases still match.
TEST(sithDSS, TestOriginalSyncGameState)
{
    static const char* const aKnown[] = { "ProcessSyncGameState_full", "ProcessSyncGameState_empty", "ProcessSyncGameState_tenchalkmarks",
                                          "ProcessSyncGameState_edge_cogIntMax", "ProcessSyncGameState_edge_cogIntMin", "ProcessSyncGameState_edge_cogMinus2" };
    unsigned numFailed = sithDSSTest_RunCases(dssv_aDSSCases, (unsigned)DSSV_NUMDSSCASES, &sithDSSTest_dssFixture, "SyncGameState_", "ProcessSyncGameState_", sithDSSTest_aFailText, sizeof(sithDSSTest_aFailText));
    sithDSSTest_ExpectKnownDifferences(numFailed, sithDSSTest_aFailText, aKnown, sizeof(aKnown) / sizeof(aKnown[0]),
        "sithDSS_ProcessSyncGameState rejects the last chalk mark numbers of the vectors (intentional); see tv/sithDSS/README.md.");
}
SITHDSSTEST_FAMILY(VehicleControlState, "SyncVehicleControlState", "ProcessVehicleControlsState")
SITHDSSTEST_FAMILY(EndSection, "sub_4B3760", "sub_4B3790")

// The original stores the playback speed of a puppet track as it is. The reconstruction replaces a speed of zero or
// less with 1.0 on load (marked Added: in sithDSS_ProcessPuppetStatus), so these two cases differ on purpose.
TEST(sithDSS, TestOriginalPuppetStatus)
{
    static const char* const aKnown[] = { "ProcessPuppetStatus_zerospeed", "ProcessPuppetStatus_negativespeed" };
    unsigned numFailed = sithDSSTest_RunCases(dssv_aDSSCases, (unsigned)DSSV_NUMDSSCASES, &sithDSSTest_dssFixture, "PuppetStatus_", "ProcessPuppetStatus_", sithDSSTest_aFailText, sizeof(sithDSSTest_aFailText));
    sithDSSTest_ExpectKnownDifferences(numFailed, sithDSSTest_aFailText, aKnown, sizeof(aKnown) / sizeof(aKnown[0]),
        "sithDSS_ProcessPuppetStatus replaces a playback speed <= 0 with 1.0 (intentional); see tv/sithDSS/README.md.");
}

// Every case of the table must be in the fixture and the other way around
TEST(sithDSS, TestFixtureCoversAllCases)
{
    TEST_ASSERT_EQUAL_UINT(DSSV_NUMDSSCASES, sithDSSTest_dssFixture.numCases);
    for ( unsigned i = 0; i < DSSV_NUMDSSCASES; ++i )
    {
        TEST_ASSERT_EQUAL_STRING(dssv_aDSSCases[i].name, sithDSSTest_dssFixture.aCases[i].aName);
    }
}

TEST_GROUP_RUNNER(sithDSS)
{
    RUN_TEST_CASE(sithDSS, TestFixtureCoversAllCases);
    RUN_TEST_CASE(sithDSS, TestOriginalSurfaceStatus);
    RUN_TEST_CASE(sithDSS, TestOriginalSectorStatus);
    RUN_TEST_CASE(sithDSS, TestOriginalSectorFlags);
    RUN_TEST_CASE(sithDSS, TestOriginalSyncSeenSectors);
    RUN_TEST_CASE(sithDSS, TestOriginalAIStatus);
    RUN_TEST_CASE(sithDSS, TestOriginalInventory);
    RUN_TEST_CASE(sithDSS, TestOriginalAnimStatus);
    RUN_TEST_CASE(sithDSS, TestOriginalPuppetStatus);
    RUN_TEST_CASE(sithDSS, TestOriginalSyncTaskEvents);
    RUN_TEST_CASE(sithDSS, TestOriginalSyncCameras);
    RUN_TEST_CASE(sithDSS, TestOriginalSyncGameState);
    RUN_TEST_CASE(sithDSS, TestOriginalVehicleControlState);
    RUN_TEST_CASE(sithDSS, TestOriginalEndSection);
}

// ---------------------------------------------------------------------------------------------------------------
// sithDSSThing
// ---------------------------------------------------------------------------------------------------------------
TEST_GROUP(sithDSSThing);

TEST_SETUP(sithDSSThing)
{
#if defined(_DEBUG)
    // The original zero pads fixed size names; suppress the Debug CRT's 0xFE tail fill of strncpy_s
    sithDSSTest_previousDebugFillThreshold = _CrtSetDebugFillThreshold(0u);
#endif
    TEST_ASSERT_TRUE_MESSAGE((uintptr_t)&dssv_arena >= 0x10000000u && (uintptr_t)&dssv_arena + sizeof(dssv_arena) < 0x3C000000u, "test image not loaded at its fixed base");
    dssv_InitHostServices();
    sith_g_pHS = &dssv_hostServices;
    sithDSSTest_LoadFixture(&sithDSSTest_thingFixture, "sithDSSThing.tv");
}

TEST_TEAR_DOWN(sithDSSThing)
{
#if defined(_DEBUG)
    _CrtSetDebugFillThreshold(sithDSSTest_previousDebugFillThreshold);
#endif
    sithDSSTest_FreeFixture(&sithDSSTest_thingFixture);
    sith_g_pHS = NULL;
}

#define SITHDSSTHINGTEST_FAMILY(name, prefixA, prefixB) \
    TEST(sithDSSThing, TestOriginal##name) \
    { \
        unsigned numFailed = sithDSSTest_RunCases(dssv_aThingCases, (unsigned)DSSV_NUMTHINGCASES, &sithDSSTest_thingFixture, prefixA, prefixB, sithDSSTest_aFailText, sizeof(sithDSSTest_aFailText)); \
        TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithDSSTest_aFailText); \
    }

SITHDSSTHINGTEST_FAMILY(Pos, "Pos_", "ProcessPos_")
SITHDSSTHINGTEST_FAMILY(MovePos, "MovePos_", "ProcessMovePos_")
SITHDSSTHINGTEST_FAMILY(StateUpdate, "UpdateState_", "ProcessStateUpdate_")
SITHDSSTHINGTEST_FAMILY(PlaySound, "PlaySound_", "ProcessPlaySound_")
SITHDSSTHINGTEST_FAMILY(PlaySoundMode, "PlaySoundMode_", "ProcessPlaySoundMode_")
SITHDSSTHINGTEST_FAMILY(StopSound, "StopSound_", "ProcessStopSound_")
SITHDSSTHINGTEST_FAMILY(PlayKey, "PlayKey_", "ProcessPlayKey_")
SITHDSSTHINGTEST_FAMILY(PlayKeyMode, "PlayKeyMode_", "ProcessPlayKeyMode_")
SITHDSSTHINGTEST_FAMILY(SetModel, "SetModel_", "ProcessSetModel_")
SITHDSSTHINGTEST_FAMILY(StopKey, "StopKey_", "ProcessStopKey_")
SITHDSSTHINGTEST_FAMILY(Fire, "Fire_", "ProcessFire_")
SITHDSSTHINGTEST_FAMILY(Death, "Death_", "ProcessDeath_")
SITHDSSTHINGTEST_FAMILY(Damage, "DamageThing_", "ProcessDamage_")
SITHDSSTHINGTEST_FAMILY(PathMove, "PathMove_", "ProcessPathMove_")
SITHDSSTHINGTEST_FAMILY(Take, "Take_", "ProcessTake_")
SITHDSSTHINGTEST_FAMILY(CreateThing, "CreateThing_", "ProcessCreateThing_")
SITHDSSTHINGTEST_FAMILY(DestroyThing, "DestroyThing_", "ProcessDestroyThing")
SITHDSSTHINGTEST_FAMILY(MoveToPos, "MoveToPos_", NULL)

// Every case of the table must be in the fixture and the other way around
TEST(sithDSSThing, TestFixtureCoversAllCases)
{
    TEST_ASSERT_EQUAL_UINT(DSSV_NUMTHINGCASES, sithDSSTest_thingFixture.numCases);
    for ( unsigned i = 0; i < DSSV_NUMTHINGCASES; ++i )
    {
        TEST_ASSERT_EQUAL_STRING(dssv_aThingCases[i].name, sithDSSTest_thingFixture.aCases[i].aName);
    }
}

TEST_GROUP_RUNNER(sithDSSThing)
{
    RUN_TEST_CASE(sithDSSThing, TestFixtureCoversAllCases);
    RUN_TEST_CASE(sithDSSThing, TestOriginalPos);
    RUN_TEST_CASE(sithDSSThing, TestOriginalMovePos);
    RUN_TEST_CASE(sithDSSThing, TestOriginalStateUpdate);
    RUN_TEST_CASE(sithDSSThing, TestOriginalPlaySound);
    RUN_TEST_CASE(sithDSSThing, TestOriginalPlaySoundMode);
    RUN_TEST_CASE(sithDSSThing, TestOriginalStopSound);
    RUN_TEST_CASE(sithDSSThing, TestOriginalPlayKey);
    RUN_TEST_CASE(sithDSSThing, TestOriginalPlayKeyMode);
    RUN_TEST_CASE(sithDSSThing, TestOriginalSetModel);
    RUN_TEST_CASE(sithDSSThing, TestOriginalStopKey);
    RUN_TEST_CASE(sithDSSThing, TestOriginalFire);
    RUN_TEST_CASE(sithDSSThing, TestOriginalDeath);
    RUN_TEST_CASE(sithDSSThing, TestOriginalDamage);
    RUN_TEST_CASE(sithDSSThing, TestOriginalPathMove);
    RUN_TEST_CASE(sithDSSThing, TestOriginalTake);
    RUN_TEST_CASE(sithDSSThing, TestOriginalCreateThing);
    RUN_TEST_CASE(sithDSSThing, TestOriginalDestroyThing);
    RUN_TEST_CASE(sithDSSThing, TestOriginalMoveToPos);
}

// ---------------------------------------------------------------------------------------------------------------
// Hand-written tests of deliberate deviations from the original (marked Fixed: in the sources). The original-engine
// vectors above do not contain these inputs (see tv/sithDSS/README.md), so the cases are built here.
// ---------------------------------------------------------------------------------------------------------------
TEST_GROUP(sithDSSFix);

static SithMessage sithDSSFixTest_msg;
static SithDSSTestGlobals sithDSSFixTest_savedGlobals;
static char sithDSSFixTest_aConsoleText[256];
static unsigned sithDSSFixTest_numConsolePrints;

// Replaces sithConsole_PrintString in the tested sources (see sithMultiSeams.def)
void J3DAPI dssv_sithConsole_PrintString(const char* pString)
{
    snprintf(sithDSSFixTest_aConsoleText, sizeof(sithDSSFixTest_aConsoleText), "%s", pString);
    ++sithDSSFixTest_numConsolePrints;
}

TEST_SETUP(sithDSSFix)
{
    dssv_InitHostServices();
    sith_g_pHS = &dssv_hostServices;
    memset(&sithDSSTest_g, 0, sizeof(sithDSSTest_g));
    memset(&sithDSSFixTest_msg, 0, sizeof(sithDSSFixTest_msg));
    dssv_numAsserts = dssv_numErrors = dssv_numStatus = dssv_numMallocs = 0;
    sithDSSFixTest_aConsoleText[0] = 0;
    sithDSSFixTest_numConsolePrints = 0;
}

TEST_TEAR_DOWN(sithDSSFix)
{
    sith_g_pHS = NULL;
}

static void sithDSSFixTest_SaveGlobals(void)
{
    memcpy(&sithDSSFixTest_savedGlobals, &sithDSSTest_g, sizeof(sithDSSTest_g));
}

static void sithDSSFixTest_AssertGlobalsUnchanged(void)
{
    TEST_ASSERT_EQUAL_MEMORY(&sithDSSFixTest_savedGlobals, &sithDSSTest_g, sizeof(sithDSSTest_g));
}

static void sithDSSFixTest_BuildWelcome(int playerNum)
{
    SITHDSS_STARTOUTEX(&sithDSSFixTest_msg, SITHDSS_WELCOME);
    SITHDSS_PUSHINT32(playerNum);
    SITHDSS_PUSHINT32(0x1234); // player id, not the local player
    memset(pCurDSSMsgData_, 'A', 32); // player name
    pCurDSSMsgData_ += 32;
    SITHDSS_ENDOUT;
}

// sithMulti_ProcessWelcome indexed sithPlayer_g_aPlayers with the player number of the message unchecked
TEST(sithDSSFix, WelcomeRejectsInvalidPlayerNum)
{
    static const int aBad[] = { SITHPLAYER_MAX_PLAYERS, SITHPLAYER_MAX_PLAYERS + 1, 1000, -1, INT_MIN };
    for ( size_t i = 0; i < sizeof(aBad) / sizeof(aBad[0]); ++i )
    {
        dssv_numErrors = 0;
        sithDSSFixTest_SaveGlobals();
        sithDSSFixTest_BuildWelcome(aBad[i]);

        TEST_ASSERT_EQUAL_INT_MESSAGE(0, sithMulti_ProcessWelcome(&sithDSSFixTest_msg), "invalid player number accepted");
        TEST_ASSERT_EQUAL_UINT(1, dssv_numErrors);
        sithDSSFixTest_AssertGlobalsUnchanged();
    }
}

static void sithDSSFixTest_BuildSyncPlayers(unsigned numPlayers)
{
    SITHDSS_STARTOUTEX(&sithDSSFixTest_msg, SITHDSS_SYNCPLAYERS);
    SITHDSS_PUSHUINT32(0x7001); // server id
    SITHDSS_PUSHUINT16((uint16_t)numPlayers);
    memset(pCurDSSMsgData_, 0, 4 * SITHPLAYER_MAX_PLAYERS); // flags of the players, none set
    SITHDSS_ENDOUT;
}

// sithMulti_ProcessSyncPlayers looped over sithPlayer_g_aPlayers with the player count of the message unchecked
TEST(sithDSSFix, SyncPlayersRejectsTooManyPlayers)
{
    static const unsigned aBad[] = { SITHPLAYER_MAX_PLAYERS + 1, 100, 0xFFFF };
    for ( size_t i = 0; i < sizeof(aBad) / sizeof(aBad[0]); ++i )
    {
        dssv_numErrors = 0;
        sithDSSTest_g.numPlayers = 3;
        memset(&sithDSSTest_g.aPlayers, 0x5A, sizeof(sithDSSTest_g.aPlayers));
        sithDSSFixTest_SaveGlobals();
        sithDSSFixTest_BuildSyncPlayers(aBad[i]);

        TEST_ASSERT_EQUAL_INT_MESSAGE(0, sithMulti_ProcessSyncPlayers(&sithDSSFixTest_msg), "invalid player count accepted");
        TEST_ASSERT_EQUAL_UINT(1, dssv_numErrors);

        // The message is ignored: the player count and the players stay as they were (the server id is stored before the count is read)
        TEST_ASSERT_EQUAL_UINT(3, sithDSSTest_g.numPlayers);
        TEST_ASSERT_EQUAL_MEMORY(&sithDSSFixTest_savedGlobals.aPlayers, &sithDSSTest_g.aPlayers, sizeof(sithDSSTest_g.aPlayers));
    }
}

// The full player array is still accepted
TEST(sithDSSFix, SyncPlayersAcceptsMaxPlayers)
{
    sithDSSFixTest_BuildSyncPlayers(SITHPLAYER_MAX_PLAYERS);
    TEST_ASSERT_EQUAL_INT(1, sithMulti_ProcessSyncPlayers(&sithDSSFixTest_msg));
    TEST_ASSERT_EQUAL_UINT(SITHPLAYER_MAX_PLAYERS, sithDSSTest_g.numPlayers);
    TEST_ASSERT_EQUAL_UINT(0, dssv_numErrors);
}

static void sithDSSFixTest_BuildChat(int playerCount, uint32_t len, const char* pText, size_t textSize)
{
    SITHDSS_STARTOUTEX(&sithDSSFixTest_msg, SITHDSS_CHAT);
    SITHDSS_PUSHINT32(0); // sender count, unused
    SITHDSS_PUSHINT32(playerCount);
    SITHDSS_PUSHUINT32(len);
    memcpy(pCurDSSMsgData_, pText, textSize);
    pCurDSSMsgData_ += textSize;
    SITHDSS_ENDOUT;
}

// sithMulti_ProcessChat wrapped len - 1 for an empty text and copied and terminated the text outside of its buffer
TEST(sithDSSFix, ChatRejectsEmptyText)
{
    sithDSSFixTest_BuildChat(-1, 0, "hello", 6);
    TEST_ASSERT_EQUAL_INT(0, sithMulti_ProcessChat(&sithDSSFixTest_msg));
    TEST_ASSERT_EQUAL_UINT(1, dssv_numErrors);
    TEST_ASSERT_EQUAL_UINT(0, sithDSSFixTest_numConsolePrints);
}

// A chat message of a negative player count (server message) prints the text as it is
TEST(sithDSSFix, ChatPrintsServerText)
{
    sithDSSFixTest_BuildChat(-1, 6, "hello", 6);
    TEST_ASSERT_EQUAL_INT(1, sithMulti_ProcessChat(&sithDSSFixTest_msg));
    TEST_ASSERT_EQUAL_UINT(0, dssv_numErrors);
    TEST_ASSERT_EQUAL_UINT(1, sithDSSFixTest_numConsolePrints);
    TEST_ASSERT_EQUAL_STRING("hello", sithDSSFixTest_aConsoleText);

    // len includes the terminator, the shortest accepted text (len 1) is the empty string
    sithDSSFixTest_numConsolePrints = 0;
    sithDSSFixTest_BuildChat(-1, 1, "x", 2);
    TEST_ASSERT_EQUAL_INT(1, sithMulti_ProcessChat(&sithDSSFixTest_msg));
    TEST_ASSERT_EQUAL_UINT(1, sithDSSFixTest_numConsolePrints);
    TEST_ASSERT_EQUAL_STRING("", sithDSSFixTest_aConsoleText);
}

// sithMulti_ProcessChat indexed sithPlayer_g_aPlayers with any non-negative sender of the message
TEST(sithDSSFix, ChatNamesOnlyListedSenders)
{
    sithDSSTest_g.numPlayers = 3;
    memset(&sithDSSTest_g.aPlayers, 0xA5, sizeof(sithDSSTest_g.aPlayers));
    sithDSSTest_g.aPlayers[2].awName[0] = L'B';
    sithDSSTest_g.aPlayers[2].awName[1] = L'o';
    sithDSSTest_g.aPlayers[2].awName[2] = L'b';
    sithDSSTest_g.aPlayers[2].awName[3] = 0;
    sithDSSTest_g.aPlayers[3].awName[0] = L'E';
    sithDSSTest_g.aPlayers[3].awName[1] = L'v';
    sithDSSTest_g.aPlayers[3].awName[2] = L'e';
    sithDSSTest_g.aPlayers[3].awName[3] = 0;

    sithDSSFixTest_BuildChat(2, 4, "hi!", 4);
    TEST_ASSERT_EQUAL_INT(1, sithMulti_ProcessChat(&sithDSSFixTest_msg));
    TEST_ASSERT_EQUAL_STRING("Bob says 'hi!'", sithDSSFixTest_aConsoleText);

    // A slot behind the player count is still inside the array, the name of the slot is printed as in the original
    sithDSSFixTest_BuildChat(3, 4, "hi!", 4);
    TEST_ASSERT_EQUAL_INT(1, sithMulti_ProcessChat(&sithDSSFixTest_msg));
    TEST_ASSERT_EQUAL_STRING("Eve says 'hi!'", sithDSSFixTest_aConsoleText);

    static const int aOutside[] = { SITHPLAYER_MAX_PLAYERS, 1000, INT_MAX };
    for ( size_t i = 0; i < sizeof(aOutside) / sizeof(aOutside[0]); ++i )
    {
        sithDSSFixTest_aConsoleText[0] = 0;
        sithDSSFixTest_BuildChat(aOutside[i], 4, "hi!", 4);
        TEST_ASSERT_EQUAL_INT(1, sithMulti_ProcessChat(&sithDSSFixTest_msg));
        TEST_ASSERT_EQUAL_STRING("hi!", sithDSSFixTest_aConsoleText);
    }
}

// Runs a writer case and keeps its message in dssv_lastMsg
static void sithDSSFixTest_RunWriter(const DssvCase* pCase)
{
    DssvBuf rec = { 0 };
    dssv_RunCase(pCase, 0xA5, NULL, &rec);
    dssvB_free(&rec);
    TEST_ASSERT_TRUE_MESSAGE(dssv_lastMsg.valid, pCase->name);
}

// Feeds dssv_lastMsg (as edited by the test) to a reader case and returns the result of the reader
static uint32_t sithDSSFixTest_RunReader(const DssvCase* pCase)
{
    DssvMsgIn in;
    in.type   = dssv_lastMsg.type;
    in.length = dssv_lastMsg.length;
    in.data   = dssv_lastMsg.data;

    DssvBuf rec = { 0 };
    dssv_RunCase(pCase, 0xA5, &in, &rec);
    uint32_t ret = sithDSSTest_Rd32(rec.p);
    dssvB_free(&rec);
    return ret;
}

// Offset of the first 32 bit word with the given value in the message of the last writer
static size_t sithDSSFixTest_FindWord(uint32_t word)
{
    size_t off = 0;
    while ( off + 4 <= dssv_lastMsg.length && sithDSSTest_Rd32(dssv_lastMsg.data + off) != word )
    {
        ++off;
    }

    TEST_ASSERT_TRUE_MESSAGE(off + 4 <= dssv_lastMsg.length, "word not found in message");
    return off;
}

static void sithDSSFixTest_SetWord(size_t off, uint32_t word)
{
    memcpy(dssv_lastMsg.data + off, &word, sizeof(word));
}

static int sithDSSFixTest_WriteGameStateChalk(DssvCtx* cx)
{
    sithFX_g_numChalkMarks    = 3;
    sithFX_g_lastChalkMarkNum = 0x12345678; // marks the chalk mark count in the message (the number follows the count)
    return dssvc_SyncGameState(cx);
}

// sithDSS_ProcessSyncGameState stored the chalk marks of the message past the end of aChalkMarks
TEST(sithDSSFix, SyncGameStateRejectsTooManyChalkMarks)
{
    static const DssvCase writer = { "Fix_SyncGameState_chalk", sithDSSFixTest_WriteGameStateChalk, 0, NULL, DSSV_NOMUT };
    static const DssvCase reader = { "Fix_ProcessSyncGameState_chalk", dssvc_ProcessSyncGameState, 0, NULL, DSSV_NOMUT };

    sithDSSFixTest_RunWriter(&writer);
    size_t countOff = sithDSSFixTest_FindWord(0x12345678) - 4;
    TEST_ASSERT_EQUAL_UINT32(3, sithDSSTest_Rd32(dssv_lastMsg.data + countOff));
    sithDSSFixTest_SetWord(countOff + 4, 5); // a valid last chalk mark number

    static const uint32_t aBad[] = { 11, 100, 0x7FFFFFFF, 0xFFFFFFFF };
    for ( size_t i = 0; i < sizeof(aBad) / sizeof(aBad[0]); ++i )
    {
        sithDSSFixTest_SetWord(countOff, aBad[i]);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, sithDSSFixTest_RunReader(&reader), "invalid chalk mark count accepted");
        TEST_ASSERT_EQUAL_UINT(1, dssv_numErrors);

        // The chalk marks of the state are kept (the reader case starts with 3 marks)
        TEST_ASSERT_EQUAL_INT(3, sithFX_g_numChalkMarks);
        for ( size_t k = 3; k < STD_ARRAYLEN(sithFX_g_aChalkMarks); ++k )
        {
            TEST_ASSERT_NULL(sithFX_g_aChalkMarks[k]);
        }
    }

    // The full array is accepted
    sithDSSFixTest_SetWord(countOff, 10);
    TEST_ASSERT_EQUAL_UINT32(1, sithDSSFixTest_RunReader(&reader));
    TEST_ASSERT_EQUAL_INT(10, sithFX_g_numChalkMarks);
}

// sithDSS_ProcessSyncGameState stored the last chalk mark number unchecked, sithFX indexes aChalkMarks after it
TEST(sithDSSFix, SyncGameStateRejectsBadLastChalkMark)
{
    static const DssvCase writer = { "Fix_SyncGameState_lastchalk", sithDSSFixTest_WriteGameStateChalk, 0, NULL, DSSV_NOMUT };
    static const DssvCase reader = { "Fix_ProcessSyncGameState_lastchalk", dssvc_ProcessSyncGameState, 0, NULL, DSSV_NOMUT };

    sithDSSFixTest_RunWriter(&writer);
    size_t lastOff = sithDSSFixTest_FindWord(0x12345678);

    static const uint32_t aBad[] = { 0xFFFFFFFE, 0xFFFFFF9C, 0x80000000, 10, 100, 0x7FFFFFFF };
    for ( size_t i = 0; i < sizeof(aBad) / sizeof(aBad[0]); ++i )
    {
        sithDSSFixTest_SetWord(lastOff, aBad[i]);
        dssv_numErrors = 0;
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, sithDSSFixTest_RunReader(&reader), "invalid last chalk mark number accepted");
        TEST_ASSERT_EQUAL_UINT(1, dssv_numErrors);
        TEST_ASSERT_EQUAL_INT(3, sithFX_g_numChalkMarks); // not stored
    }

    // -1 (none) and the last slot are accepted
    static const uint32_t aGood[] = { 0xFFFFFFFF, 0, 9 };
    for ( size_t i = 0; i < sizeof(aGood) / sizeof(aGood[0]); ++i )
    {
        sithDSSFixTest_SetWord(lastOff, aGood[i]);
        TEST_ASSERT_EQUAL_UINT32(1, sithDSSFixTest_RunReader(&reader));
        TEST_ASSERT_EQUAL_INT((int)aGood[i], sithFX_g_lastChalkMarkNum);
    }
}

// sithDSSThing_ProcessStateUpdate stored the backpack items of the message past the end of aBackpackItems
TEST(sithDSSFix, StateUpdateRejectsTooManyBackpackItems)
{
    // The writer case of the vectors: an item with a backpack of 3 items (type ids 10, 13, 16)
    static const DssvCase writer = { "Fix_UpdateState_backpack", dssvc_UpdateState, 3, NULL, DSSV_NOMUT };
    static const DssvCase reader = { "Fix_ProcessStateUpdate_backpack", dssvc_ProcessStateUpdate, 0, NULL, DSSV_NOMUT };

    sithDSSFixTest_RunWriter(&writer);

    // Find the item count: int16 3 followed by the type id 10 of the first item
    size_t countOff = 0;
    while ( countOff + 4 <= dssv_lastMsg.length && !(dssv_lastMsg.data[countOff] == 3 && dssv_lastMsg.data[countOff + 1] == 0
                                                     && dssv_lastMsg.data[countOff + 2] == 10 && dssv_lastMsg.data[countOff + 3] == 0) )
    {
        ++countOff;
    }
    TEST_ASSERT_TRUE_MESSAGE(countOff + 4 <= dssv_lastMsg.length, "item count not found in message");

    static const uint16_t aBad[] = { 33, 1000, 0x7FFF, 0xFFFF };
    for ( size_t i = 0; i < sizeof(aBad) / sizeof(aBad[0]); ++i )
    {
        memcpy(dssv_lastMsg.data + countOff, &aBad[i], sizeof(aBad[i]));
        dssv_numErrors = 0;
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, sithDSSFixTest_RunReader(&reader), "invalid backpack item count accepted");
        TEST_ASSERT_EQUAL_UINT(1, dssv_numErrors);
        TEST_ASSERT_NOT_EQUAL((int16_t)aBad[i], dssv_arena.things[2].thingInfo.itemInfo.numItems);
    }

    // The original message and a full backpack are accepted
    static const uint16_t aGood[] = { 3, 32 };
    for ( size_t i = 0; i < sizeof(aGood) / sizeof(aGood[0]); ++i )
    {
        memcpy(dssv_lastMsg.data + countOff, &aGood[i], sizeof(aGood[i]));
        TEST_ASSERT_EQUAL_UINT32(1, sithDSSFixTest_RunReader(&reader));
        TEST_ASSERT_EQUAL_INT(aGood[i], dssv_arena.things[2].thingInfo.itemInfo.numItems);
    }
}

TEST_GROUP_RUNNER(sithDSSFix)
{
    RUN_TEST_CASE(sithDSSFix, WelcomeRejectsInvalidPlayerNum);
    RUN_TEST_CASE(sithDSSFix, SyncPlayersRejectsTooManyPlayers);
    RUN_TEST_CASE(sithDSSFix, SyncPlayersAcceptsMaxPlayers);
    RUN_TEST_CASE(sithDSSFix, ChatRejectsEmptyText);
    RUN_TEST_CASE(sithDSSFix, ChatPrintsServerText);
    RUN_TEST_CASE(sithDSSFix, ChatNamesOnlyListedSenders);
    RUN_TEST_CASE(sithDSSFix, SyncGameStateRejectsTooManyChalkMarks);
    RUN_TEST_CASE(sithDSSFix, SyncGameStateRejectsBadLastChalkMark);
    RUN_TEST_CASE(sithDSSFix, StateUpdateRejectsTooManyBackpackItems);
}
