## Smush Assembly Routines: Original-Engine Test Vectors

These files hold inputs and the complete results of INSANE's MASM procedures,
captured by executing them inside the retail `Indy3D.exe`. They are consumed by
`TestOriginal*` in `Libs/smush/Tests/SmushAsmTest.c`, which runs the C
translations of the same procedures on the same inputs and requires every byte
of every buffer to match. The tests read only these files; they never launch or
depend on `Indy3D.exe` or game assets.

### Reference

| Item | Value |
| --- | --- |
| Executable | Retail v1.0 `Indy3D.exe` |
| SHA-256 | `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` (verified before capture) |
| Calling convention | `__cdecl`, prototypes from `Libs/smush/RTI/symbols.h`; every entry point's `push ebp; mov ebp,esp` prologue is checked before the first call |

| Fixture | Procedures (v1.0 address) | Cases |
| --- | --- | --- |
| `Flobject/Bloc16Asm.tv` | `dbloc16_Interpolate` (`0x004EDEF0`), `dbloc16_Codec2` (`0x004EDF70`, and through it the private `Block8`/`Block4`/`Block2` at `0x004EE02C`/`0x004EE3C4`/`0x004EE58C`) | 16 + 122 |
| `Decode/DecodeAsm.tv` | `decode_Lut8` (`0x004F0590`), `decode_Lut16` (`0x004F0632`), `decode_RGBAto555` (`0x004F06E8`), `decode_555toRGBA` (`0x004F0762`), `decode_RGBAto565` (`0x004F07E6`), `decode_565toRGBA` (`0x004F0860`), `decode_RGBAto565B` (`0x004F08E4`), `decode_565BtoRGBA` (`0x004F0951`) | 269 |
| `Sound/MixerAsm.tv` | `mixer_MemCopy` (`0x004EC560`), `mixer_MemFill` (`0x004EC598`), `mixer_Lut16` (`0x004EC5F7`), `mixer_Lut16Add` (`0x004EC62C`), `mixer_Fill16` (`0x004EC664`), `mixer_Lut8` (`0x004EC67F`), `mixer_Lut8Add` (`0x004EC6AD`), `mixer_FillB` (`0x004EC6DD`) | 346 |
| `Bitmap/BitmapAsm.tv` | `blit_Masm` (`0x004F2220`), `blit_MemCopy` (`0x004F04D0`), `blit_MemCopyB` (`0x004F04F8`) | 320 |

### File Format

All integers are little endian.

```text
file:    char magic[4] = "SATV", u32 version = 1, u32 numCases, case[numCases]
case:    u8 nameLength, char name[nameLength], u32 procedure,
         u32 numBuffers, buffer[numBuffers], u32 numArgs, arg[numArgs]
buffer:  u32 size, u8 initKind, init parameters, u32 numPatches, patch[numPatches],
         u8 outKind, result
patch:   u32 offset, u32 length, u8 bytes[length]   (applied after the init)
arg:     u8 kind; 0: u32 value; 1: u32 bufferIndex, i32 byteOffset (a pointer into that buffer)
```

Procedure ids: 1 `dbloc16_Interpolate`, 2 `dbloc16_Codec2`, 10-17 `decode_Lut8`,
`Lut16`, `RGBAto555`, `555toRGBA`, `RGBAto565`, `565toRGBA`, `RGBAto565B`,
`565BtoRGBA`, 20-27 `mixer_MemCopy`, `MemFill`, `Lut16`, `Lut16Add`, `Fill16`,
`Lut8`, `Lut8Add`, `FillB`, 30-32 `blit_Masm`, `blit_MemCopy`, `blit_MemCopyB`.

Buffer contents are generated, so that large inputs cost a few bytes. The
generator is `s = s * 1664525 + 1013904223` starting from the seed, returning the
new `s`:

| initKind | Parameters | Contents |
| --- | --- | --- |
| 0 | `u8 fill` | every byte `fill` |
| 1 | `u32 seed` | byte `i` = `lcg() >> 24` |
| 2 | `u8 bytes[size]` | stored (opcode streams, crafted inputs) |
| 3 | `u32 seed, u32 mod, i32 bias` | word `i` = `(u16)((lcg() >> 8) % mod + bias)` |
| 4 | `u8 elemSize, u32 start` | element `i` = `start + i`, 1, 2 or 4 bytes |
| 5 | `u32 seed, u32 mod, u32 entrySize, u32 countOffset, u32 maxCount` | glyph table: kind 3 words with bias 0, then with the same generator for every entry `e` the two bytes at `e * entrySize + countOffset` = `(lcg() >> 8) % (maxCount + 1)` |

| outKind | Result |
| --- | --- |
| 0 | the buffer is unchanged |
| 1 | `u32 start, u32 length, u8 bytes[length]`: the changed range; bytes outside it are unchanged |
| 2 | `u64` FNV-1a hash of the whole buffer (results whose changed range exceeds 2048 bytes) |

The tests surround every buffer with 64 guard bytes and also require those to
stay untouched.

### Cases

- `dbloc16_Codec2`: crafted streams that put every opcode at every block level
  (8x8: `00-F4`, `F5`, `F6`, `F7`, `F8`, `F9-FC`, `FD`, `FE`, `FF`; 4x4 under
  `FF`; 2x2 under `FF FF`, including the 2x2 literal `FF`/`F8` and palette `F7`
  forms) with separate previous/delta frames and with the previous (or delta)
  frame equal to the current one, where the copies overlap their source and the
  order of the original's row-pair copies matters; grammar-random streams over 16
  sizes (8x8 to 320x16, sizes that are not multiples of 8, 1x1, 0x8, 8x0,
  2040x8, 2048x8, 2056x8, 8x2056) in four frame layouts (negative, zero and
  positive frame offsets); raw random byte streams with any `F5` vector. Glyph
  tables are kind 5 tables with offsets inside the block's 8 lines and counts up
  to 16/64; entries 0-3 are patched to counts 0/0, 0/max, max/0 and 1/max-1 and
  each crafted stream's first four glyphs use them.
- `dbloc16_Interpolate` (no caller in the original): strides 0-320 and 0-16 rows,
  including odd strides and a row count above 16 bits.
- `decode_Lut8`/`decode_Lut16`: destination sizes 1-4, unsupported sizes (0, 5)
  and sizes whose low byte selects the path (`0x101`, `0x204`, `0xFFFFFF03`),
  counts -2 to 17, every source value (256 bytes, 65536 words), unaligned sources.
- Pixel converters: `n` = 0-7 and 31 with random data, every 16-bit value
  (`*toRGBA`, n = 65536 and 65535), 65536 and 4097 random RGBA pixels, extreme
  RGBA values, and in-place `RGBAto*` conversions.
- `mixer_MemCopy`/`blit_MemCopy`: sizes 0-17, 100, 1001, 4099, aligned and
  unaligned separate buffers, forward and backward overlaps by 1-5 bytes and
  identical buffers.
- `mixer_MemFill`/`blit_MemCopyB`: sizes around the 20000-byte runs (19999-20004,
  40000-40003, 45678, 60005), small sizes, values with upper bits set.
- `mixer_Lut16`/`Lut16Add`/`Lut8`/`Lut8Add`: 1-1000 samples, table indices 0-3,
  every source byte, in place and overlapping; `mixer_Fill16`: 0-100 words;
  `mixer_FillB`: sizes 0-12 and 100-103.
- `blit_Masm`: widths 0-64 with odd, contiguous source and/or destination
  pitches, heights 0, 1, 2, -1, -5, row offsets above 16 bits, pitches and rows
  above 16 bits (truncated), negative rows and columns, and in-place scrolls in
  every direction by 1-5 bytes.

### Quirks Preserved

