project(smush C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

list(FILTER SOURCES EXCLUDE REGEX ".*/Tests/.*")
list(FILTER HEADERS EXCLUDE REGEX ".*/Tests/.*")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC j3dcore)
target_link_libraries(${PROJECT_NAME} PUBLIC rdroid)
target_link_libraries(${PROJECT_NAME} PUBLIC std)
target_link_libraries(${PROJECT_NAME} PUBLIC dsound)
target_link_libraries(${PROJECT_NAME} PUBLIC Winmm) # timeGetTime
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

if (JONES3D_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
