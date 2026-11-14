cmake_minimum_required(VERSION 3.15)
project(xBRZ CXX)

# xBRZ image upscaler (https://sourceforge.net/projects/xbrz/) with a small C API for the C code
add_library(xBRZ STATIC
    xbrz.cpp
    xbrz.h
    xbrz_config.h
    xbrz_tools.h
    xbrz_capi.cpp
    xbrz_capi.h
)

target_include_directories(xBRZ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(xBRZ PRIVATE cxx_std_17)

if (MSVC)
    # Vendored code, keep its warnings out of the build output
    target_compile_options(xBRZ PRIVATE /W0)
endif()
