## sithAIMove Original-Engine Test Vectors

`sithAIMove.tv` holds, for every case of `sithAIMove_aCases` (`Libs/sith/AI/Tests/sithAIMoveVec.h` and
`sithAIMoveVecCases.h`), the complete observable result of the original `sithAIMove` functions: return and output
values, the engine calls they made and every byte they changed in a synthetic AI world, call by call. It was captured
by executing the functions inside the retail `Indy3D.exe` with the world-state vector harness
([Libs/sith/Tests/Vec](../../../Tests/Vec/README.md)). The fixture is consumed by `sithAIMoveVecTest.c`
(`sithAIMoveVecTests`, CTest `sith.AIMove`), which never launches `Indy3D.exe`.

### Reference

| Item | Value |
| --- | --- |
| Executable | Retail v1.0 `Indy3D.exe`, SHA-256 `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` (verified before capture) |
| Debug build | Not captured; the v1.0 entry points below were used |
| Calling convention | `__cdecl` |
| `sithAIMove_Update` | `0x004950C0` |
| `sithAIMove_UpdateMineCar` | `0x00495360` |
| `sithAIMove_UpdateTurn` | `0x004958B0` |
| `sithAIMove_UpdateMove` | `0x00495CD0` |
| `sithAIMove_CheckMoveAlign` | `0x004961A0` |
| `sithAIMove_GetGoalTolerance` | `0x00496200` |
| `sithAIMove_CheckWalkMove` / `CheckFlyMove` / `CheckSwimMove` | `0x00496550` / `0x004966D0` / `0x00496820` |
| `sithAIMove_AIGetMoveState` | `0x004969A0` |
| `sithAIMove_AISpecialTurn` | `0x00496A50` |
| `sithAIMove_UpdateAIMove` | `0x00496BB0` |
| `sithAIMove_GetAIMoveModes` | `0x00496EA0` |
| `sithAIMove_SetSubMode` | `0x00497230` |
| `sithAIMove_PuppetCallback` | `0x00497440` |
| `sithAIMove_AISetLookThing` / `AISetLookPos` / `AISetLookPosEyeLevel` | `0x00497930` / `0x00497A20` / `0x00497BE0` |
| `sithAIMove_AISetMovePos` / `AISetMoveTargetPos` | `0x00497CF0` / `0x00497F30` |
| `sithAIMove_AISpecialMove` / `AIFinalizeSpecialMove` | `0x00497FF0` / `0x00498890` |
| `sithAIMove_AIJump` / `AIStop` / `SetGoalReached` / `Unreachable` | `0x00498B00` / `0x00498B70` / `0x00498BF0` / `0x00498C80` |
| `sithAIMove_StopAIMovement` / `ResetAILook` | `0x00498CF0` / `0x00498D50` |
| `sithAIMove_UpdateBoss` | `0x00498D90` |
| `sithAIMove_UpdateHeadTrackTarget` / `UpdateHeadTrackLookPos` | `0x00499090` / `0x004996C0` |
| `sithAIMove_GetQuetzStrikeAngles` / `UpdateQuetzStrike` | `0x00499A80` / `0x00499CA0` |
| `sithAIMove_GetQuetzAnim2Angles` / `UpdateQuetzAnim2` / `UpdateQuetzAnim3` / `UpdateQuetzAnim5` / `UpdateQuetzAnim4` | `0x0049A020` / `0x0049A1B0` / `0x0049A450` / `0x0049A630` / `0x0049A810` |
| `sithAIMove_UpdateQuetzTail` | `0x0049A9C0` |
| `sithAIMove_StartQuetzStrike` / `StartQuetzAnim2` / `StartQuetzAnim3` / `StartQuetzAnim5` / `StartQuetzAnim4` | `0x0049AA60` / `0x0049AB80` / `0x0049AC50` / `0x0049AF80` / `0x0049B1B0` |
| `sithAIMove_UpdateMardukTail` | `0x0049B260` |

Every function of `sithAIMove.c` of the retail executable is covered. `sithAIMove_AISetMoveTargetPos2` and the
`J3D_DEBUG` block of `sithAIMove_AISetMovePos` only exist in the debug build and are not part of the corpus.

### Setup

