#ifndef STD_STDCONTROL_H
#define STD_STDCONTROL_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

#ifdef J3D_DIRECTX6
#include <DirectX6/dinput.h>
#elif defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
#  ifndef DIRECTINPUT_VERSION
#       define DIRECTINPUT_VERSION 0x0800
#  endif
#include <dinput.h>
#endif 

#include <stdbool.h>

J3D_EXTERN_C_START

//----------------------- MOUSE --------------------------
// Mouse axis control ids
#define STDCONTROL_AID_MOUSE_X    STDCONTROL_MAX_JOYSTICK_AXES // 48
#define STDCONTROL_AID_MOUSE_Y    STDCONTROL_AID_MOUSE_X + 1
#define STDCONTROL_AID_MOUSE_Z    STDCONTROL_AID_MOUSE_X + 2

// Button key ids
#define STDCONTROL_KID_MOUSE_LBUTTON   (STDCONTROL_JOYSTICK_FIRSTKID + STDCONTROL_JOYSTICK_TOTALCIDS ) // 640
#define STDCONTROL_KID_MOUSE_RBUTTON   STDCONTROL_KID_MOUSE_LBUTTON + 1
#define STDCONTROL_KID_MOUSE_MBUTTON   STDCONTROL_KID_MOUSE_LBUTTON + 2
#define STDCONTROL_KID_MOUSE_XBUTTON   STDCONTROL_KID_MOUSE_LBUTTON + 3

// ------------------------ AXES ---------------------------
// Axis aid flags
#define STDCONTROL_AID_LOW_SENSITIVITY 0x20000000
#define STDCONTROL_AID_POSITIVE_AXIS   0x80000000
#define STDCONTROL_AID_NEGATIVE_AXIS   0xC0000000

#define STDCONTROL_GETAID(axis) (axis & ~(STDCONTROL_AID_POSITIVE_AXIS | STDCONTROL_AID_NEGATIVE_AXIS | STDCONTROL_AID_LOW_SENSITIVITY))

// ------------------------- JOYSTICK -----------------------
#define STDCONTROL_JOYSTICK_FIRSTCID   STDCONTROL_MAX_KEYBOARD_BUTTONS 

#define STDCONTROL_JOYSTICK_FIRSTKID   STDCONTROL_JOYSTICK_FIRSTCID // First button CID
#define STDCONTROL_JOYSTICK_LASTKID    STDCONTROL_JOYSTICK_GETBUTTON(STDCONTROL_MAX_JOYSTICK_DEVICES - 1, STDCONTROL_NUM_JOYSTICK_BUTTONS -1) // Last button CID

#define STDCONTROL_JOYSTICK_FIRTPOVCID  STDCONTROL_JOYSTICK_FIRSTKID + STDCONTROL_NUM_JOYSTICK_BUTTONS // First POV controller direction CID
#define STDCONTROL_JOYSTICK_LASTPOVCID  STDCONTROL_JOYSTICK_GETPOV(STDCONTROL_MAX_JOYSTICK_DEVICES - 1, STDCONTROL_MAX_JOYSTICK_POVCONTROLERS -1, STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS - 1)

#define STDCONTROL_JOYSTICK_TOTALCIDS   (STDCONTROL_MAX_JOYSTICK_DEVICES * STDCONTROL_TOTAL_JOYSTICK_CONTROLS)

// Joystick button ID or POV controller direction control ID
#define STDCONTROL_JOYSTICK_GETBUTTON(joyNum, btnNum)  ((STDCONTROL_TOTAL_JOYSTICK_CONTROLS) * (joyNum) + (btnNum) + STDCONTROL_JOYSTICK_FIRSTKID)
#define STDCONTROL_JOYSTICK_GETBUTTONINDEX(cid)        ((cid - STDCONTROL_JOYSTICK_FIRSTKID) % STDCONTROL_TOTAL_JOYSTICK_CONTROLS ) // Get button index number from CID

