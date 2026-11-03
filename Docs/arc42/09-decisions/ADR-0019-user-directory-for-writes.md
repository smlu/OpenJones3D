# ADR-0019: Writes go to a user directory; the game keeps its search order

- **Status:** Accepted
- **Date:** 2026-11-03

## Context

Quake 3 and libultraship read through an ordered list of locations and archives, and Quake 3 sends all writes
to a separate home path ([research](../../Research/Engine-Patterns.md)).

Jones3D already reads that way: `JonesFile_OpenFile` searches the install directory, the working directory,
the CD and `Resource`, each first for a loose file and then in its GOB archives. Loose files win over
archives.

Writes don't: `Jones.cfg` and savegames are written relative to the working directory, next to the game data.
Game data directories can be read-only, as in a macOS app bundle or a system-wide Linux install, and several
users can share one install.

## Decision

- The read search order stays in game code (`JonesFile`). Mod folders, when they come (M11 and later), are further entries
  in that list, marked `// Added:`.
- Babylon's Files module provides the per-user writable directory for each platform, through SDL3.
- `Jones.cfg`, savegames, replays and logs are written there.
- Existing files are still found: if no file exists in the user directory, the old location next to the game
  data is read.

## Consequences

- Babylon's Files module stays small: file access, paths and user directories, no archive or mount logic.
- Moving the write location is a user-visible change and goes into `CHANGELOG.md` when it lands.
- Tests and headless runs point the user directory at a temporary directory, so they never touch real saves.
