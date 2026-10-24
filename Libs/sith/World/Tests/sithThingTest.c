#include <unity_fixture.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <rdroid/Math/rdMatrix.h>

#include <sith/Main/sithMain.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include "sithThingTestSupport.h"

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdUtil.h>

#define SITHTHING_TEST_BINARY_HANDLE  ((tFileHandle)1u)
#define SITHTHING_TEST_FILE_CAPACITY (64u * 1024u)
#define SITHTHING_TEST_MAX_HASH_ITEMS 80u
#define SITHTHING_TEST_MAX_HASHTABLES  4u
#define SITHTHING_TEST_NO_FAILURE    0u
#define SITHTHING_TEST_PATHSIZE      1024u

typedef struct sSithThingTestFile
{
    uint8_t aData[SITHTHING_TEST_FILE_CAPACITY];
    size_t size;
    size_t pos;
    size_t numIOCalls;
    size_t failIOCall;
    int bOverflow;
} SithThingTestFile;

typedef struct sSithThingTestBinaryView
{
    const CndThingInfo* aThingInfos;
    const int32_t* aCounts;
    const CndPhysicsInfo* aPhysicsInfos;
    const int32_t* aNumPathFrames;
    const SithPathFrame* aPathFrames;
    const CndActorInfo* aActorInfos;
    const CndWeaponInfo* aWeaponInfos;
    const CndExplosionInfo* aExplosionInfos;
    const CndItemInfo* aItemInfos;
    const float* aUserValues;
    const CndParticleInfo* aParticleInfos;
    const CndAIControlInfo* aAIControlInfos;
    const rdVector3* aAIPathFrames;
} SithThingTestBinaryView;

typedef struct sSithThingTestHashItem
{
    const char* pName;
    void* pData;
} SithThingTestHashItem;

typedef struct sSithThingTestHashStore
{
    tHashTable table;
    SithThingTestHashItem aItems[SITHTHING_TEST_MAX_HASH_ITEMS];
    size_t numItems;
    int bInUse;
} SithThingTestHashStore;

enum
{
    SITHTHING_TEST_NUMPHYSICSINFOS = 0,
    SITHTHING_TEST_NUMPATHMOVES,
    SITHTHING_TEST_NUMPATHFRAMES,
    SITHTHING_TEST_NUMACTORINFOS,
    SITHTHING_TEST_NUMWEAPONINFOS,
    SITHTHING_TEST_NUMEXPLOSIONINFOS,
    SITHTHING_TEST_NUMITEMINFOS,
    SITHTHING_TEST_NUMPARTICLEINFOS,
    SITHTHING_TEST_NUMUSERVALUES,
    SITHTHING_TEST_NUMAICONTROLINFOS,
    SITHTHING_TEST_NUMAIPATHFRAMES,
    SITHTHING_TEST_NUMCOUNTS
};

static SithThingTestFile sithThingTest_file;
static uint8_t sithThingTest_aGoldenData[SITHTHING_TEST_FILE_CAPACITY];
static size_t sithThingTest_numAllocCalls;
static size_t sithThingTest_failAllocCall;
static size_t sithThingTest_numLiveAllocs;
static size_t sithThingTest_maxAllocationSize;
static tHostServices sithThingTest_hostServices;
#if defined(_DEBUG)
static size_t sithThingTest_previousDebugFillThreshold;
#endif

tHostServices* sith_g_pHS;
tHostServices* std_g_pHS;

#if defined(J3D_LOCAL_SITH_RTI)
SithWorld* sithWorld_g_pCurrentWorld;
SithWorld* sithWorld_g_pStaticWorld;
SithWorld* sithWorld_g_pLastLoadedWorld;
int sithWorld_g_bLoading;
#endif

const rdMatrix34 rdroid_g_identMatrix34 = {
    { { 1.0f }, { 0.0f }, { 0.0f } },
    { { 0.0f }, { 1.0f }, { 0.0f } },
    { { 0.0f }, { 0.0f }, { 1.0f } },
    { { 0.0f }, { 0.0f }, { 0.0f } }
};

static SithWorld sithThingTest_world;
static SithSector sithThingTest_aSectors[2];
static SithThing sithThingTest_aThings[SITH_THING_NUMTYPES];
static SithThing sithThingTest_aReadThings[SITH_THING_NUMTYPES];
static SithPathFrame sithThingTest_aPathFrames[3];
static rdVector3 sithThingTest_aAIPathFrames[2];
static SithAIControlBlock sithThingTest_aAIControls[3];
static SithAIControlBlock sithThingTest_aReadAIControls[3];
static SithAIClass sithThingTest_aiClass;
static SithWorld sithThingTest_staticWorld;
static SithSector sithThingTest_aStaticSectors[2];

static SithThing sithThingTest_baseTemplate;
static SithThing sithThingTest_sameResourceTemplate;
static SithThing sithThingTest_weaponTemplate;
static SithThing sithThingTest_explodeTemplate;
static SithThing sithThingTest_explosionTemplate;
static SithThing sithThingTest_aDebrisTemplates[16];
static rdModel3 sithThingTest_model;
static rdModel3 sithThingTest_sameModel;
static rdSprite3 sithThingTest_sprite;
static rdParticle sithThingTest_particle;
static rdMaterial sithThingTest_material;
static rdPuppet sithThingTest_rdPuppet;
static SithPuppetClass sithThingTest_puppetClass;
static SithSoundClass sithThingTest_soundClass;
static SithCogScript sithThingTest_cogScript;
static SithCog sithThingTest_cog;
static SithCogScript sithThingTest_sameCogScript;
static SithCog sithThingTest_sameCog;

static size_t sithThingTest_numInitCallbacks;
typedef struct sSithThingTestLoadState
{
    int bFailTemplates;
    int bFailModels;
    int bFailSprites;
    int bFailParticles;
    int bFailPuppets;
    int bFailSoundClasses;
    int bFailCogs;
    int bFailMaterials;
    int bFailAIClasses;
} SithThingTestLoadState;

static SithThingTestLoadState sithThingTest_loadState;
static SithThingTestHashStore sithThingTest_aHashStores[SITHTHING_TEST_MAX_HASHTABLES];
static int sithThingTest_performanceLevel;
static int sithThingTest_bConffileOpen;

TEST_GROUP(sithThingBinary);

static SithThingTestHashStore* sithThingTest_FindHashStore(const tHashTable* pTable)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aHashStores); ++i )
    {
        if ( sithThingTest_aHashStores[i].bInUse && &sithThingTest_aHashStores[i].table == pTable )
        {
            return &sithThingTest_aHashStores[i];
        }
    }
    return NULL;
}

void sithThingTest_ResetHashtables(void)
{
    STD_ZEROMEM(sithThingTest_aHashStores, sizeof(sithThingTest_aHashStores));
}

tHashTable* J3DAPI sithThingTest_HashtblNew(size_t size)
{
    (void)size;
    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aHashStores); ++i )
    {
        SithThingTestHashStore* pStore = &sithThingTest_aHashStores[i];
        if ( !pStore->bInUse )
        {
            STD_ZEROMEM(pStore, sizeof(*pStore));
            pStore->bInUse = 1;
            return &pStore->table;
        }
    }

    return NULL;
}

void J3DAPI sithThingTest_HashtblFree(tHashTable* pTable)
{
    SithThingTestHashStore* pStore = sithThingTest_FindHashStore(pTable);
    TEST_ASSERT_NOT_NULL(pStore);
    STD_ZEROMEM(pStore, sizeof(*pStore));
}

int J3DAPI sithThingTest_HashtblAdd(tHashTable* pTable, const char* pName, void* pData)
{
    SithThingTestHashStore* pStore = sithThingTest_FindHashStore(pTable);
    TEST_ASSERT_NOT_NULL(pStore);
    TEST_ASSERT_NOT_NULL(pName);
    TEST_ASSERT_NOT_NULL(pData);
    TEST_ASSERT_TRUE(pStore->numItems < STD_ARRAYLEN(pStore->aItems));

    for ( size_t i = 0; i < pStore->numItems; ++i )
    {
        if ( strcmp(pStore->aItems[i].pName, pName) == 0 )
        {
            return 0;
        }
    }

    pStore->aItems[pStore->numItems].pName = pName;
    pStore->aItems[pStore->numItems].pData = pData;
    ++pStore->numItems;
    return 1;
}

void* J3DAPI sithThingTest_HashtblFind(const tHashTable* pTable, const char* pName)
{
    SithThingTestHashStore* pStore = sithThingTest_FindHashStore(pTable);
    TEST_ASSERT_NOT_NULL(pStore);
    TEST_ASSERT_NOT_NULL(pName);

    for ( size_t i = 0; i < pStore->numItems; ++i )
    {
        if ( strcmp(pStore->aItems[i].pName, pName) == 0 )
        {
            return pStore->aItems[i].pData;
        }
    }
    return NULL;
}

int J3DAPI sithThingTest_HashtblRemove(tHashTable* pTable, const char* pName)
{
    SithThingTestHashStore* pStore = sithThingTest_FindHashStore(pTable);
    TEST_ASSERT_NOT_NULL(pStore);
    TEST_ASSERT_NOT_NULL(pName);

    for ( size_t i = 0; i < pStore->numItems; ++i )
    {
        if ( strcmp(pStore->aItems[i].pName, pName) == 0 )
        {
            size_t numRemaining = pStore->numItems - i - 1u;
            for ( size_t itemNum = 0; itemNum < numRemaining; ++itemNum )
            {
                pStore->aItems[i + itemNum] = pStore->aItems[i + itemNum + 1u];
            }
            --pStore->numItems;
            STD_ZEROMEM(&pStore->aItems[pStore->numItems], sizeof(pStore->aItems[0]));
            return 1;
        }
    }
    return 0;
}

int J3DAPI sithThingTest_ConfigGetInt(const char* pKey, int defaultValue)
{
    (void)pKey;
    (void)defaultValue;
    return 0;
}

bool sithThingTest_ConfigContains(const char* pKey)
{
    (void)pKey;
    return true;
}

bool J3DAPI sithThingTest_ConfigSetInt(const char* pKey, int value)
{
    (void)pKey;
    (void)value;
    return true;
}

int sithThingTest_GetPerformanceLevel(void)
{
    return sithThingTest_performanceLevel;
}

int J3DAPI sithThingTest_IsSphereInSector(const SithWorld* pWorld, const rdVector3* pPos, float radius, const SithSector* pSector)
{
    TEST_ASSERT_NOT_NULL(pWorld);
    TEST_ASSERT_NOT_NULL(pPos);
    TEST_ASSERT_TRUE(radius >= 0.0f);
    TEST_ASSERT_TRUE(pSector >= pWorld->aSectors);
    TEST_ASSERT_TRUE(pSector < &pWorld->aSectors[pWorld->numSectors]);
    return 1;
}

void J3DAPI sithThingTest_StopAllSoundsThing(const SithThing* pThing)
{
    TEST_ASSERT_NOT_NULL(pThing);
}

int J3DAPI sithThingTest_IsGameActive(void)
{
    return 0;
}

int J3DAPI sithThingTest_IsGameHost(void)
{
    return 0;
}

SithThing* J3DAPI sithThingTest_GetTemplate(const char* pName)
{
    if ( sithThingTest_loadState.bFailTemplates || !pName || !pName[0] )
    {
        return NULL;
    }

    SithThing* apTemplates[] = {
        &sithThingTest_baseTemplate,
        &sithThingTest_sameResourceTemplate,
        &sithThingTest_weaponTemplate,
        &sithThingTest_explodeTemplate,
        &sithThingTest_explosionTemplate
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(apTemplates); ++i )
    {
        if ( strcmp(pName, apTemplates[i]->aName) == 0 )
        {
            return apTemplates[i];
        }
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aDebrisTemplates); ++i )
    {
        if ( strcmp(pName, sithThingTest_aDebrisTemplates[i].aName) == 0 )
        {
            return &sithThingTest_aDebrisTemplates[i];
        }
    }

    // Template-list tests resolve bases through the production template cache.
    return sithTemplate_GetTemplate(pName);
}

rdModel3* J3DAPI sithThingTest_LoadModel(const char* pName, int bSkipDefault)
{
    (void)bSkipDefault;
    if ( sithThingTest_loadState.bFailModels || strcmp(pName, sithThingTest_model.aName) != 0 )
    {
        return NULL;
    }
    return &sithThingTest_model;
}

rdSprite3* J3DAPI sithThingTest_LoadSprite(SithWorld* pWorld, const char* pName)
{
    (void)pWorld;
    if ( sithThingTest_loadState.bFailSprites || strcmp(pName, sithThingTest_sprite.aName) != 0 )
    {
        return NULL;
    }
    return &sithThingTest_sprite;
}

rdParticle* J3DAPI sithThingTest_LoadParticle(SithWorld* pWorld, const char* pName)
{
    (void)pWorld;
    if ( sithThingTest_loadState.bFailParticles || strcmp(pName, sithThingTest_particle.aName) != 0 )
    {
        return NULL;
    }
    return &sithThingTest_particle;
}

SithPuppetClass* J3DAPI sithThingTest_LoadPuppetClass(const char* pName)
{
    if ( sithThingTest_loadState.bFailPuppets || strcmp(pName, sithThingTest_puppetClass.aName) != 0 )
    {
        return NULL;
    }
    return &sithThingTest_puppetClass;
}

SithSoundClass* J3DAPI sithThingTest_LoadSoundClass(SithWorld* pWorld, const char* pName)
{
    (void)pWorld;
    if ( sithThingTest_loadState.bFailSoundClasses || strcmp(pName, sithThingTest_soundClass.aName) != 0 )
    {
        return NULL;
    }
    return &sithThingTest_soundClass;
}

SithCog* J3DAPI sithThingTest_LoadCog(SithWorld* pWorld, const char* pName)
{
    (void)pWorld;
    if ( sithThingTest_loadState.bFailCogs || strcmp(pName, sithThingTest_cogScript.aName) != 0 )
    {
        return NULL;
    }
    return &sithThingTest_cog;
}

rdMaterial* J3DAPI sithThingTest_LoadMaterial(const char* pName)
{
    if ( sithThingTest_loadState.bFailMaterials || strcmp(pName, sithThingTest_material.aName) != 0 )
    {
        return NULL;
    }
    return &sithThingTest_material;
}

SithAIClass* J3DAPI sithThingTest_LoadAIClass(SithWorld* pWorld, const char* pName)
{
    (void)pWorld;
    if ( sithThingTest_loadState.bFailAIClasses || strcmp(pName, sithThingTest_aiClass.aName) != 0 )
    {
        return NULL;
    }
    return &sithThingTest_aiClass;
}

void J3DAPI sithThingTest_FreeRdThingEntry(rdThing* pThing)
{
    pThing->pPuppet                = NULL;
    pThing->paJointMatrices        = NULL;
    pThing->apTweakedAngles        = NULL;
    pThing->paJointAmputationFlags = NULL;
}

