// D_wave.c: the VIMA wave frame-object decoder (original 0x004E7D60-0x004E8510).
#include <smush/insane/Flobject/D_wave.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void D_wave_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(dwave_Register);
    // J3D_HOOKFUNC(dwave_Unregister);
    // J3D_HOOKFUNC(dwave_Startup);
    // J3D_HOOKFUNC(dwave_Shutdown);
    // J3D_HOOKFUNC(dwave_Name);
    // J3D_HOOKFUNC(dwave_GetTag);
    // J3D_HOOKFUNC(dwave_GetFlags);
    // J3D_HOOKFUNC(dwave_Reset);
    // J3D_HOOKFUNC(dwave_Parse);
    // J3D_HOOKFUNC(dwave_Decode);
    // J3D_HOOKFUNC(dwave_DecodeVima);
}

void D_wave_ResetGlobals(void)
{}
