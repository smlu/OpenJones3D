## v0.4.0
### DirectX 9 Port, Redesigned HUD, Auto Aim Reticle, Enhanced Graphics & Gameplay QOL Improvements
### General:
  - Added **DirectX 9** port:
    * Added Direct3D9 port for `stdDisplay` & `std3D` modules (6290b4b)
    * Added DirectInput8 port for `stdControl` module (a465622)
    * Added DirectSound8 port for `sound` module (9fde6dc)
    * Abstracted `stdComm` (a465622)
  - Refactored & Added new rdVector functions: (5239a55, 68dc8b7, f1ed7a9, b23651f, a74d948, bff6f47, 6aab22c)
    * `rdVector_IsZero2`, `rdVector_IsZero3`, `rdVector_IsZero4`
    * `rdVector_ScaleAdd2Acc`, `rdVector_ScaleAdd3Acc`, `rdVector_ScaleAdd4Acc`
    * `rdVector_MultAcc2`, `rdVector_MultAcc3`, `rdVector_MultAcc4` 
    * `rdVector_Zero2`, `rdVector_Zero3`, `rdVector_Zero4 `
    * `rdMath_ProjectPointOntoPlane`, `rdMath_ProjectPointOntoPlaneNormalized`
    * `rdVector_Equal2`, `rdVector_Equal3`, `rdVector_Equal4`,
    * `rdVector_SmoothDamp2`, `rdVector_SmoothDamp3`, `rdVector_SmoothDamp4`
    * `rdVector_SmoothDamp2Acc`, `rdVector_SmoothDamp3Acc`, `rdVector_SmoothDamp4Acc`
    * `rdVector_IsNAN2`, `rdVector_IsNAN3`, `rdVector_IsNAN4`
  - Added dead code found in debug version (b042bea)
  - Added new macro `RDVECTOR_NEG3` (e78e93b)
  - Fixed names of thing move animation functions in `sithAnimate` module (db77fe8)
  - Renamed `sithCollision_CheckDistance` to `sithCollision_CheckFloorDistance` (b0b6116)
  - Fixed debug assert macro in `rdroid` module (4ec9416)
  - Added functions for copying array of `rdVector*` (58ade59)
  - Added `rdQClip_Face3T` from debug version (7f9159a)
  - Implemented missing block allocation functions of `stdMemory` module (b99b92a)
  - Implemented modules:
    * `sithPlayerActions` (67f2eab)
    * `sithVehicleControls` (63685d3)
    * `sithPlayerControls` (d31ee9b)
    * `sithAIMove` [incomplete] (05c806d)
    * `smush` (INSANE movie player used for the intro movies, including its assembly routines translated to C) ()
  - Moved high poly option to advance display settings (7b58bff)
  - Fixed selecting stored MipMap filter mode in developer dialog (ad1633c)
  - Added new graphic options for MSAA, anisotropic texture filtering and mipmap auto gen to display settings (77df36d)
  - Set trilinear texture filtering and triple buffer option as default options in display settings (77df36d)
  - Fixed releasing of front buffer DC object (84a53aa)
  - Renamed `rdCamera` field `orient` to `viewMatrix` (5382bba)
  - Fixed `rdFace_New` function name (ccc602c)
  - Fixed `pLine->face.aTexVertices` allocation size in `rdPolyline_NewEntry` (ccc602c)
  - Fixed logic error in `stdMemory_Close` where incorrect check for module being open was performed (26430c4)  
    This error prevented check for memory leaks when module was open. 
  - Added new module `stdJSON` for parsing and writing JSON files (575cf36)
    This new feature adds new dependency: `Jansson`
  - Added new module `stdConfig` for storing engine configurations (68d6e6b)
  - Refactored boolean functions and add new functions to store / retrieve binary data in `wuRegistry` module (970dab5)
  - Added new function `Sound_GetChannelPlayProgress` for retrieving current play progress of a sound channel (81efaf9)
  - Added implementation for creating MineCar user block (e1beaca)
  - Added new macros for max puppet modes and calculating puppet major mode from armed & move modes (7d3cc1b)
  - Refactored enum `SithThingMoveStatus` (14236fa)
  - Added implementation for updating physics of attached/detached, underwater, climbing, raft thing (3a6fffa)
  - Added partial implementation for updating physics of MineCar thing (3a6fffa)
  - Added implementation of `sithPhysics_FindFloor` function (3a6fffa)
  - Added github actions CI for building DirectX 9/6 (82d5ba3)
  - Renamed collision system function `sithCollision_FindActivatedThing` to `sithCollision_FindItemThing` (0fe8351)
  - Renamed collision functions for checking LOS (09434a9)
  - Fixed bug to correctly update key press state in `stdControl_ReadKey` function (c98937b)
  - Added check for model render type `sithThing_GetThingMeshIndex` function (431c683)
  - Added `sithPhysics_CalcWheelRotationAngle` function (adacbeb)
  - Added data buffer utility functions to `stdUtils` module (e50873b)
  - Fixed code to use correct physics union member and fields (714cfbc, 4a828da)
  - Refactored and cleaned the `sithFX` module (eecb68f, fa7dbf6)
  - Added functions `sithPlayerControls_CalculateThrust`, `sithPlayerControls_CalculateYawVelocity` & `sithPlayerControls_CalculateAcceleratedThrust` (04b311b, e3de290)
  - Added function checking if cos angle is in slope range; `sithPhysics_CheckSlopeAngle` (d650f3e)
  - Added `J3D_QOL_VALUE` macro (0a0bba7)
  - Added type alias `tStdColor` (9e8b848)
  - Added additional RHW scale macro `RD_FIXEDPOINT_RHW_SCALE_X6` (857ae72)
  - Added interpolation function `stdMath_SmoothDamp` & `stdMath_Expm1f` (e5ea49b)
  - Added PI constant macros `STDMATH_PI` & `STDMATH_PI_F` (d8b0006)
  - Fixed inline declaration for `rdVector` functions (a74d948)
  - Changed definition of 3D vector constants to be defined in the code (a74d948)
  - Added `rdModel3` functions for bending joints with smooth damp interpolation (7f762e3)
  - Added `sithActor` head rotating function with optional delta smooth damp interpolation (6c43467)
  - Added vehicle check function for thing object and physics info (7add165)
  - Refactored `SithActor` & `SithThing` structs and move ai state change fields to `SithActor` (2f71a7e)
  - Renamed `sithPuppet_StopForceMove` to `sithPuppet_FinishForceMove` (88f971a)
  - Added new `sithPuppet` module functions `sithPuppet_SetMoveModeEx` & `sithPuppet_GetArmedMode` (4e964f7)
  - Renamed `SithThing` field `transformedPos` -> `cameraSpacePos` (1786796)
  - Refactored `SithActorInfo` strict by replacing weapon state fields with new `SithActorWeaponInfo` struct (b3656d7)
  - Renamed `sithWeapon_InitializeActor` -> `sithWeapon_InitializeActorWeapon` (b3656d7)
  - Renamed `sithWeapon_UpdateActorWeaponState` -> `sithWeapon_UpdateActorWeapon` (b3656d7)
  - Added documentation for game engine architecture (96d4c57)
  - Added documentation for COG scripting language (96d4c57)
  - Added documentation for engine configuration (96d4c57)
  - Fixed launcher DLL injector to use absolute paths, preserve Windows command-line quoting, load the DLL with `LoadLibraryW`, and clean up process resources on injection failure. (e18b9f5)
  - Fixed JonesFile resource bounds handling by accepting the final file handle slot and ignoring extra GOB files beyond the fixed resource array. (dd3aeeb)
  - Improved GOB cleanup and validation, including failed-load cleanup, shutdown state handling, directory bounds checks, seek/read clamping, and EOF behavior. (245cb3b)
  - Fixed stdConffile, file/path helper edge cases, including short config lines, empty path concatenation, Win32 find handle cleanup, and file-existence handle leaks. (7532d41)
  - Added optional runtime guard macros for release-build safety checks in std modules. (4ebf236)  
    This adds a `JONES3D_RUNTIME_GUARDS` CMake option and marks non-QOL guarded builds as `legacy-hardened`.
  - Added bounded `stdFnames_ChangeExtEx` helper and updated `stdFnames_ChangeExt` to use it while preserving the original ABI. (4ebf236)
  - Fixed std print/file formatting helpers to reject invalid inputs (4ebf236)  
    This avoid path format-string misuse, and avoid writing negative or oversized formatted byte counts after truncation.
  - Fixed BMP load/save validation in `stdBmp` (4ebf236)  
    This includes row padding, image-size bounds, header-size checks, `bfOffBits` seeking, VBuffer lock handling, and partial-load cleanup.
  - Fixed `stdConffile` error paths and include handling (4ebf236)  
    This includes stack overflow reporting, failed-open stack restore, line-buffer allocation cleanup, and formatted write truncation warnings.
  - Fixed GOB resource loading and file-handle validation in `stdGob` (4ebf236,5c1935e)
    This includes unsupported mmap fallback, directory bounds checks, failed-load cleanup, invalid seek/read rejection, and handle state cleanup.
    Directory and file-handle state checks use `JONES3D_RUNTIME_GUARDS`.
  - Fixed missing allocation-failure and size-overflow checks across std modules. (4ebf236)  
    This adds validation for buffer sizes, element-count multiplication, file/image sizes,  
    handle arrays, hash-table storage, string conversion buffers, conffile line buffers,  
    circular buffers, and GOB directory data, while cleaning up partially-created resources  
    on failure.
  - Fixed `stdMath` edge cases for power, angle normalization, trigonometry, and arcsine helpers. (4ebf236)  
    This fixes zero and negative exponent handling in `stdMath_FlexPower`, rejects NaN/infinity before lookup-table indexing in angle/trig functions, preserves explicit NaN handling, and clamps arcsine inputs to avoid domain errors from small floating-point overshoots.
  - Fixed `stdPrintf` to preserve safely truncated messages and pass formatted output as data, preventing percent characters in file paths or messages from being reinterpreted as format directives. (f5d6320)
  - Fixed `stdFnames_ChangeExtEx` to validate path termination before scanning for an extension, preventing reads beyond unterminated fixed-size buffers. (f5d6320)
  - Fixed `stdFileUtil_FindNext` to count only successful search results, allowing a failed initial search to be retried safely. (f5d6320)
  - Fixed `stdJSON_Startup` to return failure when host services are unavailable without attempting to log through a null host-services pointer. (f5d6320)
  - Added an optional runtime guard for missing file-search result storage and made path concatenation and hash-table state checks follow the runtime-guard setting. (f5d6320)
  - Fixed `stdLinkList_AddNode` to update the successor node's `prev` pointer after insertion, preventing the inserted node's `prev` pointer from pointing to itself. (29e97f2)
  - Fixed string-table validation, EOF handling, and partial-load cleanup in `stdStrTable`. (1b7cb2d)
    The loader validates message counts, decimal fields, quoted values, duplicate keys, and the trailing `END` marker, and releases partially loaded data after parsing or allocation failures.
  - Added implementations of `AudioLib_GenerateLipSyncBlock` and `AudioLib_GetMouthPosition`, preserving the original lip-sync timeline and cross-sample decoding behavior. (0facac9)
  - Added implementations of `AudioLib_ParseWaveFileHeader`, `AudioLib_Compress`, `AudioLib_Uncompress`, `AudioLib_ResetCompressor`, `AudioLib_CompressBlock`, `AudioLib_UncompressBlock`, and `AudioLib_WVSMUncompressBlock`, completing `AudioLib`; the codec tables are now owned by the DLL, and the unit tests compare against vectors captured from the original executable.
  - Added implementations of the remaining `sithAIMove` functions (turning, movement, cliff/water checks, head tracking and the Quetz and Marduk tail animations), completing `sithAIMove`.
  - Added implementation of `sithThing_UpdateQuetzUserBlock`, preserving the original Quetz tail follow behavior.
  - Added implementations of `sithPhysics_ProcessMineCarTrackMove`, `sithPhysics_ProcessTrackFace`, `sithPhysics_UpdateJeepPhysics`, `sithPhysics_CreateJeepUserBlock`, `sithPhysics_UpdateJeepFx` and the track and jeep wheel helpers, completing `sithPhysics`; the jeep physics constants are now owned by the DLL.
  - Added capacity-aware lip-sync generation helpers to calculate worst-case output sizes and validate input/output buffer capacities. (0facac9)
  - Added implementations of `sithThing_WriteThingsListBinary` and `sithThing_ReadThingsListBinary` for world things and templates, preserving the original binary record layout and block ordering. (92660f2)
  - Fixed MSVC incremental-link PDB collisions by giving the executable a separate debug-symbol database from the DLL. (db582ba)
  - Added Unity 2.7.0 test framework dependency. (3b79a5e)
  - Added j3dcore macro tests and shared test support. (eb8bba0)
  - Added std unit tests and DirectX 6/9 system tests, including external BMP vectors and optional fullscreen tests. (db582ba)
  - Fixed constant wide-string array detection and DX6 DirectPlay session-descriptor allocation failure handling and cleanup. (db582ba)
  - Changed the JONES3D_RUNTIME_GUARDS CMake option to be disabled by default. (db582ba)
  - Added AudioLib and Sound lip-sync unit tests covering generation, lookup, caching, playback offsets, and allocation failures. (19d570c)
  - Added sithThing, sithTemplate, and sithVoice unit tests, including retail-generated binary vectors and allocation/I/O failure coverage. (aa4e510)
  - Added GitHub Actions unit-test workflows for DirectX 6/9 and updated the existing CI builds to use Visual Studio 2026 with four build profiles per backend. (2995463)

