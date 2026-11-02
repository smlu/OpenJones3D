# ADR-0008: Babylon Engine is MPL-2.0 with permissive dependencies

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Babylon should be reusable, including in closed games and on platforms that require static linking, while
improvements to Babylon itself should stay open. zlib or MIT would let anyone ship a closed modified Babylon.
LGPL needs relinking for closed users, which is impractical with static linking and, in v3, conflicts with
locked consoles. AGPL would bind every user of Babylon.

## Decision

- Babylon is licensed under MPL-2.0, in its own `Babylon/LICENSE`. Copyright holder is seelenkuchen.
- Outside contributions to Babylon need a Contributor License Agreement, which keeps dual licensing possible.
- Babylon dependencies are permissive only: zlib, MIT, BSD, Apache-2.0, public domain.
- No code enters Babylon from AGPL or GPL sources. They may be read for ideas.

## Consequences

- Changes to Babylon files must be published by whoever distributes them. New files and the games using
  Babylon can stay closed.
- MPL-2.0 is compatible with the AGPL game.
- Babylon is written fresh, even where an AGPL implementation of the same thing already exists here.
- CI checks dependency licenses.
