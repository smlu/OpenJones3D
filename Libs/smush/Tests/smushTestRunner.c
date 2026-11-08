#include <unity_fixture.h>
#include <string.h>

void TEST_SmushSound_GROUP_RUNNER(void);
void TEST_SmushCodec_GROUP_RUNNER(void);
void TEST_SmushStream_GROUP_RUNNER(void);
void TEST_SmushZlib_GROUP_RUNNER(void);
void TEST_SmushCore_GROUP_RUNNER(void);
void TEST_SmushPlay_GROUP_RUNNER(void);
int SmushPlayTest_RunChild(const char* pScenario, const char* pDirectory);
void TEST_SmushAsm_GROUP_RUNNER(void);
void TEST_SmushVec_GROUP_RUNNER(void);

static void SmushTest_RunAllTests(void)
{
    TEST_SmushSound_GROUP_RUNNER();
    TEST_SmushCodec_GROUP_RUNNER();
    TEST_SmushStream_GROUP_RUNNER();
    TEST_SmushZlib_GROUP_RUNNER();
    TEST_SmushCore_GROUP_RUNNER();
    TEST_SmushPlay_GROUP_RUNNER();
    TEST_SmushAsm_GROUP_RUNNER();
    TEST_SmushVec_GROUP_RUNNER();
}

int main(int argc, const char* argv[])
{
    if ( argc > 1 && strcmp(argv[1], "--smush-play-child") == 0 )
    {
        return argc == 4 ? SmushPlayTest_RunChild(argv[2], argv[3]) : 2;
    }
    return UnityMain(argc, argv, SmushTest_RunAllTests);
}
