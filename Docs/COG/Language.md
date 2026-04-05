# COG Language

This page covers the text format and runtime model implemented by OpenJones3D's current COG parser and VM.

## File Layout

A COG script is split into the classic sections:

1. `flags`
2. `symbols`
3. `code`

`flags` is optional. `symbols` and `code` are required.

A minimal layout looks like this:

```C
flags=0x0

symbols
message startup
message activated
thing player local
flex delay=1.5 local
end

code
startup:
    Sleep(delay);
    return;

activated:
    return;
end
```

The exact whitespace and `name=value` formatting follows the same classic config parser used by the rest of the engine. In practice, most original scripts stay close to the structure above.

In the current implementation, COG source is effectively case-insensitive. The config reader lowercases script lines before tokenizing them, and the lexer lowercases identifiers before symbol lookup.

## Basic Syntax

COG syntax is broadly C-like, but it is not full C.

- variables, messages, and resource refs are declared in the `symbols` section, not inside the `code` section
- code is organized around message handlers such as `startup:` or `activated:`
- statements normally end with `;`
- blocks use `{ ... }`
- control flow is C-like, but the language is smaller and more specialized

The current grammar supports:

- assignments and expression statements
- `if` / `else`
- `while`
- `do ... while`
- `for`
- labels
- `goto`
- `call`
- `return`

The current lexer also recognizes some extra keywords from older variants, but they are not implemented by the active grammar:

- `switch`
- `break`
- `continue`

`stop` is accepted as a synonym for `return;` in the current lexer.

`end` is also a special lexer keyword, but it is not a runtime "exit" statement:

- in the `symbols` / `code` file structure, `end` closes a section
- inside the `code` parser stream, `end` terminates parsing of the code block
- it does not mean "exit the game" or "quit the application"

### Style And Naming

Because the language is case-insensitive, `sleep(delay);`, `Sleep(delay);`, and `SLEEP(delay);` all resolve the same way in the current implementation.

For readability, this doc set recommends:

- PascalCase for engine verbs, for example `Sleep`, `SetPulse`, `PlaySoundThing`
- camelCase for script variables, for example `isOpen`, `moveSpeed`, `triggerId`
- canonical lowercase for engine message names, for example `startup`, `activated`, `timer`

Example:

```C
symbols
thing player local
surface doorSurf
int isOpen=0 local
flex waitTime=0.25 local
vector openColor local
end

code
startup:
    openColor = '0.0 1.0 0.0';
    return;

activated:
    if (!isOpen)
    {
        Sleep(waitTime);
        SetSurfaceLight(doorSurf, openColor, 0.0);
        isOpen = 1;
    }
    return;
end
```

### Comments

Comment syntax depends on the section:

- in the `flags` / `symbols` style config reader, lines starting with `#` or `;` are comments
- in those config-style sections, inline `# ...` is also stripped
- in the `code` section, the lexer supports `# ...` and `// ...` line comments
- `/* ... */` comments are not supported by the current lexer
- in the `code` section, `;` is a statement terminator, not a comment marker

Example:

```C
symbols
# symbol comment
thing player local
flex delay=1.0 local    # inline config-style comment
end

code
startup:
    // code comment
    Sleep(delay); # also valid in code
    return;
end
```

### Other Practical Specifics

- Boolean-style conditions use the usual C convention: `0` is false and non-zero is true.
- Strings use double quotes, for example `"jones"`.
- Vector literals use single quotes, for example `'0 0 1'`.
- Labels are written as `labelName:` and can be targeted by `goto labelName;` or `call labelName;`.
- There are no local declarations inside `code`; if you need a variable, declare it in `symbols`.

## Script Flags

The optional `flags=0x...` line is copied into each loaded cog instance. The current engine flag bits are:

