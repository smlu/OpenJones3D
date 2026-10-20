#include <unity_fixture.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <smush/insane/Decode/Decode.h>
#include <smush/insane/Flobject/D_bloc16.h>
#include <smush/insane/Flobject/D_wave.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/Engine.h>

// Unit tests of the smush Codec area (D_bloc16, D_wave, Decode) with synthetic chunks and mock LEC services. The tests run
// against the real Bloc16Asm, DecodeAsm, Framestream and Mixer translations.

// Test-only access to the tables of D_bloc16.c
void DBloc16_TestGetTables(const int16_t** ppOffsets, const Bl16Small** ppSmall, const Bl16Big** ppBig);

// ---- Mock LEC services ------------------------------------------------------------------------------------------

typedef struct sMockPublished
{
    const char* name;
    void* pInterface;
} MockPublished;

static MockPublished mock_published[8];
static int mock_numPublished;
static int mock_numUnpublished;
static int mock_numErrors;
static int mock_numAllocs;
static int mock_numFrees;

static int mock_Publish(const char* pName, void* pInterface)
{
    mock_published[mock_numPublished].name = pName;
    mock_published[mock_numPublished].pInterface = pInterface;
    mock_numPublished++;
    return 0;
}

static void mock_Unpublish(const char* pName)
{
    J3D_UNUSED(pName);
    mock_numUnpublished++;
}

static void* mock_Alloc(int size, const char* pFile, int line)
{
    J3D_UNUSED(pFile); J3D_UNUSED(line);
    mock_numAllocs++;
    return calloc(1, size);
}

static void mock_Free(void* pBlock, const char* pFile, int line)
{
    J3D_UNUSED(pFile); J3D_UNUSED(line);
    mock_numFrees++;
    free(pBlock);
}

static MemoryDebugInterface mock_memory = { mock_Alloc, mock_Free, NULL };

static int mock_numPlays;
static uint8_t mock_lastPacket[256];
static void mock_Play(uint8_t* pData, int id, int volume, int pan, int priority)
{
    J3D_UNUSED(id); J3D_UNUSED(volume); J3D_UNUSED(pan); J3D_UNUSED(priority);
    mock_numPlays++;
    memcpy(mock_lastPacket, pData, sizeof(mock_lastPacket));
}

static SoundInterface mock_sound;

static int mock_CreateBitmap(LecBitmapHeader** ppBitmap, int width, int height, int type)
{
    J3D_UNUSED(type);
    LecBitmapHeader* pBitmap = calloc(1, sizeof(LecBitmapHeader));
    pBitmap->pPixels   = calloc(1, (size_t)width * height * 2 + 64);
    pBitmap->width     = width;
    pBitmap->height    = height;
    pBitmap->pixelSize = 2;
    pBitmap->bpp       = 16;
    pBitmap->colorMode = 1;
    *ppBitmap = pBitmap;
    return 0;
}

static void mock_DestroyBitmap(LecBitmapHeader** ppBitmap)
{
    free((*ppBitmap)->pPixels);
    free(*ppBitmap);
    *ppBitmap = NULL;
}

static BitmapInterface mock_bitmap = { mock_CreateBitmap, NULL, mock_DestroyBitmap };

static void* mock_Subscribe(const char* pName, void* pParam)
{
    J3D_UNUSED(pParam);
    if ( strcmp(pName, "SOUND") == 0 ) {
        return &mock_sound;
    }

    if ( strcmp(pName, "MEMORY") == 0 || strcmp(pName, "MEMORY.workspace") == 0 ) {
        return &mock_memory;
    }

    if ( strcmp(pName, "BITMAP.clear") == 0 ) {
        return &mock_bitmap;
    }

    return NULL;
}

static void mock_Unsubscribe(void* pInterface) { J3D_UNUSED(pInterface); }

static void mock_ReportError(const char* pFormat, ...)
{
    J3D_UNUSED(pFormat);
    mock_numErrors++;
}

static LecShare mock_share = { mock_Publish, mock_Unpublish, mock_Subscribe, mock_Unsubscribe, NULL, NULL, NULL, NULL, mock_ReportError };

static void* Mock_Find(const char* pName)
{
    for ( int i = 0; i < mock_numPublished; i++ )
    {
        if ( strcmp(mock_published[i].name, pName) == 0 ) {
            return mock_published[i].pInterface;
        }
    }

    return NULL;
}

// ---- D_wave -----------------------------------------------------------------------------------------------------

TEST_GROUP(SmushCodec);

TEST_SETUP(SmushCodec)
{
    memset(&mock_sound, 0, sizeof(mock_sound));
    mock_sound.Play = mock_Play;
    mock_numPublished = mock_numUnpublished = mock_numErrors = mock_numAllocs = mock_numFrees = mock_numPlays = 0;
}

