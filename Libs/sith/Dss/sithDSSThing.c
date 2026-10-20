#include "sithDSSThing.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdPolyline.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithPathMove.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithItem.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#include <string.h>

void sithDSSThing_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(sithDSSThing_Pos);
    J3D_HOOKFUNC(sithDSSThing_ProcessPos);
    J3D_HOOKFUNC(sithDSSThing_MovePos);
    J3D_HOOKFUNC(sithDSSThing_ProcessMovePos);
    J3D_HOOKFUNC(sithDSSThing_UpdateState);
    J3D_HOOKFUNC(sithDSSThing_ProcessStateUpdate);
    J3D_HOOKFUNC(sithDSSThing_PlaySound);
    J3D_HOOKFUNC(sithDSSThing_ProcessPlaySound);
    J3D_HOOKFUNC(sithDSSThing_PlaySoundMode);
    J3D_HOOKFUNC(sithDSSThing_ProcessPlaySoundMode);
    J3D_HOOKFUNC(sithDSSThing_PlayKey);
    J3D_HOOKFUNC(sithDSSThing_ProcessPlayKey);
    J3D_HOOKFUNC(sithDSSThing_PlayKeyMode);
    J3D_HOOKFUNC(sithDSSThing_ProcessPlayKeyMode);
    J3D_HOOKFUNC(sithDSSThing_SetModel);
    J3D_HOOKFUNC(sithDSSThing_ProcessSetModel);
    J3D_HOOKFUNC(sithDSSThing_StopKey);
    J3D_HOOKFUNC(sithDSSThing_ProcessStopKey);
    J3D_HOOKFUNC(sithDSSThing_StopSound);
    J3D_HOOKFUNC(sithDSSThing_ProcessStopSound);
    J3D_HOOKFUNC(sithDSSThing_Fire);
    J3D_HOOKFUNC(sithDSSThing_ProcessFire);
    J3D_HOOKFUNC(sithDSSThing_Death);
    J3D_HOOKFUNC(sithDSSThing_ProcessDeath);
    J3D_HOOKFUNC(sithDSSThing_DamageThing);
    J3D_HOOKFUNC(sithDSSThing_ProcessDamage);
    J3D_HOOKFUNC(sithDSSThing_ThingFullDescription);
    J3D_HOOKFUNC(sithDSSThing_ProcessThingFullDescription);
    J3D_HOOKFUNC(sithDSSThing_PathMove);
    J3D_HOOKFUNC(sithDSSThing_ProcessPathMove);
    J3D_HOOKFUNC(sithDSSThing_Attachment);
    J3D_HOOKFUNC(sithDSSThing_ProcessAttachment);
    J3D_HOOKFUNC(sithDSSThing_Take);
    J3D_HOOKFUNC(sithDSSThing_ProcessTake);
    J3D_HOOKFUNC(sithDSSThing_CreateThing);
    J3D_HOOKFUNC(sithDSSThing_ProcessCreateThing);
    J3D_HOOKFUNC(sithDSSThing_DestroyThing);
    J3D_HOOKFUNC(sithDSSThing_ProcessDestroyThing);
    J3D_HOOKFUNC(sithDSSThing_MoveToPos);
}

void sithDSSThing_ResetGlobals(void)
{}

int J3DAPI sithDSSThing_Pos(const SithThing* pThing, DPID toID, unsigned int dpFlags)
{
    if ( !pThing || !pThing->type || !pThing->pInSector )
    {
        SITHLOG_STATUS("Asked to send state for invalid thing pointer.\n");
        return 0;
    }

    SITHDSS_STARTOUT(SITHDSS_POS);
    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHUINT16(pThing->attach.flags);

    SITH_ASSERTREL(pThing->pInSector);
    SITHDSS_PUSHINT16((int16_t)sithSector_GetSectorIndex(pThing->pInSector));
    SITHDSS_PUSHVEC3(&pThing->pos);

    rdVector3 curpyr;
    rdMatrix_ExtractAngles34(&pThing->orient, &curpyr);
    SITHDSS_PUSHVEC3(&curpyr);

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        SITHDSS_PUSHUINT32(pThing->moveInfo.physics.flags);
        SITHDSS_PUSHVEC3(&pThing->moveInfo.physics.velocity);
        SITHDSS_PUSHVEC3(&pThing->moveInfo.physics.angularVelocity);
    }

    if ( pThing->type == SITH_THING_PLAYER )
    {
        SITHDSS_PUSHFLOAT(pThing->thingInfo.actorInfo.headPYR.x);
    }

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, toID, SITHMESSAGE_STREAM_ALL, dpFlags);
}


int J3DAPI sithDSSThing_ProcessPos(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);
    if ( !sithWorld_g_pCurrentWorld )
    {
        return 0;
    }

    int index = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(index);
    if ( !pThing )
    {
        SITHLOG_ERROR("Bad thing guid %d in DSS::Pos.\n", index);
        return 0;
    }

    if ( !pThing->type || !pThing->pInSector )
    {
        SITHLOG_ERROR("Bad thing guid %d in DSS::Pos.\n", index);
        return 0;
    }

    SithAttachFlag attachFlags = SITHDSS_POPUINT16();
    if ( !attachFlags && pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
    }

    index = SITHDSS_POPUINT16();
    SithSector* pSector = sithSector_GetSectorEx(sithWorld_g_pCurrentWorld, index);
    if ( !pSector )
    {
        SITHLOG_ERROR("Bad sector index %d in DSS::Pos.\n", index);
        return 0;
    }

    rdVector3 loc;
    rdVector3 rotation;
    SITHDSS_POPVEC3(&loc);
    SITHDSS_POPVEC3(&rotation);
    rdMatrix_BuildRotate34(&pThing->orient, &rotation);

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        pThing->moveInfo.physics.flags = SITHDSS_POPUINT32();
        SITHDSS_POPVEC3(&pThing->moveInfo.physics.velocity);
        SITHDSS_POPVEC3(&pThing->moveInfo.physics.angularVelocity);
        sithDSSThing_MoveToPos(pThing, &loc, pSector);
    }
    else
    {
        rdVector_Copy3(&pThing->pos, &loc);
        sithThing_SetSector(pThing, pSector, 0);
    }

    if ( pThing->type == SITH_THING_PLAYER )
    {
        memset(&rotation, 0, sizeof(rotation));
        rotation.x = SITHDSS_POPFLOAT();
        sithActor_SetHeadPYR(pThing, &rotation);
    }

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_MovePos(const SithThing* pThing, DPID idTo, unsigned int sendFlags)
{
    if ( !pThing || !pThing->type || !pThing->pInSector )
    {
        SITHLOG_STATUS("Asked to send state for invalid thing pointer.\n");
        return 0;
    }

    SITHDSS_STARTOUT(SITHDSS_MOVEPOS);
    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT16((int16_t)sithSector_GetSectorIndex(pThing->pInSector));
    SITHDSS_PUSHVEC3(&pThing->pos);

    rdVector3 curpyr;
    rdMatrix_ExtractAngles34(&pThing->orient, &curpyr);
    SITHDSS_PUSHVEC3(&curpyr);

    SITHDSS_PUSHINT32(pThing->moveStatus);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, SITHMESSAGE_STREAM_ALL, sendFlags);
}


int J3DAPI sithDSSThing_ProcessMovePos(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int guid = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
        SITHLOG_ERROR("Bad thing guid %d in DSS::Pos.\n", guid);
        return 0;
    }

    if ( !pThing->type || !pThing->pInSector )
    {
        SITHLOG_ERROR("Bad thing guid %d in DSS::Pos.\n", guid);
        return 0;
    }

    unsigned int sectorIdx = SITHDSS_POPUINT16();
    SithSector* pSector = sithSector_GetSectorEx(sithWorld_g_pCurrentWorld, sectorIdx);
    if ( !pSector )
    {
        SITHLOG_ERROR("Bad sector index %d in DSS::Pos.\n", sectorIdx);
        return 0;
    }

    rdVector3 pos;
    SITHDSS_POPVEC3(&pos);
    rdVector_Copy3(&pThing->pos, &pos);
    sithThing_SetSector(pThing, pSector, 0);

    rdVector3 pyr;
    SITHDSS_POPVEC3(&pyr);
    rdMatrix_BuildRotate34(&pThing->orient, &pyr);

    switch ( SITHDSS_POPINT32() )
    {
        case SITHPLAYERMOVE_PUSHING:
            break;

        case SITHPLAYERMOVE_PULLINGUP:
            sithPlayerActions_ClimbPullUp(pThing);
            break;

        case SITHPLAYERMOVE_PULLINGUP_1M:
            sithPlayerActions_ClimbOn1m(pThing);
            break;

        case SITHPLAYERMOVE_UNKNOWN_60:
            sithPlayerActions_HopLeft(pThing); // Note: the original's sithPlayerActions_JumpLeft
            break;

        case SITHPLAYERMOVE_STRAFING_RIGHT:
            sithPlayerActions_StrafeRight(pThing);
            break;

        case SITHPLAYERMOVE_STRAFING_LEFT:
            sithPlayerActions_StrafeLeft(pThing);
            break;

        case SITHPLAYERMOVE_JUMPBACK:
            sithPlayerActions_HopBack(pThing);
            break;

        case SITHPLAYERMOVE_JUMP_READY:
            sithPlayerActions_JumpStart(pThing);
            break;
    }

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_UpdateState(const SithThing* pThing, DPID toID, SithMessageStream outstream)
{
    if ( !pThing || !pThing->type || !pThing->pInSector || !sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing) )
    {
        SITHLOG_STATUS("Asked to send state for invalid thing pointer.\n");
        return 0;
    }

    SITHDSS_STARTOUT(SITHDSS_THINGSTATE);
    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(pThing->unknownFlags);
    SITHDSS_PUSHUINT32(pThing->msecLifeLeft);

    SITH_ASSERTREL(pThing->pInSector);
    SITHDSS_PUSHINT16((int16_t)sithSector_GetSectorIndex(pThing->pInSector));
    SITHDSS_PUSHINT16(pThing->collide.type);
    SITHDSS_PUSHVEC3(&pThing->pos);
    SITHDSS_PUSHUINT32(pThing->flags);

    if ( pThing->pPuppetClass )
    {
        SITHDSS_PUSHINT16((int16_t)pThing->pPuppetState->armedMode);
        SITHDSS_PUSHINT16(pThing->pPuppetState->moveMode);
    }

    SITHDSS_PUSHVEC3((const rdVector3*)&pThing->light.color);

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_CORPSE:
        case SITH_THING_PLAYER:
            SITHDSS_PUSHUINT32(pThing->thingInfo.actorInfo.flags);
            break;

        case SITH_THING_WEAPON:
            SITHDSS_PUSHUINT32(pThing->thingInfo.weaponInfo.flags);
            break;

        case SITH_THING_ITEM:
            SITHDSS_PUSHUINT32(pThing->thingInfo.itemInfo.flags);
            if ( (pThing->thingInfo.itemInfo.flags & SITH_ITEM_BACKPACK) != 0 )
            {
                // Note: numItems is serialized as int16_t
                int16_t numItems = (int16_t)pThing->thingInfo.itemInfo.numItems;
                SITHDSS_PUSHINT16(numItems);
                for ( int16_t i = 0; i < numItems; ++i )
                {
                    SITHDSS_PUSHINT16((int16_t)pThing->thingInfo.itemInfo.aBackpackItems[i].typeId);
                    SITHDSS_PUSHFLOAT(pThing->thingInfo.itemInfo.aBackpackItems[i].amount);
                }
            }
            break;

        default:
            break;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        SITHDSS_PUSHUINT32(pThing->moveInfo.physics.flags);
    }

    if ( !pThing->pSwapList )
    {
        SITHDSS_PUSHINT16(-1);
    }
    else
    {
        int16_t numUsedSwapEntries = 0;
        for ( const SithThingSwapEntry* pEntry = pThing->pSwapList; pEntry; pEntry = pEntry->pNextEntry )
        {
            ++numUsedSwapEntries;
        }

        SITHDSS_PUSHINT16(numUsedSwapEntries);
        for ( const SithThingSwapEntry* pEntry = pThing->pSwapList; pEntry; pEntry = pEntry->pNextEntry )
        {
            SITHDSS_PUSHINT32(pEntry->entryNum);
            SITHDSS_PUSHINT32(pEntry->meshNum);

            char aModelName[64] = { 0 };
            STD_STRNCPY(aModelName, pEntry->pSrcModel->aName, STD_ARRAYLEN(aModelName) - 1);
            SITHDSS_PUSHSTRING(aModelName, STD_ARRAYLEN(aModelName));

            SITHDSS_PUSHINT32(pEntry->srcMeshNum);
        }
    }

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, toID, outstream, DPSEND_GUARANTEED);
}


