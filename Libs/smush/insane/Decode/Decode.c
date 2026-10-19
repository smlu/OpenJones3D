// Decode.c: pixel-format conversion and RLE decoding (original 0x004EE6A0-0x004EF890).
#include <smush/insane/Decode/Decode.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Decode_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(decode_Rle);
    // J3D_HOOKFUNC(decode_RleInit);
    // J3D_HOOKFUNC(decode_RleByte);
    // J3D_HOOKFUNC(decode_Run);
    // J3D_HOOKFUNC(decode_MakeTable);
    // J3D_HOOKFUNC(decode_Convert);
    // J3D_HOOKFUNC(decode_ToRGBA);
    // J3D_HOOKFUNC(decode_Begin);
}

void Decode_ResetGlobals(void)
{}
