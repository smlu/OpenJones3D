# 11. Risks and technical debt

## 11.1 Risks

| Risk | Effect | Mitigation |
|---|---|---|
| The retail reference is incomplete | Traces prove sameness with a build, not with retail, wherever the reference build already differs | Keep the decompilation as tie-breaker; record each known difference here |
| Simulation code depends on frame rate or wall-clock time | Traces differ between machines, or a high frame rate changes gameplay | Fixed game time in headless runs; find and isolate time sources during M5 |
| A fixed simulation step for high frame rates (M9) changes gameplay | Only player physics uses fixed slices today; the rest advances by the measured frame time, so interpolated rendering as in Doom ports isn't presentation-only | Investigate in M9 which systems depend on frame time; if adopted, an option off by default with the retail trace unchanged ([research](../Research/Engine-Patterns.md)) |
| Threading (M9) breaks determinism | Traces become flaky | Threading comes last, behind traces for everything else |
| SDL3 GPU shader pipeline | The renderer needs offline shader compilation for several formats | Settle the shader toolchain at the start of M6 |
| AGPL code leaks into Babylon | Babylon's MPL-2.0 license is no longer valid | Include and license checks in CI; Babylon is written fresh |
| Copyrightability of AI-assisted code is legally unsettled | Weakens every license claim, Babylon's included | Human review and authorship of design; keep records of decisions |
| Small team | Long milestones stall | Thin slices, each one built, tested and committed |

## 11.2 Technical debt

| Debt | Where | Plan |
|---|---|---|
| Hook and RTI machinery from the reconstruction | `j3dcore`, `<module>/RTI/` | Delete after the documentation rewrite ([ADR-0004](09-decisions/ADR-0004-drop-binary-compatibility.md)) |
| QOL behavior as one compile-time profile | `J3D_QOL_IMPROVEMENTS`, `J3D_SPEEDRUN_BUILD` | Individual runtime options ([ADR-0003](09-decisions/ADR-0003-fidelity-split.md)) |
| Platform types in shared headers | For example `std3D_DrawLineStrip(LPD3DTLVERTEX, size_t)` | Removed as subsystems move behind Babylon in M5 |
| Win32 dialogs | `jonesConfig.c`, `JonesDialog.c` | Replaced by in-game screens in M7 |
| Sound parser can read before its input buffer | `AudioLib_ParseWaveFileHeader` | Fix with a regression test; a malformed sound file triggers it |
| Lip-sync pointer restored from sound banks | `Sound.c` | Force to NULL on load; a wild pointer on 64-bit |
| Cutscene blitter relies on 32-bit pointer wrap | `Libs/smush/insane/Bitmap/BlitAsm.c` | Check with a native cutscene run, then fix |
| Sound bank scalars read as 4 bytes into `size_t` | `Sound.c` | Read through `uint32_t` |
| GCC native builds need manual flags | `-D__cdecl= -D__stdcall= -D__fastcall=` and `-Wno-error=incompatible-pointer-types` | Move into `j3d.h.in` and CMake |
| Multiplayer sends `wchar_t` arrays | `sithMulti.c` | Out of scope while multiplayer has no backend |
