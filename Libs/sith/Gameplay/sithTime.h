#ifndef SITH_SITHTIME_H
#define SITH_SITHTIME_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#if defined(J3D_STANDALONE)
extern uint32_t sithTime_g_frameTime;
#else
#define sithTime_g_frameTime J3D_DECL_FAR_VAR(sithTime_g_frameTime, uint32_t)
#endif
// extern unsigned int sithTime_g_frameTime;

#if defined(J3D_STANDALONE)
extern float sithTime_g_frameTimeFlex;
#else
#define sithTime_g_frameTimeFlex J3D_DECL_FAR_VAR(sithTime_g_frameTimeFlex, float)
#endif
// extern float sithTime_g_secFrameTime;

#if defined(J3D_STANDALONE)
extern float sithTime_g_fps;
#else
#define sithTime_g_fps J3D_DECL_FAR_VAR(sithTime_g_fps, float)
#endif
// extern float sithTime_g_fps;

#if defined(J3D_STANDALONE)
extern uint32_t sithTime_g_msecGameTime;
#else
#define sithTime_g_msecGameTime J3D_DECL_FAR_VAR(sithTime_g_msecGameTime, uint32_t)
#endif
// extern unsigned int sithTime_g_msecGameTime;

#if defined(J3D_STANDALONE)
extern float sithTime_g_secGameTime;
#else
#define sithTime_g_secGameTime J3D_DECL_FAR_VAR(sithTime_g_secGameTime, float)
#endif
// extern float sithTime_g_secGameTime;

#if defined(J3D_STANDALONE)
extern uint32_t sithTime_g_clockTime;
#else
#define sithTime_g_clockTime J3D_DECL_FAR_VAR(sithTime_g_clockTime, uint32_t)
#endif
// extern unsigned int sithTime_g_clockTime;

#if defined(J3D_STANDALONE)
extern int sithTime_g_bPaused;
#else
#define sithTime_g_bPaused J3D_DECL_FAR_VAR(sithTime_g_bPaused, int)
#endif
// extern int sithTime_g_bPaused;

void sithTime_Advance(void);
void sithTime_Pause(void);
void sithTime_Resume(void);
void J3DAPI sithTime_SetFrameTime(uint32_t frameTime); //Added
void sithTime_Reset(void);
void J3DAPI sithTime_SetGameTime(uint32_t msecTime);
int sithTime_IsPaused(void);

// Helper hooking functions
void sithTime_InstallHooks(void);
void sithTime_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHTIME_H
