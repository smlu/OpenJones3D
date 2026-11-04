// Added: the Null column's device side of stdControlNull.h. The only "device" behind this column is
// the scripted key state the headless runner's --input leaves here, and the reads hand it over in
// the DX9 order: stdControl_ReadDevices reports it as one snapshot, the way stdControlDX9.c's
// stdControl_ApplyKeyboardState and stdControl_ApplyMouseButtonState hand over the polled device
// state. That is what both platform columns agree on: applying a scripted key before the read, as
// this file used to do, lost its press count to the per-read clear in stdControl_ReadControls.
#include <j3dcore/j3d.h>

#include <std/Null/stdControlNull.h>
#include <std/Win95/stdControl.h>

// The scripted down state: one flag per engine key id, the STDCONTROL_MAX_KEYBOARD_BUTTONS keyboard
// ids at their own index and the STDCONTROL_MAX_MOUSE_BUTTONS mouse buttons (engine ids
// STDCONTROL_KID_MOUSE_LBUTTON + 0..3) behind them. All 0 at the start: no scripted key is down.
static int stdControlDevicesNull_aKeyDown[STDCONTROL_MAX_KEYBOARD_BUTTONS + STDCONTROL_MAX_MOUSE_BUTTONS] = { 0 };

// The scripted down state of one engine key id, or NULL when the id is not one of the keys this
// column can deliver.
static int* stdControlDevicesNull_KeyDown(int keyId)
{
    if ( keyId >= 0 && (size_t)keyId < STDCONTROL_MAX_KEYBOARD_BUTTONS )
    {
        return &stdControlDevicesNull_aKeyDown[keyId];
    }

    if ( STDCONTROL_ISMOUSEBUTTON((size_t)keyId) )
    {
        return &stdControlDevicesNull_aKeyDown[STDCONTROL_MAX_KEYBOARD_BUTTONS
            + ((size_t)keyId - STDCONTROL_KID_MOUSE_LBUTTON)];
    }

    return NULL;
}

void stdControl_ReadDevices(uint32_t readTime, bool bReadMouse)
{
    // Added: The keyboard snapshot, stdControlDX9.c's stdControl_ApplyKeyboardState over the whole
    // keyboard range: every id is reported as it is now, after the per-read state was cleared.
    for ( size_t keyId = 0; keyId < STDCONTROL_MAX_KEYBOARD_BUTTONS; ++keyId )
    {
        stdControl_UpdateKeyState((int)keyId, stdControlDevicesNull_aKeyDown[keyId], readTime);
    }

    // Added: The mouse button snapshot, stdControlDX9.c's stdControl_ApplyMouseButtonState, only for
    // the four buttons the engine has ids for and only while the mouse is enabled.
    if ( bReadMouse )
    {
        for ( size_t button = 0; button < STDCONTROL_MAX_MOUSE_BUTTONS; ++button )
        {
            stdControl_UpdateKeyState(STDCONTROL_KID_MOUSE_LBUTTON + (int)button,
                stdControlDevicesNull_aKeyDown[STDCONTROL_MAX_KEYBOARD_BUTTONS + button], readTime);
        }
    }
}

bool stdControl_QueueKeyEvent(int keyId, int bPressed)
{
    // Added: The scripted key is stored, not applied: it takes effect at the next
    // stdControl_ReadControls, exactly as in the Babylon column.
    int* pKeyDown = stdControlDevicesNull_KeyDown(keyId);
    if ( !pKeyDown )
    {
        return false;
    }

    *pKeyDown = bPressed != 0;
    return true;
}
