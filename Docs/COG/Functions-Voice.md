# Voice Host Functions

Source: [`Libs/sith/World/sithVoice.c`](../../Libs/sith/World/sithVoice.c)

These verbs drive spoken voice playback, subtitle display, and swap-head lip-sync support. They are separate from the generic sound verbs and from the [`PlayVoiceMode()`](Functions-Sound.md#playvoicemode) sound-class helper.

Signatures below use PascalCase for readability, even though host-function lookup in scripts is case-insensitive.

## Lip-Sync Terms And Mapping

The voice system uses two abstract mouth-shape coordinates generated from the waveform. They are not literal positions of individual lips:

- Mouth X is derived primarily from positive-going zero-crossing density and acts as a coarse frequency/shape cue.
- Mouth Y is derived primarily from normalized peak amplitude and acts as a coarse mouth-opening cue.

Sound requests four quantization levels per axis, encoded as `0`, `32`, `64`, and `96`. `sithVoice` converts each coordinate to a level from `0` through `3` with [`SOUND_LIPSYNC_GETMOUTHLEVEL()`](../../Libs/sound/Sound.h), then selects a head-model slot as follows:

```text
headSlot = voiceHeadTable[mouthYLevel][mouthXLevel]
```

The table is global, while the four head models and target mesh are configured per actor/player. The legacy `SetVoiceHeadHeight` name calls the table values "heights," but each value is actually a head-model slot from `0` through `3`, not a geometric height.

## Function Index

### Functions

- [PlayVoice](#playvoice)
- [SetThingVoiceHeads](#setthingvoiceheads)
- [SetVoiceHeadHeight](#setvoiceheadheight)
- [SetThingVoiceColor](#setthingvoicecolor)
- [SetVoiceParams](#setvoiceparams)

## Function Reference

### Functions


#### PlayVoice

```C++
PlayVoice(Thing speaker, Sound voiceSound, float volume, int wait) -> int
```

Plays voiced dialogue for `speaker`, including subtitle and swap-head support when that actor is configured for it.

Parameters:
- `speaker`: Actor or player thing that should speak the line and own its voice state.
- `voiceSound`: Voice sound resource to play.
- `volume`: Playback volume multiplier passed to the normal voice path.
- `wait`: Non-zero to suspend the current COG until the returned voice channel stops.

Returns:
- Sound channel GUID, or `-1` on failure.

Notes:
- Requires an actor or player thing with a model. Invalid thing or sound arguments return `-1`.
- Starting another line on the same speaker stops its currently tracked voice line and removes that line's pending subtitle entries.
- Subtitle text is looked up using the uppercased voice-sound filename when subtitle display is enabled.
- When `speaker` uses the special `aet_gy.3do` model, the verb uses direct mixer playback at volume `1.0` and bypasses the normal tracked voice, subtitle, and swap-head path.
- If `wait` is non-zero, the current COG enters the generic waiting-for-sound state. The returned value is still the channel GUID used by sound-control verbs.

#### SetThingVoiceHeads

```C++
SetThingVoiceHeads(Thing speaker, string headMesh, string mHead, string aHead, string amHead, string oHead)
```

Assigns the target mesh and four head-model slots used by the swap-head voice system for `speaker`. The model filenames label the slots as the M-sound, A-sound, AM-sound, and O-sound mouth poses.

Parameters:
- `speaker`: Actor or player thing whose voice-head setup should be changed.
- `headMesh`: Name of the mesh on the speaker's base model that should be replaced during speech.
- `mHead`: Model for slot `0`, the closed/neutral M-sound mouth pose.
- `aHead`: Model for slot `1`, the A-sound mouth pose.
- `amHead`: Model for slot `2`, the AM-sound mouth pose.
- `oHead`: Model for slot `3`, the O-sound mouth pose.

Notes:
- Requires an actor or player thing.
- `headMesh` must name an existing mesh on the speaker model. The function stores the resolved mesh index for later swap entries.
- Head names are resolved from models already present in the current or static world; this function does not load model files.
- An empty or unresolved model name leaves that slot without a replacement model. Most normal actor setups, including Indy, Sophia, and Turner, leave `mHead` empty so selecting slot `0` removes the speech replacement and exposes the existing/base head. Cinematic and costume-specific setups may provide an explicit M-sound model or alternate base head.
- The labels describe the authored poses. Selection remains waveform-driven rather than phoneme recognition.

#### SetVoiceHeadHeight

```C++
SetVoiceHeadHeight(int mouthYLevel, int headAtX0, int headAtX1, int headAtX2, int headAtX3)
```

Sets one mouth-Y row of the global 4 x 4 voice-head lookup table used by the swap-head system. The four values correspond to the four quantized mouth-X columns and select one of the four models configured by `SetThingVoiceHeads()`.

Parameters:
- `mouthYLevel`: Table row selected by quantized mouth Y.
- `headAtX0`: Head-model slot used when mouth X quantizes to level `0`.
- `headAtX1`: Head-model slot used when mouth X quantizes to level `1`.
- `headAtX2`: Head-model slot used when mouth X quantizes to level `2`.
- `headAtX3`: Head-model slot used when mouth X quantizes to level `3`.

Notes:
- Valid row and head-slot values are `0` through `3`. The current implementation maps any out-of-range value, including a negative value, to `3`.
- Quantized coordinate ranges are `0..31` for level `0`, `32..63` for level `1`, `64..95` for level `2`, and `96..127` for level `3`.
- The table is shared by all speakers. A call replaces one global row; it does not configure only the actor whose COG made the call.
- The shipped scripts initialize rows `0` through `3` to `{0,0,0,0}`, `{1,1,1,1}`, `{2,2,2,2}`, and `{3,3,3,3}`. This makes mouth Y directly select the M-sound, A-sound, AM-sound, or O-sound slot and deliberately neutralizes mouth X.
- A nonuniform row such as `SetVoiceHeadHeight(1, 0, 1, 2, 3)` would let mouth X choose among all four head slots while mouth Y is at level `1`. The zero-crossing-derived X signal can be noisy, so this is supported behavior rather than the shipped configuration.
- After the table lookup, the runtime occasionally substitutes a repeated nonzero head to avoid a static pose. See [Playback And Voice-Head Mapping](../Architecture/Sound-System.md#playback-and-voice-head-mapping).

#### SetThingVoiceColor

```C++
SetThingVoiceColor(Thing speaker, Vector topColor, Vector middleColor, Vector bottomLeftColor, Vector bottomRightColor)
```

Sets the four Gouraud-gradient colors copied into subsequently queued subtitle lines for `speaker`.

Parameters:
- `speaker`: Actor or player thing whose subtitle colors should be updated.
- `topColor`: RGB color for the top subtitle vertices.
- `middleColor`: RGB color for the middle subtitle vertices.
- `bottomLeftColor`: RGB color for the bottom-left subtitle vertex.
- `bottomRightColor`: RGB color for the bottom-right subtitle vertex.

Notes:
- Requires an actor or player thing.
- The vectors supply RGB only; the stored alpha component is forced to `1.0`.
- If the red component of `topColor` is `-1.0`, subtitle creation uses the engine's default red-to-yellow gradient instead of the four stored colors.
- Existing queued subtitle entries keep the colors copied when they were created.

#### SetVoiceParams

```C++
SetVoiceParams(float headSwapIntervalSec, int lookupOffsetMsec)
```

Sets the global voice-head update interval and lip-sync timeline lookup offset.

Parameters:
- `headSwapIntervalSec`: Minimum time in seconds between voice-head table lookups and swap updates.
- `lookupOffsetMsec`: Milliseconds added to the current sound playback time before looking up the mouth coordinates. Positive values look ahead in the generated timeline.

Notes:
- Both values are global and affect every speaker. They are included with the lookup table in the synchronized voice state sent to joined players.
- The function does not validate or clamp either argument. Scripts should use a non-negative interval and lookup offset.
- `lookupOffsetMsec` changes only playback-time lookup. It does not seek the sound or control the legacy cross-sample decoding used while generating a `SYNC` block.
- `actor_indy.cog` applies `SetVoiceParams(0.15, 250)` as the game's global voice configuration: a 0.15-second minimum head-swap interval and a 250-millisecond timeline look-ahead. These replace the compiled fallback values of 0.1 seconds and 50 milliseconds.
