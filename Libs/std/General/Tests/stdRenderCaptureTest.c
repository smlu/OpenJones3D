#include <unity_fixture.h>

#include <stdio.h>
#include <string.h>

#include <std/General/stdRenderCapture.h>
#include <std/General/stdRenderCaptureRecord.h>

#include "stdGeneralTest.h"

// Every expected byte below is hand-written from the RCAP v1 format in stdRenderCapture.h; none of
// it is produced by the writer's own code.

// The 20-byte file header for the 4-column column name "null".
static const uint8_t stdRenderCaptureTest_aHeaderNull[20] = {
    0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00, // "J3DRCAP\0"
    0x01, 0x00, 0x00, 0x00,                         // version 1
    0x04, 0x00, 0x00, 0x00,                         // columnLen 4
    0x6E, 0x75, 0x6C, 0x6C,                         // "null"
};

// 65 characters: one over the 64 the format allows.
static const char stdRenderCaptureTest_aLongColumn[] =
    "0123456789" "0123456789" "0123456789" "0123456789" "0123456789" "0123456789" "01234";

static uint64_t stdRenderCaptureTest_ConstantHash(const void* pData, size_t size)
{
    (void)pData;
    (void)size;
    return 0x0123456789ABCDEFull;
}

// Reads the whole file with plain stdio into pBuf and returns its size, or SIZE_MAX when the file
// does not exist or does not fit.
static size_t stdRenderCaptureTest_ReadFile(const char* pPath, uint8_t* pBuf, size_t bufSize)
{
    FILE* pFile = fopen(pPath, "rb");
    long size;
    size_t numRead;

    if ( !pFile )
    {
        return (size_t)-1;
    }

    fseek(pFile, 0, SEEK_END);
    size = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    if ( size < 0 || (size_t)size > bufSize )
    {
        fclose(pFile);
        return (size_t)-1;
    }

    numRead = fread(pBuf, 1u, (size_t)size, pFile);
    fclose(pFile);
    TEST_ASSERT_EQUAL_size_t((size_t)size, numRead);
    return (size_t)size;
}

static int stdRenderCaptureTest_FileExists(const char* pPath)
{
    FILE* pFile = fopen(pPath, "rb");

    if ( !pFile )
    {
        return 0;
    }

    fclose(pFile);
    return 1;
}

TEST_GROUP(stdRenderCapture);

TEST_SETUP(stdRenderCapture)
{
    StdGeneralTest_Startup();
    stdRenderCapture_ResetGlobals();
}

TEST_TEAR_DOWN(stdRenderCapture)
{
    stdRenderCapture_ResetGlobals();
    StdGeneralTest_Shutdown();
}

TEST(stdRenderCapture, TestHeaderOnly)
{
    uint8_t aFile[512];
    size_t size;

    remove("stdRenderCapture_HeaderOnly.rcap");
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_HeaderOnly.rcap", "null"));
    TEST_ASSERT_TRUE(stdRenderCapture_IsOpen());
    stdRenderCapture_Close();

    size = stdRenderCaptureTest_ReadFile("stdRenderCapture_HeaderOnly.rcap", aFile, sizeof(aFile));
    TEST_ASSERT_EQUAL_size_t(sizeof(stdRenderCaptureTest_aHeaderNull), size);
    TEST_ASSERT_EQUAL_MEMORY(stdRenderCaptureTest_aHeaderNull, aFile, sizeof(stdRenderCaptureTest_aHeaderNull));

    remove("stdRenderCapture_HeaderOnly.rcap");
}

