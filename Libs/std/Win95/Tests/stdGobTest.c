#include <unity_fixture.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdGob.h>

#include "stdGeneralTest.h"

#ifndef STD_WIN95_TEST_TV_DIR
#define STD_WIN95_TEST_TV_DIR "."
#endif

static const char* stdGobTest_pBasicPath        = "stdGobTest_basic.gob";
static const char* stdGobTest_pBadSignaturePath = "stdGobTest_bad_signature.gob";
static const char* stdGobTest_pBadVersionPath   = "stdGobTest_bad_version.gob";
static const char* stdGobTest_pBadDirOffsetPath = "stdGobTest_bad_dir_offset.gob";
static const char* stdGobTest_pBadDirCountPath  = "stdGobTest_bad_dir_count.gob";
static const char* stdGobTest_pBadBoundsPath    = "stdGobTest_bad_bounds.gob";
static const char* stdGobTest_pEmptyPath        = "stdGobTest_empty.gob";
static const char* stdGobTest_pScratchPath      = "stdGobTest_scratch.gob";
static char stdGobTest_aTvPath[512];

#define STDGOB_TEST_MAX_GENERATED_ENTRIES    8u
#define STDGOB_TEST_MAX_GENERATED_ENTRY_SIZE 16u
#define STDGOB_TEST_MAX_GENERATED_GOB_SIZE   2048u

static void stdGobTest_DeleteMalformedScratchFiles(void)
{
    char aScratchPath[64];
    for ( size_t i = 0; i < 32u; ++i )
    {
        STD_FORMAT(aScratchPath, "stdGobTest_bad_%u.gob", (unsigned)i);
        StdGeneralTest_DeleteFile(aScratchPath);
    }
}

static void stdGobTest_DeleteFiles(void)
{
    StdGeneralTest_DeleteFile(stdGobTest_pBasicPath);
    StdGeneralTest_DeleteFile(stdGobTest_pBadSignaturePath);
    StdGeneralTest_DeleteFile(stdGobTest_pBadVersionPath);
    StdGeneralTest_DeleteFile(stdGobTest_pBadDirOffsetPath);
    StdGeneralTest_DeleteFile(stdGobTest_pBadDirCountPath);
    StdGeneralTest_DeleteFile(stdGobTest_pBadBoundsPath);
    StdGeneralTest_DeleteFile(stdGobTest_pEmptyPath);
    StdGeneralTest_DeleteFile(stdGobTest_pScratchPath);
    stdGobTest_DeleteMalformedScratchFiles();
}

static int stdGobTest_WriteVector(const char* pVectorName, const char* pPath)
{
    STD_FORMAT(stdGobTest_aTvPath, "%s/stdGob/%s", STD_WIN95_TEST_TV_DIR, pVectorName);
    stdGobTest_aTvPath[sizeof(stdGobTest_aTvPath) - 1] = '\0';
    return StdGeneralTest_CopyFile(stdGobTest_aTvPath, pPath);
}

static void stdGobTest_WriteBasicVector(void)
{
    TEST_ASSERT_TRUE(stdGobTest_WriteVector("basic.gob", stdGobTest_pBasicPath));
}

