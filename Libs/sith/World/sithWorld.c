#include "sithWorld.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Math/rdMath.h>

#include <sith/RTI/symbols.h>
#include <sith/AI/sithAIClass.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogParse.h>
#include <sith/Devices/sithSound.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithParticle.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithPVS.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSprite.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>

#include <sound/Sound.h>

#include <std/General/stdConffile.h>
#include <std/General/stdFnames.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>

#define SITHWORLD_COPYRIGHTLINELEN  32
#define SITHWORLD_NUMCOPYRIGHTLINES 38

#define SITHWORLD_NDYMINVERSION 2
#define SITHWORLD_NDYMAXVERSION 3
#define SITHWORLD_CNDVERSION    3

// A "mocking" world struct which is stored to cnd files.
// Stores all pointers as uint32_t type
typedef struct sCndWorld
{
    char aCopyright[1216];
    char aName[64];
    SithWorldState state;
    int version;
    float gravity;
    float ceilingSkyHeight;
    float horizonDistance;
    rdVector2 horizonSkyOffset;
    rdVector2 ceilingSkyOffset;
    float distancesLOD[4];
    SithWorldFog fog;
    uint32_t numSounds;
    uint32_t numMaterials;
    uint32_t sizeMaterials;
    uint32_t aMaterials;
    uint32_t apMatArray;
    uint32_t numVertices;
    uint32_t aVertices;
    uint32_t aTransformedVertices;
    uint32_t aVertexRenderTickIds;
    uint32_t numTexVertices;
    uint32_t aTexVerticies;
    uint32_t aVertDynamicLights;
    uint32_t aVertDynamicLightsRenderTicks;
    uint32_t numAdjoins;
    uint32_t aAdjoins;
    uint32_t numSurfaces;
    uint32_t aSurfaces;
    uint32_t numSectors;
    uint32_t aSectors;
    size_t numAIClasses;
    size_t sizeAIClasses;
    uint32_t aAIClasses;
    uint32_t numModels;
    uint32_t sizeModels;
    uint32_t aModels;
    size_t numSprites;
    size_t sizeSprites;
    uint32_t aSprites;
    uint32_t numKeyframes;
    uint32_t sizeKeyframes;
    uint32_t aKeyframes;
    uint32_t numPuppetClasses;
    uint32_t sizePuppetClasses;
    uint32_t aPuppetClasses;
    uint32_t numSoundClasses;
    uint32_t sizeSoundClasses;
    uint32_t aSoundClasses;
    uint32_t numCogScripts;
    uint32_t sizeCogScripts;
    uint32_t aCogScripts;
    uint32_t numCogs;
    uint32_t sizeCogs;
    uint32_t aCogs;
    uint32_t numThingTemplates;
    uint32_t sizeThingTemplates;
    uint32_t aThingTemplates;
    uint32_t numThings;
    int32_t lastThingIdx;
    uint32_t aThings;
    uint32_t sizePVS;
    uint32_t aPVS;
} CndWorld;
static_assert(sizeof(CndWorld) == 1564, "sizeof(CndWorld) == 1564");

static bool bWorldStartup = false; // Altered: Init to 0

static bool bValidCopyright;
static char aCopyrightBuf[1216];
static const char aCopyrightSymbol[1217] =
{
    "................................"
    "................@...@...@...@..."
    ".............@...@..@..@...@...."
    "................@.@.@.@.@.@....."
    "@@@@@@@@......@...........@....."
    "@@@@@@@@....@@......@@@....@...."
    "@@.....@.....@......@@@.....@@.."
    "@@.@@@@@......@.....@@@......@@."
    "@@@@@@@@.......@....@@.....@@..."
    "@@@@@@@@.........@@@@@@@@@@....."
    "@@@@@@@@..........@@@@@@........"
    "@@.....@..........@@@@@........."
    "@@.@@@@@.........@@@@@@........."
    "@@.....@.........@@@@@@........."
    "@@@@@@@@.........@@@@@@........."
    "@@@@@@@@.........@@@@@@@........"
    "@@@...@@.........@@@@@@@........"
    "@@.@@@.@.........@.....@........"
    "@@..@..@........@.......@......."
    "@@@@@@@@........@.......@......."
    "@@@@@@@@.......@........@......."
    "@@..@@@@.......@........@......."
    "@@@@..@@......@.........@......."
    "@@@@.@.@......@.........@......."
    "@@....@@........................"
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
    "@@@@@@@@@@@@@.@@@@@@@@@@@@@@@@@@"
    "@@.@@..@@@@@..@@@@@@@@@@.@@@@@@@"
    "@@.@.@.@@@@.@.@@@.@..@@...@@@..@"
    "@@..@@@@@@....@@@..@@@@@.@@@@.@@"
    "@@@@@@@@...@@.@@@.@@@@@..@@...@@"
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
    "@.(c).lucasarts.entertainment..@"
    "@.........company.llc..........@"
    "@....(c).lucasfilm.ltd.&.tm....@"
    "@.....all.rights.reserved......@"
    "@...used.under.authorization...@"
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
};

static SithWorldLoadProgressCallback pfLoadProgressCallback;

static char aCurSection[1024];
static size_t numTextSectionParseHandlers;
static SithWorldTextSectionParseHandler aTextSectioParseHandlers[32];

static const SithWorldBinarySectionParser aBinarySectionParsers[17] =
{
    {
        "sounds",
        &sithSound_ReadSoundsListBinary,
        &sithSound_WriteSoundsListBinary
    },
    {
        "materials",
        &sithMaterial_ReadMaterialsListBinary,
        &sithMaterial_WriteMaterialsListBinary
    },
    {
        "georesource",
        &sithWorld_ReadGeoresourceBinary,
        &sithWorld_WriteGeoresourceBinary
    },
    {
        "adjoins",
        &sithSurface_ReadAdjoinsListBinary,
        &sithSurface_WriteAdjoinsListBinary
    },
    {
        "surfaces",
        &sithSurface_ReadSurfacesListBinary,
        &sithSurface_WriteSurfacesListBinary
    },
    {
        "sectors",
        &sithSector_ReadSectorsListBinary,
        &sithSector_WriteSectorsListBinary
    },
    {
        "aiclasses",
        &sithAIClass_ReadStaticAIClassesListBinary,
        &sithAIClass_WriteStaticAIClassesListBinary
    },
    {
        "models",
        &sithModel_ReadStaticModelsListBinary,
        &sithModel_WriteStaticModelsListBinary
    },
    {
        "sprites",
        &sithSprite_ReadStaticSpritesListBinary,
        &sithSprite_WriteStaticSpritesListBinary
    },
    {
        "keyframes",
        &sithPuppet_ReadStaticKeyframesListBinary,
        &sithPuppet_WriteStaticKeyframesListBinary
    },
    {
        "animclass",
        &sithPuppet_ReadStaticPuppetsListBinary,
        &sithPuppet_WriteStaticPuppetsListBinary
    },
    {
        "soundclasses",
        &sithSoundClass_ReadSoundClassesListBinary,
        &sithSoundClass_WriteSoundClassesListBinary
    },
    {
        "cogcripts",
        &sithCog_ReadCogScriptsListBinary,
        &sithCog_WriteCogScriptsListBinary
    },
    {
        "cogs",
        &sithCog_ReadCogsListBinary,
        &sithCog_WriteCogsListBinary
    },
    {
        "templates",
        &sithTemplate_ReadThingTemplatesListBinary,
        &sithTemplate_WriteThingTemplatesListBinary
    },
    {
        "things",
        &sithThing_ReadStaticThingsListBinary,
        &sithThing_WriteStaticThingsListBinary
    },
    {
        "pvs",
        &sithPVS_ReadPVSBinary,
        &sithPVS_WritePVSBinary
    }
};