TEST(stdRenderCapture, TestRecordWritesEveryValueType)
{
    static const uint8_t aExpected[] = {
        // header, column "null"
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // record header: id 0x1234, reserved 0, payloadSize 36
        0x34, 0x12, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
        // PutU8(0xAB)
        0xAB,
        // PutBool(true)
        0x01,
        // PutU16(0x4F4E)
        0x4E, 0x4F,
        // PutU32(0x89ABCDEF)
        0xEF, 0xCD, 0xAB, 0x89,
        // PutI32(-2)
        0xFE, 0xFF, 0xFF, 0xFF,
        // PutU64(0x0102030405060708)
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
        // PutF32(1.5f) = bits 0x3FC00000
        0x00, 0x00, 0xC0, 0x3F,
        // PutString("hi") = u32 2, 'h', 'i'
        0x02, 0x00, 0x00, 0x00, 0x68, 0x69,
        // PutString(NULL) = u32 0xFFFFFFFF only
        0xFF, 0xFF, 0xFF, 0xFF,
        // PutBytes(0xDE, 0xAD)
        0xDE, 0xAD,
    };
    static const uint8_t aRawBytes[2] = { 0xDE, 0xAD };
    uint8_t aFile[512];
    size_t size;

    remove("stdRenderCapture_RecordBytes.rcap");
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_RecordBytes.rcap", "null"));

    stdRenderCapture_BeginRecord(0x1234);
    stdRenderCapture_PutU8(0xAB);
    stdRenderCapture_PutBool(true);
    stdRenderCapture_PutU16(0x4F4E);
    stdRenderCapture_PutU32(0x89ABCDEF);
    stdRenderCapture_PutI32(-2);
    stdRenderCapture_PutU64(0x0102030405060708ull);
    stdRenderCapture_PutF32(1.5f);
    stdRenderCapture_PutString("hi");
    stdRenderCapture_PutString(NULL);
    stdRenderCapture_PutBytes(aRawBytes, sizeof(aRawBytes));
    TEST_ASSERT_TRUE(stdRenderCapture_EndRecord());

    stdRenderCapture_Close();
    size = stdRenderCaptureTest_ReadFile("stdRenderCapture_RecordBytes.rcap", aFile, sizeof(aFile));
    TEST_ASSERT_EQUAL_size_t(sizeof(aExpected), size);
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aFile, sizeof(aExpected));

    remove("stdRenderCapture_RecordBytes.rcap");
}

TEST(stdRenderCapture, TestBlobDedupAndInvalidArgs)
{
    static const uint8_t aExpected[] = {
        // header, column "null"
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // BLOB record, id 1: payloadSize 16 = u32 blobId + u64 size + 4 bytes
        0xF0, 0xFF, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x41, 0x41, 0x41, 0x41,
        // BLOB record, id 2
        0xF0, 0xFF, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x41, 0x41, 0x41, 0x42,
        // empty record, id 7, written after the rejected WriteBlob inside it
        0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    uint8_t aFile[512];
    size_t size;

    remove("stdRenderCapture_Blobs.rcap");
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_Blobs.rcap", "null"));

    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_WriteBlob("AAAA", 4u));
    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_WriteBlob("AAAA", 4u));
    TEST_ASSERT_EQUAL_UINT32(2u, stdRenderCapture_WriteBlob("AAAB", 4u));
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_WriteBlob(NULL, 4u));
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_WriteBlob("AAAA", 0u));

    stdRenderCapture_BeginRecord(7);
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_WriteBlob("AAAA", 4u));
    TEST_ASSERT_TRUE(stdRenderCapture_EndRecord());

    stdRenderCapture_Close();
    size = stdRenderCaptureTest_ReadFile("stdRenderCapture_Blobs.rcap", aFile, sizeof(aFile));
    TEST_ASSERT_EQUAL_size_t(sizeof(aExpected), size);
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aFile, sizeof(aExpected));

    remove("stdRenderCapture_Blobs.rcap");
}

TEST(stdRenderCapture, TestBlobForcedHashCollision)
{
    static const uint8_t aExpected[] = {
        // header, column "null"
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // BLOB record, id 1: "AAAA"
        0xF0, 0xFF, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x41, 0x41, 0x41, 0x41,
        // BLOB record, id 2: "BBBB", same forced hash and size, bytes differ on read-back
        0xF0, 0xFF, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x42, 0x42, 0x42, 0x42,
    };
    uint8_t aFile[512];
    size_t size;

    remove("stdRenderCapture_Collision.rcap");
    stdRenderCapture_SetHashFuncForTest(stdRenderCaptureTest_ConstantHash);
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_Collision.rcap", "null"));

    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_WriteBlob("AAAA", 4u));
    TEST_ASSERT_EQUAL_UINT32(2u, stdRenderCapture_WriteBlob("BBBB", 4u));
    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_WriteBlob("AAAA", 4u));
    TEST_ASSERT_EQUAL_UINT32(2u, stdRenderCapture_WriteBlob("BBBB", 4u));

    stdRenderCapture_Close();
    size = stdRenderCaptureTest_ReadFile("stdRenderCapture_Collision.rcap", aFile, sizeof(aFile));
    TEST_ASSERT_EQUAL_size_t(sizeof(aExpected), size);
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aFile, sizeof(aExpected));

    remove("stdRenderCapture_Collision.rcap");
}

