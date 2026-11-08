add_executable(rdModel3Tests
    rdModel3Test.c
    rdModel3TestHost.h
    ../rdModel3.c
    ../../Math/rdMatrix.c
    ../../Math/rdVector.c
)

if(MSVC)
    target_compile_options(rdModel3Tests PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/rdModel3TestHost.h")
else()
    target_compile_options(rdModel3Tests PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/rdModel3TestHost.h")
endif()

# The complete model translation unit also retains material/configuration symbols.
# This test executable never opens a window or graphics device.
target_link_libraries(rdModel3Tests PRIVATE rdroid unity j3dTestSupport w32util)
set_target_properties(rdModel3Tests PROPERTIES FOLDER "Tests/Libs/rdroid")

add_test(NAME rdroid.Model3 COMMAND rdModel3Tests)
set_tests_properties(rdroid.Model3 PROPERTIES LABELS "unit;rdroid;primitives" TIMEOUT 60)
