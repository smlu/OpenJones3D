#include "sithRenderVertexWelder.h"

#ifdef J3D_OPENGL

#include <std/General/std.h>
#include <std/General/stdMemory.h>

typedef struct sSithRenderVertexWelderEntry
{
    size_t vertexIndex;
    bool used;
} SithRenderVertexWelderEntry;

static uint32_t sithRenderVertexWelder_HashU32(uint32_t hash, uint32_t value)
{
    hash ^= value;
    hash *= 16777619u;
    return hash;
}

static uint32_t sithRenderVertexWelder_HashFloat(uint32_t hash, float value)
{
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    return sithRenderVertexWelder_HashU32(hash, bits);
}

static uint32_t sithRenderVertexWelder_HashVertex(const D3DTLVERTEX* pVertex)
{
    uint32_t hash = 2166136261u;

    hash = sithRenderVertexWelder_HashFloat(hash, pVertex->sx);
    hash = sithRenderVertexWelder_HashFloat(hash, pVertex->sy);
    hash = sithRenderVertexWelder_HashFloat(hash, pVertex->sz);
    hash = sithRenderVertexWelder_HashFloat(hash, pVertex->nx);
    hash = sithRenderVertexWelder_HashFloat(hash, pVertex->ny);
    hash = sithRenderVertexWelder_HashFloat(hash, pVertex->nz);
    hash = sithRenderVertexWelder_HashFloat(hash, pVertex->tu);
    hash = sithRenderVertexWelder_HashFloat(hash, pVertex->tv);
    hash = sithRenderVertexWelder_HashU32(hash, pVertex->color);

    return hash;
}

static bool sithRenderVertexWelder_AreVerticesEqual(const D3DTLVERTEX* pLeft, const D3DTLVERTEX* pRight)
{
    return memcmp(&pLeft->sx, &pRight->sx, sizeof(pLeft->sx)) == 0
        && memcmp(&pLeft->sy, &pRight->sy, sizeof(pLeft->sy)) == 0
        && memcmp(&pLeft->sz, &pRight->sz, sizeof(pLeft->sz)) == 0
        && memcmp(&pLeft->nx, &pRight->nx, sizeof(pLeft->nx)) == 0
        && memcmp(&pLeft->ny, &pRight->ny, sizeof(pLeft->ny)) == 0
        && memcmp(&pLeft->nz, &pRight->nz, sizeof(pLeft->nz)) == 0
        && memcmp(&pLeft->tu, &pRight->tu, sizeof(pLeft->tu)) == 0
        && memcmp(&pLeft->tv, &pRight->tv, sizeof(pLeft->tv)) == 0
        && pLeft->color == pRight->color;
}

static size_t sithRenderVertexWelder_GetHashCapacity(size_t numVertices)
{
    size_t capacity = 1;
    while ( capacity < numVertices * 2u )
    {
        capacity <<= 1u;
    }

    return capacity;
}

bool sithRenderVertexWelder_Weld(const D3DTLVERTEX* pVertices, size_t numVertices, GLuint* pIndices, size_t numIndices, D3DTLVERTEX** ppOutVertices, size_t* pOutNumVertices)
{
    STD_ASSERTREL(pVertices != NULL);
    STD_ASSERTREL(pIndices != NULL || numIndices == 0);
    STD_ASSERTREL(ppOutVertices != NULL);
    STD_ASSERTREL(pOutNumVertices != NULL);

    *ppOutVertices   = NULL;
    *pOutNumVertices = 0;

    if ( numVertices == 0 )
    {
        return true;
    }

    const size_t hashCapacity                 = sithRenderVertexWelder_GetHashCapacity(numVertices);
    SithRenderVertexWelderEntry* aHashEntries = (SithRenderVertexWelderEntry*)STDMALLOC(hashCapacity * sizeof(*aHashEntries));
    size_t* aRemap                            = (size_t*)STDMALLOC(numVertices * sizeof(*aRemap));
    D3DTLVERTEX* aWeldedVertices              = (D3DTLVERTEX*)STDMALLOC(numVertices * sizeof(*aWeldedVertices));

    if ( !aHashEntries || !aRemap || !aWeldedVertices )
    {
        if ( aHashEntries )
        {
            STDFREE(aHashEntries);
        }
        if ( aRemap )
        {
            STDFREE(aRemap);
        }
        if ( aWeldedVertices )
        {
            STDFREE(aWeldedVertices);
        }
        return false;
    }

    memset(aHashEntries, 0, hashCapacity * sizeof(*aHashEntries));

    size_t numWeldedVertices = 0;
    const size_t hashMask    = hashCapacity - 1u;

    for ( size_t i = 0; i < numVertices; ++i )
    {
        const D3DTLVERTEX* pVertex = &pVertices[i];
        size_t slot                = (size_t)sithRenderVertexWelder_HashVertex(pVertex) & hashMask;

        while ( aHashEntries[slot].used )
        {
            const size_t weldedIndex = aHashEntries[slot].vertexIndex;
            if ( sithRenderVertexWelder_AreVerticesEqual(pVertex, &aWeldedVertices[weldedIndex]) )
            {
                aRemap[i] = weldedIndex;
                break;
            }

            slot = (slot + 1u) & hashMask;
        }

        if ( !aHashEntries[slot].used )
        {
            const size_t weldedIndex       = numWeldedVertices++;
            aWeldedVertices[weldedIndex]   = *pVertex;
            aHashEntries[slot].vertexIndex = weldedIndex;
            aHashEntries[slot].used        = true;
            aRemap[i]                      = weldedIndex;
        }
    }

    for ( size_t i = 0; i < numIndices; ++i )
    {
        const GLuint oldIndex = pIndices[i];
        if ( oldIndex >= numVertices )
        {
            STDFREE(aHashEntries);
            STDFREE(aRemap);
            STDFREE(aWeldedVertices);
            return false;
        }
    }

    for ( size_t i = 0; i < numIndices; ++i )
    {
        pIndices[i] = (GLuint)aRemap[pIndices[i]];
    }

    STDFREE(aHashEntries);
    STDFREE(aRemap);

    *ppOutVertices   = aWeldedVertices;
    *pOutNumVertices = numWeldedVertices;
    return true;
}

#endif // J3D_OPENGL
