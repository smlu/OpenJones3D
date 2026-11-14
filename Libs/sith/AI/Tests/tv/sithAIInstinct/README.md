## sithAIInstinct Original-Engine Test Vectors

`sithAIInstinct.tv` holds, for every case of `sithAIVec_aInstinctCases` (`Libs/sith/AI/Tests/sithAIInstinctVec.h`),
the complete observable result of the original `sithAIInstinct_InitInstincts` and of the instinct registry functions
of `sithAI.c` it uses and that look the instincts up (`sithAI_RegisterInstinct`, `sithAI_FindInstinct`), captured by
executing them inside the retail `Indy3D.exe` with the world-state vector harness
([Libs/sith/Tests/Vec](../../../Tests/Vec/README.md)). They are consumed by `sithAIInstinctVecTest.c`
(`sithTests`, CTest `sith.AIInstinct`), which never launches `Indy3D.exe`.

### Reference

| Item | Value |
| --- | --- |
| Executable | Retail v1.0 `Indy3D.exe`, SHA-256 `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` (verified before capture) |
| Debug build | Not captured; the v1.0 entry points below were used |
| `sithAIInstinct_InitInstincts` | `0x0048EF50` |
| `sithAI_RegisterInstinct` | `0x0042AD40` |
| `sithAI_FindInstinct` | `0x0042AE40` |
| Registry statics | `aRegisteredInstincts` `0x0055EDC8` (pointer), `numRegisteredInstincts` `0x0055EDCC` |
| Calling convention | `__cdecl` |

### Setup

- The registry pointer is set to the arena's instinct array (32 usable entries, 8 more past the end) with a given
  number of entries already in use; those hold earlier registrations (`pre<n>`, `sithAIInstinct_Hop`, random mode
  and event masks), the rest of the array the run's garbage fill.
- Registered function pointers are recorded as their number in the list of the 25 instinct functions (v1.0 address
  on one side, the reconstructed function on the other).
- Watched: the arena, mirrors of the registry pointer and count, and a mirror of the registry entries. No seams: the
  functions only use the registry and the stdUtil string functions; `sithAI.c` is compiled into the test through
  `sithAIVecTested.c`, which gives access to the registry statics.
- Comparison normalization: the bytes of a name field after the terminator are not compared (the mirror zeroes
  them). The original copies names with `strncpy` and so zero-fills the whole field; the reconstruction copies the
  string only (the Debug CRT fills the rest of its field with `0xFE`). Nothing reads past the terminator. The name
  itself and every other field are compared exactly.
- Environment: as for the other world-state suites (MSVC 2026 x86 harness under Wine, x87 precision 24 bit); no
  floating point is involved.

### Cases

`sithAIInstinct.tv` (21 cases):

- `InitInstincts_`: 0 entries in use (engine startup), 1, 6, 7 (the 25 instincts fill the registry exactly), 8 (the
  last instinct is rejected), 20, 31; edge cases 32 (full, everything rejected), 33 and `UINT_MAX` in use; two and
  three calls in a row (duplicates until the registry is full). One checkpoint per call. After the call(s) of the
  0, 6, 7, 8 and 32 cases and of the two-call case, `sithAI_FindInstinct` looks up every registered name, case variants
  (`LISTEN`, `FearGunShot`: the lookup ignores case), a prefix, an extension, an empty and an unknown name, earlier
  registrations and NULL (error print).
- `RegisterInstinct_`: after initialization and alone: an empty name, one character, 61 and 62 characters, all-ones
  and sign-bit mode and event masks, a NULL function (assert, then registered), followed by lookups; the last free
  entry (registered, then the next one rejected); a full registry; edge cases with names of 63, 64 and 199
  characters (see Known Differences).
- `FindInstinct_`: an empty registry and one with only earlier registrations.

### Inputs Kept Out Of The Corpus

- A NULL name for `sithAI_RegisterInstinct` (the original asserts and then copies from NULL).
- Lookups in a registry whose count exceeds its 32 entries (the lookup compares names in the garbage past the end).

### Provenance And Regeneration

- Capture harness: the generic world-state capture DLL (kept outside the repository, SHA-256 of the source used:
  `5ec8edc75e92213913e56e663aa41d4eb3fb0cb6233153efdb12f51d1cf8fc53`), built with
  `/DSUITE_HEADER="\"sithAIInstinctVec.h\"" /DSUITE_SEAMS="\"sithAIInstinctVecSeams.def\""` as described in
  [Libs/sith/Tests/Vec/README.md](../../../Tests/Vec/README.md). It patches only `rand`; no other function of the
  executable is modified.
- Capture log: 21 cases, 0 unstable, 0 lookup mismatches. Two complete captures in fresh processes produced
  byte-identical files.
- The reconstruction reproduces every case except the known differences, in Debug and Release for DirectX 6 and
  DirectX 9.

### SHA-256

```text
f2824aa7f9985e0bf92db7eb9a3d5c56dc86f31dd662a2950e020f12cb4efe3b  sithAIInstinct.tv
```
