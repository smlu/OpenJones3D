## sith World-State Vector Harness

Original-engine test vectors for sith functions that need engine state: things, sectors, AI control blocks, AI
classes, the instinct registry, puppets, file statics kept between frames, and the engine calls those functions make.
It generalizes the approach of the DSS vectors (`Libs/sith/Dss/Tests`) and follows the original-engine I/O vector
rules of [Docs/Testing.md](../../../../Docs/Testing.md).

Suites:

| Suite | Executable / CTest entry | Cases | Fixture and provenance |
| --- | --- | --- | --- |
| sithThing Quetz (`sithThing_CreateQuetzUserBlock`, `sithThing_UpdateQuetzUserBlock`) | `sithThingQuetzVecTests` / `sith.ThingQuetz` | `World/Tests/sithThingVecQuetz.h` | [World/Tests/tv/sithThingQuetz](../../World/Tests/tv/sithThingQuetz/README.md) |
| sithAIInstinct (`sithAIInstinct_InitInstincts`, `sithAI_RegisterInstinct`, `sithAI_FindInstinct`) | `sithAIInstinctVecTests` / `sith.AIInstinct` | `AI/Tests/sithAIInstinctVec.h` | [AI/Tests/tv/sithAIInstinct](../../AI/Tests/tv/sithAIInstinct/README.md) |
| sithAIInstinct functions (`sithAIInstinct_Listen` ... `sithAIInstinct_SnakePlaySlitherSound`, all but `InitInstincts`) | `sithAIInstinctFnVecTests` / `sith.AIInstinctFn` | `AI/Tests/sithAIInstinctFnVec.h`, `AI/Tests/sithAIInstinctFnVecCases.inc` | [AI/Tests/tv/sithAIInstinctFn](../../AI/Tests/tv/sithAIInstinctFn/README.md) |
| sithAIMove (all functions of `sithAIMove.c`: update, turn, move, look/move positions, special moves, head tracking, Quetzalcoatl strike) | `sithAIMoveVecTests` / `sith.AIMove` | `AI/Tests/sithAIMoveVec.h`, `AI/Tests/sithAIMoveVecCases.h` | [AI/Tests/tv/sithAIMove](../../AI/Tests/tv/sithAIMove/README.md) |

### How It Works

The case header of a suite and the headers of this directory are compiled twice from the same source:

- by a temporary capture harness (`SITHVEC_HARNESS` defined): a DLL injected into the suspended retail v1.0
  `Indy3D.exe` that calls the original functions at their v1.0 addresses (`SITHVEC_FN(fn)` is
  `((fn##_TYPE)fn##_ADDR)`) and writes the fixture, and
- by the suite's test executable (`SITHVEC_FN(fn)` is `fn`), which runs the reconstructed functions and compares
  their records with the fixture. Normal test runs never start `Indy3D.exe`.

| File | Contents |
| --- | --- |
| `sithVec.h` | Record format, byte buffer, deterministic data (`sithVec_Rnd*`, seed = hash of the case name), host service counters, address normalization, watched memory, events, checkpoints, case and suite descriptors |
| `sithVecWorld.h` | The arena (`SithVecArena`: world, sectors, things, templates, AI classes and control blocks, AI frames, puppets, instinct registry, allocation pool, suite scratch), its builder, the lookup stubs and their verification, the shared stubs (`stdMemory_Malloc`/`Free`, `rand`, `sithCollision_FindSectorInRadius`, `sithCollision_MoveThing`, math helpers) and the case runner |
| `sithVecTest.h` | Test side: fixture loading, record comparison, failure description, `SITHVEC_VERBOSE=1` record dump, known differences |
| `sithVecTestAddrs.h` | Force-included into the test sources: redirect helpers for far engine globals, `rand` replaced by the stub |
| `sithVecTestRunner.c` | Shared `main` |
| `CMakeLists.txt` | `sith_add_vec_test()` and the suite registrations |

A case function sets up its inputs, calls `sithVec_Begin` (reference image of the watched memory), calls the tested
function and, when it calls it several times (frames), `sithVec_Checkpoint` after every call. A record holds the
return value, the counts of asserts, error and warning prints and allocations, and per checkpoint the engine calls
recorded by the stubs and every watched byte that changed. Addresses of the arena are normalized to offsets and
registered functions (`SITHVEC_ADDFUNC`) to their number, so records do not depend on where the state or the code
lives (the test images are linked at a fixed base far from every generated value). Every case is run twice, with the
unused tail of every arena array filled with `0xA5` and with `0x5A`; both runs must agree (a function that reads or
writes outside the state it was given fails the case), in the harness and in the tests.

Seams: engine functions the tested function calls are either run unchanged on both sides (pure code without engine
globals, functions of the tested source file itself) or replaced by a stub:

- `SITHVEC_SEAM(fn)` in the suite's seams file: the harness patches the original `fn` to jump to `sithVec_fn`, the
  CMake function renames `fn` to `sithVec_fn` in the tested sources. The stub records the call (`sithVec_Ev`) and
  returns what the case configured.
