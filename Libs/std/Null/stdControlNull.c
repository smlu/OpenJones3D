// Added: Null version of std/Win95/DX9/stdControlDX9.c (DirectInput8/XInput backend).
// There is no input device on this platform: no key is ever pressed and every axis reads 0.
// Startup/Open/activation succeed like the Windows backend, ReadControls keeps the idle
// timing bookkeeping (sithControl_secFPS), and all reader functions mirror the DX9
// handlers' guards so idle behavior is the same as a machine with no input events attached.
#include <j3dcore/j3d.h>

#include <std/Win95/stdControl.h>
#include <std/Win95/stdWin95.h>

#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>

static bool stdControl_bStartup        = false;
static bool stdControl_bOpen           = false;
static bool stdControl_bControlsActive = false;
static bool stdControl_bControlsIdle   = false;
static bool stdControl_bReadMouse      = false;
static bool stdControl_bMouseSensitivityEnabled = false;

static StdControlAxis stdControl_aAxes[STDCONTROL_MAX_AXES] = { 0 };
static int stdControl_aAxisStates[STDCONTROL_MAX_AXES]      = { 0 };
static int stdControl_aKeyInfo[STDCONTROL_MAX_KEYID]        = { 0 };
static int stdControl_aKeyIdleTimes[STDCONTROL_MAX_KEYID]   = { 0 };
static int stdControl_aKeyPressed[STDCONTROL_MAX_KEYID]     = { 0 };

static uint32_t stdControl_curReadTime    = 0;
static uint32_t stdControl_lastReadTime   = 0;
static uint32_t stdControl_readDeltaTime  = 0;
// Added: The read-rate globals stdControlDX9.c carries under the old sithControl_ names.
static float sithControl_secFPS           = 0.0f;
static float sithControl_msecFPS          = 0.0f;
static float stdControl_mouseXRange       = 0.2f;
static float stdControl_mouseYRange       = 0.2f;
static float stdControl_xSensitivity      = 1.0f;
static float stdControl_ySensitivity      = 1.0f;

// Added: Same read-timing update as stdControl_UpdateReadTiming; sithControl reads
// sithControl_secFPS/msecFPS for its own idle bookkeeping.
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
int J3DAPI stdControl_Startup(int bKeyboardForeground)
{
    J3D_UNUSED(bKeyboardForeground);

    if ( stdControl_bStartup )
    {
        return 1;
    }

    memset(stdControl_aKeyInfo, 0, sizeof(stdControl_aKeyInfo));
    memset(stdControl_aKeyIdleTimes, 0, sizeof(stdControl_aKeyIdleTimes));
    memset(stdControl_aKeyPressed, 0, sizeof(stdControl_aKeyPressed));
    memset(stdControl_aAxes, 0, sizeof(stdControl_aAxes));
    memset(stdControl_aAxisStates, 0, sizeof(stdControl_aAxisStates));

    stdControl_bMouseSensitivityEnabled = false;
    stdControl_Reset();

    stdControl_bStartup = true;
    // Returns 0 on success like stdControlDX9.c (1 means already started or failed).
    return 0;
}

bool stdControl_HasStarted(void)
{
    return stdControl_bStartup;
}

void stdControl_Shutdown(void)
{
    stdControl_bStartup = false;
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
    stdControl_bControlsIdle            = false;
    stdControl_bReadMouse               = false;
    stdControl_bMouseSensitivityEnabled = false;

    for ( size_t i = 0; i < STDCONTROL_MAX_AXES; i++ )
    {
        stdControl_aAxes[i].flags &= ~STDCONTROL_AXIS_ENABLED;
    }
}

void stdControl_ResetMousePos(void)
{
    // Added: There is no mouse cursor position to reset on this platform.
}

void stdControl_DisableReadJoysticks(void)
{
}

void stdControl_ReadControls(void)
{
    STD_ASSERTREL(stdControl_bStartup && stdControl_bOpen);

    if ( !stdControl_bControlsActive )
    {
        return;
    }

    // Added: Same structure as stdControlDX9.c ReadControls, including that any device
    // input would clear the idle flag: there is no input device, so controls stay idle.
    stdControl_bControlsIdle = true;
    memset(stdControl_aKeyIdleTimes, 0, sizeof(stdControl_aKeyIdleTimes));
    memset(stdControl_aKeyPressed, 0, sizeof(stdControl_aKeyPressed));

    stdControl_UpdateReadTiming(stdPlatform_GetTimeMsec());
    memset(stdControl_aAxisStates, 0, sizeof(stdControl_aAxisStates));

    stdControl_lastReadTime = stdControl_curReadTime;
}

