#ifndef SITH_SITHITEM_H
#define SITH_SITHITEM_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>

#include <std/types.h>
#include <std/General/stdConffile.h>

J3D_EXTERN_C_START

int J3DAPI sithItem_PlayerCollisionHandler(SithThing* pItem, SithThing* pPlayer, SithCollision* pCollision, int a5);
void J3DAPI sithItem_Initialize(SithThing* pThing);
void J3DAPI sithItem_SetItemTaken(SithThing* pItem, const SithThing* pSrcThing, int bNoMultiSync);
void J3DAPI sithItem_DestroyItem(SithThing* pItem);
int J3DAPI sithItem_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum);

// Helper hooking functions
void sithItem_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHITEM_H
