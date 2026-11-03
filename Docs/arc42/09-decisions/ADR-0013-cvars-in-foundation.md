# ADR-0013: Cvars are part of the Babylon foundation

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Every later feature needs a place for its settings: renderer options, the presentation and simulation
toggles from [ADR-0003](ADR-0003-fidelity-split.md), mod paths. Kex treats console variables as part of the
framework.

## Decision

Babylon gets console variables (cvars) in M5. The visible console comes later, as a Babylon extra (M11 and later).

## Consequences

- New options are cvars from the start. Which `Jones.cfg` settings move to cvars is decided per setting.
- Cvars that affect the simulation must be recorded in traces.
