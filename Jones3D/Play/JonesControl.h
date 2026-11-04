#ifndef JONES3D_JONESCONTROL_H
#define JONES3D_JONESCONTROL_H
#include <Jones3D/types.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define JONESCONTROL_CFG_CONTROLS_MOUSE      "controls.mouse"
#define JONESCONTROL_CFG_CONTROLS_CONTROLLER "controls.controller"

int JonesControl_Startup(void);
void JonesControl_Shutdown(void);

int J3DAPI JonesControl_ProcessControls(SithThing* pPlayer, float secDeltaTime);
void JonesControl_EnableJoystickAxes(void);

void JonesControl_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESCONTROL_H
