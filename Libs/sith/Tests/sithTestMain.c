#include <stdio.h>

// Called by the generated main() of sithTests before the suites run. Unbuffered output, so the last test before a crash
// is visible.
int SithTest_Init(int argc, const char* argv[], int* pExitCode)
{
    (void)argc;
    (void)argv;
    (void)pExitCode;
    setvbuf(stdout, NULL, _IONBF, 0);
    return 0;
}
