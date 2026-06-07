#include "std.h"
#include "stdMemory.h"
#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

#include <stdint.h>

#define STDMEMORY_HEADERMAGIC    0x12345678
#define STDMEMORY_UNINIT_PATTERN 0xCC
#define STDMEMORY_FREE_PATTERN   0xDD

#define STDMEMORY_GETHEADER(pBytes) \
    ((tMemoryHeader*)((char*)(pBytes) - sizeof(tMemoryHeader)))

#define STDMEMORY_GETHEAPALLOCSIZE(size) \
    ((size) + sizeof(tMemoryHeap))

//  Memory block & zone related helper macros

#define STDMEMORYBLOCK_GET_ZONEINDEX(pBlock) \
    ((size_t)((pBlock) - stdMemory_aZoneData)) // Get index of zone in stdMemory_aZoneData array

// Get pointer to data that is allocated after the header
#define STDMEMORYBLOCK_ZONE_GETDATA(pHeader) \
    ((void*)&((pHeader)[1]))

// Get pointer to header from data pointer
#define STDMEMORYBLOCK_ZONE_GETHEADER_FROMDATA(pData) \
    ((tMemoryBlockHeader*)((char*)(pData) - sizeof(tMemoryBlockHeader)))

// Calculate full size of block zone including header from data size
#define STDMEMORYBLOCK_ZONE_CALCBLOCKSIZE(dataSize) \
    ((dataSize) + sizeof(tMemoryBlockHeader))

// Calculate size of data in zone block header from full block size
#define STDMEMORYBLOCK_ZONE_CALCDATASIZE(blockSize) \
    ((blockSize) - sizeof(tMemoryBlockHeader))

// Pack both 16-bit values: lower bits = available, upper bits = used
#define STDMEMORYBLOCK_ZONE_PACKSIZES(avail, prevOffset) \
    (((uint32_t)(avail) & 0xFFFFu) | (((uint32_t)(prevOffset) & 0xFFFFu) << 16))

// Get/Set available size in zone block header (low 16 bits)
#define STDMEMORYBLOCK_ZONE_GETAVAIL(size) ((uint32_t)((size) & 0xFFFFu))
#define STDMEMORYBLOCK_ZONE_SETAVAIL(size, val) \
    ((size) = ((size) & 0xFFFF0000u) | ((uint32_t)(val) & 0xFFFFu))

// Get/Set previous-header offset (high 16 bits)
#define STDMEMORYBLOCK_ZONE_GETPREVOFFSET(size) ((uint32_t)(((size) >> 16) & 0xFFFFu))
#define STDMEMORYBLOCK_ZONE_SETPREVOFFSET(size, val) \
    ((size) = ((size) & 0xFFFFu) | (((uint32_t)(val) & 0xFFFFu) << 16))

// Walk to next/previous zone block headers
#define STDMEMORYBLOCK_ZONE_NEXTHEADER_ATPOS(hdr, idx) \
    ((tMemoryBlockHeader*)(((char*)(hdr)) + (idx)))

#define STDMEMORYBLOCK_ZONE_NEXTHEADER(hdr) \
    STDMEMORYBLOCK_ZONE_NEXTHEADER_ATPOS(hdr, STDMEMORYBLOCK_ZONE_GETAVAIL((hdr)->size) & ~STDMEMORYBLOCK_ZONE_INUSEMASK)

#define STDMEMORYBLOCK_ZONE_PREVHEADER(hdr) \
    ((tMemoryBlockHeader*)(((char*)(hdr)) - \
        STDMEMORYBLOCK_ZONE_GETPREVOFFSET((hdr)->size)))

// Memory block related constants
static_assert(sizeof(void*) == 4, "stdMemory: Only 32-bit system supported");

#define STDMEMORYBLOCK_MAXBLOCKS        8456      // Number could represent targeted 256 MB memory pool. As 8456 * 31744 = 268435456 bytes (~256 MiB)
#define STDMEMORYBLOCK_ALLOC_THRESHOLD  4096     // Maximum allocation size to go into zone blocks (maybe to fit system page size?)

#define STDMEMORYBLOCK_ZONE_INUSEMASK   0x8000
#define STDMEMORYBLOCK_ZONE_MAXDATASIZE 31728    // Available memory per zone