- `SITHVEC_LOOKUP(fn)`: a pure lookup that reads engine globals the tests cannot reach. The harness leaves the
  original running on the synthetic world and checks the stub against it for the pointers and indices the cases can
  produce (`sithVec_VerifyLookups`, "0 mismatches" in the capture log); the tests use the stub.
- A function defined in the tested source itself cannot be renamed (the rename would also rename its definition): it
  runs unchanged, and its own callees are seamed instead.
- `rand` is always replaced (the CRT's sequence, seeded per case): the harness patches the v1.0 `rand` at `0x004F7010`,
  the tests use a macro because the Debug CRT declares `rand` as a DLL import.

The tested source is compiled into the test executable through a wrapper (`sithThingVecTested.c`,
`sithAIVecTested.c`) that `#include`s it and adds accessors for its file statics (state kept between calls, such as
`sithThing_bQuetzTurningSharp`). The harness reaches the same variables at their v1.0 addresses. Engine globals used
through `J3D_DECL_FAR_VAR` are redirected to static storage in the tests (suite address header).

Floating point: the harness sets the x87 precision to 24 bits, so the original rounds every operation to single
precision like the reconstruction's SSE code (with 53 bits the original keeps unrounded register values and results
differ in the last bits). The rdroid/stdMath helpers compute on their own tables and precision; a suite that tests
engine logic seams them to the shared math stubs, which both sides compute identically.

### Adding A Suite Or Cases (HOWTO)

1. Pick the functions and read the reconstruction and the decompilation of each, listing every callee and every
   global it reads or writes.
2. Create, next to the module (for example `Libs/sith/AI/Tests`):
   - `<suite>Vec.h`: `#include "sithVecWorld.h"`, the parameter structs, `pfBuild` (put the case state into the arena
     with `sithVec_NewThing`, the arena arrays and `sithVec_Rnd*`), `pfWatch` (`SITHVEC_WATCH` for mirrors and
     engine globals that change, `sithVec_Ignore` for bytes that are undefined on both sides), `pfSync` (copy file
     statics/globals into watched mirrors), the case functions, the case table (`SithVecCase`: name, function, var,
     parameters), the `SithVecSuite` and `#define SITHVEC_SUITE <suite object>`. Name cases `<Function>_<state>`
     and edge cases `<Function>_edge_<state>`; comment what each group covers and why.
   - `<suite>VecSeams.def`: one `SITHVEC_SEAM(fn)` / `SITHVEC_LOOKUP(fn)` per replaced callee. Add new stubs to
     `sithVecWorld.h` when other suites can use them (`SITHVEC_STUB` prototype exactly like the engine's, record
     with `sithVec_Ev`, behavior selected by `sithVec_stub*` variables reset in `sithVec_ResetStubs`), otherwise to
     the suite header; new lookups also get a check in `sithVec_VerifyLookups`.
   - `<suite>VecAddrs.h`: `SITHVEC_REDIRECT_DECL` and `#define <global>_ADDR SITHVEC_REDIRECT_ADDR(<global>)` for
     each far global the tested code touches; define the storage in the test source with `SITHVEC_REDIRECT_DEFINE`.
   - `<module>VecTested.c`: `#include "../<module>.c"` plus accessors for the file statics the cases need (unless an
     existing wrapper of the same executable already compiles that source).
   - `<suite>VecTest.c`: a Unity group per suite with `TestFixtureCoversAllCases` and one `TestOriginal<Function>`
     per function prefix (`sithVecTest_RunCases`), and `SithVecTest_RunAllTests`.
   - Register it in `CMakeLists.txt` with `sith_add_vec_test()` (own executable and CTest entry `sith.<Area>`).
3. Capture: the harness source is kept outside the repository (see the fixture READMEs for its hash). It includes the
   suite header and seams file named on its command line, patches the seams and `rand`, verifies the lookups, runs
   every case with both fills and writes `out\<suite>.tv` next to the DLL:
   `cl /MT /O1 /LD capture.c /DSUITE_HEADER="\"<suite>Vec.h\"" /DSUITE_SEAMS="\"<suite>VecSeams.def\""
   /I<suite dir> /ILibs/sith/Tests/Vec /I<generated j3dcore dir> /ILibs /I<DirectX61c include> /Fe:Jones3D.dll`,
   then `Jones3D.exe Indy3D.exe` (this repository's launcher) with a copy of retail v1.0 `Indy3D.exe` (SHA-256
   `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53`) in the same directory. Check the log: no
   `UNSTABLE` or `EXCEPTION` lines, 0 lookup mismatches. Run it twice and require identical files.
4. Inputs that crash the original, overflow fixed arrays, or read uninitialized memory in the original (stack values
   differ between runs) stay out of the corpus; document them under "Inputs kept out of the corpus".
5. Copy the fixture to `tv/<suite>/` and run the tests in Debug and Release. A case that differs because the
   reconstruction differs from the original is not fixed in the test change: list it in the test's known
   differences (`sithVecTest_ExpectKnownDifferences`, reported as ignored) and in the fixture README, and report it.
6. Write `tv/<suite>/README.md`: reference binary, entry points, seams and stubs, environment, cases, exclusions, known
   differences, regeneration, SHA-256 of the fixture.
