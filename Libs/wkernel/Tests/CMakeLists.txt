# Added: the Babylon column's wkernel suite (wkernelBabylon.cpp: the event loop entry points,
# the activation callback hand-over and the window resize). Only that column builds it, guarded
# in the parent directory; the Null column has no wkernel behavior to test here.
set(WKERNEL_BABYLON_TESTED_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../std/Babylon/stdBabylonEvents.cpp")

add_executable(wkernelTests
    wkernelTestRunner.c
    wkernelBabylonTest.cpp
    # The pump wkernelBabylon.cpp calls, compiled with J3D_TEST below so its ResetForTest is
    # available. The std archive's copy of the same file stays out of the link: its symbols are
    # already defined here, and an archive member is only pulled in for what is still undefined.
    "${WKERNEL_BABYLON_TESTED_SOURCE}"
)

target_compile_definitions(wkernelTests PRIVATE
    J3D_TEST
)

# wkernelBabylonTest.cpp includes <std/...>, <wkernel/...> (via Libs) and <babylon/...> (via the links).
target_include_directories(wkernelTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)

target_link_libraries(wkernelTests PRIVATE
    wkernel
    j3dTestSupport
)

# The context entry (StdBabylonContext with a NullPlatform), the entry rename onto it, the
# Babylon links and the switch of the link to C++.
std_use_babylon_test_main(wkernelTests "${CMAKE_CURRENT_SOURCE_DIR}/wkernelTestRunner.c")

set_target_properties(wkernelTests PROPERTIES
    FOLDER "Tests/Libs/wkernel"
)

source_group("Tested Sources" FILES "${WKERNEL_BABYLON_TESTED_SOURCE}")

add_test(NAME wkernel COMMAND wkernelTests)
set_tests_properties(wkernel PROPERTIES
    LABELS "unit;wkernel"
)
