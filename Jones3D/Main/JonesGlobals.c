// Added: The retail-global reset sequence from Jones3D/Platform/Win32/WinMain.c
// ResetGlobals, split around the module resets that only exist on Windows
// (SoundDriver, JonesDialog, jonesConfig, stdComm, stdConsole, stdControl, stdWin95,
// wkernel, wuRegistry) so WinMain.c can keep the original Windows call order while the
// native Null platform resets every global it actually has. See JonesGlobals.h.
#include <Jones3D/types.h> // Should be included first due to Windows.h / CommCtrl.h conflict

#include <j3dcore/j3d.h>

#include <Jones3D/Display/JonesConsole.h>
#include <Jones3D/Display/JonesDisplay.h>
#include <Jones3D/Display/JonesHud.h>
#include <Jones3D/Main/JonesFile.h>
#include <Jones3D/Main/JonesGlobals.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Main/jonesString.h>
#include <Jones3D/Play/jonesCog.h>
#include <Jones3D/Play/JonesControl.h>
#include <Jones3D/Play/jonesInventory.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdCanvas.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdKeyframe.h>
#include <rdroid/Engine/rdLight.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Engine/rdQClip.h>
#include <rdroid/Engine/rdThing.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdFont.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Primitives/rdParticle.h>
#include <rdroid/Primitives/rdPolyline.h>
#include <rdroid/Primitives/rdPrimit2.h>
#include <rdroid/Primitives/rdPrimit3.h>
#include <rdroid/Primitives/rdSprite.h>
#include <rdroid/Primitives/rdWallpaper.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/Raster/rdFace.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIAwareness.h>
#include <sith/AI/sithAIClass.h>
#include <sith/AI/sithAIInstinct.h>
#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Cog/sithCogFunction.h>
#include <sith/Cog/sithCogFunctionAI.h>
#include <sith/Cog/sithCogFunctionPlayer.h>
#include <sith/Cog/sithCogFunctionSector.h>
#include <sith/Cog/sithCogFunctionSound.h>
#include <sith/Cog/sithCogFunctionSurface.h>
#include <sith/Cog/sithCogFunctionThing.h>
#include <sith/Cog/sithCogParse.h>
#include <sith/Cog/sithCogYacc.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSCog.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Dss/sithGamesave.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/Engine/sithParticle.h>
#include <sith/Engine/sithPathMove.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Engine/sithRender.h>
#include <sith/Engine/sithRenderSky.h>
#include <sith/Engine/sithShadow.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/Main/sithCommand.h>
#include <sith/Main/sithMain.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithExplosion.h>
#include <sith/World/sithItem.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithPVS.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSprite.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <smush/SmushPlay.h>

#include <sound/AudioLib.h>
#include <sound/Sound.h>

#include <std/General/std.h>
#include <std/General/stdBmp.h>
#include <std/General/stdCircBuf.h>
#include <std/General/stdColor.h>
#include <std/General/stdConffile.h>
#include <std/General/stdEffect.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdFnames.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdLinkList.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdStrTable.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdGob.h>

void Jones3D_ResetGlobalsBeforeSoundDriver(void)
{
    AudioLib_ResetGlobals();
}

void Jones3D_ResetGlobalsBeforeJonesDialog(void)
{
    JonesConsole_ResetGlobals();
    JonesControl_ResetGlobals();
}

void Jones3D_ResetGlobalsBeforeJonesConfig(void)
{
    JonesDisplay_ResetGlobals();
    JonesFile_ResetGlobals();
    JonesHud_ResetGlobals();
    JonesMain_ResetGlobals();
    Smush_ResetGlobals();
    Sound_ResetGlobals();
    jonesCog_ResetGlobals();
}

