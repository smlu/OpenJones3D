# ADR-0015: Babylon modules as CMake targets

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Babylon should stay separable from the game ([ADR-0006](ADR-0006-babylon-engine-in-repo.md)), and its parts
should stay separable from each other, so each can be tested, replaced or split out alone. Reviews alone don't
keep dependencies in check. Gradle enforces this with subprojects and `api` and `implementation`
dependencies. C++ has no direct equivalent, but CMake targets come close. C++20 modules would enforce
visibility in the language itself, but toolchain support (Apple Clang, mixing with SDL3's headers, IDEs) is
still uneven.

## Decision

- Babylon is split into modules by role, about 8 to 10: platform, input, time, files, render, audio, cvars,
  threads, log, core.
- Each module is a CMake static-library target, declared through one helper,
  `babylon_add_module(<name> DEPS <modules>)`.
- Each module has a public `include/babylon/<module>/`, and private `src/`, `backends/<backend>/` and
  `tests/`. Other modules see only the public headers, and only when they declare the dependency.
- Dependencies are `PUBLIC` when they appear in public headers, otherwise `PRIVATE`. Cycles are not allowed.
- Backends are registered explicitly at startup, never through static self-registration.
- CI exports the target graph and compares it with an allowed dependency list.
- C++20 modules are deferred and re-evaluated once all three desktop toolchains support them well.

## Consequences

- An undeclared dependency fails to compile, not just in review.
- Each module builds and tests alone, which also makes license checks per module possible.
- Changing the allowed dependency list is an architecture change, recorded in an ADR.
- More, smaller `CMakeLists.txt` files, kept short by the helper.
