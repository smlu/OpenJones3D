#include <unity_fixture.h>

#include <stdint.h>
#include <string.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

static void (J3DAPI *stdMemoryTest_pfOriginalFree)(void*);
static uint8_t stdMemoryTest_aFreedPayload[8];
static void* stdMemoryTest_pFreedHeap;
static size_t stdMemoryTest_numFreeCalls;

static void J3DAPI stdMemoryTest_CaptureFree(void* pHeap)
{
    stdMemoryTest_pFreedHeap = pHeap;
    ++stdMemoryTest_numFreeCalls;
    memcpy(stdMemoryTest_aFreedPayload, &((tMemoryHeap*)pHeap)->pMemory, sizeof(stdMemoryTest_aFreedPayload));
    stdMemoryTest_pfOriginalFree(pHeap);
}

TEST_GROUP(stdMemory);

static tMemoryBlockHeader* stdMemoryTest_GetBlockHeader(void* pData)
{
    return (tMemoryBlockHeader*)((uint8_t*)pData - sizeof(tMemoryBlockHeader));
}

TEST_SETUP(stdMemory)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdMemory)
{
    StdGeneralTest_Shutdown();
}

TEST(stdMemory, TestStartupOpenCloseShutdownLifecycle)
{
    TEST_ASSERT_EQUAL_INT(1, stdMemory_Startup());
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);

    TEST_ASSERT_EQUAL_INT(1, stdMemory_Open());
    TEST_ASSERT_EQUAL_INT(0, stdMemory_Open());

    stdMemory_Close();
    stdMemory_Close();

    stdMemory_Shutdown();
}

