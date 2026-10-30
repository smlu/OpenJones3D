## AudioLib Original-Engine Test Vectors

These files hold inputs and the complete outputs of the original `AudioLib`
functions, captured by executing them inside the retail `Indy3D.exe`. They are
consumed by `TestOriginal*` in `Libs/sound/Tests/AudioLibTest.c`. The tests
read only these files; they never launch or depend on `Indy3D.exe` or game assets.

### Reference

| Item | Value |
| --- | --- |
| Executable | Retail v1.0 `Indy3D.exe` |
| SHA-256 | `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` (verified before capture) |
| Debug build | Not captured. Only the v1.0 entry points below were resolved. |
| Calling convention | `__cdecl` (`J3DAPI`), prototypes from `Libs/sound/RTI/symbols.h` |
| Layout check | `sizeof(tAudioCompressorState) == 6` (`int8_t aStepIndex[2]`, `int16_t aPrevSample[2]`), asserted in the harness and in `types.h` |

| Function | v1.0 address | Fixture |
| --- | --- | --- |
| `AudioLib_ParseWaveFileHeader` | `0x004DF490` | `ParseWaveFileHeader.tv` |
| `AudioLib_Compress` (ADPCM streams) | `0x004DF930` | `Compress.tv` |
| `AudioLib_Compress` (WVSM streams) | `0x004DF930` | `CompressWVSM.tv` |
| `AudioLib_ResetCompressor` | `0x004DFA20` | `ResetCompressor.tv` |
| `AudioLib_Uncompress` | `0x004DFA40` | `Uncompress.tv` |
| `AudioLib_CompressBlock` | `0x004E0000` | `CompressBlock.tv` |
| `AudioLib_UncompressBlock` | `0x004E02F0` | `UncompressBlock.tv` |
| `AudioLib_WVSMCompressBlock` | `0x004E0590` | used only to build `WVSMUncompressBlock.tv` inputs |
| `AudioLib_WVSMUncompressBlock` | `0x004E08F0` | `WVSMUncompressBlock.tv` |

### File Format

All integers are little endian. Every file starts with `"AVTV"`, a `u32` kind
(1 parse, 2 CompressBlock, 3 UncompressBlock, 4 Compress, 5 Uncompress, 6
WVSMUncompressBlock, 7 ResetCompressor) and a `u32` case count. A case starts with
a `u8` name length and the case name. A blob is a `u32` length followed by the
bytes. Compressor state is the six raw bytes of `tAudioCompressorState`.

| Kind | Case fields after the name |
| --- | --- |
| 1 | `u32 flags` (bit 0: `pData` is NULL, bit n+1: output pointer n was passed; outputs are type, sample rate, bits per sample, channels, extra info, sound data size, extra data offset, sound data offset), input blob, `i32` returned offset from the input start (-1 for NULL), `u32 out[8]` after the call. Outputs start as `0xCDCDCDCD`. The extra data offset is stored as an offset from the input start because the original stores an address. |
| 2 | `u32` channels, samples, `bNativeByteOrder`, `bStateInitialized`, chained; input state; PCM blob; `i32` return value; output blob; output state |
| 3 | `u32` channels, samples, `bStateInitialized`, chained; input state; compressed blob; PCM output blob; output state |
| 4 | `u32` channel argument, size, chained; input state; PCM blob; `i32` return value; written bytes blob; output state |
| 5 | `u32` output size, chained (unused); input state; compressed blob; written bytes blob; output state |
| 6 | `u32` block size; compressed blob; `i32` consumed bytes; written bytes blob |
| 7 | input state, output state |

Inputs are followed by 64 zero bytes in the harness and in the tests, and output
buffers are prefilled with `0xA5`. Output blobs hold exactly the bytes the
original wrote; the tests also require every following byte to stay `0xA5`.
Chained cases use the output state of the previous case as input state.

### Cases

Cases are deterministic. PCM comes from integer-only generators; noise uses
`s = s * 1664525 + 1013904223` (24 bits of output) seeded with
`seed * 2654435761 + 12345` for PCM, `+ 777` for random byte streams and `+ 999`
for container filler bytes. Seeds are part of the case names or fixed in the harness.

The first part of every file holds the original corpus; the edge cases captured later
follow it (names containing `edge_` or starting with `trunc_`), so the original
cases kept their order and bytes.

