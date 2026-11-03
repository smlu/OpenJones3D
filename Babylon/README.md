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

`core` (version, assert) is the first module, `log` (the global logging API with
pluggable sinks) the second. `files` hides the host file system: file access,
directory listing, base path and per-user writable path (ADR-0019), with an
SDL3 backend and an in-memory Null backend. `platform` hides the operating
system layer: windows and one polled queue of OS events (ADR-0009), with the
same two backends. `input` has no backend of its own: `InputState` folds the
polled events into the state a game reads per frame, and `InputScript` replays
recorded events so headless runs see exactly what a device would (ADR-0018).

## Declaring a module

    babylon_add_module(<name>
        SOURCES <files...>
        [DEPS <babylon modules...>]          # private dependencies
        [PUBLIC_DEPS <babylon modules...>]   # dependencies in public headers
        [LIBS <external targets...>])        # non-Babylon targets, linked PRIVATE

Creates the library `babylon_<name>` (alias `babylon::<name>`) with C++20, Babylon's
warning flags and, with `BABYLON_SANITIZE=ON`, address and undefined behavior sanitizer
flags. Dependencies must name modules added before this one, so the target graph stays
acyclic. `LIBS` names targets outside Babylon, such as `${BABYLON_SDL3_TARGET}` from
`cmake/BabylonSdl3.cmake`; an unknown target is a configure error.

    babylon_add_module_tests(<name>
        SOURCES <files...>
        [DEPS <babylon modules...>])        # modules the tests use directly

Links `babylon_<name>` against the shared doctest main and registers the test
`babylon.<name>` with the labels `babylon;unit`. An optional `DEPS` module is
checked like a module dependency and linked PRIVATE to the test executable,
for modules the tests call that the tested module keeps private, such as
`log` when a test attaches a `CaptureSink`; such a link adds no edge to the
module graph.

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
