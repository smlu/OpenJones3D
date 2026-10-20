# Test list of the smush Codec area (D_bloc16, D_wave, Bloc16Asm, Decode, DecodeAsm).
#
list(APPEND SMUSH_TEST_SOURCES
    SmushCodecTest.c
)

list(APPEND SMUSH_TESTED_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../insane/Flobject/D_bloc16.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../insane/Flobject/D_wave.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../insane/Decode/Decode.c"
)
