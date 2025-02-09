#include <dinput.h>

#include "stdControl.h"
#include "stdWin95.h"

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/RTI/symbols.h>

#define STDCONTROL_MOUSE_BUFFERSIZE 32u

static bool stdControl_bStartup = false;
static bool stdControl_bOpen    = false;

static LPDIRECTINPUTA stdControl_pDI = NULL;

static bool stdControl_bReadMouse               = true;
static POINT stdControl_mousePos                = { 0, 0 };
static StdInputDevice stdControl_mouse          = { 0 };
static bool stdControl_bMouseSensitivityEnabled = false;
static int stdControl_cursorDisplayCounter      = 0;

static StdInputDevice stdControl_keyboard = { 0 };
static uint8_t stdControl_aKeyboardState[STDCONTROL_MAX_KEYBOARD_BUTTONS] = { 0 };

static bool stdControl_bReadJoysticks        = false;
static size_t  stdControl_numJoystickDevices = 0;
static StdControlJoystickDevice stdControl_aJoystickDevices[STDCONTROL_MAX_JOYSTICK_DEVICES] = { 0 };

static StdControlAxis stdControl_aAxes[STDCONTROL_MAX_AXES] = { 0 };
static int stdControl_aAxisStates[STDCONTROL_MAX_AXES]      = { 0 };

static uint32_t stdControl_aKeyIdleTimes[STDCONTROL_MAX_KEYID] = { 0 };
static int stdControl_aKeyInfos[STDCONTROL_MAX_KEYID]          = { 0 };
static int stdControl_aKeyPressCounter[STDCONTROL_MAX_KEYID]   = { 0 };

static int stdControl_bControlsIdle;
static int stdControl_bControlsActive;

static float sithControl_secFPS  = 0.0;
static float sithControl_msecFPS = 0.0;

static uint32_t stdControl_curReadTime   = 0;
static uint32_t stdControl_lastReadTime  = 0;
static uint32_t stdControl_readDeltaTime = 0;

static float stdControl_mouseXRange = 0.0;
static float stdControl_mouseYRange = 0.0;

static char stdControl_aStrBuf[128] = { 0 };

static const DXStatus stdControl_aDIStatusTbl[34] = {
    { DI_OK,                        "DI_OK" },
    { DI_NOTATTACHED,               "DI_NOTATTACHED" },
    { DI_BUFFEROVERFLOW,            "DI_BUFFEROVERFLOW" },
    { DI_PROPNOEFFECT,              "DI_PROPNOEFFECT" },
    { DI_POLLEDDEVICE,              "DI_POLLEDDEVICE" },
    { DIERR_OLDDIRECTINPUTVERSION,  "DIERR_OLDDIRECTINPUTVERSION" },
    { DIERR_BETADIRECTINPUTVERSION, "DIERR_BETADIRECTINPUTVERSION" },
    { DIERR_BADDRIVERVER,           "DIERR_BADDRIVERVER" },
    { DIERR_DEVICENOTREG,           "DIERR_DEVICENOTREG" },
    { DIERR_NOTFOUND,               "DIERR_NOTFOUND" },
    { DIERR_OBJECTNOTFOUND,         "DIERR_OBJECTNOTFOUND" },
    { DIERR_INVALIDPARAM,           "DIERR_INVALIDPARAM" },
    { DIERR_NOINTERFACE,            "DIERR_NOINTERFACE" },
    { DIERR_GENERIC,                "DIERR_GENERIC" },
    { DIERR_OUTOFMEMORY,            "DIERR_OUTOFMEMORY" },
    { DIERR_UNSUPPORTED,            "DIERR_UNSUPPORTED" },
    { DIERR_NOTINITIALIZED,         "DIERR_NOTINITIALIZED" },
    { DIERR_ALREADYINITIALIZED,     "DIERR_ALREADYINITIALIZED" },
    { DIERR_NOAGGREGATION,          "DIERR_NOAGGREGATION" },
    { DIERR_OTHERAPPHASPRIO,        "DIERR_OTHERAPPHASPRIO" },
    { DIERR_INPUTLOST,              "DIERR_INPUTLOST" },
    { DIERR_ACQUIRED,               "DIERR_ACQUIRED" },
    { DIERR_NOTACQUIRED,            "DIERR_NOTACQUIRED" },
    { DIERR_READONLY,               "DIERR_READONLY" },
    { DIERR_HANDLEEXISTS,           "DIERR_HANDLEEXISTS" },
    { DIERR_INSUFFICIENTPRIVS,      "DIERR_INSUFFICIENTPRIVS" },
    { DIERR_DEVICEFULL,             "DIERR_DEVICEFULL" },
    { DIERR_MOREDATA,               "DIERR_MOREDATA" },
    { DIERR_NOTDOWNLOADED,          "DIERR_NOTDOWNLOADED" },
    { DIERR_HASEFFECTS,             "DIERR_HASEFFECTS" },
    { DIERR_NOTEXCLUSIVEACQUIRED,   "DIERR_NOTEXCLUSIVEACQUIRED" },
    { DIERR_INCOMPLETEEFFECT,       "DIERR_INCOMPLETEEFFECT" },
    { DIERR_NOTBUFFERED,            "DIERR_NOTBUFFERED" },
    { DIERR_EFFECTPLAYING,          "DIERR_EFFECTPLAYING" }
};

