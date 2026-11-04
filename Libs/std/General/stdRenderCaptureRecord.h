#ifndef STD_STDRENDERCAPTURE_RECORD_H
#define STD_STDRENDERCAPTURE_RECORD_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

// Added: M6.0 render capture record ids reserved by the writer. Game records use any other u16.
#define STDRENDERCAPTURE_RECORD_BLOB   0xFFF0
#define STDRENDERCAPTURE_RECORD_GLOBAL 0xFFF1
#define STDRENDERCAPTURE_RECORD_FRAMES 0xFFF2

/**
 * @brief Handle kinds the capture assigns stable ids to. Per kind, ids start at 1, count up and are
 * never reused within a capture, even after release. They reset on every Open.
 */
typedef enum eStdRenderCaptureHandleKind
{
    STDRENDERCAPTURE_HANDLE_VBUFFER = 0,
    STDRENDERCAPTURE_HANDLE_SYSTEXTURE = 1,
} StdRenderCaptureHandleKind;

/**
 * @brief Start buffering a record payload in memory. When a capture is open but a record is already
 * open, logs an error and discards the old one. A no-op when nothing is open. Logs nothing else.
 */
void stdRenderCapture_BeginRecord(uint16_t id);

// Append to the open record's payload. All are no-ops when nothing is open and log an error when a
// capture is open without an open record. PutBool writes a u8 0/1, PutF32 the IEEE-754 bits as a
// u32, PutString a u32 length followed by the bytes without a NUL (NULL writes only the u32
// 0xFFFFFFFF), PutBytes raw bytes without a length prefix. A payload over UINT32_MAX bytes logs an
// error, drops the record and closes the capture.

void stdRenderCapture_PutU8(uint8_t value);
void stdRenderCapture_PutBool(bool value);
void stdRenderCapture_PutU16(uint16_t value);
void stdRenderCapture_PutU32(uint32_t value);
void stdRenderCapture_PutI32(int32_t value);
void stdRenderCapture_PutU64(uint64_t value);
void stdRenderCapture_PutF32(float value);
void stdRenderCapture_PutString(const char* pStr);
void stdRenderCapture_PutBytes(const void* pData, size_t size);

/**
 * @brief Write the buffered record: the 8-byte header (id, reserved 0, real payload size) followed
 * by the payload. Returns true when the record was written. No open record or closed capture logs an
 * error and returns false (the error log is only for an open capture without an open record); a
 * failed write logs an error and closes the capture. A no-op returning false when nothing is open.
 */
bool stdRenderCapture_EndRecord(void);

/**
 * @brief Write a BLOB record for the given data and return its id, or 0 when nothing was written.
 *
 * Blob ids start at 1, count up per capture and are never reused. NULL data or size 0 returns 0 and
 * writes nothing. Identical content (same size and bytes) returns the existing id without writing;
 * the writer verifies a hash match by reading the stored bytes back. Must be called outside a
 * record: inside one it logs an error and returns 0. A failed write, seek or read logs an error and
 * closes the capture. A no-op returning 0 when nothing is open.
 */
uint32_t stdRenderCapture_WriteBlob(const void* pData, size_t size);

/**
 * @brief Write a GLOBAL record (globalId 1..8) unless valueBits equal the last value written for
 * that id in this capture; the first call for an id always writes. Returns true when it wrote or had
 * nothing to write. Inside a record it logs an error and returns false, as does an id outside 1..8;
 * a failed write logs an error and closes the capture. A no-op returning false when nothing is open.
 */
bool stdRenderCapture_WriteGlobal(uint32_t globalId, uint32_t valueBits);

/**
 * @brief Return the live capture handle of p for this kind, or 0 when p is NULL or has no live
 * handle. A no-op returning 0 when nothing is open. Logs nothing.
 */
uint32_t stdRenderCapture_FindHandle(StdRenderCaptureHandleKind kind, const void* p);

/**
 * @brief Return the live handle of p for this kind, assigning the next free number when p has none.
 * Returns 0 when p is NULL. Handles count up per kind, are never reused within a capture and are
 * assigned in call order, never from pointer values. A no-op returning 0 when nothing is open.
 * Logs nothing.
 */
uint32_t stdRenderCapture_AddHandle(StdRenderCaptureHandleKind kind, const void* p);

/**
 * @brief Remove p from this kind's live set, so a later AddHandle assigns it a new handle. The
 * table holds live entries only and keeps them in assignment order. A no-op when p is NULL or has
 * no live handle. A no-op when nothing is open. Logs nothing.
 */
void stdRenderCapture_ReleaseHandle(StdRenderCaptureHandleKind kind, const void* p);

/**
 * @brief Return the handle of the first live entry of this kind, in assignment order, for which
 * pfMatch returns true; pContext is passed through to every call. Returns 0 when none matches,
 * when nothing is open, or when pfMatch is NULL. May be called while a record is open (it writes
 * nothing). Logs nothing (an invalid kind logs an error like the other handle calls).
 */
uint32_t stdRenderCapture_FindHandleWhere(
    StdRenderCaptureHandleKind kind,
    bool (*pfMatch)(const void* p, const void* pContext),
    const void* pContext
);

#ifdef J3D_TEST
/**
 * @brief Test seam: replace the blob content hash (64-bit FNV-1a by default) to force collisions.
 * NULL restores FNV-1a. Only affects which stored blobs are read back for verification, never the
 * bytes written. Call when no capture is open; stdRenderCapture_ResetGlobals restores FNV-1a.
 */
void stdRenderCapture_SetHashFuncForTest(uint64_t (*pfHash)(const void* pData, size_t size));
#endif

J3D_EXTERN_C_END
#endif // STD_STDRENDERCAPTURE_RECORD_H
