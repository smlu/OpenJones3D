set(RDROID_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Math: vector and matrix functions
set(RDROID_MATH_TEST_SOURCES
    "${RDROID_SOURCE_DIR}/Math/Tests/rdMathTest.h"
    "${RDROID_SOURCE_DIR}/Math/Tests/rdVectorTest.c"
    "${RDROID_SOURCE_DIR}/Math/Tests/rdMatrixTest.c"
    "${RDROID_SOURCE_DIR}/Math/rdVector.c"
    "${RDROID_SOURCE_DIR}/Math/rdMatrix.c"
)

# Model3: per-draw face flags, with the cache queue and clipping replaced by CPU fixtures
set(RDROID_MODEL3_TEST_SOURCES
    "${RDROID_SOURCE_DIR}/Primitives/Tests/rdModel3Test.c"
    "${RDROID_SOURCE_DIR}/Primitives/rdModel3.c"
)

# Cache: alpha ordering and batch boundaries
set(RDROID_CACHE_TEST_SOURCES
    "${RDROID_SOURCE_DIR}/Raster/Tests/rdCacheTest.c"
    "${RDROID_SOURCE_DIR}/Raster/rdCache.c"
)

add_executable(rdroidTests
    rdroidTest.c
    rdroidTestHost.h
    "${RDROID_SOURCE_DIR}/Primitives/Tests/rdModel3TestHost.h"
    "${RDROID_SOURCE_DIR}/Raster/Tests/rdCacheTestHost.h"
    ${RDROID_MATH_TEST_SOURCES}
    ${RDROID_MODEL3_TEST_SOURCES}
    ${RDROID_CACHE_TEST_SOURCES}
)

# Each area force-includes its test host header, which relocates the RTI addresses of its translation units before
# inline helpers expand them. rdroidTestHost.h is shared by all of them.
function(rdroid_test_force_include HEADER)
    if (MSVC)
        set(OPTION "/FI${HEADER}")
    else()
        set(OPTION -include "${HEADER}")
    endif()
    set_property(SOURCE ${ARGN} APPEND PROPERTY COMPILE_OPTIONS ${OPTION})
endfunction()

rdroid_test_force_include("${CMAKE_CURRENT_SOURCE_DIR}/rdroidTestHost.h" ${RDROID_MATH_TEST_SOURCES})
rdroid_test_force_include("${RDROID_SOURCE_DIR}/Primitives/Tests/rdModel3TestHost.h" ${RDROID_MODEL3_TEST_SOURCES})
rdroid_test_force_include("${RDROID_SOURCE_DIR}/Raster/Tests/rdCacheTestHost.h" ${RDROID_CACHE_TEST_SOURCES})
set_property(SOURCE ${RDROID_CACHE_TEST_SOURCES} APPEND PROPERTY COMPILE_DEFINITIONS J3D_LOCAL_RTI)

# The complete model translation unit also retains material/configuration symbols.
# This test executable never opens a window or graphics device.
target_link_libraries(rdroidTests PRIVATE
    rdroid
    unity
    j3dTestSupport
    w32util
)

jones3d_add_test_suites(rdroidTests
    UNIT
        rdVector
        rdMatrix
        rdModel3
        rdCache
)

set_target_properties(rdroidTests PROPERTIES
    FOLDER "Tests/Libs/rdroid"
)

source_group("Test Support" FILES rdroidTest.c rdroidTestHost.h)
source_group("Tests\\Math" FILES ${RDROID_MATH_TEST_SOURCES})
source_group("Tests\\Model3" FILES ${RDROID_MODEL3_TEST_SOURCES} "${RDROID_SOURCE_DIR}/Primitives/Tests/rdModel3TestHost.h")
source_group("Tests\\Cache" FILES ${RDROID_CACHE_TEST_SOURCES} "${RDROID_SOURCE_DIR}/Raster/Tests/rdCacheTestHost.h")

add_test(NAME rdroid.Math COMMAND rdroidTests --suite rdVector,rdMatrix)
set_tests_properties(rdroid.Math PROPERTIES
    LABELS "unit;rdroid;math"
    TIMEOUT 60
)

add_test(NAME rdroid.Model3 COMMAND rdroidTests --suite rdModel3)
set_tests_properties(rdroid.Model3 PROPERTIES
    LABELS "unit;rdroid;primitives"
    TIMEOUT 60
)

add_test(NAME rdroid.Cache COMMAND rdroidTests --suite rdCache)
set_tests_properties(rdroid.Cache PROPERTIES
    LABELS "unit;rdroid;cache"
    TIMEOUT 60
)
