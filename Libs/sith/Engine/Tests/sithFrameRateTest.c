#include <unity_fixture.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Same seams as the tested sithActor.c and sithPlayer.c, so sithThing_DamageThingAtRate deals damage through the stub
#include "sithFrameRateTestSeams.h"

#include <rdroid/Math/rdMatrix.h>

#include <sith/AI/sithAIMove.h>
#include <sith/Cog/sithCog.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithThing.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

#include "sithEngineTest.h"

// The cases, frame model and stubs shared with the harness that captured the original-engine vectors
#define SITHFRAMERATEVEC_CHECK(cond)     TEST_ASSERT_TRUE(cond)
#define SITHFRAMERATEVEC_CALL(func, ...) func(__VA_ARGS__)
#define SITHFRAMERATEVEC_SCHEDULE(msec)  sithTime_CalcDeadline(msec)
extern float sithPathMoveTest_moveReturnScale; // sithPathMoveTest.c's sithCollision_MoveThing stub
#define SITHFRAMERATEVEC_SET_MOVE_BLOCKED(b) (sithPathMoveTest_moveReturnScale = (b) ? 0.0f : 1.0f)
#include "sithFrameRateVec.h"

// Frame-rate tests: simulate the same time at different frame rates and compare the totals.
// The original engine scaled some controls and per-frame damage by the frame rate, see issues #10 and #48.
// The COG timing, surfacing and damage cases also compare with vectors of the original functions in the retail executable,
// captured with the cases of sithFrameRateVec.h (tv/sithFrameRate/README.md).

// Storage for the engine globals redirected in sithEngineTestAddrs.h
float sithEngineTest_fps;
SithPlayer* sithEngineTest_pLocalPlayer;
int sithEngineTest_impFireType;
float sithEngineTest_impState;
int sithEngineTest_bPlayerInPor;
uint32_t sithEngineTest_msecGameTime;
float sithEngineTest_secGameTime;
SithEvent* sithEngineTest_pFirstQueuedEvent;
uint32_t sithEngineTest_msecFrameTime;
float sithEngineTest_frameTimeFlex;
size_t sithEngineTest_frameNumber;

// The designed 30 FPS, common monitor refresh rates and uncapped rates
static const size_t sithFrameRateTest_aFps[] = { 30u, 60u, 144u, 300u, 1000u };

#define SITHFRAMERATETEST_MAXROTVELOCITY 70.0f // Indy's actor maxrotthrust in ijim.tpl
#define SITHFRAMERATETEST_DESIGNFPS      30u   // The frame rate the game was made for

// Original-engine vectors

#define SITHFRAMERATETEST_MAXVECTORS 2048u

typedef struct sSithFrameRateTestVector
{
    char aCase[32];
    size_t fps;
    SithFrameRateVecResult result;
} SithFrameRateTestVector;

static SithFrameRateTestVector sithFrameRateTest_aVectors[SITHFRAMERATETEST_MAXVECTORS];
static size_t sithFrameRateTest_numVectors;

