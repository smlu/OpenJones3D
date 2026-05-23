#include "sithIntersect.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Main/sithMain.h>
#include <sith/World/sithSector.h>
#include <sith/RTI/symbols.h>

#include <std/General/stdMath.h>

#define LOG_ST_INTERSECTION(pThing, pCheck, hitType, hitDist, hitNorm, pHitMesh, pHitFace) SITHLOG_DEBUG("pThing=%-10s pCheck=%-10s hitType=0x%x hitDist=%f hitNorm=(%f/%f/%f) bHitMesh=%d bHitFace=%d\n", pThing ? pThing->aName : "NULL", pCheck->aName, hitType, hitDist, (hitNorm).x, (hitNorm).y, (hitNorm).z, pHitMesh != NULL, pHitFace != NULL)

static const size_t sithIntersect_aFaceProjectionAxes[3][2] = { { 2, 1 }, { 0, 2 }, { 1, 0 } };

void sithIntersect_InstallHooks(void)
{
    J3D_HOOKFUNC(sithIntersect_IsSphereInSector);
    J3D_HOOKFUNC(sithIntersect_CheckFaceVerticesIntersection);
    J3D_HOOKFUNC(sithIntersect_CheckSphereThingIntersection);
    J3D_HOOKFUNC(sithIntersect_CheckSphereThingModelIntersection);
    J3D_HOOKFUNC(sithIntersect_CheckSphereMeshIntersection);
    J3D_HOOKFUNC(sithIntersect_CheckSphereIntersection);
    J3D_HOOKFUNC(sithIntersect_TestSphereFaceHit);
    J3D_HOOKFUNC(sithIntersect_CheckSphereFaceHitVerticesIntersection);
    J3D_HOOKFUNC(sithIntersect_CheckSphereHit);
    J3D_HOOKFUNC(sithIntersect_CheckSphereFaceIntersectionEx);
    J3D_HOOKFUNC(sithIntersect_CheckSphereFaceIntersection);
}

void sithIntersect_ResetGlobals(void)
{}

void J3DAPI sithIntersect_GetProjectionAxes(const rdVector3* pNormal, int* pOffs1, int* pOffs2);

int J3DAPI sithIntersect_IsSphereInSector(const SithWorld* pWorld, const rdVector3* pos, float radius, const SithSector* pSector)
{
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(pWorld, pSector));
    SITH_ASSERTREL(pos != NULL);
    SITH_ASSERTREL(radius >= 0.0f);

    if ( (pSector->flags & SITH_SECTOR_HASCOLLIDEBOX) != 0
        && pos->z - radius > pSector->collideBox.v0.z
        && pos->y - radius > pSector->collideBox.v0.y
        && pos->x - radius > pSector->collideBox.v0.x
        && pos->x + radius < pSector->collideBox.v1.x
        && pos->y + radius < pSector->collideBox.v1.y
        && pos->z + radius < pSector->collideBox.v1.z )
    {
        return 1;
    }

    SithSurface* pCurSurf = pSector->pFirstSurface;
    for ( size_t i = 0; i < pSector->numSurfaces; ++i )
    {
        if ( (pCurSurf->flags & SITH_SURFACE_COLLISION) != 0 || pCurSurf->pAdjoin && (pCurSurf->pAdjoin->flags & SITH_ADJOIN_MOVE) != 0 )
        {
            rdVector3* pVert = &pWorld->aVertices[pCurSurf->face.aVertices[0]];
            float dist = rdMath_DistancePointToPlane(pos, &pCurSurf->face.normal, pVert);
            dist = stdMath_ClipNearZero(dist);
            if ( dist < radius )
            {
                return 0;
            }
        }

        ++pCurSurf;
    }

    return 1;
}

