#include <unity_fixture.h>

#include <string.h>
#include <wchar.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdStrTable.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

TEST_GROUP(stdStrTable);

static void stdStrTableTest_AssertEmpty(const tStringTable* pTable)
{
    TEST_ASSERT_EQUAL_UINT32(0u, pTable->nMsgs);
    TEST_ASSERT_NULL(pTable->pData);
    TEST_ASSERT_NULL(pTable->pHashtbl);
    TEST_ASSERT_EQUAL_INT(0, pTable->magic);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
}

TEST_SETUP(stdStrTable)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdStrTable)
{
    StdGeneralTest_DeleteFile("stdStrTableTest_no_newline.tmp");
    StdGeneralTest_Shutdown();
}

TEST(stdStrTable, TestParseLiteralReturnsBoundsAndNextPosition)
{
    char* pBegin = NULL;
    char* pEnd   = NULL;
    char* pNext;

    pNext = stdStrTable_ParseLiteral("prefix \"Temple\" suffix", &pBegin, &pEnd);
    TEST_ASSERT_NULL(pNext);
    TEST_ASSERT_EQUAL_MEMORY("Temple", pBegin, 6u);
    TEST_ASSERT_EQUAL_STRING("\" suffix", pEnd);
    TEST_ASSERT_EQUAL_INT(6, (int)(pEnd - pBegin));

    pBegin = NULL;
    pEnd = NULL;
    pNext = stdStrTable_ParseLiteral("\"unterminated", &pBegin, &pEnd);
    TEST_ASSERT_NULL(pNext);
    TEST_ASSERT_EQUAL_STRING("unterminated", pBegin);
    TEST_ASSERT_NULL(pEnd);

    pBegin = NULL;
    pEnd = NULL;
    pNext = stdStrTable_ParseLiteral("no literal", &pBegin, &pEnd);
    TEST_ASSERT_NULL(pNext);
    TEST_ASSERT_NULL(pBegin);
    TEST_ASSERT_NULL(pEnd);
}

