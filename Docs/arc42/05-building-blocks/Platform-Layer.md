# Platform Layer

> **Superseded in part.** This page describes the platform layer built in milestones M3 and M4, where the recovered
> function names were the interface. From M5 on, Babylon defines its own interfaces and these functions become
> adapters ([ADR-0007](../09-decisions/ADR-0007-adapter-between-game-and-babylon.md)). Struct layouts no longer
> need to stay fixed ([ADR-0004](../09-decisions/ADR-0004-drop-binary-compatibility.md)).

How OpenJones3D separates engine code from Windows and DirectX, so the engine can run on other platforms. Milestone names such as M3.6 refer to stages of the native port.

## Summary

Keep the engine's recovered function names as the interface, select one backend per build, and move platform code below those functions. Replace platform types in shared headers with engine types of the same size, so struct layouts don't change. Dialogs move behind their existing 15 external functions instead of being redesigned.

## SOLID, and what it changes

Structure may change to follow SOLID. Behavior can't change during the layer work, because the regression runs compare against the original.

| Principle | Rule for new and moved code |
|---|---|
| Single responsibility | A file has one job. Win32 UI code and dialog logic get separated. Backend files that mix unrelated jobs may be split |
| Open/closed | A new backend adds a column and changes no engine code |
| Liskov substitution | Each header states its contract, and every backend, including Null, passes the same tests through the interface |
| Interface segregation | Wide headers split by role, so callers include only what they use. For example `std3D.h` into device setup, textures and drawing, and `sound/Driver.h` into playback and 3D positioning. Each split gets decided from real callers when its sub-milestone starts |
| Dependency inversion | Engine code depends on interfaces only. Reaching upward always goes through a registered callback |

**Limits:**
- **Recovered function names stay** where they still fit, because binary comparison uses them. Splitting a header doesn't rename its functions.
- **Link-time selection stays.** Runtime dispatch would add indirection with no need for it.
- **Struct layouts stay** while the layer work needs comparison with the original, and M4 revisits them for 64-bit.
- **No abstractions without a second user.** SOLID isn't a reason to add a table or wrapper that only one backend needs.

## What the separation looks like

The separation is a horizontal cut through the existing modules, not a new module. The downward dependency order in `AGENTS.md` stays. What changes is that each platform-facing module splits into a header above the cut and implementation below it.

```text
  Jones3D   game flow, HUD, inventory, config logic
  sith      gameplay, COG, physics, AI, savegames
  smush     cutscene decoding
  sound     Sound.c, AudioLib.c
  rdroid    rendering pipeline, rdCache
  std       General/: files, config, memory, utilities
  j3dcore   macros, sized types
======================= engine types only above this line =======================
  Interfaces (headers):
  std3D.h  stdDisplay.h  stdControl.h  stdComm.h  stdWin95.h  stdGob.h
  sound/Driver.h  wkernel.h  wuRegistry.h
======================= the build links exactly one column ======================
  Win32 + DX9      | Win32 + DX6 (paused) | Null (M4)      | Native backend (M5, M6)
  Win95/DX9/*.c    | Win95/DX6/*.c        | std/Null/*.c   | added by later milestones
  DriverDX9.c      | DriverDX6.c          | null driver    |
  Jones3D/Platform/Win32/, wkernel.c,  |                |
  wuRegistry.c (shared by both columns)|                |
---------------------------------------------------------------------------------
  Windows, Direct3D, DirectInput, DirectSound ...
```

Three rules define the cut:

1. **Code above the line can't name a platform type or include a platform header.** It compiles with no Windows or DirectX headers on the include path.
2. **Code above the line contains no backend `#if`.** `J3D_DIRECTX*`, `_WIN32` and `_MSC_VER` go away there, 47 lines today. Compiler and CRT differences move to `j3dcore`.
3. **Backends follow the same downward order as their module.** A `std` backend can't include anything above `std`, and a `sound` backend can use `rdroid` math, as `DriverDX9.c` does today. Files in one column can call each other, as `std3DX9.c` calls `stdShaderDX9`, but a column never uses another column, and no backend reaches `sith` or `Jones3D`. All current backend includes already follow this.

### Three traces through the cut

**A call going down: reading a key.**

```text
sithControl.c   stdControl_ReadKey(STDKEY_LEFT, &bPressed)      engine number, engine call
---------------------------------------------------------------------------------
stdControlDX9.c reads DirectInput state, indexed by the same number
stdControlNull  reports nothing pressed
```

