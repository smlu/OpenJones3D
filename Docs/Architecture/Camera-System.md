# Camera System

This page documents the gameplay-facing camera subsystem: the built-in system cameras, the `SithCamera` runtime state, the per-frame interpolation logic, the collision-limited camera placement helpers, and the script-facing cutscene workflow built on top of that runtime.

Primary source files:

- [`Libs/sith/Engine/sithCamera.c`](../../Libs/sith/Engine/sithCamera.c)
- [`Libs/sith/Engine/sithCamera.h`](../../Libs/sith/Engine/sithCamera.h)
- [`Libs/sith/types.h`](../../Libs/sith/types.h)
- [`Libs/sith/Cog/sithCogFunction.c`](../../Libs/sith/Cog/sithCogFunction.c)
- [`Libs/sith/Cog/sithCogFunctionThing.c`](../../Libs/sith/Cog/sithCogFunctionThing.c)
- [`Libs/sith/Engine/sithAnimate.c`](../../Libs/sith/Engine/sithAnimate.c)
- [`Jones3D/Play/jonesCog.c`](../../Jones3D/Play/jonesCog.c)

## Scope And Role

`sithCamera` is the bridge between gameplay state and the renderer-facing `rdCamera`.

At the gameplay level it understands concepts such as:

- which thing the camera follows
- which thing the camera looks at
- whether camera motion or look direction should interpolate
- how external camera offsets change for climbing, jeeps, underwater movement, and other player states
- how temporary shake offsets decay back to zero
- how the camera should be kept out of world geometry

