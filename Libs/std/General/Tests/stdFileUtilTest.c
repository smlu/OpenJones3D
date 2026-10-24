#include <unity_fixture.h>

#include <string.h>
#include <time.h>

#include <std/General/std.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

static const char* stdFileUtilTest_pDir    = "stdFileUtilTest_dir";
static const char* stdFileUtilTest_pSubdir = "stdFileUtilTest_dir\\child";

static void stdFileUtilTest_DeleteFile(const char* pPath)
{
    StdGeneralTest_DeleteFile(pPath);
}

static void stdFileUtilTest_RemoveDir(void)
{
    stdFileUtil_RmDir(stdFileUtilTest_pSubdir);
    stdFileUtil_RmDir(stdFileUtilTest_pDir);
}

static void stdFileUtilTest_MakeFile(const char* pPath, const char* pText)
{
    tFileHandle fh;
    size_t len;

    stdFileUtilTest_DeleteFile(pPath);

    fh = stdFileOpen(pPath, "wb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    len = strlen(pText);
    TEST_ASSERT_EQUAL_size_t(len, stdFileWrite(fh, pText, len));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
}

static bool stdFileUtilTest_NameHasSuffix(const char* pName, const char* pSuffix)
{
    size_t nameLen = strlen(pName);
    size_t suffixLen = strlen(pSuffix);

    return nameLen >= suffixLen && strcmp(&pName[nameLen - suffixLen], pSuffix) == 0;
}

TEST_GROUP(stdFileUtil);

TEST_SETUP(stdFileUtil)
{
    StdGeneralTest_Startup();
    stdFileUtilTest_RemoveDir();
}

TEST_TEAR_DOWN(stdFileUtil)
{
    stdFileUtilTest_DeleteFile("stdFileUtilTest_dir\\a.txt");
    stdFileUtilTest_DeleteFile("stdFileUtilTest_dir\\b.txt");
    stdFileUtilTest_DeleteFile("stdFileUtilTest_dir\\c.bin");
    stdFileUtilTest_RemoveDir();
    StdGeneralTest_Shutdown();
}

TEST(stdFileUtil, TestNewFindBuildsFiltersForModes)
{
    FindFileData* pFind;

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_NULL(stdFileUtil_NewFind(NULL, 0, NULL));
#endif

    pFind = stdFileUtil_NewFind("base", -1, NULL);
    TEST_ASSERT_NOT_NULL(pFind);
    TEST_ASSERT_EQUAL_STRING("", pFind->aSearchFilter);
    stdFileUtil_DisposeFind(pFind);

    pFind = stdFileUtil_NewFind("base", 0, NULL);
    TEST_ASSERT_NOT_NULL(pFind);
    TEST_ASSERT_EQUAL_STRING("base\\*.*", pFind->aSearchFilter);
    stdFileUtil_DisposeFind(pFind);

    pFind = stdFileUtil_NewFind("base", 3, ".txt");
    TEST_ASSERT_NOT_NULL(pFind);
    TEST_ASSERT_EQUAL_STRING("base\\*.txt", pFind->aSearchFilter);
    stdFileUtil_DisposeFind(pFind);

    pFind = stdFileUtil_NewFind("base", 3, NULL);
    TEST_ASSERT_NOT_NULL(pFind);
    TEST_ASSERT_EQUAL_STRING("base\\*.*", pFind->aSearchFilter);
    stdFileUtil_DisposeFind(pFind);

    pFind = stdFileUtil_NewFind("base", 99, "txt");
    TEST_ASSERT_NOT_NULL(pFind);
    TEST_ASSERT_EQUAL_STRING("", pFind->aSearchFilter);
    stdFileUtil_DisposeFind(pFind);

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_NULL(stdFileUtil_NewFind("base", 0, NULL));
    StdGeneralTest_ClearAllocationFailures();
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);

    stdFileUtil_DisposeFind(NULL);
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FindNext(NULL, NULL));
}

TEST(stdFileUtil, TestDirectoryFileExistenceAndDeleteHelpers)
{
    TEST_ASSERT_FALSE(stdFileUtil_FileExists("stdFileUtilTest_missing.tmp"));
    TEST_ASSERT_FALSE(stdFileUtil_DelFile("stdFileUtilTest_missing.tmp"));
    TEST_ASSERT_FALSE(stdFileUtil_RmDir("stdFileUtilTest_missing_dir"));
    TEST_ASSERT_TRUE(stdFileUtil_MkDir(stdFileUtilTest_pDir));
    TEST_ASSERT_FALSE(stdFileUtil_MkDir(stdFileUtilTest_pDir));
    TEST_ASSERT_TRUE(stdFileUtil_FileExists(stdFileUtilTest_pDir));
    TEST_ASSERT_TRUE(stdUtil_DirExists(stdFileUtilTest_pDir));
    TEST_ASSERT_FALSE(stdUtil_DirExists("stdFileUtilTest_missing_dir"));

    stdFileUtilTest_MakeFile("stdFileUtilTest_dir\\a.txt", "a");
    TEST_ASSERT_TRUE(stdFileUtil_FileExists("stdFileUtilTest_dir\\a.txt"));
    TEST_ASSERT_FALSE(stdFileUtil_RmDir(stdFileUtilTest_pDir));
    TEST_ASSERT_TRUE(stdFileUtil_DelFile("stdFileUtilTest_dir\\a.txt"));
    TEST_ASSERT_FALSE(stdFileUtil_FileExists("stdFileUtilTest_dir\\a.txt"));
    TEST_ASSERT_TRUE(stdFileUtil_RmDir(stdFileUtilTest_pDir));
}

