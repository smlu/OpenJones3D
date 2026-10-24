#include <unity_fixture.h>

#include <string.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdLinkList.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

static unsigned int J3DAPI stdHashtblTest_ConstantHash(const char* pName, signed int hashSize)
{
    J3D_UNUSED(pName);
    return hashSize > 0 ? 0u : 0u;
}

TEST_GROUP(stdHashtbl);

TEST_SETUP(stdHashtbl)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdHashtbl)
{
    StdGeneralTest_Shutdown();
}

TEST(stdHashtbl, TestNextPrime)
{
    TEST_ASSERT_EQUAL_size_t(2u, stdHashtbl_nextPrime(0u));
    TEST_ASSERT_EQUAL_size_t(2u, stdHashtbl_nextPrime(2u));
    TEST_ASSERT_EQUAL_size_t(3u, stdHashtbl_nextPrime(3u));
    TEST_ASSERT_EQUAL_size_t(5u, stdHashtbl_nextPrime(4u));
    TEST_ASSERT_EQUAL_size_t(2003u, stdHashtbl_nextPrime(2000u));
}

TEST(stdHashtbl, TestCalculateHashVectorsAndGuards)
{
    char aHighByte[2] = { 0 };
    tHashTable* pTable = stdHashtbl_New(1u);

    TEST_ASSERT_NOT_NULL(pTable);
    memset(aHighByte, 0xFF, 1u);

    TEST_ASSERT_EQUAL_UINT(0u, pTable->pfHashFunc("", 23));
    TEST_ASSERT_EQUAL_UINT(5u, pTable->pfHashFunc("a", 23));
    TEST_ASSERT_EQUAL_UINT(10u, pTable->pfHashFunc("abc", 23));
    TEST_ASSERT_EQUAL_UINT(15u, pTable->pfHashFunc("Indiana Jones", 53));
    TEST_ASSERT_EQUAL_UINT(18u, pTable->pfHashFunc(aHighByte, 79));
    TEST_ASSERT_EQUAL_UINT(860u, pTable->pfHashFunc("generated/file07.bin", 1024));

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_UINT(0u, pTable->pfHashFunc(NULL, 23));
    TEST_ASSERT_EQUAL_UINT(0u, pTable->pfHashFunc("abc", 0));
    TEST_ASSERT_EQUAL_UINT(0u, pTable->pfHashFunc("abc", -1));
#endif

    stdHashtbl_Free(pTable);
}

TEST(stdHashtbl, TestNewRoundsToPrimeAndInitializes)
{
    tHashTable* pTable = stdHashtbl_New(23u);

    TEST_ASSERT_NOT_NULL(pTable);
    TEST_ASSERT_EQUAL_size_t(53u, pTable->numNodes);
    TEST_ASSERT_NOT_NULL(pTable->paNodes);
    TEST_ASSERT_NOT_NULL(pTable->pfHashFunc);

    for ( size_t i = 0; i < pTable->numNodes; ++i )
    {
        TEST_ASSERT_NULL(pTable->paNodes[i].name);
        TEST_ASSERT_NULL(pTable->paNodes[i].data);
        TEST_ASSERT_NULL(pTable->paNodes[i].next);
        TEST_ASSERT_NULL(pTable->paNodes[i].prev);
    }

    stdHashtbl_Free(pTable);

    pTable = stdHashtbl_New(1999u);
    TEST_ASSERT_NOT_NULL(pTable);
    TEST_ASSERT_EQUAL_size_t(1999u, pTable->numNodes);
    stdHashtbl_Free(pTable);

    pTable = stdHashtbl_New(2000u);
    TEST_ASSERT_NOT_NULL(pTable);
    TEST_ASSERT_EQUAL_size_t(2003u, pTable->numNodes);
    stdHashtbl_Free(pTable);
}

TEST(stdHashtbl, TestAddFindDuplicateAndRemoveSingleBucket)
{
    int data = 42;
    int nodeIdx = -1;
    tLinkListNode* pNode;
    tHashTable* pTable = stdHashtbl_New(1u);

    TEST_ASSERT_NOT_NULL(pTable);

    TEST_ASSERT_EQUAL_INT(1, stdHashtbl_Add(pTable, "indy", &data));
    TEST_ASSERT_EQUAL_PTR(&data, stdHashtbl_Find(pTable, "indy"));
    TEST_ASSERT_EQUAL_INT(0, stdHashtbl_Add(pTable, "indy", &data));

    pNode = stdHashtbl_FindNode(pTable, "indy", &nodeIdx);
    TEST_ASSERT_NOT_NULL(pNode);
    TEST_ASSERT_EQUAL_INT((int)pTable->pfHashFunc("indy", (int)pTable->numNodes), nodeIdx);
    TEST_ASSERT_EQUAL_PTR(&data, pNode->data);
    TEST_ASSERT_EQUAL_PTR(&pTable->paNodes[nodeIdx], pNode);
    TEST_ASSERT_EQUAL_STRING("indy", pNode->name);
    TEST_ASSERT_EQUAL_PTR(pNode, stdHashtbl_GetTailNode(&pTable->paNodes[nodeIdx]));

    TEST_ASSERT_EQUAL_INT(1, stdHashtbl_Remove(pTable, "indy"));
    TEST_ASSERT_NULL(stdHashtbl_Find(pTable, "indy"));
    TEST_ASSERT_EQUAL_INT(0, stdHashtbl_Remove(pTable, "indy"));

    stdHashtbl_Free(pTable);
}

