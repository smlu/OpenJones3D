#ifndef SITH_SITHWORLD_H
#define SITH_SITHWORLD_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

// Added
// The macro converts the resource index to static resource index (jones3Dstatic.cnd)
#define SITHWORLD_STATICINDEX(idx) ((idx) | SITH_STATICRESOURCE_INDEXMASK)
#define SITHWORLD_FROM_STATICINDEX(idx) ((idx) & ~SITH_STATICRESOURCE_INDEXMASK)
#define SITHWORLD_IS_STATICINDEX(idx) ((idx & SITH_STATICRESOURCE_INDEXMASK) != 0)


#if defined(J3D_LOCAL_SITH_RTI)
extern SithWorld* sithWorld_g_pCurrentWorld;
extern SithWorld* sithWorld_g_pStaticWorld;
extern SithWorld* sithWorld_g_pLastLoadedWorld;
extern int sithWorld_g_bLoading;
#else
#   define sithWorld_g_pCurrentWorld J3D_DECL_FAR_VAR(sithWorld_g_pCurrentWorld, SithWorld*)
#   define sithWorld_g_pStaticWorld J3D_DECL_FAR_VAR(sithWorld_g_pStaticWorld, SithWorld*)
#   define sithWorld_g_pLastLoadedWorld J3D_DECL_FAR_VAR(sithWorld_g_pLastLoadedWorld, SithWorld*)
#   define sithWorld_g_bLoading J3D_DECL_FAR_VAR(sithWorld_g_bLoading, int)
#endif

int sithWorld_Startup(void);
void sithWorld_Shutdown(void);

void J3DAPI sithWorld_SetLoadProgressCallback(SithWorldLoadProgressCallback pfProgressCallback);
void J3DAPI sithWorld_UpdateLoadProgress(float progress);
int J3DAPI sithWorld_Load(SithWorld* pWorld, const char* pFilename);
int J3DAPI sithWorld_LoadPostProcess(SithWorld* pWorld);
int J3DAPI sithWorld_LoadEntryText(SithWorld* pWorld, const char* pFilename, int bSkipParsing);
void J3DAPI sithWorld_Close(SithWorld* pWorld);

SithWorld* sithWorld_NewEntry(void);
void J3DAPI sithWorld_Free(SithWorld* pWorld);

int J3DAPI sithWorld_ReadHeaderText(SithWorld* pWorld, int bSkip);
int J3DAPI sithWorld_ReadCopyrightText(SithWorld* pWorld, int bSkip);

void J3DAPI sithWorld_GetMemoryUsage(const SithWorld* pWorld, size_t(*aMemUsed)[17], size_t(*aCount)[17]);

int J3DAPI sithWorld_RegisterTextSectionParser(const char* aSectionName, SithWorldTextSectionParseFunc pfParseFunction);

void J3DAPI sithWorld_CalcSurfaceNormals(SithWorld* pWorld); // Added from debug
void J3DAPI sithWorld_ResetRenderState(SithWorld* pWorld);
void J3DAPI sithWorld_ResetGeoresource(SithWorld* pWorld); // Added from debug

int J3DAPI sithWorld_ValidateWorld(const SithWorld* pWorld);
uint32_t J3DAPI sithWorld_CalcWorldChecksum(SithWorld* pWorld, uint32_t seed); // Added from debug

int J3DAPI sithWorld_InitPlayers(SithWorld* pWorld);
int J3DAPI sithWorld_ReadTextSection(SithWorld* pWorld, const char* aSectionName, int bSkip);

const char* sithWorld_GetCurReadSection(void); // Added from debug

int J3DAPI sithWorld_GetTextSectionParserIndex(const char* aSectionName);

int J3DAPI sithWorld_WriteGeoresourceText(const SithWorld* pWorld); // Added from debug
int J3DAPI sithWorld_ReadGeoresourceText(SithWorld* pWorld, int bSkip);

int J3DAPI sithWorld_WriteGeoresourceBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithWorld_ReadGeoresourceBinary(tFileHandle fh, SithWorld* pWorld);

int J3DAPI sithWorld_LoadEntryBinary(SithWorld* pWorld, const char* pFilePath);

// Helper hooking functions
void sithWorld_InstallHooks(void);
void sithWorld_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHWORLD_H
