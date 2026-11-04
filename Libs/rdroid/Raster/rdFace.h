#ifndef RDROID_RDFACE_H
#define RDROID_RDFACE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

rdFace* J3DAPI rdFace_New(void); // Added
int J3DAPI rdFace_NewEntry(rdFace* pFace);

void J3DAPI rdFace_Free(rdFace* pFace); // Added
void J3DAPI rdFace_FreeEntry(rdFace* pFace);

void rdFace_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDFACE_H
