# ADR-0006: Babylon Engine lives in this repository, game-agnostic

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Babylon should be usable for other games one day, but there is only one game today. A separate repository
now would add release overhead and invite designing for users that don't exist.

## Decision

Babylon lives in `Babylon/` in this repository, filled step by step during M5. It is C++ with its own naming
and style, has its own tests in `Babylon/`, and its interfaces are game-agnostic: no Jones3D or Sith names,
types or assumptions. It can be split out when a second user exists.

## Consequences

- CI builds and tests Babylon on its own, and an include check rejects anything outside `Babylon/`.
- Babylon is one building block in this arc42 documentation. It gets its own documentation when it's split out.
- The game reaches Babylon only through the adapter ([ADR-0007](ADR-0007-adapter-between-game-and-babylon.md)).
