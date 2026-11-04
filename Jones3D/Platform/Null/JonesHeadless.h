// Added: the headless run shared by the Null and Babylon platform columns. The entry points
// (main.c for the Null column, Platform/Babylon/main.cpp for the Babylon column) call this.
#ifndef JONES3D_PLATFORM_NULL_JONESHEADLESS_H
#define JONES3D_PLATFORM_NULL_JONESHEADLESS_H

#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

// Runs the headless engine loop. Same arguments as main; returns the process exit code. Call once.
int JonesHeadless_Run(int argc, char** argv);

J3D_EXTERN_C_END
#endif // JONES3D_PLATFORM_NULL_JONESHEADLESS_H
