# ADR-0012: Desktop platforms first, portable interfaces

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Kex targets consoles too. Console SDKs are under NDA and can't be part of an open repository, but a design
that assumes a desktop would make a later port expensive.

## Decision

Windows, Linux and macOS are the supported platforms, with CI on all three from M5. No console backends and
no store SDKs. Every Babylon interface avoids desktop assumptions: a mouse may be missing, the working
directory may not be writable, there may be one fixed display, and file access may be asynchronous.
Every interface has a null backend.

## Consequences

- macOS is not a late milestone of its own; it is built and tested from M5.
- Null backends make the whole game run headless, which traces need ([ADR-0003](ADR-0003-fidelity-split.md)).
- A console port later needs only new backends, kept in their own files ([ADR-0008](ADR-0008-babylon-license.md)).
