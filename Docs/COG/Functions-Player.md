# Player Host Functions

Source: [`Libs/sith/Cog/sithCogFunctionPlayer.c`](../../Libs/sith/Cog/sithCogFunctionPlayer.c)

These verbs cover player inventory flags, backpack helpers, local-player special states, and a few multiplayer or front-end specific helpers.

## Notes

- Several verbs are explicitly local-player only even though they do not take a `thing` argument.
- [`IsAiming()`](#isaiming) is registered under the lowercase script verb `isaiming`, even though the backing C function is spelled `IsAming`.
- [`MakeMeAPirate()`](#makemeapirate) is the Guybrush easter-egg toggle.
- Signatures below use PascalCase for readability, even though host-function lookup in scripts is case-insensitive.

## Function Index

### Inventory Availability And Goal Flags

- [SetInvActivated](#setinvactivated)
- [SetInvAvailable](#setinvavailable)
- [SetInvDisabled](#setinvdisabled)
- [IsInvActivated](#isinvactivated)
- [IsInvAvailable](#isinvavailable)
- [SetGoalFlags](#setgoalflags)
- [ClearGoalFlags](#cleargoalflags)
- [GetCurItem](#getcuritem)
- [SetSwimmingInventory](#setswimminginventory)
- [ResetInventory](#resetinventory)
- [IsItemFound](#isitemfound)
- [SetWhipElectric](#setwhipelectric)

### Player And Backpack Queries

- [GetNumPlayers](#getnumplayers)
- [GetMaxPlayers](#getmaxplayers)
- [GetAbsoluteMaxPlayers](#getabsolutemaxplayers)
- [GetLocalPlayerThing](#getlocalplayerthing)
- [GetPlayerThing](#getplayerthing)
- [GetPlayerNum](#getplayernum)
- [PickupBackpack](#pickupbackpack)
- [CreateBackpack](#createbackpack)
- [NthBackpackBin](#nthbackpackbin)
- [NthBackpackValue](#nthbackpackvalue)
- [NumBackpackItems](#numbackpackitems)
- [GetRespawnMask](#getrespawnmask)
- [SetRespawnMask](#setrespawnmask)
- [SyncScores](#syncscores)

### Local-Player Special States

- [JewelFlyingStatus](#jewelflyingstatus)
- [StartJewelFlying](#startjewelflying)
- [IsAiming](#isaiming)
- [StartInvisibility](#startinvisibility)
- [EndInvisibility](#endinvisibility)
- [IsInvisible](#isinvisible)
- [MakeMeStop](#makemestop)
- [PlayerInPor](#playerinpor)
- [GetCutsceneMode](#getcutscenemode)
- [GetLastWeapon](#getlastweapon)
- [MakeMeAPirate](#makemeapirate)

### Firing Helpers

- [IMPStartFiring](#impstartfiring)
- [IMPEndFiring](#impendfiring)

## Function Reference

### Inventory Availability And Goal Flags


#### SetInvActivated

```C++
SetInvActivated(Thing thing, int typeId, int bActivated)
```

Sets whether inventory type `typeId` is marked as activated for the player `thing`.

Parameters:
- `thing`: Player thing whose inventory activation state should be updated.
- `typeId`: Inventory type or bin id to mark as activated or deactivated.
- `bActivated`: Non-zero to mark that inventory entry as activated, `0` to clear the activated state.

Notes:
- Requires a player thing with player data.


#### SetInvAvailable

```C++
SetInvAvailable(Thing thing, int typeId, int bAvailable)
```

Sets whether inventory type `typeId` is available to the player `thing`.

Parameters:
- `thing`: Player thing whose inventory availability state should be updated.
- `typeId`: Inventory type or bin id to mark as available or unavailable.
- `bAvailable`: Non-zero to make that inventory entry available, `0` to mark it unavailable.

Notes:
- Requires a player thing with player data.


#### SetInvDisabled

```C++
SetInvDisabled(Thing thing, int typeId, int bDisabled)
```

Sets whether inventory type `typeId` is disabled for the player `thing`.

Parameters:
- `thing`: Player thing whose inventory disabled state should be updated.
- `typeId`: Inventory type or bin id to mark as disabled or enabled.
- `bDisabled`: Non-zero to disable that inventory entry, `0` to enable it.


#### IsInvActivated

```C++
IsInvActivated(Thing thing, int bin) -> int
```

Returns whether inventory bin `bin` is currently activated for the player `thing`.

Parameters:
- `thing`: Player thing whose inventory activation state should be queried.
- `bin`: Inventory bin id to test.

Returns:
- Non-zero when the condition is true, otherwise `0`.

Notes:
- Requires a player thing with player data.


#### IsInvAvailable

```C++
IsInvAvailable(Thing thing, int typeId) -> int
```

Returns whether inventory type `typeId` is currently available for the player `thing`.

Parameters:
- `thing`: Player thing whose inventory availability state should be queried.
- `typeId`: Inventory type or bin id to test.

Returns:
- Non-zero when the condition is true, otherwise `0`.

Notes:
- Requires a player thing with player data.


#### SetGoalFlags

```C++
SetGoalFlags(Thing thing, int bin, int flags)
```

Sets goal flags on the goal inventory entry selected by `bin` for the player `thing`.

Parameters:
- `thing`: Player thing whose goal inventory flags should be updated.
- `bin`: Goal bin index. The wrapper offsets it into the internal goal-inventory range.
- `flags`: Raw goal-entry bitmask to OR into the selected goal inventory entry. See [Goal Entry Masks And Respawn Masks](Flags.md#goal-entry-masks-and-respawn-masks).

Notes:
- Requires a player thing with player data.


#### ClearGoalFlags

```C++
ClearGoalFlags(Thing thing, int bin, int flags)
```

Clears goal flags from the goal inventory entry selected by `bin` for the player `thing`.

Parameters:
- `thing`: Player thing whose goal inventory flags should be updated.
- `bin`: Goal bin index. The wrapper offsets it into the internal goal-inventory range.
- `flags`: Raw goal-entry bitmask to clear from the selected goal inventory entry. See [Goal Entry Masks And Respawn Masks](Flags.md#goal-entry-masks-and-respawn-masks).

Notes:
- Requires a player thing with player data.


#### GetCurItem

```C++
GetCurItem(Thing thing) -> int
```

Returns the currently selected inventory item for the player `thing`.

Parameters:
- `thing`: Player thing for which to retrieve the currently selected inventory item.

Returns:
- Inventory item id of the currently selected item, or `-1` on invalid input.

Notes:
- Requires a player thing with player data.


#### SetSwimmingInventory

```C++
SetSwimmingInventory(Thing thing, int bItemsAvailable)
```

Enables or disables the swimming inventory loadout for the player `thing`.

Parameters:
- `thing`: Player thing whose swimming inventory state should be updated.
- `bItemsAvailable`: Non-zero to enable the swimming inventory loadout, `0` to disable it.


#### ResetInventory

```C++
ResetInventory(Thing thing)
```

Reinitializes the player's inventory and clears its swap-list state.

Parameters:
- `thing`: Player thing whose inventory should be reset.

Notes:
- Requires a player thing with player data.


#### IsItemFound

```C++
IsItemFound(int itemID) -> int
```

Returns whether the local player has already found inventory item `itemID`.

Parameters:
- `itemID`: Inventory item id to test in the local-player inventory table.

Returns:
- `1` when the item has been marked as found, otherwise `0`.

Notes:
- The wrapper checks the local player inventory state. If no local player is available, the current implementation may return without pushing a fallback value.


#### SetWhipElectric

```C++
SetWhipElectric(int bElectric)
```

Enables or disables the local player's electric-whip state.

Parameters:
- `bElectric`: Non-zero to enable the electric whip, `0` to disable it.

Notes:
- The wrapper updates both the local-player actor flag and the whip material cel used by the rendered effect.

### Player And Backpack Queries


#### GetNumPlayers

```C++
GetNumPlayers() -> int
```

Returns the number of players currently marked as joined to the game.

Returns:
- Joined-player count.

#### GetMaxPlayers

```C++
GetMaxPlayers() -> int
```

Returns the current player-slot count managed by the engine.

Returns:
- Current player-slot count.

#### GetAbsoluteMaxPlayers

```C++
GetAbsoluteMaxPlayers() -> int
```

Returns the hard engine limit for player slots.

Returns:
- Absolute player-slot limit.

#### GetLocalPlayerThing

```C++
GetLocalPlayerThing() -> Thing
```

Returns the thing for the local player.

Returns:
- Local player thing, or `-1` when no local player exists.

#### GetPlayerThing

```C++
GetPlayerThing(int playerNum) -> Thing
```

Returns the thing assigned to player slot `playerNum`.

Parameters:
- `playerNum`: Player slot to query.

Returns:
- Player thing for that slot, or `-1` on invalid input.

#### GetPlayerNum

```C++
GetPlayerNum(Thing thing) -> int
```

Returns the current player number of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current player number.

Returns:
- Current player number of the `thing`.


#### PickupBackpack

```C++
PickupBackpack(Thing thing, Thing backpack)
```

Transfers the contents of `backpack` into `thing`'s inventory.

Parameters:
- `thing`: Player thing that should receive the backpack contents.
- `backpack`: Backpack item thing to pick up.

Notes:
- Requires a player thing and an item thing with the [`SITH_ITEM_BACKPACK`](Flags.md#item-flags) flag.

#### CreateBackpack

```C++
CreateBackpack(Thing thing) -> Thing
```

Creates a backpack drop from `thing`'s current inventory state.

Parameters:
- `thing`: Player thing whose inventory should be packed into a backpack.

Returns:
- Created backpack thing, or `-1` on failure.

Notes:
- Requires a player thing.

#### NthBackpackBin

```C++
NthBackpackBin(Thing thing, int itemNum) -> int
```

Returns the inventory bin id stored at backpack slot `itemNum`.

Parameters:
- `thing`: Backpack item thing to inspect.
- `itemNum`: Backpack entry index.

Returns:
- Inventory bin id stored in that backpack slot.

Notes:
- Requires an item thing with the [`SITH_ITEM_BACKPACK`](Flags.md#item-flags) flag.
- On invalid input, the current wrapper logs an error and returns without pushing a fallback value.

#### NthBackpackValue

```C++
NthBackpackValue(Thing thing, int itemId) -> float
```

Returns the stored inventory amount for backpack entry `itemId`.

Parameters:
- `thing`: Backpack item thing to inspect.
- `itemId`: Inventory bin id stored in the backpack.

Returns:
- Stored value for that backpack entry.

Notes:
- Requires an item thing with the [`SITH_ITEM_BACKPACK`](Flags.md#item-flags) flag.
- On invalid input, the current wrapper logs an error and returns without pushing a fallback value.

#### NumBackpackItems

```C++
NumBackpackItems(Thing thing) -> int
```

Returns how many inventory entries are stored in `thing`.

Parameters:
- `thing`: Backpack item thing to inspect.

Returns:
- Number of stored backpack entries.

Notes:
- Requires an item thing with the [`SITH_ITEM_BACKPACK`](Flags.md#item-flags) flag.
- On invalid input, the current wrapper logs an error and returns without pushing a fallback value.

#### GetRespawnMask

```C++
GetRespawnMask(Thing thing) -> int
```

Returns the current respawn mask of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current respawn mask.

Returns:
- Current respawn mask of the `thing`. See [Goal Entry Masks And Respawn Masks](Flags.md#goal-entry-masks-and-respawn-masks).

Notes:
- Requires a player thing with player data.


#### SetRespawnMask

```C++
SetRespawnMask(Thing thing, int mask)
```

Sets the respawn mask of the `thing`.

Parameters:
- `thing`: Thing whose respawn mask to set.
- `mask`: Raw respawn mask bitfield to copy into the player's stored respawn mask value. See [Goal Entry Masks And Respawn Masks](Flags.md#goal-entry-masks-and-respawn-masks).

Notes:
- Requires a player thing with player data.


#### SyncScores

```C++
SyncScores()
```

Requests a multiplayer score sync.

Notes:
- The wrapper only calls the multiplayer sync helper when a network game is active.

### Local-Player Special States


#### JewelFlyingStatus

```C++
JewelFlyingStatus(int bEnable, Thing plasmaThing)
```

Enables or disables jewel-flying state for the local player.

Parameters:
- `bEnable`: Non-zero to enable jewel flying, `0` to disable it.
- `plasmaThing`: Thing to store as the jewel-flying plasma anchor. When jewel flying starts, the engine unhides this thing and uses its X/Y position as the target point for the player's initial move-in.

Notes:
- When `bEnable` is non-zero, the wrapper stores `plasmaThing` in `sithPlayerActions_g_pPlasma` and sets the jewel-flying enabled flag.
- If the active IMP firing type is already `SITHWEAPON_IMP3` when jewel flying is enabled, the wrapper immediately tries to start jewel flying.
- `plasmaThing` may be null. In that case jewel flying can still be enabled, but the player will not be moved toward a plasma anchor thing on start.
- When jewel flying is disabled, the stored plasma thing is hidden again by setting [`SITH_TF_DISABLED`](Flags.md#thing-flags), then the stored reference is cleared.
- On disable, the wrapper only calls `DisableJewelFlying()` when the local player is not already in `SITHPLAYERMOVE_JEWELFLYING`.

#### StartJewelFlying

```C++
StartJewelFlying() -> int
```

Attempts to start jewel-flying mode for the local player.

Returns:
- Non-zero when jewel flying started successfully, otherwise `0`.

Notes:
- The current implementation requires the local player's current weapon to be `SITHWEAPON_IMP3`.
- Jewel flying must already have been enabled through [`JewelFlyingStatus(1, plasmaThing)`](#jewelflyingstatus) before this call can succeed.
- If a plasma anchor thing was provided earlier, the player is moved toward that thing's X/Y position and the thing is unhidden before the floating keyframe starts.

#### IsAiming

```C++
IsAiming(Thing thing) -> int
```

Returns whether `thing` is currently aiming a weapon.

Parameters:
- `thing`: Actor or player thing to test.

Returns:
- `1` when the thing is aiming, otherwise `0`.

Notes:
- Requires an actor or player thing.

#### StartInvisibility

```C++
StartInvisibility()
```

Starts the local-player invisibility effect.

#### EndInvisibility

```C++
EndInvisibility()
```

Ends the local-player invisibility effect.

#### IsInvisible

```C++
IsInvisible() -> int
```

Returns whether the local player is currently invisible.

Returns:
- `1` when invisibility is active, otherwise `0`.

#### MakeMeStop

```C++
MakeMeStop() -> int
```

Forces the local player to come to a stop and then blocks the current COG until that stop finishes.

Returns:
- Local player GUID when waiting started, or `-1` when the request is invalid for the current player state.

Notes:
- Fails for jeep, raft, and minecar vehicle states.
- On success, the wrapper sets `SITHCOG_STATUS_WAITING_PLAYER_TO_STOP`.

#### PlayerInPor

```C++
PlayerInPor(int bInPor)
```

Sets the global player-in-`PoR` flag used by the player system.

Parameters:
- `bInPor`: Non-zero to mark the player as being in `PoR`, otherwise `0`.

Notes:
- The current source never expands the `PoR` acronym into a longer name.
- Based on `sithPlayer_Update()`, this flag marks a safe state or area where the player recovers from Aetherium buildup instead of continuing to accumulate it.
- `portal` is a plausible reading of `PoR`, but that is an inference rather than an explicit engine definition.

#### GetCutsceneMode

```C++
GetCutsceneMode() -> int
```

Returns whether player cutscene mode is currently enabled.

Returns:
- `1` when cutscene mode is enabled, otherwise `0`.

#### GetLastWeapon

```C++
GetLastWeapon() -> int
```

Returns the last weapon id remembered by the weapon system.

Returns:
- Last weapon id.

#### MakeMeAPirate

```C++
MakeMeAPirate()
```

Toggles the Guybrush pirate easter egg for the player.

#### IMPStartFiring

```C++
IMPStartFiring(int fireType)
```

Starts IMP firing state for the requested IMP weapon type.

Parameters:
- `fireType`: IMP weapon id to store as the active IMP firing type.

Notes:
- Valid values handled by the current player update logic are `SITHWEAPON_IMP1` (`14`), `SITHWEAPON_IMP2` (`15`), `SITHWEAPON_IMP3` (`16`), `SITHWEAPON_IMP4` (`17`), and `SITHWEAPON_IMP5` (`18`).
- `SITHWEAPON_IMP1` and `SITHWEAPON_IMP4` are treated as single-shot modes and are cleared automatically on the next player update.
- `SITHWEAPON_IMP2`, `SITHWEAPON_IMP3`, and `SITHWEAPON_IMP5` stay active until a matching [`IMPEndFiring()`](#impendfiring) call clears them.
- `SITHWEAPON_IMP3` is also checked by the jewel-flying code path.
- Other integer values can be stored, but the current player update logic does not give them any IMP-specific behavior.

#### IMPEndFiring

```C++
IMPEndFiring(int fireType)
```

Ends IMP firing state for the requested IMP weapon type.

Parameters:
- `fireType`: IMP weapon id to clear if it matches the currently active IMP firing type.

Notes:
- Use the same IMP weapon ids documented under [`IMPStartFiring()`](#impstartfiring).
- The current implementation only clears the firing state when `fireType` exactly matches the stored active IMP fire type.
