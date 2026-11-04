// Added: M6.0 render capture file writer (format RCAP v1). Writes the capture header and records
// through the std host file services; integers are packed little-endian byte by byte, never by
// writing a struct, so the bytes depend only on the sequence of calls and their data.

#include <limits.h>
#include <stdio.h> // Added: SEEK_SET, the host pFileSeek origin convention
#include <string.h>

#include "std.h"
#include "stdMemory.h"
#include "stdRenderCapture.h"
#include "stdRenderCaptureRecord.h"
#include "stdUtil.h"

#define STDRENDERCAPTURE_VERSION         1u
#define STDRENDERCAPTURE_MAXCOLUMNLEN    64u
#define STDRENDERCAPTURE_MAXGLOBALID     8u
#define STDRENDERCAPTURE_RECORDHEADERSIZE 8u
#define STDRENDERCAPTURE_BLOBPREFIXSIZE  12u // u32 blobId + u64 size
#define STDRENDERCAPTURE_READCHUNKSIZE   65536u
#define STDRENDERCAPTURE_STRINGNULLLEN   0xFFFFFFFFu
#define STDRENDERCAPTURE_FNV1AOFFSET     0xcbf29ce484222325ull
#define STDRENDERCAPTURE_FNV1APRIME      0x100000001b3ull
#define STDRENDERCAPTURE_NUMKINDS        2

static const uint8_t stdRenderCapture_aSignature[8] = { 'J', '3', 'D', 'R', 'C', 'A', 'P', 0 };

typedef struct sStdRenderCaptureBlob
{
    uint64_t hash;      // content hash key
    size_t size;        // content size, part of the key
    uint64_t dataOffset; // file offset of the first content byte
    uint32_t blobId;
} StdRenderCaptureBlob;

// One live handle. Released entries leave the table, so the arrays hold live pointers only.
typedef struct sStdRenderCaptureHandleEntry
{
    const void* pPtr;
    uint32_t handle;
} StdRenderCaptureHandleEntry;

static bool stdRenderCapture_bOpen;
static tFileHandle stdRenderCapture_fileHandle;
// Added: the file services of the Open, kept so a game that swaps the host file services
// mid-run (JonesFile does) cannot change the functions used for this capture's handle.
static tHostServices stdRenderCapture_fileServices;
static char* stdRenderCapture_pPath; // owned copy, so blob verification can reopen the file for reading
static uint64_t stdRenderCapture_filePos; // logical end-of-file, tracked byte by byte
static bool stdRenderCapture_bFramesEnabled = true;

static bool stdRenderCapture_bRecordOpen;
static uint16_t stdRenderCapture_recordId;
static uint8_t* stdRenderCapture_pRecordBuf;
static size_t stdRenderCapture_recordSize;
static size_t stdRenderCapture_recordCapacity;

static uint32_t stdRenderCapture_nextBlobId = 1;
static StdRenderCaptureBlob* stdRenderCapture_pBlobs;
static size_t stdRenderCapture_numBlobs;
static size_t stdRenderCapture_blobCapacity;

static uint32_t stdRenderCapture_aGlobalBits[STDRENDERCAPTURE_MAXGLOBALID];
static bool stdRenderCapture_aGlobalWritten[STDRENDERCAPTURE_MAXGLOBALID];

static StdRenderCaptureHandleEntry* stdRenderCapture_apHandleEntries[STDRENDERCAPTURE_NUMKINDS];
static size_t stdRenderCapture_aHandleCounts[STDRENDERCAPTURE_NUMKINDS];
static size_t stdRenderCapture_aHandleCapacities[STDRENDERCAPTURE_NUMKINDS];
static uint32_t stdRenderCapture_aNextHandles[STDRENDERCAPTURE_NUMKINDS];

static uint64_t (*stdRenderCapture_pfHashFunc)(const void* pData, size_t size);

static bool stdRenderCapture_IsValidKind(StdRenderCaptureHandleKind kind)
{
    return kind == STDRENDERCAPTURE_HANDLE_VBUFFER || kind == STDRENDERCAPTURE_HANDLE_SYSTEXTURE;
}

