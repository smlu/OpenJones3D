#ifndef SITH_SITHCONTROL_H
#define SITH_SITHCONTROL_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#ifdef J3D_OPENGL
#define SITHCONTROL_MAXFUNCTIONS   59u
#else
#define SITHCONTROL_MAXFUNCTIONS   58u
#endif

#define STDCONTROL_IDLECAM_TIMEOUT 90000 // 90 sec, time in milliseconds before switching to idle camera due to control inactivity

#define sithControl_g_controlOptions J3D_DECL_FAR_VAR(sithControl_g_controlOptions, int)
// extern int sithControl_g_controlOptions;

int J3DAPI sithControl_Startup(int bKeyboardForeground);
void J3DAPI sithControl_Shutdown(void);

void sithControl_DefaultInit(void); // Added

int J3DAPI sithControl_Open(void);
int J3DAPI sithControl_Close(void);
bool sithControl_IsOpen(void); // Added

void J3DAPI sithControl_RegisterAxisFunction(SithControlFunction functionId, SithControlFunctionFlag flag);
void J3DAPI sithControl_RegisterKeyFunction(SithControlFunction functionId);
void J3DAPI sithControl_BindControl(SithControlFunction functionId, size_t controlId, SithControlBindFlag flags);
SithControlBinding* J3DAPI sithControl_BindAxis(SithControlFunction functionId, size_t controlId, SithControlBindFlag flags);

void J3DAPI sithControl_UnbindFunction(SithControlFunction fctnID);
void J3DAPI sithControl_UnbindFunctionIndex(size_t funcId, size_t bindIndex);
void J3DAPI sithControl_UnbindFunctionControl(SithControlFunction fctnID, size_t controlId); // Added
void J3DAPI sithControl_UnbindControl(SithControlBindFlag flags, size_t controlId);

void J3DAPI sithControl_RegisterControlCallback(SithControlCallback pfCallback);
void J3DAPI sithControl_Update(float secDeltaTime, uint32_t msecDeltaTime);

float J3DAPI sithControl_GetKeyAsAxisNormalized(size_t axisId); // Added;
float J3DAPI sithControl_GetKeyAsAxis(SithControlFunction axisId);
float J3DAPI sithControl_GetAxis(SithControlFunction axisId);
int J3DAPI sithControl_GetKey(SithControlFunction keyId, int* pState);

void sithControl_ReadControls(void);
void sithControl_FinishRead(void);
void sithControl_RebindKeyboard(void); // Added

void sithControl_UnbindJoystickAxes(void);
void sithControl_RebindJoystick(void); // Added

void sithControl_UnbindMouseAxes(void);
void sithControl_RebindMouse(void); // Added

void sithControl_Reset(void); // Clears all bindings

void sithControl_RegisterControlFunctions(void);
void sithControl_RegisterKeyboardBindings(void);
void sithControl_RegisterJoystickBindings(void);
void sithControl_RegisterMouseBindings(void); // Added

void sithControl_EnableDevControls(bool bEnable); // Added

// Helper hooking functions
void sithControl_InstallHooks(void);
void sithControl_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCONTROL_H
