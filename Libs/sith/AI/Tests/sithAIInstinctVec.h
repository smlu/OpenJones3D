// Cases of the sithAIInstinct vectors, shared by the capture harness and sithAIInstinctVecTests. See
// tv/sithAIInstinct/README.md.
//
// sithAIInstinct_InitInstincts registers the engine's instincts through sithAI_RegisterInstinct into the instinct
// registry of sithAI.c (a pointer to an array of SITHAI_MAXREGISTEREDINSTINCTS entries and a count). The cases point
// the registry at the arena with a given number of entries already used and record what InitInstincts,
// sithAI_RegisterInstinct and sithAI_FindInstinct do with it.
#ifndef SITH_AI_TESTS_SITHAIINSTINCTVEC_H
#define SITH_AI_TESTS_SITHAIINSTINCTVEC_H

#include "sithVecWorld.h"

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIInstinct.h>

#include <stddef.h>

// File statics of sithAI.c: the registry. The harness reaches them at their v1.0 addresses, the tests through
// sithAIVecTested.c.
#ifdef SITHVEC_HARNESS
#define SITHAIVEC_REGISTRY    (*(SithAIRegisteredInstinct**)sithAI_aRegisteredInstincts_ADDR)
#define SITHAIVEC_NUMREGISTRY (*(size_t*)sithAI_numRegisteredInstincts_ADDR)
#define SITHAIVEC_FUNC(fn)    ((SithAIInstinctFunc)fn##_ADDR)
#else
SithAIRegisteredInstinct** sithAIVecTest_GetRegisteredInstincts(void);
size_t* sithAIVecTest_GetNumRegisteredInstincts(void);
#define SITHAIVEC_REGISTRY    (*sithAIVecTest_GetRegisteredInstincts())
#define SITHAIVEC_NUMREGISTRY (*sithAIVecTest_GetNumRegisteredInstincts())
#define SITHAIVEC_FUNC(fn)    ((SithAIInstinctFunc)fn)
#endif

enum
{
    SITHAIVEC_EV_REGISTER = SITHVEC_EV_SUITE, // return value of a sithAI_RegisterInstinct call of a case
    SITHAIVEC_EV_FIND,                        // result of a sithAI_FindInstinct call of a case
};

// Mirrors of the registry statics and of the registry entries, watched with the arena (the entries themselves are not
// compared directly, see sithAIVec_Sync)
static uint32_t sithAIVec_aMirror[2];
static SithAIRegisteredInstinct sithAIVec_aMirrorEntries[SITHVEC_CAPINSTINCTS];

// The original zero-fills the whole 64 byte name field (strncpy), the reconstruction copies the string and leaves the
// rest of its 63 byte field as it was (the Debug CRT fills it with 0xFE). Nothing reads past the terminator, so the
// mirror holds the entries with the bytes after the terminator zeroed; the name itself and every other field are
// compared exactly.
static void sithAIVec_Sync(void)
{
    sithAIVec_aMirror[0] = (uint32_t)(uintptr_t)SITHAIVEC_REGISTRY;
    sithAIVec_aMirror[1] = (uint32_t)SITHAIVEC_NUMREGISTRY;

    memcpy(sithAIVec_aMirrorEntries, sithVec_arena.instincts, sizeof(sithAIVec_aMirrorEntries));
    for ( size_t i = 0; i < SITHVEC_CAPINSTINCTS; ++i )
    {
        uint8_t* pName  = (uint8_t*)&sithAIVec_aMirrorEntries[i];
        size_t nameSize = offsetof(SithAIRegisteredInstinct, pfInstinct);
        uint8_t* pEnd   = (uint8_t*)memchr(pName, 0, nameSize);
        if ( pEnd )
        {
            memset(pEnd, 0, nameSize - (size_t)(pEnd - pName));
        }
    }
}

