#ifndef RDROID_RDWALLPAPER_H
#define RDROID_RDWALLPAPER_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <rdroid/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

rdWallpaper* J3DAPI rdWallpaper_New(const char* pName);
void J3DAPI rdWallpaper_Free(rdWallpaper* pWallpaper);

void J3DAPI rdWallpaper_Draw(const rdWallpaper* pWallpaper);
rdWallLine* J3DAPI rdWallpaper_NewWallLine(float startX, float startY, float endX, float endY, const rdVector4* pColor);
void J3DAPI rdWallpaper_FreeWallLine(rdWallLine* pWallLine);
void J3DAPI rdWallpaper_DrawWallLine(const rdWallLine* pLine, float progress);

// Helper hooking functions
void rdWallpaper_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDWALLPAPER_H
