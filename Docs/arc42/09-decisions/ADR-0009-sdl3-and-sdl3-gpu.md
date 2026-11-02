# ADR-0009: SDL3 for the platform and the SDL3 GPU API for rendering

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Babylon needs window, input, timer and file access on Windows, Linux and macOS, and a renderer that reaches
Vulkan, Metal and Direct3D 12 without three implementations. An earlier decision ruled out Vulkan.

## Decision

Babylon's platform backend uses SDL3. The renderer backend uses the SDL3 GPU API, which maps to Vulkan,
Metal or Direct3D 12. This replaces the earlier "no Vulkan" decision.

## Consequences

- One renderer backend reaches all three desktop platforms.
- SDL3 is zlib-licensed, so it fits [ADR-0008](ADR-0008-babylon-license.md).
- Shaders need a cross-compilation step for the SDL3 GPU shader formats.
- Existing OpenGL and GLES work (OpenJones3D's `OpenGL` branch, Jones3D-Portable) is AGPL and serves as reference only.
