#include <unity_fixture.h>

#include <stdio.h>
#include <string.h>

#include <std/General/stdUtil.h>

#include <std/Win95/stdConsole.h>

#include "stdGeneralTest.h"

int J3DAPI stdConsole_WriteConsoleReal(const char* pText, uint32_t textAttribute);

// Use a separate process so console startup/shutdown cannot redirect the Unity runner.
int StdConsoleTest_RunConsoleChild(void)
{
    const char* pTitle = "OpenJones3D stdConsole child test";
    WORD attributes = FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE;
    if ( stdConsole_Startup(pTitle, attributes, 1) != 1 ) return 10;
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if ( !GetConsoleScreenBufferInfo(hOutput, &info) )
    {
        stdConsole_Shutdown();
        return 77; // Host has no usable native console.
    }

    // Startup must set the exact title and attributes, and clear the full console buffer.
    char aTitle[128];
    if ( GetConsoleTitleA(aTitle, sizeof(aTitle)) != strlen(pTitle) || strcmp(pTitle, aTitle) != 0 ) return 11;
    if ( info.wAttributes != attributes ) return 12;

    COORD origin = { 0, 0 };
    char aCleared[2001];
    char aExpected[2001];
    DWORD numRead = 0u;
    if ( !ReadConsoleOutputCharacterA(hOutput, aCleared, 2000u, origin, &numRead) || numRead != 2000u ) return 13;
    aCleared[2000] = '\0';
    memset(aExpected, ' ', 2000u);
    aExpected[2000] = '\0';
    if ( strcmp(aExpected, aCleared) != 0 ) return 14;

    // Read back literal text and every character attribute after a successful native write.
    if ( !SetConsoleCursorPosition(hOutput, origin) ) return 15;
    WORD textAttributes = FOREGROUND_RED | FOREGROUND_INTENSITY;
    const char* pText = "console output: literal %s and %n";
    if ( stdConsole_WriteConsoleReal(pText, textAttributes) != 1 ) return 16;
    char aReadText[128];
    if ( !ReadConsoleOutputCharacterA(hOutput, aReadText, (DWORD)strlen(pText), origin, &numRead) || numRead != strlen(pText) ) return 17;
    aReadText[numRead] = '\0';
    if ( strcmp(pText, aReadText) != 0 ) return 18;
    WORD aWrittenAttributes[128];
    if ( !ReadConsoleOutputAttribute(hOutput, aWrittenAttributes, (DWORD)strlen(pText), origin, &numRead) || numRead != strlen(pText) ) return 19;
    for ( size_t i = 0u; i < numRead; ++i )
    {
        if ( aWrittenAttributes[i] != textAttributes ) return 20;
    }

    // Pending attributes and explicit text attributes must reach the native screen buffer.
    if ( stdConsole_SetAttributes(FOREGROUND_BLUE) != 1 || stdConsole_InitOutputConsole() != 1 ) return 21;
    if ( !GetConsoleScreenBufferInfo(hOutput, &info) || info.wAttributes != FOREGROUND_BLUE ) return 22;
    if ( stdConsole_SetConsoleTextAttribute(FOREGROUND_GREEN) != 1 ) return 23;
    if ( !GetConsoleScreenBufferInfo(hOutput, &info) || info.wAttributes != FOREGROUND_GREEN ) return 24;

    // After shutdown, writes and attribute changes must fail safely.
    stdConsole_Shutdown();
    if ( stdConsole_SetConsoleTextAttribute(FOREGROUND_RED) != 0 ) return 25;
    if ( stdConsole_WriteConsoleReal("after shutdown", FOREGROUND_RED) != 0 ) return 26;
    return 0;
}

TEST_GROUP(stdConsole);

TEST_SETUP(stdConsole)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdConsole)
{
    StdGeneralTest_Shutdown();
}

TEST(stdConsole, TestSetAttributesStoresPendingAttributes)
{
    TEST_ASSERT_EQUAL_INT(1, stdConsole_SetAttributes(FOREGROUND_RED | FOREGROUND_INTENSITY));
    TEST_ASSERT_EQUAL_INT(0, stdConsole_InitOutputConsole());
}

TEST(stdConsole, TestSetConsoleTextAttributeWithoutStartupFailsSafely)
{
    TEST_ASSERT_EQUAL_INT(0, stdConsole_SetConsoleTextAttribute(FOREGROUND_GREEN));
    TEST_ASSERT_EQUAL_INT(0, stdConsole_WriteConsoleReal("test", FOREGROUND_GREEN));
    TEST_ASSERT_EQUAL_INT(0, stdConsole_WriteConsoleReal("", FOREGROUND_BLUE));
}

TEST(stdConsole, TestNativeConsoleLifecycleAndExactOutputsInChildProcess)
{
    // Run console lifecycle checks in a child so the main test runner retains its output handles.
    char aExecutable[MAX_PATH];
    DWORD pathLength = GetModuleFileNameA(NULL, aExecutable, STD_ARRAYLEN(aExecutable));
    TEST_ASSERT_TRUE(pathLength > 0u && pathLength < STD_ARRAYLEN(aExecutable));
    char aCommand[MAX_PATH * 2u];
    STD_FORMAT(aCommand, "\"%s\" --std-console-child", aExecutable);
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    STD_ZEROMEM(&startup, sizeof(startup));
    STD_ZEROMEM(&process, sizeof(process));
    startup.cb = sizeof(startup);
    TEST_ASSERT_TRUE(CreateProcessA(NULL, aCommand, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startup, &process));
    // Bound the child lifetime, close its handles, and propagate a failed native check.
    DWORD waitResult = WaitForSingleObject(process.hProcess, 30000u);
    if ( waitResult != WAIT_OBJECT_0 )
    {
        TerminateProcess(process.hProcess, 99u);
        WaitForSingleObject(process.hProcess, 5000u);
    }
    DWORD exitCode = 99u;
    BOOL bGotExitCode = GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    TEST_ASSERT_EQUAL_UINT32(WAIT_OBJECT_0, waitResult);
    TEST_ASSERT_TRUE(bGotExitCode);
    if ( exitCode == 77u ) TEST_IGNORE_MESSAGE("The host could not allocate a usable native console.");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0u, exitCode, "Native console child failed; exit code identifies its failed check.");
}

TEST_GROUP_RUNNER(stdConsole)
{
    RUN_TEST_CASE(stdConsole, TestNativeConsoleLifecycleAndExactOutputsInChildProcess);
    RUN_TEST_CASE(stdConsole, TestSetAttributesStoresPendingAttributes);
    RUN_TEST_CASE(stdConsole, TestSetConsoleTextAttributeWithoutStartupFailsSafely);
}