SithCollisionType J3DAPI sithIntersect_CheckFaceVerticesIntersection(const rdVector3* startPos, float radius, const rdFace* pFace, const rdVector3* aVertices, rdVector3* pHitPos)
{
    int hitmask = 0; // Added init to 0
    if ( !sithIntersect_TestSphereFaceHit(startPos, radius, pFace, aVertices, &hitmask) )
    {
        return 0;
    }

    if ( !hitmask )
    {
        return SITHCOLLISION_FACE;
    }

    SITH_ASSERTREL(radius > 0.0f);
    return sithIntersect_CheckSphereFaceHitVerticesIntersection(startPos, radius, pFace, aVertices, hitmask, pHitPos);
}


SithCollisionType J3DAPI sithIntersect_CheckSphereThingIntersection(SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, SithThing* pCheck, int colflags, float* pHitDistance, rdModel3Mesh** ppHitMesh, rdFace** ppHitFace, rdVector3* hitNorm)
{
    SITH_ASSERTREL(pCheck->collide.type != SITH_COLLIDE_NONE);
    SITH_ASSERTREL(pHitDistance != NULL);
    SITH_ASSERTREL(ppHitMesh && ppHitFace);

    if ( (pCheck->flags & SITH_TF_JEEPSTOP) != 0 )
    {
        if ( !pThing || pThing->type != SITH_THING_PLAYER )
        {
            return 0;
        }

        if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
        {
            if ( rdVector_Dot3(&pCheck->orient.lvec, &pThing->moveInfo.physics.velocity) >= 0.0f ) // if not facing same direction
            {
                return 0;
            }
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_MINECAR) == 0 )
        {
            return 0;
        }
    }

    bool bFaceCol = false;
    if ( (colflags & 0x80) == 0 && (pCheck->collide.type == SITH_COLLIDE_FACE || pThing && pThing->collide.type == SITH_COLLIDE_FACE) )
    {
        bFaceCol = true;
    }

    float hitDistance = 0.0f;

    // TODO: [DEAD] Note, this is additional code found in debug version that is skipped.
    //       The issue is that the vox code can be too aggressive in some cases, causing collision to block movement.
    //       e.g.: sol intro cutscene the minecar gets stuck/stopped after entering the scene.
    //if ( true && pThing ) // maybe special collflag could be tested or box collide type
    if ( false )
    {
        if ( pCheck->collide.height > 0.0f )
        {
            if ( !sithIntersect_CheckThingBoundBoxIntersection(pThing, pCheck, moveNorm, moveDist, &hitDistance) )
            {
                return 0;
            }
        }
        else
        {
            if ( !sithIntersect_CheckSphereIntersection(startPos, moveNorm, moveDist, pThing->collide.height, &pCheck->pos, pCheck->collide.size, &hitDistance, bFaceCol, colflags) )
            {
                if ( !sithIntersect_CheckSphereIntersection(startPos, moveNorm, moveDist, pThing->collide.width, &pCheck->pos, pCheck->collide.size, &hitDistance, bFaceCol, colflags) )
                {
                    return 0;
                }
            }
        }
    }
    else
    {
        if ( !sithIntersect_CheckSphereIntersection(startPos, moveNorm, moveDist, radius, &pCheck->pos, pCheck->collide.size, &hitDistance, bFaceCol, colflags) )
        {
            return 0;
        }
    }

    if ( !bFaceCol )
    {
        rdVector_ScaleAdd3(hitNorm, moveNorm, hitDistance, startPos);
        rdVector_Sub3Acc(hitNorm, &pCheck->pos);
        rdVector_Normalize3Acc(hitNorm);

        *pHitDistance = hitDistance;
        return SITHCOLLISION_THING;
    }

    rdVector3 dir, pos;
    bool bThingFaceCol = false;
    SithThing* pComplex = NULL;

    if ( pCheck->collide.type == SITH_COLLIDE_FACE )
    {
        pComplex      = pCheck;
        bThingFaceCol = false;

        dir = *moveNorm;
        pos = *startPos;
    }
    else
    {
        SITH_ASSERTREL(pThing && (pThing->collide.type != SITH_COLLIDE_SPHERE));

        pComplex     = pThing;
        radius       = pCheck->collide.size;
        bThingFaceCol = true;

        rdVector_Neg3(&dir, moveNorm);
        pos = pCheck->pos;
    }

    SITH_ASSERTREL(pComplex->renderData.type == RD_THING_MODEL3);

    rdMatrix34 tmat;
    pComplex->orient.dvec = pComplex->pos;
    rdMatrix_InvertOrtho34(&tmat, &pComplex->orient);

    // Fixed: Cleared previously assigned dvec.
    //        OG code was missing this step, and could lead to incorrect transform
    //        in any other place in engine that depended on dvec being zeroed out.
    rdVector_Zero3(&pComplex->orient.dvec);

    // Transform to world space
    rdMatrix_TransformPoint34Acc(&pos, &tmat);
    rdMatrix_TransformVector34Acc(&dir, &tmat);

    // TODO: [DEAD] following dead code (in if scope) was found in debug version
    // Fixed: Keep hitType initialized even if debug-only collision branches change in the future.
    SithCollisionType hitType = 0;
    if ( false )
    {
        float heightHitDistance, widthHitDistance;
        rdFace* heightHitFace;
        rdFace* widthHitFace;
        rdModel3Mesh* heightHitMesh;
        rdModel3Mesh* widthHitMesh;
        rdVector3 heightHitNormal, widthHitNormal;

        SithCollisionType heightCollision = sithIntersect_CheckSphereThingModelIntersection(&pos, &dir, moveDist, pThing->collide.height, &pComplex->renderData, &heightHitDistance, &heightHitFace, &heightHitMesh, &heightHitNormal);
        SithCollisionType widthCollision  = sithIntersect_CheckSphereThingModelIntersection(&pos, &dir, moveDist, pThing->collide.width, &pComplex->renderData, &widthHitDistance, &widthHitFace, &widthHitMesh, &widthHitNormal);

        if ( !heightCollision || !widthCollision )
        {
            if ( heightCollision || !widthCollision )
            {
                if ( !widthCollision && heightCollision )
                {
                    hitType = heightCollision;
                    *pHitDistance = heightHitDistance;
                    *ppHitFace = heightHitFace;
                    *ppHitMesh = heightHitMesh;
                    *hitNorm = heightHitNormal;
                }
            }
            else
            {
                hitType = widthCollision;
                *pHitDistance = widthHitDistance;
                *ppHitFace = widthHitFace;
                *ppHitMesh = widthHitMesh;
                *hitNorm = widthHitNormal;
            }
        }
        else
        {
            if ( fabsf(rdVector_Dot3(moveNorm, &rdroid_g_zVector3)) <= 0.800000011920929 )
            {
                hitType = widthCollision;
                *pHitDistance = widthHitDistance;
                *ppHitFace = widthHitFace;
                *ppHitMesh = widthHitMesh;
                *hitNorm = widthHitNormal;
            }
            else
            {
                hitType = heightCollision;
                *pHitDistance = heightHitDistance;
                *ppHitFace = heightHitFace;
                *ppHitMesh = heightHitMesh;
                *hitNorm = heightHitNormal;
            }
        }
    }
    else
    {
        // This code is always executed in retail version
        hitType = sithIntersect_CheckSphereThingModelIntersection(&pos, &dir, moveDist, radius, &pComplex->renderData, pHitDistance, ppHitFace, ppHitMesh, hitNorm);
    }

    if ( !hitType )
    {
        return 0;
    }

    rdMatrix_TransformVector34Acc(hitNorm, &pComplex->orient);
    if ( !bThingFaceCol )
    {
        return hitType | SITHCOLLISION_THING;
    }

    rdVector_Neg3Acc(hitNorm);
    return hitType | SITHCOLLISION_THING;
}

