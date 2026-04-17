# PAR File Format

This document describes the text `.par` particle format used by OpenJones3D.

In the current codebase, `.par` files are loaded through [`sithParticle_Load()`](../../Libs/sith/Engine/sithParticle.c#L69), which resolves the resource path and then calls [`rdParticle_LoadEntry()`](../../Libs/rdroid/Primitives/rdParticle.c#L172) for the actual file parse.

## Purpose

A `.par` file defines a reusable particle geometry resource:

- billboard size
- material name
- lighting mode
- bounding radius
- insertion offset
- vertex positions
- material cel index for each particle vertex

At runtime, the parsed file becomes one [`rdParticle`](../../Libs/rdroid/types.h#L460) instance.

## Where It Is Used

`.par` files are used when a thing definition references a particle resource. In the current tree, that happens through the `particle` thing property handled in [`sithThing.c`](../../Libs/sith/World/sithThing.c#L3098).

## File Location And Naming

The normal loader takes a filename, not a full path, and resolves it under:

```text
misc\par\<filename>
```

That path is constructed in [`sithParticle_Load()`](../../Libs/sith/Engine/sithParticle.c#L89).

Important notes:

- The filename should include the `.par` extension.
- The particle cache is keyed by that filename string.
- Loading the same filename again in the same world returns the cached particle instead of reparsing the file.

## Related Data Loaded By PAR Entries

The `MATERIAL` field is passed to [`rdMaterial_Load()`](../../Libs/rdroid/Engine/rdMaterial.c#L43). During normal gameplay that loader is registered to [`sithMaterial_Load()`](../../Libs/sith/Engine/sithRender.c#L193), so material names are normally resolved from:

```text
mat\<materialName>
```

## Parser Rules

`.par` files use the generic [`stdConffile`](../../Libs/std/General/stdConffile.c) line reader together with [`stdConffile_ScanLine()`](../../Libs/std/General/stdConffile.h#L28)-style pattern parsing, so the following behavior comes from that shared implementation:

| Behavior | Current implementation |
| --- | --- |
| Case sensitivity | Entire lines are lowercased before matching, so the format is effectively case-insensitive. |
| Whole-line comments | Lines starting with `#` or `;` are skipped. |
| Inline comments | Inline `#` comments are stripped before pattern matching. |
| Blank lines | Ignored automatically. |
| Numeric parsing | The file uses decimal integers and floats. |

## In-Memory Layout

A parsed `.par` file fills these fields:

| Structure | Field | Meaning |
| --- | --- | --- |
| `rdParticle` | `aName[64]` | Resource name copied from the filename portion of the load path. |
| `rdParticle` | `lightningMode` | Render light mode used when drawing the particle quads. |
| `rdParticle` | `numVertices` | Number of particle vertices stored in the file. |
| `rdParticle` | `aVerticies` | Per-particle local-space positions. |
| `rdParticle` | `aVertMatCelNums` | Material cel index for each particle vertex. |
| `rdParticle` | `aExtraLights` | Extra-light array allocated during load, but not populated from the file. |
| `rdParticle` | `size` | Width and height of each billboard quad. |
| `rdParticle` | `sizeHalf` | Cached half-size derived from `size`. |
| `rdParticle` | `pMaterial` | Loaded material resource. |
| `rdParticle` | `radius` | Bounding radius. |
| `rdParticle` | `insertOffset` | Placement offset used when spawning the particle thing. |

Reference:

- [`rdParticle`](../../Libs/rdroid/types.h#L460)

## File Layout

The loader expects two sections in this order:

1. a header section
2. a geometry definition section

A simplified grammar looks like this:

```text
SECTION: HEADER
PAR 1.0
SIZE <size>
MATERIAL <materialName>
LIGHTINGMODE <lightMode>

SECTION: GEOMETRYDEF
RADIUS <radius>
INSERT OFFSET <x> <y> <z>
VERTICES <count>
<index>: <x> <y> <z> <cel>
...
```

## Header Section

The first section must contain these lines:

```text
SECTION: HEADER
PAR 1.0
SIZE <float>
MATERIAL <name>
LIGHTINGMODE <int>
```

### SECTION Line

The loader requires a syntactically valid `SECTION: <name>` line, but it does not verify that the first section name is literally `HEADER`. In practice, use `HEADER`, because that is what the writer emits in [`rdParticle_Write()`](../../Libs/rdroid/Primitives/rdParticle.c#L316).

### PAR Version

The canonical version is:

```text
PAR 1.0
```

Current behavior:

- the loader expects major version `1` and minor version `0`
- if the version line is malformed or the version differs, it logs a warning
- version mismatch does not abort the load by itself

Reference:

- [`RDPARTICLE_MAJVER`](../../Libs/rdroid/Primitives/rdParticle.c#L18)
- [`RDPARTICLE_MINVER`](../../Libs/rdroid/Primitives/rdParticle.c#L19)

### SIZE

`SIZE <float>` sets the full width and height of each rendered particle quad.

The loader also derives:

- `sizeHalf = size / 2.0`

### MATERIAL

`MATERIAL <name>` names the material resource to load.

Important notes:

- the name is passed to the registered material loader
- in gameplay builds that normally means [`sithMaterial_Load()`](../../Libs/sith/World/sithMaterial.c#L629)
- if the material cannot be loaded, the particle load fails

### LIGHTINGMODE

`LIGHTINGMODE <int>` sets [`rdLightMode`](../COG/Types-And-Modes.md#render-light-modes).

See [Render Light Modes](../COG/Types-And-Modes.md#render-light-modes).

## Geometry Definition Section

The second section must contain:

```text
SECTION: GEOMETRYDEF
RADIUS <float>
INSERT OFFSET <x> <y> <z>
VERTICES <count>
```

As with the first section, the loader checks only that the `SECTION:` line is syntactically valid. It does not verify that the second section name is literally `GEOMETRYDEF`, but that is the canonical name emitted by the writer.

### RADIUS

`RADIUS <float>` sets the particle resource's bounding radius.

### INSERT OFFSET

`INSERT OFFSET <x> <y> <z>` sets the local-space insertion offset stored in `insertOffset`.

### VERTICES

`VERTICES <count>` declares how many vertex rows follow.

Current implementation rules:

- the count must be `0` through `256`
- values above [`RDPARTICLE_MAXVERTS`](../../Libs/rdroid/Primitives/rdParticle.h#L7) fail the load
- the loader allocates `aVerticies`, `aVertMatCelNums`, and `aExtraLights` arrays sized to this count

## Vertex Rows

Each vertex row uses this form:

```text
<index>: <x> <y> <z> <cel>
```

Example:

```text
0: 0.000000 0.000000 0.000000 0
1: 0.250000 0.100000 0.000000 1
```

Column meaning:

| Column | Meaning |
| --- | --- |
| `index` | Vertex row number read from the file. |
| `x y z` | Local-space vertex position. |
| `cel` | Material cel index for that vertex. |

Important notes:

- the loader reads `index`, but does not validate that it matches the actual row number
- `cel` must be less than the loaded material's `numCels`
- if any row is malformed, the load fails

## Example

This minimal example matches the writer's canonical layout:

```text
# Example particle resource

SECTION: HEADER
PAR 1.0
SIZE 0.250000
MATERIAL dust.mat
LIGHTINGMODE 3

SECTION: GEOMETRYDEF
RADIUS 0.500000
INSERT OFFSET 0.000000 0.000000 0.000000
VERTICES 2
0: 0.000000 0.000000 0.000000 0
1: 0.200000 0.100000 0.000000 0
```

## Practical Authoring Notes

- Put the file under `misc\par\`.
- Keep the `PAR 1.0` line even though the current loader only warns on mismatch.
- Use `SECTION: HEADER` and `SECTION: GEOMETRYDEF` for compatibility with the canonical writer.
- Make sure the material exists and that each `cel` value is within that material's cel range.
- Keep vertex counts at or below `256`.

## Error Handling Summary

Current behavior on malformed data:

- file open failure: load fails
- malformed required line: load fails
- material load failure: load fails
- vertex count above `256`: load fails
- invalid vertex cel index: load fails
- unexpected EOF while reading required content: load fails

## References

- particle wrapper loader: [`sithParticle.c`](../../Libs/sith/Engine/sithParticle.c#L69)
- particle file parser and writer: [`rdParticle.c`](../../Libs/rdroid/Primitives/rdParticle.c#L172)
- particle structure: [`rdroid/types.h`](../../Libs/rdroid/types.h#L460)
- particle max vertex count: [`rdParticle.h`](../../Libs/rdroid/Primitives/rdParticle.h#L7)
- material loader registration: [`sithRender.c`](../../Libs/sith/Engine/sithRender.c#L193)
- material loader: [`sithMaterial.c`](../../Libs/sith/World/sithMaterial.c#L629)

