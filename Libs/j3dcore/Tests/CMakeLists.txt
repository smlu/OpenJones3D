# Added: The Null platform runs its tests natively; the top-level project disables testing there,
# so this directory enables it for j3dcore.Crt.
if (J3D_PLATFORM_NULL)
    enable_testing()
endif()

if (JONES3D_BUILD_TESTS)
    add_executable(j3dcoreTests
        j3dMacroTest.c
        # Added: Tests the Microsoft secure-CRT behavior against the real Windows CRT.
        j3dCrtTest.c
    )

    target_link_libraries(j3dcoreTests PRIVATE
        j3dcore
        unity
    )

    set_target_properties(j3dcoreTests PROPERTIES
        FOLDER "Tests/Libs/j3dcore"
    )

    add_test(NAME j3dcore.Macros COMMAND j3dcoreTests)
    set_tests_properties(j3dcore.Macros PROPERTIES
        LABELS "unit;j3dcore"
    )
endif()

# Added: Null platform: native executable for the j3dcrt Microsoft CRT shim (j3dCrtTest.c
# provides its own main only under J3D_PLATFORM_NULL).
if (J3D_PLATFORM_NULL)
    add_executable(j3dCrtTests
        j3dCrtTest.c
    )

    target_link_libraries(j3dCrtTests PRIVATE
        j3dcore
        unity
    )

    set_target_properties(j3dCrtTests PROPERTIES
        FOLDER "Tests/Libs/j3dcore"
    )

    add_test(NAME j3dcore.Crt COMMAND j3dCrtTests)
    set_tests_properties(j3dcore.Crt PROPERTIES
        LABELS "unit;j3dcore"
    )
endif()
