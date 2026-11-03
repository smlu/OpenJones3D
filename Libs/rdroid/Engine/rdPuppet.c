#include "rdPuppet.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <math.h>

void rdPuppet_ResetGlobals(void)
{}

rdPuppet* J3DAPI rdPuppet_New(rdThing* pParent)
{
    rdPuppet* pPuppet = (rdPuppet*)STDMALLOC(sizeof(rdPuppet));
    if ( !pPuppet )
    {
        RDLOG_ERROR("Error allocating memory for rdPuppet.\n");
        return NULL;
    }

    // Fixed: Moved memset after null pointer check to avoid write to null ptr
    STD_ZEROMEM(pPuppet, sizeof(rdPuppet));

    rdPuppet_NewEntry(pPuppet, pParent);
    pParent->pPuppet = pPuppet;
    return pPuppet;
}

void J3DAPI rdPuppet_NewEntry(rdPuppet* pPuppet, rdThing* parent)
{
    RD_ASSERTREL(pPuppet != NULL);

    // TODO: make dynamic allocation of tracks and rdPuppetTrack::aCurKfNodeEntryNums

    pPuppet->bPaused = 0;
    pPuppet->pThing  = parent;
    for ( size_t trackNum = 0; trackNum < RDPUPPET_MAX_TRACKS; ++trackNum )
    {
        pPuppet->aTracks[trackNum].playbackSpeed = 1.0f; // Added
        pPuppet->aTracks[trackNum].curFrame      = 0.0f;
        pPuppet->aTracks[trackNum].prevFrame     = 0.0f;
        rdPuppet_RemoveTrack(pPuppet, trackNum);
    }
}

void J3DAPI rdPuppet_Free(rdPuppet* pPuppet)
{
    if ( !pPuppet )
    {
        RDLOG_ERROR("Warning: attempt to free NULL puppet ptr.\n");
        return;
    }
    rdPuppet_FreeEntry(pPuppet);
    stdMemory_Free(pPuppet);
}

void J3DAPI rdPuppet_FreeEntry(rdPuppet* pPuppet)
{
    J3D_UNUSED(pPuppet);
}

void J3DAPI rdPuppet_SetPause(rdPuppet* pPuppet, bool bPaused)
{
    RD_ASSERTREL(pPuppet != NULL);
    pPuppet->bPaused = bPaused;
}

int J3DAPI rdPuppet_AddTrack(rdPuppet* pPuppet, rdKeyframe* pKFTrack, int lowPriority, int highPriority)
{
    RD_ASSERTREL(pPuppet != NULL);
    RD_ASSERTREL(pKFTrack != NULL);

    // Find non-playing track
    size_t track;
    for ( track = 0; track < STD_ARRAYLEN(pPuppet->aTracks) && pPuppet->aTracks[track].status; ++track )
    {
    }

    if ( track >= STD_ARRAYLEN(pPuppet->aTracks) )
    {
        // No track was found, try find fading track
        for ( track = 0; track < RDPUPPET_TRACK_FADEOUT; ++track )
        {
            if ( (pPuppet->aTracks[track].status & RDPUPPET_TRACK_FADEOUT) != 0
                && (pPuppet->aTracks[track].status & (RDPUPPET_TRACK_FADEOUT_PAUSE_ON_LAST_FRAME | RDPUPPET_TRACK_PAUSE_ON_LAST_FRAME)) == 0 )
            {
                rdPuppet_RemoveTrack(pPuppet, track);
                break;
            }
        }
    }
    if ( track >= STD_ARRAYLEN(pPuppet->aTracks) )
    {
        return -1;
    }

    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];
    pTrack->playbackSpeed = 1.0f; // Added
    pTrack->fps           = pKFTrack->fps;
    pTrack->pKFTrack      = pKFTrack;
    pTrack->blendWeight   = 0.0f;
    pTrack->lowPriority   = lowPriority;
    pTrack->highPriority  = highPriority;
    pTrack->status       |= RDPUPPET_TRACK_UNKNOWN_1;

    rdPuppet_ResetTrack(pPuppet, track);
    return track;
}

void J3DAPI rdPuppet_RemoveTrack(rdPuppet* pPuppet, int32_t track)
{
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));
    RD_ASSERTREL(pPuppet != NULL);
    //RD_ASSERTREL(pTrack != NULL); // TODO: ???

    if ( pPuppet->aTracks[track].pfCallback )
    {
        pPuppet->aTracks[track].pfCallback(pPuppet->pThing->pThing, track, (rdKeyMarkerType)0); // notify finished 
    }

    pPuppet->aTracks[track].status     = 0;
    pPuppet->aTracks[track].pKFTrack   = NULL;
    pPuppet->aTracks[track].pfCallback = NULL;
}

