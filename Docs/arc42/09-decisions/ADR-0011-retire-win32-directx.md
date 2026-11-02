# ADR-0011: Retire the Win32 and DirectX path after renderer parity

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

The Win32 and DirectX 9 path works today, and is the only reference for what the game should look like on
a modern renderer. Porting it into Babylon would mean porting code that is about to be deleted. DirectX 6 is
already paused.

## Decision

Win32 and DirectX are not ported into Babylon. They stay on the old path as the visual reference until the
SDL3 GPU renderer reaches parity in M6, and are removed after that. Babylon starts with SDL3 and null backends.

## Consequences

- Until M6, Windows has two paths. Shared code must keep building for both.
- After M6, Windows runs through Babylon like the other platforms.
