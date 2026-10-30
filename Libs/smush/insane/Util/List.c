// List.c: INSANE's doubly linked list (original 0x004F03E0-0x004F0460).
#include <smush/insane/Util/List.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>
#include <stddef.h>

void J3DAPI LecList_Init(LecList* pList)
{
    pList->tail.pPrev = &pList->head;
    pList->head.pPrev = NULL;
    pList->head.pNext = &pList->tail;
    pList->tail.pNext = NULL;
}

void J3DAPI LecList_Insert(LecNode* pNode, LecNode* pRef, int before)
{
    if ( before )
    {
        pNode->pNext = pRef;
        pNode->pPrev = pRef->pPrev;
        if ( pRef->pPrev )
        {
            pRef->pPrev->pNext = pNode;
        }

        pRef->pPrev = pNode;
    }
    else
    {
        pNode->pPrev = pRef;
        pNode->pNext = pRef->pNext;
        if ( pRef->pNext )
        {
            pRef->pNext->pPrev = pNode;
        }

        pRef->pNext = pNode;
    }
}

void J3DAPI LecList_Remove(LecNode* pNode)
{
    if ( pNode->pPrev )
    {
        pNode->pPrev->pNext = pNode->pNext;
    }

    if ( pNode->pNext )
    {
        pNode->pNext->pPrev = pNode->pPrev;
    }
}

void List_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(LecList_Init);
    J3D_HOOKFUNC(LecList_Insert);
    J3D_HOOKFUNC(LecList_Remove);
}

void List_ResetGlobals(void)
{}
