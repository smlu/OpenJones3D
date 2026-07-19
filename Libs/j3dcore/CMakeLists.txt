project(j3dcore C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

list(FILTER SOURCES EXCLUDE REGEX ".*/Tests/.*")
list(FILTER HEADERS EXCLUDE REGEX ".*/Tests/.*")
list(FILTER HEADERS EXCLUDE REGEX ".*/j3d\\.h$")
set(J3D_GENERATED_HEADER "${JONES3D_GENERATED_INCLUDE_DIR}/j3dcore/j3d.h")

add_library(${PROJECT_NAME} OBJECT ${SOURCES} ${HEADERS} ${J3D_GENERATED_HEADER})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
target_include_directories(${PROJECT_NAME} BEFORE PUBLIC
    ${JONES3D_GENERATED_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})
source_group("Generated" FILES ${J3D_GENERATED_HEADER})

if (JONES3D_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
