#include "sithModel.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Primitives/rdModel3.h>

#include <sith/RTI/symbols.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdFnames.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/types.h>

#include <string.h>

#define SITHMODEL_DEFAULTMODEL    "dflt.3do"
#define SITHMODEL_EXTRABUFFERSIZE 32

static bool sithModel_bModelStartup = false; // Added
static bool sithModel_bHiPoly;
static tHashTable* sithModel_pHashtable = NULL; // Added: Init to null


void J3DAPI sithModel_CacheRemove(const rdModel3* pModel);
void J3DAPI sithModel_CacheAdd(rdModel3* pModel);
rdModel3* J3DAPI sithModel_CacheFind(const char* pName);

// TODO: this is just temp wrapper for sithModel_GetModelByIndex with uint16_t modelIdx.
//       Can be removed after all DSS functions that calls sithModel_GetModelByIndex are implemented
rdModel3* J3DAPI sithModel_GetModelByIndexWrap(uint16_t modelIdx)
{
    return sithModel_GetModelByIndex(modelIdx);
}

void sithModel_InstallHooks(void)
{
    J3D_HOOKFUNC(sithModel_Startup);
    J3D_HOOKFUNC(sithModel_Shutdown);
    J3D_HOOKFUNC(sithModel_ReadStaticModelsListText);
    J3D_HOOKFUNC(sithModel_WriteStaticModelsListBinary);
    J3D_HOOKFUNC(sithModel_ReadStaticModelsListBinary);
    J3D_HOOKFUNC(sithModel_FreeWorldModels);
    J3D_HOOKFUNC(sithModel_Load);
    J3D_HOOKFUNC(sithModel_GetModelMemUsage);
    J3D_HOOKFUNC(sithModel_AllocWorldModels);

    J3DHookFunction(sithModel_GetModelByIndex_ADDR, (void*)sithModel_GetModelByIndexWrap);  //J3D_HOOKFUNC(sithModel_GetModelByIndex);
    J3D_HOOKFUNC(sithModel_GetModelIndex);
    J3D_HOOKFUNC(sithModel_GetModel);
    J3D_HOOKFUNC(sithModel_GetMeshIndex);
    J3D_HOOKFUNC(sithModel_EnableHiPoly);
    J3D_HOOKFUNC(sithModel_CacheRemove);
    J3D_HOOKFUNC(sithModel_CacheAdd);
    J3D_HOOKFUNC(sithModel_CacheFind);
}

void sithModel_ResetGlobals(void)
{}

int sithModel_Startup(void)
{
    // Added
    if ( sithModel_bModelStartup )
    {
        SITHLOG_ERROR("Multiple startup on model system.\n");
        return 1;
    }

    sithModel_pHashtable = stdHashtbl_New(256u);
    if ( !sithModel_pHashtable ) // Added
    {
        SITHLOG_ERROR("Could not allocate model hashtable.\n");
        return 1;
    }

    sithModel_bModelStartup = true;
    return 0;
}

void sithModel_Shutdown(void)
{
    if ( !sithModel_bModelStartup )
    {
        SITHLOG_ERROR("Material system not started.\n");
        return;
    }

    if ( sithModel_pHashtable )
    {
        stdHashtbl_Free(sithModel_pHashtable);
        sithModel_pHashtable = NULL;
    }

    sithModel_bModelStartup = false;
}

int J3DAPI sithModel_WriteStaticModelsListText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("###### Models information ######\n")
        || stdConffile_WriteLine("Section: MODELS\n\n")
        || stdConffile_Printf("World models %d\n\n", pWorld->numModels + SITHMODEL_EXTRABUFFERSIZE) )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numModels; ++i )
    {
        char aFilename[64] = { 0 }; // Changed: Init to 0 and changed size to 64 from 128
        STD_STRCPY(aFilename, pWorld->aModels[i].aName);
        if ( stdConffile_Printf("%d:\t%s\n", i, aFilename) )
        {
            return 1;
        }
    }

    return stdConffile_WriteLine("end\n") || stdConffile_WriteLine("################################\n\n\n");
}