SithCollisionType J3DAPI sithIntersect_CheckSphereThingModelIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, const rdThing* prdThing, float* hitDistance, rdFace** ppFace, rdModel3Mesh** ppMesh, rdVector3* hitNorm)
{
    SITH_ASSERTREL(prdThing != NULL);
    SITH_ASSERTREL(prdThing->type == RD_THING_MODEL3);
    SITH_ASSERTREL(prdThing->data.pModel3 != NULL);
    SITH_ASSERTREL(ppMesh && ppFace);
    SITH_ASSERTREL(hitNorm);

    SithCollisionType hitType = 0;
    rdModel3GeoSet* pGeos = prdThing->data.pModel3->aGeos;
    for ( size_t i = 0; i < pGeos->numMeshes; ++i )
    {
        SithCollisionType curHitType = sithIntersect_CheckSphereMeshIntersection(startPos, moveNorm, moveDist, radius, &pGeos->aMeshes[i], hitDistance, ppFace, hitNorm);
        if ( curHitType )
        {
            hitType = curHitType;
            *ppMesh  = &pGeos->aMeshes[i];
            moveDist = *hitDistance;
        }
    }

    return hitType;
}

SithCollisionType J3DAPI sithIntersect_CheckSphereMeshIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdModel3Mesh* pMesh, float* hitDist, rdFace** ppHitFace, rdVector3* hitNorm)
{
    SITH_ASSERTREL(hitNorm);

    float  maxHitDist = 1.0f;
    SithCollisionType hitType = 0;
    for ( size_t i = 0; i < pMesh->numFaces; ++i )
    {
        rdVector3 curHitNorm;
        SithCollisionType curHit = sithIntersect_CheckSphereFaceIntersectionEx(startPos, moveNorm, moveDistance, radius, &pMesh->aFaces[i], pMesh->apVertices, hitDist, &curHitNorm, 0);
        if ( curHit && (*hitDist < moveDistance
            || (hitType != SITHCOLLISION_FACE && curHit == SITHCOLLISION_FACE)
            || rdVector_Dot3(&pMesh->aFaces[i].normal, moveNorm) < maxHitDist) )
        {
            *ppHitFace   = &pMesh->aFaces[i];
            moveDistance = *hitDist;
            hitType      = curHit;
            maxHitDist   = rdVector_Dot3(&pMesh->aFaces[i].normal, moveNorm);
            rdVector_Copy3(hitNorm, &curHitNorm);
        }
    }

    return hitType;
}

