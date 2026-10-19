// Memory.c: the memory services (`MEMORY=insane`, `MEMORY=insane_debug`) (original 0x004EAE10-0x004EB210).
#include <smush/insane/Memory/Memory.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Memory_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(memory_Register);
    // J3D_HOOKFUNC(memory_Unregister);
    // J3D_HOOKFUNC(memory_debugAlloc);
    // J3D_HOOKFUNC(memory_Track);
    // J3D_HOOKFUNC(memory_debugFree);
    // J3D_HOOKFUNC(memory_Check);
    // J3D_HOOKFUNC(memory_debugRealloc);
}

void Memory_ResetGlobals(void)
{}
