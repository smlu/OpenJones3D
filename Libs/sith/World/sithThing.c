#include "sithThing.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Primitives/rdParticle.h>
#include <rdroid/Primitives/rdPolyline.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIClass.h>
#include <sith/AI/sithAIMove.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/Engine/sithParticle.h>
#include <sith/Engine/sithPathMove.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithExplosion.h>
#include <sith/World/sithItem.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSprite.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdConfig.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

// Just static regression tests
static_assert(SITHTHING_TYPEMASK(SITH_THING_FREE) == 0x01, "SITHTHING_TYPEMASK(SITH_THING_FREE) == 0x01");
static_assert(SITHTHING_TYPEMASK(SITH_THING_PLAYER) == 0x400, "SITHTHING_TYPEMASK(SITH_THING_PLAYER) == 0x400");
static_assert(SITHTHING_TYPEMASK(SITH_THING_FREE, SITH_THING_PLAYER) == 0x401, "SITHTHING_TYPEMASK(SITH_THING_FREE, SITH_THING_PLAYER) == 0x401");
static_assert(SITHTHING_TYPEMASK(SITH_THING_ACTOR, SITH_THING_PLAYER) == 0x404, "SITHTHING_TYPEMASK(SITH_THING_ACTOR, SITH_THING_PLAYER) == 0x404");
static_assert(SITHTHING_TYPEMASK(SITH_THING_ACTOR, SITH_THING_PLAYER, SITH_THING_WEAPON) == 0x40C, "SITHTHING_TYPEMASK(SITH_THING_ACTOR, SITH_THING_PLAYER, SITH_THING_WEAPON) == 0x40C");

#define SITHTHING_MAXTHINGSTOPURGE 10

#define SITHTHING_PARSEARG_PARSED      0
#define SITHTHING_PARSEARG_NOTPARSED   1
#define SITHTHING_PARSEARG_BADSYNTAX   19

#define sithThing_prevQuetzAttachInfo J3D_DECL_FAR_VAR(sithThing_prevQuetzAttachInfo, SithQuetzAttachInfo*)
#define sithThing_curQuetzAttachInfo J3D_DECL_FAR_VAR(sithThing_curQuetzAttachInfo, SithQuetzAttachInfo*)
#define sithThing_dword_5612B8 J3D_DECL_FAR_VAR(sithThing_dword_5612B8, int)

static bool sithThing_bThingStartup = false; // Added: Init to false TODO: rename to bThingStartup

SithThingUnknownFunc sithThing_pfUnknownFunc = NULL; // Fixed: Init to NULL

uint32_t sithThing_curSignature = 1u;
uint16_t sithThing_guidEntropy  = 1u;

size_t sithThing_numFreeThings;
size_t sithThing_aFreeThingIdxs[2304];

#define SITHTHING_BUFFEREXTRACAPACITY_DFLT 256
size_t sithThing_bufferExtraCapacity      = 0;

size_t sithThing_numUnsyncedThings;
SithThing* sithThing_apUnsyncedThings[16];
SithThingSyncFlags sithThing_aSyncFlags[STD_ARRAYLEN(sithThing_apUnsyncedThings)];

tHashTable* sithThing_pParseHashtbl = NULL; // Added: Init to null

static const char* sithThing_aStrThingTypes[SITH_THING_NUMTYPES] =
{
    "free",
    "camera",
    "actor",
    "weapon",
    "debris",
    "item",
    "explosion",
    "cog",
    "ghost",
    "corpse",
    "player",
    "particle",
    "hint",
    "sprite",
    "polyline"
};

static const float sithThing_aWeaponCoefficients[25] =
{
    [SITHWEAPON_NO_WEAPON] =     0.0f,
    [SITHWEAPON_FISTS]         = 0.0f,
    [SITHWEAPON_WHIP]          = 0.0f,
    [SITHWEAPON_PISTOL]        = 0.025f, // Pistol
    [SITHWEAPON_TOKAREV]       = 0.050000001f,
    [SITHWEAPON_MAUSER]        = 0.059999999f,
    [SITHWEAPON_SIMONOV]       = 0.059999999f,
    [SITHWEAPON_MACHETE]       = 0.0f,
    [SITHWEAPON_SUBMACHINE]    = 0.0099999998f,
    [SITHWEAPON_SHOTGUN]       = 0.0f, // Shotgun
    [SITHWEAPON_BAZOOKA]       = 0.0f,
    [SITHWEAPON_GRENADE]       = 0.0f,
    [SITHWEAPON_SATCHEL]       = 0.0f,
    [SITHWEAPON_ZIPPO]         = 0.0f,
    [SITHWEAPON_IMP1]          = 0.0f,
    [SITHWEAPON_IMP2]          = 0.0f,
    [SITHWEAPON_IMP3]          = 0.0f,
    [SITHWEAPON_IMP4]          = 0.0f,
    [SITHWEAPON_IMP5]          = 0.0f,
    [SITHWEAPON_COMFISTS]      = 0.0f,
    [SITHWEAPON_COMTOKAREV]    = 0.050000001f,
    [SITHWEAPON_COMMAUSER]     = 0.079999998f,
    [SITHWEAPON_COMSIMONOV]    = 0.079999998f,
    [SITHWEAPON_COMSUBMACHINE] = 0.029999999f,
    [SITHWEAPON_COMSHOTGUN]    = 0.0f
};

static const char* sithThing_aStrThingArgs[77] =
{
    "--invalid--",
    "type",
    "collide",
    "move",
    "size",
    "thingflags",
    "timer",
    "light",
    "attach",
    "soundclass",
    "model3d",
    "sprite",
    "surfdrag",
    "airdrag",
    "staticdrag",
    "mass",
    "height",
    "physflags",
    "maxrotvel",
    "maxvel",
    "vel",
    "angvel",
    "typeflags",
    "health",
    "maxthrust",
    "maxrotthrust",
    "maxheadvel",
    "maxheadyaw",
    "jumpspeed",
    "weapon",
    "damage",
    "mindamage",
    "damageclass",
    "explode",
    "frame",
    "numframes",
    "puppet",
    "blasttime",
    "babytime",
    "force",
    "maxlight",
    "range",
    "flashrgb",
    "expandtime",
    "fadetime",
    "aiclass",
    "cog",
    "respawn",
    "material",
    "rate",
    "count",
    "elementsize",
    "particle",
    "maxhealth",
    "movesize",
    "orientspeed",
    "buoyancy",
    "eyeoffset",
    "minheadpitch",
    "maxheadpitch",
    "fireoffset",
    "lightoffset",
    "lightintensity",
    "debris",
    "creatething",
    "minsize",
    "pitchrange",
    "yawrange",
    "orient",
    "userval",
    "spritething",
    "spritestart",
    "spriteend",
    "collheight",
    "collwidth",
    "voicecolor",
    "perflevel"
};

void J3DAPI sithThing_InitializeWorldThings(SithWorld* pWorld);
void J3DAPI sithThing_UpdateMove(SithThing* pThing, float secDeltaTime);
void J3DAPI sithThing_DestroyDyingThing(SithThing* pThing);
void J3DAPI sithThing_PlayCogDamageSound(SithThing* pThing, SithDamageType hitType);

void J3DAPI sithThing_EnterWater(SithThing* pThing, int bNoSplash);
void J3DAPI sithThing_ExitWater(SithThing* pThing, int bNoSplash);
void J3DAPI sithThing_PurgeGarbageThings(SithWorld* pWorld);

int J3DAPI sithThing_ParseThingArg(const StdConffileArg* pArg, SithWorld* pWorld, SithThing* pThing, int adjNum);
int J3DAPI sithThing_ParseType(const char* pType);
int J3DAPI sithThing_ParseThingPlacement(SithWorld* pWorld);

void J3DAPI sithThing_FreeThing(SithWorld* pWorld, SithThing* pThing);
void J3DAPI sithThing_FreeThingIndex(SithWorld* pWorld, size_t thingNum);
int sithThing_GetFreeThingIndex(void);

void sithThing_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(sithThing_Startup);
    J3D_HOOKFUNC(sithThing_Shutdown);
    J3D_HOOKFUNC(sithThing_Draw);
    J3D_HOOKFUNC(sithThing_AddSwapEntry);
    J3D_HOOKFUNC(sithThing_RemoveSwapEntry);
    J3D_HOOKFUNC(sithThing_ResetSwapList);
    J3D_HOOKFUNC(sithThing_Update);
    J3D_HOOKFUNC(sithThing_UpdateMove);
    J3D_HOOKFUNC(sithThing_DestroyDyingThing);
    J3D_HOOKFUNC(sithThing_GetThingParent);
    J3D_HOOKFUNC(sithThing_GetThingByIndex);
    J3D_HOOKFUNC(sithThing_GetThingIndex);
    J3D_HOOKFUNC(sithThing_GetGuidThing);
    J3D_HOOKFUNC(sithThing_DestroyThing);
    J3D_HOOKFUNC(sithThing_DamageThing);
    J3D_HOOKFUNC(sithThing_ScaleCombatDamage);
    J3D_HOOKFUNC(sithThing_PlayCogDamageSound);
    J3D_HOOKFUNC(sithThing_AllocWorldThings);
    J3D_HOOKFUNC(sithThing_FreeWorldThings);
    J3D_HOOKFUNC(sithThing_RemoveWorldThings);
    J3D_HOOKFUNC(sithThing_InitializeWorldThings);
    J3D_HOOKFUNC(sithThing_LoadPostProcess);
    J3D_HOOKFUNC(sithThing_RemoveThing);
    J3D_HOOKFUNC(sithThing_FreeThing);
    J3D_HOOKFUNC(sithThing_Initialize);
    J3D_HOOKFUNC(sithThing_Reset);
    J3D_HOOKFUNC(sithThing_SetSector);
    J3D_HOOKFUNC(sithThing_ExitSector);
    J3D_HOOKFUNC(sithThing_EnterSector);
    J3D_HOOKFUNC(sithThing_EnterWater);
    J3D_HOOKFUNC(sithThing_ExitWater);

    J3D_HOOKFUNC(sithThing_Create);
    J3D_HOOKFUNC(sithThing_SetPositionAndOrient);
    J3D_HOOKFUNC(sithThing_SetThingModel);
    J3D_HOOKFUNC(sithThing_SetThingBasedOn);
    J3D_HOOKFUNC(sithThing_CreateThingAtPos);
    J3D_HOOKFUNC(sithThing_CreateThing);

    J3D_HOOKFUNC(sithThing_AttachThingToSurface);
    J3D_HOOKFUNC(sithThing_AttachThingToClimbSurface);
    J3D_HOOKFUNC(sithThing_AttachThingToThingFace);
    J3D_HOOKFUNC(sithThing_AddThingToAttachedThings);
    J3D_HOOKFUNC(sithThing_AttachThingToThing);
    J3D_HOOKFUNC(sithThing_AttachThingToThingMesh);
    J3D_HOOKFUNC(sithThing_DetachThing);
    J3D_HOOKFUNC(sithThing_DetachAttachedThings);
    J3D_HOOKFUNC(sithThing_PurgeGarbageThings);

    J3D_HOOKFUNC(sithThing_ReadStaticThingsListText);
    J3D_HOOKFUNC(sithThing_WriteStaticThingsListBinary);
    J3D_HOOKFUNC(sithThing_ReadStaticThingsListBinary);

    // J3D_HOOKFUNC(sithThing_WriteThingsListBinary);
    // J3D_HOOKFUNC(sithThing_ReadThingsListBinary);

    J3D_HOOKFUNC(sithThing_ParseArg);
    J3D_HOOKFUNC(sithThing_ParseThingArg);
    J3D_HOOKFUNC(sithThing_ParseType);
    J3D_HOOKFUNC(sithThing_ValidateThingPointer);
    J3D_HOOKFUNC(sithThing_SyncThing);
    J3D_HOOKFUNC(sithThing_SyncThings);
    J3D_HOOKFUNC(sithThing_CanSyncThing);
    J3D_HOOKFUNC(sithThing_GetThingMeshIndex);
    J3D_HOOKFUNC(sithThing_GetThingJointIndex);
    J3D_HOOKFUNC(sithThing_FreeThingIndex);
    J3D_HOOKFUNC(sithThing_GetFreeThingIndex);
    J3D_HOOKFUNC(sithThing_ParseThingPlacement);
    J3D_HOOKFUNC(sithThing_CreateQuetzUserBlock);
    //J3D_HOOKFUNC(sithThing_UpdateQuetzUserBlock);
}

void sithThing_ResetGlobals(void)
{
    STD_ZEROMEM(&sithThing_prevQuetzAttachInfo, sizeof(sithThing_prevQuetzAttachInfo));
    STD_ZEROMEM(&sithThing_curQuetzAttachInfo, sizeof(sithThing_curQuetzAttachInfo));
    STD_ZEROMEM(&sithThing_dword_5612B8, sizeof(sithThing_dword_5612B8));
}

int J3DAPI sithThing_WriteThingsListBinary(tFileHandle fh, const SithWorld* pWorld, size_t numThings, const SithThing* aThings)
{
    return J3D_TRAMPOLINE_CALL(sithThing_WriteThingsListBinary, fh, pWorld, numThings, aThings);
}

int J3DAPI sithThing_ReadThingsListBinary(tFileHandle fh, SithWorld* pWorld, size_t numThings, SithThing* aThings, void (J3DAPI* pfInitThingFunc)(SithThing*))
{
    return J3D_TRAMPOLINE_CALL(sithThing_ReadThingsListBinary, fh, pWorld, numThings, aThings, pfInitThingFunc);
}

void J3DAPI sithThing_UpdateQuetzUserBlock(SithThing* pThing)
{
    J3D_TRAMPOLINE_CALL(sithThing_UpdateQuetzUserBlock, pThing);
}

int sithThing_Startup(void)
{
    if ( sithThing_bThingStartup )
    {
        SITHLOG_ERROR("Multiple startup on sithThing_startup().\n");
        return 0;
    }

    sithThing_pParseHashtbl = stdHashtbl_New(154u);
    if ( !sithThing_pParseHashtbl )
    {
        SITHLOG_ERROR("Could not allocate thing parsing hashtable.\n");
        return 0;
    }

    for ( size_t adjNum = 1; adjNum < STD_ARRAYLEN(sithThing_aStrThingArgs); ++adjNum )
    {
        stdHashtbl_Add(sithThing_pParseHashtbl, sithThing_aStrThingArgs[adjNum], (void*)adjNum);
    }

#ifdef J3D_QOL_IMPROVEMENTS
    sithThing_bufferExtraCapacity = stdConfig_GetInt(SITHTHING_CFG_WORLDTHINGS_EXTRACAPACITY, SITHTHING_BUFFEREXTRACAPACITY_DFLT);

    if ( !stdConfig_Contains(SITHTHING_CFG_WORLDTHINGS_EXTRACAPACITY) )
    {
        stdConfig_SetInt(SITHTHING_CFG_WORLDTHINGS_EXTRACAPACITY, sithThing_bufferExtraCapacity);
    }
#endif

    sithThing_curSignature  = 1;
    sithThing_bThingStartup = true;
    return 1;
}

