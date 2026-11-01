#ifndef STD_STDCONTROLTESTAPI_H
#define STD_STDCONTROLTESTAPI_H

#if defined(J3D_TEST)

#include <std/Win95/stdControl.h>

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#if defined(J3D_DIRECTX9)
#include <xinput.h>
#endif

void stdControl_TestResetInputState(void);
void stdControl_TestBeginRead(uint32_t currentReadTime, uint32_t lastReadTime);
void stdControl_TestSetControlsActive(bool bActive);
void stdControl_TestSetAxisState(size_t axis, int value);
int stdControl_TestGetAxisState(size_t axis);
bool stdControl_TestGetAxis(size_t axis, StdControlAxis* pAxis);
uint32_t stdControl_TestGetReadDeltaTime(void);
float stdControl_TestGetSecFPS(void);
void stdControl_TestApplyKeyboardState(const uint8_t* aKeyState, size_t numKeys);
void stdControl_TestApplyJoystickState(size_t joyNum, const DIJOYSTATE* pState, size_t numPovs);
void stdControl_TestApplyMouseState(const DIMOUSESTATE* pState, bool bApplyButtons);

#if defined(J3D_DIRECTX9)
bool stdControl_TestIsXInputJoystickIndex(int joyNum, size_t numDirectInputDevices);
bool stdControl_TestRegisterXInputDevice(DWORD userIndex, const XINPUT_STATE* pState, const XINPUT_CAPABILITIES* pCaps);
bool stdControl_TestApplyXInputState(size_t deviceIndex, const XINPUT_STATE* pState);
bool stdControl_TestDisconnectXInputDevice(size_t deviceIndex);
bool stdControl_TestIsXInputDeviceConnected(size_t deviceIndex);
#endif

#endif // J3D_TEST

#endif // STD_STDCONTROLTESTAPI_H
