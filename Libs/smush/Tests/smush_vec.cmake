# Test list of the smush C-module original-engine vectors (Mixer, D_bloc16, Decode, LecShare, Framestream, Stdfile,
# Zfilio): SmushVecCases.h generates the cases, tv/ holds the original's records (see tv/README.md, "C Modules").
list(APPEND SMUSH_TEST_SOURCES
    SmushVecTest.c
)

set_source_files_properties(SmushVecTest.c PROPERTIES
    COMPILE_DEFINITIONS SMUSH_TEST_TV_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tv"
)
