#ifndef SMUSH_TESTS_SMUSHVECCASES_H
#define SMUSH_TESTS_SMUSHVECCASES_H
/*
 * Shared case generator of the smush C-module original-engine vectors (tv/README.md, "C Modules").
 *
 * The same code runs twice: inside the retail Indy3D.exe (capture harness, outside the repository), where every function
 * pointer of SmushVecApi holds a v1.0 address, and in SmushVecTest.c, where they hold the ported functions. Every case
 * builds its inputs from fixed seeds, calls the functions under test with stub engine services (memory, bitmaps, file
 * I/O, streamer, frame decoder, zlib) and reports each observable result as a named record: the bytes themselves or
 * their 64-bit FNV-1a hash. The fixture files hold the original's records; the test requires the port's records to be
 * identical, in the same order.
 *
 * Records never contain pointers: buffers are identified by their creation order and pointers into known buffers by
 * their offset. `fill` is the byte the stubs fill fresh memory with; results must not depend on it (the capture runs
 * every area with 0x00 and 0xFF and keeps only identical record streams, the test uses a third value).
 */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <smush/insane/lec.h>
#include <smush/insane/animplay.h>
#include <smush/insane/filio.h>
#include <smush/insane/Lec/LecShare.h>
#include <smush/insane/Bitmap/Bitmap.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/Mixer.h>
#include <smush/insane/Flobject/D_bloc16.h>
#include <smush/insane/File/Cdstream.h>
#include <smush/insane/File/Framestream.h>
#include <smush/insane/File/Stdfile.h>
#include <smush/insane/File/Zfilio/Zfilio.h>
#include <smush/insane/Animate/Frame.h>

#define SMUSHVEC_MAGIC   "SVTV"
#define SMUSHVEC_VERSION 1u

#define SMUSHVEC_RAW  0 /* the record holds the bytes */
#define SMUSHVEC_HASH 1 /* the record holds the size and the FNV-1a hash of the bytes */
#define SMUSHVEC_DETAIL 3 /* a part of a section record (SmushVec_SectionBegin): only for debug dumps, not compared */

typedef struct sSmushVecGz
{
    gzFile (J3DAPI* gzopen)(const char*, const char*);
    int (J3DAPI* gzclose)(gzFile);
    int (J3DAPI* gzread)(gzFile, void*, unsigned int);
    int (J3DAPI* gzwrite)(gzFile, const void*, unsigned int);
    int (J3DAPI* gzflush)(gzFile, int);
    long (J3DAPI* gzseek)(gzFile, long, int);
} SmushVecGz;

/* The functions under test. The harness fills it with v1.0 addresses, the test with the ported functions. */
typedef struct sSmushVecApi
{
    /* Sound/Mixer.c */
    int (J3DAPI* mixer_Init)(MemoryDebugInterface* pMemory, int headroom, int size, int a4, int a5, int a6);
    int (J3DAPI* mixer_Setup)(MemoryDebugInterface* pMemory, int channels, int size, int a4);
    int (J3DAPI* mixer_Free)(MemoryDebugInterface* pMemory);
    int (*mixer_Clear)(void);
    void (J3DAPI* mixer_Feed)(void* pSrc, int offset, int n, int bytes, int a5, int volume, int pan, int format);
    void (J3DAPI* mixer_Add)(void* pSrc, uint32_t count, int bits, int channels, uint32_t n, int offset, int volume, int pan, int isSigned, int isSwapped);
    int (J3DAPI* mixer_Output)(int16_t* pDst, int n);
    void (J3DAPI* mixer_Blit)(LecBitmapHeader* pDst, MixerRect* pClip, int x, int y, LecBitmapHeader* pSrc, void* pPalette, void* pTable);

    /* Flobject/D_bloc16.c and its tables */
    int (J3DAPI* dbloc16_Register)(LecShare* pShare, void* pParam);
    int (J3DAPI* dbloc16_Configure)(int tag, ...);
    void (*dbloc16_Unregister)(void);
    int (*dbloc16_Startup)(void);
    void (*dbloc16_Shutdown)(void);
    void (J3DAPI* dbloc16_MakeTables)(int width);
    void (J3DAPI* dbloc16_MakeGlyphs)(int size);
    int (J3DAPI* dbloc16_DecodeFrame)(LecBitmapHeader* pBitmap, MixerRect* pClip, int x, int y, void* pPalette, Bl16Header* pHdr, int flags);
    void (J3DAPI* dbloc16_Scale)(uint16_t* pDst, uint16_t* pSrc, int width, int height);
    uint16_t (J3DAPI* dbloc16_Average)(uint16_t a, uint16_t b);
    void (J3DAPI* dbloc16_ScalePal)(uint16_t* pDst, uint8_t* pSrc, int width, int height, uint16_t* pPalette);
    int16_t* pOffsets;      /* dbloc16_offsets[256] */
    Bl16Small* pSmall;      /* dbloc16_small[256] */
    Bl16Big* pBig;          /* dbloc16_big[256] */

    /* Decode/Decode.c */
    void (J3DAPI* decode_Rle)(uint8_t* pDst, uint8_t* pSrc, int n);
    void (J3DAPI* decode_Run)(uint8_t* pDst, LecBitmapHeader* pBitmap, int n, uint8_t* pPixel);
    int (J3DAPI* decode_MakeTable)(LecBitmapHeader* pSrcBitmap, LecBitmapHeader* pDstBitmap, void* pTable, void* pPalette);
    int (J3DAPI* decode_Convert)(uint8_t* pSrc, LecBitmapHeader* pSrcBitmap, int n, void* pPalette, uint8_t* pDst, LecBitmapHeader* pDstBitmap, void* pTable);
    int (J3DAPI* decode_ToRGBA)(uint8_t* pSrc, LecBitmapHeader* pBitmap, int n, void* pPalette, uint8_t* pDst);
    int (J3DAPI* decode_Begin)(uint8_t* pDst, LecBitmapHeader* pBitmap, int n, void* pPalette, uint8_t* pSrc);

    /* Lec/LecShare.c */
    LecShare* (J3DAPI* LecShare_Init)(LecErrorFn pError, LecModuleEntry* pModules, const char* pA, const char* pB);
    void (J3DAPI* LecShare_Reset)(const char* pA, const char* pB);
    int (J3DAPI* LecShare_Publish)(const char* pName, void* pInterface);
    int (J3DAPI* LecShare_Unpublish)(const char* pName);
    void* (J3DAPI* LecShare_Subscribe)(const char* pName, const char* pPreferred);
    int (J3DAPI* LecShare_Unsubscribe)(void* pInterface);
    int (J3DAPI* LecShare_BatchSubscribe)(LecBatch* pTable, const char* pDefault);
    int (J3DAPI* LecShare_BatchUnsubscribe)(LecBatch* pTable);
    void* (J3DAPI* LecShare_Enumerate)(const char* pName, int index);
    int (J3DAPI* LecShare_Count)(const char* pName);
    LecEntry* (J3DAPI* LecShare_Find)(const char* pName);
    int (J3DAPI* LecShare_Match)(const char* pName, const char* pList);
    int (J3DAPI* LecShare_NameLen)(const char* pName);
    void (J3DAPI* LecShare_Strip)(char* pName);
    int (J3DAPI* LecShare_NameCmp)(const char* pA, const char* pB);
    int (J3DAPI* LecShare_NameNCmp)(const char* pA, const char* pB, int n);

    /* File/Framestream.c and the player globals it reads */
    int (J3DAPI* framestream_Reset)(AnimPlay* pFrames);
    int (J3DAPI* framestream_Tick)(AnimPlay* pFrames, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10, int a11);
    int (J3DAPI* framestream_Open)(AnimPlay* pFrames, const char* pName, int flags, int seekLen, int seekTo, int hold);
    void (J3DAPI* framestream_Seek)(AnimPlay* pFrames, int length, int to);
    int (J3DAPI* framestream_Mode)(AnimPlay* pFrames, int op);
    int (J3DAPI* framestream_IsOpen)(AnimPlay* pFrames);
    void (J3DAPI* framestream_Stall)(AnimPlay* pFrames);
    /* Sets animplay_pShare/pStreamer/pFrame/ops; pOps NULL restores the values saved by the first call */
    void (*SetAnimGlobals)(LecShare* pShare, StreamerInterface* pStreamer, AnimFrameTable* pFrame, const AnimPlayOps* pOps);

    /* File/Stdfile.c (the derived operations are reached through the published FILE interface) */
    int (J3DAPI* stdfile_Register)(LecShare* pShare, void* pParam);
    void (*stdfile_Unregister)(void);
    int (*stdfile_Startup)(void);
    void (*stdfile_Shutdown)(void);

    /* File/Zfilio/Zfilio.c */
    int (J3DAPI* zfilio_Register)(LecShare* pShare, void* pParam);
    int (J3DAPI* zfilio_Configure)(int tag, ...);
    void (*zfilio_Unregister)(void);
    int (*zfilio_Startup)(void);
    void (*zfilio_Shutdown)(void);
    /* Restores the extension list and flags to their initial values (".gz" only) */
    void (*ZfilioReset)(void);
    /* Routes Zfilio's zlib calls to the stubs (NULL: back to the defaults) */
    void (*ZfilioSetGz)(const SmushVecGz* pGz);
} SmushVecApi;

typedef struct sSmushVecSink
{
    void (*Record)(void* pUser, int kind, const char* pName, const void* pData, uint32_t size);
    void* pUser;
} SmushVecSink;

/* ------------------------------------------------------------------------------------------------------------------- */
/* Context, records and generators                                                                                     */
/* ------------------------------------------------------------------------------------------------------------------- */

#define SMUSHVEC_MAXALLOCS  64
#define SMUSHVEC_MAXBITMAPS 16
#define SMUSHVEC_MAXPUB     16
#define SMUSHVEC_LOGSIZE    4096

typedef struct sSmushVecAlloc
{
    void* p;
    int size;
} SmushVecAlloc;

typedef struct sSmushVecBitmap
{
    LecBitmapHeader* pHeader;
    uint8_t* pBlock;
    int id;
    int size;   /* pixel bytes */
} SmushVecBitmap;

typedef struct sSmushVecContext
{
    const SmushVecApi* pApi;
    SmushVecSink* pSink;
    uint8_t fill;

    MemoryDebugInterface memory;
    int failAlloc;                          /* the failAlloc-th allocation from now on fails (0: none) */
    SmushVecAlloc allocs[SMUSHVEC_MAXALLOCS]; /* allocations in order since the last reset (freed ones keep p NULL) */
    int numAllocs;
    int freedSizes[SMUSHVEC_MAXALLOCS];
    int numFreed;

    BitmapInterface bitmaps;
    SmushVecBitmap aBitmaps[SMUSHVEC_MAXBITMAPS];
    int nextBitmapId;
    int failCreate;
    uint8_t aBitmapLog[256];
    int bitmapLogSize;

    LecShare share;
    struct { const char* pName; void* pInterface; } aPub[SMUSHVEC_MAXPUB];
    int numPub;
    int numErrors;
    char aLastError[128];

    /* Generic call log of the stubs (op byte + little-endian ints), recorded and cleared per step */
    uint8_t aLog[SMUSHVEC_LOGSIZE];
    int logSize;

    int inSection;
    char aSectionName[160];
    uint8_t* pSection;
    uint32_t sectionSize;
    uint32_t sectionCap;
} SmushVecContext;

static SmushVecContext sv;

static uint32_t SmushVec_Lcg(uint32_t* pSeed)
{
    *pSeed = *pSeed * 1664525u + 1013904223u;
    return *pSeed;
}

static uint64_t SmushVec_Hash(const void* pData, size_t size)
{
    const uint8_t* p = (const uint8_t*)pData;
    uint64_t hash = 0xCBF29CE484222325ull;
    for ( size_t i = 0; i < size; ++i )
    {
        hash ^= p[i];
        hash *= 0x100000001B3ull;
    }

    return hash;
}

static void SmushVec_RandBytes(void* pDst, size_t size, uint32_t seed)
{
    uint8_t* p = (uint8_t*)pDst;
    for ( size_t i = 0; i < size; ++i )
    {
        p[i] = (uint8_t)(SmushVec_Lcg(&seed) >> 24);
    }
}

static void SmushVec_Append(const void* pData, uint32_t size)
{
    if ( sv.sectionSize + size > sv.sectionCap )
    {
        sv.sectionCap = (sv.sectionSize + size) * 2 + 4096;
        sv.pSection   = (uint8_t*)realloc(sv.pSection, sv.sectionCap);
    }

    memcpy(sv.pSection + sv.sectionSize, pData, size);
    sv.sectionSize += size;
}

static void SmushVec_Put(int kind, const void* pData, uint32_t size, const char* pFormat, va_list args)
{
    char aName[160];
    vsnprintf(aName, sizeof(aName), pFormat, args);
    aName[sizeof(aName) - 1] = 0;
    if ( !sv.inSection )
    {
        sv.pSink->Record(sv.pSink->pUser, kind, aName, pData, size);
        return;
    }

    // Inside a section the result joins the section's record (name, size, bytes); the sink sees it as a detail
    // (SMUSHVEC_DETAIL) that only the debug dumps keep
    uint32_t nameLen = (uint32_t)strlen(aName);
    SmushVec_Append(&nameLen, 4);
    SmushVec_Append(aName, nameLen);
    SmushVec_Append(&size, 4);
    SmushVec_Append(pData, size);
    sv.pSink->Record(sv.pSink->pUser, SMUSHVEC_DETAIL, aName, pData, size);
}

/* Starts a section: the following results form one record (stored up to 64 bytes, hashed above) */
static void SmushVec_SectionBegin(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    vsnprintf(sv.aSectionName, sizeof(sv.aSectionName), pFormat, args);
    va_end(args);
    sv.aSectionName[sizeof(sv.aSectionName) - 1] = 0;
    sv.inSection   = 1;
    sv.sectionSize = 0;
}

static void SmushVec_SectionEnd(void)
{
    sv.inSection = 0;
    sv.pSink->Record(sv.pSink->pUser, sv.sectionSize <= 64 ? SMUSHVEC_RAW : SMUSHVEC_HASH, sv.aSectionName, sv.pSection, sv.sectionSize);
}

static void SmushVec_Raw(const void* pData, uint32_t size, const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    SmushVec_Put(SMUSHVEC_RAW, pData, size, pFormat, args);
    va_end(args);
}

static void SmushVec_HashRec(const void* pData, uint32_t size, const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    SmushVec_Put(SMUSHVEC_HASH, pData, size, pFormat, args);
    va_end(args);
}

/* Small results are stored, larger ones hashed */
static void SmushVec_Bytes(const void* pData, uint32_t size, const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    SmushVec_Put(size <= 64 ? SMUSHVEC_RAW : SMUSHVEC_HASH, pData, size, pFormat, args);
    va_end(args);
}

static void SmushVec_Int(int value, const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    SmushVec_Put(SMUSHVEC_RAW, &value, sizeof(value), pFormat, args);
    va_end(args);
}

/* A string result: its bytes up to and including the terminator, or the single byte 0xFF for NULL */
static void SmushVec_Str(const char* pStr, const char* pFormat, ...)
{
    static const uint8_t nullMark = 0xFF;
    va_list args;
    va_start(args, pFormat);
    if ( pStr )
    {
        SmushVec_Put(SMUSHVEC_RAW, pStr, (uint32_t)strlen(pStr) + 1, pFormat, args);
    }
    else
    {
        SmushVec_Put(SMUSHVEC_RAW, &nullMark, 1, pFormat, args);
    }

    va_end(args);
}

static void SmushVec_Log8(int value)
{
    if ( sv.logSize < SMUSHVEC_LOGSIZE )
    {
        sv.aLog[sv.logSize++] = (uint8_t)value;
    }
}

static void SmushVec_Log32(int value)
{
    for ( int i = 0; i < 4; ++i )
    {
        SmushVec_Log8((int)((uint32_t)value >> (8 * i)));
    }
}

static void SmushVec_LogFlush(const char* pName)
{
    SmushVec_Bytes(sv.aLog, (uint32_t)sv.logSize, "%s: calls", pName);
    sv.logSize = 0;
}

/* --- Memory services (MEMORY.workspace, MEMORY.buffers) --- */
static void* SmushVec_MemAlloc(int size, const char* pFile, int line)
{
    (void)pFile;
    (void)line;
    if ( sv.failAlloc && --sv.failAlloc == 0 )
    {
        if ( sv.numAllocs < SMUSHVEC_MAXALLOCS )
        {
            sv.allocs[sv.numAllocs].p    = NULL;
            sv.allocs[sv.numAllocs].size = -size;
            sv.numAllocs++;
        }

        return NULL;
    }

    void* p = malloc(size > 0 ? (size_t)size : 1);
    if ( p && size > 0 )
    {
        memset(p, sv.fill, (size_t)size);
    }

    if ( sv.numAllocs < SMUSHVEC_MAXALLOCS )
    {
        sv.allocs[sv.numAllocs].p    = p;
        sv.allocs[sv.numAllocs].size = size;
        sv.numAllocs++;
    }

    return p;
}

