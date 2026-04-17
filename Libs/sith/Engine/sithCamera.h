#ifndef SITH_SITHCAMERA_H
#define SITH_SITHCAMERA_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHCAMERA_INTCAMERANUM     0 // internal camera aka 1st person
#define SITHCAMERA_EXTCAMERANUM     1 // external camera aka 3rd person
#define SITHCAMERA_CINEMACAMERANUM  2 // cinematic camera. Default camera to be used in cutscenes
#define SITHCAMERA_IDLECAMERANUM    4 // idle camera
#define SITHCAMERA_UNKNONWCAMERANUM 5 // unknown
#define SITHCAMERA_ORBCAMERANUM     6 // orbital camera

// Min/Max camera FOV
#define SITHCAMERA_FOVMIN      20.0f
#define SITHCAMERA_FOVMAX     150.0f
#define SITHCAMERA_FOVDEFAULT  90.0f // default camera fov

// Default camera attenuation
#define SITHCAMERA_ATTENUATIONMIN 0.4f
#define SITHCAMERA_ATTENUATIONMAX 0.8f

#define sithCamera_g_pCurCamera J3D_DECL_FAR_VAR(sithCamera_g_pCurCamera, SithCamera*)
// extern SithCamera *sithCamera_g_pCurCamera;

#define sithCamera_g_bExtCameraLookMode J3D_DECL_FAR_VAR(sithCamera_g_bExtCameraLookMode, int)
// extern int sithCamera_g_bExtCameraLookMode;

#define sithCamera_g_camSpot J3D_DECL_FAR_VAR(sithCamera_g_camSpot, rdVector3)
// extern rdVector3 sithCamera_g_camSpot;

#define sithCamera_g_aCameras J3D_DECL_FAR_ARRAYVAR(sithCamera_g_aCameras, SithCamera(*)[7])
// extern SithCamera sithCamera_g_aCameras[7];

#define sithCamera_g_bCurCameraSet J3D_DECL_FAR_VAR(sithCamera_g_bCurCameraSet, int)
// extern int sithCamera_g_bCurCameraSet;

#define sithCamera_g_stateFlags J3D_DECL_FAR_VAR(sithCamera_g_stateFlags, SithCameraState)
// extern SithCameraState sithCamera_g_stateFlags;

#define sithCamera_g_curCycleCamNum J3D_DECL_FAR_VAR(sithCamera_g_curCycleCamNum, size_t)
// extern int sithCamera_g_curCycleCamNum;

#define sithCamera_g_vecCameraPosOffset J3D_DECL_FAR_VAR(sithCamera_g_vecCameraPosOffset, rdVector3)
// extern rdVector3 sithCamera_g_vecCameraPosOffset;

#define sithCamera_g_vecCameraAngleOffset J3D_DECL_FAR_VAR(sithCamera_g_vecCameraAngleOffset, rdVector3)
// extern rdVector3 sithCamera_g_vecCameraAngleOffset;

#define sithCamera_g_cameraPosDelta J3D_DECL_FAR_VAR(sithCamera_g_cameraPosDelta, float)
// extern float sithCamera_g_cameraPosDelta;

#define sithCamera_g_cameraAngleDelta J3D_DECL_FAR_VAR(sithCamera_g_cameraAngleDelta, float)
// extern float sithCamera_g_cameraAngleDelta;

#define sithCamera_g_orbCamOrient J3D_DECL_FAR_VAR(sithCamera_g_orbCamOrient, rdMatrix34)
// extern rdMatrix34 sithCamera_g_orbCamOrient;

#define sithCamera_g_camLookSpot J3D_DECL_FAR_VAR(sithCamera_g_camLookSpot, rdVector3)
// extern rdVector3 sithCamera_g_camLookSpot;

int sithCamera_Startup(void);
void sithCamera_Shutdown(void);

int J3DAPI sithCamera_Open(rdCanvas* pCanvas, float aspect);
void sithCamera_Close(void);
bool sithCamera_IsOpen(void); // Added (not from debug ver)

void sithCamera_ResetAllCameras(void);

SithCamera* J3DAPI sithCamera_New(SithCameraType type, uint32_t a2, float fov, float aspect, rdCanvas* pCanvas, SithThing* pPrimaryFocus, SithThing* pSecondaryFocus); // Added from debug
int J3DAPI sithCamera_NewEntry(SithCamera* pCamera, uint32_t a2, SithCameraType type, float fov, float aspect, rdCanvas* pCanvas, SithThing* pPrimaryFocus, SithThing* pSecondaryFocus);

void J3DAPI sithCamera_Free(SithCamera* pCamera); // Added from debug
void J3DAPI sithCamera_FreeEntry(SithCamera* pCamera);

void J3DAPI sithCamera_SetCameraCanvas(SithCamera* pCamera, float aspect, rdCanvas* pCanvas);
int J3DAPI sithCamera_SetCurrentCamera(SithCamera* pCamera);

void sithCamera_CycleCamera(void);
int sithCamera_SetCurrentToCycleCamera(void);
void J3DAPI sithCamera_SetCameraFocus(SithCamera* pCamera, SithThing* pPrimaryFocusThing, SithThing* pSecondaryFocusThing);

void J3DAPI SithCamera_ZoomIn(SithCamera* pCamera); // Added from debug
void J3DAPI SithCamera_ZoomOut(SithCamera* pCamera); // Added from debug
void J3DAPI sithCamera_SetCameraFOV(SithCamera* pCamera, float fov);

void J3DAPI sithCamera_SetCameraPosition(SithCamera* pCamera, const rdVector3* pPos);
void J3DAPI sithCamera_GetCameraPosition(const SithCamera* pCamera, rdVector3* pDstPos);

void J3DAPI sithCamera_SetExtCameraOffset(const rdVector3* pOffset);
void J3DAPI sithCamera_SetExtCameraLookOffset(const rdVector3* pOffset);
void sithCamera_RestoreExtCamera(void);

SithThing* J3DAPI sithCamera_GetPrimaryFocus(const SithCamera* pCamera);
SithThing* J3DAPI sithCamera_GetSecondaryFocus(const SithCamera* pCamera);

void J3DAPI sithCamera_Update(SithCamera* pCamera);
void sithCamera_RenderScene(void);

void J3DAPI sithCamera_SetPOVShake(const rdVector3* posOffset, const rdVector3* angleOffset, float posDelta, float angleDelta);

void J3DAPI sithCamera_SetCameraStateFlags(SithCameraState flags);
SithCameraState J3DAPI sithCamera_GetCameraStateFlags();

// Helper hooking functions
void sithCamera_InstallHooks(void);
void sithCamera_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCAMERA_H
