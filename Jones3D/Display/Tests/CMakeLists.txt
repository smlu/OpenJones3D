add_executable(jonesConfigTests jonesConfigTest.c)
target_include_directories(jonesConfigTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
target_compile_definitions(jonesConfigTests PRIVATE J3D_TEST)
target_link_libraries(jonesConfigTests PRIVATE Libs unity Comctl32.lib xBRZ)
if (MSVC)
    # Keep exercised functions without retaining unrelated GUI paths.
    target_compile_options(jonesConfigTests PRIVATE /Gy)
    target_link_options(jonesConfigTests PRIVATE /OPT:REF /INCREMENTAL:NO)
endif()
set_target_properties(jonesConfigTests PROPERTIES FOLDER "Tests/Jones3D")

set(JONES_DISPLAY_TEST_BACKEND_LABEL "DirectX6")
if (J3D_DIRECTX9)
    set(JONES_DISPLAY_TEST_BACKEND_LABEL "DirectX9")
endif()
add_test(NAME Jones3D.Config COMMAND jonesConfigTests)
set_tests_properties(Jones3D.Config PROPERTIES
    LABELS "unit;Jones3D;${JONES_DISPLAY_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)

add_executable(jonesHudTests JonesHudTest.c)
target_include_directories(jonesHudTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
target_compile_definitions(jonesHudTests PRIVATE J3D_TEST)
target_link_libraries(jonesHudTests PRIVATE Libs unity Comctl32.lib)
if (MSVC)
    target_compile_options(jonesHudTests PRIVATE /Gy)
    target_link_options(jonesHudTests PRIVATE /OPT:REF /INCREMENTAL:NO)
endif()
set_target_properties(jonesHudTests PROPERTIES FOLDER "Tests/Jones3D")

add_test(NAME Jones3D.Hud COMMAND jonesHudTests)
set_tests_properties(Jones3D.Hud PROPERTIES
    LABELS "unit;Jones3D;${JONES_DISPLAY_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)

if (JONES3D_BUILD_SYSTEM_TESTS)
    add_executable(jonesConfigSystemTests jonesConfigSystemTest.c)
    target_include_directories(jonesConfigSystemTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
    target_compile_definitions(jonesConfigSystemTests PRIVATE J3D_TEST)
    target_link_libraries(jonesConfigSystemTests PRIVATE Libs unity Comctl32.lib xBRZ)
    if (MSVC)
        target_compile_options(jonesConfigSystemTests PRIVATE /Gy)
        target_link_options(jonesConfigSystemTests PRIVATE /OPT:REF /INCREMENTAL:NO)
    endif()
    set_target_properties(jonesConfigSystemTests PROPERTIES FOLDER "Tests/Jones3D")
    add_test(NAME Jones3D.ConfigSystem COMMAND jonesConfigSystemTests)
    set_tests_properties(Jones3D.ConfigSystem PROPERTIES
        LABELS "system;Jones3D;${JONES_DISPLAY_TEST_BACKEND_LABEL}"
        TIMEOUT 30
        RUN_SERIAL TRUE
    )
endif()