// Reads tv/sithFrameRate/sithFrameRate.tv once: a "<case> <fps> <total> <detail> <state>" line per vector, '#' lines are comments
static void sithFrameRateTest_LoadVectors(void)
{
    if ( sithFrameRateTest_numVectors )
    {
        return;
    }

    char aPath[512];
    snprintf(aPath, sizeof(aPath), "%s/%s", SITH_FRAMERATE_TV_DIR, "sithFrameRate.tv");
    FILE* pFile = fopen(aPath, "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(pFile, aPath);

    char aLine[256];
    bool bValid = true;
    size_t numVectors = 0;
    while ( bValid && fgets(aLine, sizeof(aLine), pFile) )
    {
        if ( aLine[0] == '#' || aLine[0] == '\n' || aLine[0] == '\r' )
        {
            continue;
        }

        bValid = numVectors < SITHFRAMERATETEST_MAXVECTORS;
        if ( !bValid )
        {
            break;
        }

        // The case name, then the numbers; base 0 reads the hex IMP state bits
        SithFrameRateTestVector* pVector = &sithFrameRateTest_aVectors[numVectors];
        const size_t nameLen = strcspn(aLine, " \t");
        bValid = nameLen > 0 && nameLen < sizeof(pVector->aCase);
        if ( bValid )
        {
            memcpy(pVector->aCase, aLine, nameLen);
            pVector->aCase[nameLen] = '\0';

            char* pEnd = aLine + nameLen;
            unsigned long aValues[4];
            for ( size_t i = 0; bValid && i < STD_ARRAYLEN(aValues); ++i )
            {
                char* pNum = pEnd;
                aValues[i] = strtoul(pNum, &pEnd, 0);
                bValid = pEnd != pNum;
            }

            pVector->fps           = (size_t)aValues[0];
            pVector->result.total  = (uint32_t)aValues[1];
            pVector->result.detail = (uint32_t)aValues[2];
            pVector->result.state  = (uint32_t)aValues[3];
            ++numVectors;
        }
    }

    fclose(pFile);
    TEST_ASSERT_TRUE_MESSAGE(bValid, aLine);
    sithFrameRateTest_numVectors = numVectors;
}

// Returns the original-engine result of a case at a frame rate
static const SithFrameRateVecResult* sithFrameRateTest_GetVector(const SithFrameRateVecCase* pCase, size_t fps)
{
    sithFrameRateTest_LoadVectors();
    const SithFrameRateVecResult* pResult = NULL;
    for ( size_t i = 0; !pResult && i < sithFrameRateTest_numVectors; ++i )
    {
        if ( sithFrameRateTest_aVectors[i].fps == fps && strcmp(sithFrameRateTest_aVectors[i].aCase, pCase->pName) == 0 )
        {
            pResult = &sithFrameRateTest_aVectors[i].result;
        }
    }

    TEST_ASSERT_NOT_NULL_MESSAGE(pResult, pCase->pName);
    return pResult;
}

// COG sleeps, pulses and timers, events, AI ticks, weapon refire and waits, item touches: scheduled on the game clock and
// checked once per frame
static bool sithFrameRateTest_IsTimingCase(const SithFrameRateVecCase* pCase)
{
    return pCase->kind <= SITHFRAMERATEVEC_ITEMTOUCH;
}

// Effects the original ran on every n-th frame: surfacing sounds, drowning sound, weapon noise, blocked messages, exhaust smoke
static bool sithFrameRateTest_IsEffectCase(const SithFrameRateVecCase* pCase)
{
    return pCase->kind >= SITHFRAMERATEVEC_SURFACING && pCase->kind <= SITHFRAMERATEVEC_EXHAUST;
}

// Effects the original ran every frame (a cycle of one reference frame in QOL builds)
static bool sithFrameRateTest_IsPerFrameEffect(const SithFrameRateVecCase* pCase)
{
    return pCase->kind == SITHFRAMERATEVEC_PATHBLOCKED || pCase->kind == SITHFRAMERATEVEC_EXHAUST;
}

static bool sithFrameRateTest_IsDamageCase(const SithFrameRateVecCase* pCase)
{
    return pCase->kind >= SITHFRAMERATEVEC_DROWNING && pCase->damagePerSec > 0.0f;
}

/**
 * Returns whether a case runs the original code at a frame rate in this build.
 * QOL builds (speedrun included) time COG sleeps, pulses and timers, events and AI ticks as at 30 FPS while frames are shorter
 * than a 30 FPS frame, and the surfacing sounds by game time. Non-speedrun builds deal the damage at its rate. The IMP state code is the original in all builds.
 */
static bool sithFrameRateTest_RunsOriginalCode(const SithFrameRateVecCase* pCase, size_t fps)
{
    if ( sithFrameRateTest_IsTimingCase(pCase) )
    {
    #ifdef J3D_QOL_IMPROVEMENTS
        return fps <= SITHFRAMERATETEST_DESIGNFPS;
    #else
        J3D_UNUSED(fps);
        return true;
    #endif
    }

    if ( sithFrameRateTest_IsEffectCase(pCase) )
    {
    #ifdef J3D_QOL_IMPROVEMENTS
        // The once-per-frame effects (exhaust, blocked messages) run every frame at 30 FPS and below, like the original
        return sithFrameRateTest_IsPerFrameEffect(pCase) && fps <= SITHFRAMERATETEST_DESIGNFPS;
    #else
        return true;
    #endif
    }

    if ( sithFrameRateTest_IsDamageCase(pCase) )
    {
    #ifdef J3D_SPEEDRUN_BUILD
        return true;
    #else
        return false;
    #endif
    }

    return true;
}

static void sithFrameRateTest_GetCaseLabel(const SithFrameRateVecCase* pCase, size_t fps, char* pLabel, size_t size)
{
    snprintf(pLabel, size, "%s at %u FPS", pCase->pName, (unsigned int)fps);
}

// Thing and player of the control tests: Indy's max rotation velocity
static void sithFrameRateTest_InitPlayer(void)
{
    sithFrameRateVec_InitThing(0);
    sithFrameRateVec_thing.thingInfo.actorInfo.maxRotVelocity = SITHFRAMERATETEST_MAXROTVELOCITY;
}

// Returns how often SITHTIME_ISFRAMECYCLE(offset, n) is true in the simulated time
static size_t sithFrameRateTest_RunTimeCycle(size_t fps, uint32_t offset, uint32_t n)
{
    sithEngineTest_msecGameTime = 0;
    size_t numCycles = 0;
    for ( size_t frameNum = 0; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        sithFrameRateVec_AdvanceFrame(fps, frameNum);
        if ( SITHTIME_ISFRAMECYCLE(offset, n) )
        {
            ++numCycles;
        }
    }

    return numCycles;
}

// Returns the yaw in degrees a held turn key turns the player in the simulated time
static float sithFrameRateTest_RunKeyTurn(size_t fps)
{
    sithFrameRateTest_InitPlayer();
    float yaw = 0.0f;
    for ( size_t frameNum = 0; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        uint32_t msecFrame = sithFrameRateVec_AdvanceFrame(fps, frameNum);
        float angularVelocity = sithPlayerControls_CalculateAngularVelocity(&sithFrameRateVec_thing.thingInfo.actorInfo, 1.0f, 1.0f, 1.0f);
        yaw += angularVelocity * (float)msecFrame * 0.001f;
    }

    return yaw;
}

// Returns the time in msec a held jewel fly up key takes to reach full thrust
static uint32_t sithFrameRateTest_RunJewelFlyThrust(size_t fps)
{
    float thrust = 0.0f;
    uint32_t msecElapsed = 0;
    for ( size_t frameNum = 0; thrust < 1.0f; ++frameNum )
    {
        TEST_ASSERT_TRUE_MESSAGE(msecElapsed < 60000u, "Jewel fly thrust didn't reach full thrust in 60 sec");
        uint32_t msecFrame = sithFrameRateVec_AdvanceFrame(fps, frameNum);
        thrust = STDMATH_CLAMP(thrust + sithPlayerControls_CalculateJewelFlyThrustStep((float)msecFrame * 0.001f), -1.0f, 1.0f);
        msecElapsed += msecFrame;
    }

    return msecElapsed;
}

// Returns the yaw in degrees a thing turns in the simulated time when it is rotated by rate * frame time every frame,
// like sithAIMove_UpdateTurn and sithPhysics turn things
static float sithFrameRateTest_RunRotation(size_t fps, float degreesPerSec)
{
    rdMatrix34 orient;
    rdMatrix_Identity34(&orient);

    for ( size_t frameNum = 0; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
    {
        uint32_t msecFrame = sithFrameRateVec_AdvanceFrame(fps, frameNum);
        rdVector3 pyr = { .pitch = 0.0f, .yaw = degreesPerSec * (float)msecFrame * 0.001f, .roll = 0.0f };
        rdMatrix_PreRotate34(&orient, &pyr);
        rdMatrix_Normalize34(&orient);
    }

    // A yaw rotation turns the look vector (0, 1, 0) to (-sin(yaw), cos(yaw), 0)
    return atan2f(-orient.lvec.x, orient.lvec.y) * 180.0f / 3.14159265f;
}

#define SITHFRAMERATETEST_AITURN_MAXMSEC 20000u // Stop a stalled AI turn

/**
 * Returns the msec an AI actor takes to turn to a look direction with the real sithAIMove_UpdateTurn,
 * like chute_indy after AISetLookThing in the 02_riv parachute cutscene. Returns SITHFRAMERATETEST_AITURN_MAXMSEC if it stalls.
 * @param angleToGoal - Angle from the actor's look direction to the goal in degrees, positive to the right
 */
static uint32_t sithFrameRateTest_RunAITurn(size_t fps, float maxRotVelocity, float angleToGoal)
{
    sithFrameRateVec_InitThing(0);
    sithFrameRateVec_thing.type = SITH_THING_ACTOR;
    sithFrameRateVec_thing.moveStatus = SITHPLAYERMOVE_STILL;
    sithFrameRateVec_thing.thingInfo.actorInfo.maxRotVelocity = maxRotVelocity;
    rdMatrix_Identity34(&sithFrameRateVec_thing.orient);

    // The goal is set with double precision math, independent of stdMath_SinCos
    const double radians = (double)angleToGoal * 3.14159265358979323846 / 180.0;
    SithAIControlBlock local;
    memset(&local, 0, sizeof(local));
    local.pOwner     = &sithFrameRateVec_thing;
    local.mode       = SITHAI_MODE_TURNING;
    local.goalLVec.x = (float)sin(radians);
    local.goalLVec.y = (float)cos(radians);

    uint32_t msecElapsed = 0;
    for ( size_t frameNum = 0; (local.mode & SITHAI_MODE_TURNING) != 0 && msecElapsed < SITHFRAMERATETEST_AITURN_MAXMSEC; ++frameNum )
    {
        uint32_t msecFrame = sithFrameRateVec_AdvanceFrame(fps, frameNum);
        sithAIMove_UpdateTurn(&local, (float)msecFrame * 0.001f);
        msecElapsed += msecFrame;
    }

    if ( msecElapsed < SITHFRAMERATETEST_AITURN_MAXMSEC )
    {
        // The last step snaps the look direction to the goal
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, local.goalLVec.x, sithFrameRateVec_thing.orient.lvec.x);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, local.goalLVec.y, sithFrameRateVec_thing.orient.lvec.y);
    }

    return msecElapsed;
}

