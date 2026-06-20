#include <std/Win95/stdControl.h>
#include <std/Win95/stdWin95.h>

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/RTI/symbols.h>

#include <Xinput.h>
#pragma comment(lib,"Xinput.lib")

#include <wbemidl.h>
#include <oleauto.h>
#pragma comment(lib, "wbemuuid.lib")

#define STDCONTROL_COMSAFE_RELEASE(p) { if (p) { (p)->lpVtbl->Release(p); (p) = NULL; } }

typedef struct sStdInputDevice
{
    LPDIRECTINPUTDEVICE8 pDIDevice;
    DIDEVCAPS diDevCaps;
} StdInputDevice;
static_assert(sizeof(StdInputDevice) == 48, "sizeof(StdInputDevice) == 48");

typedef struct sStdControlJoystickDevice
{
    DIDEVICEINSTANCE dinstance;
    LPDIRECTINPUTDEVICE8 pDIDevice;
    DIDEVCAPS caps;
} StdControlJoystickDevice;
static_assert(sizeof(StdControlJoystickDevice) == 628, "sizeof(StdControlJoystickDevice) == 628");

typedef struct sStdControlXInputDevice
{
    DWORD userIndex;
    bool bConnected;
    bool bIsGamepad;
    XINPUT_STATE state;
    XINPUT_CAPABILITIES caps;
    XINPUT_VIBRATION vibration;

    float leftStickDeadZone;
    float rightStickDeadZone;
    float triggerThreshold;
} StdControlXInputDevice;

#define STDCONTROL_MOUSE_BUFFERSIZE 32u

// XInput constants
#define STDCONTROL_XINPUT_GAMEPAD_THUMB_MINVALUE  -32768
#define STDCONTROL_XINPUT_GAMEPAD_THUMB_MAXVALUE  32767

#define STDCONTROL_XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
#define STDCONTROL_XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
#define STDCONTROL_XINPUT_GAMEPAD_TRIGGER_THRESHOLD    XINPUT_GAMEPAD_TRIGGER_THRESHOLD
#define STDCONTROL_XINPUT_RESCAN_INTERVAL_MSEC          2000u

static bool stdControl_bStartup = false;
static bool stdControl_bOpen    = false;

static LPDIRECTINPUT8 stdControl_pDI = NULL;

static bool stdControl_bReadMouse               = true;
static POINT stdControl_mousePos                = { 0, 0 };
static StdInputDevice stdControl_mouse          = { 0 };
static bool stdControl_bMouseSensitivityEnabled = false;
static int stdControl_cursorDisplayCounter      = 0;

static StdInputDevice stdControl_keyboard = { 0 };
static uint8_t stdControl_aKeyboardState[STDCONTROL_MAX_KEYBOARD_BUTTONS] = { 0 };

static bool stdControl_bReadJoysticks        = false;
static size_t  stdControl_numJoystickDevices = 0;
static StdControlJoystickDevice stdControl_aJoystickDevices[STDCONTROL_MAX_JOYSTICK_DEVICES - STDCONTROL_MAX_GAMEPAD_DEVICES] = { 0 }; // TODO: After gamepad implementation has dedicated slots reduce to STDCONTROL_MAX_JOYSTICK_DEVICES

static StdControlAxis stdControl_aAxes[STDCONTROL_MAX_AXES] = { 0 };
static int stdControl_aAxisStates[STDCONTROL_MAX_AXES]      = { 0 };

static uint32_t stdControl_aKeyIdleTimes[STDCONTROL_MAX_KEYID] = { 0 };
static bool stdControl_aKeyInfo[STDCONTROL_MAX_KEYID]          = { 0 }; // Stores key pressed down state for each key
static int stdControl_aKeyPressed[STDCONTROL_MAX_KEYID]        = { 0 }; // Stores key press state for each key

static bool stdControl_bControlsIdle;
static bool stdControl_bControlsActive;

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

// XInput static variables
//static bool stdControl_bUseXInput = true;
static bool stdControl_bReadXInput = false;
static size_t stdControl_numXInputDevices = 0;
static StdControlXInputDevice stdControl_aXInputDevices[XUSER_MAX_COUNT] = { 0 };
static DWORD stdControl_lastXInputCheck = 0;

static bool stdControl_IsXInputJoystickIndex(int joyNum, size_t numDirectInputDevices)
{
    return joyNum >= 0 && (size_t)joyNum >= numDirectInputDevices;
}

void stdControl_InitJoysticks(void);
void J3DAPI stdControl_InitKeyboard(int bForeground);
void stdControl_InitMouse(void);

void J3DAPI stdControl_EnableAxisRead(size_t axis);

void stdControl_ReadKeyboard(void);
void stdControl_ReadJoysticks(void);
void stdControl_ReadMouse(void);

static void stdControl_UpdateReadTiming(uint32_t currentReadTime);
static void stdControl_ApplyKeyboardState(const uint8_t* aKeyState, size_t numKeys);
static void stdControl_ApplyJoystickState(size_t joyNum, const DIJOYSTATE* pState, size_t numPovs);
static void stdControl_ApplyMouseAxisState(const DIMOUSESTATE* pState);
static void stdControl_ApplyMouseButtonState(const DIMOUSESTATE* pState);

const char* J3DAPI stdControl_DIGetStatus(int HRESULT);
BOOL CALLBACK stdControl_EnumDevicesCallback(LPCDIDEVICEINSTANCE pdidInstance, LPVOID pContext);

// XInput functions
void stdControl_InitXInput(void);
void stdControl_ReadXInput(void);
void stdControl_ShutdownXInput(void);
float J3DAPI stdControl_ApplyXInputDeadzone(SHORT value, float deadzone);
void J3DAPI stdControl_SetXInputVibration(int controllerIndex, float leftMotor, float rightMotor);
static int stdControl_FindXInputDevice(DWORD userIndex);
static bool stdControl_AddXInputDevice(DWORD userIndex, const XINPUT_STATE* pState, const XINPUT_CAPABILITIES* pCaps);
static void stdControl_ApplyXInputState(size_t deviceIndex, const XINPUT_STATE* pState);
static void stdControl_ClearXInputState(size_t deviceIndex);
static void stdControl_ScanXInputDevices(void);

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
    STDLOG_STATUS("Starting control system with DirectInput8 and XInput as backend...\n");
    if ( stdControl_bStartup )
    {
        return 1;
    }

    memset(stdControl_aKeyIdleTimes, 0, sizeof(stdControl_aKeyIdleTimes));
    memset(stdControl_aKeyInfo, 0, sizeof(stdControl_aKeyInfo));
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
    HRESULT hres = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8, &stdControl_pDI, NULL);
    if ( hres != DI_OK )
    {
        STDLOG_ERROR("DirectInput8Create returned %s.\n", stdControl_DIGetStatus(hres));
        return 1;
    }

    if ( IDirectInput8_EnumDevices(stdControl_pDI, DI8DEVCLASS_GAMECTRL, stdControl_EnumDevicesCallback, NULL, DIEDFL_ATTACHEDONLY) != DI_OK )
    {
        STDLOG_ERROR("Could not enumerate DInput devices.\n");
        IDirectInput8_Release(stdControl_pDI);
        stdControl_pDI = NULL;
        return 1;
    }

    stdControl_InitKeyboard(bKeyboardForeground);
    stdControl_InitJoysticks();
    stdControl_InitXInput();
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

        // Cleanup mouse
        if ( stdControl_mouse.pDIDevice )
        {
            IDirectInputDevice8_Unacquire(stdControl_mouse.pDIDevice);
            IDirectInputDevice8_Release(stdControl_mouse.pDIDevice);
        }

        memset(&stdControl_mouse, 0, sizeof(stdControl_mouse));

        // Cleanup keyboard
        if ( stdControl_keyboard.pDIDevice )
        {
            IDirectInputDevice8_Unacquire(stdControl_keyboard.pDIDevice);
            IDirectInputDevice8_Release(stdControl_keyboard.pDIDevice);
        }

        memset(&stdControl_keyboard, 0, sizeof(stdControl_keyboard));

        // Cleanup joysticks
        for ( size_t i = 0; i < stdControl_numJoystickDevices; i++ )
        {
            if ( stdControl_aJoystickDevices[i].pDIDevice )
            {
                IDirectInputDevice8_Unacquire(stdControl_aJoystickDevices[i].pDIDevice);
                IDirectInputDevice8_Release(stdControl_aJoystickDevices[i].pDIDevice);
            }
        }

        stdControl_numJoystickDevices = 0;
        memset(stdControl_aJoystickDevices, 0, sizeof(stdControl_aJoystickDevices));

        if ( stdControl_pDI )
        {
            IDirectInput8_Release(stdControl_pDI);
            stdControl_pDI = 0;
        }

        // Cleanup XInput
        stdControl_ShutdownXInput();
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
    return stdControl_bOpen;
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
    stdControl_bReadMouse               = false;
    stdControl_bReadJoysticks           = false;
    stdControl_bReadXInput              = false;
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

