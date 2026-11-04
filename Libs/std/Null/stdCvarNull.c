// Added: the Null column's simulation settings of the run (ADR-0020). There are no runtime cvars
// yet, so this is the fixed list of what the build's simulation profile is; turning the compile-time
// enhancement switches into runtime cvars is a later milestone. The Babylon column answers the same
// two functions from the babylon::cvars::Registry of the adapter context.
#include <j3dcore/j3d.h>

#include <string.h>

#include <std/General/stdUtil.h>
#include <std/Null/stdCvarNull.h>

// The simulation cvars, sorted by name: the order stdCvar_GetSimulation reports them by index.
static const struct
{
    const char* pName;
    const char* pValue;
} stdCvarNull_aSimulation[] =
{
    // The profile this build plays, which is still the compile-time QOL switch.
    { "game.profile", J3D_QOL_VALUE("enhanced", "retail") },
};

size_t stdCvar_GetNumSimulation(void)
{
    return STD_ARRAYLEN(stdCvarNull_aSimulation);
}

bool stdCvar_GetSimulation(size_t index, char* pName, size_t nameSize, char* pValue, size_t valueSize)
{
    if ( index >= STD_ARRAYLEN(stdCvarNull_aSimulation) || !pName || !pValue )
    {
        return false;
    }

    const char* pNameText  = stdCvarNull_aSimulation[index].pName;
    const char* pValueText = stdCvarNull_aSimulation[index].pValue;
    const size_t nameLen   = strlen(pNameText);
    const size_t valueLen  = strlen(pValueText);

    // Both texts have to fit before anything is written, so a refusal leaves both buffers alone.
    if ( nameLen >= nameSize || valueLen >= valueSize )
    {
        return false;
    }

    memcpy(pName, pNameText, nameLen + 1);
    memcpy(pValue, pValueText, valueLen + 1);
    return true;
}
