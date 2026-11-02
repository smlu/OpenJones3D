# 2. Constraints

## 2.1 Legal

| Constraint | Consequence |
|---|---|
| The game and its trademarks belong to Lucasfilm | No game assets, original executables or extracted data in the repository. Players need a legal copy of the game. The project name avoids the franchise name |
| The game code is AGPL-3.0, inherited from OpenJones3D | Everything outside `Babylon/` stays AGPL-3.0, copyright "OpenJones3D contributors". Distributed builds must come with their full source |
| Babylon is MPL-2.0 | No code may enter `Babylon/` from AGPL or GPL sources: not from `Libs/`, earlier backends, OpenJones3D branches, Jones3D-Portable or PowerslaveEX. Those may be read for ideas only |
| Babylon dependencies | Permissive licenses only: zlib, MIT, BSD, Apache-2.0, public domain. No LGPL, because static linking must stay possible |
| Babylon copyright | Held by seelenkuchen. Outside contributions to `Babylon/` need a Contributor License Agreement, which keeps dual licensing possible |

## 2.2 Technical

| Constraint | Consequence |
|---|---|
| Stock game data and savegames must load | File formats are fixed. In-memory structures may change, but anything read from or written to disk keeps its exact layout, through explicit serialization |
| The simulation must stay deterministic | Same inputs and savegame give the same state trace. Code in the simulation may not depend on wall-clock time, thread scheduling or uninitialized memory |
| Game logic is C11 | `Libs/sith`, `Libs/rdroid` and `Jones3D/` stay C. Babylon is C++, and the adapter between them exports C functions |
| 32-bit origins | The code was written for a 32-bit target. Anything that relied on pointer size, union punning or packing must be made explicit for 64-bit |
| One build system | CMake, with GCC, Clang and MSVC |

## 2.3 Organizational

| Constraint | Consequence |
|---|---|
| Hard fork | No more merges from upstream. Upstream fixes are taken by hand, case by case |
| Small team, AI-assisted | Work is split into thin, reviewed slices. Every slice is built and tested before it lands |
| Nothing is pushed without the owner's consent | Including tags and repository changes |

## 2.4 Conventions

| Convention | Where |
|---|---|
| Code style, naming, commits | [CONTRIBUTING.md](../../CONTRIBUTING.md) |
| Tests | [Testing.md](../Testing.md) |
| Decisions are recorded as ADRs | [Chapter 9](09-decisions/README.md) |
