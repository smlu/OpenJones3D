#include <unity_fixture.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

#if defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <sith/Main/sithMain.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdConffile.h>
#include <std/General/stdUtil.h>

#include "sithThingTestSupport.h"

#define SITHTEMPLATE_TEST_BINARY_HANDLE ((tFileHandle)2u)
#define SITHTEMPLATE_TEST_FILE_CAPACITY (16u * 1024u)
#define SITHTEMPLATE_TEST_PATHSIZE      1024u

typedef struct sSithTemplateTestFile
{
    uint8_t aData[SITHTEMPLATE_TEST_FILE_CAPACITY];
    size_t size;
    size_t pos;
    size_t numIOCalls;
    size_t failIOCall;
    int bOverflow;
} SithTemplateTestFile;

static SithTemplateTestFile sithTemplateTest_file;
static tHostServices sithTemplateTest_hostServices;
static SithWorld sithTemplateTest_world;
static SithWorld sithTemplateTest_readWorld;
static int sithTemplateTest_bConffileOpen;
#if defined(_DEBUG)
static size_t sithTemplateTest_previousDebugFillThreshold;
#endif

TEST_GROUP(sithTemplate);

static void J3DAPI sithTemplateTest_Assert(const char* pText, const char* pFile, int line)
{
    (void)pFile;
    (void)line;
    TEST_FAIL_MESSAGE(pText);
}

static int J3DAPI sithTemplateTest_Print(const char* pFormat, ...)
{
    (void)pFormat;
    return 0;
}

static tFileHandle J3DAPI sithTemplateTest_FileOpen(const char* pFilename, const char* pMode)
{
    FILE* pFile = fopen(pFilename, pMode);
    return (tFileHandle)(uintptr_t)pFile;
}

static int J3DAPI sithTemplateTest_FileClose(tFileHandle fh)
{
    if ( !fh || fh == SITHTEMPLATE_TEST_BINARY_HANDLE )
    {
        return -1;
    }
    return fclose((FILE*)(uintptr_t)fh);
}

static size_t J3DAPI sithTemplateTest_FileRead(tFileHandle fh, void* pData, size_t size)
{
    if ( fh != SITHTEMPLATE_TEST_BINARY_HANDLE )
    {
        return fread(pData, 1u, size, (FILE*)(uintptr_t)fh);
    }

    ++sithTemplateTest_file.numIOCalls;
    if ( sithTemplateTest_file.failIOCall == sithTemplateTest_file.numIOCalls )
    {
        return size ? size - 1u : 1u;
    }

    size_t available = sithTemplateTest_file.pos < sithTemplateTest_file.size
        ? sithTemplateTest_file.size - sithTemplateTest_file.pos
        : 0u;
    size_t readSize  = size < available ? size : available;
    if ( readSize )
    {
        STD_COPYMEM(pData, &sithTemplateTest_file.aData[sithTemplateTest_file.pos], readSize);
        sithTemplateTest_file.pos += readSize;
    }
    return readSize;
}

static size_t J3DAPI sithTemplateTest_FileWrite(tFileHandle fh, const void* pData, size_t size)
{
    if ( fh != SITHTEMPLATE_TEST_BINARY_HANDLE )
    {
        return fwrite(pData, 1u, size, (FILE*)(uintptr_t)fh);
    }

    ++sithTemplateTest_file.numIOCalls;
    if ( sithTemplateTest_file.failIOCall == sithTemplateTest_file.numIOCalls )
    {
        return size ? size - 1u : 1u;
    }

    if ( size > sizeof(sithTemplateTest_file.aData) - sithTemplateTest_file.size )
    {
        sithTemplateTest_file.bOverflow = 1;
        return 0;
    }

    if ( size )
    {
        STD_COPYMEM(&sithTemplateTest_file.aData[sithTemplateTest_file.size], pData, size);
        sithTemplateTest_file.size += size;
    }
    return size;
}

static char* J3DAPI sithTemplateTest_FileGets(tFileHandle fh, char* pString, size_t size)
{
    if ( !fh || fh == SITHTEMPLATE_TEST_BINARY_HANDLE || size > INT_MAX )
    {
        return NULL;
    }
    return fgets(pString, (int)size, (FILE*)(uintptr_t)fh);
}

static void sithTemplateTest_ResetFile(void)
{
    STD_ZEROMEM(&sithTemplateTest_file, sizeof(sithTemplateTest_file));
}

