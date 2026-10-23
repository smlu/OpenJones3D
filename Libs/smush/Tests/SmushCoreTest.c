#include <unity_fixture.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <smush/insane/Bitmap/Bitmap.h>
#include <smush/insane/Lec/LecShare.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Timer/Timer.h>
#include <smush/insane/Util/List.h>

// Unit tests of the smush Core area (LecShare, Timer, Memory, List, Bitmap). SmushPlay and Blit are not covered: they
// need the other areas' modules (and the assembly blitters).
TEST_GROUP(SmushCore);

TEST_SETUP(SmushCore)
{}

static int g_timerStarted;
static void* g_dbgBlocks[4];

static void SmushCoreTest_TrackBlock(void* pBlock)
{
    for ( size_t i = 0; i < sizeof(g_dbgBlocks) / sizeof(g_dbgBlocks[0]); ++i )
    {
        if ( !g_dbgBlocks[i] )
        {
            g_dbgBlocks[i] = pBlock;
            return;
        }
    }
}

static void SmushCoreTest_UntrackBlock(void* pBlock)
{
    for ( size_t i = 0; i < sizeof(g_dbgBlocks) / sizeof(g_dbgBlocks[0]); ++i )
    {
        if ( g_dbgBlocks[i] == pBlock )
        {
            g_dbgBlocks[i] = NULL;
        }
    }
}

TEST_TEAR_DOWN(SmushCore)
{
    // A failed assertion must not leave the timer thread or the registered modules running into the next test
    if ( g_timerStarted )
    {
        g_timerStarted = 0;
        LecShare_Shutdown(); // stops the timer thread (timer_Stop)
    }

    LecShare_UnregisterAll();

    // Debug allocator blocks of an aborted test
    for ( size_t i = 0; i < sizeof(g_dbgBlocks) / sizeof(g_dbgBlocks[0]); ++i )
    {
        if ( g_dbgBlocks[i] )
        {
            free((char*)g_dbgBlocks[i] - 4);
            g_dbgBlocks[i] = NULL;
        }
    }

    LecShare_ResetGlobals();
    Memory_ResetGlobals();
    Bitmap_ResetGlobals();
    Timer_ResetGlobals();
}

/* ---- Mock modules ---- */

typedef struct sSmushCoreTestModule
{
    LecModule module;
    const char* pModuleName;
    int startupResult;
    int nStartups;
    int nShutdowns;
    int nUnregisters;
    int order[8];
} SmushCoreTestModule;

static int g_startupSeq;
static int g_shutdownSeq;
static int g_unregisterSeq;

static SmushCoreTestModule g_modA, g_modB, g_modC;
static int g_svcAlpha, g_svcBeta;
static char g_lastError[256];

static void SmushCoreTest_ErrorFn(const char* pFormat, ...)
{
    // Only the format is recorded
    strncpy_s(g_lastError, sizeof(g_lastError), pFormat, _TRUNCATE);
}

static int SmushCoreTest_ModRegister(SmushCoreTestModule* pMod, LecShare* pShare)
{
    pShare->Publish(pMod->pModuleName, &pMod->module);
    return 0;
}

static int SmushCoreTest_RegA(LecShare* pShare, void* pParam)
{
    (void)pParam;
    return SmushCoreTest_ModRegister(&g_modA, pShare);
}

static int SmushCoreTest_RegB(LecShare* pShare, void* pParam)
{
    (void)pParam;
    return SmushCoreTest_ModRegister(&g_modB, pShare);
}

static int SmushCoreTest_RegC(LecShare* pShare, void* pParam)
{
    (void)pParam;
    return SmushCoreTest_ModRegister(&g_modC, pShare);
}

static int SmushCoreTest_RegServices(LecShare* pShare, void* pParam)
{
    (void)pParam;
    pShare->Publish("SVC=alpha", &g_svcAlpha);
    pShare->Publish("SVC=beta", &g_svcBeta);
    return 0;
}

#define SMUSHCORETEST_MODSLOT(name) \
    static int name##_Startup(void); \
    static void name##_Shutdown(void); \
    static void name##_Unregister(void)

SMUSHCORETEST_MODSLOT(A);
SMUSHCORETEST_MODSLOT(B);
SMUSHCORETEST_MODSLOT(C);

#define SMUSHCORETEST_MODIMPL(name, mod) \
    static int name##_Startup(void) { mod.order[0] = ++g_startupSeq; mod.nStartups++; return mod.startupResult; } \
    static void name##_Shutdown(void) { mod.order[1] = ++g_shutdownSeq; mod.nShutdowns++; } \
    static void name##_Unregister(void) { mod.order[2] = ++g_unregisterSeq; mod.nUnregisters++; }

SMUSHCORETEST_MODIMPL(A, g_modA)
SMUSHCORETEST_MODIMPL(B, g_modB)
SMUSHCORETEST_MODIMPL(C, g_modC)

static void SmushCoreTest_InitModules(void)
{
    memset(&g_modA, 0, sizeof(g_modA));
    memset(&g_modB, 0, sizeof(g_modB));
    memset(&g_modC, 0, sizeof(g_modC));
    g_startupSeq = g_shutdownSeq = g_unregisterSeq = 0;

    g_modA.module = (LecModule){ SmushCoreTest_RegA, A_Unregister, A_Startup, A_Shutdown };
    g_modA.pModuleName = "LECMODULE=a";
    g_modB.module = (LecModule){ SmushCoreTest_RegB, B_Unregister, B_Startup, B_Shutdown };
    g_modB.pModuleName = "LECMODULE=b";
    g_modC.module = (LecModule){ SmushCoreTest_RegC, C_Unregister, C_Startup, C_Shutdown };
    g_modC.pModuleName = "LECMODULE=c";
}

/* ---- LecShare: name helpers ---- */

TEST(SmushCore, LecShare_NameLen)
{
    TEST_ASSERT_EQUAL_INT(5, LecShare_NameLen("TIMER=insane"));
    TEST_ASSERT_EQUAL_INT(6, LecShare_NameLen("MEMORY.workspace"));
    TEST_ASSERT_EQUAL_INT(3, LecShare_NameLen("ABC"));
    TEST_ASSERT_EQUAL_INT(3, LecShare_NameLen("ABC  =x")); // trailing separators before '=' are not part of the name
    TEST_ASSERT_EQUAL_INT(3, LecShare_NameLen("ABC,;=x"));
    TEST_ASSERT_EQUAL_INT(0, LecShare_NameLen("=x"));
    TEST_ASSERT_EQUAL_INT(0, LecShare_NameLen(""));
    TEST_ASSERT_EQUAL_INT(9, LecShare_NameLen("LECMODULE=timer"));
}

TEST(SmushCore, LecShare_NameCmp)
{
    TEST_ASSERT_EQUAL_INT(0, LecShare_NameCmp("Timer=Insane", "TIMER=insane"));
    TEST_ASSERT_TRUE(LecShare_NameCmp("A", "B") < 0);
    TEST_ASSERT_TRUE(LecShare_NameCmp("B", "A") > 0);
    TEST_ASSERT_TRUE(LecShare_NameCmp("ab", "a") > 0);
    TEST_ASSERT_TRUE(LecShare_NameCmp("a", "ab") < 0);
    TEST_ASSERT_EQUAL_INT('B' - 'A', LecShare_NameCmp("b", "a"));

    // Two empty strings compare as -1 (the result is only updated once a character was compared)
    TEST_ASSERT_EQUAL_INT(-1, LecShare_NameCmp("", ""));
}

TEST(SmushCore, LecShare_NameNCmp)
{
    TEST_ASSERT_EQUAL_INT(0, LecShare_NameNCmp("LECMODULE", "lecmodule=timer", 9));
    TEST_ASSERT_TRUE(LecShare_NameNCmp("LECMODULE", "LECMODULX", 9) < 0);
    TEST_ASSERT_EQUAL_INT(0, LecShare_NameNCmp("ABCX", "ABCY", 3));
    TEST_ASSERT_EQUAL_INT(-1, LecShare_NameNCmp("A", "A", 0)); // nothing compared
    TEST_ASSERT_EQUAL_INT(0, LecShare_NameNCmp("A", "A", 5));  // stops at the double terminator
}

