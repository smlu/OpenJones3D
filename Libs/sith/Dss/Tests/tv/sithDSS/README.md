## sithDSS Original-Engine Test Vectors

These files hold, for every case, the complete observable result of the original
`sithDSS` functions (the save/network message writers and readers), captured by
executing them inside the retail `Indy3D.exe` on a synthetic engine state. They are
consumed by `TestOriginal*` in `Libs/sith/Dss/Tests/sithDSSTest.c`. The tests read only
these files; they never launch or depend on `Indy3D.exe` or game assets.

### Reference

| Item | Value |
| --- | --- |
| Executable | Retail v1.0 `Indy3D.exe` |
| SHA-256 | `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` (verified before capture) |
| Debug build | Not captured. Only the v1.0 entry points below were resolved. |
| Calling convention | `__cdecl` (`J3DAPI`), prototypes from `Libs/sith/RTI/symbols.h` |
| Layout check | The harness is compiled against the project's own headers (`sith/types.h` and friends); the `static_assert`s on `SithThing` (928), `SithSurface` (96), `SithSector` (208), `SithAIControlBlock` (728), `SithAnimationSlot` (168), `SithCamera` (2364), `SithMessage` (3084) and the other structs the cases fill hold in both the harness and the tests |

| Function | v1.0 address |
| --- | --- |
| `sithDSS_SurfaceStatus` | `0x004B1080` |
| `sithDSS_ProcessSurfaceStatus` | `0x004B11A0` |
| `sithDSS_SectorStatus` | `0x004B12E0` |
| `sithDSS_SyncSeenSectors` | `0x004B13B0` |
| `sithDSS_ProcessSyncSeenSecors` | `0x004B1430` |
| `sithDSS_ProcessSectorStatus` | `0x004B1490` |
| `sithDSS_SectorFlags` | `0x004B15A0` |
| `sithDSS_ProcessSectorFlags` | `0x004B1630` |
| `sithDSS_AIStatus` | `0x004B16C0` |
| `sithDSS_ProcessAIStatus` | `0x004B19E0` |
| `sithDSS_Inventory` | `0x004B1D80` |
| `sithDSS_ProcessInventory` | `0x004B1E50` |
| `sithDSS_AnimStatus` | `0x004B1F00` |
| `sithDSS_ProcessAnimStatus` | `0x004B23A0` |
| `sithDSS_PuppetStatus` | `0x004B2820` |
| `sithDSS_ProcessPuppetStatus` | `0x004B2A10` |
| `sithDSS_SyncTaskEvents` | `0x004B2C10` |
| `sithDSS_ProcessSyncTaskEvents` | `0x004B2C80` |
| `sithDSS_SyncCameras` | `0x004B2CF0` |
| `sithDSS_ProcessSyncCameras` | `0x004B2E20` |
| `sithDSS_SyncGameState` | `0x004B2F30` |
| `sithDSS_ProcessSyncGameState` | `0x004B3220` |
| `sithDSS_SyncVehicleControlState` | `0x004B3510` |
| `sithDSS_ProcessVehicleControlsState` | `0x004B3630` |
| `sithDSS_SendEndSection` (end of section message) | `0x004B3760` |
| `sithDSS_ProcessEndSection` (its reader) | `0x004B3790` |


Fixtures: `sithDSS.tv`.

### How The Vectors Are Produced

Writers serialize engine state into `sithMulti_g_message` and hand it to
`sithMessage_SendMessage`; readers change engine state. Neither can run without the
engine, so the harness builds a small synthetic engine state in the memory of the
running executable (a world with sectors, surfaces, materials, things, a player, AI
control blocks, puppets, cameras, animation slots and the global gameplay variables),
fills it with distinct seeded values, and calls the original functions by address.

- The state builder, the case table, the call stubs and the record format live in
  `Libs/sith/Dss/Tests/sithDSSVec*.h` and are compiled by the harness and by the test
  from the same source, so both sides build the same state. The seed of a case is the
  hash of its name; all values are integers or multiples of 1/16 unless a case says
  otherwise.
- Engine functions with side effects that the DSS functions call (the sends, sound,
  puppet and thing creation functions, ...; the list is `sithDSSSeams.def`) are
  replaced by shared stubs. In the harness the original functions are patched to
  jump to the stub, in the tests the calls of `sithDSS.c` are renamed to the
  stub (`dssv_<name>`). Every call is recorded with its arguments (pointers as stable
  ids) and part of the case result. `sithMessage_SendMessage` records the complete
  message, the destination, the stream and the DirectPlay flags.
- The pure lookups (thing/sector/surface/model/material/cog/keyframe by index and back)
  are not patched in the harness: the original functions run on the synthetic world
  and the harness verifies the stub of the test against them for all indices and
  pointers the cases can produce (`lookup verification: 0 mismatches` in the log).
- Writer cases: the writer is called and the case result holds its return value, the
  recorded send (with all message bytes) and a delta of the synthetic state (which is
  expected to be empty). Reader cases are fed the message bytes of a writer case,
  optionally edited (`DssvMut` in the case table, applied by the harness and stored in
  the fixture), run on a world built from a different seed, and the result holds the
  return value, the engine calls the reader made and the delta of the state: every
  byte of the synthetic world and of the watched engine globals that changed (pointers
  into the synthetic state are normalized).
- Every case is run twice in the capture with different garbage (`0xA5` / `0x5A`) in
  the unused tail of every array of the world and in the message buffers behind the
  payload (`sithMulti_g_message` for writers, the input message for readers); the two
  results must be identical, which rejects reads and writes outside the data the
  function is given. No case was dropped for that reason. The tests run every case
  with both fills as well. Both complete capture runs in fresh processes produced
  byte-identical files.
