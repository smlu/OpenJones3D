#include "rdKeyframe.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Main/rdroid.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

static rdKeyframeLoadFunc rdKeyframe_pKeyframeLoader     = NULL; // Added: Init to null
static rdKeyframeUnloadFunc rdKeyframe_pKeyframeUnloader = NULL;

void rdKeyframe_InstallHooks(void)
{
    J3D_HOOKFUNC(rdKeyframe_RegisterLoader);
    J3D_HOOKFUNC(rdKeyframe_NewEntry);
    J3D_HOOKFUNC(rdKeyframe_LoadEntry);
    J3D_HOOKFUNC(rdKeyframe_FreeEntry);
}

void rdKeyframe_ResetGlobals(void)
{}

rdKeyframeLoadFunc J3DAPI rdKeyframe_RegisterLoader(rdKeyframeLoadFunc pFunc)
{
    rdKeyframeLoadFunc pPrevFunc = rdKeyframe_pKeyframeLoader;
    rdKeyframe_pKeyframeLoader = pFunc;
    return pPrevFunc;
}

rdKeyframeUnloadFunc J3DAPI rdKeyframe_RegisterUnloader(rdKeyframeUnloadFunc pFunc)
{
    rdKeyframeUnloadFunc pPrevFunc = rdKeyframe_pKeyframeUnloader;
    rdKeyframe_pKeyframeUnloader = pFunc;
    return pPrevFunc;
}

void J3DAPI rdKeyframe_NewEntry(rdKeyframe* pKeyframe)
{
    STD_ZEROMEM(pKeyframe, sizeof(rdKeyframe));
    STD_STRCPY(pKeyframe->aName, "UNKNOWN");
}

rdKeyframe* J3DAPI rdKeyframe_Load(const char* pFilename)
{
    if ( rdKeyframe_pKeyframeLoader )
    {
        return rdKeyframe_pKeyframeLoader(pFilename);
    }

    rdKeyframe* pKeyframe = (rdKeyframe*)STDMALLOC(sizeof(rdKeyframe));
    if ( !pKeyframe )
    {
        RDLOG_ERROR("Error: Bad memory allocation for keyframe '%s'.\n", pFilename);
        return NULL;
    }

    if ( !rdKeyframe_LoadEntry(pFilename, pKeyframe) )
    {
        rdKeyframe_Free(pKeyframe);
        return NULL;
    }

    return pKeyframe;
}