static void SmushVec_MemFree(void* pBlock, const char* pFile, int line)
{
    (void)pFile;
    (void)line;
    int size = -1;
    for ( int i = 0; i < sv.numAllocs; ++i )
    {
        if ( sv.allocs[i].p == pBlock && pBlock )
        {
            size            = sv.allocs[i].size;
            sv.allocs[i].p  = NULL;
            break;
        }
    }

    if ( sv.numFreed < SMUSHVEC_MAXALLOCS )
    {
        sv.freedSizes[sv.numFreed++] = size;
    }

    free(pBlock);
}

static void* SmushVec_MemRealloc(void* pBlock, int size, const char* pFile, int line)
{
    (void)pFile;
    (void)line;
    return realloc(pBlock, (size_t)size);
}

/* Records the sizes of the allocations and frees since the last call (negative: a failed allocation of that size) */
static void SmushVec_AllocFlush(const char* pName)
{
    int aSizes[SMUSHVEC_MAXALLOCS];
    for ( int i = 0; i < sv.numAllocs; ++i )
    {
        aSizes[i] = sv.allocs[i].size;
    }

    SmushVec_Raw(aSizes, (uint32_t)(sv.numAllocs * sizeof(int)), "%s: allocations", pName);
    SmushVec_Raw(sv.freedSizes, (uint32_t)(sv.numFreed * sizeof(int)), "%s: frees", pName);
    sv.numFreed = 0;
}

static void SmushVec_AllocReset(void)
{
    sv.numAllocs = 0;
    sv.numFreed  = 0;
}

/* --- Bitmap services (BITMAP.clear): 16-bit 565 bitmaps with wide margins, so that motion vectors that leave the
       frame read deterministic data --- */
static void SmushVec_SetFormat(LecBitmapHeader* pBitmap, int colorMode, int pixelSize, const int* pFormat)
{
    pBitmap->colorMode = colorMode;
    pBitmap->pixelSize = pixelSize;
    pBitmap->bpp       = pixelSize * 8;
    if ( pFormat )
    {
        memcpy(&pBitmap->format, pFormat, sizeof(BitmapFormat));
    }
}

/* BPP r g b, shift r g b, shift right r g b, alpha BPP shift shiftRight */
static const int smushvec_fmt565[12]  = { 5, 6, 5, 11, 5, 0, 3, 2, 3, 0, 0, 0 };
static const int smushvec_fmt555[12]  = { 5, 5, 5, 10, 5, 0, 3, 3, 3, 0, 0, 0 };
static const int smushvec_fmt565B[12] = { 5, 6, 5, 0, 5, 11, 3, 2, 3, 0, 0, 0 };
static const int smushvec_fmt444[12]  = { 4, 4, 4, 8, 4, 0, 4, 4, 4, 0, 0, 0 };
static const int smushvec_fmt888[12]  = { 8, 8, 8, 16, 8, 0, 0, 0, 0, 0, 0, 0 };
static const int smushvec_fmtRGBA[12] = { 8, 8, 8, 0, 8, 16, 0, 0, 0, 8, 24, 0 };
static const int smushvec_fmtBGRA[12] = { 8, 8, 8, 16, 8, 0, 0, 0, 0, 8, 24, 0 };
static const int smushvec_fmt4444[12] = { 4, 4, 4, 12, 8, 4, 4, 4, 4, 4, 0, 4 };
static const int smushvec_fmt1555[12] = { 5, 5, 5, 10, 5, 0, 3, 3, 3, 1, 15, 7 };

static int SmushVec_BitmapMargin(int width)
{
    return (64 * (width > 0 ? width : 1) + 1024) * 2;
}

static int SmushVec_BitmapCreate(LecBitmapHeader** ppBitmap, int width, int height, int type)
{
    if ( sv.bitmapLogSize + 13 <= (int)sizeof(sv.aBitmapLog) )
    {
        uint8_t* p = sv.aBitmapLog + sv.bitmapLogSize;
        p[0] = 'C';
        memcpy(p + 1, &width, 4);
        memcpy(p + 5, &height, 4);
        memcpy(p + 9, &type, 4);
        sv.bitmapLogSize += 13;
    }

    if ( sv.failCreate && --sv.failCreate == 0 )
    {
        return -3;
    }

    int slot = 0;
    while ( slot < SMUSHVEC_MAXBITMAPS && sv.aBitmaps[slot].pHeader )
    {
        slot++;
    }

    if ( slot == SMUSHVEC_MAXBITMAPS )
    {
        return -4;
    }

    int margin = SmushVec_BitmapMargin(width);
    int size   = width * height * 2;
    SmushVecBitmap* pEntry = &sv.aBitmaps[slot];
    pEntry->pHeader = (LecBitmapHeader*)calloc(1, sizeof(LecBitmapHeader));
    pEntry->pBlock  = (uint8_t*)malloc((size_t)(2 * margin + size));
    pEntry->id      = sv.nextBitmapId++;
    pEntry->size    = size;
    SmushVec_RandBytes(pEntry->pBlock, (size_t)margin, 0xB17B0000u + (uint32_t)pEntry->id);
    memset(pEntry->pBlock + margin, sv.fill, (size_t)size);
    SmushVec_RandBytes(pEntry->pBlock + margin + size, (size_t)margin, 0xB17C0000u + (uint32_t)pEntry->id);

    LecBitmapHeader* pBitmap = pEntry->pHeader;
    pBitmap->pPixels = pEntry->pBlock + margin;
    pBitmap->width   = width;
    pBitmap->height  = height;
    pBitmap->pitch   = width * 2;
    SmushVec_SetFormat(pBitmap, 1, 2, smushvec_fmt565);
    *ppBitmap = pBitmap;
    return 0;
}

static void SmushVec_BitmapDestroy(LecBitmapHeader** ppBitmap)
{
    for ( int i = 0; i < SMUSHVEC_MAXBITMAPS; ++i )
    {
        if ( sv.aBitmaps[i].pHeader && sv.aBitmaps[i].pHeader == *ppBitmap )
        {
            if ( sv.bitmapLogSize + 2 <= (int)sizeof(sv.aBitmapLog) )
            {
                sv.aBitmapLog[sv.bitmapLogSize++] = 'D';
                sv.aBitmapLog[sv.bitmapLogSize++] = (uint8_t)sv.aBitmaps[i].id;
            }

            free(sv.aBitmaps[i].pBlock);
            free(sv.aBitmaps[i].pHeader);
            memset(&sv.aBitmaps[i], 0, sizeof(sv.aBitmaps[i]));
            *ppBitmap = NULL;
            return;
        }
    }

    if ( sv.bitmapLogSize + 1 <= (int)sizeof(sv.aBitmapLog) )
    {
        sv.aBitmapLog[sv.bitmapLogSize++] = 'X';
    }

    *ppBitmap = NULL;
}

static int SmushVec_BitmapId(const void* pBitmap)
{
    for ( int i = 0; i < SMUSHVEC_MAXBITMAPS; ++i )
    {
        if ( sv.aBitmaps[i].pHeader && (const void*)sv.aBitmaps[i].pHeader == pBitmap )
        {
            return sv.aBitmaps[i].id;
        }
    }

    return -1;
}

/* Records the creates/destroys since the last call and the pixels of every live bitmap (by id) */
static void SmushVec_BitmapFlush(const char* pName)
{
    SmushVec_Raw(sv.aBitmapLog, (uint32_t)sv.bitmapLogSize, "%s: bitmap calls", pName);
    sv.bitmapLogSize = 0;
    for ( int id = 0; id < sv.nextBitmapId; ++id )
    {
        for ( int i = 0; i < SMUSHVEC_MAXBITMAPS; ++i )
        {
            if ( sv.aBitmaps[i].pHeader && sv.aBitmaps[i].id == id )
            {
                SmushVec_Bytes(sv.aBitmaps[i].pHeader->pPixels, (uint32_t)sv.aBitmaps[i].size, "%s: bitmap %d pixels", pName, id);
            }
        }
    }
}

/* --- Share table stub (only what the modules under test call) --- */
static int SmushVec_SharePublish(const char* pName, void* pInterface)
{
    if ( sv.numPub < SMUSHVEC_MAXPUB )
    {
        sv.aPub[sv.numPub].pName      = pName;
        sv.aPub[sv.numPub].pInterface = pInterface;
        sv.numPub++;
    }

    SmushVec_Log8('P');
    SmushVec_Log32((int)strlen(pName));
    return 0;
}

static void SmushVec_ShareUnpublish(const char* pName)
{
    SmushVec_Log8('U');
    SmushVec_Log32((int)strlen(pName));
}

static void* SmushVec_Published(const char* pName)
{
    for ( int i = sv.numPub - 1; i >= 0; --i )
    {
        if ( strcmp(sv.aPub[i].pName, pName) == 0 )
        {
            return sv.aPub[i].pInterface;
        }
    }

    return NULL;
}

static void SmushVec_ShareUnsubscribe(void* pInterface)
{
    (void)pInterface;
    SmushVec_Log8('u');
}

static void SmushVec_ShareError(const char* pFormat, ...)
{
    sv.numErrors++;
    SmushVec_Log8('E');
    size_t len = strlen(pFormat);
    if ( len >= sizeof(sv.aLastError) )
    {
        len = sizeof(sv.aLastError) - 1;
    }

    memcpy(sv.aLastError, pFormat, len);
    sv.aLastError[len] = 0;

    /* The frame stream reports the four tag characters */
    if ( strstr(pFormat, "%c%c%c%c") )
    {
        va_list args;
        va_start(args, pFormat);
        for ( int i = 0; i < 4; ++i )
        {
            SmushVec_Log32(va_arg(args, int));
        }

        va_end(args);
    }
}

static void* SmushVec_ShareSubscribe(const char* pName, void* pParam);
static int smushvec_numBlits;

static void SmushVec_Begin(const SmushVecApi* pApi, SmushVecSink* pSink, uint8_t fill)
{
    for ( int i = 0; i < SMUSHVEC_MAXBITMAPS; ++i )
    {
        if ( sv.aBitmaps[i].pHeader )
        {
            free(sv.aBitmaps[i].pBlock);
            free(sv.aBitmaps[i].pHeader);
        }
    }

    free(sv.pSection);
    memset(&sv, 0, sizeof(sv));
    sv.pApi  = pApi;
    sv.pSink = pSink;
    sv.fill  = fill;
    sv.memory.Alloc   = SmushVec_MemAlloc;
    sv.memory.Free    = SmushVec_MemFree;
    sv.memory.Realloc = SmushVec_MemRealloc;
    sv.bitmaps.Create  = SmushVec_BitmapCreate;
    sv.bitmaps.Destroy = SmushVec_BitmapDestroy;
    sv.share.Publish     = SmushVec_SharePublish;
    sv.share.Unpublish   = SmushVec_ShareUnpublish;
    sv.share.Subscribe   = SmushVec_ShareSubscribe;
    sv.share.Unsubscribe = SmushVec_ShareUnsubscribe;
    sv.share.ReportError = SmushVec_ShareError;
    smushvec_numBlits    = 0;
}

/* Releases what the stubs still hold after an area */
static void SmushVec_Finish(void)
{
    for ( int i = 0; i < SMUSHVEC_MAXBITMAPS; ++i )
    {
        if ( sv.aBitmaps[i].pHeader )
        {
            free(sv.aBitmaps[i].pBlock);
            free(sv.aBitmaps[i].pHeader);
            memset(&sv.aBitmaps[i], 0, sizeof(sv.aBitmaps[i]));
        }
    }

    free(sv.pSection);
    sv.pSection    = NULL;
    sv.sectionCap  = 0;
    sv.sectionSize = 0;
}

/* ------------------------------------------------------------------------------------------------------------------- */
/* Sound/Mixer.c                                                                                                       */
/* ------------------------------------------------------------------------------------------------------------------- */

/*
 * mixer_Setup builds the 17x17 volume/pan table with the CRT's sin and the 0x10000-byte clip table. The clip table is
 * the second allocation and is recorded directly; the static pan table is observed through mixer_Add: a mono 8-bit
 * sample of 0x81 adds 256 * gain to each channel's accumulator, so the two accumulators give two table entries exactly.
 */
