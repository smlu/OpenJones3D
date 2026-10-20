// D_wave.c: the VIMA wave frame-object decoder (original 0x004E7D60-0x004E8510).
#include <smush/insane/Flobject/D_wave.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stddef.h>
#include <string.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/Engine.h>

#define DWAVE_SWAP32(v) (((((v) & 0xFF0000) | ((v) >> 16)) >> 8) | ((((v) << 16) | ((v) & 0xFF00)) << 8))

static const int8_t dwave_delta2[4] = {
    -1, 4, -1, 4
};

static const int8_t dwave_delta3[8] = {
    -1, -1, 2, 6, -1, -1, 2, 6
};

static const int8_t dwave_delta4[16] = {
    -1, -1, -1, -1, 1, 2, 4, 6, -1, -1, -1, -1, 1, 2, 4, 6
};

static const int8_t dwave_delta5[32] = {
    -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 2, 2, 4, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 2, 2, 4, 5, 6
};

static const int8_t dwave_delta6[64] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 5, 5, 6, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 5, 5, 6, 6
};

static const int8_t dwave_delta7[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
};

static const uint16_t dwave_step[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const uint8_t dwave_bits[89] = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

static LecModule dwave_module = { dwave_Register, dwave_Unregister, dwave_Startup, dwave_Shutdown };
static DWaveInterface dwave_interface = { dwave_Name, dwave_GetTag, dwave_GetFlags, dwave_Reset, dwave_Parse, dwave_Decode, dwave_Query1, dwave_Query };

static int dwave_lastHeader = -1;
static const int8_t* dwave_deltas[8] = { NULL, NULL, dwave_delta2, dwave_delta3, dwave_delta4, dwave_delta5, dwave_delta6, dwave_delta7 };

static MemoryDebugInterface* dwave_pMemory = NULL;
static struct
{
    int8_t idx[2];
    int16_t pred[2];
    uint16_t packets;
} dwave_state = { 0 };

static uint32_t dwave_total        = 0;
static LecShare* dwave_pShare      = NULL;
static uint16_t dwave_quant[89 * 64] = { 0 };
static void* dwave_pParam          = NULL;
static SoundInterface* dwave_pSound = NULL;
static char* dwave_pPacket         = NULL;
static int dwave_tableReady        = 0;

void D_wave_InstallHooks(void)
{
    J3D_HOOKFUNC(dwave_Register);
    J3D_HOOKFUNC(dwave_Unregister);
    J3D_HOOKFUNC(dwave_Startup);
    J3D_HOOKFUNC(dwave_Shutdown);
    J3D_HOOKFUNC(dwave_Name);
    J3D_HOOKFUNC(dwave_GetTag);
    J3D_HOOKFUNC(dwave_GetFlags);
    J3D_HOOKFUNC(dwave_Reset);
    J3D_HOOKFUNC(dwave_Parse);
    J3D_HOOKFUNC(dwave_Decode);
    J3D_HOOKFUNC(dwave_DecodeVima);
}

void D_wave_ResetGlobals(void)
{}

int J3DAPI dwave_Register(LecShare* pShare, void* pParam)
{
    dwave_pShare = pShare;
    dwave_pParam = pParam;
    pShare->Publish("LECMODULE=insane_wave_flobdec", &dwave_module);
    dwave_pShare->Publish("FLOBDEC=insane_wave", &dwave_interface);
    return 0;
}

void dwave_Unregister(void)
{
    dwave_pShare->Unpublish("FLOBDEC=insane_wave");
    dwave_pShare->Unpublish("LECMODULE=insane_wave_flobdec");
}

int dwave_Startup(void)
{
    dwave_pSound = dwave_pShare->Subscribe("SOUND", dwave_pParam);
    if ( dwave_pSound == NULL )
    {
        dwave_pShare->ReportError("D_WAVE: Cannot allocate sound services.");
        return -1;
    }

    dwave_pMemory = dwave_pShare->Subscribe("MEMORY", dwave_pParam);
    if ( dwave_pMemory == NULL )
    {
        dwave_pShare->ReportError("D_WAVE: Cannot allocate memory services.");
        return -1;
    }

    dwave_pPacket = dwave_pMemory->Alloc(500000, INSANE_FILE, __LINE__);
    return dwave_pPacket ? 0 : -1;
}

void dwave_Shutdown(void)
{
    dwave_pShare->Unsubscribe(dwave_pSound);
    dwave_pShare->Unsubscribe(dwave_pMemory);
    if ( dwave_pPacket ) {
        dwave_pMemory->Free(dwave_pPacket, INSANE_FILE, __LINE__);
    }

    dwave_pPacket = NULL;
}

const char* J3DAPI dwave_Name(int index)
{
    switch ( index )
    {
        case 0:
            return "Audio";

        case 1:
            return "Decompresses VBR-ADPCM 16-bit audio.";

        case 2:
            return "Decompresses Variable Bit Rate ADPCM 16-bit audio.";
    }

    return "";
}

int dwave_GetTag(void)
{
    return 'Wave';
}

int dwave_GetFlags(void)
{
    return 4;
}

void dwave_Reset(void)
{
    dwave_state.packets = 0;
}

int J3DAPI dwave_Parse(const uint8_t* pData, int size)
{
    J3D_UNUSED(size);
    uint32_t total = *(const uint32_t*)(pData + 8);

    dwave_state.packets = 0;
    dwave_total         = total;
    return 0;
}

uint32_t J3DAPI dwave_Decode(void* a1, int a2, int a3, int a4, void* a5, const uint8_t* pData, int size, int mode, int a9, int a10)
{
    J3D_UNUSED(a1); J3D_UNUSED(a2); J3D_UNUSED(a3); J3D_UNUSED(a4); J3D_UNUSED(a5); J3D_UNUSED(size); J3D_UNUSED(mode); J3D_UNUSED(a9); J3D_UNUSED(a10);
    uint32_t channels   = 1;
    const uint8_t* src  = pData;

    uint32_t v = DWAVE_SWAP32(*(const uint32_t*)src);
    src += 4;
    if ( (int)v < 0 )
    {
        uint32_t h = DWAVE_SWAP32(*(const uint32_t*)src);
        src += 4;
        if ( (int)h != dwave_lastHeader )
        {
            dwave_lastHeader    = h;
            dwave_state.packets = 0;
        }

        v = DWAVE_SWAP32(*(const uint32_t*)src);
        src += 4;
    }

    uint8_t c = *src++;
    dwave_state.idx[0] = c;
    if ( (int8_t)c < 0 )
    {
        dwave_state.idx[0] = ~c;
        channels = 2;
    }

    uint16_t w = *(const uint16_t*)src;
    dwave_state.pred[0] = (w >> 8) | ((w & 0xFF) << 8);
    src += 2;
    if ( channels > 1 )
    {
        dwave_state.idx[1] = *src++;
        w = *(const uint16_t*)src;
        dwave_state.pred[1] = (w >> 8) | ((w & 0xFF) << 8);
        src += 2;
    }

    if ( dwave_pSound )
    {
        uint32_t total = dwave_total;
        uint32_t len;
        if ( dwave_state.packets == 0 )
        {
            memcpy(dwave_pPacket, "PSAD", 4);
            len = v * channels * 2 + 0x2E;
            *(uint32_t*)(dwave_pPacket + 4) = DWAVE_SWAP32(len);
            *(int16_t*)(dwave_pPacket + 8)  = 100;
            *(int16_t*)(dwave_pPacket + 10) = dwave_state.packets;
            *(int16_t*)(dwave_pPacket + 12) = 100;
            *(char*)(dwave_pPacket + 16)    = 0x7F;
            *(char*)(dwave_pPacket + 17)    = 0;
            len = total + 0x24;
            *(uint32_t*)(dwave_pPacket + 22) = DWAVE_SWAP32(len);
            *(uint32_t*)(dwave_pPacket + 30) = 0x0C000000;
            *(char*)(dwave_pPacket + 34)     = 1;
            *(char*)(dwave_pPacket + 35)     = 8;
            *(uint32_t*)(dwave_pPacket + 36) = 0;
            len = total;
            *(uint32_t*)(dwave_pPacket + 40) = DWAVE_SWAP32(len);
            *(char*)(dwave_pPacket + 44)     = 5;
            *(char*)(dwave_pPacket + 45)     = 0;
            if ( channels == 1 ) {
                memcpy(dwave_pPacket + 46, "S6MZ", 4);
            }
            else {
                memcpy(dwave_pPacket + 46, "S6SZ", 4);
            }

            *(uint32_t*)(dwave_pPacket + 50) = DWAVE_SWAP32(len);
            dwave_DecodeVima((int16_t*)(dwave_pPacket + 54), src, v, channels);
        }
        else
        {
            memcpy(dwave_pPacket, "PSAD", 4);
            len = v * channels * 2 + 10;
            *(uint32_t*)(dwave_pPacket + 4) = DWAVE_SWAP32(len);
            *(int16_t*)(dwave_pPacket + 8)  = 100;
            *(int16_t*)(dwave_pPacket + 10) = dwave_state.packets;
            *(int16_t*)(dwave_pPacket + 12) = 100;
            *(char*)(dwave_pPacket + 16)    = 0x7F;
            *(char*)(dwave_pPacket + 17)    = 0;
            dwave_DecodeVima((int16_t*)(dwave_pPacket + 18), src, v, channels);
        }

        dwave_pSound->Play((uint8_t*)dwave_pPacket, -1, 0x7F, 0, 0x40);
        dwave_state.packets++;
    }

    return (dwave_state.packets != 0 && dwave_total > 100000) ? 0 : 0x80;
}

int dwave_Query1(void)
{
    return -1;
}

int J3DAPI dwave_Query(const uint8_t* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight)
{
    J3D_UNUSED(pData); J3D_UNUSED(tag); J3D_UNUSED(pX); J3D_UNUSED(pY); J3D_UNUSED(pWidth); J3D_UNUSED(pHeight);
    return -1;
}

void J3DAPI dwave_DecodeVima(int16_t* pDst, const uint8_t* pSrc, uint32_t count, uint32_t channels)
{
    int bitPtr = 0;
    const uint8_t* src = pSrc;

    if ( !dwave_tableReady )
    {
        for ( int start = 0, incer = 0; start < 64; start++, incer++ )
        {
            for ( int pos = 0; pos < 89; pos++ )
            {
                int put = 0;
                uint16_t value = dwave_step[pos];
                for ( uint32_t j = 0x20; j != 0; j >>= 1, value >>= 1 )
                {
                    if ( incer & j ) {
                        put += value;
                    }
                }

                dwave_quant[pos * 64 + start] = (uint16_t)put;
            }
        }

        dwave_tableReady = 1;
    }

    uint16_t bits = *src++;
    bits = (bits << 8) | *src++;
    for ( uint32_t ch = 0; ch < channels; ch++ )
    {
        int16_t* pOut = pDst + ch;
        int idx       = dwave_state.idx[ch];
        int pred      = dwave_state.pred[ch];

        for ( uint32_t n = count; n != 0; n-- )
        {
            uint8_t numBits = dwave_bits[idx];
            int highBit     = 1 << (numBits - 1);
            uint8_t lowBits = (uint8_t)(highBit - 1);

            bitPtr += numBits;
            int val = ((bits & 0xFFFF) >> (16 - bitPtr)) & (highBit | lowBits);
            if ( bitPtr > 7 )
            {
                bits = ((bits & 0xFF) << 8) | *src++;
                bitPtr -= 8;
            }

            if ( val & highBit ) {
                val ^= highBit;
            }
            else {
                highBit = 0;
            }

            if ( val == lowBits )
            {
                pred = (int16_t)(bits << bitPtr) & 0xFFFFFF00;
                bits = ((bits & 0xFF) << 8) | *src++;
                pred |= (uint8_t)(bits >> (8 - bitPtr));
                bits = ((bits & 0xFF) << 8) | *src++;
            }
            else
            {
                int delta = dwave_quant[(val << (7 - numBits)) | (idx << 6)];
                if ( val ) {
                    delta += dwave_step[idx] >> (numBits - 1);
                }

                pred += highBit ? -delta : delta;
                if ( pred < -32768 ) {
                    pred = -32768;
                }
                else if ( pred > 32767 ) {
                    pred = 32767;
                }
            }

            *pOut = (int16_t)pred;
            pOut += channels;
            idx += dwave_deltas[numBits][val];
            if ( idx < 0 ) {
                idx = 0;
            }
            else if ( idx > 88 ) {
                idx = 88;
            }
        }

        dwave_state.idx[ch]  = (int8_t)idx;
        dwave_state.pred[ch] = (int16_t)pred;
    }
}