- `dbloc16_Codec2` counts blocks in bytes: a width or height of 0 (or 2041-2048)
  decodes 256 blocks, sizes above 2048 wrap. Rows of blocks advance by their
  blocks plus 7 lines, so a width that is not a multiple of 8 shifts every row of
  blocks. Glyph counts of 0 still draw the first offset.
- `dbloc16_Interpolate` indexes its first pass from `pTable + (rows & 0xFFFF0000)`
  (the upper word of the row count is left in the index register).
- `decode_Lut8`/`decode_Lut16` with 3-byte destinations keep the previous tripled
  index's bits above the low byte (word) in the next index, and store 4 bytes per
  pixel. Only sources below 86 (`0x5556`) index the table as intended; the corpus
  uses such sources plus one short sequence each that walks the stale bits.
- The odd-first-then-pairs converters and the 2-byte `decode_Lut*` path convert
  at least one pair: `n` = 0 converts 2 pixels, `n` = 1 converts 3.
- `blit_Masm` multiplies the low words of row and pitch and keeps the row's upper
  word (`(y & 0xFFFF0000) | (u16)y * (u16)pitch`); it copies at least one row.
- `mixer_FillB` never writes the `n & 3` trailing bytes.
- Overlapping copies follow the string instructions element by element
  (`mixer_MemCopy`: words and the odd byte unless `n` is a multiple of 4).

### Inputs Kept Out Of The Corpus

- Negative counts where the original shifts them unsigned (`n >> 1` in the
  converters and the 2-byte `decode_Lut*` path, `n >> 2` in the copies and fills)
  and `n` = 0 in the `loop`-based `mixer_Lut*` procedures run for 2^31 or 2^32
  iterations.
- Negative row counts in `dbloc16_Interpolate` index the table 4 GB away.
- 3-byte `decode_Lut*` sources whose stale index bits leave the table.
- Every case was executed twice inside the capture with 4096 guard bytes around
  every buffer, `0x00` in one run and `0xFF` in the other. Cases that faulted,
  wrote a guard byte or produced different results in the two runs were dropped.
  None of the cases listed here were dropped.

### Provenance

- Capture harness: a temporary C file built with MSVC (x86, static CRT) as
  `Jones3D.dll` and kept outside the repository (SHA-256 of the source used:
  `88abe7ed457177fe9eed733b18dc02d0dff5b0c3d0c2bdcad42f2f3a617c8b1f`).
  It contains no hooks. `Jones3D.exe` (this repository's launcher) starts
  `Indy3D.exe` suspended and injects the DLL from its own directory; `DllMain`
  calls the original procedures at the addresses above on the generated inputs,
  writes the fixture files and a helper thread terminates the process.
  The private block procedures are reached through `dbloc16_Codec2`.
- Regeneration: build the harness with `cl /MT /LD capture.c /Fe:Jones3D.dll`,
  place it next to `Jones3D.exe` and a copy of `Indy3D.exe` with the SHA-256
  above, run `Jones3D.exe Indy3D.exe`, and collect the files from `out\`.
  Review regenerated fixtures as behavior changes.
- Environment: MSVC 2026 x86 compiler under Wine. No locale, time or
  floating-point state is involved; the generators use integer arithmetic only.
- Two complete runs in fresh processes produced byte-identical files.
- The C translations reproduce every case in Debug and Release for both
  DirectX 6 and DirectX 9 builds.

### SHA-256

```text
9b5e26ad3569a73855ceaffce861bf0029bc9df936335bcd17bc271e945858c6  Bitmap/BitmapAsm.tv
ead2c40c7cce3b26eca60b5f43a499a8dd6efa6c37a400ff078e581cf555e40e  Decode/DecodeAsm.tv
fdeadd2d6d36d71877f76a1d7229866fe1e75cc27605b50922b3d09654c428c6  Flobject/Bloc16Asm.tv
f0c03dc6d84525b517efb1c3f9195d68ecf577f502c2ea443ab9c7f29785b374  Sound/MixerAsm.tv
```
