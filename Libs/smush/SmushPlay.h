#ifndef SMUSHPLAY_H
#define SMUSHPLAY_H

#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SmushPlay_SysStartup_ADDR 0x004E30B0
#define SmushPlay_SysStartup_TYPE int (J3DAPI*)(HWND, LPDIRECTSOUND)

#define SmushPlay_SysShutdown_ADDR 0x004E33A0
#define SmushPlay_SysShutdown_TYPE int (*)(void)

#define SmushPlay_SetGlobalVolume_ADDR 0x004E33C0
#define SmushPlay_SetGlobalVolume_TYPE void (J3DAPI*)(size_t)

#define SmushPlay_PlayMovie_ADDR 0x004E3140
#define SmushPlay_PlayMovie_TYPE int (J3DAPI*)(const char*, int, int, int, int, int, int, int, int, int (__cdecl*)(const SmushBitmap*, int), int, int, int)

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
    int unknown21;
    int unknown22;
} SmushBitmap;


inline int J3DAPI SmushPlay_SysStartup(HWND hwnd, LPDIRECTSOUND pDSound)
{
    return J3D_TRAMPOLINE_CALL(SmushPlay_SysStartup, hwnd, pDSound);
}

inline void SmushPlay_SysShutdown(void)
{
    J3D_TRAMPOLINE_CALL(SmushPlay_SysShutdown);
}

inline void J3DAPI SmushPlay_SetGlobalVolume(size_t volume)
{
    J3D_TRAMPOLINE_CALL(SmushPlay_SetGlobalVolume, volume);
}

inline int J3DAPI SmushPlay_PlayMovie(const char* pFilename, int a1, int a3, int a4, int a5, int width, int height, int a8, int a9, int(__cdecl* pBlit)(const SmushBitmap* pBitmap, int a2), int a11, int a12, int a13)
{
    return J3D_TRAMPOLINE_CALL(SmushPlay_PlayMovie, pFilename, a1, a3, a4, a5, width, height, a8, a9, pBlit, a11, a12, a13);
}

J3D_EXTERN_C_END


#endif // SMUSHPLAY_H