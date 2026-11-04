# ADR-0020: Cvars carry flags, including a simulation flag

- **Status:** Accepted
- **Date:** 2026-11-03

## Context

[ADR-0013](ADR-0013-cvars-in-foundation.md) puts cvars into the Babylon foundation and requires cvars that
affect the simulation to be recorded in traces. Quake 3 gives every cvar flags for saving, read-only and
startup-only. A Quake 3 fork fixed a bug where game modules could change file system path cvars by
bypassing the startup-only flag ([research](../../Research/Engine-Patterns.md)).

COG scripts have no access to configuration today, so that bug can't happen yet.

## Decision

Every cvar has flags, set when it is registered:

| Flag | Meaning |
|---|---|
| Saved | Written to the configuration file |
| Read-only | Set by the program only, never by the player |
| Startup-only | Set only on the command line or in the configuration file before startup |
| Simulation | Affects the simulation: written into every trace and replay |

Babylon defines the flags generically; "simulation" is a flag the game assigns, not a Jones3D name in
Babylon.

## Consequences

- Recording simulation settings in traces happens through the flag, not through a hand-kept list.
- Changing a simulation cvar during a recorded run is either refused or recorded at that frame; the M5 plan
  decides which.
- Once a console or mods can set cvars (M11 and later), startup-only and path cvars must reject those writes. This is
  recorded as a requirement for that milestone.
