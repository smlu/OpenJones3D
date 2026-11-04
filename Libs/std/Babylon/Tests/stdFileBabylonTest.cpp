// Added: Unity Fixture suite for the Babylon column's files adapter: stdBabylonPath,
// stdFileBabylon and stdFileUtilBabylon. It compiles the adapter files directly and drives them
// through a fresh StdBabylonContext whose pFileSystem is a babylon::files::NullFileSystem (current
// path "/"), so every value is exact and deterministic. Files are prepared through the
// NullFileSystem API. This file also carries the single main() for the stdBabylonTests executable,
// running the time suite of stdPlatformTimeBabylonTest.cpp together with these groups.
#include <unity_fixture.h>

#include <cstddef>
#include <cstring>
#include <cwchar>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <babylon/files/FileSystem.h>
#include <babylon/files/NullFileSystem.h>
#include <babylon/time/NullClock.h>

#include <std/Babylon/stdBabylon.h>
#include <std/Babylon/stdBabylonPath.h>
#include <std/General/std.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdMemory.h>

TEST_GROUP(stdBabylonPath);
TEST_GROUP(stdFileBabylon);
TEST_GROUP(stdFileUtilBabylon);

// The stdBabylonTests executable does not link General/stdMemory.c; stdFileUtil_DisposeFind
// releases the FindFileData block through stdMemory_Free, which pairs here with the new the
// helpers below use.
extern "C" void J3DAPI stdMemory_Free(void* pBytes)
{
    ::operator delete(pBytes);
}

namespace
{
    babylon::time::NullClock*      pNullClock    = nullptr;
    babylon::time::NullClock*      pVirtualClock = nullptr;
    babylon::files::NullFileSystem* pFileSystem  = nullptr;
    StdBabylonContext              context;

    void InstallContext()
    {
        pNullClock    = new babylon::time::NullClock();
        pVirtualClock = new babylon::time::NullClock();
        pFileSystem   = new babylon::files::NullFileSystem();

        context.pClock        = pNullClock;
        context.pVirtualClock = pVirtualClock;
        context.pFileSystem   = pFileSystem;
        stdBabylon_SetContext(&context);
    }

    void RemoveContext()
    {
        stdBabylon_SetContext(nullptr);
        delete pFileSystem;
        delete pVirtualClock;
        delete pNullClock;
        pFileSystem   = nullptr;
        pVirtualClock = nullptr;
        pNullClock    = nullptr;
    }

    // The NullFileSystem API takes bytes; the fixtures are spelled as text.
    std::span<const std::byte> Bytes(std::string_view aText)
    {
        return std::span<const std::byte>(reinterpret_cast<const std::byte*>(aText.data()), aText.size());
    }

    // Reads a whole stored file through the babylon::files::File interface, for checking what the
    // adapter wrote; false when the file cannot be opened or a read fails.
    bool ReadWholeFile(const char* pPath, std::string& out)
    {
        std::unique_ptr<babylon::files::File> pFile = pFileSystem->open(pPath, babylon::files::OpenMode::Read);
        if ( !pFile )
        {
            return false;
        }

        out.clear();
        std::byte aChunk[256];
        for (;;)
        {
            const std::optional<std::size_t> numRead = pFile->read(std::span(aChunk));
            if ( !numRead || *numRead == 0 )
            {
                return numRead.has_value();
            }

            out.append(reinterpret_cast<const char*>(aChunk), *numRead);
        }
    }

    // Runs one complete search through the adapter and copies the reported names; the count of
    // results is returned, entries beyond numMax are not collected.
    int CollectFind(const char* pFilter, tFoundFileInfo* pInfos, int numMax)
    {
        FindFileData* pData       = new FindFileData;
        std::memset(pData, 0, sizeof(*pData));
        std::strncpy(pData->aSearchFilter, pFilter, sizeof(pData->aSearchFilter) - 1);

        int numFound = 0;
        while ( numFound < numMax && stdFileUtil_FindNext(pData, &pInfos[numFound]) )
        {
            ++numFound;
        }

        stdFileUtil_DisposeFind(pData);
        return numFound;
    }
}

TEST_SETUP(stdBabylonPath)
{
    InstallContext();
}

