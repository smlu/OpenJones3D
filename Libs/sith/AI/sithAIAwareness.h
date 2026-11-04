#ifndef SITH_SITHAIAWARENESS_H
#define SITH_SITHAIAWARENESS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHAIAWARENESS_TASKID    3u
#define SITHAIAWARENESS_NUMTYPES  4u
#define SITHAIAWARENESS_MAXEVENTS 32u

extern SithAIAwarenessSector* sithAIAwareness_g_aSectors;
// extern SithAIAwarenessSector *sithAIAwareness_g_aSectors;

int sithAIAwareness_Open(void);
void sithAIAwareness_Close(void);

int J3DAPI sithAIAwareness_CreateTransmittingEvent(SithSector* pSector, const rdVector3* pos, int type, float transmittingLevel, SithThing* pThing);
int J3DAPI sithAIAwareness_IsInTransmittingRange(const SithAIAwarenessSector* pAISector, int type, const SithThing* pThing);

void sithAIAwareness_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHAIAWARENESS_H
