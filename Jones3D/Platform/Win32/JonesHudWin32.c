#include <std/Win95/stdSysTagsWin32.h>
#include <windows.h>
#include <shellapi.h> // Needed for SHELLEXECUTEINFOA, ShellExecuteA

#include <Jones3D/Display/JonesHudSys.h>

#include <Jones3D/Main/jonesString.h>

#include <sith/Main/sithString.h>

#include <std/General/stdConfig.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

// The Windows help viewer launcher and process check, moved out of Jones3D/Display/JonesHud.c.

tSysHandle J3DAPI JonesHud_OpenHelp(tSysHandle process)
{
    J3D_UNUSED(process);

    wchar_t* pwString = sithString_GetString("SITHSTRING_HELPFILE");
    HANDLE hProcess = NULL;
    if ( !pwString )
    {
        return hProcess;
    }

    char* pFilename = stdUtil_ToAString(pwString);
    char aPath[128] = { 0 };
    stdConfig_GetString(SITH_CFG_INSTALLPATH, aPath, sizeof(aPath), "");
    if ( pFilename && strlen(aPath) )
    {
        SHELLEXECUTEINFOA execInfo = { 0 };
        execInfo.cbSize      = 60;
        execInfo.fMask       = 80;
        execInfo.lpVerb      = "open";
        execInfo.lpFile      = pFilename;
        execInfo.lpDirectory = aPath;
        execInfo.nShow       = 1;
        ShellExecuteExA(&execInfo);
        hProcess = execInfo.hProcess;
    }

    if ( pFilename )
    {
        STDFREE(pFilename);
    }

    return hProcess;
}

// True while the help viewer process has not exited yet; matches the old GetExitCodeProcess check.
bool JonesHudSys_IsProcessRunning(tSysHandle hProcess)
{
    DWORD exitCode;
    GetExitCodeProcess(hProcess, &exitCode);
    return exitCode == STILL_ACTIVE;
}
