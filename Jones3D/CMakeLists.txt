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
list(REMOVE_ITEM JONES3D_DLL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/sha256.h")

# Jones3D EXE sources
set(JONES3D_EXE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/exemain.cpp")

# Create the executable and DLL targets
add_executable(Jones3D ${JONES3D_EXE_SOURCES} )
add_library(Jones3D_DLL SHARED ${JONES3D_DLL_SOURCES} ${JONES3D_DLL_HEADERS})

# Make Jones3D.exe depending on Jones3D.dll
add_dependencies(Jones3D Jones3D_DLL)

# Add Unicode character set for Jones3D.exe
target_compile_definitions(Jones3D PRIVATE _UNICODE )

# Set DPI awareness
set_property(TARGET Jones3D PROPERTY VS_DPI_AWARE "PerMonitor")

# Rename the DLL target output
set_target_properties(Jones3D_DLL PROPERTIES OUTPUT_NAME "Jones3D")

# Add include to Jones3D.dll
target_include_directories(Jones3D_DLL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Link against Libs project
target_link_libraries(Jones3D_DLL PRIVATE Libs)
target_link_libraries(Jones3D_DLL PRIVATE Comctl32.lib)

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${JONES3D_DLL_SOURCES} ${JONES3D_DLL_HEADERS})

# If the post-build command is enabled, and the destination path is provided,
# add the post-build command to copy the output files to the specified directory.
if(JONES3D_ENABLE_POST_BUILD_COPY)
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

    # Add post-build copy command for the built SDL3.dll
    if(J3D_OPENGL)
        add_custom_command(
                TARGET Jones3D_DLL
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3::SDL3>"
                "${JONES3D_POST_BUILD_COPY_DIR}"
                COMMENT "Copying SDL3.dll to '${JONES3D_POST_BUILD_COPY_DIR}'"
        )
    endif()

endif()

if (J3D_OPENGL)
    #target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
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