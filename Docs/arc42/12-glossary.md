# 12. Glossary

| Term | Meaning |
|---|---|
| Adapter | The recovered platform functions (`std3D_*`, `stdControl_*`, `Sound_*` and similar) whose bodies call Babylon. The only place where the game meets Babylon |
| Babylon Engine | Jones3D EX's game-agnostic platform framework in `Babylon/`, C++, MPL-2.0 |
| Babylon extras | Features added after the first release: console, presentation upgrades, modding, threading |
| Backend | One implementation of a Babylon interface, such as SDL3, SDL3 GPU, miniaudio or Null |
| COG | The game's scripting language. Level logic, cutscenes and items are COG scripts run by the COG VM in `sith` |
| Cvar | Console variable: a named, typed Babylon setting |
| Decompilation | The byte-exact decompilation of the game, used as the tie-breaker for retail behavior |
| GOB | The game's archive format for its data files |
| Jones3D | The engine of the original game, a descendant of LucasArts' Sith engine. Also the application layer in `Jones3D/` |
| Jones3D EX | This project: an enhanced source port of Jones3D on the Babylon Engine |
| Kex | Nightdive Studios' closed platform framework for remasters, the model for Babylon |
| Null backend | A backend that honors an interface's contract without a device: draws are discarded, no key is pressed |
| OpenJones3D | The upstream project this is forked from, a function-by-function reconstruction |
| Presentation | Rendering, resolution, audio output and UI. Free to change |
| Enhanced profile | The default: every enhancement on (OpenJones3D's QOL changes, now Jones3D EX enhancements, and new ones) |
| QOL | Quality-of-life changes inherited from OpenJones3D; they are Jones3D EX enhancements and part of the Enhanced profile |
| Recovered name | A function or type name from the original engine, kept in game code |
| Retail | The behavior of the original game as released |
| Retail profile | Every simulation enhancement off: the original game's behavior, for players, speedrunners and the fidelity check |
| RTI | OpenJones3D's tables of retail executable addresses, used for hooking; removed with binary compatibility |
| Sith | The LucasArts engine Jones3D descends from. Also the gameplay runtime in `Libs/sith/` |
| Simulation | Physics, collision, AI, COG, game time and savegames. Measured against retail by traces |
| SMUSH | The game's cutscene video format |
| Trace | The simulation state a headless run writes, frame by frame |
