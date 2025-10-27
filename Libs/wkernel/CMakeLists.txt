project(wkernel C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

if (J3D_OPENGL)
    list(FILTER SOURCES EXCLUDE REGEX "wkernel.c$")
else()
    list(FILTER SOURCES EXCLUDE REGEX "wkernelGL.c$")
endif ()

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC j3dcore std Comctl32.lib)

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