### Engine:
  - Added check for zero size in lip sync data generation to prevent allocation errors (f79736b)
  - Added functionality to write user-mode minidump information when an unhandled exception occurs (6fc4ad5)
  - Added stack trace printing to the log when an assert error occurs (6fc4ad5)
  - Renamed developer console commands (357c287, c00c681)  
    * `interface` -> `menu`
    * `indicator` -> `hud`
  - Fixed returning correct variable in `stdControl_IsOpen` (ae3146d)
  - Refactored engine settings to use `stdConfig` module instead of `wuRegistry` (68d6e6b)
  - Fixed out of bounds write when ran out of free subtitle slots in `sithVoice_AddSubtitle` (693cbcf)
    It resolves crashes when too many subtitles are added in a short time.
  - Fixed out of bounds read in `sithVoice_Draw` (693cbcf)
  - Fixed skipping expired/empty slots and clearing correct part of pending subtitle array in `sithVoice_PurgeDrawnSubtitles` (693cbcf)  
    This resolves issues with drawing pending subtitles when `sithVoice_curSubtitleDrawIndex` < `sithVoice_numSubtitleInfos`
  - Fixed removing subtitle when stopping currently played thing voice sound in `sithVoice_PlayVoice` (4d5f54a)
  - [QOL] Changed physics fixed timestep to 150 fps and added option to configure timestep (0eaa4eb)
  - Fixed incorrect handling of default control set (4b3ea73)
  - Fixed assigning default control set when loading control set from `kfg` file fails (d6c774c)
  - Fixed not to sync MineCar engine animation to savegame file (a6c5978)  
    This fixes dangling MineCar engine material animations when loading savegames.
  - [QOL] Replaced `timeGetTime` with high resolution timer `QueryPerformanceCounter` for better timing precision (82c8a3d)  
    This should improve timing precision for time dependent systems, especially on high refresh rate monitors.
  - [QOL] Added config option to set extra capacity for static world sprites buffer (834cb1b)
  - [QOL] Added config option to set extra capacity for world things buffer (1e4245d)
  - Added reset of `sithPlayerControls` internal state upon open of game (4a1721e)
  - Fixed collision physics calculations between 2 things where incorrect absolute value was applied over total impact speed made it always positive value (41327cb)
  - Fixed incorrect absolution of total traveled distance in `sithCollision_MoveThing` (41327cb)
  - Refactored cog module to use size `sizeCogs` value rather than value of`numCogs` when allocating world cog array (81ffc09)
  - Reverted commit `2a757af` which introduced QOL for removing loop play flag from sound classes upon load (e73c4fe)  
    Apparently this flag is needed in many cases (e.g.: path moving thing).  
    This change reverts sound class play logic back to original,  
    and the buggy sound class files can cause sound buffer to fill up. (e.g.: in aetherium level there are some creatures with loop flag for move soundfx)
  - [QOL] Increased the sound buffer for simultaneously played sounds to 512 (e73c4fe)
  - Added new exponential and logarithmic modes for sound falloff by distance to software mixer, complementing the existing linear mode (0c41b49)  
    These new modes provide more realistic sound attenuation options:
    * **Exponential**: Sharper drop-off for rapid volume reduction over distance.
    * **Logarithmic**: Smoother transitions for gradual volume reduction.  

    The sound falloff mode can now be configured in engine config file.

  - Added extra check for null parent pointer on polyline update (83d50f1)
  - Refactored the minecart turned on/off state effects and unified them under the `sithPhysics` module (f70d670, b6f081a)  
    This change consolidates the logic previously spread across `sithVehicleControls` and `sithPlayerControls`.
  - Fixed orbital camera focus thing when player enters vehicle (c0becd1)
  - Implemented animation playback speed control, allowing animations to be sped up or slowed down (8e42cfe)
  - Improved AI actor pathfinding behavior when reaching a cliff (41a4b6e)
  - Implemented playback speed control for puppet mode animations (0bdc507)
  - Fixed notifying underwater thing that goal is unreachable when it hits floor surface (e3eb556)  
    This fix prevents potential underwater thing to get stuck at the floor surface indefinitely while trying to reach its goal position.  
    e.g.: shark in lagoon level might get stuck in sand floor surface.
  - Fixed COG script message type in COG execution error log (5d64205)
  - Added new water surface & position search collision function `sithCollision_FindWaterSurface` (c6b55b9)
  - Fixed potential division by zero in `rdMatrix_BuildFromVectorAngle34` when input vector's `z` component is not correctly normalized (88700c1).  
    Previously, incorrectly normalized `z` values could result in division by zero, producing NaN values.  
    This resolves numerous issues where due to float type precision error the `z` value was close to +-1.0f.  
    Most notably this fixes sprite rendering issues where rotated sprite vertices could result in NaN values,  
    causing sprites to fail to render visually.
  - Fixed puppet active animations are no longer removed when performing a 2m climb move in normal move mode. (1532c06)  
    Resolves issue where active puppet animations (e.g., weapon armed) were removed after a 2m climb animation in normal move mode.
  - Updated camera switch control function to `SITHCONTROL_CAMERAMODE` (b5da9a3)
  - Increased renderer thing collection distance to 160m (cd3ef85)  
    This change increases the maximum distance at which objects are collected for rendering from 80m to 160m,  
    which fixes rendering issues where distant objects were incorrectly culled.  
    e.g.: sun sprite in Meroe level.
  - Added separate light collection distance (cd3ef85)  
    This change increases the maximum distance at which dynamic lights are collected for rendering.  
    Substantially resolves issues where distant lights would flicker at different camera angles due to being culled prematurely.
  - Implemented new BFS algorithm for culled-sector traversal for thing and light collection (efc861e)
    Introduced an optional BFS-based algorithm for building visible thing sector list and collecting dynamic lights.  
    The BFS traversal improves upon the legacy order-dependent DFS-like approach and  
    enables multi-path propagation through already visited sectors (previously limited  
    to a single traversal path).  

    When enabled, this prevents missed lights and objects, fixes traversal-order-dependent  
    artifacts such as dynamic light flickering (e.g. in the Babylon level), and ensures  
    consistent thing/light collection up to the configured maximum distance.  

  - Fixed normalizing camera orient matrix in `sithCamera_Update` (a96f610)
  - Fixed potential game crash when weapon fire position has NAN value (3831043)
  - Fix missing dvec reset in thing orient matrix (a8e0432)  
    Clears the orient matrix translation component after inversion to ensure it  
    remains rotation-only. Prevents translation state leakage that could cause incorrect world-space  
    transforms. Fixes rare but persistent transform errors that could propagate  
    across frames, especially for non-rendered things.
  - Added optional external material loading during CND parsing (c012b07, f8c3e94)  
    Adds support to load materials from external files or GOB resources first  
    while parsing the materials section in CND files. This enables modding  
    without modifying original level files and preserves the original CND  
    material behavior when external assets are unavailable.
  - Added optional external keyframes loading during CND parsing (c012b07)  
    Adds support to load keyframes from external files or GOB resources first  
    while parsing the keyframes section in CND files. This enables modding  
    without modifying original level files and preserves the original CND  
    keyframe behavior when external assets are unavailable.
  - Fixed stale sprite/actor mesh attachments by validating the attached thing type/render type  
    before using its model matrix and clearing invalid attach info. (a30aa30)
  - Fixed DSS savegame block validation in `sithGamesave` to reject oversized blocks before reading into the fixed message buffer (8cf0d99)
  - Fixed `sithGamesave` save/restore error paths to close files, restore output stream state, and run normal cleanup after failures (8cf0d99)
  - Fixed `ThingFullDescription` restore in `sithDSSThing` to validate serialized thing types, template indices, path frame counts, swap entry counts, model references, and polyline data before use (6fa2af9)
  - Fixed puppet DSS restore in `sithDSS` to validate serialized arm modes, move modes, track counts, major modes, and submodes before indexing puppet mode tables (6fa2af9)
  - Hardened NDY sector and surface loading by validating serialized sector vertices, surface ranges, adjoins, face vertices, and texture vertices before pointer conversion.  (73819a9)
  - Initialized Sith intersection hit type defensively before collision result handling. (b4cc752)
  - Fixed fixed-size path extension handling in `jonesConfig`, `JonesMain`, `sithMain`, and `sithModel`. (9a9deab)  
    Savegame, static-level, level lookup, autosave, and hi-poly model paths now use bounded extension replacement when adding `.nds`, `.cnd`, `.ndy`, and `.3do` extensions.
  - Fixed `JonesFile_FileSeek` return value for GOB-backed files. (ec214c1)  
    `stdGob_FileSeek` returns `1` on success, but the Jones file host service expects `0` on success.
  - Fixed DSS puppet status restore to bounds-check puppet submodes before indexing the puppet mode table. (3338212)  
    This prevents malformed or invalid puppet status messages from reading outside `pPuppetClass->aModes`.
  - Fixed joystick button and POV classification macros in `stdControl` to respect the interleaved control-ID ranges of each joystick. (a8be92d)
  - Fixed DX6 input timing to avoid division by zero when consecutive reads occur within the same millisecond tick. (a8be92d)
  - Fixed DX6 axis and key validation by stripping axis flags before table lookup, rejecting malformed key events and invalid joystick indices, and adding optional runtime guards for key reads and axis registration. (a8be92d)
  - Fixed DX6 mouse Y-axis registration to use its own configured range instead of the X-axis range. (a8be92d)
  - Fixed DX6 control initialization and polling error paths to release DirectInput after failed device enumeration, skip uninitialized input devices, and continue polling other joysticks after a device error. (a8be92d)
  - Fixed DX6 mouse buffer-overflow recovery to use immediate button data only after a successful state read, and preserved separate read and reacquisition errors in diagnostics. (a8be92d)
  - Added optional runtime validation of actor/player thing types before updating voice lip sync. (4e4b5aa)
  - Fixed lip-sync output buffer overflow in `Sound_GenerateLipSync` by replacing the fixed 8192-byte stack buffer with a checked worst-case heap allocation. Highly variable audio could exceed the old buffer after about 34.1 seconds at 60 Hz; repeated mouth states could allow longer audio. (e472b74)
  - Fixed compressed lip-sync generation to reserve and initialize PCM padding for cross-sample decoding and odd-length analysis frames, preventing reads past the temporary decompression buffer. (e472b74)
  - Fixed lip-sync allocation-failure handling and cleanup, publishing cached data only after successful generation and returning neutral mouth coordinates on failure. (e472b74)
  - Fixed overflow-prone playback-time and end-of-audio calculations in `Sound_GenerateLipSync` while preserving the original 25-millisecond end margin. (e472b74)
  - Fixed DX6 DirectPlay host-session creation to mark successful sessions active instead of setting the host flag twice. (f657a7c)
  - Fixed uninitialized DirectPlay recipient and player IDs being used after failed receive or player-creation calls. (f657a7c)
  - Corrected DX6 DirectPlay receive-length handling to use a `DWORD` API value while preserving the `size_t` caller interface. (f657a7c)
  - [QOL] Increased the maximum thing capacity from 2,304 to 16,384 while retaining the original 2,304 limit when QOL improvements are disabled. (92660f2)
  - Removed registration of the additional COG functions `DebugPrint`, `DebugFlex`, `DebugInt`, `DebugVector`, `DebugLocalSymbols`, `DebugWaitForKey`, `EnablePrint`, and `IsPrintEnabled`, introduced in v0.3, to preserve retail savegame compatibility. (3f2fdc7)
    This preserves the script-local symbol counts and ordering used by positional COG state restoration.
  - Fixed `sithSector_ReadSectorsListText` and `sithSector_ReadSectorsListBinary` to clear the `NOGRAVITY` sector flag from level data as the original does; sectors marked in level files no longer become gravity free. ()
  - Fixed `sithDSSThing_ProcessThingFullDescription` reading the path movement `mode`, `numBlockedMoves`, `nextFrame` and `goalFrame` zero extended instead of sign extended; a stored `-1` is now restored as `-1` as in the original instead of `65535`. ()
  - Fixed `sithDSS_ProcessPuppetStatus` reading puppet track low and high priorities sign extended instead of zero extended as the original does. ()
  - Fixed `sithMaterial_ReadMaterialsListText` returning success when the `world materials` line is missing or malformed; it now reports an error as the original does. ()
  - Fixed `sithCogExec_PopInt` converting float values by truncating toward zero instead of flooring them, so negative non-integer COG arguments (e.g. -1.5) became -1 instead of -2. ()
  - Fixed `sithCogParse_GetNextNode` growing the syntax tree by only 8 bytes per node instead of the node size, which corrupted the heap for scripts with more than about 8000 nodes. ()
  - Fixed multiplayer messages being sent and received with a 2 byte header instead of the original 4 bytes (`sithMessage_NetWrite`, `sithMessage_ReceiveMessage`), which dropped the last 2 bytes and truncated the largest messages (more than 3070 bytes of payload). **Network protocol change:** this build no longer exchanges messages correctly with earlier OpenJones3D builds, only with the original game or builds including this fix. ()
  - Fixed the `wkernel` application GUID (`wkernel_guid`) third field to `0x11D1` as in the original, was `0x01D1`. **Network:** DirectPlay sessions created by OpenJones3D now use the original application GUID, so they are visible to and joinable by the original game, and sessions of earlier OpenJones3D builds are no longer compatible. ()
  - Fixed `Sound_Play` scanning past the end of the channel array when all sound channels are busy; a lower priority channel is now reused as in the original, or the sound is not played. ()
  - Fixed `Sound_Load` dropping the first character of the file name when the path has no directory separator (the sound was not found in the sound bank and was loaded again). ()
  - Fixed `Sound_GetSoundIndex` matching the invalid sound handle with unused sound infos; it now returns the invalid index as the original does. ()
  - Fixed `Sound_FadeVolume` and `Sound_FadePitch` comparing the remaining fade time as unsigned (`SoundFade` start and end times are now signed as in the original), which reused expired fade slots incorrectly and produced a huge fade fraction when the sound clock was behind a fade start. ()
  - Fixed `SoundDriver_LogError` passing a `va_list` as a variadic argument to the host log function (garbage output or crash when DirectSound errors are logged); the message is now formatted into a buffer first. ()
  - Fixed `stdControl_ReadMouse` smoothing (frame times under 25 msec) averaging the mouse movement with the previous read by division, which truncated toward zero; it now floors like the original, so small left/up mouse movements are no longer lost (DirectX 6 and DirectX 9). ()
  - Fixed `stdControl_ReadAxisAsKeyEx` overwriting the num pressed state with 1 for axes bound to key functions, which dropped the presses counted by other bindings of the function. The count of axis key presses is now a QOL feature (legacy builds do not update the state as the original); it adds 1 to the state. ()
  - Fixed `stdMath_Tan` for angles from 180 to 270 degrees (quadrant 2): the base table value was looked up in reverse, so the tangent started near the table asymptote instead of 0. ()
  - Fixed two entries of the tangent table of `stdMath` (indexes 4072 and 4075) which were one ulp off the original values. ()
  - Fixed `stdControl_RegisterAxis` rounding the axis deadzone threshold instead of truncating it as the original does (DirectX 6 and DirectX 9). ()
  - Fixed `SITH_RAND` scaling `rand()` by the double constant `1.0 / RAND_MAX` instead of the float constant of the original, which changed the last bits of the random values and returned exactly 1.0 for `rand() == RAND_MAX` where the original stays below 1.0 at double/extended precision. ()
  - Fixed `sithGamesave_SaveFile` writing "Oct 28 1999" into the save game header date; the original writes its build date "Oct 29 1999". ()
  - Fixed `sithIntersect_CheckSphereHit` accepting a sphere moving away from the plane when the sphere is touching it and zeroing the hit distance before adding the radius for a sphere inside the plane distance with the collision flag 0x400. ()
  - Fixed `sithIntersect_CheckSphereFaceIntersectionEx` passing the tested position also as the output hit position to the face vertices test, which changed the position tested against later edges and vertices and the resulting hit normal. ()
  - Fixed `sithPathMove_ParseArg` treating a negative `numframes` argument of a thing path as a huge frame count (failing with a memory allocation error) instead of rejecting it as a bad argument. ()
  - Fixed `sithPathMove_UpdateRotate` clamping a NaN rotation delta of a rotating path movement to 1.0 instead of leaving it like the original. ()
  - Fixed `sithEvent_Process` advancing the event queue only after calling the task of the processed event; a COG timer handler killing its own timer (`KillTimerEx`) freed the processed event twice and lost the rest of the queue. ()
  - Fixed `sithCogParse_Load` rejecting a COG script whose `flags` value cannot be converted (reported as "Unexpected EOF"); the original ignores the conversion result and loads the script with zero flags. ()
  - Fixed `sithMessage_SendMessage` not writing a message to the file stream when the stream mask includes both the network and the file stream (not reached by the stock code). ()
  - Fixed `AudioLib_WVSMCompressBlock` writing past the end of its sample bit length histograms for the sample -32768 and, with the earlier bounds fix, counting the bit lengths from 0 instead of 1, which selected different shifts than the original and produced different (larger) compressed WVSM data; it now matches the original output. ()
  - Fixed `stdMath_ClipNearZero` returning NaN unchanged; it now returns 0 for NaN like the original. ()
  - Fixed `STDMATH_CLAMP` passing NaN through; it now turns NaN into the lower bound like the original. ()
  - Fixed `rdMath_ClipVector3` and `rdMath_ClipVector3Acc` keeping vector components that are exactly equal to the minimum value; a component is now kept only if its magnitude is greater than the minimum, like the original near zero clipping of the physics velocities. ()
  - Fixed `SoundDriver_Update3DSound` playing 3D sounds at full volume while there is no camera (the sound mix callback leaves the volume untouched); such sounds are now culled by the far volume check like in the original (DirectX 6 and DirectX 9). ()

