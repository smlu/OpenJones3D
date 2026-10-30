#include <unity_fixture.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

TEST_SETUP(SmushAsm)
{}

TEST_TEAR_DOWN(SmushAsm)
{}

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
    RUN_TEST_CASE(SmushAsm, TestOriginalBloc16Asm);
    RUN_TEST_CASE(SmushAsm, TestOriginalDecodeAsm);
    RUN_TEST_CASE(SmushAsm, TestOriginalMixerAsm);
    RUN_TEST_CASE(SmushAsm, TestOriginalBitmapAsm);
}
