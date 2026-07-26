#include <unity_fixture.h>

#include <stdint.h>

#include <std/General/stdUtil.h>
#include <std/Win95/stdControl.h>

#include "stdGeneralTest.h"

const char* J3DAPI stdControl_DIGetStatus(int status);

#ifdef J3D_DIRECTX9
float J3DAPI stdControl_ApplyXInputDeadzone(SHORT value, float deadzone);
void J3DAPI stdControl_SetXInputVibration(int controllerIndex, float leftMotor, float rightMotor);
#endif

TEST_GROUP(stdControl);

TEST_SETUP(stdControl)
{
    StdGeneralTest_Startup();
    stdControl_Close();
    stdControl_Shutdown();
    stdControl_TestResetInputState();
    stdControl_Reset();
    stdControl_EnableMouse(0);
    stdControl_EnableMouseSensitivity(0);
}

TEST_TEAR_DOWN(stdControl)
{
    stdControl_Close();
    stdControl_Shutdown();
    stdControl_TestResetInputState();
    stdControl_Reset();
    stdControl_EnableMouse(0);
    stdControl_EnableMouseSensitivity(0);
    StdGeneralTest_Shutdown();
}

#define STDCONTROLTEST_FORMAT_MESSAGE(message, label, value) \
    STD_FORMAT(message, "%s %u", label, value)

static bool stdControlTest_IsExpectedKeyboardKey(uint32_t keyId)
{
    return keyId < STDCONTROL_MAX_KEYBOARD_BUTTONS;
}

static bool stdControlTest_IsExpectedJoystickKey(uint32_t keyId)
{
    return keyId >= STDCONTROL_JOYSTICK_FIRSTCID
        && keyId < STDCONTROL_JOYSTICK_FIRSTCID + STDCONTROL_JOYSTICK_TOTALCIDS;
}

static bool stdControlTest_IsExpectedJoystickButton(uint32_t keyId)
{
    return stdControlTest_IsExpectedJoystickKey(keyId)
        && ((keyId - STDCONTROL_JOYSTICK_FIRSTCID) % STDCONTROL_TOTAL_JOYSTICK_CONTROLS) < STDCONTROL_NUM_JOYSTICK_BUTTONS;
}

static bool stdControlTest_IsExpectedJoystickPov(uint32_t keyId)
{
    return stdControlTest_IsExpectedJoystickKey(keyId)
        && ((keyId - STDCONTROL_JOYSTICK_FIRSTCID) % STDCONTROL_TOTAL_JOYSTICK_CONTROLS) >= STDCONTROL_NUM_JOYSTICK_BUTTONS;
}

static bool stdControlTest_IsExpectedMouseButton(uint32_t keyId)
{
    return keyId >= STDCONTROL_KID_MOUSE_LBUTTON && keyId <= STDCONTROL_KID_MOUSE_XBUTTON;
}

static void stdControlTest_RegisterJoystickAxes(size_t joyNum, int min, int max)
{
    for ( size_t axisIndex = 0u; axisIndex < STDCONTROL_JOYSTICK_NUMAXES; ++axisIndex )
    {
        size_t aid = STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axisIndex);

        stdControl_RegisterAxis(aid, min, max, 0.0f);
        TEST_ASSERT_EQUAL_INT(1, stdControl_EnableAxis((int)aid));
    }
}

static void stdControlTest_AssertPovState(size_t joyNum, size_t povNum, bool bLeft, bool bUp, bool bRight, bool bDown)
{
    TEST_ASSERT_EQUAL_INT(bLeft, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETPOVLEFT(joyNum, povNum), NULL));
    TEST_ASSERT_EQUAL_INT(bUp, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETPOVUP(joyNum, povNum), NULL));
    TEST_ASSERT_EQUAL_INT(bRight, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETPOVRIGHT(joyNum, povNum), NULL));
    TEST_ASSERT_EQUAL_INT(bDown, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETPOVDOWN(joyNum, povNum), NULL));
}

TEST(stdControl, TestAllKeyIdsClassifyAsKeyboardJoystickOrMouse)
{
    size_t numKeyboardKeys = 0;
    size_t numJoyButtons   = 0;
    size_t numJoyPovs      = 0;
    size_t numMouseButtons = 0;

    for ( uint32_t keyId = 0; keyId < STDCONTROL_MAX_KEYID; ++keyId )
    {
        char aMessage[64];
        STDCONTROLTEST_FORMAT_MESSAGE(aMessage, "key id", keyId);

        bool bKeyboard = stdControlTest_IsExpectedKeyboardKey(keyId);
        bool bJoyBtn   = stdControlTest_IsExpectedJoystickButton(keyId);
        bool bJoyPov   = stdControlTest_IsExpectedJoystickPov(keyId);
        bool bMouse    = stdControlTest_IsExpectedMouseButton(keyId);

        TEST_ASSERT_EQUAL_INT_MESSAGE(bKeyboard, !!STDCONTROL_ISKEYBOARDBUTTON(keyId), aMessage);
        TEST_ASSERT_EQUAL_INT_MESSAGE(bJoyBtn, !!STDCONTROL_ISJOYSTICKBUTTON(keyId), aMessage);
        TEST_ASSERT_EQUAL_INT_MESSAGE(bJoyPov, !!STDCONTROL_ISJOYSTICKPOV(keyId), aMessage);
        TEST_ASSERT_EQUAL_INT_MESSAGE(bMouse, !!STDCONTROL_ISMOUSEBUTTON(keyId), aMessage);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)bKeyboard + (int)bJoyBtn + (int)bJoyPov + (int)bMouse, aMessage);

        numKeyboardKeys += bKeyboard ? 1u : 0u;
        numJoyButtons   += bJoyBtn ? 1u : 0u;
        numJoyPovs      += bJoyPov ? 1u : 0u;
        numMouseButtons += bMouse ? 1u : 0u;
    }

    TEST_ASSERT_EQUAL_size_t(STDCONTROL_MAX_KEYBOARD_BUTTONS, numKeyboardKeys);
    TEST_ASSERT_EQUAL_size_t(STDCONTROL_MAX_JOYSTICK_DEVICES * STDCONTROL_NUM_JOYSTICK_BUTTONS, numJoyButtons);
    TEST_ASSERT_EQUAL_size_t(STDCONTROL_MAX_JOYSTICK_DEVICES * STDCONTROL_MAX_JOYSTICK_POVCONTROLERS * STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS, numJoyPovs);
    TEST_ASSERT_EQUAL_size_t(STDCONTROL_MAX_MOUSE_BUTTONS, numMouseButtons);
    TEST_ASSERT_EQUAL_size_t(STDCONTROL_MAX_KEYID, numKeyboardKeys + numJoyButtons + numJoyPovs + numMouseButtons);
}

