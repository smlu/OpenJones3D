#include <unity_fixture.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/stdLinkList.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

TEST_GROUP(stdLinkList);

TEST_SETUP(stdLinkList)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdLinkList)
{
    StdGeneralTest_Shutdown();
}

static void stdLinkListTest_InitNode(tLinkListNode* pNode, const char* pName)
{
    STD_ZEROMEM(pNode, sizeof(*pNode));
    pNode->name = pName;
}

TEST(stdLinkList, TestAddNodeRelinksOriginalSuccessorBugFix)
{
    tLinkListNode head;
    tLinkListNode middle;
    tLinkListNode inserted;

    stdLinkListTest_InitNode(&head, "head");
    stdLinkListTest_InitNode(&middle, "middle");
    stdLinkListTest_InitNode(&inserted, "inserted");

    stdLinkList_AddNode(&head, &middle);
    stdLinkList_AddNode(&head, &inserted);

    // Regression: inserting between two nodes must update the original successor's prev link.
    TEST_ASSERT_NULL(head.prev);
    TEST_ASSERT_EQUAL_PTR(&inserted, head.next);
    TEST_ASSERT_EQUAL_PTR(&head, inserted.prev);
    TEST_ASSERT_EQUAL_PTR(&middle, inserted.next);
    TEST_ASSERT_EQUAL_PTR(&inserted, middle.prev);
    TEST_ASSERT_NULL(middle.next);
}

TEST(stdLinkList, TestInsertNodeBeforeCurrent)
{
    tLinkListNode head;
    tLinkListNode tail;
    tLinkListNode inserted;

    stdLinkListTest_InitNode(&head, "head");
    stdLinkListTest_InitNode(&tail, "tail");
    stdLinkListTest_InitNode(&inserted, "inserted");

    stdLinkList_AddNode(&head, &tail);
    stdLinklist_InsertNode(&tail, &inserted);

    TEST_ASSERT_EQUAL_PTR(&inserted, head.next);
    TEST_ASSERT_EQUAL_PTR(&head, inserted.prev);
    TEST_ASSERT_EQUAL_PTR(&tail, inserted.next);
    TEST_ASSERT_EQUAL_PTR(&inserted, tail.prev);
}

TEST(stdLinkList, TestAppendNode)
{
    tLinkListNode head;
    tLinkListNode second;
    tLinkListNode third;

    stdLinkListTest_InitNode(&head, "head");
    stdLinkListTest_InitNode(&second, "second");
    stdLinkListTest_InitNode(&third, "third");

    stdLinklist_AppendNode(&head, &second);
    stdLinklist_AppendNode(&head, &third);

    TEST_ASSERT_EQUAL_PTR(&second, head.next);
    TEST_ASSERT_EQUAL_PTR(&third, second.next);
    TEST_ASSERT_EQUAL_PTR(&second, third.prev);
    TEST_ASSERT_NULL(third.next);
}

TEST(stdLinkList, TestRemoveNode)
{
    tLinkListNode head;
    tLinkListNode middle;
    tLinkListNode tail;

    stdLinkListTest_InitNode(&head, "head");
    stdLinkListTest_InitNode(&middle, "middle");
    stdLinkListTest_InitNode(&tail, "tail");

    stdLinklist_AppendNode(&head, &middle);
    stdLinklist_AppendNode(&head, &tail);

    stdLinkList_RemoveNode(&middle);
    TEST_ASSERT_EQUAL_PTR(&tail, head.next);
    TEST_ASSERT_EQUAL_PTR(&head, tail.prev);
    TEST_ASSERT_NULL(middle.prev);
    TEST_ASSERT_NULL(middle.next);

    stdLinkList_RemoveNode(&head);
    TEST_ASSERT_NULL(head.prev);
    TEST_ASSERT_NULL(head.next);
    TEST_ASSERT_NULL(tail.prev);
}

TEST(stdLinkList, TestNewListAndDetachNode)
{
    tLinkListNode head;
    tLinkListNode middle;
    tLinkListNode tail;

    stdLinkListTest_InitNode(&head, "head");
    stdLinkListTest_InitNode(&middle, "middle");
    stdLinkListTest_InitNode(&tail, "tail");

    stdLinklist_AppendNode(&head, &middle);
    stdLinklist_AppendNode(&head, &tail);

    stdLinklist_NewList(&middle);
    TEST_ASSERT_NULL(head.next);
    TEST_ASSERT_NULL(middle.prev);
    TEST_ASSERT_EQUAL_PTR(&tail, middle.next);
    TEST_ASSERT_EQUAL_PTR(&middle, tail.prev);

    stdLinklist_DetachNode(&tail);
    TEST_ASSERT_NULL(tail.prev);
    TEST_ASSERT_NULL(tail.next);
}

TEST(stdLinkList, TestGetHelpers)
{
    tLinkListNode head;
    tLinkListNode middle;
    tLinkListNode tail;

    stdLinkListTest_InitNode(&head, "head");
    stdLinkListTest_InitNode(&middle, "middle");
    stdLinkListTest_InitNode(&tail, "tail");

    stdLinklist_AppendNode(&head, &middle);
    stdLinklist_AppendNode(&head, &tail);

    TEST_ASSERT_EQUAL_size_t(3u, stdLinklist_GetCount(&head));
    TEST_ASSERT_EQUAL_size_t(0u, stdLinklist_GetCount(NULL));
    TEST_ASSERT_EQUAL_PTR(&head, stdLinklist_GetNode(&head, 0));
    TEST_ASSERT_EQUAL_PTR(&middle, stdLinklist_GetNode(&head, 1));
    TEST_ASSERT_EQUAL_PTR(&tail, stdLinklist_GetNode(&head, 2));
    TEST_ASSERT_NULL(stdLinklist_GetNode(&head, 3));
    TEST_ASSERT_EQUAL_PTR(&tail, stdLinklist_GetLastNode(&head));
    TEST_ASSERT_EQUAL_PTR(&head, stdLinklist_GetFirstNode(&tail));
    TEST_ASSERT_NULL(stdLinklist_GetLastNode(NULL));
    TEST_ASSERT_NULL(stdLinklist_GetFirstNode(NULL));
}

TEST(stdLinkList, TestSafeAssertOnlyCases)
{
    tLinkListNode head;

    stdLinkListTest_InitNode(&head, "head");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_PTR(&head, stdLinklist_GetNode(&head, -1));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("n >= 0", "stdLinkList.c");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_NULL(stdLinklist_GetNode(NULL, 0));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    StdGeneralTest_AssertLastAssert("pFirstNode != NULL", "stdLinkList.c");
}

TEST_GROUP_RUNNER(stdLinkList)
{
    RUN_TEST_CASE(stdLinkList, TestAddNodeRelinksOriginalSuccessorBugFix);
    RUN_TEST_CASE(stdLinkList, TestInsertNodeBeforeCurrent);
    RUN_TEST_CASE(stdLinkList, TestAppendNode);
    RUN_TEST_CASE(stdLinkList, TestRemoveNode);
    RUN_TEST_CASE(stdLinkList, TestNewListAndDetachNode);
    RUN_TEST_CASE(stdLinkList, TestGetHelpers);
    RUN_TEST_CASE(stdLinkList, TestSafeAssertOnlyCases);
}
