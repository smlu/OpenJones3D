cmake_minimum_required(VERSION 3.15)
project(SDL3)

# Function to create an imported library
function(add_directx_library name lib_file)
    add_library(${PROJECT_NAME}::${name} STATIC IMPORTED GLOBAL)
    set_target_properties(${PROJECT_NAME}::${name} PROPERTIES
            IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/x86/${lib_file}"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3"
    )
endfunction()

# Create imported libraries for each DirectX component
add_directx_library(SDL3 SDL3.lib)

# Create a meta-target that includes all DirectX components
add_library(SDL3 INTERFACE)
target_link_libraries(SDL3 INTERFACE
        SDL3::SDL3
)