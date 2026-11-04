// Added: private helpers shared by the two renderer forwarding-layer files. Only
// std3DCapture.c, stdDisplayCapture.c and their tests include this; it is not a public std header.
#ifndef STD_STDRENDERCAPTUREFORWARD_H
#define STD_STDRENDERCAPTUREFORWARD_H

#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

void stdDisplayCapture_PutColorInfo(const ColorInfo* pColorInfo);
void stdDisplayCapture_PutVertices(const StdVertexTL* aVerts, size_t numVerts);
void stdDisplayCapture_PutIndices(const uint16_t* aIndices, size_t numIndices);
uint32_t stdDisplayCapture_VBufferHandle(const tVBuffer* pVBuffer);
uint32_t stdDisplayCapture_WriteVBufferBlob(tVBuffer* pVBuffer);
uint32_t stdDisplayCapture_CacheTextureHandle(const tSysTexture* pTex);

J3D_EXTERN_C_END
#endif // STD_STDRENDERCAPTUREFORWARD_H
