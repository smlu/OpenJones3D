#include <unity_fixture.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <smush/insane/Bitmap/BlitAsm.h>
#include <smush/insane/Bitmap/MemCopy.h>
#include <smush/insane/Bitmap/MemFill.h>
#include <smush/insane/Decode/DecodeAsm.h>
#include <smush/insane/Flobject/Bloc16Asm.h>
#include <smush/insane/Sound/MixerAsm.h>
#include <std/General/stdUtil.h>

// Original-engine vectors of the C translations of INSANE's assembly routines (Bloc16Asm, DecodeAsm, MixerAsm, BlitAsm,
// MemCopy, MemFill). File format, generators and provenance: tv/README.md.

#define SMUSHASM_TEST_MAGIC        "SATV"
#define SMUSHASM_TEST_VERSION      1u
#define SMUSHASM_TEST_MAX_BUFFERS  8
#define SMUSHASM_TEST_MAX_ARGS     12
#define SMUSHASM_TEST_GUARD_SIZE   64
#define SMUSHASM_TEST_GUARD_FILL   0xCC
#define SMUSHASM_TEST_MAX_REPORTED 20

typedef enum eSmushAsmTestProc
{
    SMUSHASM_PROC_INTERPOLATE   = 1,
    SMUSHASM_PROC_CODEC2        = 2,
    SMUSHASM_PROC_LUT8          = 10,
    SMUSHASM_PROC_LUT16         = 11,
    SMUSHASM_PROC_RGBATO555     = 12,
    SMUSHASM_PROC_555TORGBA     = 13,
    SMUSHASM_PROC_RGBATO565     = 14,
    SMUSHASM_PROC_565TORGBA     = 15,
    SMUSHASM_PROC_RGBATO565B    = 16,
    SMUSHASM_PROC_565BTORGBA    = 17,
    SMUSHASM_PROC_MIXMEMCOPY    = 20,
    SMUSHASM_PROC_MIXMEMFILL    = 21,
    SMUSHASM_PROC_MIXLUT16      = 22,
    SMUSHASM_PROC_MIXLUT16ADD   = 23,
    SMUSHASM_PROC_MIXFILL16     = 24,
    SMUSHASM_PROC_MIXLUT8       = 25,
    SMUSHASM_PROC_MIXLUT8ADD    = 26,
    SMUSHASM_PROC_MIXFILLB      = 27,
    SMUSHASM_PROC_BLITMASM      = 30,
    SMUSHASM_PROC_BLITMEMCOPY   = 31,
    SMUSHASM_PROC_BLITMEMCOPYB  = 32,
} SmushAsmTestProc;

typedef struct sSmushAsmTestReader
{
    const uint8_t* pData;
    size_t size;
    size_t pos;
    int bError;
} SmushAsmTestReader;

typedef struct sSmushAsmTestBuffer
{
    uint32_t size;
    uint8_t* pAlloc;     // guard + buffer + guard
    uint8_t* pData;
    uint8_t* pExpected;  // expected contents when outKind is 0 or 1
    uint8_t outKind;
    uint64_t hash;       // outKind 2
} SmushAsmTestBuffer;

TEST_GROUP(SmushAsm);

static void* SmushAsmTest_pGuardAllocation;

TEST_SETUP(SmushAsm)
{
    SmushAsmTest_pGuardAllocation = NULL;
}

TEST_TEAR_DOWN(SmushAsm)
{
    if ( SmushAsmTest_pGuardAllocation )
    {
        VirtualFree(SmushAsmTest_pGuardAllocation, 0, MEM_RELEASE);
        SmushAsmTest_pGuardAllocation = NULL;
    }
}

static const uint8_t* SmushAsmTest_Read(SmushAsmTestReader* pReader, size_t size)
{
    if ( pReader->bError || size > pReader->size - pReader->pos )
    {
        pReader->bError = 1;
        return NULL;
    }

    const uint8_t* p = pReader->pData + pReader->pos;
    pReader->pos += size;
    return p;
}

static uint8_t SmushAsmTest_ReadU8(SmushAsmTestReader* pReader)
{
    const uint8_t* p = SmushAsmTest_Read(pReader, 1);
    return p ? p[0] : 0;
}

static uint32_t SmushAsmTest_ReadU32(SmushAsmTestReader* pReader)
{
    const uint8_t* p = SmushAsmTest_Read(pReader, 4);
    return p ? (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24) : 0;
}

static uint64_t SmushAsmTest_ReadU64(SmushAsmTestReader* pReader)
{
    uint64_t lo = SmushAsmTest_ReadU32(pReader);
    uint64_t hi = SmushAsmTest_ReadU32(pReader);
    return lo | (hi << 32);
}

static uint32_t SmushAsmTest_Lcg(uint32_t* pState)
{
    *pState = *pState * 1664525u + 1013904223u;
    return *pState;
}

static uint64_t SmushAsmTest_Hash(const uint8_t* pData, size_t size)
{
    // FNV-1a, 64-bit
    uint64_t hash = 0xCBF29CE484222325ull;
    for ( size_t i = 0; i < size; ++i )
    {
        hash ^= pData[i];
        hash *= 0x100000001B3ull;
    }

    return hash;
}