TEST(stdRenderCapture, TestHandles)
{
    static int aTestObjects[4];
    const int unknownObject = 7;
    uint8_t aFile[512];
    size_t size;
    uint32_t handle;

    remove("stdRenderCapture_Handles.rcap");
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_Handles.rcap", "null"));

    // NULL is never a handle, live or not.
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, NULL));
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, NULL));
    stdRenderCapture_ReleaseHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, NULL);

    handle = stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aTestObjects[0]);
    TEST_ASSERT_EQUAL_UINT32(1u, handle);
    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aTestObjects[0]));
    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aTestObjects[0]));
    TEST_ASSERT_EQUAL_UINT32(2u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aTestObjects[1]));
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &unknownObject));

    // The kinds are independent: the same pointer gets a fresh 1 for the texture kind.
    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_SYSTEXTURE, &aTestObjects[0]));

    // Released pointers get a new, never-reused number when they come back.
    stdRenderCapture_ReleaseHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aTestObjects[0]);
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aTestObjects[0]));
    stdRenderCapture_ReleaseHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aTestObjects[0]); // no-op
    TEST_ASSERT_EQUAL_UINT32(3u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aTestObjects[0]));

    stdRenderCapture_Close();

    // Handles restart at 1 with every capture.
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_Handles.rcap", "null"));
    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aTestObjects[2]));
    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_SYSTEXTURE, &aTestObjects[3]));
    stdRenderCapture_Close();

    size = stdRenderCaptureTest_ReadFile("stdRenderCapture_Handles.rcap", aFile, sizeof(aFile));
    TEST_ASSERT_EQUAL_size_t(sizeof(stdRenderCaptureTest_aHeaderNull), size);
    TEST_ASSERT_EQUAL_MEMORY(stdRenderCaptureTest_aHeaderNull, aFile, sizeof(stdRenderCaptureTest_aHeaderNull));

    remove("stdRenderCapture_Handles.rcap");
}

TEST(stdRenderCapture, TestReleasedHandlesLeaveTheTable)
{
    static uint8_t aPointerPool[10000u * 4u];
    static int aLiveKept[3];
    static int aNextNew;
    int leaked;
    int i;

    remove("stdRenderCapture_HandleTable.rcap");
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_HandleTable.rcap", "null"));

    // The handles assigned first stay live through the churn.
    for ( i = 0; i < 3; ++i )
    {
        TEST_ASSERT_EQUAL_UINT32((uint32_t)(i + 1),
            stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aLiveKept[i]));
    }

    // 10000 distinct pointers, each added and released again.
    for ( i = 0; i < 10000; ++i )
    {
        const void* p = &aPointerPool[(size_t)i * 4u];
        TEST_ASSERT_EQUAL_UINT32((uint32_t)(i + 4),
            stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, p));
        stdRenderCapture_ReleaseHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, p);
    }

    // The live handles are unaffected, and none of the released pointers is still findable.
    for ( i = 0; i < 3; ++i )
    {
        TEST_ASSERT_EQUAL_UINT32((uint32_t)(i + 1),
            stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aLiveKept[i]));
    }

    leaked = 0;
    for ( i = 0; i < 10000; ++i )
    {
        if ( stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aPointerPool[(size_t)i * 4u]) != 0u )
        {
            ++leaked;
        }
    }
    TEST_ASSERT_EQUAL_INT(0, leaked);

    // The next new pointer continues the per-kind counter: 10000 released + 3 live + 1.
    TEST_ASSERT_EQUAL_UINT32(10004u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aNextNew));

    stdRenderCapture_Close();
    remove("stdRenderCapture_HandleTable.rcap");
}

