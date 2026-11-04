#include "jonesString.h"

#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdStrTable.h>
#include <std/General/stdUtil.h>

static tStringTable jonesString_jonesStringsTbl;
static char jonesString_aBuffer[512] = { 0 };
static bool jonesString_bStarted = false;

void jonesString_ResetGlobals(void)
{
    //memset(&jonesString_jonesStringsTbl, 0, sizeof(jonesString_jonesStringsTbl));
    //memset(&jonesString_aBuffer, 0, sizeof(jonesString_aBuffer));
    //memset(&jonesString_bStarted, 0, sizeof(jonesString_bStarted));
}

const char* J3DAPI jonesString_GetString(const char* pKey)
{
    if ( !jonesString_bStarted ) {
        return NULL;
    }

    if ( !pKey ) {
        return NULL;
    }

    wchar_t* pWString = stdStrTable_GetValueOrKey(&jonesString_jonesStringsTbl, pKey);
    if ( !pWString )
    {
        STD_STRCPY(jonesString_aBuffer, pKey);
        return jonesString_aBuffer;
    }

    char* pAString = stdUtil_ToAString(pWString);
    STD_STRCPY(jonesString_aBuffer, pAString);
    stdMemory_Free(pAString);
    return jonesString_aBuffer;
}

int J3DAPI jonesString_Startup()
{
    if ( jonesString_bStarted ) {
        return 0;
    }

    if ( !stdStrTable_Load(&jonesString_jonesStringsTbl, "misc\\jonesStrings.uni") )
    {
        STDLOG_STATUS("couldn't find file misc\\jonesStrings.uni\n");
        return 1;
    }

    jonesString_bStarted = true;
    return 0;
}

void jonesString_Shutdown(void)
{
    if ( jonesString_bStarted )
    {
        stdStrTable_Free(&jonesString_jonesStringsTbl);
        jonesString_bStarted = false;
    }
}