**An event coming up: the window is activated.** Lower code can't call `Jones3D` directly, so `Jones3D` registers a function at startup.

```text
Windows         WM_ACTIVATE
Win32 wndproc   unpacks wParam into engine values                Jones3D/Platform/Win32/
---------------------------------------------------------------------------------
JonesMain.c     JonesMain_OnActivate(bActive)                    no HWND, no WM_*
```

**Data crossing: a texture.** The engine holds and sorts textures without knowing what they are.

```text
rdCache.c       tSysTexture* pCachedTexture = pTex->pCachedTexture     can store and pass it
                std3D_DrawRenderList(pCachedTexture, flags, aVerts, ...)  can't look inside it
---------------------------------------------------------------------------------
std3DX9.c       sees the real IDirect3DTexture9* and draws
```

## Patterns

Eight patterns do the separating. Each row says which problem it solves, so a new leak can be matched to its fix.

| # | Pattern | Use it when | In this engine | Status |
|---|---|---|---|---:|
| 1 | Ports and adapters | Always: this is the overall shape | The header is the port, the backend `.c` the adapter | Partly there |
| 2 | Link-time selection | Choosing the implementation of a port | `Libs/std/CMakeLists.txt` drops `*DX6.c` or `*DX9.c` from the build | Exists |
| 3 | Opaque handle | The engine only stores and passes something | `tSysTexture*`, `tSysWindow` | New for most types |
| 4 | Backend-sized storage | The engine must embed or allocate something it can't read | `tSystemTexture` arrays, `tVSurface.desc` | New |
| 5 | Neutral types, translated at the edge | The engine must read or write the data | `StdVertexTL`, `STDKEY_*` | New |
| 6 | Capability query | The engine asks what a backend can do | `std3D_IsMSAASupported` | Exists, needs extending |
| 7 | Registered callback | Lower code must reach upward | `wkernel_SetProcessProc`, `wkernel_SetWindowProc` | Exists, needs cleaning |
| 8 | Null object | A platform has no such feature, or none yet | Null backend, null dialogs | New |

### 1. Ports and adapters

The port is the header, written in engine terms. The adapter is a backend that translates those calls to one platform API. The engine depends only on ports, and an interface counts as proven once two adapters implement it with no `#ifdef` in its callers.

### 2. Link-time selection

Each port has one implementation in a given build, chosen by CMake, with no runtime dispatch.

```cmake
if(J3D_DIRECTX9)
    list(FILTER SOURCES EXCLUDE REGEX ".*DX6\\.c$")
```

- **Why not function-pointer tables:** a table would hide the recovered symbol names that hooks and binary comparison use, and would add an indirect call to hot paths. Nothing needs to swap backends at runtime.
- **Existing exception:** `tHostServices` is a real function-pointer table for logging, memory and file services. It is recovered engine behavior, so it stays, and it doesn't get new platform functions.

### 3. Opaque handle

The shared header declares a struct and never defines it. The engine can hold the pointer, but dereferencing it is a compile error. Only the backend knows the real type.

```c
// Shared header
typedef struct sSysTexture tSysTexture;

// Libs/std/Win95/DX9/stdSysTypesDX9.h, first include in DX9 code
#define sSysTexture IDirect3DTexture9
```

In DX9 code the tag names the real type, so that code needs no casts and doesn't change. A pointer has the same size as before, so struct layouts don't move. This covers `tSysSurface`, `tSysTexture`, `tSysDevice3D`, `tSysDisplayDevice`, `tSysSoundBuffer`, `tSysWindow` and `tSysHandle`.

### 4. Backend-sized storage

An undefined struct can't be embedded by value, and four are embedded today:

| Field | Type | Where |
|---|---|---|
| `tVSurface.desc` | `tSysSurfaceDesc` | `Libs/std/types.h:380` |
| `StdDisplayDevice.caps` | `tSysDisplayDeviceCaps` | `Libs/std/types.h:434` |
| `Device3D.d3dDesc` | `tSysDevice3DDesc` | `Libs/std/types.h:466` |
| `StdTextureFormat.ddPixelFmt`, `tSystemTexture.format` | `tSysPixelFormat` | `Libs/std/types.h:568`, `:405` |

