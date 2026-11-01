#ifndef SMUSH_INSANE_SOUND_MIXER_H
#define SMUSH_INSANE_SOUND_MIXER_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/Bitmap/Bitmap.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/Dsnd.h>

/*
 * INSANE's software mixer (Mixer.c): the DirectSound driver (Dsnd.c) clears and clips each block through these and hands
 * mixer_Feed (a SoundFeed, Dsnd.h) to the sound engine's fill callback (Engine.c) as the per-track feed. mixer_Blit is
 * the clipped bitmap blit the blocky16 decoder (D_bloc16.c) uses.
 */

J3D_EXTERN_C_START

typedef struct sMixerRect
{
    int x;
    int y;
    int width;
    int height;
} MixerRect;
static_assert(sizeof(MixerRect) == 0x10, "sizeof(MixerRect) == 0x10");

int J3DAPI mixer_Init(MemoryDebugInterface* pMemory, int headroom, int size, int a4, int a5, int a6);
void J3DAPI mixer_Done(MemoryDebugInterface* pMemory);
int J3DAPI mixer_Begin(int n);
void J3DAPI mixer_Feed(void* pSrc, int offset, int n, int bytes, int a5, int volume, int pan, int format);
int J3DAPI mixer_Mix(int16_t* pDst, int n);
int J3DAPI mixer_Setup(MemoryDebugInterface* pMemory, int channels, int size, int a4);
int J3DAPI mixer_Free(MemoryDebugInterface* pMemory);
int mixer_Clear(void);
void J3DAPI mixer_Add(void* pSrc, uint32_t count, int bits, int channels, uint32_t n, int offset, int volume, int pan, int isSigned, int isSwapped);
int J3DAPI mixer_Output(int16_t* pDst, int n);
void J3DAPI mixer_Mono8(uint8_t* pSrc, uint32_t count, uint32_t n, int offset, int left, int right);
void J3DAPI mixer_Stereo8(uint8_t* pSrc, uint32_t count, uint32_t n, int offset, int volume);
void J3DAPI mixer_Mono16(int16_t* pSrc, uint32_t count, uint32_t n, int offset, int left, int right);
void J3DAPI mixer_Stereo16(int16_t* pSrc, uint32_t count, uint32_t n, int offset, int volume);
void J3DAPI mixer_MonoU16(uint16_t* pSrc, uint32_t count, uint32_t n, int offset, int left, int right);
void J3DAPI mixer_StereoU16(uint16_t* pSrc, uint32_t count, uint32_t n, int offset, int volume);
void J3DAPI mixer_MonoSwap16(int16_t* pSrc, uint32_t count, uint32_t n, int offset, int left, int right);
void J3DAPI mixer_StereoSwap16(int16_t* pSrc, uint32_t count, uint32_t n, int offset, int volume);
void J3DAPI mixer_Blit(LecBitmapHeader* pDst, MixerRect* pClip, int x, int y, LecBitmapHeader* pSrc, void* pPalette, void* pTable);

void Mixer_InstallHooks(void);
void Mixer_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_SOUND_MIXER_H
