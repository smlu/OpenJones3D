project(std C)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)

list(FILTER SOURCES EXCLUDE REGEX ".*/Tests/.*")
list(FILTER HEADERS EXCLUDE REGEX ".*/Tests/.*")

# Added: The Null platform has no Windows or DirectX backend: exclude the Win95 backend tree
# (DX6/DX9 device code, Windows console/comm services) and every Win32-specific file. stdGob.c is
# built here: its memory-mapped-file branch, which is dead because the streamed path is always
# chosen, is the only part guarded out for non-Windows.
if(J3D_PLATFORM_NULL)
    list(FILTER SOURCES EXCLUDE REGEX ".*/Win95/DX6/.*")
    list(FILTER SOURCES EXCLUDE REGEX ".*/Win95/DX9/.*")
    list(FILTER SOURCES EXCLUDE REGEX ".*/Win95/Tests/.*")
    list(FILTER SOURCES EXCLUDE REGEX ".*Win32\\.c$")
    list(FILTER SOURCES EXCLUDE REGEX "stdWin95\\.c$")
    list(FILTER SOURCES EXCLUDE REGEX "stdComm\\.c$")
    list(FILTER SOURCES EXCLUDE REGEX "stdConsole\\.c$")
else()
    # Windows builds never compile the Null backend sources.
    list(FILTER SOURCES EXCLUDE REGEX ".*/Null/.*")
    list(FILTER SOURCES EXCLUDE REGEX ".*Null\\.c$")
endif()

# Remove DirectX 6/9 specific files based on the build option
if(J3D_DIRECTX6)
  # Remove DX9 files.
    list(FILTER SOURCES EXCLUDE REGEX ".*DX9\\.c$")
    list(FILTER SOURCES EXCLUDE REGEX "stdComm.c$")

    set(SHADERS "")
    set(SHADER_INCLUDES "")
elseif(J3D_DIRECTX9)
    # Remove DX6 files.
    list(FILTER SOURCES EXCLUDE REGEX ".*DX6\\.c$")

    # Collect shader files
    set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Win95/DX9/Shaders")
    file(GLOB SHADERS
        "${SHADER_DIR}/*.hlsl"
    )
    file(GLOB SHADER_INCLUDES
        "${SHADER_DIR}/*.hlsli"
    )
endif()

# Make project target
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS} ${SHADERS} ${SHADER_INCLUDES})

# Add global includes
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Added: The DirectX 9 backend directory holds the backend types defines and the DirectX 9 API
# interface (std/types.h includes stdBackendTypes.h from here unconditionally).
if (J3D_DIRECTX9)
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Win95/DX9)
endif()

# Added: The Null platform provides its own replacement backend types header here.
if (J3D_PLATFORM_NULL)
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Null)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    j3dcore
)

# Added: Windows-only services; the Null platform has none of them (no DirectX, no winmm).
if (NOT J3D_PLATFORM_NULL)
    target_link_libraries(${PROJECT_NAME} PUBLIC
        winmm # timeGetTime
    )
endif()

set(EXTRA_LIBS
    jansson::jansson
)

# Added: the Null platform links no DirectX backend, but stdJSON.c still needs jansson.
if (J3D_PLATFORM_NULL)
    target_link_libraries(${PROJECT_NAME} PUBLIC
        ${EXTRA_LIBS}
    )
endif()

if (J3D_DIRECTX6)
    target_link_libraries(${PROJECT_NAME} PUBLIC
        DirectX61c::ddraw
        DirectX61c::dxguid
        DirectX61c::dinput
        DirectX61c::dplayx
        ${EXTRA_LIBS}
    )