TEST(stdHashtbl, TestCollisionChainFindAndRemove)
{
    int first = 1;
    int second = 2;
    int third = 3;
    tHashTable* pTable = stdHashtbl_New(1u);

    TEST_ASSERT_NOT_NULL(pTable);
    pTable->pfHashFunc = stdHashtblTest_ConstantHash;

    TEST_ASSERT_EQUAL_INT(1, stdHashtbl_Add(pTable, "first", &first));
    TEST_ASSERT_EQUAL_INT(1, stdHashtbl_Add(pTable, "second", &second));
    TEST_ASSERT_EQUAL_INT(1, stdHashtbl_Add(pTable, "third", &third));

    TEST_ASSERT_EQUAL_size_t(3u, stdLinklist_GetCount(&pTable->paNodes[0]));
    TEST_ASSERT_EQUAL_PTR(&first, stdHashtbl_Find(pTable, "first"));
    TEST_ASSERT_EQUAL_PTR(&second, stdHashtbl_Find(pTable, "second"));
    TEST_ASSERT_EQUAL_PTR(&third, stdHashtbl_Find(pTable, "third"));

    TEST_ASSERT_EQUAL_INT(0, stdHashtbl_Remove(pTable, "missing"));

    TEST_ASSERT_EQUAL_INT(1, stdHashtbl_Remove(pTable, "second"));
    TEST_ASSERT_NULL(stdHashtbl_Find(pTable, "second"));
    TEST_ASSERT_EQUAL_size_t(2u, stdLinklist_GetCount(&pTable->paNodes[0]));
    TEST_ASSERT_EQUAL_PTR(&first, stdHashtbl_Find(pTable, "first"));
    TEST_ASSERT_EQUAL_PTR(&third, stdHashtbl_Find(pTable, "third"));

    TEST_ASSERT_EQUAL_INT(1, stdHashtbl_Remove(pTable, "first"));
    TEST_ASSERT_NULL(stdHashtbl_Find(pTable, "first"));
    TEST_ASSERT_EQUAL_PTR(&third, stdHashtbl_Find(pTable, "third"));
    TEST_ASSERT_EQUAL_STRING("third", pTable->paNodes[0].name);
    TEST_ASSERT_NULL(pTable->paNodes[0].prev);

    TEST_ASSERT_EQUAL_INT(1, stdHashtbl_Remove(pTable, "third"));
    TEST_ASSERT_NULL(stdHashtbl_Find(pTable, "third"));
    TEST_ASSERT_NULL(pTable->paNodes[0].name);

    stdHashtbl_Free(pTable);
}

TEST(stdHashtbl, TestFindNodeRejectsInvalidInputs)
{
    int data = 7;
    int nodeIdx = 99;
    tHashTable emptyTable;
    tHashTable* pTable = stdHashtbl_New(1u);

    TEST_ASSERT_NULL(stdHashtbl_FindNode(NULL, "name", &nodeIdx));

    STD_ZEROMEM(&emptyTable, sizeof(emptyTable));
#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_NULL(stdHashtbl_FindNode(&emptyTable, "name", &nodeIdx));
#endif

    TEST_ASSERT_NOT_NULL(pTable);
    TEST_ASSERT_EQUAL_INT(1, stdHashtbl_Add(pTable, "name", &data));

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_NULL(stdHashtbl_FindNode(pTable, NULL, &nodeIdx));
    TEST_ASSERT_NULL(stdHashtbl_FindNode(pTable, "name", NULL));
#endif

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdHashtbl_Remove(NULL, "name"));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());

    stdHashtbl_Free(pTable);
}

TEST(stdHashtbl, TestAllocationFailures)
{
    int first = 1;
    int second = 2;
    tHashTable* pTable;

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_NULL(stdHashtbl_New(1u));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    StdGeneralTest_ClearAllocationFailures();

    StdGeneralTest_FailNextAllocations(1);
    TEST_ASSERT_NULL(stdHashtbl_New(1u));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    StdGeneralTest_ClearAllocationFailures();

    pTable = stdHashtbl_New(1u);
    TEST_ASSERT_NOT_NULL(pTable);
    pTable->pfHashFunc = stdHashtblTest_ConstantHash;

    TEST_ASSERT_EQUAL_INT(1, stdHashtbl_Add(pTable, "first", &first));
    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_EQUAL_INT(0, stdHashtbl_Add(pTable, "second", &second));
    TEST_ASSERT_NULL(stdHashtbl_Find(pTable, "second"));
    StdGeneralTest_ClearAllocationFailures();

    stdHashtbl_Free(pTable);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
}

