# 1. Introduction and goals

## 1.1 What Jones3D EX is

Jones3D EX is an enhanced source port of Jones3D, the engine of *Indiana Jones and the Infernal Machine*
(1999). It runs the original game on modern systems: Windows, Linux and macOS, with a modern renderer, and
with the original game data and savegames.

It follows the model of Nightdive Studios' Kex engine. Kex is not a game engine but a platform framework:
input, rendering, audio, files and platform services. The original game logic of each game sits on top of
it, mostly unchanged. The Quake II rerelease is a public example: its game module keeps the original game
code, and talks to the closed Kex engine through a cleaned-up interface.

Jones3D EX does the same with two parts:

- **The game:** the restored Jones3D and Sith game logic, in C. It decides what happens in the world.
- **The Babylon Engine:** a game-agnostic platform framework in C++. It decides how that reaches the
  screen, the speakers and the player's hands. Babylon knows nothing about Jones3D, and could carry
  another game.

## 1.2 Where it comes from

Jones3D EX is a hard fork of [OpenJones3D](https://github.com/smlu/OpenJones3D), which reconstructed the
engine function by function from the retail executable. To check each function, OpenJones3D hooked it
into the running original and kept every name, struct layout and global address identical. That work is
finished: all 3180 functions are reconstructed. A byte-exact decompilation of the game also exists.

So the reconstruction no longer needs protecting, and the rules that protected it now only stand in the way
of a port. Jones3D EX drops them, and keeps what they produced: the original behavior. The last state
compatible with upstream is tagged `upstream-compatible-final`.

## 1.3 Requirements overview

| Requirement | Meaning |
|---|---|
| Original gameplay | The simulation (physics, AI, COG scripts, savegames) is measured against retail behavior. OpenJones3D's QOL changes are on by default, and each one can be switched back to retail |
| Stock data | Original game files and savegames work without conversion |
| Modern presentation | High resolutions, widescreen, a modern renderer and audio by default, with classic toggles where sensible |
| Three desktop platforms | Windows, Linux and macOS from one codebase |
| Portable framework | Nothing in Babylon assumes a desktop, so other platforms stay possible |
| Moddable | Loose-file overrides and mod folders, with COG as the scripting language |

## 1.4 Quality goals

Ranked; when two conflict, the higher one wins.

| # | Quality goal | Scenario |
|---|---|---|
| 1 | **Fidelity** of the simulation | With the retail profile, a scripted headless run from a savegame produces the same state trace as the reference build, frame for frame |
| 2 | **Portability** | A new platform needs new Babylon backends only, no change in the game or in Babylon's interfaces |
| 3 | **Separability** | `Babylon/` builds and passes its tests with nothing from the game on its include path |
| 4 | **Maintainability** | A change to a backend touches one Babylon module and its tests, nothing above the adapter |
| 5 | **Performance** | Smooth on modest hardware at high resolutions and refresh rates |

Chapter 10 expands these into a quality tree with more scenarios.

## 1.5 Stakeholders

| Who | What they expect |
|---|---|
| Players | The game they remember, working on their system, looking better |
| Speedrunners | Movement, physics and timing exactly as the original, or a clearly marked mode that keeps them |
| Modders | Documented formats, overrides without patching files, COG access |
| Contributors | Clear module boundaries, tests, and rules for what may change |
| Babylon users | A framework they can use for another game, under MPL-2.0 |
| Rights holders | No game assets or original executables in the repository |
