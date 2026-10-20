// Stubs and cases for the sithDSSThing.c functions (see sithDSSVec.h).
#ifndef SITHDSSVECTHING_H
#define SITHDSSVECTHING_H

#include "sithDSSVecDSS.h"

// ---------------------------------------------------------------------------------------------------------------
// Stubs of the engine functions the sithDSSThing functions call (see sithDSSThingSeams.def)
// ---------------------------------------------------------------------------------------------------------------
static uint32_t dssv_FB(float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    return u;
}

static SithThing* dssv_pFiredProjectile; // returned by sithWeapon_WeaponFireProjectile
static SithThing* dssv_pCreatedThing;    // returned by sithThing_CreateThing(AtPos)
static int dssv_playKeyResult;           // returned by sithPuppet_PlayKey
static int dssv_playModeResult;          // returned by sithPuppet_PlayMode

// Sound handles: index 1..DSSV_NUMSOUNDS, the index 0 is invalid. Channel handles are 0x3000 + guid for even guids.
#define DSSV_SOUNDHANDLE(idx) (0x2000u + (idx) * 0x10u)

DSSV_STUB size_t J3DAPI dssv_Sound_GetSoundIndex(tSoundHandle hSnd)
{
    if ( hSnd >= DSSV_SOUNDHANDLE(1) && hSnd <= DSSV_SOUNDHANDLE(DSSV_NUMSOUNDS) && ((hSnd - 0x2000u) & 0xFu) == 0 ) { return (hSnd - 0x2000u) >> 4; }
    return 0;
}

DSSV_STUB tSoundHandle J3DAPI dssv_Sound_GetSoundHandle(uint32_t idx) { return (idx >= 1 && idx <= DSSV_NUMSOUNDS) ? DSSV_SOUNDHANDLE(idx) : 0; }
DSSV_STUB const char* J3DAPI dssv_Sound_GetSoundFilename(tSoundHandleType handle) { (void)handle; return "dssv.wav"; }
DSSV_STUB int J3DAPI dssv_Sound_GetChannelGUID(tSoundChannelHandle hChannel) { return hChannel ? (int)(0x700u + hChannel) : 0; }
DSSV_STUB tSoundChannelHandle J3DAPI dssv_sithSoundMixer_GetChannelHandle(int guid) { return (guid >= 0 && (guid & 1) == 0) ? (tSoundChannelHandle)(0x3000 + guid) : 0; }

DSSV_STUB tSoundChannelHandle J3DAPI dssv_sithSoundMixer_PlaySoundThing(tSoundHandle hSnd, const SithThing* pThing, float volume, float minRadius, float maxRadius, SoundPlayFlag playflags)
{
    dssv_Ev(DSSV_EV_PLAYSOUNDTHING, 6, hSnd, DSSV_P(pThing), dssv_FB(volume), dssv_FB(minRadius), dssv_FB(maxRadius), (uint32_t)playflags);
    return 0x3100;
}

DSSV_STUB tSoundChannelHandle J3DAPI dssv_sithSoundMixer_PlaySoundPos(tSoundHandle hSnd, rdVector3* pos, SithSector* pSector, float volume, float minRadius, float maxRadius, SoundPlayFlag playflags)
{
    dssv_Ev(DSSV_EV_PLAYSOUNDPOS, 10, hSnd, dssv_FB(pos->x), dssv_FB(pos->y), dssv_FB(pos->z), DSSV_P(pSector), dssv_FB(volume), dssv_FB(minRadius), dssv_FB(maxRadius), (uint32_t)playflags, 0);
    return 0x3100;
}

DSSV_STUB tSoundChannelHandle J3DAPI dssv_sithSoundMixer_PlaySound(tSoundHandle hSnd, float volume, float pan, SoundPlayFlag playflags)
{
    dssv_Ev(DSSV_EV_PLAYSOUND, 4, hSnd, dssv_FB(volume), dssv_FB(pan), (uint32_t)playflags);
    return 0x3100;
}

DSSV_STUB void J3DAPI dssv_sithSoundMixer_StopSound(tSoundChannelHandle hChannel) { dssv_Ev(DSSV_EV_STOPSOUND, 1, hChannel); }
DSSV_STUB void J3DAPI dssv_sithSoundMixer_FadeVolume(tSoundChannelHandle hChannel, float volume, float secFadeTime) { dssv_Ev(DSSV_EV_FADEVOLUME, 3, hChannel, dssv_FB(volume), dssv_FB(secFadeTime)); }

DSSV_STUB tSoundChannelHandle J3DAPI dssv_sithSoundClass_PlayMode(SithThing* pThing, SithSoundClassMode mode, float selectionRatio)
{
    dssv_Ev(DSSV_EV_SOUNDCLASSMODE, 3, DSSV_P(pThing), (uint32_t)mode, dssv_FB(selectionRatio));
    return 0x3100;
}

DSSV_STUB tSoundChannelHandle J3DAPI dssv_sithSoundClass_PlayModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    dssv_Ev(DSSV_EV_SOUNDCLASSRANDOM, 2, DSSV_P(pThing), (uint32_t)mode);
    return 0x3100;
}

DSSV_STUB int J3DAPI dssv_sithPuppet_PlayKey(rdPuppet* pPuppet, rdKeyframe* pTrack, int lowPriority, int highPriority, rdKeyframeFlags flags, rdPuppetTrackCallback pfCallback)
{
    dssv_Ev(DSSV_EV_PUPPETPLAYKEY, 6, DSSV_P(pPuppet), DSSV_P(pTrack), (uint32_t)lowPriority, (uint32_t)highPriority, (uint32_t)flags, (uint32_t)(uintptr_t)pfCallback);
    return dssv_playKeyResult;
}

DSSV_STUB int J3DAPI dssv_sithPuppet_PlayMode(SithThing* pThing, SithPuppetSubMode submode, rdPuppetTrackCallback pfCallback)
{
    dssv_Ev(DSSV_EV_PUPPETPLAYMODE, 3, DSSV_P(pThing), (uint32_t)submode, (uint32_t)(uintptr_t)pfCallback);
    return dssv_playModeResult;
}

DSSV_STUB int J3DAPI dssv_sithPuppet_StopKey(rdPuppet* pPuppet, int track, float fadeTime)
{
    dssv_Ev(DSSV_EV_PUPPETSTOPKEY, 3, DSSV_P(pPuppet), (uint32_t)track, dssv_FB(fadeTime));
    return 0;
}

