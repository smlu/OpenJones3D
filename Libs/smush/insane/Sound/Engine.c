// Engine.c: the sound engine (`SOUND=insane`) (original 0x004E5470-0x004E74A0).
#include <smush/insane/Sound/Engine.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <smush/insane/File/Framestream.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/MixerAsm.h>
#include <smush/insane/Timer/Timer.h>

#define SOUND_ABS(a) ((a) < 0 ? -(a) : (a))

static LecModule sound_module = { sound_Register, sound_Unregister, sound_Startup, sound_Shutdown };
static SoundInterface sound_interface = {
    sound_Play, sound_Stop, sound_Pause, sound_Resume, sound_StopAll, sound_GetVolume, sound_SetVolume, sound_GetPan, sound_SetPan,
    sound_GetMasterVolume, sound_SetMasterVolume, sound_GetRate, sound_SetRate, sound_GetPosition, sound_Query, sound_Control
};

// Bytes per sample frame of each track format
static const int sound_frameSize[6] = { 1, 2, 2, 4, 2, 4 };

static int sound_numTracks = 4;
static int sound_volume = 127;
static int sound_rate = 22050;
static int sound_duckFloor = 0x40;
static int sound_duckSpeed = 0x100;
static int sound_duck = 0x100;
static int sound_reset = 1;

static MemoryDebugInterface* sound_pMemory = NULL;
static int sound_streamParts[16];
static SndChannel sound_channels[16];
static int sound_trackSizes[16];
static SndDriver* sound_pDriver = NULL;
static int sound_frames = 0;
static LecShare* sound_pShare = NULL;
static int sound_bufferSize = 0;
static int sound_streamIds[16];
static int sound_playMusic = 0;
static int sound_playVoice = 0;
static TimerInterface* sound_pTimer = NULL;
static int sound_playEffects = 0;
static int sound_swapPan = 0;
static SoundTrack sound_tracks[16];
static void* sound_pParam = NULL;
static int sound_driverRate = 0;
static void* sound_pDriverParam = NULL;
static int sound_streamSeq[16];
static volatile uint8_t sound_vars[256]; // Shared with the timer thread
static int sound_field589C = 0;
static int sound_paused = 0;
static int sound_started = 0;
static int sound_syncTicks = 0;
static double sound_clock = 0;

static int sound_BE16(const uint8_t* p)
{
    return (p[0] << 8) | p[1];
}

static int sound_BE32(const uint8_t* p)
{
    return (((((p[0] << 8) | p[1]) << 8) | p[2]) << 8) | p[3];
}

int J3DAPI sound_Register(LecShare* pShare, void* pParam)
{
    sound_pShare = pShare;
    sound_pParam = pParam;
    pShare->Publish("LECMODULE=insane_sound", &sound_module);
    sound_pShare->Publish("SOUND=insane", &sound_interface);
    sound_pDriver = NULL;
    sound_driverRate = 22050;
    sound_bufferSize = 64000;
    sound_field589C = 0;
    sound_swapPan = 0;
    sound_frames = 0;
    sound_pDriverParam = NULL;
    for ( int i = 0; i < 16; i++ ) {
        sound_trackSizes[i] = i < 1 ? 100000 : 0;
    }

    sound_trackSizes[0] = 500000;
    return 0;
}

void sound_Unregister(void)
{
    sound_pShare->Unpublish("SOUND=insane");
    sound_pShare->Unpublish("LECMODULE=insane_sound");
}

int sound_Startup(void)
{
    sound_pMemory = sound_pShare->Subscribe("MEMORY.workspace", sound_pParam);
    sound_pTimer  = sound_pShare->Subscribe("TIMER.external_sync", sound_pParam);
    sound_pDriver = sound_pShare->Subscribe("SNDDRIVER.driver", sound_pDriverParam ? sound_pDriverParam : sound_pParam);
    if ( sound_pMemory == NULL ) {
        sound_pShare->ReportError("Sound Module: Cannot allocate memory services.");
        return -1;
    }

    if ( sound_pTimer == NULL ) {
        sound_pShare->ReportError("Sound Module: Cannot allocate timer services.");
        return -1;
    }

    if ( sound_pDriver == NULL ) {
        sound_pShare->ReportError("Sound Module: Cannot allocate snddriver services.");
        return -1;
    }

    return sound_Open(sound_driverRate, sound_bufferSize, sound_trackSizes[0], sound_trackSizes[1], sound_trackSizes[2], sound_trackSizes[3],
                      sound_trackSizes[4], sound_trackSizes[5], sound_trackSizes[6], sound_trackSizes[7], sound_trackSizes[8], sound_trackSizes[9],
                      sound_trackSizes[10], sound_trackSizes[11], sound_trackSizes[12], sound_trackSizes[13], sound_trackSizes[14],
                      sound_trackSizes[15], 0);
}

void sound_Shutdown(void)
{
    sound_pShare->Unsubscribe(sound_pMemory);
    sound_pShare->Unsubscribe(sound_pDriver);
    sound_pShare->Unsubscribe(sound_pTimer);
    sound_Close();
}

int J3DAPI sound_Open(int rate, int size, ...)
{
    sound_numTracks = 0;
    sound_volume = 127;
    for ( int i = 0; i < 256; i++ ) {
        sound_vars[i] = 0;
    }

    va_list args;
    va_start(args, size);
    int n = va_arg(args, int);
    while ( n != 0 )
    {
        if ( sound_AddTrack(n & ~3, size) != 0 )
        {
            sound_Close();
            va_end(args);
            return -1;
        }

        n = va_arg(args, int);
    }

    va_end(args);
    sound_started = !sound_pDriver->Start(rate, sound_numTracks, sound_Callback);
    sound_rate = sound_pDriver->GetRate();
    sound_frames = 0;
    sound_playMusic = 1;
    sound_playEffects = 1;
    sound_playVoice = 1;
    sound_pTimer->Sync(0);
    return 0;
}

