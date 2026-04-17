# KEY File Format

This document describes the text `.key` animation-keyframe format used by OpenJones3D.

In the current codebase, `.key` files are loaded by [`rdKeyframe_LoadEntry()`](../../Libs/rdroid/Engine/rdKeyframe.c) and normally resolved through [`sithPuppet_LoadKeyframe()`](../../Libs/sith/Engine/sithPuppet.c).

## Purpose

A `.key` file defines one animation keyframe resource:

- global keyframe flags and type
- frame count and playback FPS
- joint count
- optional event markers
- per-node animation entries for position and rotation

At runtime, the parsed file becomes one [`rdKeyframe`](../../Libs/rdroid/types.h).

## File Location And Naming

The normal gameplay loader resolves keyframe filenames under:

```text
3do\key\<filename>
```

That path is constructed in [`sithPuppet_LoadKeyframe()`](../../Libs/sith/Engine/sithPuppet.c).

Important note:

- extracted asset packs may flatten these files into a top-level `key\` directory for tooling convenience, but the engine-side runtime path is still `3do\key\`

## Parser Rules

`.key` files use the generic [`stdConffile`](../../Libs/std/General/stdConffile.c) parser.

| Behavior | Current implementation |
| --- | --- |
| Case sensitivity | Entire non-comment lines are lowercased before tokenization, so keywords are effectively case-insensitive. Mesh names parsed from the file are lowercased as well. |
| Whole-line comments | Lines starting with `#` or `;` are skipped. |
| Inline comments | Inline `#` comments are stripped. |
| Separators | Tokens are separated by commas, spaces, and tabs. |
| End marker | There is no file-local `end` marker. Parsing stops after the expected sections are read. |

## Top-Level Layout

The canonical layout is:

```text
SECTION: HEADER
FLAGS <hex>
TYPE <hex>
FRAMES <count>
FPS <value>
JOINTS <count>

[SECTION: MARKERS
MARKERS <count>
<marker rows>]

SECTION: KEYFRAME NODES
NODES <count>
<node blocks>
```

## Header Fields

| Field | Meaning |
| --- | --- |
| `FLAGS` | Global keyframe flags bitmask. |
| `TYPE` | Raw keyframe type value stored in `rdKeyframe.type`. |
| `FRAMES` | Number of frames in the animation. |
| `FPS` | Playback rate in frames per second. |
| `JOINTS` | Total joint slots available in the file. Must be `<= 64` in the current loader. |

### Keyframe Flags

Supported global flag bits:

| Flag | Value | Meaning |
| --- | ---: | --- |
| `RDKEYFRAME_PUPPET_CONTROLLED` | `0x01` | Puppet-controlled track. |
| `RDKEYFRAME_NOLOOP` | `0x02` | Do not loop. |
| `RDKEYFRAME_PAUSE_ON_LAST_FRAME` | `0x04` | Pause on the last frame. |
| `RDKEYFRAME_RESTART_ACTIVE` | `0x08` | Restart an already active track. |
| `RDKEYFRAME_DISABLE_FADEIN` | `0x10` | Disable fade-in blending. |
| `RDKEYFRAME_FADEOUT_NOLOOP` | `0x20` | Fade out instead of looping. |
| `RDKEYFRAME_FORCEMOVE` | `0x40` | Treat the keyframe as a force-move track. |

These values come from [`rdKeyframeFlags`](../../Libs/rdroid/types.h).

## Optional MARKERS Section

The loader supports an optional markers block between the header and node section:

```C
SECTION: MARKERS
MARKERS 1
12 25
```

Rules:

- marker count must be `<= 16`
- each marker row is `<frame> <type>`
- marker frame values are stored as `float`
- marker types are stored as [`rdKeyMarkerType`](../../Libs/rdroid/types.h) values

Current marker type names include entries such as:

- `1 = LEFTFOOT`
- `2 = RIGHTFOOT`
- `3 = ATTACK`
- `8 = RUNLEFTFOOT`
- `9 = RUNRIGHTFOOT`
- `10 = DIED`
- `16 = ACTIVATE`
- `25 = PICKUP`
- `26 = DROP`
- `27 = MOVE`
- `35 = RIGHTHAND`

See the full enum in [`Libs/rdroid/types.h`](../../Libs/rdroid/types.h).

## Node Section

The node section begins with:

```text
SECTION: KEYFRAME NODES
NODES <count>
```

Important rules:

- `NODES` count must not exceed `JOINTS`
- the loader allocates `JOINTS` slots, but only fills the `NODES` rows actually present
- any remaining node slots stay zero-initialized

Each node block has this shape:

```text
NODE <num>
MESH NAME <name>
ENTRIES <count>
<entry pair 1>
<entry pair 2>
...
```

## Entry Pairs

Each animation entry is stored across two physical lines.

Line 1:

```text
<entryNum>: <frame> <flagsHex> <x> <y> <z> <pitch> <yaw> <roll>
```

Line 2:

```text
<dposx> <dposy> <dposz> <dpitch> <dyaw> <droll>
```

These two lines fill one [`rdKeyframeNodeEntry`](../../Libs/rdroid/types.h):

| Field | Meaning |
| --- | --- |
| `frame` | Frame number for this key. |
| `flags` | Per-entry flag field stored raw. |
| `pos` | Position vector. |
| `rot` | Rotation vector stored as pitch/yaw/roll. |
| `dpos` | Delta position vector. |
| `drot` | Delta rotation vector. |

## In-Memory Layout

Important structures populated by the loader:

| Structure | Field | Meaning |
| --- | --- | --- |
| `rdKeyframe` | `flags` | Global keyframe flags. |
| `rdKeyframe` | `type` | Raw keyframe type value. |
| `rdKeyframe` | `fps` | Playback FPS. |
| `rdKeyframe` | `numFrames` | Frame count. |
| `rdKeyframe` | `numJoints` | Joint-slot count. |
| `rdKeyframe` | `numMarkers` | Number of markers. |
| `rdKeyframe` | `aMarkerFrames[]` | Marker frame values. |
| `rdKeyframe` | `aMarkerTypes[]` | Marker type values. |
| `rdKeyframeNode` | `aMeshName[64]` | Node mesh name. |
| `rdKeyframeNode` | `numEntries` | Number of key entries in this node. |
| `rdKeyframeNode` | `aEntries` | Key entry array. |

## Practical Authoring Notes

- Put runtime keyframes under `3do\key\`.
- Keep `JOINTS <= 64`.
- Keep `MARKERS <= 16`.
- Keep `NODES <= JOINTS`.
- Remember that every key entry consumes two physical lines.
- Prefer the canonical stock section order: `HEADER`, optional `MARKERS`, then `KEYFRAME NODES`.

## Error Handling Summary

Current behavior on malformed data:

- file open failure: load fails
- malformed header syntax: load fails
- `JOINTS > 64`: load fails
- `MARKERS > 16`: load fails
- `NODES > JOINTS`: load fails
- malformed entry line or truncated second line: load fails
- allocation failure: load fails

## References

- [`rdKeyframe_LoadEntry()`](../../Libs/rdroid/Engine/rdKeyframe.c)
- [`sithPuppet_LoadKeyframe()`](../../Libs/sith/Engine/sithPuppet.c)
- [`rdKeyframe`](../../Libs/rdroid/types.h)

