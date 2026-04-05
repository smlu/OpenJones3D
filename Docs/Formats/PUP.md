# PUP File Format

This document describes the text `.pup` puppet-class format used by OpenJones3D.

In the current codebase, `.pup` files are parsed by [`sithPuppet_LoadPuppetClassEntry()`](../../Libs/sith/Engine/sithPuppet.c#L2831) and loaded through [`sithPuppet_LoadPuppetClass()`](../../Libs/sith/Engine/sithPuppet.c#L2795).

## Purpose

A `.pup` file defines a puppet class:

- which animation keyframe should be used for each puppet major-mode / submode pair
- which keyframe playback flags should be applied
- which low/high joint-priority values should be used for that playback
- which model nodes should be treated as special puppet joints such as neck, hip, or aim joints

At runtime, the parsed file becomes one [`SithPuppetClass`](../../Libs/sith/types.h#L2276) instance.

## Where It Is Used

`.pup` files are used in at least two places in the current tree:

- thing definitions can reference a puppet class through the `puppet` thing property, which calls [`sithPuppet_LoadPuppetClass()`](../../Libs/sith/World/sithThing.c#L3061)
- world resource lists can reference puppet classes through the `animclass` section parser registered in [`sithWorld.c`](../../Libs/sith/World/sithWorld.c#L303)

## File Location And Naming

The loader does not take a full path in normal use. It receives a filename and resolves it under:

```text
misc\pup\<filename>
```

That path is constructed in [`sithPuppet_LoadPuppetClass()`](../../Libs/sith/Engine/sithPuppet.c#L2818).

Important notes:

- The filename should include the `.pup` extension.
- The class cache is keyed by that filename string.
- Loading the same filename again in the same world returns the cached class instead of reparsing the file.

## Related Data Loaded By PUP Entries

Submode rows usually refer to `.key` animation files. Those are loaded from:

```text
3do\key\<keyframeName>
```

That path is constructed in [`sithPuppet_LoadKeyframe()`](../../Libs/sith/Engine/sithPuppet.c#L3386).

## Parser Rules

`.pup` files use the generic [`stdConffile`](../../Libs/std/General/stdConffile.c) text parser, so the following rules come from that shared implementation:

| Behavior | Current implementation |
| --- | --- |
| Case sensitivity | Entire lines are lowercased before tokenization, so the format is effectively case-insensitive. This also means filenames written in the file are lowercased before loading. |
| Whole-line comments | Lines starting with `#` or `;` are skipped. |
| Inline comments | Inline `#` comments are stripped. Inline `;` comments are not treated specially unless the line starts with `;`. |
| Separators | Tokens are separated by commas, spaces, and tabs. |
| Assignment form | `name=value` is supported. |
| Bare tokens | A token without `=` is treated as both its own name and value. This is how `Joints` and `end` are recognized. |
| Quoted strings | Quoted tokens are supported by the generic parser, although `.pup` files normally use plain unquoted tokens. |
| Empty lines | Skipped automatically. |
| Line continuation | A line ending with `\` before the newline is continued onto the next line by the generic parser. |

Relevant parser code:

- line preprocessing in [`stdConffile_ReadLine()`](../../Libs/std/General/stdConffile.c#L260)
- token splitting in [`stdConffile_ReadArgsFromStr()`](../../Libs/std/General/stdConffile.c#L189)

## In-Memory Layout

A parsed `.pup` file fills these fields:

| Structure | Field | Meaning |
| --- | --- | --- |
| `SithPuppetClass` | `aName[64]` | Class name, copied from the filename passed to the loader. |
| `SithPuppetClass` | `aModes[24][84]` | Animation mapping table indexed as `[majorMode][submode]`. |
| `SithPuppetClass` | `aJoints[10]` | Optional special-joint mapping table. |
| `SithPuppetClassSubmode` | `pKeyframe` | Loaded keyframe pointer, or `NULL`. |
| `SithPuppetClassSubmode` | `flags` | Keyframe playback flags. |
| `SithPuppetClassSubmode` | `lowPriority` | Lower node-priority value for playback. |
| `SithPuppetClassSubmode` | `highPriority` | Higher node-priority value for playback. |

Structure definitions:

- [`SithPuppetClassSubmode`](../../Libs/sith/types.h#L2268)
- [`SithPuppetClass`](../../Libs/sith/types.h#L2276)

## Major Modes

`.pup` files store rows by puppet major mode, not by separate move-mode and armed-mode columns.

In the current engine:

- there are `24` major-mode slots total
- they are derived from `3` move modes times `8` armed modes
- the runtime major-mode formula is `armedMode + 8 * moveMode`

See:

- [Puppet Move Modes](../COG/Types-And-Modes.md#puppet-move-modes)
- [Puppet Armed Modes](../COG/Types-And-Modes.md#puppet-armed-modes)
- [Puppet Major Modes](../COG/Types-And-Modes.md#puppet-major-modes)

In practice, the stock `indy.pup` uses:

- `Mode=0` for normal unarmed movement
- `Mode=8` for unarmed swimming
- `Mode=16` for unarmed crawling

## Top-Level Grammar

At the top level, a `.pup` file is a sequence of:

- mode headers
- submode rows belonging to the current mode
- an optional `Joints` block

A simplified grammar looks like this:

```text
<file> ::= { <mode-header> | <submode-row> | <joints-block> }

<mode-header> ::= Mode=<majorMode> [ , BasedOn=<baseMode> ]

<submode-row> ::= <submodeName> <keyframeName|none> [ <flagsHex> ] [ <lowPriority> ] [ <highPriority> ]

<joints-block> ::= Joints
                   { <jointIndex>=<modelNodeIndex> }
                   end
```

## Mode Headers

Mode headers are recognized when the first parsed token is `mode=...`.

Example:

```text
Mode=3
Mode=4, BasedOn=3
```

Behavior:

- `Mode=<n>` selects the current major-mode slot.
- Valid mode values are `0` through `23`.
- The selected mode remains active until another `Mode=` line is encountered.
- A later `Mode=` line for the same value continues editing that same mode; it does not reset it.

### BasedOn

`BasedOn=<m>` is optional.

If present, the parser copies the entire `aModes[baseMode]` row into `aModes[modeNum]` with `memcpy()` before reading later overrides for the new mode.

That means:

- inheritance is all-or-nothing for the full 84-submode row
- later submode rows in the new mode overwrite inherited entries one by one
- joints are not copied by `BasedOn`; only submode entries are
- order matters: if you base a mode on another mode that has not been populated yet, you only copy that mode's current contents, which may still be the all-zero default

Implementation: [`sithPuppet_LoadPuppetClassEntry()`](../../Libs/sith/Engine/sithPuppet.c#L2854)

## Submode Rows

Any top-level line that is not a `Mode=` header or `Joints` block start is treated as a candidate submode row.

Example:

```text
stand       in_stand.key            0x18   0   1
walk        in_walk_fwd.key         0x11   2   3
holsterweapon none                  0x38   0   6
```

### Column Meaning

| Position | Meaning | Required | Default |
| --- | --- | --- | --- |
| 1 | Submode name | Yes | None |
| 2 | Keyframe filename or `none` | Yes | None |
| 3 | Keyframe flag bitmask, parsed as hexadecimal | No | `0` |
| 4 | Low priority | No | `0` |
| 5 | High priority | No | Same value as low priority |
| 6+ | Extra tokens | Ignored by the current parser | N/A |

### Submode Names

The first token must be one of the registered lower-case submode names from [`sithPuppet_aStrSubModes[]`](../../Libs/sith/Engine/sithPuppet.c#L130).

That list matches the names documented in [Puppet Submodes](../COG/Types-And-Modes.md#puppet-submodes).

Examples:

- `stand`
- `walk`
- `drawweapon`
- `aimweapon`
- `fire`
- `mountfromwater`
- `fallforward`

If the submode name is not recognized, the parser logs an error and skips that row.

### Keyframe Filename

The second token is the keyframe resource name, usually a `.key` filename such as `in_stand.key`.

Special case:

- if the keyframe token is `none`, the parser stores `NULL` for that submode instead of loading a keyframe

Important runtime consequence:

- a submode with no keyframe is still a valid slot in the class table
- but any runtime verb that tries to play that submode will fail because there is no keyframe bound to it

### Flags

The optional third token is parsed with `%x`, so hexadecimal values such as `0x18` are accepted directly.

Supported bits are the standard rdroid keyframe flags:

| Flag | Value | Meaning |
| --- | ---: | --- |
| `RDKEYFRAME_PUPPET_CONTROLLED` | `0x01` | Track is puppet-controlled. |
| `RDKEYFRAME_NOLOOP` | `0x02` | Do not loop. |
| `RDKEYFRAME_PAUSE_ON_LAST_FRAME` | `0x04` | Pause on the last frame. |
| `RDKEYFRAME_RESTART_ACTIVE` | `0x08` | Restart an already active track. |
| `RDKEYFRAME_DISABLE_FADEIN` | `0x10` | Disable fade-in blending. |
| `RDKEYFRAME_FADEOUT_NOLOOP` | `0x20` | Fade out instead of looping. |
| `RDKEYFRAME_FORCEMOVE` | `0x40` | Treat the key as a force-move track. |

Reference:

- [`rdKeyframeFlags`](../../Libs/rdroid/types.h#L129)
- [Keyframe Flags](../COG/Flags.md#keyframe-flags)

### Priorities

The optional fourth and fifth tokens set:

- `lowPriority`
- `highPriority`

If `highPriority` is omitted, it is set to the same value as `lowPriority`.

These values are passed unchanged into [`rdPuppet_AddTrack()`](../../Libs/rdroid/Engine/rdPuppet.c#L816). During track blending, rdroid chooses:

- `highPriority` for model nodes where `(node->type & keyframe->type) != 0`
- `lowPriority` for all other nodes

Relevant code:

- write to the class table in [`sithPuppet_LoadPuppetClassEntry()`](../../Libs/sith/Engine/sithPuppet.c#L2916)
- runtime use in [`rdPuppet.c`](../../Libs/rdroid/Engine/rdPuppet.c#L508)

### Duplicate Rows

If the same submode is assigned more than once for the same `Mode=`, the later row wins because it overwrites the same `aModes[modeNum][submode]` entry.

### Failed Keyframe Loads

If a referenced keyframe cannot be loaded:

- the keyframe loader logs an error
- the `.pup` parser continues
- the stored keyframe pointer remains `NULL`

So from a runtime perspective, a failed load behaves like an empty slot.

## Joints Block

The `Joints` block is optional.

Example:

```text
Joints
0=8
1=7
3=15
4=12
5=14
6=11
end
```

Behavior:

- the block starts on a top-level line whose bare token is `Joints`
- each inner line is parsed as `<jointIndex>=<modelNodeIndex>`
- the block ends when the first token is `end`
- all joint slots are initialized to `-1` before parsing, so unspecified joints stay disabled
- if the same joint slot is assigned more than once, the later assignment wins
- out-of-range joint indices are logged and ignored

Implementation: [`sithPuppet_LoadPuppetClassEntry()`](../../Libs/sith/Engine/sithPuppet.c#L2867)

### Joint Slot Table

| Slot | Constant | Meaning | Current-tree use |
| ---: | --- | --- | --- |
| `0` | `SITHPUPPET_JOINTINDEX_HEAD` | Head joint slot | No direct built-in consumer was found in the current tree, but the slot remains addressable through generic joint-angle access. |
| `1` | `SITHPUPPET_JOINTINDEX_NECK` | Neck joint slot | Used for head pitch/yaw bending in [`sithActor_UpdateHeadPYR()`](../../Libs/sith/World/sithActor.c#L719). |
| `2` | `SITHPUPPET_JOINTINDEX_HIP` | Hip joint slot | Used for partial pitch bending in [`sithActor_UpdateHeadPYR()`](../../Libs/sith/World/sithActor.c#L729). |
| `3` | `SITHPUPPET_JOINTINDEX_FIRE1` | Fire joint slot 1 | No direct built-in consumer was found in the current tree. |
| `4` | `SITHPUPPET_JOINTINDEX_FIRE2` | Fire joint slot 2 | No direct built-in consumer was found in the current tree. |
| `5` | `SITHPUPPET_JOINTINDEX_AIM1` | Aim joint slot 1 | Used for partial pitch bending in [`sithActor_UpdateHeadPYR()`](../../Libs/sith/World/sithActor.c#L737). |
| `6` | `SITHPUPPET_JOINTINDEX_AIM2` | Aim joint slot 2 | Used for partial pitch bending in [`sithActor_UpdateHeadPYR()`](../../Libs/sith/World/sithActor.c#L745). |
| `7` | `SITHPUPPET_JOINTINDEX_AIMPITCH` | Aim-pitch joint slot | Receives direct pitch updates in [`sithActor_UpdateAimJoints()`](../../Libs/sith/World/sithActor.c#L760). |
| `8` | `SITHPUPPET_JOINTINDEX_AIMYAW` | Aim-yaw joint slot | Receives direct yaw updates in [`sithActor_UpdateAimJoints()`](../../Libs/sith/World/sithActor.c#L766). |
| `9` | `SITHPUPPET_JOINTINDEX_AIMROLL` | Aim-roll joint slot | No direct built-in consumer was found in the current tree, but the slot remains addressable through generic joint-angle access. |

Any slot may also be read or written by the generic joint-angle COG verbs when the thing has a puppet class:

- [`SetJointAngle()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L4569)
- [`GetJointAngle()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L4616)

## Example

This minimal example shows the main pieces together:

```text
# Minimal puppet class example

Mode=0
stand       in_stand.key            0x18   0   1
walk        in_walk_fwd.key         0x11   2   3
run         in_run_fwd.key          0x11   2   3
drawweapon  none                    0x1c   0   4

Mode=1, BasedOn=0
drawweapon  in_attack_pull_whip.key 0x1c   1   4
aimweapon   in_attack_aim_whip.key  0x1c   0   5
fire        in_attack_fire_whip.key 0x1a   7   7

Joints
0=8
1=7
3=15
4=12
5=14
6=11
end
```

## Practical Authoring Notes

- Put the file under `misc\pup\`.
- Use the same lowercase submode names that the engine registers internally.
- Use `Mode=` values from the current major-mode table rather than inventing new mode numbers.
- Use `BasedOn=` when you want to clone a full major-mode row and then override only a few submodes.
- Use `none` when you want a defined slot with no keyframe attached.
- Keep in mind that `.pup` content is lowercased by the parser before filenames are resolved.
- Prefer `#` for inline comments, since inline `;` comments are not stripped by the generic parser.

## Error Handling Summary

Current behavior on malformed data:

- file open failure: class load fails
- too many puppet classes allocated for the world: class load fails
- unknown top-level command or unknown submode name: logged, parser continues
- invalid joint index in `Joints`: logged, entry ignored
- keyframe load failure: logged, parser continues with a `NULL` keyframe slot

## References

- puppet loader: [`sithPuppet.c`](../../Libs/sith/Engine/sithPuppet.c#L2795)
- puppet runtime definitions: [`sith/types.h`](../../Libs/sith/types.h#L2268)
- joint index constants: [`sithPuppet.h`](../../Libs/sith/Engine/sithPuppet.h#L12)
- generic config parser: [`stdConffile.c`](../../Libs/std/General/stdConffile.c#L189)
- submode names and major-mode context: [Types And Modes](../COG/Types-And-Modes.md#puppet-submodes)
- keyframe flags: [COG Flags](../COG/Flags.md#keyframe-flags)
