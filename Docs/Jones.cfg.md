# Jones.cfg Reference

`Jones.cfg` is the JSON configuration file used by OpenJones3D.

The engine creates it automatically on first launch and keeps it auto-saved while the game is running. Missing keys are written out on demand, so the file gradually fills itself with defaults and the values chosen in the in-game dialogs.

## Location

`Jones.cfg` is opened with a relative path from the game process, so it is created next to the executable in the current working directory.

For the normal release layout, that means:

- place the release files next to `Indy3D.exe` in the game's `Resource` directory
- launch `Jones3D.exe`
- expect `Jones.cfg` to appear in that same directory

## Format and behavior

- The file format is JSON.
- The engine writes a top-level `"version"` key when it creates a new file.
- Many legacy settings are mapped from the old Windows Registry names into JSON keys.
- Most display, control, and gameplay settings can still be changed from the built-in dialogs.
- Some keys are engine-managed and may be rewritten automatically after you change settings in-game.

Defaults below assume the standard build with `JONES3D_QOL_IMPROVEMENTS=ON` unless stated otherwise.

## Example

```json
{
  "version": "1.0.0",
  "graphics": {
    "window": false,
    "width": 1280,
    "height": 720,
    "bpp": 32,
    "refreshRate": 60,
    "mipmapFilter": 2,
    "mipmapAutoGen": true,
    "anisotropicFilter": true,
    "msaa": {
      "enabled": true,
      "samples": 4
    }
  },
  "engine": {
    "renderer": {
      "hipoly": true,
      "fog": {
        "enabled": true,
        "density": 1.0
      }
    }
  },
  "sound": {
    "volume": 1.0,
    "swmixer": {
      "falloff": 2
    }
  },
  "gameplay": {
    "defaultRun": false,
    "bloodSplatter": true
  }
}
```

## Enum values

### `startMode`

| Value | Meaning |
| --- | --- |
| `0` | Start game |
| `1` | Load game |
| `2` | Developer dialog |
| `3` | Sound settings |
| `4` | Display settings |

### `log.mode`

| Value | Meaning |
| --- | --- |
| `0` | Disabled |
| `1` | Console |
| `2` | Log file |

### `log.level`

| Value | Meaning |
| --- | --- |
| `0` | Errors only |
| `1` | Normal |
| `2` | Verbose |

### `graphics.mipmapFilter`

| Value | Meaning |
| --- | --- |
| `0` | None |
| `1` | Bilinear |
| `2` | Trilinear |

### `engine.renderer.geometry`

| Value | Meaning |
| --- | --- |
| `0` | None |
| `1` | Vertex |
| `2` | Wireframe |
| `3` | Solid |
| `4` | Full |

### `engine.renderer.lighting`

| Value | Meaning |
| --- | --- |
| `0` | None |
| `1` | Lit |
| `2` | Diffuse |
| `3` | Gouraud |

### `engine.renderer.culling.culledSectorTraversal.mode`

Controls how the renderer walks neighboring sectors outside the current camera-frustum-visible sector set, in order to collect additional things and lights that can still affect what is on screen.

| Value | Meaning | Notes |
| --- | --- | --- |
| `0` | Legacy DFS-like traversal | Original order-dependent single-path sector walk. Can miss sectors reachable through a different or shorter path. |
| `1` | BFS traversal | Level-by-level multi-path traversal. Gives more reliable thing and light collection, but in practice it should be paired with `engine.renderer.culling.culledSectorTraversal.maxThingCollectDistance = 18.0` (180 m). Lower values can cull sectors too aggressively and make sectors in front disappear, for example in the Lagoon level. |

### `sound.swmixer.falloff`

Controls how software-mixed positional sounds lose volume between their minimum and maximum radius.

| Value | Meaning | Notes |
| --- | --- | --- |
| `0` | Linear | Original falloff curve. Volume drops evenly between the minimum and maximum radius. |
| `1` | Exponential | Steeper than linear. Sounds lose volume faster as distance increases. |
| `2` | Logarithmic | Decibel-style falloff. Sounds carry a bit farther before fading to very low volume near the maximum radius. |

## Core and startup options

| Key | Type | Default | Notes |
| --- | --- | --- | --- |
| `version` | string | `1.0.0` | Written automatically when a new config file is created. |
| `installPath` | string | empty | Installation path. Usually engine-managed. |
| `sourcePath` | string | empty | Source/CD path used when the game requests original data. |
| `devMode` | bool | `false` | Enables developer mode features. |
| `startMode` | int | `2` | Uses the `startMode` enum above. |
| `log.mode` | int | `0` | Uses the `log.mode` enum above. |
| `log.level` | int | `1` | Uses the `log.level` enum above. |

