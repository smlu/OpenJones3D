#include <unity_fixture.h>
#include <j3dcore/j3d.h>

void TEST_stdGob_GROUP_RUNNER(void);
void TEST_std3D_GROUP_RUNNER(void);
void TEST_std3DTestVector_GROUP_RUNNER(void);
#if defined(J3D_DIRECTX9)
void TEST_stdComm_GROUP_RUNNER(void);
#elif defined(J3D_DIRECTX6)
void TEST_stdCommDX6_GROUP_RUNNER(void);
#endif
void TEST_stdConsole_GROUP_RUNNER(void);
void TEST_stdControl_GROUP_RUNNER(void);
void TEST_stdDisplay_GROUP_RUNNER(void);
void TEST_stdShader_GROUP_RUNNER(void);
void TEST_stdWin95_GROUP_RUNNER(void);
#if defined(J3D_DIRECTX9)
void TEST_stdWin95DirectX9_GROUP_RUNNER(void);
#elif defined(J3D_DIRECTX6)
void TEST_stdWin95DirectX6_GROUP_RUNNER(void);
#endif

void StdWin95Test_RunAllTests(void)
{
    TEST_stdGob_GROUP_RUNNER();
    TEST_std3D_GROUP_RUNNER();
    TEST_std3DTestVector_GROUP_RUNNER();
#if defined(J3D_DIRECTX9)
    TEST_stdComm_GROUP_RUNNER();
#elif defined(J3D_DIRECTX6)
    TEST_stdCommDX6_GROUP_RUNNER();
#endif
    TEST_stdConsole_GROUP_RUNNER();
    TEST_stdControl_GROUP_RUNNER();
    TEST_stdDisplay_GROUP_RUNNER();
    TEST_stdShader_GROUP_RUNNER();
    TEST_stdWin95_GROUP_RUNNER();
#if defined(J3D_DIRECTX9)
    TEST_stdWin95DirectX9_GROUP_RUNNER();
#elif defined(J3D_DIRECTX6)
    TEST_stdWin95DirectX6_GROUP_RUNNER();
#endif
}
