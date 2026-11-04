# Contributing

Jones3D EX is an enhanced source port of the Jones3D engine: the restored C game logic, inherited from the
OpenJones3D reconstruction, running on the Babylon Engine, a game-agnostic C++ platform framework. The
architecture and the reasons behind it are in [Docs/arc42](Docs/arc42/README.md).

The main rules are simple: keep the simulation measurable against the original game, keep Babylon free of
game code, and keep every change small, tested and explained.

## Contents

- [Contribution Workflow](#contribution-workflow)
- [General Rules](#general-rules)
- [AI-Assisted Changes](#ai-assisted-changes)
- [Licensing And The Babylon Boundary](#licensing-and-the-babylon-boundary)
- [Naming Conventions](#naming-conventions)
- [Module Layers And Helper Reuse](#module-layers-and-helper-reuse)
- [Compatibility And Behavior Changes](#compatibility-and-behavior-changes)
- [C Style](#c-style)
- [Babylon Code](#babylon-code)
- [Memory And Ownership](#memory-and-ownership)
- [Decompilation Cleanup](#decompilation-cleanup)
- [Tests](#tests)
- [Commits](#commits)

## Contribution Workflow

1. Read the owning module, its tests, and the relevant [architecture](Docs/arc42/05-building-blocks/README.md),
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
- When changing behavior, configuration, supported file formats, or architecture, update the relevant page under [Docs](Docs). Lasting decisions get an ADR in [Docs/arc42/09-decisions](Docs/arc42/09-decisions/README.md).
- Keep CHANGELOG entries focused on user-visible fixes and noteworthy engine behavior changes.
- Add or update tests for engine behavior changes and follow the
  [testing workflow](Docs/Testing.md#testing-workflow).

## AI-Assisted Changes

AI-assisted work is welcome, but it is treated the same as any other code
change: the contributor owns the final diff. Do not document or depend on a
specific assistant model in source or project docs unless that model is part of a
reproducible toolchain.

Before submitting AI-assisted changes:

- Review the generated diff manually against the surrounding code and the
  original behavior.
- Follow the [testing workflow](Docs/Testing.md#testing-workflow), including
  the required coverage and build validation. Report failed, ignored, and
  unexecuted checks explicitly.
- Follow the [fidelity rules](#compatibility-and-behavior-changes): simulation
  changes need an option, presentation is free to improve.
- Add the normal `// Added:`, `// Fixed:`, or `// Altered:` marker comments at
  changed sites in the game logic when the diff intentionally diverges from the
  original implementation.
- Keep prompts, scratch notes, and temporary generated files out of commits
  unless they are useful project documentation or checked-in test vectors.

## Licensing And The Babylon Boundary

The repository has two licenses:

- Everything outside `Babylon/` is AGPL-3.0, copyright OpenJones3D contributors.
- `Babylon/` is MPL-2.0, copyright seelenkuchen. Contributions to `Babylon/` need a signed Contributor
  License Agreement, which keeps dual licensing possible. Contributions outside `Babylon/` don't.

Babylon must stay separable and its license valid:

- Nothing in `Babylon/` includes from outside `Babylon/` or names a Jones3D or Sith concept.
- No code enters `Babylon/` from AGPL or GPL sources: `Libs/`, `Jones3D/`, earlier backends, OpenJones3D
  branches, Jones3D-Portable or PowerslaveEX. Read them for ideas, then write Babylon code fresh.
- Babylon dependencies are permissive only: zlib, MIT, BSD, Apache-2.0, public domain. No LGPL.
- CI builds Babylon on its own and checks includes and dependency licenses.

See [ADR-0006](Docs/arc42/09-decisions/ADR-0006-babylon-engine-in-repo.md),
[ADR-0007](Docs/arc42/09-decisions/ADR-0007-adapter-between-game-and-babylon.md) and
[ADR-0008](Docs/arc42/09-decisions/ADR-0008-babylon-license.md).

## Naming Conventions

These conventions apply to game and adapter code. Babylon has its own, see [Babylon Code](#babylon-code).

Original names win when they are known from symbols, logging strings, asserts, related engines, retail data, or nearby code. Prefer a recovered original name over a custom name even when the custom name looks cleaner.

- Function names use the owning source-file or subsystem prefix, followed by an underscore and a PascalCase behavior name: `<moduleOrFile>_<ActionObject>`.
- Examples: `stdFileUtil_NewFind`, `stdConffile_Open`, `sithThing_AddSwapEntry`, `JonesDisplay_Open`, `rdMaterial_Load`.
- Keep canonical source-file prefixes such as `std`, `rd`, `sith`, `Jones`, `wu`, and `Sound` when they match the recovered engine boundary.
- Follow local type naming in the file or module. Some original-style types use prefixes such as `t`, while rdroid and Sith data types often use subsystem names directly.
- Use meaningful names once behavior is understood. Temporary decompiler-style names are acceptable only while the meaning is still genuinely unknown.
- If a name is uncertain, prefer a conservative descriptive name over a guessed original name.
- Do not rename public functions, types or fields for style alone. Recovered names make comparison with the decompilation easier.

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
std, sound, wkernel (adapter) -> Babylon -> SDL3, SDL3 GPU, miniaudio
```

Layer rules:

- `j3dcore` is the lowest support layer and may be used by every module. It must not include or depend on `std`, `sound`, `rdroid`, `sith`, or `Jones3D`.
- `std` is a lower engine service layer. It may use `j3dcore`, but must not include or depend on upper gameplay or renderer layers such as `rdroid`, `sith`, or `Jones3D`.
- `sound` may use `j3dcore`, `std`, and `rdroid`; its existing spatial-audio paths depend on renderer math/data types. It must not depend on `sith` or `Jones3D`.
- `wkernel` and `w32util` may use `j3dcore` and `std`. Keep them independent of renderer, gameplay, and application layers.
- `rdroid` may use lower layers such as `j3dcore` and `std`, but must not depend on `sith` or `Jones3D`.
- `sith` may use lower engine layers such as `j3dcore`, `std`, `sound`, and `rdroid`, but must not depend on `Jones3D`.
- `Jones3D` is the application/game layer and may include lower modules.
- `Babylon` includes nothing from the game. Only the adapter (`std`, `sound`, `wkernel`) calls Babylon; game
  modules above it keep calling the recovered adapter functions.

If a desired helper would require a lower layer to include an upper layer, the helper belongs somewhere else or the dependency should be inverted through a callback, data-only type, or existing host-service path.

## Compatibility And Behavior Changes

Jones3D EX separates the **simulation** (physics, collision, AI, COG, game time, savegames) from the
**presentation** (rendering, resolution, audio output, UI). See
[ADR-0003](Docs/arc42/09-decisions/ADR-0003-fidelity-split.md) and
[chapter 8](Docs/arc42/08-crosscutting/README.md).

- Presentation may improve freely. Offer a classic toggle where players would want one.
- The simulation is measured against retail behavior by headless traces. The QOL changes inherited from
  OpenJones3D are on by default, each switchable back to retail.
- A new simulation change needs an option, on by default like the QOL changes, and the Retail profile turns
  it off. With the Retail profile, the retail trace must stay
  identical.
- Pure bug and crash fixes are always on. If a fix changes the retail trace, ask first: speedruns, savegames
  and scripts may depend on the bug.
- Use `J3D_QOL_IMPROVEMENTS` or `J3D_QOL_VALUE(qolValue, legacyValue)` for existing QOL branches until they
  become runtime options. Use `J3D_SPEEDRUN_BUILD` for preserved vanilla quirks.
- Use `J3D_RUNTIME_GUARDS` / `STD_GUARD` style helpers for opt-in hardened release-safety argument checks.
- Keep the simulation deterministic: no wall-clock time, thread timing, uninitialized memory or pointer values
  in anything that reaches game state.
- In-memory struct layouts may change. On-disk formats (savegames, game data) may not: read and write them
  through explicit serialization, never by reading bytes into a struct.

Resource limits are owned by the relevant submodule, not by one global project file. When increasing or exposing a limit:

- Prefer a named constant in the owning module over a hardcoded numeric literal.
- Expose the limit through `Jones.cfg` when it is user-tunable or compatibility-sensitive.
- Keep non-configurable limits close to the parser, resource manager, renderer, or gameplay system that owns them.
- Update [Docs/Jones.cfg.md](Docs/Jones.cfg.md) when a new config key is added.
- Treat limit increases cautiously because scripts, savegames, rendering, multiplayer sync, and authored level data may rely on original limits.

When altering original code in the game logic (`Jones3D/`, `Libs/sith/`, `Libs/rdroid/`, `Libs/smush/`), add a short marker comment at the changed site:

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

The markers are only for the game logic. Do not use them in the adapter (`Libs/std/`, `Libs/sound/`, `Libs/wkernel/`, `Libs/w32util/`), in Babylon, or in completely new game modules where there is no original implementation being altered.

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

## Babylon Code

Babylon is C++ and is not bound by the recovered-name conventions above. Its naming, style and C++ standard
are set at the start of milestone M5 and documented in `Babylon/`. Until then:

- Interfaces are designed for their role (window, input, render, audio, files, time, cvars), not for one
  game's calls. Each header states what every function promises: ownership, lifetime, units, errors, and
  when it may be called.
- Every interface has a Null backend that honors its contract without a device.
- Ownership is expressed in C++ types (RAII, owning smart pointers) inside Babylon. Whatever crosses an
  interface states who owns it.
- No platform types (Windows, DirectX, SDL) in Babylon's public headers.
- Babylon's tests live in `Babylon/` and run against every backend, including Null.
- Adapter files in `Libs/` may be C++ and export the recovered functions as `extern "C"`.

Babylon is split into modules, one per role (platform, input, time, files, render, audio, cvars, threads,
log, core), about 8 to 10 in total. See
[ADR-0015](Docs/arc42/09-decisions/ADR-0015-babylon-modules-as-cmake-targets.md).

- Each module is its own CMake target, declared with `babylon_add_module(<name> DEPS <modules>)`.
- Public headers live in `<module>/include/babylon/<module>/`, everything else in `<module>/src/` and
  `<module>/backends/<backend>/`. Only public headers are visible to other modules, and only to modules
  that declare the dependency.
- Declare a dependency `PUBLIC` only when it appears in the module's public headers, otherwise `PRIVATE`.
- No cyclic dependencies between modules.
- No static self-registration: backends are registered explicitly at startup, because the linker drops
  unreferenced objects from static libraries.
- CI compares the real target graph with the allowed dependency list. Changing that list is an
  architecture change and needs an ADR or an update of ADR-0015.

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
- Preserve original lifetime semantics for world resources, sound handles, COG state and savegame state unless the change is an explicit fix.

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

Scope mock callbacks, test compiler definitions, and dependency
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

Expected results for original behavior come from the original game, never from the reconstructed code
itself. The existing fixtures were captured by executing original engine functions, as described in the
[original-engine vector guide](Docs/Testing.md#original-engine-io-test-vectors); keep their provenance. New
vectors come from the original game or the byte-exact decompilation, with recorded provenance. Permanent
tests must use saved fixtures without invoking `Indy3D.exe` or depending on game assets.

### Build And Validation Requirements

Build and run the unit tests natively (Clang) for every engine change; see
[Docs/Testing.md](Docs/Testing.md#running-tests) for the commands. Code that the legacy Win32 and DirectX 9
build compiles (until milestone M6) also needs a Windows build, with MinGW or MSVC. Changes to the simulation
also need a headless trace comparison against the retail profile. Changes to Babylon need its tests against
every backend.

Exercise changed guard/QOL branches with the corresponding additional profiles. Verify that the intended
CTest entries were discovered and report unavailable prerequisites, ignored cases, and unresolved failures
explicitly.

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

Native builds can run under AddressSanitizer and UndefinedBehaviorSanitizer with
`-DJONES3D_SANITIZE=ON` (GCC; Clang needs its sanitizer runtime installed).

## Commits

Keep commits reviewable and grouped by module or fix theme.

Prefer one module per commit. If a fix naturally spans more than one module, use a cross-module prefix and explain why the split would be artificial.
Prefer one cleanup theme per commit. For example, rename placeholders in one commit, then simplify control flow or add tests in a separate commit.

Commit subjects should use a bracketed prefix for module or documentation changes:

- Format: `[<module>] Subject`
- Cross-module format: `[<module>,<module>] Subject`
- Do not add spaces inside the brackets.
- Use one space after the closing bracket.
- Use source-tree module names such as `std`, `rdroid`, `sith`, `sound`, `j3dcore`, `Jones3D`, `Babylon`, or area names such as `Docs`.
- General project updates, such as CMake project maintenance, do not need a prefix.

Examples:

- `[std] Harden path and config parsing`
- `[rdroid] Fix clipping bounds checks`
- `[sith] Preserve voice playback handles on restore`
- `[j3dcore] Add macro tests`
- `[Docs] Document test conventions`
- `[std,sith] Preserve restored sound handles`
- `[Babylon] Add the Null input backend`
- `Update CMake project configuration`

When a change spans multiple layers, explain why the cross-module change is needed in the commit description.