int J3DAPI sithThingTest_SetModel3(rdThing* pThing, rdModel3* pModel3)
{
    pThing->type         = RD_THING_MODEL3;
    pThing->data.pModel3 = pModel3;
    pThing->geosetNum    = -1;
    return 1;
}

int J3DAPI sithThingTest_SetSprite3(rdThing* pThing, rdSprite3* pSprite3)
{
    pThing->type          = RD_THING_SPRITE3;
    pThing->data.pSprite3 = pSprite3;
    pThing->matCelNum     = -1;
    return 1;
}

int J3DAPI sithThingTest_SetParticleCloud(rdThing* pThing, rdParticle* pParticle)
{
    pThing->type           = RD_THING_PARTICLE;
    pThing->data.pParticle = pParticle;
    return 1;
}

rdPuppet* J3DAPI sithThingTest_NewRdPuppet(rdThing* pThing)
{
    pThing->pPuppet = &sithThingTest_rdPuppet;
    return pThing->pPuppet;
}

static void J3DAPI sithThingTest_Assert(const char* pText, const char* pFile, int line)
{
    (void)pFile;
    (void)line;
    TEST_FAIL_MESSAGE(pText);
}

static int J3DAPI sithThingTest_Print(const char* pFormat, ...)
{
    (void)pFormat;
    return 0;
}

void* J3DAPI sithThingTest_Malloc(size_t size)
{
    ++sithThingTest_numAllocCalls;
    if ( sithThingTest_failAllocCall == sithThingTest_numAllocCalls || size > sithThingTest_maxAllocationSize )
    {
        return NULL;
    }

    void* pMemory = malloc(size ? size : 1u);
    if ( pMemory )
    {
        ++sithThingTest_numLiveAllocs;
    }
    return pMemory;
}

void J3DAPI sithThingTest_Free(void* pMemory)
{
    if ( pMemory )
    {
        TEST_ASSERT_TRUE(sithThingTest_numLiveAllocs > 0u);
        --sithThingTest_numLiveAllocs;
    }
    free(pMemory);
}

void* J3DAPI sithThingTest_MemoryMalloc(size_t size, const char* pFile, size_t line)
{
    (void)pFile;
    (void)line;
    return sithThingTest_Malloc(size);
}

void J3DAPI sithThingTest_MemoryFree(void* pMemory)
{
    sithThingTest_Free(pMemory);
}

int J3DAPI sithThingTest_AllocAIFrames(SithAIControlBlock* pLocal, size_t sizeFrames)
{
    pLocal->aFrames = (rdVector3*)sithThingTest_Malloc(sizeof(*pLocal->aFrames) * sizeFrames);
    if ( !pLocal->aFrames )
    {
        return 1;
    }

    STD_ZEROMEM(pLocal->aFrames, sizeof(*pLocal->aFrames) * sizeFrames);
    pLocal->sizeFrames = sizeFrames;
    pLocal->numFrames  = 0;
    return 0;
}

static tFileHandle J3DAPI sithThingTest_FileOpen(const char* pFilename, const char* pMode)
{
    FILE* pFile = fopen(pFilename, pMode);
    return (tFileHandle)(uintptr_t)pFile;
}

static int J3DAPI sithThingTest_FileClose(tFileHandle fh)
{
    if ( !fh || fh == SITHTHING_TEST_BINARY_HANDLE )
    {
        return -1;
    }
    return fclose((FILE*)(uintptr_t)fh);
}

static size_t J3DAPI sithThingTest_Read(tFileHandle fh, void* pData, size_t size)
{
    if ( fh != SITHTHING_TEST_BINARY_HANDLE )
    {
        return fread(pData, 1u, size, (FILE*)(uintptr_t)fh);
    }

    ++sithThingTest_file.numIOCalls;
    if ( sithThingTest_file.failIOCall == sithThingTest_file.numIOCalls )
    {
        return size ? size - 1u : 1u;
    }

    size_t available = sithThingTest_file.size - sithThingTest_file.pos;
    size_t readSize  = size < available ? size : available;
    if ( readSize )
    {
        STD_COPYMEM(pData, &sithThingTest_file.aData[sithThingTest_file.pos], readSize);
        sithThingTest_file.pos += readSize;
    }
    return readSize;
}

static size_t J3DAPI sithThingTest_Write(tFileHandle fh, const void* pData, size_t size)
{
    if ( fh != SITHTHING_TEST_BINARY_HANDLE )
    {
        return fwrite(pData, 1u, size, (FILE*)(uintptr_t)fh);
    }

    ++sithThingTest_file.numIOCalls;
    if ( sithThingTest_file.failIOCall == sithThingTest_file.numIOCalls )
    {
        return size ? size - 1u : 1u;
    }

    if ( size > sizeof(sithThingTest_file.aData) - sithThingTest_file.size )
    {
        sithThingTest_file.bOverflow = 1;
        return 0;
    }

    if ( size )
    {
        STD_COPYMEM(&sithThingTest_file.aData[sithThingTest_file.size], pData, size);
        sithThingTest_file.size += size;
    }
    return size;
}

static char* J3DAPI sithThingTest_FileGets(tFileHandle fh, char* pString, size_t size)
{
    if ( !fh || fh == SITHTHING_TEST_BINARY_HANDLE || size > INT_MAX )
    {
        return NULL;
    }
    return fgets(pString, (int)size, (FILE*)(uintptr_t)fh);
}

static void sithThingTest_ResetFile(void)
{
    STD_ZEROMEM(&sithThingTest_file, sizeof(sithThingTest_file));
}

static size_t sithThingTest_LoadGoldenData(const char* pFileName, uint8_t* pData, size_t capacity)
{
    char aPath[SITHTHING_TEST_PATHSIZE];
    STD_FORMAT(aPath, "%s/sithThing/%s", SITH_TEST_TV_DIR, pFileName);

    FILE* pFile = fopen(aPath, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(pFile, aPath);

    size_t size = fread(pData, 1u, capacity, pFile);
    TEST_ASSERT_FALSE_MESSAGE(ferror(pFile), aPath);
    TEST_ASSERT_EQUAL_INT_MESSAGE(EOF, fgetc(pFile), "SithThing golden vector exceeds the test buffer.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(pFile), aPath);
    return size;
}

static void sithThingTest_AssertMatchesGolden(const char* pFileName)
{
    size_t goldenSize = sithThingTest_LoadGoldenData(pFileName, sithThingTest_aGoldenData, sizeof(sithThingTest_aGoldenData));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(goldenSize, sithThingTest_file.size, pFileName);
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(sithThingTest_aGoldenData, sithThingTest_file.aData, goldenSize, pFileName);
}

static void sithThingTest_LoadGoldenFile(const char* pFileName)
{
    sithThingTest_ResetFile();
    sithThingTest_file.size = sithThingTest_LoadGoldenData(
        pFileName,
        sithThingTest_file.aData,
        sizeof(sithThingTest_file.aData)
    );
}

static void sithThingTest_RewindFile(void)
{
    sithThingTest_file.pos        = 0;
    sithThingTest_file.numIOCalls = 0;
    sithThingTest_file.failIOCall = SITHTHING_TEST_NO_FAILURE;
}

static void sithThingTest_MakeVectorPath(char* pPath, size_t pathSize, const char* pFileName)
{
    stdUtil_Format(pPath, pathSize, "%s/sithThing/%s", SITH_TEST_TV_DIR, pFileName);
    pPath[pathSize - 1u] = '\0';
}

static void sithThingTest_OpenConffile(const char* pFileName)
{
    char aPath[SITHTHING_TEST_PATHSIZE];
    sithThingTest_MakeVectorPath(aPath, STD_ARRAYLEN(aPath), pFileName);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, stdConffile_Open(aPath), aPath);
    sithThingTest_bConffileOpen = 1;
}

static void sithThingTest_CloseConffile(void)
{
    TEST_ASSERT_TRUE(sithThingTest_bConffileOpen);
    stdConffile_Close();
    sithThingTest_bConffileOpen = 0;
}

static void sithThingTest_InitStaticWorld(size_t numThings)
{
    STD_ZEROMEM(&sithThingTest_staticWorld, sizeof(sithThingTest_staticWorld));
    STD_ZEROMEM(sithThingTest_aStaticSectors, sizeof(sithThingTest_aStaticSectors));
    sithThingTest_staticWorld.aSectors   = sithThingTest_aStaticSectors;
    sithThingTest_staticWorld.numSectors = STD_ARRAYLEN(sithThingTest_aStaticSectors);
    sithThingTest_staticWorld.numThings  = numThings;
    sithWorld_g_pCurrentWorld            = &sithThingTest_staticWorld;
}

static void sithThingTest_FreeStaticWorld(void)
{
    if ( sithThingTest_staticWorld.aThings )
    {
        for ( size_t i = 0; i < sithThingTest_staticWorld.numThings; ++i )
        {
            SithThing* pThing = &sithThingTest_staticWorld.aThings[i];
            if ( pThing->moveType == SITH_MT_PATH && pThing->moveInfo.pathMovement.aFrames )
            {
                sithThingTest_Free(pThing->moveInfo.pathMovement.aFrames);
                pThing->moveInfo.pathMovement.aFrames = NULL;
            }

            if ( pThing->controlType == SITH_CT_AI
                && pThing->controlInfo.aiControl.pLocal
                && pThing->controlInfo.aiControl.pLocal->aFrames )
            {
                sithThingTest_Free(pThing->controlInfo.aiControl.pLocal->aFrames);
                pThing->controlInfo.aiControl.pLocal->aFrames = NULL;
            }
        }

        sithThingTest_Free(sithThingTest_staticWorld.aThings);
        sithThingTest_staticWorld.aThings   = NULL;
        sithThingTest_staticWorld.numThings = 0;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aStaticSectors); ++i )
    {
        sithThingTest_aStaticSectors[i].pFirstThingInSector = NULL;
    }

    if ( sithWorld_g_pCurrentWorld == &sithThingTest_staticWorld )
    {
        sithWorld_g_pCurrentWorld = NULL;
    }
}

void sithThingTest_ResetAllocationFailure(void)
{
    sithThingTest_numAllocCalls     = 0;
    sithThingTest_failAllocCall     = SITHTHING_TEST_NO_FAILURE;
    sithThingTest_maxAllocationSize = SIZE_MAX;
}

void sithThingTest_SetFailAllocCall(size_t failCall)
{
    sithThingTest_failAllocCall = failCall;
}

size_t sithThingTest_GetNumAllocCalls(void)
{
    return sithThingTest_numAllocCalls;
}

size_t sithThingTest_GetNumLiveAllocs(void)
{
    return sithThingTest_numLiveAllocs;
}

void sithThingTest_ResetLoadState(void)
{
    STD_ZEROMEM(&sithThingTest_loadState, sizeof(sithThingTest_loadState));
}

static void sithThingTest_SetIdentity(rdMatrix34* pOrient)
{
    STD_ZEROMEM(pOrient, sizeof(*pOrient));
    pOrient->rvec.x = 1.0f;
    pOrient->lvec.y = 1.0f;
    pOrient->uvec.z = 1.0f;
}

static void sithThingTest_SetVector3(rdVector3* pVector, float x, float y, float z)
{
    pVector->x = x;
    pVector->y = y;
    pVector->z = z;
}

static void sithThingTest_SetVector4(rdVector4* pVector, float x, float y, float z, float w)
{
    pVector->x = x;
    pVector->y = y;
    pVector->z = z;
    pVector->w = w;
}

static void sithThingTest_SetActorInfo(SithActorInfo* pActor, float baseValue)
{
    pActor->flags           = (SithActorFlag)(0x1000u + (uint32_t)baseValue);
    pActor->health          = baseValue + 1.0f;
    pActor->maxHealth       = baseValue + 2.0f;
    pActor->maxThrust       = baseValue + 3.0f;
    pActor->maxRotVelocity  = baseValue + 4.0f;
    pActor->maxHeadVelocity = baseValue + 5.0f;
    pActor->maxHeadYaw      = baseValue + 6.0f;
    pActor->jumpSpeed       = baseValue + 7.0f;
    sithThingTest_SetVector3(&pActor->eyeOffset, baseValue + 8.0f, baseValue + 9.0f, baseValue + 10.0f);
    pActor->minHeadPitch = baseValue + 11.0f;
    pActor->maxHeadPitch = baseValue + 12.0f;
    sithThingTest_SetVector3(&pActor->fireOffset, baseValue + 13.0f, baseValue + 14.0f, baseValue + 15.0f);
    sithThingTest_SetVector3(&pActor->lightOffset, baseValue + 16.0f, baseValue + 17.0f, baseValue + 18.0f);
    sithThingTest_SetVector4(&pActor->headLightIntensity, baseValue + 19.0f, baseValue + 20.0f, baseValue + 21.0f, baseValue + 22.0f);
    sithThingTest_SetVector4(&pActor->voiceInfo.voiceColor.top, baseValue + 23.0f, baseValue + 24.0f, baseValue + 25.0f, baseValue + 26.0f);
    sithThingTest_SetVector4(&pActor->voiceInfo.voiceColor.middle, baseValue + 27.0f, baseValue + 28.0f, baseValue + 29.0f, baseValue + 30.0f);
    sithThingTest_SetVector4(&pActor->voiceInfo.voiceColor.bottomLeft, baseValue + 31.0f, baseValue + 32.0f, baseValue + 33.0f, baseValue + 34.0f);
    sithThingTest_SetVector4(&pActor->voiceInfo.voiceColor.bottomRight, baseValue + 35.0f, baseValue + 36.0f, baseValue + 37.0f, baseValue + 38.0f);
}

