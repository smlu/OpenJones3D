// Added: the windowed composition root of the Babylon column. It builds the SDL platform and the
// game window, the SDL clock, the Null virtual clock (never latched: the windowed run uses the
// real clock), the SDL file system and the run's cvar registry, installs them as the adapter context
// together with the window, runs the windowed game loop, then uninstalls before the objects die. It
// renders nothing yet: the display and 3D backends stay Null until M6.
#include <cstdio>
#include <memory>

#include <babylon/cvars/Registry.h>
#include <babylon/files/FileSystem.h>
#include <babylon/files/SdlFileSystem.h>
#include <babylon/platform/Platform.h>
#include <babylon/platform/SdlPlatform.h>
#include <babylon/platform/Window.h>
#include <babylon/time/Clock.h>
#include <babylon/time/NullClock.h>
#include <babylon/time/SdlClock.h>

#include <std/Babylon/stdBabylon.h>
#include <std/Babylon/stdBabylonCvars.h>

#include <Jones3D/Platform/Babylon/JonesBabylon.h>

int main(int argc, char** argv)
{
    std::unique_ptr<babylon::platform::Platform> pPlatform = babylon::platform::createSdlPlatform();
    if ( !pPlatform )
    {
        fprintf(stderr, "ERROR: Jones3D: could not create the SDL platform!\n");
        return 1;
    }

    const babylon::platform::WindowDesc desc{ "Jones3D EX", { 640, 480 }, false };
    std::unique_ptr<babylon::platform::Window> pWindow = pPlatform->createWindow(desc);
    if ( !pWindow )
    {
        fprintf(stderr, "ERROR: Jones3D: could not create the game window!\n");
        return 1;
    }

    std::unique_ptr<babylon::files::FileSystem> pFileSystem = babylon::files::createSdlFileSystem();
    std::unique_ptr<babylon::time::Clock> pClock = babylon::time::createSdlClock();
    babylon::time::NullClock virtualClock;
    // Added: the run's cvars (ADR-0020), registered before finishStartup() like in the headless root,
    // so every run of this column has a cvar registry in its context.
    babylon::cvars::Registry cvars;
    if ( !stdBabylonCvars_RegisterGame(cvars) )
    {
        fprintf(stderr, "ERROR: Jones3D: could not register the game cvars!\n");
        return 1;
    }

    cvars.finishStartup();

    // The command line behind argv[0] and the --load option belong to the run: JonesBabylon_Run
    // takes main's arguments directly, like the headless entry. The windowed run has no script
    // platform: every event comes from the SDL platform itself.
    StdBabylonContext context{ pClock.get(), &virtualClock, pFileSystem.get(), pPlatform.get(),
        nullptr, pWindow.get(), &cvars };
    stdBabylon_SetContext(&context);
    int exitCode = JonesBabylon_Run(argc, argv);
    stdBabylon_SetContext(nullptr);

    // Scope end destroys pWindow before pPlatform, the order the Platform interface requires.
    return exitCode;
}
