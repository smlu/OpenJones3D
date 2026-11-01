#ifndef SMUSH_INSANE_ANIMATE_FRAME_H
#define SMUSH_INSANE_ANIMATE_FRAME_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>
#include <smush/insane/Memory/Memory.h>

/*
 * The `ANIMFRAME=insane` service Frame.c publishes (`animframe_interface`): the frame stream (Framestream.c) parses,
 * decodes and fetches frames through it, the animation player (Pcplay.c) resets and draws.
 */

J3D_EXTERN_C_START

typedef struct sAnimFrameTable
{
    void (*Reset)(void);
    int  (*Parse)(const uint8_t* pData, int size);
    uint32_t (*Decode)(void* pBitmap, int a2, int a3, int a4, void* pPalette, int* pOutA, int* pOutB, uint32_t flags, int a9, int a10, const uint8_t* pData, int size);
    void (*Draw)(void* pBitmap, int a2, int a3, int a4, void* pPalette, int a6, int a7, int a8);
    void (*Load)(const int16_t* pData, int size);
    void* (*Fetch)(int n);
} AnimFrameTable;
static_assert(sizeof(AnimFrameTable) == 0x18, "sizeof(AnimFrameTable) == 0x18");

#define ANIMFRAME_MAXDECODERS 100

/* The frame-object decoders (D_wave, D_bloc16) as Frame.c sees them: the leading slots of their 0x20-byte interface tables. */
typedef struct sFlobDecoder
{
    const char* (*Name)(int index);
    int  (*GetTag)(void);
    int  (*GetFlags)(void);
    void (*Reset)(void);
    int  (*Parse)(const uint8_t* pData, int size);
    uint32_t (*Decode)(void* pBitmap, int a2, int a3, int a4, void* pPalette, const uint8_t* pData, int size, int mode, int a9, int a10);
} FlobDecoder;
static_assert(sizeof(FlobDecoder) == 0x18, "sizeof(FlobDecoder) == 0x18");

typedef struct sAnimFrameDisplayInfo
{
    char pad[0x20];
    int f20;
} AnimFrameDisplayInfo;
static_assert(sizeof(AnimFrameDisplayInfo) == 0x24, "sizeof(AnimFrameDisplayInfo) == 0x24");

/* The display service Frame.c subscribes to (slot roles inferred). */
typedef struct sAnimFrameDisplayService
{
    void* pfn00;
    AnimFrameDisplayInfo* (*GetInfo)(void);
    void (*Blit)(void* pBitmap, int a3, int a4, int a2);
    void* pfn0C;
    void (*Present)(void* pPalette);
    void* pfn14;
    void (*SetPalette)(int a, int b, int count);
} AnimFrameDisplayService;
static_assert(sizeof(AnimFrameDisplayService) == 0x1C, "sizeof(AnimFrameDisplayService) == 0x1C");

/* The `DEBUG=animframe` interface. */
typedef struct sAnimFrameDebug
{
    const char* (*Name)(int index);
    void (*SetFlags)(int flags);
    int  (*GetFlags)(void);
} AnimFrameDebug;
static_assert(sizeof(AnimFrameDebug) == 0xC, "sizeof(AnimFrameDebug) == 0xC");

int J3DAPI animframe_Register(LecShare* pShare, void* pParam);
void animframe_Unregister(void);
int animframe_Startup(void);
void animframe_Shutdown(void);
void animframe_ResetDecoders(void);
int J3DAPI animframe_Parse(const uint8_t* pData, int size);
uint32_t J3DAPI animframe_Decode(void* pBitmap, int a2, int a3, int a4, void* pPalette, int* pOutA, int* pOutB, uint32_t flags, int a9, int a10, const uint8_t* pData, int size);
void J3DAPI animframe_Load(const int16_t* pData, int size);
void* J3DAPI animframe_Fetch(int n);
void J3DAPI animframe_Draw(void* pBitmap, int a2, int a3, int a4, void* pPalette, int a6, int a7, int a8);
const char* J3DAPI animframe_DebugName(int index);
void J3DAPI animframe_SetDebug(int flags);
int animframe_GetDebug(void);

void Frame_InstallHooks(void);
void Frame_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_ANIMATE_FRAME_H
