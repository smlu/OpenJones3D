#ifndef SITH_FRAMERATE_VEC_H
#define SITH_FRAMERATE_VEC_H

// Frame-rate vectors: the cases, the frame model, the stubs and the runners shared by sithFrameRateTest.c and the harness
// that captured tv/sithFrameRate/sithFrameRate.tv from the original functions in the retail executable (see its README.md).
//
// The includer defines, before including this header:
//   SITHFRAMERATEVEC_CALL(func, ...) - Calls an engine function: the reconstruction in the tests, its v1.0 address in the harness
//   SITHFRAMERATEVEC_CHECK(cond)     - Checks an argument the engine code passed to a stub
//   SITHFRAMERATEVEC_SCHEDULE(msec)  - The next update an AI instinct schedules: now + msec in the original, the reconstruction's
//                                      deadline in the tests (the instinct itself is a stub, sithAI_InstinctUpdate is the engine's)
//   SITHFRAMERATEVEC_SET_MOVE_BLOCKED(b) - Makes sithCollision_MoveThing move nothing (the path mover is blocked): the tests set
//                                      their sithPathMove stub, the harness has the function patched to the blocked stub
//
// The engine globals resolve to the test storage in the tests (sithEngineTestAddrs.h) and to their v1.0 addresses in the harness.
// The harness compiles this header with the original (non-QOL) struct layouts.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sith/types.h>
#include <sith/AI/sithAI.h>
#include <sith/Cog/sithCog.h>
#include <sith/Engine/sithPathMove.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/World/sithWorld.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithItem.h>
#include <sith/World/sithWeapon.h>

// COG script functions, registered by sithCogFunction.c and sithCogFunctionThing.c without a header declaration
void J3DAPI sithCogFunction_Sleep(SithCog* pCog);
void J3DAPI sithCogFunction_SetPulse(SithCog* pCog);
void J3DAPI sithCogFunctionThing_SetThingPulse(SithCog* pCog);
void J3DAPI sithCogFunction_SetTimer(SithCog* pCog);
void J3DAPI sithCogFunctionThing_SetThingTimer(SithCog* pCog);
void J3DAPI sithAI_InstinctUpdate(SithAIControlBlock* pLocal); // sithAI.c, no header declaration
void J3DAPI sithPhysics_UpdateExhaustFx(SithThing* pThing, const SithVehicleExhaustInfo* pExhaustInfo, float secDeltaTime); // sithPhysics.c, no header declaration
void J3DAPI sithActor_PlayDamageSoundFx(SithThing* pThing, SithDamageType damageType); // sithActor.c, no header declaration

#define SITHFRAMERATEVEC_MSECDURATION 3000u // Simulated game time of every case

// Frame rates of the vectors: below and at the designed 30 FPS, just above it, common refresh rates and uncapped rates
static const size_t sithFrameRateVec_aFps[] = { 20u, 30u, 34u, 45u, 60u, 72u, 75u, 100u, 120u, 144u, 165u, 240u, 300u, 1000u };

typedef enum eSithFrameRateVecKind
{
    SITHFRAMERATEVEC_COG_PULSE,      // SetPulse(interval)
    SITHFRAMERATEVEC_COG_SLEEPLOOP,  // A loop of Sleep(interval)
    SITHFRAMERATEVEC_COG_THINGPULSE, // SetThingPulse(thing, interval)
    SITHFRAMERATEVEC_COG_TIMERLOOP,  // SetTimer(interval), set again in every timer message
    SITHFRAMERATEVEC_COG_THINGTIMERLOOP, // SetThingTimer(thing, interval), set again in every timer message
    SITHFRAMERATEVEC_EVENTLOOP,      // A sithEvent (SetTimerEx) created again by its own task
    SITHFRAMERATEVEC_AITICK,         // An AI instinct that schedules its next update interval later (0: the 1 sec default)
    SITHFRAMERATEVEC_WEAPONREFIRE,   // A held weapon that fires again every interval (sithWeapon_UpdateActorWeapon)
    SITHFRAMERATEVEC_WEAPONWAIT,     // A weapon wait set once, until sithWeapon_IsMountingWeapon says it's over
    SITHFRAMERATEVEC_ITEMTOUCH,      // An item the player keeps touching: it's touched again every 500 msec
    SITHFRAMERATEVEC_SURFACING,      // Floating on the water surface after holding the breath to the limit
    SITHFRAMERATEVEC_DROWNGURGLE,    // The drowning sound of an actor that takes drowning damage every frame
    SITHFRAMERATEVEC_WEAPONNOISE,    // The AI awareness events of a decaying weapon (sithWeapon_Update)
    SITHFRAMERATEVEC_PATHBLOCKED,    // A path mover that is blocked: its blocked messages
    SITHFRAMERATEVEC_EXHAUST,        // The exhaust smoke of a vehicle (sithPhysics_UpdateExhaustFx)
    SITHFRAMERATEVEC_PATHMOVE,       // A path mover moving one unit: when it arrives
    SITHFRAMERATEVEC_PATHROTATE,     // A path mover rotating 90 degrees about its pivot: when it arrives
    SITHFRAMERATEVEC_DROWNING,       // Under water, out of breath
    SITHFRAMERATEVEC_RAFTLEAK,       // On a leaking raft
    SITHFRAMERATEVEC_IMP,            // The player's IMP state: firing an IMP, in an Aetherium sector or in the POR
} SithFrameRateVecKind;

typedef enum eSithFrameRateVecWeaponWait
{
    SITHFRAMERATEVEC_WAIT_FIRE,     // sithWeapon_SetFireWait
    SITHFRAMERATEVEC_WAIT_AIM,      // sithWeapon_SetAimWait
    SITHFRAMERATEVEC_WAIT_MOUNT,    // sithWeapon_SetMountWait (player only)
    SITHFRAMERATEVEC_WAIT_ACTIVATE, // sithWeapon_ActivateWeapon
} SithFrameRateVecWeaponWait;

typedef struct sSithFrameRateVecCase
{
    const char* pName;
    SithFrameRateVecKind kind;
    float secInterval;          // COG, event, AI, weapon: the interval; path mover: the designed duration of the move or rotation
    int waitKind;               // Weapon wait: which wait is set, a SithFrameRateVecWeaponWait
    bool bOnFrameBoundary;      // COG: within a msec of a whole number of 30 FPS frames, see sithFrameRateTest.c
    int thingIdx;               // Surfacing: thing index, the offset of the frame cycles
    int difficulty;             // Raft leak: game difficulty
    int impFireType;            // IMP: IMP weapon fired or SITHPLAYER_IMPFIRE_OFF
    SithSectorFlag sectorFlags; // IMP: SITH_SECTOR_AETHERIUM for the Aetherium sector
    int bInPor;                 // IMP: player is in the POR
    float impScalar;            // IMP: sithGetIMPDamageScalar, 1 on the hardest difficulty
    float impState;             // IMP: start state; at 180 the IMP is overheated or the Aetherium stamina has run out

    // Design values of the fixed damage tests, not used by the harness
    float damagePerSec;         // Damage rate
    uint32_t msecDamageStart;   // Game time at which the damage starts: the IMP overheats or the stamina runs out
    float finalImpState;        // IMP state at the end
} SithFrameRateVecCase;

#define SITHFRAMERATEVEC_COGCASES(kind, prefix)                                    \
    { prefix "_0.001", (kind), .secInterval = 0.001f },                            \
    { prefix "_0.010", (kind), .secInterval = 0.01f },                             \
    { prefix "_0.025", (kind), .secInterval = 0.025f },                            \
    { prefix "_0.034", (kind), .secInterval = 0.034f, .bOnFrameBoundary = true },  \
    { prefix "_0.040", (kind), .secInterval = 0.04f },                             \
    { prefix "_0.050", (kind), .secInterval = 0.05f },                             \
    { prefix "_0.100", (kind), .secInterval = 0.1f },                              \
    { prefix "_0.200", (kind), .secInterval = 0.2f },                              \
    { prefix "_0.500", (kind), .secInterval = 0.5f },                              \
    { prefix "_0.700", (kind), .secInterval = 0.7f },                              \
    { prefix "_1.000", (kind), .secInterval = 1.0f }

