// Engine.c: the sound engine (`SOUND=insane`) (original 0x004E5470-0x004E74A0).
#include <smush/insane/Sound/Engine.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Engine_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(sound_Register);
    // J3D_HOOKFUNC(sound_Unregister);
    // J3D_HOOKFUNC(sound_Startup);
    // J3D_HOOKFUNC(sound_Shutdown);
    // J3D_HOOKFUNC(sound_Open);
    // J3D_HOOKFUNC(sound_Close);
    // J3D_HOOKFUNC(sound_AddTrack);
    // J3D_HOOKFUNC(sound_Play);
    // J3D_HOOKFUNC(sound_Stop);
    // J3D_HOOKFUNC(sound_Pause);
    // J3D_HOOKFUNC(sound_Resume);
    // J3D_HOOKFUNC(sound_StopAll);
    // J3D_HOOKFUNC(sound_SetVolume);
    // J3D_HOOKFUNC(sound_GetVolume);
    // J3D_HOOKFUNC(sound_SetPan);
    // J3D_HOOKFUNC(sound_GetPan);
    // J3D_HOOKFUNC(sound_SetMasterVolume);
    // J3D_HOOKFUNC(sound_GetMasterVolume);
    // J3D_HOOKFUNC(sound_SetRate);
    // J3D_HOOKFUNC(sound_GetRate);
    // J3D_HOOKFUNC(sound_GetPosition);
    // J3D_HOOKFUNC(sound_Control);
    // J3D_HOOKFUNC(sound_Query);
    // J3D_HOOKFUNC(sound_GetVar);
    // J3D_HOOKFUNC(sound_SetVar);
    // J3D_HOOKFUNC(sound_StartVoc);
    // J3D_HOOKFUNC(sound_StartStream);
    // J3D_HOOKFUNC(sound_SyncTimer);
    // J3D_HOOKFUNC(sound_Callback);
    // J3D_HOOKFUNC(sound_Advance);
    // J3D_HOOKFUNC(sound_Copy);
    // J3D_HOOKFUNC(sound_Fill);
}

void Engine_ResetGlobals(void)
{}
