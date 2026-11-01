// Added: DirectX 9 backend API functions that call out of std and return or take DirectX 9
// platform types. Only the DirectX 9 backend files and tests may use these.
#ifndef STD_STDDX9API_H
#define STD_STDDX9API_H
#include <std/Win95/DX9/stdSysTypesDX9.h>
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

// Moved from stdDisplay.h: the DirectX 9 backend owns the IDirect3D9 interface object.
LPDIRECT3D9 stdDisplay_GetDirect3D(void);

#ifdef J3D_TEST
// Moved from stdDisplay.h: lets tests read the DirectX 9 present parameters.
void stdDisplay_TestGetPresentParameters(D3DPRESENT_PARAMETERS* pParams);
#endif

// Moved from std3D.h: returns the human-readable status text of a DirectX 9 result, 0 is success.
const char* J3DAPI std3D_D3DGetStatus(HRESULT res);
// Moved from std3D.h: returns the currently opened DirectX 9 device, or NULL when none is open.
tSysDevice3D* std3D_GetD3DDevice(void); // Added

J3D_EXTERN_C_END
#endif // STD_STDDX9API_H