int J3DAPI rdPuppet_SetStatus(rdPuppet* pPuppet, int32_t track, rdPuppetTrackStatus status)
{
    RD_ASSERTREL(pPuppet);
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));
    pPuppet->aTracks[track].status |= status;
    return 1;
}

void J3DAPI rdPuppet_SetCallback(rdPuppet* pPuppet, int32_t track, rdPuppetTrackCallback pfCallback)
{
    RD_ASSERTREL(pPuppet);
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));
    pPuppet->aTracks[track].pfCallback = pfCallback;
}

int J3DAPI rdPuppet_PlayTrack(rdPuppet* pPuppet, int32_t track)
{
    RD_ASSERTREL(pPuppet != NULL); // Fixed: Moved here
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));

    rdPuppetTrack* pTrack = &pPuppet->aTracks[track]; // Fixed: Moved after pPuppet null check to prevent null pointer access
    RD_ASSERTREL(pTrack != NULL); // TODO: What's the point of this check?

    pTrack->status |= RDPUPPET_TRACK_PLAYING;
    pTrack->status &= ~RDPUPPET_TRACK_PAUSED;
    pPuppet->aTracks[track].blendWeight = 1.0f;
    return 1;
}

int J3DAPI rdPuppet_FadeInTrack(rdPuppet* pPuppet, int32_t track, float fadeDuration)
{
    RD_ASSERTREL(pPuppet != NULL); // Fixed: Moved here
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));

    rdPuppetTrack* pTrack = &pPuppet->aTracks[track]; // Fixed: Moved after pPuppet null check to prevent null pointer access
    RD_ASSERTREL(pTrack != NULL); // TODO: What's the point of this check?

    pTrack->status &= ~RDPUPPET_TRACK_FADEOUT;
    pTrack->status |= RDPUPPET_TRACK_FADEIN | RDPUPPET_TRACK_PLAYING;
    if ( fadeDuration <= 0.0f )
    {
        pPuppet->aTracks[track].fadeRate = 1.0f;
    }
    else
    {
        pPuppet->aTracks[track].fadeRate = 1.0f / fadeDuration;
    }

    return 1;
}

int J3DAPI rdPuppet_FadeOutTrack(rdPuppet* pPuppet, int32_t track, float fadeDuration)
{
    RD_ASSERTREL(pPuppet != NULL); // Fixed: Moved here
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));

    rdPuppetTrack* pTrack = &pPuppet->aTracks[track]; // Fixed: Moved after pPuppet null check to prevent null pointer access
    RD_ASSERTREL(pTrack != NULL); // TODO: What's the point of this check?

    pTrack->status &= ~RDPUPPET_TRACK_FADEIN;
    pTrack->status |= RDPUPPET_TRACK_FADEOUT;
    if ( fadeDuration <= 0.0f )
    {
        pPuppet->aTracks[track].fadeRate = 1.0f;
    }
    else
    {
        pPuppet->aTracks[track].fadeRate = 1.0f / fadeDuration;
    }

    return 1;
}

void J3DAPI rdPuppet_SetTrackSpeed(rdPuppet* pPuppet, int32_t track, float fps)
{
    RD_ASSERTREL(pPuppet != NULL); // Fixed: Moved here
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks))); // Added

    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];
    RD_ASSERTREL(pTrack != NULL); // TODO: What's the point of this check?
    pTrack->fps = fps;
}

void J3DAPI rdPuppet_SetPlaybackSpeed(rdPuppet* pPuppet, int32_t track, float speed)
{
    RD_ASSERTREL(pPuppet != NULL);
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));
    RD_ASSERTREL(speed > 0.0f);

    pPuppet->aTracks[track].playbackSpeed = speed;
}

void J3DAPI rdPuppet_SetTrackNoise(rdPuppet* pPuppet, int32_t track, float noise)
{
    RD_ASSERTREL(pPuppet != NULL); // Fixed: Moved here
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));

    rdPuppetTrack* pTrack = &pPuppet->aTracks[track]; // Fixed: Moved after pPuppet null check to prevent null pointer access
    RD_ASSERTREL(pTrack != NULL); // TODO: What's the point of this check?

    pTrack->noise = noise;
    if ( noise == 0.0f )
    {
        pTrack->status &= ~RDPUPPET_TRACK_NOISESET;
    }
    else
    {
        pTrack->status |= RDPUPPET_TRACK_NOISESET;
    }
}