#define STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, dirNum) ((STDCONTROL_TOTAL_JOYSTICK_CONTROLS) * (joyNum) + ((povNum) * STDCONTROL_MAX_JOYSTICK_POVCONTROLERS) + (STDCONTROL_JOYSTICK_FIRTPOVCID + (dirNum)))
#define STDCONTROL_JOYSTICK_GETPOVINDEX(cid)               ((cid - STDCONTROL_JOYSTICK_FIRSTCID) % STDCONTROL_TOTAL_JOYSTICK_CONTROLS % STDCONTROL_NUM_JOYSTICK_BUTTONS / STDCONTROL_MAX_JOYSTICK_POVCONTROLERS) // Get POV controller index number from CID, i.e. control num
#define STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(cid)      ((cid - STDCONTROL_JOYSTICK_FIRSTCID) % STDCONTROL_NUM_JOYSTICK_BUTTONS % STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS) // Get POV direction index number from CID

// Joystick macro for getting control ID (CID) for POV direction button 
#define STDCONTROL_JOYSTICK_GETPOVLEFT(joyNum, povNum)  STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, STDCONTROL_JOYSTICK_POVDIRLEFT)
#define STDCONTROL_JOYSTICK_GETPOVUP(joyNum, povNum)    STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, STDCONTROL_JOYSTICK_POVDIRUP)
#define STDCONTROL_JOYSTICK_GETPOVRIGHT(joyNum, povNum) STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, STDCONTROL_JOYSTICK_POVDIRRIGHT)
#define STDCONTROL_JOYSTICK_GETPOVDOWN(joyNum, povNum)  STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, STDCONTROL_JOYSTICK_POVDIRDOWN)

// Joystick POV direction indices
#define STDCONTROL_JOYSTICK_POVDIRLEFT  0
#define STDCONTROL_JOYSTICK_POVDIRUP    1
#define STDCONTROL_JOYSTICK_POVDIRRIGHT 2
#define STDCONTROL_JOYSTICK_POVDIRDOWN  3

// Joystick/controller axes related macros
#define STDCONTROL_JOYSTICK_NUMPOSAXES  3u // number of positional axes
#define STDCONTROL_JOYSTICK_NUMROTAXES  3u // number of rotation axes
#define STDCONTROL_JOYSTICK_NUMAXES     (STDCONTROL_JOYSTICK_NUMPOSAXES + STDCONTROL_JOYSTICK_NUMROTAXES) // 6

// Joystick positional & rotation axis indices
// Note, in directX 6 thes constants are defined in dinput.h as
// DIJOFS_X, DIJOFS_Y, DIJOFS_Z, DIJOFS_RX, DIJOFS_RY, DIJOFS_RZ
#define STDCONTROL_JOYSTICK_AXIS_X   0u
#define STDCONTROL_JOYSTICK_AXIS_Y   1u
#define STDCONTROL_JOYSTICK_AXIS_Z   2u
#define STDCONTROL_JOYSTICK_AXIS_RX  3u
#define STDCONTROL_JOYSTICK_AXIS_RY  4u
#define STDCONTROL_JOYSTICK_AXIS_RZ  5u

#define STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axisIdx) (STDCONTROL_JOYSTICK_NUMAXES * joyNum + axisIdx)
#define STDCONTROL_GET_JOYSTICK_AXIS_X(joyNum)        STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_X)
#define STDCONTROL_GET_JOYSTICK_AXIS_Y(joyNum)        STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_Y)
#define STDCONTROL_GET_JOYSTICK_AXIS_Z(joyNum)        STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_Z)
#define STDCONTROL_GET_JOYSTICK_AXIS_RX(joyNum)       STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_RX)
#define STDCONTROL_GET_JOYSTICK_AXIS_RY(joyNum)       STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_RY)
#define STDCONTROL_GET_JOYSTICK_AXIS_RZ(joyNum)       STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_RZ)

