// Synthetic world builder and engine call stubs shared by the sithDSS / sithDSSThing vector harness and tests.
#ifndef SITHDSSVECWORLD_H
#define SITHDSSVECWORLD_H

#include "sithDSSVec.h"

#define RF(x)  ((x) = dssv_RndF())
#define RU(x)  ((x) = dssv_Rnd())
#define RV3(x) dssv_RndVec3(&(x))
#define RV4(x) dssv_RndVec4(&(x))

// ---------------------------------------------------------------------------------------------------------------
// World builder
// ---------------------------------------------------------------------------------------------------------------
static void dssv_BuildThing(unsigned i, SithThing* t, SithThingType type, SithThingMoveType moveType)
{
    DssvArena* a = &dssv_arena;
    memset(t, 0, sizeof(*t));
    t->idx      = (int)i;
    t->guid     = DSSV_GUID(i);
    t->type     = type;
    t->moveType = moveType;
    if ( type == SITH_THING_FREE ) { return; }

    RU(t->flags);
    t->flags &= ~(SITH_TF_REMOTE | SITH_TF_COGLINKED | SITH_TF_DESTROYED | SITH_TF_DISABLED);
    RU(t->unknownFlags);
    RU(t->msecLifeLeft);
    RU(t->msecTimerTime);
    RU(t->msecNextPulseTime);
    RU(t->msecPulseInterval);
    t->collide.type = (SithCollideType)dssv_RndI(4);
    RF(t->collide.size);
    RF(t->collide.movesize);
    RU(t->signature);
    RF(t->userval);
    RF(t->alpha);
    RU(t->moveStatus);
    RV3(t->pos);
    RV3(t->forceMoveStartPos);
    RV3(t->orient.rvec);
    RV3(t->orient.lvec);
    RV3(t->orient.uvec);
    RV3(t->orient.dvec);
    RV4(t->light.color);
    RF(t->light.minRadius);
    RF(t->light.maxRadius);
    RU(t->attach.flags);

    t->pInSector = &a->sectors[i % DSSV_NUMSECTORS];
    t->pTemplate = &a->templates[i % DSSV_NUMTEMPLATES];
    snprintf(t->aName, sizeof(t->aName), "thing%u", i);

    switch ( type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_CORPSE:
        case SITH_THING_PLAYER:
        {
            SithActorInfo* p = &t->thingInfo.actorInfo;
            RU(p->flags);
            RF(p->health);
            RF(p->maxHealth);
            RU(p->endurance.msecUnderwater);
            RF(p->extraSpeed);
            RF(p->maxThrust);
            RF(p->maxRotVelocity);
            RV3(p->headPYR);
            RV4(p->headLightIntensity);
            RV3(p->fireOffset);
            RF(p->minHeadPitch);
            RF(p->maxHeadPitch);
            RF(p->maxHeadYaw);
            RF(p->maxHeadVelocity);
            p->attachMeshNum     = dssv_RndI(100);
            p->bForceMovePlay    = dssv_RndI(2);
            p->bControlsDisabled = dssv_RndI(2);
            p->stateChange.type  = (SithActorStateChangeType)dssv_RndI(3);
            RU(p->stateChange.params.armedMode);
            RF(p->weaponInfo.secActivationStartTime);
            RF(p->weaponInfo.secLastRapidFireTime);
            RF(p->weaponInfo.secActivationWaitEndTime);
            RF(p->weaponInfo.secActivationWaitTime);
            RF(p->weaponInfo.secAimWaitEndTime);
            RF(p->weaponInfo.secSwapTime);
            RU(p->weaponInfo.selectedWeaponID);
            RU(p->weaponInfo.curWeaponID);
            RU(p->weaponInfo.deselectedWeaponID);
            RU(p->weaponInfo.swapRefNum);
            RV3(p->weaponInfo.vecUnknown0);
            RU(p->voiceInfo.hSndChannel);
            RU(p->voiceInfo.voiceHeadInfo.headMeshNum);
            RU(p->voiceInfo.voiceHeadInfo.headSwapRefNum);
            RV4(p->voiceInfo.voiceColor.top);
            break;
        }

        case SITH_THING_WEAPON:
            RU(t->thingInfo.weaponInfo.flags);
            t->thingInfo.weaponInfo.numRicochets = dssv_RndI(1000);
            break;

        case SITH_THING_ITEM:
        {
            SithItemInfo* p = &t->thingInfo.itemInfo;
            RU(p->flags);
            p->flags &= ~SITH_ITEM_BACKPACK;
            p->numItems = 0;
            break;
        }

        default:
            break;
    }

    if ( moveType == SITH_MT_PHYSICS )
    {
        SithPhysicsInfo* p = &t->moveInfo.physics;
        RU(p->flags);
        RV3(p->velocity);
        RV3(p->angularVelocity);
        RV3(p->thrust);
        RV3(p->rotThrust);
        RF(p->maxRotationVelocity);
        RF(p->maxVelocity);
        RF(p->orientSpeed);
        RF(p->buoyancy);
        RF(p->height);
        RF(p->mass);
    }
    else if ( moveType == SITH_MT_PATH )
    {
        SithPathMoveInfo* p = &t->moveInfo.pathMovement;
        p->sizeFrames = 3;
        p->numFrames  = 3;
        p->aFrames    = a->pathFrames;
        p->mode       = (SithPathMoveMode)dssv_RndI(4);
        RV3(p->vecDeltaPos);
        RF(p->moveTimeRemaining);
        RF(p->moveVel);
        RF(p->rotDeltaTime);
        RV3(p->rotOffset);
        RV3(p->goalPYR);
        RF(p->rotDelta);
        p->numBlockedMoves = dssv_RndI(100);
        p->currentFrame    = dssv_RndI(3);
        p->nextFrame       = dssv_RndI(3);
        p->goalFrame       = dssv_RndI(3);
        RV3(p->curOrient.rvec);
        RV3(p->curOrient.lvec);
        RV3(p->curOrient.uvec);
        RV3(p->curOrient.dvec);
        RV3(p->rotateToPYR);
        for ( unsigned k = 0; k < 3; ++k ) { RV3(a->pathFrames[k].pos); RV3(a->pathFrames[k].pyr); }
    }
}