int J3DAPI rdKeyframe_LoadEntry(const char* pFilename, rdKeyframe* pKeyframe)
{
    rdKeyframe_NewEntry(pKeyframe);

    const char* pName = stdFileFromPath(pFilename);
    STD_STRCPY(pKeyframe->aName, pName);

    if ( !stdConffile_Open(pFilename) )
    {
        RDLOG_ERROR("Error: Invalid load filename '%s'.\n", pFilename);
        return 0;
    }

    if ( stdConffile_ScanLine(" section: %s", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" flags %x", &pKeyframe->flags) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" type %x", &pKeyframe->type) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" frames %d", &pKeyframe->numFrames) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" fps %f", &pKeyframe->fps) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" joints %d", &pKeyframe->numJoints) != 1
        || pKeyframe->numJoints > RDPUPPET_MAX_KFNODES ) // Added: bounds check
    {
        goto syntax_error;
    }

    pKeyframe->aNodes = (rdKeyframeNode*)STDMALLOC(sizeof(rdKeyframeNode) * pKeyframe->numJoints);
    if ( !pKeyframe->aNodes )
    {
        RDLOG_ERROR("Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_g_aLine, pFilename);
        stdConffile_Close();
        return 0;
    }

    STD_ZEROMEM(pKeyframe->aNodes, sizeof(rdKeyframeNode) * pKeyframe->numJoints);

    if ( stdConffile_ScanLine(" section: %s", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)) != 1 )
    {
        goto syntax_error;
    }

    if ( streq(std_g_genBuffer, "markers") )
    {
        if ( stdConffile_ScanLine(" markers %d", &pKeyframe->numMarkers) != 1 || pKeyframe->numMarkers > RDKEYFRAME_MAX_MARKERS )
        {
            goto syntax_error;
        }

        if ( pKeyframe->numMarkers )
        {
            for ( size_t i = 0; i < pKeyframe->numMarkers; i++ )
            {
                if ( stdConffile_ScanLine("%f %d", &pKeyframe->aMarkerFrames[i], &pKeyframe->aMarkerTypes[i]) != 2 )
                {
                    goto syntax_error;
                }
            }
        }

        // Read next keyframe nodes section
        if ( stdConffile_ScanLine(" section: %s", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)) != 1 )
        {
            goto syntax_error;
        }
    }

    size_t numNodes = 0;
    if ( stdConffile_ScanLine(" nodes %d", &numNodes) != 1
        || numNodes > pKeyframe->numJoints ) // Added: bounds check
    {
        goto syntax_error;
    }

    for ( size_t nodeNum = 0; nodeNum < numNodes; nodeNum++ )
    {
        int nnum = 0;
        if ( stdConffile_ScanLine(" node %d", &nnum) != 1 )
        {
            goto syntax_error;
        }

        // Fixed: The node id is serialized; validate it before indexing aNodes.
        if ( nnum < 0 || (size_t)nnum >= pKeyframe->numJoints )
        {
            goto syntax_error;
        }

        rdKeyframeNode* pNode = &pKeyframe->aNodes[nnum];
        pNode->nodeNum = nnum;

        if ( stdConffile_ScanLine(" mesh name %s", pNode->aMeshName, (rsize_t)sizeof(pNode->aMeshName)) != 1 )
        {
            STD_ZEROMEM(pNode, sizeof(*pNode)); // Added
            goto syntax_error;
        }

        if ( stdConffile_ScanLine(" entries %d", &pNode->numEntries) != 1 )
        {
            STD_ZEROMEM(pNode, sizeof(*pNode)); // Added
            goto syntax_error;
        }

        pNode->aEntries = (rdKeyframeNodeEntry*)STDMALLOC(sizeof(rdKeyframeNodeEntry) * pNode->numEntries);
        if ( !pNode->aEntries )
        {
            RDLOG_ERROR("Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_g_aLine, pFilename);
            stdConffile_Close();
            return 0;
        }

        if ( pNode->numEntries )
        {
            for ( size_t entryNum = 0; entryNum < pNode->numEntries; entryNum++ )
            {
                rdKeyframeNodeEntry* pEntry = &pNode->aEntries[entryNum];
                int num = 0;
                size_t rlen = stdConffile_ScanLine(" %d: %f %x %f %f %f %f %f %f",
                    &num, &pEntry->frame, &pEntry->flags, &pEntry->pos.x, &pEntry->pos.y, &pEntry->pos.z, &pEntry->rot.pitch, &pEntry->rot.yaw, &pEntry->rot.roll
                );

                if ( rlen != 9 )
                {
                    STD_ZEROMEM(pEntry, sizeof(*pEntry));

                    if ( rlen < 0 )
                    {
                        // line couldn't be read probably end of file reached
                        RDLOG_ERROR("Error: Unexpected EOF in '%s'\n", pFilename);
                        stdConffile_Close();
                        return 0;
                    }

                    // There was syntax error
                    goto syntax_error;
                }

                if ( stdConffile_ScanLine(" %f %f %f %f %f %f", &pEntry->dpos.x, &pEntry->dpos.y, &pEntry->dpos.z, &pEntry->drot.pitch, &pEntry->drot.yaw, &pEntry->drot.roll) != 6 )
                {
                    STD_ZEROMEM(pEntry, sizeof(*pEntry));
                    goto syntax_error;
                }
            }
        }
    }

    // Success
    stdConffile_Close();
    return 1;

syntax_error:
    RDLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, pFilename);
    stdConffile_Close();
    return 0;
}

