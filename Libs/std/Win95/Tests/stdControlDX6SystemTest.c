#include <unity_fixture.h>

#include <std/General/stdUtil.h>
#include <std/Win95/stdControl.h>

#include "stdWin95SystemTestSupport.h"

TEST_GROUP(stdControlDX6System);

TEST_SETUP(stdControlDX6System)
{
    StdWin95SystemTest_Startup();
}

TEST_TEAR_DOWN(stdControlDX6System)
{
    StdWin95SystemTest_Shutdown();
}

static void stdControlDX6SystemTest_PumpMessages(void)
{
    MSG msg;

    while ( PeekMessageA(&msg, NULL, 0u, 0u, PM_REMOVE) )
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static void stdControlDX6SystemTest_RequireOpenControl(void)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireControlStartup();

    TEST_ASSERT_TRUE(stdControl_HasStarted());
    TEST_ASSERT_FALSE(stdControl_IsOpen());
    TEST_ASSERT_EQUAL_INT(0, stdControl_Open());

    StdWin95SystemTest_SetControlOpen(true);
    TEST_ASSERT_TRUE(stdControl_IsOpen());
    TEST_ASSERT_EQUAL_INT(1, stdControl_ControlsActive());
}

static bool stdControlDX6SystemTest_SendKeyboardScan(WORD scanCode, bool bPressed)
{
    INPUT input;

    STD_ZEROMEM(&input, sizeof(input));
    input.type       = INPUT_KEYBOARD;
    input.ki.wScan   = scanCode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE | (bPressed ? 0u : KEYEVENTF_KEYUP);

    return SendInput(1u, &input, sizeof(input)) == 1u;
}

static bool stdControlDX6SystemTest_SendMouseInput(DWORD flags, LONG dx, LONG dy)
{
    INPUT input;

    STD_ZEROMEM(&input, sizeof(input));
    input.type       = INPUT_MOUSE;
    input.mi.dx      = dx;
    input.mi.dy      = dy;
    input.mi.dwFlags = flags;

    return SendInput(1u, &input, sizeof(input)) == 1u;
}

static bool stdControlDX6SystemTest_ReadKeyEventually(size_t keyId, bool bExpectedDown, int* pbPressed)
{
    for ( size_t attempt = 0u; attempt < 10u; ++attempt )
    {
        Sleep(25u);
        stdControlDX6SystemTest_PumpMessages();
        stdControl_ReadControls();

        int bPressed = 0;
        int bDown    = stdControl_ReadKey(keyId, &bPressed);
        if ( (bDown != 0) == bExpectedDown )
        {
            if ( pbPressed )
            {
                *pbPressed = bPressed;
            }

            return true;
        }
    }

    return false;
}

static bool stdControlDX6SystemTest_ReadMouseMoveEventually(int* pX, int* pY)
{
    for ( size_t attempt = 0u; attempt < 10u; ++attempt )
    {
        Sleep(25u);
        stdControlDX6SystemTest_PumpMessages();
        stdControl_ReadControls();

        int x = stdControl_ReadAxisRaw(STDCONTROL_AID_MOUSE_X);
        int y = stdControl_ReadAxisRaw(STDCONTROL_AID_MOUSE_Y);
        if ( x != 0 || y != 0 )
        {
            *pX = x;
            *pY = y;
            return true;
        }
    }

    return false;
}

TEST(stdControlDX6System, TestDirectInputStartupOpenAndClose)
{
    stdControlDX6SystemTest_RequireOpenControl();

    stdControl_Close();
    StdWin95SystemTest_SetControlOpen(false);
    TEST_ASSERT_FALSE(stdControl_IsOpen());

    stdControl_Shutdown();
    StdWin95SystemTest_SetControlStarted(false);
    TEST_ASSERT_FALSE(stdControl_HasStarted());
}

TEST(stdControlDX6System, TestActivationReadAndMouseAxisLifecycle)
{
    stdControlDX6SystemTest_RequireOpenControl();

    stdControl_ReadControls();
    TEST_ASSERT_EQUAL_INT(1, stdControl_ControlsIdle());

    stdControl_SetActivation(0);
    TEST_ASSERT_EQUAL_INT(0, stdControl_ControlsActive());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdControl_ReadAxis(STDCONTROL_AID_MOUSE_X));

    stdControl_SetActivation(1);
    TEST_ASSERT_EQUAL_INT(1, stdControl_ControlsActive());
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableMouse(1));
    TEST_ASSERT_TRUE(stdControl_IsMouseEnabled());

    if ( !stdControl_EnableAxis(STDCONTROL_AID_MOUSE_X)
        || !stdControl_EnableAxis(STDCONTROL_AID_MOUSE_Y)
        || !stdControl_EnableAxis(STDCONTROL_AID_MOUSE_Z) )
    {
        TEST_IGNORE_MESSAGE("DirectInput mouse axes unavailable on this host.");
    }

    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_X, STDCONTROL_AXIS_ENABLED));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_Y, STDCONTROL_AXIS_ENABLED));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_Z, STDCONTROL_AXIS_ENABLED));

    stdControl_EnableMouseSensitivity(1);
    stdControl_SetMouseSensitivity(1.25f, 1.5f);
    stdControl_ReadControls();
    stdControl_FinishRead();
    stdControl_DisableReadJoysticks();
}

