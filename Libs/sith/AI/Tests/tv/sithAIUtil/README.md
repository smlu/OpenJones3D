## sithAIUtil Original-Engine Test Vectors

`sithAIUtil.tv` holds, for every case of `sithAIUtilVec_aCases` (`Libs/sith/AI/Tests/sithAIUtilVec.h`), the complete
observable result of the original `sithAIUtil` functions: return values, out parameters, the changes of the file
statics of `sithAIUtil.c` (waypoint tables, point creation state, things-in-view state), of the AI control blocks,
of the arena things and sectors, and every engine call the function made (collision searches, line of sight, weapon,
cog, AI move and sound calls). It was captured by executing the functions inside the retail `Indy3D.exe` with the
world-state vector harness ([Libs/sith/Tests/Vec](../../../Tests/Vec/README.md)). The fixture is consumed by
`sithAIUtilVecTest.c` (`sithAIUtilVecTests`, CTest `sith.AIUtil`), which never launches `Indy3D.exe`.

### Reference

| Item | Value |
| --- | --- |
| Executable | Retail v1.0 `Indy3D.exe`, SHA-256 `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` (verified before capture) |
| Debug build | Not captured; the v1.0 entry points below were used |
| Calling convention | `__cdecl` |

Entry points (v1.0): `sithAIUtil_UpdateTargetSight` `0x0049B2E0`, `UpdateGoalSight` `0x0049B640`, `GetDistanceToTarget`
`0x0049B7C0`, `GetDistanceToTargetPos` `0x0049BA60`, `AIFire` `0x0049BC30`, `AIPauseMove` `0x0049C250`,
`AIResetWaypoints` `0x0049C270`, `AISetWpnt` `0x0049C420`, `AIGetWpntThing` `0x0049C4A0`, `AISetWpntRank` `0x0049C4D0`,
`AISetWpntFlags` `0x0049C530`, `AIClearWpntFlags` `0x0049C5B0`, `AISetActiveWpntLayer` `0x0049C620`, `AIConnectWpnts`
`0x0049C660`, `AIMoveToNextWpnt` `0x0049C7B0`, `AITraverseWpnts` `0x0049C800`, `AIAdvanceToNextWpnt` `0x0049C9F0`,
`SelectWpntLink` `0x0049CC60`, `CheckWpntMoveAlign` `0x0049D170`, `AIFindNearestWpnt` `0x0049D2D0`,
`RankWpntsFromPlayer` `0x0049D640`, `RankWpntsFromThing` `0x0049D750`, `CreateAIWaypoint` `0x0049D860`,
`ClearAIWaypoint` `0x0049D910`, `GetAIWaypointNum` `0x0049D950`, `CompareAIWpntDistances` `0x0049D980`,
`RenderAIWaypoints` `0x0049D9C0`, `CheckPathToPoint` `0x0049DB50`, `CheckPosition` `0x0049DC90`, `CheckFloorAtPos`
`0x0049DD20`, `MakeRandPoint` `0x0049E040`, `RetryMakeRandPoint` `0x0049E5F0`, `CheckPathToPos` `0x0049E790`,
`SweepMoveToPathPos` `0x0049EE50`, `MakeRoamPos` `0x0049F010`, `MakePathPos` `0x0049F110`, `IsLineOfFireClear`
`0x0049F1F0`, `AIPlaySoundMode` `0x0049F330`, `AISetMode` `0x0049F390`, `AIClearMode` `0x0049F4A0`,
`SetWeaponFireFlags` `0x0049F570`, `AIGetMovePos` `0x0049F5C0`, `GetXYHeadingVector` `0x0049F700`,
`GetXYZHeadingVector` `0x0049F830`, `CanAttackTarget` `0x0049F990`, `IsThingMoving` `0x0049F9C0`, `ApplyForce`
`0x0049FAA0`, `RandomRotate` `0x0049FB00`, `GetThingsInView` `0x0049FB80`, `GetNextThingsInView` `0x0049FC80`,
`CanSeeTarget` `0x0049FF20`.

File statics reached at their v1.0 addresses: the waypoint table `0x00585730` (60 entries), the waypoint owner table
`0x005854B0` (10 entries), the sort scratch `0x00585530`, the active layer `0x00585484`, the render timestamp
`0x00586270`, the point creation state (`0x00586274`, `0x0058549C`, `0x00585490`, `0x005854A0`, `0x00585718`) and the
things-in-view state (`0x00585478` to `0x00585728`). Engine globals: `sithMain_g_frameNumber`, `sithMain_g_curRenderTick`,
`sithMain_g_sith_mode`, `sithTime_g_msecGameTime`, `sithTime_g_secGameTime`, `sithTime_g_frameTimeFlex`,
`sithPlayer_g_pLocalPlayerThing`, `sithAI_g_aControlBlocks`, `sithAI_g_lastUsedAIIndex`, `sithAIUtil_g_bRenderAIWpnts`
(redirected to static storage in the tests, `sithAIUtilVecAddrs.h`).

### Setup