TEST(stdControl, TestAllJoystickButtonIds)
{
    for ( uint32_t joyNum = 0; joyNum < STDCONTROL_MAX_JOYSTICK_DEVICES; ++joyNum )
    {
        for ( uint32_t btnNum = 0; btnNum < STDCONTROL_NUM_JOYSTICK_BUTTONS; ++btnNum )
        {
            char aMessage[64];
            uint32_t keyId     = STDCONTROL_JOYSTICK_GETBUTTON(joyNum, btnNum);
            uint32_t expected  = STDCONTROL_JOYSTICK_FIRSTKID + joyNum * STDCONTROL_TOTAL_JOYSTICK_CONTROLS + btnNum;

            STDCONTROLTEST_FORMAT_MESSAGE(aMessage, "joy button", keyId);

            TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected, keyId, aMessage);
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(btnNum, STDCONTROL_JOYSTICK_GETBUTTONINDEX(keyId), aMessage);
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(btnNum, STDCONTROL_JOYSTICK_GETLOCALCID(keyId), aMessage);
            TEST_ASSERT_TRUE_MESSAGE(STDCONTROL_ISJOYSTICKBUTTON(keyId), aMessage);
            TEST_ASSERT_FALSE_MESSAGE(STDCONTROL_ISJOYSTICKPOV(keyId), aMessage);
            TEST_ASSERT_FALSE_MESSAGE(STDCONTROL_ISKEYBOARDBUTTON(keyId), aMessage);
            TEST_ASSERT_FALSE_MESSAGE(STDCONTROL_ISMOUSEBUTTON(keyId), aMessage);
        }
    }
}

TEST(stdControl, TestAllJoystickPovIds)
{
    for ( uint32_t joyNum = 0; joyNum < STDCONTROL_MAX_JOYSTICK_DEVICES; ++joyNum )
    {
        for ( uint32_t povNum = 0; povNum < STDCONTROL_MAX_JOYSTICK_POVCONTROLERS; ++povNum )
        {
            for ( uint32_t dirNum = 0; dirNum < STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS; ++dirNum )
            {
                char aMessage[64];
                uint32_t keyId    = STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, dirNum);
                uint32_t expected = STDCONTROL_JOYSTICK_FIRTPOVCID
                    + joyNum * STDCONTROL_TOTAL_JOYSTICK_CONTROLS
                    + povNum * STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS
                    + dirNum;

                STDCONTROLTEST_FORMAT_MESSAGE(aMessage, "joy pov", keyId);

                TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected, keyId, aMessage);
                TEST_ASSERT_EQUAL_UINT32_MESSAGE(povNum, STDCONTROL_JOYSTICK_GETPOVINDEX(keyId), aMessage);
                TEST_ASSERT_EQUAL_UINT32_MESSAGE(dirNum, STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(keyId), aMessage);
                TEST_ASSERT_EQUAL_UINT32_MESSAGE(STDCONTROL_NUM_JOYSTICK_BUTTONS + povNum * STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS + dirNum, STDCONTROL_JOYSTICK_GETLOCALCID(keyId), aMessage);
                TEST_ASSERT_TRUE_MESSAGE(STDCONTROL_ISJOYSTICKPOV(keyId), aMessage);
                TEST_ASSERT_FALSE_MESSAGE(STDCONTROL_ISJOYSTICKBUTTON(keyId), aMessage);
                TEST_ASSERT_FALSE_MESSAGE(STDCONTROL_ISKEYBOARDBUTTON(keyId), aMessage);
                TEST_ASSERT_FALSE_MESSAGE(STDCONTROL_ISMOUSEBUTTON(keyId), aMessage);

                switch ( dirNum )
                {
                    case STDCONTROL_JOYSTICK_POVDIRLEFT:
                        TEST_ASSERT_EQUAL_UINT32_MESSAGE(keyId, STDCONTROL_JOYSTICK_GETPOVLEFT(joyNum, povNum), aMessage);
                        break;
                    case STDCONTROL_JOYSTICK_POVDIRUP:
                        TEST_ASSERT_EQUAL_UINT32_MESSAGE(keyId, STDCONTROL_JOYSTICK_GETPOVUP(joyNum, povNum), aMessage);
                        break;
                    case STDCONTROL_JOYSTICK_POVDIRRIGHT:
                        TEST_ASSERT_EQUAL_UINT32_MESSAGE(keyId, STDCONTROL_JOYSTICK_GETPOVRIGHT(joyNum, povNum), aMessage);
                        break;
                    case STDCONTROL_JOYSTICK_POVDIRDOWN:
                        TEST_ASSERT_EQUAL_UINT32_MESSAGE(keyId, STDCONTROL_JOYSTICK_GETPOVDOWN(joyNum, povNum), aMessage);
                        break;
                }
            }
        }
    }
}

