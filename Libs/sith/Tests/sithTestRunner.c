#include <unity_fixture.h>

#include <stdio.h>

void TEST_sithTemplate_GROUP_RUNNER(void);
void TEST_sithThingBinary_GROUP_RUNNER(void);
void TEST_sithVoice_GROUP_RUNNER(void);

static void SithTest_RunAllTests(void)
{
    TEST_sithThingBinary_GROUP_RUNNER();
    TEST_sithTemplate_GROUP_RUNNER();
    TEST_sithVoice_GROUP_RUNNER();
}

int main(int argc, const char* argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    return UnityMain(argc, argv, SithTest_RunAllTests);
}
