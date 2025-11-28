function(embed_shaders SHADER_DIR OUTPUT_HEADER)

    set(SHADER_SOURCE "${OUTPUT_HEADER}.c")

    # 1️⃣ Shader-Dateien rekursiv suchen
    file(
            GLOB_RECURSE SHADER_FILES
            RELATIVE "${SHADER_DIR}"
            "${SHADER_DIR}/*.glsl"
            "${SHADER_DIR}/*.frag"
            "${SHADER_DIR}/*.vert"
    )

    if(NOT SHADER_FILES)
        message(STATUS "No shader files found in ${SHADER_DIR}")
        return()
    endif()

    list(LENGTH SHADER_FILES SHADER_COUNT)

    message(STATUS "Found shaders: ${SHADER_FILES}")

    # Hilfsfunktion: Pfad -> C Identifier
    function(_make_ident out str)
        string(REGEX REPLACE "[^A-Za-z0-9]" "_" tmp "${str}")
        string(REGEX MATCH "^[0-9]" _first "${tmp}")
        if(_first)
            set(tmp "_${tmp}")
        endif()
        set(${out} "${tmp}" PARENT_SCOPE)
    endfunction()


    file(WRITE "${SHADER_SOURCE}" "/* Auto-generated shader embed file */\n")
    #file(APPEND "${SHADER_HEADER}" "#pragma once\n#include <stddef.h>\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
    file(APPEND "${SHADER_SOURCE}" "#include <stddef.h>\n#include <string.h>\n#include <std/Win95/GL/Shaders/stdGLSLShaders.h>\n\n")

    # Listen für Identifier und Pfade
    set(_idents)
    set(_fnames)

    # 2️⃣ Shader-Strings erzeugen
    foreach(relpath IN LISTS SHADER_FILES)
        set(fullpath "${SHADER_DIR}/${relpath}")
        _make_ident(ident "stdGLSLShaders_${relpath}")

        list(APPEND _idents "${ident}")
        list(APPEND _fnames "${relpath}")

        # Datei zeilenweise lesen
        file(STRINGS "${fullpath}" _lines)

        # Shader-String schreiben
        file(APPEND "${SHADER_SOURCE}" "/* ${relpath} */\n")
        file(APPEND "${SHADER_SOURCE}" "static const char ${ident}[] =\n")
        foreach(line IN LISTS _lines)
            string(REPLACE "\\" "\\\\" line_esc "${line}")
            string(REPLACE "\"" "\\\"" line_esc "${line_esc}")
            file(APPEND "${SHADER_SOURCE}" "    \"${line_esc}\\n\"\n")
        endforeach()
        if(_lines STREQUAL "")
            file(APPEND "${SHADER_SOURCE}" "    \"\"\n")
        endif()
        file(APPEND "${SHADER_SOURCE}" ";\n")
        #file(APPEND "${SHADER_HEADER}" "static const size_t ${ident}_size = sizeof(${ident}) - 1;\n\n")
    endforeach()

    # 3️⃣ Lookup-Tabelle erzeugen
    file(APPEND "${SHADER_SOURCE}" "typedef struct { const char* name; const char* src; size_t size; } tShaderEntry;\n\n")
    file(APPEND "${SHADER_SOURCE}" "static const tShaderEntry stdGLSLShaders_shaders[] = {\n")

    list(LENGTH _idents _count)
    math(EXPR _count_minus1 "${_count} - 1")
    math(EXPR _count_minus2 "${_count} - 2")
    foreach(idx RANGE 0 ${_count_minus2})
        list(GET _fnames ${idx} fname)
        list(GET _idents ${idx} ident)
        file(APPEND "${SHADER_SOURCE}" "    { \"${fname}\", ${ident} },\n")
    endforeach()

    list(GET _fnames ${_count_minus1} fname)
    list(GET _idents ${_count_minus1} ident)
    file(APPEND "${SHADER_SOURCE}" "    { \"${fname}\", ${ident} }\n")

    file(APPEND "${SHADER_SOURCE}" "};\n")
    file(APPEND "${SHADER_SOURCE}" "static const size_t stdGLSLShaders_shadersCount = ${SHADER_COUNT};\n\n")
    #file(APPEND "${SHADER_HEADER}" "#ifdef __cplusplus\n}\n#endif\n")
    file(APPEND "${SHADER_SOURCE}" "const char* stdGLSLShaders_GetShader(const char* shaderName)\n{\n")
    file(APPEND "${SHADER_SOURCE}" "    for ( size_t i = 0; i < stdGLSLShaders_shadersCount; i++ )\n    {\n")
    file(APPEND "${SHADER_SOURCE}" "        if ( strcmp(shaderName, stdGLSLShaders_shaders[i].name) == 0 )\n        {\n")
    file(APPEND "${SHADER_SOURCE}" "            return stdGLSLShaders_shaders[i].src;\n")
    file(APPEND "${SHADER_SOURCE}" "        }\n")
    file(APPEND "${SHADER_SOURCE}" "    }\n\n")
    file(APPEND "${SHADER_SOURCE}" "    return NULL;\n}")

    # 4️⃣ CMake: Shader-Dateien überwachen
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHADER_FILES})

endfunction()