// stdMemory asserts on a NULL free unless the runtime guards are on, so every free checks first.
static void stdRenderCapture_Free(void** ppData)
{
    if ( *ppData )
    {
        STDFREE(*ppData);
        *ppData = NULL;
    }
}

static uint64_t stdRenderCapture_HashFnv1a(const void* pData, size_t size)
{
    const uint8_t* pBytes = (const uint8_t*)pData;
    uint64_t hash = STDRENDERCAPTURE_FNV1AOFFSET;
    size_t i;

    for ( i = 0; i < size; ++i )
    {
        hash ^= pBytes[i];
        hash *= STDRENDERCAPTURE_FNV1APRIME;
    }

    return hash;
}

static uint64_t stdRenderCapture_HashBytes(const void* pData, size_t size)
{
    return stdRenderCapture_pfHashFunc
        ? stdRenderCapture_pfHashFunc(pData, size)
        : stdRenderCapture_HashFnv1a(pData, size);
}

static void stdRenderCapture_PackU32(uint8_t* pDst, uint32_t value)
{
    pDst[0] = (uint8_t)value;
    pDst[1] = (uint8_t)(value >> 8);
    pDst[2] = (uint8_t)(value >> 16);
    pDst[3] = (uint8_t)(value >> 24);
}

static void stdRenderCapture_PackU64(uint8_t* pDst, uint64_t value)
{
    stdRenderCapture_PackU32(pDst, (uint32_t)value);
    stdRenderCapture_PackU32(pDst + 4, (uint32_t)(value >> 32));
}

// Writes bytes at the current end of the capture file and advances the logical position. Returns
// false when the write came up short; the caller logs and closes.
static bool stdRenderCapture_WriteData(const void* pData, size_t size)
{
    if ( size != stdRenderCapture_fileServices.pFileWrite(stdRenderCapture_fileHandle, pData, size) )
    {
        return false;
    }

    stdRenderCapture_filePos += size;
    return true;
}

static bool stdRenderCapture_WriteRecordHeader(uint16_t id, uint32_t payloadSize)
{
    uint8_t aHeader[STDRENDERCAPTURE_RECORDHEADERSIZE];

    aHeader[0] = (uint8_t)id;
    aHeader[1] = (uint8_t)(id >> 8);
    aHeader[2] = 0;
    aHeader[3] = 0;
    stdRenderCapture_PackU32(&aHeader[4], payloadSize);
    return stdRenderCapture_WriteData(aHeader, sizeof(aHeader));
}

static bool stdRenderCapture_WriteFramesRecord(bool bEnabled)
{
    uint8_t aPayload[1];

    aPayload[0] = bEnabled ? 1u : 0u;
    return stdRenderCapture_WriteRecordHeader(STDRENDERCAPTURE_RECORD_FRAMES, sizeof(aPayload))
        && stdRenderCapture_WriteData(aPayload, sizeof(aPayload));
}

// Frees all per-capture state; the module is closed afterwards. Keeps the frames-enabled flag and
// the test hash hook, which are not per-capture state.
static void stdRenderCapture_ResetCaptureState(void)
{
    int kind;

    stdRenderCapture_bRecordOpen = false;
    stdRenderCapture_recordId    = 0;
    stdRenderCapture_recordSize  = 0;
    stdRenderCapture_Free((void**)&stdRenderCapture_pRecordBuf);
    stdRenderCapture_recordCapacity = 0;

    stdRenderCapture_Free((void**)&stdRenderCapture_pPath);

    stdRenderCapture_Free((void**)&stdRenderCapture_pBlobs);
    stdRenderCapture_numBlobs     = 0;
    stdRenderCapture_blobCapacity = 0;
    stdRenderCapture_nextBlobId   = 1;
    stdRenderCapture_filePos      = 0;

    STD_ZEROMEM(stdRenderCapture_aGlobalBits, sizeof(stdRenderCapture_aGlobalBits));
    STD_ZEROMEM(stdRenderCapture_aGlobalWritten, sizeof(stdRenderCapture_aGlobalWritten));

    for ( kind = 0; kind < STDRENDERCAPTURE_NUMKINDS; ++kind )
    {
        stdRenderCapture_Free((void**)&stdRenderCapture_apHandleEntries[kind]);
        stdRenderCapture_aHandleCounts[kind]     = 0;
        stdRenderCapture_aHandleCapacities[kind] = 0;
        stdRenderCapture_aNextHandles[kind]      = 1;
    }
}

