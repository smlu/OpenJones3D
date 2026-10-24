#include <unity_fixture.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <j3dcore/j3d.h>

typedef int (J3DAPI* J3DCoreTestPrintfFunc)(const char* pFormat, ...);
typedef void (J3DAPI* J3DCoreTestAssertFunc)(const char* pErrorStr, const char* pFilename, int linenum);

typedef struct J3DCoreTestHostServices
{
    J3DCoreTestPrintfFunc pDebugPrint;
    J3DCoreTestPrintfFunc pStatusPrint;
    J3DCoreTestPrintfFunc pMessagePrint;
    J3DCoreTestPrintfFunc pWarningPrint;
    J3DCoreTestPrintfFunc pErrorPrint;
    J3DCoreTestAssertFunc pAssert;
} J3DCoreTestHostServices;

typedef enum J3D_ENUM_TYPE(uint8_t)
{
    J3DCORE_TEST_ENUM_VALUE = 7
} J3DCoreTestEnum;

static char j3dCoreTest_aLastLogFormat[128];
static char j3dCoreTest_aLastLogMessage[256];
static char j3dCoreTest_aLastLogFile[128];
static char j3dCoreTest_aLastPrintChannel[32];
static char j3dCoreTest_aLastPrintedMessage[256];
static unsigned int j3dCoreTest_lastLogLine;

static int j3dCoreTest_numAsserts;
static char j3dCoreTest_aLastAssertText[128];
static char j3dCoreTest_aLastAssertFile[128];
static int j3dCoreTest_lastAssertLine;

static void J3DCoreTest_CopyString(char* pDest, size_t destSize, const char* pSource)
{
    if ( destSize == 0 )
    {
        return;
    }

    snprintf(pDest, destSize, "%s", pSource ? pSource : "");
    pDest[destSize - 1] = '\0';
}

static void J3DCoreTest_ResetCapture(void)
{
    j3dCoreTest_aLastLogFormat[0] = '\0';
    j3dCoreTest_aLastLogMessage[0] = '\0';
    j3dCoreTest_aLastLogFile[0] = '\0';
    j3dCoreTest_aLastPrintChannel[0] = '\0';
    j3dCoreTest_aLastPrintedMessage[0] = '\0';
    j3dCoreTest_lastLogLine = 0;

    j3dCoreTest_numAsserts = 0;
    j3dCoreTest_aLastAssertText[0] = '\0';
    j3dCoreTest_aLastAssertFile[0] = '\0';
    j3dCoreTest_lastAssertLine = 0;
}

static int J3DCoreTest_PrintToChannel(const char* pChannel, const char* pFormat, va_list args)
{
    J3DCoreTest_CopyString(j3dCoreTest_aLastPrintChannel, sizeof(j3dCoreTest_aLastPrintChannel), pChannel);
    vsnprintf(j3dCoreTest_aLastPrintedMessage, sizeof(j3dCoreTest_aLastPrintedMessage), pFormat, args);
    j3dCoreTest_aLastPrintedMessage[sizeof(j3dCoreTest_aLastPrintedMessage) - 1] = '\0';
    return (int)strlen(j3dCoreTest_aLastPrintedMessage);
}

static int J3DAPI J3DCoreTest_DebugPrint(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    int result = J3DCoreTest_PrintToChannel("debug", pFormat, args);
    va_end(args);
    return result;
}

static int J3DAPI J3DCoreTest_StatusPrint(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    int result = J3DCoreTest_PrintToChannel("status", pFormat, args);
    va_end(args);
    return result;
}

static int J3DAPI J3DCoreTest_MessagePrint(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    int result = J3DCoreTest_PrintToChannel("message", pFormat, args);
    va_end(args);
    return result;
}

static int J3DAPI J3DCoreTest_WarningPrint(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    int result = J3DCoreTest_PrintToChannel("warning", pFormat, args);
    va_end(args);
    return result;
}

static int J3DAPI J3DCoreTest_ErrorPrint(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    int result = J3DCoreTest_PrintToChannel("error", pFormat, args);
    va_end(args);
    return result;
}

static void J3DAPI J3DCoreTest_Assert(const char* pErrorStr, const char* pFilename, int linenum)
{
    ++j3dCoreTest_numAsserts;
    J3DCoreTest_CopyString(j3dCoreTest_aLastAssertText, sizeof(j3dCoreTest_aLastAssertText), pErrorStr);
    J3DCoreTest_CopyString(j3dCoreTest_aLastAssertFile, sizeof(j3dCoreTest_aLastAssertFile), pFilename);
    j3dCoreTest_lastAssertLine = linenum;
}

static J3DCoreTestHostServices J3DCoreTest_MakeHostServices(void)
{
    J3DCoreTestHostServices hs = {
        J3DCoreTest_DebugPrint,
        J3DCoreTest_StatusPrint,
        J3DCoreTest_MessagePrint,
        J3DCoreTest_WarningPrint,
        J3DCoreTest_ErrorPrint,
        J3DCoreTest_Assert
    };

    return hs;
}