int J3DAPI sithDSSThing_ProcessStateUpdate(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int idx = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(idx);
    if ( !pThing )
    {
        SITHLOG_ERROR("Thing guid %x not found.\n", idx);
        return 0;
    }

    if ( !pThing->type || !pThing->pInSector )
    {
        SITHLOG_STATUS("Warning - got state update for non-existant thing index %d.\n", idx);
        return 0;
    }

    pThing->unknownFlags = SITHDSS_POPINT32();
    pThing->msecLifeLeft = SITHDSS_POPUINT32();

    idx = SITHDSS_POPUINT16();
    SithSector* pSector = sithSector_GetSectorEx(sithWorld_g_pCurrentWorld, idx);
    if ( !pSector )
    {
        SITHLOG_ERROR("Bad sector index %d in DSS::Pos.\n", idx);
        return 0;
    }

    pThing->collide.type = SITHDSS_POPUINT16();
    SITHDSS_POPVEC3(&pThing->pos);
    sithThing_SetSector(pThing, pSector, 0);

    SithThingFlag flags = SITHDSS_POPUINT32();
    if ( (pThing->flags & SITH_TF_REMOTE) != 0 )
    {
        flags |= SITH_TF_REMOTE;
    }
    else
    {
        flags &= ~SITH_TF_REMOTE;
    }

    pThing->flags = flags;

    if ( pThing->pPuppetClass )
    {
        unsigned int armedMode = SITHDSS_POPUINT16();
        SithPuppetMoveMode moveMode = SITHDSS_POPUINT16();
        sithPuppet_SetArmedMode(pThing, armedMode);
        sithPuppet_SetMoveMode(pThing, moveMode);
    }

    SITHDSS_POPVEC3((rdVector3*)&pThing->light.color);
    pThing->light.color.alpha = 0.0f;

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_CORPSE:
        case SITH_THING_PLAYER:
            pThing->thingInfo.actorInfo.flags = SITHDSS_POPUINT32();
            break;

        case SITH_THING_WEAPON:
            pThing->thingInfo.weaponInfo.flags = SITHDSS_POPUINT32();
            break;

        case SITH_THING_ITEM:
            pThing->thingInfo.itemInfo.flags = SITHDSS_POPUINT32();
            if ( (pThing->thingInfo.itemInfo.flags & SITH_ITEM_BACKPACK) != 0 )
            {
                // Note: The original stores numItems as int16_t (writes only the low 2 bytes of the field)
                *(int16_t*)&pThing->thingInfo.itemInfo.numItems = SITHDSS_POPINT16();
                int16_t numItems = *(int16_t*)&pThing->thingInfo.itemInfo.numItems;

                // TODO: [BUG] numItems is not validated against the size of aBackpackItems (32) and can overflow it with a malformed message
                for ( int16_t i = 0; i < numItems; ++i )
                {
                    pThing->thingInfo.itemInfo.aBackpackItems[i].typeId = SITHDSS_POPINT16();
                    pThing->thingInfo.itemInfo.aBackpackItems[i].amount = SITHDSS_POPFLOAT();
                }
            }
            break;

        default:
            break;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        pThing->moveInfo.physics.flags = SITHDSS_POPUINT32();
    }

    if ( pThing->pSwapList )
    {
        sithThing_ResetSwapList(pThing);
    }

    int16_t numEntries = SITHDSS_POPINT16();
    if ( numEntries != -1 )
    {
        for ( int16_t i = 0; i < numEntries; ++i )
        {
            SITHDSS_POPINT32(); // model num, unused
            int meshNum = SITHDSS_POPINT32();

            char aModelName[64];
            SITHDSS_POPSTRING(aModelName, STD_ARRAYLEN(aModelName));
            aModelName[STD_ARRAYLEN(aModelName) - 1] = 0;

            int srcMeshNum = SITHDSS_POPINT32();
            rdModel3* pModel = sithModel_Load(aModelName, 1);
            if ( pModel )
            {
                sithThing_AddSwapEntry(pThing, meshNum, pModel, srcMeshNum);
            }
        }
    }

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_PlaySound(const SithThing* pThing, const rdVector3* pPos, tSoundHandle hSnd, float volume, float pan, SoundPlayFlag playflags, int guid, DPID idTo, SithMessageStream outstream)
{
    SITH_ASSERTREL(hSnd);

    SITHDSS_STARTOUT(SITHDSS_PLAYSOUND);
    SITHDSS_PUSHUINT32(playflags);
    SITHDSS_PUSHFLOAT(volume);
    SITHDSS_PUSHFLOAT(pan);
    SITHDSS_PUSHUINT16((uint16_t)Sound_GetSoundIndex(hSnd));

    if ( (playflags & SOUNDPLAY_THING_POS) != 0 )
    {
        SITH_ASSERTREL(pThing);
        SITHDSS_PUSHINT32(pThing->guid);
    }
    else if ( (playflags & SOUNDPLAY_ABSOLUTE_POS) != 0 )
    {
        SITHDSS_PUSHVEC3(pPos);
    }

    SITHDSS_PUSHINT32(guid);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, 0);
}


int J3DAPI sithDSSThing_ProcessPlaySound(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    SoundPlayFlag playflags = SITHDSS_POPUINT32();
    float minRadius = SITHDSS_POPFLOAT();
    float maxRadius = SITHDSS_POPFLOAT();
    int idx = SITHDSS_POPUINT16();

    tSoundHandle hSnd = Sound_GetSoundHandle(idx);
    if ( !hSnd )
    {
        SITHLOG_ERROR("Sound index %d not valid in DSS::PlaySound.\n", idx);
        return 0;
    }

    if ( (playflags & SOUNDPLAY_THING_POS) != 0 )
    {
        idx = SITHDSS_POPINT32();
        SithThing* pThing = sithThing_GetGuidThing(idx);
        if ( !pThing )
        {
            SITHLOG_ERROR("Thing guid %d not found in DSS::PlaySound (sound=%s)\n", idx, Sound_GetSoundFilename(hSnd));
            return 0;
        }

        sithSoundMixer_PlaySoundThing(hSnd, pThing, 1.0f, minRadius, maxRadius, playflags);
    }
    else if ( (playflags & SOUNDPLAY_ABSOLUTE_POS) != 0 )
    {
        rdVector3 pos;
        SITHDSS_POPVEC3(&pos);
        sithSoundMixer_PlaySoundPos(hSnd, &pos, NULL, 1.0f, minRadius, maxRadius, playflags);
    }
    else
    {
        sithSoundMixer_PlaySound(hSnd, minRadius, maxRadius, playflags);
    }

    SITHDSS_POPINT32(); // sound guid, unused
    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_PlaySoundMode(const SithThing* pThing, int16_t mode, int channel, float a4)
{
    SITH_ASSERTREL(pThing);

    SITHDSS_STARTOUT(SITHDSS_PLAYSOUNDMODE);
    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(channel);
    SITHDSS_PUSHFLOAT(a4);
    SITHDSS_PUSHINT16(mode);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, (DPID)-1, SITHMESSAGE_STREAM_ALL, 0);
}


int J3DAPI sithDSSThing_ProcessPlaySoundMode(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int guid = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
        SITHLOG_ERROR("Thing guid %d not found in DSS::PlaySoundMode\n", guid);
        return 0;
    }

    SITHDSS_POPINT32(); // channel, unused
    float selectionRatio = SITHDSS_POPFLOAT();
    SithSoundClassMode mode = SITHDSS_POPUINT16();

    if ( selectionRatio < 0.0f )
    {
        sithSoundClass_PlayModeRandom(pThing, mode);
    }
    else
    {
        sithSoundClass_PlayMode(pThing, mode, selectionRatio);
    }

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_PlayKey(const SithThing* pThing, const rdKeyframe* pKey, rdKeyframeFlags flags, int16_t lo, unsigned int trackGUID, DPID idTo, SithMessageStream outstream)
{
    SITH_ASSERTREL(pThing && pKey);

    SITHDSS_STARTOUT(SITHDSS_PLAYKEY);
    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(pKey->idx);
    SITHDSS_PUSHINT16(lo);
    SITHDSS_PUSHINT32(flags);
    SITHDSS_PUSHINT32(trackGUID);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, 0);
}


