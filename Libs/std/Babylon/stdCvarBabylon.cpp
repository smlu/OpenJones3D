// Added: the Babylon column's simulation settings of the run (ADR-0020): the same two functions the
// Null column answers from a fixed table, read here from the context's babylon::cvars::Registry.
// The registry holds every cvar of the run; this seam hands out only the ones flagged Simulation, in
// the name order the registry already keeps.
#include <babylon/cvars/Registry.h>

#include <cstddef>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <std/Null/stdCvarNull.h>

#include "stdBabylon.h"

size_t stdCvar_GetNumSimulation(void)
{
    StdBabylonContext& rContext = stdBabylon_RequireContext("stdCvar_GetNumSimulation");
    return rContext.pCvars->simulationValues().size();
}

bool stdCvar_GetSimulation(size_t index, char* pName, size_t nameSize, char* pValue, size_t valueSize)
{
    StdBabylonContext& rContext = stdBabylon_RequireContext("stdCvar_GetSimulation");
    const std::vector<std::pair<std::string, std::string>> values = rContext.pCvars->simulationValues();
    if ( index >= values.size() || !pName || !pValue )
    {
        return false;
    }

    // Both texts have to fit before anything is written, so a refusal leaves both buffers alone.
    const std::string& rName  = values[index].first;
    const std::string& rValue = values[index].second;
    if ( rName.size() >= nameSize || rValue.size() >= valueSize )
    {
        return false;
    }

    memcpy(pName, rName.c_str(), rName.size() + 1);
    memcpy(pValue, rValue.c_str(), rValue.size() + 1);
    return true;
}