static void sithTemplateTest_RewindFile(void)
{
    sithTemplateTest_file.pos        = 0;
    sithTemplateTest_file.numIOCalls = 0;
    sithTemplateTest_file.failIOCall = 0;
}

static void sithTemplateTest_MakeVectorPath(char* pPath, size_t pathSize, const char* pFileName)
{
    stdUtil_Format(pPath, pathSize, "%s/sithTemplate/%s", SITH_TEST_TV_DIR, pFileName);
    pPath[pathSize - 1u] = '\0';
}

static void sithTemplateTest_OpenConffile(const char* pFileName)
{
    char aPath[SITHTEMPLATE_TEST_PATHSIZE];
    sithTemplateTest_MakeVectorPath(aPath, STD_ARRAYLEN(aPath), pFileName);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, stdConffile_Open(aPath), aPath);
    sithTemplateTest_bConffileOpen = 1;
}

static void sithTemplateTest_CloseConffile(void)
{
    TEST_ASSERT_TRUE(sithTemplateTest_bConffileOpen);
    stdConffile_Close();
    sithTemplateTest_bConffileOpen = 0;
}

static void sithTemplateTest_LoadText(SithWorld* pWorld, const char* pFileName)
{
    sithTemplateTest_OpenConffile(pFileName);
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_ReadThingTemplatesListText(pWorld, /*bSkip=*/0));
    sithTemplateTest_CloseConffile();
}

static void sithTemplateTest_FreeWorldTemplates(SithWorld* pWorld)
{
    if ( !pWorld->aThingTemplates )
    {
        return;
    }

    // A failed binary read leaves the requested count in the world, so retain only initialized entries for cleanup.
    size_t numInitialized = 0;
    while ( numInitialized < pWorld->sizeThingTemplates && pWorld->aThingTemplates[numInitialized].aName[0] )
    {
        ++numInitialized;
    }
    pWorld->numThingTemplates = numInitialized;
    sithTemplate_FreeWorldTemplates(pWorld);
}

static void sithTemplateTest_AssertLoadedTemplates(const SithWorld* pWorld)
{
    TEST_ASSERT_EQUAL_size_t(5u, pWorld->sizeThingTemplates);
    TEST_ASSERT_EQUAL_size_t(4u, pWorld->numThingTemplates);

    const SithThing* pCamera = &pWorld->aThingTemplates[0];
    TEST_ASSERT_EQUAL_INT(0, pCamera->idx);
    TEST_ASSERT_EQUAL_STRING("tpl_camera", pCamera->aName);
    TEST_ASSERT_EQUAL_INT(SITH_THING_CAMERA, pCamera->type);
    TEST_ASSERT_NULL(pCamera->pTemplate);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, pCamera->collide.size);
    TEST_ASSERT_EQUAL_UINT32(2250u, pCamera->msecLifeLeft);
    TEST_ASSERT_EQUAL_FLOAT(4.5f, pCamera->light.maxRadius);

    const SithThing* pDerived = &pWorld->aThingTemplates[1];
    TEST_ASSERT_EQUAL_INT(1, pDerived->idx);
    TEST_ASSERT_EQUAL_STRING("tpl_camera_large", pDerived->aName);
    TEST_ASSERT_EQUAL_INT(SITH_THING_CAMERA, pDerived->type);
    TEST_ASSERT_EQUAL_PTR(pCamera, pDerived->pTemplate);
    TEST_ASSERT_EQUAL_FLOAT(0.75f, pDerived->collide.size);
    TEST_ASSERT_EQUAL_FLOAT(1.25f, pDerived->collide.height);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, pDerived->collide.width);
    TEST_ASSERT_EQUAL_UINT32(2250u, pDerived->msecLifeLeft);
    TEST_ASSERT_EQUAL_FLOAT(4.5f, pDerived->light.maxRadius);

    const SithThing* pHint = &pWorld->aThingTemplates[2];
    TEST_ASSERT_EQUAL_INT(2, pHint->idx);
    TEST_ASSERT_EQUAL_STRING("tpl_hint", pHint->aName);
    TEST_ASSERT_EQUAL_INT(SITH_THING_HINT, pHint->type);
    TEST_ASSERT_EQUAL_FLOAT(7.5f, pHint->userval);

    const SithThing* pGhost = &pWorld->aThingTemplates[3];
    TEST_ASSERT_EQUAL_INT(3, pGhost->idx);
    TEST_ASSERT_EQUAL_STRING("tpl_ghost", pGhost->aName);
    TEST_ASSERT_EQUAL_INT(SITH_THING_GHOST, pGhost->type);
    TEST_ASSERT_NULL(pGhost->pTemplate);
    TEST_ASSERT_EQUAL_HEX32(0x400u, pGhost->flags);

    TEST_ASSERT_EQUAL_PTR(pCamera, sithTemplate_GetTemplate("tpl_camera"));
    TEST_ASSERT_EQUAL_PTR(pDerived, sithTemplate_GetTemplate("tpl_camera_large"));
    TEST_ASSERT_EQUAL_PTR(pHint, sithTemplate_GetTemplate("tpl_hint"));
    TEST_ASSERT_EQUAL_PTR(pGhost, sithTemplate_GetTemplate("tpl_ghost"));
    TEST_ASSERT_NULL(sithTemplate_GetTemplate("tpl_invalid"));
}

