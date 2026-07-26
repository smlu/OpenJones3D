#include <unity_fixture.h>

#include <stdio.h>
#include <string.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/std.h>
#include <std/General/stdConffile.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

static void stdConffileTest_DeleteFile(const char* pPath)
{
    StdGeneralTest_DeleteFile(pPath);
}

void stdConffile_PopStack(void);
void stdConffile_PushStack(void);

#define STDCONFFILE_TEST_STACKSIZE 20u

TEST_GROUP(stdConffile);

TEST_SETUP(stdConffile)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdConffile)
{
    stdConffile_CloseWrite();
    stdConffileTest_DeleteFile("stdConffile_inner.cfg");
    stdConffileTest_DeleteFile("stdConffile_outer.cfg");
    stdConffileTest_DeleteFile("stdConffile_write.cfg");
    stdConffileTest_DeleteFile("stdConffile_write.bin");
    stdConffileTest_DeleteFile("stdConffile_generated.cfg");
    StdGeneralTest_Shutdown();
}

TEST(stdConffile, TestOpenModeNoneAllocatesLineBufferAndReadArgsFromString)
{
    char aLine[] = "Key=Value, Flag, \"Quoted Value\", Path=DATA\\CONFIG";

    TEST_ASSERT_EQUAL_INT(1, stdConffile_OpenMode("none", "r"));
    TEST_ASSERT_NOT_NULL(stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_STRING("none", stdConffile_GetFilename());
    TEST_ASSERT_EQUAL_size_t(0u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(0, stdConffile_ReadArgsFromStr(aLine));
    TEST_ASSERT_EQUAL_size_t(4u, stdConffile_g_entry.numArgs);
    TEST_ASSERT_EQUAL_STRING("Key", stdConffile_g_entry.aArgs[0].argName);
    TEST_ASSERT_EQUAL_STRING("Value", stdConffile_g_entry.aArgs[0].argValue);
    TEST_ASSERT_EQUAL_STRING("Flag", stdConffile_g_entry.aArgs[1].argName);
    TEST_ASSERT_EQUAL_STRING("Flag", stdConffile_g_entry.aArgs[1].argValue);
    TEST_ASSERT_EQUAL_STRING("Quoted Value", stdConffile_g_entry.aArgs[2].argName);
    TEST_ASSERT_EQUAL_STRING("Quoted Value", stdConffile_g_entry.aArgs[2].argValue);
    TEST_ASSERT_EQUAL_STRING("Path", stdConffile_g_entry.aArgs[3].argName);
    TEST_ASSERT_EQUAL_STRING("DATA\\CONFIG", stdConffile_g_entry.aArgs[3].argValue);

    stdConffile_Close();
}

TEST(stdConffile, TestReadLineSkipsCommentsLowercasesAndJoinsContinuations)
{
    const char* pPath = StdGeneralTest_GetTestVectorPath("stdConffile/read_lines.cfg");

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pPath));
    TEST_ASSERT_NOT_EQUAL(0u, stdConffile_GetFileHandle());
    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("      ", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(4u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("   \t ", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(5u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("first=value ", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(6u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("multiline", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(8u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("tabbed\tvalue\tline", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(9u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("lastline=done", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(10u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(0, stdConffile_ReadLine());

    stdConffile_Close();
}

TEST(stdConffile, TestReadLineHandlesOverlongPhysicalLine)
{
    const char* pPath = StdGeneralTest_GetTestVectorPath("stdConffile/long_line.cfg");

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pPath));

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_size_t(STDCONFFILE_LINESIZE - 2u, strlen(stdConffile_g_aLine));
    char aExpected[STDCONFFILE_LINESIZE];
    const char* pPattern = "abcdefghijklmnopqrstuvwxyz0123456789";
    memcpy(aExpected, "toolong=", 8u);
    for ( size_t i = 8u; i < STDCONFFILE_LINESIZE - 2u; ++i )
    {
        aExpected[i] = pPattern[(i - 8u) % 36u];
    }
    aExpected[STDCONFFILE_LINESIZE - 2u] = '\0';
    // Verify the exact retained prefix and remainder of the oversized physical line.
    TEST_ASSERT_EQUAL_STRING(aExpected, stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(1u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_size_t(54u, strlen(stdConffile_g_aLine));
    TEST_ASSERT_EQUAL_STRING("stuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(2u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("afterlong=ok", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(3u, stdConffile_GetLineNumber());

    stdConffile_Close();
}

TEST(stdConffile, TestReadLinePreservesWhitespaceOnlyLinesAfterSkippingBlankLines)
{
    const char* pPath = StdGeneralTest_GetTestVectorPath("stdConffile/whitespace_only.cfg");

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pPath));

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("\t\t", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(4u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("   \t  ", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(5u, stdConffile_GetLineNumber());

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("value=afterwhitespace", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_size_t(6u, stdConffile_GetLineNumber());

    stdConffile_Close();
}

TEST(stdConffile, TestReadArgsSkipsEmptyLinesAndParsesCurrentLine)
{
    const char* pPath = StdGeneralTest_GetTestVectorPath("stdConffile/read_args.cfg");

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pPath));
    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadArgs());
    TEST_ASSERT_EQUAL_size_t(2u, stdConffile_g_entry.numArgs);
    TEST_ASSERT_EQUAL_STRING("key", stdConffile_g_entry.aArgs[0].argName);
    TEST_ASSERT_EQUAL_STRING("value", stdConffile_g_entry.aArgs[0].argValue);
    TEST_ASSERT_EQUAL_STRING("flag", stdConffile_g_entry.aArgs[1].argName);

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadArgs());
    TEST_ASSERT_EQUAL_size_t(3u, stdConffile_g_entry.numArgs);
    TEST_ASSERT_EQUAL_STRING("quoted value", stdConffile_g_entry.aArgs[0].argName);
    TEST_ASSERT_EQUAL_STRING("quoted value", stdConffile_g_entry.aArgs[0].argValue);
    TEST_ASSERT_EQUAL_STRING("path", stdConffile_g_entry.aArgs[1].argName);
    TEST_ASSERT_EQUAL_STRING("data\\config", stdConffile_g_entry.aArgs[1].argValue);
    TEST_ASSERT_EQUAL_STRING("loosetoken", stdConffile_g_entry.aArgs[2].argName);
    TEST_ASSERT_EQUAL_STRING("loosetoken", stdConffile_g_entry.aArgs[2].argValue);

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadArgs());
    TEST_ASSERT_EQUAL_size_t(3u, stdConffile_g_entry.numArgs);
    TEST_ASSERT_EQUAL_STRING("alpha", stdConffile_g_entry.aArgs[0].argName);
    TEST_ASSERT_EQUAL_STRING("one", stdConffile_g_entry.aArgs[0].argValue);
    TEST_ASSERT_EQUAL_STRING("beta", stdConffile_g_entry.aArgs[1].argName);
    TEST_ASSERT_EQUAL_STRING("two", stdConffile_g_entry.aArgs[1].argValue);
    TEST_ASSERT_EQUAL_STRING("gamma", stdConffile_g_entry.aArgs[2].argName);
    TEST_ASSERT_EQUAL_STRING("three", stdConffile_g_entry.aArgs[2].argValue);

    TEST_ASSERT_EQUAL_INT(0, stdConffile_ReadArgs());

    stdConffile_Close();
}

TEST(stdConffile, TestTooManyArgsReturnsReadArgsFailure)
{
    const char* pPath = StdGeneralTest_GetTestVectorPath("stdConffile/too_many_args.cfg");

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pPath));
    TEST_ASSERT_EQUAL_INT(0, stdConffile_ReadArgs());
    TEST_ASSERT_EQUAL_size_t(0u, stdConffile_g_entry.numArgs);
    stdConffile_Close();
}

TEST(stdConffile, TestGeneratedReadArgsMaxBoundaryAndOverflow)
{
    char aLine[8192];
    size_t offset = 0;

    for ( size_t i = 0; i < STDCONFILE_MAXARGS; ++i )
    {
        int nWritten = snprintf(
            &aLine[offset],
            sizeof(aLine) - offset,
            "k%03u=v%03u%s",
            (unsigned)i,
            (unsigned)i,
            i + 1u == STDCONFILE_MAXARGS ? "" : ", "
        );

        TEST_ASSERT_TRUE(nWritten > 0);
        TEST_ASSERT_TRUE((size_t)nWritten < sizeof(aLine) - offset);
        offset += (size_t)nWritten;
    }

    TEST_ASSERT_EQUAL_INT(0, stdConffile_ReadArgsFromStr(aLine));
    TEST_ASSERT_EQUAL_size_t(STDCONFILE_MAXARGS, stdConffile_g_entry.numArgs);
    TEST_ASSERT_EQUAL_STRING("k000", stdConffile_g_entry.aArgs[0].argName);
    TEST_ASSERT_EQUAL_STRING("v000", stdConffile_g_entry.aArgs[0].argValue);
    TEST_ASSERT_EQUAL_STRING("k255", stdConffile_g_entry.aArgs[255].argName);
    TEST_ASSERT_EQUAL_STRING("v255", stdConffile_g_entry.aArgs[255].argValue);
    TEST_ASSERT_EQUAL_STRING("k511", stdConffile_g_entry.aArgs[511].argName);
    TEST_ASSERT_EQUAL_STRING("v511", stdConffile_g_entry.aArgs[511].argValue);

    offset = 0;
    for ( size_t i = 0; i < STDCONFILE_MAXARGS + 1u; ++i )
    {
        int nWritten = snprintf(
            &aLine[offset],
            sizeof(aLine) - offset,
            "arg%03u%s",
            (unsigned)i,
            i == STDCONFILE_MAXARGS ? "" : ","
        );

        TEST_ASSERT_TRUE(nWritten > 0);
        TEST_ASSERT_TRUE((size_t)nWritten < sizeof(aLine) - offset);
        offset += (size_t)nWritten;
    }

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadArgsFromStr(aLine));
    TEST_ASSERT_EQUAL_size_t(0u, stdConffile_g_entry.numArgs);
}

TEST(stdConffile, TestGeneratedReadArgsDelimiterAndQuoteCorpus)
{
    {
        char aLine[] = ",,, \t\r\n";

        TEST_ASSERT_EQUAL_INT(0, stdConffile_ReadArgsFromStr(aLine));
        TEST_ASSERT_EQUAL_size_t(0u, stdConffile_g_entry.numArgs);
    }

    {
        char aLine[] = "alpha=one,, beta=two\tgamma=three\r\n";

        TEST_ASSERT_EQUAL_INT(0, stdConffile_ReadArgsFromStr(aLine));
        TEST_ASSERT_EQUAL_size_t(3u, stdConffile_g_entry.numArgs);
        TEST_ASSERT_EQUAL_STRING("alpha", stdConffile_g_entry.aArgs[0].argName);
        TEST_ASSERT_EQUAL_STRING("one", stdConffile_g_entry.aArgs[0].argValue);
        TEST_ASSERT_EQUAL_STRING("beta", stdConffile_g_entry.aArgs[1].argName);
        TEST_ASSERT_EQUAL_STRING("two", stdConffile_g_entry.aArgs[1].argValue);
        TEST_ASSERT_EQUAL_STRING("gamma", stdConffile_g_entry.aArgs[2].argName);
        TEST_ASSERT_EQUAL_STRING("three", stdConffile_g_entry.aArgs[2].argValue);
    }

    {
        char aLine[] = "\"quoted token\", key=\"quotedvalue\", loose";

        TEST_ASSERT_EQUAL_INT(0, stdConffile_ReadArgsFromStr(aLine));
        TEST_ASSERT_EQUAL_size_t(3u, stdConffile_g_entry.numArgs);
        TEST_ASSERT_EQUAL_STRING("quoted token", stdConffile_g_entry.aArgs[0].argName);
        TEST_ASSERT_EQUAL_STRING("quoted token", stdConffile_g_entry.aArgs[0].argValue);
        TEST_ASSERT_EQUAL_STRING("quotedvalue", stdConffile_g_entry.aArgs[1].argName);
        TEST_ASSERT_EQUAL_STRING("quotedvalue", stdConffile_g_entry.aArgs[1].argValue);
        TEST_ASSERT_EQUAL_STRING("loose", stdConffile_g_entry.aArgs[2].argName);
        TEST_ASSERT_EQUAL_STRING("loose", stdConffile_g_entry.aArgs[2].argValue);
    }
}

TEST(stdConffile, TestReadAndScanLineFormatsAndErrors)
{
    const char* pPath = StdGeneralTest_GetTestVectorPath("stdConffile/raw_and_scan.cfg");
    char aData[5];
    int value             = 0;
    int signedValue       = 0;
    unsigned int hexValue = 0;
    float floatValue      = 0.0f;
    char aWord[16];
    char charValue        = '\0';
    char aBracket[16];

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pPath));
    TEST_ASSERT_EQUAL_INT(1, stdConffile_Read(aData, 4u));
    aData[4] = '\0';
    TEST_ASSERT_EQUAL_STRING("ABCD", aData);

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ScanLine("value %d", &value));
    TEST_ASSERT_EQUAL_INT(27, value);

    TEST_ASSERT_EQUAL_INT(2, stdConffile_ScanLine("pair %d %x", &signedValue, &hexValue));
    TEST_ASSERT_EQUAL_INT(-12, signedValue);
    TEST_ASSERT_EQUAL_UINT(0x2Au, hexValue);

    TEST_ASSERT_EQUAL_INT(
        4,
        stdConffile_ScanLine("mixed %d %f %15s %c", &value, &floatValue, aWord, (unsigned)sizeof(aWord), &charValue, (unsigned)1)
    );
    TEST_ASSERT_EQUAL_INT(42, value);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.5f, floatValue);
    TEST_ASSERT_EQUAL_STRING("word", aWord);
    TEST_ASSERT_EQUAL_INT('z', charValue);

    TEST_ASSERT_EQUAL_INT(
        2,
        stdConffile_ScanLine("chars %c %15s", &charValue, (unsigned)1, aWord, (unsigned)sizeof(aWord))
    );
    TEST_ASSERT_EQUAL_INT('q', charValue);
    TEST_ASSERT_EQUAL_STRING("temple", aWord);

    TEST_ASSERT_EQUAL_INT(1, stdConffile_ScanLine("bracket %15[a-z0-9]", aBracket, (unsigned)sizeof(aBracket)));
    TEST_ASSERT_EQUAL_STRING("abc123", aBracket);

    {
        int decimalValue           = 0;
        unsigned int unsignedValue = 0;
        int octalAutoValue        = 0;
        int hexLowerAutoValue     = 0;
        int hexUpperAutoValue     = 0;

        TEST_ASSERT_EQUAL_INT(
            5,
            stdConffile_ScanLine(
                "bases %d %u %i %i %i",
                &decimalValue,
                &unsignedValue,
                &octalAutoValue,
                &hexLowerAutoValue,
                &hexUpperAutoValue
            )
        );
        TEST_ASSERT_EQUAL_INT(-42, decimalValue);
        TEST_ASSERT_EQUAL_UINT(42u, unsignedValue);
        TEST_ASSERT_EQUAL_INT(42, octalAutoValue);
        TEST_ASSERT_EQUAL_INT(42, hexLowerAutoValue);
        TEST_ASSERT_EQUAL_INT(42, hexUpperAutoValue);
    }

    {
        unsigned int hexPlain       = 0;
        unsigned int hexLowerPrefix = 0;
        unsigned int hexUpperPrefix = 0;
        unsigned int hexUpper       = 0;
        unsigned int hexPadded      = 0;

        TEST_ASSERT_EQUAL_INT(
            5,
            stdConffile_ScanLine(
                "hexes %x %x %x %x %x",
                &hexPlain,
                &hexLowerPrefix,
                &hexUpperPrefix,
                &hexUpper,
                &hexPadded
            )
        );
        TEST_ASSERT_EQUAL_UINT(0x2Au, hexPlain);
        TEST_ASSERT_EQUAL_UINT(0x2Au, hexLowerPrefix);
        TEST_ASSERT_EQUAL_UINT(0x2Au, hexUpperPrefix);
        TEST_ASSERT_EQUAL_UINT(0xBEEFu, hexUpper);
        TEST_ASSERT_EQUAL_UINT(0x00FFu, hexPadded);
    }

    {
        unsigned int prefixedMixedHex = 0;
        unsigned int plainMixedHex    = 0;
        unsigned int mixedPrefixHex   = 0;

        TEST_ASSERT_EQUAL_INT(
            3,
            stdConffile_ScanLine(
                "mixedcasehex %x %x %x",
                &prefixedMixedHex,
                &plainMixedHex,
                &mixedPrefixHex
            )
        );
        TEST_ASSERT_EQUAL_UINT(0x2ABFu, prefixedMixedHex);
        TEST_ASSERT_EQUAL_UINT(0xDEADC0DEu, plainMixedHex);
        TEST_ASSERT_EQUAL_UINT(0xABCDEFu, mixedPrefixHex);
    }

    {
        int spacedValue       = 0;
        unsigned int spacedHexValue = 0;
        char aSpacedWord[16];

        TEST_ASSERT_EQUAL_INT(
            3,
            stdConffile_ScanLine(
                "spaced %d %x %15s",
                &spacedValue,
                &spacedHexValue,
                aSpacedWord,
                (unsigned)sizeof(aSpacedWord)
            )
        );
        TEST_ASSERT_EQUAL_INT(17, spacedValue);
        TEST_ASSERT_EQUAL_UINT(0x2ABFu, spacedHexValue);
        TEST_ASSERT_EQUAL_STRING("spaced", aSpacedWord);
    }

    {
        short shortValue                     = 0;
        unsigned short unsignedShortValue    = 0;
        long longValue                       = 0;
        unsigned long unsignedLongValue      = 0;
        unsigned long long hexLongLongValue  = 0;

        TEST_ASSERT_EQUAL_INT(
            5,
            stdConffile_ScanLine(
                "lengths %hd %hu %ld %lu %llx",
                &shortValue,
                &unsignedShortValue,
                &longValue,
                &unsignedLongValue,
                &hexLongLongValue
            )
        );
        TEST_ASSERT_EQUAL_INT(-123, shortValue);
        TEST_ASSERT_EQUAL_UINT(65000u, unsignedShortValue);
        TEST_ASSERT_EQUAL_INT(-200000, longValue);
        TEST_ASSERT_EQUAL_UINT32(4000000000UL, unsignedLongValue);
        TEST_ASSERT_EQUAL_UINT32(0x11223344u, (uint32_t)(hexLongLongValue >> 32));
        TEST_ASSERT_EQUAL_UINT32(0x55667788u, (uint32_t)hexLongLongValue);
    }

    {
        float scanFloat                = 0.0f;
        double scanDouble              = 0.0;
        float exponentFloat            = 0.0f;
        float hexFloat                 = 0.0f;
        long double longDoubleValue    = 0.0L;

        TEST_ASSERT_EQUAL_INT(
            5,
            stdConffile_ScanLine(
                "floats %f %lf %e %a %Lf",
                &scanFloat,
                &scanDouble,
                &exponentFloat,
                &hexFloat,
                &longDoubleValue
            )
        );
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.25f, scanFloat);
        TEST_ASSERT_TRUE(scanDouble > -2.5001 && scanDouble < -2.4999);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 62.5f, exponentFloat);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 6.0f, hexFloat);
        TEST_ASSERT_TRUE((double)longDoubleValue > 9.7499 && (double)longDoubleValue < 9.7501);
    }

    {
        char aLowerHex[8];
        char aUpperHex[8];
        char aMixedHex[16];

        TEST_ASSERT_EQUAL_INT(
            3,
            stdConffile_ScanLine(
                "hexstrings %7[0-9a-fx] %7[0-9a-fx] %15[0-9a-f]",
                aLowerHex,
                (unsigned)sizeof(aLowerHex),
                aUpperHex,
                (unsigned)sizeof(aUpperHex),
                aMixedHex,
                (unsigned)sizeof(aMixedHex)
            )
        );
        TEST_ASSERT_EQUAL_STRING("0x2a", aLowerHex);
        TEST_ASSERT_EQUAL_STRING("0x2a", aUpperHex);
        TEST_ASSERT_EQUAL_STRING("deadbeef", aMixedHex);
    }

    TEST_ASSERT_EQUAL_INT(0, stdConffile_ScanLine("literal_only"));
    TEST_ASSERT_EQUAL_INT(0, stdConffile_ScanLine("badint %d", &value));
    TEST_ASSERT_EQUAL_INT(-1, stdConffile_ScanLine("%d", &value));

    stdConffile_Close();
}

