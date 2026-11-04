#include <unity_fixture.h>

#include <j3dcore/j3d.h>

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
void TEST_stdRenderCapture_GROUP_RUNNER(void);
void TEST_stdFileUtil_GROUP_RUNNER(void);
void TEST_stdJSON_GROUP_RUNNER(void);
void TEST_stdStrTable_GROUP_RUNNER(void);
#if defined(J3D_PLATFORM_NULL)
// Added: the non-Windows columns' simulation cvar seam (ADR-0020), stdCvarNull.c; the Babylon
// column's replacement is covered by stdCvarBabylon.cpp in stdBabylonTests.
void TEST_stdCvarNull_GROUP_RUNNER(void);
// Added: the renderer forwarding layer recording; it is only built on the Null platform columns.
void TEST_stdRenderCaptureForward_GROUP_RUNNER(void);
#endif
#if !defined(J3D_PLATFORM_NULL)
// Added: these suites are not built on the Null platform: stdBmpTest.c checks GDI HBITMAPs from the
// Win32 stdBmp backend, stdPlatformTest.c uses Windows stack frames, minidumps and a CreateProcessA
// death child, and the whole Win95 backend suite needs windows.h.
void TEST_stdBmp_GROUP_RUNNER(void);
void TEST_stdPlatform_GROUP_RUNNER(void);
void StdWin95Test_RunAllTests(void);
int StdPlatformTest_RunDeathChild(void);
int StdConsoleTest_RunConsoleChild(void);
#endif

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
    TEST_stdRenderCapture_GROUP_RUNNER();
    TEST_stdFileUtil_GROUP_RUNNER();
    TEST_stdJSON_GROUP_RUNNER();
#if !defined(J3D_PLATFORM_NULL)
    TEST_stdBmp_GROUP_RUNNER();
    TEST_stdPlatform_GROUP_RUNNER();
#endif
#if defined(J3D_PLATFORM_NULL)
    TEST_stdCvarNull_GROUP_RUNNER();
    TEST_stdRenderCaptureForward_GROUP_RUNNER();
#endif
    TEST_stdStrTable_GROUP_RUNNER();
#if !defined(J3D_PLATFORM_NULL)
    StdWin95Test_RunAllTests();
#endif
}

// Added: the entry name is overridable so the Babylon column's test main can call this runner
// after it has installed the platform context (J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main); plain
// builds keep main.
#ifndef J3D_TEST_RUNNER_ENTRY
#define J3D_TEST_RUNNER_ENTRY main
#endif

int J3D_TEST_RUNNER_ENTRY(int argc, const char* argv[])
{
#if !defined(J3D_PLATFORM_NULL)
    if ( argc == 2 && strcmp(argv[1], "--std-platform-death-child") == 0 )
    {
        return StdPlatformTest_RunDeathChild();
    }

    if ( argc == 2 && strcmp(argv[1], "--std-console-child") == 0 )
    {
        return StdConsoleTest_RunConsoleChild();
    }
#endif

    return UnityMain(argc, argv, runAllTests);
}