void sithWorld_InstallHooks(void)
{
    J3D_HOOKFUNC(sithWorld_Startup);
    J3D_HOOKFUNC(sithWorld_Shutdown);
    J3D_HOOKFUNC(sithWorld_SetLoadProgressCallback);
    J3D_HOOKFUNC(sithWorld_UpdateLoadProgress);
    J3D_HOOKFUNC(sithWorld_Load);
    J3D_HOOKFUNC(sithWorld_LoadPostProcess);
    J3D_HOOKFUNC(sithWorld_LoadEntryText);
    J3D_HOOKFUNC(sithWorld_Close);
    J3D_HOOKFUNC(sithWorld_NewEntry);
    J3D_HOOKFUNC(sithWorld_Free);
    J3D_HOOKFUNC(sithWorld_ReadHeaderText);
    J3D_HOOKFUNC(sithWorld_ReadCopyrightText);
    J3D_HOOKFUNC(sithWorld_GetMemoryUsage);
    J3D_HOOKFUNC(sithWorld_RegisterTextSectionParser);
    J3D_HOOKFUNC(sithWorld_ResetRenderState);
    J3D_HOOKFUNC(sithWorld_ValidateWorld);
    J3D_HOOKFUNC(sithWorld_InitPlayers);
    J3D_HOOKFUNC(sithWorld_ReadTextSection);
    J3D_HOOKFUNC(sithWorld_GetTextSectionParserIndex);
    J3D_HOOKFUNC(sithWorld_ReadGeoresourceText);
    J3D_HOOKFUNC(sithWorld_WriteGeoresourceBinary);
    J3D_HOOKFUNC(sithWorld_ReadGeoresourceBinary);
    J3D_HOOKFUNC(sithWorld_LoadEntryBinary);
}

void sithWorld_ResetGlobals(void)
{
    memset(&sithWorld_g_pCurrentWorld, 0, sizeof(sithWorld_g_pCurrentWorld));
    memset(&sithWorld_g_pStaticWorld, 0, sizeof(sithWorld_g_pStaticWorld));
    memset(&sithWorld_g_pLastLoadedWorld, 0, sizeof(sithWorld_g_pLastLoadedWorld));
    memset(&sithWorld_g_bLoading, 0, sizeof(sithWorld_g_bLoading));
}

int sithWorld_Startup(void)
{
    SITH_ASSERTREL(bWorldStartup == false);

    numTextSectionParseHandlers = 0;

    if ( sithWorld_RegisterTextSectionParser("copyright", sithWorld_ReadCopyrightText)
        || sithWorld_RegisterTextSectionParser("header", sithWorld_ReadHeaderText)
        || sithWorld_RegisterTextSectionParser("sounds", sithSound_ReadSoundsListText)
        || sithWorld_RegisterTextSectionParser("materials", sithMaterial_ReadMaterialsListText)
        || sithWorld_RegisterTextSectionParser("georesource", sithWorld_ReadGeoresourceText)
        || sithWorld_RegisterTextSectionParser("sectors", sithSector_ReadSectorsListText)
        || sithWorld_RegisterTextSectionParser("aiclass", sithAIClass_ReadStaticAIClassesListText)
        || sithWorld_RegisterTextSectionParser("models", sithModel_ReadStaticModelsListText)
        || sithWorld_RegisterTextSectionParser("sprites", sithSprite_ReadStaticSpritesListText)
        || sithWorld_RegisterTextSectionParser("keyframes", sithPuppet_ReadStaticKeyframesListText)
        || sithWorld_RegisterTextSectionParser("animclass", sithPuppet_ReadStaticPuppetsListText)
        || sithWorld_RegisterTextSectionParser("soundclass", sithSoundClass_ReadSoundClassesListText)
        || sithWorld_RegisterTextSectionParser("cogscripts", sithCog_ReadCogScriptsListText)
        || sithWorld_RegisterTextSectionParser("cogs", sithCog_ReadCogsListText)
        || sithWorld_RegisterTextSectionParser("templates", sithTemplate_ReadThingTemplatesListText)
        || sithWorld_RegisterTextSectionParser("things", sithThing_ReadStaticThingsListText)
        || sithWorld_RegisterTextSectionParser("pvs", sithPVS_ReadPVSText) )
    {
        return 0;
    }

    bWorldStartup = true;
    return 1;
}

void sithWorld_Shutdown(void)
{
    SITH_ASSERTREL(bWorldStartup == true);
    if ( sithWorld_g_pCurrentWorld )
    {
        stdMemory_Free(sithWorld_g_pCurrentWorld);
    }

    if ( sithWorld_g_pStaticWorld )
    {
        stdMemory_Free(sithWorld_g_pStaticWorld);
    }

    sithWorld_g_pCurrentWorld    = NULL;
    sithWorld_g_pStaticWorld     = NULL;
    sithWorld_g_pLastLoadedWorld = NULL;

    bWorldStartup = false;
}

void J3DAPI sithWorld_SetLoadProgressCallback(SithWorldLoadProgressCallback pfProgressCallback)
{
    pfLoadProgressCallback = pfProgressCallback;
}

void J3DAPI sithWorld_UpdateLoadProgress(float progress)
{
    if ( pfLoadProgressCallback )
    {
        pfLoadProgressCallback(progress);
    }
}

int J3DAPI sithWorld_Load(SithWorld* pWorld, const char* pFilename)
{
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(bWorldStartup == true);

    if ( !pWorld )
    {
        return 1;
    }

    if ( pFilename )
    {
        sithWorld_g_bLoading         = 1;
        sithWorld_g_pLastLoadedWorld = pWorld;

        const char* pExt = stdFnames_FindExt(pFilename);
        int bLoadFailed = strcmp(pExt, "cnd")
            ? sithWorld_LoadEntryText(pWorld, pFilename, /*bSkipParsing=*/0)
            : sithWorld_LoadEntryBinary(pWorld, pFilename);

        if ( bLoadFailed )
        {
            SITHLOG_ERROR("Parse problem in file '%s'.\n", pFilename);
            sithWorld_g_bLoading = 0;
            return 1;
        }
    }

    if ( sithWorld_LoadPostProcess(pWorld) )
    {
        // Post-process error
        if ( pFilename )
        {
            SITHLOG_ERROR("Post-load processing failed for %s.\n", pFilename);
        }
        else
        {
            SITHLOG_ERROR("Post-load processing failed.\n");
        }

        sithWorld_g_bLoading = 0;
        return 1;
    }

    // Success
    sithWorld_g_bLoading = 0;
    return 0;
}

