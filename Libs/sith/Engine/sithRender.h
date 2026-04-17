#ifndef SITH_SITHRENDER_H
#define SITH_SITHRENDER_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

// Traversal algorithm types
typedef enum eSithRenderThingTraversal
{
    // DFS: original legacy custom Depth-First Search traversal (order-dependent, single-path per sector).
    // DFS-like traversal that marks sectors as visited on first encounter.
    // Traversal order depends on adjoin order and accumulated path distance.
    // May miss sectors reachable via alternative or shorter paths.
    SITHRENDER_THING_TRAVERSAL_LDFS = 0,

    // BFS: breadth-first traversal allowing multi-path coverage.
    // Traverses sectors level-by-level, allowing multiple paths to reach
    // the same sector. Ensures full sector coverage within collection
    // distance limits and avoids path-order dependency.
    SITHRENDER_THING_TRAVERSAL_BFS = 1
} SithRenderThingTraversal;

#define SITHRENDER_MAX_VISIBLE_SECTORS       J3D_QOL_VALUE(128, 4096) // Altered: Was 128
#define SITHRENDER_MAX_VISIBLE_THING_SECTORS SITHRENDER_MAX_VISIBLE_SECTORS * 2 // Altered: Was 256

#define SITHRENDER_CULLEDSECTOR_TRAVERSALMODE_DEFAULT J3D_QOL_VALUE(SITHRENDER_THING_TRAVERSAL_BFS, SITHRENDER_THING_TRAVERSAL_LDFS)
#define SITHRENDER_MAXTHINGCOLLECTDISTANCE_DEFAULT    J3D_QOL_VALUE(18.0f, 8.0f)  // Max distance from each visible sector to collect things to be rendered. Altered: Changed to 18 (180m) form 8 (80m) as the BFS algo requires longer distance to collect sectors infront
#define SITHRENDER_MAXLIGHTCOLLECTDISTANCE_DEFAULT    J3D_QOL_VALUE(18.0f, 8.0f)  // Max distance from each visible sector to collect emitting lights.
                                                                                  // This is the new configurable variable. Originally, light collecting distance was limited to things collecting distance (8.0f) and
                                                                                  // max number of collected things - SITHRENDER_MAX_VISIBLE_THING_SECTORS
                                                                                  // The new default distance - 60m was determined based on the light flickering issue in Babylon level (court yard)

#define sithRender_g_fogDensity J3D_DECL_FAR_VAR(sithRender_g_fogDensity, float)
// extern float sithRender_g_fogDensity ;

#define sithRender_g_numDrawnThings J3D_DECL_FAR_VAR(sithRender_g_numDrawnThings, size_t)
// extern int sithRender_g_numDrawnThings;

#define sithRender_g_numArchPolys J3D_DECL_FAR_VAR(sithRender_g_numArchPolys, size_t)
// extern int sithRender_g_numArchPolys;

#define sithRender_g_numAlphaArchPolys J3D_DECL_FAR_VAR(sithRender_g_numAlphaArchPolys, size_t)
// extern int sithRender_g_numAlphaArchPolys;

#define sithRender_g_numThingPolys J3D_DECL_FAR_VAR(sithRender_g_numThingPolys, size_t)
// extern int sithRender_g_numThingPolys;

#define sithRender_g_numAlphaThingPoly J3D_DECL_FAR_VAR(sithRender_g_numAlphaThingPoly, size_t)
// extern int sithRender_g_numAlphaThingPoly;

#define sithRender_g_numVisibleAdjoins J3D_DECL_FAR_VAR(sithRender_g_numVisibleAdjoins, size_t)
// extern int sithRender_g_numVisibleAdjoins;

#define sithRender_g_numVisibleSectors J3D_DECL_FAR_VAR(sithRender_g_numVisibleSectors, size_t)
// extern int sithRender_g_numVisibleSectors;

int sithRender_Startup(void);
void sithRender_Shutdown(void);

int sithRender_Open(void);
void sithRender_Close(void);

void J3DAPI sithRender_SetRenderFlags(int flags);
int sithRender_GetRenderFlags(void);

void J3DAPI sithRender_SetLightingMode(rdLightMode mode);
rdLightMode sithRender_GetLightingMode(void); // Added

/**
 * Sets the visible thing sector collection traversal algorithm.
 * @param mode - Traversal algorithm to use (LEGACY DFS or BFS).
 */
void J3DAPI sithRender_SetCulledSectorTraversalMode(SithRenderThingTraversal mode); // New

/**
 * Gets the current visible thing sector collection traversal algorithm.
 * @return Current traversal mode.
 */
SithRenderThingTraversal sithRender_GetCulledSectorTraversalMode(void); // New

/**
 * Retrieves the maximum path distance to travel from each visible sector into culled sectors for collecting visible things sectors.
 * @return Maximum thing collection distance.
 */
float sithRender_GetMaxThingCollectDistance(void); // New

/**
 * Sets the maximum path distance to travel from each visible sector into culled sectors for collecting visible things sectors.
 * @param distance - Maximum thing collection distance.
 */
void J3DAPI sithRender_SetMaxThingCollectDistance(float distance); // New

/**
 * Retrieves the maximum path distance to travel from each visible sector into culled sectors for collecting dynamic lights.
 * @return Maximum light collection distance.
 */
float sithRender_GetMaxLightCollectDistance(void); // New

/**
 * Sets the maximum path distance to travel from each visible sector into culled sectors for collecting dynamic lights.
 * @param distance - Maximum light collection distance.
 */
void J3DAPI sithRender_SetMaxLightCollectDistance(float distance); // New

//!< Renders sithWorld_g_pCurrentWorld from position of sithCamera_g_pCurCamera
void sithRender_RenderScene(void); // Added from debug

void J3DAPI sithRender_EnablePVSCull(bool bEnable); // New
void sithRender_TogglePVSCull(void); // Added from debug

int sithRender_MakeScreenShot(void);

// Helper hooking functions
void sithRender_InstallHooks(void);
void sithRender_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHRENDER_H
