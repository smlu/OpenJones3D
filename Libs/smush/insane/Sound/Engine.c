// Engine.c: the sound engine (`SOUND=insane`) (original 0x004E5470-0x004E74A0).
#include <smush/insane/Sound/Engine.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stdarg.h>
#include <stdbool.h>
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
static const int sound_frameSize[SOUND_FORMAT_COUNT] = { 1, 2, 2, 4, 2, 4 };

// Defaults of sound_Register
#define SOUND_DEFAULT_RATE      22050
#define SOUND_DEFAULT_BUFFER    64000
#define SOUND_DEFAULT_TRACKSIZE 500000 // Ring buffer size of the first track (the others are not created)

#define SOUND_MAXVOLUME  127
#define SOUND_DUCK_NONE  0x100 // Music ducking gain 1.0 in 8.8 fixed point
#define SOUND_DUCK_FLOOR 0x40
#define SOUND_DUCK_SPEED 0x100

// Block scripts and streams
#define SOUND_VOC_HEADERSIZE 0xC   // The block header sound_StartVoc writes in front of the VOC samples
#define SOUND_MIXCHUNK       0x400 // Most bytes fed to the mixer at once
#define SOUND_STREAMLEAD     15000 // Bytes a streamed track keeps unplayed ahead of the write position
#define SOUND_RAMPMAX        16    // Fade-in steps of a starting track
#define SOUND_AGEMAX_STREAM  0xFF
#define SOUND_AGEMAX_VOC     0x100

// Track selection score weights of sound_StartVoc and sound_StartStream
#define SOUND_SCORE_FREE     0x1000
#define SOUND_SCORE_LOWER    0x800
#define SOUND_SCORE_EQUAL    0x400
#define SOUND_SCORE_STARVED  0x200

static int sound_numTracks = 4;
static int sound_volume    = SOUND_MAXVOLUME;
static int sound_rate      = SOUND_DEFAULT_RATE;
static int sound_duckFloor = SOUND_DUCK_FLOOR;
static int sound_duckSpeed = SOUND_DUCK_SPEED;
static int sound_duck      = SOUND_DUCK_NONE;
static int sound_reset     = 1;

static MemoryDebugInterface* sound_pMemory = NULL;
static int sound_streamParts[SOUND_NUMTRACKS];
static SndChannel sound_channels[SOUND_NUMTRACKS];
static int sound_trackSizes[SOUND_NUMTRACKS];
static SndDriver* sound_pDriver = NULL;
static int sound_frames = 0;
static LecShare* sound_pShare = NULL;
static int sound_bufferSize = 0;
static int sound_streamIds[SOUND_NUMTRACKS];
static int sound_playMusic = 0;
static int sound_playVoice = 0;
static TimerInterface* sound_pTimer = NULL;
static int sound_playEffects = 0;
static int sound_swapPan = 0;
static SoundTrack sound_tracks[SOUND_NUMTRACKS];
static void* sound_pParam = NULL;
static int sound_driverRate = 0;
static void* sound_pDriverParam = NULL;
static int sound_streamSeq[SOUND_NUMTRACKS];
static volatile uint8_t sound_vars[256]; // Shared with the timer thread
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
    sound_driverRate = SOUND_DEFAULT_RATE;
    sound_bufferSize = SOUND_DEFAULT_BUFFER;
    sound_swapPan = 0;
    sound_frames = 0;
    sound_pDriverParam = NULL;
    memset(sound_trackSizes, 0, sizeof(sound_trackSizes));
    sound_trackSizes[0] = SOUND_DEFAULT_TRACKSIZE;
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
    sound_volume = SOUND_MAXVOLUME;
    for ( size_t i = 0; i < sizeof(sound_vars); i++ ) {
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
        SoundTrack* pTrack = &sound_tracks[i];
        if ( pTrack->pBuf ) {
            sound_pMemory->Free(pTrack->pBuf, INSANE_FILE, __LINE__);
        }

        pTrack->pBuf = NULL;
        if ( pTrack->pMix ) {
            sound_pMemory->Free(pTrack->pMix, INSANE_FILE, __LINE__);
        }

        pTrack->pMix = NULL;
    }

    sound_numTracks = 0;
    return sound_frames;
}

