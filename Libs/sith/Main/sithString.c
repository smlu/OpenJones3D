#include "sithMain.h"
#include "sithString.h"

#include <std/General/stdMemory.h>
#include <std/General/stdStrTable.h>

#include <wchar.h>

static tStringTable sithStringsTbl;
static tStringTable voiceStringsTbl;
static bool bStartup = false;

void sithString_ResetGlobals(void)
{
}

int sithString_Startup(void)
{
    if ( bStartup )
    {
        SITHLOG_STATUS("sithString startup -- already started!\n");
        return 0;
    }

    if ( !stdStrTable_Load(&voiceStringsTbl, "misc\\voiceStrings.uni")
           && !stdStrTable_Load(&voiceStringsTbl, "\\Jones3D\\resource\\misc\\voiceStrings.uni") )
    {
        SITHLOG_ERROR("Error opening stringtable 'misc\\voiceStrings.uni'.\n");
        return 1;
    }
    else if ( !stdStrTable_Load(&sithStringsTbl, "misc\\sithStrings.uni")
           && !stdStrTable_Load(&sithStringsTbl, "\\Jones3D\\resource\\misc\\sithStrings.uni") )
    {
        SITHLOG_ERROR("Error opening stringtable 'misc\\sithStrings.uni'.\n");
        return 1;
    }

    // Success
    bStartup = true;
    return 0;
}

void sithString_Shutdown(void)
{
    if ( bStartup )
    {
        stdStrTable_Free(&voiceStringsTbl);
        stdStrTable_Free(&sithStringsTbl);
    }

    bStartup = false;
}

// Found in dbg version
wchar_t* J3DAPI sithString_sub_44D5CA(const char* pStr)
{
    SITH_ASSERTREL(bStartup);
    if ( !pStr )
    {
        return NULL;
    }

    if ( strncmp("SITH", pStr, 4u) == 0 )
    {
        return stdStrTable_GetValue(&sithStringsTbl, pStr);
    }

    return stdStrTable_GetValue(&voiceStringsTbl, pStr);
}

wchar_t* J3DAPI sithString_GetString(const char* pStr)
{
    SITH_ASSERTREL(bStartup);
    if ( pStr )
    {
        if ( !strncmp("SITH", pStr, 4u) )
        {
            return stdStrTable_GetValue(&sithStringsTbl, pStr);
        }
        else
        {
            return stdStrTable_GetValueOrKey(&voiceStringsTbl, pStr);
        }
    }
    else
    {
        wchar_t* pRetStr  = (wchar_t*)STDMALLOC(sizeof(wchar_t));
        memset(pRetStr, 0, sizeof(wchar_t));
        return pRetStr;
    }
}
