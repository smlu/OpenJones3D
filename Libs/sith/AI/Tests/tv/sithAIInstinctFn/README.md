## sithAIInstinct Function Original-Engine Test Vectors

`sithAIInstinctFn.tv` holds, for every case of `sithAIFnVec_aCases` (`Libs/sith/AI/Tests/sithAIInstinctFnVecCases.inc`,
included by `sithAIInstinctFnVec.h`), the complete observable result of the original instinct functions of
`sithAIInstinct.c` (everything but `sithAIInstinct_InitInstincts`, which has its own fixture in
[tv/sithAIInstinct](../sithAIInstinct/README.md)): the return value, every engine call the function makes with its
arguments, every change of the AI state, and the counts of asserts, error prints and warning prints. It was captured by
executing the functions inside the retail `Indy3D.exe` with the world-state vector harness
([Libs/sith/Tests/Vec](../../../Tests/Vec/README.md)). The fixture is consumed by `sithAIInstinctFnVecTest.c`
(suite `sithAIInstinctFn` of `sithTests`), which never launches `Indy3D.exe`.

### Reference

| Item | Value |
| --- | --- |
| Executable | Retail v1.0 `Indy3D.exe`, SHA-256 `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` (verified before capture) |
| Debug build | Not captured; the v1.0 entry points below were used |
| Calling convention | `__cdecl` |

| Function | Address | Function | Address |
| --- | --- | --- | --- |
| `sithAIInstinct_Listen` | `0x0048F220` | `sithAIInstinct_Dodge` | `0x00491800` |
| `sithAIInstinct_LookForTarget` | `0x0048F8B0` | `sithAIInstinct_HitAndRun` | `0x00491E80` |
| `sithAIInstinct_SenseDanger` | `0x0048FC40` | `sithAIInstinct_Retreat` | `0x00492050` |
| `sithAIInstinct_FearGunshot` | `0x0048FE60` | `sithAIInstinct_RandomMove` | `0x004921B0` |
| `sithAIInstinct_Hop` | `0x0048FFD0` | `sithAIInstinct_HumanCombatMove` | `0x004924D0` |
| `sithAIInstinct_HoverDrift` | `0x00490080` | `sithAIInstinct_Roam` | `0x00492E50` |
| `sithAIInstinct_Talk` | `0x004901F0` | `sithAIInstinct_ReturnHome` | `0x00493190` |
| `sithAIInstinct_RandomTurn` | `0x00490510` | `sithAIInstinct_Flee` | `0x00493240` |
| `sithAIInstinct_OpenDoors` | `0x00490640` | `sithAIInstinct_BasicFallow` | `0x004935D0` |
| `sithAIInstinct_Jump` | `0x00490670` | `sithAIInstinct_WallCrawl` | `0x00494080` |
| `sithAIInstinct_LobFire` | `0x00490950` | `sithAIInstinct_SnakeFindMovePos` | `0x00494360` |
| `sithAIInstinct_PrimaryFire` | `0x00490B00` | `sithAIInstinct_SnakeMungeTestCheck` | `0x00494600` |
| `sithAIInstinct_AlternateFire` | `0x00491020` | `sithAIInstinct_SnakeFollow` | `0x004948D0` |
| `sithAIInstinct_CircleStrafe` | `0x004912D0` | `sithAIInstinct_SnakePlaySlitherSound` | `0x00494FF0` |

File statics of `sithAIInstinct.c`: `sithAIInstinct_snakeLastDotFwd` `0x00539AB8`, `sithAIInstinct_snakeSlitherDir`
`0x00585464`, `sithAIInstinct_snakeFollowNearRange` `0x00585468` (never written by the original).

### Setup