void J3DAPI rdPuppet_SetTrackPriority(rdPuppet* pPuppet, int32_t track, int lowPri, int heighPri)
{
    RD_ASSERTREL(pPuppet != NULL);
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));

    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];
    RD_ASSERTREL(pTrack != NULL); // TODO: What's the point of this check?
    pTrack->lowPriority  = lowPri;
    pTrack->highPriority = heighPri;
}

void J3DAPI rdPuppet_AdvanceTrack(rdPuppet* pPuppet, int32_t track, float frames)
{
    RD_ASSERTREL(pPuppet != NULL); // Fixed: Moved here
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));
    RD_ASSERTREL(frames >= 0.0f);

    if ( !pPuppet->aTracks[track].pKFTrack )
    {
        RDLOG_ERROR("Track %d does not exist.\n", track);
        return;
    }

    if ( frames == 0.0f )
    {
        return;
    }

    rdPuppetTrack* pTrack = &pPuppet->aTracks[track]; // Fixed: Moved after pPuppet null check to prevent null pointer access

    // Advance cur track
    pTrack->curFrame = pTrack->prevFrame + frames;

    bool bFinish = false;
    float progress = 0.0f;
    if ( (float)pTrack->pKFTrack->numFrames <= pTrack->curFrame )
    {
        if ( (pTrack->status & RDPUPPET_TRACK_NOLOOP) != 0 )
        {
            pTrack->curFrame = (float)pTrack->pKFTrack->numFrames;
            bFinish = true;
        }
        else if ( (pTrack->status & RDPUPPET_TRACK_PAUSE_ON_LAST_FRAME) != 0 )
        {
            pTrack->curFrame = (float)pTrack->pKFTrack->numFrames;
            pTrack->status |= RDPUPPET_TRACK_PAUSED;
        }
        else if ( (pTrack->status & RDPUPPET_TRACK_FADEOUT_NOLOOP) != 0 )
        {
            pTrack->curFrame = (float)pTrack->pKFTrack->numFrames;
            rdPuppet_FadeOutTrack(pPuppet, track, 0.25f);
            pTrack->status |= RDPUPPET_TRACK_PAUSED;
        }
        else // loop animation
        {
            progress = floorf(pTrack->curFrame / (float)pTrack->pKFTrack->numFrames);
            pTrack->curFrame -= (float)pTrack->pKFTrack->numFrames * progress;

            // Fixed: Added bounds check
            if ( pPuppet->pThing->data.pModel3->numHNodes < STD_ARRAYLEN(pTrack->aCurKfNodeEntryNums) )
            {
                STD_ZEROMEM(pTrack->aCurKfNodeEntryNums, sizeof(pTrack->aCurKfNodeEntryNums[0]) * pPuppet->pThing->data.pModel3->numHNodes);
            }
            else
            {
                STD_ZEROMEM(pTrack->aCurKfNodeEntryNums, sizeof(pTrack->aCurKfNodeEntryNums));
            }
        }
    }

    if ( pTrack->pfCallback && pTrack->pKFTrack->numMarkers )
    {
        RD_ASSERTREL(pPuppet->pThing);

        if ( progress == 0.0f )
        {
            if ( pTrack->pKFTrack )
            {
                const size_t numMarkers = pTrack->pKFTrack->numMarkers;
                for ( size_t i = 0; pTrack->pKFTrack && i < numMarkers && pTrack->pKFTrack->aMarkerFrames[i] <= pTrack->curFrame; ++i )
                {
                    if ( pTrack->pKFTrack->aMarkerFrames[i] > pTrack->prevFrame || pTrack->prevFrame == 0.0f )
                    {
                        pTrack->pfCallback(pPuppet->pThing->pThing, track, pTrack->pKFTrack->aMarkerTypes[i]);
                    }
                }
            }
        }
        else if ( progress <= 1.0f )
        {
            if ( pTrack->pKFTrack )
            {
                const size_t numMarkers  = pTrack->pKFTrack->numMarkers;
                for ( size_t i = 0; pTrack->pKFTrack && i < numMarkers; ++i )
                {
                    if ( pTrack->pKFTrack->aMarkerFrames[i] > (double)pTrack->prevFrame
                        || pTrack->pKFTrack->aMarkerFrames[i] <= (double)pTrack->curFrame )
                    {
                        pTrack->pfCallback(pPuppet->pThing->pThing, track, pTrack->pKFTrack->aMarkerTypes[i]);
                    }
                }
            }
        }
        else if ( pTrack->pKFTrack ) // progress > 1.0f 
        {
            const size_t numMarkers  = pTrack->pKFTrack->numMarkers;
            for ( size_t i = 0; pTrack->pKFTrack && i < numMarkers; ++i )
            {
                pTrack->pfCallback(pPuppet->pThing->pThing, track, pTrack->pKFTrack->aMarkerTypes[i]);
            }
        }
    }

    if ( bFinish )
    {
        rdPuppet_RemoveTrack(pPuppet, track);
    }
    else
    {
        pTrack->prevFrame = pTrack->curFrame;
    }
}