TEST_TEAR_DOWN(SmushCodec)
{}

TEST(SmushCodec, DWaveRegisterAndInfo)
{
    TEST_ASSERT_EQUAL_INT(0, dwave_Register(&mock_share, NULL));
    TEST_ASSERT_EQUAL_INT(2, mock_numPublished);
    TEST_ASSERT_NOT_NULL(Mock_Find("LECMODULE=insane_wave_flobdec"));

    DWaveInterface* pInterface = Mock_Find("FLOBDEC=insane_wave");
    TEST_ASSERT_NOT_NULL(pInterface);
    TEST_ASSERT_EQUAL_INT('Wave', pInterface->GetTag());
    TEST_ASSERT_EQUAL_INT(4, pInterface->GetFlags());
    TEST_ASSERT_EQUAL_STRING("Audio", pInterface->Name(0));
    TEST_ASSERT_EQUAL_STRING("", pInterface->Name(3));
    TEST_ASSERT_EQUAL_INT(-1, pInterface->Query1());
    TEST_ASSERT_EQUAL_INT(-1, pInterface->Query(NULL, 0x100, NULL, NULL, NULL, NULL));

    dwave_Unregister();
    TEST_ASSERT_EQUAL_INT(2, mock_numUnpublished);
}

TEST(SmushCodec, DWaveDecodeMonoPackets)
{
    // idx = 0, predictor = 0: nibbles 0000 then 0001 decode to the samples 0 and 1
    static const uint8_t aChunk[] = {
        0x00, 0x00, 0x00, 0x02, // sample count (big endian)
        0x00,                   // index, mono
        0x00, 0x00,             // predictor
        0x01, 0x00, 0x00, 0x00  // bit stream
    };
    uint8_t aMeta[16] = { 0 };
    aMeta[8] = 0x10; // total size 16

    TEST_ASSERT_EQUAL_INT(0, dwave_Register(&mock_share, NULL));
    TEST_ASSERT_EQUAL_INT(0, dwave_Startup());
    dwave_Parse(aMeta, sizeof(aMeta));

    DWaveInterface* pInterface = Mock_Find("FLOBDEC=insane_wave");
    TEST_ASSERT_EQUAL_UINT32(0x80, pInterface->Decode(NULL, 0, 0, 0, NULL, aChunk, sizeof(aChunk), 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(1, mock_numPlays);
    TEST_ASSERT_EQUAL_MEMORY("PSAD", mock_lastPacket, 4);
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_lastPacket[4]);
    TEST_ASSERT_EQUAL_HEX8(0x32, mock_lastPacket[7]); // 2 * 2 + 0x2E
    TEST_ASSERT_EQUAL_MEMORY("S6MZ", mock_lastPacket + 46, 4);
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_lastPacket[54]);
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_lastPacket[55]);
    TEST_ASSERT_EQUAL_HEX8(0x01, mock_lastPacket[56]);
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_lastPacket[57]);

    // The next chunk continues the stream with the short packet header
    pInterface->Decode(NULL, 0, 0, 0, NULL, aChunk, sizeof(aChunk), 0, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, mock_numPlays);
    TEST_ASSERT_EQUAL_HEX8(0x0E, mock_lastPacket[7]); // 2 * 2 + 10
    TEST_ASSERT_EQUAL_HEX8(0x01, mock_lastPacket[10]);

    dwave_Shutdown();
    TEST_ASSERT_EQUAL_INT(1, mock_numFrees);
    dwave_Unregister();
}

TEST(SmushCodec, DWaveDecodeVimaZeroCount)
{
    int16_t aOut[4] = { 0x1234, 0x1234, 0x1234, 0x1234 };
    static const uint8_t aSrc[] = { 0, 0 };
    dwave_DecodeVima(aOut, aSrc, 0, 1);
    TEST_ASSERT_EQUAL_INT16(0x1234, aOut[0]);
}

// ---- Decode -----------------------------------------------------------------------------------------------------

TEST(SmushCodec, DecodeRle)
{
    static const uint8_t aSrc[] = { 0x05, 0xAA, 0x04, 1, 2, 3 }; // run of 3 x AA, 3 literals
    uint8_t aDst[6];
    decode_Rle(aDst, (uint8_t*)aSrc, 6);

    static const uint8_t aExpected[] = { 0xAA, 0xAA, 0xAA, 1, 2, 3 };
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aDst, 6);
}

TEST(SmushCodec, DecodeRleByteStream)
{
    static const uint8_t aSrc[] = { 0x00, 0x77, 0x01, 0x55 }; // one literal, then a run of 1 (flag set at bit 0 means run)
    decode_RleInit((uint8_t*)aSrc);
    TEST_ASSERT_EQUAL_HEX8(0x77, decode_RleByte());
    TEST_ASSERT_EQUAL_HEX8(0x55, decode_RleByte());
    decode_RleEnd();
}