- A case builds one AI control block (thing 0, the AI actor, in sector 0), the class (sight distance 10, heard
  distance 8, fov -1 unless given), the things the events refer to (roles `PLAYER`, `TARGET`, `GOAL`, `FLEE`, `OTHER`,
  `SRC` for a weapon in flight and `PARENT` for its shooter), the instinct with its 24 float and int arguments (both
  arrays hold the same class file value, `.args`), the instinct state (`.st`, `.stFlags`, `.stNext`), the awareness
  sector of the actor, the game time (100000 ms and 100 s unless given), the sith mode, the local player, the Quetz block
  of the snake and the three snake statics. Names of the argument and state slots are the `SITHAIINSTINCT_*` getters
  of `sithAIInstinct.c`; each group of cases states the meaning of the slots it sets.
- The first `rand()` results of a case are chosen by bucket (`.rnd`, for example `"LH"`: a low result, then a high one,
  `L` < 0.15, `M` 0.4 to 0.6, `m` 0.52 to 0.78, `S`/`H` > 0.82/0.85, `l`/`h` below/above 0.5, and narrow windows
  just beside the thresholds of the code such as 0.1, 0.25, 0.4, 0.5, 0.6 and 0.8, see `sithAIFnVec_RndMatches`): the runner searches the seed
  of the deterministic `rand` stub that produces them. The stub replaces the original's `rand` (patched at `0x004F7010`).
- Every engine call the instincts make is replaced by a stub that records the call with its arguments and returns what
  the case configured: `sithAIMove_*` (look, move, jump, stop, special move), `sithAIUtil_*` (mode changes, sight
  checks, distances, path and roam positions, fire, waypoints, floor and position checks), `sithAI_*` (instinct
  lookup, enable, force update, event emission), `sithAIAwareness_*`, sounds (`sithSoundClass_PlayModeRandom/First`,
  `sithSound_Load`, `sithSoundMixer_*`), puppet, physics, `sithThing_GetThingParent/DetachThing/CreateQuetzUserBlock/
  AttachThingToSurface`, `sithPlayerActions_Activate`, `sithWeapon_*`, `sithCollision_FindSectorInRadius`,
  `rdVector_Normalize3(Acc)`, `rdVector_Rotate3(Acc)` (rotation about z by the yaw), `rdMath_DeltaAngleNormalized`, and
  `sithSector_GetSectorIndex` as a verified lookup (0 mismatches). The stubs are a definition of their own, not under
  test: for example `sithAIUtil_AISetMode` only adds the mode bits and `AIClearMode` removes them, `AISetMovePos`
  stores the position, and the sequences of results are given per case (`SEQ` of four 2 bit results repeated, `ALL(v)`
  for the same result every call; sight states 0 visible, 1 too far, 2 out of fov, 3 hidden). Sight checks that only
  update fields of the control block (`UpdateTargetSight`, `UpdateGoalSight`) leave them as the case set them.
  The helpers of `sithAIInstinct.c` itself run unchanged on both sides: `BasicFallow` (called by `WallCrawl`),
  `SnakeFindMovePos`, `SnakeMungeTestCheck` and `SnakePlaySlitherSound` (called by `SnakeFollow`).
- Watched: the whole arena (control blocks, things, instinct, state, awareness sectors, surfaces, Quetz block) and
  mirrors of the three snake statics. A case with several calls (`.calls`, `.dtMs` ms between them) records the stubs
  and the changes after every call; the stub result sequences and the snake statics carry over between the calls.
- Environment: as for the other world-state suites (MSVC 2026 x86 harness under Wine, x87 precision 24 bit).

### Cases

`sithAIInstinctFn.tv` (1022 cases; each case runs twice in the capture with the unused tail of every arena array
filled with `0xA5` and `0x5A` and both runs agree):