TEST(stdRenderCapture, TestGlobals)
{
    static const uint8_t aExpected[] = {
        // header, column "null"
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // GLOBAL 1 = 0x11111111 (the first write for the id always emits)
        0xF1, 0xFF, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11,
        // GLOBAL 1 = 0x22222222 (changed bits emit)
        0xF1, 0xFF, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22,
        // empty record, id 3, written after the rejected WriteGlobal inside it
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // GLOBAL 8 = 0xAAAAAAAA (id 8 is the last valid id)
        0xF1, 0xFF, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA,
    };
    uint8_t aFile[512];
    size_t size;

    remove("stdRenderCapture_Globals.rcap");
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_Globals.rcap", "null"));

    TEST_ASSERT_TRUE(stdRenderCapture_WriteGlobal(1u, 0x11111111u));
    TEST_ASSERT_TRUE(stdRenderCapture_WriteGlobal(1u, 0x11111111u)); // unchanged: no record
    TEST_ASSERT_TRUE(stdRenderCapture_WriteGlobal(1u, 0x22222222u));
    TEST_ASSERT_FALSE(stdRenderCapture_WriteGlobal(0u, 1u));
    TEST_ASSERT_FALSE(stdRenderCapture_WriteGlobal(9u, 1u));

    stdRenderCapture_BeginRecord(3);
    TEST_ASSERT_FALSE(stdRenderCapture_WriteGlobal(2u, 5u)); // inside a record
    TEST_ASSERT_TRUE(stdRenderCapture_EndRecord());

    TEST_ASSERT_TRUE(stdRenderCapture_WriteGlobal(8u, 0xAAAAAAAAu));
    TEST_ASSERT_TRUE(stdRenderCapture_WriteGlobal(8u, 0xAAAAAAAAu));

    stdRenderCapture_Close();
    size = stdRenderCaptureTest_ReadFile("stdRenderCapture_Globals.rcap", aFile, sizeof(aFile));
    TEST_ASSERT_EQUAL_size_t(sizeof(aExpected), size);
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aFile, sizeof(aExpected));

    remove("stdRenderCapture_Globals.rcap");
}

TEST(stdRenderCapture, TestFramesRecords)
{
    static const uint8_t aExpectedDisabledHeader[29] = {
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // FRAMES 0, written right after the header because the flag was off at Open
        0xF2, 0xFF, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    };
    static const uint8_t aExpectedToggled[38] = {
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // FRAMES 0
        0xF2, 0xFF, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        // FRAMES 1
        0xF2, 0xFF, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    };
    uint8_t aFile[512];
    size_t size;

    // The flag defaults to true.
    TEST_ASSERT_TRUE(stdRenderCapture_GetFramesEnabled());

    remove("stdRenderCapture_FramesToggle.rcap");
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_FramesToggle.rcap", "null"));
    stdRenderCapture_SetFramesEnabled(false);
    stdRenderCapture_SetFramesEnabled(false); // same value: no second record
    stdRenderCapture_SetFramesEnabled(true);
    stdRenderCapture_Close();

    size = stdRenderCaptureTest_ReadFile("stdRenderCapture_FramesToggle.rcap", aFile, sizeof(aFile));
    TEST_ASSERT_EQUAL_size_t(sizeof(aExpectedToggled), size);
    TEST_ASSERT_EQUAL_MEMORY(aExpectedToggled, aFile, sizeof(aExpectedToggled));

    // Disabling while closed keeps the flag; Open then writes FRAMES 0 after the header, and the
    // flag survives Close.
    stdRenderCapture_SetFramesEnabled(false);
    remove("stdRenderCapture_FramesDisabled.rcap");
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_FramesDisabled.rcap", "null"));
    stdRenderCapture_Close();
    TEST_ASSERT_FALSE(stdRenderCapture_GetFramesEnabled());

    size = stdRenderCaptureTest_ReadFile("stdRenderCapture_FramesDisabled.rcap", aFile, sizeof(aFile));
    TEST_ASSERT_EQUAL_size_t(sizeof(aExpectedDisabledHeader), size);
    TEST_ASSERT_EQUAL_MEMORY(aExpectedDisabledHeader, aFile, sizeof(aExpectedDisabledHeader));

    remove("stdRenderCapture_FramesToggle.rcap");
    remove("stdRenderCapture_FramesDisabled.rcap");
}