int J3DAPI sithWorld_LoadPostProcess(SithWorld* pWorld)
{
    if ( (pWorld->state & SITH_WORLD_STATE_INITIALIZED) != 0 )
    {
        return 0;
    }

    pWorld->state |= SITH_WORLD_STATE_INIT_HUD | SITH_WORLD_STATE_UPDATE_FOG;

    if ( !pWorld->numVertices )
    {
        pWorld->state |= SITH_WORLD_STATE_INITIALIZED;
        return 0;
    }

    // Alloc render arrays

    pWorld->aTransformedVertices = (rdVector3*)STDMALLOC(sizeof(rdVector3) * pWorld->numVertices);
    if ( !pWorld->aTransformedVertices )
    {
        goto alloc_error;
    }
    memset(pWorld->aTransformedVertices, 0, sizeof(rdVector3) * pWorld->numVertices); // Added: Init to 0

    pWorld->aVertDynamicLights = (rdVector4*)STDMALLOC(sizeof(rdVector4) * pWorld->numVertices);
    if ( !pWorld->aVertDynamicLights )
    {
        goto alloc_error;
    }
    memset(pWorld->aVertDynamicLights, 0, sizeof(rdVector4) * pWorld->numVertices);

    pWorld->aVertexRenderTickIds = (uint32_t*)STDMALLOC(sizeof(uint32_t) * pWorld->numVertices);
    if ( !pWorld->aVertexRenderTickIds )
    {
        goto alloc_error;
    }
    memset(pWorld->aVertexRenderTickIds, 0, sizeof(uint32_t) * pWorld->numVertices);

    pWorld->aVertDynamicLightsRenderTicks = (uint32_t*)STDMALLOC(sizeof(uint32_t) * pWorld->numVertices);
    if ( !pWorld->aVertDynamicLightsRenderTicks )
    {
        goto alloc_error;
    }
    memset(pWorld->aVertDynamicLightsRenderTicks, 0, sizeof(uint32_t) * pWorld->numVertices);

    // Init adjoints
    SithSurfaceAdjoin* pPrevAdjoin = NULL;
    SithSector* pPrevSector        = NULL;
    for ( size_t surfNum = 0; surfNum < pWorld->numSurfaces; ++surfNum )
    {
        SithSurface* pSurf = &pWorld->aSurfaces[surfNum];
        if ( pSurf->pAdjoin )
        {
            SithSurfaceAdjoin* pAdjoin = pSurf->pAdjoin;
            if ( pAdjoin->pMirrorAdjoin )
            {
                pAdjoin->pAdjoinSector = pAdjoin->pMirrorAdjoin->pAdjoinSurface->pSector;
            }

            if ( pPrevAdjoin && pPrevSector == pSurf->pSector )
            {
                pPrevAdjoin->pNextAdjoin = pAdjoin;
            }
            else
            {
                pSurf->pSector->pFirstAdjoin = pAdjoin;
            }

            pPrevAdjoin = pAdjoin;
            pPrevSector = pSurf->pSector;
        }
    }

    // Init players
    sithPlayer_PlacePlayers(pWorld);

     // Find floor for things

    bool bNoCurWorld = false;
    if ( !sithWorld_g_pCurrentWorld )
    {
        bNoCurWorld = true;
        sithWorld_g_pCurrentWorld = pWorld;
    }

    for ( size_t i = 0; i < pWorld->numThings; ++i )
    {
        SithThing* pThing = &pWorld->aThings[i];
        if ( pThing->type && pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & (SITH_PF_WALLSTICK | SITH_PF_FLOORSTICK)) != 0 )
        {
            sithPhysics_FindFloor(pThing, 1);
        }
    }

    if ( bNoCurWorld )
    {
        sithWorld_g_pCurrentWorld = NULL;
    }

    // Validate world
    if ( sithWorld_ValidateWorld(pWorld) )
    {
        SITHLOG_ERROR("Some required sections not found or loaded in %s.\n", pWorld->aName);
        return 1;
    }

    // Success
    pWorld->state |= SITH_WORLD_STATE_INITIALIZED;
    return 0;

alloc_error:
    SITHLOG_ERROR("Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_g_aLine, pWorld->aName);
    return 1;
}

int J3DAPI sithWorld_LoadEntryText(SithWorld* pWorld, const char* pFilename, int bSkipParsing)
{
    // Function reads NDY level file

    bValidCopyright = false;

    // Set world name
    const char* pName = stdFnames_FindMedName(pFilename);
    STD_STRCPY(pWorld->aName, pName);
    stdUtil_ToLower(pWorld->aName);

    uint32_t startTime = stdPlatform_GetTimeMsec();
    if ( !stdConffile_Open(pFilename) )
    {
        SITHLOG_ERROR("Error: Invalid load filename '%s'.\n", pFilename);
        return 1;
    }

    // Read NDY file sections
    while ( stdConffile_ReadLine() )
    {
        char aSection[64];
        if ( sscanf_s(stdConffile_g_aLine, " section: %s", aSection, (rsize_t)STD_ARRAYLEN(aSection)) == 1 )
        {
            STD_STRCPY(aCurSection, aSection);
            if ( sithWorld_ReadTextSection(pWorld, aSection, bSkipParsing) )
            {
                SITHLOG_ERROR("Warning: Section '%s' contained errors.\n", aSection);
                SITHLOG_ERROR("Load of file %s failed due to syntax errors.\n", pFilename);

                stdConffile_Close();
                return 1;
            }
        }
    }

    sithWorld_UpdateLoadProgress(100.0f);

    if ( !bValidCopyright )
    {
        SITHLOG_ERROR("Error: Load of file '%s' failed due to missing copyright.\n", pFilename);
        stdConffile_Close();
        return 1;
    }

    stdConffile_Close();

    uint32_t endTime = stdPlatform_GetTimeMsec();
    SITHLOG_STATUS("%f seconds to load %s.\n", (float)(endTime - startTime) / 1000.0f, pWorld->aName);

    return 0;
}

void J3DAPI sithWorld_Close(SithWorld* pWorld)
{
    SITH_ASSERTREL(bWorldStartup == true);
    if ( pWorld )
    {
        sithWorld_Free(pWorld);
    }
}

SithWorld* sithWorld_NewEntry(void)
{
    SithWorld* pWorld = (SithWorld*)STDMALLOC(sizeof(SithWorld));
    if ( !pWorld )
    {
        return NULL;
    }

    memset(pWorld, 0, sizeof(SithWorld));
    return pWorld;
}