void J3DAPI rdKeyframe_Free(rdKeyframe* pKeyframe)
{
    if ( !pKeyframe )
    {
        RDLOG_ERROR("Warning: Attempt to free NULL keyframe ptr.\n");
        return;
    }

    if ( rdKeyframe_pKeyframeUnloader )
    {
        rdKeyframe_pKeyframeUnloader(pKeyframe);
    }
    else
    {
        rdKeyframe_FreeEntry(pKeyframe);
        stdMemory_Free(pKeyframe);
        RDLOG_DEBUG("Keyframe successfully freed.\n");
    }
}

void J3DAPI rdKeyframe_FreeEntry(rdKeyframe* pKeyframe)
{
    if ( pKeyframe->aNodes )
    {
        rdKeyframeNode* pCurNode = pKeyframe->aNodes;
        for ( size_t i = 0; i < pKeyframe->numJoints; ++i )
        {
            if ( pCurNode->aEntries )
            {
                stdMemory_Free(pCurNode->aEntries);
                pCurNode->aEntries = NULL;
            }

            ++pCurNode;
        }

        stdMemory_Free(pKeyframe->aNodes);
        pKeyframe->aNodes = NULL;
    }
}

void rdKeyframe_PrintKeyframe(const rdKeyframe* pKeyframe)
{
    RDLOG_DEBUG("Keyframe\n");
    RDLOG_DEBUG("-----------------\n");
    RDLOG_DEBUG("Name %s\n", pKeyframe->aName);
    RDLOG_DEBUG("Num %d\n", pKeyframe->idx);
    RDLOG_DEBUG("Flags 0x%04x\n", pKeyframe->flags);
    RDLOG_DEBUG("Type 0x%X\n", pKeyframe->type);
    RDLOG_DEBUG("Frames %d\n", pKeyframe->numFrames);
    RDLOG_DEBUG("Fps %.3f\n", pKeyframe->fps);
    RDLOG_DEBUG("Joints %d\n\n", pKeyframe->numJoints);

    RDLOG_DEBUG("Markers %d\n", pKeyframe->numMarkers);
    for ( size_t i = 0; i < pKeyframe->numMarkers; i++ )
    {
        RDLOG_DEBUG("  %f %d\n", pKeyframe->aMarkerFrames[i], pKeyframe->aMarkerTypes[i]);
    }

    RDLOG_DEBUG("\n");
    RDLOG_DEBUG("Nodes \n");

    for ( size_t i = 0; i < pKeyframe->numJoints; i++ )
    {
        RDLOG_DEBUG("  Node    %d\n", i);
        RDLOG_DEBUG("  Mesh Name %s\n", pKeyframe->aNodes[i].aMeshName);
        RDLOG_DEBUG("  Entries %d\n\n", pKeyframe->aNodes[i].numEntries);

        RDLOG_DEBUG("  num:   frame:   flags:           x:           y:           z:           p:           y:           r:\n");
        RDLOG_DEBUG("                                  dx:          dy:          dz:          dp:          dy:          dr:\n");

        for ( size_t j = 0; j < pKeyframe->aNodes[i].numEntries; j++ )
        {
            const rdKeyframeNodeEntry* pEntry = &pKeyframe->aNodes[i].aEntries[j];
            RDLOG_DEBUG("  %3d:  %7d   0x%04x %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n", i, (int)pEntry->frame, pEntry->flags, pEntry->pos.x, pEntry->pos.y, pEntry->pos.z, pEntry->rot.pitch, pEntry->rot.yaw, pEntry->rot.roll);
            RDLOG_DEBUG("  %35.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n", pEntry->dpos.x, pEntry->dpos.y, pEntry->dpos.z, pEntry->drot.pitch, pEntry->drot.yaw, pEntry->drot.roll);
        }
        RDLOG_DEBUG("\n");
    }
}