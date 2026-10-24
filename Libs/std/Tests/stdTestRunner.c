#include <unity_fixture.h>

#include <string.h>

void TEST_std_GROUP_RUNNER(void);
void TEST_stdMemory_GROUP_RUNNER(void);
void TEST_stdUtil_GROUP_RUNNER(void);
void TEST_stdMath_GROUP_RUNNER(void);
void TEST_stdFnames_GROUP_RUNNER(void);
void TEST_stdLinkList_GROUP_RUNNER(void);
void TEST_stdHashtbl_GROUP_RUNNER(void);
void TEST_stdEffect_GROUP_RUNNER(void);
void TEST_stdColor_GROUP_RUNNER(void);
void TEST_stdCircBuf_GROUP_RUNNER(void);
void TEST_stdConffile_GROUP_RUNNER(void);
void TEST_stdConfig_GROUP_RUNNER(void);
void TEST_stdFileUtil_GROUP_RUNNER(void);
void TEST_stdJSON_GROUP_RUNNER(void);
void TEST_stdBmp_GROUP_RUNNER(void);
void TEST_stdPlatform_GROUP_RUNNER(void);
void TEST_stdStrTable_GROUP_RUNNER(void);
void StdWin95Test_RunAllTests(void);
int StdPlatformTest_RunDeathChild(void);
int StdConsoleTest_RunConsoleChild(void);

static void runAllTests(void)
{
    TEST_std_GROUP_RUNNER();
    TEST_stdMemory_GROUP_RUNNER();
    TEST_stdUtil_GROUP_RUNNER();
    TEST_stdMath_GROUP_RUNNER();
    TEST_stdFnames_GROUP_RUNNER();
    TEST_stdLinkList_GROUP_RUNNER();
    TEST_stdHashtbl_GROUP_RUNNER();
    TEST_stdEffect_GROUP_RUNNER();
    TEST_stdColor_GROUP_RUNNER();
    TEST_stdCircBuf_GROUP_RUNNER();
    TEST_stdConffile_GROUP_RUNNER();
    TEST_stdConfig_GROUP_RUNNER();
    TEST_stdFileUtil_GROUP_RUNNER();
    TEST_stdJSON_GROUP_RUNNER();
    TEST_stdBmp_GROUP_RUNNER();
    TEST_stdPlatform_GROUP_RUNNER();
    TEST_stdStrTable_GROUP_RUNNER();
    StdWin95Test_RunAllTests();
}

int main(int argc, const char* argv[])
{
    if ( argc == 2 && strcmp(argv[1], "--std-platform-death-child") == 0 )
    {
        return StdPlatformTest_RunDeathChild();
    }

    if ( argc == 2 && strcmp(argv[1], "--std-console-child") == 0 )
    {
        return StdConsoleTest_RunConsoleChild();
    }

    return UnityMain(argc, argv, runAllTests);
}
