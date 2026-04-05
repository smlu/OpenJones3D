# SPR File Format

This document describes the text `.spr` sprite-definition format used by OpenJones3D.

In the current codebase, sprite files are loaded by [`sithSprite_Load()`](../../Libs/sith/World/sithSprite.c) and instantiated through [`rdSprite_NewEntry()`](../../Libs/rdroid/Primitives/rdSprite.c).

## Purpose

A `.spr` file defines one sprite resource:

- which material should be drawn on the sprite quad
- which sprite orientation type should be used
- the sprite width and height
- the render geometry and lighting modes
- the extra light color
- the local draw offset

At runtime, the parsed file becomes one [`rdSprite3`](../../Libs/rdroid/types.h) entry.

## Where It Is Used

`.spr` files are loaded for sprite things and sprite-based effects.

The normal sprite loader resolves filenames under:

```text
misc\spr\<filename>
```

That path is constructed in [`sithSprite_Load()`](../../Libs/sith/World/sithSprite.c).

## Missing-File Fallback

If a requested sprite file cannot be opened:

- and the requested file is not `default.spr`, the loader retries with `default.spr`
- if `default.spr` also fails, the load fails completely

This fallback is implemented in [`sithSprite_Load()`](../../Libs/sith/World/sithSprite.c).

## Related Data Loaded By SPR Entries

The first column is a material filename. It is passed to [`rdMaterial_Load()`](../../Libs/rdroid/Engine/rdMaterial.h), which in the gameplay loader resolves it under:

```text
mat\<filename>
```

See [MAT](MAT.md).

## Parser Rules

`.spr` files use the generic [`stdConffile`](../../Libs/std/General/stdConffile.c) text parser.

| Behavior | Current implementation |
| --- | --- |
| Case sensitivity | Entire non-comment lines are lowercased before tokenization, so the format is effectively case-insensitive. This also lowercases material filenames before loading. |
| Whole-line comments | Lines starting with `#` or `;` are skipped. |
| Inline comments | Inline `#` comments are stripped. |
| Separators | Tokens are separated by commas, spaces, and tabs. |
| Empty lines | Skipped automatically. |
| End marker | Individual `.spr` files are read until the first non-empty data line is successfully parsed. There is no file-local `end` marker. |

## File Layout

After any comments, the loader expects exactly one data row with `14` parsed arguments:

```text
<material> <type> <width> <height> <geometryMode> <lightMode> <textureMode> <extraR> <extraG> <extraB> <extraA> <xoff> <yoff> <zoff>
```

Example from the shipped assets:

```C
#Material                                  Type Width Height Geo Light Tex Extralight(RGBA)    Xoff Yoff Zoff
aet_4sprite_mophia_morph_lens_flare.mat    0    0.5   0.5    4   3     0   1.0 1.0 1.0 1.0    0.0  0.0  0.0
```

## Column Meaning

| Position | Meaning | Required | Notes |
| --- | --- | --- | --- |
| 1 | Material filename | Yes | Loaded as a material resource. |
| 2 | Sprite type | Yes | Must currently be `0`, `1`, or `2`. |
| 3 | Width | Yes | Must be greater than `0`. |
| 4 | Height | Yes | Must be greater than `0`. |
| 5 | Geometry mode | Yes | See [Render Geometry Modes](../COG/Types-And-Modes.md#render-geometry-modes). |
| 6 | Lighting mode | Yes | See [Render Light Modes](../COG/Types-And-Modes.md#render-light-modes). |
| 7 | Texture mode | Yes | Parsed, but ignored by the current loader. |
| 8 | Extra-light red | Yes | Stored in `face.extraLight.red`. |
| 9 | Extra-light green | Yes | Stored in `face.extraLight.green`. |
| 10 | Extra-light blue | Yes | Stored in `face.extraLight.blue`. |
| 11 | Extra-light alpha | Yes | Stored in `face.extraLight.alpha`. |
| 12 | Local X offset | Yes | Stored in `offset.x`. |
| 13 | Local Y offset | Yes | Stored in `offset.y`. |
| 14 | Local Z offset | Yes | Stored in `offset.z`. |

## Sprite Type Values

The current parser accepts types up to `2`, but the draw code only has explicit behavior for the following values:

| Type | Meaning in the current renderer |
| --- | --- |
| `0` | Camera-facing billboard. The quad is built in view-facing space, then positioned at the thing location plus the sprite offset. |
| `2` | Oriented billboard. The sprite uses the thing orientation as its local facing basis and builds a quad around that axis. |

Notes:

- `type = 1` is accepted by the loader because the range check only rejects values greater than `2`.
- the current draw code does not have a dedicated `type == 1` branch
- for authored content, `0` and `2` are the safe values to use

## Runtime Defaults Applied By The Loader

After parsing, [`rdSprite_NewEntry()`](../../Libs/rdroid/Primitives/rdSprite.c) applies these defaults:

- `face.flags = RD_FF_FOG_ENABLED | RD_FF_DOUBLE_SIDED`
- `face.numVertices = 4`
- if the geometry mode is textured, texture coordinates are auto-generated as the full `[0,1] x [0,1]` quad
- `widthHalf`, `heightHalf`, and `radius` are derived from the width and height

## In-Memory Layout

Important fields filled from the file:

| Structure | Field | Meaning |
| --- | --- | --- |
| `rdSprite3` | `aName[64]` | Sprite filename passed to the loader. |
| `rdSprite3` | `type` | Sprite orientation type. |
| `rdSprite3` | `width`, `height` | Sprite size. |
| `rdSprite3` | `widthHalf`, `heightHalf` | Cached half-size values. |
| `rdSprite3` | `radius` | Bounding radius derived from width and height. |
| `rdSprite3` | `offset` | Local draw offset. |
| `rdSprite3` | `face.pMaterial` | Loaded material pointer. |
| `rdSprite3` | `face.geometryMode` | Geometry mode. |
| `rdSprite3` | `face.lightingMode` | Lighting mode. |
| `rdSprite3` | `face.extraLight` | Extra-light RGBA vector. |

## Practical Authoring Notes

- Put the file under `misc\spr\`.
- Include the `.spr` extension in resource references.
- Use a material that exists under `mat\`.
- Write the texture-mode column even though the current loader ignores it, because stock files still include it.
- Prefer `type = 0` for ordinary camera-facing effects and `type = 2` for orientation-aware sprites such as ripples or directional effects.

## Error Handling Summary

Current behavior on malformed data:

- file open failure: falls back to `default.spr` when possible
- wrong number of parsed arguments: load fails
- `type > 2`: load fails
- `width <= 0` or `height <= 0`: load fails
- material load failure: load fails
- allocation failure while building the runtime quad: load fails

## References

- [`sithSprite_Load()`](../../Libs/sith/World/sithSprite.c)
- [`rdSprite_NewEntry()`](../../Libs/rdroid/Primitives/rdSprite.c)
- [`rdSprite_Draw()`](../../Libs/rdroid/Primitives/rdSprite.c)

