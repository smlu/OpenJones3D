#include <unity_fixture.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <smush/insane/File/Cdstream.h>
#include <smush/insane/File/Stdfile.h>
#include <smush/insane/File/Zfilio/Zfilio.h>
#include <smush/insane/Memory/Memory.h>

#define ZLIBTEST_MAX_FILES    8
#define ZLIBTEST_PAYLOAD_SIZE (0x10000 + 37)
#define ZLIBTEST_PACKED_SIZE  (ZLIBTEST_PAYLOAD_SIZE + 0x1000)

static char zlibTest_aDirectory[MAX_PATH];
static char zlibTest_aPaths[ZLIBTEST_MAX_FILES][MAX_PATH];
static int zlibTest_numPaths;
static FILE* zlibTest_apFiles[ZLIBTEST_MAX_FILES];
static FILE* zlibTest_pRawFile;
static gzFile zlibTest_hFixture;
static ZfileHandle* zlibTest_pFile;
static Stream* zlibTest_pStream;
static ZfilioInterface* zlibTest_pZfilio;
static StdFileInterface* zlibTest_pStdfile;
static int zlibTest_chainZfilio;
static int zlibTest_bStarted;
static int zlibTest_bStdfileStarted;
static int zlibTest_bStreamerStarted;
static int zlibTest_bFailAlloc;
static int zlibTest_numAllocs;
static int zlibTest_numFrees;
static int zlibTest_numFileOpens;
static int zlibTest_numFileCloses;
static int zlibTest_numFileFlushes;
static int zlibTest_numErrors;
static uint8_t zlibTest_aPayload[ZLIBTEST_PAYLOAD_SIZE];
static uint8_t zlibTest_aOutput[ZLIBTEST_PAYLOAD_SIZE + 16];
static uint8_t zlibTest_aPacked[ZLIBTEST_PACKED_SIZE];

static void* ZlibTest_Alloc(int size, const char* pFile, int line)
{
    J3D_UNUSED(pFile);
    J3D_UNUSED(line);
    if ( zlibTest_bFailAlloc )
    {
        return NULL;
    }

    void* pBlock = malloc(size);
    if ( pBlock )
    {
        ++zlibTest_numAllocs;
        memset(pBlock, 0xA5, size);
    }
    return pBlock;
}

static void ZlibTest_Free(void* pBlock, const char* pFile, int line)
{
    J3D_UNUSED(pFile);
    J3D_UNUSED(line);
    ++zlibTest_numFrees;
    free(pBlock);
}

static MemoryDebugInterface zlibTest_memory = { ZlibTest_Alloc, ZlibTest_Free, NULL };

static int ZlibTest_FileOpen(const char* pPath, const char* pMode)
{
    ++zlibTest_numFileOpens;
    FILE* pFile = NULL;
    if ( fopen_s(&pFile, pPath, pMode) != 0 )
    {
        return 0;
    }
    for ( int i = 0; i < ZLIBTEST_MAX_FILES; ++i )
    {
        if ( !zlibTest_apFiles[i] )
        {
            zlibTest_apFiles[i] = pFile;
            return i + 1;
        }
    }
    fclose(pFile);
    return 0;
}

static void ZlibTest_FileClose(int handle)
{
    if ( handle > 0 && handle <= ZLIBTEST_MAX_FILES && zlibTest_apFiles[handle - 1] )
    {
        fclose(zlibTest_apFiles[handle - 1]);
        zlibTest_apFiles[handle - 1] = NULL;
        ++zlibTest_numFileCloses;
    }
}

static int ZlibTest_FileRead(int handle, void* pBuffer, int size)
{
    if ( handle <= 0 || handle > ZLIBTEST_MAX_FILES || !zlibTest_apFiles[handle - 1] )
    {
        return 0;
    }
    return (int)fread(pBuffer, 1, size, zlibTest_apFiles[handle - 1]);
}

static int ZlibTest_FileWrite(int handle, const void* pBuffer, int size)
{
    return (int)fwrite(pBuffer, 1, size, zlibTest_apFiles[handle - 1]);
}

static int ZlibTest_FileTell(int handle)
{
    return (int)ftell(zlibTest_apFiles[handle - 1]);
}

static int ZlibTest_FileSeek(int handle, int offset, int whence)
{
    if ( handle <= 0 || handle > ZLIBTEST_MAX_FILES || !zlibTest_apFiles[handle - 1] )
    {
        return -1;
    }
    return fseek(zlibTest_apFiles[handle - 1], offset, whence);
}

static void ZlibTest_FileFlush(int handle)
{
    ++zlibTest_numFileFlushes;
    fflush(zlibTest_apFiles[handle - 1]);
}