#define STDCONTROL_MAX_JOYSTICK_DEVICES       (8u + STDCONTROL_MAX_GAMEPAD_DEVICES) // TODO: When gamepad has dedicated slots assigned reduce to 8u
#define STDCONTROL_MAX_JOYSTICK_POVCONTROLERS 4u  // max number of point-of-view controllers e.g.: Hat switch
#define STDCONTROL_MAX_JOYSTICK_AXES          (STDCONTROL_MAX_JOYSTICK_DEVICES * STDCONTROL_JOYSTICK_NUMAXES) // 48, total number of axes from all devices

// ------------------------- GAMEPAD -----------------------
// Extra slots for xbox gamepads
#if defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
#define STDCONTROL_MAX_GAMEPAD_DEVICES       4u
#else
#define STDCONTROL_MAX_GAMEPAD_DEVICES       0u
#endif

// ------------------------- MISC CONSTANTS -----------------------
#define STDCONTROL_MAX_AXES                   (STDCONTROL_MAX_JOYSTICK_AXES + 3) // 51; 3 - mouse axes 
#define STDCONTROL_MAX_KEYBOARD_BUTTONS       256u // aka max keyboard kids
#define STDCONTROL_MAX_MOUSE_BUTTONS            4u
#define STDCONTROL_NUM_JOYSTICK_BUTTONS        32u // number of joy buttons per device
#define STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS   4u // number of directions per point-of-view controller
#define STDCONTROL_TOTAL_JOYSTICK_CONTROLS     (STDCONTROL_NUM_JOYSTICK_BUTTONS + (STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS * STDCONTROL_MAX_JOYSTICK_POVCONTROLERS))// max number of joy buttons + POV controllers directions per device
#define STDCONTROL_MAX_KEYID                   (STDCONTROL_MAX_KEYBOARD_BUTTONS + STDCONTROL_JOYSTICK_TOTALCIDS + STDCONTROL_MAX_MOUSE_BUTTONS) // 644

// ------------------------- HELPER MACROS -----------------------
// Helper macros to test if specific keyid or aid is either button or axis of specific device

#define STDCONTROL_ISKEYBOARDBUTTON(kid) (kid < STDCONTROL_MAX_KEYBOARD_BUTTONS)

#define STDCONTROL_ISMOUSEAXIS(aid) (STDCONTROL_GETAID(aid) >= STDCONTROL_AID_MOUSE_X && STDCONTROL_GETAID(aid) <= STDCONTROL_AID_MOUSE_Z )
#define STDCONTROL_ISMOUSEBUTTON(kid) (kid >= STDCONTROL_KID_MOUSE_LBUTTON && kid <= STDCONTROL_KID_MOUSE_XBUTTON)

// Check for joystick controller ids
#define STDCONTROL_ISJOYSTICKAXIS(aid) ((STDCONTROL_GETAID(aid) < STDCONTROL_AID_MOUSE_X))
#define STDCONTROL_ISJOYSTICKBUTTON(kid) ((STDCONTROL_GETAID(kid) >= STDCONTROL_JOYSTICK_FIRSTKID) && (STDCONTROL_GETAID(kid) <= STDCONTROL_JOYSTICK_LASTKID))
#define STDCONTROL_ISJOYSTICKPOV(cid) ((STDCONTROL_GETAID(cid) >= STDCONTROL_JOYSTICK_FIRTPOVCID) && (STDCONTROL_GETAID(cid) <= STDCONTROL_JOYSTICK_LASTPOVCID))

// In-place regression tests
static_assert(STDCONTROL_JOYSTICK_NUMAXES == 6, "STDCONTROL_JOYSTICK_NUM_AXIS == 6");
static_assert(STDCONTROL_JOYSTICK_FIRSTCID == 256, "STDCONTROL_JOYSTICK_FIRSTCID == 256");

