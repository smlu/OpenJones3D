#include <unity_fixture.h>

void TEST_SmushSound_GROUP_RUNNER(void);
void TEST_SmushCodec_GROUP_RUNNER(void);
void TEST_SmushStream_GROUP_RUNNER(void);
void TEST_SmushCore_GROUP_RUNNER(void);
void TEST_SmushAsm_GROUP_RUNNER(void);
void TEST_SmushVec_GROUP_RUNNER(void);

static void SmushTest_RunAllTests(void)
{
    TEST_SmushSound_GROUP_RUNNER();
    TEST_SmushCodec_GROUP_RUNNER();
    TEST_SmushStream_GROUP_RUNNER();
    TEST_SmushCore_GROUP_RUNNER();
    TEST_SmushAsm_GROUP_RUNNER();
    TEST_SmushVec_GROUP_RUNNER();
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, SmushTest_RunAllTests);
}
