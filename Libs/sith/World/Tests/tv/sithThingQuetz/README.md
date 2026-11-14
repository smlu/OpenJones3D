## sithThing Quetz Original-Engine Test Vectors

`sithThingQuetz.tv` holds, for every case of `sithThingVec_aQuetzCases` (`Libs/sith/World/Tests/sithThingVecQuetz.h`),
the complete observable result of the original `sithThing_CreateQuetzUserBlock` and
`sithThing_UpdateQuetzUserBlock`, captured by executing them inside the retail `Indy3D.exe` on a synthetic engine
state with the world-state vector harness ([Libs/sith/Tests/Vec](../../../Tests/Vec/README.md)). They are consumed
by `sithThingQuetzVecTest.c` (suite `sithThingQuetz` of `sithTests`), which never launches
`Indy3D.exe`.

### Reference

| Item | Value |
| --- | --- |
| Executable | Retail v1.0 `Indy3D.exe`, SHA-256 `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` (verified before capture) |
| Debug build | Not captured; the v1.0 entry points below were used |
| `sithThing_CreateQuetzUserBlock` | `0x0043A670` |
| `sithThing_UpdateQuetzUserBlock` | `0x0043A840` |
| File statics | `sithThing_prevQuetzAttachInfo` `0x0055EE5C`, `sithThing_curQuetzAttachInfo` `0x00561260`, `sithThing_bQuetzTurningSharp` `0x005612B8` (an `int` in v1.0, a `bool` in the reconstruction; recorded as 0/1) |
| Calling convention, layouts | `__cdecl`; the harness is compiled against the project's headers (`static_assert`s on `SithThing` 928 bytes and the other structs hold) |

### Setup

- Thing 0 is the Quetz head (actor, physics) in sector 0 at (64, 96, 32). Its `pAttachedThing` chain holds the tail
  segments (actors with `SITH_ATTACH_TAIL`, attach offset z of `-n/64`, placed 0.09 apart behind the head) and,
  per case, other attached things (items with `SITH_ATTACH_THING`) before the first, after the first and after the
  last segment. Orientations come from integer headings (normalized with `sqrtf`), so they are exact on both sides.
- Watched: the whole arena (things, sectors and their thing lists, the allocation pool holding the user block) and
  mirrors of the three file statics.
- Seams (`sithThingQuetzVecSeams.def`): `stdMemory_Malloc` (allocates from the arena pool, can fail),
  `sithCollision_FindSectorInRadius` (returns the start sector or, per frame, the next sector; records its
  arguments), `sithCollision_MoveThing` (moves the full, half or no distance; records its arguments),
  `sithSector_ValidateSectorPointer` (lookup, verified against the original: 0 mismatches), and the math helpers
  `rdVector_Normalize3`, `rdVector_Normalize3Acc`, `rdMatrix_TransformVector34`, `rdMatrix_BuildFromLook34`,
  `rdMatrix_PreRotate34`, `rdMath_DeltaAngleNormalized` and `stdMath_SinCos` (the shared math stubs; their tables
  and precision are not what is under test). `sithThing_SetSector` (and the `sithThing_ExitSector`,
  `sithThing_EnterSector` and `sithThing_ValidateThingPointer` it calls) belong to `sithThing.c` and run unchanged
  on both sides; the sectors have no water, aetherium or COG flags, so it only moves segments between sector lists.
- Environment: MSVC 2026 x86 harness under Wine, x87 precision control 24 bit (the original then rounds every
  operation to single precision like the reconstruction), stdMath rounding control words set as `stdStartup` does.
  `sithTime_g_fps` is set per case.
- Comparison normalization: `orient.dvec` of every thing is not compared. `sithThing_UpdateQuetzUserBlock` copies
  its local orientation into the segment after `rdMatrix_BuildFromLook34`/`rdMatrix_PreRotate34`, which never set
  `dvec`, so the copied `dvec` is an uninitialized stack value on both sides (and unused).

### Cases

`sithThingQuetz.tv` (48 cases; 16 `Create_`, 32 `Update_`; 8 named `_edge_`):