- The rotation helpers `rdMatrix_BuildRotate34` / `rdMatrix_ExtractAngles34` and
  `rdPuppet_NewEntry` are stubbed as well (simple deterministic formulas, no
  trigonometry): their math is not what is under test here.

### File Format

All integers are little endian. Every file starts with `"DSTV"`, a `u32` version (1)
and a `u32` case count. A case is: `u8` name length, name, `u8` hasInput, (if
hasInput: `u16` message type, `u32` message length, message payload), `u32` record
size, record.

A record is: `u32` return value; four `u16` counts of host service calls
(asserts, error prints, status prints, `stdMemory_Malloc` calls); `u32` size and
bytes of the recorded engine calls (`DssvEvent` ids with `u32` arguments, see
`sithDSSVec.h`); `u32` number of state changes followed by that many
`u32 offset, u16 size, bytes` runs, where the offset is into the concatenation of the
watched memory (the synthetic world first, then the engine globals, see
`dssv_WatchAll`).

The expected record of a case is compared byte for byte with the record produced by the
reconstructed function. On a failure the test prints the section and the first
differing bytes; `SITHDSS_VERBOSE=1` in the environment also prints the decoded state
changes of both records.

### Cases

`sithDSS.tv` (165 cases):

- Surface status: every surface kind (with and without material and adjoin, 3 and 4
  vertices). Reader: all of them, a surface and material index past the end, a static
  material, a cel number beyond the material's cels and `0xFFFF`.
- Sector status/flags/seen sectors: sectors with and without the thrust vector and the
  adjoins-off flag; reader edits that switch the adjoins off (`HideSectorAdjoins`) and
  on, indices past the end, the missing world.
- AI status: every combination of the moving, turning and fleeing modes (which add
  fields), no frames, no goal/target thing, a class with five instincts; reader: all of
  them, failed frame allocation, bad thing index, a non-AI thing and a bad class.
- Inventory: player, actor, rejected weapon/free/corpse things, item 0, 17 and 199;
  reader edits with bad thing/item indices.
- Animation status: one case per flag group, a combination, no flags (the reader stops
  the animation) and all flags at once; reader: all of them, a new slot for an unknown
  animation id, a sprite thing, bad thing and surface indices.
- Puppet status: three things times callbacks (all seven known callbacks across the
  cases and an unknown one), no puppet state, a short track list, no tracks; reader:
  these, failing `sithPuppet_NewTrack`, bad things and a thing without puppet.
- Task events (future and past event time), cameras (current camera 0, 3 and 6, camera
  numbers past the last), game state (everything set, nothing set, ten chalk marks),
  vehicle control state and the end of section message.

### Known Differences

- `ProcessPuppetStatus_zerospeed`, `ProcessPuppetStatus_negativespeed`: the
  original stores the playback speed of a puppet track as read; the reconstruction
  replaces a value of zero or less with 1.0 (an intentional `Added:` change in
  `sithDSS_ProcessPuppetStatus`). `TestOriginalPuppetStatus` expects exactly these two
  cases to differ and is reported as ignored.

### Inputs Kept Out Of The Corpus

- Messages that make a reader consume more bytes than the message holds (for example a
  state update of a thing with a puppet class applied to a message written for a thing
  without one) read the bytes behind the payload and depend on the fill (the original
  can crash). The malformed-input checks the reconstruction added (`Fixed:` in
  `sithDSS_ProcessPuppetStatus`, armed/move modes and track counts) have no original
  behavior to compare with and are not part of the corpus.
- Counts that overflow fixed arrays (`sithDSS_ProcessSyncGameState` chalk marks, a
  preserved original bug).

### Provenance

- Capture harness: a temporary C file built with MSVC (x86, static CRT) as
  `Jones3D.dll` and kept outside the repository (SHA-256 of the source used:
  `b5ff22764bc51ca780b5f0a5557988eb0ebf8f87260262db5f96930cc391aec8`), including the shared headers of `Libs/sith/Dss/Tests`. It
  patches the engine functions of `sithDSSSeams.def` to jump to the stubs, builds the
  state of each case, calls the original functions at the addresses above and writes the
  fixtures; no other function of the executable is modified. `Jones3D.exe` (this
  repository's launcher) starts `Indy3D.exe` suspended and injects the DLL from its own
  directory; a helper thread terminates the process after the files are written.
- Regeneration: build the harness with
  `cl /MT /LD capture.c /DDSSV_HARNESS /I<Libs/sith/Dss/Tests> /I<generated j3dcore header dir> /I<Libs> /I<DirectX61c include> /Fe:Jones3D.dll`
  place it next to `Jones3D.exe` and
  a copy of `Indy3D.exe` with the SHA-256 above, run `Jones3D.exe Indy3D.exe` and collect
  the files from `out\`. Review regenerated fixtures as behavior changes.
- Environment: MSVC 2026 x86 compiler, executed under Wine. No locale, time or
  floating-point control state is involved.
- The tests redirect the engine globals the DSS code reaches through `J3D_DECL_FAR_VAR`
  to static memory (`sithDSSTestAddrs.h`, force-included) because the original addresses
  cannot be mapped in every host; the test image is linked at a fixed high base so the
  normalized addresses of the synthetic state can never equal a generated value.
- The reconstructed functions reproduce every case except the ones listed under Known
  Differences, in Debug and Release for both DirectX 6 and DirectX 9 builds.

### SHA-256

```text
820dc9c5c44824ea34fd3e3a1077529cb0deeb6016821f54f3818aaa17f18b9d  sithDSS.tv
```
