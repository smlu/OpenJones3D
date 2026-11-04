#ifndef SITH_SITHPARTICLE_H
#define SITH_SITHPARTICLE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

#include <std/types.h>
#include <std/General/stdConffile.h>

J3D_EXTERN_C_START


int sithParticle_Startup(void);
void sithParticle_Shutdown(void);

int J3DAPI sithParticle_AllocWorldParticles(SithWorld* pWorld, size_t size);
void J3DAPI sithParticle_FreeWorldParticles(SithWorld* pWorld);

rdParticle* J3DAPI sithParticle_Load(SithWorld* pWorld, const char* pName);

int J3DAPI sithParticle_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum);
void J3DAPI sithParticle_Update(SithThing* pThing, float secDeltaTime);
void J3DAPI sithParticle_Initalize(SithThing* pThing);
void J3DAPI sithParticle_DestroyParticle(SithThing* pThing);
void J3DAPI sithParticle_Free(SithThing* pThing);


// Helper hooking functions
void sithParticle_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHPARTICLE_H
