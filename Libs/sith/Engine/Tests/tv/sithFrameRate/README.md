## sithFrameRate Original-Engine Test Vectors

`sithFrameRate.tv` holds the result of every frame-rate case of `Libs/sith/Engine/Tests/sithFrameRateVec.h` at 14 frame
rates. Each result was captured by running the original COG timing, event, AI instinct, weapon, item, path mover, exhaust,
actor and player update functions inside the retail `Indy3D.exe` over 3 sec of game time. It is consumed by the `sithFrameRate` group in
`Libs/sith/Engine/Tests/sithFrameRateTest.c`. The tests read only this file; they never launch or depend on `Indy3D.exe`
or game assets.

The vectors pin down how the original engine's COG sleeps, pulses and timers, events, AI instinct ticks, weapon refire and
waits, item touches, surfacing and drowning sounds, weapon awareness events, exhaust smoke, blocked path movers and per-frame
damage depend on the frame rate (issues #10 and #48). The reconstruction must reproduce them wherever it runs the original code. The fixes are
checked against them too: their timing must match the original at 30 FPS, the frame rate the game was made for, and their
damage must be at least the original's.

### Reference

| Item | Value |
| --- | --- |
| Executable | Retail v1.0 `Indy3D.exe` |
| SHA-256 | `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` (verified before capture) |
| Calling convention | `__cdecl` (`J3DAPI`), prototypes from `Libs/sith/RTI/symbols.h` |
| Struct layouts | Original (non-QOL) `SithThing`, `SithSector`, `SithPlayer`, `SithCog`, `SithCogScript` |

| Function | v1.0 address |
| --- | --- |
| `sithCogFunction_Sleep` | `0x004775D0` |
| `sithCogFunction_SetPulse` | `0x00477B90` |
| `sithCogFunctionThing_SetThingPulse` | `0x004700C0` |
| `sithCogFunction_SetTimer` | `0x00477C50` |
| `sithCogFunctionThing_SetThingTimer` | `0x00470160` |
| `sithCog_ProcessCog` | `0x00424B30` |
| `sithCog_UpdateThingTimer` | `0x004224C0` |
| `sithEvent_Startup`, `sithEvent_Reset` | `0x004524C0`, `0x004525A0` |
| `sithEvent_RegisterTask`, `sithEvent_CreateEvent`, `sithEvent_Process` | `0x00452710`, `0x004525C0`, `0x004527D0` |
| `sithAI_InstinctUpdate` | `0x0042A3B0` |
| `sithWeapon_UpdateActorWeapon` | `0x004468B0` |
| `sithWeapon_SetFireWait`, `sithWeapon_SetAimWait`, `sithWeapon_SetMountWait`, `sithWeapon_GetMountWait` | `0x004485A0`, `0x00448610`, `0x00448580`, `0x00448570` |
| `sithWeapon_ActivateWeapon`, `sithWeapon_IsMountingWeapon` | `0x00448660`, `0x00448CB0` |
| `sithItem_PlayerCollisionHandler` | `0x004AEE10` |
| `sithActor_PlayDamageSoundFx` | `0x0048C0B0` |
| `sithWeapon_Update` | `0x00446C00` |
| `sithPathMove_Update`, `sithPathMove_RotatePivot` | `0x004AC540`, `0x004ABF10` |
| `sithPhysics_UpdateExhaustFx` | `0x00483CC0` |
| `sithActor_Update` | `0x0048BAB0` |
| `sithPlayer_Update` | `0x00427700` |

The original functions also run the engine code they call for these cases unpatched. For the pulses that is the message
sends `sithCog_SendMessage` (`0x00422620`) and `sithCog_ThingSendMessage` (`0x00424190`). They reach no handler: the
script has none and the thing has no COG or links.

ABI checks: the harness asserts the offsets of every field these functions access, read from their v1.0 disassembly.

| Struct | Fields and offsets |
| --- | --- |
| `SithThing` | `flags` 0x000, `idx` 0x004, `moveType` 0x010, `moveStatus` 0x018, `msecTimerTime` 0x024, `msecNextPulseTime` 0x028, `msecPulseInterval` 0x02C, `attach.flags` 0x04C, `pInSector` 0x074 |
| `SithThing.thingInfo.actorInfo` | `flags` 0x160, `endurance.msecUnderwater` and `endurance.raftLeakDamage` 0x16C (one union), `pThingMeshAttached` 0x1E0 |
| `SithThing.moveInfo.physics` | `flags` 0x28C |
| `SithSector` | `flags` 0x000 |
| `SithPlayer` | `pThing` 0x6DC |
| `SithCog` | `pScript` 0x000, `flags` 0x004, `status` 0x00C, `statusParams` 0x014, `msecPulseInterval` 0x01C, `msecNextPulseTime` 0x020, `msecTimerTimeout` 0x024, `aName` 0x10A0 |
| `SithAIControlBlock` | `pClass` 0x004, `mode` 0x008, `aInstinctStates` 0x010 (0x18 each, `msecNextUpdate` at +4), `numInstincts` 0x190, `msecNextUpdate` 0x194 |
| `SithAIClass`, `SithAIInstinct` | `aInstincts` 0x034; an instinct is 0xD0 bytes with `updateModes` 0x0, `updateBlockModes` 0x4, `pfInstinct` 0xCC |
| `SithEvent` | 28 bytes, `msecEventTime` 0x0, `params` 0x8 |
| `SithThing.thingInfo.actorInfo.weaponInfo` | `secActivationWaitEndTime` 0x1FC, `secActivationWaitTime` 0x200, `secAimWaitEndTime` 0x204, `secSwapTime` 0x208, `curWeaponID` 0x210 |
| `SithThing.thingInfo.itemInfo` | `msecLastTouchTime` 0x178 |
| `SithInventoryType` | `flags` 0x0, `pCog` 0x8C |
| `SithThing.thingInfo.weaponInfo` (projectile) | `flags` 0x160 |
| `SithThing.moveInfo.pathMovement` | `mode` 0x298, `vecDeltaPos` 0x29C, `moveTimeRemaining` 0x2A8, `moveVel` 0x2AC, `numBlockedMoves` 0x30C |
| `SithWorld` | `aThings` 0x610 |
| `tHostServices` | 112 bytes |

### How The Vectors Are Produced

The harness and the tests compile the same header, `sithFrameRateVec.h`. It holds the cases, the frame model, the stubs and
the runners; only the calls and the stub argument checks are defined by the includer. The harness calls the v1.0 addresses
through `J3D_TRAMPOLINE_CALL`, and the tests call the reconstruction.

- **Frame model.** Frame `n` at `fps` lasts `(n + 1) * 1000 / fps - n * 1000 / fps` msec, so the frames add up to exactly
  3000 msec and e.g. 144 FPS mixes 6 and 7 msec frames. Every frame sets these engine globals the way `sithTime_Advance`
  does: `sithTime_g_frameTime`, `sithTime_g_frameTimeFlex` (msec * 0.001), `sithTime_g_fps` (1 / frameTimeFlex),
  `sithTime_g_msecGameTime` (+= frame time) and `sithMain_g_frameNumber` (the frame index). Game time starts at 0 in every
  case.
- **COG cases.** In frame 0 the script calls `Sleep`, `SetPulse`, `SetThingPulse(thing, interval)`, `SetTimer` or
  `SetThingTimer(thing, interval)`. Every later frame runs `sithCog_ProcessCog` or `sithCog_UpdateThingTimer`. A pulse is
  counted when its next pulse time moves. A woken sleep loop counts the wake and sleeps again: `sithCogExec_Execute` is a
  stub that calls the original `Sleep`. A timer is counted when its flag is cleared, and the runner sets it again in that
  frame, as a script does in its timer message (which the engine sends in the same frame).
- **Event loop.** After the original `sithEvent_Startup` (once) and `sithEvent_Reset`, task 4 is registered with a stub
  that counts the event and creates it again with the same delay, like a script calling `SetTimerEx` in its timer message.
  Frame 0 creates the first event, every later frame runs `sithEvent_Process`.
- **AI tick.** A zeroed AI control block with a class of one instinct (mode and update mode `SITHAI_MODE_ACTIVE`). Every
  frame in which the AI's next update time is reached runs the original `sithAI_InstinctUpdate`, as `sithAI_Update` does.
  The instinct is a stub that counts the tick and, for a non-zero interval, schedules its next update `now + interval`
  as the original instincts do (`AITick_default` keeps the loop's 1 sec default).
- **Weapon refire.** An actor holding the fire key: a zeroed actor with the pistol as current weapon, no weapon selected or
  deselected, a fire wait of the interval and a fire wait end of 0. Every frame runs the original
  `sithWeapon_UpdateActorWeapon`, which fires the weapon script (a weapon type with a script without handlers) and sets the
  next fire wait end. A shot is counted when the fire wait end moves.
- **Weapon wait.** A player with the waits over: frame 0 sets one wait (`SetFireWait`, `SetAimWait`, `SetMountWait` or
  `ActivateWeapon`), every later frame asks the original `sithWeapon_IsMountingWeapon` until the wait is over. The result is
  the game time at which it ended.
- **Item touch.** The player on an item: every frame runs the original `sithItem_PlayerCollisionHandler`, with the item
  always in the player's line of sight. A touch is counted when the item's next touch time moves.
- **Drown gurgle.** Every frame runs the original `sithActor_PlayDamageSoundFx` with drowning damage, as for an actor that
  takes a drowning damage point every frame; the drowning sounds are counted.
- **Weapon noise.** A weapon projectile with the decay-awareness flag: every frame runs the original `sithWeapon_Update`;
  the AI awareness events are counted.
- **Path blocked.** A COG-linked path mover with a move under way that `sithCollision_MoveThing` (a stub) never moves. Every
  frame runs the original `sithPathMove_Update`; the blocked messages are counted (`sithCog_ThingSendMessage` is a stub)
  and the game time of the first one recorded. A world with the mover as its only thing satisfies the thing pointer check.
- **Exhaust.** A vehicle at a steady speed with a small speed change per frame: every frame runs the original
  `sithPhysics_UpdateExhaustFx`; the smoke things it creates are counted (the template, the created thing and its sprite
  size animation are stubs).
- **Path move, path rotate.** A COG-linked path mover with a move of one unit under way (speed 1 / duration), or started on a
  90 degree pivot rotation of the duration by the original `sithPathMove_RotatePivot`. Every frame runs the original
  `sithPathMove_Update` until the arrived message (`sithCog_ThingSendMessage` stub); `sithCollision_MoveThing` moves all the
  way and `sithCollision_ApplyThingOrient` copies the orientation (stubs, the collision system isn't running). The result is
  the game time of the arrival.
- **Actor and player cases.** Every frame runs `sithActor_Update(thing, msec)` or
  `sithPlayer_Update(player, msec * 0.001)` on a zeroed player thing in a zeroed sector:
  - Surfacing: on the water surface, breath at the limit.
  - Drowning: in an underwater sector, out of breath.
  - Raft leak: on a leaking raft.
  - IMP: the IMP fire type, the Aetherium sector flag, the POR flag, the IMP damage scalar and the start state of the case.
  
  The thing is the local player and its own perpetrator.
- **Stubs.** These functions are stubs:
  - `sithThing_DamageThing` records the whole damage points and the frames that dealt any. Like the original, it ignores
    damage ≤ 0.
  - `sithGetGameDifficulty` returns the case's difficulty.
  - `sithGetIMPDamageScalar` returns the case's scalar.
  - `sithSoundClass_PlayModeRandom` counts the breath and gasp sounds.
  - `sithVoice_UpdateLipSync` and `sithWeapon_UpdateActorWeapon` do nothing.
  - `sithCogExec_PopFlex` and `sithCogExec_PopThing` return the interval and the thing.
  - `sithInventory_GetType` returns the weapon type of the refire case, `sithCollision_CheckThingLOS` returns true.
  - The event task and the AI instinct described above are stubs registered or installed through the engine's own tables,
    not patches.
  - `sithWeapon_UpdateActorWeapon` is a stub (the IMP cases don't update the player's weapon) except during the weapon
    refire cases, where the harness restores its original bytes.
  - `sithAIAwareness_CreateTransmittingEvent`, `sithCog_ThingSendMessage` (also the pulse messages of the COG cases, to no
    handler), `sithCollision_MoveThing` (moves nothing in the blocked case, all the way otherwise),
    `sithCollision_ApplyThingOrient`, `sithTemplate_GetTemplate`, `sithThing_CreateThingAtPos` and
    `sithAnimate_StartAnimateSpriteSize` are stubs for the effect and path mover cases.

  In the harness, each original function is patched to jump to its stub. In the tests, `sithFrameRateTestSeams.h` and
  `sithCogTimingTestSeams.h` rename the calls. Each stub checks its pointer and damage type arguments; a failed check fails
  the case.
- **Host services.** The host services of `sith`, `std` and `rdroid` count every print and assert, and any call fails the
  case. None was made.
- **Floating point.** The x87 precision is 24 bits, the precision Direct3D sets when it creates the device. The game runs its
  frames with it, so the original float-to-msec conversions round like float math. With 53 bits some results change, see
  Quirks. The retail floor and ceil helpers (used by the rotation math) restore the control word `stdStartup` saved, so the
  harness stores that word at the same precision; otherwise the cases after a rotation would run with 53 bits.
- **Stability.** Every case is run twice from a freshly initialized state, and both results must be identical. All 540
  were.

### File Format

The file is text, one vector per line, and `#` starts a comment line:

```
<case> <fps> <total> <detail> <state>
```

| Kind | `total` | `detail` | `state` |
| --- | --- | --- | --- |
| COG pulse, thing pulse | pulses fired | pulse interval in msec | last next pulse time |
| COG sleep loop | wakes | first deadline minus the game time (the converted interval) | last deadline |
| COG timer loop, thing timer loop | timer messages | first deadline minus the game time | last deadline |
| Event loop | events | first event time minus the game time | next event time |
| AI tick | instinct runs | next update minus the game time after the first tick | the AI's last next update time |
| Weapon refire | shots | first fire wait end minus the game time, in msec | last fire wait end (float bits) |
| Weapon wait | game time at which the wait ended | the wait end minus the game time, in msec | the wait end (float bits) |
| Item touch | touches | first next touch time minus the game time | last next touch time |
| Drown gurgle, weapon noise, exhaust | sounds / events / puffs | 0 | 0 |
| Path blocked | blocked messages | game time of the first message | blocked move count |
| Path move, path rotate | game time of the arrival | frames until it | blocked move count / rotation fraction bits |
| Surfacing | gasp sounds | breath sounds | `msecUnderwater` at the end |
| Drowning, raft leak | damage points | frames that dealt damage | `msecUnderwater` / `raftLeakDamage` at the end |
| IMP | damage points | frames that dealt damage | IMP state bits (`0x...`, the float follows in a comment) |

### Cases

`sithFrameRate.tv` holds 112 cases, each at 20, 30, 34, 45, 60, 72, 75, 100, 120, 144, 165, 240, 300 and 1000 FPS, so
1568 vectors. The rates cover the designed 30 FPS, the 60–75 Hz and 144–165 Hz monitor ranges, 240 Hz and uncapped rates.

- **`CogPulse`, `CogSleep`, `CogThingPulse`, `CogTimer`, `CogThingTimer`, `Event` (11 each).** Intervals 0.001, 0.01,
  0.025, 0.034, 0.04, 0.05, 0.1, 0.2, 0.5, 0.7 and 1 sec. This covers a sub-frame interval, the intervals between one and two 30 FPS frames, and whole-frame intervals.
  0.034 is within a msec of one 30 FPS frame. 0.01, 0.04 and 0.7 are floats just below the decimal, see Quirks.
- **`AITick_default`, `AITick_0.150`, `_0.250`, `_0.500`, `_0.750`.** The 1 sec default of `sithAI_InstinctUpdate` and the
  intervals the engine's instincts set (150, 250, 500 and 750 msec).
- **`WeaponRefire_0.050`, `_0.100`, `_0.150`, `_0.250`, `_0.500`.** A held weapon with these fire waits.
- **`WeaponFireWait_0.150`, `_0.500`, `WeaponAimWait_0.150`, `WeaponMountWait_0.150`, `WeaponActivate_0.150`,
  `_1.000`.** The waits a weapon script sets once.
- **`ItemTouch`.** The 500 msec between touches of an item.
- **`Surfacing`, `Surfacing_idx5`.** Thing index 0 and 5, the offset of the frame cycles.
- **`DrownGurgle`, `WeaponNoise`, `Exhaust`.** Effects the original ran on every 16th, 8th and every frame.
- **`PathBlocked`.** The blocked messages of a path mover: after 3 blocked frames, then every frame.
- **`PathMove_2.000`, `_0.500`, `PathRotate_1.000`, `_0.250`.** A path mover's move and pivot rotation of that duration: when
  it arrives.
- **`Drowning`, `RaftLeak_easy`, `RaftLeak_hard`.** The easiest and the hardest difficulty.
- **`ImpOverheated_imp2`, `_imp3`, `_imp5`, `_imp2_half`, `_imp5_half`.** IMP2, IMP3 and IMP5 overheated from the start,
  with damage scalar 1 (the hardest difficulty) or 0.5.
- **`ImpOverheats_imp5`, `_half`.** IMP5 from state 170, so it overheats after 1 or 2 sec.
- **`AetheriumOut`, `_half`.** In an Aetherium sector with the stamina run out.
- **`AetheriumRunsOut`, `_half`, `_inPor`.** In an Aetherium sector from state 179, so the stamina runs out after 1 or
  2 sec. The `_inPor` case also has the POR flag set, which the Aetherium sector overrides.
- **`PorRecovers`, `PorRecoversToZero`.** In the POR from state 100 and 10: the stamina recovers by 8 per sec and stops at 0.
- **`PorImp2Firing`, `PorImp5Overheated`.** In the POR while an IMP heats up (no recovery), and while IMP5 is overheated
  (damage).

The original results, for some cases. The number is the total over the 3 sec: ticks for the COG and surfacing cases,
damage points for the damage cases.

| Case | 20 FPS | 30 FPS | 60 FPS | 144 FPS | 300 FPS | 1000 FPS | Designed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `CogPulse_0.010` | 59 | 89 | 179 | 215 | 299 | 299 | |
| `CogSleep_0.010` | 59 | 89 | 179 | 215 | 224 | 272 | |
| `CogSleep_0.100` | 19 | 22 | 25 | 28 | 29 | 29 | |
| `CogTimer_0.050` (re-armed) | 59 | 44 | 59 | 53 | 59 | 59 | |
| `Event_0.100` (re-armed) | 19 | 22 | 25 | 28 | 29 | 29 | |
| `AITick_0.150` | 20 | 18 | 20 | 20 | 20 | 20 | |
| `AITick_default` (1 s) | 3 | 3 | 3 | 3 | 3 | 3 | |
| `WeaponRefire_0.050` | 58 | 45 | 59 | 54 | 60 | 60 | |
| `WeaponRefire_0.150` | 19 | 18 | 20 | 20 | 20 | 20 | |
| `WeaponFireWait_0.150` (msec until over) | 200 | 200 | 166 | 159 | 153 | 151 | |
| `WeaponActivate_1.000` (msec until over) | 1050 | 1033 | 1016 | 1006 | 1003 | 1001 | |
| `ItemTouch` | 6 | 6 | 6 | 6 | 6 | 6 | |
| `DrownGurgle` | 4 | 6 | 12 | 27 | 57 | 188 | |
| `WeaponNoise` | 8 | 12 | 23 | 54 | 113 | 375 | |
| `PathBlocked` (messages) | 58 | 88 | 178 | 430 | 898 | 2998 | |
| `Exhaust` (puffs) | 60 | 90 | 180 | 432 | 900 | 3000 | |
| `PathMove_2.000` (arrival msec) | 2000 | 2000 | 2000 | 2000 | 2000 | 2000 | 2000 |
| `PathRotate_1.000` (arrival msec) | 1000 | 1033 | 1000 | 1000 | 1000 | 1001 | 1000 |
| `Surfacing` (gasps) | 2 | 3 | 5 | 12 | 24 | 79 | |
| `Drowning` | 600 | 540 | 540 | 432 | 0 | 0 | 600 |
| `RaftLeak_easy` | 2460 | 2460 | 2460 | 2160 | 2100 | 0 | 2500 |
| `RaftLeak_hard` | 4980 | 4920 | 4920 | 4728 | 4800 | 3000 | 5000 |
| `ImpOverheated_imp2` | 120 | 90 | 0 | 0 | 0 | 0 | 120 |
| `ImpOverheats_imp5` | 80 | 60 | 0 | 0 | 0 | 0 | 80 |
| `AetheriumOut` | 540 | 480 | 480 | 432 | 0 | 0 | 540 |
| `AetheriumRunsOut` | 369 | 326 | 323 | 289 | 0 | 0 | 360 |

The original COG sleeps, pulses, re-armed timers and events and AI ticks tick once per frame for intervals under a frame,
and up to a third more often than at 30 FPS for longer ones that aren't whole 30 FPS frames (a sleep or event always waits
one frame more than its interval, so even whole-frame sleeps run faster). Whole-frame intervals (0.1, 0.5, 1 sec) are
unchanged. The surfacing sounds count frames. The per-frame damage loses the fraction of every frame's damage.

### How The Tests Use The Vectors

- **`TestOriginalCodeMatchesTheRetailVectors`.** Every case and frame rate that runs the original code in the build must
  reproduce `total`, `detail` and `state` exactly:
  - Legacy builds (QOL off, speedrun forced): every case at every frame rate.
  - QOL builds: the once-per-frame effects (`PathBlocked`, `Exhaust`) at 30 FPS and below, where a cycle of one reference
    frame runs every frame like the original.
  - QOL builds: the COG, event, AI, weapon and item cases at 30 FPS and below. Above that, they tick as at 30 FPS.
  - Speedrun builds: also the damage cases.
  - All builds: the POR and IMP heat-up cases without damage.
- **`TestCogSleepAndPulseTickAsAt30FpsAtHigherFrameRates`** (QOL). Above 30 FPS the ticks of every COG, event, AI, weapon
  refire and item case are within one of the original at 30 FPS (two for the weapon refire, see Quirks), a weapon wait
  ends within a 30 FPS frame of the original's end at 30 FPS, and the pulse interval equals the original's. The 0.034 cases are left out: the original waits one or two
  33/34 msec frames for them depending on the frame.
- **`TestSurfacingSoundsPlayAsOftenAsAt30FpsAtEveryFrameRate`** (QOL). At every frame rate the gasps and breaths, drowning
  sounds, weapon awareness events, exhaust puffs and blocked messages are within one of the original at 30 FPS (the first
  blocked message within a 30 FPS frame), and the breath equals the original's. The once-per-frame effects are compared
  above 30 FPS only.
- **`TestDamageIsAtTheDesignedRateAtEveryFrameRate`** (non-speedrun). At every frame rate:
  - The damage is the designed rate over the damage time. The tolerance is one point, plus a frame of damage and 1% of the
    start time for a damage that starts mid-run.
  - The damage is at least the original's.
  - The state equals the original's.
- **`TestImpStateChangesAtItsRateAtEveryFrameRate`.** The original and reconstructed IMP state are within 0.02 of the
  designed value at every frame rate.
- **`TestPathMoverArrivesOnTimeAtEveryFrameRate`.** The original and reconstructed path mover arrive in the first frame at
  or after the designed duration at every frame rate: the move and rotation code was never frame-rate dependent.

### Quirks

- **x87 precision.** The original converts sleep, pulse and timer intervals to msec in float on the x87. With 24 bits, as in
  the game, 0.01, 0.04 and 0.7 sec become 10, 40 and 700 msec. With 53 bits they become 9, 39 and 699 msec, which changes
  the ticks of those COG cases (the event and AI cases get their interval in msec and aren't affected). With 53 bits, `ImpOverheats_imp5` at 20 and 30 FPS and `_half` at 20 FPS also overheat a frame
  earlier. The vectors use 24 bits. The reconstruction uses SSE float math, which matches them.
- **Thing pulse and timer conversion.** `SetThingPulse` and `SetThingTimer` multiply by the float constant 1000.0f
  (`0x00507630`). The reconstruction used double math, which stored 9 msec for 0.01 sec. These vectors found that, and it
  now converts like the original.
- **Sleep wakes after its deadline.** A sleep wakes when the game time is past its deadline, but a pulse fires at its
  deadline. So a `Sleep(0.01)` loop at 1000 FPS wakes every 11 msec (272 times), while `SetPulse(0.01)` fires every
  10 msec (299 times).
- **Shared breath field.** The raft leak flag is the breath field (`msecUnderwater`/`raftLeakDamage` are one union), so the
  raft cases end with state 1.
- **Weapon times are float seconds.** `sithWeapon` keeps its waits in float seconds and re-arms with `secGameTime + wait`.
  The float sum loses a frame now and then: a 0.1 sec refire fires 29 times in 3 sec at 30 FPS instead of 30. The
  reconstruction's plain path is the same float sum (bit-exact in the vectors); its 30 FPS-frame deadlines above 30 FPS
  don't lose the frame, hence the tolerance of two.
- **Event loop guard.** The event task stops re-creating the event after 100000 events, so a wrong "reached" check (which
  would re-fire the re-created event in the same `sithEvent_Process` loop forever) fails the test instead of hanging it.
- **One-frame cycles at 30 FPS.** Whole-msec frames of 33 and 34 msec don't always cross a 30 FPS reference frame
  boundary (every 33.33 msec), so a time-based cycle of one would skip a third of the frames at exactly 30 FPS.
  `sithTime_IsFrameCycle` runs a cycle of one every frame while frames are at least a reference frame long, which is the
  original behavior there; the `PathBlocked` and `Exhaust` vectors at 20 and 30 FPS check it.
- **Not covered by vectors.** The AI move pause, lob-fire wait, combat-move and jump deadlines are set inside
  `sithAIMove_UpdateMove`, `sithAIUtil_AIFire` and the instinct bodies, which need most of the AI and weapon systems
  running. The AI instinct vector suite (`Libs/sith/AI/Tests`) runs the real jump, combat-move and lob-fire instincts
  against their own retail vectors; its QOL test `TestDeadlinesLandOn30FpsFramesAtHigherFrameRates` re-runs those cases at
  144 FPS and checks that the deadlines they set lie on 30 FPS frame starts within a frame of the 30 FPS ones. The move
  pause is stubbed in that suite and only covered by the deadline function's own test.
- **Crush damage.** `sithCollision_ThingCollisionHandler`'s crush damage now uses `sithThing_DamageThingAtRate` like the
  drowning damage; it has no vector of its own (the collision system is needed to reach it).

### Known Differences

None where the reconstruction runs the original code. The fixed code of QOL and non-speedrun builds differs from the
original by design, and the tests listed above check how.

### Provenance

- **Capture harness.** A temporary C file, kept outside the repository, built with MSVC 2026 (x86, Release, static CRT,
  C11) as `Jones3D.dll`. Its source SHA-256 is `9742728cc5bc621310a39561fdcdc9f6887188a83e3bf19987a081e896b431f6`. It
  includes `sithFrameRateVec.h`, whose SHA-256 at capture was
  `5fd19fbd1edf4f90de20360a65587280cf729d36207ed534918ab67fd6ecb369`.
- **What the harness does.**
  1. Compiles against the generated legacy (`J3D_QOL_IMPROVEMENTS` off) `j3d.h`, so the struct layouts are the original
     ones. It fails to build with QOL on.
  2. Sets the FPU control words that the original floor and ceil helpers read (`0x0183962C`, `0x0183962E`, `0x01839630`)
     as `stdStartup` does, and sets the host services.
  3. Patches the stubbed functions to jump to the stubs, and defines the AI instinct's scheduling as `now + interval`.
  4. Runs every case twice at 24 bits and once more at 53 bits for the Quirks, and writes the fixture.
  5. Patches the entry point of the executable to exit, so the game never starts.

  No other function of the executable is modified.
- **Launcher.** `Jones3D.exe` (this repository's launcher) checks the SHA-256 of `Indy3D.exe`, starts it suspended and
  injects the DLL from its own directory.
- **Regeneration.**
  1. Build the harness as a Win32 DLL named `Jones3D.dll`. Use the include directories of a legacy build tree (its
     `generated` directory, `Libs`, and `Libs/sith/Engine/Tests`). Do not add the DirectX 6 SDK headers; the DirectX 9
     configuration uses the Windows SDK.
  2. Place `Jones3D.dll` next to `Jones3D.exe`.
  3. Run `Jones3D.exe <path to Indy3D.exe>` with that directory as the working directory.
  4. Collect `sithFrameRate.tv` and `sithFrameRate-capture.txt` (the stability, check and 53-bit summary).

  Review regenerated fixtures as behavior changes.
- **Results.** The reconstruction reproduces every vector where it runs the original code: legacy, QOL and QOL+speedrun
  builds, DirectX 6 and DirectX 9, Debug and Release.

### SHA-256

| File | SHA-256 |
| --- | --- |
| `sithFrameRate.tv` (LF line endings, as captured) | `a307f70e4388600e7c13ebfa4caae489f721493b50ca375ef555f9f948e39d8c` |
