// List.c: INSANE's doubly linked list (original 0x004F03E0-0x004F0460).
#include <smush/insane/Util/List.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void List_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(LecList_Init);
    // J3D_HOOKFUNC(LecList_Insert);
    // J3D_HOOKFUNC(LecList_Remove);
}

void List_ResetGlobals(void)
{}
