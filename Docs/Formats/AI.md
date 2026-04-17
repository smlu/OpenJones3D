# AI File Format

This document describes the text `.ai` AI-class format used by OpenJones3D.

In the current codebase, `.ai` files are loaded through [`sithAIClass_Load()`](../../Libs/sith/AI/sithAIClass.c#L191) and parsed by [`sithAIClass_LoadEntry()`](../../Libs/sith/AI/sithAIClass.c#L246).

## Purpose

An `.ai` file defines an AI class:

- class-wide numeric tuning values such as sight distance, hearing distance, field of view, and armed mode
- the ordered list of instincts attached to the class
- up to 24 numeric arguments per instinct row

At runtime, the parsed file becomes one [`SithAIClass`](../../Libs/sith/types.h#L1498) instance.

## Where It Is Used

`.ai` files are used in at least two places in the current tree:

- thing definitions can reference an AI class through the `aiclass` thing property in [`sithThing.c`](../../Libs/sith/World/sithThing.c#L3074)
- world resource lists can reference AI classes through the `aiclass` section parser registered in [`sithWorld.c`](../../Libs/sith/World/sithWorld.c#L299)

AI-class symbol references in COG also ultimately resolve through [`sithAIClass_Load()`](../../Libs/sith/Cog/sithCog.c#L1175).

## File Location And Naming

The normal loader takes a filename and resolves it under:

```text
misc\ai\<filename>
```

That path is constructed in [`sithAIClass_Load()`](../../Libs/sith/AI/sithAIClass.c#L208).

Important notes:

- the filename should include the `.ai` extension
- the class cache is keyed by that filename string
- loading the same filename again in the same world returns the cached class

### Missing-Class Fallback

If a requested AI class cannot be loaded:

- and the requested name is not `default.ai`, the loader tries to load `default.ai`
- if `default.ai` loads successfully, the caller receives that default class instead
- if `default.ai` also fails, the load fails completely

This fallback is implemented in [`sithAIClass_Load()`](../../Libs/sith/AI/sithAIClass.c#L229).

## Parser Rules

`.ai` files use the generic [`stdConffile`](../../Libs/std/General/stdConffile.c) token parser.

| Behavior | Current implementation |
| --- | --- |
| Case sensitivity | Entire lines are lowercased before tokenization, so the format is effectively case-insensitive. |
| Whole-line comments | Lines starting with `#` or `;` are skipped. |
| Inline comments | Inline `#` comments are stripped. |
| Separators | Tokens are separated by commas, spaces, and tabs. |
| Assignment form | `name=value` is supported. |
| Empty lines | Skipped automatically. |
| End marker | Individual `.ai` files are read until EOF; there is no file-local `end` marker. |

## In-Memory Layout

| Structure | Field | Meaning |
| --- | --- | --- |
| `SithAIClass` | `num` | World-local AI class index. |
| `SithAIClass` | `armedMode` | Default puppet armed mode applied when AI is created. |
| `SithAIClass` | `alignment` | Alignment tuning value. |
| `SithAIClass` | `rank` | Rank tuning value. |
| `SithAIClass` | `maxStep` | Maximum step height. |
| `SithAIClass` | `sightDistance` | Sight distance threshold. |
| `SithAIClass` | `heardDistance` | Hearing distance threshold. |
| `SithAIClass` | `fov` | Stored field-of-view threshold after conversion from degrees. |
| `SithAIClass` | `weakupDistance` | Wake-up distance field. |
| `SithAIClass` | `accurancy` | Accuracy field. |
| `SithAIClass` | `degTurnAlign` | Turn-alignment angle in degrees. |
| `SithAIClass` | `numInstincts` | Number of instinct rows successfully added. |
| `SithAIClass` | `aInstincts[16]` | Instinct table populated from the body rows. |
| `SithAIClass` | `aName[64]` | Class filename. |

Reference:

- [`SithAIClass`](../../Libs/sith/types.h#L1498)

## File Layout

An `.ai` file has two logical parts:

1. one class-parameter line
2. zero or more instinct rows until EOF

A simplified grammar looks like this:

```text
<file> ::= <class-params> { <instinct-row> }

<class-params> ::= <name=value> { , <name=value> }

<instinct-row> ::= <instinctName> [ <arg1> ... <arg24> ]
```

## Class Parameter Line

The first non-comment line is parsed as the class parameter line.

Example:

```text
alignment=-1.0, rank=0.5, maxstep=0.2, sightdist=7.0, heardist=4.0, fov=170, accuracy=1.0
```

Important notes:

- only the first parsed line is treated as the class parameter line
- later `name=value` lines are not treated specially and would instead be parsed as instinct rows
- unknown keys on the first line are ignored

### Recognized Keys

These keys are recognized by the current loader:

| Key | Meaning | Default |
| --- | --- | --- |
| `alignment` | Alignment tuning value. | `0.0` |
| `rank` | Rank tuning value. | `0.0` |
| `maxstep` | Maximum step height. | `0.5` |
| `sightdist` | Sight distance. | `20.0` |
| `heardist` | Hearing distance. | `10.0` |
| `wakeupdist` | Wake-up distance. Stored in the engine's `weakupDistance` field. | `0.0` |
| `fov` | Field of view in degrees. Internally converted to `cos(fov / 2)`. | stored threshold `0.0` |
| `accuracy` | Accuracy tuning value. Stored in the engine's `accurancy` field. | `0.5` |
| `armedmode` | Default puppet armed mode applied to the AI thing. | `0` |
| `degturnalign` | Turn-alignment angle in degrees. | `45.0` |

### Armed Mode

`armedmode` must be within the valid puppet armed-mode range:

- `0` through `7`

If a larger value is supplied, the loader resets it to `0` and logs an error.

See [Puppet Armed Modes](../COG/Types-And-Modes.md#puppet-armed-modes).

### Field Of View Conversion

`fov` is written in degrees in the file, but the loader stores:

```text
cos(fov / 2)
```

Examples:

- `fov=360` stores `-1.0`
- `fov=180` stores `0.0`
- `fov=120` stores `0.5`

That conversion happens in [`sithAIClass_LoadEntry()`](../../Libs/sith/AI/sithAIClass.c#L286).

## Instinct Rows

After the first line, every remaining non-comment line is treated as an instinct row.

Example:

```text
LookForTarget 500 5000 0
BasicFollow   0   0.09 0.05
Roam          10000 5.0 0
```

Row behavior:

- the first token is the instinct name
- up to 24 numeric arguments are copied into both `fltArg[]` and `intArg[]`
- missing arguments default to `0`
- extra arguments beyond the first 24 are ignored

### Instinct Name Matching

The instinct name must match one of the instincts registered at startup by [`sithAIInstinct_InitInstincts()`](../../Libs/sith/AI/sithAI.c#L92) through the runtime instinct registry.

Name matching is case-insensitive via [`stdUtil_StrCmp()`](../../Libs/std/General/stdUtil.c#L202), so stock files commonly use PascalCase names even though the parser lowercases the input internally.

If a name is not registered:

- the loader logs an error
- the row is skipped

### Instinct Count Limit

A class can store at most `16` instincts in [`aInstincts[16]`](../../Libs/sith/types.h#L1515).

If more rows are present:

- the loader logs an error
- it continues scanning the file
- extra rows are not added to the class

## Observed Instinct Names

The actual loader accepts whatever names the runtime registered. In the shipped combined assets, the following instinct names are observed:

```text
AlternateFire
BasicFollow
BlindFire
CircleStrafe
Crouch
Dodge
FearGunshot
Flee
Follow
HitAndRun
Hop
HoverDrift
HumanCombatMove
Jump
Listen
LobFire
LookForTarget
OpenDoors
PrimaryFire
RandomMove
RandomTurn
Retreat
ReturnHome
Roam
SaberFighting
SenseDanger
SnakeFollow
Talk
TurretFire
WallCrawl
Withdraw
```

Those names were collected from the shipped `.ai` files under `misc\ai\`. Some of them correspond directly to known instinct entry points in [`sithAIInstinct.c`](../../Libs/sith/AI/sithAIInstinct.c), while others are currently only observable through asset usage and the original runtime registration path.

## Example

This minimal example matches the stock file shape:

```text
alignment=-1.0, rank=0.5, maxstep=0.2, sightdist=7.0, heardist=4.0, fov=170, accuracy=1.0

LookForTarget 500 5000 0
BasicFollow   0   0.09 0.05
Roam          10000 5.0 0
```

## Practical Authoring Notes

- Put the file under `misc\ai\`.
- Keep all class-wide `name=value` settings on the first non-comment line.
- Use instinct names that the runtime has actually registered.
- Prefer the stock PascalCase instinct naming style for readability, even though parsing is case-insensitive.
- Keep instinct counts at `16` or fewer if you want every row to be retained.

## Error Handling Summary

Current behavior on malformed data:

- file open failure: load fails
- first-line read failure: load fails
- unknown class parameter key: ignored
- invalid `armedmode`: reset to `0` and load continues
- unknown instinct name: logged and skipped
- too many instinct rows: logged and extras are skipped
- missing class file: loader falls back to `default.ai` if possible

## References

- AI class loader: [`sithAIClass.c`](../../Libs/sith/AI/sithAIClass.c#L191)
- AI class structure: [`sith/types.h`](../../Libs/sith/types.h#L1498)
- instinct registry lookup: [`sithAI.c`](../../Libs/sith/AI/sithAI.c#L831)
- instinct entry points: [`sithAIInstinct.c`](../../Libs/sith/AI/sithAIInstinct.c#L49)
- puppet armed modes: [Types And Modes](../COG/Types-And-Modes.md#puppet-armed-modes)