#define STDMEMORYBLOCK_ZONE_INITIALBLOCKSIZE (STDMEMORYBLOCK_ZONE_CALCBLOCKSIZE(STDMEMORYBLOCK_ZONE_MAXDATASIZE))    // Zone size + one header
static_assert(STDMEMORYBLOCK_ZONE_INITIALBLOCKSIZE == 31736, "STDMEMORYBLOCK_ZONE_INITIALBLOCKSIZE must be 31736 bytes");

#define STDMEMORYBLOCK_ZONE_ALLOCSIZE       (STDMEMORYBLOCK_ZONE_INITIALBLOCKSIZE + sizeof(tMemoryBlockHeader))    // Total allocated size when allocating a zone heap data
static_assert(STDMEMORYBLOCK_ZONE_ALLOCSIZE == 31744, "STDMEMORYBLOCK_ZONE_ALLOCSIZE must be 31744 bytes");

// Module vars
static bool bStartup = false;
static bool bOpen    = false;

static tMemoryBlock stdMemory_aZoneData[STDMEMORYBLOCK_MAXBLOCKS] = { 0 }; // Array of memory blocks; Altered: Init to 0

void stdMemory_InstallHooks(void)
{
    J3D_HOOKFUNC(stdMemory_Startup);
    J3D_HOOKFUNC(stdMemory_Shutdown);
    J3D_HOOKFUNC(stdMemory_Open);
    J3D_HOOKFUNC(stdMemory_Close);
    J3D_HOOKFUNC(stdMemory_Malloc);
    J3D_HOOKFUNC(stdMemory_Free);
    J3D_HOOKFUNC(stdMemory_Realloc);
    J3D_HOOKFUNC(stdMemory_BlockMalloc);
    J3D_HOOKFUNC(stdMemory_BlockFree);
    J3D_HOOKFUNC(stdMemory_BlockRealloc);
    J3D_HOOKFUNC(stdMemory_BlockAlloc);
}

void stdMemory_ResetGlobals(void)
{
    memset(&stdMemory_g_curState, 0, sizeof(stdMemory_g_curState));
}

int stdMemory_Startup(void)
{
    STD_ASSERTREL(bStartup == false);
    memset(&stdMemory_g_curState, 0, sizeof(stdMemory_g_curState));
    bStartup = true;
    return 1;
}

void stdMemory_Shutdown(void)
{
    STD_ASSERTREL(bStartup == true);
    // TODO: Should we close the system here?
    bStartup = false;
}

int stdMemory_Open(void)
{
    STD_ASSERTREL(bStartup == true);
    if ( bOpen )
    {
        STDLOG_ERROR("Warning: System already open!\n");
        return 0;
    }
    else
    {
        bOpen = true;
        return 1;
    }
}

void stdMemory_Close(void)
{
    if ( !bOpen )
    {
        STDLOG_ERROR("Warning: System already closed!\n");
        return;
    }

    STDLOG_STATUS("Maximum dynamic memory used: %d bytes.\n", stdMemory_g_curState.maxBytes);

    if ( stdMemory_g_curState.totalAllocs || stdMemory_g_curState.totalBytes )
    {
        STDLOG_ERROR("UNFREED MEMORY ALERT!\n\n");
        std_g_pHS->pErrorPrint("File\tLine\tSize\tNumber\n\n");

        tMemoryHeader* pCur = stdMemory_g_curState.header.pNext;
        size_t count = 0;
        while ( pCur && count < 100 )
        {
            std_g_pHS->pErrorPrint("%s\t%d\t%d\t%d\n", pCur->pFilename, pCur->line, pCur->size, pCur->number);
            pCur = pCur->pNext;;
            ++count;
        }
    }

    bOpen = false;
}

