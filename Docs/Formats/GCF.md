# GCF File Format

This document describes the binary `.gcf` font-metrics format used by OpenJones3D.

In the current codebase, `.gcf` files are loaded by [`rdFont_LoadEntry()`](../../Libs/rdroid/Primitives/rdFont.c) and paired with a same-base `.mat` atlas through [`rdFont_Load()`](../../Libs/rdroid/Primitives/rdFont.c).

## Purpose

A `.gcf` file stores the metrics half of a bitmap font:

- line spacing
- font name
- nominal font size
- glyph UV and baseline metrics for `256` character slots

The glyph images themselves are not stored in the `.gcf` file. They live in the paired `.mat` texture atlas.

## Required MAT Pair

`.gcf` is not a standalone font resource in normal use.

The font loader accepts either:

- a `.gcf` filename, or
- a `.mat` filename

and derives the other file by replacing the extension with the same basename.

Example:

```text
mat\jonesCalisto MT20.gcf
mat\jonesCalisto MT20.mat
```

That pairing logic is implemented in [`rdFont_Load()`](../../Libs/rdroid/Primitives/rdFont.c).

See [MAT](MAT.md).

## Where It Is Used

The current voice/caption system loads:

```text
mat\jonesCalisto MT20.gcf
```

through [`sithVoice.c`](../../Libs/sith/World/sithVoice.c).

Other font assets may also live in different folders, such as `misc\ui\`, as long as the paired `.gcf` and `.mat` files share the same basename and accessible path.

## Binary Layout

The on-disk layout is:

```text
int32 lineSpacing
char  name[256]
int32 fontSize
rdGlyphMetrics glyphs[256]
```

The current loader and writer hardcode:

- `RDFONT_MAXNAMELEN = 256`
- `RDFONT_MAXGLYPHS = 256`

## Header Fields

| Field | Meaning |
| --- | --- |
| `lineSpacing` | Default line advance for the font. |
| `name[256]` | Fixed-size font name buffer. |
| `fontSize` | Nominal point-size-like scale used by the runtime. |
| `glyphs[256]` | Metrics for character codes `0..255`. |

Important note about the name field:

- the loader reads the full 256-byte buffer verbatim
- real files may leave trailing bytes uninitialized or non-zero after the first NUL terminator
- tools should therefore treat it as a fixed-size buffer whose visible name ends at the first `\0`

## Glyph Metric Layout

Each glyph record is one [`rdGlyphMetrics`](../../Libs/rdroid/types.h):

| Field | Meaning in the current renderer |
| --- | --- |
| `left` | Left U coordinate in the atlas. |
| `top` | Top V coordinate in the atlas. |
| `right` | Right U coordinate in the atlas. |
| `bottom` | Bottom V coordinate in the atlas. |
| `baselineOriginY` | Vertical baseline offset used when placing the glyph quad. |
| `baselineOriginX` | Horizontal advance used when measuring and laying out text. |

Runtime use in [`rdFont_DrawCharScaled()`](../../Libs/rdroid/Primitives/rdFont.c):

- glyph width is `right - left`
- glyph height is `bottom - top`
- atlas coordinates are used directly as UVs on the font material
- `baselineOriginX` advances the text cursor
- `baselineOriginY` shifts the glyph vertically relative to the text baseline

## Special Runtime Cases

The current text renderer has two notable special cases:

- if a glyph's `baselineOriginY == -fontSize`, the engine treats that glyph as missing and substitutes `?`
- if a glyph's `baselineOriginY == -1`, the renderer skips drawing that glyph directly

Those behaviors come from the current runtime text functions in [`rdFont.c`](../../Libs/rdroid/Primitives/rdFont.c).

## In-Memory Layout

Important runtime fields:

| Structure | Field | Meaning |
| --- | --- | --- |
| `rdFont` | `lineSpacing` | Default line spacing. |
| `rdFont` | `pName` | Heap copy of the 256-byte name buffer. |
| `rdFont` | `fontSize` | Base font size used for scaling. |
| `rdFont` | `aGlyphs` | Array of 256 glyph metrics. |
| `rdFont` | `pMaterial` | Paired material atlas loaded from the same-base `.mat` file. |

## Practical Authoring Notes

- Always provide a same-base `.mat` alongside the `.gcf`.
- Keep exactly `256` glyph records.
- Treat `baselineOriginX` as the horizontal advance field.
- Treat `left/top/right/bottom` as atlas UV coordinates, not pixel coordinates.

## Error Handling Summary

Current behavior on malformed data:

- unsupported extension passed to `rdFont_Load()`: load fails
- `.gcf` open failure: load fails
- `.mat` open or load failure: whole font load fails
- truncated `.gcf` header or glyph table: load fails
- allocation failure for glyphs or name buffer: load fails

## References

- [`rdFont_Load()`](../../Libs/rdroid/Primitives/rdFont.c)
- [`rdFont_LoadEntry()`](../../Libs/rdroid/Primitives/rdFont.c)
- [`rdGlyphMetrics`](../../Libs/rdroid/types.h)
- [`sithVoice.c`](../../Libs/sith/World/sithVoice.c)

