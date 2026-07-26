#include <unity_fixture.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

TEST_GROUP(stdUtil);

TEST_SETUP(stdUtil)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdUtil)
{
    StdGeneralTest_Shutdown();
}

TEST(stdUtil, TestArrayAndMemoryMacros)
{
    int values[4] = { 1, 2, 3, 4 };
    char text[8];
    const char zeroes[sizeof(text)] = { 0 };
    char moved[8] = "abcdef";

    TEST_ASSERT_EQUAL_size_t(4u, STD_ARRAYLEN(values));
    TEST_ASSERT_TRUE(STD_ISSTRARRAY(text));
    const char aConstText[] = "const";
    const wchar_t awConstText[] = L"const";
    char* pText = text;
    wchar_t awText[8];
    TEST_ASSERT_TRUE(STD_ISSTRARRAY(aConstText));
    TEST_ASSERT_TRUE(STD_ISWSTRARRAY(awText));
    TEST_ASSERT_TRUE(STD_ISWSTRARRAY(awConstText));
    TEST_ASSERT_FALSE(STD_ISSTRARRAY(pText));
    TEST_ASSERT_FALSE(STD_ISSTRARRAY(values));
    TEST_ASSERT_FALSE(STD_ISSTRARRAY(awText));
    TEST_ASSERT_FALSE(STD_ISWSTRARRAY(text));

    STD_FILLMEM(text, 'x', sizeof(text));
    TEST_ASSERT_EACH_EQUAL_UINT8('x', text, sizeof(text));

    STD_ZEROMEM(text, sizeof(text));
    TEST_ASSERT_EQUAL_MEMORY(zeroes, text, sizeof(text));

    STD_COPYMEM(text, "abcd", 5);
    TEST_ASSERT_EQUAL_STRING("abcd", text);

    STD_MOVEMEM(&moved[1], moved, 5);
    TEST_ASSERT_EQUAL_STRING("aabcde", moved);
    TEST_ASSERT_TRUE(STD_EQUALMEM("aabc", moved, 4));
}

TEST(stdUtil, TestStringCopyAndConcatHelpers)
{
    char text[16];
    wchar_t wtext[16];

    TEST_ASSERT_TRUE(stdUtil_StringCopy(text, sizeof(text), "Temple"));
    TEST_ASSERT_EQUAL_STRING("Temple", text);

    TEST_ASSERT_TRUE(stdUtil_StringNumCopy(text, sizeof(text), "Infernal", 3));
    TEST_ASSERT_EQUAL_STRING("Inf", text);

    TEST_ASSERT_TRUE(stdUtil_StringCat(text, sizeof(text), "erno"));
    TEST_ASSERT_EQUAL_STRING("Inferno", text);

    TEST_ASSERT_TRUE(stdUtil_StringNumCat(text, sizeof(text), "Machine", 3));
    TEST_ASSERT_EQUAL_STRING("InfernoMac", text);

    TEST_ASSERT_TRUE(stdUtil_WStringCopy(wtext, STD_ARRAYLEN(wtext), L"Indy"));
    TEST_ASSERT_EQUAL(0, wcscmp(L"Indy", wtext));

    TEST_ASSERT_TRUE(stdUtil_WStringNumCopy(wtext, STD_ARRAYLEN(wtext), L"Jones", 4));
    TEST_ASSERT_EQUAL(0, wcscmp(L"Jone", wtext));
}