#define SITHFRAMERATEVEC_IMPCASE(name, fireType, sector, inPor, scalar, state, rate, msecStart, finalState)           \
    { name, SITHFRAMERATEVEC_IMP, .impFireType = (fireType), .sectorFlags = (sector), .bInPor = (inPor),               \
      .impScalar = (scalar), .impState = (state), .damagePerSec = (rate), .msecDamageStart = (msecStart), .finalImpState = (finalState) }

static const SithFrameRateVecCase sithFrameRateVec_aCases[] = {
    SITHFRAMERATEVEC_COGCASES(SITHFRAMERATEVEC_COG_PULSE, "CogPulse"),
    SITHFRAMERATEVEC_COGCASES(SITHFRAMERATEVEC_COG_SLEEPLOOP, "CogSleep"),
    SITHFRAMERATEVEC_COGCASES(SITHFRAMERATEVEC_COG_THINGPULSE, "CogThingPulse"),
    SITHFRAMERATEVEC_COGCASES(SITHFRAMERATEVEC_COG_TIMERLOOP, "CogTimer"),
    SITHFRAMERATEVEC_COGCASES(SITHFRAMERATEVEC_COG_THINGTIMERLOOP, "CogThingTimer"),
    SITHFRAMERATEVEC_COGCASES(SITHFRAMERATEVEC_EVENTLOOP, "Event"),

    // AI instinct ticks: the 1 sec default of sithAI_InstinctUpdate and the intervals the instincts set themselves
    { "AITick_default", SITHFRAMERATEVEC_AITICK, .secInterval = 0.0f },
    { "AITick_0.150",   SITHFRAMERATEVEC_AITICK, .secInterval = 0.15f },
    { "AITick_0.250",   SITHFRAMERATEVEC_AITICK, .secInterval = 0.25f },
    { "AITick_0.500",   SITHFRAMERATEVEC_AITICK, .secInterval = 0.5f },
    { "AITick_0.750",   SITHFRAMERATEVEC_AITICK, .secInterval = 0.75f },

    // A held weapon firing again every fire wait, and the waits a weapon script sets once
    { "WeaponRefire_0.050", SITHFRAMERATEVEC_WEAPONREFIRE, .secInterval = 0.05f },
    { "WeaponRefire_0.100", SITHFRAMERATEVEC_WEAPONREFIRE, .secInterval = 0.1f },
    { "WeaponRefire_0.150", SITHFRAMERATEVEC_WEAPONREFIRE, .secInterval = 0.15f },
    { "WeaponRefire_0.250", SITHFRAMERATEVEC_WEAPONREFIRE, .secInterval = 0.25f },
    { "WeaponRefire_0.500", SITHFRAMERATEVEC_WEAPONREFIRE, .secInterval = 0.5f },
    { "WeaponFireWait_0.150", SITHFRAMERATEVEC_WEAPONWAIT, .secInterval = 0.15f, .waitKind = SITHFRAMERATEVEC_WAIT_FIRE },
    { "WeaponFireWait_0.500", SITHFRAMERATEVEC_WEAPONWAIT, .secInterval = 0.5f,  .waitKind = SITHFRAMERATEVEC_WAIT_FIRE },
    { "WeaponAimWait_0.150",  SITHFRAMERATEVEC_WEAPONWAIT, .secInterval = 0.15f, .waitKind = SITHFRAMERATEVEC_WAIT_AIM },
    { "WeaponMountWait_0.150", SITHFRAMERATEVEC_WEAPONWAIT, .secInterval = 0.15f, .waitKind = SITHFRAMERATEVEC_WAIT_MOUNT },
    { "WeaponActivate_0.150", SITHFRAMERATEVEC_WEAPONWAIT, .secInterval = 0.15f, .waitKind = SITHFRAMERATEVEC_WAIT_ACTIVATE },
    { "WeaponActivate_1.000", SITHFRAMERATEVEC_WEAPONWAIT, .secInterval = 1.0f,  .waitKind = SITHFRAMERATEVEC_WAIT_ACTIVATE },

    // The player standing on an item
    { "ItemTouch", SITHFRAMERATEVEC_ITEMTOUCH },

    { "Surfacing",      SITHFRAMERATEVEC_SURFACING, .thingIdx = 0 },
    { "Surfacing_idx5", SITHFRAMERATEVEC_SURFACING, .thingIdx = 5 },

    // Effects the original ran on every n-th frame, and the blocked messages of a path mover
    { "DrownGurgle",  SITHFRAMERATEVEC_DROWNGURGLE },
    { "WeaponNoise",  SITHFRAMERATEVEC_WEAPONNOISE },
    { "PathBlocked",  SITHFRAMERATEVEC_PATHBLOCKED },
    { "Exhaust",      SITHFRAMERATEVEC_EXHAUST },

    // Path movers: a move of one unit at 0.5 units per sec and a 90 degree pivot rotation in 1 sec arrive after that time
    { "PathMove_2.000",   SITHFRAMERATEVEC_PATHMOVE,   .secInterval = 2.0f },
    { "PathMove_0.500",   SITHFRAMERATEVEC_PATHMOVE,   .secInterval = 0.5f },
    { "PathRotate_1.000", SITHFRAMERATEVEC_PATHROTATE, .secInterval = 1.0f },
    { "PathRotate_0.250", SITHFRAMERATEVEC_PATHROTATE, .secInterval = 0.25f },

    // 1 damage per SITHACTOR_DROWN_MSEC_PER_DAMAGE msec; msec / 1.2 at the easiest, msec / 0.6 at the hardest difficulty
    { "Drowning",      SITHFRAMERATEVEC_DROWNING, .damagePerSec = 1000.0f / (float)SITHACTOR_DROWN_MSEC_PER_DAMAGE },
    { "RaftLeak_easy", SITHFRAMERATEVEC_RAFTLEAK, .difficulty = 0, .damagePerSec = 1000.0f / 1.2f },
    { "RaftLeak_hard", SITHFRAMERATEVEC_RAFTLEAK, .difficulty = SITH_GAME_DIFFICULTY_MAX, .damagePerSec = 1000.0f / 0.6f },

    // IMP2, IMP3 and IMP5 heat up by 10 per sec and deal 40 per sec when overheated, both scaled by sithGetIMPDamageScalar
    SITHFRAMERATEVEC_IMPCASE("ImpOverheated_imp2",      SITHWEAPON_IMP2, 0, 0, 1.0f, 180.0f, 40.0f, 0u, 180.0f),
    SITHFRAMERATEVEC_IMPCASE("ImpOverheated_imp3",      SITHWEAPON_IMP3, 0, 0, 1.0f, 180.0f, 40.0f, 0u, 180.0f),
    SITHFRAMERATEVEC_IMPCASE("ImpOverheated_imp5",      SITHWEAPON_IMP5, 0, 0, 1.0f, 180.0f, 40.0f, 0u, 180.0f),
    SITHFRAMERATEVEC_IMPCASE("ImpOverheated_imp2_half", SITHWEAPON_IMP2, 0, 0, 0.5f, 180.0f, 20.0f, 0u, 180.0f),
    SITHFRAMERATEVEC_IMPCASE("ImpOverheated_imp5_half", SITHWEAPON_IMP5, 0, 0, 0.5f, 180.0f, 20.0f, 0u, 180.0f),
    SITHFRAMERATEVEC_IMPCASE("ImpOverheats_imp5",       SITHWEAPON_IMP5, 0, 0, 1.0f, 170.0f, 40.0f, 1000u, 180.0f),
    SITHFRAMERATEVEC_IMPCASE("ImpOverheats_imp5_half",  SITHWEAPON_IMP5, 0, 0, 0.5f, 170.0f, 20.0f, 2000u, 180.0f),

    // In an Aetherium sector the stamina runs out by 1 per sec and then deals 180 per sec, both scaled
    SITHFRAMERATEVEC_IMPCASE("AetheriumOut",           SITHPLAYER_IMPFIRE_OFF, SITH_SECTOR_AETHERIUM, 0, 1.0f, 180.0f, 180.0f, 0u, 180.0f),
    SITHFRAMERATEVEC_IMPCASE("AetheriumOut_half",      SITHPLAYER_IMPFIRE_OFF, SITH_SECTOR_AETHERIUM, 0, 0.5f, 180.0f, 90.0f, 0u, 180.0f),
    SITHFRAMERATEVEC_IMPCASE("AetheriumRunsOut",       SITHPLAYER_IMPFIRE_OFF, SITH_SECTOR_AETHERIUM, 0, 1.0f, 179.0f, 180.0f, 1000u, 180.0f),
    SITHFRAMERATEVEC_IMPCASE("AetheriumRunsOut_half",  SITHPLAYER_IMPFIRE_OFF, SITH_SECTOR_AETHERIUM, 0, 0.5f, 179.0f, 90.0f, 2000u, 180.0f),
    SITHFRAMERATEVEC_IMPCASE("AetheriumRunsOut_inPor", SITHPLAYER_IMPFIRE_OFF, SITH_SECTOR_AETHERIUM, 1, 1.0f, 179.0f, 180.0f, 1000u, 180.0f),

    // In the POR the stamina recovers by 8 per sec, but not while the IMP heats up
    SITHFRAMERATEVEC_IMPCASE("PorRecovers",        SITHPLAYER_IMPFIRE_OFF, 0, 1, 1.0f, 100.0f, 0.0f, 0u, 76.0f),
    SITHFRAMERATEVEC_IMPCASE("PorRecoversToZero",  SITHPLAYER_IMPFIRE_OFF, 0, 1, 1.0f, 10.0f, 0.0f, 0u, 0.0f),
    SITHFRAMERATEVEC_IMPCASE("PorImp2Firing",      SITHWEAPON_IMP2, 0, 1, 1.0f, 100.0f, 0.0f, 0u, 130.0f),
    SITHFRAMERATEVEC_IMPCASE("PorImp5Overheated",  SITHWEAPON_IMP5, 0, 1, 1.0f, 180.0f, 40.0f, 0u, 180.0f),
};