DSSV_STUB SithThing* J3DAPI dssv_sithWeapon_WeaponFireProjectile(SithThing* pShooter, const SithThing* pProjectileTemplate, const rdVector3* pFireDir, rdVector3* pFirePos, tSoundHandle hFireSnd, SithPuppetSubMode submode, float extra, SithFireProjectileFlags flags, float secDeltaTime)
{
    dssv_Ev(DSSV_EV_FIREPROJECTILE, 14, DSSV_P(pShooter), DSSV_P(pProjectileTemplate),
        dssv_FB(pFireDir->x), dssv_FB(pFireDir->y), dssv_FB(pFireDir->z), dssv_FB(pFirePos->x), dssv_FB(pFirePos->y), dssv_FB(pFirePos->z),
        hFireSnd, (uint32_t)submode, dssv_FB(extra), (uint32_t)flags, dssv_FB(secDeltaTime), 0);
    return dssv_pFiredProjectile;
}

DSSV_STUB void J3DAPI dssv_sithActor_KillActor(SithThing* pThing, SithThing* pSrcThing, SithDamageType damageType) { dssv_Ev(DSSV_EV_KILLACTOR, 3, DSSV_P(pThing), DSSV_P(pSrcThing), (uint32_t)damageType); }
DSSV_STUB void J3DAPI dssv_sithPlayer_KillPlayer(SithThing* pPlayerThing) { dssv_Ev(DSSV_EV_KILLPLAYER, 1, DSSV_P(pPlayerThing)); }

DSSV_STUB float J3DAPI dssv_sithThing_DamageThing(SithThing* pThing, const SithThing* pPurpetrator, float damage, SithDamageType hitType)
{
    dssv_Ev(DSSV_EV_DAMAGETHING, 4, DSSV_P(pThing), DSSV_P(pPurpetrator), dssv_FB(damage), (uint32_t)hitType);
    return 0.0f;
}

DSSV_STUB void J3DAPI dssv_sithPathMove_MoveToFrame(SithThing* pThing, size_t frame, float velocity) { dssv_Ev(DSSV_EV_PATHMOVETOFRAME, 3, DSSV_P(pThing), (uint32_t)frame, dssv_FB(velocity)); }
DSSV_STUB void J3DAPI dssv_sithPathMove_SkipToFrame(SithThing* pThing, size_t frame, float velocity) { dssv_Ev(DSSV_EV_PATHMOVESKIP, 3, DSSV_P(pThing), (uint32_t)frame, dssv_FB(velocity)); }
DSSV_STUB void J3DAPI dssv_sithPathMove_Finish(SithThing* pThing) { dssv_Ev(DSSV_EV_PATHMOVEFINISH, 1, DSSV_P(pThing)); }
DSSV_STUB void J3DAPI dssv_sithItem_SetItemTaken(SithThing* pItem, const SithThing* pSrcThing, int bNoMultiSync) { dssv_Ev(DSSV_EV_SETITEMTAKEN, 3, DSSV_P(pItem), DSSV_P(pSrcThing), (uint32_t)bNoMultiSync); }

DSSV_STUB SithThing* J3DAPI dssv_sithThing_CreateThing(const SithThing* pTemplate, SithThing* pThing)
{
    dssv_Ev(DSSV_EV_CREATETHING, 2, DSSV_P(pTemplate), DSSV_P(pThing));
    return dssv_pCreatedThing;
}

DSSV_STUB SithThing* J3DAPI dssv_sithThing_CreateThingAtPos(const SithThing* pTemplate, const rdVector3* pos, const rdMatrix34* orient, SithSector* pSector, SithThing* pParent)
{
    dssv_Ev(DSSV_EV_CREATETHINGATPOS, 3, DSSV_P(pTemplate), DSSV_P(pSector), DSSV_P(pParent));
    dssv_EvWords(DSSV_EV_CREATETHINGATPOS, pos, sizeof(*pos));
    dssv_EvWords(DSSV_EV_CREATETHINGATPOS, orient, sizeof(*orient));
    return dssv_pCreatedThing;
}

DSSV_STUB void J3DAPI dssv_sithThing_DestroyThing(SithThing* pThing) { dssv_Ev(DSSV_EV_DESTROYTHING, 1, DSSV_P(pThing)); }
DSSV_STUB void J3DAPI dssv_sithActor_SetHeadPYR(SithThing* pThing, const rdVector3* headAngles) { dssv_Ev(DSSV_EV_SETHEADPYR, 4, DSSV_P(pThing), dssv_FB(headAngles->x), dssv_FB(headAngles->y), dssv_FB(headAngles->z)); }

DSSV_STUB void J3DAPI dssv_sithThing_SetSector(SithThing* pThing, SithSector* pSector, int bNotify)
{
    dssv_Ev(DSSV_EV_SETSECTOR, 3, DSSV_P(pThing), DSSV_P(pSector), (uint32_t)bNotify);
    pThing->pInSector = pSector;
}

DSSV_STUB void J3DAPI dssv_sithThing_DetachThing(SithThing* pThing)
{
    dssv_Ev(DSSV_EV_DETACH, 1, DSSV_P(pThing));
    pThing->attach.flags = 0;
}

DSSV_STUB void J3DAPI dssv_sithThing_ResetSwapList(SithThing* pThing)
{
    dssv_Ev(DSSV_EV_RESETSWAPLIST, 1, DSSV_P(pThing));
    pThing->pSwapList = NULL;
    pThing->numSwapEntries = 0;
}

DSSV_STUB int J3DAPI dssv_sithThing_AddSwapEntry(SithThing* pThing, int meshNum, rdModel3* pSrcModel, int meshNumSrc)
{
    dssv_Ev(DSSV_EV_ADDSWAPENTRY, 4, DSSV_P(pThing), (uint32_t)meshNum, DSSV_P(pSrcModel), (uint32_t)meshNumSrc);
    return 1;
}

DSSV_STUB int J3DAPI dssv_sithThing_SetThingModel(SithThing* pThing, rdModel3* pModel)
{
    dssv_Ev(DSSV_EV_SETTHINGMODEL, 2, DSSV_P(pThing), DSSV_P(pModel));
    pThing->renderData.data.pModel3 = pModel;
    return 1;
}

DSSV_STUB rdModel3* J3DAPI dssv_sithModel_Load(const char* pName, int bSkipDefault)
{
    dssv_Ev(DSSV_EV_MODELLOAD, 2, dssv_Hash(pName), (uint32_t)bSkipDefault);
    for ( unsigned i = 0; i < DSSV_NUMMODELS; ++i )
    {
        if ( strcmp(dssv_arena.models[i].aName, pName) == 0 ) { return &dssv_arena.models[i]; }
    }
    return NULL;
}

DSSV_STUB int J3DAPI dssv_sithThing_ValidateThingPointer(const SithWorld* pWorld, const SithThing* pThing)
{
    if ( !pThing || pThing->idx != pThing - pWorld->aThings || pThing->idx >= 2304 ) { return 0; }
    return 1;
}

