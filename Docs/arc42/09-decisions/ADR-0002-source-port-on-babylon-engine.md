# ADR-0002: A Kex-style source port on the Babylon Engine

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

The reconstruction is complete (3180 of 3180 functions), and a byte-exact decompilation of the game exists
elsewhere, so this project doesn't need to be the faithful reconstruction. Nightdive's Kex engine shows a
model for remasters: a game-agnostic platform framework with the original game logic on top, mostly unchanged.
The Quake II rerelease, whose game module is public, is a concrete example.

## Decision

The project becomes **Jones3D EX**, an enhanced source port in that model. Its platform framework is the
**Babylon Engine**. The restored game logic runs on Babylon the way the Quake II game module runs on Kex.
Features added after the first release are called Babylon extras.

## Consequences

- The repository is renamed to `Jones3DEX` with the documentation rewrite.
- The game and the framework have separate identities, licenses and tests
  ([ADR-0006](ADR-0006-babylon-engine-in-repo.md), [ADR-0008](ADR-0008-babylon-license.md)).
- The name avoids the franchise trademark.