TEST_SETUP(sithTemplate)
{
    // Start both owners because template parsing uses the thing argument table and its own name cache.
#if defined(_DEBUG)
    // Keep fixed-size serialized string tails deterministic, matching the retail-vector fixture.
    sithTemplateTest_previousDebugFillThreshold = _CrtSetDebugFillThreshold(0u);
#endif

    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_GetNumLiveAllocs());
    sithThingTest_ResetHashtables();
    sithThingTest_ResetAllocationFailure();
    sithThingTest_ResetLoadState();
    sithTemplateTest_ResetFile();
    STD_ZEROMEM(&sithTemplateTest_hostServices, sizeof(sithTemplateTest_hostServices));
    STD_ZEROMEM(&sithTemplateTest_world, sizeof(sithTemplateTest_world));
    STD_ZEROMEM(&sithTemplateTest_readWorld, sizeof(sithTemplateTest_readWorld));
    sithTemplateTest_bConffileOpen = 0;

    sithTemplateTest_hostServices.pAssert       = sithTemplateTest_Assert;
    sithTemplateTest_hostServices.pStatusPrint  = sithTemplateTest_Print;
    sithTemplateTest_hostServices.pMessagePrint = sithTemplateTest_Print;
    sithTemplateTest_hostServices.pWarningPrint = sithTemplateTest_Print;
    sithTemplateTest_hostServices.pErrorPrint   = sithTemplateTest_Print;
    sithTemplateTest_hostServices.pDebugPrint   = sithTemplateTest_Print;
    sithTemplateTest_hostServices.pMalloc       = sithThingTest_Malloc;
    sithTemplateTest_hostServices.pFree         = sithThingTest_Free;
    sithTemplateTest_hostServices.pFileOpen     = sithTemplateTest_FileOpen;
    sithTemplateTest_hostServices.pFileClose    = sithTemplateTest_FileClose;
    sithTemplateTest_hostServices.pFileRead     = sithTemplateTest_FileRead;
    sithTemplateTest_hostServices.pFileGets     = sithTemplateTest_FileGets;
    sithTemplateTest_hostServices.pFileWrite    = sithTemplateTest_FileWrite;
    sith_g_pHS = &sithTemplateTest_hostServices;
    std_g_pHS  = &sithTemplateTest_hostServices;

    TEST_ASSERT_EQUAL_INT(1, sithThing_Startup());
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_Startup());
}

TEST_TEAR_DOWN(sithTemplate)
{
    // Normalize partial reader state, release both caches, and require every test allocation to be returned.
    sithThingTest_SetFailAllocCall(0u);
    if ( sithTemplateTest_bConffileOpen )
    {
        sithTemplateTest_CloseConffile();
    }
    sithTemplateTest_FreeWorldTemplates(&sithTemplateTest_readWorld);
    sithTemplateTest_FreeWorldTemplates(&sithTemplateTest_world);
    sithTemplate_Shutdown();
    TEST_ASSERT_EQUAL_INT(1, sithThing_Shutdown());
    TEST_ASSERT_EQUAL_size_t(0u, sithThingTest_GetNumLiveAllocs());
#if defined(_DEBUG)
    _CrtSetDebugFillThreshold(sithTemplateTest_previousDebugFillThreshold);
#endif
}

TEST(sithTemplate, TestReadsTextTemplates)
{
    // Parse blank and inherited templates, then verify indices, fields, base links, and cache entries.
    sithTemplateTest_LoadText(&sithTemplateTest_world, "templates.txt");
    sithTemplateTest_AssertLoadedTemplates(&sithTemplateTest_world);
}

