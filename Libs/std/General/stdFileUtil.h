#ifndef STD_STDFILEUTIL_H
#define STD_STDFILEUTIL_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

FindFileData* J3DAPI stdFileUtil_NewFind(const char* path, int mode, const char* pFilter);
void J3DAPI stdFileUtil_DisposeFind(FindFileData* pFileData);
int J3DAPI stdFileUtil_FindNext(FindFileData* ffStruct, tFoundFileInfo* pFileInfo);
int J3DAPI stdFileUtil_FindQuick(const char* pPath, int mode, const char* pFilter, tFoundFileInfo* pFileInfo); // Added
int J3DAPI stdFileUtil_CountMatches(const char* pPath, int mode, const char* pFilter); // Added
int J3DAPI stdFileUtil_MkDir(const char* lpPathName);
int J3DAPI stdFileUtil_FileExists(const char* pFilename);
int J3DAPI stdFileUtil_RmDir(const char* pDir); // Added
int J3DAPI stdFileUtil_DelFile(const char* pFilename); // Added

// Helper hooking functions
void stdFileUtil_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDFILEUTIL_H
