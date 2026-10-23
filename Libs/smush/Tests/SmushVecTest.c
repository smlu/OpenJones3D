#include <unity_fixture.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SmushVecCases.h"

#include <smush/insane/Animate/Pcplay.h>
#include <smush/insane/Decode/Decode.h>
#include <std/General/stdUtil.h>

// Original-engine vectors of the smush C modules (Mixer, D_bloc16, Decode, LecShare, Framestream, Stdfile, Zfilio).
// SmushVecCases.h generates every case and reports each result as a named record; the fixtures in tv/ hold the records
// of the retail executable running the same generator (tv/README.md, "C Modules"), and every record of the port must
// be identical, in the same order. Records marked unstable in a fixture (the original's result depended on memory it
// does not own) are skipped.

#define SMUSHVEC_TEST_FILL         0xA5 // the capture used 0x00 and 0xFF: results must not depend on it
#define SMUSHVEC_TEST_MAX_REPORTED 12

void DBloc16_TestGetTables(const int16_t** ppOffsets, const Bl16Small** ppSmall, const Bl16Big** ppBig);

typedef struct sSmushVecTestRecord
{
    char aName[256];
    uint8_t kind;   // SMUSHVEC_RAW, SMUSHVEC_HASH, 2: unstable
    uint32_t size;
    const uint8_t* pData;
    uint64_t hash;
} SmushVecTestRecord;

// A difference the port keeps on purpose or a finding still to be decided: the records whose names start with
// pPrefix may differ; the test then reports IGNORED with pReason instead of failing
typedef struct sSmushVecTestKnown
{
    const char* pPrefix;
    const char* pReason;
    int bExact; // the whole record name must match, not only its start
} SmushVecTestKnown;

typedef struct sSmushVecTestChecker
{
    const char* pPath;
    SmushVecTestRecord* aRecords;
    uint32_t numRecords;
    uint32_t next;
    uint32_t numChecked;
    uint32_t numUnstable;
    uint32_t numFailed;
    uint32_t numKnown;
    const SmushVecTestKnown* aKnown;
    size_t numKnownPrefixes;
    const char* pKnownReason;
    FILE* pDump;
} SmushVecTestChecker;

TEST_GROUP(SmushVec);

TEST_SETUP(SmushVec)
{}

TEST_TEAR_DOWN(SmushVec)
{}

// --- The ported functions ---
static int smushvec_bAnimSaved;
static LecShare* smushvec_pSavedShare;
static StreamerInterface* smushvec_pSavedStreamer;
static AnimFrameTable* smushvec_pSavedFrame;
static AnimPlayOps smushvec_savedOps;

static void SmushVecTest_SetAnimGlobals(LecShare* pShare, StreamerInterface* pStreamer, AnimFrameTable* pFrame, const AnimPlayOps* pOps)
{
    if ( !smushvec_bAnimSaved )
    {
        smushvec_pSavedShare    = animplay_pShare;
        smushvec_pSavedStreamer = animplay_pStreamer;
        smushvec_pSavedFrame    = animplay_pFrame;
        smushvec_savedOps       = animplay_ops;
        smushvec_bAnimSaved     = 1;
    }

    if ( !pOps )
    {
        animplay_pShare    = smushvec_pSavedShare;
        animplay_pStreamer = smushvec_pSavedStreamer;
        animplay_pFrame    = smushvec_pSavedFrame;
        animplay_ops       = smushvec_savedOps;
        return;
    }

    animplay_pShare    = pShare;
    animplay_pStreamer = pStreamer;
    animplay_pFrame    = pFrame;
    animplay_ops       = *pOps;
}

static void SmushVecTest_ZfilioSetGz(const SmushVecGz* pGz)
{
    if ( !pGz )
    {
        Zfilio_TestSetGz(NULL);
        return;
    }

    ZfilioTestGz gz;
    gz.gzopen  = pGz->gzopen;
    gz.gzclose = pGz->gzclose;
    gz.gzread  = pGz->gzread;
    gz.gzwrite = pGz->gzwrite;
    gz.gzflush = pGz->gzflush;
    gz.gzseek  = pGz->gzseek;
    Zfilio_TestSetGz(&gz);
}

