# Sound System

This page documents the complete audio stack from world/gameplay calls down to the low-level channel allocator and 3D update loop.

Jones3D audio is layered in three tiers:

1. `sound/` low-level banks, handles, channels, fades, and backend I/O
2. `sithSoundMixer` listener-aware world playback and ambient management
3. `sithSound`, `sithSoundClass`, and `sithVoice` gameplay-facing asset and semantic playback

Primary source files:

- [`Libs/sith/Devices/sithSound.c`](../../Libs/sith/Devices/sithSound.c)
- [`Libs/sith/Devices/sithSoundMixer.c`](../../Libs/sith/Devices/sithSoundMixer.c)
- [`Libs/sith/World/sithSoundClass.c`](../../Libs/sith/World/sithSoundClass.c)
- [`Libs/sith/World/sithVoice.c`](../../Libs/sith/World/sithVoice.c)
- [`Libs/sound/Sound.c`](../../Libs/sound/Sound.c)
- [`Libs/sound/DriverDX9.c`](../../Libs/sound/DriverDX9.c)
- [`Libs/sound/DriverDX6.c`](../../Libs/sound/DriverDX6.c)

## Startup And Ownership Layers

The top-level gameplay entry point is [`sithSound_Startup()`](../../Libs/sith/Devices/sithSound.c#L55).

That function:

1. opens the low-level sound layer through [`Sound_Open()`](../../Libs/sound/Sound.c#L314)
2. installs the listener/thing callback bridge
3. sets 3D globals
4. enables runtime sound loading
5. starts the gameplay-side mixer through [`sithSoundMixer_Startup()`](../../Libs/sith/Devices/sithSoundMixer.c#L68)
6. starts sound-class support through [`sithSoundClass_Startup()`](../../Libs/sith/World/sithSoundClass.c#L207)

Shutdown reverses that through [`sithSound_Shutdown()`](../../Libs/sith/Devices/sithSound.c#L83).

Architecturally, this means the gameplay layer owns:

- what sounds mean
- what listener state is
- what world object a sound belongs to

The low-level layer owns:

- sound banks
- channel handles
- fades
- backend buffers and playback state

## Low-Level Sound Core

The low-level runtime lives in [`Sound.c`](../../Libs/sound/Sound.c#L1).

### State Machine

The core state machine is:

- initialized by [`Sound_Initialize()`](../../Libs/sound/Sound.c#L252)
- moved into startup by [`Sound_Startup()`](../../Libs/sound/Sound.c#L284)
- moved into open/live state by [`Sound_Open()`](../../Libs/sound/Sound.c#L314)

`Sound_Open()` installs:

- the thing-info callback used for thing-attached channels
- the listener-mix callback used for software 3D mixing
- the maximum active channel budget
- flags for 3D sound, compression, lip-sync, and global focus

### Channel Budget

The gameplay layer passes the maximum channel count via `SITHSOUND_MAXCHANNELS`, defined in [`Libs/sith/Devices/sithSound.h`](../../Libs/sith/Devices/sithSound.h#L13).

OpenJones3D keeps the original architecture but expands the budget under `QOL`:

- original style: `32`
- OpenJones3D `QOL` default: `512`

The allocator and update loops are otherwise the same design.

## Banks, Handles, And Asset Loading

### Static And Normal Banks

The core sound layer stores audio in banks and can reset either:

- all banks
- or only the non-static world bank

That behavior is implemented in [`Sound_Reset()`](../../Libs/sound/Sound.c#L1255).

This split mirrors the world system:

- static resources survive across level/world transitions more often
- normal world sounds are tied to the current world instance

### File Loading

The low-level loader is [`Sound_Load()`](../../Libs/sound/Sound.c#L1003).

It is responsible for:

- validating load state
- locating file data
- creating or growing the sound-info arrays
- importing WAV or compressed audio data into the bank cache
- generating lip-sync data lazily when needed later

The gameplay-facing wrapper is [`sithSound_Load()`](../../Libs/sith/Devices/sithSound.c#L218), which prefixes runtime paths with `sound\` and chooses between static and normal bank loading based on world state.

This is why gameplay and format code usually talk to `sithSound`, not directly to `Sound_Load()`.

## Channel Allocation And Playback

### Generic Playback

[`Sound_Play()`](../../Libs/sound/Sound.c#L1537) is the generic channel allocator.

Its algorithm is:

1. reject invalid runtime state or invalid sound handles
2. optionally force 3D-cap flags depending on backend capabilities
3. reject duplicate `PLAYONCE` playback
4. allocate the channel table lazily if needed
5. scan for a free channel while tracking the lowest-priority live channel
6. if no free channel exists, reuse a lower-priority one
7. create and play a backend sound buffer, retrying by evicting lower-priority channels if necessary
8. write runtime state such as volume, pitch, priority, GUID, thing id, and flags

This is a classic fixed-budget voice allocator. The sound system is willing to steal lower-priority channels, but only through one central policy path.

### Positional Playback

The 3D entry points are:

- [`Sound_PlayPos()`](../../Libs/sound/Sound.c#L1706)
- [`Sound_PlayThing()`](../../Libs/sound/Sound.c#L1757)

Both perform an important extra step before channel allocation:

- ask the backend whether the sound is audible enough to play now
- if not, reject one-shot sounds outright
- or mark looping sounds as `STARTFAR`, meaning they exist logically but start in a dormant distant state

That "start far, restart when near again" behavior is a major part of how Jones3D keeps looping world sounds scalable.

### Per-Channel Fades

Volume and pitch fades are handled by:

- [`Sound_FadeVolume()`](../../Libs/sound/Sound.c#L1927)
- [`Sound_FadePitch()`](../../Libs/sound/Sound.c#L2123)

Both use a fixed fade array and are advanced each frame by the internal fade pass at the start of [`Sound_Update()`](../../Libs/sound/Sound.c#L2357).

Again, the engine prefers fixed runtime structures over ad-hoc allocation.

## Per-Frame Audio Update

The master update loop is [`Sound_Update()`](../../Libs/sound/Sound.c#L2357).

Its responsibilities are:

1. advance fades
2. update listener position/orientation
3. iterate every active 3D channel
4. refresh thing-attached channel positions through the gameplay callback
5. recompute spatialization for each channel
6. stop channels that are no longer valid
7. restart channels leaving the dormant `FAR` state
8. release channels whose playback has finished

### Listener Update

The listener transform comes from the gameplay mixer, not from the sound core itself. [`Sound_Update()`](../../Libs/sound/Sound.c#L2357) simply consumes the listener vectors it is given and commits them to the backend.

### Thing-Attached Channel Refresh

For channels flagged as thing-attached, [`Sound_Update()`](../../Libs/sound/Sound.c#L2357) asks the gameplay side for current position, velocity, and environment flags through the callback installed at [`Sound_Open()`](../../Libs/sound/Sound.c#L314).

That is how a low-level channel can follow a world thing without `Sound.c` knowing what a `SithThing` is.

### Distant Loop Restart

If a far looping sound becomes audible again, [`Sound_Update()`](../../Libs/sound/Sound.c#L2357) reconstructs it by replaying either:

- [`Sound_PlayThing()`](../../Libs/sound/Sound.c#L1757)
- or [`Sound_PlayPos()`](../../Libs/sound/Sound.c#L1706)

and then restores the original logical handle.

That design avoids keeping backend buffers alive for sounds that are currently too far away to matter while preserving the higher-level idea that the loop is still active.

## Gameplay-Aware Mixer

The gameplay bridge lives in [`sithSoundMixer.c`](../../Libs/sith/Devices/sithSoundMixer.c#L1).

### Startup And Config

[`sithSoundMixer_Startup()`](../../Libs/sith/Devices/sithSoundMixer.c#L68) reads the software falloff mode from config and resets current ambient-sector state through [`sithSoundMixer_ClearAmbientSector()`](../../Libs/sith/Devices/sithSoundMixer.c#L99).

OpenJones3D preserves the original linear model but extends it with:

- linear
- exponential
- logarithmic

Under `QOL`, logarithmic becomes the default instead of the original linear curve.

### Playback Wrappers

Gameplay code usually enters through:

- [`sithSoundMixer_PlaySound()`](../../Libs/sith/Devices/sithSoundMixer.c#L121)
- [`sithSoundMixer_PlaySoundPos()`](../../Libs/sith/Devices/sithSoundMixer.c#L134)
- [`sithSoundMixer_PlaySoundThing()`](../../Libs/sith/Devices/sithSoundMixer.c#L168)

These wrappers translate gameplay flags into low-level channel flags, assign channel GUIDs, and map thing references into the integer identifiers expected by the low-level sound layer.

### Frame Update And Ambient Sector Crossfade

[`sithSoundMixer_Update()`](../../Libs/sith/Devices/sithSoundMixer.c#L259) runs every frame before the rest of the main simulation.

Its responsibilities are:

- handle the no-camera case cleanly
- detect when the current camera entered a different sector
- fade out the previous sector's ambient sound
- fade in the new sector's ambient sound
- derive forward/up vectors from the current camera
- pass the listener transform to [`Sound_Update()`](../../Libs/sound/Sound.c#L2357)

This is why ambient audio in Jones3D is sector-owned rather than globally zoned. The sound mixer treats sector transitions as first-class listener events.

### Thing Callback And Software Spatial Mix

The bridge callbacks used by the low-level layer are:

- [`sithSoundMixer_GetThingInfo()`](../../Libs/sith/Devices/sithSoundMixer.c#L316)
- [`sithSoundMixer_CalcCameraRelativeSoundMix()`](../../Libs/sith/Devices/sithSoundMixer.c#L363)

[`sithSoundMixer_GetThingInfo()`](../../Libs/sith/Devices/sithSoundMixer.c#L316) resolves the compact integer thing id back into current thing position, velocity, and environment flags.

[`sithSoundMixer_CalcCameraRelativeSoundMix()`](../../Libs/sith/Devices/sithSoundMixer.c#L363) computes:

- behind-listener attenuation
- stereo pan from camera right vector
- underwater path adjustments
- distance falloff

The falloff branch is one of the clearest examples of an OpenJones3D architectural extension: the old linear path still exists, but new exponential and logarithmic models were added without changing the rest of the stack.

## Gameplay Sound Assets And Semantics

### World Sound Loading

[`sithSound_Load()`](../../Libs/sith/Devices/sithSound.c#L218) is the world-facing loader. It maps a world-relative sound name to the low-level bank system and records the resulting handle in the world.

So `sithSound` is the asset/resource-facing layer between world loading and the generic sound core.

### Sound Classes

[`sithSoundClass.c`](../../Libs/sith/World/sithSoundClass.c#L24) defines a semantic sound-mode namespace such as movement, landing, impact, hurt, voice, and state transitions.

The main runtime entry points are:

- [`sithSoundClass_PlayModeFirstEx()`](../../Libs/sith/World/sithSoundClass.c#L547)
- [`sithSoundClass_PlayModeRandom()`](../../Libs/sith/World/sithSoundClass.c#L591)
- [`sithSoundClass_PlayVoiceModeRandom()`](../../Libs/sith/World/sithSoundClass.c#L635)
- [`sithSoundClass_PlayModeEntry()`](../../Libs/sith/World/sithSoundClass.c#L874)

`sithSoundClass` reduces coupling between gameplay code and exact file names. Systems ask for "land hard" or "hurt fire", not for a concrete WAV.

### Awareness Propagation From Sound

[`sithSoundClass_PlayModeEntry()`](../../Libs/sith/World/sithSoundClass.c#L874) does more than play audio.

For certain modes it also creates AI awareness events through [`sithAIAwareness_CreateTransmittingEvent()`](../../Libs/sith/AI/sithAIAwareness.c#L87), which are later processed by [`sithAIAwareness_Update()`](../../Libs/sith/AI/sithAIAwareness.c#L107).

This is one of the most important architectural cross-links in the engine:

- sound is not purely cosmetic
- some played sounds also become AI-perception stimuli

## Voice, Lip Sync, And Subtitles

The voice layer is implemented in [`sithVoice.c`](../../Libs/sith/World/sithVoice.c#L101).

Important entry points are:

- [`sithVoice_PlayThingVoice()`](../../Libs/sith/World/sithVoice.c#L186)
- [`sithVoice_UpdateLipSync()`](../../Libs/sith/World/sithVoice.c#L244)
- [`sithVoice_AddSubtitle()`](../../Libs/sith/World/sithVoice.c#L466)
- [`sithVoice_Draw()`](../../Libs/sith/World/sithVoice.c#L704)

### Voice Playback

[`sithVoice_PlayThingVoice()`](../../Libs/sith/World/sithVoice.c#L186) plays a sound through the same mixer/channel system as ordinary world audio, but it additionally:

- tracks the voice channel handle in the actor/player voice info
- resets lip-sync state
- optionally queues subtitles
- prepares swap-head state for mouth animation

Voice is therefore a specialization layered on the same core audio path, not a separate playback engine.

### Lip Sync

Lip-sync samples come from [`Sound_GenerateLipSync()`](../../Libs/sound/Sound.c#L2534), which can lazily build mouth-position data for a sound.

[`sithVoice_UpdateLipSync()`](../../Libs/sith/World/sithVoice.c#L244) then uses that data to:

- choose a current mouth/open-head variant
- swap meshes on the speaking thing
- remove the swap when playback finishes

So lip sync is not phoneme-driven skeletal animation. It is a mesh/head-swap system sitting on top of sound amplitude analysis.

### Subtitles

[`sithVoice_AddSubtitle()`](../../Libs/sith/World/sithVoice.c#L466) creates wrapped subtitle entries with start/end/display timing, while [`sithVoice_Draw()`](../../Libs/sith/World/sithVoice.c#L704) renders the currently valid lines each frame.

Subtitles are therefore a voice-layer feature, not a generic HUD text system.

## Original Design And OpenJones3D Extensions

The sound architecture itself remains close to the original:

- a low-level bank/channel backend
- a gameplay-aware listener/mixer bridge
- semantic sound classes
- a voice layer on top

OpenJones3D extends that architecture in a few important places without replacing it:

- larger simultaneous channel budget via `SITHSOUND_MAXCHANNELS`
- configurable software falloff with exponential and logarithmic models
- bug fixes in fade, restart, and subtitle handling
- improved modern backend behavior in both DX6 and DX9 paths

So the sound system is a good example of the project's general strategy: keep the original layers, but make the modernized behavior live inside those layers rather than inventing a brand-new audio architecture.