#if defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
static_assert(STDCONTROL_JOYSTICK_TOTALCIDS == 576, "STDCONTROL_JOYSTICK_TOTALCIDS == 576");
static_assert(STDCONTROL_KID_MOUSE_LBUTTON == 832, "STDCONTROL_AID_MOUSE_LBUTTON == 832");
static_assert(STDCONTROL_KID_MOUSE_RBUTTON == 833, "STDCONTROL_KID_MOUSE_RBUTTON == 833");
static_assert(STDCONTROL_KID_MOUSE_MBUTTON == 834, "STDCONTROL_KID_MOUSE_MBUTTON == 834");
static_assert(STDCONTROL_KID_MOUSE_XBUTTON == 835, "STDCONTROL_KID_MOUSE_XBUTTON == 835");
#else
static_assert(STDCONTROL_JOYSTICK_TOTALCIDS == 384, "STDCONTROL_JOYSTICK_TOTALCIDS == 384");
static_assert(STDCONTROL_KID_MOUSE_LBUTTON == 640, "STDCONTROL_AID_MOUSE_LBUTTON == 640");
static_assert(STDCONTROL_KID_MOUSE_RBUTTON == 641, "STDCONTROL_KID_MOUSE_RBUTTON == 641");
static_assert(STDCONTROL_KID_MOUSE_MBUTTON == 642, "STDCONTROL_KID_MOUSE_MBUTTON == 642");
static_assert(STDCONTROL_KID_MOUSE_XBUTTON == 643, "STDCONTROL_KID_MOUSE_XBUTTON == 643");
#endif
static_assert(STDCONTROL_NUM_JOYSTICK_BUTTONS == 32, "STDCONTROL_NUM_JOYSTICK_BUTTONS == 32");
static_assert(STDCONTROL_MAX_JOYSTICK_POVCONTROLERS == 4, "STDCONTROL_MAX_JOYSTICK_POVCONTROLERS == 4");
static_assert(STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS == 4, "STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS == 4");
static_assert(STDCONTROL_TOTAL_JOYSTICK_CONTROLS == 48, "STDCONTROL_TOTAL_JOYSTICK_CONTROLS == 48");

#if defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
static_assert(STDCONTROL_MAX_KEYID == 836, "STDCONTROL_MAX_KEYID == 836");
static_assert(STDCONTROL_MAX_JOYSTICK_AXES == 72, "STDCONTROL_MAX_JOYSTICK_AXES == 72");
static_assert(STDCONTROL_MAX_AXES == 75, "STDCONTROL_MAX_AXES == 75");
static_assert(STDCONTROL_AID_MOUSE_X == 72, "STDCONTROL_AID_MOUSE_X == 72");
#else
static_assert(STDCONTROL_MAX_KEYID == 644, "STDCONTROL_MAX_KEYID == 644");
static_assert(STDCONTROL_MAX_JOYSTICK_AXES == 48, "STDCONTROL_MAX_JOYSTICK_AXES == 48");
static_assert(STDCONTROL_MAX_AXES == 51, "STDCONTROL_MAX_AXES == 51");
static_assert(STDCONTROL_AID_MOUSE_X == 48, "STDCONTROL_AID_MOUSE_X == 48");
#endif

static_assert(STDCONTROL_JOYSTICK_FIRSTKID == 256, "STDCONTROL_JOYSTICK_FIRSTKID == 256");

#if defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
static_assert(STDCONTROL_JOYSTICK_LASTKID == 815, "STDCONTROL_JOYSTICK_LASTKID == 815");
#else
static_assert(STDCONTROL_JOYSTICK_LASTKID == 623, "STDCONTROL_JOYSTICK_LASTKID == 623");
#endif

