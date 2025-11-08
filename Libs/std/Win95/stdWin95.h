#ifndef STD_STDWIN95_H
#define STD_STDWIN95_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>
#if defined(J3D_OPENGL)
#include <SDL3/SDL.h>
#endif


J3D_EXTERN_C_START

void J3DAPI stdWin95_SetWindow(HWND hwnd);
HWND J3DAPI stdWin95_GetWindow();
void J3DAPI stdWin95_SetInstance(HINSTANCE hinstance);
HINSTANCE stdWin95_GetInstance(void);
void J3DAPI stdWin95_SetGuid(const GUID* pGuid);
const GUID* J3DAPI stdWin95_GetGuid(void); // Added

// Helper hooking functions
void stdWin95_InstallHooks(void);
void stdWin95_ResetGlobals(void);

#if defined (J3D_OPENGL)

void J3DAPI stdWin95_SetGLContext(SDL_GLContext context);

SDL_GLContext J3DAPI stdWin95_GetGLContext(void);

void J3DAPI stdWin95_SetSDLWindow(SDL_Window *sdlWindow);

SDL_Window* J3DAPI stdWin95_GetSDLWindow(void);

#endif

J3D_EXTERN_C_END
#endif // STD_STDWIN95_H