void* J3DAPI stdMemory_Malloc(size_t size, const char* pFilename, size_t line)
{
    // Fixed: Reject allocation/accounting sizes that would overflow the heap header math.
    if ( size > SIZE_MAX - sizeof(tMemoryHeap)
        || size > SIZE_MAX - stdMemory_g_curState.totalBytes )
    {
        return NULL;
    }

    tMemoryHeap* pHeap = (tMemoryHeap*)std_g_pHS->pMalloc(STDMEMORY_GETHEAPALLOCSIZE(size)); // In stdPlatform module pMalloc is set to stdMemory_BlockMalloc
    if ( !pHeap )
    {
        return NULL;
    }

    pHeap->header.number    = stdMemory_g_curState.totalAllocs;
    pHeap->header.id        = (uint32_t)pHeap;
    pHeap->header.size      = size;
    pHeap->header.pFilename = pFilename;
    pHeap->header.line      = line;
    pHeap->header.pNext     = stdMemory_g_curState.header.pNext;
    if ( pHeap->header.pNext )
    {
        pHeap->header.pNext->pPrev = &pHeap->header;
    }

    pHeap->header.pPrev = &stdMemory_g_curState.header;
    stdMemory_g_curState.header.pNext = &pHeap->header;

    // Set the uninitialized memory pattern
    memset(&pHeap->pMemory, STDMEMORY_UNINIT_PATTERN, size);

    pHeap->header.magic = STDMEMORY_HEADERMAGIC;
    *(void**)((char*)&pHeap->pMemory + size) = (void*)STDMEMORY_HEADERMAGIC; // Set end marker

    stdMemory_g_curState.totalBytes += size;

    size_t totalBytes = stdMemory_g_curState.maxBytes;
    if ( stdMemory_g_curState.maxBytes <= stdMemory_g_curState.totalBytes )
    {
        totalBytes = stdMemory_g_curState.totalBytes;
    }

    stdMemory_g_curState.maxBytes = totalBytes;
    ++stdMemory_g_curState.totalAllocs;
    return &pHeap->pMemory;
}

void J3DAPI stdMemory_Free(void* pBytes)
{
    // Added: Match free(NULL) behavior in release builds.
    STD_GUARD_VOID(pBytes);

    tMemoryHeader* pHeader = STDMEMORY_GETHEADER(pBytes);

    STD_ASSERTREL((pHeader != NULL) && ((uint32_t)pHeader == pHeader->id));
    STD_ASSERTREL(pHeader->magic == STDMEMORY_HEADERMAGIC);
    STD_ASSERTREL(*(uint32_t*)((uint8_t*)pBytes + pHeader->size) == STDMEMORY_HEADERMAGIC);

    // Set freed memory to a free pattern
    memset(pBytes, STDMEMORY_FREE_PATTERN, pHeader->size);

    if ( pHeader->pNext )
    {
        pHeader->pNext->pPrev = pHeader->pPrev;
    }

    STD_ASSERTREL(pHeader->pPrev);
    pHeader->pPrev->pNext = pHeader->pNext;

    stdMemory_g_curState.totalBytes -= pHeader->size;
    --stdMemory_g_curState.totalAllocs;
    std_g_pHS->pFree(pHeader);
}

void* J3DAPI stdMemory_Realloc(void* pBytes, size_t size, const char* pFilename, size_t line)
{
    if ( !pBytes )
    {
        return stdMemory_Malloc(size, pFilename, line);
    }

    if ( !size )
    {
        stdMemory_Free(pBytes);
        return NULL;
    }

    // Fixed: Reject sizes that would overflow the debug heap header allocation.
    if ( size > SIZE_MAX - sizeof(tMemoryHeap) )
    {
        return NULL;
    }

    tMemoryHeader* pHeader = STDMEMORY_GETHEADER(pBytes);
    STD_ASSERTREL((pHeader != NULL) && ((uint32_t)pHeader == pHeader->id));

    size_t curSize = pHeader->size;
    // Fixed: Avoid total-byte accounting overflow when a reallocation grows.
    if ( size > curSize && size - curSize > SIZE_MAX - stdMemory_g_curState.totalBytes )
    {
        return NULL;
    }

    tMemoryHeap* pHeap = (tMemoryHeap*)std_g_pHS->pRealloc(pHeader, STDMEMORY_GETHEAPALLOCSIZE(size));
    if ( !pHeap )
    {
        return NULL;
    }

    pHeap->header.id        = (uint32_t)pHeap;
    pHeap->header.size      = size;
    pHeap->header.pFilename = pFilename;
    pHeap->header.line      = line;
    if ( pHeap->header.pNext )
    {
        pHeap->header.pNext->pPrev = &pHeap->header;
    }

    if ( pHeap->header.pPrev )
    {
        pHeap->header.pPrev->pNext = &pHeap->header;
    }

    // TODO: If new size is larger set the unused memory to uninitialized pattern

    pHeap->header.magic = STDMEMORY_HEADERMAGIC;
    *(void**)((char*)&pHeap->pMemory + size) = (void*)STDMEMORY_HEADERMAGIC; // Set end marker

    if ( size >= curSize )
    {
        stdMemory_g_curState.totalBytes +=  size - curSize;
    }
    else
    {
        // Fixed: Handle reductions correctly
        stdMemory_g_curState.totalBytes -= curSize - size;
    }

    size_t totalBytes = stdMemory_g_curState.maxBytes;
    if ( stdMemory_g_curState.maxBytes <= stdMemory_g_curState.totalBytes )
    {
        totalBytes = stdMemory_g_curState.totalBytes;
    }

    stdMemory_g_curState.maxBytes = totalBytes;
    return &pHeap->pMemory;
}

