// sithThing.c as tested by sithThingQuetzVecTests: compiled with the seam renames and redirected engine globals of
// the suite (see Libs/sith/Tests/Vec/CMakeLists.txt) and with access to the file statics the Quetz functions keep
// between frames.
#include "../sithThing.c"

bool* sithThingVecTest_GetQuetzTurningSharp(void)
{
    return &sithThing_bQuetzTurningSharp;
}

SithQuetzAttachInfo** sithThingVecTest_GetPrevQuetzAttachInfo(void)
{
    return &sithThing_prevQuetzAttachInfo;
}

SithQuetzAttachInfo** sithThingVecTest_GetCurQuetzAttachInfo(void)
{
    return &sithThing_curQuetzAttachInfo;
}