TEST(stdHashtbl, TestDiagnosticsAndDumpTable)
{
    tLinkListNode nodes[3];
    tHashTable table;

    STD_ZEROMEM(nodes, sizeof(nodes));
    STD_ZEROMEM(&table, sizeof(table));

#ifdef J3D_RUNTIME_GUARDS
    stdHashtbl_PrintTableDiagnostics(&table);
    TEST_ASSERT_EQUAL_STRING("", StdGeneralTest_GetDebugOutput());
#endif

    StdGeneralTest_ResetDebugOutput();
    stdHashtbl_DumpTable(&table);
    TEST_ASSERT_EQUAL_STRING("\nHASHTABLE\n---------\n", StdGeneralTest_GetDebugOutput());

    nodes[0].name = "only";
    table.numNodes = 1;
    table.paNodes = &nodes[0];
    table.pfHashFunc = stdHashtblTest_ConstantHash;

    StdGeneralTest_ResetDebugOutput();
    stdHashtbl_PrintTableDiagnostics(&table);
    TEST_ASSERT_EQUAL_STRING(
        "\nHASHTABLE Diagnostics\n---------------------\n"
        " Maximum Lookups = 1\n Filled Indices = 1/1 (100.00%)\n"
        " Average Lookup = 1.00\n Weighted Lookup = 1.00\n---------------------\n",
        StdGeneralTest_GetDebugOutput()
    );

    StdGeneralTest_ResetDebugOutput();
    stdHashtbl_DumpTable(&table);
    TEST_ASSERT_EQUAL_STRING("\nHASHTABLE\n---------\nIndex: 0\tStrings: 'only'\n", StdGeneralTest_GetDebugOutput());

    nodes[0].name = "first";
    nodes[0].next = &nodes[1];
    nodes[1].name = "second";
    nodes[1].prev = &nodes[0];
    nodes[1].next = &nodes[2];
    nodes[2].name = "third";
    nodes[2].prev = &nodes[1];

    StdGeneralTest_ResetDebugOutput();
    stdHashtbl_PrintTableDiagnostics(&table);
    TEST_ASSERT_EQUAL_STRING(
        "\nHASHTABLE Diagnostics\n---------------------\n"
        " Maximum Lookups = 3\n Filled Indices = 1/1 (100.00%)\n"
        " Average Lookup = 3.00\n Weighted Lookup = 3.00\n---------------------\n",
        StdGeneralTest_GetDebugOutput()
    );

    StdGeneralTest_ResetDebugOutput();
    stdHashtbl_DumpTable(&table);
    TEST_ASSERT_EQUAL_STRING(
        "\nHASHTABLE\n---------\nIndex: 0\tStrings: 'first' 'second' 'third'\n",
        StdGeneralTest_GetDebugOutput()
    );
}

TEST(stdHashtbl, TestEmptyBucketsAndMissingNodeIndex)
{
    tHashTable* pTable = stdHashtbl_New(1u);
    TEST_ASSERT_NOT_NULL(pTable);
    pTable->pfHashFunc = stdHashtblTest_ConstantHash;

    // An unsuccessful lookup must still return the bucket index supplied by the hash callback.
    int nodeIdx = -1;
    TEST_ASSERT_NULL(stdHashtbl_FindNode(pTable, "missing", &nodeIdx));
    TEST_ASSERT_EQUAL_INT(0, nodeIdx);

    // An allocated table with empty buckets must produce the complete zero-occupancy diagnostic.
    StdGeneralTest_ResetDebugOutput();
    stdHashtbl_PrintTableDiagnostics(pTable);
    TEST_ASSERT_EQUAL_STRING(
        "\nHASHTABLE Diagnostics\n---------------------\n"
        " Maximum Lookups = 0\n Filled Indices = 0/23 (0.00%)\n"
        " Average Lookup = 0.00\n Weighted Lookup = 0.00\n---------------------\n",
        StdGeneralTest_GetDebugOutput()
    );
    stdHashtbl_Free(pTable);
}

TEST_GROUP_RUNNER(stdHashtbl)
{
    RUN_TEST_CASE(stdHashtbl, TestEmptyBucketsAndMissingNodeIndex);
    RUN_TEST_CASE(stdHashtbl, TestNextPrime);
    RUN_TEST_CASE(stdHashtbl, TestCalculateHashVectorsAndGuards);
    RUN_TEST_CASE(stdHashtbl, TestNewRoundsToPrimeAndInitializes);
    RUN_TEST_CASE(stdHashtbl, TestAddFindDuplicateAndRemoveSingleBucket);
    RUN_TEST_CASE(stdHashtbl, TestCollisionChainFindAndRemove);
    RUN_TEST_CASE(stdHashtbl, TestFindNodeRejectsInvalidInputs);
    RUN_TEST_CASE(stdHashtbl, TestAllocationFailures);
    RUN_TEST_CASE(stdHashtbl, TestDiagnosticsAndDumpTable);
}