int J3DAPI sithDSSThing_ProcessPlayKey(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int index = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(index);
    if ( !pThing )
    {
        SITHLOG_ERROR("Got DSS::PlayKey message for thing GUID %d - not found.\n", index);
        return 0;
    }

    if ( !pThing->renderData.pPuppet )
    {
        SITHLOG_ERROR("Got DSS::PlayKey message for thing %s - no puppet.\n", pThing->aName);
        return 0;
    }

    index = SITHDSS_POPINT32();
    rdKeyframe* pKey = sithPuppet_GetKeyframeByIndex(index);
    if ( !pKey )
    {
        SITHLOG_ERROR("Key %d not found in DSS::PlayKey.\n", index);
        return 0;
    }

    int bLowPriority = SITHDSS_POPUINT16();
    rdKeyframeFlags flags = SITHDSS_POPINT32();
    unsigned int curTrackGUID = SITHDSS_POPUINT32();

    int trackNum = sithPuppet_PlayKey(pThing->renderData.pPuppet, pKey, bLowPriority, bLowPriority + 2, flags, NULL);
    if ( trackNum >= 0 )
    {
        pThing->renderData.pPuppet->aTracks[trackNum].guid = curTrackGUID;
    }

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_PlayKeyMode(const SithThing* pThing, SithPuppetSubMode mode, unsigned int trackGUID, DPID idTo, SithMessageStream outstream)
{
    SITH_ASSERTREL(pThing);

    SITHDSS_STARTOUT(SITHDSS_PLAYKEYMODE);
    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(trackGUID);
    SITHDSS_PUSHINT16(mode);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, 0);
}