TEST(stdConffile, TestGeneratedScanLineMatrix)
{
    const char* pPath = "stdConffile_generated.cfg";

    stdConffileTest_DeleteFile(pPath);

    TEST_ASSERT_EQUAL_INT(1, stdConffile_OpenWrite(pPath));
    for ( size_t i = 0; i < 32u; ++i )
    {
        TEST_ASSERT_EQUAL_INT(
            0,
            stdConffile_Printf(
                "case %u %d 0x%X %.3f word%02u\n",
                (unsigned)i,
                (int)i - 16,
                (unsigned)(i * 17u + 0x20u),
                ((double)(int)i - 8.0) / 4.0,
                (unsigned)i
            )
        );
    }
    stdConffile_CloseWrite();

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pPath));
    for ( size_t i = 0; i < 32u; ++i )
    {
        int caseIndex         = -1;
        int signedValue       = 0;
        unsigned int hexValue = 0;
        float floatValue      = 0.0f;
        char aWord[16];

        TEST_ASSERT_EQUAL_INT(
            5,
            stdConffile_ScanLine(
                "case %d %d %x %f %15s",
                &caseIndex,
                &signedValue,
                &hexValue,
                &floatValue,
                aWord,
                (unsigned)sizeof(aWord)
            )
        );
        TEST_ASSERT_EQUAL_INT((int)i, caseIndex);
        TEST_ASSERT_EQUAL_INT((int)i - 16, signedValue);
        TEST_ASSERT_EQUAL_UINT((unsigned)(i * 17u + 0x20u), hexValue);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, ((float)(int)i - 8.0f) / 4.0f, floatValue);

        {
            char aExpectedWord[16];
            STD_FORMAT(aExpectedWord, "word%02u", (unsigned)i);
            TEST_ASSERT_EQUAL_STRING(aExpectedWord, aWord);
        }
    }
    TEST_ASSERT_EQUAL_INT(0, stdConffile_ReadLine());
    stdConffile_Close();

    stdConffileTest_DeleteFile(pPath);
}