int sithThing_Shutdown(void)
{
    if ( !sithThing_bThingStartup )
    {
        SITHLOG_ERROR("Thing system not started.\n");
        return 0;
    }

    stdHashtbl_Free(sithThing_pParseHashtbl);
    SITHLOG_STATUS("Thing signatures used=%d.\n", sithThing_curSignature - 1);

    sithThing_bThingStartup = false;
    return 1;
}

void J3DAPI sithThing_RegisterUnknownFunc(SithThingUnknownFunc pFunc)
{
    if ( pFunc )
    {
        sithThing_pfUnknownFunc = pFunc;
    }
}

int J3DAPI sithThing_Draw(SithThing* pThing)
{
    if ( pThing->type == SITH_THING_POLYLINE )
    {
        if ( !pThing->renderData.data.pPolyline )
        {
            SITHLOG_ERROR("WARNING: Polyline thing \"%s\" has no polyline data!\n", pThing->aName);
        }
        else
        {
            pThing->renderData.data.pPolyline->face.extraLight.alpha = pThing->alpha;
        }
    }

    if ( pThing->renderData.type == RD_THING_MODEL3 )
    {
        rdVector4 color = { 0 };
        color.alpha = pThing->alpha;

        rdModel3_SetThingColor(&pThing->renderData, &color);
    }

    int drawRes = 0;
    if ( pThing->pSwapList )
    {
        rdVector4 color = { 0 };
        rdModel3_GetThingColor(&pThing->renderData, &color);

        for ( SithThingSwapEntry* pEntry = pThing->pSwapList; pEntry; pEntry = pEntry->pNextEntry )
        {
            rdModel3_SetModelColor(pEntry->pSrcModel, &color);
            rdModel3_SwapMesh(pThing->renderData.data.pModel3, pEntry->meshNum, pEntry->pSrcModel, pEntry->srcMeshNum);
        }

        // Draw thing
        drawRes = rdThing_Draw(&pThing->renderData, &pThing->orient);

        rdVector_Zero4(&color);
        color.alpha = 1.0f;

        for ( SithThingSwapEntry* pEntry = pThing->pSwapList; pEntry; pEntry = pEntry->pNextEntry )
        {
            rdModel3_SwapMesh(pThing->renderData.data.pModel3, pEntry->meshNum, pEntry->pSrcModel, pEntry->srcMeshNum);
            rdModel3_SetModelColor(pEntry->pSrcModel, &color);
        }
    }
    else
    {
        switch ( pThing->type )
        {
            case SITH_THING_CAMERA:
            case SITH_THING_GHOST:
            case SITH_THING_HINT:
                drawRes = 0;
                break;

            case SITH_THING_SPRITE:
                drawRes = sithSprite_Draw(pThing);
                break;

            case SITH_THING_POLYLINE:
                sithFX_UpdatePolyline(pThing);
                drawRes = rdThing_Draw(&pThing->renderData, &pThing->orient);
                sithFX_ResetPolylineTexVertOffset(pThing);
                break;

            default:
                drawRes = rdThing_Draw(&pThing->renderData, &pThing->orient);
                break;
        }
    }

    return drawRes;
}

int J3DAPI sithThing_AddSwapEntry(SithThing* pThing, int meshNum, rdModel3* pSrcModel, int meshNumSrc)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pSrcModel);

    if ( meshNum < 0 || meshNumSrc < 0 )
    {
        SITHLOG_ERROR("-1 mesh num passed to sithThing_AddSwapEntry!\n");
        return -1;
    }

    if ( pThing->pSwapList )
    {
        for ( SithThingSwapEntry* pCurEntry = pThing->pSwapList; pCurEntry; pCurEntry = pCurEntry->pNextEntry )
        {
            if ( pCurEntry->meshNum == meshNum )
            {
                pCurEntry->srcMeshNum = meshNumSrc;
                pCurEntry->pSrcModel  = pSrcModel;
                return pCurEntry->entryNum;
            }
        }
    }

    SithThingSwapEntry* pNewEntry = (SithThingSwapEntry*)STDMALLOC(sizeof(SithThingSwapEntry));
    if ( !pNewEntry )
    {
        SITHLOG_ERROR("Couldn't allocate a swap list entry!\n");
        return -1;
    }

    pNewEntry->meshNum    = meshNum;
    pNewEntry->srcMeshNum = meshNumSrc;
    pNewEntry->pSrcModel  = pSrcModel;
    pNewEntry->pNextEntry = NULL;
    pNewEntry->entryNum   = pThing->numSwapEntries++; // This looks like to be an entry signature rather than entry num

    if ( !pThing->pSwapList )
    {
        pThing->pSwapList = pNewEntry;
    }
    else
    {
        SithThingSwapEntry* pCurEntry = pThing->pSwapList;
        for ( ; pCurEntry->pNextEntry; pCurEntry = pCurEntry->pNextEntry )
        {
            ;
        }

        pCurEntry->pNextEntry = pNewEntry;
    }

    return pNewEntry->entryNum;
}

int J3DAPI sithThing_RemoveSwapEntry(SithThing* pThing, int refnum)
{
    if ( !pThing->pSwapList )
    {
        return 0;
    }

    if ( refnum < 0 )
    {
        SITHLOG_ERROR("Please don't pass -1 as a refnum to sithThing_RemoveSwapEntry!\n");
        return 1;
    }

    SithThingSwapEntry* pCurEntry  = pThing->pSwapList;
    SithThingSwapEntry* pPrevEntry = NULL;

    bool bFinished = false;
    while ( !bFinished )
    {
        if ( pCurEntry->entryNum == refnum )
        {
            SithThingSwapEntry* pNextEntry = pCurEntry->pNextEntry;
            if ( pPrevEntry )
            {
                pPrevEntry->pNextEntry = pNextEntry;
            }
            else
            {
                pThing->pSwapList = pNextEntry;
            }

            STDFREE(pCurEntry);
            bFinished = true;
        }
        else
        {
            pPrevEntry = pCurEntry;
            pCurEntry = pCurEntry->pNextEntry;
            if ( !pCurEntry )
            {
                return 0;
            }
        }
    }

    return 0;
}

void J3DAPI sithThing_ResetSwapList(SithThing* pThing)
{
    while ( pThing->pSwapList )
    {
        sithThing_RemoveSwapEntry(pThing, pThing->pSwapList->entryNum);
    }
}

void J3DAPI sithThing_Update(float secDeltaTime, uint32_t msecDeltaTime)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    SITH_ASSERTREL(pWorld != NULL);

    const int lastThingIdx = pWorld->lastThingIdx; // Fixed: Moved after assert
    for ( int i = 0; i <= lastThingIdx; i++ )
    {
        SithThing* pThing = &pWorld->aThings[i];
        if ( pThing->type == SITH_THING_FREE )
        {
            continue;
        }

        if ( (pThing->flags & SITH_TF_DESTROYED) != 0 )
        {
            sithThing_RemoveThing(pWorld, pThing);
            continue;
        }

        if ( pThing->msecLifeLeft )
        {
            if ( pThing->msecLifeLeft > msecDeltaTime )
            {
                pThing->msecLifeLeft -= msecDeltaTime;
            }
            else
            {
                sithThing_DestroyDyingThing(pThing);
            }
        }

        if ( (pThing->flags & SITH_TF_DISABLED) != 0 )
        {
            continue;
        }

        // Update time
        if ( (pThing->flags & (SITH_TF_TIMERSET | SITH_TF_PULSESET)) != 0 )
        {
            sithCog_UpdateThingTimer(pThing);
        }

        // Update control type
        switch ( pThing->controlType )
        {
            case SITH_CT_AI:
                sithAIMove_Update(pThing, secDeltaTime);
                break;

            case SITH_CT_EXPLOSION:
                sithExplosion_Update(pThing, secDeltaTime);
                break;

            case SITH_CT_PARTICLE:
                sithParticle_Update(pThing, secDeltaTime);
                break;
        }

        switch ( pThing->type )
        {
            case SITH_THING_WEAPON:
                sithWeapon_Update(pThing, secDeltaTime);
                break;

            case SITH_THING_PLAYER:
                sithPlayer_Update(pThing->thingInfo.actorInfo.pPlayer, secDeltaTime);
                // Fall through to actor handling

            case SITH_THING_ACTOR:
            {
                if ( pThing->thingInfo.actorInfo.pThingMeshAttached )
                {
                    SithThing* pThingAttached = pThing->thingInfo.actorInfo.pThingMeshAttached;
                    if ( pThingAttached->type == SITH_THING_PLAYER )
                    {
                        pThing->alpha = pThingAttached->alpha;
                    }
                }

                sithActor_Update(pThing, msecDeltaTime);
            } break;

            case SITH_THING_SPRITE:
            {
                if ( pThing->thingInfo.spriteInfo.pThingMeshAttached )
                {
                    SithThing* pThingAttached = pThing->thingInfo.spriteInfo.pThingMeshAttached;
                    if ( pThingAttached->type == SITH_THING_PLAYER )
                    {
                        pThing->thingInfo.spriteInfo.alpha = pThing->pTemplate->thingInfo.spriteInfo.alpha * pThingAttached->alpha; // TODO: why template alpha is multiplied by attached alpha?
                    }
                }

                sithSprite_Update(pThing, msecDeltaTime);
            } break;

            default:
                break;
        }

        if ( sithThing_pfUnknownFunc && pThing->unknownFlags )
        {
            sithThing_pfUnknownFunc(pThing);
        }

        // Update movement
        switch ( pThing->moveType )
        {
            case SITH_MT_NONE:
                break;
            case SITH_MT_PHYSICS:
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    if ( pThing->type == SITH_THING_PLAYER && pThing != sithPlayer_g_pLocalPlayerThing )
                    {
                        pThing->moveInfo.physics.flags |= SITH_PF_FORCEAPPLIED; // Multiplayer non-local players always have force applied, and won't have surface drag applied
                    }
                    sithPhysics_UpdateThing(pThing, secDeltaTime);
                }
                break;
            case SITH_MT_PATH:
                sithPathMove_Update(pThing, secDeltaTime);
                break;
        }

        // Move thing based on physics/path updated velocity and rotation
        // TODO: why is it assumed that pThing is actor here???
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            sithThing_UpdateMove(pThing, secDeltaTime);
        }

        // Update puppet animation
        sithPuppet_UpdatePuppet(pThing, secDeltaTime);
    }
}