static void dssv_PlayerAction(unsigned which, SithThing* pThing) { dssv_Ev(DSSV_EV_PLAYERACTION, 2, which, DSSV_P(pThing)); }
DSSV_STUB void J3DAPI dssv_sithPlayerActions_ClimbPullUp(SithThing* pThing) { dssv_PlayerAction(1, pThing); }
DSSV_STUB void J3DAPI dssv_sithPlayerActions_ClimbOn1m(SithThing* pThing) { dssv_PlayerAction(2, pThing); }
DSSV_STUB void J3DAPI dssv_sithPlayerActions_HopLeft(SithThing* pThing) { dssv_PlayerAction(3, pThing); }
DSSV_STUB void J3DAPI dssv_sithPlayerActions_StrafeRight(SithThing* pThing) { dssv_PlayerAction(4, pThing); }
DSSV_STUB void J3DAPI dssv_sithPlayerActions_StrafeLeft(SithThing* pThing) { dssv_PlayerAction(5, pThing); }
DSSV_STUB void J3DAPI dssv_sithPlayerActions_HopBack(SithThing* pThing) { dssv_PlayerAction(6, pThing); }
DSSV_STUB void J3DAPI dssv_sithPlayerActions_JumpStart(SithThing* pThing) { dssv_PlayerAction(7, pThing); }

// Matrix <-> angles: replaced with simple deterministic formulas, the rotation math itself is not under test here
DSSV_STUB void J3DAPI dssv_rdMatrix_BuildRotate34(rdMatrix34* mat, const rdVector3* pyr)
{
    mat->rvec.x = pyr->x;     mat->rvec.y = pyr->y;     mat->rvec.z = pyr->z;
    mat->lvec.x = pyr->y;     mat->lvec.y = pyr->z;     mat->lvec.z = pyr->x;
    mat->uvec.x = pyr->z;     mat->uvec.y = pyr->x;     mat->uvec.z = pyr->y;
    mat->dvec.x = 0.0f;       mat->dvec.y = 0.0f;       mat->dvec.z = 0.0f;
}

DSSV_STUB void J3DAPI dssv_rdMatrix_ExtractAngles34(const rdMatrix34* mat, rdVector3* pyr)
{
    pyr->x = mat->rvec.x + mat->lvec.y * 2.0f;
    pyr->y = mat->lvec.z - mat->uvec.x * 0.5f;
    pyr->z = mat->uvec.y + mat->dvec.z + mat->rvec.z * 4.0f;
}


// ---------------------------------------------------------------------------------------------------------------
// Cases
// ---------------------------------------------------------------------------------------------------------------
#define DSSV_T(i) (&DSSV_A->things[i])
#define DSSV_MSGTYPE 0xFFFFu // DssvMut offset that sets the type of the message instead of payload bytes

static float dssv_MsgFloat(const DssvCtx* cx, unsigned off) { float f; memcpy(&f, &cx->inMsg.data[off], 4); return f; }
static int dssv_MsgInt(const DssvCtx* cx, unsigned off) { int v; memcpy(&v, &cx->inMsg.data[off], 4); return v; }

static void dssv_SetSwapList(unsigned thingIdx, unsigned numEntries)
{
    SithThing* t = DSSV_T(thingIdx);
    t->pSwapList = NULL;
    for ( unsigned k = numEntries; k > 0; --k )
    {
        SithThingSwapEntry* e = &DSSV_A->swaps[thingIdx][k - 1];
        e->entryNum  = (int)(100 + k);
        e->meshNum   = (int)(3 + k);
        e->pSrcModel = &DSSV_A->models[k % DSSV_NUMMODELS];
        e->srcMeshNum = (int)(7 * k);
        e->pNextEntry = t->pSwapList;
        t->pSwapList  = e;
    }
    t->numSwapEntries = numEntries;
}

static void dssv_SetBackpack(unsigned thingIdx, unsigned numItems)
{
    SithItemInfo* p = &DSSV_T(thingIdx)->thingInfo.itemInfo;
    p->flags |= SITH_ITEM_BACKPACK;
    p->numItems = (int16_t)numItems;
    for ( unsigned k = 0; k < 32; ++k )
    {
        p->aBackpackItems[k].typeId = (int16_t)(10 + k * 3);
        p->aBackpackItems[k].amount = dssv_RndF();
    }
}

// --- Pos / MovePos -----------------------------------------------------------------------------------------------
// var: thing 0 player, 1 actor, 2 item (no physics), 4 path mover; 6 free thing (rejected), 8 NULL, 9 thing without sector
static int dssvc_Pos(DssvCtx* cx)
{
    SithThing* t = DSSV_VAR == 8 ? NULL : DSSV_T(DSSV_VAR == 9 ? 0 : DSSV_VAR);
    if ( DSSV_VAR == 9 ) { t->pInSector = NULL; }
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_Pos)(t, DSSV_IDTO, 0x10 + DSSV_VAR);
}

// var 1: world is NULL; 2: moves thing close to the received position (prediction distance 0.25); 3: exactly onto it
static int dssvc_ProcessPos(DssvCtx* cx)
{
    if ( DSSV_VAR == 2 || DSSV_VAR == 3 )
    {
        // Message: guid i32, attach u16, sector u16, pos, pyr, physics flags, velocity
        SithThing* t = DSSV_T(0);
        rdVector3 predicted;
        predicted.x = dssv_MsgFloat(cx, 36) * 0.25f + dssv_MsgFloat(cx, 8);
        predicted.y = dssv_MsgFloat(cx, 40) * 0.25f + dssv_MsgFloat(cx, 12);
        predicted.z = dssv_MsgFloat(cx, 44) * 0.25f + dssv_MsgFloat(cx, 16);
        t->pos = predicted;
        if ( DSSV_VAR == 2 ) { t->pos.x -= 0.25f; }
    }
    if ( DSSV_VAR == 1 ) { sithWorld_g_pCurrentWorld = NULL; }
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessPos)(DSSV_MSG);
}

static int dssvc_MovePos(DssvCtx* cx)
{
    SithThing* t = DSSV_VAR == 8 ? NULL : DSSV_T(DSSV_VAR);
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_MovePos)(t, DSSV_IDTO, 0x10 + DSSV_VAR);
}

static int dssvc_ProcessMovePos(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessMovePos)(DSSV_MSG);
}

// --- UpdateState -------------------------------------------------------------------------------------------------
// var: 0 player, 1 actor, 2 item, 3 item with backpack, 4 weapon, 5 corpse, 6 sprite, 7 actor with swap list, 8 free thing,
//      9 NULL, 10 thing index not matching its position, 11 player without puppet class
static int dssvc_UpdateState(DssvCtx* cx)
{
    static const unsigned things[] = { 0, 1, 2, 2, 3, 7, 5, 1, 6, 0, 3, 0 };
    SithThing* t = DSSV_VAR == 9 ? NULL : DSSV_T(things[DSSV_VAR]);
    if ( DSSV_VAR == 3 ) { dssv_SetBackpack(2, 3); }
    if ( DSSV_VAR == 7 ) { dssv_SetSwapList(1, 2); }
    if ( DSSV_VAR == 10 ) { t->idx = 9; }
    if ( DSSV_VAR == 11 ) { t->pPuppetClass = NULL; }
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_UpdateState)(t, DSSV_IDTO, DSSV_STREAM);
}

