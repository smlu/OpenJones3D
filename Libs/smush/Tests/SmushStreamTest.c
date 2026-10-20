#include <unity_fixture.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <smush/insane/Animate/Frame.h>
#include <smush/insane/Animate/Pcplay.h>
#include <smush/insane/File/Cdstream.h>
#include <smush/insane/File/Framestream.h>
#include <smush/insane/File/Stdfile.h>
#include <smush/insane/File/Stdio.h>
#include <smush/insane/File/Zfilio/Zfilio.h>
#include <smush/insane/Image/Readauto.h>
#include <smush/insane/Sound/Engine.h>
#include <smush/insane/Timer/Timer.h>

// Unit tests of the smush Stream area (Cdstream, Framestream, Stdfile, Stdio, Zfilio, Readauto, Frame, Pcplay).
// The INSANE services are mocked: a share table that publishes/subscribes by name, an in-memory FILIO file system and malloc
// as the allocator. The executable's gz functions are replaced through the Zfilio test seam.

// ---------------------------------------------------------------------------------------------------------------------
// Mock share table
// ---------------------------------------------------------------------------------------------------------------------
#define MOCK_MAXENTRIES 16

static struct
{
    char name[48];
    void* pInterface;
} Test_published[MOCK_MAXENTRIES];
static size_t Test_numPublished;

// Subscribe("<service>.<anything>") answers from Test_subscriptions
static struct
{
    char name[48];
    void* pInterface;
} Test_subscriptions[MOCK_MAXENTRIES];
static size_t Test_numSubscriptions;

static char Test_lastError[128];
static int Test_numErrors;
static int Test_numUnsubscribed;

static int Test_Publish(const char* pName, void* pInterface)
{
    strncpy_s(Test_published[Test_numPublished].name, sizeof(Test_published[0].name), pName, _TRUNCATE);
    Test_published[Test_numPublished++].pInterface = pInterface;
    return 0;
}

static void Test_Unpublish(const char* pName)
{
    for ( size_t i = 0; i < Test_numPublished; ++i )
    {
        if ( strcmp(Test_published[i].name, pName) == 0 )
        {
            Test_published[i] = Test_published[--Test_numPublished];
            return;
        }
    }
}

static void* Test_Subscribe(const char* pName, void* pParam)
{
    if ( strcmp(pName, "READANIM.chainto") == 0 || strcmp(pName, "FLOBDEC.temp") == 0 )
    {
        return pParam;
    }

    for ( size_t i = 0; i < Test_numSubscriptions; ++i )
    {
        if ( strcmp(Test_subscriptions[i].name, pName) == 0 )
        {
            return Test_subscriptions[i].pInterface;
        }
    }

    return NULL;
}

static void Test_Unsubscribe(void* pInterface)
{
    (void)pInterface;
    ++Test_numUnsubscribed;
}

static void* Test_Enumerate(const char* pName, int index)
{
    size_t len = strlen(pName);
    int n      = 0;
    for ( size_t i = 0; i < Test_numPublished; ++i )
    {
        if ( strncmp(Test_published[i].name, pName, len) == 0 && Test_published[i].name[len] == '=' )
        {
            if ( n++ == index )
            {
                return Test_published[i].pInterface;
            }
        }
    }

    return NULL;
}

static int Test_Count(const char* pName)
{
    size_t len = strlen(pName);
    int n      = 0;
    for ( size_t i = 0; i < Test_numPublished; ++i )
    {
        if ( strncmp(Test_published[i].name, pName, len) == 0 && Test_published[i].name[len] == '=' )
        {
            ++n;
        }
    }

    return n;
}

static void Test_ReportError(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    vsnprintf_s(Test_lastError, sizeof(Test_lastError), _TRUNCATE, pFormat, args);
    va_end(args);
    ++Test_numErrors;
}

static LecShare Test_share = { Test_Publish, Test_Unpublish, Test_Subscribe, Test_Unsubscribe, NULL, NULL, Test_Enumerate, Test_Count, Test_ReportError };

static void Test_Subscription(const char* pName, void* pInterface)
{
    strncpy_s(Test_subscriptions[Test_numSubscriptions].name, sizeof(Test_subscriptions[0].name), pName, _TRUNCATE);
    Test_subscriptions[Test_numSubscriptions++].pInterface = pInterface;
}

// ---------------------------------------------------------------------------------------------------------------------
// Mock allocator
// ---------------------------------------------------------------------------------------------------------------------
static int Test_numAllocs;
static int Test_numFrees;

static void* Test_Alloc(int size, const char* pFile, int line)
{
    (void)pFile;
    (void)line;
    ++Test_numAllocs;
    return malloc(size);
}

static void Test_Free(void* pBlock, const char* pFile, int line)
{
    (void)pFile;
    (void)line;
    ++Test_numFrees;
    free(pBlock);
}

static MemoryDebugInterface Test_memory = { Test_Alloc, Test_Free, NULL };

// ---------------------------------------------------------------------------------------------------------------------
// Mock FILIO: a handful of in-memory files; handle = index + 1
// ---------------------------------------------------------------------------------------------------------------------
#define MOCK_MAXFILES 4

typedef struct sMockFile
{
    char name[32];
    uint8_t data[0x4000];
    int size;
    int pos;
    int isOpen;
} MockFile;

static MockFile Test_files[MOCK_MAXFILES];
static int Test_numFileOpens;
static int Test_numFileCloses;
static int Test_numFileFlushes;

static MockFile* Test_AddFile(const char* pName, const void* pData, int size)
{
    for ( int i = 0; i < MOCK_MAXFILES; ++i )
    {
        if ( !Test_files[i].name[0] )
        {
            strncpy_s(Test_files[i].name, sizeof(Test_files[i].name), pName, _TRUNCATE);
            if ( pData ) { memcpy(Test_files[i].data, pData, size); }
            Test_files[i].size = size;
            return &Test_files[i];
        }
    }

    return NULL;
}

static int Test_FilioOpen(const char* pPath, const char* pMode)
{
    (void)pMode;
    ++Test_numFileOpens;
    for ( int i = 0; i < MOCK_MAXFILES; ++i )
    {
        if ( strcmp(Test_files[i].name, pPath) == 0 )
        {
            Test_files[i].pos    = 0;
            Test_files[i].isOpen = 1;
            return i + 1;
        }
    }

    return 0;
}

static void Test_FilioClose(int fh)
{
    ++Test_numFileCloses;
    Test_files[fh - 1].isOpen = 0;
}

static int Test_FilioRead(int fh, void* pBuf, int size)
{
    MockFile* pFile = &Test_files[fh - 1];
    int n           = size;
    if ( pFile->pos + n > pFile->size ) { n = pFile->size - pFile->pos; }
    if ( n < 0 ) { n = 0; }
    memcpy(pBuf, pFile->data + pFile->pos, n);
    pFile->pos += n;
    return n;
}

static int Test_FilioWrite(int fh, const void* pBuf, int size)
{
    MockFile* pFile = &Test_files[fh - 1];
    memcpy(pFile->data + pFile->pos, pBuf, size);
    pFile->pos += size;
    if ( pFile->pos > pFile->size ) { pFile->size = pFile->pos; }
    return size;
}

static int Test_FilioTell(int fh)
{
    return Test_files[fh - 1].pos;
}

static int Test_FilioSeek(int fh, int offset, int whence)
{
    MockFile* pFile = &Test_files[fh - 1];
    int base        = whence == 0 ? 0 : whence == 1 ? pFile->pos : pFile->size;
    pFile->pos      = base + offset;
    return 0;
}

static void Test_FilioFlush(int fh)
{
    (void)fh;
    ++Test_numFileFlushes;
}

static FilioInterface Test_filio = { Test_FilioOpen, Test_FilioClose, Test_FilioRead, Test_FilioWrite, Test_FilioTell, Test_FilioSeek, Test_FilioFlush };

static void SmushStreamTest_Reset(void)
{
    Cdstream_ResetGlobals();
    Framestream_ResetGlobals();
    Stdfile_ResetGlobals();
    Stdio_ResetGlobals();
    Zfilio_ResetGlobals();
    Readauto_ResetGlobals();
    Frame_ResetGlobals();
    Pcplay_ResetGlobals();
    Zfilio_TestSetGz(NULL);

    memset(Test_published, 0, sizeof(Test_published));
    memset(Test_subscriptions, 0, sizeof(Test_subscriptions));
    memset(Test_files, 0, sizeof(Test_files));
    Test_numPublished = Test_numSubscriptions = 0;
    Test_lastError[0]                         = 0;
    Test_numErrors = Test_numUnsubscribed = 0;
    Test_numAllocs = Test_numFrees = 0;
    Test_numFileOpens = Test_numFileCloses = Test_numFileFlushes = 0;
}

TEST_GROUP(SmushStream);

TEST_SETUP(SmushStream)
{
    SmushStreamTest_Reset();
}

TEST_TEAR_DOWN(SmushStream)
{
    SmushStreamTest_Reset();
}

// ---------------------------------------------------------------------------------------------------------------------
// Stdfile
// ---------------------------------------------------------------------------------------------------------------------
static StdFileInterface* SmushStreamTest_StartStdfile(void)
{
    Test_Subscription("FILIO.chainto", &Test_filio);
    TEST_ASSERT_EQUAL_INT(0, stdfile_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdfile_Startup());
    return Test_Enumerate("FILE", 0);
}