void J3DAPI sithThing_UpdateMove(SithThing* pThing, float secDeltaTime)
{
    // Function update thing movement

    int moveFlags = 0;
    if ( (pThing->attach.flags & SITH_ATTACH_NOMOVE) != 0 )
    {
        return;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        pThing->moveDir = pThing->moveInfo.physics.deltaVelocity;
        pThing->distanceMoved = 0.0f;
    }
    else // path
    {
        rdVector_Zero3(&pThing->moveDir);
        moveFlags |= 0x04;
    }

    if ( pThing->attach.flags
        && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0
        && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_SCROLLING) != 0 )
    {
        SithSurface* pSurface = pThing->attach.attachedToStructure.pSurfaceAttached;
        SITH_ASSERTREL(pSurface);

        rdVector3 scrollDir;
        sithAnimate_GetSurfaceScrollingDirection(pSurface, &scrollDir);

        rdVector_MultAcc3(&pThing->moveDir, &scrollDir, secDeltaTime);
    }

    if ( rdVector_IsZero3(&pThing->moveDir) )
    {
        if ( pThing->moveType == SITH_MT_PHYSICS
            && (pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0
            && pThing->attach.attachedToStructure.pThingAttached->moveType == SITH_MT_PATH
            && pThing->moveStatus != SITHPLAYERMOVE_HANGING )
        {
            sithPhysics_FindFloor(pThing, /*bNotThingStateUpdate*/0);
        }
    }
    else
    {
        rdVector3 moveNorm;
        float moveDist = rdVector_Normalize3(&moveNorm, &pThing->moveDir);

        float distanceMoved = 0.0f;

        if ( pThing->type == SITH_THING_PLAYER )
        {
            moveFlags |= 0x200; // Probably no collision with adjoin which has flag NOPLAYERMOVE set
        }

        if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0
            && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_83
            && pThing->moveStatus != SITHPLAYERMOVE_JEEP_IMPACT )
        {
            // Update Jeep move

            rdVector3 newPos;
            rdVector_ScaleAdd3(&newPos, &pThing->orient.uvec, 0.14f, &pThing->pos);

            SithSector* pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            pThing->pos = newPos;

            if ( pNewSector != pThing->pInSector )
            {
                sithThing_SetSector(pThing, pNewSector, /*bNotify=*/0);
            }

            // Move thing
            distanceMoved = sithCollision_MoveThing(pThing, &moveNorm, moveDist, moveFlags);

            rdVector_ScaleAdd3(&newPos, &pThing->orient.uvec, -0.14f, &pThing->pos);
            pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            pThing->pos = newPos;

            if ( pNewSector != pThing->pInSector )
            {
                sithThing_SetSector(pThing, pNewSector, /*bNotify=*/0);
            }

            sithPhysics_FindFloor(pThing, /*bNoThingStateUpdate=*/0);

            // Update attached things
            for ( SithThing* pAttachedThing = pThing->pAttachedThing; pAttachedThing; pAttachedThing = pAttachedThing->pNextAttachedThing )
            {
                if ( (pAttachedThing->attach.flags & SITH_ATTACH_NOMOVE) != 0 )
                {
                    rdMatrix_TransformVector34(&pAttachedThing->pos, &pAttachedThing->attach.posOffset, &pThing->orient);
                    rdVector_Add3Acc(&pAttachedThing->pos, &pThing->pos);

                    if ( pAttachedThing->pInSector != pThing->pInSector )
                    {
                        sithThing_SetSector(pAttachedThing, pThing->pInSector, /*bNotify=*/0);
                    }
                }
            }
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
        {
            // Update raft move

            rdVector3 upNorm = pThing->orient.uvec;
            if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
            {
                upNorm = pThing->attach.attachedToStructure.pSurfaceAttached->face.normal;
            }

            rdVector3 newPos;
            rdVector_ScaleAdd3(&newPos, &upNorm, 0.08f, &pThing->pos);

            SithSector* pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            pThing->pos = newPos;

            if ( pNewSector != pThing->pInSector )
            {
                sithThing_SetSector(pThing, pNewSector, /*bNotify=*/0);
            }

            distanceMoved = sithCollision_MoveThing(pThing, &moveNorm, moveDist, moveFlags);

            rdVector_ScaleAdd3(&newPos, &upNorm, -0.08f, &pThing->pos);

            pNewSector = sithCollision_FindWaterSector(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            pThing->pos = newPos;

            if ( pNewSector != pThing->pInSector )
            {
                sithThing_SetSector(pThing, pNewSector, /*bNotify=*/0);
            }

            sithPhysics_FindFloor(pThing, /*bNoThingStateUpdate=*/0);
        }
        else if ( pThing->pAttachedThing && (pThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
        {
            // Update tail attach move

            if ( !sithThing_CreateQuetzUserBlock(pThing) )
            {
                return;
            }

            SithQuetzUserBlock* pQuetzUserBlock = pThing->userblock.pQuetz;
            SITH_ASSERTREL(!pQuetzUserBlock->bPosFudged);

            pQuetzUserBlock->bPosFudged = 1;
            pQuetzUserBlock->unknown393 = 0.18000001f;

            rdVector3 newPos;
            rdVector_ScaleAdd3(&newPos, &pThing->orient.uvec, 0.18f, &pThing->pos);

            SithSector* pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            pThing->pos = newPos;
            sithThing_SetSector(pThing, pNewSector, /*bNotify=*/1);

            distanceMoved = sithCollision_MoveThing(pThing, &moveNorm, moveDist, moveFlags);

            rdVector_ScaleAdd3(&newPos, &pThing->orient.uvec, -0.18f, &pThing->pos);

            pNewSector  = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);
            pThing->pos = newPos;
            sithThing_SetSector(pThing, pNewSector, /*bNotify=*/1);

            pQuetzUserBlock->bPosFudged = 0;
            sithPhysics_FindFloor(pThing, 0);
        }
        else
        {
            distanceMoved = sithCollision_MoveThing(pThing, &moveNorm, moveDist, moveFlags);
        }

        pThing->distanceMoved = distanceMoved;
        SITH_ASSERTREL(distanceMoved >= 0.0f);
    }

    if ( pThing->pAttachedThing && (pThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
    {
        sithThing_UpdateQuetzUserBlock(pThing);
    }
}

void J3DAPI sithThing_DestroyDyingThing(SithThing* pThing)
{
    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
            sithActor_DestroyActor(pThing);
            break;

        case SITH_THING_WEAPON:
            sithWeapon_DestroyWeapon(pThing);
            break;

        case SITH_THING_ITEM:
            sithItem_DestroyItem(pThing);
            break;

        case SITH_THING_CORPSE:
            sithActor_DestroyCorpse(pThing);
            break;

        case SITH_THING_PLAYER:
            SITH_ASSERTREL(0);
            break;

        case SITH_THING_PARTICLE:
            sithParticle_DestroyParticle(pThing);
            break;

        default:
            sithThing_DestroyThing(pThing);
            break;
    }
}

SithThing* J3DAPI sithThing_GetThingParent(const SithThing* pThing)
{
    while ( pThing->pParent && pThing->parentSignature == pThing->pParent->signature )
    {
        pThing = pThing->pParent;
    }

    return (SithThing*)pThing;
}

SithThing* J3DAPI sithThing_GetThingByIndex(int idx)
{
    if ( idx >= 0 && idx < sithWorld_g_pCurrentWorld->numThings && sithWorld_g_pCurrentWorld->aThings[idx].type )
    {
        return &sithWorld_g_pCurrentWorld->aThings[idx];
    }

    return NULL;
}

int J3DAPI sithThing_GetThingIndex(const SithThing* pThing)
{
    if ( !pThing )
    {
        return -1;
    }

    if ( pThing < sithWorld_g_pCurrentWorld->aThings || pThing >= &sithWorld_g_pCurrentWorld->aThings[sithWorld_g_pCurrentWorld->numThings] )
    {
        return -1;
    }

    if ( pThing->type != SITH_THING_FREE )
    {
        return pThing->idx;
    }

    return -1;
}

SithThing* J3DAPI sithThing_GetGuidThing(int guid)
{
    if ( guid < 0 )
    {
        return NULL;
    }

    if ( (guid & 0xFFFF0000) == 0 && guid < sithWorld_g_pCurrentWorld->numThings && sithWorld_g_pCurrentWorld->aThings[guid].type )
    {
        return &sithWorld_g_pCurrentWorld->aThings[guid];
    }

    for ( int i = 0; i <= sithWorld_g_pCurrentWorld->lastThingIdx; ++i )
    {
        if ( sithWorld_g_pCurrentWorld->aThings[i].guid == guid && sithWorld_g_pCurrentWorld->aThings[i].type )
        {
            return &sithWorld_g_pCurrentWorld->aThings[i];
        }
    }

    return NULL;
}

void J3DAPI sithThing_DestroyThing(SithThing* pThing)
{
    SITH_ASSERTREL(pThing != NULL);

    pThing->flags |= SITH_TF_DESTROYED;
    sithCog_ThingSendMessage(pThing, 0, SITHCOG_MSG_REMOVED);
}

float J3DAPI sithThing_DamageThing(SithThing* pThing, const SithThing* pDamageThing, float damage, SithDamageType hitType)
{
    SITH_ASSERTREL(pThing);

    if ( damage <= 0.0f || (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return 0.0f;
    }

    float retDamage = (float)(unsigned int)sithCog_ThingSendMessageEx(pThing, pDamageThing, SITHCOG_MSG_DAMAGED, (int32_t)damage, hitType, 0, 0);
    if ( retDamage > 0.0f )
    {
        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
            {
                if ( ((hitType & SITH_DAMAGE_RAZOR_ROCK) != 0 || (hitType & SITH_DAMAGE_RAFT_LEAK) != 0)
                    && (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
                {
                    retDamage = retDamage - sithActor_DamageRaftActor(pThing, (SithThing*)pDamageThing, retDamage, hitType);
                }
                else
                {
                    retDamage = retDamage - sithActor_DamageActor(pThing, (SithThing*)pDamageThing, retDamage, hitType);
                }
            } break;

            case SITH_THING_WEAPON:
                sithWeapon_DamageWeapon(pThing, pDamageThing, retDamage);
                break;

            case SITH_THING_COG:
                sithThing_PlayCogDamageSound(pThing, hitType);
                break;

            default:
                return retDamage;
        }
    }

    return retDamage;
}

float J3DAPI sithThing_ScaleCombatDamage(const SithThing* pThing, SithThing* pHitThing)
{
    if ( !pThing )
    {
        SITHLOG_ERROR("Bad params passed to sithThing_ScaleCombatDamage().\n");
        return 0.0f;
    }

    float damage = 0.0f;
    SithDamageType damageType = 0;
    if ( pThing->type == SITH_THING_WEAPON )
    {
        damageType = pThing->thingInfo.weaponInfo.damageType;
        damage     = pThing->thingInfo.weaponInfo.damage;
    }
    else if ( pThing->type == SITH_THING_EXPLOSION )
    {
        damage = pThing->thingInfo.explosionInfo.damage;
    }

    if ( damage == 0.0f )
    {
        return damage;
    }

    SithThing* pParent = sithThing_GetThingParent(pThing);
    if ( !pParent )
    {
        return damage;
    }

    if ( !pHitThing )
    {
        return damage;
    }

    SithAIControlBlock* pLocal = NULL;
    if ( pHitThing->controlType != SITH_CT_PLAYER )
    {
        if ( pHitThing->controlType != SITH_CT_AI )
        {
            return damage;
        }

        pLocal = pHitThing->controlInfo.aiControl.pLocal;
    }

    if ( pHitThing->type != SITH_THING_ACTOR )
    {
        if ( pHitThing->type != SITH_THING_PLAYER )
        {
            return damage;
        }

        // Hit Player
        return damage * sithGetHitAccuarancyScalar();
    }

    // From here on logic is for hit AI actor

    if ( pParent->type == SITH_THING_ACTOR )
    {
        // Fixed: Use cached damage type instead of pointer to pThing->thingInfo which wasn't initialized for explosion thing
        if ( damageType != SITH_DAMAGE_DROWN
            && damageType != SITH_DAMAGE_CRUSH
            && (pParent->thingInfo.actorInfo.flags & SITH_AF_FULLDAMAGE) == 0 )
        {
            return damage * 0.5f;
        }

        return damage;
    }

    if ( pParent->type != SITH_THING_PLAYER )
    {
        return damage;
    }

    // Player is damaging actor

    int weaponId = sithInventory_GetCurrentWeapon(pParent);
    if ( weaponId < SITHWEAPON_PISTOL || weaponId > SITHWEAPON_SHOTGUN )
    {
        return damage;
    }

    // Note, pThing should be weapon from here on because abouve check makes sure the weapon is not granade or rocket launcher where explosion could damage the actor
    SITH_ASSERT(pThing->type == SITH_THING_WEAPON); // Added: make sure we have weapon here

    float maxAimDist = sithWeapon_GetWeaponMaxAimDistance((SithWeaponId)weaponId);
    float dist       = rdVector_Dist3(&pThing->pos, &pParent->pos) - pParent->collide.size - pHitThing->collide.size;
    if ( (maxAimDist * (sithGetCombatDamageScalar() * 0.25f)) <= dist )
    {
        if ( SITH_RAND() > sithThing_aWeaponCoefficients[weaponId] )
        {
            if ( pHitThing->moveInfo.physics.mass > 0.0f && pHitThing->moveInfo.physics.mass < 50.0f )
            {
                damage *= pHitThing->moveInfo.physics.mass / 50.0f; // TODO: Why?
            }

            if ( !pLocal )
            {
                return damage;
            }

            if ( (pLocal->mode & SITHAI_MODE_TOUGHSKIN) != 0 )
            {
                damage *= sithGetCombatDamageScalar() * 0.75f;
            }

            if ( (pLocal->mode & SITHAI_MODE_ARMOREDSKIN) != 0 )
            {
                damage *= sithGetCombatDamageScalar() * 0.5f;
            }

            if ( (pLocal->mode & (SITHAI_MODE_FLEEING | SITHAI_MODE_UNKNOWN_10)) == 0 )
            {
                return damage;
            }

            if ( dist <= (double)maxAimDist )
            {
                return damage * sithGetCombatDamageScalar() * 0.9f;
            }

            return damage * sithGetCombatDamageScalar() * 0.8f;
        }
    }

    if ( pThing->pTemplate )
    {
        return pThing->pTemplate->thingInfo.weaponInfo.damage; // TODO: Check that there is no case the pThing is explosion
    }

    return damage;
}

void J3DAPI sithThing_PlayCogDamageSound(SithThing* pThing, SithDamageType hitType)
{
    tSoundHandle hSnd = 0;
    if ( (hitType & SITH_DAMAGE_IMPACT) != 0 )
    {
        // Altered: Added impact sound fx for earth, wood, snow thing when cog has no HURTIMPACT soundmode
        if ( sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTIMPACT) == SOUND_INVALIDHANDLE )
        {
        #ifdef J3D_QOL_IMPROVEMENTS
            if ( (pThing->flags & SITH_TF_EARTH) != 0 )
            {
                hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(84)); // gen_machete_hit_vine.wav
            }
            else if ( (pThing->flags & SITH_TF_WOOD) != 0 )
            {
                hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(83)); // gen_machete_hit_wood.wav
            }
            else if ( (pThing->flags & SITH_TF_SNOW) != 0 )
            {
                hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(110)); // fol_in_jumpsnow.wav
            }
            else
            #else
            if ( (pThing->flags & (SITH_TF_EARTH | SITH_TF_SNOW | SITH_TF_WOOD)) == 0 )
            #endif
            {
                int sndIndex = rand() % 2 + 141; // gen_ricochet_a.wav or gen_ricochet_b.wav
                hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(sndIndex));
            }
        }
    }
    else if ( (hitType & SITH_DAMAGE_MACHETE) != 0 && !sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTMACHETE) )
    {
        if ( (pThing->flags & SITH_TF_METAL) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(82)); // 0x8052 - gen_machete_hit_metal.wav
        }
        else if ( (pThing->flags & (SITH_TF_EARTH)) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(84)); // 0x8054 - gen_machete_hit_vine.wav
        }
        // Altered: OG snow was handled via earth sound
        else if ( (pThing->flags & (SITH_TF_SNOW)) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(110)); // fol_in_jumpsnow.wav
        }
        else if ( (pThing->flags & SITH_TF_WOOD) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(83)); // 0x8053 - gen_machete_hit_wood.wav
        }
        else
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(81)); // 0x8051 - gen_machete_hit_stone.wav
        }
    }

    if ( hSnd )
    {
        sithSoundMixer_PlaySoundThing(hSnd, pThing, 1.0f, 0.15f, 5.8f, (SoundPlayFlag)0);
    }
}

int J3DAPI sithThing_AllocWorldThings(SithWorld* pWorld, size_t numThings)
{
    SITH_ASSERTREL((numThings <= STD_ARRAYLEN(sithThing_aFreeThingIdxs)));
    SITH_ASSERTREL(pWorld->aThings == NULL);
    SITH_ASSERTREL(sithThing_bThingStartup);

    // Added: Resized requested buffer size for configured extra capacity
    numThings = J3DMIN(numThings + sithThing_bufferExtraCapacity, STD_ARRAYLEN(sithThing_aFreeThingIdxs));

    pWorld->aThings = (SithThing*)STDMALLOC(sizeof(SithThing) * numThings);
    if ( !pWorld->aThings )
    {
        SITHLOG_ERROR("Failed to allocated mem for %d things.\n", numThings);
        return 1;
    }

    STD_ZEROMEM(pWorld->aThings, sizeof(SithThing) * numThings);
    pWorld->numThings = numThings;

    sithThing_InitializeWorldThings(pWorld);
    return 0;
}

void J3DAPI sithThing_FreeWorldThings(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld);
    if ( pWorld->aThings )
    {
        sithThing_RemoveWorldThings(pWorld);
        STDFREE(pWorld->aThings);

        pWorld->aThings      = NULL;
        pWorld->numThings    = 0;
        pWorld->lastThingIdx = -1;
    }
}

void J3DAPI sithThing_RemoveWorldThings(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld);

    if ( pWorld->aThings )
    {
        for ( size_t i = 0; i < pWorld->numThings; ++i )
        {
            SithThing* pThing = &pWorld->aThings[i];
            if ( pThing->type != SITH_THING_FREE )
            {
                sithThing_RemoveThing(pWorld, pThing);
            }
        }
    }
}

void J3DAPI sithThing_InitializeWorldThings(SithWorld* pWorld)
{
    sithThing_numFreeThings = 0;
    for ( int thingNum = pWorld->numThings - 1; thingNum >= 0; --thingNum )
    {
        sithThing_Reset(&pWorld->aThings[thingNum]);

        pWorld->aThings[thingNum].idx  = thingNum;
        pWorld->aThings[thingNum].guid = -1;
        sithThing_FreeThingIndex(pWorld, thingNum);
    }
}

void J3DAPI sithThing_LoadPostProcess(SithWorld* pWorld)
{
    sithThing_numFreeThings = 0;
    pWorld->lastThingIdx    = -1;

    for ( int thingNum = pWorld->numThings - 1; thingNum >= 0; --thingNum )
    {
        if ( pWorld->aThings[thingNum].type != SITH_THING_FREE )
        {
            if ( thingNum > pWorld->lastThingIdx )
            {
                pWorld->lastThingIdx = thingNum;
            }
        }
        else
        {
            // TODO: Following calls should probably be remove.
            //       The free thing index should have already been put to sithThing_aFreeThingIdxs by sithThing_InitializeWorldThings
            sithThing_FreeThingIndex(pWorld, thingNum);
        }
    }
}