void stdControl_FinishRead(void)
{
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
    return 1;
}

void J3DAPI stdControl_RegisterAxis(size_t aid, int min, int max, float deadzoneScale)
{
    if ( aid >= STDCONTROL_MAX_AXES )
    {
        return;
    }

    // Added: Same registration math as stdControlDX9.c RegisterAxis; a Null axis never
    // moves from its center, but the flag losses still track registration/enable state.
    stdControl_aAxes[aid].flags     |= STDCONTROL_AXIS_REGISTERED;
    stdControl_aAxes[aid].min        = min;
    stdControl_aAxes[aid].max        = max;
    stdControl_aAxes[aid].center     = (min + max) / 2;
    stdControl_aAxes[aid].scale      = 2.0f / (float)(max - min);
    stdControl_aAxes[aid].deadzoneThreshold = (int)((max - min) * deadzoneScale * 0.05f);
    if ( stdControl_aAxes[aid].deadzoneThreshold )
    {
        stdControl_aAxes[aid].flags |= STDCONTROL_AXIS_HASDEADZONE;
    }
}

void J3DAPI stdControl_RegisterMouseAxesXY(float xrange, float yrange)
{
    stdControl_mouseXRange = xrange;
    stdControl_mouseYRange = yrange;

    if ( (stdControl_aAxes[STDCONTROL_AID_MOUSE_X].flags & STDCONTROL_AXIS_REGISTERED) != 0 )
    {
        int range = (int)lroundf(stdControl_mouseXRange * 250.0f);
        stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_X, -range, range, 0.0f);
    }

    if ( (stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].flags & STDCONTROL_AXIS_REGISTERED) != 0 )
    {
        int range = (int)lroundf(stdControl_mouseYRange * 200.0f);
        stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Y, -range, range, 0.0f);
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

    // Added: A Null axis state equals its center, so the value is always 0 and the idle
    // flag flip of the DX9 reader can never trigger.
    return 0.0f;
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

    return 0;
}

float J3DAPI stdControl_ReadKeyAsAxis(size_t keyId)
{
    STD_ASSERTREL((keyId < STDCONTROL_MAX_KEYID));
    STD_GUARD(keyId < STDCONTROL_MAX_KEYID, 0.0f);

    if ( !stdControl_bControlsActive )
    {
        return 0.0f;
    }

    if ( !stdControl_aKeyInfo[keyId] )
    {
        return 0.0f;
    }

    // Added: Same idle bookkeeping as stdControlDX9.c: an idle input read clears the flag.
    if ( stdControl_bControlsIdle )
    {
        stdControl_bControlsIdle = false;
    }

    return 0.0f;
}

int J3DAPI stdControl_ReadAxisAsKeyEx(size_t axis, int* pbPressed, float lowValue)
{
    float pos = stdControl_ReadAxis(axis);

    // Added: Same comparison structure as stdControlDX9.c ReadAxisAsKeyEx.
    if ( (axis & (STDCONTROL_AID_LOW_SENSITIVITY | STDCONTROL_AID_POSITIVE_AXIS | STDCONTROL_AID_NEGATIVE_AXIS)) != 0 )
    {
        int axisFlag = (int)(axis & STDCONTROL_AID_NEGATIVE_AXIS);
        if ( axisFlag == (int)STDCONTROL_AID_POSITIVE_AXIS && pos > (double)lowValue )
        {
            if ( pbPressed )
            {
                *pbPressed = 1;
            }
            return 1;
        }

        if ( axisFlag == (int)STDCONTROL_AID_NEGATIVE_AXIS && -lowValue > pos )
        {
            if ( pbPressed )
            {
                *pbPressed = 1;
            }
            return 1;
        }
    }
    else if ( fabsf(pos) > lowValue )
    {
        if ( pbPressed )
        {
            *pbPressed = 1;
        }
        return 1;
    }

    return 0;
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

    return stdControl_ReadAxisAsKeyEx(axis, pbPressed, 0.75f);
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
        // Added: No key is ever pressed on the Null platform, so aKeyDown stays 0;
        // aKeyInfo is read out just like the DX9 reader.
        if ( pbPressed )
        {
            *pbPressed += stdControl_aKeyPressed[keyNum];
        }

        return stdControl_aKeyInfo[keyNum];
    }

    if ( pbPressed )
    {
        *pbPressed = 0;
    }

    return 0;
}

