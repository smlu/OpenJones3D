# Each smush area has its own test list file (smush_<area>.cmake) that appends to SMUSH_TEST_SOURCES (test files) and
# SMUSH_TESTED_SOURCES (library sources compiled into the test executable, e.g. to use J3D_TEST hooks), so that the area
# lanes never edit this file or the runner.
set(SMUSH_TEST_SOURCES
    smushTestRunner.c
)

set(SMUSH_TESTED_SOURCES "")

include(smush_sound.cmake)   # Sound: Engine, Mixer, Dsnd, MixerAsm
include(smush_codec.cmake)   # Codec: D_bloc16, D_wave, Bloc16Asm, Decode, DecodeAsm
include(smush_stream.cmake)  # Stream/File/Anim: Cdstream, Framestream, Stdfile, Stdio, Zfilio, Readauto, Frame, Pcplay
include(smush_core.cmake)    # Core/Bitmap: SmushPlay, LecShare, Timer, Memory, List, Bitmap, Blit, BlitAsm, MemCopy, MemFill
include(smush_asm.cmake)     # Assembly translations: Bloc16Asm, DecodeAsm, MixerAsm, BlitAsm, MemCopy, MemFill (original-engine vectors)
include(smush_vec.cmake)     # C modules: Mixer, D_bloc16, Decode, LecShare, Framestream, Stdfile, Zfilio (original-engine vectors)

add_executable(smushTests
    ${SMUSH_TEST_SOURCES}
    ${SMUSH_TESTED_SOURCES}
)

target_link_libraries(smushTests PRIVATE
    smush
    unity
)

target_compile_definitions(smushTests PRIVATE
    J3D_TEST
)

set_target_properties(smushTests PROPERTIES
    FOLDER "Tests/Libs/smush"
)

source_group("Tests" FILES ${SMUSH_TEST_SOURCES})
source_group("Tested Sources" FILES ${SMUSH_TESTED_SOURCES})

set(SMUSH_TEST_BACKEND_LABEL "")
if (J3D_DIRECTX6)
    set(SMUSH_TEST_BACKEND_LABEL "DirectX6")
elseif (J3D_DIRECTX9)
    set(SMUSH_TEST_BACKEND_LABEL "DirectX9")
endif()

add_test(NAME smush COMMAND smushTests)
# The in-memory suites normally finish in under a second; thread tests (timer, sound) may need more on slow hosts.
set_tests_properties(smush PROPERTIES
    LABELS "unit;smush;${SMUSH_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)