#define SITHFRAMERATETEST_HEADTRACK_MAXMSEC 5000u // Stop a head that never reaches the look position

/**
 * Returns the msec an AI actor that can't move its body (BLOCK mode) takes to turn its head to a look position
 * with the real sithAIMove_UpdateHeadTrackLookPos, and the head yaw when it's done.
 * @param angleToLook - Angle from the actor's look direction to the look position in degrees
 */
static uint32_t sithFrameRateTest_RunAIHeadTrack(size_t fps, float angleToLook, float* pHeadYaw)
{
    sithFrameRateVec_InitThing(0);
    sithFrameRateVec_thing.type = SITH_THING_ACTOR;
    rdMatrix_Identity34(&sithFrameRateVec_thing.orient);
    SithActorInfo* pActor   = &sithFrameRateVec_thing.thingInfo.actorInfo;
    pActor->flags           = SITH_AF_CANROTATEHEAD;
    pActor->maxHeadVelocity = 100.0f; // maxheadvel of most actor templates in ijim.tpl
    pActor->maxHeadYaw      = 80.0f;
    pActor->minHeadPitch    = -60.0f;
    pActor->maxHeadPitch    = 60.0f;

    // Look position 2 units away at eye height, set with double precision math
    const double radians = (double)angleToLook * 3.14159265358979323846 / 180.0;
    SithAIControlBlock local;
    memset(&local, 0, sizeof(local));
    local.pOwner    = &sithFrameRateVec_thing;
    local.mode      = SITHAI_MODE_LOOKING | SITHAI_MODE_BLOCK;
    local.lookPos.x = (float)(2.0 * sin(radians));
    local.lookPos.y = (float)(2.0 * cos(radians));

    uint32_t msecElapsed = 0;
    for ( size_t frameNum = 0; (local.mode & SITHAI_MODE_LOOKING) != 0 && msecElapsed < SITHFRAMERATETEST_HEADTRACK_MAXMSEC; ++frameNum )
    {
        uint32_t msecFrame = sithFrameRateVec_AdvanceFrame(fps, frameNum);
        sithAIMove_UpdateHeadTrackLookPos(&local, (float)msecFrame * 0.001f);
        msecElapsed += msecFrame;
    }

    *pHeadYaw = pActor->headPYR.y;
    return msecElapsed;
}

TEST_GROUP(sithFrameRate);
TEST_SETUP(sithFrameRate)
{
    SithEngineTest_Startup();
    sithEngineTest_fps          = 0.0f;
    sithEngineTest_pLocalPlayer = NULL;
    sithEngineTest_impFireType  = SITHPLAYER_IMPFIRE_OFF;
    sithEngineTest_impState     = 0.0f;
    sithEngineTest_bPlayerInPor = 0;
    sithEngineTest_msecGameTime  = 0;
    sithEngineTest_secGameTime   = 0.0f;
    sithEngineTest_msecFrameTime = 0;
    sithEngineTest_frameTimeFlex = 0.0f;
    sithEngineTest_frameNumber   = 0;
    sithFrameRateVec_numBreathSounds = 0;
    sithFrameRateVec_numGaspSounds   = 0;
    sithFrameRateVec_difficulty = 0;
    sithFrameRateVec_impScalar  = 1.0f;
}

TEST_TEAR_DOWN(sithFrameRate)
{
    sithEngineTest_pLocalPlayer = NULL;
    SithEngineTest_Shutdown();
}

TEST(sithFrameRate, TestFramesAddUpToTheSimulatedTime)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateVec_aFps); ++i )
    {
        size_t fps = sithFrameRateVec_aFps[i];
        uint32_t msecTotal = 0;
        for ( size_t frameNum = 0; frameNum < sithFrameRateVec_GetNumFrames(fps); ++frameNum )
        {
            uint32_t msecFrame = sithFrameRateVec_AdvanceFrame(fps, frameNum);
            TEST_ASSERT_TRUE(msecFrame == 1000u / fps || msecFrame == 1000u / fps + 1u);
            msecTotal += msecFrame;
        }

        TEST_ASSERT_EQUAL_UINT32(SITHFRAMERATEVEC_MSECDURATION, msecTotal);
    }
}