void Jones3D_ResetGlobalsBeforeStdComm(void)
{
    jonesInventory_ResetGlobals();
    jonesString_ResetGlobals();
    rdCache_ResetGlobals();
    rdCamera_ResetGlobals();
    rdCanvas_ResetGlobals();
    rdClip_ResetGlobals();
    rdFace_ResetGlobals();
    rdFont_ResetGlobals();
    rdKeyframe_ResetGlobals();
    rdLight_ResetGlobals();
    rdMaterial_ResetGlobals();
    rdMath_ResetGlobals();
    rdMatrix_ResetGlobals();
    rdModel3_ResetGlobals();
    rdParticle_ResetGlobals();
    rdPolyline_ResetGlobals();
    rdPrimit2_ResetGlobals();
    rdPrimit3_ResetGlobals();
    rdPuppet_ResetGlobals();
    rdQClip_ResetGlobals();
    rdSprite_ResetGlobals();
    rdThing_ResetGlobals();
    rdVector_ResetGlobals();
    rdWallpaper_ResetGlobals();
    rdroid_ResetGlobals();
    sithAIAwareness_ResetGlobals();
    sithAIClass_ResetGlobals();
    sithAIInstinct_ResetGlobals();
    sithAIMove_ResetGlobals();
    sithAIUtil_ResetGlobals();
    sithAI_ResetGlobals();
    sithActor_ResetGlobals();
    sithAnimate_ResetGlobals();
    sithCamera_ResetGlobals();
    sithCogExec_ResetGlobals();
    sithCogFunctionAI_ResetGlobals();
    sithCogFunctionPlayer_ResetGlobals();
    sithCogFunctionSector_ResetGlobals();
    sithCogFunctionSound_ResetGlobals();
    sithCogFunctionSurface_ResetGlobals();
    sithCogFunctionThing_ResetGlobals();
    sithCogFunction_ResetGlobals();
    sithCogParse_ResetGlobals();
    sithCogYacc_ResetGlobals();
    sithCog_ResetGlobals();
    sithCollision_ResetGlobals();
    sithComm_ResetGlobals();
    sithCommand_ResetGlobals();
    sithConsole_ResetGlobals();
    sithControl_ResetGlobals();
    sithDSSCog_ResetGlobals();
    sithDSSThing_ResetGlobals();
    sithDSS_ResetGlobals();
    sithEvent_ResetGlobals();
    sithExplosion_ResetGlobals();
    sithFX_ResetGlobals();
    sithGamesave_ResetGlobals();
    sithIntersect_ResetGlobals();
    sithInventory_ResetGlobals();
    sithItem_ResetGlobals();
    sithMain_ResetGlobals();
    sithMaterial_ResetGlobals();
    sithModel_ResetGlobals();
    sithMulti_ResetGlobals();
    sithOverlayMap_ResetGlobals();
    sithPVS_ResetGlobals();
    sithParticle_ResetGlobals();
    sithPathMove_ResetGlobals();
    sithPhysics_ResetGlobals();
    sithPlayerActions_ResetGlobals();
    sithPlayerControls_ResetGlobals();
    sithPlayer_ResetGlobals();
    sithPuppet_ResetGlobals();
    sithRender_ResetGlobals();
    sithRenderSky_ResetGlobals();
    sithSector_ResetGlobals();
    sithShadow_ResetGlobals();
    sithSoundClass_ResetGlobals();
    sithSoundMixer_ResetGlobals();
    sithSound_ResetGlobals();
    sithSprite_ResetGlobals();
    sithString_ResetGlobals();
    sithSurface_ResetGlobals();
    sithTemplate_ResetGlobals();
    sithThing_ResetGlobals();
    sithTime_ResetGlobals();
    sithVehicleControls_ResetGlobals();
    sithVoice_ResetGlobals();
    sithWeapon_ResetGlobals();
    sithWhip_ResetGlobals();
    sithWorld_ResetGlobals();
    std3D_ResetGlobals();
    stdBmp_ResetGlobals();
    stdCircBuf_ResetGlobals();
    stdColor_ResetGlobals();
}

void Jones3D_ResetGlobalsBeforeStdConsole(void)
{
    stdConffile_ResetGlobals();
}

void Jones3D_ResetGlobalsBeforeStdWin95(void)
{
    stdDisplay_ResetGlobals();
    stdEffect_ResetGlobals();
    stdFileUtil_ResetGlobals();
    stdFnames_ResetGlobals();
    stdGob_ResetGlobals();
    stdHashtbl_ResetGlobals();
    stdLinkList_ResetGlobals();
    stdMath_ResetGlobals();
    stdMemory_ResetGlobals();
    stdPlatform_ResetGlobals();
    stdStrTable_ResetGlobals();
    stdUtil_ResetGlobals();
}

void Jones3D_ResetGlobalsBeforeWkernel(void)
{
    std_ResetGlobals();
}

void Jones3D_ResetAllGlobals(void)
{
    Jones3D_ResetGlobalsBeforeSoundDriver();
    // Altered: the Windows-only SoundDriver, JonesDialog, jonesConfig, stdComm,
    // stdConsole, stdControl and stdWin95 resets are not here; WinMain.c calls them
    // between the runs below in their original positions.
    Jones3D_ResetGlobalsBeforeJonesDialog();
    Jones3D_ResetGlobalsBeforeJonesConfig();
    Jones3D_ResetGlobalsBeforeStdComm();
    Jones3D_ResetGlobalsBeforeStdConsole();
    Jones3D_ResetGlobalsBeforeStdWin95();
    Jones3D_ResetGlobalsBeforeWkernel();
    // Altered: the Windows-only wkernel and wuRegistry resets stay in WinMain.c.
}
