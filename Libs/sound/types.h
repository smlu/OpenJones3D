#ifndef SOUND_TYPES_H
#define SOUND_TYPES_H
#include <stdint.h>
#include <Mmreg.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

#if defined (J3D_DIRECTX6)
#include <DirectX6/dsound.h>
#elif defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
#include <dsound.h>
#else
#error "Unsuported system sound API. Please define J3D_DIRECTX6 or J3D_DIRECTX9 in your project."
#endif

J3D_EXTERN_C_START

#if defined (J3D_DIRECTX6)
typedef IDirectSoundBuffer tSysSoundBuffer;
#elif defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
typedef IDirectSoundBuffer8 tSysSoundBuffer;
#else
#error "Unsuported system sound API. Please define J3D_DIRECTX6 or J3D_DIRECTX9 in your project."
#endif

typedef enum eSoundOpenFlags
{
    SOUNDOPEN_NO3DSOUND           = 0x02,
    SOUNDOPEN_HWNDISDIRECTSOUND   = 0x04,
    SOUNDOPEN_GLOBALFOCUS         = 0x08,
    SOUNDOPEN_NOSOUNDCOMPRESSION  = 0x10,
    SOUNDOPEN_NOLIPSYNC           = 0x20,
} SoundOpenFlags;

typedef enum eSoundPlayFlag
{
    SOUNDPLAY_LOOP                 = 0x01,
    SOUNDPLAY_REMOVE_AFTER_FADEOUT = 0x02,
    SOUNDPLAY_AMBIENT              = 0x04,
    SOUNDPLAY_USE_DOPPLAR_FX       = 0x08,
    SOUNDPLAY_FADEIN               = 0x10,
    SOUNDPLAY_FADEOUT              = 0x20,
    SOUNDPLAY_ABSOLUTE_POS         = 0x40,
    SOUNDPLAY_THING_POS            = 0x80,
    SOUNDPLAY_HIGH_PRIORITY        = 0x100,
    SOUNDPLAY_HIGHEST_PRIORITY     = 0x200,
    SOUNDPLAY_PLAYONCE             = 0x400,
    SOUNDPLAY_PLAYTHINGONCE        = 0x800,
} SoundPlayFlag;

typedef enum eSoundChannelFlag J3D_ENUM_TYPE(uint32_t)
{
    SOUND_CHANNEL_PLAYING          = 0x01,
        SOUND_CHANNEL_LOOP             = 0x02,
        SOUND_CHANNEL_HWMIXING         = 0x04,
        SOUND_CHANNEL_GLOBALFOCUS      = 0x08,
        SOUND_CHANNEL_VOLUMEFADE       = 0x10,
        SOUND_CHANNEL_PITCHFADE        = 0x20,
        SOUND_CHANNEL_PLAYONCE         = 0x100,
        SOUND_CHANNEL_PLAYTHINGONCE    = 0x200,
        SOUND_CHANNEL_PAUSED           = 0x20000,
        SOUND_CHANNEL_3DSOUND          = 0x40000,
        SOUND_CHANNEL_THING            = 0x80000,
        SOUND_CHANNEL_COMPRESSED       = 0x100000,
        SOUND_CHANNEL_FAR              = 0x200000,
        SOUND_CHANNEL_SWMIXING         = 0x400000,
        SOUND_CHANNEL_UNKNOWN_800000   = 0x800000,
        SOUND_CHANNEL_RESTART          = 0x1000000,
        SOUND_CHANNEL_STARTFAR         = 0x2000000, // this is used in sound module when looping sound is far away from initial camera position, so the sound doesn't play
        SOUND_CHANNEL_USE3DCAPS        = 0x4000000,
        SOUND_CHANNEL_UNKNOWN_8000000  = 0x8000000, // Disables distance check for 3D sound
        SOUND_CHANNEL_DISABLEHWMIXING  = 0x10000000,
} tSoundChannelFlag;

typedef enum eSoundEnvFlags
{
    SOUND_ENV_UNDERWATER = 0x01,
} SoundEnvFlags;

typedef uint32_t tSoundHandleType;
typedef tSoundHandleType tSoundChannelHandle;
typedef tSoundHandleType tSoundHandle;

typedef struct sSoundThingInfo SoundThingInfo;
typedef struct sSoundSpatialInfo SoundSpatialInfo;

typedef int (J3DAPI* SoundGetThingInfoCallback)(int thingID, SoundThingInfo* pInfo);
typedef void (J3DAPI* SoundCalcListenerSoundMixFunc)(const SoundSpatialInfo*, float* volume, float* pan, float* pitch);


typedef struct sSoundInfo
{
    tSoundHandle hSnd;
    uint32_t bankNum;
    uint32_t filePathOffset;
    uint32_t nameOffset;
    uint32_t dataOffset;
    uint8_t* pLipSyncData;
    uint32_t sampleRate;
    uint32_t sempleBitSize;
    uint32_t numChannels;
    uint32_t dataSize;
    uint32_t bCompressed;
    uint32_t idx;
} SoundInfo;
static_assert(sizeof(SoundInfo) == 48, "sizeof(SoundInfo) == 48"); // Note the size must stay 48 bytes due to bank serialization to CND file

typedef struct sSoundChannel
{
    tSoundChannelHandle handle;
    int priority;
    tSoundHandle hSnd;
    int thingId;
    tSysSoundBuffer* pDSoundBuffer;
    rdVector3 playPos;
    float volume;
    float pitch;
    float sampleRate;
    tSoundChannelFlag flags;
    SoundEnvFlags envflags;
    int guid;
    float minRadius;
    float maxRadius;
} tSoundChannel;

typedef struct sSoundFade
{
    tSoundChannelHandle hChannel;
    int bPitch;
    float startValue;
    float endValue;
    uint32_t msecStartTime;
    uint32_t msecEndTime;
} SoundFade;
static_assert(sizeof(SoundFade) == 24, "sizeof(SoundFade) == 24");

struct sSoundThingInfo
{
    rdVector3 pos;
    rdVector3 velocity;
    SoundEnvFlags envflags;
};

typedef struct sAudioCompressorState
{
    uint8_t unknown0;
    uint8_t unknown1;
    int16_t unknown2;
    int16_t unknown3;
    int16_t unknown4;
} tAudioCompressorState;

typedef struct sAudioCompressedData
{
    uint32_t uncompressedSize; // Note, don't change int type as it must be 4 bytes due to serialization
    uint8_t compressedData[1];
} tAudioCompressedData;
static_assert(sizeof(tAudioCompressedData) == 8, "sizeof(tAudioCompressedData) == 8"); // 8 - due to alignment

struct sSoundSpatialInfo
{
    rdVector3 pos;
    int unknown1;
    int unknown2;
    int unknown3;
    float minRadius;
    float maxRadius;
    float volume;
    float pitch;
    SoundEnvFlags flags;
};

typedef struct sSoundInstanceInfo
{
    tSoundChannelHandle handle;
    const char* pName;
    int thingId;
    tSoundChannelFlag flags;
} SoundInstanceInfo;


J3D_EXTERN_C_END
#endif //SOUND_TYPES_H