TEST(SmushCodec, DecodeRunPixelSizes)
{
    LecBitmapHeader bitmap = { 0 };
    uint8_t aDst[16];

    bitmap.pixelSize = 1;
    memset(aDst, 0, sizeof(aDst));
    uint8_t aPix[4] = { 7, 8, 9, 10 };
    decode_Run(aDst, &bitmap, 3, aPix);
    TEST_ASSERT_EQUAL_HEX8(7, aDst[2]);
    TEST_ASSERT_EQUAL_HEX8(0, aDst[3]);

    bitmap.pixelSize = 2;
    memset(aDst, 0, sizeof(aDst));
    decode_Run(aDst, &bitmap, 3, aPix);
    static const uint8_t aExpected2[] = { 7, 8, 7, 8, 7, 8, 0 };
    TEST_ASSERT_EQUAL_MEMORY(aExpected2, aDst, sizeof(aExpected2));

    bitmap.pixelSize = 3;
    memset(aDst, 0, sizeof(aDst));
    decode_Run(aDst, &bitmap, 2, aPix);
    static const uint8_t aExpected3[] = { 7, 8, 9, 7, 8, 9, 0 };
    TEST_ASSERT_EQUAL_MEMORY(aExpected3, aDst, sizeof(aExpected3));

    bitmap.pixelSize = 4;
    memset(aDst, 0, sizeof(aDst));
    decode_Run(aDst, &bitmap, 2, aPix);
    static const uint8_t aExpected4[] = { 7, 8, 9, 10, 7, 8, 9, 10, 0 };
    TEST_ASSERT_EQUAL_MEMORY(aExpected4, aDst, sizeof(aExpected4));

    // Equal bytes take the fill path
    uint8_t aSame[2] = { 0x11, 0x11 };
    bitmap.pixelSize = 2;
    decode_Run(aDst, &bitmap, 4, aSame);
    TEST_ASSERT_EQUAL_HEX8(0x11, aDst[7]);
}

TEST(SmushCodec, DecodeMakeTable)
{
    LecBitmapHeader src = { 0 };
    LecBitmapHeader dst = { 0 };
    uint8_t aPalette[1024];
    uint8_t aTable[1024];

    src.colorMode = 0;
    src.pixelSize = 1;
    dst.colorMode = 1;
    dst.pixelSize = 2;
    dst.format.redPosShift   = 10;
    dst.format.greenPosShift = 5;
    TEST_ASSERT_EQUAL_INT(-1, decode_MakeTable(&src, &dst, aTable, NULL));

    memset(aPalette, 0, sizeof(aPalette));
    aPalette[4 * 1 + 0] = 0xFF; // palette entry 1: red
    aPalette[4 * 2 + 2] = 0xFF; // palette entry 2: blue
    memset(aTable, 0xEE, sizeof(aTable));
    TEST_ASSERT_EQUAL_INT(0, decode_MakeTable(&src, &dst, aTable, aPalette));
    const uint16_t* pTable = (const uint16_t*)aTable;
    TEST_ASSERT_EQUAL_HEX16(0x0000, pTable[0]);
    TEST_ASSERT_EQUAL_HEX16(0x7C00, pTable[1]);
    TEST_ASSERT_EQUAL_HEX16(0x001F, pTable[2]);
    TEST_ASSERT_EQUAL_HEX16(0x0000, pTable[255]);

    dst.colorMode = 0;
    TEST_ASSERT_EQUAL_INT(-1, decode_MakeTable(&src, &dst, aTable, aPalette));
}

TEST(SmushCodec, DecodeConvertSameFormatCopies)
{
    LecBitmapHeader src = { 0 };
    src.pixelSize = 2;
    src.bpp       = 16;
    src.colorMode = 1;
    src.format.redPosShift   = 11;
    src.format.greenPosShift = 5;
    LecBitmapHeader dst = src;

    uint16_t aSrc[3] = { 1, 2, 3 };
    uint16_t aDst[3] = { 0 };
    TEST_ASSERT_EQUAL_INT(0, decode_Convert((uint8_t*)aSrc, &src, 3, NULL, (uint8_t*)aDst, &dst, NULL));
    TEST_ASSERT_EQUAL_UINT16(3, aDst[2]);
}

