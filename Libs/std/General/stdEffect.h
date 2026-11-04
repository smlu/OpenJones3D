#ifndef STD_STDEFFECT_H
#define STD_STDEFFECT_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

void stdEffect_Startup(void);
void stdEffect_Shutdown(void); // Added
void stdEffect_Reset(void);
void J3DAPI stdEffect_SetFadeFactor(int bEnabled, float factor);
const tStdFadeFactor* J3DAPI stdEffect_GetFadeFactor();

// Helper hooking functions
void stdEffect_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDEFFECT_H
