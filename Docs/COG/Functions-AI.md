# AI Host Functions

Source: [`Libs/sith/Cog/sithCogFunctionAI.c`](../../Libs/sith/Cog/sithCogFunctionAI.c)

These verbs operate on AI-controlled things, AI classes, line-of-sight queries, and the waypoint graph.

## Notes

- Most verbs expect an AI-controlled `thing`.
- `AIWaitForStop` and `AIWaitForHeadTracking` suspend the current cog until the AI state changes.
- The waypoint verbs operate on the engine's shared waypoint data, not per-cog local structures.
- Signatures below use PascalCase for readability, even though host-function lookup in scripts is case-insensitive.

## Function Index

### Mode And State

- [AIGetMode](#aigetmode)
- [AISetMode](#aisetmode)
- [AIClearMode](#aiclearmode)
- [AIGetSubMode](#aigetsubmode)
- [AISetSubMode](#aisetsubmode)
- [AIClearSubMode](#aiclearsubmode)
- [AIGetArmedMode](#aigetarmedmode)
- [AISetClass](#aisetclass)
- [AIEnableInstinct](#aienableinstinct)
- [AISetCutsceneMode](#aisetcutscenemode)
- [AIClearCutsceneMode](#aiclearcutscenemode)
- [AISetInstinctWpntMode](#aisetinstinctwpntmode)
- [AIClearInstinctWpntMode](#aiclearinstinctwpntmode)

### Goals, Movement, And Waiting

- [AIGetMovePos](#aigetmovepos)
- [AISetMovePos](#aisetmovepos)
- [AIGetHomePos](#aigethomepos)
- [AIPauseMove](#aipausemove)
- [AISetMaxHomeDist](#aisetmaxhomedist)
- [AIGetGoalThing](#aigetgoalthing)
- [AIGetGoalLVec](#aigetgoallvec)
- [AISetFireTarget](#aisetfiretarget)
- [AISetMoveThing](#aisetmovething)
- [AISetMoveSpeed](#aisetmovespeed)
- [AISetGoalThing](#aisetgoalthing)
- [AIFlee](#aiflee)
- [AIStopFlee](#aistopflee)
- [AIFleeToWpnt](#aifleetowpnt)
- [AIJump](#aijump)
- [AIKnockout](#aiknockout)
- [AIRunOver](#airunover)
- [AIWaitForStop](#aiwaitforstop)

### Vision And Tracking

- [FirstThingInView](#firstthinginview)
- [NextThingInView](#nextthinginview)
- [ThingViewDot](#thingviewdot)
- [IsAITargetInSight](#isaitargetinsight)
- [AISetLookPos](#aisetlookpos)
- [AISetLookFrame](#aisetlookframe)
- [AISetMoveFrame](#aisetmoveframe)
- [AISetLookThing](#aisetlookthing)
- [AISetLookThingEyeLevel](#aisetlookthingeyelevel)
- [AIEnableHeadTracking](#aienableheadtracking)
- [AIDisableHeadTracking](#aidisableheadtracking)
- [AIEnableBodyTracking](#aienablebodytracking)
- [AIDisableBodyTracking](#aidisablebodytracking)
- [AIWaitForHeadTracking](#aiwaitforheadtracking)

### Surface And Navigation Filters

- [AISetAllowedSurfaceType](#aisetallowedsurfacetype)
- [AIClearAllowedSurfaceType](#aiclearallowedsurfacetype)
- [AISpat](#aispat)

### Waypoints

- [AISetWpnt](#aisetwpnt)
- [AISetWpntRank](#aisetwpntrank)
- [AISetWpntFlags](#aisetwpntflags)
- [AIClearWpntFlags](#aiclearwpntflags)
- [AISetActiveWpntLayer](#aisetactivewpntlayer)
- [AIConnectWpnts](#aiconnectwpnts)
- [AIConnectWpntsOneWay](#aiconnectwpntsoneway)
- [AITraverseWpnts](#aitraversewpnts)
- [AIClearTraverseWpnts](#aicleartraversewpnts)
- [AIFindNearestWpnt](#aifindnearestwpnt)
- [AIWpntHuntTarget](#aiwpnthunttarget)

## Function Reference

### Mode And State


#### AIGetMode

```C++
AIGetMode(Thing thing) -> int
```

Returns the current AI mode of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current mode.

Returns:
- Current mode of the `thing`. See [AI Mode Flags](Flags.md#ai-mode-flags).

Notes:
- Requires an AI-controlled thing.


#### AISetMode

```C++
AISetMode(Thing thing, int newMode)
```

Sets AI mode flags on `thing`.

Parameters:
- `thing`: Thing whose mode to set.
- `newMode`: AI mode bitmask to OR into the current mode flags. See [AI Mode Flags](Flags.md#ai-mode-flags).

Notes:
- Requires an AI-controlled thing.
- The current implementation adds flags instead of replacing the entire mode value.
- When the mode bitfield changes, the wrapper emits `SITHAI_EVENT_MODECHANGED`.


#### AIClearMode

```C++
AIClearMode(Thing thing, int mode)
```

Clears the specified mode on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified mode.
- `mode`: AI mode bitmask to clear from the current mode flags. See [AI Mode Flags](Flags.md#ai-mode-flags).

Notes:
- Requires an AI-controlled thing.
- When the mode bitfield changes, the wrapper emits `SITHAI_EVENT_MODECHANGED`.


#### AIGetSubMode

```C++
AIGetSubMode(Thing thing) -> int
```

Returns the current AI sub mode of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current sub mode.

Returns:
- Current sub mode of the `thing`. See [AI Submode Flags](Flags.md#ai-submode-flags).

Notes:
- Requires an AI-controlled thing.


#### AISetSubMode

```C++
AISetSubMode(Thing thing, int submode)
```

Sets AI submode flags on `thing`.

Parameters:
- `thing`: Thing whose sub mode to set.
- `submode`: AI submode bitmask to OR into the current submode flags. See [AI Submode Flags](Flags.md#ai-submode-flags).

Notes:
- Requires an AI-controlled thing.


#### AIClearSubMode

```C++
AIClearSubMode(Thing thing, int submode)
```

Clears the specified sub mode on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified sub mode.
- `submode`: AI submode bitmask to clear from the current submode flags. See [AI Submode Flags](Flags.md#ai-submode-flags).

Notes:
- Requires an AI-controlled thing.


#### AIGetArmedMode

```C++
AIGetArmedMode(Thing thing) -> int
```

Returns the current AI armed mode of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current armed mode.

Returns:
- Current puppet armed-mode value of the `thing`. See [Puppet Armed Modes](Types-And-Modes.md#puppet-armed-modes).


#### AISetClass

```C++
AISetClass(Thing thing, AIClass aiClass)
```

Sets the AI class of the `thing`.

Parameters:
- `thing`: Thing whose class to set.
- `aiClass`: AI class resource to assign to the thing.

Notes:
- Requires an AI-controlled thing.
- The wrapper updates both the thing-level AI class pointer and the local AI state, then refreshes the local instinct count from the new class.


#### AIEnableInstinct

```C++
AIEnableInstinct(Thing thing, string instinctName, int bEnable) -> int
```

Enables or disables the requested instinct feature.

Parameters:
- `thing`: AI-controlled thing whose instinct setting should be changed.
- `instinctName`: Name of the instinct to enable or disable.
- `bEnable`: Non-zero to enable the feature, zero to disable it.

Returns:
- Non-zero on success, otherwise `0`.

Notes:
- Requires an AI-controlled thing.


#### AISetCutsceneMode

```C++
AISetCutsceneMode(Thing thing)
```

Sets the AI cutscene mode of the `thing`.

Parameters:
- `thing`: Thing whose cutscene mode to set.

Notes:
- Requires an AI-controlled thing.


#### AIClearCutsceneMode

```C++
AIClearCutsceneMode(Thing thing)
```

Clears the specified cutscene mode on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified cutscene mode.

Notes:
- Requires an AI-controlled thing.


#### AISetInstinctWpntMode

```C++
AISetInstinctWpntMode(Thing thing)
```

Enables instinct waypoint mode for the AI controlling `thing`.

Parameters:
- `thing`: AI-controlled thing whose instinct waypoint mode should be enabled.

Notes:
- Requires an AI-controlled thing.


#### AIClearInstinctWpntMode

```C++
AIClearInstinctWpntMode(Thing thing)
```

Disables instinct waypoint mode for the AI controlling `thing`.

Parameters:
- `thing`: AI-controlled thing whose instinct waypoint mode should be disabled.

Notes:
- Requires an AI-controlled thing.

### Goals, Movement, And Waiting


#### AIGetMovePos

```C++
AIGetMovePos(Thing thing) -> Vector
```

Returns the current AI move position of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current move position.

Returns:
- Current move position of the `thing`.

Notes:
- Requires an AI-controlled thing.


#### AISetMovePos

```C++
AISetMovePos(Thing thing, Vector pos, int bWait)
```

Requests AI movement toward a world position.

Parameters:
- `thing`: Thing whose move position to set.
- `pos`: World position the AI should move toward.
- `bWait`: Non-zero to block the current cog until the AI stops, but only if the move request is accepted.

Notes:
- Requires an AI-controlled thing.
- If certain movement submodes are active, the wrapper stops the current AI movement before issuing the new move request.
- When the move request fails, the wrapper immediately sends an `arrived` message instead of waiting.


#### AIGetHomePos

```C++
AIGetHomePos(Thing thing) -> Vector
```

Returns the current AI home position of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current home position.

Returns:
- Current home position of the `thing`.

Notes:
- Requires an AI-controlled thing.


#### AIPauseMove

```C++
AIPauseMove(Thing thing, int msecPause)
```

Pauses the current move on the `thing`.

Parameters:
- `thing`: Thing whose move should be paused.
- `msecPause`: Pause duration in milliseconds.

Notes:
- Requires an AI-controlled thing.


#### AISetMaxHomeDist

```C++
AISetMaxHomeDist(Thing thing, float dist)
```

Sets how far the AI may stray from its home position.

Parameters:
- `thing`: AI-controlled thing whose maximum home distance should be updated.
- `dist`: Maximum allowed distance from the AI home position.

Notes:
- Requires an AI-controlled thing.


#### AIGetGoalThing

```C++
AIGetGoalThing(Thing thing) -> Thing
```

Returns the current AI goal thing of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current goal thing.

Returns:
- Current goal thing of the `thing`.

Notes:
- Requires an AI-controlled thing.


#### AIGetGoalLVec

```C++
AIGetGoalLVec(Thing thing) -> Vector
```

Returns the current AI goal left vector of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current goal left vector.

Returns:
- Current goal left vector of the `thing`.


#### AISetFireTarget

```C++
AISetFireTarget(Thing thing, Thing target)
```

Sets the AI fire target of the `thing`.

Parameters:
- `thing`: Thing whose fire target to set.
- `target`: Thing to assign as both the current fire target and the current goal thing.

Notes:
- Requires an AI-controlled thing.
- The wrapper enters attacking mode, stamps the attack start time, and forces updates for the `primaryfire`, `lobfire`, and `alternatefire` instincts.
- Unless continuous waypoint motion is active, it also clears traverse-waypoints mode.


#### AISetMoveThing

```C++
AISetMoveThing(Thing thing, Thing goalThing, int bWait)
```

Requests AI movement toward another thing.

Parameters:
- `thing`: AI-controlled thing that should move.
- `goalThing`: Thing whose current position should be used as the move target. The wrapper also stores this thing as the AI goal thing.
- `bWait`: Non-zero to block the current cog until the AI stops moving.

Notes:
- Requires an AI-controlled thing.
- The current implementation always enters the wait state when `bWait` is non-zero; it does not first check whether the move request succeeded.


#### AISetMoveSpeed

```C++
AISetMoveSpeed(Thing thing, float speed)
```

Sets the AI move speed of the `thing`.

Parameters:
- `thing`: Thing whose move speed to set.
- `speed`: New movement speed to store in the AI local state.

Notes:
- Requires an AI-controlled thing.


#### AISetGoalThing

```C++
AISetGoalThing(Thing thing, Thing goalThing)
```

Sets the AI goal thing without starting movement.

Parameters:
- `thing`: AI-controlled thing whose goal thing should be updated.
- `goalThing`: Thing to store as the new goal thing.

Notes:
- Requires an AI-controlled thing.


#### AIFlee

```C++
AIFlee(Thing thing, Thing fleeFromThing)
```

Puts `thing` into AI fleeing mode away from `fleeFromThing`.

Parameters:
- `thing`: AI-controlled actor that should flee.
- `fleeFromThing`: Thing the AI should flee from.

Notes:
- Requires an AI-controlled actor with local AI state.

#### AIStopFlee

```C++
AIStopFlee(Thing thing)
```

Clears AI fleeing mode on `thing` and restores searching mode.

Parameters:
- `thing`: AI-controlled actor that should stop fleeing.

Notes:
- Requires an AI-controlled actor with local AI state.

#### AIFleeToWpnt

```C++
AIFleeToWpnt(Thing thing, int idx)
```

Requests that `thing` flee toward waypoint `idx`.

Parameters:
- `thing`: AI-controlled actor that should flee.
- `idx`: Waypoint index to flee toward.

Notes:
- Requires an AI-controlled actor with local AI state.

#### AIJump

```C++
AIJump(Thing thing, Vector movePos, float arg)
```

Requests an AI jump toward `movePos`.

Parameters:
- `thing`: AI-controlled actor that should jump.
- `movePos`: Target position for the jump.
- `arg`: Extra float argument passed straight through to the AI jump helper. The current source does not give this value a more descriptive name.

Notes:
- Requires an AI-controlled actor with local AI state and a non-zero attach flag.

#### AIKnockout

```C++
AIKnockout(Thing thing, float secDuration, int timerId)
```

Knocks out an AI-controlled actor for a timed duration, or restores it from knockout when the duration is non-positive.

Parameters:
- `thing`: AI-controlled actor to knock out or restore.
- `secDuration`: Knockout duration in seconds. Values greater than `0` start a timed knockout; values less than or equal to `0` trigger the recovery path instead.
- `timerId`: Timer id delivered back through the later cog timer event when a timed knockout is started.

Notes:
- Requires an AI-controlled actor with a puppet and local AI state.
- A timed knockout removes puppet tracks, disables controls and collision, marks the actor immobile/invulnerable, hides its shadow, sets [`SITHAI_MODE_DISABLED`](Flags.md#ai-mode-flags), and schedules a cog timer event.
- The recovery path plays the wake-up / draw-weapon puppet sequence and restores the shadow flag.


#### AIRunOver

```C++
AIRunOver(Thing thing, float duration, int timerId)
```

Places `thing` into the AI run-over state, plays the run-over puppet mode, and schedules a callback timer.

Parameters:
- `thing`: AI-controlled actor to mark as run over.
- `duration`: Duration in seconds before the timer event is raised.
- `timerId`: Timer id delivered back through the later AI callback event.

Notes:
- Requires an AI-controlled thing with a puppet and local AI state.
- The wrapper disables the AI, marks the actor immobile/invulnerable, plays the death sound class, and queues a cog timer event.

#### AIWaitForStop

```C++
AIWaitForStop(Thing thing)
```

Blocks the current COG until the AI-controlled `thing` stops moving or turning.

Parameters:
- `thing`: AI-controlled thing to wait on until it stops moving or turning.

Notes:
- Requires an AI-controlled thing.
- The current COG status is changed to wait for the AI thing to stop.

### Vision And Tracking


#### FirstThingInView

```C++
FirstThingInView(Thing thing, float fovX, float distance, int thingTypeMask) -> Thing
```

Builds a visible-thing list from `thing` and returns the first matching result.

Parameters:
- `thing`: Thing whose view should be used for the search.
- `fovX`: Horizontal and vertical field of view in degrees. The current wrapper passes the same value for both axes.
- `distance`: Maximum search distance.
- `thingTypeMask`: Thing-type bitmask to include in the search results.

Returns:
- First matching thing, or `-1` when no matching thing is visible.

Notes:
- For actors and players, the wrapper applies head rotation and eye offset before gathering visible things.
- Use [`NextThingInView()`](#nextthinginview) to iterate the cached result set.

#### NextThingInView

```C++
NextThingInView() -> Thing
```

Advances the cached [`FirstThingInView()`](#firstthinginview) result set and returns the next matching thing.

Returns:
- Next cached result, or `-1` when the list is exhausted.

#### ThingViewDot

```C++
ThingViewDot(Thing thing1, Thing thing2) -> float
```

Returns the dot product between `thing1`'s view direction and the normalized direction from `thing1` to `thing2`.

Parameters:
- `thing1`: Viewer thing whose look direction should be used.
- `thing2`: Target thing to compare against the view direction.

Returns:
- View-direction dot product, or `-1000.0` on invalid input.

Notes:
- For actors and players, the wrapper includes `headPYR` in the view direction.

#### IsAITargetInSight

```C++
IsAITargetInSight(Thing thing) -> int
```

Returns whether the `thing` currently has the requested ai target in sight state.

Parameters:
- `thing`: Thing for which to test the requested ai target in sight state.

Returns:
- Non-zero when the condition is true, otherwise `0`.

Notes:
- Requires an AI-controlled thing.


#### AISetLookPos

```C++
AISetLookPos(Thing thing, Vector targetPos)
```

Sets the world-space point that `thing` should look at.

Parameters:
- `thing`: AI-controlled thing whose look target should be updated.
- `targetPos`: World-space position to look at.

Notes:
- Requires an AI-controlled thing.

#### AISetLookFrame

```C++
AISetLookFrame(Thing thing, int frameNum)
```

Sets the path frame that `thing` should use as its look target.

Parameters:
- `thing`: AI-controlled thing whose look frame should be updated.
- `frameNum`: Path frame index to look at.

Notes:
- Requires an AI-controlled thing.

#### AISetMoveFrame

```C++
AISetMoveFrame(Thing thing, int frame)
```

Sets the path frame that `thing` should move toward.

Parameters:
- `thing`: AI-controlled thing whose move frame should be updated.
- `frame`: Path frame index to move toward.

Notes:
- Requires an AI-controlled thing.

#### AISetLookThing

```C++
AISetLookThing(Thing thing, Thing target)
```

Makes `thing` look directly at `target`.

Parameters:
- `thing`: AI-controlled thing whose look target should be updated.
- `target`: Thing to look at.

Notes:
- Requires an AI-controlled thing.

#### AISetLookThingEyeLevel

```C++
AISetLookThingEyeLevel(Thing thing, Thing target)
```

Makes `thing` look at `target` using the target eye-level position.

Parameters:
- `thing`: AI-controlled thing whose look target should be updated.
- `target`: Thing whose eye-level position should be tracked.

Notes:
- Requires an AI-controlled thing.

#### AIEnableHeadTracking

```C++
AIEnableHeadTracking(Thing thing, Thing target)
```

Enables AI head tracking on `thing` toward `target`.

Parameters:
- `thing`: AI-controlled thing that should head-track.
- `target`: Thing to track.

Notes:
- Requires an AI-controlled thing.

#### AIDisableHeadTracking

```C++
AIDisableHeadTracking(Thing thing)
```

Disables AI head tracking on `thing`.

Parameters:
- `thing`: AI-controlled thing whose head tracking should be disabled.

Notes:
- Requires an AI-controlled thing.

#### AIEnableBodyTracking

```C++
AIEnableBodyTracking(Thing thing, Thing target)
```

Enables AI body tracking on `thing` toward `target`.

Parameters:
- `thing`: AI-controlled thing that should body-track.
- `target`: Thing to track.

Notes:
- Requires an AI-controlled thing.

#### AIDisableBodyTracking

```C++
AIDisableBodyTracking(Thing thing)
```

Disables AI body tracking on `thing`.

Parameters:
- `thing`: AI-controlled thing whose body tracking should be disabled.

Notes:
- Requires an AI-controlled thing.

#### AIWaitForHeadTracking

```C++
AIWaitForHeadTracking(Thing thing)
```

Blocks the current COG until the AI-controlled `thing` stops its current head-tracking motion.

Parameters:
- `thing`: AI-controlled thing to wait on until its current head-tracking motion stops.

Notes:
- Requires an AI-controlled thing.
- The current COG status is changed to wait for the AI head-tracking motion to stop.

### Surface And Navigation Filters


#### AISetAllowedSurfaceType

```C++
AISetAllowedSurfaceType(Thing thing, int surftypes)
```

Sets the allowed surface-type mask used by `thing`'s AI movement.

Parameters:
- `thing`: AI-controlled thing whose allowed surface mask should be updated.
- `surftypes`: Surface-type bitmask to allow.

Notes:
- Requires an AI-controlled thing.

#### AIClearAllowedSurfaceType

```C++
AIClearAllowedSurfaceType(Thing thing, int surftypes)
```

Clears bits from the allowed surface-type mask used by `thing`'s AI movement.

Parameters:
- `thing`: AI-controlled thing whose allowed surface mask should be updated.
- `surftypes`: Surface-type bitmask to clear.

Notes:
- Requires an AI-controlled thing.

#### AISpat

```C++
AISpat(Thing thing, float duration, int timerId)
```

Temporarily disables an AI-controlled actor in a scripted "spat" state, then optionally restores it when the duration expires or when called with a non-positive duration.

Parameters:
- `thing`: AI-controlled actor to place into or restore from the spat state.
- `duration`: Duration in seconds. Values greater than `0` start the spat state and queue a timer; values less than or equal to `0` clear the disabled/controls-blocked state.
- `timerId`: Timer id delivered back through the later cog timer event when a timed spat state is started.

Notes:
- Requires an AI-controlled actor with a puppet and local AI state.
- A timed spat removes puppet tracks, plays the run puppet mode, disables actor controls, sets move status to running, queues a cog timer event, and sets [`SITHAI_MODE_DISABLED`](Flags.md#ai-mode-flags).

### Waypoints


#### AISetWpnt

```C++
AISetWpnt(Thing thing, int wpntIdx)
```

Sets the current AI waypoint index for `thing`.

Parameters:
- `thing`: AI-controlled thing whose current waypoint index should be updated.
- `wpntIdx`: Waypoint index to assign.


#### AISetWpntRank

```C++
AISetWpntRank(int wpntNum, int rank)
```

Sets the rank value for waypoint `wpntNum`.

Parameters:
- `wpntNum`: Waypoint index to update.
- `rank`: Rank value to assign.

#### AISetWpntFlags

```C++
AISetWpntFlags(int wpntNum, int flags)
```

Sets waypoint flags on `wpntNum`.

Parameters:
- `wpntNum`: Waypoint index to update.
- `flags`: Waypoint-layer flag bits to set. See [AI Waypoint Layer Flags](Flags.md#ai-waypoint-layer-flags).

#### AIClearWpntFlags

```C++
AIClearWpntFlags(int wpntIdx, int flags)
```

Clears waypoint flags on `wpntIdx`.

Parameters:
- `wpntIdx`: Waypoint index to update.
- `flags`: Waypoint-layer flag bits to clear. See [AI Waypoint Layer Flags](Flags.md#ai-waypoint-layer-flags).

#### AISetActiveWpntLayer

```C++
AISetActiveWpntLayer(int layer)
```

Sets the currently active waypoint layer.

Parameters:
- `layer`: Waypoint layer to activate.

#### AIConnectWpnts

```C++
AIConnectWpnts(int wpntIdx1, int wpntIdx2)
```

Creates a two-way connection between waypoint `wpntIdx1` and `wpntIdx2`.

Parameters:
- `wpntIdx1`: First waypoint index.
- `wpntIdx2`: Second waypoint index.

#### AIConnectWpntsOneWay

```C++
AIConnectWpntsOneWay(int wpntIdx1, int wpntIdx2)
```

Creates a one-way connection from waypoint `wpntIdx1` to `wpntIdx2`.

Parameters:
- `wpntIdx1`: Source waypoint index.
- `wpntIdx2`: Destination waypoint index.

#### AITraverseWpnts

```C++
AITraverseWpnts(Thing thing, int wpntIdx, float moveSpeed, float degTurn, int mode) -> int
```

Starts waypoint traversal for `thing` from waypoint `wpntIdx`.

Parameters:
- `thing`: AI-controlled thing that should traverse waypoints.
- `wpntIdx`: Starting waypoint index.
- `moveSpeed`: Movement speed to use.
- `degTurn`: Turn rate in degrees.
- `mode`: Traversal mode passed to the AI waypoint code.

Returns:
- Integer status returned by the traversal request.

Notes:
- Requires an AI-controlled thing.

#### AIClearTraverseWpnts

```C++
AIClearTraverseWpnts(Thing thing)
```

Clears the AI traverse-waypoints mode on `thing`.

Parameters:
- `thing`: AI-controlled thing whose traverse-waypoints mode should be cleared.

Notes:
- Requires an AI-controlled thing.


#### AIFindNearestWpnt

```C++
AIFindNearestWpnt(Thing thing) -> int
```

Returns the nearest waypoint index for `thing`.

Parameters:
- `thing`: Player thing or AI-controlled thing for which to find the nearest waypoint.

Returns:
- Nearest waypoint index, or `-1` when no waypoint is found.

Notes:
- Requires an AI-controlled thing.


#### AIWpntHuntTarget

```C++
AIWpntHuntTarget(Thing thing, float moveSpeed, float degTurn) -> int
```

Starts waypoint-based hunt-target behavior for `thing`.

Parameters:
- `thing`: AI-controlled thing that should hunt along waypoints.
- `moveSpeed`: Movement speed to use.
- `degTurn`: Turn rate in degrees.

Returns:
- Integer status returned by the AI code.

Notes:
- Requires an AI-controlled thing.
