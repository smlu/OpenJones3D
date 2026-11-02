project(w32util C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

# Added: The Null platform has no Windows utilities, so only the wuRegistryNull.c stub is
# compiled (wuRegistry.c calls the Win32 registry API and stays a Windows-only file).
if(J3D_PLATFORM_NULL)
    file(GLOB NULL_SOURCES *Null.c)
    add_library(${PROJECT_NAME} STATIC ${NULL_SOURCES} ${HEADERS})
else()
    # Windows builds never compile the Null backend sources.
    list(FILTER SOURCES EXCLUDE REGEX ".*Null\\.c$")
    add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC j3dcore std)

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})