static_assert(STDCONTROL_JOYSTICK_GETBUTTON(0, 0) == 256, "STDCONTROL_JOYSTICK_GETBUTTON(0, 0) == 256");
static_assert(STDCONTROL_JOYSTICK_GETBUTTON(0, 1) == 257, "STDCONTROL_JOYSTICK_GETBUTTON(0, 0) == 257");
static_assert(STDCONTROL_JOYSTICK_GETBUTTON(0, 1) == 257, "STDCONTROL_JOYSTICK_GETBUTTON(0, 0) == 257");
static_assert(STDCONTROL_JOYSTICK_GETBUTTON(1, 0) == 304, "STDCONTROL_JOYSTICK_GETBUTTON(1, 0) == 304");
static_assert(STDCONTROL_JOYSTICK_GETBUTTON(1, 1) == 305, "STDCONTROL_JOYSTICK_GETBUTTON(1, 1) == 305");
static_assert(STDCONTROL_JOYSTICK_GETBUTTON(7, 31) == 623, "STDCONTROL_JOYSTICK_GETBUTTON(7, 31) == 623");
static_assert(STDCONTROL_ISJOYSTICKBUTTON(256) == true, "STDCONTROL_ISJOYSTICKBUTTON(256) == true");
static_assert(STDCONTROL_ISJOYSTICKBUTTON(623) == true, "STDCONTROL_ISJOYSTICKBUTTON(623) == true");
static_assert(STDCONTROL_ISJOYSTICKBUTTON(0) == false, "STDCONTROL_ISJOYSTICKBUTTON(0) == false");
static_assert(STDCONTROL_ISJOYSTICKBUTTON(255) == false, "STDCONTROL_ISJOYSTICKBUTTON(255) == false");

#if defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
static_assert(STDCONTROL_ISJOYSTICKBUTTON(816) == false, "STDCONTROL_ISJOYSTICKBUTTON(816) == false"); // One past last button
static_assert(STDCONTROL_ISJOYSTICKBUTTON(831) == false, "STDCONTROL_ISJOYSTICKBUTTON(831) == false"); // POV button
#else
static_assert(STDCONTROL_ISJOYSTICKBUTTON(624) == false, "STDCONTROL_ISJOYSTICKBUTTON(624) == false"); // One past last button
static_assert(STDCONTROL_ISJOYSTICKBUTTON(639) == false, "STDCONTROL_ISJOYSTICKBUTTON(639) == false"); // POV button
#endif

static_assert(STDCONTROL_MAX_JOYSTICK_POVCONTROLERS == 4, "STDCONTROL_MAX_JOYSTICK_POVCONTROLERS == 4");
static_assert(STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS == 4, "STDCONTROL_NUM_JOYSTICK_POVBUTTONS == 4");
static_assert(STDCONTROL_JOYSTICK_FIRTPOVCID == 288, "STDCONTROL_JOYSTICK_FIRTPOVKID == 288");

#if defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
static_assert(STDCONTROL_JOYSTICK_LASTPOVCID == 831, "STDCONTROL_JOYSTICK_LASTPOVCID == 831");
#else
static_assert(STDCONTROL_JOYSTICK_LASTPOVCID == 639, "STDCONTROL_JOYSTICK_LASTPOVCID == 639");
#endif

