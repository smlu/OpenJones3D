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

The engine does not let gameplay code talk to the platform input API or raw key codes directly.

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

The DX9 backend uses DirectInput8 for the keyboard, mouse, and joysticks, and XInput for gamepads. DirectInput devices are enumerated at startup. XInput pads are scanned at startup and rescanned every two seconds while the game runs; a connected pad is appended to the joystick list after the DirectInput joysticks. A pad registers six axes: X and Y on the left stick, RX and RY on the right stick, and Z and RZ on the left and right triggers. Its A/B/X/Y, left/right shoulder, Back, Start, and stick-click buttons map to joystick buttons 0 through 9, and the d-pad drives POV 0. Trigger values below 30 (out of 255) read as zero.

The device readers copy the raw state into shared arrays. Keyboard state is the DirectInput key array. Joysticks are polled and their `DIJOYSTATE` axes (X, Y, Z, RX, RY, RZ), buttons, and POV hats are copied into the axis and key state. Mouse X, Y, and Z are deltas since the previous read; Z is the wheel delta in DirectInput units (a wheel notch is 120), and Y is positive down. With mouse sensitivity enabled, the accumulated mouse deltas are clamped to the registered axis range. The DX6 backend has the same shape but uses the older DirectInput interfaces and has no XInput path.

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

## Control Scheme Files

The hardcoded defaults above are not the only source of bindings. The game also supports the original `.kfg` control schemes, implemented in [`jonesConfig.c`](../../Jones3D/Display/jonesConfig.c).

The active scheme is named by the `controls.configFile` key in `Jones.cfg`. [`jonesConfig_Startup()`](../../Jones3D/Display/jonesConfig.c#L1177) sets up the key-set path, loads the active scheme through [`jonesConfig_LoadActiveControlScheme()`](../../Jones3D/Display/jonesConfig.c#L2049), and applies it with [`jonesConfig_BindControls()`](../../Jones3D/Display/jonesConfig.c#L2582). Scheme files live in the key-sets directory: `<installPath>\<keysets>` when `installPath` is set, otherwise `..\<keysets>` relative to the working directory. The `<keysets>` name comes from the `SITHSTRING_KEYSETS` string.

A scheme file starts with a localized heading, the enabled marker `1`, and the `Actions`/`Keys` column header, then one quoted action line for each of the 37 actions followed by its quoted bindings. The columns are padded and the file uses CRLF line endings. [`jonesConfig_LoadControlScheme()`](../../Jones3D/Display/jonesConfig.c#L2140) reads it and rejects a truncated or malformed file. The format can name keyboard keys, joystick buttons, POV directions, and the X, Y, Z, and RZ joystick axes. It cannot name mouse buttons or the right-stick RX/RY axes. Mouse bindings come from the hardcoded [`sithControl_RegisterMouseBindings()`](../../Libs/sith/Devices/sithControl.c#L842) defaults and the `controls.mouse` option instead.

The default schemes are complete tables in `jonesConfig.c`, built by `jonesConfig_SetDefaultControlScheme()`. When a scheme is bound, its bindings replace the engine's for each action it covers: for example, activate uses Ctrl and the first joystick button instead of the grave key, and `I` toggles the HUD menu. For every scheme, `jonesConfig_BindControls()` skips joystick bindings while `controls.controller` is false, and applies `controls.mouse` through [`jonesConfig_EnableMouseControl()`](../../Jones3D/Display/jonesConfig.c#L4888).

The scheme's interface action is also the HUD menu key. [`JonesMain_ProcessMenu()`](../../Jones3D/Main/JonesMain.c#L1324) checks the keys bound from that action and Escape, ignores them while Ctrl is held, and toggles the HUD menu or hides the console.