TEST(stdControl, TestAllAxisIdsAndFlaggedAxisIds)
{
    static const uint32_t aAxisFlags[] =
    {
        0u,
        STDCONTROL_AID_LOW_SENSITIVITY,
        STDCONTROL_AID_POSITIVE_AXIS,
        STDCONTROL_AID_NEGATIVE_AXIS,
        STDCONTROL_AID_LOW_SENSITIVITY | STDCONTROL_AID_POSITIVE_AXIS,
        STDCONTROL_AID_LOW_SENSITIVITY | STDCONTROL_AID_NEGATIVE_AXIS,
    };

    for ( uint32_t joyNum = 0; joyNum < STDCONTROL_MAX_JOYSTICK_DEVICES; ++joyNum )
    {
        for ( uint32_t axisIdx = 0; axisIdx < STDCONTROL_JOYSTICK_NUMAXES; ++axisIdx )
        {
            char aMessage[64];
            uint32_t aid = STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axisIdx);

            STDCONTROLTEST_FORMAT_MESSAGE(aMessage, "joy axis", aid);

            TEST_ASSERT_EQUAL_UINT32_MESSAGE(joyNum * STDCONTROL_JOYSTICK_NUMAXES + axisIdx, aid, aMessage);
            TEST_ASSERT_TRUE_MESSAGE(STDCONTROL_ISJOYSTICKAXIS(aid), aMessage);
            TEST_ASSERT_FALSE_MESSAGE(STDCONTROL_ISMOUSEAXIS(aid), aMessage);

            for ( size_t i = 0; i < STD_ARRAYLEN(aAxisFlags); ++i )
            {
                uint32_t flaggedAid = aid | aAxisFlags[i];

                TEST_ASSERT_EQUAL_UINT32_MESSAGE(aid, STDCONTROL_GETAID(flaggedAid), aMessage);
                TEST_ASSERT_TRUE_MESSAGE(STDCONTROL_ISJOYSTICKAXIS(flaggedAid), aMessage);
                TEST_ASSERT_FALSE_MESSAGE(STDCONTROL_ISMOUSEAXIS(flaggedAid), aMessage);
            }
        }
    }

    for ( uint32_t axisIdx = 0; axisIdx < 3u; ++axisIdx )
    {
        char aMessage[64];
        uint32_t aid = STDCONTROL_AID_MOUSE_X + axisIdx;

        STDCONTROLTEST_FORMAT_MESSAGE(aMessage, "mouse axis", aid);

        TEST_ASSERT_TRUE_MESSAGE(STDCONTROL_ISMOUSEAXIS(aid), aMessage);
        TEST_ASSERT_FALSE_MESSAGE(STDCONTROL_ISJOYSTICKAXIS(aid), aMessage);

        for ( size_t i = 0; i < STD_ARRAYLEN(aAxisFlags); ++i )
        {
            uint32_t flaggedAid = aid | aAxisFlags[i];

            TEST_ASSERT_EQUAL_UINT32_MESSAGE(aid, STDCONTROL_GETAID(flaggedAid), aMessage);
            TEST_ASSERT_TRUE_MESSAGE(STDCONTROL_ISMOUSEAXIS(flaggedAid), aMessage);
            TEST_ASSERT_FALSE_MESSAGE(STDCONTROL_ISJOYSTICKAXIS(flaggedAid), aMessage);
        }
    }
}

TEST(stdControl, TestNamedJoystickAxisMacros)
{
    for ( uint32_t joyNum = 0; joyNum < STDCONTROL_MAX_JOYSTICK_DEVICES; ++joyNum )
    {
        char aMessage[64];
        STDCONTROLTEST_FORMAT_MESSAGE(aMessage, "joy axis macros", joyNum);

        TEST_ASSERT_EQUAL_UINT32_MESSAGE(STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_X), STDCONTROL_GET_JOYSTICK_AXIS_X(joyNum), aMessage);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_Y), STDCONTROL_GET_JOYSTICK_AXIS_Y(joyNum), aMessage);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_Z), STDCONTROL_GET_JOYSTICK_AXIS_Z(joyNum), aMessage);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_RX), STDCONTROL_GET_JOYSTICK_AXIS_RX(joyNum), aMessage);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_RY), STDCONTROL_GET_JOYSTICK_AXIS_RY(joyNum), aMessage);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_RZ), STDCONTROL_GET_JOYSTICK_AXIS_RZ(joyNum), aMessage);
    }
}

TEST(stdControl, TestMouseAndOutOfRangeBoundaries)
{
    TEST_ASSERT_EQUAL_UINT32(STDCONTROL_KID_MOUSE_LBUTTON + 1u, STDCONTROL_KID_MOUSE_RBUTTON);
    TEST_ASSERT_EQUAL_UINT32(STDCONTROL_KID_MOUSE_LBUTTON + 2u, STDCONTROL_KID_MOUSE_MBUTTON);
    TEST_ASSERT_EQUAL_UINT32(STDCONTROL_KID_MOUSE_LBUTTON + 3u, STDCONTROL_KID_MOUSE_XBUTTON);
    TEST_ASSERT_EQUAL_UINT32(STDCONTROL_AID_MOUSE_X + 1u, STDCONTROL_AID_MOUSE_Y);
    TEST_ASSERT_EQUAL_UINT32(STDCONTROL_AID_MOUSE_X + 2u, STDCONTROL_AID_MOUSE_Z);

    TEST_ASSERT_FALSE(STDCONTROL_ISKEYBOARDBUTTON(STDCONTROL_MAX_KEYBOARD_BUTTONS));
    TEST_ASSERT_FALSE(STDCONTROL_ISJOYSTICKCID(STDCONTROL_JOYSTICK_FIRSTCID - 1u));
    TEST_ASSERT_FALSE(STDCONTROL_ISJOYSTICKCID(STDCONTROL_JOYSTICK_FIRSTCID + STDCONTROL_JOYSTICK_TOTALCIDS));
    TEST_ASSERT_FALSE(STDCONTROL_ISJOYSTICKBUTTON(STDCONTROL_JOYSTICK_FIRSTCID + STDCONTROL_JOYSTICK_TOTALCIDS));
    TEST_ASSERT_FALSE(STDCONTROL_ISJOYSTICKPOV(STDCONTROL_JOYSTICK_FIRSTCID + STDCONTROL_JOYSTICK_TOTALCIDS));
    TEST_ASSERT_FALSE(STDCONTROL_ISMOUSEBUTTON(STDCONTROL_KID_MOUSE_LBUTTON - 1u));
    TEST_ASSERT_FALSE(STDCONTROL_ISMOUSEBUTTON(STDCONTROL_KID_MOUSE_XBUTTON + 1u));
    TEST_ASSERT_FALSE(STDCONTROL_ISMOUSEAXIS(STDCONTROL_AID_MOUSE_X - 1u));
    TEST_ASSERT_FALSE(STDCONTROL_ISMOUSEAXIS(STDCONTROL_AID_MOUSE_Z + 1u));
    TEST_ASSERT_FALSE(STDCONTROL_ISJOYSTICKAXIS(STDCONTROL_AID_MOUSE_X));
    TEST_ASSERT_FALSE(STDCONTROL_ISJOYSTICKAXIS(STDCONTROL_MAX_AXES));
}

