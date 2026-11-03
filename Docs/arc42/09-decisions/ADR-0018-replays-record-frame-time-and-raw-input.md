# ADR-0018: Replays record frame time and raw input per frame

- **Status:** Accepted
- **Date:** 2026-11-03

## Context

Replays serve trace runs, bug reports and recorded runs. Doom source ports record a ticcmd per simulation step:
a small struct of logical actions such as "move forward" ([research](../../Research/Engine-Patterns.md)).

Jones3D differs in two ways:

- `stdControl_ReadControls()` samples devices once per frame, and `sithControl` pulls key and axis values and
  resolves the bindings itself. A logical command record would have to be built inside `sithControl`, which
  is game logic.
- The simulation step is variable: `sithTime_Advance()` uses the measured frame time. `stdControl_ReadKeyAsAxis`
  also scales a held key by the frame's read time. Input alone doesn't reproduce a run.

The headless runner's `--input` script already drives input per frame at the key level.

## Decision

- A replay is a sequence of frames. Each frame records the frame time and the raw key and axis state the game
  reads.
- Recording and playback sit at the `stdControl` and time boundary, in the adapter. Game logic doesn't change.
- Playback feeds both through Babylon's Script input source and the Null clock.
- The replay file is an on-disk format: versioned, written and read through explicit serialization, and
  validated on load.

## Consequences

- A replay reproduces a run exactly on any machine, given the same build profile and simulation cvars.
- Replays don't survive a change of the binding layer or the input read path; the format version catches this.
- Replays are larger than Doom demos: frame time plus every key and axis per frame. Run-length or change-only
  encoding can shrink them later without changing the content.
- A later fixed simulation step ([chapter 11](../11-risks-and-technical-debt.md)) would need a new replay
  format version.
