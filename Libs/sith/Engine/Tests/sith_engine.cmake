# Engine suites: sphere/plane intersection, rotating paths, fixed-step player physics and frame-rate regressions.
# Included by Libs/sith/Tests/CMakeLists.txt.
set(SITH_ENGINE_TEST_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(SITH_ENGINE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

set(SITH_ENGINE_TEST_SOURCES
    "${SITH_ENGINE_TEST_DIR}/sithEngineTest.c"
    "${SITH_ENGINE_TEST_DIR}/sithFrameRateTest.c"
    "${SITH_ENGINE_TEST_DIR}/sithIntersectTest.c"
    "${SITH_ENGINE_TEST_DIR}/sithPathMoveTest.c"
    "${SITH_ENGINE_TEST_DIR}/sithPhysicsTest.c"
)

set(SITH_ENGINE_TESTED_SOURCES
    "${SITH_ENGINE_DIR}/Engine/sithIntersect.c"
    "${SITH_ENGINE_DIR}/Engine/sithPathMove.c"
    "${SITH_ENGINE_DIR}/Engine/sithPhysics.c"
    "${SITH_ENGINE_DIR}/World/sithActor.c"
    "${SITH_ENGINE_DIR}/Gameplay/sithPlayer.c"
    "${SITH_ENGINE_DIR}/AI/sithAIMove.c"
    "${SITH_ENGINE_DIR}/Cog/sithCog.c"
    "${SITH_ENGINE_DIR}/Cog/sithCogFunction.c"
    "${SITH_ENGINE_DIR}/Cog/sithCogFunctionThing.c"
    "${SITH_ENGINE_DIR}/Gameplay/sithEvent.c"
    "${SITH_ENGINE_DIR}/AI/sithAI.c"
    "${SITH_ENGINE_DIR}/World/sithWeapon.c"
    "${SITH_ENGINE_DIR}/World/sithItem.c"
)

# Compile the real modules with controlled orientation/collision dependencies.
set_property(SOURCE "${SITH_ENGINE_DIR}/Engine/sithPathMove.c" PROPERTY COMPILE_DEFINITIONS
    rdMatrix_PostRotate34=sithPathMoveTest_PostRotate34
    rdMatrix_PreRotate34=sithPathMoveTest_PreRotate34
    rdMatrix_BuildRotate34=sithPathMoveTest_BuildRotate34
    rdVector_Normalize3Acc=sithPathMoveTest_Normalize3Acc
    sithCollision_ApplyThingOrient=sithPathMoveTest_ApplyThingOrient
    sithCollision_MoveThing=sithPathMoveTest_MoveThing
    sithCog_ThingSendMessage=sithFrameRateVec_ThingSendMessage # the blocked messages of the frame-rate tests
)

if (MSVC)
    set_property(SOURCE "${SITH_ENGINE_DIR}/Engine/sithPhysics.c" APPEND PROPERTY COMPILE_OPTIONS
        "/FI${SITH_ENGINE_TEST_DIR}/sithPhysicsTestSeams.h")

    # Frame-rate tests run the real per-frame damage code and record the damage it deals.
    # sithAIMove.c is compiled unchanged, so the AI turn test uses the real rotation math.
    set_property(SOURCE
        "${SITH_ENGINE_DIR}/World/sithActor.c"
        "${SITH_ENGINE_DIR}/Gameplay/sithPlayer.c"
        APPEND PROPERTY COMPILE_OPTIONS "/FI${SITH_ENGINE_TEST_DIR}/sithFrameRateTestSeams.h")

    # Weapon refire/wait and item touch timing: the real sithWeapon.c and sithItem.c with a weapon type and line of sight provided.
    set_property(SOURCE
        "${SITH_ENGINE_DIR}/World/sithWeapon.c"
        "${SITH_ENGINE_DIR}/World/sithItem.c"
        APPEND PROPERTY COMPILE_OPTIONS "/FI${SITH_ENGINE_TEST_DIR}/sithFrameRateWeaponSeams.h")

    # COG timing tests run the real sleep, pulse and timer code; only the script argument pops and the script run are replaced.
    # sithEvent.c and sithAI.c are compiled unchanged for the event and AI instinct timing.
    set_property(SOURCE
        "${SITH_ENGINE_DIR}/Cog/sithCog.c"
        "${SITH_ENGINE_DIR}/Cog/sithCogFunction.c"
        "${SITH_ENGINE_DIR}/Cog/sithCogFunctionThing.c"
        APPEND PROPERTY COMPILE_OPTIONS "/FI${SITH_ENGINE_TEST_DIR}/sithCogTimingTestSeams.h")
endif()

set(SITH_ENGINE_SUITES
    sithFrameRate
    sithIntersect
    sithPathMove
    sithPhysics
)

# The engine globals of the original executable are redirected by sithEngineTestAddrs.h, force-included first
sith_add_test_suite(sithEngineSuite
    SUITES        ${SITH_ENGINE_SUITES}
    TESTS         ${SITH_ENGINE_TEST_SOURCES}
    TESTED        ${SITH_ENGINE_TESTED_SOURCES}
    SCAN          "${SITH_ENGINE_TEST_DIR}/sithFrameRateVec.h"
    DEFINITIONS
        J3D_TEST
        SITH_FRAMERATE_TV_DIR="${SITH_ENGINE_TEST_DIR}/tv/sithFrameRate"
    FORCE_INCLUDE "${SITH_ENGINE_TEST_DIR}/sithEngineTestAddrs.h"
)