static_assert(STDCONTROL_JOYSTICK_GETPOV(0, 0, 0) == 288, "STDCONTROL_JOYSTICK_GETPOVBUTTON(0, 0, 0) == 288");
static_assert(STDCONTROL_JOYSTICK_GETPOVLEFT(0, 0) == 288, "STDCONTROL_JOYSTICK_GETPOVLEFT(0, 0) == 288");
static_assert(STDCONTROL_JOYSTICK_GETPOV(0, 0, 1) == 289, "STDCONTROL_JOYSTICK_GETPOVBUTTON(0, 0, 1) == 289");
static_assert(STDCONTROL_JOYSTICK_GETPOVUP(0, 0) == 289, "STDCONTROL_JOYSTICK_GETPOVUP(0, 0) == 289");
static_assert(STDCONTROL_JOYSTICK_GETPOV(0, 0, 2) == 290, "STDCONTROL_JOYSTICK_GETPOVBUTTON(0, 0, 2) == 290");
static_assert(STDCONTROL_JOYSTICK_GETPOVRIGHT(0, 0) == 290, "STDCONTROL_JOYSTICK_GETPOVRIGHT(0, 0) == 290");
static_assert(STDCONTROL_JOYSTICK_GETPOV(0, 0, 3) == 291, "STDCONTROL_JOYSTICK_GETPOVBUTTON(0, 0, 3) == 291");
static_assert(STDCONTROL_JOYSTICK_GETPOVDOWN(0, 0) == 291, "STDCONTROL_JOYSTICK_GETPOVDOWN(0, 0) == 291");
static_assert(STDCONTROL_JOYSTICK_GETPOV(0, 1, 0) == 292, "STDCONTROL_JOYSTICK_GETPOVBUTTON(0, 1, 0) == 292");
static_assert(STDCONTROL_JOYSTICK_GETPOVLEFT(0, 1) == 292, "STDCONTROL_JOYSTICK_GETPOVLEFT(0, 1) == 292");
static_assert(STDCONTROL_JOYSTICK_GETPOV(0, 1, 1) == 293, "STDCONTROL_JOYSTICK_GETPOVBUTTON(0, 1, 1) == 293");
static_assert(STDCONTROL_JOYSTICK_GETPOVUP(0, 1) == 293, "STDCONTROL_JOYSTICK_GETPOVUP(0, 1) == 293");
static_assert(STDCONTROL_JOYSTICK_GETPOV(1, 0, 0) == 336, "STDCONTROL_JOYSTICK_GETPOVBUTTON(1, 0, 0) == 336");
static_assert(STDCONTROL_JOYSTICK_GETPOVLEFT(1, 0) == 336, "STDCONTROL_JOYSTICK_GETPOVLEFT(1, 0) == 336");
static_assert(STDCONTROL_JOYSTICK_GETPOV(1, 0, 1) == 337, "STDCONTROL_JOYSTICK_GETPOVBUTTON(1, 0, 1) == 337");
static_assert(STDCONTROL_JOYSTICK_GETPOVUP(1, 0) == 337, "STDCONTROL_JOYSTICK_GETPOVUP(1, 0) == 337");
static_assert(STDCONTROL_JOYSTICK_GETPOV(1, 1, 0) == 340, "STDCONTROL_JOYSTICK_GETPOVBUTTON(1, 1, 0) == 340");
static_assert(STDCONTROL_JOYSTICK_GETPOVLEFT(1, 1) == 340, "STDCONTROL_JOYSTICK_GETPOVLEFT(1, 1) == 340");
static_assert(STDCONTROL_JOYSTICK_GETPOV(1, 1, 1) == 341, "STDCONTROL_JOYSTICK_GETPOVBUTTON(1, 1, 1) == 341");
static_assert(STDCONTROL_JOYSTICK_GETPOVUP(1, 1) == 341, "STDCONTROL_JOYSTICK_GETPOVUP(1, 1) == 341");
static_assert(STDCONTROL_JOYSTICK_GETPOV(7, 3, 3) == 639, "STDCONTROL_JOYSTICK_GETPOVBUTTON(7, 3, 3) == 639");
static_assert(STDCONTROL_JOYSTICK_GETPOVDOWN(7, 3) == 639, "STDCONTROL_JOYSTICK_GETPOVDOWN(7, 3) == 639");

static_assert(STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(288) == 0, "STDCONTROL_GETPOVDIRECTIONINDEX(288) == 0");
static_assert(STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(287) == 3, "STDCONTROL_GETPOVDIRECTIONINDEX(287) == 3");
static_assert(STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(639) == 3, "STDCONTROL_GETPOVDIRECTIONINDEX(639) == 3");
static_assert(STDCONTROL_ISJOYSTICKPOV(288) == true, "STDCONTROL_ISJOYSTICKPOV(288) == true");
static_assert(STDCONTROL_ISJOYSTICKPOV(639) == true, "STDCONTROL_ISJOYSTICKPOV(639) == true");
static_assert(STDCONTROL_ISJOYSTICKPOV(0) == false, "STDCONTROL_ISJOYSTICKPOV(0) == false");
static_assert(STDCONTROL_ISJOYSTICKPOV(0) == false, "STDCONTROL_ISJOYSTICKPOV(0) == false");
static_assert(STDCONTROL_ISJOYSTICKPOV(287) == false, "STDCONTROL_ISJOYSTICKPOV(287) == false");

#if defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
static_assert(STDCONTROL_ISJOYSTICKPOV(832) == false, "STDCONTROL_ISJOYSTICKPOV(832) == false");
#elif
static_assert(STDCONTROL_ISJOYSTICKPOV(640) == false, "STDCONTROL_ISJOYSTICKPOV(640) == false");
#endif