static void J3DAPI stdPrintf(J3DCoreTestPrintfFunc pfPrint, const char* pFilePath, unsigned int linenum, const char* pFormat, ...)
{
    J3DCoreTest_CopyString(j3dCoreTest_aLastLogFormat, sizeof(j3dCoreTest_aLastLogFormat), pFormat);
    J3DCoreTest_CopyString(j3dCoreTest_aLastLogFile, sizeof(j3dCoreTest_aLastLogFile), pFilePath);
    j3dCoreTest_lastLogLine = linenum;

    va_list args;
    va_start(args, pFormat);
    vsnprintf(j3dCoreTest_aLastLogMessage, sizeof(j3dCoreTest_aLastLogMessage), pFormat ? pFormat : "", args);
    va_end(args);
    j3dCoreTest_aLastLogMessage[sizeof(j3dCoreTest_aLastLogMessage) - 1] = '\0';

    if ( pfPrint )
    {
        pfPrint("%s", j3dCoreTest_aLastLogMessage);
    }
}

static int J3DCoreTest_GuardReturn(int value)
{
    STD_GUARD(value > 0, -11);
    return value;
}

static void J3DCoreTest_GuardVoid(int value)
{
    J3D_UNUSED(value);
    STD_GUARD_VOID(value > 0);
    ++j3dCoreTest_numAsserts;
}

static int J3DCoreTest_GuardExReturn(int value)
{
    STD_GUARDEX(value > 0, {
        return -17;
    });
    return value;
}

TEST_GROUP(j3dMacros);

TEST_SETUP(j3dMacros)
{
    J3DCoreTest_ResetCapture();
}

TEST_TEAR_DOWN(j3dMacros)
{
}

TEST(j3dMacros, TestBuildMacros)
{
    TEST_ASSERT_TRUE(strlen(J3D_FILE) > 0);

#ifdef J3D_QOL_IMPROVEMENTS
    TEST_ASSERT_EQUAL_INT(10, J3D_QOL_VALUE(10, 20));
#else
    TEST_ASSERT_EQUAL_INT(20, J3D_QOL_VALUE(10, 20));
#endif

#ifdef J3D_64BIT
    TEST_ASSERT_EQUAL_size_t(8u, sizeof(void*));
#else
    TEST_ASSERT_EQUAL_size_t(4u, sizeof(void*));
#endif

#ifdef J3D_OS_WINDOWS
    TEST_ASSERT_EQUAL_INT(1, J3D_OS_WINDOWS);
#endif

    TEST_ASSERT_EQUAL_INT(J3DCORE_TEST_ENUM_VALUE, (int)(J3DCoreTestEnum)7);
}

TEST(j3dMacros, TestMemoryAndStringEquivalenceMacros)
{
    const char bytesA[] = { 1, 2, 3, 4 };
    const char bytesB[] = { 1, 2, 3, 5 };

    TEST_ASSERT_TRUE(memeq(bytesA, bytesB, 3));
    TEST_ASSERT_FALSE(memeq(bytesA, bytesB, 4));

    TEST_ASSERT_TRUE(streq("Indy", "Indy"));
    TEST_ASSERT_FALSE(streq("Indy", "indy"));
    TEST_ASSERT_TRUE(strneq("Temple", "Temporary", 4));
    TEST_ASSERT_FALSE(strneq("Temple", "TemPorary", 4));

    TEST_ASSERT_TRUE(wstreq(L"Indy", L"Indy"));
    TEST_ASSERT_FALSE(wstreq(L"Indy", L"indy"));
    TEST_ASSERT_TRUE(wstrneq(L"Temple", L"Temporary", 4));
    TEST_ASSERT_FALSE(wstrneq(L"Temple", L"TemPorary", 4));
}

TEST(j3dMacros, TestCaseInsensitiveStringMacros)
{
    TEST_ASSERT_TRUE(streqi("Indy", "indy"));
    TEST_ASSERT_FALSE(streqi("Indy", "Indx"));
    TEST_ASSERT_TRUE(strneqi("Temple", "temporary", 4));
    TEST_ASSERT_FALSE(strneqi("Temple", "temaorary", 4));

    TEST_ASSERT_TRUE(wstreqi(L"Indy", L"indy"));
    TEST_ASSERT_FALSE(wstreqi(L"Indy", L"Indx"));
    TEST_ASSERT_TRUE(wstrneqi(L"Temple", L"temporary", 4));
    TEST_ASSERT_FALSE(wstrneqi(L"Temple", L"temaorary", 4));
}

TEST(j3dMacros, TestUtilityMacros)
{
    TEST_ASSERT_EQUAL_INT(3, J3DMIN(3, 9));
    TEST_ASSERT_EQUAL_INT(9, J3DMAX(3, 9));

    int value = 42;
    TEST_ASSERT_EQUAL_INT(42, J3D_UNUSED(value));

    char  text[]   = "alpha,beta";
    char* pContext = NULL;
    TEST_ASSERT_EQUAL_STRING("alpha", strtok_r(text, ",", &pContext));
    TEST_ASSERT_EQUAL_STRING("beta", strtok_r(NULL, ",", &pContext));
}

