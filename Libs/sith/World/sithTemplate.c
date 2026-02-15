#include "sithTemplate.h"
#include <j3dcore/j3dhook.h>

#include <sith/RTI/symbols.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <rdroid/Engine/rdThing.h>

#define SITHTEMPLAT_MAXFILETEMPLATES   3072u
#define SITHTEMPLAT_MAXCACHEDTEMPLATES 1024u

typedef struct sSithTemplate
{
    const char* pName;
    const char* pDescription;
    const char* pText;
    const char* pBasedOn;
} SithTemplate;

static tHashTable* sithTemplate_pHashtable = NULL; // Added: Init to NULL

static size_t sithTemplate_masterFileCount       = 0; // Added: Init to 0
static SithTemplate* sithTemplate_pMasterFile    = NULL; // Added: Init to NULL
static tHashTable* sithTemplate_pMasterHashtable = NULL; // Added: Init to NULL

void J3DAPI sithTemplate_AddTemplate(SithThing* pTemplate);
int J3DAPI sithTemplate_MasterFileAddDescription(const char* pDesc); // Added: From debug version
int J3DAPI sithTemplate_MasterFileAddTemplate(const char* pName, const char* pDesc, const char* pText, const char* pBasedOn); // Added: From debug version
SithTemplate* J3DAPI sithTemplate_MasterFileGetTemplateByIndex(size_t index); // Added: From debug version
size_t sithTemplate_MasterFileCount(void); // Added: From debug version
SithTemplate* J3DAPI sithTemplate_MasterFileGetTemplate(const char* pName); // Added: From debug version

SithThing* J3DAPI sithTemplate_Parse(SithWorld* pWorld);

SithThing* J3DAPI sithTemplate_CacheFind(const char* pName);
void J3DAPI sithTemplate_CacheAdd(SithThing* pThing);
void J3DAPI sithTemplate_CacheRemove(const SithThing* pThing);
int J3DAPI sithTemplate_ParseTemplateList(SithWorld* pWorld);

void sithTemplate_InstallHooks(void)
{
    J3D_HOOKFUNC(sithTemplate_Startup);
    J3D_HOOKFUNC(sithTemplate_Shutdown);
    J3D_HOOKFUNC(sithTemplate_AllocWorldTemplates);
    J3D_HOOKFUNC(sithTemplate_GetTemplateByIndex);
    J3D_HOOKFUNC(sithTemplate_GetTemplateIndex);
    J3D_HOOKFUNC(sithTemplate_ReadThingTemplatesListText);
    J3D_HOOKFUNC(sithTemplate_WriteThingTemplatesListBinary);
    J3D_HOOKFUNC(sithTemplate_ReadThingTemplatesListBinary);
    J3D_HOOKFUNC(sithTemplate_AddTemplate);
    J3D_HOOKFUNC(sithTemplate_FreeWorldTemplates);
    J3D_HOOKFUNC(sithTemplate_Parse);
    J3D_HOOKFUNC(sithTemplate_CacheFind);
    J3D_HOOKFUNC(sithTemplate_CacheAdd);
    J3D_HOOKFUNC(sithTemplate_CacheRemove);
    J3D_HOOKFUNC(sithTemplate_ParseTemplateList);
}

void sithTemplate_ResetGlobals(void)
{}

int sithTemplate_Startup(void)
{
    sithTemplate_pHashtable = stdHashtbl_New(SITHTEMPLAT_MAXCACHEDTEMPLATES);
    return sithTemplate_pHashtable == NULL;
}

void sithTemplate_Shutdown(void)
{
    if ( sithTemplate_pHashtable )
    {
        stdHashtbl_Free(sithTemplate_pHashtable);
    }

    sithTemplate_pHashtable = NULL;
}

int J3DAPI sithTemplate_AllocWorldTemplates(SithWorld* pWorld, size_t size)
{
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aThingTemplates == NULL);
    SITH_ASSERTREL(sithTemplate_pHashtable);

    if ( !size )
    {
        return 0;
    }

    pWorld->aThingTemplates = (SithThing*)STDMALLOC(sizeof(SithThing) * size);
    if ( !pWorld->aThingTemplates )
    {
        return 1;
    }

    STD_ZEROMEM(pWorld->aThingTemplates, sizeof(SithThing) * size);

    for ( size_t i = 0; i < size; ++i )
    {
        sithThing_Reset(&pWorld->aThingTemplates[i]);
        if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
        {
            pWorld->aThingTemplates[i].idx = SITHWORLD_STATICINDEX(i);
        }
        else
        {
            pWorld->aThingTemplates[i].idx = i;
        }
    }

    pWorld->sizeThingTemplates = size;
    pWorld->numThingTemplates  = 0;
    return 0;
}