int J3DAPI sithModel_ReadStaticModelsListText(SithWorld* pWorld, int bSkip)
{
    if ( bSkip )
    {
        return 1;
    }

    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aModels == NULL);
    SITH_ASSERTREL(pWorld->numModels == 0);

    stdConffile_ReadArgs();
    if ( strcmp(stdConffile_g_entry.aArgs[0].argValue, "world") != 0 || strcmp(stdConffile_g_entry.aArgs[1].argValue, "models") != 0 )
    {
        SITHLOG_ERROR("Parse error reading static models list line %d.\n", stdConffile_GetLineNumber());
        return 1;
    }

    size_t sizeModels = atoi(stdConffile_g_entry.aArgs[2].argValue); // TODO: Should convert text number to unsigned
    if ( sizeModels == 0 )
    {
        SITHLOG_STATUS("Models section not needed if numModels == 0.f\n");
        return 0;
    }

    if ( !sithModel_AllocWorldModels(pWorld, sizeModels) )
    {
        float progress = 60.0f;
        float step = (10.0f / (float)sizeModels);
        sithWorld_UpdateLoadProgress(progress);

        while ( stdConffile_ReadArgs() && strcmp(stdConffile_g_entry.aArgs[0].argValue, "end") != 0 )
        {
            sithModel_Load(stdConffile_g_entry.aArgs[1].argValue, /*bSkipDefault=*/0);

            progress += step;
            sithWorld_UpdateLoadProgress(progress);
        }

        sithWorld_UpdateLoadProgress(70.0f);
        SITHLOG_STATUS("Models read.  Model array space %d, used %d.\n", pWorld->sizeModels, pWorld->numModels);
        SITH_ASSERTREL(pWorld->numModels <= pWorld->sizeModels);
        return 0;
    }
    else
    {
        SITHLOG_ERROR("Memory error while reading models, line %d.\n", stdConffile_GetLineNumber());
        return 1;
    }
}

int J3DAPI sithModel_WriteStaticModelsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    for ( size_t i = 0; i < pWorld->numModels; ++i )
    {
        char aFilename[64] = { 0 }; // Changed: Init to 0 and changed size to 64 from 128
        STD_STRCPY(aFilename, pWorld->aModels[i].aName);
        if ( sith_g_pHS->pFileWrite(fh, aFilename, STD_ARRAYLEN(aFilename)) != STD_ARRAYLEN(aFilename) )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithModel_ReadStaticModelsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t numModels = pWorld->numModels; // Note, the num of models must be cached because it's set to 0 in sithModel_AllocWorldModels
    if ( sithModel_AllocWorldModels(pWorld, numModels) )
    {
        return 1;
    }

    for ( size_t i = 0; i < numModels; ++i )
    {
        char aFilename[64] = { 0 }; // Added: Init to 0
        if ( sith_g_pHS->pFileRead(fh, aFilename, STD_ARRAYLEN(aFilename)) != STD_ARRAYLEN(aFilename) )
        {
            return 1;
        }

        sithModel_Load(aFilename, 0);
    }

    return 0;
}

void J3DAPI sithModel_FreeWorldModels(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld != NULL);
    if ( !pWorld->sizeModels )
    {
        SITH_ASSERTREL(pWorld->aModels == NULL);
        return;
    }

    SITH_ASSERTREL(pWorld->aModels != NULL);
    for ( size_t i = 0; i < pWorld->numModels; ++i )
    {
        sithModel_CacheRemove(&pWorld->aModels[i]);
        rdModel3_FreeEntryGeometryOnly(&pWorld->aModels[i]);
    }

    stdMemory_Free(pWorld->aModels);

    pWorld->aModels    = NULL;
    pWorld->numModels  = 0;
    pWorld->sizeModels = 0;
}