void J3DAPI stdControl_UpdateKeyState(int keyId, int bPressed, unsigned int tickTime)
{
    if ( keyId < 0 || (size_t)keyId >= STDCONTROL_MAX_KEYID )
    {
        return;
    }

    // Added: Same state machine as stdControlDX9.c UpdateKeyState, driven by its callers.
    if ( bPressed && !stdControl_aKeyInfo[keyId] )
    {
        stdControl_aKeyInfo[keyId]      = true;
        stdControl_aKeyIdleTimes[keyId] = (int)(stdControl_curReadTime - tickTime);
        ++stdControl_aKeyPressed[keyId];
    }
    else if ( !bPressed && stdControl_aKeyInfo[keyId] )
    {
        stdControl_aKeyInfo[keyId] = false;
        if ( !stdControl_aKeyIdleTimes[keyId] )
        {
            stdControl_aKeyIdleTimes[keyId] = (int)stdControl_readDeltaTime;
        }

        stdControl_aKeyIdleTimes[keyId] -= (int)(stdControl_curReadTime - tickTime);
    }
}

int J3DAPI stdControl_SetActivation(int bActive)
{
    if ( !stdControl_bOpen )
    {
        return 1;
    }

    for ( size_t keyId = 0; keyId < STDCONTROL_MAX_KEYBOARD_BUTTONS; ++keyId )
    {
        stdControl_UpdateKeyState((int)keyId, 0, stdControl_curReadTime);
    }

    if ( stdControl_bReadMouse )
    {
        for ( size_t keyId = 0; keyId < STDCONTROL_MAX_MOUSE_BUTTONS; ++keyId )
        {
            stdControl_UpdateKeyState(STDCONTROL_KID_MOUSE_LBUTTON + (int)keyId, 0, stdControl_curReadTime);
        }
    }

    // Added: Acquiring the devices always succeeds on the Null platform, so the DX9
    // success convention (0 after the acquire calls) applies.
    stdControl_bControlsActive = bActive != 0;
    return 0;
}

int J3DAPI stdControl_EnableMouse(int bEnable)
{
    if ( bEnable != stdControl_bReadMouse )
    {
        stdControl_bReadMouse = bEnable != 0;
    }

    return 1;
}

bool stdControl_IsMouseEnabled(void)
{
    return stdControl_bReadMouse;
}

void J3DAPI stdControl_EnableMouseSensitivity(int bEnable)
{
    stdControl_bMouseSensitivityEnabled = bEnable != 0;
}

void J3DAPI stdControl_SetMouseSensitivity(float xSensitivity, float ySensitivity)
{
    stdControl_xSensitivity = xSensitivity;
    stdControl_ySensitivity = ySensitivity;
}

void J3DAPI stdControl_ShowMouseCursor(int bShow)
{
    J3D_UNUSED(bShow);
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
        flags = (flags & ~(STDCONTROL_AXIS_POSITIVE | STDCONTROL_AXIS_ENABLED)) | STDCONTROL_AXIS_POSITIVE;
    }

    if ( bNegativeAxis )
    {
        flags = (flags & ~(STDCONTROL_AXIS_NEGATIVE | STDCONTROL_AXIS_ENABLED)) | STDCONTROL_AXIS_NEGATIVE;
    }

    return flags & stdControl_aAxes[aid].flags;
}

void J3DAPI stdControl_SetAxisFlags(size_t axis, StdControlAxisFlag flags)
{
    uint32_t bPositiveAxis = axis & STDCONTROL_AID_POSITIVE_AXIS;
    uint32_t bNegativeAxis = axis & STDCONTROL_AID_NEGATIVE_AXIS;
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

size_t stdControl_GetNumJoysticks(void)
{
    return 0;
}

const char* J3DAPI stdControl_GetJoysticDescription(int joyNum)
{
    J3D_UNUSED(joyNum);
    return NULL;
}

int J3DAPI stdControl_IsGamePad(int joyNum)
{
    J3D_UNUSED(joyNum);
    return 0;
}

void stdControl_TestResetInputState(void)
{
    // Added: Same Test-API invariant as stdControlDX9.c for the in-place regression tests.
    stdControl_bControlsIdle = false;
    memset(stdControl_aKeyInfo, 0, sizeof(stdControl_aKeyInfo));
    memset(stdControl_aKeyIdleTimes, 0, sizeof(stdControl_aKeyIdleTimes));
    memset(stdControl_aKeyPressed, 0, sizeof(stdControl_aKeyPressed));
    memset(stdControl_aAxisStates, 0, sizeof(stdControl_aAxisStates));
}