void J3DAPI sithWorld_Free(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld);

    if ( pWorld->aPVS )
    {
        stdMemory_Free(pWorld->aPVS);
        pWorld->aPVS = NULL;
    }

    if ( pWorld->aThings )
    {
        sithThing_FreeWorldThings(pWorld);
    }

    if ( pWorld->aSectors )
    {
        sithSector_FreeWorldSectors(pWorld);
    }

    if ( pWorld->aModels )
    {
        sithModel_FreeWorldModels(pWorld);
    }

    if ( pWorld->aSprites )
    {
        sithSprite_FreeWorldSprites(pWorld);
    }

    if ( pWorld->aParticles )
    {
        sithParticle_FreeWorldParticles(pWorld);
    }

    if ( pWorld->aKeyframes )
    {
        sithPuppet_FreeWorldKeyframes(pWorld);
    }

    if ( pWorld->aThingTemplates )
    {
        sithTemplate_FreeWorldTemplates(pWorld);
    }

    if ( pWorld->aVertices )
    {
        stdMemory_Free(pWorld->aVertices);
        pWorld->aVertices = NULL;
    }

    if ( pWorld->aTransformedVertices )
    {
        stdMemory_Free(pWorld->aTransformedVertices);
        pWorld->aTransformedVertices = NULL;
    }

    if ( pWorld->aVertDynamicLights )
    {
        stdMemory_Free(pWorld->aVertDynamicLights);
        pWorld->aVertDynamicLights = NULL;
    }

    if ( pWorld->aVertDynamicLightsRenderTicks )
    {
        stdMemory_Free(pWorld->aVertDynamicLightsRenderTicks);
        pWorld->aVertDynamicLightsRenderTicks = NULL;
    }

    if ( pWorld->aTexVerticies )
    {
        stdMemory_Free(pWorld->aTexVerticies);
        pWorld->aTexVerticies = NULL;
    }

    if ( pWorld->aSurfaces )
    {
        sithSurface_FreeWorldSurfaces(pWorld);
    }

    if ( pWorld->aVertexRenderTickIds )
    {
        stdMemory_Free(pWorld->aVertexRenderTickIds);
        pWorld->aVertexRenderTickIds = NULL;
    }

    if ( pWorld->aMaterials )
    {
        sithMaterial_FreeWorldMaterials(pWorld);
    }

    sithSound_FreeWorldSounds(pWorld);
    if ( pWorld->aCogs || pWorld->aCogScripts )
    {
        sithCog_FreeWorldCogs(pWorld);
    }

    if ( pWorld->aPuppetClasses )
    {
        sithPuppet_FreeWorldPuppets(pWorld);
    }

    if ( pWorld->aAIClasses )
    {
        sithAIClass_FreeWorldAIClasses(pWorld);
    }

    if ( pWorld->aSoundClasses )
    {
        sithSoundClass_FreeWorldSoundClasses(pWorld);
    }

    stdMemory_Free(pWorld);
}

int J3DAPI sithWorld_ReadHeaderText(SithWorld* pWorld, int bSkip)
{
    if ( bSkip )
    {
        return 1;
    }

    uint32_t version;
    if ( stdConffile_ScanLine("version %d", &version) != 1 )
    {
        goto syntax_error;
    }

    if ( version < SITHWORLD_NDYMINVERSION || version > SITHWORLD_NDYMAXVERSION )
    {
        SITHLOG_WARNING("Warning: Invalid version %.2f for file '%s'.\n", (float)version, pWorld->aName); // Altered: Changed log level to warning from error
        goto syntax_error;
    }

    if ( stdConffile_ScanLine("world gravity %f", &pWorld->gravity) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine("ceiling sky z %f", &pWorld->ceilingSkyHeight) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine("horizon distance %f", &pWorld->horizonDistance) != 1 )
    {
        goto syntax_error;
    }

    float pixperRev;
    if ( stdConffile_ScanLine("horizon pixels per rev %f", &pixperRev) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine("horizon sky offset %f %f", &pWorld->horizonSkyOffset, &pWorld->horizonSkyOffset.y) != 2 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine("ceiling sky offset %f %f", &pWorld->ceilingSkyOffset, &pWorld->ceilingSkyOffset.y) != 2 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine("lod distances %f %f %f %f", pWorld->distancesLOD, &pWorld->distancesLOD[1], &pWorld->distancesLOD[2], &pWorld->distancesLOD[3]) != 4 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(
        "fog %d %f %f %f %f %f %f\n",
        &pWorld->fog.bEnabled,
        &pWorld->fog.color.red,
        &pWorld->fog.color.green,
        &pWorld->fog.color.blue,
        &pWorld->fog.color.alpha,
        &pWorld->fog.startDepth,
        &pWorld->fog.endDepth) != 7 )
    {
        goto syntax_error;
    }

    // Success
    return 0;

syntax_error:
    SITHLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, pWorld->aName);
    return 1;
}

int J3DAPI sithWorld_WriteHeaderText(const SithWorld* pWorld)
{
    return stdConffile_WriteLine("###### Header information ######\n")
        || stdConffile_WriteLine("SECTION: HEADER\n")
        || stdConffile_WriteLine("# version and global constant settings\n")
        || stdConffile_Printf("Version          %d\n", 3)
        || stdConffile_Printf("World Gravity    %.2f\n", pWorld->gravity)
        || stdConffile_Printf("Ceiling Sky Z    %.6f\n", pWorld->ceilingSkyHeight)
        || stdConffile_Printf("Horizon Distance %.6f\n", pWorld->horizonDistance)
        || stdConffile_Printf("Horizon Pixels per Rev %.6f\n", 768.0f)
        || stdConffile_Printf("Horizon Sky Offset   %.6f %.6f\n", pWorld->horizonSkyOffset.x, pWorld->horizonSkyOffset.y)
        || stdConffile_Printf("Ceiling Sky Offset   %.6f %.6f\n", pWorld->ceilingSkyOffset.x, pWorld->ceilingSkyOffset.y)
        || stdConffile_Printf(
            "LOD Distances\t\t%.6f\t%.6f\t%.6f\t%.6f\n",
            pWorld->distancesLOD[0],
            pWorld->distancesLOD[1],
            pWorld->distancesLOD[2],
            pWorld->distancesLOD[3])
        || stdConffile_Printf(
            "fog\t\t%d\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\n",
            pWorld->fog.bEnabled,
            pWorld->fog.color.x,
            pWorld->fog.color.y,
            pWorld->fog.color.z,
            pWorld->fog.color.w,
            pWorld->fog.startDepth,
            pWorld->fog.endDepth)
        || stdConffile_WriteLine("################################\n\n\n");
}

int J3DAPI sithWorld_ReadCopyrightText(SithWorld* pWorld, int bSkip)
{
    if ( bSkip )
    {
        return 1;
    }

    for ( size_t i = 0; i < SITHWORLD_NUMCOPYRIGHTLINES; ++i )
    {
        if ( !stdConffile_ReadLine() )
        {
            SITHLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, pWorld->aName);
            return 1;
        }

        memcpy(&aCopyrightBuf[SITHWORLD_COPYRIGHTLINELEN * i], stdConffile_g_aLine, SITHWORLD_COPYRIGHTLINELEN); // Note, must be exact size, no null term
    }

    if ( memcmp(aCopyrightBuf, aCopyrightSymbol, STD_ARRAYLEN(aCopyrightSymbol) - 1) != 0 )
    {
        SITHLOG_ERROR("Error: Invalid copyright symbol.\n", stdConffile_g_aLine, pWorld->aName);
        bValidCopyright = false;
        return 1;
    }

    // Success
    bValidCopyright = true;
    return 0;
}