- `Create_`: no tail segment (with and without other attachments, no attachments at all), 1, 2, 3, 4, 8 (with
  other attachments in all three places) and 25 segments; 26 segments (the block is full and `numAttachInfos` stays
  25), 27 and 34 segments (creation stops at 26); a pitched and a vertical head (the flattened direction of a
  vertical look vector is a zero vector); an existing block (returns 1, nothing changes); failed allocation (error
  print, returns 0).
- `Update_` frame scripts (one checkpoint per frame; the head turns and moves before each frame): straight with 1,
  3, 8 (block made by an explicit create) and 26 segments; a gentle consistent turn (4 and 12 segments, 60 fps);
  45 degrees per frame with 2, 4, 8 and 26 segments (sharp turning on, longer segments on reset); a U-turn;
  alternating turns (turn direction flips along the tail, `bTurnFlipped`, inconsistent turns); `bResetSegment`
  before two frames; sharp turning set before the first frame (straight and with long steps); steps longer than
  a segment (every ratio clamps to 0); backwards, zero, sideways, upward and diagonal velocities (dot clamped,
  zero, cos 45); negative `distanceMoved` (ratio clamps to 1); segments changing sector on every move with full,
  half and blocked collision moves; other attachments first and everywhere; a segment losing and regaining its tail
  flag and the last segment being detached; other attachments becoming tail segments the block has no state for
  (length 0: the ratio of a moving Quetz is -inf and clamps to 0); a standing Quetz whose first segment is put
  exactly where it should be (no movement: no sector search, no collision move); a pitched head; failed allocation
  in every frame; existing blocks with random segment state (3, 6 and 10 segments); 0 and 1000 fps.

A temporary instrumentation of the reconstruction (not committed) confirmed that the cases reach every branch of
both functions except the unreachable non-negative segment length in `sithThing_CreateQuetzUserBlock`: block
allocation failure, other attachments skipped, every segment kind of the creation, the 26 segment limit, the dot
clamp on both sides, the ratio clamp on both sides, no movement, each reset reason (ratio, `bResetSegment`, flipped
turn), length with and without a previous segment, the sharp turning scale, inconsistent turns and the final sharp
turning check in both directions.

### Inputs Kept Out Of The Corpus

- `sithThing_UpdateQuetzUserBlock` on a head without any tail segment: the original leaves `bTurnConsistent`,
  `firstAngle` and `totalAngle` uninitialized (also in the debug build) and then reads them, so its result depends
  on the stack. The reconstruction zero-initializes them (`Fixed:` in `sithThing.c`); there is no original behavior
  to compare with.
- More than 26 tail segments in `sithThing_UpdateQuetzUserBlock`: it indexes `aAttachInfos` past its 26 entries and
  overwrites the rest of the user block. (Creation alone with 27 and 34 segments is in the corpus.)
- A standing Quetz with tail segments the block has no state for (length 0, ratio 0/0 = NaN).
- `sithCollision_FindSectorInRadius` returning NULL (`sithThing_SetSector` asserts and dereferences it) and sectors
  or things that make `sithThing_EnterSector` call further engine functions (water, aetherium, COG links).
- The tail branch of `sithThing_UpdateMove` (position fudge around the collision move) is a different function and
  not part of this corpus.

### Known Differences

None. The reconstruction reproduces every case byte for byte in Debug and Release for DirectX 6 and DirectX 9.

### Provenance And Regeneration

- Capture harness: the generic world-state capture DLL (kept outside the repository, SHA-256 of the source used:
  `5ec8edc75e92213913e56e663aa41d4eb3fb0cb6233153efdb12f51d1cf8fc53`), built with
  `/DSUITE_HEADER="\"sithThingVecQuetz.h\"" /DSUITE_SEAMS="\"sithThingQuetzVecSeams.def\""` as described in
  [Libs/sith/Tests/Vec/README.md](../../../Tests/Vec/README.md). It patches only the seams above and `rand`; no
  other function of the executable is modified.
- Capture log: 48 cases, 0 unstable, 0 lookup mismatches. Two complete captures in fresh processes produced
  byte-identical files.
- Review regenerated fixtures as behavior changes and rerun the tests.

### SHA-256

```text
7a587c17f8924078760ed58a8fa73c1622550a98d94d0e92d9951a22b0a34c6a  sithThingQuetz.tv
```