| Function | Cases | Function | Cases |
| --- | --- | --- | --- |
| `Listen` | 60 | `Dodge` | 73 |
| `LookForTarget` | 44 | `HitAndRun` | 20 |
| `SenseDanger` | 26 | `Retreat` | 23 |
| `FearGunshot` | 22 | `RandomMove` | 32 |
| `Hop` | 9 | `HumanCombatMove` | 81 |
| `HoverDrift` | 10 | `Roam` | 40 |
| `Talk` | 55 | `ReturnHome` | 8 |
| `RandomTurn` | 11 | `Flee` | 45 |
| `OpenDoors` | 4 | `BasicFallow` | 114 |
| `Jump` | 28 | `WallCrawl` | 47 |
| `LobFire` | 14 | `SnakeFollow` | 70 |
| `PrimaryFire` | 55 | `SnakeFindMovePos` | 17 |
| `AlternateFire` | 25 | `SnakeMungeTestCheck` | 19 |
| `CircleStrafe` | 53 | `SnakePlaySlitherSound` | 17 |

What the groups cover (see the comment above each group in `sithAIInstinctFnVecCases.inc`):

- Events: every event the function reacts to (HIT_SECTOR, SOUND, TOUCHED, MODECHANGED, TALK, FIRE, TARGETED,
  HIT_WALL, HIT_CLIFF, HIT_THING, HIT_FLOOR, LAND_FLOOR, GOAL_REACHED, GOAL_UNREACHABLE and the periodic update, event 0),
  and unhandled events.
- Modes and submodes that gate the function (searching, active, attacking, moving, fleeing, dodging, berserk, circle
  strafing, hunting, wall crawling, the unknown 0x1000000 and 0x400000 modes, continuous motion, waypoint modes) and the
  modes the function sets and clears (recorded through the `AISetMode`/`AIClearMode` stubs and the direct changes of
  `mode` and `submode`).
- Targets visible, too far, out of the field of view and hidden; the target or goal missing, not attackable, dying,
  destroyed, invisible, a vehicle; the actor blind, deaf, human, flying, immobile, submerged; underwater sectors.
- Timers expired, not yet expired and exactly at the boundary (cooldowns, attack time, next jump time, panic time, hunt
  time, move delays); thresholds of distances, health fractions, chances and dots below, at and above the limit.
- Arguments zero (defaults) and set, several `rand()` outcomes per branch, state parameters in their possible states,
  and multi-call sequences in which the state evolves.

Coverage: with a temporary instrumented build of `sithAIInstinct.c` (a probe at the start of every `if`, `else`, `case`,
loop and label block of the 28 functions, not committed) every block is executed by at least one case except four: the
two hunting blocks of `Listen` excluded below, `Listen`'s `if ( !pSectorInfo )` (the pointer is computed from an array
index and is never NULL) and the "no LOS to start point" error of `SnakeMungeTestCheck` (its delta is always zero, the end
position is a copy of the start position). Mutating thresholds, constants and conditions at more than 50 places of the source made
the test of the mutated function fail every time (a mutation run is not part of the test).

### Fixed-Behavior Regression Tests

The separate `sithAIInstinctFix` group in `sithAIInstinctFnVecTest.c` checks the
corrected contracts without changing the original captures:

- RandomTurn's visible path must return 1 and pass the sight-tested point to the
  eye-level look setter; non-searching and non-visible results are checked too.
- Berserk PrimaryFire bursts 6 and -6 reach AIFire unchanged without division by
  zero; neighboring authored counts 5 and -5 still randomize to 25 and -25.
- QOL Listen hearing a valid source in HUNTING or INSTINCTUSEWPNTS mode uses its
  transmitted noise level, retains a louder prior level, and performs the
  expected sound/mode/waypoint callbacks in order.
- QOL SnakeMungeTestCheck passes the complete, unmasked four-flag path mask and
  the expected geometric arguments/output. Extra flag bits fail this assertion.

The undefined Listen and snake paths are not executed by these fixed-behavior
checks in legacy builds. Original fixture masking and exclusions below still
apply solely to original-engine comparisons. The separate registry executable
also tests the real `sithAI_InstinctUpdate` caller: a handled callback restarts
its scan so an earlier, newly enabled instinct runs before later instincts.

### Inputs Kept Out Of The Corpus