TEST(stdConffile, TestWriteLineWritePrintfAndCloseWrite)
{
    const char* pPath = "stdConffile_write.cfg";
    char aText[64];
    tFileHandle fh;

    stdConffileTest_DeleteFile(pPath);

    TEST_ASSERT_EQUAL_INT(1, stdConffile_OpenWrite(pPath));
    TEST_ASSERT_NOT_EQUAL(0u, stdConffile_GetWriteFileHandle());
    TEST_ASSERT_EQUAL_STRING(pPath, stdConffile_GetWriteFilename());
    TEST_ASSERT_EQUAL_INT(0, stdConffile_WriteLine("alpha\n"));
    TEST_ASSERT_EQUAL_INT(0, stdConffile_Write("beta", 4u));
    TEST_ASSERT_EQUAL_INT(0, stdConffile_Printf("-%d", 33));
    stdConffile_CloseWrite();
    TEST_ASSERT_EQUAL_STRING("NOT_OPEN", stdConffile_GetWriteFilename());

    fh = stdFileOpen(pPath, "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(13u, stdFileRead(fh, aText, 13u));
    aText[13] = '\0';
    TEST_ASSERT_EQUAL_STRING("alpha\nbeta-33", aText);
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    stdConffileTest_DeleteFile(pPath);
}

TEST(stdConffile, TestWriteBinaryAndRejectInvalidWriteRequests)
{
    const char* pPath = "stdConffile_write.bin";
    const uint8_t aBinary[] = { 0x00u, 0x7Fu, 0x80u, 0xFFu, 'I', 'J' };
    uint8_t aRead[sizeof(aBinary)];
    tFileHandle fh;

    stdConffileTest_DeleteFile(pPath);

    TEST_ASSERT_EQUAL_INT(1, stdConffile_WriteLine("no file\n"));
    TEST_ASSERT_EQUAL_INT(1, stdConffile_Write(aBinary, sizeof(aBinary)));
    TEST_ASSERT_EQUAL_INT(1, stdConffile_Printf("no file"));

    TEST_ASSERT_EQUAL_INT(1, stdConffile_OpenWrite(pPath));
    TEST_ASSERT_EQUAL_INT(0, stdConffile_OpenWrite(pPath));
    TEST_ASSERT_EQUAL_INT(1, stdConffile_WriteLine(NULL));
    TEST_ASSERT_EQUAL_INT(1, stdConffile_Write(NULL, sizeof(aBinary)));
    TEST_ASSERT_EQUAL_INT(1, stdConffile_Printf(NULL));
    TEST_ASSERT_EQUAL_INT(0, stdConffile_Write(aBinary, sizeof(aBinary)));
    stdConffile_CloseWrite();

    fh = stdFileOpen(pPath, "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(sizeof(aBinary), stdFileRead(fh, aRead, sizeof(aRead)));
    TEST_ASSERT_EQUAL_MEMORY(aBinary, aRead, sizeof(aBinary));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    stdConffileTest_DeleteFile(pPath);
}

TEST(stdConffile, TestReadAndWriteFailuresReturnErrors)
{
    const char* pPath = "stdConffile_write.cfg";
    char aData[4];

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(StdGeneralTest_GetTestVectorPath("stdConffile/raw_and_scan.cfg")));

    StdGeneralTest_FailNextFileReads(0);
    TEST_ASSERT_EQUAL_INT(0, stdConffile_Read(aData, sizeof(aData)));
    StdGeneralTest_ClearFileFailures();

    StdGeneralTest_FailNextFileGets(0);
    TEST_ASSERT_EQUAL_INT(0, stdConffile_ReadLine());
    StdGeneralTest_ClearFileFailures();

    stdConffile_Close();

    StdGeneralTest_FailNextFileOpens(0);
    TEST_ASSERT_EQUAL_INT(0, stdConffile_OpenWrite(pPath));
    StdGeneralTest_ClearFileFailures();

    TEST_ASSERT_EQUAL_INT(1, stdConffile_OpenWrite(pPath));

    StdGeneralTest_FailNextFileWrites(0);
    TEST_ASSERT_EQUAL_INT(1, stdConffile_WriteLine("alpha\n"));
    StdGeneralTest_ClearFileFailures();

    StdGeneralTest_FailNextFileWrites(0);
    TEST_ASSERT_EQUAL_INT(1, stdConffile_Write("beta", 4u));
    StdGeneralTest_ClearFileFailures();

    StdGeneralTest_FailNextFileWrites(0);
    TEST_ASSERT_EQUAL_INT(1, stdConffile_Printf("gamma"));
    StdGeneralTest_ClearFileFailures();

    stdConffile_CloseWrite();
}

TEST(stdConffile, TestPrintfTruncatesAndWarnsForOversizedOutput)
{
    const char* pPath = "stdConffile_write.cfg";
    // Verify both the complete truncation warning and every byte written to the file.
    char aLargeText[5000];

    memset(aLargeText, 'x', sizeof(aLargeText));
    aLargeText[sizeof(aLargeText) - 1u] = '\0';

    TEST_ASSERT_EQUAL_INT(1, stdConffile_OpenWrite(pPath));
    StdGeneralTest_ResetDebugOutput();
    TEST_ASSERT_EQUAL_INT(0, stdConffile_Printf("%s", aLargeText));
    TEST_ASSERT_EQUAL_STRING(
        "stdConffile.c(240): stdConffile_Printf: Truncated formatted output for 'stdConffile_write.cfg' from 4999 to 4095 bytes.\n",
        StdGeneralTest_GetDebugOutput()
    );
    stdConffile_CloseWrite();
    TEST_ASSERT_EQUAL_size_t(4095u, stdFileSize(pPath));
    tFileHandle fh = stdFileOpen(pPath, "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    char aWritten[4096];
    TEST_ASSERT_EQUAL_size_t(4095u, stdFileRead(fh, aWritten, sizeof(aWritten)));
    aWritten[4095] = '\0';
    aLargeText[4095] = '\0';
    TEST_ASSERT_EQUAL_STRING(aLargeText, aWritten);
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
}

TEST(stdConffile, TestNestedOpenRestoresPreviousFileState)
{
    const char* pOuterPath = "stdConffile_outer.cfg";
    const char* pInnerPath = "stdConffile_inner.cfg";
    tFileHandle outerHandle;
    tFileHandle innerHandle;

    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile("stdConffile/outer.cfg", pOuterPath));
    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile("stdConffile/inner.cfg", pInnerPath));

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pOuterPath));
    outerHandle = stdConffile_GetFileHandle();
    TEST_ASSERT_NOT_EQUAL(0u, outerHandle);
    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("outer_one", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_STRING(pOuterPath, stdConffile_GetFilename());

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pInnerPath));
    innerHandle = stdConffile_GetFileHandle();
    TEST_ASSERT_NOT_EQUAL(0u, innerHandle);
    TEST_ASSERT_NOT_EQUAL(outerHandle, innerHandle);
    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("inner_one", stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_STRING(pInnerPath, stdConffile_GetFilename());

    stdConffile_Close();
    TEST_ASSERT_EQUAL(outerHandle, stdConffile_GetFileHandle());
    TEST_ASSERT_EQUAL_STRING(pOuterPath, stdConffile_GetFilename());
    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("outer_two", stdConffile_g_aLine);

    stdConffile_Close();
}