TEST(SmushCodec, DecodeConvertTableDispatch)
{
    LecBitmapHeader src = { 0 };
    LecBitmapHeader dst = { 0 };
    src.pixelSize = 1;
    src.bpp       = 8;
    dst.pixelSize = 2;
    dst.bpp       = 16;
    dst.colorMode = 1;
    uint8_t aSrc[4] = { 1, 3, 0, 0 };
    uint8_t aDst[8] = { 0 };
    uint16_t aTable[8] = { 0x1000, 0x1111, 0x1222, 0x1333, 0x1444, 0x1555, 0x1666, 0x1777 };

    // 8-bit source through the 16-bit table (two pixels)
    TEST_ASSERT_EQUAL_INT(0, decode_Convert(aSrc, &src, 2, NULL, aDst, &dst, aTable));
    TEST_ASSERT_EQUAL_HEX16(0x1111, ((uint16_t*)aDst)[0]);
    TEST_ASSERT_EQUAL_HEX16(0x1333, ((uint16_t*)aDst)[1]);

    // 16-bit source through the table
    src.pixelSize = 2;
    src.bpp       = 16;
    src.colorMode = 2;
    uint16_t aSrc16[2] = { 1, 3 };
    memset(aDst, 0, sizeof(aDst));
    TEST_ASSERT_EQUAL_INT(0, decode_Convert((uint8_t*)aSrc16, &src, 2, NULL, aDst, &dst, aTable));
    TEST_ASSERT_EQUAL_HEX16(0x1111, ((uint16_t*)aDst)[0]);
    TEST_ASSERT_EQUAL_HEX16(0x1333, ((uint16_t*)aDst)[1]);
}

TEST(SmushCodec, DecodeToRGBAFastPaths)
{
    LecBitmapHeader bitmap = { 0 };
    uint8_t aSrc[8] = { 0 };
    uint8_t aDst[32];

    bitmap.colorMode = 1;
    bitmap.pixelSize = 2;
    bitmap.format.redPosShift = 10;
    bitmap.format.greenPosShift = 5;
    TEST_ASSERT_EQUAL_INT(0, decode_ToRGBA(aSrc, &bitmap, 2, NULL, aDst));
    static const uint8_t aExpected555[] = { 0x07, 0x07, 0x07, 0xFF, 0x07, 0x07, 0x07, 0xFF };
    TEST_ASSERT_EQUAL_MEMORY(aExpected555, aDst, sizeof(aExpected555));

    bitmap.format.redPosShift = 0;
    bitmap.format.bluePosShift = 11;
    memset(aDst, 0xEE, sizeof(aDst));
    TEST_ASSERT_EQUAL_INT(0, decode_ToRGBA(aSrc, &bitmap, 2, NULL, aDst));
    static const uint8_t aExpected565B[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    TEST_ASSERT_EQUAL_MEMORY(aExpected565B, aDst, sizeof(aExpected565B));

    bitmap.format.redPosShift = 11;
    bitmap.format.bluePosShift = 0;
    TEST_ASSERT_EQUAL_INT(0, decode_ToRGBA(aSrc, &bitmap, 2, NULL, aDst));
    static const uint8_t aExpected565[] = { 0x07, 0x03, 0x07, 0xFF, 0x07, 0x03, 0x07, 0xFF };
    TEST_ASSERT_EQUAL_MEMORY(aExpected565, aDst, sizeof(aExpected565));

    // Palette bitmap: no palette at all fails, the header's palette wins over the argument
    memset(&bitmap, 0, sizeof(bitmap));
    TEST_ASSERT_EQUAL_INT(-1, decode_ToRGBA(aSrc, &bitmap, 2, NULL, aDst));
    uint32_t aPalA[256] = { 0x11223344 };
    uint32_t aPalB[256] = { 0x55667788 };
    TEST_ASSERT_EQUAL_INT(0, decode_ToRGBA(aSrc, &bitmap, 2, aPalA, aDst));
    TEST_ASSERT_EQUAL_HEX32(0x11223344, ((uint32_t*)aDst)[0]);
    bitmap.indexed.pPalette = aPalB;
    TEST_ASSERT_EQUAL_INT(0, decode_ToRGBA(aSrc, &bitmap, 2, aPalA, aDst));
    TEST_ASSERT_EQUAL_HEX32(0x55667788, ((uint32_t*)aDst)[0]);
}

TEST(SmushCodec, DecodeToRGBAGeneric)
{
    LecBitmapHeader bitmap = { 0 };
    bitmap.colorMode = 1;
    bitmap.pixelSize = 2;
    bitmap.format.redBPP = 5;
    bitmap.format.greenBPP = 5;
    bitmap.format.blueBPP = 5;
    bitmap.format.redPosShift = 1;
    bitmap.format.greenPosShift = 6;
    bitmap.format.bluePosShift = 11;
    bitmap.format.redPosShiftRight = 3;
    bitmap.format.greenPosShiftRight = 3;
    bitmap.format.bluePosShiftRight = 3;

    uint16_t aSrc[2] = { 0xFFFF, 0x0000 };
    uint8_t aDst[8];
    TEST_ASSERT_EQUAL_INT(0, decode_ToRGBA((uint8_t*)aSrc, &bitmap, 2, NULL, aDst));
    static const uint8_t aExpected[] = { 0xF8, 0xF8, 0xF8, 0xFF, 0, 0, 0, 0xFF };
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aDst, sizeof(aExpected));

    // Mode 2 carries alpha
    bitmap.colorMode = 2;
    bitmap.format.alphaBPP = 1;
    bitmap.format.alphaPosShift = 0;
    bitmap.format.alphaPosShiftRight = 0;
    TEST_ASSERT_EQUAL_INT(0, decode_ToRGBA((uint8_t*)aSrc, &bitmap, 2, NULL, aDst));
    TEST_ASSERT_EQUAL_HEX8(0x80, aDst[3]);

    bitmap.pixelSize = 5;
    TEST_ASSERT_EQUAL_INT(-1, decode_ToRGBA((uint8_t*)aSrc, &bitmap, 2, NULL, aDst));
}

