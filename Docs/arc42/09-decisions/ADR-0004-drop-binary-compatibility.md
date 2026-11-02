# ADR-0004: Drop binary compatibility with the retail executable

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

OpenJones3D kept `RTI/` address tables, hook and trampoline machinery in `j3dcore`, frozen struct layouts,
and `Scripts/analyze.py` to count hooked functions. All of it served checking the reconstruction inside the
original executable. After [ADR-0001](ADR-0001-hard-fork.md) it has no user, and the frozen layouts block
64-bit and the new platform layer.

## Decision

Delete the `RTI/` directories, the hook machinery and the progress tracking, and stop freezing in-memory
layouts. This happens in its own change series after the documentation rewrite and before M5. Recovered
function names stay ([ADR-0005](ADR-0005-game-logic-stays-c11.md)).

## Consequences

- On-disk formats keep their exact layout, now through explicit serialization instead of matching structs.
- Behavior can no longer be checked in the original executable. Traces take over ([ADR-0003](ADR-0003-fidelity-split.md)).
- The `JONES3D_STANDALONE` build option becomes the only build and disappears.