## Controls and gameplay options

| Key | Type | Default | Notes |
| --- | --- | --- | --- |
| `controls.configFile` | string | empty | Selected input preset name. Usually set by the control dialog. |
| `controls.mouse` | bool | `false` | Enables mouse control. |
| `controls.controller` | bool | `false` | Enables joystick/controller control. |
| `gameplay.defaultRun` | bool | `false` | Makes running the default movement mode. |
| `gameplay.startLevel` | string | empty | Developer/startup override. |
| `gameplay.difficulty` | int | `5` | Difficulty slider value used by the game. |
| `gameplay.showText` | bool | `false` | Shows spoken subtitle text. |
| `gameplay.showHints` | bool | `false` | Enables overlay-map hints. |
| `gameplay.mapRotation` | bool | `false` | Rotates the overlay map with player facing. |
| `gameplay.lastSaveGame` | string | auto | Last save filename used by the save/load flow. Engine-managed. |
| `gameplay.bloodSplatter` | bool | `true` | Enables the hit blood-splatter effect. |

## Graphics and display options

| Key | Type | Default | Notes |
| --- | --- | --- | --- |
| `graphics.allDevices` | bool | `false` | Show all display devices, including less-preferred ones. |
| `graphics.window` | bool | `false` | Windowed mode toggle. |
| `graphics.dualMonitor` | bool | `false` | Dual-monitor window mode toggle. |
| `graphics.buffering` | bool | `false` | Buffered display mode toggle. |
| `graphics.mipmapFilter` | int | `2` | Uses the `graphics.mipmapFilter` enum above. |
| `graphics.display` | string | auto | Selected display adapter name. Usually engine-managed. |
| `graphics.device` | string | auto | Selected 3D device name. Usually engine-managed. |
| `graphics.width` | int | `640` | Requested render width. |
| `graphics.height` | int | `480` | Requested render height. |
| `graphics.bpp` | int | `32` | Requested color depth. |
| `graphics.refreshRate` | int | `60` | Requested refresh rate. |
| `graphics.performanceLevel` | int | `4` | Performance preset used by the display dialog. |
| `graphics.mipmapAutoGen` | bool | `true` | DirectX 9 only. Disabled automatically if unsupported by the device. |
| `graphics.anisotropicFilter` | bool | `true` | DirectX 9 only. Disabled automatically if unsupported by the device. |
| `graphics.msaa.enabled` | bool | `true` | DirectX 9 only. |
| `graphics.msaa.samples` | int | `16` | DirectX 9 only. Clamped to `2`, `4`, `8`, or `16`, with runtime fallback if unsupported. |

## Renderer, world, and engine options

| Key | Type | Default | Notes |
| --- | --- | --- | --- |
| `engine.renderer.hipoly` | bool | `true` | Enables the original high-poly models by default. |
| `engine.renderer.geometry` | int | `4` | Uses the `engine.renderer.geometry` enum above. |
| `engine.renderer.lighting` | int | `3` | Uses the `engine.renderer.lighting` enum above. |
| `engine.renderer.fog.enabled` | bool | `true` | Global fog toggle. |
| `engine.renderer.fog.density` | float | `1.0` | Global fog density multiplier. |
| `engine.renderer.culling.pvs.enabled` | bool | `true` | Enables PVS sector culling. |
| `engine.renderer.culling.culledSectorTraversal.mode` | int | `1` | Default build uses BFS. Legacy/QOL-off builds use `0`. |
| `engine.renderer.culling.culledSectorTraversal.maxThingCollectDistance` | float | `18.0` | Default build value. Legacy/QOL-off builds use `8.0`. |
| `engine.renderer.culling.culledSectorTraversal.maxLightCollectDistance` | float | `18.0` | Default build value. Legacy/QOL-off builds use `8.0`. |
| `engine.renderer.vfx.weapon.fireFlash.enabled` | bool | `true` | Default build value. Legacy/QOL-off builds use `false`. |
| `engine.renderer.vfx.weapon.fireFlash.ambientThreshold` | float | `0.65` | Ambient-light threshold for projectile fire flash effects. |
| `engine.physics.fixedTimestep` | float | `150.0` | QOL builds only. Expressed as FPS. |
| `engine.world.static.materials.extraCapacity` | int | `32` | Extra buffer space for static world materials. |
| `engine.world.cnd.materials.loadExternal` | bool | `true` | If enabled, the engine first tries to load a matching external material from `mat\\<name>` instead of using the material data embedded in the CND. Falls back to the CND copy if no external file is found or loading fails. |
| `engine.world.cnd.keyframes.loadExternal` | bool | `true` | If enabled, the engine first tries to load a matching external keyframe from `3do\\key\\<name>` instead of using the keyframe data embedded in the CND. Falls back to the CND copy if no external file is found or loading fails. |
| `engine.world.normal.things.extraCapacity` | int | `256` | QOL builds only. Extra buffer space for normal world things. |
| `engine.world.static.sprites.extraCapacity` | int | `128` | QOL builds only. Extra buffer space for static world sprites. |