// var 1: the target thing is marked remote; 2: has a swap list
static int dssvc_ProcessStateUpdate(DssvCtx* cx)
{
    if ( DSSV_VAR == 1 ) { for ( unsigned i = 0; i < 8; ++i ) { DSSV_T(i)->flags |= SITH_TF_REMOTE; } }
    if ( DSSV_VAR == 2 ) { dssv_SetSwapList(1, 1); dssv_SetSwapList(0, 1); }
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessStateUpdate)(DSSV_MSG);
}

// --- Sounds ------------------------------------------------------------------------------------------------------
// var 0: thing position, 1: absolute position, 2: neither
static int dssvc_PlaySound(DssvCtx* cx)
{
    static const SoundPlayFlag flags[3] = { SOUNDPLAY_THING_POS | SOUNDPLAY_LOOP, SOUNDPLAY_ABSOLUTE_POS | SOUNDPLAY_HIGH_PRIORITY, SOUNDPLAY_PLAYONCE };
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_PlaySound)(DSSV_T(2), &DSSV_A->aiFrames[1], DSSV_SOUNDHANDLE(3), 0.75f, -0.25f, flags[DSSV_VAR], 0x4242, DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessPlaySound(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessPlaySound)(DSSV_MSG);
}

// var 0: negative selection ratio, 1: positive
static int dssvc_PlaySoundMode(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_PlaySoundMode)(DSSV_T(1), (int16_t)(138 + DSSV_VAR), 0x1234 + DSSV_VAR, DSSV_VAR == 0 ? -1.0f : 0.5f);
}

static int dssvc_ProcessPlaySoundMode(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessPlaySoundMode)(DSSV_MSG);
}

// var 0: a sound channel, 1: no channel
static int dssvc_StopSound(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_StopSound)(DSSV_VAR == 0 ? 0x3802u : 0u, 1.5f, DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessStopSound(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessStopSound)(DSSV_MSG);
}

// --- Keys and puppets --------------------------------------------------------------------------------------------
static int dssvc_PlayKey(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_PlayKey)(DSSV_T(0), &DSSV_A->keys[2], (rdKeyframeFlags)(0x11 + DSSV_VAR), (int16_t)(3 + DSSV_VAR), 0xA0A0u + DSSV_VAR, DSSV_IDTO, DSSV_STREAM);
}

// var: result of sithPuppet_PlayKey, 0 is track 0, -1 is failure
static int dssvc_ProcessPlayKey(DssvCtx* cx)
{
    dssv_playKeyResult = DSSV_VAR == 0 ? 3 : -1;
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessPlayKey)(DSSV_MSG);
}

static int dssvc_PlayKeyMode(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_PlayKeyMode)(DSSV_T(1), (SithPuppetSubMode)(20 + DSSV_VAR), 0xB0B0u + DSSV_VAR, DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessPlayKeyMode(DssvCtx* cx)
{
    dssv_playModeResult = DSSV_VAR == 0 ? 5 : -1;
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessPlayKeyMode)(DSSV_MSG);
}

// var 0: model thing, 1: thing without model3 (rejected), 2: NULL
static int dssvc_SetModel(DssvCtx* cx)
{
    SithThing* t = DSSV_VAR == 2 ? NULL : DSSV_T(DSSV_VAR == 0 ? 0 : 5);
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_SetModel)(t, DSSV_IDTO);
}

static int dssvc_ProcessSetModel(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessSetModel)(DSSV_MSG);
}

static int dssvc_StopKey(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_StopKey)(DSSV_T(0), (int)(0xC0C0 + DSSV_VAR), 0.5f + (float)DSSV_VAR, DSSV_IDTO, DSSV_STREAM);
}

// var 0: a track has the guid of the message, 1: none has
static int dssvc_ProcessStopKey(DssvCtx* cx)
{
    if ( DSSV_VAR == 0 ) { DSSV_T(0)->renderData.pPuppet->aTracks[5].guid = (uint32_t)dssv_MsgInt(cx, 4); }
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessStopKey)(DSSV_MSG);
}

// --- Fire --------------------------------------------------------------------------------------------------------
// var 0: with fire sound, 1: without
static int dssvc_Fire(DssvCtx* cx)
{
    rdVector3 dir = { 0.25f, -0.5f, 0.75f };
    rdVector3 pos = { 10.5f, -20.25f, 30.125f };
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_Fire)(DSSV_T(0), &DSSV_A->templates[1], &dir, &pos, DSSV_VAR == 0 ? DSSV_SOUNDHANDLE(4) : 0u, 31, 1.5f, 0x21, 0.125f, 0x5151 + DSSV_VAR, DSSV_IDTO, DSSV_STREAM);
}

// var 0: a projectile is created, 1: none
static int dssvc_ProcessFire(DssvCtx* cx)
{
    dssv_pFiredProjectile = DSSV_VAR == 0 ? DSSV_T(5) : NULL;
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessFire)(DSSV_MSG);
}

// --- Death and damage --------------------------------------------------------------------------------------------
// var: 0 actor with killer, 1 actor without killer, 2 player kill, 3 player no kill, 4 item
static int dssvc_Death(DssvCtx* cx)
{
    static const unsigned things[] = { 1, 1, 0, 0, 2 };
    SithThing* killer = DSSV_VAR == 1 ? NULL : DSSV_T(3);
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_Death)(DSSV_T(things[DSSV_VAR]), killer, (char)(DSSV_VAR == 2 ? 1 : 0), DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessDeath(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessDeath)(DSSV_MSG);
}

// var 0: with perpetrator, 1: without
static int dssvc_DamageThing(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_DamageThing)(DSSV_T(1), DSSV_VAR == 0 ? DSSV_T(3) : NULL, 12.5f, (SithDamageType)(DSSV_VAR == 0 ? SITH_DAMAGE_FIRE : SITH_DAMAGE_WHIP), DSSV_IDTO, DSSV_STREAM);
}

// var 0: the message comes from the server, 1: from somebody else
static int dssvc_ProcessDamage(DssvCtx* cx)
{
    dssv_senderId = DSSV_VAR == 0 ? sithMulti_g_serverId : 0x7777;
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessDamage)(DSSV_MSG);
}