int J3DAPI sithIntersect_CheckSphereIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, const rdVector3* endPos, float size, float* hitDistance, int bCheckEndPos, int colflags)
{
    rdVector3 dir;
    rdVector_Sub3(&dir, endPos, startPos);

    float maxDist = radius + size;

    if ( moveDist == 0.0f )
    {
        float distance = rdVector_Len3(&dir);
        if ( distance >= maxDist )
        {
            return 0;
        }

        *hitDistance = 0.0f;
        if ( (colflags & 0x400) != 0 )
        {
            *hitDistance = distance;
        }

        return 1;
    }

    float dirdot = rdVector_Dot3(moveNorm, &dir);
    if ( dirdot < 0.0f || (moveDist + maxDist) < dirdot )
    {
        if ( !bCheckEndPos )
        {
            return 0;
        }

        float distance = rdVector_Len3(&dir);
        if ( distance >= maxDist )
        {
            return 0;
        }

        *hitDistance = 0.0f;
        if ( (colflags & 0x400) != 0 )
        {
            *hitDistance = distance;
        }

        return 1;
    }

    dir.x = moveNorm->x * dirdot + startPos->x;
    dir.y = moveNorm->y * dirdot + startPos->y;
    dir.z = moveNorm->z * dirdot + startPos->z;

    float a = (endPos->x - dir.x) * (endPos->x - dir.x) + (endPos->y - dir.y) * (endPos->y - dir.y) + (endPos->z - dir.z) * (endPos->z - dir.z);
    float distance = sqrtf(a);
    if ( distance >= maxDist )
    {
        return 0;
    }

    *hitDistance = 0.0f;
    distance = dirdot - sqrtf(maxDist * maxDist - distance * distance);
    if ( distance <= moveDist && distance >= 0.0f )
    {
        *hitDistance = distance;
    }

    return 1;
}