// Reads a buffer description and builds its initial contents in pBuf->pData
static int SmushAsmTest_ReadBuffer(SmushAsmTestReader* pReader, SmushAsmTestBuffer* pBuf)
{
    uint32_t size = SmushAsmTest_ReadU32(pReader);
    if ( pReader->bError || size > 0x4000000u )
    {
        return 0;
    }

    pBuf->size      = size;
    pBuf->pAlloc    = (uint8_t*)malloc(size + 2 * SMUSHASM_TEST_GUARD_SIZE);
    pBuf->pExpected = (uint8_t*)malloc(size + 1);
    if ( !pBuf->pAlloc || !pBuf->pExpected )
    {
        return 0;
    }

    memset(pBuf->pAlloc, SMUSHASM_TEST_GUARD_FILL, size + 2 * SMUSHASM_TEST_GUARD_SIZE);
    pBuf->pData = pBuf->pAlloc + SMUSHASM_TEST_GUARD_SIZE;

    uint8_t* p = pBuf->pData;
    uint8_t initKind = SmushAsmTest_ReadU8(pReader);
    switch ( initKind )
    {
        case 0: // fill
            memset(p, SmushAsmTest_ReadU8(pReader), size);
            break;

        case 1: // random bytes
        {
            uint32_t state = SmushAsmTest_ReadU32(pReader);
            for ( uint32_t i = 0; i < size; ++i )
            {
                p[i] = (uint8_t)(SmushAsmTest_Lcg(&state) >> 24);
            }
            break;
        }

        case 2: // stored
        {
            const uint8_t* pBytes = SmushAsmTest_Read(pReader, size);
            if ( pBytes )
            {
                memcpy(p, pBytes, size);
            }
            break;
        }

        case 3: // random words
        case 5: // glyph table: random words, then two random count bytes per entry
        {
            uint32_t state       = SmushAsmTest_ReadU32(pReader);
            uint32_t mod         = SmushAsmTest_ReadU32(pReader);
            int32_t bias         = 0;
            uint32_t entrySize   = 0;
            uint32_t countOffset = 0;
            uint32_t maxCount    = 0;
            if ( initKind == 3 )
            {
                bias = (int32_t)SmushAsmTest_ReadU32(pReader);
            }
            else
            {
                entrySize   = SmushAsmTest_ReadU32(pReader);
                countOffset = SmushAsmTest_ReadU32(pReader);
                maxCount    = SmushAsmTest_ReadU32(pReader);
            }

            if ( pReader->bError || mod == 0 || (initKind == 5 && entrySize == 0) )
            {
                return 0;
            }

            for ( uint32_t i = 0; i + 1 < size; i += 2 )
            {
                uint16_t word = (uint16_t)((int32_t)((SmushAsmTest_Lcg(&state) >> 8) % mod) + bias);
                p[i]          = (uint8_t)word;
                p[i + 1]      = (uint8_t)(word >> 8);
            }

            if ( initKind == 5 )
            {
                for ( uint32_t entry = 0; entry * entrySize + countOffset + 2 <= size; ++entry )
                {
                    p[entry * entrySize + countOffset]     = (uint8_t)((SmushAsmTest_Lcg(&state) >> 8) % (maxCount + 1));
                    p[entry * entrySize + countOffset + 1] = (uint8_t)((SmushAsmTest_Lcg(&state) >> 8) % (maxCount + 1));
                }
            }
            break;
        }

        case 4: // sequential elements
        {
            uint8_t elemSize = SmushAsmTest_ReadU8(pReader);
            uint32_t start   = SmushAsmTest_ReadU32(pReader);
            if ( elemSize == 0 || elemSize > 4 )
            {
                return 0;
            }

            for ( uint32_t i = 0; i + elemSize <= size; i += elemSize )
            {
                uint32_t value = start + i / elemSize;
                for ( uint32_t k = 0; k < elemSize; ++k )
                {
                    p[i + k] = (uint8_t)(value >> (8 * k));
                }
            }
            break;
        }

        default:
            return 0;
    }

    uint32_t numPatches = SmushAsmTest_ReadU32(pReader);
    for ( uint32_t i = 0; i < numPatches && !pReader->bError; ++i )
    {
        uint32_t offset       = SmushAsmTest_ReadU32(pReader);
        uint32_t length       = SmushAsmTest_ReadU32(pReader);
        const uint8_t* pBytes = SmushAsmTest_Read(pReader, length);
        if ( !pBytes || offset > size || length > size - offset )
        {
            return 0;
        }

        memcpy(p + offset, pBytes, length);
    }

    memcpy(pBuf->pExpected, p, size);
    pBuf->outKind = SmushAsmTest_ReadU8(pReader);
    switch ( pBuf->outKind )
    {
        case 0: // unchanged
            break;

        case 1: // changed range
        {
            uint32_t start        = SmushAsmTest_ReadU32(pReader);
            uint32_t length       = SmushAsmTest_ReadU32(pReader);
            const uint8_t* pBytes = SmushAsmTest_Read(pReader, length);
            if ( !pBytes || start > size || length > size - start )
            {
                return 0;
            }

            memcpy(pBuf->pExpected + start, pBytes, length);
            break;
        }

        case 2: // hash of the whole buffer
            pBuf->hash = SmushAsmTest_ReadU64(pReader);
            break;

        default:
            return 0;
    }

    return !pReader->bError;
}