TEST_TEAR_DOWN(stdBabylonPath)
{
    RemoveContext();
}

TEST(stdBabylonPath, ResolvesExistingEntriesCaseInsensitively)
{
    TEST_ASSERT_TRUE(pFileSystem->addFile("/data/sub/file.txt", Bytes("text")));

    std::string out;
    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "Data\\Sub\\File.TXT", out));
    TEST_ASSERT_EQUAL_STRING("/data/sub/file.txt", out.c_str());

    // A leading backslash has no drive to root it against, so it is relative (the current path is
    // "/" here, the same root the absolute spelling gets).
    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "\\data\\x", out));
    TEST_ASSERT_EQUAL_STRING("/data/x", out.c_str());

    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "/data/sub/file.txt", out));
    TEST_ASSERT_EQUAL_STRING("/data/sub/file.txt", out.c_str());

    // Among case-folded equal names the byte order picks: "AB" before "ab".
    TEST_ASSERT_TRUE(pFileSystem->addFile("/d/ab", Bytes("1")));
    TEST_ASSERT_TRUE(pFileSystem->addFile("/d/AB", Bytes("2")));
    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "/d/Ab", out));
    TEST_ASSERT_EQUAL_STRING("/d/AB", out.c_str());
}

TEST(stdBabylonPath, NormalizesDotsAndStopsAtMissingSegments)
{
    TEST_ASSERT_TRUE(pFileSystem->addFile("/data/sub/file.txt", Bytes("text")));

    std::string out;
    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "a/./b/../c", out));
    TEST_ASSERT_EQUAL_STRING("/a/c", out.c_str());

    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "../x", out));
    TEST_ASSERT_EQUAL_STRING("/x", out.c_str());

    // A missing segment stops resolving: it and everything after it stay as given, while the
    // earlier segments keep their resolved spelling.
    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "/Data/NEW/Thing", out));
    TEST_ASSERT_EQUAL_STRING("/data/NEW/Thing", out.c_str());

    // The root survives an all-dots path.
    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "..", out));
    TEST_ASSERT_EQUAL_STRING("/", out.c_str());

    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "/", out));
    TEST_ASSERT_EQUAL_STRING("/", out.c_str());
}

TEST(stdBabylonPath, KeepsTrailingSeparatorsAndRejectsEmptyInput)
{
    TEST_ASSERT_TRUE(pFileSystem->addFile("/data/sub/file.txt", Bytes("text")));

    std::string out;
    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "/data/sub/", out));
    TEST_ASSERT_EQUAL_STRING("/data/sub/", out.c_str());

    TEST_ASSERT_TRUE(stdBabylonPath_Resolve(*pFileSystem, "Data\\Sub\\", out));
    TEST_ASSERT_EQUAL_STRING("/data/sub/", out.c_str());

    out = "keep";
    TEST_ASSERT_FALSE(stdBabylonPath_Resolve(*pFileSystem, NULL, out));
    TEST_ASSERT_EQUAL_STRING("keep", out.c_str());

    TEST_ASSERT_FALSE(stdBabylonPath_Resolve(*pFileSystem, "", out));
    TEST_ASSERT_EQUAL_STRING("keep", out.c_str());
}

TEST_GROUP_RUNNER(stdBabylonPath)
{
    RUN_TEST_CASE(stdBabylonPath, ResolvesExistingEntriesCaseInsensitively);
    RUN_TEST_CASE(stdBabylonPath, NormalizesDotsAndStopsAtMissingSegments);
    RUN_TEST_CASE(stdBabylonPath, KeepsTrailingSeparatorsAndRejectsEmptyInput);
}

TEST_SETUP(stdFileBabylon)
{
    InstallContext();
}

TEST_TEAR_DOWN(stdFileBabylon)
{
    RemoveContext();
}