static void SmushVec_RunMixer(const SmushVecApi* pApi, SmushVecSink* pSink, uint8_t fill)
{
    SmushVec_Begin(pApi, pSink, fill);

    /* Init sets the headroom (4) and asks for 2 channels of 256 * 4 frames: a 4096-byte accumulator (512 stereo frames) */
    SmushVec_AllocReset();
    SmushVec_Int(pApi->mixer_Init(&sv.memory, 4, 256, 0, 0, 0), "mixer_Init rc");
    SmushVec_AllocFlush("mixer_Init");
    int32_t* pAcc = (int32_t*)sv.allocs[0].p;
    const int16_t* pClip = (const int16_t*)sv.allocs[1].p;
    if ( !pAcc || !pClip )
    {
        return;
    }

    SmushVec_SectionBegin("mixer_Setup clip table");
    for ( int k = 0; k < 64; ++k )
    {
        SmushVec_HashRec(pClip + k * 1024, 2048, "mixer_Setup clip[%d..%d]", k * 1024, k * 1024 + 1023);
    }

    SmushVec_SectionEnd();

    // Every entry of the pan table: volume v (0-16) and pan p (-8..8) select mixer_pan[v][8 - p] and [8 + p]
    static uint8_t aOne[16] = { 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81 };
    // (stored as 17 x 17 pairs of 256 * left gain, 256 * right gain)
    int32_t aPan[17 * 17 * 2];
    for ( int v = 0; v <= 16; ++v )
    {
        for ( int p = -8; p <= 8; ++p )
        {
            int volume = v ? 8 * v - 1 : 0;
            int pan    = p <= 0 ? 8 * (p + 8) : 8 * (p + 7) + 1;
            pApi->mixer_Clear();
            pApi->mixer_Add(aOne, 4, 8, 1, 4, 0, volume, pan, 1, 0);
            memcpy(&aPan[(v * 17 + p + 8) * 2], pAcc, 8);
        }
    }

    SmushVec_Raw(aPan, sizeof(aPan), "mixer_Add pan table (v 0-16, p -8..8)");

    // The volume and pan mappings: every pan value at one volume, volumes beyond the table
    int32_t aSweep[128 * 2];
    for ( int pan = 0; pan < 128; ++pan )
    {
        pApi->mixer_Clear();
        pApi->mixer_Add(aOne, 4, 8, 1, 4, 0, 100, pan, 1, 0);
        memcpy(&aSweep[pan * 2], pAcc, 8);
    }

    SmushVec_Raw(aSweep, sizeof(aSweep), "mixer_Add volume 100 pan 0-127");

    SmushVec_SectionBegin("mixer_Add volumes");
    static const int aVolumes[] = { 1, 7, 8, 9, 120, 127, 128, 129, 200, 1000, -1, -100 };
    for ( size_t i = 0; i < sizeof(aVolumes) / sizeof(aVolumes[0]); ++i )
    {
        pApi->mixer_Clear();
        pApi->mixer_Add(aOne, 4, 8, 1, 4, 0, aVolumes[i], 64, 1, 0);
        SmushVec_Raw(pAcc, 8, "mixer_Add volume %d pan 64", aVolumes[i]);
        pApi->mixer_Clear();
        pApi->mixer_Add(aOne, 4, 8, 2, 4, 0, aVolumes[i], 64, 1, 0);
        SmushVec_Raw(pAcc, 8, "mixer_Add stereo volume %d", aVolumes[i]);
    }

    SmushVec_SectionEnd();

    // Every format through mixer_Feed, with resampling steps of 1/2 (the interpolating path), 1, 2 and odd ratios
    static const struct { int count, n, offset; } aRates[] = {
        { 64, 128, 0 }, { 64, 64, 0 }, { 128, 64, 0 }, { 50, 77, 3 }, { 77, 50, 5 }, { 1, 2, 0 }, { 1, 1, 7 }, { 255, 256, 1 }, { 300, 100, 0 },
    };
    static const struct { int volume, pan; } aMix[] = { { 127, 0 }, { 64, -127 }, { 100, 127 }, { 0, 30 }, { 127, -1 } };
    uint8_t aSrc[1600];
    int16_t aOut[1024];
    for ( int format = 0; format < 6; ++format )
    {
        int sampleSize = (format >= 2 ? 2 : 1) * ((format & 1) ? 2 : 1);
        SmushVec_SectionBegin("mixer_Feed format %d", format);
        for ( size_t r = 0; r < sizeof(aRates) / sizeof(aRates[0]); ++r )
        {
            for ( size_t m = 0; m < sizeof(aMix) / sizeof(aMix[0]); ++m )
            {
                SmushVec_RandBytes(aSrc, sizeof(aSrc), 0x3E000000u + (uint32_t)(format * 1000 + r * 10 + m));
                pApi->mixer_Clear();
                pApi->mixer_Feed(aSrc, aRates[r].offset, aRates[r].n, aRates[r].count * sampleSize, 0, aMix[m].volume, aMix[m].pan, format);
                SmushVec_HashRec(pAcc, 4096, "mixer_Feed format %d count %d n %d offset %d volume %d pan %d", format, aRates[r].count, aRates[r].n,
                    aRates[r].offset, aMix[m].volume, aMix[m].pan);
            }
        }

        SmushVec_SectionEnd();
    }

    // The byte-swapped 16-bit paths (mixer_Feed never selects them)
    SmushVec_SectionBegin("mixer_Add swapped");
    for ( int channels = 1; channels <= 2; ++channels )
    {
        for ( size_t r = 0; r < sizeof(aRates) / sizeof(aRates[0]); ++r )
        {
            SmushVec_RandBytes(aSrc, sizeof(aSrc), 0x5A000000u + (uint32_t)(channels * 100 + r));
            pApi->mixer_Clear();
            pApi->mixer_Add(aSrc, (uint32_t)aRates[r].count, 16, channels, (uint32_t)aRates[r].n, aRates[r].offset, 90, 40, 1, 1);
            SmushVec_HashRec(pAcc, 4096, "mixer_Add swapped channels %d count %d n %d", channels, aRates[r].count, aRates[r].n);
        }
    }

    SmushVec_SectionEnd();

    // Ignored inputs: no source, no samples
    pApi->mixer_Clear();
    pApi->mixer_Add(NULL, 10, 8, 1, 10, 0, 100, 64, 1, 0);
    pApi->mixer_Add(aSrc, 0, 8, 1, 10, 0, 100, 64, 1, 0);
    SmushVec_HashRec(pAcc, 4096, "mixer_Add null source and zero count");

    // Output: several loud feeds summed and clipped, then converted with the 7-bit headroom shift
    pApi->mixer_Clear();
    for ( int k = 0; k < 6; ++k )
    {
        SmushVec_RandBytes(aSrc, sizeof(aSrc), 0x0C000000u + (uint32_t)k);
        pApi->mixer_Feed(aSrc, k, 256, 512 * ((k & 1) + 1), 0, 127, (k - 3) * 40, k < 3 ? 4 : 5);
    }

    memset(aOut, 0xEE, sizeof(aOut));
    SmushVec_Int(pApi->mixer_Output(aOut, 300), "mixer_Output rc");
    SmushVec_HashRec(aOut, sizeof(aOut), "mixer_Output stereo 300");
    SmushVec_Int(pApi->mixer_Output(aOut, 0), "mixer_Output zero rc");
    SmushVec_Int(pApi->mixer_Output(NULL, 4), "mixer_Output null rc");

    // A mono mixer: Output converts n samples instead of 2n
    SmushVec_Int(pApi->mixer_Free(&sv.memory), "mixer_Free rc");
    SmushVec_AllocFlush("mixer_Free");
    SmushVec_AllocReset();
    SmushVec_Int(pApi->mixer_Setup(&sv.memory, 1, 1024, 5), "mixer_Setup mono rc");
    SmushVec_AllocFlush("mixer_Setup mono");
    pAcc = (int32_t*)sv.allocs[0].p;
    if ( !pAcc )
    {
        return;
    }

    uint32_t accSeed = 0x77u;
    for ( int i = 0; i < 256; ++i )
    {
        pAcc[i] = (int32_t)SmushVec_Lcg(&accSeed) >> (i & 15);
    }

    memset(aOut, 0xEE, sizeof(aOut));
    SmushVec_Int(pApi->mixer_Output(aOut, 200), "mixer_Output mono rc");
    SmushVec_HashRec(aOut, sizeof(aOut), "mixer_Output mono 200");

    // Allocation failures: the accumulator, then the clip table
    SmushVec_Int(pApi->mixer_Free(&sv.memory), "mixer_Free mono rc");
    SmushVec_AllocFlush("mixer_Free mono");
    SmushVec_AllocReset();
    sv.failAlloc = 1;
    SmushVec_Int(pApi->mixer_Setup(&sv.memory, 2, 64, 0), "mixer_Setup fail 1 rc");
    SmushVec_Int(pApi->mixer_Free(&sv.memory), "mixer_Free after fail 1 rc");
    SmushVec_AllocFlush("mixer_Setup fail 1");
    SmushVec_AllocReset();
    sv.failAlloc = 2;
    SmushVec_Int(pApi->mixer_Setup(&sv.memory, 2, 64, 0), "mixer_Setup fail 2 rc");
    SmushVec_Int(pApi->mixer_Output(aOut, 4), "mixer_Output after fail 2 rc");
    SmushVec_Int(pApi->mixer_Free(&sv.memory), "mixer_Free after fail 2 rc");
    SmushVec_AllocFlush("mixer_Setup fail 2");
    SmushVec_Int(pApi->mixer_Output(aOut, 4), "mixer_Output after free rc");
    sv.failAlloc = 0;
}

/* ------------------------------------------------------------------------------------------------------------------- */
/* Flobject/D_bloc16.c                                                                                                 */
/* ------------------------------------------------------------------------------------------------------------------- */

static void* SmushVec_ShareSubscribe(const char* pName, void* pParam);

/* Codec 2 opcode streams: the grammar of the assembly vectors (Bloc16Asm.tv), with motion vectors kept within the margins */
typedef struct sSmushVecStream
{
    uint8_t* p;
    int size;
    int cap;
} SmushVecStream;

static void SmushVec_S8(SmushVecStream* s, uint32_t v)
{
    if ( s->size + 1 > s->cap )
    {
        s->cap = s->cap * 2 + 256;
        s->p   = (uint8_t*)realloc(s->p, (size_t)s->cap);
    }

    s->p[s->size++] = (uint8_t)v;
}

static void SmushVec_S16(SmushVecStream* s, uint32_t v)
{
    SmushVec_S8(s, v);
    SmushVec_S8(s, v >> 8);
}

static void SmushVec_S32(SmushVecStream* s, uint32_t v)
{
    SmushVec_S16(s, v);
    SmushVec_S16(s, v >> 16);
}

static uint32_t SmushVec_Rnd(uint32_t* r)
{
    return SmushVec_Lcg(r) >> 8;
}

/* Two draws, in this order (never two draws in one expression: the evaluation order is unspecified) */
static uint32_t SmushVec_Rnd32(uint32_t* r)
{
    uint32_t lo = SmushVec_Rnd(r);
    uint32_t hi = SmushVec_Rnd(r);
    return lo ^ (hi << 16);
}

static void SmushVec_Codec2Block(SmushVecStream* s, int level, uint32_t* r, int mv)
{
    // Weights: motion table, F5, F6, F7, F8, F9-FC, FD, FE, FF (subdivide)
    static const int aWeights[] = { 6, 3, 3, 3, 3, 1, 1, 1, 1, 2, 2, 6 };
    int x = (int)(SmushVec_Rnd(r) % 32u);
    int kind = 0;
    while ( x >= aWeights[kind] )
    {
        x -= aWeights[kind];
        kind++;
    }

    switch ( kind )
    {
        case 0: SmushVec_S8(s, SmushVec_Rnd(r) % 0xF5); break;
        case 1: SmushVec_S8(s, 0xF5); SmushVec_S16(s, (uint32_t)((int)(SmushVec_Rnd(r) % (uint32_t)(2 * mv + 1)) - mv)); break;
        case 2: SmushVec_S8(s, 0xF6); break;
        case 3:
            SmushVec_S8(s, 0xF7);
            if ( level == 2 )
            {
                SmushVec_S32(s, SmushVec_Rnd32(r));
            }
            else
            {
                SmushVec_S8(s, SmushVec_Rnd(r));
                SmushVec_S8(s, SmushVec_Rnd(r));
                SmushVec_S8(s, SmushVec_Rnd(r));
            }
            break;

        case 4:
            SmushVec_S8(s, 0xF8);
            if ( level == 2 )
            {
                SmushVec_S32(s, SmushVec_Rnd32(r));
                SmushVec_S32(s, SmushVec_Rnd32(r));
            }
            else
            {
                SmushVec_S8(s, SmushVec_Rnd(r));
                SmushVec_S32(s, SmushVec_Rnd32(r));
            }
            break;

        case 5: case 6: case 7: case 8: SmushVec_S8(s, 0xF9 + (uint32_t)(kind - 5)); break;
        case 9: SmushVec_S8(s, 0xFD); SmushVec_S8(s, SmushVec_Rnd(r)); break;
        case 10: SmushVec_S8(s, 0xFE); SmushVec_S16(s, SmushVec_Rnd(r)); break;
        default:
            SmushVec_S8(s, 0xFF);
            if ( level == 2 )
            {
                SmushVec_S32(s, SmushVec_Rnd32(r));
                SmushVec_S32(s, SmushVec_Rnd32(r));
            }
            else
            {
                for ( int q = 0; q < 4; ++q )
                {
                    SmushVec_Codec2Block(s, level / 2, r, mv);
                }
            }
            break;
    }
}

/* RLE streams of decode_RleByte: (length - 1) << 1 | repeat, then one byte (repeat) or length bytes (literal) */
static void SmushVec_Rle(SmushVecStream* s, int size, uint32_t seed)
{
    uint32_t r = seed;
    while ( size > 0 )
    {
        uint32_t range = SmushVec_Rnd(&r) % 4 == 0 ? 128u : 8u;
        int len = 1 + (int)(SmushVec_Rnd(&r) % range);
        if ( len > size )
        {
            len = size;
        }

        if ( SmushVec_Rnd(&r) & 1 )
        {
            SmushVec_S8(s, (uint32_t)(((len - 1) << 1) | 1));
            SmushVec_S8(s, SmushVec_Rnd(&r));
        }
        else
        {
            SmushVec_S8(s, (uint32_t)((len - 1) << 1));
            for ( int i = 0; i < len; ++i )
            {
                SmushVec_S8(s, SmushVec_Rnd(&r));
            }
        }

        size -= len;
    }
}

static int SmushVec_Blocks(int v)
{
    int b = (uint8_t)((uint32_t)(v + 7) >> 3);
    return b ? b : 256;
}

typedef struct sSmushVecFrame
{
    int seq;
    int codec;
    int rotate;
    int flags;
    int width;      /* 0: the scenario's */
    int height;
    int depth;
    int noBitmap;   /* decode without a destination bitmap */
} SmushVecFrame;

static Bl16Header* SmushVec_MakeFrame(const SmushVecFrame* pFrame, int width, int height, uint32_t seed, int* pSize)
{
    SmushVecStream s = { 0 };
    uint32_t r = seed;
    switch ( pFrame->codec )
    {
        case 0:
            for ( int i = 0; i < width * height * 2; ++i ) SmushVec_S8(&s, SmushVec_Rnd(&r));
            break;

        case 1:
            for ( int i = 0; i < (width / 2 + 2) * (height / 2 + 2) * 2; ++i ) SmushVec_S8(&s, SmushVec_Rnd(&r));
            break;

        case 2:
        {
            int mv = 2 * width + 8;
            int blocks = SmushVec_Blocks(width) * SmushVec_Blocks(height);
            for ( int i = 0; i < blocks; ++i ) SmushVec_Codec2Block(&s, 8, &r, mv);
        } break;

        case 5:
            SmushVec_Rle(&s, width * height * 2 - 6, seed);
            break;

        case 6:
            for ( int i = 0; i < width * height; ++i ) SmushVec_S8(&s, SmushVec_Rnd(&r));
            break;

        case 7:
            for ( int i = 0; i < (width / 2 + 2) * (height / 2 + 2); ++i ) SmushVec_S8(&s, SmushVec_Rnd(&r));
            break;

        case 8:
            SmushVec_Rle(&s, width * height, seed);
            break;

        default:
            break;
    }

    int size = (int)offsetof(Bl16Header, data) + s.size + 64;
    Bl16Header* pHdr = (Bl16Header*)calloc(1, (size_t)size);
    SmushVec_RandBytes(pHdr, offsetof(Bl16Header, data), seed ^ 0x48445221u);
    pHdr->x       = (int16_t)(seed % 7);
    pHdr->y       = (int16_t)(seed % 5);
    pHdr->width   = (int16_t)width;
    pHdr->height  = (int16_t)height;
    pHdr->seq     = (int16_t)pFrame->seq;
    pHdr->codec   = (uint8_t)pFrame->codec;
    pHdr->rotate  = (uint8_t)pFrame->rotate;
    pHdr->rleSize = width * height * 2 - 6;
    pHdr->depth   = (int16_t)pFrame->depth;
    if ( s.size )
    {
        memcpy(pHdr->data, s.p, (size_t)s.size);
    }

    free(s.p);
    *pSize = size;
    return pHdr;
}

static void* smushvec_pPalette;
static void* smushvec_pDst;

static int SmushVec_Blit(LecBitmapHeader* pDst, MixerRect* pClip, int x, int y, LecBitmapHeader* pSrc, void* pPalette, void* pTable)
{
    int aInfo[9];
    aInfo[0] = SmushVec_BitmapId(pSrc);
    aInfo[1] = pDst == smushvec_pDst;
    aInfo[2] = pPalette == smushvec_pPalette;
    aInfo[3] = x;
    aInfo[4] = y;
    aInfo[5] = pClip ? pClip->x : -9999;
    aInfo[6] = pClip ? pClip->y : -9999;
    aInfo[7] = pClip ? pClip->width : -9999;
    aInfo[8] = pTable != NULL;
    SmushVec_Raw(aInfo, sizeof(aInfo), "blit %d", smushvec_numBlits);
    if ( pTable )
    {
        SmushVec_HashRec(pTable, 0x20000, "blit %d table", smushvec_numBlits);
    }

    smushvec_numBlits++;
    return 0x40 + smushvec_numBlits;
}

typedef struct sSmushVecScene
{
    const char* pName;
    int width;
    int height;
    int useMixerBlit;   /* blit with mixer_Blit instead of the callback */
    int dstColorMode;
    int dstPixelSize;
    const int* pDstFormat;
    int useClip;
    const SmushVecFrame* pFrames;
    int numFrames;
} SmushVecScene;

static void SmushVec_RunScene(const SmushVecScene* pScene)
{
    const SmushVecApi* pApi = sv.pApi;
    char aName[96];

    pApi->dbloc16_Configure(1, pScene->useMixerBlit ? NULL : SmushVec_Blit, 0);

    // The destination bitmap, larger than the frames, with seeded pixels
    LecBitmapHeader dst;
    memset(&dst, 0, sizeof(dst));
    int dstW = pScene->width + 12, dstH = pScene->height + 10;
    SmushVec_SetFormat(&dst, pScene->dstColorMode, pScene->dstPixelSize, pScene->pDstFormat);
    dst.width  = dstW;
    dst.height = dstH;
    dst.pitch  = dstW * pScene->dstPixelSize + 4;
    uint8_t* pDstPixels = (uint8_t*)malloc((size_t)(dst.pitch * dstH));
    SmushVec_RandBytes(pDstPixels, (size_t)(dst.pitch * dstH), 0xD5700000u);
    dst.pPixels = pDstPixels;
    smushvec_pDst = &dst;

    uint16_t aPalette[256];
    SmushVec_RandBytes(aPalette, sizeof(aPalette), 0x9A1E7700u);
    smushvec_pPalette = aPalette;
    MixerRect clip = { 3, 2, pScene->width, pScene->height + 3 };

    for ( int i = 0; i < pScene->numFrames; ++i )
    {
        const SmushVecFrame* pFrame = &pScene->pFrames[i];
        int width  = pFrame->width ? pFrame->width : pScene->width;
        int height = pFrame->height ? pFrame->height : pScene->height;
        int size;
        Bl16Header* pHdr = SmushVec_MakeFrame(pFrame, width, height, 0xF0000000u + (uint32_t)(i * 7919) + (uint32_t)pScene->width, &size);
        snprintf(aName, sizeof(aName), "%s frame %d seq %d codec %d rotate %d", pScene->pName, i, pFrame->seq, pFrame->codec, pFrame->rotate);

        SmushVec_SectionBegin("%s", aName);
        int blitsBefore = smushvec_numBlits;
        int rc = pApi->dbloc16_DecodeFrame(pFrame->noBitmap ? NULL : &dst, pScene->useClip ? &clip : NULL, 2, 1, aPalette, pHdr, pFrame->flags);

        // rc is the blit callback's, 1 after mixer_Blit or 0 for an early return; otherwise the original leaves it
        // uninitialized and it is not recorded
        int early = (pFrame->flags & 4) && !pFrame->rotate && pFrame->codec >= 2 && pFrame->codec <= 4;
        if ( smushvec_numBlits != blitsBefore || early || (pScene->useMixerBlit && !pFrame->noBitmap) )
        {
            SmushVec_Int(rc, "%s: rc", aName);
        }

        SmushVec_BitmapFlush(aName);
        if ( pScene->useMixerBlit )
        {
            SmushVec_Bytes(pDstPixels, (uint32_t)(dst.pitch * dstH), "%s: destination", aName);
        }

        SmushVec_AllocFlush(aName);
        SmushVec_SectionEnd();
        free(pHdr);
    }

    free(pDstPixels);
    smushvec_pDst = NULL;
}

