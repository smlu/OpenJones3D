# Test list of the smush Stream area (Cdstream, Framestream, Stdfile, Stdio, Zfilio, Readauto, Frame, Pcplay). Add this area's test files and tested library sources here (its test file is already listed):
#   list(APPEND SMUSH_TEST_SOURCES SmushStreamTest.c)
#   list(APPEND SMUSH_TESTED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../<path>.c")
list(APPEND SMUSH_TEST_SOURCES
    SmushStreamTest.c
)