TEST(sithFrameRate, TestVectorsCoverEveryCaseAndFrameRate)
{
    // The fixture holds a vector for every case of sithFrameRateVec.h at every frame rate of the capture, and nothing else
    sithFrameRateTest_LoadVectors();
    TEST_ASSERT_EQUAL_size_t(STD_ARRAYLEN(sithFrameRateVec_aCases) * STD_ARRAYLEN(sithFrameRateVec_aFps), sithFrameRateTest_numVectors);
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateVec_aCases); ++i )
    {
        for ( size_t j = 0; j < STD_ARRAYLEN(sithFrameRateVec_aFps); ++j )
        {
            TEST_ASSERT_NOT_NULL(sithFrameRateTest_GetVector(&sithFrameRateVec_aCases[i], sithFrameRateVec_aFps[j]));
        }
    }
}

TEST(sithFrameRate, TestOriginalCodeMatchesTheRetailVectors)
{
    // Every case and frame rate that runs the original code in this build reproduces the original functions exactly:
    // the ticks, intervals and deadlines of sleeps and pulses, the surfacing sounds, and the damage and IMP state.
    // E.g. SetPulse(0.01) fires 89 times in 3 sec at 30 FPS and 299 times at 1000 FPS, drowning deals 540 damage at 30 FPS
    // and none at 300 FPS.
    size_t numMatched = 0;
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateVec_aCases); ++i )
    {
        const SithFrameRateVecCase* pCase = &sithFrameRateVec_aCases[i];
        for ( size_t j = 0; j < STD_ARRAYLEN(sithFrameRateVec_aFps); ++j )
        {
            const size_t fps = sithFrameRateVec_aFps[j];
            if ( !sithFrameRateTest_RunsOriginalCode(pCase, fps) )
            {
                continue;
            }

            char aLabel[64];
            sithFrameRateTest_GetCaseLabel(pCase, fps, aLabel, sizeof(aLabel));
            const SithFrameRateVecResult* pExpected = sithFrameRateTest_GetVector(pCase, fps);
            const SithFrameRateVecResult result     = sithFrameRateVec_Run(pCase, fps);
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(pExpected->total, result.total, aLabel);
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(pExpected->detail, result.detail, aLabel);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(pExpected->state, result.state, aLabel);
            ++numMatched;
        }
    }

    TEST_ASSERT_TRUE(numMatched > 0);
}

TEST(sithFrameRate, TestImpStateChangesAtItsRateAtEveryFrameRate)
{
    // The IMP heat, the Aetherium stamina and the POR recovery add up the frame time, so the original and the reconstruction
    // reach the same state at every frame rate. 0.02 covers the float sum of 3000 frames.
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateVec_aCases); ++i )
    {
        const SithFrameRateVecCase* pCase = &sithFrameRateVec_aCases[i];
        if ( pCase->kind != SITHFRAMERATEVEC_IMP )
        {
            continue;
        }

        for ( size_t j = 0; j < STD_ARRAYLEN(sithFrameRateVec_aFps); ++j )
        {
            const size_t fps = sithFrameRateVec_aFps[j];
            char aLabel[64];
            sithFrameRateTest_GetCaseLabel(pCase, fps, aLabel, sizeof(aLabel));
            const SithFrameRateVecResult result = sithFrameRateVec_Run(pCase, fps);
            TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.02f, pCase->finalImpState, sithFrameRateVec_GetBitsFloat(result.state), aLabel);
            TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.02f, pCase->finalImpState, sithFrameRateVec_GetBitsFloat(sithFrameRateTest_GetVector(pCase, fps)->state), aLabel);
        }
    }
}

TEST(sithFrameRate, TestPathMoverArrivesOnTimeAtEveryFrameRate)
{
    // A path mover's move (speed x frame time, capped at the time left) and pivot rotation (the fraction done, rebuilt every
    // frame) are independent of the frame rate: they arrive in the first frame at or after the designed time at every frame
    // rate, in the original (the vectors) and the reconstruction.
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateVec_aCases); ++i )
    {
        const SithFrameRateVecCase* pCase = &sithFrameRateVec_aCases[i];
        if ( pCase->kind != SITHFRAMERATEVEC_PATHMOVE && pCase->kind != SITHFRAMERATEVEC_PATHROTATE )
        {
            continue;
        }

        const uint32_t msecDesigned = (uint32_t)(pCase->secInterval * 1000.0f + 0.5f);
        for ( size_t j = 0; j < STD_ARRAYLEN(sithFrameRateVec_aFps); ++j )
        {
            const size_t fps = sithFrameRateVec_aFps[j];
            const uint32_t msecFrameMax = (1000u + (uint32_t)fps - 1u) / (uint32_t)fps;
            char aLabel[64];
            sithFrameRateTest_GetCaseLabel(pCase, fps, aLabel, sizeof(aLabel));
            const SithFrameRateVecResult result     = sithFrameRateVec_Run(pCase, fps);
            const SithFrameRateVecResult* pOriginal = sithFrameRateTest_GetVector(pCase, fps);
            TEST_ASSERT_TRUE_MESSAGE(result.total >= msecDesigned && result.total <= msecDesigned + msecFrameMax, aLabel);
            TEST_ASSERT_TRUE_MESSAGE(pOriginal->total >= msecDesigned && pOriginal->total <= msecDesigned + msecFrameMax, aLabel);
        }
    }
}

TEST(sithFrameRate, TestMouseTurnConvertsTheFrameAxisValueWithTheFrameRate)
{
    // The mouse axis value is read per frame, so the frame rate converts it to a turn rate
    sithFrameRateTest_InitPlayer();
    SithActorInfo* pActor = &sithFrameRateVec_thing.thingInfo.actorInfo;

    sithEngineTest_fps = 30.0f;
    TEST_ASSERT_EQUAL_FLOAT(15.0f, sithPlayerControls_CalculateMouseAngularVelocity(pActor, 0.5f, 0.0f, 1.0f));
    TEST_ASSERT_EQUAL_FLOAT(15.0f - SITHFRAMERATETEST_MAXROTVELOCITY, sithPlayerControls_CalculateMouseAngularVelocity(pActor, 0.5f, -1.0f, 1.0f));

    sithEngineTest_fps = 144.0f;
    TEST_ASSERT_EQUAL_FLOAT(72.0f, sithPlayerControls_CalculateMouseAngularVelocity(pActor, 0.5f, 0.0f, 1.0f));
    TEST_ASSERT_EQUAL_FLOAT(72.0f + SITHFRAMERATETEST_MAXROTVELOCITY * 0.5f, sithPlayerControls_CalculateMouseAngularVelocity(pActor, 0.5f, 1.0f, 0.5f));
}