/**
 * Result of a case:
 * | Kind       | total             | detail                                     | state                                |
 * | ---------- | ----------------- | ------------------------------------------ | ------------------------------------ |
 * | COG        | wakes or pulses   | interval in msec (sleep: first deadline)   | last deadline                        |
 * | Timer loop | timer messages    | first deadline minus the game time         | last deadline                        |
 * | Event loop | events            | first event time minus the game time       | next event time                      |
 * | AI tick    | instinct runs     | next update minus the game time (1st tick) | last next update of the AI           |
 * | Weapon refire | shots          | first fire wait end minus the game time    | last fire wait end (float bits)      |
 * | Weapon wait | msec until the wait is over | the wait end minus the game time | the wait end (float bits)            |
 * | Item touch | touches           | first touch deadline minus the game time   | last touch deadline                  |
 * | Surfacing  | gasp sounds       | breath sounds                              | msecUnderwater at the end            |
 * | Gurgle, noise, exhaust | sounds / events / puffs | 0                            | 0                                    |
 * | Path blocked | blocked messages | game time of the first message             | blocked move count                   |
 * | Path move, rotate | game time of the arrival | frames until it                | rotation fraction bits / blocked moves |
 * | Damage     | damage points     | frames that dealt damage                   | msecUnderwater or IMP state bits     |
 */
typedef struct sSithFrameRateVecResult
{
    uint32_t total;
    uint32_t detail;
    uint32_t state;
} SithFrameRateVecResult;

static SithThing sithFrameRateVec_thing;
static SithSector sithFrameRateVec_sector;
static SithPlayer sithFrameRateVec_player;
static SithCog sithFrameRateVec_cog;
static SithCogScript sithFrameRateVec_cogScript;

static int sithFrameRateVec_difficulty;
static float sithFrameRateVec_impScalar;
static float sithFrameRateVec_secCogInterval;
static SithDamageType sithFrameRateVec_expectedDamageType;

static int32_t sithFrameRateVec_damageTotal;
static size_t sithFrameRateVec_numDamageFrames;
static size_t sithFrameRateVec_numBreathSounds;
static size_t sithFrameRateVec_numGaspSounds;
static size_t sithFrameRateVec_numDrownSounds;
static size_t sithFrameRateVec_numAwarenessEvents;
static size_t sithFrameRateVec_numBlockedMessages;
static size_t sithFrameRateVec_numArrivedMessages;
static size_t sithFrameRateVec_numSmokes;
static SithThing sithFrameRateVec_smokeTemplate;
static SithThing sithFrameRateVec_smoke;
static SithWorld sithFrameRateVec_world;
static size_t sithFrameRateVec_numCogWakes;

static SithAIControlBlock sithFrameRateVec_aiLocal;
static SithAIClass sithFrameRateVec_aiClass;
static tStdTime sithFrameRateVec_msecAIInterval;
static size_t sithFrameRateVec_numAITicks;
static tStdTime sithFrameRateVec_msecEventInterval;
static size_t sithFrameRateVec_numEvents;
static bool sithFrameRateVec_bEventStarted;

static SithThing sithFrameRateVec_item;
static SithInventoryType sithFrameRateVec_weaponType;

// Stubs of the engine functions the tested code calls: sithFrameRateTestSeams.h and sithCogTimingTestSeams.h rename the calls
// in the tests, the harness patches the original functions to jump to them

float J3DAPI sithFrameRateVec_DamageThing(SithThing* pThing, const SithThing* pPurpetrator, float damage, SithDamageType hitType)
{
    SITHFRAMERATEVEC_CHECK(pThing == &sithFrameRateVec_thing);
    SITHFRAMERATEVEC_CHECK(pPurpetrator == &sithFrameRateVec_thing);
    SITHFRAMERATEVEC_CHECK(hitType == sithFrameRateVec_expectedDamageType);

    // Like sithThing_DamageThing: ignore damage <= 0 and pass only the whole damage points to the COG damage handler
    if ( damage <= 0.0f )
    {
        return 0.0f;
    }

    const int32_t points = (int32_t)damage;
    if ( points > 0 )
    {
        sithFrameRateVec_damageTotal += points;
        ++sithFrameRateVec_numDamageFrames;
    }

    return (float)points;
}

int sithFrameRateVec_GetGameDifficulty(void)
{
    return sithFrameRateVec_difficulty;
}

float sithFrameRateVec_GetIMPDamageScalar(void)
{
    return sithFrameRateVec_impScalar;
}

tSoundChannelHandle J3DAPI sithFrameRateVec_PlayModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    SITHFRAMERATEVEC_CHECK(pThing == &sithFrameRateVec_thing);
    if ( mode == SITHSOUNDCLASS_BREATH )
    {
        ++sithFrameRateVec_numBreathSounds;
    }
    else if ( mode == SITHSOUNDCLASS_GASP )
    {
        ++sithFrameRateVec_numGaspSounds;
    }
    else if ( mode == SITHSOUNDCLASS_DROWNING )
    {
        ++sithFrameRateVec_numDrownSounds;
    }

    return 0;
}

void J3DAPI sithFrameRateVec_UpdateLipSync(SithThing* pThing)
{
    SITHFRAMERATEVEC_CHECK(pThing == &sithFrameRateVec_thing);
}

void J3DAPI sithFrameRateVec_UpdateActorWeapon(SithThing* pThing)
{
    SITHFRAMERATEVEC_CHECK(pThing == &sithFrameRateVec_thing);
}

float J3DAPI sithFrameRateVec_PopFlex(SithCog* pCog)
{
    SITHFRAMERATEVEC_CHECK(pCog == &sithFrameRateVec_cog);
    return sithFrameRateVec_secCogInterval;
}

