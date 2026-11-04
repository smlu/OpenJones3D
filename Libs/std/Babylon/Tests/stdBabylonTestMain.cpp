// Added: the Babylon column's game-test entry point. std_use_babylon_test_main() adds it to
// every native test executable that reaches std's file and time services: it installs the
// adapter context (the real SDL file system, an SDL clock, a NullClock, a NullPlatform and the run's
// cvar registry), calls the Unity runner's renamed entry (J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main in
// the runner source) and uninstalls before the objects die. The suites themselves are unchanged.

// The sith vector executables force-include a header that replaces the C runtime's rand through
// a macro for their capture records. The force-include is prepended to this file too, and the
// macro would break the C++ standard headers; this entry never draws a random number, so the
// macro is removed again before anything else is read.
#ifdef rand
#undef rand
#endif

#include <cstdio>
#include <memory>

#include <babylon/cvars/Registry.h>
#include <babylon/files/FileSystem.h>
#include <babylon/files/SdlFileSystem.h>
#include <babylon/platform/NullPlatform.h>
#include <babylon/time/Clock.h>
#include <babylon/time/NullClock.h>
#include <babylon/time/SdlClock.h>

#include <std/Babylon/stdBabylon.h>
#include <std/Babylon/stdBabylonCvars.h>

// The runner source compiled with J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main.
extern "C" int j3dTestRunner_Main(int argc, const char* argv[]);

int main(int argc, char** argv)
{
    std::unique_ptr<babylon::files::FileSystem> pFileSystem = babylon::files::createSdlFileSystem();
    std::unique_ptr<babylon::time::Clock> pClock = babylon::time::createSdlClock();
    babylon::time::NullClock virtualClock;
    // Added: the game tests read no OS input, but the context contract needs a platform; the
    // NullPlatform stands in, and as the script platform it is the queue --input would use.
    babylon::platform::NullPlatform platform;
    // Added: the run's cvars, registered like the composition roots do, so a test that reads the
    // simulation values through stdCvarNull.h sees the same cvars the game would.
    babylon::cvars::Registry cvars;
    if ( !stdBabylonCvars_RegisterGame(cvars) )
    {
        fprintf(stderr, "ERROR: stdBabylonTestMain: could not register the game cvars!\n");
        return 1;
    }

    cvars.finishStartup();
    // Added: no game window in the game tests: the context's pWindow stays null.
    StdBabylonContext context{ pClock.get(), &virtualClock, pFileSystem.get(), &platform, &platform, nullptr, &cvars };
    stdBabylon_SetContext(&context);

    // C++ refuses the char** to const char** qualification change that C accepts for main's argv.
    int exitCode = j3dTestRunner_Main(argc, const_cast<const char**>(argv));

    stdBabylon_SetContext(nullptr);
    return exitCode;
}