TEST(SmushCodec, DecodeBeginGeneric)
{
    LecBitmapHeader bitmap = { 0 };
    bitmap.colorMode = 1;
    bitmap.pixelSize = 2;
    bitmap.format.redBPP = 5;
    bitmap.format.greenBPP = 5;
    bitmap.format.blueBPP = 5;
    bitmap.format.redPosShift = 12;
    bitmap.format.greenPosShift = 7;
    bitmap.format.bluePosShift = 2;
    bitmap.format.redPosShiftRight = 3;
    bitmap.format.greenPosShiftRight = 3;
    bitmap.format.bluePosShiftRight = 3;

    uint8_t aSrc[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0 };
    uint16_t aDst[2] = { 0 };
    TEST_ASSERT_EQUAL_INT(0, decode_Begin((uint8_t*)aDst, &bitmap, 2, NULL, aSrc));
    TEST_ASSERT_EQUAL_HEX16(0xFFFC, aDst[0]); // (0x1F << 12) | (0x1F << 7) | (0x1F << 2), truncated to 16 bits
    TEST_ASSERT_EQUAL_HEX16(0, aDst[1]);

    bitmap.pixelSize = 4;
    uint32_t aDst4[2] = { 0 };
    TEST_ASSERT_EQUAL_INT(0, decode_Begin((uint8_t*)aDst4, &bitmap, 2, NULL, aSrc));
    TEST_ASSERT_EQUAL_HEX32((0x1Fu << 12) | (0x1Fu << 7) | (0x1Fu << 2), aDst4[0]);

    bitmap.pixelSize = 3;
    uint8_t aDst3[6] = { 0 };
    TEST_ASSERT_EQUAL_INT(0, decode_Begin(aDst3, &bitmap, 2, NULL, aSrc));
    TEST_ASSERT_EQUAL_HEX8(0xFC, aDst3[0]);
    TEST_ASSERT_EQUAL_HEX8(0, aDst3[3]);

    bitmap.pixelSize = 6;
    TEST_ASSERT_EQUAL_INT(-1, decode_Begin(aDst3, &bitmap, 2, NULL, aSrc));
    bitmap.colorMode = 7;
    TEST_ASSERT_EQUAL_INT(-1, decode_Begin(aDst3, &bitmap, 2, NULL, aSrc));
}

TEST(SmushCodec, DecodeBeginFastPaths)
{
    LecBitmapHeader bitmap = { 0 };
    uint8_t aSrc[8] = { 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF }; // RGBA: red, blue
    uint16_t aDst[2];
    bitmap.colorMode = 1;
    bitmap.pixelSize = 2;
    bitmap.format.redPosShift = 10;
    bitmap.format.greenPosShift = 5;
    TEST_ASSERT_EQUAL_INT(0, decode_Begin((uint8_t*)aDst, &bitmap, 2, NULL, aSrc));
    TEST_ASSERT_EQUAL_HEX16(0x7C00, aDst[0]);
    TEST_ASSERT_EQUAL_HEX16(0x001F, aDst[1]);

    bitmap.format.redPosShift = 0;
    bitmap.format.bluePosShift = 11;
    TEST_ASSERT_EQUAL_INT(0, decode_Begin((uint8_t*)aDst, &bitmap, 2, NULL, aSrc));
    TEST_ASSERT_EQUAL_HEX16(0x001F, aDst[0]);
    TEST_ASSERT_EQUAL_HEX16(0xF800, aDst[1]);

    bitmap.format.redPosShift = 11;
    bitmap.format.bluePosShift = 0;
    TEST_ASSERT_EQUAL_INT(0, decode_Begin((uint8_t*)aDst, &bitmap, 2, NULL, aSrc));
    TEST_ASSERT_EQUAL_HEX16(0xF800, aDst[0]);
    TEST_ASSERT_EQUAL_HEX16(0x001F, aDst[1]);
}

