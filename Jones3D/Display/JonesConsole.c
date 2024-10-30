#include "JonesConsole.h"
#include <j3dcore/j3dhook.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/RTI/symbols.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Primitives/rdFont.h>

#include <sith/Engine/sithRender.h>
#include <sith/Devices/sithConsole.h>

#include <std/General/std.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>

static bool JonesConsole_bOpened = false;

size_t JonesConsole_nextIndex = 0;
JonesConsoleTextLine JonesConsole_aBuffers[15] = { 0 };

static size_t JonesConsole_cursorPos;
static char JonesConsole_aCmdLine[128] = { 0 };

static char JonesConsole_aUnused_552770[2304]; // Unused

static rdFont* JonesConsole_pFont = NULL;
static tHashTable* pFuncHashtbl   = NULL;


void JonesConsole_InstallHooks(void)
{
    J3D_HOOKFUNC(JonesConsole_Startup);
    J3D_HOOKFUNC(JonesConsole_Shutdown);
    J3D_HOOKFUNC(JonesConsole_Open);
    J3D_HOOKFUNC(JonesConsole_Close);
    J3D_HOOKFUNC(JonesConsole_FlushToDisplay);
    J3D_HOOKFUNC(JonesConsole_PrintText);
    J3D_HOOKFUNC(JonesConsole_PrintTextWithID);
    J3D_HOOKFUNC(JonesConsole_ClearText);
    J3D_HOOKFUNC(JonesConsole_ExeCommand);
    J3D_HOOKFUNC(JonesConsole_ShowConsole);
    J3D_HOOKFUNC(JonesConsole_HandelChar);
    J3D_HOOKFUNC(JonesConsole_FlushCommandLine);
    J3D_HOOKFUNC(JonesConsole_HideConsole);
    J3D_HOOKFUNC(JonesConsole_PrintVersion);
    J3D_HOOKFUNC(JonesConsole_PrintFramerate);
    J3D_HOOKFUNC(JonesConsole_PrintPolys);
    J3D_HOOKFUNC(JonesConsole_ShowEndCredits);
    J3D_HOOKFUNC(JonesConsole_RemoveExpired);
}

void JonesConsole_ResetGlobals(void)
{
    memset(&JonesConsole_g_bVisible, 0, sizeof(JonesConsole_g_bVisible));
    memset(&JonesConsole_g_bStarted, 0, sizeof(JonesConsole_g_bStarted));
}

int J3DAPI JonesConsole_Startup()
{
    STD_ASSERTREL(!JonesConsole_g_bStarted);
    if ( sithConsole_Startup() )
    {
        return 1;
    }

    sithConsole_RegisterPrintFunctions(JonesConsole_PrintText, 0, JonesConsole_FlushToDisplay);

    memset(JonesConsole_aUnused_552770, 0, sizeof(JonesConsole_aUnused_552770));// TODO: ??

    JonesConsole_pFont      = NULL;
    JonesConsole_g_bStarted = 1;
    JonesConsole_bOpened    = false;

    return 0;
}

void JonesConsole_Shutdown(void)
{
    if ( JonesConsole_bOpened )
    {
        JonesConsole_Close();
    }

    sithConsole_Shutdown();
    JonesConsole_g_bStarted = 0;
}

int JonesConsole_Open(void)
{
    if ( JonesConsole_bOpened )
    {
        return 0;
    }

    if ( sithConsole_Open(16, 64) )
    {
        return 1;
    }

    pFuncHashtbl = stdHashtbl_New(64);// ??? seems like this is created for no reason
    STD_ASSERTREL(pFuncHashtbl);

    sithConsole_RegisterCommand(JonesConsole_PrintVersion, "version", 0);
    sithConsole_RegisterCommand(JonesConsole_PrintFramerate, "framerate", 0);
    sithConsole_RegisterCommand(JonesConsole_PrintPolys, "polys", 0);
    sithConsole_RegisterCommand(JonesConsole_ShowEndCredits, "endcredit", 0);

    JonesConsole_nextIndex = 0;

    JonesConsole_pFont = rdFont_Load("mat\\jonesBook Antiqua24.gcf");
    if ( JonesConsole_pFont )
    {
        JonesConsole_bOpened = true;
    }

    return 0;
}

void JonesConsole_Close(void)
{
    if ( JonesConsole_bOpened )
    {
        sithConsole_Close();
        if ( JonesConsole_pFont )
        {
            rdFont_Free(JonesConsole_pFont);
            JonesConsole_pFont = NULL;
        }

        if ( pFuncHashtbl )
        {
            stdHashtbl_Free(pFuncHashtbl);
            pFuncHashtbl = NULL;
        }

        JonesConsole_bOpened = false;
    }
}

