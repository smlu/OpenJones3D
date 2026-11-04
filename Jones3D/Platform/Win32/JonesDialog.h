#ifndef JONES3D_JONESDIALOG_H
#define JONES3D_JONESDIALOG_H
#include <Jones3D/types.h>
#include <Jones3D/Platform/Win32/JonesDialogTypes.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI JonesDialog_InitCursorTracking(int* pAlreadyInited);
int J3DAPI JonesDialog_TrackCursor(int bReDraw);
int J3DAPI JonesDialog_ResetCursorTracking();
int J3DAPI JonesDialog_CopyRectToDrawBuffer(GDIDIBSectionInfo* pInfo, const PPOINT lpSourceOrigin, LPRECT pDstRect);
int J3DAPI JonesDialog_RestoreBackground(HDC hdc, HWND hwnd, LPPOINT a3, LPRECT a4);
int J3DAPI JonesDialog_ShowDialog(LPCSTR lpTemplateName, HWND hWnd, DLGPROC lpDialogFunc, LPARAM dwInitParam);
void J3DAPI JonesDialog_SetGameState(JonesDialogGameState* pState, int bWindowModeSupported);
void J3DAPI JonesDialog_ResetGameState(JonesDialogGameState* pState, int bWindowModeSupported);
int JonesDialog_DummyFlipGDI(void);
INT_PTR CALLBACK JonesDialog_SubclassDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK JonesDialog_SubclassDialogWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void J3DAPI JonesDialog_HandleWM_ERASEBKGND(HWND hwnd, WPARAM wParam, int bIconEraseBkgnd, JonesDialogData* pData);
void J3DAPI JonesDialog_HandleWM_PAINT(HWND hWnd, JonesDialogData* lpData);
void J3DAPI JonesDialog_HandleWM_NCPAINT(HWND hwnd, WPARAM wParam, JonesDialogData* lpData);
int J3DAPI JonesDialog_AllocScreenDBISection(int* pbSkippedAllocation);
int J3DAPI JonesDialog_AllocOffScreenGDISection(GDIDIBSectionInfo* pInfo, HDC hdc, int b24bpp, int* pBSkipedAllocation);
void J3DAPI JonesDialog_FreeScreenDIBSection();
int J3DAPI JonesDialog_AllocOffScreenDIBSection(int* pBSkipedAllocation);
int J3DAPI JonesDialog_FreeOffScreenDIBSection();
BOOL J3DAPI JonesDialog_ShowFileSelectDialog(LPOPENFILENAMEA pofn, int bOpen);
UINT_PTR CALLBACK JonesDialog_SubclassFileDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void JonesDialog_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESDIALOG_H
