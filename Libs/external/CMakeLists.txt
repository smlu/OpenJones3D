cmake_minimum_required(VERSION 3.15)

if (J3D_DIRECTX6)
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

# Disable the problematic warning for MSVC on Jansson target
if(MSVC)
    target_compile_options(${JANSSON_LIBRARIES} PRIVATE 
        /wd4146  # Disable C4146: unary minus operator applied to unsigned type
        /wd4703  # Disable C4703: potentially uninitialized local pointer variable
    )
endif()