At the render boundary it provides the final camera matrix and view parameters used by [`rdCamera_Update()`](../../Libs/rdroid/Engine/rdCamera.c#L358) and [`sithRender_RenderScene()`](../../Libs/sith/Engine/sithRender.c#L299).

## Built-In Camera Numbers

The scripting layer addresses cameras by camera number. Valid camera numbers are `0..6`.

These numbers refer to a fixed internal array of system cameras declared in [`sithCamera.h`](../../Libs/sith/Engine/sithCamera.h#L12).

| Camera Number | Camera Type | Typical Role | Notes |
| ---: | --- | --- | --- |
| `0` | `SITHCAMERA_INTERNAL` | First-person / POV camera | Uses actor head rotation and eye offset. |
| `1` | `SITHCAMERA_EXTERNAL` | Third-person gameplay camera | The default gameplay camera outside the editor. |
| `2` | `SITHCAMERA_CINEMATIC` | Script-driven cinematic camera | Intended cutscene camera. Uses primary focus for position and secondary focus for look target. |
| `3` | Unused fixed slot | None | Present in the fixed array but not initialized as a named built-in camera during [`sithCamera_Startup()`](../../Libs/sith/Engine/sithCamera.c#L104). |
| `4` | `SITHCAMERA_IDLE` | Idle attract camera | Orbits the focus thing with a slowly rotating offset. |
| `5` | `SITHCAMERA_UNKNOWN_40` | Helper camera | Uses a fixed directional offset camera mode. |
| `6` | `SITHCAMERA_ORBITAL` | Orbital camera | Uses a stored orbital orientation around the focus thing. |

Only camera numbers `0`, `1`, and `6` participate in normal camera cycling through [`sithCamera_CycleCamera()`](../../Libs/sith/Engine/sithCamera.c#L327).

## Camera Limits And Defaults

The shared FOV constants are defined in [`sithCamera.h`](../../Libs/sith/Engine/sithCamera.h#L20):

- minimum FOV: `20.0`
- maximum FOV: `150.0`
- default FOV: `90.0`

[`sithCamera_SetCameraFOV()`](../../Libs/sith/Engine/sithCamera.c#L397) only applies values inside that range.

The default third-person rig offsets come from the internal static defaults in [`sithCamera.c`](../../Libs/sith/Engine/sithCamera.c#L28):

- external camera offset: `{ 0.0, -0.2, 0.065 }`
- external camera look offset: `{ 0.0, 0.02, 0.02 }`

[`sithCamera_RestoreExtCamera()`](../../Libs/sith/Engine/sithCamera.c#L442) restores those defaults and also resets the current external camera interpolation speed back to `0.35`.

## Runtime Camera Object

The full runtime camera object is `SithCamera`, declared in [`Libs/sith/types.h`](../../Libs/sith/types.h#L2686).

The important field groups are:

- `type`, `fov`, and `aspectRatio`
  What kind of camera this is and how it projects.
- `pPrimaryFocusThing` and `pSecondaryFocusThing`
  The gameplay things the camera follows or looks at.
- `pSector`
  The sector that currently contains the resolved camera position.
- `offset`
  The active camera-local translation, especially important for the external camera.
- `orient`
  The final world matrix for this camera.
- `lookPos` and `lookPYR`
  Cached look target and extracted look angles from the final orientation.
- `rdCamera`
  The renderer-facing camera object consumed by `rdroid`.
- `posInterpState`, `lookInterpState`, and `focusInterpState`
  Small state machines that drive camera-position, look-target, and focus-point interpolation.
- `pos`, `curLookInterp`, and `focusPos`
  The persistent interpolation working values.
- `secPosInterpDeltaTime`, `secLookInterpDeltaTime`, and `secFocusInterpDeltaTime`
  Elapsed interpolation timers in seconds.
- `bPosInterp` and `bLookInterp`
  Script-visible toggles for position and look interpolation.
- `interpSpeed`
  The main interpolation duration value.

## Interpolation State Model

The camera system does not use a generic animation graph for following and aiming. Instead, it uses a few small integer state machines inside [`sithCamera_Update()`](../../Libs/sith/Engine/sithCamera.c#L471).

### Position Interpolation

`posInterpState` is used by the cinematic and external-style camera paths:

- `-1`
  Force a snap on the next update, then move to steady state.
- `0`
  Steady state. The camera compares the newly requested position against the stored `pos`. If the target changed, it transitions to active interpolation.
- `1`
  Active interpolation. The camera advances `secPosInterpDeltaTime` each frame until it reaches `interpSpeed`, then snaps exactly to the new target and returns to state `0`.

`bPosInterp` is the on/off switch exposed to COG by [`SetCameraPosInterp()`](../COG/Functions-Thing.md#setcameraposinterp), implemented by [`sithCogFunctionThing_SetCameraPosInterp()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1227).

For the cinematic camera, enabling position interpolation effectively gives the camera a dolly-like move when the primary focus changes.

For the external camera, position interpolation smooths the resolved camera position after offset selection and collision limiting.

### Look Interpolation

`lookInterpState` is the equivalent state machine for the camera's look target:

- `-1`
  Force the current look target to snap into `curLookInterp` on the next update.
- `0`
  Steady state. If the desired look target changed, the camera enters active interpolation.
- `1`
  Active interpolation. The camera moves `curLookInterp` toward the new target over `interpSpeed`.

`bLookInterp` is exposed through [`SetCameraLookInterp()`](../COG/Functions-Thing.md#setcameralookinterp), implemented by [`sithCogFunctionThing_SetCameraLookInterp()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1204).

This is mainly relevant for the cinematic camera, where the camera position can follow one thing while the look target smoothly pans toward another.

### Focus Interpolation

`focusInterpState` is a special-case external-camera helper, not a generic camera-mode interpolation.

It smooths the external camera's followed focus point before the actual camera offset is applied:

- `-1`
  Initialize `focusPos` from the focus thing position.
- `0`
  Follow directly unless the focus thing's `z` changes enough to trigger smoothing.
- `1`
  Smooth the focus `z` value over time and return to `0` once the transition finishes.

The important detail is that this is mostly a vertical smoothing path. It is there to keep third-person camera motion steadier when the followed thing steps up, drops down, or changes height abruptly.

The external camera uses `interpSpeed / 2` for this focus smoothing, so the focus-point transition is intentionally shorter than the full camera-position interpolation window.

### Interpolation Speed And Timers

`interpSpeed` is the core duration parameter:

- external camera default: `0.35` seconds, set during [`sithCamera_Startup()`](../../Libs/sith/Engine/sithCamera.c#L104)
- cinematic camera default: `1.0` second, also set during startup

Scripts can override it per camera through [`SetCameraInterpSpeed()`](../COG/Functions-Thing.md#setcamerainterpspeed), implemented by [`sithCogFunctionThing_SetCameraInterpSpeed()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1269).

The timers:

- `secPosInterpDeltaTime`
- `secLookInterpDeltaTime`
- `secFocusInterpDeltaTime`

store elapsed time in seconds and advance by `sithTime_g_frameTimeFlex` each update.

### Snap And Seed Helpers

Two helpers are important when scripting cameras:

- [`sithCamera_SetCameraPosition()`](../../Libs/sith/Engine/sithCamera.c#L407)
  Stores a camera position immediately and resets `posInterpState` and `focusInterpState` to `0`.
- [`sithCamera_GetCameraPosition()`](../../Libs/sith/Engine/sithCamera.c#L416)
  Returns `orient.dvec`, which is the final resolved rendered position, not the interpolation cache in `pos`.

That difference matters when a script wants the actual on-screen camera position rather than the interpolation seed position.

## Startup, Open, And Reset

The built-in camera set is created by [`sithCamera_Startup()`](../../Libs/sith/Engine/sithCamera.c#L104), which uses [`sithCamera_NewEntry()`](../../Libs/sith/Engine/sithCamera.c#L230) to populate the fixed camera array.

Important defaults established here:

- first-person, third-person, cinematic, idle, helper, and orbital cameras are created
- the external camera starts with the default external offset
- the external camera interpolation speed is set to `0.35`
- the cinematic camera interpolation speed is set to `1.0`

When the render side opens, [`sithCamera_Open()`](../../Libs/sith/Engine/sithCamera.c#L143):

- binds the current canvas and aspect ratio to each built-in camera through [`sithCamera_SetCameraCanvas()`](../../Libs/sith/Engine/sithCamera.c#L281)
- resets all cameras through [`sithCamera_ResetAllCameras()`](../../Libs/sith/Engine/sithCamera.c#L180)
- immediately updates the current camera once

The reset path is important because it re-establishes gameplay-facing defaults:

- clears the cutscene state flag
- points most built-in cameras at the world's `pCameraFocusThing`
- restores the default external offset and external look offset
- restores the external camera default FOV
- in editor mode, selects the internal camera
- in normal gameplay, selects the external camera focused on the local player if no current camera was set yet

## Current Camera Selection

[`sithCamera_SetCurrentCamera()`](../../Libs/sith/Engine/sithCamera.c#L288) makes one `SithCamera` the active render camera and immediately updates it.

This does more than just swapping a pointer:

- sets `sithCamera_g_pCurCamera`
- makes the embedded `rdCamera` current immediately
- updates the cycle-camera state for internal, external, and orbital cameras
- initializes the idle camera's orbit orientation when switching into idle mode
- forces an immediate camera update so the new current camera is render-ready right away

The public cycle behavior comes from [`sithCamera_CycleCamera()`](../../Libs/sith/Engine/sithCamera.c#L327) and [`sithCamera_SetCurrentToCycleCamera()`](../../Libs/sith/Engine/sithCamera.c#L337).

## Per-Frame Placement In The Render Pipeline

Camera update happens during the draw half of the frame rather than during pure gameplay simulation.

The main path is:

1. [`sithDrawScene()`](../../Libs/sith/Main/sithMain.c#L596)
2. [`sithCamera_Update()`](../../Libs/sith/Engine/sithCamera.c#L471)
3. [`sithCamera_RenderScene()`](../../Libs/sith/Engine/sithCamera.c#L1421)
4. [`rdCamera_SetCurrent()`](../../Libs/rdroid/Engine/rdCamera.c#L145)
5. [`rdCamera_Update()`](../../Libs/rdroid/Engine/rdCamera.c#L358)
6. [`sithRender_RenderScene()`](../../Libs/sith/Engine/sithRender.c#L299)

That ordering means the camera always sees already-updated gameplay state for the current frame.

## Camera Update By Camera Type

The main branch logic lives in [`sithCamera_Update()`](../../Libs/sith/Engine/sithCamera.c#L471).

### Internal Camera

The internal camera path follows the primary focus thing directly.

Important behavior:

- starts from the focus thing orientation
- if the thing is a path mover with joint matrices, uses the first joint matrix directly
- for actors and players, applies `headPYR`
- for physics movers, adds a roll term derived from lateral velocity
- for the local player, applies the global camera angle shake offset
- translates by the thing position
- for actors and players, adds `eyeOffset`
- for the local player, also adds the global camera position shake offset
- resolves the containing sector with [`sithCollision_FindSectorInRadius()`](../../Libs/sith/Engine/sithCollision.c#L238)

So the first-person camera is not just "thing position plus look angles." It is a composition of thing orientation, actor head motion, eye offset, and temporary shake offsets.

### External Camera

The external camera path is the most complex branch in the system. It is also reused by the `SITHCAMERA_UNKNOWN_100` type.

The update stages are:

1. choose the effective external offset
2. smooth the followed focus position if needed
3. build a look-point matrix from the focus thing orientation and head pose
4. apply external look offset
5. apply external camera offset
6. resolve the wanted camera position against the world
7. optionally smooth the actual camera position
8. build the final orientation from the resolved camera position toward the look point
9. fade the focus thing out if the camera ends up too close

#### Offset selection

When the external camera follows the local player and the script has not forced a custom offset, the engine automatically selects different offsets for:

- underwater or aetherium sectors
- swim idle at the water surface
- jeep driving
- climbing up, down, left, and right
- forward and backward sliding
- jump rollback
- weapon-drawn third-person mode

If the script explicitly sets a custom external offset through [`SetExtCamOffset()`](../COG/Functions-Thing.md#setextcamoffset) or a related helper, the update path uses that override instead of recalculating a state-based offset.

#### Focus smoothing

Before the real camera body is positioned, the external camera smooths the followed focus point through `focusInterpState`.

This is mostly a height stabilizer:

- it tracks the focus thing directly most of the time
- if the `z` difference crosses the threshold, it starts a short vertical interpolation
- it uses `interpSpeed / 2` as the smoothing duration

This avoids some of the "camera bobbing over height steps" behavior that would otherwise happen if the actual camera offset were applied directly to every small vertical change.

#### Look offset translation

The external camera also has a second local offset called the look offset.

When the look offset changes through [`sithCamera_SetExtCameraLookOffset()`](../../Libs/sith/Engine/sithCamera.c#L431), the system:

- stores the previous look offset
- starts a fixed `0.5` second blend window
- interpolates from the previous look offset to the new one during later updates

This is separate from `bLookInterp`. `bLookInterp` smooths the look target for cinematic-style cameras, while the external look-offset blend smooths the external-camera rig itself.

#### Position smoothing

After the desired third-person camera position is resolved, the external camera runs the normal `posInterpState` smoothing pass.

One notable special case is `SITHPLAYERMOVE_PULLINGUP`: while that movement status is active, the interpolation path skips x/y smoothing and only smooths `z`.

#### Fade-out of the followed thing

If the final distance from the camera to the look point becomes too small, the followed thing is faded out through `sithAnimate_StartThingFadeAnim()`.

That is how the third-person camera avoids showing an oversized player model when collision pushes the camera too close to the character.

When the camera moves away again, the thing is faded back in.

### Cinematic Camera

The cinematic camera uses the clearest two-target model:

- primary focus thing = camera position anchor
- secondary focus thing = look target

The update flow is:

1. take the primary focus thing position as the wanted camera position
2. add the global camera shake position offset
3. take the secondary focus thing position as the wanted look target
4. optionally smooth position with `bPosInterp` and `posInterpState`
5. optionally smooth look target with `bLookInterp` and `lookInterpState`
6. build the final orientation with `rdMatrix_LookAt()`
7. apply the global camera angle offset
8. resolve the containing sector through [`sithCamera_SearchSectorInRadius()`](../../Libs/sith/Engine/sithCamera.c#L1440)

This is why camera-number `2` is the intended scriptable cutscene camera: it cleanly separates "where the camera is" from "what the camera looks at."

### Idle Camera

The idle camera uses:

- a stored idle orientation
- a stored idle offset
- the primary focus thing as the orbit center

It:

- transforms the idle offset through the idle orientation
- positions the camera around the target
- looks back at the target
- rotates the idle orientation a little each frame
- fades the focus thing out when the camera gets too close

This is not a script-choreography camera. It is more of an attract/demo orbit camera.

### Helper Camera And Orbital Camera

The helper camera (`SITHCAMERA_UNKNOWN_40`) builds a camera from a fixed directional vector and places it a short distance from the focus thing.

The orbital camera copies the stored global orbital orientation, translates it by the primary focus thing position, and uses that as the final camera matrix.

## Global Shake And Offset Damping

[`sithCamera_SetPOVShake()`](../../Libs/sith/Engine/sithCamera.c#L1431) installs two global transient offsets:

- position offset
- angle offset

and two decay rates:

- position decay rate
- angle decay rate

Those values are then consumed by [`sithCamera_Update()`](../../Libs/sith/Engine/sithCamera.c#L471) every frame.

The important architectural detail is that this shake state is global, not stored per camera. The currently updating camera consumes the same offsets, and the offsets decay back toward zero component by component at the end of each update.

That is why [`SetPOVShake()`](../COG/Functions-System.md#setpovshake) is best understood as a temporary camera impulse applied to the active camera behavior, not as an animation owned by one specific camera slot.

## Camera Collision And Sector Resolution

The camera system needs more than just a view matrix. It also needs a valid sector and a camera position that is not embedded inside walls or blocked by world geometry.

The key helper here is [`sithCamera_SearchSectorInRadius()`](../../Libs/sith/Engine/sithCamera.c#L1440).

Its job is:

- trace a camera-sized sweep from `startPos` toward `endPos`
- walk through adjoins when the trace crosses portals
- stop at the first blocking collision that should really stop the camera
- clamp `endPos` to the blocking distance
- return the sector containing the final camera position

This helper builds on the collision system described in [Collision-System.md](Collision-System.md), but it applies camera-specific policy:

- adjoin touches update the current sector
- items, corpses, most weapon things, and the local player are ignored as hard blockers
- a slightly inflated search radius is used so the camera stays clear of surfaces instead of grazing exactly on them

This helper is used heavily by:

- the cinematic camera
- the idle camera
- the helper camera
- the external camera

It is one of the main reasons the camera subsystem belongs in `sith` rather than in the raw renderer. Camera placement depends directly on gameplay-world collision and sector topology.

## Script-Facing Camera Control

The runtime camera logic is controlled from two COG layers:

- system verbs in [`sithCogFunction.c`](../../Libs/sith/Cog/sithCogFunction.c)
- thing/camera verbs in [`sithCogFunctionThing.c`](../../Libs/sith/Cog/sithCogFunctionThing.c)

The full verb reference lives under [../COG](../COG/README.md). This section focuses on how those verbs map onto the camera architecture.

### Core Camera Verbs

These verbs are the main camera-selection and focus-routing entry points:

- [`SetCurrentCamera()`](../COG/Functions-System.md#setcurrentcamera), implemented by [`sithCogFunction_SetCameraMode()`](../../Libs/sith/Cog/sithCogFunction.c#L984)
- [`GetCurrentCamera()`](../COG/Functions-System.md#getcurrentcamera), implemented by [`sithCogFunction_GetCameraMode()`](../../Libs/sith/Cog/sithCogFunction.c#L996)
- [`CycleCamera()`](../COG/Functions-System.md#cyclecamera), implemented by [`sithCogFunction_CycleCamera()`](../../Libs/sith/Cog/sithCogFunction.c#L1008)
- [`SetCameraFocus()`](../COG/Functions-System.md#setcamerafocus), implemented by [`sithCogFunction_SetCameraFocus()`](../../Libs/sith/Cog/sithCogFunction.c#L905)
- [`SetCameraSecondaryFocus()`](../COG/Functions-System.md#setcamerasecondaryfocus), implemented by [`sithCogFunction_SetCameraSecondaryFocus()`](../../Libs/sith/Cog/sithCogFunction.c#L929)
- [`GetPrimaryFocus()`](../COG/Functions-System.md#getprimaryfocus), implemented by [`sithCogFunction_GetPrimaryFocus()`](../../Libs/sith/Cog/sithCogFunction.c#L942)
- [`GetSecondaryFocus()`](../COG/Functions-System.md#getsecondaryfocus), implemented by [`sithCogFunction_GetSecondaryFocus()`](../../Libs/sith/Cog/sithCogFunction.c#L963)
- [`SetPOVShake()`](../COG/Functions-System.md#setpovshake), implemented by [`sithCogFunction_SetPOVShake()`](../../Libs/sith/Cog/sithCogFunction.c#L1014)
- [`SetCameraStateFlags()`](../COG/Functions-System.md#setcamerastateflags), implemented by [`sithCogFunction_SetCameraStateFlags()`](../../Libs/sith/Cog/sithCogFunction.c#L1459)
- [`GetCameraStateFlags()`](../COG/Functions-System.md#getcamerastateflags), implemented by [`sithCogFunction_GetCameraStateFlags()`](../../Libs/sith/Cog/sithCogFunction.c#L1453)

One important special case lives in [`sithCogFunction_SetCameraFocus()`](../../Libs/sith/Cog/sithCogFunction.c#L905):

- if the target camera is the cinematic camera
- and position interpolation is enabled
- and the camera already has a primary focus thing

then the wrapper seeds `pCamera->pos` from the old primary focus position before replacing the focus thing.

That is what gives cutscene scripts a usable "move from old anchor to new anchor" behavior when re-targeting camera `2`.

### Interpolation And Rigging Verbs

The second layer of verbs controls how a selected camera behaves:

- [`SetCameraPosInterp()`](../COG/Functions-Thing.md#setcameraposinterp), implemented by [`sithCogFunctionThing_SetCameraPosInterp()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1227)
- [`SetCameraLookInterp()`](../COG/Functions-Thing.md#setcameralookinterp), implemented by [`sithCogFunctionThing_SetCameraLookInterp()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1204)
- [`SetCameraInterpSpeed()`](../COG/Functions-Thing.md#setcamerainterpspeed), implemented by [`sithCogFunctionThing_SetCameraInterpSpeed()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1269)
- [`SetCameraPosition()`](../COG/Functions-Thing.md#setcameraposition), implemented by [`sithCogFunctionThing_SetCameraPosition()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1192)
- [`GetCameraPosition()`](../COG/Functions-Thing.md#getcameraposition), implemented by [`sithCogFunctionThing_GetCameraPosition()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1252)
- [`SetCameraFOV()`](../COG/Functions-Thing.md#setcamerafov), implemented by [`sithCogFunctionThing_SetCameraFOV()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1279)
- [`ResetCameraFOV()`](../COG/Functions-Thing.md#resetcamerafov), implemented by [`sithCogFunctionThing_ResetCameraFOV()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1299)
- [`SetCameraFadeThing()`](../COG/Functions-Thing.md#setcamerafadething), implemented by [`sithCogFunctionThing_SetCameraFadeThing()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1322)
- [`SetExtCamOffset()`](../COG/Functions-Thing.md#setextcamoffset), implemented by [`sithCogFunctionThing_SetExtCamOffset()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1057)
- [`SetExtCamOffsetToThing()`](../COG/Functions-Thing.md#setextcamoffsettothing), implemented by [`sithCogFunctionThing_SetExtCamOffsetToThing()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1065)
- [`SetExtCamLookOffset()`](../COG/Functions-Thing.md#setextcamlookoffset), implemented by [`sithCogFunctionThing_SetExtCamLookOffset()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1088)
- [`SetExtCamLookOffsetToThing()`](../COG/Functions-Thing.md#setextcamlookoffsettothing), implemented by [`sithCogFunctionThing_SetExtCamLookOffsetToThing()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1096)
- [`RestoreExtCam()`](../COG/Functions-Thing.md#restoreextcam), implemented by [`sithCogFunctionThing_RestoreExtCam()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1186)
- [`MakeCamera2LikeCamera1()`](../COG/Functions-Thing.md#makecamera2likecamera1), implemented by [`sithCogFunctionThing_MakeCamera2LikeCamera1()`](../../Libs/sith/Cog/sithCogFunctionThing.c#L1120)

Two practical details matter a lot for cutscenes:

- [`SetCameraFOV()`](../COG/Functions-Thing.md#setcamerafov) and [`ResetCameraFOV()`](../COG/Functions-Thing.md#resetcamerafov) operate on the current camera, not on an arbitrary camera number. Interpolated FOV changes are routed through [`sithAnimate_CameraZoom()`](../../Libs/sith/Engine/sithAnimate.c#L820).
- [`SetCameraFadeThing()`](../COG/Functions-Thing.md#setcamerafadething) is a special helper that builds a camera setup around helper things and the current external camera state. It is useful for transition/fadeplate style shots, not just ordinary follow/look setups.

### Jones Cutscene Layer

OpenJones3D also has a Jones-specific cutscene layer on top of the generic camera verbs:

- [`StartCutscene()`](../COG/Functions-Jones.md#startcutscene), implemented by [`jonesCog_StartCutsceneFunc()`](../../Jones3D/Play/jonesCog.c#L170)
- [`EndCutscene()`](../COG/Functions-Jones.md#endcutscene), implemented by [`jonesCog_EndCutscene()`](../../Jones3D/Play/jonesCog.c#L260)

These functions do not author the camera path themselves.

`StartCutscene()` is a gameplay/HUD wrapper that:

- stores or defers cutscene state when the menu is open
- closes the overlay map if needed
- hides the console
- hides or fades HUD elements depending on the cutscene type
- disables the in-game menu and gamesaving
- sets the camera cutscene flag
- makes the local player temporarily invulnerable
- sends `USER2` to the local player cog
- destroys active weapon things
- enables player cutscene mode
- stops attacking AIs

`EndCutscene()` restores that state and sends `USER3` back to the player cog.

The important architectural point is:

- [`StartCutscene()`](../COG/Functions-Jones.md#startcutscene) prepares the game for a cutscene
- generic camera verbs still define the actual cutscene shot

So a script normally uses both layers together.

### Typical Scripted Cinematic Flow

A typical cutscene flow is:

1. call [`StartCutscene()`](../COG/Functions-Jones.md#startcutscene)
2. switch to camera number `2` with [`SetCurrentCamera()`](../COG/Functions-System.md#setcurrentcamera)
3. point that camera at helper things with [`SetCameraFocus()`](../COG/Functions-System.md#setcamerafocus) and [`SetCameraSecondaryFocus()`](../COG/Functions-System.md#setcamerasecondaryfocus)
4. enable or disable interpolation with [`SetCameraPosInterp()`](../COG/Functions-Thing.md#setcameraposinterp) and [`SetCameraLookInterp()`](../COG/Functions-Thing.md#setcameralookinterp)
5. adjust shot timing with [`SetCameraInterpSpeed()`](../COG/Functions-Thing.md#setcamerainterpspeed)
6. optionally animate FOV with [`SetCameraFOV()`](../COG/Functions-Thing.md#setcamerafov)
7. optionally add shake with [`SetPOVShake()`](../COG/Functions-System.md#setpovshake)
8. optionally use [`SetCameraFadeThing()`](../COG/Functions-Thing.md#setcamerafadething) for a fadeplate/transition shot
9. when the sequence is over, restore gameplay state with [`EndCutscene()`](../COG/Functions-Jones.md#endcutscene)

One common pattern is to use ordinary invisible things as camera anchors:

- one thing marks the camera body position
- another thing marks the look target

The cinematic camera then follows those things instead of following the player directly.

Example:

```C
startup:
    StartCutscene(2);
    SetCurrentCamera(2);

    SetCameraPosInterp(2, 1);
    SetCameraLookInterp(2, 1);
    SetCameraInterpSpeed(2, 1.0);

    SetCameraFocus(2, cameraAnchor0);
    SetCameraSecondaryFocus(2, indy);
    SetCameraFOV(70.0, 1, 0.5);

timer:
    SetCameraFocus(2, cameraAnchor1);
    SetCameraSecondaryFocus(2, villain);

stop:
    SetCurrentCamera(1);
    EndCutscene();
```

This works because camera `2` treats the primary focus as the wanted camera position and the secondary focus as the wanted look target.

## Camera State Flags

The camera-state flag word is exposed through:

- [`sithCamera_SetCameraStateFlags()`](../../Libs/sith/Engine/sithCamera.c#L1479)
- [`sithCamera_GetCameraStateFlags()`](../../Libs/sith/Engine/sithCamera.c#L1484)

The currently documented runtime flag is [`SITHCAMERA_STATE_CUTSCENE`](../../Libs/sith/types.h#L1163).

Jones gameplay code uses that flag during cutscenes, but scripts can also manipulate it directly through [`SetCameraStateFlags()`](../COG/Functions-System.md#setcamerastateflags) and [`GetCameraStateFlags()`](../COG/Functions-System.md#getcamerastateflags) when they need camera-state control without using the full Jones cutscene wrapper.

## Related Documentation

- [Rendering-And-Visibility.md](Rendering-And-Visibility.md) explains how the resolved camera feeds visible-sector construction and final scene rendering.
- [Collision-System.md](Collision-System.md) explains the lower-level collision search machinery used by camera placement.
- [Control-And-Input.md](Control-And-Input.md) explains how player and camera control intent is generated before camera update reads it.
- [../COG/README.md](../COG/README.md) contains the detailed COG language and host-function references.
