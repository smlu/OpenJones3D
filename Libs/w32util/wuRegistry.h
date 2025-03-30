#ifndef W32UTIL_WUREGISTRY_H
#define W32UTIL_WUREGISTRY_H
#define STRICT
#include <Windows.h>
#include <j3dcore/j3d.h>
#include <w32util/RTI/addresses.h>

J3D_EXTERN_C_START

int J3DAPI wuRegistry_Startup(HKEY hKey, LPCSTR lpSubKey);
bool wuRegistry_HasStarted(void);
void J3DAPI wuRegistry_Shutdown();
int J3DAPI wuRegistry_SaveInt(const char* pKey, int value);
int J3DAPI wuRegistry_SaveFloat(const char* pKey, float value);
int J3DAPI wuRegistry_GetInt(const char* pKey, int defaultValue);
float J3DAPI wuRegistry_GetFloat(const char* pKey, float defaultValue);
int J3DAPI wuRegistry_SaveIntEx(const char* pKey, int defaultValue);
int J3DAPI wuRegistry_GetIntEx(const char* pKey, int defaultValue);
int J3DAPI wuRegistry_SaveStr(const char* pKey, const char* pStr);
int J3DAPI wuRegistry_GetStr(const char* pKey, char* pDstStr, size_t size, const char* pDefaultValue);

// Helper hooking functions
void wuRegistry_InstallHooks(void);
void wuRegistry_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // W32UTIL_WUREGISTRY_H