static void SmushVecTest_InitApi(SmushVecApi* pApi)
{
    memset(pApi, 0, sizeof(*pApi));
    pApi->mixer_Init   = mixer_Init;
    pApi->mixer_Setup  = mixer_Setup;
    pApi->mixer_Free   = mixer_Free;
    pApi->mixer_Clear  = mixer_Clear;
    pApi->mixer_Feed   = mixer_Feed;
    pApi->mixer_Add    = mixer_Add;
    pApi->mixer_Output = mixer_Output;
    pApi->mixer_Blit   = mixer_Blit;

    pApi->dbloc16_Register    = dbloc16_Register;
    pApi->dbloc16_Configure   = dbloc16_Configure;
    pApi->dbloc16_Unregister  = dbloc16_Unregister;
    pApi->dbloc16_Startup     = dbloc16_Startup;
    pApi->dbloc16_Shutdown    = dbloc16_Shutdown;
    pApi->dbloc16_MakeTables  = dbloc16_MakeTables;
    pApi->dbloc16_MakeGlyphs  = dbloc16_MakeGlyphs;
    pApi->dbloc16_DecodeFrame = dbloc16_DecodeFrame;
    pApi->dbloc16_Scale       = dbloc16_Scale;
    pApi->dbloc16_Average     = dbloc16_Average;
    pApi->dbloc16_ScalePal    = dbloc16_ScalePal;
    const int16_t* pOffsets;
    const Bl16Small* pSmall;
    const Bl16Big* pBig;
    DBloc16_TestGetTables(&pOffsets, &pSmall, &pBig);
    pApi->pOffsets = (int16_t*)pOffsets;
    pApi->pSmall   = (Bl16Small*)pSmall;
    pApi->pBig     = (Bl16Big*)pBig;

    pApi->decode_Rle       = decode_Rle;
    pApi->decode_Run       = decode_Run;
    pApi->decode_MakeTable = decode_MakeTable;
    pApi->decode_Convert   = decode_Convert;
    pApi->decode_ToRGBA    = decode_ToRGBA;
    pApi->decode_Begin     = decode_Begin;

    pApi->LecShare_Init             = LecShare_Init;
    pApi->LecShare_Reset            = LecShare_Reset;
    pApi->LecShare_Publish          = LecShare_Publish;
    pApi->LecShare_Unpublish        = LecShare_Unpublish;
    pApi->LecShare_Subscribe        = LecShare_Subscribe;
    pApi->LecShare_Unsubscribe      = LecShare_Unsubscribe;
    pApi->LecShare_BatchSubscribe   = LecShare_BatchSubscribe;
    pApi->LecShare_BatchUnsubscribe = LecShare_BatchUnsubscribe;
    pApi->LecShare_Enumerate        = LecShare_Enumerate;
    pApi->LecShare_Count            = LecShare_Count;
    pApi->LecShare_Find             = LecShare_Find;
    pApi->LecShare_Match            = LecShare_Match;
    pApi->LecShare_NameLen          = LecShare_NameLen;
    pApi->LecShare_Strip            = LecShare_Strip;
    pApi->LecShare_NameCmp          = LecShare_NameCmp;
    pApi->LecShare_NameNCmp         = LecShare_NameNCmp;

    pApi->framestream_Reset  = framestream_Reset;
    pApi->framestream_Tick   = framestream_Tick;
    pApi->framestream_Open   = framestream_Open;
    pApi->framestream_Seek   = framestream_Seek;
    pApi->framestream_Mode   = framestream_Mode;
    pApi->framestream_IsOpen = framestream_IsOpen;
    pApi->framestream_Stall  = framestream_Stall;
    pApi->SetAnimGlobals     = SmushVecTest_SetAnimGlobals;

    pApi->stdfile_Register   = stdfile_Register;
    pApi->stdfile_Unregister = stdfile_Unregister;
    pApi->stdfile_Startup    = stdfile_Startup;
    pApi->stdfile_Shutdown   = stdfile_Shutdown;

    pApi->zfilio_Register   = zfilio_Register;
    pApi->zfilio_Configure  = zfilio_Configure;
    pApi->zfilio_Unregister = zfilio_Unregister;
    pApi->zfilio_Startup    = zfilio_Startup;
    pApi->zfilio_Shutdown   = zfilio_Shutdown;
    pApi->ZfilioReset       = Zfilio_ResetGlobals;
    pApi->ZfilioSetGz       = SmushVecTest_ZfilioSetGz;
}

// --- Fixture reading and comparison ---
static uint8_t* SmushVecTest_Load(const char* pPath, size_t* pSize)
{
    FILE* pFile = NULL;
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fopen_s(&pFile, pPath, "rb"), pPath);
    TEST_ASSERT_NOT_NULL_MESSAGE(pFile, pPath);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(pFile, 0, SEEK_END), pPath);
    long size = ftell(pFile);
    TEST_ASSERT_TRUE_MESSAGE(size >= 12, pPath);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(pFile, 0, SEEK_SET), pPath);
    uint8_t* pData = (uint8_t*)malloc((size_t)size);
    TEST_ASSERT_NOT_NULL_MESSAGE(pData, pPath);
    TEST_ASSERT_EQUAL_size_t_MESSAGE((size_t)size, fread(pData, 1, (size_t)size, pFile), pPath);
    fclose(pFile);
    *pSize = (size_t)size;
    return pData;
}