For these CND `*.loadExternal` options, `external` means loose files on disk that override the matching data packed into the world CND. This is mainly useful for modding, testing, or replacing materials and animation keyframes without rebuilding the CND itself.

## Audio options

| Key | Type | Default | Notes |
| --- | --- | --- | --- |
| `sound.hw` | bool | `false` | Enables hardware 3D sound when available. |
| `sound.reverse` | bool | `false` | Reverses left/right sound channels. |
| `sound.volume` | float | `1.0` | Master sound volume. |
| `sound.swmixer.falloff` | int | `2` | Default build uses logarithmic falloff. Legacy/QOL-off builds use `0`. |

## Reticle options

| Key | Type | Default | Notes |
| --- | --- | --- | --- |
| `ui.hud.reticle.enabled` | bool | `true` | Master auto-aim reticle toggle. |
| `ui.hud.reticle.radius` | float | `50.0` | Base reticle radius. |
| `ui.hud.reticle.arrowSize` | float | `11.0` | Arrow size. |
| `ui.hud.reticle.arrowWidthScale` | float | `2.5` | Arrow width multiplier. |
| `ui.hud.reticle.lockScale` | float | `2.5` | Start scale for lock-in animation. |
| `ui.hud.reticle.lockAnimationDuration` | float | `0.3` | Lock animation duration in seconds. |
| `ui.hud.reticle.unlockScale` | float | `1.5` | End scale for unlock animation. |
| `ui.hud.reticle.unlockAnimationDuration` | float | `0.2` | Unlock animation duration in seconds. |
| `ui.hud.reticle.rotationSpeed` | float | `45.0` | Degrees per second. |
| `ui.hud.reticle.pulseMagnitude` | float | `0.45` | Pulse animation amount. |
| `ui.hud.reticle.color` | color string | engine default | Hex color in `#RRGGBBAA` format. |
| `ui.hud.reticle.distanceScaling` | bool | unused | Declared in code but currently not read. |
| `ui.hud.reticle.minDistance` | float | `0.1` | Distance where the reticle is largest. |
| `ui.hud.reticle.maxDistance` | float | `2.18` | Distance where the reticle is smallest. |
| `ui.hud.reticle.minDistanceScale` | float | `2.5` | Scale multiplier at minimum distance. |
| `ui.hud.reticle.maxDistanceScale` | float | `0.45` | Scale multiplier at maximum distance. |

## Menu and HUD options

These are mostly advanced UI tuning keys. The engine writes them out automatically when they are first touched.