- One AI actor (thing 0) with its control block, a target (thing 1, `pTargetThing`), a goal thing (thing 2,
  `goalThing`), a tail segment (thing 3, attached to the actor with `SITH_ATTACH_TAIL`) and a bystander item, in
  three sectors whose flags a case chooses (underwater, `NOACTORENTER`). The actor stands at the origin looking along
  +Y with +Z up unless a case says otherwise. The actor's rig (`renderData.apTweakedAngles`, `paJointMatrices`), a wall
  surface for wall crawling, a puppet state and a puppet class with a chosen set of keyframes (`aModes[major][submode]`)
  and, for the Quetzalcoatl functions, a user block with a strike (random snapshots or filled by the `StartQuetz*`
  functions) are part of the state.
- A case is a list of calls (frames). Before every call it can add to the game time, toggle bits of the AI mode and
  submode, and change the move status and the puppet submode. After every call the harness records a checkpoint. The
  sweeps of `AIGetMoveState` (move status 0..127), `GetAIMoveModes` (28 speeds by 4 axes) and `SetSubMode` (every
  new submode 1..83 from the same starting submode) are loops inside one call; their per-iteration results are
  recorded as result events.
- Replaced by stubs (`sithAIMoveVecSeams.def`, stubs in `sithAIMoveVec.h` and `sithVecWorld.h`): every engine function
  outside `sithAIMove.c` that has side effects or reads tables the tests cannot reach: `sithAI_EmitEvent`,
  `sithAI_HasInstinct`, the `sithAIUtil_*` movement helpers (`CheckPosition`, `CheckPathToPoint`, the heading
  vectors, `ApplyForce`, `AISetMode`, `AIClearMode`, `SweepMoveToPathPos`, `AIAdvanceToNextWpnt`,
  `CheckWpntMoveAlign`), `sithWeapon_HasWeaponSelected`/`IsAiming`, `sithPlayerControls_ProcessWeaponAim`,
  `sithPlayerActions_Jump`, `sithThing_GetThingMeshIndex`/`GetThingJointIndex`/`DetachThing`/
  `CreateQuetzUserBlock`, the puppet functions (`PlayMode`, `ClearMode`, `FindActiveTrack`, `SynchMode`,
  `SetSubMode`, `DefaultCallback`, `FreeTrackByIndex`), the sound class functions, `sithCog_ThingSendMessage`,
  `sithActor_SetHeadPYR`, `sithPhysics_FindFloor`, `sithCollision_FindSectorInRadius`/`MoveThing` and the math
  helpers that are not inline (`rdVector_Normalize3Acc`, `rdVector_Rotate3`/`Rotate3Acc`, `rdMatrix_*`,
  `rdMath_DeltaAngleNormalized`, `stdMath_ArcSin3`/`NormalizeAngle`/`Dist2D1`). The stubs record their arguments
  as events and return what the case configures (results by call number for `CheckPosition` and
  `CheckPathToPoint`); the math stubs follow the structure of the helpers but compute with the shared float
  formulas of the harness, so both sides use the same trigonometry. The functions of `sithAIMove.c` itself, the inline
  vector helpers of `rdVector.h`/`rdMath.h` and `stdUtil_StrCmp` run unchanged on both sides.
- `rand` is the harness stub with a seed a case may choose (`AISpecialMove` with `SITHACTORSPECIALMOVE_DIR_RANDOM`).
- Far globals: `sithTime_g_msecGameTime`, `sithMain_g_frameNumber` and `sithMain_g_sith_mode` are set by the case on
  both sides.
- Comparison: the whole arena (things, control block, puppet state, rig, Quetzalcoatl block, sectors) after every call;
  nothing is excluded or normalized beyond the address normalization of the harness (the puppet class lives in a second
  registered region).
- Environment: as for the other world-state suites (MSVC 2026 x86 harness under Wine, x87 precision 24 bit).

### Cases

`sithAIMove.tv` (788 cases). Case names are `<Function>_<state>`; the table gives the number of cases per function and
what they cover.