void J3DAPI sithThing_RemoveThing(SithWorld* pWorld, SithThing* pThing)
{
    if ( stdComm_IsGameActive() && stdComm_IsGameHost() && (pThing->guid & 0xFFFF0000) == 0 )
    {
        sithMulti_RemoveStaticThing(pThing->guid);
    }

    sithThing_FreeThing(pWorld, pThing);
    sithThing_FreeThingIndex(pWorld, pThing->idx);
}

void J3DAPI sithThing_FreeThing(SithWorld* pWorld, SithThing* pThing)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing));
    SITH_ASSERTREL((pThing->type != SITH_THING_FREE));

    if ( pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
    }

    sithThing_DetachAttachedThings(pThing);

    if ( pThing->pInSector )
    {
        sithThing_ExitSector(pThing);
    }

    if ( pThing->moveType == SITH_MT_PATH && pThing->moveInfo.pathMovement.aFrames )
    {
        STDFREE(pThing->moveInfo.pathMovement.aFrames);
    }

    if ( pThing->controlType == SITH_CT_AI )
    {
        sithAI_Free(pThing);
    }

    if ( pThing->type == SITH_THING_PARTICLE )
    {
        sithParticle_Free(pThing);
    }

    if ( pThing->pPuppetClass )
    {
        sithPuppet_Free(pThing);
    }

    sithThing_ResetSwapList(pThing);
    rdThing_FreeEntry(&pThing->renderData);
    sithSoundMixer_StopAllSoundsThing(pThing);

    if ( pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR )
    {
        sithVoice_UpdateLipSync(pThing);
    }

    if ( pThing->userblock.pMinecar ) // Note This could UB as different union member could be set
    {
        // Added: Free user block entry
        if ( pThing->moveType == SITH_MT_PHYSICS )
        {
            sithPhysics_FreeUserBlockEntry(pThing);
        }

        STDFREE(pThing->userblock.pMinecar);
        pThing->userblock.pMinecar = NULL;
    }

    pThing->type      = SITH_THING_FREE;
    pThing->signature = 0;
    pThing->guid      = -1;
}

void J3DAPI sithThing_Initialize(const SithWorld* pWorld, SithThing* pThing, int bFindFloor)
{
    SITH_ASSERTREL(pThing);

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        {
            if ( rdVector_IsZero4(&pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top) )
            {
                rdVector_Set4(&pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top, -1.0f, -1.0f, -1.0f, -1.0f);
            }

            sithActor_ResetStateChange(pThing);
            sithWeapon_InitalizeActorWeapon(pThing);
            sithActor_SetDifficulty(pThing);
        } break;

        case SITH_THING_ITEM:
            sithItem_Initialize(pThing);
            break;

        case SITH_THING_EXPLOSION:
            sithExplosion_Initialize(pThing);
            break;

        case SITH_THING_PLAYER:
        {
            if ( rdVector_IsZero4(&pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top) )
            {
                rdVector_Set4(&pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top, -1.0f, -1.0f, -1.0f, -1.0f);
            }

            sithActor_ResetStateChange(pThing);
            sithWeapon_InitalizeActorWeapon(pThing);
        } break;

        case SITH_THING_PARTICLE:
            sithParticle_Initalize(pThing);
            break;

        case SITH_THING_SPRITE:
            sithSprite_Initialize(pThing);
            break;

        default:
            break;
    }

    if ( pThing->renderData.pPuppet )
    {
        sithPuppet_New(pThing);
    }

    if ( pThing->controlType == SITH_CT_AI )
    {
        sithAI_Create(pThing);
    }

    if ( pThing->pSoundClass )
    {
        sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_CREATE);
    }

    if ( bFindFloor && (pWorld->state & SITH_WORLD_STATE_INITIALIZED) != 0 )
    {
        if ( pThing->pCog && (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
        {
            sithCog_SendMessage(pThing->pCog, SITHCOG_MSG_INITIALIZED, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }

        if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & (SITH_PF_WALLSTICK | SITH_PF_FLOORSTICK)) != 0 )
        {
            sithPhysics_FindFloor(pThing, /*bNoThingStateUpdate=*/1);
        }
    }
}

void J3DAPI sithThing_Reset(SithThing* pThing)
{
    int idx      = pThing->idx;
    uint32_t sig = pThing->signature;

    STD_ZEROMEM(pThing, sizeof(SithThing));
    pThing->alpha = 1.0f;

    pThing->orient = rdroid_g_identMatrix34;
    rdThing_NewEntry(&pThing->renderData, pThing);

    pThing->idx       = idx;
    pThing->signature = sig;
}

void J3DAPI sithThing_SetSector(SithThing* pThing, SithSector* pSector, int bNotify)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_g_pCurrentWorld, pSector));

    if ( pThing->pInSector )
    {
        if ( pThing->pInSector == pSector )
        {
            return;
        }

        sithThing_ExitSector(pThing);
    }

    sithThing_EnterSector(pThing, pSector, /*bNoWaterSplash=*/0, bNotify);
}

void J3DAPI sithThing_ExitSector(SithThing* pThing)
{
    if ( sithWorld_g_pCurrentWorld )
    {
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
        SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_g_pCurrentWorld, pThing->pInSector));
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_COGLINKED) != 0 )
    {
        rdVector3 prevPos = pThing->pos;
        if ( (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
        {
            sithCog_SectorSendMessage(pThing->pInSector, pThing, SITHCOG_MSG_EXITED);
        }

        if ( !STD_EQUALMEM(&prevPos, &pThing->pos, sizeof(rdVector3)) ) // TODO: maybe use rdVector_Equal3 to do component wise comparison
        {
            // The position changed, thing has to be in sector
            SITH_ASSERTREL(pThing->pInSector);
            return;
        }
    }

    // Not COG linked or the position wasn't changed after sending exited message
    // Remove thing from sector list

    if ( pThing->pPrevThingInSector )
    {
        pThing->pPrevThingInSector->pNextThingInSector = pThing->pNextThingInSector;
        if ( pThing->pNextThingInSector )
        {
            pThing->pNextThingInSector->pPrevThingInSector = pThing->pPrevThingInSector;
        }
    }
    else
    {
        pThing->pInSector->pFirstThingInSector = pThing->pNextThingInSector;
        if ( pThing->pNextThingInSector )
        {
            pThing->pNextThingInSector->pPrevThingInSector = NULL;
        }
    }

    pThing->pInSector          = NULL;
    pThing->pPrevThingInSector = NULL;
    pThing->pNextThingInSector = NULL;
}

void J3DAPI sithThing_EnterSector(SithThing* pThing, SithSector* pNewSector, int bNoWaterSplash, int bNoNotify)
{
    SITH_ASSERTREL(pThing->pInSector == NULL);
    SITH_ASSERTREL(pNewSector->pFirstThingInSector != pThing);
    SITH_ASSERTREL(pThing->type != SITH_THING_FREE);

    pThing->pNextThingInSector = pNewSector->pFirstThingInSector;
    if ( pThing->pNextThingInSector )
    {
        pThing->pNextThingInSector->pPrevThingInSector = pThing;
    }

    pThing->pPrevThingInSector      = NULL;
    pNewSector->pFirstThingInSector = pThing;
    pThing->pInSector              = pNewSector;

    if ( (pNewSector->flags & SITH_SECTOR_UNDERWATER) != 0
        || (pNewSector->flags & SITH_SECTOR_AETHERIUM) != 0
        && pThing->type == SITH_THING_PLAYER )
    {
        if ( pThing->attach.flags && (pThing->attach.flags & SITH_ATTACH_NOMOVE) == 0 && pThing->moveType == SITH_MT_PHYSICS )
        {
            sithThing_DetachThing(pThing);
        }

        if ( (pThing->flags & SITH_TF_SUBMERGED) == 0 )
        {
            if ( (pNewSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
            {
                sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0);
                sithPlayer_g_bInAetheriumSector = 1;
                sithWeapon_DeselectWeapon(pThing);

                pThing->flags &= ~SITH_TF_SHADOW;
            }
            else
            {
                sithThing_EnterWater(pThing, bNoNotify | bNoWaterSplash);
            }
        }
    }
    else if ( (pThing->flags & SITH_TF_SUBMERGED) != 0 )
    {
        if ( (pNewSector->flags & SITH_SECTOR_AETHERIUM) == 0 )
        {
            sithThing_ExitWater(pThing, bNoNotify | bNoWaterSplash);
        }
    }
    else if ( pThing->type == SITH_THING_PLAYER && sithPlayer_g_bInAetheriumSector == 1 )
    {
        sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/1);
        sithPlayer_g_bInAetheriumSector = 0;
        pThing->flags |= SITH_TF_SHADOW;
    }

    if ( !bNoNotify && (pThing->pInSector->flags & SITH_SECTOR_COGLINKED) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
    {
        sithCog_SectorSendMessage(pThing->pInSector, pThing, SITHCOG_MSG_ENTERED);
    }
}

void J3DAPI sithThing_EnterWater(SithThing* pThing, int bNoSplash)
{
    pThing->flags |= SITH_TF_SUBMERGED;
    if ( pThing->pPuppetClass )
    {
    #ifdef J3D_QOL_IMPROVEMENTS
        // Change mode only if not dead thing
        if ( (pThing->flags & SITH_TF_DYING) == 0 )
        #endif
        {
            sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
        }

        if ( pThing == sithPlayer_g_pLocalPlayerThing && pThing->moveStatus != SITHPLAYERMOVE_PULLINGUP )
        {
            pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
        }
    }

    if ( (pThing->flags & SITH_TF_WATERDESTROYED) != 0 )
    {
        sithThing_DestroyThing(pThing);
        return;
    }

    if ( !bNoSplash )
    {
        if ( pThing->pSoundClass )
        {
            if ( pThing->moveType == SITH_MT_PHYSICS && pThing->moveInfo.physics.velocity.z > 0.75f ) // TODO: note in ExitWater function speed is calculated from velocity
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_ENTERWATERSLOW);
            }
            else
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_ENTERWATER);
            }
        }

        if ( sithPlayer_g_pLocalPlayerThing && (pThing->flags & SITH_TF_SPLASH) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
        {
            if ( pThing->pCog )
            {
                sithCog_SendMessage(pThing->pCog, SITHCOG_MSG_SPLASH, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 1, 0);
            }

            if ( pThing->pCaptureCog )
            {
                sithCog_SendMessage(pThing->pCaptureCog, SITHCOG_MSG_SPLASH, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 1, 0);
            }
        }

        if ( pThing->moveType == SITH_MT_PHYSICS )
        {
            pThing->moveInfo.physics.velocity.z *=  0.25f; // Slowdown fall

            if ( pThing->type == SITH_THING_WEAPON
                && (pThing->moveInfo.physics.flags & SITH_PF_USEGRAVITY) == 0
                && (pThing->thingInfo.weaponInfo.damageType & SITH_DAMAGE_IMPACT) != 0 )
            {
                rdVector_Scale3Acc(&pThing->moveInfo.physics.velocity, 0.5f); // Slowdown weapon
            }
        }
    }
}