static FilioInterface zlibTest_filio = {
    ZlibTest_FileOpen, ZlibTest_FileClose, ZlibTest_FileRead, ZlibTest_FileWrite,
    ZlibTest_FileTell, ZlibTest_FileSeek, ZlibTest_FileFlush
};

static int ZlibTest_Publish(const char* pName, void* pInterface)
{
    if ( strcmp(pName, "FILIO=insane_zfilio") == 0 )
    {
        zlibTest_pZfilio = pInterface;
    }
    else if ( strcmp(pName, "FILE=insane_stdfile") == 0 )
    {
        zlibTest_pStdfile = pInterface;
    }
    return 0;
}

static void ZlibTest_Unpublish(const char* pName)
{
    J3D_UNUSED(pName);
}

static void* ZlibTest_Subscribe(const char* pName, void* pParam)
{
    if ( strcmp(pName, "FILIO.chainto") == 0 )
    {
        return pParam == &zlibTest_chainZfilio ? (void*)zlibTest_pZfilio : &zlibTest_filio;
    }
    if ( strcmp(pName, "MEMORY.buffers") == 0 || strcmp(pName, "MEMORY.workspace") == 0 )
    {
        return &zlibTest_memory;
    }
    if ( strcmp(pName, "FILIO.io") == 0 )
    {
        return zlibTest_pStdfile;
    }
    return NULL;
}

static void ZlibTest_Unsubscribe(void* pInterface)
{
    J3D_UNUSED(pInterface);
}

static void ZlibTest_ReportError(const char* pFormat, ...)
{
    J3D_UNUSED(pFormat);
    ++zlibTest_numErrors;
}

static LecShare zlibTest_share = {
    ZlibTest_Publish, ZlibTest_Unpublish, ZlibTest_Subscribe, ZlibTest_Unsubscribe,
    NULL, NULL, NULL, NULL, ZlibTest_ReportError
};

static const char* ZlibTest_Path(const char* pName)
{
    TEST_ASSERT_LESS_THAN_INT(ZLIBTEST_MAX_FILES, zlibTest_numPaths);
    char* pPath = zlibTest_aPaths[zlibTest_numPaths++];
    int size    = snprintf(pPath, MAX_PATH, "%s\\%s", zlibTest_aDirectory, pName);
    TEST_ASSERT_TRUE(size > 0 && size < MAX_PATH);
    return pPath;
}

