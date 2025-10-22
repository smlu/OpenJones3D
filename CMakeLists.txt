cmake_minimum_required(VERSION 3.5)

project(OpenJones3D VERSION 0.4.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20) # C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable solution folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # option() honors normal variables (variables of same name set prior to calling the option()).

option(JONES3D_BUILD_PROGRAMS "Build programs" ON)
option(JONES3D_QOL_IMPROVEMENTS "Enable quality-of-life improvements" ON)
option(JONES3D_SPEEDRUN_BUILD "Enable vanilla bugs and glitches used in speedruns" OFF)

# --- GAPI options ---
# Add an option to choose graphics API. Default is DirectX9.
set(JONES3D_RENDERER "OpenGL" CACHE STRING "Choose graphics backend")
set_property(CACHE JONES3D_RENDERER PROPERTY STRINGS "OpenGL;DirectX9;DirectX6")
#option(JONES3D_USE_DIRECTX9 "Build with DirectX 9 support" ON)

#---------------------

if(JONES3D_QOL_IMPROVEMENTS)
    message(STATUS "QOL improvements enabled")
    set(J3D_QOL_IMPROVEMENTS ON)
else()
    # Legacy build, enable the speedrun build option to restore all vanilla engine quirks and bugs
    set(JONES3D_SPEEDRUN_BUILD ON)
endif()

if (JONES3D_SPEEDRUN_BUILD)  
    message(STATUS "Speedrun build enabled")
    set(J3D_SPEEDRUN_BUILD on)
    if (JONES3D_QOL_IMPROVEMENTS)
      set(JONES3D_BUILD_SUFFIX "-sr") # Add '-sr' version suffix when not legacy build
    endif()
endif()

if (JONES3D_RENDERER STREQUAL "OpenGL")
    message(STATUS "Configuring for OpenGL")
    set(J3D_OPENGL ON) # Set the variable for the header template
elseif(JONES3D_RENDERER STREQUAL "DirectX9")
    message(STATUS "Configuring for DirectX 9")
    set(J3D_DIRECTX9 ON) # Set the variable for the header template
elseif(JONES3D_RENDERER STREQUAL "DirectX6")
    message(STATUS "Configuring for vanilla DirectX 6 (default)")
    set(J3D_DIRECTX6 ON) # Set the variable for the header template
    set(J3D_COLORKEYSUPPORTED ON)
else()
    message(FATAL_ERROR "Unsupported graphics API : ${JONES3D_RENDERER}")
endif()

# Configure the version header file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/LIBS/j3dcore/j3d.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/LIBS/j3dcore/j3d.h"
)

# Set global property for MSVC runtime library (CMake 3.15+)
set(CMAKE_MSVC_RUNTIME_LIBRARY "$<$<CONFIG:Release>:MultiThreaded>$<$<CONFIG:MinSizeRel>:MultiThreaded>")

if (MSVC)
    #add_compile_definitions("__FILE_NAME__=\"%(Filename)%(Extension)\"")
    add_compile_options(
        /utf-8
        /Zc:__STDC__
        /Zc:__cplusplus
        /diagnostics:caret
        /sdl
        #"$<$<CONFIG:DEBUG>:/Gm>" # minimal rebuild TODO: uncomment
        /W4 # warning level 4
        /Zc:preprocessor # This option enables a token-based preprocessor that conforms to C99 and C++11 and later standards.
        "$<$<NOT:$<CXX_COMPILER_ID:Clang>>:/MP>" # TODO: remove when /Gm is uncommented
        #$<$<AND:$<NOT:$<CXX_COMPILER_ID:Clang>>,$<NOT:$<CONFIG:DEBUG>>>:/MP> TODO: uncomment when /Gm is uncommented
        "$<$<CONFIG:DEBUG>:/ZI>"
    )
    add_link_options("/Brepro")
else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Add Libs subdirectory
add_subdirectory(Libs)

# Add main project Jones3D
add_subdirectory(Jones3D)

if (JONES3D_BUILD_PROGRAMS)
    add_subdirectory(Programs)
endif()
