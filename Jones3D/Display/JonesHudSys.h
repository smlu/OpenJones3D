#ifndef JONES3D_JONESHUDSYS_H
#define JONES3D_JONESHUDSYS_H

#include <stdbool.h>
#include <std/types.h>

J3D_EXTERN_C_START

// True while the process has not exited, matching the old GetExitCodeProcess check; a stopped
// process makes this as false, which resets the help state in JonesHud.c.
bool JonesHudSys_IsProcessRunning(tSysHandle hProcess); // Implemented in Jones3D/Platform/Win32/JonesHudWin32.c

// Opens the help file named by the SITHSTRING_HELPFILE string relative to the install path and
// returns the viewer process handle, or NULL. The process argument is unused, as in the original.
tSysHandle J3DAPI JonesHud_OpenHelp(tSysHandle process); // Implemented in Jones3D/Platform/Win32/JonesHudWin32.c

J3D_EXTERN_C_END
#endif // JONES3D_JONESHUDSYS_H
