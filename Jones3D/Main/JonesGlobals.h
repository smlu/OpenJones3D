#ifndef JONES3D_JONESGLOBALS_H
#define JONES3D_JONESGLOBALS_H
#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

// Added: Shared retail-global reset sequence, moved out of the ResetGlobals body in
// Jones3D/Platform/Win32/WinMain.c so the native Null platform can run it too.
//
// The original WinMain.c order interleaves modules that only exist on Windows:
// SoundDriver, JonesDialog, jonesConfig, stdComm, stdConsole, stdControl, stdWin95,
// wkernel and wuRegistry. Those resets stay in WinMain.c, which calls the runs below
// around them so the Windows order is unchanged.
// Jones3D_ResetAllGlobals runs every part in the original order; on the Null platform
// the Windows-only modules have no compiled globals, so it is the complete reset.

// Resets Jones3D_ResetAllGlobals(void) calls, in engine order:

// AudioLib; runs before the Windows-only SoundDriver reset.
void Jones3D_ResetGlobalsBeforeSoundDriver(void);

// JonesConsole, JonesControl; runs before the Windows-only JonesDialog reset.
void Jones3D_ResetGlobalsBeforeJonesDialog(void);

// JonesDisplay, JonesFile, JonesHud, JonesMain, Smush, Sound, jonesCog; runs before
// the Windows-only jonesConfig reset.
void Jones3D_ResetGlobalsBeforeJonesConfig(void);

// jonesInventory, jonesString, rdroid, sith and the std General/Null front; runs
// before the Windows-only stdComm reset.
void Jones3D_ResetGlobalsBeforeStdComm(void);

// stdConffile; runs before the Windows-only stdConsole and stdControl resets.
void Jones3D_ResetGlobalsBeforeStdConsole(void);

// stdDisplay, stdEffect, stdFileUtil, stdFnames, stdGob, stdHashtbl, stdLinkList,
// stdMath, stdMemory, stdPlatform, stdStrTable, stdUtil; runs before the Windows-only
// stdWin95 reset.
void Jones3D_ResetGlobalsBeforeStdWin95(void);

// std; runs before the Windows-only wkernel and wuRegistry resets.
void Jones3D_ResetGlobalsBeforeWkernel(void);

// The whole cross-platform reset sequence, every run above in order.
void Jones3D_ResetAllGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESGLOBALS_H