TEST(stdRenderCapture, TestRecordApiWithoutCapture)
{
    stdRenderCapture_BeginRecord(1);
    stdRenderCapture_PutU8(2);
    stdRenderCapture_PutBool(true);
    stdRenderCapture_PutU16(3);
    stdRenderCapture_PutU32(4u);
    stdRenderCapture_PutI32(5);
    stdRenderCapture_PutU64(6u);
    stdRenderCapture_PutF32(7.5f);
    stdRenderCapture_PutString("eight");
    stdRenderCapture_PutBytes("nine", 4u);
    TEST_ASSERT_FALSE(stdRenderCapture_EndRecord());
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_WriteBlob("ten", 3u));
    TEST_ASSERT_FALSE(stdRenderCapture_WriteGlobal(1u, 2u));
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, "p"));
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, "p"));
    stdRenderCapture_ReleaseHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, "p");
    stdRenderCapture_Close(); // safe no-op
    stdRenderCapture_Close();
    TEST_ASSERT_FALSE(stdRenderCapture_IsOpen());
    stdRenderCapture_SetFramesEnabled(true); // allowed while closed, it is not a record call
    TEST_ASSERT_TRUE(stdRenderCapture_GetFramesEnabled());
}

TEST(stdRenderCapture, TestOpenFailures)
{
    static const uint8_t aExpected[29] = {
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // the capture that was already open stays writable: record id 5, payload u8 9
        0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x09,
    };
    uint8_t aFile[512];
    size_t size;

    remove("stdRenderCapture_AlreadyOpen.rcap");
    remove("stdRenderCapture_SecondOpen.rcap");
    remove("stdRenderCapture_OpenFails.rcap");

    TEST_ASSERT_FALSE(stdRenderCapture_Open(NULL, "null"));
    TEST_ASSERT_FALSE(stdRenderCapture_Open("stdRenderCapture_AlreadyOpen.rcap", NULL));
    TEST_ASSERT_FALSE(stdRenderCapture_Open("stdRenderCapture_AlreadyOpen.rcap", ""));
    TEST_ASSERT_FALSE(stdRenderCapture_Open("stdRenderCapture_AlreadyOpen.rcap", stdRenderCaptureTest_aLongColumn));
    TEST_ASSERT_FALSE(stdRenderCapture_IsOpen());

    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_AlreadyOpen.rcap", "null"));
    TEST_ASSERT_FALSE(stdRenderCapture_Open("stdRenderCapture_SecondOpen.rcap", "other"));
    TEST_ASSERT_FALSE(stdRenderCaptureTest_FileExists("stdRenderCapture_SecondOpen.rcap"));
    TEST_ASSERT_TRUE(stdRenderCapture_IsOpen());

    // The first capture stays writable after the rejected second Open.
    stdRenderCapture_BeginRecord(5);
    stdRenderCapture_PutU8(9);
    TEST_ASSERT_TRUE(stdRenderCapture_EndRecord());
    stdRenderCapture_Close();

    size = stdRenderCaptureTest_ReadFile("stdRenderCapture_AlreadyOpen.rcap", aFile, sizeof(aFile));
    TEST_ASSERT_EQUAL_size_t(sizeof(aExpected), size);
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aFile, sizeof(aExpected));

    StdGeneralTest_FailNextFileOpens(0);
    TEST_ASSERT_FALSE(stdRenderCapture_Open("stdRenderCapture_OpenFails.rcap", "null"));
    TEST_ASSERT_FALSE(stdRenderCapture_IsOpen());
    StdGeneralTest_ClearFileFailures();

    remove("stdRenderCapture_AlreadyOpen.rcap");
    remove("stdRenderCapture_OpenFails.rcap");
}

TEST(stdRenderCapture, TestWriteFailureClosesCapture)
{
    remove("stdRenderCapture_WriteFail.rcap");
    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_WriteFail.rcap", "null"));

    StdGeneralTest_FailNextFileWrites(0);
    stdRenderCapture_BeginRecord(1);
    stdRenderCapture_PutU8(2);
    TEST_ASSERT_FALSE(stdRenderCapture_EndRecord());
    TEST_ASSERT_FALSE(stdRenderCapture_IsOpen());
    StdGeneralTest_ClearFileFailures();

    // Later calls are no-ops on the closed module.
    TEST_ASSERT_FALSE(stdRenderCapture_EndRecord());
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, "p"));

    // What reached the file is just the header.
    {
        uint8_t aFile[512];
        size_t size = stdRenderCaptureTest_ReadFile("stdRenderCapture_WriteFail.rcap", aFile, sizeof(aFile));
        TEST_ASSERT_EQUAL_size_t(sizeof(stdRenderCaptureTest_aHeaderNull), size);
        TEST_ASSERT_EQUAL_MEMORY(stdRenderCaptureTest_aHeaderNull, aFile, sizeof(stdRenderCaptureTest_aHeaderNull));
    }

    remove("stdRenderCapture_WriteFail.rcap");
}

