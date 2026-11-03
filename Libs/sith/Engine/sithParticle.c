#include "sithParticle.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdParticle.h>

#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithThing.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#define SITHPARTICLE_MAXWORLDPARTICLES 64u // load particel when now particles allocated

static tHashTable* sithParticle_pHashtable;

rdParticle* J3DAPI sithParticle_CacheFind(const char* pName);
void J3DAPI sithParticle_CacheAdd(rdParticle* pParticle);
void J3DAPI sithParticle_CacheRemove(const rdParticle* pParticle);

void sithParticle_InstallHooks(void)
{
    J3D_HOOKFUNC(sithParticle_Startup);
    J3D_HOOKFUNC(sithParticle_Shutdown);
    J3D_HOOKFUNC(sithParticle_Load);
    J3D_HOOKFUNC(sithParticle_AllocWorldParticles);
    J3D_HOOKFUNC(sithParticle_ParseArg);
    J3D_HOOKFUNC(sithParticle_Update);
    J3D_HOOKFUNC(sithParticle_Initalize);
    J3D_HOOKFUNC(sithParticle_DestroyParticle);
    J3D_HOOKFUNC(sithParticle_Free);
    J3D_HOOKFUNC(sithParticle_FreeWorldParticles);
    J3D_HOOKFUNC(sithParticle_CacheFind);
    J3D_HOOKFUNC(sithParticle_CacheAdd);
    J3D_HOOKFUNC(sithParticle_CacheRemove);
}

void sithParticle_ResetGlobals(void)
{}

int sithParticle_Startup(void)
{
    sithParticle_pHashtable = stdHashtbl_New(128u);
    if ( !sithParticle_pHashtable )
    {
        SITHLOG_ERROR("Failed to allocate memory for particless.\n");
        return 1;
    }

    return 0;
}

void sithParticle_Shutdown(void)
{
    if ( sithParticle_pHashtable )
    {
        stdHashtbl_Free(sithParticle_pHashtable);
        sithParticle_pHashtable = NULL;
    }
}

rdParticle* J3DAPI sithParticle_Load(SithWorld* pWorld, const char* pName)
{
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pName != NULL);

    if ( !pWorld->aParticles && sithParticle_AllocWorldParticles(pWorld, SITHPARTICLE_MAXWORLDPARTICLES) )
    {
        return NULL;
    }

    rdParticle* pParticle = sithParticle_CacheFind(pName);
    if ( pParticle )
    {
        return pParticle;
    }

    if ( pWorld->numParticles >= pWorld->sizeParticles )
    {
        SITHLOG_ERROR("No space to define new particle '%s'.\n", pName);
        return NULL;
    }

    pParticle = &pWorld->aParticles[pWorld->numParticles];

    char aPath[128];
    STD_FORMAT(aPath, "%s%c%s", "misc\\par", '\\', pName);

    if ( !rdParticle_LoadEntry(aPath, pParticle) )
    {
        SITHLOG_ERROR("Load failed for particle file %s.\n", aPath);
        return NULL;
    }

    sithParticle_CacheAdd(pParticle);
    ++pWorld->numParticles;
    return pParticle;
}

int J3DAPI sithParticle_AllocWorldParticles(SithWorld* pWorld, size_t size)
{
    SITH_ASSERTREL(pWorld->aParticles == NULL);
    SITH_ASSERTREL(pWorld->sizeParticles == 0);

    pWorld->aParticles = (rdParticle*)STDMALLOC(sizeof(rdParticle) * size);
    if ( !pWorld->aParticles )
    {
        SITHLOG_ERROR("Failed to allocate particles array.\n");
        return 1;
    }

    pWorld->sizeParticles = size;
    pWorld->numParticles  = 0;
    memset(pWorld->aParticles, 0, sizeof(rdParticle) * pWorld->sizeParticles);
    return 0;
}

int J3DAPI sithParticle_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    switch ( adjNum )
    {
        case SITHTHING_ARG_TYPEFLAGS:
        {
            if ( sscanf_s(pArg->argValue, "%x", &pThing->thingInfo.particleInfo.flags) == 1 )
            {
                return 1;
            }
            return 0;
        }
        case SITHTHING_ARG_MAXTHRUST:
        {
            pThing->thingInfo.particleInfo.growthSpeed = atof(pArg->argValue);
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.growthSpeed >= 0.0f);
            return 1;
        }
        case SITHTHING_ARG_RANGE:
        {
            pThing->thingInfo.particleInfo.maxRadius = atof(pArg->argValue);
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.maxRadius >= 0.0f);
            return 1;
        }
        case SITHTHING_ARG_MATERIAL:
        {
            pThing->thingInfo.particleInfo.pMaterial = sithMaterial_Load(pArg->argValue);
            return 1;
        }
        case SITHTHING_ARG_RATE:
        {
            pThing->thingInfo.particleInfo.timeoutRate = atof(pArg->argValue);
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.timeoutRate >= 0.0f);
            return 1;
        }
        case SITHTHING_ARG_COUNT:
        {
            pThing->thingInfo.particleInfo.numParticles = atoi(pArg->argValue);
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.numParticles <= RDPARTICLE_MAXVERTS);
            return 1;
        }
        case SITHTHING_ARG_ELEMENTSIZE:
        {
            pThing->thingInfo.particleInfo.size = atof(pArg->argValue);
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.size > 0.0f);
            return 1;
        }
        case SITHTHING_ARG_MINSIZE:
        {
            pThing->thingInfo.particleInfo.minRadius = atof(pArg->argValue);
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.minRadius >= 0.0f);
            return 1;
        }
        case SITHTHING_ARG_PITCHRANGE:
        {
            pThing->thingInfo.particleInfo.pitchRange =  atof(pArg->argValue);
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.pitchRange >= 0.0f);
            return 1;
        }
        case SITHTHING_ARG_YAWRANGE:
        {
            pThing->thingInfo.particleInfo.yawRange = atof(pArg->argValue);
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.yawRange >= 0.0f);
            return 1;
        }
    }

    return 0;
}

