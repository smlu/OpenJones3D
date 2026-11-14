#include <string.h>

int StdPlatformTest_RunDeathChild(void);
int StdPlatformTest_RunStackOverflowChild(void);
int StdPlatformTest_RunNoCrashChild(void);
int StdConsoleTest_RunConsoleChild(void);

// Child process modes of the std tests, called by the generated main() before the suites run
int StdTest_HandleChildMode(int argc, const char* argv[], int* pExitCode)
{
    if ( argc != 2 )
    {
        return 0;
    }

    if ( strcmp(argv[1], "--std-platform-death-child") == 0 )
    {
        *pExitCode = StdPlatformTest_RunDeathChild();
        return 1;
    }

    if ( strcmp(argv[1], "--std-platform-stack-overflow-child") == 0 )
    {
        *pExitCode = StdPlatformTest_RunStackOverflowChild();
        return 1;
    }

    if ( strcmp(argv[1], "--std-platform-no-crash-child") == 0 )
    {
        *pExitCode = StdPlatformTest_RunNoCrashChild();
        return 1;
    }

    if ( strcmp(argv[1], "--std-console-child") == 0 )
    {
        *pExitCode = StdConsoleTest_RunConsoleChild();
        return 1;
    }

    return 0;
}
