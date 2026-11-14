get_filename_component(JONES3D_TEST_GAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Unit suites: video mode lookup (JonesMain), dialog layout and controls (jonesConfig), HUD inventory and fades
# (JonesHud). jonesConfigTest.c and JonesHudTest.c compile jonesConfig.c and JonesHud.c themselves, with their
# Win32, sound and renderer calls replaced, so these two are left out of the game sources below.
set(JONES3D_UNIT_TEST_SOURCES
    "${JONES3D_TEST_GAME_ROOT}/Main/Tests/JonesMainTest.c"
    "${JONES3D_TEST_GAME_ROOT}/Display/Tests/jonesConfigTest.c"
    "${JONES3D_TEST_GAME_ROOT}/Display/Tests/JonesHudTest.c"
)

# The test executable links the game sources directly, mirroring how Jones3D_DLL is built, because JonesMain.c
# depends on the rest of the game layer. The tests don't touch the fixed-address retail globals of these sources.
file(GLOB_RECURSE JONES3D_TEST_GAME_SOURCES "${JONES3D_TEST_GAME_ROOT}/*.c")
list(FILTER JONES3D_TEST_GAME_SOURCES EXCLUDE REGEX ".*/Tests/.*")
list(FILTER JONES3D_TEST_GAME_SOURCES EXCLUDE REGEX ".*/dllmain\\.c$")
list(FILTER JONES3D_TEST_GAME_SOURCES EXCLUDE REGEX ".*/Display/jonesConfig\\.c$")
list(FILTER JONES3D_TEST_GAME_SOURCES EXCLUDE REGEX ".*/Display/JonesHud\\.c$")

set(JONES3D_SYSTEM_TEST_SOURCES "")
set(JONES3D_SYSTEM_SUITES "")
if (JONES3D_BUILD_SYSTEM_TESTS)
    # Hidden native GDI windows (jonesConfigSystem). jonesConfigSystemTest.c compiles its own copy of jonesConfig.c
    # with the real Win32 calls. Every external function and global of that copy gets a jonesConfigSystemCopy_
    # prefix (jonesConfigSystemCopyRenames.h, generated from jonesConfig.c), so both copies link into this
    # executable.
    set(JONES3D_SYSTEM_TEST_SOURCES "${JONES3D_TEST_GAME_ROOT}/Display/Tests/jonesConfigSystemTest.c")
    set(JONES3D_SYSTEM_SUITES jonesConfigSystem)

    jones3d_isolate_test_sources(jonesConfigSystemCopy
        SOURCES ${JONES3D_SYSTEM_TEST_SOURCES}
        SCAN    "${JONES3D_TEST_GAME_ROOT}/Display/jonesConfig.c"
    )
endif()

add_executable(jones3dTests
    ${JONES3D_UNIT_TEST_SOURCES}
    ${JONES3D_SYSTEM_TEST_SOURCES}
    ${JONES3D_TEST_GAME_SOURCES}
)

target_include_directories(jones3dTests PRIVATE "${JONES3D_TEST_GAME_ROOT}/..")

# Only the test translation units use the test hooks; the game sources compile as for Jones3D_DLL
set_property(SOURCE ${JONES3D_UNIT_TEST_SOURCES} ${JONES3D_SYSTEM_TEST_SOURCES} APPEND PROPERTY COMPILE_DEFINITIONS J3D_TEST)

target_link_libraries(jones3dTests PRIVATE
    Libs
    unity
    Comctl32.lib
    xBRZ # jonesConfig.c dialog image upscaling
)

if (MSVC)
    # Keep exercised functions without retaining unrelated GUI paths
    target_compile_options(jones3dTests PRIVATE /Gy)
    target_link_options(jones3dTests PRIVATE /OPT:REF /INCREMENTAL:NO)
endif()

jones3d_add_test_suites(jones3dTests
    UNIT
        JonesMain
        jonesConfig
        JonesHud
    SYSTEM
        ${JONES3D_SYSTEM_SUITES}
)

set_target_properties(jones3dTests PROPERTIES
    FOLDER "Tests/Jones3D"
)

source_group("Tests" FILES ${JONES3D_UNIT_TEST_SOURCES})
source_group("System Tests" FILES ${JONES3D_SYSTEM_TEST_SOURCES})
source_group("Tested Sources" FILES ${JONES3D_TEST_GAME_SOURCES})

set(JONES3D_TEST_BACKEND_LABEL "DirectX6")
if (J3D_DIRECTX9)
    set(JONES3D_TEST_BACKEND_LABEL "DirectX9")
endif()

add_test(NAME Jones3D.Main COMMAND jones3dTests --suite JonesMain)
set_tests_properties(Jones3D.Main PROPERTIES
    LABELS "unit;Jones3D;${JONES3D_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)

add_test(NAME Jones3D.Config COMMAND jones3dTests --suite jonesConfig)
set_tests_properties(Jones3D.Config PROPERTIES
    LABELS "unit;Jones3D;${JONES3D_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)

add_test(NAME Jones3D.Hud COMMAND jones3dTests --suite JonesHud)
set_tests_properties(Jones3D.Hud PROPERTIES
    LABELS "unit;Jones3D;${JONES3D_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)

if (JONES3D_BUILD_SYSTEM_TESTS)
    add_test(NAME Jones3D.ConfigSystem COMMAND jones3dTests --kind system)
    set_tests_properties(Jones3D.ConfigSystem PROPERTIES
        LABELS "system;Jones3D;${JONES3D_TEST_BACKEND_LABEL}"
        TIMEOUT 30
        RUN_SERIAL TRUE
    )
endif()