### Graphics:
  - Fixed an issue where active textures used in the current render frame were being removed from the cache prematurely in low VRAM situations (f37ecb7)
    Fixes issue [#28](https://github.com/smlu/OpenJones3D/issues/28)
  - Fixed missing alpha lerp & fixed minor bugs in `rdClip_Face3T` clip function (a8b6907)
  - Renamed function `rdClip_QFaceW` to `rdClip_QClipFaceW` (7c87a38)
  - Added new clipping function `rdClip_QClipFaceT` from debug version (7c87a38)
  - Fixed vertex intensities and minor bugs in `rdPrimit3_ClipFace` clip function (b87f3b9)
  - Implemented basic HLSL shader system with support for VBO & IBO (fb26728)
  - Fixed selecting first HAL display device in developer dialog (6a1551e)
  - Renamed `rdCamera.orient` field to `rdCamera.viewMatrix` (5382bba)
  - Fixed resetting cached mipmap filter on std3D init/close (7eee500)  
    This resolve issue with mipmap filter not being set when `std3D` is reopened (changed display settings).
  - Changed default mipmap filter to trilinear (b2a322e)
  - Added support for anisotropic texture filtering (737d83c)
  - Added support for MSAA anti-aliasing (9a1bc12)
  - Added auto generation of mipmap texture chain (b7e7273)
  - Fixed normalizing calculated right & up dir vectors for type 2 sprite in `rdSprite_Draw` (120a6f2)
    This corrects the size of rendered type sprites, especially when sprite is on uneven surface.
    e.g.: raft row sprite on uneven water surface.
  - [QOL] Wrapped all improvements to DirectX 6 implementation of `std3D` & `stdDisplay` in QOL macro (ba404fe)  
    This allows compiling these 2 modules to the original state when QOL is disabled.  
    i.e.: 16 bpp textures, not VSync disable option, no anisotropic texture filtering and default bilinear filtering.
  - Fixed selecting display device when setting default video mode (dcd8962)  
    This was vanilla engine bug where no display device was selected resulting in game reporting no 3D cards found.
  - Fixed water ripple position to be placed slightly above water surface in `sithFX` module (fa7dbf6)  
    Originally there was a bug where water ripple was placed slightly below water surface causing ripple to be clipped.
  - [QOL] Enhanced raft wake fx (929c21d, 26f277f)
  - Increased the max collection distance of things to be rendered from 8.0 (80m) to 16.0 (160m) (0bd7b35)  
    Fixes rendering glitches where objects were prematurely culled while still visible in camera view.
    e.g.: sun sprite in Meroe level.
  - Fixed weapon fire flash FX by correcting the light range and added config options (5453817)
  - Added new texture-tiling render option to `rdPolyline` module (c947bd9)
  - Fixed texture tiling on rope and metal-wire polylines by hardcoding textures that need UV tiling (12419d0, abf4b32)
    This fixes rope polyline texture issue in river level and issue with texture tiling for elevator metal wire in volcano level
  - Added new texture-rotate render option to `rdPolyline` module (281620c)
  - Added new red rear light attached to minecar when car is turned on (f70d670)
  - Fixed the minecart rear light polygon effect to appear turned off when the cart is not running (f70d670, 26ef2e8)
  - Fixed the minecart front light polygon effect to appear turned off when the cart is not running (f70d670, 26ef2e8)
  - Fixed issue with dynamic light of static minecar (797251c, 26ef2e8)
  - [QOL] Implemented turn movement animation playback for water surface movement (ba57877)
  - Fixed water ripple creation on the water surface (18e09d5)  
    The engine previously offset ripple positions at a fixed distance from the thing position,  
    which could cause ripples to appear incorrectly above or below the water surface.  
    This was particularly noticeable during actions like climbing out of water.  
    Ripples now spawn precisely at the water surface level.
  - Added new text rendering functions that support font size scaling (0c97fa8)
  - Refactored and Redesigned HUD and inventory menu UI (96d430b, efc861e, ea0d1ad, f0dad58)  
    * Adjusted HUD indicator scaling for improved visibility.
    * Repositioned and resized the endurance indicator.
    * Redesigned the inventory menu:
      - Scaled down item elements for better clarity.
      - Increased column size for improved organization.
      - Smoothed item movement animations using smooth damp interpolation.
      - Added ambient theme music that plays while the inventory menu is open.
    * Exposed HUD and menu constants to a config file for customization.
  - Fixed actor headlight position (b647d11, a589b92)  
    The headlight offset was incorrectly transformed to world space by applying `actor.orient.dvec` in the calculation.  
    This caused the headlight to be mispositioned when `dvec` was not zero and could cause light flickering issues.
  - Fixed actor head light range (b647d11)  
    The actor light min/max range were incorrectly assigned to the head light min/max range fields  
    in the process of culled sector collection. Now the head light range is correctly set to the alpha field of it's color.
  - Fixed actor head light not to lit when actor has invisible flag set (a589b92)  
    This prevents actor head light from being rendered when the actor is invisible.
  - Added new HD HUD indicator textures [PR #38](https://github.com/smlu/OpenJones3D/pull/38)
  - Hardened KEY, MAT, and 3DO loading with serialized index checks, texture/mip validation, corrected allocation sizes, and safer face token parsing. (872c3c1, b6132e4, 229f4c2)
  - Fixed clipped Gouraud face intensity interpolation (OpenJones3D bug) (ed059c7)
  - Fixed MAT loader error cleanup so failed cel reads, mip validation, or color conversion release partially created mip buffers correctly. (e3a9c15)
  - Fixed advanced display settings rasterizer selection to read from the rasterizer combo box instead of the 3D device combo box. (8235ece)
  - Fixed false graphics restart prompts when confirming advanced display settings without changing the performance level. (8235ece)
  - Fixed low-bit RGB and alpha expansion in `stdColor`, including 1-, 2-, and 3-bit channels, by repeating source bits without negative shift counts while preserving existing RGB565 upscale results. (f5d6320)
  - Fixed `stdColor_CalcColorBits` to return zeroed fields for empty masks instead of looping indefinitely. (f5d6320)
  - Added a debug assertion and optional runtime guard for BMP export of software VBuffers without pixel memory. (f5d6320)
  - Fixed DX6 graphics startup and failed-open cleanup to release partially initialized Direct3D and DirectDraw resources, and stopped treating COM reference counts returned by `Release` as HRESULTs. (4faef7c)
  - Fixed DX6 display device bounds and mode lifecycle handling, including fullscreen failure cleanup, desktop restoration before leaving exclusive mode, and correct windowed-mode failure reporting. (4faef7c)
  - Fixed DX6 VBuffer allocation cleanup, Z-buffer replacement and attachment cleanup, and back-buffer and clipper reference leaks. (4faef7c)
  - Corrected DX6 software pixel fills to repeat complete 16-bit and 32-bit pixel values, matching the original DWORD-fill semantics instead of repeating a single byte. (4faef7c)
  - Fixed nested DX6 hardware VBuffer locks to hold one DirectDraw surface lock until the final unlock and clear the transient pixel pointer afterward. (4faef7c)
  - Fixed DX6 back-buffer device-context retrieval to return the context only when DirectDraw reports success. (4faef7c)
  - Fixed DX6 texture uploads to respect source row sizes and signed surface pitch, resize matching source and destination mip levels, and release locks and COM references on failure. (4faef7c)
  - Fixed DX6 untextured draws to explicitly unbind the previous texture and keep the cached texture pointer consistent with device state. (4faef7c)
  - Fixed DX6 texture-filter selection to check magnification and minification capabilities independently. (4faef7c)
  - Fixed DX6 depth-format enumeration to accept compatible combined pixel-format flags and texture-format enumeration to bound the count by array entries rather than bytes. (4faef7c)
  - Added read-only DX6 back-buffer access for readback without requesting write-only DirectDraw locks. (4faef7c)
  - [QOL] Improved DX6 RGB565 component expansion using bit replication while preserving the original conversion when QOL is disabled. (4faef7c)
  - [QOL] Updated DX6 anisotropic filtering detection and reporting to require matching filter capabilities and a usable anisotropy level. (4faef7c)
  - Fixed DX9 fullscreen mode setup overwriting advertised video modes when the runtime promotes the created surface format, preserving the available modes for later selections. (ec573c8)
  - Fixed `sithParticle_Update` timeout-rate particles growing by at most one vertex per update (used min instead of max with 1). ()
  - Fixed `rdMatrix_BuildFromLook34` using swapped cross product operands when the look vector is parallel to the z axis, which rotated the resulting matrix 180 degrees about the look axis (things or projectiles pointing straight up or down). ()
  - Fixed `rdLight_CalcFaceIntensity` writing the distance attenuation back into its parameter, which attenuated every following light by all earlier ones; each light is now attenuated on its own. ()
  - Fixed `rdParticle_Draw` overwriting the per-thing RGBA material flag with the per-particle distance test, so after one particle beyond the alpha distance all remaining particles of the thing were drawn opaque. ()
  - Fixed `rdClip_Face3GTOrtho` leaving the previous UV pointer unset for one clip plane, which gave wrong texture coordinates for polygons clipped by an orthographic camera. ()
  - Fixed `rdClip_Face3TOrtho` incrementing the clip index twice for each vertex kept by the far plane, which corrupted the clipped polygon for orthographic cameras. ()
  - Fixed `rdCache_SendFaceListToHardware` writing the clamped texture cel back into the batch cel number, which split the draw calls of faces using the current material cel (-1) into one batch per face, and clamped the cel with unsigned comparisons. ()
  - Fixed `std3D_Open` (DirectX 6) leaving the color key of the color key texture formats NULL; they now use the original color key {0, 0}, which avoids a NULL dereference in `stdColor_ColorConvertOneRow` and sets the same source color key on texture surfaces as the original. ()
  - Fixed `sithRender_CollectVisibleThingSector` registering sectors without things in the visible thing sector list (legacy collection mode), which wasted list slots and counted them in the visible thing sector statistic. ()
  - Fixed `std3D_DrawRenderList` truncating the table-less vertex fog alpha instead of rounding it to nearest like the original, which made the fog alpha 1/255 lower for about half of the values (DirectX 6 and DirectX 9). ()
  - Fixed `rdModel3_SwapMesh` not replacing the mesh of the second model when replacing the mesh of the first model fails. ()
  - Fixed `rdPolyline_NewEntry` allocating the texture vertices and UVs only for the exact full geometry mode instead of for full or higher geometry modes. ()
  - Fixed `rdParticle_LoadEntry` rejecting particle files with a negative material cel number (-1, the material's current cel) as a syntax error because the cel number was compared with the cel count as unsigned. ()
  - Fixed `rdFont_DrawTextLine` and `rdFont_DrawTextLineClipped` (and their `Ex` variants) centering or right aligning text at a fractional pixel instead of a whole pixel, and centering the text when both the center and right alignment flags are set (right alignment wins like in the original). ()
  - Fixed `rdFont_GetTextWidth`, `rdFont_GetWrapLine`, `rdFont_DrawTextLine` and `rdFont_DrawTextLineClipped` dereferencing a NULL font to read the default font size; they now return 0, NULL or draw nothing like the original. ()

### Game play:
  - Fixed bug in `sithPlayer_Update` where force move animation could be stopped when required distance to move was almost zero (127aa92)
  - [QOL] Enhanced whip-swing movement to stop animation early and make Indy fall when landing on non-solid surfaces (i.e.: mid air) (8a2548c, b4100ab)
  - [QOL] Enhanced smooth transition of idle camera to external camera (572aaec)
  - Fixed ledge detection (32acc4c)  
    This fixes detection of ledges positioned 0.2 units (2 meters) from the ground.  
    Fixes issue [#20](https://github.com/smlu/OpenJones3D/issues/20)
  - [DX9] Implemented XInput API for game pad controllers (f4a6e61)
  - Added `speedrun` build option to enable vanilla engine quirks and bugs that can be leveraged in speedrun gameplay [PR #23](https://github.com/smlu/OpenJones3D/pull/23)
  - Fixed minor MineCar bug when player is trying to unborad left or right side of car and unboarding is blocked (307691c)
  - [QOL] Fixed dying thing to not change it's puppet mode to swim when entering water (85abbe6)  
    This prevents dead things from playing swim idle animation when dead thing falls into water.
  - [QOL] Enabled jeep exhaust smoke fx (c889ecb)
  - Fixed Infinite loop bug in `sithCogFunction_CopyPlayerHolsters` function (51180ef)
  - Fixed end position in strafe move collision check and reduced the collision search radius (20f2573)  
    This fixes Indy to be able to strafe move when facing tilted surfaces, e.g., when standing at the start of the slope
  - Added a customizable auto-aim reticle (485de86)  
    The reticle design was inspired by the N64 port of the game and The Legend of Zelda: Ocarina.
  - [QOL] Added smooth damp interpolation for twisting actors' aiming joints (301fe1f)  
    This improves the fluidity and realism of animations during auto-aiming.
  - [QOL] Added blood splatter effect when actor is hit by a weapon thing (51ec333, 02578a2)  
    This extends the existing blood splatter effect (originally only triggered on actor death)  
    to spawn every time an actor is hit by a weapon.
  - [QOL] Added sound effects for projectile impact collisions with world surfaces (51ec333)  
    This enhances gameplay feedback by playing appropriate sounds when projectiles collide with level surfaces, such as walls or terrain.
  - [QOL] Enhanced sound effects for projectile impact collisions with cog thing (51ec333)  
    This adds projectile impact sound fx fpr cog things flagged as snow, earth or wood. (07cecb3, 2aaa779)
  - Fixed an issue where the player's move status was not set to falling when a roll move ended in mid-air (ca74780)  
    This resolves a glitch where the player could initiate unintended actions (e.g.: jump, backward move, chalk draw) after a roll, allowing slow-fall exploits.
  - [QOL] Implemented climb move speed boost when "run by default" is enabled or the run key is pressed (9845bde, bc1fd0f)
  - [QOL] Added underwater swim boost mechanic (af0516e)  
    Pressing the run/boost key while underwater triggers a 3-second speed boost at the cost of endurance penalty
  - Fixed `user0` message not being sent to the master COG on game restore from savegame when `SITHDEBUG_INEDITOR` flag is set (7896215)  
    Previously, the engine would incorrectly send the `user0` message during savegame restoration,  
    causing the intro cutscene to play even when the `SITHDEBUG_INEDITOR` flag was set.  
    This fix ensures the message is suppressed in this scenario, preventing unintended cutscene playback.
  - Fixed `fixme` console command to restore player floorstick physics flag and stop forcemove animation on game restore (00ac4c2)
  - Players can now climb 1m and 2m thing objects or surfaces while carrying a weapon (482be1c, 4586a8c)
  - Fixed an issue where the walk animation would incorrectly persist during jumps (b990683)
  - Increased floor item search radius by 20% (0429f98)
  - Prevented playing climb-pullup animation when movement could be blocked (144d684)
  - Fixed not to play climb left/right animation when found climb surface is currently attached surface (c0b8367)
  - [QOL] Enabled mounting climb surface while walking/running (8e1f3e3)
  - Applied official 1.2 update fix to `jonesConfig_ControlToString`: key names in the control configuration dialog are now queried from the system keyboard layout with `GetKeyNameTextA` instead of a hardcoded English table. ()
  - Applied official 1.2 update fix to `jonesConfig_StoreDialog_HandleWM_COMMAND`: closing the store dialog with a balance of exactly 0 now shows the purchase confirmation instead of leaving silently. ()
  - Applied official 1.2 update fix to `jonesConfig_ShowDialogInsertCD` and `jonesConfig_InsertCD_HandleWM_COMMAND`: the cursor is hidden when the insert-CD dialog is closed from its OK/Quit handler instead of after the dialog returns. ()
  - Applied official 1.2 update change to the end credits (`JonesHud_DrawCredits`): added "Director of Sales Operations: JASON HORSTMAN" (the credits table now has 327 records) and swapped the order of `JOSH MOORE` and `HEATHER TWIST`. ()
  - Applied official 1.2 update fix to push/pull in `sithPlayerControls_ProcessGeneralMove`: the collision check that decides whether the Marduk statue (`2marpushstatue`) can be pushed or pulled starts 0.05 lower. ()
  - Applied official 1.2 update fix to `sithPlayerControls_ProcessPushPullMove` (part of `sithPlayerControls_ProcessGeneralMove` in the original): the collision check that decides whether the Marduk statue (`2marpushstatue`) can be pushed or pulled starts 0.05 lower. ()
  - Applied official 1.2 update fix to `sithCamera_SetCameraCanvas`: changing the canvas resets the focus interpolation of the external camera so the camera focus is re-initialized. ()
  - Applied official 1.2 update fix to `sithWeapon_ProcessWeaponControls`: activating the whip also clears the deactivated player weapon id. ()
  - Applied official 1.2 update fix to `sithActor_DamageActor`: the player no longer plays a damage sound when damaged by itself with damage types `0x1000`, `0x2000` or `0x4000`. ()
  - Applied official 1.2 update fix to `sithWhip_CreatePlayerWhip`: the whip thing pointer is cleared when the whip template or the player hand mesh is missing, instead of keeping a stale pointer from a previous level. ()
  - Applied official 1.2 update fix to `sithWhip_CreatePlayerWhip`: the whip thing pointer is cleared when the whip template or the player hand mesh is missing, instead of keeping its previous value. ()
  - Fixed `sithInventory_CreateBackpack`, `sithInventory_PickupBackpack` and `sithInventory_GetBackpackItemValue` to store and read the backpack item amount inline as the original does, instead of dereferencing the float bits as an item pointer (crash when a backpack is created or picked up). ()
  - Fixed `sithPhysics_UpdateMineCarPhysics` braking sign: a minecart moving forward with backward thrust now decelerates instead of accelerating. ()
  - Fixed `sithPhysics_PowerOffMineCar` clearing all thing flags except `EMITLIGHT` when leaving the minecart; it now only clears `EMITLIGHT` to turn off the rear light. ()
  - Fixed `sithAnimate_UpdateSlideSkyAnim` advancing the horizon sky offset twice per update, which made the horizon sky scroll at twice the original speed. ()
  - Fixed `sithExplosion_UpdateExpandFade` clearing only the fade flag instead of both the expand and fade flags, which left explosions stuck in the expand/fade state so that later explosions skipped their first-time setup. ()
  - Fixed `sithExplosion_Update` variable light: the explosion light color is now the emit color plus the max light scaled by the remaining blast time, instead of the emit color multiplied by it. ()
  - Fixed `sithThing_PlayCogDamageSound` to keep the original sound radii (0.5 and 2.5) for the impact and machete hit sounds outside of QOL builds. ()
  - Fixed `sithSoundClass_PlayPlayerVoiceModeRandom` repeat avoidance wrapping one entry too early, so the last player voice entry of a mode was skipped after a repeat. ()
  - Fixed the `strtof`/`strtoul` conversion range checks in `sithActor_ParseArg`, `sithWeapon_ParseArg`, `sithItem_ParseArg` and `sithThing_ParseThingArg` to reset `errno` before each conversion, so a stale `ERANGE` no longer rejects valid values (template stats could silently be lost at load). ()
  - Fixed the blood splatter sprite end size in `sithWeapon_GenBloodSplatterEx` outside of QOL builds: the third component of the end size (the sprite alpha, not a depth) is 1.0 as in the original, was 0.8. ()
  - Fixed the bad argument log message in `sithThing_ParseThingArg` passing the file name and line number in swapped order (crash on a malformed thing argument). ()
  - Fixed `sithThing_DetachThing` to validate the thing pointer only when a world is loaded, as the original does, instead of dereferencing a NULL world. ()
  - Fixed `sithThing_PurgeGarbageThings` checking the purge limit with a pre-increment, which checked one slot fewer per call than the original. ()
  - Fixed `sithExplosion_CreateSpriteThing` testing the explosion flags instead of the explosion info pointer before setting the sprite roll angle, so sprites of explosions without flags now get their roll angle reset. ()
  - Fixed `sithCogFunction_SetTimerEx` truncating the delay in milliseconds toward zero instead of flooring it. ()
  - Fixed `sithAIClass_LoadEntry` truncating instinct integer arguments toward zero instead of flooring them. ()
  - Fixed `sithCogFunctionThing_SetThingStateChange` storing the two COG arguments in swapped fields, which gave COG-driven AI state changes the wrong type and parameter. ()
  - Fixed `sithCogFunction_FindNewSectorFromThing` pushing no result when the thing is invalid, which unbalanced the COG stack; it now returns -1. ()
  - Fixed `sithCogFunctionThing_GetInventoryCog` pushing no result for inventory types without a COG, which unbalanced the COG stack; it now returns -1. ()
  - Fixed `sithCogFunctionThing_TrackToMode` not returning after a bad thing error, which pushed a second result or dereferenced the invalid thing. ()
  - Fixed `sithCogFunctionThing_GetThingRotVel` not returning after pushing the zero vector for a bad or non-physics thing, which pushed a second vector or dereferenced a NULL thing. ()
  - Fixed `sithCogFunctionThing_SetThingMaxHeadVel` not returning for an invalid thing, which dereferenced a NULL pointer. ()
  - Fixed `sithCogFunctionThing_ThingLightAnim` pushing no result when the light animation could not be started, which unbalanced the COG stack; it now returns -1. ()
  - Fixed `sithCogFunctionThing_StartQuetzAnim` accepting negative modes without logging an error or pushing the -1 result. ()
  - Fixed `sithCogFunctionAI_AIGetGoalThing` pushing no result for an invalid or non-AI thing, which unbalanced the COG stack; it now returns -1. ()
  - Fixed `sithCogFunctionAI_AIGetGoalLVec` pushing no vector for an invalid thing and reading the AI control data of non-AI controlled things; both now return the zero vector. ()
  - Fixed `sithCogFunctionAI_IsAITargetInSight` pushing no result for an invalid AI thing, which unbalanced the COG stack; it now returns 0. ()
  - Fixed `sithCogFunctionSurface_SlideWall` pushing no result for an illegal surface or vector, which unbalanced the COG stack; it now returns -1. ()
  - Fixed `sithCogFunctionSurface_SetWallCel` pushing no result for an illegal surface, which unbalanced the COG stack; it now returns -1. ()
  - Fixed `sithCog_GetSymbolRefInitializer` converting a COG reference of -1 (no cog) to the number of cogs. ()
  - Fixed `sithCogFunctionThing_SetInventory` accepting an inventory type equal to the maximum type count, which wrote one entry past the end of the inventory. ()
  - Fixed `sithOverlayMap_UpdateHints` not removing the SEEN flag from the hint value because of an operator precedence error in the mask, which made the map move on to the wrong hint once a hint had been seen. ()
  - Fixed `sithOverlayMap_DrawSector` testing the lower bound of the vertical canvas range with `>` instead of `<=`, which wrongly culled map sectors. ()
  - Fixed `sithPlayerActions_HopBack` always playing the first sound of the jump mode instead of a random one. ()
  - Fixed `sithFX_CreateChalkMark` scaling the random chalk mark pick by the array length instead of length - 1, which could index one past the end of the chalk mark template names. ()
  - Fixed `sithPlayerControls_ProcessJewelFlyMove` not stopping the vertical velocity when no vertical key is pressed and the player cannot move down, while flying with the jewel. ()
  - Fixed `sithPlayerControls_ProcessPushPullMove` giving the back (pull) key priority over the forward (push) key when both are held; the push now wins like in the original. ()
  - Fixed `sithPlayerControls_ProcessSwimMove` rotating the swim thrust by the head angles, which the original never does, so swimming direction followed the head instead of the body. ()
  - Fixed `sithPlayerControls_ProcessClimbMove` ignoring the jump, climb and strafe keys while the action key (pull up) is held, even when no pull up was possible. ()
  - Fixed the water ripple sector in `sithFX_GetWaterPosition` callers (raft splatter, raft wake and paddle water effects): the found water surface sector is now returned through a pointer instead of being assigned to a copy. ()
  - Fixed `JonesHud_MenuClose` replacing instead of accumulating the collapsed column bits of the menu close animation, which broke the closing of inventory menus with more than one expanded column. ()
  - Fixed the quick load key (F8) leaving saving disabled when there is no quick save file or when loading the quick save and the last save both fail. ()
  - Fixed `JonesHud_GetKey` resetting the menu move duration on every key read, which disabled the speed up of the inventory menu when a direction key is held. ()
  - Fixed the key assignment dialog (`jonesConfig_AssignControlKey_ReadKey`) rejecting the number keys for the fists, whip, Mauser, PPSh-41, pistol, Simonov, Tokarev and TOZ-34 actions; they are accepted for all weapon actions from fists to grenade as in the original. ()
  - Fixed the key assignment dialog (`jonesConfig_AssignControlKey_ReadKey`) showing the "number keys only for weapons" message and clearing the key name after an accepted number key. ()
  - Fixed editing the assignment of an action that has no bindings in the controls dialog (`jonesConfig_EditControlScheme_EditAssignment`), which was silently ignored after the key was chosen. ()
  - Fixed adding a first binding to an unbound action in the controls dialog (`jonesConfig_EditControlScheme_AddAssignment`) not storing the new control in the list item, which made later edits and deletes of that row use the wrong binding. ()
  - Fixed `jonesConfig_LoadControlScheme` accepting a truncated or malformed control scheme (.kfg) file as successfully loaded; the scheme is now rejected and cleared like the original. ()
  - Fixed `jonesConfig_ShowGameOverDialog` returning the game over music channel to `JonesHud_ShowGameOverDialog` in builds without QOL improvements, where the original never does; with QOL improvements the channel is still returned so the game over music is stopped. ()
  - Fixed `sithControl_UnbindJoystickAxes` leaving joystick POV hat direction bindings bound when the joystick bindings are reset, because `STDCONTROL_ISJOYSTICKBUTTON` no longer matches POV control ids. ()
  - Fixed `sithControl_GetKeyAsAxisNormalized`, `sithControl_GetKeyAsAxis` and `sithControl_sub_44F334` no longer matching joystick POV hat bindings in the joystick disabled checks and the binding enumeration. ()
  - Fixed `jonesConfig_LoadControlScheme` never reporting "too many bindings" for an action with more than 8 bindings in a control scheme file; the clamped binding count could not exceed the limit, so the loader kept writing bindings past the end of the action. ()
  - Fixed the save game dialog hook (`jonesConfig_SaveGameDialogHookProc`) falling into the CDN_INITDONE code after accepting a new file name, which created the dialog fonts a second time and leaked the previous one. ()
  - Fixed `jonesConfig_InitKeySetsPath` always inserting an extra backslash between the install path and the key sets directory, even when the install path already ends with one. ()
  - Fixed `jonesConfig_Shutdown` leaving the deleted dialog icon bitmap handles in the table, so the handles could be deleted again after a restart of the module. ()
  - Fixed the game statistics dialog level name table lacking the summary page entry, which made the title fallback of the summary page read past the end of the table. ()
  - Fixed `jonesInventory_UpdateLevelStatisticTime` narrowing the hours of the level and total game time statistics to 8 bits, which made the played time wrap to zero after 256 hours. ()
  - Fixed the save and load game file dialogs building their file filter string by formatting a buffer into itself (`jonesConfig_GetSaveGameFilePath`, `jonesConfig_GetLoadGameFilePath`), which is undefined behavior. ()
  - Fixed `jonesConfig_ShowControlOptions` leaking the copied control configuration when the dialog returns with an invalid selected scheme index. ()
  - Fixed `JonesMain_GameWndProc` returning an uninitialized window procedure result for handled `SC_KEYMENU` and `SC_SCREENSAVE` system commands. ()
  - Fixed `sithAI_Startup` creating the instinct registry and resetting all AIs again when called while the AI system is already initialized. ()
  - Fixed `sithAI_Shutdown` freeing the instinct registry again (asserting in debug builds) when called while the AI system is already shut down. ()
  - Fixed `sithAI_Close` resetting all AIs when called while the AI system is not open. ()
  - Fixed `sithAI_ProcessUnhandledEvent` scaling the random cliff retreat distance by the double constant 0.2 instead of the float constant 0.2f, which changed the distance by a fraction of a float ulp. ()
  - Fixed `sithPhysics_UpdateExhaustFx` (mine cart and jeep exhaust smoke) placing the smoke puffs without the vehicle speed factor in their spawn offset, scaling the puff life scatter and the puff size and alpha in a different operation order than the original, and drawing the random numbers of the puff alpha in the wrong order. ()
  - Fixed `sithPlayerControls_ProcessPlayerDebugControls` stopping the processing of the player controls while the debug key is held and no player position key is pressed; the original continues with the normal controls. ()
  - Fixed `sithPlayerControls_CanStrafeMove` transforming the vertex of a thing face with the translation of the thing orientation matrix; the original transforms it as a vector and adds the thing position. ()
  - Fixed `sithVehicleControls_ProcessMineCarMove` ending the accelerating state of the mine cart using the new thrust after the first acceleration step from standstill, where the original tests the old (zero) thrust. ()
  - Fixed `sithPlayerControls_ProcessHangMove` starting the hang shimmy left animation, sound and surface attach in a different order than the original. ()
  - Fixed `sithPlayerControls_ProcessSwimMove` resetting the vertical swim thrust of the player while on the water surface; the original only resets it when not on the surface. ()
  - Fixed `JonesHud_Process` rounding the health state passed to the health indicator to the nearest integer where the original truncates, which made the indicator and the hit overlay step one unit early. ()
  - Fixed `sithThing_ScaleCombatDamage` scaling the damage by the combat damage scalar and the 0.9 or 0.8 factor in separate steps for fleeing actors shot by the player, which differs from the original in the last bit. ()
  - Fixed `sithActor_Update` computing the raft leak damage interval in a different operation order than the original, which differs in the last bit. ()

## v0.3.1
### General:
  - Fixed radius of fading plate 3DO models (5d705fb)
  - Codebase update and cleanup (bf2c4f7)
  - Fixed bug calculating time delta when path move thing is blocked (f361b65)
    This fixes the bug where another thing object blocks the path of path moving objects, causing them to continue moving indefinitely as long as the path is blocked.
    e.g.: pressed button moves out of socket if player blocks its path when the button moves to initial position.
  - Fixed incorrect usage of enum flags (918bcee)
  - Added define guard for `STRICT` macro to prevent redefinition warnings (1b0e597)
  - Fixed incorrect enumerator names enum `SithCollisionType` (f2cbab2)

## v0.3
### General:
  - Fixed various bugs
  - Fixed game crash on save game restore when an object is attached to a non-primary mesh face of a 3D model (45c8065) 
  - Fixed hiding console on cutscene start (27ca92b)
  - Fixed light range in `ThingLight` / `SetThingLight` COG functions (e9d4167)  
    This fix ensures that the light range is correctly set when the COG function is called.  
    Fixes light flickering bug when camera is facing and is near the light source.
  - Fixed casting float to int (0ada120)  
    This fix ensures that:
    * The correct integer values are popped from the COG execution stack.
    * AI instinct parameters are correctly parsed when the AI class is parsed.
  - Added developer COG verbs (33f8f9d)
  - Added error log when popping invalid string from COG stack (34c9e20)
  - Rename debug mode flag `SITHDEBUG_AILOOK_FOR_TARGET_DISABLED` to `SITHDEBUG_AINOTARGET` (53b9091)
  - Changed argument order of debug print COG functions (a80ed30)  
    Also changed the log level to debug.
  - Added programs `yacc` & `flex` (d6f5be6 + b669ca8)
  - Fixed yacc initial line number at the start of code parsing. (232f268)  
    This fixes line number in log when error is encountered.
  - Fixed bug in multiple COG thing functions (`sithCogFunctionThing` module) where a return value  
   was not pushed to the execution stack on error. (e9d4167)  
    This fixes corruption of the COG's execution stack when an error occurs.
  - Added new `rdVector` arithmetic functions (c4c8ee5)
  - Fix null pointer dereference && infinitive loop in `sithAnimate_BuildScrollFaceRotate`, `sithAnimate_PushItem`, `sithAnimate_PullItem`, `sithAnimate_UpdatePushItemAnim` and `sithAnimate_UpdatePullItemAnim` (70cf7bf)
  - Fixed handling division by zero in `sithAnimate_StartSpriteSizeAnim` & `sithAnimate_StartThingMoveAnim` (f5dc718)
  - Altered the `AnimateSpriteSize` COG function to return -1 when deltaTime is 0. (039a227)  
    This change prevents unnecessary animation creation, thereby saving animation slots.  
    The function now returns -1 to the script when the sprite size is set instantly but no animation is created,  
    whereas previously, -1 was returned only on animation creation errors.
  - Fixed deterministic restoration of 3D sound handles from savegame file and prevented sound handle collisions (6c3acc4)  
    * Recalculates original handle entropy from saved handle before playing or loading sounds.
    * Ensures restored handle entropy never falls below the highest handle used during restore to prevent handle collision.
    * Prevents handle collisions that could swap playing sounds and cause looping sounds to stick at the player's position.
    * Ensures restored 3D and far playing sounds have exactly the same handles they had when the sound state was saved.  
    This fixes issues with COG scripts that store sound handles in variables.  
    Previously, restored sounds had different handles than when they were saved, which potentially broke script logic.
  - [QOL] Removed the loop flag from sound modes (2a757af)  
    This prevents repeated accumulation of the same sound in sound buffer, ensuring the buffer doesn't fill up and preventing new sounds from playing.  
    *Note, marked as QOL because this issue becomes most relevant when QOL for looping far sound is in place.*
  - [QOL] Increased max number of simultaneous playing sounds to 128 (d8fdedc)
  - Fixed OOB bug in command line level selection (24ab38b)
  - Added level related hardcoded constants (fedb5bf)
  - Fixed resetting IMP fire state, player visibility, and disabling of swimming inventory on system close (645471a)
  - Added new system COG functions:
    * `DebugVector` (eb860f8)
    * `EnablePrint` (32763c2)
    * `IsPrintEnabled` (32763c2)
    * `DebugPrint` (33f8f9d)
    * `DebugFlex` (33f8f9d)
    * `DebugInt` (33f8f9d)
    * `DebugLocalSymbols` (33f8f9d)
    * `DebugWaitForKey` (33f8f9d)
    * `sithSprite` (3f0d87f)
    * `sithParticle` (3f0d87f)
    * `sithPathMove` (8ba32fb)
  - Implemented `Print*` Cog functions (33f8f9d)
  - Implemented sound save & restore functions (4ab0c6f)
  - Implemented modules:
    * `sithConsole` (7879d45)
    * `sithCommand` (4ec4941)
    * `sithCogFunction` (33f8f9d)
    * `sithCogFunctionSector` (e46c2ab)
    * `sithCogFunctionSurface` (40b6d38)
    * `sithCogFunctionSound` (2df2214)
    * `sithAI` (ca79900)
    * `sithAIAwareness` (d7f822f)
    * `sithAIClass` (d7f822f)
    * `sithPuppet` (a68d1a7)
    * `sithTemplate` (a53d895)
    * `SoundDriver` (0530afe)
    * `sithSoundClass` (3d2d82d)
    * `sithCogFunctionAI` (f6c8fb8)
    * `sithSound` (5bbb9eb)
    * `sithExplosion` (d15ac1c)
    * `sithCogParse` (b669ca8)
    * `sithCogYacc` (b669ca8)
    * `sithCogFunctionPlayer` ([PR#19](https://github.com/smlu/OpenJones3D/pull/19))
    * `sithCogFunctionThing` ([PR#19](https://github.com/smlu/OpenJones3D/pull/19))
    * `jonesConfig` ([PR#21](https://github.com/smlu/OpenJones3D/pull/21))
    * `sithWorld` (85a9ebe)
    * `sithOverlayMap` (967e433)
    * `sithAnimate` (ef93c3c)
    * `sithGamesave` (3dc6ee4)
    * `sithDSSCog` (0b8f620)
    * `Sound` (664b8f8)
  - Added `Sound_GetAllInstanceInfo` & `Sound_SoundDump` functions (8654185)
  - Added `stdConffile_GetWriteFileHandle` function (cee02bd)
  - Fixed bug to correctly update key press state in `stdControl_ReadAxisAsKey` function (1ab4c44)
  - Refactored `stdControl` module (fc4060c)
  - Fixed bug to correctly obtain the file handle when opening a savegame file for reading and writing (a90a59d)
  - Added devmode logic to `sithConsole` module (57b685b)  
    This enables commands flagged with `SITHCONSOLE_DEVMODE` flag to be available only
    when console flags have `SITHCONSOLE_DEVMODE` set.
  - Added devmode console commands:
    * `jumplevel` (e59ffab)
    * `radius` (e59ffab)
    * `pvs` (e59ffab)
    * `nextlevel` (e59ffab)
    * `endlevel` (e59ffab)
    * `restart` (e59ffab)
    * `sounds` (e59ffab)
    * `devmode` (e59ffab)
    * `ineditor` (e59ffab)
    * `money` (1e9a47f)
    * `interface` (1e9a47f)
    * `indicator` (1e9a47f)
    * `invul`, `wuss`, `ouch`, `warp`, `jump`, `imp1`, `imp2` (9d399b0)
    * `imp3`, `imp4`, `imp5`, `fly`, `players`, `ping`, `kick` (9d399b0)
    * `tick`, `session`, `electroman`, `trackshots`, `caminfo` (9d399b0)
    * `fov`, `tj`, `weapons`, `help`, `disableai`, `notarget` (9d399b0)
    * `disablepuppet`, `cogtrace`, `coglist`, `cogstatus` (9d399b0)
    * `noaishots`, `aistatus`, `ailist`, `cogpause` (9d399b0)
    * `pauseallcogs`, `matlist`, `activate`, `slowmo` (9d399b0)
    * `stopsound`, `wpdraw` (9d399b0)
    * `noclip` (2d9f47a)
    * `print` (44e28c5)

### Display & Render:
  - Added support for display resolutions higher than 2048-pixel (6f9db7e)
  - Fixed bug rendering ceiling sky (2a200d5)
  - Fixed thumbnail rendering in save game dialog (c7bb134)
  - Fixed thumbnail rendering in exit dialog (b64fb63)
  - Increased size of game save/load dialogs (ce321bb)
  - Changed the native open file dialog for game save dialog (5871f09)
  - Implemented the top layer of the game save dialog, resulting in a dialog style update (f38ff8a)
  - Fixed incorrect scaling & positioning of dialog controls with nested child windows. (50a3f83)  
    This fixes positioning of lists in Indy's trade post and key config dialogs.
  - Increased font size for all GDI dialogs (d1a09a0)
  - Added scaling for embedded dialog icon(s) (d1a09a0)
  - Increased size of exit dialog icon (fc92734)
  - Implemented chapter completed dialog (8ceab2c) 
  - Fixed rendering of IQ points in chapter completed dialog (8ceab2c)
  - Implemented Indy's trading post dialog (a8a32ff)
  - Fixed column padding in trading post dialog (a8a32ff)
  - Increased the size of store item icon to 48 (a8a32ff)
  - Fixed drawing 2D line to 24 BPP & 32 BPP video buffer (81b4bc7)
  - Fixed the alpha value of the health indicator on the first run and when the screen resolution changes (e32a52d)
  - Fixed the hardware-accelerated drawing of 2D lines (5a63bc7)  
    Subsequently, this fix addresses the drawing error of level wireframe geometry on the overlay map

### Game play:
  - [QOL] Fixed removal of loop-playing far sound. (2a25add)  
    This ensures the loop-playing sound continues to play when the listener comes in sound hearing range.
  - Fixed submersion of dead thing in cold-water/lava (18d828d)
  - Fixed mouse sensitivity when in-editor mode (d25081f)
  - Added mouse button actions for fire/activate, jump, weapon toggle, and weapon selection when in-editor mode (d25081f)
  - Restricted developer/debug control keys to in-editor/dev mode only (d25081f)
  - [QOL] Fixed stop looping far sound when volume is below threshold (5a169d7)
  - Fixed playing far sound on sound restore from savegame (b8a82d4)
  - [QOL] Improved AI controlled actor movement at cliff edges (8ed5690)  
    Enhanced the AI actors's behavior when reaching cliff edges. Previously, AI actor would perform a 180-degree turn followed by a random 60-degree movement in new direction. Now, it performs a random 60-degree movement based on its prior direction, resulting in more natural and predictable navigation.
  - [QOL] Fixed AI collision deadlock on predefined paths (49ef284)  
    Two AI actors that collided head-on traversing a predefined waypoint path previously entered a "dancing" loop as they continuously attempted to pass each other. Now, upon detecting a collision, each actor randomly adjusts its direction by 0–45° in its current movement direction, enabling them to maneuver past one another.
  - [QOL] Fixed an issue with falling velocity during jump-rollback movements (c1673ac)  
  Originally, when player performed a jump-rollback, the fall velocity was incorrectly reset to 0, allowing them to drop from dangerous heights without taking damage. Now, the fall speed is properly maintained.

## v0.2
### General:
  - Fixed various bugs
  - Fixed floating point precision bug in `rdVector` & `rdMatrix34` functions (3b800ad)
  - Made OpenJones3D binaries portable by adding absolute resource dir path to JonesFile VFS (fca6ef2)  
    This change allows binaries to be placed in any arbitrary location, eliminating the requirement for a Resource folder.
  - Fixed typo in framerate output text (47537aa)
  - Fixed initializing and clearing resource array on `JonesFile_Close` (47537aa)
  - Fixed the invocation of `jonesString_GetString("JONES_STR_LOADERROR")` on every call to `JonesMain_ProcessGamesaveState` when there is no load error or the game is not being restored (47537aa)
  - Fixed error text when it was cached by `jonesString_GetString` in call to `JonesMain_LogErrorToFile` (485d553)
  - Changed the location of `JonesError.txt` to be always written in current working directory instead of installed path (485d553)  
    This change resolves a potential fatal error that could occur when writing the error log due to the registry system failing to startup.
  - Changed warning log to output to console or file (dcd3015)
  - Made console log output colorized (dcd3015)
  - Added debugger log output (dcd3015)
  - Removed material buffer size constraint when loading material section form NDY level file in `sithMaterial_LoadMaterialsText` (dcd3015) 
  - Fixed potential infinitive loop bug when .uni file is missing line break at the end of file (6b71793)
  - Implemented `sithSurface` module (b448041)

### Display & Render:
  - Fixed scaling of color component when converting pixel data row in `stdColor_ColorConvertOneRow` (9e6ba77)
  - Changed default engine texture format to 32 bit from 16 bit (249a416)
  - Removed support for 16 bpp textures (3152496)
  - Increased `rdCache` vertex buffer size to 32768 * 64 (25ceaba)
  - Increased `rdModel3` (.3DO) mesh constraints based on `rdCache` constraints (a008386)
  - Increased max no. of sectors in camera view to 4096 (b448041)
  - Increased max no. of thing with sectors to 8192 (b448041)

### Game play:
  - Changed to by default play the higher resolution intro movie (800x600) (ffa2e9d)
  - Fixed retrieving model by index number when loading savegame (669904d)  
    This change adds temporary wrapper function `sithModel_GetModelByIndexWrap`.
  - Added support for 24/32 bit textures to CND cnd file format  (91d0a87)
  - Changed how developer dialog discovers level files (30630d1)  
    Now dialog first looks for ndy folder in current working directory, and if found searches for level files in that folder.  
    If ndy folder is not found the dialog checks for level files in `Install Path\Resource\ndy`.
  - Fixed searching for ndy level file when corresponding cnd file is not found (5e64e5b)  
    When game progresses to the next level this enables using ndy file for original levels when the corresponding cnd file is not found
  - Enabled weapon COG functions in devmode (a9cbc0b)

### HUD:
  - Fixed drawing of horizontal wall line (8046835)

## v0.1
### General:
  - Fixed minor bugs
  - Improve game loading times by disabling VSync and blt frame buffers instead of flip.
  - Changed DPI awareness to per monitor aware v2
  - Added check for NAN value of provided angle in `stdMath_SinCos`
  - Fixed issue with float precision in `stdMath_NormalizeAngle` which could result in angles larger than 360 degrees.  
    This fixes array out of bound read in `stdMath_SinCos`, which fixes random crash when opening inventory menu  
    or calling COG function `JonesInvItemChanged` from script.
  - Fixed deleting hdc on pixel data read error in `stdBmp_Load`
  - Fixed reading/writing 24 bpp pixel from bytes in `stdColor_ColorConvertOneRow`.
    This fixes conversion of 24bit RGB image.
  - Fixed exit code (0) when app successfully exits
  - Fixed infinitive loop when call to `wkernel_pfProcess` returned value other than -1, 0 or 1 (`wkernel_Run`)
  - Fixed copying full pixel data row from source mipmap in `std3D_AllocSystemTexture`. Originally was fixed to 16 bpp row.
  - (`JonesFile`) Fixed path buffer size to 128 chars, was 48 chars
  - (`JonesFile`) Fixed writing to file by using correct file write function in `JonesFile_FilePrintf`
  - Added bounds check for number of animating joints when loading keyframe from *.key file
  - Fixed a bug in sound compression where clipping of the audio waveform occurred
  - Fixed infinitive loop bug when voice subtitle contains too long word (`sithVoice_AddSubtitle`)
  - Fixed accessing null pointer in `sithWhip_Reset` when `sithCamera` is already closed.  
    Fixed checking for null `sithWeapon_SendMessageAim` before calling  
    function`sithWeapon_IsLocalPlayerUnableToUseWeapon` in `sithWeapon_SendMessageAim` functions.  
    This 2 fixes solve the game crash when player is aiming with whip and exits the game/program
    at the same time.

  - Fixed game crash when player aims with the whip and exits the game simultaneously by:
    - Addressing a null pointer access in `sithWhip_Reset` when `sithCamera` is already closed.
    - Adding a null check for `sithWeapon_SendMessageAim` before calling `sithWeapon_IsLocalPlayerUnableToUseWeapon`.
    - Added high poly checkbox option to developer dialog (`High poly objects`)
    - Added high poly checkbox option to gameplay options (`High poly objects`)

### Game play:
- Fixed the direction of fired projectiles for 2-hand-held weapons by setting the initial direction.  
  This should resolve the issue of random crashes when firing the weapon, as the projectile will now be correctly aimed at the target.
- Fixed damaging player when game runs at > 40 FPS and Taklit's part, Azerim's part or tool from beyond runs out of energy (`sithPlayer_Update`)
- Fixed rendering file list in load game dialog
- Fixed DPI scaling of dialogs
- Fixed bug where controls could become unresponsive when climbing down a whip and pressing the jump-off key simultaneously.
- Enabled HD 3D models by default

### Display & Render:
  - Added "simulated" VSync in window mode
  - Added resolution cap to HD resolutions since Direct3D3 doesn't support higher resolutions
  - Fixed raster to use correct vertices buffer when in vertex only mode
  - Fixed clearing raster vertex buffer on startup
  - Added option to disable VSync in `stdDisplay_Update`
  - Fixed rendering of sprite when fog is globally disabled
  - Fixed rendering of polyline when fog is globally disabled
  - Fixed projecting Z coordinate when camera aspect ratio != 1.0  
    In addition, this resolves rendering of ceiling sky when aspect ratio != 1.0
  - Fixed screen space projection by correctly applying camera aspect ratio
  - Fixed restoring backbuffer when surface is lost on surface lock
  - Removed hardcoded 16 BPP resolution limitation; the game now defaults to the resolution stored in settings.
  - Removed hardcoded 16 BPP startup resolution limitation when initializing developer dialog allowing for the resolution stored in settings to be selected by default.
  - Set 32 BPP resolution as default
  - Fixed using 32 BPP resolution stored in settings

### HUD:
  - (`JonesDialog`) Fixed rendering game background when system dialog box is open
  - Removed help menu option
  - Fixed inventory menu position for wide screen resolutions
  - Fixed inventory menu scaling when aspect ratio is other than 4:3
  - Fixed inventory changed animation position and scaling when aspect ratio is other than 4:3
  - Fixed scaling of HUD health & endurance indicators for screen resolutions other than 4:3
  - Fixed possible menu close lock and game freeze due to uninitialized icon translation move rate.
  - Fixed start icon transition when closing menu.  
    Properly initialized move rate for the first transitioning icon, preventing its immediate disappearance.
  - Fixed bug in inventory item change render function (`JonesHud_RenderInventoryItemChange`) which used  
    an unbounded yaw value (yaw > 360°) in the calculation of the item's yaw angle. This could lead to  
    an infinite loop and prevent the menu from opening.
  - Fixed rotation bug for selected inventory item (`JonesHud_UpdateItem`).  
    The yaw added to delta yaw was not bounded to 360 degrees, which could lead to incorrect rotation of the item.
  - Fixed rendering end credits on different screen resolutions
  - Fixed fadeout of credits theme music
  - Increased credits fadeout to 2 sec
