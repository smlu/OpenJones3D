# MAT File Format

This document describes the binary `.mat` material-texture format used by OpenJones3D.

In the current codebase, `.mat` files are loaded by [`rdMaterial_LoadEntry()`](../../Libs/rdroid/Engine/rdMaterial.c) and wrapped by the gameplay-side loader [`sithMaterial_Load()`](../../Libs/sith/World/sithMaterial.c).

## Purpose

A `.mat` file stores one material resource:

- header metadata
- the stored color-format description
- per-cel metadata
- per-cel texture headers
- raw mipmap pixel data for each cel

At runtime, the parsed file becomes one [`rdMaterial`](../../Libs/rdroid/types.h).

## Where It Is Used

`.mat` files are used by many other resources:

- `3DO` model material lists
- `SPR` sprite definitions
- `GCF` font atlases
- world material lists and embedded/external world materials

The normal gameplay loader resolves filenames under:

```text
mat\<filename>
```

That path is constructed in [`sithMaterial_Load()`](../../Libs/sith/World/sithMaterial.c).

## High-Level Structure

The binary layout is:

```text
rdMatHeader
repeat numCels times:
    rdMatCelInfo
    [rdMatTextureCelInfo if (type & 0x8) != 0]
repeat numCels times:
    rdMatTextureHeader
    raw mip level 0 pixels
    raw mip level 1 pixels
    ...
```

## Header

The file begins with [`rdMatHeader`](../../Libs/rdroid/types.h):

| Field | Meaning |
| --- | --- |
| `magic[4]` | Must be `"MAT "`. |
| `version` | Must be `50` in the current loader. |
| `type` | Material type. The current writer emits `2` (`RDMAT_TYPE_TEXTURE`). |
| `numCels` | Number of material cels. |
| `numTextures` | Number of texture records. The current loader requires this to equal `numCels`. |
| `colorInfo` | Stored renderer color-format description used to interpret the pixel data. |

Important validation rules:

- bad magic rejects the file
- bad version rejects the file
- `numCels != numTextures` rejects the file as an old-style material

## Per-Cel Metadata

After the header, the loader reads one [`rdMatCelInfo`](../../Libs/rdroid/types.h) per cel:

| Field | Meaning in the current code |
| --- | --- |
| `type` | Cel-type flags. If bit `0x8` is set, an additional `rdMatTextureCelInfo` record is read. |
| `colorIdx` | Stored but not interpreted by the current loader. |
| `unknown1` | Stored but not interpreted by the current loader. |
| `unknown2` | Stored but not interpreted by the current loader. |
| `unknown3` | Stored but not interpreted by the current loader. |
| `unknown4` | Stored but not interpreted by the current loader. |

If `type & 0x8` is non-zero, the loader also reads one [`rdMatTextureCelInfo`](../../Libs/rdroid/types.h) for that cel.

The current writer emits:

- `celInfo.type = 0x8`
- `celInfo.unknown4 = 1.0`
- `texcelInfo.celNum = i`
- all remaining texture-cel fields zeroed

## Per-Cel Texture Headers

Each cel then has one [`rdMatTextureHeader`](../../Libs/rdroid/types.h):

| Field | Meaning |
| --- | --- |
| `width` | Base mip width. |
| `height` | Base mip height. |
| `transparentBool` | Stored field, not interpreted by the current loader logic. |
| `unknown1` | Stored field, not interpreted by the current loader logic. |
| `unknown2` | Stored field, not interpreted by the current loader logic. |
| `numMipLevels` | Number of raw mip levels stored after this header. |

## Pixel Data

For each cel, the loader reads `numMipLevels` raw raster buffers immediately after the texture header.

Important implementation details:

- the pixel data is read using the dimensions and color format from the current mip level
- after each mip level, width and height are halved for the next level
- the raw data is read exactly as stored and then optionally converted to the renderer's desired runtime texture format

Because of that conversion step:

- the on-disk format may differ from the runtime texture format
- for example, older 1-bit-alpha formats may be converted to 32-bit runtime textures in the modern renderer

## In-Memory Layout

Important fields filled by the loader:

| Structure | Field | Meaning |
| --- | --- | --- |
| `rdMaterial` | `aName[64]` | Material name, usually copied from the filename basename. |
| `rdMaterial` | `formatType` | Runtime texture format chosen after load/conversion. |
| `rdMaterial` | `width`, `height` | Width and height of the most recently read cel header. In normal stock assets, cels typically share the same size. |
| `rdMaterial` | `numCels` | Number of cels. |
| `rdMaterial` | `aTextures` | Runtime texture handles for all cels. |

## Practical Authoring Notes

- Put material files under `mat\`.
- Use the `.mat` extension in resource references.
- Keep `numCels` and `numTextures` identical.
- If you write tools for this format, emit the extra texture-cel record whenever `celInfo.type` has bit `0x8` set.
- Font atlases use this same format; see [GCF](GCF.md).

## Error Handling Summary

Current behavior on malformed data:

- bad file open: load fails
- bad magic or version: load fails
- old-style material with `numCels != numTextures`: load fails
- truncated cel info, texture headers, or pixel data: load fails
- allocation failure: load fails

## References

- [`rdMaterial_LoadEntry()`](../../Libs/rdroid/Engine/rdMaterial.c)
- [`rdMaterial_Write()`](../../Libs/rdroid/Engine/rdMaterial.c)
- [`sithMaterial_Load()`](../../Libs/sith/World/sithMaterial.c)
- [`rdMaterial`](../../Libs/rdroid/types.h)

