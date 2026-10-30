#ifndef SMUSH_INSANE_UTIL_LIST_H
#define SMUSH_INSANE_UTIL_LIST_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>

/*
 * INSANE's doubly linked list: a list is two linked nodes, a head and a tail sentinel; iteration runs from head.pNext
 * to the tail's address. Names are inferred from the pointer shapes.
 */

J3D_EXTERN_C_START

typedef struct sLecNode LecNode;
struct sLecNode
{
    LecNode* pPrev;
    LecNode* pNext;
};
static_assert(sizeof(LecNode) == 8, "sizeof(LecNode) == 8");

typedef struct sLecList
{
    LecNode head;
    LecNode tail;
} LecList;
static_assert(sizeof(LecList) == 0x10, "sizeof(LecList) == 0x10");

void J3DAPI LecList_Init(LecList* pList);
void J3DAPI LecList_Insert(LecNode* pNode, LecNode* pRef, int before);
void J3DAPI LecList_Remove(LecNode* pNode);

void List_InstallHooks(void);
void List_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_UTIL_LIST_H
