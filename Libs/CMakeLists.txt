cmake_minimum_required(VERSION 3.12)
project(Libs)

set(LIB_MODULES
    j3dcore
    rdroid
    sith
    smush
    sound
    std
    w32util
    wkernel
)

# Add external libs
add_subdirectory(external)

# Create an interface library to represent all Libs
add_library(${PROJECT_NAME} INTERFACE)

# First, add j3dcore as it's a dependency for other modules
add_subdirectory(j3dcore)
target_link_libraries(${PROJECT_NAME} INTERFACE j3dcore)

# Then add other modules
foreach(MODULE ${LIB_MODULES})
    if(NOT ${MODULE} STREQUAL "j3dcore")
        add_subdirectory(${MODULE})
        target_link_libraries(${PROJECT_NAME} INTERFACE ${MODULE})
    endif()
endforeach()


# Set include directories for the interface library
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Group all lib modules under "Libs" in Visual Studio
set_target_properties(${LIB_MODULES} PROPERTIES FOLDER "Libs")