int sound_Close(void)
{
    if ( sound_started )
    {
        sound_pDriver->Stop();
        sound_started = 0;
    }

    for ( int i = 0; i < sound_numTracks; i++ )
    {
        if ( sound_tracks[i].pBuf ) {
            sound_pMemory->Free(sound_tracks[i].pBuf, INSANE_FILE, __LINE__);
        }

        sound_tracks[i].pBuf = NULL;
        if ( sound_tracks[i].pMix ) {
            sound_pMemory->Free(sound_tracks[i].pMix, INSANE_FILE, __LINE__);
        }

        sound_tracks[i].pMix = NULL;
    }

    sound_numTracks = 0;
    return sound_frames;
}

int J3DAPI sound_AddTrack(int size, int minSize)
{
    int i = sound_numTracks;

    sound_tracks[i].state = 0;
    sound_tracks[i].position = 0;
    sound_tracks[i].priority = 0;
    sound_tracks[i].id = SOUND_NOTRACK;
    sound_tracks[i].pBuf = NULL;
    sound_tracks[i].pMix = NULL;
    sound_tracks[i].size = size & ~3;

    sound_tracks[i].pMix = sound_pMemory->Alloc(0x1800, INSANE_FILE, __LINE__);
    if ( sound_tracks[i].pMix == NULL ) {
        return -1;
    }

    sound_tracks[i].pBuf = sound_pMemory->Alloc(sound_tracks[i].size, INSANE_FILE, __LINE__);
    if ( sound_tracks[i].pBuf == NULL )
    {
        while ( sound_tracks[i].size > minSize )
        {
            sound_tracks[i].pBuf = sound_pMemory->Alloc(sound_tracks[i].size, INSANE_FILE, __LINE__);
            if ( sound_tracks[i].pBuf != NULL ) {
                break;
            }

            sound_tracks[i].size -= 10000;
        }

        sound_tracks[i].size &= ~3;

        int bFailed;
        if ( sound_tracks[i].size <= minSize ) {
            bFailed = 1;
        }
        else
        {
            if ( sound_tracks[i].pBuf ) {
                sound_pMemory->Free(sound_tracks[i].pBuf, INSANE_FILE, __LINE__);
            }

            sound_tracks[i].pBuf = NULL;
            sound_tracks[i].size -= minSize;
            sound_tracks[i].pBuf = sound_pMemory->Alloc(sound_tracks[i].size, INSANE_FILE, __LINE__);
            bFailed = sound_tracks[i].pBuf == NULL;
        }

        if ( bFailed )
        {
            if ( sound_tracks[i].pMix ) {
                sound_pMemory->Free(sound_tracks[i].pMix, INSANE_FILE, __LINE__);
            }

            sound_tracks[i].pMix = NULL;
            return -2;
        }
    }

    framestream_CopyB(sound_tracks[i].pBuf, 0x7F, sound_tracks[i].size);
    sound_numTracks++;
    return 0;
}

void J3DAPI sound_Play(uint8_t* pData, int id, int volume, int pan, int priority)
{
    if ( !sound_started ) {
        return;
    }

    switch ( (uint32_t)sound_BE32(pData) )
    {
        case 'Crea':
        {
            int offset = pData[0x14] + (pData[0x15] << 8);
            // TODO: [BUG] The VOC block's 24-bit length is built from byte offset + 1 three times instead of offset + 1..3.
            sound_StartVoc(pData + offset + 6, (((pData[offset + 1] << 8) + pData[offset + 1]) << 8) + pData[offset + 1], id, volume, pan, priority);
        } break;

        case 'PSAD':
        case 'PVOC':
        {
            if ( pData[8] == 0 && pData[9] == 0 && pData[0xC] == 0 && pData[0xD] == 0 && pData[0x10] == 0 && pData[0x11] == 0 )
            {
                sound_StartStream(pData + 0x14, sound_BE32(pData + 4) - 0xC, id, volume, pan, priority, sound_BE16(pData + 0xA), sound_BE16(pData + 0xE),
                                  sound_BE16(pData + 0x12), 1);
            }
            else
            {
                sound_StartStream(pData + 0x12, sound_BE32(pData + 4) - 0xA, id, pData[0x10] * volume >> 7,
                                  (int8_t)pData[0x11] < -127 || (int8_t)pData[0x11] > 127 ? 0x80 : (int8_t)pData[0x11] + pan,
                                  *(int16_t*)(pData + 0xE) | priority, *(int16_t*)(pData + 8), *(int16_t*)(pData + 0xA), *(int16_t*)(pData + 0xC), 1);
            }
        } break;

        case 'SAUD':
            sound_StartStream(pData, sound_BE32(pData + 4) + 8, id, volume, pan, priority, 0, 0, 1, 0);
            break;
    }
}

void J3DAPI sound_Stop(int id)
{
    for ( int i = 0; i < sound_numTracks; i++ )
    {
        if ( sound_tracks[i].id == id )
        {
            sound_tracks[i].state = 0;
            sound_tracks[i].id = SOUND_NOTRACK;
        }
    }
}

void sound_Pause(void)
{
    sound_paused++;
}

void sound_Resume(void)
{
    if ( sound_paused ) {
        sound_paused--;
    }
}

