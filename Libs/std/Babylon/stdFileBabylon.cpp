// Added: the Babylon column's stdFile host file functions on babylon::files::FileSystem, replacing
// General/stdFile.c there. Same functions, signatures and observable behavior as the C runtime
// version: the handle is a pointer to the file-private struct below, 0 means "no file", reads are
// served from an 8 KiB buffer, and a failing operation returns what the C runtime returns. Adapter
// code: no Fixed/Added markers inside; the file uses Babylon headers and the standard C++ library
// only.
#include <babylon/files/FileSystem.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <span>

#include <std/General/std.h>

#include "stdBabylon.h"
#include "stdBabylonPath.h"

namespace
{
    // The size of the read-ahead buffer one handle fills.
    constexpr size_t kReadBufferSize = 8192;

    // The file-private data behind a tFileHandle, created by stdFileOpen and deleted by
    // stdFileClose. 0 means "no file".
    struct sStdFileHandle
    {
        std::unique_ptr<babylon::files::File> pFile; // the open Babylon file, never null
        bool bReading;                               // opened for reading ("r" mode)
        bool bEof;                                   // a File::read returned 0 too early
        std::byte aBuffer[kReadBufferSize];          // the read-ahead buffer
        size_t curBuffer;                            // next buffered byte to hand out
        size_t numBuffer;                            // valid bytes in the buffer
    };

    sStdFileHandle* stdFileBabylon_AsHandle(tFileHandle fh)
    {
        return reinterpret_cast<sStdFileHandle*>(fh);
    }

    // Makes sure the buffer holds at least one byte, reading a buffer's worth from the file.
    // Returns false at the end of the file (which sets the EOF flag) or after a failed read
    // (which leaves it alone; the interface logged it).
    bool stdFileBabylon_Refill(sStdFileHandle* pHandle)
    {
        if ( pHandle->curBuffer < pHandle->numBuffer )
        {
            return true;
        }

        const std::optional<std::size_t> numRead =
            pHandle->pFile->read(std::span(pHandle->aBuffer, kReadBufferSize));
        pHandle->curBuffer = 0;
        pHandle->numBuffer = numRead.value_or(0);
        if ( !numRead )
        {
            return false;
        }

        if ( pHandle->numBuffer == 0 )
        {
            pHandle->bEof = true;
            return false;
        }

        return true;
    }

    // The logical position: the file position minus the buffered bytes not handed out yet, or
    // SIZE_MAX when the file cannot report it.
    uint64_t stdFileBabylon_LogicalPosition(const sStdFileHandle* pHandle)
    {
        const std::optional<std::int64_t> curPosition = pHandle->pFile->tell();
        if ( !curPosition )
        {
            return UINT64_MAX;
        }

        return static_cast<uint64_t>(*curPosition) - (pHandle->numBuffer - pHandle->curBuffer);
    }
}