static void stdRenderCapture_CloseInternal(void)
{
    if ( stdRenderCapture_bOpen )
    {
        stdRenderCapture_fileServices.pFileClose(stdRenderCapture_fileHandle);
    }

    stdRenderCapture_fileHandle = 0;
    stdRenderCapture_bOpen      = false;
    stdRenderCapture_ResetCaptureState();
}

// One error log, then close and free: any failed write, seek or read leaves the module closed.
static void stdRenderCapture_FailAndClose(const char* pMessage)
{
    STDLOG_ERROR("Render capture: %s\n", pMessage);
    stdRenderCapture_CloseInternal();
}

// Drops an open record and closes the capture (payload over UINT32_MAX bytes or a failed grow).
static void stdRenderCapture_DropRecordAndClose(const char* pMessage)
{
    stdRenderCapture_bRecordOpen = false;
    stdRenderCapture_FailAndClose(pMessage);
}

static bool stdRenderCapture_ReserveRecord(size_t needed)
{
    size_t capacity = stdRenderCapture_recordCapacity ? stdRenderCapture_recordCapacity : 256u;
    void* pBuf;

    if ( needed <= stdRenderCapture_recordCapacity )
    {
        return true;
    }

    while ( capacity < needed )
    {
        if ( capacity > SIZE_MAX / 2u )
        {
            capacity = needed;
            break;
        }

        capacity *= 2u;
    }

    pBuf = STDREALLOC(stdRenderCapture_pRecordBuf, capacity);
    if ( !pBuf )
    {
        return false;
    }

    stdRenderCapture_pRecordBuf     = (uint8_t*)pBuf;
    stdRenderCapture_recordCapacity = capacity;
    return true;
}

static void stdRenderCapture_Append(const void* pData, size_t size)
{
    if ( size > (size_t)UINT32_MAX - stdRenderCapture_recordSize )
    {
        stdRenderCapture_DropRecordAndClose("a record payload exceeded 0xFFFFFFFF bytes.");
        return;
    }

    if ( !stdRenderCapture_ReserveRecord(stdRenderCapture_recordSize + size) )
    {
        stdRenderCapture_DropRecordAndClose("could not grow the record payload buffer.");
        return;
    }

    memcpy(&stdRenderCapture_pRecordBuf[stdRenderCapture_recordSize], pData, size);
    stdRenderCapture_recordSize += size;
}

static void stdRenderCapture_AppendU8(uint8_t value)
{
    stdRenderCapture_Append(&value, sizeof(value));
}

static void stdRenderCapture_AppendU32(uint32_t value)
{
    uint8_t aPacked[4];

    stdRenderCapture_PackU32(aPacked, value);
    stdRenderCapture_Append(aPacked, sizeof(aPacked));
}

static void stdRenderCapture_AppendU64(uint64_t value)
{
    uint8_t aPacked[8];

    stdRenderCapture_PackU64(aPacked, value);
    stdRenderCapture_Append(aPacked, sizeof(aPacked));
}

// The guard every buffered Put shares: silent no-op when nothing is open, error log when a capture
// is open without an open record.
static bool stdRenderCapture_PreparePut(const char* pFuncName)
{
    if ( !stdRenderCapture_bOpen )
    {
        return false;
    }

    if ( !stdRenderCapture_bRecordOpen )
    {
        STDLOG_ERROR("Render capture: %s called with no open record.\n", pFuncName);
        return false;
    }

    return true;
}