TEST(stdStrTable, TestReadLineSkipsCommentsBlankLinesAndDiscardsLongRemainder)
{
    const char* pPath          = StdGeneralTest_GetTestVectorPath("stdStrTable/long_lines.tbl");
    const char* pNoNewlinePath = "stdStrTableTest_no_newline.tmp";
    char aLine[8];
    tFileHandle fh;

    fh = stdFileOpen(pPath, "rt");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_INT(1, stdStrTable_ReadLine(fh, aLine, sizeof(aLine)));
    TEST_ASSERT_EQUAL_STRING("1234567", aLine);
    TEST_ASSERT_EQUAL_INT(1, stdStrTable_ReadLine(fh, aLine, sizeof(aLine)));
    TEST_ASSERT_EQUAL_STRING("next\n", aLine);
    TEST_ASSERT_EQUAL_INT(0, stdStrTable_ReadLine(fh, aLine, sizeof(aLine)));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    fh = stdFileOpen(pNoNewlinePath, "wb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(5u, stdFileWrite(fh, "final", 5u));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    fh = stdFileOpen(pNoNewlinePath, "rt");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_INT(1, stdStrTable_ReadLine(fh, aLine, sizeof(aLine)));
    TEST_ASSERT_EQUAL_STRING("final", aLine);
    TEST_ASSERT_EQUAL_INT(0, stdStrTable_ReadLine(fh, aLine, sizeof(aLine)));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
}

TEST(stdStrTable, TestLoadGetValueGetValueOrKeyAndFree)
{
    const char* pPath = StdGeneralTest_GetTestVectorPath("stdStrTable/messages.tbl");
    tStringTable table;
    wchar_t* pValue;

    STD_ZEROMEM(&table, sizeof(table));

    TEST_ASSERT_EQUAL_INT(1, stdStrTable_Load(&table, pPath));
    TEST_ASSERT_EQUAL_UINT32(2u, table.nMsgs);
    TEST_ASSERT_EQUAL_INT('sTbl', table.magic);

    pValue = stdStrTable_GetValue(&table, "KEY_ONE");
    TEST_ASSERT_NOT_NULL(pValue);
    TEST_ASSERT_EQUAL(0, wcscmp(L"Value One", pValue));

    pValue = stdStrTable_GetValue(&table, "KEY_TWO");
    TEST_ASSERT_NOT_NULL(pValue);
    TEST_ASSERT_EQUAL(0, wcscmp(L"Value Two", pValue));
    TEST_ASSERT_EQUAL_INT(0, table.pData[0].unknown);
    TEST_ASSERT_EQUAL_INT(3, table.pData[1].unknown);

    TEST_ASSERT_NULL(stdStrTable_GetValue(&table, "MISSING"));
    pValue = stdStrTable_GetValueOrKey(&table, "MISSING");
    TEST_ASSERT_NOT_NULL(pValue);
    TEST_ASSERT_EQUAL(0, wcscmp(L"MISSING", pValue));

    stdStrTable_Free(&table);
    stdStrTableTest_AssertEmpty(&table);
}

TEST(stdStrTable, TestLoadAcceptsEmptyTable)
{
    tStringTable table;

    STD_ZEROMEM(&table, sizeof(table));
    TEST_ASSERT_EQUAL_INT(1, stdStrTable_Load(&table, StdGeneralTest_GetTestVectorPath("stdStrTable/empty.tbl")));
    TEST_ASSERT_EQUAL_UINT32(0u, table.nMsgs);
    TEST_ASSERT_NOT_NULL(table.pHashtbl);
    TEST_ASSERT_EQUAL_INT('sTbl', table.magic);
    TEST_ASSERT_NULL(stdStrTable_GetValue(&table, "MISSING"));

    stdStrTable_Free(&table);
    stdStrTableTest_AssertEmpty(&table);
}

TEST(stdStrTable, TestLoadParsesSignedDecimalNumericFields)
{
    tStringTable table;

    STD_ZEROMEM(&table, sizeof(table));
    TEST_ASSERT_EQUAL_INT(1, stdStrTable_Load(&table, StdGeneralTest_GetTestVectorPath("stdStrTable/numeric_fields.tbl")));
    TEST_ASSERT_EQUAL_INT(0, table.pData[0].unknown);
    TEST_ASSERT_EQUAL_INT(-7, table.pData[1].unknown);
    TEST_ASSERT_EQUAL_INT(42, table.pData[2].unknown);

    stdStrTable_Free(&table);
    stdStrTableTest_AssertEmpty(&table);
}

TEST(stdStrTable, TestLoadRejectsMalformedFilesAndCleansState)
{
    static const char* aMalformedFiles[] = {
        "stdStrTable/bad_header.tbl",
        "stdStrTable/missing_msg_line.tbl",
        "stdStrTable/msg_missing_number.tbl",
        "stdStrTable/msg_letter_number.tbl",
        "stdStrTable/negative_count.tbl",
        "stdStrTable/huge_count.tbl",
        "stdStrTable/premature_end.tbl",
        "stdStrTable/missing_entry_eof.tbl",
        "stdStrTable/missing_end.tbl",
        "stdStrTable/msg_entry_missing_value.tbl",
        "stdStrTable/malformed_numeric_field.tbl",
        "stdStrTable/hex_numeric_field.tbl",
        "stdStrTable/empty_key.tbl",
        "stdStrTable/duplicate_keys.tbl"
    };
    tStringTable table;

    for ( size_t i = 0; i < STD_ARRAYLEN(aMalformedFiles); ++i )
    {
        STD_ZEROMEM(&table, sizeof(table));
        TEST_ASSERT_EQUAL_INT(0, stdStrTable_Load(&table, StdGeneralTest_GetTestVectorPath(aMalformedFiles[i])));
        stdStrTableTest_AssertEmpty(&table);
    }

    STD_ZEROMEM(&table, sizeof(table));
    TEST_ASSERT_EQUAL_INT(0, stdStrTable_Load(&table, "stdStrTableTest_missing.tbl"));
    stdStrTableTest_AssertEmpty(&table);
}

TEST(stdStrTable, TestLoadCleansEveryAllocationFailure)
{
    const char* pPath = StdGeneralTest_GetTestVectorPath("stdStrTable/messages.tbl");

    // Inject failure at each allocation and verify cleanup plus the exact diagnostic or fatal callback.
    for ( int allocation = 0; allocation <= 6; ++allocation )
    {
        tStringTable table;
        STD_ZEROMEM(&table, sizeof(table));

        J3DTest_ResetAssertCapture();
        StdGeneralTest_ResetDebugOutput();
        StdGeneralTest_FailNextAllocations(allocation);
        TEST_ASSERT_EQUAL_INT(0, stdStrTable_Load(&table, pPath));
        StdGeneralTest_ClearAllocationFailures();

        int expectedAsserts = (allocation == 4 || allocation == 6) ? 2 : 1;
        TEST_ASSERT_EQUAL_INT(expectedAsserts, J3DTest_GetAssertCount());

        if ( allocation <= 2 )
        {
            const char* pExpectedAssert = allocation == 0
                ? "pStrTable->pData != NULL"
                : "pStrTable->pHashtbl != NULL";
            const char* pExpectedLog = allocation == 0
                ? "stdStrTable.c(84): Out of memory--cannot load string table"
                : "stdStrTable.c(96): Out of memory--cannot load string table";

            TEST_ASSERT_EQUAL_STRING(pExpectedLog, StdGeneralTest_GetDebugOutput());
            StdGeneralTest_AssertLastAssert(pExpectedAssert, "stdStrTable.c");
        }

        if ( allocation > 2 )
        {
            TEST_ASSERT_EQUAL_STRING("Out of memory--cannot load string table", J3DTest_GetLastAssertText());
            TEST_ASSERT_EQUAL_STRING("stdStrTable.c", J3DTest_GetLastAssertFile());
        }
        stdStrTableTest_AssertEmpty(&table);
    }
}

TEST(stdStrTable, TestLoadCleansEveryFileReadFailure)
{
    const char* pPath = StdGeneralTest_GetTestVectorPath("stdStrTable/messages.tbl");

    // Header, two entries, and trailing END are four separate line reads.
    for ( int successfulReads = 0; successfulReads < 4; ++successfulReads )
    {
        tStringTable table;

        STD_ZEROMEM(&table, sizeof(table));
        StdGeneralTest_FailNextFileGets(successfulReads);
        TEST_ASSERT_EQUAL_INT(0, stdStrTable_Load(&table, pPath));
        StdGeneralTest_ClearFileFailures();
        stdStrTableTest_AssertEmpty(&table);
    }
}

TEST(stdStrTable, TestFreePreservesAssertAndGuardContract)
{
#ifdef J3D_RUNTIME_GUARDS
    J3DTest_ResetAssertCapture();
    stdStrTable_Free(NULL);
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
#endif
}

TEST_GROUP_RUNNER(stdStrTable)
{
    RUN_TEST_CASE(stdStrTable, TestParseLiteralReturnsBoundsAndNextPosition);
    RUN_TEST_CASE(stdStrTable, TestReadLineSkipsCommentsBlankLinesAndDiscardsLongRemainder);
    RUN_TEST_CASE(stdStrTable, TestLoadGetValueGetValueOrKeyAndFree);
    RUN_TEST_CASE(stdStrTable, TestLoadAcceptsEmptyTable);
    RUN_TEST_CASE(stdStrTable, TestLoadParsesSignedDecimalNumericFields);
    RUN_TEST_CASE(stdStrTable, TestLoadRejectsMalformedFilesAndCleansState);
    RUN_TEST_CASE(stdStrTable, TestLoadCleansEveryAllocationFailure);
    RUN_TEST_CASE(stdStrTable, TestLoadCleansEveryFileReadFailure);
    RUN_TEST_CASE(stdStrTable, TestFreePreservesAssertAndGuardContract);
}