void sound_StopAll(void)
{
    for ( int i = 0; i < sound_numTracks; i++ )
    {
        sound_tracks[i].state = 0;
        sound_tracks[i].id = SOUND_NOTRACK;
    }
}

void J3DAPI sound_SetVolume(int id, int volume)
{
    if ( id == SOUND_NOTRACK )
    {
        sound_volume = volume;
        return;
    }

    for ( int i = 0; i < sound_numTracks; i++ )
    {
        if ( sound_tracks[i].id == id ) {
            sound_tracks[i].volume = (int8_t)volume;
        }
    }
}

int J3DAPI sound_GetVolume(int id)
{
    if ( id == SOUND_NOTRACK ) {
        return sound_volume;
    }

    for ( int i = 0; i < sound_numTracks; i++ )
    {
        if ( sound_tracks[i].id == id ) {
            return sound_tracks[i].volume;
        }
    }

    return -1;
}

void J3DAPI sound_SetPan(int id, int pan)
{
    for ( int i = 0; i < sound_numTracks; i++ )
    {
        if ( sound_tracks[i].id == id ) {
            sound_tracks[i].pan = (int8_t)pan;
        }
    }
}

int J3DAPI sound_GetPan(int id)
{
    for ( int i = 0; i < sound_numTracks; i++ )
    {
        if ( sound_tracks[i].id == id ) {
            return sound_tracks[i].pan;
        }
    }

    return -1;
}

void J3DAPI sound_SetMasterVolume(int volume)
{
    if ( sound_pDriver ) {
        sound_pDriver->SetVolume(volume);
    }
}

int sound_GetMasterVolume(void)
{
    if ( sound_pDriver ) {
        return sound_pDriver->GetVolume();
    }

    return -1;
}

void J3DAPI sound_SetRate(int rate)
{
    sound_rate = rate;
    if ( sound_pDriver ) {
        sound_pDriver->SetRate(rate);
    }
}

int sound_GetRate(void)
{
    if ( sound_pDriver ) {
        return sound_pDriver->GetRate();
    }

    return -1;
}

int J3DAPI sound_GetPosition(int id)
{
    if ( id == SOUND_NOTRACK )
    {
        for ( int i = 0; i < sound_numTracks; i++ )
        {
            if ( sound_tracks[i].state != 0 ) {
                return sound_tracks[i].position + 1;
            }
        }

        return 0;
    }

    if ( id < 0 && -1 - id < sound_numTracks )
    {
        if ( sound_tracks[-1 - id].state == 0 ) {
            return 0;
        }

        return sound_tracks[-1 - id].position + 1;
    }

    for ( int i = 0; i < sound_numTracks; i++ )
    {
        if ( sound_tracks[i].id == id )
        {
            if ( sound_tracks[i].state == 0 ) {
                return 0;
            }

            return sound_tracks[i].position + 1;
        }
    }

    return 0;
}

int J3DAPI sound_Control(int a1, int tag, int a3, int a4)
{
    J3D_UNUSED(a1);
    switch ( tag )
    {
        case 1:
            sound_SetVar((uint8_t)a3, (uint8_t)a4);
            return 0;

        case 2:
            if ( a3 ) {
                sound_duckFloor = a3;
            }

            if ( a4 ) {
                sound_duckSpeed = a4;
            }

            return 0;

        case 3:
            sound_playMusic = a3;
            return 0;

        case 4:
            sound_playEffects = a3;
            return 0;

        case 5:
            sound_playVoice = a3;
            return 0;
    }

    return -1;
}

int J3DAPI sound_Query(int a1, int tag, int* pA, int* pB)
{
    J3D_UNUSED(a1);
    switch ( tag )
    {
        case 1:
            if ( pA ) {
                *pA = sound_GetVar((uint8_t)(uintptr_t)pB);
            }

            return 0;

        case 2:
            if ( pA ) {
                *pA = sound_duckFloor;
            }

            if ( pB ) {
                *pB = sound_duckSpeed;
            }

            return 0;

        case 3:
            if ( pA ) {
                *pA = sound_playMusic;
            }

            return 0;

        case 4:
            if ( pA ) {
                *pA = sound_playEffects;
            }

            return 0;

        case 5:
            if ( pA ) {
                *pA = sound_playVoice;
            }

            return 0;
    }

    return -1;
}

uint8_t J3DAPI sound_GetVar(uint8_t index)
{
    if ( index == 0xFF ) {
        return (uint8_t)sound_volume;
    }

    return sound_vars[index];
}

void J3DAPI sound_SetVar(uint8_t index, uint8_t value)
{
    if ( index == 0xFF ) {
        sound_volume = value;
    }

    sound_vars[index] = value;
}

