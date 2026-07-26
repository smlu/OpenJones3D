#include <unity_fixture.h>

#include <string.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/Win95/stdWin95.h>

#include "stdGeneralTest.h"

TEST_GROUP(stdWin95);

TEST_SETUP(stdWin95)
{
    StdGeneralTest_Startup();
    stdWin95_SetWindow(NULL);
    stdWin95_SetInstance(NULL);

    GUID guid = { 0 };
    stdWin95_SetGuid(&guid);
}

TEST_TEAR_DOWN(stdWin95)
{
    stdWin95_SetWindow(NULL);
    stdWin95_SetInstance(NULL);
    StdGeneralTest_Shutdown();
}

TEST(stdWin95, TestWindowAndInstanceAccessors)
{
    HWND hwnd             = (HWND)(uintptr_t)0x1234u;
    HINSTANCE hInstance  = (HINSTANCE)(uintptr_t)0x5678u;

    stdWin95_SetWindow(hwnd);
    stdWin95_SetInstance(hInstance);

    TEST_ASSERT_EQUAL_PTR(hwnd, stdWin95_GetWindow());
    TEST_ASSERT_EQUAL_PTR(hInstance, stdWin95_GetInstance());
}

TEST(stdWin95, TestGuidAccessorCopiesInput)
{
    GUID guid =
    {
        0x12345678u,
        0x9ABCu,
        0xDEF0u,
        { 0x11u, 0x22u, 0x33u, 0x44u, 0x55u, 0x66u, 0x77u, 0x88u }
    };
    GUID changedGuid = guid;

    stdWin95_SetGuid(&guid);
    changedGuid.Data1 = 0u;

    TEST_ASSERT_NOT_NULL(stdWin95_GetGuid());
    TEST_ASSERT_EQUAL_MEMORY(&guid, stdWin95_GetGuid(), sizeof(guid));
    TEST_ASSERT_NOT_EQUAL(changedGuid.Data1, stdWin95_GetGuid()->Data1);
}

TEST(stdWin95, TestGuidAccessorKeepsStableStorageAndLatestValue)
{
    GUID firstGuid =
    {
        0x11111111u,
        0x2222u,
        0x3333u,
        { 0x44u, 0x55u, 0x66u, 0x77u, 0x88u, 0x99u, 0xAAu, 0xBBu }
    };
    GUID secondGuid =
    {
        0x01020304u,
        0x0506u,
        0x0708u,
        { 0x09u, 0x0Au, 0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u }
    };

    stdWin95_SetGuid(&firstGuid);
    const GUID* pGuid = stdWin95_GetGuid();

    TEST_ASSERT_EQUAL_MEMORY(&firstGuid, pGuid, sizeof(firstGuid));

    stdWin95_SetGuid(&secondGuid);

    TEST_ASSERT_EQUAL_PTR(pGuid, stdWin95_GetGuid());
    TEST_ASSERT_EQUAL_MEMORY(&secondGuid, stdWin95_GetGuid(), sizeof(secondGuid));
}

TEST_GROUP_RUNNER(stdWin95)
{
    RUN_TEST_CASE(stdWin95, TestWindowAndInstanceAccessors);
    RUN_TEST_CASE(stdWin95, TestGuidAccessorCopiesInput);
    RUN_TEST_CASE(stdWin95, TestGuidAccessorKeepsStableStorageAndLatestValue);
}
