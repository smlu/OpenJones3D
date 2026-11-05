set(SITH_ENGINE_TEST_SOURCES
    sithEngineTest.c
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
)

if (MSVC)
    set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../sithPhysics.c" APPEND PROPERTY COMPILE_OPTIONS
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/sithPhysicsTestSeams.h")
endif()

add_executable(sithEngineTests
    ${SITH_ENGINE_TEST_SOURCES}
    ../sithIntersect.c
    ../sithPathMove.c
    ../sithPhysics.c
)
target_compile_definitions(sithEngineTests PRIVATE J3D_TEST)
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
