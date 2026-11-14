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
    # prefix, generated from jonesConfig.c, so both copies link into this executable.
    set(JONES3D_SYSTEM_TEST_SOURCES "${JONES3D_TEST_GAME_ROOT}/Display/Tests/jonesConfigSystemTest.c")
    set(JONES3D_SYSTEM_SUITES jonesConfigSystem)

    set(JONES3D_CONFIG_SOURCE "${JONES3D_TEST_GAME_ROOT}/Display/jonesConfig.c")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${JONES3D_CONFIG_SOURCE}")
    file(READ "${JONES3D_CONFIG_SOURCE}" JONES3D_CONFIG_TEXT)
    # Make every line a list element: protect the list separators and escapes first
    string(REPLACE "\\" "<BS>" JONES3D_CONFIG_TEXT "${JONES3D_CONFIG_TEXT}")
    string(REPLACE ";" "<SC>" JONES3D_CONFIG_TEXT "${JONES3D_CONFIG_TEXT}")
    string(REPLACE "[" "<LB>" JONES3D_CONFIG_TEXT "${JONES3D_CONFIG_TEXT}")
    string(REPLACE "]" "<RB>" JONES3D_CONFIG_TEXT "${JONES3D_CONFIG_TEXT}")
    string(REPLACE "\r" "" JONES3D_CONFIG_TEXT "${JONES3D_CONFIG_TEXT}")
    string(REPLACE "\n" ";" JONES3D_CONFIG_LINES "${JONES3D_CONFIG_TEXT}")

    set(JONES3D_CONFIG_EXTERNALS "")
    foreach(LINE IN LISTS JONES3D_CONFIG_LINES)
        # Only file-scope definitions start at column 0 with a type
        if (NOT LINE MATCHES "^[A-Za-z_]" OR LINE MATCHES "^(static|extern|typedef|struct|union|enum)[ \t]")
            continue()
        endif()

        # Each test is the last MATCHES of its condition, which sets CMAKE_MATCH_1
        set(bPrototype FALSE)
        if (LINE MATCHES "<SC>[ \t]*$")
            set(bPrototype TRUE)
        endif()

        if (NOT bPrototype AND LINE MATCHES "^[^(=]*[ *]([A-Za-z_][A-Za-z0-9_]*)[ \t]*\\(")
            list(APPEND JONES3D_CONFIG_EXTERNALS "${CMAKE_MATCH_1}") # Function definition
        elseif (LINE MATCHES "^[^(=]*[ *]([A-Za-z_][A-Za-z0-9_]*)[ \t]*(<LB>[^<]*<RB>)*[ \t]*(=.*|<SC>[ \t]*)$")
            list(APPEND JONES3D_CONFIG_EXTERNALS "${CMAKE_MATCH_1}") # Global variable definition
        endif()
    endforeach()
    list(REMOVE_DUPLICATES JONES3D_CONFIG_EXTERNALS)

    set(JONES3D_CONFIG_RENAMES "")
    foreach(NAME IN LISTS JONES3D_CONFIG_EXTERNALS)
        string(APPEND JONES3D_CONFIG_RENAMES "#define ${NAME} jonesConfigSystemCopy_${NAME}\n")
    endforeach()

    set(JONES3D_CONFIG_RENAME_HEADER "${CMAKE_CURRENT_BINARY_DIR}/jonesConfigSystemRenames.h")
    file(GENERATE OUTPUT "${JONES3D_CONFIG_RENAME_HEADER}" CONTENT
"// Generated from jonesConfig.c for jonesConfigSystemTest.c. Do not edit.
#ifndef JONESCONFIGSYSTEMRENAMES_H
#define JONESCONFIGSYSTEMRENAMES_H
${JONES3D_CONFIG_RENAMES}#endif
")

    if (MSVC)
        set_property(SOURCE ${JONES3D_SYSTEM_TEST_SOURCES} APPEND PROPERTY COMPILE_OPTIONS "/FI${JONES3D_CONFIG_RENAME_HEADER}")
    else()
        set_property(SOURCE ${JONES3D_SYSTEM_TEST_SOURCES} APPEND PROPERTY COMPILE_OPTIONS -include "${JONES3D_CONFIG_RENAME_HEADER}")
    endif()
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
