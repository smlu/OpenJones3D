#include "rdThing.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Primitives/rdParticle.h>
#include <rdroid/Primitives/rdPolyline.h>
#include <rdroid/Primitives/rdSprite.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

void rdThing_ResetGlobals(void)
{}

rdThing* J3DAPI rdThing_New(SithThing* pThing)
{
    rdThing* prdThing = (rdThing*)STDMALLOC(sizeof(rdThing));
    if ( !prdThing )
    {
        RDLOG_ERROR("Error allocating memory for thing.\n");
        return NULL;
    }

    rdThing_NewEntry(prdThing, pThing);
    return prdThing;
}

void J3DAPI rdThing_NewEntry(rdThing* prdThing, SithThing* pThing)
{
    prdThing->data.pModel3        = NULL;
    prdThing->type                = RD_THING_NONE;
    prdThing->pPuppet             = NULL;
    prdThing->bSkipBuildingJoints = 0;
    prdThing->rdFrameNum          = 0;
    prdThing->geosetNum           = -1;
    prdThing->matCelNum           = -1;
    prdThing->lightMode           = RD_LIGHTING_GOURAUD;
    prdThing->paJointMatrices     = NULL;
    prdThing->pThing              = pThing;
}

void J3DAPI rdThing_Free(rdThing* pThing)
{
    if ( !pThing )
    {
        RDLOG_ERROR("Warning: attempt to free NULL thing ptr.\n");
        return;
    }

    rdThing_FreeEntry(pThing);
    stdMemory_Free(pThing);
}

void J3DAPI rdThing_FreeEntry(rdThing* pThing)
{
    if ( pThing->type == RD_THING_MODEL3 )
    {
        if ( pThing->paJointMatrices )
        {
            stdMemory_Free(pThing->paJointMatrices);
            pThing->paJointMatrices = NULL;
        }

        if ( pThing->apTweakedAngles )
        {
            stdMemory_Free(pThing->apTweakedAngles);
            pThing->apTweakedAngles = NULL;
        }

        if ( pThing->paJointAmputationFlags )
        {
            stdMemory_Free(pThing->paJointAmputationFlags);
            pThing->paJointAmputationFlags = NULL;
        }
    }
    else if ( pThing->type == RD_THING_POLYLINE )
    {
        rdPolyline_Free(pThing->data.pPolyline);
    }

    if ( pThing->pPuppet )
    {
        rdPuppet_Free(pThing->pPuppet);
        pThing->pPuppet = NULL;
    }
}

int J3DAPI rdThing_SetModel3(rdThing* pThing, rdModel3* pModel3)
{
    RD_ASSERTREL(pThing != NULL);
    RD_ASSERTREL(pModel3 != NULL);

    pThing->type            = RD_THING_MODEL3;
    pThing->data.pModel3    = pModel3;
    pThing->geosetNum       = -1;
    pThing->paJointMatrices = (rdMatrix34*)STDMALLOC(sizeof(rdMatrix34) * pModel3->numHNodes);
    pThing->apTweakedAngles = (rdVector3*)STDMALLOC(sizeof(rdVector3) * pModel3->numHNodes);
    STD_ZEROMEM(pThing->apTweakedAngles, sizeof(rdVector3) * pModel3->numHNodes);

    if ( !pThing->paJointMatrices || !pThing->apTweakedAngles )
    {
        RDLOG_ERROR("Error allocating memory for joint matrices or tweaked angles.\n");
        return 0;
    }

    pThing->paJointAmputationFlags = (uint32_t*)STDMALLOC(sizeof(uint32_t) * pModel3->numHNodes);
    if ( !pThing->paJointAmputationFlags )

    {
        RDLOG_ERROR("Error allocating memory for joint amputation flags.\n");
        return 0;
    }

    STD_ZEROMEM(pThing->paJointAmputationFlags, sizeof(*pThing->paJointAmputationFlags) * pModel3->numHNodes);

    rdModel3HNode* pCurNode = pModel3->aHierarchyNodes;
    for ( size_t i = 0; i < pModel3->numHNodes; ++i )
    {
        rdMatrix_Build34(&pCurNode->meshOrient, &pCurNode->pyr, &pCurNode->pos);
        ++pCurNode;
    }

    return 1;
}

