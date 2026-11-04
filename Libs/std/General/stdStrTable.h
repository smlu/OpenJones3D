#ifndef STD_STDSTRTABLE_H
#define STD_STDSTRTABLE_H
#include <j3dcore/j3d.h>
#include <std/types.h>

#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>

J3D_EXTERN_C_START

int J3DAPI stdStrTable_Load(tStringTable* pStrTable, const char* pFilename);
char* J3DAPI stdStrTable_ParseLiteral(const char* pStr, char** ppBegin, char** ppEnd);
void J3DAPI stdStrTable_Free(tStringTable* pStrTable);
wchar_t* J3DAPI stdStrTable_GetValue(const tStringTable* pStrTable, const char* pKey);
wchar_t* J3DAPI stdStrTable_GetValueOrKey(const tStringTable* pStrTable, const char* pKey);
int J3DAPI stdStrTable_ReadLine(tFileHandle fh, char* pStr, int size);

// Helper hooking functions
void stdStrTable_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDSTRTABLE_H