void* J3DAPI stdMemory_BlockMalloc(size_t size)
{
    // Fixed: Avoid zero-sized zone allocation and null best-block assertions.
    if ( !size )
    {
        return NULL;
    }

    if ( size > STDMEMORYBLOCK_ALLOC_THRESHOLD )
    {
        // Allocate memory normally on heap
        return stdMemory_BlockAlloc(size);
    }

    // Fixed: Avoid overflow in the 4-byte alignment adjustment.
    if ( size > SIZE_MAX - 3u )
    {
        return NULL;
    }

    size = (size + 3) & ~3u; // 4-byte alignment

    // Try to find zone with enough free memory
    tMemoryBlock* pBlock = NULL;
    for ( size_t i = 0; i < STDMEMORYBLOCK_MAXBLOCKS; i++ )
    {
        if ( !stdMemory_aZoneData[i].bAllocated ||
            stdMemory_aZoneData[i].availableMem >= size )
        {
            pBlock = &stdMemory_aZoneData[i];
            break;
        }
    }

    if ( !pBlock || !pBlock->bAllocated )
    {
        // Try to find empty block
        for ( size_t i = 0; i < STDMEMORYBLOCK_MAXBLOCKS; i++ )
        {
            if ( !stdMemory_aZoneData[i].bAllocated || !stdMemory_aZoneData[i].pFirst )
            {
                pBlock = &stdMemory_aZoneData[i];
                break;
            }
        }

        if ( !pBlock )
        {
            // Not enough memory in any zone or no free blocks available, allocate memory normally
            return stdMemory_BlockAlloc(size);
        }

        // Initialize new zone
        tMemoryBlockHeader* pNewHeap = (tMemoryBlockHeader*)malloc(STDMEMORYBLOCK_ZONE_ALLOCSIZE);
        if ( !pNewHeap )
        {
            return stdMemory_BlockAlloc(size);
        }

        pBlock->pLargestFree = pNewHeap;
        pBlock->pFirst       = pNewHeap;
        pBlock->availableMem = STDMEMORYBLOCK_ZONE_MAXDATASIZE;
        pBlock->bAllocated   = 1;

        // Set up start header
        pNewHeap->size   = STDMEMORYBLOCK_ZONE_PACKSIZES(STDMEMORYBLOCK_ZONE_INITIALBLOCKSIZE, 0);
        pNewHeap->pBlock = pBlock;

        // Set up end marker
        tMemoryBlockHeader* pEndHeader = STDMEMORYBLOCK_ZONE_NEXTHEADER_ATPOS(pNewHeap, STDMEMORYBLOCK_ZONE_INITIALBLOCKSIZE);
        pEndHeader->size   = STDMEMORYBLOCK_ZONE_PACKSIZES(0, STDMEMORYBLOCK_ZONE_INITIALBLOCKSIZE);
        pEndHeader->pBlock = pBlock;
    }

    // Find best fit block
    size_t bestFitSize = STDMEMORYBLOCK_ZONE_ALLOCSIZE;
    tMemoryBlockHeader* pBest = NULL;

    tMemoryBlockHeader* pCurHeader = pBlock->pFirst;
    uint32_t requiredSize = STDMEMORYBLOCK_ZONE_CALCBLOCKSIZE(size);

    for ( size_t curSize = STDMEMORYBLOCK_ZONE_GETAVAIL(pCurHeader->size); curSize; curSize = STDMEMORYBLOCK_ZONE_GETAVAIL(pCurHeader->size) )
    {
        if ( (curSize & STDMEMORYBLOCK_ZONE_INUSEMASK) != 0 )
        {
            // heap fragment is used, skip it
            curSize &= ~STDMEMORYBLOCK_ZONE_INUSEMASK;
        }
        else if ( curSize >= requiredSize )
        {
            size_t remainingSize = curSize - requiredSize;
            if ( remainingSize < bestFitSize )
            {
                bestFitSize = remainingSize;
                pBest       = pCurHeader;
            }
        }

        pCurHeader = STDMEMORYBLOCK_ZONE_NEXTHEADER_ATPOS(pCurHeader, curSize);
    }

    STD_ASSERTREL(pBest);

    // Mark the block as used
    size_t originalSize = STDMEMORYBLOCK_ZONE_GETAVAIL(pBest->size);
    STDMEMORYBLOCK_ZONE_SETAVAIL(pBest->size, requiredSize | STDMEMORYBLOCK_ZONE_INUSEMASK);

    if ( originalSize != requiredSize )
    {
        // Split the block - create new free block for remainder
        tMemoryBlockHeader* pNewFreeBlock = STDMEMORYBLOCK_ZONE_NEXTHEADER_ATPOS(pBest, requiredSize);
        size_t remainingSize = originalSize - requiredSize;
        pNewFreeBlock->size = STDMEMORYBLOCK_ZONE_PACKSIZES(remainingSize, requiredSize);

        // Update end marker
        tMemoryBlockHeader* pEndMarker = STDMEMORYBLOCK_ZONE_NEXTHEADER_ATPOS(pBest, originalSize);
        STDMEMORYBLOCK_ZONE_SETPREVOFFSET(pEndMarker->size, remainingSize);
    }

    pBest->pBlock = pBlock;

    // If pBest is not last
    if ( pBest != pBlock->pLargestFree )
    {
        return STDMEMORYBLOCK_ZONE_GETDATA(pBest); // Return pointer to memory after header to allocated memory
    }

    // Update zone's largest free block tracking
    // Find new largest free block
    size_t largestFreeSize = sizeof(tMemoryBlockHeader);
    tMemoryBlockHeader* pLargestFree = NULL;

    pCurHeader = pBlock->pFirst;
    for ( size_t curSize = STDMEMORYBLOCK_ZONE_GETAVAIL(pCurHeader->size); curSize; curSize = STDMEMORYBLOCK_ZONE_GETAVAIL(pCurHeader->size) )
    {
        if ( (curSize & STDMEMORYBLOCK_ZONE_INUSEMASK) != 0 )
        {
            curSize &= ~STDMEMORYBLOCK_ZONE_INUSEMASK;
        }
        else if ( curSize > largestFreeSize )
        {
            largestFreeSize = curSize;
            pLargestFree = pCurHeader;
        }

        pCurHeader = STDMEMORYBLOCK_ZONE_NEXTHEADER_ATPOS(pCurHeader, curSize);
    }

    pBlock->availableMem = (largestFreeSize > sizeof(tMemoryBlockHeader))
        ? STDMEMORYBLOCK_ZONE_CALCDATASIZE(largestFreeSize)
        : 0;
    pBlock->pLargestFree = pLargestFree;
    return STDMEMORYBLOCK_ZONE_GETDATA(pBest); // Return pointer after header to allocated memory
}