static void dssv_BuildPuppet(unsigned i, SithThing* t)
{
    DssvArena* a = &dssv_arena;
    rdPuppet* pup = &a->puppets[i];
    SithPuppetState* ps = &a->puppetStates[i];
    memset(pup, 0, sizeof(*pup));
    memset(ps, 0, sizeof(*ps));
    pup->pThing = &t->renderData;
    t->renderData.pPuppet = pup;
    t->pPuppetClass = &a->puppetClass;
    t->pPuppetState = ps;

    ps->armedMode = dssv_RndI(SITH_PUPPET_NUMARMEDMODES);
    ps->moveMode  = (SithPuppetMoveMode)dssv_RndI(SITH_PUPPET_NUMMOVEMODES);
    ps->majorMode = dssv_RndI(SITH_PUPPET_MAXMODES);
    ps->submode   = (SithPuppetSubMode)dssv_RndI(SITH_PUPPET_NUMSUBMODES);
    RU(ps->msecLastFidgetStillMoveTime);

    // Three live tracks: slot 1, 4 and 6
    static const unsigned liveTracks[3] = { 1, 4, 6 };
    for ( unsigned k = 0; k < 3; ++k )
    {
        rdPuppetTrack* tr = &pup->aTracks[liveTracks[k]];
        tr->status        = (rdPuppetTrackStatus)(1 + dssv_RndI(3));
        tr->pKFTrack      = &a->keys[(i + k) % DSSV_NUMKEYS];
        tr->playbackSpeed = 0.25f + (float)dssv_RndI(16);
        tr->lowPriority   = dssv_RndI(8);
        tr->highPriority  = dssv_RndI(8);
        RF(tr->fps);
        RF(tr->blendWeight);
        RF(tr->curFrame);
        RF(tr->prevFrame);
        RU(tr->guid);
    }
    pup->aTracks[1].pfCallback = (rdPuppetTrackCallback)sithPuppet_DefaultCallback_ADDR;
    pup->aTracks[4].pfCallback = (rdPuppetTrackCallback)sithAIMove_PuppetCallback_ADDR;
    pup->aTracks[6].pfCallback = NULL;

    // Puppet state track list: three entries
    ps->pFirstTrack = &a->puppetTracks[i][0];
    for ( unsigned k = 0; k < 3; ++k )
    {
        SithPuppetTrack* st = &a->puppetTracks[i][k];
        st->pSubmode   = &a->puppetClass.aModes[ps->majorMode][k];
        st->trackNum   = (int)liveTracks[k];
        st->submode    = (SithPuppetSubMode)(5 + k * 7);
        st->pNextTrack = k < 2 ? &a->puppetTracks[i][k + 1] : NULL;
    }
}

