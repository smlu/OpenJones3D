#ifndef SITH_SITHCOGFUNCTIONTHING_H
#define SITH_SITHCOGFUNCTIONTHING_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI sithCogFunctionThing_RegisterFunctions(SithCogSymbolTable* pTable);

void sithCogFunctionThing_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCOGFUNCTIONTHING_H
