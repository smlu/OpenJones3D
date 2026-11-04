cmake_minimum_required(VERSION 3.15)
project(Jones3D)

# Added: The Null platform compiles the application layer into an object library so native builds
# validate it. The Windows-only Platform/Win32 files, the Platform/Null headless runner and the
# tests are excluded from the object library; the headless runner links it in below.
if (J3D_PLATFORM_NULL)
    file(GLOB_RECURSE JONES3D_GAME_SOURCES *.c)
    file(GLOB_RECURSE JONES3D_GAME_HEADERS *.h)
    list(FILTER JONES3D_GAME_SOURCES EXCLUDE REGEX ".*/Tests/.*")
    list(FILTER JONES3D_GAME_HEADERS EXCLUDE REGEX ".*/Tests/.*")
    list(FILTER JONES3D_GAME_SOURCES EXCLUDE REGEX ".*/Platform/Win32/.*")
    list(FILTER JONES3D_GAME_HEADERS EXCLUDE REGEX ".*/Platform/Win32/.*")
    list(FILTER JONES3D_GAME_SOURCES EXCLUDE REGEX ".*/Platform/Null/.*")
    list(FILTER JONES3D_GAME_HEADERS EXCLUDE REGEX ".*/Platform/Null/.*")
    # Added: the Babylon column's platform directory (JonesBabylon.c) is linked into the
    # executables that own it, never globbed into the shared application layer.
    list(FILTER JONES3D_GAME_SOURCES EXCLUDE REGEX ".*/Platform/Babylon/.*")
    list(FILTER JONES3D_GAME_HEADERS EXCLUDE REGEX ".*/Platform/Babylon/.*")

    add_library(Jones3DGame OBJECT ${JONES3D_GAME_SOURCES} ${JONES3D_GAME_HEADERS})

    # Same include directory as the Jones3D targets built for Windows.
    target_include_directories(Jones3DGame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

    target_link_libraries(Jones3DGame PUBLIC Libs)

    # Added: The Null headless runner: the application layer object library plus the
    # Platform/Null main(). Link groups/repeated libs resolve the static-library cycles.
    file(GLOB JONES3D_NULL_SOURCES Platform/Null/*.c)

    # Added: the Babylon column swaps the Null main() for the Platform/Babylon composition root.
    set(JONES3D_HEADLESS_SOURCES ${JONES3D_NULL_SOURCES})
    if (J3D_PLATFORM_BABYLON)
        enable_language(CXX)
        list(REMOVE_ITEM JONES3D_HEADLESS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Platform/Null/main.c")
        list(APPEND JONES3D_HEADLESS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Platform/Babylon/main.cpp")
    endif()

    add_executable(Jones3D-headless ${JONES3D_HEADLESS_SOURCES})
    target_include_directories(Jones3D-headless PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

    if (J3D_PLATFORM_BABYLON)
        set_target_properties(Jones3D-headless PROPERTIES LINKER_LANGUAGE CXX)
    endif()

    # Added: The same "Libs" interface dependencies as the Windows Jones3D targets link,
    # repeated inside a GNU ld start/end group so the static-library cycles resolve.
    get_property(JONES3D_LIBS_INTERFACE TARGET Libs PROPERTY INTERFACE_LINK_LIBRARIES)

    # Added: --start-group/--end-group are GNU ld and lld options; macOS's ld64 has no such options
    # and rescans static libraries by itself, so the Apple branch links the same libraries without
    # the group flags. Linux and Windows link lines are unchanged.
    if (APPLE)
        target_link_libraries(Jones3D-headless PRIVATE
            Jones3DGame
            ${JONES3D_LIBS_INTERFACE}
        )
    else()
        target_link_libraries(Jones3D-headless PRIVATE
            -Wl,--start-group
            Jones3DGame
            ${JONES3D_LIBS_INTERFACE}
            -Wl,--end-group
        )
    endif()

    # Added: The Null platform links libm directly (the Windows CRT math functions come
    # with the CRT shim wherever they are shimmed). macOS has no libm: its math functions are
    # in libSystem, which the driver always links, so naming it there would fail to resolve.
    if (NOT APPLE)
        target_link_libraries(Jones3D-headless PRIVATE m)
    endif()

    # Added: the windowed Jones3D of the Babylon column: the Platform/Babylon composition root
    # and run, plus the Platform/Null helpers except its main() and the headless runner. Linked
    # exactly like Jones3D-headless. Jones3D-headless itself never sees these sources.
    if (J3D_PLATFORM_BABYLON)
        set(JONES3D_WINDOWED_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/Platform/Babylon/mainWindowed.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/Platform/Babylon/JonesBabylon.c"
        )
        foreach(JONES3D_NULL_SOURCE ${JONES3D_NULL_SOURCES})
            if (NOT JONES3D_NULL_SOURCE MATCHES "/Platform/Null/(main|JonesHeadless)\\.c$")
                list(APPEND JONES3D_WINDOWED_SOURCES ${JONES3D_NULL_SOURCE})
            endif()
        endforeach()

        add_executable(Jones3D ${JONES3D_WINDOWED_SOURCES})
        target_include_directories(Jones3D PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
        set_target_properties(Jones3D PROPERTIES LINKER_LANGUAGE CXX)

        # Added: no group flags on macOS, as for Jones3D-headless above.
        if (APPLE)
            target_link_libraries(Jones3D PRIVATE
                Jones3DGame
                ${JONES3D_LIBS_INTERFACE}
            )
        else()
            target_link_libraries(Jones3D PRIVATE
                -Wl,--start-group
                Jones3DGame
                ${JONES3D_LIBS_INTERFACE}
                -Wl,--end-group
            )
        endif()
        if (NOT APPLE)
            target_link_libraries(Jones3D PRIVATE m)
        endif()
    endif()
    return()
endif()

# Collect Jones3D sources
file(GLOB_RECURSE JONES3D_SOURCES *.c)
file(GLOB_RECURSE JONES3D_HEADERS *.h)
list(FILTER JONES3D_SOURCES EXCLUDE REGEX ".*/Tests/.*")
list(FILTER JONES3D_HEADERS EXCLUDE REGEX ".*/Tests/.*")
# Added: Windows builds never compile the Null platform backend sources.
list(FILTER JONES3D_SOURCES EXCLUDE REGEX ".*/Platform/Null/.*")
list(FILTER JONES3D_HEADERS EXCLUDE REGEX ".*/Platform/Null/.*")
list(REMOVE_ITEM JONES3D_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/sha256.h")

# Added: One self-contained Jones3D.exe instead of a DLL plus an injector.
# Added: Compile the dialog resources into the standalone exe so its DialogBoxParam calls find their templates.
enable_language(RC)
add_executable(Jones3D WIN32 ${JONES3D_SOURCES} ${JONES3D_HEADERS} Indy3D.rc)

# Set DPI awareness
set_property(TARGET Jones3D PROPERTY VS_DPI_AWARE "PerMonitor")

# Add include to Jones3D
target_include_directories(Jones3D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Link against Libs project
target_link_libraries(Jones3D PRIVATE Libs)
target_link_libraries(Jones3D PRIVATE comctl32)

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${JONES3D_SOURCES} ${JONES3D_HEADERS})

# Added: Same 1 MB stack reserve as the retail exe. Its image base moves so no address in the retail range is ever mapped by this program.
if (MINGW)
    target_link_options(Jones3D PRIVATE -Wl,--stack,1048576 -Wl,--image-base=0x10000000 -Wl,--disable-dynamicbase)
endif()

if (JONES3D_BUILD_TESTS)
    add_subdirectory(Main/Tests)
endif()