int J3DAPI stdControl_Startup(int bKeyboardForeground);
bool stdControl_HasStarted(void);
void stdControl_Shutdown(void);

int stdControl_Open(void);
bool stdControl_IsOpen(void);
void stdControl_Close(void);
void stdControl_Reset(void);

void stdControl_DisableReadJoysticks(void);
int J3DAPI stdControl_EnableAxis(int axisID);

void stdControl_ReadControls(void);
void stdControl_FinishRead(void); // Added

float J3DAPI stdControl_ReadAxis(size_t axis);
int J3DAPI stdControl_ReadAxisRaw(size_t axis);
float J3DAPI stdControl_ReadKeyAsAxis(size_t keyId);

/**
 * @brief Emulates a key press from a joystick/gamepad axis.
 *
 * Returns non-zero if the axis is "pressed". If pbPressed is not NULL,
 * it is set to non-zero if the axis was just pressed - always true if non-zero value is returned.
 *
 * @param axis - The axis index.
 * @param pbPressed - Pointer to an int for the "just pressed" state.
 * @return A non-zero value if the key is currently pressed down; zero otherwise.
 */
int J3DAPI stdControl_ReadAxisAsKey(size_t axis, int* pbPressed);

/**
 * @brief Emulates a key press from a joystick/gamepad axis with a threshold.
 *
 * Returns non-zero if the axis value exceeds the specified threshold. If pbPressed is not NULL,
 * it is set to non-zero if the axis was just pressed - always true if non-zero value is returned.
 *
 * @param axis - The axis index.
 * @param pbPressed - Pointer to an int for the "just pressed" state.
 * @param lowValue - The threshold value for detecting a press.
 * @return A non-zero value if the key is currently pressed down; zero otherwise.
 */
int J3DAPI stdControl_ReadAxisAsKeyEx(size_t axis, int* pbPressed, float lowValue);

/**
 * @brief Reads the current and just pressed state of a specified key.
 *
 * Returns non-zero if the key is currently pressed. If pbPressed is not NULL,
 * it is set to non-zero if the key was just pressed.
 *
 * @param keyNum - The key identifier.
 * @param pbPressed - Pointer to an int for the "just pressed" state.
 *
 * @return A non-zero value if the key is currently pressed down; zero otherwise.
 */
int J3DAPI stdControl_ReadKey(size_t keyNum, int* pbPressed);

int stdControl_ControlsActive(void);
int J3DAPI stdControl_SetActivation(int bActive); // returns 0 if mouse control was successfully processed 

int stdControl_ToggleMouse(void);
int J3DAPI stdControl_EnableMouse(int bEnable);
bool stdControl_IsMouseEnabled(void); // Added

void J3DAPI stdControl_RegisterMouseAxesXY(float xrange, float yrange); // Added
void stdControl_ResetMousePos(void);
int stdControl_MouseSensitivityEnabled(void); // Added: From debug version
void J3DAPI stdControl_EnableMouseSensitivity(int bEnable);
void J3DAPI stdControl_SetMouseSensitivity(float xSensitivity, float ySensitivity); // Added
void J3DAPI stdControl_ShowMouseCursor(int bShow);

int stdControl_ControlsIdle(void);

int J3DAPI stdControl_TestAxisFlag(size_t axis, StdControlAxisFlag flags);
void J3DAPI stdControl_SetAxisFlags(size_t axis, StdControlAxisFlag flags);

void J3DAPI stdControl_RegisterAxis(size_t aid, int min, int max, float deadzoneScale);
size_t stdControl_GetMaxJoystickButtons(void);
size_t stdControl_GetNumJoysticks(void);
const char* J3DAPI stdControl_GetJoysticDescription(int joyNum); // Added: From debug version

int J3DAPI stdControl_IsGamePad(int joyNum);

// Helper hooking functions
void stdControl_InstallHooks(void);
void stdControl_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDCONTROL_H
