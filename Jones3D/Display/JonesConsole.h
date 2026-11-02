#ifndef JONES3D_JONESCONSOLE_H
#define JONES3D_JONESCONSOLE_H
#include <Jones3D/types.h>
#include <Jones3D/RTI/addresses.h>
#include <j3dcore/j3d.h>

#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define JONESCONSOLE_FRAMERATEID 102u

#if defined(J3D_STANDALONE)
extern int JonesConsole_g_bVisible;
#else
#define JonesConsole_g_bVisible J3D_DECL_FAR_VAR(JonesConsole_g_bVisible, int)
#endif
// extern int JonesConsole_g_bVisible;

#if defined(J3D_STANDALONE)
extern int JonesConsole_g_bStarted;
#else
#define JonesConsole_g_bStarted J3D_DECL_FAR_VAR(JonesConsole_g_bStarted, int)
#endif
// extern int JonesConsole_g_bStarted;

int JonesConsole_Startup(void);
void JonesConsole_Shutdown(void);

int JonesConsole_Open(void);
void JonesConsole_Close(void);

size_t J3DAPI JonesConsole_PrintText(const char* pText);
void J3DAPI JonesConsole_PrintTextWithID(int ID, const char* pText);
void J3DAPI JonesConsole_ClearText(int yourID);

void JonesConsole_ShowConsole(void);
void J3DAPI JonesConsole_HandelChar(char chr);
void JonesConsole_HideConsole(void);

// Helper hooking functions
void JonesConsole_InstallHooks(void);
void JonesConsole_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESCONSOLE_H
