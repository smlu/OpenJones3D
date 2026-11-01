# Original-engine vector suites of the sith world-state harness (see README.md). Every suite is its own executable so
# each can compile its tested sources with its own seam renames.
#
# sith_add_vec_test(<target>
#     TEST    <CTest name>
#     SOURCES <test sources>                (the suite's test source; the shared runner is added)
#     TESTED  <tested sources>              (compiled with the seam renames: name -> sithVec_name)
#     SEAMS   <seams .def files>            (SITHVEC_SEAM(name) / SITHVEC_LOOKUP(name) lines)
#     ADDRS   <address header>              (force-included after sithVecTestAddrs.h)
#     TV_DIR  <fixture directory>)          (SITHVEC_TV_DIR)
function(sith_add_vec_test TARGET)
    cmake_parse_arguments(SV "" "TEST;ADDRS;TV_DIR" "SOURCES;TESTED;SEAMS" ${ARGN})

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
            "/FI${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sithVecTestAddrs.h" "/FI${SV_ADDRS}")
    endif()

    add_executable(${TARGET}
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sithVecTestRunner.c"
        ${SV_SOURCES}
        ${SV_TESTED}
    )

    # Same force-include as the MSVC /FI above; per-source set_property does not expand the SHELL:
    # prefix, so the option is target-wide (it also reaches the shared runner).
    if (MINGW)
        target_compile_options(${TARGET} PRIVATE
            "SHELL:-include ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sithVecTestAddrs.h"
            "SHELL:-include ${SV_ADDRS}")
    endif()

    target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")

    target_link_libraries(${TARGET} PRIVATE
        sith
        sound
        unity
        w32util
    )

    target_compile_definitions(${TARGET} PRIVATE
        J3D_TEST
        J3D_LOCAL_SITH_RTI
        SITHVEC_TV_DIR="${SV_TV_DIR}"
    )

    # The records normalize the addresses of the synthetic state (sithVec_NormWord); that only works when they cannot
    # be confused with the generated integers and floats. Load the image high and fixed.
    if (MSVC)
        target_link_options(${TARGET} PRIVATE /BASE:0x20000000 /DYNAMICBASE:NO)
    elseif(MINGW)
        target_link_options(${TARGET} PRIVATE -Wl,--image-base=0x20000000 -Wl,--disable-dynamicbase)
    endif()

    set_target_properties(${TARGET} PROPERTIES
        FOLDER "Tests/Libs/sith"
    )

    source_group("Tests" FILES ${SV_SOURCES})
    source_group("Tested Sources" FILES ${SV_TESTED})

    set(SV_BACKEND_LABEL "")
    if (J3D_DIRECTX6)
        set(SV_BACKEND_LABEL "DirectX6")
    elseif (J3D_DIRECTX9)
        set(SV_BACKEND_LABEL "DirectX9")
    endif()

    add_test(NAME ${SV_TEST} COMMAND ${TARGET})
    set_tests_properties(${SV_TEST} PROPERTIES
        LABELS "unit;sith;${SV_BACKEND_LABEL}"
        TIMEOUT 60
    )
endfunction()

set(SITH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# sithThing_CreateQuetzUserBlock, sithThing_UpdateQuetzUserBlock
sith_add_vec_test(sithThingQuetzVecTests
    TEST    sith.ThingQuetz
    SOURCES "${SITH_DIR}/World/Tests/sithThingQuetzVecTest.c"
    TESTED  "${SITH_DIR}/World/Tests/sithThingVecTested.c"
    SEAMS   "${SITH_DIR}/World/Tests/sithThingQuetzVecSeams.def"
    ADDRS   "${SITH_DIR}/World/Tests/sithThingQuetzVecAddrs.h"
    TV_DIR  "${SITH_DIR}/World/Tests/tv/sithThingQuetz"
)

# sithAIInstinct_InitInstincts and the instinct registry of sithAI.c (sithAI_RegisterInstinct, sithAI_FindInstinct)
sith_add_vec_test(sithAIInstinctVecTests
    TEST    sith.AIInstinct
    SOURCES "${SITH_DIR}/AI/Tests/sithAIInstinctVecTest.c"
    TESTED  "${SITH_DIR}/AI/Tests/sithAIVecTested.c"
    SEAMS   "${SITH_DIR}/AI/Tests/sithAIInstinctVecSeams.def"
    ADDRS   "${SITH_DIR}/AI/Tests/sithAIInstinctVecAddrs.h"
    TV_DIR  "${SITH_DIR}/AI/Tests/tv/sithAIInstinct"
)

# The instinct functions of sithAIInstinct.c (all but sithAIInstinct_InitInstincts)
sith_add_vec_test(sithAIInstinctFnVecTests
    TEST    sith.AIInstinctFn
    SOURCES "${SITH_DIR}/AI/Tests/sithAIInstinctFnVecTest.c"
    TESTED  "${SITH_DIR}/AI/Tests/sithAIInstinctFnVecTested.c"
    SEAMS   "${SITH_DIR}/AI/Tests/sithAIInstinctFnVecSeams.def"
    ADDRS   "${SITH_DIR}/AI/Tests/sithAIInstinctFnVecAddrs.h"
    TV_DIR  "${SITH_DIR}/AI/Tests/tv/sithAIInstinctFn"
)
# Many cases (the fixture has more than a thousand): allow slow runners
set_tests_properties(sith.AIInstinctFn PROPERTIES TIMEOUT 180)
# sithAIMove: the update, turn, move, look, special move, head tracking and Quetzalcoatl strike functions
sith_add_vec_test(sithAIMoveVecTests
    TEST    sith.AIMove
    SOURCES "${SITH_DIR}/AI/Tests/sithAIMoveVecTest.c"
    TESTED  "${SITH_DIR}/AI/Tests/sithAIMoveVecTested.c"
    SEAMS   "${SITH_DIR}/AI/Tests/sithAIMoveVecSeams.def"
    ADDRS   "${SITH_DIR}/AI/Tests/sithAIMoveVecAddrs.h"
    TV_DIR  "${SITH_DIR}/AI/Tests/tv/sithAIMove"
)
# sithAIUtil: waypoints, sight and distance, firing, path checks and point creation, things in view
sith_add_vec_test(sithAIUtilVecTests
    TEST    sith.AIUtil
    SOURCES "${SITH_DIR}/AI/Tests/sithAIUtilVecTest.c"
    TESTED  "${SITH_DIR}/AI/Tests/sithAIUtilVecTested.c"
    SEAMS   "${SITH_DIR}/AI/Tests/sithAIUtilVecSeams.def"
    ADDRS   "${SITH_DIR}/AI/Tests/sithAIUtilVecAddrs.h"
    TV_DIR  "${SITH_DIR}/AI/Tests/tv/sithAIUtil"
)