static uint32_t SmushVecTest_U32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void SmushVecTest_Parse(SmushVecTestChecker* pChecker, const uint8_t* pData, size_t size)
{
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(SMUSHVEC_MAGIC, pData, 4, pChecker->pPath);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(SMUSHVEC_VERSION, SmushVecTest_U32(pData + 4), pChecker->pPath);
    pChecker->numRecords = SmushVecTest_U32(pData + 8);
    pChecker->aRecords   = (SmushVecTestRecord*)calloc(pChecker->numRecords + 1, sizeof(SmushVecTestRecord));
    TEST_ASSERT_NOT_NULL(pChecker->aRecords);

    size_t pos = 12;
    for ( uint32_t i = 0; i < pChecker->numRecords; ++i )
    {
        SmushVecTestRecord* pRec = &pChecker->aRecords[i];
        TEST_ASSERT_TRUE_MESSAGE(pos + 1 <= size, pChecker->pPath);
        uint8_t nameLen = pData[pos++];
        TEST_ASSERT_TRUE_MESSAGE(pos + nameLen + 5 <= size, pChecker->pPath);
        memcpy(pRec->aName, pData + pos, nameLen);
        pRec->aName[nameLen] = 0;
        pos += nameLen;
        pRec->kind = pData[pos++];
        pRec->size = SmushVecTest_U32(pData + pos);
        pos += 4;
        if ( pRec->kind == SMUSHVEC_RAW )
        {
            TEST_ASSERT_TRUE_MESSAGE(pos + pRec->size <= size, pRec->aName);
            pRec->pData = pData + pos;
            pos += pRec->size;
        }
        else if ( pRec->kind == SMUSHVEC_HASH )
        {
            TEST_ASSERT_TRUE_MESSAGE(pos + 8 <= size, pRec->aName);
            memcpy(&pRec->hash, pData + pos, 8);
            pos += 8;
        }
        else
        {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, pRec->kind, pRec->aName);
        }
    }

    // The whole file must be consumed so that a changed layout cannot go unnoticed
    TEST_ASSERT_EQUAL_size_t_MESSAGE(size, pos, pChecker->pPath);
}

static const SmushVecTestKnown* SmushVecTest_FindKnown(const SmushVecTestChecker* pChecker, const char* pName)
{
    for ( size_t i = 0; i < pChecker->numKnownPrefixes; ++i )
    {
        if ( pChecker->aKnown[i].bExact ? strcmp(pName, pChecker->aKnown[i].pPrefix) == 0
                                        : strncmp(pName, pChecker->aKnown[i].pPrefix, strlen(pChecker->aKnown[i].pPrefix)) == 0 )
        {
            return &pChecker->aKnown[i];
        }
    }

    return NULL;
}