- Thing 0 is the AI actor (control block 0 of `sithAI_g_aControlBlocks`, class 0 with sight distance 20, field of
  view 0.5, step height 0.25, accuracy 0.5, turn alignment 30), thing 1 the target, thing 2 the goal, thing 3 the
  flee-from thing; waypoint ghosts are things 10 and up. The waypoint graphs are a chain, a star, a ring with a tail,
  two components, a one-way chain, and variants with a disabled waypoint, a waypoint of another layer, and a
  waypoint of high rank. The first 8 control blocks are watched and filled with the run's garbage fill.
- The statics are reset before every case (waypoint table as after `AIResetWaypoints`, everything else zero), so the
  cases do not depend on each other. A mirror of the statics, the render tick and the local player is watched.
- Engine calls the functions make are replaced by stubs (`sithAIUtilVecSeams.def`) that record their arguments
  as events. Results are configured per case: scripted collision searches (the collisions a search returns to the
  popping loop of the function, with surface, adjoin, thing and distance), line of sight results by call number,
  weapon, aiming, mounting and inventory type results, the result of `sithAIMove_AISetMovePos` by call number. The
  math helpers (`rdVector_Normalize3`, `rdVector_Normalize3Acc`, `rdVector_Rotate3`, `rdVector_Rotate3Acc`,
  `rdMatrix_TransformVector34`, `stdMath_SinCos`) are the shared stubs, the intensity of a light is the average of its
  colors. `rand` is the harness stub; cases that depend on the random numbers set its seed (see Cases).
  The functions of `sithAIUtil.c` that call each other run unchanged on both sides, as do `qsort` (the order of
  equal waypoint distances is not defined, so no case has equal distances) and the inline vector helpers.
- Floating point: x87 precision 24 bit like the other world-state suites.

### Cases

`sithAIUtil.tv` (1700 cases). Names are `<Function>_<state>`, edge cases `<Function>_edge_<state>`, sweeps
`<Function>_sweep<what>_<state>`.

| Function | Cases | Function | Cases |
| --- | --- | --- | --- |
| `SetWeaponFireFlags` | 11 | `CheckPathToPoint` | 19 |
| `CanAttackTarget` | 12 | `CheckFloorAtPos` | 51 |
| `IsThingMoving` | 18 | `CheckPosition` | 15 |
| `AIPauseMove` | 7 | `AIFindNearestWpnt` | 38 |
| `AISetMode` | 75 | `AIMoveToNextWpnt` | 10 |
| `AIClearMode` | 75 | `AITraverseWpnts` | 21 |
| `AIGetMovePos` | 16 | `AIAdvanceToNextWpnt` | 25 |
| `GetXYHeadingVector` | 13 | `CheckWpntMoveAlign` | 14 |
| `GetXYZHeadingVector` | 13 | `RenderAIWaypoints` | 13 |
| `ApplyForce` | 5 | `GetDistanceToTarget` | 135 |
| `RandomRotate` | 6 | `GetDistanceToTargetPos` | 135 |
| `AIPlaySoundMode` | 12 | `CanSeeTarget` | 104 |
| `AISetWpnt` | 9 | `UpdateTargetSight` | 36 |
| `AISetWpntRank` | 20 | `UpdateGoalSight` | 36 |
| `AISetWpntFlags` | 20 | `IsLineOfFireClear` | 27 |
| `AIClearWpntFlags` | 20 | `AIFire` | 94 |
| `AISetActiveWpntLayer` | 11 | `GetThingsInView` | 52 |
| `AIConnectWpnts` | 16 | `GetNextThingsInView` | 11 |
| `CreateAIWaypoint` | 13 | `MakeRandPoint` | 65 |
| `ClearAIWaypoint` | 13 | `RetryMakeRandPoint` | 30 |
| `GetAIWaypointNum` | 13 | `CheckPathToPos` | 63 |
| `CompareAIWpntDistances` | 10 | `SweepMoveToPathPos` | 15 |
| `AIGetWpntThing` | 5 | `MakeRoamPos` | 14 |
| `RankWpntsFromThing` | 20 | `MakePathPos` | 21 |
| `RankWpntsFromPlayer` | 13 | `AIResetWaypoints` | 2 |
| `SelectWpntLink` | 208 |  |  |

- Waypoint tables: reset (dirty tables, render flag), `AISetWpnt` (valid, last, out of range, wrong thing types),
  rank and flag setters (clamping, range, masks), active layer (valid and invalid layer flags), `AIConnectWpnts`
  (full link lists, invalid waypoints, one and both directions), owner table (`CreateAIWaypoint`, `ClearAIWaypoint`,
  `GetAIWaypointNum` with free, occupied and matching slots, turn angle clamps), distance comparison, ranking from a
  thing and from the local player on every graph.
- `SelectWpntLink`: plain priority selection, attacking and searching (back-track penalty, random flip), hunting
  (ranks from the player), fleeing (maximum rank, facing the flee-from thing, rank steps) and fleeing to a waypoint
  (ranks from a waypoint ghost). Every case also runs three sweeps: the first random number over 16 values, the
  priority and the rank of the second candidate from 0 to 30 against 15 of the lead, so each penalty and bonus
  shows at which value the decision changes.