| Key | Type | Default | Notes |
| --- | --- | --- | --- |
| `ui.menu.item.scale` | float | `0.6` | Inventory/menu item scale. |
| `ui.menu.item.textPosY` | float | `-13.0` | Default build value. Legacy/QOL-off builds use `70.0`. |
| `ui.menu.item.textSize` | float | `11.0` | Default build value. Legacy/QOL-off builds use `14.0`. |
| `ui.menu.item.font` | string | `mat\\jonesComic Sans MS14.gcf` | Menu item font resource. |
| `ui.menu.item.moveDuration` | int | `250` | Menu item move duration in milliseconds. |
| `ui.menu.column.height` | float | `0.23` | Default build value. Legacy/QOL-off builds use about `0.195`. |
| `ui.menu.column.spacing` | float | `0.045` | Default build value. Legacy/QOL-off builds use about `0.065`. |
| `ui.menu.column.horizontalSpacing` | float | `0.065` | Horizontal spacing between menu columns. |
| `ui.menu.column.expandDuration` | int | `50` | Default build value. Legacy/QOL-off builds use `100`. |
| `ui.menu.column.collapseDuration` | int | `50` | Default build value. Legacy/QOL-off builds use `100`. |
| `ui.menu.openStartSlideDuration` | int | `100` | Menu open pre-slide duration in milliseconds. |
| `ui.menu.openSlideDuration` | int | `100` | Menu open slide duration in milliseconds. |
| `ui.menu.closeStartSlideDuration` | int | `50` | Menu close pre-slide duration in milliseconds. |
| `ui.menu.closeSlideDuration` | int | `50` | Menu close slide duration in milliseconds. |
| `ui.menu.music.enabled` | bool | `true` | Inventory/menu ambient music toggle. |
| `ui.menu.music.music` | string | `mus_gen_maploadloop.wav` | Menu music filename. Exact key name is legacy. |
| `ui.menu.music.file` | float | `0.55` | Menu music volume. Exact key name is legacy. |
| `ui.menu.music.fade` | float | `0.75` | Menu music fade duration in seconds. |
| `ui.hud.itemChanged.showDuration` | int | `2000` | Item-change popup duration in milliseconds. |
| `ui.hud.itemChanged.slideEaseInRate` | float | `4000.0` | Slide ease-in rate. |
| `ui.hud.itemChanged.slideSpeedRate` | int | `3` | Slide speed multiplier. |
| `ui.hud.indicators.health.size` | float | `60.0` | Health indicator size. |
| `ui.hud.indicators.health.posX` | float | `0.041379310` | Health indicator X position. |
| `ui.hud.indicators.health.posY` | float | `0.942857142` | Health indicator Y position. |
| `ui.hud.indicators.health.fade` | float | `0.65` | Default build value. Legacy/QOL-off builds use `0.2`. |
| `ui.hud.indicators.health.fadeTime` | int | `3000` | Health indicator fade time in milliseconds. |
| `ui.hud.indicators.health.textures.base` | string | `hud_health_base.mat` | Base health indicator material. |
| `ui.hud.indicators.health.textures.overlay` | string | `hud_health_overlay.mat` | Health overlay material. |
| `ui.hud.indicators.health.textures.overlayHit` | string | `hud_health_overlay_hit.mat` | Hit overlay material. |
| `ui.hud.indicators.health.textures.overlayPoisoned` | string | `hud_health_overlay_skull.mat` | Poisoned overlay material. |
| `ui.hud.indicators.health.colors.full` | color string | engine default | Hex RGB color serialized by the engine. |
| `ui.hud.indicators.health.colors.medium` | color string | engine default | Hex RGB color serialized by the engine. |
| `ui.hud.indicators.health.colors.low` | color string | engine default | Hex RGB color serialized by the engine. |
| `ui.hud.indicators.health.colors.critical` | color string | engine default | Hex RGB color serialized by the engine. |
| `ui.hud.indicators.endurance.size` | float | `32.0` | Default build value. Legacy/QOL-off builds use `60.0`. |
| `ui.hud.indicators.endurance.posX` | float | `0.102` | Default build value. Legacy/QOL-off builds use about `0.9586206896551`. |
| `ui.hud.indicators.endurance.posY` | float | `0.95` | Default build value. Legacy/QOL-off builds use about `0.942857142`. |
| `ui.hud.indicators.endurance.fade` | float | `0.70` | Default build value. Legacy/QOL-off builds use `0.2`. |
| `ui.hud.indicators.endurance.fadeTime` | int | `3000` | Endurance indicator fade time in milliseconds. |
| `ui.hud.indicators.endurance.textures.overlay` | string | `hud_breath_overlay.mat` | Endurance overlay material. |
| `ui.hud.indicators.endurance.colors.oxygen` | color string | engine default | Hex RGB color serialized by the engine. |
| `ui.hud.indicators.endurance.colors.raft` | color string | engine default | Hex RGB color serialized by the engine. |
| `ui.hud.indicators.endurance.colors.impEnergy` | color string | engine default | Hex RGB color serialized by the engine. |

## Notes

- `Jones.cfg` is the correct filename. The engine opens `Jones.cfg`, not `Jones3D.cfg`.
- Keys under `graphics.display`, `graphics.device`, resolution, refresh rate, and several UI sections are often rewritten by the in-game configuration dialogs.
- DirectX 9-only settings such as MSAA, anisotropic filtering, and automatic mipmap generation are ignored by the legacy DirectX 6.1c backend.
- A few defaults depend on whether the build was compiled with `JONES3D_QOL_IMPROVEMENTS=ON`.