// --- Path movement -----------------------------------------------------------------------------------------------
// var: move type 0, 1, 2 for thing 4; 3 thing 0 (not a path mover, rejected); 4 NULL
static int dssvc_PathMove(DssvCtx* cx)
{
    SithThing* t = DSSV_VAR == 4 ? NULL : DSSV_T(DSSV_VAR == 3 ? 0 : 4);
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_PathMove)(t, 2, 3.5f, DSSV_VAR < 3 ? DSSV_VAR : 0, DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessPathMove(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessPathMove)(DSSV_MSG);
}

// --- Take, create and destroy ------------------------------------------------------------------------------------
// var 0: client (message is sent to the server), 1: host (message is processed locally)
static int dssvc_Take(DssvCtx* cx)
{
    dssv_bGameHost = DSSV_VAR;
    dssv_senderId = 0x7003;
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_Take)(DSSV_T(2), DSSV_T(0), DSSV_STREAM);
}

// var bit 0: host, bit 1: the item is disabled
static int dssvc_ProcessTake(DssvCtx* cx)
{
    dssv_bGameHost = DSSV_VAR & 1;
    dssv_senderId = 0x7003;
    if ( DSSV_VAR & 2 ) { DSSV_T(2)->flags |= SITH_TF_DISABLED; }
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessTake)(DSSV_MSG);
}

// var 0: at a marker, 1: at a position
static int dssvc_CreateThing(DssvCtx* cx)
{
    rdVector3 pos = { 1.5f, 2.5f, -3.5f };
    rdVector3 pyr = { 10.0f, 20.0f, -30.0f };
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_CreateThing)(&DSSV_A->templates[2], DSSV_T(5), DSSV_VAR == 0 ? DSSV_T(3) : NULL, &DSSV_A->sectors[2], &pos, &pyr, DSSV_STREAM, 0x20 + DSSV_VAR);
}

// var 1: the engine fails to create the thing
static int dssvc_ProcessCreateThing(DssvCtx* cx)
{
    dssv_pCreatedThing = DSSV_VAR == 1 ? NULL : DSSV_T(5);
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessCreateThing)(DSSV_MSG);
}

static int dssvc_DestroyThing(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_DestroyThing)(DSSV_GUID(DSSV_VAR), DSSV_IDTO);
}

static int dssvc_ProcessDestroyThing(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSSThing_ProcessDestroyThing)(DSSV_MSG);
}

// var 0: prediction closer than 0.5 but not on the thing, 1: exactly on the thing, 2: far away;
// 3..5: like 0 and 2 with values that are not exactly representable (rounding of the float arithmetic)
static int dssvc_MoveToPos(DssvCtx* cx)
{
    SithThing* t = DSSV_T(0);
    rdVector3 target = { 100.5f, -50.25f, 8.0f };
    rdVector3 off = { 0.0f, -0.375f, 0.0f };
    if ( DSSV_VAR >= 3 )
    {
        target.x = 100.123f; target.y = -50.987f; target.z = 8.3f;
        t->moveInfo.physics.velocity.x = 1.7f; t->moveInfo.physics.velocity.y = -2.3f; t->moveInfo.physics.velocity.z = 0.9f;
        off.x = 0.1f; off.y = -0.2f; off.z = 0.05f;
        if ( DSSV_VAR == 4 ) { off.x = 0.3f; off.y = 0.3f; off.z = 0.3f; }
        if ( DSSV_VAR == 5 ) { off.x = 0.2f; off.y = 0.2f; off.z = 0.2f; }
    }

    rdVector3 predicted;
    predicted.x = t->moveInfo.physics.velocity.x * 0.25f + target.x;
    predicted.y = t->moveInfo.physics.velocity.y * 0.25f + target.y;
    predicted.z = t->moveInfo.physics.velocity.z * 0.25f + target.z;
    t->pos = predicted;
    if ( DSSV_VAR == 0 ) { t->pos.y -= 0.375f; }
    if ( DSSV_VAR == 2 ) { t->pos.z += 20.0f; }
    if ( DSSV_VAR >= 3 ) { t->pos.x += off.x; t->pos.y += off.y; t->pos.z += off.z; }
    dssv_Begin(cx);
    DSSV_FN(sithDSSThing_MoveToPos)(t, &target, &DSSV_A->sectors[1]);
    return 0;
}

