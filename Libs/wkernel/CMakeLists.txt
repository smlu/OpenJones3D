project(wkernel C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

# Added: The Null platform has no Windows kernel services, so only the wkernelNull.c stub
# is compiled (wkernel.c calls the Win32 API and stays a Windows-only file).
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
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC comctl32)
endif()

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

