#include <unity_fixture.h>

#if defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sithMultiVec.h"

// Original-engine test vectors for the sithMulti functions. tv/sithMulti/sithMulti.tv holds, for every case of
// msv_aCases, the result of the retail v1.0 function (return value, host service calls, the messages and engine calls
// it made and the bytes it changed in the synthetic state, the players and the module state). The tests rebuild the
// same state, run the reconstructed functions and compare. See tv/sithMulti/README.md.

// Engine globals of the original executable, see sithDSSTestAddrs.h and sithMultiTestAddrs.h
SithDSSTestGlobals sithDSSTest_g;
SithMultiTestGlobals sithMultiTest_g;

typedef struct
{
    char aName[96];
    int bHasIn;
    uint16_t inType;
    uint32_t inLength;
    uint8_t* pInData;
    uint32_t recSize;
    uint8_t* pRec;
} SithMultiTestFixCase;

static SithMultiTestFixCase* sithMultiTest_aCases;
static unsigned sithMultiTest_numCases;
static char sithMultiTest_aFailText[8192];

#if defined(_DEBUG)
static size_t sithMultiTest_previousDebugFillThreshold;
#endif

static uint32_t sithMultiTest_Rd32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void sithMultiTest_LoadFixture(void)
{
    char aPath[1024];
    snprintf(aPath, sizeof(aPath), "%s/%s", SITH_MULTI_TV_DIR, "sithMulti.tv");

    FILE* f = fopen(aPath, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, aPath);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* pData = (uint8_t*)malloc((size_t)size);
    TEST_ASSERT_EQUAL_INT((int)size, (int)fread(pData, 1, (size_t)size, f));
    fclose(f);

    TEST_ASSERT_EQUAL_MEMORY("DSTV", pData, 4);
    TEST_ASSERT_EQUAL_UINT32(1, sithMultiTest_Rd32(pData + 4));
    sithMultiTest_numCases = sithMultiTest_Rd32(pData + 8);
    sithMultiTest_aCases   = (SithMultiTestFixCase*)calloc(sithMultiTest_numCases, sizeof(SithMultiTestFixCase));

    const uint8_t* p = pData + 12;
    for ( unsigned i = 0; i < sithMultiTest_numCases; ++i )
    {
        SithMultiTestFixCase* c = &sithMultiTest_aCases[i];
        unsigned nameLen = *p++;
        memcpy(c->aName, p, nameLen);
        p += nameLen;
        c->bHasIn = *p++;
        if ( c->bHasIn )
        {
            c->inType   = (uint16_t)(p[0] | (p[1] << 8));
            c->inLength = sithMultiTest_Rd32(p + 2);
            p += 6;
            c->pInData = (uint8_t*)malloc(c->inLength ? c->inLength : 1);
            memcpy(c->pInData, p, c->inLength);
            p += c->inLength;
        }
        c->recSize = sithMultiTest_Rd32(p);
        p += 4;
        c->pRec = (uint8_t*)malloc(c->recSize);
        memcpy(c->pRec, p, c->recSize);
        p += c->recSize;
    }

    TEST_ASSERT_EQUAL_PTR(pData + size, p);
    free(pData);
}

static void sithMultiTest_FreeFixture(void)
{
    for ( unsigned i = 0; i < sithMultiTest_numCases; ++i )
    {
        free(sithMultiTest_aCases[i].pInData);
        free(sithMultiTest_aCases[i].pRec);
    }
    free(sithMultiTest_aCases);
    sithMultiTest_aCases   = NULL;
    sithMultiTest_numCases = 0;
}

