#ifndef SOUND_RTI_ADDRESSES_H
#define SOUND_RTI_ADDRESSES_H
#include <j3dcore/j3d.h>
#include <sound/types.h>

// Function addresses

#define SoundDriver_Open_ADDR 0x004DDBC0
#define SoundDriver_Close_ADDR 0x004DDC60
#define SoundDriver_SetMaxVolume_ADDR 0x004DDC90
#define SoundDriver_CreateAndPlay_ADDR 0x004DDCA0
#define SoundDriver_Release_ADDR 0x004DE280
#define SoundDriver_Play_ADDR 0x004DE290
#define SoundDriver_Stop_ADDR 0x004DE310
#define SoundDriver_SetVolume_ADDR 0x004DE330
#define SoundDriver_GetVolume_ADDR 0x004DE360
#define SoundDriver_SetPan_ADDR 0x004DE3A0
#define SoundDriver_SetFrequency_ADDR 0x004DE3D0
#define SoundDriver_GetFrequency_ADDR 0x004DE400
#define SoundDriver_GetCurrentPosition_ADDR 0x004DE440
#define SoundDriver_GetStatusAndCaps_ADDR 0x004DE480
#define SoundDriver_SetListenerPosition_ADDR 0x004DE530
#define SoundDriver_ListenerCommitDeferred_ADDR 0x004DE650
#define SoundDriver_SetPosAndVelocity_ADDR 0x004DE680
#define SoundDriver_Update3DSound_ADDR 0x004DE790
#define SoundDriver_SetGlobals_ADDR 0x004DEA10
#define SoundDriver_Use3DCaps_ADDR 0x004DEAD0
#define SoundDriver_GetDSound_ADDR 0x004DEAE0
#define SoundDriver_CreateDirectSound_ADDR 0x004DEAF0
#define SoundDriver_ReleaseDirectSound_ADDR 0x004DEBF0
#define SoundDriver_CreateListener_ADDR 0x004DEC10
#define SoundDriver_ReleaseListener_ADDR 0x004DECC0
#define SoundDriver_SetOuputFormat_ADDR 0x004DECE0
#define SoundDriver_GetPrimaryDSBuffer_ADDR 0x004DED80
#define SoundDriver_ReleasePrimaryDSBuffer_ADDR 0x004DEE20
#define SoundDriver_GetDecibelVolume_ADDR 0x004DEE40
#define SoundDriver_GetIntPan_ADDR 0x004DEE70
#define SoundDriver_GetVolumeFromDecibels_ADDR 0x004DEEA0
#define SoundDriver_InitPanTable_ADDR 0x004DEEE0
#define SoundDriver_DSCheckStatus_ADDR 0x004DEF10
#define SoundDriver_CalcListeneSoundMix_ADDR 0x004DEF60
#define SoundDriver_CalcDistToListener_ADDR 0x004DF190
#define SoundDriver_RestoreSoundBufferData_ADDR 0x004DF220
#define SoundDriver_Sleep_ADDR 0x004DF480
#define AudioLib_ParseWaveFileHeader_ADDR 0x004DF490
#define AudioLib_Compress_ADDR 0x004DF930
#define AudioLib_ResetCompressor_ADDR 0x004DFA20
#define AudioLib_Uncompress_ADDR 0x004DFA40
#define AudioLib_GetMouthPosition_ADDR 0x004DFB20
#define AudioLib_GenerateLipSynchBlock_ADDR 0x004DFBF0
#define AudioLib_CompressBlock_ADDR 0x004E0000
#define AudioLib_UncompressBlock_ADDR 0x004E02F0
#define AudioLib_WVSMCompressBlock_ADDR 0x004E0590
#define AudioLib_WVSMUncompressBlock_ADDR 0x004E08F0
#define Sound_Initialize_ADDR 0x004DA240
#define Sound_Uninitialize_ADDR 0x004DA280
#define Sound_Startup_ADDR 0x004DA2A0
#define Sound_Shutdown_ADDR 0x004DA2F0
#define Sound_Open_ADDR 0x004DA310
#define Sound_Close_ADDR 0x004DA440
#define Sound_Save_ADDR 0x004DA4A0
#define Sound_Restore_ADDR 0x004DA8D0
#define Sound_Pause_ADDR 0x004DB0B0
#define Sound_Resume_ADDR 0x004DB130
#define Sound_Set3DGlobals_ADDR 0x004DB1E0
#define Sound_Has3DHW_ADDR 0x004DB220
#define Sound_Set3DHWState_ADDR 0x004DB250
#define Sound_Get3DHWState_ADDR 0x004DB360
#define Sound_GetMemoryUsage_ADDR 0x004DB3A0
#define Sound_Load_ADDR 0x004DB3D0
#define Sound_LoadStatic_ADDR 0x004DB860
#define Sound_Reset_ADDR 0x004DB8A0
#define Sound_EnableLoad_ADDR 0x004DBA20
#define Sound_ExportStaticBank_ADDR 0x004DBA30
#define Sound_ImportStaticBank_ADDR 0x004DBA40
#define Sound_ExportNormalBank_ADDR 0x004DBA50
#define Sound_ImportNormalBank_ADDR 0x004DBA60
#define Sound_SkipSoundFileSection_ADDR 0x004DBA70
#define Sound_GetSoundHandle_ADDR 0x004DBB00
#define Sound_GetSoundIndex_ADDR 0x004DBB50
#define Sound_GetChannelHandle_ADDR 0x004DBBA0
#define Sound_GetChannelGUID_ADDR 0x004DBBF0
#define Sound_GetSoundFilename_ADDR 0x004DBC40
#define Sound_GetLengthMsec_ADDR 0x004DBC80
#define Sound_SetMaxVolume_ADDR 0x004DBD20
#define Sound_GetMaxVolume_ADDR 0x004DBDC0
#define Sound_Play_ADDR 0x004DBDD0
#define Sound_PlayPos_ADDR 0x004DC0B0
#define Sound_PlayThing_ADDR 0x004DC270
#define Sound_StopChannel_ADDR 0x004DC4E0
#define Sound_StopAllSounds_ADDR 0x004DC540
#define Sound_StopThing_ADDR 0x004DC590
#define Sound_SetVolume_ADDR 0x004DC650
#define Sound_FadeVolume_ADDR 0x004DC6C0
#define Sound_SetVolumeThing_ADDR 0x004DC800
#define Sound_FadeVolumeThing_ADDR 0x004DC8C0
#define Sound_IsThingFadingVol_ADDR 0x004DC990
#define Sound_SetPitch_ADDR 0x004DCA50
#define Sound_GetPitch_ADDR 0x004DCA80
#define Sound_FadePitch_ADDR 0x004DCAA0
#define Sound_SetPitchThing_ADDR 0x004DCBD0
#define Sound_GetChannelFlags_ADDR 0x004DCC90
#define Sound_Update_ADDR 0x004DCCB0
#define Sound_GenerateLipSync_ADDR 0x004DD180
#define Sound_SetReverseSound_ADDR 0x004DD3D0
#define Sound_GenerateSoundHandle_ADDR 0x004DD3E0
#define Sound_GenerateChannelHandle_ADDR 0x004DD420
#define Sound_GetFreeCache_ADDR 0x004DD460
#define Sound_IncreaseFreeCache_ADDR 0x004DD530
#define Sound_WriteSoundFilepathToBank_ADDR 0x004DD560
#define Sound_GetDeltaTime_ADDR 0x004DD5A0
#define Sound_GetSoundInfo_ADDR 0x004DD5B0
#define Sound_GetChannel_ADDR 0x004DD610
#define Sound_GetChannelBySoundHandle_ADDR 0x004DD660
#define Sound_GetSoundBufferData_ADDR 0x004DD6B0
#define Sound_MemFileRead_ADDR 0x004DD740
#define Sound_MemFileWrite_ADDR 0x004DD7C0
#define Sound_MemFileReset_ADDR 0x004DD860
#define Sound_MemFileFree_ADDR 0x004DD870
#define Sound_StopAllNonStaticSounds_ADDR 0x004DD8B0
#define Sound_ExportBank_ADDR 0x004DD910
#define Sound_ImportBank_ADDR 0x004DD9E0
#define Sound_ReadFile_ADDR 0x004DDB60

// Variable addresses

#define SoundDriver_aDBTable_ADDR 0x00509C80
#define SoundDriver_aDStatusTbl_ADDR 0x0050A080
#define SoundDriver_maxVolume_ADDR 0x0054BB08
#define SoundDriver_minDistance_ADDR 0x0054BB10
#define SoundDriver_maxDistance_ADDR 0x0054BB14
#define SoundDriver_defaultRolloffFactor_ADDR 0x0054BB18
#define SoundDriver_defaultDopplerFactor_ADDR 0x0054BB1C
#define SoundDriver_pfLogError_ADDR 0x014E4460
#define SoundDriver_aPanTable_ADDR 0x014E4468
#define SoundDriver_frontOrientATan_ADDR 0x014E4868
#define SoundDriver_bNoDSound3D_ADDR 0x014E486C
#define SoundDriver_bUseGlobalFocusBuf_ADDR 0x014E4870
#define SoundDriver_hwnd_ADDR 0x014E4874
#define SoundDriver_bDSoundNotCreated_ADDR 0x014E4878
#define SoundDriver_pfCalcListenerSoundMix_ADDR 0x014E487C
#define SoundDriver_pfGetSoundBufferData_ADDR 0x014E4880
#define SoundDriver_pDirectSound_ADDR 0x014E4884
#define SoundDriver_pDSBuffer_ADDR 0x014E4888
#define SoundDriver_pDS3DListener_ADDR 0x014E488C
#define SoundDriver_curListenerPos_ADDR 0x014E4890
#define SoundDriver_bUse3DCaps_ADDR 0x014E489C
#define SoundDriver_bHas3DHW_ADDR 0x014E48A0
#define AudioLib_aStepTable_ADDR 0x0050A2A0
#define AudioLib_aStepBits_ADDR 0x0050A358
#define AudioLib_aIndexTableTable_ADDR 0x0054BBD0
#define AudioLib_aDeltaTable_ADDR 0x014E48A8
#define AudioLib_word_14E4928_ADDR 0x014E4928
#define AudioLib_bDeltaTableInitialized_ADDR 0x014E7528
#define Sound_cacheBlockSize_ADDR 0x0054B434
#define Sound_maxVolume_ADDR 0x0054B438
#define SoundDriver_defaultDistanceFactor_ADDR 0x0054BB0C
#define Sound_pfGetThingInfoCallback_ADDR 0x014E1718
#define Sound_aFades_ADDR 0x014E1720
#define Sound_nextHandle_ADDR 0x014E1BA0
#define Sound_loadBuffer_ADDR 0x014E1BA8
#define Sound_bNoSound3D_ADDR 0x014E42B8
#define Sound_hwnd_ADDR 0x014E42BC
#define Sound_pDirectSound_ADDR 0x014E42C0
#define Sound_pfCalcListenerSoundMix_ADDR 0x014E42C4
#define Sound_pausedRefCount_ADDR 0x014E42C8
#define Sound_bNoLipSync_ADDR 0x014E42CC
#define Sound_msecPauseStartTime_ADDR 0x014E42D0
#define Sound_msecElapsed_ADDR 0x014E42D4
#define Sound_bGlobalFocusBuf_ADDR 0x014E42D8
#define Sound_sndFilename_ADDR 0x014E42E0
#define Sound_bNoSoundCompression_ADDR 0x014E43E0
#define Sound_maxChannels_ADDR 0x014E43E4
#define Sound_bLoadEnabled_ADDR 0x014E43E8
#define soundbank_apSoundCache_ADDR 0x014E43F0
#define soundbank_aUsedCacheSizes_ADDR 0x014E43F8
#define soundbank_aCacheSizes_ADDR 0x014E4400
#define soundbank_lastImportedBankNum_ADDR 0x014E4408
#define soundbank_apSoundInfos_ADDR 0x014E4410
#define soundbank_aNumSounds_ADDR 0x014E4418
#define soundbank_aSizeSounds_ADDR 0x014E4420
#define Sound_apChannels_ADDR 0x014E4428
#define Sound_numChannels_ADDR 0x014E442C
#define Sound_sizeChannels_ADDR 0x014E4430
#define Sound_pMemfileBuf_ADDR 0x014E4434
#define Sound_memfileSize_ADDR 0x014E4438
#define Sound_memfilePos_ADDR 0x014E443C
#define Sound_pHS_ADDR 0x014E4440
#define Sound_state_ADDR 0x014E4444
#define Sound_bReverseSound_ADDR 0x014E4448
#define Sound_curUpdatePos_ADDR 0x014E4450

#endif // SOUND_RTI_ADDRESSES_H