void J3DAPI sound_StartVoc(uint8_t* pData, int len, int id, int volume, int pan, int priority)
{
    int bestSize = 0;
    for ( int i = sound_numTracks - 1; i >= 0; i-- )
    {
        if ( sound_tracks[i].size > bestSize ) {
            bestSize = sound_tracks[i].size;
        }
    }

    int bestDiff = 10000000;
    for ( int i = sound_numTracks - 1; i >= 0; i-- )
    {
        if ( SOUND_ABS(sound_tracks[i].size - len) < bestDiff && len < sound_tracks[i].size )
        {
            bestDiff = SOUND_ABS(sound_tracks[i].size - len);
            bestSize = sound_tracks[i].size;
        }
    }

    int score[16];
    for ( int i = sound_numTracks - 1; i >= 0; i-- )
    {
        score[i] = 0;
        if ( sound_tracks[i].size == bestSize
            && (sound_tracks[i].state == 0 || sound_tracks[i].state == 3 || sound_tracks[i].priority <= priority) )
        {
            if ( sound_tracks[i].state == 0 ) {
                score[i] += 0x1000;
            }

            if ( sound_tracks[i].priority < priority ) {
                score[i] += 0x800;
            }

            if ( sound_tracks[i].priority == priority ) {
                score[i] += 0x400;
            }

            if ( sound_tracks[i].state == 3 ) {
                score[i] += 0x200;
            }

            score[i] += sound_tracks[i].age + 1;
        }
    }

    bestDiff = 0;
    int best = -1;
    for ( int i = sound_numTracks - 1; i >= 0; i-- )
    {
        if ( score[i] > bestDiff )
        {
            best = i;
            bestDiff = score[i];
        }
    }

    if ( best < 0 ) {
        return;
    }

    for ( int i = sound_numTracks - 1; i >= 0; i-- )
    {
        if ( sound_tracks[i].age < 0x100 ) {
            sound_tracks[i].age++;
        }
    }

    sound_tracks[best].age = 0;
    sound_tracks[best].state = 0;
    sound_tracks[best].pBuf[0] = 1;
    sound_tracks[best].pBuf[1] = 8;

    uint8_t* p = sound_tracks[best].pBuf + 2;
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0;

    p = sound_tracks[best].pBuf + 6;
    p[0] = (uint8_t)(len >> 24);
    p[1] = (uint8_t)(len >> 16);
    p[2] = (uint8_t)(len >> 8);
    p[3] = (uint8_t)len;

    sound_tracks[best].pBuf[0xA] = 0;
    sound_tracks[best].pBuf[0xB] = 0;
    framestream_Copy(sound_tracks[best].pBuf + 0xC, pData, len);
    sound_tracks[best].pBlock = sound_tracks[best].pBuf;
    sound_tracks[best].pData = sound_tracks[best].pBuf + 0xC;
    sound_tracks[best].ringSize = sound_tracks[best].size - 0xC;
    sound_tracks[best].written = len;
    sound_tracks[best].id = id;
    sound_tracks[best].volume = (int8_t)volume;
    sound_tracks[best].pan = (int8_t)pan;
    sound_tracks[best].priority = priority;
    sound_tracks[best].position = 0;
    sound_tracks[best].state = 2;
}

void J3DAPI sound_StartStream(uint8_t* pData, int len, int id, int volume, int pan, int priority, int stream, int part, int parts, int a10)
{
    if ( part == 0 )
    {
        int total;
        if ( parts == 1 && a10 == 0 ) {
            total = 0;
        }
        else {
            total = sound_BE32(pData + 4);
        }

        int bestSize = 0;
        for ( int i = sound_numTracks - 1; i >= 0; i-- )
        {
            if ( sound_tracks[i].size > bestSize ) {
                bestSize = sound_tracks[i].size;
            }
        }

        int bestDiff = 10000000;
        if ( (priority & 0xC0) == 0x40 && parts > 1 ) {
            sound_SyncTimer();
        }

        if ( (priority & 0xC0) != 0x40 )
        {
            for ( int i = sound_numTracks - 1; i >= 0; i-- )
            {
                if ( SOUND_ABS(sound_tracks[i].size - total) < bestDiff && sound_tracks[i].size > total * 2 / 3
                    && sound_tracks[i].size > (parts > 1 ? len * 2 : len) )
                {
                    bestDiff = SOUND_ABS(sound_tracks[i].size - total);
                    bestSize = sound_tracks[i].size;
                }
            }
        }

        int score[16];
        for ( int i = sound_numTracks - 1; i >= 0; i-- )
        {
            score[i] = 0;
            if ( sound_tracks[i].size == bestSize
                && (sound_tracks[i].state == 0 || sound_tracks[i].state == 3 || sound_tracks[i].priority <= priority) )
            {
                if ( sound_tracks[i].state == 0 ) {
                    score[i] += 0x1000;
                }

                if ( sound_tracks[i].priority < priority ) {
                    score[i] += 0x800;
                }

                if ( sound_tracks[i].priority == priority ) {
                    score[i] += 0x400;
                }

                if ( sound_tracks[i].state == 3 ) {
                    score[i] += 0x200;
                }

                score[i] += sound_tracks[i].age + 1;
            }
        }

        bestDiff = -1;
        int best = -1;
        for ( int i = sound_numTracks - 1; i >= 0; i-- )
        {
            if ( score[i] != 0 && score[i] > bestDiff )
            {
                best = i;
                bestDiff = score[i];
            }
        }

        if ( best < 0 ) {
            return;
        }

        for ( int i = sound_numTracks - 1; i >= 0; i-- )
        {
            if ( sound_tracks[i].age < 0xFF ) {
                sound_tracks[i].age++;
            }
        }

        sound_tracks[best].age = 0;
        sound_tracks[best].state = 0;
        sound_streamSeq[best] = 0;
        sound_streamIds[best] = stream;
        sound_streamParts[best] = parts;

        int hdrLen = sound_BE32(pData + 0xC);
        total = sound_BE32(pData + hdrLen + 0x14);
        if ( parts == 1 && a10 == 0 )
        {
            sound_tracks[best].pBlock = pData + 0x10;
            sound_tracks[best].ringSize = len - hdrLen - 0x18;
        }
        else
        {
            framestream_Copy(sound_tracks[best].pBuf, pData, len < sound_tracks[best].size ? len : sound_tracks[best].size);
            sound_tracks[best].pBlock = sound_tracks[best].pBuf + 0x10;
            sound_tracks[best].ringSize = sound_tracks[best].size - hdrLen - 0x18;
        }

        sound_tracks[best].pData = sound_tracks[best].pBlock + hdrLen + 8;
        sound_tracks[best].written = len - hdrLen - 0x18;
        sound_tracks[best].total = total;
        sound_tracks[best].id = id;
        sound_tracks[best].volume = 0x7F;
        sound_tracks[best].pan = 0;
        if ( volume >= 0 && volume < 0x80 ) {
            sound_tracks[best].volume = (int8_t)volume;
        }

        if ( pan > -0x80 && pan < 0x80 ) {
            sound_tracks[best].pan = (int8_t)pan;
        }

        sound_tracks[best].priority = priority;
        sound_tracks[best].position = 0;
        sound_tracks[best].state = 2;

        switch ( (uint32_t)sound_BE32(sound_tracks[best].pData - 8) )
        {
            case 'S6SZ':
                sound_tracks[best].format = 5;
                break;

            case 'S6MZ':
                sound_tracks[best].format = 4;
                break;

            case 'SD6M':
                sound_tracks[best].format = 2;
                break;

            case 'SD2S':
            case 'SD8S':
                sound_tracks[best].format = 1;
                break;

            case 'SD6S':
                sound_tracks[best].format = 3;
                break;

            default:
                sound_tracks[best].format = 0;
                break;
        }

        return;
    }

    int i = sound_numTracks - 1;
    for ( ; ; i-- )
    {
        if ( i < 0 ) {
            return;
        }

        if ( sound_tracks[i].state != 0 && stream == sound_streamIds[i] && part == sound_streamSeq[i] + 1 && parts == sound_streamParts[i] ) {
            break;
        }
    }

    sound_streamSeq[i]++;

    int pos = sound_tracks[i].written % sound_tracks[i].ringSize;
    if ( pos + len > sound_tracks[i].ringSize )
    {
        framestream_Copy(sound_tracks[i].pData + pos, pData, sound_tracks[i].ringSize - pos);
        framestream_Copy(sound_tracks[i].pData, pData + (sound_tracks[i].ringSize - pos), pos - sound_tracks[i].ringSize + len);
    }
    else {
        framestream_Copy(sound_tracks[i].pData + pos, pData, len);
    }

    if ( volume >= 0 && volume < 0x7F ) {
        sound_tracks[i].volume = (int8_t)volume;
    }

    if ( pan > -0x80 && pan < 0x80 ) {
        sound_tracks[i].pan = (int8_t)pan;
    }

    sound_tracks[i].written += len;
}

