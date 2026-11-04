#ifndef SITH_SITHSTRING_H
#define SITH_SITHSTRING_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

#include <wchar.h>

J3D_EXTERN_C_START

int sithString_Startup(void);
void sithString_Shutdown(void);
wchar_t* J3DAPI sithString_GetString(const char* pStr);

// Helper hooking functions
void sithString_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHSTRING_H
