# System Host Functions

Source: `Libs/sith/Cog/sithCogFunction.c`

This page covers the core verb set shared by most scripts: message context, timers, printing/debug helpers, vector math, messaging, camera helpers, misc world queries, and several compatibility verbs inherited from the original engine.

## Important Notes

- [`Print()`](#print), [`PrintInt()`](#printint), [`PrintFlex()`](#printflex), [`PrintVector()`](#printvector), and [`PrintHex()`](#printhex) only output text after [`EnablePrint(1)`](#enableprint) has been called.
- [`GetSenderID()`](#getsenderid) returns the link id attached to the dispatch, not the sender reference itself.
- [`InEditor()`](#ineditor), [`SetDebugModeFlags()`](#setdebugmodeflags), [`GetDebugModeFlags()`](#getdebugmodeflags), and [`ClearDebugModeFlags()`](#cleardebugmodeflags) are always registered. They report or change debug/editor state, but they are not themselves devmode-only verbs.
- [`SetTimerEx()`](#settimerex) is more specialized than the old public docs imply:
  - the first extra integer is used as the timer id and later shows up through [`GetSenderID()`](#getsenderid)
  - the next two integers become message params `0` and `1`
  - params `2` and `3` are zero in the current implementation
- [`GetCogByIndex()`](#getcogbyindex) looks up static-resource cogs by index rather than arbitrary current-world cogs.
- [`GetMaterialCel()`](#getmaterialcel) currently always pushes `-1` in the OpenJones3D implementation.
- [`SelectWeapon()`](#selectweapon), [`SelectWeaponWait()`](#selectweaponwait), [`DeselectWeaponWait()`](#deselectweaponwait), and [`SetWeaponModel()`](#setweaponmodel) are not devmode-gated in the current tree; the old debug-only checks were removed.
- [`DebugWaitForKey()`](#debugwaitforkey) returns immediately unless [`InEditor()`](#ineditor) is true.
- Signatures below use PascalCase for readability, even though host-function lookup in scripts is case-insensitive.

## Function Index

### Message And Cog Context

- [GetSenderRef](#getsenderref)
- [GetSenderType](#getsendertype)
- [GetSenderID](#getsenderid)
- [GetSourceType](#getsourcetype)
- [GetSourceRef](#getsourceref)
- [GetParam](#getparam)
- [SetParam](#setparam)
- [ReturnEx](#returnex)
- [GetSelfCog](#getselfcog)
- [GetMasterCog](#getmastercog)
- [SetMasterCog](#setmastercog)
- [GetCogByIndex](#getcogbyindex)
- [SendMessage](#sendmessage)
- [SendMessageEx](#sendmessageex)
- [SendTrigger](#sendtrigger)

### Time, World, And State Queries

- [GetSysDate](#getsysdate)
- [GetSysTime](#getsystime)
- [InEditor](#ineditor)
- [GetThingCount](#getthingcount)
- [GetGravity](#getgravity)
- [SetGravity](#setgravity)
- [GetLevelTime](#getleveltime)
- [GetGameTime](#getgametime)
- [GetFlexGameTime](#getflexgametime)
- [GetThingTemplateCount](#getthingtemplatecount)
- [SetFog](#setfog)
- [FindNewSector](#findnewsector)
- [FindNewSectorFromThing](#findnewsectorfromthing)
- [GetSithMode](#getsithmode)
- [GetDifficulty](#getdifficulty)
- [SetSubModeFlags](#setsubmodeflags)
- [GetSubModeFlags](#getsubmodeflags)
- [ClearSubModeFlags](#clearsubmodeflags)
- [SetDebugModeFlags](#setdebugmodeflags)
- [GetDebugModeFlags](#getdebugmodeflags)
- [ClearDebugModeFlags](#cleardebugmodeflags)
- [SetMapModeFlags](#setmapmodeflags)
- [GetMapModeFlags](#getmapmodeflags)
- [ClearMapModeFlags](#clearmapmodeflags)
- [GetPerformanceLevel](#getperformancelevel)
- [IsLevelName](#islevelname)
- [IsMulti](#ismulti)
- [IsServer](#isserver)
- [AutoSavegame](#autosavegame)

### Timing And Flow Control

- [Sleep](#sleep)
- [SetPulse](#setpulse)
- [SetTimer](#settimer)
- [SetTimerEx](#settimerex)
- [KillTimerEx](#killtimerex)
- [Reset](#reset)

### Resource Loading And Animation Helpers

- [MaterialAnim](#materialanim)
- [StopMaterialAnim](#stopmaterialanim)
- [StopAnim](#stopanim)
- [StopSurfaceAnim](#stopsurfaceanim)
- [GetSurfaceAnim](#getsurfaceanim)
- [SurfaceAnim](#surfaceanim)
- [GetKeyLen](#getkeylen)
- [LoadTemplate](#loadtemplate)
- [LoadKeyframe](#loadkeyframe)
- [LoadModel](#loadmodel)

### Printing And Debugging

- [EnablePrint](#enableprint)
- [IsPrintEnabled](#isprintenabled)
- [Print](#print)
- [PrintInt](#printint)
- [PrintFlex](#printflex)
- [PrintVector](#printvector)
- [PrintHex](#printhex)
- [DebugPrint](#debugprint)
- [DebugFlex](#debugflex)
- [DebugInt](#debugint)
- [DebugVector](#debugvector)
- [DebugLocalSymbols](#debuglocalsymbols)
- [DebugWaitForKey](#debugwaitforkey)

### Vector And Math Helpers

- [VectorAdd](#vectoradd)
- [VectorSub](#vectorsub)
- [VectorDot](#vectordot)
- [VectorCross](#vectorcross)
- [VectorSet](#vectorset)
- [VectorLen](#vectorlen)
- [VectorScale](#vectorscale)
- [VectorDist](#vectordist)
- [VectorX](#vectorx)
- [VectorY](#vectory)
- [VectorZ](#vectorz)
- [VectorNorm](#vectornorm)
- [VectorEqual](#vectorequal)
- [VectorRotate](#vectorrotate)
- [VectorTransformToOrient](#vectortransformtoorient)
- [BitSet](#bitset)
- [BitTest](#bittest)
- [BitClear](#bitclear)
- [Rand](#rand)
- [RandBetween](#randbetween)
- [RandVec](#randvec)
- [Round](#round)
- [Truncate](#truncate)
- [Abs](#abs)
- [Pow](#pow)
- [Sin](#sin)
- [Cos](#cos)
- [ArcTan](#arctan)

### Weapon And Combat Helpers

- [FireProjectile](#fireprojectile)
- [ActivateWeapon](#activateweapon)
- [DeactivateWeapon](#deactivateweapon)
- [DeactivateCurWeapon](#deactivatecurweapon)
- [SetMountWait](#setmountwait)
- [SetFireWait](#setfirewait)
- [SetAimWait](#setaimwait)
- [SelectWeapon](#selectweapon)
- [SelectWeaponWait](#selectweaponwait)
- [DeselectWeapon](#deselectweapon)
- [DeselectWeaponWait](#deselectweaponwait)
- [SetCurWeapon](#setcurweapon)
- [GetCurWeapon](#getcurweapon)
- [SetWeaponModel](#setweaponmodel)
- [ResetWeaponModel](#resetweaponmodel)
- [LoadHolsterModel](#loadholstermodel)
- [SetHolsterModel](#setholstermodel)
- [ResetHolsterModel](#resetholstermodel)
- [GetLastPistol](#getlastpistol)
- [GetLastRifle](#getlastrifle)
- [CopyPlayerHolsters](#copyplayerholsters)
- [SetInvFlags](#setinvflags)

### Camera, Material, And World Helpers

- [GetMaterialCel](#getmaterialcel)
- [SetMaterialCel](#setmaterialcel)
- [SetCameraFocus](#setcamerafocus)
- [GetPrimaryFocus](#getprimaryfocus)
- [GetSecondaryFocus](#getsecondaryfocus)
- [SetCurrentCamera](#setcurrentcamera)
- [GetCurrentCamera](#getcurrentcamera)
- [CycleCamera](#cyclecamera)
- [SetPOVShake](#setpovshake)
- [SetCameraStateFlags](#setcamerastateflags)
- [GetCameraStateFlags](#getcamerastateflags)
- [SetCameraSecondaryFocus](#setcamerasecondaryfocus)

### Heap, Hints, And Misc Compatibility Verbs

- [HeapNew](#heapnew)
- [HeapSet](#heapset)
- [HeapGet](#heapget)
- [HeapFree](#heapfree)
- [GetHintSolved](#gethintsolved)
- [SetHintSolved](#sethintsolved)
- [SetHintUnsolved](#sethintunsolved)

## Function Reference

### Message And Cog Context


#### GetSenderRef

```C
GetSenderRef() -> int
```

Returns the current message sender reference.

Returns:
- Sender reference value from the active message context.


#### GetSenderType

```C
GetSenderType() -> int
```

Returns the current message sender type.

Returns:
- Sender symbol-reference type from the active message context. See [COG Symbol Reference Types](Types-And-Modes.md#cog-symbol-reference-types).


#### GetSenderID

```C
GetSenderID() -> int
```

Returns the current message link id.

Returns:
- Current link id from the active message context.


#### GetSourceType

```C
GetSourceType() -> int
```

Returns the current message source type.

Returns:
- Source symbol-reference type from the active message context. See [COG Symbol Reference Types](Types-And-Modes.md#cog-symbol-reference-types).


#### GetSourceRef

```C
GetSourceRef() -> int
```

Returns the current message source reference.

Returns:
- Source reference value from the active message context.


#### GetParam

```C
GetParam(int idx) -> int
```

Returns message parameter `idx` from the current send context.

Parameters:
- `idx`: Zero-based message parameter slot to read. Valid slots are `0..3`.

Returns:
- Current message parameter at index `idx`, or `-9999` when `idx` is out of range.


#### SetParam

```C
SetParam(int num, int val)
```

Overwrites message parameter `num` in the current send context.

Parameters:
- `num`: Zero-based message parameter slot to overwrite. Valid slots are `0..3`.
- `val`: New integer value to store in that parameter slot.

Notes:
- Out-of-range parameter indices are ignored.


#### ReturnEx

```C
ReturnEx(int returnValue)
```

Sets the explicit return value for the current message handler.

Parameters:
- `returnValue`: Explicit integer return value for the current message handler.


#### GetSelfCog

```C
GetSelfCog() -> Cog
```

Returns the index/reference of the currently executing cog.

Returns:
- Current cog reference.


#### GetMasterCog

```C
GetMasterCog() -> Cog
```

Returns the global master cog reference.

Returns:
- Master cog reference, or `-1` when no master cog is set.


#### SetMasterCog

```C
SetMasterCog(Cog masterCog)
```

Sets the global master cog reference.

Parameters:
- `masterCog`: Cog to store as the global master cog.


#### GetCogByIndex

```C
GetCogByIndex(int idx) -> Cog
```

Returns a cog from the static-resource world by index.

Parameters:
- `idx`: Static-resource cog index. The wrapper forces this index through `SITHWORLD_STATICINDEX()` before lookup.

Returns:
- Cog reference from the static-resource world, or `-1` when the lookup fails.

Notes:
- This verb looks up cogs in the static resource set rather than the current level world.
- In practice this means the lookup targets the static `Jones3DStatic` resources such as `jones3dStatic.cnd` / `jones3dStatic.ndy`.


#### SendMessage

```C
SendMessage(Cog dstCog, int msgType)
```

Sends a message to another cog without explicit integer parameters.

Parameters:
- `dstCog`: Destination cog that should receive the message.
- `msgType`: Message type id to send.

Notes:
- The wrapper sends the current cog as the sender (`SITHCOG_SYM_REF_COG`, current cog index).
- The current source type/reference are forwarded from the active message context.


#### SendMessageEx

```C
SendMessageEx(Cog dstCog, int msg, int param0, int param1, int param2, int param3) -> int
```

Sends a message with four explicit integer parameters to another cog.

Parameters:
- `dstCog`: Destination cog that should receive the message.
- `msg`: Message type id to send.
- `param0`: First integer message parameter.
- `param1`: Second integer message parameter.
- `param2`: Third integer message parameter.
- `param3`: Fourth integer message parameter.

Returns:
- Return value from the destination cog handler, or `-9999` when no explicit return value is produced.

Notes:
- The wrapper sends the current cog as the sender and forwards the current source type/reference.


#### SendTrigger

```C
SendTrigger(Thing thing, int srcIdx, int param0, int param1, int param2, int param3)
```

Sends a network-style `trigger` message through the player messaging path.

Parameters:
- `thing`: Target player thing. When null or invalid for direct player delivery, the trigger is broadcast.
- `srcIdx`: Integer source reference value attached to the outgoing trigger message.
- `param0`: First integer trigger parameter.
- `param1`: Second integer trigger parameter.
- `param2`: Third integer trigger parameter.
- `param3`: Fourth integer trigger parameter.

Notes:
- The wrapper uses the local player thing as the sender reference for the outgoing trigger.

### Time, World, And State Queries


#### GetSysDate

```C
GetSysDate() -> Vector
```

Returns the local system date.

Returns:
- Vector `<year, month, day>`.


#### GetSysTime

```C
GetSysTime() -> Vector
```

Returns the local system time.

Returns:
- Vector `<hour, minute, second>`.


#### InEditor

```C
InEditor() -> int
```

Returns whether the engine is currently flagged as running in editor/debug-editor mode.

Returns:
- `1` when [`SITHDEBUG_INEDITOR`](Flags.md#debug-mode-flags) is set, otherwise `0`.


#### GetThingCount

```C
GetThingCount() -> int
```

Returns the number of thing slots currently in the world.

Returns:
- Total thing count in the current world.


#### GetGravity

```C
GetGravity() -> float
```

Returns the world's current gravity value.

Returns:
- Current world gravity.


#### SetGravity

```C
SetGravity(float gravity)
```

Sets the world's gravity value.

Parameters:
- `gravity`: New world gravity value.


#### GetLevelTime

```C
GetLevelTime() -> float
```

Returns elapsed level time in seconds.

Returns:
- Elapsed level time in seconds.


#### GetGameTime

```C
GetGameTime() -> int
```

Returns game time in milliseconds.

Returns:
- Current game time in milliseconds.


#### GetFlexGameTime

```C
GetFlexGameTime() -> float
```

Returns game time in seconds as a floating-point value.

Returns:
- Current game time in seconds.


#### GetThingTemplateCount

```C
GetThingTemplateCount(Template template) -> int
```

Returns how many live things in the current world were created from `template`.

Parameters:
- `template`: Thing template to count.

Returns:
- Number of non-corpse things in the current world whose template pointer matches `template`.


#### SetFog

```C
SetFog(int bEnable, Vector vecColor, float start, float end)
```

Sets the current world fog state.

Parameters:
- `bEnable`: Non-zero to enable the feature, zero to disable it.
- `vecColor`: Fog RGB color.
- `start`: Fog start depth.
- `end`: Fog end depth.

Notes:
- The wrapper marks the world with `SITH_WORLD_STATE_UPDATE_FOG` after updating these values.


#### FindNewSector

```C
FindNewSector(Vector startPos, Sector sector, Vector endPos) -> Sector
```

Finds the sector reached when moving from `startPos` toward `endPos`, beginning the search in `sector`.

Parameters:
- `startPos`: Start position for the search.
- `sector`: Sector to start the search from.
- `endPos`: Destination position to test.

Returns:
- Sector reference containing the traced destination, or `-1` when no sector is found.


#### FindNewSectorFromThing

```C
FindNewSectorFromThing(Thing thing, Vector endPos) -> Sector
```

Finds the sector reached when moving from `thing`'s current position toward `endPos`.

Parameters:
- `thing`: Thing whose current sector and position should seed the search.
- `endPos`: Destination position to test.

Returns:
- Sector reference containing the traced destination, or `-1` when no sector is found.


#### GetSithMode

```C
GetSithMode() -> int
```

Returns the current master game mode.

Returns:
- Current `sithMain_g_sith_mode.masterMode` value. See [Master Game Modes](Types-And-Modes.md#master-game-modes).


#### GetDifficulty

```C
GetDifficulty() -> int
```

Returns the current game difficulty setting.

Returns:
- Current difficulty value. See [Difficulty Values](Types-And-Modes.md#difficulty-values).


#### SetSubModeFlags

```C
SetSubModeFlags(int flags)
```

Sets bits in the global game submode flags.

Parameters:
- `flags`: Game-submode bitmask to OR into `sithMain_g_sith_mode.subModeFlags`. See [Game Submode Flags](Flags.md#game-submode-flags).


#### GetSubModeFlags

```C
GetSubModeFlags() -> int
```

Returns the global game submode flags.

Returns:
- Current `sithMain_g_sith_mode.subModeFlags` bitmask. See [Game Submode Flags](Flags.md#game-submode-flags).


#### ClearSubModeFlags

```C
ClearSubModeFlags(int flags)
```

Clears bits from the global game submode flags.

Parameters:
- `flags`: Game-submode bitmask to clear from `sithMain_g_sith_mode.subModeFlags`. See [Game Submode Flags](Flags.md#game-submode-flags).


#### SetDebugModeFlags

```C
SetDebugModeFlags(int flags)
```

Sets bits in the global debug-mode flags.

Parameters:
- `flags`: Debug-mode bitmask to OR into `sithMain_g_sith_mode.debugModeFlags`. See [Debug Mode Flags](Flags.md#debug-mode-flags).


#### GetDebugModeFlags

```C
GetDebugModeFlags() -> int
```

Returns the global debug-mode flags.

Returns:
- Current `sithMain_g_sith_mode.debugModeFlags` bitmask. See [Debug Mode Flags](Flags.md#debug-mode-flags).


#### ClearDebugModeFlags

```C
ClearDebugModeFlags(int flags)
```

Clears bits from the global debug-mode flags.

Parameters:
- `flags`: Debug-mode bitmask to clear from `sithMain_g_sith_mode.debugModeFlags`. See [Debug Mode Flags](Flags.md#debug-mode-flags).


#### SetMapModeFlags

```C
SetMapModeFlags(int flags)
```

Sets bits in the global map-mode flags.

Parameters:
- `flags`: Map-mode bitmask to OR into `sithMain_g_sith_mode.mapModeFlags`. See [Map Mode Flags](Flags.md#map-mode-flags).


#### GetMapModeFlags

```C
GetMapModeFlags() -> int
```

Returns the global map-mode flags.

Returns:
- Current `sithMain_g_sith_mode.mapModeFlags` bitmask. See [Map Mode Flags](Flags.md#map-mode-flags).


#### ClearMapModeFlags

```C
ClearMapModeFlags(int flags)
```

Clears bits from the global map-mode flags.

Parameters:
- `flags`: Map-mode bitmask to clear from `sithMain_g_sith_mode.mapModeFlags`. See [Map Mode Flags](Flags.md#map-mode-flags).


#### GetPerformanceLevel

```C
GetPerformanceLevel() -> int
```

Returns the current engine performance level.

Returns:
- Value returned by `sithGetPerformanceLevel()`.


#### IsLevelName

```C
IsLevelName(string name) -> int
```

Checks whether the current world name matches `name` case-insensitively.

Parameters:
- `name`: Name string used by the operation.

Returns:
- Non-zero when the condition is true, otherwise `0`.


#### IsMulti

```C
IsMulti() -> int
```

Returns whether a multiplayer game session is currently active.

Returns:
- Non-zero when the condition is true, otherwise `0`.


#### IsServer

```C
IsServer() -> int
```

Returns whether the local instance is the multiplayer host/server.

Returns:
- Non-zero when the condition is true, otherwise `0`.


#### AutoSavegame

```C
AutoSavegame()
```

Performs the requested savegame operation.

### Timing And Flow Control


#### Sleep

```C
Sleep(float secWait)
```

Blocks the current COG for `secWait` seconds.

Parameters:
- `secWait`: Delay in seconds before the cog wakes up again. Non-positive values are clamped to `0.1`.


#### SetPulse

```C
SetPulse(float interval)
```

Enables or disables repeated `pulse` messages for the current COG.

Parameters:
- `interval`: Pulse interval in seconds. Values less than or equal to `0` disable pulsing.

Notes:
- When the current cog has the debug flag set, the wrapper prints the new pulse state to the console.

#### SetTimer

```C
SetTimer(float when)
```

Enables or disables the one-shot `timer` message for the current COG.

Parameters:
- `when`: Delay in seconds before the `timer` message fires. Values less than or equal to `0` cancel the timer.

Notes:
- When the current cog has the debug flag set, the wrapper prints the new timer state to the console.

#### SetTimerEx

```C
SetTimerEx(float seconds, int timerId, int param0, int param1)
```

Queues an extended `timer` event for the current cog with an explicit timer id and extra parameters.

Parameters:
- `seconds`: Delay in seconds before the timer event is delivered.
- `timerId`: Timer id that later appears through [`GetSenderID()`](#getsenderid) when the timer event is delivered.
- `param0`: First extra timer parameter delivered with the event.
- `param1`: Second extra timer parameter delivered with the event.


#### KillTimerEx

```C
KillTimerEx(int timerId)
```

Cancels queued extended timer events for the current COG whose timer id matches `timerId`.

Parameters:
- `timerId`: Timer id previously passed as the first integer parameter to [`SetTimerEx()`](#settimerex).

#### Reset

```C
Reset()
```

Clears the current COG call depth.

Notes:
- The wrapper only resets `pCog->callDepth`; it does not reset symbols, timers, or script state.

#### MaterialAnim

```C
MaterialAnim(Material mat, float fps, int flags) -> int
```

Starts a material cel animation on `mat`.

Parameters:
- `mat`: Material whose cels should be animated.
- `fps`: Playback speed in frames per second. Values less than or equal to `0` are forced to `15.0` by the wrapper.
- `flags`: Animation bitmask passed through to the animation system. See [Animation Flags](Flags.md#animation-flags).

Returns:
- Animation id, or `-1` when the animation could not be started.

#### StopMaterialAnim

```C
StopMaterialAnim(Material material)
```

Stops the current animation playing on `material`.

Parameters:
- `material`: Material whose current animation should be stopped.


#### StopAnim

```C
StopAnim(int animID)
```

Stops the animation identified by `animID`.

Parameters:
- `animID`: Animation id returned by an earlier animation-start call.


#### StopSurfaceAnim

```C
StopSurfaceAnim(Surface surf)
```

Stops the current animation playing on `surf`.

Parameters:
- `surf`: Surface whose current animation should be stopped.


#### GetSurfaceAnim

```C
GetSurfaceAnim(Surface surf) -> int
```

Returns the current animation id associated with `surf`.

Parameters:
- `surf`: Surface for which to retrieve the current animation id.

Returns:
- Current animation id for `surf`, or `-1` when no animation is active.


#### SurfaceAnim

```C
SurfaceAnim(Surface surf, float speed, int flags) -> int
```

Starts a surface animation on `surf`.

Parameters:
- `surf`: Surface whose animation should be started.
- `speed`: Playback speed in frames per second. Values less than or equal to `0` are forced to `15.0`.
- `flags`: Animation bitmask passed through to the surface-animation system. See [Animation Flags](Flags.md#animation-flags).

Returns:
- Animation id, or `-1` when the animation could not be started.


#### GetKeyLen

```C
GetKeyLen(Keyframe keyframe) -> float
```

Returns the current len of the `keyframe`.

Parameters:
- `keyframe`: Keyframe for which to retrieve the current len.

Returns:
- Current len of the `keyframe`.


#### LoadTemplate

```C
LoadTemplate(string templateName) -> Template
```

Loads the requested template resource.

Parameters:
- `templateName`: Template resource name to load.

Returns:
- Template reference returned by the engine.


#### LoadKeyframe

```C
LoadKeyframe(string keyFilename) -> Keyframe
```

Loads the requested keyframe resource.

Parameters:
- `keyFilename`: Keyframe resource name to load.

Returns:
- Keyframe reference returned by the engine.


#### LoadModel

```C
LoadModel(string filename) -> Model
```

Loads the requested model resource.

Parameters:
- `filename`: Filename string used by the operation.

Returns:
- Model reference returned by the engine.

### Printing And Debugging


#### EnablePrint

```C
EnablePrint(int bEnable) -> int
```

Enables or disables the global COG print helpers.

Parameters:
- `bEnable`: Non-zero to enable the feature, zero to disable it.

Returns:
- Previous print-enabled state: `1` if printing was enabled before the call, otherwise `0`.


#### IsPrintEnabled

```C
IsPrintEnabled() -> int
```

Returns whether the global COG print helpers are currently enabled.

Returns:
- Non-zero when the condition is true, otherwise `0`.


#### Print

```C
Print(string str)
```

Prints the supplied value to the console output.

Parameters:
- `str`: String to print. Nothing is printed when the global print toggle is disabled.


#### PrintInt

```C
PrintInt(int val)
```

Prints the supplied int to the console output.

Parameters:
- `val`: Integer value to print. Nothing is printed when the global print toggle is disabled.


#### PrintFlex

```C
PrintFlex(float value)
```

Prints the supplied flex to the console output.

Parameters:
- `value`: Floating-point value to print. Nothing is printed when the global print toggle is disabled.


#### PrintVector

```C
PrintVector(Vector vec)
```

Prints the supplied vector to the console output.

Parameters:
- `vec`: Vector to print. Nothing is printed when the global print toggle is disabled.


#### PrintHex

```C
PrintHex(int value)
```

Prints the supplied hex to the console output.

Parameters:
- `value`: Integer value to print in hexadecimal. Nothing is printed when the global print toggle is disabled.


#### DebugPrint

```C
DebugPrint(string str)
```

Writes the supplied string to the debug log.

Parameters:
- `str`: String to write to the debug log.


#### DebugFlex

```C
DebugFlex(string str, float val) -> float
```

Writes the supplied label and float to the debug log, then returns the float unchanged.

Parameters:
- `str`: Label string to print before the value.
- `val`: Floating-point value to print and return unchanged.

Returns:
- The same floating-point value passed in `val`.


#### DebugInt

```C
DebugInt(string str, int val) -> int
```

Writes the supplied label and int to the debug log, then returns the int unchanged.

Parameters:
- `str`: Label string to print before the value.
- `val`: Integer value to print and return unchanged.

Returns:
- The same integer value passed in `val`.


#### DebugVector

```C
DebugVector(string str, Vector vec) -> Vector
```

Writes the supplied label and vector to the debug log, then returns the vector unchanged.

Parameters:
- `str`: Label string to print before the value.
- `vec`: Vector value to print and return unchanged.

Returns:
- The same vector value passed in `vec`.


#### DebugLocalSymbols

```C
DebugLocalSymbols(string headerText) -> int
```

Logs the current cog's non-local symbol values for debugging.

Parameters:
- `headerText`: Header text to print before the local-symbol dump.

Returns:
- `1`.


#### DebugWaitForKey

```C
DebugWaitForKey()
```

Blocks until the debug key is pressed, but only while the engine is flagged as running in editor/debug-editor mode.

Notes:
- Outside [`SITHDEBUG_INEDITOR`](Flags.md#debug-mode-flags), this verb returns immediately.

### Vector And Math Helpers


#### VectorAdd

```C
VectorAdd(Vector a, Vector b) -> Vector
```

Returns the vector sum `a + b`.

Parameters:
- `a`: Left-hand vector.
- `b`: Right-hand vector.

Returns:
- Resulting vector sum.

#### VectorSub

```C
VectorSub(Vector a, Vector b) -> Vector
```

Returns the vector difference `a - b`.

Parameters:
- `a`: Left-hand vector.
- `b`: Right-hand vector.

Returns:
- Resulting vector difference.

#### VectorDot

```C
VectorDot(Vector a, Vector b) -> float
```

Returns the dot product of `a` and `b`.

Parameters:
- `a`: First vector.
- `b`: Second vector.

Returns:
- Dot product of the two vectors.

#### VectorCross

```C
VectorCross(Vector a, Vector b) -> Vector
```

Returns the cross product of `a` and `b`.

Parameters:
- `a`: First vector.
- `b`: Second vector.

Returns:
- Cross-product vector.

Notes:
- The source notes that debug and release builds historically disagreed on operand order, but the current wrapper calls `rdVector_Cross3(&c, &a, &b)`.

#### VectorSet

```C
VectorSet(float x, float y, float z) -> Vector
```

Builds and returns a vector from its `x`, `y`, and `z` components.

Parameters:
- `x`: X component.
- `y`: Y component.
- `z`: Z component.

Returns:
- Constructed vector.

#### VectorLen

```C
VectorLen(Vector vec) -> float
```

Returns the length of `vec`.

Parameters:
- `vec`: Vector to measure.

Returns:
- Vector length.

#### VectorScale

```C
VectorScale(Vector a, float scalar) -> Vector
```

Returns `a` scaled by `scalar`.

Parameters:
- `a`: Vector to scale.
- `scalar`: Scalar multiplier.

Returns:
- Scaled vector.

#### VectorDist

```C
VectorDist(Vector a, Vector b) -> float
```

Returns the distance between `a` and `b`.

Parameters:
- `a`: First point/vector.
- `b`: Second point/vector.

Returns:
- Distance between the two vectors.

#### VectorX

```C
VectorX(Vector vec) -> float
```

Returns the `x` component of `vec`.

Parameters:
- `vec`: Vector to inspect.

Returns:
- X component.

#### VectorY

```C
VectorY(Vector vec) -> float
```

Returns the `y` component of `vec`.

Parameters:
- `vec`: Vector to inspect.

Returns:
- Y component.

#### VectorZ

```C
VectorZ(Vector vec) -> float
```

Returns the `z` component of `vec`.

Parameters:
- `vec`: Vector to inspect.

Returns:
- Z component.

#### VectorNorm

```C
VectorNorm(Vector vec) -> Vector
```

Returns a normalized copy of `vec`.

Parameters:
- `vec`: Vector to normalize.

Returns:
- Normalized vector.

#### VectorEqual

```C
VectorEqual(Vector a, Vector b) -> int
```

Returns whether `a` and `b` are exactly equal component by component.

Parameters:
- `a`: First vector.
- `b`: Second vector.

Returns:
- `1` when all three components match exactly, otherwise `0`.

#### VectorRotate

```C
VectorRotate(Vector vec, Vector pyr) -> Vector
```

Returns `vec` rotated by the supplied `pyr` angles.

Parameters:
- `vec`: Vector to rotate.
- `pyr`: Pitch-yaw-roll angles in degrees.

Returns:
- Rotated vector.

#### VectorTransformToOrient

```C
VectorTransformToOrient(Thing thing, Vector vec) -> Vector
```

Transforms `vec` by `thing`'s orientation and returns the rotated vector.

Parameters:
- `thing`: Thing whose orientation should be used.
- `vec`: Local-space vector to transform.

Returns:
- Vector transformed into the thing's oriented space.

#### BitSet

```C
BitSet(int flags, int mask) -> int
```

Performs the requested set operation.

Parameters:
- `flags`: Bitmask of flags used by the operation.
- `mask`: Integer value used by the operation.

Returns:
- Integer result returned by the engine.


#### BitTest

```C
BitTest(int flags, int mask) -> int
```

Performs the requested test operation.

Parameters:
- `flags`: Bitmask of flags used by the operation.
- `mask`: Integer value used by the operation.

Returns:
- Integer result returned by the engine.


#### BitClear

```C
BitClear(int flags, int mask) -> int
```

Performs the requested clear operation.

Parameters:
- `flags`: Bitmask of flags used by the operation.
- `mask`: Integer value used by the operation.

Returns:
- Integer result returned by the engine.


#### Rand

```C
Rand() -> float
```

Performs the requested value operation.

Returns:
- Floating-point result returned by the engine.


#### RandBetween

```C
RandBetween(int min, int max) -> int
```

Performs the requested between operation.

Parameters:
- `min`: Integer value used by the operation.
- `max`: Integer value used by the operation.

Returns:
- Integer result returned by the engine.


#### RandVec

```C
RandVec() -> Vector
```

Performs the requested vec operation.

Returns:
- Vector result returned by the engine.


#### Round

```C
Round(float val) -> float
```

Performs the requested value operation.

Parameters:
- `val`: Floating-point value used by the operation.

Returns:
- Floating-point result returned by the engine.


#### Truncate

```C
Truncate(float val) -> float
```

Performs the requested value operation.

Parameters:
- `val`: Floating-point value used by the operation.

Returns:
- Floating-point result returned by the engine.


#### Abs

```C
Abs(float val) -> float
```

Performs the requested value operation.

Parameters:
- `val`: Floating-point value used by the operation.

Returns:
- Floating-point result returned by the engine.


#### Pow

```C
Pow(float base, float exp) -> float
```

Performs the requested value operation.

Parameters:
- `base`: Floating-point value used by the operation.
- `exp`: Floating-point value used by the operation.

Returns:
- Floating-point result returned by the engine.


#### Sin

```C
Sin(float angle) -> float
```

Performs the requested value operation.

Parameters:
- `angle`: Floating-point value used by the operation.

Returns:
- Floating-point result returned by the engine.


#### Cos

```C
Cos(float angle) -> float
```

Performs the requested value operation.

Parameters:
- `angle`: Floating-point value used by the operation.

Returns:
- Floating-point result returned by the engine.


#### ArcTan

```C
ArcTan(float x, float y) -> float
```

Performs the requested tan operation.

Parameters:
- `x`: Floating-point value used by the operation.
- `y`: Floating-point value used by the operation.

Returns:
- Floating-point result returned by the engine.

### Weapon And Combat Helpers


#### FireProjectile

```C
FireProjectile(Thing shooter, Template projectileTemplate, Sound hFireSnd, int submode, Vector fireOffset, Vector fireError, float extra, int flags, float autoAimFovX, float autoAimFovZ) -> Thing
```

Creates and fires a projectile from `shooter` using `projectileTemplate`.

Parameters:
- `shooter`: Thing that fires the projectile.
- `projectileTemplate`: Projectile template to instantiate.
- `hFireSnd`: Optional fire sound to play alongside the shot.
- `submode`: Puppet submode associated with the firing action. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).
- `fireOffset`: Spawn offset for the projectile relative to the shooter.
- `fireError`: Aim/spread error vector applied to the shot.
- `extra`: Extra scaling value used by several flag-controlled projectile behaviors.
- `flags`: Projectile-fire behavior bitmask. See [Projectile Fire Flags](Flags.md#projectile-fire-flags).
- `autoAimFovX`: Horizontal auto-aim field of view used by player auto-aim logic.
- `autoAimFovZ`: Vertical auto-aim field of view used by player auto-aim logic.

Returns:
- Spawned projectile thing, or `-1` on failure.

Notes:
- Several flags change how `extra`, auto-aim, muzzle origin, and weapon-fire FX are applied; check the wrapper and `sithWeapon_FireProjectile()` call path when documenting flag-level behavior in more detail.

#### ActivateWeapon

```C
ActivateWeapon(Thing thing, float timeToWait)
```

Starts weapon activation on `thing` and passes the current cog as the callback owner.

Parameters:
- `thing`: Actor or player thing whose weapon should be activated.
- `timeToWait`: Delay parameter passed to the weapon system. Values below `0` are rejected by the wrapper.

#### DeactivateWeapon

```C
DeactivateWeapon(Thing thing) -> float
```

Deactivates the active weapon on `thing`.

Parameters:
- `thing`: Thing whose active weapon should be deactivated.

Returns:
- Time in seconds returned by the weapon system for the deactivation, or `-1.0` on invalid input.

#### DeactivateCurWeapon

```C
DeactivateCurWeapon(Thing thing)
```

Deactivates the current weapon on `thing` without returning a delay value.

Parameters:
- `thing`: Thing whose current weapon should be deactivated.

#### SetMountWait

```C
SetMountWait(Thing thing, float timeToWait)
```

Sets the global mount wait used by the player weapon system.

Parameters:
- `thing`: Player thing used for validation by the wrapper.
- `timeToWait`: Mount wait in seconds. The wrapper asserts values greater than or equal to `-1.0`.

Notes:
- The implementation ultimately calls `sithWeapon_SetMountWait(timeToWait)`; the wait is not stored per-thing.

#### SetFireWait

```C
SetFireWait(Thing thing, float waitTime)
```

Sets the fire-wait timer on `thing`.

Parameters:
- `thing`: Actor or player thing whose fire wait should be updated.
- `waitTime`: Fire wait in seconds. Values below `-1.0` are rejected by the wrapper.

#### SetAimWait

```C
SetAimWait(Thing thing, float timeToWait)
```

Sets the aim-wait timer on `thing`.

Parameters:
- `thing`: Actor or player thing whose aim wait should be updated.
- `timeToWait`: Aim wait in seconds. The wrapper asserts values greater than or equal to `-1.0`.

#### SelectWeapon

```C
SelectWeapon(Thing thing, int weaponID) -> int
```

Requests selection of `weaponID` on `thing`.

Parameters:
- `thing`: Actor or player thing whose weapon should be selected.
- `weaponID`: Inventory weapon id to select.

Returns:
- `1` when the weapon-selection request succeeded, otherwise `0`.

#### SelectWeaponWait

```C
SelectWeaponWait(Thing thing, int typeID) -> int
```

Requests selection of `typeID` on `thing` and, when a transition is needed, blocks the current COG until it finishes.

Parameters:
- `thing`: Player thing whose weapon should be selected.
- `typeID`: Inventory weapon id to select.

Returns:
- Previously active weapon id when the request is accepted, `0` when no weapon was active and no wait is needed, or `-1` on invalid input.

Notes:
- On a deferred select, the wrapper sets the cog status to `SITHCOG_STATUS_WAITING_ACTOR_WEAPON_SELECT_FINISH`.

#### DeselectWeapon

```C
DeselectWeapon(Thing thing) -> int
```

Requests weapon deselection on `thing`.

Parameters:
- `thing`: Thing whose current weapon should be deselected.

Returns:
- Current weapon id before deselection, or `-1` on invalid input.

#### DeselectWeaponWait

```C
DeselectWeaponWait(Thing thing) -> int
```

Requests weapon deselection on `thing` and blocks the current COG until the deselection finishes.

Parameters:
- `thing`: Player thing whose current weapon should be deselected.

Returns:
- Weapon id being deselected, `0` when there is nothing to deselect, or `-1` on invalid input.

Notes:
- On a deferred deselect, the wrapper sets the cog status to `SITHCOG_STATUS_WAITING_ACTOR_WEAPON_DESELECT_FINISH`.

#### SetCurWeapon

```C
SetCurWeapon(Thing thing, int weaponID)
```

Sets the current weapon of the `thing`.

Parameters:
- `thing`: Thing whose current weapon to set.
- `weaponID`: Weapon id used by the operation.


#### GetCurWeapon

```C
GetCurWeapon(Thing thing) -> int
```

Returns the currently selected weapon for `thing`.

Parameters:
- `thing`: Player thing for which to retrieve the currently selected weapon.

Returns:
- Weapon id of the currently selected weapon.


#### SetWeaponModel

```C
SetWeaponModel(Thing thing, int typeId)
```

Sets the model of the `weapon`.

Parameters:
- `thing`: Thing reference used by the operation.
- `typeId`: Type or bin id used by the operation.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ResetWeaponModel

```C
ResetWeaponModel(Thing thing)
```

Performs the requested weapon model operation.

Parameters:
- `thing`: Thing reference used by the operation.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### LoadHolsterModel

```C
LoadHolsterModel(int holsterType, string modelFilename) -> int
```

Loads the requested holster model resource.

Parameters:
- `holsterType`: Holster type used by the operation.
- `modelFilename`: Model filename to load.

Returns:
- Integer result returned by the engine.


#### SetHolsterModel

```C
SetHolsterModel(Thing thing, int weaponId, int meshNum)
```

Sets the holster model of the `thing`.

Parameters:
- `thing`: Thing whose holster model to set.
- `weaponId`: Integer value used by the operation.
- `meshNum`: Mesh index used by the model.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ResetHolsterModel

```C
ResetHolsterModel(Thing thing, int holsterNum)
```

Performs the requested holster model operation.

Parameters:
- `thing`: Thing used by the operation.
- `holsterNum`: Holster slot number used by the operation.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetLastPistol

```C
GetLastPistol(Thing thing) -> int
```

Returns the current last pistol of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current last pistol.

Returns:
- Current last pistol of the `thing`.

Notes:
- Requires a player thing with player data.


#### GetLastRifle

```C
GetLastRifle(Thing thing) -> int
```

Returns the current last rifle of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current last rifle.

Returns:
- Current last rifle of the `thing`.

Notes:
- Requires a player thing with player data.


#### CopyPlayerHolsters

```C
CopyPlayerHolsters(Thing sourceThing, Thing destThing)
```

Copies the requested player holsters from one runtime object to another.

Parameters:
- `sourceThing`: Thing reference used by the operation.
- `destThing`: Thing reference used by the operation.


#### SetInvFlags

```C
SetInvFlags(Thing thing, int bin, int flags)
```

Sets inventory flags on inventory bin `bin` for the player `thing`.

Parameters:
- `thing`: Player thing whose inventory flags should be updated.
- `bin`: Inventory bin id whose flags should be changed.
- `flags`: Inventory-type bitmask to apply to that bin. See [Inventory Type Flags](Flags.md#inventory-type-flags).

### Camera, Material, And World Helpers


#### GetMaterialCel

```C
GetMaterialCel(Material material) -> int
```

Returns the current material cel.

Parameters:
- `material`: Material whose current cel would be queried.

Returns:
- Always `-1` in the current implementation.

Notes:
- The current wrapper pops the material argument but does not return the runtime cel value yet.


#### SetMaterialCel

```C
SetMaterialCel(Material mat, int celNum) -> int
```

Sets the current cel of a material.

Parameters:
- `mat`: Material whose current cel should be changed.
- `celNum`: New material cel index. The current implementation accepts values in the range `0 .. numCels - 1`.

Returns:
- Previous material cel index, or `-1` when the material/cel is invalid.


### Camera Number Reference

Valid camera numbers range from `0` to `6`. The highest valid `camNum` is `6`.
Technically, this number is the index into the engine's fixed internal list of predefined system cameras.

| Camera Number | Type | Meaning |
| --- | --- | --- |
| `0` | Internal | First-person camera. |
| `1` | External | Third-person follow camera. |
| `2` | Cinematic | Cutscene / scripted camera. |
| `3` | Unnamed | Valid index with no named built-in camera role established by the current startup path. |
| `4` | Idle | Idle camera. |
| `5` | Unknown helper | Built-in helper camera of currently unknown purpose. |
| `6` | Orbital | Orbital camera. |

#### SetCameraFocus

```C
SetCameraFocus(int camNum, Thing thing)
```

Sets the primary focus of one built-in engine camera.

Parameters:
- `camNum`: Camera number to update. See [Camera Number Reference](#camera-number-reference). Valid values are `0..6`.
- `thing`: Thing to assign as the camera's primary focus.

Notes:
- When used on the cinema camera while position interpolation is active, the wrapper snaps the camera position to the previous primary focus before switching focus.


#### GetPrimaryFocus

```C
GetPrimaryFocus(int camNum) -> Thing
```

Returns the current primary focus thing for one built-in engine camera.

Parameters:
- `camNum`: Camera number to query. See [Camera Number Reference](#camera-number-reference). Valid values are `0..6`.

Returns:
- Primary focus thing reference, or `-1` when the camera has no primary focus.


#### GetSecondaryFocus

```C
GetSecondaryFocus(int camNum) -> Thing
```

Returns the current secondary focus thing for one built-in engine camera.

Parameters:
- `camNum`: Camera number to query. See [Camera Number Reference](#camera-number-reference). Valid values are `0..6`.

Returns:
- Secondary focus thing reference, or `-1` when the camera has no secondary focus.


#### SetCurrentCamera

```C
SetCurrentCamera(int camNum)
```

Sets the current active camera by camera number.

Parameters:
- `camNum`: Camera number to activate. See [Camera Number Reference](#camera-number-reference). Valid values are `0..6`.


#### GetCurrentCamera

```C
GetCurrentCamera() -> int
```

Returns the number of the current active camera.

Returns:
- Camera number from the [Camera Number Reference](#camera-number-reference), or `-1` when there is no valid current camera.


#### CycleCamera

```C
CycleCamera()
```

Cycles to the next camera using the engine camera-cycle helper.

Notes:
- The cycle helper currently rotates only among camera numbers `0`, `1`, and `6` from the [Camera Number Reference](#camera-number-reference).


#### SetPOVShake

```C
SetPOVShake(Vector posOffset, Vector angleOffset, float posDelta, float angDelta)
```

Applies a point-of-view shake effect to the camera system.

Parameters:
- `posOffset`: Positional shake offset vector.
- `angleOffset`: Angular shake offset vector.
- `posDelta`: Positional shake magnitude/scalar.
- `angDelta`: Angular shake magnitude/scalar.


#### SetCameraStateFlags

```C
SetCameraStateFlags(int flags)
```

Sets the global camera-state flags.

Parameters:
- `flags`: Camera-state bitmask to pass to `sithCamera_SetCameraStateFlags()`. See [Camera State Flags](Flags.md#camera-state-flags).


#### GetCameraStateFlags

```C
GetCameraStateFlags() -> int
```

Returns the global camera-state flags.

Returns:
- Current global camera-state flag bitmask. See [Camera State Flags](Flags.md#camera-state-flags).


#### SetCameraSecondaryFocus

```C
SetCameraSecondaryFocus(int camNum, Thing focus)
```

Sets the secondary focus of one built-in engine camera.

Parameters:
- `camNum`: Camera number to update. See [Camera Number Reference](#camera-number-reference). Valid values are `0..6`.
- `focus`: Thing to assign as the camera's secondary focus.

### Heap, Hints, And Misc Compatibility Verbs


#### HeapNew

```C
HeapNew(int size)
```

Allocates or reallocates the current cog's heap array.

Parameters:
- `size`: Number of entries to allocate in the heap array. Non-positive values are ignored.

Notes:
- When a heap already exists, the wrapper frees it before allocating the new one.


#### HeapSet

```C
HeapSet(int idx, value val)
```

Stores a value in the current cog heap.

Parameters:
- `idx`: Zero-based index in the heap array previously allocated by `HeapNew()`.
- `val`: Generic script/VM value to store at that heap index. See [Generic `value` Type](Language.md#generic-value-type).

Notes:
- `HeapSet()` stores the current resolved value, not a live symbol reference.
- The write is ignored when `idx` is out of range or the value cannot be popped as a symbol.


#### HeapGet

```C
HeapGet(int num) -> value
```

Returns a value from the current cog heap.

Parameters:
- `num`: Zero-based index in the heap array previously allocated by `HeapNew()`.

Returns:
- Stored generic script/VM value from that heap index. See [Generic `value` Type](Language.md#generic-value-type). Out-of-range reads log an error and push integer `0`.


#### HeapFree

```C
HeapFree()
```

Frees the current cog heap array, if one is allocated.


#### GetHintSolved

```C
GetHintSolved(Thing thing) -> int
```

Returns whether a hint thing is marked as solved.

Parameters:
- `thing`: Hint thing to query.

Returns:
- `1` when the hint is solved, `0` when unsolved, or `-1` when `thing` is not a hint.


#### SetHintSolved

```C
SetHintSolved(Thing thing)
```

Marks a hint thing as solved.

Parameters:
- `thing`: Hint thing to mark as solved.

Notes:
- Requires a thing of type `SITH_THING_HINT`.


#### SetHintUnsolved

```C
SetHintUnsolved(Thing thing)
```

Marks a hint thing as unsolved.

Parameters:
- `thing`: Hint thing to mark as unsolved.

Notes:
- Requires a thing of type `SITH_THING_HINT`.

## Debug And Devmode Verbs

The current tree keeps a small family of debugging verbs:

- [`InEditor()`](#ineditor)
- [`SetDebugModeFlags()`](#setdebugmodeflags)
- [`GetDebugModeFlags()`](#getdebugmodeflags)
- [`ClearDebugModeFlags()`](#cleardebugmodeflags)
- [`DebugPrint()`](#debugprint)
- [`DebugFlex()`](#debugflex)
- [`DebugInt()`](#debugint)
- [`DebugVector()`](#debugvector)
- [`DebugLocalSymbols()`](#debuglocalsymbols)
- [`DebugWaitForKey()`](#debugwaitforkey)

Only [`DebugWaitForKey()`](#debugwaitforkey) is hard-gated by [`SITHDEBUG_INEDITOR`](Flags.md#debug-mode-flags) in the current implementation. The other debug verbs are registered normally and are primarily logging helpers.