SithThing* J3DAPI sithTemplate_GetTemplateByIndex(int idx)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(idx) )
    {
        pWorld = sithWorld_g_pStaticWorld;
        idx = SITHWORLD_FROM_STATICINDEX(idx);
    }

    if ( !pWorld )
    {
        return NULL;
    }

    if ( idx >= 0 && idx < pWorld->numThingTemplates )
    {
        return &pWorld->aThingTemplates[idx];
    }

    return NULL;
}

int J3DAPI sithTemplate_GetTemplateIndex(const SithThing* pTemplate)
{
    if ( !pTemplate )
    {
        return -1;
    }

    if ( pTemplate < sithWorld_g_pCurrentWorld->aThingTemplates
        || pTemplate >= &sithWorld_g_pCurrentWorld->aThingTemplates[sithWorld_g_pCurrentWorld->numThingTemplates] )
    {
        return -1;
    }

    if ( pTemplate->type )
    {
        return pTemplate->idx;
    }

    return -1;
}

// TODO: Uncomment after stdConffile_WriteLine function is implemented.
//int J3DAPI sithTemplate_WriteThingTemplateListText(const SithWorld* pWorld)
//{
//    SithThing baseTpl = { 0 }; // Altered: Init to 0
//    baseTpl.idx       = 0;
//    baseTpl.signature = 0;
//
//    sithThing_Reset(&baseTpl);
//
//    if ( stdConffile_WriteLine("##### Templates information ####\n")
//        || stdConffile_WriteLine("Section: TEMPLATES\n")
//        || stdConffile_Printf("\nWorld templates %d\n\n", pWorld->numThingTemplates)
//        || stdConffile_WriteLine("#Name:           Based On:        Params:\n") )
//    {
//        return 1;
//    }
//
//    for ( size_t i = 0; i < pWorld->numThingTemplates; ++i )
//    {
//        SithThing* pTemplate      = &pWorld->aThingTemplates[i];
//        const SithThing* pBaseTpl = pTemplate->pTemplate;
//
//        if ( pTemplate->type == SITH_THING_FREE || pTemplate->type >= SITH_THING_NUMTYPES )
//        {
//            sith_g_pHS->pAssert(
//                "(pTemplate->type != SITH_THING_FREE) && (pTemplate->type < SITH_THING_NUMTYPES)",
//                "C:\\Jones3D\\Libs\\sith\\World\\sithTemplate.c",
//                197);
//        }
//
//        const char* pBaseName = "none";
//        if ( pBaseTpl )
//        {
//            pBaseName = pBaseTpl->aName;
//        }
//
//        if ( stdConffile_Printf("%-16s %-16s ", pTemplate->aName, pBaseName) )
//        {
//            return 1;
//        }
//
//        if ( !pBaseTpl )
//        {
//            pBaseTpl = &baseTpl;
//        }
//
//        if ( sithWrite_WriteThingArgs(pTemplate, pBaseTpl, 1) || stdConffile_WriteLine("\n") )
//        {
//            return 1;
//        }
//    }
//
//    return stdConffile_WriteLine("end\n") || stdConffile_WriteLine("################################\n\n\n");
//}

int J3DAPI sithTemplate_ReadThingTemplatesListText(SithWorld* pWorld, int bSkip)
{
    if ( bSkip )
    {
        return 1;
    }

    stdConffile_ReadArgs();
    if ( !streq(stdConffile_g_entry.aArgs[0].argValue, "world") || !streq(stdConffile_g_entry.aArgs[1].argValue, "templates") )
    {
        SITHLOG_ERROR("Parse error reading thing template list line %d.\n", stdConffile_GetLineNumber());
        return 1;
    }

    size_t numTemplates = atoi(stdConffile_g_entry.aArgs[2].argValue);
    if ( !numTemplates )
    {
        return 0;
    }

    else if ( sithTemplate_AllocWorldTemplates(pWorld, numTemplates) )
    {
        SITHLOG_ERROR("Memory allocation failure reading thing templates.\n");
        return 1;
    }

    return sithTemplate_ParseTemplateList(pWorld);
}