- `ParseWaveFileHeader.tv` (42 + 261 cases): NULL input, RIFF WAVE (mono/stereo, 8/16
  bit, empty data, 18 byte `fmt `, skipped `LIST` chunk, `sync` chunk with and
  without each output pointer, no outputs, single outputs), well-formed files
  that are rejected (non-PCM format tag, no `data` chunk, missing `WAVEfmt `),
  INDYWV (with and without extra data, with and without output pointers), iMUS,
  AIFF (44100, 22050, 11025 Hz exponents, an unlisted exponent, extra chunk,
  empty data, wrong `COMM`), `MCMP` wrapped WAV/iMUS/INDYWV/AIFF (AIFF is
  rejected after `MCMP`) and an unknown signature.
  Edge cases: an empty non-NULL input; RIFF sizes ending exactly at and one byte
  past the size field of the `data` chunk, zero and very large; channel counts 0, 6,
  `0x7FFF`, `0x8000` and `0xFFFF` and bit depths `0x8000`/`0xFFFF` (read as signed
  16-bit values and sign extended), sample rates 0, 1 and `0xFFFFFFFF`, bit depths
  0, 4, 24 and 32; format tags 0, 2, `0x0101`, `0x8001` and `0xFFFE` (rejected); `fmt `
  sizes 17 and 40, an odd `LIST` chunk without the RIFF pad byte, empty chunks, a
  `sync` chunk of size 0, two `sync` chunks (the last one wins), `sync` after `data`
  (ignored) and without `data`, an uppercase `DATA` chunk (skipped), a `data` size
  of `0xFFFFFFFF`, `WAVEFMT `, `RIFX` and `riff`; INDYWV with all fields 0 and all
  fields `0xFFFFFFFF`, extra data of 1 and 255 bytes, each extra data output alone,
  a very large extra size without the extra outputs and a lowercase signature; iMUS
  `MAP ` sizes 0 to 52 (the sound data size read and the returned pointer move
  across the header), zero and maximum format fields, a lowercase `frmt`; AIFF rate
  exponent bytes 0, 13 and 255 with other sign/exponent high bytes, channel and bit
  fields whose high byte is ignored, `COMM` size 22, an odd chunk without pad byte,
  `SSND` sizes 0, 7, 8 and `0xFFFFFFFF` (the size minus 8 wraps), FORM sizes ending
  at and one past the `SSND` size field and zero, `AIFC` and lowercase `ssnd`
  (rejected); `MCMP` with 0, 1 and 3 index entries, empty and 5 byte stored blocks,
  INDYWV and iMUS contents, a non-PCM WAV, nested `MCMP` and unknown contents.
  Truncation sweeps (`trunc_<container>_<length>`): every prefix of one WAV (with
  `sync` chunk), INDYWV, iMUS, AIFF and `MCMP` WAV file; the parser has no size
  argument and reads the zero padding behind a short input. 170 of the 313 prefixes
  are kept; the others are listed below.
- `CompressBlock.tv` (57 + 34 cases): 0 to 4096 samples, mono and stereo, silence,
  square, sine (integer approximation), ramp, impulses, full-scale alternation,
  seeded noise at several amplitudes, loud-to-quiet bursts (the step index sweeps
  through every sample width), supplied and reset states, big endian input
  (`bNativeByteOrder == 0`), and four block chains with odd block sizes.
  The original's encoder uses 4, 5, 6 and 7 bit samples, escape codes and
  saturation in both directions across these cases.
  Edge cases: full-scale constants (`32767`, `-32768`, one per channel),
  alternating `+1/-1` and full-scale samples, `-32768` impulses, supplied states at
  step index 88 with the previous sample at the opposite limit of the input, step
  indices 0 and 1, a stereo state with both limits for one sample, big endian
  extremes, 2, 4097 mono and 4097 stereo samples, and chains of single samples,
  1 to 3 sample stereo blocks and empty blocks.
- `UncompressBlock.tv` (87 + 42 cases): the original encoder's blocks decoded,
  garbage input state with the reset flag, block chains, and pseudo-random bit
  streams (seeds 101 to 113) in mono and stereo, with supplied states at the
  limits of the step index and sample range.
  Edge cases: the original encoder's blocks of the native-order CompressBlock
  edge cases and chains, constant bit patterns (`0x00`, `0xFF`, `0x77`, `0x55/0xAA`)
  decoded from states at step index 0, 44 and 88 with the previous sample at either
  limit, and pseudo-random streams of 1 and 2 samples from limit states and of 4097
  samples.
