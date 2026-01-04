#include "sithDSSThing.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdPolyline.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithMulti.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>


void sithDSSThing_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(sithDSSThing_Pos);
    // J3D_HOOKFUNC(sithDSSThing_ProcessPos);
    // J3D_HOOKFUNC(sithDSSThing_MovePos);
    // J3D_HOOKFUNC(sithDSSThing_ProcessMovePos);
    // J3D_HOOKFUNC(sithDSSThing_UpdateState);
    // J3D_HOOKFUNC(sithDSSThing_ProcessStateUpdate);
    // J3D_HOOKFUNC(sithDSSThing_PlaySound);
    // J3D_HOOKFUNC(sithDSSThing_ProcessPlaySound);
    // J3D_HOOKFUNC(sithDSSThing_PlaySoundMode);
    // J3D_HOOKFUNC(sithDSSThing_ProcessPlaySoundMode);
    // J3D_HOOKFUNC(sithDSSThing_PlayKey);
    // J3D_HOOKFUNC(sithDSSThing_ProcessPlayKey);
    // J3D_HOOKFUNC(sithDSSThing_PlayKeyMode);
    // J3D_HOOKFUNC(sithDSSThing_ProcessPlayKeyMode);
    // J3D_HOOKFUNC(sithDSSThing_SetModel);
    // J3D_HOOKFUNC(sithDSSThing_ProcessSetModel);
    // J3D_HOOKFUNC(sithDSSThing_StopKey);
    // J3D_HOOKFUNC(sithDSSThing_ProcessStopKey);
    // J3D_HOOKFUNC(sithDSSThing_StopSound);
    // J3D_HOOKFUNC(sithDSSThing_ProcessStopSound);
    // J3D_HOOKFUNC(sithDSSThing_Fire);
    // J3D_HOOKFUNC(sithDSSThing_ProcessFire);
    // J3D_HOOKFUNC(sithDSSThing_Death);
    // J3D_HOOKFUNC(sithDSSThing_ProcessDeath);
    // J3D_HOOKFUNC(sithDSSThing_DamageThing);
    // J3D_HOOKFUNC(sithDSSThing_ProcessDamage);
    J3D_HOOKFUNC(sithDSSThing_ThingFullDescription);
    J3D_HOOKFUNC(sithDSSThing_ProcessThingFullDescription);
    // J3D_HOOKFUNC(sithDSSThing_PathMove);
    // J3D_HOOKFUNC(sithDSSThing_ProcessPathMove);
    J3D_HOOKFUNC(sithDSSThing_Attachment);
    J3D_HOOKFUNC(sithDSSThing_ProcessAttachment);
    // J3D_HOOKFUNC(sithDSSThing_Take);
    // J3D_HOOKFUNC(sithDSSThing_ProcessTake);
    // J3D_HOOKFUNC(sithDSSThing_CreateThing);
    // J3D_HOOKFUNC(sithDSSThing_ProcessCreateThing);
    // J3D_HOOKFUNC(sithDSSThing_DestroyThing);
    // J3D_HOOKFUNC(sithDSSThing_ProcessDestroyThing);
    // J3D_HOOKFUNC(sithDSSThing_MoveToPos);
}

void sithDSSThing_ResetGlobals(void)
{

}

int J3DAPI sithDSSThing_Pos(const SithThing* pThing, DPID toID, unsigned int dpFlags)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_Pos, pThing, toID, dpFlags);
}

int J3DAPI sithDSSThing_ProcessPos(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessPos, pMsg);
}

int J3DAPI sithDSSThing_MovePos(const SithThing* pThing, DPID idTo, unsigned int sendFlags)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_MovePos, pThing, idTo, sendFlags);
}

int J3DAPI sithDSSThing_ProcessMovePos(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessMovePos, pMsg);
}

int J3DAPI sithDSSThing_UpdateState(const SithThing* pThing, DPID toID, SithMessageStream outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_UpdateState, pThing, toID, outstream);
}

int J3DAPI sithDSSThing_ProcessStateUpdate(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessStateUpdate, pMsg);
}