TEST(j3dMacros, TestRuntimeGuardMacros)
{
#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_INT(-11, J3DCoreTest_GuardReturn(0));
    TEST_ASSERT_EQUAL_INT(5, J3DCoreTest_GuardReturn(5));

    j3dCoreTest_numAsserts = 0;
    J3DCoreTest_GuardVoid(0);
    TEST_ASSERT_EQUAL_INT(0, j3dCoreTest_numAsserts);
    J3DCoreTest_GuardVoid(1);
    TEST_ASSERT_EQUAL_INT(1, j3dCoreTest_numAsserts);

    TEST_ASSERT_EQUAL_INT(-17, J3DCoreTest_GuardExReturn(0));
    TEST_ASSERT_EQUAL_INT(9, J3DCoreTest_GuardExReturn(9));
#else
    TEST_ASSERT_EQUAL_INT(0, J3DCoreTest_GuardReturn(0));
    TEST_ASSERT_EQUAL_INT(5, J3DCoreTest_GuardReturn(5));

    j3dCoreTest_numAsserts = 0;
    J3DCoreTest_GuardVoid(0);
    TEST_ASSERT_EQUAL_INT(1, j3dCoreTest_numAsserts);

    TEST_ASSERT_EQUAL_INT(0, J3DCoreTest_GuardExReturn(0));
    TEST_ASSERT_EQUAL_INT(9, J3DCoreTest_GuardExReturn(9));
#endif
}

TEST(j3dMacros, TestLoggingMacros)
{
    J3DCoreTestHostServices hs = J3DCoreTest_MakeHostServices();

    J3DLOG_DEBUG((&hs), "debug %d", 7);
    TEST_ASSERT_EQUAL_STRING("debug %d", j3dCoreTest_aLastLogFormat);
    TEST_ASSERT_EQUAL_STRING("debug 7", j3dCoreTest_aLastLogMessage);
    TEST_ASSERT_EQUAL_STRING("debug", j3dCoreTest_aLastPrintChannel);
    TEST_ASSERT_EQUAL_STRING("debug 7", j3dCoreTest_aLastPrintedMessage);
    TEST_ASSERT_TRUE(j3dCoreTest_lastLogLine > 0);

    J3DLOG_STATUS((&hs), "status");
    TEST_ASSERT_EQUAL_STRING("status", j3dCoreTest_aLastPrintChannel);

    J3DLOG_MESSAGE((&hs), "message");
    TEST_ASSERT_EQUAL_STRING("message", j3dCoreTest_aLastPrintChannel);

    J3DLOG_WARNING((&hs), "warning");
    TEST_ASSERT_EQUAL_STRING("warning", j3dCoreTest_aLastPrintChannel);

    J3DLOG_ERROR((&hs), "error");
    TEST_ASSERT_EQUAL_STRING("error", j3dCoreTest_aLastPrintChannel);

    J3DLOG_FATAL((&hs), "fatal");
    TEST_ASSERT_EQUAL_INT(1, j3dCoreTest_numAsserts);
    TEST_ASSERT_EQUAL_STRING("fatal", j3dCoreTest_aLastAssertText);
    TEST_ASSERT_TRUE(j3dCoreTest_lastAssertLine > 0);
}

TEST(j3dMacros, TestAssertMacros)
{
    J3DCoreTestHostServices  hs  = J3DCoreTest_MakeHostServices();
    J3DCoreTestHostServices* pHS = &hs;

    J3D_ASSERTREL(1, pHS);
    TEST_ASSERT_EQUAL_INT(0, j3dCoreTest_numAsserts);

    J3D_ASSERTREL(0, pHS);
    TEST_ASSERT_EQUAL_INT(1, j3dCoreTest_numAsserts);
    TEST_ASSERT_EQUAL_STRING("0", j3dCoreTest_aLastAssertText);
    TEST_ASSERT_TRUE(j3dCoreTest_lastAssertLine > 0);

    J3DCoreTest_ResetCapture();
#ifdef J3D_DEBUG
    J3D_ASSERT(0, pHS);
    TEST_ASSERT_EQUAL_INT(1, j3dCoreTest_numAsserts);
    TEST_ASSERT_EQUAL_STRING("0", j3dCoreTest_aLastAssertText);
#else
    J3D_ASSERT(0, pHS);
    TEST_ASSERT_EQUAL_INT(0, j3dCoreTest_numAsserts);
#endif
}

TEST_GROUP_RUNNER(j3dMacros)
{
    RUN_TEST_CASE(j3dMacros, TestBuildMacros);
    RUN_TEST_CASE(j3dMacros, TestMemoryAndStringEquivalenceMacros);
    RUN_TEST_CASE(j3dMacros, TestCaseInsensitiveStringMacros);
    RUN_TEST_CASE(j3dMacros, TestUtilityMacros);
    RUN_TEST_CASE(j3dMacros, TestRuntimeGuardMacros);
    RUN_TEST_CASE(j3dMacros, TestLoggingMacros);
    RUN_TEST_CASE(j3dMacros, TestAssertMacros);
}

void TEST_j3dMacros_GROUP_RUNNER(void);

static void runAllTests(void)
{
    TEST_j3dMacros_GROUP_RUNNER();
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, runAllTests);
}