TEST(stdUtil, TestFormatSpecifiers)
{
    char text[128];
    wchar_t wtext[128];
    int result;

    result = stdUtil_Format(text, sizeof(text), "%d|%i|%u", -12, 34, 4000000000u);
    TEST_ASSERT_EQUAL_STRING("-12|34|4000000000", text);
    TEST_ASSERT_EQUAL_INT((int)strlen(text), result);

    result = stdUtil_Format(text, sizeof(text), "%x|%X|%#x|%#X", 48879u, 48879u, 48879u, 48879u);
    TEST_ASSERT_EQUAL_STRING("beef|BEEF|0xbeef|0XBEEF", text);
    TEST_ASSERT_EQUAL_INT((int)strlen(text), result);

    result = stdUtil_Format(text, sizeof(text), "%o|%#o", 10u, 10u);
    TEST_ASSERT_EQUAL_STRING("12|012", text);
    TEST_ASSERT_EQUAL_INT((int)strlen(text), result);

    result = stdUtil_Format(text, sizeof(text), "%+d|% d|%04d|%-5s|%.3s|%c|%%", 7, 7, 7, "hi", "abcdef", 'Z');
    TEST_ASSERT_EQUAL_STRING("+7| 7|0007|hi   |abc|Z|%", text);
    TEST_ASSERT_EQUAL_INT((int)strlen(text), result);

    result = stdUtil_Format(text, sizeof(text), "%.2f", 3.5);
    TEST_ASSERT_EQUAL_STRING("3.50", text);
    TEST_ASSERT_EQUAL_INT((int)strlen(text), result);

    result = stdUtil_WFormat(wtext, STD_ARRAYLEN(wtext), L"%ls|%d|%#X|%04d|%%", L"wide", -7, 48879u, 5);
    TEST_ASSERT_EQUAL(0, wcscmp(L"wide|-7|0XBEEF|0005|%", wtext));
    TEST_ASSERT_EQUAL_INT((int)wcslen(wtext), result);
}

TEST(stdUtil, TestArrayConvenienceMacros)
{
    char text[32];
    wchar_t wtext[32];

    STD_FORMAT(text, "%s %d", "Room", 17);
    TEST_ASSERT_EQUAL_STRING("Room 17", text);

    STD_STRCPY(text, "indy");
    TEST_ASSERT_EQUAL_STRING("indy", text);

    STD_STRNCPY(text, "treasure", 5);
    TEST_ASSERT_EQUAL_STRING("treas", text);

    STD_STRCAT(text, " map");
    TEST_ASSERT_EQUAL_STRING("treas map", text);

    STD_WFORMAT(wtext, L"%ls %d", L"Room", 17);
    TEST_ASSERT_EQUAL(0, wcscmp(L"Room 17", wtext));

    STD_WSTRCPY(wtext, L"indy");
    TEST_ASSERT_EQUAL(0, wcscmp(L"indy", wtext));

    STD_TOWSTR(wtext, "abc");
    TEST_ASSERT_EQUAL(0, wcscmp(L"abc", wtext));

    STD_TOSTR(text, L"xyz");
    TEST_ASSERT_EQUAL_STRING("xyz", text);
}

TEST(stdUtil, TestFormatGuards)
{
    char text[16] = "unchanged";
    wchar_t wtext[16] = L"unchanged";

    TEST_ASSERT_EQUAL_INT(6, stdUtil_Format(text, sizeof(text), "%s", "format"));
    TEST_ASSERT_EQUAL_STRING("format", text);

    TEST_ASSERT_EQUAL_INT(4, stdUtil_WFormat(wtext, STD_ARRAYLEN(wtext), L"%ls", L"wide"));
    TEST_ASSERT_EQUAL(0, wcscmp(L"wide", wtext));

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_INT(-1, stdUtil_Format(NULL, sizeof(text), "%s", "bad"));
    TEST_ASSERT_EQUAL_INT(-1, stdUtil_Format(text, 0, "%s", "bad"));
    TEST_ASSERT_EQUAL_INT(-1, stdUtil_Format(text, sizeof(text), NULL));
    TEST_ASSERT_EQUAL_INT(-1, stdUtil_WFormat(NULL, STD_ARRAYLEN(wtext), L"%ls", L"bad"));
    TEST_ASSERT_EQUAL_INT(-1, stdUtil_WFormat(wtext, 0, L"%ls", L"bad"));
    TEST_ASSERT_EQUAL_INT(-1, stdUtil_WFormat(wtext, STD_ARRAYLEN(wtext), NULL));
#endif
}

