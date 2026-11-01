project(wkernel C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

# Added: The Null platform has no Windows kernel services: build headers only, no .c files
# (wkernel.c calls the Win32 API and stays a Windows-only interface for upper modules).
if(J3D_PLATFORM_NULL)
    add_library(${PROJECT_NAME} INTERFACE ${HEADERS})
    target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_link_libraries(${PROJECT_NAME} INTERFACE j3dcore std comctl32)
else()
    add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_link_libraries(${PROJECT_NAME} PUBLIC j3dcore std comctl32)
endif()

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

