add_executable(rdroidTests
    rdroidTestRunner.c
    ../Math/Tests/rdMathTest.h
    ../Math/Tests/rdMathTestHost.h
    ../Math/Tests/rdVectorTest.c
    ../Math/Tests/rdMatrixTest.c
    ../Math/rdVector.c
    ../Math/rdMatrix.c
)

# Relocate the test build's host slot before compiling the real math sources.
if (MSVC)
    target_compile_options(rdroidTests PRIVATE
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/../Math/Tests/rdMathTestHost.h"
    )
else()
    target_compile_options(rdroidTests PRIVATE
        -include "${CMAKE_CURRENT_SOURCE_DIR}/../Math/Tests/rdMathTestHost.h"
    )
endif()

target_link_libraries(rdroidTests PRIVATE
    rdroid
    unity
    j3dTestSupport
)

set_target_properties(rdroidTests PROPERTIES
    FOLDER "Tests/Libs/rdroid"
)

add_test(NAME rdroid.Math COMMAND rdroidTests)
set_tests_properties(rdroid.Math PROPERTIES
    LABELS "unit;rdroid;math"
    TIMEOUT 60
)
