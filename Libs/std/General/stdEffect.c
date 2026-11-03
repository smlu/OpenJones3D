#include "stdEffect.h"
#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

static tStdFadeFactor stdEffect_fadeFactor;

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
}

const tStdFadeFactor* J3DAPI stdEffect_GetFadeFactor()
{
    return &stdEffect_fadeFactor;
}