elseif(J3D_DIRECTX9) 
    target_link_libraries(${PROJECT_NAME} PUBLIC
        d3d9
        dxguid
        dinput8
        ${EXTRA_LIBS}
    )

    if (MINGW)
        # GCC ignores the #pragma comment(lib, ...) hints in stdControlDX9.c and stdShaderDX9.c
        # (overdue XInput/wbemuuid/d3dcompiler/oleaut32/ole32), so they are added here
        # (GNU ld is order-sensitive; CMake repeats cyclic static libraries on the link line).
        target_link_libraries(${PROJECT_NAME} PUBLIC
            xinput
            wbemuuid
            d3dcompiler_47
            ole32
            oleaut32
        )

        # Wine has no MSVC shader compiler, so build the hlslc D3DCompile wrapper (cmake/shaders/hlslc.c)
        # and generate the shader headers with it into this directory.
        add_executable(std_hlslc ${CMAKE_SOURCE_DIR}/cmake/shaders/hlslc.c)
        target_link_libraries(std_hlslc PRIVATE d3dcompiler_47)

        set(JONES3D_SHADER_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
        file(MAKE_DIRECTORY "${JONES3D_SHADER_HEADER_DIR}")
    endif()

    # Define HLSL shaders to be compiled
    foreach(SHADER ${SHADERS})
        get_filename_component(FILE_NAME ${SHADER} NAME_WE) # name without extension
        set(HEADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Win95/DX9/Shaders/std_${FILE_NAME}.h")

        # Get shader type from filename
        set(SHADER_MODEL "3.0")
        if(FILE_NAME MATCHES "_vs$")
            set(SHADER_TYPE "Vertex")
        elseif(FILE_NAME MATCHES "_ps$")
            set(SHADER_TYPE "Pixel")
        else()
            message(FATAL_ERROR "Unknown shader type for ${FILE_NAME} - expected _vs or _ps suffix!")
        endif()

        # Adjust shader compilation properties
        set_source_files_properties(${SHADER} PROPERTIES
            VS_SHADER_TYPE ${SHADER_TYPE}
            VS_SHADER_MODEL ${SHADER_MODEL}
            VS_SHADER_ENTRYPOINT "main"
            VS_SHADER_OUTPUT_HEADER_FILE "${HEADER_FILE}"
            VS_SHADER_OBJECT_FILE_NAME ""
            VS_SHADER_VARIABLE_NAME "std_${FILE_NAME}"
        )

        # Add dependency so headers are built before your game compiles
        list(APPEND GENERATED_HEADERS ${HEADER_FILE})

        if (MINGW)
            set(SHADER_TARGET "ps_3_0")
            if (SHADER_TYPE STREQUAL "Vertex")
                set(SHADER_TARGET "vs_3_0")
            endif()

            add_custom_command(
                OUTPUT "${JONES3D_SHADER_HEADER_DIR}/Shaders/std_${FILE_NAME}.h"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${JONES3D_SHADER_HEADER_DIR}/Shaders"
                # Required: Wine's shader preprocessor does not expand the nested HLSL
                # macros, so run the preprocessed source through GCC into the shader
                # header directory before compiling it with std_hlslc under Wine.
                COMMAND ${CMAKE_C_COMPILER} -E -P -x c -o "${FILE_NAME}.pp.hlsl" "${SHADER}"
                COMMAND std_hlslc "${FILE_NAME}.pp.hlsl" "${SHADER_TARGET}" "Shaders/std_${FILE_NAME}.h" "std_${FILE_NAME}"
                WORKING_DIRECTORY "${JONES3D_SHADER_HEADER_DIR}"
                DEPENDS "${SHADER}" "${SHADER_DIR}/common.hlsli" std_hlslc
                VERBATIM
            )
            list(APPEND MINGW_GENERATED_HEADERS "${JONES3D_SHADER_HEADER_DIR}/Shaders/std_${FILE_NAME}.h")
        endif()
    endforeach()

    if (MINGW)
        add_custom_target(std_shaders DEPENDS ${MINGW_GENERATED_HEADERS})
        add_dependencies(${PROJECT_NAME} std_shaders)
    endif()
endif()

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC
        dbghelp # for minidump & stack trace
    )
endif()

if (MSVC)
    target_link_options(${PROJECT_NAME} INTERFACE "/SAFESEH:NO") # due to DirectX 6.1 libs
endif()

if (JONES3D_SHADER_HEADER_DIR)
    # The MinGW build generates these headers into the build tree (std_shaders above).
    target_include_directories(${PROJECT_NAME} PUBLIC ${JONES3D_SHADER_HEADER_DIR})
endif()

# Organize source files in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS} ${SHADERS} ${SHADER_INCLUDES} ${GENERATED_HEADERS})

if (JONES3D_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
