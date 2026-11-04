#ifndef JONES3D_JONESFILE_H
#define JONES3D_JONESFILE_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

void J3DAPI JonesFile_Startup(tHostServices* pHS);
void JonesFile_Shutdown(void);

int J3DAPI JonesFile_Open(tHostServices* pEnv, const char* pInstallPath, const char* pPathCD);
void JonesFile_Close(void);

int JonesFile_GetCurrentCDNum(void);
int J3DAPI JonesFile_FileExists(const char* pFilename);

void J3DAPI JonesFile_SetInstallPath(const char* pPath);
const char* JonesFile_GetInstallPath(void);
const char* JonesFile_GetResourcePath(void);

const char* JonesFile_GetWorkingDirPath(void);

void J3DAPI JonesFile_SetCDPath(const char* pPath);
const char* JonesFile_GetCDPath(void);

void JonesFile_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESFILE_H
