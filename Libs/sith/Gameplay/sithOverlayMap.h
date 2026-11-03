#ifndef SITH_SITHOVERLAYMAP_H
#define SITH_SITHOVERLAYMAP_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHOVERLAYMAP_CFG_GAMEPLAY_SHOWHINTS   "gameplay.showHints"
#define SITHOVERLAYMAP_CFG_GAMEPLAY_MAPROTATION "gameplay.mapRotation"

int J3DAPI sithOverlayMap_Open(SithOverlayMapConfig* pConfig);
void sithOverlayMap_Close(void);

int sithOverlayMap_IsMapVisible(void);
int sithOverlayMap_ToggleMap(void);
void sithOverlayMap_ZoomIn(void);
void sithOverlayMap_ZoomOut(void);

void sithOverlayMap_Draw(void);

void J3DAPI sithOverlayMap_SetShowHints(int bShow);
int J3DAPI sithOverlayMap_GetShowHints(void);
int J3DAPI sithOverlayMap_UpdateHints(void);

void J3DAPI sithOverlayMap_EnableMapRotation(int bEnable);
int sithOverlayMap_GetMapRotation(void);

int J3DAPI sithOverlayMap_SetHintSolved(SithThing* pThing);
int J3DAPI sithOverlayMap_SetHintUnsolved(SithThing* pThing);

void J3DAPI sithOverlayMap_GetNumSeenHints(size_t* pOutNumSeenHints, size_t* pOutNumHints);
SithThing* sithOverlayMap_GetCurrentHintThing(void); // Added: From debug


// Helper hooking functions
void sithOverlayMap_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHOVERLAYMAP_H
