#ifndef WKERNEL_WKERNELWIN32_H
#define WKERNEL_WKERNELWIN32_H
#include <windows.h>
#include <wkernel/wkernel.h>

J3D_EXTERN_C_START

typedef int (J3DAPI* WKERNELWNDPROC)(HWND, UINT, WPARAM, LPARAM, int* pRetValue);

// Runs the window and event loop; the process procs must be set before.
int J3DAPI wkernel_Run(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd, LPCSTR lpWindowName); // Note, process proc must be set before

void J3DAPI wkernel_SetWindowStyle(LONG dwNewLong);
void J3DAPI wkernel_SetWindowProc(WKERNELWNDPROC pfProc); // Window process

J3D_EXTERN_C_END
#endif // WKERNEL_WKERNELWIN32_H
