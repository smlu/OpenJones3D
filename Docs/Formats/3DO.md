# 3DO File Format

This document describes the text `.3do` model format used by OpenJones3D.

In the current codebase, `.3do` files are loaded by [`rdModel3_LoadEntry()`](../../Libs/rdroid/Primitives/rdModel3.c) and wrapped by the gameplay-side loader [`sithModel_Load()`](../../Libs/sith/World/sithModel.c).

## Purpose

A `.3do` file defines one model resource:

- the material list used by the model
- one or more geometry sets
- one or more meshes per geometry set
- vertex, texture-vertex, face, and normal data
- the hierarchy tree that attaches meshes to model nodes

At runtime, the parsed file becomes one [`rdModel3`](../../Libs/rdroid/types.h).

## File Location And Naming

The normal gameplay loader resolves model filenames like this:

- if high-poly models are enabled, it first tries `hi3do\<name>.3do`
- otherwise, or if the high-poly file is missing, it uses `3do\<name>.3do`

That logic lives in [`sithModel_Load()`](../../Libs/sith/World/sithModel.c).

Important notes:

- the gameplay loader ensures the `.3do` extension is present
- model cache lookup is by filename/basename string

## Missing-Model Fallback

If the requested model cannot be loaded:

- and the call is allowed to fall back, the engine retries with `dflt.3do`
- if `dflt.3do` also fails, the load fails completely

This fallback is implemented in [`sithModel_Load()`](../../Libs/sith/World/sithModel.c).

## Related Data Loaded By 3DO Entries

Material names from the model file are passed to the material loader and normally resolved under:

```text
mat\<filename>
```

See [MAT](MAT.md).

## Parser Rules

`.3do` files use the generic [`stdConffile`](../../Libs/std/General/stdConffile.c) parser.

| Behavior | Current implementation |
| --- | --- |
| Case sensitivity | Entire non-comment lines are lowercased before tokenization, so keywords are effectively case-insensitive. Material names and mesh names read through the parser are lowercased too. |
| Whole-line comments | Lines starting with `#` or `;` are skipped. |
| Inline comments | Inline `#` comments are stripped. |
| Separators | Tokens are separated by commas, spaces, and tabs. |
| Section headers | The loader expects section-header lines in the right places, but only lightly validates the section-name text. |
| End marker | There is no file-local `end` marker. Parsing runs until the expected structures are read or an error occurs. |

## Supported 3DO Versions

The current loader recognizes `3DO 2.x` with these minor versions:

| Version | Meaning in the current loader |
| --- | --- |
| `2.1` | Intensity vertex/face lighting data. |
| `2.2` | RGB vertex/face lighting data. |
| `2.3` | RGBA vertex/face lighting data. |

The shipped combined assets inspected for this document are primarily `2.3`, but the loader still contains dedicated handling for `2.1` and `2.2`.

## Top-Level Layout

The canonical layout is:

```text
SECTION: HEADER
3DO <major>.<minor>

SECTION: MODELRESOURCE
MATERIALS <count>
<material rows>

SECTION: GEOMETRYDEF
RADIUS <value>
INSERT OFFSET <x> <y> <z>
GEOSETS <count>
<geoset blocks>

SECTION: HIERARCHYDEF
HIERARCHY NODES <count>
<hierarchy node rows>
```

## Material Section

The material section begins with:

```text
MATERIALS <count>
```

and is followed by indexed rows:

```text
0: material_a.mat
1: material_b.mat
```

Each material name is loaded immediately through the material loader.

## Geometry Section

The geometry section contains:

| Directive | Meaning |
| --- | --- |
| `RADIUS` | Model radius stored in `rdModel3.radius`. |
| `INSERT OFFSET` | Model insertion offset vector. |
| `GEOSETS` | Number of geometry sets stored in `aGeos[]`. |

Each geoset then begins with:

```text
GEOSET <n>
MESHES <count>
```

## Mesh Block

Each mesh block contains:

```text
MESH <n>
NAME <name>
RADIUS <value>
GEOMETRYMODE <mode>
LIGHTINGMODE <mode>
TEXTUREMODE <mode>
VERTICES <count>
...
TEXTURE VERTICES <count>
...
VERTEX NORMALS
...
FACES <count>
...
FACE NORMALS
...
```

Notes:

- `GEOMETRYMODE` uses the standard render geometry modes. See [Render Geometry Modes](../COG/Types-And-Modes.md#render-geometry-modes).
- `LIGHTINGMODE` uses the standard render light modes. See [Render Light Modes](../COG/Types-And-Modes.md#render-light-modes).
- `TEXTUREMODE` is parsed, but not used by the current loader.

## Vertex Rows

For `2.1` files, each vertex row has:

```text
<idx>: <x> <y> <z> <intensity>
```

For `2.2` files, each vertex row has:

```text
<idx>: <x> <y> <z> <red> <green> <blue>
```

For `2.3` files, each vertex row typically has:

```text
<idx>: <x> <y> <z> <red> <green> <blue> <alpha>
```

The loader also accepts RGB rows without the alpha component and defaults alpha to `1.0`.

## Texture-Vertex Rows

Texture vertices are stored as indexed UV rows:

```text
<idx>: <u> <v>
```

## Face Rows

Each face line is parsed token by token and has this shape:

```text
<faceNum>: <materialIdx> <faceFlagsHex> <geometryMode> <lightMode> <textureMode> <extraLight> <vertexCount> <vertIdx,texIdx>...
```

Example from a `2.3` file:

```C
0: 0 0x0005 4 3 3 (0.000000/0.000000/0.000000/1.000000) 4 0,0 1,1 3,2 2,3
```

Column meaning:

| Field | Meaning |
| --- | --- |
| `materialIdx` | Material list index. `-1` means the face is untextured. |
| `faceFlagsHex` | Face flags bitmask. See [Face Flags](../COG/Flags.md#face-flags). |
| `geometryMode` | Per-face geometry mode. |
| `lightMode` | Per-face lighting mode. |
| `textureMode` | Parsed, but not used by the current loader. |
| `extraLight` | Either intensity or RGB/RGBA extra-light data depending on file version. |
| `vertexCount` | Number of vertices in this face. |
| `vertIdx,texIdx` pairs | Vertex and texture-vertex indices for each corner. |

Important notes:

- even when a face has no material (`materialIdx == -1`), the file still contains `vertIdx,texIdx` pairs
- the loader ignores the texture-vertex half of each pair for untextured faces
- out-of-range vertex or texture-vertex indices are clamped to `0` and logged as errors

## Normal Sections

The literal `VERTEX NORMALS` and `FACE NORMALS` header lines are read as fixed markers and skipped, then followed by indexed normal rows:

```text
<idx>: <x> <y> <z>
```

## Hierarchy Section

The hierarchy section begins with:

```text
SECTION: HIERARCHYDEF
HIERARCHY NODES <count>
```

Each hierarchy node row has the shape:

```text
<num>: <flags> <type> <mesh> <parent> <child> <sibling> <numChildren> <x> <y> <z> <pitch> <yaw> <roll> <pivotx> <pivoty> <pivotz> <name>
```

Important notes:

- `parent`, `child`, and `sibling` use `-1` to mean “no linked node”
- the loader resolves those indices into pointers after reading the row
- the hierarchy `flags` column is parsed, but the current [`rdModel3HNode`](../../Libs/rdroid/types.h) structure does not store it

## In-Memory Layout

Important structures populated by the loader:

| Structure | Meaning |
| --- | --- |
| `rdModel3` | Top-level model resource. |
| `rdModel3GeoSet` | One geometry-set container. |
| `rdModel3Mesh` | One mesh with vertices, faces, normals, and render modes. |
| `rdFace` | One polygon face with indices, material, flags, and extra light. |
| `rdModel3HNode` | One hierarchy node binding a mesh into the model tree. |

See [`Libs/rdroid/types.h`](../../Libs/rdroid/types.h).

## Practical Authoring Notes

- Put standard models under `3do\`.
- Put optional higher-definition replacements under `hi3do\` with the same filename.
- Keep hierarchy indices consistent, especially parent/child/sibling references.
- Use `-1` for missing hierarchy links.
- Keep vertex, texture-vertex, and face indices within range to avoid silent clamping to `0`.
- Keep section order in the stock layout shown above.

## Error Handling Summary

Current behavior on malformed data:

- file open failure: load fails
- malformed header or section syntax: load fails
- bad vertex, face, or hierarchy counts beyond engine limits: load fails
- missing referenced material: load fails
- allocation failure: load fails
- out-of-range face indices: logged and clamped to `0`, then loading continues

## References

- [`rdModel3_LoadEntry()`](../../Libs/rdroid/Primitives/rdModel3.c)
- [`sithModel_Load()`](../../Libs/sith/World/sithModel.c)
- [`rdModel3`](../../Libs/rdroid/types.h)

