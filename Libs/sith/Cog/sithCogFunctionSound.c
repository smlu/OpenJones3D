#include "sithCogFunctionSound.h"
#include <j3dcore/j3dhook.h>
#include <sith/RTI/symbols.h>

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogexec.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdMath.h>

void J3DAPI sithCogFunctionSound_PlaySoundThing(SithCog* pCog)
{
    SoundPlayFlag playflags = sithCogExec_PopInt(pCog);
    float maxRadius         = sithCogExec_PopFlex(pCog);
    float minRadius         = sithCogExec_PopFlex(pCog);
    float volume            = sithCogExec_PopFlex(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);
    tSoundHandle hSnd       = sithCogExec_PopSound(pCog);

    if ( !hSnd )
    {
        SITHLOG_ERROR("Cog %s: Bad sound handle passed to PlaySoundThing().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Convert distances to IM units
    minRadius = minRadius >= 0.0f ? minRadius * 0.1f : 0.5f;
    maxRadius = maxRadius >= 0.0f ? maxRadius * 0.1f : 2.5f;
    if ( maxRadius <= minRadius )
    {
        maxRadius = minRadius;
    }

    int guid = -1;
    if ( pThing )
    {
        tSoundChannelHandle hSndChannel;
        if ( (playflags & SOUNDPLAY_THING_POS) == 0 )
        {
            playflags |= SOUNDPLAY_ABSOLUTE_POS;
            hSndChannel = sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, volume, minRadius, maxRadius, playflags);
        }
        else
        {
            playflags &= ~SOUNDPLAY_ABSOLUTE_POS;
            hSndChannel = sithSoundMixer_PlaySoundThing(hSnd, pThing, volume, minRadius, maxRadius, playflags);
        }

        if ( hSndChannel != SOUND_INVALIDHANDLE )
        {
            guid = Sound_GetChannelGUID(hSndChannel);
        }

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithDSSThing_PlaySound(pThing, &pThing->pos, hSnd, minRadius, maxRadius, playflags, guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }
        }
    }
    else
    {
        playflags &= ~(SOUNDPLAY_THING_POS | SOUNDPLAY_ABSOLUTE_POS);
        tSoundChannelHandle hSndChannel = sithSoundMixer_PlaySound(hSnd, volume, 0.0f, playflags);
        if ( hSndChannel != SOUND_INVALIDHANDLE )
        {
            guid = Sound_GetChannelGUID(hSndChannel);
        }

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithDSSThing_PlaySound(NULL, NULL, hSnd, volume, 0.0f, playflags, guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }
        }
    }

    sithCogExec_PushInt(pCog, guid);
}

void J3DAPI sithCogFunctionSound_StopSoundThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    tSoundHandle hSnd = sithCogExec_PopSound(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing reference in StopSoundThing.\n", pCog->aName);
        return;
    }

    sithSoundMixer_StopSoundThing(pThing, hSnd);
}

void J3DAPI sithCogFunctionSound_StopAllSoundsThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing reference in StopSoundThing.\n", pCog->aName);
        return;
    }

    sithSoundMixer_StopAllSoundsThing(pThing);
}

