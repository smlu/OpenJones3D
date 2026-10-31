## sithMulti Original-Engine Test Vectors

`sithMulti.tv` holds, for every case, the complete observable result of the original `sithMulti` functions (the
multiplayer message writers and readers, the player checks and the welcome state machine), captured by executing them
inside the retail `Indy3D.exe` on a synthetic engine state. It is consumed by `TestOriginal*` in
`Libs/sith/Dss/Tests/sithMultiTest.c`. The tests read only this file; they never launch or depend on `Indy3D.exe` or
game assets.

Multiplayer is dormant in the game (there is no UI for it and the DirectX 9 build uses a stub of the comm layer), so
the comm layer and every other engine function the module calls are stubbed and their calls recorded.

### Reference

| Item | Value |
| --- | --- |
| Executable | Retail v1.0 `Indy3D.exe` |
| SHA-256 | `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` (verified before capture) |
| Calling convention | `__cdecl` (`J3DAPI`), prototypes from `Libs/sith/RTI/symbols.h` |
| Module state | The v1.0 addresses of the module variables in `Libs/sith/RTI/addresses.h` (`sithMulti_tickRate` .. `sithMulti_msecSyncTime`) |

| Function | v1.0 address |
| --- | --- |
| `sithMulti_CloseGame` | `0x00454CD0` |
| `sithMulti_CheckPlayers` | `0x00454D30` |
| `sithMulti_ProcessPlayerLost` | `0x00454E70` |
| `sithMulti_RemovePlayer` | `0x00454EE0` |
| `sithMulti_SendWelcome` | `0x00455000` |
| `sithMulti_ProcessWelcome` | `0x004550A0` |
| `sithMulti_ProcessPlayerJoin` | `0x004552A0` |
| `sithMulti_SyncPlayers` | `0x00455350` |
| `sithMulti_ProcessSyncPlayers` | `0x00455490` |
| `sithMulti_ProcessJoinRequest` | `0x00455640` |
| `sithMulti_FinishJoining` | `0x00455A00` |
| `sithMulti_ProcessChat` | `0x00455A40` |
| `sithMulti_ProcessPing` | `0x00455AF0` |
| `sithMulti_ProcessPong` | `0x00455B20` |
| `sithMulti_QuitPlayer` | `0x00455BE0` |
| `sithMulti_ProcessQuit` | `0x00455C20` |
| `sithMulti_Update` | `0x00455CD0` |
| `sithMulti_SyncScores` | `0x00455DF0` |
| `sithMulti_GetPlayerIndexByID` | `0x00455E00` |
| `sithMulti_ProcessKilledPlayer` | `0x00455E30` |
| `sithMulti_QuitGame` | `0x00455FD0` |
| `sithMulti_Respawn` | `0x00456000` |
| `sithMulti_RemoveStaticThing` | `0x004561E0` |
| `sithMulti_UpdateSurfaces` | `0x00456230` |
| `sithMulti_UpdateSectors` | `0x004562E0` |
| `sithMulti_UpdateThings` | `0x004563A0` |
| `sithMulti_UpdateRemovals` (and the welcome finish it calls) | `0x004564A0` |
| `sithMulti_StartWelcomingPlayer` | `0x004565A0` |
| `sithMulti_StopWelcomingPlayer` | `0x00456650` |

`sithMulti_OpenGame`, `sithMulti_Ping`, `sithMulti_GetTickRate` and `sithMulti_SetTickRate` are not in the retail
executable (linker-discarded, known from the debug build) and have no vectors.

### How The Vectors Are Produced

The harness and the tests share the case table and the state builder (`Libs/sith/Dss/Tests/sithMultiVec*.h`), which
reuse the synthetic world, the probe fills, the message stub and the record format of the DSS vectors
(`tv/sithDSS/README.md`).

- Every case builds the world of the DSS vectors (seeded with the hash of the case name) and, on top of it, the 24
  player slots (`sithPlayer_g_aPlayers`: names, flags, net ids, things, sectors, orientations, comm times), the
  player count, the local player and net id, the server id, the message streams, the game and clock time, the submode
  flags and the module state of `sithMulti` (welcome state and progress, removed static things, quit state and time,
  checksum, score sync, ping and sync times, tick rate, the new player callback). Player slots that are not built hold
  the probe garbage.
