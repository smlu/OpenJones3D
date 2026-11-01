#include <unity_fixture.h>

#include <stdio.h>

// Defined by the test source of the suite
void SithVecTest_RunAllTests(void);

int main(int argc, const char* argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    return UnityMain(argc, argv, SithVecTest_RunAllTests);
}