void stdControl_InstallHooks(void)
{
    J3D_HOOKFUNC(stdControl_Startup);
    J3D_HOOKFUNC(stdControl_Shutdown);
    J3D_HOOKFUNC(stdControl_Open);
    J3D_HOOKFUNC(stdControl_Close);
    J3D_HOOKFUNC(stdControl_Reset);
    J3D_HOOKFUNC(stdControl_DisableReadJoysticks);
    J3D_HOOKFUNC(stdControl_EnableAxis);
    J3D_HOOKFUNC(stdControl_ReadControls);
    J3D_HOOKFUNC(stdControl_ReadAxis);
    J3D_HOOKFUNC(stdControl_ReadAxisRaw);
    J3D_HOOKFUNC(stdControl_ReadKeyAsAxis);
    J3D_HOOKFUNC(stdControl_ReadAxisAsKey);
    J3D_HOOKFUNC(stdControl_ReadAxisAsKeyEx);
    J3D_HOOKFUNC(stdControl_ReadKey);
    J3D_HOOKFUNC(stdControl_ControlsActive);
    J3D_HOOKFUNC(stdControl_SetActivation);
    J3D_HOOKFUNC(stdControl_ToggleMouse);
    J3D_HOOKFUNC(stdControl_EnableMouse);
    J3D_HOOKFUNC(stdControl_ControlsIdle);
    J3D_HOOKFUNC(stdControl_TestAxisFlag);
    J3D_HOOKFUNC(stdControl_SetAxisFlags);
    J3D_HOOKFUNC(stdControl_InitJoysticks);
    J3D_HOOKFUNC(stdControl_InitKeyboard);
    J3D_HOOKFUNC(stdControl_InitMouse);
    J3D_HOOKFUNC(stdControl_EnableAxisRead);
    J3D_HOOKFUNC(stdControl_ReadKeyboard);
    J3D_HOOKFUNC(stdControl_ReadJoysticks);
    J3D_HOOKFUNC(stdControl_ReadMouse);
    J3D_HOOKFUNC(stdControl_RegisterAxis);
    J3D_HOOKFUNC(stdControl_DIGetStatus);
    J3D_HOOKFUNC(stdControl_EnumDevicesCallback);
    J3D_HOOKFUNC(stdControl_ResetMousePos);
    J3D_HOOKFUNC(stdControl_GetMaxJoystickButtons);
    J3D_HOOKFUNC(stdControl_GetNumJoysticks);
    J3D_HOOKFUNC(stdControl_EnableMouseSensitivity);
    J3D_HOOKFUNC(stdControl_ShowMouseCursor);
    J3D_HOOKFUNC(stdControl_IsGamePad);
}

void stdControl_ResetGlobals(void)
{}

int J3DAPI stdControl_Startup(int bKeyboardForeground)
{
    if ( stdControl_bStartup )
    {
        return 1;
    }

    memset(stdControl_aKeyIdleTimes, 0, sizeof(stdControl_aKeyIdleTimes));
    memset(stdControl_aKeyInfos, 0, sizeof(stdControl_aKeyInfos));
    memset(stdControl_aAxes, 0, sizeof(stdControl_aAxes));
    memset(stdControl_aAxisStates, 0, sizeof(stdControl_aAxisStates));

    stdControl_mousePos.x = 0;
    stdControl_mousePos.y = 0;
    stdControl_cursorDisplayCounter = 0;

    memset(&stdControl_mouse, 0, sizeof(stdControl_mouse));
    memset(&stdControl_keyboard, 0, sizeof(stdControl_keyboard));

    stdControl_numJoystickDevices = 0;
    memset(stdControl_aJoystickDevices, 0, sizeof(stdControl_aJoystickDevices));

    HINSTANCE hInstance = stdWin95_GetInstance();
    HRESULT hres = DirectInputCreate(hInstance, DIRECTINPUT_VERSION, &stdControl_pDI, 0);
    if ( hres != DI_OK )
    {
        STDLOG_ERROR("DirectInputCreate returned %s.\n", stdControl_DIGetStatus(hres));
        return 1;
    }
    else if ( IDirectInput_EnumDevices(stdControl_pDI, 0U, stdControl_EnumDevicesCallback, NULL, DIEDFL_ATTACHEDONLY) != DI_OK )
    {
        STDLOG_ERROR("Could not create DInput Joystick device.\n");
        return 1;
    }

    stdControl_InitKeyboard(bKeyboardForeground);
    stdControl_InitJoysticks();
    stdControl_InitMouse();
    stdControl_Reset();
    stdControl_bStartup = true;
    return 0;
}

bool stdControl_HasStarted(void)
{
    return stdControl_bStartup;
}

void stdControl_Shutdown(void)
{
    if ( stdControl_bStartup )
    {
        stdControl_bStartup = false;
        if ( stdControl_mouse.pDIDevice )
        {
            IDirectInputDevice_Unacquire(stdControl_mouse.pDIDevice);
            IDirectInputDevice_Release(stdControl_mouse.pDIDevice);
        }

        memset(&stdControl_mouse, 0, sizeof(stdControl_mouse));

        if ( stdControl_keyboard.pDIDevice )
        {
            IDirectInputDevice_Unacquire(stdControl_keyboard.pDIDevice);
            IDirectInputDevice_Release(stdControl_keyboard.pDIDevice);
        }

        memset(&stdControl_keyboard, 0, sizeof(stdControl_keyboard));

        for ( size_t i = 0; i < stdControl_numJoystickDevices; i++ )
        {
            if ( stdControl_aJoystickDevices[i].pDIDevice )
            {
                IDirectInputDevice2_Unacquire(stdControl_aJoystickDevices[i].pDIDevice);
                IDirectInputDevice2_Release(stdControl_aJoystickDevices[i].pDIDevice);
            }
        }

        stdControl_numJoystickDevices = 0;
        memset(stdControl_aJoystickDevices, 0, sizeof(stdControl_aJoystickDevices));
        if ( stdControl_pDI )
        {
            IDirectInput_Release(stdControl_pDI);
            stdControl_pDI = 0;
        }
    }
}

int stdControl_Open(void)
{
    if ( !stdControl_bStartup )
    {
        return 1;
    }

    stdControl_bOpen = true;
    stdControl_SetActivation(1);
    return 0;
}

bool stdControl_IsOpen(void)
{
    return stdControl_bStartup;
}

void stdControl_Close(void)
{
    if ( stdControl_bOpen )
    {
        stdControl_SetActivation(0);
        stdControl_bOpen = false;
    }
}

void stdControl_Reset(void)
{
    stdControl_bReadMouse     = false;
    stdControl_bReadJoysticks = false;
    stdControl_bMouseSensitivityEnabled = false;
    for ( size_t i = 0; i < STDCONTROL_MAX_AXES; i++ )
    {
        stdControl_aAxes[i].flags &= ~STDCONTROL_AXIS_ENABLED;
    }
}

void stdControl_DisableReadJoysticks(void)
{
    stdControl_bReadJoysticks = false;
}

int J3DAPI stdControl_EnableAxis(int axisID)
{
    size_t axis = STDCONTROL_GETAID(axisID);
    if ( axis >= STDCONTROL_MAX_AXES )
    {
        return 0;
    }

    if ( (stdControl_aAxes[axis].flags & STDCONTROL_AXIS_REGISTERED) == 0 )
    {
        return 0;
    }

    stdControl_aAxes[axis].flags |= STDCONTROL_AXIS_ENABLED;
    stdControl_EnableAxisRead(axis);
    return 1;
}

void stdControl_ReadControls(void)
{
    STD_ASSERTREL(stdControl_bStartup && stdControl_bOpen);
    if ( stdControl_bControlsActive )
    {
        stdControl_bControlsIdle = 1;
        memset(stdControl_aKeyIdleTimes, 0, sizeof(stdControl_aKeyIdleTimes));
        memset(stdControl_aKeyPressCounter, 0, sizeof(stdControl_aKeyPressCounter));

        stdControl_curReadTime   = stdPlatform_GetTimeMsec();
        stdControl_readDeltaTime = stdControl_curReadTime - stdControl_lastReadTime;

        sithControl_secFPS  = 1.0f / (float)(stdControl_curReadTime - stdControl_lastReadTime);
        sithControl_msecFPS = 1000.0f * sithControl_secFPS;

        if ( stdControl_bMouseSensitivityEnabled )
        {
            memset(stdControl_aAxisStates, 0, 7u);
        }
        else
        {
            memset(stdControl_aAxisStates, 0, sizeof(stdControl_aAxisStates));
        }

        stdControl_ReadKeyboard();

        if ( stdControl_bReadJoysticks )
        {
            stdControl_ReadJoysticks();
        }

        stdControl_ReadMouse();
        stdControl_lastReadTime = stdControl_curReadTime;
    }
}

float J3DAPI stdControl_ReadAxis(size_t axis)
{
    size_t aid = STDCONTROL_GETAID(axis);
    if ( aid >= STDCONTROL_MAX_AXES )
    {
        return 0.0f;
    }

    if ( !stdControl_bControlsActive )
    {
        return 0.0f;
    }

    if ( (stdControl_aAxes[aid].flags & STDCONTROL_AXIS_ENABLED) == 0 )
    {
        return 0.0f;
    }

    int curPos = stdControl_aAxisStates[aid] - stdControl_aAxes[aid].center;
    if ( !curPos )
    {
        return 0.0f;
    }

    if ( (stdControl_aAxes[aid].flags & STDCONTROL_AXIS_HASDEADZONE) == 0 && stdControl_aAxes[aid].deadzoneThreshold )
    {
        int pos = curPos >= 0 ? stdControl_aAxisStates[aid] - stdControl_aAxes[aid].center : stdControl_aAxes[aid].center - stdControl_aAxisStates[aid];
        if ( pos < stdControl_aAxes[aid].deadzoneThreshold )
        {
            return 0.0f;
        }
    }

    float pos = (float)curPos * stdControl_aAxes[aid].scale;
    pos = stdMath_ClipNearZero(pos);

    if ( !stdControl_bControlsIdle )
    {
        return pos;
    }

    if ( pos != 0.0f )
    {
        stdControl_bControlsIdle = 0;
    }

    return pos;
}

int J3DAPI stdControl_ReadAxisRaw(size_t axis)
{
    size_t aid = STDCONTROL_GETAID(axis);
    if ( aid >= STDCONTROL_MAX_AXES )
    {
        return 0;
    }

    if ( !stdControl_bControlsActive )
    {
        return 0;
    }

    if ( (stdControl_aAxes[aid].flags & STDCONTROL_AXIS_ENABLED) == 0 )
    {
        return 0;
    }

    int pos = stdControl_aAxisStates[aid] - stdControl_aAxes[aid].center;
    if ( !pos )
    {
        return 0;
    }

    if ( stdControl_bControlsIdle )
    {
        stdControl_bControlsIdle = 0;
    }

    return pos;
}

float J3DAPI stdControl_ReadKeyAsAxis(size_t keyId)
{
    STD_ASSERTREL((keyId < STDCONTROL_MAX_KEYID));
    if ( !stdControl_bControlsActive )
    {
        return 0.0f;
    }

    uint32_t time = stdControl_aKeyIdleTimes[keyId];
    if ( !time )
    {
        if ( !stdControl_aKeyInfos[keyId] )
        {
            return 0.0f;
        }

        time = stdControl_readDeltaTime;
    }

    if ( time >= stdControl_readDeltaTime )
    {
        time = stdControl_readDeltaTime;
    }

    float deltaTime = (float)time * sithControl_secFPS;
    if ( !stdControl_bControlsIdle )
    {
        return deltaTime;
    }

    if ( deltaTime != 0.0f )
    {
        stdControl_bControlsIdle = 0;
    }

    return deltaTime;
}

int J3DAPI stdControl_ReadAxisAsKey(size_t axis, int* pState)
{
    if ( (axis & STDCONTROL_AID_LOW_SENSITIVITY) == 0 || (stdControl_aAxes[axis].flags & STDCONTROL_AXIS_GAMEPAD) != 0 )
    {
        return stdControl_ReadAxisAsKeyEx(axis, pState, 0.25f);
    }
    else
    {
        return stdControl_ReadAxisAsKeyEx(axis, pState, 0.75f);
    }
}

int J3DAPI stdControl_ReadAxisAsKeyEx(size_t axis, int* pState, float lowValue)
{
    J3D_UNUSED(pState);

    float pos = stdControl_ReadAxis(axis);
    if ( (axis & (STDCONTROL_AID_LOW_SENSITIVITY | STDCONTROL_AID_POSITIVE_AXIS | STDCONTROL_AID_NEGATIVE_AXIS)) != 0 )
    {
        int axisFlag = axis & STDCONTROL_AID_NEGATIVE_AXIS;
        if ( axisFlag == STDCONTROL_AID_POSITIVE_AXIS && pos > (double)lowValue )
        {
            return 1;
        }

        if ( axisFlag == STDCONTROL_AID_NEGATIVE_AXIS && -lowValue > pos )
        {
            return 1;
        }
    }
    else
    {
        if ( fabsf(pos) > lowValue )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI stdControl_ReadKey(size_t keyNum, int* pNumPressed)
{
    STD_ASSERTREL(keyNum < STDCONTROL_MAX_KEYID);
    if ( stdControl_bControlsActive )
    {
        if ( pNumPressed )
        {
            *pNumPressed += stdControl_aKeyPressCounter[keyNum];
        }

        if ( stdControl_bControlsIdle && stdControl_aKeyInfos[keyNum] )
        {
            stdControl_bControlsIdle = 0;
        }

        return stdControl_aKeyInfos[keyNum];
    }
    else
    {
        if ( pNumPressed )
        {
            *pNumPressed = 0;
        }

        return 0;
    }
}

void stdControl_FinishRead(void)
{}

void J3DAPI stdControl_RegisterMouseAxesXY(float xrange, float yrange)
{
    stdControl_mouseXRange = xrange;
    stdControl_mouseYRange = yrange;
    if ( (stdControl_aAxes[STDCONTROL_AID_MOUSE_X].flags & STDCONTROL_AXIS_REGISTERED) != 0 )
    {
        int range = lround(stdControl_mouseXRange * 250.0f);
        stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_X, -range, range, 0.0f);
    }

    if ( (stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].flags & STDCONTROL_AXIS_REGISTERED) != 0 )
    {
        int range = lround(stdControl_mouseXRange * 200.0f);
        stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Y, -range, range, 0.0f);
    }
}

int stdControl_ControlsActive(void)
{
    return stdControl_bControlsActive;
}

void J3DAPI stdControl_UpdateKeyState(int keyId, int bPressed, unsigned int tickTime)
{
    if ( !bPressed || stdControl_aKeyInfos[keyId] )
    {
        if ( !bPressed && stdControl_aKeyInfos[keyId] )
        {
            stdControl_aKeyInfos[keyId] = 0;
            if ( !stdControl_aKeyIdleTimes[keyId] )
            {
                stdControl_aKeyIdleTimes[keyId] = stdControl_readDeltaTime;
            }

            stdControl_aKeyIdleTimes[keyId] -= stdControl_curReadTime - tickTime;
        }
    }
    else
    {
        stdControl_aKeyInfos[keyId] = 1;
        stdControl_aKeyIdleTimes[keyId] = stdControl_curReadTime - tickTime;
        ++stdControl_aKeyPressCounter[keyId];
    }
}

int J3DAPI stdControl_SetActivation(int bActive)
{
    HRESULT hr = 1;
    if ( !stdControl_bOpen )
    {
        return 1;
    }

    for ( size_t keyId = 0; keyId < STDCONTROL_MAX_KEYBOARD_BUTTONS; ++keyId )
    {
        stdControl_UpdateKeyState(keyId, 0, stdControl_curReadTime);
    }

    if ( stdControl_bReadMouse && stdControl_mouse.pDIDevice )
    {
        for ( size_t keyId = 0; keyId < STDCONTROL_MAX_MOUSE_BUTTONS; ++keyId )
        {
            stdControl_UpdateKeyState(STDCONTROL_KID_MOUSE_LBUTTON + keyId, 0, stdControl_curReadTime);
        }
    }

    if ( bActive )
    {
        if ( stdControl_bReadMouse && stdControl_mouse.pDIDevice )
        {
            hr = IDirectInputDevice_Acquire(stdControl_mouse.pDIDevice);
        }

        if ( stdControl_keyboard.pDIDevice )
        {
            IDirectInputDevice_Acquire(stdControl_keyboard.pDIDevice);
        }

        for ( size_t joyNum = 0; joyNum < stdControl_numJoystickDevices; ++joyNum )
        {
            if ( stdControl_aJoystickDevices[joyNum].pDIDevice )
            {
                IDirectInputDevice2_Acquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
            }
        }

        stdControl_bControlsActive = 1;
    }
    else
    {
        if ( stdControl_mouse.pDIDevice )
        {
            hr = IDirectInputDevice_Unacquire(stdControl_mouse.pDIDevice);
        }

        if ( stdControl_keyboard.pDIDevice )
        {
            IDirectInputDevice_Unacquire(stdControl_keyboard.pDIDevice);
        }

        for ( size_t joyNum = 0; joyNum < stdControl_numJoystickDevices; ++joyNum )
        {
            if ( stdControl_aJoystickDevices[joyNum].pDIDevice )
            {
                IDirectInputDevice2_Unacquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
            }
        }

        stdControl_bControlsActive = 0;
    }

    return hr;
}

int stdControl_ToggleMouse(void)
{
    if ( stdControl_bReadMouse )
    {
        stdControl_bReadMouse = false;
        if ( stdControl_mouse.pDIDevice && SUCCEEDED(IDirectInputDevice_Unacquire(stdControl_mouse.pDIDevice)) )
        {
            stdControl_ShowMouseCursor(1);
        }
    }
    else
    {
        stdControl_bReadMouse = true;
        if ( stdControl_mouse.pDIDevice && SUCCEEDED(IDirectInputDevice_Acquire(stdControl_mouse.pDIDevice)) )
        {
            stdControl_ShowMouseCursor(0);
        }
    }

    return stdControl_bReadMouse;
}


int J3DAPI stdControl_EnableMouse(int bEnable)
{
    if ( bEnable != stdControl_bReadMouse )
    {
        stdControl_ToggleMouse();
    }

    return 1;
}

bool stdControl_IsMouseEnabled(void)
{
    return stdControl_bReadMouse;
}

int stdControl_ControlsIdle(void)
{
    return stdControl_bControlsIdle;
}

int J3DAPI stdControl_TestAxisFlag(size_t axis, StdControlAxisFlag flags)
{
    uint32_t bPositiveAxis = axis & STDCONTROL_AID_POSITIVE_AXIS;
    uint32_t bNegativeAxis = axis & STDCONTROL_AID_NEGATIVE_AXIS;
    size_t aid = STDCONTROL_GETAID(axis);
    if ( aid >= STDCONTROL_MAX_AXES )
    {
        return 0;
    }

    if ( (flags & STDCONTROL_AXIS_ENABLED) == 0 )
    {
        return flags & stdControl_aAxes[aid].flags;
    }

    if ( bPositiveAxis )
    {
        flags = flags & ~(STDCONTROL_AXIS_POSITIVE | STDCONTROL_AXIS_ENABLED) | STDCONTROL_AXIS_POSITIVE;
    }

    if ( bNegativeAxis )
    {
        flags = flags & ~(STDCONTROL_AXIS_NEGATIVE | STDCONTROL_AXIS_ENABLED) | STDCONTROL_AXIS_NEGATIVE;
    }

    return flags & stdControl_aAxes[aid].flags;
}

