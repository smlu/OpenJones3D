cmake_minimum_required(VERSION 3.15)
project(Jones3D)

# Option to enable or disable the post-build command.
# When set to ON, a post-build step will be added that copies the
# output files (.dll and .exe) to a specified directory.
option(JONES3D_ENABLE_POST_BUILD_COPY "Enable copying of output files (Jones3D.dll, Jones3D.exe) after building" OFF)

# Path to the directory where the output files should be copied.
# This should be set to an absolute path or a path relative to the project root.
# The copied files will be placed in this directory under a 'Debug' subfolder.
set(JONES3D_POST_BUILD_COPY_DIR "" CACHE PATH "Destination folder for the post-build copied files (Jones3D.dll, Jones3D.exe)")

# Collect Jones3D DLL sources
file(GLOB_RECURSE JONES3D_DLL_SOURCES *.c)
file(GLOB_RECURSE JONES3D_DLL_HEADERS *.h)
list(FILTER JONES3D_DLL_SOURCES EXCLUDE REGEX ".*/Tests/.*")
list(FILTER JONES3D_DLL_HEADERS EXCLUDE REGEX ".*/Tests/.*")
list(REMOVE_ITEM JONES3D_DLL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/sha256.h")

# Jones3D EXE sources
set(JONES3D_EXE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/exemain.cpp")

# Create the executable and DLL targets
if (JONES3D_STANDALONE)
    # Added: One self-contained Jones3D.exe instead of a DLL plus an injector (exemain.cpp).
    # Added: Compile the dialog resources into the standalone exe so its DialogBoxParam calls find their templates.
    enable_language(RC)
    add_executable(Jones3D WIN32 ${JONES3D_DLL_SOURCES} ${JONES3D_DLL_HEADERS} Indy3D.rc)

    # Set DPI awareness
    set_property(TARGET Jones3D PROPERTY VS_DPI_AWARE "PerMonitor")

    # Add include to Jones3D
    target_include_directories(Jones3D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

    # Link against Libs project
    target_link_libraries(Jones3D PRIVATE Libs)
    target_link_libraries(Jones3D PRIVATE comctl32)

    # Organize source files in Visual Studio
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${JONES3D_DLL_SOURCES} ${JONES3D_DLL_HEADERS})

    # Added: Same 1 MB stack reserve as the retail exe. Its image base moves so no address in the retail range is ever mapped by this program.
    if (MINGW)
        target_link_options(Jones3D PRIVATE -Wl,--stack,1048576 -Wl,--image-base=0x10000000 -Wl,--disable-dynamicbase)
    endif()
else()
    add_executable(Jones3D ${JONES3D_EXE_SOURCES} )
    add_library(Jones3D_DLL SHARED ${JONES3D_DLL_SOURCES} ${JONES3D_DLL_HEADERS})

    # Make Jones3D.exe depending on Jones3D.dll
    add_dependencies(Jones3D Jones3D_DLL)

    # Add Unicode character set for Jones3D.exe
    # Added: UNICODE besides _UNICODE - exemain.cpp calls the explicit wide API functions
    # (CreateProcessW etc.) and both character-set macros must be defined for the Win32 headers
    # to select the wide struct types (STARTUPINFOW) on every compiler.
    target_compile_definitions(Jones3D PRIVATE _UNICODE UNICODE)

    # Set DPI awareness
    set_property(TARGET Jones3D PROPERTY VS_DPI_AWARE "PerMonitor")

    # Rename the DLL target output
    set_target_properties(Jones3D_DLL PROPERTIES OUTPUT_NAME "Jones3D")

    # Altered: MinGW GCC prefixes shared library outputs with "lib" by default; the DLL must stay "Jones3D.dll".
    if (MINGW)
        set_target_properties(Jones3D_DLL PROPERTIES PREFIX "")
    endif()

    # Added: MinGW equivalent of the MSVC CRT "wmain" entry point used by exemain.cpp.
    if (MINGW)
        target_link_options(Jones3D PRIVATE -municode)
    endif()

    # Keep executable and DLL linker databases separate for incremental MSVC builds.
    set_target_properties(Jones3D PROPERTIES PDB_NAME "Jones3D_EXE")

    # Add include to Jones3D.dll
    target_include_directories(Jones3D_DLL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

    # Link against Libs project
    target_link_libraries(Jones3D_DLL PRIVATE Libs)
    target_link_libraries(Jones3D_DLL PRIVATE comctl32)

    # Organize source files in Visual Studio
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${JONES3D_DLL_SOURCES} ${JONES3D_DLL_HEADERS})
endif()

# If the post-build command is enabled, and the destination path is provided,
# add the post-build command to copy the output files to the specified directory.
if(JONES3D_ENABLE_POST_BUILD_COPY)
    # Added: Skipped in standalone builds - the block refers to DLL paths and no Jones3D.dll is produced there.
    if(NOT JONES3D_STANDALONE)
    if(NOT JONES3D_POST_BUILD_COPY_DIR)
        message(FATAL_ERROR "Post-build command is enabled, but no path was specified. Please set JONES3D_POST_BUILD_COPY_DIR.")
    endif()

    # Add post-build copy command for Jones3D.exe
    add_custom_command(
        TARGET Jones3D
        POST_BUILD
        COMMAND xcopy /y /d "$(OutDir)$(TargetName)$(TargetExt)" "\"${JONES3D_POST_BUILD_COPY_DIR}\""
        COMMENT "Copying output file Jones3D.exe to '${JONES3D_POST_BUILD_COPY_DIR}'"
    )

    # Set debugging work dir to copy dir
    set_target_properties(Jones3D PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${JONES3D_POST_BUILD_COPY_DIR}"
    )

    # Add post-build copy command for Jones3D.dll
    add_custom_command(
        TARGET Jones3D_DLL
        POST_BUILD
        COMMAND xcopy /y /d "$(OutDir)$(TargetName)$(TargetExt)" "\"${JONES3D_POST_BUILD_COPY_DIR}\""
        COMMAND xcopy /y /d "$(OutDir)$(TargetName).pdb" "\"${JONES3D_POST_BUILD_COPY_DIR}\""
        COMMENT "Copying output file Jones3D.dll and Jones3D.pdb to '${JONES3D_POST_BUILD_COPY_DIR}'"
    )
    endif()

endif()

# # Set up post-build event for Debug configuration
# set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Output")
# file(TO_NATIVE_PATH "${OUTPUT_DIR}" OUTPUT_DIR_NATIVE)

# set_target_properties(Jones3D PROPERTIES
#     VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
#     VS_DEBUGGER_COMMAND "${CMAKE_BINARY_DIR}/$(Configuration)/Jones3D.exe"
# )

#  #xcopy /Y /I "$(TargetPath)" "${OUTPUT_DIR_NATIVE}"
# add_custom_command(TARGET Jones3D POST_BUILD
#     COMMAND if $(Configuration) == Debug (
#         xcopy /y /d  "$(OutDir)$(TargetName)$(TargetExt)" "${OUTPUT_DIR_NATIVE}"
#     )
#     COMMENT "Copying executable to ${OUTPUT_DIR_NATIVE}"
# )

# add_custom_command(TARGET Jones3D_DLL POST_BUILD
#     COMMAND if $(Configuration) == Debug (
#         xcopy /y /d  "$(OutDir)$(TargetName)$(TargetExt)" "${OUTPUT_DIR_NATIVE}"
#         xcopy /y /d  "$(OutDir)$(TargetName).pdb" "${OUTPUT_DIR_NATIVE}"
#     )
#     COMMENT "Copying DLL to ${OUTPUT_DIR_NATIVE}"
# )

if (JONES3D_BUILD_TESTS)
    add_subdirectory(Main/Tests)
endif()