TEST(stdConffile, TestNestedOpenAllocationFailureRestoresPreviousFileState)
{
    const char* pOuterPath = "stdConffile_outer.cfg";
    const char* pInnerPath = "stdConffile_inner.cfg";

    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile("stdConffile/outer.cfg", pOuterPath));
    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile("stdConffile/inner.cfg", pInnerPath));

    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pOuterPath));
    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("outer_one", stdConffile_g_aLine);

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_EQUAL_INT(0, stdConffile_Open(pInnerPath));
    StdGeneralTest_ClearAllocationFailures();

    TEST_ASSERT_EQUAL_STRING(pOuterPath, stdConffile_GetFilename());
    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("outer_two", stdConffile_g_aLine);

    stdConffile_Close();
}

#ifdef J3D_RUNTIME_GUARDS
TEST(stdConffile, TestOpenStackOverflowAndEmptyPopNoop)
{
    TEST_ASSERT_EQUAL_INT(1, stdConffile_OpenMode("none", "r"));

    for ( size_t i = 0; i < STDCONFFILE_TEST_STACKSIZE; ++i )
    {
        TEST_ASSERT_EQUAL_INT(1, stdConffile_OpenMode("none", "r"));
    }

    TEST_ASSERT_EQUAL_INT(0, stdConffile_OpenMode("none", "r"));

    J3DTest_ResetAssertCapture();
    stdConffile_PushStack();
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    TEST_ASSERT_EQUAL_STRING("none", stdConffile_GetFilename());

    for ( size_t i = 0; i < STDCONFFILE_TEST_STACKSIZE + 1u; ++i )
    {
        stdConffile_Close();
    }

    stdConffile_PopStack();
    TEST_ASSERT_EQUAL_INT(1, stdConffile_OpenMode("none", "r"));
    stdConffile_Close();
}
#endif