See [COG Script Flags](Flags.md#cog-script-flags) for the cross-reference used by the host-function docs.

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHCOG_DEBUG` | `0x001` | Enables extra runtime logging from the COG layer. |
| `SITHCOG_DISABLED` | `0x002` | Prevents the cog from handling messages until re-enabled. |
| `SITHCOG_PULSE_SET` | `0x004` | Internal runtime bit used while a pulse is active. |
| `SITHCOG_TIMER_SET` | `0x008` | Internal runtime bit used while a one-shot timer is active. |
| `SITHCOG_PAUSED` | `0x010` | Prevents message execution while the cog is paused. |
| `SITHCOG_CLASS` | `0x020` | Marks a class/template-style cog rather than a placed runtime cog instance. |
| `SITHCOG_LOCAL` | `0x040` | Prevents normal remote forwarding for multiplayer message sends. |
| `SITHCOG_SERVER` | `0x080` | Server-oriented flag preserved from the original system. |
| `SITHCOG_GLOBAL` | `0x100` | Global-scope compatibility flag preserved on the script/cog. |
| `SITHCOG_NOSYNC` | `0x200` | Suppresses many gameplay sync sends triggered by mutating host functions. |

The runtime bits such as `PULSE_SET` and `TIMER_SET` are usually managed by the engine rather than handwritten in source scripts, but the parser does preserve whatever value the script file supplies.

## Symbols Section

The parser accepts the following symbol types in the `symbols` section:

| Type | Meaning |
| --- | --- |
| `thing` | Thing reference |
| `surface` | Surface reference |
| `sector` | Sector reference |
| `sound` | Sound reference |
| `template` | Thing-template reference |
| `model` | 3DO model reference |
| `keyframe` | Keyframe reference |
| `cog` | Cog reference |
| `material` | Material reference |
| `ai` | AI class reference |
| `int` | Integer variable |
| `flex` / `float` | Floating-point variable |
| `vector` | Vector variable |
| `message` | Message handler label |

A symbol declaration generally looks like:

```C
<type> <name>[=<initializer>] [attributes...]
```

Example:

```C
symbols
message activated
thing player local
surface doorSurf linkid=1 mask=0x401
sound alarmSnd
int state=0 local
flex waitTime=0.5 local
end
```

### Symbol Attributes

The parser recognizes these attributes on symbol definitions:

| Attribute | Meaning |
| --- | --- |
| `local` | Keep the value local to the script instance instead of pulling it from placement/save data. |
| `desc` | Description text for tools that parse symbol metadata. |
| `mask` | Thing-type source mask used by linked thing/sector/surface refs. It controls which source thing types will trigger sending messages for that declared thing, sector, or surface symbol. |
| `linkid` | Sender link id exposed through [`GetSenderID()`](Functions-System.md#getsenderid). |
| `nolink` | Disable automatic link creation for thing/sector/surface refs. Equivalent to link id `-1`. |

`mask`, `linkid`, and `nolink` only matter for linkable references such as `thing`, `sector`, and `surface`.

For `mask`, the current parser default is `SITHTHING_TYPEMASK(SITH_THING_FREE, SITH_THING_PLAYER)`, which is `0x401`.

- `SITH_THING_FREE` acts as the "system / no source thing" bucket used when the engine sends a linked message without a concrete source thing.
- `SITH_THING_PLAYER` keeps the default link active for player-sourced events.
- Thing-type masks are built bitwise as `1 << thing_type_num`, exposed through the helper macros behind `SITHTHING_TYPEMASK(...)`.

### Primitive Variables Versus Reference Slots

- Primitive `int` and `flex` declarations are initialized directly from the symbol line.
- Reference declarations store a textual initializer and are resolved when the world opens.
- `vector` declarations in the current parser are zero-initialized in the symbol table. Vector literals are primarily a code-section feature rather than a rich `symbols`-section initializer feature.
- A symbol marked `local` is excluded from placement/save initialization and keeps its per-instance runtime value private to that cog instance.

### Message Declarations

`message <name>` does two things:

- binds the handler name to one of the engine message ids
- creates a symbol label so the code generator can jump to the handler body

If the message name does not exist in the global symbol table, parsing fails.

## Name Resolution

- The lexer lowercases identifiers before looking them up.
- A name missing from both the local and global symbol tables is created automatically as a new local float symbol with default value `0.0`.
- That auto-created symbol behavior is convenient for compatibility, but it also means typos can silently become new variables.

Even though the language is case-insensitive, consistent naming still helps readability. A practical convention is PascalCase for verbs and camelCase for variables.

## Literals And Expressions

The current grammar supports:

- identifiers
- integer constants
- floating-point constants
- quoted string literals
- vector literals written as `'x y z'`
- array indexing syntax `name[index]`
- function calls `verb(...)`
- arithmetic, comparison, bitwise, and logical operators

Supported control flow includes:

- `if`
- `if ... else`
- `while`
- `do ... while`
- `for`
- `goto`
- `call`
- `return`

`call label;` performs an internal code jump through the VM call stack. Host-function calls are still written in the normal `name(arg0, arg1)` style.

## Host Function Calling Convention

COG source writes arguments in the natural order:

```C
SetThingLight(thing, color, radius, timeDelta);
```

Internally the VM pops them from right to left. That matters when reading the C implementation: the last source argument is usually popped first.

The docs in this folder always write signatures in source order, not pop order.

### Generic `value` Type

Some host-function signatures use `value`, for example `HeapSet(int idx, value val)` and `HeapGet(int num) -> value`.

In these docs, `value` means the generic COG VM runtime value container, backed by `SithCogSymbolValue` in the engine.

In script-facing terms, that means it can hold the current resolved value of an expression or symbol, such as:

- `int`
- `flex`
- `vector`
- `string`
- reference-style values represented in the VM as integers, such as thing, sector, surface, cog, material, sound, template, model, keyframe, or AI references

When a symbol is passed into a verb such as `HeapSet`, the wrapper stores that symbol's current resolved value, not a live reference back to the symbol itself.

The engine also has internal pointer-tagged runtime values, but those are implementation details rather than a normal script-facing data type.

## Runtime Model

Every loaded cog instance has:

- its own duplicated symbol table
- a current execution position
- a current message type
- four integer message params
- a return value slot
- an optional private heap created by `HeapNew`

Script files are loaded from `cog\<name>`. Each runtime cog instance duplicates the script's symbol table, so placement values, message params, heap contents, and suspended execution state belong to the instance rather than the shared script definition.

## Placement, Linking, And Save Data

Placed world cogs store only their non-`local` symbol refs in placement/save data.

- non-`local` refs are read from placement text/binary and re-resolved when the world opens
- `local` refs stay inside the duplicated runtime symbol table and are not restored from world placement data
- only non-class cogs are written to the world cog-placement/save lists

For linked `thing`, `sector`, and `surface` refs:

- `linkid` becomes the value later exposed by [`GetSenderID()`](Functions-System.md#getsenderid)
- `mask` filters linked dispatch by source thing type, using `SITH_THING_FREE` as the synthetic "system/no source thing" type when the sender has no concrete source thing
- `nolink` disables automatic link creation by forcing the link id to `-1`

The engine sends linked thing messages in a specific order: direct thing cog first, capture cog second, then all cogs linked through symbol refs. Sector and surface links only use the linked-cog lists because sectors and surfaces do not own their own primary cog pointer in the same way things do.

The VM can stop and resume execution through a status machine. The current code uses statuses such as:

- sleeping
- waiting for a thing to stop
- waiting for a keyframe or sound to stop
- waiting for AI movement or head tracking
- waiting for weapon select/deselect
- waiting for the local player to stop

That is why verbs such as [`Sleep()`](Functions-System.md#sleep), [`WaitForSound()`](Functions-Sound.md#waitforsound), [`WaitForStop()`](Functions-Thing.md#waitforstop), [`WaitMode()`](Functions-Thing.md#waitmode), or [`AIWaitForStop()`](Functions-AI.md#aiwaitforstop) can suspend the current handler and continue later.

## Timers And Pulses

There are three main timing styles:

- [`Sleep(seconds)`](Functions-System.md#sleep) pauses the current handler and resumes it later.
- [`SetPulse(seconds)`](Functions-System.md#setpulse) causes the cog to receive recurring `pulse` messages.
- [`SetTimer(seconds)`](Functions-System.md#settimer) schedules a one-shot `timer` message on the cog itself.

[`SetTimerEx()`](Functions-System.md#settimerex) is slightly different and is documented in more detail in [Functions-System](Functions-System.md), because its first extra integer is used as the timer id / sender id rather than as a visible message param.

## Messages, Sender, And Source

Message handlers receive context through:

- [`GetSenderType()`](Functions-System.md#getsendertype)
- [`GetSenderRef()`](Functions-System.md#getsenderref)
- [`GetSenderID()`](Functions-System.md#getsenderid)
- [`GetSourceType()`](Functions-System.md#getsourcetype)
- [`GetSourceRef()`](Functions-System.md#getsourceref)
- [`GetParam(index)`](Functions-System.md#getparam)

The important distinction is:

- sender = the thing/surface/sector/cog the message is being dispatched from
- source = the secondary source object, if the caller supplied one
- sender id = the link id for linked dispatch, not the sender reference itself

See [Messages](Messages.md) for the full message list and the edge cases around `damaged`, `created`, `initialized`, and devmode.
