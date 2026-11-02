project(smush C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

list(FILTER SOURCES EXCLUDE REGEX ".*/Tests/.*")
list(FILTER HEADERS EXCLUDE REGEX ".*/Tests/.*")

# Added: The Null platform has no Windows DirectSound timer or DirectSound sound
# output: exclude the backend files.
if(J3D_PLATFORM_NULL)
    list(FILTER SOURCES EXCLUDE REGEX "Dsnd\\.c$")
    list(FILTER SOURCES EXCLUDE REGEX "TimerWin32\\.c$")
else()
    # Windows builds never compile the Null backend sources.
    list(FILTER SOURCES EXCLUDE REGEX ".*Null\\.c$")
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC j3dcore)
target_link_libraries(${PROJECT_NAME} PUBLIC rdroid)
target_link_libraries(${PROJECT_NAME} PUBLIC std)

# Added: Windows-only services; the Null platform has no dsound and no winmm.
if (NOT J3D_PLATFORM_NULL)
    target_link_libraries(${PROJECT_NAME} PUBLIC dsound)
    target_link_libraries(${PROJECT_NAME} PUBLIC winmm) # timeGetTime
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

if (JONES3D_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
