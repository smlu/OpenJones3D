<a name="testing"></a>

# 🧪 Testing

Jones3D EX uses Unity with Unity Fixture for C tests and CTest as the test runner.
Tests are enabled by default through `JONES3D_BUILD_TESTS`. Babylon's C++ tests
live in `Babylon/`; their framework is chosen in milestone M5. Behavior of the
whole game is checked with [headless traces](#trace-tests). Contributors and
LLM agents should follow the same workflow and coverage requirements below.

Much of this guide comes from OpenJones3D and still describes the legacy Win32
and DirectX build, which exists until milestone M6. Native builds are the
default from now on; see [Running Tests](#running-tests).

<a name="contents"></a>

## 📑 Contents

- [🔄 Testing Workflow](#testing-workflow)
- [🎯 Coverage Requirements](#coverage-requirements)
    - [🐛 Regression Proof](#regression-proof)
    - [📐 Numerical Comparisons](#numerical-comparisons)
    - [📊 Coverage Reports](#coverage-reports)
- [🗂️ Test Layout](#test-layout)
- [✍️ Test Style](#test-style)
    - [🧹 Test Isolation And Cleanup](#test-isolation-and-cleanup)
- [🧭 Test Scope](#test-scope)
    - [🎞️ Trace Tests](#trace-tests)
- [📦 Original-Engine I/O Test Vectors](#original-engine-io-test-vectors)
- [▶️ Running Tests](#running-tests)
    - [🤖 CI Profiles](#ci-profiles)
    - [🐧 Native CI Columns](#native-ci-columns)
    - [🔎 Intermittent Failures](#intermittent-failures)
    - [⏱️ Timeouts And Shared Devices](#timeouts-and-shared-devices)
- [🖼️ Visual System Tests](#visual-system-tests)
- [📋 Validation Report](#validation-report)
- [🚧 Planned Test Integrations](#planned-test-integrations)
    - [🛡️ AddressSanitizer In CI](#deferred-addresssanitizer-integration)

<a name="testing-workflow"></a>

## 🔄 Testing Workflow

Define the contract, establish expected results, add focused tests, build and run
them, then report the results and remaining gaps.

1. **Identify the contract.** Read the owning module and its existing tests.
   Decide whether the change preserves original behavior or introduces a fix,
   runtime guard, or QOL behavior. Check the actual CMake targets and CTest
   entries; descriptions marked **planned** do not establish that a suite exists.
2. **Choose cases and expected results.** Cover the success and failure paths
   listed in [Coverage Requirements](#coverage-requirements). For reconstructed
   I/O, follow the [original-engine capture procedure](#generation-procedure)
   and [reference-version policy](#reference-binaries-and-version-policy).
   For fixes, plan [regression proof](#regression-proof); document any numerical
   tolerance using [Numerical Comparisons](#numerical-comparisons).
3. **Implement focused tests.** Put tests and fixtures beside the owning module,
   follow [Test Style](#test-style), and register them with CTest. Comment larger,
   bulk, and specific edge cases beside the code: explain what they test and
   why they matter; see [Test Case Comments](#test-case-comments). Follow
   [Test Isolation And Cleanup](#test-isolation-and-cleanup). Keep permanent
   tests independent of the original executable and game assets.
4. **Build and run.** Build natively and [discover and run the unit suites](#native-unit-builds).
   Add the builds required by [Validation Scope](#backend-validation-scope):
   the Windows legacy build for code it compiles, a [trace comparison](#trace-tests)
   for simulation changes, and every backend for Babylon changes. Build
   production code as well as tests, then exercise changed runtime-guard or QOL
   branches in their configuration.
5. **Check host-dependent behavior.** Run relevant [system tests](#system-test-runs)
   on a suitable desktop. Run [disruptive tests](#disruptive-test-runs) separately
   when that coverage is needed. Record unavailable prerequisites and ignored
   cases; they do not count as exercised behavior. Verify the registration's
   [timeout and device scheduling](#timeouts-and-shared-devices).
6. **Review and report.** Inspect the final diff and fixture provenance. Rerun
   regenerated vectors with update modes disabled, then provide the
   [validation report](#validation-report), including any remaining gaps and
   [intermittent failures](#intermittent-failures).

Documentation-only changes require review of links, commands, and configuration
claims; they do not require rebuilding unchanged engine code.

<a name="coverage-requirements"></a>

## 🎯 Coverage Requirements

For every function or macro being tested, cover each applicable success and
failure path within its call contract. Separate preserved original behavior
from intentional fixes, malformed-input rejection, runtime guards, and QOL
expectations.

<a name="cases-and-observable-results"></a>

### ✅ Cases And Observable Results

- Exercise empty, minimal, typical, boundary-size, and relevant type/flag cases.
  Include invalid inputs where the API defines their handling.
- Check the return value and every observable output: output parameters, changed
  object/global state, generated files or buffers, resource ownership, and logs
  where they are part of the behavior being tested.
- Check function-pointer and callback behavior: call order, arguments, output
  parameters, return values, and their effect on the caller's result.
- Compare complete strings for equality, with termination checked where the API
  requires it. Compare complete serialized bytes and lengths; a prefix,
  substring, checksum alone, or successful round trip is insufficient.
- Cover allocation and I/O failures where applicable. Verify partial cleanup,
  released resources, and whether outputs remain unchanged or reset on failure.
- Use appropriate Debug/Release and guard/QOL configurations for branches that
  are compiled out elsewhere. Do not invoke unchecked invalid-input paths just
  to make a guard-disabled profile appear to cover rejection behavior.

<a name="regression-proof"></a>

### 🐛 Regression Proof

For a bug fix, demonstrate where practical that the new regression case fails
for the intended reason with the old behavior and passes with the fix. A build
failure or missing prerequisite does not establish that the assertion catches
the bug. Use an isolated checkout or temporary test build for this comparison;
keep the final production code and fixtures intact.

Include the regression test in the same change as the fix. If the old behavior
cannot be exercised reliably, record the reason and the available evidence.
Review whether the assertions would detect a recurrence of the original bug.
See [Google's test review guidance](https://google.github.io/eng-practices/review/reviewer/looking-for.html#tests).

<a name="numerical-comparisons"></a>

### 📐 Numerical Comparisons

Keep integers, strings, serialized bytes, and representation-sensitive original
behavior exact. For calculated floating-point results, use a documented absolute
or relative tolerance only when the numerical contract permits rounding
variation. Explain the tolerance's scale and purpose beside the case; preserve
exact checks wherever the original behavior or file format requires them.

Unity's `TEST_ASSERT_EQUAL_FLOAT` performs an approximate comparison.
`TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)` supplies an explicit absolute
tolerance. Use representation comparisons for bit-exact expectations and check
NaN, infinity, and signed zero explicitly when they are part of the contract.
An unexpected compatibility difference needs investigation before changing a
tolerance or expected vector.
See [Unity's assertion reference](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md#floating-point-if-enabled).

<a name="coverage-reports"></a>

### 📊 Coverage Reports

Coverage measurement is optional until suitable tooling is integrated and
validated with the supported MSVC build. When reports are available, review
uncovered lines and branches in changed code, including backend and guard/QOL
variants. Add meaningful cases or explain exclusions and remaining gaps.

Coverage records execution; review the assertions and edge cases as well. A
percentage alone does not establish correctness, and there is no universal
percentage target for this project.
See [Google's coverage guidance](https://testing.googleblog.com/2020/08/code-coverage-best-practices.html).

<a name="test-data-and-readability"></a>

### 📝 Test Data And Readability

Use independently established expected results. Reconstructed code must not
produce its own compatibility expectations. Consider attributed external corpora
for supported formats and original-engine vectors for reconstructed I/O. Keep
malformed-input regressions clearly identified and document fixture provenance.

Keep cases deterministic and focused on behavior. Use fixed local seeds and
controlled time, callbacks, and host dependencies. Tests should explain a
contract or regression rather than repeat the implementation's calculations.
Use `NULL` for null-pointer expectations in C test code.

<a name="test-case-comments"></a>

### 💬 Test Case Comments

Place comments beside the case or immediately before a related block of cases.
For larger tests, bulk/table-driven cases, and specific edge cases, explain both
**what behavior is being tested** and **why the case matters**. Describe the
boundary, preserved quirk, regression, or chosen data that makes the case useful.
For a loop or case table, explain the shared purpose and comment individual
entries where a particular edge case needs its own explanation.

Small, straightforward cases can omit comments when the name, setup, and
assertions immediately show what they test. Comments should explain intent,
not repeat obvious assignments or assertions.

<a name="test-layout"></a>

## 🗂️ Test Layout

Place tests next to the module or subsystem they cover, using a local `Tests` directory. The project convention is plural `Tests`, with a capital `T`.

Examples:

- `Libs/j3dcore/Tests`
- `Libs/std/Tests`
- `Libs/std/General/Tests`
- `Libs/rdroid/Primitives/Tests`
- `Libs/sith/World/Tests`
- `Libs/sith/Cog/Tests`
- `Libs/sound/Tests`
- `Libs/smush/Tests`
- `Jones3D/Main/Tests`

Prefer the narrowest source-area owner that makes sense. For example, tests for `stdFnames` belong under `Libs/std/General/Tests`, while tests for voice playback state belong under `Libs/sith/World/Tests`.

Subareas can keep their own colocated test files and fixtures while still sharing the parent module test executable. For example, `Libs/std/Win95/Tests/stdGobTest.c` is owned by `std/Win95` and runs through the Win95 suite inside the `stdTests` executable.

Avoid collecting all tests in one repository-level directory. Keeping tests beside the reconstructed module helps preserve the original engine boundaries and makes ownership clearer.

<a name="test-style"></a>

## ✍️ Test Style

<a name="naming-and-ownership"></a>

### 🏷️ Naming And Ownership

Use Unity Fixture groups for module-level test files. Name test groups after the subsystem or source file under test, such as `stdFnames`, `stdConffile`, `rdClip`, `sithVoice`, or `AudioLib`.

Name test source files after the module followed by `Test.c`, matching the reconstructed codebase's C naming style. For example, use `stdMathTest.c` for `stdMath.c`.

When a module has different DirectX 6 and DirectX 9 implementations, split backend-specific unit tests by backend instead of hiding `#ifdef` branches inside a broad test file. For example, keep DirectPlay-backed communication checks in `stdCommDX6Test.c` and DX9/stub communication checks in `stdCommTest.c` or `stdCommDX9Test.c` when the source module is split that way.

Name system test source files after the exact module or backend they exercise, followed by `SystemTest.c`. For example, use `stdDisplayDX9SystemTest.c` for DirectX 9 display device coverage and `stdDisplayDX6SystemTest.c` for DirectX 6 display device coverage.

<a name="test-executables-and-registration"></a>

### 🧩 Test Executables And Registration

Prefer one executable per source area or module boundary, with shared fixture
startup and a stable module-level CTest entry.

| Executable | CTest entry | Coverage | Integration |
| --- | --- | --- | --- |
| `j3dcoreTests` | `j3dcore.Macros` | Core macros | Available |
| `rdroidTests` | `rdroid.Math` | Vector and matrix debug-source numerical fidelity | Available |
| `stdTests` | `std` | General and deterministic Win95 services | Available |
| `stdSystemTests` | `std.System` | Win95 windows and DirectX devices | Available |
| `stdDisruptiveSystemTests` | `std.DisruptiveSystem` | Opt-in fullscreen/device changes | Available |
| `soundTests` | `sound` | Deterministic lip-sync generation and lookup, AudioLib codec and header vectors | Available |
| `smushTests` | `smush` | INSANE movie player: core, bitmap, codec, sound, stream areas and original-engine assembly vectors | Available |
| `sithTests` | `sith` | World thing/template I/O, voice lip-sync and the cogstatus console command bounds | Available |
| `sithThingQuetzVecTests` | `sith.ThingQuetz` | Original-engine vectors of the Quetz tail functions (world-state harness, `Libs/sith/Tests/Vec`) | Available |
| `sithAIInstinctVecTests` | `sith.AIInstinct` | Original-engine vectors of the instinct registration (world-state harness) | Available |
| `sithAIInstinctFnVecTests` | `sith.AIInstinctFn` | Original-engine vectors of the instinct functions (world-state harness) | Available |
| `sithAIMoveVecTests` | `sith.AIMove` | Original-engine vectors of AI movement and turning (world-state harness) | Available |
| `sithAIUtilVecTests` | `sith.AIUtil` | Original-engine vectors of the AI utility functions: waypoints, sight, firing, path checks (world-state harness) | Available |
| `sithDSSTests` | `sith.DSS` | Original-engine vectors of the DSS and DSS thing save/network serialization functions | Available |
| `sithMultiTests` | `sith.Multi` | Original-engine vectors of multiplayer state and synchronization | Available |

Name CTest entries after the owning module when the executable covers that whole module, such as `std`. When an executable covers only a subarea, use the owning module first and a short area name second, separated by a dot, such as `j3dcore.Macros`. Use labels that match the owning module, such as `j3dcore`, `std`, `rdroid`, `sith`, `sound`, or `Jones3D`.

Backend-specific test builds should also label the CTest entry with the active backend, such as `DirectX6`, `DirectX9`, or later `OpenGL`, so filtered test runs can choose the intended API surface.

<a name="fixture-structure"></a>

### 🏗️ Fixture Structure

Test files should normally provide:

- one `TEST_GROUP`
- one `TEST_SETUP`
- one `TEST_TEAR_DOWN`
- one `TEST_GROUP_RUNNER`
- focused `TEST` cases for the behavior under test

Include Unity headers as external headers, for example `#include <unity_fixture.h>`.

Follow the owning module's coding conventions, including local declarations
near first use and aligned assignment initializers in consecutive declaration blocks.

<a name="test-isolation-and-cleanup"></a>

### 🧹 Test Isolation And Cleanup

Every test must work individually and in any order. Initialize its own state;
never depend on a previous test leaving globals, files, or resources behind.
A scenario that needs several ordered operations belongs in one test with its
own setup and assertions for those operations.

Restore globals, callback bindings, allocation-failure controls, clocks, random
state, environment/current-directory changes, and host state modified by the
case. Give temporary files/directories unique ownership and remove them during
cleanup. Release allocations, handles, devices, windows, and replaced hooks in
teardown, including after failed assertions or partial setup.

Shared fixture helpers may reduce duplication, but each case must establish its
own preconditions. Unit tests should use controlled dependencies and local
fixtures; real devices and external services belong in labelled system tests.
See [Google's test isolation guidance](https://testing.googleblog.com/2010/12/test-sizes.html).

<a name="shared-test-support-and-macro-assertions"></a>

### 🔧 Shared Test Support And Macro Assertions

Tests that need host-service assertion capture should link `j3dTestSupport` and include `#include <j3dcore/Tests/j3dTest.h>`. Use `J3DTest_Startup()` and `J3DTest_BindHostServicesGlobal((uintptr_t)&<module>_g_pHS)` in `TEST_SETUP`, then `J3DTest_Shutdown()` in `TEST_TEAR_DOWN`. Some older tests pass a `<module>_g_pHS_ADDR` macro that their test header defines as that address; the retail addresses these names once stood for are gone ([ADR-0004](arc42/09-decisions/ADR-0004-drop-binary-compatibility.md)).

For header and macro tests, prefer stable behavior checks over release-specific value checks. For example, `j3d.h` macro tests should verify string/memory helpers, runtime guard behavior, platform flags, logging/assertion macros, and compile-time shape, but should not pin exact `J3D_VERSION_STRING` or `J3D_VERSION_FULL` values because those change with release numbers and suffixes.

<a name="test-scope"></a>

## 🧭 Test Scope

<a name="backend-validation-scope"></a>

### 🖥️ Validation Scope

Choose validation from the changed production paths and test dependencies, not
only the module name or whether a test file was edited.

| Changed code or tests | Required validation |
| --- | --- |
| Any engine code | Native build (Clang), unit suites in Debug |
| Code the legacy Win32/DirectX 9 build compiles (until M6) | Also a Windows build (MinGW or MSVC) with its unit suites. DX6 is paused and not required |
| Simulation behavior (physics, collision, AI, COG, game time, savegames) | Also a [trace comparison](#trace-tests) against the retail profile, and against the Enhanced profile when an enhancement changed |
| Memory handling, file or savegame parsing, 64-bit portability | Also a sanitizer build (`-DJONES3D_SANITIZE=ON`) |
| Babylon interfaces or backends | Babylon's own tests against every backend, including Null, and its standalone build |

Trace the affected dependencies and explain the selected validation scope in the report.

<a name="trace-tests"></a>

### 🎞️ Trace Tests

A trace test runs the whole game headless: it loads a savegame, feeds scripted
input for a fixed number of frames with fixed game time, and writes the
simulation state. Comparing state files shows whether a change altered the
simulation. See [ADR-0003](arc42/09-decisions/ADR-0003-fidelity-split.md) and
[chapter 8](arc42/08-crosscutting/README.md).

```bash
cd <game>/Resource
<build>/Jones3D/Jones3D-headless --load ../SaveGames/<save>.nds --frames 1000 --state state.txt
```

Both non-Windows columns build `Jones3D-headless`; the Babylon column
(`-DJONES3D_PLATFORM_BABYLON=ON`) must give the same state file as the Null column for the same run.
`--input <file>` feeds a scripted-input file of `frame <n>: cmd <command>` and
`frame <n>: key <name|number> down|up` lines; scripted keys take effect at the next
`stdControl_ReadControls`, as device input does on Windows.

The state file starts with one `cvar <name> <value>` line per simulation cvar (today
`cvar game.profile enhanced` or `retail`), followed by the frame, game time, player state and one line per
active thing.

Since M5 native builds use Microsoft's `rand` generator (the `j3dcrt` shim), as the original does, so native
traces follow the retail random sequence ([research](Research/Time-Sources.md)).

Rules:

- The reference for the retail profile is a build of the tagged baseline with
  all simulation options set to retail. The Enhanced profile (the default) is compared
  against the same baseline with its enhancements on.
- Identical state files are the expected result. Any difference is investigated
  before anything else changes; reference traces are never edited to make a
  change pass.
- The same run must give identical state files on 32-bit, 64-bit and sanitizer
  builds.
- Traces need game data, so they are not permanent unit tests. Record the
  savegame, input script, frame count and build in the validation report.

**Status:** the headless runner, scripted input and state files exist and are
deterministic across 32-bit, 64-bit and sanitizer builds. Selecting the retail
or Enhanced profile at run time waits for the QOL options to become runtime
options; until then, the profile is the build's `JONES3D_QOL_IMPROVEMENTS`
setting.

<a name="deterministic-unit-and-regression-tests"></a>

### 🔬 Deterministic Unit And Regression Tests

Start with deterministic unit and regression tests for parser, file, math, resource, and state-machine behavior.
Integration tests can still live near the module that owns the scenario, but should use CTest labels such as `integration`, `network`, `slow`, or `gui` when they require extra orchestration.

For parsers and file formats, prefer deterministic fuzz-style corpora in the normal unit tests before adding a separate fuzzing harness. Good examples are generated malformed JSON strings, exhaustive small truncation sweeps for binary containers, boundary-size argument lists, and generated round-trip vectors. These tests should be reproducible, fast enough for every CTest run, and should not use process-wide random seeds.

The deterministic `stdTests` executable also owns process-isolated death tests when the production contract necessarily terminates the process. `stdPlatform` re-enters the same test executable in a private temporary directory, raises an unhandled exception after installing the production exception filter, and verifies the resulting `core.dmp` signature, version, stream directory bounds, and exception exit code. Keep child-only modes out of the normal Unity runner and suppress host error-reporting dialogs so CI cannot block on them.

The `stdBmp` tests include generated vectors plus selected public-domain images from BMP Suite 2.8. Their provenance, expected checksums, and malformed-file purpose are recorded in `Libs/std/General/Tests/tv/stdBmp/README.md`. Prefer extending that attributed corpus over copying unknown BMP files from the web.

<a name="system-and-device-tests"></a>

### 🖥️ System And Device Tests

System tests are integration-style tests that touch host or device services such as Win32 windows, DirectInput, DirectDraw, Direct3D, OpenGL, audio devices, or filesystem locations outside small test vectors. Keep them in separate executables from deterministic unit tests and label them with `system`; add more specific labels such as `device`, `Win95`, `DirectX`, `OpenGL`, `audio`, or `gui` as appropriate.

System/device tests may use Unity ignore results when the host cannot provide the needed service, for example a headless CI worker without a render device. They should still fail normally when the service was available and the engine behavior is wrong.

When multiple system tests share host setup, keep that setup in a small local support file and keep the test cases themselves in the owning module files. For example, Win95 tests can share Win32 test-window setup through `stdWin95SystemTestSupport.c`, while DirectInput, display, and 3D checks live in their own `stdControlDX*SystemTest.c`, `stdDisplayDX*SystemTest.c`, and `std3DX*SystemTest.c` files.

<a name="disruptive-system-tests"></a>

### ⚠️ Disruptive System Tests

Disruptive system tests are disabled by default through `JONES3D_BUILD_DISRUPTIVE_SYSTEM_TESTS=OFF`. Use them for scenarios that can change fullscreen display modes, reset or recreate graphics devices, or disturb desktop input focus. Keep them in separate source files named `*DisruptiveSystemTest.c`, label them with `disruptive`, and make host capability failures explicit with Unity ignore messages.

<a name="backend-specific-coverage"></a>

### 🎮 Backend-Specific Coverage

DX9 fullscreen color-depth tests check the retained request separately from the
swap chain returned by Direct3D. Hosts may promote an RGB565 request to a
32-bit swap chain. The tests still verify the actual current-mode and buffer
formats, rendered pixels, restoration to windowed mode, and unchanged enumerated
mode descriptions across the switch.

DirectPlay-backed communication tests should stay deterministic until the reconstructed code exposes a real public DirectPlay startup/connection path. Keep no-device DX6 communication checks in `stdCommDX6Test.c`; add a `stdCommDX6SystemTest.c` only when it can initialize DirectPlay through normal engine entry points.

<a name="original-engine-io-test-vectors"></a>

## 📦 Original-Engine I/O Test Vectors

> **Historical procedure.** The capture harness below runs inside the original
> `Indy3D.exe` and relies on the hook and RTI machinery that Jones3D EX removes
> ([ADR-0004](arc42/09-decisions/ADR-0004-drop-binary-compatibility.md)). Existing
> vectors keep their recorded provenance and stay valid. New vectors come from
> the original game (for example savegames and traces) or the byte-exact
> decompilation, with recorded provenance. The rules on provenance, failure
> inputs and review below still apply.

For reconstructed readers, writers, codecs, and other behavior-sensitive I/O,
expected compatibility vectors must come from the selected original `Indy3D.exe`
function when an original implementation exists. Generate the same cases with
OpenJones3D and compare the results. A reconstructed writer/reader round trip
checks internal consistency, but does not by itself establish original-engine
compatibility.
A renamed copy of decompiled code is useful for investigation; record that source
separately from vectors captured by executing the original binary.

Keep original-behavior vectors separate from synthetic malformed-input,
runtime-guard, bug-fix, and QOL expectations. New modules and behavior without a
retail equivalent need independently specified expectations and clear provenance.
For example, BMP masks regenerated by an OpenJones3D backend remain rendering
regression baselines unless they were also independently checked against retail.

<a name="reference-binaries-and-version-policy"></a>

### 🔖 Reference Binaries And Version Policy

The primary references are the original debug build and retail v1.0 of
`Indy3D.exe`. Use these for engine behavior analysis and compatibility vectors.
Identify the exact executable by SHA-256 before invoking it:

| Reference binary | Use | SHA-256 |
| --- | --- | --- |
| Original debug build (`indy3d-dbg-1999-10-28.exe`) | Primary reference | `65f9165283b2e00a78baa93476073b3b302f5d561c240f2e0ae304fd98d028cb` |
| Retail v1.0 (`Indy3D_v1.0.exe`) | Primary reference | `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` |
| Retail v1.2 (`Indy3D_v1.2.exe`) | Reference only for an implemented v1.2 fix | `55fb00c0a2cb30793ff451dc6fd17681c5128aa32419aa711015fd5117d608bf` |

For functions shared by the debug build and v1.0, compare the same cases with
both where practical. Record debug-only assertions, diagnostics, and other
behavior differences explicitly; do not assume debug results are identical to
retail v1.0 results.

Use v1.2 only when the tested function has a fix that is absent from both primary
reference versions and OpenJones3D implements that specific fix. Record the
function, the version difference that establishes the fix, and the corresponding
OpenJones3D implementation. Keep those fixtures identified as v1.2 fix vectors;
preserve the debug/v1.0 baselines and do not use v1.2 as the general oracle for
other behavior.

<a name="capture-harness"></a>

### 🛠️ Capture Harness

Use a small, temporary MSVC Win32 C or C++ capture harness running inside the
original engine process. Native code can reuse the project's ABI declarations,
construct large engine structs, manage their referenced resources, and call the
original functions with the declared calling conventions. Verify original struct
sizes, field offsets, packing, and pointer widths; QOL-expanded layouts must not
be passed to original functions.

Python may define case lists and seeds, launch the original process with the
capture harness, enforce timeouts, collect output files, and calculate hashes.
Keep the engine struct construction and function calls in the native harness,
so nested pointers, callbacks, and ABI layouts have one checked definition.
Loading an EXE with `ctypes`/`LoadLibrary` is not a replacement for running the
initialized engine: Windows does not resolve an EXE's static imports that way.
See [Microsoft's LoadLibrary documentation](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya).

The RTI `<function>_ADDR` and `<function>_TYPE` declarations of the reconstruction (removed in Jones3D EX,
still in the `upstream-compatible-final` tag) described retail v1.0 entry points and prototypes. Resolve and verify the matching addresses
and layouts separately for the debug build or an eligible v1.2 fix; do not reuse
v1.0 addresses for a different binary. `J3D_CALLFUNCFAR` and
`J3D_TRAMPOLINE_CALL` call addresses directly; they do not preserve an unpatched
copy of a function.
Capture from unmodified original entry points and dependencies. Normal
`Jones3D.dll` startup installs replacement hooks, so use a dedicated capture
module and avoid installing those hooks for an original-engine reference run.
`j3dTestSupport` maps globals for standalone tests; it does not load original code.

<a name="generation-procedure"></a>

### ⚙️ Generation Procedure

1. Select the reference binary according to the version policy above and verify
   its SHA-256. Resolve its matching function addresses and ABI declarations in
   the loaded image, then initialize the services needed by the target function.
2. Construct deterministic cases in native code. Initialize every serialized
   field and relevant engine global; provide valid object lifetimes, resource
   references, allocators, file handles, and host callbacks. Cover empty,
   minimal, typical, boundary-size, and each relevant type/flag combination.
   Control seeds, time, locale, floating-point settings, and callback behavior.
3. Invoke the selected original writer or codec and capture the complete emitted
   bytes, return values, output parameters, and callback calls. Record callback
   order, arguments, and outputs using stable object IDs. Generate each case twice
   from freshly initialized state and require identical results before freezing
   it as a reference vector.
4. Run the reconstructed writer or codec with the same semantic inputs. Compare
   the complete file/buffer and its length with the original output, and compare
   all returned and callback-produced outputs. For strings, check full equality,
   including the terminator where it is part of the API's output contract.
5. Feed original-generated files to both readers. Compare all meaningful parsed
   fields, resource references, return values, and callback results. When the
   format permits it, serialize both parsed states again with the original
   writer and require identical output. Also check fields and side effects that
   the writer does not serialize; reserialization alone cannot cover them.
6. Commit the portable input/output fixtures and standalone Unity assertions
   under the owning module's `Tests/tv` directory. Normal CTest runs must consume
   the saved fixtures without launching, injecting into, or depending on
   `Indy3D.exe`. Once equivalent standalone coverage is available, remove
   temporary oracle hooks, decompiled comparison copies, and capture-only call
   seams from permanent engine/test paths.

<a name="failure-inputs"></a>

### 🧯 Failure Inputs

Keep unsafe or unsupported original-engine inputs out of the compatibility corpus.
Exercise malformed data and allocation/I/O failures in separately identified
regression cases. Capture original failure behavior only where the original
function accepts that contract; label intentional fixes and hardened rejection
behavior explicitly.

<a name="provenance-and-review"></a>

### 🔎 Provenance And Review

For each corpus, record the original EXE hash/version, function name and
entry-point address, ABI/layout checks, input cases and seeds, setup and host
callback behavior, relevant environment settings, capture-harness source/version,
regeneration procedure, and SHA-256 of each output. Record whether the two
original-engine runs and reconstructed results matched, plus any preserved quirk
or intentional difference. Keep game assets, the original executable, and
temporary injected oracle code out of the committed fixture set.

Store semantic inputs and stable identifiers rather than raw process addresses
or dumps of pointer-bearing structs. Preserve the original serializer's actual
output bytes. Any format-defined nondeterministic field or permitted comparison
normalization must be documented explicitly; do not silently rewrite a captured
file to make the comparison pass. Review regenerated fixtures as a behavior
change and rerun the permanent tests with vector-update modes disabled.

<a name="running-tests"></a>

## ▶️ Running Tests

<a name="build-options"></a>

### ⚙️ Build Options

| CMake option | Default | Effect |
| --- | --- | --- |
| `JONES3D_BUILD_TESTS` | ON | Builds Unity test executables and enables CTest |
| `JONES3D_BUILD_SYSTEM_TESTS` | ON | Adds system/device tests when normal tests are enabled |
| `JONES3D_BUILD_DISRUPTIVE_SYSTEM_TESTS` | OFF | Adds opt-in disruptive tests; requires system tests |
| `JONES3D_RUNTIME_GUARDS` | OFF | Enables opt-in release safety checks and their rejection paths |

Disabling normal tests also disables system tests. Disabling system tests forces
disruptive tests off. Use separate build trees for backend and guard/QOL settings
so an existing CMake cache does not silently select a different profile.

<a name="native-unit-builds"></a>

### 🐧 Native Unit Builds

The default build on Linux and macOS. Clang is the tested compiler; GCC needs
extra flags for now (`-D__cdecl= -D__stdcall= -D__fastcall=` and, for GCC 14+,
`-Wno-error=incompatible-pointer-types`). On non-Windows hosts CMake selects the
Null platform and the standalone build automatically.

```bash
cmake -S . -B build/native -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build/native
ctest --test-dir build/native -N -L unit
ctest --test-dir build/native --output-on-failure -L unit
```

Sanitizer build (GCC; Clang needs its sanitizer runtime installed):

```bash
cmake -S . -B build/asan -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug -DJONES3D_SANITIZE=ON
cmake --build build/asan
ctest --test-dir build/asan --output-on-failure -L unit
```

Windows build with MinGW (i686), from any host:

```bash
cmake -S . -B build/mingw -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686.cmake -DCMAKE_BUILD_TYPE=Debug -DJONES3D_BUILD_PROGRAMS=OFF
cmake --build build/mingw
```

Under MinGW, `stdConffile` has 2 known failing cases (`stdConffileTest.c:447`,
`:662`) that pass natively.

<a name="msvc-unit-builds"></a>

### 🛠️ MSVC Unit Builds (legacy, until M6)

Run these examples from the repository root in `cmd.exe` or a Visual Studio 2026
Developer Command Prompt. They use MSVC Win32 and the Visual Studio 2026 generator,
which requires CMake 4.2 or newer. Use subdirectories under `build/codex` for
local alternate configurations.

Use both sets of commands when required by
[Validation Scope](#backend-validation-scope). For independent changes,
choose the representative backend and run the affected unit suites in both
Debug and Release.

The following builds compile production code and all available tests. CTest
lists the selected unit suites before running them; confirm that the expected
module entries are present. Zero discovered tests do not validate a change.

DX9, Debug and Release:

```bat
cmake -S . -B build\codex\dx9-win32 -G "Visual Studio 18 2026" -A Win32 ^
  -DJONES3D_USE_DIRECTX9=ON -DJONES3D_BUILD_PROGRAMS=OFF ^
  -DJONES3D_BUILD_TESTS=ON -DJONES3D_BUILD_SYSTEM_TESTS=ON ^
  -DJONES3D_QOL_IMPROVEMENTS=ON -DJONES3D_RUNTIME_GUARDS=OFF
cmake --build build\codex\dx9-win32 --config Debug
ctest --test-dir build\codex\dx9-win32 -C Debug -N -L unit
ctest --test-dir build\codex\dx9-win32 -C Debug --output-on-failure -L unit
cmake --build build\codex\dx9-win32 --config Release
ctest --test-dir build\codex\dx9-win32 -C Release -N -L unit
ctest --test-dir build\codex\dx9-win32 -C Release --output-on-failure -L unit
```

DX6, Debug and Release:

```bat
cmake -S . -B build\codex\dx6-win32 -G "Visual Studio 18 2026" -A Win32 ^
  -DJONES3D_USE_DIRECTX9=OFF -DJONES3D_BUILD_PROGRAMS=OFF ^
  -DJONES3D_BUILD_TESTS=ON -DJONES3D_BUILD_SYSTEM_TESTS=ON ^
  -DJONES3D_QOL_IMPROVEMENTS=ON -DJONES3D_RUNTIME_GUARDS=OFF
cmake --build build\codex\dx6-win32 --config Debug
ctest --test-dir build\codex\dx6-win32 -C Debug -N -L unit
ctest --test-dir build\codex\dx6-win32 -C Debug --output-on-failure -L unit
cmake --build build\codex\dx6-win32 --config Release
ctest --test-dir build\codex\dx6-win32 -C Release -N -L unit
ctest --test-dir build\codex\dx6-win32 -C Release --output-on-failure -L unit
```

<a name="guard-and-legacy-profiles"></a>

### 🛡️ Guard And Legacy Profiles

Runtime-guard rejection tests require `JONES3D_RUNTIME_GUARDS=ON`. Default builds
preserve the original unchecked contracts while retaining `STD_ASSERTREL`.
When changing guard behavior, exercise the guard-enabled branch in a separate
legacy-hardened tree:

```bat
cmake -S . -B build\codex\dx9-legacy-hardened-win32 -G "Visual Studio 18 2026" -A Win32 ^
  -DJONES3D_USE_DIRECTX9=ON -DJONES3D_BUILD_PROGRAMS=OFF ^
  -DJONES3D_BUILD_TESTS=ON -DJONES3D_BUILD_SYSTEM_TESTS=OFF ^
  -DJONES3D_QOL_IMPROVEMENTS=OFF -DJONES3D_RUNTIME_GUARDS=ON
cmake --build build\codex\dx9-legacy-hardened-win32 --config Release
ctest --test-dir build\codex\dx9-legacy-hardened-win32 -C Release -N -L unit
ctest --test-dir build\codex\dx9-legacy-hardened-win32 -C Release --output-on-failure -L unit
```

For shared guard changes, repeat with `build\codex\dx6-legacy-hardened-win32`
and `JONES3D_USE_DIRECTX9=OFF`. When changing QOL or preserved legacy behavior,
also run with `JONES3D_QOL_IMPROVEMENTS=OFF` and `JONES3D_RUNTIME_GUARDS=OFF` in
separate legacy trees for the affected backends.

<a name="system-test-runs"></a>

### 🖥️ System Test Runs

The baseline trees above build normal system tests but do not run them with
`-L unit`. On a suitable interactive desktop, run relevant device coverage
separately:

```bat
ctest --test-dir build\codex\dx9-win32 -C Debug --output-on-failure -R "^std\.System$"
ctest --test-dir build\codex\dx9-win32 -C Release --output-on-failure -R "^std\.System$"
ctest --test-dir build\codex\dx6-win32 -C Debug --output-on-failure -R "^std\.System$"
ctest --test-dir build\codex\dx6-win32 -C Release --output-on-failure -R "^std\.System$"
```

Report Unity ignores and their prerequisite reasons separately from passes and
failures. A passing CTest executable may still contain ignored Unity cases.

<a name="disruptive-test-runs"></a>

### ⚠️ Disruptive Test Runs

Use separate build directories for local disruptive coverage. Run on a desktop
where fullscreen changes and input focus changes can be exercised.

```bat
cmake -S . -B build\codex\dx9-win32-disruptive -G "Visual Studio 18 2026" -A Win32 ^
  -DJONES3D_USE_DIRECTX9=ON -DJONES3D_BUILD_PROGRAMS=OFF ^
  -DJONES3D_BUILD_TESTS=ON -DJONES3D_BUILD_SYSTEM_TESTS=ON ^
  -DJONES3D_BUILD_DISRUPTIVE_SYSTEM_TESTS=ON
cmake --build build\codex\dx9-win32-disruptive --config Debug --target stdDisruptiveSystemTests
ctest --test-dir build\codex\dx9-win32-disruptive -C Debug --output-on-failure -R "^std\.DisruptiveSystem$"
```

```bat
cmake -S . -B build\codex\dx6-win32-disruptive -G "Visual Studio 18 2026" -A Win32 ^
  -DJONES3D_USE_DIRECTX9=OFF -DJONES3D_BUILD_PROGRAMS=OFF ^
  -DJONES3D_BUILD_TESTS=ON -DJONES3D_BUILD_SYSTEM_TESTS=ON ^
  -DJONES3D_BUILD_DISRUPTIVE_SYSTEM_TESTS=ON
cmake --build build\codex\dx6-win32-disruptive --config Debug --target stdDisruptiveSystemTests
ctest --test-dir build\codex\dx6-win32-disruptive -C Debug --output-on-failure -R "^std\.DisruptiveSystem$"
```

Also build and run Release when changing disruptive tests or their production
paths. Keep disruptive failures, ignores, and unexecuted scenarios explicit in
the report; a unit-suite pass does not resolve them.

<a name="ci-profiles"></a>

### 🤖 CI Profiles

> The workflows below are inherited from OpenJones3D and cover the legacy MSVC
> Win32 build. From milestone M5, CI builds and tests on Windows, Linux and
> macOS, builds Babylon on its own, and checks includes and dependency licenses.

Both backends use MSVC Win32 on the `windows-2025-vs2026` runner with
the `Visual Studio 18 2026` CMake generator. The runner includes Visual Studio
2026 and the Windows SDK. DX9 uses the SDK's Direct3D headers, import libraries,
and FXC shader compiler; no separate legacy DirectX SDK installation is needed.
DX6 still downloads the archived DirectX 6.1 SDK because it needs legacy APIs.

The CI test integration uses four profiles for both backends:

| Profile | Configuration | QOL | Runtime guards |
| --- | --- | --- | --- |
| `release-qol` | Release | ON | OFF |
| `legacy-hardened` | Release | OFF | ON |
| `legacy` | Release | OFF | OFF |
| `debug-qol` | Debug | ON | OFF |

Each profile enables `JONES3D_BUILD_TESTS` and disables
`JONES3D_BUILD_SYSTEM_TESTS`. The MSVC Win32 build workflow uploads its build tree
with the exact checked-out source revision and configuration. The reusable test
workflow checks out that revision, restores the build at its original workspace
path, discovers every registered CTest suite, and runs them all with finite
timeouts, without a test-name filter. New suites registered in the CI build are
included automatically. Discovery also requires the existing 13 suites:

- `j3dcore.Macros`, `rdroid.Math`, `std`, `sound`, and `smush`
- `sith`, `sith.DSS`, `sith.Multi`, and `sith.ThingQuetz`
- `sith.AIInstinct`, `sith.AIInstinctFn`, `sith.AIMove`, and `sith.AIUtil`

Test failures, missing required suites, and empty discovery fail the job.
JUnit results and CTest logs are uploaded even when tests fail.

Each build workflow calls its reusable test workflow with `needs: build` after
all four build profiles succeed. The test jobs appear in the same workflow run,
and a test failure fails that run. The test workflows use `workflow_call` and a
local `uses: ./.github/workflows/test-dx*.yml` reference, so they come from the
same commit as the caller and work on `develop` and pull requests without first
merging the workflow files into `main`. Artifacts are downloaded from the current
run. Separate build and test summary jobs require all four profiles in their
stage to succeed. The four README badges track those named check results on
`develop`, independently for each backend and stage. A failed, cancelled, or
skipped stage does not report a passing result. See
[GitHub's reusable workflow documentation](https://docs.github.com/en/actions/how-tos/reuse-automations/reuse-workflows).

These profiles check debug assertions, optimization-sensitive behavior, and
opt-in guard rejection paths. System and disruptive tests remain outside CI
because they require local devices or can disturb the desktop.

<a name="native-ci-columns"></a>

### 🐧 Native CI Columns

`Game (native)` (`.github/workflows/game-native.yml`) builds the game natively and runs its
tests on Linux and macOS in four Debug columns with Ninja and `fail-fast: false`: the Null
column and the Babylon column (`-DJONES3D_PLATFORM_BABYLON=ON`) on `ubuntu-24.04` with Clang,
and the same two on `macos-15-intel` with the system Apple clang. SDL3 is built from source in
CI, so the Linux columns install the same dependency packages as `babylon.yml`; macOS needs
only Ninja.

- Unit suites run as `ctest -N -L unit` followed by `ctest --output-on-failure -L unit`, so the
  discovery list is logged before the results.
- The Babylon boundary checks (`babylon.check.boundary`, `babylon.check.selftest`) carry the
  labels `babylon;check`, not `unit`, because they inspect the source tree instead of running
  engine code. A second labelled call, `ctest -L check`, runs them in every column.
- `python3 Scripts/checkboundary.py --check` (the Windows/DirectX include boundary) runs once per
  push, in the `linux-null` column.
- The address-pinned sith vector suites (`sith.DSS`, `sith.Multi`, `sith.ThingQuetz`,
  `sith.AIInstinct`, `sith.AIInstinctFn`, `sith.AIMove`, `sith.AIUtil`) are not built on Apple
  hosts: they need GNU ld's `-no-pie -Wl,-Ttext-segment=0x20000000` to put the synthetic arena in
  the window that `sithVecTest_AssertArenaPlacement` requires, and ld64 offers no equivalent.
  Both Linux columns still run them.

<a name="intermittent-failures"></a>

### 🔎 Intermittent Failures

Preserve the initial failure, logs, input seed/fixture identity, build settings,
and relevant host/device details. Use bounded reruns to investigate whether the
cause is test state, timing, a host prerequisite, or production behavior. Report
the original failure and subsequent outcomes; a later pass does not resolve an
unexplained failure.

If a fix is deferred, record the outstanding problem and follow-up. A Unity
ignore is appropriate for an unavailable prerequisite, as defined under
[System And Device Tests](#system-and-device-tests). Keep behavior failures
visible when the prerequisite was available. Investigate unexpected outputs
before changing assertions, tolerances, or fixture expectations.
See [Google's experience with flaky tests](https://testing.googleblog.com/2016/05/flaky-tests-at-google-and-how-we.html).

<a name="timeouts-and-shared-devices"></a>

### ⏱️ Timeouts And Shared Devices

Set a finite CTest `TIMEOUT` for process and device tests when adding or changing
their registration. Choose a documented limit from expected runtime with margin
for slower hosts. Verify the configured timeout and failure-path cleanup;
CTest can terminate a timed-out process before its teardown runs.

Within one CTest run, use a shared `RESOURCE_LOCK` for tests that compete for the
desktop, display mode, input focus, or the same device. Use `RUN_SERIAL` when a
test must run without any other test in that invocation. These properties
coordinate one CTest scheduler; serialize conflicting DX6/DX9 test runs across
build directories and separate invocations as well.

A capability skip still needs a clear reason, and a timeout is a failure to
investigate. Verify the actual registration instead of assuming that these
properties are already configured for an existing suite.
See CMake's [TIMEOUT](https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html),
[RESOURCE_LOCK](https://cmake.org/cmake/help/latest/prop_test/RESOURCE_LOCK.html),
and [RUN_SERIAL](https://cmake.org/cmake/help/latest/prop_test/RUN_SERIAL.html) documentation.

<a name="visual-system-tests"></a>

## 🖼️ Visual System Tests

<a name="presentation-and-input"></a>

### 🎥 Presentation And Input

Win95 visual system tests show their render window by default so local runs can
be inspected while the BMP masks are compared. Set
`JONES3D_SYSTEM_TEST_HIDE_WINDOW=1` to restore hidden-window behavior, and set
`JONES3D_SYSTEM_TEST_FRAME_DELAY_MS=<milliseconds>` to adjust or disable the
minimum visual frame interval. A value of `0` disables pacing. When the exact
interval is not set, `JONES3D_SYSTEM_TEST_VISUAL_SPEED=<factor>` scales the
default 100 ms interval; `2` is twice as fast, `0.5` is half speed, and `0`
disables pacing. Window presentation, render, readback, and comparison work count
toward the interval, so 320x240 and 640x480 sequences are presented at the same
rate when the host can keep up.

In windowed mode, the visible preview paints a read-only copy of the same
backbuffer used by the BMP mask assertions. Exclusive fullscreen presentation
is owned by the graphics backend and does not use that GDI preview.

Injected keyboard and mouse system tests require their visible test window to
hold foreground focus before sending input. If the host denies focus, or hidden
window mode is enabled, these cases report that prerequisite as a skip. They
attempt to restore the previous foreground window during teardown. Run them on
an unlocked interactive desktop that allows the test window to become active.

The DX6 depth-buffer mode-switch test uses the complete pixel format and memory
placement selected by `std3D_Open()`, then checks the attached buffer after each
recreation; it does not assume that a bare 16-bit depth format is supported.

<a name="bmp-fixture-coverage"></a>

### 🖼️ BMP Fixture Coverage

Win95 `std3D` system-test BMP vectors live under
`Libs/std/Win95/Tests/tv/std3D/<backend>`. Most are backend-specific 320x240
24-bit BMP masks; magenta pixels are ignored by the comparison helper, while all
other pixels are expected rendered output. The DX6 explicit-mipmap cube, DX9
automatic-mipmap cube, and combined DX9 mipmap/MSAA/anisotropic cube sequences
use 640x480 masks so texture LOD and edge filtering remain useful to inspect
without quadrupling the size of every visual vector. Both the explicit-chain
and automatic-mipmap variants of the combined DX9 sequence have their own
masks.

`std3DTestVectorTest.c` audits the golden BMP corpus without opening a graphics
device. It pins the expected DX6, DX9, and OpenGL file counts; verifies every
expected filename, Windows 3.x BMP header, dimensions, 24-bit BI_RGB layout,
and exact pixel payload; rejects blank or single-color images; and checks that
adjacent animation frames differ while each sequence retains substantial frame
diversity. This deterministic audit belongs to the `std` unit suite and can run in CI
without graphics services. When adding or intentionally regenerating vectors,
update the audit inventory in the same change.

The feature cube vectors use a 32-frame sequence: rotate near the camera, move
backward while rotating, rotate at the far distance, then move forward while
rotating. The shared geometry-mode baseline uses nine 10-degree rotation frames
for vertex, wireframe, white solid, textured, textured with per-vertex color,
and solid with interpolated RGB vertex-color cases.

The mipmap cube vectors use explicit UV-grid mip chains and project the full
texture once onto each visible cube face. Every lower level is generated from
its parent with a deterministic 2x2 box filter. DX9 uses the complete ten-level
chain from 512x512 through 1x1; DX6 uses the complete nine-level chain from
256x256 through 1x1 for devices that follow the legacy texture-size limit. The
32-frame DX6 explicit-chain sequence uses 640x480 output, while the compact DX9
explicit-chain baseline remains 320x240.

DX9 also has a separate 32-frame 640x480 automatic-mipmap sequence. It enables
`graphics.mipmapAutoGen`, uploads only the 512x512 top level, verifies that the
cached texture uses `D3DUSAGE_AUTOGENMIPMAP`, and renders near/far frames that
exercise the driver-generated levels. Direct3D keeps those generated sublevels
driver-managed, so the test verifies their behavior through sampled output
rather than attempting to read the hidden levels directly.

DX9-only system vectors cover anisotropic filtering and MSAA. The MSAA coverage
includes a triangle probe plus 32-frame cube masks for wireframe, white solid,
textured, textured with vertex color, and solid with interpolated RGB vertex
color. Tests request 16x MSAA and let the DX9 display layer select the highest
supported standard level by falling back through 8x, 4x, and 2x. The tests are
ignored when the host cannot create any of those windowed MSAA modes. A
separate 32-frame 640x480 sequence combines the explicit mip chain, trilinear
mip filtering when supported, anisotropic minification, and the selected MSAA
level. A second sequence exercises the same MSAA and anisotropic path with the
DX9 driver-generated mip chain. DX6 has no MSAA or anisotropic vectors.

<a name="regenerating-backend-baselines"></a>

### ♻️ Regenerating Backend Baselines

Set `JONES3D_SYSTEM_TEST_UPDATE_VECTORS=1` only when intentionally regenerating
BMP expectations from the current backend. The test writes the displayed back
buffer before loading and comparing the corresponding vector, so rerun without
the variable afterward to validate the generated files normally.

<a name="validation-report"></a>

## 📋 Validation Report

Include the following in the change description or review reply:

- Functions or macros covered, important cases, and any known coverage gaps.
- Regression proof for fixes, or the reason it was unavailable, plus numerical
  tolerance rationale and coverage findings when collected.
- Build trees (native, sanitizer, Windows), compiler, Debug/Release
  configuration, guard/QOL settings and scope rationale, with the commands
  actually run.
- For simulation changes: the trace runs (savegame, input script, frame count,
  profile, reference build) and whether the state files were identical.
- Build results and discovered CTest suites. Report CTest executable totals and
  Unity case totals where available; distinguish passes, failures, ignores, and
  tests not run.
- Fixture provenance for compatibility vectors,
  plus reproducibility results and documented intentional differences.
- Host limitations and unresolved device/disruptive failures. Include initial
  intermittent failures, diagnostic reruns, timeouts, and remaining follow-up.
  Do not describe skipped or unavailable cases as validated behavior.

<a name="planned-test-integrations"></a>

## 🚧 Planned Test Integrations

This section describes pending integrations. Verify the current CMake targets
and workflow files before using these commands or claiming CI coverage.

<a name="deferred-addresssanitizer-integration"></a>

### 🛡️ AddressSanitizer In CI

Native sanitizer builds work locally (see [Native Unit Builds](#native-unit-builds)).
Making them a required CI check is planned with the three-platform CI in M5.

<a name="additional-module-commands"></a>

### 🧩 Additional Module Commands

The `rdroidTests`, `soundTests`, `smushTests`, `sithTests`, and `sithDSSTests` targets are available and
included in a full build. Use the matching target and CTest entry for a focused rebuild and run:

```bat
cmake --build build\codex\dx9-win32 --config Debug --target rdroidTests
ctest --test-dir build\codex\dx9-win32 -C Debug --output-on-failure -R "^rdroid\.Math$"
cmake --build build\codex\dx9-win32 --config Debug --target soundTests
ctest --test-dir build\codex\dx9-win32 -C Debug --output-on-failure -R "^sound$"
cmake --build build\codex\dx9-win32 --config Debug --target smushTests
ctest --test-dir build\codex\dx9-win32 -C Debug --output-on-failure -R "^smush$"
cmake --build build\codex\dx9-win32 --config Debug --target sithTests
ctest --test-dir build\codex\dx9-win32 -C Debug --output-on-failure -R "^sith$"
cmake --build build\codex\dx9-win32 --config Debug --target sithDSSTests
ctest --test-dir build\codex\dx9-win32 -C Debug --output-on-failure -R "^sith\.DSS$"
```

These focused commands supplement the validation selected under
[Validation Scope](#backend-validation-scope).

The `rdroid.Math` suite compares vector normalization, reciprocal scaling,
axis-angle construction, and angle extraction against source-level arithmetic
oracles transcribed from `Jones3DRE/v1.0-dbg/indy3d-dbg.exe.c`. It checks exact
float representations (and NaN classification), including rounding-sensitive
inputs, zero/underflow/overflow behavior, singularities, range clamps, and the
existing near-axis guards. These oracles validate debug-source reconstruction,
not execution of the original x87 binary or bitwise parity with optimized retail
builds. They do not require game assets or invoke the original executable.

The `stdMath` group inside `stdTests` checks all sine/tangent table entries
against standalone binary fixtures, third-quadrant tangent interpolation,
finite-angle normalization across exponent ranges, inverse-trig polynomial
rounding, negative-power subnormals and signed zero, and damping with extreme
finite endpoints. Its inverse-trig cases pin binary32 C arithmetic, not x87
runtime parity. Reference provenance belongs in the
[math fixture documentation](../Libs/std/General/Tests/tv/stdMath/README.md),
not in test names or binary-address comments. Normal runs do not require the
original executable.

The fixture descriptions below cover the available `soundTests`, `smushTests`,
`sithTests`, and `sithDSSTests` targets.

<a name="audiolib-fixtures"></a>

### 🎙️ AudioLib Fixtures

The `AudioLib` tests cover packed mouth-position lookup boundaries, invalid
headers and times, the preserved final-timestamp and zero-entry lookup bugs, PCM format
rejection, exact silence and deterministic-waveform output blocks, independent
X/Y quantization, cross-sample PCM decoding, multiple analysis/sample rates,
terminal timestamps, checked input/output capacities, arithmetic limits, and
generated-block round trips. Generated timelines are queried at every
millisecond through and beyond their terminal entry. During
reconstruction validation, a temporary renamed copy of the decompiled original
was compared against the production implementation for 49,158 lookups and 72
generation combinations in both Debug and Release. The oracle was removed after
the results matched byte-for-byte; the permanent suite keeps independent golden
blocks and behavior assertions.

The ADPCM and WVSM codec functions and `AudioLib_ParseWaveFileHeader` are
covered by original-engine vectors captured from retail v1.0 `Indy3D.exe` and
stored in `Libs/sound/Tests/tv/AudioLib` (provenance, case lists, preserved
quirks and the inputs kept out of the corpus are in that directory's
`README.md`). The `TestOriginal*` cases compare complete outputs, return values
and compressor state, including chained block calls.

<a name="smush-fixtures"></a>

### 🎞️ Smush Fixtures

The `smush` suite (`smushTests`, CTest entry `smush`, labels `unit;smush;<backend>`) links the smush library and runs
every area against the real functions of the other areas; the only mocks are the LEC services (share table, allocator,
timer), an in-memory file system, the executable's zlib `gz*` functions (through the `Zfilio_TestSetGz` seam) and a pure C
`IDirectSound`. Each area has its own test file and CMake list under `Libs/smush/Tests`: `SmushCoreTest.c` (SmushPlay,
LecShare, Timer, Memory, List, Bitmap, Blit), `SmushSoundTest.c` (Engine, Mixer, Dsnd), `SmushCodecTest.c` (D_bloc16,
D_wave, Decode), `SmushStreamTest.c` (Cdstream, Framestream, Stdfile, Stdio, Zfilio, Readauto, Frame, Pcplay) and
`SmushAsmTest.c`.

`SmushAsmTest.c` covers the six assembly files that were translated to C (`Bloc16Asm`, `DecodeAsm`, `MixerAsm`,
`BlitAsm`, `MemCopy`, `MemFill`) with original-engine vectors: inputs and complete results of every procedure captured by
executing the retail v1.0 `Indy3D.exe`, stored in `Libs/smush/Tests/tv` (format, reference executable hash, procedure
addresses and case counts in `Libs/smush/Tests/tv/README.md`). The test requires every byte of every buffer to match and
never launches the executable. The vectors include the assembly's quirks (zero counts that run the loop once, 16-bit
pitch multiplies, index carry-over in the three-byte table lookups), which carry `TODO: [BUG]` notes in the code.

Not covered by unit tests: the intro movie playback itself (needs the game data and a window; checked by running the
game), and the exact output of `mixer_Setup`'s sine and clip tables, which are computed with the DLL's CRT instead of the
executable's.

<a name="sound-fixtures"></a>

### 🔊 Sound Fixtures

The `Sound` fixture exercises `Sound_GenerateLipSync()` without opening an audio
device. Test-only controls replace channel position and decompression while the
production cache, allocation, generation, and mouth-position paths remain in
use. Coverage includes uncompressed and compressed input, lazy cache reuse,
blocks larger than the original 8192-byte temporary buffer, allocation cleanup,
playback window boundaries, time offsets, generated-data lookup, and every
seven-bit input to `SOUND_LIPSYNC_GETMOUTHLEVEL()`. Malformed
sound-bank metadata belongs to the `Sound_Load()` and `Sound_ImportBank()` test
fixtures because those functions own admission of data into the bank.

<a name="siththing-fixtures"></a>

### 📦 sithThing Fixtures

The `sithThing` fixture validates the reconstructed binary codec with standalone
retail-generated golden vectors covering every thing type, movement and control
block, resource name, sector reference, bounded name, inherited weapon data,
allocation failure, short I/O, and malformed block counts. Synthetic static-world
text sections additionally exercise text placement parsing, skip behavior,
performance filtering, wrapper serialization, binary restoration, runtime ID
assignment, and wrapper failure propagation without loading game assets.
Recalculated Euler angles use an absolute tolerance of `0.00001` degrees;
all other serialized fields remain byte-exact against the unchanged vectors.
The math suites separately validate the numerical evaluation and constants.

<a name="sithtemplate-fixtures"></a>

### 🧬 sithTemplate Fixtures

The `sithTemplate` fixture starts the thing parser and template cache together.
Synthetic text vectors cover blank and inherited templates, missing bases,
discarded type-less entries, duplicate names, capacity exhaustion, missing end
markers, static indices, and allocation failure. Binary tests then verify exact
round trips, ordered cache reconstruction, empty lists, short I/O, truncation,
and cleanup after partial deserialization.

<a name="sithvoice-fixtures"></a>

### 🗣️ sithVoice Fixtures

The `sithVoice` fixture exercises the gameplay mapping without opening an audio
device or loading actor assets. Test-only seams provide deterministic lip-sync
coordinates, game time, and swap operations while production builds retain the
original calls. Coverage includes every 4 x 4 lookup-table cell, the shipped
Y-only table layout, repeated-head variation, missing and existing M-sound
heads, targeted swap cleanup, dying speakers, and the original global timing
and cross-speaker state behavior.

<a name="sithdss-fixtures"></a>

### 📨 sithDSS Fixtures

The `sithDSS` and `sithDSSThing` fixtures compare the save/network message writers and readers of `sithDSS.c` and `sithDSSThing.c` with
original-engine vectors captured from retail v1.0 `Indy3D.exe` and stored in
`Libs/sith/Dss/Tests/tv/sithDSS` (provenance, regeneration, case lists and the
documented differences are in that directory's `README.md`). The writers and readers
run on a synthetic world (sectors, surfaces, things, AI blocks, puppets, cameras and
the gameplay globals) that is built by the same source code in the capture harness and
in the tests. The engine functions the DSS code calls with side effects are replaced by
shared stubs that record their arguments, so a case result holds the return value, the
complete message handed to `sithMessage_SendMessage`, the engine calls made and every
byte of the state that changed. Each case is run with two different fills of the unused
parts of the world and of the message buffers to reject accesses outside the data the
function is given. The engine globals of the original executable are redirected to
static memory in this suite, and the suite links its image at a fixed high base so the
normalized addresses of the synthetic state cannot collide with generated values.
The one expected difference (an intentional playback speed fix in
`sithDSS_ProcessPuppetStatus`) makes the matching test report as ignored.