void J3DAPI sithParticle_Update(SithThing* pThing, float secDeltaTime)
{
    SITH_ASSERTREL(pThing && (pThing->type == SITH_THING_PARTICLE));
    SITH_ASSERTREL(pThing->renderData.data.pParticle);

    SithParticleInfo* pParticle = &pThing->thingInfo.particleInfo;
    pParticle->secElapsed += secDeltaTime;

    if ( (pParticle->flags & SITHPARTICLE_FLAG_RANDOM_CEL_CHANGE) != 0 )
    {
        double dtime = (pParticle->secElapsed * 1000.0 / (double)pThing->msecLifeLeft) * secDeltaTime;
        const size_t numCels = pParticle->pMaterial->numCels;

        for ( size_t i = 0; i < pThing->renderData.data.pParticle->numVertices; ++i )
        {
            int dcel = numCels - pThing->renderData.data.pParticle->aVertMatCelNums[i] - 1;
            if ( dcel )
            {
                if ( (double)dcel * dtime > SITH_RAND() )
                {
                    ++pThing->renderData.data.pParticle->aVertMatCelNums[i];
                }
            }
        }
    }

    if ( (pParticle->flags & SITHPARTICLE_FLAG_TIMEOUT_RATE) != 0 )
    {
        int32_t dverts = J3DMAX((int32_t)(pThing->thingInfo.particleInfo.timeoutRate * 0.15000001f), 1);
        pThing->renderData.data.pParticle->numVertices += dverts;
        if ( pThing->renderData.data.pParticle->numVertices > pThing->thingInfo.particleInfo.numParticles )
        {
            pThing->renderData.data.pParticle->numVertices = pThing->thingInfo.particleInfo.numParticles;
        }
    }

    if ( sithPlayerActions_IsInvisible() )
    {
        sithFX_UpdateFairyDustUserBlock(pThing);
    }

    if ( (pParticle->flags & SITHPARTICLE_FLAG_OUTWARD_EXPANDING) != 0 )
    {
        float growDelta = pParticle->growthSpeed * secDeltaTime;

        float scale;
        if ( pParticle->curGrowthSize >= 0.0099999998f )
        {
            scale = (pParticle->curGrowthSize + growDelta) / pParticle->curGrowthSize;
            pParticle->curGrowthSize = pParticle->curGrowthSize + growDelta;
        }
        else
        {
            scale = 0.0099999998f / pParticle->curGrowthSize;
            pParticle->curGrowthSize = 0.0099999998f;
        }

        rdMatrix34 scaleMat;
        rdVector3 vecScale = { scale, scale, scale };
        rdMatrix_BuildScale34(&scaleMat, &vecScale);
        rdMatrix_PostMultiply34(&pThing->orient, &scaleMat);

        pThing->renderData.data.pParticle->radius = pThing->renderData.data.pParticle->radius * scale;
    }
}

