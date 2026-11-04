# Original-engine vector tests of the DSS (save/network) serialization functions.
# The tested sources run on synthetic engine state; the engine functions they call are replaced by the stubs of
# sithDSSVec*.h through the rename list of sithDSSSeams.def (name -> dssv_name).
#
# Added: both suites of this directory link at a fixed high base with the GNU ld options
# -no-pie -Wl,-Ttext-segment=0x20000000 below (see the same note in Tests/Vec/CMakeLists.txt), which
# is what keeps the normalized addresses out of the generated integers and floats. ld64 (macOS) has
# no equivalent and rejects -Ttext-segment, so these suites are not built on Apple hosts.
if (APPLE)
    return()
endif()

set(SITHDSS_TEST_SOURCES
    sithDSSTestRunner.c
    sithDSSTest.c
)

set(SITHDSS_TESTED_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../sithDSS.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../sithDSSThing.c"
)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/sithDSSSeams.def" SITHDSS_SEAM_LINES REGEX "^DSSV_(SEAM|LOOKUP)\\(")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/sithDSSThingSeams.def" SITHDSS_THING_SEAM_LINES REGEX "^DSSV_(SEAM|LOOKUP)\\(")
list(APPEND SITHDSS_SEAM_LINES ${SITHDSS_THING_SEAM_LINES})
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

# Same force-include as the MSVC /FI above; the target contains only the two source lists, so the
# option scope is identical. (Per-source set_property does not expand the SHELL: prefix.)
# Altered: MinGW only before; the native Clang/GCC (Null) build needs the same -include, and it is a
# GNU-compiler option, so the condition is now every non-MSVC compiler. Windows behavior is unchanged.
if (NOT MSVC)
    target_compile_options(sithDSSTests PRIVATE
        "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/sithDSSTestAddrs.h")
endif()

target_link_libraries(sithDSSTests PRIVATE
    sith
    sound
    unity
    w32util
)

# Added: the Babylon column runs this suite under the adapter context.
std_use_babylon_test_main(sithDSSTests "${CMAKE_CURRENT_SOURCE_DIR}/sithDSSTestRunner.c")

target_compile_definitions(sithDSSTests PRIVATE
    J3D_TEST
    SITH_DSS_TV_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tv/sithDSS"
)

# The records compare synthetic state with the addresses of that state normalized (see dssv_NormWord); that only works
# when the addresses cannot be confused with the generated integers and floats. Load the image high and fixed.
if (MSVC)
    target_link_options(sithDSSTests PRIVATE /BASE:0x20000000 /DYNAMICBASE:NO)
elseif(MINGW)
    target_link_options(sithDSSTests PRIVATE -Wl,--image-base=0x20000000 -Wl,--disable-dynamicbase)
elseif(J3D_PLATFORM_NULL)
    # Added: The Null platform needs the same fixed, high load address for the same reason (see
    # sithVecTest_AssertArenaPlacement): a non-PIE image with its text segment at 0x20000000.
    target_link_options(sithDSSTests PRIVATE -no-pie -Wl,-Ttext-segment=0x20000000)
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

# Original-engine vector tests of sithMulti (multiplayer messages and state). sithMulti.c is compiled in
# sithMultiTested.c (for access to its module state); its engine calls are replaced by the stubs of sithMultiVec.h
# through the rename list of sithMultiSeams.def (name -> msv_name). The cases reuse the DSS state builder.
set(SITHMULTI_TEST_SOURCES
    sithMultiTestRunner.c
    sithMultiTest.c
)

set(SITHMULTI_TESTED_SOURCES
    sithMultiTested.c
)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/sithMultiSeams.def" SITHMULTI_SEAM_LINES REGEX "^MSV_SEAM\\(")
set(SITHMULTI_SEAM_RENAMES "")
foreach(SEAM_LINE IN LISTS SITHMULTI_SEAM_LINES)
    string(REGEX REPLACE "^MSV_SEAM\\(([A-Za-z0-9_]+)\\).*$" "\\1" SEAM_NAME "${SEAM_LINE}")
    list(APPEND SITHMULTI_SEAM_RENAMES "${SEAM_NAME}=msv_${SEAM_NAME}")
endforeach()

set_property(SOURCE ${SITHMULTI_TESTED_SOURCES} PROPERTY COMPILE_DEFINITIONS ${SITHMULTI_SEAM_RENAMES})

if (MSVC)
    set_property(SOURCE ${SITHMULTI_TESTED_SOURCES} ${SITHMULTI_TEST_SOURCES} APPEND PROPERTY COMPILE_OPTIONS
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/sithMultiTestAddrs.h")
endif()

add_executable(sithMultiTests
    ${SITHMULTI_TEST_SOURCES}
    ${SITHMULTI_TESTED_SOURCES}
)

# Same force-include as the MSVC /FI above; the target contains only the two source lists, so the
# option scope is identical. (Per-source set_property does not expand the SHELL: prefix.)
# Altered: MinGW only before; the native Clang/GCC (Null) build needs the same -include, and it is a
# GNU-compiler option, so the condition is now every non-MSVC compiler. Windows behavior is unchanged.
if (NOT MSVC)
    target_compile_options(sithMultiTests PRIVATE
        "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/sithMultiTestAddrs.h")
endif()

target_link_libraries(sithMultiTests PRIVATE
    sith
    sound
    unity
    w32util
)

# Added: the Babylon column runs this suite under the adapter context.
std_use_babylon_test_main(sithMultiTests "${CMAKE_CURRENT_SOURCE_DIR}/sithMultiTestRunner.c")

target_compile_definitions(sithMultiTests PRIVATE
    J3D_TEST
    SITH_MULTI_TV_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tv/sithMulti"
)

if (MSVC)
    target_link_options(sithMultiTests PRIVATE /BASE:0x20000000 /DYNAMICBASE:NO)
elseif(MINGW)
    target_link_options(sithMultiTests PRIVATE -Wl,--image-base=0x20000000 -Wl,--disable-dynamicbase)
elseif(J3D_PLATFORM_NULL)
    # Added: The Null platform needs the same fixed, high load address for the same reason (see
    # sithVecTest_AssertArenaPlacement): a non-PIE image with its text segment at 0x20000000.
    target_link_options(sithMultiTests PRIVATE -no-pie -Wl,-Ttext-segment=0x20000000)
endif()

set_target_properties(sithMultiTests PROPERTIES
    FOLDER "Tests/Libs/sith"
)

source_group("Tests" FILES ${SITHMULTI_TEST_SOURCES})
source_group("Tested Sources" FILES ${SITHMULTI_TESTED_SOURCES})

add_test(NAME sith.Multi COMMAND sithMultiTests)
set_tests_properties(sith.Multi PROPERTIES
    LABELS "unit;sith;${SITHDSS_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)