static const SmushVecFrame smushvec_framesAll[] = {
    { 0, 0, 0, 0 },  { 1, 2, 1, 0 },  { 2, 2, 2, 0 },  { 3, 3, 0, 0 },  { 4, 4, 1, 0 },  { 5, 5, 2, 0 },
    { 6, 6, 0, 0 },  { 7, 1, 1, 0 },  { 8, 7, 2, 0 },  { 9, 8, 0, 0 },  { 10, 2, 2, 0 },
    { 12, 2, 1, 0 }, // out of sequence: codec 2 skips the frame, no rotation
    { 13, 2, 1, 0 }, // in sequence again
    { 14, 2, 0, 4 }, // flags 4 without rotation: early return
    { 15, 3, 1, 4 }, // flags 4 with rotation: decoded
    { 16, 4, 0, 4 }, { 17, 3, 0, 4 },
    { 18, 9, 1, 0 }, // unknown codec: the last decoded buffer is blitted again
    { 19, 2, 2, 0, 0, 0, 0, 1 }, // no destination bitmap
    { 0, 2, 0, 0 },  // sequence 0 again: tables rebuilt, delta buffers refilled, then codec 2 in sequence
    { 1, 2, 3, 0 },  // rotate 3: no rotation
    { 2, 2, 1, 0 },
};

static const SmushVecFrame smushvec_framesOdd[] = {
    { 0, 0, 0, 0 }, { 1, 2, 2, 0 }, { 2, 2, 1, 0 }, { 3, 5, 0, 0 }, { 4, 2, 2, 0 }, { 5, 8, 1, 0 }, { 6, 2, 2, 0 },
};

static const SmushVecFrame smushvec_framesResize[] = {
    { 0, 0, 0, 0 },
    { 1, 2, 1, 0 },
    { 0, 6, 0, 0, 32, 8 },      // wider: the buffers are reallocated
    { 1, 2, 2, 0, 32, 8 },
    { 0, 0, 0, 0, 32, 5, 0 },   // height 5 rounds up to the allocated 8: no reallocation
    { 1, 2, 1, 0, 32, 5 },
    { 0, 1, 0, 0, 16, 16, 16 },  // smaller again
};

static const SmushVecFrame smushvec_framesMixer[] = {
    { 0, 0, 0, 0 }, { 1, 2, 1, 0 }, { 2, 6, 2, 0 }, { 3, 2, 2, 0 }, { 4, 7, 0, 0 }, { 5, 2, 1, 0 },
};

static void SmushVec_RunBloc16(const SmushVecApi* pApi, SmushVecSink* pSink, uint8_t fill)
{
    SmushVec_Begin(pApi, pSink, fill);

    // Glyph and motion tables from a cleared state (MakeTables keeps the tables for the width it last built them for)
    memset(pApi->pOffsets, 0, 256 * sizeof(int16_t));
    memset(pApi->pSmall, 0, BL16_NUMTABLES * sizeof(Bl16Small));
    memset(pApi->pBig, 0, BL16_NUMTABLES * sizeof(Bl16Big));
    pApi->dbloc16_MakeTables(-12345);
    pApi->dbloc16_MakeGlyphs(4);
    SmushVec_SectionBegin("dbloc16_MakeGlyphs 4");
    for ( int k = 0; k < 16; ++k )
    {
        SmushVec_HashRec(pApi->pSmall + k * 16, 16 * sizeof(Bl16Small), "dbloc16_MakeGlyphs 4: small[%d..%d]", k * 16, k * 16 + 15);
    }

    SmushVec_SectionEnd();
    pApi->dbloc16_MakeGlyphs(8);
    SmushVec_SectionBegin("dbloc16_MakeGlyphs 8");
    for ( int k = 0; k < 16; ++k )
    {
        SmushVec_HashRec(pApi->pBig + k * 16, 16 * sizeof(Bl16Big), "dbloc16_MakeGlyphs 8: big[%d..%d]", k * 16, k * 16 + 15);
    }

    SmushVec_HashRec(pApi->pSmall, BL16_NUMTABLES * sizeof(Bl16Small), "dbloc16_MakeGlyphs 8: small unchanged");
    SmushVec_SectionEnd();

    static const int aWidths[] = { 8, 16, 24, 320, 640, 1, 0, 3, 13, 1000, 2049, 4097, 65544, -8, 16 };
    for ( size_t i = 0; i < sizeof(aWidths) / sizeof(aWidths[0]); ++i )
    {
        pApi->dbloc16_MakeTables(aWidths[i]);
        SmushVec_SectionBegin("dbloc16_MakeTables %d", aWidths[i]);
        SmushVec_Raw(pApi->pOffsets, 256 * sizeof(int16_t), "dbloc16_MakeTables %d: offsets", aWidths[i]);
        SmushVec_HashRec(pApi->pSmall, BL16_NUMTABLES * sizeof(Bl16Small), "dbloc16_MakeTables %d: small", aWidths[i]);
        SmushVec_HashRec(pApi->pBig, BL16_NUMTABLES * sizeof(Bl16Big), "dbloc16_MakeTables %d: big", aWidths[i]);
        SmushVec_SectionEnd();
    }

    // The same width again keeps the (here cleared) offsets
    memset(pApi->pOffsets, 0, 256 * sizeof(int16_t));
    pApi->dbloc16_MakeTables(16);
    SmushVec_Raw(pApi->pOffsets, 256 * sizeof(int16_t), "dbloc16_MakeTables 16 again: offsets");
    pApi->dbloc16_MakeTables(-12345);

    // Average: special values and random pairs
    static const uint16_t aSpecial[] = { 0x0000, 0xFFFF, 0x8000, 0x7FFF, 0x07E0, 0xF81F, 0x001F, 0xF800, 0x0821, 0xF7DE, 0x1234, 0xFEDC };
    uint16_t aAvg[4096];
    int n = 0;
    for ( size_t a = 0; a < sizeof(aSpecial) / sizeof(aSpecial[0]); ++a )
    {
        for ( size_t b = 0; b < sizeof(aSpecial) / sizeof(aSpecial[0]); ++b )
        {
            aAvg[n++] = pApi->dbloc16_Average(aSpecial[a], aSpecial[b]);
        }
    }

    SmushVec_Raw(aAvg, (uint32_t)(n * sizeof(uint16_t)), "dbloc16_Average special values");
    uint32_t r = 0xA7E5A6Eu;
    for ( int i = 0; i < 4096; ++i )
    {
        uint32_t v = SmushVec_Lcg(&r);
        aAvg[i] = pApi->dbloc16_Average((uint16_t)v, (uint16_t)(v >> 16));
    }

    SmushVec_HashRec(aAvg, sizeof(aAvg), "dbloc16_Average random pairs");

    // Scale and ScalePal: even, odd and degenerate sizes, into a seeded buffer with room around the image
    static const struct { int w, h; } aSizes[] = {
        { 8, 4 }, { 16, 8 }, { 2, 2 }, { 4, 1 }, { 6, 3 }, { 7, 5 }, { 320, 16 }, { 1, 1 }, { 0, 4 }, { 8, 0 }, { 3, 2 }, { 640, 6 }, { 9, 9 },
    };
    uint16_t aPal[256];
    SmushVec_RandBytes(aPal, sizeof(aPal), 0x5CA1E000u);
    for ( size_t i = 0; i < sizeof(aSizes) / sizeof(aSizes[0]); ++i )
    {
        int w = aSizes[i].w, h = aSizes[i].h;
        size_t dstCount = (size_t)((h + 4) * (w + 4) + 64);
        size_t srcCount = (size_t)(w * h + 64);
        uint16_t* pDstBuf = (uint16_t*)malloc(dstCount * 2);
        uint16_t* pSrcBuf = (uint16_t*)malloc(srcCount * 2);
        SmushVec_SectionBegin("dbloc16_Scale and ScalePal %dx%d", w, h);
        for ( int pal = 0; pal < 2; ++pal )
        {
            SmushVec_RandBytes(pDstBuf, dstCount * 2, 0xD0000000u + (uint32_t)i);
            SmushVec_RandBytes(pSrcBuf, srcCount * 2, 0x50000000u + (uint32_t)i);
            if ( pal )
            {
                pApi->dbloc16_ScalePal(pDstBuf + 32, (uint8_t*)pSrcBuf, w, h, aPal);
            }
            else
            {
                pApi->dbloc16_Scale(pDstBuf + 32, pSrcBuf, w, h);
            }

            SmushVec_Bytes(pDstBuf, (uint32_t)(dstCount * 2), "dbloc16_%s %dx%d", pal ? "ScalePal" : "Scale", w, h);
        }

        SmushVec_SectionEnd();
        free(pDstBuf);
        free(pSrcBuf);
    }

    // DecodeFrame through the module's own services
    SmushVec_Int(pApi->dbloc16_Register(&sv.share, (void*)"param"), "dbloc16_Register rc");
    SmushVec_Int(pApi->dbloc16_Startup(), "dbloc16_Startup rc");
    SmushVec_LogFlush("dbloc16 register and startup");

    static const SmushVecScene aScenes[] = {
        { "all codecs 16x16", 16, 16, 0, 1, 2, smushvec_fmt565, 0, smushvec_framesAll, sizeof(smushvec_framesAll) / sizeof(smushvec_framesAll[0]) },
        { "555 lut 24x13", 24, 13, 0, 1, 2, smushvec_fmt555, 1, smushvec_framesAll, 12 },
        { "565B lut 40x16", 40, 16, 0, 1, 2, smushvec_fmt565B, 0, smushvec_framesOdd, sizeof(smushvec_framesOdd) / sizeof(smushvec_framesOdd[0]) },
        { "odd width 20x8", 20, 8, 0, 1, 2, smushvec_fmt565, 0, smushvec_framesOdd, sizeof(smushvec_framesOdd) / sizeof(smushvec_framesOdd[0]) },
        { "resize 16x8", 16, 8, 0, 1, 2, smushvec_fmt565, 0, smushvec_framesResize, sizeof(smushvec_framesResize) / sizeof(smushvec_framesResize[0]) },
        { "mixer blit 565 24x16", 24, 16, 1, 1, 2, smushvec_fmt565, 1, smushvec_framesMixer, sizeof(smushvec_framesMixer) / sizeof(smushvec_framesMixer[0]) },
        { "mixer blit RGBA 16x8", 16, 8, 1, 2, 4, smushvec_fmtRGBA, 1, smushvec_framesMixer, sizeof(smushvec_framesMixer) / sizeof(smushvec_framesMixer[0]) },
        { "mixer blit 555 16x8", 16, 8, 1, 1, 2, smushvec_fmt555, 0, smushvec_framesMixer, sizeof(smushvec_framesMixer) / sizeof(smushvec_framesMixer[0]) },
    };

    for ( size_t i = 0; i < sizeof(aScenes) / sizeof(aScenes[0]); ++i )
    {
        SmushVec_RunScene(&aScenes[i]);
    }

    // Parse through the published interface: depth changes and allocation failures (bitmap 2, then the lookup table)
    Bl16Interface* pInterface = (Bl16Interface*)SmushVec_Published("FLOBDEC=insane_blocky16");
    if ( pInterface )
    {
        Bl16Parse parse = { 2, 16, 9, 16 };
        SmushVec_Int(pInterface->Parse(&parse, 8), "dbloc16 Parse 16x9 rc");
        SmushVec_BitmapFlush("dbloc16 Parse 16x9");
        parse.depth = 8;
        SmushVec_Int(pInterface->Parse(&parse, 8), "dbloc16 Parse depth 8 rc");
        SmushVec_BitmapFlush("dbloc16 Parse depth 8");
        parse.width = 24;
        sv.failCreate = 2;
        SmushVec_Int(pInterface->Parse(&parse, 8), "dbloc16 Parse create fails rc");
        SmushVec_BitmapFlush("dbloc16 Parse create fails");
        sv.failCreate = 0;
        SmushVec_AllocReset();
        sv.failAlloc = 1;
        parse.width = 32;
        SmushVec_Int(pInterface->Parse(&parse, 8), "dbloc16 Parse lut fails rc");
        SmushVec_BitmapFlush("dbloc16 Parse lut fails");
        SmushVec_AllocFlush("dbloc16 Parse lut fails");
        sv.failAlloc = 0;
        SmushVec_Int(pInterface->Parse(&parse, 8), "dbloc16 Parse again rc");
        SmushVec_BitmapFlush("dbloc16 Parse again");

        int x = -1, y = -1, w = -1, h = -1;
        SmushVecFrame frame = { 3, 0, 0, 0 };
        int size;
        Bl16Header* pHdr = SmushVec_MakeFrame(&frame, 8, 8, 77, &size);
        SmushVec_Int(pInterface->Query(pHdr, 0x100, &x, &y, &w, &h), "dbloc16 Query rc");
        int aQuery[4] = { x, y, w, h };
        SmushVec_Raw(aQuery, sizeof(aQuery), "dbloc16 Query result");
        SmushVec_Int(pInterface->Query(pHdr, 0x101, &x, &y, &w, &h), "dbloc16 Query other tag rc");
        SmushVec_Int(pInterface->Query(pHdr, 0x100, NULL, NULL, NULL, NULL), "dbloc16 Query null rc");
        free(pHdr);
        SmushVec_Int(pInterface->GetTag(), "dbloc16 GetTag");
        for ( int i = -1; i < 4; ++i )
        {
            SmushVec_Str(pInterface->Name(i), "dbloc16 Name %d", i);
        }
    }

    // Configure: get the blit callback (one of ours or none), an unknown tag
    Bl16Blit pBlit = (Bl16Blit)(intptr_t)1;
    SmushVec_Int(pApi->dbloc16_Configure(1, SmushVec_Blit, 2, &pBlit, 0), "dbloc16_Configure rc");
    SmushVec_Int(pBlit == SmushVec_Blit, "dbloc16_Configure get blit");
    SmushVec_Int(pApi->dbloc16_Configure(7, 0), "dbloc16_Configure unknown tag rc");
    pApi->dbloc16_Configure(1, NULL, 0);

    pApi->dbloc16_Shutdown();
    SmushVec_BitmapFlush("dbloc16_Shutdown");
    pApi->dbloc16_Unregister();
    SmushVec_LogFlush("dbloc16 shutdown and unregister");
    pApi->dbloc16_MakeTables(-12345);
}

/* ------------------------------------------------------------------------------------------------------------------- */
/* Decode/Decode.c                                                                                                     */
/* ------------------------------------------------------------------------------------------------------------------- */

typedef struct sSmushVecFormat
{
    const char* pName;
    int colorMode;
    int pixelSize;
    const int* pFormat;
} SmushVecFormat;

static const SmushVecFormat smushvec_formats[] = {
    { "indexed", 0, 1, NULL },
    { "555", 1, 2, smushvec_fmt555 },
    { "565", 1, 2, smushvec_fmt565 },
    { "565B", 1, 2, smushvec_fmt565B },
    { "444", 1, 2, smushvec_fmt444 },
    { "888", 1, 3, smushvec_fmt888 },
    { "x888", 1, 4, smushvec_fmt888 },
    { "RGBA", 2, 4, smushvec_fmtRGBA },
    { "BGRA", 2, 4, smushvec_fmtBGRA },
    { "4444", 2, 2, smushvec_fmt4444 },
    { "1555", 2, 2, smushvec_fmt1555 },
    { "a888", 2, 3, smushvec_fmtBGRA },
    { "332", 1, 1, NULL },
};

static void SmushVec_MakeBitmap(LecBitmapHeader* pBitmap, const SmushVecFormat* pFormat, void* pPalette)
{
    static const int fmt332[12] = { 3, 3, 2, 5, 2, 0, 5, 5, 6, 0, 0, 0 };
    memset(pBitmap, 0, sizeof(*pBitmap));
    SmushVec_SetFormat(pBitmap, pFormat->colorMode, pFormat->pixelSize, pFormat->pFormat ? pFormat->pFormat : (pFormat->colorMode ? fmt332 : NULL));
    if ( pFormat->colorMode == 0 )
    {
        pBitmap->indexed.pPalette = pPalette;
        pBitmap->indexed.nColors  = 256;
    }
}

