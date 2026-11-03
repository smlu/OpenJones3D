#include "rdFace.h"
#include <j3dcore/j3dhook.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/RTI/symbols.h>
#include <std/General/stdMemory.h>

void rdFace_ResetGlobals(void)
{}

rdFace* J3DAPI rdFace_New(void)
{
    rdFace* pFace;

    pFace = (rdFace*)STDMALLOC(sizeof(rdFace));
    if ( !pFace )

    {
        RDLOG_ERROR("Error allocating memory for face.\n");
        return NULL;
    }

    rdFace_NewEntry(pFace);
    RDLOG_DEBUG("New face created.\n");
    return pFace;
}

int J3DAPI rdFace_NewEntry(rdFace* pFace)
{
    pFace->num   = 0;
    pFace->flags = 0;

    pFace->numVertices  = 0;
    pFace->aVertices    = NULL;
    pFace->aTexVertices = NULL;

    pFace->pMaterial = NULL;
    pFace->matCelNum = -1;

    pFace->normal = rdroid_g_zeroVector3;

    pFace->texVertOffset.x = 0.0f;
    pFace->texVertOffset.y = 0.0f;

    pFace->extraLight.red   = 0.0f;
    pFace->extraLight.green = 0.0f;
    pFace->extraLight.blue  = 0.0f;
    pFace->extraLight.alpha = 0.0f;
    return 1;
}

void J3DAPI rdFace_Free(rdFace* pFace)
{
    if ( !pFace )
    {
        RDLOG_ERROR("Warning: Attempt to free NULL face ptr.\n");
        return;
    }

    rdFace_FreeEntry(pFace);
    stdMemory_Free(pFace);
}

void J3DAPI rdFace_FreeEntry(rdFace* pFace)
{
    if ( pFace->aVertices )
    {
        stdMemory_Free(pFace->aVertices);
    }

    if ( pFace->aTexVertices )
    {
        stdMemory_Free(pFace->aTexVertices);
    }
}