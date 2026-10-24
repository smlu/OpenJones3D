set(SOUND_TEST_SOURCES
    soundTestRunner.c
    AudioLibTest.c
    SoundTest.c
)

set(SOUND_TESTED_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../Sound.c"
)

add_executable(soundTests
    ${SOUND_TEST_SOURCES}
    ${SOUND_TESTED_SOURCES}
)

target_link_libraries(soundTests PRIVATE
    sound
    unity
)

target_compile_definitions(soundTests PRIVATE
    J3D_TEST
)

set_target_properties(soundTests PROPERTIES
    FOLDER "Tests/Libs/sound"
)

source_group("Tests" FILES ${SOUND_TEST_SOURCES})
source_group("Tested Sources" FILES ${SOUND_TESTED_SOURCES})

set(SOUND_TEST_BACKEND_LABEL "")
if (J3D_DIRECTX6)
    set(SOUND_TEST_BACKEND_LABEL "DirectX6")
elseif (J3D_DIRECTX9)
    set(SOUND_TEST_BACKEND_LABEL "DirectX9")
endif()

add_test(NAME sound COMMAND soundTests)
# The in-memory suite normally finishes in under a second; allow slower hosts.
set_tests_properties(sound PROPERTIES
    LABELS "unit;sound;${SOUND_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)
