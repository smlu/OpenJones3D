# Test list of the smush Stream area (Cdstream, Framestream, Stdfile, Stdio, Zfilio, Readauto, Frame, Pcplay).
list(APPEND SMUSH_TEST_SOURCES
    SmushStreamTest.c
    ZfilioZlibTest.c
)

# Compile Zfilio separately to enable its test callbacks.
set(SMUSH_ZFILIO_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../insane/File/Zfilio/Zfilio.c")
list(APPEND SMUSH_TESTED_SOURCES "${SMUSH_ZFILIO_SOURCE}")