- `Compress.tv` (20 + 11 cases): ADPCM streams (channel argument 3 and 4) of 0 to 8192
  bytes, supplied states, and three-call chains per channel layout.
  Edge cases: sizes 1 and 7 (mono) and 1, 2, 3 and 5 (stereo), smaller than or
  not a multiple of a sample frame, full-scale constants and states at step index
  88, and a stereo header whose first step index is 0 (header byte `0xFF`).
- `CompressWVSM.tv` (14 + 12 cases): WVSM streams (channel argument 1 and 2) of 0 to
  10000 bytes, including sizes around the 0x1000 block boundary.
  Edge cases: sizes 1, 4092, 4097 and 12288 (three full blocks), channel
  argument 0, constants `32767` and `-32767`, alternating full-scale and `+1/-1`
  samples, impulses, a silent right channel and samples within the byte range.
- `Uncompress.tv` (43 + 42 cases): the original compressor's ADPCM and WVSM streams
  decoded with a garbage input state, and hand-assembled ADPCM headers with
  pseudo-random payloads (including a header resembling the WVSM signature).
  Edge cases: every new Compress/CompressWVSM stream decoded with the full size
  and, from 1024 bytes on, with a third of it; headers at step index 88 with the
  previous sample at both limits, output sizes 0 to 5 bytes for mono and stereo, a
  WVSM stream with a lowercase signature tag (decoded as ADPCM) and a WVSM stream with
  one short block decoded with output sizes 0, 1 and 6.
- `ResetCompressor.tv` (3 + 2 cases): garbage, extreme and zero states; edge cases
  with the type limits (`-128`, `127`, `-32768`, `32767`) and all bits set.
- `WVSMUncompressBlock.tv` (22 + 14 cases): the original encoder's blocks for several
  signals and sizes (4 to 4096 bytes), hand-assembled blocks with `0x80` escapes,
  shift nibbles, odd sample counts and zero/one/two byte block sizes, and
  pseudo-random byte streams.
  Edge cases: the original encoder's blocks of 6 and 4094 bytes, full-scale
  constants, alternating full-scale samples and byte-range noise; hand-assembled
  blocks with every sample escaped (raw `0x8000`, `0x7FFF`, `0xFFFF`, `0x0000`), shift
  nibble 15 with the byte extremes, shifts 0/15, block sizes 2 and 3 and an escaped
  right sample; pseudo-random streams of 4095 and 4097 bytes.

### Quirks Preserved

- `AudioLib_Compress` returns a size 10 bytes larger than the WVSM stream it
  wrote, because the header size is added to an offset that already includes it.
  `Compress*.tv` store the original return value.
- With `bNativeByteOrder == 0`, `AudioLib_CompressBlock` byte-swaps samples as
  unsigned values, so negative samples are not sign extended. The corpus includes
  negative big-endian samples and the tests compare the original's bytes. The
  QOL build sign extends the swapped samples, so it skips the big-endian cases of
  this corpus that contain a negative sample (`CompressBlockSwappedMatchesNative_Fixed` covers the fix); the
  legacy build (`JONES3D_QOL_IMPROVEMENTS=OFF`) compares all of them.
- `AudioLib_Uncompress` takes the compressor state from the stream header; the
  supplied state is overwritten.

### Inputs Kept Out Of The Corpus

- Full-scale alternating samples (`-32768`, `32767`) crash the original
  `AudioLib_WVSMCompressBlock` (the sample bit length reaches index 17 of a
  17-entry histogram, which `AudioLib_WVSMCompressBlock` no longer overflows).
  They are used only with `AudioLib_CompressBlock`.
- Channel counts above two in `AudioLib_CompressBlock`/`AudioLib_UncompressBlock`
  index past the two-entry compressor state and are not covered.
- Every case was executed twice inside the capture, with different padding bytes
  after the input (`0x00`/`0xFF`) and different output fill (`0xA5`/`0x5A`). Any
  case whose result depended on bytes outside the declared input or output was
  dropped. None of the original cases were dropped. Since the edge-case capture
  every input ends 64 bytes before an inaccessible guard page, so a read further past
  the input faults; such cases are caught and dropped as well.
- Dropped edge cases, `AudioLib_ParseWaveFileHeader`: 124 truncated prefixes whose
  result depends on the padding (a chunk size, count or field is read from it) and
  19 whose `0xFF` padding run read beyond the padding (`trunc_wav_041`-`043`,
  `trunc_aiff_017`-`019`, `trunc_mcmp_wav_004`-`016`: an index count, chunk size
  or block size of `0xFF` bytes moves the parser far past the input). Files whose
  chunk sizes point outside the file are not covered for the same reason.