static void SmushVec_RunDecode(const SmushVecApi* pApi, SmushVecSink* pSink, uint8_t fill)
{
    SmushVec_Begin(pApi, pSink, fill);
    const int numFormats = (int)(sizeof(smushvec_formats) / sizeof(smushvec_formats[0]));

    uint8_t aPalette[1024];
    SmushVec_RandBytes(aPalette, sizeof(aPalette), 0xFA1E7000u);
    uint8_t aSrc[0x1400 * 4 + 64];
    uint8_t aDst[0x1400 * 4 + 64];
    static uint8_t aTable[0x40000 + 64];
    LecBitmapHeader src, dst;

    // Every source/destination format pair: decode_Convert without a table (copy, direct or through RGBA)
    for ( int s = 0; s < numFormats; ++s )
    {
        SmushVec_SectionBegin("decode_Convert from %s", smushvec_formats[s].pName);
        for ( int d = 0; d < numFormats; ++d )
        {
            SmushVec_MakeBitmap(&src, &smushvec_formats[s], NULL);
            SmushVec_MakeBitmap(&dst, &smushvec_formats[d], NULL);
            SmushVec_RandBytes(aSrc, 37 * 4 + 16, 0xC0000000u + (uint32_t)(s * 32 + d));
            memset(aDst, 0xEE, 37 * 4 + 16);
            int rc = pApi->decode_Convert(aSrc, &src, 37, aPalette, aDst, &dst, NULL);
            SmushVec_Int(rc, "decode_Convert %s to %s rc", smushvec_formats[s].pName, smushvec_formats[d].pName);
            SmushVec_Raw(aDst, 37 * 4 + 16, "decode_Convert %s to %s", smushvec_formats[s].pName, smushvec_formats[d].pName);
        }

        SmushVec_SectionEnd();
    }

    // More pixels than one 0x500-pixel chunk; the indexed bitmap's own palette, no palette at all
    static const int aBig[][2] = { { 2, 1 }, { 1, 7 }, { 7, 3 }, { 0, 2 }, { 5, 6 } };
    for ( size_t i = 0; i < sizeof(aBig) / sizeof(aBig[0]); ++i )
    {
        SmushVec_MakeBitmap(&src, &smushvec_formats[aBig[i][0]], aPalette);
        SmushVec_MakeBitmap(&dst, &smushvec_formats[aBig[i][1]], NULL);
        SmushVec_RandBytes(aSrc, sizeof(aSrc), 0xB1600000u + (uint32_t)i);
        memset(aDst, 0xEE, sizeof(aDst));
        int rc = pApi->decode_Convert(aSrc, &src, 0x1003, NULL, aDst, &dst, NULL);
        SmushVec_Int(rc, "decode_Convert 0x1003 %s to %s rc", smushvec_formats[aBig[i][0]].pName, smushvec_formats[aBig[i][1]].pName);
        SmushVec_HashRec(aDst, sizeof(aDst), "decode_Convert 0x1003 %s to %s", smushvec_formats[aBig[i][0]].pName, smushvec_formats[aBig[i][1]].pName);
    }

    SmushVec_MakeBitmap(&src, &smushvec_formats[0], NULL);
    SmushVec_MakeBitmap(&dst, &smushvec_formats[2], NULL);
    SmushVec_Int(pApi->decode_Convert(aSrc, &src, 5, NULL, aDst, &dst, NULL), "decode_Convert indexed without palette rc");

    // With a table: decode_MakeTable for indexed sources, a 16-bit lookup table for 16-bit sources
    SmushVec_SectionBegin("decode_MakeTable and indexed table conversions");
    for ( int d = 0; d < numFormats; ++d )
    {
        SmushVec_MakeBitmap(&src, &smushvec_formats[0], aPalette);
        SmushVec_MakeBitmap(&dst, &smushvec_formats[d], NULL);
        memset(aTable, 0xEE, 1024 + 16);
        int rc = pApi->decode_MakeTable(&src, &dst, aTable, NULL);
        SmushVec_Int(rc, "decode_MakeTable indexed to %s rc", smushvec_formats[d].pName);
        SmushVec_Raw(aTable, 1024 + 16, "decode_MakeTable indexed to %s", smushvec_formats[d].pName);
        // 3-byte destinations keep stale index bits and leave the table (see Bloc16Asm.tv's README notes)
        if ( rc == 0 && smushvec_formats[d].pixelSize != 3 )
        {
            SmushVec_RandBytes(aSrc, 64, 0x7AB1E000u + (uint32_t)d);
            memset(aDst, 0xEE, 64 * 4 + 16);
            SmushVec_Int(pApi->decode_Convert(aSrc, &src, 64, NULL, aDst, &dst, aTable), "decode_Convert table indexed to %s rc", smushvec_formats[d].pName);
            SmushVec_Raw(aDst, 64 * 4 + 16, "decode_Convert table indexed to %s", smushvec_formats[d].pName);
        }
    }

    SmushVec_SectionEnd();
    SmushVec_MakeBitmap(&src, &smushvec_formats[0], NULL);
    SmushVec_MakeBitmap(&dst, &smushvec_formats[2], NULL);
    SmushVec_Int(pApi->decode_MakeTable(&src, &dst, aTable, aPalette), "decode_MakeTable palette argument rc");
    SmushVec_Int(pApi->decode_MakeTable(&src, &dst, aTable, NULL), "decode_MakeTable no palette rc");
    SmushVec_MakeBitmap(&src, &smushvec_formats[2], NULL);
    SmushVec_Int(pApi->decode_MakeTable(&src, &dst, aTable, aPalette), "decode_MakeTable 16-bit source rc");

    for ( int t = 0; t < 0x10000; ++t )
    {
        ((uint16_t*)aTable)[t] = (uint16_t)(t * 40503u);
    }

    SmushVec_SectionBegin("decode_Convert 16-bit table");
    for ( int d = 1; d < numFormats; ++d )
    {
        if ( smushvec_formats[d].pixelSize == 3 )
        {
            continue;
        }

        SmushVec_MakeBitmap(&src, &smushvec_formats[1], NULL);
        SmushVec_MakeBitmap(&dst, &smushvec_formats[d], NULL);
        SmushVec_RandBytes(aSrc, 128, 0x7AB16000u + (uint32_t)d);
        memset(aDst, 0xEE, 64 * 4 + 16);
        SmushVec_Int(pApi->decode_Convert(aSrc, &src, 63, NULL, aDst, &dst, aTable), "decode_Convert 16-bit table to %s rc", smushvec_formats[d].pName);
        SmushVec_Raw(aDst, 64 * 4 + 16, "decode_Convert 16-bit table to %s", smushvec_formats[d].pName);
    }

    SmushVec_SectionEnd();

    // decode_ToRGBA and decode_Begin on their own, including the unsupported pixel sizes
    for ( int f = 0; f < numFormats; ++f )
    {
        SmushVec_SectionBegin("decode_ToRGBA and decode_Begin %s", smushvec_formats[f].pName);
        for ( int ps = 0; ps <= 5; ++ps )
        {
            SmushVec_MakeBitmap(&src, &smushvec_formats[f], NULL);
            if ( ps )
            {
                src.pixelSize = ps;
            }

            SmushVec_RandBytes(aSrc, 33 * 5, 0x70000000u + (uint32_t)(f * 8 + ps));
            memset(aDst, 0xEE, 33 * 5 + 16);
            int rc = pApi->decode_ToRGBA(aSrc, &src, 33, aPalette, aDst);
            SmushVec_Int(rc, "decode_ToRGBA %s size %d rc", smushvec_formats[f].pName, ps);
            SmushVec_Raw(aDst, 33 * 4 + 16, "decode_ToRGBA %s size %d", smushvec_formats[f].pName, ps);
            memset(aDst, 0xEE, 33 * 5 + 16);
            rc = pApi->decode_Begin(aDst, &src, 33, aPalette, aSrc);
            SmushVec_Int(rc, "decode_Begin %s size %d rc", smushvec_formats[f].pName, ps);
            SmushVec_Raw(aDst, 33 * 5 + 16, "decode_Begin %s size %d", smushvec_formats[f].pName, ps);
        }

        SmushVec_SectionEnd();
    }

    // decode_Run: every pixel size, with equal and different bytes
    static const uint8_t aPixels[][4] = { { 0x12, 0x12, 0x12, 0x12 }, { 0x12, 0x34, 0x56, 0x78 }, { 0x12, 0x12, 0x34, 0x12 }, { 0x12, 0x12, 0x12, 0x34 } };
    SmushVec_SectionBegin("decode_Run");
    for ( int ps = 0; ps <= 5; ++ps )
    {
        for ( int k = 0; k < 4; ++k )
        {
            memset(&dst, 0, sizeof(dst));
            dst.pixelSize = ps;
            memset(aDst, 0xEE, 64);
            pApi->decode_Run(aDst, &dst, 9, (uint8_t*)aPixels[k]);
            SmushVec_Raw(aDst, 64, "decode_Run size %d pixel %d", ps, k);
        }
    }

    SmushVec_SectionEnd();

    // decode_Rle on generated streams of several lengths
    static const int aRleSizes[] = { 1, 2, 3, 100, 129, 1000, 4096 };
    SmushVec_SectionBegin("decode_Rle");
    for ( size_t i = 0; i < sizeof(aRleSizes) / sizeof(aRleSizes[0]); ++i )
    {
        SmushVecStream s = { 0 };
        SmushVec_Rle(&s, aRleSizes[i] + 7, 0x41E00000u + (uint32_t)i);
        memset(aDst, 0xEE, (size_t)aRleSizes[i] + 16);
        pApi->decode_Rle(aDst, s.p, aRleSizes[i]);
        SmushVec_Bytes(aDst, (uint32_t)aRleSizes[i] + 16, "decode_Rle %d", aRleSizes[i]);
        free(s.p);
    }

    SmushVec_SectionEnd();
}

/* ------------------------------------------------------------------------------------------------------------------- */
/* Lec/LecShare.c                                                                                                      */
/* ------------------------------------------------------------------------------------------------------------------- */

static int smushvec_lecErrors;
static void SmushVec_LecError(const char* pFormat, ...)
{
    (void)pFormat;
    smushvec_lecErrors++;
}

static void SmushVec_RunLec(const SmushVecApi* pApi, SmushVecSink* pSink, uint8_t fill)
{
    SmushVec_Begin(pApi, pSink, fill);

    // Names against lists in the forms the modules and the host use: NAME.instance=provider entries, separators
    // (space, tab, comma, semicolon, control and high characters), case, partial names, empty entries
    static const char* const aNames[] = {
        "FILIO", "FILIO.chainto", "MEMORY.workspace", "MEMORY", "SOUND", "filio", "FIL", "FILIOX", "BITMAP.clear", "", "A", "A.b",
        "ANIMPLAY.app", "X.Y.Z", "TIMER",
    };
    static const char* const aLists[] = {
        "FILIO=insane_stdio",
        "FILIO.chainto=insane_zfilio FILIO=insane_stdio",
        "MEMORY=insane_debug,MEMORY.workspace=insane;SOUND=insane_dsnd",
        "  filio = insane_stdio , memory=insane ",
        "FILIO.chainto=insane_zfilio",
        "FILIO=",
        "=insane",
        "FILIO",
        "FILIO=a=b",
        "BITMAP.clear=insane BITMAP=insane_other",
        "A=1 A.b=2 A=3",
        "A.b=2 A=1",
        "\tFILIO\t=\tinsane_tab\n",
        "FILIO=insane_stdio\x01MEMORY=x",
        "FILIO=insane\x80stdio MEMORY=\xC1y",
        "ANIMPLAY=insane ANIMPLAY.app=insane_app",
        "X.Y=1 X=2",
        "FILIO.=dot",
        "TIMER=insane;;;TIMER=other",
        "",
        ";,;, ,",
        "FILIO=insane_stdio FILIO.chainto=insane_zfilio FILIO.other=x",
    };

    for ( size_t l = 0; l < sizeof(aLists) / sizeof(aLists[0]); ++l )
    {
        SmushVec_SectionBegin("LecShare_Match list %d", (int)l);
        for ( size_t n = 0; n < sizeof(aNames) / sizeof(aNames[0]); ++n )
        {
            SmushVec_Str((const char*)(intptr_t)pApi->LecShare_Match(aNames[n], aLists[l]), "LecShare_Match name %d list %d", (int)n, (int)l);
        }

        SmushVec_SectionEnd();
    }

    // A list longer than the 0x1FFF-byte parse buffer is cut
    {
        char* pLong = (char*)malloc(0x2400);
        for ( int i = 0; i < 0x23F0; ++i )
        {
            pLong[i] = (char)('a' + i % 26);
        }

        memcpy(pLong + 0x1FF0, " FILIO=cut_here", 15);
        memcpy(pLong + 0x23C0, " MEMORY=gone", 12);
        pLong[0x23F0] = 0;
        SmushVec_Str((const char*)(intptr_t)pApi->LecShare_Match("FILIO", pLong), "LecShare_Match long list FILIO");
        SmushVec_Str((const char*)(intptr_t)pApi->LecShare_Match("MEMORY", pLong), "LecShare_Match long list MEMORY");
        free(pLong);
    }

    static const char* const aLenNames[] = {
        "FILIO", "FILIO.chainto", "FILIO=insane", "FILIO =insane", "FILIO  \t=x", "=x", " =x", "", ".", "A.B=C", "AB= ", "A,=b", "A;=b", "A\x01=b", "LECMODULE=insane_x",
    };
    SmushVec_SectionBegin("LecShare_NameLen");
    for ( size_t i = 0; i < sizeof(aLenNames) / sizeof(aLenNames[0]); ++i )
    {
        SmushVec_Int(pApi->LecShare_NameLen(aLenNames[i]), "LecShare_NameLen %d", (int)i);
    }

    SmushVec_SectionEnd();

    static const char* const aStrip[] = {
        "FILIO=insane_stdio", " F I L I O ", "\tA\tB\n", "", "   ", "A\x80" "B\xFF" "C", "~!=\x7F" "x", "AB\x01" "CD",
    };
    SmushVec_SectionBegin("LecShare_Strip");
    for ( size_t i = 0; i < sizeof(aStrip) / sizeof(aStrip[0]); ++i )
    {
        char aBuf[32];
        memset(aBuf, 0xEE, sizeof(aBuf));
        memcpy(aBuf, aStrip[i], strlen(aStrip[i]) + 1);
        pApi->LecShare_Strip(aBuf);
        SmushVec_Raw(aBuf, sizeof(aBuf), "LecShare_Strip %d", (int)i);
    }

    SmushVec_SectionEnd();

    static const char* const aCmp[][2] = {
        { "abc", "ABC" }, { "abc", "abd" }, { "abd", "abc" }, { "", "" }, { "a", "" }, { "", "a" }, { "FILIO=x", "filio=X" },
        { "a[", "A{" }, { "a\x80", "A\x80" }, { "a\xE1", "A\xC1" }, { "abc", "abcd" }, { "abcd", "abc" }, { "Z", "a" }, { "_", "a" },
    };
    for ( size_t i = 0; i < sizeof(aCmp) / sizeof(aCmp[0]); ++i )
    {
        SmushVec_SectionBegin("LecShare_NameCmp and NameNCmp %d", (int)i);
        SmushVec_Int(pApi->LecShare_NameCmp(aCmp[i][0], aCmp[i][1]), "LecShare_NameCmp %d", (int)i);
        static const int aN[] = { 0, 1, 2, 3, 4, 10, -1 };
        for ( size_t k = 0; k < sizeof(aN) / sizeof(aN[0]); ++k )
        {
            SmushVec_Int(pApi->LecShare_NameNCmp(aCmp[i][0], aCmp[i][1], aN[k]), "LecShare_NameNCmp %d n %d", (int)i, aN[k]);
        }

        SmushVec_SectionEnd();
    }

    // The share table: publish, subscribe with host lists (pA: defaults, pB: overrides) and preferences, enumerate,
    // count, find, unpublish and unsubscribe
    static int aIfaces[8];
    static char aPubNames[8][40];
    static const char* const aPub[] = {
        "FILIO=insane_stdio", "FILIO=insane_zfilio", "MEMORY=insane", "MEMORY=insane_debug", "SOUND=insane_dsnd", "filio=lower", "BITMAP=insane", " TIMER = spaced ",
    };
    for ( int i = 0; i < 8; ++i )
    {
        memcpy(aPubNames[i], aPub[i], strlen(aPub[i]) + 1);
    }

    pApi->LecShare_Reset(NULL, NULL);
    SmushVec_SectionBegin("LecShare table publish and subscribe");
    SmushVec_Int(pApi->LecShare_Subscribe("FILIO", NULL) != NULL, "LecShare_Subscribe before Init");
    smushvec_lecErrors = 0;
    LecShare* pTable = pApi->LecShare_Init(SmushVec_LecError, NULL, "FILIO=insane_stdio MEMORY=insane_debug", "SOUND=missing FILIO.chainto=insane_zfilio");
    SmushVec_Int(pTable != NULL, "LecShare_Init table");
    for ( int i = 0; i < 8; ++i )
    {
        SmushVec_Int(pApi->LecShare_Publish(aPubNames[i], &aIfaces[i]), "LecShare_Publish %d rc", i);
    }

    static const char* const aSub[][2] = {
        { "FILIO", NULL }, { "FILIO", "FILIO=insane_zfilio" }, { "FILIO.chainto", NULL }, { "FILIO.other", "FILIO.other=insane_stdio" },
        { "MEMORY", NULL }, { "MEMORY.workspace", "MEMORY.workspace=insane" }, { "MEMORY", "MEMORY=nothing" }, { "SOUND", NULL },
        { "SOUND", "SOUND=insane_dsnd" }, { "BITMAP", NULL }, { "BITMAP.clear", "" }, { "TIMER", NULL }, { "NONE", NULL },
        { "filio", NULL }, { "FILIO", "FILIO=lower" }, { "FILIO", "FILIO=LOWER" },
    };
    for ( size_t i = 0; i < sizeof(aSub) / sizeof(aSub[0]); ++i )
    {
        void* p = pApi->LecShare_Subscribe(aSub[i][0], aSub[i][1]);
        int index = -1;
        for ( int k = 0; k < 8; ++k )
        {
            if ( p == &aIfaces[k] )
            {
                index = k;
            }
        }

        SmushVec_Int(p ? index : -2, "LecShare_Subscribe %d", (int)i);
    }

    SmushVec_SectionEnd();
    SmushVec_SectionBegin("LecShare table count, enumerate, find");
    static const char* const aCount[] = { "FILIO", "MEMORY", "SOUND", "filio", "FIL", "TIMER", "NONE" };
    for ( size_t i = 0; i < sizeof(aCount) / sizeof(aCount[0]); ++i )
    {
        SmushVec_Int(pApi->LecShare_Count(aCount[i]), "LecShare_Count %s", aCount[i]);
    }

    SmushVec_Int(pApi->LecShare_Count(NULL), "LecShare_Count all");

    // Enumerate forward, backward, by name and over the whole table; the cursor is kept between calls
    static const struct { const char* pName; int index; } aEnum[] = {
        { "FILIO", 0 }, { "FILIO", 1 }, { "FILIO", 2 }, { "FILIO", 3 }, { "FILIO", 1 }, { "FILIO", 0 }, { "FILIO", 2 }, { "MEMORY", 1 },
        { "MEMORY", 0 }, { NULL, 0 }, { NULL, 5 }, { NULL, 3 }, { NULL, 7 }, { NULL, 8 }, { NULL, 2 }, { "MEMORY", -1 }, { "TIMER", 0 },
        { "SOUND", 0 }, { "SOUND", 0 },
    };
    for ( size_t i = 0; i < sizeof(aEnum) / sizeof(aEnum[0]); ++i )
    {
        SmushVec_Str((const char*)pApi->LecShare_Enumerate(aEnum[i].pName, aEnum[i].index), "LecShare_Enumerate %d", (int)i);
    }

    static const char* const aFind[] = { "FILIO=insane_stdio", "filio=INSANE_STDIO", "FILIO", "TIMER=spaced", " TIMER = spaced ", "MEMORY=insane" };
    for ( size_t i = 0; i < sizeof(aFind) / sizeof(aFind[0]); ++i )
    {
        LecEntry* pEntry = pApi->LecShare_Find(aFind[i]);
        SmushVec_Str(pEntry ? pEntry->pName : NULL, "LecShare_Find %d", (int)i);
        SmushVec_Int(pEntry ? pEntry->refs : -99, "LecShare_Find %d refs", (int)i);
    }

    SmushVec_SectionEnd();
    SmushVec_SectionBegin("LecShare table batches and unpublish");

    // Batch subscription with defaults and one that fails
    void* aOut[4] = { NULL, NULL, NULL, NULL };
    LecBatch aBatch[] = {
        { &aOut[0], "FILIO", "FILIO=insane_zfilio" }, { &aOut[1], "MEMORY", "" }, { &aOut[2], "SOUND", NULL }, { NULL, NULL, NULL },
    };
    SmushVec_Int(pApi->LecShare_BatchSubscribe(aBatch, "MEMORY=insane"), "LecShare_BatchSubscribe rc");
    for ( int i = 0; i < 3; ++i )
    {
        int index = -1;
        for ( int k = 0; k < 8; ++k )
        {
            if ( aOut[i] == &aIfaces[k] )
            {
                index = k;
            }
        }

        SmushVec_Int(aOut[i] ? index : -2, "LecShare_BatchSubscribe out %d", i);
    }

    SmushVec_Int(pApi->LecShare_BatchUnsubscribe(aBatch), "LecShare_BatchUnsubscribe rc");
    LecBatch aFail[] = { { &aOut[3], "NONE", "NONE=x" }, { NULL, NULL, NULL } };
    SmushVec_Int(pApi->LecShare_BatchSubscribe(aFail, NULL), "LecShare_BatchSubscribe missing rc");
    SmushVec_Int(smushvec_lecErrors, "LecShare errors");

    SmushVec_Int(pApi->LecShare_Unsubscribe(&aIfaces[0]), "LecShare_Unsubscribe 0 rc");
    SmushVec_Int(pApi->LecShare_Unsubscribe(&aOut), "LecShare_Unsubscribe unknown rc");
    for ( int i = 0; i < 8; ++i )
    {
        SmushVec_Int(pApi->LecShare_Unpublish(aPubNames[i]), "LecShare_Unpublish %d refs", i);
        SmushVec_Raw(aPubNames[i], sizeof(aPubNames[i]), "LecShare_Unpublish %d name", i);
    }

    SmushVec_Int(pApi->LecShare_Unpublish(aPubNames[0]), "LecShare_Unpublish again rc");
    SmushVec_Int(pApi->LecShare_Count(NULL), "LecShare_Count after unpublish");
    SmushVec_SectionEnd();
    pApi->LecShare_Reset(NULL, NULL);
}