TEST(sithTemplate, TestTextControlFlow)
{
    // bSkip returns immediately and leaves the supplied world untouched.
    sithTemplateTest_world.numThingTemplates = 7u;
    TEST_ASSERT_EQUAL_INT(1, sithTemplate_ReadThingTemplatesListText(&sithTemplateTest_world, /*bSkip=*/1));
    TEST_ASSERT_EQUAL_size_t(7u, sithTemplateTest_world.numThingTemplates);
    sithTemplateTest_world.numThingTemplates = 0u;

    // Invalid headers and zero-capacity sections terminate before allocating a template array.
    sithTemplateTest_OpenConffile("templates_bad_header.txt");
    TEST_ASSERT_EQUAL_INT(1, sithTemplate_ReadThingTemplatesListText(&sithTemplateTest_world, /*bSkip=*/0));
    sithTemplateTest_CloseConffile();
    TEST_ASSERT_NULL(sithTemplateTest_world.aThingTemplates);

    sithTemplateTest_OpenConffile("templates_zero.txt");
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_ReadThingTemplatesListText(&sithTemplateTest_world, /*bSkip=*/0));
    sithTemplateTest_CloseConffile();
    TEST_ASSERT_NULL(sithTemplateTest_world.aThingTemplates);

    // Allocation failure is propagated after the header has been accepted.
    sithTemplateTest_OpenConffile("templates.txt");
    sithThingTest_ResetAllocationFailure();
    sithThingTest_SetFailAllocCall(1u);
    TEST_ASSERT_EQUAL_INT(1, sithTemplate_ReadThingTemplatesListText(&sithTemplateTest_world, /*bSkip=*/0));
    sithThingTest_SetFailAllocCall(0u);
    sithTemplateTest_CloseConffile();
    TEST_ASSERT_NULL(sithTemplateTest_world.aThingTemplates);

    // Parsing is intentionally soft for capacity overflow, duplicate names, and EOF without an end marker.
    sithTemplateTest_LoadText(&sithTemplateTest_world, "templates_capacity.txt");
    TEST_ASSERT_EQUAL_size_t(1u, sithTemplateTest_world.numThingTemplates);
    TEST_ASSERT_EQUAL_STRING("tpl_first", sithTemplateTest_world.aThingTemplates[0].aName);
    TEST_ASSERT_NULL(sithTemplate_GetTemplate("tpl_overflow"));
    sithTemplateTest_FreeWorldTemplates(&sithTemplateTest_world);

    sithTemplateTest_LoadText(&sithTemplateTest_world, "templates_duplicate.txt");
    TEST_ASSERT_EQUAL_size_t(1u, sithTemplateTest_world.numThingTemplates);
    TEST_ASSERT_EQUAL_INT(SITH_THING_CAMERA, sithTemplateTest_world.aThingTemplates[0].type);
    sithTemplateTest_FreeWorldTemplates(&sithTemplateTest_world);

    sithTemplateTest_LoadText(&sithTemplateTest_world, "templates_no_end.txt");
    TEST_ASSERT_EQUAL_size_t(1u, sithTemplateTest_world.numThingTemplates);
    TEST_ASSERT_EQUAL_STRING("tpl_eof", sithTemplateTest_world.aThingTemplates[0].aName);
}

TEST(sithTemplate, TestAllocatesAndFreesWorldTemplates)
{
    // Zero-size allocation is a no-op, while static worlds receive masked resource indices.
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_AllocWorldTemplates(&sithTemplateTest_world, 0u));
    TEST_ASSERT_NULL(sithTemplateTest_world.aThingTemplates);

    sithTemplateTest_world.state = SITH_WORLD_STATE_STATIC;
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_AllocWorldTemplates(&sithTemplateTest_world, 3u));
    TEST_ASSERT_EQUAL_size_t(3u, sithTemplateTest_world.sizeThingTemplates);
    TEST_ASSERT_EQUAL_size_t(0u, sithTemplateTest_world.numThingTemplates);
    for ( size_t i = 0; i < sithTemplateTest_world.sizeThingTemplates; ++i )
    {
        TEST_ASSERT_EQUAL_INT(SITH_THING_FREE, sithTemplateTest_world.aThingTemplates[i].type);
        TEST_ASSERT_EQUAL_INT(SITHWORLD_STATICINDEX(i), sithTemplateTest_world.aThingTemplates[i].idx);
    }
    sithTemplateTest_FreeWorldTemplates(&sithTemplateTest_world);
    TEST_ASSERT_NULL(sithTemplateTest_world.aThingTemplates);
    TEST_ASSERT_EQUAL_size_t(0u, sithTemplateTest_world.sizeThingTemplates);

    // The world remains empty when its backing allocation cannot be made.
    sithThingTest_ResetAllocationFailure();
    sithThingTest_SetFailAllocCall(1u);
    TEST_ASSERT_EQUAL_INT(1, sithTemplate_AllocWorldTemplates(&sithTemplateTest_world, 2u));
    TEST_ASSERT_EQUAL_size_t(1u, sithThingTest_GetNumAllocCalls());
    TEST_ASSERT_NULL(sithTemplateTest_world.aThingTemplates);
}