static void ZlibTest_WriteRaw(const char* pPath, const void* pData, int size)
{
    TEST_ASSERT_EQUAL_INT(0, fopen_s(&zlibTest_pRawFile, pPath, "wb"));
    size_t written = fwrite(pData, 1, size, zlibTest_pRawFile);
    int rc         = fclose(zlibTest_pRawFile);
    zlibTest_pRawFile = NULL;
    TEST_ASSERT_EQUAL_INT(size, (int)written);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

static int ZlibTest_ReadRaw(const char* pPath)
{
    TEST_ASSERT_EQUAL_INT(0, fopen_s(&zlibTest_pRawFile, pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(0, fseek(zlibTest_pRawFile, 0, SEEK_END));
    long size = ftell(zlibTest_pRawFile);
    TEST_ASSERT_TRUE(size >= 0 && size <= ZLIBTEST_PACKED_SIZE);
    rewind(zlibTest_pRawFile);
    size_t read = fread(zlibTest_aPacked, 1, size, zlibTest_pRawFile);
    int rc      = fclose(zlibTest_pRawFile);
    zlibTest_pRawFile = NULL;
    TEST_ASSERT_EQUAL_INT((int)size, (int)read);
    TEST_ASSERT_EQUAL_INT(0, rc);
    return (int)size;
}

static void ZlibTest_WriteGzip(const char* pPath, const void* pData, int size)
{
    zlibTest_hFixture = gzopen(pPath, "wb");
    TEST_ASSERT_NOT_NULL(zlibTest_hFixture);
    TEST_ASSERT_EQUAL_INT(size, gzwrite(zlibTest_hFixture, pData, size));
    int rc = gzclose(zlibTest_hFixture);
    zlibTest_hFixture = NULL;
    TEST_ASSERT_EQUAL_INT(Z_OK, rc);
}

static ZfileHandle* ZlibTest_Open(const char* pPath, const char* pMode)
{
    TEST_ASSERT_NULL(zlibTest_pFile);
    zlibTest_pFile = zfilio_open(pPath, pMode);
    return zlibTest_pFile;
}

static void ZlibTest_Close(void)
{
    if ( zlibTest_pFile )
    {
        zfilio_close(zlibTest_pFile);
        zlibTest_pFile = NULL;
    }
    TEST_ASSERT_EQUAL_INT(zlibTest_numAllocs, zlibTest_numFrees);
}

static void ZlibTest_OpenWriter(const char* pPath)
{
    // zfilio_open intentionally routes write modes to the chained FILIO. Construct
    // a valid gzip-backed handle to exercise the retained gzwrite/gzflush wrappers.
    zlibTest_pFile = ZlibTest_Alloc(sizeof(*zlibTest_pFile), J3D_FILE, __LINE__);
    TEST_ASSERT_NOT_NULL(zlibTest_pFile);
    memset(zlibTest_pFile, 0, sizeof(*zlibTest_pFile));
    zlibTest_pFile->isGz  = 1;
    zlibTest_pFile->hFile = gzopen(pPath, "wb");
    TEST_ASSERT_NOT_NULL(zlibTest_pFile->hFile);
}

static int ZlibTest_Drain(void)
{
    int total = 0;
    int rc    = 0;
    for ( int i = 0; i < 128; ++i )
    {
        int size = (int)sizeof(zlibTest_aOutput) - total;
        if ( size > 4096 )
        {
            size = 4096;
        }
        TEST_ASSERT_GREATER_THAN_INT(0, size);
        rc = zfilio_read(zlibTest_pFile, zlibTest_aOutput + total, size);
        if ( rc <= 0 )
        {
            TEST_ASSERT_EQUAL_INT(ZLIBTEST_PAYLOAD_SIZE, total);
            TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, total);
            return rc;
        }
        total += rc;
        TEST_ASSERT_LESS_OR_EQUAL_INT(ZLIBTEST_PAYLOAD_SIZE, total);
    }
    TEST_FAIL_MESSAGE("The gzip stream did not finish within the fixture bound.");
}

TEST_GROUP(SmushZlib);

TEST_SETUP(SmushZlib)
{
    zlibTest_aDirectory[0] = '\0';
    zlibTest_numPaths = 0;
    memset(zlibTest_aPaths, 0, sizeof(zlibTest_aPaths));
    memset(zlibTest_apFiles, 0, sizeof(zlibTest_apFiles));
    zlibTest_pRawFile = NULL;
    zlibTest_hFixture = NULL;
    zlibTest_pFile = NULL;
    zlibTest_pStream = NULL;
    zlibTest_pZfilio = NULL;
    zlibTest_pStdfile = NULL;
    zlibTest_bStarted = zlibTest_bFailAlloc = 0;
    zlibTest_bStdfileStarted = zlibTest_bStreamerStarted = 0;
    zlibTest_numAllocs = zlibTest_numFrees = 0;
    zlibTest_numFileOpens = zlibTest_numFileCloses = zlibTest_numFileFlushes = 0;
    zlibTest_numErrors = 0;
    for ( int i = 0; i < ZLIBTEST_PAYLOAD_SIZE; ++i )
    {
        zlibTest_aPayload[i] = (uint8_t)(i * 29 + (i >> 8));
    }
    memset(zlibTest_aOutput, 0xCC, sizeof(zlibTest_aOutput));

    char aTempPath[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, aTempPath);
    TEST_ASSERT_TRUE(len > 0 && len < MAX_PATH);
    TEST_ASSERT_NOT_EQUAL_UINT(0u, GetTempFileNameA(aTempPath, "OJZ", 0, zlibTest_aDirectory));
    TEST_ASSERT_TRUE(DeleteFileA(zlibTest_aDirectory));
    TEST_ASSERT_TRUE(CreateDirectoryA(zlibTest_aDirectory, NULL));

    Zfilio_ResetGlobals();
    Stdfile_ResetGlobals();
    Cdstream_ResetGlobals();
    Zfilio_TestSetGz(NULL);
    TEST_ASSERT_EQUAL_INT(0, zfilio_Register(&zlibTest_share, NULL));
    TEST_ASSERT_EQUAL_INT(0, zfilio_Startup());
    zlibTest_bStarted = 1;
    TEST_ASSERT_EQUAL_PTR(&zlibTest_filio, zfilio_pFilio);
}

TEST_TEAR_DOWN(SmushZlib)
{
    if ( zlibTest_pRawFile )
    {
        fclose(zlibTest_pRawFile);
        zlibTest_pRawFile = NULL;
    }
    if ( zlibTest_hFixture )
    {
        gzclose(zlibTest_hFixture);
        zlibTest_hFixture = NULL;
    }
    if ( zlibTest_pStream )
    {
        streamer_Destroy(&zlibTest_pStream);
    }
    if ( zlibTest_pFile )
    {
        zfilio_close(zlibTest_pFile);
        zlibTest_pFile = NULL;
    }
    for ( int i = 0; i < ZLIBTEST_MAX_FILES; ++i )
    {
        if ( zlibTest_apFiles[i] )
        {
            fclose(zlibTest_apFiles[i]);
            zlibTest_apFiles[i] = NULL;
        }
    }
    if ( zlibTest_bStreamerStarted )
    {
        streamer_Shutdown();
        streamer_Unregister();
    }
    if ( zlibTest_bStdfileStarted )
    {
        stdfile_Shutdown();
        stdfile_Unregister();
    }
    if ( zlibTest_bStarted )
    {
        zfilio_Shutdown();
        zfilio_Unregister();
    }
    Zfilio_TestSetGz(NULL);
    Zfilio_ResetGlobals();
    Stdfile_ResetGlobals();
    Cdstream_ResetGlobals();

    int bCleanupOk = 1;
    for ( int i = 0; i < zlibTest_numPaths; ++i )
    {
        if ( !DeleteFileA(zlibTest_aPaths[i]) && GetLastError() != ERROR_FILE_NOT_FOUND )
        {
            bCleanupOk = 0;
        }
    }
    if ( zlibTest_aDirectory[0] )
    {
        DeleteFileA(zlibTest_aDirectory); // Also clean a partial setup before CreateDirectory.
        if ( !RemoveDirectoryA(zlibTest_aDirectory) && GetLastError() != ERROR_PATH_NOT_FOUND )
        {
            bCleanupOk = 0;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(bCleanupOk, "Temporary gzip fixtures were not removed.");
    TEST_ASSERT_EQUAL_INT(zlibTest_numAllocs, zlibTest_numFrees);
    TEST_ASSERT_EQUAL_INT(0, zlibTest_numErrors);
}

TEST(SmushZlib, RegisteredPlainSnmFallsBackToChainedFilio)
{
    const char* pPath = ZlibTest_Path("plain.snm");
    ZlibTest_WriteRaw(pPath, zlibTest_aPayload, 257);
    TEST_ASSERT_EQUAL_INT(0, zfilio_Configure(ZFILIO_TAG_EXTENSION, ".snm", 0));
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(0, zlibTest_pFile->isGz);
    TEST_ASSERT_EQUAL_INT(0, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 0));
    TEST_ASSERT_EQUAL_INT(0, zfilio_tell(zlibTest_pFile));
    TEST_ASSERT_EQUAL_HEX8(0xCC, zlibTest_aOutput[0]);
    TEST_ASSERT_EQUAL_INT(257, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 257));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, 257);
    TEST_ASSERT_EQUAL_HEX8(0xCC, zlibTest_aOutput[257]);
    TEST_ASSERT_EQUAL_INT(257, zfilio_tell(zlibTest_pFile));
    TEST_ASSERT_EQUAL_INT(0, zfilio_seek(zlibTest_pFile, -17, SEEK_END));
    TEST_ASSERT_EQUAL_INT(240, zfilio_tell(zlibTest_pFile));
    TEST_ASSERT_EQUAL_INT(17, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 17));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload + 240, zlibTest_aOutput, 17);
    TEST_ASSERT_EQUAL_INT(0, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 1));
    TEST_ASSERT_EQUAL_INT(257, zfilio_tell(zlibTest_pFile));
    ZlibTest_Close();
}