// Runs the sequence twice with different pointer objects; the bytes must not depend on that.
static void stdRenderCaptureTest_WriteDeterministicSequence(const char* pPath, const void* pObject)
{
    uint32_t handle;

    TEST_ASSERT_TRUE(stdRenderCapture_Open(pPath, "null"));
    handle = stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, pObject);
    stdRenderCapture_BeginRecord(0x2A);
    stdRenderCapture_PutU32(7u);
    stdRenderCapture_PutString("abc");
    TEST_ASSERT_TRUE(stdRenderCapture_EndRecord());
    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_WriteBlob("DATA", 4u));
    TEST_ASSERT_TRUE(stdRenderCapture_WriteGlobal(3u, 0x55u));
    stdRenderCapture_BeginRecord(0x2B);
    stdRenderCapture_PutU32(handle);
    TEST_ASSERT_TRUE(stdRenderCapture_EndRecord());
    stdRenderCapture_Close();
}

TEST(stdRenderCapture, TestDeterministicBytes)
{
    static int aObjectOne;
    static int aObjectTwo;
    uint8_t aFileOne[512];
    uint8_t aFileTwo[512];
    size_t sizeOne;
    size_t sizeTwo;

    remove("stdRenderCapture_DeterminismA.rcap");
    remove("stdRenderCapture_DeterminismB.rcap");

    stdRenderCaptureTest_WriteDeterministicSequence("stdRenderCapture_DeterminismA.rcap", &aObjectOne);
    stdRenderCapture_ResetGlobals();
    stdRenderCaptureTest_WriteDeterministicSequence("stdRenderCapture_DeterminismB.rcap", &aObjectTwo);

    sizeOne = stdRenderCaptureTest_ReadFile("stdRenderCapture_DeterminismA.rcap", aFileOne, sizeof(aFileOne));
    sizeTwo = stdRenderCaptureTest_ReadFile("stdRenderCapture_DeterminismB.rcap", aFileTwo, sizeof(aFileTwo));
    TEST_ASSERT_TRUE(sizeOne > 0u);
    TEST_ASSERT_EQUAL_size_t(sizeOne, sizeTwo);
    TEST_ASSERT_EQUAL_MEMORY(aFileOne, aFileTwo, sizeOne);

    remove("stdRenderCapture_DeterminismA.rcap");
    remove("stdRenderCapture_DeterminismB.rcap");
}

// Matches when the live entry pointer is one of the wanted pointers in pContext.
typedef struct sStdRenderCaptureTest_MatchSet
{
    const void* apWant[4];
    size_t numWant;
} StdRenderCaptureTest_MatchSet;

static bool stdRenderCaptureTest_MatchAny(const void* p, const void* pContext)
{
    const StdRenderCaptureTest_MatchSet* pSet = (const StdRenderCaptureTest_MatchSet*)pContext;
    size_t i;

    for ( i = 0; i < pSet->numWant; ++i )
    {
        if ( pSet->apWant[i] == p )
        {
            return true;
        }
    }

    return false;
}