static void sithThingTest_InitResources(void)
{
    STD_ZEROMEM(&sithThingTest_baseTemplate, sizeof(sithThingTest_baseTemplate));
    STD_ZEROMEM(&sithThingTest_sameResourceTemplate, sizeof(sithThingTest_sameResourceTemplate));
    STD_ZEROMEM(&sithThingTest_weaponTemplate, sizeof(sithThingTest_weaponTemplate));
    STD_ZEROMEM(&sithThingTest_explodeTemplate, sizeof(sithThingTest_explodeTemplate));
    STD_ZEROMEM(&sithThingTest_explosionTemplate, sizeof(sithThingTest_explosionTemplate));
    STD_ZEROMEM(sithThingTest_aDebrisTemplates, sizeof(sithThingTest_aDebrisTemplates));
    STD_ZEROMEM(&sithThingTest_model, sizeof(sithThingTest_model));
    STD_ZEROMEM(&sithThingTest_sameModel, sizeof(sithThingTest_sameModel));
    STD_ZEROMEM(&sithThingTest_sprite, sizeof(sithThingTest_sprite));
    STD_ZEROMEM(&sithThingTest_particle, sizeof(sithThingTest_particle));
    STD_ZEROMEM(&sithThingTest_material, sizeof(sithThingTest_material));
    STD_ZEROMEM(&sithThingTest_rdPuppet, sizeof(sithThingTest_rdPuppet));
    STD_ZEROMEM(&sithThingTest_puppetClass, sizeof(sithThingTest_puppetClass));
    STD_ZEROMEM(&sithThingTest_soundClass, sizeof(sithThingTest_soundClass));
    STD_ZEROMEM(&sithThingTest_cogScript, sizeof(sithThingTest_cogScript));
    STD_ZEROMEM(&sithThingTest_cog, sizeof(sithThingTest_cog));
    STD_ZEROMEM(&sithThingTest_sameCogScript, sizeof(sithThingTest_sameCogScript));
    STD_ZEROMEM(&sithThingTest_sameCog, sizeof(sithThingTest_sameCog));
    STD_ZEROMEM(&sithThingTest_aiClass, sizeof(sithThingTest_aiClass));

    STD_STRCPY(sithThingTest_baseTemplate.aName, "base_template");
    STD_STRCPY(sithThingTest_sameResourceTemplate.aName, "same_resource_template");
    STD_STRCPY(sithThingTest_weaponTemplate.aName, "actor_weapon");
    STD_STRCPY(sithThingTest_explodeTemplate.aName, "actor_explosion");
    STD_STRCPY(sithThingTest_explosionTemplate.aName, "weapon_explosion");
    STD_STRCPY(sithThingTest_model.aName, "different_model.3do");
    STD_STRCPY(sithThingTest_sameModel.aName, "inherited_model.3do");
    STD_STRCPY(sithThingTest_sprite.aName, "test_sprite.spr");
    STD_STRCPY(sithThingTest_particle.aName, "test_particle.par");
    STD_STRCPY(sithThingTest_material.aName, "test_material.mat");
    STD_STRCPY(sithThingTest_puppetClass.aName, "test_puppet.pup");
    STD_STRCPY(sithThingTest_soundClass.aName, "test_sound.snd");
    STD_STRCPY(sithThingTest_cogScript.aName, "test_logic.cog");
    STD_STRCPY(sithThingTest_sameCogScript.aName, "inherited_logic.cog");
    STD_STRCPY(sithThingTest_aiClass.aName, "test_ai.ai");

    // The original reader applies the base template before entering the saved sector.
    sithThingTest_baseTemplate.type = SITH_THING_CAMERA;
    sithThingTest_cog.pScript     = &sithThingTest_cogScript;
    sithThingTest_sameCog.pScript = &sithThingTest_sameCogScript;
    sithThingTest_sameResourceTemplate.renderData.type         = RD_THING_MODEL3;
    sithThingTest_sameResourceTemplate.renderData.data.pModel3 = &sithThingTest_sameModel;
    sithThingTest_sameResourceTemplate.pPuppetClass            = &sithThingTest_puppetClass;
    sithThingTest_sameResourceTemplate.pCog                    = &sithThingTest_sameCog;
    sithThingTest_sameResourceTemplate.flags                  |= SITH_TF_COGLINKED;

    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aDebrisTemplates); ++i )
    {
        STD_FORMAT(sithThingTest_aDebrisTemplates[i].aName, "debris_%02u", (unsigned int)i);
    }
}

static void sithThingTest_InitThings(int bUseResources, int bUseSector)
{
    STD_ZEROMEM(&sithThingTest_world, sizeof(sithThingTest_world));
    STD_ZEROMEM(sithThingTest_aSectors, sizeof(sithThingTest_aSectors));
    STD_ZEROMEM(sithThingTest_aThings, sizeof(sithThingTest_aThings));
    STD_ZEROMEM(sithThingTest_aPathFrames, sizeof(sithThingTest_aPathFrames));
    STD_ZEROMEM(sithThingTest_aAIPathFrames, sizeof(sithThingTest_aAIPathFrames));
    STD_ZEROMEM(sithThingTest_aAIControls, sizeof(sithThingTest_aAIControls));
    sithThingTest_InitResources();

    sithThingTest_world.aSectors   = sithThingTest_aSectors;
    sithThingTest_world.numSectors = STD_ARRAYLEN(sithThingTest_aSectors);

    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aThings); ++i )
    {
        SithThing* pThing = &sithThingTest_aThings[i];
        float baseValue   = (float)(i * 10u);

        pThing->type         = (SithThingType)i;
        pThing->flags        = (SithThingFlag)(SITH_TF_SEEN | SITH_TF_EMITLIGHT | (i & 1u ? SITH_TF_SHADOW : 0u));
        pThing->moveType     = SITH_MT_NONE;
        pThing->controlType  = SITH_CT_PLOT;
        pThing->msecLifeLeft = 1000u + (uint32_t)i;
        pThing->perfLevel    = 20 + (int)i;
        STD_FORMAT(pThing->aName, "thing_%02u", (unsigned int)i);

        sithThingTest_SetIdentity(&pThing->orient);
        sithThingTest_SetVector3(&pThing->pos, baseValue + 1.0f, baseValue + 2.0f, baseValue + 3.0f);
        sithThingTest_SetVector4(&pThing->light.color, baseValue + 4.0f, baseValue + 5.0f, baseValue + 6.0f, baseValue + 7.0f);
        sithThingTest_SetVector4(&pThing->light.emitColor, baseValue + 8.0f, baseValue + 9.0f, baseValue + 10.0f, baseValue + 11.0f);
        pThing->light.minRadius   = baseValue + 12.0f;
        pThing->light.maxRadius   = baseValue + 13.0f;
        pThing->collide.type      = (SithCollideType)(i % SITH_COLLIDE_NUMTYPES);
        pThing->collide.movesize  = baseValue + 14.0f;
        pThing->collide.size      = baseValue + 15.0f;
        pThing->collide.width     = baseValue + 16.0f;
        pThing->collide.height    = baseValue + 17.0f;
        pThing->collide.unkWidth  = baseValue + 18.0f;
        pThing->collide.unkHeight = baseValue + 19.0f;
        pThing->renderData.type   = RD_THING_NONE;

        if ( bUseSector && i == SITH_THING_CAMERA )
        {
            pThing->pInSector = &sithThingTest_aSectors[1];
        }
    }

    rdVector3 cameraPyr;
    sithThingTest_SetVector3(&cameraPyr, 15.0f, -30.0f, 45.0f);
    rdMatrix_BuildRotate34(&sithThingTest_aThings[SITH_THING_CAMERA].orient, &cameraPyr);

    SithPhysicsInfo* pActorPhysics = &sithThingTest_aThings[SITH_THING_ACTOR].moveInfo.physics;
    sithThingTest_aThings[SITH_THING_ACTOR].moveType = SITH_MT_PHYSICS;
    pActorPhysics->flags               = (SithPhysicsFlags)0x12345678u;
    pActorPhysics->mass                = 11.0f;
    pActorPhysics->height              = 12.0f;
    pActorPhysics->airDrag             = 13.0f;
    pActorPhysics->surfDrag            = 14.0f;
    pActorPhysics->staticDrag          = 15.0f;
    pActorPhysics->maxRotationVelocity = 16.0f;
    pActorPhysics->maxVelocity         = 17.0f;
    pActorPhysics->orientSpeed         = 18.0f;
    pActorPhysics->buoyancy            = 19.0f;
    sithThingTest_SetVector3(&pActorPhysics->angularVelocity, 20.0f, 21.0f, 22.0f);
    sithThingTest_SetVector3(&pActorPhysics->velocity, 23.0f, 24.0f, 25.0f);

    SithPhysicsInfo* pPlayerPhysics = &sithThingTest_aThings[SITH_THING_PLAYER].moveInfo.physics;
    sithThingTest_aThings[SITH_THING_PLAYER].moveType = SITH_MT_PHYSICS;
    pPlayerPhysics->flags               = (SithPhysicsFlags)0x87654321u;
    pPlayerPhysics->mass                = 31.0f;
    pPlayerPhysics->height              = 32.0f;
    pPlayerPhysics->airDrag             = 33.0f;
    pPlayerPhysics->surfDrag            = 34.0f;
    pPlayerPhysics->staticDrag          = 35.0f;
    pPlayerPhysics->maxRotationVelocity = 36.0f;
    pPlayerPhysics->maxVelocity         = 37.0f;
    pPlayerPhysics->orientSpeed         = 38.0f;
    pPlayerPhysics->buoyancy            = 39.0f;
    sithThingTest_SetVector3(&pPlayerPhysics->angularVelocity, 40.0f, 41.0f, 42.0f);
    sithThingTest_SetVector3(&pPlayerPhysics->velocity, 43.0f, 44.0f, 45.0f);

    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aPathFrames); ++i )
    {
        float baseValue = 100.0f + (float)i * 10.0f;
        sithThingTest_SetVector3(&sithThingTest_aPathFrames[i].pos, baseValue + 1.0f, baseValue + 2.0f, baseValue + 3.0f);
        sithThingTest_SetVector3(&sithThingTest_aPathFrames[i].pyr, baseValue + 4.0f, baseValue + 5.0f, baseValue + 6.0f);
    }

    sithThingTest_aThings[SITH_THING_WEAPON].moveType = SITH_MT_PATH;
    sithThingTest_aThings[SITH_THING_WEAPON].moveInfo.pathMovement.numFrames = 2u;
    sithThingTest_aThings[SITH_THING_WEAPON].moveInfo.pathMovement.aFrames   = &sithThingTest_aPathFrames[0];
    sithThingTest_aThings[SITH_THING_ITEM].moveType = SITH_MT_PATH;
    sithThingTest_aThings[SITH_THING_ITEM].moveInfo.pathMovement.numFrames = 1u;
    sithThingTest_aThings[SITH_THING_ITEM].moveInfo.pathMovement.aFrames   = &sithThingTest_aPathFrames[2];
    sithThingTest_aThings[SITH_THING_CAMERA].moveType = SITH_MT_PATH;

    sithThingTest_SetActorInfo(&sithThingTest_aThings[SITH_THING_ACTOR].thingInfo.actorInfo, 200.0f);
    sithThingTest_SetActorInfo(&sithThingTest_aThings[SITH_THING_PLAYER].thingInfo.actorInfo, 300.0f);

    SithWeaponInfo* pWeapon = &sithThingTest_aThings[SITH_THING_WEAPON].thingInfo.weaponInfo;
    pWeapon->flags      = (SithWeaponFlag)0x2468ACE0u;
    pWeapon->damage     = 401.0f;
    pWeapon->minDamage  = 402.0f;
    pWeapon->rate       = 403.0f;
    pWeapon->damageType = (SithDamageType)0x1357u;
    pWeapon->range      = 404.0f;
    pWeapon->force      = 405.0f;

    SithItemInfo* pItem = &sithThingTest_aThings[SITH_THING_ITEM].thingInfo.itemInfo;
    pItem->flags              = (SithItemFlag)0x55AAu;
    pItem->secRespawnInterval = 501.0f;

    SithExplosionInfo* pExplosion = &sithThingTest_aThings[SITH_THING_EXPLOSION].thingInfo.explosionInfo;
    pExplosion->flags          = (SithExplosionFlag)0x10203040u;
    pExplosion->damage         = 601.0f;
    pExplosion->damageType     = (SithDamageType)0x2468u;
    pExplosion->range          = 602.0f;
    pExplosion->force          = 603.0f;
    pExplosion->msecBlastTime  = 604u;
    pExplosion->msecBabyTime   = 605u;
    pExplosion->msecExpandTime = 606u;
    pExplosion->msecFadeTime   = 607u;
    pExplosion->maxLight       = 608.0f;
    sithThingTest_SetVector3(&pExplosion->spriteStart, 609.0f, 610.0f, 611.0f);
    sithThingTest_SetVector3(&pExplosion->spriteEnd, 612.0f, 613.0f, 614.0f);
    SithParticleInfo* pParticle = &sithThingTest_aThings[SITH_THING_PARTICLE].thingInfo.particleInfo;
    pParticle->flags        = (SithParticleFlag)0x1234u;
    pParticle->growthSpeed  = 701.0f;
    pParticle->minRadius    = 702.0f;
    pParticle->maxRadius    = 703.0f;
    pParticle->size         = 704.0f;
    pParticle->timeoutRate  = 705.0f;
    pParticle->numParticles = 706u;
    pParticle->pitchRange   = 707.0f;
    pParticle->yawRange     = 708.0f;
    sithThingTest_aThings[SITH_THING_HINT].userval = 801.0f;

    sithThingTest_SetVector3(&sithThingTest_aAIPathFrames[0], 901.0f, 902.0f, 903.0f);
    sithThingTest_SetVector3(&sithThingTest_aAIPathFrames[1], 904.0f, 905.0f, 906.0f);
    sithThingTest_aAIControls[0].aFrames   = sithThingTest_aAIPathFrames;
    sithThingTest_aAIControls[0].numFrames = STD_ARRAYLEN(sithThingTest_aAIPathFrames);
    sithThingTest_aAIControls[0].sizeFrames = STD_ARRAYLEN(sithThingTest_aAIPathFrames);
    sithThingTest_aThings[SITH_THING_ACTOR].controlType = SITH_CT_AI;
    sithThingTest_aThings[SITH_THING_ACTOR].controlInfo.aiControl.pLocal = &sithThingTest_aAIControls[0];
    sithThingTest_aThings[SITH_THING_GHOST].controlType = SITH_CT_AI;
    sithThingTest_aThings[SITH_THING_GHOST].controlInfo.aiControl.pLocal = NULL;
    sithThingTest_aThings[SITH_THING_CORPSE].controlType = SITH_CT_AI;
    sithThingTest_aThings[SITH_THING_CORPSE].controlInfo.aiControl.pLocal = &sithThingTest_aAIControls[2];

    if ( !bUseResources )
    {
        return;
    }

    sithThingTest_aThings[SITH_THING_CAMERA].pTemplate = &sithThingTest_baseTemplate;
    sithThingTest_aThings[SITH_THING_CAMERA].renderData.type         = RD_THING_MODEL3;
    sithThingTest_aThings[SITH_THING_CAMERA].renderData.data.pModel3 = &sithThingTest_model;
    sithThingTest_aThings[SITH_THING_COG].renderData.type         = RD_THING_MODEL3;
    sithThingTest_aThings[SITH_THING_COG].renderData.data.pModel3 = &sithThingTest_model;
    sithThingTest_aThings[SITH_THING_DEBRIS].pTemplate = &sithThingTest_sameResourceTemplate;
    sithThingTest_aThings[SITH_THING_DEBRIS].renderData.type         = RD_THING_MODEL3;
    sithThingTest_aThings[SITH_THING_DEBRIS].renderData.data.pModel3 = &sithThingTest_sameModel;
    sithThingTest_aThings[SITH_THING_SPRITE].renderData.type          = RD_THING_SPRITE3;
    sithThingTest_aThings[SITH_THING_SPRITE].renderData.data.pSprite3 = &sithThingTest_sprite;
    sithThingTest_aThings[SITH_THING_PARTICLE].renderData.type           = RD_THING_PARTICLE;
    sithThingTest_aThings[SITH_THING_PARTICLE].renderData.data.pParticle = &sithThingTest_particle;

    SithThing* pActorThing = &sithThingTest_aThings[SITH_THING_ACTOR];
    pActorThing->pTemplate            = &sithThingTest_baseTemplate;
    pActorThing->pPuppetClass         = &sithThingTest_puppetClass;
    pActorThing->pSoundClass          = &sithThingTest_soundClass;
    pActorThing->pCreateThingTemplate = &sithThingTest_baseTemplate;
    pActorThing->pCog                 = &sithThingTest_cog;
    pActorThing->flags               |= SITH_TF_COGLINKED;
    pActorThing->thingInfo.actorInfo.pWeaponTemplate  = &sithThingTest_weaponTemplate;
    pActorThing->thingInfo.actorInfo.pExplodeTemplate = &sithThingTest_explodeTemplate;
    pActorThing->controlInfo.aiControl.pClass = &sithThingTest_aiClass;
    sithThingTest_aThings[SITH_THING_GHOST].controlInfo.aiControl.pClass = &sithThingTest_aiClass;

    SithThing* pPlayerThing = &sithThingTest_aThings[SITH_THING_PLAYER];
    pPlayerThing->pTemplate    = &sithThingTest_sameResourceTemplate;
    pPlayerThing->pPuppetClass = &sithThingTest_puppetClass;
    pPlayerThing->pSoundClass  = &sithThingTest_soundClass;
    pPlayerThing->pCog         = &sithThingTest_sameCog;
    pPlayerThing->flags       |= SITH_TF_COGLINKED;

    pWeapon->pExplosionTemplate = &sithThingTest_explosionTemplate;
    pParticle->pMaterial        = &sithThingTest_material;
    STD_STRCPY(pExplosion->aSpriteTemplateName, "sprite_template");
    for ( size_t i = 0; i < STD_ARRAYLEN(pExplosion->apDebries); ++i )
    {
        pExplosion->apDebries[i] = &sithThingTest_aDebrisTemplates[i];
    }
}

