project(sound C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

# Remove DirectX 6/9 specific files based on the build option
if(J3D_DIRECTX6)
  # Remove DX9 files.
    list(FILTER SOURCES EXCLUDE REGEX ".*DX9\\.c$")
elseif(J3D_DIRECTX9 OR J3D_OPENGL) #TODO: Make extra case for OpenGL, once SDL is fully implemented
    # Remove DX6 files.
    list(FILTER SOURCES EXCLUDE REGEX ".*DX6\\.c$")
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC j3dcore)
target_link_libraries(${PROJECT_NAME} PUBLIC rdroid)
target_link_libraries(${PROJECT_NAME} PUBLIC std)
target_link_libraries(${PROJECT_NAME} PUBLIC dsound)

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})
