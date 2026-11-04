# Added: std_use_babylon_test_main(<target> <runner source>) gives one game test executable of
# the Babylon column the adapter context: it adds stdBabylonTestMain.cpp (whose main installs the
# context and calls the runner's renamed entry), renames that runner source's entry to
# j3dTestRunner_Main, links the Babylon interfaces and switches the link to C++. It is a no-op
# for the Null column and every Windows build, which keep their plain main.
#
# The runner entry is renamed per source file, not per target, so <runner source> must be used
# only by that executable (stdTestRunner.c is shared by stdTests and stdTestsBabylon; the latter
# therefore renames the entry with a target definition in Libs/std/Tests/CMakeLists.txt).
#
# Included from Libs/CMakeLists.txt before the module loop: a function lives in the directory
# scope of the file that defines it, and that loop adds std after rdroid, sith, smush and sound.

# The directory of this file, captured once so the function can find the test main in the Tests
# directory next to it.
set(STDBABYLON_TEST_MAIN_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(std_use_babylon_test_main target runner_source)
    if (NOT J3D_PLATFORM_BABYLON)
        return()
    endif()

    target_sources(${target} PRIVATE "${STDBABYLON_TEST_MAIN_DIR}/Tests/stdBabylonTestMain.cpp")
    set_property(SOURCE "${runner_source}" APPEND PROPERTY COMPILE_DEFINITIONS
        "J3D_TEST_RUNNER_ENTRY=j3dTestRunner_Main")
    target_link_libraries(${target} PRIVATE babylon::time babylon::files)
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()