int J3DAPI sithDSSThing_PlaySound(const SithThing* pThing, const rdVector3* pPos, tSoundHandle hSnd, float volume, float pan, SoundPlayFlag playflags, int guid, DPID idTo, SithMessageStream outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_PlaySound, pThing, pPos, hSnd, volume, pan, playflags, guid, idTo, outstream);
}

int J3DAPI sithDSSThing_ProcessPlaySound(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessPlaySound, pMsg);
}

int J3DAPI sithDSSThing_PlaySoundMode(const SithThing* pThing, int16_t mode, int channel, float a4)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_PlaySoundMode, pThing, mode, channel, a4);
}

int J3DAPI sithDSSThing_ProcessPlaySoundMode(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessPlaySoundMode, pMsg);
}

int J3DAPI sithDSSThing_PlayKey(const SithThing* pThing, const rdKeyframe* pKey, rdKeyframeFlags flags, int16_t lo, unsigned int trackGUID, DPID idTo, SithMessageStream outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_PlayKey, pThing, pKey, flags, lo, trackGUID, idTo, outstream);
}

int J3DAPI sithDSSThing_ProcessPlayKey(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessPlayKey, pMsg);
}

int J3DAPI sithDSSThing_PlayKeyMode(const SithThing* pThing, SithPuppetSubMode mode, unsigned int trackGUID, DPID idTo, SithMessageStream outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_PlayKeyMode, pThing, mode, trackGUID, idTo, outstream);
}

int J3DAPI sithDSSThing_ProcessPlayKeyMode(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessPlayKeyMode, pMsg);
}

int J3DAPI sithDSSThing_SetModel(const SithThing* pThing, DPID idTo)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_SetModel, pThing, idTo);
}

int J3DAPI sithDSSThing_ProcessSetModel(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessSetModel, pMsg);
}

int J3DAPI sithDSSThing_StopKey(const SithThing* pThing, int trackGUID, float fadeTime, DPID idTo, SithMessageStream outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_StopKey, pThing, trackGUID, fadeTime, idTo, outstream);
}

int J3DAPI sithDSSThing_ProcessStopKey(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessStopKey, pMsg);
}

int J3DAPI sithDSSThing_StopSound(tSoundChannelHandle hChannel, float secFadeTime, DPID idTo, SithMessageStream outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_StopSound, hChannel, secFadeTime, idTo, outstream);
}

int J3DAPI sithDSSThing_ProcessStopSound(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessStopSound, pMsg);
}

int J3DAPI sithDSSThing_Fire(const SithThing* pShooter, const SithThing* pProjectileTemplate, const rdVector3* fireDir, const rdVector3* firePos, tSoundHandle hFireSnd, uint16_t puppetSubmode, float extra, int16_t projectileFlags, float sedRapidFireTime, int projectileGUID, DPID idTo, SithMessageStream outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_Fire, pShooter, pProjectileTemplate, fireDir, firePos, hFireSnd, puppetSubmode, extra, projectileFlags, sedRapidFireTime, projectileGUID, idTo, outstream);
}

int J3DAPI sithDSSThing_ProcessFire(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessFire, pMsg);
}

int J3DAPI sithDSSThing_Death(const SithThing* pThing, const SithThing* pKiller, char bKillPlayer, DPID to, SithMessageStream outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_Death, pThing, pKiller, bKillPlayer, to, outstream);
}

int J3DAPI sithDSSThing_ProcessDeath(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessDeath, pMsg);
}

int J3DAPI sithDSSThing_DamageThing(const SithThing* pVictim, const SithThing* pPurpetrator, float damage, SithDamageType hitType, DPID idTo, SithMessageStream outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_DamageThing, pVictim, pPurpetrator, damage, hitType, idTo, outstream);
}

