# Thing Host Functions

Source: [`Libs/sith/Cog/sithCogFunctionThing.c`](../../Libs/sith/Cog/sithCogFunctionThing.c)

This is the largest host-function group in the current tree. It covers object creation, movement, health and damage, inventory bins, puppet/keyframe playback, attachment and mesh swapping, special FX, and camera helpers.

## Important Notes

- Many mutating thing verbs trigger network sync unless the current cog has [`SITHCOG_NOSYNC`](Flags.md#cog-script-flags) or the current message is `startup` / `shutdown`.
- Several verbs are object-type specific. Player-only, actor-only, and model/puppet-dependent calls are common in this module.
- The registered healing verb is spelled `healthing`, matching the legacy compatibility name.
- `isthingautoaiming` is still registered, but the source marks it as redundant.
- [`GetHeadLightIntensity()`](#getheadlightintensity) still pops and discards an extra legacy vector argument internally; the documented signature shows the intended script-facing form.
- [`CreatePolylineThing()`](#createpolylinething) accepts an optional destination thing in the current implementation. When omitted, the line uses `endPos` only.
- Signatures below use PascalCase for readability, even though host-function lookup in scripts is case-insensitive.

## Function Index

### Lifecycle, Damage, And Light

- [WaitForStop](#waitforstop)
- [WaitForAnimStop](#waitforanimstop)
- [StopThing](#stopthing)
- [DestroyThing](#destroything)
- [GetThingHealth](#getthinghealth)
- [GetThingMaxHealth](#getthingmaxhealth)
- [GetHealth](#gethealth)
- [Healthing](#healthing)
- [GetThingLight](#getthinglight)
- [SetThingLight](#setthinglight)
- [ThingLight](#thinglight)
- [ThingLightAnim](#thinglightanim)
- [ThingFadeAnim](#thingfadeanim)
- [CreateThing](#creatething)
- [CreateThingAtPos](#createthingatpos)
- [CaptureThing](#capturething)
- [ReleaseThing](#releasething)
- [DamageThing](#damagething)
- [SetLifeLeft](#setlifeleft)
- [GetLifeLeft](#getlifeleft)
- [SetThingHealth](#setthinghealth)
- [SetHealth](#sethealth)
- [AmputateJoint](#amputatejoint)
- [SetActorWeapon](#setactorweapon)
- [GetActorWeapon](#getactorweapon)
- [TakeItem](#takeitem)
- [HasLOS](#haslos)

### Motion, Transform, And Orientation

- [SetThingVel](#setthingvel)
- [AddThingVel](#addthingvel)
- [ApplyForce](#applyforce)
- [DetachThing](#detachthing)
- [GetAttachFlags](#getattachflags)
- [GetThingAttachFlags](#getthingattachflags)
- [AttachThingToSurf](#attachthingtosurf)
- [AttachThingToThing](#attachthingtothing)
- [AttachThingToThingEx](#attachthingtothingex)
- [GetThingAttachedThing](#getthingattachedthing)
- [SetArmedMode](#setarmedmode)
- [SetThingFlags](#setthingflags)
- [ClearThingFlags](#clearthingflags)
- [TeleportThing](#teleportthing)
- [SetThingType](#setthingtype)
- [SetCollideType](#setcollidetype)
- [SetHeadLightIntensity](#setheadlightintensity)
- [GetThingCurLightMode](#getthingcurlightmode)
- [SetThingCurLightMode](#setthingcurlightmode)
- [SetActorExtraSpeed](#setactorextraspeed)
- [SetThingPosEx](#setthingposex)
- [SetThingMaxVel](#setthingmaxvel)
- [SetThingMaxAngVel](#setthingmaxangvel)
- [SetThingJointAngle](#setthingjointangle)
- [SetThingMaxHeadPitch](#setthingmaxheadpitch)
- [SetThingMinHeadPitch](#setthingminheadpitch)
- [GetThingMaxHeadPitch](#getthingmaxheadpitch)
- [GetThingMinHeadPitch](#getthingminheadpitch)
- [SetThingMaxHeadYaw](#setthingmaxheadyaw)
- [GetThingMaxHeadYaw](#getthingmaxheadyaw)
- [SetThingLVecPYR](#setthinglvecpyr)
- [SetActorHeadPYR](#setactorheadpyr)
- [SetThingAirDrag](#setthingairdrag)
- [SetThingMaxRotVel](#setthingmaxrotvel)
- [SetThingMaxHeadVel](#setthingmaxheadvel)
- [ResetThing](#resetthing)
- [MoveThing](#movething)
- [MoveThingToPos](#movethingtopos)
- [GetThingType](#getthingtype)
- [IsThingMoving](#isthingmoving)
- [IsMoving](#ismoving)
- [GetCurFrame](#getcurframe)
- [GetGoalFrame](#getgoalframe)
- [GetThingParent](#getthingparent)
- [GetThingSector](#getthingsector)
- [GetThingPos](#getthingpos)
- [SetThingPos](#setthingpos)
- [GetThingVel](#getthingvel)
- [GetThingUVec](#getthinguvec)
- [GetThingLVec](#getthinglvec)
- [GetThingRVec](#getthingrvec)
- [GetThingFlags](#getthingflags)
- [GetCollideType](#getcollidetype)
- [GetHeadLightIntensity](#getheadlightintensity)
- [IsThingVisible](#isthingvisible)
- [GetThingGuid](#getthingguid)
- [GetGuidThing](#getguidthing)
- [GetThingMaxVel](#getthingmaxvel)
- [GetThingMaxAngVel](#getthingmaxangvel)
- [GetThingJointAngle](#getthingjointangle)
- [InterpolatePYR](#interpolatepyr)
- [GetThingLVecPYR](#getthinglvecpyr)
- [GetActorHeadPYR](#getactorheadpyr)
- [GetThingJointPos](#getthingjointpos)
- [IsThingModelName](#isthingmodelname)
- [GetThingMaxRotVel](#getthingmaxrotvel)
- [GetThingMaxHeadVel](#getthingmaxheadvel)
- [CopyOrient](#copyorient)
- [CopyOrientAndPos](#copyorientandpos)
- [GetThingInsertOffset](#getthinginsertoffset)
- [SetThingInsertOffset](#setthinginsertoffset)
- [GetThingEyeOffset](#getthingeyeoffset)
- [GetThingRotVel](#getthingrotvel)
- [SetThingRotVel](#setthingrotvel)
- [GetThingRotThrust](#getthingrotthrust)
- [SetThingRotThrust](#setthingrotthrust)
- [SetThingLook](#setthinglook)
- [SetThingHeadLookPos](#setthingheadlookpos)
- [SetThingHeadLookThing](#setthingheadlookthing)
- [IsThingCrouching](#isthingcrouching)
- [IsCrouching](#iscrouching)
- [GetThingRespawn](#getthingrespawn)
- [GetThingSignature](#getthingsignature)
- [GetThingUserData](#getthinguserdata)
- [SetThingUserData](#setthinguserdata)
- [GetThingCollideSize](#getthingcollidesize)
- [SetThingCollideSize](#setthingcollidesize)
- [GetThingMoveSize](#getthingmovesize)
- [SetThingMoveSize](#setthingmovesize)
- [GetThingMass](#getthingmass)
- [SetThingMass](#setthingmass)
- [CheckFloorDistance](#checkfloordistance)
- [CheckPathToPoint](#checkpathtopoint)
- [GetMoveStatus](#getmovestatus)
- [SetMoveMode](#setmovemode)
- [SetThingStateChange](#setthingstatechange)
- [BoardVehicle](#boardvehicle)
- [IsGhostVisible](#isghostvisible)

### Inventory, Timers, Physics, And Type Flags

- [SetThingPulse](#setthingpulse)
- [SetThingTimer](#setthingtimer)
- [GetInv](#getinv)
- [SetInv](#setinv)
- [ChangeInv](#changeinv)
- [GetInvCog](#getinvcog)
- [GetInvMin](#getinvmin)
- [GetInvMax](#getinvmax)
- [GetPhysicsFlags](#getphysicsflags)
- [SetPhysicsFlags](#setphysicsflags)
- [ClearPhysicsFlags](#clearphysicsflags)
- [GetThingClassCog](#getthingclasscog)
- [SetThingClassCog](#setthingclasscog)
- [GetThingCaptureCog](#getthingcapturecog)
- [SetThingCaptureCog](#setthingcapturecog)
- [SetThingAttachFlags](#setthingattachflags)
- [ClearThingAttachFlags](#clearthingattachflags)
- [GetParticleSize](#getparticlesize)
- [SetParticleSize](#setparticlesize)
- [GetParticleGrowthSpeed](#getparticlegrowthspeed)
- [SetParticleGrowthSpeed](#setparticlegrowthspeed)
- [GetParticleTimeoutRate](#getparticletimeoutrate)
- [SetParticleTimeoutRate](#setparticletimeoutrate)
- [GetTypeFlags](#gettypeflags)
- [SetTypeFlags](#settypeflags)
- [ClearTypeFlags](#cleartypeflags)
- [GetActorFlags](#getactorflags)
- [SetActorFlags](#setactorflags)
- [ClearActorFlags](#clearactorflags)
- [GetWeaponFlags](#getweaponflags)
- [SetWeaponFlags](#setweaponflags)
- [ClearWeaponFlags](#clearweaponflags)
- [GetExplosionFlags](#getexplosionflags)
- [SetExplosionFlags](#setexplosionflags)
- [ClearExplosionFlags](#clearexplosionflags)
- [GetItemFlags](#getitemflags)
- [SetItemFlags](#setitemflags)
- [ClearItemFlags](#clearitemflags)
- [GetParticleFlags](#getparticleflags)
- [SetParticleFlags](#setparticleflags)
- [ClearParticleFlags](#clearparticleflags)
- [GetThingFireOffset](#getthingfireoffset)
- [SetThingFireOffset](#setthingfireoffset)

### Keyframes, Puppets, Frames, And Path Movement

- [PlayKey](#playkey)
- [PlayKeyEx](#playkeyex)
- [StopKey](#stopkey)
- [PauseKey](#pausekey)
- [ResumeKey](#resumekey)
- [SetThingModel](#setthingmodel)
- [GetThingModel](#getthingmodel)
- [PlayMode](#playmode)
- [StopMode](#stopmode)
- [SynchMode](#synchmode)
- [IsModePlaying](#ismodeplaying)
- [PauseMode](#pausemode)
- [ResumeMode](#resumemode)
- [TrackToMode](#tracktomode)
- [WaitMode](#waitmode)
- [GetMajorMode](#getmajormode)
- [FirstThingInSector](#firstthinginsector)
- [NextThingInSector](#nextthinginsector)
- [PrevThingInSector](#prevthinginsector)
- [MoveToFrame](#movetoframe)
- [SkipToFrame](#skiptoframe)
- [JumpToFrame](#jumptoframe)
- [PathMovePause](#pathmovepause)
- [PathMoveResume](#pathmoveresume)
- [Rotate](#rotate)
- [RotatePivot](#rotatepivot)
- [RotateToPYR](#rotatetopyr)
- [GetThingTemplate](#getthingtemplate)
- [ParseArg](#parsearg)
- [SetPuppetModeFPS](#setpuppetmodefps)
- [StartQuetzAnim](#startquetzanim)
- [FadeInTrack](#fadeintrack)
- [PlayForceMoveMode](#playforcemovemode)

### Mesh, Camera, And FX Helpers

- [SyncThingPos](#syncthingpos)
- [SyncThingAttachment](#syncthingattachment)
- [SyncThingState](#syncthingstate)
- [GetMeshByName](#getmeshbyname)
- [GetNodeByName](#getnodebyname)
- [AttachThingToThingMesh](#attachthingtothingmesh)
- [DetachThingMesh](#detachthingmesh)
- [SetThingMesh](#setthingmesh)
- [RestoreThingMesh](#restorethingmesh)
- [GetThingAlpha](#getthingalpha)
- [SetThingAlpha](#setthingalpha)
- [GetCameraFOV](#getcamerafov)
- [SetCameraFOV](#setcamerafov)
- [ResetCameraFOV](#resetcamerafov)
- [SetCameraLookInterp](#setcameralookinterp)
- [SetCameraPosInterp](#setcameraposinterp)
- [SetCameraInterpSpeed](#setcamerainterpspeed)
- [SetCameraPosition](#setcameraposition)
- [AnimateSpriteSize](#animatespritesize)
- [GetCameraPosition](#getcameraposition)
- [SetCameraFadeThing](#setcamerafadething)
- [SetExtCamOffset](#setextcamoffset)
- [SetExtCamOffsetToThing](#setextcamoffsettothing)
- [SetExtCamLookOffsetToThing](#setextcamlookoffsettothing)
- [SetExtCamLookOffset](#setextcamlookoffset)
- [RestoreExtCam](#restoreextcam)
- [IsThingAutoAiming](#isthingautoaiming)
- [CreateLaser](#createlaser)
- [CreateLightning](#createlightning)
- [MakeFairyDust](#makefairydust)
- [MakeFairyDustDeluxe](#makefairydustdeluxe)
- [CreatePolylineThing](#createpolylinething)
- [MakeCamera2LikeCamera1](#makecamera2likecamera1)

## Function Reference

### Lifecycle, Damage, And Light


#### WaitForStop

```C++
WaitForStop(Thing thing)
```

Blocks the current COG until the `thing` stops path moving or rotating.

Parameters:
- `thing`: Path-move thing to wait on until it stops moving or rotating.

Notes:
- Requires a path-move thing.
- The current COG status is changed to wait for the thing to stop.
- Emits a debug console line when the current cog has the debug flag set.


#### WaitForAnimStop

```C++
WaitForAnimStop(int animID)
```

Blocks the current COG until the animation with id `animID` stops.

Parameters:
- `animID`: Animation id to wait on until it stops.

Notes:
- The current COG status is changed to wait for the animation to stop.
- Emits a debug console line when the current cog has the debug flag set.


#### StopThing

```C++
StopThing(Thing thing)
```

Stops movement on the `thing`.

Parameters:
- `thing`: Thing whose movement should be stopped.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### DestroyThing

```C++
DestroyThing(Thing thing)
```

Destroys `thing` and removes it from the world.

Parameters:
- `thing`: Thing to destroy.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.

#### GetThingHealth

```C++
GetThingHealth(Thing thing) -> float
```

Returns the current health value of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current health value.

Returns:
- Current health value of the `thing`.

Notes:
- Requires an actor or player thing.


#### GetThingMaxHealth

```C++
GetThingMaxHealth(Thing thing) -> float
```

Returns the current maximum health value of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current maximum health value.

Returns:
- Current maximum health value of the `thing`.

Notes:
- Requires an actor or player thing.


#### GetHealth

```C++
GetHealth(Thing thing) -> float
```

Returns the current health value of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current health value.

Returns:
- Current health value of the `thing`.

Notes:
- Shares the same implementation as [`GetThingHealth()`](#getthinghealth).
- Requires an actor or player thing.


#### Healthing

```C++
Healthing(Thing thing, float health)
```

Adds `health` to `thing`, clamping the result to the thing's maximum health.

Parameters:
- `thing`: Actor or player thing to heal.
- `health`: Positive amount of health to add.

Notes:
- Only affects actor and player things.
- Non-positive health values are ignored.

#### GetThingLight

```C++
GetThingLight(Thing thing) -> Vector
```

Returns the current RGB light color stored on `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current light.

Returns:
- Current RGB light color stored on the thing.

Notes:
- This verb returns only the light color vector. It does not return the light radius.


#### SetThingLight

```C++
SetThingLight(Thing thing, Vector color, float range, float litupTime)
```

Sets `thing`'s dynamic light immediately, or starts a one-shot light animation when `litupTime` is non-zero.

Parameters:
- `thing`: Thing whose light to set.
- `color`: Light RGB color.
- `range`: Light radius. The wrapper also stores this in the light alpha field.
- `litupTime`: Transition time in seconds. `0` applies the light immediately.

Notes:
- When applied immediately, the wrapper sets `minRadius = range` and `maxRadius = range * 1.1`.
- A non-zero color also sets the [`SITH_TF_EMITLIGHT`](Flags.md#thing-flags) flag.


#### ThingLight

```C++
ThingLight(Thing thing, Vector color, float range, float litupTime)
```

Sets `thing`'s dynamic light immediately, or starts a one-shot light animation when `litupTime` is non-zero.

Parameters:
- `thing`: Thing whose dynamic light should be updated.
- `color`: Light RGB color.
- `range`: Light radius. The wrapper also stores this in the light alpha field.
- `litupTime`: Transition time in seconds. `0` applies the light immediately.

Notes:
- When applied immediately, the wrapper sets `minRadius = range` and `maxRadius = range * 1.1`.
- A non-zero color also sets the [`SITH_TF_EMITLIGHT`](Flags.md#thing-flags) flag.

#### ThingLightAnim

```C++
ThingLightAnim(Thing thing, Vector startColor, float startRadius, Vector endColor, float endRadius, float speed) -> int
```

Starts a looping light animation on `thing` between the supplied start and end colors/radii.

Parameters:
- `thing`: Thing whose dynamic light should be animated.
- `startColor`: Starting light RGB color.
- `startRadius`: Starting light radius.
- `endColor`: Ending light RGB color.
- `endRadius`: Ending light radius.
- `speed`: Loop timing parameter in seconds. This value is split across the two interpolation legs, so each leg runs for half of the passed `speed` value.

Returns:
- Animation id, or `-1` on failure.

Notes:
- Requires a positive `speed`.

#### ThingFadeAnim

```C++
ThingFadeAnim(Thing thing, float startAlpha, float endAlpha, float timeDelta, int bLoop) -> int
```

Starts an alpha fade animation on `thing` from `startAlpha` to `endAlpha`.

Parameters:
- `thing`: Thing whose alpha should be animated.
- `startAlpha`: Starting alpha value.
- `endAlpha`: Ending alpha value.
- `timeDelta`: Fade duration in seconds.
- `bLoop`: Non-zero to loop the fade back and forth.

Returns:
- Animation id, or `-1` on failure.

Notes:
- For polyline things, the wrapper animates `face.extraLight.alpha`; for other things it animates the model color alpha.

#### CreateThing

```C++
CreateThing(Template template, Thing refThing) -> Thing
```

Creates a new thing from `template` using `refThing` as the spawn marker.

Parameters:
- `template`: Template to instantiate.
- `refThing`: Reference thing that provides spawn position, orientation, and sector.

Returns:
- Created thing, or `-1` on failure.

Notes:
- `refThing` must exist, must not be free/deleted, and must be in a sector.
- The wrapper sets the spawned thing move status to `SITHPLAYERMOVE_STILL`.
- When sync is enabled and the current message is not `startup` or `shutdown`, the creation is broadcast to joined players.

#### CreateThingAtPos

```C++
CreateThingAtPos(Template template, Sector sector, Vector pos, Vector pyr) -> Thing
```

Creates a new thing from `template` in `sector` at `pos` with orientation `pyr`.

Parameters:
- `template`: Template to instantiate.
- `sector`: Sector where the thing should be created.
- `pos`: Spawn position.
- `pyr`: Spawn orientation in pitch-yaw-roll degrees.

Returns:
- Created thing, or `-1` on failure.

Notes:
- The wrapper applies the template model/sprite insert offset before spawning.
- When sync is enabled and the current message is not `startup` or `shutdown`, the creation is broadcast to joined players.

#### CaptureThing

```C++
CaptureThing(Thing thing)
```

Captures `thing` for the current COG.

Parameters:
- `thing`: Thing whose capture-cog pointer should be set to the current COG.

Notes:
- The wrapper also sets the [`SITH_TF_COGLINKED`](Flags.md#thing-flags) flag.

#### ReleaseThing

```C++
ReleaseThing(Thing thing)
```

Releases the current cog's capture of `thing`.

Parameters:
- `thing`: Thing whose capture-cog pointer should be cleared.

Notes:
- If the thing is no longer linked to any cog afterward, the wrapper clears [`SITH_TF_COGLINKED`](Flags.md#thing-flags).

#### DamageThing

```C++
DamageThing(Thing victim, float damage, int damageClass, Thing source) -> float
```

Applies `damage` of `damageClass` to `victim` and returns the amount actually dealt.

Parameters:
- `victim`: Thing to damage.
- `damage`: Damage amount to apply. Values less than or equal to `0` are rejected.
- `damageClass`: Damage-class bitfield passed to the damage system. See [Damage Flags](Flags.md#damage-flags).
- `source`: Thing credited as the damage source. When omitted or invalid, the wrapper falls back to `victim`.

Returns:
- Actual damage dealt, or `-1` on invalid input.

Notes:
- On the host, synced games broadcast the damage event to joined players.

#### SetLifeLeft

```C++
SetLifeLeft(Thing thing, float secLeft)
```

Sets the life left of the `thing`.

Parameters:
- `thing`: Thing whose life left to set.
- `secLeft`: Remaining lifetime in seconds. The wrapper converts this to milliseconds and stores it in `thing->msecLifeLeft`.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetLifeLeft

```C++
GetLifeLeft(Thing thing) -> float
```

Returns the current life left of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current life left.

Returns:
- Current life left of the `thing`.


#### SetThingHealth

```C++
SetThingHealth(Thing thing, float health)
```

Sets the current health value of the `thing`.

Parameters:
- `thing`: Thing whose current health value to set.
- `health`: New health value to assign.


#### SetHealth

```C++
SetHealth(Thing thing, float health)
```

Sets the current health value of the `thing`.

Parameters:
- `thing`: Thing whose current health value to set.
- `health`: New health value to assign.

Notes:
- Shares the same implementation as [`SetThingHealth()`](#setthinghealth).


#### AmputateJoint

```C++
AmputateJoint(Thing thing, int joint)
```

Marks model hierarchy joint `joint` as amputated on `thing`.

Parameters:
- `thing`: Thing whose joint-amputation flags should be updated.
- `joint`: Model hierarchy node index to mark as amputated.

Notes:
- Requires a thing with a `Model3`.
- The wrapper checks `joint` against the model's `numHNodes`.


#### SetActorWeapon

```C++
SetActorWeapon(Thing actor, Template weapon)
```

Sets `weapon` as the actor weapon template for `actor`.

Parameters:
- `actor`: Actor or player thing whose actor-weapon template should be changed.
- `weapon`: Weapon template to assign.

Notes:
- Requires an actor or player thing.

#### GetActorWeapon

```C++
GetActorWeapon(Thing actor) -> Template
```

Returns the current actor-weapon template of `actor`.

Parameters:
- `actor`: Actor or player thing to inspect.

Returns:
- Actor-weapon template, or `-1` on failure.

Notes:
- Requires an actor or player thing.

#### TakeItem

```C++
TakeItem(Thing item, Thing thing)
```

Marks `item` as taken by `thing`.

Parameters:
- `item`: Item thing to take.
- `thing`: Thing that is taking the item.

Notes:
- Requires `item` to be an item thing.

#### HasLOS

```C++
HasLOS(Thing viewer, Thing target) -> int
```

Returns whether `viewer` currently has line of sight to `target`.

Parameters:
- `viewer`: Thing whose line of sight is being tested.
- `target`: Thing that must be visible in the viewer's line of sight.

Returns:
- `1` when line of sight is clear, `0` when it is blocked, and `-1` on invalid input.

### Motion, Transform, And Orientation


#### SetThingVel

```C++
SetThingVel(Thing thing, Vector vel)
```

Sets the velocity of the `thing`.

Parameters:
- `thing`: Thing whose velocity to set.
- `vel`: Velocity vector to apply.

Notes:
- Requires a physics thing.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### AddThingVel

```C++
AddThingVel(Thing thing, Vector vel)
```

Adds `vel` to the current physics velocity of `thing`.

Parameters:
- `thing`: Physics thing whose current velocity should be increased.
- `vel`: Velocity vector to apply.

Notes:
- Requires a physics thing.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ApplyForce

```C++
ApplyForce(Thing thing, Vector force)
```

Applies the force vector `force` to `thing`.

Parameters:
- `thing`: Thing that should receive the force.
- `force`: Force vector to apply.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.

#### DetachThing

```C++
DetachThing(Thing thing)
```

Detaches `thing` from whatever it is currently attached to.

Parameters:
- `thing`: Thing to detach.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.

#### GetAttachFlags

```C++
GetAttachFlags(Thing thing) -> int
```

Returns the current attach flags of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current attach flags.

Returns:
- Current attach flags of the `thing`. See [Attach Flags](Flags.md#attach-flags).


#### GetThingAttachFlags

```C++
GetThingAttachFlags(Thing thing) -> int
```

Returns the current attach flags of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current attach flags.

Returns:
- Current attach flags of the `thing`. See [Attach Flags](Flags.md#attach-flags).

Notes:
- Shares the same implementation as [`GetAttachFlags()`](#getattachflags).


#### AttachThingToSurf

```C++
AttachThingToSurf(Thing thing, Surface surf)
```

Attaches `thing` to `surf`.

Parameters:
- `thing`: Thing to attach.
- `surf`: Surface to attach the thing to.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the new attachment is broadcast to joined players.

#### AttachThingToThing

```C++
AttachThingToThing(Thing thing, Thing attachThing)
```

Attaches `thing` to `attachThing`.

Parameters:
- `thing`: Thing to attach.
- `attachThing`: Parent thing to attach to.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the new attachment is broadcast to joined players.

#### AttachThingToThingEx

```C++
AttachThingToThingEx(Thing thing, Thing attachThing, int attflags)
```

Attaches `thing` to `attachThing` and ORs `attFlags` into the resulting attachment flags.

Parameters:
- `thing`: Thing to attach.
- `attachThing`: Parent thing to attach to.
- `attFlags`: Additional attachment-flag bits to set after attaching. See [Attach Flags](Flags.md#attach-flags).

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the new attachment is broadcast to joined players.

#### GetThingAttachedThing

```C++
GetThingAttachedThing(Thing thing, int attype) -> Thing
```

Returns the current attached thing of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current attached thing.
- `attype`: Thing type to filter attached children by. Pass `-1` to return the first attached thing regardless of type.

Returns:
- First attached thing matching `attype`, or `-1` when no matching attached thing exists.


#### SetArmedMode

```C++
SetArmedMode(Thing thing, int armedMode)
```

Sets the puppet armed mode of the `thing`.

Parameters:
- `thing`: Thing whose puppet armed mode to set.
- `armedMode`: New puppet armed-mode value. See [Puppet Armed Modes](Types-And-Modes.md#puppet-armed-modes).

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### SetThingFlags

```C++
SetThingFlags(Thing thing, int flags)
```

Sets the flags of the `thing`.

Parameters:
- `thing`: Thing whose flags to set.
- `flags`: Thing-flag bitmask to OR into `thing->flags`. See [Thing Flags](Flags.md#thing-flags).

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ClearThingFlags

```C++
ClearThingFlags(Thing thing, int flags)
```

Clears the specified flags on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified flags.
- `flags`: Thing-flag bitmask to clear from `thing->flags`. See [Thing Flags](Flags.md#thing-flags).

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### TeleportThing

```C++
TeleportThing(Thing destThing, Thing srcThing)
```

Teleports `destThing` to the position, orientation, and sector of `srcThing`.

Parameters:
- `destThing`: Thing that should be moved.
- `srcThing`: Thing whose position/orientation should be copied.

Notes:
- If `destThing` is attached, the wrapper detaches it first.
- Physics things with [`SITH_PF_FLOORSTICK`](Flags.md#physics-flags) re-find the floor after teleporting.
- When `destThing` is the local player, the current camera is updated immediately.
- When sync is enabled and the current message is not `startup` or `shutdown`, the new position is broadcast to joined players.

#### SetThingType

```C++
SetThingType(Thing thing, int type)
```

Sets the type of the `thing`.

Parameters:
- `thing`: Thing whose type to set.
- `type`: New thing-type value to store in `thing->type`. See [Thing Types](Types-And-Modes.md#thing-types).

Notes:
- Valid values are the runtime [Thing Types](Types-And-Modes.md#thing-types).
- The wrapper updates only the runtime type field and does not resynchronize the change.


#### SetCollideType

```C++
SetCollideType(Thing thing, int type)
```

Sets the collide type of the `thing`.

Parameters:
- `thing`: Thing whose collide type to set.
- `type`: New collide-type value to store in `thing->collide.type`. See [Collide Types](Types-And-Modes.md#collide-types).

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### SetHeadLightIntensity

```C++
SetHeadLightIntensity(Thing thing, Vector color) -> Vector
```

Sets the current headlight color or intensity vector of the `thing`.

Parameters:
- `thing`: Actor or player thing whose headlight intensity to set.
- `color`: New headlight color or intensity vector to store.

Returns:
- Applied headlight color or intensity vector.

Notes:
- Requires an actor or player thing.


#### GetThingCurLightMode

```C++
GetThingCurLightMode(Thing thing) -> int
```

Returns the current light mode of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current light mode.

Returns:
- Current render light-mode value of the `thing`. See [Render Light Modes](Types-And-Modes.md#render-light-modes).


#### SetThingCurLightMode

```C++
SetThingCurLightMode(Thing thing, int mode)
```

Sets the current light mode of the `thing`.

Parameters:
- `thing`: Thing whose current light mode to set.
- `mode`: New render light-mode value to store in `thing->renderData.lightMode`. See [Render Light Modes](Types-And-Modes.md#render-light-modes).


#### SetActorExtraSpeed

```C++
SetActorExtraSpeed(Thing thing, float speed)
```

Sets the extra speed of the `thing`.

Parameters:
- `thing`: Thing whose extra speed to set.
- `speed`: New extra-speed scalar stored in `thing->thingInfo.actorInfo.extraSpeed`.

Notes:
- Requires an actor or player thing.


#### SetThingPosEx

```C++
SetThingPosEx(Thing thing, Vector newPos, Sector sector) -> int
```

Sets `thing` to the world position `newPos`, optionally forcing it into `sector`.

Parameters:
- `thing`: Thing to reposition.
- `newPos`: New position vector to apply.
- `sector`: Sector that should contain the thing after the move. If null or invalid, the wrapper searches for the sector at `newPos`.

Returns:
- `1` on success, or `0` when `thing` or `newPos` is invalid.

Notes:
- Physics things with [`SITH_PF_FLOORSTICK`](Flags.md#physics-flags) re-run floor detection after the move.
- When `thing` is the local player, the current camera is updated immediately.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### SetThingMaxVel

```C++
SetThingMaxVel(Thing thing, float maxSpeed)
```

Sets the maximum velocity of the `thing`.

Parameters:
- `thing`: Thing whose maximum velocity to set.
- `maxSpeed`: New maximum linear velocity stored in `thing->moveInfo.physics.maxVelocity`.

Notes:
- Requires a physics thing.


#### SetThingMaxAngVel

```C++
SetThingMaxAngVel(Thing thing, float maxRotSpeed)
```

Sets the maximum angular velocity of the `thing`.

Parameters:
- `thing`: Thing whose maximum angular velocity to set.
- `maxRotSpeed`: New maximum angular velocity stored in `thing->moveInfo.physics.maxRotationVelocity`.

Notes:
- Requires a physics thing.


#### SetThingJointAngle

```C++
SetThingJointAngle(Thing thing, int jointNum, float angle)
```

Sets the joint angle of the `thing`.

Parameters:
- `thing`: Actor, player, or corpse thing whose tweaked joint pitch should be updated.
- `jointNum`: Puppet joint index. The wrapper resolves this through `thing->pPuppetClass->aJoints[]` to find the actual model hierarchy node.
- `angle`: New pitch angle in degrees for the resolved tweaked joint.

Notes:
- Requires an actor, player, or corpse with a puppet class, a model, and allocated tweaked-angle storage.


#### SetThingMaxHeadPitch

```C++
SetThingMaxHeadPitch(Thing thing, float maxPitch) -> float
```

Sets the maximum head pitch of the `thing`.

Parameters:
- `thing`: Thing whose maximum head pitch to set.
- `maxPitch`: New upper head-pitch limit in degrees.

Returns:
- Previous maximum head-pitch limit, or `-1.0` when `thing` is invalid.


#### SetThingMinHeadPitch

```C++
SetThingMinHeadPitch(Thing thing, float minPitch) -> float
```

Sets the minimum head pitch of the `thing`.

Parameters:
- `thing`: Thing whose minimum head pitch to set.
- `minPitch`: New lower head-pitch limit in degrees.

Returns:
- Previous minimum head-pitch limit, or `-1.0` when `thing` is invalid.


#### GetThingMaxHeadPitch

```C++
GetThingMaxHeadPitch(Thing thing) -> float
```

Returns the current maximum head pitch of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current maximum head pitch.

Returns:
- Current maximum head pitch of the `thing`.


#### GetThingMinHeadPitch

```C++
GetThingMinHeadPitch(Thing thing) -> float
```

Returns the current minimum head pitch of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current minimum head pitch.

Returns:
- Current minimum head pitch of the `thing`.


#### SetThingMaxHeadYaw

```C++
SetThingMaxHeadYaw(Thing thing, float maxYaw) -> float
```

Sets the maximum head yaw of the `thing`.

Parameters:
- `thing`: Thing whose maximum head yaw to set.
- `maxYaw`: New absolute head-yaw limit in degrees.

Returns:
- Previous maximum head-yaw limit, or `-1.0` when `thing` is invalid.


#### GetThingMaxHeadYaw

```C++
GetThingMaxHeadYaw(Thing thing) -> float
```

Returns the current maximum head yaw of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current maximum head yaw.

Returns:
- Current maximum head yaw of the `thing`.


#### SetThingLVecPYR

```C++
SetThingLVecPYR(Thing thing, Vector pyr)
```

Builds a new orientation for `thing` from the supplied pitch-yaw-roll angles.

Parameters:
- `thing`: Thing whose orientation should be replaced.
- `pyr`: Pitch-yaw-roll angles in degrees.

Notes:
- Despite the verb name, the input vector is interpreted as PYR angles, not as a raw left-vector direction.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### SetActorHeadPYR

```C++
SetActorHeadPYR(Thing thing, Vector pyr)
```

Sets the head PYR orientation of the `thing`.

Parameters:
- `thing`: Thing whose head PYR orientation to set.
- `pyr`: Desired head pitch-yaw-roll angles in degrees. The wrapper clamps pitch and yaw to the thing's configured head limits before applying them.

Notes:
- Requires an actor or player thing with [`SITH_AF_CANROTATEHEAD`](Flags.md#actor-flags).


#### SetThingAirDrag

```C++
SetThingAirDrag(Thing thing, float drag)
```

Sets the air drag of the `thing`.

Parameters:
- `thing`: Thing whose air drag to set.
- `drag`: New air-drag coefficient stored in `thing->moveInfo.physics.airDrag`.

Notes:
- Requires a physics thing.


#### SetThingMaxRotVel

```C++
SetThingMaxRotVel(Thing thing, float vel)
```

Sets the maximum rotation velocity of the `thing`.

Parameters:
- `thing`: Thing whose maximum rotation velocity to set.
- `vel`: New maximum body-rotation velocity.

Notes:
- Requires an actor or player thing.


#### SetThingMaxHeadVel

```C++
SetThingMaxHeadVel(Thing thing, float vel)
```

Sets the maximum head velocity of the `thing`.

Parameters:
- `thing`: Thing whose maximum head velocity to set.
- `vel`: New maximum head-rotation velocity.

Notes:
- Requires an actor or player thing.


#### ResetThing

```C++
ResetThing(Thing thing)
```

Resets movement and animation state on `thing`.

Parameters:
- `thing`: Thing whose movement and puppet state should be reset.

Notes:
- Requires a thing with puppet data.
- The wrapper resets physics movement, removes all puppet tracks, and sets `moveStatus` to `SITHPLAYERMOVE_STILL`.


#### MoveThing

```C++
MoveThing(Thing thing, Vector vecDirection, float moveDist, float timeDelta) -> int
```

Moves `thing` along a direction vector by a fixed distance.

Parameters:
- `thing`: Thing to move.
- `vecDirection`: World-space direction vector to move along.
- `moveDist`: Distance to move along `vecDirection`.
- `timeDelta`: Duration in seconds. Values greater than `0` start a move animation; values less than or equal to `0` move immediately through the collision system.

Returns:
- Animation id when a timed move animation starts, or `-1` for immediate moves and error cases.


#### MoveThingToPos

```C++
MoveThingToPos(Thing thing, Vector pos, float time) -> int
```

Moves `thing` toward a target world position.

Parameters:
- `thing`: Thing to move.
- `pos`: Target world position.
- `time`: Duration in seconds. Values greater than `0` start a move-to-position animation; values less than or equal to `0` move immediately through the collision system.

Returns:
- Animation id when a timed move animation starts, or `-1` for immediate moves and failure cases.


#### GetThingType

```C++
GetThingType(Thing thing) -> int
```

Returns the current type of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current type.

Returns:
- Current thing-type value of the `thing`. See [Thing Types](Types-And-Modes.md#thing-types).


#### IsThingMoving

```C++
IsThingMoving(Thing thing) -> int
```

Returns whether the `thing` is currently moving or turning.

Parameters:
- `thing`: Thing for which to test the requested moving state.

Returns:
- Non-zero when the condition is true, otherwise `0`.


#### IsMoving

```C++
IsMoving(Thing thing) -> int
```

Returns whether the `thing` currently has the requested moving state.

Parameters:
- `thing`: Thing for which to test the requested moving state.

Returns:
- Non-zero when the condition is true, otherwise `0`.

Notes:
- Shares the same implementation as [`IsThingMoving()`](#isthingmoving).


#### GetCurFrame

```C++
GetCurFrame(Thing thing) -> int
```

Returns the current path frame index of the `thing`.

Parameters:
- `thing`: Path-moving thing for which to retrieve the current frame index.

Returns:
- Current path frame index of the `thing`.

Notes:
- Requires a path-move thing.


#### GetGoalFrame

```C++
GetGoalFrame(Thing thing) -> int
```

Returns the current goal frame of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current goal frame.

Returns:
- Current goal frame of the `thing`.

Notes:
- Requires a path-move thing.


#### GetThingParent

```C++
GetThingParent(Thing thing) -> Thing
```

Returns the current parent of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current parent.

Returns:
- Current parent of the `thing`.


#### GetThingSector

```C++
GetThingSector(Thing thing) -> Sector
```

Returns the current sector of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current sector.

Returns:
- Current sector of the `thing`.


#### GetThingPos

```C++
GetThingPos(Thing thing) -> Vector
```

Returns the current position of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current position.

Returns:
- Current position of the `thing`.


#### SetThingPos

```C++
SetThingPos(Thing thing, Vector newPos) -> int
```

Sets the position of the `thing`.

Parameters:
- `thing`: Thing whose position to set.
- `newPos`: New position vector to apply.

Returns:
- Integer result returned by the engine.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetThingVel

```C++
GetThingVel(Thing thing) -> Vector
```

Returns the current velocity of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current velocity.

Returns:
- Current velocity of the `thing`.


#### GetThingUVec

```C++
GetThingUVec(Thing thing) -> Vector
```

Returns the current up vector of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current up vector.

Returns:
- Current up vector of the `thing`.


#### GetThingLVec

```C++
GetThingLVec(Thing thing) -> Vector
```

Returns the current left vector of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current left vector.

Returns:
- Current left vector of the `thing`.


#### GetThingRVec

```C++
GetThingRVec(Thing thing) -> Vector
```

Returns the current right vector of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current right vector.

Returns:
- Current right vector of the `thing`.


#### GetThingFlags

```C++
GetThingFlags(Thing thing) -> int
```

Returns the current flags of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current flags.

Returns:
- Current flags of the `thing`.


#### GetCollideType

```C++
GetCollideType(Thing thing) -> int
```

Returns the current collide type of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current collide type.

Returns:
- Current collide-type value of the `thing`. See [Collide Types](Types-And-Modes.md#collide-types).


#### GetHeadLightIntensity

```C++
GetHeadLightIntensity(Thing thing) -> Vector
```

Returns the current headlight color or intensity vector of the `thing`.

Parameters:
- `thing`: Actor or player thing for which to retrieve the headlight intensity.

Returns:
- Current headlight color or intensity vector of the `thing`.

Notes:
- Requires an actor or player thing.
- The current implementation still pops and discards a legacy extra vector internally.


#### IsThingVisible

```C++
IsThingVisible(Thing thing) -> int
```

Returns whether the `thing` currently has the requested visible state.

Parameters:
- `thing`: Thing for which to test the requested visible state.

Returns:
- Non-zero when the condition is true, otherwise `0`.


#### GetThingGuid

```C++
GetThingGuid(Thing thing) -> int
```

Returns the current GUID of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current GUID.

Returns:
- Current GUID of the `thing`.


#### GetGuidThing

```C++
GetGuidThing(int guid) -> Thing
```

Resolves a runtime GUID back to its current thing reference.

Parameters:
- `guid`: Runtime thing GUID to resolve.

Returns:
- Thing matching that GUID, or `-1` when the GUID is invalid.

#### GetThingMaxVel

```C++
GetThingMaxVel(Thing thing) -> float
```

Returns the current maximum velocity of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current maximum velocity.

Returns:
- Current maximum velocity of the `thing`.

Notes:
- Requires a physics thing.


#### GetThingMaxAngVel

```C++
GetThingMaxAngVel(Thing thing) -> float
```

Returns the current maximum angular velocity of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current maximum angular velocity.

Returns:
- Current maximum angular velocity of the `thing`.

Notes:
- Requires a physics thing.


#### GetThingJointAngle

```C++
GetThingJointAngle(Thing thing, int jointNum) -> float
```

Returns the current joint angle of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current tweaked joint pitch.
- `jointNum`: Puppet joint index. The wrapper resolves this through `thing->pPuppetClass->aJoints[]` to find the actual model hierarchy node.

Returns:
- Current tweaked pitch angle for the resolved joint, or `-1.0` when the joint cannot be resolved.

Notes:
- Requires an actor, player, or corpse with a puppet class, a model, and allocated tweaked-angle storage.


#### InterpolatePYR

```C++
InterpolatePYR(Vector axisX, Vector axisY, Vector axisZ, float angle) -> Vector
```

Builds two look directions from `(axisX -> axisY)` and `(axisX -> axisZ)`, converts them to PYR angles, and linearly interpolates between those angles.

Parameters:
- `axisX`: First reference point.
- `axisY`: Second reference point defining the first look direction.
- `axisZ`: Third reference point defining the second look direction.
- `angle`: Interpolation factor passed to `rdVector_Lerp3()`.

Returns:
- Interpolated PYR vector.

#### GetThingLVecPYR

```C++
GetThingLVecPYR(Thing thing) -> Vector
```

Returns the current orientation of `thing` as extracted pitch-yaw-roll angles.

Parameters:
- `thing`: Thing whose current orientation should be converted to pitch-yaw-roll angles.

Returns:
- Current pitch-yaw-roll orientation of the thing.


#### GetActorHeadPYR

```C++
GetActorHeadPYR(Thing thing) -> Vector
```

Returns the current head PYR orientation of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current head PYR orientation.

Returns:
- Current head PYR orientation of the `thing`.


#### GetThingJointPos

```C++
GetThingJointPos(Thing thing, int jointNum) -> Vector
```

Returns the current joint position of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current joint position.
- `jointNum`: Model hierarchy node index whose current joint matrix position should be returned.

Returns:
- Current joint position of the requested model hierarchy node.

Notes:
- Requires a thing with puppet data and a valid model hierarchy node index.


#### IsThingModelName

```C++
IsThingModelName(Thing thing, string modelName) -> int
```

Checks whether `thing`'s current model name matches `modelName`.

Parameters:
- `thing`: Thing whose current model name should be compared.
- `modelName`: Model name to compare against the thing's current `Model3` name.

Returns:
- `1` when the current model name matches, otherwise `0`.

Notes:
- The current model-name comparison is case-sensitive.


#### GetThingMaxRotVel

```C++
GetThingMaxRotVel(Thing thing) -> float
```

Returns the current maximum rotation velocity of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current maximum rotation velocity.

Returns:
- Current maximum rotation velocity of the `thing`.

Notes:
- Requires an actor or player thing.


#### GetThingMaxHeadVel

```C++
GetThingMaxHeadVel(Thing thing) -> float
```

Returns the current maximum head velocity of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current maximum head velocity.

Returns:
- Current maximum head velocity of the `thing`.

Notes:
- Requires an actor or player thing.


#### CopyOrient

```C++
CopyOrient(Thing srcThing, Thing destThing)
```

Copies the orientation from `srcThing` to `destThing`.

Parameters:
- `srcThing`: Source thing from which to copy the orientation.
- `destThing`: Destination thing that should receive the copied orientation.


#### CopyOrientAndPos

```C++
CopyOrientAndPos(Thing srcThing, Thing dstThing)
```

Copies the orientation and position from `srcThing` to `dstThing`.

Parameters:
- `srcThing`: Source thing from which to copy the orientation and position.
- `dstThing`: Destination thing that should receive the copied orientation and position.


#### GetThingInsertOffset

```C++
GetThingInsertOffset(Thing thing) -> Vector
```

Returns the current insert offset of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current insert offset.

Returns:
- Current insert offset of the `thing`.


#### SetThingInsertOffset

```C++
SetThingInsertOffset(Thing thing, Vector offset) -> Vector
```

Sets the insert offset of the `thing`.

Parameters:
- `thing`: Thing whose insert offset to set.
- `offset`: New model insert offset to store in `thing`'s current `Model3`.

Returns:
- The applied insert offset, or the zero vector on error.

Notes:
- Requires a thing with a current `Model3`.


#### GetThingEyeOffset

```C++
GetThingEyeOffset(Thing thing) -> Vector
```

Returns the current eye offset of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current eye offset.

Returns:
- Current eye offset of the `thing`.


#### GetThingRotVel

```C++
GetThingRotVel(Thing thing) -> Vector
```

Returns the current rotation velocity of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current rotation velocity.

Returns:
- Current rotation velocity of the `thing`.

Notes:
- Requires a physics thing.


#### SetThingRotVel

```C++
SetThingRotVel(Thing thing, Vector vel)
```

Sets the rotation velocity of the `thing`.

Parameters:
- `thing`: Thing whose rotation velocity to set.
- `vel`: Velocity vector to apply.

Notes:
- Requires a physics thing.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetThingRotThrust

```C++
GetThingRotThrust(Thing thing) -> Vector
```

Returns the current rotation thrust of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current rotation thrust.

Returns:
- Current rotation thrust of the `thing`.

Notes:
- Requires a physics thing.


#### SetThingRotThrust

```C++
SetThingRotThrust(Thing thing, Vector thrust)
```

Sets the rotation thrust of the `thing`.

Parameters:
- `thing`: Thing whose rotation thrust to set.
- `thrust`: Thrust vector to apply.

Notes:
- Requires a physics thing.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### SetThingLook

```C++
SetThingLook(Thing thing, Vector look)
```

Sets the look of the `thing`.

Parameters:
- `thing`: Thing whose look to set.
- `look`: World-space look direction. The wrapper normalizes it and rebuilds the full orientation matrix from that direction.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### SetThingHeadLookPos

```C++
SetThingHeadLookPos(Thing thing, Vector look)
```

Sets the head look position of the `thing`.

Parameters:
- `thing`: Thing whose head look position to set.
- `look`: World-space position the thing's head should attempt to face.

Notes:
- Requires an actor or player thing with [`SITH_AF_CANROTATEHEAD`](Flags.md#actor-flags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### SetThingHeadLookThing

```C++
SetThingHeadLookThing(Thing thing, Thing lookThing)
```

Sets the head look thing of the `thing`.

Parameters:
- `thing`: Thing whose head look thing to set.
- `lookThing`: Thing whose current position should be used as the head-look target.

Notes:
- Requires an actor or player thing with [`SITH_AF_CANROTATEHEAD`](Flags.md#actor-flags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### IsThingCrouching

```C++
IsThingCrouching(Thing thing) -> int
```

Returns whether the `thing` currently has the requested crouching state.

Parameters:
- `thing`: Thing for which to test the requested crouching state.

Returns:
- Non-zero when the condition is true, otherwise `0`.

Notes:
- Requires a physics thing.


#### IsCrouching

```C++
IsCrouching(Thing thing) -> int
```

Returns whether the `thing` currently has the requested crouching state.

Parameters:
- `thing`: Thing for which to test the requested crouching state.

Returns:
- Non-zero when the condition is true, otherwise `0`.

Notes:
- Shares the same implementation as [`IsThingCrouching()`](#isthingcrouching).
- Requires a physics thing.


#### GetThingRespawn

```C++
GetThingRespawn(Thing thing) -> float
```

Returns the item respawn interval of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current respawn interval.

Returns:
- Respawn interval of the item `thing` in seconds.

Notes:
- Requires an item thing.


#### GetThingSignature

```C++
GetThingSignature(Thing thing) -> int
```

Returns the current signature of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current signature.

Returns:
- Current signature of the `thing`.


#### GetThingUserData

```C++
GetThingUserData(Thing thing) -> float
```

Returns the current user data of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current user data.

Returns:
- Current user data of the `thing`.


#### SetThingUserData

```C++
SetThingUserData(Thing thing, float userval)
```

Sets the user data of the `thing`.

Parameters:
- `thing`: Thing whose user data to set.
- `userval`: New floating-point user value stored in `thing->userval`.


#### GetThingCollideSize

```C++
GetThingCollideSize(Thing thing) -> float
```

Returns the current collide size of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current collide size.

Returns:
- Current collide size of the `thing`.


#### SetThingCollideSize

```C++
SetThingCollideSize(Thing thing, float size)
```

Sets the collide size of the `thing`.

Parameters:
- `thing`: Thing whose collide size to set.
- `size`: New collision size stored in `thing->collide.size`.


#### GetThingMoveSize

```C++
GetThingMoveSize(Thing thing) -> float
```

Returns the current move size of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current move size.

Returns:
- Current move size of the `thing`.


#### SetThingMoveSize

```C++
SetThingMoveSize(Thing thing, float movesize)
```

Sets the move size of the `thing`.

Parameters:
- `thing`: Thing whose move size to set.
- `movesize`: New movement/collision clearance size stored in `thing->collide.movesize`.


#### GetThingMass

```C++
GetThingMass(Thing thing) -> float
```

Returns the current mass of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current mass.

Returns:
- Current mass of the `thing`.

Notes:
- Requires a physics thing.


#### SetThingMass

```C++
SetThingMass(Thing thing, float mass)
```

Sets the mass of the `thing`.

Parameters:
- `thing`: Thing whose mass to set.
- `mass`: New physics mass stored in `thing->moveInfo.physics.mass`.

Notes:
- Requires a physics thing.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### CheckFloorDistance

```C++
CheckFloorDistance(Thing thing) -> float
```

Returns the distance from `thing` straight down to the floor.

Parameters:
- `thing`: Thing to test against the floor.

Returns:
- Distance to the floor, or `-1.0` on invalid input.

#### CheckPathToPoint

```C++
CheckPathToPoint(Thing viewer, Vector vecTarget, int bDetectThings, int bSkipFloor) -> float
```

Checks the path from `viewer` to `targetPos` and returns the reachable distance.

Parameters:
- `viewer`: Thing whose pathing/collision size should be used.
- `targetPos`: World position to test.
- `bDetectThings`: Non-zero to include things in the collision/path test.
- `bSkipFloor`: Non-zero to skip floor checks during the test.

Returns:
- Distance reported by the AI path-check helper, or `0.0` on invalid input.

#### GetMoveStatus

```C++
GetMoveStatus(Thing thing) -> int
```

Returns the current move status of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current move status.

Returns:
- Current runtime move-status value of the `thing`. See [Thing Move Status Values](Types-And-Modes.md#thing-move-status-values).


#### SetMoveMode

```C++
SetMoveMode(Thing thing, int newMode) -> int
```

Sets the puppet move mode of the `thing`.

Parameters:
- `thing`: Thing whose move mode to set.
- `newMode`: New puppet move-mode value to apply. See [Puppet Move Modes](Types-And-Modes.md#puppet-move-modes).

Returns:
- `0` on success, `-1` on error.


#### SetThingStateChange

```C++
SetThingStateChange(Thing thing, int state, int type)
```

Stores a pending actor/player state-change request on `thing`.

Parameters:
- `thing`: Actor or player thing whose state-change payload should be updated.
- `state`: State payload value written into `stateChange.params.moveFlags`. When `type` is [Actor State-Change Types](Types-And-Modes.md#actor-state-change-types) `SITHACTORSTATECHANGE_ANIMMOVE`, this is typically a combination of [Actor Special-Move Flags](Flags.md#actor-special-move-flags). When `type` is `SITHACTORSTATECHANGE_ARMEDMODE`, this is a [Puppet Armed Modes](Types-And-Modes.md#puppet-armed-modes) value.
- `type`: State-change type selector. See [Actor State-Change Types](Types-And-Modes.md#actor-state-change-types).

Notes:
- Requires an actor or player thing.
- The wrapper stores the numeric payload in `stateChange.params.moveFlags`; depending on `type`, game code may interpret that payload as move flags or armed-mode state.

#### BoardVehicle

```C++
BoardVehicle(Thing thing) -> int
```

Attempts to board a vehicle with `thing`.

Parameters:
- `thing`: Player thing that should board a vehicle.

Returns:
- Non-zero when boarding succeeded, otherwise `0`.

Notes:
- Requires a player thing.

#### IsGhostVisible

```C++
IsGhostVisible(Thing thing, Thing ghostThing, float angle) -> int
```

Checks whether `ghostThing` lies inside `thing`'s forward visibility cone.

Parameters:
- `thing`: Viewer thing whose forward direction should be tested.
- `ghostThing`: Target thing that must lie within the viewer's forward visibility cone.
- `angle`: Forward-cone half-angle in degrees. Valid values are `1.0` through `90.0`.

Returns:
- `1` when `ghostThing` lies within the forward cone, `0` when it does not, and `-1` on invalid input.

### Inventory, Timers, Physics, And Type Flags


#### SetThingPulse

```C++
SetThingPulse(Thing thing, float secPulse)
```

Enables or disables the per-thing pulse timer on `thing`.

Parameters:
- `thing`: Thing whose pulse timer should be updated.
- `secPulse`: Pulse interval in seconds. `0.0` disables pulsing.

Notes:
- A non-zero interval sets [`SITH_TF_PULSESET`](Flags.md#thing-flags), stores the interval in milliseconds, and schedules the next pulse time.

#### SetThingTimer

```C++
SetThingTimer(Thing thing, float secTimer)
```

Enables or disables the one-shot per-thing timer on `thing`.

Parameters:
- `thing`: Thing whose timer should be updated.
- `secTimer`: Timer delay in seconds. `0.0` disables the timer.

Notes:
- A non-zero delay sets [`SITH_TF_TIMERSET`](Flags.md#thing-flags) and stores an absolute timeout in milliseconds.

#### GetInv

```C++
GetInv(Thing thing, int typeId) -> float
```

Returns the current amount stored in inventory type `typeId` for the player `thing`.

Parameters:
- `thing`: Player thing for which to retrieve the inventory amount.
- `typeId`: Inventory type id to query.

Returns:
- Current amount stored in inventory type `typeId` for the player `thing`.

Notes:
- Requires a player thing with player data.


#### SetInv

```C++
SetInv(Thing thing, int typeId, float amount)
```

Sets the current amount stored in inventory type `typeId` for the player `thing`.

Parameters:
- `thing`: Player thing whose inventory amount to set.
- `typeId`: Inventory type id to update.
- `amount`: Amount to set or add.

Notes:
- Requires a player thing with player data.


#### ChangeInv

```C++
ChangeInv(Thing thing, int typeId, float amount) -> float
```

Adds `amount` to inventory type `typeId` for the player `thing` and returns the new amount.

Parameters:
- `thing`: Player thing whose inventory amount to change.
- `typeId`: Inventory type id to update.
- `amount`: Amount to set or add.

Returns:
- New amount stored in inventory type `typeId` after the change.

Notes:
- Requires a player thing with player data.


#### GetInvCog

```C++
GetInvCog(Thing thing, int typeId) -> Cog
```

Returns the class cog associated with inventory type `typeId` for the player `thing`.

Parameters:
- `thing`: Player thing for which to retrieve the inventory class cog.
- `typeId`: Inventory type id whose class cog should be returned.

Returns:
- Inventory class cog for inventory type `typeId`, or `-1` when none is assigned.

Notes:
- Requires a player thing with player data.


#### GetInvMin

```C++
GetInvMin(Thing thing, int bin) -> float
```

Returns the configured minimum value for inventory bin `bin` on the player `thing`.

Parameters:
- `thing`: Player thing for which to retrieve the inventory minimum.
- `bin`: Inventory bin id to query.

Returns:
- Configured minimum value of inventory bin `bin` on the player `thing`.

Notes:
- Requires a player thing with player data.


#### GetInvMax

```C++
GetInvMax(Thing thing, int bin) -> float
```

Returns the configured maximum value for inventory bin `bin` on the player `thing`.

Parameters:
- `thing`: Player thing for which to retrieve the inventory maximum.
- `bin`: Inventory bin id to query.

Returns:
- Configured maximum value of inventory bin `bin` on the player `thing`.

Notes:
- Requires a player thing with player data.


#### GetPhysicsFlags

```C++
GetPhysicsFlags(Thing thing) -> int
```

Returns the current physics flags of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current physics flags.

Returns:
- Current physics flags of the `thing`. See [Physics Flags](Flags.md#physics-flags).

Notes:
- Requires a physics thing.


#### SetPhysicsFlags

```C++
SetPhysicsFlags(Thing thing, int physflags)
```

Sets the physics flags of the `thing`.

Parameters:
- `thing`: Thing whose physics flags to set.
- `physflags`: Physics-flag bitmask to apply. See [Physics Flags](Flags.md#physics-flags).

Notes:
- Requires a physics thing.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ClearPhysicsFlags

```C++
ClearPhysicsFlags(Thing thing, int physflags)
```

Clears the specified physics flags on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified physics flags.
- `physflags`: Physics-flag bitmask to clear. See [Physics Flags](Flags.md#physics-flags).

Notes:
- Requires a physics thing.


#### GetThingClassCog

```C++
GetThingClassCog(Thing thing) -> Cog
```

Returns the current class cog of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current class cog.

Returns:
- Current class cog of the `thing`.


#### SetThingClassCog

```C++
SetThingClassCog(Thing thing, Cog thingCog)
```

Sets the class cog of `thing`.

Parameters:
- `thing`: Thing whose class cog should be updated.
- `thingCog`: Cog to assign as the thing class cog.

#### GetThingCaptureCog

```C++
GetThingCaptureCog(Thing thing) -> Cog
```

Returns the current capture cog of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current capture cog.

Returns:
- Current capture cog of the `thing`.


#### SetThingCaptureCog

```C++
SetThingCaptureCog(Thing thing, Cog captureCog)
```

Sets the capture cog of `thing` directly.

Parameters:
- `thing`: Thing whose capture cog should be updated.
- `captureCog`: Cog to assign as the capture cog.

#### SetThingAttachFlags

```C++
SetThingAttachFlags(Thing thing, int attflags)
```

Sets the attach flags of the `thing`.

Parameters:
- `thing`: Thing whose attach flags to set.
- `attflags`: Attachment-flag bitmask to apply. See [Attach Flags](Flags.md#attach-flags).

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ClearThingAttachFlags

```C++
ClearThingAttachFlags(Thing thing, int attflags)
```

Clears the specified attach flags on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified attach flags.
- `attflags`: Attachment-flag bitmask to clear. See [Attach Flags](Flags.md#attach-flags).

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetParticleSize

```C++
GetParticleSize(Thing thing) -> float
```

Returns the particle size of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current particle size.

Returns:
- Particle size of the `thing`.

Notes:
- Requires a particle thing.


#### SetParticleSize

```C++
SetParticleSize(Thing thing, float size)
```

Sets the particle size of the `thing`.

Parameters:
- `thing`: Thing whose particle size to set.
- `size`: New particle size stored in `thing->thingInfo.particleInfo.size`.

Notes:
- Requires a particle thing.


#### GetParticleGrowthSpeed

```C++
GetParticleGrowthSpeed(Thing thing) -> float
```

Returns the particle growth speed of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current particle growth speed.

Returns:
- Particle growth speed of the `thing`.

Notes:
- Requires a particle thing.


#### SetParticleGrowthSpeed

```C++
SetParticleGrowthSpeed(Thing thing, float speed)
```

Sets the particle growth speed of the `thing`.

Parameters:
- `thing`: Thing whose particle growth speed to set.
- `speed`: New particle growth speed stored in `thing->thingInfo.particleInfo.growthSpeed`.

Notes:
- Requires a particle thing.


#### GetParticleTimeoutRate

```C++
GetParticleTimeoutRate(Thing thing) -> float
```

Returns the particle timeout rate of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current particle timeout rate.

Returns:
- Particle timeout rate of the `thing`.

Notes:
- Requires a particle thing.


#### SetParticleTimeoutRate

```C++
SetParticleTimeoutRate(Thing thing, float timeoutRate)
```

Sets the particle timeout rate of the `thing`.

Parameters:
- `thing`: Thing whose particle timeout rate to set.
- `timeoutRate`: New particle timeout-rate value stored in `thing->thingInfo.particleInfo.timeoutRate`.

Notes:
- Requires a particle thing.


#### GetTypeFlags

```C++
GetTypeFlags(Thing thing) -> int
```

Returns the current type flags of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current type flags.

Returns:
- Current type-specific flag bitfield stored in `thing->thingInfo.actorInfo.flags`. See [Shared Type-Specific Flag Field](Flags.md#shared-type-specific-flag-field).

Notes:
- Valid only for actor, weapon, item, explosion, player, and particle things.


#### SetTypeFlags

```C++
SetTypeFlags(Thing thing, int typeflags)
```

Sets the type flags of the `thing`.

Parameters:
- `thing`: Thing whose type flags to set.
- `typeflags`: Bitmask to OR into the type-specific flag field stored in `thing->thingInfo.actorInfo.flags`. See [Shared Type-Specific Flag Field](Flags.md#shared-type-specific-flag-field).

Notes:
- Valid only for actor, weapon, item, explosion, player, and particle things.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ClearTypeFlags

```C++
ClearTypeFlags(Thing thing, int typeflags)
```

Clears the specified type flags on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified type flags.
- `typeflags`: Bitmask to clear from the type-specific flag field stored in `thing->thingInfo.actorInfo.flags`. See [Shared Type-Specific Flag Field](Flags.md#shared-type-specific-flag-field).

Notes:
- Valid only for actor, weapon, item, explosion, player, and particle things.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetActorFlags

```C++
GetActorFlags(Thing thing) -> int
```

Returns the current flags of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current flags.

Returns:
- Current actor-flag bitfield stored in `thing->thingInfo.actorInfo.flags`. See [Actor Flags](Flags.md#actor-flags).

Notes:
- Shares the same implementation as [`GetTypeFlags()`](#gettypeflags).


#### SetActorFlags

```C++
SetActorFlags(Thing thing, int typeflags)
```

Sets the flags of the `thing`.

Parameters:
- `thing`: Thing whose flags to set.
- `typeflags`: Actor-flag bitmask to OR into the shared type-specific flag field. See [Actor Flags](Flags.md#actor-flags).

Notes:
- Shares the same implementation as [`SetTypeFlags()`](#settypeflags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ClearActorFlags

```C++
ClearActorFlags(Thing thing, int typeflags)
```

Clears the specified flags on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified flags.
- `typeflags`: Actor-flag bitmask to clear from the shared type-specific flag field. See [Actor Flags](Flags.md#actor-flags).

Notes:
- Shares the same implementation as [`ClearTypeFlags()`](#cleartypeflags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetWeaponFlags

```C++
GetWeaponFlags(Thing thing) -> int
```

Returns the current flags of the `weapon`.

Parameters:
- `thing`: Weapon thing whose type-specific flag bitfield should be read.

Returns:
- Current weapon-flag bitfield of the `weapon`. See [Weapon Flags](Flags.md#weapon-flags).

Notes:
- Shares the same implementation as [`GetTypeFlags()`](#gettypeflags).


#### SetWeaponFlags

```C++
SetWeaponFlags(Thing thing, int typeflags)
```

Sets the flags of the `weapon`.

Parameters:
- `thing`: Weapon thing whose type-specific flag bitfield should be updated.
- `typeflags`: Weapon-flag bitmask to OR into the shared type-specific flag field. See [Weapon Flags](Flags.md#weapon-flags).

Notes:
- Shares the same implementation as [`SetTypeFlags()`](#settypeflags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ClearWeaponFlags

```C++
ClearWeaponFlags(Thing thing, int typeflags)
```

Clears the specified flags on the `weapon`.

Parameters:
- `thing`: Weapon thing whose type-specific flag bitfield should be updated.
- `typeflags`: Weapon-flag bitmask to clear from the shared type-specific flag field. See [Weapon Flags](Flags.md#weapon-flags).

Notes:
- Shares the same implementation as [`ClearTypeFlags()`](#cleartypeflags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetExplosionFlags

```C++
GetExplosionFlags(Thing thing) -> int
```

Returns the current explosion flags of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current explosion flags.

Returns:
- Current explosion-flag bitfield of the `thing`. See [Explosion Flags](Flags.md#explosion-flags).

Notes:
- Shares the same implementation as [`GetTypeFlags()`](#gettypeflags).


#### SetExplosionFlags

```C++
SetExplosionFlags(Thing thing, int typeflags)
```

Sets the explosion flags of the `thing`.

Parameters:
- `thing`: Thing whose explosion flags to set.
- `typeflags`: Explosion-flag bitmask to OR into the shared type-specific flag field. See [Explosion Flags](Flags.md#explosion-flags).

Notes:
- Shares the same implementation as [`SetTypeFlags()`](#settypeflags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ClearExplosionFlags

```C++
ClearExplosionFlags(Thing thing, int typeflags)
```

Clears the specified explosion flags on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified explosion flags.
- `typeflags`: Explosion-flag bitmask to clear from the shared type-specific flag field. See [Explosion Flags](Flags.md#explosion-flags).

Notes:
- Shares the same implementation as [`ClearTypeFlags()`](#cleartypeflags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetItemFlags

```C++
GetItemFlags(Thing thing) -> int
```

Returns the current item flags of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current item flags.

Returns:
- Current item-flag bitfield of the `thing`. See [Item Flags](Flags.md#item-flags).

Notes:
- Shares the same implementation as [`GetTypeFlags()`](#gettypeflags).


#### SetItemFlags

```C++
SetItemFlags(Thing thing, int typeflags)
```

Sets the item flags of the `thing`.

Parameters:
- `thing`: Thing whose item flags to set.
- `typeflags`: Item-flag bitmask to OR into the shared type-specific flag field. See [Item Flags](Flags.md#item-flags).

Notes:
- Shares the same implementation as [`SetTypeFlags()`](#settypeflags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ClearItemFlags

```C++
ClearItemFlags(Thing thing, int typeflags)
```

Clears the specified item flags on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified item flags.
- `typeflags`: Item-flag bitmask to clear from the shared type-specific flag field. See [Item Flags](Flags.md#item-flags).

Notes:
- Shares the same implementation as [`ClearTypeFlags()`](#cleartypeflags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetParticleFlags

```C++
GetParticleFlags(Thing thing) -> int
```

Returns the current particle flags of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current particle flags.

Returns:
- Current particle-flag bitfield of the `thing`. See [Particle Flags](Flags.md#particle-flags).

Notes:
- Shares the same implementation as [`GetTypeFlags()`](#gettypeflags).


#### SetParticleFlags

```C++
SetParticleFlags(Thing thing, int typeflags)
```

Sets the particle flags of the `thing`.

Parameters:
- `thing`: Thing whose particle flags to set.
- `typeflags`: Particle-flag bitmask to OR into the shared type-specific flag field. See [Particle Flags](Flags.md#particle-flags).

Notes:
- Shares the same implementation as [`SetTypeFlags()`](#settypeflags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### ClearParticleFlags

```C++
ClearParticleFlags(Thing thing, int typeflags)
```

Clears the specified particle flags on the `thing`.

Parameters:
- `thing`: Thing on which to clear the specified particle flags.
- `typeflags`: Particle-flag bitmask to clear from the shared type-specific flag field. See [Particle Flags](Flags.md#particle-flags).

Notes:
- Shares the same implementation as [`ClearTypeFlags()`](#cleartypeflags).
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetThingFireOffset

```C++
GetThingFireOffset(Thing thing) -> Vector
```

Returns the current local fire/projectile spawn offset of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current fire offset.

Returns:
- Current local fire/projectile spawn offset of the `thing`.


#### SetThingFireOffset

```C++
SetThingFireOffset(Thing thing, Vector offset)
```

Sets the local fire/projectile spawn offset of the `thing`.

Parameters:
- `thing`: Thing whose fire offset to set.
- `offset`: New local fire/projectile spawn offset stored in `thing->thingInfo.actorInfo.fireOffset`.

### Keyframes, Puppets, Frames, And Path Movement


#### PlayKey

```C++
PlayKey(Thing thing, Keyframe kfTrack, int lowPriority, int kfflags, int bWait) -> int
```

Starts playing keyframe `kfTrack` on `thing`'s puppet.

Parameters:
- `thing`: Thing on which to play the keyframe.
- `kfTrack`: Keyframe track to play.
- `lowPriority`: Low joint-priority value passed to the puppet system. The wrapper uses `lowPriority + 2` as the high priority.
- `kfflags`: Keyframe playback flags. See [Keyframe Flags](Flags.md#keyframe-flags).
- `bWait`: Non-zero to block the current COG until the started track stops.

Returns:
- Track number used for the new keyframe playback, or `-1` on failure.

Notes:
- Requires a thing with a puppet.
- If `thing` is path-moving, the wrapper finishes any active path move before playing the key.
- When sync is enabled and the current message is not `startup` or `shutdown`, the key playback is broadcast to joined players.

#### PlayKeyEx

```C++
PlayKeyEx(Thing thing, Keyframe kfTrack, int lowPriority, int highPriority, int kfflags, int bWait) -> int
```

Starts playing keyframe `kfTrack` on `thing`'s puppet using explicit low and high priorities.

Parameters:
- `thing`: Thing on which to play the keyframe.
- `kfTrack`: Keyframe track to play.
- `lowPriority`: Low joint-priority value.
- `highPriority`: High joint-priority value.
- `kfflags`: Keyframe playback flags. See [Keyframe Flags](Flags.md#keyframe-flags).
- `bWait`: Non-zero to block the current COG until the started track stops.

Returns:
- Track number used for the new keyframe playback, or `-1` on failure.

Notes:
- Requires a thing with a puppet.
- If `thing` is path-moving, the wrapper finishes any active path move before playing the key.

#### StopKey

```C++
StopKey(Thing thing, int trackNum, float fadeTime)
```

Stops puppet track `trackNum` on `thing`.

Parameters:
- `thing`: Thing whose puppet track should be stopped.
- `trackNum`: Puppet track number to stop.
- `fadeTime`: Fade-out time in seconds. Values below `0` are rejected.

Notes:
- Requires a thing with a puppet.
- When sync is enabled and the current message is not `startup` or `shutdown`, the stop request is broadcast to joined players.

#### PauseKey

```C++
PauseKey(Thing thing, int trackNum) -> int
```

Pauses puppet track `trackNum` on `thing`.

Parameters:
- `thing`: Thing whose puppet track should be paused.
- `trackNum`: Puppet track number to pause.

Returns:
- `1` on success, or `-1` on invalid input.

Notes:
- Requires a thing with a puppet class and a valid existing track.

#### ResumeKey

```C++
ResumeKey(Thing thing, int trackNum) -> int
```

Resumes puppet track `trackNum` on `thing`.

Parameters:
- `thing`: Thing whose puppet track should be resumed.
- `trackNum`: Puppet track number to resume.

Returns:
- `1` on success, or `-1` on invalid input.

Notes:
- Requires a thing with a puppet class and a valid existing track.

#### SetThingModel

```C++
SetThingModel(Thing thing, Model model) -> int
```

Changes `thing`'s model and returns the previous model index.

Parameters:
- `thing`: Thing whose model should be changed.
- `model`: New model to assign.

Returns:
- Previous model index, or `-1` on failure.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the new model state is broadcast to joined players.

#### GetThingModel

```C++
GetThingModel(Thing thing) -> int
```

Returns the current model of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current model.

Returns:
- Current model of the `thing`.


#### PlayMode

```C++
PlayMode(Thing thing, int mode, int bWait) -> int
```

Starts playing the requested puppet `mode` on the `thing`.

Parameters:
- `thing`: Thing on which to play the puppet mode.
- `mode`: Puppet submode to play within the thing's current major mode. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).
- `bWait`: Non-zero to block the current COG until the started mode track stops.

Returns:
- Track number used for the new mode playback, or `-1` on failure.

Notes:
- If `bWait` is non-zero, the current COG blocks until the started track stops.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### StopMode

```C++
StopMode(Thing thing, int mode, float fadeTime)
```

Stops the current mode on the `thing`.

Parameters:
- `thing`: Thing whose current mode should be stopped.
- `mode`: Puppet submode to stop. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).
- `fadeTime`: Fade-out time in seconds passed to the puppet stop helper.


#### SynchMode

```C++
SynchMode(Thing thing, int oldMode, int newMode, float unk, int bReverse)
```

Synchronizes puppet submode playback from `oldMode` to `newMode` on `thing`.

Parameters:
- `thing`: Thing whose puppet mode should be synchronized.
- `oldMode`: Current puppet submode to sync from. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).
- `newMode`: Target puppet submode to sync to. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).
- `unk`: Extra floating-point parameter passed through to the puppet sync helper.
- `bReverse`: Non-zero to request reverse synchronization behavior.

#### IsModePlaying

```C++
IsModePlaying(Thing thing, int mode) -> int
```

Checks whether the requested puppet `mode` is currently active on the `thing`.

Parameters:
- `thing`: Thing for which to test the requested mode playing state.
- `mode`: Puppet submode to inspect within the thing's current major mode. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).

Returns:
- `0` when the mode is not playing, `1` when playing, `2` when paused, and `-1` on invalid input.

Notes:
- Requires a thing with puppet data.
- Requires a thing with a puppet class.


#### PauseMode

```C++
PauseMode(Thing thing, int mode) -> int
```

Pauses the currently active puppet `mode` on the `thing`.

Parameters:
- `thing`: Thing whose mode should be paused.
- `mode`: Puppet submode to pause within the thing's current major mode. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).

Returns:
- `0` when the mode is not active, `1` when it was paused, `2` when it was already paused, and `-1` on invalid input.

Notes:
- Requires a thing with puppet data.
- Requires a thing with a puppet class.


#### ResumeMode

```C++
ResumeMode(Thing thing, int mode) -> int
```

Resumes a paused puppet `mode` on the `thing`.

Parameters:
- `thing`: Thing whose mode should be resumed.
- `mode`: Puppet submode to resume within the thing's current major mode. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).

Returns:
- `0` when the mode is not active, `1` when it was resumed, `2` when it was not paused, and `-1` on invalid input.

Notes:
- Requires a thing with puppet data.
- Requires a thing with a puppet class.


#### TrackToMode

```C++
TrackToMode(Thing thing, int trackNum) -> int
```

Resolves puppet track `trackNum` back to the current submode on `thing`.

Parameters:
- `thing`: Thing whose puppet track should be inspected.
- `trackNum`: Puppet track number to translate.

Returns:
- Puppet submode associated with that track, or `-1` on failure. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).

Notes:
- Requires a thing with a puppet class and an active puppet.

#### WaitMode

```C++
WaitMode(Thing thing, int mode) -> int
```

Blocks the current COG until the active track for `mode` on the `thing` stops.

Parameters:
- `thing`: Thing whose active puppet mode track should be waited on.
- `mode`: Puppet submode whose active track should be waited on. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).

Returns:
- `1` when waiting started, `0` when the mode is not active, and `-1` on invalid input.

Notes:
- When the mode is active, the current COG blocks until that track stops.


#### GetMajorMode

```C++
GetMajorMode(Thing thing) -> int
```

Returns the current major mode of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current major mode.

Returns:
- Current puppet major-mode value of the `thing`. See [Puppet Major Modes](Types-And-Modes.md#puppet-major-modes).

Notes:
- Requires a thing with puppet data.
- Requires a thing with a puppet class.


#### FirstThingInSector

```C++
FirstThingInSector(Sector sector) -> Thing
```

Returns the first thing linked in `sector`'s thing list.

Parameters:
- `sector`: Sector whose linked thing list should be inspected.

Returns:
- First thing in the sector list, or `-1` when the sector is invalid or empty.

#### NextThingInSector

```C++
NextThingInSector(Thing thing) -> Thing
```

Returns the next thing after `thing` in its sector linked list.

Parameters:
- `thing`: Thing whose `pNextThingInSector` link should be followed.

Returns:
- Next thing in the sector list, or `-1` when there is no next thing.

#### PrevThingInSector

```C++
PrevThingInSector(Thing thing) -> Thing
```

Returns the previous thing before `thing` in its sector linked list.

Parameters:
- `thing`: Thing whose `pPrevThingInSector` link should be followed.

Returns:
- Previous thing in the sector list, or `-1` when there is no previous thing.

#### MoveToFrame

```C++
MoveToFrame(Thing thing, int frame, float speed)
```

Starts path movement toward frame `frame`.

Parameters:
- `thing`: Path-moving thing that should start moving toward a path frame.
- `frame`: Target path-frame index.
- `speed`: Path-movement speed parameter. This value is multiplied by `0.1` before it is used by the path-move system. A passed value of `0` falls back to `0.5` after that conversion.

Notes:
- Requires a path-move thing.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### SkipToFrame

```C++
SkipToFrame(Thing thing, int frame, float speed)
```

Starts path movement toward frame `frame` at the supplied `speed`.

Parameters:
- `thing`: Path-moving thing that should advance toward the requested frame.
- `frame`: Target path frame index.
- `speed`: Path-movement speed parameter. This value is multiplied by `0.1` before it is used by the path-move system. A passed value of `0` falls back to `0.5` after that conversion.

Notes:
- Requires a path-move thing.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### JumpToFrame

```C++
JumpToFrame(Thing thing, int frame, Sector newSector)
```

Immediately moves `thing` to path frame `frame` and places it in `newSector`.

Parameters:
- `thing`: Path-moving thing to reposition.
- `frame`: Path frame index to jump to.
- `newSector`: Sector that should contain the thing after the jump.

Notes:
- Requires a path-move thing.


#### PathMovePause

```C++
PathMovePause(Thing thing) -> int
```

Pauses path movement on `thing`.

Parameters:
- `thing`: Path-moving thing to pause.

Returns:
- `thing` when the pause succeeded, otherwise `-1`.

Notes:
- Requires a path-move thing.

#### PathMoveResume

```C++
PathMoveResume(Thing thing) -> int
```

Resumes paused path movement on `thing`.

Parameters:
- `thing`: Path-moving thing to resume.

Returns:
- `thing` when the resume succeeded, otherwise `-1`.

Notes:
- Requires a path-move thing.

#### Rotate

```C++
Rotate(Thing thing, float degrees, int axis, float rotTime)
```

Rotates a path-moving thing around its current insertion pivot.

Parameters:
- `thing`: Path-moving thing to rotate.
- `degrees`: Rotation amount in degrees.
- `axis`: Rotation axis selector: `0` = pitch, `1` = yaw, `2` = roll.
- `rotTime`: Non-zero rotation duration. The wrapper rejects `0`.

Notes:
- Requires a path-move thing.


#### RotatePivot

```C++
RotatePivot(Thing thing, int frame, float rotTime)
```

Rotates a path-moving thing around the pivot stored in one of its path frames.

Parameters:
- `thing`: Path-moving thing to rotate around one of its path-frame pivots.
- `frame`: Path-frame index whose stored position is used as the pivot.
- `rotTime`: Rotation duration. `0` is coerced to `1.0`. Positive values apply the frame's stored PYR rotation; negative values apply the inverse rotation over `abs(rotTime)`.

Notes:
- Requires a path-move thing.


#### RotateToPYR

```C++
RotateToPYR(Thing thing, Vector pyr, float time)
```

Rotates a path-moving thing to a target pitch-yaw-roll orientation over time.

Parameters:
- `thing`: Path-moving thing to rotate.
- `pyr`: Target pitch-yaw-roll orientation in degrees.
- `time`: Rotation duration in seconds. Must be greater than `0`.

Notes:
- Requires a path-move thing.


#### GetThingTemplate

```C++
GetThingTemplate(Thing thing) -> Template
```

Returns the current template of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current template.

Returns:
- Current template of the `thing`.


#### ParseArg

```C++
ParseArg(Thing thing, string args)
```

Parses a config-style argument string and applies each parsed `name=value` entry to `thing` through the standard thing-argument parser.

Parameters:
- `thing`: Thing whose parsed arguments should be updated.
- `args`: Space-separated thing arguments in the same syntax used by thing/template configuration data.


#### SetPuppetModeFPS

```C++
SetPuppetModeFPS(Thing thing, int majorMode, int submode, float fps) -> float
```

Changes the keyframe playback FPS for one major-mode/submode pair in `thing`'s puppet class.

Parameters:
- `thing`: Thing whose puppet-class mode track should be modified.
- `majorMode`: Puppet major-mode value to modify. See [Puppet Major Modes](Types-And-Modes.md#puppet-major-modes).
- `submode`: Puppet submode index within `majorMode`. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).
- `fps`: New keyframe playback FPS to store on that mode's keyframe.

Returns:
- Previous FPS value for the selected puppet mode track.

Notes:
- Requires a thing with a puppet class.


#### StartQuetzAnim

```C++
StartQuetzAnim(Thing thing, int mode) -> int
```

Starts one of the hard-coded Quetz animation helpers selected by `mode`.

Parameters:
- `thing`: Thing whose Quetz animation helper should be triggered.
- `mode`: Quetz animation mode selector.

Returns:
- The current implementation only pushes `-1` on invalid input; successful paths do not return a documented value.


#### FadeInTrack

```C++
FadeInTrack(Thing thing, int track, float speed) -> int
```

Fades in puppet track `track` on `thing` using the supplied `speed`.

Parameters:
- `thing`: Thing whose puppet track should be faded in.
- `track`: Puppet track index to fade in.
- `speed`: Fade-in speed.

Returns:
- Non-zero on success, otherwise `0`.


#### PlayForceMoveMode

```C++
PlayForceMoveMode(Thing thing, int submode) -> int
```

Starts a forced-move puppet submode on `thing`.

Parameters:
- `thing`: Thing on which to start the forced-move puppet submode.
- `submode`: Puppet submode to play. See [Puppet Submodes](Types-And-Modes.md#puppet-submodes).

Returns:
- Runtime id or track number returned by the playback call.

### Mesh, Camera, And FX Helpers


#### SyncThingPos

```C++
SyncThingPos(Thing thing)
```

Synchronizes the position of `thing` through the engine's thing-sync path.

Parameters:
- `thing`: Thing whose position should be synchronized.


#### SyncThingAttachment

```C++
SyncThingAttachment(Thing thing)
```

Synchronizes the attachment state of `thing` to joined players.

Parameters:
- `thing`: Thing whose attachment state should be synchronized.


#### SyncThingState

```C++
SyncThingState(Thing thing)
```

Synchronizes the general runtime state of `thing` through the engine's thing-sync path.

Parameters:
- `thing`: Thing whose state should be synchronized.


#### GetMeshByName

```C++
GetMeshByName(Thing thing, string meshName) -> int
```

Looks up a mesh index by name in `thing`'s current model.

Parameters:
- `thing`: Thing whose current `Model3` meshes should be searched.
- `meshName`: Mesh name to search for.

Returns:
- Mesh index in the thing's current model, or `-1` when no such mesh exists.

Notes:
- The current mesh-name lookup is case-sensitive.


#### GetNodeByName

```C++
GetNodeByName(Thing thing, string nodeName) -> int
```

Looks up a hierarchy node index by name in `thing`'s current model.

Parameters:
- `thing`: Thing whose current `Model3` hierarchy nodes should be searched.
- `nodeName`: Hierarchy node name to search for.

Returns:
- Hierarchy node index in the thing's current model, or `-1` when no such node exists.

Notes:
- The current node-name lookup is case-insensitive.


#### AttachThingToThingMesh

```C++
AttachThingToThingMesh(Thing attachThing, Template template, int meshNum) -> Thing
```

Creates a new thing from `template` and attaches it to mesh `meshNum` on `attachThing`.

Parameters:
- `attachThing`: Thing whose mesh should receive the attached child thing.
- `template`: Template used to create the attached child thing.
- `meshNum`: Mesh index used by the model.

Returns:
- Created attached thing, or `-1` on failure.

Notes:
- Requires `attachThing` to have both a puppet and a `Model3`.
- When the created child is an explosion thing, the wrapper also sets its parent to `attachThing`.


#### DetachThingMesh

```C++
DetachThingMesh(Thing thing)
```

Destroys `thing`, which is typically an auxiliary thing that had been attached to a mesh.

Parameters:
- `thing`: Attached helper thing to destroy.


#### SetThingMesh

```C++
SetThingMesh(Thing thing, int meshNum, Model model, int meshNumSrc) -> int
```

Installs or updates a mesh-swap entry on `thing`.

Parameters:
- `thing`: Thing whose mesh to set.
- `meshNum`: Destination mesh index on `thing` that should be replaced.
- `model`: Source model that provides the replacement mesh.
- `meshNumSrc`: Mesh index inside `model` that should be used as the replacement source.

Returns:
- Swap-entry reference number, or `-1` on failure.

Notes:
- Reusing the same destination mesh updates the existing swap entry instead of creating a second one.
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### RestoreThingMesh

```C++
RestoreThingMesh(Thing thing, int refnum)
```

Removes a previously installed mesh-swap entry from `thing`, or clears all mesh swaps when `refnum` is `-99`.

Parameters:
- `thing`: Thing whose mesh-swap list should be restored.
- `refnum`: Swap-entry reference number returned by [`SetThingMesh()`](#setthingmesh). Use `-99` to clear the entire swap list.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the change is broadcast to joined players.


#### GetThingAlpha

```C++
GetThingAlpha(Thing thing) -> float
```

Returns the current alpha value of the `thing`.

Parameters:
- `thing`: Thing for which to retrieve the current alpha value.

Returns:
- Current alpha value of the `thing`.


#### SetThingAlpha

```C++
SetThingAlpha(Thing thing, float alpha) -> float
```

Sets the alpha value of the `thing`.

Parameters:
- `thing`: Thing whose alpha value to set.
- `alpha`: Alpha value to apply.

Returns:
- Floating-point value returned by the setter.


#### GetCameraFOV

```C++
GetCameraFOV() -> float
```

Returns the current field of view of the `camera`.

Returns:
- Current field of view of the `camera`.


#### SetCameraFOV

```C++
SetCameraFOV(float fov, int bInterp, float timeDelta)
```

Sets the field of view of the `camera`.

Parameters:
- `fov`: New camera field of view.
- `bInterp`: Non-zero to interpolate the change instead of applying it immediately.
- `timeDelta`: Interpolation time in seconds when `bInterp` is non-zero.


#### ResetCameraFOV

```C++
ResetCameraFOV(int bInterp, float time)
```

Resets the current camera field of view back to its default value.

Parameters:
- `bInterp`: Non-zero to interpolate the change instead of applying it immediately.
- `time`: Interpolation time in seconds when `bInterp` is non-zero.


#### SetCameraLookInterp

```C++
SetCameraLookInterp(int camNum, int bEnable)
```

Sets whether camera `camNum` uses look interpolation.

Parameters:
- `camNum`: Camera slot whose look interpolation setting should be changed.
- `bEnable`: Non-zero to enable look interpolation, `0` to disable it.


#### SetCameraPosInterp

```C++
SetCameraPosInterp(int camNum, int bDollyMode)
```

Sets whether camera `camNum` uses position interpolation (dolly mode).

Parameters:
- `camNum`: Camera slot whose position interpolation setting should be changed.
- `bDollyMode`: Non-zero to enable position interpolation (dolly mode), `0` to disable it.


#### SetCameraInterpSpeed

```C++
SetCameraInterpSpeed(int camNum, float speed)
```

Sets the interpolation speed for camera `camNum`.

Parameters:
- `camNum`: Camera slot whose interpolation speed should be changed.
- `speed`: Interpolation speed to apply.


#### SetCameraPosition

```C++
SetCameraPosition(int camNum, Vector vec)
```

Sets the position of the `camera`.

Parameters:
- `camNum`: Camera slot to reposition.
- `vec`: New world-space camera position.


#### AnimateSpriteSize

```C++
AnimateSpriteSize(Thing sprite, Vector vecStart, Vector vecEnd, float deltaTime) -> int
```

Animates a sprite thing from `(width, height, alpha) = vecStart` to `(width, height, alpha) = vecEnd`.

Parameters:
- `sprite`: Sprite thing whose size and alpha should be animated.
- `vecStart`: Starting `(width, height, alpha)` values.
- `vecEnd`: Ending `(width, height, alpha)` values.
- `deltaTime`: Animation duration in seconds. Values less than or equal to `0` apply `vecEnd` immediately.

Returns:
- Animation id when a timed sprite-size animation starts, or `-1` when the change is applied immediately or the animation cannot be created.


#### GetCameraPosition

```C++
GetCameraPosition(int camNum) -> Vector
```

Returns the current position of the `camera`.

Parameters:
- `camNum`: Camera slot to query.

Returns:
- Current world-space position of the camera slot. Invalid camera numbers return the zero vector.


#### SetCameraFadeThing

```C++
SetCameraFadeThing(int camNum, Thing fadeThing, Thing camPosThing, int bWhiteFade)
```

Configures a camera slot to use two helper things for a fade-plate camera setup.

Parameters:
- `camNum`: Camera slot to configure.
- `fadeThing`: Secondary-focus helper thing used as the fade plate. The wrapper aligns it to the current external-camera orientation and places it slightly in front of the external camera look position.
- `camPosThing`: Primary-focus helper thing placed at the current external camera look position and sector, then used as the camera's primary focus anchor.
- `bWhiteFade`: Fade-plate side selector. When this value is `0`, the wrapper rotates `fadeThing` by `180` degrees before focusing the camera; non-zero keeps the default orientation.

Notes:
- The wrapper copies position, FOV, and orientation from the external camera, then disables position/look interpolation on the target camera slot.
- Internally, the code comment describes the rotated side as the fade plate's backside, with the front side black and the back side white.


#### SetExtCamOffset

```C++
SetExtCamOffset(Vector vec)
```

Sets the external-camera position offset vector.

Parameters:
- `vec`: External-camera position offset to apply.


#### SetExtCamOffsetToThing

```C++
SetExtCamOffsetToThing(Thing thing)
```

Sets the external-camera position offset so it points from the local player toward `thing`.

Parameters:
- `thing`: Thing whose relative position should be converted into the external-camera position offset.


#### SetExtCamLookOffsetToThing

```C++
SetExtCamLookOffsetToThing(Thing thing)
```

Sets the external-camera look offset so it points from the local player toward `thing`.

Parameters:
- `thing`: Thing whose relative position should be converted into the external-camera look offset.


#### SetExtCamLookOffset

```C++
SetExtCamLookOffset(Vector vec)
```

Sets the external-camera look offset vector.

Parameters:
- `vec`: External-camera look offset to apply.


#### RestoreExtCam

```C++
RestoreExtCam()
```

Restores the default external-camera settings.


#### IsThingAutoAiming

```C++
IsThingAutoAiming()
```

This verb is currently a stub in the engine and does not report an auto-aiming state.


#### CreateLaser

```C++
CreateLaser(Thing srcThing, Vector offset, Vector endPos, float baseRadius, float tipRadius, float duration) -> Thing
```

Creates a laser effect thing starting from `srcThing`.

Parameters:
- `srcThing`: Source thing from which the laser originates.
- `offset`: Start offset from the source thing.
- `endPos`: World-space end position of the laser.
- `baseRadius`: Base radius used by the effect.
- `tipRadius`: Tip radius used by the effect.
- `duration`: Duration in seconds.

Returns:
- Created laser thing reference.


#### CreateLightning

```C++
CreateLightning(Thing sourceThing, Vector offset, Vector endPos, float baseRadius, float tipRadius, float duration) -> Thing
```

Creates a lightning effect thing starting from `sourceThing`.

Parameters:
- `sourceThing`: Source thing from which the lightning originates.
- `offset`: Start offset from the source thing.
- `endPos`: World-space end position of the lightning.
- `baseRadius`: Base radius used by the effect.
- `tipRadius`: Tip radius used by the effect.
- `duration`: Duration in seconds.

Returns:
- Created lightning thing reference.


#### MakeFairyDust

```C++
MakeFairyDust(Thing thing, Vector pos)
```

Creates the standard fairy-dust effect at `pos` in the current sector of `thing`.

Parameters:
- `thing`: Thing whose current sector should receive the effect.
- `pos`: World-space position at which to spawn the fairy-dust effect.


#### MakeFairyDustDeluxe

```C++
MakeFairyDustDeluxe(Thing thing, Vector pos)
```

Creates or clears the deluxe fairy-dust effect for `thing`.

Parameters:
- `thing`: Thing whose deluxe fairy-dust effect should be updated.
- `pos`: World-space position for the effect. A zero vector clears the deluxe effect instead.


#### CreatePolylineThing

```C++
CreatePolylineThing(Thing srcThing, Thing destThing, Vector endPos, Material material, float baseRadius, float tipRadius, float duration) -> Thing
```

Creates a temporary polyline effect thing between `srcThing` and either `destThing` or `endPos`.

Parameters:
- `srcThing`: Source thing from which the polyline starts.
- `destThing`: Optional destination thing. When omitted, `endPos` is used instead.
- `endPos`: Fallback end position used when `destThing` is omitted.
- `material`: Material used to render the polyline.
- `baseRadius`: Radius at the start of the polyline.
- `tipRadius`: Radius at the end of the polyline.
- `duration`: Lifetime of the polyline effect in seconds.

Returns:
- Created polyline thing reference.

Notes:
- The destination thing parameter is optional in the current implementation.


#### MakeCamera2LikeCamera1

```C++
MakeCamera2LikeCamera1(Thing cam1Pos, Thing cam1Look) -> int
```

Copies the current camera spot and look spot into the positions of `cam1Pos` and `cam1Look`.

Parameters:
- `cam1Pos`: Thing that should receive the current camera position.
- `cam1Look`: Thing that should receive the current camera look-at position.

Returns:
- `1` on success, otherwise `0`.