void J3DAPI sithThing_ExitWater(SithThing* pThing, int bNoSplash)
{
    pThing->flags &= ~SITH_TF_SUBMERGED;

    int bFalling = 0;
    if ( pThing == sithPlayer_g_pLocalPlayerThing
        && pThing->moveStatus == SITHPLAYERMOVE_SWIMIDLE
        && !pThing->thingInfo.actorInfo.bForceMovePlay
        && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
    {
        bFalling = 1;
        pThing->moveStatus = SITHPLAYERMOVE_FALLING;

        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
        sithPhysics_SetThingLook(pThing, &rdroid_g_zVector3, 0.0f);
    }

    if ( pThing->pSoundClass )
    {
        if ( pThing->moveType == SITH_MT_PHYSICS && rdVector_Len3(&pThing->moveInfo.physics.velocity) < 0.75f )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_EXITWATERSLOW);
        }
        else
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_EXITWATER);
        }
    }

    if ( (pThing->flags & SITH_TF_AIRDESTROYED) != 0 )
    {
        sithThing_DestroyThing(pThing);
        return;
    }

    if ( !bNoSplash
        && (pThing == sithPlayer_g_pLocalPlayerThing)
        && (pThing->flags & SITH_TF_SPLASH) != 0
        && (pThing->flags & SITH_TF_REMOTE) == 0
        && !bFalling )
    {
        if ( sithPlayer_g_pLocalPlayerThing->pCog )
        {
            sithCog_SendMessage(sithPlayer_g_pLocalPlayerThing->pCog, SITHCOG_MSG_SPLASH, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }

        if ( sithPlayer_g_pLocalPlayerThing->pCaptureCog )
        {
            sithCog_SendMessage(sithPlayer_g_pLocalPlayerThing->pCaptureCog, SITHCOG_MSG_SPLASH, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }
}

SithThing* J3DAPI sithThing_Create(SithThingType type)
{
    int thingIndex = sithThing_GetFreeThingIndex();
    if ( thingIndex < 0 && type != SITH_THING_EXPLOSION && type != SITH_THING_DEBRIS && type != SITH_THING_PARTICLE )
    {
        sithThing_PurgeGarbageThings(sithWorld_g_pCurrentWorld);
        thingIndex = sithThing_GetFreeThingIndex();
    }

    if ( thingIndex < 0 )
    {
        SITHLOG_ERROR("Call to sithThing_Create could not be satisfied - no free objects.\n");
        return NULL;
    }

    SithThing* pThing = &sithWorld_g_pCurrentWorld->aThings[thingIndex];
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->type == SITH_THING_FREE);

    sithThing_Reset(pThing);
    pThing->idx = thingIndex;

    if ( !sithThing_guidEntropy )
    {
        sithThing_guidEntropy = 1;
    }

    pThing->guid      = sithThing_guidEntropy++ | ((sithPlayer_g_playerNum + 1) << 16);
    pThing->signature = sithThing_curSignature++;

    if ( !sithThing_curSignature )
    {
        sithThing_curSignature = 1;
    }

    return pThing;
}

void J3DAPI sithThing_SetPositionAndOrient(SithThing* pThing, const rdVector3* pos, const rdMatrix34* pOrient)
{
    SITH_ASSERTREL(pThing->pInSector == NULL);

    pThing->pos    = *pos;
    pThing->orient = *pOrient;
    rdVector_Zero3(&pThing->orient.dvec);
}

int J3DAPI sithThing_SetThingModel(SithThing* pThing, rdModel3* pModel)
{
    SITH_ASSERTREL(pThing && pModel);

    if ( pThing->renderData.type == RD_THING_MODEL3 && pThing->renderData.data.pModel3 == pModel )
    {
        return 0;
    }

    rdPuppet* pPuppet = pThing->renderData.pPuppet;
    pThing->renderData.pPuppet = NULL;

    rdThing_FreeEntry(&pThing->renderData);
    rdThing_NewEntry(&pThing->renderData, pThing);

    rdThing_SetModel3(&pThing->renderData, pModel);
    pThing->renderData.pPuppet = pPuppet;

    return 1;
}

void J3DAPI sithThing_SetThingBasedOn(SithThing* pThing, const SithThing* pTemplate)
{
    SITH_ASSERTREL(pThing != pTemplate);

    int idx                  = pThing->idx;
    int guid                 = pThing->guid;
    uint32_t signature       = pThing->signature;
    SithThing* pRdThingThing = pThing->renderData.pThing;

    if ( !pTemplate )
    {
        sithThing_Reset(pThing);
    }
    else
    {
        // Deep copy assign template to thing
        *pThing = *pTemplate;

        switch ( pThing->renderData.type )
        {
            case RD_THING_MODEL3:
                rdThing_SetModel3(&pThing->renderData, pThing->renderData.data.pModel3);
                break;

            case RD_THING_PARTICLE:
                rdThing_SetParticleCloud(&pThing->renderData, pThing->renderData.data.pParticle);
                break;

            case RD_THING_SPRITE3:
                rdThing_SetSprite3(&pThing->renderData, pThing->renderData.data.pSprite3);
                sithSprite_Create(pThing);
                break;
        }

        if ( pThing->pPuppetClass )
        {
            rdPuppet_New(&pThing->renderData);
        }

        if ( pThing->moveType == SITH_MT_PATH && pThing->moveInfo.pathMovement.aFrames )
        {
            SITH_ASSERTREL(pThing->moveInfo.pathMovement.sizeFrames > 0);
            pThing->moveInfo.pathMovement.aFrames = (SithPathFrame*)STDMALLOC(sizeof(SithPathFrame) * pThing->moveInfo.pathMovement.sizeFrames);
            STD_COPYMEM(pThing->moveInfo.pathMovement.aFrames, pTemplate->moveInfo.pathMovement.aFrames, sizeof(SithPathFrame) * pThing->moveInfo.pathMovement.sizeFrames);
        }
    }

    pThing->idx               = idx;
    pThing->guid              = guid;
    pThing->signature         = signature;
    pThing->renderData.pThing = pRdThingThing;
    pThing->pTemplate         = pTemplate;
}

SithThing* J3DAPI sithThing_CreateThingAtPos(const SithThing* pTemplate, const rdVector3* pos, const rdMatrix34* orient, SithSector* pSector, SithThing* pParent)
{
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_g_pCurrentWorld, pSector));
    SITH_ASSERTREL(pTemplate != NULL);

    SithThing* pThing = sithThing_Create(pTemplate->type);
    if ( !pThing )
    {
        SITHLOG_ERROR("Failed to create thing from template %s.\n", pTemplate->aName); // Altered: Changed log level to error from status
        return NULL;
    }

    if ( pTemplate->aName[0] == '_' )
    {
        SITHLOG_WARNING("Warning -- create object from base-class template %s\n", pTemplate->aName); // Altered: Changed log level to warning from error
    }

    sithThing_SetThingBasedOn(pThing, pTemplate);
    sithThing_SetPositionAndOrient(pThing, pos, orient);
    rdMatrix_PreMultiply34(&pThing->orient, &pTemplate->orient);

    sithThing_EnterSector(pThing, pSector, /*bNoWaterSplash=*/1, /*bNoNotify*/0);

    if ( pParent )
    {
        pThing->pParent         = pParent;
        pThing->parentSignature = pParent->signature;
    }

    sithThing_Initialize(sithWorld_g_pCurrentWorld, pThing, 1);
    if ( pThing->type == SITH_THING_EXPLOSION )
    {
        sithExplosion_CreateSpriteThing(pThing);
    }

    if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & SITH_PF_STARTORIENTMOVE) == 0 )
    {
        rdMatrix_TransformVector34Acc(&pThing->moveInfo.physics.velocity, &pThing->orient);
    }

    if ( pThing->pCog && (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        sithCog_SendMessage(pThing->pCog, SITHCOG_MSG_CREATED, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
    }

    if ( !pThing->pCreateThingTemplate )
    {
        return pThing;
    }

    if ( pThing->type == SITH_THING_EXPLOSION )
    {
        return pThing;
    }

    SithThing* pCreateThing = sithThing_CreateThingAtPos(pThing->pCreateThingTemplate, pos, orient, pSector, pParent);
    if ( !pCreateThing )
    {
        return pThing;
    }

    if ( (pThing->flags & SITH_TF_REMOTE) == 0 )
    {
        return pThing;
    }

    pCreateThing->flags |= SITH_TF_REMOTE;
    return pThing;
}

SithThing* J3DAPI sithThing_CreateThing(const SithThing* pTemplate, SithThing* pMarker)
{
    rdVector3 offset = { 0 };
    switch ( pTemplate->renderData.type )
    {
        case RD_THING_MODEL3:
            offset = pTemplate->renderData.data.pModel3->insertOffset;
            break;
        case RD_THING_SPRITE3:
            offset = pTemplate->renderData.data.pSprite3->offset;
            break;
        default:
            //rdVector_Zero3(&offset);
            break;
    }

    rdVector3 markerOffset = { 0 };
    switch ( pMarker->renderData.type )
    {
        case RD_THING_MODEL3:
            markerOffset = pMarker->renderData.data.pModel3->insertOffset;
            break;
        case RD_THING_SPRITE3:
            markerOffset = pMarker->renderData.data.pSprite3->offset;
            break;
        default:
            //rdVector_Zero3(&markerOffset);
            break;
    }

    rdVector_Sub3Acc(&offset, &markerOffset);

    rdVector3 toffset;
    rdMatrix_TransformVector34(&toffset, &offset, &pMarker->orient);

    rdVector3 pos;
    rdVector_Add3(&pos, &pMarker->pos, &toffset);
    SithSector* pSector = sithCollision_FindSectorInRadius(pMarker->pInSector, &pMarker->pos, &pos, 0.0f);

    SithThing* pNewThing = sithThing_CreateThingAtPos(pTemplate, &pos, &pMarker->orient, pSector, NULL);
    if ( !pNewThing )
    {
        return NULL;
    }

    if ( pNewThing->type == SITH_THING_SPRITE )
    {
        sithSprite_Create(pNewThing);
    }

    if ( pNewThing->moveType == SITH_MT_PATH
        && pMarker->moveType == SITH_MT_PATH
        && pMarker->moveInfo.pathMovement.aFrames
        && !pNewThing->moveInfo.pathMovement.aFrames )
    {
        sithPathMove_Create(pNewThing, pMarker, &offset);
    }

    return pNewThing;
}

void J3DAPI sithThing_AttachThingToSurface(SithThing* pThing, SithSurface* pSurface, int bNoImpactUpdate)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(sithSurface_ValidateSurfacePointer(pSurface));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);

    bool bNoPrevAttach = true;
    if ( pThing->attach.flags )
    {
        if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 && pThing->attach.attachedToStructure.pSurfaceAttached == pSurface )
        {
            return;
        }

        bNoPrevAttach = false;
        sithThing_DetachThing(pThing);
    }

    if ( (pSurface->flags & SITH_SURFACE_LAVA) != 0 )
    {
        if ( pThing->type == SITH_THING_PLAYER && (sithPuppet_g_bPlayerLeapForward == 1 || pThing->moveStatus == SITHPLAYERMOVE_LEAPFWD)
            || (pThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) != 0 )
        {
            goto update_attachment;
        }

        if ( pThing->type == SITH_THING_WEAPON || pThing->type == SITH_THING_ITEM )
        {
            sithThing_DestroyThing(pThing);
        }

        if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR
            || (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0
            || pThing->type == SITH_THING_ACTOR
            && pThing->controlInfo.aiControl.pLocal
            && (pThing->controlInfo.aiControl.pLocal->allowedSurfaceTypes & SITH_SURFACE_LAVA) != 0 )
        {
            goto update_attachment;
        }

        if ( (pSurface->flags & SITH_SURFACE_WATER) != 0 )
        {
            sithActor_KillActor(pThing, NULL, SITH_DAMAGE_COLD_WATER);
        }
        else
        {
            sithActor_KillActor(pThing, NULL, SITH_DAMAGE_LAVA);
        }
    }

    // Handle attaching to killfloor
    if ( (pSurface->flags & SITH_SURFACE_KILLFLOOR) != 0 && (pThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) == 0 )
    {
        if ( pThing->type == SITH_THING_WEAPON )
        {
            sithThing_DestroyThing(pThing);
        }

        if ( (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR) && (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
        {
            sithActor_KillActor(pThing, NULL, SITH_DAMAGE_IMPACT);
        }
    }

update_attachment:
    if ( (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 || (pSurface->flags & SITH_SURFACE_LAVA) == 0 ) // Added: Fixes not attaching dead player to lava surface
    {
        pThing->attach.flags                                = SITH_ATTACH_SURFACE;
        pThing->attach.attachedFaceFirstVert                = sithWorld_g_pCurrentWorld->aVertices[*pSurface->face.aVertices];
        pThing->attach.pFace                                = &pSurface->face;
        pThing->attach.attachedToStructure.pSurfaceAttached = pSurface;
    }

    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;

    if ( (pSurface->flags & SITH_SURFACE_SCROLLING) != 0 && pThing->moveType == SITH_MT_PHYSICS )
    {
        rdVector3 scrollDir;
        sithAnimate_GetSurfaceScrollingDirection(pSurface, &scrollDir);
        rdVector_Sub3Acc(&pThing->moveInfo.physics.velocity, &scrollDir);
    }

    if ( (pSurface->flags & SITH_SURFACE_COGLINKED) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
    {
        sithCog_SurfaceSendMessage(pSurface, pThing, SITHCOG_MSG_ENTERED);
    }

    if ( !bNoImpactUpdate && bNoPrevAttach )
    {
        float impactForce = -(rdVector_Dot3(&pThing->moveInfo.physics.velocity, &pSurface->face.normal));

        if ( pThing->type == SITH_THING_ACTOR && pThing->controlInfo.aiControl.pLocal )
        {
            sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_LAND_FLOOR, pSurface);
        }

        if ( (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
        {
            if ( pThing->moveStatus == SITHPLAYERMOVE_FALLING )
            {
                pThing->moveInfo.physics.velocity.z = 0;
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDWATER);
                if ( sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, sithVehicleControls_PuppetCallback) < 0 )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_RAFT_IDLE;
                }
                else
                {
                    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                    pThing->moveStatus = SITHPLAYERMOVE_LAND;
                }

                sithFX_CreateRaftSplatterFX(pThing, /*bCreateSplash=*/1);
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALL);
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALLFORWARD);
            }
            else
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_IDLE;
            }
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
        {
            if ( pThing->moveInfo.physics.velocity.z < -1.65f )// Note, negative value means downward movement, i.e.: falling
            {
                // TODO: this should be only in debug version
            #ifdef J3D_DEBUG
                if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
                {
                    char aText[256] = { 0 };
                    sprintf_s(aText, STD_ARRAYLEN(aText), "Falling Death Attach: %f.", pThing->moveInfo.physics.velocity.z);
                    sithConsole_PrintString(aText);
                }
            #endif

                if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_JEEP_IMPACT;
                    sithActor_KillActor(pThing, pThing, SITH_DAMAGE_IMPACT);
                }
            }
            else if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_82 ) // SITHPLAYERMOVE_UNKNOWN_82 probably land hard
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, 0);
                pThing->moveStatus = SITHPLAYERMOVE_JEEP_IDLE;
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDHARD);
            }
        }
        else
        {
            // Damage or kill falling thing
            if ( (pThing->moveStatus == SITHPLAYERMOVE_FALLING || !pThing->attach.flags) && impactForce >= 1.45f )
            {
                if ( impactForce >= 1.65f )
                {
                    if ( (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR)
                        && (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
                    {
                        sithActor_KillActor(pThing, pThing, SITH_DAMAGE_IMPACT);
                    }
                }
                else if ( impactForce >= 1.55f )
                {
                    sithThing_DamageThing(pThing, pThing, 500.0f, SITH_DAMAGE_IMPACT);
                }
                else
                {
                    sithThing_DamageThing(pThing, pThing, 200.0f, SITH_DAMAGE_IMPACT);
                }

                if ( pThing->pSoundClass )
                {
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDHURT);
                }
            }

            // Play surface land sound fx
            if ( pThing->pSoundClass )
            {
                if ( (pSurface->flags & (SITH_SURFACE_WOODECHO | SITH_SURFACE_ECHO | SITH_SURFACE_WOOD | SITH_SURFACE_SNOW | SITH_SURFACE_WEB | SITH_SURFACE_EARTH | SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_WATER | SITH_SURFACE_METAL | SITH_SURFACE_AETHERIUM | SITH_SURFACE_EARTHECHO)) != 0 )
                {
                    if ( (pSurface->flags & SITH_SURFACE_METAL) != 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDMETAL);
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_WATER) != 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDWATER);
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_SHALLOWWATER) != 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDPUDDLE);
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_SNOW) != 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDSNOW);
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_WOOD) != 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDWOOD);
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_EARTH) != 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDEARTH);
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_ECHO) != 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDHARDECHO);
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_WOODECHO) != 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDWOODECHO);
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_EARTHECHO) != 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDEARTHECHO);
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_AETHERIUM) != 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDAET);
                    }
                }
                else
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDHARD);
                }
            }

            if ( (pThing->flags & SITH_TF_DYING) == 0
                && (pThing->flags & SITH_TF_DESTROYED) == 0
                && pThing->pPuppetClass
                && pThing->moveType == SITH_MT_PHYSICS
                && pThing->moveStatus != SITHPLAYERMOVE_CRAWLIDLE )
            {
                if ( pThing->moveStatus == SITHPLAYERMOVE_FALLING || pThing->moveInfo.physics.deltaVelocity.z < -0.15000001f )
                {
                    if ( pThing->moveStatus != SITHPLAYERMOVE_SLIDEDOWNFORWARD && pThing->moveStatus != SITHPLAYERMOVE_SLIDEDOWNBACK )
                    {
                        // Calc surface slope
                        float slopeDot = rdVector_Dot3(&pSurface->face.normal, &rdroid_g_zVector3);
                        if ( sithPhysics_CheckSlopeAngle(slopeDot) )// 35 - 45 degrees
                        {
                            // Slide down the slope

                            // Calculate LVec
                            rdVector3 lvec = pThing->orient.lvec;
                            lvec.z = 0.0f;
                            rdVector_Normalize3Acc(&lvec);

                            rdVector3 newLVec = pSurface->face.normal;
                            newLVec.z = 0.0f;
                            rdVector_Normalize3Acc(&newLVec);

                            if ( rdVector_Dot3(&newLVec, &lvec) >= 0.0f )
                            {
                                pThing->moveStatus = SITHPLAYERMOVE_SLIDEDOWNFORWARD;
                            }
                            else
                            {
                                rdVector_Neg3Acc(&newLVec);
                                pThing->moveStatus = SITHPLAYERMOVE_SLIDEDOWNBACK;
                            }

                            // Set lvec to newwly calculated vector
                            pThing->orient.lvec = newLVec;

                            // Calculate RVec from new lvec and up vector
                            rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_g_zVector3);
                            rdVector_Normalize3Acc(&pThing->orient.rvec);

                            // Set uvec to up vector
                            pThing->orient.uvec = rdroid_g_zVector3;
                        }
                        else
                        {
                            if ( sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, sithPlayerControls_PuppetCallback) >= 0 )
                            {
                                pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                            }

                            pThing->moveStatus = SITHPLAYERMOVE_LAND;
                            if ( (pSurface->flags & SITH_SURFACE_SHALLOWWATER) != 0 || (pSurface->flags & SITH_SURFACE_WATER) != 0 )
                            {
                                sithFX_CreateWaterRipple(pThing);
                            }
                        }
                    }

                    sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALL);
                    sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALLFORWARD);
                }

                switch ( pThing->moveStatus )
                {
                    case SITHPLAYERMOVE_JUMPFWD:
                    case SITHPLAYERMOVE_JUMPBACK:
                    case SITHPLAYERMOVE_JUMPROLLBACK:
                    case SITHPLAYERMOVE_JUMPROLLFWD:
                    case SITHPLAYERMOVE_JUMPLEFT:
                    case SITHPLAYERMOVE_JUMPRIGHT:
                    case SITHPLAYERMOVE_LEAPFWD:
                        return;

                    default:
                        if ( pThing->moveStatus == SITHPLAYERMOVE_JUMPFWD || pThing->moveStatus == SITHPLAYERMOVE_JUMPBACK )
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_STILL;
                        }

                        sithPhysics_ResetThingMovement(pThing);
                        break;
                }
            }
        }
    }
}