static void J3DAPI sithThingTest_InitCallback(SithThing* pThing)
{
    TEST_ASSERT_NOT_NULL(pThing);
    ++sithThingTest_numInitCallbacks;
}

static void sithThingTest_InitReadThings(void)
{
    STD_ZEROMEM(sithThingTest_aReadThings, sizeof(sithThingTest_aReadThings));
    STD_ZEROMEM(sithThingTest_aReadAIControls, sizeof(sithThingTest_aReadAIControls));
    sithThingTest_numInitCallbacks = 0;

    size_t aiControlNum = 0;
    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aReadThings); ++i )
    {
        // The original reader enters sectors before replacing the existing slot type.
        sithThingTest_aReadThings[i].type      = sithThingTest_aThings[i].type;
        sithThingTest_aReadThings[i].idx       = (int)i;
        sithThingTest_aReadThings[i].signature = 100u + (uint32_t)i;
        if ( sithThingTest_aThings[i].controlType == SITH_CT_AI )
        {
            if ( sithThingTest_aThings[i].controlInfo.aiControl.pLocal )
            {
                sithThingTest_aReadThings[i].controlInfo.aiControl.pLocal = &sithThingTest_aReadAIControls[aiControlNum];
            }
            ++aiControlNum;
        }
    }
}

static void sithThingTest_FreeReadThingData(void)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aReadThings); ++i )
    {
        SithThing* pThing = &sithThingTest_aReadThings[i];
        if ( pThing->moveType == SITH_MT_PATH && pThing->moveInfo.pathMovement.aFrames )
        {
            sithThingTest_Free(pThing->moveInfo.pathMovement.aFrames);
            pThing->moveInfo.pathMovement.aFrames = NULL;
        }

        if ( pThing->controlType == SITH_CT_AI
            && pThing->controlInfo.aiControl.pLocal
            && pThing->controlInfo.aiControl.pLocal->aFrames )
        {
            sithThingTest_Free(pThing->controlInfo.aiControl.pLocal->aFrames);
            pThing->controlInfo.aiControl.pLocal->aFrames = NULL;
        }
    }
}

static int sithThingTest_ParseBinaryView(size_t numThings, SithThingTestBinaryView* pView)
{
    const uint8_t* pCur = sithThingTest_file.aData;
    const uint8_t* pEnd = pCur + sithThingTest_file.size;

#define SITHTHING_TEST_TAKE(type, count, member) \
    do { \
        size_t dataSize = sizeof(type) * (size_t)(count); \
        if ( dataSize > (size_t)(pEnd - pCur) ) \
        { \
            return 0; \
        } \
        pView->member = (const type*)pCur; \
        pCur += dataSize; \
    } while ( 0 )

    SITHTHING_TEST_TAKE(CndThingInfo, numThings, aThingInfos);
    SITHTHING_TEST_TAKE(int32_t, SITHTHING_TEST_NUMCOUNTS, aCounts);
    SITHTHING_TEST_TAKE(CndPhysicsInfo, pView->aCounts[SITHTHING_TEST_NUMPHYSICSINFOS], aPhysicsInfos);
    SITHTHING_TEST_TAKE(int32_t, pView->aCounts[SITHTHING_TEST_NUMPATHMOVES], aNumPathFrames);
    SITHTHING_TEST_TAKE(SithPathFrame, pView->aCounts[SITHTHING_TEST_NUMPATHFRAMES], aPathFrames);
    SITHTHING_TEST_TAKE(CndActorInfo, pView->aCounts[SITHTHING_TEST_NUMACTORINFOS], aActorInfos);
    SITHTHING_TEST_TAKE(CndWeaponInfo, pView->aCounts[SITHTHING_TEST_NUMWEAPONINFOS], aWeaponInfos);
    SITHTHING_TEST_TAKE(CndExplosionInfo, pView->aCounts[SITHTHING_TEST_NUMEXPLOSIONINFOS], aExplosionInfos);
    SITHTHING_TEST_TAKE(CndItemInfo, pView->aCounts[SITHTHING_TEST_NUMITEMINFOS], aItemInfos);
    SITHTHING_TEST_TAKE(float, pView->aCounts[SITHTHING_TEST_NUMUSERVALUES], aUserValues);
    SITHTHING_TEST_TAKE(CndParticleInfo, pView->aCounts[SITHTHING_TEST_NUMPARTICLEINFOS], aParticleInfos);
    SITHTHING_TEST_TAKE(CndAIControlInfo, pView->aCounts[SITHTHING_TEST_NUMAICONTROLINFOS], aAIControlInfos);
    SITHTHING_TEST_TAKE(rdVector3, pView->aCounts[SITHTHING_TEST_NUMAIPATHFRAMES], aAIPathFrames);

#undef SITHTHING_TEST_TAKE

    return pCur == pEnd;
}

static void sithThingTest_AssertVector3(const rdVector3* pExpected, const rdVector3* pActual)
{
    TEST_ASSERT_EQUAL_FLOAT(pExpected->x, pActual->x);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->y, pActual->y);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->z, pActual->z);
}

static void sithThingTest_AssertVector4(const rdVector4* pExpected, const rdVector4* pActual)
{
    TEST_ASSERT_EQUAL_FLOAT(pExpected->x, pActual->x);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->y, pActual->y);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->z, pActual->z);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->w, pActual->w);
}

static void sithThingTest_AssertOrientationVector(const rdVector3* pExpected, const rdVector3* pActual)
{
    // Angle extraction and reconstruction pass through the engine's approximate trig tables.
    const float tolerance = 0.002f;
    TEST_ASSERT_FLOAT_WITHIN(tolerance, pExpected->x, pActual->x);
    TEST_ASSERT_FLOAT_WITHIN(tolerance, pExpected->y, pActual->y);
    TEST_ASSERT_FLOAT_WITHIN(tolerance, pExpected->z, pActual->z);
}

static void sithThingTest_AssertPhysicsInfo(const SithPhysicsInfo* pExpected, const CndPhysicsInfo* pActual)
{
    TEST_ASSERT_EQUAL_INT(pExpected->flags, pActual->flags);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->mass, pActual->mass);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->height, pActual->height);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->airDrag, pActual->airDrag);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->surfDrag, pActual->surfaceDrag);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->staticDrag, pActual->staticDrag);
    sithThingTest_AssertVector3(&pExpected->angularVelocity, &pActual->angularVelocity);
    sithThingTest_AssertVector3(&pExpected->velocity, &pActual->velocity);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->maxRotationVelocity, pActual->maxRotationVelocity);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->maxVelocity, pActual->maxVelocity);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->orientSpeed, pActual->orientSpeed);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->buoyancy, pActual->buoyancy);
}

static void sithThingTest_AssertActorInfo(const SithActorInfo* pExpected, const CndActorInfo* pActual)
{
    TEST_ASSERT_EQUAL_INT(pExpected->flags, pActual->flags);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->health, pActual->health);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->maxHealth, pActual->maxHealth);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->maxThrust, pActual->maxThrust);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->maxRotVelocity, pActual->maxRotVelocity);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->maxHeadVelocity, pActual->maxHeadVelocity);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->maxHeadYaw, pActual->maxHeadYaw);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->jumpSpeed, pActual->jumpSpeed);
    sithThingTest_AssertVector3(&pExpected->eyeOffset, &pActual->eyeOffset);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->minHeadPitch, pActual->minHeadPitch);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->maxHeadPitch, pActual->maxHeadPitch);
    sithThingTest_AssertVector3(&pExpected->fireOffset, &pActual->fireOffset);
    sithThingTest_AssertVector3(&pExpected->lightOffset, &pActual->lightOffset);
    sithThingTest_AssertVector4(&pExpected->headLightIntensity, &pActual->headLightColor);
    TEST_ASSERT_EQUAL_MEMORY(&pExpected->voiceInfo.voiceColor, &pActual->voiceColor, sizeof(pActual->voiceColor));

    TEST_ASSERT_EQUAL_STRING(
        pExpected->pWeaponTemplate ? pExpected->pWeaponTemplate->aName : "",
        pActual->aWeaponTemplateName
    );
    TEST_ASSERT_EQUAL_STRING(
        pExpected->pExplodeTemplate ? pExpected->pExplodeTemplate->aName : "",
        pActual->aExplodeTemplateName
    );
}

static void sithThingTest_AssertGenericThingInfo(const SithThing* pExpected, const CndThingInfo* pActual, int expectedSectorNum)
{
    rdVector3 expectedPyr;
    rdMatrix_ExtractAngles34(&pExpected->orient, &expectedPyr);

    TEST_ASSERT_EQUAL_STRING(pExpected->pTemplate ? pExpected->pTemplate->aName : "", pActual->aBaseTemplateName);
    TEST_ASSERT_EQUAL_STRING(pExpected->aName, pActual->aName);
    sithThingTest_AssertVector3(&pExpected->pos, &pActual->pos);
    sithThingTest_AssertVector3(&expectedPyr, &pActual->pyr);
    TEST_ASSERT_EQUAL_INT(0, pActual->unknown6);
    TEST_ASSERT_EQUAL_INT(expectedSectorNum, pActual->sectorNum);
    TEST_ASSERT_EQUAL_INT(pExpected->type, pActual->type);
    TEST_ASSERT_EQUAL_HEX32((uint32_t)pExpected->flags & ~(uint32_t)SITH_TF_SEEN, (uint32_t)pActual->flags);
    TEST_ASSERT_EQUAL_INT(pExpected->moveType, pActual->moveType);
    TEST_ASSERT_EQUAL_INT(pExpected->controlType, pActual->controlType);
    sithThingTest_AssertVector4(&pExpected->light.color, &pActual->light.color);
    sithThingTest_AssertVector4(&pExpected->light.emitColor, &pActual->light.emitColor);
    TEST_ASSERT_EQUAL_UINT32(pExpected->msecLifeLeft, pActual->msecLifeLeft);
    TEST_ASSERT_EQUAL_INT(pExpected->renderData.type, pActual->rdThingType);
    TEST_ASSERT_EQUAL_MEMORY(&pExpected->collide, &pActual->collide, sizeof(pActual->collide));
    TEST_ASSERT_EQUAL_INT(pExpected->perfLevel, pActual->perfLevel);
}