TEST(stdFileBabylon, OpenReadEofTellAcrossBufferSizes)
{
    std::string aBig;
    for ( int cur = 0; cur < 20000; ++cur )
    {
        aBig.push_back(static_cast<char>(cur & 0xFF));
    }
    TEST_ASSERT_TRUE(pFileSystem->addFile("/big.bin", Bytes(aBig)));

    // Relative and spelled with backslashes and another case: the resolver finds the file.
    tFileHandle fh = stdFileOpen("Big.bin", "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);

    char aSmall[4];
    TEST_ASSERT_EQUAL_size_t(3u, stdFileRead(fh, aSmall, 3u));
    TEST_ASSERT_EQUAL_MEMORY(aBig.data(), aSmall, 3u);
    TEST_ASSERT_EQUAL_INT(3, stdFileTell(fh));
    TEST_ASSERT_EQUAL_INT(0, stdFileEof(fh));

    static char aMid[5000];
    TEST_ASSERT_EQUAL_size_t(5000u, stdFileRead(fh, aMid, 5000u));
    TEST_ASSERT_EQUAL_MEMORY(aBig.data() + 3, aMid, 5000u);
    TEST_ASSERT_EQUAL_INT(5003, stdFileTell(fh));
    TEST_ASSERT_EQUAL_INT(0, stdFileEof(fh));

    static char aRest[14997];
    TEST_ASSERT_EQUAL_size_t(14997u, stdFileRead(fh, aRest, 14997u));
    TEST_ASSERT_EQUAL_MEMORY(aBig.data() + 5003, aRest, 14997u);
    TEST_ASSERT_EQUAL_INT(20000, stdFileTell(fh));

    // EOF only after a read hits the end.
    char aOne;
    TEST_ASSERT_EQUAL_size_t(0u, stdFileRead(fh, &aOne, 1u));
    TEST_ASSERT_EQUAL_INT(1, stdFileEof(fh));
    TEST_ASSERT_EQUAL_INT(20000, stdFileTell(fh));

    // A read on a Write handle returns 0.
    tFileHandle fhWrite = stdFileOpen("/write.bin", "wb");
    TEST_ASSERT_NOT_EQUAL(0u, fhWrite);
    TEST_ASSERT_EQUAL_size_t(0u, stdFileRead(fhWrite, aSmall, sizeof(aSmall)));
    TEST_ASSERT_EQUAL_INT(0, stdFileFlush(fhWrite));

    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fhWrite));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(0));
}

TEST(stdFileBabylon, Gets)
{
    TEST_ASSERT_TRUE(pFileSystem->addFile("/lines.txt", Bytes("alpha\nbravo")));

    tFileHandle fh = stdFileOpen("/lines.txt", "r");
    TEST_ASSERT_NOT_EQUAL(0u, fh);

    char aBuf[16];
    TEST_ASSERT_EQUAL_PTR(aBuf, stdFileGets(fh, aBuf, sizeof(aBuf)));
    TEST_ASSERT_EQUAL_STRING("alpha\n", aBuf);
    TEST_ASSERT_EQUAL_INT(6, stdFileTell(fh));
    TEST_ASSERT_EQUAL_INT(0, stdFileEof(fh));

    TEST_ASSERT_EQUAL_PTR(aBuf, stdFileGets(fh, aBuf, sizeof(aBuf)));
    TEST_ASSERT_EQUAL_STRING("bravo", aBuf);

    TEST_ASSERT_NULL(stdFileGets(fh, aBuf, sizeof(aBuf)));
    TEST_ASSERT_EQUAL_INT(1, stdFileEof(fh));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    // A line longer than size - 1 splits across calls.
    TEST_ASSERT_TRUE(pFileSystem->addFile("/long.txt", Bytes("abcdefgh\n")));
    fh = stdFileOpen("/long.txt", "r");
    TEST_ASSERT_NOT_EQUAL(0u, fh);

    char aFour[5];
    TEST_ASSERT_EQUAL_PTR(aFour, stdFileGets(fh, aFour, sizeof(aFour)));
    TEST_ASSERT_EQUAL_STRING("abcd", aFour);
    TEST_ASSERT_EQUAL_PTR(aFour, stdFileGets(fh, aFour, sizeof(aFour)));
    TEST_ASSERT_EQUAL_STRING("efgh", aFour);
    TEST_ASSERT_EQUAL_PTR(aFour, stdFileGets(fh, aFour, sizeof(aFour)));
    TEST_ASSERT_EQUAL_STRING("\n", aFour);
    TEST_ASSERT_NULL(stdFileGets(fh, aFour, sizeof(aFour)));
    TEST_ASSERT_EQUAL_INT(1, stdFileEof(fh));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    // size 1 terminates without reading, size 0 fails like fgets.
    fh = stdFileOpen("/long.txt", "r");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    char aOneByte[1];
    TEST_ASSERT_EQUAL_PTR(aOneByte, stdFileGets(fh, aOneByte, 1u));
    TEST_ASSERT_EQUAL_STRING("", aOneByte);
    TEST_ASSERT_EQUAL_INT(0, stdFileTell(fh));
    TEST_ASSERT_NULL(stdFileGets(fh, aOneByte, 0u));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    // Nothing read at the end of an empty file: NULL with EOF set.
    TEST_ASSERT_TRUE(pFileSystem->addFile("/empty.txt", Bytes("")));
    fh = stdFileOpen("/empty.txt", "r");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_NULL(stdFileGets(fh, aBuf, sizeof(aBuf)));
    TEST_ASSERT_EQUAL_INT(1, stdFileEof(fh));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
}

