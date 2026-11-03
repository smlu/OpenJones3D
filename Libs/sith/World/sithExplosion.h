#ifndef SITH_SITHEXPLOSION_H
#define SITH_SITHEXPLOSION_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>

#include <std/types.h>
#include <std/General/stdConffile.h>

J3D_EXTERN_C_START

void J3DAPI sithExplosion_Initialize(SithThing* pThing);
void J3DAPI sithExplosion_CreateSpriteThing(SithThing* pThing);

void J3DAPI sithExplosion_UpdateExpandFade(SithExplosionInfo* pExplode);
float J3DAPI sithExplosion_Lerp(float min, float max, float maxTime, float deltaTime);
void J3DAPI sithExplosion_Update(SithThing* pThing, float secDeltaTime);
void J3DAPI sithExplosion_MakeBlast(SithThing* pThing);
int J3DAPI sithExplosion_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum);

// Helper hooking functions
void sithExplosion_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHEXPLOSION_H
