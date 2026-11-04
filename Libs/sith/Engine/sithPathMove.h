#ifndef SITH_SITHPATHMOVE_H
#define SITH_SITHPATHMOVE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

#include <std/types.h>
#include <std/General/stdConffile.h>

J3D_EXTERN_C_START

void J3DAPI sithPathMove_SkipToFrame(SithThing* pThing, size_t frame, float velocity);
void J3DAPI sithPathMove_MoveToFrame(SithThing* pThing, size_t frame, float velocity);
void J3DAPI sithPathMove_RotatePivot(SithThing* pThing, const rdVector3* pOffset, const rdVector3* pRot, float rotTime);

float J3DAPI sithPathMove_MoveToPos(SithThing* pThing, SithPathFrame* pos);
void J3DAPI sithPathMove_RotateToPYR(SithThing* pThing, rdVector3* pyr, float time);

void J3DAPI sithPathMove_Update(SithThing* pThing, float simTime);
int J3DAPI sithPathMove_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum);

void J3DAPI sithPathMove_Finish(SithThing* pThing);

int J3DAPI sithPathMove_PathMovePause(SithThing* pThing);
int J3DAPI sithPathMove_PathMoveResume(SithThing* pThing);

void J3DAPI sithPathMove_Create(SithThing* pNewThing, const SithThing* pMarker, const rdVector3* pOffset);

// Helper hooking functions
void sithPathMove_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHPATHMOVE_H