TEST(sithFrameRate, TestDamageAtRateDealsTheWholePointsOfEachFrame)
{
    // 40 damage per sec in 1 msec frames: a whole point in every 25th frame, no fraction is lost
    sithFrameRateVec_InitThing(0);
    sithFrameRateVec_expectedDamageType = SITH_DAMAGE_IMP_BLAST;
    for ( size_t frameNum = 0; frameNum < 1000u; ++frameNum )
    {
        sithFrameRateVec_AdvanceFrame(1000u, frameNum);
        const float damage = sithThing_DamageThingAtRate(&sithFrameRateVec_thing, &sithFrameRateVec_thing, 40.0f, SITH_DAMAGE_IMP_BLAST);
        TEST_ASSERT_EQUAL_FLOAT((frameNum + 1u) % 25u == 0u ? 1.0f : 0.0f, damage);
    }
    TEST_ASSERT_EQUAL_INT32(40, sithFrameRateVec_damageTotal);

    // A 33 msec frame from game time 1000 msec at 200 per sec: floor(206.6) - floor(200) = 6 points
    sithEngineTest_msecGameTime  = 1033u;
    sithEngineTest_msecFrameTime = 33u;
    TEST_ASSERT_EQUAL_FLOAT(6.0f, sithThing_DamageThingAtRate(&sithFrameRateVec_thing, &sithFrameRateVec_thing, 200.0f, SITH_DAMAGE_IMP_BLAST));

    // No damage at rate 0
    TEST_ASSERT_EQUAL_FLOAT(0.0f, sithThing_DamageThingAtRate(&sithFrameRateVec_thing, &sithFrameRateVec_thing, 0.0f, SITH_DAMAGE_IMP_BLAST));
}

TEST(sithFrameRate, TestSmallTurnStepsAddUpAtEveryFrameRate)
{
    // Regression: stdMath_SinCos read the table one entry (0.022 degrees) off for negative angles, so every right turn
    // step lost 0.022 degrees: 90 degrees in 3 sec came out as 88 at 30 FPS and 24 at 1000 FPS (02_riv parachute cutscene).
    // The remaining error is the table rounding (up to 1e-6 per step), at most about 0.2 degrees over 3000 steps.
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateTest_aFps); ++i )
    {
        size_t fps = sithFrameRateTest_aFps[i];
        TEST_ASSERT_FLOAT_WITHIN(0.25f, -90.0f, sithFrameRateTest_RunRotation(fps, -30.0f));
        TEST_ASSERT_FLOAT_WITHIN(0.25f, 90.0f, sithFrameRateTest_RunRotation(fps, 30.0f));
    }
}

TEST(sithFrameRate, TestAITurnTakesTheSameTimeAtEveryFrameRate)
{
    // Regression for the 02_riv parachute cutscene: chute_indy turns 89 degrees at 30 degrees/sec after AISetLookThing.
    // 89 degrees at 30 degrees/sec plus the slow turn below 2.1 degrees take about 2.98 sec; 50 msec covers the
    // frame time differences. With the stdMath_SinCos mirror error a right turn took 3.3 sec at 144 FPS
    // and 3.8 sec at 300 FPS, and stalled at 1000 FPS once its steps got under one table entry near the goal.
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateTest_aFps); ++i )
    {
        size_t fps = sithFrameRateTest_aFps[i];
        TEST_ASSERT_UINT32_WITHIN(50u, 2990u, sithFrameRateTest_RunAITurn(fps, 30.0f, 89.0f));
        TEST_ASSERT_UINT32_WITHIN(50u, 2990u, sithFrameRateTest_RunAITurn(fps, 30.0f, -89.0f));
    }
}

#ifdef J3D_QOL_IMPROVEMENTS
// Fixed effect timing (QOL builds, speedrun included): effects the original ran on every n-th frame run by game time,
// and COG sleeps and pulses tick as at 30 FPS

TEST(sithFrameRate, TestTimeCycleRunsAsOftenAsAt30FpsAtEveryFrameRate)
{
    // n frames at 30 FPS: 3 sec hold 90 / n cycles, one more or less depending on where the frames end
    const uint32_t aCycles[] = { 1u, 2u, 4u, 8u, 16u };
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateTest_aFps); ++i )
    {
        for ( size_t j = 0; j < STD_ARRAYLEN(aCycles); ++j )
        {
            const uint32_t n = aCycles[j];
            for ( uint32_t offset = 0; offset < n; offset += J3DMAX(n / 2u, 1u) )
            {
                TEST_ASSERT_UINT32_WITHIN(1u, 90u / n, (uint32_t)sithFrameRateTest_RunTimeCycle(sithFrameRateTest_aFps[i], offset, n));
            }
        }
    }
}