int J3DAPI sithIntersect_TestSphereFaceHit(const rdVector3* startPos, float radius, const rdFace* pFace, const rdVector3* aVertices, int* pHitMask)
{
    if ( pHitMask )
    {
        *pHitMask = 0;
    }

    int axis1, axis2;
    sithIntersect_GetProjectionAxes(&pFace->normal, &axis1, &axis2);

    float sx = *(&startPos->x + axis1);
    float sy = *(&startPos->x + axis2);

    int curMask = 1;
    for ( size_t vertNum = 0; vertNum < pFace->numVertices; ++vertNum )
    {
        rdVector2 edge;
        edge.x = -*(&aVertices[pFace->aVertices[vertNum]].x + axis1); // Get n-th component of vertex, i.e. x, y or z 
        edge.y = -*(&aVertices[pFace->aVertices[vertNum]].x + axis2); // Get n-th component of vertex, i.e. x, y or z

        rdVector2 nedge;
        size_t nextVertNum = (vertNum + 1) % pFace->numVertices;
        nedge.x = edge.x + *(&aVertices[pFace->aVertices[nextVertNum]].x + axis1); // Get n-th component of next vertex, i.e. x, y or z
        nedge.y = edge.y + *(&aVertices[pFace->aVertices[nextVertNum]].x + axis2); // Get n-th component of next vertex, i.e. x, y or z

        // Don't move from here as it must be calculated after nedge
        edge.x += sx;
        edge.y += sy;

        if ( rdVector_Cross2(&edge, &nedge) < 0.0f )
        {
            if ( radius == 0.0f || !pHitMask )
            {
                return 0;
            }

            rdVector_Normalize2Acc(&nedge);
            if ( -radius > rdVector_Cross2(&edge, &nedge) )
            {
                return 0;
            }

            *pHitMask |= curMask;
        }

        curMask *= 2;
    }

    return 1;
}

SithCollisionType J3DAPI sithIntersect_CheckSphereFaceHitVerticesIntersection(const rdVector3* startPos, float radius, const rdFace* pFace, const rdVector3* aVertices, int vertHitMask, rdVector3* pHitPos)
{
    SITH_ASSERTREL(pFace != NULL);
    SITH_ASSERTREL(radius >= 0.0f);

    SithCollisionType hitType = 0;
    float maxHitDist = radius + 1.0f;

    for ( size_t vertNum = 0; vertNum < pFace->numVertices && vertHitMask; ++vertNum )
    {
        if ( (vertHitMask & 1) != 0 )
        {
            const rdVector3* pVert = &aVertices[pFace->aVertices[vertNum]];
            const rdVector3* pNextVert = &aVertices[pFace->aVertices[(vertNum + 1) % pFace->numVertices]];

            rdVector3 dVertStartPos;
            rdVector_Sub3(&dVertStartPos, startPos, pVert);

            rdVector3 dverts;
            rdVector_Sub3(&dverts, pNextVert, pVert);

            float vertdist = rdVector_Normalize3Acc(&dverts);
            float dot = rdVector_Dot3(&dverts, &dVertStartPos);

            if ( -radius <= dot && (dot - radius) <= vertdist )
            {
                rdVector3 hitPos;
                SithCollisionType curHitType = SITHCOLLISION_FACEVERTEX;

                if ( dot >= 0.0f )
                {
                    if ( dot <= vertdist )
                    {
                        curHitType = SITHCOLLISION_FACEEDGE;
                        hitPos.x = dverts.x * dot + pVert->x;
                        hitPos.y = dverts.y * dot + pVert->y;
                        hitPos.z = dverts.z * dot + pVert->z;
                    }
                    else
                    {
                        rdVector_Copy3(&hitPos, pNextVert);
                    }
                }
                else
                {
                    rdVector_Copy3(&hitPos, pVert);
                }

                float a = (startPos->x - hitPos.x) * (startPos->x - hitPos.x)
                    + (startPos->y - hitPos.y) * (startPos->y - hitPos.y)
                    + (startPos->z - hitPos.z) * (startPos->z - hitPos.z);
                float hitDist = sqrtf(a);
                if ( hitDist <= radius && hitDist < maxHitDist )
                {
                    hitType = curHitType;
                    maxHitDist = hitDist;
                    if ( pHitPos )
                    {
                        rdVector_Copy3(pHitPos, &hitPos);
                    }
                }
            }
        }

        vertHitMask = (unsigned int)vertHitMask >> 1;
    }

    return hitType;
}

