// Stdfile.c: the `FILE=insane_stdfile` service (FILIO plus size, gets, eof, printf, getc, putc) (original 0x004E9810-0x004E9B30).
#include <smush/insane/File/Stdfile.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Stdfile_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(stdfile_Register);
    // J3D_HOOKFUNC(stdfile_Unregister);
    // J3D_HOOKFUNC(stdfile_Startup);
    // J3D_HOOKFUNC(stdfile_Shutdown);
    // J3D_HOOKFUNC(stdfile_open);
    // J3D_HOOKFUNC(stdfile_close);
    // J3D_HOOKFUNC(stdfile_read);
    // J3D_HOOKFUNC(stdfile_write);
    // J3D_HOOKFUNC(stdfile_tell);
    // J3D_HOOKFUNC(stdfile_seek);
    // J3D_HOOKFUNC(stdfile_flush);
    // J3D_HOOKFUNC(stdfile_size);
    // J3D_HOOKFUNC(stdfile_gets);
    // J3D_HOOKFUNC(stdfile_eof);
    // J3D_HOOKFUNC(stdfile_printf);
    // J3D_HOOKFUNC(stdfile_getc);
    // J3D_HOOKFUNC(stdfile_putc);
}

void Stdfile_ResetGlobals(void)
{}
