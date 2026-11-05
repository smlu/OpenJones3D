add_executable(jonesConfigTests jonesConfigTest.c)
target_include_directories(jonesConfigTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
target_compile_definitions(jonesConfigTests PRIVATE J3D_TEST)
target_link_libraries(jonesConfigTests PRIVATE Libs unity Comctl32.lib)
if (MSVC)
    # Keep the actual parser/key-name functions without retaining the whole GUI.
    target_compile_options(jonesConfigTests PRIVATE /Gy)
    target_link_options(jonesConfigTests PRIVATE /OPT:REF /INCREMENTAL:NO)
endif()
set_target_properties(jonesConfigTests PROPERTIES FOLDER "Tests/Jones3D")

set(JONES_CONFIG_TEST_BACKEND_LABEL "DirectX6")
if (J3D_DIRECTX9)
    set(JONES_CONFIG_TEST_BACKEND_LABEL "DirectX9")
endif()
add_test(NAME Jones3D.Config COMMAND jonesConfigTests)
set_tests_properties(Jones3D.Config PROPERTIES
    LABELS "unit;Jones3D;${JONES_CONFIG_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)
