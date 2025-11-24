cmake_minimum_required(VERSION 3.15)

if (J3D_OPENGL)
    # SDL3
    set(SDL_STATIC ON)
    set(SDL_SHARED OFF)
    set(SDL_TESTS  OFF)
    set(SDL_TEST_LIBRARY  OFF)
    set(SDL_LIBC  ON)

    add_subdirectory(SDL3)
    set_target_properties(SDL3-static PROPERTIES FOLDER "Libs/external/SDL3") # Group all lib modules under "Libs/external" in Visual Studio
    set_target_properties(SDL_uclibc PROPERTIES FOLDER "Libs/external/SDL3") # Group all lib modules under "Libs/external" in Visual Studio

    # GLAD
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    message(STATUS "Python executable: ${Python3_EXECUTABLE}")

    # optional: Versuch, jinja2 zu installieren, falls nicht vorhanden
    execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import jinja2; print('jinja2-ok')"
            RESULT_VARIABLE _jinja_check
            OUTPUT_QUIET
            ERROR_QUIET
    )

    if(NOT _jinja_check EQUAL 0)
        message(STATUS "jinja2 not found — installing via pip (this requires network access)...")
        execute_process(
                COMMAND ${Python3_EXECUTABLE} -m pip install --upgrade pip setuptools
                RESULT_VARIABLE _pip_upgrade_res
        )
        execute_process(
                COMMAND ${Python3_EXECUTABLE} -m pip install jinja2
                RESULT_VARIABLE _pip_install_res
        )
        if(NOT _pip_install_res EQUAL 0)
            message(FATAL_ERROR "Failed to install jinja2 via pip. Please install it manually or use a venv.")
        endif()
    endif()

    set(GLAD_API "gl:compatibility=3.3")
    set(GLAD_DIR "${CMAKE_SOURCE_DIR}/Libs/external/glad")
    set(GLAD_GENERATED_DIR "${CMAKE_SOURCE_DIR}/Libs/external/glad/build")

    if(EXISTS "${GLAD_GENERATED_DIR}")
        message(STATUS "Cleaning old glad generated files in ${GLAD_GENERATED_DIR}")
        file(REMOVE_RECURSE "${GLAD_GENERATED_DIR}")
    endif()

    if(EXISTS "${GLAD_GENERATED_DIR}")
        message(STATUS "Cleaning old glad generated files in ${GLAD_GENERATED_DIR}")
        file(REMOVE_RECURSE "${GLAD_GENERATED_DIR}")
    endif()

    execute_process(
            COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${GLAD_DIR}
            ${Python3_EXECUTABLE} -m glad --api=${GLAD_API} --out-path ${GLAD_GENERATED_DIR}
            RESULT_VARIABLE _glad_result
            OUTPUT_VARIABLE _glad_out
            ERROR_VARIABLE _glad_err
    )


    if(NOT _glad_result EQUAL 0)
        message(FATAL_ERROR "GLAD generation failed:\n${_glad_err}")
    else()
        message(STATUS "GLAD generated successfully at: ${GLAD_GENERATED_DIR}")
    endif()

    add_library(glad STATIC ${GLAD_GENERATED_DIR}/src/gl.c)

    target_include_directories(glad PUBLIC ${GLAD_GENERATED_DIR}/include)
    set_target_properties(glad PROPERTIES FOLDER "Libs/external")


#    add_custom_target(generate_glad DEPENDS
#            ${GLAD_GENERATED_DIR}/include/glad/gl.h
#            ${GLAD_GENERATED_DIR}/src/gl.c
#    )
#
#    add_library(glad STATIC
#            ${GLAD_GENERATED_DIR}/src/gl.c
#    )
#
#    add_dependencies(glad generate_glad)
#
#    target_include_directories(glad PUBLIC ${GLAD_GENERATED_DIR}/include)
#    set_target_properties(glad PROPERTIES FOLDER "Libs/external")

elseif (J3D_DIRECTX6)
    set(EXTERNAL_MODULES
        DirectX61c
    )
endif()

# Add external modules
set(EXTERNAL_MODULES
    ${EXTERNAL_MODULES}
    jansson
)

# Set Jansson config
set(JANSSON_BUILD_SHARED_LIBS OFF)
set(JANSSON_INSTAL OFF)
set(JANSSON_BUILD_DOCS OFF) 
set(JANSSON_WITHOUT_TESTS ON)
set(JANSSON_EXAMPLES OFF)

if (MSVC)
    set(JANSSON_STATIC_CRT ON )
endif ()

# Include dependency subdirs in CMake project
foreach(MODULE ${EXTERNAL_MODULES})
    add_subdirectory(${MODULE})
    set_target_properties(${MODULE} PROPERTIES FOLDER "Libs/external") # Group all lib modules under "Libs/external" in Visual Studio
endforeach()


# Add Jansson library
add_library(jansson_interface INTERFACE)
target_link_libraries(jansson_interface INTERFACE ${JANSSON_LIBRARIES})
target_include_directories(jansson_interface INTERFACE
   "${JANSSON_INCLUDE_DIRS}"
)
add_library(Jansson ALIAS jansson_interface)

# Disable the problematic warning for MSVC on Jansson target
if(MSVC)
    target_compile_options(${JANSSON_LIBRARIES} PRIVATE
        /wd4146  # Disable C4146: unary minus operator applied to unsigned type
        /wd4703  # Disable C4703: potentially uninitialized local pointer variable
    )
endif()
