# Contributing

OpenJones3D is a function-by-function decompilation and reconstruction of the Jones3D engine. Changes should preserve the original subsystem boundaries and naming wherever possible, and modernize behavior only when the change is intentional and documented.

The main rule is simple: make reconstructed code readable and maintainable while keeping original engine behavior, ownership, and binary-reconstruction assumptions clear.

## Contents

- [Contribution Workflow](#contribution-workflow)
- [General Rules](#general-rules)
- [AI-Assisted Changes](#ai-assisted-changes)
- [Naming Conventions](#naming-conventions)
- [Module Layers And Helper Reuse](#module-layers-and-helper-reuse)
- [Compatibility And Behavior Changes](#compatibility-and-behavior-changes)
- [C Style](#c-style)
- [Memory And Ownership](#memory-and-ownership)
- [Decompilation Cleanup](#decompilation-cleanup)
- [Tests](#tests)
- [Commits](#commits)

## Contribution Workflow

1. Read the owning module, its tests, and the relevant [architecture](Docs/Architecture/README.md),
   [format](Docs/Formats/README.md), or [COG](Docs/COG/README.md) documentation.
   Establish the original contract and any intended behavior difference.
2. Make a focused change, add applicable regression coverage, and update the
   affected documentation. Follow [Tests](#tests) for expected results and
   original-engine fixture provenance.
3. Select and run the validation required by the affected paths. Build production
   code alongside engine tests; review links, commands, and configuration claims
   for documentation-only changes.
4. Review the staged diff for scope, temporary files, and accidental test controls
   in production. Describe the behavior change, evidence, validation results,
   and remaining gaps in the commit or review description.

## General Rules

- Keep changes scoped to the module or subsystem being worked on.
- Do not mix unrelated cleanup with a behavioral fix unless the cleanup is needed to make the fix safe.
- Preserve original file placement and module ownership. For example, `std` helpers should not move into `sith`, and gameplay logic should not drift into renderer modules.
- Prefer existing project helpers and conventions over new abstractions.
- When changing behavior, configuration, supported file formats, or reconstruction assumptions, update the relevant page under [Docs](Docs).
- Keep CHANGELOG entries focused on user-visible fixes and noteworthy engine behavior changes.
- Add or update tests for engine behavior changes and follow the
  [testing workflow](Docs/Testing.md#testing-workflow).

## AI-Assisted Changes

AI-assisted work is welcome, but it is treated the same as any other code
change: the contributor owns the final diff. Do not document or depend on a
specific assistant model in source or project docs unless that model is part of a
reproducible toolchain.

Before submitting AI-assisted changes:

- Review the generated diff manually against the surrounding code and original
  reconstruction intent.
- Follow the [testing workflow](Docs/Testing.md#testing-workflow), including
  the required coverage and MSVC validation. Report failed, ignored, and
  unexecuted checks explicitly.
- Preserve original engine behavior unless the change is clearly a fix,
  hardening change, QOL improvement, or documented compatibility option.
- Add the normal `// Added:`, `// Fixed:`, or `// Altered:` marker comments at
  changed original-code sites when the diff intentionally diverges from the
  original implementation.
- Keep prompts, scratch notes, and temporary generated files out of commits
  unless they are useful project documentation or checked-in test vectors.

## Naming Conventions

Original names win when they are known from symbols, logging strings, asserts, related engines, retail data, or nearby code. Prefer a recovered original name over a custom name even when the custom name looks cleaner.

- Function names use the owning source-file or subsystem prefix, followed by an underscore and a PascalCase behavior name: `<moduleOrFile>_<ActionObject>`.
- Examples: `stdFileUtil_NewFind`, `stdConffile_Open`, `sithThing_AddSwapEntry`, `JonesDisplay_Open`, `rdMaterial_Load`.
- Keep canonical source-file prefixes such as `std`, `rd`, `sith`, `Jones`, `wu`, and `Sound` when they match the recovered engine boundary.
- Follow local type naming in the file or module. Some original-style types use prefixes such as `t`, while rdroid and Sith data types often use subsystem names directly.
- Use meaningful names once behavior is understood. Temporary decompiler-style names are acceptable only while the meaning is still genuinely unknown.
- If a name is uncertain, prefer a conservative descriptive name over a guessed original name.
- Do not rename public functions, types, fields, or RTI symbols for style alone. Stable reconstructed names are part of the project API and make comparison against original binary analysis easier.

For new or cleaned-up type names, prefer the local module convention:

- `s` prefixes for struct tags when the file already uses them, for example `struct sJonesDisplaySettings`.
- `t` prefixes for typedef names when the reconstructed local convention uses them, for example `tHostServices` or `tSoundChannel`.
- `e` prefixes for enum tags when the file already uses them, for example `enum eSoundOpenFlags` or `enum eJonesStartMode`.

The original developers did not follow these prefixes perfectly, and recovered names still take priority. Do not refactor existing public types just to force a stricter `s` / `t` / `e` scheme.

Keep parameter, field, and local names consistent with surrounding reconstructed code. Common naming pieces include:

- Use camelCase for variable names, local constants, and function arguments unless a recovered original name or local module convention says otherwise.
- `p` for pointers: `pThing`, `pWorld`, `pFilename`.
- `pp` for pointer-to-pointer outputs when needed.
- `a` for arrays and fixed buffers: `aName`, `aVertices`, `aCurLevelFilename`.
- `b` for boolean-style state, even when represented as `int`: `bOpen`, `bWindowMode`, `bControlsActive`.
- `cur` for current state or current index: `curPos`, `curSubtitleDrawIndex`, `curTotal`.
- `num` for counts: `numVertices`, `numGobFiles`, `numSubtitleInfos`.
- `idx` or `index` for indexes, matching nearby code.
- `h` for handles where local code already uses handle naming.
- `pf`, `fp`, or `pfn` for function pointers, matching the local module.
- `pOut` / `pDst` / `pSrc` for output, destination, and source parameters when the role matters.

Use PascalCase for function behavior names, struct/type names that follow the reconstructed module style, and enum names when local code does so. Use upper-case names for preprocessor macros.

## Module Layers And Helper Reuse

Prefer existing functions, macros, and module helpers before adding new ones. Search the owning module and lower-level modules first; many common operations already have established helpers such as `STD_ARRAYLEN`, `STD_ZEROMEM`, `J3D_QOL_VALUE`, `STDMATH_CLAMP`, path helpers, config parsers, logging macros, and resource cleanup helpers.

Add a new helper only when it removes real duplication, clarifies a repeated original-engine concept, or belongs to the same abstraction level as the caller. Put new helpers in the lowest module that owns the concept, not in a higher module just because the first caller lives there.

Respect the engine dependency direction. Includes and link dependencies flow from higher-level modules down to lower-level modules, never the other way around:

```text
Jones3D -> sith -> sound -> rdroid -> std -> j3dcore
wkernel -> std -> j3dcore
w32util -> std -> j3dcore
```

Layer rules:

- `j3dcore` is the lowest OpenJones3D support layer and may be used by every module. It must not include or depend on `std`, `sound`, `rdroid`, `sith`, or `Jones3D`.
- `std` is a lower engine service layer. It may use `j3dcore`, but must not include or depend on upper gameplay or renderer layers such as `rdroid`, `sith`, or `Jones3D`.
- `sound` may use `j3dcore`, `std`, and `rdroid`; its existing spatial-audio paths depend on renderer math/data types. It must not depend on `sith` or `Jones3D`.
- `wkernel` and `w32util` may use `j3dcore` and `std`. Keep them independent of renderer, gameplay, and application layers.
- `rdroid` may use lower layers such as `j3dcore` and `std`, but must not depend on `sith` or `Jones3D`.
- `sith` may use lower engine layers such as `j3dcore`, `std`, `sound`, and `rdroid`, but must not depend on `Jones3D`.
- `Jones3D` is the application/game layer and may include lower modules.

If a desired helper would require a lower layer to include an upper layer, the helper belongs somewhere else or the dependency should be inverted through a callback, data-only type, or existing host-service path.

## Compatibility And Behavior Changes

OpenJones3D has both reconstruction goals and quality-of-life goals. Keep those categories visible in code.

- Preserve original behavior by default unless the change is explicitly a fix, hardening change, QOL improvement, or documented compatibility option.
- Put optional user-facing behavior changes behind an existing configuration path or compile-time option when appropriate.
- Bug fixes may apply to the legacy profile when they do not break original gameplay, save compatibility, scripts, or speedrun-relevant behavior.
- New functionality, feature additions, and improvements that replace buggy-but-authored behavior should normally be scoped behind `J3D_QOL_IMPROVEMENTS`, a config option, or both.
- Use `J3D_QOL_IMPROVEMENTS` or `J3D_QOL_VALUE(qolValue, legacyValue)` for behavior that intentionally diverges from the legacy profile.
- Use `J3D_SPEEDRUN_BUILD` for vanilla quirks and glitches that are intentionally preserved for speedrun compatibility.
- `JONES3D_QOL_IMPROVEMENTS=OFF` disables QOL behavior and enables speedrun behavior through CMake. Keep `JONES3D_RUNTIME_GUARDS=OFF` as well for original unchecked call contracts; enabling guards selects a hardened legacy profile.
- Keep legacy/original behavior readable when adding QOL branches. Avoid burying original logic under large unrelated refactors.
- Use `J3D_RUNTIME_GUARDS` / `STD_GUARD` style helpers for opt-in hardened release-safety argument checks. Guard-disabled profiles preserve the original unchecked call contracts.
- Avoid changing original struct layout unless the owning module is fully understood and the impact on hooks, RTI globals, savegames, and binary compatibility is clear.
- Additions to original-layout structs require extra care. Prefer external side tables, config state, or QOL-scoped fields when a change would affect binary comparison, save/load, RTI assumptions, or reverse-engineering tools.

Resource limits are owned by the relevant submodule, not by one global project file. When increasing or exposing a limit:

- Prefer a named constant in the owning module over a hardcoded numeric literal.
- Expose the limit through `Jones.cfg` when it is user-tunable or compatibility-sensitive.
- Keep non-configurable limits close to the parser, resource manager, renderer, or gameplay system that owns them.
- Update [Docs/Jones.cfg.md](Docs/Jones.cfg.md) when a new config key is added.
- Treat limit increases cautiously because scripts, savegames, rendering, multiplayer sync, and authored level data may rely on original limits.

When altering code that corresponds to original engine behavior, add a short marker comment at the changed site:

- `// Fixed:` for bug fixes.
- `// Added:` for new safety checks, helpers, or supported behavior.
- `// Altered:` for intentional behavior differences from the original.

Keep marker comments short and specific. They should explain why the reconstructed code differs from the original, not restate the code.

Examples:

```c
// Fixed: Reject invalid submodes before indexing the mode table.
if ( submode >= STD_ARRAYLEN(aModes) )
{
    return 0;
}
```

```c
// Altered: Use the QOL fixed-step rate instead of the original 50 Hz update.
float timestep = J3D_QOL_VALUE(configuredStep, legacyStep);
```

Do not add these marker comments to completely new modules where there is no original implementation being altered, unless the comment helps explain compatibility.

Use TODO comments only when they identify actionable future work:

- `// TODO:` for normal incomplete cleanup or investigation.
- `// TODO: [BUG]` for a known bug that is intentionally preserved or not fixed yet.
- `// TODO: [RE]` for reverse-engineering uncertainty that needs more original-code analysis.
- `// TODO: [COMPAT]` for behavior that may need a compatibility switch or legacy profile check.

TODO comments should name the unresolved problem and, when known, the expected direction for fixing it.

## C Style

- Use C11 for engine and test code.
- Follow the repository [.editorconfig](.editorconfig) for indentation, braces, pointer spacing, and general formatting. If an editor disagrees with local code, match the nearby code first and update `.editorconfig` only as a deliberate style change.
- Use four spaces for indentation.
- Put control-statement braces on separate lines and do not compress blocks into one line.
- Use spaces inside control-statement parentheses: `if ( condition )`, `while ( condition )`, `for ( size_t i = 0; i < count; ++i )`.
- Use spaces after commas and around binary and assignment operators unless aligning a local declaration block.
- Keep function calls tight to the name: `FunctionName(arg0, arg1)`.
- Define local variables close to first use instead of collecting declarations at the top of a function.
- When consecutive variable declarations include assignment initializers, align the assignment operator with the previous declaration line. A blank line starts a new alignment group.
- Keep pointer stars with the type side, matching the project style: `char* pName`, not `char *pName`.
- Use `NULL` for null pointers and null-pointer comparisons in C code and tests.
- Use structured parsers or existing helpers instead of ad hoc string manipulation when the codebase provides a suitable tool.
- Keep comments short and useful. Explain intent, original behavior, or compatibility reasons, not obvious assignments.

Example:

```c
float indexFloat  = normAngle * SIN_TABLE_DEGREES_TO_INDEX;
float fracPart    = indexFloat - floorf(indexFloat);
int32_t index     = (int32_t)indexFloat;
int32_t nextIndex = index + 1;

float sinLookup = 0.0f;
```

Do not align unrelated expressions, function calls, or declarations separated by a blank line.

## Memory And Ownership

Use the engine memory layer unless a subsystem has a specific reason to use a platform or backend allocator.

- Prefer `STDMALLOC`, `STDREALLOC`, and `STDFREE` for ordinary engine allocations so file/line tracking and `stdMemory` accounting stay useful.
- Prefer `STD_ZEROMEM` over raw `memset(..., 0, ...)` in reconstructed engine code when the local module already uses std helpers.
- Check allocation results before use unless the original function has a deliberate fail-fast contract.
- When allocating arrays, validate counts and multiplication sizes before allocating if the count comes from a file, config, savegame, network packet, or script.
- Prefer `sizeof(*pData)` style allocation sizes when the pointer type is already known. This keeps allocations correct if the pointed-to type changes.
- Initialize newly allocated structures when original code expects zeroed fields. Use `STD_ZEROMEM` after `STDMALLOC` unless a local constructor/helper already initializes every field.
- Keep allocation and ownership cleanup in the same module that owns the data structure.
- Use one cleanup path for partially initialized structures when a function has multiple allocation steps.
- Do not mix allocators. Memory allocated with `STDMALLOC` or `STDREALLOC` must be freed with `STDFREE` or `stdMemory_Free`.
- Direct `malloc`, `realloc`, and `free` are only appropriate for isolated third-party/backend code paths or tests that intentionally stub host allocation.
- Set pointer fields to `NULL` after freeing when the object can remain reachable or cleanup may be called more than once.
- Preserve original lifetime semantics for world resources, sound handles, COG state, savegame state, and RTI globals unless the change is an explicit fix.

When hardening an original allocation or buffer path, document the behavior change
and follow [Compatibility And Behavior Changes](#compatibility-and-behavior-changes).

For fixed-size buffers, prefer bounded writes and truncation checks. If truncation is acceptable, document or log it where the user or developer would need to know. If truncation would corrupt parser state, fail the operation instead.

## Decompilation Cleanup

Clean decompiled code in small, verifiable steps.

Identify whether a compatibility claim comes from original-engine execution,
recovered code, or inference. Record the reference binary/version and routine
where relevant, and keep unresolved analysis explicit.

- Preserve behavior first, then improve readability.
- Remove redundant temporaries when they do not document an important original value.
- Replace magic constants with named constants when the meaning is known or can be derived from table sizes, file format fields, or engine limits.
- Keep numeric constants as-is when the meaning is not known yet; add a short TODO only when it records a useful open question.
- Rename placeholder variables, globals, and functions once their meaning is known. Keep recognizable decompiler-style placeholders such as `v12`, `a3`, `dword_...`, or `sub_...` only while behavior is genuinely unknown.
- Avoid vague custom placeholders such as `unknownThingMaybe` or `idkValue`; use a clear name, remove the temporary, or keep the original-style placeholder.
- Fix incorrectly named reconstructed functions or types as soon as the real behavior is known, before the bad name spreads into more callers.
- Check for likely inlined original helpers before adding new local logic. Math, vector, matrix, parser, string, and cleanup code often corresponds to an existing `std`, `rdMath`, `rdVector`, `rdMatrix`, or module helper.
- Prefer clear structured control flow, but keep `goto` when it represents shared cleanup, error unwinding, or original control flow that is not safe to reshape yet.
- Convert decompiler label flow into structured loops and branches when the behavior is understood. Keep `goto` for cleanup/error unwinding or escaping complex nested flow when that remains clearer.
- Do not compress function signatures or initializer blocks in ways that make diffs harder to compare against decompiler output.
- When a function is being kept intentionally 1:1 with original decompiled behavior, avoid bug fixes and unrelated refactors in that pass.

## Tests

Engine behavior changes require focused tests, including regression coverage
for fixes. Tests use Unity with Unity Fixture and are registered through CTest.
Follow [Docs/Testing.md](Docs/Testing.md) for the workflow, coverage checklist,
fixture generation, layout, naming, build commands, and validation reporting.
These requirements apply equally to manual and AI-assisted changes.

Scope mock callbacks, local RTI storage, test compiler definitions, and dependency
substitutions to test targets and their source files. Review the generated build
configuration to confirm that production targets retain their normal dependencies
and contain no test controls or original-executable calls.

### Required Coverage

- Cover applicable success and failure paths, boundary cases, and preserved
  original quirks for every function or macro being tested.
- Verify return values, all output parameters, state and resource changes, and
  function-pointer/callback arguments, outputs, return values, and call order.
- Compare complete result strings for equality and serialized buffers by exact
  bytes and length. Verify cleanup after allocation and I/O failures.
- Keep original behavior, intentional fixes, malformed-input handling, runtime
  guards, and QOL expectations distinct.
- Put tests beside the owning module, use deterministic inputs, and name test
  files `<module>Test.c`.
- Comment larger, bulk, and specific edge cases in place, explaining what is
  tested and why the case matters. Small cases whose name, setup, and assertions
  make the purpose immediately clear can omit comments. Follow the
  [test comment guidance](Docs/Testing.md#test-case-comments).
- Do not pin release-version strings in macro tests; numbers and suffixes change.

### Original-Engine Fixtures

For reconstructed I/O, generate compatibility fixtures by executing the original
engine function as described in the
[original-engine vector guide](Docs/Testing.md#original-engine-io-test-vectors).
The debug build and v1.0 are the primary references. Use v1.2 only for a specific
fix absent from both primary versions that OpenJones3D implements. Verify the
[documented executable hashes](Docs/Testing.md#reference-binaries-and-version-policy)
and record provenance. Permanent tests must use saved fixtures without invoking
`Indy3D.exe` or depending on game assets. Consider attributed external corpora
where they provide independent format coverage.

### Build And Validation Requirements

Use MSVC Win32 in Debug and Release when changing engine code or tests. Build
and test both DX6 and DX9 when the changed production paths or related tests
use DirectX directly or indirectly, including shared `std` services and upper
modules in the graphics/input pipeline. This requirement applies even when no
test file is changed.

Backend-independent changes, such as an isolated `sith` physics calculation,
can use focused production builds and tests on one representative backend.
Follow [Backend Validation Scope](Docs/Testing.md#backend-validation-scope),
trace affected dependencies, and explain the selection in the validation report.
Exercise changed guard/QOL branches with the corresponding additional profiles.
Run relevant system tests on a suitable host and disruptive tests separately
when needed. Verify that the intended CTest entries were discovered and report
unavailable prerequisites, ignored cases, and unresolved failures explicitly.

Include the [validation report](Docs/Testing.md#validation-report) in the change
description or review reply. Documentation-only changes require link, command,
and configuration review rather than rebuilding unchanged engine code.

### Testing Review Checklist

Before submitting an engine change, check the following against
[Docs/Testing.md](Docs/Testing.md):

- [Regression proof](Docs/Testing.md#regression-proof): the test catches the old
  bug where practical and accompanies the fix; explain unavailable proof.
- [Isolation and cleanup](Docs/Testing.md#test-isolation-and-cleanup): each test
  works independently, restores shared state, and releases owned resources.
- [Numerical comparisons](Docs/Testing.md#numerical-comparisons): exact outputs
  remain exact; justified floating-point tolerances are documented.
- [Intermittent failures](Docs/Testing.md#intermittent-failures): initial failures
  and diagnostic reruns are retained, with unresolved causes reported.
- [Timeouts and shared devices](Docs/Testing.md#timeouts-and-shared-devices),
  where applicable: affected process/device registrations have appropriate bounds
  and conflicting host tests are scheduled separately.
- [Coverage reports](Docs/Testing.md#coverage-reports), when collected, have been
  reviewed for gaps in changed code and applicable configuration branches.

AddressSanitizer integration is
[deferred until full reimplementation](Docs/Testing.md#deferred-addresssanitizer-integration).

## Commits

Keep commits reviewable and grouped by module or fix theme.

Prefer one module per commit. If a fix naturally spans more than one module, use a cross-module prefix and explain why the split would be artificial.
Prefer one cleanup theme per commit. For example, rename placeholders in one commit, then simplify control flow or add tests in a separate commit.

Commit subjects should use a bracketed prefix for module or documentation changes:

- Format: `[<module>] Subject`
- Cross-module format: `[<module>,<module>] Subject`
- Do not add spaces inside the brackets.
- Use one space after the closing bracket.
- Use source-tree module names such as `std`, `rdroid`, `sith`, `sound`, `j3dcore`, `Jones3D`, or area names such as `Docs`.
- General project updates, such as CMake project maintenance, do not need a prefix.

Examples:

- `[std] Harden path and config parsing`
- `[rdroid] Fix clipping bounds checks`
- `[sith] Preserve voice playback handles on restore`
- `[j3dcore] Add macro tests`
- `[Docs] Document test conventions`
- `[std,sith] Preserve restored sound handles`
- `Update CMake project configuration`

When a change spans multiple layers, explain why the cross-module change is needed in the commit description.
