# ADR-0003: Original simulation, enhanced presentation

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Dropping binary compatibility removes the old way of proving behavior: hooking a function into the
original executable. Players and speedrunners still expect the original game, and savegames and level
scripts depend on exact behavior. At the same time, a port that only reproduces the original look has little
reason to exist. Kex remasters split it the same way: presentation upgrades on by default with classic
toggles, gameplay close to the original.

## Decision

- **Simulation** (physics, collision, AI, COG, timing of game logic, savegames) keeps retail behavior as
  its reference. Every change to it is a separate option, so retail behavior is always one setting away.
- **Presentation** (renderer, resolution, audio output, UI) defaults to enhanced, with a classic toggle where
  it makes sense.
- Behavior is proven by deterministic headless runs: a savegame, scripted input and a frame count produce a
  state trace, which is compared frame for frame against a reference build. The byte-exact decompilation is
  the tie-breaker when the reference itself is in doubt.

The trace reference is **retail behavior**. OpenJones3D's QOL improvements are kept and **switched on by
default**. Those that change the simulation (for example player physics at 150 Hz instead of 50 Hz, larger
thing and light collection distances, breadth-first visibility traversal) become individual runtime options
instead of one compile-time profile, so a player or speedrunner can return to retail behavior. Pure bug and
crash fixes stay on unconditionally. New simulation changes are opt-in. Traces run against the retail
profile and against the default profile.

## Consequences

- Traces only look at simulation state, never at pixels or sound, so the renderer and audio are free to change.
- The simulation must stay deterministic: no wall-clock time, thread timing or uninitialized memory in it.
- Every simulation change needs an option and must keep the retail trace identical when the option is off.
- `JONES3D_QOL_IMPROVEMENTS` and `JONES3D_SPEEDRUN_BUILD` turn from compile-time profiles into runtime options.