static void stdControl_UpdateReadTiming(uint32_t currentReadTime)
{
    stdControl_curReadTime   = currentReadTime;
    stdControl_readDeltaTime = stdControl_curReadTime - stdControl_lastReadTime;

    if ( stdControl_readDeltaTime )
    {
        sithControl_secFPS  = 1.0f / (float)stdControl_readDeltaTime;
        sithControl_msecFPS = 1000.0f * sithControl_secFPS;
    }
    else
    {
        sithControl_secFPS  = 0.0f;
        sithControl_msecFPS = 0.0f;
    }
}

void stdControl_ReadControls(void)
{
    STD_ASSERTREL(stdControl_bStartup && stdControl_bOpen);

    if ( stdControl_bControlsActive )
    {
        stdControl_bControlsIdle = true;
        memset(stdControl_aKeyIdleTimes, 0, sizeof(stdControl_aKeyIdleTimes));
        memset(stdControl_aKeyPressed, 0, sizeof(stdControl_aKeyPressed));

        stdControl_UpdateReadTiming(stdPlatform_GetTimeMsec());

        if ( stdControl_bMouseSensitivityEnabled )
        {
            memset(stdControl_aAxisStates, 0, 7u); // TTODO: Why only 7 bytes?
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

        if ( stdControl_bReadXInput
            || (DWORD)(stdControl_curReadTime - stdControl_lastXInputCheck) >= STDCONTROL_XINPUT_RESCAN_INTERVAL_MSEC )
        {
            stdControl_ReadXInput();
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
        stdControl_bControlsIdle = false;
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
        stdControl_bControlsIdle = false;
    }

    return pos;
}

float J3DAPI stdControl_ReadKeyAsAxis(size_t keyId)
{
    STD_ASSERTREL((keyId < STDCONTROL_MAX_KEYID));
    STD_GUARD(keyId < STDCONTROL_MAX_KEYID, 0.0f);

    if ( !stdControl_bControlsActive )
    {
        return 0.0f;
    }

    uint32_t time = stdControl_aKeyIdleTimes[keyId];
    if ( !time )
    {
        if ( !stdControl_aKeyInfo[keyId] )
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
        stdControl_bControlsIdle = false;
    }

    return deltaTime;
}

int J3DAPI stdControl_ReadAxisAsKey(size_t axis, int* pbPressed)
{
    size_t aid = STDCONTROL_GETAID(axis);
    if ( aid >= STDCONTROL_MAX_AXES )
    {
        return 0;
    }

    if ( (axis & STDCONTROL_AID_LOW_SENSITIVITY) == 0 || (stdControl_aAxes[aid].flags & STDCONTROL_AXIS_GAMEPAD) != 0 )
    {
        return stdControl_ReadAxisAsKeyEx(axis, pbPressed, 0.25f);
    }
    else
    {
        return stdControl_ReadAxisAsKeyEx(axis, pbPressed, 0.75f);
    }
}

int J3DAPI stdControl_ReadAxisAsKeyEx(size_t axis, int* pbPressed, float lowValue)
{
    float pos = stdControl_ReadAxis(axis);
    if ( (axis & (STDCONTROL_AID_LOW_SENSITIVITY | STDCONTROL_AID_POSITIVE_AXIS | STDCONTROL_AID_NEGATIVE_AXIS)) != 0 )
    {
        int axisFlag = axis & STDCONTROL_AID_NEGATIVE_AXIS;
        if ( axisFlag == STDCONTROL_AID_POSITIVE_AXIS && pos > (double)lowValue )
        {
            if ( pbPressed )
            {
                *pbPressed = 1;
            }
            return 1;
        }

        if ( axisFlag == STDCONTROL_AID_NEGATIVE_AXIS && -lowValue > pos )
        {
            // Fixed: Increment num key pressed by 1
            if ( pbPressed )
            {
                *pbPressed = 1;
            }
            return 1;
        }
    }
    else
    {
        if ( fabsf(pos) > lowValue )
        {
            // Fixed: Increment num key pressed by 1
            if ( pbPressed )
            {
                *pbPressed = 1;
            }
            return 1;
        }
    }

    return 0;
}

int J3DAPI stdControl_ReadKey(size_t keyNum, int* pbPressed)
{
    STD_ASSERTREL(keyNum < STDCONTROL_MAX_KEYID);
    STD_GUARDEX(keyNum < STDCONTROL_MAX_KEYID, {
        if ( pbPressed )
        {
            *pbPressed = 0;
        }
        return 0;
    });

    if ( stdControl_bControlsActive )
    {
        if ( pbPressed )
        {
            *pbPressed += stdControl_aKeyPressed[keyNum];
        }

        if ( stdControl_bControlsIdle && stdControl_aKeyInfo[keyNum] )
        {
            stdControl_bControlsIdle = false;
        }

        return stdControl_aKeyInfo[keyNum];
    }
    else
    {
        if ( pbPressed )
        {
            *pbPressed = 0;
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
        int range = lround(stdControl_mouseYRange * 200.0f);
        stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Y, -range, range, 0.0f);
    }
}

int stdControl_ControlsActive(void)
{
    return stdControl_bControlsActive ? 1 : 0;
}

void J3DAPI stdControl_UpdateKeyState(int keyId, int bPressed, unsigned int tickTime)
{
    if ( keyId < 0 || (size_t)keyId >= STDCONTROL_MAX_KEYID )
    {
        return;
    }

    if ( bPressed && !stdControl_aKeyInfo[keyId] )
    {
        stdControl_aKeyInfo[keyId]      = true;
        stdControl_aKeyIdleTimes[keyId] = stdControl_curReadTime - tickTime;
        ++stdControl_aKeyPressed[keyId];
    }
    else if ( !bPressed && stdControl_aKeyInfo[keyId] )
    {
        stdControl_aKeyInfo[keyId] = false;
        if ( !stdControl_aKeyIdleTimes[keyId] )
        {
            stdControl_aKeyIdleTimes[keyId] = stdControl_readDeltaTime;
        }

        stdControl_aKeyIdleTimes[keyId] -= stdControl_curReadTime - tickTime;
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
            hr = IDirectInputDevice8_Acquire(stdControl_mouse.pDIDevice);
        }

        if ( stdControl_keyboard.pDIDevice )
        {
            IDirectInputDevice8_Acquire(stdControl_keyboard.pDIDevice);
        }

        for ( size_t joyNum = 0; joyNum < stdControl_numJoystickDevices; ++joyNum )
        {
            if ( stdControl_aJoystickDevices[joyNum].pDIDevice )
            {
                IDirectInputDevice8_Acquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
            }
        }

        stdControl_bControlsActive = true;
    }
    else
    {
        if ( stdControl_mouse.pDIDevice )
        {
            hr = IDirectInputDevice8_Unacquire(stdControl_mouse.pDIDevice);
        }

        if ( stdControl_keyboard.pDIDevice )
        {
            IDirectInputDevice8_Unacquire(stdControl_keyboard.pDIDevice);
        }

        for ( size_t joyNum = 0; joyNum < stdControl_numJoystickDevices; ++joyNum )
        {
            if ( stdControl_aJoystickDevices[joyNum].pDIDevice )
            {
                IDirectInputDevice8_Unacquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
            }
        }

        stdControl_bControlsActive = false;
    }

    return hr;
}

int stdControl_ToggleMouse(void)
{
    if ( stdControl_bReadMouse )
    {
        stdControl_bReadMouse = false;
        if ( stdControl_mouse.pDIDevice && SUCCEEDED(IDirectInputDevice8_Unacquire(stdControl_mouse.pDIDevice)) )
        {
            stdControl_ShowMouseCursor(1);
        }
    }
    else
    {
        stdControl_bReadMouse = true;
        if ( stdControl_mouse.pDIDevice && SUCCEEDED(IDirectInputDevice8_Acquire(stdControl_mouse.pDIDevice)) )
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
    return stdControl_bControlsIdle ? 1 : 0;
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

void stdControl_InitJoysticks(void)
{
    for ( size_t joyNum = 0; joyNum < stdControl_numJoystickDevices; ++joyNum )
    {
        HRESULT hres = IDirectInput8_CreateDevice(stdControl_pDI, &stdControl_aJoystickDevices[joyNum].dinstance.guidInstance, &stdControl_aJoystickDevices[joyNum].pDIDevice, NULL);
        if FAILED(hres)
        {
            STDLOG_ERROR("Could not create DInput8 Joystick device.\n");
            goto error;
        }

        stdControl_aJoystickDevices[joyNum].caps.dwSize = sizeof(DIDEVCAPS);
        hres = IDirectInputDevice8_GetCapabilities(stdControl_aJoystickDevices[joyNum].pDIDevice, &stdControl_aJoystickDevices[joyNum].caps);
        if FAILED(hres)
        {
            goto error;
        }

        hres = IDirectInputDevice8_SetDataFormat(stdControl_aJoystickDevices[joyNum].pDIDevice, &c_dfDIJoystick);
        if FAILED(hres)
        {
            goto error;
        }

        HWND hwnd = stdWin95_GetWindow();
        hres = IDirectInputDevice8_SetCooperativeLevel(stdControl_aJoystickDevices[joyNum].pDIDevice, hwnd, DISCL_BACKGROUND | DISCL_EXCLUSIVE);
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

        if SUCCEEDED(IDirectInputDevice8_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_X(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        dirange.diph.dwObj = DIJOFS_Y;
        if SUCCEEDED(IDirectInputDevice8_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_Y(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        dirange.diph.dwObj = DIJOFS_Z;
        if SUCCEEDED(IDirectInputDevice8_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_Z(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        dirange.diph.dwObj = DIJOFS_RX;
        if SUCCEEDED(IDirectInputDevice8_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_RX(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        dirange.diph.dwObj = DIJOFS_RY;
        if SUCCEEDED(IDirectInputDevice8_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_RY(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        dirange.diph.dwObj = DIJOFS_RZ;
        if SUCCEEDED(IDirectInputDevice8_GetProperty(stdControl_aJoystickDevices[joyNum].pDIDevice, DIPROP_RANGE, &dirange.diph))
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_RZ(joyNum), dirange.lMin, dirange.lMax, 0.2f);
        }

        if ( GET_DIDEVICE_TYPE(stdControl_aJoystickDevices[joyNum].dinstance.dwDevType) == DI8DEVTYPE_GAMEPAD )
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
            // DX8 Note: Force feedback initialization would be different in DX8
            // Could use IDirectInputDevice8::CreateEffect and IDirectInputEffect interface
            // TODO: missing logic for force feedback setup
        }

        hres = IDirectInputDevice8_Acquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
        if FAILED(hres)
        {
        error:
            STDLOG_STATUS("%s error Acquiring Joystick.\n", stdControl_DIGetStatus(hres));
            if ( stdControl_aJoystickDevices[joyNum].pDIDevice )
            {
                IDirectInputDevice8_Release(stdControl_aJoystickDevices[joyNum].pDIDevice);
            }

            stdControl_aJoystickDevices[joyNum].pDIDevice = NULL;

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
        HRESULT hres = IDirectInput8_CreateDevice(stdControl_pDI, &GUID_SysKeyboard, &stdControl_keyboard.pDIDevice, NULL);
        if FAILED(hres)
        {
            goto error;
        }

        stdControl_keyboard.diDevCaps.dwSize = sizeof(stdControl_keyboard.diDevCaps);
        hres = IDirectInputDevice8_GetCapabilities(stdControl_keyboard.pDIDevice, &stdControl_keyboard.diDevCaps);
        if FAILED(hres)
        {
            goto error;
        }

        hres = IDirectInputDevice8_SetDataFormat(stdControl_keyboard.pDIDevice, &c_dfDIKeyboard);
        if FAILED(hres)
        {
            goto error;
        }

        HWND hwnd = stdWin95_GetWindow();
        hres = bForeground
            ? IDirectInputDevice8_SetCooperativeLevel(stdControl_keyboard.pDIDevice, hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)
            : IDirectInputDevice8_SetCooperativeLevel(stdControl_keyboard.pDIDevice, hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
        if ( FAILED(hres) )
        {
            goto error;
        }

        DIPROPDWORD didpw;
        didpw.diph.dwSize       = sizeof(DIPROPDWORD);
        didpw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        didpw.diph.dwObj        = 0;
        didpw.diph.dwHow        = DIPH_DEVICE;
        didpw.dwData            = STD_ARRAYLEN(stdControl_aKeyboardState); // input buffer size

        hres = IDirectInputDevice8_SetProperty(stdControl_keyboard.pDIDevice, DIPROP_BUFFERSIZE, &didpw.diph);
        if ( hres != DI_OK && hres != DI_PROPNOEFFECT )
        {
        error:
            STDLOG_STATUS("%s error Acquiring Keyboard.\n", stdControl_DIGetStatus(hres));

            if ( stdControl_keyboard.pDIDevice )
            {
                IDirectInputDevice8_Release(stdControl_keyboard.pDIDevice);
            }
            stdControl_keyboard.pDIDevice = NULL;
        }
    }
}

void stdControl_InitMouse(void)
{
    if ( stdControl_pDI )
    {
        HRESULT hres = IDirectInput8_CreateDevice(stdControl_pDI, &GUID_SysMouse, &stdControl_mouse.pDIDevice, NULL);
        if FAILED(hres)
        {
            goto error;
        }

        stdControl_mouse.diDevCaps.dwSize = sizeof(stdControl_mouse.diDevCaps);
        hres = IDirectInputDevice8_GetCapabilities(stdControl_mouse.pDIDevice, &stdControl_mouse.diDevCaps);
        if FAILED(hres)
        {
            goto error;
        }

        hres = IDirectInputDevice8_SetDataFormat(stdControl_mouse.pDIDevice, &c_dfDIMouse);
        if FAILED(hres)
        {
            goto error;
        }

        HWND hwnd = stdWin95_GetWindow();
        hres = IDirectInputDevice8_SetCooperativeLevel(stdControl_mouse.pDIDevice, hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
        if ( FAILED(hres) )
        {
            goto error;
        }

        DIPROPDWORD didpw;
        didpw.diph.dwSize       = sizeof(DIPROPDWORD);
        didpw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        didpw.diph.dwObj        = 0;
        didpw.diph.dwHow        = DIPH_DEVICE;
        didpw.dwData            = STDCONTROL_MOUSE_BUFFERSIZE; // input buffer size

        hres = IDirectInputDevice8_SetProperty(stdControl_mouse.pDIDevice, DIPROP_BUFFERSIZE, &didpw.diph);
        if ( hres != DI_OK && hres != DI_PROPNOEFFECT )
        {
        error:
            STDLOG_STATUS("%s error Acquiring Mouse.\n", stdControl_DIGetStatus(hres));
            if ( stdControl_mouse.pDIDevice )
            {
                IDirectInputDevice8_Release(stdControl_mouse.pDIDevice);
            }

            stdControl_mouse.pDIDevice = NULL;
            return;
        }

        stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_X, -250, 250, 0.0f);
        stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Y, -200, 200, 0.0f);
        stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Z, -20, 20, 0.0f);
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
        stdControl_bReadXInput    = true;
    }
}

static void stdControl_ApplyKeyboardState(const uint8_t* aKeyState, size_t numKeys)
{
    size_t numKeysToApply = J3DMIN(numKeys, STDCONTROL_MAX_KEYBOARD_BUTTONS);

    for ( size_t keyId = 0; keyId < numKeysToApply; ++keyId )
    {
        stdControl_UpdateKeyState((int)keyId, aKeyState[keyId] & 0x80, stdControl_curReadTime);
    }
}

void stdControl_ReadKeyboard(void)
{
    if ( !stdControl_keyboard.pDIDevice )
    {
        return;
    }

    HRESULT hr = IDirectInputDevice8_GetDeviceState(stdControl_keyboard.pDIDevice, STD_ARRAYLEN(stdControl_aKeyboardState), stdControl_aKeyboardState);
    if ( hr != DIERR_NOTACQUIRED && hr != DIERR_INPUTLOST )
    {
        if ( hr == DI_OK )
        {
            stdControl_ApplyKeyboardState(stdControl_aKeyboardState, STD_ARRAYLEN(stdControl_aKeyboardState));

            return;
        }
        STDLOG_ERROR("GetDeviceState from keyboard returned %s.\n", stdControl_DIGetStatus(hr));
    }

    hr = IDirectInputDevice8_Acquire(stdControl_keyboard.pDIDevice);
    if ( hr != DI_OK && hr != DIERR_OTHERAPPHASPRIO )
    {
        STDLOG_ERROR("Acquire keyboard returned %s.\n", stdControl_DIGetStatus(hr));
    }
}

static void stdControl_ApplyJoystickState(size_t joyNum, const DIJOYSTATE* pState, size_t numPovs)
{
    if ( !pState || joyNum >= STDCONTROL_MAX_JOYSTICK_DEVICES )
    {
        return;
    }

    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_X(joyNum)]  = pState->lX;
    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_Y(joyNum)]  = pState->lY;
    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_Z(joyNum)]  = pState->lZ;
    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RX(joyNum)] = pState->lRx;
    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RY(joyNum)] = pState->lRy;
    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RZ(joyNum)] = pState->lRz;

    for ( size_t btnNum = 0; btnNum < STDCONTROL_NUM_JOYSTICK_BUTTONS; ++btnNum )
    {
        stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETBUTTON(joyNum, btnNum), pState->rgbButtons[btnNum], stdControl_curReadTime);
    }

    numPovs = J3DMIN(numPovs, STDCONTROL_MAX_JOYSTICK_POVCONTROLERS);
    for ( size_t povNum = 0; povNum < numPovs; ++povNum )
    {
        DWORD pov     = pState->rgdwPOV[povNum];
        bool bCentred = (uint16_t)pov == 0xFFFF;

        stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVLEFT(joyNum, povNum), pov >= 225 * DI_DEGREES && pov <= 315 * DI_DEGREES, stdControl_curReadTime);
        stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVUP(joyNum, povNum), !bCentred && (pov <= 45 * DI_DEGREES || pov >= 315 * DI_DEGREES), stdControl_curReadTime);
        stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVRIGHT(joyNum, povNum), pov >= 45 * DI_DEGREES && pov <= 135 * DI_DEGREES, stdControl_curReadTime);
        stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVDOWN(joyNum, povNum), pov >= 135 * DI_DEGREES && pov <= 225 * DI_DEGREES, stdControl_curReadTime);
    }
}

