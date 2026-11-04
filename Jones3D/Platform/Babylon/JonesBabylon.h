// Added: the Babylon column's windowed run. This is the run logic (JonesBabylon_Run), not the
// entry point: the windowed composition root Platform/Babylon/mainWindowed.cpp calls it after it
// has installed the adapter context with the SDL platform, clock, file system and game window.
// Same header contracts as the headless run.
#ifndef JONES3D_PLATFORM_BABYLON_JONESBABYLON_H
#define JONES3D_PLATFORM_BABYLON_JONESBABYLON_H

#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

// Runs the windowed engine loop until the engine quits, the window closes or a quit is requested.
// Same arguments as main: argv[0] is the program name; a "--load <save.nds>" pair answers the
// Null load-game dialog (needs the load-game start mode in Jones.cfg, like the headless runner);
// every other argument joins with single spaces into the command line of JonesMain_Startup. A
// "--load" without a value, or a joined command line longer than the engine's command line
// buffer, prints one error and returns 1 before the engine starts. Returns the process exit
// code. Call once.
int JonesBabylon_Run(int argc, char** argv);

J3D_EXTERN_C_END
#endif // JONES3D_PLATFORM_BABYLON_JONESBABYLON_H
