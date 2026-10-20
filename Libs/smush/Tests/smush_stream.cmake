# Test list of the smush Stream area (Cdstream, Framestream, Stdfile, Stdio, Zfilio, Readauto, Frame, Pcplay).
list(APPEND SMUSH_TEST_SOURCES
    SmushStreamTest.c
)

# Zfilio.c defines zfilio_pFilio (a far variable of the executable otherwise) and exposes the gz function test seam.
set(SMUSH_ZFILIO_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../insane/File/Zfilio/Zfilio.c")
list(APPEND SMUSH_TESTED_SOURCES "${SMUSH_ZFILIO_SOURCE}")
# The test file reads zfilio_pFilio too
set_source_files_properties("${SMUSH_ZFILIO_SOURCE}" "${CMAKE_CURRENT_SOURCE_DIR}/SmushStreamTest.c" PROPERTIES
    COMPILE_DEFINITIONS "J3D_LOCAL_RTI"
)