- `Listen`, `SOUND` event: the branch in which a hunting actor (mode HUNTING or INSTINCTUSEWPNTS) hears a source thing that
  exists, is within hearing range and passes the hear chance (the `bHunt` path). The original never sets the local
  `loudness` on it and compares and stores it afterwards (`// TODO: [BUG]` in the reconstruction); its value is whatever
  the stack held. Neighbouring paths are covered: hunting with no source or a freed source thing, and not hunting.
- `SnakeMungeTestCheck`: the path flags passed to `sithAIUtil_CheckPathToPos` start from an uninitialized local in the
  original (`// TODO: [BUG]`). Instead of dropping these paths the `CheckPathToPos` stub records only the four bits that
  are defined (`IGNOREPLAYER`, `IGNORETHINGS`, `GOALVISIBLE`, `STRAIGHT`, `pathFlagsMask` of the cases using it) and
  ignores the rest, so the undefined bits cannot influence the result; every other argument and result of the call is
  compared. This applies to `SnakeMungeTestCheck`, `SnakeFindMovePos` and `SnakeFollow`.
- `RandomTurn`: the function has no `return` at its end and returns the sight state (1 to 3, covered) when the look
  point is not visible, but leftover register contents when it is visible (`// TODO: [BUG]`). The return value of the
  visible cases is not recorded (`noRet`); everything else about them is compared.
- NaN: a health of 0 with a maximum health of 0 gives a NaN health fraction, which the original (x87 compares) and the
  reconstruction (SSE) order differently. The health cases use maximum health 0 with a positive health (infinite fraction).
- `PrimaryFire` in berserk mode with a burst argument of 6 or -6: `rand() % (6 - abs(burst))` divides by zero.
- Pointers the instincts dereference without a check: the shooter returned by `sithThing_GetThingParent` for TOUCHED
  and HIT_SECTOR in `Listen` (on the investigating paths) and for Dodge's weapon events, the thing of a TOUCHED event in
  `SnakeFollow`, the Quetz block when `sithThing_CreateQuetzUserBlock` succeeds, the target of `AlternateFire` and
  `LobFire` when it can be attacked, the target of `LookForTarget` when no player exists and the actor can attack.

### Known Differences

No case differs in its result. One difference of the engine calls is normalized in the harness:

- `SnakeMungeTestCheck` (`0x00494600`) calls `sithPhysics_GetThingHeight` twice in the original (once for the start
  position, once more for an unused position `curFarPos` that the decompilation shows and the reconstruction drops) and
  once in the reconstruction. The function is a pure query with the same result both times, so the call is not recorded
  by the stub; all other engine calls of the function, its result and its output point are compared exactly.

### Provenance And Regeneration

- Capture harness: the generic world-state capture DLL (kept outside the repository, SHA-256 of the source used:
  `5ec8edc75e92213913e56e663aa41d4eb3fb0cb6233153efdb12f51d1cf8fc53`), built with
  `/DSUITE_HEADER="\"sithAIInstinctFnVec.h\"" /DSUITE_SEAMS="\"sithAIInstinctFnVecSeams.def\""` as described in
  [Libs/sith/Tests/Vec/README.md](../../../Tests/Vec/README.md). It patches `rand` and the engine functions of
  `sithAIInstinctFnVecSeams.def`; the instinct functions themselves are not modified.
- Capture log: 1022 cases, 0 unstable, 0 lookup mismatches. Three complete captures in fresh processes produced
  byte-identical files.
- The reconstruction reproduces every case, in Debug and Release for DirectX 6 and DirectX 9. The shared stub of
  `sithCollision_FindSectorInRadius` in `sithVecWorld.h` gained an optional NULL result from a given call on
  (`sithVec_stubFindSectorNullFrom`), used by the `Jump` and `SnakeMungeTestCheck` cases.

### SHA-256

```text
9ad0793ed232c1850e368dd7b90996f0394ff71fd97467a6281b48f45502a6a5  sithAIInstinctFn.tv
```
