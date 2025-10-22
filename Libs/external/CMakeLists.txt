cmake_minimum_required(VERSION 3.15)

if (J3D_OPENGL)
    set(EXTERNAL_MODULES
            SDL3
    )
    #find_package(OpenGL REQUIRED)
    #target_link_libraries(SDL3Vendor INTERFACE OpenGL::GL)
elseif (J3D_DIRECTX6)
    set(EXTERNAL_MODULES
        DirectX61c
    )
endif()

set(EXTERNAL_MODULES
    ${EXTERNAL_MODULES}
    jansson
)

# Add external modules

# Set Jansson config
set(JANSSON_BUILD_SHARED_LIBS OFF)
set(JANSSON_INSTAL OFF)
set(JANSSON_BUILD_DOCS OFF) 
set(JANSSON_WITHOUT_TESTS ON)
set(JANSSON_EXAMPLES OFF)

if (MSVC)
    set(JANSSON_STATIC_CRT ON )
endif ()

foreach(MODULE ${EXTERNAL_MODULES})
    add_subdirectory(${MODULE})
    set_target_properties(${MODULE} PROPERTIES FOLDER "Libs/external") # Group all lib modules under "Libs/external" in Visual Studio
endforeach()


# Add Jansson subdirectory
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

