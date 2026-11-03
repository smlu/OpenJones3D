#ifndef RDROID_RDPRIMIT2_H
#define RDROID_RDPRIMIT2_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <rdroid/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI rdPrimit2_DrawLine(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, uint32_t color, uint32_t pattern); // Added
int J3DAPI rdPrimit2_DrawClippedLine(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, uint32_t color, uint32_t pattern);
void rdPrimit2_Reset(void);
int J3DAPI rdPrimit2_DrawClippedLine2(float x1, float y1, float x2, float y2, uint32_t color);
void J3DAPI rdPrimit2_DrawClippedCircle(rdCanvas* pCanvas, int x, int y, float radius, float step, uint32_t color, uint32_t pattern);
void J3DAPI rdPrimit2_DrawClippedCircle2(float centerX, float centerY, float radius, float degrees, uint32_t color);

int J3DAPI rdPrimit2_DrawRectangle(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, uint32_t color, uint32_t pattern); //Added
int J3DAPI rdPrimit2_DrawTriangle(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, int a6, int a7, uint32_t color, uint32_t pattern); // Added

// Helper hooking functions
void rdPrimit2_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDPRIMIT2_H
