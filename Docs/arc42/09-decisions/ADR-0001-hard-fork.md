# ADR-0001: Hard fork from OpenJones3D

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

This repository forked [OpenJones3D](https://github.com/smlu/OpenJones3D), which reconstructs the Jones3D engine
function by function and keeps every name, struct layout and global address compatible with the retail
executable, so each function can be hooked into the original and checked there. The goals here differ: a
source port with a new platform layer and renderer. Keeping upstream compatibility would mean keeping hooks,
RTI address tables and frozen layouts, and merging upstream would keep pulling those rules back in.

## Decision

Jones3D EX is a hard fork. No more merges from upstream, no upstream compatibility. Upstream fixes are
taken by hand when they're wanted. The last compatible state is tagged `upstream-compatible-final`
(`8b5fd0c6`). The four open contributions (upstream #65 to #68) were finished as the last ones. The
repository stays in GitHub's fork network for now; detaching it is revisited later.

## Consequences

- Upstream's later work isn't inherited automatically. Someone has to watch upstream for fixes worth taking.
- Code that existed only for upstream compatibility can go ([ADR-0004](ADR-0004-drop-binary-compatibility.md)).
- The license stays AGPL-3.0 and the copyright notice stays "OpenJones3D contributors".