SithThing* J3DAPI sithFrameRateVec_PopThing(SithCog* pCog)
{
    SITHFRAMERATEVEC_CHECK(pCog == &sithFrameRateVec_cog);
    return &sithFrameRateVec_thing;
}

void J3DAPI sithFrameRateVec_Execute(SithCog* pCog)
{
    // The script of a sleep loop: it woke up and sleeps again
    SITHFRAMERATEVEC_CHECK(pCog == &sithFrameRateVec_cog);
    ++sithFrameRateVec_numCogWakes;
    SITHFRAMERATEVEC_CALL(sithCogFunction_Sleep, pCog);
}

#ifdef sithWeapon_UpdateActorWeapon
#  undef sithWeapon_UpdateActorWeapon // sithFrameRateTestSeams.h stubs it for sithPlayer.c; the weapon cases run the real one
#  ifdef sithEngineSuite_RENAMES_H
     // In sithTests the engine tests' copy of sithWeapon.c is renamed (sithEngineSuiteRenames.h)
#    define sithWeapon_UpdateActorWeapon sithEngineSuite_sithWeapon_UpdateActorWeapon
#  endif
void J3DAPI sithWeapon_UpdateActorWeapon(SithThing* pThing); // sithWeapon.h declared it under the stub's name
#endif

// The weapon of the refire case: a weapon type with a script (without handlers), so sithWeapon_UpdateActorWeapon fires it
SithInventoryType* J3DAPI sithFrameRateVec_GetInventoryType(size_t typeId)
{
    SITHFRAMERATEVEC_CHECK(typeId == (size_t)sithFrameRateVec_thing.thingInfo.actorInfo.weaponInfo.curWeaponID);
    return &sithFrameRateVec_weaponType;
}

// The item is always in the player's line of sight
int J3DAPI sithFrameRateVec_CheckThingLOS(const SithThing* pThing, const SithThing* pTarget, int bFlag)
{
    SITHFRAMERATEVEC_CHECK(pThing == &sithFrameRateVec_thing);
    SITHFRAMERATEVEC_CHECK(pTarget == &sithFrameRateVec_item);
    SITHFRAMERATEVEC_CHECK(bFlag == 0);
    return 1;
}

// The AI awareness events of the weapon noise case
int J3DAPI sithFrameRateVec_CreateTransmittingEvent(SithSector* pSector, const rdVector3* pPos, int type, float level, SithThing* pThing)
{
    SITHFRAMERATEVEC_CHECK(pSector == &sithFrameRateVec_sector);
    SITHFRAMERATEVEC_CHECK(pPos == &sithFrameRateVec_thing.pos);
    SITHFRAMERATEVEC_CHECK(type == 2 && level == 0.5f);
    SITHFRAMERATEVEC_CHECK(pThing == &sithFrameRateVec_thing);
    ++sithFrameRateVec_numAwarenessEvents;
    return 1;
}

// The blocked messages of the path mover (the COG and thing pulse cases send their pulses through it too, to no handler)
int J3DAPI sithFrameRateVec_ThingSendMessage(const SithThing* pThing, const SithThing* pSrcThing, SithCogMsgType msgType)
{
    SITHFRAMERATEVEC_CHECK(pThing == &sithFrameRateVec_thing);
    if ( msgType == SITHCOG_MSG_BLOCKED )
    {
        SITHFRAMERATEVEC_CHECK(pSrcThing == NULL);
        ++sithFrameRateVec_numBlockedMessages;
    }
    else if ( msgType == SITHCOG_MSG_ARRIVED )
    {
        SITHFRAMERATEVEC_CHECK(pSrcThing == NULL);
        ++sithFrameRateVec_numArrivedMessages;
    }

    return 0;
}

// The exhaust smoke of the vehicle: the smoke template and the created smoke things (one static thing stands in for all)
SithThing* J3DAPI sithFrameRateVec_GetTemplate(const char* pName)
{
    SITHFRAMERATEVEC_CHECK(strcmp(pName, "smoke") == 0);
    return &sithFrameRateVec_smokeTemplate;
}

SithThing* J3DAPI sithFrameRateVec_CreateThingAtPos(const SithThing* pTemplate, const rdVector3* pPos, const rdMatrix34* pOrient, SithSector* pSector, SithThing* pParent)
{
    SITHFRAMERATEVEC_CHECK(pTemplate == &sithFrameRateVec_smokeTemplate);
    SITHFRAMERATEVEC_CHECK(pPos != NULL && pOrient == &sithFrameRateVec_thing.orient);
    SITHFRAMERATEVEC_CHECK(pSector == &sithFrameRateVec_sector && pParent == NULL);
    ++sithFrameRateVec_numSmokes;
    return &sithFrameRateVec_smoke;
}

SithAnimationSlot* J3DAPI sithFrameRateVec_StartAnimateSpriteSize(SithThing* pSprite, const rdVector3* pStart, const rdVector3* pEnd, float secDuration)
{
    SITHFRAMERATEVEC_CHECK(pSprite == &sithFrameRateVec_smoke);
    J3D_UNUSED(pStart);
    J3D_UNUSED(pEnd);
    J3D_UNUSED(secDuration);
    return NULL;
}

// The task of the event loop: the event creates itself again, like a script that calls SetTimerEx in its timer message
int J3DAPI sithFrameRateVec_EventTask(int msecTime, SithEventParams* pParams)
{
    SITHFRAMERATEVEC_CHECK(msecTime == 0);
    SITHFRAMERATEVEC_CHECK(pParams->idx == 1234);
    ++sithFrameRateVec_numEvents;
    if ( sithFrameRateVec_numEvents > 100000u )
    {
        return 1; // An event due again in the same frame would otherwise loop forever; the count fails the test
    }

    SithEventParams params = *pParams;
    SITHFRAMERATEVEC_CALL(sithEvent_CreateEvent, SITHCOG_TASKID, &params, sithFrameRateVec_msecEventInterval);
    return 1;
}

// The instinct of the AI tick: schedules its next update like the engine's instincts do, or keeps the loop's 1 sec default
int J3DAPI sithFrameRateVec_Instinct(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SITHFRAMERATEVEC_CHECK(pLocal == &sithFrameRateVec_aiLocal);
    SITHFRAMERATEVEC_CHECK(pInstinct == &sithFrameRateVec_aiClass.aInstincts[0]);
    SITHFRAMERATEVEC_CHECK(pState == &sithFrameRateVec_aiLocal.aInstinctStates[0]);
    SITHFRAMERATEVEC_CHECK(event == 0 && pObject == NULL);
    ++sithFrameRateVec_numAITicks;
    if ( sithFrameRateVec_msecAIInterval )
    {
        pState->msecNextUpdate = SITHFRAMERATEVEC_SCHEDULE(sithFrameRateVec_msecAIInterval);
    }

    return 0;
}

static size_t sithFrameRateVec_GetNumFrames(size_t fps)
{
    return fps * SITHFRAMERATEVEC_MSECDURATION / 1000u;
}

/**
 * Advances the simulated engine clock by one frame, like sithTime_Advance.
 * The frame time is in whole msec, so e.g. 144 FPS gives a mix of 6 and 7 msec frames that add up to 1000 msec per second.
 * Sets the engine frame rate, frame time, game time and frame number globals and returns the frame time in msec.
 */
static uint32_t sithFrameRateVec_AdvanceFrame(size_t fps, size_t frameNum)
{
    const uint32_t msecFrame = (uint32_t)(((frameNum + 1u) * 1000u / fps) - (frameNum * 1000u / fps));
    sithTime_g_frameTime      = msecFrame;
    sithTime_g_frameTimeFlex  = (float)msecFrame * 0.001f;
    sithTime_g_fps            = 1.0f / sithTime_g_frameTimeFlex;
    sithTime_g_msecGameTime  += msecFrame;
    sithTime_g_secGameTime    = (float)sithTime_g_msecGameTime * 0.001f;
    sithMain_g_frameNumber    = frameNum; // SITH_ISFRAMECYCLE counts frames
    return msecFrame;
}