TEST(sithTemplate, TestBinaryRoundTrip)
{
    // Serialize text-parsed templates, then preserve the exact stream while rebuilding inheritance and cache state.
    uint8_t aExpected[SITHTEMPLATE_TEST_FILE_CAPACITY];
    sithTemplateTest_LoadText(&sithTemplateTest_world, "templates.txt");
    sithTemplateTest_ResetFile();
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_WriteThingTemplatesListBinary(SITHTEMPLATE_TEST_BINARY_HANDLE, &sithTemplateTest_world));
    TEST_ASSERT_FALSE(sithTemplateTest_file.bOverflow);
    size_t expectedSize = sithTemplateTest_file.size;
    STD_COPYMEM(aExpected, sithTemplateTest_file.aData, expectedSize);

    // The first block consists of the four serialized template records in list order.
    const CndThingInfo* aThingInfos = (const CndThingInfo*)sithTemplateTest_file.aData;
    TEST_ASSERT_EQUAL_STRING("tpl_camera", aThingInfos[0].aName);
    TEST_ASSERT_EQUAL_STRING("tpl_camera_large", aThingInfos[1].aName);
    TEST_ASSERT_EQUAL_STRING("tpl_hint", aThingInfos[2].aName);
    TEST_ASSERT_EQUAL_STRING("tpl_ghost", aThingInfos[3].aName);

    // Remove source cache entries before proving that the binary callback repopulates them in dependency order.
    sithTemplateTest_FreeWorldTemplates(&sithTemplateTest_world);
    sithTemplateTest_readWorld.numThingTemplates = 4u;
    sithTemplateTest_RewindFile();
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_ReadThingTemplatesListBinary(SITHTEMPLATE_TEST_BINARY_HANDLE, &sithTemplateTest_readWorld));
    TEST_ASSERT_EQUAL_size_t(4u, sithTemplateTest_readWorld.sizeThingTemplates);
    TEST_ASSERT_EQUAL_size_t(4u, sithTemplateTest_readWorld.numThingTemplates);
    TEST_ASSERT_EQUAL_PTR(
        &sithTemplateTest_readWorld.aThingTemplates[0],
        sithTemplateTest_readWorld.aThingTemplates[1].pTemplate
    );
    TEST_ASSERT_EQUAL_PTR(
        &sithTemplateTest_readWorld.aThingTemplates[1],
        sithTemplate_GetTemplate("tpl_camera_large")
    );
    TEST_ASSERT_EQUAL_FLOAT(7.5f, sithTemplateTest_readWorld.aThingTemplates[2].userval);

    sithTemplateTest_ResetFile();
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_WriteThingTemplatesListBinary(SITHTEMPLATE_TEST_BINARY_HANDLE, &sithTemplateTest_readWorld));
    TEST_ASSERT_EQUAL_size_t(expectedSize, sithTemplateTest_file.size);
    TEST_ASSERT_EQUAL_MEMORY(aExpected, sithTemplateTest_file.aData, expectedSize);
}

TEST(sithTemplate, TestEmptyBinaryRoundTrip)
{
    // An empty template section uses the generic eleven-count header and performs no allocations.
    sithTemplateTest_ResetFile();
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_WriteThingTemplatesListBinary(SITHTEMPLATE_TEST_BINARY_HANDLE, &sithTemplateTest_world));
    TEST_ASSERT_EQUAL_size_t(11u * sizeof(int32_t), sithTemplateTest_file.size);
    for ( size_t i = 0; i < sithTemplateTest_file.size; ++i )
    {
        TEST_ASSERT_EQUAL_UINT8(0u, sithTemplateTest_file.aData[i]);
    }

    sithTemplateTest_RewindFile();
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_ReadThingTemplatesListBinary(SITHTEMPLATE_TEST_BINARY_HANDLE, &sithTemplateTest_readWorld));
    TEST_ASSERT_NULL(sithTemplateTest_readWorld.aThingTemplates);
    TEST_ASSERT_EQUAL_size_t(0u, sithTemplateTest_readWorld.numThingTemplates);
}

