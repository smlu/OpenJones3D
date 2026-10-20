#include <unity_fixture.h>

#include <stdio.h>

void TEST_sithDSS_GROUP_RUNNER(void);
void TEST_sithDSSThing_GROUP_RUNNER(void);

static void SithDSSTest_RunAllTests(void)
{
    TEST_sithDSS_GROUP_RUNNER();
    TEST_sithDSSThing_GROUP_RUNNER();
}

int main(int argc, const char* argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    return UnityMain(argc, argv, SithDSSTest_RunAllTests);
}