// Returns 1 when the stored bytes equal the candidate content, 0 when they do not (including an
// offset too far to seek back to). The capture file's write handle is not readable on every
// platform (the Babylon file adapter opens "w+b" write-only and refuses reads), so the comparison
// reopens the capture path for reading; the reposition flushes the buffered bytes of the write
// handle first, which is what makes the freshly appended data visible to the second reader. The
// write handle's position never moves, so appends continue at the end. -1 means the capture closed.
static int stdRenderCapture_CompareStoredBlob(const StdRenderCaptureBlob* pEntry, const void* pCandidate)
{
    const uint8_t* pCmp = (const uint8_t*)pCandidate;
    tFileHandle readHandle;
    uint8_t* pChunk;
    size_t remaining = pEntry->size;
    int bMatch = 1;

    if ( pEntry->dataOffset > (uint64_t)INT_MAX )
    {
        return 0; // pFileSeek takes an int offset: too far to verify is treated as no match
    }

    if ( stdRenderCapture_fileServices.pFileSeek(stdRenderCapture_fileHandle, 0, SEEK_CUR) )
    {
        stdRenderCapture_FailAndClose("could not flush the capture for a blob comparison.");
        return -1;
    }

    readHandle = stdRenderCapture_fileServices.pFileOpen(stdRenderCapture_pPath, "rb");
    if ( !readHandle )
    {
        stdRenderCapture_FailAndClose("could not reopen the capture to verify a blob.");
        return -1;
    }

    if ( stdRenderCapture_fileServices.pFileSeek(readHandle, (int)pEntry->dataOffset, SEEK_SET) )
    {
        stdRenderCapture_fileServices.pFileClose(readHandle);
        stdRenderCapture_FailAndClose("could not seek to a stored blob.");
        return -1;
    }

    pChunk = (uint8_t*)STDMALLOC(STDRENDERCAPTURE_READCHUNKSIZE);
    if ( !pChunk )
    {
        STDLOG_WARNING("Render capture: no memory to verify a stored blob; writing a duplicate.\n");
        bMatch = 0;
    }

    while ( bMatch && remaining )
    {
        size_t chunkSize = remaining > STDRENDERCAPTURE_READCHUNKSIZE
            ? STDRENDERCAPTURE_READCHUNKSIZE
            : remaining;

        if ( stdRenderCapture_fileServices.pFileRead(readHandle, pChunk, chunkSize) != chunkSize )
        {
            STDFREE(pChunk);
            stdRenderCapture_fileServices.pFileClose(readHandle);
            stdRenderCapture_FailAndClose("could not read back a stored blob.");
            return -1;
        }

        if ( memcmp(pChunk, pCmp, chunkSize) != 0 )
        {
            bMatch = 0;
            break;
        }

        pCmp += chunkSize;
        remaining -= chunkSize;
    }

    if ( pChunk )
    {
        STDFREE(pChunk);
    }

    stdRenderCapture_fileServices.pFileClose(readHandle);
    return bMatch;
}

bool stdRenderCapture_Open(const char* pPath, const char* pColumn)
{
    uint8_t aHeader[16];
    char* pPathCopy;
    size_t pathLen;
    size_t columnLen;

    if ( stdRenderCapture_bOpen )
    {
        STDLOG_ERROR("Render capture: stdRenderCapture_Open called while a capture is already open.\n");
        return false;
    }

    if ( !pPath || !pColumn )
    {
        STDLOG_ERROR("Render capture: stdRenderCapture_Open called with a NULL argument.\n");
        return false;
    }

    columnLen = strlen(pColumn);
    if ( columnLen == 0 || columnLen > STDRENDERCAPTURE_MAXCOLUMNLEN )
    {
        STDLOG_ERROR(
            "Render capture: stdRenderCapture_Open got a column name of %llu bytes.\n",
            (unsigned long long)columnLen
        );
        return false;
    }

    pathLen = strlen(pPath) + 1u;
    pPathCopy = (char*)STDMALLOC(pathLen);
    if ( !pPathCopy )
    {
        STDLOG_ERROR("Render capture: could not allocate the capture path.\n");
        return false;
    }

    memcpy(pPathCopy, pPath, pathLen);

    stdRenderCapture_fileServices = *std_g_pHS;
    stdRenderCapture_fileHandle = stdRenderCapture_fileServices.pFileOpen(pPath, "w+b");
    if ( !stdRenderCapture_fileHandle )
    {
        STDFREE(pPathCopy);
        STDLOG_ERROR("Render capture: could not open '%s'.\n", pPath);
        return false;
    }

    stdRenderCapture_ResetCaptureState();
    stdRenderCapture_pPath = pPathCopy;
    stdRenderCapture_bOpen = true;
    memcpy(aHeader, stdRenderCapture_aSignature, sizeof(stdRenderCapture_aSignature));
    stdRenderCapture_PackU32(&aHeader[8], STDRENDERCAPTURE_VERSION);
    stdRenderCapture_PackU32(&aHeader[12], (uint32_t)columnLen);

    if ( !stdRenderCapture_WriteData(aHeader, sizeof(aHeader))
        || !stdRenderCapture_WriteData(pColumn, columnLen) )
    {
        stdRenderCapture_FailAndClose("could not write the capture header.");
        return false;
    }

    if ( !stdRenderCapture_bFramesEnabled && !stdRenderCapture_WriteFramesRecord(false) )
    {
        stdRenderCapture_FailAndClose("could not write the initial FRAMES record.");
        return false;
    }

    return true;
}