- The module state is a set of `.data`/`.bss` variables in the executable and file statics in the reconstruction
  (some with other types, e.g. `bool` for `int`). The cases set and compare it through `MsvState`
  (`sithMultiVecState.h`): the harness accesses the v1.0 variables, the tests the statics through accessors that
  `sithMultiTested.c` compiles together with `sithMulti.c`.
- Engine functions that `sithMulti` calls (`sithMultiSeams.def`: the sends, `stdComm`, `sithMessage`, `sithEvent`,
  the DSS writers, strings, console, sound, players, cogs, things, surfaces, sectors, models, collisions and
  `stdPlatform_GetTimeMsec`) and the CRT `rand` of `SITH_RAND()` are stubbed. In the harness the original functions
  (and the static CRT `rand` of the executable at `0x004F7010`) are patched to jump to the stub, in the tests the calls
  of `sithMulti.c` are renamed to the stub (`msv_<name>`). Every call is recorded with its arguments (pointers as stable
  ids), the sends with the complete message, the console prints with the complete text. `sithString_GetString`
  returns its key as the text, so the formats with `%s` work as in the game's string table. Calls between the
  `sithMulti` functions themselves are not stubbed.
- Reader messages are built by the case (with names of every length, unterminated names and strings, edge values) or
  taken from a writer case (`ProcessWelcome_fromSendWelcome`, `ProcessSyncPlayers_from*`, `ProcessQuit_fromQuitPlayer`).
- The record of a case is its return value, the counts of asserts, error, status and warning prints, the recorded
  engine calls and the delta of the watched state: the DSS world and globals, all 24 player slots, the module state
  and the globals listed above.
- Every case is run twice with different garbage (`0xA5` / `0x5A`) in the unused parts of the world, the unbuilt player
  slots and the message buffers; both results must be identical. The tests run every case with both fills as well.
  Two complete capture runs in fresh processes produced byte-identical files.

### File Format

As `tv/sithDSS/README.md` (magic `"DSTV"`, version 1), except that the fourth host service count of a record is the
number of warning prints instead of `stdMemory_Malloc` calls (`sithMulti` allocates nothing).

### Cases

`sithMulti.tv` (303 cases):

- `CloseGame` (6): host and client, welcoming a player or net id 0, all stream bits, net stream already off.
- `CheckPlayers` (18): player timeouts one ms before, at (45000 ms, not kicked) and past the timeout, the local and a
  not joined player timed out, 24 players with three timed out, the score sync interval at and past its end, a client
  losing the server at and past the timeout, no world, no local thing, syncing, no players, comm and sync times that
  wrap, game time 0.
- `ProcessPlayerLost` (8), `RemovePlayer` (9): players, the server (prints that it left), the player being welcomed,
  unknown ids, no players, net id 0, `playerNum == numPlayers` (assert), names of 0, 15 and 63 characters, slot 23.
- `SendWelcome` (10), `ProcessWelcome` (15), `ProcessPlayerJoin` (6): names of 0, 15, 16 (truncated) and 63
  characters, a name without terminator, a shorter name over a longer one, ids with all bits set, slot 23 of 24, a slot
  past `numPlayers` (no range check), the local client entering the game while syncing (and the duplicate when not
  syncing), tick rate 0 (assert), host and client, a new player callback, no local thing.
- `SyncPlayers` (13), `ProcessSyncPlayers` (18): 0, 4 and 24 players, entries without flag 4 (no name), every flag
  set, net id 0, local id 0, model not found (the reader stops), names of every length and with characters above
  0x7F, a 16-byte name without terminator, a join missed and a removal by the server (and both while syncing), a new
  server id, a list that grows from 4 to 6 players.
- `ProcessJoinRequest` (22): every branch: client, wrong level, level name in other case and of 64 characters
  without terminator, already in the game, quitting, busy, welcoming the sender, game full (also with an unplaced free
  slot and no players), player list update and player verification failures, wrong checksum, accepted joins (also in
  slot 23 of 24, with an unterminated or shorter name, with empty level names, checksum `INT_MIN`, two free slots),
  sender id 0.
