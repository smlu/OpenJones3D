#include <unity_fixture.h>

#include <j3dcore/j3d.h>

#ifdef J3D_DIRECTX9
void TEST_stdDisplayDX9DisruptiveSystem_GROUP_RUNNER(void);
#elif defined(J3D_DIRECTX6)
void TEST_stdDisplayDX6DisruptiveSystem_GROUP_RUNNER(void);
#endif

static void RunAllTests(void)
{
#ifdef J3D_DIRECTX9
    RUN_TEST_GROUP(stdDisplayDX9DisruptiveSystem);
#elif defined(J3D_DIRECTX6)
    RUN_TEST_GROUP(stdDisplayDX6DisruptiveSystem);
#endif
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, RunAllTests);
}