TEST(stdControl, TestDirectXVariantDependentLimits)
{
#if defined(J3D_DIRECTX9)
    TEST_ASSERT_EQUAL_UINT32(4u, STDCONTROL_MAX_GAMEPAD_DEVICES);
    TEST_ASSERT_EQUAL_UINT32(12u, STDCONTROL_MAX_JOYSTICK_DEVICES);
    TEST_ASSERT_EQUAL_UINT32(72u, STDCONTROL_MAX_JOYSTICK_AXES);
    TEST_ASSERT_EQUAL_UINT32(75u, STDCONTROL_MAX_AXES);
    TEST_ASSERT_EQUAL_UINT32(836u, STDCONTROL_MAX_KEYID);
    TEST_ASSERT_EQUAL_UINT32(832u, STDCONTROL_KID_MOUSE_LBUTTON);
    TEST_ASSERT_EQUAL_UINT32(835u, STDCONTROL_KID_MOUSE_XBUTTON);
#else
    TEST_ASSERT_EQUAL_UINT32(0u, STDCONTROL_MAX_GAMEPAD_DEVICES);
    TEST_ASSERT_EQUAL_UINT32(8u, STDCONTROL_MAX_JOYSTICK_DEVICES);
    TEST_ASSERT_EQUAL_UINT32(48u, STDCONTROL_MAX_JOYSTICK_AXES);
    TEST_ASSERT_EQUAL_UINT32(51u, STDCONTROL_MAX_AXES);
    TEST_ASSERT_EQUAL_UINT32(644u, STDCONTROL_MAX_KEYID);
    TEST_ASSERT_EQUAL_UINT32(640u, STDCONTROL_KID_MOUSE_LBUTTON);
    TEST_ASSERT_EQUAL_UINT32(643u, STDCONTROL_KID_MOUSE_XBUTTON);
#endif
}

TEST(stdControl, TestNoDeviceLifecycleAndReadDefaults)
{
    int pressed = 7;

    TEST_ASSERT_FALSE(stdControl_HasStarted());
    TEST_ASSERT_FALSE(stdControl_IsOpen());
    TEST_ASSERT_EQUAL_INT(1, stdControl_Open());
    TEST_ASSERT_FALSE(stdControl_IsOpen());
    TEST_ASSERT_EQUAL_INT(1, stdControl_SetActivation(1));
    TEST_ASSERT_EQUAL_INT(0, stdControl_ControlsActive());
    TEST_ASSERT_EQUAL_INT(0, stdControl_ControlsIdle());

    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdControl_ReadAxis(STDCONTROL_AID_MOUSE_X));
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadAxisRaw(STDCONTROL_AID_MOUSE_X));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdControl_ReadKeyAsAxis(0u));
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadKey(0u, &pressed));
    TEST_ASSERT_EQUAL_INT(0, pressed);

    pressed = 3;
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadAxisAsKey(STDCONTROL_AID_MOUSE_X, &pressed));
    TEST_ASSERT_EQUAL_INT(3, pressed);
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadAxisAsKeyEx(STDCONTROL_AID_MOUSE_X, &pressed, 0.25f));
    TEST_ASSERT_EQUAL_INT(3, pressed);

    stdControl_DisableReadJoysticks();
    stdControl_FinishRead();
}

TEST(stdControl, TestMouseAndSensitivityState)
{
    TEST_ASSERT_FALSE(stdControl_IsMouseEnabled());
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableMouse(1));
    TEST_ASSERT_TRUE(stdControl_IsMouseEnabled());
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableMouse(1));
    TEST_ASSERT_TRUE(stdControl_IsMouseEnabled());
    TEST_ASSERT_EQUAL_INT(0, stdControl_ToggleMouse());
    TEST_ASSERT_FALSE(stdControl_IsMouseEnabled());
    TEST_ASSERT_EQUAL_INT(1, stdControl_ToggleMouse());
    TEST_ASSERT_TRUE(stdControl_IsMouseEnabled());
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableMouse(0));
    TEST_ASSERT_FALSE(stdControl_IsMouseEnabled());

    TEST_ASSERT_EQUAL_INT(0, stdControl_MouseSensitivityEnabled());
    stdControl_EnableMouseSensitivity(1);
    TEST_ASSERT_EQUAL_INT(1, stdControl_MouseSensitivityEnabled());
    stdControl_EnableMouseSensitivity(0);
    TEST_ASSERT_EQUAL_INT(0, stdControl_MouseSensitivityEnabled());
}