- `FinishJoining` (13): every join status, an unnamed code, codes `INT_MAX` and -1, progress values 0.25, 0.5, 1, -0,
  `FLT_MAX`, the smallest denormal and the largest negative denormal, ids 0, `INT_MAX`, `INT_MIN`, `UINT_MAX`, a failed
  send.
- `ProcessChat` (15): player number -1 and `INT_MIN` (plain text), lengths 1, 2, 128, 200 and `UINT_MAX` (clamped to
  128), a text without terminator, a terminator inside the length, format characters, names of 0 and 63
  characters, slot 23 of 24, a slot past `numPlayers`.
- `ProcessPing` (2), `ProcessPong` (8): the ping answer (the original sends the global message, not the ping, and
  changes the type of the received message; both recorded), matching and other start times, unknown sender, a negative
  ping time, start time 0, no players, two players with the sender's id.
- `QuitPlayer` (4), `ProcessQuit` (10), `QuitGame` (11): host and client, ids 0 and `UINT_MAX`, a quit from a
  non-server, the local player ejected (and already ejected, quitting, at a game time that wraps the quit time),
  another or an unknown player quitting, server id 0, the quit state and time comparisons at their boundaries.
- `Update` (20): not active, client, ejected/quit/unknown quit state expired and exactly at the quit time, scores to
  sync, welcoming while quitting, welcome steps of 59, 60, 600 and 6000 ms (the whole welcome and past its end, where
  the original keeps stepping with welcome state SURFACES), the left over time, a delta that wraps, welcome states 0
  and 5, the things and removals steps.
- `UpdateSurfaces` (9), `UpdateSectors` (7), `UpdateThings` (9), `UpdateRemovals` (8): from the first, a middle and the
  last element, at and past the end (state switch), nothing or everything to sync, empty lists (`lastThingIdx` -1),
  sectors with the sync and adjoins off flags, weapons and explosions (skipped), guids above 0xFFFF and negative (full
  description), the welcome finish (also while not welcoming, into slot 23, for net id 0, removal 255 of 256).
- `StartWelcomingPlayer` (5), `StopWelcomingPlayer` (6), `SyncScores` (2), `GetPlayerIndexByID` (8),
  `RemoveStaticThing` (6): assert paths (already welcoming, id 0, not welcoming), error values 0, 1, 2, -1, the
  removed static things list at 255, 256 and 300 entries, duplicate and unknown ids, ids past `numPlayers`.
- `ProcessKilledPlayer` (16): no killer, an actor, another player, suicides with `rand()` at both sides of every text
  boundary (0, 8191, 8192, 16383, 16384, 24575, 24576, 32767), a killer without player (assert), two 56 character names
  (127 characters of text), empty and 63 character names, no player thing.
- `Respawn` (19): one slot, all slots masked, two and four slots with collisions (a live player, an actor, a dying and
  a destroyed player, a surface; every pick occupied), `rand()` 0 and 32767 (clamped), 24 slots, a non-player thing
  (assert), and the `_edge_` cases where `rand() * numFound` is a multiple of 32767.

### Known Differences

- `ProcessWelcome_staleName`, `_last24`, `_pastNumPlayers`, `_emptyName`: the original copies the name of a welcome
  with `wcsncpy(awName, pName, 15)`, which writes zeros up to 15 characters; `sithMulti_ProcessWelcome` copies it with
  `STD_WSTRCPY`, which stops at the terminator and leaves the characters of a longer previous name behind it. The
  names are equal, the bytes of `SithPlayer::awName` after the terminator differ.
- `ProcessJoinRequest_levelCase`, `_levelUnterminated`, `_joins`, `_unterminatedName`, `_staleName`, `_last24`,
  `_emptyLevel`, `_checksumIntMin`, `_twoFreeSlots` (every accepted join request): the original prints
  "Starting welcome of player %d." through `pErrorPrint`, `sithMulti_ProcessJoinRequest` through `SITHLOG_STATUS`
  (one error print less, one status print more). `_staleName` also differs in the zero padding of the name as above.