TEST(stdFileBabylon, GetwsClocale)
{
    TEST_ASSERT_TRUE(pFileSystem->addFile("/wide.txt", Bytes("wide\n")));

    tFileHandle fh = stdFileOpen("/wide.txt", "r");
    TEST_ASSERT_NOT_EQUAL(0u, fh);

    wchar_t aWide[16];
    TEST_ASSERT_EQUAL_PTR(aWide, stdFileGetws(fh, aWide, std::size(aWide)));
    TEST_ASSERT_EQUAL_INT(0, std::wcscmp(L"wide\n", aWide));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    // In the "C" locale a byte of 0x80 or more is not a wide character: the call returns NULL.
    std::string aHigh;
    aHigh.push_back(static_cast<char>(0x80));
    aHigh.push_back('x');
    TEST_ASSERT_TRUE(pFileSystem->addFile("/high.txt", Bytes(aHigh)));

    fh = stdFileOpen("/high.txt", "r");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_NULL(stdFileGetws(fh, aWide, std::size(aWide)));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
}

TEST(stdFileBabylon, SeekClearsEofAndDropsTheBuffer)
{
    std::string aBig;
    for ( int cur = 0; cur < 20000; ++cur )
    {
        aBig.push_back(static_cast<char>(cur & 0xFF));
    }
    TEST_ASSERT_TRUE(pFileSystem->addFile("/big.bin", Bytes(aBig)));

    tFileHandle fh = stdFileOpen("/big.bin", "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);

    char aSmall[16];
    TEST_ASSERT_EQUAL_size_t(3u, stdFileRead(fh, aSmall, 3u)); // fills the read-ahead buffer

    // A failing seek restores the logical position: the read-ahead filled the buffer to 8192,
    // seeking past the end is refused, Tell still reports 3 and the next read continues at
    // byte 3. (fseek may go past the end of a read file; the Babylon interface refuses that.)
    TEST_ASSERT_EQUAL_INT(-1, stdFileSeek(fh, 19999, 1)); // logical 3 + 19999 > 20000
    TEST_ASSERT_EQUAL_INT(3, stdFileTell(fh));
    TEST_ASSERT_EQUAL_size_t(1u, stdFileRead(fh, aSmall, 1u));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(static_cast<unsigned char>(aBig[3])),
        static_cast<int>(static_cast<unsigned char>(aSmall[0])));

    // Current is turned into Begin from the logical position, not the buffered file position.
    TEST_ASSERT_EQUAL_INT(0, stdFileSeek(fh, 99, 1)); // logical 4 + 99
    TEST_ASSERT_EQUAL_INT(103, stdFileTell(fh));
    TEST_ASSERT_EQUAL_size_t(4u, stdFileRead(fh, aSmall, 4u));
    TEST_ASSERT_EQUAL_MEMORY(aBig.data() + 103, aSmall, 4u);

    TEST_ASSERT_EQUAL_INT(0, stdFileSeek(fh, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, stdFileTell(fh));
    TEST_ASSERT_EQUAL_size_t(2u, stdFileRead(fh, aSmall, 2u));
    TEST_ASSERT_EQUAL_MEMORY(aBig.data(), aSmall, 2u);

    TEST_ASSERT_EQUAL_INT(0, stdFileSeek(fh, -10, 2));
    TEST_ASSERT_EQUAL_INT(19990, stdFileTell(fh));
    TEST_ASSERT_EQUAL_size_t(10u, stdFileRead(fh, aSmall, 10u));
    TEST_ASSERT_EQUAL_MEMORY(aBig.data() + 19990, aSmall, 10u);

    // fseek can move a read file past the end; the Babylon interface refuses it, so this is -1.
    TEST_ASSERT_EQUAL_INT(-1, stdFileSeek(fh, 10, 2));
    TEST_ASSERT_EQUAL_INT(20000, stdFileTell(fh));
    TEST_ASSERT_EQUAL_INT(-1, stdFileSeek(fh, -5, 0));
    TEST_ASSERT_EQUAL_INT(-1, stdFileSeek(fh, 0, 5));

    // Seek clears the EOF flag on success.
    char aOne;
    TEST_ASSERT_EQUAL_size_t(0u, stdFileRead(fh, &aOne, 1u));
    TEST_ASSERT_EQUAL_INT(1, stdFileEof(fh));
    TEST_ASSERT_EQUAL_INT(0, stdFileSeek(fh, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, stdFileEof(fh));

    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    // Handle 0 behaves like the failure paths of the C runtime calls.
    TEST_ASSERT_EQUAL_INT(-1, stdFileTell(0));
    TEST_ASSERT_EQUAL_INT(-1, stdFileSeek(0, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, stdFileEof(0));
}

TEST(stdFileBabylon, WriteAndAppendModes)
{
    TEST_ASSERT_TRUE(pFileSystem->addFile("/trunc.txt", Bytes("oldcontent")));

    // w truncates.
    tFileHandle fh = stdFileOpen("/Trunc.txt", "wb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(5u, stdFileWrite(fh, "12345", 5u));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
    TEST_ASSERT_EQUAL_size_t(5u, stdFileSize("/trunc.txt"));

    std::string aContents;
    TEST_ASSERT_TRUE(ReadWholeFile("/trunc.txt", aContents));
    TEST_ASSERT_EQUAL_STRING("12345", aContents.c_str());

    // a appends.
    TEST_ASSERT_TRUE(pFileSystem->addFile("/app.txt", Bytes("AB")));
    fh = stdFileOpen("/app.txt", "ab");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(2u, stdFileWrite(fh, "CD", 2u));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
    TEST_ASSERT_EQUAL_size_t(4u, stdFileSize("/app.txt"));
    TEST_ASSERT_TRUE(ReadWholeFile("/app.txt", aContents));
    TEST_ASSERT_EQUAL_STRING("ABCD", aContents.c_str());

    // wb+ and wt+ open for writing; a read on such a handle returns 0.
    fh = stdFileOpen("/plus.bin", "wb+");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(1u, stdFileWrite(fh, "x", 1u));
    char aRead[2];
    TEST_ASSERT_EQUAL_size_t(0u, stdFileRead(fh, aRead, sizeof(aRead)));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
    TEST_ASSERT_EQUAL_size_t(1u, stdFileSize("/plus.bin"));

    fh = stdFileOpen("/plus.bin", "wt+");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(1u, stdFileWrite(fh, "y", 1u));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
    TEST_ASSERT_TRUE(ReadWholeFile("/plus.bin", aContents));
    TEST_ASSERT_EQUAL_STRING("y", aContents.c_str());

    fh = stdFileOpen("/app.txt", "at+");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(1u, stdFileWrite(fh, "E", 1u));
    TEST_ASSERT_EQUAL_size_t(0u, stdFileRead(fh, aRead, sizeof(aRead)));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
    TEST_ASSERT_EQUAL_size_t(5u, stdFileSize("/app.txt"));

    // Open of a missing file for reading returns 0 (the probe never logs); Size says 0 too.
    TEST_ASSERT_EQUAL(0u, stdFileOpen("/missing.bin", "rb"));
    TEST_ASSERT_EQUAL_size_t(0u, stdFileSize("/missing.bin"));
    TEST_ASSERT_EQUAL(0u, stdFileSize("/missing.bin"));

    // Null name, null or unknown mode: 0.
    TEST_ASSERT_EQUAL(0u, stdFileOpen(NULL, "rb"));
    TEST_ASSERT_EQUAL(0u, stdFileOpen("/trunc.txt", NULL));
    TEST_ASSERT_EQUAL(0u, stdFileOpen("/trunc.txt", ""));
    TEST_ASSERT_EQUAL(0u, stdFileOpen("/trunc.txt", "x"));
}

TEST_GROUP_RUNNER(stdFileBabylon)
{
    RUN_TEST_CASE(stdFileBabylon, OpenReadEofTellAcrossBufferSizes);
    RUN_TEST_CASE(stdFileBabylon, Gets);
    RUN_TEST_CASE(stdFileBabylon, GetwsClocale);
    RUN_TEST_CASE(stdFileBabylon, SeekClearsEofAndDropsTheBuffer);
    RUN_TEST_CASE(stdFileBabylon, WriteAndAppendModes);
}

TEST_SETUP(stdFileUtilBabylon)
{
    InstallContext();
}

TEST_TEAR_DOWN(stdFileUtilBabylon)
{
    RemoveContext();
}

TEST(stdFileUtilBabylon, FindNextReportsDotEntriesInSortedOrder)
{
    TEST_ASSERT_TRUE(pFileSystem->createDirectory("/fx/child/"));
    TEST_ASSERT_TRUE(pFileSystem->addFile("/fx/a.txt", Bytes("1")));
    TEST_ASSERT_TRUE(pFileSystem->addFile("/fx/AbC.txt", Bytes("2")));
    TEST_ASSERT_TRUE(pFileSystem->addFile("/fx/b.txt", Bytes("3")));
    TEST_ASSERT_TRUE(pFileSystem->addFile("/fx/c.bin", Bytes("4")));

    // The POSIX listing reports "." and ".."; the search adds them back as directories. Order:
    // case-insensitive name order with the byte order breaking ties.
    tFoundFileInfo aInfos[16];
    TEST_ASSERT_EQUAL_INT(7, CollectFind("/fx/*.*", aInfos, 16));
    TEST_ASSERT_EQUAL_STRING(".", aInfos[0].aName);
    TEST_ASSERT_EQUAL_INT(1, aInfos[0].bIsDirectory);
    TEST_ASSERT_EQUAL_STRING("..", aInfos[1].aName);
    TEST_ASSERT_EQUAL_INT(1, aInfos[1].bIsDirectory);
    TEST_ASSERT_EQUAL_STRING("a.txt", aInfos[2].aName);
    TEST_ASSERT_EQUAL_INT(0, aInfos[2].bIsDirectory);
    TEST_ASSERT_EQUAL_STRING("AbC.txt", aInfos[3].aName);
    TEST_ASSERT_EQUAL_STRING("b.txt", aInfos[4].aName);
    TEST_ASSERT_EQUAL_STRING("c.bin", aInfos[5].aName);
    TEST_ASSERT_EQUAL_STRING("child", aInfos[6].aName);
    TEST_ASSERT_EQUAL_INT(1, aInfos[6].bIsDirectory);

    // Babylon reports no times for directory entries.
    for ( int cur = 0; cur < 7; ++cur )
    {
        TEST_ASSERT_EQUAL_UINT32(0u, aInfos[cur].lastChanged);
    }

    TEST_ASSERT_EQUAL_INT(3, CollectFind("/fx/*.txt", aInfos, 16));
    TEST_ASSERT_EQUAL_STRING("a.txt", aInfos[0].aName);
    TEST_ASSERT_EQUAL_STRING("AbC.txt", aInfos[1].aName);
    TEST_ASSERT_EQUAL_STRING("b.txt", aInfos[2].aName);

    TEST_ASSERT_EQUAL_INT(1, CollectFind("/fx/A?C.*", aInfos, 16));
    TEST_ASSERT_EQUAL_STRING("AbC.txt", aInfos[0].aName);

    // No match and a missing directory: no cursor, FindNext returns 0.
    TEST_ASSERT_EQUAL_INT(0, CollectFind("/fx/*.xyz", aInfos, 16));
    TEST_ASSERT_EQUAL_INT(0, CollectFind("/missing/*.*", aInfos, 16));
}

TEST(stdFileUtilBabylon, FindNextRetriesAfterAFailedSearch)
{
    TEST_ASSERT_TRUE(pFileSystem->addFile("/fx/a.txt", Bytes("1")));

    FindFileData* pData = new FindFileData;
    std::memset(pData, 0, sizeof(*pData));
    std::strncpy(pData->aSearchFilter, "/missing/*.*", sizeof(pData->aSearchFilter) - 1);

    tFoundFileInfo info;
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FindNext(pData, &info));
    TEST_ASSERT_EQUAL_INT(0, pData->nFoundFiles);

    // Only successful results are counted, so the same FindFileData can be searched again.
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FindNext(pData, &info));
    TEST_ASSERT_EQUAL_INT(0, pData->nFoundFiles);

    std::strncpy(pData->aSearchFilter, "/FX/*.txt", sizeof(pData->aSearchFilter) - 1);
    TEST_ASSERT_EQUAL_INT(1, stdFileUtil_FindNext(pData, &info));
    TEST_ASSERT_EQUAL_STRING("a.txt", info.aName);
    TEST_ASSERT_EQUAL_INT(1, pData->nFoundFiles);

    stdFileUtil_DisposeFind(pData);

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FindNext(NULL, &info));
    FindFileData* pGuardData = new FindFileData;
    std::memset(pGuardData, 0, sizeof(*pGuardData));
    std::strncpy(pGuardData->aSearchFilter, "/FX/*.*", sizeof(pGuardData->aSearchFilter) - 1);
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FindNext(pGuardData, NULL));
    stdFileUtil_DisposeFind(pGuardData);
#endif

    stdFileUtil_DisposeFind(NULL);
}