void JonesConsole_FlushToDisplay(void)
{
    if ( JonesConsole_pFont )
    {
        JonesConsole_RemoveExpired();

        uint32_t width, height;
        stdDisplay_GetBackBufferSize(&width, &height);

        float x = 4.0f / (float)width;
        float y = 0.0f; // TODO: add initial small top padding

        rdFontColor textColor;
        rdVector_Set4(&textColor[0], 1.0f, 0.0f, 0.0f, 1.0f);
        rdVector_Set4(&textColor[1], 1.0f, 0.0f, 0.0f, 1.0f);
        rdVector_Set4(&textColor[2], 1.0f, 1.0f, 0.0f, 1.0f);
        rdVector_Set4(&textColor[3], 1.0f, 1.0f, 0.0f, 1.0f);
        rdFont_SetFontColor(textColor);

        unsigned char aPrintText[256] = { 0 };
        for ( size_t i = 0; i < JonesConsole_nextIndex && i < STD_ARRAYLEN(JonesConsole_aBuffers); ++i )
        {
            const unsigned char* pText = (unsigned char*)JonesConsole_aBuffers[i].aLine;
            while ( pText )
            {
                const unsigned char* pEnd = rdFont_GetWrapLine(pText, JonesConsole_pFont, /*widthScale=*/1.0f);
                if ( pEnd == pText )
                {
                    pEnd = NULL;
                }

                if ( pEnd )
                {
                    memset(aPrintText, 0, sizeof(aPrintText));
                    stdUtil_StringNumCopy((char*)aPrintText, sizeof(aPrintText), (const char*)pText, pEnd - pText);
                    rdFont_DrawTextLine(aPrintText, x, y, rdCamera_g_pCurCamera->pFrustum->nearPlane, JonesConsole_pFont, RDFONT_ALIGNLEFT);
                    pText = pEnd;
                }
                else
                {
                    rdFont_DrawTextLine(pText, x, y, rdCamera_g_pCurCamera->pFrustum->nearPlane, JonesConsole_pFont, RDFONT_ALIGNLEFT);
                    pText = NULL;
                }

                y = rdFont_GetNormLineSpacing(JonesConsole_pFont) + y;
            }
        }
    }
}

size_t J3DAPI JonesConsole_PrintText(const char* pText)
{
    if ( JonesConsole_nextIndex >= STD_ARRAYLEN(JonesConsole_aBuffers) )
    {
        size_t index = 0;
        for ( size_t i = 0; i < STD_ARRAYLEN(JonesConsole_aBuffers); ++i )
        {
            if ( JonesConsole_aBuffers[i].msecExpireTime != -1 )
            {
                index = i;
                break;
            }
        }

        JonesConsole_aBuffers[index].msecExpireTime = 0;
        JonesConsole_RemoveExpired();
    }

    size_t nextIndex = JonesConsole_nextIndex;
    STD_ASSERTREL((((nextIndex) >= (0)) ? (((nextIndex) <= (STD_ARRAYLEN(JonesConsole_aBuffers) - 1)) ? 1 : 0) : 0));

    STD_STRCPY(JonesConsole_aBuffers[nextIndex].aLine, pText);
    JonesConsole_aBuffers[nextIndex].ID = 0;
    JonesConsole_aBuffers[nextIndex].msecExpireTime = stdPlatform_GetTimeMsec() + 10000;
    ++JonesConsole_nextIndex;
    return nextIndex;
}

void J3DAPI JonesConsole_PrintTextWithID(int ID, const char* pText)
{
    // Check if existing id is in the queue
    bool bExists = false;
    for ( size_t i = 0; i < JonesConsole_nextIndex && !bExists; ++i )
    {
        if ( JonesConsole_aBuffers[i].ID == ID )
        {
            // Replace existing text
            if ( strcmp(JonesConsole_aBuffers[i].aLine, pText) != 0 ) { // if 
                STD_STRCPY(JonesConsole_aBuffers[i].aLine, pText);
            }

            bExists = true;
            // TODO: probably should break here
        }
    }

    if ( !bExists )
    {
        int idx = JonesConsole_PrintText(pText);
        JonesConsole_aBuffers[idx].ID = ID;
        JonesConsole_aBuffers[idx].msecExpireTime = (size_t)-1;
    }
}

void J3DAPI JonesConsole_ClearText(int yourID)
{
    STD_ASSERTREL(yourID != 0);
    size_t i = 0;
    for ( i = 0; i < JonesConsole_nextIndex && JonesConsole_aBuffers[i].ID != yourID; ++i )
    {
        ;
    }

    if ( i < JonesConsole_nextIndex )
    {
        JonesConsole_aBuffers[i].msecExpireTime = 0;
    }
}

