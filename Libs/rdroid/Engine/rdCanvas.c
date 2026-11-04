#include "rdCanvas.h"

#include <rdroid/Main/rdroid.h>

#include <std/General/stdMemory.h>

void rdCanvas_ResetGlobals(void)
{

}

rdCanvas* J3DAPI rdCanvas_New(int flags, tVBuffer* pVBuffer, int left, int top, int right, int bottom)
{
    rdCanvas* pCanvas = (rdCanvas*)STDMALLOC(sizeof(rdCanvas));
    if ( !pCanvas )
    {
        RDLOG_ERROR("Error allocating memory for canvas.\n");
        return NULL;
    }

    rdCanvas_NewEntry(pCanvas, flags, pVBuffer, left, top, right, bottom);
    return pCanvas;
}

int J3DAPI rdCanvas_NewEntry(rdCanvas* pCanvas, int flags, tVBuffer* pVBuffer, int left, int top, int right, int bottom)
{
    pCanvas->flags    = flags;
    pCanvas->pVBuffer = pVBuffer;
    if ( (flags & 0x01) != 0 )
    {
        pCanvas->rect.left   = left;
        pCanvas->rect.top    = top;
        pCanvas->rect.right  = right;
        pCanvas->rect.bottom = bottom;
    }
    else
    {
        pCanvas->rect.left   = 0;
        pCanvas->rect.top    = 0;
        pCanvas->rect.right  = pVBuffer->rasterInfo.width - 1;
        pCanvas->rect.bottom = pVBuffer->rasterInfo.height - 1;
    }

    pCanvas->center.x = (float)(pCanvas->rect.right - pCanvas->rect.left + 1) * 0.5f + (float)pCanvas->rect.left;
    pCanvas->center.y = (float)(pCanvas->rect.bottom - pCanvas->rect.top + 1) * 0.5f + (float)pCanvas->rect.top;
    return 1;
}

void J3DAPI rdCanvas_Free(rdCanvas* pCanvas)
{
    if ( !pCanvas )
    {
        RDLOG_ERROR("Warning: Attempt to free NULL Canvas ptr.\n");
        return;
    }

    rdCanvas_FreeEntry(pCanvas);
    stdMemory_Free(pCanvas);
}

void J3DAPI rdCanvas_FreeEntry(rdCanvas* pCanvas)
{
    J3D_UNUSED(pCanvas);
}