- `AIFindNearestWpnt`, `AIMoveToNextWpnt`, `AITraverseWpnts`, `AIAdvanceToNextWpnt`, `CheckWpntMoveAlign`,
  `RenderAIWaypoints`: waypoint reached, nearest by distance with and without path checks (blocked, floors ignored,
  all blocked), disabled and other layer waypoints, invalid waypoints, local player, links gathered by path check,
  owner table full, mode and turn angle arguments, move position accepted or refused, end of chain,
  continuous motion flags, render timing.
- Sight: `GetDistanceToTarget` and `GetDistanceToTargetPos` (invisible, blind, submerged, too far, field of view
  of both signs, line of sight) each also with a sweep of the target around the viewer in 15 degree steps and
  with the target distance from 17 to 22.75; `CanSeeTarget` (close, far, dark, headlight, crouching, still,
  invisible, blind) each also with the first random number over 0..32767 in steps of 128, which shows the chance of
  being seen; `UpdateTargetSight`, `UpdateGoalSight`, `IsLineOfFireClear`.
- `AIFire`: gates (AI disabled, dying, no target, underwater), waiting, sight, distance and field of view limits,
  weapon flags, delayed fire, sight update, lead and lob, aim error, tail attack, weapon template and the weapon
  cog of human weapons.
- Path checks: `CheckPathToPoint`, `CheckFloorAtPos`, `CheckPosition` (surfaces, adjoins, sectors, things, attached
  states), `MakeRandPoint` and `RetryMakeRandPoint` (all flag combinations, retries, stale state),
  `CheckPathToPos` (straight and sides), `SweepMoveToPathPos`, `MakeRoamPos`, `MakePathPos`.
- `GetThingsInView` and `GetNextThingsInView`: field of view, size limit, type mask, distance limit, adjoin
  flags, face kinds, visited sectors, search limits.
- Seeds: cases that expose the random numbers as floats use seeds whose numbers are exact in both
  `SITH_RAND` forms (see Known Differences); the sweeps only record decisions.

### Inputs Kept Out Of The Corpus

- `SelectWpntLink` while fleeing to a waypoint without a flee-from thing: `RankWpntsFromThing(NULL)` dereferences
  NULL (the original and the reconstruction).
- `CheckPathToPos` with `SIDES` and a minimum distance not below the maximum distance: the end positions of both
  sides are never written and the function returns uninitialized stack values.
- `MakeRandPoint` with a reference flag that finds no position (`AWAYFROMFLEE` without a flee-from thing): the
  original leaves the reference position uninitialized (`// TODO: [BUG]` in the reconstruction).
- Waypoints that hold the thing number -1 in `AITraverseWpnts` (the thing before the first one is read).
- Things without an AI control block in `AIFindNearestWpnt` (the control block of the thing is dereferenced).
- `sithAIUtil_SyncWpnts` and `sithAIUtil_ProcessSyncWpnts` are not covered: they only serialize the waypoint and owner
  tables into a multiplayer message and back, without branches (they need the message buffer of `sithMulti`).

### Known Differences

- `MakeRandPoint_edge_rand_ulp`, `MakeRoamPos_edge_rand_ulp`: `SITH_RAND()` (`Libs/sith/Main/sithMain.h`) is
  `(double)rand() * (1.0 / RAND_MAX)` in the reconstruction and `(float)rand() * (1.0f / RAND_MAX)` in the original
  (`fild`, `fmul dword ptr [0x005079E8]`, for example at `0x0049FB0F`), so a random number that is used as a float
  differs by one unit in the last place for some `rand()` values (rand = 1658 gives 0.050599687 in the original and
  0.05059969 in the reconstruction, times 60 degrees 3.0359812 against 3.0359814). Comparisons with a random number
  are unaffected except in the last bit. The two cases use the seed that exposes it (a raw seed) and
  `TestOriginalMakeRandPoint` and `TestOriginalMakeRoamPos` expect exactly these cases to differ (reported as
  ignored); all other cases use seeds whose numbers are exact in both forms. The implementation and the macro are
  unchanged.

### Provenance And Regeneration

- Capture harness: the generic world-state capture DLL (kept outside the repository, SHA-256 of the source used:
  `5ec8edc75e92213913e56e663aa41d4eb3fb0cb6233153efdb12f51d1cf8fc53`), built with
  `/DSUITE_HEADER="\"sithAIUtilVec.h\"" /DSUITE_SEAMS="\"sithAIUtilVecSeams.def\""` as described in
  [Libs/sith/Tests/Vec/README.md](../../../Tests/Vec/README.md). It patches the seam functions listed in
  `sithAIUtilVecSeams.def` and `rand`; no other function of the executable is modified.
- Capture log: 1700 cases, 0 unstable, 0 lookup mismatches. Two complete captures in fresh processes produced
  byte-identical files.
- The reconstruction reproduces every case except the known differences, in Debug and Release for DirectX 6 and
  DirectX 9.

### SHA-256

```text
b8ab41ecdd289a809fdbb3e99b55f858ffdaf8ce94aa063f2b6d8eb50bda8014  sithAIUtil.tv
```