TEST(SmushZlib, GzipAndZnmDecodeCompletePayloadAcrossBuffers)
{
    const char* apNames[] = { "large.gz", "large.znm" };
    TEST_ASSERT_EQUAL_INT(0, zfilio_Configure(ZFILIO_TAG_EXTENSION, ".znm", 0));
    for ( int file = 0; file < 2; ++file )
    {
        const char* pPath = ZlibTest_Path(apNames[file]);
        ZlibTest_WriteGzip(pPath, zlibTest_aPayload, ZLIBTEST_PAYLOAD_SIZE);
        TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
        TEST_ASSERT_EQUAL_INT(1, zlibTest_pFile->isGz);
        TEST_ASSERT_EQUAL_INT(ZLIBTEST_PAYLOAD_SIZE, zlibTest_pFile->gzSize);
        const int aChunks[] = { 17, 4093, 3, 32771, ZLIBTEST_PAYLOAD_SIZE - 17 - 4093 - 3 - 32771 };
        int offset = 0;
        for ( int i = 0; i < 5; ++i )
        {
            TEST_ASSERT_EQUAL_INT(aChunks[i], zfilio_read(zlibTest_pFile, zlibTest_aOutput + offset, aChunks[i]));
            offset += aChunks[i];
            TEST_ASSERT_EQUAL_INT(offset, zfilio_tell(zlibTest_pFile));
        }
        TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, ZLIBTEST_PAYLOAD_SIZE);
        TEST_ASSERT_EQUAL_INT(0, gzdirect(zlibTest_pFile->hFile));
        ZlibTest_Close();
    }
}

