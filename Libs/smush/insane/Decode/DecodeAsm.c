// DecodeAsm.c: C translation of INSANE's MASM source DecodeAsm.asm (the pixel converters) (original 0x004F0590-0x004F09BA).
#include <smush/insane/Decode/DecodeAsm.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void DecodeAsm_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(decode_Lut8);
    // J3D_HOOKFUNC(decode_Lut16);
    // J3D_HOOKFUNC(decode_RGBAto555);
    // J3D_HOOKFUNC(decode_555toRGBA);
    // J3D_HOOKFUNC(decode_RGBAto565);
    // J3D_HOOKFUNC(decode_565toRGBA);
    // J3D_HOOKFUNC(decode_RGBAto565B);
    // J3D_HOOKFUNC(decode_565BtoRGBA);
}

void DecodeAsm_ResetGlobals(void)
{}
