# Planned enhancements

Enhancements the owner has asked for, with what each needs from the game code and from the Babylon Engine.
The [roadmap](Roadmap.md) says when the milestones they depend on come; this page is the list to plan from.

Rules that apply to all of them ([ADR-0003](arc42/09-decisions/ADR-0003-fidelity-split.md)):

- Players choose between two profiles: **Enhanced** (the default, every enhancement on) and **Retail** (the
  original game's behavior).
- **Presentation** enhancements (rendering, UI, audio output) may change freely.
- **Simulation** enhancements (anything that changes physics, timing, AI, COG or savegame state) are on in
  Enhanced and off in Retail, each as its own simulation cvar ([ADR-0020](arc42/09-decisions/ADR-0020-cvar-flags.md)).
  With the Retail profile the retail trace must stay identical.
- Visual effects must not draw from the simulation's random numbers or change game state, or they become
  simulation changes.

## The list

| # | Enhancement | Kind | Game side | Babylon needs | Earliest |
|---|---|---|---|---|---|
| E1 | **Weapon and item wheel**: hold a button, pick a weapon or item with the stick or the mouse | Presentation; time slowing down while the wheel is open would be simulation (Enhanced on, Retail off) | Selection goes through the existing weapon and inventory selection, the same actions as the number keys | Gamepad sticks and mouse motion in the input adapter (today's TODOs); a 2D overlay layer (sprites, text, simple shapes) drawn over the 3D view; input routing so the wheel takes the input while open | M7 for input, M9 for the overlay |
| E2 | **Custom inventory**: a new inventory screen | Presentation if it only shows and selects; simulation if it changes what items do | Reads and selects through the existing inventory code | The same 2D overlay layer and input routing as E1; UI navigation with keyboard, mouse and gamepad | M9 |
| E3 | **Texture upscaling in the installer** (Real-ESRGAN or similar) | Presentation, offline tooling | None: the game loads the upscaled textures instead of the originals | High-resolution texture overrides: loose-file or mod-folder overrides (M11 modding), 32-bit RGBA textures with mipmaps next to the 8-bit palettized originals, texture coordinates independent of the stored size; the installer (M8) runs the upscaler as an external tool and writes an override pack | M8 installer, needs the override loading |
| E4 | **Faster crouching and weapon holster and unholster animations** | Simulation: animation timing changes when actions finish (Enhanced on, Retail off) | Playback speed options in the puppet and key-frame code (`sithPuppet`), one cvar per action group | Simulation cvars (M5.8, in the game from M5.13) | After M5.13 |
| E5 | **Overlay weather effects**, such as snow in outdoor levels | Presentation, as long as it neither changes game state nor uses the simulation's random numbers | Per-level settings saying where and what (outdoor sectors, density) | A particle or screen-space overlay pass in the renderer, its own random number source, per-level effect data | M9, after the renderer (M6) |
| E6 | **Cheat menu** | Presentation for the menu itself; the cheats are the existing console commands (`devmode`, `invul`, `weapons`, ...) and change game state as they do today | Calls the existing console commands; may mark a savegame as cheated if speedrun or statistics rules need it | The 2D overlay layer and input routing (E1); the visible console (M11) shares the command plumbing | M9, or with the visible console |

## Shared needs

E1, E2 and E6 need the same three Babylon pieces, so they are planned together:

1. **A 2D overlay layer** drawn after the 3D view: textured quads, text with a font, simple shapes, scaled for
   any resolution. It is also what the M7 in-game dialogs need.
2. **Input routing**: an open overlay takes keyboard, mouse and gamepad input before the game does, and gives
   it back when it closes.
3. **Gamepad and mouse motion** in the input adapter (the TODOs in `stdControlDevicesBabylon.cpp`).

## Open questions

- E2: what "custom" means: a redesigned screen, items rearranged by the player, or new items defined by mods.
- E3: which upscaler and models: Real-ESRGAN's code is BSD-3-Clause, but model weights have their own
  licenses; the installer would run it as a separate program, never bundled into Babylon. Where it runs
  (CPU or GPU) decides how long installation takes.
- E4: which actions exactly, and how much faster; whether the speed is one setting or one per action.
- E5: which levels count as outdoor, and whether the effect follows the camera or the level geometry.
- E6: whether using cheats marks a savegame or the statistics.
