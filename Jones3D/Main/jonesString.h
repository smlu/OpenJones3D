#ifndef JONES3D_JONESSTRING_H
#define JONES3D_JONESSTRING_H
#include <Jones3D/types.h>
#include <Jones3D/RTI/addresses.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

const char* J3DAPI jonesString_GetString(const char* pKey);
int J3DAPI jonesString_Startup();
void jonesString_Shutdown(void);

// Helper hooking functions
void jonesString_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESSTRING_H