void J3DAPI stdMemory_BlockFree(void* pMemory)
{
    STD_ASSERTREL(pMemory); // Fixed: Moved this check to the top before extracting header to prevent dereferencing NULL pointer
    // Added: Keep release builds from dereferencing NULL block pointers.
    STD_GUARD_VOID(pMemory);

    tMemoryBlockHeader* pHeader = STDMEMORYBLOCK_ZONE_GETHEADER_FROMDATA(pMemory);

    tMemoryBlock* pBlock = pHeader->pBlock;
    if ( !pBlock )
    {
        // Not part of zone data blocks, just free memory that was allocated by stdMemory_BlockAlloc
        free(pHeader);
        return;
    }

    if ( (STDMEMORYBLOCK_ZONE_GETAVAIL(pHeader->size) & STDMEMORYBLOCK_ZONE_INUSEMASK) == 0
        || STDMEMORYBLOCK_GET_ZONEINDEX(pBlock) >= STDMEMORYBLOCK_MAXBLOCKS )
    {
        std_g_pHS->pWarningPrint("Attempting to dispose a bogus or already-disposed-of block!\n"); // Altered: Added line brake
        return;
    }

    size_t availableSize = STDMEMORYBLOCK_ZONE_GETAVAIL(pHeader->size) & ~STDMEMORYBLOCK_ZONE_INUSEMASK;

    // Check if we can merge with next block
    tMemoryBlockHeader* pNextHeader = STDMEMORYBLOCK_ZONE_NEXTHEADER_ATPOS(pHeader, availableSize);
    size_t nextSize = STDMEMORYBLOCK_ZONE_GETAVAIL(pNextHeader->size);

    if ( (nextSize & STDMEMORYBLOCK_ZONE_INUSEMASK) == 0 ) // Check if next block is free
    {
        availableSize += nextSize;
    }

    // Check if we can merge with previous block
    tMemoryBlockHeader* pPrevHeader = STDMEMORYBLOCK_ZONE_PREVHEADER(pHeader);

    size_t prevSize = STDMEMORYBLOCK_ZONE_GETAVAIL(pPrevHeader->size);
    if ( (prevSize & STDMEMORYBLOCK_ZONE_INUSEMASK) == 0 ) // Check if previous block is free
    {
        availableSize += prevSize;
        pHeader = pPrevHeader;
    }

    // Update the merged free block
    STDMEMORYBLOCK_ZONE_SETAVAIL(pHeader->size, availableSize);

    // Update end marker
    tMemoryBlockHeader* pEndMarker = STDMEMORYBLOCK_ZONE_NEXTHEADER_ATPOS(pHeader, availableSize);
    STDMEMORYBLOCK_ZONE_SETPREVOFFSET(pEndMarker->size, availableSize);

    // Check if entire zone block is free
    if ( STDMEMORYBLOCK_ZONE_CALCDATASIZE(availableSize) > pBlock->availableMem )
    {
        if ( availableSize == STDMEMORYBLOCK_ZONE_INITIALBLOCKSIZE )
        {
            // Entire zone is free, deallocate it
            free(pBlock->pFirst);
            pBlock->pFirst       = NULL;
            pBlock->availableMem = 0;
            pBlock->pLargestFree = NULL;
            pBlock->bAllocated   = 0;
        }
        else
        {
            // Update largest free block tracking
            pBlock->availableMem = STDMEMORYBLOCK_ZONE_CALCDATASIZE(availableSize);
            pBlock->pLargestFree = pHeader;
        }
    }
}