TEST(stdMemory, TestMallocFreePatternsAndAccounting)
{
    uint8_t* pData = (uint8_t*)stdMemory_Malloc(8u, "stdMemoryTest.c", 12u);

    TEST_ASSERT_NOT_NULL(pData);
    TEST_ASSERT_EACH_EQUAL_UINT8(0xCCu, pData, 8u);
    TEST_ASSERT_EQUAL_size_t(8u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(1u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(8u, stdMemory_g_curState.maxBytes);

    memset(pData, 0x5Au, 8u);
    stdMemory_Free(pData);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
}

TEST(stdMemory, TestReallocGrowShrinkMallocAndFreeBehavior)
{
    uint8_t* pData = (uint8_t*)stdMemory_Realloc(NULL, 4u, "stdMemoryTest.c", 30u);

    TEST_ASSERT_NOT_NULL(pData);
    pData[0] = 1u;
    pData[1] = 2u;
    pData[2] = 3u;
    pData[3] = 4u;

    pData = (uint8_t*)stdMemory_Realloc(pData, 12u, "stdMemoryTest.c", 37u);
    TEST_ASSERT_NOT_NULL(pData);
    TEST_ASSERT_EQUAL_UINT8(1u, pData[0]);
    TEST_ASSERT_EQUAL_UINT8(4u, pData[3]);
    TEST_ASSERT_EQUAL_size_t(12u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(1u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(12u, stdMemory_g_curState.maxBytes);

    pData = (uint8_t*)stdMemory_Realloc(pData, 2u, "stdMemoryTest.c", 45u);
    TEST_ASSERT_NOT_NULL(pData);
    TEST_ASSERT_EQUAL_UINT8(1u, pData[0]);
    TEST_ASSERT_EQUAL_UINT8(2u, pData[1]);
    TEST_ASSERT_EQUAL_size_t(2u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(1u, stdMemory_g_curState.totalAllocs);

    TEST_ASSERT_NULL(stdMemory_Realloc(pData, 0u, "stdMemoryTest.c", 52u));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
}

TEST(stdMemory, TestAllocationFailureAndOverflowRejections)
{
    uint8_t* pData;
    uint8_t* pResult;

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_NULL(stdMemory_Malloc(8u, "stdMemoryTest.c", 61u));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    StdGeneralTest_ClearAllocationFailures();

    TEST_ASSERT_NULL(stdMemory_Malloc(SIZE_MAX, "stdMemoryTest.c", 66u));

    pData = (uint8_t*)stdMemory_Malloc(8u, "stdMemoryTest.c", 68u);
    TEST_ASSERT_NOT_NULL(pData);
    memset(pData, 0x5Au, 8u);

    StdGeneralTest_FailNextAllocations(0);
    pResult = (uint8_t*)stdMemory_Realloc(pData, 16u, "stdMemoryTest.c", 72u);
    StdGeneralTest_ClearAllocationFailures();
    TEST_ASSERT_NULL(pResult);
    TEST_ASSERT_EACH_EQUAL_UINT8(0x5Au, pData, 8u);
    TEST_ASSERT_EQUAL_size_t(8u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(1u, stdMemory_g_curState.totalAllocs);

    TEST_ASSERT_NULL(stdMemory_Realloc(pData, SIZE_MAX, "stdMemoryTest.c", 70u));
    TEST_ASSERT_EQUAL_size_t(8u, stdMemory_g_curState.totalBytes);
    stdMemory_Free(pData);
}

TEST(stdMemory, TestCloseReportsUnfreedAllocations)
{
    void* pData;

    TEST_ASSERT_EQUAL_INT(1, stdMemory_Startup());
    TEST_ASSERT_EQUAL_INT(1, stdMemory_Open());

    pData = stdMemory_Malloc(24u, "stdMemoryLeakTest.c", 91u);
    TEST_ASSERT_NOT_NULL(pData);

    StdGeneralTest_ResetDebugOutput();
    stdMemory_Close();
    TEST_ASSERT_EQUAL_STRING(
        "stdMemory.c(149): Maximum dynamic memory used: 24 bytes.\n"
        "stdMemory.c(153): UNFREED MEMORY ALERT!\n\n"
        "File\tLine\tSize\tNumber\n\nstdMemoryLeakTest.c\t91\t24\t0\n",
        StdGeneralTest_GetDebugOutput()
    );

    stdMemory_Free(pData);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);

    stdMemory_Shutdown();
}

#ifdef J3D_RUNTIME_GUARDS
TEST(stdMemory, TestFreeNullRuntimeGuard)
{
    J3DTest_ResetAssertCapture();
    stdMemory_Free(NULL);
    TEST_ASSERT_EQUAL_INT(0, J3DTest_GetAssertCount());
}
#endif

TEST(stdMemory, TestBlockAllocatorSmallLargeReallocAndZeroSize)
{
    uint8_t* pSmall = (uint8_t*)stdMemory_BlockMalloc(16u);
    uint8_t* pLarge = (uint8_t*)stdMemory_BlockMalloc(5000u);

    TEST_ASSERT_NOT_NULL(pSmall);
    TEST_ASSERT_NOT_NULL(pLarge);

    for ( size_t i = 0; i < 16u; ++i )
    {
        pSmall[i] = (uint8_t)i;
    }

    pSmall = (uint8_t*)stdMemory_BlockRealloc(pSmall, 32u);
    TEST_ASSERT_NOT_NULL(pSmall);
    for ( size_t i = 0; i < 16u; ++i )
    {
        TEST_ASSERT_EQUAL_UINT8((uint8_t)i, pSmall[i]);
    }

    pLarge = (uint8_t*)stdMemory_BlockRealloc(pLarge, 6000u);
    TEST_ASSERT_NOT_NULL(pLarge);

    stdMemory_BlockFree(pSmall);
    stdMemory_BlockFree(pLarge);

    TEST_ASSERT_NULL(stdMemory_BlockMalloc(0u));
    TEST_ASSERT_NULL(stdMemory_BlockAlloc(0u));
    TEST_ASSERT_NULL(stdMemory_BlockAlloc(SIZE_MAX));
    TEST_ASSERT_NULL(stdMemory_BlockMalloc(SIZE_MAX));

    pSmall = (uint8_t*)stdMemory_BlockRealloc(NULL, 12u);
    TEST_ASSERT_NOT_NULL(pSmall);
    TEST_ASSERT_NULL(stdMemory_BlockRealloc(pSmall, 0u));
}

TEST(stdMemory, TestDirectBlockAllocAndReallocFailurePreservesOriginalBlock)
{
    uint8_t* pData = (uint8_t*)stdMemory_BlockAlloc(5000u);
    uint8_t* pReallocResult;

    TEST_ASSERT_NOT_NULL(pData);
    pData[0] = 0x12u;
    pData[4999] = 0x34u;

    pReallocResult = (uint8_t*)stdMemory_BlockRealloc(pData, SIZE_MAX);
    TEST_ASSERT_NULL(pReallocResult);
    TEST_ASSERT_EQUAL_UINT8(0x12u, pData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34u, pData[4999]);

    stdMemory_BlockFree(pData);
}

TEST(stdMemory, TestBlockAllocatorReusesFreedFragmentsAndWarnsOnDoubleFree)
{
    uint8_t* pA = (uint8_t*)stdMemory_BlockMalloc(16u);
    uint8_t* pB = (uint8_t*)stdMemory_BlockMalloc(16u);
    uint8_t* pC = (uint8_t*)stdMemory_BlockMalloc(16u);
    uint8_t* pD;

    TEST_ASSERT_NOT_NULL(pA);
    TEST_ASSERT_NOT_NULL(pB);
    TEST_ASSERT_NOT_NULL(pC);

    pA[0] = 0xA1u;
    pB[0] = 0xB2u;
    pC[0] = 0xC3u;

    stdMemory_BlockFree(pB);

    StdGeneralTest_ResetDebugOutput();
    stdMemory_BlockFree(pB);
    TEST_ASSERT_EQUAL_STRING("Attempting to dispose a bogus or already-disposed-of block!\n", StdGeneralTest_GetDebugOutput());

    pD = (uint8_t*)stdMemory_BlockMalloc(8u);
    TEST_ASSERT_EQUAL_PTR(pB, pD);
    pD[0] = 0xD4u;

    stdMemory_BlockFree(pA);
    stdMemory_BlockFree(pD);
    stdMemory_BlockFree(pC);
}

TEST(stdMemory, TestBlockAllocatorAlignsOddSizesAndUsesThresholdBoundary)
{
    static const size_t aOddSizes[] = { 1u, 3u, 5u, 31u };
    void* apOddBlocks[STD_ARRAYLEN(aOddSizes)];

    for ( size_t i = 0; i < STD_ARRAYLEN(aOddSizes); ++i )
    {
        apOddBlocks[i] = stdMemory_BlockMalloc(aOddSizes[i]);
        TEST_ASSERT_NOT_NULL(apOddBlocks[i]);
        TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)((uintptr_t)apOddBlocks[i] & 3u));
        memset(apOddBlocks[i], (int)(0x20u + i), aOddSizes[i]);
    }

    void* pZoneBlock = stdMemory_BlockMalloc(4096u);
    void* pHeapBlock = stdMemory_BlockMalloc(4097u);
    TEST_ASSERT_NOT_NULL(pZoneBlock);
    TEST_ASSERT_NOT_NULL(pHeapBlock);
    TEST_ASSERT_NOT_NULL(stdMemoryTest_GetBlockHeader(pZoneBlock)->pBlock);
    TEST_ASSERT_NULL(stdMemoryTest_GetBlockHeader(pHeapBlock)->pBlock);

    for ( size_t i = 0; i < STD_ARRAYLEN(apOddBlocks); ++i )
    {
        stdMemory_BlockFree(apOddBlocks[i]);
    }
    stdMemory_BlockFree(pZoneBlock);
    stdMemory_BlockFree(pHeapBlock);
}

TEST(stdMemory, TestBlockAllocatorUsesBestAndExactFits)
{
    uint8_t* pLarge = (uint8_t*)stdMemory_BlockMalloc(64u);
    void* pSeparatorA = stdMemory_BlockMalloc(16u);
    uint8_t* pSmall = (uint8_t*)stdMemory_BlockMalloc(32u);
    void* pSeparatorB = stdMemory_BlockMalloc(16u);

    TEST_ASSERT_NOT_NULL(pLarge);
    TEST_ASSERT_NOT_NULL(pSeparatorA);
    TEST_ASSERT_NOT_NULL(pSmall);
    TEST_ASSERT_NOT_NULL(pSeparatorB);

    stdMemory_BlockFree(pLarge);
    stdMemory_BlockFree(pSmall);

    // The 32-byte fragment is the closest fit for a 24-byte request.
    uint8_t* pBestFit = (uint8_t*)stdMemory_BlockMalloc(24u);
    TEST_ASSERT_EQUAL_PTR(pSmall, pBestFit);
    stdMemory_BlockFree(pBestFit);

    // An equal-size allocation must reuse the exact 64-byte fragment.
    uint8_t* pExactFit = (uint8_t*)stdMemory_BlockMalloc(64u);
    TEST_ASSERT_EQUAL_PTR(pLarge, pExactFit);

    stdMemory_BlockFree(pExactFit);
    stdMemory_BlockFree(pSeparatorA);
    stdMemory_BlockFree(pSeparatorB);
}

TEST(stdMemory, TestBlockAllocatorCoalescesNextPreviousAndBothNeighbors)
{
    uint8_t* pA = (uint8_t*)stdMemory_BlockMalloc(64u);
    uint8_t* pB = (uint8_t*)stdMemory_BlockMalloc(64u);
    void* pSentinel = stdMemory_BlockMalloc(16u);

    TEST_ASSERT_NOT_NULL(pA);
    TEST_ASSERT_NOT_NULL(pB);
    TEST_ASSERT_NOT_NULL(pSentinel);
    stdMemory_BlockFree(pB);
    stdMemory_BlockFree(pA);

    void* pMergedNext = stdMemory_BlockMalloc(120u);
    TEST_ASSERT_EQUAL_PTR(pA, pMergedNext);
    stdMemory_BlockFree(pMergedNext);
    stdMemory_BlockFree(pSentinel);

    pA = (uint8_t*)stdMemory_BlockMalloc(64u);
    pB = (uint8_t*)stdMemory_BlockMalloc(64u);
    pSentinel = stdMemory_BlockMalloc(16u);
    TEST_ASSERT_NOT_NULL(pA);
    TEST_ASSERT_NOT_NULL(pB);
    TEST_ASSERT_NOT_NULL(pSentinel);
    stdMemory_BlockFree(pA);
    stdMemory_BlockFree(pB);

    void* pMergedPrevious = stdMemory_BlockMalloc(120u);
    TEST_ASSERT_EQUAL_PTR(pA, pMergedPrevious);
    stdMemory_BlockFree(pMergedPrevious);
    stdMemory_BlockFree(pSentinel);

    pA = (uint8_t*)stdMemory_BlockMalloc(64u);
    pB = (uint8_t*)stdMemory_BlockMalloc(64u);
    uint8_t* pC = (uint8_t*)stdMemory_BlockMalloc(64u);
    pSentinel = stdMemory_BlockMalloc(16u);
    TEST_ASSERT_NOT_NULL(pA);
    TEST_ASSERT_NOT_NULL(pB);
    TEST_ASSERT_NOT_NULL(pC);
    TEST_ASSERT_NOT_NULL(pSentinel);
    stdMemory_BlockFree(pA);
    stdMemory_BlockFree(pC);
    stdMemory_BlockFree(pB);

    void* pMergedBoth = stdMemory_BlockMalloc(192u);
    TEST_ASSERT_EQUAL_PTR(pA, pMergedBoth);
    stdMemory_BlockFree(pMergedBoth);
    stdMemory_BlockFree(pSentinel);
}

TEST(stdMemory, TestBlockAllocatorCreatesAndReleasesMultipleZones)
{
    void* apBlocks[10];

    for ( size_t i = 0; i < STD_ARRAYLEN(apBlocks); ++i )
    {
        apBlocks[i] = stdMemory_BlockMalloc(4000u);
        TEST_ASSERT_NOT_NULL(apBlocks[i]);
        memset(apBlocks[i], (int)i, 4000u);
    }

    tMemoryBlock* pFirstZone  = stdMemoryTest_GetBlockHeader(apBlocks[0])->pBlock;
    tMemoryBlock* pSecondZone = stdMemoryTest_GetBlockHeader(apBlocks[7])->pBlock;
    TEST_ASSERT_NOT_NULL(pFirstZone);
    TEST_ASSERT_NOT_NULL(pSecondZone);
    TEST_ASSERT_EQUAL_PTR(pFirstZone, stdMemoryTest_GetBlockHeader(apBlocks[6])->pBlock);
    TEST_ASSERT_NOT_EQUAL(pFirstZone, pSecondZone);

    for ( size_t i = 0; i < STD_ARRAYLEN(apBlocks); ++i )
    {
        TEST_ASSERT_EACH_EQUAL_UINT8((uint8_t)i, apBlocks[i], 4000u);
        stdMemory_BlockFree(apBlocks[i]);
    }
}

TEST(stdMemory, TestBlockReallocPreservesDataAcrossZoneAndHeapBoundaries)
{
    uint8_t* pData = (uint8_t*)stdMemory_BlockMalloc(16u);
    TEST_ASSERT_NOT_NULL(pData);
    for ( size_t i = 0; i < 16u; ++i )
    {
        pData[i] = (uint8_t)(0x40u + i);
    }

    pData = (uint8_t*)stdMemory_BlockRealloc(pData, 3001u);
    TEST_ASSERT_NOT_NULL(pData);
    for ( size_t i = 0; i < 16u; ++i )
    {
        TEST_ASSERT_EQUAL_UINT8((uint8_t)(0x40u + i), pData[i]);
    }
    memset(&pData[16], 0x5Au, 3001u - 16u);

    pData = (uint8_t*)stdMemory_BlockRealloc(pData, 5000u);
    TEST_ASSERT_NOT_NULL(pData);
    TEST_ASSERT_NULL(stdMemoryTest_GetBlockHeader(pData)->pBlock);
    TEST_ASSERT_EACH_EQUAL_UINT8(0x5Au, &pData[16], 3001u - 16u);

    pData = (uint8_t*)stdMemory_BlockRealloc(pData, 7u);
    TEST_ASSERT_NOT_NULL(pData);
    TEST_ASSERT_NOT_NULL(stdMemoryTest_GetBlockHeader(pData)->pBlock);
    for ( size_t i = 0; i < 7u; ++i )
    {
        TEST_ASSERT_EQUAL_UINT8((uint8_t)(0x40u + i), pData[i]);
    }

    stdMemory_BlockFree(pData);
}

TEST(stdMemory, TestBlockAllocatorHandlesDeterministicFragmentationStress)
{
    uint8_t* apData[24] = { 0 };
    size_t aSizes[STD_ARRAYLEN(apData)] = { 0 };

    uint32_t randomState = 0x13579BDFu;

    for ( size_t operation = 0; operation < 512u; ++operation )
    {
        randomState = randomState * 1664525u + 1013904223u;
        size_t index = (randomState >> 8u) % STD_ARRAYLEN(apData);

        if ( !apData[index] )
        {
            size_t size = ((randomState >> 16u) % 5200u) + 1u;
            apData[index] = (uint8_t*)stdMemory_BlockMalloc(size);
            TEST_ASSERT_NOT_NULL(apData[index]);
            aSizes[index] = size;
            memset(apData[index], (int)(index + 1u), size);
        }
        else if ( randomState & 1u )
        {
            size_t newSize = ((randomState >> 16u) % 5200u) + 1u;
            size_t preservedSize = aSizes[index] < newSize ? aSizes[index] : newSize;
            uint8_t* pResized = (uint8_t*)stdMemory_BlockRealloc(apData[index], newSize);
            TEST_ASSERT_NOT_NULL(pResized);
            TEST_ASSERT_EACH_EQUAL_UINT8((uint8_t)(index + 1u), pResized, preservedSize);

            apData[index] = pResized;
            aSizes[index] = newSize;
            memset(apData[index], (int)(index + 1u), newSize);
        }
        else
        {
            TEST_ASSERT_EACH_EQUAL_UINT8((uint8_t)(index + 1u), apData[index], aSizes[index]);
            stdMemory_BlockFree(apData[index]);
            apData[index] = NULL;
            aSizes[index] = 0u;
        }
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(apData); ++i )
    {
        if ( apData[i] )
        {
            TEST_ASSERT_EACH_EQUAL_UINT8((uint8_t)(i + 1u), apData[i], aSizes[i]);
            stdMemory_BlockFree(apData[i]);
        }
    }
}

#ifdef J3D_RUNTIME_GUARDS
TEST(stdMemory, TestBlockFreeNullAssertsAndReturnsUnderRuntimeGuard)
{
    J3DTest_ResetAssertCapture();
    stdMemory_BlockFree(NULL);
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
}
#endif

TEST(stdMemory, TestHeapHeadersAccountingOverflowAndFreeCallbackPayload)
{
    // Verify the allocation header, linked-list pointers, accounting fields, and end marker.
    uint8_t* pData = (uint8_t*)stdMemory_Malloc(8u, "header.c", 17u);
    TEST_ASSERT_NOT_NULL(pData);
    tMemoryHeader* pHeader = (tMemoryHeader*)(pData - sizeof(tMemoryHeader));
    TEST_ASSERT_EQUAL_INT(0, pHeader->number);
    TEST_ASSERT_EQUAL_HEX32((uint32_t)(uintptr_t)pHeader, pHeader->id);
    TEST_ASSERT_EQUAL_size_t(8u, pHeader->size);
    TEST_ASSERT_EQUAL_STRING("header.c", pHeader->pFilename);
    TEST_ASSERT_EQUAL_size_t(17u, pHeader->line);
    TEST_ASSERT_EQUAL_PTR(&stdMemory_g_curState.header, pHeader->pPrev);
    TEST_ASSERT_NULL(pHeader->pNext);
    TEST_ASSERT_EQUAL_PTR(pHeader, stdMemory_g_curState.header.pNext);
    TEST_ASSERT_EQUAL_HEX32(0x12345678u, pHeader->magic);
    uint32_t endMarker;
    memcpy(&endMarker, pData + 8u, sizeof(endMarker));
    TEST_ASSERT_EQUAL_HEX32(0x12345678u, endMarker);

    // Accounting overflow must reject allocation and growth while preserving the complete state.
    tMemoryState state = stdMemory_g_curState;
    stdMemory_g_curState.totalBytes = SIZE_MAX;
    tMemoryState fullState = stdMemory_g_curState;
    TEST_ASSERT_NULL(stdMemory_Malloc(1u, "overflow.c", 1u));
    TEST_ASSERT_NULL(stdMemory_Realloc(pData, 9u, "overflow.c", 2u));
    TEST_ASSERT_EQUAL_MEMORY(&fullState, &stdMemory_g_curState, sizeof(fullState));
    stdMemory_g_curState = state;

    // Capture the heap pointer and poisoned payload passed to the host free callback.
    memset(pData, 0x5Au, 8u);
    tHostServices* pHS = StdGeneralTest_GetHostServices();
    stdMemoryTest_pfOriginalFree = pHS->pFree;
    stdMemoryTest_numFreeCalls = 0u;
    pHS->pFree = stdMemoryTest_CaptureFree;
    stdMemory_Free(pData);
    pHS->pFree = stdMemoryTest_pfOriginalFree;
    TEST_ASSERT_EQUAL_size_t(1u, stdMemoryTest_numFreeCalls);
    TEST_ASSERT_EQUAL_PTR(pHeader, stdMemoryTest_pFreedHeap);
    TEST_ASSERT_EACH_EQUAL_UINT8(0xDDu, stdMemoryTest_aFreedPayload, sizeof(stdMemoryTest_aFreedPayload));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(8u, stdMemory_g_curState.maxBytes);
    TEST_ASSERT_NULL(stdMemory_g_curState.header.pNext);
}

TEST_GROUP_RUNNER(stdMemory)
{
    RUN_TEST_CASE(stdMemory, TestHeapHeadersAccountingOverflowAndFreeCallbackPayload);
    RUN_TEST_CASE(stdMemory, TestStartupOpenCloseShutdownLifecycle);
    RUN_TEST_CASE(stdMemory, TestMallocFreePatternsAndAccounting);
    RUN_TEST_CASE(stdMemory, TestReallocGrowShrinkMallocAndFreeBehavior);
    RUN_TEST_CASE(stdMemory, TestAllocationFailureAndOverflowRejections);
    RUN_TEST_CASE(stdMemory, TestCloseReportsUnfreedAllocations);
#ifdef J3D_RUNTIME_GUARDS
    RUN_TEST_CASE(stdMemory, TestFreeNullRuntimeGuard);
#endif
    RUN_TEST_CASE(stdMemory, TestBlockAllocatorSmallLargeReallocAndZeroSize);
    RUN_TEST_CASE(stdMemory, TestDirectBlockAllocAndReallocFailurePreservesOriginalBlock);
    RUN_TEST_CASE(stdMemory, TestBlockAllocatorReusesFreedFragmentsAndWarnsOnDoubleFree);
    RUN_TEST_CASE(stdMemory, TestBlockAllocatorAlignsOddSizesAndUsesThresholdBoundary);
    RUN_TEST_CASE(stdMemory, TestBlockAllocatorUsesBestAndExactFits);
    RUN_TEST_CASE(stdMemory, TestBlockAllocatorCoalescesNextPreviousAndBothNeighbors);
    RUN_TEST_CASE(stdMemory, TestBlockAllocatorCreatesAndReleasesMultipleZones);
    RUN_TEST_CASE(stdMemory, TestBlockReallocPreservesDataAcrossZoneAndHeapBoundaries);
    RUN_TEST_CASE(stdMemory, TestBlockAllocatorHandlesDeterministicFragmentationStress);
#ifdef J3D_RUNTIME_GUARDS
    RUN_TEST_CASE(stdMemory, TestBlockFreeNullAssertsAndReturnsUnderRuntimeGuard);
#endif
}
