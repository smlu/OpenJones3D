add_executable(rdCacheTests
    rdCacheTest.c
    rdCacheTestHost.h
    ../rdCache.c
)

if (MSVC)
    target_compile_options(rdCacheTests PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/rdCacheTestHost.h")
else()
    target_compile_options(rdCacheTests PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/rdCacheTestHost.h")
endif()

target_compile_definitions(rdCacheTests PRIVATE J3D_LOCAL_RTI)

target_link_libraries(rdCacheTests PRIVATE rdroid unity j3dTestSupport)
set_target_properties(rdCacheTests PROPERTIES FOLDER "Tests/Libs/rdroid")
add_test(NAME rdroid.Cache COMMAND rdCacheTests)
set_tests_properties(rdroid.Cache PROPERTIES LABELS "unit;rdroid;cache" TIMEOUT 60)