TEST(stdUtil, TestFormatCapacityBoundaries)
{
    char text[5] = { 'x', 'x', 'x', 'x', 'x' };
    wchar_t wtext[5] = { L'x', L'x', L'x', L'x', L'x' };

    TEST_ASSERT_EQUAL_INT(4, stdUtil_Format(text, STD_ARRAYLEN(text), "%s", "1234"));
    TEST_ASSERT_EQUAL_STRING("1234", text);
    TEST_ASSERT_EQUAL_INT(-1, stdUtil_Format(text, STD_ARRAYLEN(text), "%s", "12345"));
    TEST_ASSERT_EQUAL_STRING("1234", text);
    TEST_ASSERT_EQUAL_INT('\0', text[STD_ARRAYLEN(text) - 1u]);

    TEST_ASSERT_EQUAL_INT(4, stdUtil_WFormat(wtext, STD_ARRAYLEN(wtext), L"%ls", L"1234"));
    TEST_ASSERT_EQUAL(0, wcscmp(L"1234", wtext));
    TEST_ASSERT_EQUAL_INT(-1, stdUtil_WFormat(wtext, STD_ARRAYLEN(wtext), L"%ls", L"12345"));
    TEST_ASSERT_EQUAL(0, wcscmp(L"1234", wtext));
    TEST_ASSERT_EQUAL_INT(L'\0', wtext[STD_ARRAYLEN(wtext) - 1u]);
}

TEST(stdUtil, TestStringSplit)
{
    char token[16];
    char* pNext;

    pNext = stdUtil_StringSplit(",,,alpha,beta", token, sizeof(token), ",");
    TEST_ASSERT_EQUAL_STRING("alpha", token);
    TEST_ASSERT_NOT_NULL(pNext);
    TEST_ASSERT_EQUAL_STRING(",beta", pNext);

    pNext = stdUtil_StringSplit("   alpha", token, sizeof(token), " ");
    TEST_ASSERT_EQUAL_STRING("alpha", token);
    TEST_ASSERT_NULL(pNext);

    pNext = stdUtil_StringSplit("alpha,beta", NULL, 0, ",");
    TEST_ASSERT_NOT_NULL(pNext);
    TEST_ASSERT_EQUAL_STRING(",beta", pNext);

    pNext = stdUtil_StringSplit("abcdef", token, 4, ",");
    TEST_ASSERT_EQUAL_STRING("abc", token);
    TEST_ASSERT_NULL(pNext);

    pNext = stdUtil_StringSplit(",,,", token, sizeof(token), ",");
    TEST_ASSERT_NULL(pNext);
    TEST_ASSERT_EQUAL_STRING("", token);
    pNext = stdUtil_StringSplit("", token, sizeof(token), ",");
    TEST_ASSERT_NULL(pNext);
    TEST_ASSERT_EQUAL_STRING("", token);
}

TEST(stdUtil, TestParseLiteral)
{
    char literal[16];
    char* pEnd;

    pEnd = stdUtil_ParseLiteral("prefix \"Temple\" suffix", literal, sizeof(literal));
    TEST_ASSERT_EQUAL_STRING("Temple", literal);
    TEST_ASSERT_NOT_NULL(pEnd);
    TEST_ASSERT_EQUAL_STRING(" suffix", pEnd);

    pEnd = stdUtil_ParseLiteral("prefix \"TooLong\"", literal, 5);
    TEST_ASSERT_EQUAL_STRING("TooL", literal);
    TEST_ASSERT_NOT_NULL(pEnd);
    TEST_ASSERT_EQUAL_STRING("", pEnd);

    pEnd = stdUtil_ParseLiteral("prefix \"Temple\" suffix", NULL, 0);
    TEST_ASSERT_NOT_NULL(pEnd);
    TEST_ASSERT_EQUAL_STRING(" suffix", pEnd);

    STD_STRCPY(literal, "x");
    pEnd = stdUtil_ParseLiteral("no literal", literal, sizeof(literal));
    TEST_ASSERT_NULL(pEnd);
    TEST_ASSERT_EQUAL_STRING("", literal);

    pEnd = stdUtil_ParseLiteral("\"unterminated", literal, sizeof(literal));
    TEST_ASSERT_NULL(pEnd);
    TEST_ASSERT_EQUAL_STRING("", literal);
}

