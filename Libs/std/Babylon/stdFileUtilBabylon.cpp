// Added: the Babylon column's Null platform file utilities on babylon::files::FileSystem,
// replacing Null/stdFileUtilPosix.c there. Same header contracts as the POSIX file: the directory
// search reports one entry per stdFileUtil_FindNext call and the directory and file operations
// return nonzero on success and 0 on failure. Paths are resolved first, because the engine writes
// them the Windows way: case-insensitive and with backslash separators. Adapter code: no
// Fixed/Added markers inside; Babylon headers and the standard C++ library only.
#include <babylon/files/FileSystem.h>
#include <babylon/files/Path.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <std/General/stdFileUtil.h>
#include <std/General/stdMemory.h>

#include "stdBabylon.h"
#include "stdBabylonPath.h"

namespace
{
    // The longest directory entry name a search reports (the NAME_MAX plus the terminator the
    // POSIX file skipped longer names at); longer names are skipped.
    constexpr size_t kMaxNameLength = 256;

    // ASCII case folding only: 'A'..'Z' to 'a'..'z', everything else unchanged.
    char stdFileUtilBabylon_FoldAscii(char ch)
    {
        return ( ch >= 'A' && ch <= 'Z' ) ? static_cast<char>(ch - 'A' + 'a') : ch;
    }

    bool stdFileUtilBabylon_CharsEqualFolded(unsigned char chLeft, unsigned char chRight)
    {
        return stdFileUtilBabylon_FoldAscii(static_cast<char>(chLeft)) ==
            stdFileUtilBabylon_FoldAscii(static_cast<char>(chRight));
    }

    // Case-insensitive name order with the byte order (unsigned) breaking ties, so the listing
    // does not depend on the order the directory happens to be read in. Returns < 0 when the
    // left name sorts first.
    int stdFileUtilBabylon_CompareEntries(std::string_view aLeft, std::string_view aRight)
    {
        const size_t numCompared = ( aLeft.size() < aRight.size() ) ? aLeft.size() : aRight.size();
        for ( size_t cur = 0; cur < numCompared; ++cur )
        {
            const unsigned char chLeft  = static_cast<unsigned char>(aLeft[cur]);
            const unsigned char chRight = static_cast<unsigned char>(aRight[cur]);
            if ( !stdFileUtilBabylon_CharsEqualFolded(chLeft, chRight) )
            {
                return stdFileUtilBabylon_FoldAscii(static_cast<char>(chLeft)) <
                    stdFileUtilBabylon_FoldAscii(static_cast<char>(chRight)) ? -1 : 1;
            }
        }

        if ( aLeft.size() != aRight.size() )
        {
            return ( aLeft.size() < aRight.size() ) ? -1 : 1;
        }

        return aLeft.compare(aRight);
    }

    // Whether a directory entry name matches the search mask, ASCII-case-insensitively: '*'
    // stands for any run of bytes and '?' for one byte; every other byte compares as itself.
    // Windows matches "*.*" against every name, including names without any dot, so it is
    // searched as "*". A small matcher, no fnmatch and no POSIX headers in this adapter.
    bool stdFileUtilBabylon_MaskMatches(std::string_view aName, std::string_view aMask)
    {
        if ( aMask == "*.*" )
        {
            aMask = "*";
        }

        size_t curName = 0;
        size_t curMask = 0;
        size_t curStar      = std::string_view::npos;
        size_t curNameAfter = 0;
        while ( curName < aName.size() )
        {
            if ( curMask < aMask.size() &&
                ( aMask[curMask] == '?' ||
                stdFileUtilBabylon_CharsEqualFolded(static_cast<unsigned char>(aName[curName]),
                    static_cast<unsigned char>(aMask[curMask])) ) )
            {
                ++curName;
                ++curMask;
            }
            else if ( curMask < aMask.size() && aMask[curMask] == '*' )
            {
                curStar      = curMask;
                curNameAfter = curName;
                ++curMask;
            }
            else if ( curStar != std::string_view::npos )
            {
                curMask = curStar + 1;
                curName = ++curNameAfter;
            }
            else
            {
                return false;
            }
        }

        while ( curMask < aMask.size() && aMask[curMask] == '*' )
        {
            ++curMask;
        }

        return curMask == aMask.size();
    }

    struct sStdFileUtilFindEntry
    {
        std::string aName;
        bool        bIsDirectory;
        std::int64_t modifiedSeconds;
    };

    // The search state stdFileUtil_FindNext keeps in FindFileData::handle, the equivalent of the
    // Windows find handle that FindNextFile walks and FindClose releases.
    struct sStdFileUtilFindCursor
    {
        size_t                             curEntry;
        std::vector<sStdFileUtilFindEntry> aEntries;
    };

