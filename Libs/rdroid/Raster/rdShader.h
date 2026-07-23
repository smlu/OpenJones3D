#ifndef RDROID_RDSHADER_H
#define RDROID_RDSHADER_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <rdroid/RTI/addresses.h>
#include <std/types.h>
#include <std/Win95/std3D.h>

J3D_EXTERN_C_START

void rdShader_ConvertToMat4(const rdMatrix34* pMat, float out[16]);
void rdShader_UpdateCameraData(void);
void rdShader_SetShaderLights(void);

J3D_EXTERN_C_END

#endif // RDROID_RDSHADER_H
