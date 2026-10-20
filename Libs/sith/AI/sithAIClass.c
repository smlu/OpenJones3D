#include "sithAIClass.h"
#include <j3dcore/j3dhook.h>

#include <sith/AI/sithAI.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/RTI/symbols.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <math.h>

SithAIClass* J3DAPI sithAIClass_CacheFind(const char* pName);
void J3DAPI sithAIClass_CacheAdd(SithAIClass* pClass);
void J3DAPI sithAIClass_CacheRemove(const SithAIClass* pClass);

void sithAIClass_InstallHooks(void)
{
    J3D_HOOKFUNC(sithAIClass_Startup);
    J3D_HOOKFUNC(sithAIClass_Shutdown);
    J3D_HOOKFUNC(sithAIClass_ReadStaticAIClassesListText);
    J3D_HOOKFUNC(sithAIClass_WriteStaticAIClassesListBinary);
    J3D_HOOKFUNC(sithAIClass_ReadStaticAIClassesListBinary);
    J3D_HOOKFUNC(sithAIClass_AllocWorldAIClasses);
    J3D_HOOKFUNC(sithAIClass_FreeWorldAIClasses);
    J3D_HOOKFUNC(sithAIClass_Load);
    J3D_HOOKFUNC(sithAIClass_LoadEntry);
    J3D_HOOKFUNC(sithAIClass_CacheFind);
    J3D_HOOKFUNC(sithAIClass_CacheAdd);
    J3D_HOOKFUNC(sithAIClass_CacheRemove);
}

void sithAIClass_ResetGlobals(void)
{
    STD_ZEROMEM(&sithAIClass_g_pHashtable, sizeof(sithAIClass_g_pHashtable));
}

int sithAIClass_Startup(void)
{
    sithAIClass_g_pHashtable = stdHashtbl_New(64u);
    return sithAIClass_g_pHashtable == NULL;
}

void sithAIClass_Shutdown(void)
{
    if ( sithAIClass_g_pHashtable )
    {
        stdHashtbl_Free(sithAIClass_g_pHashtable);
    }

    sithAIClass_g_pHashtable = NULL;
}

int J3DAPI sithAIClass_WriteStaticAIClassesListText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("######### AI Classes ###########\n")
        || stdConffile_WriteLine("Section: AICLASS\n\n")
        || stdConffile_Printf("World AIClasses %d\n", pWorld->numAIClasses) )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numAIClasses; ++i )
    {
        if ( stdConffile_Printf("%d:\t%s\n", i, pWorld->aAIClasses[i].aName) )
        {
            return 1;
        }
    }

    return stdConffile_WriteLine("end\n") || stdConffile_WriteLine("################################\n\n\n");
}

int J3DAPI sithAIClass_ReadStaticAIClassesListText(SithWorld* pWorld, int bSkip)
{
    if ( bSkip )
    {
        return 1;
    }

    SITH_ASSERTREL(pWorld != NULL);

    stdConffile_ReadArgs();
    if ( strcmp(stdConffile_g_entry.aArgs[0].argValue, "world") || strcmp(stdConffile_g_entry.aArgs[1].argValue, "aiclasses") ) // TODO: Replace strcmp with strcmpi
    {
        SITHLOG_ERROR("Parse error reading static aiclasses list line %d.\n", stdConffile_GetLineNumber());
        return 1;
    }

    size_t numClasses = atoi(stdConffile_g_entry.aArgs[2].argValue);
    if ( !numClasses )
    {
        return 0;
    }

    if ( sithAIClass_AllocWorldAIClasses(pWorld, numClasses) )
    {
        SITHLOG_ERROR("Memory error while reading aiclasses, line %d.\n", stdConffile_GetLineNumber());
        return 1;
    }

    while ( stdConffile_ReadArgs() && strcmp(stdConffile_g_entry.aArgs[0].argValue, "end") ) // TODO: Replace strcmp with strcmpi
    {
        if ( !sithAIClass_Load(pWorld, stdConffile_g_entry.aArgs[1].argValue) )
        {
            SITHLOG_ERROR("Parse error while reading aiclasses, line %d.\n", stdConffile_GetLineNumber());
            return 1;
        }
    }

    SITHLOG_STATUS("AI classes read.  AI array space %d, used %d.\n", pWorld->sizeAIClasses, pWorld->numAIClasses);
    SITH_ASSERTREL(pWorld->numAIClasses <= pWorld->sizeAIClasses);
    return 0;
}