int J3DAPI JonesConsole_ExeCommand(const char* pLine)
{
    STD_ASSERTREL(pLine);

    char aLine[128] = { 0 };
    STD_STRCPY(aLine, pLine);
    stdUtil_ToLower(aLine);

    // Extract command from input string line
    char* pntok = NULL;
    const char* pCommand = strtok_r(aLine, ", \t\n\r", &pntok);
    if ( !pCommand ) {
        return 0;
    }

    SithConsoleCommand* pFunc = (SithConsoleCommand*)stdHashtbl_Find(pFuncHashtbl, pCommand);
    if ( !pFunc ) {
        return 0;
    }

    STD_ASSERTREL(pFunc->pfFunc);

    // Extract function args & Exec function
    const char* pArg = strtok_r(NULL, "\n\r", &pntok);
    pFunc->pfFunc(pFunc, pArg);
    return 1;
}

void J3DAPI JonesConsole_ShowConsole()
{
    JonesConsole_g_bVisible = 1;

    // Print cursor at the start 
    stdUtil_StringNumCopy(JonesConsole_aCmdLine, sizeof(JonesConsole_aCmdLine), "> ", 3u); // Changed: Changed cursor from ']' to '>'
    JonesConsole_cursorPos = 2;

    stdControl_SetActivation(0);
    JonesConsole_FlushCommandLine();
}

void J3DAPI JonesConsole_HandelChar(char chr)
{
    if ( chr == '\r' ) // if enter key pressed
    {
        if ( (unsigned int)JonesConsole_cursorPos > 2 && !JonesConsole_ExeCommand(&JonesConsole_aCmdLine[2]) ) {
            sithConsole_ExeCommand(&JonesConsole_aCmdLine[2]);
        }
        JonesConsole_HideConsole();
    }
    else
    {
        if ( chr == '\b' ) // if backspace key pressed
        {
            if ( (unsigned int)JonesConsole_cursorPos > 2 ) {
                JonesConsole_aCmdLine[--JonesConsole_cursorPos] = 0;
            }
        }
        else if ( (unsigned int)JonesConsole_cursorPos < 127 )
        {
            JonesConsole_aCmdLine[JonesConsole_cursorPos] = chr;
            JonesConsole_aCmdLine[++JonesConsole_cursorPos] = 0;
        }

        JonesConsole_FlushCommandLine();
    }
}

void JonesConsole_FlushCommandLine(void)
{
    JonesConsole_PrintTextWithID(103, JonesConsole_aCmdLine);
}

void JonesConsole_HideConsole(void)
{
    JonesConsole_cursorPos = 0;
    JonesConsole_g_bVisible = 0;
    JonesConsole_ClearText(103);
    stdControl_SetActivation(1);
}

int J3DAPI JonesConsole_PrintVersion(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);
    sithConsole_PrintString(J3D_VERSION_FULL);
    return 1;
}

int J3DAPI JonesConsole_PrintFramerate(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    JonesConsole_ClearText(102);
    JonesMain_TogglePrintFramerate();
    return 1;
}

int J3DAPI JonesConsole_PrintPolys(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    STD_FORMAT(std_g_genBuffer, "Arch polys: %d", sithRender_g_numArchPolys);
    sithConsole_PrintString(std_g_genBuffer);

    STD_FORMAT(std_g_genBuffer, "Alpha Arch polys: %d", sithRender_g_numAlphaArchPolys);
    sithConsole_PrintString(std_g_genBuffer);

    STD_FORMAT(std_g_genBuffer, "Thing polys: %d", sithRender_g_numThingPolys);
    sithConsole_PrintString(std_g_genBuffer);

    STD_FORMAT(std_g_genBuffer, "Alpha Thing polys: %d", sithRender_g_numAlphaThingPoly);
    sithConsole_PrintString(std_g_genBuffer);
    return 1;
}

int J3DAPI JonesConsole_ShowEndCredits(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);
    JonesMain_ShowEndCredits();
    return 1;
}

int J3DAPI JonesConsole_RemoveExpired()
{
    size_t nextIdx = 0;
    size_t time = stdPlatform_GetTimeMsec();
    int bFound = 0;
    for ( size_t i = 0; i < JonesConsole_nextIndex; ++i )
    {
        if ( JonesConsole_aBuffers[i].msecExpireTime > time )
        {
            if ( nextIdx != i )
            {
                //strncpy(JonesConsole_aBuffers[nextIdx].aLine, JonesConsole_aBuffers[i].aLine, 0x80u);
                STD_STRCPY(JonesConsole_aBuffers[nextIdx].aLine, JonesConsole_aBuffers[i].aLine);

                JonesConsole_aBuffers[nextIdx].msecExpireTime = JonesConsole_aBuffers[i].msecExpireTime;
                JonesConsole_aBuffers[nextIdx].ID = JonesConsole_aBuffers[i].ID;
                bFound = 1;
            }

            ++nextIdx;
        }
    }

    JonesConsole_nextIndex = nextIdx;
    return bFound;
}