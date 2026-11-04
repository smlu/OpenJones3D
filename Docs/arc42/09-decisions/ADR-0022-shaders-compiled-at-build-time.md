# ADR-0022: Shaders are compiled at build time, with a local cache

- **Status:** Accepted
- **Date:** 2026-11-04

## Context

The SDL3 GPU API (ADR-0009) takes shaders as SPIR-V (Vulkan), MSL (Metal) or DXIL (Direct3D 12) and picks the
format the device accepts at run time. These are portable intermediate forms: the graphics driver compiles
them to the GPU's own code when a pipeline is created, so nothing in them is specific to one GPU. They still
have to be produced from the shader source by a compiler: SDL_shadercross (zlib), which brings DXC and
SPIRV-Cross. The choices were to commit the generated files, to compile during the build, or to compile when
the game starts.

## Decision

Shaders are compiled during the build, from source, into all three formats. Nothing generated is committed.
The outputs are also kept in a gitignored cache directory in the checkout, named by a hash of the shader
source, the compiler version and the options, so a new build tree reuses them and only changed shaders are
recompiled. Building the game needs the shader compiler; there is no build from the cache alone. CI builds
the compiler or restores it with its cache.

## Consequences

- No generated shader files in the repository, so none can drift from their source.
- Every build machine needs SDL_shadercross and what it brings; the first build takes longer.
- Babylon's dependency list (ADR-0017) gains SDL_shadercross, DXC and SPIRV-Cross as build tools; their
  licenses must stay permissive (ADR-0008).
