#ifndef RDROID_RDROID_H
#define RDROID_RDROID_H
#include <j3dcore/j3d.h>

#include <rdroid/types.h>

#include <std/types.h>
#include <std/General/std.h>

J3D_EXTERN_C_START

#define RD_REF_WIDTH       640.0f
#define RD_REF_HEIGHT      480.0f
#define RD_REF_APECTRATIO (RD_REF_WIDTH / RD_REF_HEIGHT) // Original aspect ratio (4:3)

// Guessed macro: Older GPUs, particularly those in the DirectX 6 era, often used fixed - point math instead of floating - point for performance reasons.
// The rhw value was frequently packed into a 16 - bit or 32 - bit fixed - point format, and 32767.5f is the middle value for a signed 16 - bit integer range(-32768 to 32767).
// Using 1.0f / 32767.5f normalizes the W component, keeping it within a more precise range for the hardware.
#define RD_FIXEDPOINT_RHW_SCALE_BASE (1.0f / 32767.5f) // 0.000030518044f

// Predefined RHW scale factors.
// Higher multipliers yield a larger RHW value, which moves vertices further from the camera.
// These are useful for quickly selecting standard scaling values.
#define RD_FIXEDPOINT_RHW_SCALE_X1   RD_FIXEDPOINT_RHW_SCALE(1) // same as RD_FIXEDPOINT_RHW_SCALE_BASE
#define RD_FIXEDPOINT_RHW_SCALE_X2   RD_FIXEDPOINT_RHW_SCALE(2) // 0.000061036088
#define RD_FIXEDPOINT_RHW_SCALE_X3   RD_FIXEDPOINT_RHW_SCALE(3) // 0.000091554131
#define RD_FIXEDPOINT_RHW_SCALE_X4   RD_FIXEDPOINT_RHW_SCALE(4) // 0.00012207218
#define RD_FIXEDPOINT_RHW_SCALE_X5   RD_FIXEDPOINT_RHW_SCALE(5) // 0.00015259022
#define RD_FIXEDPOINT_RHW_SCALE_X6   RD_FIXEDPOINT_RHW_SCALE(6) // 0.000183108264; Added
#define RD_FIXEDPOINT_RHW_SCALE_X7   RD_FIXEDPOINT_RHW_SCALE(7) // 0.000213626306; Added
#define RD_FIXEDPOINT_RHW_SCALE_X8   RD_FIXEDPOINT_RHW_SCALE(8) // 0.000244162978; Added
#define RD_FIXEDPOINT_RHW_SCALE_X9   RD_FIXEDPOINT_RHW_SCALE(9) // 0.000274662394; Added

// Macro to compute an RHW scale factor for an arbitrary multiplier.
// Use this macro when you need a non-standard scale factor instead of the predefined ones.
#define RD_FIXEDPOINT_RHW_SCALE(multiplier) ((multiplier) * RD_FIXEDPOINT_RHW_SCALE_BASE)

#define RDLOG_DEBUG(format, ...) \
    J3DLOG_DEBUG(rdroid_g_pHS, format, ##__VA_ARGS__)

#define RDLOG_STATUS(format, ...) \
    J3DLOG_STATUS(rdroid_g_pHS, format, ##__VA_ARGS__)

#define RDLOG_MESSAGE(format, ...) \
    J3DLOG_MESSAGE(rdroid_g_pHS, format, ##__VA_ARGS__)

#define RDLOG_WARNING(format, ...) \
    J3DLOG_WARNING(rdroid_g_pHS, format, ##__VA_ARGS__)

#define RDLOG_ERROR(format, ...) \
    J3DLOG_ERROR(rdroid_g_pHS, format, ##__VA_ARGS__)

#define RDLOG_FATAL(message) \
    J3DLOG_FATAL(rdroid_g_pHS, message)

#define RD_ASSERT(condition) \
    J3D_ASSERT(condition, rdroid_g_pHS )

#define RD_ASSERTREL(condition) \
    J3D_ASSERTREL(condition, rdroid_g_pHS )

extern rdLightMode rdroid_g_curLightingMode;
// extern rdLightMode rdroid_g_curLightingMode;

extern tHostServices* rdroid_g_pHS;
// extern tHostServices *rdroid_g_pHS;

extern rdGeometryMode rdroid_g_curGeometryMode;
// extern rdGeometryMode rdroid_g_curGeometryMode;

extern rdRoidFlags rdroid_g_curRenderOptions;
// extern rdRoidFlags rdroid_g_curRenderOptions;

void J3DAPI rdStartup();
void J3DAPI rdShutdown();

int J3DAPI rdOpen();
void J3DAPI rdClose();

void J3DAPI rdSetServices(tHostServices* pHS);
void rdClearServices(void);

rdRoidFlags J3DAPI rdGetRenterOptions();
void J3DAPI rdSetRenderOptions(rdRoidFlags options);

rdGeometryMode J3DAPI rdSGetGeometryMode(void); // Added
void J3DAPI rdSetGeometryMode(rdGeometryMode mode);

rdLightMode rdGetLightingMode(void); // Added
void J3DAPI rdSetLightingMode(rdLightMode mode);

void rdroid_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDROID_H
