#include <unity_fixture.h>

#include <limits.h>
#include <stdint.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/stdCircBuf.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

TEST_GROUP(stdCircBuf);

TEST_SETUP(stdCircBuf)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdCircBuf)
{
    StdGeneralTest_Shutdown();
}

TEST(stdCircBuf, TestNewInitializesAndZerosStorage)
{
    tCircularBuffer circ;
    uint32_t* pWords;

    TEST_ASSERT_EQUAL_INT(1, stdCircBuf_New(&circ, 3, sizeof(uint32_t)));
    TEST_ASSERT_EQUAL_size_t(3u, circ.numAllocated);
    TEST_ASSERT_EQUAL_size_t(sizeof(uint32_t), circ.elementSize);
    TEST_ASSERT_EQUAL_size_t(0u, circ.iFirst);
    TEST_ASSERT_EQUAL_size_t(0u, circ.numValidElements);
    TEST_ASSERT_NOT_NULL(circ.paElements);

    pWords = (uint32_t*)circ.paElements;
    TEST_ASSERT_EQUAL_UINT32(0u, pWords[0]);
    TEST_ASSERT_EQUAL_UINT32(0u, pWords[1]);
    TEST_ASSERT_EQUAL_UINT32(0u, pWords[2]);

    stdCircBuf_Free(&circ);
}

TEST(stdCircBuf, TestGetNextElementAndWrapPurgesOldest)
{
    tCircularBuffer circ;
    int* pValue;

    TEST_ASSERT_EQUAL_INT(1, stdCircBuf_New(&circ, 3, sizeof(int)));

    pValue = (int*)stdCircBuf_GetNextElement(&circ);
    TEST_ASSERT_NOT_NULL(pValue);
    *pValue = 10;

    pValue = (int*)stdCircBuf_GetNextElement(&circ);
    TEST_ASSERT_NOT_NULL(pValue);
    *pValue = 20;

    pValue = (int*)stdCircBuf_GetNextElement(&circ);
    TEST_ASSERT_NOT_NULL(pValue);
    *pValue = 30;

    TEST_ASSERT_EQUAL_size_t(3u, circ.numValidElements);
    TEST_ASSERT_EQUAL_size_t(0u, circ.iFirst);

    pValue = (int*)stdCircBuf_GetNextElement(&circ);
    TEST_ASSERT_NOT_NULL(pValue);
    TEST_ASSERT_EQUAL_INT(10, *pValue);
    *pValue = 40;

    TEST_ASSERT_EQUAL_size_t(3u, circ.numValidElements);
    TEST_ASSERT_EQUAL_size_t(1u, circ.iFirst);
    TEST_ASSERT_EQUAL_INT(20, *(int*)&circ.paElements[circ.elementSize * circ.iFirst]);

    stdCircBuf_Free(&circ);
}

TEST(stdCircBuf, TestPurge)
{
    tCircularBuffer circ;

    TEST_ASSERT_EQUAL_INT(1, stdCircBuf_New(&circ, 2, sizeof(int)));
    *(int*)stdCircBuf_GetNextElement(&circ) = 1;
    *(int*)stdCircBuf_GetNextElement(&circ) = 2;

    stdCircBuf_Purge(&circ);
    TEST_ASSERT_EQUAL_size_t(1u, circ.numValidElements);
    TEST_ASSERT_EQUAL_size_t(1u, circ.iFirst);

    stdCircBuf_Purge(&circ);
    TEST_ASSERT_EQUAL_size_t(0u, circ.numValidElements);
    TEST_ASSERT_EQUAL_size_t(0u, circ.iFirst);

    stdCircBuf_Purge(&circ);
    TEST_ASSERT_EQUAL_size_t(0u, circ.numValidElements);
    TEST_ASSERT_EQUAL_size_t(0u, circ.iFirst);

    stdCircBuf_Free(&circ);
}

TEST(stdCircBuf, TestFreeAndGetNextOnEmptyBuffer)
{
    tCircularBuffer circ;

    TEST_ASSERT_EQUAL_INT(1, stdCircBuf_New(&circ, 1, sizeof(int)));
    TEST_ASSERT_NOT_NULL(stdCircBuf_GetNextElement(&circ));

    stdCircBuf_Free(&circ);
    TEST_ASSERT_NULL(circ.paElements);
    TEST_ASSERT_EQUAL_size_t(0u, circ.numAllocated);
    TEST_ASSERT_EQUAL_size_t(0u, circ.numValidElements);
    TEST_ASSERT_NULL(stdCircBuf_GetNextElement(&circ));
}

#ifdef J3D_RUNTIME_GUARDS
TEST(stdCircBuf, TestInvalidNewRejections)
{
    tCircularBuffer circ;

    STD_ZEROMEM(&circ, sizeof(circ));

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdCircBuf_New(&circ, 0, sizeof(int)));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    TEST_ASSERT_NULL(circ.paElements);

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdCircBuf_New(&circ, 1, 0));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    TEST_ASSERT_NULL(circ.paElements);

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdCircBuf_New(NULL, 1, sizeof(int)));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
}
#endif

TEST(stdCircBuf, TestAllocationOverflowRejection)
{
    tCircularBuffer circ;

    STD_ZEROMEM(&circ, sizeof(circ));
    TEST_ASSERT_EQUAL_INT(0, stdCircBuf_New(&circ, INT_MAX, INT_MAX));
    TEST_ASSERT_NULL(circ.paElements);
    TEST_ASSERT_EQUAL_size_t(0u, circ.numAllocated);
}

TEST(stdCircBuf, TestAllocationFailureRejection)
{
    tCircularBuffer circ;

    STD_ZEROMEM(&circ, sizeof(circ));
    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_EQUAL_INT(0, stdCircBuf_New(&circ, 2, sizeof(int)));
    TEST_ASSERT_NULL(circ.paElements);
    TEST_ASSERT_EQUAL_size_t(0u, circ.numAllocated);
    StdGeneralTest_ClearAllocationFailures();
}

TEST_GROUP_RUNNER(stdCircBuf)
{
    RUN_TEST_CASE(stdCircBuf, TestNewInitializesAndZerosStorage);
    RUN_TEST_CASE(stdCircBuf, TestGetNextElementAndWrapPurgesOldest);
    RUN_TEST_CASE(stdCircBuf, TestPurge);
    RUN_TEST_CASE(stdCircBuf, TestFreeAndGetNextOnEmptyBuffer);
#ifdef J3D_RUNTIME_GUARDS
    RUN_TEST_CASE(stdCircBuf, TestInvalidNewRejections);
#endif
    RUN_TEST_CASE(stdCircBuf, TestAllocationOverflowRejection);
    RUN_TEST_CASE(stdCircBuf, TestAllocationFailureRejection);
}
