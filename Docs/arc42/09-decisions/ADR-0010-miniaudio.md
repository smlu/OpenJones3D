# ADR-0010: miniaudio for audio

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

The game mixes 3D-positioned sounds. OpenAL Soft is the usual choice, and Jones3D-Portable uses it, but it
is LGPL, which [ADR-0008](ADR-0008-babylon-license.md) rules out for Babylon.

## Decision

Babylon's audio backend uses miniaudio (public domain or MIT-0), which includes 3D spatialization.

## Consequences

- Single-file dependency, permissive, on all three desktop platforms.
- The sound adapter maps the game's `Driver_*` and `Sound_*` functions onto Babylon audio, using
  Jones3D-Portable's `Driver_openal.c` as a reference.
