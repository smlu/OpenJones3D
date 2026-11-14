# sithTests: every sith test suite in one executable. Each area (world, engine, DSS, multiplayer and the vector
# suites of Vec/) is described in its own .cmake file, included here so its source properties apply to this target.
#
# The areas compile the same production sources with different seams (for example sithAI.c, sithThing.c and
# sithCog.c) and define fixtures with the same names, so sith_add_test_suite gives each area's sources their own names
# (jones3d_isolate_test_sources). What an area doesn't compile comes from the sith library, as before.

set(SITH_TESTS_SOURCES "")
set(SITH_TESTS_SUITES "")

# sith_add_test_suite(<prefix>
#     SUITES        <suites>          (Unity group runners, in run order)
#     TESTS         <test sources>
#     TESTED        <tested sources>  (production sources or the wrappers that include them)
#     [SCAN         <files>]          (more files whose definitions are renamed: included production sources and
#                                      headers that define functions)
#     [DEFINITIONS  <definitions>]    (compile definitions of the area's sources)
#     [FORCE_INCLUDE <headers>]       (force-included first in all of the area's sources)
#     [NAMES        <names>]          (more names to rename, e.g. fixtures that macros define)
#     [EXCLUDE      <names>]          (definitions not to rename: names that headers also define as macros, defined by
#                                      this area only)
# )
function(sith_add_test_suite PREFIX)
    cmake_parse_arguments(ST "" "" "SUITES;TESTS;TESTED;SCAN;DEFINITIONS;FORCE_INCLUDE;NAMES;EXCLUDE" ${ARGN})

    # A source file is compiled once per target, but several areas compile the same production source with their own
    # seams. Each area compiles its production sources through a generated wrapper that includes them, which takes over
    # the source properties the area set on the production source.
    set(ST_TESTED_UNITS "")
    foreach(ST_SOURCE IN LISTS ST_TESTED)
        get_filename_component(ST_DIR "${ST_SOURCE}" DIRECTORY)
        get_filename_component(ST_DIR "${ST_DIR}" ABSOLUTE)
        if (ST_DIR MATCHES "/Tests$")
            list(APPEND ST_TESTED_UNITS "${ST_SOURCE}") # Already a wrapper of the area
            continue()
        endif()

        get_filename_component(ST_NAME "${ST_SOURCE}" NAME)
        set(ST_WRAPPER "${CMAKE_CURRENT_BINARY_DIR}/${PREFIX}/${ST_NAME}")
        file(GENERATE OUTPUT "${ST_WRAPPER}" CONTENT "// Generated for the ${PREFIX} test sources. Do not edit.\n#include \"${ST_SOURCE}\"\n")

        foreach(ST_PROPERTY COMPILE_DEFINITIONS COMPILE_OPTIONS)
            get_property(ST_VALUE SOURCE "${ST_SOURCE}" PROPERTY ${ST_PROPERTY})
            set_property(SOURCE "${ST_WRAPPER}" PROPERTY ${ST_PROPERTY} ${ST_VALUE})
            set_property(SOURCE "${ST_SOURCE}" PROPERTY ${ST_PROPERTY} "")
        endforeach()

        list(APPEND ST_TESTED_UNITS "${ST_WRAPPER}")
    endforeach()

    set(ST_ALL ${ST_TESTS} ${ST_TESTED_UNITS})
    list(APPEND ST_SCAN ${ST_TESTED})

    if (ST_DEFINITIONS)
        set_property(SOURCE ${ST_ALL} APPEND PROPERTY COMPILE_DEFINITIONS ${ST_DEFINITIONS})
    endif()

    if (ST_FORCE_INCLUDE AND MSVC)
        set(ST_FI "")
        foreach(ST_HEADER IN LISTS ST_FORCE_INCLUDE)
            list(APPEND ST_FI "/FI${ST_HEADER}")
        endforeach()
        foreach(ST_SOURCE IN LISTS ST_ALL)
            get_property(ST_OPTIONS SOURCE "${ST_SOURCE}" PROPERTY COMPILE_OPTIONS)
            set_property(SOURCE "${ST_SOURCE}" PROPERTY COMPILE_OPTIONS ${ST_FI} ${ST_OPTIONS})
        endforeach()
    endif()

    jones3d_isolate_test_sources(${PREFIX}
        SOURCES ${ST_ALL}
        SCAN    ${ST_TESTS} ${ST_SCAN}
        NAMES   ${ST_NAMES}
        EXCLUDE ${ST_EXCLUDE}
    )

    set(SITH_TESTS_SOURCES ${SITH_TESTS_SOURCES} ${ST_ALL} PARENT_SCOPE)
    set(SITH_TESTS_SUITES ${SITH_TESTS_SUITES} ${ST_SUITES} PARENT_SCOPE)
endfunction()

set(SITH_TEST_BACKEND_LABEL "")
if (J3D_DIRECTX6)
    set(SITH_TEST_BACKEND_LABEL "DirectX6")
elseif (J3D_DIRECTX9)
    set(SITH_TEST_BACKEND_LABEL "DirectX9")
endif()

include(sith_world.cmake)
include(../Engine/Tests/sith_engine.cmake)
include(../Dss/Tests/sith_dss.cmake)
include(Vec/sith_vec.cmake)

add_executable(sithTests
    sithTestMain.c
    ${SITH_TESTS_SOURCES}
)

target_include_directories(sithTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Vec")

target_link_libraries(sithTests PRIVATE
    sith
    sound
    unity
    j3dTestSupport
    w32util
)

jones3d_add_test_suites(sithTests
    CHILD_HANDLER SithTest_Init
    UNIT
        ${SITH_TESTS_SUITES}
)

# The DSS, multiplayer and vector records normalize the addresses of the synthetic state (dssv_NormWord,
# sithVec_NormWord); that only works when they cannot be confused with the generated integers and floats.
# Load the image high and fixed.
if (MSVC)
    target_link_options(sithTests PRIVATE /BASE:0x20000000 /DYNAMICBASE:NO)
endif()

set_target_properties(sithTests PROPERTIES
    FOLDER "Tests/Libs/sith"
)

source_group("Tests" FILES ${SITH_TESTS_SOURCES})

# One CTest entry per area, selecting its suites
function(sith_add_ctest NAME TIMEOUT)
    string(REPLACE ";" "," SUITES "${ARGN}")
    add_test(NAME ${NAME} COMMAND sithTests --suite ${SUITES})
    set_tests_properties(${NAME} PROPERTIES
        LABELS "unit;sith;${SITH_TEST_BACKEND_LABEL}"
        TIMEOUT ${TIMEOUT}
    )
endfunction()

# The local fixtures perform bounded in-memory I/O; allow slower hosts and file reads.
sith_add_ctest(sith 60 ${SITH_WORLD_SUITES})
sith_add_ctest(sith.Engine 60 ${SITH_ENGINE_SUITES})
sith_add_ctest(sith.DSS 60 ${SITH_DSS_SUITES})
sith_add_ctest(sith.Multi 60 ${SITH_MULTI_SUITES})
sith_add_ctest(sith.ThingQuetz 60 ${SITH_VEC_THINGQUETZ_SUITES})
sith_add_ctest(sith.AIInstinct 60 ${SITH_VEC_AIINSTINCT_SUITES})
# Many cases (the fixture has more than a thousand): allow slow runners
sith_add_ctest(sith.AIInstinctFn 180 ${SITH_VEC_AIINSTINCTFN_SUITES})
sith_add_ctest(sith.AIMove 60 ${SITH_VEC_AIMOVE_SUITES})
sith_add_ctest(sith.AIUtil 60 ${SITH_VEC_AIUTIL_SUITES})