TEST(SmushCore, LecShare_Strip)
{
    char buf[32];

    strcpy_s(buf, sizeof(buf), "  A B\tC\r\n ");
    LecShare_Strip(buf);
    TEST_ASSERT_EQUAL_STRING("ABC", buf);

    strcpy_s(buf, sizeof(buf), "NOCHANGE=1");
    LecShare_Strip(buf);
    TEST_ASSERT_EQUAL_STRING("NOCHANGE=1", buf);

    strcpy_s(buf, sizeof(buf), " \t ");
    LecShare_Strip(buf);
    TEST_ASSERT_EQUAL_STRING("", buf);

    // Characters above 0x7F are removed as well
    strcpy_s(buf, sizeof(buf), "A\x80" "B\xFF" "C");
    LecShare_Strip(buf);
    TEST_ASSERT_EQUAL_STRING("ABC", buf);
}

TEST(SmushCore, LecShare_Strip_ReadOnlyUnchangedLiteral)
{
    // A name that doesn't need stripping must not be written to (it can be a string literal in read-only memory)
    const char* pName = "READONLY=literal";
    LecShare_Strip((char*)pName);
    TEST_ASSERT_EQUAL_STRING("READONLY=literal", pName);
}

static const char* SmushCoreTest_Match(const char* pName, const char* pList)
{
    return (const char*)(intptr_t)LecShare_Match(pName, pList);
}

TEST(SmushCore, LecShare_Match)
{
    TEST_ASSERT_EQUAL_STRING("insane_x", SmushCoreTest_Match("TIMER", "MEMORY=insane TIMER=insane_x"));
    TEST_ASSERT_EQUAL_STRING("insane", SmushCoreTest_Match("MEMORY", "MEMORY=insane TIMER=insane_x"));
    TEST_ASSERT_EQUAL_STRING("b", SmushCoreTest_Match("TIMER", "MEMORY=a,TIMER=b"));
    TEST_ASSERT_EQUAL_STRING("b", SmushCoreTest_Match("timer", "MEMORY=a;TIMER=b"));
    TEST_ASSERT_EQUAL_STRING("b", SmushCoreTest_Match("TIMER", "  TIMER = b  "));
    TEST_ASSERT_NULL(SmushCoreTest_Match("VIDEO", "MEMORY=insane TIMER=insane_x"));
    TEST_ASSERT_NULL(SmushCoreTest_Match("TIMER", ""));

    // A "NAME.role" request matches the "NAME=provider" entry
    TEST_ASSERT_EQUAL_STRING("insane_debug", SmushCoreTest_Match("MEMORY.workspace", "MEMORY=insane_debug"));

    // A more specific "NAME.role=provider" entry matches the role only
    TEST_ASSERT_EQUAL_STRING("special", SmushCoreTest_Match("MEMORY.workspace", "MEMORY=plain MEMORY.workspace=special"));
    TEST_ASSERT_EQUAL_STRING("plain", SmushCoreTest_Match("MEMORY.app", "MEMORY=plain MEMORY.workspace=special"));

    // The first matching entry wins
    TEST_ASSERT_EQUAL_STRING("one", SmushCoreTest_Match("SVC", "SVC=one SVC=two"));
}

/* ---- LecShare: registry ---- */

TEST(SmushCore, LecShare_Init_RegistersModules)
{
    SmushCoreTest_InitModules();
    LecModuleEntry modules[] = { { SmushCoreTest_RegA, NULL }, { SmushCoreTest_RegServices, NULL }, { NULL, NULL } };
    LecShare* pShare = LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");
    TEST_ASSERT_NOT_NULL(pShare);
    TEST_ASSERT_TRUE(pShare->Publish == LecShare_Publish);
    TEST_ASSERT_TRUE(pShare->Subscribe == (void* (*)(const char*, void*))LecShare_Subscribe);
    TEST_ASSERT_TRUE(pShare->ReportError == SmushCoreTest_ErrorFn);
    TEST_ASSERT_EQUAL_INT(3, LecShare_Count(NULL));
    TEST_ASSERT_EQUAL_INT(1, LecShare_Count("LECMODULE"));
    TEST_ASSERT_EQUAL_INT(2, LecShare_Count("SVC"));
    TEST_ASSERT_EQUAL_INT(0, LecShare_Count("NOPE"));
}