TEST(sithFrameRate, TestSurfacingSoundsPlayAsOftenAsAt30FpsAtEveryFrameRate)
{
    // Floating after holding the breath to the limit: at 30 FPS the original plays the gasp on every 16th and the breath on the
    // other 8th frames while the breath recovers above 50 sec, which takes 1.25 sec as it recovers at 8 times the time.
    // It counted frames, so it played them 26 times as often at 1000 FPS (79 gasps instead of 3). The fixed sounds play as
    // often as the original at 30 FPS at every frame rate, one more or less depending on where the frames end, and the
    // breath recovers as in the original. The same for the other effects the original ran on every n-th frame: the drowning
    // sound of an actor taking damage every frame, a decaying weapon's awareness events, a vehicle's exhaust puffs (at least one
    // per frame in the original), and the blocked messages of a path mover (3 blocked frames, then every frame in the original).
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateVec_aCases); ++i )
    {
        const SithFrameRateVecCase* pCase = &sithFrameRateVec_aCases[i];
        if ( !sithFrameRateTest_IsEffectCase(pCase) )
        {
            continue;
        }

        const SithFrameRateVecResult* pAt30Fps = sithFrameRateTest_GetVector(pCase, SITHFRAMERATETEST_DESIGNFPS);
        for ( size_t j = 0; j < STD_ARRAYLEN(sithFrameRateVec_aFps); ++j )
        {
            const size_t fps = sithFrameRateVec_aFps[j];
            if ( sithFrameRateTest_IsPerFrameEffect(pCase) && fps <= SITHFRAMERATETEST_DESIGNFPS )
            {
                continue; // Every frame, like the original: TestOriginalCodeMatchesTheRetailVectors
            }

            char aLabel[64];
            sithFrameRateTest_GetCaseLabel(pCase, fps, aLabel, sizeof(aLabel));
            const SithFrameRateVecResult result = sithFrameRateVec_Run(pCase, fps);
            TEST_ASSERT_UINT32_WITHIN_MESSAGE(1u, pAt30Fps->total, result.total, aLabel); // sounds, events, puffs or messages
            switch ( pCase->kind )
            {
                case SITHFRAMERATEVEC_SURFACING:
                    TEST_ASSERT_UINT32_WITHIN_MESSAGE(1u, pAt30Fps->detail, result.detail, aLabel); // breaths
                    TEST_ASSERT_EQUAL_UINT32_MESSAGE(sithFrameRateTest_GetVector(pCase, fps)->state, result.state, aLabel);
                    break;
                case SITHFRAMERATEVEC_PATHBLOCKED:
                    TEST_ASSERT_UINT32_WITHIN_MESSAGE(34u, pAt30Fps->detail, result.detail, aLabel); // the first message within a 30 FPS frame
                    TEST_ASSERT_UINT32_WITHIN_MESSAGE(1u, pAt30Fps->state, result.state, aLabel);    // blocked moves
                    break;
                default:
                    break;
            }
        }
    }
}

TEST(sithFrameRate, TestAIHeadTrackingReachesTheLookPositionAtEveryFrameRate)
{
    // 45 degrees at 100 degrees/sec take 0.45 sec; the look is done within the 0.5 degree done angle. At 30 FPS the head
    // reaches the look position in the 14th frame and the 15th sees it's done (0.5 sec, the same frame as the original).
    // The original compared the frame's step with the done angle and stopped after one frame above 200 FPS.
    // 0.6 degrees covers the done angle and the approximate inverse trig of rdMath_DeltaAngleNormalized, which measures the
    // angle to the look position.
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateTest_aFps); ++i )
    {
        float headYaw = 0.0f;
        const uint32_t msecElapsed = sithFrameRateTest_RunAIHeadTrack(sithFrameRateTest_aFps[i], 45.0f, &headYaw);
        TEST_ASSERT_FLOAT_WITHIN(0.6f, 45.0f, fabsf(headYaw));
        TEST_ASSERT_TRUE(msecElapsed >= 440u && msecElapsed <= 500u);
    }
}

TEST(sithFrameRate, TestCogSleepAndPulseTickAsAt30FpsAtHigherFrameRates)
{
    // Above 30 FPS, sleeps, COG and thing pulses, re-armed COG and thing timers, re-armed events, AI instinct ticks, weapon
    // refire and item touches tick as often as the original at 30 FPS, also where the frames don't line up with 30 FPS frames,
    // one tick more or less depending on where the 3 sec end. A weapon wait set once ends within a 30 FPS frame of the original. The original ticked once per frame
    // for intervals under a frame (SetPulse(0.01) 299 times in 3 sec at 1000 FPS instead of 89) and up to a third more often
    // for longer ones. Intervals within a msec of a whole number of 30 FPS frames (0.034) are left out: 30 FPS frames take 33
    // or 34 msec, so the original waits one or two frames for them depending on the frame, while sithTime_CalcDeadline rounds
    // them to two. The pulse interval is converted as in the original (a sleep stores only its deadline).
    size_t numCompared = 0;
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateVec_aCases); ++i )
    {
        const SithFrameRateVecCase* pCase = &sithFrameRateVec_aCases[i];
        if ( !sithFrameRateTest_IsTimingCase(pCase) || pCase->bOnFrameBoundary )
        {
            continue;
        }

        const SithFrameRateVecResult* pAt30Fps = sithFrameRateTest_GetVector(pCase, SITHFRAMERATETEST_DESIGNFPS);
        for ( size_t j = 0; j < STD_ARRAYLEN(sithFrameRateVec_aFps); ++j )
        {
            const size_t fps = sithFrameRateVec_aFps[j];
            if ( fps <= SITHFRAMERATETEST_DESIGNFPS )
            {
                continue; // TestOriginalCodeMatchesTheRetailVectors
            }

            char aLabel[64];
            sithFrameRateTest_GetCaseLabel(pCase, fps, aLabel, sizeof(aLabel));
            const SithFrameRateVecResult result = sithFrameRateVec_Run(pCase, fps);
            if ( pCase->kind == SITHFRAMERATEVEC_WEAPONWAIT )
            {
                // A wait set once ends within a 30 FPS frame of the original's end at 30 FPS
                TEST_ASSERT_UINT32_WITHIN_MESSAGE(34u, pAt30Fps->total, result.total, aLabel);
            }
            else if ( pCase->kind == SITHFRAMERATEVEC_WEAPONREFIRE )
            {
                // The original keeps the refire time in float seconds, and its float sum loses a frame now and then at 30 FPS
                // (29 shots in 3 sec for a 0.1 sec wait instead of 30); the deadlines on the 30 FPS frames don't
                TEST_ASSERT_UINT32_WITHIN_MESSAGE(2u, pAt30Fps->total, result.total, aLabel);
            }
            else
            {
                TEST_ASSERT_UINT32_WITHIN_MESSAGE(1u, pAt30Fps->total, result.total, aLabel);
            }

            if ( pCase->kind == SITHFRAMERATEVEC_COG_PULSE || pCase->kind == SITHFRAMERATEVEC_COG_THINGPULSE )
            {
                TEST_ASSERT_EQUAL_UINT32_MESSAGE(sithFrameRateTest_GetVector(pCase, fps)->detail, result.detail, aLabel);
            }
            ++numCompared;
        }
    }

    TEST_ASSERT_TRUE(numCompared > 0);
}

