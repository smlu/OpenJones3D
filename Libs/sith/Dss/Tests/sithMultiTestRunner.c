#include <unity_fixture.h>

#include <stdio.h>

void TEST_sithMulti_GROUP_RUNNER(void);

static void SithMultiTest_RunAllTests(void)
{
    TEST_sithMulti_GROUP_RUNNER();
}

int main(int argc, const char* argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    return UnityMain(argc, argv, SithMultiTest_RunAllTests);
}
