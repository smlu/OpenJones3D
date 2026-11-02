project(j3dcore C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

list(FILTER SOURCES EXCLUDE REGEX ".*/Tests/.*")
list(FILTER HEADERS EXCLUDE REGEX ".*/Tests/.*")
list(FILTER HEADERS EXCLUDE REGEX ".*/j3d\\.h$")
# Added: The Microsoft CRT shim builds as its own j3dcrt target on the Null platform; on Windows
# the real CRT is used and nothing about j3dcore changes.
list(FILTER SOURCES EXCLUDE REGEX ".*/j3dcrt\\.c$")
set(J3D_GENERATED_HEADER "${JONES3D_GENERATED_INCLUDE_DIR}/j3dcore/j3d.h")

add_library(${PROJECT_NAME} OBJECT ${SOURCES} ${HEADERS} ${J3D_GENERATED_HEADER})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
target_include_directories(${PROJECT_NAME} BEFORE PUBLIC
    ${JONES3D_GENERATED_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Added: Null platform: build the Microsoft CRT shim as a static library and link it through
# j3dcore's interface, so engine code keeps calling the Microsoft names on non-Windows hosts.
if (J3D_PLATFORM_NULL)
    add_library(j3dcrt STATIC
        j3dcrt.c
        j3dcrt.h
    )
    set_target_properties(j3dcrt PROPERTIES FOLDER "Libs")
    target_link_libraries(${PROJECT_NAME} PUBLIC j3dcrt)
endif()

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})
source_group("Generated" FILES ${J3D_GENERATED_HEADER})

if (JONES3D_BUILD_TESTS OR J3D_PLATFORM_NULL)
    add_subdirectory(Tests)
endif()
