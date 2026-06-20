#include "std.h"
#include "stdLinkList.h"

#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

void stdLinkList_InstallHooks(void)
{
    J3D_HOOKFUNC(stdLinkList_AddNode);
    J3D_HOOKFUNC(stdLinkList_RemoveNode);
}

void stdLinkList_ResetGlobals(void)
{}

void J3DAPI stdLinkList_AddNode(tLinkListNode* pCur, tLinkListNode* pNodeToAdd)
{
    STD_ASSERTREL(pCur != NULL);
    STD_ASSERTREL(pNodeToAdd != NULL);
    STD_ASSERTREL(pNodeToAdd->prev == NULL);
    STD_ASSERTREL(pNodeToAdd->next == NULL);

    tLinkListNode* pNext = pCur->next;
    pNodeToAdd->prev     = pCur;
    pNodeToAdd->next     = pCur->next;
    pCur->next           = pNodeToAdd;
    if ( pNext ) {
        // Fixed: Relink the original successor back to the inserted node.
        pNext->prev = pNodeToAdd;
    }
}

void J3DAPI stdLinklist_InsertNode(tLinkListNode* pCur, tLinkListNode* pNodeToInsert)
{
    STD_ASSERTREL(pCur != NULL);
    STD_ASSERTREL(pNodeToInsert != NULL);
    STD_ASSERTREL(pNodeToInsert->prev == NULL);
    STD_ASSERTREL(pNodeToInsert->next == NULL);

    tLinkListNode* pPrev = pCur->prev;
    pNodeToInsert->next  = pCur;
    pNodeToInsert->prev  = pPrev;
    pCur->prev           = pNodeToInsert;

    if ( pPrev ) {
        pPrev->next = pNodeToInsert;
    }
}

void J3DAPI stdLinklist_AppendNode(tLinkListNode* pCur, tLinkListNode* pNodeToAdd)
{
    STD_ASSERTREL(pCur != NULL);
    STD_ASSERTREL(pNodeToAdd != NULL);
    STD_ASSERTREL(pNodeToAdd->prev == NULL);
    STD_ASSERTREL(pNodeToAdd->next == NULL);

    tLinkListNode* pPrev;
    for ( pPrev = pCur; pPrev->next; pPrev = pPrev->next ) {
        ;
    }

    pNodeToAdd->prev = pPrev;
    pNodeToAdd->next = NULL;
    pPrev->next = pNodeToAdd;
}

void J3DAPI stdLinkList_RemoveNode(tLinkListNode* pCur)
{
    STD_ASSERTREL(pCur != NULL);
    if ( pCur->prev ) {
        pCur->prev->next = pCur->next;
    }

    if ( pCur->next ) {
        pCur->next->prev = pCur->prev;
    }

    pCur->next = NULL;
    pCur->prev = NULL;
}

void J3DAPI stdLinklist_NewList(tLinkListNode* pFirstOfNewList)
{
    STD_ASSERTREL(pFirstOfNewList != NULL);
    if ( pFirstOfNewList->prev ) {
        pFirstOfNewList->prev->next = NULL;
    }

    pFirstOfNewList->prev = NULL;
}

void J3DAPI stdLinklist_DetachNode(tLinkListNode* pNode)
{
    pNode->prev = NULL;
    pNode->next = NULL;
}

size_t J3DAPI stdLinklist_GetCount(const tLinkListNode* pCur)
{
    int count = 0;
    while ( pCur )
    {
        ++count;
        pCur = pCur->next;
    }

    return count;
}

tLinkListNode* J3DAPI stdLinklist_GetNode(const tLinkListNode* pFirstNode, int n)
{
    STD_ASSERTREL(n >= 0);
    STD_ASSERTREL(pFirstNode != NULL);

    while ( pFirstNode && n > 0 )
    {
        --n;
        pFirstNode = pFirstNode->next;
    }

    return (tLinkListNode*)pFirstNode;
}

tLinkListNode* J3DAPI stdLinklist_GetLastNode(const tLinkListNode* pCur)
{
    if ( !pCur ) {
        return NULL;
    }

    while ( pCur->next ) {
        pCur = pCur->next;
    }

    return (tLinkListNode*)pCur;
}

tLinkListNode* J3DAPI stdLinklist_GetFirstNode(const tLinkListNode* pCur)
{
    if ( !pCur ) {
        return NULL;
    }

    while ( pCur->prev ) {
        pCur = pCur->prev;
    }

    return (tLinkListNode*)pCur;
}
