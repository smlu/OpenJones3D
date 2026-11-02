#include "rdroid.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Primitives/rdFont.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/RTI/symbols.h>

#include <stdbool.h>

#ifdef J3D_STANDALONE
// Added: Standalone builds have no retail executable to hold these. *_ResetGlobals sets the starting values.
rdLightMode rdroid_g_curLightingMode;
tHostServices* rdroid_g_pHS;
rdGeometryMode rdroid_g_curGeometryMode;
rdRoidFlags rdroid_g_curRenderOptions;
#endif

static bool rdroid_bRDroidStartup;
static bool rdroid_bRDroidOpen;


void rdroid_InstallHooks(void)
{
    J3D_HOOKFUNC(rdSetServices);
    J3D_HOOKFUNC(rdClearServices);
    J3D_HOOKFUNC(rdStartup);
    J3D_HOOKFUNC(rdShutdown);
    J3D_HOOKFUNC(rdOpen);
    J3D_HOOKFUNC(rdClose);
    J3D_HOOKFUNC(rdSetRenderOptions);
    J3D_HOOKFUNC(rdSetGeometryMode);
    J3D_HOOKFUNC(rdSetLightingMode);
    J3D_HOOKFUNC(rdGetRenterOptions);
}

void rdroid_ResetGlobals(void)
{
    memset(&rdroid_g_curLightingMode, 0, sizeof(rdroid_g_curLightingMode));
    memset(&rdroid_g_pHS, 0, sizeof(rdroid_g_pHS));
    memset(&rdroid_g_curGeometryMode, 0, sizeof(rdroid_g_curGeometryMode));
    memset(&rdroid_g_curRenderOptions, 0, sizeof(rdroid_g_curRenderOptions));
}

void J3DAPI rdSetServices(tHostServices* pHS)
{
    rdroid_g_pHS = pHS;
}

void rdClearServices(void)
{
    rdroid_g_pHS = NULL;
}

void J3DAPI rdStartup()
{
    if ( rdroid_bRDroidStartup ) {
        RDLOG_ERROR("Warning: rdStartup() has already been called!\n");
        // TODO: should return with error
    }

    rdCache_Startup();
    rdFont_Startup();
    rdroid_bRDroidStartup = true;
}

void J3DAPI rdShutdown()
{
    if ( rdroid_bRDroidStartup )
    {
        rdFont_Shutdown();
        rdroid_bRDroidStartup = false;
    }
}

int J3DAPI rdOpen()
{
    if ( !rdroid_bRDroidStartup ) {
        return 0;
    }

    if ( rdroid_bRDroidOpen )
    {
        RDLOG_ERROR("Warning: System already open!\n");
        return 1;
    }

    if ( rdFont_Open() ) {
        return 0;
    }

    rdroid_g_curGeometryMode  = RD_GEOMETRY_FULL;
    rdroid_g_curLightingMode  = RD_LIGHTING_GOURAUD;
    rdroid_g_curRenderOptions = RDROID_BACKFACE_CULLING_ENABLED;
    rdroid_bRDroidOpen        = true;
    return 1;
}

void J3DAPI rdClose()
{
    if ( rdroid_bRDroidStartup )
    {
        if ( rdroid_bRDroidOpen ) {
            rdFont_Close();
        }
        rdroid_bRDroidOpen = false;
    }
}

rdRoidFlags J3DAPI rdGetRenterOptions()
{
    return rdroid_g_curRenderOptions;
}

void J3DAPI rdSetRenderOptions(rdRoidFlags options)
{
    rdroid_g_curRenderOptions = options;
}

rdGeometryMode J3DAPI rdSGetGeometryMode(void)
{
    return rdroid_g_curGeometryMode;
}

void J3DAPI rdSetGeometryMode(rdGeometryMode mode)
{
    rdroid_g_curGeometryMode = mode;
}

rdLightMode rdGetLightingMode(void)
{
    return rdroid_g_curLightingMode;
}

void J3DAPI rdSetLightingMode(rdLightMode mode)
{
    rdroid_g_curLightingMode = mode;
}