- Dropped edge cases, `AudioLib_Compress` (WVSM): sizes 2, 3, 6, 4095, 4098 and
  8191, where the last block holds an odd number of 16-bit values or a partial
  value. `AudioLib_WVSMCompressBlock` reads whole left/right pairs, so it reads past
  the input and the result depends on the bytes behind it.
- ADPCM headers with a step index above 88 index past the step table in the
  original (a fault in the capture): the original WVSM signature bytes with one
  byte changed (`0xE4 0x11 0x10 ...`, `... 0x22 0x23` and the tag `WVSm`) are decoded
  as ADPCM with the second step index `0x64` (100) and are not covered. Supplied
  states with a step index outside 0 to 88 and channel arguments above 4 for
  `AudioLib_Compress` (more than two channels) are not covered either.

### WVSM Compression

`AudioLib_WVSMCompressBlock` matches the original on every case of
`CompressWVSM.tv`, including the returned sizes. It keeps the original sample
bit length counters (starting at 1) and uses 18-entry histograms, so the
sample -32768 (bit length 17) no longer writes past the end of the histograms
as it does in the original. `TestCompressWVSMFullScaleSamples` covers that
input, which is kept out of the corpus because it crashes the original.
The streams written by this implementation use the original format.

### Provenance

- Capture harness: a temporary C file built with MSVC (x86, static CRT) as
  `Jones3D.dll` and kept outside the repository (SHA-256 of the sources used:
  `capture.c` `6cb457ff33e4250383dcf5d7cb5358f904eb436afaa029a6f4fde8bf8bfdce0b`,
  the included `edge_parse.inc`
  `4a7c3582ea7faeccf2161da1aa5ed4b6dfdffcb6f085e3ba3fe2bb700cac8061` and
  `edge_codec.inc` `826a9f4503742c3ac20dc4046890bed3fc4a31b7ffc0b5412866e2ad10b4334a`;
  the original cases were captured with the first version,
  `40eddd503bd423fa4d990898ae35d613fe66bc4e75d07a16d353ca48db044437`, and the edge-case
  capture reproduced them byte for byte).
  It contains no hooks. `Jones3D.exe` (this repository's launcher) starts
  `Indy3D.exe` suspended and injects the DLL from its own directory; `DllMain`
  calls the original functions at the addresses above on the generated inputs,
  writes the fixture files and terminates the process.
- Regeneration: build the harness with `cl /MT /LD capture.c /Fe:Jones3D.dll`,
  place it next to `Jones3D.exe` and a copy of `Indy3D.exe` with the SHA-256
  above, run `Jones3D.exe Indy3D.exe`, and collect the files from `out\`.
  Review regenerated fixtures as behavior changes.
- Environment: MSVC 2026 x86 compiler under Wine. No locale, time or
  floating-point state is involved; the generators use integer arithmetic only.
- Two complete runs in fresh processes produced byte-identical files (for the
  original and for the edge-case capture).
- The reconstructed functions reproduce every case in `ParseWaveFileHeader.tv`,
  `CompressBlock.tv`, `UncompressBlock.tv`, `Compress.tv`, `Uncompress.tv`,
  `ResetCompressor.tv` and `WVSMUncompressBlock.tv` and
  `CompressWVSM.tv`, in Debug and Release for both DirectX 6 and DirectX 9
  builds.

### SHA-256

```text
4b7bcee71329b6e349d86e7d56ae7819f22f803c6f1ab68bb4b97cf2f270970b  Compress.tv
265c6e85974539e08c5875a6ad6b81f795e2dca1edee40cf66f2b7c0f33314d6  CompressBlock.tv
f60a5c92b4b6af8a1068e40d0978cee8723ea783dba72374f85156654e644991  CompressWVSM.tv
17690770c764607356ff3fead665d357f6add825e881526ce9bdd0bb722d3e83  ParseWaveFileHeader.tv
414c80b2791dcd4ca7da06ec951eba00bdec37441fe38225a717feda26b01a47  ResetCompressor.tv
34608c7032cad459ee145c0356df43d042b2963cc8ac86ee178ab16a2b4ad87a  Uncompress.tv
8dd03e7f1e7198de7c78b51c03b755158500c61102fd14354273ada867ccc3bd  UncompressBlock.tv
a22e4402ad9d32aac9d24ae6ea1ec72e6d3da626428e438aaebd3e57647b8f76  WVSMUncompressBlock.tv
```
