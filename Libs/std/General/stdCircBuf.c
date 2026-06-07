#include "std.h"
#include "stdMemory.h"
#include "stdCircBuf.h"
#include "stdUtil.h"

#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

#include <stdint.h>

void stdCircBuf_InstallHooks(void)
{
    J3D_HOOKFUNC(stdCircBuf_New);
    J3D_HOOKFUNC(stdCircBuf_Free);
    J3D_HOOKFUNC(stdCircBuf_Purge);
    J3D_HOOKFUNC(stdCircBuf_GetNextElement);
}

void stdCircBuf_ResetGlobals(void)
{}

int J3DAPI stdCircBuf_New(tCircularBuffer* pCirc, int numElementsDesired, int sizeOfEachElement)
{
    STD_ASSERTREL(pCirc != NULL);
    STD_ASSERTREL(numElementsDesired > 0);
    STD_ASSERTREL(sizeOfEachElement > 0);

    // Added: Keep release builds from allocating invalid circular buffers.
    STD_GUARD(pCirc && numElementsDesired > 0 && sizeOfEachElement > 0, 0);

    STD_ZEROMEM(pCirc, sizeof(tCircularBuffer));

    size_t numElements = (size_t)numElementsDesired;
    size_t elementSize = (size_t)sizeOfEachElement;
    // Fixed: Avoid integer overflow in element count * element size.
    if ( elementSize > SIZE_MAX / numElements )
    {
        return 0;
    }

    size_t bufferSize = elementSize * numElements;
    pCirc->paElements = (uint8_t*)STDMALLOC(bufferSize);
    if ( !pCirc->paElements )
    {
        return 0;
    }

    STD_ZEROMEM(pCirc->paElements, bufferSize);
    pCirc->numAllocated = numElementsDesired;
    pCirc->elementSize  = sizeOfEachElement;
    return 1;
}

void J3DAPI stdCircBuf_Free(tCircularBuffer* pCirc)
{
    if ( pCirc->paElements )
    {
        STDFREE(pCirc->paElements);
    }

    pCirc->numAllocated     = 0;
    pCirc->paElements       = 0;
    pCirc->numValidElements = 0;
}

void J3DAPI stdCircBuf_Purge(tCircularBuffer* pCirc)
{
    STD_ASSERTREL(pCirc != NULL);
    if ( pCirc->paElements && pCirc->numValidElements )
    {
        --pCirc->numValidElements;
        STD_ASSERT(pCirc->paElements != NULL);
        pCirc->iFirst = (pCirc->iFirst + 1) % pCirc->numAllocated;
    }
}

void* J3DAPI stdCircBuf_GetNextElement(tCircularBuffer* pCirc)
{
    int idx;

    STD_ASSERTREL(pCirc != NULL);
    if ( !pCirc->paElements )
    {
        return NULL;
    }

    if ( pCirc->numValidElements >= pCirc->numAllocated )
    {
        stdCircBuf_Purge(pCirc);
    }

    STD_ASSERTREL(pCirc->numValidElements <= pCirc->numAllocated);
    STD_ASSERTREL(pCirc->iFirst < pCirc->numAllocated);

    idx = (pCirc->numValidElements + pCirc->iFirst) % pCirc->numAllocated;
    ++pCirc->numValidElements;

    STD_ASSERTREL(pCirc->paElements != NULL);
    return &pCirc->paElements[pCirc->elementSize * idx];
}
