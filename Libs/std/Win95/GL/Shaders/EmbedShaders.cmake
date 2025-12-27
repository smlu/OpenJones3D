function(embed_shaders SHADER_DIR OUTPUT_FILE)
    # Location of python script
    set(EMBED_SCRIPT "${SHADER_DIR}/embed_shaders.py")

    if(NOT EXISTS "${EMBED_SCRIPT}")
        message(FATAL_ERROR "embed_shaders.py could nor be found in ${SHADER_DIR}")
    endif()

    # Find shader files
    file(
            GLOB_RECURSE SHADER_FILES
            "${SHADER_DIR}/*.glsl"
            "${SHADER_DIR}/*.frag"
            "${SHADER_DIR}/*.vert"
            "${SHADER_DIR}/*.geom"
            "${SHADER_DIR}/*.tesc"
            "${SHADER_DIR}/*.tese"
            "${SHADER_DIR}/*.comp"
            "${SHADER_DIR}/*.incl"
    )

    if(NOT SHADER_FILES)
        message(STATUS "No Shaders found in ${SHADER_DIR}")
        return()
    endif()

    message(STATUS "Embedded shaders will be generated in ${SHADER_DIR}")
    list(LENGTH SHADER_FILES SHADER_COUNT)
    message(STATUS "  Shader count: ${SHADER_COUNT}")

    # Custom python command for generating shader files
    add_custom_command(
            OUTPUT "${OUTPUT_FILE}"
            COMMAND ${Python3_EXECUTABLE} "${EMBED_SCRIPT}" "${SHADER_DIR}" "${OUTPUT_FILE}"
            DEPENDS ${SHADER_FILES} "${EMBED_SCRIPT}"
            COMMENT "Kompiliere und embedde Shader aus ${SHADER_DIR}"
            VERBATIM
    )

#    execute_process(
#            COMMAND ${Python3_EXECUTABLE} "${EMBED_SCRIPT}" "${SHADER_DIR}" "${OUTPUT_FILE}"
#            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
#            RESULT_VARIABLE EMBED_RESULT
#            OUTPUT_VARIABLE EMBED_OUTPUT
#            ERROR_VARIABLE EMBED_ERROR
#    )

    # Check for changes in shader files
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHADER_FILES})
    target_sources(std PRIVATE ${OUTPUT_FILE})

endfunction()