TEST(SmushZlib, LazySeeksUseUncompressedOffsetsAndReopenAtStart)
{
    const char* pPath = ZlibTest_Path("seek.gz");
    ZlibTest_WriteGzip(pPath, zlibTest_aPayload, ZLIBTEST_PAYLOAD_SIZE);
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(0, zfilio_seek(zlibTest_pFile, 40001, SEEK_SET));
    TEST_ASSERT_EQUAL_INT(0, (int)gztell(zlibTest_pFile->hFile)); // Zfilio has not sought the backend yet.
    TEST_ASSERT_EQUAL_INT(31, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 31));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload + 40001, zlibTest_aOutput, 31);
    TEST_ASSERT_EQUAL_INT(40032, (int)gztell(zlibTest_pFile->hFile));
    TEST_ASSERT_EQUAL_INT(0, zfilio_seek(zlibTest_pFile, -40000, SEEK_CUR));
    TEST_ASSERT_EQUAL_INT(16, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 16));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload + 32, zlibTest_aOutput, 16);
    TEST_ASSERT_EQUAL_INT(0, zfilio_seek(zlibTest_pFile, -37, SEEK_END));
    TEST_ASSERT_EQUAL_INT(37, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 37));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload + ZLIBTEST_PAYLOAD_SIZE - 37, zlibTest_aOutput, 37);
    ZlibTest_Close();
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(0, zfilio_tell(zlibTest_pFile));
    TEST_ASSERT_EQUAL_INT(19, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 19));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, 19);
    ZlibTest_Close();
}

TEST(SmushZlib, ShortReadAndEofKeepLegacyRequestedPositionThenRewind)
{
    const char* pPath = ZlibTest_Path("short-read.gz");
    ZlibTest_WriteGzip(pPath, zlibTest_aPayload, 5);
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(5, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 16));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, 5);
    TEST_ASSERT_EQUAL_HEX8(0xCC, zlibTest_aOutput[5]);
    TEST_ASSERT_EQUAL_INT(16, zfilio_tell(zlibTest_pFile)); // The original advances by the requested count.
    TEST_ASSERT_EQUAL_INT(5, (int)gztell(zlibTest_pFile->hFile));
    TEST_ASSERT_EQUAL_INT(0, zfilio_read(zlibTest_pFile, zlibTest_aOutput + 5, 3));
    TEST_ASSERT_EQUAL_INT(19, zfilio_tell(zlibTest_pFile));
    TEST_ASSERT_EQUAL_HEX8(0xCC, zlibTest_aOutput[5]);
    TEST_ASSERT_EQUAL_INT(0, zfilio_seek(zlibTest_pFile, 0, SEEK_SET));
    TEST_ASSERT_EQUAL_INT(5, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 5));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, 5);
    ZlibTest_Close();
}

TEST(SmushZlib, EmptyAndMinimalPlainInputsPassThrough)
{
    TEST_ASSERT_EQUAL_INT(0, zfilio_Configure(ZFILIO_TAG_EXTENSION, ".snm", 0));
    for ( int size = 0; size <= 3; ++size )
    {
        char aName[32];
        snprintf(aName, sizeof(aName), "minimal-%d.snm", size);
        const char* pPath = ZlibTest_Path(aName);
        ZlibTest_WriteRaw(pPath, zlibTest_aPayload, size);
        TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
        TEST_ASSERT_EQUAL_INT(0, zlibTest_pFile->isGz);
        memset(zlibTest_aOutput, 0xCC, 8);
        TEST_ASSERT_EQUAL_INT(size, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 4));
        if ( size )
        {
            TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, size);
        }
        TEST_ASSERT_EQUAL_HEX8(0xCC, zlibTest_aOutput[size]);
        TEST_ASSERT_EQUAL_INT(0, zfilio_seek(zlibTest_pFile, 0, SEEK_END));
        TEST_ASSERT_EQUAL_INT(size, zfilio_tell(zlibTest_pFile));
        ZlibTest_Close();
    }
}

TEST(SmushZlib, EmptyGzipHasZeroSizeAndEof)
{
    const char* pPath = ZlibTest_Path("empty.gz");
    ZlibTest_WriteGzip(pPath, zlibTest_aPayload, 0);
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(0, zlibTest_pFile->gzSize);
    TEST_ASSERT_EQUAL_INT(0, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 1));
    TEST_ASSERT_EQUAL_HEX8(0xCC, zlibTest_aOutput[0]);
    int error = -1;
    gzerror(zlibTest_pFile->hFile, &error);
    TEST_ASSERT_EQUAL_INT(Z_OK, error);
    ZlibTest_Close();
}