void J3DAPI sithWorld_GetMemoryUsage(const SithWorld* pWorld, size_t(*aMemUsed)[17], size_t(*aCount)[17])
{
    SITH_ASSERTREL(aMemUsed && aCount);
    SITH_ASSERTREL(pWorld);

    memset(aMemUsed, 0, sizeof(*aMemUsed)); static_assert(sizeof(*aMemUsed) == 0x44, "");
    memset(aCount, 0, sizeof(*aCount)); static_assert(sizeof(*aCount) == 0x44, "");

    (*aCount)[0] = pWorld->numMaterials;
    for ( size_t i = 0; i < pWorld->numMaterials; ++i )
    {
        (*aMemUsed)[0] += rdMaterial_GetMaterialMemUsage(&pWorld->aMaterials[i]);
    }

    (*aCount)[1] = pWorld->numVertices;
    (*aMemUsed)[1] = (3 * sizeof(rdVector3) + sizeof(rdVector4)) * pWorld->numVertices; static_assert((3 * sizeof(rdVector3) + sizeof(rdVector4)) == 52, "");

    (*aCount)[2] = pWorld->numTexVertices;
    (*aMemUsed)[2] = sizeof(rdVector2) * pWorld->numTexVertices;

    (*aCount)[3] = pWorld->numSurfaces;
    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        (*aMemUsed)[3] += sizeof(rdVector3) * pWorld->aSurfaces[i].face.numVertices + sizeof(SithSurface);// TODO: Shouldn't it be (sizeof(rdVector3) + sizeof(rdVector2)) * numVertices like for sprite?
    }

    (*aCount)[4] = pWorld->numAdjoins;
    (*aMemUsed)[4] = sizeof(SithSurfaceAdjoin) * pWorld->numAdjoins;

    (*aCount)[5] = pWorld->numSectors;
    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        (*aMemUsed)[5] += sizeof(*pWorld->aSectors[i].aVertIdxs) * pWorld->aSectors[i].numVertices + sizeof(SithSector); static_assert(sizeof(*pWorld->aSectors[i].aVertIdxs) == 4, ""); // 4 - sizeof(int)
    }

    (*aCount)[6] = pWorld->numSounds;
    (*aMemUsed)[6] = Sound_GetMemoryUsage();

    (*aCount)[8] = pWorld->numCogScripts;
    for ( size_t i = 0; i < pWorld->numCogScripts; ++i )
    {
        (*aMemUsed)[8] += sizeof(*pWorld->aCogScripts[i].pCode) * pWorld->aCogScripts[i].codeSize + sizeof(SithCogScript); static_assert(sizeof(*pWorld->aCogScripts[i].pCode) == 4, ""); // 4 - sizeof(uint32_t)
        (*aMemUsed)[8] += sizeof(SithCogSymbol) * pWorld->aCogScripts[i].pSymbolTable->numUsedSymbols + sizeof(SithCogSymbolTable);
    }

    (*aCount)[7] = pWorld->numCogs;
    for ( size_t i = 0; i < pWorld->numCogs; ++i )
    {
        (*aMemUsed)[7] += sizeof(SithCogSymbol) * pWorld->aCogs[i].pSymbolTable->numUsedSymbols + sizeof(SithCog) + sizeof(SithCogSymbolTable); static_assert((sizeof(SithCog) + sizeof(SithCogSymbolTable)) == 0x5100, "");
    }

    (*aCount)[10] = pWorld->numModels;
    for ( size_t i = 0; i < pWorld->numModels; ++i )
    {
        (*aMemUsed)[10] += sithModel_GetModelMemUsage(&pWorld->aModels[i]);
    }

    (*aCount)[11] = pWorld->numKeyframes;
    for ( size_t i = 0; i < pWorld->numKeyframes; ++i )
    {
        (*aMemUsed)[11] += sizeof(rdKeyframe);
        (*aMemUsed)[11] += sizeof(rdKeyframeNode) * pWorld->aKeyframes[i].numJoints;
        for ( size_t j = 0; j < pWorld->aKeyframes[i].numJoints; ++j )
        {
            (*aMemUsed)[11] += sizeof(rdKeyframeNodeEntry) * pWorld->aKeyframes[i].aNodes[j].numEntries;
        }
    }

    (*aCount)[12] = pWorld->numPuppetClasses;
    (*aMemUsed)[12] = sizeof(SithPuppetClass) * pWorld->numPuppetClasses;

    (*aCount)[13] = pWorld->numSprites;
    (*aMemUsed)[13] = sizeof(rdSprite3) * pWorld->numSprites;
    for ( size_t i = 0; i < pWorld->numSprites; ++i )
    {
        (*aMemUsed)[13] +=
            (sizeof(rdVector3) + sizeof(rdVector2)) * pWorld->aSprites[i].face.numVertices; static_assert(sizeof(rdVector3) + sizeof(rdVector2) == 20, ""); //  sizeof(rdVector3) - offset ;  sizeof(rdVector2) - aTexVerts
    }

    (*aCount)[14] = pWorld->numThingTemplates;
    (*aMemUsed)[14] = sizeof(SithThing) * pWorld->numThingTemplates;

    (*aCount)[15] = pWorld->numThings;
    (*aMemUsed)[15] = sizeof(SithThing) * pWorld->numThings;
}

int J3DAPI sithWorld_RegisterTextSectionParser(const char* aSectionName, SithWorldTextSectionParseFunc pfParseFunction)
{
    int idx = sithWorld_GetTextSectionParserIndex(aSectionName);
    if ( idx == -1 )
    {
        if ( numTextSectionParseHandlers >= STD_ARRAYLEN(aTextSectioParseHandlers) )
        {
            return 1;
        }

        idx = numTextSectionParseHandlers++;
    }

    STD_STRCPY(aTextSectioParseHandlers[idx].aName, aSectionName);
    aTextSectioParseHandlers[idx].pfHandler = pfParseFunction;
    return 0;
}

void J3DAPI sithWorld_CalcSurfaceNormals(SithWorld* pWorld)
{
    for ( size_t surfNum = 0; surfNum < pWorld->numSurfaces; ++surfNum )
    {
        rdFace* pFace = &pWorld->aSurfaces[surfNum].face;
        SITH_ASSERTREL(pFace->numVertices > 2);

        size_t prevVertNum = 0, nextVertNum = 0;
        size_t vertNum = 0;
        for ( ; vertNum < pFace->numVertices; ++vertNum )
        {
            if ( vertNum == 0 )
            {
                prevVertNum = pFace->numVertices - 1;
            }
            else
            {
                prevVertNum = vertNum - 1;
            }

            nextVertNum = (vertNum + 1) % pFace->numVertices;
            if ( !rdMath_PointsCollinear(
                &pWorld->aVertices[pFace->aVertices[vertNum]],
                &pWorld->aVertices[pFace->aVertices[nextVertNum]],
                &pWorld->aVertices[pFace->aVertices[prevVertNum]]
            ) )
            {
                break;
            }
        }

        if ( vertNum >= pFace->numVertices )
        {
            SITHLOG_ERROR("Warning: Invalid Face encountered while calculating normals (surface #%d).\n", surfNum);
        }
        else
        {
            rdMath_CalcSurfaceNormal(
                &pFace->normal,
                &pWorld->aVertices[pFace->aVertices[vertNum]],
                &pWorld->aVertices[pFace->aVertices[nextVertNum]],
                &pWorld->aVertices[pFace->aVertices[prevVertNum]]);
        }
    }
}

