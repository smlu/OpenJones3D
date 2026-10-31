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

#if defined(J3D_STANDALONE)
extern SithCamera* sithCamera_g_pCurCamera;
#else
#define sithCamera_g_pCurCamera J3D_DECL_FAR_VAR(sithCamera_g_pCurCamera, SithCamera*)
#endif
// extern SithCamera *sithCamera_g_pCurCamera;

#if defined(J3D_STANDALONE)
extern int sithCamera_g_bExtCameraLookMode;
#else
#define sithCamera_g_bExtCameraLookMode J3D_DECL_FAR_VAR(sithCamera_g_bExtCameraLookMode, int)
#endif
// extern int sithCamera_g_bExtCameraLookMode;

#if defined(J3D_STANDALONE)
extern rdVector3 sithCamera_g_camSpot;
#else
#define sithCamera_g_camSpot J3D_DECL_FAR_VAR(sithCamera_g_camSpot, rdVector3)
#endif
// extern rdVector3 sithCamera_g_camSpot;

#if defined(J3D_STANDALONE)
extern SithCamera sithCamera_g_aCameras[7];
#else
#define sithCamera_g_aCameras J3D_DECL_FAR_ARRAYVAR(sithCamera_g_aCameras, SithCamera(*)[7])
#endif
// extern SithCamera sithCamera_g_aCameras[7];

#if defined(J3D_STANDALONE)
extern int sithCamera_g_bCurCameraSet;
#else
#define sithCamera_g_bCurCameraSet J3D_DECL_FAR_VAR(sithCamera_g_bCurCameraSet, int)
#endif
// extern int sithCamera_g_bCurCameraSet;

#if defined(J3D_STANDALONE)
extern SithCameraState sithCamera_g_stateFlags;
#else
#define sithCamera_g_stateFlags J3D_DECL_FAR_VAR(sithCamera_g_stateFlags, SithCameraState)
#endif
// extern SithCameraState sithCamera_g_stateFlags;

#if defined(J3D_STANDALONE)
extern size_t sithCamera_g_curCycleCamNum;
#else
#define sithCamera_g_curCycleCamNum J3D_DECL_FAR_VAR(sithCamera_g_curCycleCamNum, size_t)
#endif
// extern int sithCamera_g_curCycleCamNum;

#if defined(J3D_STANDALONE)
extern rdVector3 sithCamera_g_vecCameraPosOffset;
#else
#define sithCamera_g_vecCameraPosOffset J3D_DECL_FAR_VAR(sithCamera_g_vecCameraPosOffset, rdVector3)
#endif
// extern rdVector3 sithCamera_g_vecCameraPosOffset;

#if defined(J3D_STANDALONE)
extern rdVector3 sithCamera_g_vecCameraAngleOffset;
#else
#define sithCamera_g_vecCameraAngleOffset J3D_DECL_FAR_VAR(sithCamera_g_vecCameraAngleOffset, rdVector3)
#endif
// extern rdVector3 sithCamera_g_vecCameraAngleOffset;

#if defined(J3D_STANDALONE)
extern float sithCamera_g_cameraPosDelta;
#else
#define sithCamera_g_cameraPosDelta J3D_DECL_FAR_VAR(sithCamera_g_cameraPosDelta, float)
#endif
// extern float sithCamera_g_cameraPosDelta;

#if defined(J3D_STANDALONE)
extern float sithCamera_g_cameraAngleDelta;
#else
#define sithCamera_g_cameraAngleDelta J3D_DECL_FAR_VAR(sithCamera_g_cameraAngleDelta, float)
#endif
// extern float sithCamera_g_cameraAngleDelta;

#if defined(J3D_STANDALONE)
extern rdMatrix34 sithCamera_g_orbCamOrient;
#else
#define sithCamera_g_orbCamOrient J3D_DECL_FAR_VAR(sithCamera_g_orbCamOrient, rdMatrix34)
#endif
// extern rdMatrix34 sithCamera_g_orbCamOrient;

#if defined(J3D_STANDALONE)
extern rdVector3 sithCamera_g_camLookSpot;
#else
#define sithCamera_g_camLookSpot J3D_DECL_FAR_VAR(sithCamera_g_camLookSpot, rdVector3)
#endif
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
