#include <unity_fixture.h>

#include <j3dcore/j3d.h>

void TEST_stdWin95System_GROUP_RUNNER(void);
#if defined(J3D_DIRECTX9)
void TEST_stdControlDX9System_GROUP_RUNNER(void);
void TEST_stdDisplayDX9System_GROUP_RUNNER(void);
void TEST_std3DX9System_GROUP_RUNNER(void);
void TEST_stdShaderDX9System_GROUP_RUNNER(void);
#elif defined(J3D_DIRECTX6)
void TEST_stdControlDX6System_GROUP_RUNNER(void);
void TEST_stdDisplayDX6System_GROUP_RUNNER(void);
void TEST_std3DX6System_GROUP_RUNNER(void);
#endif

static void StdSystemTest_RunAllTests(void)
{
    TEST_stdWin95System_GROUP_RUNNER();
#if defined(J3D_DIRECTX9)
    TEST_stdControlDX9System_GROUP_RUNNER();
    TEST_stdDisplayDX9System_GROUP_RUNNER();
    TEST_std3DX9System_GROUP_RUNNER();
    TEST_stdShaderDX9System_GROUP_RUNNER();
#elif defined(J3D_DIRECTX6)
    TEST_stdControlDX6System_GROUP_RUNNER();
    TEST_stdDisplayDX6System_GROUP_RUNNER();
    TEST_std3DX6System_GROUP_RUNNER();
#endif
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, StdSystemTest_RunAllTests);
}