/* ------------------------------------------------------------------------------------------------------------------- */
/* File/Framestream.c                                                                                                  */
/* ------------------------------------------------------------------------------------------------------------------- */

/* Streamer stub: Advance hands out the next n bytes of the synthetic file, -1 (not yet buffered) or -2 on scripted
   calls, NULL at the end; IsActive and SetFile return scripted values. The frame table stub's Fetch hands out a
   held copy of the stream (paused playback). */
typedef struct sSmushVecFs
{
    uint8_t* pData;
    int size;
    int pos;
    int numAdvance;
    int atEnd;
    int notReadyAt[4];
    int errorAt;
    int isActive;
    int setFileRc;
    uint8_t* pHeld;
    int heldSize;
    int heldPos;
    uint32_t decodeResults;   /* 2 bits per Decode call */
    int numDecode;
} SmushVecFs;

static SmushVecFs smushvec_fs;
static Stream* const smushvec_pStream = (Stream*)(intptr_t)0x5717EA00;

static int SmushVec_Offset(const void* p)
{
    const uint8_t* q = (const uint8_t*)p;
    if ( q == NULL )
    {
        return 0;
    }

    if ( (intptr_t)q == -1 || (intptr_t)q == -2 )
    {
        return (int)(intptr_t)q;
    }

    if ( smushvec_fs.pData && q >= smushvec_fs.pData && q <= smushvec_fs.pData + smushvec_fs.size )
    {
        return 0x100000 + (int)(q - smushvec_fs.pData);
    }

    if ( smushvec_fs.pHeld && q >= smushvec_fs.pHeld && q <= smushvec_fs.pHeld + smushvec_fs.heldSize )
    {
        return 0x200000 + (int)(q - smushvec_fs.pHeld);
    }

    return 0x7FFFFFFF;
}

static int SmushVec_StreamSetFile(Stream* pStream, const char* pName, int startOffset)
{
    SmushVec_Log8('S');
    SmushVec_Log32(pStream == smushvec_pStream);
    SmushVec_Log32((int)strlen(pName));
    SmushVec_Log32(startOffset);
    return smushvec_fs.setFileRc;
}

static int SmushVec_StreamIsActive(Stream* pStream)
{
    SmushVec_Log8('A');
    SmushVec_Log8(pStream == smushvec_pStream);
    return smushvec_fs.isActive;
}

static int SmushVec_StreamAdvance(Stream* pStream, int n)
{
    SmushVec_Log8('a');
    SmushVec_Log8(pStream == smushvec_pStream);
    SmushVec_Log32(n);
    int call = smushvec_fs.numAdvance++;
    for ( int i = 0; i < 4; ++i )
    {
        if ( smushvec_fs.notReadyAt[i] == call + 1 )
        {
            return -1;
        }
    }

    if ( smushvec_fs.errorAt == call + 1 )
    {
        return -2;
    }

    // The end of the data: one NULL (end of stream), then "not yet buffered" (the frame stream would loop forever on NULL)
    if ( n < 0 || smushvec_fs.pos + n > smushvec_fs.size )
    {
        return smushvec_fs.atEnd++ ? -1 : 0;
    }

    int p = (int)(intptr_t)(smushvec_fs.pData + smushvec_fs.pos);
    smushvec_fs.pos += n;
    return p;
}

static void SmushVec_StreamReset(Stream* pStream)
{
    SmushVec_Log8('R');
    SmushVec_Log8(pStream == smushvec_pStream);
}

static void SmushVec_FrameReset(void)
{
    SmushVec_Log8('r');
}

static int SmushVec_FrameParse(const uint8_t* pData, int size)
{
    SmushVec_Log8('p');
    SmushVec_Log32(SmushVec_Offset(pData));
    SmushVec_Log32(size);
    return 0;
}

static uint32_t SmushVec_FrameDecode(void* pBitmap, int a2, int a3, int a4, void* pPalette, int* pOutA, int* pOutB, uint32_t flags, int a9, int a10, const uint8_t* pData, int size)
{
    SmushVec_Log8('d');
    SmushVec_Log32(pBitmap != NULL);
    SmushVec_Log32(a2);
    SmushVec_Log32(a3);
    SmushVec_Log32(a4);
    SmushVec_Log32(pPalette != NULL);
    SmushVec_Log32(pOutA ? *pOutA : -1);
    SmushVec_Log32(pOutB ? *pOutB : -1);
    SmushVec_Log32((int)flags);
    SmushVec_Log32(a9);
    SmushVec_Log32(a10);
    SmushVec_Log32(SmushVec_Offset(pData));
    SmushVec_Log32(size);
    uint32_t rc = (smushvec_fs.decodeResults >> (2 * (smushvec_fs.numDecode++ & 15))) & 3;
    return rc;
}

static void* SmushVec_FrameFetch(int n)
{
    SmushVec_Log8('f');
    SmushVec_Log32(n);
    if ( n < 0 || !smushvec_fs.pHeld || smushvec_fs.heldPos + n > smushvec_fs.heldSize )
    {
        return NULL;
    }

    void* p = smushvec_fs.pHeld + smushvec_fs.heldPos;
    smushvec_fs.heldPos += n;
    return p;
}

static void SmushVec_OpsSetRate(AnimPlay* pPlay, int rate)
{
    (void)pPlay;
    SmushVec_Log8('t');
    SmushVec_Log32(rate);
}

static void SmushVec_OpsRewind(AnimPlay* pPlay)
{
    (void)pPlay;
    SmushVec_Log8('w');
}

static int SmushVec_OpsIsDone(AnimPlay* pPlay)
{
    (void)pPlay;
    SmushVec_Log8('i');
    return 0;
}

static void SmushVec_Chunk(SmushVecStream* s, const char* pTag, int size, uint32_t seed, int bodySize)
{
    for ( int i = 0; i < 4; ++i )
    {
        SmushVec_S8(s, (uint8_t)pTag[i]);
    }

    SmushVec_S8(s, (uint32_t)size >> 24);
    SmushVec_S8(s, (uint32_t)size >> 16);
    SmushVec_S8(s, (uint32_t)size >> 8);
    SmushVec_S8(s, (uint32_t)size);
    uint32_t r = seed;
    for ( int i = 0; i < bodySize; ++i )
    {
        SmushVec_S8(s, SmushVec_Rnd(&r));
    }
}

/* An FLHD-style header chunk body (FrameHeader): version, count, rate, palette */
static void SmushVec_HeaderChunk(SmushVecStream* s, const char* pTag, int count, int rate, uint32_t seed)
{
    SmushVec_Chunk(s, pTag, (int)sizeof(FrameHeader), 0, 0);
    int start = s->size;
    for ( int i = 0; i < (int)sizeof(FrameHeader); ++i )
    {
        SmushVec_S8(s, 0);
    }

    FrameHeader hdr;
    SmushVec_RandBytes(&hdr, sizeof(hdr), seed);
    hdr.version = 2;
    hdr.count   = (int16_t)count;
    hdr.rate    = rate;
    memcpy(s->p + start, &hdr, sizeof(hdr));
}

typedef struct sSmushVecFsScript
{
    const char* pName;
    int variant;
} SmushVecFsScript;

static void SmushVec_BuildStream(SmushVecStream* s, int variant)
{
    switch ( variant )
    {
        case 0: // a well-formed animation: header, annotation, frames, end
            SmushVec_HeaderChunk(s, "SHDR", 5, 15, 11);
            SmushVec_Chunk(s, "ANNO", 7, 12, 8);    // odd size: rounded up
            for ( int i = 0; i < 5; ++i )
            {
                SmushVec_Chunk(s, "FRME", 20 + i, 100u + (uint32_t)i, (20 + i + 1) & ~1);
            }
            break;

        case 1: // FLHD handed to the frame parser, a header with rate 0 and a large count, unknown tag
            SmushVec_Chunk(s, "FLHD", 16, 21, 16);
            SmushVec_HeaderChunk(s, "SHDR", 30, 0, 22);
            SmushVec_Chunk(s, "FRME", 8, 23, 8);
            SmushVec_Chunk(s, "FRME", 0, 0, 0);
            SmushVec_Chunk(s, "XYZW", 4, 24, 4);
            break;

        case 2: // explicit state tags in the stream
            SmushVec_HeaderChunk(s, "SHDR", 3, 30, 31);
            SmushVec_Chunk(s, "FRME", 6, 32, 6);
            SmushVec_Chunk(s, "paws", 0, 0, 0);
            SmushVec_Chunk(s, "FRME", 6, 33, 6);
            SmushVec_Chunk(s, "wait", 0, 0, 0);
            SmushVec_Chunk(s, "end ", 0, 0, 0);
            break;

        default: // many frames for seeking
            SmushVec_HeaderChunk(s, "SHDR", 12, 1000000, 41);
            for ( int i = 0; i < 12; ++i )
            {
                SmushVec_Chunk(s, "FRME", 4, 200u + (uint32_t)i, 4);
            }
            break;
    }
}

static void SmushVec_FsRecordState(const AnimPlay* pPlay, const char* pName)
{
    AnimPlay copy = *pPlay;
    copy.pCur    = (void*)(intptr_t)SmushVec_Offset(pPlay->pCur);
    copy.pStream = (Stream*)(intptr_t)(pPlay->pStream == smushvec_pStream);
    SmushVec_Raw(&copy, sizeof(copy), "%s: state", pName);
}

typedef struct sSmushVecFsCase
{
    const char* pName;
    int variant;
    int flags;          /* framestream_Open */
    int seekLen;
    int seekTo;
    int hold;
    int a9;             /* Tick flags */
    int isActive;
    int setFileRc;
    int notReadyAt[4];
    int errorAt;
    uint32_t decodeResults;
    int numTicks;
    int modeAt;         /* tick before which framestream_Mode(modeOp) is called (0: never) */
    int modeOp;
    int held;           /* paused playback from a held copy */
} SmushVecFsCase;