void J3DAPI sithThing_AttachThingToClimbSurface(SithThing* pThing, SithSurface* pSurface)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(sithSurface_ValidateSurfacePointer(pSurface));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);

    bool bNoPrevAttach = true;
    if ( pThing->attach.flags )
    {
        if ( (pThing->attach.flags & SITH_ATTACH_CLIMBSURFACE) != 0 && pThing->attach.attachedToStructure.pSurfaceAttached == pSurface )
        {
            return;
        }

        bNoPrevAttach = false;
        sithThing_DetachThing(pThing);
    }

    // Assign attachment data
    pThing->attach.flags                                = SITH_ATTACH_CLIMBSURFACE;
    pThing->attach.attachedFaceFirstVert                = sithWorld_g_pCurrentWorld->aVertices[*pSurface->face.aVertices];
    pThing->attach.pFace                                = &pSurface->face;
    pThing->attach.attachedToStructure.pSurfaceAttached = pSurface;

    // Calculate new LVec
    rdVector_Neg3(&pThing->orient.lvec, &pSurface->face.normal);

     // Calculate new RVec
    rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_g_zVector3);
    rdVector_Normalize3Acc(&pThing->orient.rvec);

    // Calculate new UVec
    rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);

    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;

    if ( (pSurface->flags & SITH_SURFACE_SCROLLING) != 0 && pThing->moveType == SITH_MT_PHYSICS )
    {
        rdVector3 scrollDir;
        sithAnimate_GetSurfaceScrollingDirection(pSurface, &scrollDir);
        rdVector_Sub3Acc(&pThing->moveInfo.physics.velocity, &scrollDir);
    }

    if ( (pSurface->flags & SITH_SURFACE_COGLINKED) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
    {
        sithCog_SurfaceSendMessage(pSurface, pThing, SITHCOG_MSG_ENTERED);
    }
}

void J3DAPI sithThing_AttachThingToThingFace(SithThing* pThing, SithThing* pAttachThing, rdFace* pFace, const rdVector3* aVertices, int bNoImpactUpdate)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pAttachThing));
    SITH_ASSERTREL(pFace && aVertices);
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);

    bool bNoPrevAttach = true;
    if ( pThing->attach.flags )
    {
        if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0
            && pThing->attach.attachedToStructure.pThingAttached == pAttachThing
            && pThing->attach.pFace == pFace )
        {
            return;
        }

        bNoPrevAttach = false;
        sithThing_DetachThing(pThing);
    }

    SITH_ASSERTREL(!pThing->pNextAttachedThing);
    SITH_ASSERTREL(!pThing->pPrevAttachedThing);
    SITH_ASSERTREL(!pThing->attach.flags);
    SITH_ASSERTREL(!pThing->attach.attachedToStructure.pThingAttached);

    // Assign attacment data
    pThing->attach.flags                              = SITH_ATTACH_THINGFACE;
    pThing->attach.attachedFaceFirstVert              = aVertices[*pFace->aVertices];
    pThing->attach.pFace                              = pFace;
    pThing->attach.attachedToStructure.pThingAttached = pAttachThing;

    // Add thing to attach list
    sithThing_AddThingToAttachedThings(pThing, pAttachThing);

    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;

    if ( pAttachThing->moveType == SITH_MT_PHYSICS )
    {
        rdVector_Sub3Acc(&pThing->moveInfo.physics.velocity, &pAttachThing->moveInfo.physics.velocity);
    }
    else if ( pAttachThing->moveType == SITH_MT_PATH )
    {
        rdVector_MultAcc3(&pThing->moveInfo.physics.velocity, &pAttachThing->moveInfo.pathMovement.vecDeltaPos, -pAttachThing->moveInfo.pathMovement.moveVel);
    }

    rdVector3 dpos;
    rdVector_Sub3(&dpos, &pThing->pos, &pAttachThing->pos);
    rdMatrix_TransformVectorOrtho34(&pThing->attach.posOffset, &dpos, &pAttachThing->orient);

    if ( (pAttachThing->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
    {
        sithCog_ThingSendMessage(pAttachThing, pThing, SITHCOG_MSG_ENTERED);
    }

    if ( bNoPrevAttach && !bNoImpactUpdate )
    {
        rdVector3 tnormal;
        rdMatrix_TransformVector34(&tnormal, &pFace->normal, &pAttachThing->orient);

        float impactForce = -(rdVector_Dot3(&pThing->moveInfo.physics.velocity, &tnormal));
        if ( (pThing->moveStatus == SITHPLAYERMOVE_FALLING || !pThing->attach.flags) && impactForce >= 1.45f )
        {
            if ( impactForce >= 1.65f )
            {
                if ( (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR) && (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
                {
                    sithActor_KillActor(pThing, pThing, SITH_DAMAGE_IMPACT);
                }
            }
            else if ( impactForce >= 1.55f )
            {
                sithThing_DamageThing(pThing, pThing, 500.0f, SITH_DAMAGE_IMPACT);
            }
            else
            {
                sithThing_DamageThing(pThing, pThing, 200.0f, SITH_DAMAGE_IMPACT);
            }

            if ( pThing->pSoundClass )
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDHURT);
            }
        }

        if ( pThing->pSoundClass )
        {
            if ( (pAttachThing->flags & SITH_TF_METAL) != 0 )
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDMETAL);
            }
            else if ( (pAttachThing->flags & SITH_TF_EARTH) != 0 )
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDEARTH);
            }
            else if ( (pAttachThing->flags & SITH_TF_SNOW) != 0 )
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDSNOW);
            }
            else if ( (pAttachThing->flags & SITH_TF_WOOD) != 0 )
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDWOOD);
            }
            else
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDHARD);
            }
        }

        if ( pThing->pPuppetClass && pThing->moveType == SITH_MT_PHYSICS && pThing->moveStatus != SITHPLAYERMOVE_CRAWLIDLE )
        {
            if ( pThing->moveStatus == SITHPLAYERMOVE_FALLING || pThing->moveInfo.physics.deltaVelocity.z < -0.15000001f )
            {
                if ( sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, sithPlayerControls_PuppetCallback) >= 0 )
                {
                    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                }

                pThing->moveStatus = SITHPLAYERMOVE_LAND;
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALL);
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALLFORWARD);
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_JUMPFWD )
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
            }

            sithPhysics_ResetThingMovement(pThing);
        }
    }
}

void J3DAPI sithThing_AddThingToAttachedThings(SithThing* pThing, SithThing* pAttachThing)
{
    pThing->pNextAttachedThing = pAttachThing->pAttachedThing;
    if ( pThing->pNextAttachedThing )
    {
        SITH_ASSERTREL(pThing->pNextAttachedThing->pPrevAttachedThing == NULL);
        pThing->pNextAttachedThing->pPrevAttachedThing = pThing;
    }

    pThing->pPrevAttachedThing   = NULL;
    pAttachThing->pAttachedThing = pThing;
}

void J3DAPI sithThing_AttachThingToThing(SithThing* pThing, SithThing* pAttachThing)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pAttachThing));

    if ( pThing->attach.flags )
    {
        if ( (pThing->attach.flags & SITH_ATTACH_THING) != 0 && pThing->attach.attachedToStructure.pThingAttached == pAttachThing )
        {
            return;
        }

        sithThing_DetachThing(pThing);
    }

    SITH_ASSERTREL(!pThing->pNextAttachedThing);
    SITH_ASSERTREL(!pThing->pPrevAttachedThing);
    SITH_ASSERTREL(!pThing->attach.flags);
    SITH_ASSERTREL(!pThing->attach.attachedToStructure.pThingAttached);

    pThing->attach.flags = SITH_ATTACH_THING;
    pThing->attach.attachedToStructure.pThingAttached = pAttachThing;
    sithThing_AddThingToAttachedThings(pThing, pAttachThing);

    rdVector3 dpos;
    rdVector_Sub3(&dpos, &pThing->pos, &pAttachThing->pos);
    rdMatrix_TransformVectorOrtho34(&pThing->attach.posOffset, &dpos, &pAttachThing->orient);

    if ( (pAttachThing->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
    {
        sithCog_ThingSendMessage(pAttachThing, pThing, SITHCOG_MSG_ENTERED);
    }
}

void J3DAPI sithThing_AttachThingToThingMesh(SithThing* pThing, SithThing* pAttachThing, int meshNum)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pAttachThing);

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
            pThing->thingInfo.actorInfo.pThingMeshAttached = pAttachThing;
            pThing->thingInfo.actorInfo.attachMeshNum      = meshNum;
            break;

        case SITH_THING_EXPLOSION:
            if ( pThing->thingInfo.explosionInfo.pSprite )
            {
                SithSpriteInfo* pSpriteInfo = &pThing->thingInfo.explosionInfo.pSprite->thingInfo.spriteInfo;
                pSpriteInfo->pThingMeshAttached = pAttachThing;
                pSpriteInfo->attachMeshNum      = meshNum;
            }
            break;

        case SITH_THING_SPRITE:
            pThing->thingInfo.spriteInfo.pThingMeshAttached = pAttachThing;
            pThing->thingInfo.spriteInfo.attachMeshNum      = meshNum;
            break;

        default:
            SITHLOG_ERROR("Trying to attach a invalid thing type to a thing mesh, not allowed!\n");
            break;
    }
}

// Added: Shared validation for thing mesh attach targets
int J3DAPI sithThing_ValidateThingMeshAttach(const SithThing* pAttachThing)
{
    return pAttachThing
        && pAttachThing->type != SITH_THING_FREE
        && pAttachThing->renderData.type == RD_THING_MODEL3;
}