void stdRenderCapture_Close(void)
{
    stdRenderCapture_CloseInternal();
}

bool stdRenderCapture_IsOpen(void)
{
    return stdRenderCapture_bOpen;
}

void stdRenderCapture_SetFramesEnabled(bool bEnabled)
{
    if ( bEnabled == stdRenderCapture_bFramesEnabled )
    {
        return;
    }

    stdRenderCapture_bFramesEnabled = bEnabled;
    if ( stdRenderCapture_bOpen && !stdRenderCapture_WriteFramesRecord(bEnabled) )
    {
        stdRenderCapture_FailAndClose("could not write a FRAMES record.");
    }
}

bool stdRenderCapture_GetFramesEnabled(void)
{
    return stdRenderCapture_bFramesEnabled;
}

void stdRenderCapture_ResetGlobals(void)
{
    stdRenderCapture_CloseInternal();
    stdRenderCapture_bFramesEnabled = true;
    stdRenderCapture_pfHashFunc     = NULL;
}

#ifdef J3D_TEST
void stdRenderCapture_SetHashFuncForTest(uint64_t (*pfHash)(const void* pData, size_t size))
{
    stdRenderCapture_pfHashFunc = pfHash;
}
#endif

void stdRenderCapture_BeginRecord(uint16_t id)
{
    if ( !stdRenderCapture_bOpen )
    {
        return;
    }

    if ( stdRenderCapture_bRecordOpen )
    {
        STDLOG_ERROR("Render capture: stdRenderCapture_BeginRecord discards an open record.\n");
    }

    stdRenderCapture_recordId    = id;
    stdRenderCapture_recordSize  = 0;
    stdRenderCapture_bRecordOpen = true;
}

void stdRenderCapture_PutU8(uint8_t value)
{
    if ( stdRenderCapture_PreparePut("stdRenderCapture_PutU8") )
    {
        stdRenderCapture_AppendU8(value);
    }
}

void stdRenderCapture_PutBool(bool value)
{
    if ( stdRenderCapture_PreparePut("stdRenderCapture_PutBool") )
    {
        stdRenderCapture_AppendU8(value ? 1u : 0u);
    }
}

void stdRenderCapture_PutU16(uint16_t value)
{
    uint8_t aPacked[2];

    if ( !stdRenderCapture_PreparePut("stdRenderCapture_PutU16") )
    {
        return;
    }

    aPacked[0] = (uint8_t)value;
    aPacked[1] = (uint8_t)(value >> 8);
    stdRenderCapture_Append(aPacked, sizeof(aPacked));
}

void stdRenderCapture_PutU32(uint32_t value)
{
    if ( stdRenderCapture_PreparePut("stdRenderCapture_PutU32") )
    {
        stdRenderCapture_AppendU32(value);
    }
}

void stdRenderCapture_PutI32(int32_t value)
{
    if ( stdRenderCapture_PreparePut("stdRenderCapture_PutI32") )
    {
        stdRenderCapture_AppendU32((uint32_t)value);
    }
}

void stdRenderCapture_PutU64(uint64_t value)
{
    if ( stdRenderCapture_PreparePut("stdRenderCapture_PutU64") )
    {
        stdRenderCapture_AppendU64(value);
    }
}

