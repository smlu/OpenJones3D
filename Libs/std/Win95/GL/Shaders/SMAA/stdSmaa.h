#ifndef STD_SMAA_H
#define STD_SMAA_H
//module for applying smaa anti-aliasing in OpenGL

#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

int stdSmaa_InitShaders(void);

int stdSmaa_InitFBOs(uint32_t width, uint32_t height);

void stdSmaa_Reset(void);

void stdSmaa_ApplySmaa(void);

J3D_EXTERN_C_END

#endif
