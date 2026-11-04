# Time and randomness sources in the simulation

Report for M5.14, 2026-11-04. It lists everything outside the simulation's own state that can reach it: wall
clock and frame time, the calendar, and random numbers. It answers the risk in
[chapter 11](../arc42/11-risks-and-technical-debt.md) about frame-time dependence and feeds the later decisions
on a fixed simulation step (M9) and on determinism across platforms.

Method: a search of `Libs/sith`, `Libs/sound`, `Libs/rdroid` and `Jones3D` (excluding tests and the Win32,
DX6 and DX9 backends) for `stdPlatform_GetTimeMsec`, `stdPlatform_GetLocalTime`, `SoundDriver_GetTimeMsec`,
`time()`, `clock()`, the Windows timers and `rand()`, then a reading of each site.

## Summary

| Source | Reaches the simulation? | Deterministic in headless runs? | Same as retail Windows? |
|---|---|---|---|
| Frame time (`sithTime_Advance`) | Yes, by design: every system steps by it | Yes, on the virtual clock | Only with the same frame times |
| Calendar (`GetSysDate`, `GetSysTime` COG verbs) | Yes, if a COG script uses them | **No**: the real date and time | No |
| Voice and subtitle timing (`sithVoice`) | Subtitles only, as far as read | No (real clock) | n/a |
| Sound clock (`Sound.c`, `SoundDriver_GetTimeMsec`) | Not directly; see open points | Null driver: yes | n/a |
| `rand()` | Yes: AI, COG, physics, impact sounds, multiplayer spawns | Yes (no `srand`, fixed start) | **No**: the native C runtime's `rand` is a different generator |
| Display, HUD, console, overlay map timers | No: presentation | n/a | n/a |
| Level load timing (`sithWorld`) | No: log output only | n/a | n/a |

## Frame time

`sithTime_Advance` (`Libs/sith/Gameplay/sithTime.c:37`) reads `stdPlatform_GetTimeMsec()` once per frame,
clamps the difference to 1 ms .. `SITHTIME_MAXFRAMETIME` and derives the frame and game time from it. Pause,
resume and reset (`:71`, `:80`, `:89`, `:107`, `:117`) read the clock too. Everything in the simulation advances
by this value except player physics, which uses fixed slices with the QOL changes on. This is the frame-time
dependence chapter 11 names: a fixed simulation step (M9) has to replace this one value, and the trace shows
whether it does so without changing behavior.

The headless runners latch the virtual clock before startup and advance it by `--step` milliseconds per
frame and by 1 ms per present, so this source is deterministic there. In the Babylon column the same rules
run on `babylon::time::NullClock` (M5.9).

`JonesMain_frameTime` (`Jones3D/Main/JonesMain.c:1092`) and the frame limiter in `JonesDisplay.c:460-497`
(a busy wait on the clock) only pace presentation.

## Calendar: the `GetSysDate` and `GetSysTime` COG verbs

`sithCogFunction_GetSysDate` and `sithCogFunction_GetSysTime` (`Libs/sith/Cog/sithCogFunction.c:94`, `:106`)
push the local date and time to the calling COG script. Whether a shipped level script uses them needs a
search of the COG scripts in the game data (not in the repository). If one does, a run's result depends on
the calendar.

Proposal: route both verbs through a clock the run controls: the wall clock in normal play, a fixed date in
headless runs and replays (a cvar, written into the state file). Low priority until a script is found that
uses them.

## Voice and subtitle timing

`sithVoice` (`Libs/sith/World/sithVoice.c:582-814`) stamps and expires subtitle lines with the real clock.
From the code read so far it only decides which subtitle text is shown. Open point: confirm that no COG
message or game state depends on a subtitle's expiry.

## Sound clock

`Sound.c` keeps its own elapsed time from `SoundDriver_GetTimeMsec()` (`:383` to `:3041`), for fades and
pause bookkeeping. The Null sound driver's clock is deterministic. Open point: whether any simulation code
waits on a sound's end or position (COG verbs that query a playing sound, voice lines that gate scripts),
which would make the sound clock a simulation input once a real driver (miniaudio, M7) plays at real speed.

## Random numbers

The game calls the C runtime's `rand()` and never `srand()`, so every run starts from the runtime's default
seed:

| Site | Use |
|---|---|
| `Libs/sith/AI/sithAIInstinct.c:1476-1590, 2626, 2638` | Weapon burst lengths, weapon choice, AI pause lengths |
| `Libs/sith/Cog/sithCogFunction.c:1571` | The COG `rand` verb: `min + rand() % (max - min + 1)` |
| `Libs/sith/Engine/sithPhysics.c:5164` | Jeep bump chance: `rand() * (1.0f / 32767.0f) < chance` |
| `Libs/sith/World/sithSurface.c:961, 996`, `sithThing.c:2302` | Ricochet sound choice |
| `Libs/sith/Dss/sithMulti.c:767` | Multiplayer spawn point: `rand() * (1.0 / RAND_MAX) * numFound` |
| `Jones3D/Display/JonesHud.c:3264` | HUD sound choice (presentation) |

The original links Microsoft's runtime statically: `rand` is the linear congruential generator
`seed = seed * 214013 + 2531011; return (seed >> 16) & 0x7FFF`, seed 1 at start, `RAND_MAX` 32767. The sith
vector tests already reproduce exactly that sequence (`sithVec_rand` in `Libs/sith/Tests/Vec/sithVecWorld.h`).

Natively the game uses glibc's `rand`, a different generator with `RAND_MAX` 2147483647. Two consequences:

1. **Native runs can't match retail.** Every AI burst, COG `rand` result and ricochet differs from the
   Windows build from the first call on. Runs stay deterministic among themselves, which is why the M4 and M5
   traces agree with each other.
2. **A native bug.** `sithPhysics.c:5164` scales by the literal `1/32767`, which assumes Microsoft's
   `RAND_MAX`. With glibc's values the product is almost always far above 1, so the jeep bump practically never
   happens natively.

Fix (pure bug fix, always on): `j3dcrt`, the shim that already gives non-Windows builds Microsoft's documented
CRT behavior, gets Microsoft's `rand`, `srand` and `RAND_MAX`. Windows builds are unchanged. Native traces
change wherever `rand` is used and should then match a Windows run of the same input, which is the next
cross-check to run. A separate, later question is whether presentation code (the HUD sound choice, future
weather effects) should draw from its own generator, so that adding a visual effect never shifts the
simulation's sequence (see [Enhancements.md](../Enhancements.md)).

## Not sources

`sithWorld.c` (`:505` to `:1573`) times level loading for its log lines; the bare calls at `:1098` and
`:1106` discard their result. `sithOverlayMap.c:841` blinks map icons, `JonesConsole.c` expires console lines,
`JonesHud.c` times HUD effects and credits: presentation only. `sithMulti.c:313` stamps multiplayer
synchronization, outside single-player traces.
