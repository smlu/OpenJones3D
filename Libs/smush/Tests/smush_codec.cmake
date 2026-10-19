# Test list of the smush Codec area (D_bloc16, D_wave, Bloc16Asm, Decode, DecodeAsm). Add this area's test files and tested library sources here (its test file is already listed):
#   list(APPEND SMUSH_TEST_SOURCES SmushCodecTest.c)
#   list(APPEND SMUSH_TESTED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../<path>.c")
list(APPEND SMUSH_TEST_SOURCES
    SmushCodecTest.c
)