int J3DAPI rdPuppet_UpdateTracks(rdPuppet* pPuppet, float secDeltaTime)
{
    RD_ASSERTREL(pPuppet != NULL);
    if ( pPuppet->bPaused )
    {
        return 0;
    }

    size_t numUpdatedTracks = 0;
    for ( size_t trackNum = 0; trackNum < STD_ARRAYLEN(pPuppet->aTracks); trackNum++ )
    {
        rdPuppetTrack* pTrack = &pPuppet->aTracks[trackNum];
        if ( pTrack->status )
        {
            ++numUpdatedTracks;

            if ( (pTrack->status & RDPUPPET_TRACK_PAUSED) == 0 )
            {
                float frames = pTrack->fps * secDeltaTime * pTrack->playbackSpeed; // Altered: Added multiplication by playbackSpeed
                rdPuppet_AdvanceTrack(pPuppet, trackNum, frames);
            }

            if ( (pTrack->status & RDPUPPET_TRACK_FADEIN) != 0 )
            {
                pTrack->blendWeight += pTrack->fadeRate * secDeltaTime;
                if ( pTrack->blendWeight >= 1.0f )
                {
                    pTrack->blendWeight = 1.0f;
                    pTrack->status &= ~RDPUPPET_TRACK_FADEIN;
                }
            }
            else if ( (pTrack->status & RDPUPPET_TRACK_FADEOUT) != 0 )
            {
                pTrack->blendWeight -= pTrack->fadeRate * secDeltaTime;
                if ( pTrack->blendWeight <= 0.0f )
                {
                    if ( (pTrack->status & RDPUPPET_TRACK_FADEOUT_PAUSE_ON_LAST_FRAME) != 0 )
                    {
                        pTrack->status &= ~RDPUPPET_TRACK_FADEOUT;
                        pTrack->status |= RDPUPPET_TRACK_PAUSED;
                    }
                    else
                    {
                        rdPuppet_RemoveTrack(pPuppet, trackNum);
                    }
                }
            }
        }
    }

    return numUpdatedTracks;
}

void J3DAPI rdPuppet_ResetTrack(rdPuppet* pPuppet, int32_t track)
{
    RD_ASSERT(pPuppet); // Added
    RD_ASSERTREL((track >= 0) && (track < STD_ARRAYLEN(pPuppet->aTracks)));

    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];

    // Fixed: Added bounds check
    if ( pPuppet->pThing->data.pModel3->numHNodes < STD_ARRAYLEN(pTrack->aCurKfNodeEntryNums) )
    {
        STD_ZEROMEM(pTrack->aCurKfNodeEntryNums, sizeof(pTrack->aCurKfNodeEntryNums[0]) * pPuppet->pThing->data.pModel3->numHNodes);
    }
    else
    {
        STD_ZEROMEM(pTrack->aCurKfNodeEntryNums, sizeof(pTrack->aCurKfNodeEntryNums));
    }

    pPuppet->aTracks[track].playbackSpeed = 1.0f; // Added
    pPuppet->aTracks[track].curFrame      = 0.0f;
    pPuppet->aTracks[track].prevFrame     = 0.0f;
    pPuppet->aTracks[track].status        = RDPUPPET_TRACK_PLAYING | RDPUPPET_TRACK_UNKNOWN_1;
}