void stdRenderCapture_PutF32(float value)
{
    uint32_t bits;

    if ( !stdRenderCapture_PreparePut("stdRenderCapture_PutF32") )
    {
        return;
    }

    memcpy(&bits, &value, sizeof(bits));
    stdRenderCapture_AppendU32(bits);
}

void stdRenderCapture_PutString(const char* pStr)
{
    size_t len;

    if ( !stdRenderCapture_PreparePut("stdRenderCapture_PutString") )
    {
        return;
    }

    if ( !pStr )
    {
        stdRenderCapture_AppendU32(STDRENDERCAPTURE_STRINGNULLLEN);
        return;
    }

    len = strlen(pStr);
    if ( len > (size_t)UINT32_MAX )
    {
        stdRenderCapture_DropRecordAndClose("a record string exceeded 0xFFFFFFFF bytes.");
        return;
    }

    stdRenderCapture_AppendU32((uint32_t)len);
    if ( stdRenderCapture_bRecordOpen )
    {
        stdRenderCapture_Append(pStr, len);
    }
}

void stdRenderCapture_PutBytes(const void* pData, size_t size)
{
    if ( !stdRenderCapture_PreparePut("stdRenderCapture_PutBytes") )
    {
        return;
    }

    if ( !pData )
    {
        STDLOG_ERROR("Render capture: stdRenderCapture_PutBytes got a NULL pointer.\n");
        return;
    }

    stdRenderCapture_Append(pData, size);
}

bool stdRenderCapture_EndRecord(void)
{
    if ( !stdRenderCapture_bOpen )
    {
        return false;
    }

    if ( !stdRenderCapture_bRecordOpen )
    {
        STDLOG_ERROR("Render capture: stdRenderCapture_EndRecord called with no open record.\n");
        return false;
    }

    stdRenderCapture_bRecordOpen = false;
    if ( !stdRenderCapture_WriteRecordHeader(stdRenderCapture_recordId, (uint32_t)stdRenderCapture_recordSize)
        || ( stdRenderCapture_recordSize > 0 && !stdRenderCapture_WriteData(stdRenderCapture_pRecordBuf, stdRenderCapture_recordSize) ) )
    {
        stdRenderCapture_FailAndClose("could not write a record.");
        return false;
    }

    stdRenderCapture_recordSize = 0;
    return true;
}

uint32_t stdRenderCapture_WriteBlob(const void* pData, size_t size)
{
    StdRenderCaptureBlob* pBlob;
    uint64_t hash;
    uint32_t blobId;
    size_t i;
    int cmp;

    if ( !stdRenderCapture_bOpen )
    {
        return 0;
    }

    if ( stdRenderCapture_bRecordOpen )
    {
        STDLOG_ERROR("Render capture: stdRenderCapture_WriteBlob called inside an open record.\n");
        return 0;
    }

    if ( !pData || size == 0 )
    {
        return 0;
    }

    if ( size > (size_t)UINT32_MAX - STDRENDERCAPTURE_BLOBPREFIXSIZE )
    {
        stdRenderCapture_FailAndClose("a blob record payload exceeded 0xFFFFFFFF bytes.");
        return 0;
    }

    hash = stdRenderCapture_HashBytes(pData, size);
    for ( i = 0; i < stdRenderCapture_numBlobs; ++i )
    {
        if ( stdRenderCapture_pBlobs[i].hash != hash || stdRenderCapture_pBlobs[i].size != size )
        {
            continue;
        }

        cmp = stdRenderCapture_CompareStoredBlob(&stdRenderCapture_pBlobs[i], pData);
        if ( cmp < 0 )
        {
            return 0;
        }

        if ( cmp > 0 )
        {
            return stdRenderCapture_pBlobs[i].blobId;
        }
    }

    if ( stdRenderCapture_numBlobs == stdRenderCapture_blobCapacity )
    {
        size_t capacity = stdRenderCapture_blobCapacity;

        if ( capacity > SIZE_MAX / 2u )
        {
            stdRenderCapture_FailAndClose("too many blobs for one capture.");
            return 0;
        }

        capacity = capacity ? capacity * 2u : 8u;
        if ( capacity > SIZE_MAX / sizeof(StdRenderCaptureBlob) )
        {
            stdRenderCapture_FailAndClose("too many blobs for one capture.");
            return 0;
        }

        pBlob = (StdRenderCaptureBlob*)STDREALLOC(stdRenderCapture_pBlobs, capacity * sizeof(StdRenderCaptureBlob));
        if ( !pBlob )
        {
            stdRenderCapture_FailAndClose("could not grow the blob index.");
            return 0;
        }

        stdRenderCapture_pBlobs       = pBlob;
        stdRenderCapture_blobCapacity = capacity;
    }

    pBlob  = &stdRenderCapture_pBlobs[stdRenderCapture_numBlobs];
    blobId = stdRenderCapture_nextBlobId;

    if ( !stdRenderCapture_WriteRecordHeader(STDRENDERCAPTURE_RECORD_BLOB, (uint32_t)(STDRENDERCAPTURE_BLOBPREFIXSIZE + size)) )
    {
        stdRenderCapture_FailAndClose("could not write a BLOB record header.");
        return 0;
    }

    pBlob->hash       = hash;
    pBlob->size       = size;
    pBlob->dataOffset = stdRenderCapture_filePos + STDRENDERCAPTURE_BLOBPREFIXSIZE;
    pBlob->blobId     = blobId;

    {
        uint8_t aPrefix[STDRENDERCAPTURE_BLOBPREFIXSIZE];

        stdRenderCapture_PackU32(aPrefix, blobId);
        stdRenderCapture_PackU64(&aPrefix[4], size);
        if ( !stdRenderCapture_WriteData(aPrefix, sizeof(aPrefix)) )
        {
            stdRenderCapture_FailAndClose("could not write a BLOB record prefix.");
            return 0;
        }
    }

    if ( !stdRenderCapture_WriteData(pData, size) )
    {
        stdRenderCapture_FailAndClose("could not write blob data.");
        return 0;
    }

    ++stdRenderCapture_numBlobs;
    ++stdRenderCapture_nextBlobId;
    return blobId;
}

