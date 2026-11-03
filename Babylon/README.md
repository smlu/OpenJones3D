# Babylon Engine

Babylon is the game-agnostic platform framework that the restored Jones3D EX game logic
runs on, in the model of the Kex engine. It is C++20, licensed under the Mozilla Public
License 2.0 ([LICENSE](LICENSE)), and builds both alone and as a subdirectory of the
Jones3D EX repository.

## The boundary

Nothing in `Babylon/` includes or names anything outside `Babylon/`: no Jones3D or Sith
concepts reach it. All dependencies are permissive (zlib, MIT, BSD, Apache-2.0, public
domain) and recorded in [third_party/README.md](third_party/README.md). Decisions:
ADR-0006, ADR-0008 and ADR-0015 to ADR-0017 in `../Docs/arc42/09-decisions/`.

## Layout

Each module hides one platform decision and is one CMake static-library target
(ADR-0015):

    <module>/
        include/babylon/<module>/   public headers, the only thing other modules see
        src/                        private implementation
        backends/<backend>/         per-backend code
        tests/                      doctest unit tests

`core` (version, assert) is the first module.

## Declaring a module

    babylon_add_module(<name>
        SOURCES <files...>
        [DEPS <babylon modules...>]          # private dependencies
        [PUBLIC_DEPS <babylon modules...>])  # dependencies in public headers

Creates the library `babylon_<name>` (alias `babylon::<name>`) with C++20, Babylon's
warning flags and, with `BABYLON_SANITIZE=ON`, address and undefined behavior sanitizer
flags. Dependencies must name modules added before this one, so the target graph stays
acyclic.

    babylon_add_module_tests(<name> SOURCES <files...>)

Links `babylon_<name>` against the shared doctest main and registers the test
`babylon.<name>` with the labels `babylon;unit`.

## Build and test alone

    cmake -S Babylon -B build/babylon -G Ninja
    cmake --build build/babylon
    ctest --test-dir build/babylon --output-on-failure

In the repository build, `JONES3D_BUILD_BABYLON` gates the subdirectory and
`JONES3D_BUILD_TESTS` and `JONES3D_SANITIZE` drive the Babylon equivalents.

## Checks

`tools/check_boundary.py` enforces the boundary: every `#include` resolves to
Babylon, a C++20 standard header or a third-party header at an allowed
location, public headers stay babylon/standard, and no game name appears in
code or CMake files. It also checks that `third_party/README.md` lists every
vendored directory and FetchContent dependency under a permissive license
(ADR-0008). `tools/check_graph.py` compares a `--graphviz` configure
of the standalone build against `allowed-deps.txt`: each module's direct
dependencies must match exactly and the graph must be acyclic. Both run as
ctest tests labelled `check` (plus a self-test of the checks) and in
`.github/workflows/babylon.yml`. Changing `allowed-deps.txt` is an
architecture change and needs an ADR (ADR-0015).

Design rules (naming, interfaces, patterns):
`../Docs/arc42/08-crosscutting/Babylon-Design-Rules.md`.
