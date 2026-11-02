# ADR-0007: An adapter between the game and Babylon

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

The earlier layer design made the recovered platform functions (`std3D_*`, `stdControl_*`, `Sound_*` and
similar) the interface, with backends below them. Those functions are shaped by the 1999 game and its
DirectX and Win32 use, so making them Babylon's interface would tie Babylon to Jones3D.

## Decision

Babylon defines its own interfaces. The recovered functions in `Libs/std`, `Libs/wkernel` and `Libs/sound`
become the adapter: their bodies call Babylon. Adapter files may be C++ and export the recovered functions as
`extern "C"`. Babylon never includes from `Libs/` or `Jones3D/`.

## Consequences

- The game code doesn't change when Babylon or its backends change.
- The adapter is AGPL and may borrow from other AGPL Jones3D work, for example Jones3D-Portable's OpenAL
  driver as a reference for the sound adapter.
- Babylon's interfaces are designed for their role, not for one game's calls.
