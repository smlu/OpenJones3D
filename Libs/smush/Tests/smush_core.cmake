# Test list of the smush Core area (SmushPlay, LecShare, Timer, Memory, List, Bitmap, Blit, BlitAsm, MemCopy, MemFill). Add this area's test files and tested library sources here (its test file is already listed):
#   list(APPEND SMUSH_TEST_SOURCES SmushCoreTest.c)
#   list(APPEND SMUSH_TESTED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../<path>.c")
list(APPEND SMUSH_TEST_SOURCES
    SmushCoreTest.c
)