int J3DAPI sithTemplate_WriteThingTemplatesListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    return sithThing_WriteThingsListBinary(fh, pWorld, pWorld->numThingTemplates, pWorld->aThingTemplates);
}

int J3DAPI sithTemplate_ReadThingTemplatesListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t numThingTemplates = pWorld->numThingTemplates;
    if ( sithTemplate_AllocWorldTemplates(pWorld, numThingTemplates) )
    {
        return 1;
    }

    pWorld->numThingTemplates = numThingTemplates;
    return sithThing_ReadThingsListBinary(fh, pWorld, numThingTemplates, pWorld->aThingTemplates, sithTemplate_AddTemplate);
}

void J3DAPI sithTemplate_AddTemplate(SithThing* pTemplate)
{
    sithTemplate_CacheAdd(pTemplate);
}

int J3DAPI sithTemplate_LoadMasterFile(const char* pFilename)
{
    SITH_ASSERTREL(pFilename);
    if ( !stdConffile_Open(pFilename) )
    {
        return 1;
    }

    sithTemplate_pMasterFile = (SithTemplate*)STDMALLOC(SITHTEMPLAT_MAXFILETEMPLATES * sizeof(SithTemplate)); // 3072 * 16 - ie.: 16 - size of master file entry
    sithTemplate_pMasterHashtable = stdHashtbl_New(SITHTEMPLAT_MAXFILETEMPLATES * 2);

    if ( !sithTemplate_pMasterFile || !sithTemplate_pMasterHashtable )
    {
        stdConffile_Close();
        return 1;
    }

    STD_ZEROMEM(sithTemplate_pMasterFile, SITHTEMPLAT_MAXFILETEMPLATES * sizeof(SithTemplate));

    bool bFinished = false;
    const char* pDesc = NULL;
    while ( !bFinished && stdConffile_ReadLine() )
    {
        char aFirstToken[64];
        char* pEnd = stdUtil_StringSplit(stdConffile_g_aLine, aFirstToken, STD_ARRAYLEN(aFirstToken), " \t");
        if ( streq(aFirstToken, "end") )
        {
            bFinished = true;
        }
        else if ( streq(aFirstToken, "desc:") )
        {
            // if previous description was parsed and  no template data was found, make just entry with description
            if ( pDesc )
            {
                sithTemplate_MasterFileAddDescription(pDesc);
            }

            pDesc = stdUtil_StringDuplicate(pEnd, sith_g_pHS);
        }
        else if ( aFirstToken[0] )
        {
            if ( !pEnd )
            {
                SITHLOG_ERROR("Syntax Error in file %s on line %d\n", pFilename, stdConffile_GetLineNumber());
            }
            else
            {
                char aBasedOn[64];
                stdUtil_StringSplit(pEnd, aBasedOn, STD_ARRAYLEN(aBasedOn), " \t");
                const char* pBaseOn = stdUtil_StringDuplicate(aBasedOn, sith_g_pHS);

                const char* pText = stdUtil_StringDuplicate(stdConffile_g_aLine, sith_g_pHS);
                const char* pName = stdUtil_StringDuplicate(aFirstToken, sith_g_pHS);
                sithTemplate_MasterFileAddTemplate(pName, pDesc, pText, pBaseOn);
            }

            pDesc = NULL;
        }
    }

    stdConffile_Close();
    return 0;
}

void sithTemplate_FreeMasterFile(void)
{
    for ( size_t i = 0; i < sithTemplate_masterFileCount; ++i )
    {
        if ( sithTemplate_pMasterFile[i].pName )
        {
            STDFREE((void*)sithTemplate_pMasterFile[i].pName);
        }

        if ( sithTemplate_pMasterFile[i].pText )
        {
            STDFREE((void*)sithTemplate_pMasterFile[i].pText);
        }

        if ( sithTemplate_pMasterFile[i].pBasedOn )
        {
            STDFREE((void*)sithTemplate_pMasterFile[i].pBasedOn);
        }

        if ( sithTemplate_pMasterFile[i].pDescription )
        {
            STDFREE((void*)sithTemplate_pMasterFile[i].pDescription);
        }
    }

    if ( sithTemplate_pMasterFile )
    {
        STDFREE(sithTemplate_pMasterFile);
        sithTemplate_pMasterFile = NULL;
    }

    sithTemplate_masterFileCount = 0;
    if ( sithTemplate_pMasterHashtable )
    {
        stdHashtbl_Free(sithTemplate_pMasterHashtable);
        sithTemplate_pMasterHashtable = NULL;
    }
}