static void sithMultiTest_Describe(char* pOut, size_t outSize, const uint8_t* pExp, size_t expSize, const uint8_t* pAct, size_t actSize)
{
    size_t n = expSize < actSize ? expSize : actSize;
    size_t i = 0;
    while ( i < n && pExp[i] == pAct[i] ) { ++i; }

    const char* pSection = "header (return value or host call counts)";
    if ( i >= 12 )
    {
        uint32_t evSize = sithMultiTest_Rd32(pExp + 12);
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

// Prints the state changes of a record (SITHMULTI_VERBOSE=1); offsets past the arena are into the watched globals
static void sithMultiTest_PrintDeltas(const char* pTitle, const uint8_t* pRec, size_t recSize)
{
    uint32_t evSize = sithMultiTest_Rd32(pRec + 12);
    const uint8_t* p = pRec + 16 + evSize;
    uint32_t numDeltas = sithMultiTest_Rd32(p);
    p += 4;
    printf("    %s: ret %08x asserts/errors/status/warnings %u/%u/%u/%u, events %u bytes, %u state changes\n", pTitle, sithMultiTest_Rd32(pRec),
        pRec[4] | (pRec[5] << 8), pRec[6] | (pRec[7] << 8), pRec[8] | (pRec[9] << 8), pRec[10] | (pRec[11] << 8), evSize, numDeltas);

    for ( uint32_t i = 0; i < numDeltas && p < pRec + recSize; ++i )
    {
        uint32_t off = sithMultiTest_Rd32(p);
        unsigned len = p[4] | (p[5] << 8);
        if ( off >= sizeof(DssvArena) ) { printf("      globals+%-20u len %u:", (unsigned)(off - (uint32_t)sizeof(DssvArena)), len); }
        else { printf("      arena+%-22u len %u:", (unsigned)off, len); }
        for ( unsigned k = 0; k < len && k < 16; ++k ) { printf(" %02x", p[6 + k]); }
        printf("\n");
        p += 6 + len;
    }
}

// Runs the cases of the fixture whose name starts with pPrefix (both probe fills) and returns the number that differ
static unsigned sithMultiTest_RunCases(const char* pPrefix)
{
    // Added: the images are recorded from the 32-bit retail build; pointers and size_t are 8 bytes on a 64-bit
    // build, so no reconstructed record can ever match. Every TestOriginal... case starts here, so they all
    // ignore themselves here instead of failing. Windows 32-bit behavior is unchanged.
    if ( sizeof(void*) != 4 )
    {
        TEST_IGNORE_MESSAGE("32-bit state images; needs a 32-bit build");
    }

    unsigned numFailed = 0;
    unsigned numCasesFound = 0;
    sithMultiTest_aFailText[0] = 0;

    for ( unsigned i = 0; i < sithMultiTest_numCases; ++i )
    {
        const SithMultiTestFixCase* fc = &sithMultiTest_aCases[i];
        if ( strncmp(fc->aName, pPrefix, strlen(pPrefix)) != 0 )
        {
            continue;
        }

        const DssvCase* c = NULL;
        for ( unsigned k = 0; k < MSV_NUMCASES; ++k )
        {
            if ( strcmp(msv_aCases[k].name, fc->aName) == 0 ) { c = &msv_aCases[k]; break; }
        }
        TEST_ASSERT_NOT_NULL_MESSAGE(c, fc->aName);
        ++numCasesFound;

        DssvMsgIn in;
        in.type   = fc->inType;
        in.length = fc->inLength;
        in.data   = fc->pInData;

        for ( uint32_t fill = 0xA5; fill != 0; fill = (fill == 0xA5 ? 0x5A : 0) )
        {
            DssvBuf rec = { 0 };
            msv_RunCase(c, fill, fc->bHasIn ? &in : NULL, &rec);
            if ( rec.size != fc->recSize || memcmp(rec.p, fc->pRec, rec.size) != 0 )
            {
                ++numFailed;
                char aDesc[768];
                sithMultiTest_Describe(aDesc, sizeof(aDesc), fc->pRec, fc->recSize, rec.p, rec.size);
                size_t len = strlen(sithMultiTest_aFailText);
                snprintf(sithMultiTest_aFailText + len, sizeof(sithMultiTest_aFailText) - len, "\n  %s (fill %02x): %s", fc->aName, fill, aDesc);
#ifdef _WIN32
                if ( GetEnvironmentVariableA("SITHMULTI_VERBOSE", NULL, 0) != 0 )
#else
                // Altered: GetEnvironmentVariableA is the Windows-only way this harness asks whether
                // SITHMULTI_VERBOSE is set; the native build reads the same variable with getenv().
                if ( getenv("SITHMULTI_VERBOSE") != NULL )
#endif
                {
                    printf("  %s\n", fc->aName);
                    sithMultiTest_PrintDeltas("expected", fc->pRec, fc->recSize);
                    sithMultiTest_PrintDeltas("actual  ", rec.p, rec.size);
                }
                dssvB_free(&rec);
                break;
            }
            dssvB_free(&rec);
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(numCasesFound > 0, pPrefix);
    return numFailed;
}

// Cases in which the reconstruction differs from the original (tv/sithMulti/README.md, Known Differences). The test
// fails on any other difference and is reported as ignored while the listed cases still differ.
static void sithMultiTest_ExpectKnownDifferences(unsigned numFailed, const char* const* apKnown, unsigned numKnown, const char* pIgnoreMessage)
{
    unsigned numKnownFailed = 0;
    for ( unsigned i = 0; i < numKnown; ++i )
    {
        if ( strstr(sithMultiTest_aFailText, apKnown[i]) ) { ++numKnownFailed; }
    }

    TEST_ASSERT_EQUAL_UINT_MESSAGE(numKnownFailed, numFailed, sithMultiTest_aFailText);
    if ( numKnownFailed )
    {
        TEST_IGNORE_MESSAGE(pIgnoreMessage);
    }
}

TEST_GROUP(sithMulti);

TEST_SETUP(sithMulti)
{
#if defined(_DEBUG)
    // The original zero pads fixed size names; suppress the Debug CRT's 0xFE tail fill of the secure string functions
    sithMultiTest_previousDebugFillThreshold = _CrtSetDebugFillThreshold(0u);
#endif
    // Addresses of the synthetic state must be far from the values the state is filled with (sithDSSVec.h)
    TEST_ASSERT_TRUE_MESSAGE((uintptr_t)&dssv_arena >= 0x10000000u && (uintptr_t)&dssv_arena + sizeof(dssv_arena) < 0x3C000000u, "test image not loaded at its fixed base");
    dssv_InitHostServices();
    sith_g_pHS = &dssv_hostServices;
    sithMultiTest_LoadFixture();
}

TEST_TEAR_DOWN(sithMulti)
{
#if defined(_DEBUG)
    _CrtSetDebugFillThreshold(sithMultiTest_previousDebugFillThreshold);
#endif
    sithMultiTest_FreeFixture();
    sith_g_pHS = NULL;
}

#define SITHMULTITEST_FAMILY(name, prefix) \
    TEST(sithMulti, TestOriginal##name) \
    { \
        unsigned numFailed = sithMultiTest_RunCases(prefix); \
        TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithMultiTest_aFailText); \
    }

SITHMULTITEST_FAMILY(CloseGame, "CloseGame_")
SITHMULTITEST_FAMILY(CheckPlayers, "CheckPlayers_")
SITHMULTITEST_FAMILY(ProcessPlayerLost, "ProcessPlayerLost_")
SITHMULTITEST_FAMILY(RemovePlayer, "RemovePlayer_")
SITHMULTITEST_FAMILY(SendWelcome, "SendWelcome_")
SITHMULTITEST_FAMILY(ProcessPlayerJoin, "ProcessPlayerJoin_")
SITHMULTITEST_FAMILY(SyncPlayers, "SyncPlayers_")
SITHMULTITEST_FAMILY(ProcessSyncPlayers, "ProcessSyncPlayers_")
SITHMULTITEST_FAMILY(FinishJoining, "FinishJoining_")
SITHMULTITEST_FAMILY(ProcessChat, "ProcessChat_")
SITHMULTITEST_FAMILY(ProcessPing, "ProcessPing_")
SITHMULTITEST_FAMILY(ProcessPong, "ProcessPong_")
SITHMULTITEST_FAMILY(QuitPlayer, "QuitPlayer_")
SITHMULTITEST_FAMILY(ProcessQuit, "ProcessQuit_")
SITHMULTITEST_FAMILY(QuitGame, "QuitGame_")
SITHMULTITEST_FAMILY(Update, "Update_")
SITHMULTITEST_FAMILY(SyncScores, "SyncScores_")
SITHMULTITEST_FAMILY(GetPlayerIndexByID, "GetPlayerIndexByID_")
SITHMULTITEST_FAMILY(ProcessKilledPlayer, "ProcessKilledPlayer_")
SITHMULTITEST_FAMILY(RemoveStaticThing, "RemoveStaticThing_")
SITHMULTITEST_FAMILY(UpdateSurfaces, "UpdateSurfaces_")
SITHMULTITEST_FAMILY(UpdateSectors, "UpdateSectors_")
SITHMULTITEST_FAMILY(UpdateThings, "UpdateThings_")
SITHMULTITEST_FAMILY(UpdateRemovals, "UpdateRemovals_")
SITHMULTITEST_FAMILY(StartWelcomingPlayer, "StartWelcomingPlayer_")
SITHMULTITEST_FAMILY(StopWelcomingPlayer, "StopWelcomingPlayer_")

#define SITHMULTITEST_KNOWNFAMILY(name, prefix, message, ...) \
    TEST(sithMulti, TestOriginal##name) \
    { \
        static const char* const aKnown[] = { __VA_ARGS__ }; \
        unsigned numFailed = sithMultiTest_RunCases(prefix); \
        sithMultiTest_ExpectKnownDifferences(numFailed, aKnown, STD_ARRAYLEN(aKnown), message); \
    }

SITHMULTITEST_FAMILY(ProcessWelcome, "ProcessWelcome_")

// The original prints "Starting welcome of player %d." through the error print, the reconstruction through the status
// print (SITHLOG_STATUS, an intentional change marked TODO in sithMulti_ProcessJoinRequest); every accepted join
// request differs in the error and status counts.
SITHMULTITEST_KNOWNFAMILY(ProcessJoinRequest, "ProcessJoinRequest_",
    "sithMulti_ProcessJoinRequest logs the start of a welcome as status instead of error (intentional); see tv/sithMulti/README.md.",
    "ProcessJoinRequest_levelCase ", "ProcessJoinRequest_levelUnterminated ", "ProcessJoinRequest_joins ", "ProcessJoinRequest_unterminatedName ",
    "ProcessJoinRequest_staleName ", "ProcessJoinRequest_last24 ", "ProcessJoinRequest_emptyLevel ", "ProcessJoinRequest_checksumIntMin ",
    "ProcessJoinRequest_twoFreeSlots ")

SITHMULTITEST_FAMILY(Respawn, "Respawn_")

// Every case of the table must be in the fixture and the other way around
TEST(sithMulti, TestFixtureCoversAllCases)
{
    TEST_ASSERT_EQUAL_UINT(MSV_NUMCASES, sithMultiTest_numCases);
    for ( unsigned i = 0; i < MSV_NUMCASES; ++i )
    {
        TEST_ASSERT_EQUAL_STRING(msv_aCases[i].name, sithMultiTest_aCases[i].aName);
    }
}

TEST_GROUP_RUNNER(sithMulti)
{
    RUN_TEST_CASE(sithMulti, TestFixtureCoversAllCases);
    RUN_TEST_CASE(sithMulti, TestOriginalCloseGame);
    RUN_TEST_CASE(sithMulti, TestOriginalCheckPlayers);
    RUN_TEST_CASE(sithMulti, TestOriginalProcessPlayerLost);
    RUN_TEST_CASE(sithMulti, TestOriginalRemovePlayer);
    RUN_TEST_CASE(sithMulti, TestOriginalSendWelcome);
    RUN_TEST_CASE(sithMulti, TestOriginalProcessWelcome);
    RUN_TEST_CASE(sithMulti, TestOriginalProcessPlayerJoin);
    RUN_TEST_CASE(sithMulti, TestOriginalSyncPlayers);
    RUN_TEST_CASE(sithMulti, TestOriginalProcessSyncPlayers);
    RUN_TEST_CASE(sithMulti, TestOriginalProcessJoinRequest);
    RUN_TEST_CASE(sithMulti, TestOriginalFinishJoining);
    RUN_TEST_CASE(sithMulti, TestOriginalProcessChat);
    RUN_TEST_CASE(sithMulti, TestOriginalProcessPing);
    RUN_TEST_CASE(sithMulti, TestOriginalProcessPong);
    RUN_TEST_CASE(sithMulti, TestOriginalQuitPlayer);
    RUN_TEST_CASE(sithMulti, TestOriginalProcessQuit);
    RUN_TEST_CASE(sithMulti, TestOriginalQuitGame);
    RUN_TEST_CASE(sithMulti, TestOriginalUpdate);
    RUN_TEST_CASE(sithMulti, TestOriginalSyncScores);
    RUN_TEST_CASE(sithMulti, TestOriginalGetPlayerIndexByID);
    RUN_TEST_CASE(sithMulti, TestOriginalProcessKilledPlayer);
    RUN_TEST_CASE(sithMulti, TestOriginalRespawn);
    RUN_TEST_CASE(sithMulti, TestOriginalRemoveStaticThing);
    RUN_TEST_CASE(sithMulti, TestOriginalUpdateSurfaces);
    RUN_TEST_CASE(sithMulti, TestOriginalUpdateSectors);
    RUN_TEST_CASE(sithMulti, TestOriginalUpdateThings);
    RUN_TEST_CASE(sithMulti, TestOriginalUpdateRemovals);
    RUN_TEST_CASE(sithMulti, TestOriginalStartWelcomingPlayer);
    RUN_TEST_CASE(sithMulti, TestOriginalStopWelcomingPlayer);
}
