// Added: Null version of smush/insane/Sound/Dsnd.c: the DirectSound driver module is
// substituted by a pure config store. Register behaves like the Windows version minus the
// publishes, and Configure keeps the vararg tag loop; nothing touches a sound device.
#include <stdarg.h>

#include <j3dcore/j3d.h>

#include <smush/insane/Sound/Dsnd.h>
#include <smush/insane/lec.h>

#include <std/General/std.h>
#include <smush/RTI/symbols.h>

static LecShare* dsnd_pShare = NULL;
static void* dsnd_pParam     = NULL;

void Dsnd_ResetGlobals(void)
{
}

// Added: Same success convention as Dsnd.c's dsnd_Register (returns 0), without publishing
// "LECMODULE/SNDDRIVER=insane_dsnd": there is no sound device behind the Null platform, so
// the module's Startup/Start entry points must stay unpublished and unreachable.
int J3DAPI dsnd_Register(LecShare* pShare, void* pParam)
{
    dsnd_pShare = pShare;
    dsnd_pParam = pParam;
    return 0;
}

// Added: Same tag-loop contract as Dsnd.c's dsnd_Configure: int config values are stored for
// parity with the original globals, the game's DirectSound object is accepted as an opaque
// handle (dsnd_ConfigDSound), any unknown tag returns -1, and a terminated list returns 0.
static int dsnd_cfg1 = 0;
static int dsnd_cfg2 = 0;
static int dsnd_cfg3 = 0;
static int dsnd_cfg4 = 0;
static void* dsnd_pDS = NULL;

int J3DAPI dsnd_Configure(int tag, ...)
{
    va_list args;
    va_start(args, tag);
    while ( tag )
    {
        switch ( tag )
        {
            case DSND_CONFIG_1:
                dsnd_cfg1 = va_arg(args, int);
                break;

            case DSND_CONFIG_2:
                dsnd_cfg2 = va_arg(args, int);
                break;

            case DSND_CONFIG_3:
                dsnd_cfg3 = va_arg(args, int);
                break;

            case DSND_CONFIG_4:
                dsnd_cfg4 = va_arg(args, int);
                break;

            case DSND_CONFIG_DSOUND:
                dsnd_pDS = va_arg(args, void*);
                break;

            default:
                va_end(args);
                return -1;
        }

        tag = va_arg(args, int);
    }

    va_end(args);
    return 0;
}
