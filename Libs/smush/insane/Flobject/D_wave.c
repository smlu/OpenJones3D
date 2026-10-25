// D_wave.c: the VIMA wave frame-object decoder (original 0x004E7D60-0x004E8510).
#include <smush/insane/Flobject/D_wave.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <std/General/stdUtil.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/Engine.h>

#define DWAVE_PACKETSIZE 500000 // Size of the sound packet buffer
#define DWAVE_QUANTROWS  64     // Quantization table entries per step
#define DWAVE_MAXINDEX   (STD_ARRAYLEN(dwave_step) - 1)
#define DWAVE_STARTED_MINTOTAL 100000 // Total size above which the decoder reports the audio as started

#define DWAVE_VOLUME_MAX 0x7F
#define DWAVE_PACKET_WORDVALUE 100
#define DWAVE_PLAY_ID    -1
#define DWAVE_PLAY_PAN   0
#define DWAVE_PLAY_PRIORITY 0x40

#define DWAVE_RET_NOSOUND 0x80 // Returned when the packet stream has not started (or is too short)

// Layout of the sound packet handed to the sound engine (multi-byte sizes are big-endian)
typedef enum eDWavePacket
{
    DWAVE_PACKET_SIZE   = 4,
    DWAVE_PACKET_WORD8  = 8,
    DWAVE_PACKET_INDEX  = 10,
    DWAVE_PACKET_WORD12 = 12,
    DWAVE_PACKET_VOLUME = 16,
    DWAVE_PACKET_PAN    = 17,
    DWAVE_PACKET_DATA   = 18, // Samples of a continuation packet

    // The first packet embeds the sound's header chunk before its samples
    DWAVE_PACKET_HEADERSIZE = 22,
    DWAVE_PACKET_FORMAT     = 30,
    DWAVE_PACKET_CHANNELS   = 34,
    DWAVE_PACKET_BITS       = 35,
    DWAVE_PACKET_FLAGS      = 36,
    DWAVE_PACKET_TOTAL      = 40,
    DWAVE_PACKET_DATATYPE   = 44,
    DWAVE_PACKET_DATAKIND   = 45,
    DWAVE_PACKET_VIMATAG    = 46,
    DWAVE_PACKET_DATASIZE   = 50,
    DWAVE_PACKET_FIRSTDATA  = 54, // Samples of the first packet
} DWavePacket;

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

// The decoder state carried between packets: per channel the step index and the predicted sample
static struct
{
    int8_t index[2];
    int16_t pred[2];
    uint16_t packets;
} dwave_state = { 0 };

static uint32_t dwave_total         = 0;
static LecShare* dwave_pShare       = NULL;
static uint16_t dwave_quant[STD_ARRAYLEN(dwave_step) * DWAVE_QUANTROWS] = { 0 };
static void* dwave_pParam           = NULL;
static SoundInterface* dwave_pSound = NULL;
static uint8_t* dwave_pPacket       = NULL;
static bool dwave_bTableReady       = false;

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

    dwave_pPacket = dwave_pMemory->Alloc(DWAVE_PACKETSIZE, INSANE_FILE, __LINE__);
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

    dwave_state.packets = 0;
    dwave_total         = *(const uint32_t*)(pData + 8);
    return 0;
}