TEST(stdControl, TestAxisRegistrationAndFlags)
{
    size_t axis = STDCONTROL_GET_JOYSTICK_AXIS_X(0u);

    TEST_ASSERT_EQUAL_INT(0, stdControl_EnableAxis(STDCONTROL_MAX_AXES));
    TEST_ASSERT_EQUAL_INT(0, stdControl_EnableAxis((int)axis));
    TEST_ASSERT_EQUAL_INT(0, stdControl_TestAxisFlag(axis, STDCONTROL_AXIS_REGISTERED));

    stdControl_RegisterAxis(axis, -100, 100, 0.25f);
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(axis, STDCONTROL_AXIS_REGISTERED));
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableAxis((int)axis));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(axis, STDCONTROL_AXIS_ENABLED));

    stdControl_SetAxisFlags(axis | STDCONTROL_AID_POSITIVE_AXIS, STDCONTROL_AXIS_ENABLED);
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(axis | STDCONTROL_AID_POSITIVE_AXIS, STDCONTROL_AXIS_ENABLED));

    stdControl_SetAxisFlags(axis | STDCONTROL_AID_NEGATIVE_AXIS, STDCONTROL_AXIS_ENABLED);
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(axis | STDCONTROL_AID_NEGATIVE_AXIS, STDCONTROL_AXIS_ENABLED));

    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdControl_ReadAxis(axis));
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadAxisRaw(axis));
}

#ifdef J3D_RUNTIME_GUARDS
TEST(stdControl, TestRuntimeGuardsRejectOutOfRangeInputs)
{
    int pressed = 7;

    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdControl_ReadKeyAsAxis(STDCONTROL_MAX_KEYID));
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadKey(STDCONTROL_MAX_KEYID, &pressed));
    TEST_ASSERT_EQUAL_INT(0, pressed);

    stdControl_RegisterAxis(STDCONTROL_MAX_AXES, -100, 100, 0.0f);
    stdControl_RegisterAxis(0u, 10, 10, 0.0f);
    TEST_ASSERT_EQUAL_INT(0, stdControl_TestAxisFlag(0u, STDCONTROL_AXIS_REGISTERED));
}
#endif

TEST(stdControl, TestMouseAxisRegistrationAndSensitivityHelpers)
{
    stdControl_RegisterMouseAxesXY(1.5f, 2.0f);
    TEST_ASSERT_EQUAL_INT(0, stdControl_EnableAxis(STDCONTROL_AID_MOUSE_X));
    TEST_ASSERT_EQUAL_INT(0, stdControl_EnableAxis(STDCONTROL_AID_MOUSE_Y));

    stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_X, -50, 50, 0.0f);
    stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Y, -40, 40, 0.0f);
    stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Z, -10, 10, 0.0f);

    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_X, STDCONTROL_AXIS_REGISTERED));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_Y, STDCONTROL_AXIS_REGISTERED));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_Z, STDCONTROL_AXIS_REGISTERED));

    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableAxis(STDCONTROL_AID_MOUSE_X));
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableAxis(STDCONTROL_AID_MOUSE_Y));
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableAxis(STDCONTROL_AID_MOUSE_Z));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_X, STDCONTROL_AXIS_ENABLED));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_Y, STDCONTROL_AXIS_ENABLED));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_Z, STDCONTROL_AXIS_ENABLED));

    stdControl_SetMouseSensitivity(2.0f, 3.0f);
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_X, STDCONTROL_AXIS_REGISTERED));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_Y, STDCONTROL_AXIS_REGISTERED));

    stdControl_Reset();
    TEST_ASSERT_EQUAL_INT(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_X, STDCONTROL_AXIS_ENABLED));
    TEST_ASSERT_EQUAL_INT(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_Y, STDCONTROL_AXIS_ENABLED));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_X, STDCONTROL_AXIS_REGISTERED));
    TEST_ASSERT_NOT_EQUAL(0, stdControl_TestAxisFlag(STDCONTROL_AID_MOUSE_Y, STDCONTROL_AXIS_REGISTERED));

    stdControl_RegisterMouseAxesXY(0.5f, 0.75f);
    stdControl_ResetMousePos();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdControl_ReadAxis(STDCONTROL_AID_MOUSE_X));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdControl_ReadAxis(STDCONTROL_AID_MOUSE_Y));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdControl_ReadAxis(STDCONTROL_AID_MOUSE_Z));
}

TEST(stdControl, TestReadTimingAndFlaggedAxes)
{
    size_t axis = STDCONTROL_GET_JOYSTICK_AXIS_X(0u);

    stdControl_RegisterAxis(axis, -100, 100, 0.0f);
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableAxis((int)axis));
    stdControl_TestSetControlsActive(true);

    stdControl_TestBeginRead(100u, 100u);
    TEST_ASSERT_EQUAL_UINT32(0u, stdControl_TestGetReadDeltaTime());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdControl_TestGetSecFPS());

    stdControl_TestSetAxisState(axis, 80);
    TEST_ASSERT_EQUAL_INT(80, stdControl_ReadAxisRaw(axis | STDCONTROL_AID_POSITIVE_AXIS));

    int pressed = 0;
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadAxisAsKey(axis | STDCONTROL_AID_LOW_SENSITIVITY | STDCONTROL_AID_POSITIVE_AXIS, &pressed));
    TEST_ASSERT_EQUAL_INT(1, pressed);

    stdControl_TestSetAxisState(axis, -80);
    pressed = 0;
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadAxisAsKey(axis | STDCONTROL_AID_LOW_SENSITIVITY | STDCONTROL_AID_NEGATIVE_AXIS, &pressed));
    TEST_ASSERT_EQUAL_INT(1, pressed);

    uint8_t aKeyboardState[STDCONTROL_MAX_KEYBOARD_BUTTONS] = { 0 };
    aKeyboardState[DIK_F15] = 0x80u;

    stdControl_TestBeginRead(116u, 100u);
    stdControl_TestApplyKeyboardState(aKeyboardState, STD_ARRAYLEN(aKeyboardState));
    TEST_ASSERT_EQUAL_UINT32(16u, stdControl_TestGetReadDeltaTime());
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.0625f, stdControl_TestGetSecFPS());
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 1.0f, stdControl_ReadKeyAsAxis(DIK_F15));

    stdControl_TestBeginRead(116u, 116u);
    stdControl_TestApplyKeyboardState(aKeyboardState, STD_ARRAYLEN(aKeyboardState));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdControl_ReadKeyAsAxis(DIK_F15));
}

