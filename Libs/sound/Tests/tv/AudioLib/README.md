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

- `ParseWaveFileHeader.tv` (42 cases): NULL input, RIFF WAVE (mono/stereo, 8/16
  bit, empty data, 18 byte `fmt `, skipped `LIST` chunk, `sync` chunk with and
  without each output pointer, no outputs, single outputs), well-formed files
  that are rejected (non-PCM format tag, no `data` chunk, missing `WAVEfmt `),
  INDYWV (with and without extra data, with and without output pointers), iMUS,
  AIFF (44100, 22050, 11025 Hz exponents, an unlisted exponent, extra chunk,
  empty data, wrong `COMM`), `MCMP` wrapped WAV/iMUS/INDYWV/AIFF (AIFF is
  rejected after `MCMP`) and an unknown signature.
- `CompressBlock.tv` (57 cases): 0 to 4096 samples, mono and stereo, silence,
  square, sine (integer approximation), ramp, impulses, full-scale alternation,
  seeded noise at several amplitudes, loud-to-quiet bursts (the step index sweeps
  through every sample width), supplied and reset states, big endian input
  (`bNativeByteOrder == 0`), and four block chains with odd block sizes.
  The original's encoder uses 4, 5, 6 and 7 bit samples, escape codes and
  saturation in both directions across these cases.
- `UncompressBlock.tv` (87 cases): the original encoder's blocks decoded,
  garbage input state with the reset flag, block chains, and pseudo-random bit
  streams (seeds 101 to 113) in mono and stereo, with supplied states at the
  limits of the step index and sample range.
- `Compress.tv` (20 cases): ADPCM streams (channel argument 3 and 4) of 0 to 8192
  bytes, supplied states, and three-call chains per channel layout.
- `CompressWVSM.tv` (14 cases): WVSM streams (channel argument 1 and 2) of 0 to
  10000 bytes, including sizes around the 0x1000 block boundary.
- `Uncompress.tv` (43 cases): the original compressor's ADPCM and WVSM streams
  decoded with a garbage input state, and hand-assembled ADPCM headers with
  pseudo-random payloads (including a header resembling the WVSM signature).
- `ResetCompressor.tv` (3 cases): garbage, extreme and zero states.
- `WVSMUncompressBlock.tv` (22 cases): the original encoder's blocks for several
  signals and sizes (4 to 4096 bytes), hand-assembled blocks with `0x80` escapes,
  shift nibbles, odd sample counts and zero/one/two byte block sizes, and
  pseudo-random byte streams.

### Quirks Preserved

- `AudioLib_Compress` returns a size 10 bytes larger than the WVSM stream it
  wrote, because the header size is added to an offset that already includes it.
  `Compress*.tv` store the original return value.
- With `bNativeByteOrder == 0`, `AudioLib_CompressBlock` byte-swaps samples as
  unsigned values, so negative samples are not sign extended. The corpus includes
  negative big-endian samples and the tests compare the original's bytes.
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
  dropped. None were dropped.

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
  `Jones3D.dll` and kept outside the repository (SHA-256 of the source used:
  `40eddd503bd423fa4d990898ae35d613fe66bc4e75d07a16d353ca48db044437`).
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
- Two complete runs in fresh processes produced byte-identical files.
- The reconstructed functions reproduce every case in `ParseWaveFileHeader.tv`,
  `CompressBlock.tv`, `UncompressBlock.tv`, `Compress.tv`, `Uncompress.tv`,
  `ResetCompressor.tv` and `WVSMUncompressBlock.tv`, and the six matching cases
  of `CompressWVSM.tv`, in Debug and Release for both DirectX 6 and DirectX 9
  builds.

### SHA-256

```text
25376c7d34a24c317456bfc109132d2d7d87477c37aca9f6cc0944169791abc5  Compress.tv
82f2c360213b333f0c5a548e0399045c9f0cf3a7b6296bd9ed689d95fd6abd61  CompressBlock.tv
f908ffa3283b654fda4a7fe6087e0615ac885297c5fb0dd8004bc03e1607d86f  CompressWVSM.tv
b09461c9fed09456bb907c9a8c36096d8921261fcc1d6e40113be25075eac6ad  ParseWaveFileHeader.tv
0fcc376c268a029200c11f666210224428b6214d80043d1870ded58fd0ed45d9  ResetCompressor.tv
b08ec4806cfaf4b4d84664b6b1f67408083b7f19b282b978893182744c025caa  Uncompress.tv
d96a8e023990463052f97375b4d6e951edc4683ed3213a736d1f5a96fcf65b9e  UncompressBlock.tv
95693d3b1a7ae73ea97bee5ff9e795b13d9e06e72ba5186f498095612831fd6d  WVSMUncompressBlock.tv
```