extern "C" {

tFileHandle J3DAPI stdFileOpen(const char* pFilename, const char* mode)
{
    if ( !pFilename || !mode )
    {
        return 0;
    }

    babylon::files::OpenMode eMode;
    bool                     bReading;
    switch ( mode[0] )
    {
        case 'r':
            eMode    = babylon::files::OpenMode::Read;
            bReading = true;
            break;
        case 'w':
            eMode    = babylon::files::OpenMode::Write;
            bReading = false;
            break;
        case 'a':
            eMode    = babylon::files::OpenMode::Append;
            bReading = false;
            break;
        default:
            return 0;
    }

    // The remaining mode characters ('b', 't', '+') are ignored: no engine caller reads back a
    // "+" file; a read on a Write or Append handle returns 0.
    babylon::files::FileSystem& rFileSystem = *stdBabylon_RequireContext("stdFileOpen").pFileSystem;

    std::string aResolved;
    if ( !stdBabylonPath_Resolve(rFileSystem, pFilename, aResolved) )
    {
        return 0;
    }

    // Probing a missing file is normal engine behavior and must not log, while the interface
    // promises a warning for a Read open that fails, so check existence first.
    if ( bReading && !rFileSystem.exists(aResolved) )
    {
        return 0;
    }

    std::unique_ptr<babylon::files::File> pFile = rFileSystem.open(aResolved, eMode);
    if ( !pFile )
    {
        return 0;
    }

    sStdFileHandle* pHandle = new sStdFileHandle;
    pHandle->pFile      = std::move(pFile);
    pHandle->bReading   = bReading;
    pHandle->bEof       = false;
    pHandle->curBuffer  = 0;
    pHandle->numBuffer  = 0;
    return reinterpret_cast<tFileHandle>(pHandle);
}

int J3DAPI stdFileClose(tFileHandle fh)
{
    // delete of 0 is a no-op, as the C runtime's own close of a failed handle never runs.
    delete stdFileBabylon_AsHandle(fh);
    return 0;
}

size_t J3DAPI stdFileRead(tFileHandle fh, void* pOutData, size_t size)
{
    sStdFileHandle* pHandle = stdFileBabylon_AsHandle(fh);
    if ( !pHandle || !pHandle->bReading )
    {
        return 0;
    }

    std::byte* pDst = static_cast<std::byte*>(pOutData);
    size_t     numDelivered = 0;
    while ( numDelivered < size )
    {
        if ( pHandle->curBuffer < pHandle->numBuffer )
        {
            const size_t numUnread = pHandle->numBuffer - pHandle->curBuffer;
            const size_t numCopy   = ( numUnread < (size - numDelivered) ) ? numUnread : (size - numDelivered);
            std::memcpy(pDst + numDelivered, pHandle->aBuffer + pHandle->curBuffer, numCopy);
            pHandle->curBuffer += numCopy;
            numDelivered       += numCopy;
            continue;
        }

        // When the remaining request is at least the buffer size, read straight into the
        // caller's buffer instead of through the read-ahead buffer.
        const size_t numRemaining = size - numDelivered;
        if ( numRemaining >= kReadBufferSize )
        {
            const std::optional<std::size_t> numRead =
                pHandle->pFile->read(std::span(pDst + numDelivered, numRemaining));
            if ( !numRead )
            {
                break; // a failed read ends the call with what was delivered so far
            }

            if ( *numRead == 0 )
            {
                pHandle->bEof = true;
                break;
            }

            numDelivered += *numRead;
            continue;
        }

        if ( !stdFileBabylon_Refill(pHandle) )
        {
            break;
        }
    }

    return numDelivered;
}

size_t J3DAPI stdFileWrite(tFileHandle fh, const void* pData, size_t size)
{
    sStdFileHandle* pHandle = stdFileBabylon_AsHandle(fh);
    if ( !pHandle || pHandle->bReading )
    {
        return 0;
    }

    const bool bWritten = pHandle->pFile->write(std::span(static_cast<const std::byte*>(pData), size));
    return ( bWritten ) ? size : 0;
}

int J3DAPI stdFileFlush(tFileHandle fh)
{
    static_cast<void>(fh); // writes are not buffered here; everything already reached the file
    return 0;
}

char* J3DAPI stdFileGets(tFileHandle fh, char* pStr, size_t size)
{
    sStdFileHandle* pHandle = stdFileBabylon_AsHandle(fh);
    if ( !pHandle || !pHandle->bReading || size == 0 )
    {
        return nullptr;
    }

    if ( size == 1 )
    {
        pStr[0] = '\0';
        return pStr;
    }

    size_t curChar = 0;
    while ( curChar + 1 < size )
    {
        if ( !stdFileBabylon_Refill(pHandle) )
        {
            break;
        }

        const char ch = static_cast<char>(pHandle->aBuffer[pHandle->curBuffer++]);
        pStr[curChar] = ch;
        ++curChar;
        if ( ch == '\n' )
        {
            break;
        }
    }

    if ( curChar == 0 )
    {
        // Nothing read at the end of the file: the refill already set the EOF flag, a failed
        // read left it alone. Either way fgets returns NULL.
        return nullptr;
    }

    pStr[curChar] = '\0';
    return pStr;
}

wchar_t* J3DAPI stdFileGetws(tFileHandle fh, wchar_t* pOutStr, size_t size)
{
    // fgetws in the "C" locale, the way glibc does it for the Null column: every byte below 0x80
    // is one character; a byte of 0x80 or more is not a valid wide character there and fails.
    // No engine code calls this function; pFileGetws is only stored.
    sStdFileHandle* pHandle = stdFileBabylon_AsHandle(fh);
    if ( !pHandle || !pHandle->bReading || size == 0 )
    {
        return nullptr;
    }

    if ( size == 1 )
    {
        pOutStr[0] = 0;
        return pOutStr;
    }

    size_t curChar = 0;
    while ( curChar + 1 < size )
    {
        if ( !stdFileBabylon_Refill(pHandle) )
        {
            break;
        }

        const unsigned ch = static_cast<unsigned>(pHandle->aBuffer[pHandle->curBuffer++]);
        if ( ch >= 0x80u )
        {
            return nullptr;
        }

        pOutStr[curChar] = static_cast<wchar_t>(ch);
        ++curChar;
        if ( ch == static_cast<unsigned>('\n') )
        {
            break;
        }
    }

    if ( curChar == 0 )
    {
        return nullptr;
    }

    pOutStr[curChar] = 0;
    return pOutStr;
}

int J3DAPI stdFileEof(tFileHandle fh)
{
    const sStdFileHandle* pHandle = stdFileBabylon_AsHandle(fh);
    return ( pHandle && pHandle->bEof ) ? 1 : 0;
}

int J3DAPI stdFileTell(tFileHandle fh)
{
    const sStdFileHandle* pHandle = stdFileBabylon_AsHandle(fh);
    if ( !pHandle )
    {
        return -1;
    }

    const uint64_t curPosition = stdFileBabylon_LogicalPosition(pHandle);
    if ( curPosition == UINT64_MAX )
    {
        return -1;
    }

    return static_cast<int>(curPosition);
}

int J3DAPI stdFileSeek(tFileHandle fh, int offset, int origin)
{
    sStdFileHandle* pHandle = stdFileBabylon_AsHandle(fh);
    if ( !pHandle )
    {
        return -1;
    }

    // A failing seek restores the position from here, so take the logical position first and
    // fail with -1 when the file cannot report it.
    const uint64_t curLogical = stdFileBabylon_LogicalPosition(pHandle);
    if ( curLogical == UINT64_MAX )
    {
        return -1;
    }

    babylon::files::SeekOrigin eOrigin;
    std::int64_t               aOffset = offset;
    switch ( origin )
    {
        case 0: // SEEK_SET
            eOrigin = babylon::files::SeekOrigin::Begin;
            break;
        case 1: // SEEK_CUR: counted from the logical position, not the buffered file position.
            aOffset = static_cast<std::int64_t>(curLogical) + offset;
            eOrigin = babylon::files::SeekOrigin::Begin;
            break;
        case 2: // SEEK_END
            eOrigin = babylon::files::SeekOrigin::End;
            break;
        default:
            return -1;
    }

    // The buffered bytes are past: the position now comes from the file alone.
    pHandle->curBuffer = 0;
    pHandle->numBuffer = 0;

    // Note: fseek can move a read file past the end and the next read then returns 0; the
    // Babylon interface refuses that. Seek the file back to the logical position, so the
    // reader continues where it was before the failed call, and return -1.
    if ( !pHandle->pFile->seek(aOffset, eOrigin) )
    {
        static_cast<void>(pHandle->pFile->seek(static_cast<std::int64_t>(curLogical),
            babylon::files::SeekOrigin::Begin));
        return -1;
    }

    pHandle->bEof = false;
    return 0;
}

size_t J3DAPI stdFileSize(const char* pFilePath)
{
    tFileHandle fh = stdFileOpen(pFilePath, "rb");
    if ( !fh )
    {
        return 0;
    }

    stdFileSeek(fh, 0, 2);
    int size = stdFileTell(fh);
    stdFileClose(fh);
    return size;
}

} // extern "C"