TEST(stdControl, TestKeyboardStateTransitions)
{
    uint8_t aKeyboardState[STDCONTROL_MAX_KEYBOARD_BUTTONS] = { 0 };
    int pressed = 0;

    stdControl_TestSetControlsActive(true);
    stdControl_TestBeginRead(100u, 84u);

    aKeyboardState[DIK_F15] = 0x80u;
    stdControl_TestApplyKeyboardState(aKeyboardState, STD_ARRAYLEN(aKeyboardState));
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadKey(DIK_F15, &pressed));
    TEST_ASSERT_EQUAL_INT(1, pressed);

    stdControl_TestBeginRead(116u, 100u);
    stdControl_TestApplyKeyboardState(aKeyboardState, STD_ARRAYLEN(aKeyboardState));
    pressed = 0;
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadKey(DIK_F15, &pressed));
    TEST_ASSERT_EQUAL_INT(0, pressed);

    aKeyboardState[DIK_F15] = 0u;
    stdControl_TestApplyKeyboardState(aKeyboardState, STD_ARRAYLEN(aKeyboardState));
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadKey(DIK_F15, &pressed));

    aKeyboardState[0] = 0x80u;
    stdControl_TestApplyKeyboardState(aKeyboardState, 1u);
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadKey(0u, NULL));
    stdControl_TestApplyKeyboardState(NULL, STD_ARRAYLEN(aKeyboardState));
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadKey(0u, NULL));
}

TEST(stdControl, TestDirectInputJoystickStateTranslation)
{
    static const struct
    {
        DWORD pov;
        bool bLeft;
        bool bUp;
        bool bRight;
        bool bDown;
    } aPovCases[] =
    {
        { 0u * DI_DEGREES,   false, true,  false, false },
        { 45u * DI_DEGREES,  false, true,  true,  false },
        { 90u * DI_DEGREES,  false, false, true,  false },
        { 135u * DI_DEGREES, false, false, true,  true  },
        { 180u * DI_DEGREES, false, false, false, true  },
        { 225u * DI_DEGREES, true,  false, false, true  },
        { 270u * DI_DEGREES, true,  false, false, false },
        { 315u * DI_DEGREES, true,  true,  false, false },
        { 0xFFFFu,           false, false, false, false },
    };

    DIJOYSTATE state;
    STD_ZEROMEM(&state, sizeof(state));
    state.lX             = 600;
    state.lY             = -500;
    state.lZ             = 400;
    state.lRx            = -300;
    state.lRy            = 200;
    state.lRz            = -100;
    state.rgbButtons[0]  = 0x80u;
    state.rgbButtons[31] = 0x80u;

    stdControlTest_RegisterJoystickAxes(0u, -1000, 1000);
    stdControl_TestSetControlsActive(true);

    for ( size_t i = 0u; i < STD_ARRAYLEN(aPovCases); ++i )
    {
        stdControl_TestBeginRead(100u + (uint32_t)i * 16u, 84u + (uint32_t)i * 16u);
        state.rgdwPOV[0] = aPovCases[i].pov;
        stdControl_TestApplyJoystickState(0u, &state, 1u);
        stdControlTest_AssertPovState(0u, 0u, aPovCases[i].bLeft, aPovCases[i].bUp, aPovCases[i].bRight, aPovCases[i].bDown);
    }

    TEST_ASSERT_EQUAL_INT(600, stdControl_ReadAxisRaw(STDCONTROL_GET_JOYSTICK_AXIS_X(0u)));
    TEST_ASSERT_EQUAL_INT(-500, stdControl_ReadAxisRaw(STDCONTROL_GET_JOYSTICK_AXIS_Y(0u)));
    TEST_ASSERT_EQUAL_INT(400, stdControl_ReadAxisRaw(STDCONTROL_GET_JOYSTICK_AXIS_Z(0u)));
    TEST_ASSERT_EQUAL_INT(-300, stdControl_ReadAxisRaw(STDCONTROL_GET_JOYSTICK_AXIS_RX(0u)));
    TEST_ASSERT_EQUAL_INT(200, stdControl_ReadAxisRaw(STDCONTROL_GET_JOYSTICK_AXIS_RY(0u)));
    TEST_ASSERT_EQUAL_INT(-100, stdControl_ReadAxisRaw(STDCONTROL_GET_JOYSTICK_AXIS_RZ(0u)));
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETBUTTON(0u, 0u), NULL));
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETBUTTON(0u, 31u), NULL));

    state.rgbButtons[0]  = 0u;
    state.rgbButtons[31] = 0u;
    stdControl_TestApplyJoystickState(0u, &state, STDCONTROL_MAX_JOYSTICK_POVCONTROLERS + 1u);
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETBUTTON(0u, 0u), NULL));
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETBUTTON(0u, 31u), NULL));

    stdControl_TestApplyJoystickState(STDCONTROL_MAX_JOYSTICK_DEVICES, &state, 1u);
    stdControl_TestApplyJoystickState(0u, NULL, 1u);
}

