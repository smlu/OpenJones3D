#ifndef SITH_ENGINE_SITHRENDERVERTEXWELDER_H
#define SITH_ENGINE_SITHRENDERVERTEXWELDER_H

#include <j3dcore/j3d.h>
#include <std/types.h>

#ifdef J3D_OPENGL

J3D_EXTERN_C_START

bool sithRenderVertexWelder_Weld(const D3DTLVERTEX* pVertices, size_t numVertices, GLuint* pIndices, size_t numIndices, D3DTLVERTEX** ppOutVertices, size_t* pOutNumVertices);

J3D_EXTERN_C_END

#endif // J3D_OPENGL

#endif // SITH_ENGINE_SITHRENDERVERTEXWELDER_H