static void sithThingTest_AssertRoundTripThing(const SithThing* pExpected, const SithThing* pActual)
{
    TEST_ASSERT_EQUAL_STRING(pExpected->aName, pActual->aName);
    TEST_ASSERT_EQUAL_INT(pExpected->type, pActual->type);
    TEST_ASSERT_EQUAL_HEX32((uint32_t)pExpected->flags & ~(uint32_t)SITH_TF_SEEN, (uint32_t)pActual->flags);
    TEST_ASSERT_EQUAL_INT(pExpected->moveType, pActual->moveType);
    TEST_ASSERT_EQUAL_INT(pExpected->controlType, pActual->controlType);
    sithThingTest_AssertVector3(&pExpected->pos, &pActual->pos);
    sithThingTest_AssertOrientationVector(&pExpected->orient.rvec, &pActual->orient.rvec);
    sithThingTest_AssertOrientationVector(&pExpected->orient.lvec, &pActual->orient.lvec);
    sithThingTest_AssertOrientationVector(&pExpected->orient.uvec, &pActual->orient.uvec);
    sithThingTest_AssertVector4(&pExpected->light.color, &pActual->light.color);
    sithThingTest_AssertVector4(&pExpected->light.emitColor, &pActual->light.emitColor);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->light.color.w, pActual->light.minRadius);
    TEST_ASSERT_EQUAL_FLOAT(pExpected->light.color.w, pActual->light.maxRadius);
    TEST_ASSERT_EQUAL_UINT32(pExpected->msecLifeLeft, pActual->msecLifeLeft);
    TEST_ASSERT_EQUAL_MEMORY(&pExpected->collide, &pActual->collide, sizeof(pActual->collide));
    TEST_ASSERT_EQUAL_INT(pExpected->perfLevel, pActual->perfLevel);

    if ( pExpected->moveType == SITH_MT_PHYSICS )
    {
        const SithPhysicsInfo* pExpectedPhysics = &pExpected->moveInfo.physics;
        const SithPhysicsInfo* pActualPhysics   = &pActual->moveInfo.physics;
        TEST_ASSERT_EQUAL_INT(pExpectedPhysics->flags, pActualPhysics->flags);
        TEST_ASSERT_EQUAL_FLOAT(pExpectedPhysics->mass, pActualPhysics->mass);
        TEST_ASSERT_EQUAL_FLOAT(pExpectedPhysics->height, pActualPhysics->height);
        TEST_ASSERT_EQUAL_FLOAT(pExpectedPhysics->airDrag, pActualPhysics->airDrag);
        TEST_ASSERT_EQUAL_FLOAT(pExpectedPhysics->surfDrag, pActualPhysics->surfDrag);
        TEST_ASSERT_EQUAL_FLOAT(pExpectedPhysics->staticDrag, pActualPhysics->staticDrag);
        sithThingTest_AssertVector3(&pExpectedPhysics->angularVelocity, &pActualPhysics->angularVelocity);
        sithThingTest_AssertVector3(&pExpectedPhysics->velocity, &pActualPhysics->velocity);
        TEST_ASSERT_EQUAL_FLOAT(pExpectedPhysics->maxRotationVelocity, pActualPhysics->maxRotationVelocity);
        TEST_ASSERT_EQUAL_FLOAT(pExpectedPhysics->maxVelocity, pActualPhysics->maxVelocity);
        TEST_ASSERT_EQUAL_FLOAT(pExpectedPhysics->orientSpeed, pActualPhysics->orientSpeed);
        TEST_ASSERT_EQUAL_FLOAT(pExpectedPhysics->buoyancy, pActualPhysics->buoyancy);
    }
    else if ( pExpected->moveType == SITH_MT_PATH )
    {
        const SithPathMoveInfo* pExpectedPath = &pExpected->moveInfo.pathMovement;
        const SithPathMoveInfo* pActualPath   = &pActual->moveInfo.pathMovement;
        TEST_ASSERT_EQUAL_size_t(pExpectedPath->numFrames, pActualPath->numFrames);
        TEST_ASSERT_EQUAL_size_t(pExpectedPath->numFrames, pActualPath->sizeFrames);
        if ( pExpectedPath->numFrames )
        {
            TEST_ASSERT_EQUAL_MEMORY(pExpectedPath->aFrames, pActualPath->aFrames, sizeof(SithPathFrame) * pExpectedPath->numFrames);
        }
        else
        {
            TEST_ASSERT_NULL(pActualPath->aFrames);
        }
    }

    switch ( pExpected->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_PLAYER:
        {
            const SithActorInfo* pExpectedActor = &pExpected->thingInfo.actorInfo;
            const SithActorInfo* pActualActor   = &pActual->thingInfo.actorInfo;
            TEST_ASSERT_EQUAL_INT(pExpectedActor->flags, pActualActor->flags);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedActor->health, pActualActor->health);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedActor->maxHealth, pActualActor->maxHealth);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedActor->maxThrust, pActualActor->maxThrust);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedActor->maxRotVelocity, pActualActor->maxRotVelocity);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedActor->maxHeadVelocity, pActualActor->maxHeadVelocity);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedActor->maxHeadYaw, pActualActor->maxHeadYaw);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedActor->jumpSpeed, pActualActor->jumpSpeed);
            sithThingTest_AssertVector3(&pExpectedActor->eyeOffset, &pActualActor->eyeOffset);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedActor->minHeadPitch, pActualActor->minHeadPitch);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedActor->maxHeadPitch, pActualActor->maxHeadPitch);
            sithThingTest_AssertVector3(&pExpectedActor->fireOffset, &pActualActor->fireOffset);
            sithThingTest_AssertVector3(&pExpectedActor->lightOffset, &pActualActor->lightOffset);
            sithThingTest_AssertVector4(&pExpectedActor->headLightIntensity, &pActualActor->headLightIntensity);
            TEST_ASSERT_EQUAL_MEMORY(&pExpectedActor->voiceInfo.voiceColor, &pActualActor->voiceInfo.voiceColor, sizeof(pActualActor->voiceInfo.voiceColor));
            break;
        }

        case SITH_THING_WEAPON:
            TEST_ASSERT_EQUAL_INT(pExpected->thingInfo.weaponInfo.flags, pActual->thingInfo.weaponInfo.flags);
            TEST_ASSERT_EQUAL_FLOAT(pExpected->thingInfo.weaponInfo.damage, pActual->thingInfo.weaponInfo.damage);
            TEST_ASSERT_EQUAL_FLOAT(pExpected->thingInfo.weaponInfo.minDamage, pActual->thingInfo.weaponInfo.minDamage);
            TEST_ASSERT_EQUAL_FLOAT(pExpected->thingInfo.weaponInfo.rate, pActual->thingInfo.weaponInfo.rate);
            TEST_ASSERT_EQUAL_INT(pExpected->thingInfo.weaponInfo.damageType, pActual->thingInfo.weaponInfo.damageType);
            TEST_ASSERT_EQUAL_FLOAT(pExpected->thingInfo.weaponInfo.range, pActual->thingInfo.weaponInfo.range);
            TEST_ASSERT_EQUAL_FLOAT(pExpected->thingInfo.weaponInfo.force, pActual->thingInfo.weaponInfo.force);
            break;

        case SITH_THING_ITEM:
            TEST_ASSERT_EQUAL_INT(pExpected->thingInfo.itemInfo.flags, pActual->thingInfo.itemInfo.flags);
            TEST_ASSERT_EQUAL_FLOAT(pExpected->thingInfo.itemInfo.secRespawnInterval, pActual->thingInfo.itemInfo.secRespawnInterval);
            break;

        case SITH_THING_EXPLOSION:
        {
            const SithExplosionInfo* pExpectedExplosion = &pExpected->thingInfo.explosionInfo;
            const SithExplosionInfo* pActualExplosion   = &pActual->thingInfo.explosionInfo;
            TEST_ASSERT_EQUAL_INT(pExpectedExplosion->flags, pActualExplosion->flags);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedExplosion->damage, pActualExplosion->damage);
            TEST_ASSERT_EQUAL_INT(pExpectedExplosion->damageType, pActualExplosion->damageType);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedExplosion->range, pActualExplosion->range);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedExplosion->force, pActualExplosion->force);
            TEST_ASSERT_EQUAL_UINT32(pExpectedExplosion->msecBlastTime, pActualExplosion->msecBlastTime);
            TEST_ASSERT_EQUAL_UINT32(pExpectedExplosion->msecBabyTime, pActualExplosion->msecBabyTime);
            TEST_ASSERT_EQUAL_UINT32(pExpectedExplosion->msecExpandTime, pActualExplosion->msecExpandTime);
            TEST_ASSERT_EQUAL_UINT32(pExpectedExplosion->msecFadeTime, pActualExplosion->msecFadeTime);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedExplosion->maxLight, pActualExplosion->maxLight);
            sithThingTest_AssertVector3(&pExpectedExplosion->spriteStart, &pActualExplosion->spriteStart);
            sithThingTest_AssertVector3(&pExpectedExplosion->spriteEnd, &pActualExplosion->spriteEnd);
            TEST_ASSERT_EQUAL_STRING(pExpectedExplosion->aSpriteTemplateName, pActualExplosion->aSpriteTemplateName);
            break;
        }

        case SITH_THING_PARTICLE:
        {
            const SithParticleInfo* pExpectedParticle = &pExpected->thingInfo.particleInfo;
            const SithParticleInfo* pActualParticle   = &pActual->thingInfo.particleInfo;
            TEST_ASSERT_EQUAL_INT(pExpectedParticle->flags, pActualParticle->flags);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedParticle->growthSpeed, pActualParticle->growthSpeed);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedParticle->minRadius, pActualParticle->minRadius);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedParticle->maxRadius, pActualParticle->maxRadius);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedParticle->size, pActualParticle->size);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedParticle->timeoutRate, pActualParticle->timeoutRate);
            TEST_ASSERT_EQUAL_size_t(pExpectedParticle->numParticles, pActualParticle->numParticles);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedParticle->pitchRange, pActualParticle->pitchRange);
            TEST_ASSERT_EQUAL_FLOAT(pExpectedParticle->yawRange, pActualParticle->yawRange);
            break;
        }

        case SITH_THING_HINT:
            TEST_ASSERT_EQUAL_FLOAT(pExpected->userval, pActual->userval);
            break;

        default:
            break;
    }

    if ( pExpected->controlType == SITH_CT_AI )
    {
        const SithAIControlBlock* pExpectedAI = pExpected->controlInfo.aiControl.pLocal;
        const SithAIControlBlock* pActualAI   = pActual->controlInfo.aiControl.pLocal;
        if ( pExpectedAI && pExpectedAI->numFrames )
        {
            TEST_ASSERT_NOT_NULL(pActualAI);
            TEST_ASSERT_EQUAL_size_t(pExpectedAI->numFrames, pActualAI->numFrames);
            TEST_ASSERT_EQUAL_size_t(pExpectedAI->numFrames, pActualAI->sizeFrames);
            TEST_ASSERT_EQUAL_MEMORY(pExpectedAI->aFrames, pActualAI->aFrames, sizeof(rdVector3) * pExpectedAI->numFrames);
        }
    }
}

TEST_SETUP(sithThingBinary)
{
    // Reset the in-memory file, counted allocator, resource loaders, and local RTI state for each case.
#if defined(_DEBUG)
    // Retail vectors retain zero-filled string tails; suppress the Debug CRT's 0xFE tail fill.
    sithThingTest_previousDebugFillThreshold = _CrtSetDebugFillThreshold(0u);
#endif

    sithThingTest_numLiveAllocs = 0;
    sithThingTest_ResetHashtables();
    sithThingTest_ResetFile();
    sithThingTest_ResetAllocationFailure();
    STD_ZEROMEM(sithThingTest_aReadThings, sizeof(sithThingTest_aReadThings));
    STD_ZEROMEM(sithThingTest_aReadAIControls, sizeof(sithThingTest_aReadAIControls));
    sithThingTest_ResetLoadState();
    STD_ZEROMEM(&sithThingTest_hostServices, sizeof(sithThingTest_hostServices));
    STD_ZEROMEM(&sithThingTest_staticWorld, sizeof(sithThingTest_staticWorld));
    STD_ZEROMEM(sithThingTest_aStaticSectors, sizeof(sithThingTest_aStaticSectors));
    sithWorld_g_pCurrentWorld      = NULL;
    sithWorld_g_pStaticWorld       = NULL;
    sithWorld_g_pLastLoadedWorld   = NULL;
    sithWorld_g_bLoading           = 0;
    sithThingTest_performanceLevel = 3;
    sithThingTest_bConffileOpen    = 0;

    sithThingTest_hostServices.pAssert       = sithThingTest_Assert;
    sithThingTest_hostServices.pStatusPrint  = sithThingTest_Print;
    sithThingTest_hostServices.pMessagePrint = sithThingTest_Print;
    sithThingTest_hostServices.pWarningPrint = sithThingTest_Print;
    sithThingTest_hostServices.pErrorPrint   = sithThingTest_Print;
    sithThingTest_hostServices.pDebugPrint   = sithThingTest_Print;
    sithThingTest_hostServices.pMalloc       = sithThingTest_Malloc;
    sithThingTest_hostServices.pFree         = sithThingTest_Free;
    sithThingTest_hostServices.pFileOpen     = sithThingTest_FileOpen;
    sithThingTest_hostServices.pFileClose    = sithThingTest_FileClose;
    sithThingTest_hostServices.pFileRead     = sithThingTest_Read;
    sithThingTest_hostServices.pFileGets     = sithThingTest_FileGets;
    sithThingTest_hostServices.pFileWrite    = sithThingTest_Write;
    sith_g_pHS = &sithThingTest_hostServices;
    std_g_pHS  = &sithThingTest_hostServices;

    TEST_ASSERT_EQUAL_INT(1, sithThing_Startup());
}

TEST_TEAR_DOWN(sithThingBinary)
{
    // Release nested test data, shut down the module, and require a leak-free fixture boundary.
    sithThingTest_failAllocCall     = SITHTHING_TEST_NO_FAILURE;
    sithThingTest_maxAllocationSize = SIZE_MAX;
    if ( sithThingTest_bConffileOpen )
    {
        sithThingTest_CloseConffile();
    }
    sithThingTest_FreeStaticWorld();
    sithThingTest_FreeReadThingData();
    TEST_ASSERT_EQUAL_INT(1, sithThing_Shutdown());
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
#if defined(_DEBUG)
    _CrtSetDebugFillThreshold(sithThingTest_previousDebugFillThreshold);
#endif
}