TEST(stdRenderCapture, TestFindHandleWhere)
{
    static int aObjects[5];
    const StdRenderCaptureTest_MatchSet matchA2 = { { &aObjects[2], NULL, NULL, NULL }, 1u };
    const StdRenderCaptureTest_MatchSet matchA2A0 = { { &aObjects[2], &aObjects[0], NULL, NULL }, 2u };
    const StdRenderCaptureTest_MatchSet matchNone = { { &aObjects[4], NULL, NULL, NULL }, 1u };
    const StdRenderCaptureTest_MatchSet matchAll = {
        { &aObjects[0], &aObjects[1], &aObjects[2], &aObjects[3] }, 4u
    };
    const StdRenderCaptureTest_MatchSet matchA0A3 = {
        { &aObjects[0], &aObjects[3], NULL, NULL }, 2u
    };

    remove("stdRenderCapture_FindHandleWhere.rcap");

    // A NULL predicate and any call while closed return 0.
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, stdRenderCaptureTest_MatchAny, &matchA2));
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, NULL, NULL));

    TEST_ASSERT_TRUE(stdRenderCapture_Open("stdRenderCapture_FindHandleWhere.rcap", "null"));

    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aObjects[0]));
    TEST_ASSERT_EQUAL_UINT32(2u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aObjects[1]));
    TEST_ASSERT_EQUAL_UINT32(3u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aObjects[2]));

    // Single match, and the first entry in assignment order when several match.
    TEST_ASSERT_EQUAL_UINT32(3u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, stdRenderCaptureTest_MatchAny, &matchA2));
    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, stdRenderCaptureTest_MatchAny, &matchA2A0));

    // No match, a NULL predicate and the other kind all return 0.
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, stdRenderCaptureTest_MatchAny, &matchNone));
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, NULL, &matchA2));
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_SYSTEXTURE, stdRenderCaptureTest_MatchAny, &matchAll));

    // Released entries are skipped: with aObjects[0] released, the first live match of the
    // assignment order is now aObjects[2].
    stdRenderCapture_ReleaseHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aObjects[0]);
    TEST_ASSERT_EQUAL_UINT32(3u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, stdRenderCaptureTest_MatchAny, &matchA2A0));
    TEST_ASSERT_EQUAL_UINT32(2u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, stdRenderCaptureTest_MatchAny, &matchAll));

    // A newly added entry keeps the assignment order at the end of the table: the released
    // aObjects[0] is skipped and the new aObjects[3] matches last.
    TEST_ASSERT_EQUAL_UINT32(4u, stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &aObjects[3]));
    TEST_ASSERT_EQUAL_UINT32(4u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, stdRenderCaptureTest_MatchAny, &matchA0A3));

    // It may be called while a record is open and writes nothing.
    stdRenderCapture_BeginRecord(0x21);
    TEST_ASSERT_EQUAL_UINT32(4u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, stdRenderCaptureTest_MatchAny, &matchA0A3));
    TEST_ASSERT_TRUE(stdRenderCapture_EndRecord());

    stdRenderCapture_Close();
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandleWhere(STDRENDERCAPTURE_HANDLE_VBUFFER, stdRenderCaptureTest_MatchAny, &matchA2));

    // Only the 8-byte record header reached the file: the handle table and the query write nothing.
    {
        static const uint8_t aExpected[] = {
            0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
            0x01, 0x00, 0x00, 0x00,
            0x04, 0x00, 0x00, 0x00,
            0x6E, 0x75, 0x6C, 0x6C,
            0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };
        uint8_t aFile[512];
        size_t size = stdRenderCaptureTest_ReadFile("stdRenderCapture_FindHandleWhere.rcap", aFile, sizeof(aFile));
        TEST_ASSERT_EQUAL_size_t(sizeof(aExpected), size);
        TEST_ASSERT_EQUAL_MEMORY(aExpected, aFile, sizeof(aExpected));
    }

    remove("stdRenderCapture_FindHandleWhere.rcap");
}

TEST_GROUP_RUNNER(stdRenderCapture)
{
    RUN_TEST_CASE(stdRenderCapture, TestHeaderOnly);
    RUN_TEST_CASE(stdRenderCapture, TestRecordWritesEveryValueType);
    RUN_TEST_CASE(stdRenderCapture, TestBlobDedupAndInvalidArgs);
    RUN_TEST_CASE(stdRenderCapture, TestBlobForcedHashCollision);
    RUN_TEST_CASE(stdRenderCapture, TestHandles);
    RUN_TEST_CASE(stdRenderCapture, TestFindHandleWhere);
    RUN_TEST_CASE(stdRenderCapture, TestReleasedHandlesLeaveTheTable);
    RUN_TEST_CASE(stdRenderCapture, TestGlobals);
    RUN_TEST_CASE(stdRenderCapture, TestFramesRecords);
    RUN_TEST_CASE(stdRenderCapture, TestRecordApiWithoutCapture);
    RUN_TEST_CASE(stdRenderCapture, TestOpenFailures);
    RUN_TEST_CASE(stdRenderCapture, TestWriteFailureClosesCapture);
    RUN_TEST_CASE(stdRenderCapture, TestDeterministicBytes);
}
