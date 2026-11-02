#include <unity_fixture.h>

#include <stddef.h>

#include <sith/Cog/sithCog.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdUtil.h>

#define SITHCOG_TEST_MAXPRINTS    8u
#define SITHCOG_TEST_PRINTSIZE  256u
#define SITHCOG_TEST_NUMCOGS      2u

TEST_GROUP(sithCog);

#if defined(J3D_LOCAL_RTI)
// The tested copy of sithCog.c reaches std_g_genBuffer through the local RTI
// externs; std.c's fixed-address definition is not linked into sithTests, so
// this fixture owns the buffer the SITHCONSOLE_PRINTF path formats into.
char std_g_genBuffer[2048] = { 0 };
#endif

// Replacement for sithConsole_PrintString in the tested copy of sithCog.c,
// selected by the per-source COMPILE_DEFINITIONS in Tests/CMakeLists.txt.
void J3DAPI sithCogTest_PrintString(const char* pString);

static char sithCogTest_aPrints[SITHCOG_TEST_MAXPRINTS][SITHCOG_TEST_PRINTSIZE];
static size_t sithCogTest_numPrints;

static SithWorld sithCogTest_world;
static SithCog sithCogTest_aCogs[3];
static SithCogScript sithCogTest_aScripts[3];
static SithCogSymbolTable sithCogTest_aSymbolTables[3];
static const char sithCogTest_aCogNames[3][16]    = { "firstCog", "midCog", "ghostCog" };
static const char sithCogTest_aScriptNames[3][16] = { "first.cog", "mid.cog", "ghost.cog" };

void J3DAPI sithCogTest_PrintString(const char* pString)
{
    // Capture each full console string so cases can compare exact output.
    if ( pString && sithCogTest_numPrints < SITHCOG_TEST_MAXPRINTS )
    {
        STD_STRCPY(sithCogTest_aPrints[sithCogTest_numPrints], pString);
    }

    ++sithCogTest_numPrints;
}

TEST_SETUP(sithCog)
{
    // Build a two-cog world whose aCogs array retains one more fully populated
    // entry past numCogs; the original 'index > numCogs' bound let cogstatus
    // read that element at index == numCogs and print its status instead of
    // rejecting it.
    sithCogTest_numPrints = 0;
    STD_ZEROMEM(sithCogTest_aPrints, sizeof(sithCogTest_aPrints));
    STD_ZEROMEM(&sithCogTest_world, sizeof(sithCogTest_world));
    STD_ZEROMEM(sithCogTest_aCogs, sizeof(sithCogTest_aCogs));
    STD_ZEROMEM(sithCogTest_aScripts, sizeof(sithCogTest_aScripts));
    STD_ZEROMEM(sithCogTest_aSymbolTables, sizeof(sithCogTest_aSymbolTables));

    sithCogTest_world.numCogs  = SITHCOG_TEST_NUMCOGS;
    sithCogTest_world.sizeCogs = STD_ARRAYLEN(sithCogTest_aCogs);
    sithCogTest_world.aCogs    = sithCogTest_aCogs;

    for ( size_t i = 0; i < STD_ARRAYLEN(sithCogTest_aCogs); i++ )
    {
        SithCog* pCog = &sithCogTest_aCogs[i];
        pCog->idx          = (int)i;
        pCog->pScript      = &sithCogTest_aScripts[i];
        pCog->pSymbolTable = &sithCogTest_aSymbolTables[i];
        STD_STRCPY(pCog->aName, sithCogTest_aCogNames[i]);
        STD_STRCPY(pCog->pScript->aName, sithCogTest_aScriptNames[i]);
    }

    sithWorld_g_pCurrentWorld = &sithCogTest_world;
}

TEST_TEAR_DOWN(sithCog)
{
    // Restore the shared world global so every group initializes its own state.
    sithWorld_g_pCurrentWorld = NULL;
    sithCogTest_numPrints     = 0;
}

TEST(sithCog, TestRejectsIndexEqualToNumCogs)
{
    // Regression: the original bound tested index > numCogs, so index == numCogs
    // slipped through, read aCogs[2] one element past the world's two logical
    // cogs, and printed that fully populated element's status.
    TEST_ASSERT_EQUAL_INT(0, sithCog_CogStatus(NULL, "2"));
    TEST_ASSERT_EQUAL_size_t(1u, sithCogTest_numPrints);
    TEST_ASSERT_EQUAL_STRING("Error, bad parameters.\n", sithCogTest_aPrints[0]);
}

TEST(sithCog, TestRejectsIndexPastCogArray)
{
    // Index 3 exceeds both numCogs and sizeCogs; the command must report the
    // same parameter error without reading the array.
    TEST_ASSERT_EQUAL_INT(0, sithCog_CogStatus(NULL, "3"));
    TEST_ASSERT_EQUAL_size_t(1u, sithCogTest_numPrints);
    TEST_ASSERT_EQUAL_STRING("Error, bad parameters.\n", sithCogTest_aPrints[0]);
}

TEST(sithCog, TestPrintsCogStatusForValidIndex)
{
    // The last valid index (numCogs - 1) must keep printing its status line and
    // return 1; a boundary fix that over-rejects would fail here. The zero-symbol
    // table keeps the expected console output to exactly this line.
    TEST_ASSERT_EQUAL_INT(1, sithCog_CogStatus(NULL, "1"));
    TEST_ASSERT_EQUAL_size_t(1u, sithCogTest_numPrints);
    TEST_ASSERT_EQUAL_STRING("Cog #1: Name:midCog  Script mid.cog\n", sithCogTest_aPrints[0]);
}

TEST_GROUP_RUNNER(sithCog)
{
    RUN_TEST_CASE(sithCog, TestRejectsIndexEqualToNumCogs);
    RUN_TEST_CASE(sithCog, TestRejectsIndexPastCogArray);
    RUN_TEST_CASE(sithCog, TestPrintsCogStatusForValidIndex);
}