// ---- D_bloc16 ---------------------------------------------------------------------------------------------------

TEST(SmushCodec, DBloc16AverageAndConfigure)
{
    // Averages each of the three 565 channels (with the carry of the halving shifted in)
    TEST_ASSERT_EQUAL_HEX16(0x0000, dbloc16_Average(0x0000, 0x0000));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, dbloc16_Average(0xFFFF, 0xFFFF));
    TEST_ASSERT_EQUAL_HEX16(0x0020, dbloc16_Average(0x0000, 0x0040));

    TEST_ASSERT_EQUAL_INT(-1, dbloc16_Configure(99, 0));
    Bl16Blit pBlit = (Bl16Blit)1;
    TEST_ASSERT_EQUAL_INT(0, dbloc16_Configure(2, &pBlit, 0));
    TEST_ASSERT_NULL(pBlit);
    LecBitmapHeader* pShown = (LecBitmapHeader*)1;
    TEST_ASSERT_EQUAL_INT(0, dbloc16_Configure(3, &pShown, 0));
    TEST_ASSERT_NULL(pShown);
}

TEST(SmushCodec, DBloc16RegisterAndInfo)
{
    TEST_ASSERT_EQUAL_INT(0, dbloc16_Register(&mock_share, NULL));
    TEST_ASSERT_NOT_NULL(Mock_Find("LECMODULE=insane_blocky16_flobdec"));
    Bl16Interface* pInterface = Mock_Find("FLOBDEC=insane_blocky16");
    TEST_ASSERT_NOT_NULL(pInterface);
    TEST_ASSERT_EQUAL_INT('Bl16', pInterface->GetTag());
    TEST_ASSERT_EQUAL_INT(0, pInterface->GetFlags());
    TEST_ASSERT_EQUAL_INT(-1, pInterface->Query1());
    TEST_ASSERT_EQUAL_STRING("Blocky16-Tile", pInterface->Name(0));
    TEST_ASSERT_EQUAL_STRING("", pInterface->Name(-1));

    Bl16Header hdr = { 0 };
    hdr.x      = 3;
    hdr.y      = 4;
    hdr.width  = 640;
    hdr.height = 480;
    int x = 0, y = 0, w = 0, h = 0;
    TEST_ASSERT_EQUAL_INT(-1, pInterface->Query(&hdr, 0x101, &x, &y, &w, &h));
    TEST_ASSERT_EQUAL_INT(0, pInterface->Query(&hdr, 0x100, &x, &y, &w, &h));
    TEST_ASSERT_EQUAL_INT(3, x);
    TEST_ASSERT_EQUAL_INT(4, y);
    TEST_ASSERT_EQUAL_INT(640, w);
    TEST_ASSERT_EQUAL_INT(480, h);
    TEST_ASSERT_EQUAL_INT(0, pInterface->Query(&hdr, 0x100, NULL, NULL, NULL, NULL));

    dbloc16_Unregister();
    TEST_ASSERT_EQUAL_INT(2, mock_numUnpublished);
}

static int blit_count;
static LecBitmapHeader* blit_pSrc;
static int blitTest_Callback(LecBitmapHeader* pDst, MixerRect* pClip, int x, int y, LecBitmapHeader* pSrc, void* pPalette, void* pTable)
{
    J3D_UNUSED(pDst); J3D_UNUSED(pClip); J3D_UNUSED(x); J3D_UNUSED(y); J3D_UNUSED(pPalette); J3D_UNUSED(pTable);
    blit_count++;
    blit_pSrc = pSrc;
    return 42;
}

static Bl16Header* DBloc16Test_MakeFrame(int width, int height, int codec, int seq, int dataSize)
{
    Bl16Header* pHdr = calloc(1, 0x230 + dataSize + 16);
    pHdr->width  = (int16_t)width;
    pHdr->height = (int16_t)height;
    pHdr->codec  = (uint8_t)codec;
    pHdr->seq    = (int16_t)seq;
    pHdr->depth  = 16;
    return pHdr;
}

TEST(SmushCodec, DBloc16ParseAllocatesBitmaps)
{
    TEST_ASSERT_EQUAL_INT(0, dbloc16_Register(&mock_share, NULL));
    TEST_ASSERT_EQUAL_INT(0, dbloc16_Startup());
    Bl16Interface* pInterface = Mock_Find("FLOBDEC=insane_blocky16");

    Bl16Parse parse = { 0, 16, 13, 16 };
    TEST_ASSERT_EQUAL_INT(0, pInterface->Parse(&parse, sizeof(parse)));
    TEST_ASSERT_EQUAL_INT(1, mock_numAllocs); // the LUT
    TEST_ASSERT_EQUAL_INT(0, pInterface->Parse(&parse, sizeof(parse)));
    TEST_ASSERT_EQUAL_INT(1, mock_numAllocs); // unchanged size: nothing to do

    parse.height = 40;
    pInterface->Parse(&parse, sizeof(parse));
    TEST_ASSERT_EQUAL_INT(2, mock_numAllocs);
    TEST_ASSERT_EQUAL_INT(1, mock_numFrees);

    dbloc16_Shutdown();
    TEST_ASSERT_EQUAL_INT(2, mock_numFrees);
    dbloc16_Unregister();
}

