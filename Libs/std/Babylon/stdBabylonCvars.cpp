// Added: the game's cvars of the Babylon column (ADR-0020). The composition roots call this before
// finishStartup(), so the run's registry holds the settings the trace records; the simulation values
// reach the game through stdCvarBabylon.cpp.
#include "stdBabylonCvars.h"

#include <string>

#include <j3dcore/j3d.h>

bool stdBabylonCvars_RegisterGame(babylon::cvars::Registry& rRegistry)
{
    babylon::cvars::Desc desc;
    desc.name         = "game.profile";
    desc.description  = "Enhanced or Retail: which simulation profile the build plays";
    desc.defaultValue = std::string{ J3D_QOL_VALUE("enhanced", "retail") };
    desc.flags        = babylon::cvars::Flag::ReadOnly | babylon::cvars::Flag::Simulation;

    return rRegistry.add(desc);
}