void J3DAPI rdPuppet_BuildJointMatrices(rdThing* prdThing, const rdMatrix34* pPlacement)
{
    rdPuppet* pPuppet = prdThing->pPuppet;
    rdModel3* pModel3 = prdThing->data.pModel3;

    if ( prdThing->bSkipBuildingJoints )
    {
        return;
    }
    if ( !pPuppet || pPuppet->bPaused )
    {
        for ( size_t nodeNum = 0; nodeNum < pModel3->numHNodes; ++nodeNum )
        {
            prdThing->paJointMatrices[nodeNum] = pModel3->aHierarchyNodes[nodeNum].meshOrient;
        }
        rdThing_AccumulateMatrices(prdThing, pModel3->aHierarchyNodes, pPlacement);
        prdThing->rdFrameNum = rdCache_GetFrameNum();
        return;
    }

    // Advance track KFNode indices to >= curframe
    for ( size_t trackNum = 0; trackNum < RDPUPPET_MAX_TRACKS; trackNum++ )
    {
        rdPuppetTrack* pTrack = &pPuppet->aTracks[trackNum];
        if ( pTrack->status )
        {
            const rdKeyframe* pKFTrack = pTrack->pKFTrack;
            for ( size_t jointNum = 0; jointNum < pKFTrack->numJoints; jointNum++ )
            {
                bool bFinish = false;
                const rdKeyframeNode* pKfNode = &pKFTrack->aNodes[jointNum];
                if ( pKfNode->numEntries )
                {
                    size_t nodeIdx = pTrack->aCurKfNodeEntryNums[pKfNode->nodeNum];
                    if ( nodeIdx != pKfNode->numEntries - 1 && pTrack->curFrame >= (double)pKfNode->aEntries[nodeIdx + 1].frame )
                    {
                        ++nodeIdx;
                        while ( !bFinish )
                        {
                            if ( nodeIdx == pKfNode->numEntries - 1 )
                            {
                                bFinish = true;
                            }
                            else if ( pTrack->curFrame >= (double)pKfNode->aEntries[nodeIdx + 1].frame )
                            {
                                ++nodeIdx;
                            }
                            else
                            {
                                bFinish = true;
                            }
                        }

                        pTrack->aCurKfNodeEntryNums[pKfNode->nodeNum] = nodeIdx;
                    }
                }
            }
        }
    }

    for ( size_t nodeNum = 0; nodeNum < pModel3->numHNodes; nodeNum++ )
    {
        rdModel3HNode* pNode = &pModel3->aHierarchyNodes[nodeNum];
        int highPri = 0;
        int lowPri  = 0;
        float blendWeight     = 0.0f;
        float lowPriPlaySpeed = 0.0f;

        rdVector3 highPriPos = { 0 };
        rdVector3 highPriPyr = { 0 };
        rdVector3 lowPriPos = { 0 };
        rdVector3 lowPriPyr = { 0 };

        for ( size_t trackNum = 0; trackNum < RDPUPPET_MAX_TRACKS; trackNum++ )
        {
            rdPuppetTrack* pTrack = &pPuppet->aTracks[trackNum];
            if ( pTrack->pKFTrack )
            {
                int priority = (pNode->type & pTrack->pKFTrack->type) != 0 ? pTrack->highPriority : pTrack->lowPriority;
                if ( (pTrack->status & RDPUPPET_TRACK_PLAYING) != 0
                    && pTrack->pKFTrack->aNodes[pNode->num].numEntries
                    && priority >= lowPri
                    && (priority >= highPri || blendWeight < 1.0f) )
                {
                    if ( pTrack->pKFTrack->numJoints <= pNode->num )
                    {
                        RDLOG_ERROR("KEY %s playing on model %s - mismatched node count!\n", pTrack->pKFTrack->aName, pModel3->aName);
                        continue;
                    }

                    rdKeyframeNodeEntry* pKfFrame = &pTrack->pKFTrack->aNodes[pNode->num].aEntries[pTrack->aCurKfNodeEntryNums[pNode->num]];
                    float baseDeltaFrames = pTrack->curFrame - pKfFrame->frame;
                    RD_ASSERTREL(baseDeltaFrames >= 0.0f);

                    rdVector3 dkfpos, dkfrot;
                    if ( (pKfFrame->flags & 1) != 0 )// dpos
                    {
                        dkfpos.x = pKfFrame->dpos.x * baseDeltaFrames + pKfFrame->pos.x;
                        dkfpos.y = pKfFrame->dpos.y * baseDeltaFrames + pKfFrame->pos.y;
                        dkfpos.z = pKfFrame->dpos.z * baseDeltaFrames + pKfFrame->pos.z;
                    }
                    else
                    {
                        dkfpos = pKfFrame->pos;
                    }

                    if ( (pKfFrame->flags & 2) != 0 )// drot
                    {
                        dkfrot.x = pKfFrame->drot.x * baseDeltaFrames + pKfFrame->rot.x;
                        dkfrot.y = pKfFrame->drot.y * baseDeltaFrames + pKfFrame->rot.y;
                        dkfrot.z = pKfFrame->drot.z * baseDeltaFrames + pKfFrame->rot.z;
                    }
                    else
                    {
                        dkfrot = pKfFrame->rot;
                    }

                    rdVector_Sub3Acc(&dkfpos, &pNode->pos);
                    rdVector_Sub3Acc(&dkfrot, &pNode->pyr);

                    dkfrot.pitch = stdMath_NormalizeAngleAcute(dkfrot.pitch);
                    dkfrot.yaw   = stdMath_NormalizeAngleAcute(dkfrot.yaw);
                    dkfrot.roll  = stdMath_NormalizeAngleAcute(dkfrot.roll);

                    if ( pTrack->blendWeight < 1.0f )
                    {
                        rdVector_Scale3Acc(&dkfpos, pTrack->blendWeight);
                        rdVector_Scale3Acc(&dkfrot, pTrack->blendWeight);
                    }

                    if ( priority == highPri )
                    {
                        blendWeight += pTrack->blendWeight;
                        rdVector_Add3Acc(&highPriPos, &dkfpos);
                        rdVector_Add3Acc(&highPriPyr, &dkfrot);
                    }
                    else if ( priority > highPri )
                    {
                        lowPriPlaySpeed = blendWeight;
                        lowPri          = highPri;
                        highPri         = priority;
                        blendWeight     = pTrack->blendWeight;

                        lowPriPos = highPriPos;
                        lowPriPyr = highPriPyr;

                        highPriPos = dkfpos;
                        highPriPyr = dkfrot;
                    }

                    else if ( priority > lowPri )
                    {
                        lowPriPlaySpeed = pTrack->blendWeight;
                        lowPri          = priority;

                        lowPriPos = dkfpos;
                        lowPriPyr = dkfrot;
                    }
                    else // must be lees then lowPri
                    {
                        RD_ASSERTREL(priority == lowPri);
                        RD_ASSERTREL(highPri != lowPri);

                        lowPriPlaySpeed = lowPriPlaySpeed + pTrack->blendWeight;
                        rdVector_Add3Acc(&lowPriPos, &dkfpos);
                        rdVector_Add3Acc(&lowPriPyr, &dkfrot);
                    }
                }
            }
        }

        if ( blendWeight >= 1.0f || lowPriPlaySpeed <= 0.0f )
        {
            if ( blendWeight > 1.0f )
            {
                rdVector_InvScale3Acc(&highPriPos, blendWeight); // highPriPos / blendWeight
                rdVector_InvScale3Acc(&highPriPyr, blendWeight);
            }
        }
        else
        {
            if ( lowPriPlaySpeed > 1.0f )
            {
                rdVector_InvScale3Acc(&lowPriPos, lowPriPlaySpeed); // lowPriPos / lowPriPlaySpeed
                rdVector_InvScale3Acc(&lowPriPyr, lowPriPlaySpeed);
            }

            float lerpFact = 1.0f - blendWeight;
            highPriPos.x += lowPriPos.x * lerpFact;
            highPriPos.y += lowPriPos.y * lerpFact;
            highPriPos.z += lowPriPos.z * lerpFact;

            highPriPyr.pitch += lowPriPyr.pitch * lerpFact;
            highPriPyr.yaw   += lowPriPyr.yaw * lerpFact;
            highPriPyr.roll  += lowPriPyr.roll * lerpFact;
        }

        rdVector_Add3Acc(&highPriPos, &pNode->pos);

        highPriPyr.pitch = stdMath_NormalizeAngleAcute(highPriPyr.pitch);
        highPriPyr.yaw   = stdMath_NormalizeAngleAcute(highPriPyr.yaw);
        highPriPyr.roll  = stdMath_NormalizeAngleAcute(highPriPyr.roll);
        rdVector_Add3Acc(&highPriPyr, &pNode->pyr);

        rdMatrix_Build34(&prdThing->paJointMatrices[nodeNum], &highPriPyr, &highPriPos);

        if ( !rdVector_IsZero3(&prdThing->apTweakedAngles[nodeNum]) )
        {
            rdMatrix_PreRotate34(&prdThing->paJointMatrices[nodeNum], &prdThing->apTweakedAngles[nodeNum]);
        }
    }

    rdThing_AccumulateMatrices(prdThing, pModel3->aHierarchyNodes, pPlacement);
    prdThing->rdFrameNum = rdCache_GetFrameNum();
}