static void sithAIVec_Watch(void)
{
    SITHVEC_WATCH(sithAIVec_aMirror);
    SITHVEC_WATCH(sithAIVec_aMirrorEntries);
    sithVec_Ignore(sithVec_arena.instincts, sizeof(sithVec_arena.instincts));
}

// Instinct functions; registered function pointers are recorded as their number in this list
#define SITHAIVEC_NUMFUNCS 25u
#define SITHAIVEC_NOFUNC   0xFFu // a NULL function

static SithAIInstinctFunc sithAIVec_apFuncs[SITHAIVEC_NUMFUNCS];

#define SITHAIVEC_ADDINSTINCT(i, fn) \
    SITHVEC_ADDFUNC(fn);             \
    sithAIVec_apFuncs[i] = SITHAIVEC_FUNC(fn)

static void sithAIVec_AddInstinctFuncs(void)
{
    SITHAIVEC_ADDINSTINCT(0, sithAIInstinct_Listen);
    SITHAIVEC_ADDINSTINCT(1, sithAIInstinct_LookForTarget);
    SITHAIVEC_ADDINSTINCT(2, sithAIInstinct_SenseDanger);
    SITHAIVEC_ADDINSTINCT(3, sithAIInstinct_FearGunshot);
    SITHAIVEC_ADDINSTINCT(4, sithAIInstinct_Hop);
    SITHAIVEC_ADDINSTINCT(5, sithAIInstinct_HoverDrift);
    SITHAIVEC_ADDINSTINCT(6, sithAIInstinct_Talk);
    SITHAIVEC_ADDINSTINCT(7, sithAIInstinct_RandomTurn);
    SITHAIVEC_ADDINSTINCT(8, sithAIInstinct_OpenDoors);
    SITHAIVEC_ADDINSTINCT(9, sithAIInstinct_Jump);
    SITHAIVEC_ADDINSTINCT(10, sithAIInstinct_LobFire);
    SITHAIVEC_ADDINSTINCT(11, sithAIInstinct_PrimaryFire);
    SITHAIVEC_ADDINSTINCT(12, sithAIInstinct_AlternateFire);
    SITHAIVEC_ADDINSTINCT(13, sithAIInstinct_CircleStrafe);
    SITHAIVEC_ADDINSTINCT(14, sithAIInstinct_Dodge);
    SITHAIVEC_ADDINSTINCT(15, sithAIInstinct_HitAndRun);
    SITHAIVEC_ADDINSTINCT(16, sithAIInstinct_Retreat);
    SITHAIVEC_ADDINSTINCT(17, sithAIInstinct_RandomMove);
    SITHAIVEC_ADDINSTINCT(18, sithAIInstinct_HumanCombatMove);
    SITHAIVEC_ADDINSTINCT(19, sithAIInstinct_Roam);
    SITHAIVEC_ADDINSTINCT(20, sithAIInstinct_ReturnHome);
    SITHAIVEC_ADDINSTINCT(21, sithAIInstinct_Flee);
    SITHAIVEC_ADDINSTINCT(22, sithAIInstinct_BasicFallow);
    SITHAIVEC_ADDINSTINCT(23, sithAIInstinct_WallCrawl);
    SITHAIVEC_ADDINSTINCT(24, sithAIInstinct_SnakeFollow);
}

// A direct sithAI_RegisterInstinct call. pName NULL: the last nameLen characters of sithAIVec_aLongName (up to 209).
typedef struct
{
    const char* pName;
    uint8_t nameLen;
    uint8_t func; // index into sithAIVec_apFuncs or SITHAIVEC_NOFUNC
    uint32_t updateModes;
    uint32_t updateBlockModes;
    uint32_t triggerEvents;
} SithAIVecRegister;

typedef struct
{
    uint32_t numUsed;                     // registry entries in use before the case
    uint8_t numInitCalls;                 // sithAIInstinct_InitInstincts calls (one checkpoint each)
    uint8_t numRegisters;                 // then these sithAI_RegisterInstinct calls (one checkpoint)
    const SithAIVecRegister* aRegisters;
    uint8_t numFinds;                     // then these sithAI_FindInstinct calls (one checkpoint)
    const char* const* apFinds;
} SithAIInstinctVecParams;

