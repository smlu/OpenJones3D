#ifndef RDROID_RDPRIMIT3_H
#define RDROID_RDPRIMIT3_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI rdPrimit3_DrawPoint(rdVector3* pPoint, int32_t* pPPoint, uint32_t color); // Added
int J3DAPI rdPrimit3_DrawClippedLine(rdVector3* pPoint1, rdVector3* pPoint2, uint32_t color); // Added
void J3DAPI rdPrimit3_DrawClippedCircle(const rdVector3* pPos, float radius, float step, uint32_t color, uint32_t pattern);
void J3DAPI rdPrimit3_ClipFace(const rdClipFrustum* pFrustrum, rdGeometryMode geoMode, rdLightMode lightMode, const rdPrimit3* pSrc, rdPrimit3* pDest, const rdVector2* pTexVertOffset);

void rdPrimit3_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDPRIMIT3_H
