#include "stdEffect.h"
#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>
#ifdef J3D_OPENGL
#include <std/Win95/stdShader.h>
#endif


static tStdFadeFactor stdEffect_fadeFactor;

void stdEffect_InstallHooks(void)
{
    J3D_HOOKFUNC(stdEffect_Startup);
    J3D_HOOKFUNC(stdEffect_Reset);
    J3D_HOOKFUNC(stdEffect_SetFadeFactor);
    J3D_HOOKFUNC(stdEffect_GetFadeFactor);
}

void stdEffect_ResetGlobals(void)
{
}

void stdEffect_Startup(void)
{
    stdEffect_Reset();
}

void stdEffect_Shutdown(void)
{
    stdEffect_Reset();
}

void stdEffect_Reset(void)
{
    stdEffect_fadeFactor.bEnabled = 0;
    stdEffect_fadeFactor.factor   = 1.0f;
}

void J3DAPI stdEffect_SetFadeFactor(int bEnabled, float factor)
{
    stdEffect_fadeFactor.bEnabled = bEnabled;
    stdEffect_fadeFactor.factor = factor;
#ifdef J3D_OPENGL
    stdShader_UpdateFadeFactor();
#endif
}

const tStdFadeFactor* J3DAPI stdEffect_GetFadeFactor()
{
    return &stdEffect_fadeFactor;
}