TEST(SmushZlib, MissingGzipFreesItsHandleAfterBothOpenPathsFail)
{
    const char* pPath = ZlibTest_Path("missing.gz");
    TEST_ASSERT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(1, zlibTest_numAllocs);
    TEST_ASSERT_EQUAL_INT(1, zlibTest_numFrees);
    TEST_ASSERT_EQUAL_INT(1, zlibTest_numFileOpens); // Real gzopen failed before the chained fallback.
    TEST_ASSERT_EQUAL_INT(0, zlibTest_numFileCloses);
}

TEST(SmushZlib, AllocationFailureDoesNotOpenTheExistingGzip)
{
    const char* pPath = ZlibTest_Path("allocation.gz");
    ZlibTest_WriteGzip(pPath, zlibTest_aPayload, 9);
    zlibTest_bFailAlloc = 1;
    TEST_ASSERT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(0, zlibTest_numAllocs);
    TEST_ASSERT_EQUAL_INT(0, zlibTest_numFileOpens);
    zlibTest_bFailAlloc = 0;
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(9, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 9));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, 9);
    ZlibTest_Close();
}

TEST(SmushZlib, UnregisteredAndWritableFilesKeepChainedFilioRouting)
{
    const char* pPath = ZlibTest_Path("chained.snm");
    ZlibTest_WriteRaw(pPath, zlibTest_aPayload, 17);
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(0, zlibTest_pFile->isGz);
    TEST_ASSERT_EQUAL_INT(4, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 4));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, 4);
    TEST_ASSERT_EQUAL_INT(0, zfilio_seek(zlibTest_pFile, -2, SEEK_END));
    TEST_ASSERT_EQUAL_INT(15, zfilio_tell(zlibTest_pFile));
    ZlibTest_Close();

    TEST_ASSERT_EQUAL_INT(0, zfilio_Configure(ZFILIO_TAG_EXTENSION, ".snm", 0));
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "wb"));
    TEST_ASSERT_EQUAL_INT(0, zlibTest_pFile->isGz);
    TEST_ASSERT_EQUAL_INT(17, zfilio_write(zlibTest_pFile, zlibTest_aPayload, 17));
    zfilio_flush(zlibTest_pFile);
    TEST_ASSERT_EQUAL_INT(1, zlibTest_numFileFlushes);
    ZlibTest_Close();
    TEST_ASSERT_EQUAL_INT(17, ZlibTest_ReadRaw(pPath));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aPacked, 17);
    TEST_ASSERT_EQUAL_INT(zlibTest_numFileOpens, zlibTest_numFileCloses);
}

TEST(SmushZlib, RealGzipWriteAndFinishFlushProduceReadableMembers)
{
    const char* pPath = ZlibTest_Path("written.gz");
    ZlibTest_OpenWriter(pPath);
    TEST_ASSERT_EQUAL_INT(0, zfilio_write(zlibTest_pFile, zlibTest_aPayload, 0));
    TEST_ASSERT_EQUAL_INT(13, zfilio_write(zlibTest_pFile, zlibTest_aPayload, 13));
    zfilio_flush(zlibTest_pFile); // Z_FINISH completes one member; the next write starts another.
    TEST_ASSERT_EQUAL_INT(24, zfilio_write(zlibTest_pFile, zlibTest_aPayload + 13, 24));
    zfilio_flush(zlibTest_pFile);
    TEST_ASSERT_EQUAL_INT(37, zfilio_tell(zlibTest_pFile));
    ZlibTest_Close();

    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(37, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 37));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, 37);
    TEST_ASSERT_EQUAL_INT(0, zfilio_read(zlibTest_pFile, zlibTest_aOutput + 37, 1));
    TEST_ASSERT_EQUAL_HEX8(0xCC, zlibTest_aOutput[37]);
    ZlibTest_Close();
}

TEST(SmushZlib, WritingAReadOnlyGzipReturnsTheBackendFailure)
{
    const char* pPath = ZlibTest_Path("read-only.gz");
    ZlibTest_WriteGzip(pPath, zlibTest_aPayload, 21);
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(0, zfilio_write(zlibTest_pFile, zlibTest_aPayload + 21, 7));
    TEST_ASSERT_EQUAL_INT(7, zfilio_tell(zlibTest_pFile)); // Preserve requested-byte bookkeeping on failure.
    ZlibTest_Close();
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(21, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 22));
    TEST_ASSERT_EQUAL_MEMORY(zlibTest_aPayload, zlibTest_aOutput, 21);
    TEST_ASSERT_EQUAL_HEX8(0xCC, zlibTest_aOutput[21]);
    ZlibTest_Close();
}

