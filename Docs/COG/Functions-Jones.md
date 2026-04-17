# Jones Host Functions

Source: [`Jones3D/Play/jonesCog.c`](../../Jones3D/Play/jonesCog.c)

This is the Jones-specific extension layer that sits on top of the base Sith COG system.

Signatures below use PascalCase for readability, even though host-function lookup in scripts is case-insensitive.

## Function Index

### Functions

- [HealthDisplayOff](#healthdisplayoff)
- [HealthDisplayOn](#healthdisplayon)
- [JonesInvItemChanged](#jonesinvitemchanged)
- [JonesEndLevel](#jonesendlevel)
- [ExitToShell](#exittoshell)
- [StartCutscene](#startcutscene)
- [EndCutscene](#endcutscene)
- [UpdateDifficulty](#updatedifficulty)
- [EnableInterface](#enableinterface)

## Function Reference

### Functions


#### HealthDisplayOff

```C++
HealthDisplayOff()
```

Hides the health HUD and triggers the Jones HUD fade helper.


#### HealthDisplayOn

```C++
HealthDisplayOn()
```

Shows the health HUD and triggers the Jones HUD fade helper.


#### JonesInvItemChanged

```C++
JonesInvItemChanged(int bin)
```

Notifies the Jones HUD that inventory bin `bin` changed.

Parameters:
- `bin`: Inventory bin that changed.


#### JonesEndLevel

```C++
JonesEndLevel()
```

Runs the Jones level-complete flow.


#### ExitToShell

```C++
ExitToShell()
```

Leaves gameplay and routes to the Jones shell or ending flow.


#### StartCutscene

```C++
StartCutscene(int type)
```

Enters Jones cutscene mode.

Parameters:
- `type`: Cutscene presentation type. See the [StartCutscene type table](#startcutscene-type-table).


#### EndCutscene

```C++
EndCutscene()
```

Leaves Jones cutscene mode and restores HUD and interface state.


#### UpdateDifficulty

```C++
UpdateDifficulty(int difficulty)
```

Updates Jones difficulty-dependent state such as IQ-point handling.

Parameters:
- `difficulty`: Difficulty value to apply to the Jones gameplay layer. See [Difficulty Values](Types-And-Modes.md#difficulty-values).


#### EnableInterface

```C++
EnableInterface(int bEnable)
```

Enables or disables the in-game interface and gamesave availability.

Parameters:
- `bEnable`: Non-zero to enable the interface and gamesaving, zero to disable them.

<a id="startcutscene-type-table"></a>

## `StartCutscene(int type)`

`type` controls how much of the HUD remains visible:

| Value | Behavior |
| ---: | --- |
| `0` | Keep the HUD visible. |
| `1` | Fade the health display out while leaving the rest of the transition smoother. |
| `2` | Hide the HUD immediately. |

When a cutscene starts, the current Jones layer also:

- closes the overlay map if it is open
- hides the console if it is open
- disables the menu/interface
- disables gamesaving
- sets the cutscene camera flag
- makes the local player invulnerable
- sends `user2` to the player thing cog, if present
- destroys active weapon things
- stops currently attacking AIs

If the Jones menu is already visible, the requested cutscene type is deferred until the menu closes.

## `EndCutscene()`

When the cutscene ends, the current code:

- restores the overlay map if it was visible before the cutscene
- fades the health HUD back in
- re-enables the menu/interface
- re-enables gamesaving
- clears the cutscene camera flag
- clears player cutscene mode
- removes temporary invulnerability unless some other player flag still requires it
- sends `user3` to the player thing cog, if present

## `EnableInterface(int bEnable)`

`EnableInterface` affects both the visible interface and whether gamesaving is allowed.

Like `StartCutscene`, it also has a deferred path when the Jones menu is currently visible: the desired interface state is stored and applied later.
