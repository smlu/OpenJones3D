// Added: the Babylon column's engine-path resolver: the babylon::files::FileSystem equivalent of
// j3dcrt_ResolvePath for the Null column. It turns an engine path (Windows style: backslashes, any
// case, relative to the working directory) into the absolute '/' path of the real file system
// entry. Adapter code: no Fixed/Added markers inside; ASCII case folding only, no locale and no
// POSIX headers.
#include "stdBabylonPath.h"

#include <babylon/files/Path.h>

#include <cstddef>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    // ASCII case folding only: 'A'..'Z' to 'a'..'z', everything else unchanged.
    char stdBabylonPath_FoldAscii(char ch)
    {
        return ( ch >= 'A' && ch <= 'Z' ) ? static_cast<char>(ch - 'A' + 'a') : ch;
    }

    // ASCII-case-insensitive name order with the byte order (unsigned) breaking ties, the same
    // pick as j3dcrt_FindPathEntry: returns < 0 when left sorts before right, 0 when equal.
    int stdBabylonPath_CompareNames(std::string_view aLeft, std::string_view aRight)
    {
        const size_t numCompared = ( aLeft.size() < aRight.size() ) ? aLeft.size() : aRight.size();
        for ( size_t cur = 0; cur < numCompared; ++cur )
        {
            const unsigned char chLeft  = static_cast<unsigned char>(stdBabylonPath_FoldAscii(aLeft[cur]));
            const unsigned char chRight = static_cast<unsigned char>(stdBabylonPath_FoldAscii(aRight[cur]));
            if ( chLeft != chRight )
            {
                return ( chLeft < chRight ) ? -1 : 1;
            }
        }

        if ( aLeft.size() != aRight.size() )
        {
            return ( aLeft.size() < aRight.size() ) ? -1 : 1;
        }

        return aLeft.compare(aRight);
    }

    // True when the two names are equal ignoring ASCII case.
    bool stdBabylonPath_EqualsFolded(std::string_view aLeft, std::string_view aRight)
    {
        if ( aLeft.size() != aRight.size() )
        {
            return false;
        }

        for ( size_t cur = 0; cur < aLeft.size(); ++cur )
        {
            if ( stdBabylonPath_FoldAscii(aLeft[cur]) != stdBabylonPath_FoldAscii(aRight[cur]) )
            {
                return false;
            }
        }

        return true;
    }
}

bool stdBabylonPath_Resolve(babylon::files::FileSystem& fs, const char* pPath, std::string& out)
{
    if ( !pPath || pPath[0] == '\0' )
    {
        return false;
    }

    // A path that starts with '/' is absolute. A path that starts with '\' is relative (j3dcrt:
    // there is no drive to root it against); everything else is relative too. The decision comes
    // before the copy, which turns every '\' into '/'.
    const bool bAbsolute = ( pPath[0] == '/' );

    std::string aNormalized;
    aNormalized.reserve(std::strlen(pPath) + 1);
    if ( !bAbsolute )
    {
        const std::string aCurrentPath = fs.currentPath();
        if ( aCurrentPath.empty() )
        {
            return false;
        }

        aNormalized = aCurrentPath;
    }

    for ( const char* p = pPath; *p != '\0'; ++p )
    {
        aNormalized.push_back( ( *p == '\\' ) ? '/' : *p );
    }

    // Split on '/': drop empty and "." segments, let ".." remove the previous segment, and drop
    // it at the root. Remember whether the input ended with a separator.
    const bool bTrailingSeparator = ( !aNormalized.empty() && aNormalized.back() == '/' );
    std::vector<std::string> aSegments;
    size_t cur = 0;
    while ( cur < aNormalized.size() )
    {
        size_t curNext = aNormalized.find('/', cur);
        if ( curNext == std::string::npos )
        {
            curNext = aNormalized.size();
        }

        const std::string_view aSegment(aNormalized.data() + cur, curNext - cur);
        if ( aSegment.empty() || aSegment == "." )
        {
            // Nothing: empty and "." segments are dropped.
        }
        else if ( aSegment == ".." )
        {
            if ( !aSegments.empty() )
            {
                aSegments.pop_back();
            }
        }
        else
        {
            aSegments.emplace_back(aSegment);
        }

        cur = curNext + 1;
    }

    // Joins the segments into the absolute result. A helper keeps out unchanged until a valid
    // path has been built.
    const auto joinSegments = [](const std::vector<std::string>& aJoinSegments)
    {
        std::string aJoined = "/";
        for ( size_t curJoin = 0; curJoin < aJoinSegments.size(); ++curJoin )
        {
            if ( curJoin != 0 )
            {
                aJoined += "/";
            }

            aJoined += aJoinSegments[curJoin];
        }

        return aJoined;
    };

    // Fast path: when the whole normalized path already exists (as a file or as a directory), it
    // is already exact: the walk below would keep every segment, so the two exist() probes stand
    // in for one pair per segment. The engine mostly opens existing files by their real path.
    const std::string aFull = joinSegments(aSegments);
    if ( fs.exists(aFull) || fs.exists(aFull + "/") )
    {
        std::string aFastResult = aFull;
        if ( bTrailingSeparator && !aSegments.empty() )
        {
            aFastResult += "/";
        }

        out = std::move(aFastResult);
        return true;
    }

    // Case: walk the segments from the root. A segment that exists with the case it is written in
    // is kept; otherwise the parent directory (the root for the first segment) is listed and the
    // smallest name that is ASCII-case-insensitively equal replaces it. Without a match, or when
    // the listing fails, this and all later segments stay as given.
    std::string aResolved;
    for ( size_t curSegment = 0; curSegment < aSegments.size(); ++curSegment )
    {
        const std::string aPrefix = aResolved + "/" + aSegments[curSegment];
        if ( fs.exists(aPrefix) || fs.exists(aPrefix + "/") )
        {
            aResolved = aPrefix;
            continue;
        }

        const std::optional<std::vector<babylon::files::DirectoryEntry>> aListing =
            fs.listDirectory(aResolved.empty() ? std::string_view("/") : std::string_view(aResolved));
        if ( !aListing )
        {
            break;
        }

        bool           bFound  = false;
        std::string_view aPick;
        for ( const babylon::files::DirectoryEntry& rEntry : *aListing )
        {
            if ( !stdBabylonPath_EqualsFolded(rEntry.name, aSegments[curSegment]) )
            {
                continue;
            }

            if ( !bFound || stdBabylonPath_CompareNames(rEntry.name, aPick) < 0 )
            {
                aPick  = rEntry.name;
                bFound = true;
            }
        }

        if ( !bFound )
        {
            break;
        }

        aSegments[curSegment].assign(aPick);
        aResolved += "/";
        aResolved += aPick;
    }

    std::string aResult = joinSegments(aSegments);
    if ( bTrailingSeparator && !aSegments.empty() )
    {
        aResult += "/";
    }

    if ( !babylon::files::isValidPath(aResult) )
    {
        return false; // out stays unchanged
    }

    out = std::move(aResult);
    return true;
}