static void SmushVecTest_Record(void* pUser, int kind, const char* pName, const void* pData, uint32_t size)
{
    SmushVecTestChecker* pChecker = (SmushVecTestChecker*)pUser;
    if ( pChecker->pDump )
    {
        uint8_t nameLen = (uint8_t)strlen(pName);
        uint8_t k       = (uint8_t)kind;
        fwrite(&nameLen, 1, 1, pChecker->pDump);
        fwrite(pName, nameLen, 1, pChecker->pDump);
        fwrite(&k, 1, 1, pChecker->pDump);
        fwrite(&size, 4, 1, pChecker->pDump);
        fwrite(pData, size, 1, pChecker->pDump);
    }

    // The parts of a section record are only kept in the dumps
    if ( kind == SMUSHVEC_DETAIL )
    {
        return;
    }

    char aMessage[512];
    if ( pChecker->next >= pChecker->numRecords )
    {
        if ( pChecker->numFailed++ < SMUSHVEC_TEST_MAX_REPORTED )
        {
            STD_FORMAT(aMessage, "%s: extra record '%s'", pChecker->pPath, pName);
            TEST_MESSAGE(aMessage);
        }

        return;
    }

    const SmushVecTestRecord* pRec = &pChecker->aRecords[pChecker->next++];
    if ( strcmp(pRec->aName, pName) != 0 )
    {
        if ( pChecker->numFailed++ < SMUSHVEC_TEST_MAX_REPORTED )
        {
            STD_FORMAT(aMessage, "%s: record %u is '%s', the original's is '%s'", pChecker->pPath, (unsigned)(pChecker->next - 1), pName, pRec->aName);
            TEST_MESSAGE(aMessage);
        }

        return;
    }

    if ( pRec->kind == 2 )
    {
        pChecker->numUnstable++;
        return;
    }

    pChecker->numChecked++;
    int bMatch = pRec->size == size && (int)pRec->kind == kind;
    uint32_t diff = 0;
    if ( bMatch && kind == SMUSHVEC_RAW )
    {
        while ( diff < size && ((const uint8_t*)pData)[diff] == pRec->pData[diff] )
        {
            diff++;
        }

        bMatch = diff == size;
    }
    else if ( bMatch )
    {
        bMatch = SmushVec_Hash(pData, size) == pRec->hash;
    }

    if ( bMatch )
    {
        return;
    }

    const SmushVecTestKnown* pKnown = SmushVecTest_FindKnown(pChecker, pName);
    if ( pKnown )
    {
        pChecker->numKnown++;
        pChecker->pKnownReason = pKnown->pReason;
        return;
    }

    if ( pChecker->numFailed++ < SMUSHVEC_TEST_MAX_REPORTED )
    {
        if ( kind == SMUSHVEC_RAW && pRec->size == size )
        {
            char aPort[64] = { 0 }, aOrig[64] = { 0 };
            for ( uint32_t i = 0; i < 16 && diff + i < size; ++i )
            {
                snprintf(aPort + 3 * i, 4, "%02X ", ((const uint8_t*)pData)[diff + i]);
                snprintf(aOrig + 3 * i, 4, "%02X ", pRec->pData[diff + i]);
            }

            STD_FORMAT(aMessage, "%s: '%s' differs from the original at byte %u: port %s, original %s", pChecker->pPath, pName, (unsigned)diff, aPort, aOrig);
        }
        else
        {
            STD_FORMAT(aMessage, "%s: '%s' differs from the original (size %u, original %u)", pChecker->pPath, pName, (unsigned)size, (unsigned)pRec->size);
        }

        TEST_MESSAGE(aMessage);
    }
}

typedef void (*SmushVecTestArea)(const SmushVecApi* pApi, SmushVecSink* pSink, uint8_t fill);

static void SmushVecTest_Run(const char* pFileName, SmushVecTestArea area, const SmushVecTestKnown* aKnown, size_t numKnown)
{
    char aPath[512];
    STD_FORMAT(aPath, "%s/%s", SMUSH_TEST_TV_DIR, pFileName);
    size_t size;
    uint8_t* pData = SmushVecTest_Load(aPath, &size);

    SmushVecTestChecker checker;
    memset(&checker, 0, sizeof(checker));
    checker.pPath            = aPath;
    checker.aKnown           = aKnown;
    checker.numKnownPrefixes = numKnown;
    SmushVecTest_Parse(&checker, pData, size);

    // Development aid: SMUSHVEC_DUMP_DIR=<dir> writes every record of the port with its full bytes
    const char* pDumpDir = getenv("SMUSHVEC_DUMP_DIR");
    if ( pDumpDir && *pDumpDir )
    {
        char aDump[512];
        const char* pBase = strrchr(pFileName, '/');
        STD_FORMAT(aDump, "%s/%s.port", pDumpDir, pBase ? pBase + 1 : pFileName);
        if ( fopen_s(&checker.pDump, aDump, "wb") != 0 )
        {
            checker.pDump = NULL;
        }
    }

    SmushVecApi api;
    SmushVecTest_InitApi(&api);
    SmushVecSink sink = { SmushVecTest_Record, &checker };
    area(&api, &sink, SMUSHVEC_TEST_FILL);
    SmushVec_Finish();

    if ( checker.pDump )
    {
        fclose(checker.pDump);
    }

    char aMessage[512];
    if ( checker.next != checker.numRecords && checker.numFailed++ < SMUSHVEC_TEST_MAX_REPORTED )
    {
        STD_FORMAT(aMessage, "%s: the port produced %u of the original's %u records", aPath, (unsigned)checker.next, (unsigned)checker.numRecords);
        TEST_MESSAGE(aMessage);
    }

    free(checker.aRecords);
    free(pData);

    TEST_ASSERT_TRUE_MESSAGE(checker.numChecked > 0, aPath);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, checker.numFailed, aPath);
    if ( checker.numKnown )
    {
        STD_FORMAT(aMessage, "%u of %u records differ from the original as known: %s", (unsigned)checker.numKnown, (unsigned)checker.numChecked, checker.pKnownReason);
        TEST_IGNORE_MESSAGE(aMessage);
    }
}