int J3DAPI sithTemplate_MasterFileAddDescription(const char* pDesc)
{
    SITH_ASSERTREL(pDesc);
    if ( sithTemplate_masterFileCount >= SITHTEMPLAT_MAXFILETEMPLATES )
    {
        return 0;
    }

    sithTemplate_pMasterFile[sithTemplate_masterFileCount++].pDescription = pDesc;
    return 1;
}

int J3DAPI sithTemplate_MasterFileAddTemplate(const char* pName, const char* pDesc, const char* pText, const char* pBasedOn)
{
    SITH_ASSERTREL(pName && pText);
    if ( sithTemplate_masterFileCount >= SITHTEMPLAT_MAXFILETEMPLATES )
    {
        return 0;
    }

    sithTemplate_pMasterFile[sithTemplate_masterFileCount].pName        = pName;
    sithTemplate_pMasterFile[sithTemplate_masterFileCount].pDescription = pDesc;
    sithTemplate_pMasterFile[sithTemplate_masterFileCount].pText        = pText;
    sithTemplate_pMasterFile[sithTemplate_masterFileCount].pBasedOn     = pBasedOn;
    stdHashtbl_Add(sithTemplate_pMasterHashtable, pName, &sithTemplate_pMasterFile[sithTemplate_masterFileCount]);

    ++sithTemplate_masterFileCount;
    return 1;
}

SithTemplate* J3DAPI sithTemplate_MasterFileGetTemplateByIndex(size_t index)
{
    return &sithTemplate_pMasterFile[index];
}

size_t sithTemplate_MasterFileCount(void)
{
    return sithTemplate_masterFileCount;
}

SithTemplate* J3DAPI sithTemplate_MasterFileGetTemplate(const char* pName)
{
    SITH_ASSERTREL(pName != NULL);

    if ( !sithTemplate_pMasterHashtable )
    {
        return NULL;
    }

    return (SithTemplate*)stdHashtbl_Find(sithTemplate_pMasterHashtable, pName);
}

void J3DAPI sithTemplate_FreeWorldTemplates(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld != NULL);

    for ( size_t i = 0; i < pWorld->numThingTemplates; ++i )
    {
        rdThing_FreeEntry(&pWorld->aThingTemplates[i].renderData);
        sithTemplate_CacheRemove(&pWorld->aThingTemplates[i]);
    }

    if ( !pWorld->aThingTemplates )
    {
        SITH_ASSERTREL(pWorld->sizeThingTemplates == 0);
    }
    else
    {
        SITH_ASSERTREL(pWorld->sizeThingTemplates > 0);

        STDFREE(pWorld->aThingTemplates);
        pWorld->aThingTemplates    = NULL;
        pWorld->sizeThingTemplates = 0;
        pWorld->numThingTemplates  = 0;
    }
}

SithThing* J3DAPI sithTemplate_GetTemplate(const char* pName)
{
    SITH_ASSERTREL(pName);

    if ( streq(pName, "none") || !*pName )
    {
        return NULL;
    }

    SithThing* pTemplate = sithTemplate_CacheFind(pName);
    if ( pTemplate )
    {
        return pTemplate;
    }

    const SithTemplate* pMFTemplate = NULL;
    if ( !sithTemplate_masterFileCount || (pMFTemplate = (const SithTemplate*)stdHashtbl_Find(sithTemplate_pMasterHashtable, pName)) == 0 )
    {
        SITHLOG_ERROR("Template %s could not be found.\n", pName);
        return NULL;
    }

    if ( pMFTemplate->pBasedOn )
    {
        sithTemplate_GetTemplate(pMFTemplate->pBasedOn);
    }

    char aTemplateText[2048] = { 0 };
    STD_STRCPY(aTemplateText, pMFTemplate->pText);

    stdConffile_Open("none");
    if ( !stdConffile_ReadArgsFromStr(aTemplateText) )
    {
        sithTemplate_Parse(sithWorld_g_pLastLoadedWorld); // first arg is base
    }

    pTemplate = sithTemplate_Parse(sithWorld_g_pLastLoadedWorld);
    stdConffile_Close();

    if ( !pTemplate )
    {
        SITHLOG_ERROR("Template %s found in master file, but could not be loaded.\n", pName);
        return NULL;
    }

    return pTemplate;
}

