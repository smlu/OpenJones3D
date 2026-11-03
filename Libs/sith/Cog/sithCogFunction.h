#ifndef SITH_SITHCOGFUNCTION_H
#define SITH_SITHCOGFUNCTION_H
#include <j3dcore/j3d.h>
#include <sith/types.h>

J3D_EXTERN_C_START

int J3DAPI sithCogFunction_RegisterFunctions(SithCogSymbolTable* pTable);
void J3DAPI sithCogFunction_EnablePrint(bool bEnable); // Added
bool sithCogFunction_IsPrintEnabled(void); // Added

// Helper hooking functions
void sithCogFunction_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCOGFUNCTION_H