void J3DAPI stdControl_SetAxisFlags(size_t axis, StdControlAxisFlag flags)
{
    uint32_t bPositiveAxis = axis & STDCONTROL_AID_POSITIVE_AXIS;
    uint32_t  bNegativeAxis = axis & STDCONTROL_AID_NEGATIVE_AXIS;
    size_t aid = STDCONTROL_GETAID(axis);
    if ( aid < STDCONTROL_MAX_AXES )
    {
        if ( (flags & STDCONTROL_AXIS_ENABLED) != 0 )
        {
            if ( bPositiveAxis )
            {
                flags |= STDCONTROL_AXIS_POSITIVE;
            }

            if ( bNegativeAxis )
            {
                flags |= STDCONTROL_AXIS_NEGATIVE;
            }
        }

        stdControl_aAxes[aid].flags |= flags;
    }
}

void J3DAPI stdControl_InitJoysticks()
{
    for ( size_t joyNum = 0; joyNum < stdControl_numJoystickDevices; ++joyNum )
    {
        LPDIRECTINPUTDEVICEA lpd = NULL;
        if FAILED(IDirectInput_CreateDevice(stdControl_pDI, &stdControl_aJoystickDevices[joyNum].dinstance.guidInstance, &lpd, 0))
        {
            STDLOG_ERROR("Could not create DInput Joystick device.\n");
        }

        int hres = IDirectInputDevice_QueryInterface(lpd, &IID_IDirectInputDevice2A, (LPVOID*)&stdControl_aJoystickDevices[joyNum].pDIDevice);
        IDirectInputDevice_Release(lpd);

        if FAILED(hres)
        {
            goto error;
        }

        stdControl_aJoystickDevices[joyNum].caps.dwSize = sizeof(DIDEVCAPS);
        hres = IDirectInputDevice2_GetCapabilities(stdControl_aJoystickDevices[joyNum].pDIDevice, &stdControl_aJoystickDevices[joyNum].caps);
        if FAILED(hres)
        {
            goto error;
        }

        hres = IDirectInputDevice2_SetDataFormat(stdControl_aJoystickDevices[joyNum].pDIDevice, &c_dfDIJoystick);
        if FAILED(hres)
        {
            goto error;
        }

        HWND hwnd = stdWin95_GetWindow();
        hres = IDirectInputDevice2_SetCooperativeLevel(stdControl_aJoystickDevices[joyNum].pDIDevice, hwnd, DISCL_BACKGROUND | DISCL_EXCLUSIVE);
        if FAILED(hres)
        {
            goto error;
        }

        // Note, below the constant 6 in the array indexing is the number of joystick axis which is 3 - positional and 3 - rotation axis
        DIPROPRANGE dirange       = { 0 }; // Added: Init to 0
        dirange.diph.dwSize       = sizeof(DIPROPRANGE);
        dirange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dirange.diph.dwObj        = DIJOFS_X;
        dirange.diph.dwHow        = DIPH_BYOFFSET;

        if SUCCEEDED(IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_X(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        dirange.diph.dwObj = DIJOFS_Y;
        if SUCCEEDED(IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_Y(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        dirange.diph.dwObj = DIJOFS_Z;
        if SUCCEEDED(IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_Z(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        dirange.diph.dwObj = DIJOFS_RX;
        if SUCCEEDED(IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_RX(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        dirange.diph.dwObj = DIJOFS_RY;
        if SUCCEEDED(IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_RY(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        dirange.diph.dwObj = DIJOFS_RZ;
        if SUCCEEDED(IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_RZ(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        if ( GET_DIDEVICE_SUBTYPE(stdControl_aJoystickDevices[joyNum].dinstance.dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD )
        {
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_X(joyNum)].flags |= STDCONTROL_AXIS_GAMEPAD;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_Y(joyNum)].flags |= STDCONTROL_AXIS_GAMEPAD;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_Z(joyNum)].flags |= STDCONTROL_AXIS_GAMEPAD;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RX(joyNum)].flags |= STDCONTROL_AXIS_GAMEPAD;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RY(joyNum)].flags |= STDCONTROL_AXIS_GAMEPAD;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RZ(joyNum)].flags |= STDCONTROL_AXIS_GAMEPAD;
        }

        if ( (stdControl_aJoystickDevices[joyNum].caps.dwFlags & DIDC_FORCEFEEDBACK) != 0 )
        {
            // TODO: missing logic
           /* v11 = 20;
            v12 = 16;
            v13 = 0;
            v14 = 0;
            v15 = 0;*/
        }

        hres = IDirectInputDevice2_Acquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
        if FAILED(hres)
        {
        error:
            STDLOG_STATUS("%s error Acquiring Joystick.\n", stdControl_DIGetStatus(hres));
            if ( stdControl_aJoystickDevices[joyNum].pDIDevice )
            {
                IDirectInputDevice2_Release(stdControl_aJoystickDevices[joyNum].pDIDevice);
            }

            stdControl_aJoystickDevices[joyNum].pDIDevice = 0;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_X(joyNum)].flags &= ~STDCONTROL_AXIS_REGISTERED;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_Y(joyNum)].flags &= ~STDCONTROL_AXIS_REGISTERED;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_Z(joyNum)].flags &= ~STDCONTROL_AXIS_REGISTERED;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RX(joyNum)].flags &= ~STDCONTROL_AXIS_REGISTERED;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RY(joyNum)].flags &= ~STDCONTROL_AXIS_REGISTERED;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RZ(joyNum)].flags &= ~STDCONTROL_AXIS_REGISTERED;
        }
    }
}

void J3DAPI stdControl_InitKeyboard(int bForeground)
{
    if ( stdControl_pDI )
    {
        HRESULT hres = IDirectInput_CreateDevice(stdControl_pDI, &GUID_SysKeyboard, &stdControl_keyboard.pDIDevice, 0);
        if FAILED(hres)
        {
            goto error;
        }

        stdControl_keyboard.diDevCaps.dwSize = 44;
        hres = IDirectInputDevice_GetCapabilities(stdControl_keyboard.pDIDevice, &stdControl_keyboard.diDevCaps);
        if FAILED(hres)
        {
            goto error;
        }

        hres = IDirectInputDevice_SetDataFormat(stdControl_keyboard.pDIDevice, &c_dfDIKeyboard);
        if FAILED(hres)
        {
            goto error;
        }

        DIPROPDWORD didpw;
        HWND hwnd = stdWin95_GetWindow();
        hres = bForeground
            ? IDirectInputDevice_SetCooperativeLevel(stdControl_keyboard.pDIDevice, hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)
            : IDirectInputDevice_SetCooperativeLevel(stdControl_keyboard.pDIDevice, hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
        if ( FAILED(hres)
            || (didpw.diph.dwSize = sizeof(DIPROPDWORD),
                didpw.diph.dwHeaderSize = sizeof(DIPROPHEADER),
                didpw.diph.dwObj = 0,
                didpw.diph.dwHow = DIPH_DEVICE,
                didpw.dwData     = STD_ARRAYLEN(stdControl_aKeyboardState), // input buffer size
                hres = IDirectInputDevice_SetProperty(stdControl_keyboard.pDIDevice, DIPROP_BUFFERSIZE, &didpw.diph),
                hres < 0) )
        {
        error:
            STDLOG_STATUS("%s error Acquiring Keyboard.\n", stdControl_DIGetStatus(hres));

            if ( stdControl_keyboard.pDIDevice )
            {
                IDirectInputDevice_Release(stdControl_keyboard.pDIDevice);
            }
            stdControl_keyboard.pDIDevice = NULL;
        }
    }
}

void J3DAPI stdControl_InitMouse()
{
    if ( stdControl_pDI )
    {
        HRESULT hres = IDirectInput_CreateDevice(stdControl_pDI, &GUID_SysMouse, &stdControl_mouse.pDIDevice, 0);
        if FAILED(hres)
        {
            goto error;
        }

        stdControl_mouse.diDevCaps.dwSize = 44;
        hres = IDirectInputDevice_GetCapabilities(stdControl_mouse.pDIDevice, &stdControl_mouse.diDevCaps);
        if FAILED(hres)
        {
            goto error;
        }

        hres = IDirectInputDevice_SetDataFormat(stdControl_mouse.pDIDevice, &c_dfDIMouse);
        if FAILED(hres)
        {
            goto error;
        }

        DIPROPDWORD didpw;
        HWND hwnd = stdWin95_GetWindow();
        hres = IDirectInputDevice_SetCooperativeLevel(stdControl_mouse.pDIDevice, hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
        if ( FAILED(hres)
            || (didpw.diph.dwSize = sizeof(DIPROPDWORD),
                didpw.diph.dwHeaderSize = sizeof(DIPROPHEADER),
                didpw.diph.dwObj = 0,
                didpw.diph.dwHow = DIPH_DEVICE,
                didpw.dwData     = STDCONTROL_MOUSE_BUFFERSIZE, // input buffer size
                hres = IDirectInputDevice_SetProperty(stdControl_mouse.pDIDevice, DIPROP_BUFFERSIZE, &didpw.diph),
                hres < 0) )
        {
        error:
            STDLOG_STATUS("%s error Acquiring Mouse.\n", stdControl_DIGetStatus(hres));
            if ( stdControl_mouse.pDIDevice )
            {
                IDirectInputDevice_Release(stdControl_mouse.pDIDevice);
            }

            stdControl_mouse.pDIDevice = 0;
        }
        else
        {
            stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_X, -250, 250, 0.0f);
            stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Y, -200, 200, 0.0f);
            stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Z, -20, 20, 0.0f);
        }
    }
}

void J3DAPI stdControl_EnableAxisRead(size_t axis)
{
    if ( axis >= STDCONTROL_AID_MOUSE_X && axis < STDCONTROL_MAX_AXES )
    {
        stdControl_bReadMouse = true;
    }

    else if ( axis < STDCONTROL_AID_MOUSE_X )
    {
        stdControl_bReadJoysticks = true;
    }
}

void stdControl_ReadKeyboard(void)
{
    HRESULT hr = IDirectInputDevice_GetDeviceState(stdControl_keyboard.pDIDevice, STD_ARRAYLEN(stdControl_aKeyboardState), stdControl_aKeyboardState);
    if ( hr != DIERR_NOTACQUIRED && hr != DIERR_INPUTLOST )
    {
        if ( hr == DI_OK )
        {
            for ( size_t keyId = 0; keyId < STDCONTROL_MAX_KEYBOARD_BUTTONS; ++keyId )
            {
                stdControl_UpdateKeyState(keyId, stdControl_aKeyboardState[keyId] & 0x80, stdControl_curReadTime);// data  & 0x80 -> get key press state i.e.: not zero - button went down
            }

            return;
        }
        STDLOG_ERROR("GetDeviceState from keyboard returned %s.\n", stdControl_DIGetStatus(hr));
    }

    hr = IDirectInputDevice_Acquire(stdControl_keyboard.pDIDevice);
    if ( hr != DI_OK && hr != DIERR_OTHERAPPHASPRIO )
    {
        STDLOG_ERROR("Acquire keyboard returned %s.\n", stdControl_DIGetStatus(hr));
    }
}

void stdControl_ReadJoysticks(void)
{
    for ( size_t joyNum = 0; joyNum < stdControl_numJoystickDevices; joyNum++ )
    {
        HRESULT hr = IDirectInputDevice2_Poll(stdControl_aJoystickDevices[joyNum].pDIDevice);
        if FAILED(hr)
        {
            STDLOG_STATUS("%s error Poll Joystick.\n", stdControl_DIGetStatus(hr));
            IDirectInputDevice2_Acquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
            return;
        }

        DIJOYSTATE jstate;
        hr = IDirectInputDevice2_GetDeviceState(stdControl_aJoystickDevices[joyNum].pDIDevice, sizeof(DIJOYSTATE), &jstate);
        if FAILED(hr)
        {
            STDLOG_STATUS("%s error GetDeviceState Joystick.\n", stdControl_DIGetStatus(hr));
            IDirectInputDevice2_Acquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
            return;
        }

        // Set joy axes state
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_X(joyNum)]  = jstate.lX;
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_Y(joyNum)]  = jstate.lY;
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_Z(joyNum)]  = jstate.lZ;
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RX(joyNum)] = jstate.lRx;
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RY(joyNum)] = jstate.lRy;
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RZ(joyNum)] = jstate.lRz;

        for ( size_t btnNum = 0; btnNum < STDCONTROL_NUM_JOYSTICK_BUTTONS; ++btnNum )
        {
            stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETBUTTON(joyNum, btnNum), jstate.rgbButtons[btnNum], stdControl_curReadTime);
        }

        for ( size_t j = 0; j < stdControl_aJoystickDevices[joyNum].caps.dwPOVs && j < 4; ++j )
        {
            DWORD pov = jstate.rgdwPOV[j];
            bool bCentred = (uint16_t)pov == 0xFFFF;// POVCentered = (LOWORD(dwPOV) == 0xFFFF);

            if ( pov < 225 * DI_DEGREES || pov > 315 * DI_DEGREES )
            {

                stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVBUTTON(joyNum, j, 0), 0, stdControl_curReadTime);
            }
            else
            {
                stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVBUTTON(joyNum, j, 0), 1, stdControl_curReadTime);
            }

            if ( pov < 315 * DI_DEGREES && pov > 45 * DI_DEGREES || bCentred )
            {
                stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVBUTTON(joyNum, j, 1), 0, stdControl_curReadTime);
            }
            else
            {
                stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVBUTTON(joyNum, j, 1), 1, stdControl_curReadTime);
            }

            if ( pov < 45 * DI_DEGREES || pov > 135 * DI_DEGREES )
            {
                stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVBUTTON(joyNum, j, 2), 0, stdControl_curReadTime);
            }
            else
            {
                stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVBUTTON(joyNum, j, 2), 1, stdControl_curReadTime);
            }

            if ( pov < 135 * DI_DEGREES || pov > 225 * DI_DEGREES )
            {
                stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVBUTTON(joyNum, j, 3), 0, stdControl_curReadTime);
            }
            else
            {
                stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVBUTTON(joyNum, j, 3), 1, stdControl_curReadTime);
            }
        }
    }
}