`tSystemTexture` also has different fields per backend, behind an `#ifdef` at `Libs/std/types.h:398`. The engine has to know its size, because `rdMaterial.c:153` and `sithMaterial.c:536` allocate arrays of it, and `rdCache.c:315` reads its `pCachedTexture` field.

The fix is a byte array of the right size above the line, and the real type below it. The size comes from a header in the selected backend's directory, which defines numbers only.

```c
// Libs/std/Win95/DX9/stdBackendTypes.h, found through the backend's include path
#define STD_SYS_SURFACEDESC_SIZE 32
#define STD_SYS_ALIGN            4

// Libs/std/types.h
#if !defined(STD_SYS_BACKEND_TYPES)
typedef struct sSysSurfaceDesc { _Alignas(STD_SYS_ALIGN) uint8_t aData[STD_SYS_SURFACEDESC_SIZE]; } tSysSurfaceDesc;
#endif

// Libs/std/Win95/DX9/stdSysTypesDX9.h, first include in DX9 code
#define STD_SYS_BACKEND_TYPES
typedef D3DSURFACE_DESC tSysSurfaceDesc;
static_assert(sizeof(D3DSURFACE_DESC) == STD_SYS_SURFACEDESC_SIZE, "backend size must match");
```

- **Why not a pointer to separately allocated state:** that changes the layout and adds an allocation per texture. This way the layout stays byte-identical.
- **Backend code doesn't change.** It keeps reading fields like `pBuf->surface.desc.Width`.
- **Size checks:** the `static_assert`s in the backend header replace the commented-out size checks in `std/types.h`.

### 5. Neutral types, translated at the edge

When the engine really uses the contents, it gets its own definition, and the backend converts or checks that it matches.

| Data | Engine definition | How the backend adapts |
|---|---|---|
| Vertices | `StdVertexTL`, same 32 bytes | DX uses the same layout, and a size check proves it. Other backends copy |
| Key codes | `STDKEY_*`, same numbers as DirectInput | DX uses them directly, with `static_assert(STDKEY_X == DIK_X)`. Other backends translate |
| Colors | `uint32_t`, `0xAARRGGBB` | DX passes through. Other backends swizzle |
| Thumbnails | Pixel buffer | The backend converts from `HBITMAP` |

### 6. Capability query

Code above the line must not read a backend's capability struct. It asks a function, and the backend answers. For example, `JonesMain_CurDisplaySupportsBPP` (`JonesMain.c:3368`) and its copy in `jonesConfig.c:7746` read `d3dDesc.dwDeviceRenderBitDepth` inside `#if defined(J3D_DIRECTX6)`.

```c
/** Returns whether the 3D device can render at the given bit depth. */
bool std3D_DeviceSupportsBitDepth(const Device3D* pDevice, size_t bpp);
```

The backend owns the check, and the `#if` goes away from both callers. The two callers don't treat every depth alike, so M3.6 reads both and keeps each caller's results unchanged.

### 7. Registered callback

Dependencies only go down, so lower code reaches upward through a function that upper code gave it. `wkernel` already does this. It needs two fixes:

- **`WKERNELWNDPROC` leaks `HWND`, `UINT`, `WPARAM` and `LPARAM`.** The window procedure moves to `Jones3D/Platform/Win32/`, and the engine side gets `JonesMain_On*` functions.
- **`stdConfig.c` calls `wuRegistry_*` directly,** which breaks the layering. `Jones3D` registers a fallback reader at startup and the read order stays the same.

### 8. Null object

A backend that does nothing, but honors the interface's contract: draws are discarded, no key is ever pressed, a dialog returns its documented default. Callers never test for it. It makes M4 possible, gives tests a backend with no device, and is the proof that an interface doesn't secretly need one platform.

### Moving code as a block

Not a separation pattern, but the choice for dialogs. `jonesConfig.c` and `JonesDialog.c` are Win32 UI with a small external surface, so they move below the line as they are. Their 15 functions become the interface. Splitting their logic from their UI waits until M7 needs it.

## How the separation is enforced

| Check | Catches | When |
|---|---|---|
| `Scripts/checkboundary.py --check` | Platform names and includes above the line, per file. The allowlist can only shrink | Every slice |
| Compile with no platform headers | What the text check can't see, such as names arriving through `j3d.h` | Repeated during M3, must pass at M5 |
| Null backend links | Interfaces that are incomplete or depend on one platform | M4 |
| Per-backend `static_assert`s | Size, layout and key number drift | Every build |
| Tests through the interface | Behavior that differs between backends | Every slice |

## Wrong ways to cross the line

These are refused in review, even when they would pass the check:

- **A getter that hands out the platform object.** `std3D_GetD3DDevice` and `SoundDriver_GetDSound` move to private headers.
- **Passing a platform object through the interface.** `SmushPlay_SysStartup(HWND, tDirectSound*)` gets this from `JonesMain.c:2024`, and needs its own fix in M3.3.
- **One parameter with two meanings.** `Sound.c:451` casts its `hwnd` argument to `tDirectSound*` when `SOUNDOPEN_HWNDISDIRECTSOUND` is set.
- **A renamed platform type.** `typedef HWND tSysWindow` hides the name, but the header still needs `windows.h`.
- **Backend-specific batch functions called from upper modules,** as `origin/OpenGL` does. See "Batching".

## Measurements

| Module, at the design's start | Check matches, 1,926 total | No-headers compile errors, 4,415 total |
|---|---:|---:|
| `Jones3D` | 1,507 | 3,266 |
| `Libs/sith` | 208 | 554 |
| `Libs/std` | 93 | 349, plus 22 in `Win95` headers |
| `Libs/smush` | 54 | 149 |
| `Libs/sound` | 35 | 11 |
| `Libs/rdroid` | 21 | 44 |
| `Libs/j3dcore` | 8 | 13 |

`jonesConfig.c` alone is 1,103 check matches and 2,472 errors. Compile errors include cascades, so they show where the work is, not how much of it. 47 lines above the backends test `J3D_DIRECTX*`, `_WIN32` or `_MSC_VER`.

## Where the layer is

- **Interface:** the existing headers, at their existing paths: `Libs/std/Win95/*.h`, `Libs/sound/Driver.h`, `Libs/wkernel/wkernel.h`, `Libs/w32util/wuRegistry.h`. The `Win95` path name is wrong for an interface, but moving it changes every include and makes upstream review harder. A rename can be a single separate commit at the end.
- **Backends:** `Libs/std/Win95/DX6/`, `Libs/std/Win95/DX9/`, `Libs/sound/DriverDX*.c`, and new `Libs/std/Null/` in M4. Win32 code that lives above `std` moves to `Jones3D/Platform/Win32/`.
- **The check** skips only backends and platform `.c` files, so leaks in interface headers count.

## Selection

Build time, as DX6 and DX9 already work: one implementation linked per build, chosen in CMake. No function-pointer tables. `tHostServices` stays as it is. This keeps the original symbol names, which hooks and binary comparison rely on.

## Names

Recovered names stay wherever they fit: `std3D_`, `stdDisplay_`, `stdControl_`, `stdComm_`, `wkernel_`, `wuRegistry_`, `Sound_`. A signature changes only where it leaks a platform type. `RTI/symbols.h` is updated with it, so the injected build still compiles. On 32-bit Windows the replacement types have the same size and calling convention, so hooks keep working.

`J3D_HOOKFUNC` lines stay. In builds without injection, including native ones, the macro expands to nothing. `VirtualProtect` and `FlushInstructionCache` in `j3dhook.h` compile only for the injected build.

## Types

All replacements keep size and layout on 32-bit, checked by the existing `static_assert`s.

| Today | Replacement | Note |
|---|---|---|
| `HWND`, `HANDLE`, `HDC`, `HBITMAP`, `HFONT` | `tSysWindow`, `tSysHandle` and so on: pointers to declared but undefined structs | Backends map the tag to the real type. Follows the `tSys*` pattern |
| `IDirect3DTexture9` and other `tSys*` interface pointers | The same `tSys*` names, as opaque structs | The shared header no longer picks a type by backend |
| `tSysSurfaceDesc`, `tSysDisplayDeviceCaps`, `tSysDevice3DDesc`, `tSysPixelFormat`, embedded by value | The same names, as backend-sized byte arrays | See pattern 4 |
| `tDirectSound` in `sound/Driver.h` | Moves to a private DX header, or becomes an opaque handle where `Sound.c` and `SmushPlay` still pass it | M3.3 decides. |
| `DWORD`, `WORD`, `BYTE`, `LONG` in shared structs | `uint32_t`, `uint16_t`, `uint8_t`, `int32_t` | Same size on 32-bit |
| `DPID` | `typedef uint32_t DPID`, keeping the recovered name | Also keeps the struct size on 64-bit |
| `GUID` | `tSysGuid`, 16 bytes | Used in device and player structs |
| `D3DCOLOR` | `uint32_t`, documented as `0xAARRGGBB` | A guarantee for backends that swizzle |
| `D3DTLVERTEX`, `LPD3DTLVERTEX` | `StdVertexTL`, same 32-byte layout | DX backends cast, with a size check |
| `LPWORD` | `uint16_t*` | |
| `WAVEFORMATEX` in `sound/types.h` | Engine struct, same layout | |
| `RECT`, `POINT` | Stay private to Win32 dialog code | Only dialog state uses them |

`j3d.h` and `std/types.h` lose their platform includes. Win32 backends include platform headers themselves.

## Subsystems

| Subsystem | Interface | What changes |
|---|---|---|
| Time | `stdPlatform_GetTimeMsec`, `stdPlatform_InitHighResTimer` | `timeGetTime` and `QueryPerformance*` move to a Win32 file |
| Timer thread | New small function for `smush/insane/Timer/Timer.c` | Its own `CreateThread` loop moves down. Thread priority and period stay |
| Files and search | `stdFileUtil_*`, unchanged | `FindFirstFile` moves down. `FindFileData.handle` becomes `tSysHandle` |
| GOB mapping | `stdGob_*` | `Gob`'s `HANDLE` and `LPVOID` fields become engine types |
| Registry and config | `wuRegistry_*` | `stdConfig.c` falls back to the registry. It gets a callback set by `Jones3D`, which removes the `std -> w32util` break and keeps the read order |
| Input | `stdControl_*`, with `STDKEY_*` | See below |
| Sound | `SoundDriver_*`, already a boundary | `tSysSoundBuffer` becomes opaque. `Sound.c`'s one test goes |
| Cutscene sound | `SNDDRIVER`, which `Engine.c` already uses | `Dsnd.c` is a backend as a whole, and another platform replaces the file |
| Window and lifecycle | `wkernel_*` | See below |
| Dialogs | `jonesConfig_*`, `JonesDialog_*` | See below |
| Thumbnails | `stdBmp_Load`, `sithGamesave_*Thumbnail` | The code moves to Win32 files and `HBITMAP` becomes `tSysBitmap`. A pixel buffer waits for the in-game UI that needs it |
| Renderer | `std3D_*`, `stdDisplay_*` | See below |
| Networking | `stdComm_*` | Types only. A null backend in M4 reports no game |

**Input.** `STDKEY_*` constants replace `DIK_*` in `sithControl.c`, `JonesHud.c`, `JonesDisplay.c`, `JonesMain.c` and `jonesConfig.c`, with the same numbers. The values are treated as a contract, because stored control settings may depend on them; M3.2 confirms where they're stored. The Win32 backend checks each with `static_assert(STDKEY_X == DIK_X)`. Other backends translate into these numbers.

**Window.** `JonesMain.c` handles about a dozen `WM_` messages in its window procedure. The Win32 part moves to `Jones3D/Platform/Win32/` and calls platform-neutral `JonesMain_On*` functions for activate, close and similar events, in the same order. `wkernel_Run`'s `WinMain` parameters move to the Win32 entry point.

**Dialogs.** Outside its own file, the code uses only these from `jonesConfig`: `Startup`, `Shutdown`, `UpdateCurrentLevelNum`, `GetLoadGameFilePath`, `GetSaveGameFilePath`, and `ShowControlOptions`, `ShowDialogInsertCD`, `ShowDisplaySettingsDialog`, `ShowExitGameDialog`, `ShowGameOverDialog`, `ShowGamePlayOptions`, `ShowLevelCompletedDialog`, `ShowMessageDialog`, `ShowSoundSettingsDialog`, `ShowStatisticsDialog`. From `JonesDialog`, 3 names are used. So `jonesConfig.c` and `JonesDialog.c` move as they are to `Jones3D/Platform/Win32/`. The header keeps these functions with engine types. Null versions return each dialog's documented default. Logic that an in-game UI needs later gets extracted at M7.

