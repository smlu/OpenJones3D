#ifndef SITH_SITHSHADOW_H
#define SITH_SITHSHADOW_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

void J3DAPI sithShadow_RenderThingShadow(SithThing* pThing);
int J3DAPI sithShadow_SearchForAttachDistance(SithThing* pThing, float* distance, float* radius);
float J3DAPI sithShadow_DistanceThingToSurface(const SithThing* pThing, const rdVector3* normal, const rdFace* pFace);
void J3DAPI sithShadow_DrawShadow(const rdMatrix34* orient, float size, float scale, int bCar);
void J3DAPI sithShadow_DrawWalkShadow(float size, float scale, const rdVector3* leg, const rdVector3* rleg, const rdVector3* lvec, const rdVector3* rvec);

// Helper hooking functions
void sithShadow_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHSHADOW_H
