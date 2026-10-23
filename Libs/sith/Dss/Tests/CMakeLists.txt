# Original-engine vector tests of the DSS (save/network) serialization functions.
# The tested sources run on synthetic engine state; the engine functions they call are replaced by the stubs of
# sithDSSVec*.h through the rename list of sithDSSSeams.def (name -> dssv_name).
set(SITHDSS_TEST_SOURCES
    sithDSSTestRunner.c
    sithDSSTest.c
)

set(SITHDSS_TESTED_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../sithDSS.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../sithDSSThing.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../sithMulti.c"
)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/sithDSSSeams.def" SITHDSS_SEAM_LINES REGEX "^DSSV_(SEAM|LOOKUP)\\(")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/sithDSSThingSeams.def" SITHDSS_THING_SEAM_LINES REGEX "^DSSV_(SEAM|LOOKUP)\\(")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/sithMultiSeams.def" SITHMULTI_SEAM_LINES REGEX "^DSSV_(SEAM|LOOKUP)\\(")
list(APPEND SITHDSS_SEAM_LINES ${SITHDSS_THING_SEAM_LINES} ${SITHMULTI_SEAM_LINES})
set(SITHDSS_SEAM_RENAMES "")
foreach(SEAM_LINE IN LISTS SITHDSS_SEAM_LINES)
    string(REGEX REPLACE "^DSSV_[A-Z]+\\(([A-Za-z0-9_]+)\\).*$" "\\1" SEAM_NAME "${SEAM_LINE}")
    list(APPEND SITHDSS_SEAM_RENAMES "${SEAM_NAME}=dssv_${SEAM_NAME}")
endforeach()

set_property(SOURCE ${SITHDSS_TESTED_SOURCES} PROPERTY COMPILE_DEFINITIONS ${SITHDSS_SEAM_RENAMES})

# The engine globals of the original executable are redirected to static memory in the tested sources and the tests
if (MSVC)
    set_property(SOURCE ${SITHDSS_TESTED_SOURCES} ${SITHDSS_TEST_SOURCES} APPEND PROPERTY COMPILE_OPTIONS
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/sithDSSTestAddrs.h")
endif()

add_executable(sithDSSTests
    ${SITHDSS_TEST_SOURCES}
    ${SITHDSS_TESTED_SOURCES}
)

target_link_libraries(sithDSSTests PRIVATE
    sith
    sound
    unity
    w32util
)

target_compile_definitions(sithDSSTests PRIVATE
    J3D_TEST
    J3D_LOCAL_SITH_RTI
    SITH_DSS_TV_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tv/sithDSS"
)

# The records compare synthetic state with the addresses of that state normalized (see dssv_NormWord); that only works
# when the addresses cannot be confused with the generated integers and floats. Load the image high and fixed.
if (MSVC)
    target_link_options(sithDSSTests PRIVATE /BASE:0x20000000 /DYNAMICBASE:NO)
endif()

set_target_properties(sithDSSTests PROPERTIES
    FOLDER "Tests/Libs/sith"
)

source_group("Tests" FILES ${SITHDSS_TEST_SOURCES})
source_group("Tested Sources" FILES ${SITHDSS_TESTED_SOURCES})

set(SITHDSS_TEST_BACKEND_LABEL "")
if (J3D_DIRECTX6)
    set(SITHDSS_TEST_BACKEND_LABEL "DirectX6")
elseif (J3D_DIRECTX9)
    set(SITHDSS_TEST_BACKEND_LABEL "DirectX9")
endif()

add_test(NAME sith.DSS COMMAND sithDSSTests)
set_tests_properties(sith.DSS PROPERTIES
    LABELS "unit;sith;${SITHDSS_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)