void J3DAPI sithWorld_ResetRenderState(SithWorld* pWorld)
{
    SITH_ASSERTREL((pWorld != NULL));
    SITH_ASSERTREL((pWorld->aSectors != NULL));

    memset(pWorld->aVertexRenderTickIds, 0, sizeof(*pWorld->aVertexRenderTickIds) * pWorld->numVertices);
    memset(pWorld->aVertDynamicLightsRenderTicks, 0, sizeof(*pWorld->aVertDynamicLightsRenderTicks) * pWorld->numVertices);

    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        SithSector* pSec = &pWorld->aSectors[i];

        SithSurface* pCurSurf = pSec->pFirstSurface;
        for ( size_t j = 0; j < pSec->numSurfaces; ++j )
        {
            pCurSurf->renderTick = 0;
            ++pCurSurf;
        }

        pSec->renderTick = 0;
    }
}

void J3DAPI sithWorld_ResetGeoresource(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld);

    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        pWorld->aSurfaces[i].face.matCelNum = 0;
    }

    for ( size_t j = 0; j < pWorld->numSectors; ++j )
    {
        memset(&pWorld->aSectors[j].thrust, 0, sizeof(pWorld->aSectors[j].thrust));

        pWorld->aSectors[j].tint.red = 0.0f;
        pWorld->aSectors[j].tint.green = 0.0f;
        pWorld->aSectors[j].tint.blue = 0.0f;
    }
}

int J3DAPI sithWorld_ValidateWorld(const SithWorld* pWorld)
{
    if ( !pWorld->aThings && pWorld->numThings )
    {
        SITHLOG_ERROR("Problem with things array, should not be NULL.\n");
        return 1;
    }

    if ( !pWorld->aSprites && pWorld->numSprites )
    {
        SITHLOG_ERROR("Problem with spriates array, should not be NULL.\n");
        return 1;
    }

    if ( !pWorld->aModels && pWorld->numModels )
    {
        SITHLOG_ERROR("Problem with models array, should not be NULL.\n");
        return 1;
    }

    if ( !pWorld->aSectors || !pWorld->aSurfaces || !pWorld->aVertices )
    {
        SITHLOG_ERROR("A required geometry section is missing from the level file.\n");
        return 1;
    }

    if ( !sithSurface_ValidateWorldSurfaces(pWorld) )
    {
        SITHLOG_ERROR("Surface resources did not pass validation.\n");

        return 1;
    }

    // Success
    return 0;
}

uint32_t J3DAPI sithWorld_CalcWorldChecksum(SithWorld* pWorld, uint32_t seed)
{
    uint32_t checksum = seed;
    for ( size_t i = 0; i < pWorld->numCogScripts; ++i )
    {
        checksum = stdUtil_CalcChecksum((const uint8_t*)pWorld->aCogScripts[i].pCode, pWorld->aCogScripts[i].codeSize, checksum); // TODO: should the code size be multiplied by sizeof(uint32_t)?
    }

    checksum = stdUtil_CalcChecksum((const uint8_t*)pWorld->aVertices, sizeof(*pWorld->aVertices) * pWorld->numVertices, checksum); static_assert(sizeof(*pWorld->aVertices) == 12, "");

    for ( size_t i = 0; i < pWorld->numThingTemplates; ++i )
    {
        checksum = sithThing_CalcThingChecksum(&pWorld->aThingTemplates[i], checksum);
    }

    if ( sithWorld_g_pStaticWorld )
    {
        for ( size_t i = 0; i < sithWorld_g_pStaticWorld->numCogScripts; ++i )
        {
            checksum = stdUtil_CalcChecksum((const uint8_t*)sithWorld_g_pStaticWorld->aCogScripts[i].pCode, sithWorld_g_pStaticWorld->aCogScripts[i].codeSize, checksum); // TODO: should the code size be multiplied by sizeof(uint32_t)?
        }
    }

    SITHLOG_STATUS("Checksum computed:  Seed=%x, Result=%x.\n", seed, checksum);
    return checksum;
}

int J3DAPI sithWorld_InitPlayers(SithWorld* pWorld)
{
    J3D_UNUSED(pWorld);
    for ( size_t i = 1; i < sithPlayer_g_numPlayers; ++i )
    {
        sithPlayer_HidePlayer(i);
    }

    sithPlayer_SetLocalPlayer(SITHPLAYER_DEFAULTLOCALPLAYERNUM);
    return 1;
}

int J3DAPI sithWorld_ReadTextSection(SithWorld* pWorld, const char* aSectionName, int bSkip)
{
    SITH_ASSERTREL(pWorld);

    int handlerIdx = sithWorld_GetTextSectionParserIndex(aSectionName);
    if ( handlerIdx == -1 )
    {
        SITHLOG_ERROR("Warning: Unknown Section '%s'.\n", aSectionName);
        return 0;
    }

    stdPlatform_GetTimeMsec(); // ??
    if ( aTextSectioParseHandlers[handlerIdx].pfHandler(pWorld, bSkip) )
    {
        SITHLOG_ERROR("Error while parsing section %s.\n", aSectionName);
        return 1;
    }

    // Success
    stdPlatform_GetTimeMsec(); // ??
    return 0;
}

const char* sithWorld_GetCurReadSection(void)
{
    return aCurSection;
}

int J3DAPI sithWorld_GetTextSectionParserIndex(const char* aSectionName)
{
    for ( size_t i = 0; i < numTextSectionParseHandlers; ++i )
    {
        if ( strneqi(aTextSectioParseHandlers[i].aName, aSectionName, STD_ARRAYLEN(aTextSectioParseHandlers[i].aName)) )
        {
            return i;
        }
    }

    return -1;
}

int J3DAPI sithWorld_WriteGeoresourceText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("#### Geomtry Resources Info ####\n")
        || stdConffile_WriteLine("SECTION: GEORESOURCE\n\n")
        || stdConffile_WriteLine("#----- Vertices Subsection -----\n")
        || stdConffile_Printf("World vertices %d\n", pWorld->numVertices)
        || stdConffile_WriteLine("#num:\tvertex:\n") )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numVertices; ++i )
    {
        if ( stdConffile_Printf("%d:\t%f\t%f\t%f\n", i, pWorld->aVertices[i].x, pWorld->aVertices[i].y, pWorld->aVertices[i].z) )
        {
            return 1;
        }
    }

    if ( stdConffile_WriteLine("\n\n")
        || stdConffile_WriteLine("#-- Texture Verts Subsection ---\n")
        || stdConffile_Printf("World texture vertices %d\n", pWorld->numTexVertices)
        || stdConffile_WriteLine("#num:\tu:\tv:\n") )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numTexVertices; ++i )
    {
        if ( stdConffile_Printf("%d:\t%f\t%f\n", i, pWorld->aTexVerticies[i].x, pWorld->aTexVerticies[i].y) )
        {
            return 1;
        }
    }

    return stdConffile_WriteLine("\n\n")
        || sithSurface_WriteAdjoinsListText(pWorld)
        || sithSurface_WriteSurfacesListText(pWorld)
        || stdConffile_WriteLine("################################\n\n\n");
}