int J3DAPI rdThing_SetCamera(rdThing* pThing, rdCamera* pCamera)
{
    RD_ASSERTREL(pThing != NULL);
    RD_ASSERTREL(pCamera != NULL);

    pThing->type         = RD_THING_CAMERA;
    pThing->data.pCamera = pCamera;
    return 1;
}

int J3DAPI rdThing_SetLight(rdThing* pThing, rdLight* pLight)
{
    RD_ASSERTREL(pThing != NULL);
    RD_ASSERTREL(pLight != NULL);

    pThing->type        = RD_THING_LIGHT;
    pThing->data.pLight = pLight;
    return 1;
}

int J3DAPI rdThing_SetSprite3(rdThing* pThing, rdSprite3* pSprite3)
{
    RD_ASSERTREL(pThing != NULL);
    RD_ASSERTREL(pSprite3 != NULL);

    pThing->type          = RD_THING_SPRITE3;
    pThing->data.pSprite3 = pSprite3;
    pThing->matCelNum     = -1;
    return 1;
}

int J3DAPI rdThing_SetPolyline(rdThing* pThing, rdPolyline* pPolyline)
{
    RD_ASSERTREL(pThing != NULL);
    RD_ASSERTREL(pPolyline != NULL);

    pThing->type         = RD_THING_POLYLINE;
    pThing->data.pModel3 = (rdModel3*)pPolyline;
    pThing->matCelNum    = -1;
    return 1;
}

int J3DAPI rdThing_SetParticleCloud(rdThing* pThing, rdParticle* pParticle)
{
    RD_ASSERTREL(pThing != NULL);
    RD_ASSERTREL(pParticle != NULL);

    pThing->type           = RD_THING_PARTICLE;
    pThing->data.pParticle = pParticle;
    return 1;
}

int J3DAPI rdThing_Draw(rdThing* pThing, const rdMatrix34* pOrient)
{
    RD_ASSERTREL(pThing);
    if ( rdroid_g_curGeometryMode == RD_GEOMETRY_NONE )
    {
        return 0;
    }

    switch ( pThing->type )
    {
        case RD_THING_NONE:
            return 0;
            break;

        case RD_THING_MODEL3:
            return rdModel3_Draw(pThing, pOrient);
            break;

        case RD_THING_CAMERA:
            return 0;
            break;

        case RD_THING_LIGHT:
            return 0;
            break;

        case RD_THING_SPRITE3:
            return rdSprite_Draw(pThing, pOrient);
            break;

        case RD_THING_PARTICLE:
            return rdParticle_Draw(pThing, pOrient);
            break;

        case RD_THING_POLYLINE:
            return rdPolyline_Draw(pThing, pOrient);
            break;
    }

    return 0;
}

void J3DAPI rdThing_AccumulateMatrices(rdThing* pThing, const rdModel3HNode* pNode, const rdMatrix34* pPlacement)
{
    rdMatrix34 mat;
    rdMatrix_BuildTranslate34(&mat, &pNode->pivot);
    rdMatrix_PostMultiply34(&mat, &pThing->paJointMatrices[pNode->num]);
    if ( pNode->pParent )
    {
        rdVector3 tvec = RDVECTOR_NEG3(pNode->pParent->pivot);
        rdMatrix_PostTranslate34(&mat, &tvec);
    }

    rdMatrix_Multiply34(&pThing->paJointMatrices[pNode->num], pPlacement, &mat);
    if ( pNode->numChildren )
    {
        rdModel3HNode* pChild = pNode->pChild;
        for ( size_t i = 0; i < pNode->numChildren; ++i )
        {
            if ( !pThing->paJointAmputationFlags[pChild->num] )
            {
                rdThing_AccumulateMatrices(pThing, pChild, &pThing->paJointMatrices[pNode->num]);
            }

            pChild = pChild->pSibling;
        }
    }
}
