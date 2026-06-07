cmake_minimum_required(VERSION 3.10)

project(OpenJones3D VERSION 0.4.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20) # C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable solution folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # option() honors normal variables (variables of same name set prior to calling the option()).

function(jones3d_print_config_entry label value)
    set(jones3d_dot_fill "........................................")
    string(LENGTH "${label}" label_len)
    math(EXPR pad_len "28 - ${label_len}")
    if (pad_len LESS 1)
        set(pad_len 1)
    endif()
    string(SUBSTRING "${jones3d_dot_fill}" 0 ${pad_len} dot_pad)
    message(STATUS "  ${label} ${dot_pad} ${value}")
endfunction()

function(jones3d_print_config_option label value)
    if(${value})
        set(option_state "ON")
    else()
        set(option_state "OFF")
    endif()
    jones3d_print_config_entry("${label}" "${option_state}")
endfunction()

option(JONES3D_BUILD_PROGRAMS "Build programs" ON)
option(JONES3D_QOL_IMPROVEMENTS "Enable quality-of-life improvements" ON)
option(JONES3D_SPEEDRUN_BUILD "Enable vanilla bugs and glitches used in speedruns" OFF)
option(JONES3D_RUNTIME_GUARDS "Enable runtime safety guards for hardened release behavior" ON)

# --- GAPI options ---
# Add an option to enable DirectX 9 support. Default is ON.
option(JONES3D_USE_DIRECTX9 "Build with DirectX 9 support" ON)
#---------------------

if (JONES3D_QOL_IMPROVEMENTS)
    set(J3D_QOL_IMPROVEMENTS ON)
else()
    # Legacy build, enable the speedrun build option to restore all vanilla engine quirks and bugs
    set(JONES3D_SPEEDRUN_BUILD ON)
endif()

if (JONES3D_SPEEDRUN_BUILD)
    set(J3D_SPEEDRUN_BUILD on)
    if (JONES3D_QOL_IMPROVEMENTS)
      set(JONES3D_BUILD_SUFFIX "-sr") # Add '-sr' version suffix when not legacy build
    endif()
endif()

if (JONES3D_RUNTIME_GUARDS)
    set(J3D_RUNTIME_GUARDS ON)
endif()

if(JONES3D_USE_DIRECTX9)
    set(JONES3D_GAPI_NAME "DirectX 9")
    set(J3D_DIRECTX9 ON) # Set the variable for the header template
    if (NOT JONES3D_QOL_IMPROVEMENTS)
        if (JONES3D_RUNTIME_GUARDS)
            set(JONES3D_BUILD_SUFFIX "-dx9-legacy-hardened") # Legacy behavior with release safety guards
        else()
            set(JONES3D_BUILD_SUFFIX "-dx9-legacy") # Pure legacy behavior
        endif()
    endif()
else()
    set(JONES3D_GAPI_NAME "DirectX 6.1c (original)")
    set(J3D_DIRECTX6 ON) # Set the variable for the header template
    set(J3D_COLORKEYSUPPORTED ON)
    if (JONES3D_QOL_IMPROVEMENTS)
      set(JONES3D_BUILD_SUFFIX "-dx6") # Add '-sr' version suffix when not legacy build
    else()
        if (JONES3D_RUNTIME_GUARDS)
            set(JONES3D_BUILD_SUFFIX "-legacy-hardened") # Legacy behavior with release safety guards
        else()
            set(JONES3D_BUILD_SUFFIX "-legacy") # Pure legacy behavior
        endif()
    endif()
endif()

# Configure the version header file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/LIBS/j3dcore/j3d.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/LIBS/j3dcore/j3d.h"
)

# Set global property for MSVC runtime library (CMake 3.15+)
set(CMAKE_MSVC_RUNTIME_LIBRARY "$<$<CONFIG:Release>:MultiThreaded>$<$<CONFIG:MinSizeRel>:MultiThreaded>")

if (MSVC)
    # Clion IDE can't read the preprocessor macro below and needs special treatment
    if($ENV{CLION_IDE})
        message(STATUS "Generating in Clion environment")
        add_compile_definitions(__FILE_NAME__=__FILE__)
    else()
        add_compile_definitions("__FILE_NAME__=\"%(Filename)%(Extension)\"")
    endif()

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

if (CMAKE_CONFIGURATION_TYPES)
    string(REPLACE ";" ", " JONES3D_CONFIGURATION_SUMMARY "${CMAKE_CONFIGURATION_TYPES}")
else()
    if (CMAKE_BUILD_TYPE)
        set(JONES3D_CONFIGURATION_SUMMARY "${CMAKE_BUILD_TYPE}")
    else()
        set(JONES3D_CONFIGURATION_SUMMARY "<default>")
    endif()
endif()

message(STATUS "")
message(STATUS "OpenJones3D configuration summary")
jones3d_print_config_entry("Version" "v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}${JONES3D_BUILD_SUFFIX}")
jones3d_print_config_entry("Generator" "${CMAKE_GENERATOR}")
if (CMAKE_GENERATOR_PLATFORM)
    jones3d_print_config_entry("Platform" "${CMAKE_GENERATOR_PLATFORM}")
endif()
if (CMAKE_CONFIGURATION_TYPES)
    jones3d_print_config_entry("Configurations" "${JONES3D_CONFIGURATION_SUMMARY}")
else()
    jones3d_print_config_entry("Build type" "${JONES3D_CONFIGURATION_SUMMARY}")
endif()
jones3d_print_config_entry("Graphics API" "${JONES3D_GAPI_NAME}")
jones3d_print_config_option("QOL improvements" JONES3D_QOL_IMPROVEMENTS)
jones3d_print_config_option("Runtime guards" JONES3D_RUNTIME_GUARDS)
jones3d_print_config_option("Speedrun build" JONES3D_SPEEDRUN_BUILD)
jones3d_print_config_option("Build programs" JONES3D_BUILD_PROGRAMS)
jones3d_print_config_option("Post-build copy" JONES3D_ENABLE_POST_BUILD_COPY)
if (JONES3D_ENABLE_POST_BUILD_COPY)
    if (JONES3D_POST_BUILD_COPY_DIR)
        jones3d_print_config_entry("Post-build copy dir" "${JONES3D_POST_BUILD_COPY_DIR}")
    else()
        jones3d_print_config_entry("Post-build copy dir" "<not set>")
    endif()
endif()
