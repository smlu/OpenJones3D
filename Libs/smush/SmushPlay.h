#ifndef SMUSH_SMUSHPLAY_H
#define SMUSH_SMUSHPLAY_H
#include <assert.h>
#include <stdint.h>

#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>
#include <sound/Driver.h>
#include <std/types.h>

J3D_EXTERN_C_START

/*
 * SmushPlay: the glue between the game and LucasArts' INSANE movie-player library (original 0x004E30B0-0x004E33F0). It
 * registers every INSANE module with the LEC share table, subscribes to the services it drives and plays a movie by looping the
 * animation player's frame pacer, handing each decoded frame to the game's blit callback.
 */

typedef struct sSmushColorFormat
{
    int redBPP;
    int greenBPP;
    int blueBPP;
    int redPosShift;
    int greenPosShift;
    int bluePosShift;
    int redPosShiftRight;
    int greenPosShiftRight;
    int bluePosShiftRight;
    int alphaBPP;
    int alphaPosShift;
    int alphaPosShiftRight;
} SmushColorFormat;
static_assert(sizeof(SmushColorFormat) == 0x30, "sizeof(SmushColorFormat) == 0x30");

/* The original's bitmap header (INSANE's LecBitmapHeader, Bitmap.h): SmushPlay_PlayMovie hands its blit callback a pointer to one. */
typedef struct sSmushBitmap
{
    void* pPixels;
    uint32_t width;
    uint32_t height;
    int pixelSize;
    int bpp;
    size_t pitch;
    int unknown6;
    int unknown7;
    tColorMode colorMode;
    SmushColorFormat format;
} SmushBitmap;
static_assert(sizeof(SmushBitmap) == 0x54, "sizeof(SmushBitmap) == 0x54");

int J3DAPI SmushPlay_SysStartup(HWND hwnd, tDirectSound* pDSound);
void SmushPlay_SysShutdown(void);
void J3DAPI SmushPlay_SetGlobalVolume(size_t volume);
int J3DAPI SmushPlay_PlayMovie(const char* pFilename, int a1, int a3, int a4, int a5, int width, int height, int a8, int a9, int(__cdecl* pBlit)(const SmushBitmap* pBitmap, int a2), int a11, int a12, int a13);
int J3DAPI smushplay_Blit(int a1, int a2, int a3, int a4, void* pBitmap);
void* smushplay_GetHwnd(void);

// Installs/resets every smush translation unit (called from the DLL's InstallHooks/ResetGlobals once the library is enabled)
void Smush_InstallHooks(void);
void Smush_ResetGlobals(void);

void SmushPlay_InstallHooks(void);
void SmushPlay_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_SMUSHPLAY_H