uint32_t J3DAPI dwave_Decode(void* pBitmap, int clip, int x, int y, void* pPalette, const uint8_t* pData, int size, int flags, int unused9, int unused10)
{
    J3D_UNUSED(pBitmap); J3D_UNUSED(clip); J3D_UNUSED(x); J3D_UNUSED(y); J3D_UNUSED(pPalette);
    J3D_UNUSED(size); J3D_UNUSED(flags); J3D_UNUSED(unused9); J3D_UNUSED(unused10);

    const uint8_t* pSrc = pData;

    uint32_t numSamples = DWAVE_SWAP32(*(const uint32_t*)pSrc);
    pSrc += 4;

    // A negative sample count marks an extended header: a stream id (a new id restarts the packet count) and the real count
    if ( (int)numSamples < 0 )
    {
        int header = (int)DWAVE_SWAP32(*(const uint32_t*)pSrc);
        pSrc += 4;

        if ( header != dwave_lastHeader )
        {
            dwave_lastHeader    = header;
            dwave_state.packets = 0;
        }

        numSamples = DWAVE_SWAP32(*(const uint32_t*)pSrc);
        pSrc += 4;
    }

    // A set high bit of the first step index marks stereo; the index is stored inverted
    uint32_t numChannels = 1;
    uint8_t stepIndex    = *pSrc++;
    dwave_state.index[0] = stepIndex;
    if ( (int8_t)stepIndex < 0 )
    {
        dwave_state.index[0] = ~stepIndex;
        numChannels          = 2;
    }

    uint16_t sample     = *(const uint16_t*)pSrc;
    dwave_state.pred[0] = (sample >> 8) | ((sample & 0xFF) << 8);
    pSrc += 2;

    if ( numChannels > 1 )
    {
        dwave_state.index[1] = *pSrc++;
        sample               = *(const uint16_t*)pSrc;
        dwave_state.pred[1]  = (sample >> 8) | ((sample & 0xFF) << 8);
        pSrc += 2;
    }

    if ( dwave_pSound )
    {
        uint8_t* pPacket = dwave_pPacket;
        memcpy(pPacket, "PSAD", 4);

        if ( dwave_state.packets == 0 )
        {
            *(uint32_t*)(pPacket + DWAVE_PACKET_SIZE) = DWAVE_SWAP32((uint32_t)(numSamples * numChannels * 2 + 0x2E));
            *(int16_t*)(pPacket + DWAVE_PACKET_WORD8)  = DWAVE_PACKET_WORDVALUE;
            *(int16_t*)(pPacket + DWAVE_PACKET_INDEX)  = dwave_state.packets;
            *(int16_t*)(pPacket + DWAVE_PACKET_WORD12) = DWAVE_PACKET_WORDVALUE;
            pPacket[DWAVE_PACKET_VOLUME] = DWAVE_VOLUME_MAX;
            pPacket[DWAVE_PACKET_PAN]    = 0;

            *(uint32_t*)(pPacket + DWAVE_PACKET_HEADERSIZE) = DWAVE_SWAP32((uint32_t)(dwave_total + 0x24));
            *(uint32_t*)(pPacket + DWAVE_PACKET_FORMAT) = 0x0C000000;
            pPacket[DWAVE_PACKET_CHANNELS]              = 1;
            pPacket[DWAVE_PACKET_BITS]                  = 8;
            *(uint32_t*)(pPacket + DWAVE_PACKET_FLAGS)  = 0;
            *(uint32_t*)(pPacket + DWAVE_PACKET_TOTAL) = DWAVE_SWAP32((uint32_t)(dwave_total));
            pPacket[DWAVE_PACKET_DATATYPE] = 5;
            pPacket[DWAVE_PACKET_DATAKIND] = 0;
            memcpy(pPacket + DWAVE_PACKET_VIMATAG, numChannels == 1 ? "S6MZ" : "S6SZ", 4);
            *(uint32_t*)(pPacket + DWAVE_PACKET_DATASIZE) = DWAVE_SWAP32((uint32_t)(dwave_total));

            dwave_DecodeVima((int16_t*)(pPacket + DWAVE_PACKET_FIRSTDATA), pSrc, numSamples, numChannels);
        }
        else
        {
            *(uint32_t*)(pPacket + DWAVE_PACKET_SIZE) = DWAVE_SWAP32((uint32_t)(numSamples * numChannels * 2 + 10));
            *(int16_t*)(pPacket + DWAVE_PACKET_WORD8)  = DWAVE_PACKET_WORDVALUE;
            *(int16_t*)(pPacket + DWAVE_PACKET_INDEX)  = dwave_state.packets;
            *(int16_t*)(pPacket + DWAVE_PACKET_WORD12) = DWAVE_PACKET_WORDVALUE;
            pPacket[DWAVE_PACKET_VOLUME] = DWAVE_VOLUME_MAX;
            pPacket[DWAVE_PACKET_PAN]    = 0;

            dwave_DecodeVima((int16_t*)(pPacket + DWAVE_PACKET_DATA), pSrc, numSamples, numChannels);
        }

        dwave_pSound->Play(pPacket, DWAVE_PLAY_ID, DWAVE_VOLUME_MAX, DWAVE_PLAY_PAN, DWAVE_PLAY_PRIORITY);
        dwave_state.packets++;
    }

    return (dwave_state.packets != 0 && dwave_total > DWAVE_STARTED_MINTOTAL) ? 0 : DWAVE_RET_NOSOUND;
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

// Decodes variable bit rate ADPCM (VIMA): per channel, every sample is a code of 4-7 bits (the width follows the step
// index); the code's top bit is the sign, the rest selects a fraction of the step size. The all-ones magnitude is an escape
// that carries the next 16-bit sample verbatim. The code width and magnitude then also move the step index.
void J3DAPI dwave_DecodeVima(int16_t* pDst, const uint8_t* pSrc, uint32_t count, uint32_t channels)
{
    const uint8_t* pIn = pSrc;

    if ( !dwave_bTableReady )
    {
        // dwave_quant[step][code]: the sum of the step size's halvings selected by the bits of the code
        for ( int code = 0; code < DWAVE_QUANTROWS; code++ )
        {
            for ( size_t step = 0; step < STD_ARRAYLEN(dwave_step); step++ )
            {
                int sum        = 0;
                uint16_t value = dwave_step[step];
                for ( uint32_t mask = 0x20; mask != 0; mask >>= 1, value >>= 1 )
                {
                    if ( code & mask ) {
                        sum += value;
                    }
                }

                dwave_quant[step * DWAVE_QUANTROWS + code] = (uint16_t)sum;
            }
        }

        dwave_bTableReady = true;
    }

    // The bit window: 16 bits of the stream, refilled a byte at a time
    int bitPos      = 0;
    uint16_t window = *pIn++;
    window          = (window << 8) | *pIn++;

    for ( uint32_t ch = 0; ch < channels; ch++ )
    {
        int16_t* pOut = pDst + ch;
        int index     = dwave_state.index[ch];
        int pred      = dwave_state.pred[ch];

        for ( uint32_t n = count; n != 0; n-- )
        {
            uint8_t numBits = dwave_bits[index];
            int signBit     = 1 << (numBits - 1);
            uint8_t escape  = (uint8_t)(signBit - 1); // All magnitude bits set

            bitPos += numBits;
            int code = ((window & 0xFFFF) >> (16 - bitPos)) & (signBit | escape);
            if ( bitPos > 7 )
            {
                window = ((window & 0xFF) << 8) | *pIn++;
                bitPos -= 8;
            }

            if ( code & signBit ) {
                code ^= signBit;
            }
            else {
                signBit = 0;
            }

            if ( code == escape )
            {
                pred   = (int16_t)(window << bitPos) & 0xFFFFFF00;
                window = ((window & 0xFF) << 8) | *pIn++;
                pred  |= (uint8_t)(window >> (8 - bitPos));
                window = ((window & 0xFF) << 8) | *pIn++;
            }
            else
            {
                int delta = dwave_quant[(code << (7 - numBits)) | (index * DWAVE_QUANTROWS)];
                if ( code ) {
                    delta += dwave_step[index] >> (numBits - 1);
                }

                pred += signBit ? -delta : delta;
                if ( pred < INT16_MIN ) {
                    pred = INT16_MIN;
                }
                else if ( pred > INT16_MAX ) {
                    pred = INT16_MAX;
                }
            }

            *pOut = (int16_t)pred;
            pOut += channels;

            index += dwave_deltas[numBits][code];
            if ( index < 0 ) {
                index = 0;
            }
            else if ( index > (int)DWAVE_MAXINDEX ) {
                index = (int)DWAVE_MAXINDEX;
            }
        }

        dwave_state.index[ch] = (int8_t)index;
        dwave_state.pred[ch]  = (int16_t)pred;
    }
}
