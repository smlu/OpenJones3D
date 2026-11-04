#ifndef SITH_SITHCOGFUNCTIONSURFACE_H
#define SITH_SITHCOGFUNCTIONSURFACE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

void J3DAPI sithCogFunctionSurface_RegisterFunctions(SithCogSymbolTable* pTbl);

// Helper hooking functions
void sithCogFunctionSurface_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCOGFUNCTIONSURFACE_H
