// Added: Null version of std/Win95/stdConsole.c (the engine status console). There is no
// Windows console on this platform: Startup succeeds without allocating one, the attribute
// setters keep the same return convention, and WriteConsole mirrors the text to stdout.
#include <j3dcore/j3d.h>

#include <stdio.h>
#include <string.h>

#include <std/Win95/stdConsole.h>

static int stdConsole_textAttribute = 0;
static bool stdConsole_bStarted     = false;

int J3DAPI stdConsole_Startup(const char* pTitleText, int attributes, int bShowMinimized)
{
    J3D_UNUSED(pTitleText);
    J3D_UNUSED(bShowMinimized);

    if ( stdConsole_bStarted )
    {
        return 1;
    }

    stdConsole_bStarted = true;
    stdConsole_SetAttributes((uint16_t)attributes);
    return 1;
}

void stdConsole_Shutdown(void)
{
    stdConsole_bStarted = false;
}

int J3DAPI stdConsole_SetAttributes(uint16_t wAttributes)
{
    J3D_UNUSED(wAttributes);
    return 1;
}

int J3DAPI stdConsole_SetConsoleTextAttribute(uint16_t wAttributes)
{
    stdConsole_textAttribute = wAttributes;
    return 1;
}

int J3DAPI stdConsole_WriteConsole(const char* pText, uint32_t textAttribute)
{
    J3D_UNUSED(textAttribute);

    if ( !pText )
    {
        return 0;
    }

    return fputs(pText, stdout) >= 0 ? 1 : 0;
}