void J3DAPI sithParticle_Initalize(SithThing* pThing)
{
    SITH_ASSERTREL(pThing && (pThing->type == SITH_THING_PARTICLE));

    SithParticleInfo* pInfo = &pThing->thingInfo.particleInfo;

    if ( pThing->renderData.type == RD_THING_PARTICLE )
    {
        rdParticle* pParticle = rdParticle_Duplicate(pThing->renderData.data.pParticle);
        rdThing_SetParticleCloud(&pThing->renderData, pParticle);

        pInfo->pMaterial     = pParticle->pMaterial;
        pInfo->numParticles  = pParticle->numVertices;
        pInfo->curGrowthSize = 1.0f;

        if ( (pThing->thingInfo.particleInfo.flags & SITHPARTICLE_FLAG_TIMEOUT_RATE) != 0 )
        {
            pThing->renderData.data.pParticle->numVertices = 1;
        }
        return;
    }

    rdLightMode lightMode =  (pInfo->flags & SITHPARTICLE_FLAG_EMIT_LIGHT) != 0
        ? RD_LIGHTING_NONE
        : RD_LIGHTING_DIFFUSE;

    rdParticle* pParticle = rdParticle_New(pInfo->numParticles, pInfo->size, pInfo->pMaterial, lightMode);
    if ( pParticle )
    {
        rdThing_SetParticleCloud(&pThing->renderData, pParticle);

        float pitch = 720.0f;
        if ( pInfo->pitchRange != 0.0f )
        {
            pitch = pInfo->pitchRange * 2.0f;
        }

        float yaw  = 720.0f;
        if ( pInfo->yawRange != 0.0f )
        {
            yaw = pInfo->yawRange * 2.0f;
        }

        float dradius = pInfo->maxRadius - pInfo->minRadius;
        if ( dradius <= 0.0f )
        {
            dradius = 0.0f;
        }

        const size_t numCels =  pInfo->pMaterial->numCels;
        for ( size_t i = 0; i < pThing->renderData.data.pParticle->numVertices; ++i )
        {
            rdVector3 pyr;
            pyr.pitch = (SITH_RANDF() - 0.5f) * pitch;
            pyr.yaw   = (SITH_RANDF() - 0.5f) * yaw;
            pyr.roll  = 0.0f;

            rdVector_Rotate3(&pThing->renderData.data.pParticle->aVerticies[i], &rdroid_g_yVector3, &pyr);

            float scale = SITH_RANDF() * dradius + pThing->thingInfo.particleInfo.minRadius;
            rdVector_Scale3Acc(&pThing->renderData.data.pParticle->aVerticies[i], scale);

            if ( numCels > 1 && (pInfo->flags & SITHPARTICLE_FLAG_RANDOM_START_CEL) != 0 )
            {
                size_t celNum = (size_t)(SITH_RAND() * (double)numCels);
                if ( celNum >= numCels - 1 )
                {
                    celNum = numCels - 1;
                }

                pThing->renderData.data.pParticle->aVertMatCelNums[i] = celNum;
            }
            else
            {
                pThing->renderData.data.pParticle->aVertMatCelNums[i] = -1;
            }
        }

        pThing->renderData.data.pParticle->radius = pInfo->maxRadius;
        pInfo->curGrowthSize = 1.0f;
        if ( (pThing->thingInfo.particleInfo.flags & SITHPARTICLE_FLAG_TIMEOUT_RATE) != 0 )
        {
            pThing->renderData.data.pParticle->numVertices = 1;
        }

        return;
    }
}

void J3DAPI sithParticle_DestroyParticle(SithThing* pThing)
{
    SITH_ASSERTREL(pThing && (pThing->type == SITH_THING_PARTICLE));
    pThing->thingInfo.particleInfo.flags &= ~SITHPARTICLE_FLAG_TIMEOUT_RATE;

    size_t dverts;
    if ( (pThing->thingInfo.particleInfo.flags & SITHPARTICLE_FLAG_FADE_OUT_OVER_TIME) != 0
        && ((size_t)(pThing->thingInfo.particleInfo.timeoutRate * 0.1f)
            ? (dverts = (size_t)(pThing->thingInfo.particleInfo.timeoutRate * 0.1f))
            : (dverts = 1),
            dverts < pThing->renderData.data.pParticle->numVertices) )
    {
        pThing->renderData.data.pParticle->numVertices -= dverts;
        pThing->msecLifeLeft = (uint32_t)(SITH_RAND() * 100.0) + 1;
    }
    else
    {
        sithThing_DestroyThing(pThing);
    }
}

void J3DAPI sithParticle_Free(SithThing* pThing)
{
    if ( pThing->renderData.data.pParticle )
    {
        rdParticle_Free(pThing->renderData.data.pParticle);
        pThing->renderData.data.pParticle = NULL;
    }
}

void J3DAPI sithParticle_FreeWorldParticles(SithWorld* pWorld)
{

    for ( size_t i = 0; i < pWorld->numParticles; ++i )
    {
        sithParticle_CacheRemove(&pWorld->aParticles[i]);
        rdParticle_FreeEntry(&pWorld->aParticles[i]);
    }

    stdMemory_Free(pWorld->aParticles);
    pWorld->aParticles    = NULL;
    pWorld->sizeParticles = 0;
    pWorld->numParticles  = 0;
}

rdParticle* J3DAPI sithParticle_CacheFind(const char* pName)
{
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(sithParticle_pHashtable != NULL);

    return (rdParticle*)stdHashtbl_Find(sithParticle_pHashtable, pName);
}

void J3DAPI sithParticle_CacheAdd(rdParticle* pParticle)
{
    SITH_ASSERTREL(pParticle != NULL);
    SITH_ASSERTREL(strlen(pParticle->aName) > 0);
    SITH_ASSERTREL(sithParticle_pHashtable != NULL);

    (tLinkListNode*)stdHashtbl_Add(sithParticle_pHashtable, pParticle->aName, pParticle);
}

void J3DAPI sithParticle_CacheRemove(const rdParticle* pParticle)
{
    SITH_ASSERTREL(pParticle != NULL);
    SITH_ASSERTREL(strlen(pParticle->aName) > 0);
    SITH_ASSERTREL(sithParticle_pHashtable != NULL);

    stdHashtbl_Remove(sithParticle_pHashtable, pParticle->aName);
}