bool stdRenderCapture_WriteGlobal(uint32_t globalId, uint32_t valueBits)
{
    uint8_t aPayload[8];
    size_t i;

    if ( !stdRenderCapture_bOpen )
    {
        return false;
    }

    if ( stdRenderCapture_bRecordOpen )
    {
        STDLOG_ERROR("Render capture: stdRenderCapture_WriteGlobal called inside an open record.\n");
        return false;
    }

    if ( globalId < 1u || globalId > STDRENDERCAPTURE_MAXGLOBALID )
    {
        STDLOG_ERROR("Render capture: stdRenderCapture_WriteGlobal got global id %u.\n", globalId);
        return false;
    }

    i = globalId - 1u;
    if ( stdRenderCapture_aGlobalWritten[i] && stdRenderCapture_aGlobalBits[i] == valueBits )
    {
        return true;
    }

    stdRenderCapture_PackU32(aPayload, globalId);
    stdRenderCapture_PackU32(&aPayload[4], valueBits);
    if ( !stdRenderCapture_WriteRecordHeader(STDRENDERCAPTURE_RECORD_GLOBAL, sizeof(aPayload))
        || !stdRenderCapture_WriteData(aPayload, sizeof(aPayload)) )
    {
        stdRenderCapture_FailAndClose("could not write a GLOBAL record.");
        return false;
    }

    stdRenderCapture_aGlobalBits[i]    = valueBits;
    stdRenderCapture_aGlobalWritten[i] = true;
    return true;
}

uint32_t stdRenderCapture_FindHandle(StdRenderCaptureHandleKind kind, const void* p)
{
    size_t i;

    if ( !stdRenderCapture_bOpen || !p )
    {
        return 0;
    }

    if ( !stdRenderCapture_IsValidKind(kind) )
    {
        STDLOG_ERROR("Render capture: invalid handle kind %d.\n", (int)kind);
        return 0;
    }

    for ( i = 0; i < stdRenderCapture_aHandleCounts[kind]; ++i )
    {
        if ( stdRenderCapture_apHandleEntries[kind][i].pPtr == p )
        {
            return stdRenderCapture_apHandleEntries[kind][i].handle;
        }
    }

    return 0;
}

