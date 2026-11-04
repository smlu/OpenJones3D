#ifndef JONES3D_JONESCOG_H
#define JONES3D_JONESCOG_H
#include <Jones3D/types.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

extern int jonesCog_g_bShowHealthHUD;
// extern int jonesCog_g_bShowHealthHUD;

extern int jonesCog_g_bMenuVisible;
// extern int jonesCog_g_bMenuVisible;

extern int jonesCog_g_bEnableGamesave;
// extern int jonesCog_g_bEnableGamesave;

extern int jonesCog_g_dword_17EFFCC;
// extern int jonesCog_g_dword_17EFFCC;

int jonesCog_Startup(void);
void jonesCog_Shutdown(void);

void jonesCog_EndLevel(void);
void J3DAPI jonesCog_StartCutscene(SithCog* pCog);// TODO: this should actually be restore cutscene if NULL is passed

void J3DAPI jonesCog_ToggleInterface(SithCog* pCog); // TODO: this should actually be restore interface if NULL is passed
void J3DAPI jonesCog_EnableInterface(bool bEnable); // Added

void J3DAPI jonesCog_EnableIndicatrors(bool bEnable); // Added

// Helper hooking functions
void jonesCog_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESCOG_H
