# UNI File Format

This document describes the text `.uni` string-table format used by OpenJones3D.

In the current codebase, `.uni` files are loaded by [`stdStrTable_Load()`](../../Libs/std/General/stdStrTable.c) and consumed by [`sithString_Startup()`](../../Libs/sith/Main/sithString.c).

## Purpose

A `.uni` file defines a keyed string table:

- each entry has a string key
- each entry stores one integer field
- each entry stores one text value, converted to a wide string at load time

At runtime, the parsed file becomes one [`tStringTable`](../../Libs/std/types.h).

## Where It Is Used

The current engine loads at least these tables:

- `misc\voiceStrings.uni`
- `misc\sithStrings.uni`

Those paths are hardcoded in [`sithString_Startup()`](../../Libs/sith/Main/sithString.c), with an additional legacy fallback to:

```text
\Jones3D\resource\misc\voiceStrings.uni
\Jones3D\resource\misc\sithStrings.uni
```

## Parser Rules

`.uni` files do not use `stdConffile`. They are parsed by the dedicated line reader in [`stdStrTable_Load()`](../../Libs/std/General/stdStrTable.c).

| Behavior | Current implementation |
| --- | --- |
| Header keyword | The first non-comment line must match `MSGS <count>` exactly enough for `sscanf("MSGS %d")` to parse it. |
| End keyword | The trailing `END` line is checked case-insensitively. |
| Entry key matching | Keys are stored and looked up case-sensitively through `stdHashtbl`. |
| Whole-line comments | Lines whose first non-space character is `#` are skipped. |
| Inline comments | Not stripped. `#` is only special when it begins the line after optional leading whitespace. |
| Empty lines | Skipped automatically. |
| Quoted strings | Required for the key and the value. |
| Escape syntax | No dedicated escape processing is implemented. The parser effectively uses the first and last double quote on the line as the string bounds. Embedded quotes therefore remain part of the value text. |

## File Layout

The canonical layout is:

```text
MSGS <count>
<entry 1>
<entry 2>
...
END
```

Each entry line has this shape:

```text
"<key>" <number> "<value>"
```

Example:

```C
MSGS 24
"SITHSTRING_AUTO" 0 "start_"
"SITHSTRING_QUICK" 0 "QUICKSAVE"
END
```

## Entry Columns

| Position | Meaning | Required | Notes |
| --- | --- | --- | --- |
| 1 | Key string | Yes | Parsed as a quoted literal. |
| 2 | Integer field | Yes | Stored in `tStringTableNode.unknown`. |
| 3 | Value string | Yes | Parsed as a quoted literal and converted to `wchar_t*`. |

## In-Memory Layout

| Structure | Field | Meaning |
| --- | --- | --- |
| `tStringTable` | `nMsgs` | Number of loaded entries. |
| `tStringTable` | `pData` | Flat array of parsed nodes. |
| `tStringTable` | `pHashtbl` | Hash table for key lookup. |
| `tStringTableNode` | `pKey` | Heap-allocated copy of the entry key. |
| `tStringTableNode` | `value` | Wide-string version of the text value. |
| `tStringTableNode` | `unknown` | Integer field read from column 2. |

The structure definitions live in [`Libs/std/types.h`](../../Libs/std/types.h).

## Duplicate Keys

If the same key appears more than once:

- the later line is still parsed into the flat `pData` array
- `stdHashtbl_Add()` logs an error because the key already exists
- the duplicate entry is not inserted into the lookup table

Practical consequence:

- key lookup returns the first inserted entry for a duplicate key
- later duplicates are effectively unreachable through normal `GetValue()` calls

## Count And END Validation

The loader expects both:

- the `MSGS <count>` header to match the number of entries that follow
- a trailing `END` line after the last declared entry

Failure modes:

- if `END` appears too early, the loader reports a premature end
- if the declared count is too small, the loader later reports missing `END`
- if the declared count is too large, the loader reaches EOF or malformed lines while expecting more entries

## Practical Authoring Notes

- Keep the `MSGS` count accurate.
- Quote both the key and the value.
- Treat keys as case-sensitive in practice, because lookup is case-sensitive.
- Prefer `#` for comments.
- Do not rely on inline comments.
- If you need literal double quotes inside the displayed text, keep the outermost value quotes intact. The current parser keeps interior quotes as part of the value.

## Error Handling Summary

Current behavior on malformed data:

- bad or missing `MSGS` header: load fails
- duplicate key: logged, but loading continues
- malformed entry line: logged, but loading continues as far as the current entry parse allows
- premature `END`: load reports failure
- missing trailing `END`: load reports failure

## References

- [`stdStrTable_Load()`](../../Libs/std/General/stdStrTable.c)
- [`stdStrTable_ReadLine()`](../../Libs/std/General/stdStrTable.c)
- [`stdStrTable_GetValue()`](../../Libs/std/General/stdStrTable.c)
- [`sithString_Startup()`](../../Libs/sith/Main/sithString.c)

