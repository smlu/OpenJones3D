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
    size_t numTrusted = 0;
    if ( !bAbsolute )
    {
        const std::string aCurrentPath = fs.currentPath();
        if ( aCurrentPath.empty() )
        {
            return false;
        }

        aNormalized = aCurrentPath;

        // The current path's segments are real directory names, so they exist with the case the
        // backend reports them in and the walk below must not probe them. Count them with the
        // same normalization the split uses: a valid path has no '.' or '..' segment and no
        // empty segment except the leading one.
        size_t curTrusted = 0;
        while ( curTrusted < aCurrentPath.size() )
        {
            size_t curEnd = aCurrentPath.find('/', curTrusted);
            if ( curEnd == std::string::npos )
            {
                curEnd = aCurrentPath.size();
            }

            if ( curEnd != curTrusted )
            {
                ++numTrusted;
            }

            curTrusted = curEnd + 1;
        }
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

            // A ".." that pops into the current path's segments leaves fewer of them trusted.
            numTrusted = ( numTrusted < aSegments.size() ) ? numTrusted : aSegments.size();
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
    // Skipped when every segment is trusted: then the path is the current directory itself and
    // there is nothing to check.
    if ( numTrusted < aSegments.size() )
    {
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

            // Case: walk the segments from the first untrusted one. A segment that exists with the
            // case it is written in is kept; otherwise the parent directory is listed and the
            // smallest name that is ASCII-case-insensitively equal replaces it. Without a match, or
            // when the listing fails, this and all later segments stay as given. The trusted leading
            // segments are the backend's own current path: real names, not probed. listNames() is
            // enough here: only the names decide the pick, no entry metadata.
            std::string aResolved;
        for ( size_t curTrustedSegment = 0; curTrustedSegment < numTrusted; ++curTrustedSegment )
        {
            aResolved += "/";
            aResolved += aSegments[curTrustedSegment];
        }

        for ( size_t curSegment = numTrusted; curSegment < aSegments.size(); ++curSegment )
        {
            const std::string aPrefix = aResolved + "/" + aSegments[curSegment];
            if ( fs.exists(aPrefix) || fs.exists(aPrefix + "/") )
            {
                aResolved = aPrefix;
                continue;
            }

            const std::optional<std::vector<std::string>> aListing =
                fs.listNames(aResolved.empty() ? std::string_view("/") : std::string_view(aResolved));
            if ( !aListing )
            {
                break;
            }

            bool             bFound = false;
            std::string_view aPick;
            for ( const std::string& aName : *aListing )
            {
                if ( !stdBabylonPath_EqualsFolded(aName, aSegments[curSegment]) )
                {
                    continue;
                }

                if ( !bFound || stdBabylonPath_CompareNames(aName, aPick) < 0 )
                {
                    aPick  = aName;
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
