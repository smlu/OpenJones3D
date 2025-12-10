#!/usr/bin/env python3
"""
Shader Embedding Script - Embeds GLSL files as Byte-Arrays with #include support
Usage: python embed_shaders.py <shader_dir> <output_file>
"""

import os
import sys
import re
from pathlib import Path


def make_c_identifier(path_str):
    """converts path to valid C-Identifier"""
    ident = re.sub(r'[^A-Za-z0-9]', '_', path_str)
    if ident and ident[0].isdigit():
        ident = '_' + ident
    return f"stdGLSLShaders_{ident}"


def find_shaders(shader_dir):
    """Finds all shader files recursively"""
    extensions = {'.glsl', '.frag', '.vert', '.geom', '.tesc', '.tese', '.comp'}
    shader_dir = Path(shader_dir)

    shaders = []
    for ext in extensions:
        shaders.extend(shader_dir.rglob(f'*{ext}'))

    return sorted(shaders)


def resolve_includes(shader_path, shader_dir, processed_files=None):
    """
    Resolves #include directives recursively
    Returns the processed shader source as string
    """
    if processed_files is None:
        processed_files = set()

    # Verhindere zirkuläre Includes
    abs_path = shader_path.resolve()
    if abs_path in processed_files:
        print(f"  WARNING: Circular include detected: {shader_path}")
        return ""

    processed_files.add(abs_path)

    try:
        with open(shader_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except UnicodeDecodeError:
        # Fallback zu Latin-1 wenn UTF-8 fehlschlägt
        with open(shader_path, 'r', encoding='latin-1') as f:
            lines = f.readlines()

    result = []
    include_pattern = re.compile(r'^\s*#\s*include\s+[<"]([^>"]+)[>"]')

    for line_num, line in enumerate(lines, 1):
        match = include_pattern.match(line)
        if match:
            include_file = match.group(1)

            # Suche Include-Datei relativ zum aktuellen Shader
            include_path = shader_path.parent / include_file

            # Falls nicht gefunden, suche relativ zum Shader-Root
            if not include_path.exists():
                include_path = shader_dir / include_file

            if include_path.exists():
                print(f"    Including: {include_file}")
                # Rekursiv includes auflösen
                included_content = resolve_includes(include_path, shader_dir, processed_files.copy())
                result.append(f"// BEGIN INCLUDE: {include_file}\n")
                result.append(included_content)
                result.append(f"// END INCLUDE: {include_file}\n")
            else:
                print(f"  WARNING: Include not found: {include_file} (referenced in {shader_path}:{line_num})")
                result.append(f"// ERROR: Include not found: {include_file}\n")
        else:
            result.append(line)

    return ''.join(result)


def process_shader(shader_path, shader_dir):
    """
    Processes a shader file, resolves includes, and returns bytes
    """
    processed_source = resolve_includes(shader_path, shader_dir)
    return processed_source.encode('utf-8')


def generate_c_code(shader_dir, output_file):
    """Generates a c file with embedded shaders as Byte-Arrays"""
    shader_dir = Path(shader_dir)
    output_file = Path(output_file)

    shaders = find_shaders(shader_dir)

    if not shaders:
        print(f"No shaders found in {shader_dir}")
        return

    print(f"Found Shaders: {len(shaders)}")
    for s in shaders:
        print(f"  - {s.relative_to(shader_dir)}")

    # load shader
    shader_data = []
    for shader_path in shaders:
        rel_path = shader_path.relative_to(shader_dir)
        # convert path slashes
        rel_path_str = str(rel_path).replace('\\', '/')
        print(f"Processing: {rel_path_str}...")

        try:
            shader_bytes = process_shader(shader_path, shader_dir)
            ident = make_c_identifier(rel_path_str)
            shader_data.append((rel_path_str, ident, shader_bytes))
        except Exception as e:
            print(f"Error while processing {rel_path}: {e}")
            import traceback
            traceback.print_exc()
            continue

    if not shader_data:
        print("No Shaders could be loaded!")
        return

    # write to C file
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("/* Auto-generated shader embed file */\n\n")
        f.write("#include <stddef.h>\n")
        f.write("#include <string.h>\n")
        f.write("#include <std/Win95/GL/Shaders/stdGLSLShaders.h>\n\n")

        # Write bytecode arrays
        for rel_path, ident, shader_bytes in shader_data:
            f.write(f"/* {rel_path} */\n")
            f.write(f"static const char {ident}[] = {{\n")

            # 12 bytes per line
            for i in range(0, len(shader_bytes), 12):
                chunk = shader_bytes[i:i+12]
                hex_bytes = ', '.join(f'0x{b:02x}' for b in chunk)
                # Kein Komma nach dem letzten Byte
                if i + 12 >= len(shader_bytes):
                    f.write(f"    {hex_bytes}\n")
                else:
                    f.write(f"    {hex_bytes},\n")

            f.write("};\n\n")

        # Lookup-Table
        f.write("typedef struct { const char* name; const char* src; size_t size; } tShaderEntry;\n\n")
        f.write("static const tShaderEntry stdGLSLShaders_shaders[] = {\n")

        for i, (rel_path, ident, _) in enumerate(shader_data):
            comma = ',' if i < len(shader_data) - 1 else ''
            f.write(f"    {{ \"{rel_path}\", {ident}, sizeof({ident}) }}{comma}\n")

        f.write("};\n")
        f.write(f"static const size_t stdGLSLShaders_shadersCount = {len(shader_data)};\n\n")

        # Getter-Function for shader source
        f.write("const char* stdGLSLShaders_GetShader(const char* shaderName, size_t* outSize)\n{\n")
        f.write("    for ( size_t i = 0; i < stdGLSLShaders_shadersCount; i++ )\n    {\n")
        f.write("        if ( strcmp(shaderName, stdGLSLShaders_shaders[i].name) == 0 )\n        {\n")
        f.write("            if (outSize) *outSize = stdGLSLShaders_shaders[i].size;\n")
        f.write("            return stdGLSLShaders_shaders[i].src;\n")
        f.write("        }\n")
        f.write("    }\n\n")
        f.write("    if (outSize) *outSize = 0;\n")
        f.write("    return NULL;\n")
        f.write("}\n")

    print(f"\nSuccessfully generated: {output_file}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python embed_shaders.py <shader_dir> <output_file>")
        sys.exit(1)

    shader_dir = sys.argv[1]
    output_file = sys.argv[2]

    generate_c_code(shader_dir, output_file)