static void stdGobTest_WriteBytes(const char* pPath, const void* pData, size_t size)
{
    tFileHandle fh;

    StdGeneralTest_DeleteFile(pPath);
    fh = stdFileOpen(pPath, "wb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    if ( size )
    {
        TEST_ASSERT_EQUAL_size_t(size, stdFileWrite(fh, pData, size));
    }
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
}

static uint8_t stdGobTest_GetGeneratedPayloadByte(size_t entryNum, size_t byteNum)
{
    return (uint8_t)(entryNum * 31u + byteNum * 17u + 0x42u);
}

static size_t stdGobTest_BuildGeneratedGob(uint8_t* pData, size_t dataSize, size_t numEntries)
{
    GobFileHeader header;
    tGobFileEntry aEntries[STDGOB_TEST_MAX_GENERATED_ENTRIES];
    uint8_t aPayload[STDGOB_TEST_MAX_GENERATED_ENTRIES * STDGOB_TEST_MAX_GENERATED_ENTRY_SIZE];
    uint32_t numEntries32 = (uint32_t)numEntries;
    size_t payloadSize    = 0;
    size_t dirOffset;
    size_t totalSize;

    TEST_ASSERT_TRUE(numEntries <= STDGOB_TEST_MAX_GENERATED_ENTRIES);

    STD_ZEROMEM(&header, sizeof(header));
    STD_ZEROMEM(aEntries, sizeof(aEntries));
    STD_ZEROMEM(aPayload, sizeof(aPayload));

    memcpy(header.signature, "GOB ", 4u);
    header.version = 20;

    for ( size_t i = 0; i < numEntries; ++i )
    {
        size_t entrySize = i + 3u;

        TEST_ASSERT_TRUE(entrySize <= STDGOB_TEST_MAX_GENERATED_ENTRY_SIZE);
        aEntries[i].offset = (int)(sizeof(header) + payloadSize);
        aEntries[i].size   = (int)entrySize;
        STD_FORMAT(aEntries[i].aName, "generated/file%02u.bin", (unsigned)i);

        for ( size_t j = 0; j < entrySize; ++j )
        {
            aPayload[payloadSize + j] = stdGobTest_GetGeneratedPayloadByte(i, j);
        }

        payloadSize += entrySize;
    }

    dirOffset        = sizeof(header) + payloadSize;
    header.dirOffset = (int)dirOffset;
    totalSize        = dirOffset + sizeof(numEntries32) + sizeof(tGobFileEntry) * numEntries;

    TEST_ASSERT_TRUE(totalSize <= dataSize);

    memcpy(pData, &header, sizeof(header));
    memcpy(&pData[sizeof(header)], aPayload, payloadSize);
    memcpy(&pData[dirOffset], &numEntries32, sizeof(numEntries32));
    memcpy(&pData[dirOffset + sizeof(numEntries32)], aEntries, sizeof(tGobFileEntry) * numEntries);
    return totalSize;
}

TEST_GROUP(stdGob);

TEST_SETUP(stdGob)
{
    StdGeneralTest_Startup();
    stdGobTest_DeleteFiles();
    stdGob_Startup(StdGeneralTest_GetHostServices());
}

TEST_TEAR_DOWN(stdGob)
{
    stdGob_Shutdown();
    stdGobTest_DeleteFiles();
    StdGeneralTest_Shutdown();
}

TEST(stdGob, TestLoadVectorAndReadTextAndBinaryEntries)
{
    GobFileHandle* pTextHandle;
    GobFileHandle* pBinaryHandle;
    Gob* pGob;
    char aText[16];
    uint8_t aBinary[8];

    stdGobTest_WriteBasicVector();
    pGob = stdGob_Load(stdGobTest_pBasicPath, 2, 0);
    TEST_ASSERT_NOT_NULL(pGob);
    TEST_ASSERT_EQUAL_STRING(stdGobTest_pBasicPath, pGob->aFilePath);
    TEST_ASSERT_EQUAL_UINT32(2u, pGob->directory.numEntries);
    TEST_ASSERT_NOT_NULL(pGob->directory.aEntries);
    TEST_ASSERT_NOT_NULL(pGob->pDirHash);
    TEST_ASSERT_NOT_NULL(pGob->aHandles);

    pTextHandle = stdGob_FileOpen(pGob, "TEXT/HELLO.TXT");
    TEST_ASSERT_NOT_NULL(pTextHandle);
    TEST_ASSERT_EQUAL_INT(0, stdGob_FileTell(pTextHandle));
    TEST_ASSERT_EQUAL_size_t(5u, stdGob_FileRead(pTextHandle, aText, 5u));
    aText[5] = '\0';
    TEST_ASSERT_EQUAL_STRING("Hello", aText);
    TEST_ASSERT_EQUAL_INT(5, stdGob_FileTell(pTextHandle));
    TEST_ASSERT_TRUE(stdGob_FileSeek(pTextHandle, 1, 1));
    TEST_ASSERT_EQUAL_PTR(aText, stdGob_FileGets(pTextHandle, aText, sizeof(aText)));
    TEST_ASSERT_EQUAL_STRING("World\n", aText);
    TEST_ASSERT_TRUE(stdGob_FileEOF(pTextHandle));

    pBinaryHandle = stdGob_FileOpen(pGob, "data/raw.bin");
    TEST_ASSERT_NOT_NULL(pBinaryHandle);
    TEST_ASSERT_EQUAL_size_t(6u, stdGob_FileRead(pBinaryHandle, aBinary, sizeof(aBinary)));
    TEST_ASSERT_EQUAL_UINT8(0x00u, aBinary[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01u, aBinary[1]);
    TEST_ASSERT_EQUAL_UINT8(0x02u, aBinary[2]);
    TEST_ASSERT_EQUAL_UINT8(0xFEu, aBinary[3]);
    TEST_ASSERT_EQUAL_UINT8(0xFFu, aBinary[4]);
    TEST_ASSERT_EQUAL_UINT8(0x5Au, aBinary[5]);
    TEST_ASSERT_TRUE(stdGob_FileEOF(pBinaryHandle));

    stdGob_FileClose(pBinaryHandle);
    stdGob_FileClose(pTextHandle);
    stdGob_Free(pGob);
}

TEST(stdGob, TestLoadValidEmptyGob)
{
    Gob* pGob;

    TEST_ASSERT_TRUE(stdGobTest_WriteVector("empty.gob", stdGobTest_pEmptyPath));
    pGob = stdGob_Load(stdGobTest_pEmptyPath, 1, 0);
    TEST_ASSERT_NOT_NULL(pGob);
    TEST_ASSERT_EQUAL_STRING(stdGobTest_pEmptyPath, pGob->aFilePath);
    TEST_ASSERT_EQUAL_UINT32(0u, pGob->directory.numEntries);
    TEST_ASSERT_NOT_NULL(pGob->pDirHash);
    TEST_ASSERT_NOT_NULL(pGob->aHandles);
    TEST_ASSERT_NULL(stdGob_FileOpen(pGob, "anything.bin"));

    stdGob_Free(pGob);
}

TEST(stdGob, TestGeneratedGobContainersRoundTripEntries)
{
    uint8_t aGobData[STDGOB_TEST_MAX_GENERATED_GOB_SIZE];

    for ( size_t numEntries = 0; numEntries <= STDGOB_TEST_MAX_GENERATED_ENTRIES; ++numEntries )
    {
        size_t gobSize = stdGobTest_BuildGeneratedGob(aGobData, sizeof(aGobData), numEntries);
        Gob* pGob;

        stdGobTest_WriteBytes(stdGobTest_pScratchPath, aGobData, gobSize);
        pGob = stdGob_Load(stdGobTest_pScratchPath, (int)numEntries + 1, 0);
        TEST_ASSERT_NOT_NULL(pGob);
        TEST_ASSERT_EQUAL_UINT32((uint32_t)numEntries, pGob->directory.numEntries);

        for ( size_t i = 0; i < numEntries; ++i )
        {
            uint8_t aData[STDGOB_TEST_MAX_GENERATED_ENTRY_SIZE];
            size_t entrySize = i + 3u;

            char aName[64];
            STD_FORMAT(aName, "GENERATED/FILE%02u.BIN", (unsigned)i);
            GobFileHandle* pHandle = stdGob_FileOpen(pGob, aName);
            TEST_ASSERT_NOT_NULL(pHandle);
            TEST_ASSERT_EQUAL_size_t(entrySize, stdGob_FileRead(pHandle, aData, sizeof(aData)));
            TEST_ASSERT_TRUE(stdGob_FileEOF(pHandle));

            for ( size_t j = 0; j < entrySize; ++j )
            {
                TEST_ASSERT_EQUAL_UINT8(stdGobTest_GetGeneratedPayloadByte(i, j), aData[j]);
            }

            stdGob_FileClose(pHandle);
        }

        TEST_ASSERT_NULL(stdGob_FileOpen(pGob, "generated/missing.bin"));
        stdGob_Free(pGob);
    }
}

TEST(stdGob, TestSeekBoundsHandleLimitAndInvalidOperations)
{
    GobFileHandle* pTextHandle;
    GobFileHandle* pMissingHandle;
    Gob* pGob;
#ifdef J3D_RUNTIME_GUARDS
    char aText[8];
#endif

    stdGobTest_WriteBasicVector();
    pGob = stdGob_Load(stdGobTest_pBasicPath, 1, 0);
    TEST_ASSERT_NOT_NULL(pGob);

    pTextHandle = stdGob_FileOpen(pGob, "text/hello.txt");
    TEST_ASSERT_NOT_NULL(pTextHandle);
    pMissingHandle = stdGob_FileOpen(pGob, "data/raw.bin");
    TEST_ASSERT_NULL(pMissingHandle);
    TEST_ASSERT_NULL(stdGob_FileOpen(pGob, "missing.txt"));

    TEST_ASSERT_TRUE(stdGob_FileSeek(pTextHandle, 6, 0));
    TEST_ASSERT_EQUAL_INT(6, stdGob_FileTell(pTextHandle));
    TEST_ASSERT_TRUE(stdGob_FileSeek(pTextHandle, -1, 1));
    TEST_ASSERT_EQUAL_INT(5, stdGob_FileTell(pTextHandle));
    TEST_ASSERT_TRUE(stdGob_FileSeek(pTextHandle, 0, 2));
    TEST_ASSERT_EQUAL_INT(12, stdGob_FileTell(pTextHandle));
    TEST_ASSERT_FALSE(stdGob_FileSeek(pTextHandle, 1, 2));
    TEST_ASSERT_FALSE(stdGob_FileSeek(pTextHandle, -1, 0));
    TEST_ASSERT_FALSE(stdGob_FileSeek(pTextHandle, 0, 99));

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_INT(-1, stdGob_FileTell(NULL));
    TEST_ASSERT_EQUAL_INT(1, stdGob_FileEOF(NULL));
    TEST_ASSERT_EQUAL_size_t(0u, stdGob_FileRead(NULL, aText, sizeof(aText)));
    TEST_ASSERT_EQUAL_size_t(0u, stdGob_FileRead(pTextHandle, NULL, sizeof(aText)));
    TEST_ASSERT_EQUAL_size_t(0u, stdGob_FileRead(pTextHandle, aText, 0u));
    TEST_ASSERT_NULL(stdGob_FileGets(NULL, aText, sizeof(aText)));
    TEST_ASSERT_NULL(stdGob_FileGets(pTextHandle, NULL, sizeof(aText)));
    TEST_ASSERT_NULL(stdGob_FileGets(pTextHandle, aText, 0u));
#endif

    stdGob_FileClose(pTextHandle);
#ifdef J3D_RUNTIME_GUARDS
    stdGob_FileClose(pTextHandle);
    TEST_ASSERT_NULL(stdGob_FileOpen(NULL, "text/hello.txt"));
    TEST_ASSERT_NULL(stdGob_FileOpen(pGob, NULL));
#endif

    stdGob_Free(pGob);
}

TEST(stdGob, TestInvalidHandleInternalsFailSafely)
{
    Gob gob;
    GobFileHandle handle;
    tGobFileEntry entry;
    char aText[8];

    STD_ZEROMEM(&gob, sizeof(gob));
    STD_ZEROMEM(&handle, sizeof(handle));
    STD_ZEROMEM(&entry, sizeof(entry));

    handle.bUsed  = 1;
    handle.offset = 2;

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_FALSE(stdGob_FileSeek(&handle, 0, 0));
    TEST_ASSERT_EQUAL_INT(2, stdGob_FileTell(&handle));
    TEST_ASSERT_TRUE(stdGob_FileEOF(&handle));
    TEST_ASSERT_EQUAL_size_t(0u, stdGob_FileRead(&handle, aText, sizeof(aText)));
    TEST_ASSERT_NULL(stdGob_FileGets(&handle, aText, sizeof(aText)));
#endif

    handle.pEntry = &entry;
    entry.offset  = 12;
    entry.size    = 6;
#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_FALSE(stdGob_FileSeek(&handle, 0, 0));
    TEST_ASSERT_FALSE(stdGob_FileEOF(&handle));
    TEST_ASSERT_EQUAL_size_t(0u, stdGob_FileRead(&handle, aText, sizeof(aText)));
    TEST_ASSERT_NULL(stdGob_FileGets(&handle, aText, sizeof(aText)));
#endif

    handle.pGob   = &gob;
    handle.offset = -1;
    TEST_ASSERT_EQUAL_size_t(0u, stdGob_FileRead(&handle, aText, sizeof(aText)));
    TEST_ASSERT_NULL(stdGob_FileGets(&handle, aText, sizeof(aText)));

    entry.offset  = INT_MAX;
    entry.size    = 2;
    handle.offset = 1;
    TEST_ASSERT_EQUAL_size_t(0u, stdGob_FileRead(&handle, aText, sizeof(aText)));
    TEST_ASSERT_NULL(stdGob_FileGets(&handle, aText, sizeof(aText)));

    handle.offset = 7;
    TEST_ASSERT_EQUAL_size_t(0u, stdGob_FileRead(&handle, aText, sizeof(aText)));
    TEST_ASSERT_NULL(stdGob_FileGets(&handle, aText, sizeof(aText)));
}

TEST(stdGob, TestMultipleHandlesKeepIndependentOffsets)
{
    GobFileHandle* pTextHandle;
    GobFileHandle* pBinaryHandle;
    GobFileHandle* pSecondTextHandle;
    Gob* pGob;
    char aText[8];
    uint8_t aBinary[4];

    stdGobTest_WriteBasicVector();
    pGob = stdGob_Load(stdGobTest_pBasicPath, 3, 0);
    TEST_ASSERT_NOT_NULL(pGob);

    pTextHandle       = stdGob_FileOpen(pGob, "text/hello.txt");
    pBinaryHandle     = stdGob_FileOpen(pGob, "data/raw.bin");
    pSecondTextHandle = stdGob_FileOpen(pGob, "text/hello.txt");
    TEST_ASSERT_NOT_NULL(pTextHandle);
    TEST_ASSERT_NOT_NULL(pBinaryHandle);
    TEST_ASSERT_NOT_NULL(pSecondTextHandle);

    TEST_ASSERT_EQUAL_size_t(6u, stdGob_FileRead(pTextHandle, aText, 6u));
    aText[6] = '\0';
    TEST_ASSERT_EQUAL_STRING("Hello\n", aText);
    TEST_ASSERT_EQUAL_INT(6, stdGob_FileTell(pTextHandle));

    TEST_ASSERT_EQUAL_size_t(2u, stdGob_FileRead(pBinaryHandle, aBinary, 2u));
    TEST_ASSERT_EQUAL_UINT8(0x00u, aBinary[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01u, aBinary[1]);
    TEST_ASSERT_EQUAL_INT(2, stdGob_FileTell(pBinaryHandle));
    TEST_ASSERT_EQUAL_INT(6, stdGob_FileTell(pTextHandle));

    TEST_ASSERT_EQUAL_size_t(6u, stdGob_FileRead(pTextHandle, aText, 6u));
    aText[6] = '\0';
    TEST_ASSERT_EQUAL_STRING("World\n", aText);
    TEST_ASSERT_TRUE(stdGob_FileEOF(pTextHandle));

    TEST_ASSERT_EQUAL_size_t(5u, stdGob_FileRead(pSecondTextHandle, aText, 5u));
    aText[5] = '\0';
    TEST_ASSERT_EQUAL_STRING("Hello", aText);
    TEST_ASSERT_EQUAL_INT(5, stdGob_FileTell(pSecondTextHandle));

    stdGob_FileClose(pTextHandle);
    pTextHandle = stdGob_FileOpen(pGob, "text/hello.txt");
    TEST_ASSERT_NOT_NULL(pTextHandle);
    TEST_ASSERT_EQUAL_INT(0, stdGob_FileTell(pTextHandle));

    stdGob_FileClose(pTextHandle);
    stdGob_FileClose(pSecondTextHandle);
    stdGob_FileClose(pBinaryHandle);
    stdGob_Free(pGob);
}

TEST(stdGob, TestFileGetsTruncatesAndStopsAtEntryEnd)
{
    GobFileHandle* pTextHandle;
    Gob* pGob;
    char aText[8];

    stdGobTest_WriteBasicVector();
    pGob = stdGob_Load(stdGobTest_pBasicPath, 1, 0);
    TEST_ASSERT_NOT_NULL(pGob);

    pTextHandle = stdGob_FileOpen(pGob, "text/hello.txt");
    TEST_ASSERT_NOT_NULL(pTextHandle);

    TEST_ASSERT_EQUAL_PTR(aText, stdGob_FileGets(pTextHandle, aText, 4u));
    TEST_ASSERT_EQUAL_STRING("Hel", aText);
    TEST_ASSERT_EQUAL_INT(3, stdGob_FileTell(pTextHandle));

    TEST_ASSERT_EQUAL_PTR(aText, stdGob_FileGets(pTextHandle, aText, 4u));
    TEST_ASSERT_EQUAL_STRING("lo\n", aText);
    TEST_ASSERT_EQUAL_INT(6, stdGob_FileTell(pTextHandle));

    TEST_ASSERT_EQUAL_PTR(aText, stdGob_FileGets(pTextHandle, aText, sizeof(aText)));
    TEST_ASSERT_EQUAL_STRING("World\n", aText);
    TEST_ASSERT_EQUAL_INT(12, stdGob_FileTell(pTextHandle));
    TEST_ASSERT_TRUE(stdGob_FileEOF(pTextHandle));
    TEST_ASSERT_NULL(stdGob_FileGets(pTextHandle, aText, sizeof(aText)));

    stdGob_FileClose(pTextHandle);
    stdGob_Free(pGob);
}

TEST(stdGob, TestRejectsMalformedGobVectors)
{
    char aScratchPath[64];
    static const char* const apVectors[] =
    {
        "garbage.gob",
        "missing_version_header.gob",
        "missing_dir_offset_header.gob",
        "bad_signature.gob",
        "bad_version.gob",
        "bad_dir_offset.gob",
        "negative_dir_offset.gob",
        "bad_dir_count.gob",
        "bad_entry_bounds.gob",
        "bad_entry_offset_out.gob",
        "bad_entry_size_out.gob",
        "bad_entry_partial_eof.gob",
        "bad_entry_negative_offset.gob",
        "bad_entry_negative_size.gob"
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(apVectors); ++i )
    {
        STD_FORMAT(aScratchPath, "stdGobTest_bad_%u.gob", (unsigned)i);
        StdGeneralTest_DeleteFile(aScratchPath);

        TEST_ASSERT_TRUE_MESSAGE(stdGobTest_WriteVector(apVectors[i], aScratchPath), apVectors[i]);
        TEST_ASSERT_NULL(stdGob_Load(aScratchPath, 1, 0));
        StdGeneralTest_DeleteFile(aScratchPath);
    }
}

TEST(stdGob, TestGeneratedGobTruncationCorpusRejects)
{
    uint8_t aGobData[STDGOB_TEST_MAX_GENERATED_GOB_SIZE];
    size_t gobSize = stdGobTest_BuildGeneratedGob(aGobData, sizeof(aGobData), 3u);

    for ( size_t truncatedSize = 0; truncatedSize < gobSize; ++truncatedSize )
    {
        stdGobTest_WriteBytes(stdGobTest_pScratchPath, aGobData, truncatedSize);
        TEST_ASSERT_NULL(stdGob_Load(stdGobTest_pScratchPath, 2, 0));
    }
}

TEST(stdGob, TestLoadEntryFreeEntryMmapFallbackAndRejectionVectors)
{
    Gob gob;
    Gob* pGob;

    stdGobTest_WriteBasicVector();
    TEST_ASSERT_TRUE(stdGobTest_WriteVector("bad_signature.gob", stdGobTest_pBadSignaturePath));
    TEST_ASSERT_TRUE(stdGobTest_WriteVector("bad_version.gob", stdGobTest_pBadVersionPath));
    TEST_ASSERT_TRUE(stdGobTest_WriteVector("bad_dir_offset.gob", stdGobTest_pBadDirOffsetPath));
    TEST_ASSERT_TRUE(stdGobTest_WriteVector("bad_dir_count.gob", stdGobTest_pBadDirCountPath));
    TEST_ASSERT_TRUE(stdGobTest_WriteVector("bad_entry_bounds.gob", stdGobTest_pBadBoundsPath));

    STD_ZEROMEM(&gob, sizeof(gob));
    TEST_ASSERT_EQUAL_INT(1, stdGob_LoadEntry(&gob, stdGobTest_pBasicPath, 1, 0));
    TEST_ASSERT_EQUAL_UINT32(2u, gob.directory.numEntries);
    stdGob_FreeEntry(&gob);
    TEST_ASSERT_NULL(gob.directory.aEntries);
    TEST_ASSERT_NULL(gob.aHandles);
    TEST_ASSERT_EQUAL(0u, gob.hGobFile);

    pGob = stdGob_Load(stdGobTest_pBasicPath, 1, 1);
    TEST_ASSERT_NOT_NULL(pGob);
    TEST_ASSERT_FALSE(pGob->bFileMap);
    stdGob_Free(pGob);

    TEST_ASSERT_NULL(stdGob_Load("stdGobTest_missing.gob", 1, 0));
    TEST_ASSERT_NULL(stdGob_Load(stdGobTest_pBadSignaturePath, 1, 0));
    TEST_ASSERT_NULL(stdGob_Load(stdGobTest_pBadVersionPath, 1, 0));
    TEST_ASSERT_NULL(stdGob_Load(stdGobTest_pBadDirOffsetPath, 1, 0));
    TEST_ASSERT_NULL(stdGob_Load(stdGobTest_pBadDirCountPath, 1, 0));
    TEST_ASSERT_NULL(stdGob_Load(stdGobTest_pBadBoundsPath, 1, 0));
    TEST_ASSERT_NULL(stdGob_Load(stdGobTest_pBasicPath, -1, 0));
    TEST_ASSERT_NULL(stdGob_Load(stdGobTest_pBasicPath, 0, 0));
#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_NULL(stdGob_Load(NULL, 1, 0));
#endif

    STD_ZEROMEM(&gob, sizeof(gob));
#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_INT(0, stdGob_LoadEntry(NULL, stdGobTest_pBasicPath, 1, 0));
    TEST_ASSERT_EQUAL_INT(0, stdGob_LoadEntry(&gob, NULL, 1, 0));
#endif
    TEST_ASSERT_EQUAL_INT(0, stdGob_LoadEntry(&gob, stdGobTest_pBasicPath, INT_MAX, 0));

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_NULL(stdGob_Load(stdGobTest_pBasicPath, 1, 0));
    StdGeneralTest_ClearAllocationFailures();
}

TEST(stdGob, TestAllocationFailuresDuringLoadCleanlyReject)
{
    Gob* pGob;

    stdGobTest_WriteBasicVector();

    for ( int numSuccessfulAllocations = 0; numSuccessfulAllocations < 5; ++numSuccessfulAllocations )
    {
        StdGeneralTest_FailNextAllocations(numSuccessfulAllocations);
        pGob = stdGob_Load(stdGobTest_pBasicPath, 2, 0);
        TEST_ASSERT_NULL(pGob);
        StdGeneralTest_ClearAllocationFailures();
    }

    StdGeneralTest_FailNextAllocations(5);
    pGob = stdGob_Load(stdGobTest_pBasicPath, 2, 0);
    StdGeneralTest_ClearAllocationFailures();
    TEST_ASSERT_NOT_NULL(pGob);
    TEST_ASSERT_NOT_NULL(stdGob_FileOpen(pGob, "text/hello.txt"));
    TEST_ASSERT_NOT_NULL(stdGob_FileOpen(pGob, "data/raw.bin"));
    stdGob_Free(pGob);
}

TEST(stdGob, TestFileReadFailuresDuringLoadCleanlyReject)
{
    stdGobTest_WriteBasicVector();

    for ( int numSuccessfulReads = 0; numSuccessfulReads < 4; ++numSuccessfulReads )
    {
        StdGeneralTest_FailNextFileReads(numSuccessfulReads);
        TEST_ASSERT_NULL(stdGob_Load(stdGobTest_pBasicPath, 2, 0));
        StdGeneralTest_ClearFileFailures();
    }
}

TEST(stdGob, TestEntryReadFailuresDoNotAdvanceOffsets)
{
    GobFileHandle* pTextHandle;
    Gob* pGob;
    char aText[8];

    stdGobTest_WriteBasicVector();
    pGob = stdGob_Load(stdGobTest_pBasicPath, 1, 0);
    TEST_ASSERT_NOT_NULL(pGob);

    pTextHandle = stdGob_FileOpen(pGob, "text/hello.txt");
    TEST_ASSERT_NOT_NULL(pTextHandle);

    StdGeneralTest_FailNextFileReads(0);
    TEST_ASSERT_EQUAL_size_t(0u, stdGob_FileRead(pTextHandle, aText, sizeof(aText)));
    StdGeneralTest_ClearFileFailures();
    TEST_ASSERT_EQUAL_INT(0, stdGob_FileTell(pTextHandle));

    StdGeneralTest_FailNextFileGets(0);
    TEST_ASSERT_NULL(stdGob_FileGets(pTextHandle, aText, sizeof(aText)));
    StdGeneralTest_ClearFileFailures();
    TEST_ASSERT_EQUAL_INT(0, stdGob_FileTell(pTextHandle));

    stdGob_FileClose(pTextHandle);
    stdGob_Free(pGob);
}

TEST(stdGob, TestStartupShutdownAndFreeGuards)
{
    J3DTest_ResetAssertCapture();
    stdGob_Startup(StdGeneralTest_GetHostServices());
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("bStartup == 0", "stdGob.c");

    stdGob_Shutdown();
    stdGob_Shutdown();
    stdGob_Startup(StdGeneralTest_GetHostServices());

    StdGeneralTest_ResetDebugOutput();
    stdGob_Free(NULL);
    TEST_ASSERT_EQUAL_STRING("stdGob.c(264): Warning: attempt to free NULL Gob file ptr.\n", StdGeneralTest_GetDebugOutput());

#ifdef J3D_RUNTIME_GUARDS
    J3DTest_ResetAssertCapture();
    stdGob_FreeEntry(NULL);
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    StdGeneralTest_AssertLastAssert("pGob != NULL", "stdGob.c");

    J3DTest_ResetAssertCapture();
    stdGob_FileClose(NULL);
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("pHandle", "stdGob.c");

    GobFileHandle handle;
    STD_ZEROMEM(&handle, sizeof(handle));
    J3DTest_ResetAssertCapture();
    stdGob_FileClose(&handle);
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("pHandle->bUsed", "stdGob.c");
#endif
}

TEST_GROUP_RUNNER(stdGob)
{
    RUN_TEST_CASE(stdGob, TestLoadVectorAndReadTextAndBinaryEntries);
    RUN_TEST_CASE(stdGob, TestLoadValidEmptyGob);
    RUN_TEST_CASE(stdGob, TestGeneratedGobContainersRoundTripEntries);
    RUN_TEST_CASE(stdGob, TestSeekBoundsHandleLimitAndInvalidOperations);
    RUN_TEST_CASE(stdGob, TestInvalidHandleInternalsFailSafely);
    RUN_TEST_CASE(stdGob, TestMultipleHandlesKeepIndependentOffsets);
    RUN_TEST_CASE(stdGob, TestFileGetsTruncatesAndStopsAtEntryEnd);
    RUN_TEST_CASE(stdGob, TestRejectsMalformedGobVectors);
    RUN_TEST_CASE(stdGob, TestGeneratedGobTruncationCorpusRejects);
    RUN_TEST_CASE(stdGob, TestLoadEntryFreeEntryMmapFallbackAndRejectionVectors);
    RUN_TEST_CASE(stdGob, TestAllocationFailuresDuringLoadCleanlyReject);
    RUN_TEST_CASE(stdGob, TestFileReadFailuresDuringLoadCleanlyReject);
    RUN_TEST_CASE(stdGob, TestEntryReadFailuresDoNotAdvanceOffsets);
    RUN_TEST_CASE(stdGob, TestStartupShutdownAndFreeGuards);
}