void* J3DAPI stdMemory_BlockRealloc(void* pMemory, size_t size)
{
    if ( !pMemory )
    {
        return stdMemory_BlockMalloc(size);
    }

    if ( !size )
    {
        stdMemory_BlockFree(pMemory);
        return NULL;
    }

    tMemoryBlockHeader* pHeader = STDMEMORYBLOCK_ZONE_GETHEADER_FROMDATA(pMemory); // Fixed: Moved this line here to ensue pMemory is not NULL. Originally was at the top before null checks.

    void* pNewData = stdMemory_BlockMalloc(size);
    if ( !pNewData )
    {
        return NULL;
    }

    size_t curAvailable;
    if ( pHeader->pBlock )
    {
        curAvailable = STDMEMORYBLOCK_ZONE_CALCDATASIZE((STDMEMORYBLOCK_ZONE_GETAVAIL(pHeader->size) & ~STDMEMORYBLOCK_ZONE_INUSEMASK));
    }
    else
    {
        // Not part of zone block data. Data was allocated by stdMemory_BlockAlloc function
        curAvailable = STDMEMORYBLOCK_ZONE_CALCDATASIZE(pHeader->size);
    }

    size_t copySize = (size < curAvailable) ? size : curAvailable;
    memcpy(pNewData, pMemory, copySize); // TODO: Verify that this is correct and it doesn't corrupt any data of heap header
    stdMemory_BlockFree(pMemory);

    return pNewData;
}

void* J3DAPI stdMemory_BlockAlloc(size_t size)
{
    // Fixed: Reject zero-sized and header-overflowing direct block allocations.
    if ( !size || size > SIZE_MAX - sizeof(tMemoryBlockHeader) )
    {
        return NULL;
    }

    tMemoryBlockHeader* pHeader = (tMemoryBlockHeader*)malloc(STDMEMORYBLOCK_ZONE_CALCBLOCKSIZE(size));
    if ( !pHeader )
    {
        return NULL;
    }

    pHeader->size   = STDMEMORYBLOCK_ZONE_CALCBLOCKSIZE(size);
    pHeader->pBlock = NULL;
    return STDMEMORYBLOCK_ZONE_GETDATA(pHeader);
}