TEST(stdControl, TestMouseStateTranslationAndRanges)
{
    DIMOUSESTATE state;
    StdControlAxis axis;

    stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_X, -100, 100, 0.0f);
    stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Y, -100, 100, 0.0f);
    stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Z, -100, 100, 0.0f);
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableAxis(STDCONTROL_AID_MOUSE_X));
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableAxis(STDCONTROL_AID_MOUSE_Y));
    TEST_ASSERT_EQUAL_INT(1, stdControl_EnableAxis(STDCONTROL_AID_MOUSE_Z));

    stdControl_TestSetControlsActive(true);
    stdControl_TestBeginRead(130u, 100u);

    STD_ZEROMEM(&state, sizeof(state));
    state.lX            = 40;
    state.lY            = -20;
    state.lZ            = 5;
    state.rgbButtons[0] = 0x80u;
    state.rgbButtons[3] = 0x80u;
    stdControl_TestApplyMouseState(&state, true);

    TEST_ASSERT_EQUAL_INT(40, stdControl_ReadAxisRaw(STDCONTROL_AID_MOUSE_X));
    TEST_ASSERT_EQUAL_INT(-20, stdControl_ReadAxisRaw(STDCONTROL_AID_MOUSE_Y));
    TEST_ASSERT_EQUAL_INT(5, stdControl_ReadAxisRaw(STDCONTROL_AID_MOUSE_Z));
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadKey(STDCONTROL_KID_MOUSE_LBUTTON, NULL));
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadKey(STDCONTROL_KID_MOUSE_XBUTTON, NULL));

    stdControl_RegisterMouseAxesXY(1.5f, 2.0f);
    TEST_ASSERT_TRUE(stdControl_TestGetAxis(STDCONTROL_AID_MOUSE_X, &axis));
    TEST_ASSERT_EQUAL_INT(-375, axis.min);
    TEST_ASSERT_EQUAL_INT(375, axis.max);
    TEST_ASSERT_TRUE(stdControl_TestGetAxis(STDCONTROL_AID_MOUSE_Y, &axis));
    TEST_ASSERT_EQUAL_INT(-400, axis.min);
    TEST_ASSERT_EQUAL_INT(400, axis.max);
    TEST_ASSERT_FALSE(stdControl_TestGetAxis(STDCONTROL_MAX_AXES, &axis));
    TEST_ASSERT_FALSE(stdControl_TestGetAxis(STDCONTROL_AID_MOUSE_X, NULL));

    stdControl_EnableMouseSensitivity(1);
    state.lX            = 500;
    state.lY            = -500;
    state.lZ            = 500;
    state.rgbButtons[0] = 0u;
    state.rgbButtons[3] = 0u;
    stdControl_TestApplyMouseState(&state, true);
    TEST_ASSERT_EQUAL_INT(375, stdControl_TestGetAxisState(STDCONTROL_AID_MOUSE_X));
    TEST_ASSERT_EQUAL_INT(-400, stdControl_TestGetAxisState(STDCONTROL_AID_MOUSE_Y));
    TEST_ASSERT_EQUAL_INT(100, stdControl_TestGetAxisState(STDCONTROL_AID_MOUSE_Z));
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadKey(STDCONTROL_KID_MOUSE_LBUTTON, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadKey(STDCONTROL_KID_MOUSE_XBUTTON, NULL));

    stdControl_EnableMouseSensitivity(0);
    stdControl_TestBeginRead(140u, 130u);
    state.lX = 60;
    state.lY = 20;
    state.lZ = 0;
    stdControl_TestApplyMouseState(&state, false);
    TEST_ASSERT_EQUAL_INT(50, stdControl_TestGetAxisState(STDCONTROL_AID_MOUSE_X));
    TEST_ASSERT_EQUAL_INT(0, stdControl_TestGetAxisState(STDCONTROL_AID_MOUSE_Y));

    stdControl_TestApplyMouseState(NULL, true);
    TEST_ASSERT_EQUAL_INT(0, stdControl_TestGetAxisState(STDCONTROL_MAX_AXES));
}

TEST(stdControl, TestStatusAndEmptyJoystickState)
{
    TEST_ASSERT_EQUAL_STRING("DI_OK", stdControl_DIGetStatus(DI_OK));
    TEST_ASSERT_EQUAL_STRING("Unknown Error", stdControl_DIGetStatus((int)0x12345678));
    TEST_ASSERT_EQUAL_size_t(0u, stdControl_GetMaxJoystickButtons());
    TEST_ASSERT_EQUAL_size_t(0u, stdControl_GetNumJoysticks());

#ifdef J3D_DIRECTX9
    TEST_ASSERT_EQUAL_STRING("", stdControl_GetJoysticDescription(-1));
    TEST_ASSERT_EQUAL_INT(0, stdControl_IsGamePad(-1));
#endif
}

#ifdef J3D_DIRECTX9
TEST(stdControl, TestXInputHelpersWithoutDevices)
{
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 0.0f, stdControl_ApplyXInputDeadzone(0, 0.25f));
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 0.0f, stdControl_ApplyXInputDeadzone(1000, 0.25f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 32767.0f, stdControl_ApplyXInputDeadzone(32767, 0.25f));
    TEST_ASSERT_FLOAT_WITHIN(2.0f, -32768.0f, stdControl_ApplyXInputDeadzone(-32768, 0.25f));

    stdControl_SetXInputVibration(-1, 1.0f, 1.0f);
    stdControl_SetXInputVibration(0, 1.0f, 1.0f);
}

