set(SITH_ENGINE_TEST_SOURCES
    sithEngineTest.c
    sithFrameRateTest.c
    sithIntersectTest.c
    sithPathMoveTest.c
    sithPhysicsTest.c
)

# Compile the real modules with controlled orientation/collision dependencies.
# These source properties are scoped to this test directory, not the sith library.
set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../sithPathMove.c" PROPERTY COMPILE_DEFINITIONS
    rdMatrix_PostRotate34=sithPathMoveTest_PostRotate34
    rdMatrix_PreRotate34=sithPathMoveTest_PreRotate34
    rdMatrix_BuildRotate34=sithPathMoveTest_BuildRotate34
    rdVector_Normalize3Acc=sithPathMoveTest_Normalize3Acc
    sithCollision_ApplyThingOrient=sithPathMoveTest_ApplyThingOrient
    sithCollision_MoveThing=sithPathMoveTest_MoveThing
    sithCog_ThingSendMessage=sithFrameRateVec_ThingSendMessage # the blocked messages of the frame-rate tests
)

if (MSVC)
    set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../sithPhysics.c" APPEND PROPERTY COMPILE_OPTIONS
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/sithPhysicsTestSeams.h")

    # Frame-rate tests run the real per-frame damage code and record the damage it deals.
    # sithAIMove.c is compiled unchanged, so the AI turn test uses the real rotation math.
    set_property(SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../World/sithActor.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Gameplay/sithPlayer.c"
        APPEND PROPERTY COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/sithFrameRateTestSeams.h")

    # Weapon refire/wait and item touch timing: the real sithWeapon.c and sithItem.c with a weapon type and line of sight provided.
    set_property(SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../World/sithWeapon.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../World/sithItem.c"
        APPEND PROPERTY COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/sithFrameRateWeaponSeams.h")

    # COG timing tests run the real sleep, pulse and timer code; only the script argument pops and the script run are replaced.
    # sithEvent.c and sithAI.c are compiled unchanged for the event and AI instinct timing.
    set_property(SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Cog/sithCog.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Cog/sithCogFunction.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Cog/sithCogFunctionThing.c"
        APPEND PROPERTY COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/sithCogTimingTestSeams.h")
endif()

add_executable(sithEngineTests
    ${SITH_ENGINE_TEST_SOURCES}
    ../sithIntersect.c
    ../sithPathMove.c
    ../sithPhysics.c
    ../../World/sithActor.c
    ../../Gameplay/sithPlayer.c
    ../../AI/sithAIMove.c
    ../../Cog/sithCog.c
    ../../Cog/sithCogFunction.c
    ../../Cog/sithCogFunctionThing.c
    ../../Gameplay/sithEvent.c
    ../../AI/sithAI.c
    ../../World/sithWeapon.c
    ../../World/sithItem.c
)
target_compile_definitions(sithEngineTests PRIVATE
    J3D_TEST
    SITH_FRAMERATE_TV_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tv/sithFrameRate"
)
target_link_libraries(sithEngineTests PRIVATE sith sound j3dTestSupport w32util)
if (MSVC)
    target_compile_options(sithEngineTests PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/sithEngineTestAddrs.h")
endif()
set_target_properties(sithEngineTests PROPERTIES FOLDER "Tests/Libs/sith")
source_group("Tests" FILES ${SITH_ENGINE_TEST_SOURCES})

set(SITH_ENGINE_TEST_BACKEND_LABEL "DirectX6")
if (J3D_DIRECTX9)
    set(SITH_ENGINE_TEST_BACKEND_LABEL "DirectX9")
endif()

add_test(NAME sith.Engine COMMAND sithEngineTests)
set_tests_properties(sith.Engine PROPERTIES
    LABELS "unit;sith;${SITH_ENGINE_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)
