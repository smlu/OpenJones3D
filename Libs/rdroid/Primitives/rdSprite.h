#ifndef RDROID_RDSPRITE_H
#define RDROID_RDSPRITE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <rdroid/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

rdSprite3* J3DAPI rdSprite_New(int type, const char* pName, const char* pMatNam, float width, float height, rdGeometryMode geomode, rdLightMode lightmode, const rdVector4* pExtraLight, const rdVector3* pOffset); // Added
int J3DAPI rdSprite_NewEntry(rdSprite3* pSprite, const char* pName, int type, const char* pMatName, float width, float height, rdGeometryMode geomode, rdLightMode lightmode, const rdVector4* pExtraLight, const rdVector3* pOffset);
void J3DAPI rdSprite_Free(rdSprite3* pSprite); // Added
void J3DAPI rdSprite_FreeEntry(rdSprite3* pSprite);
int J3DAPI rdSprite_Draw(rdThing* prdThing, const rdMatrix34* orient);

// Helper hooking functions
void rdSprite_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDSPRITE_H