TEST(stdControlDX6System, TestShowMouseCursorSmoke)
{
    stdControlDX6SystemTest_RequireOpenControl();

    stdControl_ShowMouseCursor(1);
    stdControl_ShowMouseCursor(0);
    stdControl_ShowMouseCursor(1);
}

TEST(stdControlDX6System, TestKeyboardSendInputIsObservedWhenHostAllowsInput)
{
    int bPressed = 0;

    StdWin95SystemTest_RequireForegroundWindow();
    stdControlDX6SystemTest_RequireOpenControl();

    if ( !stdControlDX6SystemTest_SendKeyboardScan(DIK_F11, false) )
    {
        TEST_IGNORE_MESSAGE("SendInput key release was rejected by the host desktop.");
    }

    if ( !stdControlDX6SystemTest_ReadKeyEventually(DIK_F11, false, NULL) )
    {
        TEST_IGNORE_MESSAGE("DirectInput keyboard state already reports the test key as pressed.");
    }

    if ( !stdControlDX6SystemTest_SendKeyboardScan(DIK_F11, true) )
    {
        TEST_IGNORE_MESSAGE("SendInput key press was rejected by the host desktop.");
    }

    if ( !stdControlDX6SystemTest_ReadKeyEventually(DIK_F11, true, &bPressed) )
    {
        stdControlDX6SystemTest_SendKeyboardScan(DIK_F11, false);
        TEST_IGNORE_MESSAGE("DirectInput did not observe the injected keyboard state on this host.");
    }

    TEST_ASSERT_TRUE(bPressed > 0);
    TEST_ASSERT_TRUE(stdControl_ReadKeyAsAxis(DIK_F11) > 0.0f);

    TEST_ASSERT_TRUE(stdControlDX6SystemTest_SendKeyboardScan(DIK_F11, false));
    TEST_ASSERT_TRUE(stdControlDX6SystemTest_ReadKeyEventually(DIK_F11, false, NULL));
}

TEST(stdControlDX6System, TestMouseSendInputIsObservedWhenHostAllowsInput)
{
    int x = 0;
    int y = 0;
    int bPressed = 0;

    StdWin95SystemTest_RequireForegroundWindow();
    stdControlDX6SystemTest_RequireOpenControl();

    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableMouse(1));
    if ( !stdControl_EnableAxis(STDCONTROL_AID_MOUSE_X)
        || !stdControl_EnableAxis(STDCONTROL_AID_MOUSE_Y) )
    {
        TEST_IGNORE_MESSAGE("DirectInput mouse axes unavailable on this host.");
    }

    stdControl_ResetMousePos();
    stdControl_EnableMouseSensitivity(0);

    if ( !stdControlDX6SystemTest_SendMouseInput(MOUSEEVENTF_LEFTUP, 0, 0) )
    {
        TEST_IGNORE_MESSAGE("SendInput mouse release was rejected by the host desktop.");
    }

    if ( !stdControlDX6SystemTest_ReadKeyEventually(STDCONTROL_KID_MOUSE_LBUTTON, false, NULL) )
    {
        TEST_IGNORE_MESSAGE("DirectInput mouse state already reports the test button as pressed.");
    }

    if ( !stdControlDX6SystemTest_SendMouseInput(MOUSEEVENTF_MOVE, 24, -12) )
    {
        TEST_IGNORE_MESSAGE("SendInput mouse move was rejected by the host desktop.");
    }

    if ( !stdControlDX6SystemTest_ReadMouseMoveEventually(&x, &y) )
    {
        TEST_IGNORE_MESSAGE("DirectInput did not observe the injected mouse movement on this host.");
    }

    TEST_ASSERT_TRUE(x != 0 || y != 0);

    if ( !stdControlDX6SystemTest_SendMouseInput(MOUSEEVENTF_LEFTDOWN, 0, 0) )
    {
        TEST_IGNORE_MESSAGE("SendInput mouse button press was rejected by the host desktop.");
    }

    if ( !stdControlDX6SystemTest_ReadKeyEventually(STDCONTROL_KID_MOUSE_LBUTTON, true, &bPressed) )
    {
        stdControlDX6SystemTest_SendMouseInput(MOUSEEVENTF_LEFTUP, 0, 0);
        TEST_IGNORE_MESSAGE("DirectInput did not observe the injected mouse button state on this host.");
    }

    TEST_ASSERT_TRUE(bPressed > 0);

    TEST_ASSERT_TRUE(stdControlDX6SystemTest_SendMouseInput(MOUSEEVENTF_LEFTUP, 0, 0));
    TEST_ASSERT_TRUE(stdControlDX6SystemTest_ReadKeyEventually(STDCONTROL_KID_MOUSE_LBUTTON, false, NULL));
}

TEST_GROUP_RUNNER(stdControlDX6System)
{
    RUN_TEST_CASE(stdControlDX6System, TestDirectInputStartupOpenAndClose);
    RUN_TEST_CASE(stdControlDX6System, TestActivationReadAndMouseAxisLifecycle);
    RUN_TEST_CASE(stdControlDX6System, TestShowMouseCursorSmoke);
    RUN_TEST_CASE(stdControlDX6System, TestKeyboardSendInputIsObservedWhenHostAllowsInput);
    RUN_TEST_CASE(stdControlDX6System, TestMouseSendInputIsObservedWhenHostAllowsInput);
}