TEST(SmushVec, TestOriginalMixer)
{
    SmushVecTest_Run("Sound/MixerVec.tv", SmushVec_RunMixer, NULL, 0);
}

TEST(SmushVec, TestOriginalBloc16)
{
    SmushVecTest_Run("Flobject/Bloc16Vec.tv", SmushVec_RunBloc16, NULL, 0);
}

TEST(SmushVec, TestOriginalDecode)
{
    SmushVecTest_Run("Decode/DecodeVec.tv", SmushVec_RunDecode, NULL, 0);
}

TEST(SmushVec, TestOriginalLecShare)
{
    SmushVecTest_Run("Lec/LecShareVec.tv", SmushVec_RunLec, NULL, 0);
}

TEST(SmushVec, TestOriginalFramestream)
{
    SmushVecTest_Run("File/FramestreamVec.tv", SmushVec_RunFramestream, NULL, 0);
}

TEST(SmushVec, TestOriginalStdfile)
{
    // stdfile_printf formats with the CRT: the original's (VC5) prints three exponent digits and rounds decimal
    // halves up, the port's (UCRT vsnprintf_s) prints two and rounds the exact binary value (0.5 -> "0", 2.5 -> "2",
    // 0.25 -> "0.2"); integer and string conversions match. stdfile_eof calls stdfile_size before stdfile_tell in the
    // original (VC5's evaluation order of `tell >= size`), the port the other way round; the result is the same.
    static const SmushVecTestKnown aKnown[] = {
        { "stdfile printf fixed", "printf float rounding and exponent digits (VC5 CRT vs UCRT); stdfile_eof call order" },
        { "stdfile printf round half", "printf float rounding and exponent digits (VC5 CRT vs UCRT); stdfile_eof call order" },
        { "stdfile printf general", "printf float rounding and exponent digits (VC5 CRT vs UCRT); stdfile_eof call order" },
        { "stdfile printf exponent", "printf float rounding and exponent digits (VC5 CRT vs UCRT); stdfile_eof call order" },
        { "stdfile output", "printf float rounding and exponent digits (VC5 CRT vs UCRT); stdfile_eof call order" },
        { "stdfile calls", "printf float rounding and exponent digits (VC5 CRT vs UCRT); stdfile_eof call order" },
        // stdfile_gets terminates the string and reads at most size - 1 characters (the original leaves a line that fills the buffer unterminated); the records
        // of the buffer sizes that cannot hold every line of the test file differ
        { "stdfile gets size 1", "stdfile_gets terminates the string and reads at most size - 1 characters", 1 },
        { "stdfile gets size 2", "stdfile_gets terminates the string and reads at most size - 1 characters", 1 },
        { "stdfile gets size 3", "stdfile_gets terminates the string and reads at most size - 1 characters", 1 },
        { "stdfile gets size 5", "stdfile_gets terminates the string and reads at most size - 1 characters", 1 },
        { "stdfile gets size 11", "stdfile_gets terminates the string and reads at most size - 1 characters", 1 },
        { "stdfile gets size 12", "stdfile_gets terminates the string and reads at most size - 1 characters", 1 },
        { "stdfile gets size 16", "stdfile_gets terminates the string and reads at most size - 1 characters", 1 },
    };
    SmushVecTest_Run("File/StdfileVec.tv", SmushVec_RunStdfile, aKnown, STD_ARRAYLEN(aKnown));
}

TEST(SmushVec, TestOriginalZfilio)
{
    // zfilio_Configure consumes the extension argument that is dropped when the table is full (the original leaves it for the next tag
    // read and fails with -1): the two records of the full table case differ
    static const SmushVecTestKnown aKnown[] = {
        { "zfilio_Configure full", "zfilio_Configure consumes the dropped extension argument" },
    };
    SmushVecTest_Run("File/ZfilioVec.tv", SmushVec_RunZfilio, aKnown, STD_ARRAYLEN(aKnown));
}

TEST_GROUP_RUNNER(SmushVec)
{
    RUN_TEST_CASE(SmushVec, TestOriginalMixer);
    RUN_TEST_CASE(SmushVec, TestOriginalBloc16);
    RUN_TEST_CASE(SmushVec, TestOriginalDecode);
    RUN_TEST_CASE(SmushVec, TestOriginalLecShare);
    RUN_TEST_CASE(SmushVec, TestOriginalFramestream);
    RUN_TEST_CASE(SmushVec, TestOriginalStdfile);
    RUN_TEST_CASE(SmushVec, TestOriginalZfilio);
}
