#ifndef SMUSH_INSANE_FLOBJECT_D_WAVE_H
#define SMUSH_INSANE_FLOBJECT_D_WAVE_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/Engine.h>

/* D_wave.c: the VIMA wave frame-object decoder (`FLOBJECT=insane_d_wave`): plays an audio chunk's packets through SOUND. */

J3D_EXTERN_C_START

/* The frame-object decoder interface D_wave and D_bloc16 publish (Frame.c sees its leading six slots as FlobDecoder). */
typedef struct sDWaveInterface
{
    const char* (*Name)(int index);
    int  (*GetTag)(void);
    int  (*GetFlags)(void);
    void (*Reset)(void);
    int  (*Parse)(const uint8_t* pData, int size);
    uint32_t (*Decode)(void* a1, int a2, int a3, int a4, void* a5, const uint8_t* pData, int size, int mode, int a9, int a10);
    int  (*Query1)(void);
    int  (*Query)(const uint8_t* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight);
} DWaveInterface;
static_assert(sizeof(DWaveInterface) == 0x20, "sizeof(DWaveInterface) == 0x20");

int J3DAPI dwave_Register(LecShare* pShare, void* pParam);
void dwave_Unregister(void);
int dwave_Startup(void);
void dwave_Shutdown(void);
const char* J3DAPI dwave_Name(int index);
int dwave_GetTag(void);
int dwave_GetFlags(void);
void dwave_Reset(void);
int J3DAPI dwave_Parse(const uint8_t* pData, int size);
uint32_t J3DAPI dwave_Decode(void* a1, int a2, int a3, int a4, void* a5, const uint8_t* pData, int size, int mode, int a9, int a10);
int dwave_Query1(void); // Folded onto 0x004E8260 (shared with dbloc16_Query1, dwave_Query); never hook (see RTI/addresses.h)
int J3DAPI dwave_Query(const uint8_t* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight); // Folded onto 0x004E8260; no address of its own, never hook
void J3DAPI dwave_DecodeVima(int16_t* pDst, const uint8_t* pSrc, uint32_t count, uint32_t channels);

void D_wave_InstallHooks(void);
void D_wave_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_FLOBJECT_D_WAVE_H
