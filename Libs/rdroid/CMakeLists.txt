project(rdroid C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

# Filter out rdShader when OpenGL is not used.
if(NOT J3D_OPENGL)
    list(FILTER SOURCES EXCLUDE REGEX ".*/Raster/rdShader\\.c$")
    list(FILTER HEADERS EXCLUDE REGEX ".*/Raster/rdShader\\.h$")
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC j3dcore)
target_link_libraries(${PROJECT_NAME} PUBLIC std)

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})