**Renderer.** See the example below. Debug and device accessors such as `std3D_GetD3DDevice`, `stdDisplay_GetDirect3D`, `stdDisplay_GetSystemDevice` and `std3D_D3DGetStatus` move to a private DX header. `std3D_SetProjection` and `stdDisplay_RemoveBufferClipper` return a plain `int`, with 0 for success. Their only known caller ignores the projection result.

## Batching

Batching happens below `std3D`. A backend may defer draws, as long as they appear in submission order by `std3D_EndScene`. `rdCache_Flush` stays one code path for every backend, and DX9 keeps drawing immediately.

`origin/OpenGL` went the other way: `#ifdef J3D_OPENGL` in `rdCache.c`, `sithRender.c`, `sithShadow.c` and the primitives, and batch functions such as `std3D_DrawModelBatch`. Some of that serves instancing, per-pixel lighting and sky shaders, which need data `std3D_DrawRenderList` doesn't receive. Those features become additive functions when requested. **Unverified:** whether a backend that sees only the current calls performs well enough. `rdCache` already groups by texture, and M6 finds out.

## Example headers

**Time:** no caller changes.

```c
/** Milliseconds since an unspecified start. Monotonic, and wraps at 2^32. Valid after stdPlatform_InitServices. */
unsigned int stdPlatform_GetTimeMsec(void);
```

Win32 uses the existing timer code, and null a monotonic clock. **Must not change:** tick source and resolution on Win32.

**Input:** callers change a constant's name, not its number.

```c
// Values equal the DirectInput scan codes. Stored control settings may depend on them.
#define STDKEY_ESCAPE 0x01
#define STDKEY_RETURN 0x1C
#define STDKEY_LEFT   0xCB

// Unchanged, from Libs/std/Win95/stdControl.h:303.
int J3DAPI stdControl_ReadKey(size_t keyNum, int* pbPressed);
```

DX9 and DX6 keep their DirectInput code, and null reports nothing pressed. **Must not change:** key numbers and read order.

**Renderer:** callers change a type's name, not its layout.

```c
typedef struct sStdVertexTL
{
    float sx, sy, sz, rhw;
    uint32_t color;    // 0xAARRGGBB
    uint32_t specular; // 0xAARRGGBB
    float tu, tv;
} StdVertexTL;
static_assert(sizeof(StdVertexTL) == 32, "sizeof(StdVertexTL) == 32");

typedef struct sSysTexture tSysTexture; // Owned by the backend

/**
 * Submits indexed triangles. May draw now or later, and draws appear in submission order by std3D_EndScene.
 * The arrays need only stay valid during the call. Call between std3D_StartScene and std3D_EndScene.
 */
void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, StdVertexTL* aVerts, size_t numVerts, uint16_t* aIndices, size_t numIndices);
```

The parameters match `Libs/std/Win95/std3D.h:48`, with only the platform types replaced. DX9 sees the real types and draws now. Null discards. A deferred backend copies. No caller needs an `#ifdef`. **Must not change:** draw order and render states on DX9.

## Beyond Windows: seen early, solved in M4

- **Standard C through `windows.h`:** `atoi`, `rand`, `errno` and more. M3.0 adds real includes.
- **Microsoft CRT:** `sscanf_s` (58 uses), `vsnprintf_s`, `strncpy_s`, `strnlen_s`, `fopen_s`, `strcmpi`. A shim in `j3dcore`, matching Microsoft's truncation behavior, with tests. `sscanf_s` takes buffer sizes as extra arguments, so it needs a real wrapper.
- **Size assertions:** 136 `sizeof` checks fail on 64-bit. They hold only for 32-bit layouts, so they become 32-bit-only, and on-disk data gets separate fixed-layout structs.

## Design inputs

- **`origin/OpenGL`:** it adds `Libs/std/Win95/GL/` and, in `std3D.h`, batch functions behind `#ifdef`s. **Take:** its idea of backend files beside DX9. **Avoid:** `#ifdef`s in upper modules, and bypassing `std3D_DrawRenderList`. **Unverified:** its quality, since it hasn't been run.
- **OpenJKDF2:** the same engine family, whose README reports 64-bit Linux, macOS and Windows with SDL and OpenGL 3.3. So this kind of port is known to work. **Take:** keeping original names, and platform directories. **Avoid:** its `#ifdef SDL2_RENDER` in shared headers, and keeping Windows names like `LPCSTR` in them. Only its headers and README were read. It appears to carry a permissive license for its own code, but it should be checked before copying anything beyond ideas.
