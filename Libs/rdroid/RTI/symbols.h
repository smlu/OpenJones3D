#ifndef RDROID_RTI_SYMBOLS_H
#define RDROID_RTI_SYMBOLS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

#define rdWallpaper_New_TYPE rdWallpaper* (J3DAPI*)(const char*)
#define rdWallpaper_Free_TYPE void (J3DAPI*)(rdWallpaper*)
#define rdWallpaper_Draw_TYPE void (J3DAPI*)(const rdWallpaper*)
#define rdWallpaper_NewWallLine_TYPE rdWallLine* (J3DAPI*)(float, float, float, float, const rdVector4*)
#define rdWallpaper_FreeWallLine_TYPE void (J3DAPI*)(rdWallLine*)
#define rdWallpaper_DrawWallLine_TYPE void (J3DAPI*)(const rdWallLine*, float)
#define rdSetServices_TYPE void (J3DAPI*)(tHostServices*)
#define rdClearServices_TYPE void (*)(void)
#define rdStartup_TYPE void (J3DAPI*)()
#define rdShutdown_TYPE void (J3DAPI*)()
#define rdOpen_TYPE int (J3DAPI*)()
#define rdClose_TYPE void (J3DAPI*)()
#define rdSetRenderOptions_TYPE void (J3DAPI*)(rdRoidFlags)
#define rdSetGeometryMode_TYPE void (J3DAPI*)(rdGeometryMode)
#define rdSetLightingMode_TYPE void (J3DAPI*)(rdLightMode)
#define rdGetRenterOptions_TYPE rdRoidFlags (J3DAPI*)()
#define rdModel3_RegisterLoader_TYPE rdModel3LoaderFunc (J3DAPI*)(rdModel3LoaderFunc)
#define rdModel3_NewEntry_TYPE void (J3DAPI*)(rdModel3*)
#define rdModel3_LoadEntry_TYPE int (J3DAPI*)(const char*, rdModel3*)
#define rdModel3_FreeEntry_TYPE void (J3DAPI*)(rdModel3*)
#define rdModel3_FreeEntryGeometryOnly_TYPE void (J3DAPI*)(rdModel3*)
#define rdModel3_BuildExpandedRadius_TYPE void (J3DAPI*)(const rdModel3*, const rdModel3HNode*, const rdMatrix34*)
#define rdModel3_GetMeshMatrix_TYPE int (J3DAPI*)(rdThing*, const rdMatrix34*, unsigned int, rdMatrix34*)
#define rdModel3_ReplaceMesh_TYPE int (J3DAPI*)(rdModel3*, unsigned int, unsigned int, const rdModel3Mesh*)
#define rdModel3_SwapMesh_TYPE int (J3DAPI*)(rdModel3*, unsigned int, rdModel3*, int)
#define rdModel3_GetThingColor_TYPE void (J3DAPI*)(const rdThing*, rdVector4*)
#define rdModel3_SetModelColor_TYPE void (J3DAPI*)(rdModel3*, const rdVector4*)
#define rdModel3_SetThingColor_TYPE void (J3DAPI*)(rdThing*, const rdVector4*)
#define rdModel3_SetNodeColor_TYPE void (J3DAPI*)(const rdModel3*, rdModel3GeoSet*, const rdModel3HNode*, const rdVector4*)
#define rdModel3_SetNodeColorForNonAmputatedJoints_TYPE void (J3DAPI*)(const rdThing*, rdModel3GeoSet*, const rdModel3HNode*, const rdVector4*)
#define rdModel3_Draw_TYPE int (J3DAPI*)(rdThing*, const rdMatrix34*)
#define rdModel3_DrawHNode_TYPE void (J3DAPI*)(const rdModel3GeoSet*, const rdModel3HNode*)
#define rdModel3_DrawMesh_TYPE void (J3DAPI*)(const rdModel3Mesh*, const rdMatrix34*)
#define rdModel3_DrawFace_TYPE void (J3DAPI*)(const rdFace*, const rdVector3*, int, const rdVector4*)
#define rdModel3_EnableFogRendering_TYPE void (J3DAPI*)(int)
#define rdParticle_New_TYPE rdParticle* (J3DAPI*)(size_t, float, rdMaterial*, rdLightMode)
#define rdParticle_NewEntry_TYPE int (J3DAPI*)(rdParticle*, size_t, float, rdMaterial*, rdLightMode)
#define rdParticle_Duplicate_TYPE rdParticle* (J3DAPI*)(const rdParticle*)
#define rdParticle_Free_TYPE void (J3DAPI*)(rdParticle*)
#define rdParticle_FreeEntry_TYPE void (J3DAPI*)(rdParticle*)
#define rdParticle_LoadEntry_TYPE int (J3DAPI*)(const char*, rdParticle*)
#define rdParticle_Draw_TYPE int (J3DAPI*)(const rdThing*, const rdMatrix34*)
#define rdFont_Load_TYPE rdFont* (J3DAPI*)(const char*)
#define rdFont_LoadEntry_TYPE int (J3DAPI*)(const char*, rdFont*)
#define rdFont_LoadMaterial_TYPE int (J3DAPI*)(const char*, rdFont*)
#define rdFont_Free_TYPE void (J3DAPI*)(rdFont*)
#define rdFont_SetFontColor_TYPE void (J3DAPI*)(const rdVector4*)
#define rdFont_GetWrapLine_TYPE const char* (J3DAPI*)(const char*, const rdFont*, float)
#define rdFont_GetTextWidth_TYPE size_t (J3DAPI*)(const char*, const rdFont*)
#define rdFont_DrawTextLine_TYPE void (J3DAPI*)(const char*, float, float, float, const rdFont*, int)
#define rdFont_DrawChar_TYPE void (J3DAPI*)(size_t, float, float, float, const rdFont*)
#define rdFont_DrawTextLineClipped_TYPE void (J3DAPI*)(const char*, float, float, float, const rdFont*, int)
#define rdFont_Startup_TYPE void (J3DAPI*)()
#define rdFont_Shutdown_TYPE void (*)()
#define rdFont_Open_TYPE int (J3DAPI*)()
#define rdFont_Close_TYPE void (J3DAPI*)()
#define rdCamera_New_TYPE rdCamera* (J3DAPI*)(float, int, float, float, float)
#define rdCamera_NewEntry_TYPE int (J3DAPI*)(rdCamera*, float, int, float, float, float)
#define rdCamera_Free_TYPE void (J3DAPI*)(rdCamera*)
#define rdCamera_FreeEntry_TYPE void (J3DAPI*)(rdCamera*)
#define rdCamera_SetCanvas_TYPE void (J3DAPI*)(rdCamera*, rdCanvas*)
#define rdCamera_SetCurrent_TYPE void (J3DAPI*)(rdCamera*)
#define rdCamera_SetFOV_TYPE void (J3DAPI*)(rdCamera*, float)
#define rdCamera_SetProjectType_TYPE void (J3DAPI*)(rdCamera*, rdCameraProjType)
#define rdCamera_SetAspectRatio_TYPE void (J3DAPI*)(rdCamera*, float)
#define rdCamera_BuildFOV_TYPE void (J3DAPI*)(rdCamera*)
#define rdCamera_BuildClipFrustrum_TYPE void (J3DAPI*)(const rdCamera*, rdClipFrustum*, float, float)
#define rdCamera_SetFrustrum_TYPE int (J3DAPI*)(rdCamera*, rdClipFrustum*, int, int, int, int)
#define rdCamera_Update_TYPE void (J3DAPI*)(const rdMatrix34*)
#define rdCamera_OrthoProject_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*)
#define rdCamera_OrthoProjectLst_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*, size_t)
#define rdCamera_OrthoProjectSquare_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*)
#define rdCamera_OrthoProjectSquareLst_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*, size_t)
#define rdCamera_PerspProject_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*)
#define rdCamera_PerspProjectLst_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*, size_t)
#define rdCamera_PerspProjectSquare_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*)
#define rdCamera_PerspProjectSquareLst_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*, size_t)
#define rdCamera_SetAmbientLight_TYPE void (J3DAPI*)(rdCamera*, const rdVector4*)
#define rdCamera_SetAttenuation_TYPE void (J3DAPI*)(rdCamera*, float, float)
#define rdCamera_AddLight_TYPE int (J3DAPI*)(rdCamera*, rdLight*, const rdVector3*)
#define rdCamera_ClearLights_TYPE int (J3DAPI*)(rdCamera*)
#define rdCamera_sub_4C60B0_TYPE void (J3DAPI*)(int)
#define rdCache_Startup_TYPE void (J3DAPI*)()
#define rdCache_AdvanceFrame_TYPE void (*)(void)
#define rdCache_GetFrameNum_TYPE int (*)(void)
#define rdCache_GetProcEntry_TYPE rdCacheProcEntry* (*)(void)
#define rdCache_GetAlphaProcEntry_TYPE rdCacheProcEntry* (*)(void)
#define rdCache_Flush_TYPE void (*)(void)
#define rdCache_FlushAlpha_TYPE void (*)(void)
#define rdCache_AddProcFace_TYPE void (J3DAPI*)(size_t)
#define rdCache_AddAlphaProcFace_TYPE void (J3DAPI*)(size_t)
#define rdCache_SendFaceListToHardware_TYPE void (J3DAPI*)(size_t, rdCacheProcEntry*, rdCacheSortFunc)
#define rdCache_SendWireframeFaceListToHardware_TYPE void (J3DAPI*)(size_t, rdCacheProcEntry*)
#define rdCache_AddToTextureCache_TYPE void (J3DAPI*)(tSystemTexture*, StdColorFormatType)
#define rdCache_ProcFaceDistanceCompare_TYPE int (J3DAPI*)(const rdCacheProcEntry*, const rdCacheProcEntry*)
#define rdCache_ProcFaceCompare_TYPE int (J3DAPI*)(const rdCacheProcEntry*, const rdCacheProcEntry*)
#define rdMaterial_RegisterLoader_TYPE rdMaterialLoaderFunc (J3DAPI*)(rdMaterialLoaderFunc)
#define rdMaterial_Load_TYPE rdMaterial* (J3DAPI*)(const char*)
#define rdMaterial_LoadEntry_TYPE int (J3DAPI*)(const char*, rdMaterial*)
#define rdMaterial_Free_TYPE void (J3DAPI*)(rdMaterial*)
#define rdMaterial_FreeEntry_TYPE void (J3DAPI*)(rdMaterial*)
#define rdMaterial_GetMaterialMemUsage_TYPE size_t (J3DAPI*)(const rdMaterial*)
#define rdMaterial_GetMipSize_TYPE size_t (J3DAPI*)(int, int, size_t)
#define rdPrimit2_DrawClippedLine_TYPE int (J3DAPI*)(rdCanvas*, int, int, int, int, uint32_t, uint32_t)
#define rdPrimit2_Reset_TYPE void (*)(void)
#define rdPrimit2_DrawClippedLine2_TYPE int (J3DAPI*)(float, float, float, float, uint32_t)
#define rdPrimit2_DrawClippedCircle_TYPE void (J3DAPI*)(rdCanvas*, int, int, float, float, uint32_t, uint32_t)
#define rdPrimit2_DrawClippedCircle2_TYPE void (J3DAPI*)(float, float, float, float, uint32_t)
#define rdKeyframe_RegisterLoader_TYPE rdKeyframeLoadFunc (J3DAPI*)(rdKeyframeLoadFunc)
#define rdKeyframe_NewEntry_TYPE void (J3DAPI*)(rdKeyframe*)
#define rdKeyframe_LoadEntry_TYPE int (J3DAPI*)(const char*, rdKeyframe*)
#define rdKeyframe_FreeEntry_TYPE void (J3DAPI*)(rdKeyframe*)
#define rdClip_Line2_TYPE int (J3DAPI*)(const rdCanvas*, int*, int*, int*, int*)
#define rdClip_CalcOutcode2_TYPE rdClipOutcode (J3DAPI*)(const rdCanvas*, int, int)
#define rdClip_Line2Ex_TYPE int (J3DAPI*)(float*, float*, float*, float*)
#define rdClip_CalcOutcode2Ex_TYPE int (J3DAPI*)(float, float, float, float)
#define rdClip_ClipFacePVS_TYPE int (J3DAPI*)(rdClipFrustum*, const rdPrimit3*, rdPrimit3*)
#define rdClip_Face3WPVS_TYPE int (J3DAPI*)(rdClipFrustum*, rdVector3*, size_t)
#define rdClip_Face3W_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, size_t)
#define rdClip_Face3WOrtho_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, size_t)
#define rdClip_Face3S_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, size_t)
#define rdClip_Face3SOrtho_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, size_t)
#define rdClip_Face3GS_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, rdVector4*, size_t)
#define rdClip_Face3GSOrtho_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, rdVector4*, size_t)
#define rdClip_Face3GT_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, rdVector2*, size_t)
#define rdClip_Face3GTOrtho_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, rdVector2*, size_t)
#define rdClip_Face3T_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, rdVector2*, rdVector4*, size_t)
#define rdClip_Face3TOrtho_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, rdVector2*, rdVector4*, size_t)
#define rdClip_SphereInFrustrum_TYPE RdFrustumCull (J3DAPI*)(const rdClipFrustum*, const rdVector3*, float)
#define rdClip_QFace3W_TYPE void (J3DAPI*)(const rdClipFrustum*, const rdPrimit3*, rdPrimit3*)
#define rdClip_FaceToPlane_TYPE int (J3DAPI*)(const rdClipFrustum*, rdCacheProcEntry*, const rdFace*, const rdVector3*, const rdVector2*, const rdVector4*, const rdVector4*)
#define rdClip_VerticesToPlane_TYPE void (J3DAPI*)(rdCacheProcEntry*, const rdVector3*, const rdVector2*, size_t)
#define rdPolyline_New_TYPE rdPolyline* (J3DAPI*)(const char*, const char*, const char*, float, float, float, rdGeometryMode, rdLightMode, const rdVector4*)
#define rdPolyline_NewEntry_TYPE int (J3DAPI*)(rdPolyline*, const char*, const char*, float, float, float, rdGeometryMode, rdLightMode, const rdVector4*)
#define rdPolyline_Free_TYPE void (J3DAPI*)(rdPolyline*)
#define rdPolyline_FreeEntry_TYPE void (J3DAPI*)(rdPolyline*)
#define rdPolyline_Draw_TYPE int (J3DAPI*)(const rdThing*, const rdMatrix34*)
#define rdPolyline_DrawFace_TYPE void (J3DAPI*)(const rdThing*, const rdFace*, const rdVector3*, const rdVector2*)
#define rdSprite_NewEntry_TYPE int (J3DAPI*)(rdSprite3*, const char*, int, const char*, float, float, rdGeometryMode, rdLightMode, const rdVector4*, const rdVector3*)
#define rdSprite_FreeEntry_TYPE void (J3DAPI*)(rdSprite3*)
#define rdSprite_Draw_TYPE int (J3DAPI*)(rdThing*, const rdMatrix34*)
#define rdQClip_VerticesInFrustrum_TYPE size_t (J3DAPI*)(const rdClipFrustum*, const rdVector3*, size_t)
#define rdQClip_Face3W_TYPE int (J3DAPI*)(const rdClipFrustum*, rdVector3*, size_t)
#define rdCanvas_New_TYPE rdCanvas* (J3DAPI*)(int, tVBuffer*, int, int, int, int)
#define rdCanvas_NewEntry_TYPE int (J3DAPI*)(rdCanvas*, int, tVBuffer*, int, int, int, int)
#define rdCanvas_Free_TYPE void (J3DAPI*)(rdCanvas*)
#define rdMatrix_Build34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*, const rdVector3*)
#define rdMatrix_BuildFromLook34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*)
#define rdMatrix_InvertOrtho34_TYPE void (J3DAPI*)(rdMatrix34*, const rdMatrix34*)
#define rdMatrix_BuildRotate34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*)
#define rdMatrix_BuildTranslate34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*)
#define rdMatrix_BuildScale34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*)
#define rdMatrix_BuildFromVectorAngle34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*, float)
#define rdMatrix_LookAt_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*, const rdVector3*, float)
#define rdMatrix_ExtractAngles34_TYPE void (J3DAPI*)(const rdMatrix34*, rdVector3*)
#define rdMatrix_Normalize34_TYPE void (J3DAPI*)(rdMatrix34*)
#define rdMatrix_Copy34_TYPE void (J3DAPI*)(rdMatrix34*, const rdMatrix34*)
#define rdMatrix_Multiply34_TYPE void (J3DAPI*)(rdMatrix34*, const rdMatrix34*, const rdMatrix34*)
#define rdMatrix_PreMultiply34_TYPE void (J3DAPI*)(rdMatrix34*, const rdMatrix34*)
#define rdMatrix_PostMultiply34_TYPE void (J3DAPI*)(rdMatrix34*, const rdMatrix34*)
#define rdMatrix_PreRotate34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*)
#define rdMatrix_PostRotate34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*)
#define rdMatrix_PreTranslate34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*)
#define rdMatrix_PostTranslate34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*)
#define rdMatrix_PostScale34_TYPE void (J3DAPI*)(rdMatrix34*, const rdVector3*)
#define rdMatrix_TransformVector34_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*, const rdMatrix34*)
#define rdMatrix_TransformVectorOrtho34_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*, const rdMatrix34*)
#define rdMatrix_TransformVector34Acc_TYPE void (J3DAPI*)(rdVector3*, const rdMatrix34*)
#define rdMatrix_TransformPoint34_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*, const rdMatrix34*)
#define rdMatrix_TransformPoint34Acc_TYPE void (J3DAPI*)(rdVector3*, const rdMatrix34*)
#define rdMatrix_TransformPointList34_TYPE void (J3DAPI*)(const rdMatrix34*, const rdVector3*, rdVector3*, size_t)
#define rdThing_New_TYPE rdThing* (J3DAPI*)(SithThing*)
#define rdThing_NewEntry_TYPE void (J3DAPI*)(rdThing*, SithThing*)
#define rdThing_Free_TYPE void (J3DAPI*)(rdThing*)
#define rdThing_FreeEntry_TYPE void (J3DAPI*)(rdThing*)
#define rdThing_SetModel3_TYPE int (J3DAPI*)(rdThing*, rdModel3*)
#define rdThing_SetSprite3_TYPE int (J3DAPI*)(rdThing*, rdSprite3*)
#define rdThing_SetPolyline_TYPE int (J3DAPI*)(rdThing*, rdPolyline*)
#define rdThing_SetParticleCloud_TYPE int (J3DAPI*)(rdThing*, rdParticle*)
#define rdThing_Draw_TYPE int (J3DAPI*)(rdThing*, const rdMatrix34*)
#define rdThing_AccumulateMatrices_TYPE void (J3DAPI*)(rdThing*, const rdModel3HNode*, const rdMatrix34*)
#define rdVector_Set3_TYPE void (J3DAPI*)(rdVector3*, float, float, float)
#define rdVector_Set4_TYPE void (J3DAPI*)(rdVector4*, float, float, float, float)
#define rdVector_Neg3_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*)
#define rdVector_Add3Acc_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*)
#define rdVector_Dot3_TYPE float (J3DAPI*)(const rdVector3*, const rdVector3*)
#define rdVector_Len3_TYPE float (J3DAPI*)(const rdVector3*)
#define rdVector_Normalize3_TYPE float (J3DAPI*)(rdVector3*, const rdVector3*)
#define rdVector_Normalize2Acc_TYPE float (J3DAPI*)(rdVector2*)
#define rdVector_Normalize3Acc_TYPE float (J3DAPI*)(rdVector3*)
#define rdVector_Normalize3QuickAcc_TYPE float (J3DAPI*)(rdVector3*)
#define rdVector_InvScale3_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*, float)
#define rdVector_Rotate3_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*, const rdVector3*)
#define rdVector_Rotate3Acc_TYPE void (J3DAPI*)(rdVector3*, const rdVector3*)
#define rdPuppet_New_TYPE rdPuppet* (J3DAPI*)(rdThing*)
#define rdPuppet_NewEntry_TYPE void (J3DAPI*)(rdPuppet*, rdThing*)
#define rdPuppet_Free_TYPE void (J3DAPI*)(rdPuppet*)
#define rdPuppet_AddTrack_TYPE int (J3DAPI*)(rdPuppet*, rdKeyframe*, int, int)
#define rdPuppet_RemoveTrack_TYPE void (J3DAPI*)(rdPuppet*, int32_t)
#define rdPuppet_SetStatus_TYPE int (J3DAPI*)(rdPuppet*, int32_t, rdPuppetTrackStatus)
#define rdPuppet_SetCallback_TYPE void (J3DAPI*)(rdPuppet*, int32_t, rdPuppetTrackCallback)
#define rdPuppet_PlayTrack_TYPE int (J3DAPI*)(rdPuppet*, int32_t)
#define rdPuppet_FadeInTrack_TYPE int (J3DAPI*)(rdPuppet*, int32_t, float)
#define rdPuppet_FadeOutTrack_TYPE int (J3DAPI*)(rdPuppet*, int32_t, float)
#define rdPuppet_SetTrackSpeed_TYPE void (J3DAPI*)(rdPuppet*, int32_t, float)
#define rdPuppet_AdvanceTrack_TYPE void (J3DAPI*)(rdPuppet*, int32_t, float)
#define rdPuppet_UpdateTracks_TYPE int (J3DAPI*)(rdPuppet*, float)
#define rdPuppet_ResetTrack_TYPE void (J3DAPI*)(rdPuppet*, int32_t)
#define rdPuppet_BuildJointMatrices_TYPE void (J3DAPI*)(rdThing*, const rdMatrix34*)
#define rdMath_DistancePointToPlane_TYPE float (J3DAPI*)(const rdVector3*, const rdVector3*, const rdVector3*)
#define rdMath_DeltaAngleNormalized_TYPE float (J3DAPI*)(const rdVector3*, const rdVector3*, const rdVector3*)
#define rdPrimit3_DrawClippedCircle_TYPE void (J3DAPI*)(const rdVector3*, float, float, uint32_t, uint32_t)
#define rdPrimit3_ClipFace_TYPE void (J3DAPI*)(const rdClipFrustum*, rdGeometryMode, rdLightMode, const rdPrimit3*, rdPrimit3*, const rdVector2*)
#define rdLight_NewEntry_TYPE int (J3DAPI*)(rdLight*)
#define rdLight_CalcVertexIntensities_TYPE void (J3DAPI*)(const rdLight**, const rdVector3*, size_t, const rdVector3*, const rdVector3*, const rdVector4*, rdVector4*, size_t)
#define rdLight_CalcFaceIntensity_TYPE void (J3DAPI*)(const rdLight**, const rdVector3*, size_t, const rdFace*, const rdVector3*, const rdVector3*, float, rdVector4*)
#define rdLight_GetIntensity_TYPE float (J3DAPI*)(const rdVector4*)
#define rdFace_NewEntry_TYPE int (J3DAPI*)(rdFace*)
#define rdFace_FreeEntry_TYPE void (J3DAPI*)(rdFace*)

#endif // RDROID_RTI_SYMBOLS_H
