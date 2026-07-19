add_executable(j3dcoreTests
    j3dMacroTest.c
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
