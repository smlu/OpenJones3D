#ifndef STD_STDLINKLIST_H
#define STD_STDLINKLIST_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

void J3DAPI stdLinkList_AddNode(tLinkListNode* pCur, tLinkListNode* pNodeToAdd);
void J3DAPI stdLinklist_InsertNode(tLinkListNode* pCur, tLinkListNode* pNodeToInsert); // Added
void J3DAPI stdLinklist_AppendNode(tLinkListNode* pCur, tLinkListNode* pNodeToAdd); // Added
void J3DAPI stdLinkList_RemoveNode(tLinkListNode* pCur);

void J3DAPI stdLinklist_NewList(tLinkListNode* pFirstOfNewList); // Added
void J3DAPI stdLinklist_DetachNode(tLinkListNode* pNode); // Added
size_t J3DAPI stdLinklist_GetCount(const tLinkListNode* pCur); // Added
tLinkListNode* J3DAPI stdLinklist_GetNode(const tLinkListNode* pFirstNode, int n); // Added
tLinkListNode* J3DAPI stdLinklist_GetLastNode(const tLinkListNode* pCur); // Added
tLinkListNode* J3DAPI stdLinklist_GetFirstNode(const tLinkListNode* pCur); // Added

// Helper hooking functions
void stdLinkList_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDLINKLIST_H