int J3DAPI sithDSSThing_ProcessPlayKeyMode(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int thingGuid = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(thingGuid);
    if ( !pThing )
    {
        SITHLOG_ERROR("Got DSS::PlayKey message for thing GUID %d - not found.\n", thingGuid);
        return 0;
    }

    if ( !pThing->renderData.pPuppet )
    {
        SITHLOG_ERROR("Got DSS::PlayKeyMode message for thing %s - no puppet.\n", pThing->aName);
        return 0;
    }

    unsigned int trackGUID = SITHDSS_POPUINT32();
    SithPuppetSubMode mode = SITHDSS_POPUINT16();

    int trackNum = sithPuppet_PlayMode(pThing, mode, NULL);
    if ( trackNum >= 0 )
    {
        pThing->renderData.pPuppet->aTracks[trackNum].guid = trackGUID;
    }

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_SetModel(const SithThing* pThing, DPID idTo)
{
    SITH_ASSERTREL(pThing);

    if ( !pThing || pThing->renderData.type != RD_THING_MODEL3 || !pThing->renderData.data.pModel3 )
    {
        SITHLOG_STATUS("Asked to send state for invalid thing pointer.\n");
        return 0;
    }

    SITHDSS_STARTOUT(SITHDSS_SETMODEL);
    SITHDSS_PUSHINT32(pThing->guid);

    char aModelName[64] = { 0 };
    STD_STRNCPY(aModelName, pThing->renderData.data.pModel3->aName, STD_ARRAYLEN(aModelName) - 1);
    SITHDSS_PUSHSTRING(aModelName, STD_ARRAYLEN(aModelName));

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
}


int J3DAPI sithDSSThing_ProcessSetModel(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int guid = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
        SITHLOG_ERROR("Got DSS::PlayKey message for thing GUID %d - not found.\n", guid);
        return 0;
    }

    char aModelName[64];
    SITHDSS_POPSTRING(aModelName, STD_ARRAYLEN(aModelName));
    aModelName[STD_ARRAYLEN(aModelName) - 1] = 0;

    rdModel3* pModel = sithModel_Load(aModelName, 1);
    if ( !pModel )
    {
        return 0;
    }

    SITHLOG_STATUS("Changing %s to use model %s.\n", pThing->aName, pModel->aName);
    sithThing_SetThingModel(pThing, pModel);

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_StopKey(const SithThing* pThing, int trackGUID, float fadeTime, DPID idTo, SithMessageStream outstream)
{
    SITH_ASSERTREL(pThing);

    SITHDSS_STARTOUT(SITHDSS_STOPKEY);
    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(trackGUID);
    SITHDSS_PUSHFLOAT(fadeTime);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}


int J3DAPI sithDSSThing_ProcessStopKey(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int id = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(id);
    if ( !pThing || !pThing->renderData.pPuppet )
    {
        SITHLOG_ERROR("Bad thing guid %d in DSS::StopKey.\n", id);
        return 0;
    }

    unsigned int trackGUID = SITHDSS_POPUINT32();
    float fadeTime = SITHDSS_POPFLOAT();

    size_t track;
    for ( track = 0; track < RDPUPPET_MAX_TRACKS; ++track )
    {
        if ( pThing->renderData.pPuppet->aTracks[track].guid == trackGUID )
        {
            break;
        }
    }

    if ( track < RDPUPPET_MAX_TRACKS )
    {
        sithPuppet_StopKey(pThing->renderData.pPuppet, track, fadeTime);
    }
    else
    {
        SITHLOG_ERROR("Track with guid %x not found.\n", trackGUID);
    }

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_StopSound(tSoundChannelHandle hChannel, float secFadeTime, DPID idTo, SithMessageStream outstream)
{
    SITHDSS_STARTOUT(SITHDSS_STOPSOUND);

    int guid = hChannel ? Sound_GetChannelGUID(hChannel) : -1;
    SITHDSS_PUSHINT32(guid);
    SITHDSS_PUSHFLOAT(secFadeTime);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}


int J3DAPI sithDSSThing_ProcessStopSound(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int guid = SITHDSS_POPINT32();
    float secFadeTime = SITHDSS_POPFLOAT();

    tSoundChannelHandle hChannel = sithSoundMixer_GetChannelHandle(guid);
    if ( hChannel )
    {
        if ( secFadeTime <= 0.0f )
        {
            sithSoundMixer_StopSound(hChannel);
        }
        else
        {
            sithSoundMixer_FadeVolume(hChannel, 0.0f, secFadeTime);
        }
    }
    else
    {
        SITHLOG_ERROR("Could not find a channel with GUID %d.\n", guid);
    }

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_Fire(const SithThing* pShooter, const SithThing* pProjectileTemplate, const rdVector3* fireDir, const rdVector3* firePos, tSoundHandle hFireSnd, uint16_t puppetSubmode, float extra, int16_t projectileFlags, float sedRapidFireTime, int projectileGUID, DPID idTo, SithMessageStream outstream)
{
    SITH_ASSERTREL(pShooter && fireDir && firePos);

    SITHDSS_STARTOUT(SITHDSS_FIRE);
    SITHDSS_PUSHINT32(pShooter->guid);
    SITHDSS_PUSHINT16(projectileFlags);
    SITHDSS_PUSHINT16((int16_t)sithTemplate_GetTemplateIndex(pProjectileTemplate));
    if ( hFireSnd )
    {
        SITHDSS_PUSHINT16((int16_t)Sound_GetSoundIndex(hFireSnd));
    }
    else
    {
        SITHDSS_PUSHINT16(-1);
    }

    SITHDSS_PUSHINT16(puppetSubmode);
    SITHDSS_PUSHVEC3(firePos);
    SITHDSS_PUSHVEC3(fireDir);
    SITHDSS_PUSHFLOAT(extra);
    SITHDSS_PUSHFLOAT(sedRapidFireTime);
    SITHDSS_PUSHINT32(projectileGUID);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, 0);
}


int J3DAPI sithDSSThing_ProcessFire(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int idx = SITHDSS_POPINT32();
    SithThing* pShooter = sithThing_GetGuidThing(idx);
    if ( !pShooter )
    {
        SITHLOG_ERROR("Bad shooter guid %d in DSS:Fire.\n", idx);
        return 0;
    }

    SithFireProjectileFlags flags = SITHDSS_POPUINT16();
    idx = SITHDSS_POPUINT16();
    SithThing* pTemplate = sithTemplate_GetTemplateByIndex(idx);
    idx = SITHDSS_POPUINT16();
    tSoundHandle hSnd = Sound_GetSoundHandle(idx);
    SithPuppetSubMode submode = SITHDSS_POPUINT16();

    rdVector3 firePos;
    rdVector3 fireDir;
    SITHDSS_POPVEC3(&firePos);
    SITHDSS_POPVEC3(&fireDir);

    float extra = SITHDSS_POPFLOAT();
    float secDeltaTime = SITHDSS_POPFLOAT();
    int projectileGUID = SITHDSS_POPINT32();

    SithThing* pProjectile = sithWeapon_WeaponFireProjectile(pShooter, pTemplate, &fireDir, &firePos, hSnd, submode, extra, flags, secDeltaTime);
    if ( pProjectile )
    {
        pProjectile->flags |= SITH_TF_REMOTE;
        pProjectile->guid = projectileGUID;
    }

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_Death(const SithThing* pThing, const SithThing* pKiller, char bKillPlayer, DPID to, SithMessageStream outstream)
{
    SITHDSS_STARTOUT(SITHDSS_DEATH);
    SITHDSS_PUSHINT32(pThing->guid);
    if ( pKiller )
    {
        SITHDSS_PUSHINT32(pKiller->guid);
    }
    else
    {
        SITHDSS_PUSHINT32(-1);
    }

    SITHDSS_PUSHUINT8(bKillPlayer);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, to, outstream, DPSEND_GUARANTEED);
}


int J3DAPI sithDSSThing_ProcessDeath(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);
    SITHDSS_STARTIN(pMsg);

    int curguid = SITHDSS_POPINT32();
    int killerGuid = SITHDSS_POPINT32();
    int bKillPlayer = SITHDSS_POPUINT8();

    SithThing* pCurThing = sithThing_GetGuidThing(curguid);
    if ( !pCurThing )
    {
        SITHLOG_ERROR("Got DSS::Death message for thing guid %d I don't have locally.\n", curguid);
        return 0;
    }

    SithThing* pKiller = sithThing_GetGuidThing(killerGuid);
    switch ( pCurThing->type )
    {
        case SITH_THING_ACTOR:
            sithActor_KillActor(pCurThing, pKiller, 0);
            break;

        case SITH_THING_PLAYER:
            if ( bKillPlayer == 1 )
            {
                sithPlayer_KillPlayer(pCurThing);
            }
            else
            {
                sithActor_KillActor(pCurThing, pKiller, 0);
            }
            break;

        case SITH_THING_FREE:
            SITHLOG_ERROR("Got death message from unused thing %d.\n", pCurThing->idx);
            // Note: no break, falls through to the default case as in the original

        default:
            SITHLOG_ERROR("Got death message for unhandled thing type %d, index %d.\n", pCurThing->type, pCurThing->idx);
            break;
    }

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_DamageThing(const SithThing* pVictim, const SithThing* pPurpetrator, float damage, SithDamageType hitType, DPID idTo, SithMessageStream outstream)
{
    SITH_ASSERTREL(pVictim);

    SITHDSS_STARTOUT(SITHDSS_DAMAGE);
    SITHDSS_PUSHINT32(pVictim->guid);
    if ( pPurpetrator )
    {
        SITHDSS_PUSHINT32(pPurpetrator->guid);
    }
    else
    {
        SITHDSS_PUSHINT32(-1);
    }

    SITHDSS_PUSHFLOAT(damage);
    SITHDSS_PUSHINT16(hitType);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}


int J3DAPI sithDSSThing_ProcessDamage(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);
    SITHDSS_STARTIN(pMsg);

    if ( sithMessage_GetSenderID() != sithMulti_g_serverId )
    {
        return 0;
    }

    int guid = SITHDSS_POPINT32();
    int purpGuid = SITHDSS_POPINT32();

    SithThing* pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
        SITHLOG_ERROR("Got DSS::Death message for thing guid %d I don't have locally.\n", guid);
        return 0;
    }

    SithThing* pPurpetrator = sithThing_GetGuidThing(purpGuid);
    if ( !pPurpetrator )
    {
        pPurpetrator = pThing;
    }

    float damage = SITHDSS_POPFLOAT();
    SithDamageType hitType = SITHDSS_POPUINT16();
    sithThing_DamageThing(pThing, pPurpetrator, damage, hitType);

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_ThingFullDescription(const SithThing* pThing, DPID idTo, SithMessageStream outstream)
{
    SITHDSS_STARTOUT(SITHDSS_THINGFULLDESC);

    // Basic thing info
    SITHDSS_PUSHUINT16(pThing->idx);
    SITHDSS_PUSHUINT16(pThing->type);

    if ( pThing->type == SITH_THING_FREE )
    {
        SITH_ASSERT(SITHDSS_CURPOS() == 4); // Added: Sanity check
        SITHDSS_ENDOUT;
        return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
    }

    // Template and identification
    SITH_ASSERT(SITHDSS_CURPOS() == 4); // Added: Sanity check
    SITHDSS_PUSHINT16(sithTemplate_GetTemplateIndex(pThing->pTemplate));
    SITHDSS_PUSHUINT32(pThing->signature);
    SITHDSS_PUSHINT32(pThing->guid);

    // Position and orientation
    SITH_ASSERT(SITHDSS_CURPOS() == 14); // Added: Sanity check
    SITHDSS_PUSHVEC3(&pThing->pos);
    SITHDSS_PUSHVEC3(&pThing->forceMoveStartPos);
    SITHDSS_PUSHVEC3(&pThing->orient.rvec);
    SITHDSS_PUSHVEC3(&pThing->orient.lvec);
    SITHDSS_PUSHVEC3(&pThing->orient.uvec);

    // Movement and sector
    SITH_ASSERT(SITHDSS_CURPOS() == 74); // Added: Sanity check
    SITHDSS_PUSHUINT32(pThing->moveStatus);
    SITHDSS_PUSHINT16(sithSector_GetSectorIndex(pThing->pInSector));

    // Flags and timers
    SITH_ASSERT(SITHDSS_CURPOS() == 80); // Added: Sanity check
    SITHDSS_PUSHUINT32(pThing->flags);
    SITHDSS_PUSHUINT32(pThing->msecLifeLeft);
    SITHDSS_PUSHUINT32(pThing->msecTimerTime);
    SITHDSS_PUSHUINT32(pThing->msecNextPulseTime);
    SITHDSS_PUSHUINT32(pThing->msecPulseInterval);

    // User value and collision
    SITH_ASSERT(SITHDSS_CURPOS() == 100); // Added: Sanity check
    SITHDSS_PUSHFLOAT(pThing->userval);
    SITHDSS_PUSHUINT16(pThing->collide.type);
    SITHDSS_PUSHFLOAT(pThing->collide.size);
    SITHDSS_PUSHFLOAT(pThing->collide.movesize);

    // Light
    SITH_ASSERT(SITHDSS_CURPOS() == 114); // Added: Sanity check
    SITHDSS_PUSHVEC4(&pThing->light.color);
    SITHDSS_PUSHFLOAT(pThing->light.minRadius);
    SITHDSS_PUSHFLOAT(pThing->light.maxRadius);

    SITH_ASSERT(SITHDSS_CURPOS() == 138); // Added: Sanity check
    SITHDSS_PUSHUINT32(pThing->unknownFlags);

    // COG linkage
    if ( (pThing->flags & SITH_TF_COGLINKED) != 0 )
    {
        SITH_ASSERT(SITHDSS_CURPOS() == 142); // Added: Sanity check
        SITHDSS_PUSHINT16(pThing->pCog ? pThing->pCog->idx : -1);
        SITHDSS_PUSHINT16(pThing->pCaptureCog ? pThing->pCaptureCog->idx : -1);
        SITH_ASSERT(SITHDSS_CURPOS() == 146); // Added: Sanity check
    }

    // Type-specific info
    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_CORPSE:
        case SITH_THING_PLAYER:
        {
            const SithActorInfo* pActor = &pThing->thingInfo.actorInfo;

            SITHDSS_PUSHUINT32(pActor->flags);
            SITHDSS_PUSHFLOAT(pActor->health);
            SITHDSS_PUSHFLOAT(pActor->maxHealth);
            SITHDSS_PUSHFLOAT(pActor->extraSpeed);
            SITHDSS_PUSHVEC3(&pActor->headPYR);
            SITHDSS_PUSHVEC3((const rdVector3*)&pActor->headLightIntensity);

            SITHDSS_PUSHUINT32(pActor->stateChange.type);
            SITHDSS_PUSHUINT32(pActor->stateChange.params.armedMode);

            SITHDSS_PUSHUINT8(pActor->bForceMovePlay);
            SITHDSS_PUSHUINT8(pActor->bControlsDisabled);

            SITHDSS_PUSHUINT32(pActor->endurance.msecUnderwater);
            SITHDSS_PUSHFLOAT(pActor->maxThrust);
            SITHDSS_PUSHFLOAT(pActor->maxRotVelocity);
            SITHDSS_PUSHFLOAT(pActor->minHeadPitch);
            SITHDSS_PUSHFLOAT(pActor->maxHeadPitch);
            SITHDSS_PUSHFLOAT(pActor->maxHeadYaw);
            SITHDSS_PUSHFLOAT(pActor->maxHeadVelocity);
            SITHDSS_PUSHINT16(sithThing_GetThingIndex(pActor->pThingMeshAttached));
            SITHDSS_PUSHINT32(pActor->attachMeshNum);

            SITHDSS_PUSHINT16(pActor->pPlayer ? pActor->pPlayer - sithPlayer_g_aPlayers : -1);
            break;
        }

        case SITH_THING_WEAPON:
            SITHDSS_PUSHUINT32(pThing->thingInfo.weaponInfo.flags);
            SITHDSS_PUSHUINT16(pThing->thingInfo.weaponInfo.numRicochets);
            break;

        case SITH_THING_EXPLOSION:
            SITHDSS_PUSHUINT32(pThing->thingInfo.explosionInfo.flags);
            break;

        case SITH_THING_SPRITE:
        {
            const SithSpriteInfo* pSprite = &pThing->thingInfo.spriteInfo;

            SITHDSS_PUSHFLOAT(pSprite->width);
            SITHDSS_PUSHFLOAT(pSprite->height);
            SITHDSS_PUSHFLOAT(pSprite->alpha);
            SITHDSS_PUSHFLOAT(pSprite->rollAngle);
            SITHDSS_PUSHINT16(sithThing_GetThingIndex(pSprite->pThingMeshAttached));
            SITHDSS_PUSHINT32(pSprite->attachMeshNum);
            break;
        }

        case SITH_THING_POLYLINE:
        {
            rdPolyline* pPolyline = pThing->renderData.data.pPolyline;
            SITH_ASSERTREL(pPolyline);

            // TODO: make this not hacky and hardcoded
            if ( streq(pThing->aName, "+plcogend") )
            {
                SITHDSS_PUSHINT16(sithThing_GetThingIndex(pThing->pParent));
                pPolyline->face.extraLight.alpha = pThing->alpha; // TODO: Is this a bug? Why set alpha here?
            }

            char aMatName[64] = { 0 }; // Fixed: Inited to 0
            STD_STRCPY(aMatName, pPolyline->face.pMaterial->aName);
            SITHDSS_PUSHSTRING(aMatName, STD_ARRAYLEN(aMatName));

            SITHDSS_PUSHFLOAT(pPolyline->length);
            SITHDSS_PUSHFLOAT(pPolyline->baseRadius);
            SITHDSS_PUSHFLOAT(pPolyline->tipRadius);
            SITHDSS_PUSHUINT16(pPolyline->geoMode);
            SITHDSS_PUSHUINT16(pPolyline->lightMode);
            SITHDSS_PUSHVEC4(&pPolyline->face.extraLight);
            break;
        }

        default:
            break;
    }

    // Movement info
    // TODO: Find a way to serialize moveType as on restore the moveType can be restored to template and not to thing placement or cog set (ParseArg) move type
    SithThingMoveType moveType = pThing->moveType;
    if ( moveType == SITH_MT_PHYSICS )
    {
        const SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;

        SITHDSS_PUSHUINT32(pPhysics->flags);
        SITHDSS_PUSHVEC3(&pPhysics->velocity);
        SITHDSS_PUSHVEC3(&pPhysics->angularVelocity);
        SITHDSS_PUSHVEC3(&pPhysics->thrust);
        SITHDSS_PUSHVEC3(&pPhysics->rotThrust);
        SITHDSS_PUSHFLOAT(pPhysics->maxRotationVelocity);
        SITHDSS_PUSHFLOAT(pPhysics->maxVelocity);
        SITHDSS_PUSHFLOAT(pPhysics->orientSpeed);
        SITHDSS_PUSHFLOAT(pPhysics->buoyancy);
        SITHDSS_PUSHFLOAT(pPhysics->height);
        SITHDSS_PUSHFLOAT(pPhysics->mass);
    }
    else if ( moveType == SITH_MT_PATH )
    {
        const SithPathMoveInfo* pPath = &pThing->moveInfo.pathMovement;

        SITHDSS_PUSHUINT16(pPath->mode);
        SITHDSS_PUSHVEC3(&pPath->vecDeltaPos);
        SITHDSS_PUSHFLOAT(pPath->moveTimeRemaining);
        SITHDSS_PUSHFLOAT(pPath->moveVel);

        SITHDSS_PUSHFLOAT(pPath->rotDeltaTime);
        SITHDSS_PUSHVEC3(&pPath->rotOffset);
        SITHDSS_PUSHVEC3(&pPath->goalPYR);
        SITHDSS_PUSHFLOAT(pPath->rotDelta);

        SITHDSS_PUSHUINT16(pPath->numBlockedMoves);
        SITHDSS_PUSHINT16(pPath->currentFrame);
        SITHDSS_PUSHUINT16(pPath->nextFrame);
        SITHDSS_PUSHUINT16(pPath->goalFrame);

        SITHDSS_PUSHVEC3(&pPath->curOrient.rvec);
        SITHDSS_PUSHVEC3(&pPath->curOrient.lvec);
        SITHDSS_PUSHVEC3(&pPath->curOrient.uvec);
        SITHDSS_PUSHVEC3(&pPath->curOrient.dvec);
        SITHDSS_PUSHVEC3(&pPath->rotateToPYR);

        SITHDSS_PUSHUINT16(pPath->numFrames);
        for ( size_t i = 0; i < pPath->numFrames; ++i )
        {
            SITHDSS_PUSHVEC3(&pPath->aFrames[i].pos);
            SITHDSS_PUSHVEC3(&pPath->aFrames[i].pyr);
        }
    }

    // Swap list
    if ( pThing->pSwapList )
    {
        int16_t numUsedSwapEntries = 0;
        for ( const SithThingSwapEntry* pEntry = pThing->pSwapList; pEntry; pEntry = pEntry->pNextEntry )
        {
            ++numUsedSwapEntries;
        }
        SITHDSS_PUSHINT16(numUsedSwapEntries);

        for ( const SithThingSwapEntry* pEntry = pThing->pSwapList; pEntry; pEntry = pEntry->pNextEntry )
        {
            SITHDSS_PUSHUINT32(pEntry->entryNum);
            SITHDSS_PUSHINT32(pEntry->meshNum);
            SITHDSS_PUSHINT16(sithModel_GetModelIndex(pEntry->pSrcModel));
            SITHDSS_PUSHINT32(pEntry->srcMeshNum);
        }
    }
    else
    {
        SITHDSS_PUSHINT16(-1);
    }

    // Serialize total number of swap entries (for validation on restore)
    SITHDSS_PUSHUINT32(pThing->numSwapEntries);

    // 3DO model index and insert offset
    // TODO: Why is this even necessary. Assuming due to model could be changed at runtime, e.g.: via COG script
    // Fixed: Added check for render type to be RD_THING_MODEL3. OG was checking only for pModel3 being non-null,
    //        This was causing serialization corruption when thing had different render type and sithModel_GetModelIndex would return -1 but still write insertOffset.
    //        Such thing would then deserialize incorrectly, as insertOffset wouldn't be read when model index was -1, causing all subsequent data to be misaligned.
    //
    //        For example, a sprite thing would serialize data of 2D vector pThing->renderData.data.pSprite3->face.texVertOffset
    //        and pThing->renderData.data.pSprite3->face.extraLight.red. But on deserialization, since model index would be -1,
    //        these values wouldn't be read out as insertOffset vector and pThing->alpha = pThing->renderData.data.pSprite3->face.texVertOffset.x
    //        causing thing transparency value to be incorrect.
    if ( pThing->renderData.type == RD_THING_MODEL3 && pThing->renderData.data.pModel3 )
    {
        SITHDSS_PUSHINT16(sithModel_GetModelIndex(pThing->renderData.data.pModel3));
        SITHDSS_PUSHVEC3(&pThing->renderData.data.pModel3->insertOffset);
    }
    else
    {
        SITHDSS_PUSHINT16(-1);
    }

    // Actor/Player weapon and voice info
    SithThingType type = pThing->type;
    if ( type == SITH_THING_ACTOR || type == SITH_THING_PLAYER )
    {
        const SithActorWeaponInfo* pWeaponInfo = &pThing->thingInfo.actorInfo.weaponInfo;
        const SithActorVoiceInfo* pVoiceInfo   = &pThing->thingInfo.actorInfo.voiceInfo;

        SITHDSS_PUSHFLOAT(pWeaponInfo->secActivationStartTime);
        SITHDSS_PUSHFLOAT(pWeaponInfo->secLastRapidFireTime);
        SITHDSS_PUSHFLOAT(pWeaponInfo->secActivationWaitEndTime);
        SITHDSS_PUSHFLOAT(pWeaponInfo->secActivationWaitTime);
        SITHDSS_PUSHFLOAT(pWeaponInfo->secAimWaitEndTime);
        SITHDSS_PUSHFLOAT(pWeaponInfo->secSwapTime);
        SITHDSS_PUSHINT32(pWeaponInfo->selectedWeaponID);
        SITHDSS_PUSHINT32(pWeaponInfo->curWeaponID);
        SITHDSS_PUSHINT32(pWeaponInfo->deselectedWeaponID);
        SITHDSS_PUSHINT32(pWeaponInfo->swapRefNum);
        SITHDSS_PUSHVEC3(&pWeaponInfo->vecUnknown0);
        SITHDSS_PUSHVEC3(&pThing->thingInfo.actorInfo.fireOffset);

        SITHDSS_PUSHVEC4(&pVoiceInfo->voiceColor.top); // TODO: Why only top color is serialized?
        SITHDSS_PUSHUINT32(pVoiceInfo->hSndChannel);
        SITHDSS_PUSHINT32(pVoiceInfo->voiceHeadInfo.headMeshNum);
        SITHDSS_PUSHINT32(pVoiceInfo->voiceHeadInfo.headSwapRefNum);

        for ( size_t k = 0; k < STD_ARRAYLEN(pVoiceInfo->voiceHeadInfo.apSoundHeadModels); ++k )
        {
            static_assert(STD_ARRAYLEN(pVoiceInfo->voiceHeadInfo.apSoundHeadModels) == 4, "Array length mismatch");
            SITHDSS_PUSHINT16(sithModel_GetModelIndex(pVoiceInfo->voiceHeadInfo.apSoundHeadModels[k]));
        }
    }

    // Note, serialized here as alpha is new field added to SithThing struct in Jones engine.
    SITHDSS_PUSHFLOAT(pThing->alpha);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // OpenJones3D extension from here onwards
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if ( pThing->type == SITH_THING_POLYLINE )
    {
        // Added: Serialize polyline new flags field
        SITHDSS_PUSHINT32(pThing->renderData.data.pPolyline->flags);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, (SithMessageStream)outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessThingFullDescription(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);

    if ( stdComm_IsGameActive() && (sithMain_g_sith_mode.subModeFlags & SITH_SUBMODE_SYNC) == 0 )
    {
        SITHLOG_ERROR("Received DSS::ThingFullDesc message when not in sync mode.\n");
        return 0;
    }

    SITHDSS_STARTIN(pMsg);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;

    // Basic thing info
    size_t thingIdx = SITHDSS_POPUINT16();
    if ( thingIdx >= pWorld->numThings )
    {
        SITHLOG_ERROR("FullDescription received for thing id %d, out of range.\n", thingIdx);
        return 0;
    }

    SithThingType thingType = SITHDSS_POPUINT16();
    if ( thingType >= SITH_THING_NUMTYPES ) // Fixed: Reject save-provided thing types before they select unions or initialization paths.
    {
        SITHLOG_ERROR("FullDescription received invalid thing type %d for thing id %d.\n", thingType, (int)thingIdx);
        return 0;
    }

    SithThing* pThing = &pWorld->aThings[thingIdx];
    if ( pThing->type != SITH_THING_FREE )
    {
        // This will make thing free
        sithThing_RemoveThing(pWorld, pThing);
    }

    if ( pWorld->lastThingIdx <= (int)thingIdx )
    {
        pWorld->lastThingIdx = thingIdx;
    }

    if ( thingType == SITH_THING_FREE )
    {
        SITHDSS_ENDIN;
        return 1;
    }

    pThing = &pWorld->aThings[thingIdx]; // Why get thing again?
    sithThing_Reset(pThing);

    // Template and identification
    size_t templateIdx = SITHDSS_POPUINT16(); // Note, don't change type as it will set max unsigned value if negative
    SithThing* pTemplate = sithTemplate_GetTemplateByIndex((int)templateIdx); // Fixed: Use template pointer returned by sithTemplate_GetTemplateByIndex
    if ( pTemplate ) // Fixed: Use the validated template pointer;
    {
        sithThing_SetThingBasedOn(pThing, pTemplate); // Fixed: Static template indices are not valid offsets into the current world array, vanilla was: &pWorld->aThingTemplates[templateIdx]
    }

    pThing->type      = thingType; // Note, Don't move as SetThingBasedOn may change type
    pThing->signature = SITHDSS_POPUINT32();
    pThing->guid      = SITHDSS_POPINT32();

    // Position and orientation
    SITHDSS_POPVEC3(&pThing->pos);
    SITHDSS_POPVEC3(&pThing->forceMoveStartPos);
    SITHDSS_POPVEC3(&pThing->orient.rvec);
    SITHDSS_POPVEC3(&pThing->orient.lvec);
    SITHDSS_POPVEC3(&pThing->orient.uvec);

    // Movement and sector
    pThing->moveStatus = SITHDSS_POPUINT32();

    int sectorIdx = SITHDSS_POPINT16();
    SithSector* pSector = sithSector_GetSectorEx(pWorld, sectorIdx);
    if ( pSector )
    {
        sithThing_SetSector(pThing, pSector, /*bNotify=*/1);
    }

    // Flags and timers
    pThing->flags             = SITHDSS_POPUINT32();
    pThing->msecLifeLeft      = SITHDSS_POPUINT32();
    pThing->msecTimerTime     = SITHDSS_POPUINT32();
    pThing->msecNextPulseTime = SITHDSS_POPUINT32();
    pThing->msecPulseInterval = SITHDSS_POPUINT32();

    // User value and collision
    pThing->userval          = SITHDSS_POPFLOAT();
    pThing->collide.type     = SITHDSS_POPUINT16();
    pThing->collide.size     = SITHDSS_POPFLOAT();
    pThing->collide.movesize = SITHDSS_POPFLOAT();

    // Light
    SITHDSS_POPVEC4(&pThing->light.color);
    pThing->light.minRadius = SITHDSS_POPFLOAT();
    pThing->light.maxRadius = SITHDSS_POPFLOAT();

    pThing->unknownFlags = SITHDSS_POPUINT32();

    // COG linkage
    if ( (pThing->flags & SITH_TF_COGLINKED) != 0 )
    {
        int cogIdx   = SITHDSS_POPUINT16(); // Deserialize as unsigned to avoid sign-extension issues
        pThing->pCog = sithCog_GetCogByIndex(cogIdx);

        cogIdx = SITHDSS_POPUINT16(); // Deserialize as unsigned to avoid sign-extension issues
        pThing->pCaptureCog = sithCog_GetCogByIndex(cogIdx);
    }

    // Type-specific info
    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_CORPSE:
        case SITH_THING_PLAYER:
        {
            SithActorInfo* pActor = &pThing->thingInfo.actorInfo;

            pActor->flags      = SITHDSS_POPUINT32();
            pActor->health     = SITHDSS_POPFLOAT();
            pActor->maxHealth  = SITHDSS_POPFLOAT();
            pActor->extraSpeed = SITHDSS_POPFLOAT();

            SITHDSS_POPVEC3(&pActor->headPYR);
            SITHDSS_POPVEC3((rdVector3*)&pActor->headLightIntensity); // Note: no alpha in DSS stream
            pActor->headLightIntensity.alpha = 0.0f;

            pActor->stateChange.type             = SITHDSS_POPUINT32();
            pActor->stateChange.params.armedMode = SITHDSS_POPUINT32();

            pActor->bForceMovePlay    = SITHDSS_POPUINT8();
            pActor->bControlsDisabled = SITHDSS_POPUINT8();

            pActor->endurance.msecUnderwater = SITHDSS_POPUINT32();
            pActor->maxThrust                = SITHDSS_POPFLOAT();
            pActor->maxRotVelocity           = SITHDSS_POPFLOAT();
            pActor->minHeadPitch             = SITHDSS_POPFLOAT();
            pActor->maxHeadPitch             = SITHDSS_POPFLOAT();
            pActor->maxHeadYaw               = SITHDSS_POPFLOAT();
            pActor->maxHeadVelocity          = SITHDSS_POPFLOAT();

            size_t attachThingIdx = SITHDSS_POPINT16(); // Note, don't change type as it will set max unsigned value if negative
            if ( attachThingIdx < pWorld->numThings )
            {
                pActor->pThingMeshAttached = &pWorld->aThings[attachThingIdx]; // TODO: use sithThing_GetThingByIndex instead.
            }

            pActor->attachMeshNum = SITHDSS_POPINT32();

            size_t playerIdx = SITHDSS_POPINT16(); // Note, don't change type as it will set max unsigned value if negative
            if ( playerIdx < SITHPLAYER_MAX_PLAYERS )
            {
                pActor->pPlayer = &sithPlayer_g_aPlayers[playerIdx];
            }
            break;
        }

        case SITH_THING_WEAPON:
        {
            SithWeaponInfo* pWeapon = &pThing->thingInfo.weaponInfo;
            pWeapon->flags        = SITHDSS_POPUINT32();
            pWeapon->numRicochets = SITHDSS_POPUINT16();
            break;
        }

        case SITH_THING_EXPLOSION:
        {
            SithExplosionInfo* pExplosion = &pThing->thingInfo.explosionInfo;
            pExplosion->flags = SITHDSS_POPUINT32();
            break;
        }

        case SITH_THING_SPRITE:
        {
            SithSpriteInfo* pSprite = &pThing->thingInfo.spriteInfo;
            pSprite->width     = SITHDSS_POPFLOAT();
            pSprite->height    = SITHDSS_POPFLOAT();
            pSprite->alpha     = SITHDSS_POPFLOAT();
            pSprite->rollAngle = SITHDSS_POPFLOAT();

            size_t attachThingIdx = SITHDSS_POPINT16(); // Note, don't change type as it will set max unsigned value if negative
            if ( attachThingIdx < pWorld->numThings )
            {
                pSprite->pThingMeshAttached = &pWorld->aThings[attachThingIdx];
            }

            pSprite->attachMeshNum = SITHDSS_POPINT32();
            break;
        }

        case SITH_THING_POLYLINE:
        {
            if ( streq(pThing->aName, "+plcogend") )
            {
                size_t parentIdx = SITHDSS_POPINT16(); // Note, don't change type as it will set max unsigned value if negative
                if ( parentIdx < pWorld->numThings )
                {
                    pThing->pParent = &pWorld->aThings[parentIdx];
                }
            }

            char matName[64] = { 0 }; // Fixed: Init to 0
            SITHDSS_POPSTRING(matName, STD_ARRAYLEN(matName));

            float length           = SITHDSS_POPFLOAT();
            float baseRadius       = SITHDSS_POPFLOAT();
            float tipRadius        = SITHDSS_POPFLOAT();
            rdGeometryMode geoMode = SITHDSS_POPUINT16();
            rdLightMode lightMode  = SITHDSS_POPUINT16();

            rdVector4 polylineColor;
            SITHDSS_POPVEC4(&polylineColor);

            rdPolyline* pPolyline = rdPolyline_New(pThing->aName, matName, matName, length, baseRadius, tipRadius, geoMode, lightMode, &polylineColor);
            if ( pPolyline )
            {
                rdThing_NewEntry(&pThing->renderData, NULL); // Note: sithThing_Reset calls rdThing_NewEntry(&pThing->renderData, pThing);
                rdThing_SetPolyline(&pThing->renderData, pPolyline);
            }

            if ( streq(pThing->aName, "+plcogend") )
            {
                pThing->alpha = polylineColor.alpha;
            }
            break;
        }

        default:
            break;
    }

    // Movement info
    // TODO: Find a way to deserialize moveType as move type could be changed via thing placement args or COG script (ParseArg) but here template move type is used.
    SithThingMoveType moveType = pThing->moveType;
    if ( moveType == SITH_MT_PHYSICS )
    {
        SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;

        pPhysics->flags = SITHDSS_POPUINT32();
        SITHDSS_POPVEC3(&pPhysics->velocity);
        SITHDSS_POPVEC3(&pPhysics->angularVelocity);
        SITHDSS_POPVEC3(&pPhysics->thrust);
        SITHDSS_POPVEC3(&pPhysics->rotThrust);

        pPhysics->maxRotationVelocity = SITHDSS_POPFLOAT();
        pPhysics->maxVelocity         = SITHDSS_POPFLOAT();
        pPhysics->orientSpeed         = SITHDSS_POPFLOAT();
        pPhysics->buoyancy            = SITHDSS_POPFLOAT();
        pPhysics->height              = SITHDSS_POPFLOAT();
        pPhysics->mass                = SITHDSS_POPFLOAT();
    }
    else if ( moveType == SITH_MT_PATH )
    {
        SithPathMoveInfo* pPath = &pThing->moveInfo.pathMovement;

        pPath->mode = SITHDSS_POPUINT16();

        SITHDSS_POPVEC3(&pPath->vecDeltaPos);
        pPath->moveTimeRemaining = SITHDSS_POPFLOAT();
        pPath->moveVel           = SITHDSS_POPFLOAT();
        pPath->rotDeltaTime      = SITHDSS_POPFLOAT();

        SITHDSS_POPVEC3(&pPath->rotOffset);
        SITHDSS_POPVEC3(&pPath->goalPYR);
        pPath->rotDelta = SITHDSS_POPFLOAT();

        pPath->numBlockedMoves = SITHDSS_POPUINT16();
        pPath->currentFrame    = SITHDSS_POPINT16();
        pPath->nextFrame       = SITHDSS_POPUINT16();
        pPath->goalFrame       = SITHDSS_POPUINT16();

        SITHDSS_POPVEC3(&pPath->curOrient.rvec);
        SITHDSS_POPVEC3(&pPath->curOrient.lvec);
        SITHDSS_POPVEC3(&pPath->curOrient.uvec);
        SITHDSS_POPVEC3(&pPath->curOrient.dvec);
        SITHDSS_POPVEC3(&pPath->rotateToPYR);

        pPath->numFrames  = SITHDSS_POPUINT16();
        if ( pPath->numFrames > 0 )
        {
            // Fixed: Reject corrupt path frame counts before reading past the DSS payload.
            const size_t frameDataSize = sizeof(SithPathFrame) * pPath->numFrames;
            if ( SITHDSS_CURPOS() > pMsg->length || frameDataSize > pMsg->length - SITHDSS_CURPOS() )
            {
                SITHLOG_ERROR("FullDescription path frame count %d exceeds remaining message data for thing %s.\n", (int)pPath->numFrames, pThing->aName);
                return 0;
            }

            pPath->sizeFrames =  pPath->numFrames;
            pPath->aFrames    = (SithPathFrame*)STDMALLOC(sizeof(SithPathFrame) * pPath->numFrames);
            if ( !pPath->aFrames ) // Fixed: Handle allocation failure before writing restored path frames.
            {
                SITHLOG_ERROR("Failed to allocate %d restored path frames for thing %s.\n", (int)pPath->numFrames, pThing->aName);
                return 0;
            }

            for ( size_t i = 0; i < pPath->numFrames; ++i )
            {
                SITHDSS_POPVEC3(&pPath->aFrames[i].pos);
                SITHDSS_POPVEC3(&pPath->aFrames[i].pyr);
            }
        }
    }

    // Now we have enough info to initialize the thing
    sithThing_Initialize(pWorld, pThing, /*bFindFloor=*/0);

    // Swap list
    if ( pThing->pSwapList )
    {
        // Remove existing swap list
        sithThing_ResetSwapList(pThing);
    }

    int16_t numUsedSwapEntries = SITHDSS_POPINT16();
    if ( numUsedSwapEntries < -1 ) // Fixed: Reject invalid negative swap counts from corrupt save data.
    {
        SITHLOG_ERROR("FullDescription received invalid swap entry count %d for thing %s.\n", numUsedSwapEntries, pThing->aName);
        return 0;
    }

    if ( numUsedSwapEntries != -1 )
    {
        // Fixed: Reject corrupt swap-list counts before they drive reads past the DSS payload.
        const size_t swapEntryDataSize = (sizeof(uint32_t) + sizeof(int32_t) + sizeof(int16_t) + sizeof(int32_t)) * (size_t)numUsedSwapEntries;
        if ( SITHDSS_CURPOS() > pMsg->length || swapEntryDataSize > pMsg->length - SITHDSS_CURPOS() )
        {
            SITHLOG_ERROR("FullDescription swap entry count %d exceeds remaining message data for thing %s.\n", numUsedSwapEntries, pThing->aName);
            return 0;
        }

        for ( int i = 0; i < numUsedSwapEntries; ++i )
        {
            uint32_t entryNum = SITHDSS_POPUINT32();
            int meshNum       = SITHDSS_POPINT32();
            uint16_t modelIdx = SITHDSS_POPINT16(); // Note, don't change type as it will set max unsigned value if negative, but sithModel_GetModelByIndex will handle out of range indices and return null.
            rdModel3* pModel  = sithModel_GetModelByIndex(modelIdx);
            int meshNumSrc    = SITHDSS_POPINT32();

            if ( pModel )
            {
                pThing->numSwapEntries = entryNum; // Note: sithThing_AddSwapEntry sets entry num as:  pNewEntry->entryNum = pThing->numSwapEntries++;
                sithThing_AddSwapEntry(pThing, meshNum, pModel, meshNumSrc);
            }
        }
    }

    pThing->numSwapEntries = SITHDSS_POPUINT32();

    // 3DO model radius
    int16_t modelIdx = SITHDSS_POPINT16();
    if ( modelIdx != -1 )
    {
        rdModel3* pModel = sithModel_GetModelByIndex((uint16_t)modelIdx); // Note: cast to uint16_t to match type expected by sithModel_GetModelByIndex and avoid sign-extension issues with negative values.
        if ( !pModel ) // Fixed: Reject invalid model indices before dereferencing renderData.data.pModel3.
        {
            SITHLOG_ERROR("FullDescription received invalid model index %d for thing %s.\n", modelIdx, pThing->aName);
            return 0;
        }

        if ( pModel && pThing->renderData.data.pModel3 != pModel )
        {
            sithThing_SetThingModel(pThing, pModel);
        }

        // Fixed: A non-model render entry cannot receive a restored model insert offset.
        if ( pThing->renderData.type != RD_THING_MODEL3 || !pThing->renderData.data.pModel3 )
        {
            SITHLOG_ERROR("FullDescription model insert offset received for non-model thing %s.\n", pThing->aName);
            return 0;
        }

        SITHDSS_POPVEC3(&pThing->renderData.data.pModel3->insertOffset);
    }

    // Actor/Player weapon and voice info
    if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
    {
        SithActorWeaponInfo* pWeaponInfo = &pThing->thingInfo.actorInfo.weaponInfo;
        SithActorVoiceInfo* pVoiceInfo   = &pThing->thingInfo.actorInfo.voiceInfo;

        pWeaponInfo->secActivationStartTime   = SITHDSS_POPFLOAT();
        pWeaponInfo->secLastRapidFireTime     = SITHDSS_POPFLOAT();
        pWeaponInfo->secActivationWaitEndTime = SITHDSS_POPFLOAT();
        pWeaponInfo->secActivationWaitTime    = SITHDSS_POPFLOAT();
        pWeaponInfo->secAimWaitEndTime        = SITHDSS_POPFLOAT();
        pWeaponInfo->secSwapTime              = SITHDSS_POPFLOAT();
        pWeaponInfo->selectedWeaponID         = SITHDSS_POPINT32();
        pWeaponInfo->curWeaponID              = SITHDSS_POPINT32();
        pWeaponInfo->deselectedWeaponID       = SITHDSS_POPINT32();
        pWeaponInfo->swapRefNum               = SITHDSS_POPINT32();
        SITHDSS_POPVEC3(&pWeaponInfo->vecUnknown0);
        SITHDSS_POPVEC3(&pThing->thingInfo.actorInfo.fireOffset);

        SITHDSS_POPVEC4(&pVoiceInfo->voiceColor.top);
        pVoiceInfo->hSndChannel                  = SITHDSS_POPUINT32();
        pVoiceInfo->voiceHeadInfo.headMeshNum    = SITHDSS_POPINT32();
        pVoiceInfo->voiceHeadInfo.headSwapRefNum = SITHDSS_POPINT32();

        for ( size_t i = 0; i < STD_ARRAYLEN(pVoiceInfo->voiceHeadInfo.apSoundHeadModels); ++i )
        {
            static_assert(STD_ARRAYLEN(pVoiceInfo->voiceHeadInfo.apSoundHeadModels) == 4, "Array length mismatch");
            modelIdx = SITHDSS_POPINT16();
            pVoiceInfo->voiceHeadInfo.apSoundHeadModels[i] = sithModel_GetModelByIndex((uint16_t)modelIdx); // Note: cast to uint16_t to match type expected by sithModel_GetModelByIndex and avoid sign-extension issues with negative values.
        }
    }

    float alphaValue = SITHDSS_POPFLOAT();
    if ( !streq(pThing->aName, "+plcogend") ) // TODO: Make this less hacky
    {
        pThing->alpha = alphaValue;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // OpenJones3D extension from here onwards
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if ( SITHDSS_CURPOS() < pMsg->length ) // Only new streams will have extra data (savegame files create with oj3D >= v0.4)
    {
        if ( pThing->type == SITH_THING_POLYLINE )
        {
            if ( pMsg->length - SITHDSS_CURPOS() < sizeof(int32_t) || !pThing->renderData.data.pPolyline )
            {
                SITHLOG_ERROR("FullDescription polyline extension data is truncated for thing %s.\n", pThing->aName);
                return 0;
            }

            // Added: Deserialize polyline new flags field
            pThing->renderData.data.pPolyline->flags = SITHDSS_POPINT32();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    SITHDSS_ENDIN;

    // Added: Sanity check to see if all bytes were consumed from the message
#ifdef J3D_DEBUG
    if ( SITHDSS_CURPOS() != pMsg->length )
    {
        SITHLOG_WARNING("sithDSSThing_ProcessThingFullDescription: Not all data was processed for thing: %s [curpos: %d msglen: %d]\n", pThing->aName, SITHDSS_CURPOS(), pMsg->length);
    }
#endif

    return 1;
}

int J3DAPI sithDSSThing_PathMove(const SithThing* pThing, int16_t frame, float speed, int moveType, DPID idTO, SithMessageStream outstream)
{
    if ( !pThing || pThing->moveType != SITH_MT_PATH || !pThing->type || !pThing->pInSector )
    {
        return 0;
    }

    SITHDSS_STARTOUT(SITHDSS_PATHMOVE);
    SITHDSS_PUSHINT32(moveType);
    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(sithMain_g_frameNumber);
    SITHDSS_PUSHINT16((int16_t)sithSector_GetSectorIndex(pThing->pInSector));
    SITHDSS_PUSHVEC3(&pThing->pos);

    rdVector3 curpyr;
    rdMatrix_ExtractAngles34(&pThing->orient, &curpyr);
    SITHDSS_PUSHVEC3(&curpyr);

    SITHDSS_PUSHINT16(frame);
    SITHDSS_PUSHFLOAT(speed);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTO, outstream, DPSEND_GUARANTEED);
}


int J3DAPI sithDSSThing_ProcessPathMove(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);
    SITHDSS_STARTIN(pMsg);

    int moveClass = SITHDSS_POPINT32();
    int curGuid = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(curGuid);
    if ( !pThing || pThing->moveType != SITH_MT_PATH )
    {
        goto error;
    }

    unsigned int moveFrame = SITHDSS_POPUINT32();
    if ( (unsigned int)pThing->moveFrame > moveFrame ) // Note: unsigned compare, as in the original
    {
        SITHLOG_STATUS("Received old move command: Frame %d < frame %d.  Ignored.\n", moveFrame, pThing->moveFrame);
        return 1;
    }

    pThing->moveFrame = moveFrame;
    int sectIndex = SITHDSS_POPUINT16();
    SithSector* pSector = sithSector_GetSectorEx(sithWorld_g_pCurrentWorld, sectIndex);
    if ( !pSector )
    {
        goto error;
    }

    SITHDSS_POPVEC3(&pThing->pos);
    sithThing_SetSector(pThing, pSector, 0);

    rdVector3 curPyr;
    SITHDSS_POPVEC3(&curPyr);
    rdMatrix_BuildRotate34(&pThing->orient, &curPyr);

    unsigned int curFrm = SITHDSS_POPUINT16();
    float velocity = SITHDSS_POPFLOAT();

    switch ( moveClass )
    {
        case 0:
            sithPathMove_MoveToFrame(pThing, curFrm, velocity);
            break;

        case 1:
            sithPathMove_SkipToFrame(pThing, curFrm, velocity);
            break;

        case 2:
            sithPathMove_Finish(pThing);
            break;
    }

    SITHDSS_ENDIN;
    return 1;

error:
    SITHLOG_ERROR("Bad format in DSS::PathMove message, GUID=%x.\n", curGuid);
    return 0;
}


int J3DAPI sithDSSThing_Attachment(const SithThing* pThing, DPID idTo, SithMessageStream outstream, unsigned int sendFlags)
{
    SITHDSS_STARTOUT(SITHDSS_ATTACHMENT);
    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHUINT16(pThing->attach.flags);

    /**(uint32_t*)sithMulti_g_message.data = pThing->guid;
    *(uint16_t*)&sithMulti_g_message.data[4] = pThing->attach.flags;*/
    //uint8_t* pCurOut = &sithMulti_g_message.data[6];

    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 || (pThing->attach.flags & SITH_ATTACH_CLIMBSURFACE) != 0 )
    {
        SITHDSS_PUSHINT16(sithSurface_GetSurfaceIndex(pThing->attach.attachedToStructure.pSurfaceAttached));
        //*(uint16_t*)pCurOut = sithSurface_GetSurfaceIndex(pThing->attach.attachedToStructure.pSurfaceAttached);
        //pCurOut = &sithMulti_g_message.data[8];
    }
    else if ( (pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0 )
    {
        SITHDSS_PUSHINT32(pThing->attach.attachedToStructure.pThingAttached->guid);
       /**(uint32_t*)pCurOut = pThing->attach.attachedToStructure.pThingAttached->guid;
       pCurOut = &sithMulti_g_message.data[10];*/

        if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 )
        {
            SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
            if ( pThingAttached && pThingAttached->renderData.data.pModel3 )
            {
                // Note: This is buggy as the attached pFace can be from any of the model's mesh and not from the first one.
                const rdModel3Mesh* pMesh = pThing->attach.attachedToStructure.pThingAttached->renderData.data.pModel3->aGeos[0].aMeshes;
                int16_t faceIdx = pThing->attach.pFace - pMesh->aFaces;

                // Fixed: Added
                if ( faceIdx >= 0 && faceIdx < pMesh->numFaces )
                {
                    SITHDSS_PUSHINT16(faceIdx);
                }
                else
                {
                    // TODO: This is quick fix, a better solution would be to pack mesh index and face index in int16.
                    //       Note, the vanilla engine doesn't handle -1 index on load and can crash the game.
                    //       But we don't care as it could crash anyway since the index is invalid.
                    SITHDSS_PUSHINT16(-1);
                }

                /**(uint16_t*)pCurOut = pThing->attach.pFace - pThing->attach.attachedToStructure.pThingAttached->renderData.data.pModel3->aGeos[0].aMeshes->aFaces;
                pCurOut = &sithMulti_g_message.data[12];*/
            }
            else
            {
                SITHDSS_PUSHINT16(-1);
                /**(uint16_t*)pCurOut = -1;
                pCurOut = &sithMulti_g_message.data[12];*/
            }
        }
        else
        {
            SITHDSS_PUSHVEC3(&pThing->attach.posOffset);
            /*memcpy(pCurOut, &pThing->attach.posOffset, 12u);
            pCurOut = &sithMulti_g_message.data[22];*/
        }
    }
    else if ( (pThing->attach.flags & SITH_ATTACH_THINGCLIMBWHIP) != 0 )
    {
        SITHDSS_PUSHFLOAT(pThing->attach.posOffset.z);
        /**(float*)pCurOut = pThing->attach.posOffset.z;
        pCurOut = &sithMulti_g_message.data[10];*/
    }

    /*sithMulti_g_message.type = SITHDSS_ATTACHMENT;
    sithMulti_g_message.length = pCurOut - sithMulti_g_message.data;*/
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, sendFlags);
}