void stdControl_ReadMouse(void)
{
    DIDEVICEOBJECTDATA aMouseBuffer[STDCONTROL_MOUSE_BUFFERSIZE] = { 0 }; // Added: Init to 0
    if ( !stdControl_bReadMouse || !stdControl_mouse.pDIDevice )
    {
        return;
    }

    DIMOUSESTATE mouseState;
    HRESULT hr = IDirectInputDevice_GetDeviceState(stdControl_mouse.pDIDevice, sizeof(DIMOUSESTATE), &mouseState);
    if ( hr != DI_OK )
    {
        if ( hr != DIERR_NOTACQUIRED && hr != DIERR_INPUTLOST ) {
            STDLOG_ERROR("GetDeviceState(mouse) returned %s.\n", stdControl_DIGetStatus(hr));
        }

        hr = IDirectInputDevice_Acquire(stdControl_mouse.pDIDevice);
        if ( hr != DI_OK && hr != DIERR_OTHERAPPHASPRIO )
        {
            STDLOG_ERROR("Acquire mouse returned %s.\n", stdControl_DIGetStatus(hr));
        }
    }
    else
    {
        // Update axis state
        if ( stdControl_bMouseSensitivityEnabled )
        {
            stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X] += mouseState.lX;
            stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X]  = STDMATH_CLAMP(stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X], stdControl_aAxes[STDCONTROL_AID_MOUSE_X].min, stdControl_aAxes[STDCONTROL_AID_MOUSE_X].max);

            stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y] += mouseState.lY;
            stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y]  = STDMATH_CLAMP(stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y], stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].min, stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].max);

            stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z] += mouseState.lZ;
            stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z]  = STDMATH_CLAMP(stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z], stdControl_aAxes[STDCONTROL_AID_MOUSE_Z].min, stdControl_aAxes[STDCONTROL_AID_MOUSE_Z].max);
        }
        else
        {
            stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X] = mouseState.lX;
            stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y] = mouseState.lY;
            stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z] = mouseState.lZ;
            if ( stdControl_readDeltaTime < 25 )
            {
                stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X] = (stdControl_mousePos.x + mouseState.lX) / 2;
                stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y] = (stdControl_mousePos.y + mouseState.lY) / 2;
            }

            stdControl_mousePos.x = mouseState.lX;
            stdControl_mousePos.y = mouseState.lY;
        }
    }

    DWORD bufferSize = STDCONTROL_MOUSE_BUFFERSIZE;
    hr = IDirectInputDevice_GetDeviceData(stdControl_mouse.pDIDevice, sizeof(DIDEVICEOBJECTDATA), aMouseBuffer, &bufferSize, 0);
    if ( hr != DI_OK )
    {
        if ( hr == DI_BUFFEROVERFLOW )
        {
            for ( size_t i = 0; i < STDCONTROL_MAX_MOUSE_BUTTONS; i++ )
            {
                stdControl_UpdateKeyState(STDCONTROL_KID_MOUSE_LBUTTON + i, mouseState.rgbButtons[i], stdControl_curReadTime);
            }
        }
        else
        {
            hr = IDirectInputDevice_Acquire(stdControl_mouse.pDIDevice);
            if ( hr != DIERR_OTHERAPPHASPRIO )
            {
                STDLOG_ERROR("GetDeviceData from mouse returned %s.\n", stdControl_DIGetStatus(hr));
            }
        }
    }
    else
    {
        for ( size_t i = 0; i < bufferSize; i++ )
        {
            DIDEVICEOBJECTDATA* pData = &aMouseBuffer[i];
            if ( pData->dwOfs >= DIMOFS_BUTTON0 && pData->dwOfs <= DIMOFS_BUTTON3 )// IF 12 - DIMOFS_BUTTON0 ... 15 - DIMOFS_BUTTON3
            {
                static_assert((STDCONTROL_KID_MOUSE_LBUTTON - DIMOFS_BUTTON0) == 628, "(STDCONTROL_KID_MOUSE_LBUTTON - DIMOFS_BUTTON0) == 628");
                stdControl_UpdateKeyState(pData->dwOfs + (STDCONTROL_KID_MOUSE_LBUTTON - DIMOFS_BUTTON0), pData->dwData & 0x80, pData->dwTimeStamp);// pCurData->dwData & 0x80 -> extract mouse button press state i.e.: not zero - button went down
            }
        }
    }
}

void J3DAPI stdControl_RegisterAxis(size_t aid, int min, int max, float deadzoneScale)
{
    STD_ASSERTREL(max > min);
    STD_ASSERTREL(max - min > 0);
    STD_ASSERTREL(aid < STDCONTROL_MAX_AXES);

    int center = (max - min + 1) / 2 + min;

    stdControl_aAxes[aid].flags |= STDCONTROL_AXIS_REGISTERED;
    stdControl_aAxes[aid].min    = min;
    stdControl_aAxes[aid].max    = max;
    stdControl_aAxes[aid].center = center;
    stdControl_aAxes[aid].scale  = 1.0f / (float)(max - center);

    if ( deadzoneScale == 0.0f )
    {
        stdControl_aAxes[aid].deadzoneThreshold = 0;
    }
    else
    {
        stdControl_aAxes[aid].deadzoneThreshold = lround((float)(max - center) * deadzoneScale);
    }
}