TEST(SmushCore, LecShare_Subscribe)
{
    SmushCoreTest_InitModules();
    LecModuleEntry modules[] = { { SmushCoreTest_RegServices, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");

    // No provider selected: the first published one
    TEST_ASSERT_TRUE(LecShare_Subscribe("SVC", NULL) == &g_svcAlpha);
    TEST_ASSERT_TRUE(LecShare_Subscribe("SVC.role", NULL) == &g_svcAlpha);

    // The preferred list is searched next
    TEST_ASSERT_TRUE(LecShare_Subscribe("SVC", "SVC=beta") == &g_svcBeta);
    TEST_ASSERT_TRUE(LecShare_Subscribe("SVC", "SVC=ALPHA") == &g_svcAlpha); // names are case insensitive
    TEST_ASSERT_NULL(LecShare_Subscribe("SVC", "SVC=gamma"));
    TEST_ASSERT_NULL(LecShare_Subscribe("NOPE", NULL));

    // Each successful subscription is counted (Unpublish returns the count)
    TEST_ASSERT_EQUAL_INT(3, LecShare_Unpublish("SVC=alpha"));
    TEST_ASSERT_EQUAL_INT(1, LecShare_Unpublish("SVC=beta"));
    TEST_ASSERT_EQUAL_INT(1, LecShare_Unpublish("SVC=beta")); // not published anymore
}

TEST(SmushCore, LecShare_Subscribe_DefaultLists)
{
    SmushCoreTest_InitModules();
    LecModuleEntry modules[] = { { SmushCoreTest_RegServices, NULL }, { NULL, NULL } };

    // The second list (pB) overrides the preferred one, the first (pA) is the default
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "SVC=beta", "");
    TEST_ASSERT_TRUE(LecShare_Subscribe("SVC", NULL) == &g_svcBeta);
    TEST_ASSERT_TRUE(LecShare_Subscribe("SVC", "SVC=alpha") == &g_svcAlpha);

    LecShare_Init(SmushCoreTest_ErrorFn, modules, "SVC=beta", "SVC=alpha");
    TEST_ASSERT_TRUE(LecShare_Subscribe("SVC", NULL) == &g_svcAlpha);
    TEST_ASSERT_TRUE(LecShare_Subscribe("SVC", "SVC=beta") == &g_svcAlpha);
}

static void* g_regTimeSubscribe;

static int SmushCoreTest_RegSubscribeEarly(LecShare* pShare, void* pParam)
{
    (void)pParam;
    pShare->Publish("EARLY=x", &g_svcAlpha);
    g_regTimeSubscribe = pShare->Subscribe("EARLY", NULL);
    return 0;
}

TEST(SmushCore, LecShare_Subscribe_NotBeforeInit)
{
    // Subscribing is not possible while the modules are registered
    g_regTimeSubscribe = &g_svcBeta;
    LecModuleEntry modules[] = { { SmushCoreTest_RegSubscribeEarly, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");
    TEST_ASSERT_NULL(g_regTimeSubscribe);
    TEST_ASSERT_TRUE(LecShare_Subscribe("EARLY", NULL) == &g_svcAlpha);
}

TEST(SmushCore, LecShare_Unsubscribe)
{
    SmushCoreTest_InitModules();
    LecModuleEntry modules[] = { { SmushCoreTest_RegServices, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");

    TEST_ASSERT_TRUE(LecShare_Subscribe("SVC", NULL) == &g_svcAlpha);
    TEST_ASSERT_TRUE(LecShare_Subscribe("SVC", NULL) == &g_svcAlpha);
    TEST_ASSERT_EQUAL_INT(0, LecShare_Unsubscribe(&g_svcAlpha));
    TEST_ASSERT_EQUAL_INT(0, LecShare_Unsubscribe(&g_svcAlpha));
    TEST_ASSERT_EQUAL_INT(1, LecShare_Unsubscribe(&g_modA)); // unknown interface
    TEST_ASSERT_EQUAL_INT(0, LecShare_Unpublish("SVC=alpha"));
}

TEST(SmushCore, LecShare_Publish_Unpublish)
{
    LecShare* pShare = LecShare_Init(SmushCoreTest_ErrorFn, NULL, "", "");
    TEST_ASSERT_EQUAL_INT(0, LecShare_Count(NULL));

    TEST_ASSERT_EQUAL_INT(0, pShare->Publish("ONE=a", &g_svcAlpha));
    TEST_ASSERT_EQUAL_INT(0, pShare->Publish("TWO=b", &g_svcBeta));
    TEST_ASSERT_EQUAL_INT(2, LecShare_Count(NULL));

    // Unpublish matches the name case insensitively and ignores the whitespace in it
    char name[] = " one = a ";
    TEST_ASSERT_EQUAL_INT(0, LecShare_Unpublish(name));
    TEST_ASSERT_EQUAL_STRING("one=a", name); // the caller's string is stripped in place
    TEST_ASSERT_EQUAL_INT(1, LecShare_Count(NULL));
    TEST_ASSERT_EQUAL_INT(1, LecShare_Unpublish("ONE=a"));

    // The free slot is reused
    TEST_ASSERT_EQUAL_INT(0, pShare->Publish("THREE=c", &g_modA));
    TEST_ASSERT_EQUAL_INT(2, LecShare_Count(NULL));
    TEST_ASSERT_EQUAL_INT(1, LecShare_Count("THREE"));
    TEST_ASSERT_EQUAL_INT(0, LecShare_Unpublish("TWO=b"));
    TEST_ASSERT_EQUAL_INT(0, LecShare_Unpublish("THREE=c"));
    TEST_ASSERT_EQUAL_INT(0, LecShare_Count(NULL));
}

TEST(SmushCore, LecShare_Publish_TableFull)
{
    static char names[LECSHARE_MAXENTRIES + 1][12];

    LecShare* pShare = LecShare_Init(SmushCoreTest_ErrorFn, NULL, "", "");
    for ( int i = 0; i < LECSHARE_MAXENTRIES; ++i )
    {
        sprintf_s(names[i], sizeof(names[i]), "N%d", i);
        TEST_ASSERT_EQUAL_INT(0, pShare->Publish(names[i], &g_svcAlpha));
    }

    sprintf_s(names[LECSHARE_MAXENTRIES], sizeof(names[0]), "FULL");
    TEST_ASSERT_EQUAL_INT(1, pShare->Publish(names[LECSHARE_MAXENTRIES], &g_svcAlpha));
    TEST_ASSERT_EQUAL_INT(LECSHARE_MAXENTRIES, LecShare_Count(NULL));
    TEST_ASSERT_EQUAL_INT(1, LecShare_Count("N511"));
}

TEST(SmushCore, LecShare_Enumerate)
{
    SmushCoreTest_InitModules();
    LecModuleEntry modules[] = { { SmushCoreTest_RegA, NULL }, { SmushCoreTest_RegServices, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");

    TEST_ASSERT_EQUAL_STRING("SVC=alpha", (const char*)LecShare_Enumerate("SVC", 0));
    TEST_ASSERT_EQUAL_STRING("SVC=beta", (const char*)LecShare_Enumerate("SVC", 1));
    TEST_ASSERT_EQUAL_STRING("SVC=alpha", (const char*)LecShare_Enumerate("SVC", 0)); // backwards from the cached position
    TEST_ASSERT_EQUAL_STRING("SVC=alpha", (const char*)LecShare_Enumerate("SVC", 0)); // same index
    TEST_ASSERT_NULL(LecShare_Enumerate("SVC", 2));
    TEST_ASSERT_EQUAL_STRING("SVC=beta", (const char*)LecShare_Enumerate("svc", 1));
    TEST_ASSERT_NULL(LecShare_Enumerate("SVC", -1));
    TEST_ASSERT_NULL(LecShare_Enumerate("NOPE", 0));

    // No name: every entry
    TEST_ASSERT_EQUAL_STRING("LECMODULE=a", (const char*)LecShare_Enumerate(NULL, 0));
    TEST_ASSERT_EQUAL_STRING("SVC=beta", (const char*)LecShare_Enumerate(NULL, 2));
    TEST_ASSERT_EQUAL_STRING("SVC=alpha", (const char*)LecShare_Enumerate(NULL, 1));
    TEST_ASSERT_NULL(LecShare_Enumerate(NULL, 3));
}

TEST(SmushCore, LecShare_Find)
{
    SmushCoreTest_InitModules();
    LecModuleEntry modules[] = { { SmushCoreTest_RegServices, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");

    LecEntry* pEntry = LecShare_Find("svc=BETA");
    TEST_ASSERT_NOT_NULL(pEntry);
    TEST_ASSERT_TRUE(pEntry->pInterface == &g_svcBeta);
    TEST_ASSERT_EQUAL_INT(0, pEntry->refs);
    TEST_ASSERT_NULL(LecShare_Find("SVC=gamma"));
}

TEST(SmushCore, LecShare_BatchSubscribe)
{
    SmushCoreTest_InitModules();
    LecModuleEntry modules[] = { { SmushCoreTest_RegServices, NULL }, { SmushCoreTest_RegA, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "SVC=alpha", "");

    void* pOutA = &g_modB; // overwritten
    void* pOutB = NULL;
    void* pOutC = NULL;
    LecBatch table[] = {
        { &pOutA, "SVC.app", NULL },
        { &pOutB, "SVC.app", "SVC=beta" },
        { &pOutC, "LECMODULE", NULL },
        { NULL, NULL, NULL }
    };

    // The default list is used if there is no preferred one
    TEST_ASSERT_EQUAL_INT(0, LecShare_BatchSubscribe(table, "SVC=beta"));
    TEST_ASSERT_TRUE(pOutA == &g_svcBeta);
    TEST_ASSERT_TRUE(pOutB == &g_svcBeta);
    TEST_ASSERT_TRUE(pOutC == &g_modA.module);

    TEST_ASSERT_EQUAL_INT(0, LecShare_BatchUnsubscribe(table));
    TEST_ASSERT_NULL(pOutA);
    TEST_ASSERT_NULL(pOutB);
    TEST_ASSERT_NULL(pOutC);

    // Without the default
    TEST_ASSERT_EQUAL_INT(0, LecShare_BatchSubscribe(table, NULL));
    TEST_ASSERT_TRUE(pOutA == &g_svcAlpha);
    TEST_ASSERT_TRUE(pOutB == &g_svcBeta);

    // A table without entries
    LecBatch empty[] = { { NULL, NULL, NULL } };
    TEST_ASSERT_EQUAL_INT(0, LecShare_BatchSubscribe(empty, NULL));
    TEST_ASSERT_EQUAL_INT(1, LecShare_BatchUnsubscribe(empty));
}

TEST(SmushCore, LecShare_BatchSubscribe_Failure)
{
    SmushCoreTest_InitModules();
    LecModuleEntry modules[] = { { SmushCoreTest_RegServices, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");

    void* pOutA = NULL;
    void* pOutB = &g_modA;
    void* pOutC = &g_modA;
    LecBatch table[] = {
        { &pOutA, "SVC", NULL },
        { &pOutB, "MISSING", NULL },
        { &pOutC, "SVC", NULL },
        { NULL, NULL, NULL }
    };

    g_lastError[0] = 0;
    TEST_ASSERT_EQUAL_INT(1, LecShare_BatchSubscribe(table, NULL));
    TEST_ASSERT_EQUAL_STRING("LecShare:BatchSubscribe Couldn't subscribe to %s interface.", g_lastError);
    TEST_ASSERT_TRUE(pOutA == &g_svcAlpha);
    TEST_ASSERT_NULL(pOutB);
    TEST_ASSERT_TRUE(pOutC == &g_modA); // not reached

    // Unsubscribing an entry that wasn't subscribed fails (the batch is walked backwards)
    g_lastError[0] = 0;
    pOutB = NULL;
    TEST_ASSERT_EQUAL_INT(1, LecShare_BatchUnsubscribe(table));
    TEST_ASSERT_EQUAL_STRING("LecShare:BatchUnsubscribe Couldn't unsubscribe from %s interface.", g_lastError);
}

TEST(SmushCore, LecShare_Startup_Shutdown_Order)
{
    SmushCoreTest_InitModules();
    LecModuleEntry modules[] = { { SmushCoreTest_RegA, NULL }, { SmushCoreTest_RegB, NULL }, { SmushCoreTest_RegC, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");

    TEST_ASSERT_EQUAL_INT(1, LecShare_Shutdown()); // nothing was started

    TEST_ASSERT_EQUAL_INT(0, LecShare_Startup());
    TEST_ASSERT_EQUAL_INT(1, g_modA.order[0]);
    TEST_ASSERT_EQUAL_INT(2, g_modB.order[0]);
    TEST_ASSERT_EQUAL_INT(3, g_modC.order[0]);

    TEST_ASSERT_EQUAL_INT(0, LecShare_Shutdown());
    TEST_ASSERT_EQUAL_INT(1, g_modC.order[1]);
    TEST_ASSERT_EQUAL_INT(2, g_modB.order[1]);
    TEST_ASSERT_EQUAL_INT(3, g_modA.order[1]);

    TEST_ASSERT_EQUAL_INT(0, LecShare_UnregisterAll());
    TEST_ASSERT_EQUAL_INT(1, g_modC.order[2]);
    TEST_ASSERT_EQUAL_INT(2, g_modB.order[2]);
    TEST_ASSERT_EQUAL_INT(3, g_modA.order[2]);
}

TEST(SmushCore, LecShare_Startup_Failure)
{
    SmushCoreTest_InitModules();
    LecModuleEntry modules[] = { { SmushCoreTest_RegA, NULL }, { SmushCoreTest_RegB, NULL }, { SmushCoreTest_RegC, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");

    g_modB.startupResult = 7;
    g_lastError[0] = 0;
    TEST_ASSERT_EQUAL_INT(7, LecShare_Startup());
    TEST_ASSERT_EQUAL_STRING("LecShare_Startup: Failed with module %s.", g_lastError);

    TEST_ASSERT_EQUAL_INT(1, g_modA.nStartups);
    TEST_ASSERT_EQUAL_INT(1, g_modB.nStartups);
    TEST_ASSERT_EQUAL_INT(0, g_modC.nStartups);

    // Only the modules started before the failing one are shut down
    TEST_ASSERT_EQUAL_INT(1, g_modA.nShutdowns);
    TEST_ASSERT_EQUAL_INT(0, g_modB.nShutdowns);
    TEST_ASSERT_EQUAL_INT(0, g_modC.nShutdowns);
}

TEST(SmushCore, LecShare_Reset)
{
    LecShare_Init(SmushCoreTest_ErrorFn, NULL, "", "");
    LecShare_Publish("ONE=a", &g_svcAlpha);
    TEST_ASSERT_EQUAL_INT(1, LecShare_Count(NULL));
    LecShare_Reset("SVC=a", "SVC=b");
    TEST_ASSERT_EQUAL_INT(0, LecShare_Count(NULL));
    TEST_ASSERT_NULL(LecShare_Subscribe("ONE", NULL)); // not initialized anymore
}

TEST(SmushCore, lec_Log_Wraps)
{
    for ( int i = 0; i < 45; ++i )
    {
        lec_Log("message %d: %s", i, "text");
    }
}

/* ---- List ---- */

TEST(SmushCore, LecList_Init)
{
    LecList list;
    memset(&list, 0xCD, sizeof(list));
    LecList_Init(&list);
    TEST_ASSERT_NULL(list.head.pPrev);
    TEST_ASSERT_TRUE(list.head.pNext == &list.tail);
    TEST_ASSERT_TRUE(list.tail.pPrev == &list.head);
    TEST_ASSERT_NULL(list.tail.pNext);
}

TEST(SmushCore, LecList_Insert_Remove)
{
    LecList list;
    LecNode a, b, c;
    LecList_Init(&list);

    LecList_Insert(&a, &list.head, 0); // after the head
    TEST_ASSERT_TRUE(list.head.pNext == &a);
    TEST_ASSERT_TRUE(a.pPrev == &list.head);
    TEST_ASSERT_TRUE(a.pNext == &list.tail);
    TEST_ASSERT_TRUE(list.tail.pPrev == &a);

    LecList_Insert(&b, &list.tail, 1); // before the tail
    TEST_ASSERT_TRUE(a.pNext == &b);
    TEST_ASSERT_TRUE(b.pPrev == &a);
    TEST_ASSERT_TRUE(b.pNext == &list.tail);
    TEST_ASSERT_TRUE(list.tail.pPrev == &b);

    LecList_Insert(&c, &b, 1); // before b
    TEST_ASSERT_TRUE(a.pNext == &c);
    TEST_ASSERT_TRUE(c.pPrev == &a);
    TEST_ASSERT_TRUE(c.pNext == &b);
    TEST_ASSERT_TRUE(b.pPrev == &c);

    LecList_Remove(&c);
    TEST_ASSERT_TRUE(a.pNext == &b);
    TEST_ASSERT_TRUE(b.pPrev == &a);
    TEST_ASSERT_TRUE(c.pPrev == &a); // the node's own links are left as they were
    TEST_ASSERT_TRUE(c.pNext == &b);

    LecList_Remove(&a);
    LecList_Remove(&b);
    TEST_ASSERT_TRUE(list.head.pNext == &list.tail);
    TEST_ASSERT_TRUE(list.tail.pPrev == &list.head);
}

TEST(SmushCore, LecList_Insert_AtEnds)
{
    // The sentinels' missing neighbors are not written
    LecList list;
    LecNode a, b;
    LecList_Init(&list);

    LecList_Insert(&a, &list.head, 1); // before the head
    TEST_ASSERT_TRUE(a.pNext == &list.head);
    TEST_ASSERT_NULL(a.pPrev);
    TEST_ASSERT_TRUE(list.head.pPrev == &a);

    LecList_Insert(&b, &list.tail, 0); // after the tail
    TEST_ASSERT_TRUE(b.pPrev == &list.tail);
    TEST_ASSERT_NULL(b.pNext);
    TEST_ASSERT_TRUE(list.tail.pNext == &b);

    LecList_Remove(&a); // a node without previous
    TEST_ASSERT_NULL(list.head.pPrev);
    LecList_Remove(&b); // a node without next
    TEST_ASSERT_NULL(list.tail.pNext);
}

/* ---- Memory ---- */

static MemoryDebugInterface* SmushCoreTest_InitDebugMemory(void)
{
    LecModuleEntry modules[] = { { memory_Register, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "MEMORY=insane_debug", "");
    return LecShare_Subscribe("MEMORY.workspace", NULL);
}

TEST(SmushCore, Memory_Register_PublishesServices)
{
    LecModuleEntry modules[] = { { memory_Register, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");

    TEST_ASSERT_EQUAL_INT(1, LecShare_Count("LECMODULE"));
    TEST_ASSERT_EQUAL_INT(2, LecShare_Count("MEMORY"));

    MemoryInterface* pPlain = LecShare_Subscribe("MEMORY", "MEMORY=insane");
    TEST_ASSERT_NOT_NULL(pPlain);
    void* p = pPlain->Alloc(16);
    TEST_ASSERT_NOT_NULL(p);
    memset(p, 0xAB, 16);
    p = pPlain->Realloc(p, 64);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT8(0xAB, ((uint8_t*)p)[15]);
    pPlain->Free(p);

    TEST_ASSERT_NOT_NULL(LecShare_Subscribe("MEMORY", "MEMORY=insane_debug"));
    LecShare_UnregisterAll();
    TEST_ASSERT_EQUAL_INT(0, LecShare_Count("MEMORY"));
    TEST_ASSERT_EQUAL_INT(0, LecShare_Count("LECMODULE"));
}

TEST(SmushCore, Memory_Debug_AllocFree)
{
    MemoryDebugInterface* pMem = SmushCoreTest_InitDebugMemory();
    TEST_ASSERT_NOT_NULL(pMem);
    TEST_ASSERT_TRUE(pMem->Alloc == memory_debugAlloc);

    uint8_t* p = pMem->Alloc(100, "file.c", 12);
    TEST_ASSERT_NOT_NULL(p);
    SmushCoreTest_TrackBlock(p);

    // The block starts with its size and ends with a guard word pointing to the block
    char* pRaw = (char*)p - 4;
    TEST_ASSERT_EQUAL_INT(100, *(int*)pRaw);
    TEST_ASSERT_TRUE(*(char**)(pRaw + 100 + 4) == pRaw);

    memset(p, 0x5A, 100);
    TEST_ASSERT_EQUAL_INT(0, memory_Check(pRaw, 100, "file.c", 13, "Check")); // reference dropped to 0
    TEST_ASSERT_EQUAL_INT(-1, memory_Check(pRaw, 100, "file.c", 14, "Check")); // freed twice
    TEST_ASSERT_EQUAL_INT(-2, memory_Check(pRaw + 4, 100 - 4, "file.c", 15, "Check")); // unknown block (the guard is read at +100)

    memory_Track(pRaw, 100, "file.c", 16, "Track"); // reference count -1 -> 0
    memory_Track(pRaw, 100, "file.c", 16, "Track"); // 1
    pMem->Free(p, "file.c", 17);
    SmushCoreTest_UntrackBlock(p);
    LecShare_UnregisterAll();
}

TEST(SmushCore, Memory_Debug_Realloc)
{
    MemoryDebugInterface* pMem = SmushCoreTest_InitDebugMemory();
    TEST_ASSERT_NOT_NULL(pMem);

    uint8_t* p = pMem->Alloc(10, "file.c", 1);
    TEST_ASSERT_NOT_NULL(p);
    SmushCoreTest_TrackBlock(p);
    for ( int i = 0; i < 10; ++i )
    {
        p[i] = (uint8_t)(i + 1);
    }

    uint8_t* p2 = pMem->Realloc(p, 300, "file.c", 2);
    TEST_ASSERT_NOT_NULL(p2);
    SmushCoreTest_UntrackBlock(p);
    SmushCoreTest_TrackBlock(p2);
    for ( int i = 0; i < 10; ++i )
    {
        TEST_ASSERT_EQUAL_UINT8(i + 1, p2[i]);
    }

    char* pRaw = (char*)p2 - 4;
    TEST_ASSERT_EQUAL_INT(300, *(int*)pRaw);
    TEST_ASSERT_TRUE(*(char**)(pRaw + 300 + 4) == pRaw);

    memset(p2, 0, 300);
    pMem->Free(p2, "file.c", 3);
    SmushCoreTest_UntrackBlock(p2);
    LecShare_UnregisterAll();
}

TEST(SmushCore, Memory_Debug_OverwrittenGuardStillFreed)
{
    MemoryDebugInterface* pMem = SmushCoreTest_InitDebugMemory();
    uint8_t* p = pMem->Alloc(8, "file.c", 1);
    TEST_ASSERT_NOT_NULL(p);
    SmushCoreTest_TrackBlock(p);
    p[8] = 0xFF; // overwrite the guard word
    pMem->Free(p, "file.c", 2);
    SmushCoreTest_UntrackBlock(p);
    LecShare_UnregisterAll();
}

/* ---- Bitmap ---- */

typedef struct sSmushCoreTestAlloc
{
    int nAllocs;
    int nFrees;
    int lastSize;
    void* pLast;
    int failNext;
} SmushCoreTestAlloc;

static SmushCoreTestAlloc g_alloc;

static void* SmushCoreTest_MockAlloc(int size, const char* pFile, int line)
{
    (void)pFile;
    (void)line;
    g_alloc.nAllocs++;
    g_alloc.lastSize = size;
    if ( g_alloc.failNext )
    {
        g_alloc.failNext = 0;
        return NULL;
    }

    g_alloc.pLast = calloc(1, (size_t)size);
    return g_alloc.pLast;
}

static void SmushCoreTest_MockFree(void* pBlock, const char* pFile, int line)
{
    (void)pFile;
    (void)line;
    g_alloc.nFrees++;
    free(pBlock);
}

static void* SmushCoreTest_MockRealloc(void* pBlock, int size, const char* pFile, int line)
{
    (void)pFile;
    (void)line;
    return realloc(pBlock, (size_t)size);
}

static MemoryDebugInterface g_mockMemory = { SmushCoreTest_MockAlloc, SmushCoreTest_MockFree, SmushCoreTest_MockRealloc };

static int SmushCoreTest_RegMockMemory(LecShare* pShare, void* pParam)
{
    (void)pParam;
    pShare->Publish("MEMORY=mock", &g_mockMemory);
    return 0;
}

static BitmapInterface* SmushCoreTest_InitBitmap(void)
{
    memset(&g_alloc, 0, sizeof(g_alloc));
    LecModuleEntry modules[] = { { SmushCoreTest_RegMockMemory, NULL }, { bitmap_Register, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "MEMORY=mock", "");
    TEST_ASSERT_EQUAL_INT(0, LecShare_Startup());
    return LecShare_Subscribe("BITMAP", NULL);
}

static void SmushCoreTest_ShutdownBitmap(void)
{
    LecShare_Shutdown();
    LecShare_UnregisterAll();
}

TEST(SmushCore, Bitmap_Startup_NoMemory)
{
    LecModuleEntry modules[] = { { bitmap_Register, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");

    g_lastError[0] = 0;
    TEST_ASSERT_EQUAL_INT(-1, LecShare_Startup());
    TEST_ASSERT_EQUAL_STRING("LecShare_Startup: Failed with module %s.", g_lastError);
}

TEST(SmushCore, Bitmap_Create_Rgb565)
{
    BitmapInterface* pBitmaps = SmushCoreTest_InitBitmap();
    TEST_ASSERT_NOT_NULL(pBitmaps);
    TEST_ASSERT_TRUE(pBitmaps->Create == bitmap_Create);

    LecBitmapHeader* pBitmap = NULL;
    TEST_ASSERT_EQUAL_INT(0, pBitmaps->Create(&pBitmap, 5, 3, 3));
    TEST_ASSERT_NOT_NULL(pBitmap);
    TEST_ASSERT_EQUAL_INT(5 * 3 * 2 + 0x5C, g_alloc.lastSize);

    // The header is at +8 of the block, the pixels follow at +0x5C
    TEST_ASSERT_TRUE((char*)pBitmap == (char*)g_alloc.pLast + 8);
    TEST_ASSERT_TRUE(pBitmap->pPixels == (char*)g_alloc.pLast + 0x5C);
    TEST_ASSERT_EQUAL_INT(5, pBitmap->width);
    TEST_ASSERT_EQUAL_INT(3, pBitmap->height);
    TEST_ASSERT_EQUAL_INT(2, pBitmap->pixelSize);
    TEST_ASSERT_EQUAL_INT(10, pBitmap->pitch);
    TEST_ASSERT_EQUAL_INT(16, pBitmap->bpp);
    TEST_ASSERT_EQUAL_INT(1, pBitmap->colorMode);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->unknown6);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->unknown7);
    TEST_ASSERT_EQUAL_INT(5, pBitmap->format.redBPP);
    TEST_ASSERT_EQUAL_INT(6, pBitmap->format.greenBPP);
    TEST_ASSERT_EQUAL_INT(5, pBitmap->format.blueBPP);
    TEST_ASSERT_EQUAL_INT(11, pBitmap->format.redPosShift);
    TEST_ASSERT_EQUAL_INT(5, pBitmap->format.greenPosShift);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->format.bluePosShift);
    TEST_ASSERT_EQUAL_INT(3, pBitmap->format.redPosShiftRight);
    TEST_ASSERT_EQUAL_INT(2, pBitmap->format.greenPosShiftRight);
    TEST_ASSERT_EQUAL_INT(3, pBitmap->format.bluePosShiftRight);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->format.alphaBPP);

    pBitmaps->Destroy(&pBitmap);
    TEST_ASSERT_NULL(pBitmap);
    TEST_ASSERT_EQUAL_INT(1, g_alloc.nFrees);
    SmushCoreTest_ShutdownBitmap();
}

typedef struct sSmushCoreTestBitmapType
{
    int type;
    int pixelSize;
    int bpp;
    int colorMode;
    int bpps[4];       // red, green, blue, alpha
    int shifts[4];
    int shiftsRight[4];
} SmushCoreTestBitmapType;

TEST(SmushCore, Bitmap_Create_Types)
{
    static const SmushCoreTestBitmapType aTypes[] = {
        // type, pixelSize, bpp, colorMode, {R,G,B,A bpp}, {R,G,B,A shift}, {R,G,B,A shift right}
        { 5,  2, 16, 2, { 5, 5, 5, 1 }, { 10, 5, 0, 15 }, { 3, 3, 3, 7 } },
        { 10, 2, 16, 2, { 5, 5, 5, 1 }, { 0, 5, 10, 15 }, { 3, 3, 3, 7 } },
        { 2,  2, 15, 1, { 5, 5, 5, 0 }, { 10, 5, 0, 0 }, { 3, 3, 3, 0 } },
        { 7,  2, 15, 1, { 5, 5, 5, 0 }, { 0, 5, 10, 0 }, { 3, 3, 3, 0 } },
        { 3,  2, 16, 1, { 5, 6, 5, 0 }, { 11, 5, 0, 0 }, { 3, 2, 3, 0 } },
        { 8,  2, 16, 1, { 5, 6, 5, 0 }, { 0, 5, 11, 0 }, { 3, 2, 3, 0 } },
        { 4,  3, 24, 1, { 8, 8, 8, 0 }, { 16, 8, 0, 0 }, { 0, 0, 0, 0 } },
        { 9,  3, 24, 1, { 8, 8, 8, 0 }, { 0, 8, 16, 0 }, { 0, 0, 0, 0 } },
        { 6,  4, 32, 2, { 8, 8, 8, 8 }, { 16, 8, 0, 24 }, { 0, 0, 0, 0 } },
        { 11, 4, 32, 2, { 8, 8, 8, 8 }, { 0, 8, 8, 24 }, { 0, 0, 0, 0 } },
        { 12, 4, 32, 2, { 8, 8, 8, 8 }, { 24, 16, 8, 0 }, { 0, 0, 0, 0 } },
    };

    BitmapInterface* pBitmaps = SmushCoreTest_InitBitmap();
    TEST_ASSERT_NOT_NULL(pBitmaps);

    for ( size_t i = 0; i < sizeof(aTypes) / sizeof(aTypes[0]); ++i )
    {
        const SmushCoreTestBitmapType* pType = &aTypes[i];
        LecBitmapHeader* pBitmap = NULL;
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, pBitmaps->Create(&pBitmap, 7, 2, pType->type), "Create");
        TEST_ASSERT_EQUAL_INT(7 * 2 * pType->pixelSize + 0x5C, g_alloc.lastSize);
        TEST_ASSERT_EQUAL_INT(pType->pixelSize, pBitmap->pixelSize);
        TEST_ASSERT_EQUAL_INT(7 * pType->pixelSize, pBitmap->pitch);
        TEST_ASSERT_EQUAL_INT(pType->bpp, pBitmap->bpp);
        TEST_ASSERT_EQUAL_INT(pType->colorMode, pBitmap->colorMode);
        TEST_ASSERT_EQUAL_INT(pType->bpps[0], pBitmap->format.redBPP);
        TEST_ASSERT_EQUAL_INT(pType->bpps[1], pBitmap->format.greenBPP);
        TEST_ASSERT_EQUAL_INT(pType->bpps[2], pBitmap->format.blueBPP);
        TEST_ASSERT_EQUAL_INT(pType->bpps[3], pBitmap->format.alphaBPP);
        TEST_ASSERT_EQUAL_INT(pType->shifts[0], pBitmap->format.redPosShift);
        TEST_ASSERT_EQUAL_INT(pType->shifts[1], pBitmap->format.greenPosShift);
        TEST_ASSERT_EQUAL_INT(pType->shifts[2], pBitmap->format.bluePosShift);
        TEST_ASSERT_EQUAL_INT(pType->shifts[3], pBitmap->format.alphaPosShift);
        TEST_ASSERT_EQUAL_INT(pType->shiftsRight[0], pBitmap->format.redPosShiftRight);
        TEST_ASSERT_EQUAL_INT(pType->shiftsRight[1], pBitmap->format.greenPosShiftRight);
        TEST_ASSERT_EQUAL_INT(pType->shiftsRight[2], pBitmap->format.bluePosShiftRight);
        TEST_ASSERT_EQUAL_INT(pType->shiftsRight[3], pBitmap->format.alphaPosShiftRight);
        pBitmaps->Destroy(&pBitmap);
    }

    SmushCoreTest_ShutdownBitmap();
}

TEST(SmushCore, Bitmap_Create_Indexed)
{
    BitmapInterface* pBitmaps = SmushCoreTest_InitBitmap();
    TEST_ASSERT_NOT_NULL(pBitmaps);

    // 8 bit with a palette of 256 colors (4 bytes each) stored after the header
    LecBitmapHeader* pBitmap = NULL;
    TEST_ASSERT_EQUAL_INT(0, pBitmaps->Create(&pBitmap, 4, 3, 1));
    TEST_ASSERT_EQUAL_INT(4 * 3 + 256 * 4 + 0x5C, g_alloc.lastSize);
    TEST_ASSERT_EQUAL_INT(1, pBitmap->pixelSize);
    TEST_ASSERT_EQUAL_INT(8, pBitmap->bpp);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->colorMode);
    TEST_ASSERT_EQUAL_INT(256, pBitmap->indexed.nColors);
    TEST_ASSERT_TRUE(pBitmap->indexed.pPalette == pBitmap->pPixels);
    pBitmaps->Destroy(&pBitmap);

    // 8 bit without a palette
    TEST_ASSERT_EQUAL_INT(0, pBitmaps->Create(&pBitmap, 4, 3, 0));
    TEST_ASSERT_EQUAL_INT(4 * 3 + 0x5C, g_alloc.lastSize);
    TEST_ASSERT_EQUAL_INT(8, pBitmap->bpp);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->colorMode);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->indexed.nColors);
    TEST_ASSERT_NULL(pBitmap->indexed.pPalette);
    pBitmaps->Destroy(&pBitmap);

    SmushCoreTest_ShutdownBitmap();
}

TEST(SmushCore, Bitmap_Create_AllocFailure)
{
    BitmapInterface* pBitmaps = SmushCoreTest_InitBitmap();
    TEST_ASSERT_NOT_NULL(pBitmaps);

    LecBitmapHeader* pBitmap = (LecBitmapHeader*)&g_alloc;
    g_alloc.failNext = 1;
    TEST_ASSERT_EQUAL_INT(-101, pBitmaps->Create(&pBitmap, 4, 3, 3));
    TEST_ASSERT_TRUE(pBitmap == (LecBitmapHeader*)&g_alloc); // untouched

    g_alloc.failNext = 1;
    int format[12] = { 0 };
    TEST_ASSERT_EQUAL_INT(-101, pBitmaps->CreateEx(&pBitmap, 4, 3, 2, 2, 16, format));
    TEST_ASSERT_TRUE(pBitmap == (LecBitmapHeader*)&g_alloc);

    SmushCoreTest_ShutdownBitmap();
}

TEST(SmushCore, Bitmap_CreateEx)
{
    BitmapInterface* pBitmaps = SmushCoreTest_InitBitmap();
    TEST_ASSERT_NOT_NULL(pBitmaps);

    int format[12];
    for ( int i = 0; i < 12; ++i )
    {
        format[i] = 100 + i;
    }

    // Color mode 2 (and 4, 6): the whole 12 value format is copied
    LecBitmapHeader* pBitmap = NULL;
    TEST_ASSERT_EQUAL_INT(0, pBitmaps->CreateEx(&pBitmap, 6, 2, 2, 4, 32, format));
    TEST_ASSERT_EQUAL_INT(6 * 2 * 4 + 0x5C, g_alloc.lastSize);
    TEST_ASSERT_EQUAL_INT(6, pBitmap->width);
    TEST_ASSERT_EQUAL_INT(2, pBitmap->height);
    TEST_ASSERT_EQUAL_INT(4, pBitmap->pixelSize);
    TEST_ASSERT_EQUAL_INT(24, pBitmap->pitch);
    TEST_ASSERT_EQUAL_INT(32, pBitmap->bpp);
    TEST_ASSERT_EQUAL_INT(2, pBitmap->colorMode);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->unknown6);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->unknown7);
    TEST_ASSERT_EQUAL_MEMORY(format, &pBitmap->format, sizeof(format));
    pBitmaps->Destroy(&pBitmap);

    // Color mode 1 (and 3, 5): nine values
    TEST_ASSERT_EQUAL_INT(0, pBitmaps->CreateEx(&pBitmap, 6, 2, 1, 3, 24, format));
    TEST_ASSERT_EQUAL_MEMORY(format, &pBitmap->format, 9 * sizeof(int));
    TEST_ASSERT_EQUAL_INT(0, pBitmap->format.alphaBPP);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->format.alphaPosShift);
    TEST_ASSERT_EQUAL_INT(0, pBitmap->format.alphaPosShiftRight);
    pBitmaps->Destroy(&pBitmap);

    // Color mode 7: three values
    TEST_ASSERT_EQUAL_INT(0, pBitmaps->CreateEx(&pBitmap, 6, 2, 7, 2, 16, format));
    TEST_ASSERT_EQUAL_MEMORY(format, &pBitmap->format, 3 * sizeof(int));
    TEST_ASSERT_EQUAL_INT(0, pBitmap->format.redPosShift);
    pBitmaps->Destroy(&pBitmap);

    // Color mode 0: the palette size is the second value of the format
    int palFormat[2] = { 0, 16 };
    TEST_ASSERT_EQUAL_INT(0, pBitmaps->CreateEx(&pBitmap, 6, 2, 0, 1, 8, palFormat));
    TEST_ASSERT_EQUAL_INT(6 * 2 + 16 * 4 + 0x5C, g_alloc.lastSize);
    TEST_ASSERT_EQUAL_INT(16, pBitmap->indexed.nColors);
    TEST_ASSERT_TRUE(pBitmap->indexed.pPalette == pBitmap->pPixels);
    pBitmaps->Destroy(&pBitmap);

    SmushCoreTest_ShutdownBitmap();
}

TEST(SmushCore, Bitmap_Create_UnsupportedType_Fixed)
{
    BitmapInterface* pBitmaps = SmushCoreTest_InitBitmap();
    TEST_ASSERT_NOT_NULL(pBitmaps);

    // Fix: the original logged the line number with "%s" and read an invalid string address (access violation) for an unsupported type
    LecBitmapHeader* pBitmap = (LecBitmapHeader*)&g_alloc;
    TEST_ASSERT_EQUAL_INT(-1, pBitmaps->Create(&pBitmap, 4, 3, 99));
    TEST_ASSERT_TRUE(pBitmap == (LecBitmapHeader*)&g_alloc);
    TEST_ASSERT_EQUAL_INT(0, g_alloc.nAllocs);

    SmushCoreTest_ShutdownBitmap();
}

TEST(SmushCore, Bitmap_Destroy_Null_Fixed)
{
    BitmapInterface* pBitmaps = SmushCoreTest_InitBitmap();
    TEST_ASSERT_NOT_NULL(pBitmaps);

    // Fix: the original tested the pointer already moved to the block start, so a NULL bitmap freed address -8
    LecBitmapHeader* pBitmap = NULL;
    pBitmaps->Destroy(&pBitmap);
    TEST_ASSERT_NULL(pBitmap);
    TEST_ASSERT_EQUAL_INT(0, g_alloc.nFrees);

    TEST_ASSERT_EQUAL_INT(0, pBitmaps->Create(&pBitmap, 4, 3, 3));
    pBitmaps->Destroy(&pBitmap);
    TEST_ASSERT_NULL(pBitmap);
    TEST_ASSERT_EQUAL_INT(1, g_alloc.nFrees);

    SmushCoreTest_ShutdownBitmap();
}

/* ---- Timer ---- */

static volatile LONG g_timerCalls[TIMER_COUNT + 2];
static volatile LONG g_timerParam;

static void SmushCoreTest_TimerCallback(int index, int param)
{
    InterlockedIncrement(&g_timerCalls[index]);
    g_timerParam = param;
}

static TimerInterface* SmushCoreTest_InitTimer(void)
{
    memset((void*)g_timerCalls, 0, sizeof(g_timerCalls));
    g_timerParam = 0;
    LecModuleEntry modules[] = { { memory_Register, NULL }, { timer_Register, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "MEMORY=insane", "");
    TEST_ASSERT_EQUAL_INT(0, LecShare_Startup());
    g_timerStarted = 1;
    return LecShare_Subscribe("TIMER", NULL);
}

static void SmushCoreTest_ShutdownTimer(void)
{
    g_timerStarted = 0;
    LecShare_Shutdown();
    LecShare_UnregisterAll();
}

static BOOL SmushCoreTest_WaitForCalls(int index, LONG count, DWORD timeoutMs)
{
    DWORD start = GetTickCount();
    while ( g_timerCalls[index] < count )
    {
        if ( GetTickCount() - start > timeoutMs )
        {
            return FALSE;
        }

        Sleep(5);
    }

    return TRUE;
}

TEST(SmushCore, Timer_Register_PublishesInterface)
{
    LecModuleEntry modules[] = { { timer_Register, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");
    TimerInterface* pTimer = LecShare_Subscribe("TIMER", NULL);
    TEST_ASSERT_NOT_NULL(pTimer);
    TEST_ASSERT_TRUE(pTimer->Alloc == timer_Alloc);
    TEST_ASSERT_TRUE(pTimer->Sync == timer_Sync);
    TEST_ASSERT_EQUAL_INT(0, pTimer->GetTicks());
    TEST_ASSERT_EQUAL_INT(0, pTimer->GetTickLen()); // set when started
    LecShare_UnregisterAll();
    TEST_ASSERT_EQUAL_INT(0, LecShare_Count(NULL));
}

TEST(SmushCore, Timer_Alloc_Free)
{
    TimerInterface* pTimer = NULL;
    LecModuleEntry modules[] = { { timer_Register, NULL }, { NULL, NULL } };
    LecShare_Init(SmushCoreTest_ErrorFn, modules, "", "");
    pTimer = LecShare_Subscribe("TIMER", NULL);
    TEST_ASSERT_NOT_NULL(pTimer);

    // Indices run from 1 to 19, 0 means no free timer
    int aIndices[TIMER_COUNT];
    for ( int i = 0; i < TIMER_COUNT; ++i )
    {
        aIndices[i] = pTimer->Alloc(SmushCoreTest_TimerCallback, 1000, 1, i);
        TEST_ASSERT_EQUAL_INT(i + 1, aIndices[i]);
    }

    TEST_ASSERT_EQUAL_INT(0, pTimer->Alloc(SmushCoreTest_TimerCallback, 1000, 1, 0));

    // Out of range indices are ignored
    pTimer->Free(0);
    pTimer->Free(TIMER_COUNT + 1);
    pTimer->Free(-1);
    pTimer->SetInterval(0, 5);
    pTimer->SetInterval(TIMER_COUNT + 1, 5);
    TEST_ASSERT_EQUAL_INT(0, pTimer->Alloc(SmushCoreTest_TimerCallback, 1000, 1, 0));

    // A freed slot is the next one allocated
    pTimer->Free(5);
    pTimer->Free(3);
    TEST_ASSERT_EQUAL_INT(3, pTimer->Alloc(SmushCoreTest_TimerCallback, 1000, 1, 0));
    TEST_ASSERT_EQUAL_INT(5, pTimer->Alloc(SmushCoreTest_TimerCallback, 1000, 1, 0));
    TEST_ASSERT_EQUAL_INT(0, pTimer->Alloc(SmushCoreTest_TimerCallback, 1000, 1, 0));
    LecShare_UnregisterAll();
}

TEST(SmushCore, Timer_Startup_Shutdown)
{
    TimerInterface* pTimer = SmushCoreTest_InitTimer();
    TEST_ASSERT_NOT_NULL(pTimer);
    TEST_ASSERT_EQUAL_INT(0x4DAE, pTimer->GetTickLen());

    // The clock advances in whole tick lengths
    int ticks = pTimer->GetTicks();
    DWORD start = GetTickCount();
    while ( pTimer->GetTicks() == ticks && GetTickCount() - start < 3000 )
    {
        Sleep(1);
    }

    TEST_ASSERT_TRUE(pTimer->GetTicks() != ticks);
    TEST_ASSERT_EQUAL_INT(0, pTimer->GetTicks() % pTimer->GetTickLen());

    SmushCoreTest_ShutdownTimer();
}

TEST(SmushCore, Timer_Periodic_Callback)
{
    TimerInterface* pTimer = SmushCoreTest_InitTimer();
    TEST_ASSERT_NOT_NULL(pTimer);

    int index = pTimer->Alloc(SmushCoreTest_TimerCallback, pTimer->GetTickLen() * 2, 1, 1234);
    TEST_ASSERT_TRUE(index > 0);
    TEST_ASSERT_TRUE_MESSAGE(SmushCoreTest_WaitForCalls(index, 5, 5000), "periodic timer didn't fire");
    TEST_ASSERT_EQUAL_INT(1234, g_timerParam);

    pTimer->Free(index);
    Sleep(100); // let a tick in progress finish
    LONG calls = g_timerCalls[index];
    Sleep(150);
    TEST_ASSERT_EQUAL_INT(calls, g_timerCalls[index]);

    SmushCoreTest_ShutdownTimer();
}

TEST(SmushCore, Timer_OneShot_Callback)
{
    TimerInterface* pTimer = SmushCoreTest_InitTimer();
    TEST_ASSERT_NOT_NULL(pTimer);

    // Flag 2: the interval is cleared after the first call
    int index = pTimer->Alloc(SmushCoreTest_TimerCallback, pTimer->GetTickLen(), 1 | 2, 7);
    TEST_ASSERT_TRUE(index > 0);
    TEST_ASSERT_TRUE_MESSAGE(SmushCoreTest_WaitForCalls(index, 1, 5000), "one shot timer didn't fire");
    Sleep(200);
    TEST_ASSERT_EQUAL_INT(1, g_timerCalls[index]);

    // A new interval arms it again
    pTimer->SetInterval(index, pTimer->GetTickLen());
    TEST_ASSERT_TRUE_MESSAGE(SmushCoreTest_WaitForCalls(index, 2, 5000), "one shot timer didn't fire again");
    Sleep(200);
    TEST_ASSERT_EQUAL_INT(2, g_timerCalls[index]);

    SmushCoreTest_ShutdownTimer();
}

TEST(SmushCore, Timer_Deferred_Callback)
{
    TimerInterface* pTimer = SmushCoreTest_InitTimer();
    TEST_ASSERT_NOT_NULL(pTimer);

    // Flag 4: called from the poll (timer_Poll/timer_RunDeferred) after the tick's periodic timers
    int index = pTimer->Alloc(SmushCoreTest_TimerCallback, pTimer->GetTickLen(), 4, 99);
    TEST_ASSERT_TRUE(index > 0);
    TEST_ASSERT_TRUE_MESSAGE(SmushCoreTest_WaitForCalls(index, 5, 5000), "deferred timer didn't fire");
    TEST_ASSERT_EQUAL_INT(99, g_timerParam);

    SmushCoreTest_ShutdownTimer();
}

TEST(SmushCore, Timer_Default_Callback_NoFlags)
{
    TimerInterface* pTimer = SmushCoreTest_InitTimer();
    TEST_ASSERT_NOT_NULL(pTimer);

    // No flags: handled by the second loop of the tick (guarded by the busy flag)
    int index = pTimer->Alloc(SmushCoreTest_TimerCallback, pTimer->GetTickLen(), 0, 5);
    TEST_ASSERT_TRUE(index > 0);
    TEST_ASSERT_TRUE_MESSAGE(SmushCoreTest_WaitForCalls(index, 5, 5000), "timer didn't fire");

    SmushCoreTest_ShutdownTimer();
}

TEST(SmushCore, Timer_Wait)
{
    TimerInterface* pTimer = SmushCoreTest_InitTimer();
    TEST_ASSERT_NOT_NULL(pTimer);

    int start = pTimer->GetTicks();
    pTimer->Wait(pTimer->GetTickLen() * 3);
    TEST_ASSERT_TRUE(pTimer->GetTicks() - start >= pTimer->GetTickLen() * 3);

    SmushCoreTest_ShutdownTimer();
}

TEST(SmushCore, Timer_Sync)
{
    TimerInterface* pTimer = SmushCoreTest_InitTimer();
    TEST_ASSERT_NOT_NULL(pTimer);

    // Syncing a timer with flag 8 moves the elapsed time forward: the timer fires without waiting for the interval
    int interval = pTimer->GetTickLen() * 100000; // far in the future
    int index = pTimer->Alloc(SmushCoreTest_TimerCallback, interval, 1 | 8, 3);
    TEST_ASSERT_TRUE(index > 0);
    Sleep(100);
    TEST_ASSERT_EQUAL_INT(0, g_timerCalls[index]);

    pTimer->Sync(interval);
    TEST_ASSERT_TRUE_MESSAGE(SmushCoreTest_WaitForCalls(index, 1, 5000), "synced timer didn't fire");

    SmushCoreTest_ShutdownTimer();
}

TEST(SmushCore, Timer_Paused_NoCallbacks)
{
    TimerInterface* pTimer = SmushCoreTest_InitTimer();
    TEST_ASSERT_NOT_NULL(pTimer);

    pTimer->Pause();
    Sleep(50); // a tick in progress finishes
    int index = pTimer->Alloc(SmushCoreTest_TimerCallback, pTimer->GetTickLen(), 1, 0);
    int ticks = pTimer->GetTicks();
    Sleep(200);
    TEST_ASSERT_EQUAL_INT(0, g_timerCalls[index]);
    TEST_ASSERT_TRUE(pTimer->GetTicks() > ticks); // the clock runs while paused

    pTimer->Resume();
    TEST_ASSERT_TRUE_MESSAGE(SmushCoreTest_WaitForCalls(index, 1, 5000), "timer didn't fire after resume");

    SmushCoreTest_ShutdownTimer();
}

TEST(SmushCore, Timer_Start_Stop_Reference_Counted)
{
    TimerInterface* pTimer = SmushCoreTest_InitTimer();
    TEST_ASSERT_NOT_NULL(pTimer);

    // A second start only counts, the thread stops with the last stop
    TEST_ASSERT_EQUAL_INT(0, timer_Start(0, 0));
    timer_Stop();

    int ticks = pTimer->GetTicks();
    Sleep(100);
    TEST_ASSERT_TRUE(pTimer->GetTicks() > ticks);

    SmushCoreTest_ShutdownTimer();
    ticks = pTimer->GetTicks();
    Sleep(100);
    TEST_ASSERT_EQUAL_INT(ticks, pTimer->GetTicks());
}

/* ---- Runner ---- */

TEST_GROUP_RUNNER(SmushCore)
{
    RUN_TEST_CASE(SmushCore, LecShare_NameLen);
    RUN_TEST_CASE(SmushCore, LecShare_NameCmp);
    RUN_TEST_CASE(SmushCore, LecShare_NameNCmp);
    RUN_TEST_CASE(SmushCore, LecShare_Strip);
    RUN_TEST_CASE(SmushCore, LecShare_Strip_ReadOnlyUnchangedLiteral);
    RUN_TEST_CASE(SmushCore, LecShare_Match);
    RUN_TEST_CASE(SmushCore, LecShare_Init_RegistersModules);
    RUN_TEST_CASE(SmushCore, LecShare_Subscribe);
    RUN_TEST_CASE(SmushCore, LecShare_Subscribe_DefaultLists);
    RUN_TEST_CASE(SmushCore, LecShare_Subscribe_NotBeforeInit);
    RUN_TEST_CASE(SmushCore, LecShare_Unsubscribe);
    RUN_TEST_CASE(SmushCore, LecShare_Publish_Unpublish);
    RUN_TEST_CASE(SmushCore, LecShare_Publish_TableFull);
    RUN_TEST_CASE(SmushCore, LecShare_Enumerate);
    RUN_TEST_CASE(SmushCore, LecShare_Find);
    RUN_TEST_CASE(SmushCore, LecShare_BatchSubscribe);
    RUN_TEST_CASE(SmushCore, LecShare_BatchSubscribe_Failure);
    RUN_TEST_CASE(SmushCore, LecShare_Startup_Shutdown_Order);
    RUN_TEST_CASE(SmushCore, LecShare_Startup_Failure);
    RUN_TEST_CASE(SmushCore, LecShare_Reset);
    RUN_TEST_CASE(SmushCore, lec_Log_Wraps);
    RUN_TEST_CASE(SmushCore, LecList_Init);
    RUN_TEST_CASE(SmushCore, LecList_Insert_Remove);
    RUN_TEST_CASE(SmushCore, LecList_Insert_AtEnds);
    RUN_TEST_CASE(SmushCore, Memory_Register_PublishesServices);
    RUN_TEST_CASE(SmushCore, Memory_Debug_AllocFree);
    RUN_TEST_CASE(SmushCore, Memory_Debug_Realloc);
    RUN_TEST_CASE(SmushCore, Memory_Debug_OverwrittenGuardStillFreed);
    RUN_TEST_CASE(SmushCore, Bitmap_Startup_NoMemory);
    RUN_TEST_CASE(SmushCore, Bitmap_Create_Rgb565);
    RUN_TEST_CASE(SmushCore, Bitmap_Create_Types);
    RUN_TEST_CASE(SmushCore, Bitmap_Create_Indexed);
    RUN_TEST_CASE(SmushCore, Bitmap_Create_AllocFailure);
    RUN_TEST_CASE(SmushCore, Bitmap_CreateEx);
    RUN_TEST_CASE(SmushCore, Bitmap_Create_UnsupportedType_Fixed);
    RUN_TEST_CASE(SmushCore, Bitmap_Destroy_Null_Fixed);
    RUN_TEST_CASE(SmushCore, Timer_Register_PublishesInterface);
    RUN_TEST_CASE(SmushCore, Timer_Alloc_Free);
    RUN_TEST_CASE(SmushCore, Timer_Startup_Shutdown);
    RUN_TEST_CASE(SmushCore, Timer_Periodic_Callback);
    RUN_TEST_CASE(SmushCore, Timer_OneShot_Callback);
    RUN_TEST_CASE(SmushCore, Timer_Deferred_Callback);
    RUN_TEST_CASE(SmushCore, Timer_Default_Callback_NoFlags);
    RUN_TEST_CASE(SmushCore, Timer_Wait);
    RUN_TEST_CASE(SmushCore, Timer_Sync);
    RUN_TEST_CASE(SmushCore, Timer_Paused_NoCallbacks);
    RUN_TEST_CASE(SmushCore, Timer_Start_Stop_Reference_Counted);
}
