#ifndef SITH_SITHDSS_H
#define SITH_SITHDSS_H
#include <j3dcore/j3d.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/types.h>

#include <sith/Dss/sithMulti.h>
#include <sith/types.h>
#include <sith/RTI/addresses.h>

#include <std/types.h>

J3D_EXTERN_C_START

//Helper macros for data serialization / de-serialization

#define SITHDSS_STARTOUT(t) SITHDSS_STARTOUTEX(&sithMulti_g_message, t)
#define SITHDSS_STARTOUTEX(msg, t) SithMessage* pCurDSSMsg_ = msg; pCurDSSMsg_->type = t; uint8_t* pCurDSSMsgData_ = &pCurDSSMsg_->data[0]
#define SITHDSS_ENDOUT (pCurDSSMsg_->length = SITHDSS_CURPOS())

#define SITHDSS_STARTIN(msg) const SithMessage* pCurDSSMsg_ = msg; const uint8_t* pCurDSSMsgData_ = &pCurDSSMsg_->data[0]
#define SITHDSS_ENDIN

#define SITHDSS_CURPOS() ((uintptr_t)pCurDSSMsgData_ - (uintptr_t)&pCurDSSMsg_->data[0])

#define SITHDSS_PUSHINT8(x) sithDSS_WriteInt8(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHUINT8(x) sithDSS_WriteUint8(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHINT16(x) sithDSS_WriteInt16(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHUINT16(x) sithDSS_WriteUint16(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHINT32(x) sithDSS_WriteInt32(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHUINT32(x) sithDSS_WriteUint32(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHFLOAT(x) sithDSS_WriteFloat(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHVEC2(x) sithDSS_WriteVec2(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHVEC3(x) sithDSS_WriteVec3(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHVEC4(x) sithDSS_WriteVec4(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHMAT34(x) sithDSS_WriteMat34(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHSTRING(x, len) sithDSS_WriteString(&pCurDSSMsgData_, (x), (len))
#define SITHDSS_PUSHWSTRING(x, len) sithDSS_WriteWString(&pCurDSSMsgData_, (x), (len))

#define SITHDSS_POPINT8() sithDSS_ReadInt8(&pCurDSSMsgData_)
#define SITHDSS_POPUINT8() sithDSS_ReadUint8(&pCurDSSMsgData_)
#define SITHDSS_POPINT16() sithDSS_ReadInt16(&pCurDSSMsgData_)
#define SITHDSS_POPUINT16() sithDSS_ReadUint16(&pCurDSSMsgData_)
#define SITHDSS_POPINT32() sithDSS_ReadInt32(&pCurDSSMsgData_)
#define SITHDSS_POPUINT32() sithDSS_ReadUint32(&pCurDSSMsgData_)
#define SITHDSS_POPFLOAT() sithDSS_ReadFloat(&pCurDSSMsgData_)
#define SITHDSS_POPVEC2(x) sithDSS_ReadVec2(&pCurDSSMsgData_, (x))
#define SITHDSS_POPVEC3(x) sithDSS_ReadVec3(&pCurDSSMsgData_, (x))
#define SITHDSS_POPVEC4(x) sithDSS_ReadVec4(&pCurDSSMsgData_, (x))
#define SITHDSS_POPMAT34(x) sithDSS_ReadMat34(&pCurDSSMsgData_, (x))
#define SITHDSS_POPSTRING(x, len)  sithDSS_ReadString(&pCurDSSMsgData_, (x), (len))
#define SITHDSS_POPWSTRING(x, len) sithDSS_ReadWString(&pCurDSSMsgData_, (x), (len))


