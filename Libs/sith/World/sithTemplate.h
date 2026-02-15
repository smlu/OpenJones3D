#ifndef SITH_SITHTEMPLATE_H
#define SITH_SITHTEMPLATE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

int sithTemplate_Startup(void);
void J3DAPI sithTemplate_Shutdown(void);

int J3DAPI sithTemplate_AllocWorldTemplates(SithWorld* pWorld, size_t size);
void J3DAPI sithTemplate_FreeWorldTemplates(SithWorld* pWorld);

SithThing* J3DAPI sithTemplate_GetTemplateByIndex(int idx);
int J3DAPI sithTemplate_GetTemplateIndex(const SithThing* pTemplate);

//int J3DAPI sithTemplate_WriteThingTemplateListText(const SithWorld* pWorld); // Added from debug
int J3DAPI sithTemplate_ReadThingTemplatesListText(SithWorld* pWorld, int bSkip);
int J3DAPI sithTemplate_WriteThingTemplatesListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithTemplate_ReadThingTemplatesListBinary(tFileHandle fh, SithWorld* pWorld);

int J3DAPI sithTemplate_LoadMasterFile(const char* pFilename); // Added: From debug version
void sithTemplate_FreeMasterFile(void); // Added: From debug version

SithThing* J3DAPI sithTemplate_GetTemplate(const char* pName);

// Helper hooking functions
void sithTemplate_InstallHooks(void);
void sithTemplate_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHTEMPLATE_H