TEST(sithThingBinary, TestWritesAllThingData)
{
    // Serializes all 15 thing types and validates every emitted binary block.
    static const int32_t aExpectedCounts[SITHTHING_TEST_NUMCOUNTS] = { 2, 3, 3, 2, 1, 1, 1, 1, 1, 3, 2 };

    // Require byte identity with the full retail golden vector before inspecting its layout.
    sithThingTest_InitThings(/*bUseResources=*/1, /*bUseSector=*/1);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));
    TEST_ASSERT_FALSE(sithThingTest_file.bOverflow);
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
    sithThingTest_AssertMatchesGolden("all_fields.bin");

    // Parse the output independently of the reader to verify block counts and field placement.
    SithThingTestBinaryView view;
    STD_ZEROMEM(&view, sizeof(view));
    TEST_ASSERT_TRUE(sithThingTest_ParseBinaryView(STD_ARRAYLEN(sithThingTest_aThings), &view));
    TEST_ASSERT_EQUAL_INT32_ARRAY(aExpectedCounts, view.aCounts, STD_ARRAYLEN(aExpectedCounts));

    // Check each generic thing record, including sector and resource-name references.
    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aThings); ++i )
    {
        int expectedSectorNum = i == SITH_THING_CAMERA ? 1 : -1;
        sithThingTest_AssertGenericThingInfo(&sithThingTest_aThings[i], &view.aThingInfos[i], expectedSectorNum);
    }

    TEST_ASSERT_EQUAL_STRING(sithThingTest_model.aName, view.aThingInfos[SITH_THING_CAMERA].aRdFilename);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_model.aName, view.aThingInfos[SITH_THING_COG].aRdFilename);
    TEST_ASSERT_EQUAL_STRING("", view.aThingInfos[SITH_THING_DEBRIS].aRdFilename);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_sprite.aName, view.aThingInfos[SITH_THING_SPRITE].aRdFilename);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_particle.aName, view.aThingInfos[SITH_THING_PARTICLE].aRdFilename);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_puppetClass.aName, view.aThingInfos[SITH_THING_ACTOR].aPupFilename);
    TEST_ASSERT_EQUAL_STRING("", view.aThingInfos[SITH_THING_PLAYER].aPupFilename);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_soundClass.aName, view.aThingInfos[SITH_THING_ACTOR].aSndFilename);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_soundClass.aName, view.aThingInfos[SITH_THING_PLAYER].aSndFilename);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_baseTemplate.aName, view.aThingInfos[SITH_THING_ACTOR].aCreateThingTemplateName);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_cogScript.aName, view.aThingInfos[SITH_THING_ACTOR].aCogFilename);
    TEST_ASSERT_EQUAL_STRING("", view.aThingInfos[SITH_THING_PLAYER].aCogFilename);

    // Check movement blocks and their flattened path-frame data.
    sithThingTest_AssertPhysicsInfo(&sithThingTest_aThings[SITH_THING_ACTOR].moveInfo.physics, &view.aPhysicsInfos[0]);
    sithThingTest_AssertPhysicsInfo(&sithThingTest_aThings[SITH_THING_PLAYER].moveInfo.physics, &view.aPhysicsInfos[1]);
    TEST_ASSERT_EQUAL_INT32(0, view.aNumPathFrames[0]);
    TEST_ASSERT_EQUAL_INT32(2, view.aNumPathFrames[1]);
    TEST_ASSERT_EQUAL_INT32(1, view.aNumPathFrames[2]);
    TEST_ASSERT_EQUAL_MEMORY(sithThingTest_aPathFrames, view.aPathFrames, sizeof(sithThingTest_aPathFrames));

    // Check type-specific blocks for actors, weapons, explosions, items, hints, and particles.
    sithThingTest_AssertActorInfo(&sithThingTest_aThings[SITH_THING_ACTOR].thingInfo.actorInfo, &view.aActorInfos[0]);
    sithThingTest_AssertActorInfo(&sithThingTest_aThings[SITH_THING_PLAYER].thingInfo.actorInfo, &view.aActorInfos[1]);

    const SithWeaponInfo* pWeapon = &sithThingTest_aThings[SITH_THING_WEAPON].thingInfo.weaponInfo;
    TEST_ASSERT_EQUAL_INT(pWeapon->flags, view.aWeaponInfos[0].flags);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_explosionTemplate.aName, view.aWeaponInfos[0].aExplosionTemplateName);
    TEST_ASSERT_EQUAL_FLOAT(pWeapon->damage, view.aWeaponInfos[0].damage);
    TEST_ASSERT_EQUAL_FLOAT(pWeapon->minDamage, view.aWeaponInfos[0].minDamage);
    TEST_ASSERT_EQUAL_FLOAT(pWeapon->rate, view.aWeaponInfos[0].rate);
    TEST_ASSERT_EQUAL_INT(pWeapon->damageType, view.aWeaponInfos[0].damageType);
    TEST_ASSERT_EQUAL_FLOAT(pWeapon->range, view.aWeaponInfos[0].range);
    TEST_ASSERT_EQUAL_FLOAT(pWeapon->force, view.aWeaponInfos[0].force);

    const SithExplosionInfo* pExplosion = &sithThingTest_aThings[SITH_THING_EXPLOSION].thingInfo.explosionInfo;
    TEST_ASSERT_EQUAL_INT(pExplosion->flags, view.aExplosionInfos[0].flags);
    TEST_ASSERT_EQUAL_FLOAT(pExplosion->damage, view.aExplosionInfos[0].damage);
    TEST_ASSERT_EQUAL_INT(pExplosion->damageType, view.aExplosionInfos[0].damageType);
    TEST_ASSERT_EQUAL_FLOAT(pExplosion->range, view.aExplosionInfos[0].range);
    TEST_ASSERT_EQUAL_FLOAT(pExplosion->force, view.aExplosionInfos[0].force);
    TEST_ASSERT_EQUAL_UINT32(pExplosion->msecBlastTime, view.aExplosionInfos[0].msecBlastTime);
    TEST_ASSERT_EQUAL_UINT32(pExplosion->msecBabyTime, view.aExplosionInfos[0].msecBabyTime);
    TEST_ASSERT_EQUAL_UINT32(pExplosion->msecExpandTime, view.aExplosionInfos[0].msecExpandTime);
    TEST_ASSERT_EQUAL_UINT32(pExplosion->msecFadeTime, view.aExplosionInfos[0].msecFadeTime);
    TEST_ASSERT_EQUAL_FLOAT(pExplosion->maxLight, view.aExplosionInfos[0].maxLight);
    sithThingTest_AssertVector3(&pExplosion->spriteStart, &view.aExplosionInfos[0].spriteStartPos);
    sithThingTest_AssertVector3(&pExplosion->spriteEnd, &view.aExplosionInfos[0].spriteEndPos);
    TEST_ASSERT_EQUAL_STRING(pExplosion->aSpriteTemplateName, view.aExplosionInfos[0].aSpriteTemplateName);
    for ( size_t i = 0; i < STD_ARRAYLEN(pExplosion->apDebries); ++i )
    {
        TEST_ASSERT_EQUAL_STRING(pExplosion->apDebries[i]->aName, view.aExplosionInfos[0].aDebrisTemplateNames[i]);
    }

    const SithItemInfo* pItem = &sithThingTest_aThings[SITH_THING_ITEM].thingInfo.itemInfo;
    TEST_ASSERT_EQUAL_INT(pItem->flags, view.aItemInfos[0].flags);
    TEST_ASSERT_EQUAL_FLOAT(pItem->secRespawnInterval, view.aItemInfos[0].secRespawnInterval);
    TEST_ASSERT_EQUAL_FLOAT(sithThingTest_aThings[SITH_THING_HINT].userval, view.aUserValues[0]);

    const SithParticleInfo* pParticle = &sithThingTest_aThings[SITH_THING_PARTICLE].thingInfo.particleInfo;
    TEST_ASSERT_EQUAL_INT(pParticle->flags, view.aParticleInfos[0].flags);
    TEST_ASSERT_EQUAL_FLOAT(pParticle->growthSpeed, view.aParticleInfos[0].growthSpeed);
    TEST_ASSERT_EQUAL_FLOAT(pParticle->minRadius, view.aParticleInfos[0].minRadius);
    TEST_ASSERT_EQUAL_FLOAT(pParticle->maxRadius, view.aParticleInfos[0].maxRadius);
    TEST_ASSERT_EQUAL_FLOAT(pParticle->size, view.aParticleInfos[0].size);
    TEST_ASSERT_EQUAL_FLOAT(pParticle->timeoutRate, view.aParticleInfos[0].timeoutRate);
    TEST_ASSERT_EQUAL_INT(pParticle->numParticles, view.aParticleInfos[0].numParticles);
    TEST_ASSERT_EQUAL_FLOAT(pParticle->pitchRange, view.aParticleInfos[0].pitchRange);
    TEST_ASSERT_EQUAL_FLOAT(pParticle->yawRange, view.aParticleInfos[0].yawRange);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_material.aName, view.aParticleInfos[0].aMaterialName);

    // Check AI class references and the flattened AI path-frame block.
    TEST_ASSERT_EQUAL_STRING(sithThingTest_aiClass.aName, view.aAIControlInfos[0].aFileName);
    TEST_ASSERT_EQUAL_INT(2, view.aAIControlInfos[0].numFrames);
    TEST_ASSERT_EQUAL_STRING(sithThingTest_aiClass.aName, view.aAIControlInfos[1].aFileName);
    TEST_ASSERT_EQUAL_INT(0, view.aAIControlInfos[1].numFrames);
    TEST_ASSERT_EQUAL_STRING("", view.aAIControlInfos[2].aFileName);
    TEST_ASSERT_EQUAL_INT(0, view.aAIControlInfos[2].numFrames);
    TEST_ASSERT_EQUAL_MEMORY(sithThingTest_aAIPathFrames, view.aAIPathFrames, sizeof(sithThingTest_aAIPathFrames));
}

TEST(sithThingBinary, TestRoundTripsAllThingData)
{
    // Round-trips all thing types through the resource-free retail golden vector.
    // First confirm that the writer still produces the expected source bytes.
    sithThingTest_InitThings(/*bUseResources=*/0, /*bUseSector=*/0);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));
    sithThingTest_AssertMatchesGolden("core.bin");
    sithThingTest_LoadGoldenFile("core.bin");

    // Read known-good bytes into fresh things and run the supplied initializer for every entry.
    sithThingTest_InitReadThings();
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(
        1u,
        &sithThingTest_world,
        STD_ARRAYLEN(sithThingTest_aReadThings),
        sithThingTest_aReadThings,
        sithThingTest_InitCallback
    ));

    // Compare all reconstructed movement, type-specific, and AI state with the source fixture.
    TEST_ASSERT_EQUAL_size_t(STD_ARRAYLEN(sithThingTest_aReadThings), sithThingTest_numInitCallbacks);
    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aThings); ++i )
    {
        sithThingTest_AssertRoundTripThing(&sithThingTest_aThings[i], &sithThingTest_aReadThings[i]);
    }

    sithThingTest_FreeReadThingData();
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
}

TEST(sithThingBinary, TestRestoresResourcesAndSector)
{
    // Verifies that serialized names are resolved back to resources and sector links.
    sithThingTest_InitThings(/*bUseResources=*/1, /*bUseSector=*/1);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));
    sithThingTest_AssertMatchesGolden("all_fields.bin");
    sithThingTest_LoadGoldenFile("all_fields.bin");

    // Provide the runtime objects that the reader must find while restoring the vector.
    sithThingTest_InitReadThings();
    sithThingTest_sameResourceTemplate.pPuppetClass = NULL;
    sithThingTest_baseTemplate.controlInfo.aiControl.pLocal = &sithThingTest_aReadAIControls[0];
    sithThingTest_aiClass.numInstincts = 7u;
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(
        1u,
        &sithThingTest_world,
        STD_ARRAYLEN(sithThingTest_aReadThings),
        sithThingTest_aReadThings,
        sithThingTest_InitCallback
    ));

    // Check generic render, template, sound, COG, and puppet relationships.
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_baseTemplate, sithThingTest_aReadThings[SITH_THING_CAMERA].pTemplate);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_model, sithThingTest_aReadThings[SITH_THING_CAMERA].renderData.data.pModel3);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_model, sithThingTest_aReadThings[SITH_THING_COG].renderData.data.pModel3);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_sameResourceTemplate, sithThingTest_aReadThings[SITH_THING_DEBRIS].pTemplate);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_sameModel, sithThingTest_aReadThings[SITH_THING_DEBRIS].renderData.data.pModel3);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_sprite, sithThingTest_aReadThings[SITH_THING_SPRITE].renderData.data.pSprite3);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_particle, sithThingTest_aReadThings[SITH_THING_PARTICLE].renderData.data.pParticle);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_puppetClass, sithThingTest_aReadThings[SITH_THING_ACTOR].pPuppetClass);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_soundClass, sithThingTest_aReadThings[SITH_THING_ACTOR].pSoundClass);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_baseTemplate, sithThingTest_aReadThings[SITH_THING_ACTOR].pCreateThingTemplate);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_cog, sithThingTest_aReadThings[SITH_THING_ACTOR].pCog);
    TEST_ASSERT_BITS_HIGH(SITH_TF_COGLINKED, sithThingTest_aReadThings[SITH_THING_ACTOR].flags);
    TEST_ASSERT_BITS_HIGH(SITHCOG_LOCAL | SITHCOG_CLASS, sithThingTest_cog.flags);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_sameCog, sithThingTest_aReadThings[SITH_THING_PLAYER].pCog);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_soundClass, sithThingTest_aReadThings[SITH_THING_PLAYER].pSoundClass);

    // Check type-specific templates, particle material, AI state, and sector insertion.
    TEST_ASSERT_EQUAL_PTR(
        &sithThingTest_weaponTemplate,
        sithThingTest_aReadThings[SITH_THING_ACTOR].thingInfo.actorInfo.pWeaponTemplate
    );
    TEST_ASSERT_EQUAL_PTR(
        &sithThingTest_explodeTemplate,
        sithThingTest_aReadThings[SITH_THING_ACTOR].thingInfo.actorInfo.pExplodeTemplate
    );
    TEST_ASSERT_EQUAL_PTR(
        &sithThingTest_explosionTemplate,
        sithThingTest_aReadThings[SITH_THING_WEAPON].thingInfo.weaponInfo.pExplosionTemplate
    );
    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aDebrisTemplates); ++i )
    {
        TEST_ASSERT_EQUAL_PTR(
            &sithThingTest_aDebrisTemplates[i],
            sithThingTest_aReadThings[SITH_THING_EXPLOSION].thingInfo.explosionInfo.apDebries[i]
        );
    }
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_material, sithThingTest_aReadThings[SITH_THING_PARTICLE].thingInfo.particleInfo.pMaterial);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_aiClass, sithThingTest_aReadThings[SITH_THING_ACTOR].controlInfo.aiControl.pClass);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_aiClass, sithThingTest_aReadThings[SITH_THING_GHOST].controlInfo.aiControl.pClass);
    TEST_ASSERT_NULL(sithThingTest_aReadThings[SITH_THING_GHOST].controlInfo.aiControl.pLocal);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_aiClass, sithThingTest_aReadAIControls[0].pClass);
    TEST_ASSERT_EQUAL_size_t(sithThingTest_aiClass.numInstincts, sithThingTest_aReadAIControls[0].numInstincts);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_aSectors[1], sithThingTest_aReadThings[SITH_THING_CAMERA].pInSector);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_aReadThings[SITH_THING_CAMERA], sithThingTest_aSectors[1].pFirstThingInSector);
    TEST_ASSERT_EQUAL_size_t(STD_ARRAYLEN(sithThingTest_aReadThings), sithThingTest_numInitCallbacks);

    // Finally compare every restored value with the source fixture.
    for ( size_t i = 0; i < STD_ARRAYLEN(sithThingTest_aThings); ++i )
    {
        sithThingTest_AssertRoundTripThing(&sithThingTest_aThings[i], &sithThingTest_aReadThings[i]);
    }

    sithThingTest_aReadThings[SITH_THING_CAMERA].pInSector = NULL;
    sithThingTest_aSectors[1].pFirstThingInSector = NULL;
    sithThingTest_FreeReadThingData();
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
}

TEST(sithThingBinary, TestRejectsMissingPuppet)
{
    // A named puppet is required when neither the thing nor its template provides one.
    sithThingTest_InitThings(/*bUseResources=*/1, /*bUseSector=*/1);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));

    sithThingTest_InitReadThings();
    sithThingTest_sameResourceTemplate.pPuppetClass = NULL;
    sithThingTest_baseTemplate.controlInfo.aiControl.pLocal = &sithThingTest_aReadAIControls[0];
    sithThingTest_loadState.bFailPuppets = 1;
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(1, sithThing_ReadThingsListBinary(
        1u,
        &sithThingTest_world,
        STD_ARRAYLEN(sithThingTest_aReadThings),
        sithThingTest_aReadThings,
        sithThingTest_InitCallback
    ));
    sithThingTest_aReadThings[SITH_THING_CAMERA].pInSector = NULL;
    sithThingTest_aSectors[1].pFirstThingInSector = NULL;
    sithThingTest_FreeReadThingData();
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
}

TEST(sithThingBinary, TestAllowsExistingRenderPuppet)
{
    // A failed optional puppet-class load must not discard an existing render puppet.
    sithThingTest_InitThings(/*bUseResources=*/1, /*bUseSector=*/0);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));

    sithThingTest_InitReadThings();
    sithThingTest_baseTemplate.pPuppetClass = &sithThingTest_puppetClass;
    sithThingTest_baseTemplate.controlInfo.aiControl.pLocal = &sithThingTest_aReadAIControls[0];
    sithThingTest_loadState.bFailPuppets = 1;
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(
        1u,
        &sithThingTest_world,
        STD_ARRAYLEN(sithThingTest_aReadThings),
        sithThingTest_aReadThings,
        sithThingTest_InitCallback
    ));
    TEST_ASSERT_NULL(sithThingTest_aReadThings[SITH_THING_ACTOR].pPuppetClass);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_rdPuppet, sithThingTest_aReadThings[SITH_THING_ACTOR].renderData.pPuppet);
    sithThingTest_FreeReadThingData();
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
}