static const char sithAIVec_aLongName[] =
    "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefgh"
    "ijklmnopqrst"; // 209 characters

// The used entries hold earlier registrations ("pre<n>", Hop, random modes); the rest of the array (and the entries
// past its end) the run's garbage fill
static void sithAIVec_Build(const SithVecCase* pCase)
{
    const SithAIInstinctVecParams* pParams = (const SithAIInstinctVecParams*)pCase->pParams;
    sithAIVec_AddInstinctFuncs();

    for ( size_t i = 0; i < SITHAI_MAXREGISTEREDINSTINCTS && i < pParams->numUsed; ++i )
    {
        SithAIRegisteredInstinct* pEntry = &sithVec_arena.instincts[i];
        memset(pEntry->aName, 0, sizeof(pEntry->aName));
        snprintf(pEntry->aName, sizeof(pEntry->aName), "pre%u", (unsigned int)i);
        pEntry->pfInstinct       = sithAIVec_apFuncs[4];
        pEntry->updateModes      = (SithAIMode)sithVec_Rnd();
        pEntry->updateBlockModes = (SithAIMode)sithVec_Rnd();
        pEntry->triggerEvents    = (SithAIEventType)sithVec_Rnd();
    }

    SITHAIVEC_REGISTRY    = sithVec_arena.instincts;
    SITHAIVEC_NUMREGISTRY = pParams->numUsed;
}

