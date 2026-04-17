# SND File Format

This document describes the text `.snd` sound-class format used by OpenJones3D.

In the current codebase, `.snd` files are loaded through [`sithSoundClass_Load()`](../../Libs/sith/World/sithSoundClass.c#L348) and parsed by [`sithSoundClass_LoadEntry()`](../../Libs/sith/World/sithSoundClass.c#L397).

## Purpose

A `.snd` file defines a sound class:

- which sound-class mode names exist for the class
- which sound file each mode should use
- which playback flags should be applied
- which min and max sound radii should be used
- which maximum volume scalar should be used
- which alternative entries exist for the same mode

At runtime, the parsed file becomes one [`SithSoundClass`](../../Libs/sith/types.h#L2261) with zero or more linked [`SithSoundClassEntry`](../../Libs/sith/types.h#L2248) chains.

## Where It Is Used

`.snd` files are used in at least two places in the current tree:

- thing definitions can reference a sound class through the `soundclass` thing property in [`sithThing.c`](../../Libs/sith/World/sithThing.c#L3001)
- world resource lists can reference sound classes through the `soundclass` section parser registered in [`sithWorld.c`](../../Libs/sith/World/sithWorld.c#L304)

## File Location And Naming

The normal loader takes a filename and resolves it under:

```text
misc\snd\<filename>
```

That path is constructed in [`sithSoundClass_Load()`](../../Libs/sith/World/sithSoundClass.c#L380).

Important notes:

- the filename should include the `.snd` extension
- the sound-class cache is keyed by that filename string
- loading the same filename again in the same world returns the cached class instead of reparsing the file
- the special names `none` and an empty string are treated as no sound class and return `NULL`

## Related Data Loaded By SND Entries

The second column of each row is a sound filename. That name is passed to [`sithSound_Load()`](../../Libs/sith/Devices/sithSound.c#L218), which resolves it under:

```text
sound\<filename>
```

## Parser Rules

`.snd` files use the generic [`stdConffile`](../../Libs/std/General/stdConffile.c) token parser.

| Behavior | Current implementation |
| --- | --- |
| Case sensitivity | Entire lines are lowercased before tokenization, so the format is effectively case-insensitive. This also lowercases filenames before loading. |
| Whole-line comments | Lines starting with `#` or `;` are skipped. |
| Inline comments | Inline `#` comments are stripped. |
| Separators | Tokens are separated by commas, spaces, and tabs. |
| Empty lines | Skipped automatically. |
| End marker | Individual `.snd` files are read until EOF; there is no file-local `end` marker. |

## In-Memory Layout

| Structure | Field | Meaning |
| --- | --- | --- |
| `SithSoundClass` | `aName[64]` | Class name copied from the filename passed to the loader. |
| `SithSoundClass` | `aEntries[141]` | Per-mode entry heads, indexed by sound-class mode number. |
| `SithSoundClassEntry` | `hSnd` | Loaded sound handle, or `0` for `none`. |
| `SithSoundClassEntry` | `playflags` | Playback flags parsed from the row. |
| `SithSoundClassEntry` | `maxVolume` | Maximum volume scalar for this entry. |
| `SithSoundClassEntry` | `minRadius` | Minimum radius for attenuation or spatial playback. |
| `SithSoundClassEntry` | `maxRadius` | Maximum radius for attenuation or spatial playback. |
| `SithSoundClassEntry` | `numEntries` | Number of linked entries for this mode. Stored only on the head entry. |
| `SithSoundClassEntry` | `pNextMode` | Next alternative entry for the same mode. |

References:

- [`SithSoundClassEntry`](../../Libs/sith/types.h#L2248)
- [`SithSoundClass`](../../Libs/sith/types.h#L2261)

## Row Format

Each non-comment row uses this format:

```text
<modeName> <soundFile|none> [flagsHex] [minRadius] [maxRadius] [maxVolume]
```

Example:

```text
lwalkhard fol_in_lwalkhard.wav 0x80 0.01 2.0 0.5
```

Column meaning:

| Position | Meaning | Required | Default |
| --- | --- | --- | --- |
| 1 | Sound-class mode name | Yes | None |
| 2 | Sound filename or `none` | Yes | None |
| 3 | Playback flags, parsed with `%x` | No | `0x40` for the first row, then the most recently parsed flag value for later rows if omitted |
| 4 | Minimum radius | No | `0.5` |
| 5 | Maximum radius | No | `2.5` |
| 6 | Maximum volume scalar | No | `1.0` |
| 7+ | Extra tokens | Ignored by the current parser | N/A |

## Mode Names

The first token must be one of the registered sound-class mode names from [`sithSoundClass_aSoundModeNames[]`](../../Libs/sith/World/sithSoundClass.c#L22).

See [Sound Class Modes](../COG/Types-And-Modes.md#sound-class-modes).

Current range:

- valid mode values are `1` through `140`
- mode `0` is reserved and intentionally unused

If the mode name is not recognized, the parser logs an error and skips that row.

## Sound File Column

The second token is the sound filename, usually a `.wav` file under `sound\`.

Special case:

- if the token is `none`, the parser creates an entry with `hSnd = 0` instead of loading a sound

Runtime consequence:

- the mode still exists in the class table
- attempting to play that entry returns an invalid handle because [`sithSoundClass_PlayModeEntry()`](../../Libs/sith/World/sithSoundClass.c#L821) rejects entries with `hSnd == 0`

## Playback Flags

The optional third token is parsed with `%x`, so values should be written as hexadecimal, typically with a `0x` prefix.

See [Sound Play Flags](../COG/Flags.md#sound-play-flags).

Important implementation detail:

- the loader initializes `flags` to `SOUNDPLAY_ABSOLUTE_POS`
- when a row supplies an explicit flag value, that value replaces the current `flags`
- when a later row omits the flags column, the previous `flags` value is reused instead of resetting to `SOUNDPLAY_ABSOLUTE_POS`

That means omitted flags inherit the most recently parsed explicit flags value in the current file.

Common runtime behavior:

- if `SOUNDPLAY_ABSOLUTE_POS` is set, playback uses [`sithSoundMixer_PlaySoundPos()`](../../Libs/sith/World/sithSoundClass.c#L830) at the thing's current world position
- otherwise playback uses [`sithSoundMixer_PlaySoundThing()`](../../Libs/sith/World/sithSoundClass.c#L873) and stays attached to the thing
- `SOUNDPLAY_PLAYONCE` and `SOUNDPLAY_PLAYTHINGONCE` are commonly used on looping motion sounds such as vehicle engines

## Radii And Volume

If present, the optional numeric columns mean:

- `minRadius`: minimum attenuation radius
- `maxRadius`: maximum attenuation radius
- `maxVolume`: maximum volume scalar used when the sound is played

These defaults are applied before optional overrides:

- `minRadius = 0.5`
- `maxRadius = 2.5`
- `maxVolume = 1.0`

## Duplicate Modes And Alternatives

If the same mode name appears more than once:

- the first row becomes the head entry for that mode
- later rows are appended to a linked list through `pNextMode`
- the head entry's `numEntries` is updated to the total number of alternatives

Runtime use:

- [`sithSoundClass_PlayModeRandom()`](../../Libs/sith/World/sithSoundClass.c#L589) picks a random alternative
- [`sithSoundClass_PlayMode()`](../../Libs/sith/World/sithSoundClass.c#L718) selects an entry by ratio
- [`sithSoundClass_PlayModeFirst()`](../../Libs/sith/World/sithSoundClass.c#L568) uses the first entry only

## Example

This is the canonical row shape used by stock files such as `indy.snd`:

```text
lwalkhard   fol_in_lwalkhard.wav   0x80   0.01   2.0   0.5
hurtimpact  inxj016g.wav           0x80   0.01   1.0   0.7
```

This is also valid and common for duplicate-mode alternatives:

```text
hithard jep_crash_small_1.wav 0x880 0.1 3.0 1.0
hithard jep_crash_small_2.wav 0x880 0.1 3.0 1.0
hithard jep_crash_small_3.wav 0x880 0.1 3.0 1.0
```

## Practical Authoring Notes

- Put the file under `misc\snd\`.
- Use mode names from the engine's registered sound-class mode table.
- Use hexadecimal flag values explicitly on every row if you do not want accidental flag inheritance.
- Use duplicate rows for the same mode when you want random alternates.
- Use `none` only when you intentionally want a mode slot that resolves to no playable sound.

## Error Handling Summary

Current behavior on malformed data:

- file open failure: class load fails
- row with fewer than 2 arguments: logged and skipped
- unknown mode name: logged and skipped
- sound load failure: logged and skipped
- memory allocation failure for a row: logged and skipped
- other valid rows in the same file continue to load

## References

- sound-class loader: [`sithSoundClass.c`](../../Libs/sith/World/sithSoundClass.c#L348)
- sound loader: [`sithSound.c`](../../Libs/sith/Devices/sithSound.c#L218)
- sound-class structures: [`sith/types.h`](../../Libs/sith/types.h#L2248)
- sound play flags: [COG Flags](../COG/Flags.md#sound-play-flags)
- sound mode list: [Types And Modes](../COG/Types-And-Modes.md#sound-class-modes)