typedef enum eSithDSSType
{
    SITHDSS_POS                 = 1,
    SITHDSS_CHAT                = 2,
    SITHDSS_SECTORFLAGS         = 3,
    SITHDSS_FIRE                = 4,
    SITHDSS_DEATH               = 5,
    SITHDSS_DAMAGE              = 6,
    SITHDSS_SETMODEL            = 7,
    SITHDSS_COGMESSAGE          = 8,
    SITHDSS_PLAYKEY             = 9,
    SITHDSS_PLAYSOUND           = 10,
    SITHDSS_THINGSTATE          = 11,
    SITHDSS_THINGFULLDESC       = 12,
    SITHDSS_COGSTATE            = 13,
    SITHDSS_SURFACESTATUS       = 14,
    SITHDSS_AISTATUS            = 15,
    SITHDSS_INVENTORY           = 16,
    SITHDSS_ANIMSTATUS          = 17,
    SITHDSS_SECTORSTATUS        = 18,
    SITHDSS_PLAYKEYMODE         = 19,
    SITHDSS_PATHMOVE            = 20,
    SITHDSS_PUPPETSTATUS        = 21,
    SITHDSS_ATTACHMENT          = 22,
    SITHDSS_SYNCTASKEVENTS      = 23,
    SITHDSS_SYNCCAMERAS         = 24,
    SITHDSS_TAKE                = 25,
    SITHDSS_TAKE2               = 26,
    SITHDSS_STOPKEY             = 27,
    SITHDSS_STOPSOUND           = 28,
    SITHDSS_CREATETHING         = 29,
    SITHDSS_WHIPSTATUS          = 30,
    SITHDSS_SYNCGAMESTATE       = 31,
    SITHDSS_SYNCPLAYERS         = 32,
    SITHDSS_WELCOME             = 33,
    SITHDSS_JOIN                = 34,
    SITHDSS_DESTROYTHING        = 35,
    SITHDSS_JOINSTATUS          = 36,
    SITHDSS_PLAYSOUNDMODE       = 37,
    SITHDSS_PING                = 38,
    SITHDSS_PONG                = 39,
    SITHDSS_MOVEPOS             = 40,
    SITHDSS_CHECKMEMBERSHIPLIST = 41,
    SITHDSS_QUIT                = 42,
    SITHDSS_UNKNOWN_44          = 44,
    SITHDSS_VOICESTATE          = 45,
    SITHDSS_VEHICLECONTROLSTATE = 46,
    SITHDSS_HUDSTATE            = 47,
    SITHDSS_SYNCWPNT            = 48,
    SITHDSS_SYNCSEENSECTORS     = 49,
} SithDSSType;