// Resets the thing, its sector, the local player and the counters, and starts the game time at 0
static void sithFrameRateVec_InitThing(SithSectorFlag sectorFlags)
{
    memset(&sithFrameRateVec_thing, 0, sizeof(sithFrameRateVec_thing));
    memset(&sithFrameRateVec_sector, 0, sizeof(sithFrameRateVec_sector));
    memset(&sithFrameRateVec_player, 0, sizeof(sithFrameRateVec_player));

    sithFrameRateVec_sector.flags   = sectorFlags;
    sithFrameRateVec_thing.type     = SITH_THING_PLAYER;
    sithFrameRateVec_thing.pInSector = &sithFrameRateVec_sector;

    sithFrameRateVec_player.pThing = &sithFrameRateVec_thing;
    sithPlayer_g_pLocalPlayer      = &sithFrameRateVec_player;

    sithFrameRateVec_damageTotal     = 0;
    sithFrameRateVec_numDamageFrames = 0;
    sithFrameRateVec_numBreathSounds = 0;
    sithFrameRateVec_numGaspSounds   = 0;
    sithFrameRateVec_numCogWakes     = 0;
    sithFrameRateVec_numDrownSounds  = 0;
    sithFrameRateVec_numAwarenessEvents = 0;
    sithFrameRateVec_numBlockedMessages = 0;
    sithFrameRateVec_numArrivedMessages = 0;
    sithFrameRateVec_numSmokes       = 0;
    sithTime_g_msecGameTime          = 0;
    sithTime_g_secGameTime           = 0.0f;
}