uint32_t stdRenderCapture_AddHandle(StdRenderCaptureHandleKind kind, const void* p)
{
    StdRenderCaptureHandleEntry* pEntries;
    size_t count;
    size_t i;

    if ( !stdRenderCapture_bOpen || !p )
    {
        return 0;
    }

    if ( !stdRenderCapture_IsValidKind(kind) )
    {
        STDLOG_ERROR("Render capture: invalid handle kind %d.\n", (int)kind);
        return 0;
    }

    count = stdRenderCapture_aHandleCounts[kind];
    for ( i = 0; i < count; ++i )
    {
        if ( stdRenderCapture_apHandleEntries[kind][i].pPtr == p )
        {
            return stdRenderCapture_apHandleEntries[kind][i].handle;
        }
    }

    pEntries = stdRenderCapture_apHandleEntries[kind];
    if ( count == stdRenderCapture_aHandleCapacities[kind] )
    {
        size_t capacity = stdRenderCapture_aHandleCapacities[kind];

        if ( capacity > SIZE_MAX / 2u || capacity * 2u > SIZE_MAX / sizeof(StdRenderCaptureHandleEntry) )
        {
            STDLOG_ERROR("Render capture: too many handles for one capture.\n");
            return 0;
        }

        capacity = capacity ? capacity * 2u : 8u;
        pEntries = (StdRenderCaptureHandleEntry*)STDREALLOC(
            stdRenderCapture_apHandleEntries[kind],
            capacity * sizeof(StdRenderCaptureHandleEntry)
        );
        if ( !pEntries )
        {
            STDLOG_ERROR("Render capture: could not grow the handle table.\n");
            return 0;
        }

        stdRenderCapture_apHandleEntries[kind]   = pEntries;
        stdRenderCapture_aHandleCapacities[kind] = capacity;
    }

    pEntries[count].pPtr   = p;
    pEntries[count].handle = stdRenderCapture_aNextHandles[kind];
    ++stdRenderCapture_aHandleCounts[kind];

    // Handles count up and are never reused within a capture.
    if ( stdRenderCapture_aNextHandles[kind] != UINT32_MAX )
    {
        ++stdRenderCapture_aNextHandles[kind];
    }

    return pEntries[count].handle;
}

// Releasing removes the entry and keeps the assignment order of the remaining live handles, so the
// table only ever grows with the live set; a long capture does not pay for pointers already freed.
void stdRenderCapture_ReleaseHandle(StdRenderCaptureHandleKind kind, const void* p)
{
    StdRenderCaptureHandleEntry* pEntries;
    size_t count;
    size_t i;

    if ( !stdRenderCapture_bOpen || !p )
    {
        return;
    }

    if ( !stdRenderCapture_IsValidKind(kind) )
    {
        STDLOG_ERROR("Render capture: invalid handle kind %d.\n", (int)kind);
        return;
    }

    pEntries = stdRenderCapture_apHandleEntries[kind];
    count = stdRenderCapture_aHandleCounts[kind];
    for ( i = 0; i < count; ++i )
    {
        if ( pEntries[i].pPtr == p )
        {
            memmove(&pEntries[i], &pEntries[i + 1], ( count - i - 1u ) * sizeof(*pEntries));
            stdRenderCapture_aHandleCounts[kind] = count - 1u;
            return;
        }
    }
}

// The live table is kept in assignment order (ReleaseHandle moves the later entries down), so the
// first match is the first-assigned match.
uint32_t stdRenderCapture_FindHandleWhere(
    StdRenderCaptureHandleKind kind,
    bool (*pfMatch)(const void* p, const void* pContext),
    const void* pContext
)
{
    size_t i;

    if ( !stdRenderCapture_bOpen || !pfMatch )
    {
        return 0u;
    }

    if ( !stdRenderCapture_IsValidKind(kind) )
    {
        STDLOG_ERROR("Render capture: invalid handle kind %d.\n", (int)kind);
        return 0u;
    }

    for ( i = 0; i < stdRenderCapture_aHandleCounts[kind]; ++i )
    {
        if ( pfMatch(stdRenderCapture_apHandleEntries[kind][i].pPtr, pContext) )
        {
            return stdRenderCapture_apHandleEntries[kind][i].handle;
        }
    }

    return 0u;
}
