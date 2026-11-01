project(sound C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

list(FILTER SOURCES EXCLUDE REGEX ".*/Tests/.*")
list(FILTER HEADERS EXCLUDE REGEX ".*/Tests/.*")

# Added: The Null platform has no DirectSound driver: exclude the backend driver files.
if(J3D_PLATFORM_NULL)
    list(FILTER SOURCES EXCLUDE REGEX "DriverDX.*\\.c$")
else()
    # Windows builds never compile the Null backend sources.
    list(FILTER SOURCES EXCLUDE REGEX ".*Null\\.c$")
endif()

# Remove DirectX 6/9 specific files based on the build option
if(J3D_DIRECTX6)
  # Remove DX9 files.
    list(FILTER SOURCES EXCLUDE REGEX ".*DX9\\.c$")
elseif(J3D_DIRECTX9)
    # Remove DX6 files.
    list(FILTER SOURCES EXCLUDE REGEX ".*DX6\\.c$")
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC j3dcore)
target_link_libraries(${PROJECT_NAME} PUBLIC rdroid)
target_link_libraries(${PROJECT_NAME} PUBLIC std)

# Added: Windows-only audio service; the Null platform has no dsound.
if (NOT J3D_PLATFORM_NULL)
    target_link_libraries(${PROJECT_NAME} PUBLIC dsound)
endif()

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

if (JONES3D_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
