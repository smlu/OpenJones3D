# COG Flag Reference

This page gathers the main flag and bitmask sets referenced by the COG scripting docs.

- Flag names and values come from the current OpenJones3D source tree.
- `UNKNOWN_*` entries are preserved as-is when the current codebase does not yet reveal a better meaning.
- Some COG verbs read or write raw masks with no named enum in the current tree. Those cases are called out explicitly.

## Quick Links

- [COG Script Flags](#cog-script-flags)
- [Animation Flags](#animation-flags)
- [Debug Mode Flags](#debug-mode-flags)
- [Map Mode Flags](#map-mode-flags)
- [Game Submode Flags](#game-submode-flags)
- [Camera State Flags](#camera-state-flags)
- [Thing Flags](#thing-flags)
- [Sector Flags](#sector-flags)
- [Surface Flags](#surface-flags)
- [Adjoin Flags](#adjoin-flags)
- [Face Flags](#face-flags)
- [Physics Flags](#physics-flags)
- [Attach Flags](#attach-flags)
- [Inventory Type Flags](#inventory-type-flags)
- [Goal Entry Masks And Respawn Masks](#goal-entry-masks-and-respawn-masks)
- [Damage Flags](#damage-flags)
- [Shared Type-Specific Flag Field](#shared-type-specific-flag-field)
- [Explosion Flags](#explosion-flags)
- [Actor Flags](#actor-flags)
- [Weapon Flags](#weapon-flags)
- [Particle Flags](#particle-flags)
- [Item Flags](#item-flags)
- [AI Mode Flags](#ai-mode-flags)
- [AI Submode Flags](#ai-submode-flags)
- [AI Waypoint Layer Flags](#ai-waypoint-layer-flags)
- [Hint Flags](#hint-flags)
- [Projectile Fire Flags](#projectile-fire-flags)
- [Keyframe Flags](#keyframe-flags)
- [Sound Play Flags](#sound-play-flags)
- [Actor Special-Move Flags](#actor-special-move-flags)

## COG Script Flags

Used by:
- the `flags=0x...` line documented in [Language](Language.md)
- sync-related notes throughout [Thing host functions](Functions-Thing.md)

Stored in:
- `SithCog.flags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHCOG_DEBUG` | `0x001` | Enables extra runtime logging from the COG layer. |
| `SITHCOG_DISABLED` | `0x002` | Prevents the cog from handling messages until re-enabled. |
| `SITHCOG_PULSE_SET` | `0x004` | Internal runtime bit used while a pulse is active. |
| `SITHCOG_TIMER_SET` | `0x008` | Internal runtime bit used while a one-shot timer is active. |
| `SITHCOG_PAUSED` | `0x010` | Prevents message execution while the cog is paused. |
| `SITHCOG_CLASS` | `0x020` | Marks a class or template-style cog rather than a placed runtime cog instance. |
| `SITHCOG_LOCAL` | `0x040` | Prevents normal remote forwarding for multiplayer message sends. |
| `SITHCOG_SERVER` | `0x080` | Server-oriented compatibility flag preserved from the original system. |
| `SITHCOG_GLOBAL` | `0x100` | Global-scope compatibility flag preserved on the script/cog. |
| `SITHCOG_NOSYNC` | `0x200` | Suppresses many gameplay sync sends triggered by mutating host functions. |

## Animation Flags

Used by:
- [MaterialAnim](Functions-System.md#materialanim)
- [SurfaceAnim](Functions-System.md#surfaceanim)

Stored in:
- `SithAnimationSlot.flags`

Notes:
- The low bits control looping and start/finish behavior.
- The high bits identify what kind of animation slot was created. For the COG material and surface verbs, those object-type bits are normally set by the engine-side start helper.

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHANIMATE_LOOP` | `0x00000001` | Loop the animation instead of stopping at the end. |
| `SITHANIMATE_START_SECOND_FRAME` | `0x00000002` | Start on the second frame/cel. |
| `SITHANIMATE_START_THIRD_FRAME` | `0x00000004` | Start on the third frame/cel. |
| `SITHANIMATE_FINISH_FIRST_FRAME` | `0x00000008` | Return to the first frame/cel when finishing. |
| `SITHANIMATE_PUSHITEM` | `0x00000010` | Push-item behavior flag used by the animation system. |
| `SITHANIMATE_PULLITEM` | `0x00000020` | Pull-item behavior flag used by the animation system. |
| `SITHANIMATE_NOSYNC` | `0x00000040` | OpenJones3D-added flag that keeps the animation out of sync/save propagation. |
| `SITHANIMATE_MATERIAL` | `0x00010000` | Animation slot is a material animation. |
| `SITHANIMATE_SURFACE` | `0x00020000` | Animation slot is a surface animation. |
| `SITHANIMATE_THING` | `0x00040000` | Animation slot is a thing animation. |
| `SITHANIMATE_SPRITE` | `0x00080000` | Animation slot is a sprite animation. |
| `SITHANIMATE_SCROLL` | `0x00100000` | Animation slot performs scrolling. |
| `SITHANIMATE_PAGEFLIP` | `0x00200000` | Animation slot performs page flipping. |
| `SITHANIMATE_LIGHT` | `0x00400000` | Animation slot animates light values. |
| `SITHANIMATE_SKYHORIZON` | `0x00800000` | Animation slot targets horizon sky data. |
| `SITHANIMATE_SKYCEILING` | `0x01000000` | Animation slot targets ceiling sky data. |
| `SITHANIMATE_SECTOR` | `0x02000000` | Animation slot targets sector data. |
| `SITHANIMATE_CAMERA_ZOOM` | `0x04000000` | Animation slot targets camera zoom. |
| `SITHANIMATE_THING_FADE` | `0x08000000` | Animation slot fades a thing. |
| `SITHANIMATE_SPRITE_SIZE` | `0x10000000` | Animation slot animates sprite size. |
| `SITHANIMATE_THING_MOVE` | `0x20000000` | Animation slot moves a thing. |
| `SITHANIMATE_THING_MOVEPOS` | `0x40000000` | Animation slot moves a thing to a target position. |
| `SITHANIMATE_THING_QUICKTURN` | `0x80000000` | Animation slot performs a quick turn. |

## Debug Mode Flags

Used by:
- [InEditor](Functions-System.md#ineditor)
- [SetDebugModeFlags](Functions-System.md#setdebugmodeflags)
- [GetDebugModeFlags](Functions-System.md#getdebugmodeflags)
- [ClearDebugModeFlags](Functions-System.md#cleardebugmodeflags)
- editor/devmode notes in [README](README.md) and [Messages](Messages.md)

Stored in:
- `sithMain_g_sith_mode.debugModeFlags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHDEBUG_AIEVENTS_DISABLED` | `0x01` | Disables AI event processing. |
| `SITHDEBUG_PUPPETSYSTEM_DISABLED` | `0x02` | Disables the puppet system. |
| `SITHDEBUG_UNKNOWN_8` | `0x08` | Undecoded debug flag. |
| `SITHDEBUG_TRACKSHOTS` | `0x40` | Enables projectile/shot tracking debug behavior. |
| `SITHDEBUG_AIDISABLED` | `0x80` | Disables AI updates. |
| `SITHDEBUG_INEDITOR` | `0x100` | Marks the engine as running in editor/debug-editor mode. |
| `SITHDEBUG_AINOTARGET` | `0x200` | Prevents AI target acquisition. |
| `SITHDEBUG_SLOWMODE` | `0x400` | Enables slow-mode behavior. |

## Map Mode Flags

Used by:
- [SetMapModeFlags](Functions-System.md#setmapmodeflags)
- [GetMapModeFlags](Functions-System.md#getmapmodeflags)
- [ClearMapModeFlags](Functions-System.md#clearmapmodeflags)

Stored in:
- `sithMain_g_sith_mode.mapModeFlags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHMAPMODE_SHOWALLSECTORS` | `0x02` | Show all sectors on the map. |
| `SITHMAPMODE_SHOWPLAYERS` | `0x04` | Show players on the map. |
| `SITHMAPMODE_SHOWACTORS` | `0x08` | Show actors on the map. |
| `SITHMAPMODE_SHOWITEMS` | `0x10` | Show items on the map. |
| `SITHMAPMODE_SHOWWEAPONS` | `0x20` | Show weapons on the map. |
| `SITHMAPMODE_SHOWALLTHINGS` | `0x40` | Show all things on the map. |

## Game Submode Flags

Used by:
- [SetSubModeFlags](Functions-System.md#setsubmodeflags)
- [GetSubModeFlags](Functions-System.md#getsubmodeflags)
- [ClearSubModeFlags](Functions-System.md#clearsubmodeflags)

Stored in:
- `sithMain_g_sith_mode.subModeFlags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_SUBMODE_SYNC` | `0x08` | Enables the game's sync-oriented submode bit. |

## Camera State Flags

Used by:
- [SetCameraStateFlags](Functions-System.md#setcamerastateflags)
- [GetCameraStateFlags](Functions-System.md#getcamerastateflags)

Stored in:
- the global camera-state flag field managed by the camera system

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHCAMERA_STATE_CUTSCENE` | `0x1` | Puts the camera system into cutscene mode. |

## Thing Flags

Used by:
- [GetThingFlags](Functions-Thing.md#getthingflags)
- [SetThingFlags](Functions-Thing.md#setthingflags)
- [ClearThingFlags](Functions-Thing.md#clearthingflags)
- notes for [SetThingLight](Functions-Thing.md#setthinglight), [ThingLight](Functions-Thing.md#thinglight), [CaptureThing](Functions-Thing.md#capturething), [ReleaseThing](Functions-Thing.md#releasething), [SetThingPulse](Functions-Thing.md#setthingpulse), and [SetThingTimer](Functions-Thing.md#setthingtimer)

Stored in:
- `thing->flags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_TF_EMITLIGHT` | `0x00000001` | Thing emits dynamic light. |
| `SITH_TF_DESTROYED` | `0x00000002` | Thing is destroyed. |
| `SITH_TF_NOWEAPONCOLLIDE` | `0x00000004` | Ignore weapon collision. |
| `SITH_TF_WHIPCLIMB` | `0x00000008` | Thing supports whip climbing. |
| `SITH_TF_INVISIBLE` | `0x00000010` | Thing is invisible. |
| `SITH_TF_NOSYNC` | `0x00000020` | Do not sync this thing to savegame or newer sync paths. |
| `SITH_TF_STANDON` | `0x00000040` | Thing can be stood on. |
| `SITH_TF_MOUNTABLE` | `0x00000080` | Thing is mountable. |
| `SITH_TF_REMOTE` | `0x00000100` | Thing is remote/network-owned. |
| `SITH_TF_DYING` | `0x00000200` | Thing is in its dying state. |
| `SITH_TF_COGLINKED` | `0x00000400` | Thing is linked to one or more cogs. |
| `SITH_TF_NOCRUSH` | `0x00000800` | Thing is protected from crush damage/logic. |
| `SITH_TF_UNKNOWN_1000` | `0x00001000` | Undecoded thing flag. |
| `SITH_TF_WOOD` | `0x00002000` | Thing uses wood material classification. |
| `SITH_TF_SHADOW` | `0x00004000` | Thing renders/uses a shadow flag. |
| `SITH_TF_JEEPSTOP` | `0x00008000` | Jeep-stop interaction flag. |
| `SITH_TF_SNOW` | `0x00010000` | Thing uses snow material classification. |
| `SITH_TF_PULSESET` | `0x00020000` | Per-thing pulse timer is active. |
| `SITH_TF_TIMERSET` | `0x00040000` | Per-thing one-shot timer is active. |
| `SITH_TF_DISABLED` | `0x00080000` | Thing is disabled. |
| `SITH_TF_SEEN` | `0x00100000` | Thing has been seen/visited by runtime logic. |
| `SITH_TF_UNKNOWN_200000` | `0x00200000` | Undecoded thing flag. |
| `SITH_TF_METAL` | `0x00400000` | Thing uses metal material classification. |
| `SITH_TF_EARTH` | `0x00800000` | Thing uses earth material classification. |
| `SITH_TF_NOSOUND` | `0x01000000` | Thing suppresses sound behavior. |
| `SITH_TF_SUBMERGED` | `0x02000000` | Thing is submerged. |
| `SITH_TF_CLIMBCRATE` | `0x04000000` | Thing behaves as a climb crate. |
| `SITH_TF_WATERDESTROYED` | `0x08000000` | Thing is destroyed by water. |
| `SITH_TF_AIRDESTROYED` | `0x10000000` | Thing is destroyed by air/non-water state. |
| `SITH_TF_SPLASH` | `0x20000000` | Thing uses splash behavior. |
| `SITH_TF_MOVABLE` | `0x40000000` | Thing is movable. |
| `SITH_TF_WHIPSWING` | `0x80000000` | Thing supports whip swinging. |

## Sector Flags

Used by:
- [GetSectorFlags](Functions-Sector.md#getsectorflags)
- [SetSectorFlags](Functions-Sector.md#setsectorflags)
- [ClearSectorFlags](Functions-Sector.md#clearsectorflags)

Stored in:
- `sector->flags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_SECTOR_NOGRAVITY` | `0x0001` | Sector has no gravity. |
| `SITH_SECTOR_UNDERWATER` | `0x0002` | Sector is underwater. |
| `SITH_SECTOR_COGLINKED` | `0x0004` | Sector is linked to one or more cogs. |
| `SITH_SECTOR_USETHRUST` | `0x0008` | Sector thrust is active. |
| `SITH_SECTOR_HIDEONMAP` | `0x0010` | Hide sector on the automap. |
| `SITH_SECTOR_NOACTORENTER` | `0x0020` | Actors are not allowed to enter. |
| `SITH_SECTOR_FALLDEATH` | `0x0040` | Falling into the sector is lethal. |
| `SITH_SECTOR_ADJOINSOFF` | `0x0080` | Sector adjoins are disabled. |
| `SITH_SECTOR_AETHERIUM` | `0x0100` | Sector has Aetherium behavior. |
| `SITH_SECTOR_HASCOLLIDEBOX` | `0x1000` | Sector owns a collide box. |
| `SITH_SECTOR_SEEN` | `0x4000` | Sector has been seen. |
| `SITH_SECTOR_SYNC` | `0x8000` | Sector participates in sync/save handling. |

## Surface Flags

Used by:
- [GetSurfaceFlags](Functions-Surface.md#getsurfaceflags)
- [SetSurfaceFlags](Functions-Surface.md#setsurfaceflags)
- [ClearSurfaceFlags](Functions-Surface.md#clearsurfaceflags)
- [SetSectorSurfflags](Functions-Sector.md#setsectorsurfflags)
- [ClearSectorSurfflags](Functions-Sector.md#clearsectorsurfflags)

Stored in:
- `surface->surfaceFlags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_SURFACE_ISFLOOR` | `0x00000001` | Surface is marked as a floor. |
| `SITH_SURFACE_COGLINKED` | `0x00000002` | Surface is linked to one or more cogs. |
| `SITH_SURFACE_COLLISION` | `0x00000004` | Surface participates in collision. |
| `SITH_SURFACE_NOAIMOVE` | `0x00000008` | AI should not move across the surface. |
| `SITH_SURFACE_DOUBLESIZE` | `0x00000010` | Doubles surface texture scale. |
| `SITH_SURFACE_HALFSIZE` | `0x00000020` | Halves surface texture scale. |
| `SITH_SURFACE_EIGHTSIZE` | `0x00000040` | Uses one-eighth texture scale. |
| `SITH_SURFACE_AETHERIUM` | `0x00000080` | Surface has Aetherium behavior. |
| `SITH_SURFACE_HORIZONSKY` | `0x00000200` | Surface is a horizon sky surface. |
| `SITH_SURFACE_CEILINGSKY` | `0x00000400` | Surface is a ceiling sky surface. |
| `SITH_SURFACE_SCROLLING` | `0x00000800` | Surface is scrolling. |
| `SITH_SURFACE_KILLFLOOR` | `0x00001000` | Surface is lethal as a floor. |
| `SITH_SURFACE_CLIMBABLE` | `0x00002000` | Surface is climbable. |
| `SITH_SURFACE_TRACK` | `0x00004000` | Surface is a track surface. |
| `SITH_SURFACE_SYNC` | `0x00008000` | Surface participates in sync/save handling. |
| `SITH_SURFACE_METAL` | `0x00010000` | Metal material classification. |
| `SITH_SURFACE_WATER` | `0x00020000` | Water material classification. |
| `SITH_SURFACE_SHALLOWWATER` | `0x00040000` | Shallow-water material classification. |
| `SITH_SURFACE_EARTH` | `0x00080000` | Earth material classification. |
| `SITH_SURFACE_WEB` | `0x00100000` | Web material classification. |
| `SITH_SURFACE_LAVA` | `0x00200000` | Lava material classification. |
| `SITH_SURFACE_SNOW` | `0x00400000` | Snow material classification. |
| `SITH_SURFACE_WOOD` | `0x00800000` | Wood material classification. |
| `SITH_SURFACE_LEDGE` | `0x01000000` | Ledge surface classification. |
| `SITH_SURFACE_WATERLEDGE` | `0x02000000` | Water ledge surface classification. |
| `SITH_SURFACE_QUARTERSIZE` | `0x04000000` | Uses quarter texture scale. |
| `SITH_SURFACE_QUADRUPLESIZE` | `0x08000000` | Uses quadruple texture scale. |
| `SITH_SURFACE_WHIPAIM` | `0x10000000` | Surface supports whip aim behavior. |
| `SITH_SURFACE_ECHO` | `0x20000000` | Echo acoustic classification. |
| `SITH_SURFACE_WOODECHO` | `0x40000000` | Wood echo acoustic classification. |
| `SITH_SURFACE_EARTHECHO` | `0x80000000` | Earth echo acoustic classification. |

## Adjoin Flags

Used by:
- [GetAdjoinFlags](Functions-Surface.md#getadjoinflags)
- [SetAdjoinFlags](Functions-Surface.md#setadjoinflags)
- [ClearAdjoinFlags](Functions-Surface.md#clearadjoinflags)

Stored in:
- `surface->adjoin->flags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_ADJOIN_VISIBLE` | `0x01` | Adjoin is visible/open for rendering traversal. |
| `SITH_ADJOIN_MOVE` | `0x02` | Movement through the adjoin is allowed. |
| `SITH_ADJOIN_RESERVED_JKDF_ALLOW_SOUND_PASS` | `0x04` | Legacy JKDF sound-pass flag preserved in the data model. |
| `SITH_ADJOIN_NOAIMOVE` | `0x08` | AI should not move through the adjoin. |
| `SITH_ADJOIN_NOPLAYERMOVE` | `0x10` | Player movement through the adjoin is blocked. |
| `SITH_ADJOIN_SECTORSET` | `0x20` | Adjoin sector data is set/valid. |
| `SITH_ADJOIN_UNKNOWN_80` | `0x80` | Undecoded adjoin flag. |

## Face Flags

Used by:
- [GetFaceType](Functions-Surface.md#getfacetype)
- [SetFaceType](Functions-Surface.md#setfacetype)
- [ClearFaceType](Functions-Surface.md#clearfacetype)

Stored in:
- `surface->face.flags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `RD_FF_DOUBLE_SIDED` | `0x001` | Render both sides of the face. |
| `RD_FF_TEX_TRANSLUCENT` | `0x002` | Face texture is translucent. |
| `RD_FF_TEX_CLAMP_X` | `0x004` | Clamp texture coordinates on the X/U axis. |
| `RD_FF_TEX_CLAMP_Y` | `0x008` | Clamp texture coordinates on the Y/V axis. |
| `RD_FF_TEX_FILTER_NEAREST` | `0x010` | Use nearest filtering for the face texture. |
| `RD_FF_ZWRITE_DISABLED` | `0x020` | Disable Z-buffer writes for the face. |
| `RD_FF_3DO_LEDGE` | `0x040` | Ledge flag preserved from 3DO data. |
| `RD_FF_UNKNOWN_80` | `0x080` | Undecoded face flag. |
| `RD_FF_FOG_ENABLED` | `0x100` | Enable fog on the face. |
| `RD_FF_3DO_WHIP_AIM` | `0x200` | Whip-aim flag preserved from 3DO data. |

## Physics Flags

Used by:
- [GetPhysicsFlags](Functions-Thing.md#getphysicsflags)
- [SetPhysicsFlags](Functions-Thing.md#setphysicsflags)
- [ClearPhysicsFlags](Functions-Thing.md#clearphysicsflags)
- notes in [TeleportThing](Functions-Thing.md#teleportthing) and [SetThingPosEx](Functions-Thing.md#setthingposex)

Stored in:
- `thing->moveInfo.physics.flags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_PF_USEGRAVITY` | `0x00000001` | Apply gravity. |
| `SITH_PF_USETHRUST` | `0x00000002` | Apply thrust. |
| `SITH_PF_UNKNOWN_4` | `0x00000004` | Undecoded physics flag. |
| `SITH_PF_UNKNOWN_8` | `0x00000008` | Undecoded physics flag. |
| `SITH_PF_ALIGNSURFACE` | `0x00000010` | Align to the supporting surface. |
| `SITH_PF_SURFACEBOUNCE` | `0x00000020` | Bounce on surfaces. |
| `SITH_PF_FLOORSTICK` | `0x00000040` | Stick to the detected floor. |
| `SITH_PF_WALLSTICK` | `0x00000080` | Stick to walls. |
| `SITH_PF_ALIGNED` | `0x00000100` | Thing is currently aligned. |
| `SITH_PF_USEROTATIONVELOCITY` | `0x00000200` | Use rotation velocity. |
| `SITH_PF_BANKEDTURN` | `0x00000400` | Use banked turning. |
| `SITH_PF_ALIGNUP` | `0x00000800` | Align up-vector. |
| `SITH_PF_USEANGULARTHRUST` | `0x00001000` | Apply angular thrust. |
| `SITH_PF_FLY` | `0x00002000` | Flying physics behavior. |
| `SITH_PF_USEBLASTFORCE` | `0x00004000` | Respond to blast force. |
| `SITH_PF_FORCEAPPLIED` | `0x00008000` | Force has been applied this frame/state. |
| `SITH_PF_CROUCHING` | `0x00010000` | Thing is crouching. |
| `SITH_PF_STARTORIENTMOVE` | `0x00020000` | Start movement oriented to current facing. |
| `SITH_PF_PARTIALGRAVITY` | `0x00040000` | Use partial gravity. |
| `SITH_PF_UNKNOWN_80000` | `0x00080000` | Undecoded physics flag. |
| `SITH_PF_ONWATERSURFACE` | `0x00100000` | Thing is on the water surface. |
| `SITH_PF_UNKNOWN_200000` | `0x00200000` | Undecoded physics flag. |
| `SITH_PF_NOTHRUST` | `0x00400000` | Disable thrust application. |
| `SITH_PF_NOUPDATE` | `0x00800000` | Skip normal physics updates. |
| `SITH_PF_MINECAR` | `0x01000000` | Minecar physics mode. |
| `SITH_PF_RAFT` | `0x02000000` | Raft physics mode. |
| `SITH_PF_JEEP` | `0x04000000` | Jeep physics mode. |
| `SITH_PF_UNKNOWN_8000000` | `0x08000000` | Undecoded physics flag. |
| `SITH_PF_UNKNOWN_10000000` | `0x10000000` | Undecoded physics flag. |
| `SITH_PF_UNKNOWN_20000000` | `0x20000000` | Undecoded physics flag. |
| `SITH_PF_UNKNOWN_40000000` | `0x40000000` | Undecoded physics flag. |
| `SITH_PF_UNKNOWN_80000000` | `0x80000000` | Undecoded physics flag. |

## Attach Flags

Used by:
- [GetAttachFlags](Functions-Thing.md#getattachflags)
- [GetThingAttachFlags](Functions-Thing.md#getthingattachflags)
- [AttachThingToThingEx](Functions-Thing.md#attachthingtothingex)
- [SetThingAttachFlags](Functions-Thing.md#setthingattachflags)
- [ClearThingAttachFlags](Functions-Thing.md#clearthingattachflags)

Stored in:
- `thing->attach.flags`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_ATTACH_SURFACE` | `0x01` | Attached to a surface. |
| `SITH_ATTACH_THINGFACE` | `0x02` | Attached to a thing face. |
| `SITH_ATTACH_THING` | `0x04` | Attached directly to another thing. |
| `SITH_ATTACH_NOMOVE` | `0x08` | Attachment does not inherit normal movement. |
| `SITH_ATTACH_CLIMBSURFACE` | `0x10` | Attachment is on a climbable surface. |
| `SITH_ATTACH_THINGCLIMBWHIP` | `0x20` | Attachment is tied to whip-climb behavior on a thing. |
| `SITH_ATTACH_TAIL` | `0x40` | Tail-style attachment. |

## Inventory Type Flags

Used by:
- [SetInvFlags](Functions-System.md#setinvflags)

Also used by:
- inventory-type registration and inventory queries in the gameplay inventory system

Stored in:
- per-inventory-entry flag storage managed by the inventory system

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHINVENTORY_TYPE_REGISTERED` | `0x001` | Inventory type is registered. |
| `SITHINVENTORY_TYPE_ITEM` | `0x002` | Inventory type behaves as an item. |
| `SITHINVENTORY_TYPE_WEAPON` | `0x004` | Inventory type behaves as a weapon. |
| `SITHINVENTORY_TYPE_AUTOAIM` | `0x008` | Inventory type participates in auto-aim behavior. |
| `SITHINVENTORY_TYPE_DAMAGEABLE` | `0x010` | Inventory type can be damaged/depleted. |
| `SITHINVENTORY_TYPE_DEFAULT` | `0x020` | Inventory type is available by default. |
| `SITHINVENTORY_TYPE_NOT_CARRIED_BETWEEN_LEVELS` | `0x040` | Do not carry this inventory entry between levels. |
| `SITHINVENTORY_TYPE_BACKPACKITEM` | `0x080` | Backpack-compatible inventory entry. |
| `SITHINVENTORY_TYPE_UNKNOWN_200` | `0x200` | Undecoded inventory flag. |
| `SITHINVENTORY_TYPE_PLAYERWEAPON` | `0x400` | Inventory type is a player weapon. |

## Goal Entry Masks And Respawn Masks

Used by:
- [SetGoalFlags](Functions-Player.md#setgoalflags)
- [ClearGoalFlags](Functions-Player.md#cleargoalflags)
- [GetRespawnMask](Functions-Player.md#getrespawnmask)
- [SetRespawnMask](Functions-Player.md#setrespawnmask)

Notes:
- `SetGoalFlags()` and `ClearGoalFlags()` operate on raw integer values stored in the internal goal-inventory entries at bin `100 + userBin`.
- `GetRespawnMask()` and `SetRespawnMask()` read and write `player->respawnMask` directly.
- I did not find named symbolic bit definitions for the goal-entry mask or the player respawn mask in the current codebase.

## Damage Flags

Used by:
- [DamageThing](Functions-Thing.md#damagething)

Stored in:
- the damage-class bitfield passed through the damage system

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_DAMAGE_IMPACT` | `0x00000001` | Impact damage. |
| `SITH_DAMAGE_ENERGY` | `0x00000002` | Energy damage. |
| `SITH_DAMAGE_FIRE` | `0x00000004` | Fire damage. |
| `SITH_DAMAGE_FISTS` | `0x00000008` | Fists/melee damage. |
| `SITH_DAMAGE_WHIP` | `0x00000010` | Whip damage. |
| `SITH_DAMAGE_MACHETE` | `0x00000020` | Machete damage. |
| `SITH_DAMAGE_DROWN` | `0x00000040` | Drowning damage. |
| `SITH_DAMAGE_CRUSH` | `0x00000080` | Crush damage. |
| `SITH_DAMAGE_POISON` | `0x00000100` | Poison damage. |
| `SITH_DAMAGE_LAVA` | `0x00000200` | Lava damage. |
| `SITH_DAMAGE_400` | `0x00000400` | Undecoded damage bit. |
| `SITH_DAMAGE_ELECTROWHIP` | `0x00000800` | Electric-whip damage. |
| `SITH_DAMAGE_IMP1` | `0x00001000` | IMP1 damage. |
| `SITH_DAMAGE_2000` | `0x00002000` | Undecoded damage bit. |
| `SITH_DAMAGE_IMP4` | `0x00004000` | IMP4 damage. |
| `SITH_DAMAGE_IMP5` | `0x00005000` | IMP5 damage combination/mask value. |
| `SITH_DAMAGE_LIGHTNING` | `0x00100000` | Lightning damage. |
| `SITH_DAMAGE_LASER` | `0x00200000` | Laser damage. |
| `SITH_DAMAGE_RAZOR_ROCK` | `0x00400000` | Razor-rock damage. |
| `SITH_DAMAGE_RAFT_LEAK` | `0x00800000` | Raft-leak damage. |
| `SITH_DAMAGE_SCRAPE` | `0x01000000` | Scrape damage. |
| `SITH_DAMAGE_VEHICLE` | `0x02000000` | Vehicle impact damage. |
| `SITH_DAMAGE_BONK` | `0x04000000` | Bonk damage. |
| `SITH_DAMAGE_DEBRIS` | `0x08000000` | Debris damage. |
| `SITH_DAMAGE_IMP_BLAST` | `0x10000000` | IMP blast damage. |
| `SITH_DAMAGE_HIT` | `0x20000000` | Hit damage. |
| `SITH_DAMAGE_COLD_WATER` | `0x40000000` | Cold-water damage. |
| `SITH_DAMAGE_DART` | `0x80000000` | Dart damage. |

## Shared Type-Specific Flag Field

Used by:
- [GetTypeFlags](Functions-Thing.md#gettypeflags)
- [SetTypeFlags](Functions-Thing.md#settypeflags)
- [ClearTypeFlags](Functions-Thing.md#cleartypeflags)
- the alias verbs for actor, weapon, explosion, item, and particle flags on [Thing host functions](Functions-Thing.md)

Stored in:
- the type-specific flag field inside the current thing-type union

Notes:
- The actor, weapon, explosion, item, and particle alias verbs all expose the same kind of script-facing pattern, but the correct bit meanings depend on the thing type.
- Interpret the returned bitmask using the table that matches the runtime thing type.
- For actor and player things, use [Actor Flags](#actor-flags).
- For weapon things, use [Weapon Flags](#weapon-flags).
- For explosion things, use [Explosion Flags](#explosion-flags).
- For item things, use [Item Flags](#item-flags).
- For particle things, use [Particle Flags](#particle-flags).

## Explosion Flags

Used by:
- [GetExplosionFlags](Functions-Thing.md#getexplosionflags)
- [SetExplosionFlags](Functions-Thing.md#setexplosionflags)
- [ClearExplosionFlags](Functions-Thing.md#clearexplosionflags)

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_EF_ANIMATESPRITE` | `0x0001` | Animate the explosion sprite. |
| `SITH_EF_BLASTPHASE` | `0x0002` | Explosion has a blast phase. |
| `SITH_EF_BLASTDAMAGE` | `0x0004` | Explosion deals blast damage. |
| `SITH_EF_CHILDEXPLOSION` | `0x0008` | Explosion spawns child explosions. |
| `SITH_EF_VARIABLELIGHT` | `0x0010` | Explosion light varies over time. |
| `SITH_EF_RANDOMROLL` | `0x0020` | Apply random sprite roll. |
| `SITH_EF_NOSHOOTERDAMAGE` | `0x0040` | Shooter is protected from this explosion. |
| `SITH_EF_RANDOMDEBRIS` | `0x0080` | Spawn random debris. |
| `SITH_EF_BLINDPLAYER` | `0x0100` | Explosion can blind the player. |
| `SITH_EF_ANIMATEDEBRISMATERIAL` | `0x0200` | Animate debris material. |
| `SITH_EF_UPDATEDEBRISMATERIAL` | `0x0400` | Update debris material during runtime. |
| `SITH_EF_EXPAND` | `0x0800` | Explosion expands over time. |
| `SITH_EF_FADE` | `0x1000` | Explosion fades over time. |

## Actor Flags

Used by:
- [GetActorFlags](Functions-Thing.md#getactorflags)
- [SetActorFlags](Functions-Thing.md#setactorflags)
- [ClearActorFlags](Functions-Thing.md#clearactorflags)
- [GetTypeFlags](Functions-Thing.md#gettypeflags), [SetTypeFlags](Functions-Thing.md#settypeflags), and [ClearTypeFlags](Functions-Thing.md#cleartypeflags) when the thing is an actor or player

Referenced by:
- head-look/head-rotation notes in [Thing host functions](Functions-Thing.md)

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_AF_CANROTATEHEAD` | `0x00000001` | Actor can rotate its head independently. |
| `SITH_AF_VIEWCENTRING` | `0x00000002` | Actor is view-centering. |
| `SITH_AF_HEADLIGHT` | `0x00000004` | Actor uses a headlight. |
| `SITH_AF_INVULNERABLE` | `0x00000008` | Actor is invulnerable. |
| `SITH_AF_VIEWCENTRED` | `0x00000010` | Actor is currently view-centred. |
| `SITH_AF_EXPLODE_WHEN_KILLED` | `0x00000020` | Actor explodes on death. |
| `SITH_AF_BREATHEUNDERWATER` | `0x00000040` | Actor can breathe underwater. |
| `SITH_AF_INVISIBLE` | `0x00000080` | Actor is invisible. |
| `SITH_AF_DROID` | `0x00000100` | Actor is classified as a droid. |
| `SITH_AF_BOSS` | `0x00000200` | Actor is classified as a boss. |
| `SITH_AF_DEAF` | `0x00000400` | Actor is deaf. |
| `SITH_AF_BLIND` | `0x00000800` | Actor is blind. |
| `SITH_AF_SEEINVISIBLE` | `0x00001000` | Actor can see invisible things. |
| `SITH_AF_POISONED` | `0x00002000` | Actor is poisoned. |
| `SITH_AF_FASTMOVE15` | `0x00004000` | Fast movement modifier. |
| `SITH_AF_FASTMOVE10` | `0x00008000` | Fast movement modifier. |
| `SITH_AF_NOSLOPEMOVE` | `0x00010000` | Prevent movement on slopes. |
| `SITH_AF_DELAYFIRE` | `0x00020000` | Delay weapon fire. |
| `SITH_AF_IMMOBILE` | `0x00040000` | Actor is immobile. |
| `SITH_AF_NOUNDERWATERFIRE` | `0x00080000` | Actor cannot fire underwater. |
| `SITH_AF_NOTARGET` | `0x00100000` | Actor cannot be targeted normally. |
| `SITH_AF_CONTROLSDISABLED` | `0x00200000` | Actor controls are disabled. |
| `SITH_AF_FALLKILLED` | `0x00400000` | Actor was killed by falling. |
| `SITH_AF_NOIDLECAMERA` | `0x00800000` | Disable idle camera for this actor. |
| `SITH_AF_FULLDAMAGE` | `0x01000000` | Actor takes full damage. |
| `SITH_AF_SEEINDARK` | `0x02000000` | Actor can see in the dark. |
| `SITH_AF_UNKNOWN_4000000` | `0x04000000` | Undecoded actor flag. |
| `SITH_AF_HUMAN` | `0x08000000` | Actor is classified as human. |
| `SITH_AF_FLYERMOVE` | `0x10000000` | Actor uses flyer-style movement. |
| `SITH_AF_UNKNOWN_20000000` | `0x20000000` | Undecoded actor flag. |
| `SITH_AF_ELECTRICWHIP` | `0x40000000` | Actor uses electric-whip behavior. |
| `SITH_AF_ARACHNID` | `0x80000000` | Actor is classified as arachnid. |

## Weapon Flags

Used by:
- [GetWeaponFlags](Functions-Thing.md#getweaponflags)
- [SetWeaponFlags](Functions-Thing.md#setweaponflags)
- [ClearWeaponFlags](Functions-Thing.md#clearweaponflags)

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_WF_NOSHOOTERDAMAGE` | `0x000001` | Shooter is protected from the weapon's damage. |
| `SITH_WF_FACEHITEXPLODE` | `0x000004` | Explode on face hit. |
| `SITH_WF_THINGHITEXPLODE` | `0x000008` | Explode on thing hit. |
| `SITH_WF_ATTACHFACE` | `0x000080` | Attach weapon/projectile to a face. |
| `SITH_WF_EXPLODE` | `0x000100` | Weapon explodes. |
| `SITH_WF_DAMAGEDESTROY` | `0x000200` | Destroy weapon when damaged. |
| `SITH_WF_IMPACTSOUND` | `0x000400` | Play impact sound. |
| `SITH_WF_ATTACHTHING` | `0x000800` | Attach weapon/projectile to a thing. |
| `SITH_WF_PROXIMITY` | `0x001000` | Proximity-triggered weapon. |
| `SITH_WF_INSTANTIMPACT` | `0x002000` | Instant-impact weapon. |
| `SITH_WF_DAMAGEDECAY` | `0x004000` | Damage decays over time/distance. |
| `SITH_WF_OBJECTTRAIL` | `0x008000` | Spawn object trail behavior. |
| `SITH_WF_AITARGETNODODGE` | `0x020000` | AI targets do not dodge this weapon. |
| `SITH_WF_DECAYEMITSOUNDAWARENESSEVENT` | `0x040000` | Decay emits sound-awareness events. |
| `SITH_WF_SURFACERICOCHET` | `0x080000` | Ricochet on surfaces. |
| `SITH_WF_EMITAITARGETEDEVENT` | `0x200000` | Emit AI-targeted event. |
| `SITH_WF_BLOODSPLATTER` | `0x400000` | Spawn blood splatter on impact. |
| `SITH_WF_MOPHIABOMB` | `0x800000` | Mophia-bomb behavior. |

## Particle Flags

Used by:
- [GetParticleFlags](Functions-Thing.md#getparticleflags)
- [SetParticleFlags](Functions-Thing.md#setparticleflags)
- [ClearParticleFlags](Functions-Thing.md#clearparticleflags)

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHPARTICLE_FLAG_OUTWARD_EXPANDING` | `0x01` | Particle expands outward. |
| `SITHPARTICLE_FLAG_ANIMATE_CEL` | `0x02` | Animate particle cels. |
| `SITHPARTICLE_FLAG_RANDOM_START_CEL` | `0x04` | Start on a random cel. |
| `SITHPARTICLE_FLAG_FADE_OUT_OVER_TIME` | `0x08` | Fade out over time. |
| `SITHPARTICLE_FLAG_EMIT_LIGHT` | `0x10` | Particle emits light. |
| `SITHPARTICLE_FLAG_RANDOM_CEL_CHANGE` | `0x20` | Change cels randomly. |
| `SITHPARTICLE_FLAG_TIMEOUT_RATE` | `0x40` | Use timeout rate. |

## Item Flags

Used by:
- [GetItemFlags](Functions-Thing.md#getitemflags)
- [SetItemFlags](Functions-Thing.md#setitemflags)
- [ClearItemFlags](Functions-Thing.md#clearitemflags)

Referenced by:
- backpack-related notes in [Player host functions](Functions-Player.md)

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_ITEM_RESPAWN_MP` | `0x1` | Item respawns in multiplayer. |
| `SITH_ITEM_RESPAWN_SP` | `0x2` | Item respawns in single-player. |
| `SITH_ITEM_BACKPACK` | `0x4` | Item is a backpack item/container. |

## AI Mode Flags

Used by:
- [AIGetMode](Functions-AI.md#aigetmode)
- [AISetMode](Functions-AI.md#aisetmode)
- [AIClearMode](Functions-AI.md#aiclearmode)

Stored in:
- `thing->controlInfo.aiControl.pLocal->mode`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHAI_MODE_MOVING` | `0x00000001` | AI is moving. |
| `SITHAI_MODE_ATTACKING` | `0x00000002` | AI is attacking. |
| `SITHAI_MODE_SEARCHING` | `0x00000004` | AI is searching. |
| `SITHAI_MODE_TURNING` | `0x00000008` | AI is turning. |
| `SITHAI_MODE_UNKNOWN_10` | `0x00000010` | Undecoded AI mode bit. |
| `SITHAI_MODE_TOUGHSKIN` | `0x00000020` | Tough-skin behavior. |
| `SITHAI_MODE_NOCHECKFORCLIFF` | `0x00000040` | Skip cliff checks. |
| `SITHAI_MODE_UNKNOWN_80` | `0x00000080` | Undecoded AI mode bit. |
| `SITHAI_MODE_BLOCK` | `0x00000100` | Blocking mode. |
| `SITHAI_MODE_ACTIVE` | `0x00000200` | AI is active. |
| `SITHAI_MODE_TARGETVISIBLE` | `0x00000400` | Current target is visible. |
| `SITHAI_MODE_FLEEING` | `0x00000800` | AI is fleeing. |
| `SITHAI_MODE_SLEEPING` | `0x00001000` | AI is sleeping. |
| `SITHAI_MODE_DISABLED` | `0x00002000` | AI is disabled. |
| `SITHAI_MODE_CIRCLESTRAFING` | `0x00004000` | AI is circle-strafing. |
| `SITHAI_MODE_UNKNOWN_8000` | `0x00008000` | Undecoded AI mode bit. |
| `SITHAI_MODE_WANTALLEVENTS` | `0x00010000` | Receive all AI events. |
| `SITHAI_MODE_LOSTSIGHTOFGOAL` | `0x00020000` | Lost sight of current goal. |
| `SITHAI_MODE_INSTINCTUSEWPNTS` | `0x00040000` | Instinct logic may use waypoints. |
| `SITHAI_MODE_CHASE_GOAL` | `0x00080000` | Chase current goal. |
| `SITHAI_MODE_UNKNOWN_100000` | `0x00100000` | Undecoded AI mode bit. |
| `SITHAI_MODE_WALLCRAWLING` | `0x00200000` | AI is wall-crawling. |
| `SITHAI_MODE_UNKNOWN_400000` | `0x00400000` | Undecoded AI mode bit. |
| `SITHAI_MODE_HUNTING` | `0x00800000` | AI is hunting. |
| `SITHAI_MODE_UNKNOWN_1000000` | `0x01000000` | Undecoded AI mode bit. |
| `SITHAI_MODE_NOCHASING` | `0x02000000` | Disable chasing behavior. |
| `SITHAI_MODE_TRAVERSEWPNTS` | `0x04000000` | Traverse waypoints. |
| `SITHAI_MODE_ARMOREDSKIN` | `0x08000000` | Armored-skin behavior. |
| `SITHAI_MODE_FLEEINGTOWAYPOINT` | `0x10000000` | Fleeing toward a waypoint. |

## AI Submode Flags

Used by:
- [AIGetSubMode](Functions-AI.md#aigetsubmode)
- [AISetSubMode](Functions-AI.md#aisetsubmode)
- [AIClearSubMode](Functions-AI.md#aiclearsubmode)

Stored in:
- `thing->controlInfo.aiControl.pLocal->submode`

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHAI_SUBMODE_UNKNOWN_1` | `0x00000001` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_UNKNOWN_2` | `0x00000002` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_UNKNOWN_4` | `0x00000004` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_UNKNOWN_8` | `0x00000008` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_UNKNOWN_10` | `0x00000010` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_SKIPCHECKFIREFOV` | `0x00000020` | Skip fire-FOV checks. |
| `SITHAI_SUBMODE_FIREADDEYEOFFSET` | `0x00000040` | Add eye offset when firing. |
| `SITHAI_SUBMODE_NOMOVEBACKWARDS` | `0x00000080` | Do not move backward. |
| `SITHAI_SUBMODE_UNKNOWN_100` | `0x00000100` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_CONTINUOUSMOTION` | `0x00000200` | Use continuous motion. |
| `SITHAI_SUBMODE_UNKNOWN_400` | `0x00000400` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_UNKNOWN_800` | `0x00000800` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_HEADTRACKINGMOTION` | `0x00001000` | Use head-tracking motion. |
| `SITHAI_SUBMODE_BODYTRACKINGMOTION` | `0x00002000` | Use body-tracking motion. |
| `SITHAI_SUBMODE_UNKNOWN_4000` | `0x00004000` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_CONTINUOUSWPNTMOTION` | `0x00008000` | Continuous waypoint motion. |
| `SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION` | `0x00010000` | Semi-continuous waypoint motion. |
| `SITHAI_SUBMODE_SEMICONTINUOUSMOTION` | `0x00020000` | Semi-continuous motion. |
| `SITHAI_SUBMODE_QUICKMODEFADE` | `0x00040000` | Quick mode fade. |
| `SITHAI_SUBMODE_SLOWMODEFADE` | `0x00080000` | Slow mode fade. |
| `SITHAI_SUBMODE_USEMATCHVELOCITY` | `0x00100000` | Use matched velocity. |
| `SITHAI_SUBMODE_SWIMNEARSURFACE` | `0x00200000` | Swim near the water surface. |
| `SITHAI_SUBMODE_UNKNOWN_400000` | `0x00400000` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_ALLOWSTEPTHING` | `0x00800000` | Allow stepping onto things. |
| `SITHAI_SUBMODE_SPECIALTURNS` | `0x01000000` | Use special turns. |
| `SITHAI_SUBMODE_UNKNOWN_2000000` | `0x02000000` | Undecoded AI submode bit. |
| `SITHAI_SUBMODE_WALLCRAWLLOCKED` | `0x08000000` | Lock wall-crawling state. |

## AI Waypoint Layer Flags

Used by:
- [AISetWpntFlags](Functions-AI.md#aisetwpntflags)
- [AIClearWpntFlags](Functions-AI.md#aiclearwpntflags)

Stored in:
- per-waypoint layer/flag data managed by the AI waypoint utilities

Notes:
- These are stored as mask bits, not plain layer numbers.
- [AISetActiveWpntLayer](Functions-AI.md#aisetactivewpntlayer) uses a plain layer number instead of this bitmask.

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_AIWPNT_DISABLED` | `0x00100` | Disable the waypoint. |
| `SITH_AIWPNT_LAYER0` | `0x01000` | Waypoint belongs to layer 0. |
| `SITH_AIWPNT_LAYER1` | `0x02000` | Waypoint belongs to layer 1. |
| `SITH_AIWPNT_LAYER2` | `0x04000` | Waypoint belongs to layer 2. |
| `SITH_AIWPNT_LAYER3` | `0x08000` | Waypoint belongs to layer 3. |
| `SITH_AIWPNT_LAYER4` | `0x10000` | Waypoint belongs to layer 4. |
| `SITH_AIWPNT_LAYER5` | `0x20000` | Waypoint belongs to layer 5. |

## Hint Flags

Used by:
- [GetHintSolved](Functions-System.md#gethintsolved)
- [SetHintSolved](Functions-System.md#sethintsolved)
- [SetHintUnsolved](Functions-System.md#sethintunsolved)

Stored in:
- the hint thing's hint-flag field

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITH_HINT_SOLVED` | `0x4000` | Hint is solved. |
| `SITH_HINT_SEEN` | `0x8000` | Hint has been seen. |

## Projectile Fire Flags

Used by:
- [FireProjectile](Functions-System.md#fireprojectile)

Stored in:
- the fire-projectile request flags passed into weapon/projectile code

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHFIREPROJECTILE_SCALE_VELOCITY` | `0x001` | Scale projectile velocity using the `extra` parameter. |
| `SITHFIREPROJECTILE_SCALE_DAMAGE` | `0x002` | Scale projectile damage using the `extra` parameter. |
| `SITHFIREPROJECTILE_UNKNOWN_4` | `0x004` | Undecoded projectile-fire flag. |
| `SITHFIREPROJECTILE_UNKNOWN_8` | `0x008` | Undecoded projectile-fire flag. |
| `SITHFIREPROJECTILE_RAPID_FIRE` | `0x010` | Use rapid-fire handling. |
| `SITHFIREPROJECTILE_AIM_ERROR` | `0x020` | Apply the supplied aim-error vector. |
| `SITHFIREPROJECTILE_RIGHTHAND_FIRE` | `0x080` | Fire from the right-hand origin. |
| `SITHFIREPROJECTILE_TORSO_FIRE` | `0x100` | Fire from the torso origin. |
| `SITHFIREPROJECTILE_FIRE_EFFECT` | `0x200` | Trigger weapon-fire effect handling. |

## Keyframe Flags

Used by:
- [PlayKey](Functions-Thing.md#playkey)
- [PlayKeyEx](Functions-Thing.md#playkeyex)

Stored in:
- keyframe playback track flags

| Flag | Value | Meaning |
| --- | ---: | --- |
| `RDKEYFRAME_PUPPET_CONTROLLED` | `0x01` | Track is puppet-controlled. |
| `RDKEYFRAME_NOLOOP` | `0x02` | Do not loop. |
| `RDKEYFRAME_PAUSE_ON_LAST_FRAME` | `0x04` | Pause on the last frame. |
| `RDKEYFRAME_RESTART_ACTIVE` | `0x08` | Restart an already active track. |
| `RDKEYFRAME_DISABLE_FADEIN` | `0x10` | Disable fade-in blending. |
| `RDKEYFRAME_FADEOUT_NOLOOP` | `0x20` | Fade out instead of looping. |
| `RDKEYFRAME_FORCEMOVE` | `0x40` | Treat the key as a force-move track. |

## Sound Play Flags

Used by:
- [PlaySoundThing](Functions-Sound.md#playsoundthing)
- [PlaySoundPos](Functions-Sound.md#playsoundpos)
- [PlaySoundLocal](Functions-Sound.md#playsoundlocal)
- [PlaySoundGlobal](Functions-Sound.md#playsoundglobal)

Stored in:
- sound playback requests passed into the mixer

Notes:
- [PlaySoundPos](Functions-Sound.md#playsoundpos) forces `SOUNDPLAY_ABSOLUTE_POS`.
- [PlaySoundLocal](Functions-Sound.md#playsoundlocal) and [PlaySoundGlobal](Functions-Sound.md#playsoundglobal) clear positional flags before playing.
- The mixer directly maps `SOUNDPLAY_LOOP`, `SOUNDPLAY_PLAYONCE`, and `SOUNDPLAY_PLAYTHINGONCE` into channel flags. Priority bits are handled separately by the mixer priority helper.

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SOUNDPLAY_LOOP` | `0x001` | Loop the sound. |
| `SOUNDPLAY_REMOVE_AFTER_FADEOUT` | `0x002` | Remove the sound after fade-out finishes. |
| `SOUNDPLAY_AMBIENT` | `0x004` | Ambient-sound playback flag. |
| `SOUNDPLAY_USE_DOPPLAR_FX` | `0x008` | Apply Doppler effects. |
| `SOUNDPLAY_FADEIN` | `0x010` | Fade in the sound. |
| `SOUNDPLAY_FADEOUT` | `0x020` | Fade out the sound. |
| `SOUNDPLAY_ABSOLUTE_POS` | `0x040` | Use an absolute world position. |
| `SOUNDPLAY_THING_POS` | `0x080` | Use thing-attached positioning. |
| `SOUNDPLAY_HIGH_PRIORITY` | `0x100` | High-priority playback. |
| `SOUNDPLAY_HIGHEST_PRIORITY` | `0x200` | Highest-priority playback. |
| `SOUNDPLAY_PLAYONCE` | `0x400` | One instance only for the sound. |
| `SOUNDPLAY_PLAYTHINGONCE` | `0x800` | One instance only for the sound/thing combination. |

## Actor Special-Move Flags

Used by:
- [SetThingStateChange](Functions-Thing.md#setthingstatechange) when the state-change `type` selects animated movement

Stored in:
- `thing->thingInfo.actorInfo.stateChange.params.moveFlags`

Notes:
- These flags combine a base movement type with optional direction and evasion-strategy bits.

| Flag | Value | Meaning |
| --- | ---: | --- |
| `SITHACTORSPECIALMOVE_ROLL` | `0x0001` | Roll or hop move. |
| `SITHACTORSPECIALMOVE_STRAFE` | `0x0002` | Strafe move. |
| `SITHACTORSPECIALMOVE_TURN45` | `0x0004` | 45-degree turn. |
| `SITHACTORSPECIALMOVE_TURN90` | `0x0008` | 90-degree turn. |
| `SITHACTORSPECIALMOVE_TURN135` | `0x0010` | 135-degree turn. |
| `SITHACTORSPECIALMOVE_TURN180` | `0x0020` | 180-degree turn. |
| `SITHACTORSPECIALMOVE_MOUNTWALL` | `0x0040` | Mount a wall/upward wall move. |
| `SITHACTORSPECIALMOVE_DIR_RANDOM` | `0x0100` | Choose direction randomly. |
| `SITHACTORSPECIALMOVE_DIR_LEFT` | `0x0200` | Force left direction. |
| `SITHACTORSPECIALMOVE_DIR_RIGHT` | `0x0400` | Force right direction. |
| `SITHACTORSPECIALMOVE_EVADE_PERPENDICULAR` | `0x1000` | Evade perpendicular to the threat. |
| `SITHACTORSPECIALMOVE_EVADE_ANGLE` | `0x2000` | Evade at an angle from the threat. |
| `SITHACTORSPECIALMOVE_EVADE_AWAY` | `0x4000` | Evade directly away from the threat. |
| `SITHACTORSPECIALMOVE_EVADE_PATHFIND` | `0x8000` | Use pathfinding while evading. |