TEST(stdUtil, TestWideAndAnsiConversions)
{
    wchar_t wtext[8] = { 0 };
    char text[8] = { 0 };
    wchar_t* pWide;
    char* pAnsi;
    const wchar_t sourceWithHighChar[] = { L'A', 0x0100, L'B', 0 };

    TEST_ASSERT_EQUAL_INT(3, stdUtil_ToWStringEx(wtext, "abc", 8));
    TEST_ASSERT_EQUAL(0, wcscmp(L"abc", wtext));

    memset(wtext, 0x7f, sizeof(wtext));
    TEST_ASSERT_EQUAL_INT(3, stdUtil_ToWStringEx(wtext, "abcdef", 3));
    TEST_ASSERT_EQUAL_INT(L'a', wtext[0]);
    TEST_ASSERT_EQUAL_INT(L'c', wtext[2]);

    stdUtil_ToAStringEx(text, L"abc", sizeof(text));
    TEST_ASSERT_EQUAL_STRING("abc", text);

    stdUtil_ToAStringEx(text, sourceWithHighChar, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("A?B", text);

    pWide = stdUtil_ToWString("Indy");
    TEST_ASSERT_NOT_NULL(pWide);
    TEST_ASSERT_EQUAL(0, wcscmp(L"Indy", pWide));
    STDFREE(pWide);

    pAnsi = stdUtil_ToAString(L"Jones");
    TEST_ASSERT_NOT_NULL(pAnsi);
    TEST_ASSERT_EQUAL_STRING("Jones", pAnsi);
    STDFREE(pAnsi);
}

TEST(stdUtil, TestCaseConversionAndCompare)
{
    char text[16] = "AbC123!";

    stdUtil_ToLower(text);
    TEST_ASSERT_EQUAL_STRING("abc123!", text);

    stdUtil_ToUpper(text);
    TEST_ASSERT_EQUAL_STRING("ABC123!", text);

    TEST_ASSERT_EQUAL_INT(0, stdUtil_StrCmp("Indy", "indy"));
    TEST_ASSERT_TRUE(stdUtil_StrCmp("abc", "abd") < 0);
    TEST_ASSERT_TRUE(stdUtil_StrCmp("abd", "abc") > 0);
    TEST_ASSERT_TRUE(stdUtil_StrCmp("abc", "abcd") < 0);

#ifdef J3D_RUNTIME_GUARDS
    stdUtil_ToLower(NULL);
    stdUtil_ToUpper(NULL);
    TEST_ASSERT_EQUAL_INT(0, J3DTest_GetAssertCount());
    TEST_ASSERT_EQUAL_INT(-1, stdUtil_StrCmp(NULL, "abc"));
    TEST_ASSERT_EQUAL_INT(1, stdUtil_StrCmp("abc", NULL));
    TEST_ASSERT_EQUAL_INT(0, stdUtil_StrCmp(NULL, NULL));
#endif
}

TEST(stdUtil, TestDuplicateFileExistsAndChecksum)
{
    char* pDup;
    const uint8_t bytes[] = { 1u, 2u, 3u };

    pDup = stdUtil_StringDuplicate("copy", StdGeneralTest_GetHostServices());
    TEST_ASSERT_NOT_NULL(pDup);
    TEST_ASSERT_EQUAL_STRING("copy", pDup);
    STDFREE(pDup);

    StdGeneralTest_SetExistingFileName("exists.dat");
    TEST_ASSERT_TRUE(stdUtil_FileExists("exists.dat"));
    TEST_ASSERT_FALSE(stdUtil_FileExists("missing.dat"));

    TEST_ASSERT_EQUAL_UINT32(3u, stdUtil_CalcChecksum(bytes, sizeof(bytes), 0u));
    TEST_ASSERT_EQUAL_UINT32(0x12345678u, stdUtil_CalcChecksum(bytes, 0, 0x12345678u));
}

TEST(stdUtil, TestAllocationFailures)
{
    J3DTest_ResetAssertCapture();
    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_NULL(stdUtil_StringDuplicate("copy", StdGeneralTest_GetHostServices()));
    TEST_ASSERT_EQUAL_INT(0, J3DTest_GetAssertCount());
    StdGeneralTest_ClearAllocationFailures();

    J3DTest_ResetAssertCapture();
    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_NULL(stdUtil_ToWString("Indy"));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    StdGeneralTest_ClearAllocationFailures();

    J3DTest_ResetAssertCapture();
    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_NULL(stdUtil_ToAString(L"Jones"));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    StdGeneralTest_ClearAllocationFailures();
}

#ifdef J3D_RUNTIME_GUARDS
TEST(stdUtil, TestGuardedRejections)
{
    char text[4] = "abc";
    wchar_t wtext[4] = L"abc";

    TEST_ASSERT_FALSE(stdUtil_StringCopy(NULL, sizeof(text), "x"));
    TEST_ASSERT_FALSE(stdUtil_StringCopy(text, 0, "x"));
    TEST_ASSERT_FALSE(stdUtil_StringCopy(text, sizeof(text), NULL));
    TEST_ASSERT_FALSE(stdUtil_WStringCopy(NULL, STD_ARRAYLEN(wtext), L"x"));
    TEST_ASSERT_FALSE(stdUtil_WStringCopy(wtext, 0, L"x"));
    TEST_ASSERT_FALSE(stdUtil_WStringCopy(wtext, STD_ARRAYLEN(wtext), NULL));
    TEST_ASSERT_FALSE(stdUtil_StringNumCopy(NULL, sizeof(text), "x", 1));
    TEST_ASSERT_FALSE(stdUtil_StringCat(NULL, sizeof(text), "x"));
    TEST_ASSERT_FALSE(stdUtil_StringCat(text, sizeof(text), NULL));
    TEST_ASSERT_FALSE(stdUtil_StringNumCat(NULL, sizeof(text), "x", 1));

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_NULL(stdUtil_ToWString(NULL));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    StdGeneralTest_AssertLastAssert("pString != NULL", "stdUtil.c");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_NULL(stdUtil_ToAString(NULL));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    StdGeneralTest_AssertLastAssert("pwString != NULL", "stdUtil.c");
}
#endif

TEST(stdUtil, TestStringCapacitiesTruncationAndUnterminatedDestination)
{
    char aText[6] = { 'x', 'x', 'x', 'x', 'x', 'x' };
    wchar_t awText[6] = { L'x', L'x', L'x', L'x', L'x', L'x' };

    // Capacity one allows only a terminator and must not overwrite the adjacent sentinel.
    TEST_ASSERT_TRUE(stdUtil_StringCopy(aText, 1u, "abc"));
    TEST_ASSERT_EQUAL_STRING("", aText);
    TEST_ASSERT_EQUAL_INT('x', aText[1]);
    TEST_ASSERT_TRUE(stdUtil_WStringCopy(awText, 1u, L"abc"));
    TEST_ASSERT_EQUAL(0, wcscmp(L"", awText));
    TEST_ASSERT_EQUAL_INT(L'x', awText[1]);

    // Truncated copies must return the exact prefix and preserve bytes beyond capacity.
    TEST_ASSERT_TRUE(stdUtil_StringCopy(aText, 5u, "abcdef"));
    TEST_ASSERT_EQUAL_STRING("abcd", aText);
    TEST_ASSERT_EQUAL_INT('x', aText[5]);
    TEST_ASSERT_TRUE(stdUtil_WStringNumCopy(awText, 5u, L"abcdef", SIZE_MAX));
    TEST_ASSERT_EQUAL(0, wcscmp(L"abcd", awText));
    TEST_ASSERT_EQUAL_INT(L'x', awText[5]);

    // A zero character count must produce an empty narrow or wide string.
    TEST_ASSERT_TRUE(stdUtil_StringNumCopy(aText, 5u, "abcdef", 0u));
    TEST_ASSERT_EQUAL_STRING("", aText);
    TEST_ASSERT_TRUE(stdUtil_WStringNumCopy(awText, 5u, L"abcdef", 0u));
    TEST_ASSERT_EQUAL(0, wcscmp(L"", awText));

    // Appending to an empty or full destination must keep its exact bounded contents.
    TEST_ASSERT_TRUE(stdUtil_StringCat(aText, 5u, "abcdef"));
    TEST_ASSERT_EQUAL_STRING("abcd", aText);
    TEST_ASSERT_TRUE(stdUtil_StringNumCat(aText, 5u, "z", SIZE_MAX));
    TEST_ASSERT_EQUAL_STRING("abcd", aText);
    TEST_ASSERT_EQUAL_INT('x', aText[5]);

    // Reject an unterminated destination without changing any byte.
    const char aUnterminated[] = { 'a', 'b', 'c', 'd', 'e', 'f' };
    memcpy(aText, aUnterminated, sizeof(aText));
    TEST_ASSERT_FALSE(stdUtil_StringCat(aText, 5u, "z"));
    TEST_ASSERT_EQUAL_MEMORY(aUnterminated, aText, sizeof(aText));
    TEST_ASSERT_FALSE(stdUtil_StringNumCat(aText, 5u, "z", 1u));
    TEST_ASSERT_EQUAL_MEMORY(aUnterminated, aText, sizeof(aText));
}

TEST(stdUtil, TestConversionCapacityAndHighBytes)
{
    const char aBytes[] = { 'A', (char)0x80, (char)0xFF, '\0' };
    const wchar_t awExpected[] = { L'A', 0x80, 0xFF, 0 };
    wchar_t awText[5] = { L'x', L'x', L'x', L'x', L'x' };
    char aText[5] = "xxxx";

    // Widen high bytes without sign extension; test zero, exact, and terminated capacities.
    TEST_ASSERT_EQUAL_INT(0, stdUtil_ToWStringEx(awText, aBytes, 0u));
    TEST_ASSERT_EACH_EQUAL_UINT16(L'x', awText, STD_ARRAYLEN(awText));
    TEST_ASSERT_EQUAL_INT(3, stdUtil_ToWStringEx(awText, aBytes, 3u));
    TEST_ASSERT_EQUAL_MEMORY(awExpected, awText, 3u * sizeof(wchar_t));
    TEST_ASSERT_EQUAL_INT(L'x', awText[3]);
    TEST_ASSERT_EQUAL_INT(3, stdUtil_ToWStringEx(awText, aBytes, 4u));
    TEST_ASSERT_EQUAL(0, wcscmp(awExpected, awText));
    TEST_ASSERT_EQUAL_INT(L'x', awText[4]);

    // Narrowing must preserve high bytes and leave unused destination capacity untouched.
    stdUtil_ToAStringEx(aText, awExpected, 0u);
    TEST_ASSERT_EQUAL_STRING("xxxx", aText);
    stdUtil_ToAStringEx(aText, awExpected, 3u);
    TEST_ASSERT_EQUAL_MEMORY(aBytes, aText, 3u);
    TEST_ASSERT_EQUAL_INT('x', aText[3]);
    stdUtil_ToAStringEx(aText, awExpected, 4u);
    TEST_ASSERT_EQUAL_STRING(aBytes, aText);
    TEST_ASSERT_EQUAL_INT('\0', aText[4]);
}

TEST_GROUP_RUNNER(stdUtil)
{
    RUN_TEST_CASE(stdUtil, TestConversionCapacityAndHighBytes);
    RUN_TEST_CASE(stdUtil, TestStringCapacitiesTruncationAndUnterminatedDestination);
    RUN_TEST_CASE(stdUtil, TestArrayAndMemoryMacros);
    RUN_TEST_CASE(stdUtil, TestStringCopyAndConcatHelpers);
    RUN_TEST_CASE(stdUtil, TestFormatSpecifiers);
    RUN_TEST_CASE(stdUtil, TestFormatGuards);
    RUN_TEST_CASE(stdUtil, TestFormatCapacityBoundaries);
    RUN_TEST_CASE(stdUtil, TestStringSplit);
    RUN_TEST_CASE(stdUtil, TestParseLiteral);
    RUN_TEST_CASE(stdUtil, TestWideAndAnsiConversions);
    RUN_TEST_CASE(stdUtil, TestCaseConversionAndCompare);
    RUN_TEST_CASE(stdUtil, TestDuplicateFileExistsAndChecksum);
    RUN_TEST_CASE(stdUtil, TestAllocationFailures);
    RUN_TEST_CASE(stdUtil, TestArrayConvenienceMacros);
#ifdef J3D_RUNTIME_GUARDS
    RUN_TEST_CASE(stdUtil, TestGuardedRejections);
#endif
}
