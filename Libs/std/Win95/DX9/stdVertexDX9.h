#ifndef STD_STDVERTEX_DX9_H
#define STD_STDVERTEX_DX9_H

#include <std/Win95/DX9/stdSysTypesDX9.h>
#include <std/types.h>

// Added: Keeps the original Direct3D names for the engine-owned transformed and
// lit vertex and colors, so the DX9 backend and tests can spell them the way the
// original code did. StdVertexTL and the STD_COLOR_* macros are in std/types.h.
#if defined(J3D_DIRECTX9)
typedef StdVertexTL D3DTLVERTEX, *LPD3DTLVERTEX;

 // The FVF format for D3DTLVERTEX
#define D3DTLVERTEX_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)

#define D3DRGBA D3DCOLOR_COLORVALUE
#define RGBA_MAKE D3DCOLOR_ARGB
#define D3DRGB(r, g , b) \
     D3DCOLOR_XRGB((DWORD)((r)*255.f),(DWORD)((g)*255.f),(DWORD)((b)*255.f))
#endif // J3D_DIRECTX9

#endif // STD_STDVERTEX_DX9_H