TEST(SmushZlib, CorruptCrcReturnsAllPriorDataThenReportsDataError)
{
    const char* pPath = ZlibTest_Path("corrupt.gz");
    ZlibTest_WriteGzip(pPath, zlibTest_aPayload, ZLIBTEST_PAYLOAD_SIZE);
    int size = ZlibTest_ReadRaw(pPath);
    TEST_ASSERT_GREATER_THAN_INT(18, size);
    zlibTest_aPacked[size - 8] ^= 1; // Corrupt only the gzip CRC, preserving the deflate stream and size.
    ZlibTest_WriteRaw(pPath, zlibTest_aPacked, size);
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(-1, ZlibTest_Drain());
    int error = Z_OK;
    gzerror(zlibTest_pFile->hFile, &error);
    TEST_ASSERT_EQUAL_INT(Z_DATA_ERROR, error);
    ZlibTest_Close();
}

TEST(SmushZlib, TruncatedTrailerReturnsPayloadAndReportsIncompleteStream)
{
    const char* pPath = ZlibTest_Path("truncated.gz");
    ZlibTest_WriteGzip(pPath, zlibTest_aPayload, ZLIBTEST_PAYLOAD_SIZE);
    int size = ZlibTest_ReadRaw(pPath);
    TEST_ASSERT_GREATER_THAN_INT(18, size);
    ZlibTest_WriteRaw(pPath, zlibTest_aPacked, size - 8);
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(0, ZlibTest_Drain());
    int error = Z_OK;
    gzerror(zlibTest_pFile->hFile, &error);
    TEST_ASSERT_EQUAL_INT(Z_BUF_ERROR, error);
    ZlibTest_Close();
}