    // Whether a directory exists at the resolved, separator-free path (a '/' probe answers the
    // directory question; the path itself would also match a file there).
    bool stdFileUtilBabylon_DirectoryExistsAt(babylon::files::FileSystem& rFileSystem, const std::string& aPath)
    {
        return rFileSystem.exists( ( aPath == "/" ) ? std::string_view("/") : std::string_view(aPath + "/") );
    }

    // Copies one collected entry into the caller's result, as the Windows version copies the
    // find data.
    void stdFileUtilBabylon_FillInfo(tFoundFileInfo* pFileInfo, const sStdFileUtilFindEntry& rEntry)
    {
        std::strncpy(pFileInfo->aName, rEntry.aName.c_str(), sizeof(pFileInfo->aName) - 1);
        pFileInfo->aName[sizeof(pFileInfo->aName) - 1] = '\0';

        // Keep the original 32-bit field conversion explicit, as the Posix file does on
        // st_mtime: seconds since the epoch truncated into the field the engine carries.
        pFileInfo->lastChanged  = static_cast<uint32_t>(rEntry.modifiedSeconds);
        pFileInfo->bIsDirectory = ( rEntry.bIsDirectory ) ? 1 : 0;
    }

    // Reads the directory part of pFilter (a "dir/mask" search path), keeps every entry whose
    // name matches the mask, "." and ".." included, and sorts them. Returns nullptr when the
    // directory is missing or holds no match, so the caller reports the search as failed with
    // nothing to release.
    sStdFileUtilFindCursor* stdFileUtilBabylon_NewCursor(babylon::files::FileSystem& rFileSystem, const char* pFilter)
    {
        std::string aResolved;
        if ( !stdBabylonPath_Resolve(rFileSystem, pFilter, aResolved) )
        {
            return nullptr;
        }

        // Split the resolved path into the directory to read and the mask to match its names
        // against. The resolved path is absolute, so it always has a '/'; an empty directory part
        // is the root.
        const size_t curSeparator   = aResolved.rfind('/');
        const size_t numDirectory   = curSeparator;
        const std::string_view aMask(aResolved.data() + curSeparator + 1, aResolved.size() - curSeparator - 1);

        // The POSIX listing reports "." and ".."; the Babylon listing promises to omit them, so
        // the search adds them back as directories to report the same entries. The listed
        // directory's own modification time is not available through the listing, so these
        // report 0.
        std::vector<sStdFileUtilFindEntry> aEntries;
        aEntries.push_back({".", true, 0});
        aEntries.push_back({"..", true, 0});

        const std::optional<std::vector<babylon::files::DirectoryEntry>> aListing =
            rFileSystem.listDirectory( ( numDirectory == 0 ) ? std::string_view("/") : std::string_view(aResolved.substr(0, numDirectory)) );
        if ( !aListing )
        {
            return nullptr;
        }

        for ( const babylon::files::DirectoryEntry& rEntry : *aListing )
        {
            aEntries.push_back({rEntry.name, rEntry.isDirectory, rEntry.modifiedSeconds});
        }

        // Keep the names that match the mask and drop the ones a find-data name buffer cannot
        // hold.
        std::vector<sStdFileUtilFindEntry> aMatches;
        for ( sStdFileUtilFindEntry& rEntry : aEntries )
        {
            if ( rEntry.aName.size() >= kMaxNameLength || !stdFileUtilBabylon_MaskMatches(rEntry.aName, aMask) )
            {
                continue;
            }

            aMatches.push_back(std::move(rEntry));
        }

        if ( aMatches.empty() )
        {
            return nullptr;
        }

        std::sort(aMatches.begin(), aMatches.end(),
            [](const sStdFileUtilFindEntry& rLeft, const sStdFileUtilFindEntry& rRight)
            {
                return stdFileUtilBabylon_CompareEntries(rLeft.aName, rRight.aName) < 0;
            });

        sStdFileUtilFindCursor* pCursor = new sStdFileUtilFindCursor;
        pCursor->curEntry = 0;
        pCursor->aEntries = std::move(aMatches);
        return pCursor;
    }

}

