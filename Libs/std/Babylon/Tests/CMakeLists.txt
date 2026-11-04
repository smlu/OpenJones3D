# Added: the Babylon column's stdPlatform time and files adapter tests. Only the Babylon column
# builds them (guarded by the caller), matching how stdTests is registered in Libs/std/Tests.
set(STDBABYLON_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_executable(stdBabylonTests
    stdPlatformTimeBabylonTest.cpp
    stdFileBabylonTest.cpp
    "${STDBABYLON_SOURCE_DIR}/stdBabylon.cpp"
    "${STDBABYLON_SOURCE_DIR}/stdPlatformTimeBabylon.cpp"
    "${STDBABYLON_SOURCE_DIR}/stdBabylonPath.cpp"
    "${STDBABYLON_SOURCE_DIR}/stdFileBabylon.cpp"
    "${STDBABYLON_SOURCE_DIR}/stdFileUtilBabylon.cpp"
)

target_compile_definitions(stdBabylonTests PRIVATE
    J3D_TEST
)

# The test and the adapter files include <std/...> (via Libs) and <babylon/...> (via the links).
target_include_directories(stdBabylonTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../.."
)

target_link_libraries(stdBabylonTests PRIVATE
    j3dcore
    j3dTestSupport
    babylon::time
    babylon::files
)

set_target_properties(stdBabylonTests PROPERTIES
    FOLDER "Tests/Libs/std"
    LINKER_LANGUAGE CXX
)

source_group("Tested Sources" FILES
    "${STDBABYLON_SOURCE_DIR}/stdBabylon.cpp"
    "${STDBABYLON_SOURCE_DIR}/stdPlatformTimeBabylon.cpp"
    "${STDBABYLON_SOURCE_DIR}/stdBabylonPath.cpp"
    "${STDBABYLON_SOURCE_DIR}/stdFileBabylon.cpp"
    "${STDBABYLON_SOURCE_DIR}/stdFileUtilBabylon.cpp"
)

add_test(NAME std.Babylon COMMAND stdBabylonTests)
set_tests_properties(std.Babylon PROPERTIES
    LABELS "unit;std"
)