void sound_SyncTimer(void)
{
    sound_syncTicks = sound_pTimer->GetTicks();
}

void J3DAPI sound_Callback(int n, SoundFeed pfnFeed)
{
    sound_frames++;
    sound_Advance(n * 1193180.0 / sound_rate);
    if ( sound_paused != 0 ) {
        return;
    }

    if ( sound_reset != 0 )
    {
        sound_reset = 0;
        for ( int i = 0; i < 16; i++ )
        {
            sound_channels[i].fadeLeft = 0;
            sound_channels[i].loopLeft = 0;
        }
    }

    int ducking = 0;
    for ( int i = 0; i < sound_numTracks; i++ )
    {
        int enabled;
        if ( ((sound_tracks[i].priority & 0xC0) == 0x80 && sound_playEffects) || ((sound_tracks[i].priority & 0xC0) == 0x40 && sound_playMusic) || ((sound_tracks[i].priority & 0xC0) == 0 && sound_playVoice) ) {
            enabled = 1;
        }
        else {
            enabled = 0;
        }

        int volume = sound_tracks[i].volume * sound_volume / 127;
        if ( (sound_tracks[i].priority & 0xC0) == 0x40 && sound_playEffects ) {
            volume = sound_duck * volume >> 8;
        }

        if ( sound_tracks[i].state == 2 && sound_channels[i].prevState == 1 )
        {
            int skip = sound_channels[i].pos % sound_channels[i].ringSize;
            sound_channels[i].fadeLeft = 0x1800;
            sound_channels[i].fadeVolume = sound_tracks[i].volume;
            sound_channels[i].fadeFormat = sound_tracks[i].format;
            sound_channels[i].fadeRate = sound_channels[i].rate;
            sound_Fill(sound_tracks[i].pMix, 0x80, 0x1800);

            int len = sound_channels[i].ringSize - skip;
            if ( len > 0x1800 ) {
                len = 0x1800;
            }

            sound_Copy(sound_tracks[i].pMix, sound_channels[i].pData + skip, len);
            sound_channels[i].ramp = 0;
        }
        else if ( sound_tracks[i].state == 1 )
        {
            if ( sound_channels[i].pos < sound_tracks[i].written - sound_tracks[i].ringSize + 15000 && sound_tracks[i].written < sound_tracks[i].total )
            {
                sound_channels[i].ramp = 0;

                int len = sound_tracks[i].written - sound_channels[i].pos - 15000;
                if ( len > sound_channels[i].avail ) {
                    len = sound_channels[i].avail;
                }

                if ( sound_channels[i].pos + len > sound_tracks[i].total - sound_channels[i].ringSize ) {
                    len = sound_tracks[i].total - sound_channels[i].ringSize - sound_channels[i].pos;
                }

                if ( len > 0 )
                {
                    sound_channels[i].fadeLeft = 0x1800;
                    sound_channels[i].fadeRate = sound_channels[i].rate;
                    sound_Copy(sound_tracks[i].pMix, sound_channels[i].pData + sound_channels[i].pos % sound_channels[i].ringSize, 0x1800);
                    sound_channels[i].pos += len;
                    sound_channels[i].avail -= len;
                }
            }
        }

        if ( sound_channels[i].fadeLeft > sound_frameSize[sound_channels[i].fadeFormat] )
        {
            int want = sound_frameSize[sound_channels[i].fadeFormat] * sound_channels[i].fadeRate * n / sound_rate;
            int done = 0;
            int left = sound_channels[i].fadeLeft < want ? sound_channels[i].fadeLeft : want;
            left -= left % 4;
            while ( left >= 4 )
            {
                int chunk = left > 0x400 ? 0x400 : left;
                int fadeVolume = sound_channels[i].fadeLeft * sound_channels[i].fadeVolume * sound_volume / (0x1800 * 127);

                int out;
                if ( chunk == want ) {
                    out = n;
                }
                else {
                    out = chunk / sound_frameSize[sound_channels[i].fadeFormat] * sound_rate / sound_channels[i].fadeRate;
                }

                if ( enabled )
                {
                    int pan = sound_swapPan ? -sound_tracks[i].pan : sound_tracks[i].pan;
                    pfnFeed(sound_tracks[i].pMix + 0x1800 - sound_channels[i].fadeLeft, done, out, chunk, 1, fadeVolume, pan, sound_channels[i].fadeFormat);
                }

                done += out;
                left -= chunk;
                sound_channels[i].fadeLeft -= chunk;
            }
        }

        if ( sound_tracks[i].state == 2 )
        {
            if ( sound_channels[i].prevState != 1 ) {
                sound_channels[i].ramp = 16;
            }

            sound_channels[i].pBlock = sound_tracks[i].pBlock;
            sound_channels[i].pData = sound_tracks[i].pData;
            sound_channels[i].ringSize = sound_tracks[i].ringSize;
            sound_channels[i].format = sound_tracks[i].format;
            sound_channels[i].avail = 0;
            sound_channels[i].loopLeft = 0;
            sound_tracks[i].state = 1;
        }

        if ( sound_tracks[i].state != 0 )
        {
            int done = 0;
            int left = n;
            while ( left > 0 )
            {
                if ( sound_channels[i].avail > 0 )
                {
                    int offset = sound_channels[i].pos % sound_channels[i].ringSize;
                    int want = sound_frameSize[sound_channels[i].format] * sound_channels[i].rate * left / sound_rate;
                    int len = sound_channels[i].avail < want ? sound_channels[i].avail : want;
                    if ( offset + len > sound_channels[i].ringSize ) {
                        len = sound_channels[i].ringSize - offset;
                    }

                    if ( sound_channels[i].pos + len > sound_tracks[i].written )
                    {
                        if ( sound_channels[i].ramp != 0 ) {
                            sound_channels[i].ramp--;
                        }

                        sound_tracks[i].state = 3;
                        if ( len > 0x400 ) {
                            len = 0x400;
                        }

                        sound_channels[i].pos -= len;
                        sound_channels[i].avail += len;
                        offset = sound_channels[i].pos % sound_channels[i].ringSize;
                    }
                    else
                    {
                        if ( sound_channels[i].ramp < 16 ) {
                            sound_channels[i].ramp++;
                        }

                        if ( len > 0x400 ) {
                            len = 0x400;
                        }

                        sound_tracks[i].state = 1;
                        if ( (sound_tracks[i].priority & 0xC0) == 0x80 ) {
                            ducking = 1;
                        }
                    }

                    int out;
                    if ( len == want ) {
                        out = left;
                    }
                    else {
                        out = len / sound_frameSize[sound_channels[i].format] * sound_rate / sound_channels[i].rate;
                    }

                    if ( enabled )
                    {
                        int pan = sound_swapPan ? -sound_tracks[i].pan : sound_tracks[i].pan;
                        pfnFeed(sound_channels[i].pData + offset, done, out, len, 1, volume * sound_channels[i].ramp >> 4, pan, sound_channels[i].format);
                    }

                    sound_channels[i].avail -= len;
                    sound_channels[i].pos += len;
                    left -= out;
                    done += out;
                }

                while ( sound_channels[i].avail <= 0 )
                {
                    if ( left == 0 ) {
                        break;
                    }

                    uint8_t* p = sound_channels[i].pBlock;
                    switch ( *p )
                    {
                        case 1:
                        {
                            sound_channels[i].loopLeft = 0;
                            sound_channels[i].pos = sound_BE32(sound_channels[i].pBlock + 2);
                            sound_channels[i].avail = sound_BE32(sound_channels[i].pBlock + 6);
                            sound_channels[i].rate = sound_rate;
                            sound_channels[i].pBlock = sound_channels[i].pBlock + sound_channels[i].pBlock[1] + 2;
                            if ( sound_channels[i].pos < (sound_tracks[i].written < sound_tracks[i].total ? 15000 : 0) - sound_tracks[i].ringSize + sound_tracks[i].written )
                            {
                                int len = sound_tracks[i].written - sound_tracks[i].ringSize - sound_channels[i].pos + 15000;
                                if ( len > sound_channels[i].avail )
                                {
                                    sound_tracks[i].state = 0;
                                    sound_tracks[i].id = SOUND_NOTRACK;
                                    left = 0;
                                    continue;
                                }

                                sound_channels[i].pos += len;
                                sound_channels[i].avail -= len;
                            }

                            continue;
                        }

                        case 6:
                        {
                            sound_channels[i].loopLeft = 0;
                            sound_channels[i].pos = sound_BE32(sound_channels[i].pBlock + 2);
                            sound_channels[i].avail = sound_BE32(sound_channels[i].pBlock + 6);
                            sound_channels[i].rate = sound_BE32(sound_channels[i].pBlock + 10);
                            sound_channels[i].pBlock = sound_channels[i].pBlock + sound_channels[i].pBlock[1] + 2;
                            if ( sound_channels[i].pos < (sound_tracks[i].written < sound_tracks[i].total ? 15000 : 0) - sound_tracks[i].ringSize + sound_tracks[i].written )
                            {
                                int len = sound_tracks[i].written - sound_tracks[i].ringSize - sound_channels[i].pos + 15000;
                                if ( len > sound_channels[i].avail )
                                {
                                    sound_tracks[i].state = 0;
                                    sound_tracks[i].id = SOUND_NOTRACK;
                                    left = 0;
                                    continue;
                                }

                                sound_channels[i].pos += len;
                                sound_channels[i].avail -= len;
                            }

                            continue;
                        }

                        case 7:
                        {
                            if ( sound_channels[i].loopLeft == 0 )
                            {
                                sound_channels[i].loopLeft = sound_BE32(p + 6);
                                sound_channels[i].loopStart = 0;
                            }

                            sound_channels[i].pos = sound_BE32(sound_channels[i].pBlock + 2) + sound_channels[i].loopStart;
                            sound_channels[i].avail = (uint32_t)sound_channels[i].loopLeft < (uint32_t)sound_BE32(sound_channels[i].pBlock + 0xE) ? sound_channels[i].loopLeft : sound_BE32(sound_channels[i].pBlock + 0xE);
                            sound_channels[i].rate = sound_BE32(sound_channels[i].pBlock + 0xA);
                            sound_channels[i].loopLeft -= sound_channels[i].avail;
                            sound_channels[i].loopStart += sound_channels[i].avail;
                            if ( sound_channels[i].loopLeft == 0 ) {
                                sound_channels[i].pBlock = sound_channels[i].pBlock + ((int16_t)(sound_channels[i].pBlock[0x12] << 8) | sound_channels[i].pBlock[0x13]);
                            }
                            else {
                                sound_channels[i].pBlock = sound_channels[i].pBlock + sound_channels[i].pBlock[1] + 2;
                            }

                            if ( sound_channels[i].pos < (sound_tracks[i].written < sound_tracks[i].total ? 15000 : 0) - sound_tracks[i].ringSize + sound_tracks[i].written )
                            {
                                int len = sound_tracks[i].written - sound_tracks[i].ringSize - sound_channels[i].pos + 15000;
                                if ( len > sound_channels[i].avail )
                                {
                                    sound_tracks[i].state = 0;
                                    sound_tracks[i].id = SOUND_NOTRACK;
                                    left = 0;
                                    continue;
                                }

                                sound_channels[i].pos += len;
                                sound_channels[i].avail -= len;
                            }

                            continue;
                        }

                        case 2:
                        case 8:
                        case 9:
                        case 10:
                        case 11:
                        {
                            uint8_t value;
                            if ( sound_channels[i].pBlock[4] == 0xFF ) {
                                value = (uint8_t)sound_volume;
                            }
                            else if ( sound_channels[i].pBlock[4] == 0xFE ) {
                                value = sound_tracks[i].volume;
                            }
                            else if ( sound_channels[i].pBlock[4] == 0xFD ) {
                                value = sound_tracks[i].pan;
                            }
                            else {
                                value = sound_vars[sound_channels[i].pBlock[4]];
                            }

                            switch ( *p )
                            {
                                case 2:
                                    if ( value == 0 && sound_channels[i].pBlock[4] != 0 ) {
                                        value = 0;
                                    }
                                    else {
                                        value = 1;
                                    }
                                    break;

                                case 8:
                                    value = value > sound_channels[i].pBlock[5];
                                    break;

                                case 9:
                                    value = value < sound_channels[i].pBlock[5];
                                    break;

                                case 10:
                                    value = value == sound_channels[i].pBlock[5];
                                    break;

                                case 11:
                                    value = value != sound_channels[i].pBlock[5];
                                    break;
                            }

                            if ( value ) {
                                sound_channels[i].pBlock = sound_channels[i].pBlock + ((int16_t)(sound_channels[i].pBlock[2] << 8) | sound_channels[i].pBlock[3]);
                            }
                            else {
                                sound_channels[i].pBlock = sound_channels[i].pBlock + sound_channels[i].pBlock[1] + 2;
                            }

                            continue;
                        }

                        case 3:
                            if ( sound_channels[i].pBlock[2] == 0xFF ) {
                                sound_volume = sound_channels[i].pBlock[3];
                            }
                            else if ( sound_channels[i].pBlock[2] == 0xFE )
                            {
                                sound_tracks[i].volume = sound_channels[i].pBlock[3];
                                volume = sound_tracks[i].volume * sound_volume / 127;
                                if ( (sound_tracks[i].priority & 0xC0) == 0x40 && sound_playEffects ) {
                                    volume = sound_duck * volume >> 8;
                                }
                            }
                            else if ( sound_channels[i].pBlock[2] == 0xFD ) {
                                sound_tracks[i].pan = sound_channels[i].pBlock[3];
                            }
                            else {
                                sound_vars[sound_channels[i].pBlock[2]] = sound_channels[i].pBlock[3];
                            }

                            sound_channels[i].pBlock = sound_channels[i].pBlock + sound_channels[i].pBlock[1] + 2;
                            continue;

                        case 4:
                            if ( sound_channels[i].pBlock[2] == 0xFF ) {
                                sound_volume += sound_channels[i].pBlock[3];
                            }
                            else if ( sound_channels[i].pBlock[2] == 0xFE ) {
                                sound_tracks[i].volume += sound_channels[i].pBlock[3];
                            }
                            else if ( sound_channels[i].pBlock[2] == 0xFD ) {
                                sound_tracks[i].pan += sound_channels[i].pBlock[3];
                            }
                            else {
                                sound_vars[sound_channels[i].pBlock[2]] += sound_channels[i].pBlock[3];
                            }

                            sound_channels[i].pBlock = sound_channels[i].pBlock + sound_channels[i].pBlock[1] + 2;
                            continue;

                        default:
                            sound_tracks[i].state = 0;
                            sound_tracks[i].id = SOUND_NOTRACK;
                            left = 0;
                            continue;
                    }
                }
            }
        }

        sound_tracks[i].position = sound_channels[i].pos;
        sound_channels[i].prevState = sound_tracks[i].state;
    }

    if ( ducking )
    {
        if ( sound_duck > sound_duckFloor )
        {
            sound_duck -= sound_duckSpeed * n >> 13;
            if ( sound_duck < sound_duckFloor ) {
                sound_duck = sound_duckFloor;
            }
        }

        return;
    }

    if ( sound_duck < 0x100 )
    {
        sound_duck += sound_duckSpeed * n >> 15;
        if ( sound_duck > 0x100 ) {
            sound_duck = 0x100;
        }
    }
}

