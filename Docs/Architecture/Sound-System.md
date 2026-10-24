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
- [`Libs/sound/AudioLib.c`](../../Libs/sound/AudioLib.c)
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

The voice layer is implemented in [`sithVoice.c`](../../Libs/sith/World/sithVoice.c).

Important entry points are:

- [`sithVoice_PlayThingVoice()`](../../Libs/sith/World/sithVoice.c)
- [`sithVoice_UpdateLipSync()`](../../Libs/sith/World/sithVoice.c)
- [`sithVoice_AddSubtitle()`](../../Libs/sith/World/sithVoice.c)
- [`sithVoice_Draw()`](../../Libs/sith/World/sithVoice.c)

### Voice Playback

[`sithVoice_PlayThingVoice()`](../../Libs/sith/World/sithVoice.c) plays a sound through the same mixer/channel system as ordinary world audio, but it additionally:

- tracks the voice channel handle in the actor/player voice info
- resets lip-sync state
- optionally queues subtitles
- prepares swap-head state for mouth animation

Voice is therefore a specialization layered on the same core audio path, not a separate playback engine.

### Lip Sync

In short, the lip-sync system analyzes 16-bit mono PCM at 60 analysis frames per second. It converts amplitude and zero-crossing activity into two quantized mouth coordinates, stores only coordinate changes in a compact `SYNC` timeline, and looks up that timeline while the voice sound plays. `sithVoice` then maps the coordinates through a 4 x 4 table to one of four replacement head models authored as the M-sound, A-sound, AM-sound, and O-sound mouth poses. The shipped scripts configure every column in a row identically, so the game ultimately selects these models from mouth Y alone.

This is a crude waveform-driven viseme selector rather than speech or phoneme recognition. Mouth Y primarily follows normalized amplitude and acts as a coarse openness value. Mouth X follows positive-going zero-crossing density and could supply a rough spectral cue through a different 4 x 4 table, but the game's table configuration ignores it.

Here, "lip sync" names the complete generation and playback process, while "mouth X/Y" names the two abstract mouth-shape coordinates stored in that data. They are not geometric positions of individual lips. This distinction is reflected in names such as `AUDIOLIB_LIPSYNC_MOUTH_POSITION_COUNT`: the lip-sync timeline carries mouth-position values.

#### Timeline Generation

[`Sound_GenerateLipSync()`](../../Libs/sound/Sound.c) generates and caches the timeline lazily the first time a playing sound needs lip sync. Compressed sound data is first expanded into temporary PCM. The generator accepts only 16-bit, single-channel PCM; failures leave no partial cache entry and return neutral mouth coordinates.

[`AudioLib_GenerateLipSyncBlock()`](../../Libs/sound/AudioLib.c) retains the recovered algorithm:

1. Scan adjacent sample pairs across the waveform, beginning with the second analyzed sample, to find the maximum absolute amplitude and total number of negative-to-nonnegative zero crossings.
2. Divide the PCM into complete analysis frames using `2 * sampleRate / analysisRateHz` bytes per frame. Sound requests an analysis rate of 60 Hz.
3. For each frame, measure peak amplitude, average absolute amplitude, and positive-going zero crossings. The first sample is used only as the predecessor of the second sample, its amplitude is not included, and the pair spanning two adjacent frames is never examined.
4. Compare the frame peak with a 15-frame rolling average of recent peaks. The rolling value is kept at least one quarter of the whole-wave peak so quiet sections do not become disproportionately large.
5. Derive raw mouth Y from the frame peak and raw mouth X from the frame's zero-crossing count relative to the whole-wave rate. Together these features choose a coarse viseme-like state; quiet frames blend X toward the recovered neutral value of 37.
6. Clamp each raw coordinate to 100 percent, scale it into the 7-bit range 0 through 127, and apply the recovered transient decay when average frame amplitude is less than 30 percent of its peak.
7. Quantize both coordinates to the requested number of levels. Sound requests four levels, producing values 0, 32, 64, or 96 for each axis. [`SOUND_LIPSYNC_GETMOUTHLEVEL()`](../../Libs/sound/Sound.h) converts those values back to table levels `0` through `3`.
8. Emit an entry only when either quantized coordinate changes, then append a final timestamped entry with both coordinates zero.

