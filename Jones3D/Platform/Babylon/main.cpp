// Added: the Babylon column's composition root. It builds the Babylon clocks and file system,
// installs them as the adapter's context, runs the shared headless loop, then uninstalls before
// the objects die.
#include <memory>

#include <babylon/files/FileSystem.h>
#include <babylon/files/SdlFileSystem.h>
#include <babylon/time/Clock.h>
#include <babylon/time/NullClock.h>
#include <babylon/time/SdlClock.h>

#include <std/Babylon/stdBabylon.h>

#include <Jones3D/Platform/Null/JonesHeadless.h>

int main(int argc, char** argv)
{
    std::unique_ptr<babylon::files::FileSystem> pFileSystem = babylon::files::createSdlFileSystem();
    std::unique_ptr<babylon::time::Clock> pClock = babylon::time::createSdlClock();
    babylon::time::NullClock virtualClock;
    StdBabylonContext context{ pClock.get(), &virtualClock, pFileSystem.get() };
    stdBabylon_SetContext(&context);
    int exitCode = JonesHeadless_Run(argc, argv);
    stdBabylon_SetContext(nullptr);
    return exitCode;
}