void J3DAPI sithCogFunctionSound_PlaySoundPos(SithCog* pCog)
{
    rdVector3 pos;
    SoundPlayFlag playflags = sithCogExec_PopInt(pCog);
    float maxRadius         = sithCogExec_PopFlex(pCog);
    float minRadius         = sithCogExec_PopFlex(pCog);
    float  volume           = sithCogExec_PopFlex(pCog);
    int bPos                = sithCogExec_PopVector(pCog, &pos);
    tSoundHandle hSnd       = sithCogExec_PopSound(pCog);

    if ( !hSnd )
    {
        SITHLOG_ERROR("Cog %s: Bad sound handle passed to PlaySoundPos().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !bPos )
    {
        SITHLOG_ERROR("Cog %s: Bad sound position vector passed to PlaySoundPos().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    minRadius = minRadius >= 0.0f ? minRadius * 0.1f : 0.5f;
    maxRadius = maxRadius >= 0.0f ? maxRadius * 0.1f : 2.5f;
    if ( maxRadius <= minRadius )
    {
        maxRadius = minRadius;
    }

    playflags |= SOUNDPLAY_ABSOLUTE_POS;
    tSoundChannelHandle hChannel = sithSoundMixer_PlaySoundPos(hSnd, &pos, 0, volume, minRadius, maxRadius, playflags);

    int guid = -1;
    if ( hChannel != SOUND_INVALIDHANDLE )
    {
        guid = Sound_GetChannelGUID(hChannel);
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_PlaySound(NULL, &pos, hSnd, minRadius, maxRadius, playflags, guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }

    sithCogExec_PushInt(pCog, guid);
}

void J3DAPI sithCogFunctionSound_PlaySoundLocal(SithCog* pCog)
{
    int bWait               = sithCogExec_PopInt(pCog);
    SoundPlayFlag playflags = sithCogExec_PopInt(pCog);
    float pan               = sithCogExec_PopFlex(pCog);
    float volume            = sithCogExec_PopFlex(pCog);
    tSoundHandle hSnd       = sithCogExec_PopSound(pCog);

    if ( !hSnd )
    {
        SITHLOG_ERROR("Cog %s: Bad sound handle passed to PlaySoundLocal().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pan = STDMATH_CLAMP(pan, -1.0f, 1.0f);

    playflags &= ~(SOUNDPLAY_THING_POS | SOUNDPLAY_ABSOLUTE_POS);
    tSoundChannelHandle hChannel = sithSoundMixer_PlaySound(hSnd, volume, pan, playflags);
    if ( hChannel == SOUND_INVALIDHANDLE )
    {
        SITHLOG_ERROR("Cog %s: Unable to play sound in PlaySoundLocal().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int guid = Sound_GetChannelGUID(hChannel);
    if ( bWait )
    {
        pCog->status = SITHCOG_STATUS_WAITING_SOUND_TO_STOP;
        pCog->statusParams[0] = guid;
    }

    sithCogExec_PushInt(pCog, guid);
}

void J3DAPI sithCogFunctionSound_PlaySoundGlobal(SithCog* pCog)
{
    int bWait               = sithCogExec_PopInt(pCog);
    SoundPlayFlag playflags = sithCogExec_PopInt(pCog);
    float pan               = sithCogExec_PopFlex(pCog);
    float volume            = sithCogExec_PopFlex(pCog);
    tSoundHandle hSnd       = sithCogExec_PopSound(pCog);

    if ( !hSnd )
    {
        SITHLOG_ERROR("Cog %s: Bad sound handle passed to PlaySoundGlobal().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pan = STDMATH_CLAMP(pan, -1.0f, 1.0f);

    playflags &= ~(SOUNDPLAY_THING_POS | SOUNDPLAY_ABSOLUTE_POS);
    tSoundChannelHandle hChannel = sithSoundMixer_PlaySound(hSnd, volume, pan, playflags);

    // TODO: Hmm why setting guid to -1 here when in the next scope -1 is returned if !hChannel?
    int guid = -1;
    if ( hChannel != SOUND_INVALIDHANDLE )
    {
        guid = Sound_GetChannelGUID(hChannel);
    }

    if ( hChannel == SOUND_INVALIDHANDLE )
    {
        SITHLOG_ERROR("Cog %s: Unable to play sound in PlaySoundLocal().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_PlaySound(0, 0, hSnd, volume, pan, playflags, guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }

    if ( bWait )
    {
        pCog->status = SITHCOG_STATUS_WAITING_SOUND_TO_STOP;
        pCog->statusParams[0] = guid;
    }

    sithCogExec_PushInt(pCog, guid);
}

void J3DAPI sithCogFunctionSound_StopSound(SithCog* pCog)
{
    float secFadeTime = sithCogExec_PopFlex(pCog);
    int guid          = sithCogExec_PopInt(pCog);

    tSoundChannelHandle hSndChannel = sithSoundMixer_GetChannelHandle(guid);
    if ( hSndChannel == SOUND_INVALIDHANDLE )
    {
        SITHLOG_ERROR("%s: Couldn't find sound instance in StopSound().\n", pCog->aName);
        return;
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_StopSound(hSndChannel, secFadeTime, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }

    if ( secFadeTime > 0.0f )
    {
        sithSoundMixer_FadeVolume(hSndChannel, 0.0f, secFadeTime);
    }
    else
    {
        sithSoundMixer_StopSound(hSndChannel);
    }
}

void J3DAPI sithCogFunctionSound_LoadSound(SithCog* pCog)
{
    const char* pFilename = sithCogExec_PopString(pCog);

    tSoundHandle hSnd;
    if ( pFilename && (hSnd = sithSound_Load(sithWorld_g_pCurrentWorld, pFilename)) != 0 )
    {
        int sndIdx = Sound_GetSoundIndex(hSnd);
        sithCogExec_PushInt(pCog, sndIdx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSound_PlaySoundClass(SithCog* pCog)
{
    SithSoundClassMode mode = sithCogExec_PopInt(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);

    int guid = -1;
    if ( pThing && pThing->pSoundClass )
    {
        tSoundChannelHandle hChannel = sithSoundClass_PlayModeRandom(pThing, mode);
        if ( hChannel != SOUND_INVALIDHANDLE )
        {
            guid = Sound_GetChannelGUID(hChannel);
        }

        if ( hChannel != SOUND_INVALIDHANDLE && (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_PlaySoundMode(pThing, mode, guid, -1.0f);
        }
    }

    sithCogExec_PushInt(pCog, guid);
}

void J3DAPI sithCogFunctionSound_StopSoundClass(SithCog* pCog)
{
    SithSoundClassMode mode = sithCogExec_PopInt(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pSoundClass )
    {
        SITHLOG_ERROR("Invalid THING for call to StopSoundClass()");
    }
    else
    {
        sithSoundClass_StopMode(pThing, mode);
    }
}

void J3DAPI sithCogFunctionSound_PlayVoiceMode(SithCog* pCog)
{
    SithSoundClassMode mode = sithCogExec_PopInt(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);
    sithSoundClass_PlayVoiceModeRandom(pThing, mode);
}

void J3DAPI sithCogFunctionSound_ChangeVolume(SithCog* pCog)
{
    float secFadeTime               = sithCogExec_PopFlex(pCog);
    float volume                    = sithCogExec_PopFlex(pCog);
    int guid                        = sithCogExec_PopInt(pCog);
    tSoundChannelHandle hSndChannel = sithSoundMixer_GetChannelHandle(guid);

    if ( hSndChannel == SOUND_INVALIDHANDLE )
    {
        SITHLOG_ERROR("Cog %s: No sound playing on channel %d in ChangeVolume().\n", pCog->aName, guid);
        return;
    }

    if ( secFadeTime <= 0.0f )
    {
        SITHLOG_ERROR("Cog %s: Bad fade time %f in ChangePitch() (time must be > 0).\n", pCog->aName, secFadeTime);
        return;
    }

    sithSoundMixer_FadeVolume(hSndChannel, volume, secFadeTime);
}

void J3DAPI sithCogFunctionSound_ChangePitch(SithCog* pCog)
{
    float secFadeTime               = sithCogExec_PopFlex(pCog);
    float pitch                     = sithCogExec_PopFlex(pCog);
    int guid                        = sithCogExec_PopInt(pCog);
    tSoundChannelHandle hSndChannel = sithSoundMixer_GetChannelHandle(guid);

    if ( hSndChannel == SOUND_INVALIDHANDLE )
    {
        SITHLOG_ERROR("Cog %s: No sound playing on channel %d in ChangePitch().\n", pCog->aName, guid);
        return;
    }

    if ( secFadeTime <= 0.0f )
    {
        SITHLOG_ERROR("Cog %s: Bad fade time %f  in ChangePitch() (time must be > 0).\n", pCog->aName, secFadeTime);
        return;
    }

    sithSoundMixer_FadePitch(hSndChannel, pitch, secFadeTime);
}

void J3DAPI sithCogFunctionSound_SectorSound(SithCog* pCog)
{
    float volume        = sithCogExec_PopFlex(pCog);
    tSoundHandle hSnd   = sithCogExec_PopSound(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);

    if ( !pSector )
    {
        SITHLOG_ERROR("Bad arguments in SectorSound verb.\n");
        return;
    }

    sithSoundMixer_SetSectorAmbientSound(pSector, hSnd, volume);
}

void J3DAPI sithCogFunctionSound_GetSoundLen(SithCog* pCog)
{
    tSoundHandle hSnd = sithCogExec_PopSound(pCog);
    if ( hSnd )
    {
        float secLen = (float)Sound_GetLengthMsec(hSnd) * 0.001f;
        sithCogExec_PushFlex(pCog, secLen);
    }
    else
    {
        sithCogExec_PushFlex(pCog, 0.0f);
    }
}

void J3DAPI sithCogFunctionSound_WaitForSound(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    int channel = sithCogExec_PopInt(pCog);
    if ( channel )
    {
        pCog->status = SITHCOG_STATUS_WAITING_SOUND_TO_STOP;
        pCog->statusParams[0] = channel;
        sithCogExec_PushInt(pCog, channel);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

int J3DAPI sithCogFunctionSound_RegisterFunctions(SithCogSymbolTable* pTable)
{
    return sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlaySoundThing, "playsoundthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_StopSoundThing, "stopsoundthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlaySoundPos, "playsoundpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlaySoundLocal, "playsoundlocal")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlaySoundGlobal, "playsoundglobal")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_StopSound, "stopsound")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_LoadSound, "loadsound")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlaySoundClass, "playsoundclass")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_StopSoundClass, "stopsoundclass")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlayVoiceMode, "playvoicemode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_ChangeVolume, "changesoundvol")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_ChangePitch, "changesoundpitch")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_SectorSound, "sectorsound")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_GetSoundLen, "getsoundlen")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_WaitForSound, "waitforsound")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSound_StopAllSoundsThing, "stopallsoundsthing");
}

void sithCogFunctionSound_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCogFunctionSound_RegisterFunctions);
    J3D_HOOKFUNC(sithCogFunctionSound_PlaySoundThing);
    J3D_HOOKFUNC(sithCogFunctionSound_StopSoundThing);
    J3D_HOOKFUNC(sithCogFunctionSound_StopAllSoundsThing);
    J3D_HOOKFUNC(sithCogFunctionSound_PlaySoundPos);
    J3D_HOOKFUNC(sithCogFunctionSound_PlaySoundLocal);
    J3D_HOOKFUNC(sithCogFunctionSound_PlaySoundGlobal);
    J3D_HOOKFUNC(sithCogFunctionSound_StopSound);
    J3D_HOOKFUNC(sithCogFunctionSound_LoadSound);
    J3D_HOOKFUNC(sithCogFunctionSound_PlaySoundClass);
    J3D_HOOKFUNC(sithCogFunctionSound_StopSoundClass);
    J3D_HOOKFUNC(sithCogFunctionSound_PlayVoiceMode);
    J3D_HOOKFUNC(sithCogFunctionSound_ChangeVolume);
    J3D_HOOKFUNC(sithCogFunctionSound_ChangePitch);
    J3D_HOOKFUNC(sithCogFunctionSound_SectorSound);
    J3D_HOOKFUNC(sithCogFunctionSound_GetSoundLen);
    J3D_HOOKFUNC(sithCogFunctionSound_WaitForSound);
}

void sithCogFunctionSound_ResetGlobals(void)
{
}