TEST(stdFileUtil, TestFindNextFindQuickAndCountMatches)
{
    FindFileData* pFind;
    tFoundFileInfo info;
    bool bFoundDirectory = false;
    bool bFoundFile = false;
    int numTxtFiles = 0;

    TEST_ASSERT_TRUE(stdFileUtil_MkDir(stdFileUtilTest_pDir));
    TEST_ASSERT_TRUE(stdFileUtil_MkDir(stdFileUtilTest_pSubdir));
    stdFileUtilTest_MakeFile("stdFileUtilTest_dir\\a.txt", "a");
    stdFileUtilTest_MakeFile("stdFileUtilTest_dir\\b.txt", "b");
    stdFileUtilTest_MakeFile("stdFileUtilTest_dir\\c.bin", "c");

    TEST_ASSERT_EQUAL_INT(2, stdFileUtil_CountMatches(stdFileUtilTest_pDir, 3, ".txt"));
    TEST_ASSERT_EQUAL_INT(1, stdFileUtil_CountMatches(stdFileUtilTest_pDir, 3, "bin"));
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_CountMatches("stdFileUtilTest_missing_dir", 3, "txt"));
    TEST_ASSERT_FALSE(stdFileUtil_FindQuick("stdFileUtilTest_missing_dir", 3, "txt", &info));

    STD_ZEROMEM(&info, sizeof(info));
    TEST_ASSERT_TRUE(stdFileUtil_FindQuick(stdFileUtilTest_pDir, 3, ".txt", &info));
    TEST_ASSERT_TRUE(stdFileUtilTest_NameHasSuffix(info.aName, ".txt"));
    TEST_ASSERT_FALSE(info.bIsDirectory);
    TEST_ASSERT_NOT_EQUAL(0u, info.lastChanged);
    time_t currentTime = time(NULL);
    TEST_ASSERT_TRUE(info.lastChanged <= (uint32_t)(currentTime + 2));
    TEST_ASSERT_TRUE(info.lastChanged >= (uint32_t)(currentTime - 10));

    pFind = stdFileUtil_NewFind(stdFileUtilTest_pDir, 3, ".txt");
    TEST_ASSERT_NOT_NULL(pFind);
    while ( stdFileUtil_FindNext(pFind, &info) )
    {
        TEST_ASSERT_TRUE(stdFileUtilTest_NameHasSuffix(info.aName, ".txt"));
        ++numTxtFiles;
    }
    TEST_ASSERT_EQUAL_INT(2, numTxtFiles);
    stdFileUtil_DisposeFind(pFind);

    pFind = stdFileUtil_NewFind(stdFileUtilTest_pDir, 0, NULL);
    TEST_ASSERT_NOT_NULL(pFind);
#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FindNext(pFind, NULL));
    TEST_ASSERT_EQUAL_INT(0, pFind->nFoundFiles);
#endif
    while ( stdFileUtil_FindNext(pFind, &info) )
    {
        if ( strcmp(info.aName, "child") == 0 )
        {
            TEST_ASSERT_TRUE(info.bIsDirectory);
            bFoundDirectory = true;
        }
        else if ( strcmp(info.aName, "a.txt") == 0 )
        {
            TEST_ASSERT_FALSE(info.bIsDirectory);
            bFoundFile = true;
        }
    }
    TEST_ASSERT_TRUE(bFoundDirectory);
    TEST_ASSERT_TRUE(bFoundFile);
    stdFileUtil_DisposeFind(pFind);

    pFind = stdFileUtil_NewFind("stdFileUtilTest_missing_dir", 0, NULL);
    TEST_ASSERT_NOT_NULL(pFind);
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FindNext(pFind, &info));
    TEST_ASSERT_EQUAL_INT(0, pFind->nFoundFiles);
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FindNext(pFind, &info));
    TEST_ASSERT_EQUAL_INT(0, pFind->nFoundFiles);
    stdFileUtil_DisposeFind(pFind);
}

TEST(stdFileUtil, TestFindHelpersHandleAllocationFailure)
{
    tFoundFileInfo info;

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_FindQuick(".", 0, NULL, &info));
    StdGeneralTest_ClearAllocationFailures();
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_EQUAL_INT(0, stdFileUtil_CountMatches(".", 0, NULL));
    StdGeneralTest_ClearAllocationFailures();
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
}

TEST_GROUP_RUNNER(stdFileUtil)
{
    RUN_TEST_CASE(stdFileUtil, TestNewFindBuildsFiltersForModes);
    RUN_TEST_CASE(stdFileUtil, TestDirectoryFileExistenceAndDeleteHelpers);
    RUN_TEST_CASE(stdFileUtil, TestFindNextFindQuickAndCountMatches);
    RUN_TEST_CASE(stdFileUtil, TestFindHelpersHandleAllocationFailure);
}
