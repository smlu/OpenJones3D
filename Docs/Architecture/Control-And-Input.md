# Control And Input

This page documents how raw keyboard, mouse, and joystick state becomes gameplay intent inside Jones3D.

The input architecture is intentionally layered:

1. a platform/device layer (`stdControl`)
2. a gameplay binding layer (`sithControl`)
3. gameplay consumers such as the player controller and camera logic

Primary source files:

- [`Libs/sith/Devices/sithControl.c`](../../Libs/sith/Devices/sithControl.c)
- [`Libs/std/Win95/stdControl.h`](../../Libs/std/Win95/stdControl.h)
- [`Libs/std/Win95/DX9/stdControlDX9.c`](../../Libs/std/Win95/DX9/stdControlDX9.c)
- [`Libs/std/Win95/DX6/stdControlDX6.c`](../../Libs/std/Win95/DX6/stdControlDX6.c)
- [`Libs/sith/Gameplay/sithPlayerControls.c`](../../Libs/sith/Gameplay/sithPlayerControls.c)
- [`Libs/sith/Engine/sithCamera.c`](../../Libs/sith/Engine/sithCamera.c)

## Architectural Split

The engine does not let gameplay code talk to DirectInput or raw key codes directly.

Instead it uses:

- `stdControl` for device enumeration, acquisition, raw state reads, axis normalization, and idle detection
- `sithControl` for logical gameplay functions such as forward, jump, activate, fire, look, and camera mode

That split is important because it lets the gameplay layer speak in terms of "actions" and "axes" rather than hardware details.

## Gameplay Binding Layer

The binding layer is centered in [`sithControl.c`](../../Libs/sith/Devices/sithControl.c#L1).

Its core state includes:

- a callback list for gameplay control consumers
- a function-flag table describing which logical functions are keys or axes
- a binding table that maps logical functions to one or more keys, buttons, or axes
- a hardcoded function-name list used by config/save logic and UI

The important entry points are:

- [`sithControl_Startup()`](../../Libs/sith/Devices/sithControl.c#L132)
- [`sithControl_Open()`](../../Libs/sith/Devices/sithControl.c#L158)
- [`sithControl_Reset()`](../../Libs/sith/Devices/sithControl.c#L679)
- [`sithControl_RegisterControlFunctions()`](../../Libs/sith/Devices/sithControl.c#L685)
- [`sithControl_DefaultInit()`](../../Libs/sith/Devices/sithControl.c#L587)

### Registered Gameplay Functions

[`sithControl_RegisterControlFunctions()`](../../Libs/sith/Devices/sithControl.c#L685) builds the engine's logical action namespace.

Examples include:

- movement and turning
- jump and crawl toggle
- weapon and item activation
- map, save, screenshot, and camera controls
- mouse-look and pitch-turn axes

This is the stable contract the rest of the gameplay code consumes.

### Default Bindings

[`sithControl_DefaultInit()`](../../Libs/sith/Devices/sithControl.c#L587) resets the binding tables and then installs hardcoded defaults through:

- [`sithControl_RegisterKeyboardBindings()`](../../Libs/sith/Devices/sithControl.c#L751)
- [`sithControl_RegisterJoystickBindings()`](../../Libs/sith/Devices/sithControl.c#L815)
- [`sithControl_RegisterMouseBindings()`](../../Libs/sith/Devices/sithControl.c#L842)

Architecturally, the defaults are still data baked into code rather than an external action-map asset. The runtime config system changes where those bindings are stored and reloaded, but the underlying action model is still the original one.

## Platform Device Layer

The platform/input backend lives in `stdControl`. In current builds the default path is the DX9 Win32 implementation in [`stdControlDX9.c`](../../Libs/std/Win95/DX9/stdControlDX9.c#L209), while the legacy DX6 build uses the parallel implementation in [`stdControlDX6.c`](../../Libs/std/Win95/DX6/stdControlDX6.c#L163).

The two backends follow the same architecture:

- startup creates the DirectInput side and enumerates available devices
- open acquires the runtime control state
- per-frame reads update raw key/button/axis arrays
- helper functions expose normalized axis and key state to `sithControl`

Important entry points in the DX9 path are:

- [`stdControl_Startup()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L209)
- [`stdControl_Open()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L309)
- [`stdControl_ReadControls()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L371)
- [`stdControl_EnableAxis()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L351)
- [`stdControl_SetActivation()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L659)

The backend also has dedicated device readers:

- [`stdControl_ReadKeyboard()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L1058)
- [`stdControl_ReadJoysticks()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L1082)
- [`stdControl_ReadMouse()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L1161)

## Per-Frame Read Pass

[`stdControl_ReadControls()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L371) is the raw device sampling step.

Each frame it:

1. resets idle state and just-pressed state
2. captures current read time and delta time
3. derives frame-rate-style timing helpers used by the higher binding layer
4. clears axis state accumulators
5. reads keyboard, joystick, and mouse devices that are currently enabled
6. stores the read time as the new baseline

This means the gameplay layer sees one coherent input snapshot per frame rather than each gameplay system polling devices independently.

## Axis And Key Normalization

The platform layer exposes several different views of the same raw data:

- [`stdControl_ReadAxis()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L413) returns a normalized analog-style value with deadzone handling
- [`stdControl_ReadAxisRaw()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L462) returns the raw centered delta
- [`stdControl_ReadKeyAsAxis()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L494) turns a digital key hold into an axis-like value proportional to read time
- [`stdControl_ReadKey()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L586) returns direct key/button state
- [`stdControl_ControlsIdle()`](../../Libs/std/Win95/DX9/stdControlDX9.c#L765) exposes whether the current frame's reads were effectively idle

This is why the higher layer can represent both styles cleanly:

- digital actions such as jump or activate
- analog or pseudo-analog inputs such as turning, pitch, or wheel scrolling

## Binding Resolution In `sithControl`

Once raw input is available, the gameplay layer resolves it through binding lists:

- [`sithControl_GetKey()`](../../Libs/sith/Devices/sithControl.c#L545)
- [`sithControl_GetAxis()`](../../Libs/sith/Devices/sithControl.c#L512)
- [`sithControl_GetKeyAsAxis()`](../../Libs/sith/Devices/sithControl.c#L476)
- [`sithControl_GetKeyAsAxisNormalized()`](../../Libs/sith/Devices/sithControl.c#L421)

The binding layer can:

- merge multiple bindings into one logical function
- apply axis inversion
- apply per-binding sensitivity
- treat some button bindings as axis contributors
- suppress certain joystick reads depending on control options

This lets the engine map many hardware forms onto the same movement and camera model without rewriting the gameplay code.

## Callback Dispatch And Camera Side Effects

[`sithControl_Update()`](../../Libs/sith/Devices/sithControl.c#L334) is the high-level runtime entry point called from the frame loop.

It performs two jobs.

### 1. Idle-Camera Management

Before dispatching gameplay controls, [`sithControl_Update()`](../../Libs/sith/Devices/sithControl.c#L334) checks:

- whether a local player exists
- whether the player is stationary
- whether camera cutscene mode is inactive
- whether the player is not hanging, climbing, or in a vehicle
- whether raw controls have been idle long enough

If so, it switches the camera through:

- [`sithCamera_SetCameraFocus()`](../../Libs/sith/Engine/sithCamera.c#L356)
- [`sithCamera_SetCurrentCamera()`](../../Libs/sith/Engine/sithCamera.c#L288)

So the control layer is not just about reading input. It also owns one of the engine's user-experience policies: the automatic idle camera.

### 2. Gameplay Control Callback Dispatch

After camera management, [`sithControl_Update()`](../../Libs/sith/Devices/sithControl.c#L334) reads controls once through [`sithControl_ReadControls()`](../../Libs/sith/Devices/sithControl.c#L576), dispatches the registered callback chain, and finalizes the pass through [`sithControl_FinishRead()`](../../Libs/sith/Devices/sithControl.c#L582).

The callback model means:

- the raw device state is sampled once per frame
- gameplay consumers all see the same snapshot
- the first callback that fully handles the frame can stop further callback processing

## Main Gameplay Consumer: Player Controls

The main gameplay consumer logic lives in [`sithPlayerControls_Process()`](../../Libs/sith/Gameplay/sithPlayerControls.c#L471).

That function fans control input into several sub-pipelines:

- [`sithPlayerControls_ProcessLookControls()`](../../Libs/sith/Gameplay/sithPlayerControls.c#L833)
- [`sithPlayerControls_ProcessGeneralMove()`](../../Libs/sith/Gameplay/sithPlayerControls.c#L900)
- [`sithPlayerControls_ProcessHUDControls()`](../../Libs/sith/Gameplay/sithPlayerControls.c#L1565)
- [`sithPlayerControls_ProcessWeaponAim()`](../../Libs/sith/Gameplay/sithPlayerControls.c#L2410)

This is where logical actions such as forward, jump, activate, turn, and mouse turn finally become:

- thrust and angular velocity
- movement-state changes
- interaction attempts
- HUD toggles
- aim-mode changes and weapon messaging

For example, aim-mode transitions eventually emit script-visible aim messages through [`sithWeapon_SendMessageAim()`](../../Libs/sith/World/sithWeapon.c#L1984).

## Why The Layering Matters

The input stack is easy to summarize as:

- `stdControl` owns devices
- `sithControl` owns action binding and per-frame dispatch
- `sithPlayerControls` and related gameplay systems own interpretation

That separation is one of the cleaner parts of the engine architecture. Even though the code is old-school and heavily procedural, the responsibilities are well split:

- hardware details do not leak into player movement code
- gameplay code does not need to know which device produced an action
- camera idling and control policies can sit above the raw device layer without contaminating it
