// sithAIUtil.c as tested by the sithAIUtil suite of sithTests: compiled with the redirected engine globals of the suite (see
// Libs/sith/Tests/Vec/sith_vec.cmake) and with access to its file statics.
#include "../sithAIUtil.c"

#include "sithAIUtilVecStatics.h"

void sithAIUtilVecTest_GetStatics(void** apOut)
{
    apOut[SITHAIUTILVEC_ST_WPNTS]        = sithAIUtil_aAIWpnts;
    apOut[SITHAIUTILVEC_ST_OWNERS]       = sithAIUtil_aWpntOwners;
    apOut[SITHAIUTILVEC_ST_DISTANCES]    = sithAIUtil_aWpntDistances;
    apOut[SITHAIUTILVEC_ST_LAYER]        = &sithAIUtil_activeWpntLayer;
    apOut[SITHAIUTILVEC_ST_SECLASTUPDATE] = &sithAIUtil_secLastUpdate;
    apOut[SITHAIUTILVEC_ST_MKFLAGS]      = &sithAIUtil_mkPointCurFlags;
    apOut[SITHAIUTILVEC_ST_MKLOCAL]      = &sithAIUtil_pMkPointCurLocal;
    apOut[SITHAIUTILVEC_ST_MKDIR]        = &sithAIUtil_mkPointCurDir;
    apOut[SITHAIUTILVEC_ST_MKPYR]        = &sithAIUtil_mkPointCurPYR;
    apOut[SITHAIUTILVEC_ST_MKRVEC]       = &sithAIUtil_mkPointCurRVec;
    apOut[SITHAIUTILVEC_ST_FOVX]         = &sithAIUtil_thingsInViewFovX;
    apOut[SITHAIUTILVEC_ST_FOVY]         = &sithAIUtil_thingsInViewFovY;
    apOut[SITHAIUTILVEC_ST_NUMVISITED]   = &sithAIUtil_numVisitedSectors;
    apOut[SITHAIUTILVEC_ST_NUMINVIEW]    = &sithAIUtil_numThingsInView;
    apOut[SITHAIUTILVEC_ST_SIZEINVIEW]   = &sithAIUtil_sizeThingsInView;
    apOut[SITHAIUTILVEC_ST_THINGSINVIEW] = &sithAIUtil_aThingsInView;
    apOut[SITHAIUTILVEC_ST_MAXDIST]      = &sithAIUtil_maxDistanceToThingsInView;
    apOut[SITHAIUTILVEC_ST_TYPEMASK]     = &sithAIUtil_thingInViewTypeMask;
}
