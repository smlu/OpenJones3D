# ADR-0017: Babylon builds as C++20 with pinned dependencies and doctest

- **Status:** Accepted
- **Date:** 2026-11-03

## Context

Babylon needs a C++ standard, a way to get SDL3 and miniaudio on Windows, Linux and macOS
([ADR-0009](ADR-0009-sdl3-and-sdl3-gpu.md), [ADR-0010](ADR-0010-miniaudio.md),
[ADR-0012](ADR-0012-desktop-first-portable-interfaces.md)), and a test framework. Its dependencies must stay
permissive ([ADR-0008](ADR-0008-babylon-license.md)).

Options for SDL3: CMake `FetchContent`, vendoring the source or a submodule, system packages only, or a
package manager such as vcpkg or Conan. Windows has no system SDL3 and older Linux distributions lack it;
vendoring adds about 100 MB of source; a package manager is an extra tool for two or three dependencies.

## Decision

- Babylon is **C++20**. C++20 modules stay deferred
  ([ADR-0015](ADR-0015-babylon-modules-as-cmake-targets.md)).
- **SDL3** comes through CMake `FetchContent`, pinned to an exact release tag, never a branch. With
  `FIND_PACKAGE_ARGS` an installed SDL3 of that version is used instead when one is found.
- **miniaudio**, a single header, is vendored into `Babylon/` at a recorded version.
- Babylon's tests use **doctest** (MIT). Game and adapter code keep Unity Fixture.

## Consequences

- Every OS builds against the same SDL3 version, and nothing of SDL3 is committed.
- The first configure needs network access to GitHub; later configures use the copy in the build tree. Offline
  builds need a pre-filled `FETCHCONTENT_BASE_DIR` or an installed SDL3.
- Updating SDL3 or miniaudio is a deliberate change of the pinned version, checked by the license check.
- C++20 sets the minimum compilers for all three platforms; the exact versions are recorded in chapter 7.