const char* J3DAPI stdControl_DIGetStatus(int HRESULT)
{
    const char* pError = "Unknown Error";
    for ( size_t i = 0; i < STD_ARRAYLEN(stdControl_aDIStatusTbl); i++ )
    {
        if ( stdControl_aDIStatusTbl[i].code == HRESULT )
        {
            return stdControl_aDIStatusTbl[i].text;
        }
    }

    return pError;
}

BOOL CALLBACK stdControl_EnumDevicesCallback(LPCDIDEVICEINSTANCEA pdidInstance, LPVOID pContext)
{
    J3D_UNUSED(pContext);

    DWORD dwDevType = pdidInstance->dwDevType;
    if ( dwDevType == DIDEVTYPE_MOUSE )
    {
        STDLOG_STATUS("Mouse:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
    }

    else if ( dwDevType == DIDEVTYPE_KEYBOARD )
    {
        STDLOG_STATUS("Keyboard:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
    }

    else if ( dwDevType == DIDEVTYPE_JOYSTICK && (unsigned int)stdControl_numJoystickDevices < 8 )
    {
        memcpy(&stdControl_aJoystickDevices[stdControl_numJoystickDevices++].dinstance, pdidInstance, sizeof(DIDEVICEINSTANCEA));

        switch ( GET_DIDEVICE_SUBTYPE(pdidInstance->dwDevType) )
        {
            case DIDEVTYPEJOYSTICK_TRADITIONAL:
                STDLOG_STATUS("Joystick:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;

            case DIDEVTYPEJOYSTICK_FLIGHTSTICK:
                STDLOG_STATUS("Flightstick:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;

            case DIDEVTYPEJOYSTICK_GAMEPAD:
                STDLOG_STATUS("Gamepad:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;

            case DIDEVTYPEJOYSTICK_RUDDER:
                STDLOG_STATUS("Rudder:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;

            case DIDEVTYPEJOYSTICK_WHEEL:
                STDLOG_STATUS("Wheel:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;

            case DIDEVTYPEJOYSTICK_HEADTRACKER:
                STDLOG_STATUS("HeadTracker:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;

            default:
                STDLOG_STATUS("Unknown:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
        }
    }

    return 1;
}

void stdControl_ResetMousePos(void)
{
    // TODO: missing `mousePos` reset
    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X] = 0;
    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y] = 0;
    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z] = 0;
}

size_t stdControl_GetMaxJoystickButtons(void)
{
    size_t maxButtons = 0;
    for ( size_t i = 0; i < stdControl_numJoystickDevices; i++ )
    {
        if ( stdControl_aJoystickDevices[i].caps.dwButtons > maxButtons )
        {
            maxButtons = stdControl_aJoystickDevices[i].caps.dwButtons;
        }
    }

    return maxButtons;
}

size_t stdControl_GetNumJoysticks(void)
{
    return stdControl_numJoystickDevices;
}

const char* J3DAPI stdControl_GetJoysticDescription(int joyNum)
{
    // TODO: replace with stdUtil_<string_format> function
    snprintf(
        stdControl_aStrBuf,
        128,
        "%s:%s",
        stdControl_aJoystickDevices[joyNum].dinstance.tszProductName,
        stdControl_aJoystickDevices[joyNum].dinstance.tszInstanceName);
    return stdControl_aStrBuf;
}

int stdControl_MouseSensitivityEnabled(void)
{
    return stdControl_bMouseSensitivityEnabled;
}

void J3DAPI stdControl_EnableMouseSensitivity(int bEnable)
{
    stdControl_bMouseSensitivityEnabled = bEnable != 0;
}

void J3DAPI stdControl_ShowMouseCursor(int bShow)
{
    stdControl_cursorDisplayCounter = ShowCursor(bShow);
    if ( stdControl_cursorDisplayCounter % 2 )
    {
        stdControl_cursorDisplayCounter = ShowCursor(bShow == 0);
    }
}

int J3DAPI stdControl_IsGamePad(int joyNum)
{
    return GET_DIDEVICE_SUBTYPE(stdControl_aJoystickDevices[joyNum].dinstance.dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD;
}

void J3DAPI stdControl_SetMouseSensitivity(float xSensitivity, float ySensitivity)
{
    if ( (stdControl_aAxes[STDCONTROL_AID_MOUSE_X].flags & STDCONTROL_AXIS_REGISTERED) != 0 )
    {
        stdControl_aAxes[STDCONTROL_AID_MOUSE_X].deadzoneThreshold = 0;
        stdControl_aAxes[STDCONTROL_AID_MOUSE_X].max       = (int)(xSensitivity * 250.0);
        stdControl_aAxes[STDCONTROL_AID_MOUSE_X].min       = -stdControl_aAxes[STDCONTROL_AID_MOUSE_X].max;
        stdControl_aAxes[STDCONTROL_AID_MOUSE_X].flags    |= STDCONTROL_AXIS_REGISTERED;
        stdControl_aAxes[STDCONTROL_AID_MOUSE_X].center    = (2 * stdControl_aAxes[STDCONTROL_AID_MOUSE_X].max + 1) / 2 - stdControl_aAxes[STDCONTROL_AID_MOUSE_X].max;
        stdControl_aAxes[STDCONTROL_AID_MOUSE_X].scale     = 1.0f / (float)(stdControl_aAxes[STDCONTROL_AID_MOUSE_X].max - stdControl_aAxes[STDCONTROL_AID_MOUSE_X].center);
    }

    if ( (stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].flags & STDCONTROL_AXIS_REGISTERED) != 0 )
    {
        stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].deadzoneThreshold   = 0;
        stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].max       = (int)(ySensitivity * 200.0);
        stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].min       = -stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].max;
        stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].flags    |= STDCONTROL_AXIS_REGISTERED;
        stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].center    = (2 * stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].max + 1) / 2 - stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].max;
        stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].scale     = 1.0f / (float)(stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].max - stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].center);
    }
}