The resulting little-endian block contains the four-byte `SYNC` signature, a 32-bit entry count, and packed 32-bit entries. Time advances by `4096000 / analysisRateHz`, where 4096 fixed-point units represent one millisecond. Each entry stores the upper 16 bits of that time key, mouth X in bits 8 through 14, and mouth Y in bits 0 through 6. [`AudioLib_GetMouthPosition()`](../../Libs/sound/AudioLib.c) binary-searches these entries for a playback time.

The reconstructed Sound path uses capacity-aware helpers to validate the PCM source range and allocate a worst-case output buffer instead of relying on the original fixed 8192-byte stack buffer. That original buffer could hold the worst case for 2045 analysis frames, or about 34.1 seconds at 60 Hz, but this was not a hard duration limit: an entry is written only when the quantized mouth state changes, so less variable audio could be much longer. Conversely, the unchecked generator had no output-capacity parameter and would overrun the stack buffer rather than report that it was full.

The fixed stack scratch was likely an efficiency and allocation-simplicity choice: the generated `SYNC` output required no temporary heap allocation, and Sound allocated the exact persistent block only after generation succeeded and revealed its encoded size. Compressed PCM still required a separate temporary decompression buffer. This rationale is inferred from the recovered ownership flow; no original source comment states it explicitly.

#### Legacy Cross-Sample Decoding

The original `Sound_GenerateLipSync()` call enables `bCrossSample`, and `AudioLib_GenerateLipSyncBlock()` responds by advancing the PCM pointer by exactly one byte before reading 16-bit samples. Both the uncompressed and decompressed paths pass a pointer to the first PCM byte, so this does not skip a known header. It is a one-byte shift, not a one-sample shift. Given adjacent little-endian source samples `{lowN, highN}` and `{lowNPlus1, highNPlus1}`, the reconstructed word contains `{highN, lowNPlus1}`, or `int16_t((lowNPlus1 << 8) | highN)`: the current sample's high byte becomes the synthetic word's low byte, while the adjacent sample's low byte becomes its high byte.

This cross-splicing creates a synthetic sample sequence rather than averaging or interpolating the adjacent samples. The next sample's low byte supplies the sign and coarse magnitude bits of the synthetic value, while the current sample's high byte supplies its low-order magnitude bits. The resulting value can therefore differ sharply in sign and amplitude from either source sample. The generator measures peak and average absolute amplitude plus positive-going zero crossings from this synthetic sequence, so cross-sample decoding can alter both generated mouth coordinates. No rationale for this behavior is visible in the recovered code; OpenJones3D preserves it for output compatibility.

The generator accepts only 16-bit mono PCM, so cross-sample decoding operates entirely within one channel's byte stream. It neither combines channels nor selects only high- or low-amplitude samples.

The inspected game voice files use 11025 Hz PCM, producing an integer-truncated analysis-frame stride of 367 bytes. Because this stride is odd, each new frame starts on the opposite byte of a 16-bit sample. Enabling cross-sample decoding therefore makes frame zero start on a high byte, frame one on a correctly aligned low byte, and so on; disabling it reverses that parity. The game configures a 250-millisecond lookup offset and 0.15-second head-swap interval. At an ideal 60 Hz cadence, the first lookup is 15 analysis frames ahead and later lookups advance by nine frames, alternating frame parity. `AudioLib_GetMouthPosition()` also quantizes time to 16-millisecond keys and the timeline stores only state changes, so a lookup can retain a state emitted by an earlier frame. The runtime settings therefore do not support interpreting the flag as a fixed alignment-phase selector. The initial whole-wave normalization scan remains shifted throughout and interprets promoted low-order sample bytes, making the resulting timeline materially different from an aligned analysis.

The cross-sample read also explains the temporary-buffer padding. A shifted 16-bit read can need one byte beyond the declared PCM data. If `2 * sampleRate / analysisRateHz` is odd, the final frame can require a second byte. The checked API therefore requires up to two readable padding bytes; the temporary decompression path allocates and zeroes both. It still passes the original unpadded `sndDataSize` to the algorithm.

#### Playback And Voice-Head Mapping

During playback, [`Sound_GenerateLipSync()`](../../Libs/sound/Sound.c) converts the sound buffer's current byte position to milliseconds, adds the configured lip-sync lookup offset, and asks `AudioLib_GetMouthPosition()` for the current pair. The compiled fallback values are a 50-millisecond lookup offset and 0.1-second head-swap interval. `actor_indy.cog` replaces them game-wide with `SetVoiceParams(0.15, 250)`, producing a 250-millisecond look-ahead and updates no more often than every 0.15 seconds. A lookup beyond the final generated entry returns its neutral zero state, so the mouth can close approximately one look-ahead interval before playback ends. Sound stops requesting active mouth data during the final 25 milliseconds.

