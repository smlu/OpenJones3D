#ifndef RDROID_RDTHING_H
#define RDROID_RDTHING_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

rdThing* J3DAPI rdThing_New(SithThing* pThing);
void J3DAPI rdThing_NewEntry(rdThing* prdThing, SithThing* pThing);

void J3DAPI rdThing_Free(rdThing* pThing);
void J3DAPI rdThing_FreeEntry(rdThing* pThing);

int J3DAPI rdThing_SetModel3(rdThing* pThing, rdModel3* pModel3);
int J3DAPI rdThing_SetSprite3(rdThing* pThing, rdSprite3* pSprite3);
int J3DAPI rdThing_SetPolyline(rdThing* pThing, rdPolyline* pPolyline);
int J3DAPI rdThing_SetParticleCloud(rdThing* pThing, rdParticle* pParticle);

int J3DAPI rdThing_Draw(rdThing* pThing, const rdMatrix34* pOrient);

void J3DAPI rdThing_AccumulateMatrices(rdThing* pThing, const rdModel3HNode* pNode, const rdMatrix34* pPlacement);

// Helper hooking functions
void rdThing_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDTHING_H