void J3DAPI sound_Advance(double ticks)
{
    sound_clock += ticks;
    if ( sound_syncTicks != 0 )
    {
        int now = sound_pTimer->GetTicks();
        if ( now > sound_syncTicks && (now - sound_syncTicks) / 5 < 1193180 )
        {
            sound_clock -= now - sound_syncTicks;
            sound_syncTicks = 0;
        }
    }

    sound_pTimer->Sync((int)sound_clock);
    sound_clock -= (int)sound_clock;
}

void J3DAPI sound_Copy(void* pDst, const void* pSrc, int n)
{
    mixer_MemCopy(pDst, pSrc, n);
}

void J3DAPI sound_Fill(void* pDst, int value, int n)
{
    mixer_MemFill(pDst, value, n);
}

// An empty `return 0`: slot 4 of Dsnd.c's driver interface. Linker-folded onto 0x004E74A0 in the original; never hook.
int sound_Null(void)
{
    return 0;
}

#if defined(J3D_TEST)
// Added: Test-only reset of the engine's statics to their initial state.
void Engine_TestReset(void);
void Engine_TestReset(void)
{
    sound_numTracks = 4;
    sound_volume = 127;
    sound_rate = 22050;
    sound_duckFloor = 0x40;
    sound_duckSpeed = 0x100;
    sound_duck = 0x100;
    sound_reset = 1;
    sound_pMemory = NULL;
    memset(sound_streamParts, 0, sizeof(sound_streamParts));
    memset(sound_channels, 0, sizeof(sound_channels));
    memset(sound_trackSizes, 0, sizeof(sound_trackSizes));
    sound_pDriver = NULL;
    sound_frames = 0;
    sound_pShare = NULL;
    sound_bufferSize = 0;
    memset(sound_streamIds, 0, sizeof(sound_streamIds));
    sound_playMusic = 0;
    sound_playVoice = 0;
    sound_pTimer = NULL;
    sound_playEffects = 0;
    sound_swapPan = 0;
    memset(sound_tracks, 0, sizeof(sound_tracks));
    sound_pParam = NULL;
    sound_driverRate = 0;
    sound_pDriverParam = NULL;
    memset(sound_streamSeq, 0, sizeof(sound_streamSeq));
    for ( int i = 0; i < 256; i++ ) {
        sound_vars[i] = 0;
    }

    sound_field589C = 0;
    sound_paused = 0;
    sound_started = 0;
    sound_syncTicks = 0;
    sound_clock = 0;
}
#endif

