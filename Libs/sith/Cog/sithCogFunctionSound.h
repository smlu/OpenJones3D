#ifndef SITH_SITHCOGFUNCTIONSOUND_H
#define SITH_SITHCOGFUNCTIONSOUND_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI sithCogFunctionSound_RegisterFunctions(SithCogSymbolTable* pTable);

// Helper hooking functions
void sithCogFunctionSound_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCOGFUNCTIONSOUND_H
