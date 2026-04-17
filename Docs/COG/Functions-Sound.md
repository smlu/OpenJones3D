# Sound Host Functions

Source: [`Libs/sith/Cog/sithCogFunctionSound.c`](../../Libs/sith/Cog/sithCogFunctionSound.c)

These verbs cover direct sound playback, sound-class helpers, ambient sector sound, and channel fade/wait helpers.

## Compatibility Notes

- [`PlaySoundThing()`](#playsoundthing) and [`PlaySoundPos()`](#playsoundpos) convert the script-supplied min/max radii by multiplying them by `0.1` before passing them to the mixer.
- [`PlayVoiceMode()`](#playvoicemode) is a sound-class helper. It is not the same system as [`PlayVoice()`](Functions-Voice.md#playvoice), which lives in the separate voice module.
- [`WaitForSound()`](#waitforsound) waits on a channel GUID and reuses the cog's generic waiting-for-sound status.
- Signatures below use PascalCase for readability, even though host-function lookup in scripts is case-insensitive.

## Function Index

### Functions

- [PlaySoundThing](#playsoundthing)
- [StopSoundThing](#stopsoundthing)
- [PlaySoundPos](#playsoundpos)
- [PlaySoundLocal](#playsoundlocal)
- [PlaySoundGlobal](#playsoundglobal)
- [StopSound](#stopsound)
- [LoadSound](#loadsound)
- [PlaySoundClass](#playsoundclass)
- [StopSoundClass](#stopsoundclass)
- [PlayVoiceMode](#playvoicemode)
- [ChangeSoundVol](#changesoundvol)
- [ChangeSoundPitch](#changesoundpitch)
- [SectorSound](#sectorsound)
- [GetSoundLen](#getsoundlen)
- [WaitForSound](#waitforsound)
- [StopAllSoundsThing](#stopallsoundsthing)

## Function Reference

### Functions


#### PlaySoundThing

```C++
PlaySoundThing(Sound snd, Thing thing, float volume, float minRadius, float maxRadius, int playFlags) -> int
```

Plays `snd` using `thing` as the sound source, or as a non-positional sound when `thing` is invalid.

Parameters:
- `snd`: Sound resource to play.
- `thing`: Thing to attach the sound to, or `-1`/null for non-positional playback.
- `volume`: Playback volume multiplier.
- `minRadius`: Minimum falloff radius in script units. The engine converts this to mixer units by multiplying by `0.1`.
- `maxRadius`: Maximum falloff radius in script units. The engine converts this to mixer units by multiplying by `0.1`.
- `playFlags`: Mixer playback flags. See [Sound Play Flags](Flags.md#sound-play-flags).

Returns:
- Sound channel GUID, or `-1` on failure.

Notes:
- When `thing` is valid and [`SOUNDPLAY_THING_POS`](Flags.md#sound-play-flags) is clear, the verb plays at the thing position with [`SOUNDPLAY_ABSOLUTE_POS`](Flags.md#sound-play-flags) forced on.
- When sync is enabled and the current message is not `startup` or `shutdown`, the playback request is broadcast to joined players.

#### StopSoundThing

```C++
StopSoundThing(Sound snd, Thing thing)
```

Stops instances of `snd` that are currently playing on `thing`.

Parameters:
- `snd`: Sound resource to stop. Pass `-1`/null to stop all matching channels the mixer treats as unnamed for that thing.
- `thing`: Thing whose attached sound instance should be stopped.

#### PlaySoundPos

```C++
PlaySoundPos(Sound snd, Vector pos, float volume, float minRadius, float maxRadius, int playFlags) -> int
```

Plays `snd` at the world position `pos`.

Parameters:
- `snd`: Sound resource to play.
- `pos`: World position where the sound should originate.
- `volume`: Playback volume multiplier.
- `minRadius`: Minimum falloff radius in script units. The engine converts this to mixer units by multiplying by `0.1`.
- `maxRadius`: Maximum falloff radius in script units. The engine converts this to mixer units by multiplying by `0.1`.
- `playFlags`: Mixer playback flags. See [Sound Play Flags](Flags.md#sound-play-flags). [`SOUNDPLAY_ABSOLUTE_POS`](Flags.md#sound-play-flags) is forced on by the wrapper.

Returns:
- Sound channel GUID, or `-1` on failure.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the playback request is broadcast to joined players.

#### PlaySoundLocal

```C++
PlaySoundLocal(Sound snd, float volume, float pan, int playFlags, int bWait) -> int
```

Plays a non-positional local sound for the current machine.

Parameters:
- `snd`: Sound resource to play.
- `volume`: Playback volume multiplier.
- `pan`: Stereo pan in the range `[-1.0, 1.0]`. The wrapper clamps out-of-range values.
- `playFlags`: Mixer playback flags. See [Sound Play Flags](Flags.md#sound-play-flags). Positional flags are cleared by the wrapper.
- `bWait`: Non-zero to block the current COG until the channel stops.

Returns:
- Sound channel GUID, or `-1` on failure.

Notes:
- This verb does not broadcast playback to other players.

#### PlaySoundGlobal

```C++
PlaySoundGlobal(Sound snd, float volume, float pan, int playFlags, int bWait) -> int
```

Plays a non-positional sound and, when sync is enabled, broadcasts that playback to joined players.

Parameters:
- `snd`: Sound resource to play.
- `volume`: Playback volume multiplier.
- `pan`: Stereo pan in the range `[-1.0, 1.0]`. The wrapper clamps out-of-range values.
- `playFlags`: Mixer playback flags. See [Sound Play Flags](Flags.md#sound-play-flags). Positional flags are cleared by the wrapper.
- `bWait`: Non-zero to block the current COG until the channel stops.

Returns:
- Sound channel GUID, or `-1` on failure.

Notes:
- If `bWait` is non-zero, the current COG enters the generic waiting-for-sound state.

#### StopSound

```C++
StopSound(int channelGuid, float fadeSeconds)
```

Stops or fades out the active sound channel identified by `channelGuid`.

Parameters:
- `channelGuid`: Channel GUID returned by one of the sound-playback verbs.
- `fadeSeconds`: Fade-out time in seconds. Values above `0` fade to zero volume; `0` or below stops immediately.

Notes:
- When sync is enabled and the current message is not `startup` or `shutdown`, the stop request is broadcast to joined players.

#### LoadSound

```C++
LoadSound(string filename) -> Sound
```

Loads a sound resource from the current world's `sound\\` directory and returns its sound index.

Parameters:
- `filename`: Sound filename relative to the `sound\\` directory, for example `gen_boulder_roll.wav`.

Returns:
- Sound index for the loaded resource, or `-1` when the filename is invalid, the filename is `none`, or the load fails.

Notes:
- The wrapper forwards `filename` to `sithSound_Load()`, which prepends `sound\\` before loading.


#### PlaySoundClass

```C++
PlaySoundClass(Thing thing, int mode) -> int
```

Plays a random entry from the requested sound-class `mode` on `thing`.

Parameters:
- `thing`: Thing whose sound class should be used.
- `mode`: Sound-class mode to play. See [Sound Class Modes](Types-And-Modes.md#sound-class-modes).

Returns:
- Sound channel GUID, or `-1` when playback could not be started.

Notes:
- Requires a thing with a sound class assigned.

#### StopSoundClass

```C++
StopSoundClass(Thing thing, int mode)
```

Stops playback of the requested sound-class `mode` on `thing`.

Parameters:
- `thing`: Thing whose sound-class mode should be stopped.
- `mode`: Sound-class mode to stop. See [Sound Class Modes](Types-And-Modes.md#sound-class-modes).

Notes:
- Requires a thing with a sound class assigned.

#### PlayVoiceMode

```C++
PlayVoiceMode(Thing thing, int mode)
```

Plays a random voice line from the requested voice sound-class `mode` on `thing`.

Parameters:
- `thing`: Thing whose voice sound class should be used.
- `mode`: Voice sound-class mode to play. See [Sound Class Modes](Types-And-Modes.md#sound-class-modes).

#### ChangeSoundVol

```C++
ChangeSoundVol(int channelGuid, float volume, float fadeSeconds)
```

Fades the volume of an active channel to `volume` over `fadeSeconds`.

Parameters:
- `channelGuid`: Channel GUID to modify.
- `volume`: Target volume multiplier.
- `fadeSeconds`: Fade duration in seconds. This value must be greater than `0`.

#### ChangeSoundPitch

```C++
ChangeSoundPitch(int channelGuid, float pitch, float fadeSeconds)
```

Fades the pitch of an active channel to `pitch` over `fadeSeconds`.

Parameters:
- `channelGuid`: Channel GUID to modify.
- `pitch`: Target pitch multiplier.
- `fadeSeconds`: Fade duration in seconds. This value must be greater than `0`.

#### SectorSound

```C++
SectorSound(Sector sector, Sound snd, float volume)
```

Sets the ambient sound for `sector`.

Parameters:
- `sector`: Sector whose ambient sound should be updated.
- `snd`: Ambient sound resource to assign.
- `volume`: Ambient playback volume multiplier.

#### GetSoundLen

```C++
GetSoundLen(Sound snd) -> float
```

Returns the duration of `snd` in seconds.

Parameters:
- `snd`: Sound resource whose duration should be queried.

Returns:
- Sound duration in seconds, or `0.0` when the sound handle is invalid.

#### WaitForSound

```C++
WaitForSound(int channelGuid) -> int
```

Blocks the current COG until the sound channel identified by `channelGuid` stops.

Parameters:
- `channelGuid`: Channel GUID to wait on.

Returns:
- The same channel GUID when waiting started, or `-1` when `channelGuid` is zero.

Notes:
- This uses the same waiting-for-sound cog status as [`PlaySoundLocal()`](#playsoundlocal), [`PlaySoundGlobal()`](#playsoundglobal), and [`PlayVoice()`](Functions-Voice.md#playvoice).

#### StopAllSoundsThing

```C++
StopAllSoundsThing(Thing thing)
```

Stops every sound channel currently attached to `thing`.

Parameters:
- `thing`: Thing whose attached sound channels should be stopped.
