#ifndef RDROID_RDMATRIX_H
#define RDROID_RDMATRIX_H
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>

#include <rdroid/types.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/RTI/addresses.h>

#include <std/General/stdUtil.h>
#include <std/types.h>

J3D_EXTERN_C_START

#if defined(J3D_LOCAL_RDROID_RTI)
extern const rdMatrix34 rdroid_g_identMatrix34;
#else
#   define rdroid_g_identMatrix34 J3D_DECL_FAR_VAR(rdroid_g_identMatrix34, const rdMatrix34)
#endif

static inline void J3DAPI rdMatrix_Identity34(rdMatrix34* mat); // Added
static inline  void J3DAPI rdMatrix_Copy34(rdMatrix34* dest, const rdMatrix34* src);

void J3DAPI rdMatrix_Build34(rdMatrix34* mat, const rdVector3* pyr, const rdVector3* pos);
void J3DAPI rdMatrix_BuildFromLook34(rdMatrix34* mat, const rdVector3* look);
void J3DAPI rdMatrix_InvertOrtho34(rdMatrix34* dest, const rdMatrix34* src);
void J3DAPI rdMatrix_BuildRotate34(rdMatrix34* mat, const rdVector3* pyr);
void J3DAPI rdMatrix_BuildTranslate34(rdMatrix34* mat, const rdVector3* vec);
void J3DAPI rdMatrix_BuildScale34(rdMatrix34* mat, const rdVector3* scale);
void J3DAPI rdMatrix_BuildFromVectorAngle34(rdMatrix34* mat, const rdVector3* vec, float angle);

void J3DAPI rdMatrix_Normalize34(rdMatrix34* mat);

void J3DAPI rdMatrix_LookAt(rdMatrix34* mat, const rdVector3* eyePos, const rdVector3* lookPos, float angle);
void J3DAPI rdMatrix_ExtractAngles34(const rdMatrix34* mat, rdVector3* pyr);

void J3DAPI rdMatrix_Multiply34(rdMatrix34* dest, const rdMatrix34* a, const rdMatrix34* b);
void J3DAPI rdMatrix_PreMultiply34(rdMatrix34* a, const rdMatrix34* b);
void J3DAPI rdMatrix_PostMultiply34(rdMatrix34* a, const rdMatrix34* b);

void J3DAPI rdMatrix_PreRotate34(rdMatrix34* mat, const rdVector3* pyr);
void J3DAPI rdMatrix_PostRotate34(rdMatrix34* mat, const rdVector3* vecPYR);

void J3DAPI rdMatrix_PreTranslate34(rdMatrix34* mat, const rdVector3* vec);
void J3DAPI rdMatrix_PostTranslate34(rdMatrix34* pMat, const rdVector3* pVec);

void J3DAPI rdMatrix_PreScale34(rdMatrix34* mat, const rdVector3* vec);  //Added
void J3DAPI rdMatrix_PostScale34(rdMatrix34* mat, const rdVector3* vec);

void J3DAPI rdMatrix_TransformVector34(rdVector3* dest, const rdVector3* src, const rdMatrix34* mat);
void J3DAPI rdMatrix_TransformVectorOrtho34(rdVector3* dest, const rdVector3* src, const rdMatrix34* mat);
void J3DAPI rdMatrix_TransformVector34Acc(rdVector3* dest, const rdMatrix34* mat);

void J3DAPI rdMatrix_TransformPoint34(rdVector3* dest, const rdVector3* src, const rdMatrix34* mat);
void J3DAPI rdMatrix_TransformPoint34Acc(rdVector3* dest, const rdMatrix34* mat);
void J3DAPI rdMatrix_TransformPointList34(const rdMatrix34* mat, const rdVector3* aSrc, rdVector3* aDest, size_t size);


void J3DAPI rdMatrix_Identity34(rdMatrix34* mat)
{
    STD_COPYMEM(mat, &rdroid_g_identMatrix34, sizeof(rdMatrix34));
}

void J3DAPI rdMatrix_Copy34(rdMatrix34* dest, const rdMatrix34* src)
{
    RD_ASSERT(dest != src); // Added
    STD_COPYMEM(dest, src, sizeof(rdMatrix34));
}

// Helper hooking functions
void rdMatrix_InstallHooks(void);
void rdMatrix_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDMATRIX_H
