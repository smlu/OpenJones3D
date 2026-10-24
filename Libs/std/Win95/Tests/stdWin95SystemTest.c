#include <unity_fixture.h>

#include <std/Win95/stdWin95.h>

#include "stdWin95SystemTestSupport.h"

TEST_GROUP(stdWin95System);

TEST_SETUP(stdWin95System)
{
    StdWin95SystemTest_Startup();
}

TEST_TEAR_DOWN(stdWin95System)
{
    StdWin95SystemTest_Shutdown();
}

TEST(stdWin95System, TestHiddenWindowAndWin95Accessors)
{
    RECT clientRect;

    StdWin95SystemTest_RequireHiddenWindow();

    TEST_ASSERT_EQUAL_PTR(StdWin95SystemTest_GetInstance(), stdWin95_GetInstance());
    TEST_ASSERT_EQUAL_PTR(StdWin95SystemTest_GetWindow(), stdWin95_GetWindow());
    TEST_ASSERT_TRUE(GetClientRect(StdWin95SystemTest_GetWindow(), &clientRect));
    TEST_ASSERT_TRUE(clientRect.right > clientRect.left);
    TEST_ASSERT_TRUE(clientRect.bottom > clientRect.top);
}

TEST_GROUP_RUNNER(stdWin95System)
{
    RUN_TEST_CASE(stdWin95System, TestHiddenWindowAndWin95Accessors);
}