int J3DAPI sound_AddTrack(int size, int minSize)
{
    SoundTrack* pTrack = &sound_tracks[sound_numTracks];

    pTrack->state    = SOUND_TRACK_FREE;
    pTrack->position = 0;
    pTrack->priority = 0;
    pTrack->id       = SOUND_NOTRACK;
    pTrack->pBuf     = NULL;
    pTrack->pMix     = NULL;
    pTrack->size     = size & ~3;

    pTrack->pMix = sound_pMemory->Alloc(SOUND_MIXSIZE, INSANE_FILE, __LINE__);
    if ( pTrack->pMix == NULL ) {
        return -1;
    }

    pTrack->pBuf = sound_pMemory->Alloc(pTrack->size, INSANE_FILE, __LINE__);
    if ( pTrack->pBuf == NULL )
    {
        // Shrink the ring buffer in steps of 10000 bytes until it fits
        while ( pTrack->size > minSize )
        {
            pTrack->pBuf = sound_pMemory->Alloc(pTrack->size, INSANE_FILE, __LINE__);
            if ( pTrack->pBuf != NULL ) {
                break;
            }

            pTrack->size -= 10000;
        }

        pTrack->size &= ~3;

        bool bFailed;
        if ( pTrack->size <= minSize ) {
            bFailed = true;
        }
        else
        {
            if ( pTrack->pBuf ) {
                sound_pMemory->Free(pTrack->pBuf, INSANE_FILE, __LINE__);
            }

            pTrack->pBuf = NULL;
            pTrack->size -= minSize;
            pTrack->pBuf = sound_pMemory->Alloc(pTrack->size, INSANE_FILE, __LINE__);
            bFailed      = pTrack->pBuf == NULL;
        }

        if ( bFailed )
        {
            if ( pTrack->pMix ) {
                sound_pMemory->Free(pTrack->pMix, INSANE_FILE, __LINE__);
            }

            pTrack->pMix = NULL;
            return -2;
        }
    }

    framestream_CopyB(pTrack->pBuf, 0x7F, pTrack->size);
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
            if ( pData[8] == 0
                && pData[9] == 0
                && pData[0xC] == 0
                && pData[0xD] == 0
                && pData[0x10] == 0
                && pData[0x11] == 0 )
            {
                sound_StartStream(pData + 0x14, sound_BE32(pData + 4) - 0xC, id, volume, pan, priority, sound_BE16(pData + 0xA), sound_BE16(pData + 0xE),
                                  sound_BE16(pData + 0x12), 1);
            }
            else
            {
                int panOffset = (int8_t)pData[0x11];
                sound_StartStream(pData + 0x12, sound_BE32(pData + 4) - 0xA, id, pData[0x10] * volume >> 7,
                                  panOffset < -127 || panOffset > 127 ? 0x80 : panOffset + pan,
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
            sound_tracks[i].state = SOUND_TRACK_FREE;
            sound_tracks[i].id    = SOUND_NOTRACK;
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
        sound_tracks[i].state = SOUND_TRACK_FREE;
        sound_tracks[i].id    = SOUND_NOTRACK;
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
            if ( sound_tracks[i].state != SOUND_TRACK_FREE ) {
                return sound_tracks[i].position + 1;
            }
        }

        return 0;
    }

    if ( id < 0 && -1 - id < sound_numTracks )
    {
        if ( sound_tracks[-1 - id].state == SOUND_TRACK_FREE ) {
            return 0;
        }

        return sound_tracks[-1 - id].position + 1;
    }

    for ( int i = 0; i < sound_numTracks; i++ )
    {
        if ( sound_tracks[i].id == id )
        {
            if ( sound_tracks[i].state == SOUND_TRACK_FREE ) {
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
        case SOUND_TAG_VAR:
            sound_SetVar((uint8_t)a3, (uint8_t)a4);
            return 0;

        case SOUND_TAG_DUCK:
            if ( a3 ) {
                sound_duckFloor = a3;
            }

            if ( a4 ) {
                sound_duckSpeed = a4;
            }

            return 0;

        case SOUND_TAG_MUSIC:
            sound_playMusic = a3;
            return 0;

        case SOUND_TAG_EFFECTS:
            sound_playEffects = a3;
            return 0;

        case SOUND_TAG_VOICE:
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
        case SOUND_TAG_VAR:
            if ( pA ) {
                *pA = sound_GetVar((uint8_t)(uintptr_t)pB);
            }

            return 0;

        case SOUND_TAG_DUCK:
            if ( pA ) {
                *pA = sound_duckFloor;
            }

            if ( pB ) {
                *pB = sound_duckSpeed;
            }

            return 0;

        case SOUND_TAG_MUSIC:
            if ( pA ) {
                *pA = sound_playMusic;
            }

            return 0;

        case SOUND_TAG_EFFECTS:
            if ( pA ) {
                *pA = sound_playEffects;
            }

            return 0;

        case SOUND_TAG_VOICE:
            if ( pA ) {
                *pA = sound_playVoice;
            }

            return 0;
    }

    return -1;
}

uint8_t J3DAPI sound_GetVar(uint8_t index)
{
    if ( index == SOUND_VAR_VOLUME ) {
        return (uint8_t)sound_volume;
    }

    return sound_vars[index];
}

void J3DAPI sound_SetVar(uint8_t index, uint8_t value)
{
    if ( index == SOUND_VAR_VOLUME ) {
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

    int score[SOUND_NUMTRACKS];
    for ( int i = sound_numTracks - 1; i >= 0; i-- )
    {
        score[i] = 0;
        if ( sound_tracks[i].size == bestSize
            && (sound_tracks[i].state == SOUND_TRACK_FREE || sound_tracks[i].state == SOUND_TRACK_STARVED || sound_tracks[i].priority <= priority) )
        {
            if ( sound_tracks[i].state == SOUND_TRACK_FREE ) {
                score[i] += SOUND_SCORE_FREE;
            }

            if ( sound_tracks[i].priority < priority ) {
                score[i] += SOUND_SCORE_LOWER;
            }

            if ( sound_tracks[i].priority == priority ) {
                score[i] += SOUND_SCORE_EQUAL;
            }

            if ( sound_tracks[i].state == SOUND_TRACK_STARVED ) {
                score[i] += SOUND_SCORE_STARVED;
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
            best     = i;
            bestDiff = score[i];
        }
    }

    if ( best < 0 ) {
        return;
    }

    for ( int i = sound_numTracks - 1; i >= 0; i-- )
    {
        if ( sound_tracks[i].age < SOUND_AGEMAX_VOC ) {
            sound_tracks[i].age++;
        }
    }

    SoundTrack* pTrack = &sound_tracks[best];
    pTrack->age   = 0;
    pTrack->state = SOUND_TRACK_FREE;

    // Block header of a single sample block: type 1, length 8, position 0, sample count len, 2 unused bytes
    uint8_t* pBuf = pTrack->pBuf;
    pBuf[0] = SOUND_BLOCK_SAMPLES;
    pBuf[1] = 8;
    memset(pBuf + 2, 0, 4);
    pBuf[6]   = (uint8_t)(len >> 24);
    pBuf[7]   = (uint8_t)(len >> 16);
    pBuf[8]   = (uint8_t)(len >> 8);
    pBuf[9]   = (uint8_t)len;
    pBuf[0xA] = 0;
    pBuf[0xB] = 0;

    framestream_Copy(pBuf + SOUND_VOC_HEADERSIZE, pData, len);

    pTrack->pBlock   = pBuf;
    pTrack->pData    = pBuf + SOUND_VOC_HEADERSIZE;
    pTrack->ringSize = pTrack->size - SOUND_VOC_HEADERSIZE;
    pTrack->written  = len;
    pTrack->id       = id;
    pTrack->volume   = (int8_t)volume;
    pTrack->pan      = (int8_t)pan;
    pTrack->priority = priority;
    pTrack->position = 0;
    pTrack->state    = SOUND_TRACK_STARTING;
}

void J3DAPI sound_StartStream(uint8_t* pData, int len, int id, int volume, int pan, int priority, int stream, int part, int parts, int a10)
{
    if ( part == 0 )
    {
        int total = 0;
        if ( parts != 1 || a10 != 0 ) {
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
        bool bMusic  = (priority & SOUND_PRIORITY_CLASSMASK) == SOUND_CLASS_MUSIC;
        if ( bMusic && parts > 1 ) {
            sound_SyncTimer();
        }

        if ( !bMusic )
        {
            for ( int i = sound_numTracks - 1; i >= 0; i-- )
            {
                if ( SOUND_ABS(sound_tracks[i].size - total) < bestDiff
                    && sound_tracks[i].size > total * 2 / 3
                    && sound_tracks[i].size > (parts > 1 ? len * 2 : len) )
                {
                    bestDiff = SOUND_ABS(sound_tracks[i].size - total);
                    bestSize = sound_tracks[i].size;
                }
            }
        }

        int score[SOUND_NUMTRACKS];
        for ( int i = sound_numTracks - 1; i >= 0; i-- )
        {
            score[i] = 0;
            if ( sound_tracks[i].size == bestSize
                && (sound_tracks[i].state == SOUND_TRACK_FREE || sound_tracks[i].state == SOUND_TRACK_STARVED || sound_tracks[i].priority <= priority) )
            {
                if ( sound_tracks[i].state == SOUND_TRACK_FREE ) {
                    score[i] += SOUND_SCORE_FREE;
                }

                if ( sound_tracks[i].priority < priority ) {
                    score[i] += SOUND_SCORE_LOWER;
                }

                if ( sound_tracks[i].priority == priority ) {
                    score[i] += SOUND_SCORE_EQUAL;
                }

                if ( sound_tracks[i].state == SOUND_TRACK_STARVED ) {
                    score[i] += SOUND_SCORE_STARVED;
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
                best     = i;
                bestDiff = score[i];
            }
        }

        if ( best < 0 ) {
            return;
        }

        for ( int i = sound_numTracks - 1; i >= 0; i-- )
        {
            if ( sound_tracks[i].age < SOUND_AGEMAX_STREAM ) {
                sound_tracks[i].age++;
            }
        }

        SoundTrack* pTrack = &sound_tracks[best];
        pTrack->age   = 0;
        pTrack->state = SOUND_TRACK_FREE;

        sound_streamSeq[best]   = 0;
        sound_streamIds[best]   = stream;
        sound_streamParts[best] = parts;

        int hdrLen = sound_BE32(pData + 0xC);
        total      = sound_BE32(pData + hdrLen + 0x14);
        if ( parts == 1 && a10 == 0 )
        {
            pTrack->pBlock   = pData + 0x10;
            pTrack->ringSize = len - hdrLen - 0x18;
        }
        else
        {
            framestream_Copy(pTrack->pBuf, pData, len < pTrack->size ? len : pTrack->size);
            pTrack->pBlock   = pTrack->pBuf + 0x10;
            pTrack->ringSize = pTrack->size - hdrLen - 0x18;
        }

        pTrack->pData    = pTrack->pBlock + hdrLen + 8;
        pTrack->written  = len - hdrLen - 0x18;
        pTrack->total    = total;
        pTrack->id       = id;
        pTrack->volume   = SOUND_MAXVOLUME;
        pTrack->pan      = 0;
        if ( volume >= 0 && volume < 0x80 ) {
            pTrack->volume = (int8_t)volume;
        }

        if ( pan > -0x80 && pan < 0x80 ) {
            pTrack->pan = (int8_t)pan;
        }

        pTrack->priority = priority;
        pTrack->position = 0;
        pTrack->state    = SOUND_TRACK_STARTING;

        switch ( (uint32_t)sound_BE32(pTrack->pData - 8) )
        {
            case 'S6SZ':
                pTrack->format = SOUND_FORMAT_STEREO16;
                break;

            case 'S6MZ':
                pTrack->format = SOUND_FORMAT_MONO16;
                break;

            case 'SD6M':
                pTrack->format = SOUND_FORMAT_MONO16U;
                break;

            case 'SD2S':
            case 'SD8S':
                pTrack->format = SOUND_FORMAT_STEREO8;
                break;

            case 'SD6S':
                pTrack->format = SOUND_FORMAT_STEREO16U;
                break;

            default:
                pTrack->format = SOUND_FORMAT_MONO8;
                break;
        }

        return;
    }

    // Following part of a stream: find the track that plays it
    int i = sound_numTracks - 1;
    for ( ; ; i-- )
    {
        if ( i < 0 ) {
            return;
        }

        if ( sound_tracks[i].state != SOUND_TRACK_FREE
            && stream == sound_streamIds[i]
            && part == sound_streamSeq[i] + 1
            && parts == sound_streamParts[i] )
        {
            break;
        }
    }

    sound_streamSeq[i]++;

    SoundTrack* pTrack = &sound_tracks[i];

    int pos = pTrack->written % pTrack->ringSize;
    if ( pos + len > pTrack->ringSize )
    {
        framestream_Copy(pTrack->pData + pos, pData, pTrack->ringSize - pos);
        framestream_Copy(pTrack->pData, pData + (pTrack->ringSize - pos), pos - pTrack->ringSize + len);
    }
    else {
        framestream_Copy(pTrack->pData + pos, pData, len);
    }

    if ( volume >= 0 && volume < 0x7F ) {
        pTrack->volume = (int8_t)volume;
    }

    if ( pan > -0x80 && pan < 0x80 ) {
        pTrack->pan = (int8_t)pan;
    }

    pTrack->written += len;
}

void sound_SyncTimer(void)
{
    sound_syncTicks = sound_pTimer->GetTicks();
}

void J3DAPI sound_Callback(int n, SoundFeed pfnFeed)
{
    sound_frames++;
    sound_Advance(n * (double)DSND_TICKRATE / sound_rate);
    if ( sound_paused != 0 ) {
        return;
    }

    if ( sound_reset != 0 )
    {
        sound_reset = 0;
        for ( int i = 0; i < SOUND_NUMTRACKS; i++ )
        {
            sound_channels[i].fadeLeft = 0;
            sound_channels[i].loopLeft = 0;
        }
    }

    bool bDucking = false;
    for ( int i = 0; i < sound_numTracks; i++ )
    {
        SoundTrack* pTrack   = &sound_tracks[i];
        SndChannel* pChannel = &sound_channels[i];

        bool bEnabled;
        if ( ((pTrack->priority & SOUND_PRIORITY_CLASSMASK) == SOUND_CLASS_EFFECT && sound_playEffects)
            || ((pTrack->priority & SOUND_PRIORITY_CLASSMASK) == SOUND_CLASS_MUSIC && sound_playMusic)
            || ((pTrack->priority & SOUND_PRIORITY_CLASSMASK) == SOUND_CLASS_VOICE && sound_playVoice) ) {
            bEnabled = true;
        }
        else {
            bEnabled = false;
        }

        int volume = pTrack->volume * sound_volume / SOUND_MAXVOLUME;
        if ( (pTrack->priority & SOUND_PRIORITY_CLASSMASK) == SOUND_CLASS_MUSIC && sound_playEffects ) {
            volume = sound_duck * volume >> 8;
        }

        if ( pTrack->state == SOUND_TRACK_STARTING && pChannel->prevState == SOUND_TRACK_PLAYING )
        {
            // The track was restarted while playing: fade out the rest of the old one from the fade buffer
            int skip             = pChannel->pos % pChannel->ringSize;
            pChannel->fadeLeft   = SOUND_MIXSIZE;
            pChannel->fadeVolume = pTrack->volume;
            pChannel->fadeFormat = pTrack->format;
            pChannel->fadeRate   = pChannel->rate;
            sound_Fill(pTrack->pMix, 0x80, SOUND_MIXSIZE);

            int len = pChannel->ringSize - skip;
            if ( len > SOUND_MIXSIZE ) {
                len = SOUND_MIXSIZE;
            }

            sound_Copy(pTrack->pMix, pChannel->pData + skip, len);
            pChannel->ramp = 0;
        }
        else if ( pTrack->state == SOUND_TRACK_PLAYING )
        {
            if ( pChannel->pos < pTrack->written - pTrack->ringSize + SOUND_STREAMLEAD && pTrack->written < pTrack->total )
            {
                pChannel->ramp = 0;

                int len = pTrack->written - pChannel->pos - SOUND_STREAMLEAD;
                if ( len > pChannel->avail ) {
                    len = pChannel->avail;
                }

                if ( pChannel->pos + len > pTrack->total - pChannel->ringSize ) {
                    len = pTrack->total - pChannel->ringSize - pChannel->pos;
                }

                if ( len > 0 )
                {
                    pChannel->fadeLeft = SOUND_MIXSIZE;
                    pChannel->fadeRate = pChannel->rate;
                    sound_Copy(pTrack->pMix, pChannel->pData + pChannel->pos % pChannel->ringSize, SOUND_MIXSIZE);
                    pChannel->pos += len;
                    pChannel->avail -= len;
                }
            }
        }

        if ( pChannel->fadeLeft > sound_frameSize[pChannel->fadeFormat] )
        {
            int want = sound_frameSize[pChannel->fadeFormat] * pChannel->fadeRate * n / sound_rate;
            int done = 0;
            int left = pChannel->fadeLeft < want ? pChannel->fadeLeft : want;
            left -= left % 4;
            while ( left >= 4 )
            {
                int chunk      = left > SOUND_MIXCHUNK ? SOUND_MIXCHUNK : left;
                int fadeVolume = pChannel->fadeLeft * pChannel->fadeVolume * sound_volume / (SOUND_MIXSIZE * SOUND_MAXVOLUME);

                int out;
                if ( chunk == want ) {
                    out = n;
                }
                else {
                    out = chunk / sound_frameSize[pChannel->fadeFormat] * sound_rate / pChannel->fadeRate;
                }

                if ( bEnabled )
                {
                    int pan = sound_swapPan ? -pTrack->pan : pTrack->pan;
                    pfnFeed(pTrack->pMix + SOUND_MIXSIZE - pChannel->fadeLeft, done, out, chunk, 1, fadeVolume, pan, pChannel->fadeFormat);
                }

                done += out;
                left -= chunk;
                pChannel->fadeLeft -= chunk;
            }
        }

        if ( pTrack->state == SOUND_TRACK_STARTING )
        {
            if ( pChannel->prevState != SOUND_TRACK_PLAYING ) {
                pChannel->ramp = SOUND_RAMPMAX;
            }

            pChannel->pBlock   = pTrack->pBlock;
            pChannel->pData    = pTrack->pData;
            pChannel->ringSize = pTrack->ringSize;
            pChannel->format   = pTrack->format;
            pChannel->avail    = 0;
            pChannel->loopLeft = 0;
            pTrack->state      = SOUND_TRACK_PLAYING;
        }

        if ( pTrack->state != SOUND_TRACK_FREE )
        {
            int done = 0;
            int left = n;
            while ( left > 0 )
            {
                if ( pChannel->avail > 0 )
                {
                    int offset = pChannel->pos % pChannel->ringSize;
                    int want   = sound_frameSize[pChannel->format] * pChannel->rate * left / sound_rate;
                    int len    = pChannel->avail < want ? pChannel->avail : want;
                    if ( offset + len > pChannel->ringSize ) {
                        len = pChannel->ringSize - offset;
                    }

                    if ( pChannel->pos + len > pTrack->written )
                    {
                        // The stream has not delivered these samples yet: step back and wait
                        if ( pChannel->ramp != 0 ) {
                            pChannel->ramp--;
                        }

                        pTrack->state = SOUND_TRACK_STARVED;
                        if ( len > SOUND_MIXCHUNK ) {
                            len = SOUND_MIXCHUNK;
                        }

                        pChannel->pos -= len;
                        pChannel->avail += len;
                        offset = pChannel->pos % pChannel->ringSize;
                    }
                    else
                    {
                        if ( pChannel->ramp < SOUND_RAMPMAX ) {
                            pChannel->ramp++;
                        }

                        if ( len > SOUND_MIXCHUNK ) {
                            len = SOUND_MIXCHUNK;
                        }

                        pTrack->state = SOUND_TRACK_PLAYING;
                        if ( (pTrack->priority & SOUND_PRIORITY_CLASSMASK) == SOUND_CLASS_EFFECT ) {
                            bDucking = true;
                        }
                    }

                    int out;
                    if ( len == want ) {
                        out = left;
                    }
                    else {
                        out = len / sound_frameSize[pChannel->format] * sound_rate / pChannel->rate;
                    }

                    if ( bEnabled )
                    {
                        int pan = sound_swapPan ? -pTrack->pan : pTrack->pan;
                        pfnFeed(pChannel->pData + offset, done, out, len, 1, volume * pChannel->ramp >> 4, pan, pChannel->format);
                    }

                    pChannel->avail -= len;
                    pChannel->pos += len;
                    left -= out;
                    done += out;
                }

                while ( pChannel->avail <= 0 )
                {
                    if ( left == 0 ) {
                        break;
                    }

                    uint8_t* pBlock = pChannel->pBlock;
                    switch ( *pBlock )
                    {
                        case SOUND_BLOCK_SAMPLES:
                            pChannel->loopLeft = 0;
                            pChannel->pos      = sound_BE32(pBlock + 2);
                            pChannel->avail    = sound_BE32(pBlock + 6);
                            pChannel->rate     = sound_rate;
                            pChannel->pBlock   = pBlock + pBlock[1] + 2;
                            if ( pChannel->pos < (pTrack->written < pTrack->total ? SOUND_STREAMLEAD : 0) - pTrack->ringSize + pTrack->written )
                            {
                                int len = pTrack->written - pTrack->ringSize - pChannel->pos + SOUND_STREAMLEAD;
                                if ( len > pChannel->avail )
                                {
                                    pTrack->state = SOUND_TRACK_FREE;
                                    pTrack->id    = SOUND_NOTRACK;
                                    left          = 0;
                                    continue;
                                }

                                pChannel->pos += len;
                                pChannel->avail -= len;
                            }

                            continue;

                        case SOUND_BLOCK_SAMPLESRATE:
                            pChannel->loopLeft = 0;
                            pChannel->pos      = sound_BE32(pBlock + 2);
                            pChannel->avail    = sound_BE32(pBlock + 6);
                            pChannel->rate     = sound_BE32(pBlock + 10);
                            pChannel->pBlock   = pBlock + pBlock[1] + 2;
                            if ( pChannel->pos < (pTrack->written < pTrack->total ? SOUND_STREAMLEAD : 0) - pTrack->ringSize + pTrack->written )
                            {
                                int len = pTrack->written - pTrack->ringSize - pChannel->pos + SOUND_STREAMLEAD;
                                if ( len > pChannel->avail )
                                {
                                    pTrack->state = SOUND_TRACK_FREE;
                                    pTrack->id    = SOUND_NOTRACK;
                                    left          = 0;
                                    continue;
                                }

                                pChannel->pos += len;
                                pChannel->avail -= len;
                            }

                            continue;

                        case SOUND_BLOCK_LOOP:
                            if ( pChannel->loopLeft == 0 )
                            {
                                pChannel->loopLeft  = sound_BE32(pBlock + 6);
                                pChannel->loopStart = 0;
                            }

                            pChannel->pos   = sound_BE32(pBlock + 2) + pChannel->loopStart;
                            pChannel->avail = (uint32_t)pChannel->loopLeft < (uint32_t)sound_BE32(pBlock + 0xE) ? pChannel->loopLeft : sound_BE32(pBlock + 0xE);
                            pChannel->rate  = sound_BE32(pBlock + 0xA);
                            pChannel->loopLeft -= pChannel->avail;
                            pChannel->loopStart += pChannel->avail;
                            if ( pChannel->loopLeft == 0 ) {
                                pChannel->pBlock = pBlock + ((int16_t)(pBlock[0x12] << 8) | pBlock[0x13]);
                            }
                            else {
                                pChannel->pBlock = pBlock + pBlock[1] + 2;
                            }

                            if ( pChannel->pos < (pTrack->written < pTrack->total ? SOUND_STREAMLEAD : 0) - pTrack->ringSize + pTrack->written )
                            {
                                int len = pTrack->written - pTrack->ringSize - pChannel->pos + SOUND_STREAMLEAD;
                                if ( len > pChannel->avail )
                                {
                                    pTrack->state = SOUND_TRACK_FREE;
                                    pTrack->id    = SOUND_NOTRACK;
                                    left          = 0;
                                    continue;
                                }

                                pChannel->pos += len;
                                pChannel->avail -= len;
                            }

                            continue;

                        case SOUND_BLOCK_JUMP:
                        case SOUND_BLOCK_JUMPGT:
                        case SOUND_BLOCK_JUMPLT:
                        case SOUND_BLOCK_JUMPEQ:
                        case SOUND_BLOCK_JUMPNE:
                        {
                            uint8_t value;
                            if ( pBlock[4] == SOUND_VAR_VOLUME ) {
                                value = (uint8_t)sound_volume;
                            }
                            else if ( pBlock[4] == SOUND_VAR_TRACKVOLUME ) {
                                value = pTrack->volume;
                            }
                            else if ( pBlock[4] == SOUND_VAR_TRACKPAN ) {
                                value = pTrack->pan;
                            }
                            else {
                                value = sound_vars[pBlock[4]];
                            }

                            switch ( *pBlock )
                            {
                                case SOUND_BLOCK_JUMP:
                                    if ( value == 0 && pBlock[4] != 0 ) {
                                        value = 0;
                                    }
                                    else {
                                        value = 1;
                                    }
                                    break;

                                case SOUND_BLOCK_JUMPGT:
                                    value = value > pBlock[5];
                                    break;

                                case SOUND_BLOCK_JUMPLT:
                                    value = value < pBlock[5];
                                    break;

                                case SOUND_BLOCK_JUMPEQ:
                                    value = value == pBlock[5];
                                    break;

                                case SOUND_BLOCK_JUMPNE:
                                    value = value != pBlock[5];
                                    break;
                            }

                            if ( value ) {
                                pChannel->pBlock = pBlock + ((int16_t)(pBlock[2] << 8) | pBlock[3]);
                            }
                            else {
                                pChannel->pBlock = pBlock + pBlock[1] + 2;
                            }

                            continue;
                        }

                        case SOUND_BLOCK_SETVAR:
                            if ( pBlock[2] == SOUND_VAR_VOLUME ) {
                                sound_volume = pBlock[3];
                            }
                            else if ( pBlock[2] == SOUND_VAR_TRACKVOLUME )
                            {
                                pTrack->volume = pBlock[3];
                                volume         = pTrack->volume * sound_volume / SOUND_MAXVOLUME;
                                if ( (pTrack->priority & SOUND_PRIORITY_CLASSMASK) == SOUND_CLASS_MUSIC && sound_playEffects ) {
                                    volume = sound_duck * volume >> 8;
                                }
                            }
                            else if ( pBlock[2] == SOUND_VAR_TRACKPAN ) {
                                pTrack->pan = pBlock[3];
                            }
                            else {
                                sound_vars[pBlock[2]] = pBlock[3];
                            }

                            pChannel->pBlock = pBlock + pBlock[1] + 2;
                            continue;

                        case SOUND_BLOCK_ADDVAR:
                            if ( pBlock[2] == SOUND_VAR_VOLUME ) {
                                sound_volume += pBlock[3];
                            }
                            else if ( pBlock[2] == SOUND_VAR_TRACKVOLUME ) {
                                pTrack->volume += pBlock[3];
                            }
                            else if ( pBlock[2] == SOUND_VAR_TRACKPAN ) {
                                pTrack->pan += pBlock[3];
                            }
                            else {
                                sound_vars[pBlock[2]] += pBlock[3];
                            }

                            pChannel->pBlock = pBlock + pBlock[1] + 2;
                            continue;

                        default:
                            pTrack->state = SOUND_TRACK_FREE;
                            pTrack->id    = SOUND_NOTRACK;
                            left          = 0;
                            continue;
                    }
                }
            }
        }

        pTrack->position    = pChannel->pos;
        pChannel->prevState = pTrack->state;
    }

    if ( bDucking )
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

    if ( sound_duck < SOUND_DUCK_NONE )
    {
        sound_duck += sound_duckSpeed * n >> 15;
        if ( sound_duck > SOUND_DUCK_NONE ) {
            sound_duck = SOUND_DUCK_NONE;
        }
    }
}

void J3DAPI sound_Advance(double ticks)
{
    sound_clock += ticks;
    if ( sound_syncTicks != 0 )
    {
        int now = sound_pTimer->GetTicks();
        if ( now > sound_syncTicks && (now - sound_syncTicks) / 5 < DSND_TICKRATE )
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
    sound_volume    = SOUND_MAXVOLUME;
    sound_rate      = SOUND_DEFAULT_RATE;
    sound_duckFloor = SOUND_DUCK_FLOOR;
    sound_duckSpeed = SOUND_DUCK_SPEED;
    sound_duck      = SOUND_DUCK_NONE;
    sound_reset     = 1;
    sound_pMemory   = NULL;
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
    for ( size_t i = 0; i < sizeof(sound_vars); i++ ) {
        sound_vars[i] = 0;
    }

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