| Function | Cases | Covered |
| --- | ---: | --- |
| `Update` | 56 | Early outs (not an actor, health 0 and below, sleeping with and without block/no-slope-move, disabled/dying/destroyed), turn variants (plain, boss, mine car, body tracking with the head-yaw threshold at 20 degrees), the look position of a forward move, alignment (backwards, forwards, plain, waypoints), moving, weapon selected/aiming, the head tracking choice by mode and submode bits, tail and `marduk` updates |
| `UpdateMineCar` | 23 | Immobile, turns to both sides and behind (zero and random joint angles), aligned, slow-turn zone, turn limit equal/just below/just above, zero and negative rate, looking straight up/down (with and without fly), mesh index, tilted actor |
| `UpdateTurn` | 47 | Left/right/behind/diagonal turns through the final snap, slow-turn zone, turn limit equal/below/above (rate chosen so that `maxRot * 1.05` is exactly the angle), negative rate, angles of exactly 45 degrees, immobile, stationary and animated move states, looking straight up/down at the 0.999 limit (equal and above), tilted actor, special turns by angle, keyframe set and move state |
| `UpdateMove` | 71 | Walking at several speeds and distances, early outs, move-forward speed scale and side tolerance, velocity matching, move states, cliff/wall/thing results of `CheckPosition`, flying and swimming checks, goal reached with every follow-up (saved position, path sweep, waypoints, block, continuous motion) and the goal-set event distance |
| `CheckMoveAlign` | 7 | With and without `NOMOVEBACKWARDS`, facing, side, slightly behind, behind |
| `GetGoalTolerance` | 59 | Plain, underwater, flying, different sectors, move-forward side tolerance (both sides, edges), wall crawling (mounting, aligned axes with distances exactly at the tolerance, surface dot product exactly at 0.996 and around it, distance exactly at twice the move size, unreachable with and without block) |
| `CheckWalkMove`, `CheckFlyMove`, `CheckSwimMove` | 17, 9, 14 | Frame parity, wall crawling offset, every floor check result, wall crawl instinct, step-on things, submerged, sector missing/no-actor-enter/underwater, rising and sinking at the surface |
| `AIGetMoveState` | 3 | Every move status 0..127 and values outside |
| `AISpecialTurn` | 11 | 22 angles (thresholds at 22.5, 67.5, 112.5, 157.5 and 180 degrees, both signs) by submode flag, keyframe subsets and busy move states |
| `UpdateAIMove` | 45 | Velocity along every axis with walk/run/fall/fly modes, thresholds, turning, immobile, no sector, move states, controls disabled/forced move play, no keyframes, submode changes between calls |
| `GetAIMoveModes` | 13 | 28 speeds on the four axes with attached/not attached, fast-move flags, turning with every turn move status, flyer move |
| `SetSubMode` | 24 | Every new submode from stand/walk/run/turn/fall/other, with and without transition keyframes, tracks and `PlayMode` results, other major modes, missing puppet class/state, repeated calls |
| `PuppetCallback` | 24 | Finish of every move status (velocity zero/slow/fast), foot, died and other markers, with and without control block, dying/destroyed, knocked out |
| `AISetLookThing`, `AISetLookPos`, `AISetLookPosEyeLevel` | 17, 12, 7 | Actor/player/item looked at and owners, locked, disabled (editor), want-all-events, distance thresholds around 0.001, not-a-number and huge positions, tilted actor |
| `AISetMovePos`, `AISetMoveTargetPos` | 27, 8 | Distances around the goal tolerance, locked, disabled, events, home distance, underwater, flying, wall crawling, repeated calls |
| `AISpecialMove` | 77 | Roll and strafe (both directions, random, target, dot product threshold 0.4, keyframes, path and position checks, wall/cliff), turns of 45/90/135/180 degrees with missing keyframes, wall mount by wall angle (normals around the 40/60/79/80/100 degree limits, with exact 79, 80 and 100 degrees and their float neighbors) |
| `AIFinalizeSpecialMove` | 17 | Strafe, every turn status, wall mount, other flags |
| `AIJump`, `AIStop`, `SetGoalReached`, `Unreachable`, `StopAIMovement`, `ResetAILook` | 4, 5, 8, 3, 3, 2 | Animation available or not, time wrap, every combination of block/new-move/waypoint/move-state/continuous-motion bits |
| `UpdateBoss` | 19 | Turns, snaps, straight up/down limit at 0.99, tilted actor |
| `UpdateHeadTrackTarget`, `UpdateHeadTrackLookPos` | 43, 26 | Target left/right/ahead/below/behind/side with the start/stop dot thresholds, invisible/blind targets, thing types, no-slope-move/block, head yaw/pitch limits and speed, tilted actor, completion of the look |
| `GetQuetzStrikeAngles`, `GetQuetzAnim2Angles` | 12, 7 | Distances around the 0.3 and 0.5 clamps, crouching target, rotated and tilted |
| `UpdateQuetzStrike`, `UpdateQuetzAnim2`, `Anim3`, `Anim5`, `Anim4` | 14, 7, 6, 5, 5 | Every snapshot, last snapshot, steps that cross snapshots (exact, large, huge), wrong mode, no block |
| `UpdateQuetzTail` | 8 | Dispatch by strike mode |
| `StartQuetzStrike`, `Anim2`, `Anim3`, `Anim5`, `Anim4` | 4, 3, 3, 3, 3 | Start followed by frames until the animation ends, no block |
| `UpdateMardukTail` | 7 | `marduk`, `marduk2`, other names, joint indices |

