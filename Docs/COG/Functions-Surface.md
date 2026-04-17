# Surface Host Functions

Source: [`Libs/sith/Cog/sithCogFunctionSurface.c`](../../Libs/sith/Cog/sithCogFunctionSurface.c)

These verbs operate on adjoins, materials, cels, scrolling, face flags, and per-surface lighting.

## Compatibility Notes

- [`SlideWall()`](#slidewall) and [`SlideSurface()`](#slidesurface) are the same implementation.
- [`GetWallCel()`](#getwallcel) / [`SetWallCel()`](#setwallcel) and [`GetSurfaceCel()`](#getsurfacecel) / [`SetSurfaceCel()`](#setsurfacecel) are paired aliases.
- [`SetHorizonSkyOffset()`](#sethorizonskyoffset) and [`SetCeilingSkyOffset()`](#setceilingskyoffset) only use the X and Y components of the provided vector.
- [`GetAdjoinAlpha()`](#getadjoinalpha) and [`SetAdjoinAlpha()`](#setadjoinalpha) expose alpha in a `0..255` style scale even though the renderer stores `0..1`.
- [`SurfaceLightAnim()`](#surfacelightanim) has a TODO in the current source noting that the color-component order may still be wrong.
- Signatures below use PascalCase for readability, even though host-function lookup in scripts is case-insensitive.

## Function Index

### Functions

- [GetSurfaceAdjoin](#getsurfaceadjoin)
- [GetSurfaceSector](#getsurfacesector)
- [GetNumSurfaceVertices](#getnumsurfacevertices)
- [GetSurfaceVertexPos](#getsurfacevertexpos)
- [SetHorizonSkyOffset](#sethorizonskyoffset)
- [GetHorizonSkyOffset](#gethorizonskyoffset)
- [SetCeilingSkyOffset](#setceilingskyoffset)
- [GetCeilingSkyOffset](#getceilingskyoffset)
- [SlideHorizonSky](#slidehorizonsky)
- [SlideCeilingSky](#slideceilingsky)
- [GetSurfaceCount](#getsurfacecount)
- [SlideWall](#slidewall)
- [SlideSurface](#slidesurface)
- [GetWallCel](#getwallcel)
- [SetWallCel](#setwallcel)
- [GetSurfaceCel](#getsurfacecel)
- [SetSurfaceCel](#setsurfacecel)
- [GetSurfaceMat](#getsurfacemat)
- [SetSurfaceMat](#setsurfacemat)
- [GetSurfaceFlags](#getsurfaceflags)
- [SetSurfaceFlags](#setsurfaceflags)
- [ClearSurfaceFlags](#clearsurfaceflags)
- [GetAdjoinFlags](#getadjoinflags)
- [SetAdjoinFlags](#setadjoinflags)
- [ClearAdjoinFlags](#clearadjoinflags)
- [SetFaceType](#setfacetype)
- [ClearFaceType](#clearfacetype)
- [GetFaceType](#getfacetype)
- [SetFaceGeoMode](#setfacegeomode)
- [GetFaceGeoMode](#getfacegeomode)
- [SetFaceLightMode](#setfacelightmode)
- [GetFaceLightMode](#getfacelightmode)
- [GetSurfaceLight](#getsurfacelight)
- [SetSurfaceLight](#setsurfacelight)
- [GetSurfaceCenter](#getsurfacecenter)
- [SurfaceLightAnim](#surfacelightanim)
- [GetSurfaceNormal](#getsurfacenormal)
- [SyncSurface](#syncsurface)
- [GetAdjoinAlpha](#getadjoinalpha)
- [SetAdjoinAlpha](#setadjoinalpha)

## Function Reference

### Functions


#### GetSurfaceAdjoin

```C++
GetSurfaceAdjoin(Surface surf) -> Surface
```

Returns the mirrored adjoining surface connected to `surf`.

Parameters:
- `surf`: Surface whose adjoining surface should be returned.

Returns:
- Mirrored adjoining surface reference, or `-1` when no valid adjoining surface exists.


#### GetSurfaceSector

```C++
GetSurfaceSector(Surface surf) -> Sector
```

Returns the sector that owns `surf`.

Parameters:
- `surf`: Surface whose owning sector should be returned.

Returns:
- Sector reference for the owning sector, or `-1` when the surface has no sector.


#### GetNumSurfaceVertices

```C++
GetNumSurfaceVertices(Surface surf) -> int
```

Returns the number of vertices in the surface polygon.

Parameters:
- `surf`: Surface whose vertex count should be returned.

Returns:
- Number of vertices in the surface polygon.


#### GetSurfaceVertexPos

```C++
GetSurfaceVertexPos(Surface surf, int vertNum) -> Vector
```

Returns the world position of one of the surface's vertices.

Parameters:
- `surf`: Surface whose vertex position should be queried.
- `vertNum`: Zero-based vertex slot inside the surface's polygon.

Returns:
- World position of the requested surface vertex.


#### SetHorizonSkyOffset

```C++
SetHorizonSkyOffset(Vector offset)
```

Sets the horizon sky texture offset for the current world.

Parameters:
- `offset`: Sky offset vector. Only the `x` and `y` components are stored; `z` is ignored.


#### GetHorizonSkyOffset

```C++
GetHorizonSkyOffset() -> Vector
```

Returns the current horizon sky texture offset.

Returns:
- Current horizon sky offset. The returned `z` component is always `0`.


#### SetCeilingSkyOffset

```C++
SetCeilingSkyOffset(Vector offset)
```

Sets the ceiling sky texture offset for the current world.

Parameters:
- `offset`: Sky offset vector. Only the `x` and `y` components are stored; `z` is ignored.


#### GetCeilingSkyOffset

```C++
GetCeilingSkyOffset() -> Vector
```

Returns the current ceiling sky texture offset.

Returns:
- Current ceiling sky offset. The returned `z` component is always `0`.


#### SlideHorizonSky

```C++
SlideHorizonSky(float x, float y) -> int
```

Starts a scrolling animation for the horizon sky.

Parameters:
- `x`: Horizontal scroll component for the sky animation.
- `y`: Vertical scroll component for the sky animation.

Returns:
- Animation id on success, otherwise `-1`.


#### SlideCeilingSky

```C++
SlideCeilingSky(float x, float y) -> int
```

Starts a scrolling animation for the ceiling sky.

Parameters:
- `x`: Horizontal scroll component for the sky animation.
- `y`: Vertical scroll component for the sky animation.

Returns:
- Animation id on success, otherwise `-1`.


#### GetSurfaceCount

```C++
GetSurfaceCount() -> int
```

Returns the total number of surfaces in the current world.

Returns:
- Total number of surfaces in the current world.


#### SlideWall

```C++
SlideWall(Surface surf, Vector dir, float speed) -> int
```

Starts a material-scrolling animation on `surf`.

Parameters:
- `surf`: Surface whose material should be scrolled.
- `dir`: Scroll direction vector. The engine scales this vector by `speed / 10.0` before starting the animation.
- `speed`: Material-scroll speed parameter. This value is divided by `10` and applied to `dir` before the animation starts.

Returns:
- Animation id on success, otherwise `-1`.

Notes:
- Requires the surface to have a material assigned.


#### SlideSurface

```C++
SlideSurface(Surface surf, Vector dir, float speed) -> int
```

Alias of [`SlideWall()`](#slidewall).

Parameters:
- `surf`: Surface whose material should be scrolled.
- `dir`: Scroll direction vector. The engine scales this vector by `speed / 10.0` before starting the animation.
- `speed`: Material-scroll speed parameter. This value is divided by `10` and applied to `dir` before the animation starts.

Returns:
- Animation id on success, otherwise `-1`.

Notes:
- Shares the same implementation as [`SlideWall()`](#slidewall).


#### GetWallCel

```C++
GetWallCel(Surface surf) -> int
```

Returns the current material cel index for `surf`.

Parameters:
- `surf`: Surface whose current material cel index should be returned.

Returns:
- Current material cel index for `surf`, or `-1` when the surface has no material.


#### SetWallCel

```C++
SetWallCel(Surface surf, int celNum) -> int
```

Sets the material cel index for `surf`.

Parameters:
- `surf`: Surface whose material cel index should be changed.
- `celNum`: New cel index to assign. The current implementation accepts `-1` or any value below the material's `numCels`.

Returns:
- Previous material cel index, or `-1` when the request is rejected.

Notes:
- Requires the surface to have a material assigned.


#### GetSurfaceCel

```C++
GetSurfaceCel(Surface surf) -> int
```

Returns the current material cel index for `surf`.

Parameters:
- `surf`: Surface whose current material cel index should be returned.

Returns:
- Current material cel index for `surf`, or `-1` when the surface has no material.

Notes:
- Shares the same implementation as [`GetWallCel()`](#getwallcel).


#### SetSurfaceCel

```C++
SetSurfaceCel(Surface surf, int celNum) -> int
```

Sets the material cel index for `surf`.

Parameters:
- `surf`: Surface whose material cel index should be changed.
- `celNum`: New cel index to assign. The current implementation accepts `-1` or any value below the material's `numCels`.

Returns:
- Previous material cel index, or `-1` when the request is rejected.

Notes:
- Shares the same implementation as [`SetWallCel()`](#setwallcel).


#### GetSurfaceMat

```C++
GetSurfaceMat(Surface surf) -> Material
```

Returns the material currently assigned to `surf`.

Parameters:
- `surf`: Surface whose material should be returned.

Returns:
- Current material assigned to `surf`, or `-1` when no material is assigned.


#### SetSurfaceMat

```C++
SetSurfaceMat(Surface surf, Material mat) -> Material
```

Replaces the material assigned to `surf`.

Parameters:
- `surf`: Surface whose material should be changed.
- `mat`: Material to assign to the surface.

Returns:
- Previously assigned material, or `-1` when the surface had no material before the change.


#### GetSurfaceFlags

```C++
GetSurfaceFlags(Surface surf) -> int
```

Returns the current flags of the `surface`.

Parameters:
- `surf`: Surface whose flag bitmask should be returned.

Returns:
- Current surface-flag bitmask. See [Surface Flags](Flags.md#surface-flags).


#### SetSurfaceFlags

```C++
SetSurfaceFlags(Surface surf, int surfaceFlags)
```

Sets the flags of the `surface`.

Parameters:
- `surf`: Surface whose flag bitmask should be updated.
- `surfaceFlags`: Surface-flag bitmask to OR into the current surface flags. See [Surface Flags](Flags.md#surface-flags).


#### ClearSurfaceFlags

```C++
ClearSurfaceFlags(Surface surf, int surfaceFlags)
```

Clears the specified flags on the `surface`.

Parameters:
- `surf`: Surface whose flag bitmask should be updated.
- `surfaceFlags`: Surface-flag bitmask to clear from the current surface flags. See [Surface Flags](Flags.md#surface-flags).


#### GetAdjoinFlags

```C++
GetAdjoinFlags(Surface surf) -> int
```

Returns the current flags of the `adjoin`.

Parameters:
- `surf`: Surface whose adjoin flag bitmask should be returned.

Returns:
- Current adjoin-flag bitmask, or `-1` when the surface has no adjoin. See [Adjoin Flags](Flags.md#adjoin-flags).


#### SetAdjoinFlags

```C++
SetAdjoinFlags(Surface surf, int adjoinFlags)
```

Sets the flags of the `adjoin`.

Parameters:
- `surf`: Surface whose adjoin flags should be updated.
- `adjoinFlags`: Adjoin-flag bitmask to OR into the current adjoin flags. See [Adjoin Flags](Flags.md#adjoin-flags).


#### ClearAdjoinFlags

```C++
ClearAdjoinFlags(Surface surf, int adjoinFlags)
```

Clears the specified flags on the `adjoin`.

Parameters:
- `surf`: Surface whose adjoin flags should be updated.
- `adjoinFlags`: Adjoin-flag bitmask to clear from the current adjoin flags. See [Adjoin Flags](Flags.md#adjoin-flags).


#### SetFaceType

```C++
SetFaceType(Surface surf, int faceFlags)
```

Sets the type of the `face`.

Parameters:
- `surf`: Surface whose face flag bitmask should be updated.
- `faceFlags`: Face-flag bitmask to OR into the current face flags. See [Face Flags](Flags.md#face-flags).


#### ClearFaceType

```C++
ClearFaceType(Surface surf, int faceFlags)
```

Clears the specified type on the `face`.

Parameters:
- `surf`: Surface whose face flag bitmask should be updated.
- `faceFlags`: Face-flag bitmask to clear from the current face flags. See [Face Flags](Flags.md#face-flags).


#### GetFaceType

```C++
GetFaceType(Surface surf) -> int
```

Returns the current type of the `face`.

Parameters:
- `surf`: Surface whose face flag bitmask should be returned.

Returns:
- Current face-flag bitmask. See [Face Flags](Flags.md#face-flags).


#### SetFaceGeoMode

```C++
SetFaceGeoMode(Surface surf, int geoMode)
```

Sets the geo mode of the `face`.

Parameters:
- `surf`: Surface whose face geometry mode should be updated.
- `geoMode`: New geometry-mode value to store on the face. See [Render Geometry Modes](Types-And-Modes.md#render-geometry-modes). If the surface has no material, the engine forces `RD_GEOMETRY_NONE`.


#### GetFaceGeoMode

```C++
GetFaceGeoMode(Surface surf) -> int
```

Returns the current geo mode of the `face`.

Parameters:
- `surf`: Surface whose face geometry mode should be returned.

Returns:
- Current face geometry-mode value. See [Render Geometry Modes](Types-And-Modes.md#render-geometry-modes).


#### SetFaceLightMode

```C++
SetFaceLightMode(Surface surf, int lightMode)
```

Sets the light mode of the `face`.

Parameters:
- `surf`: Surface whose face lighting mode should be updated.
- `lightMode`: New lighting-mode value to store on the face. See [Render Light Modes](Types-And-Modes.md#render-light-modes).


#### GetFaceLightMode

```C++
GetFaceLightMode(Surface surf) -> int
```

Returns the current light mode of the `face`.

Parameters:
- `surf`: Surface whose face lighting mode should be returned.

Returns:
- Current face lighting-mode value. See [Render Light Modes](Types-And-Modes.md#render-light-modes).


#### GetSurfaceLight

```C++
GetSurfaceLight(Surface surf) -> Vector
```

Returns the current extra-light color stored on the surface face.

Parameters:
- `surf`: Surface whose extra-light color should be returned.

Returns:
- Current extra-light RGB color for the surface.


#### SetSurfaceLight

```C++
SetSurfaceLight(Surface surf, Vector color, float timeDelta) -> int
```

Sets or animates the surface extra-light color.

Parameters:
- `surf`: Surface whose extra-light color should be updated.
- `color`: Target extra-light RGB color for the surface.
- `timeDelta`: Transition time in seconds. `0` applies the color immediately; non-zero starts a surface light animation.

Returns:
- Animation id when an animated light transition is started, `-1` on failure. In the immediate `timeDelta == 0` path, the current wrapper does not push a meaningful return value.


#### GetSurfaceCenter

```C++
GetSurfaceCenter(Surface surf) -> Vector
```

Returns the geometric center point of `surf`.

Parameters:
- `surf`: Surface whose center point should be returned.

Returns:
- Current center point of the `surface`.


#### SurfaceLightAnim

```C++
SurfaceLightAnim(Surface surf, float startR, float startG, float startB, float endR, float endG, float endB, float speed) -> int
```

Animates the extra light color on `surf` from the starting RGB values to the ending RGB values.

Parameters:
- `surf`: Surface whose extra light color should be animated.
- `startR`: Starting red component.
- `startG`: Starting green component.
- `startB`: Starting blue component.
- `endR`: Ending red component.
- `endG`: Ending green component.
- `endB`: Ending blue component.
- `speed`: Light-color transition timing parameter. This value is multiplied by `0.5`, so the animation duration is half of the passed `speed` value.

Returns:
- Animation id on success, or `-1` on invalid input or startup failure.


#### GetSurfaceNormal

```C++
GetSurfaceNormal(Surface surf) -> Vector
```

Returns the face normal of `surf`.

Parameters:
- `surf`: Surface whose face normal should be returned.

Returns:
- Current face normal of the surface.


#### SyncSurface

```C++
SyncSurface(Surface surf)
```

Synchronizes the surface state after a script-side change.

Parameters:
- `surf`: Surface whose state should be synchronized.


#### GetAdjoinAlpha

```C++
GetAdjoinAlpha(Surface surf) -> float
```

Returns the current alpha value of the `adjoin`.

Parameters:
- `surf`: Surface whose adjoin alpha should be returned.

Returns:
- Current adjoin alpha value in `0..255` script scale, or `-1` when the surface is invalid.


#### SetAdjoinAlpha

```C++
SetAdjoinAlpha(Surface surf, float alpha255)
```

Sets the alpha value of the `adjoin`.

Parameters:
- `surf`: Surface whose adjoin alpha should be updated.
- `alpha255`: Alpha value in `0..255` script scale. The engine converts this to `0..1`, writes it into every vertex intensity alpha, and toggles translucent rendering when the value is below full opacity.