rdModel3* J3DAPI sithModel_Load(const char* pName, int bSkipDefault)
{
    SithWorld* pWorld = sithWorld_g_pLastLoadedWorld;

    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aModels != NULL);

    SITH_ASSERTREL(pName != NULL);
    rdModel3* pModel = sithModel_CacheFind(pName);
    if ( pModel )
    {
        return pModel;
    }

    SITH_ASSERTREL(pWorld->numModels <= pWorld->sizeModels);
    if ( pWorld->numModels >= pWorld->sizeModels )
    {
        SITHLOG_ERROR("No space to load new model '%s'.\n", pName);
        return NULL;
    }

    char aPath[128];
    SITH_ASSERTREL(strlen(pName) < STD_ARRAYLEN(aPath));

    // Added: Added option to load model from hi3do folder
    if ( sithModel_bHiPoly )
    {
        stdFnames_MakePath(aPath, STD_ARRAYLEN(aPath), "hi3do", pName);
        // Fixed: Use bounded extension replacement for the fixed-size path buffer.
        stdFnames_ChangeExtEx(aPath, STD_ARRAYLEN(aPath), "3do");
        if ( !stdUtil_FileExists(aPath) )
        {
            stdFnames_MakePath(aPath, STD_ARRAYLEN(aPath), "3do", pName);
            // Fixed: Use bounded extension replacement for the fixed-size path buffer.
            stdFnames_ChangeExtEx(aPath, STD_ARRAYLEN(aPath), "3do");
        }
    }
    else
    {
        stdFnames_MakePath(aPath, STD_ARRAYLEN(aPath), "3do", pName);
        // Fixed: Use bounded extension replacement for the fixed-size path buffer.
        stdFnames_ChangeExtEx(aPath, STD_ARRAYLEN(aPath), "3do");
    }

    pModel = &pWorld->aModels[pWorld->numModels];
    if ( rdModel3_LoadEntry(aPath, pModel) )
    {
        pModel->num = pWorld->numModels;
        if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
        {
            pModel->num = SITHWORLD_STATICINDEX(pModel->num);
        }

        sithModel_CacheAdd(pModel);

        pWorld->numModels++;
        return pModel;
    }

    // Failed to Load model from aPath
    if ( bSkipDefault )
    {
        return NULL;
    }

    // Try to load default model
    pModel = sithModel_Load(SITHMODEL_DEFAULTMODEL, /*bSkipDefault=*/1);
    if ( pModel )
    {
        SITHLOG_ERROR("Model %s not found, using default.\n", pName);
        return pModel;
    }

    SITHLOG_ERROR("Model %s not found, no default loaded.\n", pName);
    return NULL;
}

size_t J3DAPI sithModel_GetModelMemUsage(const rdModel3* pModel)
{
    size_t memUsage = sizeof(rdModel3HNode) * pModel->numHNodes + sizeof(pModel->apMaterials) * pModel->numMaterials + sizeof(rdModel3);
    for ( size_t i = 0; i < pModel->numGeos; ++i )
    {
        memUsage += sizeof(rdModel3GeoSet);
        for ( size_t j = 0; j < pModel->aGeos[i].numMeshes; ++j )
        {
            memUsage += sizeof(rdFace) * pModel->aGeos[i].aMeshes[j].numFaces
                + 32 * pModel->aGeos[i].aMeshes[j].numVertices// TODO: 32 - looks to be sizeof(rdVector4) * 2 aka vert colors but it's missing sizeof(rdVector) - aVerts + sizeof(rdVector) - aVertNormals
                + sizeof(rdModel3Mesh)
                + sizeof(rdVector2) * pModel->aGeos[i].aMeshes[j].numTexVertices;

            for ( size_t k = 0; k < pModel->aGeos[i].aMeshes[j].numFaces; ++k )
            {
                // TODO: Note, theaTexVerts array is only allocated if face has material
                memUsage += 8 * pModel->aGeos[i].aMeshes[j].aFaces[k].numVertices;// I guess 8 could be sizeof(int*) + sizeof(int*) aka idix pointers for vert and uvs
            }
        }
    }

    return memUsage;
}