void J3DAPI sithThing_DetachThing(SithThing* pThing)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));

    if ( (pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0 )
    {
        SithThing* pAttached = pThing->attach.attachedToStructure.pThingAttached;
        SITH_ASSERTREL(pAttached);

        if ( pThing->moveType == SITH_MT_PHYSICS )
        {
            if ( pAttached->moveType == SITH_MT_PHYSICS )
            {
                rdVector_Add3Acc(&pThing->moveInfo.physics.velocity, &pAttached->moveInfo.physics.velocity);
            }

            else if ( pAttached->moveType == SITH_MT_PATH )
            {
                rdVector_MultAcc3(&pThing->moveInfo.physics.velocity, &pAttached->moveInfo.pathMovement.vecDeltaPos, pAttached->moveInfo.pathMovement.moveVel);
            }
        }

        if ( pThing->type == SITH_THING_PLAYER
            && (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0
            && (pThing->attach.pFace->flags & RD_FF_3DO_WHIP_AIM) != 0
            && (sithWhip_GetWhipSwingThing() || sithWhip_GetWhipClimbThing()) )
        {
            sithWhip_Reset();
            sithWeapon_SendMessageAim(pThing, /*bAim*/0);
        }

        if ( (pAttached->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
        {
            sithCog_ThingSendMessage(pAttached, pThing, SITHCOG_MSG_EXITED);
        }

        if ( pThing->pPrevAttachedThing )
        {
            pThing->pPrevAttachedThing->pNextAttachedThing = pThing->pNextAttachedThing;
            if ( pThing->pNextAttachedThing )
            {
                pThing->pNextAttachedThing->pPrevAttachedThing = pThing->pPrevAttachedThing;
            }
        }
        else
        {
            pAttached->pAttachedThing = pThing->pNextAttachedThing;
            if ( pThing->pNextAttachedThing )
            {
                pThing->pNextAttachedThing->pPrevAttachedThing = NULL;
            }
        }

        pThing->pPrevAttachedThing = NULL;
        pThing->pNextAttachedThing = NULL;
    }
    else if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 || (pThing->attach.flags & SITH_ATTACH_CLIMBSURFACE) != 0 )
    {
        SithSurface* pSurf = pThing->attach.attachedToStructure.pSurfaceAttached;
        SITH_ASSERTREL(pSurf);

        if ( (pSurf->flags & SITH_SURFACE_SCROLLING) != 0 && pThing->moveType == SITH_MT_PHYSICS )
        {
            rdVector3 scrollDir;
            sithAnimate_GetSurfaceScrollingDirection(pSurf, &scrollDir);
            rdVector_Add3Acc(&pThing->moveInfo.physics.velocity, &scrollDir);
        }

        if ( pThing->type == SITH_THING_PLAYER && (pSurf->flags & SITH_SURFACE_WHIPAIM) != 0 && (sithWhip_GetWhipSwingThing() || sithWhip_GetWhipClimbThing()) )
        {
            sithWhip_Reset();
            sithWeapon_SendMessageAim(pThing, /*bAim*/0);
        }

        if ( (pSurf->flags & SITH_SURFACE_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
        {
            sithCog_SurfaceSendMessage(pSurf, pThing, SITHCOG_MSG_EXITED);
        }
    }

    STD_ZEROMEM(&pThing->attach, sizeof(pThing->attach));
}

void J3DAPI sithThing_DetachAttachedThings(SithThing* pThing)
{
    SITH_ASSERTREL(pThing);

    SithThing* pNextAttach = NULL;
    for ( SithThing* pAttach = pThing->pAttachedThing; pAttach; pAttach = pNextAttach )
    {
        pNextAttach = pAttach->pNextAttachedThing;
        sithThing_DetachThing(pAttach);
    }
}

bool J3DAPI sithThing_IsThingAttachedToThing(SithThing* pThing)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    return (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 || (pThing->attach.flags & SITH_ATTACH_THING) != 0;
}

void J3DAPI sithThing_PurgeGarbageThings(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld && pWorld->aThings);

    size_t count = 0;
    for ( size_t thingNum = 0; thingNum < pWorld->numThings; ++thingNum )
    {
        SithThing* pThing = &pWorld->aThings[thingNum];
        if ( (pThing->flags & SITH_TF_DESTROYED) != 0 || (pThing->type == SITH_THING_DEBRIS || pThing->type == SITH_THING_PARTICLE) && pThing->msecLifeLeft )
        {
            SITHLOG_STATUS("Garbage purging thing %d.\n", pThing->idx);
            sithThing_RemoveThing(pWorld, pThing);
        }

        if ( ++count > SITHTHING_MAXTHINGSTOPURGE )
        {
            break;
        }
    }
}

int J3DAPI sithThing_ReadStaticThingsListText(SithWorld* pWorld, int bSkip)
{
    SITH_ASSERTREL(pWorld != NULL);
    sithThing_curSignature = 1;

    if ( bSkip )
    {
        sithThing_FreeWorldThings(pWorld);
        // TODO: Hmm I thing there's bug here, should return, no?
    }
    else
    {
        SITH_ASSERTREL(pWorld->aThings == NULL);
        SITH_ASSERTREL(pWorld->numThings == 0);
    }

    stdConffile_ReadArgs();
    if ( !streq(stdConffile_g_entry.aArgs[0].argValue, "world") || !streq(stdConffile_g_entry.aArgs[1].argValue, "things") )
    {
        SITHLOG_ERROR("Parse error reading thing template list line %d.\n", stdConffile_GetLineNumber());
        return 1;
    }

    // Alloc things
    int numThings = atoi(stdConffile_g_entry.aArgs[2].argValue); // TODO: Check for negative num
    if ( sithThing_AllocWorldThings(pWorld, numThings) )
    {
        SITHLOG_ERROR("Allocation failure while allocating things array.\n");
        return 1;
    }

    // Read things
    while ( stdConffile_ReadArgs() && !streq(stdConffile_g_entry.aArgs[0].argValue, "end") )
    {
        if ( sithThing_ParseThingPlacement(pWorld) )
        {
            SITHLOG_ERROR("Error parsing thing placement, line %d.\n", stdConffile_GetLineNumber());
        }
    }

    sithThing_LoadPostProcess(pWorld);
    return 0;
}

int J3DAPI sithThing_WriteStaticThingsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    return sithThing_WriteThingsListBinary(fh, pWorld, pWorld->numThings, pWorld->aThings);
}

int J3DAPI sithThing_ReadStaticThingsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t numThings = pWorld->numThings; // Note,sithThing_AllocWorldThings doesn't reset numThings to 0 but cache it anyway
    if ( sithThing_AllocWorldThings(pWorld, numThings) )
    {
        return 1;
    }

    int error = sithThing_ReadThingsListBinary(fh, pWorld, numThings, pWorld->aThings, NULL);
    if ( error )
    {
        return error;
    }

    // Init things
    for ( size_t i = 0; i < numThings; ++i )
    {
        SithThing* pThing = &pWorld->aThings[i];
        pThing->guid      = pThing->idx;
        pThing->signature = sithThing_curSignature++;
        if ( pThing->perfLevel > sithGetPerformanceLevel() )
        {
            sithThing_FreeThing(pWorld, pThing);
        }
    }

    sithThing_LoadPostProcess(pWorld);
    return 0;
}

int J3DAPI sithThing_ParseArg(StdConffileArg* pArg, SithWorld* pWorld, SithThing* pThing)
{
    SITH_ASSERTREL((pArg != NULL) && (pThing != NULL));
    SITH_ASSERTREL((strlen(pArg->argName) > 0));

    size_t adjNum = (size_t)stdHashtbl_Find(sithThing_pParseHashtbl, pArg->argName);
    if ( !adjNum )
    {
        goto parse_error;
    }

    int parseRes = sithThing_ParseThingArg(pArg, pWorld, pThing, adjNum);
    if ( parseRes == SITHTHING_PARSEARG_PARSED )
    {
        return 0;
    }

    if ( parseRes == SITHTHING_PARSEARG_BADSYNTAX )
    {
        return 1;
    }

    int bParsed = 0;
    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_PLAYER:
            bParsed = sithActor_ParseArg(pArg, pThing, adjNum);
            break;

        case SITH_THING_WEAPON:
            bParsed = sithWeapon_ParseArg(pArg, pThing, adjNum);
            break;

        case SITH_THING_ITEM:
            bParsed = sithItem_ParseArg(pArg, pThing, adjNum);
            break;

        case SITH_THING_EXPLOSION:
            bParsed = sithExplosion_ParseArg(pArg, pThing, adjNum);
            break;

        case SITH_THING_PARTICLE:
            bParsed = sithParticle_ParseArg(pArg, pThing, adjNum);
            break;

        default:
            break;
    }

    if ( bParsed )
    {
        return 0;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        bParsed = sithPhysics_ParseArg(pArg, pThing, adjNum);
    }
    else if ( pThing->moveType == SITH_MT_PATH )
    {
        bParsed = sithPathMove_ParseArg(pArg, pThing, adjNum);
    }

    if ( bParsed )
    {
        return 0;
    }

    if ( pThing->controlType == SITH_CT_AI && sithAI_ParseArg(pArg, pThing, adjNum) )
    {
        return 0;
    }

parse_error:
    SITHLOG_ERROR("Bad argument thing %s, %s=%s line %d.\n", pThing->aName, pArg->argName, pArg->argValue, stdConffile_GetLineNumber());
    return 0;
}