void stdControl_ReadJoysticks(void)
{
    for ( size_t joyNum = 0; joyNum < stdControl_numJoystickDevices; joyNum++ )
    {
        if ( !stdControl_aJoystickDevices[joyNum].pDIDevice )
        {
            continue;
        }

        HRESULT hr = IDirectInputDevice8_Poll(stdControl_aJoystickDevices[joyNum].pDIDevice);
        if FAILED(hr)
        {
            STDLOG_STATUS("%s error Poll Joystick.\n", stdControl_DIGetStatus(hr));
            IDirectInputDevice8_Acquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
            continue;
        }

        DIJOYSTATE jstate;
        hr = IDirectInputDevice8_GetDeviceState(stdControl_aJoystickDevices[joyNum].pDIDevice, sizeof(DIJOYSTATE), &jstate);
        if FAILED(hr)
        {
            STDLOG_STATUS("%s error GetDeviceState Joystick.\n", stdControl_DIGetStatus(hr));
            IDirectInputDevice8_Acquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
            continue;
        }

        stdControl_ApplyJoystickState(joyNum, &jstate, stdControl_aJoystickDevices[joyNum].caps.dwPOVs);
    }
}

static void stdControl_ApplyMouseAxisState(const DIMOUSESTATE* pState)
{
    if ( stdControl_bMouseSensitivityEnabled )
    {
        stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X] += pState->lX;
        stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X]  = STDMATH_CLAMP(stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X], stdControl_aAxes[STDCONTROL_AID_MOUSE_X].min, stdControl_aAxes[STDCONTROL_AID_MOUSE_X].max);

        stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y] += pState->lY;
        stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y]  = STDMATH_CLAMP(stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y], stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].min, stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].max);

        stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z] += pState->lZ;
        stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z]  = STDMATH_CLAMP(stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z], stdControl_aAxes[STDCONTROL_AID_MOUSE_Z].min, stdControl_aAxes[STDCONTROL_AID_MOUSE_Z].max);
        return;
    }

    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X] = pState->lX;
    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y] = pState->lY;
    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z] = pState->lZ;
    if ( stdControl_readDeltaTime < 25 )
    {
        stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X] = (stdControl_mousePos.x + pState->lX) / 2;
        stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y] = (stdControl_mousePos.y + pState->lY) / 2;
    }

    stdControl_mousePos.x = pState->lX;
    stdControl_mousePos.y = pState->lY;
}