TEST(SmushStream, stdfile_RegistersAndRequiresChainedFilio)
{
    TEST_ASSERT_EQUAL_INT(0, stdfile_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(1, Test_Count("LECMODULE"));
    TEST_ASSERT_EQUAL_INT(1, Test_Count("FILE"));

    // No FILIO.chainto subscription: Startup fails with the module's message
    TEST_ASSERT_EQUAL_INT(-1, stdfile_Startup());
    TEST_ASSERT_EQUAL_STRING("StdFile Module: Cannot allocate filio services.", Test_lastError);

    stdfile_Unregister();
    TEST_ASSERT_EQUAL_INT(0, Test_numPublished);
}

TEST(SmushStream, stdfile_ForwardsToChainedFilio)
{
    uint8_t data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    Test_AddFile("a.bin", data, sizeof(data));
    StdFileInterface* pFile = SmushStreamTest_StartStdfile();
    TEST_ASSERT_NOT_NULL(pFile);

    int fh = pFile->Open("a.bin", "rb");
    TEST_ASSERT_EQUAL_INT(1, fh);
    uint8_t buf[4] = { 0 };
    TEST_ASSERT_EQUAL_INT(4, pFile->Read(fh, buf, 4));
    TEST_ASSERT_EQUAL_UINT8(4, buf[3]);
    TEST_ASSERT_EQUAL_INT(4, pFile->Tell(fh));
    TEST_ASSERT_EQUAL_INT(0, pFile->Seek(fh, 1, 0));
    TEST_ASSERT_EQUAL_INT(1, pFile->Tell(fh));
    pFile->Flush(fh);
    TEST_ASSERT_EQUAL_INT(1, Test_numFileFlushes);
    pFile->Close(fh);
    TEST_ASSERT_EQUAL_INT(1, Test_numFileCloses);
    stdfile_Shutdown();
    TEST_ASSERT_EQUAL_INT(1, Test_numUnsubscribed);
}

TEST(SmushStream, stdfile_SizeKeepsPositionAndEofComparesToSize)
{
    uint8_t data[6] = { 'a', 'b', 'c', 'd', 'e', 'f' };
    Test_AddFile("a.bin", data, sizeof(data));
    SmushStreamTest_StartStdfile();

    int fh = stdfile_open("a.bin", "rb");
    stdfile_seek(fh, 2, 0);
    TEST_ASSERT_EQUAL_INT(6, stdfile_size(fh));
    TEST_ASSERT_EQUAL_INT(2, stdfile_tell(fh));
    TEST_ASSERT_FALSE(stdfile_eof(fh));
    stdfile_seek(fh, 0, 2);
    TEST_ASSERT_TRUE(stdfile_eof(fh));
}

TEST(SmushStream, stdfile_GetcPutc)
{
    uint8_t data[2] = { 0x41, 0xFF };
    Test_AddFile("a.bin", data, sizeof(data));
    SmushStreamTest_StartStdfile();

    int fh = stdfile_open("a.bin", "rb");
    TEST_ASSERT_EQUAL_INT(0x41, stdfile_getc(fh));
    TEST_ASSERT_EQUAL_INT(0xFF, stdfile_getc(fh));
    TEST_ASSERT_EQUAL_INT(-1, stdfile_getc(fh));

    stdfile_seek(fh, 0, 0);
    TEST_ASSERT_EQUAL_INT(0x7A, stdfile_putc(fh, 0x17A)); // returns the low byte
    stdfile_seek(fh, 0, 0);
    TEST_ASSERT_EQUAL_INT(0x7A, stdfile_getc(fh));
}

TEST(SmushStream, stdfile_GetsStopsAtNewlineAndEof)
{
    const char text[] = "ab\ncd";
    Test_AddFile("t.txt", text, 5);
    SmushStreamTest_StartStdfile();
    int fh = stdfile_open("t.txt", "rb");

    char line[16];
    memset(line, 'x', sizeof(line));
    TEST_ASSERT_EQUAL_PTR(line, stdfile_gets(fh, line, sizeof(line)));
    TEST_ASSERT_EQUAL_STRING("ab\n", line);

    // The last line ends at the end of file: terminated at the failed read
    TEST_ASSERT_EQUAL_PTR(line, stdfile_gets(fh, line, sizeof(line)));
    TEST_ASSERT_EQUAL_STRING("cd", line);

    // Nothing left: NULL
    TEST_ASSERT_NULL(stdfile_gets(fh, line, sizeof(line)));
}

TEST(SmushStream, stdfile_PrintfWritesFormattedText)
{
    Test_AddFile("o.txt", NULL, 0);
    SmushStreamTest_StartStdfile();
    int fh = stdfile_open("o.txt", "wb");

    TEST_ASSERT_EQUAL_INT(8, stdfile_printf(fh, "n=%d s=%s", 42, "x"));
    TEST_ASSERT_EQUAL_INT(8, stdfile_tell(fh));
    TEST_ASSERT_EQUAL_MEMORY("n=42 s=x", Test_files[0].data, 8);
}

// ---------------------------------------------------------------------------------------------------------------------
// Stdio (wrappers of the std file functions)
// ---------------------------------------------------------------------------------------------------------------------
TEST(SmushStream, stdio_RegistersFilioModule)
{
    TEST_ASSERT_EQUAL_INT(0, stdio_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(1, Test_Count("FILIO"));
    TEST_ASSERT_EQUAL_INT(0, stdio_Startup());
    stdio_Shutdown();
    stdio_Unregister();
    TEST_ASSERT_EQUAL_INT(0, Test_numPublished);
}

TEST(SmushStream, stdio_WritesReadsSeeksAndTellsRealFile)
{
    const char* pPath = "smush_stream_stdio_test.tmp";
    int fh            = stdio_open(pPath, "wb");
    TEST_ASSERT_NOT_EQUAL(0, fh);
    TEST_ASSERT_EQUAL_INT(5, stdio_write(fh, "hello", 5));
    TEST_ASSERT_EQUAL_INT(5, stdio_tell(fh));
    stdio_flush(fh);
    stdio_close(fh);

    fh = stdio_open(pPath, "rb");
    TEST_ASSERT_NOT_EQUAL(0, fh);
    TEST_ASSERT_EQUAL_INT(0, stdio_seek(fh, 1, 0));
    char buf[8] = { 0 };
    TEST_ASSERT_EQUAL_INT(4, stdio_read(fh, buf, 8));
    TEST_ASSERT_EQUAL_STRING("ello", buf);
    stdio_close(fh);
    remove(pPath);

    TEST_ASSERT_EQUAL_INT(0, stdio_open(pPath, "rb")); // missing file
}

// ---------------------------------------------------------------------------------------------------------------------
// Cdstream
// ---------------------------------------------------------------------------------------------------------------------
static StreamerInterface* SmushStreamTest_StartStreamer(void)
{
    Test_Subscription("MEMORY.workspace", &Test_memory);
    Test_Subscription("FILIO.io", &Test_filio);
    TEST_ASSERT_EQUAL_INT(0, streamer_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(0, streamer_Startup());
    return Test_Enumerate("STREAMER", 0);
}

TEST(SmushStream, streamer_StartupRequiresServices)
{
    TEST_ASSERT_EQUAL_INT(0, streamer_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(-1, streamer_Startup());
    TEST_ASSERT_EQUAL_STRING("Streamer Module: Cannot allocate memory services.", Test_lastError);

    Test_Subscription("MEMORY.workspace", &Test_memory);
    TEST_ASSERT_EQUAL_INT(-1, streamer_Startup());
    TEST_ASSERT_EQUAL_STRING("Streamer Module: Cannot allocate filio services.", Test_lastError);
}

TEST(SmushStream, streamer_CreateRoundsRingSizeAndDestroyFrees)
{
    StreamerInterface* pStreamer = SmushStreamTest_StartStreamer();
    Stream* pStream              = NULL;
    TEST_ASSERT_EQUAL_INT(0, pStreamer->Create(&pStream, 0x2800 + 0x123, 0x1000));
    TEST_ASSERT_NOT_NULL(pStream);
    TEST_ASSERT_EQUAL_INT(0x2800, pStream->ringSize);
    TEST_ASSERT_EQUAL_INT(0x1000, pStream->maxFrame);
    TEST_ASSERT_EQUAL_PTR(pStream->pBuf + 0x2800, pStream->pEnd);
    TEST_ASSERT_EQUAL_INT(0, pStream->hFile);

    pStreamer->Destroy(&pStream);
    TEST_ASSERT_NULL(pStream);
    TEST_ASSERT_EQUAL_INT(2, Test_numFrees);
    streamer_Shutdown();
}

TEST(SmushStream, streamer_SetFileFailsForMissingFile)
{
    StreamerInterface* pStreamer = SmushStreamTest_StartStreamer();
    Stream* pStream              = NULL;
    pStreamer->Create(&pStream, 0x2000, 0x1000);
    TEST_ASSERT_EQUAL_INT(-1, pStreamer->SetFile(pStream, "missing.snm", 8));
    TEST_ASSERT_EQUAL_INT(0, pStream->reopen);
    pStreamer->Destroy(&pStream);
}

TEST(SmushStream, streamer_FillAdvanceAndPercent)
{
    uint8_t data[0x3000];
    for ( int i = 0; i < (int)sizeof(data); ++i ) { data[i] = (uint8_t)(i * 7); }
    Test_AddFile("a.snm", data, sizeof(data));

    StreamerInterface* pStreamer = SmushStreamTest_StartStreamer();
    Stream* pStream              = NULL;
    pStreamer->Create(&pStream, 0x2000, 0x1000);
    pStreamer->Reset(pStream);
    TEST_ASSERT_EQUAL_INT(0, pStreamer->SetFile(pStream, "a.snm", 8));
    TEST_ASSERT_EQUAL_INT(0, Test_files[0].isOpen); // SetFile only probes the file

    // No file open yet and nothing filled
    TEST_ASSERT_EQUAL_INT(-1, pStreamer->Advance(pStream, 16));
    TEST_ASSERT_FALSE(pStreamer->IsActive(pStream));

    // The first Fill primes the stream (opens the file, starts at the sector below the offset) and reads one frame
    pStreamer->Fill(pStream, 0x800);
    TEST_ASSERT_EQUAL_INT(1, Test_files[0].isOpen);
    TEST_ASSERT_EQUAL_INT(0x3000, pStream->fileSize);
    TEST_ASSERT_EQUAL_INT(0x1000, pStream->filePos);
    TEST_ASSERT_EQUAL_INT(8, pStream->field20);
    TEST_ASSERT_TRUE(pStreamer->IsActive(pStream));
    TEST_ASSERT_EQUAL_MEMORY(data, pStream->pBuf, 0x1000);

    // Advancing while the stream is still priming cancels the priming and returns 0
    TEST_ASSERT_EQUAL_INT(0, pStreamer->Advance(pStream, 16));
    TEST_ASSERT_FALSE(pStreamer->IsActive(pStream));

    // The next advance returns the frame address, skipping the sector offset
    int pFrame = pStreamer->Advance(pStream, 16);
    TEST_ASSERT_EQUAL_PTR(pStream->pBuf + 8, (void*)(intptr_t)pFrame);
    TEST_ASSERT_EQUAL_INT(0, pStream->field20);
    TEST_ASSERT_EQUAL_INT(8 + 16, pStream->field2C);

    // A frame larger than the maximum is rejected with a message
    TEST_ASSERT_EQUAL_INT(-2, pStreamer->Advance(pStream, 0x1001));
    TEST_ASSERT_EQUAL_STRING("File Streamer:  Invalid frame size 4097", Test_lastError);

    // Percent: the read pointer is 0x1000 ahead of the write pointer in a 0x2000 ring
    TEST_ASSERT_EQUAL_INT(50, pStreamer->Percent(pStream));

    pStreamer->Reset(pStream);
    TEST_ASSERT_EQUAL_INT(0, Test_files[0].isOpen);
    TEST_ASSERT_EQUAL_INT(100, pStreamer->Percent(pStream)); // no file
    pStreamer->Destroy(&pStream);
}

TEST(SmushStream, streamer_PrimeRetriesThroughCallback)
{
    uint8_t data[0x1000] = { 0 };
    StreamerInterface* pStreamer = SmushStreamTest_StartStreamer();
    Stream* pStream              = NULL;
    pStreamer->Create(&pStream, 0x2000, 0x1000);
    pStreamer->Reset(pStream);
    Test_AddFile("a.snm", data, sizeof(data));
    pStreamer->SetFile(pStream, "a.snm", 0);

    // The file vanishes (disc removed): no retry callback gives up with -2 and clears the request
    Test_files[0].name[0] = 0;
    TEST_ASSERT_EQUAL_INT(-2, streamer_Prime(pStream));
    TEST_ASSERT_EQUAL_INT(0, pStream->reopen);
    TEST_ASSERT_EQUAL_INT(-1, streamer_Prime(pStream)); // nothing to do any more
    pStreamer->Destroy(&pStream);
}

TEST(SmushStream, streamer_InRangeAndCancel)
{
    StreamerInterface* pStreamer = SmushStreamTest_StartStreamer();
    Stream* pStream              = NULL;
    pStreamer->Create(&pStream, 0x2000, 0x1000);
    pStreamer->Reset(pStream);

    pStream->pRead  = pStream->pBuf + 0x100;
    pStream->pWrite = pStream->pBuf;
    TEST_ASSERT_TRUE(streamer_InRange(pStream, 0x100));
    TEST_ASSERT_FALSE(streamer_InRange(pStream, 0x101));

    // Write pointer ahead of the read pointer: the read pointer is a ring size ahead
    pStream->pRead  = pStream->pBuf;
    pStream->pWrite = pStream->pBuf + 0x1000;
    TEST_ASSERT_TRUE(streamer_InRange(pStream, 0x1000));
    TEST_ASSERT_FALSE(streamer_InRange(pStream, 0x1001));

    TEST_ASSERT_EQUAL_INT(-1, streamer_Cancel(pStream));
    pStream->active = 1;
    pStream->pStart = pStream->pBuf + 0x200;
    TEST_ASSERT_EQUAL_INT(0, streamer_Cancel(pStream));
    TEST_ASSERT_EQUAL_PTR(pStream->pBuf + 0x200, pStream->pWrite);
    TEST_ASSERT_EQUAL_INT(0, pStream->active);
    pStreamer->Destroy(&pStream);
}

// ---------------------------------------------------------------------------------------------------------------------
// Framestream: the chunk state machine over mock streamer/frame services
// ---------------------------------------------------------------------------------------------------------------------
static int Test_advanceQueue[8];
static int Test_numAdvance;
static int Test_advanceIndex;
static int Test_lastAdvanceArg[8];
static int Test_active;
static int Test_numParse;
static int Test_lastParseSize;
static int Test_numDecode;
static uint32_t Test_decodeResult;
static uint32_t Test_lastDecodeFlags;
static int Test_lastDecodeSize;
static int Test_numFrameReset;
static int Test_streamerResets;
static int Test_setFileResult;
static int Test_lastSetFileOffset;
static int Test_lastRate;
static int Test_numRewind;
static void* Test_fetchResult;

static int Test_StreamerAdvance(Stream* pStream, int n)
{
    (void)pStream;
    Test_lastAdvanceArg[Test_advanceIndex & 7] = n;
    return Test_advanceQueue[Test_advanceIndex++ % 8];
}

static int Test_StreamerIsActive(Stream* pStream) { (void)pStream; return Test_active; }
static void Test_StreamerReset(Stream* pStream) { (void)pStream; ++Test_streamerResets; }
static int Test_StreamerSetFile(Stream* pStream, const char* pName, int offset)
{
    (void)pStream;
    (void)pName;
    Test_lastSetFileOffset = offset;
    return Test_setFileResult;
}

static StreamerInterface Test_streamer = { NULL, NULL, Test_StreamerSetFile, Test_StreamerIsActive, NULL, Test_StreamerAdvance, Test_StreamerReset, NULL };

static int Test_FrameParse(const uint8_t* pData, int size) { (void)pData; ++Test_numParse; Test_lastParseSize = size; return 0; }
static uint32_t Test_FrameDecode(void* pBitmap, int a2, int a3, int a4, void* pPalette, int* pOutA, int* pOutB, uint32_t flags, int a9, int a10, const uint8_t* pData, int size)
{
    (void)pBitmap; (void)a2; (void)a3; (void)a4; (void)pPalette; (void)pOutA; (void)pOutB; (void)a9; (void)a10; (void)pData;
    ++Test_numDecode;
    Test_lastDecodeFlags = flags;
    Test_lastDecodeSize  = size;
    return Test_decodeResult;
}
static void Test_FrameReset(void) { ++Test_numFrameReset; }
static void* Test_FrameFetch(int n) { (void)n; return Test_fetchResult; }

static AnimFrameTable Test_frame = { Test_FrameReset, Test_FrameParse, Test_FrameDecode, NULL, NULL, Test_FrameFetch };

static void Test_OpsSetRate(AnimPlay* pPlay, int rate) { (void)pPlay; Test_lastRate = rate; }
static void Test_OpsRewind(AnimPlay* pPlay) { (void)pPlay; ++Test_numRewind; }

static AnimPlayOps Test_savedOps;

static void SmushStreamTest_ResetFramestreamMocks(void)
{
    memset(Test_advanceQueue, 0, sizeof(Test_advanceQueue));
    memset(Test_lastAdvanceArg, 0, sizeof(Test_lastAdvanceArg));
    Test_numAdvance = Test_advanceIndex = Test_active = Test_numParse = Test_lastParseSize = 0;
    Test_numDecode = Test_lastDecodeSize = Test_numFrameReset = Test_streamerResets = Test_setFileResult = 0;
    Test_lastSetFileOffset = Test_lastRate = Test_numRewind = 0;
    Test_decodeResult = Test_lastDecodeFlags = 0;
    Test_fetchResult = NULL;
}

static void SmushStreamTest_StartFramestream(AnimPlay* pPlay)
{
    SmushStreamTest_ResetFramestreamMocks();
    memset(pPlay, 0, sizeof(*pPlay));
    animplay_pShare    = &Test_share;
    animplay_pStreamer = &Test_streamer;
    animplay_pFrame    = &Test_frame;
    Test_savedOps      = animplay_ops;
    animplay_ops.SetRate = Test_OpsSetRate;
    animplay_ops.Rewind  = Test_OpsRewind;
    framestream_Reset(pPlay);
}

static void SmushStreamTest_StopFramestream(void)
{
    animplay_ops = Test_savedOps;
}

// A chunk header: FourCC and big-endian size
static void SmushStreamTest_ChunkHeader(uint8_t* pOut, const char* pTag, uint32_t size)
{
    memcpy(pOut, pTag, 4);
    pOut[4] = (uint8_t)(size >> 24);
    pOut[5] = (uint8_t)(size >> 16);
    pOut[6] = (uint8_t)(size >> 8);
    pOut[7] = (uint8_t)size;
}

TEST(SmushStream, framestream_ResetClearsState)
{
    AnimPlay play;
    SmushStreamTest_StartFramestream(&play);
    play.frame = 5;
    play.paused = 1;
    play.hold = 1;
    play.mode = 3;
    TEST_ASSERT_EQUAL_INT(0, framestream_Reset(&play));
    TEST_ASSERT_EQUAL_UINT32(FRAMESTREAM_TAG_NULL, play.tag);
    TEST_ASSERT_EQUAL_INT(1, play.frameCount);
    TEST_ASSERT_EQUAL_INT(0, play.frame);
    TEST_ASSERT_EQUAL_INT(0, play.paused);
    TEST_ASSERT_EQUAL_INT(0, play.hold);
    TEST_ASSERT_EQUAL_INT(0, play.mode);
    TEST_ASSERT_EQUAL_INT(2, Test_streamerResets);
    SmushStreamTest_StopFramestream();
}

TEST(SmushStream, framestream_ModeOperations)
{
    AnimPlay play;
    SmushStreamTest_StartFramestream(&play);

    TEST_ASSERT_EQUAL_INT(1, framestream_Mode(&play, 2)); // toggle on
    TEST_ASSERT_EQUAL_INT(0, framestream_Mode(&play, 2)); // toggle off
    TEST_ASSERT_EQUAL_INT(0, framestream_Mode(&play, 4)); // 4 only raises a mode that is set
    framestream_Mode(&play, 1);
    TEST_ASSERT_EQUAL_INT(3, framestream_Mode(&play, 4));
    TEST_ASSERT_EQUAL_INT(3, framestream_Mode(&play, 5)); // 5 queries
    TEST_ASSERT_EQUAL_INT(7, framestream_Mode(&play, 7));
    SmushStreamTest_StopFramestream();
}

TEST(SmushStream, framestream_OpenSeekAndStall)
{
    AnimPlay play;
    SmushStreamTest_StartFramestream(&play);

    Test_setFileResult = -1;
    TEST_ASSERT_EQUAL_INT(-1, framestream_Open(&play, "a.snm", 0x10, 3, 4, 1));
    TEST_ASSERT_EQUAL_INT(0, framestream_IsOpen(&play));
    TEST_ASSERT_EQUAL_INT(1, play.hold);
    TEST_ASSERT_EQUAL_INT(0x10, play.flagsIn);

    Test_setFileResult = 0;
    play.frame         = 9;
    TEST_ASSERT_EQUAL_INT(0, framestream_Open(&play, "a.snm", 0, 3, 4, 0));
    TEST_ASSERT_EQUAL_INT(8, Test_lastSetFileOffset);
    TEST_ASSERT_EQUAL_INT(1, framestream_IsOpen(&play));
    TEST_ASSERT_EQUAL_INT(9, play.seekBase);
    TEST_ASSERT_EQUAL_INT(4, play.seekTo);
    TEST_ASSERT_EQUAL_INT(3, play.seekLen);

    framestream_Stall(&play);
    TEST_ASSERT_EQUAL_INT(1, play.stalled);
    TEST_ASSERT_EQUAL_INT(1, Test_numFrameReset);
    // A stalled stream reports -1 once and clears the flag
    TEST_ASSERT_EQUAL_INT(-1, framestream_Tick(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, play.stalled);
    SmushStreamTest_StopFramestream();
}

TEST(SmushStream, framestream_TickReadsChunkHeaderAndParsesFLHD)
{
    AnimPlay play;
    uint8_t head[8];
    uint8_t body[8] = { 0 };
    SmushStreamTest_StartFramestream(&play);

    // Size 5 is rounded up to an even 6
    SmushStreamTest_ChunkHeader(head, "FLHD", 5);
    Test_advanceQueue[0] = (int)(intptr_t)head;
    Test_advanceQueue[1] = (int)(intptr_t)body;
    Test_advanceQueue[2] = -1; // next header not ready

    TEST_ASSERT_EQUAL_INT(5, framestream_Tick(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(8, Test_lastAdvanceArg[0]);
    TEST_ASSERT_EQUAL_INT(6, Test_lastAdvanceArg[1]);
    TEST_ASSERT_EQUAL_INT(1, Test_numParse);
    TEST_ASSERT_EQUAL_INT(6, Test_lastParseSize);
    TEST_ASSERT_EQUAL_UINT32(FRAMESTREAM_TAG_NULL, play.tag);
    SmushStreamTest_StopFramestream();
}

TEST(SmushStream, framestream_TickReportsStreamErrorsAndInvalidTags)
{
    AnimPlay play;
    uint8_t head[8];
    SmushStreamTest_StartFramestream(&play);

    Test_advanceQueue[0] = -2;
    TEST_ASSERT_EQUAL_INT(-2, framestream_Tick(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 0, 0, 0));

    SmushStreamTest_ResetFramestreamMocks();
    SmushStreamTest_ChunkHeader(head, "Zq!x", 2);
    Test_advanceQueue[0] = (int)(intptr_t)head;
    TEST_ASSERT_EQUAL_INT(-2, framestream_Tick(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 0, 0, 0));
    TEST_ASSERT_EQUAL_STRING("Read invalid frame header Zq!x -- CD may be dirty or damaged.", Test_lastError);
    SmushStreamTest_StopFramestream();
}

TEST(SmushStream, framestream_TickPresentsFrameAndEndsStream)
{
    AnimPlay play;
    uint8_t head[8];
    uint8_t body[8] = { 0 };
    SmushStreamTest_StartFramestream(&play);
    play.seekTo = -1;
    play.flagsIn = 0;

    SmushStreamTest_ChunkHeader(head, "FRME", 8);
    Test_advanceQueue[0] = (int)(intptr_t)head;
    Test_advanceQueue[1] = (int)(intptr_t)body;

    // Flag 4 in a9 skips the clear of the destination bitmap (which is a null here)
    int result = framestream_Tick(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 4, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, result); // first frame
    TEST_ASSERT_EQUAL_INT(1, Test_numDecode);
    TEST_ASSERT_EQUAL_UINT32(4, Test_lastDecodeFlags);
    TEST_ASSERT_EQUAL_INT(8, Test_lastDecodeSize);
    TEST_ASSERT_EQUAL_INT(1, play.frame);
    // frame >= frameCount - 1 and the streamer is idle: the end chunk follows
    TEST_ASSERT_EQUAL_UINT32(FRAMESTREAM_TAG_END, play.tag);

    // END: continues (no more presenting needed once Continue says no) and answers 4
    Test_decodeResult = 0;
    TEST_ASSERT_EQUAL_INT(4, framestream_Tick(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 4, 0, 0));
    TEST_ASSERT_EQUAL_INT(2, Test_numDecode);

    // WAIT keeps answering 5 until the streamer is active again
    play.tag = FRAMESTREAM_TAG_WAIT;
    TEST_ASSERT_EQUAL_INT(5, framestream_Tick(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 0, 0, 0));
    TEST_ASSERT_EQUAL_UINT32(FRAMESTREAM_TAG_WAIT, play.tag);
    Test_active = 1;
    TEST_ASSERT_EQUAL_INT(5, framestream_Tick(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 0, 0, 0));
    TEST_ASSERT_EQUAL_UINT32(FRAMESTREAM_TAG_NULL, play.tag);
    SmushStreamTest_StopFramestream();
}

TEST(SmushStream, framestream_PresentDecodeResultStallsAndRewinds)
{
    AnimPlay play;
    uint8_t body[8] = { 0 };
    SmushStreamTest_StartFramestream(&play);
    play.pCur      = body;
    play.chunkSize = 8;
    play.outFlags  = 0x20;

    Test_decodeResult = 3;
    framestream_Present(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 4, 0, 0);
    TEST_ASSERT_EQUAL_UINT32(4 | 0x20, Test_lastDecodeFlags);
    TEST_ASSERT_EQUAL_INT(1, play.stalled);
    TEST_ASSERT_EQUAL_INT(1, Test_numFrameReset);
    TEST_ASSERT_EQUAL_INT(1, Test_numRewind);
    SmushStreamTest_StopFramestream();
}

TEST(SmushStream, framestream_SwapChunkPresents)
{
    AnimPlay play;
    uint8_t body[8] = { 0 };
    SmushStreamTest_StartFramestream(&play);
    play.pCur = body;
    play.tag  = FRAMESTREAM_TAG_SWAP;
    play.mode = 3;

    TEST_ASSERT_EQUAL_INT(3, framestream_Tick(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 4, 0, 0));
    TEST_ASSERT_EQUAL_INT(1, Test_numDecode);
    TEST_ASSERT_EQUAL_INT(1, play.mode); // mode 3 drops to 1
    TEST_ASSERT_EQUAL_UINT32(FRAMESTREAM_TAG_NULL, play.tag);
    SmushStreamTest_StopFramestream();
}

TEST(SmushStream, framestream_HeaderSetsFrameCountAndRate)
{
    AnimPlay play;
    FrameHeader header;
    SmushStreamTest_StartFramestream(&play);
    memset(&header, 0, sizeof(header));
    header.count = 77;
    header.rate  = 15;

    // The palette copy is disabled by flag 0x2000
    play.flagsIn = 0x2000;
    framestream_Header(&play, &header, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(0x2000, play.outFlags);
    TEST_ASSERT_EQUAL_INT(77, play.frameCount);
    TEST_ASSERT_EQUAL_INT(1000000 / 15, Test_lastRate);
    TEST_ASSERT_EQUAL_INT(1, Test_numRewind);

    // Flag 8 keeps the file's own rate out
    Test_lastRate = 0;
    play.flagsIn  = 0x2000 | 8;
    framestream_Header(&play, &header, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(0, Test_lastRate);
    SmushStreamTest_StopFramestream();
}

TEST(SmushStream, framestream_ContinueSkipsFramesUntilSeekTarget)
{
    AnimPlay play;
    SmushStreamTest_StartFramestream(&play);

    play.open   = 0;
    play.seekTo = 5;
    play.frame  = 3;
    TEST_ASSERT_EQUAL_INT(1, framestream_Continue(&play));
    play.frame = 5;
    TEST_ASSERT_EQUAL_INT(0, framestream_Continue(&play));
    TEST_ASSERT_EQUAL_INT(-1, play.seekTo);

    play.open     = 1;
    play.seekBase = 2;
    play.seekLen  = 3;
    play.frame    = 5;
    TEST_ASSERT_EQUAL_INT(0, framestream_Continue(&play));
    play.frame = 6;
    TEST_ASSERT_EQUAL_INT(1, framestream_Continue(&play));
    play.paused = 1; // paused streams always continue to present
    TEST_ASSERT_EQUAL_INT(0, framestream_Continue(&play));
    SmushStreamTest_StopFramestream();
}

TEST(SmushStream, framestream_FetchWhilePausedUsesPreloadedFrames)
{
    AnimPlay play;
    SmushStreamTest_StartFramestream(&play);
    play.paused = 1;
    play.seekLen = 4;
    Test_fetchResult = NULL;
    framestream_Fetch(&play, 8);
    TEST_ASSERT_EQUAL_INT(0, play.paused);
    TEST_ASSERT_EQUAL_INT(0, play.seekLen);
    TEST_ASSERT_EQUAL_UINT32(FRAMESTREAM_TAG_WAIT, play.tag);
    TEST_ASSERT_EQUAL_INT(-1, Test_lastAdvanceArg[0]);

    // An end of file from the streamer closes the stream
    play.paused = 0;
    play.open   = 1;
    Test_advanceQueue[1] = 0;
    framestream_Fetch(&play, 8);
    TEST_ASSERT_EQUAL_INT(-1, play.frame);
    TEST_ASSERT_EQUAL_INT(0, play.open);
    SmushStreamTest_StopFramestream();
}

// ---------------------------------------------------------------------------------------------------------------------
// Readauto
// ---------------------------------------------------------------------------------------------------------------------
typedef struct sMockReader
{
    ReadAnimInterface iface;
    const char* pExt0;
    const char* pExt1;
    int openCount;
    int closeCount;
    int lastOp3a;
    int lastOp4a;
} MockReader;

static MockReader Test_readerA, Test_readerB;

static MockReader* Test_ReaderOf(void* p) { return (MockReader*)p; }

// The handle given to the reader callbacks is the reader's own hReader value (the mock reader pointer, set in Open)
static MockReader* Test_activeReader;

static const char* Test_ReaderAQuery(int code) { return code == 100 ? Test_readerA.pExt0 : (code == 101 ? Test_readerA.pExt1 : ""); }
static const char* Test_ReaderBQuery(int code) { return code == 100 ? Test_readerB.pExt0 : ""; }
static int Test_ReaderAOpen(void** ppHandle, const char* pFile) { (void)pFile; *ppHandle = &Test_readerA; ++Test_readerA.openCount; return 0; }
static int Test_ReaderBOpen(void** ppHandle, const char* pFile) { (void)pFile; *ppHandle = &Test_readerB; ++Test_readerB.openCount; return 0; }
static void Test_ReaderClose(void** ppHandle) { ++Test_ReaderOf(*ppHandle)->closeCount; *ppHandle = NULL; }
static int Test_ReaderOp3(void* h, int a, int b, int c, int d, int e) { Test_ReaderOf(h)->lastOp3a = a; return b + c + d + e; }
static int Test_ReaderOp4(void* h, int a, int b) { Test_ReaderOf(h)->lastOp4a = a; return b; }
static int Test_ReaderARecognize(const char* pFile, int pass) { return pass == 0 && strstr(pFile, ".aaa") != NULL; }
static int Test_ReaderBRecognize(const char* pFile, int pass) { return pass == 1 && strstr(pFile, "datab") != NULL; }

static void SmushStreamTest_StartReadauto(void)
{
    memset(&Test_readerA, 0, sizeof(Test_readerA));
    memset(&Test_readerB, 0, sizeof(Test_readerB));
    Test_readerA.iface = (ReadAnimInterface){ Test_ReaderAQuery, Test_ReaderAOpen, Test_ReaderClose, Test_ReaderOp3, Test_ReaderOp4, NULL, Test_ReaderARecognize };
    Test_readerB.iface = (ReadAnimInterface){ Test_ReaderBQuery, Test_ReaderBOpen, Test_ReaderClose, Test_ReaderOp3, Test_ReaderOp4, NULL, Test_ReaderBRecognize };
    Test_readerA.pExt0 = ".aaa";
    Test_readerA.pExt1 = ".aa2";
    Test_readerB.pExt0 = ".bbb";

    Test_Publish("READANIM=a", &Test_readerA.iface);
    Test_Publish("READANIM=b", &Test_readerB.iface);
    Test_Subscription("MEMORY.workspace", &Test_memory);
    Test_Subscription("FILE.io", &Test_share);
    TEST_ASSERT_EQUAL_INT(0, readauto_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(0, readauto_Startup());
    (void)Test_activeReader;
}

TEST(SmushStream, readauto_StartupRequiresServices)
{
    TEST_ASSERT_EQUAL_INT(0, readauto_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(-1, readauto_Startup());
    TEST_ASSERT_EQUAL_STRING("ReadAuto Module: Cannot allocate memory services.", Test_lastError);
    Test_Subscription("MEMORY.workspace", &Test_memory);
    TEST_ASSERT_EQUAL_INT(-1, readauto_Startup());
    TEST_ASSERT_EQUAL_STRING("ReadAuto Module: Cannot allocate file services.", Test_lastError);
}

TEST(SmushStream, readauto_QueryNamesAndExtensions)
{
    SmushStreamTest_StartReadauto();
    TEST_ASSERT_EQUAL_STRING("Auto Reader", readauto_Query(0));
    TEST_ASSERT_NOT_EQUAL(0, strlen(readauto_Query(1)));
    TEST_ASSERT_NOT_EQUAL(0, strlen(readauto_Query(2)));
    TEST_ASSERT_EQUAL_STRING("", readauto_Query(3));
    TEST_ASSERT_EQUAL_STRING("", readauto_Query(99));
    TEST_ASSERT_EQUAL_STRING("", readauto_Query(120));

    // The extensions of all readers in publication order (the auto reader itself is skipped)
    TEST_ASSERT_EQUAL_STRING(".aaa", readauto_Query(100));
    TEST_ASSERT_EQUAL_STRING(".aa2", readauto_Query(101));
    TEST_ASSERT_EQUAL_STRING(".bbb", readauto_Query(102));
    TEST_ASSERT_EQUAL_STRING("", readauto_Query(103));
}

TEST(SmushStream, readauto_OpenPicksReaderByNameThenByData)
{
    SmushStreamTest_StartReadauto();
    void* pHandle = NULL;

    TEST_ASSERT_EQUAL_INT(0, readauto_Open(&pHandle, "movie.aaa"));
    TEST_ASSERT_NOT_NULL(pHandle);
    TEST_ASSERT_EQUAL_INT(1, Test_readerA.openCount);
    TEST_ASSERT_EQUAL_INT(7, readauto_Op3(pHandle, 9, 1, 2, 3, 1));
    TEST_ASSERT_EQUAL_INT(9, Test_readerA.lastOp3a);
    TEST_ASSERT_EQUAL_INT(5, readauto_Op4(pHandle, 4, 5));
    readauto_Close(&pHandle);
    TEST_ASSERT_NULL(pHandle);
    TEST_ASSERT_EQUAL_INT(1, Test_readerA.closeCount);

    // Reader B only recognizes the data in the second pass
    TEST_ASSERT_EQUAL_INT(0, readauto_Open(&pHandle, "datab.bin"));
    TEST_ASSERT_EQUAL_INT(1, Test_readerB.openCount);
    readauto_Close(&pHandle);
    TEST_ASSERT_EQUAL_INT(1, Test_readerB.closeCount);

    TEST_ASSERT_EQUAL_INT(-1, readauto_Open(&pHandle, "nothing.xyz"));
    TEST_ASSERT_EQUAL_STRING("ReadAuto Module: Cannot Recognize File Type", Test_lastError);
    TEST_ASSERT_EQUAL_INT(Test_numAllocs, Test_numFrees);
}

TEST(SmushStream, readauto_ReadOpensReadsAndCloses)
{
    SmushStreamTest_StartReadauto();
    TEST_ASSERT_EQUAL_INT(11, readauto_Read(3, 11, "movie.aaa"));
    TEST_ASSERT_EQUAL_INT(3, Test_readerA.lastOp4a);
    TEST_ASSERT_EQUAL_INT(1, Test_readerA.closeCount);
    TEST_ASSERT_EQUAL_INT(-1, readauto_Read(3, 11, "nothing.xyz"));
}

// ---------------------------------------------------------------------------------------------------------------------
// Zfilio with the gz functions behind the test seam
// ---------------------------------------------------------------------------------------------------------------------
static int Test_gzOpenCount;
static int Test_gzCloseCount;
static int Test_gzFlushArg;
static long Test_gzLastSeek;
static int Test_gzSeekCount;
static unsigned int Test_gzLastReadLen;
static int Test_gzFail;
static int Test_gzFakeHandle;

static gzFile J3DAPI Test_gzopen(const char* pPath, const char* pMode) { (void)pPath; (void)pMode; ++Test_gzOpenCount; return Test_gzFail ? NULL : &Test_gzFakeHandle; }
static int J3DAPI Test_gzclose(gzFile f) { (void)f; ++Test_gzCloseCount; return 0; }
static int J3DAPI Test_gzread(gzFile f, void* pBuf, unsigned int len) { (void)f; Test_gzLastReadLen = len; memset(pBuf, 0x5A, len); return (int)len; }
static int J3DAPI Test_gzwrite(gzFile f, const void* pBuf, unsigned int len) { (void)f; (void)pBuf; return (int)len; }
static int J3DAPI Test_gzflush(gzFile f, int flush) { (void)f; Test_gzFlushArg = flush; return 0; }
static long J3DAPI Test_gzseek(gzFile f, long off, int whence) { (void)f; (void)whence; Test_gzLastSeek = off; ++Test_gzSeekCount; return off; }

static void SmushStreamTest_StartZfilio(void)
{
    ZfilioTestGz gz = { Test_gzopen, Test_gzclose, Test_gzread, Test_gzwrite, Test_gzflush, Test_gzseek };
    Test_gzOpenCount = Test_gzCloseCount = Test_gzFlushArg = Test_gzSeekCount = Test_gzFail = 0;
    Test_gzLastSeek = 0;
    Test_gzLastReadLen = 0;
    Zfilio_TestSetGz(&gz);
    Test_Subscription("FILIO.chainto", &Test_filio);
    Test_Subscription("MEMORY.buffers", &Test_memory);
    TEST_ASSERT_EQUAL_INT(0, zfilio_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(0, zfilio_Startup());
}

TEST(SmushStream, zfilio_StartupRequiresServicesAndPublishesFilioPointer)
{
    TEST_ASSERT_EQUAL_INT(0, zfilio_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(-1, zfilio_Startup());
    TEST_ASSERT_EQUAL_STRING("Zfile Module: Cannot allocate filio services.", Test_lastError);
    Test_Subscription("FILIO.chainto", &Test_filio);
    TEST_ASSERT_EQUAL_INT(-1, zfilio_Startup());
    TEST_ASSERT_EQUAL_STRING("Zfile Module: Cannot allocate memory services.", Test_lastError);
    TEST_ASSERT_EQUAL_PTR(&Test_filio, zfilio_pFilio);
}

TEST(SmushStream, zfilio_ConfigureAddsExtensionsAndRejectsUnknownTags)
{
    SmushStreamTest_StartZfilio();
    uint8_t data[4] = { 0xD2, 0x04, 0, 0 }; // gz trailer: uncompressed size 1234
    Test_AddFile("a.snm", data, 4);

    // .snm is not compressed until the host registers it
    ZfileHandle* pFile = zfilio_open("a.snm", "rb");
    TEST_ASSERT_NOT_NULL(pFile);
    TEST_ASSERT_EQUAL_INT(0, pFile->isGz);
    TEST_ASSERT_EQUAL_INT(0, Test_gzOpenCount);
    zfilio_close(pFile);

    TEST_ASSERT_EQUAL_INT(0, zfilio_Configure(1, ".snm", 1, ".znm", 0));
    pFile = zfilio_open("a.snm", "rb");
    TEST_ASSERT_EQUAL_INT(1, pFile->isGz);
    zfilio_close(pFile);

    TEST_ASSERT_EQUAL_INT(-1, zfilio_Configure(2, 0));
}

TEST(SmushStream, zfilio_ConfigureLimitsExtensionCount)
{
    SmushStreamTest_StartZfilio();
    for ( int i = 1; i < ZFILIO_MAXEXTENSIONS; ++i )
    {
        zfilio_Configure(1, ".x", 0);
    }

    // The table is full: the extension is dropped (the debug service, absent here, would print). The original does not consume
    // the dropped argument, so the next tag read takes the string pointer: the call fails.
    TEST_ASSERT_EQUAL_INT(-1, zfilio_Configure(1, ".snm", 0));
    uint8_t data[4] = { 0 };
    Test_AddFile("a.snm", data, 4);
    ZfileHandle* pFile = zfilio_open("a.snm", "rb");
    TEST_ASSERT_EQUAL_INT(0, pFile->isGz);
    zfilio_close(pFile);
}

TEST(SmushStream, zfilio_GzFileTracksLogicalPosition)
{
    SmushStreamTest_StartZfilio();
    uint8_t data[8] = { 0, 0, 0, 0, 0xD2, 0x04, 0, 0 }; // uncompressed size 1234 in the last 4 bytes
    Test_AddFile("movie.gz", data, 8);

    ZfileHandle* pFile = zfilio_open("movie.gz", "rb");
    TEST_ASSERT_NOT_NULL(pFile);
    TEST_ASSERT_EQUAL_INT(1, pFile->isGz);
    TEST_ASSERT_EQUAL_INT(1234, pFile->gzSize);
    TEST_ASSERT_EQUAL_INT(0, Test_files[0].isOpen); // the size probe closes its handle again

    // Seeks only move the logical position; the gz stream follows lazily on the next read
    zfilio_seek(pFile, 100, 0);
    zfilio_seek(pFile, 20, 1);
    zfilio_seek(pFile, -10, 2);
    TEST_ASSERT_EQUAL_INT(1224, zfilio_tell(pFile));
    TEST_ASSERT_EQUAL_INT(0, Test_gzSeekCount);

    uint8_t buf[16];
    TEST_ASSERT_EQUAL_INT(16, zfilio_read(pFile, buf, 16));
    TEST_ASSERT_EQUAL_INT(1, Test_gzSeekCount);
    TEST_ASSERT_EQUAL_INT(1224, Test_gzLastSeek);
    TEST_ASSERT_EQUAL_UINT8(0x5A, buf[15]);
    TEST_ASSERT_EQUAL_INT(1240, zfilio_tell(pFile));

    // Sequential reads do not seek again
    zfilio_read(pFile, buf, 16);
    TEST_ASSERT_EQUAL_INT(1, Test_gzSeekCount);
    TEST_ASSERT_EQUAL_INT(1256, zfilio_tell(pFile));

    TEST_ASSERT_EQUAL_INT(4, zfilio_write(pFile, buf, 4));
    zfilio_flush(pFile);
    TEST_ASSERT_EQUAL_INT(4, Test_gzFlushArg); // Z_FINISH
    zfilio_close(pFile);
    TEST_ASSERT_EQUAL_INT(1, Test_gzCloseCount);
    TEST_ASSERT_EQUAL_INT(Test_numAllocs, Test_numFrees);
}

TEST(SmushStream, zfilio_PlainFilesPassThroughToChainedFilio)
{
    SmushStreamTest_StartZfilio();
    uint8_t data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    Test_AddFile("plain.bin", data, 8);
    Test_AddFile("rw.gz", data, 8);

    ZfileHandle* pFile = zfilio_open("plain.bin", "rb");
    TEST_ASSERT_NOT_NULL(pFile);
    TEST_ASSERT_EQUAL_INT(0, pFile->isGz);
    uint8_t buf[4];
    TEST_ASSERT_EQUAL_INT(4, zfilio_read(pFile, buf, 4));
    TEST_ASSERT_EQUAL_UINT8(4, buf[3]);
    TEST_ASSERT_EQUAL_INT(4, zfilio_tell(pFile));
    zfilio_seek(pFile, 2, 0);
    TEST_ASSERT_EQUAL_INT(2, zfilio_tell(pFile));
    zfilio_flush(pFile);
    TEST_ASSERT_EQUAL_INT(1, Test_numFileFlushes);
    zfilio_close(pFile);
    TEST_ASSERT_EQUAL_INT(1, Test_numFileCloses);

    // Only read-only modes use gz
    pFile = zfilio_open("rw.gz", "r+b");
    TEST_ASSERT_EQUAL_INT(0, pFile->isGz);
    zfilio_close(pFile);
    pFile = zfilio_open("rw.gz", "wb");
    TEST_ASSERT_EQUAL_INT(0, pFile->isGz);
    zfilio_close(pFile);
    TEST_ASSERT_EQUAL_INT(0, Test_gzOpenCount);

    // A failing gzopen falls back to the chained file
    Test_gzFail = 1;
    pFile       = zfilio_open("rw.gz", "rb");
    TEST_ASSERT_EQUAL_INT(1, Test_gzOpenCount);
    TEST_ASSERT_EQUAL_INT(0, pFile->isGz);
    zfilio_close(pFile);

    // A missing file frees the handle
    int frees = Test_numFrees;
    TEST_ASSERT_NULL(zfilio_open("missing.bin", "rb"));
    TEST_ASSERT_EQUAL_INT(frees + 1, Test_numFrees);
}

// ---------------------------------------------------------------------------------------------------------------------
// Frame (ANIMFRAME)
// ---------------------------------------------------------------------------------------------------------------------
static int Test_decoderParses;
static uint32_t Test_decoderTagParsed;
static int Test_decoderDecodes;
static int Test_decoderResets;
static uint32_t Test_decoderResult;
static int Test_decoderLastSize;

static const char* Test_DecName(int i) { (void)i; return "dec"; }
static int Test_DecGetTag(void) { return 'DEC1'; }
static int Test_DecGetFlags(void) { return 0x10; }
static void Test_DecReset(void) { ++Test_decoderResets; }
static int Test_DecParse(const uint8_t* pData, int size) { (void)pData; ++Test_decoderParses; Test_decoderLastSize = size; return 0; }
static uint32_t Test_DecDecode(void* pBitmap, int a2, int a3, int a4, void* pPalette, const uint8_t* pData, int size, int mode, int a9, int a10)
{
    (void)pBitmap; (void)a2; (void)a3; (void)a4; (void)pPalette; (void)pData; (void)mode; (void)a9; (void)a10;
    ++Test_decoderDecodes;
    Test_decoderLastSize = size;
    return Test_decoderResult;
}

static FlobDecoder Test_decoder = { Test_DecName, Test_DecGetTag, Test_DecGetFlags, Test_DecReset, Test_DecParse, Test_DecDecode };

static int Test_displayBlits;
static int Test_displayPresents;
static int Test_displayPalettes;
static AnimFrameDisplayInfo Test_displayInfo;
static AnimFrameDisplayInfo* Test_DispGetInfo(void) { return &Test_displayInfo; }
static void Test_DispBlit(void* pBitmap, int a3, int a4, int a2) { (void)pBitmap; (void)a3; (void)a4; (void)a2; ++Test_displayBlits; }
static void Test_DispPresent(void* pPalette) { (void)pPalette; ++Test_displayPresents; }
static void Test_DispSetPalette(int a, int b, int count) { (void)a; (void)b; (void)count; ++Test_displayPalettes; }
static AnimFrameDisplayService Test_display = { NULL, Test_DispGetInfo, Test_DispBlit, NULL, Test_DispPresent, NULL, Test_DispSetPalette };

static void SmushStreamTest_StartFrame(void)
{
    Test_decoderParses = Test_decoderDecodes = Test_decoderResets = Test_decoderLastSize = 0;
    Test_decoderResult = 0;
    Test_displayBlits = Test_displayPresents = Test_displayPalettes = 0;
    memset(&Test_displayInfo, 0, sizeof(Test_displayInfo));
    Test_Publish("FLOBDEC=dec", &Test_decoder);
    Test_Subscription("BITMAP.buffers", &Test_memory);
    Test_Subscription("MEMORY.workspace", &Test_memory);
    Test_Subscription("VIDEO.display", &Test_display);
    TEST_ASSERT_EQUAL_INT(0, animframe_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(0, animframe_Startup());
}

TEST(SmushStream, animframe_StartupRequiresBitmapAndMemory)
{
    TEST_ASSERT_EQUAL_INT(0, animframe_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(-1, animframe_Startup());
    TEST_ASSERT_EQUAL_STRING("AnimFrame Module: Cannot allocate bitmap services.", Test_lastError);
    Test_Subscription("BITMAP.buffers", &Test_memory);
    TEST_ASSERT_EQUAL_INT(-1, animframe_Startup());
    TEST_ASSERT_EQUAL_STRING("AnimFrame Module: Cannot allocate memory services.", Test_lastError);
}

TEST(SmushStream, animframe_ParseDispatchesChunksToDecoderByTag)
{
    SmushStreamTest_StartFrame();
    // 'DEC1' with 3 bytes (padded to 4), then an unrelated chunk
    uint8_t data[] = { 'D', 'E', 'C', '1', 0, 0, 0, 3, 1, 2, 3, 0, 'X', 'X', 'X', 'X', 0, 0, 0, 2, 9, 9, 0, 0 };
    TEST_ASSERT_EQUAL_INT(0, animframe_Parse(data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(1, Test_decoderParses);
    TEST_ASSERT_EQUAL_INT(3, Test_decoderLastSize);

    animframe_ResetDecoders();
    TEST_ASSERT_EQUAL_INT(1, Test_decoderResets);
}

TEST(SmushStream, animframe_DecodeMapsDecoderResultToOutputFlags)
{
    SmushStreamTest_StartFrame();
    uint8_t data[] = { 'D', 'E', 'C', '1', 0, 0, 0, 3, 1, 2, 3, 0, 'D', 'E', 'C', '1', 0, 0, 0, 2, 7, 7, 0, 0 };
    int outA = 0, outB = 0;
    uint8_t palette[0x400] = { 0 };

    Test_decoderResult = 1 | 2 | 4 | 0x80;
    uint32_t result    = animframe_Decode(NULL, 0, 0, 0, palette, &outA, &outB, 0, 0, 0, data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(2, Test_decoderDecodes);
    TEST_ASSERT_EQUAL_UINT32(2, result);
    TEST_ASSERT_EQUAL_INT(1, outA);
    TEST_ASSERT_EQUAL_INT(3, outB);
    TEST_ASSERT_EQUAL_INT(2, Test_decoderLastSize);
}

TEST(SmushStream, animframe_DecodeRejectsBadChunkHeaders)
{
    SmushStreamTest_StartFrame();
    int outA = 0, outB = 0;
    uint8_t palette[0x400] = { 0 };

    // A tag with characters outside '0'..'z' ends the frame with 0x8000
    uint8_t bad[] = { 'B', 0x01, 'D', 'D', 0, 0, 0, 2, 0, 0, 0, 0 };
    TEST_ASSERT_EQUAL_UINT32(0x8000, animframe_Decode(NULL, 0, 0, 0, palette, &outA, &outB, 0, 0, 0, bad, sizeof(bad)));
    TEST_ASSERT_EQUAL_INT(0, Test_decoderDecodes);

    // An unknown but well-formed tag is skipped
    uint8_t unknown[] = { 'a', 'b', 'c', 'd', 0, 0, 0, 2, 0, 0, 0, 0 };
    TEST_ASSERT_EQUAL_UINT32(0, animframe_Decode(NULL, 0, 0, 0, palette, &outA, &outB, 0, 0, 0, unknown, sizeof(unknown)));
}

TEST(SmushStream, animframe_FetchWithoutPreloadReturnsNull)
{
    SmushStreamTest_StartFrame();
    TEST_ASSERT_NULL(animframe_Fetch(16));
    TEST_ASSERT_NULL(animframe_Fetch(-8));
}

TEST(SmushStream, animframe_DrawBlitsAndPresentsByFlags)
{
    SmushStreamTest_StartFrame();
    uint8_t palette[0x400] = { 0 };
    int bitmap;

    animframe_Draw(&bitmap, 0, 0, 0, palette, 1, 1, 0);
    TEST_ASSERT_EQUAL_INT(1, Test_displayBlits);
    TEST_ASSERT_EQUAL_INT(1, Test_displayPresents);
    TEST_ASSERT_EQUAL_INT(0, Test_displayPalettes);

    // Flag 0x4000 suppresses the blit, 0x2000 the palette present
    animframe_Draw(&bitmap, 0, 0, 0, palette, 1, 1, 0x4000 | 0x2000);
    TEST_ASSERT_EQUAL_INT(1, Test_displayBlits);
    TEST_ASSERT_EQUAL_INT(1, Test_displayPresents);

    // Present bit 2 resets the palette when there is no bitmap (or the display has none)
    animframe_Draw(NULL, 0, 0, 0, palette, 0, 2, 0);
    TEST_ASSERT_EQUAL_INT(1, Test_displayPalettes);
    animframe_Draw(&bitmap, 0, 0, 0, palette, 0, 2, 0);
    TEST_ASSERT_EQUAL_INT(2, Test_displayPalettes);
    Test_displayInfo.f20 = 1;
    animframe_Draw(&bitmap, 0, 0, 0, palette, 0, 2, 0);
    TEST_ASSERT_EQUAL_INT(2, Test_displayPalettes);
}

TEST(SmushStream, animframe_DebugInterface)
{
    TEST_ASSERT_EQUAL_STRING("Insane AnimFrame", animframe_DebugName(0));
    TEST_ASSERT_EQUAL_STRING("", animframe_DebugName(3));
    animframe_SetDebug(0x1234);
    TEST_ASSERT_EQUAL_INT(0x1234, animframe_GetDebug());
}

// ---------------------------------------------------------------------------------------------------------------------
// Pcplay (ANIMPLAY)
// ---------------------------------------------------------------------------------------------------------------------
static int Test_timerAllocs;
static int Test_timerFrees;
static int Test_timerFailAt;
static int Test_lastIntervalIndex;
static int Test_lastInterval;
static int Test_streamerCreateResult;
static int Test_streamerDestroys;

static int Test_TimerAlloc(TimerCallback pfn, int interval, uint32_t flags, int param)
{
    (void)pfn; (void)interval; (void)flags; (void)param;
    ++Test_timerAllocs;
    return Test_timerAllocs == Test_timerFailAt ? 0 : Test_timerAllocs;
}
static void Test_TimerFree(int index) { (void)index; ++Test_timerFrees; }
static void Test_TimerSetInterval(int index, int interval) { Test_lastIntervalIndex = index; Test_lastInterval = interval; }
static TimerInterface Test_timer = { Test_TimerAlloc, Test_TimerFree, NULL, NULL, Test_TimerSetInterval, NULL, NULL, NULL, NULL };

static int Test_PlayStreamerCreate(Stream** ppStream, int ringSize, int frameSize)
{
    (void)ringSize; (void)frameSize;
    if ( Test_streamerCreateResult ) { return Test_streamerCreateResult; }
    *ppStream = (Stream*)malloc(sizeof(Stream));
    return 0;
}
static void Test_PlayStreamerDestroy(Stream** ppStream) { free(*ppStream); *ppStream = NULL; ++Test_streamerDestroys; }
static int Test_StreamerPercent(Stream* pStream) { (void)pStream; return 50; }
static StreamerInterface Test_playStreamer = { Test_PlayStreamerCreate, Test_PlayStreamerDestroy, Test_StreamerSetFile, Test_StreamerIsActive, NULL, Test_StreamerAdvance, Test_StreamerReset, Test_StreamerPercent };

static int Test_voicePauses;
static int Test_voiceResumes;
static void Test_VoicePause(void) { ++Test_voicePauses; }
static void Test_VoiceResume(void) { ++Test_voiceResumes; }
static SoundInterface Test_voice;

static void SmushStreamTest_StartPcplay(void)
{
    SmushStreamTest_ResetFramestreamMocks();
    Test_timerAllocs = Test_timerFrees = Test_timerFailAt = Test_streamerCreateResult = Test_streamerDestroys = 0;
    Test_voicePauses = Test_voiceResumes = 0;
    memset(&Test_voice, 0, sizeof(Test_voice));
    Test_voice.Pause  = Test_VoicePause;
    Test_voice.Resume = Test_VoiceResume;
    Test_Subscription("MEMORY.workspace", &Test_memory);
    Test_Subscription("STREAMER.spooler", &Test_playStreamer);
    Test_Subscription("ANIMFRAME.render", &Test_frame);
    Test_Subscription("BITMAP.buffers", &Test_memory); // only checked for presence
    Test_Subscription("TIMER.clocks", &Test_timer);
    Test_Subscription("SOUND.lipsync", &Test_voice);
    TEST_ASSERT_EQUAL_INT(0, animplay_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(0, animplay_Startup());
}

TEST(SmushStream, animplay_RegisterPublishesInterfaces)
{
    TEST_ASSERT_EQUAL_INT(0, animplay_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(1, Test_Count("ANIMPLAY"));
    TEST_ASSERT_EQUAL_INT(1, Test_Count("DEBUG"));
    TEST_ASSERT_EQUAL_INT(1, Test_Count("LECMODULE"));
    AnimPlayTable* pTable = Test_Enumerate("ANIMPLAY", 0);
    TEST_ASSERT_TRUE(pTable->IsOpen == framestream_IsOpen);
    TEST_ASSERT_TRUE(pTable->Mode == framestream_Mode);
    animplay_Unregister();
    TEST_ASSERT_EQUAL_INT(0, Test_numPublished);
}

TEST(SmushStream, animplay_StartupChecksRequiredServicesInOrder)
{
    TEST_ASSERT_EQUAL_INT(0, animplay_Register(&Test_share, NULL));
    TEST_ASSERT_EQUAL_INT(-1, animplay_Startup());
    TEST_ASSERT_EQUAL_STRING("AnimPlay Module: Cannot allocate memory services.", Test_lastError);
    Test_Subscription("MEMORY.workspace", &Test_memory);
    TEST_ASSERT_EQUAL_INT(-1, animplay_Startup());
    TEST_ASSERT_EQUAL_STRING("AnimPlay Module: Cannot allocate streamer services.", Test_lastError);
    Test_Subscription("STREAMER.spooler", &Test_playStreamer);
    TEST_ASSERT_EQUAL_INT(-1, animplay_Startup());
    TEST_ASSERT_EQUAL_STRING("AnimPlay Module: Cannot allocate animframe services.", Test_lastError);
    Test_Subscription("ANIMFRAME.render", &Test_frame);
    TEST_ASSERT_EQUAL_INT(-1, animplay_Startup());
    TEST_ASSERT_EQUAL_STRING("AnimPlay Module: Cannot allocate bitmap services.", Test_lastError);
    Test_Subscription("BITMAP.buffers", &Test_memory);
    TEST_ASSERT_EQUAL_INT(-1, animplay_Startup());
    TEST_ASSERT_EQUAL_STRING("AnimPlay Module: Cannot allocate timer services.", Test_lastError);
    Test_Subscription("TIMER.clocks", &Test_timer);
    TEST_ASSERT_EQUAL_INT(0, animplay_Startup()); // debug and voice services are optional
}

TEST(SmushStream, animplay_CreateAllocatesTimersAndDestroyReleasesThem)
{
    SmushStreamTest_StartPcplay();

    AnimPlay* pPlay = NULL;
    TEST_ASSERT_EQUAL_INT(0, animplay_Create(&pPlay, "a.snm", 15, 0x2000, 0x1000, 0x7));
    TEST_ASSERT_NOT_NULL(pPlay);
    TEST_ASSERT_EQUAL_INT(1193180 / 15, pPlay->interval);
    TEST_ASSERT_EQUAL_INT(pPlay->interval, pPlay->interval2);
    TEST_ASSERT_EQUAL_UINT32(7, pPlay->flags);
    TEST_ASSERT_EQUAL_INT(1, pPlay->hTick);
    TEST_ASSERT_EQUAL_INT(2, pPlay->hPace);
    TEST_ASSERT_EQUAL_INT(0, pPlay->done);
    TEST_ASSERT_EQUAL_INT(1, pPlay->open);
    TEST_ASSERT_EQUAL_INT(8, Test_lastSetFileOffset);

    animplay_Destroy(&pPlay);
    TEST_ASSERT_NULL(pPlay);
    TEST_ASSERT_EQUAL_INT(2, Test_timerFrees);
    TEST_ASSERT_EQUAL_INT(1, Test_streamerDestroys);
    TEST_ASSERT_EQUAL_INT(1, Test_numFrameReset);
    TEST_ASSERT_EQUAL_INT(Test_numAllocs, Test_numFrees);
}

TEST(SmushStream, animplay_CreateUndoesPartialSetupOnFailure)
{
    SmushStreamTest_StartPcplay();
    AnimPlay* pPlay = (AnimPlay*)(intptr_t)1;

    // The pace timer cannot be allocated: the tick timer, stream and play object are released again
    Test_timerFailAt = 2;
    TEST_ASSERT_EQUAL_INT(-1, animplay_Create(&pPlay, "a.snm", 15, 0x2000, 0x1000, 0));
    TEST_ASSERT_NULL(pPlay);
    TEST_ASSERT_EQUAL_INT(1, Test_timerFrees);
    TEST_ASSERT_EQUAL_INT(1, Test_streamerDestroys);
    TEST_ASSERT_EQUAL_INT(Test_numAllocs, Test_numFrees);

    // The file cannot be opened
    Test_timerAllocs   = 0;
    Test_timerFailAt   = 0;
    Test_setFileResult = -1;
    TEST_ASSERT_EQUAL_INT(-1, animplay_Create(&pPlay, "missing.snm", 15, 0x2000, 0x1000, 0));
    TEST_ASSERT_EQUAL_INT(0, Test_timerAllocs);
    TEST_ASSERT_EQUAL_INT(Test_numAllocs, Test_numFrees);

    // The stream cannot be created
    Test_setFileResult        = 0;
    Test_streamerCreateResult = -1;
    TEST_ASSERT_EQUAL_INT(-1, animplay_Create(&pPlay, "a.snm", 15, 0x2000, 0x1000, 0));
    TEST_ASSERT_EQUAL_INT(Test_numAllocs, Test_numFrees);
}

TEST(SmushStream, animplay_TimeRateAndCallbacks)
{
    SmushStreamTest_StartPcplay();
    AnimPlay play;
    memset(&play, 0, sizeof(play));
    play.hTick     = 3;
    play.hPace     = 4;
    play.interval2 = 1193180 / 2;
    TEST_ASSERT_EQUAL_INT(500, animplay_GetTime(&play));

    animplay_SetRate(&play, 10);
    TEST_ASSERT_EQUAL_INT(119318, play.interval);
    TEST_ASSERT_EQUAL_INT(119318, play.interval2);
    TEST_ASSERT_EQUAL_INT(4, Test_lastIntervalIndex);
    TEST_ASSERT_EQUAL_INT(119318, Test_lastInterval);

    play.lead = 5;
    animplay_Rewind(&play);
    TEST_ASSERT_EQUAL_INT(0, play.lead);
    TEST_ASSERT_EQUAL_INT(3, Test_lastIntervalIndex);

    TEST_ASSERT_FALSE(animplay_IsDone(&play));
    animplay_TickCallback(0, (int)(intptr_t)&play);
    TEST_ASSERT_EQUAL_INT(1, play.ticks);
    TEST_ASSERT_TRUE(animplay_IsDone(&play));
    play.ticks = 0;
    play.state = 5;
    TEST_ASSERT_TRUE(animplay_IsDone(&play));
}

TEST(SmushStream, animplay_PaceCallbackLeadsAndPausesVoice)
{
    SmushStreamTest_StartPcplay();
    AnimPlay play;
    memset(&play, 0, sizeof(play));

    // Not playing yet: nothing happens
    animplay_PaceCallback(0, (int)(intptr_t)&play);
    TEST_ASSERT_EQUAL_INT(0, play.lead);

    play.playing = 1;
    animplay_PaceCallback(0, (int)(intptr_t)&play);
    TEST_ASSERT_EQUAL_INT(1, play.lead);

    // Flag 1 pauses the voice once the video is 7 frames ahead, and the lead stops growing
    play.flags = 1;
    play.lead  = 7;
    animplay_PaceCallback(0, (int)(intptr_t)&play);
    TEST_ASSERT_EQUAL_INT(1, Test_voicePauses);
    TEST_ASSERT_EQUAL_INT(7, play.lead);

    // Resumed once the lead is negative
    play.lead = -1;
    animplay_PaceCallback(0, (int)(intptr_t)&play);
    TEST_ASSERT_EQUAL_INT(1, Test_voiceResumes);
    TEST_ASSERT_EQUAL_INT(0, play.lead);
}

TEST(SmushStream, animplay_StepWaitsForTickThenRunsFrameStream)
{
    SmushStreamTest_StartPcplay();
    AnimPlay play;
    memset(&play, 0, sizeof(play));
    play.hTick    = 1;
    play.hPace    = 2;
    play.interval = 100000;
    play.state    = 0;
    play.tag      = FRAMESTREAM_TAG_NULL;

    // No tick yet: 6 (nothing to do)
    TEST_ASSERT_EQUAL_INT(6, animplay_Step(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 0, 0));

    // A tick lowers the lead (-1 here: the frame is late by 79545 ticks), clamps the interval to 3/4..4/3 and runs the stream
    play.ticks = 1;
    play.lead  = 0;
    Test_advanceQueue[0] = -1; // streamer not ready: state 5
    TEST_ASSERT_EQUAL_INT(5, animplay_Step(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, play.ticks);
    TEST_ASSERT_EQUAL_INT(-1, play.lead);
    TEST_ASSERT_EQUAL_INT(133333, play.interval2); // 100000 + 79545 clamped to 4/3
    TEST_ASSERT_EQUAL_INT(133333, Test_lastInterval);
    TEST_ASSERT_EQUAL_INT(5, play.state);

    // In state 5 the frame stream is polled without a tick; a stream error ends the player
    Test_advanceQueue[1] = -2;
    TEST_ASSERT_EQUAL_INT(-2, animplay_Step(&play, NULL, 0, 0, 0, NULL, NULL, NULL, 0, 0));
    TEST_ASSERT_EQUAL_INT(1, play.done);
}

TEST(SmushStream, animplay_DebugInterface)
{
    TEST_ASSERT_EQUAL_STRING("Insane AnimPlay", animplay_DebugName(0));
    TEST_ASSERT_EQUAL_STRING("", animplay_DebugName(5));
    animplay_SetDebug(0x55);
    TEST_ASSERT_EQUAL_INT(0x55, animplay_GetDebug());
}

TEST_GROUP_RUNNER(SmushStream)
{
    RUN_TEST_CASE(SmushStream, stdfile_RegistersAndRequiresChainedFilio);
    RUN_TEST_CASE(SmushStream, stdfile_ForwardsToChainedFilio);
    RUN_TEST_CASE(SmushStream, stdfile_SizeKeepsPositionAndEofComparesToSize);
    RUN_TEST_CASE(SmushStream, stdfile_GetcPutc);
    RUN_TEST_CASE(SmushStream, stdfile_GetsStopsAtNewlineAndEof);
    RUN_TEST_CASE(SmushStream, stdfile_PrintfWritesFormattedText);
    RUN_TEST_CASE(SmushStream, stdio_RegistersFilioModule);
    RUN_TEST_CASE(SmushStream, stdio_WritesReadsSeeksAndTellsRealFile);
    RUN_TEST_CASE(SmushStream, streamer_StartupRequiresServices);
    RUN_TEST_CASE(SmushStream, streamer_CreateRoundsRingSizeAndDestroyFrees);
    RUN_TEST_CASE(SmushStream, streamer_SetFileFailsForMissingFile);
    RUN_TEST_CASE(SmushStream, streamer_FillAdvanceAndPercent);
    RUN_TEST_CASE(SmushStream, streamer_PrimeRetriesThroughCallback);
    RUN_TEST_CASE(SmushStream, streamer_InRangeAndCancel);
    RUN_TEST_CASE(SmushStream, framestream_ResetClearsState);
    RUN_TEST_CASE(SmushStream, framestream_ModeOperations);
    RUN_TEST_CASE(SmushStream, framestream_OpenSeekAndStall);
    RUN_TEST_CASE(SmushStream, framestream_TickReadsChunkHeaderAndParsesFLHD);
    RUN_TEST_CASE(SmushStream, framestream_TickReportsStreamErrorsAndInvalidTags);
    RUN_TEST_CASE(SmushStream, framestream_TickPresentsFrameAndEndsStream);
    RUN_TEST_CASE(SmushStream, framestream_PresentDecodeResultStallsAndRewinds);
    RUN_TEST_CASE(SmushStream, framestream_SwapChunkPresents);
    RUN_TEST_CASE(SmushStream, framestream_HeaderSetsFrameCountAndRate);
    RUN_TEST_CASE(SmushStream, framestream_ContinueSkipsFramesUntilSeekTarget);
    RUN_TEST_CASE(SmushStream, framestream_FetchWhilePausedUsesPreloadedFrames);
    RUN_TEST_CASE(SmushStream, readauto_StartupRequiresServices);
    RUN_TEST_CASE(SmushStream, readauto_QueryNamesAndExtensions);
    RUN_TEST_CASE(SmushStream, readauto_OpenPicksReaderByNameThenByData);
    RUN_TEST_CASE(SmushStream, readauto_ReadOpensReadsAndCloses);
    RUN_TEST_CASE(SmushStream, zfilio_StartupRequiresServicesAndPublishesFilioPointer);
    RUN_TEST_CASE(SmushStream, zfilio_ConfigureAddsExtensionsAndRejectsUnknownTags);
    RUN_TEST_CASE(SmushStream, zfilio_ConfigureLimitsExtensionCount);
    RUN_TEST_CASE(SmushStream, zfilio_GzFileTracksLogicalPosition);
    RUN_TEST_CASE(SmushStream, zfilio_PlainFilesPassThroughToChainedFilio);
    RUN_TEST_CASE(SmushStream, animframe_StartupRequiresBitmapAndMemory);
    RUN_TEST_CASE(SmushStream, animframe_ParseDispatchesChunksToDecoderByTag);
    RUN_TEST_CASE(SmushStream, animframe_DecodeMapsDecoderResultToOutputFlags);
    RUN_TEST_CASE(SmushStream, animframe_DecodeRejectsBadChunkHeaders);
    RUN_TEST_CASE(SmushStream, animframe_FetchWithoutPreloadReturnsNull);
    RUN_TEST_CASE(SmushStream, animframe_DrawBlitsAndPresentsByFlags);
    RUN_TEST_CASE(SmushStream, animframe_DebugInterface);
    RUN_TEST_CASE(SmushStream, animplay_RegisterPublishesInterfaces);
    RUN_TEST_CASE(SmushStream, animplay_StartupChecksRequiredServicesInOrder);
    RUN_TEST_CASE(SmushStream, animplay_CreateAllocatesTimersAndDestroyReleasesThem);
    RUN_TEST_CASE(SmushStream, animplay_CreateUndoesPartialSetupOnFailure);
    RUN_TEST_CASE(SmushStream, animplay_TimeRateAndCallbacks);
    RUN_TEST_CASE(SmushStream, animplay_PaceCallbackLeadsAndPausesVoice);
    RUN_TEST_CASE(SmushStream, animplay_StepWaitsForTickThenRunsFrameStream);
    RUN_TEST_CASE(SmushStream, animplay_DebugInterface);
}
