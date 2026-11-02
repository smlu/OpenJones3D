#ifndef JONES3D_PLATFORM_NULL_JONESPLATFORMNULL_H
#define JONES3D_PLATFORM_NULL_JONESPLATFORMNULL_H
#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

// Added: Null-platform-only control for jonesConfig_GetLoadGameFilePath. On Windows that
// function runs the load-game OpenFileName dialog; there is no dialog here, so the
// headless runner hands the chosen .nds path over before startup instead.
//
// When a path is set, jonesConfig_GetLoadGameFilePath copies it into pDestNdsPath the way
// the Windows dialog fills it (forcing the .nds extension) and returns 1 (the dialog's
// success value), then clears the path so a one-shot request is not reused. With no path
// set the dialog keeps answering the cancel value. The stored pointer is not copied: it
// must stay valid until the dialog is answered or the path is set again.
void JonesPlatformNull_SetLoadPath(const char* pPath);

J3D_EXTERN_C_END
#endif // JONES3D_PLATFORM_NULL_JONESPLATFORMNULL_H
