# COG Types And Modes

This page documents the named non-flag value sets used by COG host functions and return values.

When a section still contains `UNKNOWN_*`, `RESERVED`, or unnamed numeric modes, that wording is preserved from the current source instead of being guessed at in the docs.

## Quick Links

- [COG Symbol Reference Types](#cog-symbol-reference-types)
- [Thing Types](#thing-types)
- [Collide Types](#collide-types)
- [Puppet Armed Modes](#puppet-armed-modes)
- [Puppet Move Modes](#puppet-move-modes)
- [Puppet Major Modes](#puppet-major-modes)
- [Puppet Submodes](#puppet-submodes)
- [Thing Move Status Values](#thing-move-status-values)
- [Render Geometry Modes](#render-geometry-modes)
- [Render Light Modes](#render-light-modes)
- [Sound Class Modes](#sound-class-modes)
- [Master Game Modes](#master-game-modes)
- [Difficulty Values](#difficulty-values)
- [Actor State-Change Types](#actor-state-change-types)

## COG Symbol Reference Types

Used by:
- [GetSenderType](Functions-System.md#getsendertype)
- [GetSourceType](Functions-System.md#getsourcetype)

| Value | Constant | Meaning |
| ---: | --- | --- |
| `0` | `SITHCOG_SYM_REF_NONE` | No typed reference. |
| `1` | `SITHCOG_SYM_REF_INT` | Integer value. |
| `2` | `SITHCOG_SYM_REF_FLEX` | Floating-point value. |
| `3` | `SITHCOG_SYM_REF_THING` | Thing reference. |
| `4` | `SITHCOG_SYM_REF_TEMPLATE` | Thing-template reference. |
| `5` | `SITHCOG_SYM_REF_SECTOR` | Sector reference. |
| `6` | `SITHCOG_SYM_REF_SURFACE` | Surface reference. |
| `7` | `SITHCOG_SYM_REF_KEYFRAME` | Keyframe reference. |
| `8` | `SITHCOG_SYM_REF_SOUND` | Sound reference. |
| `9` | `SITHCOG_SYM_REF_COG` | COG reference. |
| `10` | `SITHCOG_SYM_REF_MATERIAL` | Material reference. |
| `11` | `SITHCOG_SYM_REF_VECTOR` | Vector value. |
| `12` | `SITHCOG_SYM_REF_MODEL` | Model reference. |
| `13` | `SITHCOG_SYM_REF_AICLASS` | AI class reference. |

## Thing Types

Used by:
- [SetThingType](Functions-Thing.md#setthingtype)
- [GetThingType](Functions-Thing.md#getthingtype)
- [Shared Type-Specific Flag Field](Flags.md#shared-type-specific-flag-field)

Valid runtime thing-type values are `0` through `14`. `15` is the sentinel `SITH_THING_NUMTYPES`.

| Value | Constant | Meaning |
| ---: | --- | --- |
| `0` | `SITH_THING_FREE` | Free/system thing slot; also used as the "system/no source thing" bucket in some COG message filtering paths. |
| `1` | `SITH_THING_CAMERA` | Camera thing. |
| `2` | `SITH_THING_ACTOR` | Actor thing. |
| `3` | `SITH_THING_WEAPON` | Weapon thing. |
| `4` | `SITH_THING_DEBRIS` | Debris thing. |
| `5` | `SITH_THING_ITEM` | Inventory or pickup item thing. |
| `6` | `SITH_THING_EXPLOSION` | Explosion thing. |
| `7` | `SITH_THING_COG` | COG helper thing. |
| `8` | `SITH_THING_GHOST` | Ghost thing. |
| `9` | `SITH_THING_CORPSE` | Corpse thing. |
| `10` | `SITH_THING_PLAYER` | Player thing. |
| `11` | `SITH_THING_PARTICLE` | Particle thing. |
| `12` | `SITH_THING_HINT` | Hint thing. |
| `13` | `SITH_THING_SPRITE` | Sprite thing. |
| `14` | `SITH_THING_POLYLINE` | Polyline thing. |

## Collide Types

Used by:
- [SetCollideType](Functions-Thing.md#setcollidetype)
- [GetCollideType](Functions-Thing.md#getcollidetype)

The current collide-type enum defines three meaningful built-in values. `2` is not named in the current source. `4` is the sentinel `SITH_COLLIDE_NUMTYPES`.

| Value | Constant | Meaning |
| ---: | --- | --- |
| `0` | `SITH_COLLIDE_NONE` | No collision shape. |
| `1` | `SITH_COLLIDE_SPHERE` | Sphere collision. |
| `3` | `SITH_COLLIDE_FACE` | Face-based collision. |

## Puppet Armed Modes

Used by:
- [SetArmedMode](Functions-Thing.md#setarmedmode)
- [AIGetArmedMode](Functions-AI.md#aigetarmedmode)
- [SetThingStateChange](Functions-Thing.md#setthingstatechange) when the state-change type selects armed-mode changes
- [GetMajorMode](Functions-Thing.md#getmajormode)
- [SetPuppetModeFPS](Functions-Thing.md#setpuppetmodefps)

Notes:
- Valid armed-mode values are `0` through `7`.
- Only `0` is explicitly identifiable in the current tree as the default or unarmed mode.
- The current source does not define symbolic names for armed-mode values `1` through `7`.

| Value | Meaning |
| ---: | --- |
| `0` | Default or unarmed mode. |
| `1` | Armed mode `1` in the current puppet class. |
| `2` | Armed mode `2` in the current puppet class. |
| `3` | Armed mode `3` in the current puppet class. |
| `4` | Armed mode `4` in the current puppet class. |
| `5` | Armed mode `5` in the current puppet class. |
| `6` | Armed mode `6` in the current puppet class. |
| `7` | Armed mode `7` in the current puppet class. |

## Puppet Move Modes

Used by:
- [SetMoveMode](Functions-Thing.md#setmovemode)
- [GetMajorMode](Functions-Thing.md#getmajormode)
- [SetPuppetModeFPS](Functions-Thing.md#setpuppetmodefps)

| Value | Constant | Meaning |
| ---: | --- | --- |
| `0` | `SITHPUPPET_MOVEMODE_NORMAL` | Normal locomotion mode. |
| `1` | `SITHPUPPET_MOVEMODE_SWIM` | Swimming locomotion mode. |
| `2` | `SITHPUPPET_MOVEMODE_CRAWL` | Crawling locomotion mode. |

## Puppet Major Modes

Used by:
- [GetMajorMode](Functions-Thing.md#getmajormode)
- [SetPuppetModeFPS](Functions-Thing.md#setpuppetmodefps)
- all verbs that resolve a submode through the current puppet-state major mode, such as [PlayMode](Functions-Thing.md#playmode) and [PlayForceMoveMode](Functions-Thing.md#playforcemovemode)

Puppet major modes are not a separate named enum in the current source. They are computed as:

```text
majorMode = armedMode + 8 * moveMode
```

That produces the following valid major-mode values:

| Major mode | Move mode | Armed mode |
| ---: | ---: | ---: |
| `0` | `0` (`Normal`) | `0` |
| `1` | `0` (`Normal`) | `1` |
| `2` | `0` (`Normal`) | `2` |
| `3` | `0` (`Normal`) | `3` |
| `4` | `0` (`Normal`) | `4` |
| `5` | `0` (`Normal`) | `5` |
| `6` | `0` (`Normal`) | `6` |
| `7` | `0` (`Normal`) | `7` |
| `8` | `1` (`Swim`) | `0` |
| `9` | `1` (`Swim`) | `1` |
| `10` | `1` (`Swim`) | `2` |
| `11` | `1` (`Swim`) | `3` |
| `12` | `1` (`Swim`) | `4` |
| `13` | `1` (`Swim`) | `5` |
| `14` | `1` (`Swim`) | `6` |
| `15` | `1` (`Swim`) | `7` |
| `16` | `2` (`Crawl`) | `0` |
| `17` | `2` (`Crawl`) | `1` |
| `18` | `2` (`Crawl`) | `2` |
| `19` | `2` (`Crawl`) | `3` |
| `20` | `2` (`Crawl`) | `4` |
| `21` | `2` (`Crawl`) | `5` |
| `22` | `2` (`Crawl`) | `6` |
| `23` | `2` (`Crawl`) | `7` |

## Puppet Submodes

Used by:
- [PlayMode](Functions-Thing.md#playmode)
- [StopMode](Functions-Thing.md#stopmode)
- [SynchMode](Functions-Thing.md#synchmode)
- [IsModePlaying](Functions-Thing.md#ismodeplaying)
- [PauseMode](Functions-Thing.md#pausemode)
- [ResumeMode](Functions-Thing.md#resumemode)
- [WaitMode](Functions-Thing.md#waitmode)
- [SetPuppetModeFPS](Functions-Thing.md#setpuppetmodefps)
- [PlayForceMoveMode](Functions-Thing.md#playforcemovemode)
- [FireProjectile](Functions-System.md#fireprojectile)

Notes:
- Valid named puppet submodes are `1` through `83`.
- The current source reserves array space for `84` submodes, but value `0` does not have a named enum constant in the current tree.

```text
 1  SITHPUPPETSUBMODE_STAND
 2  SITHPUPPETSUBMODE_WALK
 3  SITHPUPPETSUBMODE_RUN
 4  SITHPUPPETSUBMODE_WALKBACK
 5  SITHPUPPETSUBMODE_HOPBACK
 6  SITHPUPPETSUBMODE_HOPLEFT
 7  SITHPUPPETSUBMODE_HOPRIGHT
 8  SITHPUPPETSUBMODE_STRAFELEFT
 9  SITHPUPPETSUBMODE_STRAFERIGHT
10  SITHPUPPETSUBMODE_TURNLEFT
11  SITHPUPPETSUBMODE_TURNRIGHT
12  SITHPUPPETSUBMODE_SLIDEDOWNFWD
13  SITHPUPPETSUBMODE_SLIDEDOWNBACK
14  SITHPUPPETSUBMODE_LEAP
15  SITHPUPPETSUBMODE_JUMPREADY
16  SITHPUPPETSUBMODE_JUMPUP
17  SITHPUPPETSUBMODE_JUMPFWD
18  SITHPUPPETSUBMODE_RISING
19  SITHPUPPETSUBMODE_FALL
20  SITHPUPPETSUBMODE_DEATH
21  SITHPUPPETSUBMODE_DEATH2
22  SITHPUPPETSUBMODE_FIDGET
23  SITHPUPPETSUBMODE_FIDGET2
24  SITHPUPPETSUBMODE_PICKUP
25  SITHPUPPETSUBMODE_PUSHPULLREADY
26  SITHPUPPETSUBMODE_PUSHITEM
27  SITHPUPPETSUBMODE_PULLITEM
28  SITHPUPPETSUBMODE_MOUNTLEDGE
29  SITHPUPPETSUBMODE_GRABLEDGE
30  SITHPUPPETSUBMODE_HANGLEDGE
31  SITHPUPPETSUBMODE_HANGSHIMLEFT
32  SITHPUPPETSUBMODE_HANGSHIMRIGHT
33  SITHPUPPETSUBMODE_MOUNTWALL
34  SITHPUPPETSUBMODE_CLIMBWALLIDLE
35  SITHPUPPETSUBMODE_CLIMBWALLUP
36  SITHPUPPETSUBMODE_CLIMBWALLDOWN
37  SITHPUPPETSUBMODE_CLIMBWALLLEFT
38  SITHPUPPETSUBMODE_CLIMBWALLRIGHT
39  SITHPUPPETSUBMODE_CLIMBPULLINGUP
40  SITHPUPPETSUBMODE_WHIPCLIMBMOUNT
41  SITHPUPPETSUBMODE_WHIPCLIMBIDLE
42  SITHPUPPETSUBMODE_WHIPCLIMBUP
43  SITHPUPPETSUBMODE_WHIPCLIMBDOWN
44  SITHPUPPETSUBMODE_WHIPCLIMBLEFT
45  SITHPUPPETSUBMODE_WHIPCLIMBRIGHT
46  SITHPUPPETSUBMODE_WHIPCLIMBDISMOUNT
47  SITHPUPPETSUBMODE_WHIPSWINGMOUNT
48  SITHPUPPETSUBMODE_WHIPSWING
49  SITHPUPPETSUBMODE_MOUNTFROMWATER
50  SITHPUPPETSUBMODE_DIVEFROMSURFACE
51  SITHPUPPETSUBMODE_MOUNT1MSTEP
52  SITHPUPPETSUBMODE_MOUNT2MLEDGE
53  SITHPUPPETSUBMODE_JUMPROLLBACK
54  SITHPUPPETSUBMODE_JUMPROLLFWD
55  SITHPUPPETSUBMODE_LAND
56  SITHPUPPETSUBMODE_HITHEADL
57  SITHPUPPETSUBMODE_HITHEADR
58  SITHPUPPETSUBMODE_HITSIDEL
59  SITHPUPPETSUBMODE_HITSIDER
60  SITHPUPPETSUBMODE_ACTIVATE
61  SITHPUPPETSUBMODE_ACTIVATEHIGH
62  SITHPUPPETSUBMODE_DRAWWEAPON
63  SITHPUPPETSUBMODE_AIMWEAPON
64  SITHPUPPETSUBMODE_HOLSTERWEAPON
65  SITHPUPPETSUBMODE_FIRE
66  SITHPUPPETSUBMODE_FIRE2
67  SITHPUPPETSUBMODE_FIRE3
68  SITHPUPPETSUBMODE_FIRE4
69  SITHPUPPETSUBMODE_STAND2WALK
70  SITHPUPPETSUBMODE_WALK2STAND
71  SITHPUPPETSUBMODE_STAND2CRAWL
72  SITHPUPPETSUBMODE_CRAWL2STAND
73  SITHPUPPETSUBMODE_WALK2ATTACK
74  SITHPUPPETSUBMODE_VICTORY
75  SITHPUPPETSUBMODE_HIT
76  SITHPUPPETSUBMODE_HIT2
77  SITHPUPPETSUBMODE_GRABARMS
78  SITHPUPPETSUBMODE_RESERVED
79  SITHPUPPETSUBMODE_CLIMBTOCLIMB
80  SITHPUPPETSUBMODE_CLIMBTOHANG
81  SITHPUPPETSUBMODE_LEAPLEFT
82  SITHPUPPETSUBMODE_LEAPRIGHT
83  SITHPUPPETSUBMODE_FALLFORWARD
```

## Thing Move Status Values

Used by:
- [GetMoveStatus](Functions-Thing.md#getmovestatus)

Notes:
- These are the runtime movement-status values returned by the current engine.
- `UNKNOWN_*` entries are preserved as-is because the current source does not define them more clearly.

```text
  0  SITHPLAYERMOVE_STILL
  1  SITHPLAYERMOVE_WALKING
  2  SITHPLAYERMOVE_RUNNING
  3  SITHPLAYERMOVE_CRAWLIDLE
  4  SITHPLAYERMOVE_UNKNOWN_4
  5  SITHPLAYERMOVE_UNKNOWN_5
  6  SITHPLAYERMOVE_HANGING
  7  SITHPLAYERMOVE_PUSHING
  8  SITHPLAYERMOVE_PULLING
  9  SITHPLAYERMOVE_CLIMBIDLE
 10  SITHPLAYERMOVE_WHIPSWINGING
 11  SITHPLAYERMOVE_WHIPCLIMBIDLE
 12  SITHPLAYERMOVE_SWIMIDLE
 13  SITHPLAYERMOVE_JUMPFWD
 14  SITHPLAYERMOVE_JUMPUP
 15  SITHPLAYERMOVE_FALLING
 16  SITHPLAYERMOVE_LAND
 17  SITHPLAYERMOVE_JEEP_IMPACT
 18  SITHPLAYERMOVE_JUMP_READY
 19  SITHPLAYERMOVE_PUSHPULL_READY
 20  SITHPLAYERMOVE_MOUNTING_WALL
 21  SITHPLAYERMOVE_UNKNOWN_21
 22  SITHPLAYERMOVE_CLIMBING_UP
 23  SITHPLAYERMOVE_CLIMBING_DOWN
 24  SITHPLAYERMOVE_CLIMBING_LEFT
 25  SITHPLAYERMOVE_CLIMBING_RIGHT
 26  SITHPLAYERMOVE_PULLINGUP
 27  SITHPLAYERMOVE_PULLINGUP_1M
 28  SITHPLAYERMOVE_WHIPCLIMB_START
 29  SITHPLAYERMOVE_WHIP_UNK2
 30  SITHPLAYERMOVE_MINECAR_IDLE
 31  SITHPLAYERMOVE_JEEP_IDLE
 32  SITHPLAYERMOVE_RAFT_IDLE
 33  SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT
 34  SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT
 35  SITHPLAYERMOVE_RAFT_TURN_LEFT
 36  SITHPLAYERMOVE_RAFT_TURN_RIGHT
 37  SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT
 38  SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT
 39  SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT
 40  SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT
 41  SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT
 42  SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT
 43  SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT
 44  SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT
 45  SITHPLAYERMOVE_RAFT_DOCKING
 46  SITHPLAYERMOVE_UNKNOWN_46
 47  SITHPLAYERMOVE_SLIDING
 48  SITHPLAYERMOVE_MINECAR_BOARDING
 49  SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT
 50  SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT
 51  SITHPLAYERMOVE_MINECAR_DUCKING
 52  SITHPLAYERMOVE_MINECAR_DUCKED
 53  SITHPLAYERMOVE_MINECAR_GETTING_UP
 54  SITHPLAYERMOVE_JEEP_BOARDING
 55  SITHPLAYERMOVE_ROLLING_LEFT
 56  SITHPLAYERMOVE_ROLLING_RIGHT
 57  SITHPLAYERMOVE_STRAFING_LEFT
 58  SITHPLAYERMOVE_STRAFING_RIGHT
 59  SITHPLAYERMOVE_ACTIVATING
 60  SITHPLAYERMOVE_UNKNOWN_60
 61  SITHPLAYERMOVE_JUMPBACK
 62  SITHPLAYERMOVE_SLIDEDOWNFORWARD
 63  SITHPLAYERMOVE_UNKNOWN_63
 64  SITHPLAYERMOVE_STAND_TO_CRAWL
 65  SITHPLAYERMOVE_CRAWL_TO_STAND
 66  SITHPLAYERMOVE_JUMPROLLBACK
 67  SITHPLAYERMOVE_JUMPROLLFWD
 68  SITHPLAYERMOVE_CLIMB_DOWN_TO_MOUNT
 69  SITHPLAYERMOVE_CLIMB_TO_HANG
 70  SITHPLAYERMOVE_RAFT_BOARDING
 71  SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT
 72  SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT
 73  SITHPLAYERMOVE_RAFT_UNBOARD_START
 74  SITHPLAYERMOVE_TURNING_LEFT
 75  SITHPLAYERMOVE_TURNING_RIGHT
 76  SITHPLAYERMOVE_WALK2STAND
 77  SITHPLAYERMOVE_STAND2WALK
 78  SITHPLAYERMOVE_UNKNOWN_78
 79  SITHPLAYERMOVE_STAND2RUN
 80  SITHPLAYERMOVE_UNKNOWN_80
 81  SITHPLAYERMOVE_UNKNOWN_81
 82  SITHPLAYERMOVE_UNKNOWN_82
 83  SITHPLAYERMOVE_UNKNOWN_83
 84  SITHPLAYERMOVE_UNKNOWN_84
 85  SITHPLAYERMOVE_JEEP_UNBOARDING
 86  SITHPLAYERMOVE_UNKNOWN_86
 87  SITHPLAYERMOVE_JEWELFLYING
 88  SITHPLAYERMOVE_JEWELFLYING_UNKN1
 89  SITHPLAYERMOVE_JEWELFLYING_UNKN2
 90  SITHPLAYERMOVE_SLIDEDOWNBACK
 91  SITHPLAYERMOVE_KNOCKEDOUT
 92  SITHPLAYERMOVE_JUMPLEFT
 93  SITHPLAYERMOVE_JUMPRIGHT
 94  SITHPLAYERMOVE_LEAPFWD
 95  SITHPLAYERMOVE_RUNOVER
 96  SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES
 97  SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES
 98  SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES
 99  SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES
100  SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES
101  SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES
102  SITHPLAYERMOVE_TURNING_180_DEGREES
103  SITHPLAYERMOVE_UNKNOWN_103
104  SITHPLAYERMOVE_UNKNOWN_104
105  SITHPLAYERMOVE_UNKNOWN_105
106  SITHPLAYERMOVE_UNKNOWN_106
```

## Render Geometry Modes

Used by:
- [SetFaceGeoMode](Functions-Surface.md#setfacegeomode)
- [GetFaceGeoMode](Functions-Surface.md#getfacegeomode)

| Value | Constant | Meaning |
| ---: | --- | --- |
| `0` | `RD_GEOMETRY_NONE` | No face geometry rendering. |
| `1` | `RD_GEOMETRY_VERTEX` | Vertex-only rendering. |
| `2` | `RD_GEOMETRY_WIREFRAME` | Wireframe rendering. |
| `3` | `RD_GEOMETRY_SOLID` | Solid untextured face rendering. |
| `4` | `RD_GEOMETRY_FULL` | Full textured face rendering. |

## Render Light Modes

Used by:
- [GetThingCurLightMode](Functions-Thing.md#getthingcurlightmode)
- [SetThingCurLightMode](Functions-Thing.md#setthingcurlightmode)
- [SetFaceLightMode](Functions-Surface.md#setfacelightmode)
- [GetFaceLightMode](Functions-Surface.md#getfacelightmode)

| Value | Constant | Meaning |
| ---: | --- | --- |
| `0` | `RD_LIGHTING_NONE` | No lighting. |
| `1` | `RD_LIGHTING_LIT` | Lit mode. |
| `2` | `RD_LIGHTING_DIFFUSE` | Diffuse lighting mode. |
| `3` | `RD_LIGHTING_GOURAUD` | Gouraud lighting mode. |

## Sound Class Modes

Used by:
- [PlaySoundClass](Functions-Sound.md#playsoundclass)
- [StopSoundClass](Functions-Sound.md#stopsoundclass)
- [PlayVoiceMode](Functions-Sound.md#playvoicemode)

Valid sound-class mode values are `1` through `140`.

### Lifecycle And Continuous Movement

```text
  1  SITHSOUNDCLASS_CREATE
  2  SITHSOUNDCLASS_ACTIVATE
  3  SITHSOUNDCLASS_STARTMOVE
  4  SITHSOUNDCLASS_STOPMOVE
  5  SITHSOUNDCLASS_MOVING
```

### Footsteps And Surface Movement

```text
  6  SITHSOUNDCLASS_LWALKHARD
  7  SITHSOUNDCLASS_RWALKHARD
  8  SITHSOUNDCLASS_LRUNHARD
  9  SITHSOUNDCLASS_RRUNHARD
 10  SITHSOUNDCLASS_LWALKMETAL
 11  SITHSOUNDCLASS_RWALKMETAL
 12  SITHSOUNDCLASS_LRUNMETAL
 13  SITHSOUNDCLASS_RRUNMETAL
 14  SITHSOUNDCLASS_LWALKWATER
 15  SITHSOUNDCLASS_RWALKWATER
 16  SITHSOUNDCLASS_LRUNWATER
 17  SITHSOUNDCLASS_RRUNWATER
 18  SITHSOUNDCLASS_LWALKPUDDLE
 19  SITHSOUNDCLASS_RWALKPUDDLE
 20  SITHSOUNDCLASS_LRUNPUDDLE
 21  SITHSOUNDCLASS_RRUNPUDDLE
 22  SITHSOUNDCLASS_LWALKEARTH
 23  SITHSOUNDCLASS_RWALKEARTH
 24  SITHSOUNDCLASS_LRUNEARTH
 25  SITHSOUNDCLASS_RRUNEARTH
 26  SITHSOUNDCLASS_LWALKSNOW
 27  SITHSOUNDCLASS_RWALKSNOW
 28  SITHSOUNDCLASS_LRUNSNOW
 29  SITHSOUNDCLASS_RRUNSNOW
 30  SITHSOUNDCLASS_LWALKWOOD
 31  SITHSOUNDCLASS_RWALKWOOD
 32  SITHSOUNDCLASS_LRUNWOOD
 33  SITHSOUNDCLASS_RRUNWOOD
 34  SITHSOUNDCLASS_LWALKHARDECHO
 35  SITHSOUNDCLASS_RWALKHARDECHO
 36  SITHSOUNDCLASS_LRUNHARDECHO
 37  SITHSOUNDCLASS_RRUNHARDECHO
 38  SITHSOUNDCLASS_LWALKWOODECHO
 39  SITHSOUNDCLASS_RWALKWOODECHO
 40  SITHSOUNDCLASS_LRUNWOODECHO
 41  SITHSOUNDCLASS_RRUNWOODECHO
 42  SITHSOUNDCLASS_LWALKEARTHECHO
 43  SITHSOUNDCLASS_RWALKEARTHECHO
 44  SITHSOUNDCLASS_LRUNEARTHECHO
 45  SITHSOUNDCLASS_RRUNEARTHECHO
 46  SITHSOUNDCLASS_LWALKAET
 47  SITHSOUNDCLASS_RWALKAET
 48  SITHSOUNDCLASS_LRUNAET
 49  SITHSOUNDCLASS_RRUNAET
```

### Water Entry, Swimming, And Treading

```text
 50  SITHSOUNDCLASS_ENTERWATER
 51  SITHSOUNDCLASS_ENTERWATERSLOW
 52  SITHSOUNDCLASS_EXITWATER
 53  SITHSOUNDCLASS_EXITWATERSLOW
 54  SITHSOUNDCLASS_LSWIMSURFACE
 55  SITHSOUNDCLASS_RSWIMSURFACE
 56  SITHSOUNDCLASS_TREADSURFACE
 57  SITHSOUNDCLASS_LSWIMUNDER
 58  SITHSOUNDCLASS_RSWIMUNDER
 59  SITHSOUNDCLASS_TREADUNDER
```

### Jumping And Landing

```text
 60  SITHSOUNDCLASS_JUMP
 61  SITHSOUNDCLASS_JUMPMETAL
 62  SITHSOUNDCLASS_JUMPWATER
 63  SITHSOUNDCLASS_JUMPEARTH
 64  SITHSOUNDCLASS_JUMPSNOW
 65  SITHSOUNDCLASS_JUMPWOOD
 66  SITHSOUNDCLASS_JUMPHARDECHO
 67  SITHSOUNDCLASS_JUMPWOODECHO
 68  SITHSOUNDCLASS_JUMPEARTHECHO
 69  SITHSOUNDCLASS_JUMPAET
 70  SITHSOUNDCLASS_LANDHARD
 71  SITHSOUNDCLASS_LANDMETAL
 72  SITHSOUNDCLASS_LANDWATER
 73  SITHSOUNDCLASS_LANDPUDDLE
 74  SITHSOUNDCLASS_LANDEARTH
 75  SITHSOUNDCLASS_LANDSNOW
 76  SITHSOUNDCLASS_LANDWOOD
 77  SITHSOUNDCLASS_LANDHARDECHO
 78  SITHSOUNDCLASS_LANDWOODECHO
 79  SITHSOUNDCLASS_LANDEARTHECHO
 80  SITHSOUNDCLASS_LANDAET
```

### Impact, Hurt, Death, And Breathing

```text
 81  SITHSOUNDCLASS_LANDHURT
 82  SITHSOUNDCLASS_HITHARD
 83  SITHSOUNDCLASS_HITMETAL
 84  SITHSOUNDCLASS_HITEARTH
 85  SITHSOUNDCLASS_DEFLECTED
 86  SITHSOUNDCLASS_SCRAPEHARD
 87  SITHSOUNDCLASS_SCRAPEMETAL
 88  SITHSOUNDCLASS_SCRAPEEARTH
 89  SITHSOUNDCLASS_HITDAMAGED
 90  SITHSOUNDCLASS_FALLING
 91  SITHSOUNDCLASS_CORPSEHIT
 92  SITHSOUNDCLASS_HURTIMPACT
 93  SITHSOUNDCLASS_HURTENERGY
 94  SITHSOUNDCLASS_HURTFIRE
 95  SITHSOUNDCLASS_HURTFISTS
 96  SITHSOUNDCLASS_HURTMACHETE
 97  SITHSOUNDCLASS_DROWNING
 98  SITHSOUNDCLASS_DEATH1
 99  SITHSOUNDCLASS_DEATH2
100  SITHSOUNDCLASS_DEATHUNDER
101  SITHSOUNDCLASS_DROWNED
102  SITHSOUNDCLASS_SPLATTERED
103  SITHSOUNDCLASS_PANT
104  SITHSOUNDCLASS_BREATH
105  SITHSOUNDCLASS_GASP
```

### Fire, Voice, Emotes, And Extra Traversal Sounds

```text
106  SITHSOUNDCLASS_FIRE1
107  SITHSOUNDCLASS_FIRE2
108  SITHSOUNDCLASS_FIRE3
109  SITHSOUNDCLASS_FIRE4
110  SITHSOUNDCLASS_CURIOUS
111  SITHSOUNDCLASS_ALERT
112  SITHSOUNDCLASS_IDLE
113  SITHSOUNDCLASS_GLOAT
114  SITHSOUNDCLASS_FEAR
115  SITHSOUNDCLASS_BOAST
116  SITHSOUNDCLASS_SLIDE1
117  SITHSOUNDCLASS_VICTORY
118  SITHSOUNDCLASS_HELP
119  SITHSOUNDCLASS_FLEE
120  SITHSOUNDCLASS_SEARCH
121  SITHSOUNDCLASS_CALM
122  SITHSOUNDCLASS_SURPRISE
123  SITHSOUNDCLASS_RESERVED1
124  SITHSOUNDCLASS_RESERVED2
125  SITHSOUNDCLASS_RESERVED3
126  SITHSOUNDCLASS_RESERVED4
127  SITHSOUNDCLASS_RESERVED5
128  SITHSOUNDCLASS_RESERVED6
129  SITHSOUNDCLASS_RESERVED7
130  SITHSOUNDCLASS_RESERVED8
131  SITHSOUNDCLASS_STAND2WALK
132  SITHSOUNDCLASS_WALK2STAND
133  SITHSOUNDCLASS_CLIMBHANDLEFT
134  SITHSOUNDCLASS_CLIMBHANDRIGHT
135  SITHSOUNDCLASS_CLIMBHORIZ
136  SITHSOUNDCLASS_CLIMBONTO
137  SITHSOUNDCLASS_CLIMBDOWNONTO
138  SITHSOUNDCLASS_LSWIMAET
139  SITHSOUNDCLASS_RSWIMAET
140  SITHSOUNDCLASS_TREADAET
```

## Master Game Modes

Used by:
- [GetSithMode](Functions-System.md#getsithmode)

| Value | Constant | Meaning |
| ---: | --- | --- |
| `0` | `SITH_MODE_CLOSED` | Closed game state. |
| `1` | `SITH_MODE_OPENED` | Opened or running game state. |
| `2` | `SITH_MODE_UNKNOWN_2` | Additional master-mode state preserved as named in the current source. |

## Difficulty Values

Used by:
- [GetDifficulty](Functions-System.md#getdifficulty)
- [UpdateDifficulty](Functions-Jones.md#updatedifficulty)

The current source uses raw difficulty values instead of a named enum. The active scalar tables define values `0` through `5`.

| Value | Current behavior in the code |
| ---: | --- |
| `0` | Lowest difficulty profile. Enemy hit accuracy scalar `0.5`, combat damage scalar `1.5`, IMP damage scalar `0.5`. |
| `1` | Easier-than-baseline profile. Enemy hit accuracy scalar `0.67`, combat damage scalar `1.33`, IMP damage scalar `0.6`. |
| `2` | Slightly easier-than-baseline profile. Enemy hit accuracy scalar `0.83`, combat damage scalar `1.17`, IMP damage scalar `0.7`. |
| `3` | Baseline profile. Enemy hit accuracy scalar `1.0`, combat damage scalar `1.0`, IMP damage scalar `0.8`. |
| `4` | Harder-than-baseline profile. Enemy hit accuracy scalar `1.17`, combat damage scalar `0.83`, IMP damage scalar `0.9`. |
| `5` | Highest difficulty profile used by the current scalar tables. Enemy hit accuracy scalar `1.33`, combat damage scalar `0.67`, IMP damage scalar `1.0`. |

## Actor State-Change Types

Used by:
- [SetThingStateChange](Functions-Thing.md#setthingstatechange)

| Value | Constant | Meaning |
| ---: | --- | --- |
| `-1` | `SITHACTORSTATECHANGE_NONE` | No pending state change. |
| `0` | `SITHACTORSTATECHANGE_ARMEDMODE` | Interpret the stored `state` payload as a puppet armed-mode value. |
| `1` | `SITHACTORSTATECHANGE_ANIMMOVE` | Interpret the stored `state` payload as [Actor Special-Move Flags](Flags.md#actor-special-move-flags). |