int J3DAPI sithWorld_ReadGeoresourceText(SithWorld* pWorld, int bSkip)
{
    if ( bSkip )
    {
        // TODO: should move cur file offset to the end of this section?
        return 1;
    }

    sithWorld_UpdateLoadProgress(50.0f);

    size_t numVerts;
    int nRead =  stdConffile_ScanLine(" world vertices %d", &numVerts);
    if ( nRead != 1 )
    {
        if ( nRead < 0 )
        {
            goto eof_error;
        }
        goto syntax_error;
    }

    pWorld->aVertices = (rdVector3*)STDMALLOC(sizeof(rdVector3) * numVerts);
    if ( !pWorld->aVertices )
    {
        goto alloc_error;
    }

    for ( size_t i = 0; i < numVerts; i++ )
    {
        rdVector3* pVert = &pWorld->aVertices[i];
        int entryNum;
        if ( nRead = stdConffile_ScanLine(" %d: %f %f %f", &entryNum, &pVert->x, &pVert->y, &pVert->z), nRead != 4 )
        {
            if ( nRead < 0 )
            {
                goto eof_error;
            }
            goto syntax_error;
        }
    }

    pWorld->numVertices = numVerts;

    size_t numTexVerts;
    if ( nRead = stdConffile_ScanLine(" world texture vertices %d", &numTexVerts), nRead != 1 )
    {
        if ( nRead < 0 )
        {
            goto eof_error;
        }
        goto syntax_error;
    }

    pWorld->aTexVerticies = (rdVector2*)STDMALLOC(sizeof(rdVector2) * numTexVerts);
    if ( !pWorld->aTexVerticies )
    {
        goto alloc_error;
    }

    for ( size_t i = 0; i < numTexVerts; ++i )
    {
        rdVector2* pTexVert =  &pWorld->aTexVerticies[i];
        int entryNum;
        if ( nRead = stdConffile_ScanLine(" %d: %f %f", &entryNum, &pTexVert->x, &pTexVert->y), nRead != 3 )
        {
            if ( nRead < 0 )
            {
                goto eof_error;
            }
            goto syntax_error;
        }
    }

    pWorld->numTexVertices = numTexVerts;

    // Read goeres succeed, now read surfaces
    return sithSurface_ReadSurfacesListText(pWorld, bSkip); // Altered, Last param was 0 changed to bSkip

eof_error:
    SITHLOG_ERROR("Error: Unexpected EOF in '%s'\n", pWorld->aName);
    return 1;

syntax_error:
    SITHLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, pWorld->aName);
    return 1;

alloc_error:
    SITHLOG_ERROR("Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_g_aLine, pWorld->aName);
    return 1;
}

int sithWorld_WriteCopyrightText(void)
{
    if ( stdConffile_WriteLine("#### Copyright information #####\n") || stdConffile_WriteLine("SECTION: COPYRIGHT\n") )
    {
        return 1;
    }

    for ( size_t i = 0; i < SITHWORLD_NUMCOPYRIGHTLINES; ++i )
    {
        char aLine[SITHWORLD_COPYRIGHTLINELEN];
        memcpy(aLine, &aCopyrightSymbol[SITHWORLD_COPYRIGHTLINELEN * i], SITHWORLD_COPYRIGHTLINELEN); // Note, must be exact size, no null term
        if ( stdConffile_Printf("%s\n", aLine) )
        {
            return 1;
        }
    }

    return stdConffile_WriteLine("################################\n\n\n") != 0;
}

int J3DAPI sithWorld_WriteGeoresourceBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t nWrite = sizeof(rdVector3) * pWorld->numVertices;
    if ( sith_g_pHS->pFileWrite(fh, pWorld->aVertices, nWrite) != nWrite )
    {
        return 1;
    }

    nWrite = sizeof(rdVector2) * pWorld->numTexVertices;
    return sith_g_pHS->pFileWrite(fh, pWorld->aTexVerticies, nWrite) != nWrite;
}

int J3DAPI sithWorld_ReadGeoresourceBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t nRead = sizeof(rdVector3) * pWorld->numVertices;
    pWorld->aVertices = (rdVector3*)STDMALLOC(nRead);
    if ( !pWorld->aVertices )
    {
        return 1;
    }

    if ( sith_g_pHS->pFileRead(fh, pWorld->aVertices, nRead) != nRead )
    {
        return 1;
    }

    nRead = sizeof(rdVector2) * pWorld->numTexVertices;
    pWorld->aTexVerticies = (rdVector2*)STDMALLOC(nRead);
    return !pWorld->aTexVerticies || sith_g_pHS->pFileRead(fh, pWorld->aTexVerticies, nRead) != nRead;
}