TEST(sithTemplate, TestBinaryFailures)
{
    // Build a valid source list once, then exercise wrapper-level write, allocation, I/O, and truncation failures.
    sithTemplateTest_LoadText(&sithTemplateTest_world, "templates.txt");
    sithTemplateTest_ResetFile();
    sithTemplateTest_file.failIOCall = 1u;
    TEST_ASSERT_EQUAL_INT(1, sithTemplate_WriteThingTemplatesListBinary(SITHTEMPLATE_TEST_BINARY_HANDLE, &sithTemplateTest_world));

    sithTemplateTest_ResetFile();
    sithThingTest_ResetAllocationFailure();
    TEST_ASSERT_EQUAL_INT(0, sithTemplate_WriteThingTemplatesListBinary(SITHTEMPLATE_TEST_BINARY_HANDLE, &sithTemplateTest_world));
    size_t validSize = sithTemplateTest_file.size;
    sithTemplateTest_FreeWorldTemplates(&sithTemplateTest_world);

    // Failure of the wrapper's world-array allocation occurs before any input is consumed.
    sithTemplateTest_readWorld.numThingTemplates = 4u;
    sithTemplateTest_RewindFile();
    sithThingTest_ResetAllocationFailure();
    sithThingTest_SetFailAllocCall(1u);
    TEST_ASSERT_EQUAL_INT(1, sithTemplate_ReadThingTemplatesListBinary(SITHTEMPLATE_TEST_BINARY_HANDLE, &sithTemplateTest_readWorld));
    TEST_ASSERT_NULL(sithTemplateTest_readWorld.aThingTemplates);
    TEST_ASSERT_EQUAL_size_t(0u, sithTemplateTest_file.pos);

    // A short first read is propagated after allocation and leaves no initialized cache entries.
    sithThingTest_SetFailAllocCall(0u);
    sithThingTest_ResetAllocationFailure();
    STD_ZEROMEM(&sithTemplateTest_readWorld, sizeof(sithTemplateTest_readWorld));
    sithTemplateTest_readWorld.numThingTemplates = 4u;
    sithTemplateTest_RewindFile();
    sithTemplateTest_file.failIOCall = 1u;
    TEST_ASSERT_EQUAL_INT(1, sithTemplate_ReadThingTemplatesListBinary(SITHTEMPLATE_TEST_BINARY_HANDLE, &sithTemplateTest_readWorld));
    TEST_ASSERT_NOT_NULL(sithTemplateTest_readWorld.aThingTemplates);
    sithTemplateTest_FreeWorldTemplates(&sithTemplateTest_readWorld);

    // Truncating the final byte rejects an otherwise valid stream and cleanup removes any earlier cache callbacks.
    STD_ZEROMEM(&sithTemplateTest_readWorld, sizeof(sithTemplateTest_readWorld));
    sithTemplateTest_readWorld.numThingTemplates = 4u;
    sithTemplateTest_file.size = validSize - 1u;
    sithTemplateTest_RewindFile();
    TEST_ASSERT_EQUAL_INT(1, sithTemplate_ReadThingTemplatesListBinary(SITHTEMPLATE_TEST_BINARY_HANDLE, &sithTemplateTest_readWorld));
    TEST_ASSERT_NOT_NULL(sithTemplateTest_readWorld.aThingTemplates);
}

TEST_GROUP_RUNNER(sithTemplate)
{
    RUN_TEST_CASE(sithTemplate, TestReadsTextTemplates);
    RUN_TEST_CASE(sithTemplate, TestTextControlFlow);
    RUN_TEST_CASE(sithTemplate, TestAllocatesAndFreesWorldTemplates);
    RUN_TEST_CASE(sithTemplate, TestBinaryRoundTrip);
    RUN_TEST_CASE(sithTemplate, TestEmptyBinaryRoundTrip);
    RUN_TEST_CASE(sithTemplate, TestBinaryFailures);
}
