# ADR-0023: Advanced graphics start on the CPU geometry path

- **Status:** Accepted
- **Date:** 2026-11-04

## Context

The new renderer (M6) has to be able to carry the graphics planned for later: per-pixel dynamic lighting and
screen-space ambient occlusion (M10), post-processing such as bloom and depth of field (M9), maybe shadows.
Today `rdroid` transforms, lights, clips and sorts everything on the CPU and hands `std3D` pre-transformed,
pre-lit vertices (`StdVertexTL`: screen position, color, texture coordinates). Two ways lead to the
features: keep the CPU geometry path and give its vertices the extra data a shader needs (option A), or submit
world-space meshes, materials and lights and let the GPU transform and light them (option B). The renderer
also changes game state: `sithRender` marks sectors as seen (`SITH_SECTOR_SEEN`), so visibility must keep
running exactly as it does.

## Decision

Option A first. `rdroid` keeps transform, clipping, sorting and visibility on the CPU. M6 adds what the later
features need without changing the classic look: an extended vertex with view-space position and normal
(carried by `rdCache`, interpolated by `rdClip`, filled by `sithRender` and `rdModel3`), the per-frame light
list as a GPU buffer, render passes with offscreen color, depth and normal targets, and a post-processing
chain that is empty by default. GPU geometry (option B) may follow later; the Babylon render interfaces must
not rule it out.

## Consequences

- `rdroid` and `sith` rendering code changes in M6. It is presentation, so it may change freely, but the
  visibility pass stays untouched and the headless traces must stay identical.
- Per-pixel lighting and SSAO in M10 become shader work on data that already reaches the GPU.
- Shadows need extra CPU passes from each light's view, or option B.
