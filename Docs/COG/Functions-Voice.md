# Voice Host Functions

Source: [`Libs/sith/World/sithVoice.c`](../../Libs/sith/World/sithVoice.c)

These verbs drive spoken voice playback, subtitle display, and swap-head lip-sync support. They are separate from the generic sound verbs and from the [`PlayVoiceMode()`](Functions-Sound.md#playvoicemode) sound-class helper.

Signatures below use PascalCase for readability, even though host-function lookup in scripts is case-insensitive.

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
PlayVoice(Thing thing, Sound snd, float volume, int bWait) -> int
```

Plays voiced dialogue on `thing`, including subtitle and swap-head support when that actor is configured for it.

Parameters:
- `thing`: Actor or player thing that should speak the line.
- `snd`: Voice sound resource to play.
- `volume`: Playback volume multiplier.
- `bWait`: Non-zero to block the current COG until the voice channel stops.

Returns:
- Sound channel GUID, or `-1` on failure.

Notes:
- Requires an actor or player thing with a model.
- When `thing` uses the special `aet_gy.3do` model, the verb falls back to direct mixer playback instead of the normal voice/subtitle path.
- If `bWait` is non-zero, the current COG enters the generic waiting-for-sound state.

#### SetThingVoiceHeads

```C++
SetThingVoiceHeads(Thing thing, string meshName, string head0, string head1, string head2, string head3)
```

Assigns the mesh slot and four head models used by the swap-head voice system for `thing`.

Parameters:
- `thing`: Actor or player thing whose voice-head setup should be changed.
- `meshName`: Name of the mesh on the base model that should be swapped during speech.
- `head0`: Model name for the first voice head.
- `head1`: Model name for the second voice head.
- `head2`: Model name for the third voice head.
- `head3`: Model name for the fourth voice head.

Notes:
- Requires an actor or player thing.

#### SetVoiceHeadHeight

```C++
SetVoiceHeadHeight(int row, int h0, int h1, int h2, int h3)
```

Sets one row of the global voice-head height lookup table used by the swap-head system.

Parameters:
- `row`: Voice-head row to update. Values above `3` are clamped to `3`.
- `h0`: Height index for the first mouth state. Values above `3` are clamped to `3`.
- `h1`: Height index for the second mouth state. Values above `3` are clamped to `3`.
- `h2`: Height index for the third mouth state. Values above `3` are clamped to `3`.
- `h3`: Height index for the fourth mouth state. Values above `3` are clamped to `3`.

#### SetThingVoiceColor

```C++
SetThingVoiceColor(Thing thing, Vector color0, Vector color1, Vector color2, Vector color3)
```

Sets the four gradient colors used when the voice system renders subtitle text for `thing`.

Parameters:
- `thing`: Actor or player thing whose subtitle colors should be updated.
- `color0`: First subtitle gradient color.
- `color1`: Second subtitle gradient color.
- `color2`: Third subtitle gradient color.
- `color3`: Fourth subtitle gradient color.

Notes:
- Requires an actor or player thing.

#### SetVoiceParams

```C++
SetVoiceParams(float headSwapInterval, int mouthYOffset)
```

Sets global timing and layout parameters for the voice/subtitle system.

Parameters:
- `headSwapInterval`: Time in seconds between swap-head updates while speech is playing.
- `mouthYOffset`: Global integer offset stored by the voice system for head/text placement.
