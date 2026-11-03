# ADR-0016: Babylon interfaces are virtual, backends are chosen at startup

- **Status:** Accepted
- **Date:** 2026-11-03

## Context

Every Babylon role has several backends: SDL3, Null and sometimes Script
([ADR-0012](ADR-0012-desktop-first-portable-interfaces.md)). The backend can be chosen in three ways:

- **Link time:** each executable links one backend per role. No virtual calls, but headless and windowed
  runs need separate executables, each backend needs its own test binary, and switching means rebuilding.
- **Startup:** virtual interfaces, one backend picked when the program starts. One binary runs windowed or
  headless, and tests run every backend in one binary.
- **Mid-run switching:** like startup, but backends can be replaced while the game runs. Handles and state
  must survive the switch, and every switch path needs its own tests.

The cost of a virtual call is not measurable at the platform level. In the renderer, command lists keep
virtual calls out of the per-draw path.

## Decision

- Babylon interfaces are pure abstract classes: pure virtual functions and a virtual destructor, no data.
- The non-virtual interface pattern (a public non-virtual function calling a private virtual one) is used
  only for contract checks every backend shares, such as argument validation and logging.
- Backends inherit exactly one level from their interface. Shared logic goes into free functions or a helper
  held as a member, not into a base class.
- Backends are chosen once, at startup, in the composition root, and passed to their users. Babylon has no
  singletons; the log is the one global API, and the C adapter edge holds the pointers the recovered
  functions need.
- The only mid-run switch is audio falling back to Null when the device is lost. The audio interface is
  designed for it: handles survive the switch, and sound durations come from the sound data, not the device.

## Consequences

- One executable can run headless, so trace runs exercise the binary that ships.
- Interface tests run every backend in one test binary.
- All backends are linked into every binary, which costs size and dependencies, not speed.
- Null backends must report everything the simulation reads exactly as the real backends do. A trace test
  checks that the same run with SDL3 audio and with Null audio gives an identical simulation trace.
- Further mid-run switching needs a new ADR.