static void SmushVec_RunFramestream(const SmushVecApi* pApi, SmushVecSink* pSink, uint8_t fill)
{
    SmushVec_Begin(pApi, pSink, fill);

    StreamerInterface streamer;
    memset(&streamer, 0, sizeof(streamer));
    streamer.SetFile  = SmushVec_StreamSetFile;
    streamer.IsActive = SmushVec_StreamIsActive;
    streamer.Advance  = SmushVec_StreamAdvance;
    streamer.Reset    = SmushVec_StreamReset;
    AnimFrameTable frame;
    memset(&frame, 0, sizeof(frame));
    frame.Reset  = SmushVec_FrameReset;
    frame.Parse  = SmushVec_FrameParse;
    frame.Decode = SmushVec_FrameDecode;
    frame.Fetch  = SmushVec_FrameFetch;
    AnimPlayOps ops = { SmushVec_OpsSetRate, SmushVec_OpsRewind, SmushVec_OpsIsDone, NULL };
    pApi->SetAnimGlobals(&sv.share, &streamer, &frame, &ops);

    static const SmushVecFsCase aCases[] = {
        { "plain", 0, 0, -1, -1, 0, 4, 0, 0, { 0 }, 0, 0, 14 },
        { "palette kept", 0, 0x2000, -1, -1, 0, 0, 0, 0, { 0 }, 0, 0, 10 },
        { "rate kept", 0, 8, -1, -1, 0, 4, 0, 0, { 0 }, 0, 0, 10 },
        { "not ready", 0, 0, -1, -1, 0, 4, 0, 0, { 2, 3, 7, 9 }, 0, 0, 16 },
        { "stream error", 0, 0, -1, -1, 0, 4, 0, 0, { 0 }, 5, 0, 8 },
        { "decode stalls and rewinds", 0, 0, -1, -1, 0, 0, 0, 0, { 0 }, 0, 0x00000D9Bu, 16 },
        { "streamer active", 0, 0, -1, -1, 0, 4, 1, 0, { 0 }, 0, 0, 14 },
        { "set file fails", 0, 0, -1, -1, 0, 4, 0, -7, { 0 }, 0, 0, 4 },
        { "flhd and unknown tag", 1, 0, -1, -1, 0, 4, 0, 0, { 0 }, 0, 0, 10 },
        { "state tags", 2, 0, -1, -1, 0, 4, 0, 0, { 0 }, 0, 0, 12 },
        { "state tags active", 2, 0, -1, -1, 0, 4, 1, 0, { 0 }, 0, 0, 12 },
        { "seek to 5", 3, 0, -1, 5, 0, 4, 0, 0, { 0 }, 0, 0, 16 },
        { "seek length 3", 3, 0, 3, -1, 0, 4, 0, 0, { 0 }, 0, 0, 16 },
        { "hold", 3, 0, -1, -1, 1, 4, 0, 0, { 0 }, 0, 0, 18, 0, 0, 1 },
        { "mode 1 swap", 0, 0, -1, -1, 0, 4, 0, 0, { 0 }, 0, 0, 14, 2, 1 },
        { "mode toggle", 0, 0, -1, -1, 0, 4, 0, 0, { 0 }, 0, 0, 14, 3, 2 },
        { "mode 3", 3, 0, -1, -1, 0, 4, 0, 0, { 0 }, 0, 0, 16, 2, 3 },
        { "mode 4 after 1", 3, 0, -1, -1, 0, 4, 0, 0, { 0 }, 0, 0, 16, 1, 1 },
        { "blit fill", 0, 0, -1, -1, 0, 0, 0, 0, { 0 }, 0, 0, 8 },
    };

    uint16_t aPixels[8 * 6];
    LecBitmapHeader bitmap;
    for ( size_t c = 0; c < sizeof(aCases) / sizeof(aCases[0]); ++c )
    {
        const SmushVecFsCase* pCase = &aCases[c];
        SmushVecStream s = { 0 };
        SmushVec_BuildStream(&s, pCase->variant);
        memset(&smushvec_fs, 0, sizeof(smushvec_fs));
        smushvec_fs.pData     = s.p;
        smushvec_fs.size      = s.size;
        smushvec_fs.isActive  = pCase->isActive;
        smushvec_fs.setFileRc = pCase->setFileRc;
        memcpy(smushvec_fs.notReadyAt, pCase->notReadyAt, sizeof(smushvec_fs.notReadyAt));
        smushvec_fs.errorAt       = pCase->errorAt;
        smushvec_fs.decodeResults = pCase->decodeResults;
        if ( pCase->held )
        {
            smushvec_fs.pHeld    = (uint8_t*)malloc((size_t)s.size);
            smushvec_fs.heldSize = s.size;
            memcpy(smushvec_fs.pHeld, s.p, (size_t)s.size);
        }

        AnimPlay play;
        memset(&play, 0, sizeof(play));
        play.pStream = smushvec_pStream;
        play.frame   = 77;
        play.tag     = 0x12345678;
        sv.logSize   = 0;

        char aName[96];
        snprintf(aName, sizeof(aName), "framestream %s", pCase->pName);
        SmushVec_SectionBegin("%s", aName);
        SmushVec_Int(pApi->framestream_Reset(&play), "%s: Reset rc", aName);
        SmushVec_Int(pApi->framestream_Open(&play, "movie.snm", pCase->flags, pCase->seekLen, pCase->seekTo, pCase->hold), "%s: Open rc", aName);
        SmushVec_Int(pApi->framestream_IsOpen(&play), "%s: IsOpen", aName);
        SmushVec_FsRecordState(&play, aName);
        SmushVec_LogFlush(aName);

        uint8_t aPalette[0x400 + 16];
        memset(aPalette, 0xEE, sizeof(aPalette));
        memset(&bitmap, 0, sizeof(bitmap));
        SmushVec_SetFormat(&bitmap, 1, 2, smushvec_fmt565);
        bitmap.width   = 8;
        bitmap.height  = 6;
        bitmap.pitch   = 16;
        bitmap.pPixels = aPixels;
        for ( int t = 0; t < pCase->numTicks; ++t )
        {
            if ( pCase->modeAt && t == pCase->modeAt )
            {
                SmushVec_Int(pApi->framestream_Mode(&play, pCase->modeOp), "%s: tick %d Mode %d", aName, t, pCase->modeOp);
                if ( pCase->modeOp == 1 )
                {
                    SmushVec_Int(pApi->framestream_Mode(&play, 4), "%s: tick %d Mode 4", aName, t);
                    SmushVec_Int(pApi->framestream_Mode(&play, 5), "%s: tick %d Mode 5", aName, t);
                }
            }

            int a7 = 0x70 + t, a8 = 0x80;
            SmushVec_RandBytes(aPixels, sizeof(aPixels), 0x919E1000u + (uint32_t)t);
            int rc = pApi->framestream_Tick(&play, &bitmap, 11, 12, 13, aPalette, &a7, &a8, pCase->a9, 19, 20);
            SmushVec_Int(rc, "%s: tick %d rc", aName, t);
            int aOut[2] = { a7, a8 };
            SmushVec_Raw(aOut, sizeof(aOut), "%s: tick %d outputs", aName, t);
            SmushVec_FsRecordState(&play, aName);
            SmushVec_HashRec(aPalette, sizeof(aPalette), "%s: tick %d palette", aName, t);
            SmushVec_Raw(aPixels, sizeof(aPixels), "%s: tick %d bitmap", aName, t);
            SmushVec_LogFlush(aName);
        }

        pApi->framestream_Seek(&play, 4, 9);
        pApi->framestream_Stall(&play);
        SmushVec_FsRecordState(&play, aName);
        SmushVec_Int(pApi->framestream_Tick(&play, &bitmap, 0, 0, 0, aPalette, NULL, NULL, 4, 0, 0), "%s: stalled tick rc", aName);
        SmushVec_LogFlush(aName);
        SmushVec_SectionEnd();
        free(s.p);
        free(smushvec_fs.pHeld);
        memset(&smushvec_fs, 0, sizeof(smushvec_fs));
    }

    SmushVec_Int(sv.numErrors, "framestream errors");
    SmushVec_Str(sv.numErrors ? sv.aLastError : NULL, "framestream last error");
    pApi->SetAnimGlobals(NULL, NULL, NULL, NULL);
}

/* ------------------------------------------------------------------------------------------------------------------- */
/* File/Stdfile.c and File/Zfilio/Zfilio.c                                                                             */
/* ------------------------------------------------------------------------------------------------------------------- */

/* FILIO stub: up to 6 memory files; handles are 1-based, 0 = failure */
#define SMUSHVEC_MAXFILES 6
typedef struct sSmushVecFile
{
    char aPath[32];
    uint8_t aData[4096];
    int size;
    int pos;
    int open;
} SmushVecFile;

static SmushVecFile smushvec_files[SMUSHVEC_MAXFILES];

static SmushVecFile* SmushVec_FileAdd(const char* pPath, const void* pData, int size)
{
    for ( int i = 0; i < SMUSHVEC_MAXFILES; ++i )
    {
        if ( smushvec_files[i].aPath[0] == 0 )
        {
            memcpy(smushvec_files[i].aPath, pPath, strlen(pPath) + 1);
            if ( pData )
            {
                memcpy(smushvec_files[i].aData, pData, (size_t)size);
            }

            smushvec_files[i].size = size;
            return &smushvec_files[i];
        }
    }

    return NULL;
}

static SmushVecFile* SmushVec_FileGet(int fh)
{
    return fh >= 1 && fh <= SMUSHVEC_MAXFILES ? &smushvec_files[fh - 1] : NULL;
}

static int SmushVec_FilioOpen(const char* pPath, const char* pMode)
{
    SmushVec_Log8('O');
    SmushVec_Log32((int)strlen(pPath));
    SmushVec_Log8(pMode[0]);
    for ( int i = 0; i < SMUSHVEC_MAXFILES; ++i )
    {
        if ( smushvec_files[i].aPath[0] && strcmp(smushvec_files[i].aPath, pPath) == 0 )
        {
            smushvec_files[i].open = 1;
            smushvec_files[i].pos  = 0;
            return i + 1;
        }
    }

    return 0;
}

static void SmushVec_FilioClose(int fh)
{
    SmushVec_Log8('C');
    SmushVec_Log32(fh);
    SmushVecFile* pFile = SmushVec_FileGet(fh);
    if ( pFile )
    {
        pFile->open = 0;
    }
}

static int SmushVec_FilioRead(int fh, void* pBuf, int size)
{
    SmushVec_Log8('R');
    SmushVec_Log32(fh);
    SmushVec_Log32(size);
    SmushVecFile* pFile = SmushVec_FileGet(fh);
    if ( !pFile || size <= 0 || pFile->pos >= pFile->size || pFile->pos < 0 )
    {
        return 0;
    }

    int n = pFile->size - pFile->pos < size ? pFile->size - pFile->pos : size;
    memcpy(pBuf, pFile->aData + pFile->pos, (size_t)n);
    pFile->pos += n;
    return n;
}

static int SmushVec_FilioWrite(int fh, const void* pBuf, int size)
{
    SmushVec_Log8('W');
    SmushVec_Log32(fh);
    SmushVec_Log32(size);
    SmushVecFile* pFile = SmushVec_FileGet(fh);
    if ( !pFile || size <= 0 || pFile->pos < 0 || pFile->pos + size > (int)sizeof(pFile->aData) )
    {
        return 0;
    }

    memcpy(pFile->aData + pFile->pos, pBuf, (size_t)size);
    pFile->pos += size;
    if ( pFile->pos > pFile->size )
    {
        pFile->size = pFile->pos;
    }

    return size;
}

static int SmushVec_FilioTell(int fh)
{
    SmushVec_Log8('T');
    SmushVec_Log32(fh);
    SmushVecFile* pFile = SmushVec_FileGet(fh);
    return pFile ? pFile->pos : -1;
}

static int SmushVec_FilioSeek(int fh, int offset, int whence)
{
    SmushVec_Log8('S');
    SmushVec_Log32(fh);
    SmushVec_Log32(offset);
    SmushVec_Log32(whence);
    SmushVecFile* pFile = SmushVec_FileGet(fh);
    if ( !pFile )
    {
        return -1;
    }

    pFile->pos = (whence == 0 ? 0 : whence == 1 ? pFile->pos : pFile->size) + offset;
    return 0x5EE0 + whence;
}

static void SmushVec_FilioFlush(int fh)
{
    SmushVec_Log8('F');
    SmushVec_Log32(fh);
}

static FilioInterface smushvec_filio = {
    SmushVec_FilioOpen, SmushVec_FilioClose, SmushVec_FilioRead, SmushVec_FilioWrite, SmushVec_FilioTell, SmushVec_FilioSeek, SmushVec_FilioFlush
};

static void SmushVec_DebugPrint(const char* pMessage)
{
    SmushVec_Log8('M');
    SmushVec_Log32((int)strlen(pMessage));
}

static LecDebug smushvec_debug = { NULL, NULL, NULL, SmushVec_DebugPrint };
static int smushvec_noDebug;

static void* SmushVec_ShareSubscribe(const char* pName, void* pParam)
{
    SmushVec_Log8('s');
    SmushVec_Log32((int)strlen(pName));
    SmushVec_Log8(pParam != NULL);
    if ( strcmp(pName, "MEMORY.workspace") == 0 || strcmp(pName, "MEMORY.buffers") == 0 )
    {
        return &sv.memory;
    }

    if ( strcmp(pName, "BITMAP.clear") == 0 )
    {
        return &sv.bitmaps;
    }

    if ( strcmp(pName, "FILIO.chainto") == 0 )
    {
        return &smushvec_filio;
    }

    if ( strcmp(pName, "VIRTHERC.debug") == 0 )
    {
        return smushvec_noDebug ? NULL : &smushvec_debug;
    }

    return NULL;
}

static void SmushVec_FileState(const char* pName)
{
    for ( int i = 0; i < SMUSHVEC_MAXFILES; ++i )
    {
        if ( smushvec_files[i].aPath[0] )
        {
            int aState[2] = { smushvec_files[i].size, smushvec_files[i].pos };
            SmushVec_Raw(aState, sizeof(aState), "%s: file %d size and position", pName, i);
            SmushVec_Bytes(smushvec_files[i].aData, (uint32_t)smushvec_files[i].size, "%s: file %d data", pName, i);
        }
    }
}

static void SmushVec_FileFlush(const char* pName)
{
    SmushVec_LogFlush(pName);
    SmushVec_FileState(pName);
}

