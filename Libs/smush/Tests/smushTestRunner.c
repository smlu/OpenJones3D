#include <string.h>

int SmushPlayTest_RunChild(const char* pScenario, const char* pDirectory);

// Child process modes of the smush tests, called by the generated main() before the suites run
int SmushTest_HandleChildMode(int argc, const char* argv[], int* pExitCode)
{
    if ( argc > 1 && strcmp(argv[1], "--smush-play-child") == 0 )
    {
        *pExitCode = argc == 4 ? SmushPlayTest_RunChild(argv[2], argv[3]) : 2;
        return 1;
    }

    return 0;
}
