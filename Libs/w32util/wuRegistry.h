#ifndef W32UTIL_WUREGISTRY_H
#define W32UTIL_WUREGISTRY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <j3dcore/j3d.h>
#include <w32util/RTI/addresses.h>

J3D_EXTERN_C_START

/** Added: Opens settings under the machine-wide root. Returns 0 on success, like wuRegistry_Startup in wuRegistry.c. pSubKey must stay valid until wuRegistry_Shutdown. */
int J3DAPI wuRegistry_StartupLocalMachine(const char* pSubKey);
bool wuRegistry_HasStarted(void); // Added
void J3DAPI wuRegistry_Shutdown();

int J3DAPI wuRegistry_SaveInt(const char* pKey, int value);
int J3DAPI wuRegistry_GetInt(const char* pKey, int defaultValue);

int J3DAPI wuRegistry_SaveFloat(const char* pKey, float value);
float J3DAPI wuRegistry_GetFloat(const char* pKey, float defaultValue);

int J3DAPI wuRegistry_SaveBool(const char* pKey, int defaultValue);
int J3DAPI wuRegistry_GetBool(const char* pKey, int defaultValue);

bool J3DAPI wuRegistry_SaveBinary(const char* pKey, const uint8_t* pData, size_t size); // Added from debug
bool J3DAPI wuRegistry_GetBinary(const char* pKey, uint8_t* pData, size_t size); // Added from debug

int J3DAPI wuRegistry_SaveStr(const char* pKey, const char* pStr);
int J3DAPI wuRegistry_GetStr(const char* pKey, char* pDstStr, size_t size, const char* pDefaultValue);


// Helper hooking functions
void wuRegistry_InstallHooks(void);
void wuRegistry_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // W32UTIL_WUREGISTRY_H
