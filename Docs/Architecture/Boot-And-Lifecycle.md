# Boot And Lifecycle

This page documents how control enters OpenJones3D, how the subsystem stack is started, how worlds are opened, and how a frame flows from the outer Windows host loop into the gameplay and renderer layers.

Primary source files:

- [`Jones3D/dllmain.c`](../../Jones3D/dllmain.c)
- [`Jones3D/Main/JonesMain.c`](../../Jones3D/Main/JonesMain.c)
- [`Libs/wkernel/wkernel.c`](../../Libs/wkernel/wkernel.c)
- [`Libs/std/General/std.c`](../../Libs/std/General/std.c)
- [`Libs/std/General/stdPlatform.c`](../../Libs/std/General/stdPlatform.c)
- [`Libs/rdroid/Main/rdroid.c`](../../Libs/rdroid/Main/rdroid.c)
- [`Libs/sith/Main/sithMain.c`](../../Libs/sith/Main/sithMain.c)

## Process Entry

The game does not start in [`JonesMain_Startup()`](../../Jones3D/Main/JonesMain.c#L254) directly. The real entry chain is:

1. [`Indy3D_WinMain()`](../../Jones3D/dllmain.c#L219) in [`Jones3D/dllmain.c`](../../Jones3D/dllmain.c)
2. [`wkernel_SetProcessProc()`](../../Libs/wkernel/wkernel.c#L104), [`wkernel_SetStartupCallback()`](../../Libs/wkernel/wkernel.c#L111), and [`wkernel_SetShutdownCallback()`](../../Libs/wkernel/wkernel.c#L118)
3. [`wkernel_Run()`](../../Libs/wkernel/wkernel.c#L46)
4. [`JonesMain_Startup()`](../../Jones3D/Main/JonesMain.c#L254)

[`Indy3D_WinMain()`](../../Jones3D/dllmain.c#L219) creates the single-instance mutex, installs the top-level callbacks, and hands control to [`wkernel_Run()`](../../Libs/wkernel/wkernel.c#L46). `wkernel` is the outer host shell: it creates the main window, pumps Win32 messages, calls the startup callback once, and then repeatedly calls the registered process callback until the application requests exit.

That means the runtime has a clear split:

- `wkernel` owns the outer process loop and window/event integration.
- `JonesMain` owns game startup, display startup, menus, level loading, and the current per-frame game callback.

## Host Services

Before the gameplay systems are opened, OpenJones3D builds a host-service table and passes it downward into the legacy-style subsystems.

The setup path is:

1. [`stdPlatform_InitServices(&JonesMain_hs)`](../../Libs/std/General/stdPlatform.c#L41)
2. [`stdStartup(&JonesMain_hs)`](../../Libs/std/General/std.c#L56)
3. [`rdSetServices(&JonesMain_hs)`](../../Libs/rdroid/Main/rdroid.c#L36)
4. [`sithSetServices(&JonesMain_hs)`](../../Libs/sith/Main/sithMain.c#L145)
5. [`sithSound_Initialize(&JonesMain_hs)`](../../Libs/sith/Devices/sithSound.c#L35)

The service table gives lower layers access to:

- logging and assert callbacks
- memory allocation
- timing
- file I/O
- handle-style allocation helpers

This is one of the original architectural patterns that OpenJones3D keeps intact. `std`, `rdroid`, `sith`, and `sound` are not hardwired to the top-level application code; they are initialized through a host-service interface.

## JonesMain Startup Sequence

[`JonesMain_Startup()`](../../Jones3D/Main/JonesMain.c#L254) is the main bootstrap function. The important stages are:

1. Build and install host services.
2. Start the common support modules:
   - [`stdStartup()`](../../Libs/std/General/std.c#L56)
   - [`rdSetServices()`](../../Libs/rdroid/Main/rdroid.c#L36)
   - [`sithSetServices()`](../../Libs/sith/Main/sithMain.c#L145)
   - [`sithSound_Initialize()`](../../Libs/sith/Devices/sithSound.c#L35)
3. Start file and string systems:
   - [`JonesFile_Startup()`](../../Jones3D/Main/JonesFile.c#L86)
   - [`jonesString_Startup()`](../../Jones3D/Main/jonesString.c#L51)
   - [`sithString_Startup()`](../../Libs/sith/Main/sithString.c#L26)
4. Start configuration:
   - [`wuRegistry_Startup(...)`](../../Libs/w32util/wuRegistry.c#L32)
   - [`stdConfig_Startup("Jones.cfg")`](../../Libs/std/General/stdConfig.c#L65)
5. Load display environment and saved settings.
6. Start audio device/runtime state.
7. Start the main engine layers:
   - [`stdEffect_Startup()`](../../Libs/std/General/stdEffect.c#L19)
   - [`rdStartup()`](../../Libs/rdroid/Main/rdroid.c#L46)
   - [`sithStartup()`](../../Libs/sith/Main/sithMain.c#L155)
8. Start Jones-specific systems:
   - [`JonesConsole_Startup()`](../../Jones3D/Display/JonesConsole.c#L99)
   - [`JonesHud_Startup()`](../../Jones3D/Display/JonesHud.c#L411)
   - [`JonesDisplay_Startup()`](../../Jones3D/Display/JonesDisplay.c#L72)
   - [`JonesControl_Startup()`](../../Jones3D/Play/JonesControl.c#L29)
   - [`jonesCog_Startup()`](../../Jones3D/Play/jonesCog.c#L68)
9. Open the static resource world.
10. Open the target gameplay world or restore a savegame.

There are two important architectural points here:

- The application layer starts `rdroid` and `sith`, but once they are open it mostly interacts with them through public subsystem entry points.
- Startup is intentionally front-loaded. By the time the frame loop begins, the current world, current camera, display mode, audio runtime, script runtime, and HUD are already live.

## Registry And JSON Config Coexistence

OpenJones3D keeps the original registry-facing startup module, but layers a JSON config system on top of it.

[`stdConfig_Startup()`](../../Libs/std/General/stdConfig.c#L65) opens or creates `Jones.cfg`, enables auto-save, and installs a mapping table between JSON keys and original registry values. Reads first consult JSON; if a key is missing, `stdConfig` falls back to the mapped registry value and then writes that value into the JSON file.

Architecturally, that matters because configuration is no longer an application-only detail. Runtime subsystems such as rendering, sound, physics, HUD, and gameplay helpers pull settings from `stdConfig`, which makes them easier to configure and easier to extend than the original registry-only model.

## Display And Rendering Device Startup

[`JonesDisplay_Startup()`](../../Jones3D/Display/JonesDisplay.c#L72) is the application-layer bridge into the lower display stack.

Its job is:

1. start the display subsystem
2. pick and apply a display mode
3. open the 3D subsystem
4. start the camera system
5. open the selected 3D device
6. open `rdroid`
7. apply initial render settings such as mipmap filter, geometry mode, and fog state

The stack at that point is:

```text
JonesDisplay
  -> stdDisplay
  -> std3D
  -> sithCamera
  -> rdOpen
```

The exact implementation under `stdDisplay` and `std3D` depends on the build:

- DirectX 6.1c build: [`Libs/std/Win95/DX6/`](../../Libs/std/Win95/DX6)
- DirectX 9 build: [`Libs/std/Win95/DX9/`](../../Libs/std/Win95/DX9)

That backend split is discussed in more detail in [Rendering-And-Visibility.md](Rendering-And-Visibility.md) and [QOL-And-Compatibility.md](QOL-And-Compatibility.md).

## Static World And Gameplay World

OpenJones3D uses two world instances:

- `sithWorld_g_pStaticWorld`
- `sithWorld_g_pCurrentWorld`

The static world is opened first through [`sithOpenStatic()`](../../Libs/sith/Main/sithMain.c#L286). It loads shared/static resources from `Jones3DSTATIC.cnd` or `Jones3DSTATIC.ndy`.

The gameplay world is then opened through [`sithOpenNormal()`](../../Libs/sith/Main/sithMain.c#L338), which creates the current world, loads the level, initializes players, and then calls [`sithOpen()`](../../Libs/sith/Main/sithMain.c#L441).

[`sithOpen()`](../../Libs/sith/Main/sithMain.c#L441) opens the per-world runtime systems in this order:

- event system
- animation system
- voice system
- AI system
- COG instance state
- controls
- player control state
- AI awareness
- renderer
- player gameplay state
- weapon system
- overlay map
- savegame support

Finally, [`sithOpenPostProcess()`](../../Libs/sith/Main/sithMain.c#L368) completes world activation by:

- resetting time
- resetting inventory state
- broadcasting startup messages to COGs
- sending `INITIALIZED` and `CREATED` to thing cogs
- preparing autosave or multiplayer welcome state
- resetting FX state

This is the moment the world stops being "loaded data" and becomes an active simulation.

## The Outer Frame Loop

Once startup is complete, `wkernel` repeatedly calls [`JonesMain_Process()`](../../Jones3D/Main/JonesMain.c#L1204), which forwards to the current process callback. During gameplay that is [`JonesMain_ProcessGame()`](../../Jones3D/Main/JonesMain.c#L1214).

The frame shape is:

1. early-out if the app is suspended
2. process menu toggles
3. advance game state through [`sithUpdate()`](../../Libs/sith/Main/sithMain.c#L543)
4. clear and begin the 3D scene
5. render the world through [`sithDrawScene()`](../../Libs/sith/Main/sithMain.c#L596)
6. draw overlay map, HUD, and quick-save UI
7. flush render caches and end the scene
8. present the back buffer
9. handle display-device refresh/reset requests
10. pump window events

That gives the engine a clean separation between:

- simulation/update work in [`sithUpdate()`](../../Libs/sith/Main/sithMain.c#L543)
- draw submission in [`sithDrawScene()`](../../Libs/sith/Main/sithMain.c#L596)
- app/window concerns in [`JonesMain_ProcessGame()`](../../Jones3D/Main/JonesMain.c#L1214)

## Pause, Suspend, And Sync Modes

There are three distinct control states worth noting:

- Application suspension: `JonesMain_bSystemSuspended` stops the gameplay callback from doing work at all.
- Time pause: [`sithTime_IsPaused()`](../../Libs/sith/Gameplay/sithTime.c#L124) keeps the sound mixer running, but bypasses most simulation work.
- Sync submode: `SITH_SUBMODE_SYNC` changes [`sithUpdate()`](../../Libs/sith/Main/sithMain.c#L543) and [`sithDrawScene()`](../../Libs/sith/Main/sithMain.c#L596) behavior into a more restricted path used by synchronization/replay/network style modes.

Architecturally, pause is not implemented by stopping the outer frame loop. The host window and device loop continue, but the gameplay runtime selectively suppresses time-dependent updates.

## Shutdown

Shutdown mirrors startup from the top down.

On the Jones side, [`JonesMain_Shutdown()`](../../Jones3D/Main/JonesMain.c#L1104) tears down:

- console and HUD
- display/device state
- Jones-specific gameplay helpers
- open worlds and save/load state
- sound runtime
- engine subsystems
- config/registry/file/string state

On the engine side:

- [`sithShutdown()`](../../Libs/sith/Main/sithMain.c#L240) closes gameplay/runtime subsystems
- [`rdShutdown()`](../../Libs/rdroid/Main/rdroid.c#L58) closes renderer-facing subsystems
- [`stdShutdown()`](../../Libs/std/General/std.c#L85) frees common support services

Because subsystem ownership is layered, each layer is responsible for freeing the state it allocated. The world object owns runtime resources such as sectors, surfaces, things, models, materials, sounds, cogs, puppet classes, AI classes, and PVS tables; tearing a world down is therefore one of the central shutdown operations, not a small leaf step.
