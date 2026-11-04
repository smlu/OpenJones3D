// Added: the game's own cvars (ADR-0020): what the composition roots register into the run's
// babylon::cvars::Registry, one entry per thing the simulation profile fixes.
#ifndef STD_STDBABYLONCVARS_H
#define STD_STDBABYLONCVARS_H
#ifndef __cplusplus
#error "stdBabylonCvars.h is C++ only: it names babylon::cvars::Registry."
#endif

#include <babylon/cvars/Registry.h>

// Registers the game's cvars into `rRegistry`, which the caller owns and must outlive the run.
// Returns Registry::add's result: false when a name is invalid or already registered, which the
// registry logs. The caller calls finishStartup() afterwards.
bool stdBabylonCvars_RegisterGame(babylon::cvars::Registry& rRegistry);

#endif // STD_STDBABYLONCVARS_H