int J3DAPI sithAIClass_WriteStaticAIClassesListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    for ( size_t i = 0; i < pWorld->numAIClasses; ++i )
    {
        if ( sith_g_pHS->pFileWrite(fh, pWorld->aAIClasses[i].aName, STD_ARRAYLEN(pWorld->aAIClasses[i].aName)) != STD_ARRAYLEN(pWorld->aAIClasses[i].aName) )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithAIClass_ReadStaticAIClassesListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t numAIClasses = pWorld->numAIClasses;
    if ( sithAIClass_AllocWorldAIClasses(pWorld, pWorld->sizeAIClasses) )
    {
        return 1;
    }

    for ( size_t i = 0; i < numAIClasses; ++i )
    {
        char aFilename[64];
        static_assert(sizeof(aFilename) == sizeof(pWorld->aAIClasses[i].aName), "Array size mismatch");
        if ( sith_g_pHS->pFileRead(fh, aFilename, STD_ARRAYLEN(aFilename)) != STD_ARRAYLEN(aFilename) || !sithAIClass_Load(pWorld, aFilename) )
        {
            // TODO: maybe add logging here
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithAIClass_AllocWorldAIClasses(SithWorld* pWorld, size_t numClasses)
{
    SITH_ASSERTREL(pWorld);
    SITH_ASSERTREL(pWorld->aAIClasses == NULL);

    pWorld->aAIClasses = (SithAIClass*)STDMALLOC(sizeof(SithAIClass) * numClasses);
    if ( !pWorld->aAIClasses )
    {
        sithAIClass_FreeWorldAIClasses(pWorld);
        SITHLOG_ERROR("Memory allocation failure.\n");
        return 1;
    }

    STD_ZEROMEM(pWorld->aAIClasses, sizeof(SithAIClass) * numClasses);
    pWorld->numAIClasses  = 0;
    pWorld->sizeAIClasses = numClasses;
    return 0;
}

void J3DAPI sithAIClass_FreeWorldAIClasses(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld);
    if ( pWorld->aAIClasses )
    {
        for ( size_t i = 0; i < pWorld->numAIClasses; ++i )
        {
            sithAIClass_CacheRemove(&pWorld->aAIClasses[i]);
        }

        STDFREE(pWorld->aAIClasses);
        pWorld->aAIClasses = NULL;
    }

    pWorld->sizeAIClasses = 0;
    pWorld->numAIClasses  = 0;
}

SithAIClass* J3DAPI sithAIClass_Load(SithWorld* pWorld, const char* pName)
{
    SITH_ASSERTREL(pWorld && pName);
    SITH_ASSERTREL(strlen(pName) < 64);

    if ( !pWorld->aAIClasses )
    {
        SITHLOG_ERROR("No space to load AI class %s.\n", pName);
        return 0;
    }

    SithAIClass* pClass = sithAIClass_CacheFind(pName);
    if ( pClass )
    {
        return pClass;
    }

    char aFilePath[128];
    STD_FORMAT(aFilePath, "%s%c%s", "misc\\ai", '\\', pName);

    if ( pWorld->numAIClasses >= pWorld->sizeAIClasses )
    {
        SITHLOG_ERROR("No space to load new AI class %s.\n", pName);
        return 0;
    }

    pClass = &pWorld->aAIClasses[pWorld->numAIClasses];
    STD_ZEROMEM(pClass, sizeof(SithAIClass));

    STD_STRCPY(pClass->aName, pName);

    if ( sithAIClass_LoadEntry(aFilePath, pClass) )
    {
        sithAIClass_CacheAdd(pClass);
        pClass->num = pWorld->numAIClasses++;
        return pClass;
    }

    if ( !strcmp(pName, "default.ai") ) // TODO: Replace strcmp with strcmpi
    {
        // TODO: add logging here
        return 0;
    }

    pClass = sithAIClass_Load(pWorld, "default.ai");
    if ( !pClass )
    {
        SITHLOG_ERROR("AI Class %s not found, no default found.\n", pName);
        return 0;
    }

    SITHLOG_ERROR("AI Class %s not found, using %s.\n", pName, "default.ai");
    return pClass;
}

int J3DAPI sithAIClass_LoadEntry(const char* pPath, SithAIClass* pClass)
{
    SITH_ASSERTREL(pPath && pClass);
    if ( !stdConffile_Open(pPath) )
    {
        return 0;
    }

    pClass->maxStep       = 0.5f;
    pClass->sightDistance = 20.0f;
    pClass->heardDistance = 10.0f;
    pClass->fov           = 0.0f;
    pClass->accurancy     = 0.5f;
    pClass->armedMode     = 0;
    pClass->degTurnAlign  = 45.0f;

    if ( !stdConffile_ReadArgs() )
    {
        SITHLOG_ERROR("Syntax error reading AI info.  File %s line %d.\n", stdConffile_GetFilename(), stdConffile_GetLineNumber());
        stdConffile_Close();
        return 0;
    }

    for ( size_t i = 0; i < stdConffile_g_entry.numArgs; ++i )
    {
        if ( !strcmp(stdConffile_g_entry.aArgs[i].argName, "alignment") ) // TODO: Replace strcmp with strcmpi
        {
            pClass->alignment = atof(stdConffile_g_entry.aArgs[i].argValue);
        }
        else if ( !strcmp(stdConffile_g_entry.aArgs[i].argName, "rank") ) // TODO: Replace strcmp with strcmpi
        {
            pClass->rank = atof(stdConffile_g_entry.aArgs[i].argValue);
        }
        else if ( !strcmp(stdConffile_g_entry.aArgs[i].argName, "maxstep") )
        {
            pClass->maxStep = atof(stdConffile_g_entry.aArgs[i].argValue);
        }
        else if ( !strcmp(stdConffile_g_entry.aArgs[i].argName, "sightdist") ) // TODO: Replace strcmp with strcmpi
        {
            pClass->sightDistance = atof(stdConffile_g_entry.aArgs[i].argValue);
        }
        else if ( !strcmp(stdConffile_g_entry.aArgs[i].argName, "heardist") ) // TODO: Replace strcmp with strcmpi
        {
            pClass->heardDistance = atof(stdConffile_g_entry.aArgs[i].argValue);
        }
        else if ( !strcmp(stdConffile_g_entry.aArgs[i].argName, "wakeupdist") ) // TODO: Replace strcmp with strcmpi
        {
            pClass->weakupDistance = atof(stdConffile_g_entry.aArgs[i].argValue);
        }
        else if ( !strcmp(stdConffile_g_entry.aArgs[i].argName, "fov") ) // TODO: Replace strcmp with strcmpi
        {
            float angle = atof(stdConffile_g_entry.aArgs[i].argValue) * 0.5f;

            float sin, out;
            stdMath_SinCos(angle, &sin, &out);
            pClass->fov = out;
        }
        else if ( !strcmp(stdConffile_g_entry.aArgs[i].argName, "accuracy") ) // TODO: Replace strcmp with strcmpi
        {
            pClass->accurancy = atof(stdConffile_g_entry.aArgs[i].argValue);
        }
        else if ( !strcmp(stdConffile_g_entry.aArgs[i].argName, "armedmode") ) // TODO: Replace strcmp with strcmpi
        {
            pClass->armedMode = atoi(stdConffile_g_entry.aArgs[i].argValue);
            if ( pClass->armedMode >= SITH_PUPPET_NUMARMEDMODES )
            {
                pClass->armedMode = 0;
                SITHLOG_ERROR("Illegal armed mode in class %s.  Setting to unarmed.\n", pClass->aName);
            }
        }
        else if ( !strcmp(stdConffile_g_entry.aArgs[i].argName, "degturnalign") ) // TODO: Replace strcmp with strcmpi
        {
            pClass->degTurnAlign = atof(stdConffile_g_entry.aArgs[i].argValue);
        }
    }

    while ( stdConffile_ReadArgs() )
    {
        SithAIInstinct* pInstinct = &pClass->aInstincts[pClass->numInstincts];
        if ( pClass->numInstincts >= STD_ARRAYLEN(pClass->aInstincts) )
        {
            SITHLOG_ERROR("Too many instincts in class %s (max %d)\n", pClass->aName, STD_ARRAYLEN(pClass->aInstincts));
            // Note, don't return or break here, continue reading instincts to read to the end of instinct section in the file
        }
        else
        {
            SithAIRegisteredInstinct* pRegInstinct = sithAI_FindInstinct(stdConffile_g_entry.aArgs[0].argValue);
            if ( !pRegInstinct )
            {
                SITHLOG_ERROR("Could not find registered instinct %s for class %s, not added.\n", stdConffile_g_entry.aArgs[0].argValue, pClass->aName);
            }
            else
            {
                pInstinct->pfInstinct       = pRegInstinct->pfInstinct;
                pInstinct->updateModes      = pRegInstinct->updateModes;
                pInstinct->updateBlockModes = pRegInstinct->updateBlockModes;
                pInstinct->triggerEvents    = pRegInstinct->triggerEvents;

                for ( size_t i = 0; i < STD_ARRAYLEN(pInstinct->fltArg); ++i )
                {
                    if ( stdConffile_g_entry.numArgs <= i + 1 )
                    {
                        pInstinct->fltArg[i] = 0.0f;
                        pInstinct->intArg[i] = 0;
                    }
                    else
                    {

                        pInstinct->fltArg[i] = atof(stdConffile_g_entry.aArgs[i + 1].argValue);
                        pInstinct->intArg[i] = (int)floorf(pInstinct->fltArg[i]);
                    }
                }

                ++pClass->numInstincts;
            }
        }
    }

    stdConffile_Close();
    return 1;
}

SithAIClass* J3DAPI sithAIClass_CacheFind(const char* pName)
{
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(sithAIClass_g_pHashtable != NULL);
    return (SithAIClass*)stdHashtbl_Find(sithAIClass_g_pHashtable, pName);
}

void J3DAPI sithAIClass_CacheAdd(SithAIClass* pClass)
{
    SITH_ASSERTREL(pClass != NULL);
    SITH_ASSERTREL(strlen(pClass->aName) > 0);
    SITH_ASSERTREL(sithAIClass_g_pHashtable != NULL);
    stdHashtbl_Add(sithAIClass_g_pHashtable, pClass->aName, pClass);
}

void J3DAPI sithAIClass_CacheRemove(const SithAIClass* pClass)
{
    SITH_ASSERTREL(pClass != NULL);
    SITH_ASSERTREL(strlen(pClass->aName) > 0);
    SITH_ASSERTREL(sithAIClass_g_pHashtable != NULL);
    stdHashtbl_Remove(sithAIClass_g_pHashtable, pClass->aName);
}