SithThing* J3DAPI sithTemplate_Parse(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(strlen(stdConffile_g_entry.aArgs[0].argValue) > 0);

    SithThing* pTemplate = sithTemplate_CacheFind(stdConffile_g_entry.aArgs[0].argValue);
    if ( pTemplate )
    {
        return pTemplate;
    }

    SithThing newTemplate;
    sithThing_Reset(&newTemplate);

    // Get base template thing
    const SithThing* pBaseTemplate = sithTemplate_CacheFind(stdConffile_g_entry.aArgs[1].argValue);
    if ( !pBaseTemplate && !streq(stdConffile_g_entry.aArgs[1].argValue, "none") )
    {
        SITHLOG_ERROR("Based-on template %s not found, using blank.\n", stdConffile_g_entry.aArgs[1].argValue);
    }

    // Assign base to template
    sithThing_SetThingBasedOn(&newTemplate, pBaseTemplate);

    STD_STRCPY(newTemplate.aName, stdConffile_g_entry.aArgs[0].argValue);

    // Parse template params
    for ( size_t i = 2; i < stdConffile_g_entry.numArgs; ++i )
    {
        if ( sithThing_ParseArg(&stdConffile_g_entry.aArgs[i], pWorld, &newTemplate) )
        {
            SITHLOG_ERROR("Unrecognized argument '%s=%s' line %d.\n", stdConffile_g_entry.aArgs[i].argName, stdConffile_g_entry.aArgs[i].argValue, stdConffile_GetLineNumber());
        }
    }

    if ( newTemplate.type == SITH_THING_FREE )
    {
        SITHLOG_ERROR("Template %s has no type.  Discarded.\n", stdConffile_g_entry.aArgs[0].argValue);
        return NULL;
    }

    if ( pWorld->numThingTemplates >= pWorld->sizeThingTemplates )
    {
        SITHLOG_ERROR("Out of space for thing templates, line %d.\n", stdConffile_GetLineNumber());
        return NULL;
    }

    // Copy template to word template array
    pTemplate = &pWorld->aThingTemplates[pWorld->numThingTemplates++];
    newTemplate.idx = pTemplate->idx;
    STD_COPYMEM(pTemplate, &newTemplate, sizeof(SithThing));

    sithTemplate_CacheAdd(pTemplate);
    return pTemplate;
}

SithThing* J3DAPI sithTemplate_CacheFind(const char* pName)
{
    SITH_ASSERTREL(pName != NULL);

    if ( !sithTemplate_pHashtable )
    {
        return NULL;
    }

    return (SithThing*)stdHashtbl_Find(sithTemplate_pHashtable, pName);
}

void J3DAPI sithTemplate_CacheAdd(SithThing* pThing)
{
    SITH_ASSERTREL(strlen(pThing->aName) > 0);
    SITH_ASSERTREL(pThing != NULL);
    SITH_ASSERTREL(sithTemplate_pHashtable);
    stdHashtbl_Add(sithTemplate_pHashtable, pThing->aName, pThing);
}

void J3DAPI sithTemplate_CacheRemove(const SithThing* pThing)
{
    SITH_ASSERTREL(strlen(pThing->aName) > 0);
    SITH_ASSERTREL(pThing != NULL);
    if ( sithTemplate_pHashtable )
    {
        stdHashtbl_Remove(sithTemplate_pHashtable, pThing->aName);
    }
}

int J3DAPI sithTemplate_ParseTemplateList(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld != NULL);

    while ( stdConffile_ReadArgs() && !streq(stdConffile_g_entry.aArgs[0].argValue, "end") )
    {
        if ( !sithTemplate_Parse(pWorld) )
        {
            SITHLOG_ERROR("Template parse failed for template on line %d.\n", stdConffile_GetLineNumber());
        }
    }

    return 0;
}