TEST(sithThingBinary, TestHandlesOptionalResourceFailures)
{
    // Optional resource lookup failures leave null references but do not reject the thing list.
    sithThingTest_InitThings(/*bUseResources=*/1, /*bUseSector=*/1);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));

    sithThingTest_InitReadThings();
    sithThingTest_sameResourceTemplate.pPuppetClass = NULL;
    STD_ZEROMEM(&sithThingTest_loadState, sizeof(sithThingTest_loadState));
    sithThingTest_loadState.bFailTemplates    = 1;
    sithThingTest_loadState.bFailModels       = 1;
    sithThingTest_loadState.bFailSprites      = 1;
    sithThingTest_loadState.bFailParticles    = 1;
    sithThingTest_loadState.bFailSoundClasses = 1;
    sithThingTest_loadState.bFailCogs         = 1;
    sithThingTest_loadState.bFailMaterials    = 1;
    sithThingTest_loadState.bFailAIClasses    = 1;
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(
        1u,
        &sithThingTest_world,
        STD_ARRAYLEN(sithThingTest_aReadThings),
        sithThingTest_aReadThings,
        sithThingTest_InitCallback
    ));

    // Confirm every failed optional lookup produced the expected empty runtime state.
    TEST_ASSERT_EQUAL_INT(RD_THING_NONE, sithThingTest_aReadThings[SITH_THING_CAMERA].renderData.type);
    TEST_ASSERT_EQUAL_INT(RD_THING_NONE, sithThingTest_aReadThings[SITH_THING_SPRITE].renderData.type);
    TEST_ASSERT_EQUAL_INT(RD_THING_NONE, sithThingTest_aReadThings[SITH_THING_PARTICLE].renderData.type);
    TEST_ASSERT_NULL(sithThingTest_aReadThings[SITH_THING_ACTOR].pTemplate);
    TEST_ASSERT_NULL(sithThingTest_aReadThings[SITH_THING_ACTOR].pSoundClass);
    TEST_ASSERT_NULL(sithThingTest_aReadThings[SITH_THING_ACTOR].pCreateThingTemplate);
    TEST_ASSERT_NULL(sithThingTest_aReadThings[SITH_THING_ACTOR].pCog);
    TEST_ASSERT_NULL(sithThingTest_aReadThings[SITH_THING_PARTICLE].thingInfo.particleInfo.pMaterial);
    TEST_ASSERT_NULL(sithThingTest_aReadThings[SITH_THING_ACTOR].controlInfo.aiControl.pClass);
    sithThingTest_aReadThings[SITH_THING_CAMERA].pInSector = NULL;
    sithThingTest_aSectors[1].pFirstThingInSector = NULL;
    sithThingTest_FreeReadThingData();
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
}

TEST(sithThingBinary, TestEmptyList)
{
    // An empty list is represented by eleven zero counts and requires no allocations.
    STD_ZEROMEM(&sithThingTest_world, sizeof(sithThingTest_world));
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, 0u, NULL));
    TEST_ASSERT_EQUAL_size_t(sizeof(int32_t) * SITHTHING_TEST_NUMCOUNTS, sithThingTest_file.size);
    sithThingTest_AssertMatchesGolden("empty.bin");

    for ( size_t i = 0; i < sithThingTest_file.size; ++i )
    {
        TEST_ASSERT_EQUAL_UINT8(0u, sithThingTest_file.aData[i]);
    }

    sithThingTest_LoadGoldenFile("empty.bin");
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(1u, &sithThingTest_world, 0u, NULL, NULL));
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
}

TEST(sithThingBinary, TestPreservesBoundedNames)
{
    // A maximum-length terminated name must survive serialization without losing its terminator.
    SithThing thing;
    SithWorld world;
    STD_ZEROMEM(&thing, sizeof(thing));
    STD_ZEROMEM(&world, sizeof(world));
    sithThingTest_SetIdentity(&thing.orient);
    thing.type = SITH_THING_HINT;

    for ( size_t i = 0; i < STD_ARRAYLEN(thing.aName) - 1u; ++i )
    {
        thing.aName[i] = (char)('A' + i % 26u);
    }
    thing.aName[STD_ARRAYLEN(thing.aName) - 1u] = '\0';

    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &world, 1u, &thing));
    sithThingTest_AssertMatchesGolden("max_names.bin");

    SithThingTestBinaryView view;
    STD_ZEROMEM(&view, sizeof(view));
    TEST_ASSERT_TRUE(sithThingTest_ParseBinaryView(1u, &view));
    TEST_ASSERT_EQUAL_STRING(thing.aName, view.aThingInfos[0].aName);
    TEST_ASSERT_EQUAL_UINT8(0u, (uint8_t)view.aThingInfos[0].aName[STD_ARRAYLEN(view.aThingInfos[0].aName) - 1u]);
}

TEST(sithThingBinary, TestPreservesInheritedWeaponExplosion)
{
    // An explosion template inherited unchanged from the base template is omitted and restored by inheritance.
    SithThing sourceThing;
    SithThing readThing;
    SithWorld world;
    STD_ZEROMEM(&sourceThing, sizeof(sourceThing));
    STD_ZEROMEM(&readThing, sizeof(readThing));
    STD_ZEROMEM(&world, sizeof(world));
    sithThingTest_InitResources();

    sithThingTest_baseTemplate.type = SITH_THING_WEAPON;
    sithThingTest_baseTemplate.thingInfo.weaponInfo.pExplosionTemplate = &sithThingTest_explosionTemplate;
    sourceThing.type = SITH_THING_WEAPON;
    sourceThing.pTemplate = &sithThingTest_baseTemplate;
    sourceThing.thingInfo.weaponInfo.pExplosionTemplate = &sithThingTest_explosionTemplate;
    sithThingTest_SetIdentity(&sourceThing.orient);

    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &world, 1u, &sourceThing));
    sithThingTest_AssertMatchesGolden("inherited_weapon.bin");

    // The binary override stays empty because it is identical to the base-template value.
    SithThingTestBinaryView view;
    STD_ZEROMEM(&view, sizeof(view));
    TEST_ASSERT_TRUE(sithThingTest_ParseBinaryView(1u, &view));
    TEST_ASSERT_EQUAL_STRING("", view.aWeaponInfos[0].aExplosionTemplateName);

    // Reading the empty override retains the explosion template inherited during initialization.
    sithThingTest_LoadGoldenFile("inherited_weapon.bin");
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(1u, &world, 1u, &readThing, sithThingTest_InitCallback));
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_baseTemplate, readThing.pTemplate);
    TEST_ASSERT_EQUAL_PTR(&sithThingTest_explosionTemplate, readThing.thingInfo.weaponInfo.pExplosionTemplate);
}

TEST(sithThingBinary, TestWriteAllocationFailures)
{
    // Every temporary writer allocation must fail cleanly and release earlier allocations.
    // Run once successfully to discover and lock down the number of allocation points.
    sithThingTest_InitThings(/*bUseResources=*/0, /*bUseSector=*/0);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));
    size_t numAllocationPoints = sithThingTest_numAllocCalls;
    TEST_ASSERT_EQUAL_size_t(12u, numAllocationPoints);
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);

    // Replay the write with each allocation point failing in turn.
    for ( size_t failCall = 1; failCall <= numAllocationPoints; ++failCall )
    {
        sithThingTest_ResetFile();
        sithThingTest_ResetAllocationFailure();
        sithThingTest_failAllocCall = failCall;

        TEST_ASSERT_EQUAL_INT(1, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));
        TEST_ASSERT_EQUAL_size_t(failCall, sithThingTest_numAllocCalls);
        TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
    }
}

TEST(sithThingBinary, TestWriteFailures)
{
    // Every binary block write must propagate failure without leaking temporary buffers.
    // Run once successfully to discover and lock down the number of write calls.
    sithThingTest_InitThings(/*bUseResources=*/0, /*bUseSector=*/0);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));
    size_t numWritePoints = sithThingTest_file.numIOCalls;
    TEST_ASSERT_EQUAL_size_t(13u, numWritePoints);

    // Replay the write with each I/O call failing in turn.
    for ( size_t failCall = 1; failCall <= numWritePoints; ++failCall )
    {
        sithThingTest_ResetFile();
        sithThingTest_ResetAllocationFailure();
        sithThingTest_file.failIOCall = failCall;

        TEST_ASSERT_EQUAL_INT(1, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));
        TEST_ASSERT_EQUAL_size_t(failCall, sithThingTest_file.numIOCalls);
        TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
    }
}

TEST(sithThingBinary, TestReadAllocationFailures)
{
    // Every reader allocation must propagate failure and leave no owned frame blocks behind.
    // Run once successfully to discover and lock down the number of allocation points.
    sithThingTest_InitThings(/*bUseResources=*/0, /*bUseSector=*/0);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));

    sithThingTest_InitReadThings();
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(
        1u,
        &sithThingTest_world,
        STD_ARRAYLEN(sithThingTest_aReadThings),
        sithThingTest_aReadThings,
        sithThingTest_InitCallback
    ));
    size_t numAllocationPoints = sithThingTest_numAllocCalls;
    TEST_ASSERT_EQUAL_size_t(15u, numAllocationPoints);
    sithThingTest_FreeReadThingData();
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);

    // Replay the read with each allocation point failing in turn.
    for ( size_t failCall = 1; failCall <= numAllocationPoints; ++failCall )
    {
        sithThingTest_InitReadThings();
        sithThingTest_RewindFile();
        sithThingTest_ResetAllocationFailure();
        sithThingTest_failAllocCall = failCall;

        TEST_ASSERT_EQUAL_INT(1, sithThing_ReadThingsListBinary(
            1u,
            &sithThingTest_world,
            STD_ARRAYLEN(sithThingTest_aReadThings),
            sithThingTest_aReadThings,
            sithThingTest_InitCallback
        ));
        TEST_ASSERT_EQUAL_size_t(failCall, sithThingTest_numAllocCalls);
        sithThingTest_FreeReadThingData();
        TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
    }
}

TEST(sithThingBinary, TestReadFailures)
{
    // Every binary block read must propagate failure and release partially restored data.
    // Run once successfully to discover and lock down the number of read calls.
    sithThingTest_InitThings(/*bUseResources=*/0, /*bUseSector=*/0);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));

    sithThingTest_InitReadThings();
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(
        1u,
        &sithThingTest_world,
        STD_ARRAYLEN(sithThingTest_aReadThings),
        sithThingTest_aReadThings,
        sithThingTest_InitCallback
    ));
    size_t numReadPoints = sithThingTest_file.numIOCalls;
    TEST_ASSERT_EQUAL_size_t(13u, numReadPoints);
    sithThingTest_FreeReadThingData();

    // Replay the read with each I/O call failing in turn.
    for ( size_t failCall = 1; failCall <= numReadPoints; ++failCall )
    {
        sithThingTest_InitReadThings();
        sithThingTest_RewindFile();
        sithThingTest_file.failIOCall = failCall;
        sithThingTest_ResetAllocationFailure();

        TEST_ASSERT_EQUAL_INT(1, sithThing_ReadThingsListBinary(
            1u,
            &sithThingTest_world,
            STD_ARRAYLEN(sithThingTest_aReadThings),
            sithThingTest_aReadThings,
            sithThingTest_InitCallback
        ));
        TEST_ASSERT_EQUAL_size_t(failCall, sithThingTest_file.numIOCalls);
        sithThingTest_FreeReadThingData();
        TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
    }
}

TEST(sithThingBinary, TestRejectsMalformedData)
{
    // Exercises malformed counts and truncation through the original reader's allocation and I/O failure paths.
    sithThingTest_InitThings(/*bUseResources=*/0, /*bUseSector=*/0);
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &sithThingTest_world, STD_ARRAYLEN(sithThingTest_aThings), sithThingTest_aThings));
    size_t fullFileSize = sithThingTest_file.size;

    // A negative count becomes an impossible allocation; the bounded test allocator rejects it safely.
    int32_t* aCounts = (int32_t*)&sithThingTest_file.aData[sizeof(CndThingInfo) * STD_ARRAYLEN(sithThingTest_aThings)];
    int32_t savedPhysicsCount = aCounts[SITHTHING_TEST_NUMPHYSICSINFOS];
    aCounts[SITHTHING_TEST_NUMPHYSICSINFOS] = -1;
    sithThingTest_InitReadThings();
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    sithThingTest_maxAllocationSize = SITHTHING_TEST_FILE_CAPACITY;
    TEST_ASSERT_EQUAL_INT(1, sithThing_ReadThingsListBinary(
        1u,
        &sithThingTest_world,
        STD_ARRAYLEN(sithThingTest_aReadThings),
        sithThingTest_aReadThings,
        sithThingTest_InitCallback
    ));
    sithThingTest_FreeReadThingData();
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);

    // An oversized aggregate path-frame block must fail when it reaches the end of the input.
    aCounts[SITHTHING_TEST_NUMPHYSICSINFOS] = savedPhysicsCount;
    int32_t savedPathFrameCount = aCounts[SITHTHING_TEST_NUMPATHFRAMES];
    aCounts[SITHTHING_TEST_NUMPATHFRAMES] = 100;
    sithThingTest_InitReadThings();
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(1, sithThing_ReadThingsListBinary(
        1u,
        &sithThingTest_world,
        STD_ARRAYLEN(sithThingTest_aReadThings),
        sithThingTest_aReadThings,
        sithThingTest_InitCallback
    ));
    sithThingTest_FreeReadThingData();
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);

    // A one-byte-truncated final block must also fail without leaking restored data.
    aCounts[SITHTHING_TEST_NUMPATHFRAMES] = savedPathFrameCount;
    sithThingTest_file.size = fullFileSize - 1u;
    sithThingTest_InitReadThings();
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(1, sithThing_ReadThingsListBinary(
        1u,
        &sithThingTest_world,
        STD_ARRAYLEN(sithThingTest_aReadThings),
        sithThingTest_aReadThings,
        sithThingTest_InitCallback
    ));
    sithThingTest_FreeReadThingData();
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_numLiveAllocs);
}

TEST(sithThingBinary, TestDefaultInitializerPath)
{
    // A null initializer callback selects the reader's built-in thing initialization path.
    SithThing sourceThing;
    SithThing readThing;
    SithWorld world;
    STD_ZEROMEM(&sourceThing, sizeof(sourceThing));
    STD_ZEROMEM(&readThing, sizeof(readThing));
    STD_ZEROMEM(&world, sizeof(world));
    sithThingTest_SetIdentity(&sourceThing.orient);
    sourceThing.type = SITH_THING_FREE;
    STD_STRCPY(sourceThing.aName, "free_thing");

    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(1u, &world, 1u, &sourceThing));
    sithThingTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(1u, &world, 1u, &readThing, NULL));
    TEST_ASSERT_EQUAL_STRING(sourceThing.aName, readThing.aName);
    TEST_ASSERT_EQUAL_INT(SITH_THING_FREE, readThing.type);
}

TEST(sithThingBinary, TestParsesAndSerializesEmptyGolden)
{
    // The empty retail vector must parse and serialize back to the same eleven zero counts.
    STD_ZEROMEM(&sithThingTest_world, sizeof(sithThingTest_world));
    sithThingTest_LoadGoldenFile("empty.bin");
    sithThingTest_RewindFile();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &sithThingTest_world, 0u, NULL, NULL));

    sithThingTest_ResetFile();
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &sithThingTest_world, 0u, NULL));
    sithThingTest_AssertMatchesGolden("empty.bin");
}

