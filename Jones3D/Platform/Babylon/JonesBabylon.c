// Added: the windowed run of the Babylon column, the wkernel_Run sequence without the Win32
// parts: argument handling, global reset, activation callback, Startup, then JonesMain_Process()
// until the engine quits or a quit or window close ends the pump, then Shutdown. It is the
// JonesHeadless_Run of the windowed entry: the retail mutex, the Win32 signal handler, the window
// creation and the stdWin95 window/instance/Guid registration stay behind, and the events now come
// from the Babylon platform through wkernel_PeekProcessEvents and wkernel_ProcessEvents. The
// result handling is wkernel_Run's, not the headless runner's: the result 1 it ends the loop on
// (the WM_QUIT a quit request or a closed window turns into) returns 0 from wkernel_Run, only the
// engine's -1 ends nonzero.
#include <stdio.h>
#include <string.h>

#include <Jones3D/Main/JonesGlobals.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Main/JonesMainSys.h>
#include <Jones3D/Platform/Babylon/JonesBabylon.h>
#include <Jones3D/Platform/Null/JonesPlatformNull.h>

#include <wkernel/wkernel.h>

// Added: the command line handed to JonesMain_Startup, the shape of the Windows lpCmdLine, in a
// fixed buffer like the engine's command line buffers (JonesConsole.c's 128 char line).
#define JONESBABYLON_MAX_CMDLINE 128

// Added: the game window's focus changes, handed over by the Babylon wkernel, reach the game
// the way WM_ACTIVATEAPP and WM_ACTIVATE reach it on Windows. hWnd stays NULL: the window
// handle is the composition root's, and every JonesMainSys use of it is a no-op here.
static void J3DAPI JonesBabylon_OnActivate(int bActive)
{
    JonesMain_OnAppActivate(NULL, bActive);
}

int JonesBabylon_Run(int argc, char** argv)
{
    // Added: --load <save.nds> answers the Null load-game dialog, like the headless runner's
    // option; everything else joins into the startup command line. Both are parsed completely
    // before anything runs, so a bad command line exits before any engine code, the way
    // JonesHeadless_Run parses its --input script up front.
    const char* pLoadPath = NULL;
    char        aCmdLine[JONESBABYLON_MAX_CMDLINE] = { 0 };
    size_t      curLen  = 0;
    for ( int i = 1; i < argc; ++i )
    {
        if ( strcmp(argv[i], "--load") == 0 )
        {
            if ( i + 1 >= argc )
            {
                fprintf(stderr, "ERROR: Jones3D: --load needs a value!\n");
                return 1;
            }

            pLoadPath = argv[++i];
            continue;
        }

        size_t argLen = strlen(argv[i]);
        if ( curLen )
        {
            aCmdLine[curLen++] = ' ';
        }

        if ( curLen + argLen >= sizeof(aCmdLine) )
        {
            fprintf(stderr, "ERROR: Jones3D: command line longer than %d characters!\n",
                JONESBABYLON_MAX_CMDLINE - 1);
            return 1;
        }

        memcpy(&aCmdLine[curLen], argv[i], argLen);
        curLen += argLen;
        aCmdLine[curLen] = '\0';
    }

    // WinMain/DllMain: the global reset before anything else runs.
    Jones3D_ResetAllGlobals();

    // Indy3D_WinMain: hand the --load path to the Null load-game dialog, which JonesMain_Startup
    // asks when Jones.cfg is in the load-game start mode.
    if ( pLoadPath )
    {
        JonesPlatformNull_SetLoadPath(pLoadPath);
    }

    // wkernel_Run: the window adapter hands the game window's focus changes to the game.
    wkernel_SetActivateCallback(JonesBabylon_OnActivate);

    // wkernel_Run: the Startup callback (the WinMain.c Startup wrapper's logic).
    if ( JonesMain_Startup(aCmdLine) )
    {
        JonesMain_Shutdown();
        fprintf(stderr, "ERROR: Jones3D: JonesMain_Startup failed, exiting nonzero!\n");
        return 1;
    }

    // wkernel_Run: the main loop until JonesMain_Process() answers, with its result handling and
    // its return codes: -1 (engine quit) ends the run nonzero, 1 (finish: a quit request or a
    // closed window turned into the WM_QUIT answer) ends it with the 0 wkernel_Run returns for it.
    int exitCode = 0;
    const char* pStopReason = NULL;
    while ( 1 )
    {
        int result = JonesMain_Process();

        if ( result == -1 )
        {
            exitCode    = 1;
            pStopReason = "JonesMain_Process() returned -1 (engine quit)";
            break;
        }

        if ( result == 1 )
        {
            exitCode    = 0;
            pStopReason = "JonesMain_Process() returned 1 (quit requested or window closed)";
            break;
        }

        if ( result )
        {
            // Added: Same answer as wkernel_Run for results it does not handle.
            fprintf(stdout, "WARNING: Jones3D: Unhandled result code: %d, skipping...\n", result);
        }
    }

    JonesMain_Shutdown();

    printf("stopped: %s\n", pStopReason ? pStopReason : "unknown reason");
    return exitCode;
}
