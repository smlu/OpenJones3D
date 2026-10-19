// LecShare.c: the LEC share table (module registry) and log (original 0x004EB210-0x004EBDE0).
#include <smush/insane/Lec/LecShare.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void LecShare_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(LecShare_Init);
    // J3D_HOOKFUNC(LecShare_UnregisterAll);
    // J3D_HOOKFUNC(LecShare_Startup);
    // J3D_HOOKFUNC(LecShare_Shutdown);
    // J3D_HOOKFUNC(LecShare_Reset);
    // J3D_HOOKFUNC(LecShare_Publish);
    // J3D_HOOKFUNC(LecShare_Unpublish);
    // J3D_HOOKFUNC(LecShare_Subscribe);
    // J3D_HOOKFUNC(LecShare_Unsubscribe);
    // J3D_HOOKFUNC(LecShare_BatchSubscribe);
    // J3D_HOOKFUNC(LecShare_BatchUnsubscribe);
    // J3D_HOOKFUNC(LecShare_Enumerate);
    // J3D_HOOKFUNC(LecShare_Count);
    // J3D_HOOKFUNC(LecShare_Find);
    // J3D_HOOKFUNC(LecShare_Match);
    // J3D_HOOKFUNC(LecShare_NameLen);
    // J3D_HOOKFUNC(LecShare_Strip);
    // J3D_HOOKFUNC(LecShare_NameCmp);
    // J3D_HOOKFUNC(LecShare_NameNCmp);
    // J3D_HOOKFUNC(lec_Log);
}

void LecShare_ResetGlobals(void)
{}