TEST(sithFrameRate, TestDeadlineIsOnThe30FpsFrame)
{
    // Game time 1000 msec is the start of 30 FPS frame 30; frame n starts at ceil(n * 1000 / 30) msec
    sithEngineTest_msecGameTime  = 1000u;
    sithEngineTest_secGameTime   = 1.0f;
    sithEngineTest_msecFrameTime = 7u; // 144 FPS

    TEST_ASSERT_EQUAL_UINT32(1034u, sithTime_CalcDeadline(1u));          // at least the next frame
    TEST_ASSERT_EQUAL_UINT32(1034u, sithTime_CalcDeadline(10u));         // the next frame
    TEST_ASSERT_EQUAL_UINT32(1067u, sithTime_CalcDeadline(50u));         // 2 frames
    TEST_ASSERT_EQUAL_UINT32(1100u, sithTime_CalcDeadline(100u));        // 3 frames
    TEST_ASSERT_EQUAL_UINT32(1033u, sithTime_CalcDeadlineAfter(10u));    // past the start of the next frame
    TEST_ASSERT_EQUAL_UINT32(1133u, sithTime_CalcDeadlineAfter(100u));   // past 100 msec, in the 4th frame
    TEST_ASSERT_EQUAL_UINT32(62000u, sithTime_CalcDeadline(61000u));     // longer than 60 sec: the plain deadline
    TEST_ASSERT_EQUAL_FLOAT(1.067f, sithTime_CalcDeadlineSec(0.05f));    // seconds: 2 frames
    TEST_ASSERT_EQUAL_FLOAT(0.9f, sithTime_CalcDeadlineSec(-0.1f));      // a past interval is already due

    // A deadline is reached at its time and passed after it
    TEST_ASSERT_FALSE(sithTime_IsDeadlineReached(1001u));
    TEST_ASSERT_TRUE(sithTime_IsDeadlineReached(1000u));
    TEST_ASSERT_FALSE(sithTime_IsDeadlinePassed(1000u));
    TEST_ASSERT_TRUE(sithTime_IsDeadlinePassed(999u));
    TEST_ASSERT_TRUE(sithTime_IsDeadlineReachedSec(1.0f));
    TEST_ASSERT_FALSE(sithTime_IsDeadlineReachedSec(1.001f));

    // At 30 FPS and below the plain deadline
    sithEngineTest_msecFrameTime = 33u;
    TEST_ASSERT_EQUAL_UINT32(1010u, sithTime_CalcDeadline(10u));
    TEST_ASSERT_EQUAL_UINT32(1100u, sithTime_CalcDeadlineAfter(100u));
    sithEngineTest_msecFrameTime = 50u;
    TEST_ASSERT_EQUAL_UINT32(1050u, sithTime_CalcDeadline(50u));
}

#else
// Original effect timing, kept in legacy builds. The COG timing and surfacing sounds match the retail vectors,
// see TestOriginalCodeMatchesTheRetailVectors.

TEST(sithFrameRate, TestOriginalFrameCycleRunsMoreOftenAtHighFrameRates)
{
    // Every n-th frame: 3 sec hold 90 / n cycles at 30 FPS and 3000 / n at 1000 FPS
    TEST_ASSERT_UINT32_WITHIN(1u, 90u / 4u, (uint32_t)sithFrameRateTest_RunTimeCycle(30u, 0u, 4u));
    TEST_ASSERT_UINT32_WITHIN(1u, 3000u / 4u, (uint32_t)sithFrameRateTest_RunTimeCycle(1000u, 0u, 4u));
}

TEST(sithFrameRate, TestOriginalAIHeadTrackingStopsAfterOneFrameAtHighFrameRates)
{
    float headYaw = 0.0f;
    sithFrameRateTest_RunAIHeadTrack(30u, 45.0f, &headYaw);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 45.0f, fabsf(headYaw));

    // One step of 100 degrees/sec in a 1 msec frame
    sithFrameRateTest_RunAIHeadTrack(1000u, 45.0f, &headYaw);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.1f, fabsf(headYaw));
}

#endif // J3D_QOL_IMPROVEMENTS

#ifndef SITHPLAYERCONTROLS_FIXEDFPS
// Original control behavior, kept in legacy builds

TEST(sithFrameRate, TestOriginalKeyTurnGrowsWithTheFrameRate)
{
    // Every frame adds 1 degree on top of the actor's max rotation velocity
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 3.0f * (30.0f + SITHFRAMERATETEST_MAXROTVELOCITY), sithFrameRateTest_RunKeyTurn(30u));
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 3.0f * (144.0f + SITHFRAMERATETEST_MAXROTVELOCITY), sithFrameRateTest_RunKeyTurn(144u));
}

TEST(sithFrameRate, TestOriginalJewelFlyThrustRampsFasterAtHighFrameRates)
{
    TEST_ASSERT_UINT32_WITHIN(34u, 5556u, sithFrameRateTest_RunJewelFlyThrust(30u));
    TEST_ASSERT_UINT32_WITHIN(7u, 243u, sithFrameRateTest_RunJewelFlyThrust(144u));
}

#else
// Fixed control behavior: the same at every frame rate

TEST(sithFrameRate, TestKeyTurnIsTheSameAtEveryFrameRate)
{
    float expectedYaw = 3.0f * (SITHPLAYERCONTROLS_FIXEDFPS + SITHFRAMERATETEST_MAXROTVELOCITY);
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateTest_aFps); ++i )
    {
        TEST_ASSERT_FLOAT_WITHIN(0.05f, expectedYaw, sithFrameRateTest_RunKeyTurn(sithFrameRateTest_aFps[i]));
    }
}

TEST(sithFrameRate, TestJewelFlyThrustReachesFullThrustInTheSameTimeAtEveryFrameRate)
{
    // The thrust grows by 0.0002 * FPS^2 per second at the fixed control frame rate: 1.389 sec at 60 FPS
    uint32_t msecExpected = (uint32_t)(1000.0f / (0.0002f * SITHPLAYERCONTROLS_FIXEDFPS * SITHPLAYERCONTROLS_FIXEDFPS) + 0.5f);
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateTest_aFps); ++i )
    {
        size_t fps = sithFrameRateTest_aFps[i];
        TEST_ASSERT_UINT32_WITHIN(1000u / fps + 1u, msecExpected, sithFrameRateTest_RunJewelFlyThrust(fps));
    }
}
#endif // SITHPLAYERCONTROLS_FIXEDFPS