- `Respawn_edge_7slots4681`, `_7slots9362`, `_14slots4681`, `_21slots14043`, `_7slots28086`: the original computes
  `rand() * 3.0518509e-05f * numFound` in x87 extended precision (the float constant is slightly below 1/32767) and
  truncates, so an exact integer product truncates to the integer below; `sithMulti_Respawn` rounds `SITH_RANDF()` to
  float first and gets the integer itself, picking the next respawn slot. Only reachable when `rand() * numFound` is
  a multiple of 32767 (32767 = 7 * 31 * 151, so with 7, 14 or 21 slots).

`TestOriginalProcessWelcome`, `TestOriginalProcessJoinRequest` and `TestOriginalRespawn` expect exactly these cases to
differ and are reported as ignored; any other difference fails them.

### Inputs Kept Out Of The Corpus

- Player numbers outside the 24 slots (`ProcessWelcome`, `ProcessChat`, `RemovePlayer`, `SendWelcome`) and player
  counts above 24 (`ProcessSyncPlayers` reads the count from the message; `SyncPlayers`, `CheckPlayers` and the other
  loops over `numPlayers`) index past `sithPlayer_g_aPlayers` (preserved original bugs). Slots past `numPlayers` but
  inside the array are covered with built slots.
- A chat length of 0: the original copies `len - 1` (`SIZE_MAX`) characters and writes before the text buffer.
- Texts of 128 or more characters in the wide formats (`ProcessKilledPlayer` with two long names): the original's
  `_snwprintf` leaves the buffer unterminated and the print reads the stack. 127 characters are covered.
- Non-ASCII player names in the narrow `%S` prints (`ProcessChat`, `ProcessPong`): the conversion depends on the CRT
  and its locale, not on `sithMulti`. Characters above 0x7F are covered in `SyncPlayers` and `ProcessSyncPlayers`.
- `Update` deltas of billions of steps (each step is one call of the welcome step functions).
- Readers fed messages shorter than what they read: the bytes behind the payload are the probe fill, so such cases
  would only be rejected as unstable.
- No case crashed the original or was unstable between the two fills in the final capture.

### Provenance

- Capture harness: a temporary C file built with MSVC (x86, static CRT, C11) as `Jones3D.dll` and kept outside the
  repository (SHA-256 of the source used:
  `b3a6b8ccf6ba3d564df85f6ea683a6f6b013c390f2e0b8518a6a0721a1371c04`), including the shared headers of
  `Libs/sith/Dss/Tests`. It patches the engine functions of `sithMultiSeams.def` and `rand` to jump to the stubs,
  builds the state of each case, calls the original functions at the addresses above and writes the fixture; no other
  function of the executable is modified. `Jones3D.exe` (this repository's launcher) starts `Indy3D.exe` suspended and
  injects the DLL from its own directory; a helper thread terminates the process after the file is written.
- Regeneration: build the harness with
  `cl /std:c11 /MT /LD capture.c /DDSSV_HARNESS /I<Libs/sith/Dss/Tests> /I<generated j3dcore header dir> /I<Libs> /I<DirectX61c include> /Fe:Jones3D.dll`,
  place it next to `Jones3D.exe` and a copy of `Indy3D.exe` with the SHA-256 above, run `Jones3D.exe Indy3D.exe` and
  collect `sithMulti.tv` from `out\`. Review regenerated fixtures as behavior changes.
- Environment: MSVC 2026 x86 compiler, executed under Wine. No locale, time or floating-point control state is
  involved beyond the default x87 control word of the process.
- The tests redirect the engine globals that `sithMulti.c` reaches through `J3D_DECL_FAR_VAR` to static memory
  (`sithMultiTestAddrs.h` on top of `sithDSSTestAddrs.h`, force-included) and are linked at the fixed high base of
  the DSS tests.
- The reconstructed functions reproduce every case except the ones listed under Known Differences, in Debug and
  Release for both DirectX 6 and DirectX 9 builds.

### SHA-256

```text
df820b5691156f588da90fea3a5f0f356f32ab9808319f235c159a2a57ead565  sithMulti.tv
```