static void stdControl_ApplyMouseButtonState(const DIMOUSESTATE* pState)
{
    for ( size_t i = 0; i < STDCONTROL_MAX_MOUSE_BUTTONS; ++i )
    {
        stdControl_UpdateKeyState(STDCONTROL_KID_MOUSE_LBUTTON + (int)i, pState->rgbButtons[i], stdControl_curReadTime);
    }
}

void stdControl_ReadMouse(void)
{
    DIDEVICEOBJECTDATA aMouseBuffer[STDCONTROL_MOUSE_BUFFERSIZE] = { 0 }; // Added: Init to 0
    if ( !stdControl_bReadMouse || !stdControl_mouse.pDIDevice )
    {
        return;
    }

    DIMOUSESTATE mouseState = { 0 };
    bool bStateValid = false;
    HRESULT hr = IDirectInputDevice8_GetDeviceState(stdControl_mouse.pDIDevice, sizeof(DIMOUSESTATE), &mouseState);
    if ( hr != DI_OK )
    {
        if ( hr != DIERR_NOTACQUIRED && hr != DIERR_INPUTLOST )
        {
            STDLOG_ERROR("GetDeviceState(mouse) returned %s.\n", stdControl_DIGetStatus(hr));
        }

        hr = IDirectInputDevice8_Acquire(stdControl_mouse.pDIDevice);
        if ( hr != DI_OK && hr != DIERR_OTHERAPPHASPRIO )
        {
            STDLOG_ERROR("Acquire mouse returned %s.\n", stdControl_DIGetStatus(hr));
        }
    }
    else
    {
        bStateValid = true;
        stdControl_ApplyMouseAxisState(&mouseState);
    }

    DWORD bufferSize = STDCONTROL_MOUSE_BUFFERSIZE;
    hr = IDirectInputDevice8_GetDeviceData(stdControl_mouse.pDIDevice, sizeof(DIDEVICEOBJECTDATA), aMouseBuffer, &bufferSize, 0);
    if ( hr != DI_OK )
    {
        HRESULT dataResult = hr;
        if ( dataResult == DI_BUFFEROVERFLOW && bStateValid )
        {
            stdControl_ApplyMouseButtonState(&mouseState);
        }
        else
        {
            HRESULT acquireResult = IDirectInputDevice8_Acquire(stdControl_mouse.pDIDevice);
            if ( dataResult != DIERR_NOTACQUIRED && dataResult != DIERR_INPUTLOST )
            {
                STDLOG_ERROR("GetDeviceData from mouse returned %s.\n", stdControl_DIGetStatus(dataResult));
            }
            if ( acquireResult != DI_OK && acquireResult != DIERR_OTHERAPPHASPRIO )
            {
                STDLOG_ERROR("Acquire mouse returned %s.\n", stdControl_DIGetStatus(acquireResult));
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
                static_assert((STDCONTROL_KID_MOUSE_LBUTTON - DIMOFS_BUTTON0) == 820, "(STDCONTROL_KID_MOUSE_LBUTTON - DIMOFS_BUTTON0) == 820");
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
    STD_GUARD_VOID(aid < STDCONTROL_MAX_AXES && max > min);

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

BOOL stdControl_IsXInputDevice(const GUID* pGuidProductFromDirectInput)
{
    IWbemLocator* pIWbemLocator        = NULL;
    IEnumWbemClassObject* pEnumDevices = NULL;
    IWbemClassObject* pDevices[20]     = { 0 };
    IWbemServices* pIWbemServices      = NULL;

    BSTR bstrNamespace   = NULL;
    BSTR bstrDeviceID    = NULL;
    BSTR bstrClassName   = NULL;
    bool bIsXinputDevice = false;

    // CoInit if needed
    HRESULT hr = CoInitialize(NULL);
    bool bCleanupCOM = SUCCEEDED(hr);

    // So we can call VariantClear() later, even if we never had a successful IWbemClassObject::Get().
    VARIANT var = { 0 };
    VariantInit(&var);

    // Create WMI
    hr = CoCreateInstance(&CLSID_WbemLocator,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IWbemLocator,
        (LPVOID*)&pIWbemLocator);
    if ( FAILED(hr) || pIWbemLocator == NULL )
    {
        goto LCleanup;
    }

    bstrNamespace = SysAllocString(L"\\\\.\\root\\cimv2");  if ( bstrNamespace == NULL ) goto LCleanup;
    bstrClassName = SysAllocString(L"Win32_PNPEntity");     if ( bstrClassName == NULL ) goto LCleanup;
    bstrDeviceID  = SysAllocString(L"DeviceID");            if ( bstrDeviceID == NULL )  goto LCleanup;

    // Connect to WMI
    hr = pIWbemLocator->lpVtbl->ConnectServer(pIWbemLocator, bstrNamespace, NULL, NULL, 0L, 0L, NULL, NULL, &pIWbemServices);
    if ( FAILED(hr) || pIWbemServices == NULL )
    {
        goto LCleanup;
    }

    // Switch security level to IMPERSONATE.
    hr = CoSetProxyBlanket((IUnknown*)pIWbemServices,
        RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE);
    if ( FAILED(hr) )
    {
        goto LCleanup;
    }

    hr = pIWbemServices->lpVtbl->CreateInstanceEnum(pIWbemServices, bstrClassName, 0, NULL, &pEnumDevices);
    if ( FAILED(hr) || pEnumDevices == NULL )
    {
        goto LCleanup;
    }

    // Loop over all devices
    for ( ;;)
    {
        ULONG uReturned = 0;
        hr = pEnumDevices->lpVtbl->Next(pEnumDevices, 10000, sizeof(pDevices) / sizeof(pDevices[0]), pDevices, &uReturned);
        if ( FAILED(hr) )
        {
            goto LCleanup;
        }

        if ( uReturned == 0 )
            break;

        for ( size_t iDevice = 0; iDevice < uReturned; ++iDevice )
        {
            if ( pDevices[iDevice] == NULL ) continue;  // Safety check to shut up IntelliSense

            // For each device, get its device ID
            hr = pDevices[iDevice]->lpVtbl->Get(pDevices[iDevice], bstrDeviceID, 0L, &var, NULL, NULL);
            if ( SUCCEEDED(hr) && V_VT(&var) == VT_BSTR && V_BSTR(&var) != NULL )
            {
                // Check if the device ID contains "IG_".  If it does, then it's an XInput device
                // This information cannot be found from DirectInput
                if ( wcsstr(V_BSTR(&var), L"IG_") )
                {
                    // If it does, then get the VID/PID from var.bstrVal
                    DWORD dwPid = 0, dwVid = 0;
                    WCHAR* strVid = wcsstr(V_BSTR(&var), L"VID_");
                    if ( strVid && swscanf_s(strVid, L"VID_%4X", &dwVid) != 1 )
                    {
                        dwVid = 0;
                    }

                    WCHAR* strPid = wcsstr(V_BSTR(&var), L"PID_");
                    if ( strPid && swscanf_s(strPid, L"PID_%4X", &dwPid) != 1 )
                    {
                        dwPid = 0;
                    }

                    // Compare the VID/PID to the DInput device
                    DWORD dwVidPid = MAKELONG(dwVid, dwPid);
                    if ( dwVidPid == pGuidProductFromDirectInput->Data1 )
                    {
                        bIsXinputDevice = true;
                        goto LCleanup;
                    }
                }
            }
            VariantClear(&var);
            STDCONTROL_COMSAFE_RELEASE(pDevices[iDevice]);
        }
    }

LCleanup:
    VariantClear(&var);

    if ( bstrNamespace )
    {
        SysFreeString(bstrNamespace);
    }

    if ( bstrDeviceID )
    {
        SysFreeString(bstrDeviceID);
    }

    if ( bstrClassName )
    {
        SysFreeString(bstrClassName);
    }

    for ( size_t iDevice = 0; iDevice < sizeof(pDevices) / sizeof(pDevices[0]); ++iDevice )
    {
        STDCONTROL_COMSAFE_RELEASE(pDevices[iDevice]);
    }

    STDCONTROL_COMSAFE_RELEASE(pEnumDevices);
    STDCONTROL_COMSAFE_RELEASE(pIWbemLocator);
    STDCONTROL_COMSAFE_RELEASE(pIWbemServices);

    if ( bCleanupCOM )
    {
        CoUninitialize();
    }

    return bIsXinputDevice;
}

BOOL CALLBACK stdControl_EnumDevicesCallback(LPCDIDEVICEINSTANCE pdidInstance, LPVOID pContext)
{
    J3D_UNUSED(pContext);

    DWORD dwDevType = GET_DIDEVICE_TYPE(pdidInstance->dwDevType);
    if ( dwDevType == DI8DEVTYPE_MOUSE )
    {
        STDLOG_STATUS("Mouse:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
        return DIENUM_CONTINUE; // continue to find other devices
    }

    if ( dwDevType == DI8DEVTYPE_KEYBOARD )
    {
        STDLOG_STATUS("Keyboard:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
        return DIENUM_CONTINUE; // continue to find other devices
    }

    if ( dwDevType == DI8DEVTYPE_DEVICE )
    {
        STDLOG_DEBUG("Skipping unspecified device:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
        return DIENUM_CONTINUE; // continue to find other devices
    }

    // Check if the device is an XInput device (e.g.: xbox controller)
    if ( stdControl_IsXInputDevice(&pdidInstance->guidProduct) )
    {
        STDLOG_DEBUG("Skipping XInput device:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
        return DIENUM_CONTINUE; // skip XInput devices
    }

    // Handle joystick, gamepad, and other devices
    if ( dwDevType == DI8DEVTYPE_JOYSTICK && stdControl_numJoystickDevices < STD_ARRAYLEN(stdControl_aJoystickDevices) )
    {
        memcpy(&stdControl_aJoystickDevices[stdControl_numJoystickDevices++].dinstance, pdidInstance, sizeof(DIDEVICEINSTANCE));

        // DX8 Note: Joystick subtypes are simplified in DirectInput8
        switch ( GET_DIDEVICE_SUBTYPE(pdidInstance->dwDevType) )
        {
            case DI8DEVTYPEJOYSTICK_LIMITED:
                STDLOG_STATUS("Joystick (Limited):%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPEJOYSTICK_STANDARD:
                STDLOG_STATUS("Joystick (Standard):%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            default:
                STDLOG_STATUS("Joystick (Unknown):%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
        }
    }
    else if ( dwDevType == DI8DEVTYPE_REMOTE && stdControl_numJoystickDevices < STD_ARRAYLEN(stdControl_aJoystickDevices) )
    {
        memcpy(&stdControl_aJoystickDevices[stdControl_numJoystickDevices++].dinstance, pdidInstance, sizeof(DIDEVICEINSTANCE));
        STDLOG_STATUS("Remote Control:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
    }
    else if ( dwDevType == DI8DEVTYPE_SUPPLEMENTAL && stdControl_numJoystickDevices < STD_ARRAYLEN(stdControl_aJoystickDevices) )
    {
        memcpy(&stdControl_aJoystickDevices[stdControl_numJoystickDevices++].dinstance, pdidInstance, sizeof(DIDEVICEINSTANCE));

        switch ( GET_DIDEVICE_SUBTYPE(pdidInstance->dwDevType) )
        {
            case DI8DEVTYPESUPPLEMENTAL_2NDHANDCONTROLLER:
                STDLOG_STATUS("Supplemental (2nd Hand):%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPESUPPLEMENTAL_HEADTRACKER:
                STDLOG_STATUS("HeadTracker:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPESUPPLEMENTAL_HANDTRACKER:
                STDLOG_STATUS("Hand Tracker:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPESUPPLEMENTAL_SHIFTSTICKGATE:
                STDLOG_STATUS("Shift Stick:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPESUPPLEMENTAL_SHIFTER:
                STDLOG_STATUS("Shifter:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPESUPPLEMENTAL_THROTTLE:
                STDLOG_STATUS("Throttle:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPESUPPLEMENTAL_SPLITTHROTTLE:
                STDLOG_STATUS("Split Throttle:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPESUPPLEMENTAL_COMBINEDPEDALS:
                STDLOG_STATUS("Combined Pedals:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPESUPPLEMENTAL_DUALPEDALS:
                STDLOG_STATUS("Dual Pedals:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPESUPPLEMENTAL_THREEPEDALS:
                STDLOG_STATUS("Three Pedals:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPESUPPLEMENTAL_RUDDERPEDALS:
                STDLOG_STATUS("Rudder:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            default:
                STDLOG_STATUS("Supplemental:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
        }
    }
    else if ( dwDevType == DI8DEVTYPE_SCREENPOINTER )
    {
        STDLOG_STATUS("Screen Pointer:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
    }
    else if ( dwDevType == DI8DEVTYPE_DEVICECTRL )
    {
        STDLOG_STATUS("Device Control:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
    }
    else if ( dwDevType == DI8DEVTYPE_GAMEPAD && stdControl_numJoystickDevices < STD_ARRAYLEN(stdControl_aJoystickDevices) )
    {
        memcpy(&stdControl_aJoystickDevices[stdControl_numJoystickDevices++].dinstance, pdidInstance, sizeof(DIDEVICEINSTANCE));

        switch ( GET_DIDEVICE_SUBTYPE(pdidInstance->dwDevType) )
        {
            case DI8DEVTYPEGAMEPAD_STANDARD:
                STDLOG_STATUS("Gamepad (Standard):%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPEGAMEPAD_TILT:
                STDLOG_STATUS("Gamepad (Tilt):%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            default:
                STDLOG_STATUS("Gamepad:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
        }
    }
    else if ( dwDevType == DI8DEVTYPE_DRIVING && stdControl_numJoystickDevices < STD_ARRAYLEN(stdControl_aJoystickDevices) )
    {
        memcpy(&stdControl_aJoystickDevices[stdControl_numJoystickDevices++].dinstance, pdidInstance, sizeof(DIDEVICEINSTANCE));

        switch ( GET_DIDEVICE_SUBTYPE(pdidInstance->dwDevType) )
        {
            case DI8DEVTYPEDRIVING_COMBINEDPEDALS:
                STDLOG_STATUS("Wheel (Combined Pedals):%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPEDRIVING_DUALPEDALS:
                STDLOG_STATUS("Wheel (Dual Pedals):%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPEDRIVING_THREEPEDALS:
                STDLOG_STATUS("Wheel (Three Pedals):%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPEDRIVING_HANDHELD:
                STDLOG_STATUS("Driving (Handheld):%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            default:
                STDLOG_STATUS("Wheel:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
        }
    }
    else if ( dwDevType == DI8DEVTYPE_FLIGHT && stdControl_numJoystickDevices < STD_ARRAYLEN(stdControl_aJoystickDevices) )
    {
        memcpy(&stdControl_aJoystickDevices[stdControl_numJoystickDevices++].dinstance, pdidInstance, sizeof(DIDEVICEINSTANCE));

        switch ( GET_DIDEVICE_SUBTYPE(pdidInstance->dwDevType) )
        {
            case DI8DEVTYPEFLIGHT_STICK:
                STDLOG_STATUS("Flightstick:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPEFLIGHT_YOKE:
                STDLOG_STATUS("Flight Yoke:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPEFLIGHT_RC:
                STDLOG_STATUS("Flight RC:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            default:
                STDLOG_STATUS("Flightstick:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
        }
    }
    else if ( dwDevType == DI8DEVTYPE_1STPERSON && stdControl_numJoystickDevices < STD_ARRAYLEN(stdControl_aJoystickDevices) )
    {
        memcpy(&stdControl_aJoystickDevices[stdControl_numJoystickDevices++].dinstance, pdidInstance, sizeof(DIDEVICEINSTANCE));

        switch ( GET_DIDEVICE_SUBTYPE(pdidInstance->dwDevType) )
        {
            case DI8DEVTYPE1STPERSON_SIXDOF:
                STDLOG_STATUS("HeadTracker:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            case DI8DEVTYPE1STPERSON_SHOOTER:
                STDLOG_STATUS("Shooter:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
            default:
                STDLOG_STATUS("1st Person:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName);
                break;
        }
    }

    return DIENUM_CONTINUE;
}

void stdControl_ResetMousePos(void)
{
    // TODO: missing `mousePos` reset
    /*stdControl_mousePos.x = 0;
    stdControl_mousePos.y = 0;*/
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

    if ( stdControl_numXInputDevices > 0 && maxButtons < 10 )
    {
        maxButtons = 10; // XInput devices have at least 10 buttons
    }

    return maxButtons;
}

size_t stdControl_GetNumJoysticks(void)
{
    return stdControl_numJoystickDevices + stdControl_numXInputDevices; // TODO: when gamepad dedicated slots are added remove XInput devices from this count
}

const char* J3DAPI stdControl_GetJoysticDescription(int joyNum)
{
    if ( joyNum < 0 || joyNum >= (int)stdControl_GetNumJoysticks() )
    {
        return "";
    }

    if ( joyNum >= (int)stdControl_numJoystickDevices )
    {
        joyNum -= stdControl_numJoystickDevices;
        if ( joyNum >= (int)stdControl_numXInputDevices || !stdControl_aXInputDevices[joyNum].bConnected )
        {
            return "";
        }
        STD_FORMAT(
            stdControl_aStrBuf,
            "XInput%d:Gamepad",
            stdControl_aXInputDevices[joyNum].userIndex + 1
        );
        return stdControl_aStrBuf;
    }

    STD_FORMAT(
        stdControl_aStrBuf,
        "%s:%s",
        stdControl_aJoystickDevices[joyNum].dinstance.tszProductName,
        stdControl_aJoystickDevices[joyNum].dinstance.tszInstanceName
    );
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
    // TODO: when gamepad dedicated slots are added, use those instead
    if ( joyNum < 0 || joyNum >= (int)stdControl_GetNumJoysticks() )
    {
        return 0;
    }

    return stdControl_IsXInputJoystickIndex(joyNum, stdControl_numJoystickDevices)
        ? stdControl_aXInputDevices[joyNum - stdControl_numJoystickDevices].bIsGamepad
        : GET_DIDEVICE_TYPE(stdControl_aJoystickDevices[joyNum].dinstance.dwDevType) == DI8DEVTYPE_GAMEPAD;
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

static int stdControl_FindXInputDevice(DWORD userIndex)
{
    for ( size_t i = 0; i < stdControl_numXInputDevices; ++i )
    {
        if ( stdControl_aXInputDevices[i].userIndex == userIndex )
        {
            return (int)i;
        }
    }

    return -1;
}

static bool stdControl_AddXInputDevice(DWORD userIndex, const XINPUT_STATE* pState, const XINPUT_CAPABILITIES* pCaps)
{
    if ( stdControl_numXInputDevices >= STD_ARRAYLEN(stdControl_aXInputDevices) )
    {
        return false;
    }

    size_t deviceIndex = stdControl_numXInputDevices;
    size_t joySlot     = stdControl_numJoystickDevices + deviceIndex;
    if ( joySlot >= STDCONTROL_MAX_JOYSTICK_DEVICES )
    {
        return false;
    }

    StdControlXInputDevice* pDevice = &stdControl_aXInputDevices[deviceIndex];
    pDevice->userIndex          = userIndex;
    pDevice->bConnected         = true;
    pDevice->bIsGamepad         = true;
    pDevice->state              = *pState;
    pDevice->caps               = *pCaps;
    pDevice->leftStickDeadZone  = (float)STDCONTROL_XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
    pDevice->rightStickDeadZone = (float)STDCONTROL_XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f;
    pDevice->triggerThreshold   = (float)STDCONTROL_XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f;

    size_t aAxisIds[] =
    {
        STDCONTROL_GET_JOYSTICK_AXIS_X(joySlot),
        STDCONTROL_GET_JOYSTICK_AXIS_Y(joySlot),
        STDCONTROL_GET_JOYSTICK_AXIS_RX(joySlot),
        STDCONTROL_GET_JOYSTICK_AXIS_RY(joySlot),
        STDCONTROL_GET_JOYSTICK_AXIS_Z(joySlot),
        STDCONTROL_GET_JOYSTICK_AXIS_RZ(joySlot)
    };

    stdControl_RegisterAxis(aAxisIds[0], -32768, 32767, pDevice->leftStickDeadZone);
    stdControl_RegisterAxis(aAxisIds[1], -32768, 32767, pDevice->leftStickDeadZone);
    stdControl_RegisterAxis(aAxisIds[2], -32768, 32767, pDevice->rightStickDeadZone);
    stdControl_RegisterAxis(aAxisIds[3], -32768, 32767, pDevice->rightStickDeadZone);
    stdControl_RegisterAxis(aAxisIds[4], -255, 255, pDevice->triggerThreshold);
    stdControl_RegisterAxis(aAxisIds[5], -255, 255, pDevice->triggerThreshold);

    for ( size_t i = 0; i < STD_ARRAYLEN(aAxisIds); ++i )
    {
        stdControl_aAxes[aAxisIds[i]].flags |= STDCONTROL_AXIS_GAMEPAD;
        if ( stdControl_bStartup )
        {
            stdControl_aAxes[aAxisIds[i]].flags |= STDCONTROL_AXIS_ENABLED;
        }
    }

    ++stdControl_numXInputDevices;
    if ( stdControl_bStartup )
    {
        stdControl_bReadXInput = true;
    }

    STDLOG_STATUS("Found XInput gamepad at user index %lu.\n", userIndex);
    return true;
}

static void stdControl_ApplyXInputState(size_t deviceIndex, const XINPUT_STATE* pState)
{
    StdControlXInputDevice* pDevice = &stdControl_aXInputDevices[deviceIndex];
    size_t joySlot = stdControl_numJoystickDevices + deviceIndex;

    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_X(joySlot)] = (int)stdControl_ApplyXInputDeadzone(pState->Gamepad.sThumbLX, pDevice->leftStickDeadZone);
    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_Y(joySlot)] = (int)stdControl_ApplyXInputDeadzone(pState->Gamepad.sThumbLY, pDevice->leftStickDeadZone);
    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RX(joySlot)] = (int)stdControl_ApplyXInputDeadzone(pState->Gamepad.sThumbRX, pDevice->rightStickDeadZone);
    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RY(joySlot)] = (int)stdControl_ApplyXInputDeadzone(pState->Gamepad.sThumbRY, pDevice->rightStickDeadZone);

    BYTE leftTrigger  = pState->Gamepad.bLeftTrigger;
    BYTE rightTrigger = pState->Gamepad.bRightTrigger;
    if ( leftTrigger < STDCONTROL_XINPUT_GAMEPAD_TRIGGER_THRESHOLD )
    {
        leftTrigger = 0;
    }
    if ( rightTrigger < STDCONTROL_XINPUT_GAMEPAD_TRIGGER_THRESHOLD )
    {
        rightTrigger = 0;
    }

    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_Z(joySlot)]  = leftTrigger;
    stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RZ(joySlot)] = rightTrigger;

    WORD buttons = pState->Gamepad.wButtons;
    const WORD aButtonMasks[] =
    {
        XINPUT_GAMEPAD_A,
        XINPUT_GAMEPAD_B,
        XINPUT_GAMEPAD_X,
        XINPUT_GAMEPAD_Y,
        XINPUT_GAMEPAD_LEFT_SHOULDER,
        XINPUT_GAMEPAD_RIGHT_SHOULDER,
        XINPUT_GAMEPAD_BACK,
        XINPUT_GAMEPAD_START,
        XINPUT_GAMEPAD_LEFT_THUMB,
        XINPUT_GAMEPAD_RIGHT_THUMB
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(aButtonMasks); ++i )
    {
        stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETBUTTON(joySlot, i), (buttons & aButtonMasks[i]) != 0, stdControl_curReadTime);
    }

    stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVLEFT(joySlot, 0), (buttons & XINPUT_GAMEPAD_DPAD_LEFT) != 0, stdControl_curReadTime);
    stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVUP(joySlot, 0), (buttons & XINPUT_GAMEPAD_DPAD_UP) != 0, stdControl_curReadTime);
    stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVRIGHT(joySlot, 0), (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0, stdControl_curReadTime);
    stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVDOWN(joySlot, 0), (buttons & XINPUT_GAMEPAD_DPAD_DOWN) != 0, stdControl_curReadTime);
}

static void stdControl_ClearXInputState(size_t deviceIndex)
{
    size_t joySlot = stdControl_numJoystickDevices + deviceIndex;
    for ( size_t axisIndex = 0; axisIndex < STDCONTROL_JOYSTICK_NUMAXES; ++axisIndex )
    {
        size_t aid = STDCONTROL_GET_JOYSTICK_AXIS(joySlot, axisIndex);
        stdControl_aAxisStates[aid] = stdControl_aAxes[aid].center;
    }

    for ( size_t buttonIndex = 0; buttonIndex < 10u; ++buttonIndex )
    {
        stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETBUTTON(joySlot, buttonIndex), 0, stdControl_curReadTime);
    }

    for ( size_t directionIndex = 0; directionIndex < STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS; ++directionIndex )
    {
        stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOV(joySlot, 0, directionIndex), 0, stdControl_curReadTime);
    }
}

static void stdControl_ScanXInputDevices(void)
{
    for ( DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex )
    {
        if ( stdControl_FindXInputDevice(userIndex) >= 0 )
        {
            continue;
        }

        XINPUT_STATE state = { 0 };
        if ( XInputGetState(userIndex, &state) != ERROR_SUCCESS )
        {
            continue;
        }

        XINPUT_CAPABILITIES caps = { 0 };
        if ( XInputGetCapabilities(userIndex, XINPUT_FLAG_GAMEPAD, &caps) != ERROR_SUCCESS )
        {
            continue;
        }

        if ( caps.Type == XINPUT_DEVTYPE_GAMEPAD )
        {
            stdControl_AddXInputDevice(userIndex, &state, &caps);
        }
    }

    stdControl_lastXInputCheck = stdControl_bStartup ? stdControl_curReadTime : stdPlatform_GetTimeMsec();
}

void stdControl_InitXInput(void)
{
    STDLOG_DEBUG("Initializing XInput controllers...\n");
    stdControl_numXInputDevices = 0;
    STD_ZEROMEM(stdControl_aXInputDevices, sizeof(stdControl_aXInputDevices));
    stdControl_ScanXInputDevices();
    STDLOG_STATUS("Total XInput gamepad controllers found: %zu\n", stdControl_numXInputDevices);
}

void stdControl_ReadXInput(void)
{
    if ( (DWORD)(stdControl_curReadTime - stdControl_lastXInputCheck) >= STDCONTROL_XINPUT_RESCAN_INTERVAL_MSEC )
    {
        stdControl_ScanXInputDevices();
    }

    for ( size_t deviceIndex = 0; deviceIndex < stdControl_numXInputDevices; ++deviceIndex )
    {
        StdControlXInputDevice* pDevice = &stdControl_aXInputDevices[deviceIndex];
        XINPUT_STATE state = { 0 };
        DWORD result = XInputGetState(pDevice->userIndex, &state);
        if ( result == ERROR_SUCCESS )
        {
            if ( !pDevice->bConnected )
            {
                pDevice->bConnected = true;
                STDLOG_STATUS("XInput device %lu reconnected.\n", pDevice->userIndex);
            }

            stdControl_ApplyXInputState(deviceIndex, &state);
            pDevice->state = state;
        }
        else if ( pDevice->bConnected )
        {
            stdControl_ClearXInputState(deviceIndex);
            pDevice->bConnected = false;
            STDLOG_STATUS("XInput device %lu disconnected.\n", pDevice->userIndex);
        }
    }
}

// XInput cleanup function
void stdControl_ShutdownXInput(void)
{
    // Stop all vibration
    for ( size_t i = 0; i < stdControl_numXInputDevices; i++ )
    {
        StdControlXInputDevice* pDevice = &stdControl_aXInputDevices[i];
        if ( pDevice->bConnected )
        {
            XINPUT_VIBRATION vibration = { 0, 0 };
            XInputSetState(pDevice->userIndex, &vibration);
        }
    }

    stdControl_numXInputDevices = 0;
    memset(stdControl_aXInputDevices, 0, sizeof(stdControl_aXInputDevices));
}

float J3DAPI stdControl_ApplyXInputDeadzone(SHORT value, float deadzone)
{
    float normalizedValue = (float)value / (float)STDCONTROL_XINPUT_GAMEPAD_THUMB_MAXVALUE;
    float absValue = fabsf(normalizedValue);

    if ( absValue < deadzone )
    {
        return 0.0f; // Within deadzone
    }

    // Scale the value to account for deadzone
    float sign = normalizedValue >= 0.0f ? 1.0f : -1.0f;
    float adjustedValue = (absValue - deadzone) / (1.0f - deadzone);

    return sign * adjustedValue * (float)STDCONTROL_XINPUT_GAMEPAD_THUMB_MAXVALUE;
}

// XInput vibration function
void J3DAPI stdControl_SetXInputVibration(int controllerIndex, float leftMotor, float rightMotor)
{
    if ( controllerIndex < 0 || (size_t)controllerIndex >= stdControl_numXInputDevices )
    {
        return;
    }

    if ( !stdControl_aXInputDevices[controllerIndex].bConnected )
    {
        return;
    }

    // Clamp values to 0.0-1.0 range
    leftMotor  = STDMATH_CLAMP(leftMotor, 0.0f, 1.0f);
    rightMotor = STDMATH_CLAMP(rightMotor, 0.0f, 1.0f);

    XINPUT_VIBRATION vibration;
    vibration.wLeftMotorSpeed  = (WORD)(leftMotor * 65535.0f);
    vibration.wRightMotorSpeed = (WORD)(rightMotor * 65535.0f);

    XInputSetState(stdControl_aXInputDevices[controllerIndex].userIndex, &vibration);
    stdControl_aXInputDevices[controllerIndex].vibration = vibration;
}