#ifndef J3D_SPEEDRUN_BUILD
// Fixed damage behavior: the damage of every frame rate is the designed rate. The original damage of speedrun and legacy builds
// matches the retail vectors, see TestOriginalCodeMatchesTheRetailVectors.

TEST(sithFrameRate, TestDamageIsAtTheDesignedRateAtEveryFrameRate)
{
    // Drowning, raft leak, IMP overheat (IMP2, IMP3, IMP5) and the Aetherium stamina running out deal their designed damage
    // in the 3 sec at every frame rate, scaled by difficulty. The original dropped the fraction of every frame's damage, so it
    // dealt less, e.g. 540 instead of 600 drowning at 30 FPS, none at 300 FPS, and no IMP overheat damage above 34 FPS.
    // The fix deals at least the original damage at every frame rate and changes only the damage, not the breath or IMP state.
    // A damage that starts mid-run (the IMP overheats, the stamina runs out) is dealt for the whole frame it starts in, and the
    // IMP state adds up the frame times in float, which moves the start by less than 1% (8 msec of 1 sec at 1000 FPS);
    // the float rate (e.g. 1000 / 1.2) can move one damage point.
    size_t numCompared = 0;
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFrameRateVec_aCases); ++i )
    {
        const SithFrameRateVecCase* pCase = &sithFrameRateVec_aCases[i];
        if ( !sithFrameRateTest_IsDamageCase(pCase) )
        {
            continue;
        }

        const float designed = pCase->damagePerSec * (float)(SITHFRAMERATEVEC_MSECDURATION - pCase->msecDamageStart) / 1000.0f;
        for ( size_t j = 0; j < STD_ARRAYLEN(sithFrameRateVec_aFps); ++j )
        {
            const size_t fps = sithFrameRateVec_aFps[j];
            const uint32_t msecFrameMax = (1000u + (uint32_t)fps - 1u) / (uint32_t)fps;
            const float msecStartError = pCase->msecDamageStart ? (float)msecFrameMax + (float)pCase->msecDamageStart / 100.0f : 0.0f;
            const float tolerance      = 1.0f + pCase->damagePerSec * msecStartError / 1000.0f;

            char aLabel[64];
            sithFrameRateTest_GetCaseLabel(pCase, fps, aLabel, sizeof(aLabel));
            const SithFrameRateVecResult* pOriginal = sithFrameRateTest_GetVector(pCase, fps);
            const SithFrameRateVecResult result     = sithFrameRateVec_Run(pCase, fps);
            TEST_ASSERT_FLOAT_WITHIN_MESSAGE(tolerance, designed, (float)result.total, aLabel);
            TEST_ASSERT_TRUE_MESSAGE(result.total >= pOriginal->total, aLabel);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(pOriginal->state, result.state, aLabel);
            ++numCompared;
        }
    }

    TEST_ASSERT_TRUE(numCompared > 0);
}

#endif // J3D_SPEEDRUN_BUILD

TEST_GROUP_RUNNER(sithFrameRate)
{
    RUN_TEST_CASE(sithFrameRate, TestFramesAddUpToTheSimulatedTime);
    RUN_TEST_CASE(sithFrameRate, TestVectorsCoverEveryCaseAndFrameRate);
    RUN_TEST_CASE(sithFrameRate, TestOriginalCodeMatchesTheRetailVectors);
    RUN_TEST_CASE(sithFrameRate, TestImpStateChangesAtItsRateAtEveryFrameRate);
    RUN_TEST_CASE(sithFrameRate, TestPathMoverArrivesOnTimeAtEveryFrameRate);
    RUN_TEST_CASE(sithFrameRate, TestMouseTurnConvertsTheFrameAxisValueWithTheFrameRate);
    RUN_TEST_CASE(sithFrameRate, TestDamageAtRateDealsTheWholePointsOfEachFrame);
    RUN_TEST_CASE(sithFrameRate, TestSmallTurnStepsAddUpAtEveryFrameRate);
    RUN_TEST_CASE(sithFrameRate, TestAITurnTakesTheSameTimeAtEveryFrameRate);
#ifdef J3D_QOL_IMPROVEMENTS
    RUN_TEST_CASE(sithFrameRate, TestTimeCycleRunsAsOftenAsAt30FpsAtEveryFrameRate);
    RUN_TEST_CASE(sithFrameRate, TestSurfacingSoundsPlayAsOftenAsAt30FpsAtEveryFrameRate);
    RUN_TEST_CASE(sithFrameRate, TestAIHeadTrackingReachesTheLookPositionAtEveryFrameRate);
    RUN_TEST_CASE(sithFrameRate, TestCogSleepAndPulseTickAsAt30FpsAtHigherFrameRates);
    RUN_TEST_CASE(sithFrameRate, TestDeadlineIsOnThe30FpsFrame);
#else
    RUN_TEST_CASE(sithFrameRate, TestOriginalFrameCycleRunsMoreOftenAtHighFrameRates);
    RUN_TEST_CASE(sithFrameRate, TestOriginalAIHeadTrackingStopsAfterOneFrameAtHighFrameRates);
#endif
#ifndef SITHPLAYERCONTROLS_FIXEDFPS
    RUN_TEST_CASE(sithFrameRate, TestOriginalKeyTurnGrowsWithTheFrameRate);
    RUN_TEST_CASE(sithFrameRate, TestOriginalJewelFlyThrustRampsFasterAtHighFrameRates);
#else
    RUN_TEST_CASE(sithFrameRate, TestKeyTurnIsTheSameAtEveryFrameRate);
    RUN_TEST_CASE(sithFrameRate, TestJewelFlyThrustReachesFullThrustInTheSameTimeAtEveryFrameRate);
#endif
#ifndef J3D_SPEEDRUN_BUILD
    RUN_TEST_CASE(sithFrameRate, TestDamageIsAtTheDesignedRateAtEveryFrameRate);
#endif
}
