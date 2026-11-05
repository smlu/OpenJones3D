# Test list of the smush Core area (SmushPlay, LecShare, Timer, Memory, List, Bitmap, Blit, BlitAsm, MemCopy, MemFill).
# The tested sources are linked from the smush library.
list(APPEND SMUSH_TEST_SOURCES
    SmushCoreTest.c
    SmushPlayTest.c
)

set_source_files_properties(SmushPlayTest.c PROPERTIES
    COMPILE_DEFINITIONS SMUSH_PLAY_TV_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tv/SmushPlay"
)