int J3DAPI sithIntersect_CheckSphereHit(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdVector3* normal, const rdVector3* point, float* pSphereHitDist, int colflags)
{
    // Function calculates sphere-plane intersection

    // Calculate distance from start point to plane
    double dist = rdMath_DistancePointToPlane(startPos, normal, point);
    dist = stdMath_ClipNearZero((float)dist);
    if ( dist < 0.0f )
    {
        return 0;
    }

    // Sub sphere radius to get distance from sphere's surface
    dist -= radius;
    if ( dist > moveDistance )
    {
        return 0;
    }

    // Calc. direction dot
    // Dot is negated to get positive dot when the motion is toward the plane
    double dot = -(rdVector_Dot3(normal, moveNorm));
    if ( dist < 0.0f ) // If away from plane
    {
        *pSphereHitDist = 0.0f;
        if ( (colflags & 0x400) != 0 )
        {
            *pSphereHitDist = *pSphereHitDist + radius;
        }

        return 1;
    }

    if ( dot == 0.0f )
    {
        return 0;
    }

    if ( dot * moveDistance < dist )
    {
        return 0;
    }

    // Calculate the amount of movement along moveNorm needed so the sphere surface reaches the plane
    *pSphereHitDist = (float)(dist / dot);
    if ( *pSphereHitDist < 0.0f )
    {
        *pSphereHitDist = 0.0f;
    }

    SITH_ASSERTREL((*pSphereHitDist <= moveDistance) && (*pSphereHitDist >= 0.0f));
    return 1;
}

SithCollisionType J3DAPI sithIntersect_CheckSphereFaceIntersectionEx(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdFace* pFace, const rdVector3* aVertices, float* hitDist, rdVector3* hitNorm, int colflags)
{
    SITH_ASSERTREL((startPos != NULL) && (pFace != NULL) && (aVertices != NULL));
    SITH_ASSERTREL((hitDist != NULL) && (moveNorm != NULL)); // Fixed check for hitDist != NULL, was hitPos != NULL
    //SITH_ASSERTREL((hitPos != NULL) && (moveNorm != NULL));
    SITH_ASSERTREL(radius >= 0.0f);

    if ( !sithIntersect_CheckSphereHit(startPos, moveNorm, moveDistance, radius, &pFace->normal, &aVertices[*pFace->aVertices], hitDist, colflags) )
    {
        return 0;
    }

    if ( (colflags & 0x400) == 0 && rdVector_Dot3(moveNorm, &pFace->normal) >= 0.0f )
    {
        return 0;
    }

    rdVector3 hitPos;
    if ( *hitDist == 0.0f )
    {
        rdVector_Copy3(&hitPos, startPos);
    }
    else
    {
        hitPos.x = moveNorm->x * *hitDist + startPos->x;
        hitPos.y = moveNorm->y * *hitDist + startPos->y;
        hitPos.z = moveNorm->z * *hitDist + startPos->z;
    }

    float dist = rdMath_DistancePointToPlane(&hitPos, &pFace->normal, &aVertices[*pFace->aVertices]);
    if ( stdMath_ClipNearZero(dist) != 0.0f )
    {
        hitPos.x = pFace->normal.x * -dist + hitPos.x;
        hitPos.y = pFace->normal.y * -dist + hitPos.y;
        hitPos.z = pFace->normal.z * -dist + hitPos.z;
    }

    if ( !hitNorm )
    {
        return sithIntersect_CheckFaceVerticesIntersection(&hitPos, radius, pFace, aVertices, NULL);
    }

    SithCollisionType hitType = sithIntersect_CheckFaceVerticesIntersection(&hitPos, radius, pFace, aVertices, &hitPos);
    if ( hitType == SITHCOLLISION_FACEEDGE || hitType == SITHCOLLISION_FACEVERTEX )
    {
        rdVector_Sub3(hitNorm, startPos, &hitPos);
        rdVector_Normalize3Acc(hitNorm);
    }
    else
    {
        // TODO: maybe hit normal should also be calculated to hitPOS
        //       even tho sithIntersect_CheckFaceVerticesIntersection didn't update hitPos in this case
        rdVector_Copy3(hitNorm, &pFace->normal);
    }

    return hitType;
}