TEST(SmushCodec, DBloc16DecodeRawFrameBlits)
{
    dbloc16_Register(&mock_share, NULL);
    dbloc16_Startup();
    Bl16Interface* pInterface = Mock_Find("FLOBDEC=insane_blocky16");

    blit_count = 0;
    dbloc16_Configure(1, blitTest_Callback, 0);

    const int width = 8, height = 8;
    Bl16Header* pHdr = DBloc16Test_MakeFrame(width, height, 0, 0, width * height * 2);
    uint16_t* pPix = (uint16_t*)pHdr->data;
    for ( int i = 0; i < width * height; i++ ) {
        pPix[i] = (uint16_t)(0x1000 + i);
    }

    LecBitmapHeader dst = { 0 };
    dst.pixelSize = 2;
    dst.bpp       = 16;
    dst.colorMode = 1;
    TEST_ASSERT_EQUAL_INT(42, pInterface->Decode(&dst, 0, 0, 0, NULL, (uint8_t*)pHdr, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(1, blit_count);
    TEST_ASSERT_NOT_NULL(blit_pSrc);
    TEST_ASSERT_EQUAL_UINT16(0x1000 + 63, ((uint16_t*)blit_pSrc->pPixels)[63]);

    LecBitmapHeader* pShown = NULL;
    dbloc16_Configure(3, &pShown, 0);
    TEST_ASSERT_EQUAL_PTR(blit_pSrc, pShown);

    // Codec 6: 8-bit indices through the header palette
    Bl16Header* pHdr6 = DBloc16Test_MakeFrame(width, height, 6, 1, width * height);
    pHdr6->palette[5] = 0xABCD;
    memset(pHdr6->data, 5, width * height);
    pInterface->Decode(&dst, 0, 0, 0, NULL, (uint8_t*)pHdr6, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_UINT16(0xABCD, ((uint16_t*)blit_pSrc->pPixels)[10]);

    // Without a blit callback the mixer blit is used and the result is 1
    dbloc16_Configure(1, NULL, 0);
    TEST_ASSERT_EQUAL_INT(1, pInterface->Decode(&dst, 0, 0, 0, NULL, (uint8_t*)pHdr, 0, 0, 0, 0));

    free(pHdr);
    free(pHdr6);
    dbloc16_Shutdown();
    dbloc16_Unregister();
}

TEST(SmushCodec, DBloc16Codec2ReceivesTables)
{
    dbloc16_Register(&mock_share, NULL);
    dbloc16_Startup();
    Bl16Interface* pInterface = Mock_Find("FLOBDEC=insane_blocky16");
    dbloc16_Configure(1, blitTest_Callback, 0);

    const int width = 24, height = 8;
    Bl16Header* pHdr = DBloc16Test_MakeFrame(width, height, 2, 0, 64);
    LecBitmapHeader dst = { 0 };
    dst.pixelSize = 2;
    dst.bpp       = 16;
    dst.colorMode = 1;
    pInterface->Decode(&dst, 0, 0, 0, NULL, (uint8_t*)pHdr, 0, 0, 0, 0);

    const int16_t* pMotion;
    const Bl16Small* pSmall;
    const Bl16Big* pBig;
    DBloc16_TestGetTables(&pMotion, &pSmall, &pBig);
    TEST_ASSERT_EQUAL_INT16(0, pMotion[0]);
    TEST_ASSERT_EQUAL_INT16(-43 * width - 1, pMotion[1]);   // (-1, -43)
    TEST_ASSERT_EQUAL_INT16(-43 * width + 6, pMotion[2]);   // (6, -43)
    TEST_ASSERT_EQUAL_INT16(1 * width + 0, pMotion[136]);   // (0, 1)

    for ( int i = 0; i < BL16_NUMTABLES; i++ )
    {
        TEST_ASSERT_EQUAL_INT(16, pSmall[i].n1 + pSmall[i].n2);
        TEST_ASSERT_EQUAL_INT(64, pBig[i].n1 + pBig[i].n2);
        for ( int j = 0; j < pSmall[i].n1; j++ ) {
            TEST_ASSERT_EQUAL_UINT16((pSmall[i].idx1[j] >> 2) * width + (pSmall[i].idx1[j] & 3), pSmall[i].off1[j]);
        }

        for ( int j = 0; j < pBig[i].n2; j++ ) {
            TEST_ASSERT_EQUAL_UINT16((pBig[i].idx2[j] >> 3) * width + (pBig[i].idx2[j] & 7), pBig[i].off2[j]);
        }
    }

    // Glyph 0 joins the top-left corner with itself: only the pixels above and left of... it fills the column up from the point
    TEST_ASSERT_TRUE(pSmall[0].n1 >= 1);

    // Further frames at the same width keep the tables (also a frame out of sequence)
    Bl16Header* pHdr2 = DBloc16Test_MakeFrame(width, height, 2, 5, 64);
    pInterface->Decode(&dst, 0, 0, 0, NULL, (uint8_t*)pHdr2, 0, 0, 0, 0);
    Bl16Header* pHdr3 = DBloc16Test_MakeFrame(width, height, 2, 6, 64);
    pInterface->Decode(&dst, 0, 0, 0, NULL, (uint8_t*)pHdr3, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_INT16(1 * width + 0, pMotion[136]);

    free(pHdr);
    free(pHdr2);
    free(pHdr3);
    dbloc16_Shutdown();
    dbloc16_Unregister();
}

TEST(SmushCodec, DBloc16ScaleDoublesRows)
{
    // 2x upscale of a 4x4 source into an 8-wide destination: the first destination row repeats the second
    enum { W = 8, H = 4 };
    uint16_t aSrc[(W / 2) * (H / 2) + 8];
    for ( int i = 0; i < (int)(sizeof(aSrc) / sizeof(aSrc[0])); i++ ) {
        aSrc[i] = 0x0841; // channel values all equal
    }

    uint16_t aDst[W * H];
    memset(aDst, 0, sizeof(aDst));
    dbloc16_Scale(aDst, aSrc, W, H);
    for ( int i = 0; i < W * H; i++ ) {
        TEST_ASSERT_EQUAL_HEX16(0x0841, aDst[i]);
    }
}

TEST(SmushCodec, DBloc16ScalePalMatchesScale)
{
    enum { W = 8, H = 4 };
    uint8_t aSrc[(W / 2) * (H / 2) + 8];
    uint16_t aPal[256];
    for ( int i = 0; i < 256; i++ ) {
        aPal[i] = (uint16_t)(i * 0x0101);
    }

    uint16_t aSrc16[sizeof(aSrc)];
    for ( int i = 0; i < (int)sizeof(aSrc); i++ )
    {
        aSrc[i]   = (uint8_t)(i * 7);
        aSrc16[i] = aPal[aSrc[i]];
    }

    uint16_t aDst1[W * H] = { 0 };
    uint16_t aDst2[W * H] = { 0 };
    dbloc16_Scale(aDst1, aSrc16, W, H);
    dbloc16_ScalePal(aDst2, aSrc, W, H, aPal);
    TEST_ASSERT_EQUAL_MEMORY(aDst1, aDst2, sizeof(aDst1));
}

TEST_GROUP_RUNNER(SmushCodec)
{
    RUN_TEST_CASE(SmushCodec, DWaveRegisterAndInfo);
    RUN_TEST_CASE(SmushCodec, DWaveDecodeMonoPackets);
    RUN_TEST_CASE(SmushCodec, DWaveDecodeVimaZeroCount);
    RUN_TEST_CASE(SmushCodec, DecodeRle);
    RUN_TEST_CASE(SmushCodec, DecodeRleByteStream);
    RUN_TEST_CASE(SmushCodec, DecodeRunPixelSizes);
    RUN_TEST_CASE(SmushCodec, DecodeMakeTable);
    RUN_TEST_CASE(SmushCodec, DecodeConvertSameFormatCopies);
    RUN_TEST_CASE(SmushCodec, DecodeConvertTableDispatch);
    RUN_TEST_CASE(SmushCodec, DecodeToRGBAFastPaths);
    RUN_TEST_CASE(SmushCodec, DecodeToRGBAGeneric);
    RUN_TEST_CASE(SmushCodec, DecodeBeginGeneric);
    RUN_TEST_CASE(SmushCodec, DecodeBeginFastPaths);
    RUN_TEST_CASE(SmushCodec, DBloc16AverageAndConfigure);
    RUN_TEST_CASE(SmushCodec, DBloc16RegisterAndInfo);
    RUN_TEST_CASE(SmushCodec, DBloc16ParseAllocatesBitmaps);
    RUN_TEST_CASE(SmushCodec, DBloc16DecodeRawFrameBlits);
    RUN_TEST_CASE(SmushCodec, DBloc16Codec2ReceivesTables);
    RUN_TEST_CASE(SmushCodec, DBloc16ScaleDoublesRows);
    RUN_TEST_CASE(SmushCodec, DBloc16ScalePalMatchesScale);
}