### Inputs Kept Out Of The Corpus

- A missing control block or owner (`pLocal` or `pLocal->pOwner` NULL) for any function: the original asserts and then
  dereferences the pointer.
- `sithAIMove_UpdateQuetz*` with the snapshot index 0 while the step does not finish it (the assert is followed by a
  read before the snapshot array) and `StartQuetz*`/strike snapshots 1..4 without a target.
- `sithAIMove_AIFinalizeSpecialMove` for a turn flag while the move status is not the matching turning status (the
  rotation is an uninitialized local on both sides) and combinations of special move flags the game never uses.
- `sithAIMove_GetAIMoveModes` with a not-a-number speed (the outputs stay unassigned).
- `UpdateMove`/`SetLookPos` family with `pInSector` NULL where the function dereferences it (`GetGoalTolerance`).
- `sithAIMove_UpdateMineCar`: the two assignments of `SITHPLAYERMOVE_TURNING_LEFT/RIGHT` (and the 105/106 variants)
  under `moveState == SITHAIMOVE_STATE_NONE` are unreachable (`moveState` is `NONE` only when the angle is +0.0, which
  is always inside the turn limit), so the vectors cannot execute them.

### Coverage Of The Corpus

Statement coverage was measured by compiling an instrumented copy of `sithAIMove.c` into the test: every statement of
the retail functions is executed except the two `UpdateMineCar` blocks named above, the final `return 0` of
`AISpecialMove` (dead code) and the debug-only `AISetMoveTargetPos2`. Comparisons were additionally probed with mutants
of the reconstruction (every `<`, `<=`, `>`, `>=` of the file plus samples of the other operators and constants): the
corpus has cases at the exact threshold values and one float step on either side wherever the compared value is an
input (speeds, distances, dot products, times, up dots, `maxRot * 1.05`, the head limits, the wall angles 79/80/100).
Thresholds on angles that the stub trigonometry produces (exactly 1, 40, 60 degrees) and the up dot of the mine car
(which cannot be set to the threshold float) are not probed at equality.

### Known Differences

- With `J3D_QOL_IMPROVEMENTS`, the fix of `sithAIMove_UpdateMineCar` queries `sithAIMove_AIGetMoveState` where the
  original never assigns its `moveState` local. That is one more AI call, so the cases listed in `sithAIMoveVecTest.c`
  are expected to differ. The legacy build matches the original in every case.

### Provenance And Regeneration

- Capture harness: the generic world-state capture DLL (kept outside the repository, SHA-256 of the source used:
  `5ec8edc75e92213913e56e663aa41d4eb3fb0cb6233153efdb12f51d1cf8fc53`), built with
  `/DSUITE_HEADER="\"sithAIMoveVec.h\"" /DSUITE_SEAMS="\"sithAIMoveVecSeams.def\""` as described in
  [Libs/sith/Tests/Vec/README.md](../../../Tests/Vec/README.md). It patches `rand` and the seams listed above; no
  other function of the executable is modified.
- Capture log: 788 cases, 0 unstable, 0 lookup mismatches (the suite has no lookups). Two complete captures in fresh
  processes produced byte-identical files.
- The reconstruction reproduces every case, in Debug and Release for DirectX 6 and DirectX 9.

### SHA-256

```text
899da01b91c9a799eac623cf42b9eaa06da7420d9d03c1604d112111206c24bc  sithAIMove.tv
```