SithCollisionType J3DAPI sithIntersect_CheckSphereFaceIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdFace* pFace, const rdVector3* aVertices, float* hitDist, int flags)
{
    SITH_ASSERTREL((startPos != NULL) && (pFace != NULL) && (aVertices != NULL));
    SITH_ASSERTREL((hitDist != NULL) && (moveNorm != NULL)); // Fixed check for hitDist != NULL, was hitPos != NULL
    //SITH_ASSERTREL((hitPos != NULL) && (moveNorm != NULL)); // Removed
    SITH_ASSERTREL(radius >= 0.0f);
    if ( !sithIntersect_CheckSphereHit(startPos, moveNorm, moveDistance, radius, &pFace->normal, &aVertices[*pFace->aVertices], hitDist, flags) )
    {
        return 0;
    }

    if ( radius != 0.0f )
    {
        return SITHCOLLISION_FACE;
    }

    rdVector3 hitPos;
    hitPos.x = moveNorm->x * *hitDist + startPos->x;
    hitPos.y = moveNorm->y * *hitDist + startPos->y;
    hitPos.z = moveNorm->z * *hitDist + startPos->z;
    return sithIntersect_CheckFaceVerticesIntersection(&hitPos, radius, pFace, aVertices, NULL);
}

int J3DAPI sithIntersect_CheckThingBoundBoxIntersection(const SithThing* pThing1, const SithThing* pThing2, const rdVector3* moveNorm, float moveDist, float* hitDist)
{
    rdVector3 center;
    rdVector_Copy3(&center, &pThing1->pos);

    center.x = moveNorm->x * moveDist + center.x;
    center.y = moveNorm->y * moveDist + center.y;
    center.z = moveNorm->z * moveDist + center.z;

    rdVector3 bbMin1;
    bbMin1.x = center.x - pThing1->collide.width;
    bbMin1.y = center.y - pThing1->collide.width;
    bbMin1.z = center.z - pThing1->collide.height;

    rdVector3 bbMax1;
    bbMax1.x = center.x + pThing1->collide.width;
    bbMax1.y = center.y + pThing1->collide.width;
    bbMax1.z = center.z + pThing1->collide.height;

    rdVector3 bbMin2;
    bbMin2.x = pThing2->pos.x - pThing2->collide.width;
    bbMin2.y = pThing2->pos.y - pThing2->collide.width;
    bbMin2.z = pThing2->pos.z - pThing2->collide.height;

    rdVector3 bbMax2;
    bbMax2.x = pThing2->pos.x + pThing2->collide.width;
    bbMax2.y = pThing2->pos.y + pThing2->collide.width;
    bbMax2.z = pThing2->pos.z + pThing2->collide.height;

    *hitDist = 0.0f;
    bool b1Intersect2X = false;
    if ( bbMax1.x > bbMin2.x && bbMax1.x < bbMax2.x )
    {
        b1Intersect2X = true;
        *hitDist = bbMin2.x - pThing1->pos.x;
    }

    if ( !b1Intersect2X )
    {
        if ( bbMin1.x < bbMax2.x && bbMin1.x > bbMin2.x )
        {
            b1Intersect2X = true;
            float dx = pThing1->pos.x - bbMax2.x;
            if ( dx > *hitDist )
            {
                *hitDist = dx;
            }
        }

        if ( !b1Intersect2X )
        {
            if ( bbMin1.x > bbMin2.x && bbMax1.x < bbMax2.x )
            {
                b1Intersect2X = true;
            }

            if ( !b1Intersect2X && bbMin2.x > bbMin1.x && bbMax2.x < bbMax1.x )
            {
                b1Intersect2X = true;
            }
        }
    }

    bool b1Intersect2Y = false;
    if ( b1Intersect2X )
    {
        if ( bbMax1.y > bbMin2.y && bbMax1.y < bbMax2.y )
        {
            b1Intersect2Y = true;
            float dy = bbMin2.y - pThing1->pos.y;
            if ( dy > *hitDist )
            {
                *hitDist = dy;
            }
        }

        if ( !b1Intersect2Y && bbMin1.y < bbMax2.y && bbMin1.y >bbMin2.y )
        {
            b1Intersect2Y = true;
            float dy = pThing1->pos.y - bbMax2.y;
            if ( dy > *hitDist )
            {
                *hitDist = dy;
            }
        }

        if ( !b1Intersect2Y )
        {
            if ( bbMin1.y > bbMin2.y && bbMax1.y < bbMax2.y )
            {
                b1Intersect2Y = true;
            }

            if ( !b1Intersect2Y && bbMin2.y > bbMin1.y && bbMax2.y < bbMax1.y )
            {
                b1Intersect2Y = true;
            }
        }
    }

    if ( !b1Intersect2Y )
    {
        return b1Intersect2X && b1Intersect2Y;
    }

    bool b1Intersect2Z = false;
    if ( bbMax1.z > bbMin2.z && bbMax1.z < bbMax2.z )
    {
        b1Intersect2Z = true;
        float dz = bbMin2.z - pThing1->pos.z;
        if ( dz > *hitDist )
        {
            *hitDist = dz;
        }
    }

    if ( !b1Intersect2Z && bbMin1.z < bbMax2.z && bbMin1.z >bbMin2.z )
    {
        b1Intersect2Z = true;
        float dz = pThing1->pos.z - bbMax2.z;
        if ( dz > *hitDist )
        {
            *hitDist = dz;
        }
    }

    if ( b1Intersect2Z )
    {
        return b1Intersect2X && b1Intersect2Y && b1Intersect2Z;
    }

    if ( bbMin1.z > bbMin2.z && bbMax1.z < bbMax2.z )
    {
        b1Intersect2Z = true;
    }

    if ( !b1Intersect2Z && bbMin2.z > bbMin1.z && bbMax2.z < bbMax1.z )
    {
        b1Intersect2Z = true;
    }

    return b1Intersect2X && b1Intersect2Y && b1Intersect2Z;
}

void J3DAPI sithIntersect_GetProjectionAxes(const rdVector3* pNormal, int* pAxis1, int* pAxis2)
{
    float x = fabsf(pNormal->x);
    float y = fabsf(pNormal->y);
    float z = fabsf(pNormal->z);

    size_t idx = 0;
    if ( x <= y )
    {
        idx = (y <= z) ? 2 : 1;
    }
    else if ( x <= z )
    {
        idx = 2;
    }

    if ( *(&pNormal->x + idx) <= 0.0f )
    {
        *pAxis1 = sithIntersect_aFaceProjectionAxes[idx][1];
        *pAxis2 = sithIntersect_aFaceProjectionAxes[idx][0];
    }
    else
    {
        *pAxis1 = sithIntersect_aFaceProjectionAxes[idx][0];
        *pAxis2 = sithIntersect_aFaceProjectionAxes[idx][1];
    }
}