TEST(stdFileUtilBabylon, MkDirFileExistsRmDirDelFile)
{
    // MkDir: 1 only when the parent exists as a directory and nothing exists at the path.
    TEST_ASSERT_EQUAL_INT(1, stdFileUtil_MkDir("/fx2"));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_MkDir("/fx2"));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_MkDir("/missing/x"));

    // Relative and backslash spellings resolve like every other path.
    TEST_ASSERT_EQUAL_INT(1, stdFileUtil_MkDir("fx2\\sub"));
    TEST_ASSERT_TRUE(pFileSystem->exists("/fx2/sub/"));

    TEST_ASSERT_EQUAL_INT(1, stdFileUtil_FileExists("/fx2"));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FileExists("/fx2/nope"));

    // RmDir fails on a not-empty directory and on a file; DelFile fails on a directory.
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_RmDir("/fx2"));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_DelFile("/fx2"));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_RmDir("/fx2/nope"));

    TEST_ASSERT_TRUE(pFileSystem->addFile("/fx2/f.txt", Bytes("x")));
    TEST_ASSERT_EQUAL_INT(1, stdFileUtil_FileExists("/FX2/F.TXT")); // case-insensitive resolve
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_RmDir("/fx2/f.txt"));
    TEST_ASSERT_EQUAL_INT(1, stdFileUtil_DelFile("/fx2/f.txt"));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_DelFile("/fx2/f.txt")); // missing after a delete
    TEST_ASSERT_EQUAL_INT(1, stdFileUtil_RmDir("/fx2/sub"));
    TEST_ASSERT_EQUAL_INT(1, stdFileUtil_RmDir("/fx2"));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FileExists("/fx2"));

    // Any resolve failure: 0.
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_MkDir(NULL));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FileExists(NULL));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_RmDir(NULL));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_DelFile(NULL));
}

TEST_GROUP_RUNNER(stdFileUtilBabylon)
{
    RUN_TEST_CASE(stdFileUtilBabylon, FindNextReportsDotEntriesInSortedOrder);
    RUN_TEST_CASE(stdFileUtilBabylon, FindNextRetriesAfterAFailedSearch);
    RUN_TEST_CASE(stdFileUtilBabylon, MkDirFileExistsRmDirDelFile);
}

static void runAllTests(void)
{
    RUN_TEST_GROUP(stdPlatformTimeBabylon);
    RUN_TEST_GROUP(stdBabylonPath);
    RUN_TEST_GROUP(stdFileBabylon);
    RUN_TEST_GROUP(stdFileUtilBabylon);
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, runAllTests);
}