TEST(SmushZlib, UnrecognizedGzipMethodFallsBackToChainedFilio)
{
    static const uint8_t aInput[] = { 0x1F, 0x8B, 0x7F, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    const char* pPath = ZlibTest_Path("unrecognized.gz");
    ZlibTest_WriteRaw(pPath, aInput, sizeof(aInput));
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    // The retail binding rejects transparent gzip input and uses ordinary FILIO.
    TEST_ASSERT_EQUAL_INT(0, zlibTest_pFile->isGz);
    TEST_ASSERT_EQUAL_INT(sizeof(aInput), zfilio_read(zlibTest_pFile, zlibTest_aOutput, 16));
    TEST_ASSERT_EQUAL_MEMORY(aInput, zlibTest_aOutput, sizeof(aInput));
    TEST_ASSERT_EQUAL_HEX8(0xCC, zlibTest_aOutput[sizeof(aInput)]);
    TEST_ASSERT_EQUAL_INT(sizeof(aInput), zfilio_tell(zlibTest_pFile));
    ZlibTest_Close();
}

TEST(SmushZlib, InvalidDeflateBlockReturnsDataErrorWithoutWritingOutput)
{
    // A recognized gzip header followed by reserved deflate block type 3.
    static const uint8_t aInvalid[] = {
        0x1F, 0x8B, 8, 0, 0, 0, 0, 0, 0, 0,
        0x07, 0, 0, 0, 0, 0, 0, 0, 0
    };
    const char* pPath = ZlibTest_Path("invalid-deflate.gz");
    ZlibTest_WriteRaw(pPath, aInvalid, sizeof(aInvalid));
    TEST_ASSERT_NOT_NULL(ZlibTest_Open(pPath, "rb"));
    TEST_ASSERT_EQUAL_INT(-1, zfilio_read(zlibTest_pFile, zlibTest_aOutput, 16));
    TEST_ASSERT_EQUAL_HEX8(0xCC, zlibTest_aOutput[0]);
    TEST_ASSERT_EQUAL_INT(16, zfilio_tell(zlibTest_pFile));
    int error = Z_OK;
    gzerror(zlibTest_pFile->hFile, &error);
    TEST_ASSERT_EQUAL_INT(Z_DATA_ERROR, error);
    ZlibTest_Close();
}

TEST(SmushZlib, PlainSanmSizeAndFirstHeaderReachTheRealStreamer)
{
    // The shipped intros are plain SANM files ending in zeroes, not gzip members.
    // A transparent gz handle misreads these last four bytes as an uncompressed
    // length of zero; streamer_Fill then never reads a header or produces a frame.
    uint8_t aSanm[0x1800] = { 0 };
    memcpy(aSanm, "SANM\0\0\x17\xF8", 8);
    const char* pPath = ZlibTest_Path("streamer.snm");
    ZlibTest_WriteRaw(pPath, aSanm, sizeof(aSanm));
    TEST_ASSERT_EQUAL_INT(0, zfilio_Configure(ZFILIO_TAG_EXTENSION, ".snm", 0));
    TEST_ASSERT_NOT_NULL(zlibTest_pZfilio);
    TEST_ASSERT_EQUAL_INT(0, stdfile_Register(&zlibTest_share, &zlibTest_chainZfilio));
    TEST_ASSERT_EQUAL_INT(0, stdfile_Startup());
    zlibTest_bStdfileStarted = 1;
    TEST_ASSERT_NOT_NULL(zlibTest_pStdfile);
    TEST_ASSERT_EQUAL_INT(0, streamer_Register(&zlibTest_share, NULL));
    TEST_ASSERT_EQUAL_INT(0, streamer_Startup());
    zlibTest_bStreamerStarted = 1;
    TEST_ASSERT_EQUAL_INT(0, streamer_Create(&zlibTest_pStream, 0x2000, 0x1000));
    streamer_Reset(zlibTest_pStream); // animplay_Create also resets the new stream before opening.
    TEST_ASSERT_EQUAL_INT(0, streamer_SetFile(zlibTest_pStream, pPath, 0));
    TEST_ASSERT_EQUAL_INT(0, streamer_Prime(zlibTest_pStream));
    TEST_ASSERT_EQUAL_INT(sizeof(aSanm), zlibTest_pStream->fileSize);
    TEST_ASSERT_EQUAL_INT(0, zlibTest_pStream->filePos);
    TEST_ASSERT_EQUAL_INT(0, zlibTest_pStream->ready);

    streamer_Fill(zlibTest_pStream, 0x1000);
    TEST_ASSERT_EQUAL_INT(0x1000, zlibTest_pStream->filePos);
    TEST_ASSERT_EQUAL_INT(1, zlibTest_pStream->ready);
    TEST_ASSERT_EQUAL_INT(0, streamer_Advance(zlibTest_pStream, 8)); // Finish activation before consuming.
    int header = streamer_Advance(zlibTest_pStream, 8);
    TEST_ASSERT_EQUAL_PTR(zlibTest_pStream->pBuf, (void*)(intptr_t)header);
    TEST_ASSERT_EQUAL_MEMORY(aSanm, (void*)(intptr_t)header, 8);
    streamer_Destroy(&zlibTest_pStream);
    TEST_ASSERT_NULL(zlibTest_pStream);
    TEST_ASSERT_EQUAL_INT(zlibTest_numAllocs, zlibTest_numFrees);
}

TEST_GROUP_RUNNER(SmushZlib)
{
    RUN_TEST_CASE(SmushZlib, RegisteredPlainSnmFallsBackToChainedFilio);
    RUN_TEST_CASE(SmushZlib, GzipAndZnmDecodeCompletePayloadAcrossBuffers);
    RUN_TEST_CASE(SmushZlib, LazySeeksUseUncompressedOffsetsAndReopenAtStart);
    RUN_TEST_CASE(SmushZlib, ShortReadAndEofKeepLegacyRequestedPositionThenRewind);
    RUN_TEST_CASE(SmushZlib, EmptyAndMinimalPlainInputsPassThrough);
    RUN_TEST_CASE(SmushZlib, EmptyGzipHasZeroSizeAndEof);
    RUN_TEST_CASE(SmushZlib, MissingGzipFreesItsHandleAfterBothOpenPathsFail);
    RUN_TEST_CASE(SmushZlib, AllocationFailureDoesNotOpenTheExistingGzip);
    RUN_TEST_CASE(SmushZlib, UnregisteredAndWritableFilesKeepChainedFilioRouting);
    RUN_TEST_CASE(SmushZlib, RealGzipWriteAndFinishFlushProduceReadableMembers);
    RUN_TEST_CASE(SmushZlib, WritingAReadOnlyGzipReturnsTheBackendFailure);
    RUN_TEST_CASE(SmushZlib, CorruptCrcReturnsAllPriorDataThenReportsDataError);
    RUN_TEST_CASE(SmushZlib, TruncatedTrailerReturnsPayloadAndReportsIncompleteStream);
    RUN_TEST_CASE(SmushZlib, UnrecognizedGzipMethodFallsBackToChainedFilio);
    RUN_TEST_CASE(SmushZlib, InvalidDeflateBlockReturnsDataErrorWithoutWritingOutput);
    RUN_TEST_CASE(SmushZlib, PlainSanmSizeAndFirstHeaderReachTheRealStreamer);
}