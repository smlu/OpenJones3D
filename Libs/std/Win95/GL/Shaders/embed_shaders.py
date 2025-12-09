#!/usr/bin/env python3
"""
Shader Embedding Script - Embeds GLSL files as Byte-Arrays
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


def read_shader_as_bytes(shader_path):
    """Read shader file as bytes"""
    with open(shader_path, 'rb') as f:
        return f.read()


def generate_c_code(shader_dir, output_file):
    """Generates a c file with embedded shaders as Byte-Arrays"""
    shader_dir = Path(shader_dir)
    output_file = Path(output_file)

    shaders = find_shaders(shader_dir)

    if not shaders:
        print(f"No shaders fount in {shader_dir}")
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
        print(f"Load: {rel_path_str}...")

        try:
            shader_bytes = read_shader_as_bytes(shader_path)
            ident = make_c_identifier(rel_path_str)
            shader_data.append((rel_path_str, ident, shader_bytes))
        except Exception as e:
            print(f"Error while loading {rel_path}: {e}")
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

            f.write("};\n")

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

    print(f"\nsuccessfully generated: {output_file}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python embed_shaders.py <shader_dir> <output_file>")
        sys.exit(1)

    shader_dir = sys.argv[1]
    output_file = sys.argv[2]

    generate_c_code(shader_dir, output_file)