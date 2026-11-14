# Original-engine vector suites of the DSS (save/network) serialization and of sithMulti. Included by
# Libs/sith/Tests/CMakeLists.txt.
set(SITH_DSS_TEST_DIR "${CMAKE_CURRENT_LIST_DIR}")
file(GLOB SITH_DSS_TEST_HEADERS "${SITH_DSS_TEST_DIR}/*.h")

# The tested sources run on synthetic engine state; the engine functions they call are replaced by the stubs of
# sithDSSVec*.h through the rename list of sithDSSSeams.def (name -> dssv_name).
set(SITHDSS_TEST_SOURCES
    "${SITH_DSS_TEST_DIR}/sithDSSTest.c"
)

set(SITHDSS_TESTED_SOURCES
    "${SITH_DSS_TEST_DIR}/../sithDSS.c"
    "${SITH_DSS_TEST_DIR}/../sithDSSThing.c"
    "${SITH_DSS_TEST_DIR}/../sithMulti.c"
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${SITH_DSS_TEST_DIR}/sithDSSSeams.def"
    "${SITH_DSS_TEST_DIR}/sithDSSThingSeams.def"
    "${SITH_DSS_TEST_DIR}/sithMultiDssSeams.def"
    "${SITH_DSS_TEST_DIR}/sithMultiSeams.def"
)

file(STRINGS "${SITH_DSS_TEST_DIR}/sithDSSSeams.def" SITHDSS_SEAM_LINES REGEX "^DSSV_(SEAM|LOOKUP)\\(")
file(STRINGS "${SITH_DSS_TEST_DIR}/sithDSSThingSeams.def" SITHDSS_THING_SEAM_LINES REGEX "^DSSV_(SEAM|LOOKUP)\\(")
file(STRINGS "${SITH_DSS_TEST_DIR}/sithMultiDssSeams.def" SITHMULTI_DSS_SEAM_LINES REGEX "^DSSV_(SEAM|LOOKUP)\\(")
list(APPEND SITHDSS_SEAM_LINES ${SITHDSS_THING_SEAM_LINES} ${SITHMULTI_DSS_SEAM_LINES})
set(SITHDSS_SEAM_RENAMES "")
foreach(SEAM_LINE IN LISTS SITHDSS_SEAM_LINES)
    string(REGEX REPLACE "^DSSV_[A-Z]+\\(([A-Za-z0-9_]+)\\).*$" "\\1" SEAM_NAME "${SEAM_LINE}")
    list(APPEND SITHDSS_SEAM_RENAMES "${SEAM_NAME}=dssv_${SEAM_NAME}")
endforeach()

set_property(SOURCE ${SITHDSS_TESTED_SOURCES} PROPERTY COMPILE_DEFINITIONS ${SITHDSS_SEAM_RENAMES})

# The engine globals of the original executable are redirected to static memory in the tested sources and the tests
if (MSVC)
    set_property(SOURCE ${SITHDSS_TESTED_SOURCES} ${SITHDSS_TEST_SOURCES} APPEND PROPERTY COMPILE_OPTIONS
        "/FI${SITH_DSS_TEST_DIR}/sithDSSTestAddrs.h")
endif()

set(SITH_DSS_SUITES
    sithDSS
    sithDSSThing
    sithDSSFix
)

sith_add_test_suite(sithDSSSuite
    SUITES      ${SITH_DSS_SUITES}
    TESTS       ${SITHDSS_TEST_SOURCES}
    TESTED      ${SITHDSS_TESTED_SOURCES}
    SCAN        ${SITH_DSS_TEST_HEADERS}
    DEFINITIONS
        J3D_TEST
        J3D_LOCAL_SITH_RTI
        SITH_DSS_TV_DIR="${SITH_DSS_TEST_DIR}/tv/sithDSS"
)

# Original-engine vector tests of sithMulti (multiplayer messages and state). sithMulti.c is compiled in
# sithMultiTested.c (for access to its module state); its engine calls are replaced by the stubs of sithMultiVec.h
# through the rename list of sithMultiSeams.def (name -> msv_name). The cases reuse the DSS state builder.
set(SITHMULTI_TEST_SOURCES
    "${SITH_DSS_TEST_DIR}/sithMultiTest.c"
)

set(SITHMULTI_TESTED_SOURCES
    "${SITH_DSS_TEST_DIR}/sithMultiTested.c"
)

file(STRINGS "${SITH_DSS_TEST_DIR}/sithMultiSeams.def" SITHMULTI_SEAM_LINES REGEX "^MSV_SEAM\\(")
set(SITHMULTI_SEAM_RENAMES "")
foreach(SEAM_LINE IN LISTS SITHMULTI_SEAM_LINES)
    string(REGEX REPLACE "^MSV_SEAM\\(([A-Za-z0-9_]+)\\).*$" "\\1" SEAM_NAME "${SEAM_LINE}")
    list(APPEND SITHMULTI_SEAM_RENAMES "${SEAM_NAME}=msv_${SEAM_NAME}")
endforeach()

set_property(SOURCE ${SITHMULTI_TESTED_SOURCES} PROPERTY COMPILE_DEFINITIONS ${SITHMULTI_SEAM_RENAMES})

if (MSVC)
    set_property(SOURCE ${SITHMULTI_TESTED_SOURCES} ${SITHMULTI_TEST_SOURCES} APPEND PROPERTY COMPILE_OPTIONS
        "/FI${SITH_DSS_TEST_DIR}/sithMultiTestAddrs.h")
endif()

set(SITH_MULTI_SUITES
    sithMulti
)

sith_add_test_suite(sithMultiSuite
    SUITES      ${SITH_MULTI_SUITES}
    TESTS       ${SITHMULTI_TEST_SOURCES}
    TESTED      ${SITHMULTI_TESTED_SOURCES}
    SCAN        "${SITH_DSS_TEST_DIR}/../sithMulti.c" ${SITH_DSS_TEST_HEADERS}
    DEFINITIONS
        J3D_TEST
        J3D_LOCAL_SITH_RTI
        SITH_MULTI_TV_DIR="${SITH_DSS_TEST_DIR}/tv/sithMulti"
)
