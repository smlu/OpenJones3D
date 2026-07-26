#include <unity_fixture.h>

#include <string.h>

#include <std/General/stdFnames.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

TEST_GROUP(stdFnames);

TEST_SETUP(stdFnames)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdFnames)
{
    StdGeneralTest_Shutdown();
}

TEST(stdFnames, TestFindMedName)
{
    TEST_ASSERT_EQUAL_STRING("file.txt", stdFnames_FindMedName("file.txt"));
    TEST_ASSERT_EQUAL_STRING("file.txt", stdFnames_FindMedName("c:\\games\\indy\\file.txt"));
    TEST_ASSERT_EQUAL_STRING("file.txt", stdFnames_FindMedName("c:\\games\\\\\\file.txt"));
    TEST_ASSERT_EQUAL_STRING("", stdFnames_FindMedName("c:\\games\\"));

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_STRING("", stdFnames_FindMedName(NULL));
#endif
}

TEST(stdFnames, TestFindExt)
{
    TEST_ASSERT_EQUAL_STRING("txt", stdFnames_FindExt("c:\\games\\file.txt"));
    TEST_ASSERT_EQUAL_STRING("gz", stdFnames_FindExt("archive.tar.gz"));
    TEST_ASSERT_NULL(stdFnames_FindExt("c:\\games.ext\\file"));
    TEST_ASSERT_NULL(stdFnames_FindExt("file"));

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_NULL(stdFnames_FindExt(NULL));
#endif
}

TEST(stdFnames, TestStripExtAndDot)
{
    char path[64];

    STD_STRCPY(path, "c:\\games\\file.txt");
    stdFnames_StripExtAndDot(path);
    TEST_ASSERT_EQUAL_STRING("c:\\games\\file", path);

    STD_STRCPY(path, "c:\\games.ext\\file");
    stdFnames_StripExtAndDot(path);
    TEST_ASSERT_EQUAL_STRING("c:\\games.ext\\file", path);

    STD_STRCPY(path, "archive.tar.gz");
    stdFnames_StripExtAndDot(path);
    TEST_ASSERT_EQUAL_STRING("archive.tar", path);

#ifdef J3D_RUNTIME_GUARDS
    stdFnames_StripExtAndDot(NULL);
#endif
}

TEST(stdFnames, TestChangeExtEx)
{
    char path[32];
    char smallPath[8];
    const char expectedUnterminated[] = { 'a', 'b', 'c', 'd', '.', 'x', '\0' };
    char unterminated[STD_ARRAYLEN(expectedUnterminated)];

    STD_COPYMEM(unterminated, expectedUnterminated, sizeof(unterminated));

    STD_STRCPY(path, "file.txt");
    stdFnames_ChangeExtEx(path, sizeof(path), "3do");
    TEST_ASSERT_EQUAL_STRING("file.3do", path);

    stdFnames_ChangeExtEx(path, sizeof(path), ".mat");
    TEST_ASSERT_EQUAL_STRING("file.mat", path);

    STD_STRCPY(path, "file");
    stdFnames_ChangeExtEx(path, sizeof(path), "cog");
    TEST_ASSERT_EQUAL_STRING("file.cog", path);

    STD_STRCPY(smallPath, "abc");
    stdFnames_ChangeExtEx(smallPath, sizeof(smallPath), "longext");
    TEST_ASSERT_EQUAL_STRING("abc.lon", smallPath);

    stdFnames_ChangeExtEx(unterminated, 4u, "txt");
    TEST_ASSERT_EQUAL_MEMORY(expectedUnterminated, unterminated, sizeof(unterminated));

#ifdef J3D_RUNTIME_GUARDS
    stdFnames_ChangeExtEx(NULL, sizeof(path), "txt");
    stdFnames_ChangeExtEx(path, sizeof(path), NULL);
    stdFnames_ChangeExtEx(path, 0, "txt");
#endif
}

TEST(stdFnames, TestChangeExtAbiWrapper)
{
    char path[32];

    STD_STRCPY(path, "level.gob");
    stdFnames_ChangeExt(path, "ndy");
    TEST_ASSERT_EQUAL_STRING("level.ndy", path);

#ifdef J3D_RUNTIME_GUARDS
    stdFnames_ChangeExt(NULL, "txt");
    stdFnames_ChangeExt(path, NULL);
#endif
}

TEST(stdFnames, TestConcat)
{
    char path[32];

    STD_STRCPY(path, "c:\\games");
    stdFnames_Concat(path, "indy", sizeof(path));
    TEST_ASSERT_EQUAL_STRING("c:\\games\\indy", path);

    STD_STRCPY(path, "c:\\games\\");
    stdFnames_Concat(path, "\\indy", sizeof(path));
    TEST_ASSERT_EQUAL_STRING("c:\\games\\indy", path);

    STD_STRCPY(path, "");
    stdFnames_Concat(path, "indy", sizeof(path));
    TEST_ASSERT_EQUAL_STRING("indy", path);

    STD_STRCPY(path, "abcdef");
    stdFnames_Concat(path, "ghi", 8);
    TEST_ASSERT_EQUAL_STRING("abcdef\\", path);

#ifdef J3D_RUNTIME_GUARDS
    STD_STRCPY(path, "abcdef");
    stdFnames_Concat(path, "ghi", 0);
    TEST_ASSERT_EQUAL_STRING("abcdef", path);

    stdFnames_Concat(NULL, "x", sizeof(path));
    stdFnames_Concat(path, NULL, sizeof(path));
#endif
}

TEST(stdFnames, TestMakePath)
{
    char path[32];

    stdFnames_MakePath(path, sizeof(path), "c:\\games", "indy");
    TEST_ASSERT_EQUAL_STRING("c:\\games\\indy", path);

    stdFnames_MakePath(path, sizeof(path), NULL, "indy");
    TEST_ASSERT_EQUAL_STRING("indy", path);

    stdFnames_MakePath(path, sizeof(path), "c:\\games", NULL);
    TEST_ASSERT_EQUAL_STRING("c:\\games", path);

    STD_MAKEPATH(path, "base", "child");
    TEST_ASSERT_EQUAL_STRING("base\\child", path);

#ifdef J3D_RUNTIME_GUARDS
    STD_STRCPY(path, "unchanged");
    stdFnames_MakePath(NULL, sizeof(path), "base", "child");
    stdFnames_MakePath(path, 0, "base", "child");
    TEST_ASSERT_EQUAL_STRING("unchanged", path);
#endif
}

TEST_GROUP_RUNNER(stdFnames)
{
    RUN_TEST_CASE(stdFnames, TestFindMedName);
    RUN_TEST_CASE(stdFnames, TestFindExt);
    RUN_TEST_CASE(stdFnames, TestStripExtAndDot);
    RUN_TEST_CASE(stdFnames, TestChangeExtEx);
    RUN_TEST_CASE(stdFnames, TestChangeExtAbiWrapper);
    RUN_TEST_CASE(stdFnames, TestConcat);
    RUN_TEST_CASE(stdFnames, TestMakePath);
}
