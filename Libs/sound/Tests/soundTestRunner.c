#include <unity_fixture.h>

void TEST_AudioLib_GROUP_RUNNER(void);
void TEST_Sound_GROUP_RUNNER(void);

static void SoundTest_RunAllTests(void)
{
    TEST_AudioLib_GROUP_RUNNER();
    TEST_Sound_GROUP_RUNNER();
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, SoundTest_RunAllTests);
}