static uint32_t sithFrameRateVec_GetFloatBits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float sithFrameRateVec_GetBitsFloat(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

// The script starts the sleep or pulse in the first frame, and the COG or thing is processed in every later frame
static SithFrameRateVecResult sithFrameRateVec_RunCog(const SithFrameRateVecCase* pCase, size_t fps)
{
    sithFrameRateVec_InitThing(0);
    memset(&sithFrameRateVec_cog, 0, sizeof(sithFrameRateVec_cog));
    memset(&sithFrameRateVec_cogScript, 0, sizeof(sithFrameRateVec_cogScript));
    sithFrameRateVec_cog.pScript    = &sithFrameRateVec_cogScript; // No message handlers, so the pulse messages reach nothing
    sithFrameRateVec_secCogInterval = pCase->secInterval;

    SithFrameRateVecResult result = { 0 };
    sithFrameRateVec_AdvanceFrame(fps, 0);
    switch ( pCase->kind )
    {
        case SITHFRAMERATEVEC_COG_PULSE:
            SITHFRAMERATEVEC_CALL(sithCogFunction_SetPulse, &sithFrameRateVec_cog);
            result.detail = sithFrameRateVec_cog.msecPulseInterval;
            break;
        case SITHFRAMERATEVEC_COG_SLEEPLOOP:
            SITHFRAMERATEVEC_CALL(sithCogFunction_Sleep, &sithFrameRateVec_cog);
            result.detail = (uint32_t)sithFrameRateVec_cog.statusParams[0] - sithTime_g_msecGameTime;
            break;
        case SITHFRAMERATEVEC_COG_THINGPULSE:
            SITHFRAMERATEVEC_CALL(sithCogFunctionThing_SetThingPulse, &sithFrameRateVec_cog);
            result.detail = sithFrameRateVec_thing.msecPulseInterval;
            break;
        case SITHFRAMERATEVEC_COG_TIMERLOOP:
            SITHFRAMERATEVEC_CALL(sithCogFunction_SetTimer, &sithFrameRateVec_cog);
            result.detail = sithFrameRateVec_cog.msecTimerTimeout - sithTime_g_msecGameTime;
            break;
        default:
            SITHFRAMERATEVEC_CALL(sithCogFunctionThing_SetThingTimer, &sithFrameRateVec_cog);
            result.detail = sithFrameRateVec_thing.msecTimerTime - sithTime_g_msecGameTime;
            break;
    }

    // A pulse fired when its next pulse time moved. A timer fired when its flag was cleared; the script sets it again in the
    // timer message, which the engine sends in the same frame.
    const bool bThing = pCase->kind == SITHFRAMERATEVEC_COG_THINGPULSE || pCase->kind == SITHFRAMERATEVEC_COG_THINGTIMERLOOP;
    uint32_t msecNextPulse = bThing ? sithFrameRateVec_thing.msecNextPulseTime : sithFrameRateVec_cog.msecNextPulseTime;
    size_t numTicks = 0;
    for ( size_t frameNum = 1; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        sithFrameRateVec_AdvanceFrame(fps, frameNum);
        if ( bThing )
        {
            SITHFRAMERATEVEC_CALL(sithCog_UpdateThingTimer, &sithFrameRateVec_thing);
        }
        else
        {
            SITHFRAMERATEVEC_CALL(sithCog_ProcessCog, &sithFrameRateVec_cog);
        }

        switch ( pCase->kind )
        {
            case SITHFRAMERATEVEC_COG_TIMERLOOP:
                if ( (sithFrameRateVec_cog.flags & SITHCOG_TIMER_SET) == 0 )
                {
                    ++numTicks;
                    SITHFRAMERATEVEC_CALL(sithCogFunction_SetTimer, &sithFrameRateVec_cog);
                }
                break;
            case SITHFRAMERATEVEC_COG_THINGTIMERLOOP:
                if ( (sithFrameRateVec_thing.flags & SITH_TF_TIMERSET) == 0 )
                {
                    ++numTicks;
                    SITHFRAMERATEVEC_CALL(sithCogFunctionThing_SetThingTimer, &sithFrameRateVec_cog);
                }
                break;
            default:
            {
                const uint32_t msecPulse = bThing ? sithFrameRateVec_thing.msecNextPulseTime : sithFrameRateVec_cog.msecNextPulseTime;
                if ( msecPulse != msecNextPulse )
                {
                    ++numTicks;
                    msecNextPulse = msecPulse;
                }
            } break;
        }
    }

    switch ( pCase->kind )
    {
        case SITHFRAMERATEVEC_COG_SLEEPLOOP:
            result.total = (uint32_t)sithFrameRateVec_numCogWakes;
            result.state = (uint32_t)sithFrameRateVec_cog.statusParams[0];
            break;
        case SITHFRAMERATEVEC_COG_TIMERLOOP:
            result.total = (uint32_t)numTicks;
            result.state = sithFrameRateVec_cog.msecTimerTimeout;
            break;
        case SITHFRAMERATEVEC_COG_THINGTIMERLOOP:
            result.total = (uint32_t)numTicks;
            result.state = sithFrameRateVec_thing.msecTimerTime;
            break;
        default:
            result.total = (uint32_t)numTicks;
            result.state = msecNextPulse;
            break;
    }

    return result;
}

// An event that creates itself again from its task, like a script calling SetTimerEx in its timer message
static SithFrameRateVecResult sithFrameRateVec_RunEvent(const SithFrameRateVecCase* pCase, size_t fps)
{
    sithFrameRateVec_InitThing(0);
    if ( !sithFrameRateVec_bEventStarted )
    {
        SITHFRAMERATEVEC_CALL(sithEvent_Startup);
        sithFrameRateVec_bEventStarted = true;
    }

    SITHFRAMERATEVEC_CALL(sithEvent_Reset);
    SITHFRAMERATEVEC_CALL(sithEvent_RegisterTask, SITHCOG_TASKID, sithFrameRateVec_EventTask, 0, SITHEVENT_TASKONDEMAND);
    sithFrameRateVec_msecEventInterval = (tStdTime)(pCase->secInterval * 1000.0f);
    sithFrameRateVec_numEvents         = 0;

    SithFrameRateVecResult result = { 0 };
    sithFrameRateVec_AdvanceFrame(fps, 0);
    SithEventParams params = { .idx = 1234 };
    SITHFRAMERATEVEC_CALL(sithEvent_CreateEvent, SITHCOG_TASKID, &params, sithFrameRateVec_msecEventInterval);
    result.detail = sithEvent_g_pFirstQueuedEvent->msecEventTime - sithTime_g_msecGameTime;

    for ( size_t frameNum = 1; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        sithFrameRateVec_AdvanceFrame(fps, frameNum);
        SITHFRAMERATEVEC_CALL(sithEvent_Process);
    }

    result.total = (uint32_t)sithFrameRateVec_numEvents;
    result.state = sithEvent_g_pFirstQueuedEvent ? sithEvent_g_pFirstQueuedEvent->msecEventTime : 0u;
    return result;
}

// An AI with one instinct, updated like sithAI_Update does: sithAI_InstinctUpdate when the AI's next update time is reached
static SithFrameRateVecResult sithFrameRateVec_RunAITick(const SithFrameRateVecCase* pCase, size_t fps)
{
    sithFrameRateVec_InitThing(0);
    memset(&sithFrameRateVec_aiLocal, 0, sizeof(sithFrameRateVec_aiLocal));
    memset(&sithFrameRateVec_aiClass, 0, sizeof(sithFrameRateVec_aiClass));
    sithFrameRateVec_aiClass.aInstincts[0].updateModes = SITHAI_MODE_ACTIVE;
    sithFrameRateVec_aiClass.aInstincts[0].pfInstinct  = sithFrameRateVec_Instinct;
    sithFrameRateVec_aiLocal.pOwner       = &sithFrameRateVec_thing;
    sithFrameRateVec_aiLocal.pClass       = &sithFrameRateVec_aiClass;
    sithFrameRateVec_aiLocal.mode         = SITHAI_MODE_ACTIVE;
    sithFrameRateVec_aiLocal.numInstincts = 1;
    sithFrameRateVec_msecAIInterval = (tStdTime)(pCase->secInterval * 1000.0f);
    sithFrameRateVec_numAITicks     = 0;

    SithFrameRateVecResult result = { 0 };
    for ( size_t frameNum = 0; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        sithFrameRateVec_AdvanceFrame(fps, frameNum);
        if ( sithFrameRateVec_aiLocal.msecNextUpdate <= sithTime_g_msecGameTime )
        {
            SITHFRAMERATEVEC_CALL(sithAI_InstinctUpdate, &sithFrameRateVec_aiLocal);
            if ( frameNum == 0 )
            {
                result.detail = sithFrameRateVec_aiLocal.aInstinctStates[0].msecNextUpdate - sithTime_g_msecGameTime;
            }
        }
    }

    result.total = (uint32_t)sithFrameRateVec_numAITicks;
    result.state = sithFrameRateVec_aiLocal.msecNextUpdate;
    return result;
}

// Runs the per-frame actor and player updates of a case
static SithFrameRateVecResult sithFrameRateVec_RunUpdate(const SithFrameRateVecCase* pCase, size_t fps)
{
    SithActorInfo* pActor = &sithFrameRateVec_thing.thingInfo.actorInfo;
    switch ( pCase->kind )
    {
        case SITHFRAMERATEVEC_SURFACING:
            sithFrameRateVec_InitThing(0);
            sithFrameRateVec_thing.idx                 = pCase->thingIdx;
            sithFrameRateVec_thing.moveInfo.physics.flags = SITH_PF_ONWATERSURFACE;
            pActor->endurance.msecUnderwater           = SITHACTOR_MAX_UNDERWATER_MSEC;
            break;
        case SITHFRAMERATEVEC_DROWNING:
            sithFrameRateVec_InitThing(SITH_SECTOR_UNDERWATER);
            pActor->endurance.msecUnderwater     = SITHACTOR_MAX_UNDERWATER_MSEC;
            sithFrameRateVec_expectedDamageType = SITH_DAMAGE_DROWN;
            break;
        case SITHFRAMERATEVEC_RAFTLEAK:
            sithFrameRateVec_InitThing(0);
            sithFrameRateVec_thing.moveInfo.physics.flags = SITH_PF_RAFT;
            pActor->endurance.raftLeakDamage    = 1; // Leaking
            sithFrameRateVec_difficulty         = pCase->difficulty;
            sithFrameRateVec_expectedDamageType = SITH_DAMAGE_RAFT_LEAK;
            break;
        default:
            sithFrameRateVec_InitThing(pCase->sectorFlags);
            sithPlayer_g_impState               = pCase->impState;
            sithPlayer_g_impFireType            = pCase->impFireType;
            sithPlayer_g_bPlayerInPor           = pCase->bInPor;
            sithFrameRateVec_impScalar          = pCase->impScalar;
            sithFrameRateVec_expectedDamageType = SITH_DAMAGE_IMP_BLAST;
            break;
    }

    for ( size_t frameNum = 0; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        const uint32_t msecFrame = sithFrameRateVec_AdvanceFrame(fps, frameNum);
        if ( pCase->kind == SITHFRAMERATEVEC_IMP )
        {
            SITHFRAMERATEVEC_CALL(sithPlayer_Update, &sithFrameRateVec_player, (float)msecFrame * 0.001f);
        }
        else
        {
            SITHFRAMERATEVEC_CALL(sithActor_Update, &sithFrameRateVec_thing, msecFrame);
        }
    }

    SithFrameRateVecResult result = { 0 };
    if ( pCase->kind == SITHFRAMERATEVEC_SURFACING )
    {
        result.total  = (uint32_t)sithFrameRateVec_numGaspSounds;
        result.detail = (uint32_t)sithFrameRateVec_numBreathSounds;
    }
    else
    {
        result.total  = (uint32_t)sithFrameRateVec_damageTotal;
        result.detail = (uint32_t)sithFrameRateVec_numDamageFrames;
    }

    result.state = pCase->kind == SITHFRAMERATEVEC_IMP ? sithFrameRateVec_GetFloatBits(sithPlayer_g_impState) : pActor->endurance.msecUnderwater;
    return result;
}

static uint32_t sithFrameRateVec_GetSecDeadlineMsec(float secDeadline)
{
    // The msec the deadline is from the game time, as the test vectors store msec
    return (uint32_t)((secDeadline - sithTime_g_secGameTime) * 1000.0f + 0.5f);
}

// A player or actor with the weapon waits over and no weapon selected or deselected, so the weapon update fires the weapon
static void sithFrameRateVec_InitWeaponThing(SithThingType type)
{
    sithFrameRateVec_InitThing(0);
    sithFrameRateVec_thing.type = type;
    SithActorWeaponInfo* pWeapon  = &sithFrameRateVec_thing.thingInfo.actorInfo.weaponInfo;
    pWeapon->curWeaponID          = SITHWEAPON_PISTOL;
    pWeapon->selectedWeaponID     = SITHWEAPON_NOWEAPONSELECTED;
    pWeapon->deselectedWeaponID   = SITHWEAPON_NOWEAPONDESELECTED;
    pWeapon->secSwapTime          = -1.0f; // SITHWEAPON_TIMENOTSET
    pWeapon->secLastRapidFireTime = -1.0f;

    memset(&sithFrameRateVec_cog, 0, sizeof(sithFrameRateVec_cog));
    memset(&sithFrameRateVec_cogScript, 0, sizeof(sithFrameRateVec_cogScript));
    memset(&sithFrameRateVec_weaponType, 0, sizeof(sithFrameRateVec_weaponType));
    sithFrameRateVec_cog.pScript     = &sithFrameRateVec_cogScript;
    sithFrameRateVec_weaponType.flags = SITHINVENTORY_TYPE_WEAPON;
    sithFrameRateVec_weaponType.pCog  = &sithFrameRateVec_cog; // The fire message reaches no handler
}

// An actor holding the fire key: sithWeapon_UpdateActorWeapon fires the weapon script again every fire wait
static SithFrameRateVecResult sithFrameRateVec_RunWeaponRefire(const SithFrameRateVecCase* pCase, size_t fps)
{
    sithFrameRateVec_InitWeaponThing(SITH_THING_ACTOR);
    SithActorWeaponInfo* pWeapon      = &sithFrameRateVec_thing.thingInfo.actorInfo.weaponInfo;
    pWeapon->secActivationWaitTime    = pCase->secInterval;
    pWeapon->secActivationWaitEndTime = 0.0f; // Fires in the first frame

    SithFrameRateVecResult result = { 0 };
    float secWaitEnd = pWeapon->secActivationWaitEndTime;
    for ( size_t frameNum = 0; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        sithFrameRateVec_AdvanceFrame(fps, frameNum);
        SITHFRAMERATEVEC_CALL(sithWeapon_UpdateActorWeapon, &sithFrameRateVec_thing);
        if ( pWeapon->secActivationWaitEndTime != secWaitEnd )
        {
            if ( result.total == 0 )
            {
                result.detail = sithFrameRateVec_GetSecDeadlineMsec(pWeapon->secActivationWaitEndTime);
            }

            ++result.total;
            secWaitEnd = pWeapon->secActivationWaitEndTime;
        }
    }

    result.state = sithFrameRateVec_GetFloatBits(secWaitEnd);
    return result;
}

// A weapon wait set in the first frame: how long sithWeapon_IsMountingWeapon reports the player waiting
static SithFrameRateVecResult sithFrameRateVec_RunWeaponWait(const SithFrameRateVecCase* pCase, size_t fps)
{
    sithFrameRateVec_InitWeaponThing(SITH_THING_PLAYER);
    SithActorWeaponInfo* pWeapon = &sithFrameRateVec_thing.thingInfo.actorInfo.weaponInfo;
    SITHFRAMERATEVEC_CALL(sithWeapon_SetMountWait, 0.0f); // No mount wait left from another case

    SithFrameRateVecResult result = { 0 };
    sithFrameRateVec_AdvanceFrame(fps, 0);
    float secWaitEnd = 0.0f;
    switch ( pCase->waitKind )
    {
        case SITHFRAMERATEVEC_WAIT_FIRE:
            SITHFRAMERATEVEC_CALL(sithWeapon_SetFireWait, &sithFrameRateVec_thing, pCase->secInterval);
            secWaitEnd = pWeapon->secActivationWaitEndTime;
            break;
        case SITHFRAMERATEVEC_WAIT_AIM:
            SITHFRAMERATEVEC_CALL(sithWeapon_SetAimWait, &sithFrameRateVec_thing, pCase->secInterval);
            secWaitEnd = pWeapon->secAimWaitEndTime;
            break;
        case SITHFRAMERATEVEC_WAIT_MOUNT:
            SITHFRAMERATEVEC_CALL(sithWeapon_SetMountWait, pCase->secInterval);
            secWaitEnd = SITHFRAMERATEVEC_CALL(sithWeapon_GetMountWait);
            break;
        default:
            SITHFRAMERATEVEC_CALL(sithWeapon_ActivateWeapon, &sithFrameRateVec_thing, &sithFrameRateVec_cog, pCase->secInterval);
            secWaitEnd = pWeapon->secActivationWaitEndTime;
            break;
    }

    result.detail = sithFrameRateVec_GetSecDeadlineMsec(secWaitEnd);
    result.state  = sithFrameRateVec_GetFloatBits(secWaitEnd);
    SITHFRAMERATEVEC_CHECK(SITHFRAMERATEVEC_CALL(sithWeapon_IsMountingWeapon, &sithFrameRateVec_thing) != 0);

    for ( size_t frameNum = 1; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        sithFrameRateVec_AdvanceFrame(fps, frameNum);
        if ( !SITHFRAMERATEVEC_CALL(sithWeapon_IsMountingWeapon, &sithFrameRateVec_thing) )
        {
            result.total = sithTime_g_msecGameTime; // The wait is over
            break;
        }
    }

    SITHFRAMERATEVEC_CALL(sithWeapon_SetMountWait, 0.0f);
    return result;
}

// The player standing on an item: the item's collision handler is called every frame
static SithFrameRateVecResult sithFrameRateVec_RunItemTouch(const SithFrameRateVecCase* pCase, size_t fps)
{
    J3D_UNUSED(pCase);
    sithFrameRateVec_InitThing(0);
    memset(&sithFrameRateVec_item, 0, sizeof(sithFrameRateVec_item));
    sithFrameRateVec_item.type = SITH_THING_ITEM;

    SithFrameRateVecResult result = { 0 };
    uint32_t msecTouch = 0;
    for ( size_t frameNum = 0; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        sithFrameRateVec_AdvanceFrame(fps, frameNum);
        SITHFRAMERATEVEC_CALL(sithItem_PlayerCollisionHandler, &sithFrameRateVec_item, &sithFrameRateVec_thing, NULL, 0);
        if ( sithFrameRateVec_item.thingInfo.itemInfo.msecLastTouchTime != msecTouch )
        {
            if ( result.total == 0 )
            {
                result.detail = sithFrameRateVec_item.thingInfo.itemInfo.msecLastTouchTime - sithTime_g_msecGameTime;
            }

            ++result.total;
            msecTouch = sithFrameRateVec_item.thingInfo.itemInfo.msecLastTouchTime;
        }
    }

    result.state = msecTouch;
    return result;
}

// Effects the original ran on every n-th frame, each called once per frame: the drowning sound of an actor that takes
// drowning damage every frame, the awareness events of a decaying weapon and the exhaust smoke of a vehicle; and a blocked path mover
static SithFrameRateVecResult sithFrameRateVec_RunEffect(const SithFrameRateVecCase* pCase, size_t fps)
{
    sithFrameRateVec_InitThing(0);
    SithFrameRateVecResult result = { 0 };

    SithVehicleExhaustInfo exhaust = { 0 };
    if ( pCase->kind == SITHFRAMERATEVEC_WEAPONNOISE )
    {
        sithFrameRateVec_thing.type = SITH_THING_WEAPON;
        sithFrameRateVec_thing.thingInfo.weaponInfo.flags = SITH_WF_DECAYEMITSOUNDAWARENESSEVENT;
    }
    else if ( pCase->kind == SITHFRAMERATEVEC_PATHBLOCKED )
    {
        // A mover on its way, linked to a COG, that can't move at all; the world holds the thing for the pointer check
        memset(&sithFrameRateVec_world, 0, sizeof(sithFrameRateVec_world));
        sithFrameRateVec_world.aThings   = &sithFrameRateVec_thing;
        sithFrameRateVec_world.numThings = 1;
        sithWorld_g_pCurrentWorld        = &sithFrameRateVec_world;
        sithFrameRateVec_thing.type      = SITH_THING_COG;
        sithFrameRateVec_thing.moveType  = SITH_MT_PATH;
        sithFrameRateVec_thing.flags     = SITH_TF_COGLINKED;
        sithFrameRateVec_thing.orient.rvec.x = 1.0f; // Identity, without rdMatrix in the harness
        sithFrameRateVec_thing.orient.lvec.y = 1.0f;
        sithFrameRateVec_thing.orient.uvec.z = 1.0f;
        SithPathMoveInfo* pPath   = &sithFrameRateVec_thing.moveInfo.pathMovement;
        pPath->mode               = SITH_PATHMOVE_MOVE;
        pPath->vecDeltaPos.y      = 1.0f;
        pPath->moveVel            = 1.0f;
        pPath->moveTimeRemaining  = 1000.0f;
        SITHFRAMERATEVEC_SET_MOVE_BLOCKED(true);
    }
    else if ( pCase->kind == SITHFRAMERATEVEC_EXHAUST )
    {
        // A vehicle at a steady speed with a small speed change per frame, so each spawn is one puff
        sithFrameRateVec_thing.type     = SITH_THING_ACTOR;
        sithFrameRateVec_thing.moveType = SITH_MT_PHYSICS;
        sithFrameRateVec_thing.orient.rvec.x = 1.0f; // Identity, without rdMatrix in the harness
        sithFrameRateVec_thing.orient.lvec.y = 1.0f;
        sithFrameRateVec_thing.orient.uvec.z = 1.0f;
        SithPhysicsInfo* pPhysics  = &sithFrameRateVec_thing.moveInfo.physics;
        pPhysics->velocity.y       = 2.0f;
        pPhysics->deltaVelocity.y  = 0.05f;
        pPhysics->maxVelocity      = 5.0f;
        memset(&sithFrameRateVec_smokeTemplate, 0, sizeof(sithFrameRateVec_smokeTemplate));
        memset(&sithFrameRateVec_smoke, 0, sizeof(sithFrameRateVec_smoke));
        exhaust.bEnabled   = 1;
        exhaust.spacing    = 0.1f;
        exhaust.posY       = -0.5f;
        exhaust.life       = 1.0f;
        exhaust.lifeFactor = 0.1f;
        exhaust.sizeStart  = 0.1f;
        exhaust.sizeEnd    = 0.3f;
        exhaust.alphaStart = 0.8f;
        exhaust.alphaEnd   = 0.0f;
        exhaust.dirZ       = 0.1f;
        exhaust.scatterX   = 0.1f;
        exhaust.scatterZ   = 0.3f;
        memcpy(exhaust.aTemplate, "smoke", sizeof("smoke"));
    }

    for ( size_t frameNum = 0; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        const uint32_t msecFrame = sithFrameRateVec_AdvanceFrame(fps, frameNum);
        switch ( pCase->kind )
        {
            case SITHFRAMERATEVEC_DROWNGURGLE:
                SITHFRAMERATEVEC_CALL(sithActor_PlayDamageSoundFx, &sithFrameRateVec_thing, SITH_DAMAGE_DROWN);
                break;
            case SITHFRAMERATEVEC_WEAPONNOISE:
                SITHFRAMERATEVEC_CALL(sithWeapon_Update, &sithFrameRateVec_thing, (float)msecFrame * 0.001f);
                break;
            case SITHFRAMERATEVEC_PATHBLOCKED:
                SITHFRAMERATEVEC_CALL(sithPathMove_Update, &sithFrameRateVec_thing, (float)msecFrame * 0.001f);
                if ( result.detail == 0 && sithFrameRateVec_numBlockedMessages )
                {
                    result.detail = sithTime_g_msecGameTime; // The first blocked message
                }
                break;
            default:
                SITHFRAMERATEVEC_CALL(sithPhysics_UpdateExhaustFx, &sithFrameRateVec_thing, &exhaust, (float)msecFrame * 0.001f);
                break;
        }
    }

    switch ( pCase->kind )
    {
        case SITHFRAMERATEVEC_DROWNGURGLE: result.total = (uint32_t)sithFrameRateVec_numDrownSounds; break;
        case SITHFRAMERATEVEC_WEAPONNOISE: result.total = (uint32_t)sithFrameRateVec_numAwarenessEvents; break;
        case SITHFRAMERATEVEC_PATHBLOCKED:
            result.total = (uint32_t)sithFrameRateVec_numBlockedMessages;
            result.state = (uint32_t)sithFrameRateVec_thing.moveInfo.pathMovement.numBlockedMoves;
            SITHFRAMERATEVEC_SET_MOVE_BLOCKED(false);
            sithWorld_g_pCurrentWorld = NULL;
            break;
        default: result.total = (uint32_t)sithFrameRateVec_numSmokes; break;
    }

    return result;
}

// A COG-linked path mover that moves one unit or rotates 90 degrees about its pivot: the game time at which it arrives
static SithFrameRateVecResult sithFrameRateVec_RunPathMover(const SithFrameRateVecCase* pCase, size_t fps)
{
    sithFrameRateVec_InitThing(0);
    memset(&sithFrameRateVec_world, 0, sizeof(sithFrameRateVec_world));
    sithFrameRateVec_world.aThings   = &sithFrameRateVec_thing;
    sithFrameRateVec_world.numThings = 1;
    sithWorld_g_pCurrentWorld        = &sithFrameRateVec_world;
    sithFrameRateVec_thing.type      = SITH_THING_COG;
    sithFrameRateVec_thing.moveType  = SITH_MT_PATH;
    sithFrameRateVec_thing.flags     = SITH_TF_COGLINKED;
    sithFrameRateVec_thing.orient.rvec.x = 1.0f;
    sithFrameRateVec_thing.orient.lvec.y = 1.0f;
    sithFrameRateVec_thing.orient.uvec.z = 1.0f;
    SITHFRAMERATEVEC_SET_MOVE_BLOCKED(false);

    SithPathMoveInfo* pPath = &sithFrameRateVec_thing.moveInfo.pathMovement;
    if ( pCase->kind == SITHFRAMERATEVEC_PATHMOVE )
    {
        pPath->mode              = SITH_PATHMOVE_MOVE;
        pPath->vecDeltaPos.y     = 1.0f;
        pPath->moveVel           = 1.0f / pCase->secInterval;
        pPath->moveTimeRemaining = pCase->secInterval; // one unit at that speed
    }
    else
    {
        const rdVector3 offset = { 0.0f, 0.0f, 0.0f };
        const rdVector3 pyr    = { 0.0f, 90.0f, 0.0f };
        SITHFRAMERATEVEC_CALL(sithPathMove_RotatePivot, &sithFrameRateVec_thing, &offset, &pyr, pCase->secInterval);
    }

    SithFrameRateVecResult result = { 0 };
    for ( size_t frameNum = 0; frameNum < sithFrameRateVec_GetNumFrames(fps) && !sithFrameRateVec_numArrivedMessages; ++frameNum )
    {
        const uint32_t msecFrame = sithFrameRateVec_AdvanceFrame(fps, frameNum);
        SITHFRAMERATEVEC_CALL(sithPathMove_Update, &sithFrameRateVec_thing, (float)msecFrame * 0.001f);
        result.detail = (uint32_t)frameNum + 1u;
    }

    result.total = sithFrameRateVec_numArrivedMessages ? sithTime_g_msecGameTime : 0u;
    result.state = pCase->kind == SITHFRAMERATEVEC_PATHMOVE ? (uint32_t)pPath->numBlockedMoves : sithFrameRateVec_GetFloatBits(pPath->rotDelta);
    sithWorld_g_pCurrentWorld = NULL;
    return result;
}

// Runs a case at a frame rate on the original functions (harness) or the reconstruction (tests)
static SithFrameRateVecResult sithFrameRateVec_Run(const SithFrameRateVecCase* pCase, size_t fps)
{
    if ( pCase->kind <= SITHFRAMERATEVEC_COG_THINGTIMERLOOP )
    {
        return sithFrameRateVec_RunCog(pCase, fps);
    }

    if ( pCase->kind == SITHFRAMERATEVEC_EVENTLOOP )
    {
        return sithFrameRateVec_RunEvent(pCase, fps);
    }

    if ( pCase->kind == SITHFRAMERATEVEC_AITICK )
    {
        return sithFrameRateVec_RunAITick(pCase, fps);
    }

    if ( pCase->kind == SITHFRAMERATEVEC_WEAPONREFIRE )
    {
        return sithFrameRateVec_RunWeaponRefire(pCase, fps);
    }

    if ( pCase->kind == SITHFRAMERATEVEC_WEAPONWAIT )
    {
        return sithFrameRateVec_RunWeaponWait(pCase, fps);
    }

    if ( pCase->kind == SITHFRAMERATEVEC_ITEMTOUCH )
    {
        return sithFrameRateVec_RunItemTouch(pCase, fps);
    }

    if ( pCase->kind >= SITHFRAMERATEVEC_DROWNGURGLE && pCase->kind <= SITHFRAMERATEVEC_EXHAUST )
    {
        return sithFrameRateVec_RunEffect(pCase, fps);
    }

    if ( pCase->kind == SITHFRAMERATEVEC_PATHMOVE || pCase->kind == SITHFRAMERATEVEC_PATHROTATE )
    {
        return sithFrameRateVec_RunPathMover(pCase, fps);
    }

    return sithFrameRateVec_RunUpdate(pCase, fps);
}

#endif // SITH_FRAMERATE_VEC_H
