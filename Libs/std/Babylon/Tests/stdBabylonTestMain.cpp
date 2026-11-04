// Added: the Babylon column's game-test entry point. std_use_babylon_test_main() adds it to
// every native test executable that reaches std's file and time services: it installs the
// adapter context (the real SDL file system, an SDL clock, a NullClock and a NullPlatform), calls
// the Unity runner's renamed entry (J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main in the runner source)
// and uninstalls before the objects die. The suites themselves are unchanged.

// The sith vector executables force-include a header that replaces the C runtime's rand through
// a macro for their capture records. The force-include is prepended to this file too, and the
// macro would break the C++ standard headers; this entry never draws a random number, so the
// macro is removed again before anything else is read.
#ifdef rand
#undef rand
#endif

#include <memory>

#include <babylon/files/FileSystem.h>
#include <babylon/files/SdlFileSystem.h>
#include <babylon/platform/NullPlatform.h>
#include <babylon/time/Clock.h>
#include <babylon/time/NullClock.h>
#include <babylon/time/SdlClock.h>

#include <std/Babylon/stdBabylon.h>

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
    StdBabylonContext context{ pClock.get(), &virtualClock, pFileSystem.get(), &platform, &platform };
    stdBabylon_SetContext(&context);

    // C++ refuses the char** to const char** qualification change that C accepts for main's argv.
    int exitCode = j3dTestRunner_Main(argc, const_cast<const char**>(argv));

    stdBabylon_SetContext(nullptr);
    return exitCode;
}