[`sithVoice_UpdateLipSync()`](../../Libs/sith/World/sithVoice.c) uses `SOUND_LIPSYNC_GETMOUTHLEVEL()` for both coordinates, then indexes the global 4 x 4 voice-head table as `[mouthYLevel][mouthXLevel]`. X support is therefore present in the runtime; `actor_indy.cog` neutralizes it by configuring rows `0` through `3` as `{0,0,0,0}` through `{3,3,3,3}`. Mouth Y consequently selects one of the four talking-head slots assigned by [`SetThingVoiceHeads()`](../COG/Functions-Voice.md#setthingvoiceheads). The legacy API calls these table values head "heights," but they are model-slot indices rather than geometric measurements. Their filenames establish the M-sound, A-sound, AM-sound, and O-sound ordering. Most normal actor setups leave the M-sound slot empty so the base head supplies the closed-lip state; cinematic and costume-specific setups may provide an explicit M-sound model or alternate base head. To prevent a static pose, the voice layer substitutes every third consecutive selection of the same nonzero head: A-sound becomes O-sound, AM-sound becomes M-sound, and O-sound becomes A-sound. The resulting model is installed as a swap entry on the configured head mesh. When playback or lip-sync lookup ends, the voice layer removes or restores the voice-head swap and clears the channel state.

Inspection of the authored 3DO head sets supports those filename meanings. The M-sound variants commonly omit mouth-interior vertices present in the open poses. In Sophia's four-pose set, the A-sound model has the greatest lower-lip/jaw displacement, AM is intermediate, and O is less open but has the narrowest mouth corners. These are authored approximations of a closed M pose, a wide A pose, an intermediate AM pose, and a rounded O pose; the runtime waveform analysis does not recognize those phonemes directly.

The table and timing parameters are global, while target meshes, head-model slots, subtitle colors, and active channel handles belong to individual actor/player voice records. The COG-facing contract and parameter meanings are documented in [Voice Host Functions](../COG/Functions-Voice.md).

#### Current Voice-Lifecycle Risks

The recovered ownership is only partly per speaker. The lookup table and configured timing values are intentionally global, but the current head slot, previous head slot, repetition counter, next-swap deadline, and `sithVoice_bThingHasSwapHead` are global as well. Consequently, overlapping speakers can affect one another's update timing and repeated-pose substitution. The existing-head flag can also be consumed while cleaning up a different speaker. Starting a new tracked voice resets part of this state, but `sithVoice_Open()` and `sithVoice_Close()` do not reset the previous-head state or next-swap deadline.

This creates a specific savegame hazard. Restore closes and reopens the Sith world, then restores the saved game time only near the end. The global next-swap deadline is neither serialized by `sithVoice_SyncVoiceState()` nor reset by open/close, so loading an earlier point in the timeline can leave a deadline in the future and suppress head updates until restored time catches up.

Active voice playback has a separate restore mismatch. Actor/player DSS state serializes `SithActorVoiceInfo::hSndChannel`, but `Sound_Save()` currently writes only 3D looping or far channels. `sithVoice_PlayThingVoice()` uses the generic `Sound_Play()` path for a 2D one-shot, so its channel is not present in the Sound save section. The actor can therefore restore a stale channel handle; the next lip-sync lookup cannot resolve it and clears the tracked voice/head state. This is the strongest code-level explanation for a voice head remaining neutral after loading a save during dialogue.

Window deactivation is another possible desynchronization path. `JonesMain_OnAppActivate()` switches processing to window events but does not call the explicit `JonesMain_PauseGame()`/`Sound_Pause()` path. Game-time and head updates stop while backend playback behavior is left to the active sound-buffer/focus state, with no resynchronization step on activation. This can leave the audio cursor ahead of the voice-head state or let the line finish before updates resume.

### Subtitles

[`sithVoice_AddSubtitle()`](../../Libs/sith/World/sithVoice.c) creates wrapped subtitle entries with start/end/display timing, while [`sithVoice_Draw()`](../../Libs/sith/World/sithVoice.c) renders the currently valid lines each frame.

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