int J3DAPI sithDSSThing_ProcessDamage(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessDamage, pMsg);
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
                pPolyline->face.extraLight.alpha = pThing->alpha; // TODO: Is this a but? Why set alpha here?
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

    SithThingType thingType = SITHDSS_POPUINT16();
    if ( thingType == SITH_THING_FREE )
    {
        SITHDSS_ENDIN;
        return 1;
    }

    pThing = &pWorld->aThings[thingIdx]; // Why get thing again?
    sithThing_Reset(pThing);

    // Template and identification
    size_t templateIdx = SITHDSS_POPUINT16(); // Note, don't change type as it will set max unsigned value if negative
    if ( sithTemplate_GetTemplateByIndex(templateIdx) )
    {
        sithThing_SetThingBasedOn(pThing, &pWorld->aThingTemplates[templateIdx]); // TODO: Use sithTemplate_GetTemplateByIndex instead.
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
        int cogIdx   = SITHDSS_POPINT16();
        pThing->pCog = sithCog_GetCogByIndex(cogIdx);

        cogIdx = SITHDSS_POPINT16();
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
            pPath->sizeFrames =  pPath->numFrames;
            pPath->aFrames    = (SithPathFrame*)STDMALLOC(sizeof(SithPathFrame) * pPath->numFrames);

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
    if ( numUsedSwapEntries != -1 )
    {
        for ( int i = 0; i < numUsedSwapEntries; ++i )
        {
            uint32_t entryNum = SITHDSS_POPUINT32();
            int meshNum       = SITHDSS_POPINT32();
            uint16_t modelIdx = SITHDSS_POPINT16();
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
        // TODO: just to be sure, maybe add check for renderData type == model3 as well?
        rdModel3* pModel = sithModel_GetModelByIndex(modelIdx);
        if ( pModel && pThing->renderData.data.pModel3 != pModel )
        {
            sithThing_SetThingModel(pThing, pModel);
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
            pVoiceInfo->voiceHeadInfo.apSoundHeadModels[i] = sithModel_GetModelByIndex(modelIdx);
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
    if ( pThing->type == SITH_THING_POLYLINE )
    {
        // Added: Deserialize polyline new flags field
        pThing->renderData.data.pPolyline->flags = SITHDSS_POPINT32();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    SITHDSS_ENDIN;

    // Added: Sanity check to see if all bytes were consumed from the message
#ifdef J3D_DEBUG
    if ( SITHDSS_CURPOS() != pMsg->length )
    {
        SITHLOG_ERROR("sithDSSThing_ProcessThingFullDescription: Not all data was processed for thing: %s [curpos: %d msglen: %d]\n", pThing->aName, SITHDSS_CURPOS(), pMsg->length);
    }
#endif

    return 1;
}

int J3DAPI sithDSSThing_PathMove(const SithThing* pThing, int16_t frame, float speed, int moveType, DPID idTO, SithMessageStream outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_PathMove, pThing, frame, speed, moveType, idTO, outstream);
}

int J3DAPI sithDSSThing_ProcessPathMove(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessPathMove, pMsg);
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
    return J3D_TRAMPOLINE_CALL(sithDSSThing_Take, pItem, pThing, outstream);
}

int J3DAPI sithDSSThing_ProcessTake(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessTake, pMsg);
}

int J3DAPI sithDSSThing_CreateThing(const SithThing* pTemplate, const SithThing* pNewThing, const SithThing* pMarker, const SithSector* pSector, const rdVector3* pos, const rdVector3* pyr, SithMessageStream outstream, unsigned int sendFlags)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_CreateThing, pTemplate, pNewThing, pMarker, pSector, pos, pyr, outstream, sendFlags);
}

int J3DAPI sithDSSThing_ProcessCreateThing(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessCreateThing, pMsg);
}

int J3DAPI sithDSSThing_DestroyThing(int guid, DPID idTo)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_DestroyThing, guid, idTo);
}

int J3DAPI sithDSSThing_ProcessDestroyThing(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSSThing_ProcessDestroyThing, pMsg);
}

void J3DAPI sithDSSThing_MoveToPos(SithThing* pThing, const rdVector3* pPos, SithSector* pSector)
{
    J3D_TRAMPOLINE_CALL(sithDSSThing_MoveToPos, pThing, pPos, pSector);
}