int J3DAPI sithModel_AllocWorldModels(SithWorld* pWorld, size_t size)
{
    SITH_ASSERTREL(pWorld->aModels == NULL);

    pWorld->aModels = STDMALLOC(sizeof(rdModel3) * size);
    if ( !pWorld->aModels )
    {
        SITHLOG_ERROR("Failed to allocate models array.\n");
        return 1;
    }

    pWorld->sizeModels = size;
    pWorld->numModels  = 0;
    memset(pWorld->aModels, 0, sizeof(rdModel3) * pWorld->sizeModels);
    return 0;
}

rdModel3* J3DAPI sithModel_GetModelByIndex(size_t modelIdx)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(modelIdx) )
    {
        pWorld   = sithWorld_g_pStaticWorld;
        modelIdx = SITHWORLD_FROM_STATICINDEX(modelIdx);
    }

    if ( pWorld && !SITHWORLD_IS_STATICINDEX(modelIdx) && modelIdx < pWorld->numModels ) // "TODO: What's the point of !SITHWORLD_IS_STATICINDEX(modelIdx) check?
    {
        return &pWorld->aModels[modelIdx];
    }

    return NULL;
}

int J3DAPI sithModel_GetModelIndex(const rdModel3* pModel)
{
    if ( pModel >= sithWorld_g_pCurrentWorld->aModels && pModel < &sithWorld_g_pCurrentWorld->aModels[sithWorld_g_pCurrentWorld->numModels] )
    {
        return pModel - sithWorld_g_pCurrentWorld->aModels;
    }

    else if ( pModel >= sithWorld_g_pStaticWorld->aModels && pModel < &sithWorld_g_pStaticWorld->aModels[sithWorld_g_pStaticWorld->numModels] )
    {
        return  SITHWORLD_STATICINDEX(pModel - sithWorld_g_pStaticWorld->aModels);
    }

    return -1;
}


rdModel3* J3DAPI sithModel_GetModel(const char* pName)
{
    if ( !pName )
    {
        return NULL;
    }

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    for ( size_t worldNum = 0; worldNum < 2; worldNum++ )
    {
        if ( worldNum == 1 )
        {
            pWorld = sithWorld_g_pStaticWorld;
        }

        if ( pWorld )
        {
            for ( size_t i = 0; i < pWorld->numModels; ++i )
            {
                if ( !strcmpi(pName, pWorld->aModels[i].aName) )
                {
                    return &pWorld->aModels[i];
                }
            }
        }
    }

    return NULL;
}

int J3DAPI sithModel_GetMeshIndex(const rdModel3* pModel, const char* pMeshName)
{
    if ( !pModel || !pMeshName )
    {
        return -1;
    }

    for ( size_t i = 0; i < pModel->aGeos[0].numMeshes; ++i )
    {
        if ( strcmp(pMeshName, pModel->aGeos[0].aMeshes[i].name) == 0 )
        {
            return i;
        }
    }

    return -1;

}

void J3DAPI sithModel_EnableHiPoly(int bEnabled)
{
    sithModel_bHiPoly = bEnabled;
}

bool sithModel_IsHiPolyEnabled(void)
{
    return sithModel_bHiPoly != 0;
}

void J3DAPI sithModel_CacheRemove(const rdModel3* pModel)
{
    SITH_ASSERTREL(pModel != NULL);
    SITH_ASSERTREL(strlen(pModel->aName) > 0);
    SITH_ASSERTREL(sithModel_pHashtable != NULL);
    stdHashtbl_Remove(sithModel_pHashtable, pModel->aName);
}

void J3DAPI sithModel_CacheAdd(rdModel3* pModel)
{
    SITH_ASSERTREL(pModel != NULL);
    SITH_ASSERTREL(strlen(pModel->aName) > 0);
    SITH_ASSERTREL(sithModel_pHashtable != NULL);
    stdHashtbl_Add(sithModel_pHashtable, pModel->aName, pModel);
}

rdModel3* J3DAPI sithModel_CacheFind(const char* pName)
{
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(sithModel_pHashtable != NULL);
    return stdHashtbl_Find(sithModel_pHashtable, pName);
}