static void SmushVec_RunStdfile(const SmushVecApi* pApi, SmushVecSink* pSink, uint8_t fill)
{
    SmushVec_Begin(pApi, pSink, fill);
    memset(smushvec_files, 0, sizeof(smushvec_files));
    static const char text[] = "first line\n\nthird\r\nab\nlast without newline";
    SmushVec_FileAdd("text.txt", text, (int)sizeof(text) - 1);
    SmushVec_FileAdd("out.txt", NULL, 0);
    SmushVec_FileAdd("empty.txt", NULL, 0);
    uint8_t aBin[300];
    SmushVec_RandBytes(aBin, sizeof(aBin), 0xB1A50000u);
    SmushVec_FileAdd("bin.dat", aBin, (int)sizeof(aBin));

    SmushVec_Int(pApi->stdfile_Register(&sv.share, (void*)"param"), "stdfile_Register rc");
    SmushVec_Int(pApi->stdfile_Startup(), "stdfile_Startup rc");
    SmushVec_LogFlush("stdfile register and startup");
    StdFileInterface* pFile = (StdFileInterface*)SmushVec_Published("FILE=insane_stdfile");
    if ( !pFile )
    {
        return;
    }

    // gets: every buffer size against short, empty and unterminated lines, end of file
    static const int aGets[] = { 1, 2, 3, 5, 11, 12, 16, 100 };
    for ( size_t i = 0; i < sizeof(aGets) / sizeof(aGets[0]); ++i )
    {
        SmushVec_SectionBegin("stdfile gets size %d", aGets[i]);
        int fh = pFile->Open("text.txt", "rb");
        SmushVec_Int(fh, "stdfile open text %d", aGets[i]);
        for ( int k = 0; k < 8; ++k )
        {
            char aBuf[128];
            memset(aBuf, 0xEE, sizeof(aBuf));
            char* p = pFile->Gets(fh, aBuf, aGets[i]);
            SmushVec_Int(p == aBuf ? 1 : p == NULL ? 0 : -1, "stdfile gets size %d line %d rc", aGets[i], k);
            SmushVec_Raw(aBuf, (uint32_t)aGets[i] + 8, "stdfile gets size %d line %d", aGets[i], k);
            SmushVec_Int(pFile->Eof(fh), "stdfile gets size %d line %d eof", aGets[i], k);
        }

        pFile->Close(fh);
        SmushVec_FileState("stdfile gets");
        SmushVec_SectionEnd();

        // The calls to the chained FILIO (separately: stdfile_eof's call order)
        char aName[48];
        snprintf(aName, sizeof(aName), "stdfile calls gets size %d", aGets[i]);
        SmushVec_LogFlush(aName);
    }

    // getc to the end, size, tell, seek, read
    SmushVec_SectionBegin("stdfile binary");
    int fh = pFile->Open("bin.dat", "rb");
    SmushVec_Int(pFile->Size(fh), "stdfile size");
    SmushVec_Int(pFile->Tell(fh), "stdfile tell after size");
    int aC[16];
    for ( int i = 0; i < 16; ++i )
    {
        aC[i] = pFile->Getc(fh);
    }

    SmushVec_Raw(aC, sizeof(aC), "stdfile getc");
    SmushVec_Int(pFile->Seek(fh, -3, 2), "stdfile seek end rc");
    for ( int i = 0; i < 5; ++i )
    {
        aC[i] = pFile->Getc(fh);
    }

    SmushVec_Raw(aC, 5 * sizeof(int), "stdfile getc at end");
    SmushVec_Int(pFile->Eof(fh), "stdfile eof at end");
    SmushVec_Int(pFile->Seek(fh, 10, 0), "stdfile seek set rc");
    uint8_t aRead[64];
    memset(aRead, 0xEE, sizeof(aRead));
    SmushVec_Int(pFile->Read(fh, aRead, 40), "stdfile read rc");
    SmushVec_Raw(aRead, sizeof(aRead), "stdfile read");
    SmushVec_Int(pFile->Seek(fh, 5, 1), "stdfile seek cur rc");
    SmushVec_Int(pFile->Tell(fh), "stdfile tell");
    SmushVec_Int(pFile->Eof(fh), "stdfile eof inside");
    pFile->Flush(fh);
    pFile->Close(fh);
    fh = pFile->Open("empty.txt", "rb");
    SmushVec_Int(pFile->Size(fh), "stdfile size empty");
    SmushVec_Int(pFile->Eof(fh), "stdfile eof empty");
    char aEmpty[8];
    memset(aEmpty, 0xEE, sizeof(aEmpty));
    SmushVec_Int(pFile->Gets(fh, aEmpty, 8) == NULL, "stdfile gets empty is null");
    SmushVec_Raw(aEmpty, sizeof(aEmpty), "stdfile gets empty");
    pFile->Close(fh);
    SmushVec_Int(pFile->Open("missing.txt", "rb"), "stdfile open missing");
    SmushVec_FileState("stdfile binary");
    SmushVec_SectionEnd();
    SmushVec_LogFlush("stdfile calls binary");

    // putc and printf into a file
    SmushVec_SectionBegin("stdfile putc and write");
    fh = pFile->Open("out.txt", "wb");
    static const int aPut[] = { 'A', 0x1FF, -1, 0, 0x80, 'z' };
    for ( size_t i = 0; i < sizeof(aPut) / sizeof(aPut[0]); ++i )
    {
        SmushVec_Int(pFile->Putc(fh, aPut[i]), "stdfile putc %d", (int)i);
    }

    SmushVec_Int(pFile->Write(fh, "write", 5), "stdfile write rc");
    SmushVec_FileFlush("stdfile putc and write");
    SmushVec_SectionEnd();
    // printf: every result's text is stored (the CRT formats it)
    SmushVecFile* pOut = &smushvec_files[1];
    int mark = pOut->size;
#define SMUSHVEC_PRINTF(name, ...)                                                                    \
    do                                                                                                \
    {                                                                                                 \
        SmushVec_Int(pFile->Printf(fh, __VA_ARGS__), "stdfile printf %s rc", name);                   \
        SmushVec_Raw(pOut->aData + mark, (uint32_t)(pOut->size - mark), "stdfile printf %s", name);   \
        mark = pOut->size;                                                                            \
    } while ( 0 )

    SMUSHVEC_PRINTF("integers", "%d|%5d|%-5d|%05d|%+d|% d|%u|%x|%X|%#x|%o|%c|%s|%10s|%-10s|%.2s|%%\n", 42, -42, 7, 33, 5, 6, 0xFFFFFFFFu, 0xBEEF,
        0xBEEF, 255, 8, 'Q', "str", "right", "left", "truncate");
    SMUSHVEC_PRINTF("fixed", "%f|%.1f|%.2f|%10.3f|%-10.1f|%f|%.3f\n", 3.14159, 0.05, 1.005, -12.3456, 7.25, 1e15, -0.0005);
    SMUSHVEC_PRINTF("round half", "%.0f|%.0f|%.0f|%.0f|%.1f|%.1f|%.2f\n", 0.5, 1.5, 2.5, 3.5, 0.25, 0.35, 2.675);
    SMUSHVEC_PRINTF("general", "%g|%g|%g|%g|%G|%g|%.3g|%#g\n", 0.0001, 123456789.0, 1e-5, 100000.0, 1e300, 0.0, 1234.5, 1.0);
    SMUSHVEC_PRINTF("exponent", "%e|%E|%.3e|%e|%e|%.0e\n", 12345.678, 0.000123, 1.0, 0.0, -1e-300, 5e10);
    SMUSHVEC_PRINTF("modifiers", "%ld|%hd|%lu|%5.1s|%c%c|%hu\n", 123456L, 70000, 4000000000ul, "abc", 'x', 0x141, 70000);
    SMUSHVEC_PRINTF("empty", "%s", "");
#undef SMUSHVEC_PRINTF
    pFile->Close(fh);
    SmushVec_FileFlush("stdfile output");

    pApi->stdfile_Shutdown();
    pApi->stdfile_Unregister();
    SmushVec_LogFlush("stdfile shutdown and unregister");
}

/* zlib stub: gz handles are slots with a generated uncompressed stream */
typedef struct sSmushVecGzFile
{
    int used;
    int pos;
    int id;
    uint8_t aData[600];
} SmushVecGzFile;

static SmushVecGzFile smushvec_gz[4];
static int smushvec_gzNext;

static int SmushVec_GzId(gzFile file)
{
    for ( int i = 0; i < 4; ++i )
    {
        if ( file == (gzFile)&smushvec_gz[i] )
        {
            return smushvec_gz[i].id;
        }
    }

    return file ? -1 : 0;
}

static gzFile J3DAPI SmushVec_GzOpen(const char* pPath, const char* pMode)
{
    SmushVec_Log8('o');
    SmushVec_Log32((int)strlen(pPath));
    SmushVec_Log8(pMode[0]);
    if ( strstr(pPath, "bad") )
    {
        return NULL;
    }

    for ( int i = 0; i < 4; ++i )
    {
        if ( !smushvec_gz[i].used )
        {
            smushvec_gz[i].used = 1;
            smushvec_gz[i].pos  = 0;
            smushvec_gz[i].id   = ++smushvec_gzNext;
            SmushVec_RandBytes(smushvec_gz[i].aData, sizeof(smushvec_gz[i].aData), 0x62000000u + (uint32_t)strlen(pPath));
            return (gzFile)&smushvec_gz[i];
        }
    }

    return NULL;
}

static int J3DAPI SmushVec_GzClose(gzFile file)
{
    SmushVec_Log8('c');
    SmushVec_Log32(SmushVec_GzId(file));
    for ( int i = 0; i < 4; ++i )
    {
        if ( file == (gzFile)&smushvec_gz[i] )
        {
            smushvec_gz[i].used = 0;
        }
    }

    return 0;
}

static int J3DAPI SmushVec_GzRead(gzFile file, void* pBuf, unsigned int len)
{
    SmushVec_Log8('r');
    SmushVec_Log32(SmushVec_GzId(file));
    SmushVec_Log32((int)len);
    SmushVecGzFile* pFile = (SmushVecGzFile*)file;
    int avail = (int)sizeof(pFile->aData) - pFile->pos;
    int n = avail < (int)len ? avail : (int)len;
    if ( n <= 0 )
    {
        return 0;
    }

    memcpy(pBuf, pFile->aData + pFile->pos, (size_t)n);
    pFile->pos += n;
    return n;
}

static int J3DAPI SmushVec_GzWrite(gzFile file, const void* pBuf, unsigned int len)
{
    SmushVec_Log8('w');
    SmushVec_Log32(SmushVec_GzId(file));
    SmushVec_Log32((int)len);
    SmushVec_Log32((int)(uint32_t)SmushVec_Hash(pBuf, len));
    return (int)len - 1;
}

static int J3DAPI SmushVec_GzFlush(gzFile file, int flush)
{
    SmushVec_Log8('f');
    SmushVec_Log32(SmushVec_GzId(file));
    SmushVec_Log32(flush);
    return 0;
}

static long J3DAPI SmushVec_GzSeek(gzFile file, long offset, int whence)
{
    SmushVec_Log8('k');
    SmushVec_Log32(SmushVec_GzId(file));
    SmushVec_Log32((int)offset);
    SmushVec_Log32(whence);
    SmushVecGzFile* pFile = (SmushVecGzFile*)file;
    pFile->pos = (int)offset;
    return offset;
}

static void SmushVec_ZRecord(ZfileHandle* pFile, const char* pName)
{
    if ( !pFile )
    {
        SmushVec_Int(0, "%s: handle", pName);
        return;
    }

    int aState[5];
    aState[0] = 1;
    aState[1] = pFile->isGz ? SmushVec_GzId(pFile->hFile) : (int)(intptr_t)pFile->hFile;
    aState[2] = pFile->isGz;
    aState[3] = pFile->isGz ? pFile->gzSize : 0;
    aState[4] = pFile->isGz ? (pFile->gzPos * 1000 + pFile->pos) : 0;
    SmushVec_Raw(aState, sizeof(aState), "%s: handle", pName);
}

static void SmushVec_RunZfilio(const SmushVecApi* pApi, SmushVecSink* pSink, uint8_t fill)
{
    SmushVec_Begin(pApi, pSink, fill);
    memset(smushvec_files, 0, sizeof(smushvec_files));
    memset(smushvec_gz, 0, sizeof(smushvec_gz));
    smushvec_gzNext  = 0;
    smushvec_noDebug = 0;

    // The chained files; a .gz file's last 4 bytes give the uncompressed size
    uint8_t aGz[64];
    SmushVec_RandBytes(aGz, sizeof(aGz), 0x6A000000u);
    int gzSize = 500;
    memcpy(aGz + 60, &gzSize, 4);
    SmushVec_FileAdd("movie.gz", aGz, 64);
    SmushVec_FileAdd("movie.znm", aGz, 64);
    SmushVec_FileAdd("plain.txt", "plain text file", 15);
    SmushVec_FileAdd("short.gz", "ab", 2);
    SmushVec_FileAdd("movie.GZ", aGz, 64);

    pApi->ZfilioReset();
    SmushVecGz gz = { SmushVec_GzOpen, SmushVec_GzClose, SmushVec_GzRead, SmushVec_GzWrite, SmushVec_GzFlush, SmushVec_GzSeek };
    pApi->ZfilioSetGz(&gz);

    SmushVec_Int(pApi->zfilio_Register(&sv.share, (void*)"param"), "zfilio_Register rc");
    SmushVec_Int(pApi->zfilio_Startup(), "zfilio_Startup rc");
    SmushVec_LogFlush("zfilio register and startup");
    ZfilioInterface* pZ = (ZfilioInterface*)SmushVec_Published("FILIO=insane_zfilio");
    if ( !pZ )
    {
        pApi->ZfilioSetGz(NULL);
        return;
    }

    // Before .znm is configured it is chained; read-only modes only
    static const struct { const char* pPath; const char* pMode; } aOpen[] = {
        { "movie.znm", "rb" }, { "movie.gz", "rb" }, { "movie.gz", "r" }, { "movie.gz", "R" }, { "movie.gz", "r+b" }, { "movie.gz", "rb+" },
        { "movie.gz", "wb" }, { "movie.GZ", "rb" }, { "plain.txt", "rb" }, { "missing.txt", "rb" }, { "bad.gz", "rb" }, { "short.gz", "rb" },
    };
    for ( int pass = 0; pass < 2; ++pass )
    {
        for ( size_t i = 0; i < sizeof(aOpen) / sizeof(aOpen[0]); ++i )
        {
            char aName[64];
            snprintf(aName, sizeof(aName), "zfilio pass %d open %s %s", pass, aOpen[i].pPath, aOpen[i].pMode);
            SmushVec_SectionBegin("%s", aName);
            SmushVec_AllocReset();
            ZfileHandle* pFile = pZ->Open(aOpen[i].pPath, aOpen[i].pMode);
            if ( pFile && pFile->isGz && !strcmp(aOpen[i].pPath, "short.gz") )
            {
                pFile->gzSize = 0; // the original leaves it uninitialized after a short read: not compared
            }

            SmushVec_ZRecord(pFile, aName);

            if ( pFile )
            {
                pZ->Close(pFile);
            }

            SmushVec_AllocFlush(aName);
            SmushVec_LogFlush(aName);
            SmushVec_SectionEnd();
        }

        if ( pass == 0 )
        {
            SmushVec_Int(pApi->zfilio_Configure(1, ".znm", 1, ".snm", 0), "zfilio_Configure rc");
            SmushVec_Int(pApi->zfilio_Configure(3, ".xyz", 0), "zfilio_Configure unknown tag rc");
        }
    }

    // gz file: reads, lazy seeks, tell, write, flush
    ZfileHandle* pFile = pZ->Open("movie.gz", "rb");
    uint8_t aBuf[256];
    if ( pFile )
    {
        SmushVec_SectionBegin("zfilio gz operations");
        static const struct { int op, a, b; } aOps[] = {
            { 'r', 10, 0 }, { 'r', 20, 0 }, { 't', 0, 0 }, { 's', 5, 0 }, { 'r', 8, 0 }, { 's', 3, 1 }, { 't', 0, 0 }, { 'r', 4, 0 },
            { 's', -10, 2 }, { 't', 0, 0 }, { 'r', 30, 0 }, { 's', 0, 7 }, { 't', 0, 0 }, { 'w', 6, 0 }, { 't', 0, 0 }, { 'f', 0, 0 },
            { 'r', 1, 0 }, { 's', 100, 0 }, { 'r', 0, 0 }, { 't', 0, 0 },
        };
        for ( size_t i = 0; i < sizeof(aOps) / sizeof(aOps[0]); ++i )
        {
            int rc = 0;
            memset(aBuf, 0xEE, sizeof(aBuf));
            switch ( aOps[i].op )
            {
                case 'r': rc = pZ->Read(pFile, aBuf, aOps[i].a); break;
                case 't': rc = pZ->Tell(pFile); break;
                case 's': pZ->Seek(pFile, aOps[i].a, aOps[i].b); break; // returns no value for gz files
                case 'w': rc = pZ->Write(pFile, "gzdata", aOps[i].a); break;
                case 'f': pZ->Flush(pFile); break;
            }

            SmushVec_Int(rc, "zfilio gz op %d %c rc", (int)i, aOps[i].op);
            SmushVec_Raw(aBuf, 40, "zfilio gz op %d buffer", (int)i);
            SmushVec_ZRecord(pFile, "zfilio gz op");
            SmushVec_LogFlush("zfilio gz op");
        }

        pZ->Close(pFile);
        SmushVec_LogFlush("zfilio gz close");
        SmushVec_SectionEnd();
    }

    // chained file through the same interface
    pFile = pZ->Open("plain.txt", "rb");
    if ( pFile )
    {
        SmushVec_SectionBegin("zfilio chained operations");
        memset(aBuf, 0xEE, sizeof(aBuf));
        SmushVec_Int(pZ->Read(pFile, aBuf, 6), "zfilio chained read rc");
        SmushVec_Raw(aBuf, 16, "zfilio chained read");
        SmushVec_Int(pZ->Seek(pFile, 2, 1), "zfilio chained seek rc");
        SmushVec_Int(pZ->Tell(pFile), "zfilio chained tell");
        SmushVec_Int(pZ->Write(pFile, "XY", 2), "zfilio chained write rc");
        pZ->Flush(pFile);
        pZ->Close(pFile);
        SmushVec_FileFlush("zfilio chained");
        SmushVec_SectionEnd();
    }

    // Allocation failure of the handle, with and without the debug service
    sv.failAlloc = 1;
    SmushVec_Int(pZ->Open("movie.gz", "rb") == NULL, "zfilio open alloc fails");
    SmushVec_LogFlush("zfilio open alloc fails");
    sv.failAlloc = 0;

    // Fill the extension table: the last free slot, then a full table (the extension argument is then read as the
    // next tag)
    int added = 0;
    while ( added < 60 )
    {
        int rc = pApi->zfilio_Configure(1, ".e", 0);
        added++;
        if ( sv.logSize > 0 )
        {
            break; // the "too many" message was printed
        }

        if ( rc )
        {
            break;
        }
    }

    SmushVec_Int(added, "zfilio_Configure until full");
    SmushVec_LogFlush("zfilio_Configure full");
    SmushVec_Int(pApi->zfilio_Configure(1, ".f", 1, ".g", 0), "zfilio_Configure full rc");
    SmushVec_LogFlush("zfilio_Configure full again");

    pApi->zfilio_Shutdown();
    pApi->zfilio_Unregister();
    SmushVec_LogFlush("zfilio shutdown and unregister");

    // Startup without the debug service and without memory
    smushvec_noDebug = 1;
    SmushVec_Int(pApi->zfilio_Startup(), "zfilio_Startup without debug rc");
    pApi->zfilio_Shutdown();
    SmushVec_LogFlush("zfilio without debug");
    smushvec_noDebug = 0;

    pApi->ZfilioSetGz(NULL);
    pApi->ZfilioReset();
}

#endif // SMUSH_TESTS_SMUSHVECCASES_H