static void SmushAsmTest_Call(uint32_t proc, const uintptr_t* a)
{
#define SMUSHASM_PTR(i) ((void*)a[i])
#define SMUSHASM_INT(i) ((int)(uint32_t)a[i])
    switch ( proc )
    {
        case SMUSHASM_PROC_INTERPOLATE:
            dbloc16_Interpolate((uint8_t*)SMUSHASM_PTR(0), (const uint8_t*)SMUSHASM_PTR(1), SMUSHASM_INT(2), SMUSHASM_INT(3), (const uint8_t*)SMUSHASM_PTR(4));
            break;
        case SMUSHASM_PROC_CODEC2:
            dbloc16_Codec2((uint16_t*)SMUSHASM_PTR(0), (uint8_t*)SMUSHASM_PTR(1), SMUSHASM_INT(2), SMUSHASM_INT(3), SMUSHASM_INT(4), (int16_t*)SMUSHASM_PTR(5),
                (uint8_t*)SMUSHASM_PTR(6), SMUSHASM_INT(7), SMUSHASM_INT(8), (Bl16Small*)SMUSHASM_PTR(9), (Bl16Big*)SMUSHASM_PTR(10), (uint16_t*)SMUSHASM_PTR(11));
            break;
        case SMUSHASM_PROC_LUT8:
            decode_Lut8(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2), SMUSHASM_INT(3), SMUSHASM_PTR(4));
            break;
        case SMUSHASM_PROC_LUT16:
            decode_Lut16(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2), SMUSHASM_INT(3), SMUSHASM_PTR(4));
            break;
        case SMUSHASM_PROC_RGBATO555:
            decode_RGBAto555(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_555TORGBA:
            decode_555toRGBA(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_RGBATO565:
            decode_RGBAto565(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_565TORGBA:
            decode_565toRGBA(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_RGBATO565B:
            decode_RGBAto565B(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_565BTORGBA:
            decode_565BtoRGBA(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_MIXMEMCOPY:
            mixer_MemCopy(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_MIXMEMFILL:
            mixer_MemFill(SMUSHASM_PTR(0), SMUSHASM_INT(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_MIXLUT16:
            mixer_Lut16((uint16_t*)SMUSHASM_PTR(0), (const uint8_t*)SMUSHASM_PTR(1), SMUSHASM_INT(2), (const uint16_t*)SMUSHASM_PTR(3), SMUSHASM_INT(4));
            break;
        case SMUSHASM_PROC_MIXLUT16ADD:
            mixer_Lut16Add((uint16_t*)SMUSHASM_PTR(0), (const uint8_t*)SMUSHASM_PTR(1), SMUSHASM_INT(2), (const uint16_t*)SMUSHASM_PTR(3), SMUSHASM_INT(4));
            break;
        case SMUSHASM_PROC_MIXFILL16:
            mixer_Fill16((uint16_t*)SMUSHASM_PTR(0), SMUSHASM_INT(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_MIXLUT8:
            mixer_Lut8((uint8_t*)SMUSHASM_PTR(0), (const uint8_t*)SMUSHASM_PTR(1), SMUSHASM_INT(2), (const uint8_t*)SMUSHASM_PTR(3), SMUSHASM_INT(4));
            break;
        case SMUSHASM_PROC_MIXLUT8ADD:
            mixer_Lut8Add((uint8_t*)SMUSHASM_PTR(0), (const uint8_t*)SMUSHASM_PTR(1), SMUSHASM_INT(2), (const uint8_t*)SMUSHASM_PTR(3), SMUSHASM_INT(4));
            break;
        case SMUSHASM_PROC_MIXFILLB:
            mixer_FillB(SMUSHASM_PTR(0), SMUSHASM_INT(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_BLITMASM:
            blit_Masm(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2), SMUSHASM_INT(3), SMUSHASM_INT(4), SMUSHASM_INT(5), SMUSHASM_INT(6), SMUSHASM_INT(7),
                SMUSHASM_INT(8), SMUSHASM_INT(9));
            break;
        case SMUSHASM_PROC_BLITMEMCOPY:
            blit_MemCopy(SMUSHASM_PTR(0), SMUSHASM_PTR(1), SMUSHASM_INT(2));
            break;
        case SMUSHASM_PROC_BLITMEMCOPYB:
            blit_MemCopyB(SMUSHASM_PTR(0), (uint8_t)a[1], SMUSHASM_INT(2));
            break;
        default:
            TEST_FAIL_MESSAGE("Unknown procedure in vector file");
    }
#undef SMUSHASM_PTR
#undef SMUSHASM_INT
}

static int SmushAsmTest_CheckBuffer(const SmushAsmTestBuffer* pBuf)
{
    for ( uint32_t i = 0; i < SMUSHASM_TEST_GUARD_SIZE; ++i )
    {
        if ( pBuf->pAlloc[i] != SMUSHASM_TEST_GUARD_FILL || pBuf->pData[pBuf->size + i] != SMUSHASM_TEST_GUARD_FILL )
        {
            return 0;
        }
    }

    if ( pBuf->outKind == 2 )
    {
        return SmushAsmTest_Hash(pBuf->pData, pBuf->size) == pBuf->hash;
    }

    return memcmp(pBuf->pData, pBuf->pExpected, pBuf->size) == 0;
}

// The unchanged corpus also deliberately exercises motion outside the logical
// image inside a larger scratch allocation. Replay the original byte operations
// independently, verify them against the captured bytes/hash, then track only
// bytes descended from an invalid pixel read. Those bytes alone become zero in
// the bounded expectation, including propagation through later overlap copies.
typedef struct sSmushAsmTestMotionReplay
{
    uint8_t* pOriginal;
    uint8_t* pInvalid;
    size_t size;
    int64_t current, previous, delta;
    size_t imageSize;
    int lineSize;
    const int16_t* pMotion;
    const uint8_t* pParams;
    const Bl16Small* pSmall;
    const Bl16Big* pBig;
    const uint16_t* pPalette;
    int bounded;
    int (*pfnContains)(uintptr_t address, size_t size); // sequence harness: mapped bitmap allocations
} SmushAsmTestMotionReplay;

static uint16_t SmushAsmTest_LoadWord(const uint8_t* p)
{
    uint16_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void SmushAsmTest_ReplayStore(SmushAsmTestMotionReplay* p, int64_t at, uint16_t value)
{
    if ( p->pfnContains )
    {
        TEST_ASSERT_TRUE(at >= 0 && (uint64_t)at <= UINTPTR_MAX);
        TEST_ASSERT_TRUE(p->pfnContains((uintptr_t)at, sizeof(value)));
        memcpy((void*)(uintptr_t)at, &value, sizeof(value));
        return;
    }
    TEST_ASSERT_TRUE(at >= 0 && (uint64_t)at + 2u <= p->size);
    memcpy(p->pOriginal + (size_t)at, &value, sizeof(value));
    memset(p->pInvalid + (size_t)at, 0, sizeof(value));
}

static void SmushAsmTest_ReplayRead(SmushAsmTestMotionReplay* p, int64_t at, int64_t image, uint8_t value[4], uint8_t invalid[4])
{
    if ( p->pfnContains )
    {
        TEST_ASSERT_TRUE(at >= 0 && (uint64_t)at <= UINTPTR_MAX);
        TEST_ASSERT_TRUE(p->pfnContains((uintptr_t)at, 4u));
        memcpy(value, (const void*)(uintptr_t)at, 4u);
        memset(invalid, 0, 4u);
        if ( p->bounded )
        {
            for ( unsigned word = 0; word < 2u; ++word )
            {
                int64_t pixel = at + word * 2u - image;
                if ( pixel < 0 || (uint64_t)pixel + 2u > p->imageSize )
                {
                    memset(value + word * 2u, 0, 2u);
                }
            }
        }
        return;
    }
    TEST_ASSERT_TRUE(at >= 0 && (uint64_t)at + 4u <= p->size);
    memcpy(value, p->pOriginal + (size_t)at, 4u);
    for ( unsigned k = 0; k < 4u; k += 2u )
    {
        int64_t pixel = at + k - image;
        int outside = pixel < 0 || (uint64_t)pixel + 2u > p->imageSize;
        invalid[k] = (uint8_t)(outside || p->pInvalid[(size_t)at + k]);
        invalid[k + 1u] = (uint8_t)(outside || p->pInvalid[(size_t)at + k + 1u]);
    }
}

static void SmushAsmTest_ReplayPairStore(SmushAsmTestMotionReplay* p, int64_t at, const uint8_t value[4], const uint8_t invalid[4])
{
    if ( p->pfnContains )
    {
        TEST_ASSERT_TRUE(at >= 0 && (uint64_t)at <= UINTPTR_MAX);
        TEST_ASSERT_TRUE(p->pfnContains((uintptr_t)at, 4u));
        memcpy((void*)(uintptr_t)at, value, 4u);
        return;
    }
    TEST_ASSERT_TRUE(at >= 0 && (uint64_t)at + 4u <= p->size);
    memcpy(p->pOriginal + (size_t)at, value, 4u);
    memcpy(p->pInvalid + (size_t)at, invalid, 4u);
}

static const uint8_t* SmushAsmTest_ReplayBlock(SmushAsmTestMotionReplay* p, const uint8_t* in, int64_t block, int size)
{
    unsigned op = *in++;
    if ( op <= 0xF6u )
    {
        int64_t image = op == 0xF6u ? p->delta : p->previous;
        int motion = 0;
        if ( op == 0xF5u )
        {
            motion = (int16_t)SmushAsmTest_LoadWord(in) * 2;
            in += 2;
        }
        else if ( op < 0xF5u )
        {
            motion = p->pMotion[op] * 2;
        }
        int64_t displacement = image - p->current + motion;
        if ( size == 2 )
        {
            for ( unsigned row = 0; row < 2u; ++row )
            {
                uint8_t value[4], invalid[4];
                int64_t at = block + row * p->lineSize;
                SmushAsmTest_ReplayRead(p, at + displacement, image, value, invalid);
                SmushAsmTest_ReplayPairStore(p, at, value, invalid);
            }
        }
        else
        {
            for ( int row = 0; row < size / 2; ++row )
            {
                for ( int x = 0; x < size * 2; x += 4 )
                {
                    uint8_t top[4], bottom[4], topInvalid[4], bottomInvalid[4];
                    int64_t upper = block + row * p->lineSize + x;
                    int64_t lower = upper + (size / 2) * p->lineSize;
                    SmushAsmTest_ReplayRead(p, lower + displacement, image, bottom, bottomInvalid);
                    SmushAsmTest_ReplayRead(p, upper + displacement, image, top, topInvalid);
                    SmushAsmTest_ReplayPairStore(p, lower, bottom, bottomInvalid);
                    SmushAsmTest_ReplayPairStore(p, upper, top, topInvalid);
                }
            }
        }
        return in;
    }
    if ( op == 0xFFu && size > 2 )
    {
        in = SmushAsmTest_ReplayBlock(p, in, block, size / 2);
        in = SmushAsmTest_ReplayBlock(p, in, block + size, size / 2);
        in = SmushAsmTest_ReplayBlock(p, in, block + (size / 2) * p->lineSize, size / 2);
        return SmushAsmTest_ReplayBlock(p, in, block + (size / 2) * p->lineSize + size, size / 2);
    }
    if ( size == 2 && (op == 0xFFu || op == 0xF8u || op == 0xF7u) )
    {
        for ( unsigned pixel = 0; pixel < 4u; ++pixel )
        {
            uint16_t color = op == 0xF7u ? p->pPalette[*in++] : SmushAsmTest_LoadWord(in);
            if ( op != 0xF7u )
            {
                in += 2;
            }
            SmushAsmTest_ReplayStore(p, block + (pixel / 2u) * p->lineSize + (pixel % 2u) * 2u, color);
        }
        return in;
    }
    if ( op == 0xF7u || op == 0xF8u )
    {
        unsigned glyph = *in++;
        uint16_t colors[2];
        if ( op == 0xF7u )
        {
            colors[0] = p->pPalette[*in++];
            colors[1] = p->pPalette[*in++];
        }
        else
        {
            colors[0] = SmushAsmTest_LoadWord(in);
            colors[1] = SmushAsmTest_LoadWord(in + 2);
            in += 4;
        }
        const uint16_t* offsets[2] = { size == 4 ? p->pSmall[glyph].off1 : p->pBig[glyph].off1,
                                      size == 4 ? p->pSmall[glyph].off2 : p->pBig[glyph].off2 };
        unsigned counts[2] = { size == 4 ? p->pSmall[glyph].n1 : p->pBig[glyph].n1,
                               size == 4 ? p->pSmall[glyph].n2 : p->pBig[glyph].n2 };
        for ( unsigned color = 0; color < 2u; ++color )
        {
            unsigned count = counts[color] ? counts[color] : 1u;
            for ( unsigned k = 0; k < count; ++k )
            {
                SmushAsmTest_ReplayStore(p, block + offsets[color][k] * 2u, colors[color]);
            }
        }
        return in;
    }
    uint16_t color;
    if ( op == 0xFEu )
    {
        color = SmushAsmTest_LoadWord(in);
        in += 2;
    }
    else if ( op == 0xFDu )
    {
        color = p->pPalette[*in++];
    }
    else
    {
        color = SmushAsmTest_LoadWord(p->pParams + (op - 0xF9u) * 2u);
    }
    for ( int row = 0; row < size; ++row )
    {
        for ( int x = 0; x < size; ++x )
        {
            SmushAsmTest_ReplayStore(p, block + row * p->lineSize + x * 2, color);
        }
    }
    return in;
}

// Shared only by the original-vector tests. The Contains callback proves each
// address belongs to a harness allocation, including its intentional margins.
// The sequence test first verifies the unbounded reference against the original
// corpus, then uses bounded reference output as its exact adapted expectation.
void SmushAsmTest_ReplayCodec2(uint16_t* pDst, uint8_t* pSrc, int offset1, int offset2, int width, int16_t* pMotion, uint8_t* pParams, int height, int nextLine, Bl16Small* pSmall, Bl16Big* pBig, uint16_t* pPalette, int bounded, int (*pfnContains)(uintptr_t, size_t))
{
    J3D_UNUSED(nextLine);
    TEST_ASSERT_NOT_NULL(pfnContains);
    SmushAsmTestMotionReplay replay = { 0 };
    replay.current = (uintptr_t)pDst;
    replay.previous = (uintptr_t)pDst + (intptr_t)(int32_t)((uint32_t)offset1 * 2u);
    replay.delta = (uintptr_t)pDst + (intptr_t)(int32_t)((uint32_t)offset2 * 2u);
    replay.lineSize = (int32_t)((uint32_t)width * 2u);
    replay.imageSize = (size_t)(uint32_t)width * (((uint32_t)height + 7u) & ~7u) * 2u;
    replay.pMotion = pMotion;
    replay.pParams = pParams;
    replay.pSmall = pSmall;
    replay.pBig = pBig;
    replay.pPalette = pPalette;
    replay.bounded = bounded;
    replay.pfnContains = pfnContains;
    unsigned rows = (uint8_t)(((uint32_t)height + 7u) / 8u);
    unsigned cols = (uint8_t)(((uint32_t)width + 7u) / 8u);
    if ( !rows )
    {
        rows = 256u;
    }
    if ( !cols )
    {
        cols = 256u;
    }
    const uint8_t* in = pSrc;
    int64_t block = replay.current;
    for ( unsigned row = 0; row < rows; ++row )
    {
        for ( unsigned col = 0; col < cols; ++col, block += 16 )
        {
            in = SmushAsmTest_ReplayBlock(&replay, in, block, 8);
        }
        block += (uint32_t)replay.lineSize * 7u;
    }
}

static void SmushAsmTest_AdaptBoundedMotion(const uintptr_t a[12], SmushAsmTestBuffer* aBuffers, unsigned numBuffers, const char* pName)
{
    SmushAsmTestBuffer* pImage = NULL;
    for ( unsigned i = 0; i < numBuffers; ++i )
    {
        uintptr_t base = (uintptr_t)aBuffers[i].pData;
        if ( a[0] >= base && a[0] - base < aBuffers[i].size )
        {
            pImage = &aBuffers[i];
        }
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(pImage, pName);
    SmushAsmTestMotionReplay replay = { 0 };
    replay.size = pImage->size + 2u * SMUSHASM_TEST_GUARD_SIZE;
    replay.pOriginal = malloc(replay.size);
    replay.pInvalid = calloc(replay.size, 1u);
    TEST_ASSERT_NOT_NULL(replay.pOriginal);
    TEST_ASSERT_NOT_NULL(replay.pInvalid);
    memcpy(replay.pOriginal, pImage->pAlloc, replay.size);
    replay.current = (int64_t)(a[0] - (uintptr_t)pImage->pAlloc);
    replay.previous = replay.current + (int32_t)((uint32_t)a[2] * 2u);
    replay.delta = replay.current + (int32_t)((uint32_t)a[3] * 2u);
    replay.lineSize = (int32_t)((uint32_t)a[4] * 2u);
    replay.imageSize = (size_t)(uint32_t)a[4] * (((uint32_t)a[7] + 7u) & ~7u) * 2u;
    replay.pMotion = (const int16_t*)a[5];
    replay.pParams = (const uint8_t*)a[6];
    replay.pSmall = (const Bl16Small*)a[9];
    replay.pBig = (const Bl16Big*)a[10];
    replay.pPalette = (const uint16_t*)a[11];
    unsigned rows = (uint8_t)(((uint32_t)a[7] + 7u) / 8u);
    unsigned cols = (uint8_t)(((uint32_t)a[4] + 7u) / 8u);
    if ( !rows )
    {
        rows = 256u;
    }
    if ( !cols )
    {
        cols = 256u;
    }
    const uint8_t* in = (const uint8_t*)a[1];
    int64_t block = replay.current;
    for ( unsigned row = 0; row < rows; ++row )
    {
        for ( unsigned col = 0; col < cols; ++col, block += 16 )
        {
            in = SmushAsmTest_ReplayBlock(&replay, in, block, 8);
        }
        block += (uint32_t)replay.lineSize * 7u;
    }
    uint8_t* pOriginal = replay.pOriginal + SMUSHASM_TEST_GUARD_SIZE;
    if ( pImage->outKind == 2 )
    {
        uint64_t hash = SmushAsmTest_Hash(pOriginal, pImage->size);
        TEST_ASSERT_TRUE_MESSAGE(hash == pImage->hash, pName);
    }
    else
    {
        TEST_ASSERT_EQUAL_MEMORY_MESSAGE(pImage->pExpected, pOriginal, pImage->size, pName);
    }
    for ( unsigned i = 0; i < pImage->size; ++i )
    {
        pImage->pExpected[i] = replay.pInvalid[SMUSHASM_TEST_GUARD_SIZE + i] ? 0 : pOriginal[i];
    }
    pImage->outKind = 1; // Full bytes remain checked, including formerly hashed cases.
    free(replay.pOriginal);
    free(replay.pInvalid);
}

// Replays every case of a vector file; returns the number of cases
static uint32_t SmushAsmTest_RunVectors(const char* pFileName, const uint32_t* aProcs, size_t numProcs)
{
    char aPath[512];
    STD_FORMAT(aPath, "%s/%s", SMUSH_TEST_TV_DIR, pFileName);

    FILE* pFile = fopen(aPath, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(pFile, aPath);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(pFile, 0, SEEK_END), aPath);
    long fileSize = ftell(pFile);
    TEST_ASSERT_TRUE_MESSAGE(fileSize > 12, aPath);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(pFile, 0, SEEK_SET), aPath);

    uint8_t* pData = (uint8_t*)malloc((size_t)fileSize);
    TEST_ASSERT_NOT_NULL_MESSAGE(pData, aPath);
    TEST_ASSERT_EQUAL_size_t_MESSAGE((size_t)fileSize, fread(pData, 1, (size_t)fileSize, pFile), aPath);
    fclose(pFile);

    SmushAsmTestReader reader = { pData, (size_t)fileSize, 0, 0 };
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(SMUSHASM_TEST_MAGIC, SmushAsmTest_Read(&reader, 4), 4, aPath);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(SMUSHASM_TEST_VERSION, SmushAsmTest_ReadU32(&reader), aPath);
    uint32_t numCases = SmushAsmTest_ReadU32(&reader);

    uint32_t numFailed = 0;
    for ( uint32_t caseIndex = 0; caseIndex < numCases; ++caseIndex )
    {
        char aName[256];
        uint8_t nameLen        = SmushAsmTest_ReadU8(&reader);
        const uint8_t* pName   = SmushAsmTest_Read(&reader, nameLen);
        TEST_ASSERT_FALSE_MESSAGE(reader.bError, aPath);
        memcpy(aName, pName, nameLen);
        aName[nameLen] = 0;

        uint32_t proc       = SmushAsmTest_ReadU32(&reader);
        uint32_t numBuffers = SmushAsmTest_ReadU32(&reader);
        TEST_ASSERT_TRUE_MESSAGE(numBuffers <= SMUSHASM_TEST_MAX_BUFFERS, aName);

        int bKnownProc = 0;
        for ( size_t i = 0; i < numProcs; ++i )
        {
            bKnownProc |= aProcs[i] == proc;
        }
        TEST_ASSERT_TRUE_MESSAGE(bKnownProc, aName);

        SmushAsmTestBuffer aBuffers[SMUSHASM_TEST_MAX_BUFFERS];
        memset(aBuffers, 0, sizeof(aBuffers));
        for ( uint32_t i = 0; i < numBuffers; ++i )
        {
            TEST_ASSERT_TRUE_MESSAGE(SmushAsmTest_ReadBuffer(&reader, &aBuffers[i]), aName);
        }

        uint32_t numArgs = SmushAsmTest_ReadU32(&reader);
        TEST_ASSERT_TRUE_MESSAGE(numArgs <= SMUSHASM_TEST_MAX_ARGS, aName);

        uintptr_t aArgs[SMUSHASM_TEST_MAX_ARGS] = { 0 };
        for ( uint32_t i = 0; i < numArgs; ++i )
        {
            uint8_t kind = SmushAsmTest_ReadU8(&reader);
            if ( kind == 0 )
            {
                aArgs[i] = SmushAsmTest_ReadU32(&reader);
            }
            else
            {
                uint32_t bufferIndex = SmushAsmTest_ReadU32(&reader);
                int32_t offset       = (int32_t)SmushAsmTest_ReadU32(&reader);
                TEST_ASSERT_TRUE_MESSAGE(kind == 1 && bufferIndex < numBuffers, aName);
                aArgs[i] = (uintptr_t)(aBuffers[bufferIndex].pData + offset);
            }
        }
        TEST_ASSERT_FALSE_MESSAGE(reader.bError, aName);

        if ( proc == SMUSHASM_PROC_CODEC2 )
        {
            SmushAsmTest_AdaptBoundedMotion(aArgs, aBuffers, numBuffers, aName);
        }
        SmushAsmTest_Call(proc, aArgs);

        int bMatch = 1;
        for ( uint32_t i = 0; i < numBuffers; ++i )
        {
            if ( !SmushAsmTest_CheckBuffer(&aBuffers[i]) )
            {
                if ( numFailed < SMUSHASM_TEST_MAX_REPORTED )
                {
                    char aMessage[320];
                    STD_FORMAT(aMessage, "%s: buffer %u differs from the original", aName, (unsigned)i);
                    TEST_MESSAGE(aMessage);
                }

                bMatch = 0;
                break;
            }
        }

        numFailed += !bMatch;
        for ( uint32_t i = 0; i < numBuffers; ++i )
        {
            free(aBuffers[i].pAlloc);
            free(aBuffers[i].pExpected);
        }
    }

    // The whole file must be consumed so that a changed layout cannot go unnoticed
    TEST_ASSERT_EQUAL_size_t_MESSAGE(reader.size, reader.pos, aPath);
    free(pData);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, numFailed, aPath);
    return numCases;
}

static uint8_t* SmushAsmTest_BuildMotionStream(uint8_t* pOut, int size, int x, int y, int targetSize, int targetX, int targetY, uint8_t op, int16_t motion)
{
    if ( x == targetX && y == targetY && size == targetSize )
    {
        *pOut++ = op;
        if ( op == 0xF5u )
        {
            memcpy(pOut, &motion, sizeof(motion));
            pOut += sizeof(motion);
        }
        return pOut;
    }
    if ( targetX >= x && targetX < x + size && targetY >= y && targetY < y + size )
    {
        *pOut++ = 0xFFu;
        pOut = SmushAsmTest_BuildMotionStream(pOut, size / 2, x, y, targetSize, targetX, targetY, op, motion);
        pOut = SmushAsmTest_BuildMotionStream(pOut, size / 2, x + size / 2, y, targetSize, targetX, targetY, op, motion);
        pOut = SmushAsmTest_BuildMotionStream(pOut, size / 2, x, y + size / 2, targetSize, targetX, targetY, op, motion);
        return SmushAsmTest_BuildMotionStream(pOut, size / 2, x + size / 2, y + size / 2, targetSize, targetX, targetY, op, motion);
    }
    *pOut++ = 0xFEu;
    *pOut++ = 0x77u;
    *pOut++ = 0x77u;
    return pOut;
}

static uint16_t* SmushAsmTest_GuardedImage(int guardAfter)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    size_t pageSize = info.dwPageSize;
    SmushAsmTest_pGuardAllocation = VirtualAlloc(NULL, pageSize * 3u, MEM_RESERVE, PAGE_NOACCESS);
    TEST_ASSERT_NOT_NULL(SmushAsmTest_pGuardAllocation);
    uint8_t* pPage = (uint8_t*)SmushAsmTest_pGuardAllocation + pageSize;
    TEST_ASSERT_EQUAL_PTR(pPage, VirtualAlloc(pPage, pageSize, MEM_COMMIT, PAGE_READWRITE));
    return (uint16_t*)(pPage + (guardAfter ? pageSize - 128u : 0u));
}

static void SmushAsmTest_CheckMotionBoundary(uint16_t* pSource, int blockSize, int guardAfter, uint8_t op, int height)
{
    uint16_t aOutput[64], aExpected[64];
    int16_t aMotion[245] = { 0 };
    uint8_t aStream[128];
    int x = guardAfter ? 8 - blockSize : 0;
    int y = x;
    int16_t motion = op == 0xF6u ? 0 : (int16_t)(guardAfter ? 63 - (y * 8 + x) : -1);
    aMotion[0] = motion;
    for ( unsigned i = 0; i < 64u; ++i )
    {
        pSource[i] = (uint16_t)(0x100u + i);
        aOutput[i] = 0xAAAAu;
        aExpected[i] = 0x7777u;
    }
    for ( int row = 0; row < blockSize; ++row )
    {
        for ( int col = 0; col < blockSize; ++col )
        {
            int at = (y + row) * 8 + x + col;
            int source = at + motion;
            aExpected[at] = source >= 0 && source < 64 ? (uint16_t)(0x100 + source) : 0;
        }
    }
    SmushAsmTest_BuildMotionStream(aStream, 8, 0, 0, blockSize, x, y, op, motion);
    int sourceOffset = (int)(((int64_t)(uintptr_t)pSource - (int64_t)(uintptr_t)aOutput) / 2);
    dbloc16_Codec2(aOutput, aStream, sourceOffset, sourceOffset, 8, aMotion, NULL, height, 0, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aOutput, sizeof(aExpected));
    for ( unsigned i = 0; i < 64u; ++i )
    {
        TEST_ASSERT_EQUAL_UINT16((uint16_t)(0x100u + i), pSource[i]);
    }
}

TEST(SmushAsm, Codec2MotionReadsStayInsideGuardedImages)
{
    static const uint8_t aOps[] = { 0u, 0xF5u };
    for ( int blockSize = 2; blockSize <= 8; blockSize *= 2 )
    {
        for ( unsigned op = 0; op < sizeof(aOps); ++op )
        {
            for ( int guardAfter = 0; guardAfter < 2; ++guardAfter )
            {
                uint16_t* pSource = SmushAsmTest_GuardedImage(guardAfter);
                // Negative motion crosses the leading guard; positive motion
                // leaves only the final 16-bit source pixel valid in its dword.
                SmushAsmTest_CheckMotionBoundary(pSource, blockSize, guardAfter, aOps[op], 8);
                TEST_ASSERT_TRUE(VirtualFree(SmushAsmTest_pGuardAllocation, 0, MEM_RELEASE));
                SmushAsmTest_pGuardAllocation = NULL;
            }
        }
    }
}

TEST(SmushAsm, Codec2MotionIgnoresReadableBytesOutsideImage)
{
    uint16_t aPadded[192];
    for ( unsigned i = 0; i < sizeof(aPadded) / sizeof(aPadded[0]); ++i )
    {
        aPadded[i] = 0xBAD5u;
    }
    for ( int blockSize = 2; blockSize <= 8; blockSize *= 2 )
    {
        for ( int after = 0; after < 2; ++after )
        {
            SmushAsmTest_CheckMotionBoundary(aPadded + 64, blockSize, after, 0xF5u, 8);
        }
    }
}

TEST(SmushAsm, Codec2DeltaReadsIncludeRoundedRowsAtGuardedBoundaries)
{
    for ( int blockSize = 2; blockSize <= 8; blockSize *= 2 )
    {
        for ( int guardAfter = 0; guardAfter < 2; ++guardAfter )
        {
            uint16_t* pSource = SmushAsmTest_GuardedImage(guardAfter);
            // The visible height is 1, but the allocated frame includes 8 rows.
            SmushAsmTest_CheckMotionBoundary(pSource, blockSize, guardAfter, 0xF6u, 1);
            TEST_ASSERT_TRUE(VirtualFree(SmushAsmTest_pGuardAllocation, 0, MEM_RELEASE));
            SmushAsmTest_pGuardAllocation = NULL;
        }
    }
}

TEST(SmushAsm, Codec2OverlappingMotionKeepsOriginalDwordOrdering)
{
    static const uint16_t aExpectedBlocks[2][16] = {
        { 9, 10, 11, 12, 25, 26, 27, 28, 25, 26, 27, 28, 33, 34, 35, 36 },
        { 17, 18, 19, 20, 25, 26, 27, 28, 33, 34, 35, 36, 41, 42, 43, 44 }
    };
    for ( unsigned variant = 0; variant < 2u; ++variant )
    {
        uint16_t aOutput[64], aExpected[64];
        uint8_t aStream[128];
        for ( unsigned i = 0; i < 64u; ++i )
        {
            aOutput[i] = (uint16_t)(i + 1u);
            aExpected[i] = 0x7777u;
        }
        for ( unsigned row = 0; row < 4u; ++row )
        {
            memcpy(aExpected + row * 8u, aExpectedBlocks[variant] + row * 4u, 4u * sizeof(uint16_t));
        }
        SmushAsmTest_BuildMotionStream(aStream, 8, 0, 0, 4, 0, 0, 0xF5u, (int16_t)(variant ? 16 : 8));
        // +8 catches later row pairs observing prior stores; +16 catches
        // the top dword being read before the paired bottom store.
        dbloc16_Codec2(aOutput, aStream, 0, 0, 8, NULL, NULL, 8, 0, NULL, NULL, NULL);
        TEST_ASSERT_EQUAL_MEMORY(aExpected, aOutput, sizeof(aExpected));
    }
}

TEST(SmushAsm, TestOriginalBloc16Asm)
{
    static const uint32_t aProcs[] = { SMUSHASM_PROC_INTERPOLATE, SMUSHASM_PROC_CODEC2 };
    TEST_ASSERT_TRUE(SmushAsmTest_RunVectors("Flobject/Bloc16Asm.tv", aProcs, STD_ARRAYLEN(aProcs)) > 0);
}

TEST(SmushAsm, TestOriginalDecodeAsm)
{
    static const uint32_t aProcs[] = {
        SMUSHASM_PROC_LUT8, SMUSHASM_PROC_LUT16, SMUSHASM_PROC_RGBATO555, SMUSHASM_PROC_555TORGBA,
        SMUSHASM_PROC_RGBATO565, SMUSHASM_PROC_565TORGBA, SMUSHASM_PROC_RGBATO565B, SMUSHASM_PROC_565BTORGBA,
    };
    TEST_ASSERT_TRUE(SmushAsmTest_RunVectors("Decode/DecodeAsm.tv", aProcs, STD_ARRAYLEN(aProcs)) > 0);
}

TEST(SmushAsm, TestOriginalMixerAsm)
{
    static const uint32_t aProcs[] = {
        SMUSHASM_PROC_MIXMEMCOPY, SMUSHASM_PROC_MIXMEMFILL, SMUSHASM_PROC_MIXLUT16, SMUSHASM_PROC_MIXLUT16ADD,
        SMUSHASM_PROC_MIXFILL16, SMUSHASM_PROC_MIXLUT8, SMUSHASM_PROC_MIXLUT8ADD, SMUSHASM_PROC_MIXFILLB,
    };
    TEST_ASSERT_TRUE(SmushAsmTest_RunVectors("Sound/MixerAsm.tv", aProcs, STD_ARRAYLEN(aProcs)) > 0);
}

TEST(SmushAsm, TestOriginalBitmapAsm)
{
    static const uint32_t aProcs[] = { SMUSHASM_PROC_BLITMASM, SMUSHASM_PROC_BLITMEMCOPY, SMUSHASM_PROC_BLITMEMCOPYB };
    TEST_ASSERT_TRUE(SmushAsmTest_RunVectors("Bitmap/BitmapAsm.tv", aProcs, STD_ARRAYLEN(aProcs)) > 0);
}

TEST_GROUP_RUNNER(SmushAsm)
{
    RUN_TEST_CASE(SmushAsm, Codec2MotionReadsStayInsideGuardedImages);
    RUN_TEST_CASE(SmushAsm, Codec2MotionIgnoresReadableBytesOutsideImage);
    RUN_TEST_CASE(SmushAsm, Codec2DeltaReadsIncludeRoundedRowsAtGuardedBoundaries);
    RUN_TEST_CASE(SmushAsm, Codec2OverlappingMotionKeepsOriginalDwordOrdering);
    RUN_TEST_CASE(SmushAsm, TestOriginalBloc16Asm);
    RUN_TEST_CASE(SmushAsm, TestOriginalDecodeAsm);
    RUN_TEST_CASE(SmushAsm, TestOriginalMixerAsm);
    RUN_TEST_CASE(SmushAsm, TestOriginalBitmapAsm);
}