static void dssv_BuildWorld(uint32_t seed)
{
    DssvArena* a = &dssv_arena;
    dssv_rng = seed;

    memset(a, 0, sizeof(*a));
    // Garbage in everything past the used elements: a function that reads or writes outside the data it is given
    // gives different records for different fills
    memset(a->sectors, (int)dssv_fill, sizeof(a->sectors));
    memset(a->surfaces, (int)dssv_fill, sizeof(a->surfaces));
    memset(a->mats, (int)dssv_fill, sizeof(a->mats));
    memset(a->things, (int)dssv_fill, sizeof(a->things));
    memset(a->templates, (int)dssv_fill, sizeof(a->templates));
    memset(a->models, (int)dssv_fill, sizeof(a->models));
    memset(a->keys, (int)dssv_fill, sizeof(a->keys));
    memset(a->cogs, (int)dssv_fill, sizeof(a->cogs));
    memset(a->aiClasses, (int)dssv_fill, sizeof(a->aiClasses));
    memset(a->staticMats, (int)dssv_fill, sizeof(a->staticMats));

    SithWorld* w = &a->world;
    SithWorld* s = &a->staticWorld;
    sithWorld_g_pCurrentWorld = w;
    sithWorld_g_pStaticWorld  = s;

    // Sectors
    w->aSectors   = a->sectors;
    w->numSectors = DSSV_NUMSECTORS;
    for ( unsigned i = 0; i < DSSV_NUMSECTORS; ++i )
    {
        SithSector* p = &a->sectors[i];
        memset(p, 0, sizeof(*p));
        RU(p->flags);
        p->flags &= ~(SITH_SECTOR_USETHRUST | SITH_SECTOR_ADJOINSOFF | SITH_SECTOR_SEEN);
        if ( dssv_RndI(2) ) { p->flags |= SITH_SECTOR_SEEN; }
        RV4(p->ambientLight);
        RV4(p->extraLight);
        RV3(p->thrust);
        p->numSurfaces = 1;
    }
    a->sectors[0].flags |= SITH_SECTOR_USETHRUST;
    a->sectors[1].flags |= SITH_SECTOR_ADJOINSOFF;
    a->sectors[3].flags |= SITH_SECTOR_USETHRUST | SITH_SECTOR_ADJOINSOFF;
    s->aSectors   = a->staticSectors;
    s->numSectors = 2;

    // Materials
    w->aMaterials   = a->mats;
    w->numMaterials = DSSV_NUMMATS;
    for ( unsigned i = 0; i < DSSV_NUMMATS; ++i )
    {
        rdMaterial* m = &a->mats[i];
        memset(m, 0, sizeof(*m));
        snprintf(m->aName, sizeof(m->aName), "mat%u.mat", i);
        m->num      = (int)i;
        m->numCels  = 1 + 2 * (i & 1) + (i == 2); // 1, 3, 2
        m->curCelNum = dssv_RndI((unsigned)m->numCels);
    }
    s->aMaterials   = a->staticMats;
    s->numMaterials = DSSV_NUMSMATS;
    for ( unsigned i = 0; i < DSSV_NUMSMATS; ++i )
    {
        rdMaterial* m = &a->staticMats[i];
        memset(m, 0, sizeof(*m));
        snprintf(m->aName, sizeof(m->aName), "smat%u.mat", i);
        m->num       = (int)(i | SITH_STATICRESOURCE_INDEXMASK);
        m->numCels   = 2 + i * 2; // 2, 4
        m->curCelNum = dssv_RndI((unsigned)m->numCels);
    }

    // Adjoins and surfaces
    w->aAdjoins   = a->adjoins;
    w->numAdjoins = DSSV_NUMADJOINS;
    w->aSurfaces   = a->surfaces;
    w->numSurfaces = DSSV_NUMSURFACES;
    for ( unsigned i = 0; i < DSSV_NUMADJOINS; ++i )
    {
        memset(&a->adjoins[i], 0, sizeof(a->adjoins[i]));
        RU(a->adjoins[i].flags);
    }
    for ( unsigned i = 0; i < DSSV_NUMSURFACES; ++i )
    {
        SithSurface* p = &a->surfaces[i];
        memset(p, 0, sizeof(*p));
        p->pSector = &a->sectors[i % DSSV_NUMSECTORS];
        p->pAdjoin = i < DSSV_NUMADJOINS ? &a->adjoins[i] : NULL;
        RU(p->flags);
        RU(p->msecLastTouched);
        p->face.num          = i;
        RU(p->face.flags);
        p->face.geometryMode = (rdGeometryMode)dssv_RndI(6);
        p->face.lightingMode = (rdLightMode)dssv_RndI(6);
        p->face.numVertices  = 3 + (i & 1);
        p->face.aVertices    = a->faceVerts[i];
        p->face.pMaterial    = (i == 3 || i == 5) ? NULL : &a->mats[i % DSSV_NUMMATS];
        p->face.matCelNum    = p->face.pMaterial ? (int)dssv_RndI((unsigned)p->face.pMaterial->numCels) : -1;
        RF(p->face.texVertOffset.x);
        RF(p->face.texVertOffset.y);
        RV4(p->face.extraLight);
        p->aIntensities = a->intens[i];
        for ( unsigned k = 0; k < 4; ++k ) { RV4(a->intens[i][k]); }
    }

    // Keyframes
    w->aKeyframes   = a->keys;
    w->numKeyframes = DSSV_NUMKEYS;
    for ( unsigned i = 0; i < DSSV_NUMKEYS; ++i )
    {
        memset(&a->keys[i], 0, sizeof(a->keys[i]));
        snprintf(a->keys[i].aName, sizeof(a->keys[i].aName), "key%u.key", i);
        a->keys[i].idx = (int)i;
    }
    s->aKeyframes   = a->staticKeys;
    s->numKeyframes = 2;
    for ( unsigned i = 0; i < 2; ++i )
    {
        snprintf(a->staticKeys[i].aName, sizeof(a->staticKeys[i].aName), "skey%u.key", i);
        a->staticKeys[i].idx = (int)(i | SITH_STATICRESOURCE_INDEXMASK);
    }

    // Models, with one mesh of four faces each (SyncGameState serializes a face index)
    w->aModels   = a->models;
    w->numModels = DSSV_NUMMODELS;
    s->aModels   = a->staticModels;
    s->numModels = 3;
    for ( unsigned i = 0; i < DSSV_NUMMODELS + 3; ++i )
    {
        rdModel3* m = i < DSSV_NUMMODELS ? &a->models[i] : &a->staticModels[i - DSSV_NUMMODELS];
        memset(m, 0, sizeof(*m));
        snprintf(m->aName, sizeof(m->aName), "%s%u.3do", i < DSSV_NUMMODELS ? "model" : "smodel", i);
        m->num      = i;
        m->numGeos  = 1;
        a->modelMeshes[i].aFaces   = a->modelFaces[i];
        a->modelMeshes[i].numFaces = 4;
        m->aGeos[0].numMeshes = 1;
        m->aGeos[0].aMeshes   = &a->modelMeshes[i];
        RV3(m->insertOffset);
    }

    // Cogs
    w->aCogs   = a->cogs;
    w->numCogs = DSSV_NUMCOGS;
    for ( unsigned i = 0; i < DSSV_NUMCOGS; ++i ) { memset(&a->cogs[i], 0, sizeof(a->cogs[i])); a->cogs[i].idx = (int)i; }
    s->aCogs   = a->staticCogs;
    s->numCogs = 2;
    for ( unsigned i = 0; i < 2; ++i ) { a->staticCogs[i].idx = (int)(i | SITH_STATICRESOURCE_INDEXMASK); }

    // AI classes
    w->aAIClasses   = a->aiClasses;
    w->numAIClasses = DSSV_NUMAICLASSES;
    for ( unsigned i = 0; i < DSSV_NUMAICLASSES; ++i )
    {
        memset(&a->aiClasses[i], 0, sizeof(a->aiClasses[i]));
        a->aiClasses[i].num          = (int)i;
        a->aiClasses[i].numInstincts = 2 + (int)i * 3; // 2, 5
    }

    // Templates
    w->aThingTemplates   = a->templates;
    w->numThingTemplates = DSSV_NUMTEMPLATES;
    for ( unsigned i = 0; i < DSSV_NUMTEMPLATES; ++i )
    {
        memset(&a->templates[i], 0, sizeof(a->templates[i]));
        a->templates[i].type = (SithThingType)(i == 0 ? SITH_THING_ACTOR : (i == 1 ? SITH_THING_WEAPON : SITH_THING_ITEM));
        a->templates[i].idx  = (int)i;
        snprintf(a->templates[i].aName, sizeof(a->templates[i].aName), "+template%u", i);
    }
    s->aThingTemplates   = a->staticTemplates;
    s->numThingTemplates = 2;
    for ( unsigned i = 0; i < 2; ++i )
    {
        a->staticTemplates[i].type = SITH_THING_ACTOR;
        a->staticTemplates[i].idx  = (int)(i | SITH_STATICRESOURCE_INDEXMASK);
    }

    // Things
    w->aThings      = a->things;
    w->numThings    = DSSV_NUMTHINGS;
    w->lastThingIdx = (int)DSSV_NUMTHINGS - 1;
    dssv_BuildThing(0, &a->things[0], SITH_THING_PLAYER, SITH_MT_PHYSICS);
    dssv_BuildThing(1, &a->things[1], SITH_THING_ACTOR, SITH_MT_PHYSICS);
    dssv_BuildThing(2, &a->things[2], SITH_THING_ITEM, SITH_MT_NONE);
    dssv_BuildThing(3, &a->things[3], SITH_THING_WEAPON, SITH_MT_PHYSICS);
    dssv_BuildThing(4, &a->things[4], SITH_THING_ACTOR, SITH_MT_PATH);
    dssv_BuildThing(5, &a->things[5], SITH_THING_SPRITE, SITH_MT_NONE);
    dssv_BuildThing(6, &a->things[6], SITH_THING_FREE, SITH_MT_NONE);
    dssv_BuildThing(7, &a->things[7], SITH_THING_CORPSE, SITH_MT_NONE);
    a->things[6].pInSector = NULL;

    // Players: sithPlayer_g_aPlayers is an exe global
    memset(&sithPlayer_g_aPlayers[0], 0, sizeof(sithPlayer_g_aPlayers[0]) * 2);
    for ( unsigned i = 0; i < 2; ++i )
    {
        SithPlayer* pl = &sithPlayer_g_aPlayers[i];
        for ( unsigned k = 0; k < SITHINVENTORY_MAXTYPES; ++k )
        {
            pl->aItems[k].amount = dssv_RndF();
            pl->aItems[k].status = (SithInventoryItemStatus)dssv_Rnd();
        }
        RU(pl->curItemID);
        RU(pl->curWeaponID);
    }
    sithPlayer_g_aPlayers[0].pThing = &a->things[0];
    sithPlayer_g_aPlayers[1].pThing = &a->things[1];
    a->things[0].thingInfo.actorInfo.pPlayer = &sithPlayer_g_aPlayers[0];
    a->things[1].thingInfo.actorInfo.pPlayer = &sithPlayer_g_aPlayers[1];
    sithPlayer_g_pLocalPlayer = &sithPlayer_g_aPlayers[0];
    w->pLocalPlayer = &a->things[0];

    // Puppets for the player and the two actors
    dssv_BuildPuppet(0, &a->things[0]);
    dssv_BuildPuppet(1, &a->things[1]);
    dssv_BuildPuppet(4, &a->things[4]);
    a->things[0].renderData.type = RD_THING_MODEL3;
    a->things[0].renderData.data.pModel3 = &a->models[0];
    a->things[1].renderData.type = RD_THING_MODEL3;
    a->things[1].renderData.data.pModel3 = &a->models[1];
    a->things[4].renderData.type = RD_THING_MODEL3;
    a->things[4].renderData.data.pModel3 = &a->models[2];
    a->things[5].renderData.type = RD_THING_SPRITE3;
    a->things[5].renderData.data.pSprite3 = &a->sprite;
    a->sprite.face.pMaterial = &a->mats[1];

    // AI control block of thing 1
    SithAIControlBlock* ai = &a->aiBlocks[0];
    memset(ai, 0, sizeof(*ai));
    a->things[1].controlType = SITH_CT_AI;
    a->things[1].controlInfo.aiControl.pClass = &a->aiClasses[0];
    a->things[1].controlInfo.aiControl.pLocal = ai;
    ai->pOwner = &a->things[1];
    ai->pClass = &a->aiClasses[0];
    ai->mode = (SithAIMode)(dssv_Rnd() & 0x3FFFF);
    ai->mode &= ~(SITHAI_MODE_MOVING | SITHAI_MODE_TURNING | SITHAI_MODE_FLEEING);
    ai->submode = (SithAISubMode)dssv_RndI(10);
    RU(ai->msecNextUpdate);
    ai->numInstincts = 2;
    for ( unsigned k = 0; k < 2; ++k )
    {
        RU(ai->aInstinctStates[k].flags);
        RU(ai->aInstinctStates[k].msecNextUpdate);
        for ( unsigned j = 0; j < 4; ++j ) { RF(ai->aInstinctStates[k].aParams[j]); }
    }
    RV3(ai->goalLVec);
    RV3(ai->movePos);
    ai->pFleeFromThing = &a->things[2];
    ai->pTargetThing   = &a->things[0];
    ai->goalThing      = &a->things[3];
    RV3(ai->targetPos);
    RV3(ai->vecUnknown122);
    RU(ai->msecAttackStart);
    RF(ai->moveSpeed);
    RV3(ai->vecUnknown6);
    RV3(ai->vecUnknown5);
    RU(ai->unknown150);
    RV3(ai->homePos);
    RV3(ai->homeOrient);
    RU(ai->msecFireWaitTime);
    RU(ai->msecPauseMoveUntil);
    ai->aFrames    = a->aiFrames;
    ai->numFrames  = 3;
    ai->sizeFrames = 4;
    for ( unsigned k = 0; k < 4; ++k ) { RV3(a->aiFrames[k]); }
    RU(ai->allowedSurfaceTypes);
    RV3(ai->vecUnknown3);
    RV3(ai->vecUnknown);
    RF(ai->maxHomeDist);

    // Animation slots
    for ( unsigned i = 0; i < DSSV_NUMANIMS + 1; ++i )
    {
        SithAnimationSlot* an = &a->anims[i];
        memset(an, 0, sizeof(*an));
        an->animID = (int)(100 + i);
        RU(an->flags);
        an->flags &= 0x7F; // plain flags only, the case selects the rest
        an->pThing = &a->things[(i + 1) % DSSV_NUMTHINGS];
        an->pItemThing = &a->things[(i + 2) % DSSV_NUMTHINGS];
        RU(an->thingSignature);
        an->pMaterial = &a->mats[i % DSSV_NUMMATS];
        an->pSurface = &a->surfaces[i % DSSV_NUMSURFACES];
        an->pSector = &a->sectors[i % DSSV_NUMSECTORS];
        RF(an->direction2.x); RF(an->direction2.y);
        RV3(an->direction3);
        RV3(an->thingEndPosition);
        RU(an->msecNextFrameTime);
        RU(an->msecPerFrame);
        RU(an->curFrame);
        an->trackNum = dssv_RndI(8);
        RF(an->curValue); RF(an->endValue); RF(an->secTimeRemaining); RF(an->deltaValue);
        RV4(an->startVector); RV4(an->curVector); RV4(an->endVector); RV4(an->deltaVector);
    }

    // Event, vehicle states
    RU(a->event.msecEventTime);
    a->event.taskNum = dssv_RndI(1000);
    RU(a->event.params.idx);
    RU(a->event.params.param1);
    RU(a->event.params.param2);
    RU(a->event.params.param3);

    SithMineCarControlsState* mc = &a->mineCar;
    RF(mc->secElapsedStoppingTime); mc->moveState = (SithMinecarControlsMoveState)dssv_RndI(5);
    RF(mc->secDuckTime); RU(mc->curDuckPuppetTrackNum); RU(mc->bCanDuck);
    mc->leanState = (SithMinecarControlsLeanState)dssv_RndI(3);
    RU(mc->curPuppetTrack); RU(mc->hBrakeSnd); RF(mc->secElapsedBrakingTime); RF(mc->secUnboardingElapsedTime);
    mc->unboardState = (SithMinecarControlsUnboardState)dssv_RndI(3);
    SithRaftControlsState* rf = &a->raft;
    rf->nextMoveStatus = (SithThingMoveStatus)dssv_Rnd(); rf->nextPuppetMode = (SithPuppetSubMode)dssv_Rnd();
    RV3(rf->unboardPos); RV3(rf->unboardNorm); RF(rf->moveSize); RF(rf->wakeTimer); RU(rf->bRowing);
    RF(rf->secRowStartTime); RF(rf->secUnboardTime);
}

#endif // SITHDSSVECWORLD_H
