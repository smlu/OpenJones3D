// SmushPlay.c: the glue between the game and the INSANE movie-player library (original 0x004E30B0-0x004E33F0).
#define SMUSHPLAY_IMPLEMENTATION
#include "SmushPlay.h"
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>
#include <string.h>

#include <smush/insane/Animate/Frame.h>
#include <smush/insane/Animate/Pcplay.h>
#include <smush/insane/Bitmap/Bitmap.h>
#include <smush/insane/Bitmap/Blit.h>
#include <smush/insane/Bitmap/BlitAsm.h>
#include <smush/insane/Bitmap/MemCopy.h>
#include <smush/insane/Bitmap/MemFill.h>
#include <smush/insane/Decode/Decode.h>
#include <smush/insane/Decode/DecodeAsm.h>
#include <smush/insane/File/Cdstream.h>
#include <smush/insane/File/Framestream.h>
#include <smush/insane/File/Stdfile.h>
#include <smush/insane/File/Stdio.h>
#include <smush/insane/File/Zfilio/Zfilio.h>
#include <smush/insane/Flobject/Bloc16Asm.h>
#include <smush/insane/Flobject/D_bloc16.h>
#include <smush/insane/Flobject/D_wave.h>
#include <smush/insane/Image/Readauto.h>
#include <smush/insane/Lec/LecShare.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/Dsnd.h>
#include <smush/insane/Sound/Engine.h>
#include <smush/insane/Sound/Mixer.h>
#include <smush/insane/Sound/MixerAsm.h>
#include <smush/insane/Timer/Timer.h>
#include <smush/insane/Util/List.h>

void SmushPlay_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(SmushPlay_SysStartup);
    // J3D_HOOKFUNC(SmushPlay_PlayMovie);
    // J3D_HOOKFUNC(smushplay_Blit);
    // J3D_HOOKFUNC(SmushPlay_SysShutdown);
    // J3D_HOOKFUNC(SmushPlay_SetGlobalVolume);
    // J3D_HOOKFUNC(smushplay_GetHwnd);
}

void SmushPlay_ResetGlobals(void)
{}

void Smush_InstallHooks(void)
{
    SmushPlay_InstallHooks();
    Frame_InstallHooks();
    Pcplay_InstallHooks();
    Bitmap_InstallHooks();
    Blit_InstallHooks();
    BlitAsm_InstallHooks();
    MemCopy_InstallHooks();
    MemFill_InstallHooks();
    Decode_InstallHooks();
    DecodeAsm_InstallHooks();
    Cdstream_InstallHooks();
    Framestream_InstallHooks();
    Stdfile_InstallHooks();
    Stdio_InstallHooks();
    Zfilio_InstallHooks();
    Bloc16Asm_InstallHooks();
    D_bloc16_InstallHooks();
    D_wave_InstallHooks();
    Readauto_InstallHooks();
    LecShare_InstallHooks();
    Memory_InstallHooks();
    Dsnd_InstallHooks();
    Engine_InstallHooks();
    Mixer_InstallHooks();
    MixerAsm_InstallHooks();
    Timer_InstallHooks();
    List_InstallHooks();
}

void Smush_ResetGlobals(void)
{
    SmushPlay_ResetGlobals();
    Frame_ResetGlobals();
    Pcplay_ResetGlobals();
    Bitmap_ResetGlobals();
    Blit_ResetGlobals();
    BlitAsm_ResetGlobals();
    MemCopy_ResetGlobals();
    MemFill_ResetGlobals();
    Decode_ResetGlobals();
    DecodeAsm_ResetGlobals();
    Cdstream_ResetGlobals();
    Framestream_ResetGlobals();
    Stdfile_ResetGlobals();
    Stdio_ResetGlobals();
    Zfilio_ResetGlobals();
    Bloc16Asm_ResetGlobals();
    D_bloc16_ResetGlobals();
    D_wave_ResetGlobals();
    Readauto_ResetGlobals();
    LecShare_ResetGlobals();
    Memory_ResetGlobals();
    Dsnd_ResetGlobals();
    Engine_ResetGlobals();
    Mixer_ResetGlobals();
    MixerAsm_ResetGlobals();
    Timer_ResetGlobals();
    List_ResetGlobals();

    // zfilio_pFilio is the only INSANE datum that lives in the executable (read by its zlib)
#ifndef J3D_LOCAL_RTI
    memset((char*)&zfilio_pFilio, 0, sizeof(zfilio_pFilio));
#endif
}
