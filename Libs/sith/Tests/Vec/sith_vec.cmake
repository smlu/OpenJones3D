# Original-engine vector suites of the sith world-state harness (see README.md). Included by
# Libs/sith/Tests/CMakeLists.txt. Each suite compiles its tested sources with its own seam renames.
#
# sith_add_vec_suite(<prefix>
#     SUITES  <suites>                      (Unity group runners)
#     SOURCES <test sources>                (the suite's test source)
#     TESTED  <tested sources>              (compiled with the seam renames: name -> sithVec_name)
#     SCAN    <files>                       (the production sources the tested sources include, and the suite's headers)
#     SEAMS   <seams .def files>            (SITHVEC_SEAM(name) / SITHVEC_LOOKUP(name) lines)
#     ADDRS   <address header>              (force-included after sithVecTestAddrs.h)
#     TV_DIR  <fixture directory>)          (SITHVEC_TV_DIR)
set(SITH_VEC_DIR "${CMAKE_CURRENT_LIST_DIR}")
file(GLOB SITH_VEC_HEADERS "${SITH_VEC_DIR}/*.h")

function(sith_add_vec_suite PREFIX)
    cmake_parse_arguments(SV "" "ADDRS;TV_DIR" "SUITES;SOURCES;TESTED;SCAN;SEAMS" ${ARGN})

    # Re-run the configuration when a seams file changes
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SV_SEAMS})

    set(SV_RENAMES "")
    foreach(SV_SEAM_FILE IN LISTS SV_SEAMS)
        file(STRINGS "${SV_SEAM_FILE}" SV_SEAM_LINES REGEX "^SITHVEC_(SEAM|LOOKUP)\\(")
        foreach(SV_SEAM_LINE IN LISTS SV_SEAM_LINES)
            string(REGEX REPLACE "^SITHVEC_[A-Z]+\\(([A-Za-z0-9_]+)\\).*$" "\\1" SV_SEAM_NAME "${SV_SEAM_LINE}")
            list(APPEND SV_RENAMES "${SV_SEAM_NAME}=sithVec_${SV_SEAM_NAME}")
        endforeach()
    endforeach()

    if (SV_RENAMES)
        set_property(SOURCE ${SV_TESTED} PROPERTY COMPILE_DEFINITIONS ${SV_RENAMES})
    endif()

    # Engine globals of the original executable are redirected to static memory, rand to the deterministic stub
    if (MSVC)
        set_property(SOURCE ${SV_TESTED} ${SV_SOURCES} APPEND PROPERTY COMPILE_OPTIONS
            "/FI${SITH_VEC_DIR}/sithVecTestAddrs.h" "/FI${SV_ADDRS}")
    endif()

    # The storage of the redirected engine globals (SITHVEC_REDIRECT_DEFINE(type, name) -> sithVecTest_g_name)
    set(SV_REDIRECTS "")
    foreach(SV_SOURCE IN LISTS SV_SOURCES)
        file(STRINGS "${SV_SOURCE}" SV_REDIRECT_LINES REGEX "^SITHVEC_REDIRECT_DEFINE\\(")
        foreach(SV_REDIRECT_LINE IN LISTS SV_REDIRECT_LINES)
            string(REGEX REPLACE "^SITHVEC_REDIRECT_DEFINE\\([^,]+,[ \t]*([A-Za-z0-9_]+)[^)]*\\).*$" "\\1" SV_REDIRECT_NAME "${SV_REDIRECT_LINE}")
            list(APPEND SV_REDIRECTS "sithVecTest_g_${SV_REDIRECT_NAME}")
        endforeach()
    endforeach()

    sith_add_test_suite(${PREFIX}
        SUITES      ${SV_SUITES}
        TESTS       ${SV_SOURCES}
        TESTED      ${SV_TESTED}
        SCAN        ${SV_SCAN} ${SITH_VEC_HEADERS}
        DEFINITIONS
            J3D_TEST
            J3D_LOCAL_SITH_RTI
            SITHVEC_TV_DIR="${SV_TV_DIR}"
        NAMES       ${SV_REDIRECTS}
    )

    set(SITH_TESTS_SOURCES ${SITH_TESTS_SOURCES} PARENT_SCOPE)
    set(SITH_TESTS_SUITES ${SITH_TESTS_SUITES} PARENT_SCOPE)
endfunction()

set(SITH_DIR "${SITH_VEC_DIR}/../..")
file(GLOB SITH_AI_VEC_HEADERS "${SITH_DIR}/AI/Tests/*.h")
file(GLOB SITH_WORLD_VEC_HEADERS "${SITH_DIR}/World/Tests/*Vec*.h")

# sithThing_CreateQuetzUserBlock, sithThing_UpdateQuetzUserBlock
set(SITH_VEC_THINGQUETZ_SUITES sithThingQuetz)
sith_add_vec_suite(sithVecThingQuetz
    SUITES  ${SITH_VEC_THINGQUETZ_SUITES}
    SOURCES "${SITH_DIR}/World/Tests/sithThingQuetzVecTest.c"
    TESTED  "${SITH_DIR}/World/Tests/sithThingVecTested.c"
    SCAN    "${SITH_DIR}/World/sithThing.c" ${SITH_WORLD_VEC_HEADERS}
    SEAMS   "${SITH_DIR}/World/Tests/sithThingQuetzVecSeams.def"
    ADDRS   "${SITH_DIR}/World/Tests/sithThingQuetzVecAddrs.h"
    TV_DIR  "${SITH_DIR}/World/Tests/tv/sithThingQuetz"
)

# sithAIInstinct_InitInstincts and the instinct registry of sithAI.c (sithAI_RegisterInstinct, sithAI_FindInstinct)
set(SITH_VEC_AIINSTINCT_SUITES sithAIInstinct)
sith_add_vec_suite(sithVecAIInstinct
    SUITES  ${SITH_VEC_AIINSTINCT_SUITES}
    SOURCES "${SITH_DIR}/AI/Tests/sithAIInstinctVecTest.c"
    # sithAIInstinct.c for InitInstincts, so it registers into this suite's copy of the registry of sithAI.c
    TESTED  "${SITH_DIR}/AI/Tests/sithAIVecTested.c" "${SITH_DIR}/AI/sithAIInstinct.c"
    SCAN    "${SITH_DIR}/AI/sithAI.c" ${SITH_AI_VEC_HEADERS}
    SEAMS   "${SITH_DIR}/AI/Tests/sithAIInstinctVecSeams.def"
    ADDRS   "${SITH_DIR}/AI/Tests/sithAIInstinctVecAddrs.h"
    TV_DIR  "${SITH_DIR}/AI/Tests/tv/sithAIInstinct"
)

# The instinct functions of sithAIInstinct.c (all but sithAIInstinct_InitInstincts)
set(SITH_VEC_AIINSTINCTFN_SUITES sithAIInstinctFn sithAIInstinctFix)
sith_add_vec_suite(sithVecAIInstinctFn
    SUITES  ${SITH_VEC_AIINSTINCTFN_SUITES}
    SOURCES "${SITH_DIR}/AI/Tests/sithAIInstinctFnVecTest.c"
    TESTED  "${SITH_DIR}/AI/Tests/sithAIInstinctFnVecTested.c"
    SCAN    "${SITH_DIR}/AI/sithAIInstinct.c" ${SITH_AI_VEC_HEADERS}
    SEAMS   "${SITH_DIR}/AI/Tests/sithAIInstinctFnVecSeams.def"
    ADDRS   "${SITH_DIR}/AI/Tests/sithAIInstinctFnVecAddrs.h"
    TV_DIR  "${SITH_DIR}/AI/Tests/tv/sithAIInstinctFn"
)

# sithAIMove: the update, turn, move, look, special move, head tracking and Quetzalcoatl strike functions
set(SITH_VEC_AIMOVE_SUITES sithAIMove)
if (J3D_QOL_IMPROVEMENTS)
    list(APPEND SITH_VEC_AIMOVE_SUITES sithAIMoveFix)
endif()
sith_add_vec_suite(sithVecAIMove
    SUITES  ${SITH_VEC_AIMOVE_SUITES}
    SOURCES "${SITH_DIR}/AI/Tests/sithAIMoveVecTest.c"
    TESTED  "${SITH_DIR}/AI/Tests/sithAIMoveVecTested.c"
    SCAN    "${SITH_DIR}/AI/sithAIMove.c" ${SITH_AI_VEC_HEADERS}
    SEAMS   "${SITH_DIR}/AI/Tests/sithAIMoveVecSeams.def"
    ADDRS   "${SITH_DIR}/AI/Tests/sithAIMoveVecAddrs.h"
    TV_DIR  "${SITH_DIR}/AI/Tests/tv/sithAIMove"
)

# sithAIUtil: waypoints, sight and distance, firing, path checks and point creation, things in view
set(SITH_VEC_AIUTIL_SUITES sithAIUtil sithAIUtilFix)
sith_add_vec_suite(sithVecAIUtil
    SUITES  ${SITH_VEC_AIUTIL_SUITES}
    SOURCES "${SITH_DIR}/AI/Tests/sithAIUtilVecTest.c"
    TESTED  "${SITH_DIR}/AI/Tests/sithAIUtilVecTested.c"
    SCAN    "${SITH_DIR}/AI/sithAIUtil.c" ${SITH_AI_VEC_HEADERS}
    SEAMS   "${SITH_DIR}/AI/Tests/sithAIUtilVecSeams.def"
    ADDRS   "${SITH_DIR}/AI/Tests/sithAIUtilVecAddrs.h"
    TV_DIR  "${SITH_DIR}/AI/Tests/tv/sithAIUtil"
)