#define DSSV_NOMUT { 0 }
static const DssvCase dssv_aThingCases[] = {
    // Pos: the thing kinds that add data (physics, player), things that are rejected
    { "Pos_player", dssvc_Pos, 0, NULL, DSSV_NOMUT },
    { "Pos_actor", dssvc_Pos, 1, NULL, DSSV_NOMUT },
    { "Pos_item_nophysics", dssvc_Pos, 2, NULL, DSSV_NOMUT },
    { "Pos_pathmover", dssvc_Pos, 4, NULL, DSSV_NOMUT },
    { "Pos_freething", dssvc_Pos, 6, NULL, DSSV_NOMUT },
    { "Pos_null", dssvc_Pos, 8, NULL, DSSV_NOMUT },
    { "Pos_nosector", dssvc_Pos, 9, NULL, DSSV_NOMUT },
    { "ProcessPos_player", dssvc_ProcessPos, 0, "Pos_player", DSSV_NOMUT },
    { "ProcessPos_actor", dssvc_ProcessPos, 0, "Pos_actor", DSSV_NOMUT },
    { "ProcessPos_item_nophysics", dssvc_ProcessPos, 0, "Pos_item_nophysics", DSSV_NOMUT },
    { "ProcessPos_pathmover", dssvc_ProcessPos, 0, "Pos_pathmover", DSSV_NOMUT },
    // Attachment flags zero in the message detach the thing
    { "ProcessPos_detach", dssvc_ProcessPos, 0, "Pos_actor", { { 4, 2, 0 } } },
    { "ProcessPos_keepattached", dssvc_ProcessPos, 0, "Pos_actor", { { 4, 2, 0x300 } } },
    // Physics things within 0.5 of the received position slide instead of snapping
    { "ProcessPos_physics_close", dssvc_ProcessPos, 2, "Pos_player", DSSV_NOMUT },
    { "ProcessPos_physics_onthing", dssvc_ProcessPos, 3, "Pos_player", DSSV_NOMUT },
    { "ProcessPos_noworld", dssvc_ProcessPos, 1, "Pos_player", DSSV_NOMUT },
    { "ProcessPos_badguid", dssvc_ProcessPos, 0, "Pos_player", { { 0, 4, 0x7777 } } },
    { "ProcessPos_freeguid", dssvc_ProcessPos, 0, "Pos_player", { { 0, 4, 0x1066 } } },
    { "ProcessPos_badsector", dssvc_ProcessPos, 0, "Pos_player", { { 6, 2, 9 } } },
    { "MovePos_player", dssvc_MovePos, 0, NULL, DSSV_NOMUT },
    { "MovePos_actor", dssvc_MovePos, 1, NULL, DSSV_NOMUT },
    { "MovePos_path", dssvc_MovePos, 4, NULL, DSSV_NOMUT },
    { "MovePos_freething", dssvc_MovePos, 6, NULL, DSSV_NOMUT },
    { "MovePos_null", dssvc_MovePos, 8, NULL, DSSV_NOMUT },
    { "ProcessMovePos_still", dssvc_ProcessMovePos, 0, "MovePos_player", { { 30, 4, 0 } } },
    { "ProcessMovePos_pushing", dssvc_ProcessMovePos, 0, "MovePos_player", { { 30, 4, 7 } } },
    { "ProcessMovePos_pullingup", dssvc_ProcessMovePos, 0, "MovePos_player", { { 30, 4, 26 } } },
    { "ProcessMovePos_pullingup1m", dssvc_ProcessMovePos, 0, "MovePos_player", { { 30, 4, 27 } } },
    { "ProcessMovePos_unknown60", dssvc_ProcessMovePos, 0, "MovePos_player", { { 30, 4, 60 } } },
    { "ProcessMovePos_strafe_right", dssvc_ProcessMovePos, 0, "MovePos_player", { { 30, 4, 58 } } },
    { "ProcessMovePos_strafe_left", dssvc_ProcessMovePos, 0, "MovePos_player", { { 30, 4, 57 } } },
    { "ProcessMovePos_jumpback", dssvc_ProcessMovePos, 0, "MovePos_player", { { 30, 4, 61 } } },
    { "ProcessMovePos_jumpready", dssvc_ProcessMovePos, 0, "MovePos_player", { { 30, 4, 18 } } },
    { "ProcessMovePos_other", dssvc_ProcessMovePos, 0, "MovePos_actor", { { 30, 4, 99 } } },
    { "ProcessMovePos_badguid", dssvc_ProcessMovePos, 0, "MovePos_actor", { { 0, 4, 0x7777 } } },
    { "ProcessMovePos_badsector", dssvc_ProcessMovePos, 0, "MovePos_actor", { { 4, 2, 77 } } },

    // State update
    { "UpdateState_player", dssvc_UpdateState, 0, NULL, DSSV_NOMUT },
    { "UpdateState_actor", dssvc_UpdateState, 1, NULL, DSSV_NOMUT },
    { "UpdateState_item", dssvc_UpdateState, 2, NULL, DSSV_NOMUT },
    { "UpdateState_item_backpack", dssvc_UpdateState, 3, NULL, DSSV_NOMUT },
    { "UpdateState_weapon", dssvc_UpdateState, 4, NULL, DSSV_NOMUT },
    { "UpdateState_corpse", dssvc_UpdateState, 5, NULL, DSSV_NOMUT },
    { "UpdateState_sprite", dssvc_UpdateState, 6, NULL, DSSV_NOMUT },
    { "UpdateState_actor_swaplist", dssvc_UpdateState, 7, NULL, DSSV_NOMUT },
    { "UpdateState_freething", dssvc_UpdateState, 8, NULL, DSSV_NOMUT },
    { "UpdateState_null", dssvc_UpdateState, 9, NULL, DSSV_NOMUT },
    { "UpdateState_badthingindex", dssvc_UpdateState, 10, NULL, DSSV_NOMUT },
    { "UpdateState_player_nopuppet", dssvc_UpdateState, 11, NULL, DSSV_NOMUT },
    { "ProcessStateUpdate_player", dssvc_ProcessStateUpdate, 0, "UpdateState_player", DSSV_NOMUT },
    { "ProcessStateUpdate_actor", dssvc_ProcessStateUpdate, 0, "UpdateState_actor", DSSV_NOMUT },
    { "ProcessStateUpdate_item", dssvc_ProcessStateUpdate, 0, "UpdateState_item", DSSV_NOMUT },
    { "ProcessStateUpdate_item_backpack", dssvc_ProcessStateUpdate, 0, "UpdateState_item_backpack", DSSV_NOMUT },
    { "ProcessStateUpdate_weapon", dssvc_ProcessStateUpdate, 0, "UpdateState_weapon", DSSV_NOMUT },
    { "ProcessStateUpdate_corpse", dssvc_ProcessStateUpdate, 0, "UpdateState_corpse", DSSV_NOMUT },
    { "ProcessStateUpdate_sprite", dssvc_ProcessStateUpdate, 0, "UpdateState_sprite", DSSV_NOMUT },
    { "ProcessStateUpdate_actor_swaplist", dssvc_ProcessStateUpdate, 0, "UpdateState_actor_swaplist", DSSV_NOMUT },
    { "ProcessStateUpdate_swaplist_replaces", dssvc_ProcessStateUpdate, 2, "UpdateState_actor_swaplist", DSSV_NOMUT },
    { "ProcessStateUpdate_remote", dssvc_ProcessStateUpdate, 1, "UpdateState_actor", DSSV_NOMUT },
    { "ProcessStateUpdate_badguid", dssvc_ProcessStateUpdate, 0, "UpdateState_actor", { { 0, 4, 0x7777 } } },
    { "ProcessStateUpdate_badsector", dssvc_ProcessStateUpdate, 0, "UpdateState_actor", { { 12, 2, 9 } } },

    // Sounds
    { "PlaySound_thing", dssvc_PlaySound, 0, NULL, DSSV_NOMUT },
    { "PlaySound_position", dssvc_PlaySound, 1, NULL, DSSV_NOMUT },
    { "PlaySound_plain", dssvc_PlaySound, 2, NULL, DSSV_NOMUT },
    { "ProcessPlaySound_thing", dssvc_ProcessPlaySound, 0, "PlaySound_thing", DSSV_NOMUT },
    { "ProcessPlaySound_position", dssvc_ProcessPlaySound, 0, "PlaySound_position", DSSV_NOMUT },
    { "ProcessPlaySound_plain", dssvc_ProcessPlaySound, 0, "PlaySound_plain", DSSV_NOMUT },
    { "ProcessPlaySound_badsound", dssvc_ProcessPlaySound, 0, "PlaySound_thing", { { 12, 2, 9 } } },
    { "ProcessPlaySound_badthing", dssvc_ProcessPlaySound, 0, "PlaySound_thing", { { 14, 4, 0x7777 } } },
    { "PlaySoundMode_random", dssvc_PlaySoundMode, 0, NULL, DSSV_NOMUT },
    { "PlaySoundMode_ratio", dssvc_PlaySoundMode, 1, NULL, DSSV_NOMUT },
    { "ProcessPlaySoundMode_random", dssvc_ProcessPlaySoundMode, 0, "PlaySoundMode_random", DSSV_NOMUT },
    { "ProcessPlaySoundMode_ratio", dssvc_ProcessPlaySoundMode, 0, "PlaySoundMode_ratio", DSSV_NOMUT },
    { "ProcessPlaySoundMode_badthing", dssvc_ProcessPlaySoundMode, 0, "PlaySoundMode_ratio", { { 0, 4, 0x7777 } } },
    { "StopSound_channel", dssvc_StopSound, 0, NULL, DSSV_NOMUT },
    { "StopSound_nochannel", dssvc_StopSound, 1, NULL, DSSV_NOMUT },
    { "ProcessStopSound_fade", dssvc_ProcessStopSound, 0, "StopSound_channel", DSSV_NOMUT },
    { "ProcessStopSound_stop", dssvc_ProcessStopSound, 0, "StopSound_channel", { { 4, 4, 0 } } },
    { "ProcessStopSound_stop_negative", dssvc_ProcessStopSound, 0, "StopSound_channel", { { 4, 4, 0xBF800000 } } },
    { "ProcessStopSound_nochannel", dssvc_ProcessStopSound, 0, "StopSound_nochannel", DSSV_NOMUT },

    // Keys and puppets
    { "PlayKey_a", dssvc_PlayKey, 0, NULL, DSSV_NOMUT },
    { "PlayKey_b", dssvc_PlayKey, 5, NULL, DSSV_NOMUT },
    { "ProcessPlayKey_track", dssvc_ProcessPlayKey, 0, "PlayKey_a", DSSV_NOMUT },
    { "ProcessPlayKey_track_lowpriority", dssvc_ProcessPlayKey, 0, "PlayKey_b", DSSV_NOMUT },
    { "ProcessPlayKey_failed", dssvc_ProcessPlayKey, 1, "PlayKey_a", DSSV_NOMUT },
    { "ProcessPlayKey_badthing", dssvc_ProcessPlayKey, 0, "PlayKey_a", { { 0, 4, 0x7777 } } },
    { "ProcessPlayKey_nopuppet", dssvc_ProcessPlayKey, 0, "PlayKey_a", { { 0, 4, 0x1022 } } },
    { "ProcessPlayKey_badkey", dssvc_ProcessPlayKey, 0, "PlayKey_a", { { 4, 4, 77 } } },
    { "PlayKeyMode_a", dssvc_PlayKeyMode, 0, NULL, DSSV_NOMUT },
    { "PlayKeyMode_b", dssvc_PlayKeyMode, 3, NULL, DSSV_NOMUT },
    { "ProcessPlayKeyMode_track", dssvc_ProcessPlayKeyMode, 0, "PlayKeyMode_a", DSSV_NOMUT },
    { "ProcessPlayKeyMode_track_b", dssvc_ProcessPlayKeyMode, 0, "PlayKeyMode_b", DSSV_NOMUT },
    { "ProcessPlayKeyMode_failed", dssvc_ProcessPlayKeyMode, 1, "PlayKeyMode_a", DSSV_NOMUT },
    { "ProcessPlayKeyMode_badthing", dssvc_ProcessPlayKeyMode, 0, "PlayKeyMode_a", { { 0, 4, 0x7777 } } },
    { "ProcessPlayKeyMode_nopuppet", dssvc_ProcessPlayKeyMode, 0, "PlayKeyMode_a", { { 0, 4, 0x1022 } } },
    { "SetModel_model", dssvc_SetModel, 0, NULL, DSSV_NOMUT },
    { "SetModel_nomodel", dssvc_SetModel, 1, NULL, DSSV_NOMUT },
    { "SetModel_null", dssvc_SetModel, 2, NULL, DSSV_NOMUT },
    { "ProcessSetModel_model", dssvc_ProcessSetModel, 0, "SetModel_model", DSSV_NOMUT },
    { "ProcessSetModel_unknownmodel", dssvc_ProcessSetModel, 0, "SetModel_model", { { 4, 4, 0x78787878 } } },
    { "ProcessSetModel_badthing", dssvc_ProcessSetModel, 0, "SetModel_model", { { 0, 4, 0x7777 } } },
    { "StopKey_a", dssvc_StopKey, 0, NULL, DSSV_NOMUT },
    { "StopKey_b", dssvc_StopKey, 2, NULL, DSSV_NOMUT },
    { "ProcessStopKey_found", dssvc_ProcessStopKey, 0, "StopKey_a", DSSV_NOMUT },
    { "ProcessStopKey_notfound", dssvc_ProcessStopKey, 1, "StopKey_b", DSSV_NOMUT },
    { "ProcessStopKey_badthing", dssvc_ProcessStopKey, 1, "StopKey_a", { { 0, 4, 0x7777 } } },
    { "ProcessStopKey_nopuppet", dssvc_ProcessStopKey, 1, "StopKey_a", { { 0, 4, 0x1022 } } },

    // Fire
    { "Fire_sound", dssvc_Fire, 0, NULL, DSSV_NOMUT },
    { "Fire_nosound", dssvc_Fire, 1, NULL, DSSV_NOMUT },
    { "ProcessFire_projectile", dssvc_ProcessFire, 0, "Fire_sound", DSSV_NOMUT },
    { "ProcessFire_nosound", dssvc_ProcessFire, 0, "Fire_nosound", DSSV_NOMUT },
    { "ProcessFire_noprojectile", dssvc_ProcessFire, 1, "Fire_sound", DSSV_NOMUT },
    { "ProcessFire_badshooter", dssvc_ProcessFire, 0, "Fire_sound", { { 0, 4, 0x7777 } } },

    // Death and damage
    { "Death_actor_killer", dssvc_Death, 0, NULL, DSSV_NOMUT },
    { "Death_actor_nokiller", dssvc_Death, 1, NULL, DSSV_NOMUT },
    { "Death_player_kill", dssvc_Death, 2, NULL, DSSV_NOMUT },
    { "Death_player_nokill", dssvc_Death, 3, NULL, DSSV_NOMUT },
    { "Death_item", dssvc_Death, 4, NULL, DSSV_NOMUT },
    { "ProcessDeath_actor_killer", dssvc_ProcessDeath, 0, "Death_actor_killer", DSSV_NOMUT },
    { "ProcessDeath_actor_nokiller", dssvc_ProcessDeath, 0, "Death_actor_nokiller", DSSV_NOMUT },
    { "ProcessDeath_player_kill", dssvc_ProcessDeath, 0, "Death_player_kill", DSSV_NOMUT },
    { "ProcessDeath_player_nokill", dssvc_ProcessDeath, 0, "Death_player_nokill", DSSV_NOMUT },
    { "ProcessDeath_item", dssvc_ProcessDeath, 0, "Death_item", DSSV_NOMUT },
    { "ProcessDeath_badthing", dssvc_ProcessDeath, 0, "Death_actor_killer", { { 0, 4, 0x7777 } } },
    { "DamageThing_perp", dssvc_DamageThing, 0, NULL, DSSV_NOMUT },
    { "DamageThing_noperp", dssvc_DamageThing, 1, NULL, DSSV_NOMUT },
    { "ProcessDamage_perp", dssvc_ProcessDamage, 0, "DamageThing_perp", DSSV_NOMUT },
    { "ProcessDamage_noperp", dssvc_ProcessDamage, 0, "DamageThing_noperp", DSSV_NOMUT },
    { "ProcessDamage_notfromserver", dssvc_ProcessDamage, 1, "DamageThing_perp", DSSV_NOMUT },
    { "ProcessDamage_badthing", dssvc_ProcessDamage, 0, "DamageThing_perp", { { 0, 4, 0x7777 } } },

    // Path movement
    { "PathMove_moveto", dssvc_PathMove, 0, NULL, DSSV_NOMUT },
    { "PathMove_skipto", dssvc_PathMove, 1, NULL, DSSV_NOMUT },
    { "PathMove_finish", dssvc_PathMove, 2, NULL, DSSV_NOMUT },
    { "PathMove_notpath", dssvc_PathMove, 3, NULL, DSSV_NOMUT },
    { "PathMove_null", dssvc_PathMove, 4, NULL, DSSV_NOMUT },
    { "ProcessPathMove_moveto", dssvc_ProcessPathMove, 0, "PathMove_moveto", { { 8, 4, 0x00FFFFFF } } },
    { "ProcessPathMove_skipto", dssvc_ProcessPathMove, 0, "PathMove_skipto", { { 8, 4, 0x00FFFFFF } } },
    { "ProcessPathMove_finish", dssvc_ProcessPathMove, 0, "PathMove_finish", { { 8, 4, 0x00FFFFFF } } },
    { "ProcessPathMove_old", dssvc_ProcessPathMove, 0, "PathMove_moveto", { { 8, 4, 0 } } },
    { "ProcessPathMove_hugeframe", dssvc_ProcessPathMove, 0, "PathMove_moveto", { { 8, 4, 0xFFFFFFFF } } },
    { "ProcessPathMove_badthing", dssvc_ProcessPathMove, 0, "PathMove_moveto", { { 4, 4, 0x7777 }, { 8, 4, 0x00FFFFFF } } },
    { "ProcessPathMove_notpath", dssvc_ProcessPathMove, 0, "PathMove_moveto", { { 4, 4, 0x1000 }, { 8, 4, 0x00FFFFFF } } },
    { "ProcessPathMove_badsector", dssvc_ProcessPathMove, 0, "PathMove_moveto", { { 8, 4, 0x00FFFFFF }, { 12, 2, 9 } } },

    // Take, create, destroy
    { "Take_client", dssvc_Take, 0, NULL, DSSV_NOMUT },
    { "Take_host", dssvc_Take, 1, NULL, DSSV_NOMUT },
    { "ProcessTake_client", dssvc_ProcessTake, 0, "Take_client", DSSV_NOMUT },
    { "ProcessTake_host", dssvc_ProcessTake, 1, "Take_client", DSSV_NOMUT },
    { "ProcessTake_host_disabled", dssvc_ProcessTake, 3, "Take_client", DSSV_NOMUT },
    { "ProcessTake_host_notitem", dssvc_ProcessTake, 1, "Take_client", { { 0, 4, 0x1033 } } },
    { "ProcessTake_take2", dssvc_ProcessTake, 0, "Take_client", { { DSSV_MSGTYPE, 2, 26 } } },
    { "ProcessTake_take2_host", dssvc_ProcessTake, 1, "Take_client", { { DSSV_MSGTYPE, 2, 26 } } },
    { "ProcessTake_unknownitem_host", dssvc_ProcessTake, 1, "Take_client", { { 0, 4, 0x7777 } } },
    { "ProcessTake_unknownitem_client", dssvc_ProcessTake, 0, "Take_client", { { 0, 4, 0x7777 } } },
    { "ProcessTake_unknowntaker", dssvc_ProcessTake, 1, "Take_client", { { 4, 4, 0x7777 } } },
    { "CreateThing_marker", dssvc_CreateThing, 0, NULL, DSSV_NOMUT },
    { "CreateThing_position", dssvc_CreateThing, 1, NULL, DSSV_NOMUT },
    { "ProcessCreateThing_marker", dssvc_ProcessCreateThing, 0, "CreateThing_marker", DSSV_NOMUT },
    { "ProcessCreateThing_position", dssvc_ProcessCreateThing, 0, "CreateThing_position", DSSV_NOMUT },
    { "ProcessCreateThing_marker_failed", dssvc_ProcessCreateThing, 1, "CreateThing_marker", DSSV_NOMUT },
    { "ProcessCreateThing_position_failed", dssvc_ProcessCreateThing, 1, "CreateThing_position", DSSV_NOMUT },
    { "ProcessCreateThing_badtemplate", dssvc_ProcessCreateThing, 0, "CreateThing_marker", { { 0, 2, 9 } } },
    { "ProcessCreateThing_statictemplate", dssvc_ProcessCreateThing, 0, "CreateThing_marker", { { 0, 2, 0x8001 } } },
    { "ProcessCreateThing_badmarker", dssvc_ProcessCreateThing, 0, "CreateThing_marker", { { 2, 4, 0x7777 } } },
    { "ProcessCreateThing_badsector", dssvc_ProcessCreateThing, 0, "CreateThing_position", { { 6, 4, 9 } } },
    { "DestroyThing_a", dssvc_DestroyThing, 3, NULL, DSSV_NOMUT },
    { "DestroyThing_b", dssvc_DestroyThing, 6, NULL, DSSV_NOMUT },
    { "ProcessDestroyThing", dssvc_ProcessDestroyThing, 0, "DestroyThing_a", DSSV_NOMUT },
    { "ProcessDestroyThing_unknown", dssvc_ProcessDestroyThing, 0, "DestroyThing_b", DSSV_NOMUT },

    // MoveToPos is called directly
    { "MoveToPos_close", dssvc_MoveToPos, 0, NULL, DSSV_NOMUT },
    { "MoveToPos_onthing", dssvc_MoveToPos, 1, NULL, DSSV_NOMUT },
    { "MoveToPos_far", dssvc_MoveToPos, 2, NULL, DSSV_NOMUT },
    { "MoveToPos_close_inexact", dssvc_MoveToPos, 3, NULL, DSSV_NOMUT },
    { "MoveToPos_edge_inexact", dssvc_MoveToPos, 4, NULL, DSSV_NOMUT },
    { "MoveToPos_close2_inexact", dssvc_MoveToPos, 5, NULL, DSSV_NOMUT },
};
#define DSSV_NUMTHINGCASES (sizeof(dssv_aThingCases) / sizeof(dssv_aThingCases[0]))

#endif // SITHDSSVECTHING_H
