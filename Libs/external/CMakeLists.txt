cmake_minimum_required(VERSION 3.15)

if (J3D_OPENGL)
    # SDL3
    set(SDL_STATIC ON)
    set(SDL_SHARED OFF)
    set(SDL_TESTS  OFF)
    set(SDL_TEST_LIBRARY  OFF)
    set(SDL_LIBC  ON)

    add_subdirectory(SDL3)
    set_target_properties(SDL3-static PROPERTIES FOLDER "Libs/external/SDL3") # Group all lib modules under "Libs/external" in Visual Studio
    set_target_properties(SDL_uclibc PROPERTIES FOLDER "Libs/external/SDL3") # Group all lib modules under "Libs/external" in Visual Studio

    # GLAD
    add_library(glad STATIC
            glad/src/gl.c
    )
    target_include_directories(glad PUBLIC glad/include)
    set_target_properties(glad PROPERTIES FOLDER "Libs/external") 
elseif (J3D_DIRECTX6)
    set(EXTERNAL_MODULES
        DirectX61c
    )
endif()

# Add external modules
set(EXTERNAL_MODULES
    ${EXTERNAL_MODULES}
    jansson
)

# Set Jansson config
set(JANSSON_BUILD_SHARED_LIBS OFF)
set(JANSSON_INSTAL OFF)
set(JANSSON_BUILD_DOCS OFF) 
set(JANSSON_WITHOUT_TESTS ON)
set(JANSSON_EXAMPLES OFF)

if (MSVC)
    set(JANSSON_STATIC_CRT ON )
endif ()

# Include dependency subdirs in CMake project
foreach(MODULE ${EXTERNAL_MODULES})
    add_subdirectory(${MODULE})
    set_target_properties(${MODULE} PROPERTIES FOLDER "Libs/external") # Group all lib modules under "Libs/external" in Visual Studio
endforeach()


# Add Jansson library
add_library(jansson_interface INTERFACE)
target_link_libraries(jansson_interface INTERFACE ${JANSSON_LIBRARIES})
target_include_directories(jansson_interface INTERFACE
   "${JANSSON_INCLUDE_DIRS}"
)
add_library(Jansson ALIAS jansson_interface)

# Disable the problematic warning for MSVC on Jansson target
if(MSVC)
    target_compile_options(${JANSSON_LIBRARIES} PRIVATE
        /wd4146  # Disable C4146: unary minus operator applied to unsigned type
        /wd4703  # Disable C4703: potentially uninitialized local pointer variable
    )
endif()