int J3DAPI sithThing_ParseThingArg(const StdConffileArg* pArg, SithWorld* pWorld, SithThing* pThing, int adjNum)
{
    switch ( adjNum )
    {
        case SITHTHING_ARG_TYPE:
        {
            pThing->type = sithThing_ParseType(pArg->argValue);
            switch ( pThing->type )
            {
                case SITH_THING_ACTOR:
                    pThing->controlType = SITH_CT_AI;
                    break;

                case SITH_THING_EXPLOSION:
                    pThing->controlType = SITH_CT_EXPLOSION;
                    break;

                case SITH_THING_PLAYER:
                    pThing->controlType = SITH_CT_PLAYER;
                    break;

                case SITH_THING_PARTICLE:
                    pThing->controlType = SITH_CT_PARTICLE;
                    break;

                case SITH_THING_SPRITE:
                case SITH_THING_POLYLINE:
                    pThing->controlType = SITH_CT_PLOT;
                    break;
            }

            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_COLLIDE:
        {
            SithCollideType coltype = strtoul(pArg->argValue, NULL, 10); // Changed: use strtoul instead atoi
            if ( coltype > SITH_COLLIDE_FACE || (errno == ERANGE) )      // Added: Conversion range error check
            {
                goto syntax_error;
            }

            pThing->collide.type = coltype;
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_MOVE:
        {
            if ( streq(pArg->argValue, "physics") )
            {
                pThing->moveType = SITH_MT_PHYSICS;
                return SITHTHING_PARSEARG_PARSED;
            }

            if ( streq(pArg->argValue, "path") )
            {
                pThing->moveType = SITH_MT_PATH;
                return SITHTHING_PARSEARG_PARSED;
            }

            if ( streq(pArg->argValue, "none") )
            {
                pThing->moveType = SITH_MT_NONE;
                return SITHTHING_PARSEARG_PARSED;
            }

            // Couldn't parse move type
            goto syntax_error;
        }
        case SITHTHING_ARG_SIZE:
        {
            float size = strtof(pArg->argValue, NULL); // Changed: use strtof instead atof
            if ( size < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto syntax_error;
            }

            pThing->collide.movesize = size;
            pThing->collide.size     = size;
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_THINGFLAGS:
        {
            int flags;
            if ( sscanf_s(pArg->argValue, "%x", &flags) != 1 )
            {
                goto syntax_error;
            }

            pThing->flags = flags;
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_TIMER:
        {
            float secLifeLeft = strtof(pArg->argValue, NULL); // Changed: use strtof instead atof
            if ( secLifeLeft < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto syntax_error;
            }

            pThing->msecLifeLeft = (int32_t)(secLifeLeft * 1000.0f);
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_LIGHT:
        {
            float range = 0.0f; // TODO: There is a bug in engine with light when range is 0.0, consider setting it to 0.01 - thing flat light
            float red, green, blue;
            if ( sscanf_s(pArg->argValue, "(%f/%f/%f/%f)", &red, &green, &blue, &range) != 4 )
            {
                if ( sscanf_s(pArg->argValue, "(%f/%f/%f)", &red, &green, &blue) != 3 )
                {
                    goto syntax_error;
                }
            }

            pThing->light.color.red   = red;
            pThing->light.color.green = green;
            pThing->light.color.blue  = blue;
            pThing->light.color.alpha = range;

            pThing->light.emitColor.red   = red;
            pThing->light.emitColor.green = green;
            pThing->light.emitColor.blue  = blue;
            pThing->light.emitColor.alpha = range;

            pThing->light.maxRadius = range; // TODO: To add light falloff effect maybe calculate max light same way as for sector point light or add additional format for max light range
            pThing->light.minRadius = range;

            pThing->flags |= SITH_TF_EMITLIGHT;
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_SOUNDCLASS:
        {
            pThing->pSoundClass = sithSoundClass_Load(pWorld, pArg->argValue);
            if ( pThing->pSoundClass )
            {
                return SITHTHING_PARSEARG_PARSED;
            }

            SITHLOG_ERROR("Could not load soundclass %s, line %d.\n", pArg->argValue, stdConffile_GetLineNumber());
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_MODEL3D:
        {
            rdThing_FreeEntry(&pThing->renderData);

            rdModel3* pModel3 = sithModel_Load(pArg->argValue, /*bSkipDefault=*/0);
            if ( !pModel3 )
            {
                SITHLOG_ERROR("Could not load model '%s' specified on line %d.\n", pArg->argValue, stdConffile_GetLineNumber());
                goto syntax_error;
            }

            rdThing_SetModel3(&pThing->renderData, pModel3);

            if ( pThing->collide.size == 0.0f )
            {
                pThing->collide.size = pThing->renderData.data.pModel3->radius;
            }

            if ( pThing->collide.movesize == 0.0f )
            {
                pThing->collide.movesize = pThing->renderData.data.pModel3->radius;
            }

            if ( pThing->collide.unkWidth == 0.0f )
            {
                pThing->collide.width    = pThing->collide.movesize;
                pThing->collide.unkWidth = pThing->collide.width;
            }

            if ( pThing->collide.unkHeight == 0.0f )
            {
                pThing->collide.height    = pThing->collide.movesize;
                pThing->collide.unkHeight = pThing->collide.height;
            }

            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_SPRITE:
        {
            rdThing_FreeEntry(&pThing->renderData);

            rdSprite3* pSprite3 = sithSprite_Load(pWorld, pArg->argValue);
            if ( !pSprite3 )
            {
                SITHLOG_ERROR("Could not create sprite %s, line %d.\n", pArg->argValue, stdConffile_GetLineNumber());
                goto syntax_error;
            }

            rdThing_SetSprite3(&pThing->renderData, pSprite3);
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_PUPPET:
        {
            pThing->pPuppetClass = sithPuppet_LoadPuppetClass(pArg->argValue);
            if ( pThing->pPuppetClass && !pThing->renderData.pPuppet )
            {
                rdPuppet_New(&pThing->renderData);
            }

            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_AICLASS:
        {
            pThing->controlType = SITH_CT_AI;
            pThing->controlInfo.aiControl.pClass = sithAIClass_Load(pWorld, pArg->argValue);
            if ( !pThing->controlInfo.aiControl.pLocal || !pThing->controlInfo.aiControl.pClass )
            {
                return SITHTHING_PARSEARG_PARSED;
            }

            pThing->controlInfo.aiControl.pLocal->pClass       = pThing->controlInfo.aiControl.pClass;
            pThing->controlInfo.aiControl.pLocal->numInstincts = pThing->controlInfo.aiControl.pClass->numInstincts;
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_COG:
        {
            pThing->pCog = sithCog_Load(pWorld, pArg->argValue);
            if ( !pThing->pCog )
            {
                return SITHTHING_PARSEARG_PARSED;
            }

            pThing->pCog->flags |= SITHCOG_LOCAL | SITHCOG_CLASS;
            pThing->flags       |= SITH_TF_COGLINKED;
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_PARTICLE:
        {
            rdParticle* pParticle = sithParticle_Load(pWorld, pArg->argValue);
            if ( !pParticle )
            {
                return SITHTHING_PARSEARG_PARSED;
            }

            rdThing_FreeEntry(&pThing->renderData);
            rdThing_SetParticleCloud(&pThing->renderData, pParticle);
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_MOVESIZE:
        {
            if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
            {
                return SITHTHING_PARSEARG_PARSED;
            }

            float movesize = strtof(pArg->argValue, NULL); // Changed: use strtof instead atof
            if ( movesize < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto syntax_error;
            }

            pThing->collide.movesize = movesize;
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_CREATETHING:
        {
            pThing->pCreateThingTemplate = sithTemplate_GetTemplate(pArg->argValue);
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_ORIENT:
        {
            rdVector3 pyr;
            if ( sscanf_s(pArg->argValue, "(%f/%f/%f)", &pyr.pitch, &pyr.yaw, &pyr.roll) != 3 )
            {
                goto syntax_error;
            }

            rdMatrix_BuildRotate34(&pThing->orient, &pyr);
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_USERVAL:
        {
            float userval = strtof(pArg->argValue, NULL); // Changed: use strtof instead atof
            if ( userval < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto syntax_error;
            }

            pThing->userval = userval;
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_COLLHEIGHT:
        {
            float height = strtof(pArg->argValue, NULL); // Changed: use strtof instead atof
            if ( height < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto syntax_error;
            }

            pThing->collide.height    = height;
            pThing->collide.unkHeight = height;
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_COLLWIDTH:
        {
            float width = strtof(pArg->argValue, NULL); // Changed: use strtof instead atof
            if ( width < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto syntax_error;
            }

            pThing->collide.width    = width;
            pThing->collide.unkWidth = width;
            return SITHTHING_PARSEARG_PARSED;
        }
        case SITHTHING_ARG_PERFLEVEL:
        {
            if ( sscanf_s(pArg->argValue, "%d", &pThing->perfLevel) != 1 )
            {
                goto syntax_error;
            }

            return SITHTHING_PARSEARG_PARSED;
        }

        default:
            return SITHTHING_PARSEARG_NOTPARSED;
    }

syntax_error:
    SITHLOG_ERROR("Bad argument %s=%s, line %d in %s.\n", pArg->argName, pArg->argValue, stdConffile_GetFilename(), stdConffile_GetLineNumber());
    return SITHTHING_PARSEARG_BADSYNTAX;
}

int J3DAPI sithThing_ParseType(const char* pType)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(sithThing_aStrThingTypes); ++i )
    {
        if ( streq(pType, sithThing_aStrThingTypes[i]) ) //streqi maybe?
        {
            return i;
        }
    }

    return SITH_THING_FREE;
}

int J3DAPI sithThing_ValidateThingPointer(const SithWorld* pWorld, const SithThing* pThing)
{
    return pThing && pThing->idx == pThing - pWorld->aThings && pThing->idx < STD_ARRAYLEN(sithThing_aFreeThingIdxs);
}

uint32_t J3DAPI sithThing_CalcThingChecksum(const SithThing* pTemplate, uint32_t seed)
{
    uint32_t checksum;
    checksum = stdUtil_CalcChecksum((const uint8_t*)pTemplate, sizeof(pTemplate), seed);
    checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->type, sizeof(pTemplate->type), checksum);
    checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveType, sizeof(pTemplate->moveType), checksum);
    checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->controlType, sizeof(pTemplate->controlType), checksum);

    if ( pTemplate->moveType == SITH_MT_PHYSICS )
    {
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo, sizeof(pTemplate->moveInfo), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.airDrag, sizeof(pTemplate->moveInfo.physics.airDrag), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.surfDrag, sizeof(pTemplate->moveInfo.physics.surfDrag), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.staticDrag, sizeof(pTemplate->moveInfo.physics.staticDrag), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.mass, sizeof(pTemplate->moveInfo.physics.mass), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.height, sizeof(pTemplate->moveInfo.physics.height), checksum);
    }
    else if ( pTemplate->type == SITH_THING_ACTOR )
    {
        // TODO: set to actor info
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.flags, sizeof(pTemplate->thingInfo.actorInfo.flags), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.health, sizeof(pTemplate->thingInfo.actorInfo.health), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.maxHealth, sizeof(pTemplate->thingInfo.actorInfo.maxHealth), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.jumpSpeed, sizeof(pTemplate->thingInfo.actorInfo.jumpSpeed), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.maxThrust, sizeof(pTemplate->thingInfo.actorInfo.maxThrust), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.maxRotVelocity, sizeof(pTemplate->thingInfo.actorInfo.maxRotVelocity), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.maxHeadVelocity, sizeof(pTemplate->thingInfo.actorInfo.maxHeadVelocity), checksum);
        return stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.maxHeadYaw, sizeof(pTemplate->thingInfo.actorInfo.maxHeadYaw), checksum);
    }
    else if ( pTemplate->type == SITH_THING_WEAPON )
    {
        // TODO: set to weapon info
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.weaponInfo.flags, sizeof(pTemplate->thingInfo.weaponInfo.flags), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.weaponInfo.damage, sizeof(pTemplate->thingInfo.weaponInfo.damage), checksum);
        checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.weaponInfo.minDamage, sizeof(pTemplate->thingInfo.weaponInfo.minDamage), checksum);
        return stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.weaponInfo.range, sizeof(pTemplate->thingInfo.weaponInfo.range), checksum);
    }

    return checksum;
}

void J3DAPI sithThing_SyncThing(SithThing* pThing, SithThingSyncFlags flags)
{
    if ( sithMessage_g_outputstream )
    {
        for ( size_t i = 0; i < sithThing_numUnsyncedThings; ++i )
        {
            if ( sithThing_apUnsyncedThings[i] == pThing )
            {
                sithThing_aSyncFlags[i] |= flags;
                return;
            }
        }

        if ( sithThing_numUnsyncedThings == STD_ARRAYLEN(sithThing_apUnsyncedThings) )
        {
            SITHLOG_ERROR("Too many modified things in a single frame!\n");
        }
        else
        {
            sithThing_apUnsyncedThings[sithThing_numUnsyncedThings] = pThing;
            sithThing_aSyncFlags[sithThing_numUnsyncedThings++] = flags;
        }
    }
}

void sithThing_SyncThings(void)
{
    for ( size_t i = 0; i < sithThing_numUnsyncedThings; ++i )
    {
        if ( (sithThing_aSyncFlags[i] & SITHTHING_SYNC_MOVEPOS) != 0 )
        {
            sithDSSThing_MovePos(sithThing_apUnsyncedThings[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, DPSEND_GUARANTEED);
        }

        if ( (sithThing_aSyncFlags[i] & SITHTHING_SYNC_FULL) != 0 )
        {
            sithDSSThing_ThingFullDescription(sithThing_apUnsyncedThings[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            return;
        }

        if ( (sithThing_aSyncFlags[i] & SITHTHING_SYNC_STATE) != 0 )
        {
            sithDSSThing_UpdateState(sithThing_apUnsyncedThings[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }

        if ( (sithThing_aSyncFlags[i] & SITHTHING_SYNC_POS) != 0 )
        {
            sithDSSThing_Pos(sithThing_apUnsyncedThings[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, 0x00);
        }
    }

    sithThing_numUnsyncedThings = 0;
}

int J3DAPI sithThing_CanSyncThing(const SithThing* pThing)
{
    if ( pThing->type == SITH_THING_FREE || (pThing->flags & SITH_TF_NOSYNC) != 0 ) // Altered: Added check for SITH_TF_NOSYNC flag
    {
        return 0;
    }

    return !pThing->msecLifeLeft || pThing->type != SITH_THING_DEBRIS && pThing->type != SITH_THING_PARTICLE;
}

int J3DAPI sithThing_GetThingMeshIndex(const SithThing* pThing, const char* pMeshName)
{
    // Fixed: Added check for renderData type
    if ( pThing->renderData.type != RD_THING_MODEL3 || !pThing->renderData.data.pModel3 || !pMeshName )
    {
        return -1;
    }

    for ( size_t i = 0; i < pThing->renderData.data.pModel3->aGeos[0].numMeshes; ++i )
    {
        if ( streq(pMeshName, pThing->renderData.data.pModel3->aGeos[0].aMeshes[i].name) ) // TODO: Why using streq? Note, sithThing_GetThingJointIndex is using streqi
        {
            return i;
        }
    }

    return -1;
}

int J3DAPI sithThing_GetThingJointIndex(const SithThing* pThing, const char* pJointName)
{
    // Fixed: Added check for renderData type
    if ( pThing->renderData.type != RD_THING_MODEL3 || !pThing->renderData.data.pModel3 || !pJointName )
    {
        return -1;
    }

    for ( size_t i = 0; i < pThing->renderData.data.pModel3->numHNodes; ++i )
    {
        if ( streqi(pJointName, pThing->renderData.data.pModel3->aHierarchyNodes[i].aName) )
        {
            return i;
        }
    }

    return -1;
}

void J3DAPI sithThing_FreeThingIndex(SithWorld* pWorld, size_t thingNum)
{
    SITH_ASSERTREL((thingNum < STD_ARRAYLEN(sithThing_aFreeThingIdxs)));
    SITH_ASSERTREL(sithThing_numFreeThings < STD_ARRAYLEN(sithThing_aFreeThingIdxs));

    // Update last used thing index
    if ( thingNum == pWorld->lastThingIdx )
    {
        int i;
        for ( i = thingNum - 1; i >= 0 && pWorld->aThings[i].type == SITH_THING_FREE; --i )
        {
            ;
        }

        pWorld->lastThingIdx = i;
    }

    sithThing_aFreeThingIdxs[sithThing_numFreeThings++] = thingNum;
}

int sithThing_GetFreeThingIndex(void)
{
    if ( !sithThing_numFreeThings )
    {
        SITHLOG_WARNING("Warning: out of object space - Create failure.\n"); // Altered: Changed log level to warning from status
        return -1;
    }

    size_t freeThingIndex = sithThing_aFreeThingIdxs[--sithThing_numFreeThings];
    SITH_ASSERTREL((freeThingIndex < sithWorld_g_pCurrentWorld->numThings));
    if ( (int)freeThingIndex > sithWorld_g_pCurrentWorld->lastThingIdx )
    {
        sithWorld_g_pCurrentWorld->lastThingIdx = freeThingIndex;
    }

    return freeThingIndex;
}

int J3DAPI sithThing_ParseThingPlacement(SithWorld* pWorld)
{
    int thingNum = atoi(stdConffile_g_entry.aArgs[0].argValue);
    SITH_ASSERTREL((thingNum >= 0) && (thingNum < (int)pWorld->numThings));

    SithThing* pNewThing = &pWorld->aThings[thingNum];
    SITH_ASSERTREL(pNewThing->type == SITH_THING_FREE);

    SithThing* pTemplate = sithTemplate_GetTemplate(stdConffile_g_entry.aArgs[1].argValue);
    if ( !pTemplate && !streq(stdConffile_g_entry.aArgs[1].argValue, "none") )
    {
        SITHLOG_ERROR("Template %s not found, line %d.\n", stdConffile_g_entry.aArgs[1].argValue, stdConffile_GetLineNumber());
    }

    if ( stdConffile_g_entry.numArgs < 10u )
    {
        SITHLOG_ERROR("Not enough fields on line %d.\n", stdConffile_GetLineNumber());
        return 1;
    }

    rdVector3 pos;
    pos.x = atof(stdConffile_g_entry.aArgs[3].argValue);
    pos.y = atof(stdConffile_g_entry.aArgs[4].argValue);
    pos.z = atof(stdConffile_g_entry.aArgs[5].argValue);

    rdVector3 pyr;
    pyr.x = atof(stdConffile_g_entry.aArgs[6].argValue);
    pyr.y = atof(stdConffile_g_entry.aArgs[7].argValue);
    pyr.z = atof(stdConffile_g_entry.aArgs[8].argValue);

    rdMatrix34 orient;
    rdMatrix_BuildRotate34(&orient, &pyr);

    int secNum = atoi(stdConffile_g_entry.aArgs[9].argValue);
    if ( secNum < 0 || secNum >= pWorld->numSectors )
    {
        SITHLOG_ERROR("Invalid sector number %d specified for thing on line %d.\n", secNum, stdConffile_GetLineNumber());
        return 1;
    }

    SithSector* pSector = sithSector_GetSectorEx(pWorld, secNum);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(pWorld, pSector));

    // Set thing based on template
    sithThing_SetThingBasedOn(pNewThing, pTemplate);

    SITH_ASSERTREL(sithIntersect_IsSphereInSector(pWorld, &pos, ((float)0.0f), pSector));
    sithThing_SetPositionAndOrient(pNewThing, &pos, &orient);
    sithThing_EnterSector(pNewThing, pSector, 1, 1);
    sithThing_Initialize(pWorld, pNewThing, 1);

    pNewThing->guid      = pNewThing->idx; // Note, static thing guid is same as thing index
    pNewThing->signature = sithThing_curSignature++;

    for ( size_t curArg = 10; curArg < stdConffile_g_entry.numArgs; ++curArg )
    {
        if ( sithThing_ParseArg(&stdConffile_g_entry.aArgs[curArg], pWorld, pNewThing) )
        {
            // Error
            return 1;
        }
    }

    if ( pNewThing->perfLevel > sithGetPerformanceLevel() )
    {
        sithThing_FreeThing(pWorld, pNewThing);
        return 0;
    }

    SITH_ASSERTREL(pNewThing != NULL); // TODO: Why this check here???
    STD_STRCPY(pNewThing->aName, stdConffile_g_entry.aArgs[2].argValue);
    return 0;
}

int J3DAPI sithThing_CreateQuetzUserBlock(SithThing* pThing)
{
    if ( pThing->userblock.pQuetz )
    {
        return 1;
    }

    pThing->userblock.pQuetz = (SithQuetzUserBlock*)STDMALLOC(sizeof(SithQuetzUserBlock));
    if ( !pThing->userblock.pQuetz )
    {
        SITHLOG_ERROR("sithThing_CreateQuetzUserBlock failed to allocate.\n");
        return 0;
    }

    SithQuetzUserBlock* pQuetz = pThing->userblock.pQuetz;
    STD_ZEROMEM(pQuetz, sizeof(SithQuetzUserBlock));

    pQuetz->strike.unknown0 = 0;
    pQuetz->strike.unknown3 = 0;

    size_t attNum = 0;
    SithThing* pPrevThing = pThing;
    for ( SithThing* pAttachedThing = pThing->pAttachedThing; pAttachedThing; pAttachedThing = pAttachedThing->pNextAttachedThing )
    {
        if ( (pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
        {
            pQuetz->aAttachInfos[attNum].unknown0 = -0.045000002f;
            pQuetz->aAttachInfos[attNum].unknown2 = -0.045000002f;
            STD_ZEROMEM(&pQuetz->aAttachInfos[attNum].vecUnknown11, sizeof(pQuetz->aAttachInfos[attNum].vecUnknown11));

            if ( attNum )
            {
                if ( attNum == 1 )
                {
                    pQuetz->aAttachInfos[1].unknown1 = pAttachedThing->attach.posOffset.z - pPrevThing->attach.posOffset.z;
                    pQuetz->aAttachInfos[1].vecUnknown11.x = 25.0f;
                    pQuetz->aAttachInfos[1].unknown14      = 0.103f;
                }
                else if ( attNum == 2 )
                {
                    pQuetz->aAttachInfos[2].unknown1       = pAttachedThing->attach.posOffset.z - pPrevThing->attach.posOffset.z;
                    pQuetz->aAttachInfos[2].vecUnknown11.x = 10.0f;
                    pQuetz->aAttachInfos[2].unknown14      = 0.118f;
                }
                else
                {
                    pQuetz->aAttachInfos[attNum].unknown1  = pAttachedThing->attach.posOffset.z - pPrevThing->attach.posOffset.z;
                    pQuetz->aAttachInfos[attNum].unknown14 = 0.12f;
                }
            }
            else
            {
                pQuetz->aAttachInfos[0].unknown1           = pAttachedThing->attach.posOffset.z - 0.02f;
                pQuetz->aAttachInfos[0].unknown0           = 0.0f;
                pQuetz->aAttachInfos[0].vecUnknown11.pitch = 40.0f;
                pQuetz->aAttachInfos[0].unknown14          = 0.055f;
            }

            pQuetz->aAttachInfos[attNum].unknown9 = pQuetz->aAttachInfos[attNum].unknown0 + pQuetz->aAttachInfos[attNum].unknown2;
            if ( pQuetz->aAttachInfos[attNum].unknown9 < 0.0f )
            {
                pQuetz->aAttachInfos[attNum].unknown9 = -pQuetz->aAttachInfos[attNum].unknown9;
            }

            pQuetz->aAttachInfos[attNum].unknown10 = pQuetz->aAttachInfos[attNum].unknown9;

            pQuetz->aAttachInfos[attNum].vecDirection   = pPrevThing->orient.lvec;
            pQuetz->aAttachInfos[attNum].vecDirection.z = 0.0f;
            rdVector_Normalize3Acc(&pQuetz->aAttachInfos[attNum].vecDirection);

            pQuetz->aAttachInfos[attNum].vecDirectionNextAttach   = pAttachedThing->orient.lvec;
            pQuetz->aAttachInfos[attNum].vecDirectionNextAttach.z = 0.0f;
            rdVector_Normalize3Acc(&pQuetz->aAttachInfos[attNum].vecDirectionNextAttach);

            attNum++;
            if ( attNum >= STD_ARRAYLEN(pQuetz->aAttachInfos) )
            {
                return 1;
            }

            ++pQuetz->numAttachInfos;
            pPrevThing = pAttachedThing;
        }
    }

    return 1;
}