int J3DAPI sithWorld_WriteEntryBinary(SithWorld* pWorld, const char* pFilename)
{
    CndWorld cndWorld;
    memcpy(&cndWorld.aCopyright, aCopyrightSymbol, STD_ARRAYLEN(aCopyrightSymbol) - 1); // Must be exact size, no null term

    STD_STRCPY(cndWorld.aName, pFilename);
    stdUtil_ToLower(cndWorld.aName);

    cndWorld.state            = pWorld->state &= ~SITH_WORLD_STATE_INITIALIZED;
    cndWorld.version          = SITHWORLD_CNDVERSION;
    cndWorld.gravity          = pWorld->gravity;
    cndWorld.ceilingSkyHeight = pWorld->ceilingSkyHeight;
    cndWorld.horizonDistance  = pWorld->horizonDistance;
    memcpy(&cndWorld.fog, &pWorld->fog, sizeof(cndWorld.fog));
    memcpy(&cndWorld.horizonSkyOffset, &pWorld->horizonSkyOffset, sizeof(cndWorld.horizonSkyOffset));
    memcpy(&cndWorld.ceilingSkyOffset, &pWorld->ceilingSkyOffset, sizeof(cndWorld.ceilingSkyOffset));
    memcpy(cndWorld.distancesLOD, pWorld->distancesLOD, sizeof(cndWorld.distancesLOD));

    cndWorld.numSounds                     = pWorld->numSounds;
    cndWorld.numMaterials                  = pWorld->numMaterials;
    cndWorld.sizeMaterials                 = pWorld->sizeMaterials;
    cndWorld.aMaterials                    = 0;
    cndWorld.apMatArray                    = 0;
    cndWorld.numVertices                   = pWorld->numVertices;
    cndWorld.aVertices                     = 0;
    cndWorld.aTransformedVertices          = 0;
    cndWorld.aVertexRenderTickIds          = 0;
    cndWorld.numTexVertices                = pWorld->numTexVertices;
    cndWorld.aTexVerticies                 = 0;
    cndWorld.aVertDynamicLights            = 0;
    cndWorld.aVertDynamicLightsRenderTicks = 0;
    cndWorld.numAdjoins                    = pWorld->numAdjoins;
    cndWorld.aAdjoins                      = 0;
    cndWorld.numSurfaces                   = pWorld->numSurfaces;
    cndWorld.aSurfaces                     = 0;
    cndWorld.numSectors                    = pWorld->numSectors;
    cndWorld.aSectors                      = 0;
    cndWorld.numAIClasses                  = pWorld->numAIClasses;
    cndWorld.sizeAIClasses                 = pWorld->sizeAIClasses;
    cndWorld.aAIClasses                    = 0;
    cndWorld.numModels                     = pWorld->numModels;
    cndWorld.sizeModels                    = pWorld->sizeModels;
    cndWorld.aModels                       = 0;
    cndWorld.numSprites                    = pWorld->numSprites;
    cndWorld.sizeSprites                   = pWorld->sizeSprites;
    cndWorld.aSprites                      = 0;
    cndWorld.numKeyframes                  = pWorld->numKeyframes;
    cndWorld.sizeKeyframes                 = pWorld->sizeKeyframes;
    cndWorld.aKeyframes                    = 0;
    cndWorld.numPuppetClasses              = pWorld->numPuppetClasses;
    cndWorld.sizePuppetClasses             = pWorld->sizePuppetClasses;
    cndWorld.aPuppetClasses                = 0;
    cndWorld.numSoundClasses               = pWorld->numSoundClasses;
    cndWorld.sizeSoundClasses              = pWorld->sizeSoundClasses;
    cndWorld.aSoundClasses                 = 0;
    cndWorld.numCogScripts                 = pWorld->numCogScripts;
    cndWorld.sizeCogScripts                = pWorld->sizeCogScripts;
    cndWorld.aCogScripts                   = 0;
    cndWorld.numCogs                       = pWorld->numCogs;
    cndWorld.sizeCogs                      = pWorld->sizeCogs;
    cndWorld.aCogs                         = 0;
    cndWorld.numThingTemplates             = pWorld->numThingTemplates;
    cndWorld.sizeThingTemplates            = pWorld->sizeThingTemplates;
    cndWorld.aThingTemplates               = 0;
    cndWorld.numThings                     = pWorld->numThings;
    cndWorld.lastThingIdx                  = 0;
    cndWorld.aThings                       = 0;
    cndWorld.sizePVS                       = pWorld->sizePVS;
    cndWorld.aPVS                          = 0;

    tFileHandle fh = sith_g_pHS->pFileOpen(pFilename, "wb");
    if ( !fh )
    {
        goto error;
    }

    uint32_t fileSize; // don't change the type
    if ( sith_g_pHS->pFileWrite(fh, &fileSize, sizeof(fileSize)) != sizeof(fileSize) )
    {
        goto error;
    }

    if ( sith_g_pHS->pFileWrite(fh, &cndWorld, sizeof(CndWorld)) != sizeof(CndWorld) )
    {
        goto error;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(aBinarySectionParsers); ++i )
    {
        if ( aBinarySectionParsers[i].pfWrite(fh, pWorld) )
        {
            goto error;
        }
    }

    fileSize = sith_g_pHS->pFileTell(fh);
    int bError = sith_g_pHS->pFileSeek(fh, 0, 0);
    if ( bError || sith_g_pHS->pFileWrite(fh, &fileSize, sizeof(fileSize)) != sizeof(fileSize) )
    {
        goto error;
    }

    // Success
    if ( fh )
    {
        sith_g_pHS->pFileClose(fh);
    }

    return 0;

error:
    if ( fh )
    {
        sith_g_pHS->pFileClose(fh);
    }

    return 1;
}

int J3DAPI sithWorld_LoadEntryBinary(SithWorld* pWorld, const char* pFilePath)
{
    // Clear input world
    memset(pWorld, 0, sizeof(SithWorld));

    uint32_t startTime = stdPlatform_GetTimeMsec();

    // open cnd file
    tFileHandle fh = sith_g_pHS->pFileOpen(pFilePath, "rb");
    if ( !fh )
    {
        goto error;
    }

    sithWorld_UpdateLoadProgress(0.0f);

    uint32_t fileSize; // Don't change tpye
    if ( sith_g_pHS->pFileRead(fh, &fileSize, sizeof(fileSize)) != sizeof(fileSize) )
    {
        goto error;
    }

    float endPos = (float)fileSize / 100.0f;
    size_t curPos = sith_g_pHS->pFileTell(fh);

    float progress = (float)curPos / endPos;
    sithWorld_UpdateLoadProgress(progress);

    // Read world header
    // TODO: CndWorld object shouldn't be read directly to pWorld due to SithWorld struct having pointer types.
    //       Read CndWorld object to temp CndWorld and copy filed by field to pWorld
    if ( sith_g_pHS->pFileRead(fh, pWorld, sizeof(CndWorld)) != sizeof(CndWorld) )
    {
        goto error;
    }

    curPos = sith_g_pHS->pFileTell(fh);
    progress = (float)curPos / endPos;
    sithWorld_UpdateLoadProgress(progress);

    const char* pFilename = stdFnames_FindMedName(pFilePath);
    STD_STRCPY(pWorld->aName, pFilename);
    stdUtil_ToLower(pWorld->aName);

    if ( memcmp(pWorld->aCopyright, aCopyrightSymbol, STD_ARRAYLEN(aCopyrightSymbol) - 1) != 0 )
    {
        goto error;
    }

    if ( pWorld->version != SITHWORLD_CNDVERSION )
    {
        SITHLOG_ERROR("Warning: Invalid version %.2f for file '%s'.\n", (float)pWorld->version, pFilePath);
        goto error;
    }

    // Make sure world is not in initialized state
    pWorld->state &= ~SITH_WORLD_STATE_INITIALIZED;

    // Now read file sections
    for ( size_t i = 0; i < STD_ARRAYLEN(aBinarySectionParsers); ++i )
    {
        uint32_t sectionReadStartTime = stdPlatform_GetTimeMsec(); // ??

        curPos = sith_g_pHS->pFileTell(fh);
        progress = (float)curPos / endPos;
        sithWorld_UpdateLoadProgress(progress);
        STD_STRCPY(aCurSection, aBinarySectionParsers[i].pName);

        Sleep(0); // ??
                  // From win32 docs: A value of zero causes the thread to relinquish the remainder of its time slice to any other thread
                  //                 that is ready to run. If there are no other threads ready to run, the function returns immediately, and the thread continues execution.

        if ( aBinarySectionParsers[i].pfRead(fh, pWorld) )
        {
            goto error;
        }

        uint32_t sectionReadEndTime = stdPlatform_GetTimeMsec(); // ??
    }

    if ( fh )
    {
        sith_g_pHS->pFileClose(fh);
    }

    // Success
    uint32_t endTime = stdPlatform_GetTimeMsec();
    SITHLOG_STATUS("%f seconds to load %s.\n", (float)(endTime - startTime) / 1000.0f, pWorld->aName);

    sithWorld_UpdateLoadProgress(100.0f);
    return 0;

error:
    if ( fh )
    {
        sith_g_pHS->pFileClose(fh);
    }

    return 1;
}