TEST(stdControl, TestXInputStateDisconnectReconnectAndStableSlots)
{
    XINPUT_STATE state;
    XINPUT_CAPABILITIES caps;

    STD_ZEROMEM(&state, sizeof(state));
    STD_ZEROMEM(&caps, sizeof(caps));
    caps.Type    = XINPUT_DEVTYPE_GAMEPAD;
    caps.SubType = XINPUT_DEVSUBTYPE_GAMEPAD;
    state.dwPacketNumber = 7u;

    TEST_ASSERT_TRUE(stdControl_TestRegisterXInputDevice(2u, &state, &caps));
    TEST_ASSERT_FALSE(stdControl_TestRegisterXInputDevice(2u, &state, &caps));
    TEST_ASSERT_EQUAL_size_t(1u, stdControl_GetNumJoysticks());
    TEST_ASSERT_TRUE(stdControl_TestIsXInputDeviceConnected(0u));
    TEST_ASSERT_EQUAL_STRING("XInput3:Gamepad", stdControl_GetJoysticDescription(0));
    TEST_ASSERT_EQUAL_INT(1, stdControl_IsGamePad(0));

    for ( size_t axisIndex = 0u; axisIndex < STDCONTROL_JOYSTICK_NUMAXES; ++axisIndex )
    {
        TEST_ASSERT_EQUAL_INT(1, stdControl_EnableAxis((int)STDCONTROL_GET_JOYSTICK_AXIS(0u, axisIndex)));
    }
    stdControl_TestSetControlsActive(true);
    stdControl_TestBeginRead(100u, 84u);

    state.Gamepad.sThumbLX      = 32767;
    state.Gamepad.sThumbLY      = -32768;
    state.Gamepad.bLeftTrigger  = XINPUT_GAMEPAD_TRIGGER_THRESHOLD - 1u;
    state.Gamepad.bRightTrigger = 255u;
    state.Gamepad.wButtons      = XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP;
    TEST_ASSERT_TRUE(stdControl_TestApplyXInputState(0u, &state));

    TEST_ASSERT_TRUE(stdControl_ReadAxisRaw(STDCONTROL_GET_JOYSTICK_AXIS_X(0u)) > 32000);
    TEST_ASSERT_TRUE(stdControl_ReadAxisRaw(STDCONTROL_GET_JOYSTICK_AXIS_Y(0u)) < -32000);
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadAxisRaw(STDCONTROL_GET_JOYSTICK_AXIS_Z(0u)));
    TEST_ASSERT_EQUAL_INT(255, stdControl_ReadAxisRaw(STDCONTROL_GET_JOYSTICK_AXIS_RZ(0u)));
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETBUTTON(0u, 0u), NULL));
    stdControlTest_AssertPovState(0u, 0u, true, true, false, false);

    TEST_ASSERT_TRUE(stdControl_TestDisconnectXInputDevice(0u));
    TEST_ASSERT_FALSE(stdControl_TestIsXInputDeviceConnected(0u));
    TEST_ASSERT_EQUAL_STRING("", stdControl_GetJoysticDescription(0));
    for ( size_t axisIndex = 0u; axisIndex < STDCONTROL_JOYSTICK_NUMAXES; ++axisIndex )
    {
        TEST_ASSERT_EQUAL_INT(0, stdControl_TestGetAxisState(STDCONTROL_GET_JOYSTICK_AXIS(0u, axisIndex)));
    }
    TEST_ASSERT_EQUAL_INT(0, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETBUTTON(0u, 0u), NULL));
    stdControlTest_AssertPovState(0u, 0u, false, false, false, false);

    stdControl_TestBeginRead(116u, 100u);
    TEST_ASSERT_TRUE(stdControl_TestApplyXInputState(0u, &state));
    TEST_ASSERT_TRUE(stdControl_TestIsXInputDeviceConnected(0u));
    TEST_ASSERT_EQUAL_STRING("XInput3:Gamepad", stdControl_GetJoysticDescription(0));
    TEST_ASSERT_EQUAL_INT(1, stdControl_ReadKey(STDCONTROL_JOYSTICK_GETBUTTON(0u, 0u), NULL));

    TEST_ASSERT_TRUE(stdControl_TestRegisterXInputDevice(0u, &state, &caps));
    TEST_ASSERT_EQUAL_size_t(2u, stdControl_GetNumJoysticks());
    TEST_ASSERT_EQUAL_STRING("XInput3:Gamepad", stdControl_GetJoysticDescription(0));
    TEST_ASSERT_EQUAL_STRING("XInput1:Gamepad", stdControl_GetJoysticDescription(1));

    TEST_ASSERT_FALSE(stdControl_TestRegisterXInputDevice(XUSER_MAX_COUNT, &state, &caps));
    TEST_ASSERT_FALSE(stdControl_TestRegisterXInputDevice(1u, NULL, &caps));
    TEST_ASSERT_FALSE(stdControl_TestApplyXInputState(2u, &state));
    TEST_ASSERT_FALSE(stdControl_TestApplyXInputState(0u, NULL));
    TEST_ASSERT_FALSE(stdControl_TestDisconnectXInputDevice(2u));
    TEST_ASSERT_FALSE(stdControl_TestIsXInputDeviceConnected(2u));
}
#endif

TEST_GROUP_RUNNER(stdControl)
{
    RUN_TEST_CASE(stdControl, TestAllKeyIdsClassifyAsKeyboardJoystickOrMouse);
    RUN_TEST_CASE(stdControl, TestAllJoystickButtonIds);
    RUN_TEST_CASE(stdControl, TestAllJoystickPovIds);
    RUN_TEST_CASE(stdControl, TestAllAxisIdsAndFlaggedAxisIds);
    RUN_TEST_CASE(stdControl, TestNamedJoystickAxisMacros);
    RUN_TEST_CASE(stdControl, TestMouseAndOutOfRangeBoundaries);
    RUN_TEST_CASE(stdControl, TestDirectXVariantDependentLimits);
    RUN_TEST_CASE(stdControl, TestNoDeviceLifecycleAndReadDefaults);
    RUN_TEST_CASE(stdControl, TestMouseAndSensitivityState);
    RUN_TEST_CASE(stdControl, TestAxisRegistrationAndFlags);
#ifdef J3D_RUNTIME_GUARDS
    RUN_TEST_CASE(stdControl, TestRuntimeGuardsRejectOutOfRangeInputs);
#endif
    RUN_TEST_CASE(stdControl, TestMouseAxisRegistrationAndSensitivityHelpers);
    RUN_TEST_CASE(stdControl, TestReadTimingAndFlaggedAxes);
    RUN_TEST_CASE(stdControl, TestKeyboardStateTransitions);
    RUN_TEST_CASE(stdControl, TestDirectInputJoystickStateTranslation);
    RUN_TEST_CASE(stdControl, TestMouseStateTranslationAndRanges);
    RUN_TEST_CASE(stdControl, TestStatusAndEmptyJoystickState);
#ifdef J3D_DIRECTX9
    RUN_TEST_CASE(stdControl, TestXInputHelpersWithoutDevices);
    RUN_TEST_CASE(stdControl, TestXInputStateDisconnectReconnectAndStableSlots);
#endif
}