int J3DAPI sithDSSThing_ProcessAttachment(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);
    SITHDSS_STARTIN(pMsg);

    //guid = *(uint32_t*)pMsg->data;
    int guid = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
        SITHLOG_ERROR("sithDSSThing_ProcessAttachment: Cannot find GUID for thing index %d.\n", guid);
        return 0;
    }

    //attachflags = *(uint16_t*)&pMsg->data[4];
    SithAttachFlag attachflags = SITHDSS_POPUINT16();

    // uint8_t* pCurIn = &pMsg->data[6];
    if ( (attachflags & (SITH_ATTACH_CLIMBSURFACE | SITH_ATTACH_SURFACE)) != 0 )
    {
        //surfIdx = *(uint16_t*)pCurIn;
        int surfIdx = SITHDSS_POPINT16();
        SithSurface* pSurf = sithSurface_GetSurfaceEx(sithWorld_g_pCurrentWorld, surfIdx);
        if ( !pSurf )
        {
            SITHLOG_ERROR("sithDSSThing_ProcessAttachment: Invalid surface index %d.\n", surfIdx);
            SITHDSS_ENDIN;
            return 0;
        }

        pThing->attach.flags = attachflags;
        pThing->attach.pFace = &pSurf->face;
        pThing->attach.attachedToStructure.pSurfaceAttached = pSurf;
        rdVector_Copy3(&pThing->attach.attachedFaceFirstVert, &sithWorld_g_pCurrentWorld->aVertices[*pSurf->face.aVertices]);
    }
    else if ( (attachflags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0 )
    {
        /*guidAttach = *(uint32_t*)pCurIn;
        pCurIn = &pMsg->data[10];*/

        int guidAttach = SITHDSS_POPINT32();
        SithThing* pThingAttached = sithThing_GetGuidThing(guidAttach);
        if ( !pThingAttached )
        {
            SITHLOG_ERROR("sithDSSThing_ProcessAttachment: Cannot find GUID for thing index %d.\n", guidAttach);
            SITHDSS_ENDIN;
            return 0;
        }

        pThing->attach.attachedToStructure.pThingAttached = pThingAttached;
        pThing->attach.flags = attachflags;
        sithThing_AddThingToAttachedThings(pThing, pThingAttached);

        if ( (attachflags & SITH_ATTACH_THINGFACE) != 0 )
        {
            int faceIdx = SITHDSS_POPINT16();

            // Fixed: Add null check and index bound check
            if ( pThingAttached->renderData.data.pModel3 && faceIdx >= 0 && faceIdx < pThingAttached->renderData.data.pModel3->aGeos[0].aMeshes->numFaces )
            {
                rdFace* pFaceAttached = &pThingAttached->renderData.data.pModel3->aGeos[0].aMeshes->aFaces[faceIdx];
                rdVector_Copy3(&pThing->attach.attachedFaceFirstVert, &pThingAttached->renderData.data.pModel3->aGeos[0].aMeshes->apVertices[*pFaceAttached->aVertices]);
                pThing->attach.pFace = pFaceAttached;
            }
            else // Added
            {
                pThing->attach.flags &= ~SITH_ATTACH_THINGFACE;
                if ( pThing->attach.flags == 0 )
                {
                    pThing->attach.flags |= SITH_ATTACH_THING;
                }
            }
        }
        else
        {
            SITHDSS_POPVEC3(&pThing->attach.posOffset);
            //rdVector_Copy3(&pThing->attach.posOffset, pCurIn, sizeof(pThing->attach.posOffset));
        }
    }
    else if ( (attachflags & SITH_ATTACH_THINGCLIMBWHIP) != 0 )
    {
        pThing->attach.flags = attachflags;
        pThing->attach.posOffset.z = SITHDSS_POPFLOAT();
        //pThing->attach.posOffset.z = *(float*)pCurIn;
    }
    else if ( pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
    }

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSSThing_Take(const SithThing* pItem, const SithThing* pThing, SithMessageStream outstream)
{
    SITHDSS_STARTOUT(SITHDSS_TAKE);
    SITHDSS_PUSHINT32(pItem->guid);
    SITHDSS_PUSHINT32(pThing->guid);

    SITHDSS_ENDOUT;
    if ( stdComm_IsGameHost() )
    {
        return sithDSSThing_ProcessTake(&sithMulti_g_message);
    }

    return sithMessage_SendMessage(&sithMulti_g_message, sithMulti_g_serverId, outstream, DPSEND_GUARANTEED);
}


int J3DAPI sithDSSThing_ProcessTake(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int guid = SITHDSS_POPINT32();
    SithThing* pItem = sithThing_GetGuidThing(guid);
    if ( !pItem && stdComm_IsGameHost() )
    {
        sithDSSThing_DestroyThing(guid, sithMessage_GetSenderID());
        return 0;
    }

    guid = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(guid);
    if ( !pItem || !pThing )
    {
        SITHLOG_ERROR("Bad thing or taker guid in DSS::Take.\n");
        return 0;
    }

    if ( pMsg->type == SITHDSS_TAKE )
    {
        if ( !stdComm_IsGameHost() )
        {
            return 0;
        }

        if ( pItem->type != SITH_THING_ITEM || (pItem->flags & (SITH_TF_DISABLED | SITH_TF_DESTROYED)) != 0 )
        {
            return 1;
        }

        ((SithMessage*)pMsg)->type = SITHDSS_TAKE2;
        sithMessage_SendMessage((SithMessage*)pMsg, (DPID)-1, SITHMESSAGE_STREAM_NET, DPSEND_GUARANTEED);
    }

    sithItem_SetItemTaken(pItem, pThing, 1);

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_CreateThing(const SithThing* pTemplate, const SithThing* pNewThing, const SithThing* pMarker, const SithSector* pSector, const rdVector3* pos, const rdVector3* pyr, SithMessageStream outstream, unsigned int sendFlags)
{
    SITHDSS_STARTOUT(SITHDSS_CREATETHING);
    SITHDSS_PUSHINT16((int16_t)sithTemplate_GetTemplateIndex(pTemplate));

    if ( pMarker )
    {
        SITHDSS_PUSHINT32(pMarker->guid);
    }
    else
    {
        SITH_ASSERTREL(pSector && pos && pyr);
        SITHDSS_PUSHINT32(-1);
        SITHDSS_PUSHINT32(sithSector_GetSectorIndex(pSector));
        SITHDSS_PUSHVEC3(pos);
        SITHDSS_PUSHVEC3(pyr);
    }

    SITHDSS_PUSHINT32(pNewThing->guid);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, (DPID)-1, outstream, sendFlags);
}


int J3DAPI sithDSSThing_ProcessCreateThing(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int id = SITHDSS_POPUINT16();
    SithThing* pTemplate = sithTemplate_GetTemplateByIndex(id);
    if ( !pTemplate )
    {
        SITHLOG_ERROR("Bad template index %d in DSS::CreateThing message.\n", id);
        return 0;
    }

    SithThing* pThing;
    id = SITHDSS_POPINT32();
    if ( id >= 0 )
    {
        SithThing* pMarker = sithThing_GetGuidThing(id);
        if ( !pMarker )
        {
            SITHLOG_ERROR("Cannot find GUID %d to create template %s.\n", id, pTemplate->aName);
            return 0;
        }

        pThing = sithThing_CreateThing(pTemplate, pMarker);
    }
    else
    {
        id = SITHDSS_POPINT32();
        SithSector* pSector = sithSector_GetSectorEx(sithWorld_g_pCurrentWorld, id);
        if ( !pSector )
        {
            SITHLOG_ERROR("Bad sector %d in DSS::CreateThing, template %s.\n", id, pTemplate->aName);
            return 0;
        }

        rdVector3 pos;
        rdVector3 curPyr;
        SITHDSS_POPVEC3(&pos);
        SITHDSS_POPVEC3(&curPyr);

        rdMatrix34 rotation;
        rdMatrix_BuildRotate34(&rotation, &curPyr);
        pThing = sithThing_CreateThingAtPos(pTemplate, &pos, &rotation, pSector, NULL);
    }

    if ( !pThing )
    {
        SITHLOG_ERROR("Failed to create thing of type %s from network instruction.\n", pTemplate->aName);
        return 0;
    }

    pThing->guid = SITHDSS_POPINT32();
    pThing->flags |= SITH_TF_REMOTE;

    SITHDSS_ENDIN;
    return 1;
}


int J3DAPI sithDSSThing_DestroyThing(int guid, DPID idTo)
{
    SITHDSS_STARTOUT(SITHDSS_DESTROYTHING);
    SITHDSS_PUSHINT32(guid);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
}


int J3DAPI sithDSSThing_ProcessDestroyThing(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int guid = SITHDSS_POPINT32();
    SithThing* pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
        SITHLOG_ERROR("Bad thing guid %d in DSS::DestroyThing.\n", guid);
        return 0;
    }

    sithThing_DestroyThing(pThing);

    SITHDSS_ENDIN;
    return 1;
}


void J3DAPI sithDSSThing_MoveToPos(SithThing* pThing, const rdVector3* pPos, SithSector* pSector)
{
    SITH_ASSERTREL(pThing && (pThing->moveType == SITH_MT_PHYSICS) && pSector && pPos);

    // Note: The original evaluates the predicted position minus the current position in x87 extended precision
    //       and rounds to float once when storing the direction. Double precision reproduces that exactly.
    rdVector3 dir;
    dir.x = (float)((double)pThing->moveInfo.physics.velocity.x * 0.25 + (double)pPos->x - (double)pThing->pos.x);
    dir.y = (float)((double)pThing->moveInfo.physics.velocity.y * 0.25 + (double)pPos->y - (double)pThing->pos.y);
    dir.z = (float)((double)pThing->moveInfo.physics.velocity.z * 0.25 + (double)pPos->z - (double)pThing->pos.z);

    float dist = rdVector_Len3(&dir);
    if ( dist != 0.0f && dist < 0.5f )
    {
        rdVector_Scale3(&pThing->moveInfo.physics.velocity, &dir, 4.0f);
    }
    else
    {
        rdVector_Copy3(&pThing->pos, pPos);
        sithThing_SetSector(pThing, pSector, 0);
    }
}