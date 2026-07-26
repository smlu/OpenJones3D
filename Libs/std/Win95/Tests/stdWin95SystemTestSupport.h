#ifndef STD_WIN95_SYSTEM_TEST_SUPPORT_H
#define STD_WIN95_SYSTEM_TEST_SUPPORT_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

void StdWin95SystemTest_Startup(void);
void StdWin95SystemTest_Shutdown(void);

HWND StdWin95SystemTest_GetWindow(void);
HINSTANCE StdWin95SystemTest_GetInstance(void);

void StdWin95SystemTest_PumpMessages(void);
void StdWin95SystemTest_SizeWindowClient(uint32_t width, uint32_t height);
void StdWin95SystemTest_PresentVisualFrame(void);
void StdWin95SystemTest_RequireHiddenWindow(void);
void StdWin95SystemTest_RequireForegroundWindow(void);
void StdWin95SystemTest_RequireDisplayConfig(void);
void StdWin95SystemTest_RequireControlStartup(void);
void StdWin95SystemTest_RequireDisplayStartup(void);
void StdWin95SystemTest_RequireDisplayDeviceOpen(void);
void StdWin95SystemTest_RequireWindowedDisplayModeAtResolution(uint32_t width, uint32_t height);
void StdWin95SystemTest_RequireWindowedDisplayMode(void);

void StdWin95SystemTest_SetControlStarted(bool bStarted);
void StdWin95SystemTest_SetControlOpen(bool bOpen);
void StdWin95SystemTest_SetDisplayStarted(bool bStarted);
void StdWin95SystemTest_SetDisplayOpen(bool bOpen);
void StdWin95SystemTest_SetStd3DStarted(bool bStarted);

#endif // STD_WIN95_SYSTEM_TEST_SUPPORT_H