TEST(stdConffile, TestOpenFailuresAndAllocationFailureRestoreState)
{
    const char* pPath = "stdConffile_inner.cfg";

    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile("stdConffile/inner.cfg", pPath));

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_INT(0, stdConffile_OpenMode(NULL, "r"));
    TEST_ASSERT_EQUAL_INT(0, stdConffile_OpenMode(pPath, NULL));
#endif
    TEST_ASSERT_EQUAL_INT(0, stdConffile_Open("stdConffile_missing.cfg"));

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_EQUAL_INT(0, stdConffile_Open(pPath));
    StdGeneralTest_ClearAllocationFailures();

    TEST_ASSERT_NULL(stdConffile_g_aLine);
    TEST_ASSERT_EQUAL_INT(1, stdConffile_Open(pPath));
    TEST_ASSERT_EQUAL_INT(1, stdConffile_ReadLine());
    TEST_ASSERT_EQUAL_STRING("inner_one", stdConffile_g_aLine);
    stdConffile_Close();
}

TEST_GROUP_RUNNER(stdConffile)
{
    RUN_TEST_CASE(stdConffile, TestOpenModeNoneAllocatesLineBufferAndReadArgsFromString);
    RUN_TEST_CASE(stdConffile, TestReadLineSkipsCommentsLowercasesAndJoinsContinuations);
    RUN_TEST_CASE(stdConffile, TestReadLineHandlesOverlongPhysicalLine);
    RUN_TEST_CASE(stdConffile, TestReadLinePreservesWhitespaceOnlyLinesAfterSkippingBlankLines);
    RUN_TEST_CASE(stdConffile, TestReadArgsSkipsEmptyLinesAndParsesCurrentLine);
    RUN_TEST_CASE(stdConffile, TestTooManyArgsReturnsReadArgsFailure);
    RUN_TEST_CASE(stdConffile, TestGeneratedReadArgsMaxBoundaryAndOverflow);
    RUN_TEST_CASE(stdConffile, TestGeneratedReadArgsDelimiterAndQuoteCorpus);
    RUN_TEST_CASE(stdConffile, TestReadAndScanLineFormatsAndErrors);
    RUN_TEST_CASE(stdConffile, TestGeneratedScanLineMatrix);
    RUN_TEST_CASE(stdConffile, TestWriteLineWritePrintfAndCloseWrite);
    RUN_TEST_CASE(stdConffile, TestWriteBinaryAndRejectInvalidWriteRequests);
    RUN_TEST_CASE(stdConffile, TestReadAndWriteFailuresReturnErrors);
    RUN_TEST_CASE(stdConffile, TestPrintfTruncatesAndWarnsForOversizedOutput);
    RUN_TEST_CASE(stdConffile, TestNestedOpenRestoresPreviousFileState);
    RUN_TEST_CASE(stdConffile, TestNestedOpenAllocationFailureRestoresPreviousFileState);
#ifdef J3D_RUNTIME_GUARDS
    RUN_TEST_CASE(stdConffile, TestOpenStackOverflowAndEmptyPopNoop);
#endif
    RUN_TEST_CASE(stdConffile, TestOpenFailuresAndAllocationFailureRestoreState);
}