TEST(sithThingBinary, TestParsesAndSerializesMaxNamesGolden)
{
    // The retail boundary-name vector must retain every byte through a read/write cycle.
    SithThing thing;
    SithWorld world;
    STD_ZEROMEM(&thing, sizeof(thing));
    STD_ZEROMEM(&world, sizeof(world));

    sithThingTest_LoadGoldenFile("max_names.bin");
    sithThingTest_RewindFile();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &world, 1u, &thing, sithThingTest_InitCallback));

    sithThingTest_ResetFile();
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &world, 1u, &thing));
    sithThingTest_AssertMatchesGolden("max_names.bin");
}

TEST(sithThingBinary, TestParsesAndSerializesInheritedWeaponGolden)
{
    // The inherited explosion template must remain implicit when the retail vector is reserialized.
    SithThing thing;
    SithWorld world;
    STD_ZEROMEM(&thing, sizeof(thing));
    STD_ZEROMEM(&world, sizeof(world));
    sithThingTest_InitResources();
    sithThingTest_baseTemplate.type = SITH_THING_WEAPON;
    sithThingTest_baseTemplate.thingInfo.weaponInfo.pExplosionTemplate = &sithThingTest_explosionTemplate;

    sithThingTest_LoadGoldenFile("inherited_weapon.bin");
    sithThingTest_RewindFile();
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &world, 1u, &thing, sithThingTest_InitCallback));

    sithThingTest_ResetFile();
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &world, 1u, &thing));
    sithThingTest_AssertMatchesGolden("inherited_weapon.bin");
}

static void sithThingTest_CreateStaticBinary(void)
{
    // Produce wrapper input from the independently parsed synthetic text section.
    sithThingTest_InitResources();
    sithThingTest_InitStaticWorld(0u);
    sithThingTest_OpenConffile("static_things.txt");
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadStaticThingsListText(&sithThingTest_staticWorld, /*bSkip=*/0));
    sithThingTest_CloseConffile();

    sithThingTest_ResetFile();
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteStaticThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &sithThingTest_staticWorld));
    sithThingTest_FreeStaticWorld();
    sithThingTest_RewindFile();
}

TEST(sithThingBinary, TestReadsStaticTextAndWritesBinary)
{
    // Parses representative static placements, validates runtime metadata, and emits a complete binary list.
    // Start with the text loader and its world-allocation and sector-insertion behavior.
    sithThingTest_InitResources();
    sithThingTest_InitStaticWorld(0u);
    sithThingTest_OpenConffile("static_things.txt");
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadStaticThingsListText(&sithThingTest_staticWorld, /*bSkip=*/0));
    sithThingTest_CloseConffile();

    // Check parsed fields plus the GUIDs, signatures, and sector links assigned by the wrapper.
    TEST_ASSERT_EQUAL_size_t(4u, sithThingTest_staticWorld.numThings);
    TEST_ASSERT_EQUAL_INT(3, sithThingTest_staticWorld.lastThingIdx);
    TEST_ASSERT_EQUAL_INT(SITH_THING_CAMERA, sithThingTest_staticWorld.aThings[0].type);
    TEST_ASSERT_EQUAL_INT(SITH_THING_HINT, sithThingTest_staticWorld.aThings[1].type);
    TEST_ASSERT_EQUAL_INT(SITH_THING_COG, sithThingTest_staticWorld.aThings[2].type);
    TEST_ASSERT_EQUAL_INT(SITH_THING_GHOST, sithThingTest_staticWorld.aThings[3].type);
    TEST_ASSERT_EQUAL_STRING("text_camera", sithThingTest_staticWorld.aThings[0].aName);
    TEST_ASSERT_EQUAL_STRING("text_hint", sithThingTest_staticWorld.aThings[1].aName);
    TEST_ASSERT_EQUAL_FLOAT(7.5f, sithThingTest_staticWorld.aThings[1].userval);
    TEST_ASSERT_EQUAL_FLOAT(1.25f, sithThingTest_staticWorld.aThings[1].collide.height);
    TEST_ASSERT_EQUAL_FLOAT(0.75f, sithThingTest_staticWorld.aThings[1].collide.width);
    for ( size_t i = 0; i < sithThingTest_staticWorld.numThings; ++i )
    {
        TEST_ASSERT_EQUAL_INT((int)i, sithThingTest_staticWorld.aThings[i].guid);
        TEST_ASSERT_EQUAL_UINT32((uint32_t)i + 1u, sithThingTest_staticWorld.aThings[i].signature);
        TEST_ASSERT_EQUAL_PTR(&sithThingTest_aStaticSectors[0], sithThingTest_staticWorld.aThings[i].pInSector);
    }

    // Serialize the resulting world and inspect the generic list blocks emitted by the wrapper.
    sithThingTest_ResetFile();
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteStaticThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &sithThingTest_staticWorld));
    SithThingTestBinaryView view;
    STD_ZEROMEM(&view, sizeof(view));
    TEST_ASSERT_TRUE(sithThingTest_ParseBinaryView(4u, &view));
    TEST_ASSERT_EQUAL_STRING("text_camera", view.aThingInfos[0].aName);
    TEST_ASSERT_EQUAL_STRING("text_hint", view.aThingInfos[1].aName);
    TEST_ASSERT_EQUAL_STRING("text_cog", view.aThingInfos[2].aName);
    TEST_ASSERT_EQUAL_STRING("text_ghost", view.aThingInfos[3].aName);
    TEST_ASSERT_EQUAL_INT(1, view.aCounts[SITHTHING_TEST_NUMUSERVALUES]);
}

TEST(sithThingBinary, TestReadsAndReserializesStaticBinary)
{
    // Restores wrapper-produced bytes, rebuilds runtime IDs, and requires stable reserialization.
    sithThingTest_CreateStaticBinary();
    size_t expectedSize = sithThingTest_file.size;
    STD_COPYMEM(sithThingTest_aGoldenData, sithThingTest_file.aData, expectedSize);

    sithThingTest_InitStaticWorld(4u);
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadStaticThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &sithThingTest_staticWorld));

    // The wrapper assigns GUIDs and continues the signature sequence used while producing the input.
    TEST_ASSERT_EQUAL_size_t(4u, sithThingTest_staticWorld.numThings);
    TEST_ASSERT_EQUAL_INT(3, sithThingTest_staticWorld.lastThingIdx);
    for ( size_t i = 0; i < sithThingTest_staticWorld.numThings; ++i )
    {
        TEST_ASSERT_EQUAL_INT((int)i, sithThingTest_staticWorld.aThings[i].guid);
        TEST_ASSERT_EQUAL_UINT32((uint32_t)i + 5u, sithThingTest_staticWorld.aThings[i].signature);
    }

    // Writing the restored world must reproduce the complete static-list vector.
    sithThingTest_ResetFile();
    TEST_ASSERT_EQUAL_INT(0, sithThing_WriteStaticThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &sithThingTest_staticWorld));
    TEST_ASSERT_EQUAL_size_t(expectedSize, sithThingTest_file.size);
    TEST_ASSERT_EQUAL_MEMORY(sithThingTest_aGoldenData, sithThingTest_file.aData, expectedSize);
}

TEST(sithThingBinary, TestStaticListPerformanceAndSkip)
{
    // Exercises performance filtering in both readers and preserves the original text skip behavior.
    sithThingTest_InitResources();
    sithThingTest_InitStaticWorld(0u);
    sithThingTest_performanceLevel = 1;
    sithThingTest_OpenConffile("static_things.txt");
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadStaticThingsListText(&sithThingTest_staticWorld, /*bSkip=*/0));
    sithThingTest_CloseConffile();
    TEST_ASSERT_EQUAL_INT(SITH_THING_CAMERA, sithThingTest_staticWorld.aThings[0].type);
    TEST_ASSERT_EQUAL_INT(SITH_THING_FREE, sithThingTest_staticWorld.aThings[1].type);
    TEST_ASSERT_EQUAL_INT(SITH_THING_COG, sithThingTest_staticWorld.aThings[2].type);
    TEST_ASSERT_EQUAL_INT(SITH_THING_FREE, sithThingTest_staticWorld.aThings[3].type);

    // bSkip frees an existing array but, as in the original, continues by parsing a replacement list.
    sithThingTest_performanceLevel = 3;
    sithThingTest_OpenConffile("static_things.txt");
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadStaticThingsListText(&sithThingTest_staticWorld, /*bSkip=*/1));
    sithThingTest_CloseConffile();
    TEST_ASSERT_EQUAL_INT(SITH_THING_HINT, sithThingTest_staticWorld.aThings[1].type);
    TEST_ASSERT_EQUAL_INT(SITH_THING_GHOST, sithThingTest_staticWorld.aThings[3].type);

    // The binary wrapper applies the same performance-level rejection after deserialization.
    sithThingTest_FreeStaticWorld();
    sithThingTest_CreateStaticBinary();
    sithThingTest_performanceLevel = 1;
    sithThingTest_InitStaticWorld(4u);
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadStaticThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &sithThingTest_staticWorld));
    TEST_ASSERT_EQUAL_INT(SITH_THING_CAMERA, sithThingTest_staticWorld.aThings[0].type);
    TEST_ASSERT_EQUAL_INT(SITH_THING_FREE, sithThingTest_staticWorld.aThings[1].type);
    TEST_ASSERT_EQUAL_INT(SITH_THING_COG, sithThingTest_staticWorld.aThings[2].type);
    TEST_ASSERT_EQUAL_INT(SITH_THING_FREE, sithThingTest_staticWorld.aThings[3].type);
}

TEST(sithThingBinary, TestStaticListFailures)
{
    // Covers malformed text, allocation failures, skipped placements, and binary wrapper failures.
    sithThingTest_InitResources();
    sithThingTest_InitStaticWorld(0u);

    // A malformed section header rejects the text list before allocating the thing array.
    sithThingTest_OpenConffile("static_bad_header.txt");
    TEST_ASSERT_EQUAL_INT(1, sithThing_ReadStaticThingsListText(&sithThingTest_staticWorld, /*bSkip=*/0));
    sithThingTest_CloseConffile();
    TEST_ASSERT_NULL(sithThingTest_staticWorld.aThings);

    // Failure to allocate the text-list thing array is propagated without retaining a world buffer.
    sithThingTest_OpenConffile("static_things.txt");
    sithThingTest_ResetAllocationFailure();
    sithThingTest_failAllocCall = 1u;
    TEST_ASSERT_EQUAL_INT(1, sithThing_ReadStaticThingsListText(&sithThingTest_staticWorld, /*bSkip=*/0));
    sithThingTest_CloseConffile();
    TEST_ASSERT_NULL(sithThingTest_staticWorld.aThings);

    // An invalid placement is skipped softly, leaving its allocated slot as a free thing.
    sithThingTest_ResetAllocationFailure();
    sithThingTest_OpenConffile("static_bad_placement.txt");
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadStaticThingsListText(&sithThingTest_staticWorld, /*bSkip=*/0));
    sithThingTest_CloseConffile();
    TEST_ASSERT_EQUAL_size_t(1u, sithThingTest_staticWorld.numThings);
    TEST_ASSERT_EQUAL_INT(SITH_THING_FREE, sithThingTest_staticWorld.aThings[0].type);
    sithThingTest_FreeStaticWorld();

    // The static writer propagates failure from the first generic serializer write.
    sithThingTest_InitResources();
    sithThingTest_InitStaticWorld(0u);
    sithThingTest_OpenConffile("static_things.txt");
    TEST_ASSERT_EQUAL_INT(0, sithThing_ReadStaticThingsListText(&sithThingTest_staticWorld, /*bSkip=*/0));
    sithThingTest_CloseConffile();
    sithThingTest_ResetFile();
    sithThingTest_file.failIOCall = 1u;
    TEST_ASSERT_EQUAL_INT(1, sithThing_WriteStaticThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &sithThingTest_staticWorld));
    sithThingTest_FreeStaticWorld();

    // Failure to allocate the binary-list thing array leaves the world unallocated.
    sithThingTest_CreateStaticBinary();
    sithThingTest_InitStaticWorld(4u);
    sithThingTest_ResetAllocationFailure();
    sithThingTest_failAllocCall = 1u;
    TEST_ASSERT_EQUAL_INT(1, sithThing_ReadStaticThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &sithThingTest_staticWorld));
    TEST_ASSERT_NULL(sithThingTest_staticWorld.aThings);

    // A binary I/O failure is reported after allocation; teardown owns and releases the retained array.
    sithThingTest_CreateStaticBinary();
    sithThingTest_InitStaticWorld(4u);
    sithThingTest_file.failIOCall = 1u;
    TEST_ASSERT_EQUAL_INT(1, sithThing_ReadStaticThingsListBinary(SITHTHING_TEST_BINARY_HANDLE, &sithThingTest_staticWorld));
    TEST_ASSERT_NOT_NULL(sithThingTest_staticWorld.aThings);
}

TEST_GROUP_RUNNER(sithThingBinary)
{
    RUN_TEST_CASE(sithThingBinary, TestWritesAllThingData);
    RUN_TEST_CASE(sithThingBinary, TestRoundTripsAllThingData);
    RUN_TEST_CASE(sithThingBinary, TestRestoresResourcesAndSector);
    RUN_TEST_CASE(sithThingBinary, TestRejectsMissingPuppet);
    RUN_TEST_CASE(sithThingBinary, TestAllowsExistingRenderPuppet);
    RUN_TEST_CASE(sithThingBinary, TestHandlesOptionalResourceFailures);
    RUN_TEST_CASE(sithThingBinary, TestEmptyList);
    RUN_TEST_CASE(sithThingBinary, TestPreservesBoundedNames);
    RUN_TEST_CASE(sithThingBinary, TestPreservesInheritedWeaponExplosion);
    RUN_TEST_CASE(sithThingBinary, TestWriteAllocationFailures);
    RUN_TEST_CASE(sithThingBinary, TestWriteFailures);
    RUN_TEST_CASE(sithThingBinary, TestReadAllocationFailures);
    RUN_TEST_CASE(sithThingBinary, TestReadFailures);
    RUN_TEST_CASE(sithThingBinary, TestRejectsMalformedData);
    RUN_TEST_CASE(sithThingBinary, TestDefaultInitializerPath);
    RUN_TEST_CASE(sithThingBinary, TestParsesAndSerializesEmptyGolden);
    RUN_TEST_CASE(sithThingBinary, TestParsesAndSerializesMaxNamesGolden);
    RUN_TEST_CASE(sithThingBinary, TestParsesAndSerializesInheritedWeaponGolden);
    RUN_TEST_CASE(sithThingBinary, TestReadsStaticTextAndWritesBinary);
    RUN_TEST_CASE(sithThingBinary, TestReadsAndReserializesStaticBinary);
    RUN_TEST_CASE(sithThingBinary, TestStaticListPerformanceAndSkip);
    RUN_TEST_CASE(sithThingBinary, TestStaticListFailures);
}
