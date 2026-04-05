# Sector Host Functions

Source: `Libs/sith/Cog/sithCogFunctionSector.c`

These verbs operate on sector tint, extra light, adjoins, flags, thrust, and a few geometry/topology queries.

## Compatibility Notes

- [`GetColormap()`](#getcolormap) and [`GetSectorColormap()`](#getsectorcolormap) currently log `NOT SUPPORTED` and return `-1`.
- [`SetColormap()`](#setcolormap) and [`SetSectorColormap()`](#setsectorcolormap) are also currently unsupported.
- [`SetSectorThrust()`](#setsectorthrust) disables thrust when the provided direction vector is zero.
- [`SetSectorSurfflags()`](#setsectorsurfflags) and [`ClearSectorSurfflags()`](#clearsectorsurfflags) apply the given [surface flags](Flags.md#surface-flags) to every surface in the sector.
- Signatures below use PascalCase for readability, even though host-function lookup in scripts is case-insensitive.

## Function Index

### Functions

- [GetSectorTint](#getsectortint)
- [SetSectorTint](#setsectortint)
- [SetSectorAdjoins](#setsectoradjoins)
- [SectorAdjoins](#sectoradjoins)
- [GetSectorLight](#getsectorlight)
- [SetSectorLight](#setsectorlight)
- [SectorLight](#sectorlight)
- [GetColormap](#getcolormap)
- [GetSectorColormap](#getsectorcolormap)
- [SetColormap](#setcolormap)
- [SetSectorColormap](#setsectorcolormap)
- [GetSectorThrust](#getsectorthrust)
- [SetSectorThrust](#setsectorthrust)
- [SectorThrust](#sectorthrust)
- [GetSectorFlags](#getsectorflags)
- [SetSectorFlags](#setsectorflags)
- [ClearSectorFlags](#clearsectorflags)
- [GetSectorThingCount](#getsectorthingcount)
- [SectorThingCount](#sectorthingcount)
- [GetSectorPlayerCount](#getsectorplayercount)
- [SectorPlayerCount](#sectorplayercount)
- [GetSectorCount](#getsectorcount)
- [GetSectorCenter](#getsectorcenter)
- [GetNumSectorVertices](#getnumsectorvertices)
- [GetSectorVertexPos](#getsectorvertexpos)
- [GetNumSectorSurfaces](#getnumsectorsurfaces)
- [GetSectorSurfaceRef](#getsectorsurfaceref)
- [SyncSector](#syncsector)
- [FindSectorAtPos](#findsectoratpos)
- [SetSectorSurfflags](#setsectorsurfflags)
- [ClearSectorSurfflags](#clearsectorsurfflags)

## Function Reference

### Functions


#### GetSectorTint

```C
GetSectorTint(Sector sector) -> Vector
```

Returns the current tint of the `sector`.

Parameters:
- `sector`: Sector for which to retrieve the current tint.

Returns:
- Current tint of the `sector`.


#### SetSectorTint

```C
SetSectorTint(Sector sector, Vector color)
```

Sets the tint of the `sector`.

Parameters:
- `sector`: Sector whose tint to set.
- `color`: RGB tint color to store in the sector. Each component is clamped into the `0..1` range.


#### SetSectorAdjoins

```C
SetSectorAdjoins(Sector sector, int bOn)
```

Enables or disables the sector's adjoins.

Parameters:
- `sector`: Sector whose adjoins should be shown or hidden.
- `bOn`: Non-zero to show the sector's adjoins, `0` to hide them.


#### SectorAdjoins

```C
SectorAdjoins(Sector sector, int bOn)
```

Alias of [`SetSectorAdjoins()`](#setsectoradjoins).

Parameters:
- `sector`: Sector whose adjoins should be shown or hidden.
- `bOn`: Non-zero to show the sector's adjoins, `0` to hide them.

Notes:
- Shares the same implementation as [`SetSectorAdjoins()`](#setsectoradjoins).


#### GetSectorLight

```C
GetSectorLight(Sector sector) -> Vector
```

Returns the current light of the `sector`.

Parameters:
- `sector`: Sector for which to retrieve the current light.

Returns:
- Current light of the `sector`.


#### SetSectorLight

```C
SetSectorLight(Sector sector, Vector color, float timeDelta)
```

Sets or animates the sector's extra light color.

Parameters:
- `sector`: Sector whose light to set.
- `color`: Target RGB extra-light color for the sector.
- `timeDelta`: Transition time in seconds. `0` applies the new color immediately; non-zero starts a sector light animation toward `color`.


#### SectorLight

```C
SectorLight(Sector sector, Vector color, float timeDelta)
```

Alias of [`SetSectorLight()`](#setsectorlight).

Parameters:
- `sector`: Sector whose extra light should be updated.
- `color`: Target RGB extra-light color for the sector.
- `timeDelta`: Transition time in seconds. `0` applies the new color immediately; non-zero starts a sector light animation.

Notes:
- Shares the same implementation as [`SetSectorLight()`](#setsectorlight).


#### GetColormap

```C
GetColormap(Sector sector) -> int
```

Returns the sector colormap reference.

Parameters:
- `sector`: Sector whose colormap would be queried.

Returns:
- Always `-1` in the current engine, because sector colormaps are not supported.

Notes:
- The wrapper logs `Sector colormaps NOT SUPPORTED`.


#### GetSectorColormap

```C
GetSectorColormap(Sector sector) -> int
```

Returns the sector colormap reference.

Parameters:
- `sector`: Sector whose colormap would be queried.

Returns:
- Always `-1` in the current engine, because sector colormaps are not supported.

Notes:
- Shares the same implementation as [`GetColormap()`](#getcolormap).


#### SetColormap

```C
SetColormap(int value)
```

Attempts to set the sector colormap reference.

Parameters:
- `value`: Colormap id requested by the script. The current implementation ignores this value.

Notes:
- This verb is currently unsupported and only logs an error.


#### SetSectorColormap

```C
SetSectorColormap(int value)
```

Attempts to set the sector colormap reference.

Parameters:
- `value`: Colormap id requested by the script. The current implementation ignores this value.

Notes:
- Shares the same implementation as [`SetColormap()`](#setcolormap).


#### GetSectorThrust

```C
GetSectorThrust(Sector sector) -> Vector
```

Returns the current thrust of the `sector`.

Parameters:
- `sector`: Sector for which to retrieve the current thrust.

Returns:
- Current thrust of the `sector`.


#### SetSectorThrust

```C
SetSectorThrust(Sector sector, Vector dir, float speed)
```

Sets the thrust of the `sector`.

Parameters:
- `sector`: Sector whose thrust to set.
- `dir`: Thrust direction vector. A zero vector disables sector thrust entirely.
- `speed`: Scalar multiplier applied to `dir` before storing the sector thrust vector.

Notes:
- The current implementation stores `dir * speed` directly; it does not normalize `dir` first.


#### SectorThrust

```C
SectorThrust(Sector sector, Vector dir, float speed)
```

Alias of [`SetSectorThrust()`](#setsectorthrust).

Parameters:
- `sector`: Sector whose thrust should be updated.
- `dir`: Thrust direction vector. A zero vector disables sector thrust entirely.
- `speed`: Scalar multiplier applied to `dir` before storing the sector thrust vector.

Notes:
- Shares the same implementation as [`SetSectorThrust()`](#setsectorthrust).

Notes:
- Shares the same implementation as [`SetSectorThrust()`](#setsectorthrust).


#### GetSectorFlags

```C
GetSectorFlags(Sector sector) -> int
```

Returns the current flags of the `sector`.

Parameters:
- `sector`: Sector for which to retrieve the current flags.

Returns:
- Current flags of the `sector`. See [Sector Flags](Flags.md#sector-flags).


#### SetSectorFlags

```C
SetSectorFlags(Sector sector, int flags)
```

Sets the flags of the `sector`.

Parameters:
- `sector`: Sector whose flags to set.
- `flags`: Sector-flag bitmask to OR into the current sector flags. See [Sector Flags](Flags.md#sector-flags).


#### ClearSectorFlags

```C
ClearSectorFlags(Sector sector, int flags)
```

Clears the specified flags on the `sector`.

Parameters:
- `sector`: Sector on which to clear the specified flags.
- `flags`: Sector-flag bitmask to clear from the current sector flags. See [Sector Flags](Flags.md#sector-flags).


#### GetSectorThingCount

```C
GetSectorThingCount(Sector sector) -> int
```

Returns the current thing count of the `sector`.

Parameters:
- `sector`: Sector for which to retrieve the current thing count.

Returns:
- Current thing count of the `sector`.


#### SectorThingCount

```C
SectorThingCount(Sector sector) -> int
```

Returns the number of things currently in `sector`.

Parameters:
- `sector`: Sector whose thing count should be returned.

Returns:
- Number of things currently in `sector`.

Notes:
- Shares the same implementation as [`GetSectorThingCount()`](#getsectorthingcount).


#### GetSectorPlayerCount

```C
GetSectorPlayerCount(Sector sector) -> int
```

Returns the current player count of the `sector`.

Parameters:
- `sector`: Sector for which to retrieve the current player count.

Returns:
- Current player count of the `sector`.


#### SectorPlayerCount

```C
SectorPlayerCount(Sector sector) -> int
```

Returns the number of player things currently in `sector`.

Parameters:
- `sector`: Sector whose player count should be returned.

Returns:
- Number of player things currently in `sector`.

Notes:
- Shares the same implementation as [`GetSectorPlayerCount()`](#getsectorplayercount).


#### GetSectorCount

```C
GetSectorCount() -> int
```

Returns the total number of sectors in the current world.

Returns:
- Total number of sectors in the current world.


#### GetSectorCenter

```C
GetSectorCenter(Sector sector) -> Vector
```

Returns the current center point of the `sector`.

Parameters:
- `sector`: Sector for which to retrieve the current center point.

Returns:
- Current center point of the `sector`.


#### GetNumSectorVertices

```C
GetNumSectorVertices(Sector sector) -> int
```

Returns how many vertex slots belong to `sector`.

Parameters:
- `sector`: Sector whose vertex count should be returned.

Returns:
- Number of sector-local vertex slots in `sector`.


#### GetSectorVertexPos

```C
GetSectorVertexPos(Sector sector, int vertNum) -> Vector
```

Returns the world position of one of the sector's vertices.

Parameters:
- `sector`: Sector whose vertex position should be queried.
- `vertNum`: Zero-based sector-local vertex slot in the sector's vertex list.

Returns:
- World position of the requested sector vertex.


#### GetNumSectorSurfaces

```C
GetNumSectorSurfaces(Sector sector) -> int
```

Returns how many surfaces belong to `sector`.

Parameters:
- `sector`: Sector whose surface count should be returned.

Returns:
- Number of sector-local surfaces in `sector`.


#### GetSectorSurfaceRef

```C
GetSectorSurfaceRef(Sector sector, int surfIdx) -> Surface
```

Returns a surface reference from the sector's surface list.

Parameters:
- `sector`: Sector whose surface list should be queried.
- `surfIdx`: Zero-based sector-local surface slot.

Returns:
- Global surface reference for the requested sector-local surface slot.


#### SyncSector

```C
SyncSector(Sector sector)
```

Synchronizes the sector state after a script-side change.

Parameters:
- `sector`: Sector whose state should be synchronized.


#### FindSectorAtPos

```C
FindSectorAtPos(Vector pos) -> Sector
```

Finds the sector containing a world position.

Parameters:
- `pos`: World position to test.

Returns:
- Sector reference containing `pos`, or `-1` when no sector is found.


#### SetSectorSurfflags

```C
SetSectorSurfflags(Sector sector, int surfFlags)
```

Sets surface flags on every surface in `sector`.

Parameters:
- `sector`: Sector whose surfaces should receive the flags.
- `surfFlags`: Surface-flag bitmask to OR into every surface in the sector. See [Surface Flags](Flags.md#surface-flags).


#### ClearSectorSurfflags

```C
ClearSectorSurfflags(Sector sector, int surfFlags)
```

Clears surface flags on every surface in `sector`.

Parameters:
- `sector`: Sector whose surfaces should have the flags cleared.
- `surfFlags`: Surface-flag bitmask to clear from every surface in the sector. See [Surface Flags](Flags.md#surface-flags).