void Engine_InstallHooks(void)
{
    J3D_HOOKFUNC(sound_Register);
    J3D_HOOKFUNC(sound_Unregister);
    J3D_HOOKFUNC(sound_Startup);
    J3D_HOOKFUNC(sound_Shutdown);
    J3D_HOOKFUNC(sound_Open);
    J3D_HOOKFUNC(sound_Close);
    J3D_HOOKFUNC(sound_AddTrack);
    J3D_HOOKFUNC(sound_Play);
    J3D_HOOKFUNC(sound_Stop);
    J3D_HOOKFUNC(sound_Pause);
    J3D_HOOKFUNC(sound_Resume);
    J3D_HOOKFUNC(sound_StopAll);
    J3D_HOOKFUNC(sound_SetVolume);
    J3D_HOOKFUNC(sound_GetVolume);
    J3D_HOOKFUNC(sound_SetPan);
    J3D_HOOKFUNC(sound_GetPan);
    J3D_HOOKFUNC(sound_SetMasterVolume);
    J3D_HOOKFUNC(sound_GetMasterVolume);
    J3D_HOOKFUNC(sound_SetRate);
    J3D_HOOKFUNC(sound_GetRate);
    J3D_HOOKFUNC(sound_GetPosition);
    J3D_HOOKFUNC(sound_Control);
    J3D_HOOKFUNC(sound_Query);
    J3D_HOOKFUNC(sound_GetVar);
    J3D_HOOKFUNC(sound_SetVar);
    J3D_HOOKFUNC(sound_StartVoc);
    J3D_HOOKFUNC(sound_StartStream);
    J3D_HOOKFUNC(sound_SyncTimer);
    J3D_HOOKFUNC(sound_Callback);
    J3D_HOOKFUNC(sound_Advance);
    J3D_HOOKFUNC(sound_Copy);
    J3D_HOOKFUNC(sound_Fill);
}

void Engine_ResetGlobals(void)
{}
