# Test list of the smush assembly translations (Bloc16Asm, DecodeAsm, MixerAsm, BlitAsm, MemCopy, MemFill): original-engine
# vectors in tv/ (see tv/README.md).
list(APPEND SMUSH_TEST_SOURCES
    SmushAsmTest.c
)

set_source_files_properties(SmushAsmTest.c PROPERTIES
    COMPILE_DEFINITIONS SMUSH_TEST_TV_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tv"
)
