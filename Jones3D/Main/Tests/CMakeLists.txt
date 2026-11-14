set(JONES3D_TEST_SOURCES
    jones3dTestRunner.c
    JonesMainTest.c
)

# The test target links the game sources directly, mirroring how Jones3D_DLL is built, because
# JonesMain.c depends on the rest of the game layer. The tests only call the pure lookup function
# JonesMain_FindClosestVideoMode, so the fixed-address retail globals are never touched.
get_filename_component(JONES3D_TEST_GAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
file(GLOB_RECURSE JONES3D_TEST_GAME_SOURCES "${JONES3D_TEST_GAME_ROOT}/*.c")
list(FILTER JONES3D_TEST_GAME_SOURCES EXCLUDE REGEX ".*/Tests/.*")
list(FILTER JONES3D_TEST_GAME_SOURCES EXCLUDE REGEX ".*/dllmain\\.c$")

add_executable(jones3dTests
    ${JONES3D_TEST_SOURCES}
    ${JONES3D_TEST_GAME_SOURCES}
)

target_include_directories(jones3dTests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

target_link_libraries(jones3dTests PRIVATE
    Libs
    unity
    comctl32
    xBRZ # jonesConfig.c dialog image upscaling
)

set_target_properties(jones3dTests PROPERTIES
    FOLDER "Tests/Jones3D"
)

source_group("Tests" FILES ${JONES3D_TEST_SOURCES})
source_group("Tested Sources" FILES ${JONES3D_TEST_GAME_SOURCES})

set(JONES3D_TEST_BACKEND_LABEL "")
if (J3D_DIRECTX6)
    set(JONES3D_TEST_BACKEND_LABEL "DirectX6")
elseif (J3D_DIRECTX9)
    set(JONES3D_TEST_BACKEND_LABEL "DirectX9")
endif()

add_test(NAME Jones3D.Main COMMAND jones3dTests)
set_tests_properties(Jones3D.Main PROPERTIES
    LABELS "unit;Jones3D;${JONES3D_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)