extern "C" {

void J3DAPI stdFileUtil_DisposeFind(FindFileData* ffData)
{
    if ( ffData )
    {
        if ( ffData->nFoundFiles && ffData->handle != NULL )
        {
            // The collected listing is the data the Windows FindClose releases.
            sStdFileUtilFindCursor* pCursor = reinterpret_cast<sStdFileUtilFindCursor*>(ffData->handle);
            delete pCursor;
        }

        stdMemory_Free(ffData);
    }
}

int J3DAPI stdFileUtil_FindNext(FindFileData* ffData, tFoundFileInfo* pFileInfo)
{
    if ( !ffData )
    {
        return 0;
    }

    // Release-build guard before writing search-result metadata.
    STD_GUARD(pFileInfo, 0);

    const int nFoundFiles = ffData->nFoundFiles;

    sStdFileUtilFindCursor* pCursor;
    if ( nFoundFiles )
    {
        pCursor = reinterpret_cast<sStdFileUtilFindCursor*>(ffData->handle);
    }
    else
    {
        // A failed search stores the null handle stdFileUtil_DisposeFind reads as nothing to
        // release, and leaves nFoundFiles alone so the same FindFileData can be searched again.
        babylon::files::FileSystem& rFileSystem =
            *stdBabylon_RequireContext("stdFileUtil_FindNext").pFileSystem;
        pCursor = stdFileUtilBabylon_NewCursor(rFileSystem, ffData->aSearchFilter);
        ffData->handle = reinterpret_cast<tSysHandle>(pCursor);
    }

    if ( !pCursor || pCursor->curEntry >= pCursor->aEntries.size() )
    {
        return 0;
    }

    stdFileUtilBabylon_FillInfo(pFileInfo, pCursor->aEntries[pCursor->curEntry]);
    ++pCursor->curEntry;

    // Count only successful results so a failed first search can be retried safely.
    ffData->nFoundFiles = nFoundFiles + 1;
    return 1;
}

int J3DAPI stdFileUtil_MkDir(const char* pPath)
{
    babylon::files::FileSystem& rFileSystem =
        *stdBabylon_RequireContext("stdFileUtil_MkDir").pFileSystem;

    std::string aResolved;
    if ( !stdBabylonPath_Resolve(rFileSystem, pPath, aResolved) )
    {
        return 0;
    }

    // mkdir does not create parent directories and fails on an existing path, so both are
    // checked before the create; the create itself may make missing parents on a backend, which
    // these checks keep out of the result.
    std::string aPath = aResolved;
    if ( aPath.size() > 1 && aPath.back() == '/' )
    {
        aPath.pop_back();
    }

    if ( stdFileUtilBabylon_DirectoryExistsAt(rFileSystem, aPath) || rFileSystem.exists(aPath) )
    {
        return 0; // something already exists at the path
    }

    const size_t curSeparator = aPath.rfind('/');
    if ( curSeparator == std::string::npos )
    {
        return 0; // the resolved path is absolute, so this cannot happen
    }

    std::string aParent = ( curSeparator == 0 ) ? "/" : aPath.substr(0, curSeparator);
    if ( !stdFileUtilBabylon_DirectoryExistsAt(rFileSystem, aParent) )
    {
        return 0;
    }

    return ( rFileSystem.createDirectory(aPath) ) ? 1 : 0;
}

int J3DAPI stdFileUtil_FileExists(const char* pFilename)
{
    babylon::files::FileSystem& rFileSystem =
        *stdBabylon_RequireContext("stdFileUtil_FileExists").pFileSystem;

    std::string aResolved;
    if ( !stdBabylonPath_Resolve(rFileSystem, pFilename, aResolved) )
    {
        return 0;
    }

    std::string aPath = aResolved;
    if ( aPath.size() > 1 && aPath.back() == '/' )
    {
        aPath.pop_back();
    }

    // A file or a directory at the path answers the question, like the stat of the POSIX file.
    return ( rFileSystem.exists(aPath) || stdFileUtilBabylon_DirectoryExistsAt(rFileSystem, aPath) ) ? 1 : 0;
}

int J3DAPI stdFileUtil_RmDir(const char* pDir)
{
    babylon::files::FileSystem& rFileSystem =
        *stdBabylon_RequireContext("stdFileUtil_RmDir").pFileSystem;

    std::string aResolved;
    if ( !stdBabylonPath_Resolve(rFileSystem, pDir, aResolved) )
    {
        return 0;
    }

    std::string aPath = aResolved;
    if ( aPath.size() > 1 && aPath.back() == '/' )
    {
        aPath.pop_back();
    }

    // rmdir only removes a directory and fails when it is not empty; the remove of the interface
    // carries the second rule, the existence probe the first one.
    if ( !stdFileUtilBabylon_DirectoryExistsAt(rFileSystem, aPath) )
    {
        return 0;
    }

    return ( rFileSystem.remove(aPath) ) ? 1 : 0;
}

int J3DAPI stdFileUtil_DelFile(const char* pFilename)
{
    babylon::files::FileSystem& rFileSystem =
        *stdBabylon_RequireContext("stdFileUtil_DelFile").pFileSystem;

    std::string aResolved;
    if ( !stdBabylonPath_Resolve(rFileSystem, pFilename, aResolved) )
    {
        return 0;
    }

    std::string aPath = aResolved;
    if ( aPath.size() > 1 && aPath.back() == '/' )
    {
        aPath.pop_back();
    }

    // unlink only removes a file, never a directory.
    if ( !rFileSystem.exists(aPath) || stdFileUtilBabylon_DirectoryExistsAt(rFileSystem, aPath) )
    {
        return 0;
    }

    return ( rFileSystem.remove(aPath) ) ? 1 : 0;
}

} // extern "C"