static int sithAIVec_Run(SithVecCtx* pCtx)
{
    const SithAIInstinctVecParams* pParams = (const SithAIInstinctVecParams*)pCtx->pCase->pParams;
    sithVec_Begin(pCtx);

    for ( unsigned int i = 0; i < pParams->numInitCalls; ++i )
    {
        SITHVEC_FN(sithAIInstinct_InitInstincts)();
        sithVec_Checkpoint(pCtx);
    }

    for ( unsigned int i = 0; i < pParams->numRegisters; ++i )
    {
        const SithAIVecRegister* pReg = &pParams->aRegisters[i];
        const char* pName = pReg->pName ? pReg->pName : &sithAIVec_aLongName[sizeof(sithAIVec_aLongName) - 1 - pReg->nameLen];
        SithAIInstinctFunc pfInstinct = pReg->func == SITHAIVEC_NOFUNC ? NULL : sithAIVec_apFuncs[pReg->func];
        int ret = SITHVEC_FN(sithAI_RegisterInstinct)(pName, pfInstinct, (SithAIMode)pReg->updateModes, (SithAIMode)pReg->updateBlockModes, (SithAIEventType)pReg->triggerEvents);
        sithVec_Ev(SITHAIVEC_EV_REGISTER, 1, (uint32_t)ret);
    }

    if ( pParams->numRegisters )
    {
        sithVec_Checkpoint(pCtx);
    }

    for ( unsigned int i = 0; i < pParams->numFinds; ++i )
    {
        SithAIRegisteredInstinct* pFound = SITHVEC_FN(sithAI_FindInstinct)(pParams->apFinds[i]);
        sithVec_Ev(SITHAIVEC_EV_FIND, 1, SITHVEC_P(pFound));
    }

    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Cases
// ---------------------------------------------------------------------------------------------------------------

// Every name InitInstincts registers, case variants, a prefix, an extension, an empty name, an unknown name and NULL
static const char* const sithAIVec_apFindAll[] = {
    "listen", "lookfortarget", "sensedanger", "primaryfire", "alternatefire", "lobfire", "opendoors", "jump",
    "randomturn", "roam", "flee", "retreat", "circlestrafe", "returnhome", "talk", "hitandrun", "dodge", "hop",
    "hoverdrift", "basicfollow", "wallcrawl", "snakefollow", "randommove", "humancombatmove", "feargunshot",
    "LISTEN", "FearGunShot", "list", "listener", "", "nosuchinstinct", "pre0", "pre5", NULL,
};

// Names up to 62 characters (they fit the name field of both sides), the extreme mode and event masks and a NULL
// function (asserts, then registers it)
static const SithAIVecRegister sithAIVec_aRegisterNames[] = {
    { "", 0, 4, 0, 0, 0 },
    { "x", 0, 0, 1, 2, 3 },
    { NULL, 61, 1, 0xFFFFFFFFu, 0, 0x80000000u },
    { NULL, 62, 2, 0, 0xFFFFFFFFu, 0xFFFFFFFFu },
    { "nullfunc", 0, SITHAIVEC_NOFUNC, 5, 6, 7 },
};

#define SITHAIVEC_LONGNAME(len) &sithAIVec_aLongName[sizeof(sithAIVec_aLongName) - 1 - (len)]

static const char* const sithAIVec_apFindNames[] = {
    "", "x", "X", SITHAIVEC_LONGNAME(61), SITHAIVEC_LONGNAME(62), "nullfunc", "hop", NULL,
};

// Names of 63, 64 and 199 characters: the original keeps 63 characters (64 byte field), the reconstruction 62
static const SithAIVecRegister sithAIVec_aRegisterName63[] = { { NULL, 63, 3, 0x7FFFFFFFu, 0x80000000u, 0 } };
static const SithAIVecRegister sithAIVec_aRegisterName64[] = { { NULL, 64, 24, 0x12345678u, 0x9ABCDEF0u, 0x0F0F0F0Fu } };
static const SithAIVecRegister sithAIVec_aRegisterName199[] = { { NULL, 199, 23, 0, 0, 0 } };

static const char* const sithAIVec_apFindLongNames[] = {
    SITHAIVEC_LONGNAME(62), SITHAIVEC_LONGNAME(63), SITHAIVEC_LONGNAME(64), SITHAIVEC_LONGNAME(199),
};

// Two registrations into a registry with one free entry: the first fills it, the second is rejected
static const SithAIVecRegister sithAIVec_aRegisterTwo[] = {
    { "last", 0, 5, 1, 1, 1 },
    { "rejected", 0, 6, 2, 2, 2 },
};

#define SITHAIVEC_FINDS(a)     (uint8_t)SITHVEC_ARRAYLEN(a), a
#define SITHAIVEC_REGISTERS(a) (uint8_t)SITHVEC_ARRAYLEN(a), a
#define P(name, numUsed, numInit, regs, finds) static const SithAIInstinctVecParams sithAIVec_p##name = { numUsed, numInit, regs, finds };
#define NOREGS 0, NULL
#define NOFINDS 0, NULL

P(Init0, 0, 1, NOREGS, SITHAIVEC_FINDS(sithAIVec_apFindAll))
P(Init1, 1, 1, NOREGS, NOFINDS)
P(Init6, 6, 1, NOREGS, SITHAIVEC_FINDS(sithAIVec_apFindAll))
P(Init7, 7, 1, NOREGS, SITHAIVEC_FINDS(sithAIVec_apFindAll))
P(Init8, 8, 1, NOREGS, SITHAIVEC_FINDS(sithAIVec_apFindAll))
P(Init20, 20, 1, NOREGS, NOFINDS)
P(Init31, 31, 1, NOREGS, NOFINDS)
P(Init32, 32, 1, NOREGS, SITHAIVEC_FINDS(sithAIVec_apFindAll))
P(Init33, 33, 1, NOREGS, NOFINDS)
P(InitMax, 0xFFFFFFFFu, 1, NOREGS, NOFINDS)
P(Init0Twice, 0, 2, NOREGS, SITHAIVEC_FINDS(sithAIVec_apFindAll))
P(Init0Thrice, 0, 3, NOREGS, NOFINDS)
P(Init0Register, 0, 1, SITHAIVEC_REGISTERS(sithAIVec_aRegisterNames), SITHAIVEC_FINDS(sithAIVec_apFindNames))
P(Register0, 0, 0, SITHAIVEC_REGISTERS(sithAIVec_aRegisterNames), SITHAIVEC_FINDS(sithAIVec_apFindNames))
P(RegisterName63, 2, 0, SITHAIVEC_REGISTERS(sithAIVec_aRegisterName63), SITHAIVEC_FINDS(sithAIVec_apFindLongNames))
P(RegisterName64, 2, 0, SITHAIVEC_REGISTERS(sithAIVec_aRegisterName64), SITHAIVEC_FINDS(sithAIVec_apFindLongNames))
P(RegisterName199, 2, 0, SITHAIVEC_REGISTERS(sithAIVec_aRegisterName199), SITHAIVEC_FINDS(sithAIVec_apFindLongNames))
P(Register31, 31, 0, SITHAIVEC_REGISTERS(sithAIVec_aRegisterTwo), NOFINDS)
P(Register32, 32, 0, SITHAIVEC_REGISTERS(sithAIVec_aRegisterTwo), NOFINDS)
P(Find0, 0, 0, NOREGS, SITHAIVEC_FINDS(sithAIVec_apFindAll))
P(Find5, 5, 0, NOREGS, SITHAIVEC_FINDS(sithAIVec_apFindAll))

#undef P
#undef NOREGS
#undef NOFINDS
#undef SITHAIVEC_FINDS
#undef SITHAIVEC_REGISTERS

#define C(name, params) { name, sithAIVec_Run, 0, &sithAIVec_p##params }

static const SithVecCase sithAIVec_aInstinctCases[] = {
    // sithAIInstinct_InitInstincts into a registry with 0..33 entries in use: 0 is engine startup; with 7 used the
    // 25 instincts fill it exactly, from 8 the last ones are rejected, from 32 all; a count far above the size
    C("InitInstincts_empty", Init0),
    C("InitInstincts_used1", Init1),
    C("InitInstincts_used6", Init6),
    C("InitInstincts_used7_fills", Init7),
    C("InitInstincts_used8_rejectslast", Init8),
    C("InitInstincts_used20", Init20),
    C("InitInstincts_used31", Init31),
    C("InitInstincts_edge_used32_full", Init32),
    C("InitInstincts_edge_used33", Init33),
    C("InitInstincts_edge_usedmax", InitMax),
    // Repeated initialization registers duplicates until the registry is full; lookups find the first entry
    C("InitInstincts_twice", Init0Twice),
    C("InitInstincts_thrice", Init0Thrice),
    // Registrations after initialization and on their own: names up to 62 characters, mode masks, a NULL function;
    // names of 63 and more characters (the name field is 64 bytes in the original, 63 in the reconstruction)
    C("RegisterInstinct_afterinit", Init0Register),
    C("RegisterInstinct_names", Register0),
    C("RegisterInstinct_edge_name63", RegisterName63),
    C("RegisterInstinct_edge_name64", RegisterName64),
    C("RegisterInstinct_edge_name199", RegisterName199),
    C("RegisterInstinct_lastfree", Register31),
    C("RegisterInstinct_full", Register32),
    // Lookups in an empty registry and one with only earlier registrations
    C("FindInstinct_empty", Find0),
    C("FindInstinct_preonly", Find5),
};

#undef C

static const SithVecSuite sithAIVec_instinctSuite = {
    "sithAIInstinct",
    sithAIVec_aInstinctCases,
    SITHVEC_ARRAYLEN(sithAIVec_aInstinctCases),
    sithAIVec_Build,
    sithAIVec_Watch,
    sithAIVec_Sync,
};

#define SITHVEC_SUITE sithAIVec_instinctSuite

#endif // SITH_AI_TESTS_SITHAIINSTINCTVEC_H
