# ADR-0005: Game logic stays C11 with recovered names

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

The game logic in `Libs/sith`, `Libs/rdroid` and `Jones3D/` is restored C. Rewriting it in C++ would risk
behavior changes and make comparisons with the decompilation harder.

## Decision

The game logic stays C11 and keeps its behavior and recovered function names, unless something blocks a
goal. Structure may still be cleaned up where it doesn't change behavior.

## Consequences

- Diffs against the decompilation stay readable.
- The boundary to Babylon (C++) needs C-callable functions ([ADR-0007](ADR-0007-adapter-between-game-and-babylon.md)).
- Original-name and local-variable conventions in `CONTRIBUTING.md` keep applying to game code.