int J3DAPI sithDSS_SurfaceStatus(const SithSurface* pSurf, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessSurfaceStatus(const SithMessage* pMsg);

int J3DAPI sithDSS_SectorStatus(const SithSector* pSector, DPID a2, unsigned int outstream);
int J3DAPI sithDSS_SyncSeenSectors(DPID idTo, unsigned int outstream);

int J3DAPI sithDSS_ProcessSyncSeenSecors(const SithMessage* pMsg);
int J3DAPI sithDSS_ProcessSectorStatus(const SithMessage* pMsg);

int J3DAPI sithDSS_SectorFlags(const SithSector* pSector, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessSectorFlags(const SithMessage* pMsg);

int J3DAPI sithDSS_AIStatus(const SithAIControlBlock* pLocal, DPID idTo, signed int outstream);
int J3DAPI sithDSS_ProcessAIStatus(const SithMessage* pMsg);

int J3DAPI sithDSS_Inventory(const SithThing* pThing, unsigned int inventoryId, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessInventory(const SithMessage* pMsg);

int J3DAPI sithDSS_AnimStatus(const SithAnimationSlot* pAnim, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessAnimStatus(const SithMessage* pMsg);

int J3DAPI sithDSS_PuppetStatus(const SithThing* pThing, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessPuppetStatus(const SithMessage* pMsg);

int J3DAPI sithDSS_SyncTaskEvents(const SithEvent* pEvent, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessSyncTaskEvents(const SithMessage* pMsg);

int J3DAPI sithDSS_SyncCameras(DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessSyncCameras(const SithMessage* pMsg);

int J3DAPI sithDSS_SyncGameState(DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessSyncGameState(const SithMessage* pMsg);

int J3DAPI sithDSS_SyncVehicleControlState(DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessVehicleControlsState(const SithMessage* pMsg);

int J3DAPI sithDSS_sub_4B3760(DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_sub_4B3790(const SithMessage* pMsg);

// Functions for de-serializing data from stream
static inline int8_t J3DAPI sithDSS_ReadInt8(uint8_t** stream);
static inline uint8_t J3DAPI sithDSS_ReadUint8(uint8_t** stream);
static inline int16_t J3DAPI sithDSS_ReadInt16(uint8_t** stream);
static inline uint16_t J3DAPI sithDSS_ReadUint16(uint8_t** stream);
static inline int32_t J3DAPI sithDSS_ReadInt32(uint8_t** stream);
static inline uint32_t J3DAPI sithDSS_ReadUint32(uint8_t** stream);
static inline float J3DAPI sithDSS_ReadFloat(uint8_t** stream);
static inline void J3DAPI sithDSS_ReadVec2(uint8_t** stream, rdVector2* vec);
static inline void J3DAPI sithDSS_ReadVec3(uint8_t** stream, rdVector3* vec);
static inline void J3DAPI sithDSS_ReadVec4(uint8_t** stream, rdVector4* vec);
static inline void J3DAPI sithDSS_ReadMat34(uint8_t** stream, rdMatrix34* mat);
static inline void J3DAPI sithDSS_ReadString(uint8_t** stream, char* str, int length);
static inline void J3DAPI sithDSS_ReadWString(uint8_t** stream, wchar_t* str, int length);

// Functions for serializing data to stream
static inline void J3DAPI sithDSS_WriteInt8(uint8_t** stream, int8_t val);
static inline void J3DAPI sithDSS_WriteUint8(uint8_t** stream, uint8_t val);
static inline void J3DAPI sithDSS_WriteInt16(uint8_t** stream, int16_t val);
static inline void J3DAPI sithDSS_WriteUint16(uint8_t** stream, uint16_t val);
static inline void J3DAPI sithDSS_WriteInt32(uint8_t** stream, int32_t val);
static inline void J3DAPI sithDSS_WriteUint32(uint8_t** stream, uint32_t val);
static inline void J3DAPI sithDSS_WriteFloat(uint8_t** stream, float val);
static inline void J3DAPI sithDSS_WriteVec2(uint8_t** stream, const rdVector2* vec);
static inline void J3DAPI sithDSS_WriteVec3(uint8_t** stream, const rdVector3* vec);
static inline void J3DAPI sithDSS_WriteVec4(uint8_t** stream, const rdVector4* vec);
static inline void J3DAPI sithDSS_WriteMat34(uint8_t** stream, const rdMatrix34* mat);
static inline void J3DAPI sithDSS_WriteString(uint8_t** stream, const char* str, int length);
static inline void J3DAPI sithDSS_WriteWString(uint8_t** stream, const wchar_t* str, int length);

// Helper hooking functions
void sithDSS_InstallHooks(void);
void sithDSS_ResetGlobals(void);


// Serialization functions

void sithDSS_WriteInt8(uint8_t** stream, int8_t val)
{
    **stream = val;
    (*stream) += sizeof(int8_t);
}

void sithDSS_WriteUint8(uint8_t** stream, uint8_t val)
{
    **stream = val;
    *stream += sizeof(uint8_t);
}

void sithDSS_WriteInt16(uint8_t** stream, int16_t val)
{
    *(int16_t*)*stream = val;
    *stream += sizeof(int16_t);
}

void sithDSS_WriteUint16(uint8_t** stream, uint16_t val)
{
    *(uint16_t*)*stream = val;
    *stream += sizeof(uint16_t);
}

void sithDSS_WriteInt32(uint8_t** stream, int32_t val)
{
    *(int32_t*)*stream = val;
    *stream += sizeof(int32_t);
}

void sithDSS_WriteUint32(uint8_t** stream, uint32_t val)
{
    *(uint32_t*)*stream = val;
    *stream += sizeof(uint32_t);
}

void sithDSS_WriteFloat(uint8_t** stream, float val)
{
    *(float*)*stream = val;
    *stream += sizeof(float);
}

void sithDSS_WriteVec2(uint8_t** stream, const rdVector2* vec)
{
    rdVector_Copy2((rdVector2*)*stream, vec);
    *stream += sizeof(rdVector2);
}

void sithDSS_WriteVec3(uint8_t** stream, const rdVector3* vec)
{
    rdVector_Copy3((rdVector3*)*stream, vec);
    *stream += sizeof(rdVector3);
}

void sithDSS_WriteVec4(uint8_t** stream, const rdVector4* vec)
{
    rdVector_Copy4((rdVector4*)*stream, vec);
    *stream += sizeof(rdVector4);
}

void sithDSS_WriteMat34(uint8_t** stream, const rdMatrix34* mat)
{
    rdMatrix_Copy34((rdMatrix34*)*stream, mat);
    *stream += sizeof(rdMatrix34);
}

void sithDSS_WriteString(uint8_t** stream, const char* str, int length)
{
    memcpy(*stream, str, length);
    *stream += length;
}

void sithDSS_WriteWString(uint8_t** stream, const wchar_t* str, int length)
{
    memcpy(*stream, str, length * sizeof(wchar_t));
    *stream += length * sizeof(wchar_t);
}

// De-serialization functions

int8_t sithDSS_ReadInt8(uint8_t** stream)
{
    int8_t data = *(int8_t*)*stream;
    *stream += sizeof(int8_t);
    return data;
}

uint8_t sithDSS_ReadUint8(uint8_t** stream)
{
    uint8_t data = **stream;
    *stream += sizeof(uint8_t);
    return data;
}

int16_t sithDSS_ReadInt16(uint8_t** stream)
{
    int16_t data = *(int16_t*)*stream;
    *stream += sizeof(int16_t);
    return data;
}

uint16_t sithDSS_ReadUint16(uint8_t** stream)
{
    uint16_t data = *(uint16_t*)*stream;
    *stream += sizeof(uint16_t);
    return data;
}

int32_t sithDSS_ReadInt32(uint8_t** stream)
{
    int32_t data = *(int32_t*)*stream;
    *stream += sizeof(int32_t);
    return data;
}

uint32_t sithDSS_ReadUint32(uint8_t** stream)
{
    uint32_t data = *(uint32_t*)*stream;
    *stream += sizeof(uint32_t);
    return data;
}

float sithDSS_ReadFloat(uint8_t** stream)
{
    float data = *(float*)*stream;
    *stream += sizeof(float);
    return data;
}

void sithDSS_ReadVec2(uint8_t** stream, rdVector2* vec)
{
    rdVector_Copy2(vec, (rdVector2*)*stream);
    *stream += sizeof(rdVector2);
}

void sithDSS_ReadVec3(uint8_t** stream, rdVector3* vec)
{
    rdVector_Copy3(vec, (rdVector3*)*stream);
    *stream += sizeof(rdVector3);
}

void sithDSS_ReadVec4(uint8_t** stream, rdVector4* vec)
{
    rdVector_Copy4(vec, (rdVector4*)*stream);
    *stream += sizeof(rdVector4);
}

void sithDSS_ReadMat34(uint8_t** stream, rdMatrix34* mat)
{
    rdMatrix_Copy34(mat, (rdMatrix34*)*stream);
    *stream += sizeof(rdMatrix34);
}

void sithDSS_ReadString(uint8_t** stream, char* str, int length)
{
    memcpy(str, *stream, length);
    *stream += length;
}

void sithDSS_ReadWString(uint8_t** stream, wchar_t* str, int length)
{
    memcpy(str, *stream, length * sizeof(wchar_t));
    *stream += length * sizeof(wchar_t);
}

J3D_EXTERN_C_END
#endif // SITH_SITHDSS_H
