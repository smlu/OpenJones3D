// Added: the Babylon column's adapter context store: the one file-static pointer the adapter edge
// shares between the composition root and the backend files. No other state.
#include "stdBabylon.h"

#include <cstdio>
#include <cstdlib>

static StdBabylonContext* stdBabylon_pContext = nullptr;

void stdBabylon_SetContext(StdBabylonContext* pContext)
{
    stdBabylon_pContext = pContext;
}

StdBabylonContext* stdBabylon_GetContext()
{
    return stdBabylon_pContext;
}

StdBabylonContext& stdBabylon_RequireContext(const char* pFunction)
{
    if ( !stdBabylon_pContext )
    {
        fprintf(stderr, "ERROR: %s: no Babylon platform context installed!\n", pFunction);
        abort();
    }

    return *stdBabylon_pContext;
}
